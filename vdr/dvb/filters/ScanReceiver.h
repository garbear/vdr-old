/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "channels/ChannelTypes.h"
#include "devices/Receiver.h"
#include "dvb/PsiBuffer.h"

#include <map>
#include <set>
#include <vector>

namespace VDR
{
class cDevice;
class cSectionSyncer;

class cScanReceiver : public iReceiver
{
public:
  struct filter_properties
  {
    uint16_t pid;
    uint8_t  tid;
    uint8_t  mask;
  };

protected:
  cScanReceiver(cDevice* device, const std::string& name);
  cScanReceiver(cDevice* device, const std::string& name, const filter_properties& filter);
  cScanReceiver(cDevice* device, const std::string& name, const std::vector<filter_properties>& filters);
  cScanReceiver(cDevice* device, const std::string& name, size_t nbFilters, const filter_properties* filters);

public:
  virtual ~cScanReceiver(void);

  void Receive(const uint16_t pid, const uint8_t* data, const size_t len);
  virtual void ReceivePacket(const uint16_t pid, const uint8_t* data) = 0;
  virtual bool Attach(void);
  virtual void Detach(void);
  virtual bool WaitForScan(uint32_t iTimeout = TRANSPONDER_TIMEOUT);

  virtual bool Start(void) { return true; }
  virtual void Stop(void) {}
  virtual void LockAcquired(void);
  virtual void LockLost(void);
  virtual void LostPriority(void) { }

  virtual bool InATSC(void) const = 0;
  virtual bool InDVB(void) const = 0;
  virtual bool InChannelScan(void) const { return false; }

  virtual bool IsPsiReceiver(void) const { return true; }
  const std::string& Name(void) const { return m_name; }

  bool Scanned(void) const;
  bool Synced(uint16_t pid) const;

protected:
  bool Sync(uint16_t pid, uint8_t version, int sectionNumber, int endSectionNumber);
  virtual void AddFilter(const filter_properties& filter);
  virtual bool HasFilters(void) {  return !m_filtersAdded.empty(); }
  void RemoveFilter(const filter_properties& filter);
  void RemoveFilters(void);
  void SetScanned(void);
  void ResetScanned(void);
  bool DynamicFilter(const filter_properties& filter) const;

  cDevice*         m_device;
  bool             m_locked;
  PLATFORM::CMutex m_mutex;
  PLATFORM::CMutex m_scannedmutex;

private:
  bool                       m_scanned;
  PLATFORM::CCondition<bool> m_scannedEvent;
  std::set<filter_properties> m_filters;
  std::set<filter_properties> m_filtersAdded;
  bool                       m_attached;
  std::string                m_name;
  std::map<filter_properties, cSectionSyncer*> m_sectionSyncers;
};

}
