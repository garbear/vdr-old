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

#include "Mark.h"
#include "MarksDefinitions.h"
#include "lib/platform/threads/mutex.h"
#include "utils/XBMCTinyXML.h"
#include "utils/StringUtils.h"

#include <math.h>

namespace VDR
{

double MarkFramesPerSecond = DEFAULTFRAMESPERSECOND;
PLATFORM::CMutex MutexMarkFramesPerSecond;

cMark::cMark(int iPosition, const std::string& strComment /* = "" */, double dFramesPerSecond /* = DEFAULTFRAMESPERSECOND */)
{
  m_iPosition        = iPosition;
  m_strComment       = strComment;
  m_dFramesPerSecond = dFramesPerSecond;
}

cMark::~cMark(void)
{
}

bool cMark::Deserialise(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  if (const char* attr = elem->Attribute(MARKS_XML_ATTR_COMMENT))
    m_strComment = attr;

  if (const char* attr = elem->Attribute(MARKS_XML_ATTR_FPS))
    m_dFramesPerSecond = StringUtils::DoubleVal(attr);

  if (const char* attr = elem->Attribute(MARKS_XML_ATTR_POSITION))
    m_iPosition = HMSFToIndex(attr, m_dFramesPerSecond);

  return true;
}

bool cMark::Serialise(TiXmlNode *node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *markElement = node->ToElement();
  if (markElement == NULL)
    return false;

  markElement->SetAttribute(MARKS_XML_ATTR_COMMENT,  m_strComment.c_str());
  markElement->SetAttribute(MARKS_XML_ATTR_FPS,      m_dFramesPerSecond);
  markElement->SetAttribute(MARKS_XML_ATTR_POSITION, IndexToHMSF(m_iPosition, true, m_dFramesPerSecond).c_str());

  return true;
}

int cMark::Position(void) const
{
  return m_iPosition;
}

std::string cMark::Comment(void) const
{
  return m_strComment;
}

void cMark::SetPosition(int Position)
{
  m_iPosition = Position;
}

void cMark::SetComment(const std::string& strComment)
{
  m_strComment = strComment;
}

std::string cMark::IndexToHMSF(int Index, bool WithFrame, double FramesPerSecond)
{
  const char *Sign = "";
  if (Index < 0)
  {
    Index = -Index;
    Sign = "-";
  }
  double Seconds;
  int f = int(
      modf((Index + 0.5) / FramesPerSecond, &Seconds) * FramesPerSecond + 1);
  int s = int(Seconds);
  int m = s / 60 % 60;
  int h = s / 3600;
  s %= 60;
  return StringUtils::Format(WithFrame ? "%s%d:%02d:%02d.%02d" : "%s%d:%02d:%02d", Sign, h, m, s, f);
}


int cMark::HMSFToIndex(const char *HMSF, double FramesPerSecond)
{
  int h, m, s, f = 1;
  int n = sscanf(HMSF, "%d:%d:%d.%d", &h, &m, &s, &f);
  if (n == 1)
    return h - 1; // plain frame number
  if (n >= 3)
    return int(round((h * 3600 + m * 60 + s) * FramesPerSecond)) + f - 1;
  return 0;
}

}
