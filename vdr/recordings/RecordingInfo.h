#pragma once

#include "channels/Channel.h"
#include "channels/ChannelID.h"
#include <stddef.h>

class cEvent;
class CEpgComponents;
class CFile;

#define INFOFILESUFFIX    "/info"

class cRecordingInfo
{
  friend class cRecording;
public:
  cRecordingInfo(const std::string& strFileName);
  ~cRecordingInfo();
  tChannelID ChannelID(void) const { return channelID; }
  const char *ChannelName(void) const { return channelName; }
  const cEvent *GetEvent(void) const { return event; }
  std::string Title(void) const;
  std::string ShortText(void) const;
  std::string Description(void) const;
  const CEpgComponents *Components(void) const;
  const char *Aux(void) const { return aux; }
  double FramesPerSecond(void) const { return framesPerSecond; }
  void SetFramesPerSecond(double FramesPerSecond);
  bool Write(CFile& file, const char *Prefix = "") const;
  bool Read(void);
  bool Write(void) const;

private:
  tChannelID channelID;
  char *channelName;
  const cEvent *event;
  cEvent *ownEvent;
  char *aux;
  double framesPerSecond;
  int priority;
  int lifetime;
  char *fileName;
  cRecordingInfo(const cChannel *Channel = NULL, const cEvent *Event = NULL);
  bool Read(CFile& file);
  void SetData(const char *Title, const char *ShortText, const char *Description);
  void SetAux(const char *Aux);
};
