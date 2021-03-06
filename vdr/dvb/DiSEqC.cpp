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

#include "DiSEqC.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"

#include <ctype.h>

using namespace PLATFORM;

namespace VDR
{

static bool ParseDeviceNumbers(const char *s, int &Devices)
{
  if (*s && s[strlen(s) - 1] == ':') {
     const char *p = s;
     while (*p && *p != ':') {
           char *t = NULL;
           int d = strtol(p, &t, 10);
           p = t;
           if (0 <= d && d < 31)
              Devices |= (1 << d);
           else {
              esyslog("ERROR: invalid device number %d in '%s'", d, s);
              return false;
              }
           }
     }
  return true;
}

// --- cScr ------------------------------------------------------------------

cScr::cScr(void)
{
  devices = 0;
  channel = -1;
  userBand = 0;
  pin = -1;
  used = false;
}

bool cScr::Parse(const char *s)
{
  if (!ParseDeviceNumbers(s, devices))
     return false;
  if (devices)
     return true;
  bool result = false;
  int fields = sscanf(s, "%d %u %d", &channel, &userBand, &pin);
  if (fields == 2 || fields == 3) {
     if (channel >= 0 && channel < 8) {
        result = true;
        if (fields == 3 && (pin < 0 || pin > 255)) {
           esyslog("Error: invalid SCR pin '%d'", pin);
           result = false;
           }
        }
     else
        esyslog("Error: invalid SCR channel '%d'", channel);
     }
  return result;
}

// --- cScrs -----------------------------------------------------------------

cScrs Scrs;

cScr *cScrs::GetUnused(unsigned int Device)
{
  CLockObject lock(mutex);
  int Devices = 0;
  for (cScr *p = First(); p; p = Next(p)) {
      if (p->Devices()) {
         Devices = p->Devices();
         continue;
         }
      if (Devices && !(Devices & (1 << Device)))
         continue;
      if (!p->Used()) {
        p->SetUsed(true);
        return p;
        }
      }
  return NULL;
}

// --- cDiseqc ---------------------------------------------------------------

cDiseqc::cDiseqc(void)
{
  devices = 0;
  source = TRANSPONDER_INVALID;
  slof = 0;
  m_polarization = POLARIZATION_HORIZONTAL;
  lof = 0;
  scrBank = -1;
  commands = NULL;
  parsing = false;
}

cDiseqc::~cDiseqc()
{
  free(commands);
}

bool cDiseqc::Parse(const char *s)
{
  if (!ParseDeviceNumbers(s, devices))
     return false;
  if (devices)
     return true;
  bool result = false;
  char *sourcebuf = NULL;
  char polarization;
  int fields = sscanf(s, "%a[^ ] %d %c %d %a[^\n]", &sourcebuf, &slof, &polarization, &lof, &commands);
  if (fields == 4)
     commands = NULL; //XXX Apparently sscanf() doesn't work correctly if the last %a argument results in an empty string
  if (4 <= fields && fields <= 5) {
     source = TRANSPONDER_INVALID; // TODO: Create source from sourcebuf

     if (source != TRANSPONDER_INVALID) {
        switch (toupper(polarization))
        {
        case 'H': m_polarization = POLARIZATION_HORIZONTAL; break;
        case 'V': m_polarization = POLARIZATION_VERTICAL; break;
        case 'L': m_polarization = POLARIZATION_CIRCULAR_LEFT; break;
        case 'R': m_polarization = POLARIZATION_CIRCULAR_RIGHT; break;
        }
        if (polarization == 'V' || polarization == 'H' || polarization == 'L' || polarization == 'R') {
           parsing = true;
           const char *CurrentAction = NULL;
           while (Execute(&CurrentAction, NULL, NULL, NULL, NULL) != daNone)
                 ;
           parsing = false;
           result = !commands || !*CurrentAction;
           }
        else
           esyslog("ERROR: unknown polarization '%c'", polarization);
        }
     else
        esyslog("ERROR: unknown source '%s'", sourcebuf);
     }
  free(sourcebuf);
  return result;
}

uint cDiseqc::SetScrFrequency(uint SatFrequency, const cScr *Scr, uint8_t *Codes) const
{
  uint t = SatFrequency == 0 ? 0 : (SatFrequency + Scr->UserBand() + 2) / 4 - 350; // '+ 2' together with '/ 4' results in rounding!
  if (t < 1024 && Scr->Channel() >= 0 && Scr->Channel() < 8) {
     Codes[3] = t >> 8 | (t == 0 ? 0 : scrBank << 2) | Scr->Channel() << 5;
     Codes[4] = t;
     if (t)
        return (t + 350) * 4 - SatFrequency;
     }
  return 0;
}

int cDiseqc::SetScrPin(const cScr *Scr, uint8_t *Codes) const
{
  if (Scr->Pin() >= 0 && Scr->Pin() <= 255) {
     Codes[2] = 0x5C;
     Codes[5] = Scr->Pin();
     return 6;
     }
  else {
     Codes[2] = 0x5A;
     return 5;
     }
}

const char *cDiseqc::Wait(const char *s) const
{
  char *p = NULL;
  errno = 0;
  int n = strtol(s, &p, 10);
  if (!errno && p != s && n >= 0) {
     if (!parsing)
       CEvent::Sleep(n);
     return p;
     }
  esyslog("ERROR: invalid value for wait time in '%s'", s - 1);
  return NULL;
}

const char *cDiseqc::GetScrBank(const char *s) const
{
  char *p = NULL;
  errno = 0;
  int n = strtol(s, &p, 10);
  if (!errno && p != s && n >= 0 && n < 8) {
     if (parsing) {
        if (scrBank < 0)
           scrBank = n;
        else
           esyslog("ERROR: more than one scr bank in '%s'", s - 1);
        }
     return p;
     }
  esyslog("ERROR: invalid value for scr bank in '%s'", s - 1);
  return NULL;
}

const char *cDiseqc::GetCodes(const char *s, uint8_t *Codes, uint8_t *MaxCodes) const
{
  const char *e = strchr(s, ']');
  if (e) {
     int NumCodes = 0;
     const char *t = s;
     while (t < e) {
           if (NumCodes < MaxDiseqcCodes) {
              errno = 0;
              char *p;
              int n = strtol(t, &p, 16);
              if (!errno && p != t && 0 <= n && n <= 255) {
                 if (Codes) {
                    if (NumCodes < *MaxCodes)
                       Codes[NumCodes++] = uint8_t(n);
                    else {
                       esyslog("ERROR: too many codes in code sequence '%s'", s - 1);
                       return NULL;
                       }
                    }
                 t = skipspace(p);
                 }
              else {
                 esyslog("ERROR: invalid code at '%s'", t);
                 return NULL;
                 }
              }
           else {
              esyslog("ERROR: too many codes in code sequence '%s'", s - 1);
              return NULL;
              }
           }
     if (MaxCodes)
        *MaxCodes = NumCodes;
     return e + 1;
     }
  else
     esyslog("ERROR: missing closing ']' in code sequence '%s'", s - 1);
  return NULL;
}

cDiseqc::eDiseqcActions cDiseqc::Execute(const char **CurrentAction, uint8_t *Codes, uint8_t *MaxCodes, const cScr *Scr, uint *Frequency) const
{
  if (!*CurrentAction)
     *CurrentAction = commands;
  while (*CurrentAction && **CurrentAction) {
        switch (*(*CurrentAction)++) {
          case ' ': break;
          case 't': return daToneOff;
          case 'T': return daToneOn;
          case 'v': return daVoltage13;
          case 'V': return daVoltage18;
          case 'A': return daMiniA;
          case 'B': return daMiniB;
          case 'W': *CurrentAction = Wait(*CurrentAction); break;
          case 'S': *CurrentAction = GetScrBank(*CurrentAction); break;
          case '[': *CurrentAction = GetCodes(*CurrentAction, Codes, MaxCodes);
                    if (*CurrentAction) {
                       if (Scr && Frequency) {
                          *Frequency = SetScrFrequency(*Frequency, Scr, Codes);
                          *MaxCodes = SetScrPin(Scr, Codes);
                          }
                       return daCodes;
                       }
                    break;
          default: return daNone;
          }
        }
  return daNone;
}

// --- cDiseqcs --------------------------------------------------------------

cDiseqcs Diseqcs;

const cDiseqc *cDiseqcs::Get(unsigned int Device, TRANSPONDER_TYPE Source, unsigned int Frequency, fe_polarization_t Polarization, const cScr **Scr) const
{
  int Devices = 0;
  for (const cDiseqc *p = First(); p; p = Next(p)) {
      if (p->Devices()) {
         Devices = p->Devices();
         continue;
         }
      if (Devices && !(Devices & (1 << Device)))
         continue;
      // TODO: Should we also compare position/direction?
      if (p->Source() == Source && p->Slof() > Frequency && p->Polarization() == Polarization) {
         if (p->IsScr() && Scr && !*Scr) {
            *Scr = Scrs.GetUnused(Device);
            if (*Scr)
               dsyslog("SCR %d assigned to device %u", (*Scr)->Channel(), Device);
            else
               esyslog("ERROR: no free SCR entry available for device %u", Device);
            }
         return p;
         }
      }
  return NULL;
}

}
