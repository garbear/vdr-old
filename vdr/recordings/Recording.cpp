/*
 * recording.c: Recording file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: recording.c 2.91.1.2 2013/08/21 13:58:35 kls Exp $
 */

#include "Recording.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS // Required for format specifiers
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "channels/ChannelManager.h"
#include "utils/I18N.h"
//#include "interface.h"
#include "devices/Remux.h"
#include "utils/Ringbuffer.h"
#include "utils/Tools.h"
#include "filesystem/Videodir.h"
#include "filesystem/ReadDir.h"
#include "utils/CRC32.h"
#include "RecordingInfo.h"
#include "filesystem/FileName.h"
#include "Recordings.h"
#include "filesystem/IndexFile.h"
#include "RecordingUserCommand.h"

#include "filesystem/Directory.h"
#include "utils/UTF8Utils.h"
#include "utils/url/URLUtils.h"

#include <algorithm>

using namespace PLATFORM;
using namespace VDR;

int DirectoryPathMax = PATH_MAX - 1;
int DirectoryNameMax = NAME_MAX;
bool DirectoryEncoding = false;
int InstanceId = 0;

// --- cRecording ------------------------------------------------------------

#define RESUME_NOT_INITIALIZED (-2)

struct tCharExchange { char a; char b; };
tCharExchange CharExchange[] = {
  { FOLDERDELIMCHAR,  '/' },
  { '/',  FOLDERDELIMCHAR },
  { ' ',  '_'    },
  // backwards compatibility:
  { '\'', '\''   },
  { '\'', '\x01' },
  { '/',  '\x02' },
  { 0, 0 }
  };

const char *InvalidChars = "\"\\/:*?|<>#";

bool NeedsConversion(const char *p)
{
  return DirectoryEncoding &&
         (strchr(InvalidChars, *p) // characters that can't be part of a Windows file/directory name
          || *p == '.' && (!*(p + 1) || *(p + 1) == FOLDERDELIMCHAR)); // Windows can't handle '.' at the end of file/directory names
}

std::string PrefixVideoFileName(const std::string& strFileName, char Prefix)
{
  std::string retval;
  char PrefixedName[strFileName.size() + 2];

  const char* strFileNameCstr = strFileName.c_str();
  const char *p = strFileNameCstr + strFileName.size(); // p points at the terminating 0
  int n = 2;
  while (p-- > strFileNameCstr && n > 0)
  {
    if (*p == '/')
    {
      if (--n == 0)
      {
        int l = p - strFileNameCstr + 1;
        strncpy(PrefixedName, strFileNameCstr, l);
        PrefixedName[l] = Prefix;
        strcpy(PrefixedName + l + 1, p + 1);
        retval = PrefixedName;
        break;
      }
    }
  }
  return retval;
}

char *cRecording::ExchangeCharsXXX(char *s, bool ToFileSystem)
{
  char *p = s;
  while (*p)
  {
    if (DirectoryEncoding)
    {
      // Some file systems can't handle all characters, so we
      // have to take extra efforts to encode/decode them:
      if (ToFileSystem)
      {
        switch (*p)
          {
        // characters that can be mapped to other characters:
        case ' ':
          *p = '_';
          break;
        case FOLDERDELIMCHAR:
          *p = '/';
          break;
        case '/':
          *p = FOLDERDELIMCHAR;
          break;
          // characters that have to be encoded:
        default:
          if (NeedsConversion(p))
          {
            int l = p - s;
            if (char *NewBuffer = (char *) realloc(s, strlen(s) + 10))
            {
              s = NewBuffer;
              p = s + l;
              char buf[4];
              sprintf(buf, "#%02X", (unsigned char) *p);
              memmove(p + 2, p, strlen(p) + 1);
              strncpy(p, buf, 3);
              p += 2;
            }
            else
              esyslog("ERROR: out of memory");
          }
          }
      }
      else
      {
        switch (*p)
          {
        // mapped characters:
        case '_':
          *p = ' ';
          break;
        case FOLDERDELIMCHAR:
          *p = '/';
          break;
        case '/':
          *p = FOLDERDELIMCHAR;
          break;
          // encoded characters:
        case '#':
          {
            if (strlen(p) > 2 && isxdigit(*(p + 1)) && isxdigit(*(p + 2)))
            {
              char buf[3];
              sprintf(buf, "%c%c", *(p + 1), *(p + 2));
              uchar c = uchar(strtol(buf, NULL, 16));
              if (c)
              {
                *p = c;
                memmove(p + 1, p + 3, strlen(p) - 2);
              }
            }
          }
          break;
          // backwards compatibility:
        case '\x01':
          *p = '\'';
          break;
        case '\x02':
          *p = '/';
          break;
        case '\x03':
          *p = ':';
          break;
        default:
          ;
          }
      }
    }
    else
    {
      for (struct tCharExchange *ce = CharExchange; ce->a && ce->b; ce++)
      {
        if (*p == (ToFileSystem ? ce->a : ce->b))
        {
          *p = ToFileSystem ? ce->b : ce->a;
          break;
        }
      }
    }
    p++;
  }
  return s;
}

char *LimitNameLengthsXXX(char *s, int PathMax, int NameMax)
{
  // Limits the total length of the directory path in 's' to PathMax, and each
  // individual directory name to NameMax. The lengths of characters that need
  // conversion when using 's' as a file name are taken into account accordingly.
  // If a directory name exceeds NameMax, it will be truncated. If the whole
  // directory path exceeds PathMax, individual directory names will be shortened
  // (from right to left) until the limit is met, or until the currently handled
  // directory name consists of only a single character. All operations are performed
  // directly on the given 's', which may become shorter (but never longer) than
  // the original value.
  // Returns a pointer to 's'.
  int Length = strlen(s);
  int PathLength = 0;
  // Collect the resulting lengths of each character:
  bool NameTooLong = false;
  int8_t a[Length];
  int n = 0;
  int NameLength = 0;
  for (char *p = s; *p; p++)
  {
    if (*p == FOLDERDELIMCHAR)
    {
      a[n] = -1; // FOLDERDELIMCHAR is a single character, neg. sign marks it
      NameTooLong |= NameLength > NameMax;
      NameLength = 0;
      PathLength += 1;
    }
    else if (NeedsConversion(p))
    {
      a[n] = 3; // "#xx"
      NameLength += 3;
      PathLength += 3;
    }
    else
    {
      int8_t l = cUtf8Utils::Utf8CharLen(p);
      a[n] = l;
      NameLength += l;
      PathLength += l;
      while (l-- > 1)
      {
        a[++n] = 0;
        p++;
      }
    }
    n++;
  }
  NameTooLong |= NameLength > NameMax;
  // Limit names to NameMax:
  if (NameTooLong)
  {
    while (n > 0)
    {
      // Calculate the length of the current name:
      int NameLength = 0;
      int i = n;
      int b = i;
      while (i-- > 0 && a[i] >= 0)
      {
        NameLength += a[i];
        b = i;
      }
      // Shorten the name if necessary:
      if (NameLength > NameMax)
      {
        int l = 0;
        i = n;
        while (i-- > 0 && a[i] >= 0)
        {
          l += a[i];
          if (NameLength - l <= NameMax)
          {
            memmove(s + i, s + n, Length - n + 1);
            memmove(a + i, a + n, Length - n + 1);
            Length -= n - i;
            PathLength -= l;
            break;
          }
        }
      }
      // Switch to the next name:
      n = b - 1;
    }
  }
  // Limit path to PathMax:
  n = Length;
  while (PathLength > PathMax && n > 0)
  {
    // Calculate how much to cut off the current name:
    int i = n;
    int b = i;
    int l = 0;
    while (--i > 0 && a[i - 1] >= 0)
    {
      if (a[i] > 0)
      {
        l += a[i];
        b = i;
        if (PathLength - l <= PathMax)
          break;
      }
    }
    // Shorten the name if necessary:
    if (l > 0)
    {
      memmove(s + b, s + n, Length - n + 1);
      Length -= n - b;
      PathLength -= l;
    }
    // Switch to the next name:
    n = i - 1;
  }
  return s;
}

std::string LimitNameLengths(const std::string& strSubject, int PathMax, int NameMax)
{
  char* ns = LimitNameLengthsXXX(strdup(strSubject.c_str()), PathMax, NameMax);
  std::string strReturn = ns;
  free(ns);
  return strReturn;
}

std::string cRecording::ExchangeChars(const std::string& strSubject, bool ToFileSystem)
{
  char* ns = ExchangeCharsXXX(strdup(strSubject.c_str()), ToFileSystem);
  std::string strReturn = ns;
  free(ns);
  return strReturn;
}

cRecording::cRecording(TimerPtr Timer, const cEvent *Event)
{
  m_iResume = RESUME_NOT_INITIALIZED;
  m_iFileSizeMB = -1; // unknown
  m_iChannel = Timer->Channel()->Number();
  m_iInstanceId = InstanceId;
  m_bIsPesRecording = false;
  m_iIsOnVideoDirectoryFileSystem = -1; // unknown
  m_dFramesPerSecond = DEFAULTFRAMESPERSECOND;
  m_iNumFrames = -1;
  m_deleted = 0;
  m_hash = -1;
  // set up the actual name:
  std::string strTitle = Event ? Event->Title() : "";
  std::string strSubtitle = Event ? Event->ShortText() : "";
  if (strTitle.empty())
    strTitle = Timer->Channel()->Name();
  if (strSubtitle.empty())
    strSubtitle = " ";
  size_t macroTitle = Timer->File().find(TIMERMACRO_TITLE);
  size_t macroEpisode = Timer->File().find(TIMERMACRO_EPISODE);
  if (macroTitle != std::string::npos || macroEpisode != std::string::npos)
  {
    std::string strName = Timer->File();
    StringUtils::Replace(strName, TIMERMACRO_TITLE, strTitle);
    StringUtils::Replace(strName, TIMERMACRO_EPISODE, strSubtitle);
    m_strName = strName;
    // avoid blanks at the end:
    size_t l = m_strName.size();
    while (l-- > 2)
    {
      if (m_strName.at(l) == ' ' && m_strName.at(l - 1) != FOLDERDELIMCHAR)
        m_strName.erase(l);
      else
        break;
    }
    if (Timer->IsRepeatingEvent())
    {
      Timer->SetFile(m_strName); // this was an instant recording, so let's set the actual data
      cTimers::Get().SetModified();
    }
  }
  else if (Timer->IsRepeatingEvent() || !g_setup.UseSubtitle)
  {
    m_strName = Timer->File();
  }
  else
  {
    m_strName = StringUtils::Format("%s~%s", Timer->File().c_str(), strSubtitle.c_str());
  }
  // substitute characters that would cause problems in file names:
  StringUtils::Replace(m_strName, '\n', ' ');
  m_start = Timer->StartTime();
  m_iPriority = Timer->Priority();
  m_iLifetime = Timer->Lifetime();
  // handle info:
  m_recordingInfo = new cRecordingInfo(Timer->Channel().get(), Event);
  m_recordingInfo->priority = m_iPriority;
  m_recordingInfo->lifetime = m_iLifetime;
}

cRecording::cRecording(const std::string& strFileName)
{
  m_iResume                       = RESUME_NOT_INITIALIZED;
  m_iFileSizeMB                   = -1; // unknown
  m_iChannel                      = -1;
  m_iInstanceId                   = -1;
  m_iPriority                     = MAXPRIORITY; // assume maximum in case there is no info file
  m_iLifetime                     = MAXLIFETIME;
  m_bIsPesRecording               = false;
  m_iIsOnVideoDirectoryFileSystem = -1; // unknown
  m_dFramesPerSecond              = DEFAULTFRAMESPERSECOND;
  m_iNumFrames                    = -1;
  m_deleted                       = 0;
  m_hash                          = -1;
  m_strFileName                   = strFileName;

  //XXX fix this up

  // strip slash
  size_t pos = m_strFileName.find('/');
  if (pos != std::string::npos)
    m_strFileName.erase(pos);

  pos = m_strFileName.find(VideoDirectory);
  if (pos != std::string::npos)
    m_strFileName.erase((size_t)0, pos);
  const char *p = strrchr(m_strFileName.c_str(), '/');

  m_strName.clear();
  m_recordingInfo = new cRecordingInfo(m_strFileName);
  if (p) {
     time_t now = time(NULL);
     struct tm tm_r;
     struct tm t = *localtime_r(&now, &tm_r); // this initializes the time zone in 't'
     t.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
     if (7 == sscanf(p + 1, DATAFORMATTS, &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &m_iChannel, &m_iInstanceId)
      || 7 == sscanf(p + 1, DATAFORMATPES, &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &m_iPriority, &m_iLifetime)) {
        t.tm_year -= 1900;
        t.tm_mon--;
        t.tm_sec = 0;
        m_start = mktime(&t);
        m_strName = strFileName;
        m_strName.erase(p - strFileName.c_str());
        m_strName = ExchangeChars(m_strName, false);
        m_bIsPesRecording = m_iInstanceId < 0;
        }
     else
        return;
     GetResume();
     // read an optional info file:
     std::string InfoFileName = StringUtils::Format("%s%s", m_strFileName.c_str(), m_bIsPesRecording ? INFOFILESUFFIX ".vdr" : INFOFILESUFFIX);
     CFile file;
     if (file.Open(InfoFileName))
     {
       if (!m_recordingInfo->Read(file))
           esyslog("ERROR: EPG data problem in file %s", InfoFileName.c_str());
        else if (!m_bIsPesRecording) {
           m_iPriority = m_recordingInfo->priority;
           m_iLifetime = m_recordingInfo->lifetime;
           m_dFramesPerSecond = m_recordingInfo->framesPerSecond;
           }
        }
     else if (errno == ENOENT)
        m_recordingInfo->ownEvent->SetTitle(m_strName);
     else
        LOG_ERROR_STR(InfoFileName.c_str());
     }
}

cRecording::~cRecording()
{
  delete m_recordingInfo;
}

char *cRecording::StripEpisodeName(char *s, bool Strip)
{
  char *t = s, *s1 = NULL, *s2 = NULL;
  while (*t) {
        if (*t == '/') {
           if (s1) {
              if (s2)
                 s1 = s2;
              s2 = t;
              }
           else
              s1 = t;
           }
        t++;
        }
  if (s1 && s2) {
     // To have folders sorted before plain recordings, the '/' s1 points to
     // is replaced by the character '1'. All other slashes will be replaced
     // by '0' in SortName() (see below), which will result in the desired
     // sequence:
     *s1 = '1';
     if (Strip) {
        s1++;
        memmove(s1, s2, t - s2 + 1);
        }
     }
  return s;
}

int cRecording::GetResume(void)
{
  if (m_iResume == RESUME_NOT_INITIALIZED) {
     cResumeFile ResumeFile(FileName(), m_bIsPesRecording);
     m_iResume = ResumeFile.Read();
     }
  return m_iResume;
}

std::string cRecording::FileName(void)
{
  if (m_strFileName.empty())
  {
    struct tm tm_r;
    struct tm *t = localtime_r(&m_start, &tm_r);
    const char* fmt = m_bIsPesRecording ? NAMEFORMATPES : NAMEFORMATTS;
    int ch = m_bIsPesRecording ? m_iPriority : m_iChannel;
    int ri = m_bIsPesRecording ? m_iLifetime : m_iInstanceId;
    std::string strName = LimitNameLengths(m_strName, DirectoryPathMax - strlen(VideoDirectory) - 1 - 42, DirectoryNameMax); // 42 = length of an actual recording directory name (generated with DATAFORMATTS) plus some reserve
    if (strName.compare(m_strName.c_str()))
      dsyslog("recording file name '%s' truncated to '%s'", m_strName.c_str(), strName.c_str());
    strName = ExchangeChars(strName, true);
    m_strFileName = StringUtils::Format(fmt, VideoDirectory, strName.c_str(), t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ch, ri);
  }

  return m_strFileName;
}

uint32_t cRecording::UID(void)
{
  /** stored so the id doesn't change when the filename changes */
  if (m_hash < 0)
    m_hash = (int64_t)CCRC32::CRC32(FileName());
  return (uint32_t)m_hash;
}

std::string cRecording::PrefixFileName(char Prefix)
{
  std::string p = PrefixVideoFileName(FileName(), Prefix);
  if (!p.empty())
  {
    m_strFileName = p;
    return m_strFileName;
  }
  return p;
}

int cRecording::HierarchyLevels(void) const
{
  const char *s = m_strName.c_str();
  int level = 0;
  while (*++s)
  {
    if (*s == FOLDERDELIMCHAR)
      level++;
  }
  return level;
}

bool cRecording::IsEdited(void) const
{
  size_t pos = m_strName.find(FOLDERDELIMCHAR);
  return pos != std::string::npos &&
      pos + 1 < m_strName.size() &&
      m_strName.at(pos + 1) == '%';
}

bool cRecording::IsOnVideoDirectoryFileSystem(void)
{
  if (m_iIsOnVideoDirectoryFileSystem < 0)
     m_iIsOnVideoDirectoryFileSystem = ::IsOnVideoDirectoryFileSystem(FileName());
  return m_iIsOnVideoDirectoryFileSystem;
}

void cRecording::ReadInfo(void)
{
  m_recordingInfo->Read();
  m_iPriority = m_recordingInfo->priority;
  m_iLifetime = m_recordingInfo->lifetime;
  m_dFramesPerSecond = m_recordingInfo->framesPerSecond;
}

bool cRecording::WriteInfo(void)
{
  std::string InfoFileName = StringUtils::Format("%s%s", m_strFileName.c_str(), m_bIsPesRecording ? INFOFILESUFFIX ".vdr" : INFOFILESUFFIX);
  CFile file;
  if (file.OpenForWrite(InfoFileName))
    m_recordingInfo->Write(file);
  else
    LOG_ERROR_STR(InfoFileName.c_str());
  return true;
}

void cRecording::SetStartTime(time_t Start)
{
  m_start = Start;
  m_strFileName.clear();
}

bool cRecording::Delete(void)
{
  bool result = true;
  std::string strNewName = FileName();
  std::string strExt = URLUtils::GetExtension(strNewName);
  if (!strExt.empty() && strcmp(strExt.c_str(), RECEXT_) == 0)
  {
    strNewName.erase(strNewName.size() - strExt.size());
    strNewName.append(DELEXT_);
    if (CFile::Exists(strNewName))
    {
      // the new name already exists, so let's remove that one first:
      isyslog("removing recording '%s'", strNewName.c_str());
      RemoveVideoFile(strNewName);
    }
    isyslog("deleting recording '%s'", FileName().c_str());
    if (CFile::Exists(FileName()))
    {
      result = RenameVideoFile(FileName(), strNewName);
      cRecordingUserCommand::Get().InvokeCommand(RUC_DELETERECORDING, strNewName);
    }
    else
    {
      isyslog("recording '%s' vanished", FileName().c_str());
      result = true; // well, we were going to delete it, anyway
    }
  }

  return result;
}

bool cRecording::Remove(void)
{
  // let's do a final safety check here:
  if (!StringUtils::EndsWith(FileName(), DELEXT_))
  {
    esyslog("attempt to remove recording %s", FileName().c_str());
    return false;
  }
  isyslog("removing recording %s", FileName().c_str());
  return RemoveVideoFile(FileName());
}

bool cRecording::Undelete(void)
{
  bool result = true;
  std::string strNewName = FileName();
  std::string strExt = URLUtils::GetExtension(strNewName);
  if (!strExt.empty() && strcmp(strExt.c_str(), DELEXT_) == 0)
  {
    strNewName.erase(strNewName.size() - strExt.size());
    strNewName.append(RECEXT_);

    if (CFile::Exists(strNewName))
    {
      // the new name already exists, so let's not remove that one:
      esyslog("ERROR: attempt to undelete '%s', while recording '%s' exists", FileName().c_str(), strNewName.c_str());
      result = false;
    }
    else
    {
      isyslog("undeleting recording '%s'", FileName().c_str());
      if (CFile::Exists(FileName()))
        result = RenameVideoFile(FileName(), strNewName);
      else
      {
        isyslog("deleted recording '%s' vanished", FileName().c_str());
        result = false;
      }
    }
  }

  return result;
}

void cRecording::ResetResume(void)
{
  m_iResume = RESUME_NOT_INITIALIZED;
}

int cRecording::NumFrames(void)
{
  if (m_iNumFrames < 0)
  {
    int nf = cIndexFile::GetLength(FileName(), IsPesRecording());
    if (time(NULL) - LastModifiedTime(cIndexFile::IndexFileName(FileName().c_str(), IsPesRecording())) < MININDEXAGE)
      return nf; // check again later for ongoing recordings
    m_iNumFrames = nf;
  }

  return m_iNumFrames;
}

int cRecording::LengthInSeconds(void)
{
  int nf = NumFrames();
  if (nf >= 0)
     return int(nf / FramesPerSecond());
  return -1;
}

int cRecording::FileSizeMB(void)
{
  if (m_iFileSizeMB < 0)
  {
    int fs = DirSizeMB(FileName());
    if (time(NULL) - LastModifiedTime(cIndexFile::IndexFileName(FileName().c_str(), IsPesRecording())) < MININDEXAGE)
      return fs; // check again later for ongoing recordings
    m_iFileSizeMB = fs;
  }
  return m_iFileSizeMB;
}

int cRecording::SecondsToFrames(int Seconds, double FramesPerSecond)
{
  return int(round(Seconds * FramesPerSecond));
}

int cRecording::ReadFrame(cUnbufferedFile *f, uchar *b, int Length, int Max)
{
  if (Length == -1)
     Length = Max; // this means we read up to EOF (see cIndex)
  else if (Length > Max) {
     esyslog("ERROR: frame larger than buffer (%d > %d)", Length, Max);
     Length = Max;
     }
  int r = f->Read(b, Length);
  if (r < 0)
     LOG_ERROR;
  return r;
}
