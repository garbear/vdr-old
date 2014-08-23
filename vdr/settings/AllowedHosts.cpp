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

#include "AllowedHosts.h"
#include "SettingsDefinitions.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <string.h>
#include <stdlib.h>

namespace VDR
{
void CAllowedHost::Save(TiXmlNode *root)
{
  char ipbuf[32];
  TiXmlElement hostElement(HOSTS_XML_ELM_HOST);
  TiXmlNode* node = root->InsertEndChild(hostElement);

  if (inet_ntop(AF_INET, &addr, ipbuf, sizeof ipbuf))
  {
    node->ToElement()->SetAttribute(HOSTS_XML_ATTR_IP,   ipbuf);
    node->ToElement()->SetAttribute(HOSTS_XML_ATTR_MASK, mask);
  }
}

CAllowedHost* CAllowedHost::Localhost(void)
{
  CAllowedHost* retval = new CAllowedHost;
  retval->mask = 0;
  inet_aton("127.0.0.1", &retval->addr);
  return retval;
}

in_addr_t CAllowedHost::AsMask(void) const
{
  int retval = 0xFFFFFFFF;
  if (mask > 0 && mask < 32)
  {
    retval <<= (32 - mask);
    retval = htonl(retval);
  }
  else
  {
    retval = 0;
  }
  return retval;
}

CAllowedHost* CAllowedHost::Load(const TiXmlNode* node)
{
  CAllowedHost* retval = NULL;
  const TiXmlElement* elem = node->ToElement();
  if (elem && elem->Attribute(HOSTS_XML_ATTR_IP))
  {
    retval = new CAllowedHost;
    retval->mask = 0;

    if (elem->Attribute(HOSTS_XML_ATTR_MASK))
      retval->mask = atoi(elem->Attribute(HOSTS_XML_ATTR_MASK));

    int result = inet_aton(elem->Attribute(HOSTS_XML_ATTR_IP), &retval->addr);
    if (result == 0)
    {
      delete retval;
      retval = NULL;
    }
    else
    {
      dsyslog("allowed host: %s/%d", elem->Attribute(HOSTS_XML_ATTR_IP), retval->mask);
    }
  }
  return retval;
}

bool CAllowedHost::IsLocalhost(void) const
{
  return addr.s_addr == htonl(INADDR_LOOPBACK);
}

bool CAllowedHost::Accepts(in_addr_t Address) const
{
  in_addr_t checkMask = AsMask();
  return (Address & checkMask) == (addr.s_addr & checkMask);
}

CAllowedHosts::CAllowedHosts(void)
{

}

CAllowedHosts& CAllowedHosts::Get(void)
{
  static CAllowedHosts _instance;
  return _instance;
}

CAllowedHosts::~CAllowedHosts(void)
{
  for (std::vector<CAllowedHost*>::iterator it = m_hosts.begin(); it != m_hosts.end(); ++it)
    delete *it;
}

bool CAllowedHosts::LocalhostOnly(void) const
{
  for (std::vector<CAllowedHost*>::const_iterator it = m_hosts.begin(); it != m_hosts.end(); ++it)
  {
    if (!(*it)->IsLocalhost())
      return false;
  }
  return true;
}

bool CAllowedHosts::Acceptable(in_addr_t Address) const
{
  for (std::vector<CAllowedHost*>::const_iterator it = m_hosts.begin(); it != m_hosts.end(); ++it)
  {
    if ((*it)->Accepts(Address))
      return true;
  }
  return false;
}

bool CAllowedHosts::Load(void)
{
  if (!Load("special://home/system/hosts.xml") &&
      !Load("special://vdr/system/hosts.xml"))
  {
    // create a new empty file
    m_hosts.push_back(CAllowedHost::Localhost());
    Save("special://home/system/hosts.xml");
    return false;
  }

  return true;
}

bool CAllowedHosts::Load(const std::string& strFilename)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strFilename.c_str()))
  {
    esyslog("failed to load '%s'", strFilename.c_str());
    return false;
  }

  m_strFilename = strFilename;

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), HOSTS_XML_ROOT))
  {
    esyslog("failed to find root element '%s' in '%s'", HOSTS_XML_ROOT, strFilename.c_str());
    return false;
  }

  CAllowedHost* allowedHost;
  const TiXmlElement* elem;
  const TiXmlNode* hostNode = root->FirstChild(HOSTS_XML_ELM_HOST);
  while (hostNode != NULL)
  {
    allowedHost = CAllowedHost::Load(hostNode);
    if (allowedHost)
      m_hosts.push_back(allowedHost);
    hostNode = hostNode->NextSibling(HOSTS_XML_ELM_HOST);
  }

  return true;
}

bool CAllowedHosts::Save(const std::string& strFilename)
{
  CXBMCTinyXML xmlDoc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  xmlDoc.LinkEndChild(decl);

  TiXmlElement rootElement(HOSTS_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  for (std::vector<CAllowedHost*>::const_iterator it = m_hosts.begin(); it != m_hosts.end(); ++it)
    (*it)->Save(root);

  if (!strFilename.empty())
    m_strFilename = strFilename;

  assert(!m_strFilename.empty());

  isyslog("saving configuration to '%s'", m_strFilename.c_str());
  if (!xmlDoc.SafeSaveFile(m_strFilename))
  {
    esyslog("failed to save the configuration: could not write to '%s'", m_strFilename.c_str());
    return false;
  }
  return true;
}

}
