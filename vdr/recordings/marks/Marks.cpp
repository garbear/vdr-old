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

#include "Marks.h"
#include "MarksDefinitions.h"
#include "recordings/Recording.h"
#include "recordings/filesystem/IndexFile.h"
#include "recordings/RecordingUserCommand.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"
#include "utils/Tools.h"
#include "utils/XBMCTinyXML.h"

using namespace PLATFORM;

namespace VDR
{

bool cMarks::Load(const std::string& strRecordingFileName, double FramesPerSecond, bool IsPesRecording)
{
  recordingFileName = strRecordingFileName;
  fileName = AddDirectory(strRecordingFileName, MARKSFILESUFFIX);
  framesPerSecond = FramesPerSecond;
  isPesRecording = IsPesRecording;
  nextUpdate = 0;
  lastFileTime = -1; // the first call to Load() must take place!
  lastChange = 0;
  return Update();
}

bool cMarks::Update(void)
{
  time_t t = time(NULL);
  if (t > nextUpdate)
  {
    time_t LastModified = LastModifiedTime(fileName);
    if (LastModified != lastFileTime) // change detected, or first run
      lastChange = LastModified > 0 ? LastModified : t;
    int d = t - lastChange;
    if (d < 60)
      d = 1; // check frequently if the file has just been modified
    else if (d < 3600)
      d = 10; // older files are checked less frequently
    else
      d /= 360; // phase out checking for very old files
    nextUpdate = t + d;
    if (LastModified != lastFileTime)
    { // change detected, or first run
      lastFileTime = LastModified;
      if (lastFileTime == t)
        lastFileTime--; // make sure we don't miss updates in the remaining second
      CLockObject lock(MutexMarkFramesPerSecond);
      MarkFramesPerSecond = framesPerSecond;

      CXBMCTinyXML xmlDoc;
      if (!xmlDoc.LoadFile(fileName))
        return false;

      TiXmlElement *root = xmlDoc.RootElement();
      if (root == NULL)
        return false;

      if (!StringUtils::EqualsNoCase(root->ValueStr(), MARKS_XML_ROOT))
      {
        esyslog("failed to find root element '%s' in file '%s'", MARKS_XML_ROOT, fileName.c_str());
        return false;
      }

      const TiXmlNode* markNode = root->FirstChild(MARKS_XML_ELM_MARK);
      while (markNode != NULL)
      {
        cMark* mark = new cMark;
        if (mark && mark->Deserialise(markNode))
          m_marks.push_back(mark);
        markNode = markNode->NextSibling(MARKS_XML_ELM_MARK);
      }

      Align();
      Sort();
      return true;
    }
  }
  return false;
}

bool cMarks::Save(void)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(MARKS_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  for (std::vector<cMark* >::const_iterator it = m_marks.begin(); it != m_marks.end(); ++it)
  {
    TiXmlElement channelElement(MARKS_XML_ELM_MARK);
    TiXmlNode *channelNode = root->InsertEndChild(channelElement);
    (*it)->Serialise(channelNode);
  }

  assert(!fileName.empty());

  isyslog("saving marks to '%s'", fileName.c_str());
  if (!xmlDoc.SafeSaveFile(fileName))
  {
    esyslog("failed to save marks: could not write to '%s'", fileName.c_str());
    return false;
  }
  return true;
}

void cMarks::Align(void)
{
  cIndexFile IndexFile(recordingFileName, false, isPesRecording);
  for (std::vector<cMark* >::const_iterator it = m_marks.begin(); it != m_marks.end(); ++it)
  {
    int p = IndexFile.GetClosestIFrame((*it)->Position());
    if (int d = (*it)->Position() - p)
    {
      isyslog("aligned editing mark %s to %s (off by %d frame%s)", cMark::IndexToHMSF((*it)->Position(), true, framesPerSecond).c_str(), cMark::IndexToHMSF(p, true, framesPerSecond).c_str(), d, abs(d) > 1 ? "s" : "");
      (*it)->SetPosition(p);
    }
  }
}

void cMarks::Sort(void)
{
  for (std::vector<cMark* >::const_iterator it1 = m_marks.begin(); it1 != m_marks.end(); ++it1)
  {
    for (std::vector<cMark* >::const_iterator it2 = m_marks.begin(); it2 != m_marks.end(); ++it2)
    {
      if ((*it2)->Position() < (*it1)->Position())
      {
        std::swap((*it1)->m_iPosition, (*it2)->m_iPosition);
        std::swap((*it1)->m_strComment, (*it2)->m_strComment);
      }
    }
  }
}

void cMarks::Add(int Position)
{
  m_marks.push_back(new cMark(Position, NULL, framesPerSecond));
  Sort();
}

cMark *cMarks::Get(int Position)
{
  for (std::vector<cMark* >::const_iterator it = m_marks.begin(); it != m_marks.end(); ++it)
  {
    if ((*it)->Position() == Position)
      return (*it);
  }
  return NULL;
}

cMark *cMarks::GetPrev(int Position)
{
  for (std::vector<cMark* >::const_reverse_iterator it = m_marks.rbegin(); it != m_marks.rend(); ++it)
  {
    if ((*it)->Position() < Position)
      return (*it);
  }
  return NULL;
}

cMark *cMarks::GetNext(int Position)
{
  for (std::vector<cMark* >::const_iterator it = m_marks.begin(); it != m_marks.end(); ++it)
  {
    if ((*it)->Position() > Position)
      return (*it);
  }
  return NULL;
}

cMark *cMarks::GetNextBegin(cMark *EndMark)
{
  if (m_marks.empty())
    return NULL;

  cMark *BeginMark = EndMark ? GetNext(EndMark->Position()) : *m_marks.begin();
  if (BeginMark)
  {
    while (cMark *NextMark = GetNext(BeginMark->Position()))
    {
      if (BeginMark->Position() == NextMark->Position())
      { // skip Begin/End at the same position
        if (!(BeginMark = GetNext(NextMark->Position())))
          break;
      }
      else
        break;
    }
  }
  return BeginMark;
}

cMark *cMarks::GetNextEnd(cMark *BeginMark)
{
  if (!BeginMark)
    return NULL;
  cMark *EndMark = GetNext(BeginMark->Position());
  if (EndMark)
  {
    while (cMark *NextMark = GetNext(EndMark->Position()))
    {
      if (EndMark->Position() == NextMark->Position())
      { // skip End/Begin at the same position
        if (!(EndMark = GetNext(NextMark->Position())))
          break;
      }
      else
        break;
    }
  }
  return EndMark;
}

int cMarks::GetNumSequences(void)
{
  int NumSequences = 0;
  if (cMark *BeginMark = GetNextBegin())
  {
    while (cMark *EndMark = GetNextEnd(BeginMark))
    {
      NumSequences++;
      BeginMark = GetNextBegin(EndMark);
    }
    if (BeginMark)
    {
      NumSequences++; // the last sequence had no actual "end" mark
      if (NumSequences == 1 && BeginMark->Position() == 0)
        NumSequences = 0; // there is only one actual "begin" mark at offset zero, and no actual "end" mark
    }
  }
  return NumSequences;
}

}
