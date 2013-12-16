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

#include <map>
#include <string>

#ifndef USE_STD_STRING_INSTEAD_OF_CVARIANT
#define USE_STD_STRING_INSTEAD_OF_CVARIANT  1
#endif

#include <map>
#include <string>

#if !USE_STD_STRING_INSTEAD_OF_CVARIANT
#include "utils/Variant.h"
#endif


class CUrlOptions
{
public:
#if USE_STD_STRING_INSTEAD_OF_CVARIANT
  typedef std::string UrlOptionValue_t;
#else
  typedef CVariant UrlOptionValue_t;
#endif

  typedef std::map<std::string, UrlOptionValue_t> UrlOptions;


  CUrlOptions();
  CUrlOptions(const std::string &options, const char *strLead = "");
  virtual ~CUrlOptions();

  virtual void Clear() { m_options.clear(); m_strLead = ""; }

  virtual const UrlOptions& GetOptions() const { return m_options; }
  virtual std::string GetOptionsString(bool withLeadingSeperator = false) const;

  virtual void AddOption(const std::string &key, const char *value);
  virtual void AddOption(const std::string &key, const std::string &value);
  virtual void AddOption(const std::string &key, int value);
  virtual void AddOption(const std::string &key, float value);
  virtual void AddOption(const std::string &key, double value);
  virtual void AddOption(const std::string &key, bool value);
  virtual void AddOptions(const std::string &options);
  virtual void AddOptions(const CUrlOptions &options);
  virtual void RemoveOption(const std::string &key);

  virtual bool HasOption(const std::string &key) const;
  virtual bool GetOption(const std::string &key, UrlOptionValue_t &value) const;

protected:
  UrlOptions m_options;
  std::string m_strLead;
};
