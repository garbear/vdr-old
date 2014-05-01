/*
 * videodir.h: Functions to maintain a distributed video directory
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: videodir.h 2.3 2012/09/30 11:01:15 kls Exp $
 */

#ifndef __VIDEODIR_H
#define __VIDEODIR_H

#include "filesystem/Directory.h"
#include "utils/Tools.h"
#include "utils/DateTime.h"

#include <stddef.h>
#include <stdlib.h>

namespace VDR
{

class CVideoFile;

extern const char *VideoDirectory;

void SetVideoDirectory(const char *Directory);
CVideoFile *OpenVideoFile(const char *FileName, int Flags);
int CloseVideoFile(CVideoFile *File);
bool RenameVideoFile(const std::string& strOldName, const std::string& strNewName);
bool RemoveVideoFile(const std::string& strFileName);
bool VideoFileSpaceAvailable(size_t iSizeBytes);
unsigned int VideoDiskSpace(disk_space_t& space); // returns the used disk space in percent
void RemoveEmptyVideoDirectories(const char *IgnoreFiles[] = NULL);
bool IsOnVideoDirectoryFileSystem(const std::string& strFileName);

class cVideoDiskUsage {
private:
  static int state;
  static CDateTime lastChecked;
  static size_t usedPercent;
  static size_t freeMB;
  static size_t freeMinutes;
public:
  static bool HasChanged(int &State);
    ///< Returns true if the usage of the video disk space has changed since the last
    ///< call to this function with the given State variable. The caller should
    ///< initialize State to -1, and it will be set to the current internal state
    ///< value of the video disk usage checker upon return. Future calls with the same
    ///< State variable can then quickly check for changes.
  static void ForceCheck(void) { lastChecked.Reset(); }
    ///< To avoid unnecessary load, the video disk usage is only actually checked
    ///< every DISKSPACECHEK seconds. Calling ForceCheck() makes sure that the next call
    ///< to HasChanged() will check the disk usage immediately. This is useful in case
    ///< some files have been deleted and the result shall be displayed instantly.
  static std::string String(void);
    ///< Returns a localized string of the form "Disk nn%  -  hh:mm free".
    ///< This function is mainly for use in skins that want to retain the display of the
    ///< free disk space in the menu title, as was the case until VDR version 1.7.27.
    ///< An implicit call to HasChanged() is done in this function, to make sure the
    ///< returned value is up to date.
  static int UsedPercent(void) { return usedPercent; }
    ///< Returns the used space of the video disk in percent.
    ///< The caller should call HasChanged() first, to make sure the value is up to date.
  static int FreeMB(void) { return freeMB; }
    ///< Returns the amount of free space on the video disk in MB.
    ///< The caller should call HasChanged() first, to make sure the value is up to date.
  static int FreeMinutes(void) { return freeMinutes; }
    ///< Returns the number of minutes that can still be recorded on the video disk.
    ///< This is an estimate and depends on the data rate of the existing recordings.
    ///< There is no guarantee that this value will actually be met.
    ///< The caller should call HasChanged() first, to make sure the value is up to date.
  };

}

#endif //__VIDEODIR_H
