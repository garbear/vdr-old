/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "URL.h"
#include "URLUtils.h"
#include "filesystem/File.h"
#include "platform/os.h"
#include "utils/StringUtils.h"
//#include "utils/RegExp.h"
//#include "utils/log.h"
//#include "Util.h"
//#include "FileItem.h"
//#include "filesystem/StackDirectory.h"
//#include "addons/Addon.h"
//#include "utils/StringUtils.h"
//#ifndef TARGET_POSIX
//#include <sys\types.h>
//#include <sys/stat.h>
//#endif

using namespace std;

namespace VDR
{

string URLEncodeInline(const string& strData)
{
  string buffer = strData;
  URLUtils::Encode(buffer);
  return buffer;
}

CURL::CURL()
 : m_iPort(0)
{
}

CURL::CURL(const string& strURL1)
{
  Parse(strURL1);
}

void CURL::Reset()
{
  m_strHostName.clear();
  m_strDomain.clear();
  m_strUserName.clear();
  m_strPassword.clear();
  m_strShareName.clear();
  m_strFileName.clear();
  m_strProtocol.clear();
  m_strFileType.clear();
  m_strOptions.clear();
  m_strProtocolOptions.clear();
  m_options.Clear();
  m_protocolOptions.Clear();
  m_iPort = 0;
}

void CURL::Parse(const string& strURL1)
{
  Reset();
  // start by validating the path
  string strURL = URLUtils::ValidatePath(strURL1);

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (!strURL.length()) return;
  if (strURL == "?") return;

  // form is format 1 or 2
  // format 1: protocol://[domain;][username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file

  // decode protocol
  size_t iPos = strURL.find("://");
  if (iPos == std::string::npos)
  {
    // This is an ugly hack that needs some work.
    // example: filename /foo/bar.zip/alice.rar/bob.avi
    // This should turn into zip://rar:///foo/bar.zip/alice.rar/bob.avi
    iPos = 0;
    bool is_apk = (strURL.find(".apk/", iPos) != std::string::npos);
    while (1)
    {
      if (is_apk)
        iPos = strURL.find(".apk/", iPos);
      else
        iPos = strURL.find(".zip/", iPos);

      int extLen = 3;
      if (iPos == std::string::npos)
      {
        /* set filename and update extension*/
        SetFileName(strURL);
        return ;
      }
      iPos += extLen + 1;
      std::string archiveName = strURL.substr(0, iPos);
      struct __stat64 s;
      struct stat64 *ps = &s;
      if (true)//CFile::Stat(archiveName, ps) == 0) // TODO
      {
#ifdef TARGET_POSIX
        if (!S_ISDIR(s.st_mode))
#else
        if (!(s.st_mode & S_IFDIR))
#endif
        {
          URLUtils::Encode(archiveName);
          if (is_apk)
          {
            CURL c("apk://" + archiveName + "/" + strURL.substr(iPos + 1));
            *this = c;
          }
          else
          {
            CURL c("zip://" + archiveName + "/" + strURL.substr(iPos + 1));
            *this = c;
          }
          return;
        }
      }
    }
  }
  else
  {
    SetProtocol(strURL.substr(0, iPos));
    iPos += 3;
  }

  // virtual protocols
  // why not handle all format 2 (protocol://file) style urls here?
  // ones that come to mind are iso9660, cdda, musicdb, etc.
  // they are all local protocols and have no server part, port number, special options, etc.
  // this removes the need for special handling below.
  if (StringUtils::EqualsNoCase(m_strProtocol, "stack") ||
      StringUtils::EqualsNoCase(m_strProtocol, "virtualpath") ||
      StringUtils::EqualsNoCase(m_strProtocol, "multipath") ||
      StringUtils::EqualsNoCase(m_strProtocol, "filereader") ||
      StringUtils::EqualsNoCase(m_strProtocol, "special"))
  {
    SetFileName(strURL.substr(iPos));
    return;
  }

  // check for username/password - should occur before first /
  if (iPos == std::string::npos) iPos = 0;

  // for protocols supporting options, chop that part off here
  // maybe we should invert this list instead?
  size_t iEnd = strURL.length();
  const char* sep = NULL;

  //TODO fix all Addon paths
  string strProtocol2 = GetTranslatedProtocol();
  if (StringUtils::EqualsNoCase(m_strProtocol, "rss") ||
      StringUtils::EqualsNoCase(m_strProtocol, "rar") ||
      StringUtils::EqualsNoCase(m_strProtocol, "addons") ||
      StringUtils::EqualsNoCase(m_strProtocol, "image") ||
      StringUtils::EqualsNoCase(m_strProtocol, "videodb") ||
      StringUtils::EqualsNoCase(m_strProtocol, "musicdb") ||
      StringUtils::EqualsNoCase(m_strProtocol, "androidapp"))
    sep = "?";
  else
  if (StringUtils::EqualsNoCase(strProtocol2, "http") ||
      StringUtils::EqualsNoCase(strProtocol2, "https") ||
      StringUtils::EqualsNoCase(strProtocol2, "plugin") ||
      StringUtils::EqualsNoCase(strProtocol2, "addons") ||
      StringUtils::EqualsNoCase(strProtocol2, "hdhomerun") ||
      StringUtils::EqualsNoCase(strProtocol2, "rtsp") ||
      StringUtils::EqualsNoCase(strProtocol2, "apk") ||
      StringUtils::EqualsNoCase(strProtocol2, "zip"))
    sep = "?;#|";
  else if (StringUtils::EqualsNoCase(strProtocol2, "ftp") ||
           StringUtils::EqualsNoCase(strProtocol2, "ftps"))
    sep = "?;|";

  if(sep)
  {
    size_t iOptions = strURL.find_first_of(sep, iPos);
    if (iOptions != std::string::npos)
    {
      // we keep the initial char as it can be any of the above
      size_t iProto = strURL.find_first_of("|",iOptions);
      if (iProto != std::string::npos)
      {
        SetProtocolOptions(strURL.substr(iProto+1));
        SetOptions(strURL.substr(iOptions,iProto-iOptions));
      }
      else
        SetOptions(strURL.substr(iOptions));
      iEnd = iOptions;
    }
  }

  size_t iSlash = strURL.find("/", iPos);
  if(iSlash >= iEnd)
    iSlash = std::string::npos; // was an invalid slash as it was contained in options

  if( !StringUtils::EqualsNoCase(m_strProtocol, "iso9660") )
  {
    size_t iAlphaSign = strURL.find("@", iPos);
    if (iAlphaSign != std::string::npos && iAlphaSign < iEnd && (iAlphaSign < iSlash || iSlash == std::string::npos))
    {
      // username/password found
      string strUserNamePassword = strURL.substr(iPos, iAlphaSign - iPos);

      // first extract domain, if protocol is smb
      if (StringUtils::EqualsNoCase(m_strProtocol, "smb"))
      {
        size_t iSemiColon = strUserNamePassword.find(";");

        if (iSemiColon != std::string::npos)
        {
          m_strDomain = strUserNamePassword.substr(0, iSemiColon);
          strUserNamePassword.erase(0, iSemiColon + 1);
        }
      }

      // username:password
      size_t iColon = strUserNamePassword.find(":");
      if (iColon != std::string::npos)
      {
        m_strUserName = strUserNamePassword.substr(0, iColon);
        m_strPassword = strUserNamePassword.substr(iColon + 1);
      }
      // username
      else
      {
        m_strUserName = strUserNamePassword;
      }

      iPos = iAlphaSign + 1;
      iSlash = strURL.find("/", iAlphaSign);

      if(iSlash >= iEnd)
        iSlash = std::string::npos;
    }
  }

  // detect hostname:port/
  if (iSlash == std::string::npos)
  {
    string strHostNameAndPort = strURL.substr(iPos, iEnd - iPos);
    size_t iColon = strHostNameAndPort.find(":");
    if (iColon != std::string::npos)
    {
      m_strHostName = strHostNameAndPort.substr(0, iColon);
      m_iPort = StringUtils::IntVal(strHostNameAndPort.substr(iColon + 1));
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }

  }
  else
  {
    string strHostNameAndPort = strURL.substr(iPos, iSlash - iPos);
    size_t iColon = strHostNameAndPort.find(":");
    if (iColon != std::string::npos)
    {
      m_strHostName = strHostNameAndPort.substr(0, iColon);
      m_iPort = StringUtils::IntVal(strHostNameAndPort.substr(iColon + 1));
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }
    iPos = iSlash + 1;
    if (iEnd > iPos)
    {
      m_strFileName = strURL.substr(iPos, iEnd - iPos);

      iSlash = m_strFileName.find("/");
      if(iSlash == std::string::npos)
        m_strShareName = m_strFileName;
      else
        m_strShareName = m_strFileName.substr(0, iSlash);
    }
  }

  // iso9960 doesnt have an hostname;-)
  if (m_strProtocol == "iso9660"
    || m_strProtocol == "musicdb"
    || m_strProtocol == "videodb"
    || m_strProtocol == "sources"
    || m_strProtocol == "pvr"
    || StringUtils::StartsWith(m_strProtocol, "mem"))
  {
    if (m_strHostName != "" && m_strFileName != "")
    {
      m_strFileName = StringUtils::Format("%s/%s", m_strHostName.c_str(), m_strFileName.c_str());
      m_strHostName = "";
    }
    else
    {
      if (!m_strHostName.empty() && strURL[iEnd-1]=='/')
        m_strFileName = m_strHostName + "/";
      else
        m_strFileName = m_strHostName;
      m_strHostName = "";
    }
  }

  StringUtils::Replace(m_strFileName, '\\', '/');

  /* update extension */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  if(URLUtils::ProtocolHasEncodedHostname(m_strProtocol))
  {
    URLUtils::Decode(m_strHostName);
    SetHostName(m_strHostName);
  }

  URLUtils::Decode(m_strUserName);
  URLUtils::Decode(m_strPassword);
}

void CURL::SetFileName(const string& strFileName)
{
  m_strFileName = strFileName;

  int slash = m_strFileName.find_last_of(GetDirectorySeparator());
  int period = m_strFileName.find_last_of('.');
  if(period != -1 && (slash == -1 || period > slash))
    m_strFileType = m_strFileName.substr(period+1);
  else
    m_strFileType = "";

  StringUtils::Trim(m_strFileType);
  StringUtils::ToLower(m_strFileType);

}

void CURL::SetProtocol(const string& strProtocol)
{
  m_strProtocol = strProtocol;
  StringUtils::ToLower(m_strProtocol);
}

void CURL::SetOptions(const string& strOptions)
{
  m_strOptions.clear();
  m_options.Clear();
  if( strOptions.length() > 0)
  {
    if(strOptions[0] == '?' ||
       strOptions[0] == '#' ||
       strOptions[0] == ';' ||
       strOptions.find("xml") != std::string::npos)
    {
      m_strOptions = strOptions;
      m_options.AddOptions(m_strOptions);
    }
    else
    {
      //CLog::Log(LOGWARNING, "%s - Invalid options specified for url %s", __FUNCTION__, strOptions.c_str()); // TODO
    }
  }
}

void CURL::SetProtocolOptions(const string& strOptions)
{
  m_strProtocolOptions.clear();
  m_protocolOptions.Clear();
  if (strOptions.length() > 0)
  {
    if (strOptions[0] == '|')
      m_strProtocolOptions = strOptions.substr(1);
    else
      m_strProtocolOptions = strOptions;
    m_protocolOptions.AddOptions(m_strProtocolOptions);
  }
}

void CURL::SetOption(const string &key, const string &value)
{
  m_options.AddOption(key, value);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::RemoveOption(const string &key)
{
  m_options.RemoveOption(key);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::SetProtocolOption(const string &key, const string &value)
{
  m_protocolOptions.AddOption(key, value);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}

void CURL::RemoveProtocolOption(const string &key)
{
  m_protocolOptions.RemoveOption(key);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}

bool CURL::GetOption(const string &key, string &value) const
{
  CUrlOptions::UrlOptionValue_t valueObj;
  if (!m_options.GetOption(key, valueObj))
    return false;

#if USE_STD_STRING_INSTEAD_OF_CVARIANT
  value = valueObj;
#else
  value = valueObj.asString();
#endif

  return true;
}

bool CURL::GetProtocolOption(const string &key, string &value) const
{
  CUrlOptions::UrlOptionValue_t valueObj;
  if (!m_protocolOptions.GetOption(key, valueObj))
    return false;

#if USE_STD_STRING_INSTEAD_OF_CVARIANT
  value = valueObj;
#else
  value = valueObj.asString();
#endif

  return true;
}

string CURL::GetOption(const string &key) const
{
  string value;
  if (!GetOption(key, value))
    return "";

  return value;
}

string CURL::GetProtocolOption(const string &key) const
{
  string value;
  if (!GetProtocolOption(key, value))
    return "";

  return value;
}

void CURL::GetOptions(std::map<string, string> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_options.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin(); option != optionsMap.end(); option++)
  {

#if USE_STD_STRING_INSTEAD_OF_CVARIANT
    options[option->first] = option->second;
#else
    options[option->first] = option->second.asString();
#endif

  }
}

void CURL::GetProtocolOptions(std::map<string, string> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_protocolOptions.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin(); option != optionsMap.end(); option++)

#if USE_STD_STRING_INSTEAD_OF_CVARIANT
    options[option->first] = option->second;
#else
    options[option->first] = option->second.asString();
#endif

}

bool CURL::HasOption(const string &key) const
{
  return m_options.HasOption(key);
}

bool CURL::HasProtocolOption(const string &key) const
{
  return m_protocolOptions.HasOption(key);
}

string CURL::Get() const
{
  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (m_strProtocol == "")
    return m_strFileName;

  string strURL;
  strURL.reserve(sizeneed);

  strURL = GetWithoutFilename();
  strURL += m_strFileName;

  if( !m_strOptions.empty() )
    strURL += m_strOptions;
  if (!m_strProtocolOptions.empty())
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

string CURL::GetTranslatedProtocol() const
{
  return URLUtils::TranslateProtocol(m_strProtocol);
}

string CURL::GetFileNameWithoutPath() const
{
  // *.zip and *.rar store the actual zip/rar path in the hostname of the url
  if ((m_strProtocol == "rar"  ||
       m_strProtocol == "zip"  ||
       m_strProtocol == "apk") &&
       m_strFileName.empty())
    return URLUtils::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  string file(m_strFileName);
  URLUtils::RemoveSlashAtEnd(file);
  return URLUtils::GetFileName(file);
}

std::string CURL::GetWithoutUserDetails(bool redact) const
{
  std::string strURL;

  if (StringUtils::EqualsNoCase(m_strProtocol, "stack"))
  {
    // stack:// protocol not implemented
    /*
    CFileItemList items;
    std::string strURL2;
    strURL2 = Get();
    XFILE::CStackDirectory dir;
    dir.GetDirectory(strURL2,items);
    vector<std::string> newItems;
    for (int i=0;i<items.Size();++i)
    {
      CURL url(items[i]->GetPath());
      items[i]->SetPath(url.GetWithoutUserDetails(redact));
      newItems.push_back(items[i]->GetPath());
    }
    dir.ConstructStackPath(newItems, strURL);
    return strURL;
    */
  }

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (redact)
    sizeneed += sizeof("USERNAME:PASSWORD@");

  strURL.reserve(sizeneed);

  if (m_strProtocol == "")
    return m_strFileName;

  strURL = m_strProtocol;
  strURL += "://";

  if (redact && !m_strUserName.empty())
  {
    strURL += "USERNAME";
    if (!m_strPassword.empty())
    {
      strURL += ":PASSWORD";
    }
    strURL += "@";
  }

  if (!m_strHostName.empty())
  {
    std::string strHostName;

    if (URLUtils::ProtocolHasParentInHostname(m_strProtocol))
      strHostName = CURL(m_strHostName).GetWithoutUserDetails();
    else
      strHostName = m_strHostName;

    if (URLUtils::ProtocolHasEncodedHostname(m_strProtocol))
      strURL += URLEncodeInline(strHostName);
    else
      strURL += strHostName;

    if ( HasPort() )
    {
      strURL += StringUtils::Format(":%i", m_iPort);
    }
    strURL += "/";
  }
  strURL += m_strFileName;

  if( m_strOptions.length() > 0 )
    strURL += m_strOptions;
  if( m_strProtocolOptions.length() > 0 )
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

string CURL::GetWithoutFilename() const
{
  if (m_strProtocol == "")
    return "";

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + 10;

  string strURL;
  strURL.reserve(sizeneed);

  strURL = m_strProtocol;
  strURL += "://";

  if (m_strDomain != "")
  {
    strURL += m_strDomain;
    strURL += ";";
  }
  else if (m_strUserName != "")
  {
    strURL += URLEncodeInline(m_strUserName);
    if (m_strPassword != "")
    {
      strURL += ":";
      strURL += URLEncodeInline(m_strPassword);
    }
    strURL += "@";
  }
  else if (m_strDomain != "")
    strURL += "@";

  if (m_strHostName != "")
  {
    if( URLUtils::ProtocolHasEncodedHostname(m_strProtocol) )
      strURL += URLEncodeInline(m_strHostName);
    else
      strURL += m_strHostName;
    if (HasPort())
    {
      string strPort = StringUtils::Format("%i", m_iPort);
      strURL += ":";
      strURL += strPort;
    }
    strURL += "/";
  }

  return strURL;
}

std::string CURL::GetRedacted() const
{
  return GetWithoutUserDetails(true);
}

bool CURL::IsLocal() const
{
  return (IsLocalHost() || m_strProtocol.empty());
}

bool CURL::IsLocalHost() const
{
  return (StringUtils::EqualsNoCase(m_strHostName, "localhost") || StringUtils::EqualsNoCase(m_strHostName, "127.0.0.1"));
}

char CURL::GetDirectorySeparator()
{
#ifndef TARGET_POSIX
  if (IsLocal())
    return '\\';
  else
#endif
    return '/';
}

}
