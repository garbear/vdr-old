#pragma once

#include "channels/Channel.h"
#include "libsi/section.h"

#include <map>
#include <stdint.h>

namespace VDR
{
class CEpgComponent
{
public:
  CEpgComponent(void);
  virtual ~CEpgComponent(void) {}

  std::string Serialise(void);
  bool Deserialse(const std::string& str);

  uint8_t Stream(void) const;
  uint8_t Type(void) const;
  std::string Language(void) const;
  std::string Description(void) const;

  void SetStream(uint8_t stream);
  void SetType(uint8_t type);
  void SetLanguage(const std::string& lang);
  void SetDescription(const std::string& desc);

private:
  uint8_t     m_stream;
  uint8_t     m_type;
  std::string m_strLanguage;
  std::string m_strDescription;
};
}
