#include "RecordingInfo.h"
#include "Recording.h"
#include "epg/Event.h"
#include "epg/Components.h"
#include "Config.h"

cRecordingInfo::cRecordingInfo(const cChannel *Channel, const cEvent *Event)
{
  channelID = Channel ? Channel->GetChannelID() : tChannelID::InvalidID;
  channelName = Channel ? strdup(Channel->Name().c_str()) : NULL;
  ownEvent = Event ? NULL : new cEvent(0);
  event = ownEvent ? ownEvent : Event;
  aux = NULL;
  framesPerSecond = DEFAULTFRAMESPERSECOND;
  priority = MAXPRIORITY;
  lifetime = MAXLIFETIME;
  fileName = NULL;
  if (Channel) {
     // Since the EPG data's component records can carry only a single
     // language code, let's see whether the channel's PID data has
     // more information:
     CEpgComponents *Components = (CEpgComponents *)event->Components();
     if (!Components)
        Components = new CEpgComponents;
     for (int i = 0; i < MAXAPIDS; i++) {
         const char *s = Channel->Alang(i);
         if (*s) {
            CEpgComponent *Component = Components->GetComponent(i, 2, 3);
            if (!Component)
               Components->SetComponent(COMPONENT_ADD_NEW, 2, 3, s, NULL);
            else if (strlen(s) > strlen(Component->Language().c_str()))
              Component->SetLanguage(s);
            }
         }
     // There's no "multiple languages" for Dolby Digital tracks, but
     // we do the same procedure here, too, in case there is no component
     // information at all:
     for (int i = 0; i < MAXDPIDS; i++) {
         const char *s = Channel->Dlang(i);
         if (*s) {
            CEpgComponent *Component = Components->GetComponent(i, 4, 0); // AC3 component according to the DVB standard
            if (!Component)
               Component = Components->GetComponent(i, 2, 5); // fallback "Dolby" component according to the "Premiere pseudo standard"
            if (!Component)
               Components->SetComponent(COMPONENT_ADD_NEW, 2, 5, s, NULL);
            else if (strlen(s) > strlen(Component->Language().c_str()))
              Component->SetLanguage(s);
            }
         }
     // The same applies to subtitles:
     for (int i = 0; i < MAXSPIDS; i++) {
         const char *s = Channel->Slang(i);
         if (*s) {
            CEpgComponent *Component = Components->GetComponent(i, 3, 3);
            if (!Component)
               Components->SetComponent(COMPONENT_ADD_NEW, 3, 3, s, NULL);
            else if (strlen(s) > strlen(Component->Language().c_str()))
              Component->SetLanguage(s);
            }
         }
     if (Components != event->Components())
        ((cEvent *)event)->SetComponents(Components);
     }
}

cRecordingInfo::cRecordingInfo(const std::string& strFileName)
{
  channelID = tChannelID::InvalidID;
  channelName = NULL;
  ownEvent = new cEvent(0);
  event = ownEvent;
  aux = NULL;
  framesPerSecond = DEFAULTFRAMESPERSECOND;
  priority = MAXPRIORITY;
  lifetime = MAXLIFETIME;
  fileName = strdup(cString::sprintf("%s%s", strFileName.c_str(), INFOFILESUFFIX));
}

cRecordingInfo::~cRecordingInfo()
{
  delete ownEvent;
  free(aux);
  free(channelName);
  free(fileName);
}

void cRecordingInfo::SetData(const char *Title, const char *ShortText, const char *Description)
{
  if (!isempty(Title))
     ((cEvent *)event)->SetTitle(Title);
  if (!isempty(ShortText))
     ((cEvent *)event)->SetShortText(ShortText);
  if (!isempty(Description))
     ((cEvent *)event)->SetDescription(Description);
}

void cRecordingInfo::SetAux(const char *Aux)
{
  free(aux);
  aux = Aux ? strdup(Aux) : NULL;
}

void cRecordingInfo::SetFramesPerSecond(double FramesPerSecond)
{
  framesPerSecond = FramesPerSecond;
}

bool cRecordingInfo::Read(CFile& file)
{
  if (ownEvent)
  {
    cReadLine ReadLine;
    std::string line;
    while (file.ReadLine(line))
    {
      char *t = skipspace(line.c_str() + 1);
      switch (*line.c_str())
      {
      case 'C':
        {
          char *p = strchr(t, ' ');
          if (p)
          {
            free(channelName);
            channelName = strdup(compactspace(p));
            *p = 0; // strips optional channel name
          }
          if (*t)
            channelID = tChannelID::Deserialize(t);
        }
        break;
      case 'E':
        {
          unsigned int EventID;
          time_t StartTime;
          int Duration;
          unsigned int TableID = 0;
          unsigned int Version = 0xFF;
          int n = sscanf(t, "%u %ld %d %X %X", &EventID, &StartTime, &Duration,
              &TableID, &Version);
          if (n >= 3 && n <= 5)
          {
            ownEvent->SetEventID(EventID);
            ownEvent->SetStartTime(StartTime);
            ownEvent->SetDuration(Duration);
            ownEvent->SetTableID(uchar(TableID));
            ownEvent->SetVersion(uchar(Version));
          }
        }
        break;
      case 'F':
        framesPerSecond = atod(t);
        break;
      case 'L':
        lifetime = atoi(t);
        break;
      case 'P':
        priority = atoi(t);
        break;
      case '@':
        free(aux);
        aux = strdup(t);
        break;
      case '#':
        break; // comments are ignored
      default:
        if (!ownEvent->Parse(line))
        {
          esyslog("ERROR: error reading EPG data");
          return false;
        }
        break;
        }
    }
    return true;
  }
  return false;
}

bool cRecordingInfo::Write(CFile& file, const char *Prefix) const
{
  char buf[256];
  size_t pos = 0;

  if (channelID.Valid())
  {
    pos = snprintf(buf, 255, "%sC %s%s%s\n", Prefix, channelID.Serialize().c_str(), channelName ? " " : "", channelName ? channelName : "");
  }
//  XXX event->Dump(f, Prefix, true);

  pos += snprintf(buf + pos, 255 - pos, "%sF %s\n", Prefix, *dtoa(framesPerSecond, "%.10g"));
  pos += snprintf(buf + pos, 255 - pos, "%sP %d\n", Prefix, priority);
  pos += snprintf(buf + pos, 255 - pos, "%sL %d\n", Prefix, lifetime);
  if (aux)
    pos += snprintf(buf + pos, 255 - pos, "%s@ %s\n", Prefix, aux);
  file.Write(buf, pos);
  return true;
}

bool cRecordingInfo::Read(void)
{
  bool Result = false;
  if (fileName)
  {
    CFile file;
    if (file.Open(fileName))
    {
      if (Read(file))
        Result = true;
      else
        esyslog("ERROR: EPG data problem in file %s", fileName);
      file.Close();
    }
    else if (CFile::Exists(fileName))
      LOG_ERROR_STR(fileName);
  }
  return Result;
}

bool cRecordingInfo::Write(void) const
{
  bool Result = false;
  if (fileName)
  {
    CFile file;
    if (file.OpenForWrite(fileName))
    {
      if (Write(file))
      {
        Result = true;
      }
    }
    else
    {
      LOG_ERROR_STR(fileName);
    }
  }
  return Result;
}

std::string cRecordingInfo::Title(void) const
{
  return event->Title();
}

std::string cRecordingInfo::ShortText(void) const
{
  return event->ShortText();
}

std::string cRecordingInfo::Description(void) const
{
  return event->Description();
}

const CEpgComponents* cRecordingInfo::Components(void) const
{
  return event->Components();
}
