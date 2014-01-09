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
#pragma once

#include <termios.h>
#include <string>
#include <stdint.h>
#include <limits.h>

#define DEFAULTEPGDATAFILENAME "epg.data"

#define DEFAULTCONFDIR  "/var/lib/vdr"
#define DEFAULTCACHEDIR "/var/cache/vdr"
#define DEFAULTRESDIR   "/usr/local/share/vdr"
#define VIDEODIR        "/srv/vdr/video" // used in videodir.cpp
#define MAXDEVICES 64

// default settings
#define ALLOWED_HOSTS_FILE  "allowed_hosts.conf"
#define FRONTEND_DEVICE     "/dev/dvb/adapter%d/frontend%d"

#define LISTEN_PORT       34890
#define LISTEN_PORT_S    "34890"
#define DISCOVERY_PORT    34890

class cPluginManager;

class cSettings
{
public:
  static cSettings& Get(void);
  ~cSettings();

  bool LoadFromCmdLine(int argc, char *argv[]);

  std::string    m_AudioCommand;
  std::string    m_CacheDirectory;
  std::string    m_ConfigDirectory;
  bool           m_DaemonMode;
  std::string    m_EpgDataFileName;
  bool           m_DisplayHelp;
  int            m_SysLogTarget;
//  cPluginManager* m_pluginManager;
  std::string    m_LocaleDirectory;
  bool           m_MuteAudio;
  bool           m_UseKbd;
  int            m_SVDRPport;
  std::string    m_ResourceDirectory;
  std::string    m_Terminal;
  std::string    m_VdrUser;
  bool           m_UserDump;
  bool           m_DisplayVersion;
  std::string    m_VideoDirectory;
  bool           m_StartedAsRoot;
  bool           m_HasStdin;
  struct termios m_savedTm;

  // Remote server settings
  uint16_t       m_ListenPort;         // Port of remote server
  uint16_t       m_StreamTimeout;      // timeout in seconds for stream data

  int            m_PmtTimeout;
  int            m_TimeshiftMode;
  int            m_TimeshiftBufferSize;
  int            m_TimeshiftBufferFileSize;
  std::string    m_TimeshiftBufferDir;

private:
  cSettings();
  bool SetKeepCaps(bool On);
  bool SetUser(const std::string& UserName, bool UserDump);
  bool DropCaps(void);
};
