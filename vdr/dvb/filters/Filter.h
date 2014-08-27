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

private:
  uint8_t m_previousVersion;
  bool    m_bSynced;
};

class cDevice;

/*!
 * Parent class for DVB section filters. Encapsulates interaction with the
 * device they were created with.
 */
class cFilter
{
public:
  /*!
   * Construct with a pointer to the device that owns the filter. Must remain
   * valid for the life of this filter.
   */
  cFilter(cDevice* device);
  virtual ~cFilter(void);

  /*!
   * Get the resources that have been opened as a result of OpenResource().
   */
  const PidResourceSet& GetResources(void) const { return m_resources; }

protected:
  /*!
   * Open a resource so that it will be included in the resources used by the
   * next call to GetSection(). This should be called from the constructor of
   * the derived class.
   */
  void OpenResource(uint16_t pid, uint8_t tid, uint8_t mask = 0xFF);
  void CloseResource(uint16_t pid);

  /*!
   * Get a section from the resources that have been opened by this filter.
   * Blocks until a section is received! The section's PID is placed in pid
   * and the section's data is placed in data. If no sections are received
   * (possibly due to a timeout), this returns false and pid/data are left
   * untouched.
   */
  bool GetSection(uint16_t& pid, std::vector<uint8_t>& data);

  /*!
   * Get the device that owns this filter.
   */
  cDevice* GetDevice(void) const { return m_device; }

  /*!
   * Get the channel that this filter's device is tuned to, or an empty pointer
   * if the device is not tuned to a channel.
   */
  cTransponder GetTransponder(void) const { return m_transponder; }

private:
  cDevice* const     m_device;      // Device that this filter belongs to
  const cTransponder m_transponder; // Transponder that the device is tuned to
  PidResourceSet     m_resources;   // Open resources held by this device
};

}
