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

#include "channels/Channel.h"

#include <stdint.h>
#include <sys/types.h>
#include <vector>

namespace VDR
{
class cSectionSyncer
{
public:
  cSectionSyncer(void);
  void Reset(void);
  bool Sync(uchar version, int number, int lastNumber);

private:
  int lastVersion;
  bool synced;
};

class cDevice;
class cFilterData;

class cFilter
{
public:
  cFilter(cDevice* device);
  cFilter(cDevice* device, u_short pid, u_char tid, u_char mask = 0xFF);
  virtual ~cFilter(void);

  void CloseHandles(void);

  bool Matches(u_short Pid, u_char Tid);
       ///< Indicates whether this filter wants to receive data from the given Pid/Tid.
       ///< If SetStatus() changed the status to off, this will return false.

  virtual void ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data) = 0;
       ///< Processes the data delivered to this filter.
       ///< Pid and Tid is one of the combinations added to this filter by
       ///< a previous call to Add(), Data is a pointer to Length bytes of
       ///< data. This function will be called from the section handler's
       ///< thread, so it has to use proper locking mechanisms in case it
       ///< accesses any global data. It is guaranteed that if several cFilters
       ///< are attached to the same cSectionHandler, only _one_ of them has
       ///< its ProcessData() function called at any given time. It is allowed
       ///< that more than one cFilter are set up to receive the same Pid/Tid.
       ///< The ProcessData() function must return as soon as possible.

  virtual void Enable(bool bEnabled);
       ///< Turns this filter on or off. If the filter is turned off, any filter data
       ///< that has been added without the Sticky parameter set to 'true' will be
       ///< automatically deleted. Those parameters that have been added with Sticky
       ///< set to 'true' will be automatically reused when Enable() is called.

  const std::vector<cFilterData>& GetFilterData() const { return m_data; }

protected:
  int Source(void);
       ///< Returns the source of the data delivered to this filter.
  int Transponder(void);
       ///< Returns the transponder of the data delivered to this filter.
  ChannelPtr Channel(void);
       ///< Returns the channel of the data delivered to this filter.

  void Set(u_short pid, u_char tid, u_char mask = 0xFF, bool bSticy = true);
       ///< Adds the given filter data to this filter.
       ///< If Sticky is true, this will survive a status change, otherwise
       ///< it will be automatically deleted.
  void Del(u_short pid, u_char tid, u_char mask = 0xFF);
       ///< Deletes the given filter data from this filter.

private:
  cDevice* const           m_device;
  std::vector<cFilterData> m_data;
  bool                     m_bEnabled;
};

}
