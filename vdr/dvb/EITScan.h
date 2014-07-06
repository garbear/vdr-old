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

#include "Config.h"
#include "devices/Device.h"
#include "devices/DeviceTypes.h"
#include "channels/ChannelTypes.h"
#include "platform/threads/threads.h"

#include <time.h>

namespace VDR
{

class cEITScanner : public PLATFORM::CThread
{
public:
  cEITScanner(void);
  virtual ~cEITScanner(void) { }

  static cEITScanner& Get();

  void ForceScan(void);

protected:
  virtual void* Process(void);

private:
  /*!
   * If m_scanList is empty, this will populate it with channels from
   * m_transponderList and channels being tracked by cChannelManager.
   */
  void CreateScanList(void);

  void AddTransponder(const ChannelPtr& transponder);

  DevicePtr          m_device;
  PLATFORM::CTimeout m_nextFullScan;
  ChannelVector      m_scanList;
  ChannelVector      m_transponderList;
  bool               m_bScanFinished;
};

}
