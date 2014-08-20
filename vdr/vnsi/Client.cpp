/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2007 Chris Tallon
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2010, 2011 Alexander Pipelka
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Client.h"
#include "Server.h"
#include "vnsi/net/RequestPacket.h"
#include "vnsi/net/ResponsePacket.h"
#include "vnsi/net/VNSICommand.h"
#include "vnsi/video/RecPlayer.h"
#include "vnsi/video/Streamer.h"
#include "channels/ChannelManager.h"
#include "channels/ChannelGroup.h"
#include "channels/ChannelFilter.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "epg/Event.h"
#include "epg/Schedules.h"
#include "filesystem/Videodir.h"
#include "recordings/RecordingInfo.h"
#include "recordings/Recordings.h"
#include "recordings/marks/Marks.h"
#include "settings/Settings.h"
#include "timers/Timers.h"
#include "transponders/CountryUtils.h"
#include "transponders/SatelliteUtils.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"

#include <map>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

using namespace PLATFORM;

namespace VDR
{

CMutex cVNSIClient::m_timerLock;

cVNSIClient::cVNSIClient(int fd, unsigned int id, const char *ClientAdr)
{
  m_Id                      = id;
  m_Streamer                = NULL;
  m_isStreaming             = false;
  m_ClientAddress           = ClientAdr;
  m_StatusInterfaceEnabled  = false;
  m_RecPlayer               = NULL;
  m_req                     = NULL;
  m_resp                    = NULL;
  m_loggedIn                = false;
  m_protocolVersion         = 0;

  m_socket.SetHandle(fd);

  CreateThread();
}

cVNSIClient::~cVNSIClient()
{
  dsyslog("%s", __FUNCTION__);
  StopChannelStreaming();
  m_socket.close(); // force closing connection
  StopThread(10000);
  dsyslog("done");
}

void* cVNSIClient::Process(void)
{
  uint32_t channelID;
  uint32_t requestID;
  uint32_t opcode;
  uint32_t dataLength;
  uint8_t* data;

  cTimers::Get().RegisterObserver(this);

  while (!IsStopped())
  {
    if (!m_socket.read((uint8_t*)&channelID, sizeof(uint32_t))) break;
    channelID = ntohl(channelID);

    if (channelID == 1)
    {
      if (!m_socket.read((uint8_t*)&requestID, sizeof(uint32_t), 10000)) break;
      requestID = ntohl(requestID);

      if (!m_socket.read((uint8_t*)&opcode, sizeof(uint32_t), 10000)) break;
      opcode = ntohl(opcode);

      if (!m_socket.read((uint8_t*)&dataLength, sizeof(uint32_t), 10000)) break;
      dataLength = ntohl(dataLength);
      if (dataLength > 200000) // a random sanity limit
      {
        esyslog("dataLength > 200000!");
        break;
      }

      if (dataLength)
      {
        data = (uint8_t*)malloc(dataLength);
        if (!data)
        {
          esyslog("Extra data buffer malloc error");
          break;
        }

        if (!m_socket.read(data, dataLength, 10000))
        {
          esyslog("Could not read data");
          free(data);
          break;
        }
      }
      else
      {
        data = NULL;
      }

      dsyslog("Received channel='%s' (%u), ser=%u, opcode='%s' (%u), edl=%u", ChannelToString(channelID), channelID, requestID, OpcodeToString(opcode), opcode, dataLength);

      if (!m_loggedIn && (opcode != VNSI_LOGIN))
      {
        esyslog("Clients must be logged in before sending commands! Aborting.");
        if (data) free(data);
        break;
      }

      cRequestPacket* req = new cRequestPacket(requestID, opcode, data, dataLength);

      processRequest(req);
    }
    else
    {
      esyslog("Incoming channel number unknown");
      break;
    }
  }

  /* If thread is ended due to closed connection delete a
     possible running stream here */
  StopChannelStreaming();

  cTimers::Get().UnregisterObserver(this);

  return NULL;
}

bool cVNSIClient::StartChannelStreaming(ChannelPtr channel, uint8_t timeshift, uint32_t timeout)
{
  m_Streamer    = new cLiveStreamer(m_Id, timeshift, timeout);
  m_isStreaming = m_Streamer->StreamChannel(channel, &m_socket, m_resp);
  if (!m_isStreaming)
  {
    delete m_Streamer;
    m_Streamer = NULL;
  }
  return m_isStreaming;
}

void cVNSIClient::StopChannelStreaming()
{
  m_isStreaming = false;
  if (m_Streamer)
  {
    delete m_Streamer;
    m_Streamer = NULL;
  }
}

void cVNSIClient::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageTimerAdded:
  case ObservableMessageTimerChanged:
  case ObservableMessageTimerDeleted:
    TimerChange();
    break;
  default:
    break;
  }
}

void cVNSIClient::TimerChange()
{
  CLockObject lock(m_msgLock);

  if (m_StatusInterfaceEnabled)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStatus(VNSI_STATUS_TIMERCHANGE))
    {
      delete resp;
      return;
    }

    resp->finalise();
    m_socket.write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

void cVNSIClient::ChannelChange()
{
  CLockObject lock(m_msgLock);

  if (!m_StatusInterfaceEnabled)
    return;

  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStatus(VNSI_STATUS_CHANNELCHANGE))
  {
    delete resp;
    return;
  }

  resp->finalise();
  m_socket.write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::RecordingsChange()
{
  CLockObject lock(m_msgLock);

  if (!m_StatusInterfaceEnabled)
    return;

  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStatus(VNSI_STATUS_RECORDINGSCHANGE))
  {
    delete resp;
    return;
  }

  resp->finalise();
  m_socket.write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cVNSIClient::EpgChange()
{
  CLockObject lock(m_msgLock);

  if (!m_StatusInterfaceEnabled)
    return;

  cSchedulesLock MutexLock;
  cSchedules *schedules = MutexLock.Get();
  if (!schedules)
    return;

  ScheduleVector updatedSchedules = schedules->GetUpdatedSchedules(m_epgUpdate, VNSIChannelFilter);
  for (ScheduleVector::const_iterator it = updatedSchedules.begin(); it != updatedSchedules.end(); ++it)
  {
    const cChannelID& channelId = (*it)->ChannelID();
    const ChannelPtr channel = cChannelManager::Get().GetByChannelID(channelId);
    isyslog("Trigger EPG update for channel %s, id: %d", channel->Name().c_str(), channelId.Hash());

    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStatus(VNSI_STATUS_EPGCHANGE))
    {
      delete resp;
      return;
    }
    m_epgUpdate[channelId.Hash()] = CDateTime::GetUTCDateTime();
    resp->add_U32(channelId.Hash());
    resp->finalise();
    m_socket.write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

void cVNSIClient::Recording(const cDevice *Device, const char *Name, const char *FileName, bool On)
{
  CLockObject lock(m_msgLock);

  if (m_StatusInterfaceEnabled)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStatus(VNSI_STATUS_RECORDING))
    {
      delete resp;
      return;
    }

    resp->add_U32(Device->Index());
    resp->add_U32(On);
    if (Name)
      resp->add_String(Name);
    else
      resp->add_String("");

    if (FileName)
      resp->add_String(FileName);
    else
      resp->add_String("");

    resp->finalise();
    m_socket.write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

void cVNSIClient::OsdStatusMessage(const char *Message)
{
  CLockObject lock(m_msgLock);

  if (m_StatusInterfaceEnabled && Message)
  {
    /* Ignore this messages */
    if (strcasecmp(Message, tr("Channel not available!")) == 0) return;
    else if (strcasecmp(Message, tr("Delete timer?")) == 0) return;
    else if (strcasecmp(Message, tr("Delete recording?")) == 0) return;
    else if (strcasecmp(Message, tr("Press any key to cancel shutdown")) == 0) return;
    else if (strcasecmp(Message, tr("Press any key to cancel restart")) == 0) return;
    else if (strcasecmp(Message, tr("Editing - shut down anyway?")) == 0) return;
    else if (strcasecmp(Message, tr("Recording - shut down anyway?")) == 0) return;
    else if (strcasecmp(Message, tr("shut down anyway?")) == 0) return;
    else if (strcasecmp(Message, tr("Recording - restart anyway?")) == 0) return;
    else if (strcasecmp(Message, tr("Editing - restart anyway?")) == 0) return;
    else if (strcasecmp(Message, tr("Delete channel?")) == 0) return;
    else if (strcasecmp(Message, tr("Timer still recording - really delete?")) == 0) return;
    else if (strcasecmp(Message, tr("Delete marks information?")) == 0) return;
    else if (strcasecmp(Message, tr("Delete resume information?")) == 0) return;
    else if (strcasecmp(Message, tr("CAM is in use - really reset?")) == 0) return;
    else if (strcasecmp(Message, tr("Really restart?")) == 0) return;
    else if (strcasecmp(Message, tr("Stop recording?")) == 0) return;
    else if (strcasecmp(Message, tr("Cancel editing?")) == 0) return;
    else if (strcasecmp(Message, tr("Cutter already running - Add to cutting queue?")) == 0) return;
    else if (strcasecmp(Message, tr("No index-file found. Creating may take minutes. Create one?")) == 0) return;

    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStatus(VNSI_STATUS_MESSAGE))
    {
      delete resp;
      return;
    }

    resp->add_U32(0);
    resp->add_String(Message);
    resp->finalise();
    m_socket.write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

bool cVNSIClient::processRequest(cRequestPacket* req)
{
  CLockObject lock(m_msgLock);

  m_req = req;
  m_resp = new cResponsePacket();
  if (!m_resp->init(m_req->getRequestID()))
  {
    esyslog("Response packet init fail");
    delete m_resp;
    delete m_req;
    m_resp = NULL;
    m_req = NULL;
    return false;
  }

  bool result = false;
  switch(m_req->getOpCode())
  {
    /** OPCODE 1 - 19: VNSI network functions for general purpose */
    case VNSI_LOGIN:
      result = process_Login();
      break;

    case VNSI_GETTIME:
      result = process_GetTime();
      break;

    case VNSI_ENABLESTATUSINTERFACE:
      result = process_EnableStatusInterface();
      break;

    case VNSI_PING:
      result = process_Ping();
      break;

    case VNSI_GETSETUP:
      result = process_GetSetup();
      break;

    case VNSI_STORESETUP:
      result = process_StoreSetup();
      break;

    /** OPCODE 20 - 39: VNSI network functions for live streaming */
    case VNSI_CHANNELSTREAM_OPEN:
      result = processChannelStream_Open();
      break;

    case VNSI_CHANNELSTREAM_CLOSE:
      result = processChannelStream_Close();
      break;

    case VNSI_CHANNELSTREAM_SEEK:
      result = processChannelStream_Seek();
      break;

    /** OPCODE 40 - 59: VNSI network functions for recording streaming */
    case VNSI_RECSTREAM_OPEN:
      result = processRecStream_Open();
      break;

    case VNSI_RECSTREAM_CLOSE:
      result = processRecStream_Close();
      break;

    case VNSI_RECSTREAM_GETBLOCK:
      result = processRecStream_GetBlock();
      break;

    case VNSI_RECSTREAM_POSTOFRAME:
      result = processRecStream_PositionFromFrameNumber();
      break;

    case VNSI_RECSTREAM_FRAMETOPOS:
      result = processRecStream_FrameNumberFromPosition();
      break;

    case VNSI_RECSTREAM_GETIFRAME:
      result = processRecStream_GetIFrame();
      break;

    case VNSI_RECSTREAM_GETLENGTH:
      result = processRecStream_GetLength();
      break;

    /** OPCODE 60 - 79: VNSI network functions for channel access */
    case VNSI_CHANNELS_GETCOUNT:
      result = processCHANNELS_ChannelsCount();
      break;

    case VNSI_CHANNELS_GETCHANNELS:
      result = processCHANNELS_GetChannels();
      break;

    case VNSI_CHANNELGROUP_GETCOUNT:
      result = processCHANNELS_GroupsCount();
      break;

    case VNSI_CHANNELGROUP_LIST:
      result = processCHANNELS_GroupList();
      break;

    case VNSI_CHANNELGROUP_MEMBERS:
      result = processCHANNELS_GetGroupMembers();
      break;

    case VNSI_CHANNELS_GETCAIDS:
      result = processCHANNELS_GetCaids();
      break;

    case VNSI_CHANNELS_GETWHITELIST:
      result = processCHANNELS_GetWhitelist();
      break;

    case VNSI_CHANNELS_GETBLACKLIST:
      result = processCHANNELS_GetBlacklist();
      break;

    case VNSI_CHANNELS_SETWHITELIST:
      result = processCHANNELS_SetWhitelist();
      break;

    case VNSI_CHANNELS_SETBLACKLIST:
      result = processCHANNELS_SetBlacklist();
      break;

    /** OPCODE 80 - 99: VNSI network functions for timer access */
    case VNSI_TIMER_GETCOUNT:
      result = processTIMER_GetCount();
      break;

    case VNSI_TIMER_GET:
      result = processTIMER_Get();
      break;

    case VNSI_TIMER_GETLIST:
      result = processTIMER_GetList();
      break;

    case VNSI_TIMER_ADD:
      result = processTIMER_Add();
      break;

    case VNSI_TIMER_DELETE:
      result = processTIMER_Delete();
      break;

    case VNSI_TIMER_UPDATE:
      result = processTIMER_Update();
      break;


    /** OPCODE 100 - 119: VNSI network functions for recording access */
    case VNSI_RECORDINGS_DISKSIZE:
      result = processRECORDINGS_GetDiskSpace();
      break;

    case VNSI_RECORDINGS_GETCOUNT:
      result = processRECORDINGS_GetCount();
      break;

    case VNSI_RECORDINGS_GETLIST:
      result = processRECORDINGS_GetList();
      break;

    case VNSI_RECORDINGS_RENAME:
      result = processRECORDINGS_Rename();
      break;

    case VNSI_RECORDINGS_DELETE:
      result = processRECORDINGS_Delete();
      break;

    case VNSI_RECORDINGS_GETEDL:
      result = processRECORDINGS_GetEdl();
      break;

    /** OPCODE 120 - 139: VNSI network functions for epg access and manipulating */
    case VNSI_EPG_GETFORCHANNEL:
      result = processEPG_GetForChannel();
      break;


    /** OPCODE 140 - 159: VNSI network functions for channel scanning */
    case VNSI_SCAN_SUPPORTED:
      result = processSCAN_ScanSupported();
      break;

    case VNSI_SCAN_GETCOUNTRIES:
      result = processSCAN_GetCountries();
      break;

    case VNSI_SCAN_GETSATELLITES:
      result = processSCAN_GetSatellites();
      break;

    case VNSI_SCAN_START:
      result = processSCAN_Start();
      break;

    case VNSI_SCAN_STOP:
      result = processSCAN_Stop();
      break;

    case VNSI_SCAN_PROGRESS:
      result = processSCAN_Progress();
      break;
  }

  delete m_resp;
  m_resp = NULL;

  delete m_req;
  m_req = NULL;

  return result;
}


/** OPCODE 1 - 19: VNSI network functions for general purpose */

bool cVNSIClient::process_Login() /* OPCODE 1 */
{
  if (m_req->getDataLength() <= 4) return false;

  m_protocolVersion      = m_req->extract_U32();
                           m_req->extract_U8();
  const char *clientName = m_req->extract_String();

  isyslog("Welcome client '%s' with protocol version '%u'", clientName, m_protocolVersion);

  // Send the login reply
  time_t timeNow;
  CDateTime now = CDateTime::GetCurrentDateTime();
  now.GetAsTime(timeNow);

  m_resp->add_U32(VNSI_PROTOCOLVERSION);
  m_resp->add_U32(timeNow);
  m_resp->add_S32(CDateTime::GetTimezoneBias().GetHours());
  m_resp->add_String("VDR-Network-Streaming-Interface (VNSI) Server");
  m_resp->add_String(VNSI_SERVER_VERSION);
  m_resp->finalise();

  if (m_protocolVersion != VNSI_PROTOCOLVERSION)
    esyslog("Client '%s' have a not allowed protocol version '%u', terminating client", clientName, m_protocolVersion);
  else
    SetLoggedIn(true);

  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  delete[] clientName;

  return true;
}

bool cVNSIClient::process_GetTime() /* OPCODE 2 */
{
  time_t timeNow;
  CDateTime now = CDateTime::GetCurrentDateTime();
  now.GetAsTime(timeNow);

  m_resp->add_U32(timeNow);
  m_resp->add_S32(CDateTime::GetTimezoneBias().GetHours());
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::process_EnableStatusInterface()
{
  bool enabled = m_req->extract_U8();

  SetStatusInterface(enabled);

  m_resp->add_U32(VNSI_RET_OK);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::process_Ping() /* OPCODE 7 */
{
  m_resp->add_U32(1);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::process_GetSetup() /* OPCODE 8 */
{
  char* name = m_req->extract_String();
  if (!strcasecmp(name, CONFNAME_PMTTIMEOUT))
    m_resp->add_U32(cSettings::Get().m_PmtTimeout);
  else if (!strcasecmp(name, CONFNAME_TIMESHIFT))
    m_resp->add_U32(cSettings::Get().m_TimeshiftMode);
  else if (!strcasecmp(name, CONFNAME_TIMESHIFTBUFFERSIZE))
    m_resp->add_U32(cSettings::Get().m_TimeshiftBufferSize);
  else if (!strcasecmp(name, CONFNAME_TIMESHIFTBUFFERFILESIZE))
    m_resp->add_U32(cSettings::Get().m_TimeshiftBufferFileSize);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::process_StoreSetup() /* OPCODE 9 */
{
  //XXX
//  char* name = m_req->extract_String();
//
//  if (!strcasecmp(name, CONFNAME_PMTTIMEOUT))
//  {
//    int value = m_req->extract_U32();
//    cPluginVNSIServer::StoreSetup(CONFNAME_PMTTIMEOUT, value);
//  }
//  else if (!strcasecmp(name, CONFNAME_TIMESHIFT))
//  {
//    int value = m_req->extract_U32();
//    cPluginVNSIServer::StoreSetup(CONFNAME_TIMESHIFT, value);
//  }
//  else if (!strcasecmp(name, CONFNAME_TIMESHIFTBUFFERSIZE))
//  {
//    int value = m_req->extract_U32();
//    cPluginVNSIServer::StoreSetup(CONFNAME_TIMESHIFTBUFFERSIZE, value);
//  }
//  else if (!strcasecmp(name, CONFNAME_TIMESHIFTBUFFERFILESIZE))
//  {
//    int value = m_req->extract_U32();
//    cPluginVNSIServer::StoreSetup(CONFNAME_TIMESHIFTBUFFERFILESIZE, value);
//  }

  m_resp->add_U32(VNSI_RET_OK);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

/** OPCODE 20 - 39: VNSI network functions for live streaming */

bool cVNSIClient::processChannelStream_Open() /* OPCODE 20 */
{
  uint32_t uid = m_req->extract_U32();
  int32_t priority = m_req->extract_S32(); // Unused
  uint8_t timeshift = m_req->extract_U8();
  uint32_t timeout = m_req->extract_U32();

  if(timeout == 0)
    timeout = cSettings::Get().m_StreamTimeout;

  if (m_isStreaming)
    StopChannelStreaming();

  // try to find channel by uid first
  ChannelPtr channel = cChannelManager::Get().GetByChannelUID(uid);

  if (!channel) {
    esyslog("Can't find channel %08x", uid);
    m_resp->add_U32(VNSI_RET_DATAINVALID);
  }
  else
  {
    if (StartChannelStreaming(channel, timeshift, timeout))
    {
      isyslog("Started streaming of channel %s (timeout %i seconds)", channel->Name().c_str(), timeout);
      // return here without sending the response
      // (was already done in cLiveStreamer::StreamChannel)
      return true;
    }

    dsyslog("Can't stream channel %s", channel->Name().c_str());
    m_resp->add_U32(VNSI_RET_DATALOCKED);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return false;
}

bool cVNSIClient::processChannelStream_Close() /* OPCODE 21 */
{
  if (m_isStreaming)
    StopChannelStreaming();

  return true;
}

bool cVNSIClient::processChannelStream_Seek() /* OPCODE 22 */
{
  uint32_t serial = 0;
  if (m_isStreaming && m_Streamer)
  {
    int64_t time = m_req->extract_S64();
    if (m_Streamer->SeekTime(time, serial))
      m_resp->add_U32(VNSI_RET_OK);
    else
      m_resp->add_U32(VNSI_RET_ERROR);
  }
  else
    m_resp->add_U32(VNSI_RET_ERROR);

  m_resp->add_U32(serial);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

/** OPCODE 40 - 59: VNSI network functions for recording streaming */

bool cVNSIClient::processRecStream_Open() /* OPCODE 40 */
{
  cRecording *recording = NULL;

  uint32_t uid = m_req->extract_U32();
  recording = Recordings.FindByUID(uid);

  if (recording && m_RecPlayer == NULL)
  {
    m_RecPlayer = new cRecPlayer(recording);

    m_resp->add_U32(VNSI_RET_OK);
    m_resp->add_U32(m_RecPlayer->getLengthFrames());
    m_resp->add_U64(m_RecPlayer->getLengthBytes());

    m_resp->add_U8(recording->IsPesRecording());//added for TS
  }
  else
  {
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
    esyslog("%s - unable to start recording !", __FUNCTION__);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processRecStream_Close() /* OPCODE 41 */
{
  if (m_RecPlayer)
  {
    delete m_RecPlayer;
    m_RecPlayer = NULL;
  }

  m_resp->add_U32(VNSI_RET_OK);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRecStream_GetBlock() /* OPCODE 42 */
{
  if (m_isStreaming)
  {
    esyslog("Get block called during live streaming");
    return false;
  }

  if (!m_RecPlayer)
  {
    esyslog("Get block called when no recording open");
    return false;
  }

  uint64_t position  = m_req->extract_U64();
  uint32_t amount    = m_req->extract_U32();

  uint8_t* p = m_resp->reserve(amount);
  uint32_t amountReceived = m_RecPlayer->getBlock(p, position, amount);

  if(amount > amountReceived) m_resp->unreserve(amount - amountReceived);

  if (!amountReceived)
  {
    m_resp->add_U32(0);
    dsyslog("written 4(0) as getblock got 0");
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRecStream_PositionFromFrameNumber() /* OPCODE 43 */
{
  uint64_t retval       = 0;
  uint32_t frameNumber  = m_req->extract_U32();

  if (m_RecPlayer)
    retval = m_RecPlayer->positionFromFrameNumber(frameNumber);

  m_resp->add_U64(retval);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  dsyslog("Wrote posFromFrameNum reply to client");
  return true;
}

bool cVNSIClient::processRecStream_FrameNumberFromPosition() /* OPCODE 44 */
{
  uint32_t retval   = 0;
  uint64_t position = m_req->extract_U64();

  if (m_RecPlayer)
    retval = m_RecPlayer->frameNumberFromPosition(position);

  m_resp->add_U32(retval);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  dsyslog("Wrote frameNumFromPos reply to client");
  return true;
}

bool cVNSIClient::processRecStream_GetIFrame() /* OPCODE 45 */
{
  bool success            = false;
  uint32_t frameNumber    = m_req->extract_U32();
  uint32_t direction      = m_req->extract_U32();
  uint64_t rfilePosition  = 0;
  uint32_t rframeNumber   = 0;
  uint32_t rframeLength   = 0;

  if (m_RecPlayer)
    success = m_RecPlayer->getNextIFrame(frameNumber, direction, &rfilePosition, &rframeNumber, &rframeLength);

  // returns file position, frame number, length
  if (success)
  {
    m_resp->add_U64(rfilePosition);
    m_resp->add_U32(rframeNumber);
    m_resp->add_U32(rframeLength);
  }
  else
  {
    m_resp->add_U32(0);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  dsyslog("Wrote GNIF reply to client %lu %u %u", rfilePosition, rframeNumber, rframeLength);
  return true;
}

bool cVNSIClient::processRecStream_GetLength() /* OPCODE 46 */
{
  uint64_t length = 0;

  if (m_RecPlayer)
  {
    m_RecPlayer->reScan();
    length = m_RecPlayer->getLengthBytes();
  }

  m_resp->add_U64(length);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

/** OPCODE 60 - 79: VNSI network functions for channel access */

bool cVNSIClient::processCHANNELS_ChannelsCount() /* OPCODE 61 */
{
  const size_t count = cChannelManager::Get().ChannelCount();

  m_resp->add_U32(count);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GetChannels() /* OPCODE 63 */
{
  if (m_req->getDataLength() != 5)
  {
    esyslog("Invalid data length received: %u != 5", m_req->getDataLength());
    return false;
  }

  bool radio = m_req->extract_U32();
  bool filter = m_req->extract_U8();

  std::string caids;
  int caid;
  int caid_idx;

  ChannelVector channels = cChannelManager::Get().GetCurrent();
  for (ChannelVector::const_iterator it = channels.begin(); it != channels.end(); ++it)
  {
    const ChannelPtr& channel = *it;
    if (radio != CChannelFilter::IsRadio(channel))
      continue;

    // skip invalid channels
    if (!channel->ID().IsValid())
      continue;

    // check filter
    if (filter && !VNSIChannelFilter.PassFilter(channel))
      continue;

    m_resp->add_U32(channel->Number());
    m_resp->add_String(m_toUTF8.Convert(channel->Name().c_str()));
    m_resp->add_String(m_toUTF8.Convert(channel->Provider().c_str()));
    m_resp->add_U32(channel->UID());
    m_resp->add_U32(channel->GetCaId(0));
    caid_idx = 0;
    caids = "caids:";
    while((caid = channel->GetCaId(caid_idx)) != 0)
    {
      caids.append(StringUtils::Format("%d;", caid));
      caid_idx++;
    }
    m_resp->add_String(caids);
    m_resp->add_U32(channel->SubNumber());
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processCHANNELS_GroupsCount()
{
  uint32_t type = m_req->extract_U32();

  CChannelGroups::Get(true).Clear();
  CChannelGroups::Get(false).Clear();

  switch(type)
  {
    // get groups defined in channels.conf
    default:
    case 0:
      cChannelManager::Get().CreateChannelGroups(false);
      break;
    // automatically create groups
    case 1:
      cChannelManager::Get().CreateChannelGroups(true);
      break;
  }

  uint32_t count = CChannelGroups::Get(true).Size() + CChannelGroups::Get(false).Size();

  m_resp->add_U32(count);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GroupList()
{
  uint32_t radio = m_req->extract_U8();
  std::vector<std::string> names = CChannelGroups::Get(radio).GroupNames();
  for(std::vector<std::string>::iterator i = names.begin(); i != names.end(); i++)
  {
    m_resp->add_String(i->c_str());
    m_resp->add_U8(radio);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GetGroupMembers()
{
  char* groupname = m_req->extract_String();
  uint32_t radio = m_req->extract_U8();
  bool filter = m_req->extract_U8();
  int index = 0;

  // unknown group
  const CChannelGroup* group = CChannelGroups::Get(radio).GetGroup(groupname);
  if(!group)
  {
    delete[] groupname;
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());
    return true;
  }

  bool automatic = group->Automatic();
  std::string name;

  ChannelVector channels = cChannelManager::Get().GetCurrent();
  for (ChannelVector::const_iterator it = channels.begin(); it != channels.end(); ++it)
  {
    ChannelPtr channel = *it;

    if(automatic)
      name = channel->Provider();

    if(name.empty())
      continue;

    if(CChannelFilter::IsRadio(channel) != radio)
      continue;

    // check filter
    if (filter && !VNSIChannelFilter.PassFilter(channel))
      continue;

    if(name == groupname)
    {
      m_resp->add_U32(channel->UID());
      m_resp->add_U32(++index);
    }
  }

//  Channels.Unlock();

  delete[] groupname;
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GetCaids()
{
  uint32_t uid = m_req->extract_U32();

  const ChannelPtr channel = cChannelManager::Get().GetByChannelUID(uid);
  if (channel)
  {
    int caid;
    int idx = 0;
    while((caid = channel->GetCaId(idx)) != 0)
    {
      m_resp->add_U32(caid);
      idx++;
    }
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processCHANNELS_GetWhitelist()
{
  bool radio = m_req->extract_U8();
  std::vector<CChannelProvider> *providers;

  if(radio)
    providers = &VNSIChannelFilter.m_providersRadio;
  else
    providers = &VNSIChannelFilter.m_providersVideo;

  VNSIChannelFilter.m_Mutex.Lock();
  for(unsigned int i=0; i<providers->size(); i++)
  {
    m_resp->add_String((*providers)[i].m_name.c_str());
    m_resp->add_U32((*providers)[i].m_caid);
  }
  VNSIChannelFilter.m_Mutex.Unlock();

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_GetBlacklist()
{
  bool radio = m_req->extract_U8();
  std::vector<int> *channels;

  if(radio)
    channels = &VNSIChannelFilter.m_channelsRadio;
  else
    channels = &VNSIChannelFilter.m_channelsVideo;

  VNSIChannelFilter.m_Mutex.Lock();
  for(unsigned int i=0; i<channels->size(); i++)
  {
    m_resp->add_U32((*channels)[i]);
  }
  VNSIChannelFilter.m_Mutex.Unlock();

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_SetWhitelist()
{
  bool radio = m_req->extract_U8();
  CChannelProvider provider;
  std::vector<CChannelProvider> *providers;

  if(radio)
    providers = &VNSIChannelFilter.m_providersRadio;
  else
    providers = &VNSIChannelFilter.m_providersVideo;

  VNSIChannelFilter.m_Mutex.Lock();
  providers->clear();

  while(!m_req->end())
  {
    char *str = m_req->extract_String();
    provider.m_name = str;
    provider.m_caid = m_req->extract_U32();
    delete [] str;
    providers->push_back(provider);
  }
  VNSIChannelFilter.StoreWhitelist(radio);
  VNSIChannelFilter.m_Mutex.Unlock();

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processCHANNELS_SetBlacklist()
{
  bool radio = m_req->extract_U8();
  CChannelProvider provider;
  std::vector<int> *channels;

  if(radio)
    channels = &VNSIChannelFilter.m_channelsRadio;
  else
    channels = &VNSIChannelFilter.m_channelsVideo;

  VNSIChannelFilter.m_Mutex.Lock();
  channels->clear();

  int id;
  while(!m_req->end())
  {
    id = m_req->extract_U32();
    channels->push_back(id);
  }
  VNSIChannelFilter.StoreBlacklist(radio);
  VNSIChannelFilter.m_Mutex.Unlock();

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

/** OPCODE 80 - 99: VNSI network functions for timer access */

bool cVNSIClient::processTIMER_GetCount() /* OPCODE 80 */
{
  CLockObject lock(m_timerLock);

  size_t count = cTimers::Get().Size();

  m_resp->add_U32(count);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Get() /* OPCODE 81 */
{
  CLockObject lock(m_timerLock);

  uint32_t number = m_req->extract_U32();

  size_t numTimers = cTimers::Get().Size();
  if (numTimers > 0)
  {
    TimerPtr timer = cTimers::Get().GetByIndex(number-1);
    if (timer)
    {
      m_resp->add_U32(VNSI_RET_OK);

      m_resp->add_U32(timer->Index()+1); //TODO
      m_resp->add_U32(timer->HasFlags(tfActive));
      m_resp->add_U32(timer->Recording());
      m_resp->add_U32(timer->Pending());
      m_resp->add_U32(timer->Priority());
      m_resp->add_U32(timer->LifetimeDays());
      m_resp->add_U32(timer->Channel()->Number());
      m_resp->add_U32(timer->Channel()->UID());
      m_resp->add_U32(timer->StartTimeAsTime());
      m_resp->add_U32(timer->EndTimeAsTime());
      m_resp->add_U32(timer->Day());
      m_resp->add_U32(timer->WeekDays());
      m_resp->add_String(m_toUTF8.Convert(timer->RecordingFilename().c_str()));
    }
    else
      m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
  }
  else
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_GetList() /* OPCODE 82 */
{
  CLockObject lock(m_timerLock);

  std::vector<TimerPtr> timers = cTimers::Get().GetTimers();

  m_resp->add_U32(timers.size());

  for (std::vector<TimerPtr>::const_iterator it = timers.begin(); it != timers.end(); ++it)
  {
    m_resp->add_U32((*it)->Index());
    m_resp->add_U32((*it)->HasFlags(tfActive));
    m_resp->add_U32((*it)->Recording());
    m_resp->add_U32((*it)->Pending());
    m_resp->add_U32((*it)->Priority());
    m_resp->add_U32((*it)->LifetimeDays());
    m_resp->add_U32((*it)->Channel()->Number());
    m_resp->add_U32((*it)->Channel()->UID());
    m_resp->add_U32((*it)->StartTimeAsTime());
    m_resp->add_U32((*it)->EndTimeAsTime());
    m_resp->add_U32((*it)->Day());
    m_resp->add_U32((*it)->WeekDays());
    m_resp->add_String(m_toUTF8.Convert((*it)->RecordingFilename().c_str()));
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Add() /* OPCODE 83 */
{
  CLockObject lock(m_timerLock);

  uint32_t flags      = m_req->extract_U32() > 0 ? tfActive : tfNone;
  uint32_t priority   = m_req->extract_U32();
  uint32_t lifetime   = m_req->extract_U32();
  uint32_t channelid  = m_req->extract_U32();
  time_t startTime    = m_req->extract_U32();
  time_t stopTime     = m_req->extract_U32();
  time_t day          = m_req->extract_U32();
  uint32_t weekdays   = m_req->extract_U32();
  const char *file    = m_req->extract_String();

  ChannelPtr channel = cChannelManager::Get().GetByChannelUID(channelid);
  if(!channel)
  {
    delete[] file;
    esyslog("Error in timer settings");
    m_resp->add_U32(VNSI_RET_DATAINVALID);
  }
  else
  {
    // XXX fix the protocol to use the duration
    cTimer* timer = new cTimer(channel, CTimerTime::FromVNSI(startTime, stopTime, day, weekdays), flags, priority, lifetime, file);
    delete[] file;

    TimerPtr t = cTimers::Get().GetTimer(timer);
    if (!t)
    {
      isyslog("Timer %s added", timer->ToDescr().c_str());
      m_resp->add_U32(VNSI_RET_OK);
      m_resp->finalise();
      m_socket.write(m_resp->getPtr(), m_resp->getLen());
      cTimers::Get().Add(TimerPtr(timer));
      return true;
    }
    else
    {
      esyslog("Timer already defined: %u %s", t->Index() + 1, t->ToDescr().c_str());
      m_resp->add_U32(VNSI_RET_DATALOCKED);
    }

    delete timer;
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Delete() /* OPCODE 84 */
{
  CLockObject lock(m_timerLock);

  uint32_t number = m_req->extract_U32();
  bool     force  = m_req->extract_U32();

  TimerPtr timer = cTimers::Get().GetByIndex(number);
  if (timer)
  {
    if (timer->Recording())
    {
      if (force)
      {
        timer->Skip();
//XXX   cRecordControls::Process(time(NULL));
      }
      else
      {
        esyslog("Timer \"%i\" is recording and can be deleted (use force=1 to stop it)", number);
        m_resp->add_U32(VNSI_RET_RECRUNNING);
        m_resp->finalise();
        m_socket.write(m_resp->getPtr(), m_resp->getLen());
        return true;
      }
    }
    isyslog("Deleting timer %s", timer->ToDescr().c_str());
    m_resp->add_U32(VNSI_RET_OK);
    cTimers::Get().Del(timer);
  }
  else
  {
    esyslog("Unable to delete timer - invalid timer identifier");
    m_resp->add_U32(VNSI_RET_DATAINVALID);
  }
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processTIMER_Update() /* OPCODE 85 */
{
  CLockObject lock(m_timerLock);

  int length      = m_req->getDataLength();
  uint32_t index  = m_req->extract_U32();
  bool active     = m_req->extract_U32();

  TimerPtr timer = cTimers::Get().GetByIndex(index);
  if (!timer)
  {
    esyslog("Timer \"%u\" not defined", index);
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());
    return true;
  }

  cTimer& timerData = *timer;

  if (length == 8)
  {
    if (active)
      timerData.SetFlags(tfActive);
    else
      timerData.ClrFlags(tfActive);
  }
  else
  {
    uint32_t flags      = active ? tfActive : tfNone;
    uint32_t priority   = m_req->extract_U32();
    uint32_t lifetime   = m_req->extract_U32();
    uint32_t channelid  = m_req->extract_U32();
    time_t startTime    = m_req->extract_U32();
    time_t stopTime     = m_req->extract_U32();
    time_t day          = m_req->extract_U32();
    uint32_t weekdays   = m_req->extract_U32();
    const char *file    = m_req->extract_String();

    ChannelPtr channel = cChannelManager::Get().GetByChannelUID(channelid);
    if(channel)
    {
      // XXX fix the protocol to use the duration
      cTimer newData(channel, CTimerTime::FromVNSI(startTime, stopTime, day, weekdays), flags, priority, lifetime, file);
      timerData = newData;
    }

    delete[] file;
  }

  cTimers::Get().SetModified();

  m_resp->add_U32(VNSI_RET_OK);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}


/** OPCODE 100 - 119: VNSI network functions for recording access */

bool cVNSIClient::processRECORDINGS_GetDiskSpace() /* OPCODE 100 */
{
  disk_space_t space;
  unsigned int Percent = VideoDiskSpace(space);

  m_resp->add_U32((space.free + space.used) / MEGABYTE(1));
  m_resp->add_U32(space.free / MEGABYTE(1));
  m_resp->add_U32(Percent);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRECORDINGS_GetCount() /* OPCODE 101 */
{
  Recordings.Load();
  m_resp->add_U32(Recordings.Size());

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRECORDINGS_GetList() /* OPCODE 102 */
{
  CLockObject lock(m_timerLock);
  CThreadLock RecordingsLock(&Recordings);
  std::vector<cRecording*> recordings = Recordings.Recordings();
  for (std::vector<cRecording*>::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    const EventPtr& event = (*it)->Info()->GetEvent();

    CDateTime recordingStart;
    int       recordingDuration = 0;
    if (event.get() != NULL)
    {
      recordingStart    = event->StartTime();
      recordingDuration = event->Duration();
    }
    else
    {
//      cRecordControl *rc = cRecordControls::GetRecordControl(recording->FileName());
//      if (rc)
//      {
//        recordingStart    = rc->Timer()->StartTime();
//        recordingDuration = rc->Timer()->StopTime() - recordingStart;
//      }
//      else
//      {
        recordingStart = (*it)->Start2();
//      }
    }
    dsyslog("GRI: RC: recordingStart=%s recordingDuration=%i", recordingStart.GetAsSaveString().c_str(), recordingDuration);

    // recording_time
    time_t tmStart;
    recordingStart.GetAsTime(tmStart);
    m_resp->add_U32(tmStart);

    // duration
    m_resp->add_U32(recordingDuration);

    // priority
    m_resp->add_U32((*it)->Priority());

    // lifetime
    m_resp->add_U32((*it)->LifetimeDays());

    // channel_name
    m_resp->add_String(!(*it)->Info()->ChannelName().empty() ? m_toUTF8.Convert((*it)->Info()->ChannelName().c_str()) : "");

    char* fullname = strdup((*it)->Name().c_str());
    char* recname = strrchr(fullname, FOLDERDELIMCHAR);
    char* directory = NULL;

    if(recname == NULL) {
      recname = fullname;
    }
    else {
      *recname = 0;
      recname++;
      directory = fullname;
    }

    // title
    m_resp->add_String(m_toUTF8.Convert(recname));

    // subtitle
    if (!(*it)->Info()->ShortText().empty())
      m_resp->add_String(m_toUTF8.Convert((*it)->Info()->ShortText().c_str()));
    else
      m_resp->add_String("");

    // description
    if (!(*it)->Info()->Description().empty())
      m_resp->add_String(m_toUTF8.Convert((*it)->Info()->Description().c_str()));
    else
      m_resp->add_String("");

    // directory
    if(directory != NULL) {
      char* p = directory;
      while(*p != 0) {
        if(*p == FOLDERDELIMCHAR) *p = '/';
        if(*p == '_') *p = ' ';
        p++;
      }
      while(*directory == '/') directory++;
    }

    m_resp->add_String((isempty(directory)) ? "" : m_toUTF8.Convert(directory));

    // filename / uid of recording
    m_resp->add_U32((*it)->UID());

    free(fullname);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processRECORDINGS_Rename() /* OPCODE 103 */
{
  uint32_t    uid          = m_req->extract_U32();
  char*       newtitle     = m_req->extract_String();
  cRecording* recording    = Recordings.FindByUID(uid);
  int         r            = VNSI_RET_DATAINVALID;

  if(recording != NULL) {
    // get filename and remove last part (recording time)
    char* filename_old = strdup((const char*)recording->FileName().c_str());
    char* sep = strrchr(filename_old, '/');
    if(sep != NULL) {
      *sep = 0;
    }

    // replace spaces in newtitle
    strreplace(newtitle, ' ', '_');
    char* filename_new = new char[1024];
    strncpy(filename_new, filename_old, 512);
    sep = strrchr(filename_new, '/');
    if(sep != NULL) {
      sep++;
      *sep = 0;
    }
    strncat(filename_new, newtitle, 512);

    isyslog("renaming recording '%s' to '%s'", filename_old, filename_new);
    r = rename(filename_old, filename_new);
    Recordings.Update();

    free(filename_old);
    delete[] filename_new;
  }

  m_resp->add_U32(r);
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processRECORDINGS_Delete() /* OPCODE 104 */
{
  std::string recName;
  cRecording* recording = NULL;

  uint32_t uid = m_req->extract_U32();
  recording = Recordings.FindByUID(uid);

  if (recording)
  {
    dsyslog("deleting recording: %s", recording->Name().c_str());

    TimerPtr tmrRecording = cTimers::Get().GetTimerForRecording(recording);
    if (tmrRecording)
    {
      esyslog("Recording \"%s\" is in use by timer %d", recording->Name().c_str(), tmrRecording->Index() + 1);
      m_resp->add_U32(VNSI_RET_DATALOCKED);
    }
    else
    {
      //XXX
      if (recording->Delete())
      {
        // Copy svdrdeveldevelp's way of doing this, see if it works
        Recordings.DelByName(recording->FileName());
        m_resp->add_U32(VNSI_RET_OK);
      }
      else
      {
        esyslog("Error while deleting recording!");
        m_resp->add_U32(VNSI_RET_ERROR);
      }
    }
  }
  else
  {
    esyslog("Error in recording name \"%s\"", recName.c_str()); // TODO: This variable is never set?
    m_resp->add_U32(VNSI_RET_DATAUNKNOWN);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

bool cVNSIClient::processRECORDINGS_GetEdl() /* OPCODE 105 */
{
  std::string recName; // TODO: This variable is unused?
  cRecording* recording = NULL;

  uint32_t uid = m_req->extract_U32();
  recording = Recordings.FindByUID(uid);

  if (recording)
  {
    cMarks marks;
    if(marks.Load(recording->FileName(), recording->FramesPerSecond(), recording->IsPesRecording()))
    {
      cMark* mark = NULL;
      double fps = recording->FramesPerSecond();
      while((mark = marks.GetNextBegin(mark)) != NULL)
      {
        m_resp->add_U64(mark->Position() *1000 / fps);
        m_resp->add_U64(mark->Position() *1000 / fps);
        m_resp->add_S32(2);
      }
    }
  }
  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  return true;
}

/** OPCODE 120 - 139: VNSI network functions for epg access and manipulating */

bool cVNSIClient::processEPG_GetForChannel() /* OPCODE 120 */
{
  uint32_t channelUID  = 0;

  channelUID = m_req->extract_U32();

  CDateTime startTime = CDateTime(m_req->extract_U32());
  uint32_t  duration  = m_req->extract_U32();
  CDateTime endTime   = startTime + CDateTimeSpan(0, 0, 0, duration);

  ChannelPtr channel = cChannelManager::Get().GetByChannelUID(channelUID);
  if(!channel)
  {
    m_resp->add_U32(0);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());

    esyslog("cannot find channel '%d'", channelUID);
    return true;
  }

  cSchedulesLock MutexLock;
  cSchedules *Schedules = MutexLock.Get();
  m_epgUpdate[channelUID].Reset();
  if (!Schedules)
  {
    m_resp->add_U32(0);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());

    dsyslog("cannot write EPG data for channel '%s': cannot acquire a lock", channel->Name().c_str());
    return true;
  }

  SchedulePtr Schedule = channel->Schedule();
  if (!Schedule)
  {
    m_resp->add_U32(0);
    m_resp->finalise();
    m_socket.write(m_resp->getPtr(), m_resp->getLen());

    dsyslog("cannot find EPG data for channel '%s'", channel->Name().c_str());
    return true;
  }

  bool atLeastOneEvent = false;

  uint32_t    thisEventID;
  CDateTime   thisEventStart;
  CDateTime   thisEventEnd;
  uint32_t    thisEventDuration;
  uint32_t    thisEventContent;
  uint32_t    thisEventRating;
  const char* thisEventTitle;
  const char* thisEventSubTitle;
  const char* thisEventDescription;

  for (EventVector::const_iterator it = Schedule->Events().begin(); it != Schedule->Events().end(); ++it)
  {
    const EventPtr& event = *it;

    thisEventID           = event->EventID();
    thisEventTitle        = event->Title().c_str();
    thisEventSubTitle     = event->ShortText().c_str();
    thisEventDescription  = event->Description().c_str();
    thisEventStart        = event->StartTime();
    thisEventEnd          = event->EndTime();
    thisEventDuration     = event->Duration();
    thisEventContent      = event->Contents();
    thisEventRating       = event->ParentalRating();

    //in the past filter
    if (thisEventEnd < CDateTime::GetCurrentDateTime().GetAsUTCDateTime()) continue;

    //start time filter
    if (thisEventEnd <= startTime) continue;

    //duration filter
    if (duration != 0 && thisEventStart >= endTime) continue;

    if (!thisEventTitle)        thisEventTitle        = "";
    if (!thisEventSubTitle)     thisEventSubTitle     = "";
    if (!thisEventDescription)  thisEventDescription  = "";

    m_resp->add_U32(thisEventID);
    m_resp->add_U32(event->StartTimeAsTime());
    m_resp->add_U32(thisEventDuration);
    m_resp->add_U32(thisEventContent);
    m_resp->add_U32(thisEventRating);

    m_resp->add_String(m_toUTF8.Convert(thisEventTitle));
    m_resp->add_String(m_toUTF8.Convert(thisEventSubTitle));
    m_resp->add_String(m_toUTF8.Convert(thisEventDescription));

    atLeastOneEvent = true;
  }

  if (!atLeastOneEvent)
  {
    m_resp->add_U32(0);
    dsyslog("cannot find EPG data for channel '%s': schedule is empty", channel->Name().c_str());
  }
  else
  {
    dsyslog("writing EPG data for channel '%s'", channel->Name().c_str());
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());

  const EventVector& events = Schedule->Events();
  EventPtr lastEvent;
  if (!events.empty())
    lastEvent = events[events.size() - 1];
  if (lastEvent)
  {
    m_epgUpdate[channelUID] = lastEvent->StartTime();
  }

  return true;
}


/** OPCODE 140 - 169: VNSI network functions for channel scanning */

bool cVNSIClient::processSCAN_ScanSupported() /* OPCODE 140 */
{
  m_resp->add_U32(VNSI_RET_OK);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_GetCountries() /* OPCODE 141 */
{
  m_resp->add_U32(VNSI_RET_OK);

  for (unsigned int i = 0; i < CountryUtils::CountryCount(); i++)
  {
    m_resp->add_U32(CountryUtils::GetCountry(i).id);
    m_resp->add_String(CountryUtils::GetCountry(i).short_name);
    m_resp->add_String(CountryUtils::GetCountry(i).full_name);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_GetSatellites() /* OPCODE 142 */
{
  m_resp->add_U32(VNSI_RET_OK);

  for (unsigned int i = 0; SatelliteUtils::HasSatellite((SATELLITE::eSatellite)i); i++)
  {
    m_resp->add_U32(SatelliteUtils::GetSatellite((SATELLITE::eSatellite)i).id);
    m_resp->add_String(SatelliteUtils::GetSatellite((SATELLITE::eSatellite)i).short_name);
    m_resp->add_String(SatelliteUtils::GetSatellite((SATELLITE::eSatellite)i).full_name);
  }

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_Start() /* OPCODE 143 */
{
  cScanConfig config;

  // For now, force values to ATSC
  config.dvbType         = TRANSPONDER_ATSC;
  config.atscModulation  = ATSC_MODULATION_VSB_8;
  config.device          = cDeviceManager::Get().GetDevice(0); // TODO: Support multiple devices

  if (m_scanner.Start(config))
    m_resp->add_U32(VNSI_RET_OK);
  else
    m_resp->add_U32(VNSI_RET_ERROR);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_Stop() /* OPCODE 144 */
{
  m_scanner.Stop(false);

  m_resp->add_U32(VNSI_RET_OK);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

bool cVNSIClient::processSCAN_Progress() /* OPCODE 145 */
{
  if (m_scanner.IsRunning())
  {
    m_resp->add_U32(VNSI_RET_OK);
    m_resp->add_double(m_scanner.GetPercentage());
    m_resp->add_U32(m_scanner.GetFrequency());
    m_resp->add_U32(m_scanner.GetChannelNumber());
  }
  else
    m_resp->add_U32(VNSI_RET_ERROR);

  m_resp->finalise();
  m_socket.write(m_resp->getPtr(), m_resp->getLen());
  return true;
}

const char* cVNSIClient::OpcodeToString(uint8_t opcode)
{
  switch (opcode)
  {
  case VNSI_LOGIN:
    return "login";
  case VNSI_GETTIME:
    return "get time";
  case VNSI_ENABLESTATUSINTERFACE:
    return "enable status interface";
  case VNSI_PING:
    return "ping";
  case VNSI_GETSETUP:
    return "get setup";
  case VNSI_STORESETUP:
    return "store setup";
  case VNSI_CHANNELSTREAM_OPEN:
    return "channel stream open";
  case VNSI_CHANNELSTREAM_CLOSE:
    return "channel stream close";
  case VNSI_CHANNELSTREAM_SEEK:
    return "channel stream seek";
  case VNSI_RECSTREAM_OPEN:
    return "rec stream open";
  case VNSI_RECSTREAM_CLOSE:
    return "rec stream close";
  case VNSI_RECSTREAM_GETBLOCK:
    return "rec stream get block";
  case VNSI_RECSTREAM_POSTOFRAME:
    return "rec stream position to frame";
  case VNSI_RECSTREAM_FRAMETOPOS:
    return "rec stream frame to position";
  case VNSI_RECSTREAM_GETIFRAME:
    return "rec stream get iframe";
  case VNSI_RECSTREAM_GETLENGTH:
    return "rec stream get length";
  case VNSI_CHANNELS_GETCOUNT:
    return "channels get count";
  case VNSI_CHANNELS_GETCHANNELS:
    return "channels get channels";
  case VNSI_CHANNELGROUP_GETCOUNT:
    return "channel group get count";
  case VNSI_CHANNELGROUP_LIST:
    return "channel group list";
  case VNSI_CHANNELGROUP_MEMBERS:
    return "channel group members";
  case VNSI_TIMER_GETCOUNT:
    return "timer get count";
  case VNSI_TIMER_GET:
    return "timer get";
  case VNSI_TIMER_GETLIST:
    return "timer get list";
  case VNSI_TIMER_ADD:
    return "timer add";
  case VNSI_TIMER_DELETE:
    return "timer delete";
  case VNSI_TIMER_UPDATE:
    return "timer update";
  case VNSI_RECORDINGS_DISKSIZE:
    return "recordings disk size";
  case VNSI_RECORDINGS_GETCOUNT:
    return "recordings get count";
  case VNSI_RECORDINGS_GETLIST:
    return "recordings get list";
  case VNSI_RECORDINGS_RENAME:
    return "recordings rename";
  case VNSI_RECORDINGS_DELETE:
    return "recordings delete";
  case VNSI_EPG_GETFORCHANNEL:
    return "epg get for channel";
  case VNSI_SCAN_SUPPORTED:
    return "scan supported";
  case VNSI_SCAN_GETCOUNTRIES:
    return "scan get countries";
  case VNSI_SCAN_GETSATELLITES:
    return "scan get satellites";
  case VNSI_SCAN_START:
    return "scan start";
  case VNSI_SCAN_STOP:
    return "scan stop";
  case VNSI_SCAN_PROGRESS:
    return "scan progress";
  case VNSI_CHANNELS_GETCAIDS:
    return "channels get caids";
  case VNSI_CHANNELS_GETWHITELIST:
    return "channels get whitelist";
  case VNSI_CHANNELS_GETBLACKLIST:
    return "channels get blacklist";
  case VNSI_CHANNELS_SETWHITELIST:
    return "channels set whitelist";
  case VNSI_CHANNELS_SETBLACKLIST:
    return "channels set blacklist";
  default:
    return "<unknown>";
  }
}

const char* cVNSIClient::ChannelToString(uint8_t channel)
{
  switch (channel)
  {
  case VNSI_CHANNEL_REQUEST_RESPONSE:
    return "request response";
  case VNSI_CHANNEL_STREAM:
    return "stream";
  case VNSI_CHANNEL_KEEPALIVE:
    return "keep alive";
  case VNSI_CHANNEL_NETLOG:
    return "netlog";
  case VNSI_CHANNEL_STATUS:
    return "status";
  case VNSI_CHANNEL_SCAN:
    return "channel scan";
  default:
    return "<unknown>";
  }
}

}
