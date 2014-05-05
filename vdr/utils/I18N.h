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
#pragma once

#include <stdio.h>
#include "Tools.h"

#include <string>
#include <vector>

#if defined(TARGET_ANDROID) && !defined(__attribute_format_arg__)
#define __attribute_format_arg__(x) __attribute__ ((__format_arg__ (x)))
#endif

namespace VDR
{
#define I18N_DEFAULT_LOCALE "en_US"
#define I18N_MAX_LOCALE_LEN 16       // for buffers that hold en_US etc.
#define I18N_MAX_LANGUAGES  256      // for buffers that hold all available languages

void I18nInitialize(const char *LocaleDir = NULL);
   ///< Detects all available locales and loads the language names and codes.
   ///< If LocaleDir is given, it must point to a static string that lives
   ///< for the entire lifetime of the program.
void I18nRegister(const char *Plugin);
   ///< Registers the named plugin, so that it can use internationalized texts.
void I18nSetLocale(const char *Locale);
   ///< Sets the current locale to Locale. The default locale is "en_US".
   ///< If no such locale has been found in the call to I18nInitialize(),
   ///< nothing happens.
int I18nCurrentLanguage(void);
   ///< Returns the index of the current language. This number stays the
   ///< same for any given language while the program is running, but may
   ///< be different when the program is run again (for instance because
   ///< a locale has been added or removed). The default locale ("en_US")
   ///< always has a zero index.
void I18nSetLanguage(int Language);
   ///< Sets the current language index to Language. If Language is outside
   ///< the range of available languages, nothing happens.
int I18nNumLanguagesWithLocale(void);
   ///< Returns the number of entries in the list returned by I18nLanguages()
   ///< that actually have a locale.
const std::vector<std::string> &I18nLanguages(void);
   ///< Returns the list of available languages. Values returned by
   ///< I18nCurrentLanguage() are indexes into this list.
   ///< Only the first I18nNumLanguagesWithLocale() entries in this list
   ///< have an actual locale installed. The rest are just dummy entries
   ///< to allow having three letter language codes for other languages
   ///< that have no actual locale on this system.
const char *I18nTranslate(const char *s, const char *Plugin = NULL) __attribute_format_arg__(1);
   ///< Translates the given string (with optional Plugin context) into
   ///< the current language. If no translation is available, the original
   ///< string will be returned.
const char *I18nLocale(int Language);
   ///< Returns the locale code of the given Language (which is an index as
   ///< returned by I18nCurrentLanguage()). If Language is outside the range
   ///< of available languages, NULL is returned.
const char *I18nLanguageCode(int Language);
   ///< Returns the three letter language code of the given Language (which
   ///< is an index as returned by I18nCurrentLanguage()). If Language is
   ///< outside the range of available languages, NULL is returned.
   ///< The returned string may consist of several alternative three letter
   ///< language codes, separated by commas (as in "deu,ger").
int I18nLanguageIndex(const char *Code);
   ///< Returns the index of the language with the given three letter
   ///< language Code. If no suitable language is found, -1 is returned.
const char *I18nNormalizeLanguageCode(const char *Code);
   ///< Returns a 3 letter language code that may not be zero terminated.
   ///< If no normalized language code can be found, the given Code is returned.
   ///< Make sure at most 3 characters are copied when using it!
bool I18nIsPreferredLanguage(int *PreferredLanguages, const char *LanguageCode, int &OldPreference, int *Position = NULL);
   ///< Checks the given LanguageCode (which may be something like "eng" or "eng+deu")
   ///< against the PreferredLanguages and returns true if one is found that has an index
   ///< smaller than OldPreference (which should be initialized to -1 before the first
   ///< call to this function in a sequence of checks). If LanguageCode is not any of
   ///< the PreferredLanguages, and OldPreference is less than zero, OldPreference will
   ///< be set to a value higher than the highest language index.  If Position is given,
   ///< it will return 0 if this was a single language code (like "eng"), 1 if it was
   ///< the first of two language codes (like "eng" out of "eng+deu") and 2 if it was
   ///< the second one (like "deu" out of ""eng+deu").

#ifdef PLUGIN_NAME_I18N
#define tr(s)  I18nTranslate(s, "vdr-" PLUGIN_NAME_I18N)
#define trVDR(s) I18nTranslate(s)  // to use a text that's in the VDR core's translation file
#else
#define tr(s)  I18nTranslate(s)
#endif

#define trNOOP(s) (s)
}
