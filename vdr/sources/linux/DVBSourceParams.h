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

#include "DVBTransponderParams.h"
#include "sources/ISourceParams.h"
#include <string>

namespace VDR
{

class cChannel;

class cDvbSourceParams : public iSourceParams
{
public:
  cDvbSourceParams(char source, const std::string &description);

  /*!
   * \brief Sets all source specific parameters to those of the given Channel
   * */
  virtual void SetData(const cChannel &channel);

  /*!
   * \brief Copies all source specific parameters to the given Channel
   */
  virtual void GetData(cChannel &channel) const;

private:
  cDvbTransponderParams m_dtp;
  int                   m_srate;
};

}
