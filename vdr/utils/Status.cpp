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

#include "Status.h"

namespace VDR
{

// --- cStatus ---------------------------------------------------------------

cList<cStatus> cStatus::statusMonitors;

cStatus::cStatus(void)
{
  statusMonitors.Add(this);
}

cStatus::~cStatus()
{
  statusMonitors.Del(this, false);
}

void cStatus::MsgTimerChange(const TimerPtr Timer, eTimerChange Change)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->TimerChange(Timer, Change);
}

void cStatus::MsgChannelSwitch(const cDevice *Device, int ChannelNumber)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->ChannelSwitch(Device, ChannelNumber);
}

void cStatus::MsgRecording(const cDevice *Device, const char *Name, const char *FileName, bool On)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->Recording(Device, Name, FileName, On);
}

void cStatus::MsgReplaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->Replaying(Control, Name, FileName, On);
}

void cStatus::MsgSetVolume(int Volume, bool Absolute)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetVolume(Volume, Absolute);
}

void cStatus::MsgSetAudioTrack(int Index, const char * const *Tracks)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetAudioTrack(Index, Tracks);
}

void cStatus::MsgSetAudioChannel(int AudioChannel)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetAudioChannel(AudioChannel);
}

void cStatus::MsgSetSubtitleTrack(int Index, const char * const *Tracks)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetSubtitleTrack(Index, Tracks);
}

void cStatus::MsgOsdClear(void)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdClear();
}

void cStatus::MsgOsdTitle(const char *Title)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdTitle(Title);
}

void cStatus::MsgOsdStatusMessage(const char *Message)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdStatusMessage(Message);
}

void cStatus::MsgOsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdHelpKeys(Red, Green, Yellow, Blue);
}

void cStatus::MsgOsdItem(const char *Text, int Index)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdItem(Text, Index);
}

void cStatus::MsgOsdCurrentItem(const char *Text)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdCurrentItem(Text);
}

void cStatus::MsgOsdTextItem(const char *Text, bool Scroll)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdTextItem(Text, Scroll);
}

void cStatus::MsgOsdChannel(const char *Text)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdChannel(Text);
}

void cStatus::MsgOsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdProgramme(PresentTime, PresentTitle, PresentSubtitle, FollowingTime, FollowingTitle, FollowingSubtitle);
}

}
