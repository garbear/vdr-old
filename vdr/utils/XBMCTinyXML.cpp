/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "XBMCTinyXML.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/CharsetDetection.h"
//#include "LangInfo.h"
#include "RegExp.h"
//#include "utils/log.h"

#include <vector>

using namespace std;

namespace VDR
{

#define MAX_ENTITY_LENGTH 8 // size of largest entity "&#xNNNN;"
#define BUFFER_SIZE 4096

CXBMCTinyXML::CXBMCTinyXML()
: TiXmlDocument()
{
}

CXBMCTinyXML::CXBMCTinyXML(const char *documentName)
: TiXmlDocument(documentName)
{
}

CXBMCTinyXML::CXBMCTinyXML(const std::string& documentName)
: TiXmlDocument(documentName)
{
}

CXBMCTinyXML::CXBMCTinyXML(const std::string& documentName, const std::string& documentCharset)
: TiXmlDocument(documentName), m_SuggestedCharset(documentCharset)
{
  StringUtils::ToUpper(m_SuggestedCharset);
}

bool CXBMCTinyXML::LoadFile(TiXmlEncoding encoding)
{
  return LoadFile(value, encoding);
}

bool CXBMCTinyXML::LoadFile(const char *_filename, TiXmlEncoding encoding)
{
  return LoadFile(std::string(_filename), encoding);
}

bool CXBMCTinyXML::LoadFile(const std::string& _filename, TiXmlEncoding encoding)
{
  value = _filename.c_str();

  CFile file;
  vector<uint8_t> buffer;

  if (!file.LoadFile(value, buffer))
  {
    SetError(TIXML_ERROR_OPENING_FILE, NULL, NULL, TIXML_ENCODING_UNKNOWN);
    return false;
  }

  // Delete the existing data:
  Clear();
  location.Clear();

  std::string data(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  buffer.clear(); // free memory early

  if (encoding == TIXML_ENCODING_UNKNOWN)
    Parse(data, file.GetContentCharset());
  else
    Parse(data, encoding);

  if (Error())
    return false;
  return true;
}

bool CXBMCTinyXML::LoadFile(const std::string& _filename, const std::string& documentCharset)
{
  m_SuggestedCharset = documentCharset;
  StringUtils::ToUpper(m_SuggestedCharset);
  return LoadFile(_filename, TIXML_ENCODING_UNKNOWN);
}

bool CXBMCTinyXML::LoadFile(FILE *f, TiXmlEncoding encoding)
{
  std::string data;
  char buf[BUFFER_SIZE] = {0};
  int result;
  while ((result = fread(buf, 1, BUFFER_SIZE, f)) > 0)
    data.append(buf, result);
  return Parse(data, encoding);
}

bool CXBMCTinyXML::SaveFile(const char *_filename) const
{
  return SaveFile(std::string(_filename));
}

bool CXBMCTinyXML::SaveFile(const std::string& filename) const
{
  CFile file;
  if (file.OpenForWrite(filename, true))
  {
    TiXmlPrinter printer;
    Accept(&printer);
    file.Write(printer.CStr(), printer.Size());
    return true;
  }
  return false;
}

bool CXBMCTinyXML::SafeSaveFile(const std::string& strFilename) const
{
  std::string strTempFile = strFilename + ".tmp";
  if (SaveFile(strTempFile))
  {
    CFile::Delete(strFilename);
    CFile::Rename(strTempFile, strFilename);
    return true;
  }
  else
  {
    CFile::Delete(strTempFile);
    return false;
  }
}

bool CXBMCTinyXML::Parse(const char *_data, TiXmlEncoding encoding)
{
  return Parse(std::string(_data), encoding);
}

bool CXBMCTinyXML::Parse(const std::string& data, const std::string& dataCharset)
{
  m_SuggestedCharset = dataCharset;
  StringUtils::ToUpper(m_SuggestedCharset);
  return Parse(data, TIXML_ENCODING_UNKNOWN);
}

bool CXBMCTinyXML::Parse(const std::string& data, TiXmlEncoding encoding /*= TIXML_DEFAULT_ENCODING */)
{
  m_UsedCharset.clear();
  if (encoding != TIXML_ENCODING_UNKNOWN)
  { // encoding != TIXML_ENCODING_UNKNOWN means "do not use m_SuggestedCharset and charset detection"
    m_SuggestedCharset.clear();
    if (encoding == TIXML_ENCODING_UTF8)
      m_UsedCharset = "UTF-8";

    return InternalParse(data, encoding);
  }

  if (!m_SuggestedCharset.empty() && TryParse(data, m_SuggestedCharset))
    return true;

  std::string detectedCharset;
  if (CCharsetDetection::DetectXmlEncoding(data, detectedCharset) && TryParse(data, detectedCharset))
    return true;

  // check for valid UTF-8
  if (m_SuggestedCharset != "UTF-8" && detectedCharset != "UTF-8" && CCharsetConverter::Get().isValidUtf8(data) &&
      TryParse(data, "UTF-8"))
      return true;

  /*
  // fallback: try user GUI charset
  if (TryParse(data, g_langInfo.GetGuiCharSet()))
    return true;
  */

  // can't detect correct data charset, try to process data as is
  return InternalParse(data, TIXML_ENCODING_UNKNOWN);
}

bool CXBMCTinyXML::TryParse(const std::string& data, const std::string& tryDataCharset)
{
  if (tryDataCharset == "UTF-8")
    InternalParse(data, TIXML_ENCODING_UTF8); // process data without conversion
  else if (!tryDataCharset.empty())
  {
    std::string converted;
    if (!CCharsetConverter::Get().ToUtf8(tryDataCharset, data, converted) || converted.empty())
      return false; // can't convert data

    InternalParse(converted, TIXML_ENCODING_UTF8);
  }
  else
    InternalParse(data, TIXML_ENCODING_LEGACY);

  // 'Error()' contains result of last run of 'TiXmlDocument::Parse()'
  if (Error())
  {
    Clear();
    location.Clear();

    return false;
  }

  m_UsedCharset = tryDataCharset;
  if (!m_SuggestedCharset.empty() && m_UsedCharset != m_SuggestedCharset)
    ;/*CLog::Log(LOGWARNING, "%s: Using \"%s\" charset instead of \"%s\" charset%s", __FUNCTION__, m_UsedCharset.c_str(), m_SuggestedCharset.c_str(),
                (value.empty() ? "" : (" for file " + value).c_str()));*/ // TODO

  return true;
}

bool CXBMCTinyXML::InternalParse(const std::string& rawdata, TiXmlEncoding encoding /*= TIXML_DEFAULT_ENCODING */)
{
  // Preprocess string, replacing '&' with '&amp; for invalid XML entities
  size_t pos = rawdata.find('&');
  if (pos == std::string::npos)
    return (TiXmlDocument::Parse(rawdata.c_str(), NULL, encoding) != NULL); // nothing to fix, process data directly

  std::string data(rawdata);
#if defined(HAVE_PCRE)
  CRegExp re(false, false, "^&(amp|lt|gt|quot|apos|#x[a-fA-F0-9]{1,4}|#[0-9]{1,5});.*");
  do
  {
    if (re.RegFind(data, pos, MAX_ENTITY_LENGTH) < 0)
      data.insert(pos + 1, "amp;");
    pos = data.find('&', pos + 1);
  } while (pos != std::string::npos);
#endif

  return (TiXmlDocument::Parse(data.c_str(), NULL, encoding) != NULL);
}

bool CXBMCTinyXML::Test()
{
  // scraper results with unescaped &
  CXBMCTinyXML doc;
  std::string data("<details><url function=\"ParseTMDBRating\" "
                  "cache=\"tmdb-en-12244.json\">"
                  "http://api.themoviedb.org/3/movie/12244"
                  "?api_key=57983e31fb435df4df77afb854740ea9"
                  "&language=en&#x3f;&#x003F;&#0063;</url></details>");
  doc.Parse(data.c_str());
  TiXmlNode *root = doc.RootElement();
  if (root && root->ValueStr() == "details")
  {
    TiXmlElement *url = root->FirstChildElement("url");
    if (url && url->FirstChild())
    {
      return (url->FirstChild()->ValueStr() == "http://api.themoviedb.org/3/movie/12244?api_key=57983e31fb435df4df77afb854740ea9&language=en???");
    }
  }
  return false;
}

}
