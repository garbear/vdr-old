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

#include "vdr/filesystem/Directory.h"
#include "vdr/utils/UTF8Utils.h"

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

char *cRecording::ExchangeChars(char *s, bool ToFileSystem)
{
  char *p = s;
  while (*p) {
        if (DirectoryEncoding) {
           // Some file systems can't handle all characters, so we
           // have to take extra efforts to encode/decode them:
           if (ToFileSystem) {
              switch (*p) {
                     // characters that can be mapped to other characters:
                     case ' ': *p = '_'; break;
                     case FOLDERDELIMCHAR: *p = '/'; break;
                     case '/': *p = FOLDERDELIMCHAR; break;
                     // characters that have to be encoded:
                     default:
                       if (NeedsConversion(p)) {
                          int l = p - s;
                          if (char *NewBuffer = (char *)realloc(s, strlen(s) + 10)) {
                             s = NewBuffer;
                             p = s + l;
                             char buf[4];
                             sprintf(buf, "#%02X", (unsigned char)*p);
                             memmove(p + 2, p, strlen(p) + 1);
                             strncpy(p, buf, 3);
                             p += 2;
                             }
                          else
                             esyslog("ERROR: out of memory");
                          }
                     }
              }
           else {
              switch (*p) {
                // mapped characters:
                case '_': *p = ' '; break;
                case FOLDERDELIMCHAR: *p = '/'; break;
                case '/': *p = FOLDERDELIMCHAR; break;
                // encoded characters:
                case '#': {
                     if (strlen(p) > 2 && isxdigit(*(p + 1)) && isxdigit(*(p + 2))) {
                        char buf[3];
                        sprintf(buf, "%c%c", *(p + 1), *(p + 2));
                        uchar c = uchar(strtol(buf, NULL, 16));
                        if (c) {
                           *p = c;
                           memmove(p + 1, p + 3, strlen(p) - 2);
                           }
                        }
                     }
                     break;
                // backwards compatibility:
                case '\x01': *p = '\''; break;
                case '\x02': *p = '/';  break;
                case '\x03': *p = ':';  break;
                default: ;
                }
              }
           }
        else {
           for (struct tCharExchange *ce = CharExchange; ce->a && ce->b; ce++) {
               if (*p == (ToFileSystem ? ce->a : ce->b)) {
                  *p = ToFileSystem ? ce->b : ce->a;
                  break;
                  }
               }
           }
        p++;
        }
  return s;
}

char *LimitNameLengths(char *s, int PathMax, int NameMax)
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
  for (char *p = s; *p; p++) {
      if (*p == FOLDERDELIMCHAR) {
         a[n] = -1; // FOLDERDELIMCHAR is a single character, neg. sign marks it
         NameTooLong |= NameLength > NameMax;
         NameLength = 0;
         PathLength += 1;
         }
      else if (NeedsConversion(p)) {
         a[n] = 3; // "#xx"
         NameLength += 3;
         PathLength += 3;
         }
      else {
         int8_t l = cUtf8Utils::Utf8CharLen(p);
         a[n] = l;
         NameLength += l;
         PathLength += l;
         while (l-- > 1) {
               a[++n] = 0;
               p++;
               }
         }
      n++;
      }
  NameTooLong |= NameLength > NameMax;
  // Limit names to NameMax:
  if (NameTooLong) {
     while (n > 0) {
           // Calculate the length of the current name:
           int NameLength = 0;
           int i = n;
           int b = i;
           while (i-- > 0 && a[i] >= 0) {
                 NameLength += a[i];
                 b = i;
                 }
           // Shorten the name if necessary:
           if (NameLength > NameMax) {
              int l = 0;
              i = n;
              while (i-- > 0 && a[i] >= 0) {
                    l += a[i];
                    if (NameLength - l <= NameMax) {
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
  while (PathLength > PathMax && n > 0) {
        // Calculate how much to cut off the current name:
        int i = n;
        int b = i;
        int l = 0;
        while (--i > 0 && a[i - 1] >= 0) {
              if (a[i] > 0) {
                 l += a[i];
                 b = i;
                 if (PathLength - l <= PathMax)
                    break;
                 }
              }
        // Shorten the name if necessary:
        if (l > 0) {
           memmove(s + b, s + n, Length - n + 1);
           Length -= n - b;
           PathLength -= l;
           }
        // Switch to the next name:
        n = i - 1;
        }
  return s;
}

cRecording::cRecording(TimerPtr Timer, const cEvent *Event)
{
  m_iResume = RESUME_NOT_INITIALIZED;
  m_strTitleBuffer = NULL;
  m_strSortBufferName = m_strSortBufferTime = NULL;
  m_strFileName = NULL;
  m_strName = NULL;
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
  const char *Title = Event ? Event->Title() : NULL;
  const char *Subtitle = Event ? Event->ShortText() : NULL;
  if (isempty(Title))
     Title = Timer->Channel()->Name().c_str();
  if (isempty(Subtitle))
     Subtitle = " ";
  size_t macroTitle   = Timer->File().find(TIMERMACRO_TITLE);
  size_t macroEpisode = Timer->File().find(TIMERMACRO_EPISODE);
  if (macroTitle != std::string::npos || macroEpisode != std::string::npos) {
     std::string strName = Timer->File();
     StringUtils::Replace(strName, TIMERMACRO_TITLE, Title);
     StringUtils::Replace(strName, TIMERMACRO_EPISODE, Subtitle);
     m_strName = strdup(strName.c_str());
     // avoid blanks at the end:
     int l = strlen(m_strName);
     while (l-- > 2) {
           if (m_strName[l] == ' ' && m_strName[l - 1] != FOLDERDELIMCHAR)
              m_strName[l] = 0;
           else
              break;
           }
     if (Timer->IsSingleEvent()) {
        Timer->SetFile(m_strName); // this was an instant recording, so let's set the actual data
        cTimers::Get().SetModified();
        }
     }
  else if (Timer->IsSingleEvent() || !g_setup.UseSubtitle)
     m_strName = strdup(Timer->File().c_str());
  else
     m_strName = strdup(cString::sprintf("%s~%s", Timer->File().c_str(), Subtitle));
  // substitute characters that would cause problems in file names:
  strreplace(m_strName, '\n', ' ');
  m_start = Timer->StartTime();
  m_iPriority = Timer->Priority();
  m_iLifetime = Timer->Lifetime();
  // handle info:
  m_recordingInfo = new cRecordingInfo(Timer->Channel().get(), Event);
  m_recordingInfo->SetAux(Timer->Aux());
  m_recordingInfo->priority = m_iPriority;
  m_recordingInfo->lifetime = m_iLifetime;
}

cRecording::cRecording(const char *FileName)
{
  m_iResume = RESUME_NOT_INITIALIZED;
  m_iFileSizeMB = -1; // unknown
  m_iChannel = -1;
  m_iInstanceId = -1;
  m_iPriority = MAXPRIORITY; // assume maximum in case there is no info file
  m_iLifetime = MAXLIFETIME;
  m_bIsPesRecording = false;
  m_iIsOnVideoDirectoryFileSystem = -1; // unknown
  m_dFramesPerSecond = DEFAULTFRAMESPERSECOND;
  m_iNumFrames = -1;
  m_deleted = 0;
  m_strTitleBuffer = NULL;
  m_strSortBufferName = m_strSortBufferTime = NULL;
  m_hash = -1;
  FileName = m_strFileName = strdup(FileName);
  if (*(m_strFileName + strlen(m_strFileName) - 1) == '/')
     *(m_strFileName + strlen(m_strFileName) - 1) = 0;
  if (strstr(FileName, VideoDirectory) == FileName)
     FileName += strlen(VideoDirectory) + 1;
  const char *p = strrchr(FileName, '/');

  m_strName = NULL;
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
        m_strName = MALLOC(char, p - FileName + 1);
        strncpy(m_strName, FileName, p - FileName);
        m_strName[p - FileName] = 0;
        m_strName = ExchangeChars(m_strName, false);
        m_bIsPesRecording = m_iInstanceId < 0;
        }
     else
        return;
     GetResume();
     // read an optional info file:
     std::string InfoFileName = *cString::sprintf("%s%s", m_strFileName, m_bIsPesRecording ? INFOFILESUFFIX ".vdr" : INFOFILESUFFIX);
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
#ifdef SUMMARYFALLBACK
     // fall back to the old 'summary.vdr' if there was no 'info.vdr':
     if (isempty(m_recordingInfo->Title())) {
        std::string SummaryFileName = *cString::sprintf("%s%s", m_strFileName, SUMMARYFILESUFFIX);
        CFile file;
        if (file.Open(SummaryFileName))
        {
           int line = 0;
           char *data[3] = { NULL };
           std::string strLine;
           while (file.ReadLine(strLine)) {
                 if (*strLine.c_str() || line > 1) {
                    if (data[line]) {
                       size_t len = strLine.size();
                       len += strlen(data[line]) + 1;
                       if (char *NewBuffer = (char *)realloc(data[line], len + 1)) {
                          data[line] = NewBuffer;
                          strcat(data[line], "\n");
                          strcat(data[line], strLine.c_str());
                          }
                       else
                          esyslog("ERROR: out of memory");
                       }
                    else
                       data[line] = strdup(strLine.c_str());
                    }
                 else
                    line++;
                 }
           if (!data[2]) {
              data[2] = data[1];
              data[1] = NULL;
              }
           else if (data[1] && data[2]) {
              // if line 1 is too long, it can't be the short text,
              // so assume the short text is missing and concatenate
              // line 1 and line 2 to be the long text:
              int len = strlen(data[1]);
              if (len > 80) {
                 if (char *NewBuffer = (char *)realloc(data[1], len + 1 + strlen(data[2]) + 1)) {
                    data[1] = NewBuffer;
                    strcat(data[1], "\n");
                    strcat(data[1], data[2]);
                    free(data[2]);
                    data[2] = data[1];
                    data[1] = NULL;
                    }
                 else
                    esyslog("ERROR: out of memory");
                 }
              }
           m_recordingInfo->SetData(data[0], data[1], data[2]);
           for (int i = 0; i < 3; i ++)
               free(data[i]);
           }
        else if (errno != ENOENT)
           LOG_ERROR_STR(SummaryFileName.c_str());
        }
#endif
     }
}

cRecording::~cRecording()
{
  free(m_strTitleBuffer);
  free(m_strSortBufferName);
  free(m_strSortBufferTime);
  free(m_strFileName);
  free(m_strName);
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

void cRecording::ClearSortName(void)
{
  DELETENULL(m_strSortBufferName);
  DELETENULL(m_strSortBufferTime);
}

int cRecording::GetResume(void)
{
  if (m_iResume == RESUME_NOT_INITIALIZED) {
     cResumeFile ResumeFile(FileName(), m_bIsPesRecording);
     m_iResume = ResumeFile.Read();
     }
  return m_iResume;
}

const char *cRecording::FileName(void)
{
  if (!m_strFileName) {
     struct tm tm_r;
     struct tm *t = localtime_r(&m_start, &tm_r);
     const char *fmt = m_bIsPesRecording ? NAMEFORMATPES : NAMEFORMATTS;
     int ch = m_bIsPesRecording ? m_iPriority : m_iChannel;
     int ri = m_bIsPesRecording ? m_iLifetime : m_iInstanceId;
     char *Name = LimitNameLengths(strdup(m_strName), DirectoryPathMax - strlen(VideoDirectory) - 1 - 42, DirectoryNameMax); // 42 = length of an actual recording directory name (generated with DATAFORMATTS) plus some reserve
     if (strcmp(Name, m_strName) != 0)
        dsyslog("recording file name '%s' truncated to '%s'", m_strName, Name);
     Name = ExchangeChars(Name, true);
     m_strFileName = strdup(cString::sprintf(fmt, VideoDirectory, Name, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ch, ri));
     free(Name);
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

const char *cRecording::Title(char Delimiter, bool NewIndicator, int Level)
{
  char New = NewIndicator && IsNew() ? '*' : ' ';
  free(m_strTitleBuffer);
  m_strTitleBuffer = NULL;
  if (Level < 0 || Level == HierarchyLevels()) {
     struct tm tm_r;
     struct tm *t = localtime_r(&m_start, &tm_r);
     char *s;
     if (Level > 0 && (s = strrchr(m_strName, FOLDERDELIMCHAR)) != NULL)
        s++;
     else
        s = m_strName;
     cString Length("");
     if (NewIndicator) {
        int Minutes = max(0, (LengthInSeconds() + 30) / 60);
        Length = cString::sprintf("%c%d:%02d",
                   Delimiter,
                   Minutes / 60,
                   Minutes % 60
                   );
        }
     m_strTitleBuffer = strdup(cString::sprintf("%02d.%02d.%02d%c%02d:%02d%s%c%c%s",
                            t->tm_mday,
                            t->tm_mon + 1,
                            t->tm_year % 100,
                            Delimiter,
                            t->tm_hour,
                            t->tm_min,
                            *Length,
                            New,
                            Delimiter,
                            s));
     // let's not display a trailing FOLDERDELIMCHAR:
     if (!NewIndicator)
        stripspace(m_strTitleBuffer);
     s = &m_strTitleBuffer[strlen(m_strTitleBuffer) - 1];
     if (*s == FOLDERDELIMCHAR)
        *s = 0;
     }
  else if (Level < HierarchyLevels()) {
     const char *s = m_strName;
     const char *p = s;
     while (*++s) {
           if (*s == FOLDERDELIMCHAR) {
              if (Level--)
                 p = s + 1;
              else
                 break;
              }
           }
     m_strTitleBuffer = MALLOC(char, s - p + 3);
     *m_strTitleBuffer = Delimiter;
     *(m_strTitleBuffer + 1) = Delimiter;
     strn0cpy(m_strTitleBuffer + 2, p, s - p + 1);
     }
  else
     return "";
  return m_strTitleBuffer;
}

const char *cRecording::PrefixFileName(char Prefix)
{
  cString p = PrefixVideoFileName(FileName(), Prefix);
  if (*p) {
     free(m_strFileName);
     m_strFileName = strdup(p);
     return m_strFileName;
     }
  return NULL;
}

int cRecording::HierarchyLevels(void) const
{
  const char *s = m_strName;
  int level = 0;
  while (*++s) {
        if (*s == FOLDERDELIMCHAR)
           level++;
        }
  return level;
}

bool cRecording::IsEdited(void) const
{
  const char *s = strrchr(m_strName, FOLDERDELIMCHAR);
  s = !s ? m_strName : s + 1;
  return *s == '%';
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
  std::string InfoFileName = *cString::sprintf("%s%s", m_strFileName, m_bIsPesRecording ? INFOFILESUFFIX ".vdr" : INFOFILESUFFIX);
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
  free(m_strFileName);
  m_strFileName = NULL;
}

bool cRecording::Delete(void)
{
  bool result = true;
  char *NewName = strdup(FileName());
  char *ext = strrchr(NewName, '.');
  if (ext && strcmp(ext, RECEXT) == 0) {
     strncpy(ext, DELEXT, strlen(ext));
     if (CFile::Exists(NewName)) {
        // the new name already exists, so let's remove that one first:
        isyslog("removing recording '%s'", NewName);
        RemoveVideoFile(NewName);
        }
     isyslog("deleting recording '%s'", FileName());
     if (CFile::Exists(FileName())) {
        result = RenameVideoFile(FileName(), NewName);
        cRecordingUserCommand::Get().InvokeCommand(RUC_DELETERECORDING, NewName);
        }
     else {
        isyslog("recording '%s' vanished", FileName());
        result = true; // well, we were going to delete it, anyway
        }
     }
  free(NewName);
  return result;
}

bool cRecording::Remove(void)
{
  // let's do a final safety check here:
  if (!endswith(FileName(), DELEXT)) {
     esyslog("attempt to remove recording %s", FileName());
     return false;
     }
  isyslog("removing recording %s", FileName());
  return RemoveVideoFile(FileName());
}

bool cRecording::Undelete(void)
{
  bool result = true;
  char *NewName = strdup(FileName());
  char *ext = strrchr(NewName, '.');
  if (ext && strcmp(ext, DELEXT) == 0) {
     strncpy(ext, RECEXT, strlen(ext));
     if (CFile::Exists(NewName)) {
        // the new name already exists, so let's not remove that one:
        esyslog("ERROR: attempt to undelete '%s', while recording '%s' exists", FileName(), NewName);
        result = false;
        }
     else {
        isyslog("undeleting recording '%s'", FileName());
        if (CFile::Exists(FileName()))
           result = RenameVideoFile(FileName(), NewName);
        else {
           isyslog("deleted recording '%s' vanished", FileName());
           result = false;
           }
        }
     }
  free(NewName);
  return result;
}

void cRecording::ResetResume(void)
{
  m_iResume = RESUME_NOT_INITIALIZED;
}

int cRecording::NumFrames(void)
{
  if (m_iNumFrames < 0) {
     int nf = cIndexFile::GetLength(FileName(), IsPesRecording());
     if (time(NULL) - LastModifiedTime(cIndexFile::IndexFileName(FileName(), IsPesRecording())) < MININDEXAGE)
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
  if (m_iFileSizeMB < 0) {
     int fs = DirSizeMB(FileName());
     if (time(NULL) - LastModifiedTime(cIndexFile::IndexFileName(FileName(), IsPesRecording())) < MININDEXAGE)
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
