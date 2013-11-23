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
#pragma once

#include <string>

class cChannel;
class cOsdItem;

/*!
 * \brief Source parameter handling interface
 */
class iSourceParams
{
public:
  /*!
   * \brief Sets up a parameter handler for the given Source
   * \param source Must be in the range 'A'...'Z', and there can only be one
   *        cSourceParam for any given source
   * \param strDescription Contains a short, one line description of this source
   *
   * Constructing a cSourceParam (e.g. for plug-ins) will trigger defining the
   * appropriate cSource automatically.
   */
  iSourceParams(char cource, const std::string &strDescription);
  virtual ~iSourceParams() { }

  char Source() const { return m_source; }

  /*!
   * \brief Sets all source specific parameters to those of the given Channel
   * */
  virtual void SetData(const cChannel &channel) = 0;

  /*!
   * \brief Copies all source specific parameters to the given Channel
   */
  virtual void GetData(cChannel &channel) const = 0;

  /*!
   * \brief Returns all the OSD items necessary for editing the source specific
   *        parameters of the channel that was given in the last call to SetData()
   *
   * Each call to GetOsdItem() returns exactly one such item. After all items
   * have been fetched, any further calls to GetOsdItem() return NULL. After
   * another call to SetData(), the OSD items can be fetched again.
   */
  //virtual cOsdItem *GetOsdItem() = 0;

private:
  char m_source;
};

// TODO: I need a home!
#include <map>
std::map<char, iSourceParams*> gSourceParams;
