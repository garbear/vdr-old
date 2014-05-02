/*
 * videodir.c: Functions to maintain a distributed video directory
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: videodir.c 2.4 2012/09/30 12:06:33 kls Exp $
 */

#include "Videodir.h"
#include "VideoFile.h"
#include "Directory.h"
#include "File.h"
#include "recordings/Recordings.h"
#include "settings/Settings.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace VDR
{

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
  bool DiskSpace(disk_space_t& space);
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

bool cVideoDirectory::DiskSpace(disk_space_t& space)
{
  string strPath = name ? name : VideoDirectory;
  return CDirectory::CalculateDiskSpace(strPath, space);
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
        disk_space_t maxSpace;
        Dir.DiskSpace(maxSpace);
        while (Dir.Next()) {
              disk_space_t space;
              CDirectory::CalculateDiskSpace(Dir.Name(), space);
              if (space.free > maxSpace.free) {
                 Dir.Store();
                 maxSpace.free = space.free;
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

bool VideoFileSpaceAvailable(size_t iSizeBytes)
{
  cVideoDirectory Dir;
  disk_space_t space;
  if (Dir.IsDistributed())
  {
    Dir.DiskSpace(space);
    if (space.free >= iSizeBytes * 2) // base directory needs additional space
      return true;

    while (Dir.Next())
    {
      Dir.DiskSpace(space);
      if (space.free >= iSizeBytes)
        return true;
    }
    return false;
  }

  Dir.DiskSpace(space);
  return space.free >= iSizeBytes;
}

unsigned int VideoDiskSpace(disk_space_t& space)
{
  memset(&space, 0, sizeof(disk_space_t));
  size_t deleted = Recordings.TotalFileSizeMB(true);
  cVideoDirectory Dir;
  disk_space_t curDir;

  do
  {
    Dir.DiskSpace(curDir);
    space.free += curDir.free;
    space.used += curDir.used;
    space.size += curDir.size;
  }
  while (Dir.Next());

  if (deleted > space.used)
    deleted = space.used; // let's not get beyond 100%

  space.free += deleted;
  space.used -= deleted;

  return (space.free + space.used) ? space.used * 100 / (space.free + space.used) : 0;
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
CDateTime cVideoDiskUsage::lastChecked = 0;
size_t cVideoDiskUsage::usedPercent = 0;
size_t cVideoDiskUsage::freeMB = 0;
size_t cVideoDiskUsage::freeMinutes = 0;

bool cVideoDiskUsage::HasChanged(int &State)
{
  CDateTime now = CDateTime::GetUTCDateTime();
  if ((now - lastChecked).GetSecondsTotal() > DISKSPACECHEK)
  {
    disk_space_t space;
    unsigned int UsedPercent = VideoDiskSpace(space);
    if (space.free / MEGABYTE(1) != freeMB)
    {
      usedPercent = UsedPercent;
      freeMB = space.free / MEGABYTE(1);
      double MBperMinute = Recordings.MBperMinute();
      if (MBperMinute <= 0)
        MBperMinute = MB_PER_MINUTE;
      freeMinutes = size_t(double(space.free / MEGABYTE(1)) / MBperMinute);
      state++;
    }
    lastChecked = now;
  }
  if (State != state)
  {
    State = state;
    return true;
  }
  return false;
}

std::string cVideoDiskUsage::String(void)
{
  HasChanged(state);
  return StringUtils::Format("%s %d%%  -  %2zu:%02zu %s", tr("Disk"), usedPercent, freeMinutes / 60, freeMinutes % 60, tr("free"));
}

}
