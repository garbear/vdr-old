/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "Channel.h"
#include "Config.h"
#include "thread.h"
#include "utils/Tools.h" // for cHash
#include "utils/Observer.h"

#include <string>
#include <vector>

class cScanList;

class cChannels : public cRwLock, public Observer
{
public:
  cChannels();
  ~cChannels() { Clear(); }

  bool Load(const std::string &fileName, bool bMustExist = false);
  bool Save(void);
  void HashChannel(cChannel *Channel);
  void UnhashChannel(cChannel *Channel);

  void Notify(const Observable &obs, const ObservableMessage msg);
  void AddChannel(cChannel* channel);
  void RemoveChannel(cChannel* channel);

  int GetNextGroup(unsigned int index);   // Get next channel group
  int GetPrevGroup(unsigned int index);   // Get previous channel group
  int GetNextNormal(unsigned int index);  // Get next normal channel (not group)
  int GetPrevNormal(unsigned int index);  // Get previous normal channel (not group)
  void ReNumber();             // Recalculate 'number' based on channel type

  cChannel *GetByNumber(int Number, int SkipGap = 0);
  cChannel *GetByServiceID(int Source, int Transponder, unsigned short ServiceID);
  cChannel *GetByChannelID(tChannelID ChannelID, bool TryWithoutRid = false, bool TryWithoutPolarization = false);
  cChannel *GetByChannelID(int nid, int tid, int sid);
  cChannel *GetByTransponderID(tChannelID ChannelID);

  int BeingEdited() { return beingEdited; }
  void IncBeingEdited() { beingEdited++; }
  void DecBeingEdited() { beingEdited--; }

  bool HasUniqueChannelID(cChannel *NewChannel, cChannel *OldChannel = NULL);
  bool SwitchTo(int Number);

  int MaxNumber() { return maxNumber; }

  int MaxChannelNameLength();
  int MaxShortChannelNameLength();
  void SetModified(bool ByUser = false);

  /*!
   * \brief Returns 0 if no channels have been modified, 1 if an automatic
   *        modification has been made, and 2 if the user has made a modification.
   *        Calling this function resets the 'modified' flag to 0.
   */
  int Modified();

  cChannel *NewChannel(const cChannel& Transponder, const std::string& Name, const std::string& ShortName, const std::string& Provider, int Nid, int Tid, int Sid, int Rid = 0);

  // Inherited from cConfig<cChannel>
  const std::string &FileName() { return m_fileName; }

  void AddTransponders(cScanList* list) const;

private:
  std::vector<cChannel*> m_channels;
  int             maxNumber;
  int             maxChannelNameLength;
  int             maxShortChannelNameLength;
  int             modified;
  int             beingEdited;
  cHash<cChannel> channelsHashSid;

  // Inherited from cConfig<cChannel>
  std::string m_fileName;
  void Clear(void);
};

extern cChannels Channels;

std::string ChannelString(const cChannel *Channel, int Number);

