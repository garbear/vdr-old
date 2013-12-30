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
