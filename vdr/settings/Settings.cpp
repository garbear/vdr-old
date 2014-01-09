/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "Settings.h"
#include "filesystem/Directory.h"
#include "Config.h"
//#include "cutter.h"
#include "devices/Device.h"
//#include "../../plugin.h"
#include "recordings/Recording.h"
#include "utils/Shutdown.h"
#include "svdrp.h"
#include "utils/Tools.h"

#include <getopt.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_CAP
#include <sys/capability.h>
#endif
#include <sys/prctl.h>
#include <sys/syslog.h>
#include <unistd.h>

#define LIRC_DEVICE "/var/run/lirc/lircd"
#define CACHEDIR    "/var/cache/vdr"
#define RESDIR      "/usr/local/share/vdr"
#define PLUGINDIR   "/usr/local/lib/vdr"
#define LOCDIR      "/usr/local/share/locale"

#define DEFAULTSVDRPPORT   6419
#define DEFAULTVIDEODIR    VIDEODIR
#define DEFAULTPLUGINDIR   PLUGINDIR
#define DEFAULTLOCDIR      LOCDIR

// for easier orientation, this is column 80|
#define MSG_HELP "Usage: vdr [OPTIONS]\n\n" \
                 "  -a CMD,   --audio=CMD    send Dolby Digital audio to stdin of command CMD\n" \
                 "            --cachedir=DIR save cache files in DIR (default: %s)\n" \
                 "  -c DIR,   --config=DIR   read config files from DIR (default: %s)\n" \
                 "  -d,       --daemon       run in daemon mode\n" \
                 "  -D NUM,   --device=NUM   use only the given DVB device (NUM = 0, 1, 2...)\n" \
                 "                           there may be several -D options (default: all DVB\n" \
                 "                           devices will be used)\n" \
                 "            --dirnames=PATH[,NAME[,ENC]]\n" \
                 "                           set the maximum directory path length to PATH\n" \
                 "                           (default: %d); if NAME is also given, it defines\n" \
                 "                           the maximum directory name length (default: %d);\n" \
                 "                           the optional ENC can be 0 or 1, and controls whether\n" \
                 "                           special characters in directory names are encoded as\n" \
                 "                           hex values (default: 0); if PATH or NAME are left\n" \
                 "                           empty (as in \",,1\" to only set ENC), the defaults\n" \
                 "                           apply\n" \
                 "            --edit=REC     cut recording REC and exit\n" \
                 "  -E FILE,  --epgfile=FILE write the EPG data into the given FILE (default is\n" \
                 "                           '%s' in the cache directory)\n" \
                 "                           '-E-' disables this\n" \
                 "                           if FILE is a directory, the default EPG file will be\n" \
                 "                           created in that directory\n" \
                 "            --filesize=SIZE limit video files to SIZE bytes (default is %dM)\n" \
                 "                           only useful in conjunction with --edit\n" \
                 "            --genindex=REC generate index for recording REC and exit\n" \
                 "  -g DIR,   --grab=DIR     write images from the SVDRP command GRAB into the\n" \
                 "                           given DIR; DIR must be the full path name of an\n" \
                 "                           existing directory, without any \"..\", double '/'\n" \
                 "                           or symlinks (default: none, same as -g-)\n" \
                 "  -h,       --help         print this help and exit\n" \
                 "  -i ID,    --instance=ID  use ID as the id of this VDR instance (default: 0)\n" \
                 "  -l LEVEL, --log=LEVEL    set log level (default: 3)\n" \
                 "                           0 = no logging, 1 = errors only,\n" \
                 "                           2 = errors and info, 3 = errors, info and debug\n" \
                 "                           if logging should be done to LOG_LOCALn instead of\n" \
                 "                           LOG_USER, add '.n' to LEVEL, as in 3.7 (n=0..7)\n" \
                 "  -L DIR,   --lib=DIR      search for plugins in DIR (default is %s)\n" \
                 "            --lirc[=PATH]  use a LIRC remote control device, attached to PATH\n" \
                 "                           (default: %s)\n" \
                 "            --localedir=DIR search for locale files in DIR (default is\n" \
                 "                           %s)\n" \
                 "  -m,       --mute         mute audio of the primary DVB device at startup\n" \
                 "            --no-kbd       don't use the keyboard as an input device\n" \
                 "  -p PORT,  --port=PORT    use PORT for SVDRP (default: %d)\n" \
                 "                           0 turns off SVDRP\n" \
                 "  -P OPT,   --plugin=OPT   load a plugin defined by the given options\n" \
                 "  -r CMD,   --record=CMD   call CMD before and after a recording, and after\n" \
                 "                           a recording has been edited or deleted\n" \
                 "            --resdir=DIR   read resource files from DIR (default: %s)\n" \
                 "  -s CMD,   --shutdown=CMD call CMD to shutdown the computer\n" \
                 "            --split        split edited files at the editing marks (only\n" \
                 "                           useful in conjunction with --edit)\n" \
                 "  -t TTY,   --terminal=TTY controlling tty\n" \
                 "  -u USER,  --user=USER    run as user USER; only applicable if started as\n" \
                 "                           root\n" \
                 "            --userdump     allow coredumps if -u is given (debugging)\n" \
                 "  -v DIR,   --video=DIR    use DIR as video directory (default: %s)\n" \
                 "  -V,       --version      print version information and exit\n" \
                 "            --vfat         for backwards compatibility (same as\n" \
                 "                           --dirnames=250,40,1\n" \
                 "\n"

cSettings::cSettings()
{
  m_DaemonMode = false;
  m_EpgDataFileName = DEFAULTEPGDATAFILENAME;
  m_DisplayHelp = false;
  m_SysLogTarget = LOG_USER;
//  m_pluginManager = new cPluginManager(DEFAULTPLUGINDIR);
  m_LocaleDirectory = DEFAULTLOCDIR;
  m_MuteAudio = false;
#if defined(REMOTE_KBD)
  m_UseKbd = true;
#else
  m_UseKbd = false;
#endif
  m_SVDRPport = DEFAULTSVDRPPORT;
#if defined(VDR_USER)
  m_VdrUser = VDR_USER;
#endif
  m_UserDump = false;
  m_DisplayVersion = false;
  m_VideoDirectory = DEFAULTVIDEODIR;
  m_StartedAsRoot = false;
  m_HasStdin = (tcgetpgrp(STDIN_FILENO) == getpid() || getppid() != (pid_t)1) && tcgetattr(STDIN_FILENO, &m_savedTm) == 0;

  m_ListenPort              = LISTEN_PORT;
  m_StreamTimeout           = 10;
  m_PmtTimeout              = 5;
  m_TimeshiftMode           = 0;
  m_TimeshiftBufferSize     = 5;
  m_TimeshiftBufferFileSize = 6;
  m_ConfigDirectory         = DEFAULTCONFDIR;
}

cSettings::~cSettings()
{
  if (m_HasStdin)
    tcsetattr(STDIN_FILENO, TCSANOW, &m_savedTm);
//  delete m_pluginManager;
}

bool cSettings::LoadFromCmdLine(int argc, char *argv[])
{
  static struct option long_options[] =
    {
      { "audio",     required_argument, NULL, 'a' },
      { "cachedir",  required_argument, NULL, 'c' | 0x100 },
      { "config",    required_argument, NULL, 'c' },
      { "daemon",    no_argument,       NULL, 'd' },
      { "device",    required_argument, NULL, 'D' },
      { "dirnames",  required_argument, NULL, 'd' | 0x100 },
      { "edit",      required_argument, NULL, 'e' | 0x100 },
      { "epgfile",   required_argument, NULL, 'E' },
      { "filesize",  required_argument, NULL, 'f' | 0x100 },
      { "genindex",  required_argument, NULL, 'g' | 0x100 },
      { "grab",      required_argument, NULL, 'g' },
      { "help",      no_argument,       NULL, 'h' },
      { "instance",  required_argument, NULL, 'i' },
      { "lib",       required_argument, NULL, 'L' },
      { "lirc",      optional_argument, NULL, 'l' | 0x100 },
      { "localedir", required_argument, NULL, 'l' | 0x200 },
      { "log",       required_argument, NULL, 'l' },
      { "mute",      no_argument,       NULL, 'm' },
      { "no-kbd",    no_argument,       NULL, 'n' | 0x100 },
      { "plugin",    required_argument, NULL, 'P' },
      { "port",      required_argument, NULL, 'p' },
      { "record",    required_argument, NULL, 'r' },
      { "resdir",    required_argument, NULL, 'r' | 0x100 },
      { "shutdown",  required_argument, NULL, 's' },
      { "split",     no_argument,       NULL, 's' | 0x100 },
      { "terminal",  required_argument, NULL, 't' },
      { "user",      required_argument, NULL, 'u' },
      { "userdump",  no_argument,       NULL, 'u' | 0x100 },
      { "version",   no_argument,       NULL, 'V' },
      { "vfat",      no_argument,       NULL, 'v' | 0x100 },
      { "video",     required_argument, NULL, 'v' },
      { NULL,        no_argument,       NULL, 0 }
    };

  int c;
  while ((c = getopt_long(argc, argv, "a:c:dD:e:E:g:hi:l:L:mp:P:r:s:t:u:v:Vw:",
      long_options, NULL)) != -1)
  {
    switch (c)
      {
    // audio
    case 'a':
      m_AudioCommand = optarg;
      break;
    // cachedir
    case 'c' | 0x100:
      m_CacheDirectory = optarg;
      break;
    // config
    case 'c':
      m_ConfigDirectory = optarg;
      break;
    // daemon
    case 'd':
      m_DaemonMode = true;
      break;
//    // device
//    case 'D':
//      if (is_number(optarg))
//      {
//        int n = atoi(optarg);
//        if (0 <= n && n < MAXDEVICES)
//        {
//          cDevice::SetUseDevice(n);
//          break;
//        }
//      }
//      fprintf(stderr, "vdr: invalid DVB device number: %s\n", optarg);
//      return false;
    // dirnames
    case 'd' | 0x100:
      {
        char *s = optarg;
        if (*s != ',')
        {
          int n = strtol(s, &s, 10);
          if (n <= 0 || n >= PATH_MAX)
          { // PATH_MAX includes the terminating 0
            fprintf(stderr, "vdr: invalid directory path length: %s\n", optarg);
            return false;
          }
          DirectoryPathMax = n;
          if (!*s)
            break;
          if (*s != ',')
          {
            fprintf(stderr, "vdr: invalid delimiter: %s\n", optarg);
            return false;
          }
        }
        s++;
        if (!*s)
          break;
        if (*s != ',')
        {
          int n = strtol(s, &s, 10);
          if (n <= 0 || n > NAME_MAX)
          { // NAME_MAX excludes the terminating 0
            fprintf(stderr, "vdr: invalid directory name length: %s\n", optarg);
            return false;
          }
          DirectoryNameMax = n;
          if (!*s)
            break;
          if (*s != ',')
          {
            fprintf(stderr, "vdr: invalid delimiter: %s\n", optarg);
            return false;
          }
        }
        s++;
        if (!*s)
          break;
        int n = strtol(s, &s, 10);
        if (n != 0 && n != 1)
        {
          fprintf(stderr, "vdr: invalid directory encoding: %s\n", optarg);
          return false;
        }
        DirectoryEncoding = n;
        if (*s)
        {
          fprintf(stderr, "vdr: unexpected data: %s\n", optarg);
          return false;
        }
      }
      break;
    // edit
//    case 'e' | 0x100:
//      return CutRecording(optarg);
    // epgfile
    case 'E':
      m_EpgDataFileName = (*optarg != '-' ? optarg : "");
      break;
    // filesize
    case 'f' | 0x100:
      Setup.MaxVideoFileSize = StrToNum(optarg) / MEGABYTE(1);
      if (Setup.MaxVideoFileSize < MINVIDEOFILESIZE)
        Setup.MaxVideoFileSize = MINVIDEOFILESIZE;
      if (Setup.MaxVideoFileSize > MAXVIDEOFILESIZETS)
        Setup.MaxVideoFileSize = MAXVIDEOFILESIZETS;
      break;
    // genindex
    case 'g' | 0x100:
      return GenerateIndex(optarg);
      // grab
//    case 'g':
//XXX      cSVDRP::SetGrabImageDir(*optarg != '-' ? optarg : NULL);
      break;
    // help
    case 'h':
      m_DisplayHelp = true;
      break;
    // instance
    case 'i':
      if (is_number(optarg))
      {
        InstanceId = atoi(optarg);
        if (InstanceId >= 0)
          break;
      }
      fprintf(stderr, "vdr: invalid instance id: %s\n", optarg);
      return false;
    // log
    case 'l':
      {
        char *p = strchr(optarg, '.');
        if (p)
          *p = 0;
        if (is_number(optarg))
        {
          int l = atoi(optarg);
          if (0 <= l && l <= 3)
          {
            SysLogLevel = l;
            if (!p)
              break;
            if (is_number(p + 1))
            {
              int l = atoi(p + 1);
              if (0 <= l && l <= 7)
              {
                int targets[] =
                  { LOG_LOCAL0, LOG_LOCAL1, LOG_LOCAL2, LOG_LOCAL3, LOG_LOCAL4,
                      LOG_LOCAL5, LOG_LOCAL6, LOG_LOCAL7 };
                m_SysLogTarget = targets[l];
                break;
              }
            }
          }
        }
        if (p)
          *p = '.';
        fprintf(stderr, "vdr: invalid log level: %s\n", optarg);
        return false;
      }
    // lib
    case 'L':
//      if (access(optarg, R_OK | X_OK) == 0)
//        m_pluginManager->SetDirectory(optarg);
//      else
//      {
//        fprintf(stderr, "vdr: can't access plugin directory: %s\n", optarg);
//        return false;
//      }
      break;
    // localedir
    case 'l' | 0x200:
      if (access(optarg, R_OK | X_OK) == 0)
        m_LocaleDirectory = optarg;
      else
      {
        fprintf(stderr, "vdr: can't access locale directory: %s\n", optarg);
        return false;
      }
      break;
    // mute
    case 'm':
      m_MuteAudio = true;
      break;
    // no-kbd
    case 'n' | 0x100:
      m_UseKbd = false;
      break;
    // plugin
    case 'p':
      if (is_number(optarg))
        m_SVDRPport = atoi(optarg);
      else
      {
        fprintf(stderr, "vdr: invalid port number: %s\n", optarg);
        return false;
      }
      break;
    // port
//    case 'P':
//      m_pluginManager->AddPlugin(optarg);
//      break;
    // record
    case 'r':
      cRecordingUserCommand::SetCommand(optarg);
      break;
    // resdir
    case 'r' | 0x100:
      m_ResourceDirectory = optarg;
      break;
    // shutdown
    case 's':
      ShutdownHandler.SetShutdownCommand(optarg);
      break;
    // split
    case 's' | 0x100:
      Setup.SplitEditedFiles = 1;
      break;
    // terminal
    case 't':
      m_Terminal = optarg;
      if (access(m_Terminal.c_str(), R_OK | W_OK) < 0)
      {
        fprintf(stderr, "vdr: can't access terminal: %s\n", m_Terminal.c_str());
        return false;
      }
      break;
    // user
    case 'u':
      if (*optarg)
        m_VdrUser = optarg;
      break;
    // userdump
    case 'u' | 0x100:
      m_UserDump = true;
      break;
    // version
    case 'V':
      m_DisplayVersion = true;
      break;
    // vfat
    case 'v' | 0x100:
      DirectoryPathMax = 250;
      DirectoryNameMax = 40;
      DirectoryEncoding = true;
      break;
    // video
    case 'v':
      m_VideoDirectory = optarg;
      while (optarg && *optarg && optarg[strlen(optarg) - 1] == '/')
        optarg[strlen(optarg) - 1] = 0;
      break;
    default:
      return false;
      }
  }

  // Help and version info:
  if (m_DisplayHelp || m_DisplayVersion)
  {
//     if (!m_pluginManager->HasPlugins())
//       m_pluginManager->AddPlugin("*"); // adds all available plugins
//     m_pluginManager->LoadPlugins();

     if (m_DisplayHelp)
     {
        printf(MSG_HELP,
               DEFAULTCACHEDIR,
               DEFAULTCONFDIR,
               PATH_MAX - 1,
               NAME_MAX,
               DEFAULTEPGDATAFILENAME,
               MAXVIDEOFILESIZEDEFAULT,
               DEFAULTPLUGINDIR,
               LIRC_DEVICE,
               DEFAULTLOCDIR,
               DEFAULTSVDRPPORT,
               DEFAULTRESDIR,
               DEFAULTVIDEODIR
               );
     }

    if (m_DisplayVersion)
      printf("vdr (%s/%s) - The Video Disk Recorder\n", VDRVERSION, APIVERSION);
//    if (m_pluginManager->HasPlugins())
//    {
//      if (m_DisplayHelp)
//        printf("Plugins: vdr -P\"name [OPTIONS]\"\n\n");
//      for (int i = 0;; i++)
//      {
//        cPlugin *p = m_pluginManager->GetPlugin(i);
//        if (p)
//        {
//          const char *help = p->CommandLineHelp();
//          printf("%s (%s) - %s\n", p->Name(), p->Version(), p->Description());
//          if (m_DisplayHelp && help)
//          {
//            printf("\n");
//            puts(help);
//          }
//        }
//        else
//          break;
//      }
//    }
    return false; // TODO: main() should return 0 instead of 2
  }

  // Log file:
  if (SysLogLevel > 0)
    openlog("vdr", LOG_CONS, m_SysLogTarget); // LOG_PID doesn't work as expected under NPTL

  // Set user id in case we were started as root:
  if (!m_VdrUser.empty() && geteuid() == 0)
  {
    m_StartedAsRoot = true;
    if (strcmp(m_VdrUser.c_str(), "root"))
    {
      if (!SetKeepCaps(true))
        return false;
      if (!SetUser(m_VdrUser, m_UserDump))
        return false;
      if (!SetKeepCaps(false))
        return false;
      if (!DropCaps())
        return false;
    }
    isyslog("switched to user '%s'", m_VdrUser.c_str());
  }

  // Check the video directory:
  if (!CDirectory::CanWrite(m_VideoDirectory))
  {
    fprintf(stderr, "vdr: can't access video directory %s\n", m_VideoDirectory.c_str());
    return false;
  }

  // Daemon mode:
  if (m_DaemonMode)
  {
    if (daemon(1, 0) == -1)
    {
      fprintf(stderr, "vdr: %m\n");
      esyslog("ERROR: %m");
      return false;
    }
    dsyslog("running as daemon (tid=%d)", cThread::ThreadId());
  }
#ifndef ANDROID
  else if (!m_Terminal.empty())
  {
    // Claim new controlling terminal
    stdin = freopen(m_Terminal.c_str(), "r", stdin);
    stdout = freopen(m_Terminal.c_str(), "w", stdout);
    stderr = freopen(m_Terminal.c_str(), "w", stderr);
    m_HasStdin = true;
    tcgetattr(STDIN_FILENO, &m_savedTm);
  }
#endif

  isyslog("VDR version %s started", VDRVERSION);

  return true;
}

bool cSettings::SetKeepCaps(bool On)
{
  // set keeping capabilities during setuid() on/off
  if (prctl(PR_SET_KEEPCAPS, On ? 1 : 0, 0, 0, 0) != 0)
  {
    fprintf(stderr, "vdr: prctl failed\n");
    return false;
  }
  return true;
}

bool cSettings::SetUser(const std::string& UserName, bool UserDump)
{
  if (!UserName.empty())
  {
    struct passwd *user = getpwnam(UserName.c_str());
    if (!user)
    {
      fprintf(stderr, "vdr: unknown user: '%s'\n", UserName.c_str());
      return false;
    }
    if (setgid(user->pw_gid) < 0)
    {
      fprintf(stderr, "vdr: cannot set group id %u: %s\n", (unsigned int) user->pw_gid, strerror(errno));
      return false;
    }
    if (initgroups(user->pw_name, user->pw_gid) < 0)
    {
      fprintf(stderr, "vdr: cannot set supplemental group ids for user %s: %s\n", user->pw_name, strerror(errno));
      return false;
    }
    if (setuid(user->pw_uid) < 0)
    {
      fprintf(stderr, "vdr: cannot set user id %u: %s\n", (unsigned int) user->pw_uid, strerror(errno));
      return false;
    }
    if (UserDump && prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0)
      fprintf(stderr, "vdr: warning - cannot set dumpable: %s\n", strerror(errno));
    setenv("HOME", user->pw_dir, 1);
    setenv("USER", user->pw_name, 1);
    setenv("LOGNAME", user->pw_name, 1);
    setenv("SHELL", user->pw_shell, 1);
  }
  return true;
}

bool cSettings::DropCaps(void)
{
#ifdef HAVE_CAP
  // drop all capabilities except selected ones
  cap_t caps = cap_from_text("= cap_sys_nice,cap_sys_time,cap_net_raw=ep");
  if (!caps)
  {
    fprintf(stderr, "vdr: cap_from_text failed: %s\n", strerror(errno));
    return false;
  }
  if (cap_set_proc(caps) == -1)
  {
    fprintf(stderr, "vdr: cap_set_proc failed: %s\n", strerror(errno));
    cap_free(caps);
    return false;
  }
  cap_free(caps);
#endif
  return true;
}

cSettings& cSettings::Get(void)
{
  static cSettings _instance;
  return _instance;
}
