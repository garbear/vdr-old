/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "CIAdapter.h"
#include "channels/ChannelID.h"
#include "thread.h"
#include "utils/Tools.h"
#include "platform/threads/threads.h"

#include <stdint.h>
#include <stdio.h>


#define MAX_CONNECTIONS_PER_CAM_SLOT  8 // maximum possible value is 254
#define CAM_READ_TIMEOUT  50 // ms

class cTPDU;
class cChannel;
class cCiTransportConnection;
class cCiSession;
class cCiCaProgramData;

class cCamSlot : public cListObject {
  friend class cCiAdapter;
  friend class cCiTransportConnection;
private:
  PLATFORM::CMutex mutex;
  PLATFORM::CCondition<bool> processed;
  bool bProcessed;
  cCiAdapter *ciAdapter;
  int slotIndex;
  int slotNumber;
  cCiTransportConnection *tc[MAX_CONNECTIONS_PER_CAM_SLOT + 1];  // connection numbering starts with 1
  eModuleStatus lastModuleStatus;
  time_t resetTime;
  cTimeMs moduleCheckTimer;
  bool resendPmt;
  int source;
  int transponder;
  cList<cCiCaProgramData> caProgramList;
  const int *GetCaSystemIds(void);
  void SendCaPmt(uint8_t CmdId);
  void NewConnection(void);
  void DeleteAllConnections(void);
  void Process(cTPDU *TPDU = NULL);
  void Write(cTPDU *TPDU);
  cCiSession *GetSessionByResourceId(uint32_t ResourceId);
public:
  cCamSlot(cCiAdapter *CiAdapter);
       ///< Creates a new CAM slot for the given CiAdapter.
       ///< The CiAdapter will take care of deleting the CAM slot,
       ///< so the caller must not delete it!
  virtual ~cCamSlot();
  bool Assign(cDevice *Device, bool Query = false);
       ///< Assigns this CAM slot to the given Device, if this is possible.
       ///< If Query is 'true', the CI adapter of this slot only checks whether
       ///< it can be assigned to the Device, but doesn't actually assign itself to it.
       ///< Returns true if this slot can be assigned to the Device.
       ///< If Device is NULL, the slot will be unassigned from any
       ///< device it was previously assigned to. The value of Query
       ///< is ignored in that case, and this function always returns
       ///< 'true'.
  cDevice *Device(void);
       ///< Returns the device this CAM slot is currently assigned to.
  int SlotIndex(void) { return slotIndex; }
       ///< Returns the index of this CAM slot within its CI adapter.
       ///< The first slot has an index of 0.
  int SlotNumber(void) { return slotNumber; }
       ///< Returns the number of this CAM slot within the whole system.
       ///< The first slot has the number 1.
  bool Reset(void);
       ///< Resets the CAM in this slot.
       ///< Returns true if the operation was successful.
  eModuleStatus ModuleStatus(void);
       ///< Returns the status of the CAM in this slot.
  const char *GetCamName(void);
       ///< Returns the name of the CAM in this slot, or NULL if there is
       ///< no ready CAM in this slot.
  bool Ready(void);
       ///< Returns 'true' if the CAM in this slot is ready to decrypt.
  bool HasMMI(void);
       ///< Returns 'true' if the CAM in this slot has an active MMI.
  bool HasUserIO(void);
       ///< Returns true if there is a pending user interaction, which shall
       ///< be retrieved via GetMenu() or GetEnquiry().
  bool EnterMenu(void);
       ///< Requests the CAM in this slot to start its menu.
  cCiMenu *GetMenu(void);
       ///< Gets a pending menu, or NULL if there is no menu.
  cCiEnquiry *GetEnquiry(void);
       ///< Gets a pending enquiry, or NULL if there is no enquiry.
  int Priority(void);
       ///< Returns the priority if the device this slot is currently assigned
       ///< to, or IDLEPRIORITY if it is not assigned to any device.
  bool ProvidesCa(const int *CaSystemIds);
       ///< Returns true if the CAM in this slot provides one of the given
       ///< CaSystemIds. This doesn't necessarily mean that it will be
       ///< possible to actually decrypt such a programme, since CAMs
       ///< usually advertise several CA system ids, while the actual
       ///< decryption is controlled by the smart card inserted into
       ///< the CAM.
  void AddPid(int ProgramNumber, int Pid, int StreamType);
       ///< Adds the given PID information to the list of PIDs. A later call
       ///< to SetPid() will (de)activate one of these entries.
  void SetPid(int Pid, bool Active);
       ///< Sets the given Pid (which has previously been added through a
       ///< call to AddPid()) to Active. A later call to StartDecrypting() will
       ///< send the full list of currently active CA_PMT entries to the CAM.
  void AddChannel(const cChannel *Channel);
       ///< Adds all PIDs if the given Channel to the current list of PIDs.
       ///< If the source or transponder of the channel are different than
       ///< what was given in a previous call to AddChannel(), any previously
       ///< added PIDs will be cleared.
  bool CanDecrypt(const cChannel *Channel);
       ///< Returns true if there is a CAM in this slot that is able to decrypt
       ///< the given Channel (or at least claims to be able to do so).
       ///< Since the QUERY/REPLY mechanism for CAMs is pretty unreliable (some
       ///< CAMs don't reply to queries at all), we always return true if the
       ///< CAM is currently not decrypting anything. If there is already a
       ///< channel being decrypted, a call to CanDecrypt() checks whether the
       ///< CAM can also decrypt the given channel. Only CAMs that have replied
       ///< to the initial QUERY will perform this check at all. CAMs that never
       ///< replied to the initial QUERY are assumed not to be able to handle
       ///< more than one channel at a time.
  void StartDecrypting(void);
       ///< Triggers sending all currently active CA_PMT entries to the CAM,
       ///< so that it will start decrypting.
  void StopDecrypting(void);
       ///< Clears the list of CA_PMT entries and tells the CAM to stop decrypting.
  bool IsDecrypting(void);
       ///< Returns true if the CAM in this slot is currently used for decrypting.
  };

class cCamSlots : public cList<cCamSlot> {};

extern cCamSlots CamSlots;
