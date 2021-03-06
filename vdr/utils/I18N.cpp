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

/*
 * In case an English phrase is used in more than one context (and might need
 * different translations in other languages) it can be preceded with an
 * arbitrary string to describe its context, separated from the actual phrase
 * by a '$' character (see for instance "Button$Stop" vs. "Stop").
 * Of course this means that no English phrase may contain the '$' character!
 * If this should ever become necessary, the existing '$' would have to be
 * replaced with something different...
 */

#include "I18N.h"
#include "Tools.h"
#include "utils/log/Log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <ctype.h>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>

using namespace std;

extern int _nl_msg_cat_cntr;

namespace VDR
{

// TRANSLATORS: The name of the language, as written natively
const char *LanguageName = trNOOP("LanguageName$English");
// TRANSLATORS: The 3-letter code of the language
const char *LanguageCode = trNOOP("LanguageCode$eng");

// List of known language codes with aliases.
// Actually we could list all codes from http://www.loc.gov/standards/iso639-2
// here, but that would be several hundreds - and for most of them it's unlikely
// they're ever going to be used...

const char *LanguageCodeList[] = {
  "eng,dos",
  "deu,ger",
  "slv,slo",
  "ita",
  "dut,nla,nld",
  "prt",
  "fra,fre",
  "nor",
  "fin,suo",
  "pol",
  "esl,spa",
  "ell,gre",
  "sve,swe",
  "rom,rum",
  "hun",
  "cat,cln",
  "rus",
  "srb,srp,scr,scc",
  "hrv",
  "est",
  "dan",
  "cze,ces",
  "tur",
  "ukr",
  "ara",
  NULL
  };

static string I18nLocaleDir;

static vector<string> LanguageLocales;
static vector<string> LanguageNames;
static vector<string> LanguageCodes;

static int NumLocales = 1;
static int CurrentLanguage = 0;

static bool ContainsCode(const char *Codes, const char *Code)
{
  while (*Codes) {
        int l = 0;
        for ( ; l < 3 && Code[l]; l++) {
            if (Codes[l] != tolower(Code[l]))
               break;
            }
        if (l == 3)
           return true;
        Codes++;
        }
  return false;
}

static const char *SkipContext(const char *s)
{
  const char *p = strchr(s, '$');
  return p ? p + 1 : s;
}

static void SetEnvLanguage(const char *Locale)
{
  setenv("LANGUAGE", Locale, 1);
  ++_nl_msg_cat_cntr;
}

void I18nInitialize(const char *LocaleDir)
{
  I18nLocaleDir = LocaleDir;
  LanguageLocales.push_back(I18N_DEFAULT_LOCALE);
  LanguageNames.push_back(SkipContext(LanguageName));
  LanguageCodes.push_back(LanguageCodeList[0]);
  textdomain("vdr");
  bindtextdomain("vdr", I18nLocaleDir.c_str());
  vector<string> Locales;
  if (GetSubDirectories(I18nLocaleDir.c_str(), Locales)) {
     char *OldLocale = strdup(setlocale(LC_MESSAGES, NULL));
     for (vector<string>::const_iterator it = Locales.begin(); it != Locales.end(); ++it) {
         string FileName = StringUtils::Format("%s/%s/LC_MESSAGES/vdr.mo", I18nLocaleDir.c_str(), it->c_str());
         if (access(FileName.c_str(), F_OK) == 0) { // found a locale with VDR texts
            if (NumLocales < I18N_MAX_LANGUAGES - 1) {
               SetEnvLanguage(it->c_str());
               const char *TranslatedLanguageName = gettext(LanguageName);
               if (TranslatedLanguageName != LanguageName) {
                  NumLocales++;
                  if (strstr(OldLocale, it->c_str()) == OldLocale)
                     CurrentLanguage = LanguageLocales.size();
                  LanguageLocales.push_back(*it);
                  LanguageNames.push_back(TranslatedLanguageName);
                  const char *Code = gettext(LanguageCode);
                  for (const char **lc = LanguageCodeList; *lc; lc++) {
                      if (ContainsCode(*lc, Code)) {
                         Code = *lc;
                         break;
                         }
                      }
                  LanguageCodes.push_back(Code);
                  }
               }
            else {
               esyslog("ERROR: too many locales - increase I18N_MAX_LANGUAGES!");
               break;
               }
            }
         }
     SetEnvLanguage(LanguageLocales[CurrentLanguage].c_str());
     free(OldLocale);
     dsyslog("found %d locales in %s", NumLocales - 1, I18nLocaleDir.c_str());
     }
  // Prepare any known language codes for which there was no locale:
  for (const char **lc = LanguageCodeList; *lc; lc++) {
      bool Found = false;
      for (vector<string>::const_iterator it = LanguageCodes.begin(); it != LanguageCodes.end(); ++it) {
          if (*it == *lc) {
             Found = true;
             break;
             }
          }
      if (!Found) {
         dsyslog("no locale for language code '%s'", *lc);
         LanguageLocales.push_back(I18N_DEFAULT_LOCALE);
         LanguageNames.push_back(*lc);
         LanguageCodes.push_back(*lc);
         }
      }
}

void I18nRegister(const char *Plugin)
{
  string Domain = StringUtils::Format("vdr-%s", Plugin);
  bindtextdomain(Domain.c_str(), I18nLocaleDir.c_str());
}

void I18nSetLocale(const char *Locale)
{
  if (Locale && *Locale) {
     vector<string>::const_iterator it = find(LanguageLocales.begin(), LanguageLocales.end(), Locale);
     int i = (it != LanguageLocales.end() ? it - LanguageLocales.begin() : -1);
     if (i >= 0) {
        CurrentLanguage = i;
        SetEnvLanguage(Locale);
        }
     else
        dsyslog("unknown locale: '%s'", Locale);
     }
}

int I18nCurrentLanguage(void)
{
  return CurrentLanguage;
}

void I18nSetLanguage(int Language)
{
  if (Language < (int)LanguageNames.size()) {
     CurrentLanguage = Language;
     I18nSetLocale(I18nLocale(CurrentLanguage));
     }
}

int I18nNumLanguagesWithLocale(void)
{
  return NumLocales;
}

const vector<string> &I18nLanguages(void)
{
  return LanguageNames;
}

const char *I18nTranslate(const char *s, const char *Plugin)
{
  if (!s)
     return s;
  if (CurrentLanguage) {
     const char *t = Plugin ? dgettext(Plugin, s) : gettext(s);
     if (t != s)
        return t;
     }
  return SkipContext(s);
}

const char *I18nLocale(int Language)
{
  return 0 <= Language && Language < (int)LanguageLocales.size() ? LanguageLocales[Language].c_str() : NULL;
}

const char *I18nLanguageCode(int Language)
{
  return 0 <= Language && Language < (int)LanguageCodes.size() ? LanguageCodes[Language].c_str() : NULL;
}

int I18nLanguageIndex(const char *Code)
{
  for (int i = 0; i < (int)LanguageCodes.size(); i++) {
      if (ContainsCode(LanguageCodes[i].c_str(), Code))
         return i;
      }
  //dsyslog("unknown language code: '%s'", Code);
  return -1;
}

const char *I18nNormalizeLanguageCode(const char *Code)
{
  for (int i = 0; i < 3; i++) {
      if (Code[i]) {
         // ETSI EN 300 468 defines language codes as consisting of three letters
         // according to ISO 639-2. This means that they are supposed to always consist
         // of exactly three letters in the range a-z - no digits, UTF-8 or other
         // funny characters. However, some broadcasters apparently don't have a
         // copy of the DVB standard (or they do, but are perhaps unable to read it),
         // so they put all sorts of non-standard stuff into the language codes,
         // like nonsense as "2ch" or "A 1" (yes, they even go as far as using
         // blanks!). Such things should go into the description of the EPG event's
         // ComponentDescriptor.
         // So, as a workaround for this broadcaster stupidity, let's ignore
         // language codes with unprintable characters...
         if (!isprint(Code[i])) {
            //dsyslog("invalid language code: '%s'", Code);
            return "???";
            }
         // ...and replace blanks with underlines (ok, this breaks the 'const'
         // of the Code parameter - but hey, it's them who started this):
         if (Code[i] == ' ')
            *((char *)&Code[i]) = '_';
         }
      else
         break;
      }
  int n = I18nLanguageIndex(Code);
  return n >= 0 ? I18nLanguageCode(n) : Code;
}

bool I18nIsPreferredLanguage(int *PreferredLanguages, const char *LanguageCode, int &OldPreference, int *Position)
{
  int pos = 1;
  bool found = false;
  while (LanguageCode) {
        int LanguageIndex = I18nLanguageIndex(LanguageCode);
        for (int i = 0; i < (int)LanguageCodes.size(); i++) {
            if (PreferredLanguages[i] < 0)
               break; // the language is not a preferred one
            if (PreferredLanguages[i] == LanguageIndex) {
               if (OldPreference < 0 || i < OldPreference) {
                  OldPreference = i;
                  if (Position)
                     *Position = pos;
                  found = true;
                  break;
                  }
               }
            }
        if ((LanguageCode = strchr(LanguageCode, '+')) != NULL) {
           LanguageCode++;
           pos++;
           }
        else if (pos == 1 && Position)
           *Position = 0;
        }
  if (OldPreference < 0) {
     OldPreference = LanguageCodes.size(); // higher than the maximum possible value
     return true; // if we don't find a preferred one, we take the first one
     }
  return found;
}

}
