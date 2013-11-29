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
#include "config.h"
#include "thread.h"
#include "tools.h" // for cHash

#include <string>

class cChannels : public cRwLock, public cList<cChannel>
{
public:
  cChannels();

  bool Load(const std::string &fileName, bool bMustExist = false);
  void HashChannel(cChannel *Channel);
  void UnhashChannel(cChannel *Channel);

  int GetNextGroup(int Idx);   // Get next channel group
  int GetPrevGroup(int Idx);   // Get previous channel group
  int GetNextNormal(int Idx);  // Get next normal channel (not group)
  int GetPrevNormal(int Idx);  // Get previous normal channel (not group)
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

  cChannel *NewChannel(const cChannel *Transponder, const char *Name, const char *ShortName, const char *Provider, int Nid, int Tid, int Sid, int Rid = 0);

  // Inherited from cConfig<cChannel>
  const std::string &FileName() { return m_fileName; }

  bool Save(void)
  {
    bool result = true;
    cChannel* l = First();
    cSafeFile f(m_fileName.c_str());
    if (f.Open())
    {
      while (l)
      {
        if (!l->Save(f))
        {
          result = false;
          break;
        }
        l = Next(l);
      }
      if (!f.Close())
        result = false;
    }
    else
      result = false;
    return result;
  }

private:
  void DeleteDuplicateChannels();

  int             maxNumber;
  int             maxChannelNameLength;
  int             maxShortChannelNameLength;
  int             modified;
  int             beingEdited;
  cHash<cChannel> channelsHashSid;

  // Inherited from cConfig<cChannel>
  std::string m_fileName;
  bool m_bAllowComments;
  void Clear(void)
  {
    m_fileName.clear();
    cList<cChannel>::Clear();
  }
};

extern cChannels Channels;

std::string ChannelString(const cChannel *Channel, int Number);

