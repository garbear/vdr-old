/*
 * tools.h: Various tools
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: tools.h 2.24 2013/02/17 13:18:06 kls Exp $
 */

#ifndef __TOOLS_H
#define __TOOLS_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <iconv.h>
#include <math.h>
#include <poll.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <vector>

#ifdef ANDROID
#include "android_sort.h"
#include "android_strchrnul.h"
#define __attribute_format_arg__(x) __attribute__ ((__format_arg__ (x)))
#endif

typedef unsigned char uchar;

extern int SysLogLevel;

#define esyslog(a...) void( (SysLogLevel > 0) ? syslog_with_tid(LOG_ERR, a) : void() )
#define isyslog(a...) void( (SysLogLevel > 1) ? syslog_with_tid(LOG_ERR, a) : void() )
#define dsyslog(a...) void( (SysLogLevel > 2) ? syslog_with_tid(LOG_ERR, a) : void() )

#define LOG_ERROR         esyslog("ERROR (%s,%d): %m", __FILE__, __LINE__)
#define LOG_ERROR_STR(s)  esyslog("ERROR (%s,%d): %s: %m", __FILE__, __LINE__, s)

#define SECSINDAY  86400

#define KILOBYTE(n) ((n) * 1024)
#define MEGABYTE(n) ((n) * 1024LL * 1024LL)

#define MALLOC(type, size)  (type *)malloc(sizeof(type) * (size))

#ifndef PRId64
#define PRId64       "I64d"
#endif

template<class T> inline void DELETENULL(T *&p) { T *q = p; p = NULL; delete q; }

#define CHECK(s) { if ((s) < 0) LOG_ERROR; } // used for 'ioctl()' calls
#define FATALERRNO (errno && errno != EAGAIN && errno != EINTR)

#ifndef __STL_CONFIG_H // in case some plugin needs to use the STL
template<class T> inline T min(T a, T b) { return a <= b ? a : b; }
template<class T> inline T max(T a, T b) { return a >= b ? a : b; }
template<class T> inline int sgn(T a) { return a < 0 ? -1 : a > 0 ? 1 : 0; }
//template<class T> inline void swap(T &a, T &b) { T t = a; a = b; b = t; }
#endif

template<class T> inline T constrain(T v, T l, T h) { return v < l ? l : v > h ? h : v; }

void syslog_with_tid(int priority, const char *format, ...) __attribute__ ((format (printf, 2, 3)));

#define BCDCHARTOINT(x) (10 * ((x & 0xF0) >> 4) + (x & 0xF))
int BCD2INT(int x);

// Unfortunately there are no platform independent macros for unaligned
// access, so we do it this way:

template<class T> inline T get_unaligned(T *p)
{
  struct s { T v; } __attribute__((packed));
  return ((s *)p)->v;
}

template<class T> inline void put_unaligned(unsigned int v, T* p)
{
  struct s { T v; } __attribute__((packed));
  ((s *)p)->v = v;
}

// Comparing doubles for equality is unsafe, but unfortunately we can't
// overwrite operator==(double, double), so this will have to do:

inline bool DoubleEqual(double a, double b)
{
  return fabs(a - b) <= DBL_EPSILON;
}

class cString {
private:
  char *s;
public:
  cString(const char *S = NULL, bool TakePointer = false);
  cString(const cString &String);
  virtual ~cString();
  operator const void * () const { return s; } // to catch cases where operator*() should be used
  operator const char * () const { return s; } // for use in (const char *) context
  const char * operator*() const { return s; } // for use in (const void *) context (printf() etc.)
  cString &operator=(const cString &String);
  cString &operator=(const char *String);
  cString &Truncate(int Index); ///< Truncate the string at the given Index (if Index is < 0 it is counted from the end of the string).
  static cString sprintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
  static cString vsprintf(const char *fmt, va_list &ap);
  };

ssize_t safe_read(int filedes, void *buffer, size_t size);
ssize_t safe_write(int filedes, const void *buffer, size_t size);
void writechar(int filedes, char c);
int WriteAllOrNothing(int fd, const uchar *Data, int Length, int TimeoutMs = 0, int RetryMs = 0);
    ///< Writes either all Data to the given file descriptor, or nothing at all.
    ///< If TimeoutMs is greater than 0, it will only retry for that long, otherwise
    ///< it will retry forever. RetryMs defines the time between two retries.
char *strcpyrealloc(char *dest, const char *src);
char *strn0cpy(char *dest, const char *src, size_t n);
char *strreplace(char *s, char c1, char c2);
char *strreplace(char *s, const char *s1, const char *s2); ///< re-allocates 's' and deletes the original string if necessary!
inline char *skipspace(const char *s)
{
  if ((uchar)*s > ' ') // most strings don't have any leading space, so handle this case as fast as possible
     return (char *)s;
  while (*s && (uchar)*s <= ' ') // avoiding isspace() here, because it is much slower
        s++;
  return (char *)s;
}
char *stripspace(char *s);
char *compactspace(char *s);
cString strescape(const char *s, const char *chars);
bool startswith(const char *s, const char *p);
bool endswith(const char *s, const char *p);
bool isempty(const char *s);
int numdigits(int n);
bool is_number(const char *s);
int64_t StrToNum(const char *s);
    ///< Converts the given string to a number.
    ///< The numerical part of the string may be followed by one of the letters
    ///< K, M, G or T to abbreviate Kilo-, Mega-, Giga- or Terabyte, respectively
    ///< (based on 1024). Everything after the first non-numeric character is
    ///< silently ignored, as are any characters other than the ones mentioned here.
bool StrInArray(const char *a[], const char *s);
    ///< Returns true if the string s is equal to one of the strings pointed
    ///< to by the (NULL terminated) array a.
double atod(const char *s);
    ///< Converts the given string, which is a floating point number using a '.' as
    ///< the decimal point, to a double value, independent of the currently selected
    ///< locale.
cString dtoa(double d, const char *Format = "%f");
    ///< Converts the given double value to a string, making sure it uses a '.' as
    ///< the decimal point, independent of the currently selected locale.
    ///< If Format is given, it will be used instead of the default.
cString itoa(int n);
cString AddDirectory(const char *DirName, const char *FileName);
bool MakeDirs(const char *FileName, bool IsDirectory = false);
bool RemoveFileOrDir(const char *FileName, bool FollowSymlinks = false);
bool RemoveEmptyDirectories(const char *DirName, bool RemoveThis = false, const char *IgnoreFiles[] = NULL);
     ///< Removes all empty directories under the given directory DirName.
     ///< If RemoveThis is true, DirName will also be removed if it is empty.
     ///< IgnoreFiles can be set to an array of file names that will be ignored when
     ///< considering whether a directory is empty. If IgnoreFiles is given, the array
     ///< must end with a NULL pointer.
int DirSizeMB(const char *DirName); ///< returns the total size of the files in the given directory, or -1 in case of an error
char *ReadLink(const char *FileName); ///< returns a new string allocated on the heap, which the caller must delete (or NULL in case of an error)
bool SpinUpDisk(const char *FileName);
void TouchFile(const char *FileName);
time_t LastModifiedTime(const char *FileName);
off_t FileSize(const char *FileName); ///< returns the size of the given file, or -1 in case of an error (e.g. if the file doesn't exist)

class cBitStream {
private:
  const uint8_t *data;
  int length; // in bits
  int index; // in bits
public:
  cBitStream(const uint8_t *Data, int Length) : data(Data), length(Length), index(0) {}
  ~cBitStream() {}
  int GetBit(void);
  uint32_t GetBits(int n);
  void ByteAlign(void);
  void WordAlign(void);
  bool SetLength(int Length);
  void SkipBits(int n) { index += n; }
  void SkipBit(void) { SkipBits(1); }
  bool IsEOF(void) const { return index >= length; }
  void Reset(void) { index = 0; }
  int Length(void) const { return length; }
  int Index(void) const { return (IsEOF() ? length : index); }
  const uint8_t *GetData(void) const { return (IsEOF() ? NULL : data + (index / 8)); }
  };

class cTimeMs {
private:
  uint64_t begin;
public:
  cTimeMs(int Ms = 0);
      ///< Creates a timer with ms resolution and an initial timeout of Ms.
      ///< If Ms is negative the timer is not initialized with the current
      ///< time.
  static uint64_t Now(void);
  void Set(int Ms = 0);
  bool TimedOut(void);
  uint64_t Elapsed(void);
  };

class cReadLine {
private:
  size_t size;
  char *buffer;
public:
  cReadLine(void);
  ~cReadLine();
  char *Read(FILE *f);
  };

class cVDRFile {
private:
  static bool files[];
  static int maxFiles;
  int f;
public:
  cVDRFile(void);
  ~cVDRFile();
  operator int () { return f; }
  bool Open(const char *FileName, int Flags, mode_t Mode = DEFFILEMODE);
  bool Open(int FileDes);
  void Close(void);
  bool IsOpen(void) { return f >= 0; }
  bool Ready(bool Wait = true);
  static bool AnyFileReady(int FileDes = -1, int TimeoutMs = 1000);
  static bool FileReady(int FileDes, int TimeoutMs = 1000);
  static bool FileReadyForWriting(int FileDes, int TimeoutMs = 1000);
  };

class cSafeFile {
private:
  FILE *f;
  char *fileName;
  char *tempName;
public:
  cSafeFile(const char *FileName);
  ~cSafeFile();
  operator FILE* () { return f; }
  bool Open(void);
  bool Close(void);
  };

/// cUnbufferedFile is used for large files that are mainly written or read
/// in a streaming manner, and thus should not be cached.

class cUnbufferedFile {
private:
  int fd;
  off_t curpos;
  off_t cachedstart;
  off_t cachedend;
  off_t begin;
  off_t lastpos;
  off_t ahead;
  size_t readahead;
  size_t written;
  size_t totwritten;
  int FadviseDrop(off_t Offset, off_t Len);
public:
  cUnbufferedFile(void);
  ~cUnbufferedFile();
  int Open(const char *FileName, int Flags, mode_t Mode = DEFFILEMODE);
  int Close(void);
  void SetReadAhead(size_t ra);
  off_t Seek(off_t Offset, int Whence);
  ssize_t Read(void *Data, size_t Size);
  ssize_t Write(const void *Data, size_t Size);
  static cUnbufferedFile *Create(const char *FileName, int Flags, mode_t Mode = DEFFILEMODE);
  };

class cLockFile {
private:
  char *fileName;
  int f;
public:
  cLockFile(const char *Directory);
  ~cLockFile();
  bool Lock(int WaitSeconds = 0);
  void Unlock(void);
  };

class cListObject {
private:
  cListObject *prev, *next;
public:
  cListObject(void);
  virtual ~cListObject();
  virtual int Compare(const cListObject &ListObject) const { return 0; }
      ///< Must return 0 if this object is equal to ListObject, a positive value
      ///< if it is "greater", and a negative value if it is "smaller".
  void Append(cListObject *Object);
  void Insert(cListObject *Object);
  void Unlink(void);
  int Index(void) const;
  cListObject *Prev(void) const { return prev; }
  cListObject *Next(void) const { return next; }
  };

class cListBase {
protected:
  cListObject *objects, *lastObject;
  cListBase(void);
  int count;
public:
  virtual ~cListBase();
  void Add(cListObject *Object, cListObject *After = NULL);
  void Ins(cListObject *Object, cListObject *Before = NULL);
  void Del(cListObject *Object, bool DeleteObject = true);
  virtual void Move(int From, int To);
  void Move(cListObject *From, cListObject *To);
  virtual void Clear(void);
  cListObject *Get(int Index) const;
  int Count(void) const { return count; }
  void Sort(void);
  };

template<class T> class cList : public cListBase {
public:
  T *Get(int Index) const { return (T *)cListBase::Get(Index); }
  T *First(void) const { return (T *)objects; }
  T *Last(void) const { return (T *)lastObject; }
  T *Prev(const T *object) const { return (T *)object->cListObject::Prev(); } // need to call cListObject's members to
  T *Next(const T *object) const { return (T *)object->cListObject::Next(); } // avoid ambiguities in case of a "list of lists"
  };

inline int CompareStrings(const void *a, const void *b)
{
  return strcmp(*(const char **)a, *(const char **)b);
}

inline int CompareStringsIgnoreCase(const void *a, const void *b)
{
  return strcasecmp(*(const char **)a, *(const char **)b);
}

// TODO: Move me to cDirectory!
// Retrieves only directory names...
bool GetSubDirectories(const std::string &strDirectory, std::vector<std::string> &vecFileNames);

class cHashObject : public cListObject {
  friend class cHashBase;
private:
  unsigned int id;
  cListObject *object;
public:
  cHashObject(cListObject *Object, unsigned int Id) { object = Object; id = Id; }
  cListObject *Object(void) { return object; }
  };

class cHashBase {
private:
  cList<cHashObject> **hashTable;
  int size;
  unsigned int hashfn(unsigned int Id) const { return Id % size; }
protected:
  cHashBase(int Size);
public:
  virtual ~cHashBase();
  void Add(cListObject *Object, unsigned int Id);
  void Del(cListObject *Object, unsigned int Id);
  void Clear(void);
  cListObject *Get(unsigned int Id) const;
  cList<cHashObject> *GetList(unsigned int Id) const;
  };

#define HASHSIZE 512

template<class T> class cHash : public cHashBase {
public:
  cHash(int Size = HASHSIZE) : cHashBase(Size) {}
  T *Get(unsigned int Id) const { return (T *)cHashBase::Get(Id); }
};

// SystemExec() implements a 'system()' call that closes all unnecessary file
// descriptors in the child process.
// With Detached=true, calls command in background and in a separate session,
// with stdin connected to /dev/null.

int SystemExec(const char *Command, bool Detached = false);

#endif //__TOOLS_H
