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

#include "DVBSourceParams.h"
#include "channels/Channel.h"

using namespace std;

cDvbSourceParams::cDvbSourceParams(char source, const string &strDescription)
 : iSourceParams(source, strDescription),
   m_srate(0)
{
}

void cDvbSourceParams::SetData(const cChannel &channel)
{
  m_srate = channel.Srate();
  m_dtp = channel.Parameters();
}

void cDvbSourceParams::GetData(cChannel &channel) const
{
  channel.SetTransponderData(channel.Source(), channel.FrequencyHz(), m_srate, m_dtp, true);
  // TODO: Overload index operators possibly
  //channel[frequency][source] = TransponderData
}
