#pragma once

#include "channels/Channel.h"
#include "channels/ChannelID.h"
#include <stddef.h>

class cEvent;
class CEpgComponents;
class CFile;

#define INFOFILESUFFIX    "/metadata.xml"

class cRecordingInfo
{
public:
  cRecordingInfo(ChannelPtr channel, const cEvent *Event = NULL);
  cRecordingInfo(const std::string& strFileName);
  ~cRecordingInfo();
  tChannelID ChannelID(void) const { return m_channelID; }
  std::string ChannelName(void) const { return m_channel ? m_channel->Name() : ""; }
  const cEvent *GetEvent(void) const { return m_event; }
  void SetEvent(const cEvent* event);
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
  tChannelID    m_channelID;
  const cEvent* m_event;
  cEvent *      m_ownEvent;
  double        m_dFramesPerSecond;
  int           m_iPriority;
  int           m_iLifetime;
  mutable std::string m_strFilename; //XXX
};
