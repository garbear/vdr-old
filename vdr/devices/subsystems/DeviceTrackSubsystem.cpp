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

#include "DeviceTrackSubsystem.h"
#include "DeviceAudioSubsystem.h"
#include "DevicePlayerSubsystem.h"
#include "DeviceReceiverSubsystem.h"
#include "DeviceSPUSubsystem.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "player.h"

#include <limits.h>

using namespace std;

cDeviceTrackSubsystem::cDeviceTrackSubsystem(cDevice *device)
 : cDeviceSubsystem(device),
   m_bKeepTracks(false), // used in ClrAvailableTracks()!
   m_currentAudioTrack(ttNone),
   m_currentSubtitleTrack(ttNone),
   m_currentAudioTrackMissingCount(0),
   m_bAutoSelectPreferredSubtitleLanguage(false),
   m_pre_1_3_19_PrivateStream(0)
{
  ClrAvailableTracks();
}

void cDeviceTrackSubsystem::ClrAvailableTracks(bool bDescriptionsOnly /* = false */, bool bIdsOnly /* = false */)
{
  if (m_bKeepTracks)
    return;
  if (bDescriptionsOnly)
  {
    for (int i = ttNone; i < ttMaxTrackTypes; i++)
      m_availableTracks[i].strDescription.clear();
  }
  else
  {
    for (int i = ttNone; i < ttMaxTrackTypes; i++)
    {
      m_availableTracks[i].id = 0;
      if (!bIdsOnly)
      {
        m_availableTracks[i].strDescription.clear();
        m_availableTracks[i].strLanguage.clear();
      }
    }
    m_pre_1_3_19_PrivateStream = 0;
    //Audio()->SetAudioChannel(0); // fall back to stereo // TODO: Can't call from constructor (references m_device)
    m_currentAudioTrackMissingCount = 0;
    m_currentAudioTrack = ttNone;
    m_currentSubtitleTrack = ttNone;
  }
}

bool cDeviceTrackSubsystem::SetAvailableTrack(eTrackType type, int index, uint16_t id, const string &strLanguage /* = "" */, const string &strDescription /* = "" */)
{
  eTrackType t = (eTrackType)(type + index);
  if ((type == ttAudio && IS_AUDIO_TRACK(t)) ||
      (type == ttDolby && IS_DOLBY_TRACK(t)) ||
      (type == ttSubtitle && IS_SUBTITLE_TRACK(t)))
  {
    if (!strLanguage.empty())
      m_availableTracks[t].strLanguage = strLanguage;
    if (!strDescription.empty())
      m_availableTracks[t].strDescription = strDescription;
    if (id)
    {
      m_availableTracks[t].id = id; // setting 'id' last to avoid the need for extensive locking
      if (type == ttAudio || type == ttDolby)
      {
        int numAudioTracks = NumAudioTracks();
        if (!m_availableTracks[m_currentAudioTrack].id && numAudioTracks && m_currentAudioTrackMissingCount++ > numAudioTracks * 10)
          EnsureAudioTrack();
        else if (t == m_currentAudioTrack)
          m_currentAudioTrackMissingCount = 0;
      }
      else if (type == ttSubtitle && m_bAutoSelectPreferredSubtitleLanguage)
        EnsureSubtitleTrack();
    }
    return true;
  }
  else
    esyslog("ERROR: SetAvailableTrack called with invalid Type/Index (%d/%d)", type, index);
  return false;
}

bool cDeviceTrackSubsystem::GetTrack(eTrackType type, tTrackId &track) const
{
  if (ttNone < type && type < ttMaxTrackTypes)
  {
    track = m_availableTracks[type];
    return true;
  }
  return false;
}

unsigned int cDeviceTrackSubsystem::NumTracks(eTrackType firstTrack, eTrackType lastTrack) const
{
  unsigned int n = 0;
  for (unsigned int i = firstTrack; i <= lastTrack; i++)
  {
    if (m_availableTracks[i].id)
      n++;
  }
  return n;
}

bool cDeviceTrackSubsystem::SetCurrentAudioTrack(eTrackType type)
{
  if (ttNone < type && type <= ttDolbyLast)
  {
    PLATFORM::CLockObject lock(m_mutexCurrentAudioTrack);
    if (IS_DOLBY_TRACK(type))
      Audio()->SetDigitalAudioDevice(true);
    m_currentAudioTrack = type;
    if (Player()->m_player)
    {
      tTrackId trackId;
      bool success = GetTrack(m_currentSubtitleTrack, trackId);
      Player()->m_player->SetAudioTrack(m_currentAudioTrack, success ? &trackId : NULL);
    }
    else
      SetAudioTrackDevice(m_currentAudioTrack);
    if (IS_AUDIO_TRACK(type))
      Audio()->SetDigitalAudioDevice(false);
    return true;
  }
  return false;
}

bool cDeviceTrackSubsystem::SetCurrentSubtitleTrack(eTrackType type, bool bManual /* = false */)
{
  if (type == ttNone || IS_SUBTITLE_TRACK(type))
  {
    m_currentSubtitleTrack = type;
    m_bAutoSelectPreferredSubtitleLanguage = !bManual;
    if (Player()->m_player)
    {
      tTrackId TrackId;
      bool success = GetTrack(m_currentSubtitleTrack, TrackId);
      Player()->m_player->SetSubtitleTrack(m_currentSubtitleTrack, success ? &TrackId : NULL);
    }
    else
      SetSubtitleTrackDevice(m_currentSubtitleTrack);
    return true;
  }
  return false;
}

void cDeviceTrackSubsystem::EnsureAudioTrack(bool bForce /* = false */)
{
  if (m_bKeepTracks)
    return;
  if (bForce || !m_availableTracks[m_currentAudioTrack].id)
  {
    eTrackType PreferredTrack = ttAudioFirst;
    int PreferredAudioChannel = 0;
    int LanguagePreference = -1;
    /* TODO
    int StartCheck = Setup.CurrentDolby ? ttDolbyFirst : ttAudioFirst;
    int EndCheck = ttDolbyLast;
    for (int i = StartCheck; i <= EndCheck; i++)
    {
      tTrackId TrackId;
      int pos = 0;
      if (GetTrack(eTrackType(i), TrackId) && TrackId.id && I18nIsPreferredLanguage(Setup.AudioLanguages, TrackId.strLanguage.c_str(), LanguagePreference, &pos))
      {
        PreferredTrack = eTrackType(i);
        PreferredAudioChannel = pos;
      }
      if (Setup.CurrentDolby && i == ttDolbyLast)
      {
        i = ttAudioFirst - 1;
        EndCheck = ttAudioLast;
      }
    }
    */

    // Make sure we're set to an available audio track:
    tTrackId Track;
    if (bForce || !GetTrack(GetCurrentAudioTrack(), Track) || !Track.id || PreferredTrack != GetCurrentAudioTrack())
    {
      if (!bForce) // only log this for automatic changes
        dsyslog("setting audio track to %d (%d)", PreferredTrack, PreferredAudioChannel);
      SetCurrentAudioTrack(PreferredTrack);
      Audio()->SetAudioChannel(PreferredAudioChannel);
    }
  }
}

void cDeviceTrackSubsystem::EnsureSubtitleTrack()
{
  // XXX remove this method
}
