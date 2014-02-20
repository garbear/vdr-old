/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2004-2005 Chris Tallon
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 * This code is taken from VOMP for VDR plugin.
 */

#include "RecPlayer.h"
#include "recordings/filesystem/IndexFile.h"
#include "filesystem/File.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

cRecPlayer::cRecPlayer(cRecording* rec, bool inProgress)
{
  m_file          = NULL;
  m_fileOpen      = -1;
  m_recordingFilename = rec->FileName();
  m_inProgress = inProgress;

  // FIXME find out max file path / name lengths
  m_pesrecording = rec->IsPesRecording();
  if(m_pesrecording) isyslog("recording '%s' is a PES recording", m_recordingFilename.c_str());
  m_indexFile = new cIndexFile(m_recordingFilename, false, m_pesrecording);

  scan();
}

void cRecPlayer::cleanup() {
  for (vector<cSegment*>::iterator it = m_segments.begin(); it != m_segments.end(); ++it)
    delete *it;
  m_segments.clear();
}

void cRecPlayer::scan()
{
  struct stat64 s;

  closeFile();

  m_totalLength = 0;
  m_fileOpen    = -1;
  m_totalFrames = 0;

  cleanup();

  isyslog("opening recording '%s'", m_recordingFilename.c_str());

  for(int i = 0; ; i++) // i think we only need one possible loop
  {
    fileNameFromIndex(i);

    if(CFile::Stat(m_fileName.c_str(), &s) == -1) {
      break;
    }

    cSegment* segment = new cSegment();
    segment->start = m_totalLength;
    segment->end = segment->start + s.st_size;

    m_segments.push_back(segment);

    m_totalLength += s.st_size;
    dsyslog("File %i found, size: %lu, totalLength now %llu", i, s.st_size, m_totalLength);
  }

  m_totalFrames = m_indexFile->Last();
  dsyslog("total frames: %u", m_totalFrames);
}

void cRecPlayer::reScan()
{
  struct stat64 s;

  m_totalLength = 0;

  for(int i = 0; ; i++) // i think we only need one possible loop
  {
    fileNameFromIndex(i);

    if(CFile::Stat(m_fileName.c_str(), &s) == -1) {
      break;
    }

    cSegment* segment;
    if (m_segments.size() < i + 1)
    {
      cSegment* segment = new cSegment();
      m_segments.push_back(segment);
      segment->start = m_totalLength;
    }
    else
      segment = m_segments[i];

    segment->end = segment->start + s.st_size;

    m_totalLength += s.st_size;
  }

  m_totalFrames = m_indexFile->Last();
}


cRecPlayer::~cRecPlayer()
{
  cleanup();
  closeFile();
}

std::string cRecPlayer::fileNameFromIndex(int index) {
  if (m_pesrecording)
    m_fileName = StringUtils::Format("%s/%03i.vdr", m_recordingFilename.c_str(), index+1);
  else
    m_fileName = StringUtils::Format("%s/%05i.ts", m_recordingFilename.c_str(), index+1);

  return m_fileName;
}

bool cRecPlayer::openFile(int index)
{
  if (index == m_fileOpen) return true;
  closeFile();

  fileNameFromIndex(index);
  dsyslog("openFile called for index %i string:%s", index, m_fileName.c_str());

  m_file = new CFile;
  if (!m_file->Open(m_fileName.c_str()))
  {
    esyslog("file '%s' failed to open", m_fileName.c_str());
    m_fileOpen = -1;
    delete m_file;
    return false;
  }
  m_fileOpen = index;
  return true;
}

void cRecPlayer::closeFile()
{
  if(!m_file) {
    return;
  }

  dsyslog("recording '%s' closed", m_fileName.c_str());
  if (m_file)
    delete m_file;
  m_file     = NULL;
  m_fileOpen = -1;
}

uint64_t cRecPlayer::getLengthBytes()
{
  return m_totalLength;
}

uint32_t cRecPlayer::getLengthFrames()
{
  return m_totalFrames;
}

int cRecPlayer::getBlock(unsigned char* buffer, uint64_t position, int amount)
{
  // dont let the block be larger than 256 kb
  if (amount > 512*1024)
    amount = 512*1024;

  if ((uint64_t)amount > m_totalLength)
    amount = m_totalLength;

  if (position >= m_totalLength)
  {
    reScan();
    if (position >= m_totalLength)
    {
      return 0;
    }
  }

  if ((position + amount) > m_totalLength)
    amount = m_totalLength - position;

  // work out what block "position" is in
  int segmentNumber = -1;
  for (int i = 0; i < m_segments.size(); i++)
  {
    if ((position >= m_segments[i]->start) && (position < m_segments[i]->end)) {
      segmentNumber = i;
      break;
    }
  }

  // segment not found / invalid position
  if (segmentNumber == -1)
    return 0;

  // open file (if not already open)
  if (!openFile(segmentNumber))
    return 0;

  // work out position in current file
  uint64_t filePosition = position - m_segments[segmentNumber]->start;

  // seek to position
  if(m_file->Seek(filePosition, SEEK_SET) == -1)
  {
    esyslog("unable to seek to position: %llu", filePosition);
    return 0;
  }

  // try to read the block
  int bytes_read = m_file->Read(buffer, amount);

  // we may got stuck at end of segment
  if ((bytes_read == 0) && (position < m_totalLength))
    bytes_read += getBlock(buffer, position+1 , amount);

  if(bytes_read <= 0)
  {
    return 0;
  }

#ifndef ANDROID
  if (!m_inProgress)
  {
    // Tell linux not to bother keeping the data in the FS cache
//    posix_fadvise(m_file, filePosition, bytes_read, POSIX_FADV_DONTNEED);
  }
#endif

  return bytes_read;
}

uint64_t cRecPlayer::positionFromFrameNumber(uint32_t frameNumber)
{
  if (!m_indexFile) return 0;
  uint16_t retFileNumber;
  off_t retFileOffset;
  bool retPicType;
  int retLength;


  if (!m_indexFile->Get((int)frameNumber, &retFileNumber, &retFileOffset, &retPicType, &retLength))
    return 0;

  if (retFileNumber >= m_segments.size())
    return 0;

  uint64_t position = m_segments[retFileNumber]->start + retFileOffset;
  return position;
}

uint32_t cRecPlayer::frameNumberFromPosition(uint64_t position)
{
  if (!m_indexFile) return 0;

  if (position >= m_totalLength)
  {
    dsyslog("Client asked for data starting past end of recording!");
    return m_totalFrames;
  }

  int segmentNumber = -1;
  for(int i = 0; i < m_segments.size(); i++)
  {
    if ((position >= m_segments[i]->start) && (position < m_segments[i]->end)) {
      segmentNumber = i;
      break;
    }
  }

  if(segmentNumber == -1) {
    return m_totalFrames;
  }

  uint32_t askposition = position - m_segments[segmentNumber]->start;
  return m_indexFile->Get((int)segmentNumber, askposition);
}


bool cRecPlayer::getNextIFrame(uint32_t frameNumber, uint32_t direction, uint64_t* rfilePosition, uint32_t* rframeNumber, uint32_t* rframeLength)
{
  // 0 = backwards
  // 1 = forwards

  if (!m_indexFile) return false;

  uint16_t waste1;
  off_t waste2;

  int iframeLength;
  int indexReturnFrameNumber;

  indexReturnFrameNumber = (uint32_t)m_indexFile->GetNextIFrame(frameNumber, (direction==1 ? true : false), &waste1, &waste2, &iframeLength);
  dsyslog("GNIF input framenumber:%u, direction=%u, output:framenumber=%i, framelength=%i", frameNumber, direction, indexReturnFrameNumber, iframeLength);

  if (indexReturnFrameNumber == -1) return false;

  *rfilePosition = positionFromFrameNumber(indexReturnFrameNumber);
  *rframeNumber = (uint32_t)indexReturnFrameNumber;
  *rframeLength = (uint32_t)iframeLength;

  return true;
}
