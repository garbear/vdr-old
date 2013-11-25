/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2011-2013 Team XBMC
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
#pragma once

#include <stdint.h>
#include <string>

class Base64
{
public:
  static void Encode(const uint8_t *input, unsigned int length, std::string &output);
  static std::string Encode(const uint8_t *input, unsigned int length);
  static void Encode(const std::string &input, std::string &output);
  static std::string Encode(const std::string &input);
  static void Decode(const uint8_t *input, unsigned int length, std::string &output);
  static std::string Decode(const uint8_t *input, unsigned int length);
  static void Decode(const std::string &input, std::string &output);
  static std::string Decode(const std::string &input);

private:
  static const std::string m_characters;
};
