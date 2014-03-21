#pragma once

#include "Types.h"
#include "channels/Channel.h"
#include "libsi/section.h"
#include <map>

namespace VDR
{
class CEpgComponent
{
public:
  CEpgComponent(void);
  virtual ~CEpgComponent(void) {}

  std::string Serialise(void);
  bool Deserialse(const std::string& str);

  uchar Stream(void) const;
  uchar Type(void) const;
  std::string Language(void) const;
  std::string Description(void) const;

  void SetStream(uchar stream);
  void SetType(uchar type);
  void SetLanguage(const std::string& lang);
  void SetDescription(const std::string& desc);

private:
  uchar       m_stream;
  uchar       m_type;
  std::string m_strLanguage;
  std::string m_strDescription;
};
}
