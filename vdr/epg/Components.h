#pragma once

#include "channels/Channel.h"
#include "libsi/section.h"
#include "Component.h"
#include <map>

class CEpgComponents
{
#define COMPONENT_ADD_NEW (-1)

public:
  CEpgComponents(void) {}
  virtual ~CEpgComponents(void) {}

  int NumComponents(void) const;
  void SetComponent(int Index, const char *s);
  void SetComponent(int Index, uchar Stream, uchar Type, const char *Language, const char *Description);

  CEpgComponent* Component(int Index, bool bCreate = false);
  CEpgComponent* GetComponent(int Index, uchar Stream, uchar Type); // Gets the Index'th component of Stream and Type, skipping other components
                                                                 // In case of an audio stream the 'type' check actually just distinguishes between "normal" and "Dolby Digital"

private:
  std::map<int, CEpgComponent> m_components;
};
