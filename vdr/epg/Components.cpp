#include "Components.h"

using namespace std;

// --- tComponent ------------------------------------------------------------

string tComponent::ToString(void)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%X %02X %s %s", stream, type, language, description ? description : "");
  return buffer;
}

bool tComponent::FromString(const std::string& str)
{
  unsigned int Stream, Type;
  int n = sscanf(str.c_str(), "%X %02X %7s %a[^\n]", &Stream, &Type, language, &description); // 7 = MAXLANGCODE2 - 1
  if (n != 4 || isempty(description))
  {
    free(description);
    description = NULL;
  }
  stream = Stream;
  type = Type;
  return n >= 3;
}

// --- cComponents -----------------------------------------------------------

void cComponents::SetComponent(int Index, const char *s)
{
  tComponent* component = Component(Index, true);
  if (component)
    component->FromString(s);
}

void cComponents::SetComponent(int Index, uchar Stream, uchar Type, const char *Language, const char *Description)
{
  tComponent* component = Component(Index, true);
  if (!component)
    return;

  component->stream = Stream;
  component->type   = Type;
  strn0cpy(component->language, Language, sizeof(component->language));
  char *q = strchr(component->language, ',');
  if (q)
    *q = 0; // strips rest of "normalized" language codes
  component->description = strcpyrealloc(component->description, !isempty(Description) ? Description : NULL);
}

tComponent *cComponents::GetComponent(int Index, uchar Stream, uchar Type)
{
  for (std::map<int, tComponent>::iterator it = m_components.begin(); it != m_components.end(); ++it)
  {
    if (it->second.stream == Stream && (
          Type == 0 || // don't care about the actual Type
          (Stream == 2 && (it->second.type < 5) == (Type < 5)) // fallback "Dolby" component according to the "Premiere pseudo standard"
         ))
    {
      if (!Index--)
        return &it->second;
    }
  }
  return NULL;
}

int cComponents::NumComponents(void) const
{
  return m_components.size();
}

tComponent* cComponents::Component(int Index, bool bCreate /* = false */)
{
  std::map<int, tComponent>::iterator it = m_components.find(Index);
  if (it != m_components.end())
    return &it->second;

  if (bCreate)
  {
    tComponent component;
    memset(&component, 0, sizeof(tComponent));
    m_components.insert(std::make_pair(Index, component));
    return &m_components[Index];
  }

  return NULL;
}
