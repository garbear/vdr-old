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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>

class TiXmlNode;

namespace VDR
{

class CAllowedHost
{
public:
  virtual ~CAllowedHost(void) {}

  bool IsLocalhost(void) const;
  bool Accepts(in_addr_t Address) const;

  void Save(TiXmlNode *root);
  static CAllowedHost* Load(const TiXmlNode *root);
  static CAllowedHost* Localhost(void);

private:
  CAllowedHost(void) {}

  in_addr_t AsMask(void) const;

  struct in_addr addr;
  int            mask;
};

class CAllowedHosts
{
public:
  static CAllowedHosts& Get(void);
  virtual ~CAllowedHosts(void);

  bool LocalhostOnly(void) const;
  bool Acceptable(in_addr_t Address) const;

  bool Load(void);
  bool Load(const std::string& strFilename);
  bool Save(const std::string& strFilename);

private:
  CAllowedHosts(void);

  std::string                m_strFilename;
  std::vector<CAllowedHost*> m_hosts;
};

}
