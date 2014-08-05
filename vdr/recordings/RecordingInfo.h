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

#include "channels/Channel.h"
#include "channels/ChannelID.h"
#include "channels/ChannelTypes.h"
#include "epg/EPGTypes.h"
#include "epg/Event.h"

#include <stddef.h>

namespace VDR
{
class CEpgComponents;
class CFile;

#define INFOFILESUFFIX    "/metadata.xml"

class cRecordingInfo
{
public:
  cRecordingInfo(ChannelPtr channel, const EventPtr& Event = cEvent::EmptyEvent);
  cRecordingInfo(const std::string& strFileName);
  ~cRecordingInfo() { }
  const cChannelID& ChannelID(void) const { return m_channelID; }
  std::string ChannelName(void) const { return m_channel ? m_channel->Name() : ""; }
  EventPtr GetEvent(void) const { return m_event; }
  void SetEvent(const EventPtr& event);
  std::string Title(void) const;
  void SetTitle(const std::string& strTitle);
  std::string ShortText(void) const;
  std::string Description(void) const;
  const CEpgComponents *Components(void) const;
  double FramesPerSecond(void) const { return m_dFramesPerSecond; }
  void SetFramesPerSecond(double FramesPerSecond);
  int Priority(void) const { return m_iPriority; }
  void SetPriority(int iPriority) { m_iPriority = iPriority; }
  int Lifetime(void) const { return m_iLifetime; }
  void SetLifetime(int iLifetime) { m_iLifetime = iLifetime; }

  bool Read(const std::string& strFilename = "");
  bool Write(const std::string& strFilename = "") const;

private:
  void SetData(const char *Title, const char *ShortText, const char *Description);

  ChannelPtr    m_channel;
  cChannelID    m_channelID;
  EventPtr      m_event;
  EventPtr      m_ownEvent;
  double        m_dFramesPerSecond;
  int           m_iPriority;
  int           m_iLifetime;
  mutable std::string m_strFilename; //XXX
};
}
