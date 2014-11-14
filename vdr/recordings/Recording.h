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

#include "RecordingTypes.h"
#include "channels/ChannelTypes.h"
#include "devices/TunerHandle.h"
#include "utils/DateTime.h"
#include "utils/Observer.h"

#include <string>

class TiXmlNode;

namespace VDR
{

class cRecorder;

class cRecording : public Observable
{
public:
  /*!
   * Construct a recording with default values or from exiting data. Recording
   * ID is initialised to RECORDING_INVALID_ID and must be set via SetID(). Once
   * set, recording ID is immutable.
   */
  cRecording(void);
  cRecording(const std::string&   strFoldername, /* requried */
             const std::string&   strTitle,      /* requried */
             const ChannelPtr&    channel,       /* required */
             const CDateTime&     startTime,     /* required */
             const CDateTime&     endTime,       /* required */
             const CDateTime&     expires        = CDateTime(),
             const CDateTimeSpan& resumePosition = CDateTimeSpan(),
             unsigned int         playCount      = 0,
             unsigned int         priority       = 0);

  virtual ~cRecording(void);

  static RecordingPtr EmptyRecording;

  /*!
   * Recording properties
   *   - ID:              Unique identifier for the recording; immutable once set
   *   - Folder name:     The foldername (under special://home/video) containing
   *                      recordings belonging to the timer
   *   - Title:           The title; used as the recording's filename
   *   - URL:             The full path of the recording
   *   - Channel:         Channel being recorded
   *   - Start time:      UTC time when the recording began
   *   - End time:        UTC time when the recording ended (must be > start time)
   *   - Expires:         UTC time when the recording expires
   *   - Resume Position: Last played position relative to start time
   *   - Play count:      As described
   *   - Priority:        ??? in the interval [0, 100]
   *   - IsPesRecording:  ???
   *   - FramesPerSecond: Set by recorder during recording
   */
  unsigned int         ID(void) const            { return m_id; }
  const std::string&   Foldername(void) const    { return m_strFoldername; }
  const std::string&   Title(void) const         { return m_strTitle; }
  std::string          URL(void) const;
  const ChannelPtr&    Channel(void) const       { return m_channel; }
  const CDateTime&     StartTime(void) const     { return m_startTime; }
  const CDateTime&     EndTime(void) const       { return m_endTime; }
  const CDateTime&     Expires(void) const       { return m_expires; }
  const CDateTimeSpan& ResumePositon(void) const { return m_resumePosition; }
  unsigned int         PlayCount(void) const     { return m_playCount; }
  unsigned int         Priority(void) const      { return m_priority; }
  bool                 IsPesRecording(void) const { return false; } // TODO

  /*!
   * Set the recording ID. This ID should be unique among recordings.
   *
   * The ID may only be set once. If the recording already has a valid ID then
   * SetID() has no effect and returns false.
   *
   * Setting the ID does not mark the recording as changed.
   */
  bool SetID(unsigned int id);

  /*!
   * Set recording properties. Properties can be set all at once via operator=(),
   * or sequentially via the Set*() methods. operator=() does not set the ID.
   *
   * Recording will be marked as changed if any properties (excluding ID) are
   * modified.
   */
  cRecording& operator=(const cRecording& rhs);

  void SetFoldername(const std::string& strFoldername);
  void SetTitle(const std::string& strTitle);
  void SetChannel(const ChannelPtr& channel);
  void SetTime(const CDateTime& startTime, const CDateTime& endTime);
  void SetExpires(const CDateTime& expires);
  void SetResumePosition(const CDateTimeSpan& resumePosition);
  void SetPlayCount(unsigned int playCount);
  void SetPriority(unsigned int priority);

  /*!
   * A recording is valid if:
   *   (1) m_id            is a valid ID (i.e. is not RECORDING_INVALID_ID)
   *   (2) m_strFoldername is a valid folder name
   *   (3) m_strTitle      is a valid file name
   *   (4) m_channel       is a valid channel
   *   (5) m_startTime     is a valid CDateTime object
   *   (6) m_endTime       is a valid CDateTime object
   *   (7) m_startTime < m_endTime
   *
   * HOWEVER, the ID check (1) may be skipped by setting bCheckID to false.
   * This allows the recording manager to check validity before assigning an ID.
   */
  bool IsValid(bool bCheckID = true) const;
  void LogInvalidProperties(bool bCheckID = true) const;

  /*!
   * Start/resume recording. Has no effect if already started.
   */
  void Resume(void);

  /*!
   * Stop/interrupt recording. Has no effect if already stopped.
   */
  void Interrupt(void);

  /*!
   * Serialise the recording. If serialisation succeeds, the serialised
   * recording is guaranteed to be valid (IsValid() returns true); otherwise,
   * false is returned.
   */
  bool Serialise(TiXmlNode* node) const;

  /*!
   * Deserialise the recording. If deserialisation succeeds, the deserialised
   * recording is guaranteed to be valid (IsValid() returns true); otherwise,
   * false is returned.
   *
   * Deserialisation will fail if the recording already has a valid ID.
   *
   * Upon deserialisation, the recording is marked as unchanged. (This mirrors
   * the state of a newly-constructed recording.)
   */
  bool Deserialise(const TiXmlNode* node);

private:
  unsigned int   m_id;
  std::string    m_strFoldername;
  std::string    m_strTitle;
  ChannelPtr     m_channel;
  CDateTime      m_startTime;
  CDateTime      m_endTime;
  CDateTime      m_expires;
  CDateTimeSpan  m_resumePosition;
  unsigned int   m_playCount;
  unsigned int   m_priority;
  float          m_fps;
  cRecorder*     m_recorder;
  TunerHandlePtr m_tunerHandle;
};

}
