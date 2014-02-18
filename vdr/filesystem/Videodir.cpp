/*
 * videodir.c: Functions to maintain a distributed video directory
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: videodir.c 2.4 2012/09/30 12:06:33 kls Exp $
 */

#include "Videodir.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "filesystem/VideoFile.h"
#include "recordings/Recordings.h"
#include "utils/Tools.h"
#include "Directory.h"
#include "File.h"
#include "settings/Settings.h"

#include <string>

using namespace std;

const char *VideoDirectory = "special://home/video"; // TODO

void SetVideoDirectory(const char *Directory)
{
  VideoDirectory = strdup(Directory);
}

class cVideoDirectory {
private:
  char *name, *stored, *adjusted;
  int length, number, digits;
public:
  cVideoDirectory(void);
  ~cVideoDirectory();
  int FreeMB(int *UsedMB = NULL);
  const char *Name(void) { return name ? name : VideoDirectory; }
  const char *Stored(void) { return stored; }
  int Length(void) { return length; }
  bool IsDistributed(void) { return name != NULL; }
  bool Next(void);
  void Store(void);
  const char *Adjust(const char *FileName);
  };

cVideoDirectory::cVideoDirectory(void)
{
  length = strlen(VideoDirectory);
  name = (VideoDirectory[length - 1] == '0') ? strdup(VideoDirectory) : NULL;
  stored = adjusted = NULL;
  number = -1;
  digits = 0;
}

cVideoDirectory::~cVideoDirectory()
{
  free(name);
  free(stored);
  free(adjusted);
}

int cVideoDirectory::FreeMB(int *UsedMB)
{
  string strPath = name ? name : VideoDirectory;
  unsigned int total, used, free;
  CDirectory::CalculateDiskSpace(strPath, total, used, free);
  if (UsedMB)
    *UsedMB = used;
  return free;
}

bool cVideoDirectory::Next(void)
{
  if (name) {
     if (number < 0) {
        int l = length;
        while (l-- > 0 && isdigit(name[l]))
              ;
        l++;
        digits = length - l;
        int n = atoi(&name[l]);
        if (n == 0)
           number = n;
        else
           return false; // base video directory must end with zero
        }
     if (++number > 0) {
        char buf[16];
        if (sprintf(buf, "%0*d", digits, number) == digits) {
           strcpy(&name[length - digits], buf);
           return CDirectory::CanWrite(name);
           }
        }
     }
  return false;
}

void cVideoDirectory::Store(void)
{
  if (name) {
     free(stored);
     stored = strdup(name);
     }
}

const char *cVideoDirectory::Adjust(const char *FileName)
{
  if (stored) {
     free(adjusted);
     adjusted = strdup(FileName);
     return strncpy(adjusted, stored, length);
     }
  return NULL;
}

CVideoFile *OpenVideoFile(const char *FileName, int Flags)
{
  const char *ActualFileName = FileName;

  // Incoming name must be in base video directory:
  if (strstr(FileName, VideoDirectory) != FileName) {
     esyslog("ERROR: %s not in %s", FileName, VideoDirectory);
     errno = ENOENT; // must set 'errno' - any ideas for a better value?
     return NULL;
     }
  // Are we going to create a new file?
  if ((Flags & O_CREAT) != 0) {
     cVideoDirectory Dir;
     if (Dir.IsDistributed()) {
        // Find the directory with the most free space:
        int MaxFree = Dir.FreeMB();
        while (Dir.Next()) {
              unsigned int total, used, free;
              CDirectory::CalculateDiskSpace(Dir.Name(), total, used, free);
              int Free = free;
              if (Free > MaxFree) {
                 Dir.Store();
                 MaxFree = Free;
                 }
              }
        if (Dir.Stored()) {
           ActualFileName = Dir.Adjust(FileName);
           if (!MakeDirs(ActualFileName, false))
              return NULL; // errno has been set by MakeDirs()
           if (symlink(ActualFileName, FileName) < 0) {
              LOG_ERROR_STR(FileName);
              return NULL;
              }
           ActualFileName = strdup(ActualFileName); // must survive Dir!
           }
        }
     }
  CVideoFile *File = CVideoFile::Create(ActualFileName, Flags, DEFFILEMODE);
  if (ActualFileName != FileName)
     free((char *)ActualFileName);
  return File;
}

int CloseVideoFile(CVideoFile *File)
{
  File->Close();
  delete File;
  return 0;
}

bool RenameVideoFile(const std::string& strOldName, const std::string& strNewName)
{
  return CFile::Rename(strOldName, strNewName);
}

bool RemoveVideoFile(const std::string& strFileName)
{
  if (CDirectory::Exists(strFileName))
    return CDirectory::Remove(strFileName);
  return CFile::Delete(strFileName);
}

bool VideoFileSpaceAvailable(int SizeMB)
{
  cVideoDirectory Dir;
  if (Dir.IsDistributed()) {
     if (Dir.FreeMB() >= SizeMB * 2) // base directory needs additional space
        return true;
     while (Dir.Next()) {
           if (Dir.FreeMB() >= SizeMB)
              return true;
           }
     return false;
     }
  return Dir.FreeMB() >= SizeMB;
}

int VideoDiskSpace(int *FreeMB, int *UsedMB)
{
  int free = 0, used = 0;
  int deleted = Recordings.TotalFileSizeMB(true);
  cVideoDirectory Dir;
  do {
     int u;
     free += Dir.FreeMB(&u);
     used += u;
     } while (Dir.Next());
  if (deleted > used)
     deleted = used; // let's not get beyond 100%
  free += deleted;
  used -= deleted;
  if (FreeMB)
     *FreeMB = free;
  if (UsedMB)
     *UsedMB = used;
  return (free + used) ? used * 100 / (free + used) : 0;
}

void RemoveEmptyVideoDirectories(const char *IgnoreFiles[])
{
  cVideoDirectory Dir;
  do {
     RemoveEmptyDirectories(Dir.Name(), false, IgnoreFiles);
     } while (Dir.Next());
}

bool IsOnVideoDirectoryFileSystem(const std::string& strFileName)
{
  cVideoDirectory Dir;
  do
  {
    if (CFile::OnSameFileSystem(Dir.Name(), strFileName))
      return true;
  }
  while (Dir.Next());
  return false;
}

// --- cVideoDiskUsage -------------------------------------------------------

#define DISKSPACECHEK     5 // seconds between disk space checks
#define MB_PER_MINUTE 25.75 // this is just an estimate!

int cVideoDiskUsage::state = 0;
time_t cVideoDiskUsage::lastChecked = 0;
int cVideoDiskUsage::usedPercent = 0;
int cVideoDiskUsage::freeMB = 0;
int cVideoDiskUsage::freeMinutes = 0;

bool cVideoDiskUsage::HasChanged(int &State)
{
  if (time(NULL) - lastChecked > DISKSPACECHEK) {
     int FreeMB;
     int UsedPercent = VideoDiskSpace(&FreeMB);
     if (FreeMB != freeMB) {
        usedPercent = UsedPercent;
        freeMB = FreeMB;
        double MBperMinute = Recordings.MBperMinute();
        if (MBperMinute <= 0)
           MBperMinute = MB_PER_MINUTE;
        freeMinutes = int(double(FreeMB) / MBperMinute);
        state++;
        }
     lastChecked = time(NULL);
     }
  if (State != state) {
     State = state;
     return true;
     }
  return false;
}

cString cVideoDiskUsage::String(void)
{
  HasChanged(state);
  return cString::sprintf("%s %d%%  -  %2d:%02d %s", tr("Disk"), usedPercent, freeMinutes / 60, freeMinutes % 60, tr("free"));
}
