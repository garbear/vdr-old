/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "ISourceParams.h"
//#include "Source.h"
#include "utils/Tools.h" // for logging

using namespace std;

iSourceParams::iSourceParams(char source, const string &strDescription)
 : m_source(source)
{
  if ('A' <= m_source && m_source <= 'Z')
  {
    if (m_source != 'A' &&
        m_source != 'C' &&
        m_source != 'S' &&
        m_source != 'T') // no, it's not "ATSC" ;-)
    {
      // Define the appropriate cSource
      //cSource sourceObject(m_source, strDescription);
      //gSources[sourceObject.Code()] = sourceObject;
    }

    dsyslog("registered source parameters for '%c - %s'", m_source, strDescription.c_str());
  }
  else
    esyslog("ERROR: invalid source '%c'", m_source);
}
