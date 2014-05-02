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
#pragma once

namespace VDR
{
typedef enum
{
  SYS_LOG_NONE = 0,
  SYS_LOG_ERROR,
  SYS_LOG_INFO,
  SYS_LOG_DEBUG
} sys_log_level_t;

typedef enum
{
  SYS_LOG_TYPE_CONSOLE = 0,
  SYS_LOG_TYPE_SYSLOG,
  SYS_LOG_TYPE_ADDON
} sys_log_type_t;

class ILog
{
public:
  virtual ~ILog(void) {}

  virtual void Log(sys_log_level_t type, const char* logline) = 0;
  virtual sys_log_type_t Type(void) const = 0;
};
}
