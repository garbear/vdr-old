/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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

#include "channels/ChannelTypes.h"
#include "devices/DeviceTypes.h"
#include "epg/EPGTypes.h"
#include "transponders/Transponder.h"

#include <stdint.h>
#include <vector>

namespace VDR
{
class cChannelID;

class iFilterCallback
{
public:
  virtual void OnChannelPropsScanned(const ChannelPtr& channel) { }
  virtual void OnChannelNamesScanned(const ChannelPtr& channel) { }
  virtual void OnEventScanned(const EventPtr& event) { }

  virtual ~iFilterCallback(void) { }
};

class cSectionSyncer
{
public:
  cSectionSyncer(void) { Reset(); }
  void Reset(void);

  enum SYNC_STATUS
  {
    SYNC_STATUS_NOT_SYNCED,  // Not synced yet (haven't encountered section number 0)
    SYNC_STATUS_NEW_VERSION, // Synced, and section has a new version and should be processed
    SYNC_STATUS_OLD_VERSION, // Synced, and section has a stale version and need not be processed
                             // (version is updated after sectionNumber == endSectionNumber)
  };

  SYNC_STATUS Sync(uint8_t version, int sectionNumber, int endSectionNumber);
  bool Synced(void) const { return m_bSynced; }

private:
  uint8_t m_previousVersion;
  bool    m_bSynced;
};

}
