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

#include "Config.h"
#include "lib/platform/threads/mutex.h"
#include "transponders/TransponderTypes.h"
#include "utils/List.h"

#include <stdint.h>

namespace VDR
{
class cScr : public cListObject {
private:
  int devices;
  int channel;
  uint userBand;
  int pin;
  bool used;
public:
  cScr(void);
  bool Parse(const char *s);
  int Devices(void) const { return devices; }
  int Channel(void) const { return channel; }
  uint UserBand(void) const { return userBand; }
  int Pin(void) const { return pin; }
  bool Used(void) const { return used; }
  void SetUsed(bool Used) { used = Used; }
  };

class cScrs : public cConfig<cScr> {
private:
  PLATFORM::CMutex mutex;
public:
  cScr *GetUnused(unsigned int Device);
  };

extern cScrs Scrs;

class cDiseqc : public cListObject {
public:
  enum eDiseqcActions {
    daNone,
    daToneOff,
    daToneOn,
    daVoltage13,
    daVoltage18,
    daMiniA,
    daMiniB,
    daScr,
    daCodes,
    };
  enum { MaxDiseqcCodes = 6 };
private:
  int devices;
  TRANSPONDER_TYPE source;
  int slof;
  fe_polarization_t m_polarization;
  int lof;
  mutable int scrBank;
  char *commands;
  bool parsing;
  uint SetScrFrequency(uint SatFrequency, const cScr *Scr, uint8_t *Codes) const;
  int SetScrPin(const cScr *Scr, uint8_t *Codes) const;
  const char *Wait(const char *s) const;
  const char *GetScrBank(const char *s) const;
  const char *GetCodes(const char *s, uint8_t *Codes = NULL, uint8_t *MaxCodes = NULL) const;
public:
  cDiseqc(void);
  ~cDiseqc();
  bool Parse(const char *s);
  eDiseqcActions Execute(const char **CurrentAction, uint8_t *Codes, uint8_t *MaxCodes, const cScr *Scr, uint *Frequency) const;
      ///< Parses the DiSEqC commands and returns the appropriate action code
      ///< with every call. CurrentAction must be the address of a character pointer,
      ///< which is initialized to NULL. This pointer is used internally while parsing
      ///< the commands and shall not be modified once Execute() has been called with
      ///< it. Call Execute() repeatedly (always providing the same CurrentAction pointer)
      ///< until it returns daNone. After a successful execution of all commands
      ///< *CurrentAction points to the value 0x00.
      ///< If the current action consists of sending code bytes to the device, those
      ///< bytes will be copied into Codes. MaxCodes must be initialized to the maximum
      ///< number of bytes Codes can handle, and will be set to the actual number of
      ///< bytes copied to Codes upon return.
      ///< If this DiSEqC entry requires SCR, the given Scr will be used. This must
      ///< be a pointer returned from a previous call to cDiseqcs::Get().
      ///< Frequency must be the frequency the tuner will be tuned to, and will be
      ///< set to the proper SCR frequency upon return (if SCR is used).
  int Devices(void) const { return devices; }
  TRANSPONDER_TYPE Source(void) const { return source; }
  int Slof(void) const { return slof; }
  fe_polarization_t Polarization(void) const { return m_polarization; }
  int Lof(void) const { return lof; }
  bool IsScr() const { return scrBank >= 0; }
  const char *Commands(void) const { return commands; }
  };

class cDiseqcs : public cConfig<cDiseqc> {
public:
  // TODO: Do we also need position/direction?
  const cDiseqc *Get(unsigned int Device, TRANSPONDER_TYPE Source, unsigned int Frequency, fe_polarization_t Polarization, const cScr **Scr) const;
      ///< Selects a DiSEqC entry suitable for the given Device and tuning parameters.
      ///< If this DiSEqC entry requires SCR and the given *Scr is NULL
      ///< a free one will be selected from the Scrs and a pointer to that will
      ///< be returned in Scr. The caller shall memorize that pointer and reuse it in
      ///< subsequent calls.
      ///< Scr may be NULL for checking whether there is any DiSEqC entry for the
      ///< given transponder.
  };

extern cDiseqcs Diseqcs;
}
