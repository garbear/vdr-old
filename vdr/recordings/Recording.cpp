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
#include "FileName.h"
#include "Recordings.h"
#include "recordings/IndexFile.h"

#include "vdr/filesystem/Directory.h"
#include "vdr/utils/UTF8Utils.h"

#include <algorithm>

using namespace PLATFORM;
using namespace VDR;

#define SUMMARYFALLBACK

/* This was the original code, which works fine in a Linux only environment.
   Unfortunately, because of Windows and its brain dead file system, we have
   to use a more complicated approach, in order to allow users who have enabled
   the --vfat command line option to see their recordings even if they forget to
   enable --vfat when restarting VDR... Gee, do I hate Windows.
   (kls 2002-07-27)
#define DATAFORMAT   "%4d-%02d-%02d.%02d:%02d.%02d.%02d" RECEXT
#define NAMEFORMAT   "%s/%s/" DATAFORMAT
*/
#define DATAFORMATPES   "%4d-%02d-%02d.%02d%*c%02d.%02d.%02d" RECEXT
#define NAMEFORMATPES   "%s/%s/" "%4d-%02d-%02d.%02d.%02d.%02d.%02d" RECEXT
#define DATAFORMATTS    "%4d-%02d-%02d.%02d.%02d.%d-%d" RECEXT
#define NAMEFORMATTS    "%s/%s/" DATAFORMATTS

#ifdef SUMMARYFALLBACK
#define SUMMARYFILESUFFIX "/summary.vdr"
#endif
#define MARKSFILESUFFIX   "/marks"

#define SORTMODEFILE      ".sort"

#define MINDISKSPACE 1024 // MB

#define REMOVECHECKDELTA   60 // seconds between checks for removing deleted files
#define DELETEDLIFETIME   300 // seconds after which a deleted recording will be actually removed
#define DISKCHECKDELTA    100 // seconds between checks for free disk space
#define REMOVELATENCY      10 // seconds to wait until next check after removing a file
#define MARKSUPDATEDELTA   10 // seconds between checks for updating editing marks

int DirectoryPathMax = PATH_MAX - 1;
int DirectoryNameMax = NAME_MAX;
bool DirectoryEncoding = false;
int InstanceId = 0;

cRecordings DeletedRecordings(true);

// --- cRemoveDeletedRecordingsThread ----------------------------------------

class cRemoveDeletedRecordingsThread : public CThread {
protected:
  virtual void* Process(void);
public:
  cRemoveDeletedRecordingsThread(void);
  };

cRemoveDeletedRecordingsThread::cRemoveDeletedRecordingsThread(void)
{
}

void* cRemoveDeletedRecordingsThread::Process(void)
{
  // Make sure only one instance of VDR does this:
  cLockFile LockFile(VideoDirectory);
  if (LockFile.Lock()) {
     bool deleted = false;
     CThreadLock DeletedRecordingsLock(&DeletedRecordings);
     for (cRecording *r = DeletedRecordings.First(); r; ) {
         if (PLATFORM::cIoThrottle::Engaged())
            return NULL;
         if (r->Deleted() && time(NULL) - r->Deleted() > DELETEDLIFETIME) {
            cRecording *next = DeletedRecordings.Next(r);
            r->Remove();
            DeletedRecordings.Del(r);
            r = next;
            deleted = true;
            continue;
            }
         r = DeletedRecordings.Next(r);
         }
     if (deleted) {
        const char *IgnoreFiles[] = { SORTMODEFILE, NULL };
        RemoveEmptyVideoDirectories(IgnoreFiles);
        }
     }
  return NULL;
}

static cRemoveDeletedRecordingsThread RemoveDeletedRecordingsThread;

// ---

void RemoveDeletedRecordings(void)
{
  static time_t LastRemoveCheck = 0;
  if (time(NULL) - LastRemoveCheck > REMOVECHECKDELTA) {
     if (!RemoveDeletedRecordingsThread.IsRunning()) {
        CThreadLock DeletedRecordingsLock(&DeletedRecordings);
        for (cRecording *r = DeletedRecordings.First(); r; r = DeletedRecordings.Next(r)) {
            if (r->Deleted() && time(NULL) - r->Deleted() > DELETEDLIFETIME) {
               RemoveDeletedRecordingsThread.CreateThread();
               break;
               }
            }
        }
     LastRemoveCheck = time(NULL);
     }
}

void AssertFreeDiskSpace(int Priority, bool Force)
{
  static CMutex Mutex;
  CLockObject lock(Mutex);
  // With every call to this function we try to actually remove
  // a file, or mark a file for removal ("delete" it), so that
  // it will get removed during the next call.
  static time_t LastFreeDiskCheck = 0;
  int Factor = (Priority == -1) ? 10 : 1;
  if (Force || time(NULL) - LastFreeDiskCheck > DISKCHECKDELTA / Factor) {
     if (!VideoFileSpaceAvailable(MINDISKSPACE)) {
        // Make sure only one instance of VDR does this:
        cLockFile LockFile(VideoDirectory);
        if (!LockFile.Lock())
           return;
        // Remove the oldest file that has been "deleted":
        isyslog("low disk space while recording, trying to remove a deleted recording...");
        CThreadLock DeletedRecordingsLock(&DeletedRecordings);
        if (DeletedRecordings.Count()) {
           cRecording *r = DeletedRecordings.First();
           cRecording *r0 = NULL;
           while (r) {
                 if (r->IsOnVideoDirectoryFileSystem()) { // only remove recordings that will actually increase the free video disk space
                    if (!r0 || r->Start() < r0->Start())
                       r0 = r;
                    }
                 r = DeletedRecordings.Next(r);
                 }
           if (r0) {
              if (r0->Remove())
                 LastFreeDiskCheck += REMOVELATENCY / Factor;
              DeletedRecordings.Del(r0);
              return;
              }
           }
        else {
           // DeletedRecordings was empty, so to be absolutely sure there are no
           // deleted recordings we need to double check:
           DeletedRecordings.Update(true);
           if (DeletedRecordings.Count())
              return; // the next call will actually remove it
           }
        // No "deleted" files to remove, so let's see if we can delete a recording:
        if (Priority > 0) {
           isyslog("...no deleted recording found, trying to delete an old recording...");
           CThreadLock RecordingsLock(&Recordings);
           if (Recordings.Count()) {
              cRecording *r = Recordings.First();
              cRecording *r0 = NULL;
              while (r) {
                    if (r->IsOnVideoDirectoryFileSystem()) { // only delete recordings that will actually increase the free video disk space
                       if (!r->IsEdited() && r->Lifetime() < MAXLIFETIME) { // edited recordings and recordings with MAXLIFETIME live forever
                          if ((r->Lifetime() == 0 && Priority > r->Priority()) || // the recording has no guaranteed lifetime and the new recording has higher priority
                              (r->Lifetime() > 0 && (time(NULL) - r->Start()) / SECSINDAY >= r->Lifetime())) { // the recording's guaranteed lifetime has expired
                             if (r0) {
                                if (r->Priority() < r0->Priority() || (r->Priority() == r0->Priority() && r->Start() < r0->Start()))
                                   r0 = r; // in any case we delete the one with the lowest priority (or the older one in case of equal priorities)
                                }
                             else
                                r0 = r;
                             }
                          }
                       }
                    r = Recordings.Next(r);
                    }
              if (r0 && r0->Delete()) {
                 Recordings.Del(r0);
                 return;
                 }
              }
           // Unable to free disk space, but there's nothing we can do about that...
           isyslog("...no old recording found, giving up");
           }
        else
           isyslog("...no deleted recording found, priority %d too low to trigger deleting an old recording", Priority);
        esyslog(tr("Low disk space!"));
        }
     LastFreeDiskCheck = time(NULL);
     }
}

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

char *ExchangeChars(char *s, bool ToFileSystem)
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

cRecording::cRecording(cTimer *Timer, const cEvent *Event)
{
  resume = RESUME_NOT_INITIALIZED;
  titleBuffer = NULL;
  sortBufferName = sortBufferTime = NULL;
  fileName = NULL;
  name = NULL;
  fileSizeMB = -1; // unknown
  channel = Timer->Channel()->Number();
  instanceId = InstanceId;
  isPesRecording = false;
  isOnVideoDirectoryFileSystem = -1; // unknown
  framesPerSecond = DEFAULTFRAMESPERSECOND;
  numFrames = -1;
  deleted = 0;
  m_hash = -1;
  // set up the actual name:
  const char *Title = Event ? Event->Title() : NULL;
  const char *Subtitle = Event ? Event->ShortText() : NULL;
  if (isempty(Title))
     Title = Timer->Channel()->Name().c_str();
  if (isempty(Subtitle))
     Subtitle = " ";
  const char *macroTITLE   = strstr(Timer->File(), TIMERMACRO_TITLE);
  const char *macroEPISODE = strstr(Timer->File(), TIMERMACRO_EPISODE);
  if (macroTITLE || macroEPISODE) {
     name = strdup(Timer->File());
     name = strreplace(name, TIMERMACRO_TITLE, Title);
     name = strreplace(name, TIMERMACRO_EPISODE, Subtitle);
     // avoid blanks at the end:
     int l = strlen(name);
     while (l-- > 2) {
           if (name[l] == ' ' && name[l - 1] != FOLDERDELIMCHAR)
              name[l] = 0;
           else
              break;
           }
     if (Timer->IsSingleEvent()) {
        Timer->SetFile(name); // this was an instant recording, so let's set the actual data
        Timers.SetModified();
        }
     }
  else if (Timer->IsSingleEvent() || !g_setup.UseSubtitle)
     name = strdup(Timer->File());
  else
     name = strdup(cString::sprintf("%s~%s", Timer->File(), Subtitle));
  // substitute characters that would cause problems in file names:
  strreplace(name, '\n', ' ');
  start = Timer->StartTime();
  priority = Timer->Priority();
  lifetime = Timer->Lifetime();
  // handle info:
  info = new cRecordingInfo(Timer->Channel().get(), Event);
  info->SetAux(Timer->Aux());
  info->priority = priority;
  info->lifetime = lifetime;
}

cRecording::cRecording(const char *FileName)
{
  resume = RESUME_NOT_INITIALIZED;
  fileSizeMB = -1; // unknown
  channel = -1;
  instanceId = -1;
  priority = MAXPRIORITY; // assume maximum in case there is no info file
  lifetime = MAXLIFETIME;
  isPesRecording = false;
  isOnVideoDirectoryFileSystem = -1; // unknown
  framesPerSecond = DEFAULTFRAMESPERSECOND;
  numFrames = -1;
  deleted = 0;
  titleBuffer = NULL;
  sortBufferName = sortBufferTime = NULL;
  m_hash = -1;
  FileName = fileName = strdup(FileName);
  if (*(fileName + strlen(fileName) - 1) == '/')
     *(fileName + strlen(fileName) - 1) = 0;
  if (strstr(FileName, VideoDirectory) == FileName)
     FileName += strlen(VideoDirectory) + 1;
  const char *p = strrchr(FileName, '/');

  name = NULL;
  info = new cRecordingInfo(fileName);
  if (p) {
     time_t now = time(NULL);
     struct tm tm_r;
     struct tm t = *localtime_r(&now, &tm_r); // this initializes the time zone in 't'
     t.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
     if (7 == sscanf(p + 1, DATAFORMATTS, &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &channel, &instanceId)
      || 7 == sscanf(p + 1, DATAFORMATPES, &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &priority, &lifetime)) {
        t.tm_year -= 1900;
        t.tm_mon--;
        t.tm_sec = 0;
        start = mktime(&t);
        name = MALLOC(char, p - FileName + 1);
        strncpy(name, FileName, p - FileName);
        name[p - FileName] = 0;
        name = ExchangeChars(name, false);
        isPesRecording = instanceId < 0;
        }
     else
        return;
     GetResume();
     // read an optional info file:
     std::string InfoFileName = *cString::sprintf("%s%s", fileName, isPesRecording ? INFOFILESUFFIX ".vdr" : INFOFILESUFFIX);
     CFile file;
     if (file.Open(InfoFileName))
     {
       if (!info->Read(file))
           esyslog("ERROR: EPG data problem in file %s", InfoFileName.c_str());
        else if (!isPesRecording) {
           priority = info->priority;
           lifetime = info->lifetime;
           framesPerSecond = info->framesPerSecond;
           }
        }
     else if (errno == ENOENT)
        info->ownEvent->SetTitle(name);
     else
        LOG_ERROR_STR(InfoFileName.c_str());
#ifdef SUMMARYFALLBACK
     // fall back to the old 'summary.vdr' if there was no 'info.vdr':
     if (isempty(info->Title())) {
        std::string SummaryFileName = *cString::sprintf("%s%s", fileName, SUMMARYFILESUFFIX);
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
           info->SetData(data[0], data[1], data[2]);
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
  free(titleBuffer);
  free(sortBufferName);
  free(sortBufferTime);
  free(fileName);
  free(name);
  delete info;
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

char *cRecording::SortName(void) const
{
  char **sb = (RecordingsSortMode == rsmName) ? &sortBufferName : &sortBufferTime;
  if (!*sb) {
     char *s = strdup(FileName() + strlen(VideoDirectory));
     if (RecordingsSortMode != rsmName || g_setup.AlwaysSortFoldersFirst)
        s = StripEpisodeName(s, RecordingsSortMode != rsmName);
     strreplace(s, '/', '0'); // some locales ignore '/' when sorting
     int l = strxfrm(NULL, s, 0) + 1;
     *sb = MALLOC(char, l);
     strxfrm(*sb, s, l);
     free(s);
     }
  return *sb;
}

void cRecording::ClearSortName(void)
{
  DELETENULL(sortBufferName);
  DELETENULL(sortBufferTime);
}

int cRecording::GetResume(void) const
{
  if (resume == RESUME_NOT_INITIALIZED) {
     cResumeFile ResumeFile(FileName(), isPesRecording);
     resume = ResumeFile.Read();
     }
  return resume;
}

int cRecording::Compare(const cListObject &ListObject) const
{
  cRecording *r = (cRecording *)&ListObject;
  return strcasecmp(SortName(), r->SortName());
}

const char *cRecording::FileName(void) const
{
  if (!fileName) {
     struct tm tm_r;
     struct tm *t = localtime_r(&start, &tm_r);
     const char *fmt = isPesRecording ? NAMEFORMATPES : NAMEFORMATTS;
     int ch = isPesRecording ? priority : channel;
     int ri = isPesRecording ? lifetime : instanceId;
     char *Name = LimitNameLengths(strdup(name), DirectoryPathMax - strlen(VideoDirectory) - 1 - 42, DirectoryNameMax); // 42 = length of an actual recording directory name (generated with DATAFORMATTS) plus some reserve
     if (strcmp(Name, name) != 0)
        dsyslog("recording file name '%s' truncated to '%s'", name, Name);
     Name = ExchangeChars(Name, true);
     fileName = strdup(cString::sprintf(fmt, VideoDirectory, Name, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ch, ri));
     free(Name);
     }
  return fileName;
}

uint32_t cRecording::UID(void)
{
  /** stored so the id doesn't change when the filename changes */
  if (m_hash < 0)
    m_hash = (int64_t)CCRC32::CRC32(FileName());
  return (uint32_t)m_hash;
}

const char *cRecording::Title(char Delimiter, bool NewIndicator, int Level) const
{
  char New = NewIndicator && IsNew() ? '*' : ' ';
  free(titleBuffer);
  titleBuffer = NULL;
  if (Level < 0 || Level == HierarchyLevels()) {
     struct tm tm_r;
     struct tm *t = localtime_r(&start, &tm_r);
     char *s;
     if (Level > 0 && (s = strrchr(name, FOLDERDELIMCHAR)) != NULL)
        s++;
     else
        s = name;
     cString Length("");
     if (NewIndicator) {
        int Minutes = max(0, (LengthInSeconds() + 30) / 60);
        Length = cString::sprintf("%c%d:%02d",
                   Delimiter,
                   Minutes / 60,
                   Minutes % 60
                   );
        }
     titleBuffer = strdup(cString::sprintf("%02d.%02d.%02d%c%02d:%02d%s%c%c%s",
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
        stripspace(titleBuffer);
     s = &titleBuffer[strlen(titleBuffer) - 1];
     if (*s == FOLDERDELIMCHAR)
        *s = 0;
     }
  else if (Level < HierarchyLevels()) {
     const char *s = name;
     const char *p = s;
     while (*++s) {
           if (*s == FOLDERDELIMCHAR) {
              if (Level--)
                 p = s + 1;
              else
                 break;
              }
           }
     titleBuffer = MALLOC(char, s - p + 3);
     *titleBuffer = Delimiter;
     *(titleBuffer + 1) = Delimiter;
     strn0cpy(titleBuffer + 2, p, s - p + 1);
     }
  else
     return "";
  return titleBuffer;
}

const char *cRecording::PrefixFileName(char Prefix)
{
  cString p = PrefixVideoFileName(FileName(), Prefix);
  if (*p) {
     free(fileName);
     fileName = strdup(p);
     return fileName;
     }
  return NULL;
}

int cRecording::HierarchyLevels(void) const
{
  const char *s = name;
  int level = 0;
  while (*++s) {
        if (*s == FOLDERDELIMCHAR)
           level++;
        }
  return level;
}

bool cRecording::IsEdited(void) const
{
  const char *s = strrchr(name, FOLDERDELIMCHAR);
  s = !s ? name : s + 1;
  return *s == '%';
}

bool cRecording::IsOnVideoDirectoryFileSystem(void) const
{
  if (isOnVideoDirectoryFileSystem < 0)
     isOnVideoDirectoryFileSystem = ::IsOnVideoDirectoryFileSystem(FileName());
  return isOnVideoDirectoryFileSystem;
}

void cRecording::ReadInfo(void)
{
  info->Read();
  priority = info->priority;
  lifetime = info->lifetime;
  framesPerSecond = info->framesPerSecond;
}

bool cRecording::WriteInfo(void)
{
  std::string InfoFileName = *cString::sprintf("%s%s", fileName, isPesRecording ? INFOFILESUFFIX ".vdr" : INFOFILESUFFIX);
  CFile file;
  if (file.OpenForWrite(InfoFileName))
    info->Write(file);
  else
    LOG_ERROR_STR(InfoFileName.c_str());
  return true;
}

void cRecording::SetStartTime(time_t Start)
{
  start = Start;
  free(fileName);
  fileName = NULL;
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
        cRecordingUserCommand::InvokeCommand(RUC_DELETERECORDING, NewName);
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

void cRecording::ResetResume(void) const
{
  resume = RESUME_NOT_INITIALIZED;
}

int cRecording::NumFrames(void) const
{
  if (numFrames < 0) {
     int nf = cIndexFile::GetLength(FileName(), IsPesRecording());
     if (time(NULL) - LastModifiedTime(cIndexFile::IndexFileName(FileName(), IsPesRecording())) < MININDEXAGE)
        return nf; // check again later for ongoing recordings
     numFrames = nf;
     }
  return numFrames;
}

int cRecording::LengthInSeconds(void) const
{
  int nf = NumFrames();
  if (nf >= 0)
     return int(nf / FramesPerSecond());
  return -1;
}

int cRecording::FileSizeMB(void) const
{
  if (fileSizeMB < 0) {
     int fs = DirSizeMB(FileName());
     if (time(NULL) - LastModifiedTime(cIndexFile::IndexFileName(FileName(), IsPesRecording())) < MININDEXAGE)
        return fs; // check again later for ongoing recordings
     fileSizeMB = fs;
     }
  return fileSizeMB;
}

// --- cMark -----------------------------------------------------------------

double MarkFramesPerSecond = DEFAULTFRAMESPERSECOND;
CMutex MutexMarkFramesPerSecond;

cMark::cMark(int Position, const char *Comment, double FramesPerSecond)
{
  position = Position;
  comment = Comment;
  framesPerSecond = FramesPerSecond;
}

cMark::~cMark()
{
}

cString cMark::ToText(void)
{
  return cString::sprintf("%s%s%s\n", *IndexToHMSF(position, true, framesPerSecond), Comment() ? " " : "", Comment() ? Comment() : "");
}

bool cMark::Parse(const char *s)
{
  comment = NULL;
  framesPerSecond = MarkFramesPerSecond;
  position = HMSFToIndex(s, framesPerSecond);
  const char *p = strchr(s, ' ');
  if (p) {
     p = skipspace(p);
     if (*p)
        comment = strdup(p);
     }
  return true;
}

bool cMark::Save(FILE *f)
{
  return fprintf(f, "%s", *ToText()) > 0;
}

// --- cMarks ----------------------------------------------------------------

bool cMarks::Load(const char *RecordingFileName, double FramesPerSecond, bool IsPesRecording)
{
  recordingFileName = RecordingFileName;
  fileName = AddDirectory(RecordingFileName, IsPesRecording ? MARKSFILESUFFIX ".vdr" : MARKSFILESUFFIX);
  framesPerSecond = FramesPerSecond;
  isPesRecording = IsPesRecording;
  nextUpdate = 0;
  lastFileTime = -1; // the first call to Load() must take place!
  lastChange = 0;
  return Update();
}

bool cMarks::Update(void)
{
  time_t t = time(NULL);
  if (t > nextUpdate) {
     time_t LastModified = LastModifiedTime(fileName);
     if (LastModified != lastFileTime) // change detected, or first run
        lastChange = LastModified > 0 ? LastModified : t;
     int d = t - lastChange;
     if (d < 60)
        d = 1; // check frequently if the file has just been modified
     else if (d < 3600)
        d = 10; // older files are checked less frequently
     else
        d /= 360; // phase out checking for very old files
     nextUpdate = t + d;
     if (LastModified != lastFileTime) { // change detected, or first run
        lastFileTime = LastModified;
        if (lastFileTime == t)
           lastFileTime--; // make sure we don't miss updates in the remaining second
        CLockObject lock(MutexMarkFramesPerSecond);
        MarkFramesPerSecond = framesPerSecond;
        if (cConfig<cMark>::Load(fileName)) {
           Align();
           Sort();
           return true;
           }
        }
     }
  return false;
}

bool cMarks::Save(void)
{
  if (cConfig<cMark>::Save()) {
     lastFileTime = LastModifiedTime(fileName);
     return true;
     }
  return false;
}

void cMarks::Align(void)
{
  cIndexFile IndexFile(recordingFileName, false, isPesRecording);
  for (cMark *m = First(); m; m = Next(m)) {
      int p = IndexFile.GetClosestIFrame(m->Position());
      if (int d = m->Position() - p) {
         isyslog("aligned editing mark %s to %s (off by %d frame%s)", *IndexToHMSF(m->Position(), true, framesPerSecond), *IndexToHMSF(p, true, framesPerSecond), d, abs(d) > 1 ? "s" : "");
         m->SetPosition(p);
         }
      }
}

void cMarks::Sort(void)
{
  for (cMark *m1 = First(); m1; m1 = Next(m1)) {
      for (cMark *m2 = Next(m1); m2; m2 = Next(m2)) {
          if (m2->Position() < m1->Position()) {
             std::swap(m1->position, m2->position);
             std::swap(m1->comment, m2->comment);
             }
          }
      }
}

void cMarks::Add(int Position)
{
  cConfig<cMark>::Add(new cMark(Position, NULL, framesPerSecond));
  Sort();
}

cMark *cMarks::Get(int Position)
{
  for (cMark *mi = First(); mi; mi = Next(mi)) {
      if (mi->Position() == Position)
         return mi;
      }
  return NULL;
}

cMark *cMarks::GetPrev(int Position)
{
  for (cMark *mi = Last(); mi; mi = Prev(mi)) {
      if (mi->Position() < Position)
         return mi;
      }
  return NULL;
}

cMark *cMarks::GetNext(int Position)
{
  for (cMark *mi = First(); mi; mi = Next(mi)) {
      if (mi->Position() > Position)
         return mi;
      }
  return NULL;
}

cMark *cMarks::GetNextBegin(cMark *EndMark)
{
  cMark *BeginMark = EndMark ? Next(EndMark) : First();
  if (BeginMark) {
     while (cMark *NextMark = Next(BeginMark)) {
           if (BeginMark->Position() == NextMark->Position()) { // skip Begin/End at the same position
              if (!(BeginMark = Next(NextMark)))
                 break;
              }
           else
              break;
           }
     }
  return BeginMark;
}

cMark *cMarks::GetNextEnd(cMark *BeginMark)
{
  if (!BeginMark)
     return NULL;
  cMark *EndMark = Next(BeginMark);
  if (EndMark) {
     while (cMark *NextMark = Next(EndMark)) {
           if (EndMark->Position() == NextMark->Position()) { // skip End/Begin at the same position
              if (!(EndMark = Next(NextMark)))
                 break;
              }
           else
              break;
           }
     }
  return EndMark;
}

int cMarks::GetNumSequences(void)
{
  int NumSequences = 0;
  if (cMark *BeginMark = GetNextBegin()) {
     while (cMark *EndMark = GetNextEnd(BeginMark)) {
           NumSequences++;
           BeginMark = GetNextBegin(EndMark);
           }
     if (BeginMark) {
        NumSequences++; // the last sequence had no actual "end" mark
        if (NumSequences == 1 && BeginMark->Position() == 0)
           NumSequences = 0; // there is only one actual "begin" mark at offset zero, and no actual "end" mark
        }
     }
  return NumSequences;
}

// --- cRecordingUserCommand -------------------------------------------------

const char *cRecordingUserCommand::command = NULL;

void cRecordingUserCommand::InvokeCommand(const char *State, const char *RecordingFileName, const char *SourceFileName)
{
  if (command) {
    cString cmd;
    if (SourceFileName)
       cmd = cString::sprintf("%s %s \"%s\" \"%s\"", command, State, *strescape(RecordingFileName, "\\\"$"), *strescape(SourceFileName, "\\\"$"));
    else
       cmd = cString::sprintf("%s %s \"%s\"", command, State, *strescape(RecordingFileName, "\\\"$"));
    isyslog("executing '%s'", *cmd);
    SystemExec(cmd);
  }
}

// --- Index stuff -----------------------------------------------------------

cString IndexToHMSF(int Index, bool WithFrame, double FramesPerSecond)
{
  const char *Sign = "";
  if (Index < 0) {
     Index = -Index;
     Sign = "-";
     }
  double Seconds;
  int f = int(modf((Index + 0.5) / FramesPerSecond, &Seconds) * FramesPerSecond + 1);
  int s = int(Seconds);
  int m = s / 60 % 60;
  int h = s / 3600;
  s %= 60;
  return cString::sprintf(WithFrame ? "%s%d:%02d:%02d.%02d" : "%s%d:%02d:%02d", Sign, h, m, s, f);
}

int HMSFToIndex(const char *HMSF, double FramesPerSecond)
{
  int h, m, s, f = 1;
  int n = sscanf(HMSF, "%d:%d:%d.%d", &h, &m, &s, &f);
  if (n == 1)
     return h - 1; // plain frame number
  if (n >= 3)
     return int(round((h * 3600 + m * 60 + s) * FramesPerSecond)) + f - 1;
  return 0;
}

int SecondsToFrames(int Seconds, double FramesPerSecond)
{
  return int(round(Seconds * FramesPerSecond));
}

// --- ReadFrame -------------------------------------------------------------

int ReadFrame(cUnbufferedFile *f, uchar *b, int Length, int Max)
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

// --- Recordings Sort Mode --------------------------------------------------

eRecordingsSortMode RecordingsSortMode = rsmName;

bool HasRecordingsSortMode(const char *Directory)
{
  return CDirectory::Exists(*AddDirectory(Directory, SORTMODEFILE));
}

void GetRecordingsSortMode(const char *Directory)
{
  CFile file;
  if (file.Open(*AddDirectory(Directory, SORTMODEFILE)))
  {
    char buf[8];
    if (file.Read(buf, sizeof(buf)))
      RecordingsSortMode = eRecordingsSortMode(constrain(atoi(buf), 0, int(rsmTime)));
    file.Close();
  }
}

void SetRecordingsSortMode(const char *Directory, eRecordingsSortMode SortMode)
{
  CFile file;
  if (file.OpenForWrite(*AddDirectory(Directory, SORTMODEFILE)))
  {
    char buf[8];
    snprintf(buf, 8, "%d\n", SortMode);
    file.Write(buf, 8);
    file.Close();
  }
}

void IncRecordingsSortMode(const char *Directory)
{
  GetRecordingsSortMode(Directory);
  RecordingsSortMode = eRecordingsSortMode(int(RecordingsSortMode) + 1);
  if (RecordingsSortMode > rsmTime)
     RecordingsSortMode = eRecordingsSortMode(0);
  SetRecordingsSortMode(Directory, RecordingsSortMode);
}
