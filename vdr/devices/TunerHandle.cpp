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

#include "TunerHandle.h"
#include "subsystems/DeviceChannelSubsystem.h"

using namespace VDR;

const TunerHandlePtr cTunerHandle::EmptyHandle;

cTunerHandle::cTunerHandle(device_tuning_type_t type, cDeviceChannelSubsystem* tuner, iTunerHandleCallbacks* callbacks, const ChannelPtr& channel) :
    m_type(type),
    m_tuner(tuner),
    m_callbacks(callbacks),
    m_channel(channel)
{
}

cTunerHandle::~cTunerHandle(void)
{
}

void cTunerHandle::LockAcquired(void)
{
  if (m_callbacks)
    m_callbacks->LockAcquired();
}

void cTunerHandle::LockLost(void)
{
  if (m_callbacks)
    m_callbacks->LockLost();
}

void cTunerHandle::LostPriority(void)
{
  if (m_callbacks)
    m_callbacks->LostPriority();
}

void cTunerHandle::Release(bool notify /* = true */)
{
  if (m_tuner)
    m_tuner->Release(this, notify);
}
