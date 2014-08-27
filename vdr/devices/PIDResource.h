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

#include <stdint.h>

namespace VDR
{

/*!
 * Represents a resource (possibly a POSIX file handle) that is associated with
 * a DVB packet ID. This is meant to abstract handles used by the Section Filter
 * and Receiver subsystems.
 *
 * TODO: Because the two subsystems were recently merged, they contain duplicate
 *       code providing similar functionality. This code should be merged.
 *
 * TODO: Consider an abstraction of a PID resource that provides data, such as
 *       the Section Filter subsystem's resources, as opposed to a resource
 *       held to enable data from a shared resource, like the Receiver
 *       subsystem's DVR device.
 */
class cPidResource
{
public:
  cPidResource(uint16_t pid) : m_pid(pid) { }
  virtual ~cPidResource(void) { }

  virtual bool Open(void) = 0;
  virtual void Close(void) = 0;

  uint16_t Pid(void) const { return m_pid; }

private:
  const uint16_t m_pid;
};

}
