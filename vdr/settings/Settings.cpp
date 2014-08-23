/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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
#include "SettingsDefinitions.h"
#include "filesystem/Directory.h"
#include "Config.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "recordings/filesystem/IndexFile.h"
#include "recordings/Recording.h"
#include "recordings/RecordingCutter.h"
#include "recordings/RecordingUserCommand.h"
#include "utils/log/Log.h"
#include "utils/Shutdown.h"
#include "utils/Tools.h"
#include "utils/XBMCTinyXML.h"

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
#include <unistd.h>

#if !defined(TARGET_ANDROID)
#include <sys/syslog.h>
#endif

namespace VDR
{

// for easier orientation, this is column 80|
#define MSG_HELP "Usage: vdr [OPTIONS]\n\n" \
                 "  -c DIR,   --config=DIR   read config files from DIR (default: %s)\n" \
                 "  -d,       --daemon       run in daemon mode\n" \
                 "  -h,       --help         print this help and exit\n" \
                 "  -V,       --version      print version information and exit\n" \
                 "\n"

cSettings::cSettings()
{
  m_DaemonMode        = false;
#if !defined(TARGET_ANDROID)
  m_SysLogTarget      = LOG_USER;
#endif
  m_StartedAsRoot     = false;
  m_bSplitEditedFiles = false;
  m_HasStdin          = (tcgetpgrp(STDIN_FILENO) == getpid() || getppid() != (pid_t)1) && tcgetattr(STDIN_FILENO, &m_savedTm) == 0;

  m_ListenPort              = LISTEN_PORT;
  m_StreamTimeout           = 10;
  m_PmtTimeout              = 5;
  m_TimeshiftMode           = (int)TS_MODE_NONE;
  m_TimeshiftBufferSize     = 5;
  m_TimeshiftBufferFileSize = 6;
  m_iInstantRecordTime      = DEFINSTRECTIME;
  m_iLnbSLOF                = 11700;
  m_iLnbFreqLow             =  9750;
  m_iLnbFreqHigh            = 10600;
  m_bDiSEqC                 = false;
  m_bSetSystemTime          = false;
  m_iTimeTransponder        = 0;
  m_iStandardCompliance     = STANDARD_DVB;
  m_iMarginStart            = 2;
  m_iMarginEnd              = 10;
  m_iEPGScanTimeout         = 5;
  m_iEPGBugfixLevel         = 3;
  m_iEPGLinger              = 0;
  m_iDefaultPriority        = 50;
  m_iDefaultLifetime        = MAXLIFETIME;
  m_bRecordSubtitleName     = true;
  m_bUseVps                 = false;
  m_iVpsMargin              = 120;
  m_iUpdateChannels         = 5;
  m_iMaxVideoFileSizeMB     = MAXVIDEOFILESIZEDEFAULT;
  m_iResumeID               = 0;
  m_EPGLanguages[0]         = -1;
  m_SysLogLevel             = SYS_LOG_DEBUG;
  m_SysLogType              = SYS_LOG_TYPE_CONSOLE;

  // TODO: Load these paths from settings (assuming they are even used)
  m_VideoDirectory  = "special://home/video";
  m_ConfigDirectory = "special://home/system";
  m_EPGDirectory    = "special://home/epg";
}

cSettings::~cSettings()
{
  if (m_HasStdin)
    tcsetattr(STDIN_FILENO, TCSANOW, &m_savedTm);
}

bool cSettings::GetSettingString(TiXmlElement* element, const char* nodeName, std::string& value)
{
  if (const TiXmlNode *node = element->FirstChild(nodeName))
  {
    const TiXmlElement *elem = node->ToElement();
    if (elem && elem->Attribute(SETTINGS_XML_ATTR_VALUE))
    {
      value = elem->Attribute(SETTINGS_XML_ATTR_VALUE);
      return true;
    }
  }
  return false;
}

bool cSettings::GetSettingInt(TiXmlElement* element, const char* nodeName, int& value)
{
  std::string strVal;
  if (GetSettingString(element, nodeName, strVal))
  {
    value = StringUtils::IntVal(strVal);
    return true;
  }
  return false;
}

bool cSettings::GetSettingDateTime(TiXmlElement* element, const char* nodeName, CDateTime& value)
{
  std::string strVal;
  if (GetSettingString(element, nodeName, strVal))
  {
    value = CDateTime((time_t)StringUtils::IntVal(strVal));
    return value.IsValid();
  }
  return false;
}

bool cSettings::GetSettingBool(TiXmlElement* element, const char* nodeName, bool& value)
{
  std::string strVal;
  if (GetSettingString(element, nodeName, strVal))
  {
    value = StringUtils::IntVal(strVal) == 1;
    return true;
  }
  return false;
}

bool cSettings::SaveSetting(TiXmlNode* element, const char* nodeName, const std::string& value)
{
  TiXmlElement channelElement(nodeName);
  TiXmlNode* node = element->InsertEndChild(channelElement);
  node->ToElement()->SetAttribute(SETTINGS_XML_ATTR_VALUE, value);
  return true;
}

bool cSettings::SaveSetting(TiXmlNode* element, const char* nodeName, int value)
{
  TiXmlElement channelElement(nodeName);
  TiXmlNode* node = element->InsertEndChild(channelElement);
  node->ToElement()->SetAttribute(SETTINGS_XML_ATTR_VALUE, value);
  return true;
}

bool cSettings::SaveSetting(TiXmlNode* element, const char* nodeName, const CDateTime& value)
{
  TiXmlElement channelElement(nodeName);
  TiXmlNode* node = element->InsertEndChild(channelElement);
  time_t tm;
  value.GetAsTime(tm);
  node->ToElement()->SetAttribute(SETTINGS_XML_ATTR_VALUE, tm);
  return true;
}

bool cSettings::SaveSetting(TiXmlNode* element, const char* nodeName, bool value)
{
  TiXmlElement channelElement(nodeName);
  TiXmlNode* node = element->InsertEndChild(channelElement);
  node->ToElement()->SetAttribute(SETTINGS_XML_ATTR_VALUE, value);
  return true;
}

bool cSettings::Load(void)
{
  if (!Load("special://home/system/config.xml") &&
      !Load("special://vdr/system/config.xml"))
  {
    // create a new empty file
    Save("special://home/system/config.xml");
    return false;
  }

  return true;
}

bool cSettings::Load(const std::string& strFilename)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strFilename.c_str()))
  {
    esyslog("failed to load '%s'", strFilename.c_str());
    return false;
  }

  m_strFilename = strFilename;

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), SETTINGS_XML_ROOT))
  {
    esyslog("failed to find root element '%s' in '%s'", SETTINGS_XML_ROOT, strFilename.c_str());
    return false;
  }

  int iValue;
  std::string strValue;

  if (GetSettingInt(root, SETTINGS_XML_ELM_LISTEN_PORT, iValue) && iValue > 0)
    m_ListenPort = iValue;

  if (GetSettingInt(root, SETTINGS_XML_ELM_STREAM_TIMEOUT, iValue) && iValue > 0)
    m_StreamTimeout = iValue;

  GetSettingInt(root,      SETTINGS_XML_ELM_PMT_TIMEOUT,                m_PmtTimeout);
  GetSettingInt(root,      SETTINGS_XML_ELM_INSTANT_RECORD_TIME,        m_iInstantRecordTime);
  GetSettingInt(root,      SETTINGS_XML_ELM_DEFAULT_RECORDING_PRIORITY, m_iDefaultPriority);
  GetSettingInt(root,      SETTINGS_XML_ELM_DEFAULT_RECORDING_LIFETIME, m_iDefaultLifetime);
  GetSettingBool(root,     SETTINGS_XML_ELM_RECORD_SUBTITLE_NAME,       m_bRecordSubtitleName);
  GetSettingBool(root,     SETTINGS_XML_ELM_USE_VPS,                    m_bUseVps);
  GetSettingInt(root,      SETTINGS_XML_ELM_VPS_MARGIN,                 m_iVpsMargin);
  GetSettingInt(root,      SETTINGS_XML_ELM_MAX_RECORDING_FILESIZE_MB,  m_iMaxVideoFileSizeMB);
  GetSettingInt(root,      SETTINGS_XML_ELM_RESUME_ID,                  m_iResumeID);
  GetSettingDateTime(root, SETTINGS_XML_ELM_NEXT_WAKEUP,                m_nextWakeupTime);

  GetSettingInt(root,      SETTINGS_XML_ELM_LNB_SLOF,                   m_iLnbSLOF);
  GetSettingInt(root,      SETTINGS_XML_ELM_LNB_FREQ_LOW,               m_iLnbFreqLow);
  GetSettingInt(root,      SETTINGS_XML_ELM_LNB_FREQ_HIGH,              m_iLnbFreqHigh);
  GetSettingBool(root,     SETTINGS_XML_ELM_DISEQC,                     m_bDiSEqC);

  GetSettingBool(root,     SETTINGS_XML_ELM_SET_SYSTEM_TIME,            m_bSetSystemTime);
  GetSettingInt(root,      SETTINGS_XML_ELM_TIME_TRANSPONDER,           m_iTimeTransponder);

  GetSettingInt(root,      SETTINGS_XML_ELM_UPDATE_CHANNELS_LEVEL,      m_iUpdateChannels);

  GetSettingInt(root,      SETTINGS_XML_ELM_STANDARD_COMPLIANCE,        m_iStandardCompliance);
  GetSettingInt(root,      SETTINGS_XML_ELM_MARGIN_START,               m_iMarginStart);
  GetSettingInt(root,      SETTINGS_XML_ELM_MARGIN_END,                 m_iMarginEnd);

  GetSettingInt(root,      SETTINGS_XML_ELM_EPG_SCAN_TIMEOUT,           m_iEPGScanTimeout);
  GetSettingInt(root,      SETTINGS_XML_ELM_EPG_BUGFIX_LEVEL,           m_iEPGBugfixLevel);
  GetSettingInt(root,      SETTINGS_XML_ELM_EPG_LINGER_TIME,            m_iEPGLinger);
  if (GetSettingString(root, SETTINGS_XML_ELM_EPG_LANGUAGES, strValue))
    ParseLanguages(strValue.c_str(), m_EPGLanguages);

  GetSettingInt(root,      SETTINGS_XML_ELM_TIMESHIFT_MODE,             m_TimeshiftMode);
  GetSettingInt(root,      SETTINGS_XML_ELM_TIMESHIFT_BUFFER_SIZE,      m_TimeshiftBufferSize);
  GetSettingInt(root,      SETTINGS_XML_ELM_TIMESHIFT_BUFFER_FILE_SIZE, m_TimeshiftBufferFileSize);
  GetSettingString(root,   SETTINGS_XML_ELM_TIMESHIFT_BUFFER_FILE,      m_TimeshiftBufferDir);

  if (GetSettingInt(root,  SETTINGS_XML_ELM_SYSLOG_TYPE,                iValue))
    m_SysLogType = (sys_log_type_t)iValue;

  if (GetSettingInt(root,  SETTINGS_XML_ELM_SYSLOG_LEVEL,               iValue))
    m_SysLogLevel = (sys_log_level_t)iValue;

  return true;
}

bool cSettings::Save(const std::string& strFilename)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(SETTINGS_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  SaveSetting(root, SETTINGS_XML_ELM_LISTEN_PORT, (int)m_ListenPort);
  SaveSetting(root, SETTINGS_XML_ELM_STREAM_TIMEOUT, (int)m_StreamTimeout);
  SaveSetting(root, SETTINGS_XML_ELM_PMT_TIMEOUT,                m_PmtTimeout);
  SaveSetting(root, SETTINGS_XML_ELM_INSTANT_RECORD_TIME,        m_iInstantRecordTime);
  SaveSetting(root, SETTINGS_XML_ELM_DEFAULT_RECORDING_PRIORITY, m_iDefaultPriority);
  SaveSetting(root, SETTINGS_XML_ELM_DEFAULT_RECORDING_LIFETIME, m_iDefaultLifetime);
  SaveSetting(root, SETTINGS_XML_ELM_RECORD_SUBTITLE_NAME,       m_bRecordSubtitleName);
  SaveSetting(root, SETTINGS_XML_ELM_USE_VPS,                    m_bUseVps);
  SaveSetting(root, SETTINGS_XML_ELM_VPS_MARGIN,                 m_iVpsMargin);
  SaveSetting(root, SETTINGS_XML_ELM_MAX_RECORDING_FILESIZE_MB,  m_iMaxVideoFileSizeMB);
  SaveSetting(root, SETTINGS_XML_ELM_RESUME_ID,                  m_iResumeID);
  SaveSetting(root, SETTINGS_XML_ELM_NEXT_WAKEUP,                m_nextWakeupTime);
  SaveSetting(root, SETTINGS_XML_ELM_SYSLOG_TYPE,                m_SysLogType);
  SaveSetting(root, SETTINGS_XML_ELM_SYSLOG_LEVEL,               m_SysLogLevel);

  SaveSetting(root, SETTINGS_XML_ELM_LNB_SLOF,                   m_iLnbSLOF);
  SaveSetting(root, SETTINGS_XML_ELM_LNB_FREQ_LOW,               m_iLnbFreqLow);
  SaveSetting(root, SETTINGS_XML_ELM_LNB_FREQ_HIGH,              m_iLnbFreqHigh);
  SaveSetting(root, SETTINGS_XML_ELM_DISEQC,                     m_bDiSEqC);

  SaveSetting(root, SETTINGS_XML_ELM_SET_SYSTEM_TIME,            m_bSetSystemTime);
  SaveSetting(root, SETTINGS_XML_ELM_TIME_TRANSPONDER,           m_iTimeTransponder);
  SaveSetting(root, SETTINGS_XML_ELM_UPDATE_CHANNELS_LEVEL,      m_iUpdateChannels);

  SaveSetting(root, SETTINGS_XML_ELM_STANDARD_COMPLIANCE,        m_iStandardCompliance);
  SaveSetting(root, SETTINGS_XML_ELM_MARGIN_START,               m_iMarginStart);
  SaveSetting(root, SETTINGS_XML_ELM_MARGIN_END,                 m_iMarginEnd);

  SaveSetting(root, SETTINGS_XML_ELM_EPG_SCAN_TIMEOUT,           m_iEPGScanTimeout);
  SaveSetting(root, SETTINGS_XML_ELM_EPG_BUGFIX_LEVEL,           m_iEPGBugfixLevel);
  SaveSetting(root, SETTINGS_XML_ELM_EPG_LINGER_TIME,            m_iEPGLinger);
  SaveSetting(root, SETTINGS_XML_ELM_EPG_LANGUAGES,              StoreLanguages(m_EPGLanguages));

  SaveSetting(root, SETTINGS_XML_ELM_TIMESHIFT_MODE,             m_TimeshiftMode);
  SaveSetting(root, SETTINGS_XML_ELM_TIMESHIFT_BUFFER_SIZE,      m_TimeshiftBufferSize);
  SaveSetting(root, SETTINGS_XML_ELM_TIMESHIFT_BUFFER_FILE_SIZE, m_TimeshiftBufferFileSize);
  SaveSetting(root, SETTINGS_XML_ELM_TIMESHIFT_BUFFER_FILE,      m_TimeshiftBufferDir);

  if (!strFilename.empty())
    m_strFilename = strFilename;

  assert(!m_strFilename.empty());

  isyslog("saving configuration to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("failed to save the configuration: could not write to '%s'", m_strFilename.c_str());
    return false;
  }
  return true;
}

bool cSettings::LoadFromCmdLine(int argc, char *argv[])
{
  std::string strVdrUser;
  bool        strUserDump = false;
  bool        bDisplayHelp = false;
  bool        bDisplayVersion = false;

#if defined(VDR_USER)
  strVdrUser = VDR_USER;
#endif

  static struct option long_options[] =
    {
      { "config",    required_argument, NULL, 'c' },
      { "daemon",    no_argument,       NULL, 'd' },
      { "help",      no_argument,       NULL, 'h' },
      { "version",   no_argument,       NULL, 'V' },
      { NULL,        no_argument,       NULL, 0 }
    };

  int c;
  while ((c = getopt_long(argc, argv, "c:d:h:V",
      long_options, NULL)) != -1)
  {
    switch (c)
      {
    // config
    case 'c':
      m_ConfigDirectory = optarg;
      break;
    // daemon
    case 'd':
      m_DaemonMode = true;
      break;
    // help
    case 'h':
      bDisplayHelp = true;
      break;
    // version
    case 'V':
      bDisplayVersion = true;
      break;
    default:
      return false;
      }
  }

  // Help and version info:
  if (bDisplayHelp || bDisplayVersion)
  {
    if (bDisplayHelp)
      printf(MSG_HELP, "DEFAULTCONFDIR");

    if (bDisplayVersion)
      printf("vdr %s - The Video Disk Recorder\n", VDRVERSION);

    return false; // TODO: main() should return 0 instead of 2
  }

  // Log file:
  CLog::Get().SetType(m_SysLogType);

  // Set user id in case we were started as root:
  if (!strVdrUser.empty() && geteuid() == 0)
  {
    m_StartedAsRoot = true;
    if (strcmp(strVdrUser.c_str(), "root"))
    {
      if (!SetKeepCaps(true))
        return false;
      if (!SetUser(strVdrUser, strUserDump))
        return false;
      if (!SetKeepCaps(false))
        return false;
      if (!DropCaps())
        return false;
    }
    isyslog("switched to user '%s'", strVdrUser.c_str());
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
    dsyslog("running as daemon");
  }

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

bool cSettings::ParseLanguages(const char *Value, int *Values)
{
  int n = 0;
  while (Value && *Value && n < (int)I18nLanguages().size())
  {
    char buffer[4];
    strn0cpy(buffer, Value, sizeof(buffer));
    int i = I18nLanguageIndex(buffer);
    if (i >= 0)
      Values[n++] = i;
    if ((Value = strchr(Value, ' ')) != NULL)
      Value++;
  }
  Values[n] = -1;
  return true;
}

std::string cSettings::StoreLanguages(int* Values)
{
  char buffer[I18nLanguages().size() * 4];
  char *q = buffer;
  for (int i = 0; i < (int)I18nLanguages().size(); i++)
  {
    if (Values[i] < 0)
      break;
    const char *s = I18nLanguageCode(Values[i]);
    if (s)
    {
      if (q > buffer)
        *q++ = ' ';
      strncpy(q, s, 3);
      q += 3;
    }
  }
  *q = 0;
  return buffer;
}

}
