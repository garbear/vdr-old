#include "AllowedHosts.h"
#include "SettingsDefinitions.h"
#include "utils/Tools.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include <string.h>
#include <stdlib.h>

namespace VDR
{

CAllowedHost* CAllowedHost::FromString(const std::string& value)
{
  CAllowedHost* newVal = new CAllowedHost;
  if (!newVal)
    return NULL;

  newVal->mask = 0xFFFFFFFF;
  std::string tmp(value);
  char* p = (char*)strchr(tmp.c_str(), '/');
  if (p)
  {
    char *error = NULL;
    int m = strtoul(p + 1, &error, 10);
    if ((error && *error && !isspace(*error)) || m > 32)
    {
      delete newVal;
      return NULL;
    }

    *p = 0;
    if (m == 0)
    {
      newVal->mask = 0;
    }
    else
    {
      newVal->mask <<= (32 - m);
      newVal->mask = htonl(newVal->mask);
    }
  }
  int result = inet_aton(tmp.c_str(), &newVal->addr);
  if (result != 0 && (newVal->mask != 0 || newVal->addr.s_addr == 0))
    return newVal;
  delete newVal;
  return NULL;
}

bool CAllowedHost::IsLocalhost(void) const
{
  return addr.s_addr == htonl(INADDR_LOOPBACK);
}

bool CAllowedHost::Accepts(in_addr_t Address) const
{
  return (Address & mask) == (addr.s_addr & mask);
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
    m_hosts.push_back(CAllowedHost::FromString("127.0.0.1"));
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
    elem = hostNode->ToElement();
    if (elem && elem->Attribute(SETTINGS_XML_ATTR_VALUE))
    {
      allowedHost = CAllowedHost::FromString(elem->Attribute(SETTINGS_XML_ATTR_VALUE));
      if (allowedHost)
        m_hosts.push_back(allowedHost);
    }
    hostNode = hostNode->NextSibling(HOSTS_XML_ELM_HOST);
  }

  return true;
}

bool CAllowedHosts::Save(const std::string& strFilename)
{
  //TODO
  return false;
}

}