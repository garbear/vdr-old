/*
 * shutdown.c: Handling of shutdown and inactivity
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original version written by Udo Richter <udo_richter@gmx.de>.
 *
 * $Id: shutdown.c 2.1 2013/02/18 10:33:26 kls Exp $
 */

#include "Shutdown.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "channels/ChannelManager.h"
#include "Config.h"
#include "recordings/RecordingCutter.h"
#include "I18N.h"
//#include "interface.h"
//#include "menu.h"
//#include "plugin.h"
#include "recordings/Timers.h"
#include "recordings/Recording.h"
#include "Tools.h"

#include "vdr/utils/CalendarUtils.h"

cShutdownHandler ShutdownHandler;

cCountdown::cCountdown(void)
{
  timeout = 0;
  counter = 0;
  timedOut = false;
  message = NULL;
}

void cCountdown::Start(const char *Message, int Seconds)
{
  timeout = time(NULL) + Seconds;
  counter = -1;
  timedOut = false;
  message = Message;
  Update();
}

void cCountdown::Cancel(void)
{
  if (timeout) {
     timeout = 0;
     timedOut = false;
     //XXX Skins.Message(mtStatus, NULL);
     }
}

bool cCountdown::Done(void)
{
  if (timedOut) {
     Cancel();
     return true;
     }
  return false;
}

bool cCountdown::Update(void)
{
  if (timeout) {
     int NewCounter = (timeout - time(NULL) + 9) / 10;
     if (NewCounter <= 0)
        timedOut = true;
     if (counter != NewCounter) {
        counter = NewCounter;
        char time[10];
        snprintf(time, sizeof(time), "%d:%d0", counter > 0 ? counter / 6 : 0, counter > 0 ? counter % 6 : 0);
        cString Message = cString::sprintf(message, time);
        //XXX Skins.Message(mtStatus, Message);
        return true;
        }
     }
  return false;
}

cShutdownHandler::cShutdownHandler(void)
{
  activeTimeout = 0;
  retry = 0;
  shutdownCommand = NULL;
  exitCode = -1;
  emergencyExitRequested = false;
}

cShutdownHandler::~cShutdownHandler()
{
  free(shutdownCommand);
}

void cShutdownHandler::RequestEmergencyExit(void)
{
  if (g_setup.EmergencyExit) {
     esyslog("initiating emergency exit");
     emergencyExitRequested = true;
     Exit(1);
     }
  else
     dsyslog("emergency exit request ignored according to setup");
}

void cShutdownHandler::CheckManualStart(int ManualStart)
{
  time_t Delta = g_setup.NextWakeupTime ? g_setup.NextWakeupTime - time(NULL) : 0;

  if (!g_setup.NextWakeupTime || abs(Delta) > ManualStart) {
     // Apparently the user started VDR manually
     dsyslog("assuming manual start of VDR");
     // Set inactive after MinUserInactivity
     SetUserInactiveTimeout();
     }
  else {
     // Set inactive from now on
     dsyslog("scheduled wakeup time in %ld minutes, assuming automatic start of VDR", Delta / 60);
     SetUserInactiveTimeout(-3, true);
     }
}

void cShutdownHandler::SetShutdownCommand(const char *ShutdownCommand)
{
  free(shutdownCommand);
  shutdownCommand = ShutdownCommand ? strdup(ShutdownCommand) : NULL;
}

void cShutdownHandler::CallShutdownCommand(time_t WakeupTime, int Channel, const char *File, bool UserShutdown)
{
  time_t Delta = WakeupTime ? WakeupTime - time(NULL) : 0;
  cString cmd = cString::sprintf("%s %ld %ld %d \"%s\" %d", shutdownCommand, WakeupTime, Delta, Channel, *strescape(File, "\\\"$"), UserShutdown);
  isyslog("executing '%s'", *cmd);
  int Status = SystemExec(cmd, true);
  if (!WIFEXITED(Status) || WEXITSTATUS(Status))
     esyslog("SystemExec() failed with status %d", Status);
  else {
     g_setup.NextWakeupTime = WakeupTime; // Remember this wakeup time for comparison on reboot
     g_setup.Save();
     }
}

void cShutdownHandler::SetUserInactiveTimeout(int Seconds, bool Force)
{
  if (!g_setup.MinUserInactivity && !Force) {
     activeTimeout = 0;
     return;
     }
  if (Seconds >= 0)
     activeTimeout = time(NULL) + Seconds;
  else if (Seconds == -1)
     activeTimeout = time(NULL) + g_setup.MinUserInactivity * 60;
  else if (Seconds == -2)
     activeTimeout = 0;
  else if (Seconds == -3)
     activeTimeout = 1;
}

bool cShutdownHandler::ConfirmShutdown(bool Interactive)
{
  if (!shutdownCommand) {
//XXX     if (Interactive)
//        Skins.Message(mtError, tr("Can't shutdown - option '-s' not given!"));
     return false;
     }
//XXX  if (cCutter::Active()) {
//     if (!Interactive || !Interface->Confirm(tr("Editing - shut down anyway?")))
//        return false;
//     }

  cTimer *timer = Timers.GetNextActiveTimer();
  time_t Next = timer ? timer->StartTime() : 0;
  time_t Delta = timer ? Next - time(NULL) : 0;

//XXX  if (cRecordControls::Active() || (Next && Delta <= 0)) {
//     // VPS recordings in timer end margin may cause Delta <= 0
//     if (!Interactive || !Interface->Confirm(tr("Recording - shut down anyway?")))
//        return false;
//     }
//  else
    if (Next && Delta <= g_setup.MinEventTimeout * 60) {
     // Timer within Min Event Timeout
     if (!Interactive)
        return false;
     cString buf = cString::sprintf(tr("Recording in %ld minutes, shut down anyway?"), Delta / 60);
//XXX     if (!Interface->Confirm(buf))
//        return false;
     }

//XXX  if (cPluginManager::Active(Interactive ? tr("shut down anyway?") : NULL))
//     return false;
//
//  cPlugin *Plugin = cPluginManager::GetNextWakeupPlugin();
//  Next = Plugin ? Plugin->WakeupTime() : 0;
//  Delta = Next ? Next - time(NULL) : 0;
//  if (Next && Delta <= g_setup.MinEventTimeout * 60) {
//     // Plugin wakeup within Min Event Timeout
//     if (!Interactive)
//        return false;
//     cString buf = cString::sprintf(tr("Plugin %s wakes up in %ld min, continue?"), Plugin->Name(), Delta / 60);
//     if (!Interface->Confirm(buf))
//        return false;
//     }

  return true;
}

bool cShutdownHandler::ConfirmRestart(bool Interactive)
{
  if (cCutter::Active()) {
//XXX     if (!Interactive || !Interface->Confirm(tr("Editing - restart anyway?")))
//        return false;
     }

  cTimer *timer = Timers.GetNextActiveTimer();
  time_t Next  = timer ? timer->StartTime() : 0;
  time_t Delta = timer ? Next - time(NULL) : 0;

//XXX  if (cRecordControls::Active() || (Next && Delta <= 0)) {
//     // VPS recordings in timer end margin may cause Delta <= 0
//     if (!Interactive || !Interface->Confirm(tr("Recording - restart anyway?")))
//        return false;
//     }

//XXX  if (cPluginManager::Active(Interactive ? tr("restart anyway?") : NULL))
//     return false;

  return true;
}

bool cShutdownHandler::DoShutdown(bool Force)
{
  time_t Now = time(NULL);
  cTimer *timer = Timers.GetNextActiveTimer();
//XXX  cPlugin *Plugin = cPluginManager::GetNextWakeupPlugin();

  time_t Next = timer ? timer->StartTime() : 0;
//XXX  time_t NextPlugin = Plugin ? Plugin->WakeupTime() : 0;
//  if (NextPlugin && (!Next || Next > NextPlugin)) {
//     Next = NextPlugin;
//     timer = NULL;
//     }
  time_t Delta = Next ? Next - Now : 0;

  if (Next && Delta < g_setup.MinEventTimeout * 60) {
     if (!Force)
        return false;
     Delta = g_setup.MinEventTimeout * 60;
     Next = Now + Delta;
     timer = NULL;
     dsyslog("reboot at %s", CalendarUtils::TimeToString(Next).c_str());
     }

  if (Next && timer) {
     dsyslog("next timer event at %s", CalendarUtils::TimeToString(Next).c_str());
     CallShutdownCommand(Next, timer->Channel()->Number(), timer->File(), Force);
     }
//XXX  else if (Next && Plugin) {
//     CallShutdownCommand(Next, 0, Plugin->Name(), Force);
//     dsyslog("next plugin wakeup at %s", CalendarUtils::TimeToString(Next).c_str());
//     }
  else
     CallShutdownCommand(Next, 0, "", Force); // Next should always be 0 here. Just for safety, pass it.

  return true;
}
