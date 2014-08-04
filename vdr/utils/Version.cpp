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

#include "Version.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

const Version Version::EmptyVersion("");

Version::Version(const char* version)
{
  Initialise(version ? version : "");
}

Version::Version(const std::string& version)
{
  Initialise(version);
}

void Version::Initialise(const std::string& originalVersion)
{
  m_originalVersion = originalVersion;
  if (m_originalVersion.empty())
    m_originalVersion = "0.0.0";

  const char *epoch_end = strchr(m_originalVersion.c_str(), ':');
  if (epoch_end != NULL)
    mEpoch = atoi(m_originalVersion.c_str());
  else
    mEpoch = 0;

  const char *upstream_start;
  if (epoch_end)
    upstream_start = epoch_end + 1;
  else
    upstream_start = m_originalVersion.c_str();

  const char *upstream_end = strrchr(upstream_start, '-');
  size_t upstream_size;
  if (upstream_end == NULL)
    upstream_size = strlen(upstream_start);
  else
    upstream_size = upstream_end - upstream_start;

  mUpstream = (char*) malloc(upstream_size + 1);
  strncpy(mUpstream, upstream_start, upstream_size);
  mUpstream[upstream_size] = '\0';

  if (upstream_end == NULL)
    mRevision = strdup("0");
  else
    mRevision = strdup(upstream_end + 1);
}

Version::Version(const Version& other)
 : mUpstream(NULL),
   mRevision(NULL)
{
  *this = other;
}

Version::~Version()
{
  free(mUpstream);
  free(mRevision);
}

Version& Version::operator=(const Version& other)
{
  free(mUpstream);
  free(mRevision);
  mEpoch = other.Epoch();
  mUpstream = strdup(other.Upstream());
  mRevision = strdup(other.Revision());
  m_originalVersion = other.m_originalVersion;
  return *this;
}

int Version::CompareComponent(const char *a, const char *b)
{
  while (*a && *b)
  {
    while (*a && *b && !isdigit(*a) && !isdigit(*b))
    {
      if (*a != *b)
      {
        if (*a == '~') return -1;
        if (*b == '~') return 1;
        return *a < *b ? -1 : 1;
      }
      a++;
      b++;
    }
    if (*a && *b && (!isdigit(*a) || !isdigit(*b)))
    {
      if (*a == '~') return -1;
      if (*b == '~') return 1;
      return isdigit(*a) ? -1 : 1;
    }

    char *next_a, *next_b;
    long int num_a = strtol(a, &next_a, 10);
    long int num_b = strtol(b, &next_b, 10);
    if (num_a != num_b)
      return num_a < num_b ? -1 : 1;

    a = next_a;
    b = next_b;
  }
  if (!*a && !*b)
    return 0;
  if (*a)
    return *a == '~' ? -1 : 1;
  else
    return *b == '~' ? 1 : -1;
}

bool Version::operator==(const Version& other) const
{
  return Epoch() == other.Epoch()
    && CompareComponent(Upstream(), other.Upstream()) == 0
    && CompareComponent(Revision(), other.Revision()) == 0;
}

bool Version::operator<(const Version& other) const
{
  if (Epoch() != other.Epoch())
    return Epoch() < other.Epoch();

  int result = CompareComponent(Upstream(), other.Upstream());
  if (result)
    return (result < 0);

  return (CompareComponent(Revision(), other.Revision()) < 0);
}

bool Version::empty() const
{
  return m_originalVersion.empty() || m_originalVersion == "0.0.0";
}

bool Version::SplitFileName(std::string& ID, std::string& version, const std::string& filename)
{
  size_t dpos = filename.rfind("-");
  if (dpos == std::string::npos)
    return false;
  ID = filename.substr(0, dpos);
  version = filename.substr(dpos + 1);
  version = version.substr(0, version.size() - 4);

  return true;
}

bool Version::Test()
{
  Version v1_0("1.0");
  Version v1_00("1.00");
  Version v1_0_0("1.0.0");
  Version v1_1("1.1");
  Version v1_01("1.01");
  Version v1_0_1("1.0.1");

  bool ret = false;

  // These are totally sane
  ret = (v1_0 < v1_1) && (v1_0 < v1_01) && (v1_0 < v1_0_1) &&
        (v1_1 > v1_0_1) && (v1_01 > v1_0_1);

  // These are rather sane
  ret &= (v1_0 != v1_0_0) && (v1_0 < v1_0_0) && (v1_0_0 > v1_0) &&
         (v1_00 != v1_0_0) && (v1_00 < v1_0_0) && (v1_0_0 > v1_00);

  // GO INSANE
  ret &= (v1_0 == v1_00) && !(v1_0 < v1_00) && !(v1_0 > v1_00);
  ret &= (v1_1 == v1_01) && !(v1_1 < v1_01) && !(v1_1 > v1_01);

  return ret;
}
