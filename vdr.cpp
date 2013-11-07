/*
 * vdr.c: Video Disk Recorder main program
 *
 * Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * The author can be reached at vdr@tvdr.de
 *
 * The project's page is at http://www.tvdr.de
 *
 * $Id: vdr.c 2.57.1.1 2013/10/16 09:46:36 kls Exp $
 */

#ifndef ANDROID
#include <langinfo.h>
#endif
#include <locale.h>
#include <signal.h>
#include <stdlib.h>

#include "audio.h"
#include "channels.h"
#include "config.h"
#include "cutter.h"
#include "device.h"
#include "diseqc.h"
#include "dvbdevice.h"
#include "eitscan.h"
#include "epg.h"
#include "i18n.h"
#include "interface.h"
#include "keys.h"
#include "libsi/si.h"
#include "lirc.h"
#include "menu.h"
#include "osdbase.h"
#include "plugin.h"
#include "recording.h"
#include "shutdown.h"
#include "skinclassic.h"
#include "skinlcars.h"
#include "skinsttng.h"
#include "sourceparams.h"
#include "sources.h"
#include "themes.h"
#include "timers.h"
#include "tools.h"
#include "transfer.h"
#include "videodir.h"
#include "vdr_main.h"

#include "vdr/filesystem/Directory.h"

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

CHelper_libXBMC_addon*   XBMC = NULL;
string         g_strUserPath    = "";
string         g_strClientPath  = "";
ADDON_STATUS   m_CurStatus      = ADDON_STATUS_UNKNOWN;

#define MINCHANNELWAIT        10 // seconds to wait between failed channel switchings
#define ACTIVITYTIMEOUT       60 // seconds before starting housekeeping
#define SHUTDOWNWAIT         300 // seconds to wait in user prompt before automatic shutdown
#define SHUTDOWNRETRY        360 // seconds before trying again to shut down
#define SHUTDOWNFORCEPROMPT    5 // seconds to wait in user prompt to allow forcing shutdown
#define SHUTDOWNCANCELPROMPT   5 // seconds to wait in user prompt to allow canceling shutdown
#define RESTARTCANCELPROMPT    5 // seconds to wait in user prompt before restarting on SIGHUP
#define MANUALSTART          600 // seconds the next timer must be in the future to assume manual start
#define CHANNELSAVEDELTA     600 // seconds before saving channels.conf after automatic modifications
#define DEVICEREADYTIMEOUT    30 // seconds to wait until all devices are ready
#define MENUTIMEOUT          120 // seconds of user inactivity after which an OSD display is closed
#define TIMERCHECKDELTA       10 // seconds between checks for timers that need to see their channel
#define TIMERDEVICETIMEOUT     8 // seconds before a device used for timer check may be reused
#define TIMERLOOKAHEADTIME    60 // seconds before a non-VPS timer starts and the channel is switched if possible
#define VPSLOOKAHEADTIME      24 // hours within which VPS timers will make sure their events are up to date
#define VPSUPTODATETIME     3600 // seconds before the event or schedule of a VPS timer needs to be refreshed

cVDRDaemon::cVDRDaemon()
 : m_EpgDataReader(NULL),
   m_Menu(NULL),
   m_LastChannel(0),
   m_LastTimerChannel(-1),
   m_PreviousChannelIndex(0),
   m_LastChannelChanged(time(NULL)),
   m_LastInteract(0),
   m_InhibitEpgScan(false),
   m_IsInfoMenu(false),
   m_CurrentSkin(NULL),
   m_LastSignal(0),
   m_finished(false)
{
  m_PreviousChannel[0] = 1;
  m_PreviousChannel[1] = 1;
}

cVDRDaemon& cVDRDaemon::Get(void)
{
  static cVDRDaemon _instance;
  return _instance;
}

cVDRDaemon::~cVDRDaemon(void)
{
  if (ShutdownHandler.EmergencyExitRequested())
    esyslog("emergency exit requested - shutting down");

  m_settings.m_pluginManager->StopPlugins();
  cRecordControls::Shutdown();
  cCutter::Stop();
  delete m_Menu;
  cControl::Shutdown();
  delete Interface;
  cOsdProvider::Shutdown();
  Remotes.Clear();
  Audios.Clear();
  Skins.Clear();
  SourceParams.Clear();
  if (ShutdownHandler.GetExitCode() != 2)
  {
    Setup.CurrentChannel = cDevice::CurrentChannel();
    Setup.CurrentVolume = cDevice::CurrentVolume();
    Setup.Save();
  }
  cDevice::Shutdown();
  EpgHandlers.Clear();
  m_settings.m_pluginManager->Shutdown(true);
  cSchedules::Cleanup(true);
  ReportEpgBugFixStats(true);
  if (m_LastSignal)
    isyslog("caught signal %d", m_LastSignal);
  if (ShutdownHandler.EmergencyExitRequested())
    esyslog("emergency exit!");
  isyslog("exiting, exit code %d", ShutdownHandler.GetExitCode());
  if (SysLogLevel > 0)
    closelog();
}

void* cVDRDaemon::Process(void)
{
  m_finished = false;
  while (IsRunning())
  {
    if (!Iterate())
      break;
  }
  m_finished = true;
  m_exitCondition.Signal();
  return NULL;
}

void cVDRDaemon::OnSignal(int signum)
{
  switch (signum)
  {
  case SIGPIPE:
    break;
  case SIGHUP:
    {
      CLockObject lock(m_critSection);
      m_LastSignal = signum;
      // SIGHUP shall cause a restart:
      if (ShutdownHandler.ConfirmRestart(true) &&
          Interface->Confirm(tr("Press any key to cancel restart"), RESTARTCANCELPROMPT, true))
      {
        ShutdownHandler.Exit(1);
      }
      m_LastSignal = 0;
      break;
    }
  default:
    {
      CLockObject lock(m_critSection);
      m_LastSignal = signum;
      Interface->Interrupt();
      ShutdownHandler.Exit(0);
      break;
    }
  }
}

bool cVDRDaemon::ReadCommandLineOptions(int argc, char *argv[])
{
  return m_settings.LoadFromCmdLine(argc, argv);
}

bool cVDRDaemon::Init(void)
{
  CLockObject lock(m_critSection);

  // Load plugins:
  if (!m_settings.m_pluginManager->LoadPlugins(true))
    return false;

  // Directories:
  SetVideoDirectory(m_settings.m_VideoDirectory);
  if (!m_settings.m_ConfigDirectory)
    m_settings.m_ConfigDirectory = DEFAULTCONFDIR;
  cPlugin::SetConfigDirectory(m_settings.m_ConfigDirectory);
  if (!m_settings.m_CacheDirectory)
    m_settings.m_CacheDirectory = DEFAULTCACHEDIR;
  cPlugin::SetCacheDirectory(m_settings.m_CacheDirectory);
  if (!m_settings.m_ResourceDirectory)
    m_settings.m_ResourceDirectory = DEFAULTRESDIR;
  cPlugin::SetResourceDirectory(m_settings.m_ResourceDirectory);
  cThemes::SetThemesDirectory(AddDirectory(m_settings.m_ConfigDirectory, "themes"));

  // Configuration data:
  Setup.Load(AddDirectory(m_settings.m_ConfigDirectory, "setup.conf"));
  Sources.Load(AddDirectory(m_settings.m_ConfigDirectory, "sources.conf"), true, true);
  Diseqcs.Load(AddDirectory(m_settings.m_ConfigDirectory, "diseqc.conf"), true,
      Setup.DiSEqC);
  Scrs.Load(AddDirectory(m_settings.m_ConfigDirectory, "scr.conf"), true);
  Channels.Load(AddDirectory(m_settings.m_ConfigDirectory, "channels.conf"), false, true);
  Timers.Load(AddDirectory(m_settings.m_ConfigDirectory, "timers.conf"));
  Commands.Load(AddDirectory(m_settings.m_ConfigDirectory, "commands.conf"));
  RecordingCommands.Load(AddDirectory(m_settings.m_ConfigDirectory, "reccmds.conf"));
  SVDRPhosts.Load(AddDirectory(m_settings.m_ConfigDirectory, "svdrphosts.conf"), true);
  Keys.Load(AddDirectory(m_settings.m_ConfigDirectory, "remote.conf"));
  KeyMacros.Load(AddDirectory(m_settings.m_ConfigDirectory, "keymacros.conf"), true);
  Folders.Load(AddDirectory(m_settings.m_ConfigDirectory, "folders.conf"));

  if (!*cFont::GetFontFileName(Setup.FontOsd))
  {
    const char *msg = "no fonts available - OSD will not show any text!";
    fprintf(stderr, "vdr: %s\n", msg);
    esyslog("ERROR: %s", msg);
  }

  // Recordings:
  Recordings.Update();
  DeletedRecordings.Update();

  // EPG data:
  if (m_settings.m_EpgDataFileName)
  {
    const char *EpgDirectory = NULL;
    if (cDirectory::CanWrite(m_settings.m_EpgDataFileName))
    {
      EpgDirectory = m_settings.m_EpgDataFileName;
      m_settings.m_EpgDataFileName = DEFAULTEPGDATAFILENAME;
    }
    else if (*m_settings.m_EpgDataFileName != '/' && *m_settings.m_EpgDataFileName != '.')
      EpgDirectory = m_settings.m_CacheDirectory;
    if (EpgDirectory)
      cSchedules::SetEpgDataFileName(
          AddDirectory(EpgDirectory, m_settings.m_EpgDataFileName));
    else
      cSchedules::SetEpgDataFileName(m_settings.m_EpgDataFileName);
    m_EpgDataReader = new cEpgDataReader;
    m_EpgDataReader->Start();
  }

  // DVB interfaces:
  cDvbDevice::Initialize();
  cDvbDevice::BondDevices(Setup.DeviceBondings);

  // Initialize plugins:
  if (!m_settings.m_pluginManager->InitializePlugins())
    return false;

  // Primary device:
  cDevice::SetPrimaryDevice(Setup.PrimaryDVB);
  if (!cDevice::PrimaryDevice() || !cDevice::PrimaryDevice()->HasDecoder())
  {
    if (cDevice::PrimaryDevice() && !cDevice::PrimaryDevice()->HasDecoder())
      isyslog(
          "device %d has no MPEG decoder", cDevice::PrimaryDevice()->DeviceNumber() + 1);
    for (int i = 0; i < cDevice::NumDevices(); i++)
    {
      cDevice *d = cDevice::GetDevice(i);
      if (d && d->HasDecoder())
      {
        isyslog("trying device number %d instead", i + 1);
        if (cDevice::SetPrimaryDevice(i + 1))
        {
          Setup.PrimaryDVB = i + 1;
          break;
        }
      }
    }
    if (!cDevice::PrimaryDevice())
    {
      const char *msg = "no primary device found - using first device!";
      fprintf(stderr, "vdr: %s\n", msg);
      esyslog("ERROR: %s", msg);
      if (!cDevice::SetPrimaryDevice(1))
        return false;
      if (!cDevice::PrimaryDevice())
      {
        const char *msg = "no primary device found - giving up!";
        fprintf(stderr, "vdr: %s\n", msg);
        esyslog("ERROR: %s", msg);
        return false;
      }
    }
  }

  // Check for timers in automatic start time window:
  ShutdownHandler.CheckManualStart(MANUALSTART);

  // User interface:
  Interface = new cInterface(m_settings.m_SVDRPport);

  // Default skins:
  new cSkinLCARS;
  new cSkinSTTNG;
  new cSkinClassic;
  Skins.SetCurrent(Setup.OSDSkin);
  cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme,
      Skins.Current()->Theme());
  m_CurrentSkin = Skins.Current();

  // Start plugins:
  if (!m_settings.m_pluginManager->StartPlugins())
    return false;

  // Set skin and theme in case they're implemented by a plugin:
  if (!m_CurrentSkin
      || (m_CurrentSkin == Skins.Current()
          && strcmp(Skins.Current()->Name(), Setup.OSDSkin) != 0))
  {
    Skins.SetCurrent(Setup.OSDSkin);
    cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme,
        Skins.Current()->Theme());
  }

  // Remote Controls:
#ifndef ANDROID
  if (m_settings.m_LircDevice)
    new cLircRemote(m_settings.m_LircDevice);
#endif

  if (!m_settings.m_DaemonMode && m_settings.m_HasStdin && m_settings.m_UseKbd)
    new cKbdRemote;
  Interface->LearnKeys();

  // External audio:
  if (m_settings.m_AudioCommand)
    new cExternalAudio(m_settings.m_AudioCommand);

  // Channel:
  if (!cDevice::WaitForAllDevicesReady(DEVICEREADYTIMEOUT))
    dsyslog("not all devices ready after %d seconds", DEVICEREADYTIMEOUT);
  if (*Setup.InitialChannel)
  {
    if (is_number(Setup.InitialChannel))
    { // for compatibility with old setup.conf files
      if (cChannel *Channel = Channels.GetByNumber(atoi(Setup.InitialChannel)))
        Setup.InitialChannel = Channel->GetChannelID().ToString();
    }
    if (cChannel *Channel = Channels.GetByChannelID(
        tChannelID::FromString(Setup.InitialChannel)))
      Setup.CurrentChannel = Channel->Number();
  }
  if (Setup.InitialVolume >= 0)
    Setup.CurrentVolume = Setup.InitialVolume;
  Channels.SwitchTo(Setup.CurrentChannel);
  if (m_settings.m_MuteAudio)
    cDevice::PrimaryDevice()->ToggleMute();
  else
    cDevice::PrimaryDevice()->SetVolume(Setup.CurrentVolume, true);
  return true;
}

bool cVDRDaemon::Iterate(void)
{
  // Main program loop:
  CLockObject lock(m_critSection);

  if (ShutdownHandler.DoExit())
    return false;

#ifdef DEBUGRINGBUFFERS
  cRingBufferLinear::PrintDebugRBL();
#endif
  // Attach launched player control:
  cControl::Attach();

  time_t Now = time(NULL);

  // Make sure we have a visible programme in case device usage has changed:
  if (!EITScanner.Active() && cDevice::PrimaryDevice()->HasDecoder())
  {
    static time_t lastTime = 0;
    if (!cDevice::PrimaryDevice()->HasProgramme())
    {
      if (!CamMenuActive() && Now - lastTime > MINCHANNELWAIT)
      { // !CamMenuActive() to avoid interfering with the CAM if a CAM menu is open
        cChannel *Channel = Channels.GetByNumber(cDevice::CurrentChannel());
        if (Channel
            && (Channel->Vpid() || Channel->Apid(0) || Channel->Dpid(0)))
        {
          if (cDevice::GetDeviceForTransponder(Channel, LIVEPRIORITY)
              && Channels.SwitchTo(Channel->Number())) // try to switch to the original channel...
            {}
          else if (m_LastTimerChannel > 0)
          {
            Channel = Channels.GetByNumber(m_LastTimerChannel);
            if (Channel
                && cDevice::GetDeviceForTransponder(Channel, LIVEPRIORITY)
                && Channels.SwitchTo(m_LastTimerChannel)) // ...or the one used by the last timer
              {}
          }
        }
        lastTime = Now; // don't do this too often
        m_LastTimerChannel = -1;
      }
    }
    else
      lastTime = 0; // makes sure we immediately try again next time
  }
  // Update the OSD size:
  {
    static time_t lastOsdSizeUpdate = 0;
    if (Now != lastOsdSizeUpdate)
    { // once per second
      cOsdProvider::UpdateOsdSize();
      lastOsdSizeUpdate = Now;
    }
  }
  // Handle channel and timer modifications:
  if (!Channels.BeingEdited() && !Timers.BeingEdited())
  {
    int modified = Channels.Modified();
    static time_t ChannelSaveTimeout = 0;
    static int TimerState = 0;
    // Channels and timers need to be stored in a consistent manner,
    // therefore if one of them is changed, we save both.
    if (modified == CHANNELSMOD_USER || Timers.Modified(TimerState))
      ChannelSaveTimeout = 1; // triggers an immediate save
    else if (modified && !ChannelSaveTimeout)
      ChannelSaveTimeout = Now + CHANNELSAVEDELTA;
    bool timeout = ChannelSaveTimeout == 1
        || (ChannelSaveTimeout && Now > ChannelSaveTimeout
            && !cRecordControls::Active());
    if ((modified || timeout) && Channels.Lock(false, 100))
    {
      if (timeout)
      {
        Channels.Save();
        Timers.Save();
        ChannelSaveTimeout = 0;
      }
      for (cChannel *Channel = Channels.First(); Channel;
          Channel = Channels.Next(Channel))
      {
        if (Channel->Modification(CHANNELMOD_RETUNE))
        {
          cRecordControls::ChannelDataModified(Channel);
          if (Channel->Number() == cDevice::CurrentChannel())
          {
            if (!cDevice::PrimaryDevice()->Replaying()
                || cDevice::PrimaryDevice()->Transferring())
            {
              if (cDevice::ActualDevice()->ProvidesTransponder(Channel))
              { // avoids retune on devices that don't really access the transponder
                isyslog(
                    "retuning due to modification of channel %d", Channel->Number());
                Channels.SwitchTo(Channel->Number());
              }
            }
          }
        }
      }
      Channels.Unlock();
    }
  }
  // Channel display:
  if (!EITScanner.Active() && cDevice::CurrentChannel() != m_LastChannel)
  {
    if (!m_Menu)
      m_Menu = new cDisplayChannel(cDevice::CurrentChannel(),
          m_LastChannel >= 0);
    m_LastChannel = cDevice::CurrentChannel();
    m_LastChannelChanged = Now;
  }
  if (Now - m_LastChannelChanged >= Setup.ZapTimeout
      && m_LastChannel != m_PreviousChannel[m_PreviousChannelIndex])
    m_PreviousChannel[m_PreviousChannelIndex ^= 1] = m_LastChannel;
  // Timers and Recordings:
  if (!Timers.BeingEdited())
  {
    // Assign events to timers:
    Timers.SetEvents();
    // Must do all following calls with the exact same time!
    // Process ongoing recordings:
    cRecordControls::Process(Now);
    // Start new recordings:
    cTimer *Timer = Timers.GetMatch(Now);
    if (Timer)
    {
      if (!cRecordControls::Start(Timer))
        Timer->SetPending(true);
      else
        m_LastTimerChannel = Timer->Channel()->Number();
    }
    // Make sure timers "see" their channel early enough:
    static time_t LastTimerCheck = 0;
    if (Now - LastTimerCheck > TIMERCHECKDELTA)
    { // don't do this too often
      m_InhibitEpgScan = false;
      for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer))
      {
        bool InVpsMargin = false;
        bool NeedsTransponder = false;
        if (Timer->HasFlags(tfActive) && !Timer->Recording())
        {
          if (Timer->HasFlags(tfVps))
          {
            if (Timer->Matches(Now, true, Setup.VpsMargin))
            {
              InVpsMargin = true;
              Timer->SetInVpsMargin(InVpsMargin);
            }
            else if (Timer->Event())
            {
              InVpsMargin = Timer->Event()->StartTime() <= Now
                  && Now < Timer->Event()->EndTime();
              NeedsTransponder = Timer->Event()->StartTime() - Now
                  < VPSLOOKAHEADTIME * 3600
                  && !Timer->Event()->SeenWithin(VPSUPTODATETIME);
            }
            else
            {
              cSchedulesLock SchedulesLock;
              const cSchedules *Schedules = cSchedules::Schedules(
                  SchedulesLock);
              if (Schedules)
              {
                const cSchedule *Schedule = Schedules->GetSchedule(
                    Timer->Channel());
                InVpsMargin = !Schedule; // we must make sure we have the schedule
                NeedsTransponder = Schedule
                    && !Schedule->PresentSeenWithin(VPSUPTODATETIME);
              }
            }
            m_InhibitEpgScan |= InVpsMargin | NeedsTransponder;
          }
          else
            NeedsTransponder = Timer->Matches(Now, true, TIMERLOOKAHEADTIME);
        }
        if (NeedsTransponder || InVpsMargin)
        {
          // Find a device that provides the required transponder:
          cDevice *Device = cDevice::GetDeviceForTransponder(Timer->Channel(),
              MINPRIORITY);
          if (!Device && InVpsMargin)
            Device = cDevice::GetDeviceForTransponder(Timer->Channel(),
                LIVEPRIORITY);
          // Switch the device to the transponder:
          if (Device)
          {
            bool HadProgramme = cDevice::PrimaryDevice()->HasProgramme();
            if (!Device->IsTunedToTransponder(Timer->Channel()))
            {
              if (Device == cDevice::ActualDevice()
                  && !Device->IsPrimaryDevice())
                cDevice::PrimaryDevice()->StopReplay(); // stop transfer mode
              dsyslog(
                  "switching device %d to channel %d", Device->DeviceNumber() + 1, Timer->Channel()->Number());
              if (Device->SwitchChannel(Timer->Channel(), false))
                Device->SetOccupied(TIMERDEVICETIMEOUT);
            }
            if (cDevice::PrimaryDevice()->HasDecoder() && HadProgramme
                && !cDevice::PrimaryDevice()->HasProgramme())
              Skins.QueueMessage(mtInfo, tr("Upcoming recording!")); // the previous SwitchChannel() has switched away the current live channel
          }
        }
      }
      LastTimerCheck = Now;
    }
    // Delete expired timers:
    Timers.DeleteExpired();
  }
  if (!m_Menu && Recordings.NeedsUpdate())
  {
    Recordings.Update();
    DeletedRecordings.Update();
  }
  // CAM control:
  if (!m_Menu && !cOsd::IsOpen())
    m_Menu = CamControl();
  // Queued messages:
  if (!Skins.IsOpen())
    Skins.ProcessQueuedMessages();

  // User Input:
  if (HandleInput(Now))
    return true;
  if (!m_Menu)
  {
    if (!m_InhibitEpgScan)
      EITScanner.Process();
    if (!cCutter::Active() && cCutter::Ended())
    {
      if (cCutter::Error())
        Skins.Message(mtError, tr("Editing process failed!"));
      else
        Skins.Message(mtInfo, tr("Editing process finished"));
    }
  }

  // Update the shutdown countdown:
  if (ShutdownHandler.countdown && ShutdownHandler.countdown.Update())
  {
    if (!ShutdownHandler.ConfirmShutdown(false))
      ShutdownHandler.countdown.Cancel();
  }

  if ((Now - m_LastInteract) > ACTIVITYTIMEOUT
      && !cRecordControls::Active() && !cCutter::Active() && !Interface->HasSVDRPConnection() && (Now - cRemote::LastActivity()) > ACTIVITYTIMEOUT){
      // Handle housekeeping tasks

      // Shutdown:
      // Check whether VDR will be ready for shutdown in SHUTDOWNWAIT seconds:
time_t      Soon = Now + SHUTDOWNWAIT;
      if (ShutdownHandler.IsUserInactive(Soon) && ShutdownHandler.Retry(Soon) && !ShutdownHandler.countdown)
      {
        if (ShutdownHandler.ConfirmShutdown(false))
        // Time to shut down - start final countdown:
        ShutdownHandler.countdown.Start(tr("VDR will shut down in %s minutes"), SHUTDOWNWAIT);// the placeholder is really %s!
        // Dont try to shut down again for a while:
        ShutdownHandler.SetRetry(SHUTDOWNRETRY);
      }
      // Countdown run down to 0?
      if (ShutdownHandler.countdown.Done())
      {
        // Timed out, now do a final check:
        if (ShutdownHandler.IsUserInactive() && ShutdownHandler.ConfirmShutdown(false))
        ShutdownHandler.DoShutdown(false);
        // Do this again a bit later:
        ShutdownHandler.SetRetry(SHUTDOWNRETRY);
      }

      // Disk housekeeping:
      RemoveDeletedRecordings();
      cSchedules::Cleanup();
      // Plugins housekeeping:
      m_settings.m_pluginManager->Housekeeping();
    }

  ReportEpgBugFixStats();

  // Main thread hooks of plugins:
  m_settings.m_pluginManager->MainThreadHook();

  return !ShutdownHandler.DoExit();
}

void cVDRDaemon::DirectMainFunction(eOSState function)
{
  m_IsInfoMenu &= (m_Menu == NULL);
  delete m_Menu;
  m_Menu = NULL;
  if (cControl::Control())
    cControl::Control()->Hide();
  m_Menu = new cMenuMain(function);
}

bool cVDRDaemon::HandleInput(time_t Now)
{
  cOsdObject *Interact = m_Menu ? m_Menu : cControl::Control();
  eKeys key = Interface->GetKey(!Interact || !Interact->NeedsFastResponse());
  if (ISREALKEY(key))
  {
    EITScanner.Activity();
    // Cancel shutdown countdown:
    if (ShutdownHandler.countdown)
      ShutdownHandler.countdown.Cancel();
    // Set user active for MinUserInactivity time in the future:
    ShutdownHandler.SetUserInactiveTimeout();
  }
  // Keys that must work independent of any interactive mode:
  switch (int(key))
    {
  // Menu control:
  case kMenu:
    {
      key = kNone; // nobody else needs to see this key
      bool WasOpen = Interact != NULL;
      bool WasMenu = Interact && Interact->IsMenu();
      if (m_Menu)
      {
        m_IsInfoMenu &= (m_Menu == NULL);
        delete m_Menu;
        m_Menu = NULL;
      }
      else if (cControl::Control())
      {
        if (cOsd::IsOpen())
          cControl::Control()->Hide();
        else
          WasOpen = false;
      }
      if (!WasOpen || (!WasMenu && !Setup.MenuKeyCloses))
        m_Menu = new cMenuMain;
    }
    break;
    // Info:
  case kInfo:
    {
      if (m_IsInfoMenu)
      {
        key = kNone; // nobody else needs to see this key
        m_IsInfoMenu &= (m_Menu == NULL);
        delete m_Menu;
        m_Menu = NULL;
      }
      else if (!m_Menu)
      {
        m_IsInfoMenu = true;
        if (cControl::Control())
        {
          cControl::Control()->Hide();
          m_Menu = cControl::Control()->GetInfo();
          if (m_Menu)
            m_Menu->Show();
          else
            m_IsInfoMenu = false;
        }
        else
        {
          cRemote::Put(kOk, true);
          cRemote::Put(kSchedule, true);
        }
        key = kNone; // nobody else needs to see this key
      }
    }
    break;
  // Direct main menu functions:
  case kSchedule:
    DirectMainFunction(osSchedule);
    key = kNone; // nobody else needs to see this key
    break;
  case kChannels:
    DirectMainFunction(osChannels);
    key = kNone; // nobody else needs to see this key
    break;
  case kTimers:
    DirectMainFunction(osTimers);
    key = kNone; // nobody else needs to see this key
    break;
  case kRecordings:
    DirectMainFunction(osRecordings);
    key = kNone; // nobody else needs to see this key
    break;
  case kSetup:
    DirectMainFunction(osSetup);
    key = kNone; // nobody else needs to see this key
    break;
  case kCommands:
    DirectMainFunction(osCommands);
    key = kNone; // nobody else needs to see this key
    break;
  case kUser0 ... kUser9:
    cRemote::PutMacro(key);
    key = kNone;
    break;
  case k_Plugin:
    {
      const char *PluginName = cRemote::GetPlugin();
      if (PluginName)
      {
        m_IsInfoMenu &= (m_Menu == NULL);
        delete m_Menu;
        m_Menu = NULL;
        if (cControl::Control())
          cControl::Control()->Hide();
        cPlugin *plugin = cPluginManager::GetPlugin(PluginName);
        if (plugin)
        {
          m_Menu = plugin->MainMenuAction();
          if (m_Menu)
            m_Menu->Show();
        }
        else
          esyslog("ERROR: unknown plugin '%s'", PluginName);
      }
      key = kNone; // nobody else needs to see these keys
    }
    break;
    // Channel up/down:
  case kChanUp | k_Repeat:
  case kChanUp:
  case kChanDn | k_Repeat:
  case kChanDn:
    if (!Interact)
      m_Menu = new cDisplayChannel(NORMALKEY(key));
    else if (cDisplayChannel::IsOpen() || cControl::Control())
    {
      Interact->ProcessKey(key);
      return true; // Iterate() should exit and return true
    }
    else
      cDevice::SwitchChannel(NORMALKEY(key) == kChanUp ? 1 : -1);
    key = kNone; // nobody else needs to see these keys
    break;
    // Volume control:
  case kVolUp | k_Repeat:
  case kVolUp:
  case kVolDn | k_Repeat:
  case kVolDn:
  case kMute:
    if (key == kMute)
    {
      if (!cDevice::PrimaryDevice()->ToggleMute() && !m_Menu)
      {
        key = kNone; // nobody else needs to see these keys
        break; // no need to display "mute off"
      }
    }
    else
      cDevice::PrimaryDevice()->SetVolume(
          NORMALKEY(key) == kVolDn ? -VOLUMEDELTA : VOLUMEDELTA);
    if (!m_Menu && !cOsd::IsOpen())
      m_Menu = cDisplayVolume::Create();
    cDisplayVolume::Process(key);
    key = kNone; // nobody else needs to see these keys
    break;
    // Audio track control:
  case kAudio:
    if (cControl::Control())
      cControl::Control()->Hide();
    if (!cDisplayTracks::IsOpen())
    {
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = cDisplayTracks::Create();
    }
    else
      cDisplayTracks::Process(key);
    key = kNone;
    break;
    // Subtitle track control:
  case kSubtitles:
    if (cControl::Control())
      cControl::Control()->Hide();
    if (!cDisplaySubtitleTracks::IsOpen())
    {
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = cDisplaySubtitleTracks::Create();
    }
    else
      cDisplaySubtitleTracks::Process(key);
    key = kNone;
    break;
    // Pausing live video:
  case kPlayPause:
  case kPause:
    if (!cControl::Control())
    {
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = NULL;
      if (Setup.PauseKeyHandling)
      {
        if (Setup.PauseKeyHandling > 1
            || Interface->Confirm(tr("Pause live video?")))
        {
          if (!cRecordControls::PauseLiveVideo())
            Skins.QueueMessage(mtError, tr("No free DVB device to record!"));
        }
      }
      key = kNone; // nobody else needs to see this key
    }
    break;
    // Instant recording:
  case kRecord:
    if (!cControl::Control())
    {
      if (cRecordControls::Start())
        Skins.QueueMessage(mtInfo, tr("Recording started"));
      key = kNone; // nobody else needs to see this key
    }
    break;
    // Power off:
  case kPower:
    isyslog("Power button pressed");
    m_IsInfoMenu &= (m_Menu == NULL);
    delete m_Menu;
    m_Menu = NULL;
    // Check for activity, request power button again if active:
    if (!ShutdownHandler.ConfirmShutdown(false)
        && Skins.Message(mtWarning,
            tr("VDR will shut down later - press Power to force"),
            SHUTDOWNFORCEPROMPT) != kPower)
    {
      // Not pressed power - set VDR to be non-interactive and power down later:
      ShutdownHandler.SetUserInactive();
      break;
    }
    // No activity or power button pressed twice - ask for confirmation:
    if (!ShutdownHandler.ConfirmShutdown(true))
    {
      // Non-confirmed background activity - set VDR to be non-interactive and power down later:
      ShutdownHandler.SetUserInactive();
      break;
    }
    // Ask the final question:
    if (!Interface->Confirm(tr("Press any key to cancel shutdown"),
        SHUTDOWNCANCELPROMPT, true))
      // If final question was canceled, continue to be active:
      break;
    // Ok, now call the shutdown script:
    ShutdownHandler.DoShutdown(true);
    // Set VDR to be non-interactive and power down again later:
    ShutdownHandler.SetUserInactive();
    // Do not attempt to automatically shut down for a while:
    ShutdownHandler.SetRetry(SHUTDOWNRETRY);
    break;
  default:
    break;
    }
  Interact = m_Menu ? m_Menu : cControl::Control(); // might have been closed in the mean time
  if (Interact)
  {
    m_LastInteract = Now;
    eOSState state = Interact->ProcessKey(key);
    if (state == osUnknown && Interact != cControl::Control())
    {
      if (ISMODELESSKEY(key) && cControl::Control())
      {
        state = cControl::Control()->ProcessKey(key);
        if (state == osEnd)
        {
          // let's not close a menu when replay ends:
          cControl::Shutdown();
          return true; // Iterate() should exit and return true
        }
      }
      else if (Now - cRemote::LastActivity() > MENUTIMEOUT)
        state = osEnd;
    }
    switch (state)
    {
    case osPause:
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = NULL;
      if (!cRecordControls::PauseLiveVideo())
        Skins.QueueMessage(mtError, tr("No free DVB device to record!"));
      break;
    case osRecord:
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = NULL;
      if (cRecordControls::Start())
        Skins.QueueMessage(mtInfo, tr("Recording started"));
      break;
    case osRecordings:
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = NULL;
      cControl::Shutdown();
      m_Menu = new cMenuMain(osRecordings, true);
      break;
    case osReplay:
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = NULL;
      cControl::Shutdown();
      cControl::Launch(new cReplayControl);
      break;
    case osStopReplay:
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = NULL;
      cControl::Shutdown();
      break;
    case osSwitchDvb:
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = NULL;
      cControl::Shutdown();
      Skins.QueueMessage(mtInfo, tr("Switching primary DVB..."));
      cDevice::SetPrimaryDevice(Setup.PrimaryDVB);
      break;
    case osPlugin:
      m_IsInfoMenu &= (m_Menu == NULL);
      delete m_Menu;
      m_Menu = cMenuMain::PluginOsdObject();
      if (m_Menu)
        m_Menu->Show();
      break;
    case osBack:
    case osEnd:
      if (Interact == m_Menu)
      {
        m_IsInfoMenu &= (m_Menu == NULL);
        delete m_Menu;
        m_Menu = NULL;
      }
      else
        cControl::Shutdown();
      break;
    default:
      break;
    }
  }
  else
  {
    // Key functions in "normal" viewing mode:
    if (key != kNone && KeyMacros.Get(key))
    {
      cRemote::PutMacro(key);
      key = kNone;
    }
    switch (int(key))
    {
    // Toggle channels:
    case kChanPrev:
    case k0:
      {
        if (m_PreviousChannel[m_PreviousChannelIndex ^ 1] == m_LastChannel
            || (m_LastChannel != m_PreviousChannel[0]
                && m_LastChannel != m_PreviousChannel[1]))
          m_PreviousChannelIndex ^= 1;
        Channels.SwitchTo(m_PreviousChannel[m_PreviousChannelIndex ^= 1]);
        break;
      }
      // Direct Channel Select:
    case k1 ... k9:
      // Left/Right rotates through channel groups:
    case kLeft | k_Repeat:
    case kLeft:
    case kRight | k_Repeat:
    case kRight:
      // Previous/Next rotates through channel groups:
    case kPrev | k_Repeat:
    case kPrev:
    case kNext | k_Repeat:
    case kNext:
      // Up/Down Channel Select:
    case kUp | k_Repeat:
    case kUp:
    case kDown | k_Repeat:
    case kDown:
      m_Menu = new cDisplayChannel(NORMALKEY(key));
      break;
      // Viewing Control:
    case kOk:
      m_LastChannel = -1;
      break; // forces channel display
      // Instant resume of the last viewed recording:
    case kPlay:
      if (cReplayControl::LastReplayed())
      {
        cControl::Shutdown();
        cControl::Launch(new cReplayControl);
      }
      else
      {
        DirectMainFunction(osRecordings);
        key = kNone; // nobody else needs to see this key
      }
      // no last viewed recording, so enter the Recordings menu
      break;
    default:
      break;
    }
  }
  return false; // Iterate() should continue normally
}

void cVDRDaemon::WaitForShutdown()
{
  CMutex mutex; // private mutex
  CLockObject lock(mutex);
  m_exitCondition.Wait(mutex, m_finished);
}

void SetSystemCharacterTable()
{
#ifndef ANDROID
  char *CodeSet = NULL;
  if (setlocale(LC_CTYPE, ""))
    CodeSet = nl_langinfo(CODESET);
  else
  {
    char *LangEnv = getenv("LANG"); // last resort in case locale stuff isn't installed
    if (LangEnv)
    {
      CodeSet = strchr(LangEnv, '.');
      if (CodeSet)
        CodeSet++; // skip the dot
    }
  }
  if (CodeSet)
  {
    bool known = SI::SetSystemCharacterTable(CodeSet);
    isyslog("codeset is '%s' - %s", CodeSet, known ? "known" : "unknown");
    cCharSetConv::SetSystemCharacterTable(CodeSet);
  }
#endif
}

int main(int argc, char *argv[])
{
  cVDRDaemon* daemon = &cVDRDaemon::Get();

  // Initiate locale:
  setlocale(LC_ALL, "");

  if (!daemon->ReadCommandLineOptions(argc, argv))
    return 2;

  cThread::SetMainThreadId();

  ::SetSystemCharacterTable();

  // Initialize internationalization:
  I18nInitialize(daemon->m_settings.m_LocaleDirectory);

  if (!daemon->Init())
    return -2;

  CSignalHandler::Get().SetSignalReceiver(SIGHUP, daemon);
  CSignalHandler::Get().SetSignalReceiver(SIGINT, daemon);
  CSignalHandler::Get().SetSignalReceiver(SIGTERM, daemon);
  CSignalHandler::Get().SetSignalReceiver(SIGPIPE, daemon);

  //alarm(daemon->m_settings.m_WatchdogTimeout); // Initial watchdog timer start (TODO)

  daemon->CreateThread();
  daemon->WaitForShutdown();

  // Reset all signal handlers to default before Interface gets deleted:
  CSignalHandler::Get().ResetSignalReceivers();

  return ShutdownHandler.GetExitCode();
}

extern "C" {

void ADDON_ReadSettings(void)
{
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  CONTENT_PROPERTIES* cprops = (CONTENT_PROPERTIES*)props;

  try
  {
    XBMC = new CHelper_libXBMC_addon;
    if (!XBMC || !XBMC->RegisterMe(hdl))
      throw ADDON_STATUS_PERMANENT_FAILURE;

    cVDRDaemon* daemon = &cVDRDaemon::Get();
    if (!daemon->Init() && !daemon->CreateThread(true))
      throw ADDON_STATUS_UNKNOWN;
  }
  catch (ADDON_STATUS status)
  {
    delete XBMC;
    XBMC = NULL;
    return status;
  }

  m_CurStatus     = ADDON_STATUS_OK;
  g_strUserPath   = cprops->strUserPath;
  g_strClientPath = cprops->strClientPath;

  ADDON_ReadSettings();
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  cVDRDaemon::Get().StopThread();
  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting *** sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  if (!settingName || !settingValue)
    return ADDON_STATUS_UNKNOWN;

  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
  cVDRDaemon::Get().StopThread();
}

void ADDON_FreeSettings()
{
}

}
