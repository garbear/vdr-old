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
#pragma once

#include "UrlOptions.h"

#ifdef TARGET_WINDOWS
#undef SetPort // WIN32INCLUDES this is defined as SetPortA in WinSpool.h which is being included _somewhere_
#endif

namespace VDR
{
class CURL
{
public:
  CURL();
  CURL(const std::string& strURL);
  virtual ~CURL(void) { }

  // Setters
  void Reset();
  void Parse(const std::string& strURL);
  void SetFileName(const std::string& strFileName);
  void SetHostName(const std::string& strHostName) { m_strHostName = strHostName; }
  void SetUserName(const std::string& strUserName) { m_strUserName = strUserName; }
  void SetPassword(const std::string& strPassword) { m_strPassword = strPassword; }
  void SetProtocol(const std::string& strProtocol); // Makes lowercase
  void SetPort(int port)                           { m_iPort = port; }
  void SetOption(const std::string &key, const std::string &value);        // m_options and m_strOptions are kept in sync
  void SetProtocolOption(const std::string &key, const std::string &value);// m_protocolOptions and m_strProtocolOptions are kept in sync
  void SetOptions(const std::string& strOptions);                          // m_options and m_strOptions are kept in sync
  void SetProtocolOptions(const std::string& strOptions);                  // m_protocolOptions and m_strProtocolOptions are kept in sync
  void RemoveOption(const std::string &key);
  void RemoveProtocolOption(const std::string &key);

  // Getters
  const std::string& GetHostName()        const { return m_strHostName; }
  const std::string& GetDomain()          const { return m_strDomain; }
  const std::string& GetUserName()        const { return m_strUserName; }
  const std::string& GetPassWord()        const { return m_strPassword; }
  const std::string& GetFileName()        const { return m_strFileName; }
  const std::string& GetProtocol()        const { return m_strProtocol; }
  const std::string& GetFileType()        const { return m_strFileType; }
  const std::string& GetShareName()       const { return m_strShareName; }
  const std::string& GetOptions()         const { return m_strOptions; }
  const std::string& GetProtocolOptions() const { return m_strProtocolOptions; }
  int GetPort()                           const { return m_iPort; }
  bool GetOption(const std::string &key, std::string &value) const;
  bool GetProtocolOption(const std::string &key, std::string &value) const;
  std::string GetOption(const std::string &key) const;
  std::string GetProtocolOption(const std::string &key) const;
  void GetOptions(std::map<std::string, std::string> &options) const;
  void GetProtocolOptions(std::map<std::string, std::string> &options) const;
  bool HasOption(const std::string &key) const;
  bool HasProtocolOption(const std::string &key) const;
  bool HasPort() const { return (m_iPort != 0); }

  // Methods that derive
  std::string Get() const;
  std::string GetTranslatedProtocol() const;
  std::string GetFileNameWithoutPath() const; // Return the filename excluding path
  std::string GetWithoutUserDetails(bool redact = false) const;
  std::string GetWithoutFilename() const;
  std::string GetRedacted() const;
  bool IsLocal() const;
  bool IsLocalHost() const;

  char GetDirectorySeparator();

protected:
  int m_iPort;
  std::string m_strHostName;
  std::string m_strShareName;
  std::string m_strDomain;
  std::string m_strUserName;
  std::string m_strPassword;
  std::string m_strFileName;
  std::string m_strProtocol;
  std::string m_strFileType;
  std::string m_strOptions;
  std::string m_strProtocolOptions;
  CUrlOptions m_options;
  CUrlOptions m_protocolOptions;
};
}
