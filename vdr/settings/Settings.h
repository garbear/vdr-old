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
#include "Types.h"
#include "utils/I18N.h"
#include "utils/DateTime.h"

#include <termios.h>
#include <string>
#include <stdint.h>
#include <limits.h>

#define MAXDEVICES 64

// default settings
#define ALLOWED_HOSTS_FILE  "allowed_hosts.conf"
#define FRONTEND_DEVICE     "/dev/dvb/adapter%d/frontend%d"

#define LISTEN_PORT       34890
#define LISTEN_PORT_S    "34890"
#define DISCOVERY_PORT    34890

class TiXmlElement;
class TiXmlNode;

class cSettings
{
public:
  static cSettings& Get(void);
  ~cSettings();

  bool LoadFromCmdLine(int argc, char *argv[]);

  bool Load(void);
  bool Load(const std::string& strFilename);
  bool Save(const std::string& strFilename);

  //XXX guess we should port over xbmc's settings too :)
  std::string         m_ConfigDirectory;
  bool                m_DaemonMode;
  int                 m_SysLogTarget;
  bool                m_bSplitEditedFiles;
  std::string         m_VideoDirectory;
  std::string         m_EPGDirectory;
  int                 m_EPGLanguages[I18N_MAX_LANGUAGES + 1];
  CDateTime           m_nextWakeupTime;

  // Remote server settings
  uint16_t            m_ListenPort;         // Port of remote server
  uint16_t            m_StreamTimeout;      // timeout in seconds for stream data

  int                 m_PmtTimeout;
  int                 m_TimeshiftMode;
  int                 m_TimeshiftBufferSize;
  int                 m_TimeshiftBufferFileSize;
  std::string         m_TimeshiftBufferDir;

  int                 m_iInstantRecordTime;
  int                 m_iDefaultPriority;
  int                 m_iDefaultLifetime;
  bool                m_bRecordSubtitleName;
  bool                m_bUseVps;
  int                 m_iVpsMargin;

  int                 m_iLnbSLOF;
  int                 m_iLnbFreqLow;
  int                 m_iLnbFreqHigh;
  bool                m_bDiSEqC;

  bool                m_bSetSystemTime;
  int                 m_iTimeSource;
  int                 m_iTimeTransponder;
  int                 m_iUpdateChannels;
  int                 m_iMaxVideoFileSizeMB;
  int                 m_iResumeID;
  std::string         m_strDeviceBondings;
  sys_log_level_t     m_SysLogLevel;
  sys_log_type_t      m_SysLogType;

  int                 m_iStandardCompliance;
  int                 m_iMarginStart;
  int                 m_iMarginEnd;

  int                 m_iEPGScanTimeout;
  int                 m_iEPGBugfixLevel;
  int                 m_iEPGLinger;
private:
  cSettings();
  bool SetKeepCaps(bool On);
  bool SetUser(const std::string& UserName, bool UserDump);
  bool DropCaps(void);

  bool GetSettingString(TiXmlElement* element, const char* nodeName, std::string& value);
  bool GetSettingInt(TiXmlElement* element, const char* nodeName, int& value);
  bool GetSettingDateTime(TiXmlElement* element, const char* nodeName, CDateTime& value);
  bool GetSettingBool(TiXmlElement* element, const char* nodeName, bool& value);

  bool SaveSetting(TiXmlNode* element, const char* nodeName, const std::string& value);
  bool SaveSetting(TiXmlNode* element, const char* nodeName, int value);
  bool SaveSetting(TiXmlNode* element, const char* nodeName, const CDateTime& value);
  bool SaveSetting(TiXmlNode* element, const char* nodeName, bool value);

  std::string StoreLanguages(int* Values);
  bool ParseLanguages(const char *Value, int *Values);

  bool           m_StartedAsRoot;
  bool           m_HasStdin;
  struct termios m_savedTm;
  std::string    m_strFilename;
};
