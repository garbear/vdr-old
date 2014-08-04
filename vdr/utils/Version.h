/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

/**
 * Versioning using the debian versioning scheme
 *
 * Version uses debian versioning, where period-separated numbers are compared
 * numerically rather than lexicographically.
 *
 * i.e. 1.00 is considered the same as 1.0, and 1.01 is considered the same as 1.1.
 *
 * Further, 1.0 < 1.0.0
 *
 * See here for more info: http://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-Version
 */
class Version
{
public:
  Version(const char* version);
  Version(const std::string& version);
  Version(const Version& other);
  ~Version();

  static const Version EmptyVersion; // 0.0.0

  int Epoch() const { return mEpoch; }
  const char *Upstream() const { return mUpstream; }
  const char *Revision() const { return mRevision; }

  Version& operator=(const Version& other);
  bool operator==(const Version& other) const;
  bool operator!=(const Version& other) const { return !(*this == other); }
  bool operator<(const Version& other) const;
  bool operator<=(const Version& other) const { return *this == other || *this < other; }
  bool operator>(const Version& other) const { return !(*this <= other); }
  bool operator>=(const Version& other) const { return *this == other || *this > other; }
  const std::string& String() const { return m_originalVersion; }
  const char *c_str() const { return m_originalVersion.c_str(); };
  bool empty() const { return *this == EmptyVersion; }

  static bool SplitFileName(std::string& ID, std::string& version,
                            const std::string& filename);

  static bool Test();

private:
  void Initialise(const std::string& originalVersion);

  std::string m_originalVersion;
  int         mEpoch;
  char*       mUpstream;
  char*       mRevision;

  /**
   * Compare two components of a Debian-style version. Return -1, 0, or 1 if a
   * is less than, equal to, or greater than b, respectively.
   */
  static int CompareComponent(const char *a, const char *b);
};
