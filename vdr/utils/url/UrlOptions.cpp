/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "UrlOptions.h"
//#include "log.h" // TODO
#include "URL.h"
#include "URLUtils.h"
#include "utils/StringUtils.h"

#include <sstream>

using namespace std;

CUrlOptions::CUrlOptions()
  : m_strLead("")
{ }

CUrlOptions::CUrlOptions(const std::string &options, const char *strLead /* = "" */)
  : m_strLead(strLead)
{
  AddOptions(options);
}

CUrlOptions::~CUrlOptions()
{ }



std::string CUrlOptions::GetOptionsString(bool withLeadingSeperator /* = false */) const
{
  std::string options;
  for (UrlOptions::const_iterator opt = m_options.begin(); opt != m_options.end(); opt++)
  {
    if (opt != m_options.begin())
      options += "&";

    options += URLUtils::Encode(opt->first);
    if (!opt->second.empty())
    {
      options += "=";

      string value;

#if USE_STD_STRING_INSTEAD_OF_CVARIANT
      value = opt->second;
#else
      value = opt->second.asString();
#endif

      options += URLUtils::Encode(value);
    }
  }

  if (withLeadingSeperator && !options.empty())
  {
    if (m_strLead.empty())
      options = "?" + options;
    else
      options = m_strLead + options;
  }

  return options;
}

void CUrlOptions::AddOption(const std::string &key, const char *value)
{
  if (key.empty() || value == NULL)
    return;

  return AddOption(key, string(value));
}

void CUrlOptions::AddOption(const std::string &key, const std::string &value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, int value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, float value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, double value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, bool value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOptions(const std::string &options)
{
  if (options.empty())
    return;

  string strOptions = options;

  // if matching the preset leading str, remove from options.
  if (!m_strLead.empty() && strOptions.compare(0, m_strLead.length(), m_strLead) == 0)
    strOptions.erase(0, m_strLead.length());
  else if (strOptions.at(0) == '?' || strOptions.at(0) == '#' || strOptions.at(0) == ';' || strOptions.at(0) == '|')
  {
    // remove leading ?, #, ; or | if present
    //if (!m_strLead.empty()) // TODO
    //CLog::Log(LOGWARNING, "%s: original leading str %s overrided by %c", __FUNCTION__, m_strLead.c_str(), strOptions.at(0)); // TODO
    m_strLead = strOptions.at(0);
    strOptions.erase(0, 1);
  }

  // split the options by & and process them one by one
  vector<string> optionList;
  StringUtils::Split(strOptions, "&", optionList);
  for (vector<string>::const_iterator option = optionList.begin(); option != optionList.end(); option++)
  {
    if (option->empty())
      continue;

    string key, value;

    size_t pos = option->find('=');
    key = URLUtils::Decode(option->substr(0, pos));
    if (pos != string::npos)
      value = URLUtils::Decode(option->substr(pos + 1));

    // the key cannot be empty
    if (!key.empty())
      AddOption(key, value);
  }
}

void CUrlOptions::AddOptions(const CUrlOptions &options)
{
  m_options.insert(options.m_options.begin(), options.m_options.end());
}

void CUrlOptions::RemoveOption(const std::string &key)
{
  if (key.empty())
    return;

  UrlOptions::iterator option = m_options.find(key);
  if (option != m_options.end())
    m_options.erase(option);
}

bool CUrlOptions::HasOption(const std::string &key) const
{
  if (key.empty())
    return false;

  return m_options.find(key) != m_options.end();
}

bool CUrlOptions::GetOption(const std::string &key, UrlOptionValue_t &value) const
{
  if (key.empty())
    return false;

  UrlOptions::const_iterator option = m_options.find(key);
  if (option == m_options.end())
    return false;

  value = option->second;
  return true;
}
