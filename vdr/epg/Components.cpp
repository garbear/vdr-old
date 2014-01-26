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

cComponents::cComponents(void)
{
  numComponents = 0;
  components = NULL;
}

cComponents::~cComponents(void)
{
  for (int i = 0; i < numComponents; i++)
    free(components[i].description);
  free(components);
}

bool cComponents::Realloc(int Index)
{
  if (Index >= numComponents)
  {
    Index++;
    if (tComponent *NewBuffer = (tComponent *) realloc(components, Index * sizeof(tComponent)))
    {
      int n = numComponents;
      numComponents = Index;
      components = NewBuffer;
      memset(&components[n], 0, sizeof(tComponent) * (numComponents - n));
    }
    else
    {
      esyslog("ERROR: out of memory");
      return false;
    }
  }
  return true;
}

void cComponents::SetComponent(int Index, const char *s)
{
  if (Realloc(Index))
    components[Index].FromString(s);
}

void cComponents::SetComponent(int Index, uchar Stream, uchar Type, const char *Language, const char *Description)
{
  if (!Realloc(Index))
    return;
  tComponent *p = &components[Index];
  p->stream = Stream;
  p->type = Type;
  strn0cpy(p->language, Language, sizeof(p->language));
  char *q = strchr(p->language, ',');
  if (q)
    *q = 0; // strips rest of "normalized" language codes
  p->description = strcpyrealloc(p->description, !isempty(Description) ? Description : NULL);
}

tComponent *cComponents::GetComponent(int Index, uchar Stream, uchar Type)
{
  for (int i = 0; i < numComponents; i++)
  {
    if (components[i].stream == Stream && (
          Type == 0 || // don't care about the actual Type
          (Stream == 2 && (components[i].type < 5) == (Type < 5)) // fallback "Dolby" component according to the "Premiere pseudo standard"
         ))
    {
      if (!Index--)
        return &components[i];
    }
  }
  return NULL;
}

int cComponents::NumComponents(void) const
{
  return numComponents;
}

tComponent* cComponents::Component(int Index) const
{
  return (Index < numComponents) ? &components[Index] : NULL;
}
