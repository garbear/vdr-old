#pragma once

#include <string>
#include <termios.h>
#include <time.h>

#include "xbmc/xbmc_addon_types.h"
#include "xbmc/libXBMC_addon.h"
#include "platform/threads/threads.h"
#include "platform/threads/mutex.h"

#include "vdr/settings/Settings.h"
#include "vdr/SignalHandler.h"

class cEpgDataReader;
class cOsdObject;
class cSkin;
class cPluginManager;

class cVDRDaemon : public PLATFORM::CThread, public ISignalReceiver
{
public:
  static cVDRDaemon& Get(void);
  virtual ~cVDRDaemon(void);

  bool ReadCommandLineOptions(int argc, char *argv[]);
  bool Init(void);
  void* Process(void);
  bool Iterate(void);

  void WaitForShutdown();

  cSettings       m_settings;

protected:
  /*!
   * \brief Inherited from ISignalReceiver
   */
  virtual void OnSignal(int signum);

private:
  cVDRDaemon(void);

  void DirectMainFunction(eOSState function);

  /*!
   \return true if the input has been handled, and Iterate() should return true
   */
  bool HandleInput(time_t Now);

  cEpgDataReader* m_EpgDataReader;
  cOsdObject*     m_Menu;
  int             m_LastChannel;
  int             m_LastTimerChannel;
  int             m_PreviousChannel[2];
  int             m_PreviousChannelIndex;
  time_t          m_LastChannelChanged;
  time_t          m_LastInteract;
  bool            m_InhibitEpgScan;
  bool            m_IsInfoMenu;
  cSkin*          m_CurrentSkin;
  int             m_LastSignal;
  PLATFORM::CMutex m_critSection;
  PLATFORM::CCondition<bool> m_exitCondition;
  bool            m_finished;
};

extern std::string                   g_strUserPath;
extern std::string                   g_strClientPath;
extern ADDON::CHelper_libXBMC_addon* XBMC;
