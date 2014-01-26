#pragma once

#include "channels/Channel.h"
#include "libsi/section.h"
#include <map>

struct tComponent
{
  uchar stream;
  uchar type;
  char language[MAXLANGCODE2];
  char *description;

  std::string ToString(void);
  bool FromString(const std::string& str);
};

class cComponents
{
public:
  cComponents(void) {}
  virtual ~cComponents(void) {}

  int NumComponents(void) const;
  void SetComponent(int Index, const char *s);
  void SetComponent(int Index, uchar Stream, uchar Type, const char *Language, const char *Description);

  tComponent* Component(int Index, bool bCreate = false);
  tComponent* GetComponent(int Index, uchar Stream, uchar Type); // Gets the Index'th component of Stream and Type, skipping other components
                                                                 // In case of an audio stream the 'type' check actually just distinguishes between "normal" and "Dolby Digital"

private:
  std::map<int, tComponent> m_components;
};
