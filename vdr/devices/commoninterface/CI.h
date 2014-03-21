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

#include "Types.h"
#include "channels/ChannelID.h"
#include "utils/Tools.h"
#include "platform/threads/threads.h"
#include "devices/Device.h"

#include <stdint.h>
#include <stdio.h>

namespace VDR
{
#define MAX_CAM_SLOTS_PER_ADAPTER     8 // maximum possible value is 255

#define MAX_CONNECTIONS_PER_CAM_SLOT  8 // maximum possible value is 254
#define CAM_READ_TIMEOUT  50 // ms

class cCiMMI;

class cCiMenu
{
  friend class cCamSlot;
  friend class cCiMMI;
private:
  enum { MAX_CIMENU_ENTRIES = 64 }; ///< XXX is there a specified maximum?
  cCiMMI *mmi;
  PLATFORM::CMutex* mutex;
  bool selectable;
  char *titleText;
  char *subTitleText;
  char *bottomText;
  char *entries[MAX_CIMENU_ENTRIES];
  int numEntries;
  bool AddEntry(char *s);
  cCiMenu(cCiMMI *MMI, bool Selectable);
public:
  ~cCiMenu();
  const char *TitleText(void) { return titleText; }
  const char *SubTitleText(void) { return subTitleText; }
  const char *BottomText(void) { return bottomText; }
  const char *Entry(int n) { return n < numEntries ? entries[n] : NULL; }
  int NumEntries(void) { return numEntries; }
  bool Selectable(void) { return selectable; }
  void Select(int Index);
  void Cancel(void);
  void Abort(void);
  bool HasUpdate(void);
  };

class cCiEnquiry {
  friend class cCamSlot;
  friend class cCiMMI;
private:
  cCiMMI *mmi;
  PLATFORM::CMutex *mutex;
  char *text;
  bool blind;
  int expectedLength;
  cCiEnquiry(cCiMMI *MMI);
public:
  ~cCiEnquiry();
  const char *Text(void) { return text; }
  bool Blind(void) { return blind; }
  int ExpectedLength(void) { return expectedLength; }
  void Reply(const char *s);
  void Cancel(void);
  void Abort(void);
  };

class cDevice;
class cCamSlot;

enum eModuleStatus { msNone, msReset, msPresent, msReady };

class cCiAdapter : public PLATFORM::CThread
{
  friend class cCamSlot;

public:
  cCiAdapter();
  // The derived class must call StopThread(3000) in its destructor
  virtual ~cCiAdapter();

  /*!
   * \brief Returns 'true' if all present CAMs in this adapter are ready
   */
  virtual bool Ready();

protected:
  /*!
   * \brief Handles the attached CAM slots in a separate thread
   *
   * The derived class must call the CreateThread() function to actually start CAM
   * handling.
   */
  virtual void *Process();

  /*!
   * \brief Reads one chunk of data into the given Buffer, up to MaxLength bytes
   * \return The number of bytes read (in case of an error it will also return 0)
   *
   * If no data is available immediately, wait for up to CAM_READ_TIMEOUT.
   */
  virtual int Read(uint8_t *Buffer, int MaxLength) = 0;

  // Writes Length bytes of the given Buffer
  virtual void Write(const uint8_t *Buffer, int Length) = 0;

  /*!
   * \brief Resets the CAM in the given Slot
   * \return True if the operation was successful
   */
  virtual bool Reset(int Slot) = 0;

  /*!
   * \brief
   * \return The status of the CAM in the given Slot
   */
  virtual eModuleStatus ModuleStatus(int Slot) = 0;

  /*!
   * \brief Assigns this adapter to the given Device, if possible
   * \param Device If NULL, the adapter will be unassigned from any device it
   *        was previously assigned to. The value of Query is ignored in that
   *        case, and this function always returns 'true'
   * \param Query If true, the adapter only checks whether it can be assigned
   *        to the Device, but doesn't actually assign itself to it
   * \return True if the adapter can be assigned to the Device
   */
  virtual bool Assign(DevicePtr Device, bool Query = false) = 0;

private:
  /*!
   * \brief Adds the given CamSlot to this CI adapter
   */
  void AddCamSlot(cCamSlot *camSlot);

  DevicePtr m_assignedDevice;
  cCamSlot *m_camSlots[MAX_CAM_SLOTS_PER_ADAPTER];
};

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
  bool Assign(DevicePtr Device, bool Query = false);
       ///< Assigns this CAM slot to the given Device, if this is possible.
       ///< If Query is 'true', the CI adapter of this slot only checks whether
       ///< it can be assigned to the Device, but doesn't actually assign itself to it.
       ///< Returns true if this slot can be assigned to the Device.
       ///< If Device is NULL, the slot will be unassigned from any
       ///< device it was previously assigned to. The value of Query
       ///< is ignored in that case, and this function always returns
       ///< 'true'.
  DevicePtr Device(void);
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
  void AddChannel(const cChannel& Channel);
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

class cChannelCamRelation;

class cChannelCamRelations : public cList<cChannelCamRelation> {
private:
  PLATFORM::CMutex mutex;
  cChannelCamRelation *GetEntry(tChannelID ChannelID);
  cChannelCamRelation *AddEntry(tChannelID ChannelID);
  time_t lastCleanup;
  void Cleanup(void);
public:
  cChannelCamRelations(void);
  void Reset(int CamSlotNumber);
  bool CamChecked(tChannelID ChannelID, int CamSlotNumber);
  bool CamDecrypt(tChannelID ChannelID, int CamSlotNumber);
  void SetChecked(tChannelID ChannelID, int CamSlotNumber);
  void SetDecrypt(tChannelID ChannelID, int CamSlotNumber);
  void ClrChecked(tChannelID ChannelID, int CamSlotNumber);
  void ClrDecrypt(tChannelID ChannelID, int CamSlotNumber);
  };

extern cChannelCamRelations ChannelCamRelations;
}
