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

#include "DeviceAudioSubsystem.h"
#include "audio.h" // For Audios
#include "Config.h" // For Setup
#include "utils/Status.h" // For cStatus

#define CONSTRAIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))

cDeviceAudioSubsystem::cDeviceAudioSubsystem(cDevice *device)
 : cDeviceSubsystem(device),
   m_bMute(false)//,
   //m_volume(Setup.CurrentVolume) // TODO
{
}

bool cDeviceAudioSubsystem::ToggleMute()
{
  unsigned int oldVolume = m_volume;
  m_bMute = !m_bMute;

  //XXX why is it necessary to use different sequences???
  if (m_bMute)
  {
    SetVolume(0, true);
    //Audios.MuteAudio(m_bMute); // Mute external audio after analog audio // TODO
  }
  else
  {
    //Audios.MuteAudio(m_bMute); // Enable external audio before analog audio // TODO
    SetVolume(oldVolume, true);
  }
  m_volume = oldVolume;
  return m_bMute;
}

int cDeviceAudioSubsystem::GetAudioChannel()
{
  int c = GetAudioChannelDevice();
  return (0 <= c && c <= 2) ? c : 0;
}

void cDeviceAudioSubsystem::SetAudioChannel(int audioChannel)
{
  if (0 <= audioChannel && audioChannel <= 2)
    SetAudioChannelDevice(audioChannel);
}

void cDeviceAudioSubsystem::SetVolume(int volume, bool bAbsolute)
{
  unsigned int oldVolume = m_volume;
  m_volume = CONSTRAIN(bAbsolute ? volume : m_volume + volume, 0, MAXVOLUME);
  SetVolumeDevice(m_volume);
  bAbsolute |= m_bMute;
  cStatus::MsgSetVolume(bAbsolute ? m_volume : m_volume - oldVolume, bAbsolute);
  if (m_volume > 0)
  {
    m_bMute = false;
    //Audios.MuteAudio(m_bMute); // TODO
  }
}
