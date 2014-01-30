#include "Component.h"

using namespace std;

CEpgComponent::CEpgComponent(void)
{
  m_stream      = 0;
  m_type        = 0;
}

string CEpgComponent::Serialise(void)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%X %02X %s %s", m_stream, m_type, m_strLanguage.c_str(), m_strDescription.c_str());
  return buffer;
}

bool CEpgComponent::Deserialse(const std::string& str)
{
  unsigned int Stream, Type;
  char* description;
  char language[MAXLANGCODE2];
  int n = sscanf(str.c_str(), "%X %02X %7s %a[^\n]", &Stream, &Type, language, &description); // 7 = MAXLANGCODE2 - 1
  if (n == 4)
  {
    m_strLanguage    = language;
    m_strDescription = description;
  }
  free(description);
  m_stream = Stream;
  m_type = Type;
  return n >= 3;
}

uchar CEpgComponent::Stream(void) const
{
  return m_stream;
}

uchar CEpgComponent::Type(void) const
{
  return m_type;
}

std::string CEpgComponent::Language(void) const
{
  return m_strLanguage;
}

std::string CEpgComponent::Description(void) const
{
  return m_strDescription;
}

void CEpgComponent::SetStream(uchar stream)
{
  m_stream = stream;
}

void CEpgComponent::SetType(uchar type)
{
  m_type = type;
}

void CEpgComponent::SetLanguage(const std::string& lang)
{
  size_t comma = lang.find(',');
  if (comma != std::string::npos)
    m_strLanguage = lang.substr(0, comma - 1); // strips rest of "normalized" language codes
  else
    m_strLanguage = lang;
}

void CEpgComponent::SetDescription(const std::string& desc)
{
  m_strDescription = desc;
}
