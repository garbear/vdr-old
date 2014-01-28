/*
 * config.c: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.c 2.38 2013/03/18 08:57:50 kls Exp $
 */

#include "Config.h"
#include <ctype.h>
#include <stdlib.h>
#include "devices/Device.h"
#include "recordings/Recording.h"

#include "vdr/utils/UTF8Utils.h"

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

#define ChkDoublePlausibility(Variable, Default) { if (Variable < 0.00001) Variable = Default; }

// --- cSVDRPhost ------------------------------------------------------------

cSVDRPhost::cSVDRPhost(void)
{
  addr.s_addr = 0;
  mask = 0;
}

bool cSVDRPhost::Parse(const char *s)
{
  mask = 0xFFFFFFFF;
  const char *p = strchr(s, '/');
  if (p) {
     char *error = NULL;
     int m = strtoul(p + 1, &error, 10);
     if ((error && *error && !isspace(*error)) || m > 32)
        return false;
     *(char *)p = 0; // yes, we know it's 'const' - will be restored!
     if (m == 0)
        mask = 0;
     else {
        mask <<= (32 - m);
        mask = htonl(mask);
        }
     }
  int result = inet_aton(s, &addr);
  if (p)
     *(char *)p = '/'; // there it is again
  return result != 0 && (mask != 0 || addr.s_addr == 0);
}

bool cSVDRPhost::IsLocalhost(void)
{
  return addr.s_addr == htonl(INADDR_LOOPBACK);
}

bool cSVDRPhost::Accepts(in_addr_t Address)
{
  return (Address & mask) == (addr.s_addr & mask);
}

// --- cSatCableNumbers ------------------------------------------------------

cSatCableNumbers::cSatCableNumbers(int Size, const std::string& bindings)
{
  size = Size;
  array = MALLOC(int, size);
  FromString(bindings.c_str());
}

cSatCableNumbers::~cSatCableNumbers()
{
  free(array);
}

bool cSatCableNumbers::FromString(const char *s)
{
  char *t;
  int i = 0;
  const char *p = s;
  while (p && *p) {
        int n = strtol(p, &t, 10);
        if (t != p) {
           if (i < size)
              array[i++] = n;
           else {
              esyslog("ERROR: too many sat cable numbers in '%s'", s);
              return false;
              }
           }
        else {
           esyslog("ERROR: invalid sat cable number in '%s'", s);
           return false;
           }
        p = skipspace(t);
        }
  for ( ; i < size; i++)
      array[i] = 0;
  return true;
}

cString cSatCableNumbers::ToString(void)
{
  cString s("");
  for (int i = 0; i < size; i++) {
      s = cString::sprintf("%s%d ", *s, array[i]);
      }
  return s;
}

int cSatCableNumbers::FirstDeviceIndex(int DeviceIndex) const
{
  if (0 <= DeviceIndex && DeviceIndex < size) {
     if (int CableNr = array[DeviceIndex]) {
        for (int i = 0; i < size; i++) {
            if (i < DeviceIndex && array[i] == CableNr)
               return i;
            }
        }
     }
  return -1;
}

// --- cNestedItem -----------------------------------------------------------

cNestedItem::cNestedItem(const char *Text, bool WithSubItems)
{
  text = strdup(Text ? Text : "");
  subItems = WithSubItems ? new cList<cNestedItem> : NULL;
}

cNestedItem::~cNestedItem()
{
  delete subItems;
  free(text);
}

int cNestedItem::Compare(const cListObject &ListObject) const
{
  return strcasecmp(text, ((cNestedItem *)&ListObject)->text);
}

void cNestedItem::AddSubItem(cNestedItem *Item)
{
  if (!subItems)
     subItems = new cList<cNestedItem>;
  if (Item)
     subItems->Add(Item);
}

void cNestedItem::SetText(const char *Text)
{
  free(text);
  text = strdup(Text ? Text : "");
}

void cNestedItem::SetSubItems(bool On)
{
  if (On && !subItems)
     subItems = new cList<cNestedItem>;
  else if (!On && subItems) {
     delete subItems;
     subItems = NULL;
     }
}

// --- cNestedItemList -------------------------------------------------------

cNestedItemList::cNestedItemList(void)
{
  fileName = NULL;
}

cNestedItemList::~cNestedItemList()
{
  free(fileName);
}

bool cNestedItemList::Parse(FILE *f, cList<cNestedItem> *List, int &Line)
{
  char *s;
  cReadLine ReadLine;
  while ((s = ReadLine.Read(f)) != NULL) {
        Line++;
        char *p = strchr(s, '#');
        if (p)
           *p = 0;
        s = skipspace(stripspace(s));
        if (!isempty(s)) {
           p = s + strlen(s) - 1;
           if (*p == '{') {
              *p = 0;
              stripspace(s);
              cNestedItem *Item = new cNestedItem(s, true);
              List->Add(Item);
              if (!Parse(f, Item->SubItems(), Line))
                 return false;
              }
           else if (*s == '}')
              break;
           else
              List->Add(new cNestedItem(s));
           }
        }
  return true;
}

bool cNestedItemList::Write(FILE *f, cList<cNestedItem> *List, int Indent)
{
  for (cNestedItem *Item = List->First(); Item; Item = List->Next(Item)) {
      if (Item->SubItems()) {
         fprintf(f, "%*s%s {\n", Indent, "", Item->Text());
         Write(f, Item->SubItems(), Indent + 2);
         fprintf(f, "%*s}\n", Indent + 2, "");
         }
      else
         fprintf(f, "%*s%s\n", Indent, "", Item->Text());
      }
  return true;
}

void cNestedItemList::Clear(void)
{
  free(fileName);
  fileName = NULL;
  cList<cNestedItem>::Clear();
}

bool cNestedItemList::Load(const char *FileName)
{
  cList<cNestedItem>::Clear();
  if (FileName) {
     free(fileName);
     fileName = strdup(FileName);
     }
  bool result = false;
  if (fileName && access(fileName, F_OK) == 0) {
     isyslog("loading %s", fileName);
     FILE *f = fopen(fileName, "r");
     if (f) {
        int Line = 0;
        result = Parse(f, this, Line);
        fclose(f);
        }
     else {
        LOG_ERROR_STR(fileName);
        result = false;
        }
     }
  return result;
}

bool cNestedItemList::Save(void)
{
  bool result = true;
  cSafeFile f(fileName);
  if (f.Open()) {
     result = Write(f, this);
     if (!f.Close())
        result = false;
     }
  else
     result = false;
  return result;
}

// --- Folders and Commands --------------------------------------------------

cNestedItemList Folders;
cNestedItemList Commands;
cNestedItemList RecordingCommands;

// --- cSVDRPhosts -----------------------------------------------------------

cSVDRPhosts SVDRPhosts;

bool cSVDRPhosts::LocalhostOnly(void)
{
  cSVDRPhost *h = First();
  while (h) {
        if (!h->IsLocalhost())
           return false;
        h = (cSVDRPhost *)h->Next();
        }
  return true;
}

bool cSVDRPhosts::Acceptable(in_addr_t Address)
{
  cSVDRPhost *h = First();
  while (h) {
        if (h->Accepts(Address))
           return true;
        h = (cSVDRPhost *)h->Next();
        }
  return false;
}

// --- cSetupLine ------------------------------------------------------------

cSetupLine::cSetupLine(void)
{
  plugin = name = value = NULL;
}

cSetupLine::cSetupLine(const char *Name, const char *Value, const char *Plugin)
{
  name = strreplace(strdup(Name), '\n', 0);
  value = strreplace(strdup(Value), '\n', 0);
  plugin = Plugin ? strreplace(strdup(Plugin), '\n', 0) : NULL;
}

cSetupLine::~cSetupLine()
{
  free(plugin);
  free(name);
  free(value);
}

int cSetupLine::Compare(const cListObject &ListObject) const
{
  const cSetupLine *sl = (cSetupLine *)&ListObject;
  if (!plugin && !sl->plugin)
     return strcasecmp(name, sl->name);
  if (!plugin)
     return -1;
  if (!sl->plugin)
     return 1;
  int result = strcasecmp(plugin, sl->plugin);
  if (result == 0)
     result = strcasecmp(name, sl->name);
  return result;
}

bool cSetupLine::Parse(const char *line)
{
  char* s = strdup(line); //XXX
  char *p = strchr(s, '=');
  if (p)
  {
    *p = 0;
    char *Name = compactspace(s);
    char *Value = compactspace(p + 1);
    if (*Name)
    { // value may be an empty string
      p = strchr(Name, '.');
      if (p)
      {
        *p = 0;
        char *Plugin = compactspace(Name);
        Name = compactspace(p + 1);
        if (!(*Plugin && *Name))
          return false;
        plugin = strdup(Plugin);
      }
      name = strdup(Name);
      value = strdup(Value);
      free(s);
      return true;
    }
  }
  free(s);
  return false;
}

bool cSetupLine::Save(FILE *f)
{
  return fprintf(f, "%s%s%s = %s\n", plugin ? plugin : "", plugin ? "." : "", name, value) > 0;
}

// --- cSetup ----------------------------------------------------------------

cSetup Setup;

cSetup::cSetup(void)
{
  MarkInstantRecord = 1;
  strcpy(NameInstantRecord, TIMERMACRO_TITLE " " TIMERMACRO_EPISODE);
  InstantRecordTime = DEFINSTRECTIME;
  LnbSLOF    = 11700;
  LnbFrequLo =  9750;
  LnbFrequHi = 10600;
  DiSEqC = 0;
  SetSystemTime = 0;
  TimeSource = 0;
  TimeTransponder = 0;
  StandardCompliance = STANDARD_DVB;
  MarginStart = 2;
  MarginStop = 10;
  EPGLanguages[0] = -1;
  EPGScanTimeout = 5;
  EPGBugfixLevel = 3;
  EPGLinger = 0;
  DefaultPriority = 50;
  DefaultLifetime = MAXLIFETIME;
  PausePriority = 10;
  PauseLifetime = 1;
  UseSubtitle = 1;
  UseVps = 0;
  VpsMargin = 120;
  AlwaysSortFoldersFirst = 1;
  VideoDisplayFormat = 1;
  VideoFormat = 0;
  UpdateChannels = 5;
  UseDolbyDigital = 1;
  MaxVideoFileSize = MAXVIDEOFILESIZEDEFAULT;
  SplitEditedFiles = 0;
  MinEventTimeout = 30;
  MinUserInactivity = 300;
  NextWakeupTime = 0;
  ResumeID = 0;
  EmergencyExit = 1;
  EPGDirectory = "special://home/epg";
}

cSetup& cSetup::operator= (const cSetup &s)
{
  memcpy(&__BeginData__, &s.__BeginData__, (char *)&s.__EndData__ - (char *)&s.__BeginData__);
  DeviceBondings = s.DeviceBondings;
  return *this;
}

cSetupLine *cSetup::Get(const char *Name, const char *Plugin)
{
  for (cSetupLine *l = First(); l; l = Next(l)) {
      if ((l->Plugin() == NULL) == (Plugin == NULL)) {
         if ((!Plugin || strcasecmp(l->Plugin(), Plugin) == 0) && strcasecmp(l->Name(), Name) == 0)
            return l;
         }
      }
  return NULL;
}

void cSetup::Store(const char *Name, const char *Value, const char *Plugin, bool AllowMultiple)
{
  if (Name && *Name) {
     cSetupLine *l = Get(Name, Plugin);
     if (l && !AllowMultiple)
        Del(l);
     if (Value)
        Add(new cSetupLine(Name, Value, Plugin));
     }
}

void cSetup::Store(const char *Name, int Value, const char *Plugin)
{
  Store(Name, cString::sprintf("%d", Value), Plugin);
}

void cSetup::Store(const char *Name, double &Value, const char *Plugin)
{
  Store(Name, dtoa(Value, "%f"), Plugin);
}

bool cSetup::Load(const char *FileName)
{
  if (cConfig<cSetupLine>::Load(FileName)) {
     bool result = true;
     for (cSetupLine *l = First(); l; l = Next(l)) {
         bool error = false;
//XXX         if (l->Plugin()) {
//            cPlugin *p = cPluginManager::GetPlugin(l->Plugin());
//            if (p && !p->SetupParse(l->Name(), l->Value()))
//               error = true;
//            }
//         else {
            if (!Parse(l->Name(), l->Value()))
               error = true;
//            }
         if (error) {
            esyslog("ERROR: unknown config parameter: %s%s%s = %s", l->Plugin() ? l->Plugin() : "", l->Plugin() ? "." : "", l->Name(), l->Value());
            result = false;
            }
         }
     return result;
     }
  return false;
}

void cSetup::StoreLanguages(const char *Name, int *Values)
{
  char buffer[I18nLanguages().size() * 4];
  char *q = buffer;
  for (int i = 0; i < (int)I18nLanguages().size(); i++) {
      if (Values[i] < 0)
         break;
      const char *s = I18nLanguageCode(Values[i]);
      if (s) {
         if (q > buffer)
            *q++ = ' ';
         strncpy(q, s, 3);
         q += 3;
         }
      }
  *q = 0;
  Store(Name, buffer);
}

bool cSetup::ParseLanguages(const char *Value, int *Values)
{
  int n = 0;
  while (Value && *Value && n < (int)I18nLanguages().size()) {
        char buffer[4];
        strn0cpy(buffer, Value, sizeof(buffer));
        int i = I18nLanguageIndex(buffer);
        if (i >= 0)
           Values[n++] = i;
        if ((Value = strchr(Value, ' ')) != NULL)
           Value++;
        }
  Values[n] = -1;
  return true;
}

bool cSetup::Parse(const char *Name, const char *Value)
{
  if (!strcasecmp(Name, "MarkInstantRecord"))   MarkInstantRecord  = atoi(Value);
  else if (!strcasecmp(Name, "NameInstantRecord"))   cUtf8Utils::Utf8Strn0Cpy(NameInstantRecord, Value, sizeof(NameInstantRecord));
  else if (!strcasecmp(Name, "InstantRecordTime"))   InstantRecordTime  = atoi(Value);
  else if (!strcasecmp(Name, "LnbSLOF"))             LnbSLOF            = atoi(Value);
  else if (!strcasecmp(Name, "LnbFrequLo"))          LnbFrequLo         = atoi(Value);
  else if (!strcasecmp(Name, "LnbFrequHi"))          LnbFrequHi         = atoi(Value);
  else if (!strcasecmp(Name, "DiSEqC"))              DiSEqC             = atoi(Value);
  else if (!strcasecmp(Name, "SetSystemTime"))       SetSystemTime      = atoi(Value);
  else if (!strcasecmp(Name, "TimeSource"))          TimeSource         = cSource::FromString(Value);
  else if (!strcasecmp(Name, "TimeTransponder"))     TimeTransponder    = atoi(Value);
  else if (!strcasecmp(Name, "StandardCompliance"))  StandardCompliance = atoi(Value);
  else if (!strcasecmp(Name, "MarginStart"))         MarginStart        = atoi(Value);
  else if (!strcasecmp(Name, "MarginStop"))          MarginStop         = atoi(Value);
  else if (!strcasecmp(Name, "EPGLanguages"))        return ParseLanguages(Value, EPGLanguages);
  else if (!strcasecmp(Name, "EPGScanTimeout"))      EPGScanTimeout     = atoi(Value);
  else if (!strcasecmp(Name, "EPGBugfixLevel"))      EPGBugfixLevel     = atoi(Value);
  else if (!strcasecmp(Name, "EPGLinger"))           EPGLinger          = atoi(Value);
  else if (!strcasecmp(Name, "DefaultPriority"))     DefaultPriority    = atoi(Value);
  else if (!strcasecmp(Name, "DefaultLifetime"))     DefaultLifetime    = atoi(Value);
  else if (!strcasecmp(Name, "PausePriority"))       PausePriority      = atoi(Value);
  else if (!strcasecmp(Name, "PauseLifetime"))       PauseLifetime      = atoi(Value);
  else if (!strcasecmp(Name, "UseSubtitle"))         UseSubtitle        = atoi(Value);
  else if (!strcasecmp(Name, "UseVps"))              UseVps             = atoi(Value);
  else if (!strcasecmp(Name, "VpsMargin"))           VpsMargin          = atoi(Value);
  else if (!strcasecmp(Name, "AlwaysSortFoldersFirst")) AlwaysSortFoldersFirst = atoi(Value);
  else if (!strcasecmp(Name, "VideoDisplayFormat"))  VideoDisplayFormat = atoi(Value);
  else if (!strcasecmp(Name, "VideoFormat"))         VideoFormat        = atoi(Value);
  else if (!strcasecmp(Name, "UpdateChannels"))      UpdateChannels     = atoi(Value);
  else if (!strcasecmp(Name, "MaxVideoFileSize"))    MaxVideoFileSize   = atoi(Value);
  else if (!strcasecmp(Name, "SplitEditedFiles"))    SplitEditedFiles   = atoi(Value);
  else if (!strcasecmp(Name, "MinEventTimeout"))     MinEventTimeout    = atoi(Value);
  else if (!strcasecmp(Name, "MinUserInactivity"))   MinUserInactivity  = atoi(Value);
  else if (!strcasecmp(Name, "NextWakeupTime"))      NextWakeupTime     = atoi(Value);
  else if (!strcasecmp(Name, "ResumeID"))            ResumeID           = atoi(Value);
  else if (!strcasecmp(Name, "DeviceBondings"))      DeviceBondings     = Value;
  else if (!strcasecmp(Name, "EmergencyExit"))       EmergencyExit      = atoi(Value);
  else if (!strcasecmp(Name, "EPGDirectory"))        EPGDirectory       = Value;
  else
     return false;
  return true;
}

bool cSetup::Save(void)
{
  Store("MarkInstantRecord",  MarkInstantRecord);
  Store("NameInstantRecord",  NameInstantRecord);
  Store("InstantRecordTime",  InstantRecordTime);
  Store("LnbSLOF",            LnbSLOF);
  Store("LnbFrequLo",         LnbFrequLo);
  Store("LnbFrequHi",         LnbFrequHi);
  Store("DiSEqC",             DiSEqC);
  Store("SetSystemTime",      SetSystemTime);
  Store("TimeTransponder",    TimeTransponder);
  Store("StandardCompliance", StandardCompliance);
  Store("MarginStart",        MarginStart);
  Store("MarginStop",         MarginStop);
  StoreLanguages("EPGLanguages", EPGLanguages);
  Store("EPGScanTimeout",     EPGScanTimeout);
  Store("EPGBugfixLevel",     EPGBugfixLevel);
  Store("EPGLinger",          EPGLinger);
  Store("DefaultPriority",    DefaultPriority);
  Store("DefaultLifetime",    DefaultLifetime);
  Store("PausePriority",      PausePriority);
  Store("PauseLifetime",      PauseLifetime);
  Store("UseSubtitle",        UseSubtitle);
  Store("UseVps",             UseVps);
  Store("VpsMargin",          VpsMargin);
  Store("AlwaysSortFoldersFirst", AlwaysSortFoldersFirst);
  Store("VideoDisplayFormat", VideoDisplayFormat);
  Store("VideoFormat",        VideoFormat);
  Store("UpdateChannels",     UpdateChannels);
  Store("MaxVideoFileSize",   MaxVideoFileSize);
  Store("SplitEditedFiles",   SplitEditedFiles);
  Store("MinEventTimeout",    MinEventTimeout);
  Store("MinUserInactivity",  MinUserInactivity);
  Store("NextWakeupTime",     NextWakeupTime);
  Store("ResumeID",           ResumeID);
  Store("DeviceBondings",     DeviceBondings.c_str());
  Store("EmergencyExit",      EmergencyExit);
  Store("EPGDirectory",       EPGDirectory.c_str());

  Sort();

  if (cConfig<cSetupLine>::Save()) {
     isyslog("saved setup to %s", FileName());
     return true;
     }
  return false;
}
