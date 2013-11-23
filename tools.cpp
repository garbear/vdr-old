/*
 * tools.c: Various tools
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: tools.c 2.29 2012/12/08 11:16:30 kls Exp $
 */

#include "tools.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <string.h>
#include "i18n.h"
#include "thread.h"

#include "vdr/filesystem/Directory.h"
#include "vdr/utils/UTF8Utils.h"

#include <algorithm>

using namespace std;

int SysLogLevel = 3;

#define MAXSYSLOGBUF 256

#ifdef ANDROID
#include "android_getline.h"

#define POSIX_FADV_RANDOM   1
#define POSIX_FADV_DONTNEED 1
#define POSIX_FADV_WILLNEED 1
#define posix_fadvise(a,b,c,d) (-1)
#endif

void syslog_with_tid(int priority, const char *format, ...)
{
  va_list ap;
  char fmt[MAXSYSLOGBUF];
  snprintf(fmt, sizeof(fmt), "[%d] %s", cThread::ThreadId(), format);
  va_start(ap, format);
  vsyslog(priority, fmt, ap);
  va_end(ap);
}

int BCD2INT(int x)
{
  return ((1000000 * BCDCHARTOINT((x >> 24) & 0xFF)) +
            (10000 * BCDCHARTOINT((x >> 16) & 0xFF)) +
              (100 * BCDCHARTOINT((x >>  8) & 0xFF)) +
                     BCDCHARTOINT( x        & 0xFF));
}

ssize_t safe_read(int filedes, void *buffer, size_t size)
{
  for (;;) {
      ssize_t p = read(filedes, buffer, size);
      if (p < 0 && errno == EINTR) {
         dsyslog("EINTR while reading from file handle %d - retrying", filedes);
         continue;
         }
      return p;
      }
}

ssize_t safe_write(int filedes, const void *buffer, size_t size)
{
  ssize_t p = 0;
  ssize_t written = size;
  const unsigned char *ptr = (const unsigned char *)buffer;
  while (size > 0) {
        p = write(filedes, ptr, size);
        if (p < 0) {
           if (errno == EINTR) {
              dsyslog("EINTR while writing to file handle %d - retrying", filedes);
              continue;
              }
           break;
           }
        ptr  += p;
        size -= p;
        }
  return p < 0 ? p : written;
}

void writechar(int filedes, char c)
{
  safe_write(filedes, &c, sizeof(c));
}

int WriteAllOrNothing(int fd, const uchar *Data, int Length, int TimeoutMs, int RetryMs)
{
  int written = 0;
  while (Length > 0) {
        int w = write(fd, Data + written, Length);
        if (w > 0) {
           Length -= w;
           written += w;
           }
        else if (written > 0 && !FATALERRNO) {
           // we've started writing, so we must finish it!
           cTimeMs t;
           cPoller Poller(fd, true);
           Poller.Poll(RetryMs);
           if (TimeoutMs > 0 && (TimeoutMs -= t.Elapsed()) <= 0)
              break;
           }
        else
           // nothing written yet (or fatal error), so we can just return the error code:
           return w;
        }
  return written;
}

char *strcpyrealloc(char *dest, const char *src)
{
  if (src) {
     int l = ::max(dest ? strlen(dest) : 0, strlen(src)) + 1; // don't let the block get smaller!
     dest = (char *)realloc(dest, l);
     if (dest)
        strcpy(dest, src);
     else
        esyslog("ERROR: out of memory");
     }
  else {
     free(dest);
     dest = NULL;
     }
  return dest;
}

char *strn0cpy(char *dest, const char *src, size_t n)
{
  char *s = dest;
  for ( ; --n && (*dest = *src) != 0; dest++, src++) ;
  *dest = 0;
  return s;
}

char *strreplace(char *s, char c1, char c2)
{
  if (s) {
     char *p = s;
     while (*p) {
           if (*p == c1)
              *p = c2;
           p++;
           }
     }
  return s;
}

char *strreplace(char *s, const char *s1, const char *s2)
{
  char *p = strstr(s, s1);
  if (p) {
     int of = p - s;
     int l  = strlen(s);
     int l1 = strlen(s1);
     int l2 = strlen(s2);
     if (l2 > l1) {
        if (char *NewBuffer = (char *)realloc(s, l + l2 - l1 + 1))
           s = NewBuffer;
        else {
           esyslog("ERROR: out of memory");
           return s;
           }
        }
     char *sof = s + of;
     if (l2 != l1)
        memmove(sof + l2, sof + l1, l - of - l1 + 1);
     strncpy(sof, s2, l2);
     }
  return s;
}

char *stripspace(char *s)
{
  if (s && *s) {
     for (char *p = s + strlen(s) - 1; p >= s; p--) {
         if (!isspace(*p))
            break;
         *p = 0;
         }
     }
  return s;
}

char *compactspace(char *s)
{
  if (s && *s) {
     char *t = stripspace(skipspace(s));
     char *p = t;
     while (p && *p) {
           char *q = skipspace(p);
           if (q - p > 1)
              memmove(p + 1, q, strlen(q) + 1);
           p++;
           }
     if (t != s)
        memmove(s, t, strlen(t) + 1);
     }
  return s;
}

cString strescape(const char *s, const char *chars)
{
  char *buffer;
  const char *p = s;
  char *t = NULL;
  while (*p) {
        if (strchr(chars, *p)) {
           if (!t) {
              buffer = MALLOC(char, 2 * strlen(s) + 1);
              t = buffer + (p - s);
              s = strcpy(buffer, s);
              }
           *t++ = '\\';
           }
        if (t)
           *t++ = *p;
        p++;
        }
  if (t)
     *t = 0;
  return cString(s, t != NULL);
}

bool startswith(const char *s, const char *p)
{
  while (*p) {
        if (*p++ != *s++)
           return false;
        }
  return true;
}

bool endswith(const char *s, const char *p)
{
  const char *se = s + strlen(s) - 1;
  const char *pe = p + strlen(p) - 1;
  while (pe >= p) {
        if (*pe-- != *se-- || (se < s && pe >= p))
           return false;
        }
  return true;
}

bool isempty(const char *s)
{
  return !(s && *skipspace(s));
}

int numdigits(int n)
{
  int res = 1;
  while (n >= 10) {
        n /= 10;
        res++;
        }
  return res;
}

bool is_number(const char *s)
{
  if (!s || !*s)
     return false;
  do {
     if (!isdigit(*s))
        return false;
     } while (*++s);
  return true;
}

int64_t StrToNum(const char *s)
{
  char *t = NULL;
  int64_t n = strtoll(s, &t, 10);
  if (t) {
     switch (*t) {
       case 'T': n *= 1024;
       case 'G': n *= 1024;
       case 'M': n *= 1024;
       case 'K': n *= 1024;
       }
     }
  return n;
}

bool StrInArray(const char *a[], const char *s)
{
  if (a) {
     while (*a) {
           if (strcmp(*a, s) == 0)
              return true;
           a++;
           }
     }
  return false;
}

cString AddDirectory(const char *DirName, const char *FileName)
{
  return cString::sprintf("%s/%s", DirName && *DirName ? DirName : ".", FileName);
}

#define DECIMAL_POINT_C '.'

double atod(const char *s)
{
  static lconv *loc = localeconv();
  if (*loc->decimal_point != DECIMAL_POINT_C) {
     char buf[strlen(s) + 1];
     char *p = buf;
     while (*s) {
           if (*s == DECIMAL_POINT_C)
              *p = *loc->decimal_point;
           else
              *p = *s;
           p++;
           s++;
           }
     *p = 0;
     return atof(buf);
     }
  else
     return atof(s);
}

cString dtoa(double d, const char *Format)
{
  static lconv *loc = localeconv();
  char buf[16];
  snprintf(buf, sizeof(buf), Format, d);
  if (*loc->decimal_point != DECIMAL_POINT_C)
     strreplace(buf, *loc->decimal_point, DECIMAL_POINT_C);
  return buf;
}

cString itoa(int n)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", n);
  return buf;
}

bool MakeDirs(const char *FileName, bool IsDirectory)
{
  bool result = true;
  char *s = strdup(FileName);
  char *p = s;
  if (*p == '/')
     p++;
  while ((p = strchr(p, '/')) != NULL || IsDirectory) {
        if (p)
           *p = 0;
        struct stat fs;
        if (stat(s, &fs) != 0 || !S_ISDIR(fs.st_mode)) {
           dsyslog("creating directory %s", s);
           if (mkdir(s, ACCESSPERMS) == -1) {
              LOG_ERROR_STR(s);
              result = false;
              break;
              }
           }
        if (p)
           *p++ = '/';
        else
           break;
        }
  free(s);
  return result;
}

bool RemoveFileOrDir(const char *FileName, bool FollowSymlinks)
{
  struct stat st;
  if (stat(FileName, &st) == 0) {
     if (S_ISDIR(st.st_mode)) {
        cReadDir d(FileName);
        if (d.Ok()) {
           struct dirent *e;
           while ((e = d.Next()) != NULL) {
                 cString buffer = AddDirectory(FileName, e->d_name);
                 if (FollowSymlinks) {
                    struct stat st2;
                    if (lstat(buffer, &st2) == 0) {
                       if (S_ISLNK(st2.st_mode)) {
                          int size = st2.st_size + 1;
                          char *l = MALLOC(char, size);
                          int n = readlink(buffer, l, size - 1);
                          if (n < 0) {
                             if (errno != EINVAL)
                                LOG_ERROR_STR(*buffer);
                             }
                          else {
                             l[n] = 0;
                             dsyslog("removing %s", l);
                             if (remove(l) < 0)
                                LOG_ERROR_STR(l);
                             }
                          free(l);
                          }
                       }
                    else if (errno != ENOENT) {
                       LOG_ERROR_STR(FileName);
                       return false;
                       }
                    }
                 dsyslog("removing %s", *buffer);
                 if (remove(buffer) < 0)
                    LOG_ERROR_STR(*buffer);
                 }
           }
        else {
           LOG_ERROR_STR(FileName);
           return false;
           }
        }
     dsyslog("removing %s", FileName);
     if (remove(FileName) < 0) {
        LOG_ERROR_STR(FileName);
        return false;
        }
     }
  else if (errno != ENOENT) {
     LOG_ERROR_STR(FileName);
     return false;
     }
  return true;
}

bool RemoveEmptyDirectories(const char *DirName, bool RemoveThis, const char *IgnoreFiles[])
{
  bool HasIgnoredFiles = false;
  cReadDir d(DirName);
  if (d.Ok()) {
     bool empty = true;
     struct dirent *e;
     while ((e = d.Next()) != NULL) {
           if (strcmp(e->d_name, "lost+found")) {
              cString buffer = AddDirectory(DirName, e->d_name);
              struct stat st;
              if (stat(buffer, &st) == 0) {
                 if (S_ISDIR(st.st_mode)) {
                    if (!RemoveEmptyDirectories(buffer, true, IgnoreFiles))
                       empty = false;
                    }
                 else if (RemoveThis && IgnoreFiles && StrInArray(IgnoreFiles, e->d_name))
                    HasIgnoredFiles = true;
                 else
                    empty = false;
                 }
              else {
                 LOG_ERROR_STR(*buffer);
                 empty = false;
                 }
              }
           }
     if (RemoveThis && empty) {
        if (HasIgnoredFiles) {
           while (*IgnoreFiles) {
                 cString buffer = AddDirectory(DirName, *IgnoreFiles);
                 if (access(buffer, F_OK) == 0) {
                    dsyslog("removing %s", *buffer);
                    if (remove(buffer) < 0) {
                       LOG_ERROR_STR(*buffer);
                       return false;
                       }
                    }
                 IgnoreFiles++;
                 }
           }
        dsyslog("removing %s", DirName);
        if (remove(DirName) < 0) {
           LOG_ERROR_STR(DirName);
           return false;
           }
        }
     return empty;
     }
  else
     LOG_ERROR_STR(DirName);
  return false;
}

int DirSizeMB(const char *DirName)
{
  cReadDir d(DirName);
  if (d.Ok()) {
     int size = 0;
     struct dirent *e;
     while (size >= 0 && (e = d.Next()) != NULL) {
           cString buffer = AddDirectory(DirName, e->d_name);
           struct stat st;
           if (stat(buffer, &st) == 0) {
              if (S_ISDIR(st.st_mode)) {
                 int n = DirSizeMB(buffer);
                 if (n >= 0)
                    size += n;
                 else
                    size = -1;
                 }
              else
                 size += st.st_size / MEGABYTE(1);
              }
           else {
              LOG_ERROR_STR(*buffer);
              size = -1;
              }
           }
     return size;
     }
  else
     LOG_ERROR_STR(DirName);
  return -1;
}

char *ReadLink(const char *FileName)
{
  if (!FileName)
     return NULL;
#ifdef ANDROID
  char *TargetName = realpath(FileName, NULL);
#else
  char *TargetName = canonicalize_file_name(FileName);
#endif
  if (!TargetName) {
     if (errno == ENOENT) // file doesn't exist
        TargetName = strdup(FileName);
     else // some other error occurred
        LOG_ERROR_STR(FileName);
     }
  return TargetName;
}

bool SpinUpDisk(const char *FileName)
{
  for (int n = 0; n < 10; n++) {
      cString buf;
      if (cDirectory::CanWrite(FileName))
         buf = cString::sprintf("%s/vdr-%06d", *FileName ? FileName : ".", n);
      else
         buf = cString::sprintf("%s.vdr-%06d", FileName, n);
      if (access(buf, F_OK) != 0) { // the file does not exist
         timeval tp1, tp2;
         gettimeofday(&tp1, NULL);
         int f = open(buf, O_WRONLY | O_CREAT, DEFFILEMODE);
         // O_SYNC doesn't work on all file systems
         if (f >= 0) {
            if (fdatasync(f) < 0)
               LOG_ERROR_STR(*buf);
            close(f);
            remove(buf);
            gettimeofday(&tp2, NULL);
            double seconds = (((long long)tp2.tv_sec * 1000000 + tp2.tv_usec) - ((long long)tp1.tv_sec * 1000000 + tp1.tv_usec)) / 1000000.0;
            if (seconds > 0.5)
               dsyslog("SpinUpDisk took %.2f seconds", seconds);
            return true;
            }
         else
            LOG_ERROR_STR(*buf);
         }
      }
  esyslog("ERROR: SpinUpDisk failed");
  return false;
}

void TouchFile(const char *FileName)
{
  if (utime(FileName, NULL) == -1 && errno != ENOENT)
     LOG_ERROR_STR(FileName);
}

time_t LastModifiedTime(const char *FileName)
{
  struct stat fs;
  if (stat(FileName, &fs) == 0)
     return fs.st_mtime;
  return 0;
}

off_t FileSize(const char *FileName)
{
  struct stat fs;
  if (stat(FileName, &fs) == 0)
     return fs.st_size;
  return -1;
}

// --- cTimeMs ---------------------------------------------------------------

cTimeMs::cTimeMs(int Ms)
{
  if (Ms >= 0)
     Set(Ms);
  else
     begin = 0;
}

uint64_t cTimeMs::Now(void)
{
#if _POSIX_TIMERS > 0 && defined(_POSIX_MONOTONIC_CLOCK)
#define MIN_RESOLUTION 5 // ms
  static bool initialized = false;
  static bool monotonic = false;
  struct timespec tp;
  if (!initialized) {
     // check if monotonic timer is available and provides enough accurate resolution:
     if (clock_getres(CLOCK_MONOTONIC, &tp) == 0) {
        long Resolution = tp.tv_nsec;
        // require a minimum resolution:
        if (tp.tv_sec == 0 && tp.tv_nsec <= MIN_RESOLUTION * 1000000) {
           if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
              dsyslog("cTimeMs: using monotonic clock (resolution is %ld ns)", Resolution);
              monotonic = true;
              }
           else
              esyslog("cTimeMs: clock_gettime(CLOCK_MONOTONIC) failed");
           }
        else
           dsyslog("cTimeMs: not using monotonic clock - resolution is too bad (%ld s %ld ns)", tp.tv_sec, tp.tv_nsec);
        }
     else
        esyslog("cTimeMs: clock_getres(CLOCK_MONOTONIC) failed");
     initialized = true;
     }
  if (monotonic) {
     if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
        return (uint64_t(tp.tv_sec)) * 1000 + tp.tv_nsec / 1000000;
     esyslog("cTimeMs: clock_gettime(CLOCK_MONOTONIC) failed");
     monotonic = false;
     // fall back to gettimeofday()
     }
#else
#  warning Posix monotonic clock not available
#endif
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0)
     return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  return 0;
}

void cTimeMs::Set(int Ms)
{
  begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void)
{
  return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void)
{
  return Now() - begin;
}

// --- cString ---------------------------------------------------------------

cString::cString(const char *S, bool TakePointer)
{
  s = TakePointer ? (char *)S : S ? strdup(S) : NULL;
}

cString::cString(const cString &String)
{
  s = String.s ? strdup(String.s) : NULL;
}

cString::~cString()
{
  free(s);
}

cString &cString::operator=(const cString &String)
{
  if (this == &String)
     return *this;
  free(s);
  s = String.s ? strdup(String.s) : NULL;
  return *this;
}

cString &cString::operator=(const char *String)
{
  if (s == String)
    return *this;
  free(s);
  s = String ? strdup(String) : NULL;
  return *this;
}

cString &cString::Truncate(int Index)
{
  int l = strlen(s);
  if (Index < 0)
     Index = l + Index;
  if (Index >= 0 && Index < l)
     s[Index] = 0;
  return *this;
}

cString cString::sprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *buffer;
  if (!fmt || vasprintf(&buffer, fmt, ap) < 0) {
     esyslog("error in vasprintf('%s', ...)", fmt);
     buffer = strdup("???");
     }
  va_end(ap);
  return cString(buffer, true);
}

cString cString::vsprintf(const char *fmt, va_list &ap)
{
  char *buffer;
  if (!fmt || vasprintf(&buffer, fmt, ap) < 0) {
     esyslog("error in vasprintf('%s', ...)", fmt);
     buffer = strdup("???");
     }
  return cString(buffer, true);
}

// --- cBitStream ------------------------------------------------------------

int cBitStream::GetBit(void)
{
  if (index >= length)
     return 1;
  int r = (data[index >> 3] >> (7 - (index & 7))) & 1;
  ++index;
  return r;
}

uint32_t cBitStream::GetBits(int n)
{
  uint32_t r = 0;
  while (n--)
        r |= GetBit() << n;
  return r;
}

void cBitStream::ByteAlign(void)
{
  int n = index % 8;
  if (n > 0)
     SkipBits(8 - n);
}

void cBitStream::WordAlign(void)
{
  int n = index % 16;
  if (n > 0)
     SkipBits(16 - n);
}

bool cBitStream::SetLength(int Length)
{
  if (Length > length)
     return false;
  length = Length;
  return true;
}

// --- cReadLine -------------------------------------------------------------

cReadLine::cReadLine(void)
{
  size = 0;
  buffer = NULL;
}

cReadLine::~cReadLine()
{
  free(buffer);
}

char *cReadLine::Read(FILE *f)
{
  int n = getline(&buffer, &size, f);
  if (n > 0) {
     n--;
     if (buffer[n] == '\n') {
        buffer[n] = 0;
        if (n > 0) {
           n--;
           if (buffer[n] == '\r')
              buffer[n] = 0;
           }
        }
     return buffer;
     }
  return NULL;
}

// --- cPoller ---------------------------------------------------------------

cPoller::cPoller(int FileHandle, bool Out)
{
  numFileHandles = 0;
  Add(FileHandle, Out);
}

bool cPoller::Add(int FileHandle, bool Out)
{
  if (FileHandle >= 0) {
     for (int i = 0; i < numFileHandles; i++) {
         if (pfd[i].fd == FileHandle && pfd[i].events == (Out ? POLLOUT : POLLIN))
            return true;
         }
     if (numFileHandles < MaxPollFiles) {
        pfd[numFileHandles].fd = FileHandle;
        pfd[numFileHandles].events = Out ? POLLOUT : POLLIN;
        pfd[numFileHandles].revents = 0;
        numFileHandles++;
        return true;
        }
     esyslog("ERROR: too many file handles in cPoller");
     }
  return false;
}

bool cPoller::Poll(int TimeoutMs)
{
  if (numFileHandles) {
     if (poll(pfd, numFileHandles, TimeoutMs) != 0)
        return true; // returns true even in case of an error, to let the caller
                     // access the file and thus see the error code
     }
  return false;
}

bool GetSubDirectories(const string &strDirectory, vector<string> &vecFileNames)
{
  vecFileNames.clear();
  if (!strDirectory.empty())
  {
    cReadDir d(strDirectory.c_str());
    struct dirent *e;
    if (d.Ok())
    {
      while ((e = d.Next()) != NULL)
      {
        struct stat64 ds;
        if (stat64(AddDirectory(strDirectory.c_str(), e->d_name), &ds) == 0)
        {
          if (!S_ISDIR(ds.st_mode))
            continue;
        }
        vecFileNames.push_back(e->d_name);
      }
      std::sort(vecFileNames.begin(), vecFileNames.end());
      return true;
    }
  }
  return false;
}

// --- cVDRFile -----------------------------------------------------------------

bool cVDRFile::files[FD_SETSIZE] = { false };
int cVDRFile::maxFiles = 0;

cVDRFile::cVDRFile(void)
{
  f = -1;
}

cVDRFile::~cVDRFile()
{
  Close();
}

bool cVDRFile::Open(const char *FileName, int Flags, mode_t Mode)
{
  if (!IsOpen())
     return Open(open(FileName, Flags, Mode));
  esyslog("ERROR: attempt to re-open %s", FileName);
  return false;
}

bool cVDRFile::Open(int FileDes)
{
  if (FileDes >= 0) {
     if (!IsOpen()) {
        f = FileDes;
        if (f >= 0) {
           if (f < FD_SETSIZE) {
              if (f >= maxFiles)
                 maxFiles = f + 1;
              if (!files[f])
                 files[f] = true;
              else
                 esyslog("ERROR: file descriptor %d already in files[]", f);
              return true;
              }
           else
              esyslog("ERROR: file descriptor %d is larger than FD_SETSIZE (%d)", f, FD_SETSIZE);
           }
        }
     else
        esyslog("ERROR: attempt to re-open file descriptor %d", FileDes);
     }
  return false;
}

void cVDRFile::Close(void)
{
  if (f >= 0) {
     close(f);
     files[f] = false;
     f = -1;
     }
}

bool cVDRFile::Ready(bool Wait)
{
  return f >= 0 && AnyFileReady(f, Wait ? 1000 : 0);
}

bool cVDRFile::AnyFileReady(int FileDes, int TimeoutMs)
{
  fd_set set;
  FD_ZERO(&set);
  for (int i = 0; i < maxFiles; i++) {
      if (files[i])
         FD_SET(i, &set);
      }
  if (0 <= FileDes && FileDes < FD_SETSIZE && !files[FileDes])
     FD_SET(FileDes, &set); // in case we come in with an arbitrary descriptor
  if (TimeoutMs == 0)
     TimeoutMs = 10; // load gets too heavy with 0
  struct timeval timeout;
  timeout.tv_sec  = TimeoutMs / 1000;
  timeout.tv_usec = (TimeoutMs % 1000) * 1000;
  return select(FD_SETSIZE, &set, NULL, NULL, &timeout) > 0 && (FileDes < 0 || FD_ISSET(FileDes, &set));
}

bool cVDRFile::FileReady(int FileDes, int TimeoutMs)
{
  fd_set set;
  struct timeval timeout;
  FD_ZERO(&set);
  FD_SET(FileDes, &set);
  if (TimeoutMs >= 0) {
     if (TimeoutMs < 100)
        TimeoutMs = 100;
     timeout.tv_sec  = TimeoutMs / 1000;
     timeout.tv_usec = (TimeoutMs % 1000) * 1000;
     }
  return select(FD_SETSIZE, &set, NULL, NULL, (TimeoutMs >= 0) ? &timeout : NULL) > 0 && FD_ISSET(FileDes, &set);
}

bool cVDRFile::FileReadyForWriting(int FileDes, int TimeoutMs)
{
  fd_set set;
  struct timeval timeout;
  FD_ZERO(&set);
  FD_SET(FileDes, &set);
  if (TimeoutMs < 100)
     TimeoutMs = 100;
  timeout.tv_sec  = 0;
  timeout.tv_usec = TimeoutMs * 1000;
  return select(FD_SETSIZE, NULL, &set, NULL, &timeout) > 0 && FD_ISSET(FileDes, &set);
}

// --- cSafeFile -------------------------------------------------------------

cSafeFile::cSafeFile(const char *FileName)
{
  f = NULL;
  fileName = ReadLink(FileName);
  tempName = fileName ? MALLOC(char, strlen(fileName) + 5) : NULL;
  if (tempName)
     strcat(strcpy(tempName, fileName), ".$$$");
}

cSafeFile::~cSafeFile()
{
  if (f)
     fclose(f);
  unlink(tempName);
  free(fileName);
  free(tempName);
}

bool cSafeFile::Open(void)
{
  if (!f && fileName && tempName) {
     f = fopen(tempName, "w");
     if (!f)
        LOG_ERROR_STR(tempName);
     }
  return f != NULL;
}

bool cSafeFile::Close(void)
{
  bool result = true;
  if (f) {
     if (ferror(f) != 0) {
        LOG_ERROR_STR(tempName);
        result = false;
        }
     fflush(f);
     fsync(fileno(f));
     if (fclose(f) < 0) {
        LOG_ERROR_STR(tempName);
        result = false;
        }
     f = NULL;
     if (result && rename(tempName, fileName) < 0) {
        LOG_ERROR_STR(fileName);
        result = false;
        }
     }
  else
     result = false;
  return result;
}

// --- cUnbufferedFile -------------------------------------------------------

#define USE_FADVISE

#define WRITE_BUFFER KILOBYTE(800)

cUnbufferedFile::cUnbufferedFile(void)
{
  fd = -1;
}

cUnbufferedFile::~cUnbufferedFile()
{
  Close();
}

int cUnbufferedFile::Open(const char *FileName, int Flags, mode_t Mode)
{
  Close();
  fd = open(FileName, Flags, Mode);
  curpos = 0;
#ifdef USE_FADVISE
  begin = lastpos = ahead = 0;
  cachedstart = 0;
  cachedend = 0;
  readahead = KILOBYTE(128);
  written = 0;
  totwritten = 0;
  if (fd >= 0)
     posix_fadvise(fd, 0, 0, POSIX_FADV_RANDOM); // we could use POSIX_FADV_SEQUENTIAL, but we do our own readahead, disabling the kernel one.
#endif
  return fd;
}

int cUnbufferedFile::Close(void)
{
  if (fd >= 0) {
#ifdef USE_FADVISE
     if (totwritten)    // if we wrote anything make sure the data has hit the disk before
        fdatasync(fd);  // calling fadvise, as this is our last chance to un-cache it.
     posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
#endif
     int OldFd = fd;
     fd = -1;
     return close(OldFd);
     }
  errno = EBADF;
  return -1;
}

// When replaying and going e.g. FF->PLAY the position jumps back 2..8M
// hence we do not want to drop recently accessed data at once.
// We try to handle the common cases such as PLAY->FF->PLAY, small
// jumps, moving editing marks etc.

#define FADVGRAN   KILOBYTE(4) // AKA fadvise-chunk-size; PAGE_SIZE or getpagesize(2) would also work.
#define READCHUNK  MEGABYTE(8)

void cUnbufferedFile::SetReadAhead(size_t ra)
{
  readahead = ra;
}

int cUnbufferedFile::FadviseDrop(off_t Offset, off_t Len)
{
  // rounding up the window to make sure that not PAGE_SIZE-aligned data gets freed.
  return posix_fadvise(fd, Offset - (FADVGRAN - 1), Len + (FADVGRAN - 1) * 2, POSIX_FADV_DONTNEED);
}

off_t cUnbufferedFile::Seek(off_t Offset, int Whence)
{
  if (Whence == SEEK_SET && Offset == curpos)
     return curpos;
  curpos = lseek(fd, Offset, Whence);
  return curpos;
}

ssize_t cUnbufferedFile::Read(void *Data, size_t Size)
{
  if (fd >= 0) {
#ifdef USE_FADVISE
     off_t jumped = curpos-lastpos; // nonzero means we're not at the last offset
     if ((cachedstart < cachedend) && (curpos < cachedstart || curpos > cachedend)) {
        // current position is outside the cached window -- invalidate it.
        FadviseDrop(cachedstart, cachedend-cachedstart);
        cachedstart = curpos;
        cachedend = curpos;
        }
     cachedstart = ::min(cachedstart, curpos);
#endif
     ssize_t bytesRead = safe_read(fd, Data, Size);
     if (bytesRead > 0) {
        curpos += bytesRead;
#ifdef USE_FADVISE
        cachedend = ::max(cachedend, curpos);

        // Read ahead:
        // no jump? (allow small forward jump still inside readahead window).
        if (jumped >= 0 && jumped <= (off_t)readahead) {
           // Trigger the readahead IO, but only if we've used at least
           // 1/2 of the previously requested area. This avoids calling
           // fadvise() after every read() call.
           if (ahead - curpos < (off_t)(readahead / 2)) {
              posix_fadvise(fd, curpos, readahead, POSIX_FADV_WILLNEED);
              ahead = curpos + readahead;
              cachedend = ::max(cachedend, ahead);
              }
           if (readahead < Size * 32) { // automagically tune readahead size.
              readahead = Size * 32;
              }
           }
        else
           ahead = curpos; // jumped -> we really don't want any readahead, otherwise e.g. fast-rewind gets in trouble.
#endif
        }
#ifdef USE_FADVISE
     if (cachedstart < cachedend) {
        if (curpos - cachedstart > READCHUNK * 2) {
           // current position has moved forward enough, shrink tail window.
           FadviseDrop(cachedstart, curpos - READCHUNK - cachedstart);
           cachedstart = curpos - READCHUNK;
           }
        else if (cachedend > ahead && cachedend - curpos > READCHUNK * 2) {
           // current position has moved back enough, shrink head window.
           FadviseDrop(curpos + READCHUNK, cachedend - (curpos + READCHUNK));
           cachedend = curpos + READCHUNK;
           }
        }
     lastpos = curpos;
#endif
     return bytesRead;
     }
  return -1;
}

ssize_t cUnbufferedFile::Write(const void *Data, size_t Size)
{
  if (fd >=0) {
     ssize_t bytesWritten = safe_write(fd, Data, Size);
#ifdef USE_FADVISE
     if (bytesWritten > 0) {
        begin = ::min(begin, curpos);
        curpos += bytesWritten;
        written += bytesWritten;
        lastpos = ::max(lastpos, curpos);
        if (written > WRITE_BUFFER) {
           if (lastpos > begin) {
              // Now do three things:
              // 1) Start writeback of begin..lastpos range
              // 2) Drop the already written range (by the previous fadvise call)
              // 3) Handle nonpagealigned data.
              //    This is why we double the WRITE_BUFFER; the first time around the
              //    last (partial) page might be skipped, writeback will start only after
              //    second call; the third call will still include this page and finally
              //    drop it from cache.
              off_t headdrop = ::min(begin, off_t(WRITE_BUFFER * 2));
              posix_fadvise(fd, begin - headdrop, lastpos - begin + headdrop, POSIX_FADV_DONTNEED);
              }
           begin = lastpos = curpos;
           totwritten += written;
           written = 0;
           // The above fadvise() works when writing slowly (recording), but could
           // leave cached data around when writing at a high rate, e.g. when cutting,
           // because by the time we try to flush the cached pages (above) the data
           // can still be dirty - we are faster than the disk I/O.
           // So we do another round of flushing, just like above, but at larger
           // intervals -- this should catch any pages that couldn't be released
           // earlier.
           if (totwritten > MEGABYTE(32)) {
              // It seems in some setups, fadvise() does not trigger any I/O and
              // a fdatasync() call would be required do all the work (reiserfs with some
              // kind of write gathering enabled), but the syncs cause (io) load..
              // Uncomment the next line if you think you need them.
              //fdatasync(fd);
              off_t headdrop = ::min(off_t(curpos - totwritten), off_t(totwritten * 2));
              posix_fadvise(fd, curpos - totwritten - headdrop, totwritten + headdrop, POSIX_FADV_DONTNEED);
              totwritten = 0;
              }
           }
        }
#endif
     return bytesWritten;
     }
  return -1;
}

cUnbufferedFile *cUnbufferedFile::Create(const char *FileName, int Flags, mode_t Mode)
{
  cUnbufferedFile *File = new cUnbufferedFile;
  if (File->Open(FileName, Flags, Mode) < 0) {
     delete File;
     File = NULL;
     }
  return File;
}

// --- cLockFile -------------------------------------------------------------

#define LOCKFILENAME      ".lock-vdr"
#define LOCKFILESTALETIME 600 // seconds before considering a lock file "stale"

cLockFile::cLockFile(const char *Directory)
{
  fileName = NULL;
  f = -1;
  if (cDirectory::CanWrite(Directory))
     fileName = strdup(AddDirectory(Directory, LOCKFILENAME));
}

cLockFile::~cLockFile()
{
  Unlock();
  free(fileName);
}

bool cLockFile::Lock(int WaitSeconds)
{
  if (f < 0 && fileName) {
     time_t Timeout = time(NULL) + WaitSeconds;
     do {
        f = open(fileName, O_WRONLY | O_CREAT | O_EXCL, DEFFILEMODE);
        if (f < 0) {
           if (errno == EEXIST) {
              struct stat fs;
              if (stat(fileName, &fs) == 0) {
                 if (abs(time(NULL) - fs.st_mtime) > LOCKFILESTALETIME) {
                    esyslog("ERROR: removing stale lock file '%s'", fileName);
                    if (remove(fileName) < 0) {
                       LOG_ERROR_STR(fileName);
                       break;
                       }
                    continue;
                    }
                 }
              else if (errno != ENOENT) {
                 LOG_ERROR_STR(fileName);
                 break;
                 }
              }
           else {
              LOG_ERROR_STR(fileName);
              break;
              }
           if (WaitSeconds)
              cCondWait::SleepMs(1000);
           }
        } while (f < 0 && time(NULL) < Timeout);
     }
  return f >= 0;
}

void cLockFile::Unlock(void)
{
  if (f >= 0) {
     close(f);
     remove(fileName);
     f = -1;
     }
}

// --- cListObject -----------------------------------------------------------

cListObject::cListObject(void)
{
  prev = next = NULL;
}

cListObject::~cListObject()
{
}

void cListObject::Append(cListObject *Object)
{
  next = Object;
  Object->prev = this;
}

void cListObject::Insert(cListObject *Object)
{
  prev = Object;
  Object->next = this;
}

void cListObject::Unlink(void)
{
  if (next)
     next->prev = prev;
  if (prev)
     prev->next = next;
  next = prev = NULL;
}

int cListObject::Index(void) const
{
  cListObject *p = prev;
  int i = 0;

  while (p) {
        i++;
        p = p->prev;
        }
  return i;
}

// --- cListBase -------------------------------------------------------------

cListBase::cListBase(void)
{
  objects = lastObject = NULL;
  count = 0;
}

cListBase::~cListBase()
{
  Clear();
}

void cListBase::Add(cListObject *Object, cListObject *After)
{
  if (After && After != lastObject) {
     After->Next()->Insert(Object);
     After->Append(Object);
     }
  else {
     if (lastObject)
        lastObject->Append(Object);
     else
        objects = Object;
     lastObject = Object;
     }
  count++;
}

void cListBase::Ins(cListObject *Object, cListObject *Before)
{
  if (Before && Before != objects) {
     Before->Prev()->Append(Object);
     Before->Insert(Object);
     }
  else {
     if (objects)
        objects->Insert(Object);
     else
        lastObject = Object;
     objects = Object;
     }
  count++;
}

void cListBase::Del(cListObject *Object, bool DeleteObject)
{
  if (Object == objects)
     objects = Object->Next();
  if (Object == lastObject)
     lastObject = Object->Prev();
  Object->Unlink();
  if (DeleteObject)
     delete Object;
  count--;
}

void cListBase::Move(int From, int To)
{
  Move(Get(From), Get(To));
}

void cListBase::Move(cListObject *From, cListObject *To)
{
  if (From && To && From != To) {
     if (From->Index() < To->Index())
        To = To->Next();
     if (From == objects)
        objects = From->Next();
     if (From == lastObject)
        lastObject = From->Prev();
     From->Unlink();
     if (To) {
        if (To->Prev())
           To->Prev()->Append(From);
        From->Append(To);
        }
     else {
        lastObject->Append(From);
        lastObject = From;
        }
     if (!From->Prev())
        objects = From;
     }
}

void cListBase::Clear(void)
{
  while (objects) {
        cListObject *object = objects->Next();
        delete objects;
        objects = object;
        }
  objects = lastObject = NULL;
  count = 0;
}

cListObject *cListBase::Get(int Index) const
{
  if (Index < 0)
     return NULL;
  cListObject *object = objects;
  while (object && Index-- > 0)
        object = object->Next();
  return object;
}

static int CompareListObjects(const void *a, const void *b)
{
  const cListObject *la = *(const cListObject **)a;
  const cListObject *lb = *(const cListObject **)b;
  return la->Compare(*lb);
}

void cListBase::Sort(void)
{
  int n = Count();
  cListObject *a[n];
  cListObject *object = objects;
  int i = 0;
  while (object && i < n) {
        a[i++] = object;
        object = object->Next();
        }
  qsort(a, n, sizeof(cListObject *), CompareListObjects);
  objects = lastObject = NULL;
  for (i = 0; i < n; i++) {
      a[i]->Unlink();
      count--;
      Add(a[i]);
      }
}

// --- cHashBase -------------------------------------------------------------

cHashBase::cHashBase(int Size)
{
  size = Size;
  hashTable = (cList<cHashObject>**)calloc(size, sizeof(cList<cHashObject>*));
}

cHashBase::~cHashBase(void)
{
  Clear();
  free(hashTable);
}

void cHashBase::Add(cListObject *Object, unsigned int Id)
{
  unsigned int hash = hashfn(Id);
  if (!hashTable[hash])
     hashTable[hash] = new cList<cHashObject>;
  hashTable[hash]->Add(new cHashObject(Object, Id));
}

void cHashBase::Del(cListObject *Object, unsigned int Id)
{
  cList<cHashObject> *list = hashTable[hashfn(Id)];
  if (list) {
     for (cHashObject *hob = list->First(); hob; hob = list->Next(hob)) {
         if (hob->object == Object) {
            list->Del(hob);
            break;
            }
         }
     }
}

void cHashBase::Clear(void)
{
  for (int i = 0; i < size; i++) {
      delete hashTable[i];
      hashTable[i] = NULL;
      }
}

cListObject *cHashBase::Get(unsigned int Id) const
{
  cList<cHashObject> *list = hashTable[hashfn(Id)];
  if (list) {
     for (cHashObject *hob = list->First(); hob; hob = list->Next(hob)) {
         if (hob->id == Id)
            return hob->object;
         }
     }
  return NULL;
}

cList<cHashObject> *cHashBase::GetList(unsigned int Id) const
{
  return hashTable[hashfn(Id)];
}
