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
#include "lib/platform/threads/mutex.h"
#include "utils/Observer.h"

#include <string>

namespace VDR
{

class CDateTime;

class cRecordingManager : public Observer, public Observable
{
public:
  static cRecordingManager& Get(void);

  ~cRecordingManager(void);

  RecordingPtr    GetByID(unsigned int id) const;
  RecordingPtr    GetByChannel(const ChannelPtr& channel, const CDateTime& time) const;
  RecordingVector GetRecordings(void) const;
  size_t          RecordingCount(void) const;

  /*!
   * Add a recording and assign it an ID. Fails if newRecording already has a
   * valid ID or if newRecording is invalid (IsValid(false) returns false).
   *
   * Returns true if newRecording is added. As a side effect, newRecording will
   * be assigned a valid ID.
   */
  bool AddRecording(const RecordingPtr& newRecording);

  /*!
   * Update the recording with the given ID. The ID of updatedRecording is
   * ignored.
   *
   * Returns false if there is no recording with the given ID. Returns true
   * otherwise (even if no properties are modified).
   */
  bool UpdateRecording(unsigned int id, const cRecording& updatedRecording);

  /*!
   * Remove the recording with the given ID. Returns true if the recording
   * was removed.
   */
  bool RemoveRecording(unsigned int id);

  void ResumeRecording(const RecordingPtr& recording);
  void InterruptRecording(const RecordingPtr& recording);

  // TODO
  size_t TotalFileSizeMB(bool bDeletedRecordings = false) { return 0; }
  double MBperMinute(void) { return -1; }

  virtual void Notify(const Observable& obs, const ObservableMessage msg);
  void NotifyObservers(void);

  bool Load(void);
  bool Load(const std::string& file);
  bool Save(const std::string& file = "");

private:
  cRecordingManager(void);

  RecordingMap     m_recordings;
  unsigned int     m_maxID;       // Monotonically increasing recording ID
  std::string      m_strFilename; // recordings.xml filename
  PLATFORM::CMutex m_mutex;
};

}
