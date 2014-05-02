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
#pragma once

#include "List.h"
#include "filesystem/File.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace VDR
{

ssize_t safe_read(int filedes, void *buffer, size_t size);
ssize_t safe_write(int filedes, const void *buffer, size_t size);
void writechar(int filedes, char c);
int WriteAllOrNothing(int fd, const uint8_t *Data, int Length, int TimeoutMs = 0, int RetryMs = 0);
    ///< Writes either all Data to the given file descriptor, or nothing at all.
    ///< If TimeoutMs is greater than 0, it will only retry for that long, otherwise
    ///< it will retry forever. RetryMs defines the time between two retries.
char *strcpyrealloc(char *dest, const char *src);
char *strn0cpy(char *dest, const char *src, size_t n);
char *strreplace(char *s, char c1, char c2);
char *strreplace(char *s, const char *s1, const char *s2); ///< re-allocates 's' and deletes the original string if necessary!
inline char *skipspace(const char *s)
{
  if ((uint8_t)*s > ' ') // most strings don't have any leading space, so handle this case as fast as possible
     return (char *)s;
  while (*s && (uint8_t)*s <= ' ') // avoiding isspace() here, because it is much slower
        s++;
  return (char *)s;
}
char *stripspace(char *s);
void stripspace(std::string& s);
char *compactspace(char *s);
std::string strescape(const char *s, const char *chars);
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
std::string AddDirectory(const std::string& strDirName, const std::string& strFileName);
bool MakeDirs(const std::string& strFileName, bool IsDirectory = false);
bool RemoveEmptyDirectories(const std::string& strDirName, bool RemoveThis = false, const char *IgnoreFiles[] = NULL);
     ///< Removes all empty directories under the given directory DirName.
     ///< If RemoveThis is true, DirName will also be removed if it is empty.
     ///< IgnoreFiles can be set to an array of file names that will be ignored when
     ///< considering whether a directory is empty. If IgnoreFiles is given, the array
     ///< must end with a NULL pointer.
size_t DirSizeMB(const std::string& strDirName); ///< returns the total size of the files in the given directory, or -1 in case of an error
char *ReadLink(const char* FileName); ///< returns a new string allocated on the heap, which the caller must delete (or NULL in case of an error)
bool SpinUpDisk(const std::string& strFileName);
void TouchFile(const std::string& strFileName);
time_t LastModifiedTime(const std::string& strFileName);
off_t FileSize(const std::string& strFileName); ///< returns the size of the given file, or -1 in case of an error (e.g. if the file doesn't exist)

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

// SystemExec() implements a 'system()' call that closes all unnecessary file
// descriptors in the child process.
// With Detached=true, calls command in background and in a separate session,
// with stdin connected to /dev/null.

int SystemExec(const char *Command, bool Detached = false);

}
