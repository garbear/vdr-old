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

#include "IndexFile.h"
#include "FileName.h"
#include "devices/Remux.h"
#include "filesystem/Directory.h"
#include "recordings/IndexFileGenerator.h"
#include "recordings/RecordingConfig.h"
#include "recordings/RecordingManager.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/I18N.h"
#include "utils/Ringbuffer.h"
#include "utils/StringUtils.h"

using namespace PLATFORM;

namespace VDR
{

#define INDEXFILESUFFIX     "/index"

// The maximum time to wait before giving up while catching up on an index file:
#define MAXINDEXCATCHUP    8 // number of retries
#define INDEXCATCHUPWAIT 100 // milliseconds

struct tIndexPes {
  uint32_t offset;
  uint8_t  type;
  uint8_t  number;
  uint16_t reserved;
  };

struct tIndexTs {
  uint64_t offset:40; // up to 1TB per file (not using off_t here - must definitely be exactly 64 bit!)
  int reserved:7;     // reserved for future use
  int independent:1;  // marks frames that can be displayed by themselves (for trick modes)
  uint16_t number:16; // up to 64K files per recording
  tIndexTs(off_t Offset, bool Independent, uint16_t Number)
  {
    offset = Offset;
    reserved = 0;
    independent = Independent;
    number = Number;
  }
  };

#define MAXWAITFORINDEXFILE     10 // max. time to wait for the regenerated index file (seconds)
#define INDEXFILECHECKINTERVAL 500 // ms between checks for existence of the regenerated index file
#define INDEXFILETESTINTERVAL   10 // ms between tests for the size of the index file in case of pausing live video

cIndexFile::cIndexFile(const std::string& strFileName, bool Record, bool IsPesRecording, bool PauseLive)
{
  m_iSize = 0;
  m_iLast = -1;
  m_index = NULL;
  m_bIsPesRecording = IsPesRecording;
  m_indexFileGenerator = NULL;
  if (!strFileName.empty()) {
     m_strFilename = IndexFileName(strFileName, m_bIsPesRecording);
     if (!Record && PauseLive) {
        // Wait until the index file contains at least two frames:
        time_t tmax = time(NULL) + MAXWAITFORINDEXFILE;
        while (time(NULL) < tmax && FileSize(m_strFilename.c_str()) < off_t(2 * sizeof(tIndexTs)))
          CEvent::Sleep(INDEXFILETESTINTERVAL);
        }
     int delta = 0;
     if (!Record && !CFile::Exists(m_strFilename)) {
        // Index file doesn't exist, so try to regenerate it:
        if (!m_bIsPesRecording) { // sorry, can only do this for TS recordings
           m_indexFileGenerator = new cIndexFileGenerator(strFileName);
           // Wait until the index file exists:
           time_t tmax = time(NULL) + MAXWAITFORINDEXFILE;
           do {
             CEvent::Sleep(INDEXFILECHECKINTERVAL); // start with a sleep, to give it a head start
              } while (!CFile::Exists(m_strFilename) && time(NULL) < tmax);
           }
        }
     if (CFile::Exists(m_strFilename)) {
        struct __stat64 buf;
        if (CFile::Stat(m_strFilename, &buf) == 0) {
           delta = int(buf.st_size % sizeof(tIndexTs));
           if (delta) {
              delta = sizeof(tIndexTs) - delta;
              esyslog("ERROR: invalid file size (%" PRId64 ") in '%s'", buf.st_size, m_strFilename.c_str());
              }
           m_iLast = int((buf.st_size + delta) / sizeof(tIndexTs) - 1);
           if (!Record && m_iLast >= 0) {
              m_iSize = m_iLast + 1;
              m_index = MALLOC(tIndexTs, m_iSize);
              if (m_index) {
                 if (m_file.Open(m_strFilename))
                 {
                   size_t sz = m_file.Read(m_index, size_t(buf.st_size));
                   if (sz != buf.st_size)
                   {
                     esyslog("ERROR: can't read from file '%s'", m_strFilename.c_str());
                     free(m_index);
                     m_index = NULL;
                   }
                    else if (m_bIsPesRecording)
                       ConvertFromPes(m_index, m_iSize);
                    if (!m_index || time(NULL) - buf.st_mtime >= MININDEXAGE)
                    {
                      m_file.Close();
                    }
                    // otherwise we don't close f here, see CatchUp()!
                    }
                 else
                    LOG_ERROR_STR(m_strFilename.c_str());
                 }
              else
                 esyslog("ERROR: can't allocate %zd bytes for index '%s'", m_iSize * sizeof(tIndexTs), m_strFilename.c_str());
              }
           }
        else
           LOG_ERROR;
        }
     else if (!Record)
        isyslog("missing index file %s", m_strFilename.c_str());
     if (Record)
     {
       if (m_file.OpenForWrite(m_strFilename, false))
       {
         if (delta)
         {
           esyslog("ERROR: padding index file with %d '0' bytes", delta);
           char buf[1] = { (char)0 };
           while (delta--)
             m_file.Write(buf, 1);
         }
       }
       else
         LOG_ERROR_STR(m_strFilename.c_str());
     }
  }
}

cIndexFile::~cIndexFile()
{
  m_file.Close();
  free(m_index);
  delete m_indexFileGenerator;
}

std::string cIndexFile::IndexFileName(const std::string& strFileName, bool IsPesRecording)
{
  return StringUtils::Format("%s%s", strFileName.c_str(), IsPesRecording ? INDEXFILESUFFIX ".vdr" : INDEXFILESUFFIX);
}

void cIndexFile::ConvertFromPes(tIndexTs *IndexTs, int Count)
{
  tIndexPes IndexPes;
  while (Count-- > 0) {
        memcpy(&IndexPes, IndexTs, sizeof(IndexPes));
        IndexTs->offset = IndexPes.offset;
        IndexTs->independent = IndexPes.type == 1; // I_FRAME
        IndexTs->number = IndexPes.number;
        IndexTs++;
        }
}

void cIndexFile::ConvertToPes(tIndexTs *IndexTs, int Count)
{
  tIndexPes IndexPes;
  while (Count-- > 0) {
        IndexPes.offset = uint32_t(IndexTs->offset);
        IndexPes.type = uint8_t(IndexTs->independent ? 1 : 2); // I_FRAME : "not I_FRAME" (exact frame type doesn't matter)
        IndexPes.number = uint8_t(IndexTs->number);
        IndexPes.reserved = 0;
        memcpy(IndexTs, &IndexPes, sizeof(*IndexTs));
        IndexTs++;
        }
}

bool cIndexFile::CatchUp(int Index)
{
  // returns true unless something really goes wrong, so that 'index' becomes NULL
  if (m_index && m_file.IsOpen())
  {
    CLockObject lock(m_mutex);
    // Note that CatchUp() is triggered even if Index is 'last' (and thus valid).
    // This is done to make absolutely sure we don't miss any data at the very end.
    for (int i = 0; i <= MAXINDEXCATCHUP && (Index < 0 || Index >= m_iLast); i++)
    {
      int64_t iPosition = m_file.GetPosition();
      int newLast = int(iPosition / sizeof(tIndexTs) - 1);
      if (newLast > m_iLast)
      {
        int NewSize = m_iSize;
        if (NewSize <= newLast)
        {
          NewSize *= 2;
          if (NewSize <= newLast)
            NewSize = newLast + 1;
        }
        if (tIndexTs *NewBuffer = (tIndexTs *) realloc(m_index,
            NewSize * sizeof(tIndexTs)))
        {
          m_iSize = NewSize;
          m_index = NewBuffer;
          int offset = (m_iLast + 1) * sizeof(tIndexTs);
          int delta = (newLast - m_iLast) * sizeof(tIndexTs);
          if (m_file.Seek(offset, SEEK_SET) == offset)
          {
            if (m_file.Read(&m_index[m_iLast + 1], delta) != delta)
            {
              esyslog("ERROR: can't read from index");
              free(m_index);
              m_index = NULL;
              m_file.Close();
              break;
            }
            if (m_bIsPesRecording)
              ConvertFromPes(&m_index[m_iLast + 1], newLast - m_iLast);
            m_iLast = newLast;
          }
          else
            LOG_ERROR_STR(m_strFilename.c_str());
        }
        else
        {
          esyslog("ERROR: can't realloc() index");
          break;
        }
      }

      if (Index < m_iLast)
        break;
      CCondition<bool> CondVar;
      bool b = false;
      CondVar.Wait(m_mutex, b, INDEXCATCHUPWAIT);
    }
  }
  return m_index != NULL;
}

bool cIndexFile::Write(bool Independent, uint16_t FileNumber, off_t FileOffset)
{
  if (m_file.IsOpen())
  {
    tIndexTs i(FileOffset, Independent, FileNumber);
    if (m_bIsPesRecording)
      ConvertToPes(&i, 1);

    if (!m_file.Write(&i, sizeof(i)))
    {
      LOG_ERROR_STR(m_strFilename.c_str());
      m_file.Close();
      return false;
    }
    m_iLast++;
  }

  return m_file.IsOpen();
}

bool cIndexFile::Get(int Index, uint16_t *FileNumber, off_t *FileOffset, bool *Independent, int *Length)
{
  if (CatchUp(Index)) {
     if (Index >= 0 && Index <= m_iLast) {
        *FileNumber = m_index[Index].number;
        *FileOffset = m_index[Index].offset;
        if (Independent)
           *Independent = m_index[Index].independent;
        if (Length) {
           if (Index < m_iLast) {
              uint16_t fn = m_index[Index + 1].number;
              off_t fo = m_index[Index + 1].offset;
              if (fn == *FileNumber)
                 *Length = int(fo - *FileOffset);
              else
                 *Length = -1; // this means "everything up to EOF" (the buffer's Read function will act accordingly)
              }
           else
              *Length = -1;
           }
        return true;
        }
     }
  return false;
}

int cIndexFile::GetNextIFrame(int Index, bool Forward, uint16_t *FileNumber, off_t *FileOffset, int *Length)
{
  if (CatchUp()) {
     int d = Forward ? 1 : -1;
     for (;;) {
         Index += d;
         if (Index >= 0 && Index <= m_iLast) {
            if (m_index[Index].independent) {
               uint16_t fn;
               if (!FileNumber)
                  FileNumber = &fn;
               off_t fo;
               if (!FileOffset)
                  FileOffset = &fo;
               *FileNumber = m_index[Index].number;
               *FileOffset = m_index[Index].offset;
               if (Length) {
                  if (Index < m_iLast) {
                     uint16_t fn = m_index[Index + 1].number;
                     off_t fo = m_index[Index + 1].offset;
                     if (fn == *FileNumber)
                        *Length = int(fo - *FileOffset);
                     else
                        *Length = -1; // this means "everything up to EOF" (the buffer's Read function will act accordingly)
                     }
                  else
                     *Length = -1;
                  }
               return Index;
               }
            }
         else
            break;
         }
     }
  return -1;
}

int cIndexFile::GetClosestIFrame(int Index)
{
  if (m_iLast > 0) {
     Index = constrain(Index, 0, m_iLast);
     if (m_index[Index].independent)
        return Index;
     int il = Index - 1;
     int ih = Index + 1;
     for (;;) {
         if (il >= 0) {
            if (m_index[il].independent)
               return il;
            il--;
            }
         else if (ih > m_iLast)
            break;
         if (ih <= m_iLast) {
            if (m_index[ih].independent)
               return ih;
            ih++;
            }
         else if (il < 0)
            break;
         }
     }
  return 0;
}

int cIndexFile::Get(uint16_t FileNumber, off_t FileOffset)
{
  if (CatchUp()) {
     //TODO implement binary search!
     int i;
     for (i = 0; i <= m_iLast; i++) {
         if (m_index[i].number > FileNumber || (m_index[i].number == FileNumber) && off_t(m_index[i].offset) >= FileOffset)
            break;
         }
     return i;
     }
  return -1;
}

bool cIndexFile::IsStillRecording()
{
  return m_file.IsOpen();
}

void cIndexFile::Delete(void)
{
  if (!m_strFilename.empty())
  {
    m_file.Close();
    dsyslog("deleting index file '%s'", m_strFilename.c_str());
    CFile::Delete(m_strFilename);
  }
}

int cIndexFile::GetLength(const std::string& strFileName, bool IsPesRecording)
{
  struct __stat64 buf;
  std::string s = IndexFileName(strFileName.c_str(), IsPesRecording); //XXX
  if (!s.empty() && CFile::Stat(s.c_str(), &buf) == 0)
     return buf.st_size / (IsPesRecording ? sizeof(tIndexTs) : sizeof(tIndexPes));
  return -1;
}

bool GenerateIndex(const char *FileName)
{
  if (CDirectory::CanWrite(FileName)) {
     /* TODO
     cRecording Recording(FileName);
     if (!Recording.Name().empty()) {
        if (!Recording.IsPesRecording()) {
           std::string IndexFileName = AddDirectory(FileName, INDEXFILESUFFIX);
           CFile::Delete(IndexFileName.c_str());
           cIndexFileGenerator *IndexFileGenerator = new cIndexFileGenerator(FileName);
           while (IndexFileGenerator->IsRunning())
             CEvent::Sleep(INDEXFILECHECKINTERVAL);
           if (CFile::Exists(IndexFileName))
              return true;
           else
              fprintf(stderr, "cannot create '%s'\n", IndexFileName.c_str());
           }
        else
           fprintf(stderr, "'%s' is not a TS recording\n", FileName);
        }
     else
        fprintf(stderr, "'%s' is not a recording\n", FileName);
     */
     }
  else
     fprintf(stderr, "'%s' is not a directory\n", FileName);
  return false;
}

}
