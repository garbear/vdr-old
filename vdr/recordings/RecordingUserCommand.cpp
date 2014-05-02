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

#include "RecordingUserCommand.h"
#include "utils/Tools.h"
#include "utils/StringUtils.h"
#include "utils/log/Log.h"

namespace VDR
{

cRecordingUserCommand& cRecordingUserCommand::Get(void)
{
  static cRecordingUserCommand _instance;
  return _instance;
}

void cRecordingUserCommand::SetCommand(const std::string& strCommand)
{
  m_strCommand = strCommand;
}

void cRecordingUserCommand::InvokeCommand(const std::string& strState, const std::string& strRecordingFileName, const std::string& strSourceFileName /* = "" */)
{
  if (!m_strCommand.empty())
  {
    std::string cmd;
    if (!strSourceFileName.empty())
       cmd = StringUtils::Format("%s %s \"%s\" \"%s\"", m_strCommand.c_str(), strState.c_str(), strescape(strState.c_str(), "\\\"$").c_str(), strescape(strSourceFileName.c_str(), "\\\"$").c_str());
    else
       cmd = StringUtils::Format("%s %s \"%s\"", m_strCommand.c_str(), strState.c_str(), strescape(strState.c_str(), "\\\"$").c_str());
    isyslog("executing '%s'", cmd.c_str());
    SystemExec(cmd.c_str());
  }
}

}
