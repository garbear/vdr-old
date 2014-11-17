/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Event.h"
#include "EPGDefinitions.h"
#include "EPGStringifier.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <stddef.h>

#define TABLE_ID_INVALID           0xFF
#define VERSION_INVALID            0xFF
#define MAX_USEFUL_EPISODE_LENGTH  40  // For fixing EPG bugs

using namespace std;

namespace VDR
{

const EventPtr cEvent::EmptyEvent;

cEvent::cEvent(unsigned int eventID)
 : m_eventID(eventID)
{
  Reset();
}

void cEvent::Reset(void)
{
  m_atscSourceId = 0;
  m_strTitle.clear();
  m_strPlotOutline.clear();
  m_strPlot.clear();
  m_channelID = cChannelID::InvalidID;
  m_startTime.Reset();
  m_endTime.Reset();
  m_genreType = EPG_GENRE_UNDEFINED;
  m_genreSubType = EPG_SUB_GENRE_DRAMA_OTHER;
  m_strCustomGenre.clear();
  m_parentalRating = 0;
  m_starRating = 0;
  m_tableID = TABLE_ID_INVALID;
  m_version = VERSION_INVALID;
  m_vps.Reset();
  m_components.clear();
  m_contents.clear();
}

cEvent& cEvent::operator=(const cEvent& rhs)
{
  if (this != &rhs)
  {
    SetAtscSourceID(rhs.AtscSourceID());
    SetTitle(rhs.Title());
    SetPlotOutline(rhs.PlotOutline());
    SetPlot(rhs.Plot());
    SetChannelID(rhs.ChannelID());
    SetStartTime(rhs.StartTime());
    SetEndTime(rhs.EndTime());
    SetGenre(rhs.Genre(), rhs.SubGenre());
    SetParentalRating(rhs.ParentalRating());
    SetStarRating(rhs.StarRating());
    SetTableID(rhs.TableID());
    SetVersion(rhs.Version());
    SetVps(rhs.Vps());
    SetComponents(rhs.Components());
    SetContents(rhs.Contents());
  }
  return *this;
}

void cEvent::SetAtscSourceID(uint32_t atscSourceId)
{
  if (m_atscSourceId != atscSourceId)
  {
    m_atscSourceId = atscSourceId;
    SetChanged();
  }
}

void cEvent::SetTitle(const std::string& strTitle)
{
  if (m_strTitle != strTitle)
  {
    m_strTitle = strTitle;
    SetChanged();
  }
}

void cEvent::SetPlotOutline(const std::string& strPlotOutline)
{
  if (m_strPlotOutline != strPlotOutline)
  {
    m_strPlotOutline = strPlotOutline;
    SetChanged();
  }
}

void cEvent::SetPlot(const std::string& strPlot)
{
  if (m_strPlot != strPlot)
  {
    m_strPlot = strPlot;
    SetChanged();
  }
}

void cEvent::SetChannelID(const cChannelID& channelId)
{
  if (m_channelID != channelId)
  {
    m_channelID = channelId;
    SetChanged();
  }
}

void cEvent::SetStartTime(const CDateTime& startTime)
{
  if (m_startTime != startTime)
  {
    m_startTime = startTime;
    SetChanged();
  }
}

void cEvent::SetEndTime(const CDateTime& endTime)
{
  if (m_endTime != endTime)
  {
    m_endTime = endTime;
    SetChanged();
  }
}

void cEvent::SetGenre(EPG_GENRE genre, EPG_SUB_GENRE subGenre)
{
  if (m_genreType != genre || m_genreSubType != subGenre)
  {
    m_genreType = genre;
    m_genreSubType = subGenre;
    SetChanged();
  }
}

void cEvent::SetCustomGenre(const std::string& strCustomGenre)
{
  if (m_genreType != EPG_GENRE_CUSTOM || m_strCustomGenre != strCustomGenre)
  {
    m_genreType = EPG_GENRE_CUSTOM;
    m_strCustomGenre = strCustomGenre;
    SetChanged();
  }
}

std::string cEvent::ParentalRatingString(void) const
{
  static const char* const ratings[8] = { "", "G", "PG", "PG-13", "R", "NR/AO", "", "NC-17" };

  std::string strBuffer = ratings[(m_parentalRating >> 10) & 0x07];

  if (m_parentalRating & 0x37F)
  {
    strBuffer += " [";
    if (m_parentalRating & 0x0230)
      strBuffer += "V,";
    if (m_parentalRating & 0x000A)
      strBuffer += "L,";
    if (m_parentalRating & 0x0044)
      strBuffer += "N,";
    if (m_parentalRating & 0x0101)
      strBuffer += "SC,";
    strBuffer += "]";
  }

  return strBuffer;
}

void cEvent::SetParentalRating(uint16_t parentalRating)
{
  if (m_parentalRating != parentalRating)
  {
    m_parentalRating = parentalRating;
    SetChanged();
  }
}

const char* cEvent::StarRatingString(void) const
{
  static const char* const critiques[8] = { "", "*", "*+", "**", "**+", "***", "***+", "****" };
  return critiques[m_starRating & 0x07];
}

void cEvent::SetStarRating(uint8_t starRating)
{
  if (m_starRating != starRating)
  {
    m_starRating = starRating;
    SetChanged();
  }
}

void cEvent::SetTableID(uint8_t tableID)
{
  if (m_tableID != tableID)
  {
    m_tableID = tableID;
    SetChanged();
  }
}

void cEvent::SetVersion(uint8_t version)
{
  if (m_version != version)
  {
    m_version = version;
    SetChanged();
  }
}

void cEvent::SetVps(const CDateTime& vps)
{
  if (m_vps != vps)
  {
    m_vps = vps;
    SetChanged();
  }
}

void cEvent::SetComponents(const std::vector<CEpgComponent>& components)
{
  if (m_components != components)
  {
    m_components = components;
    SetChanged();
  }
}

void cEvent::SetContents(const std::vector<uint8_t>& contents)
{
  if (m_contents != contents)
  {
    m_contents = contents;
    SetChanged();
  }
}

std::string cEvent::ToString(void) const
{
  std::string strVps;
  if (HasVps())
    strVps = "(VPS: " + VpsString() + ") ";
  return StartTime().GetAsDBDateTime() + " - " + EndTime().GetAsDBDateTime() + " " + strVps + " '" + Title() + "'";
}

void cEvent::FixEpgBugs(void)
{
  if (m_strTitle.empty()) {
     // we don't want any "(null)" titles
    m_strTitle = tr("No title");
  }

  if (cSettings::Get().m_iEPGBugfixLevel == 0)
    goto Final;

  // Some TV stations apparently have their own idea about how to fill in the
  // EPG data. Let's fix their bugs as good as we can:

  // Some channels put the PlotOutline in quotes and use either the PlotOutline
  // or the Plot field, depending on how long the string is:
  //
  // Title
  // "PlotOutline". Plot
  //
  if (m_strPlotOutline.empty() != !m_strPlot.empty())
  {
    std::string strCheck = !m_strPlotOutline.empty() ? m_strPlotOutline : m_strPlot;
    if (!strCheck.empty() && strCheck.at(0) == '"')
    {
      const char *delim = "\".";
      strCheck.erase(0, 1);
      size_t delimPos = strCheck.find(delim);
      if (delimPos != std::string::npos)
      {
        m_strPlot = m_strPlotOutline = strCheck;
        m_strPlot.erase(0, delimPos);
        m_strPlotOutline.erase(delimPos);
      }
    }
  }

  // Some channels put the Plot into the PlotOutline (preceded
  // by a blank) if there is no actual PlotOutline and the Plot
  // is short enough:
  //
  // Title
  //  Plot
  //
  if (!m_strPlotOutline.empty() && m_strPlot.empty())
  {
    StringUtils::Trim(m_strPlotOutline);
    m_strPlot = m_strPlotOutline;
    m_strPlotOutline.clear();
  }

  // Sometimes they repeat the Title in the PlotOutline:
  //
  // Title
  // Title
  //
  if (!m_strPlotOutline.empty() && m_strTitle == m_strPlotOutline)
    m_strPlotOutline.clear();

  // Some channels put the PlotOutline between double quotes, which is nothing
  // but annoying (some even put a '.' after the closing '"'):
  //
  // Title
  // "PlotOutline"[.]
  //
  if (!m_strPlotOutline.empty() && m_strPlotOutline.at(0) == '"')
  {
    size_t len = m_strPlotOutline.size();
    if (len > 2 &&
        (m_strPlotOutline.at(len - 1) == '"' ||
            (m_strPlotOutline.at(len - 1) == '.' && m_strPlotOutline.at(len - 2) == '"')))
    {
      m_strPlotOutline.erase(0, 1);
      const char* tmp = m_strPlotOutline.c_str();
      const char *p = strrchr(tmp, '"');
      if (p)
        m_strPlotOutline.erase(m_strPlotOutline.size() - (p - tmp));
    }
  }

  if (cSettings::Get().m_iEPGBugfixLevel <= 1)
    goto Final;

  // Some channels apparently try to do some formatting in the texts,
  // which is a bad idea because they have no way of knowing the width
  // of the window that will actually display the text.
  // Remove excess whitespace:
  StringUtils::Trim(m_strTitle);
  StringUtils::Trim(m_strPlotOutline);
  StringUtils::Trim(m_strPlot);

  // Some channels put a whole lot of information in the PlotOutline and leave
  // the Plot totally empty. So if the PlotOutline length exceeds
  // MAX_USEFUL_EPISODE_LENGTH, let's put this into the Plot
  // instead:
  if (!m_strPlotOutline.empty() && m_strPlot.empty())
  {
    if (m_strPlotOutline.size() > MAX_USEFUL_EPISODE_LENGTH)
      m_strPlot.swap(m_strPlotOutline);
  }

  // Some channels put the same information into PlotOutline and Plot.
  // In that case we delete one of them:
  if (!m_strPlotOutline.empty() && !m_strPlot.empty() && m_strPlotOutline == m_strPlot)
  {
    if (m_strPlotOutline.size() > MAX_USEFUL_EPISODE_LENGTH)
      m_strPlotOutline.clear();
    else
      m_strPlot.clear();
  }

  // Some channels use the ` ("backtick") character, where a ' (single quote)
  // would be normally used. Actually, "backticks" in normal text don't make
  // much sense, so let's replace them:
  StringUtils::Replace(m_strTitle, '`', '\'');
  StringUtils::Replace(m_strPlotOutline, '`', '\'');
  StringUtils::Replace(m_strPlot, '`', '\'');

  if (cSettings::Get().m_iEPGBugfixLevel <= 2)
    goto Final;

  // The stream components have a "plot" field which some channels
  // apparently have no idea of how to set correctly:
  if (!m_components.empty())
  {
    for (vector<CEpgComponent>::iterator itComponent = m_components.begin(); itComponent != m_components.end(); ++itComponent)
    {
      CEpgComponent& component = *itComponent;

      switch (component.Stream())
      {
      case 0x01:
        { // video
          if (!component.Description().empty())
          {
            if (StringUtils::EqualsNoCase(component.Description(), "Video") ||
                StringUtils::EqualsNoCase(component.Description(), "Bildformat"))
            {
              // Yes, we know it's video - that's what the 'stream' code
              // is for! But _which_ video is it?
              component.SetDescription("");
            }
          }
          if (component.Description().empty())
          {
            switch (component.Type())
            {
            case 0x01:
            case 0x05:
              component.SetDescription("4:3");
              break;
            case 0x02:
            case 0x03:
            case 0x06:
            case 0x07:
              component.SetDescription("16:9");
              break;
            case 0x04:
            case 0x08:
              component.SetDescription(">16:9");
              break;
            case 0x09:
            case 0x0D:
              component.SetDescription("HD 4:3");
              break;
            case 0x0A:
            case 0x0B:
            case 0x0E:
            case 0x0F:
              component.SetDescription("HD 16:9");
              break;
            case 0x0C:
            case 0x10:
              component.SetDescription("HD >16:9");
              break;
            default:
              break;
            }
          }
        }
        break;
      case 0x02:
        { // audio
          if (!component.Description().empty())
          {
            if (StringUtils::EqualsNoCase(component.Description(), "Audio"))
            {
              // Yes, we know it's audio - that's what the 'stream' code
              // is for! But _which_ audio is it?
              component.SetDescription("");
            }
          }
          if (component.Description().empty())
          {
            switch (component.Type())
            {
            case 0x05:
              component.SetDescription("Dolby Digital");
              break;
            default:
              break; // all others will just display the language
            }
          }
        }
        break;
      default:
        break;
      }
    }
  }

Final:
  return;
}

void AddEventElement(TiXmlElement* eventElement, const std::string& strElement, const std::string& strText)
{
  TiXmlElement textElement(strElement);
  TiXmlNode* textNode = eventElement->InsertEndChild(textElement);
  if (textNode)
  {
    TiXmlText* text = new TiXmlText(StringUtils::Format("%s", strText.c_str()));
    textNode->LinkEndChild(text);
  }
}

bool cEvent::Serialise(TiXmlNode* node) const
{
  if (node == NULL)
    return false;

  TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  elem->SetAttribute(EPG_XML_ATTR_EVENT_ID, m_eventID);

  if (m_atscSourceId != 0)
    elem->SetAttribute(EPG_XML_ATTR_ATSC_SOURCE_ID, m_atscSourceId);

  if (!m_strTitle.empty())
    AddEventElement(elem, EPG_XML_ELM_TITLE, m_strTitle);
  if (!m_strPlotOutline.empty())
    AddEventElement(elem, EPG_XML_ELM_PLOT_OUTLINE, m_strPlotOutline);
  if (!m_strPlot.empty())
    AddEventElement(elem, EPG_XML_ELM_PLOT, m_strPlot);

  if (!m_channelID.Serialise(node))
    return false;

  std::string strGenre = EPGStringifier::GenreToString(m_genreType);
  if (!strGenre.empty())
    elem->SetAttribute(EPG_XML_ATTR_GENRE, strGenre.c_str());

  std::string strSubGenre = EPGStringifier::SubGenreToString(m_genreType, m_genreSubType);
  if (!strSubGenre.empty())
    elem->SetAttribute(EPG_XML_ATTR_SUBGENRE, strSubGenre.c_str());

  if (m_genreType == EPG_GENRE_CUSTOM && !m_strCustomGenre.empty())
    elem->SetAttribute(EPG_XML_ATTR_CUSTOM_GENRE, m_strCustomGenre.c_str());

  time_t tmStart;
  m_startTime.GetAsTime(tmStart);
  elem->SetAttribute(EPG_XML_ATTR_START_TIME, tmStart);

  time_t tmEnd;
  m_endTime.GetAsTime(tmEnd);
  elem->SetAttribute(EPG_XML_ATTR_END_TIME, tmEnd);

  if (m_parentalRating)
    elem->SetAttribute(EPG_XML_ATTR_PARENTAL, m_parentalRating);

  if (m_starRating)
    elem->SetAttribute(EPG_XML_ATTR_STAR, m_starRating);

  if (m_tableID != TABLE_ID_INVALID)
    elem->SetAttribute(EPG_XML_ATTR_STAR, m_tableID);

  if (m_version != VERSION_INVALID)
    elem->SetAttribute(EPG_XML_ATTR_STAR, m_version);

  if (m_vps.IsValid())
  {
    time_t tmVps;
    m_vps.GetAsTime(tmVps);
    elem->SetAttribute(EPG_XML_ATTR_VPS, tmVps);
  }

  if (!m_components.empty())
  {
    TiXmlElement componentsElement(EPG_XML_ELM_COMPONENTS);
    TiXmlNode* componentsNode = elem->InsertEndChild(componentsElement);
    if (componentsNode)
    {
      for (vector<CEpgComponent>::const_iterator itComponent = m_components.begin(); itComponent != m_components.end(); ++itComponent)
      {
        TiXmlElement componentElement(EPG_XML_ELM_COMPONENT);
        TiXmlNode* componentNode = componentsNode->InsertEndChild(componentElement);
        if (componentNode)
          itComponent->Serialise(componentNode);
      }
    }
  }

  if (!m_contents.empty())
  {
    for (std::vector<uint8_t>::const_iterator it = m_contents.begin(); it != m_contents.end(); ++it)
    {
      TiXmlElement contentsElement(EPG_XML_ELM_CONTENTS);
      TiXmlNode* contentsNode = elem->InsertEndChild(contentsElement);
      if (contentsNode)
      {
        TiXmlElement* contentsElem = contentsNode->ToElement();
        if (contentsElem)
        {
          TiXmlText* text = new TiXmlText(StringUtils::Format("%u", *it));
          if (text)
            contentsElem->LinkEndChild(text);
        }
      }
    }
  }

  return true;
}

bool cEvent::Deserialise(EventPtr& event, const TiXmlNode *eventNode)
{
  if (eventNode == NULL)
    return false;

  const TiXmlElement* elem = eventNode->ToElement();
  if (elem == NULL)
    return false;

  const char* strID = elem->Attribute(EPG_XML_ATTR_EVENT_ID);
  if (strID != NULL)
  {
    event = EventPtr(new cEvent(StringUtils::IntVal(strID)));
    return event->Deserialise(eventNode);
  }

  return false;
}

bool cEvent::Deserialise(const TiXmlNode* node)
{
  if (node == NULL)
    return false;

  const TiXmlElement* elem = node->ToElement();
  if (elem == NULL)
    return false;

  Reset();

  const char* atscSourceId  = elem->Attribute(EPG_XML_ATTR_ATSC_SOURCE_ID);
  if (atscSourceId)
    m_atscSourceId = StringUtils::IntVal(atscSourceId);

  const TiXmlNode* titleNode = elem->FirstChild(EPG_XML_ELM_TITLE);
  if (titleNode != NULL)
    m_strTitle = titleNode->ToElement()->GetText();

  const TiXmlNode* plotOutlineNode = elem->FirstChild(EPG_XML_ELM_PLOT_OUTLINE);
  if (plotOutlineNode != NULL)
    m_strPlotOutline = plotOutlineNode->ToElement()->GetText();

  const TiXmlNode* plotNode = elem->FirstChild(EPG_XML_ELM_PLOT);
  if (plotNode)
    m_strPlot = plotNode->ToElement()->GetText();

  if (!m_channelID.Deserialise(node))
    return false;

  const char* strStart = elem->Attribute(EPG_XML_ATTR_START_TIME);
  if (strStart != NULL)
    m_startTime = CDateTime((time_t)StringUtils::IntVal(strStart));

  const char* strEnd = elem->Attribute(EPG_XML_ATTR_END_TIME);
  if (strEnd != NULL)
    m_startTime = CDateTime((time_t)StringUtils::IntVal(strEnd));

  const char* strGenre = elem->Attribute(EPG_XML_ATTR_GENRE);
  if (strGenre != NULL)
    m_genreType = EPGStringifier::StringToGenre(strGenre);

  if (m_genreType == EPG_GENRE_CUSTOM)
  {
    const char* strCustomGenre = elem->Attribute(EPG_XML_ATTR_CUSTOM_GENRE);
    if (strCustomGenre != NULL)
      m_strCustomGenre = strCustomGenre;
  }
  else
  {
    const char* strSubGenre = elem->Attribute(EPG_XML_ATTR_SUBGENRE);
    if (strSubGenre != NULL)
      m_genreSubType = EPGStringifier::StringToSubGenre(strSubGenre);
  }

  const char* parental  = elem->Attribute(EPG_XML_ATTR_PARENTAL);
  if (parental)
    m_parentalRating = StringUtils::IntVal(parental);

  const char* starRating  = elem->Attribute(EPG_XML_ATTR_STAR);
  if (starRating)
    m_starRating = StringUtils::IntVal(starRating);

  const char* strTableID = elem->Attribute(EPG_XML_ATTR_TABLE_ID);
  if (strTableID != NULL)
    m_tableID = StringUtils::IntVal(strTableID);

  const char* strVersion = elem->Attribute(EPG_XML_ATTR_VERSION);
  if (strVersion != NULL)
    m_version = StringUtils::IntVal(strVersion);

  const char* strVps  = elem->Attribute(EPG_XML_ATTR_VPS);
  if (strVps)
    m_vps = CDateTime((time_t)StringUtils::IntVal(strVps));

  const TiXmlNode* componentsNode = elem->FirstChild(EPG_XML_ELM_COMPONENTS);
  if (componentsNode)
  {
    const TiXmlNode* componentNode = componentsNode->FirstChild(EPG_XML_ELM_COMPONENTS);
    while (componentNode != NULL)
    {
      const char* num  = componentNode->ToElement()->Attribute(EPG_XML_ATTR_COMPONENT_ID);
      if (num)
      {
        CEpgComponent component;
        if (component.Deserialise(componentNode))
          m_components[StringUtils::IntVal(num)] = component;
      }
      componentNode = componentNode->NextSibling(EPG_XML_ELM_COMPONENT);
    }
  }

  const TiXmlNode* contentsNode = elem->FirstChild(EPG_XML_ELM_CONTENTS);
  while (contentsNode != NULL)
  {
    m_contents.push_back(StringUtils::IntVal(contentsNode->ToElement()->GetText()));
    contentsNode = contentsNode->NextSibling(EPG_XML_ELM_CONTENTS);
  }

  return true;
}

}
