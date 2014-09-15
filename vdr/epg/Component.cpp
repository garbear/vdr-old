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

#include "Component.h"
#include "EPGDefinitions.h"
#include "utils/StringUtils.h"

#include <tinyxml.h>

using namespace std;

namespace VDR
{

CEpgComponent::CEpgComponent(uint8_t stream, uint8_t type, const std::string& strLang, const std::string& strDesc)
 : m_stream(stream),
   m_type(type),
   m_strDescription(strDesc)
{
  SetLanguage(strLang);
}

void CEpgComponent::Reset(void)
{
  m_stream = 0;
  m_type = 0;
  m_strLanguage.clear();
  m_strDescription.clear();
}

bool CEpgComponent::operator==(const CEpgComponent& rhs) const
{
  return m_stream         == rhs.m_stream      &&
         m_type           == rhs.m_type        &&
         m_strLanguage    == rhs.m_strLanguage &&
         m_strDescription == rhs.m_strDescription;
}

void CEpgComponent::SetLanguage(const std::string& strLang)
{
  size_t comma = strLang.find(',');
  if (comma != std::string::npos)
    m_strLanguage = strLang.substr(0, comma - 1); // strips rest of "normalized" language codes
  else
    m_strLanguage = strLang;
}

bool CEpgComponent::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  elem->SetAttribute(EPG_COMPONENT_XML_ATTR_STREAM,      m_stream);
  elem->SetAttribute(EPG_COMPONENT_XML_ATTR_TYPE,        m_type);
  elem->SetAttribute(EPG_COMPONENT_XML_ATTR_LANGUAGE,    m_strLanguage);
  elem->SetAttribute(EPG_COMPONENT_XML_ATTR_DESCRIPTION, m_strDescription);

  return true;
}

bool CEpgComponent::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement* elem = node->ToElement();
  if (elem == NULL)
    return false;

  Reset();

  const char* strStream = elem->Attribute(EPG_COMPONENT_XML_ATTR_STREAM);
  if (strStream != NULL)
    m_stream = StringUtils::IntVal(strStream);

  const char* strType = elem->Attribute(EPG_COMPONENT_XML_ATTR_TYPE);
  if (strType != NULL)
    m_type = StringUtils::IntVal(strType);

  const char* strLanguage = elem->Attribute(EPG_COMPONENT_XML_ATTR_LANGUAGE);
  if (strLanguage != NULL)
    m_strLanguage = strLanguage;

  const char* strDescription = elem->Attribute(EPG_COMPONENT_XML_ATTR_DESCRIPTION);
  if (strDescription != NULL)
    m_strDescription = strDescription;

  return true;
}

}
