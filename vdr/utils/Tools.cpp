/*
 * tools.c: Various tools
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: tools.c 2.29 2012/12/08 11:16:30 kls Exp $
 */

#include "Tools.h"
#include "CommonMacros.h"
#include "DateTime.h"
#include "I18N.h"
#include "StringUtils.h"
#include "UTF8Utils.h"
#include "filesystem/Directory.h"
#include "filesystem/Poller.h"
#include "filesystem/ReadDir.h"
#include "filesystem/SpecialProtocol.h"
#include "platform/threads/threads.h"

#include <algorithm>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

using namespace std;

namespace VDR
{

ssize_t safe_read(int filedes, void *buffer, size_t size)
{
  for (;;)
  {
    ssize_t p = read(filedes, buffer, size);
    if (p < 0 && errno == EINTR)
    {
      dsyslog("EINTR while reading from file handle %d - retrying", filedes);
      continue;
    }
    return p;
  }
  return -1;
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

int WriteAllOrNothing(int fd, const uint8_t *Data, int Length, int TimeoutMs, int RetryMs)
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

void stripspace(std::string& s)
{
  ssize_t strip = s.length();
  for (ssize_t pos = s.length() - 1; pos > 0; pos--)
  {
    if (!isspace(s.at(pos)))
      break;
    strip = pos;
  }
  s.erase(strip);
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

std::string AddDirectory(const std::string& strDirName, const std::string& strFileName)
{
  return StringUtils::Format("%s/%s", !strDirName.empty() ? strDirName.c_str() : ".", strFileName.c_str());
}

#define DECIMAL_POINT_C '.'

double atod(const char *s)
{
  static const lconv *loc = localeconv();
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
  static const lconv *loc = localeconv();
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

bool MakeDirs(const std::string& strFileName, bool IsDirectory)
{
  //XXX
  bool result = true;
  char *s = strdup(strFileName.c_str());
  char *p = s;
  if (*p == '/')
    p++;
  while ((p = strchr(p, '/')) != NULL || IsDirectory)
  {
    if (p)
      *p = 0;
    if (strcmp(s, "special:") && strcmp(s, "special:/") && strcmp(s, "special://home"))
    {
      if (!CDirectory::Exists(s))
      {
        if (!CDirectory::Create(s))
        {
          LOG_ERROR_STR(s);
          result = false;
          break;
        }
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

bool RemoveEmptyDirectories(const std::string& strDirName, bool RemoveThis, const char *IgnoreFiles[])
{
  //XXX
  bool HasIgnoredFiles = false;
  cReadDir d(strDirName.c_str());
  if (d.Ok()) {
     bool empty = true;
     struct dirent *e;
     while ((e = d.Next()) != NULL) {
           if (strcmp(e->d_name, "lost+found")) {
              std::string buffer = AddDirectory(strDirName, e->d_name);
              struct stat st;
              if (stat(buffer.c_str(), &st) == 0) {
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
                 LOG_ERROR_STR(buffer.c_str());
                 empty = false;
                 }
              }
           }
     if (RemoveThis && empty) {
        if (HasIgnoredFiles) {
           while (*IgnoreFiles) {
                 std::string buffer = AddDirectory(strDirName, *IgnoreFiles);
                 if (access(buffer.c_str(), F_OK) == 0) {
                    dsyslog("removing %s", buffer.c_str());
                    if (remove(buffer.c_str()) < 0) {
                       LOG_ERROR_STR(buffer.c_str());
                       return false;
                       }
                    }
                 IgnoreFiles++;
                 }
           }
        dsyslog("removing %s", strDirName.c_str());
        if (remove(strDirName.c_str()) < 0) {
           LOG_ERROR_STR(strDirName.c_str());
           return false;
           }
        }
     return empty;
     }
  else
     LOG_ERROR_STR(strDirName.c_str());
  return false;
}

size_t DirSizeMB(const std::string& strDirName)
{
  //XXX
  return 0;
  cReadDir d(strDirName.c_str());
  if (d.Ok())
  {
    int size = 0;
    struct dirent *e;
    while (size >= 0 && (e = d.Next()) != NULL)
    {
      std::string buffer = AddDirectory(strDirName.c_str(), e->d_name);
      struct stat st;
      if (stat(buffer.c_str(), &st) == 0)
      {
        if (S_ISDIR(st.st_mode))
        {
          int n = DirSizeMB(buffer);
          if (n >= 0)
            size += n;
          else
            size = -1;
        }
        else
          size += st.st_size / MEGABYTE(1);
      }
      else
      {
        LOG_ERROR_STR(buffer.c_str());
        size = -1;
      }
    }
    return size;
  }
  else
    LOG_ERROR_STR(strDirName.c_str());
  return -1;
}

char *ReadLink(const char *FileName)
{
  if (!FileName)
     return NULL;

  char *TargetName = vdr_realpath(FileName);
  if (!TargetName) {
     if (errno == ENOENT) // file doesn't exist
        TargetName = strdup(FileName);
     else // some other error occurred
        LOG_ERROR_STR(FileName);
     }
  return TargetName;
}

bool SpinUpDisk(const std::string& strFileName)
{
  //XXX
  return true;
//  for (int n = 0; n < 10; n++) {
//      std::string buf;
//      if (CDirectory::CanWrite(strFileName))
//         buf = StringUtils::Format("%s/vdr-%06d", !strFileName.empty() ? strFileName.c_str() : ".", n);
//      else
//         buf = StringUtils::Format("%s.vdr-%06d", strFileName.c_str(), n);
//      if (access(buf.c_str(), F_OK) != 0) { // the file does not exist
//         timeval tp1, tp2;
//         gettimeofday(&tp1, NULL);
//         int f = open(buf.c_str(), O_WRONLY | O_CREAT, DEFFILEMODE);
//         // O_SYNC doesn't work on all file systems
//         if (f >= 0) {
//            if (fdatasync(f) < 0)
//               LOG_ERROR_STR(buf.c_str());
//            close(f);
//            remove(buf.c_str());
//            gettimeofday(&tp2, NULL);
//            double seconds = (((long long)tp2.tv_sec * 1000000 + tp2.tv_usec) - ((long long)tp1.tv_sec * 1000000 + tp1.tv_usec)) / 1000000.0;
//            if (seconds > 0.5)
//               dsyslog("SpinUpDisk took %.2f seconds", seconds);
//            return true;
//            }
//         else
//            LOG_ERROR_STR(buf.c_str());
//         }
//      }
//  esyslog("ERROR: SpinUpDisk failed");
//  return false;
}

void TouchFile(const std::string& strFileName)
{
  //XXX
  if (utime(strFileName.c_str(), NULL) == -1 && errno != ENOENT)
     LOG_ERROR_STR(strFileName.c_str());
}

time_t LastModifiedTime(const std::string& strFileName)
{
  struct stat fs;
  //XXX
  if (stat(strFileName.c_str(), &fs) == 0)
     return fs.st_mtime;
  return 0;
}

off_t FileSize(const std::string& strFileName)
{
  struct stat fs;
  //XXX
  if (stat(strFileName.c_str(), &fs) == 0)
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
        //XXX
        if (stat64(AddDirectory(strDirectory.c_str(), e->d_name).c_str(), &ds) == 0)
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

int SystemExec(const char *Command, bool Detached)
{
#if !defined(TARGET_ANDROID)
  pid_t pid;

  if ((pid = fork()) < 0)
  { // fork failed
    LOG_ERROR;
    return -1;
  }

  if (pid > 0)
  { // parent process
    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
    {
      LOG_ERROR;
      return -1;
    }
    return status;
  }
  else
  { // child process
    if (Detached)
    {
      // Fork again and let first child die - grandchild stays alive without parent
      if (fork() > 0)
        _exit(0);
      // Start a new session
      pid_t sid = setsid();
      if (sid < 0)
        LOG_ERROR;
      // close STDIN and re-open as /dev/null
      int devnull = open("/dev/null", O_RDONLY);
      if (devnull < 0 || dup2(devnull, 0) < 0)
        LOG_ERROR;
    }
    int MaxPossibleFileDescriptors = getdtablesize();
    for (int i = STDERR_FILENO + 1; i < MaxPossibleFileDescriptors; i++)
      close(i); //close all dup'ed filedescriptors
    if (execl("/bin/sh", "sh", "-c", Command, NULL) == -1)
    {
      LOG_ERROR_STR(Command);
      _exit(-1);
    }
    _exit(0);
  }
  return 0;
#else
  // disabled on android
  return -1;
#endif
}

}
