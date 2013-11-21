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
#pragma once

#include "../../devices/DeviceSubsystem.h"
#include "../../../receiver.h"
#include "../../../tools.h"

#include <string>
#include <vector>

enum eTrackType
{
  ttNone,
  ttAudio,
  ttAudioFirst = ttAudio,
  ttAudioLast  = ttAudioFirst + 31, // MAXAPIDS - 1
  ttDolby,
  ttDolbyFirst = ttDolby,
  ttDolbyLast  = ttDolbyFirst + 15, // MAXDPIDS - 1
  ttSubtitle,
  ttSubtitleFirst = ttSubtitle,
  ttSubtitleLast  = ttSubtitleFirst + 31, // MAXSPIDS - 1
  ttMaxTrackTypes
};

#define IS_AUDIO_TRACK(t) (ttAudioFirst <= (t) && (t) <= ttAudioLast)
#define IS_DOLBY_TRACK(t) (ttDolbyFirst <= (t) && (t) <= ttDolbyLast)
#define IS_SUBTITLE_TRACK(t) (ttSubtitleFirst <= (t) && (t) <= ttSubtitleLast)

struct tTrackId
{
  tTrackId() : id(0) { }
  uint16_t    id;               // The PES packet id or the PID.
  std::string strLanguage;      // something like either "eng" or "deu+eng". Up to two 3 letter language codes, separated by '+'
  std::string strDescription;   // something like "Dolby Digital 5.1"
};

class cLiveSubtitle : public cReceiver
{
public:
  cLiveSubtitle(int SPid) { AddPid(SPid); }
  virtual ~cLiveSubtitle() { cReceiver::Detach(); }

protected:
  virtual void Receive(uchar *data, int length);
  virtual void Receive(const std::vector<uchar> &data);
};

class cDeviceTrackSubsystem : protected cDeviceSubsystem
{
protected:
  cDeviceTrackSubsystem(cDevice *device);

public:
  virtual ~cDeviceTrackSubsystem();

  /*!
   * \brief Clears the list of currently available tracks
   * \param bDescriptionsOnly Only clear the track descriptions
   * \param bIdsOnly Only clear the IDs. Ignored if bDescriptionsOnly is true
   */
  void ClrAvailableTracks(bool bDescriptionsOnly = false, bool bIdsOnly = false);

  /*!
   * \brief Sets the track of the given Type and Index to the given values
   * \param type One of the basic eTrackType values, like ttAudio or ttDolby
   * \param index Tells which track of the given basic type is meant
   * \param id The ID, or 0 to leave existing ID untouched
   * \param strLanguage The language, only set if ID is 0 (TODO: Is this false?)
   * \param strDescription The description, only set if ID is 0 (TODO: Is this false?)
   * \return True if the track was set correctly, false otherwise
   */
  bool SetAvailableTrack(eTrackType type, int index, uint16_t id, const std::string &strLanguage = "", const std::string &strDescription = "");

  /*!
   * \brief Returns a pointer to the given track id, or NULL if Type is not less than ttMaxTrackTypes
   */
  bool GetTrack(eTrackType type, tTrackId &track) const;

  /*!
   * \brief Returns the number of tracks in the given range that are currently available
   *
   * NumAudioTracks() and NumSubtitleTracks() are convenience functions to
   * quickly find out whether there is more than one audio or subtitle track.
   */
  unsigned int NumTracks(eTrackType firstTrack, eTrackType lastTrack) const;
  unsigned int NumAudioTracks() const { return NumTracks(ttAudioFirst, ttDolbyLast); }
  unsigned int NumSubtitleTracks() const { return NumTracks(ttSubtitleFirst, ttSubtitleLast); }

  eTrackType GetCurrentAudioTrack() const { return m_currentAudioTrack; }

  /*!
   * \brief Sets the current audio track to the given type
   * \return True if type is a valid audio track, false otherwise.
   */
  bool SetCurrentAudioTrack(eTrackType type);

  eTrackType GetCurrentSubtitleTrack() const { return m_currentSubtitleTrack; }

  /*!
   * \brief Sets the current subtitle track to the given type
   * \param bManual If true, no automatic preferred subtitle language selection
   *        will be done for the rest of the current replay session, or until
   *        the channel is changed
   * \return True if type is a valid subtitle track, false otherwise
   */
  bool SetCurrentSubtitleTrack(eTrackType type, bool bManual = false);

  /*!
   * \brief Makes sure an audio track is selected that is actually available
   * \param bForce If true, the language and Dolby Digital settings will be
   *        verified even if the current audio track is available
   */
  void EnsureAudioTrack(bool bForce = false);

  /*!
   * \brief Makes sure one of the preferred language subtitle tracks is selected
   *
   * Only has an effect if Setup.DisplaySubtitles is on.
   */
  void EnsureSubtitleTrack();

  /*!
   * \brief Controls whether the current audio and subtitle track settings shall
   *        be kept as they currently are, or if they shall be automatically
   *        adjusted. This is used when pausing live video.
   */
  void SetKeepTracks(bool bKeepTracks) { m_bKeepTracks = bKeepTracks; }

protected:
  /*!
   * \brief Sets the current audio track to the given value
   *
   * TODO: Unused
   */
  virtual void SetAudioTrackDevice(eTrackType type) { }

  /*!
   * \brief Sets the current subtitle track to the given value
   *
   * TODO: Unused
   */
  virtual void SetSubtitleTrackDevice(eTrackType type) { }

private:
  bool         m_bKeepTracks;
  tTrackId     m_availableTracks[ttMaxTrackTypes];
  eTrackType   m_currentAudioTrack;
  eTrackType   m_currentSubtitleTrack;
  cMutex       m_mutexCurrentAudioTrack;
  cMutex       m_mutexCurrentSubtitleTrack;
  unsigned int m_currentAudioTrackMissingCount;
  bool         m_bAutoSelectPreferredSubtitleLanguage;
  int          m_pre_1_3_19_PrivateStream;
};
