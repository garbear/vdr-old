#pragma once

#include <termios.h>
#include <time.h>

class cEpgDataReader;
class cOsdObject;
class cSkin;
class cPluginManager;

class cVDRDaemon
{
public:
  cVDRDaemon(void);
  virtual ~cVDRDaemon(void);

  struct termios m_savedTm;
  bool           m_HasStdin;
  bool           m_StartedAsRoot;
  const char *   m_VdrUser;
  bool           m_UserDump;
  int            m_SVDRPport;
  const char *   m_AudioCommand;
  const char *   m_VideoDirectory;
  const char *   m_ConfigDirectory;
  const char *   m_CacheDirectory;
  const char *   m_ResourceDirectory;
  const char *   m_LocaleDirectory;
  const char *   m_EpgDataFileName;
  bool           m_DisplayHelp;
  bool           m_DisplayVersion;
  bool           m_DaemonMode;
  int            m_SysLogTarget;
  bool           m_MuteAudio;
  int            m_WatchdogTimeout;
  const char *   m_Terminal;
  bool           m_UseKbd;
  const char *   m_LircDevice;
  cPluginManager* m_pluginManager;

  int ReadCommandLineOptions(int argc, char *argv[]);
  int Init(void);
  void Process(void);
private:
  bool SetUser(const char *UserName, bool UserDump);
  bool DropCaps(void);
  bool SetKeepCaps(bool On);

  cEpgDataReader* m_EpgDataReader;
  cOsdObject*     m_Menu;
  int             m_LastChannel;
  int             m_LastTimerChannel;
  int             m_PreviousChannel[2];
  int             m_PreviousChannelIndex;
  time_t          m_LastChannelChanged;
  time_t          m_LastInteract;
  int             m_MaxLatencyTime;
  bool            m_InhibitEpgScan;
  bool            m_IsInfoMenu;
  cSkin*          m_CurrentSkin;
};
