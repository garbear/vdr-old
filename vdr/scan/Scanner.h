/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "ScanConfig.h"
#include "devices/TunerHandle.h"
#include "lib/platform/threads/threads.h"

namespace VDR
{

class cScanner : public PLATFORM::CThread, public iTunerHandleCallbacks
{
public:
  static cScanner& Get(void);
  virtual ~cScanner(void) { Stop(true); }

  /*!
   * Start the scan. Returns true if the scan was started, or false if a
   * previous scan is still running.
   */
  bool Start(const cScanConfig& setup);
  bool Start(void);
  void Stop(bool bWait);

  float GetFrequency() const { return m_frequencyHz; }
  unsigned int GetChannelNumber() const { return m_number; }
  float GetPercentage() const { return m_percentage; }

  void LockAcquired(void);
  void LockLost(void);
  void LostPriority(void);

protected:
  virtual void* Process(void);

private:
  cScanner(void);

  cScanConfig        m_setup;
  unsigned int       m_frequencyHz;
  unsigned int       m_number;
  float              m_percentage;
};

}
