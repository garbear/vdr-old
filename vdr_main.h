#pragma once

#include <string>
#include <termios.h>
#include <time.h>

#include "xbmc/xbmc_addon_types.h"
#include "xbmc/libXBMC_addon.h"
#include "platform/threads/threads.h"

#include "vdr/settings/Settings.h"

class cEpgDataReader;
class cOsdObject;
class cSkin;
class cPluginManager;

class cVDRDaemon : public PLATFORM::CThread
{
public:
  static cVDRDaemon& Get(void);
  virtual ~cVDRDaemon(void);

  int ReadCommandLineOptions(int argc, char *argv[]);
  int Init(void);
  void* Process(void);
  void Iterate(void);

  cSettings       m_settings;

private:
  cVDRDaemon(void);

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

extern std::string                   g_strUserPath;
extern std::string                   g_strClientPath;
extern ADDON::CHelper_libXBMC_addon* XBMC;
