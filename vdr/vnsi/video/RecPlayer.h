/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2007 Chris Tallon
 *      Portions Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Portions Copyright (C) 2010, 2011 Alexander Pipelka
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

/*
 * This code is taken from VOMP for VDR plugin.
 */

#ifndef VNSI_RECPLAYER_H
#define VNSI_RECPLAYER_H

#include "recordings/Recording.h"
#include "utils/Tools.h"

#include <stdio.h>
#include <vector>

namespace VDR
{

class cIndexFile;

class cSegment
{
  public:
    uint64_t start;
    uint64_t end;
};

class cRecPlayer
{
public:
  cRecPlayer(cRecording* rec, bool inProgress = false);
  ~cRecPlayer();
  uint64_t getLengthBytes();
  uint32_t getLengthFrames();
  int getBlock(unsigned char* buffer, uint64_t position, int amount);

  bool openFile(int index);
  void closeFile();

  void scan();
  void reScan();
  uint64_t positionFromFrameNumber(uint32_t frameNumber);
  uint32_t frameNumberFromPosition(uint64_t position);
  bool getNextIFrame(uint32_t frameNumber, uint32_t direction, uint64_t* rfilePosition, uint32_t* rframeNumber, uint32_t* rframeLength);

private:
  void cleanup();
  std::string fileNameFromIndex(int index);
  void checkBufferSize(int s);

  std::string m_fileName;
  cIndexFile *m_indexFile;
  CFile*      m_file;
  int         m_fileOpen;
  std::vector<cSegment*> m_segments;
  uint64_t    m_totalLength;
  uint32_t    m_totalFrames;
  std::string m_recordingFilename;
  bool        m_pesrecording;
  bool        m_inProgress;
};

}

#endif // VNSI_RECPLAYER_H
