#include "Components.h"

using namespace std;

void CEpgComponents::SetComponent(int Index, const char *s)
{
  CEpgComponent* component = Component(Index, true);
  if (component)
    component->Deserialse(s);
}

void CEpgComponents::SetComponent(int Index, uchar Stream, uchar Type, const char *Language, const char *Description)
{
  CEpgComponent* component = Component(Index, true);
  if (!component)
    return;

  component->SetStream(Stream);
  component->SetType(Type);
  component->SetLanguage(Language);
  component->SetDescription(!isempty(Description) ? Description : "");
}

CEpgComponent *CEpgComponents::GetComponent(int Index, uchar Stream, uchar Type)
{
  for (std::map<int, CEpgComponent>::iterator it = m_components.begin(); it != m_components.end(); ++it)
  {
    if (it->second.Stream() == Stream && (
          Type == 0 || // don't care about the actual Type
          (Stream == 2 && (it->second.Type() < 5) == (Type < 5)) // fallback "Dolby" component according to the "Premiere pseudo standard"
         ))
    {
      if (!Index--)
        return &it->second;
    }
  }
  return NULL;
}

int CEpgComponents::NumComponents(void) const
{
  return m_components.size();
}

CEpgComponent* CEpgComponents::Component(int Index, bool bCreate /* = false */)
{
  if (Index >= 0)
  {
    std::map<int, CEpgComponent>::iterator it = m_components.find(Index);
    if (it != m_components.end())
      return &it->second;
  }

  if (bCreate)
  {
    CEpgComponent component;
    if (Index < 0)
      Index = m_components.size();
    m_components.insert(std::make_pair(Index, component));
    return &m_components[Index];
  }

  return NULL;
}
