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
#pragma once

#include "channels/ChannelTypes.h"
#include "lib/platform/threads/threads.h"
#include "utils/Tools.h"
#include "devices/Receiver.h"
#include "scan/Scanner.h"
#include "utils/Status.h"
#include "utils/CharSetConverterVDR.h"
#include "utils/Observer.h"

#include "utils/XSocket.h"

#include <map>
#include <string>

namespace VDR
{
class cDevice;
class cLiveStreamer;
class cRequestPacket;
class cResponsePacket;
class cRecPlayer;
class cCmdControl;
//class cVnsiOsdProvider;


class cVNSIClient : public PLATFORM::CThread
                  , public Observer
{
private:

  unsigned int     m_Id;
  cxSocket         m_socket;
  bool             m_loggedIn;
  bool             m_StatusInterfaceEnabled;
  cLiveStreamer   *m_Streamer;
  bool             m_isStreaming;
  std::string      m_ClientAddress;
  cRecPlayer      *m_RecPlayer;
  cRequestPacket  *m_req;
  cResponsePacket *m_resp;
  cCharSetConv     m_toUTF8;
  uint32_t         m_protocolVersion;
  PLATFORM::CMutex m_msgLock;
  static PLATFORM::CMutex m_timerLock;
//  cVnsiOsdProvider *m_Osd;
  std::map<int, CDateTime> m_epgUpdate;
  cScanner         m_scanner;

protected:

  bool processRequest(cRequestPacket* req);

  virtual void* Process(void);

  virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
  virtual void OsdStatusMessage(const char *Message);

public:

  cVNSIClient(int fd, unsigned int id, const char *ClientAdr);
  virtual ~cVNSIClient();

  void ChannelChange();
  void RecordingsChange();
  void TimerChange();
  void EpgChange();

  unsigned int GetID() { return m_Id; }

  void Notify(const Observable &obs, const ObservableMessage msg);

protected:

  void SetLoggedIn(bool yesNo) { m_loggedIn = yesNo; }
  void SetStatusInterface(bool yesNo) { m_StatusInterfaceEnabled = yesNo; }
  bool StartChannelStreaming(ChannelPtr channel, int32_t priority, uint8_t timeshift, uint32_t timeout);
  void StopChannelStreaming();

private:

  typedef struct {
    bool automatic;
    bool radio;
    std::string name;
  } ChannelGroup;

  bool process_Login();
  bool process_GetTime();
  bool process_EnableStatusInterface();
  bool process_Ping();
  bool process_GetSetup();
  bool process_StoreSetup();

  bool processChannelStream_Open();
  bool processChannelStream_Close();
  bool processChannelStream_Seek();

  bool processRecStream_Open();
  bool processRecStream_Close();
  bool processRecStream_GetBlock();
  bool processRecStream_PositionFromFrameNumber();
  bool processRecStream_FrameNumberFromPosition();
  bool processRecStream_GetIFrame();
  bool processRecStream_GetLength();

  bool processCHANNELS_GroupsCount();
  bool processCHANNELS_ChannelsCount();
  bool processCHANNELS_GroupList();
  bool processCHANNELS_GetChannels();
  bool processCHANNELS_GetGroupMembers();
  bool processCHANNELS_GetCaids();
  bool processCHANNELS_GetWhitelist();
  bool processCHANNELS_GetBlacklist();
  bool processCHANNELS_SetWhitelist();
  bool processCHANNELS_SetBlacklist();

  bool processTIMER_GetCount();
  bool processTIMER_Get();
  bool processTIMER_GetList();
  bool processTIMER_Add();
  bool processTIMER_Delete();
  bool processTIMER_Update();

  bool processRECORDINGS_GetDiskSpace();
  bool processRECORDINGS_GetCount();
  bool processRECORDINGS_GetList();
  bool processRECORDINGS_GetInfo();
  bool processRECORDINGS_Rename();
  bool processRECORDINGS_Delete();
  bool processRECORDINGS_Move();
  bool processRECORDINGS_GetEdl();

  bool processEPG_GetForChannel();

  bool processSCAN_ScanSupported();
  bool processSCAN_GetCountries();
  bool processSCAN_GetSatellites();
  bool processSCAN_Start();
  bool processSCAN_Stop();
  bool processSCAN_Progress();

  const char* OpcodeToString(uint8_t opcode);
  const char* ChannelToString(uint8_t channel);
};

}
