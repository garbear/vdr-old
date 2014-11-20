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
#include <stdio.h>
#include <string>

namespace VDR
{

class cPsiBuffer;

/*!
 * Represents a resource (possibly a POSIX file handle) that is associated with
 * a DVB packet ID. This is used to abstract handles used by the Receiver
 * subsystem.
 */
class cPidResource
{
public:
  cPidResource(uint16_t pid);
  cPidResource(uint16_t pid, uint8_t tid);

  virtual ~cPidResource(void);

  virtual bool Equals(const cPidResource* other) const = 0;

  /*!
   * Returns true if the pid can uniquely describe the resource. If the resource
   * requires other properties to perform the comparison (e.g. streaming
   * resources require tid to be compared as well), then this return false.
   */
  virtual bool Equals(uint16_t pid) const = 0;

  virtual bool Open(void) = 0;
  virtual void Close(void) = 0;

  /*!
   * Override this is if the resource can stream data.
   */
  virtual bool Read(const uint8_t** outdata, size_t* outlen) { return false; }

  uint16_t    Pid(void) const    { return m_pid; }
  cPsiBuffer* Buffer(void) const { return m_buffer; }

  virtual std::string ToString(void) const = 0;

private:
  const uint16_t    m_pid; // Packet ID
  cPsiBuffer* const m_buffer;
};

}
