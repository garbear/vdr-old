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

#include <termios.h>

#define DEFAULTEPGDATAFILENAME "epg.data"

#define DEFAULTCONFDIR  "/var/lib/vdr"
#define DEFAULTCACHEDIR "/var/cache/vdr"
#define DEFAULTRESDIR   "/usr/local/share/vdr"
#define VIDEODIR        "/srv/vdr/video" // used in videodir.cpp

class cPluginManager;

class cSettings
{
public:
  cSettings();
  ~cSettings();

  bool LoadFromCmdLine(int argc, char *argv[]);

  const char *   m_AudioCommand;
  const char *   m_CacheDirectory;
  const char *   m_ConfigDirectory;
  bool           m_DaemonMode;
  const char *   m_EpgDataFileName;
  bool           m_DisplayHelp;
  int            m_SysLogTarget;
  cPluginManager* m_pluginManager;
  const char *   m_LircDevice;
  const char *   m_LocaleDirectory;
  bool           m_MuteAudio;
  bool           m_UseKbd;
  int            m_SVDRPport;
  const char *   m_ResourceDirectory;
  const char *   m_Terminal;
  const char *   m_VdrUser;
  bool           m_UserDump;
  bool           m_DisplayVersion;
  const char *   m_VideoDirectory;
  bool           m_StartedAsRoot;
  bool           m_HasStdin;
  struct termios m_savedTm;

private:
  bool SetKeepCaps(bool On);
  bool SetUser(const char *UserName, bool UserDump);
  bool DropCaps(void);
};
