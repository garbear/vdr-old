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

#include "devices/subsystems/DeviceChannelSubsystem.h"

#include <linux/dvb/frontend.h>

namespace VDR
{
class cChannel;
class cDvbTuner;

class cDvbChannelSubsystem : public cDeviceChannelSubsystem
{
public:
  cDvbChannelSubsystem(cDevice *device);
  virtual ~cDvbChannelSubsystem() { }

  // Not inherited from cDeviceChannelSubsystem
  virtual bool ProvidesDeliverySystem(fe_delivery_system_t deliverySystem) const;
  virtual bool ProvidesSource(TRANSPONDER_TYPE source) const;
  virtual bool ProvidesTransponder(const cChannel &channel) const;
  virtual bool ProvidesEIT() const;
  virtual unsigned int NumProvidedSystems() const;
  virtual int SignalStrength() const;
  virtual int SignalQuality() const;
  virtual cTransponder GetCurrentlyTunedTransponder() const;
  virtual bool IsTunedToTransponder(const cTransponder& transponder) const;
  bool SwitchChannel(const cChannel &channel, bool bLiveView);
  void ForceTransferMode();
  unsigned int Occupied() const;
  void SetOccupied(unsigned int seconds);
  virtual bool HasLock(void) const;

protected:
  virtual bool Tune(const cTransponder& transponder);
  virtual void ClearTransponder(void);
};
}
