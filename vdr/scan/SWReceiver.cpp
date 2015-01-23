/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "SWReceiver.h"
#include "CNICodes.h"
#include "channels/Channel.h"
#include "utils/CommonMacros.h"
#include "utils/log/Log.h"
#include "utils/Ringbuffer.h"
#include "utils/Tools.h"

#include <time.h>

using namespace PLATFORM;
using namespace std;

namespace VDR
{

#define TS_DBG  0
#define PES_DBG 0
#define CNI_DBG 0
#define FUZZY_DBG 0
#define MAXHITS 50
#define MINHITS 2
#define TIMEOUT 15
#define Dec(x)        (CharToInt(x) < 10)            /* "z" == 0x30..0x39             */
#define Hex(x)        (CharToInt(x) < 16)            /* "h" == 0x30..0x39; 0x41..0x46 */
#define Delim(x)      ((x == 46) || (x == 58))       /* date/time delimiter.          */
#define Magenta(x)    ((x == 5))                     /* ":" == 0x05                   */
#define Concealed(x)  ((x == 0x1b) || (x == 0x18))   /* "%" == 0x18; 300231 has typo. */


uint8_t Revert8(uint8_t inp)
{
  uint8_t lookup[] =
  {
    0, 8, 4, 12, 2, 10, 6, 14,
    1, 9, 5, 13, 3, 11, 7, 15
  };
  return lookup[(inp & 0xF0) >> 4] | lookup[inp & 0xF] << 4;
}

uint16_t Revert16(uint16_t inp)
{
  return Revert8(inp & 0xFF) << 8 | Revert8(inp >> 8);
}

uint8_t RevertNibbles(uint8_t inp)
{
  inp = Revert8(inp);
  return (inp & 0xF0) >> 4 | (inp & 0xF) << 4;
}

uint8_t Hamming_8_4(uint8_t aByte)
{
  uint8_t lookup[] =
  {
    21,  2,   73,  94,  100, 115,  56,  47,
    208, 199, 140, 155, 161, 182, 253, 234
  };
  return lookup[aByte];
}

int HammingDistance_8(uint8_t a, uint8_t b)
{
  int ret = 0;
  int mask[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  uint8_t XOR = a ^ b;

  for (unsigned int i = 0; i < 8; i++)
    ret += (XOR & mask[i]) >> i;

  return ret;
}

uint8_t OddParity(uint8_t u)
{
  int i;
  int mask[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  uint8_t p=0;

  for (i=0; i<8; i++)
    p += (u & mask[i]) >> i;

  return p & 1 ? u & 0x7F : 255;
}

int DeHamming_8_4(uint8_t aByte)
{
  for (uint8_t i = 0; i < 16; i++)
    if (HammingDistance_8(Hamming_8_4(i), aByte) < 2)
      return i;

  return -1; // 2 or more bit errors: non recoverable.
}

uint32_t DeHamming_24_18(uint8_t* data)
{
  //FIXME: add bit error detection and correction.
  return ((*(data + 0) >> 2) & 0x1)             |
         ((*(data + 0) >> 3) & 0xE)             |
         (uint32_t)((*(data + 1) & 0x7F) << 4 ) |
         (uint32_t)((*(data + 2) & 0x7F) << 11);
}

static inline uint8_t CharToInt(uint8_t c)
{
  switch(c)
  {
  case 0x30 ... 0x39: return c - 48;
  case 0x41 ... 0x46: return c - 55;
  case 0x61 ... 0x66: return c - 87;
  default:            return 0xff;
  }
}

/*-----------------------------------------------------------------------------
 * cSwReceiver
 *---------------------------------------------------------------------------*/

cSwReceiver::cSwReceiver(ChannelPtr Channel)
 : channel(Channel),
   buffer(new cRingBufferLinear(MEGABYTE(1), 184)),
   m_bStopped(false),
   fuzzy(false),
   hits(0),
   m_timeout(CDateTime::GetUTCDateTime() + CDateTimeSpan(0, 0, 0, TIMEOUT)),
   cni_8_30_1(0),
   cni_8_30_2(0),
   cni_X_26(0),
   cni_vps(0),
   cni_cr_idx(0),
   TsCount(0)
{
}

void cSwReceiver::Reset()
{
  m_bStopped = fuzzy = false;
  Sleep(10);
  buffer->Clear();
  cni_8_30_1 = cni_8_30_2 = cni_X_26 = cni_vps = cni_cr_idx = 0;
  hits = TsCount = 0;
  m_timeout = CDateTime::GetUTCDateTime() + CDateTimeSpan(0, 0, 0, TIMEOUT);
  CreateThread();
}

cSwReceiver::~cSwReceiver()
{
  m_bStopped = true;
  buffer->Clear();
  DELETENULL(buffer);
}

void cSwReceiver::Receive(uint8_t* Data, int Length, ts_crc_check_t& crcvalid)
{
  uint8_t* p;
  int count = 184;

  if (m_bStopped || IsStopped())
    return;

  // pointer to new PES header or next 184byte payload of current PES packet
  p = &Data[4];

  if (*(p + 0) == 0 && *(p + 1) == 0 && *(p + 2) == 1) // next PES header
  {
    uint8_t PES_header_data_length = *(p+8);
    uint8_t data_identifier;

    data_identifier = *(p + 9 + PES_header_data_length);
    if ((data_identifier < 0x10) || (data_identifier > 0x1F))
      return; // unknown EBU data. not stated in 300472

    p += 10 + PES_header_data_length;
    count -= 10 + PES_header_data_length;
  }

  buffer->Put(p, count);
}

void cSwReceiver::DecodePacket(uint8_t* data)
{
  uint8_t MsbFirst[48] = { 0 };
  uint8_t aByte;
  int i;
  int Magazine = 0;
  int PacketNumber = 0;
  static uint16_t pagenumber, subpage;
  
  switch (data[0]) // data_unit_id
  {
  case 0xC3: // VPS line 16
  {
    uint8_t* p = &MsbFirst[3];

    /* convert data from lsb first to msb first */
    for (i = 3; i < 16; i++)
      MsbFirst[i] = Revert8(data[i]);

    cni_vps = (p[10] & 0x03) << 10 |
              (p[11] & 0xC0) << 2  |
              (p[ 8] & 0xC0)       |
              (p[11] & 0x3F);
         
    if (cni_vps && GetCniNameVPS())
    {
      if (CNI_DBG)
        dsyslog("cni_vps = 0x%.3x -> %s", cni_vps, GetCniNameVPS());
      if (++hits > MAXHITS)
        m_bStopped = true;
    }
  }
  break;

  case 0x02: // EBU teletext
  {
    /* convert data from lsb first to msb first */
    for (i = 4; i < 46; i++)
      MsbFirst[i] = Revert8(data[i]);

    /* decode magazine and packet number */
    aByte = DeHamming_8_4(MsbFirst[4]);

    if (!(Magazine = aByte & 0x7))
      Magazine = 8;

    PacketNumber = (aByte & 0xF) >> 3 | DeHamming_8_4(MsbFirst[5]) << 1;

    switch (PacketNumber)
    {
    case 0: /* page header X/0 */
    {
      uint8_t* p = &MsbFirst[6];
      pagenumber = DeHamming_8_4(*(p)) + 10 * DeHamming_8_4(*(p + 1));
      p += 2;
      subpage  =  DeHamming_8_4(*(p++));
      subpage |= (DeHamming_8_4(*(p++)) & 0x7) << 4;
      subpage |=  DeHamming_8_4(*(p++)) << 7;
      subpage |= (DeHamming_8_4(*(p++)) & 0x3) << 11;
      if (Magazine == 1 && ! pagenumber)
      {
        int bytes = 32;
        p = &MsbFirst[14];
        for (int i = 0; i < bytes; i++)
          *(p+i) &= 0x7F;

        for (int i = 0; i < bytes - 2; i++)
        {
          if ((p[i] == 49) && (p[i+1] == 48) && (p[i+2] == 48))
          {
            p[i] = p[i+1] = p[i+2] = 0;
            break;
          }
        }
        for (int i = 0; i < bytes - 4; i++)
        {
          if ((p[i  ] == 118 || p[i  ] == 86) &&
              (p[i+1] == 105 || p[i+1] == 73) &&
              (p[i+2] == 100 || p[i+2] == 68) &&
              (p[i+3] == 101 || p[i+3] == 69) &&
              (p[i+4] == 111 || p[i+4] == 79))
            p[i] = p[i+1] = p[i+2] = p[i+3] = p[i+4] = 0;

          if ((p[i  ] == 116 || p[i  ] == 84) &&
              (p[i+1] == 101 || p[i+1] == 69) &&
              (p[i+2] == 120 || p[i+2] == 88) &&
              (p[i+3] == 116 || p[i+3] == 84))
            p[i] = p[i+1] = p[i+2] = p[i+3] = 0;

          if (Dec(p[i]) && Dec(p[i+1]) && Delim(p[i+2]))
            p[i] = p[i+1] = p[i+2] = 0;
        }

        for (int i = 0; i < bytes - 3; i++)
        {
          if ((p[i] <= 32) && (p[i+1] <= 32))
            p[i] = p[i+1] = 0;

          if (p[i] < 32)
            p[i] = 0;

          if (((p[i] == 45) || (p[i] == 46)) && (p[i+1] < 32))
            p[i] = 0;
        }

        while (*p < 33)
        {
          p++;
          bytes--;
        }
        while (*(p + bytes) < 33)
          bytes--;

        UpdatefromName(reinterpret_cast<const char*>(p));
      }
    }
    break;

    case 1 ... 25: /* visible text X/1..X/25 */
    {
      /* PDC embedded into visible text.
       * 300231 page 30..31 'Method A'
       */
      uint8_t* p = &MsbFirst[6];
      if (Magazine != 3)
        break;
      for (int i = 0; i < 48; i++)
        MsbFirst[i] &= 0x7F;
      for (int i=0; i < 34; i++)
      {
        if (Magenta(*(p++)))
        {
          if (Concealed(*(p)) &&
              Hex(*(p+1)) &&
              Hex(*(p+2)) &&
              Dec(*(p+3)) &&
              Dec(*(p+4)) &&
              Dec(*(p+5)) &&
              Concealed(*(p+6)))
          {
            cni_cr_idx =  CharToInt(*(p+1)) << 12 | CharToInt(*(p+2)) << 8;
            cni_cr_idx |= CharToInt(*(p+3)) * 100 + CharToInt(*(p+4)) * 10 + CharToInt(*(p+5));
            if (cni_cr_idx && GetCniNameCrIdx())
            {
              if (CNI_DBG)
                dsyslog("cni_cr_idx = %.2X%.3d -> %s", cni_cr_idx>>8, cni_cr_idx&255, GetCniNameCrIdx());
              if (++hits > MAXHITS)
                m_bStopped = true;
            }
          }
        }
      }
    }
    break;

    case 26:
    {
      /* 13 x 3-byte pairs of hammed 24/18 data.
       * 300231 page 30..31 'Method B'
       */
      uint8_t* p = &MsbFirst[7];
      for (int i=0; i < 13; i++)
      {
        uint32_t value = DeHamming_24_18(p);
        p += 3;
        uint16_t data_word_A, mode_description, data_word_B;

        data_word_A      = (value >> 0 ) & 0x3F;
        mode_description = (value >> 6 ) & 0x1F;
        data_word_B      = (value >> 11) & 0x7F;
        if (mode_description == 0x08)
        {
          // 300231, 7.3.2.3 Coding of preselection data in extension packets X/26
          if (data_word_A >= 0x30)
          {
            cni_X_26 = data_word_A << 8 | data_word_B;
            if (cni_X_26 && GetCniNameX26())
            {
              if (CNI_DBG)
                dsyslog("cni_X_26 = 0x%.4x -> %s", cni_X_26, GetCniNameX26());
              if (++hits > MAXHITS)
                m_bStopped = true;
            }
          }
        }
      }
    }
    break; // end X/26

    case 30:
    {
      if (Magazine < 8)
        break;
      int DesignationCode = DeHamming_8_4(MsbFirst[6]);
      switch (DesignationCode)
      {
      case 0:
      case 1:
      /* ETS 300 706: Table 18: Coding of Packet 8/30 Format 1 */
      {
        cni_8_30_1 = Revert16(MsbFirst[13] | MsbFirst[14] << 8);
        if ((cni_8_30_1 == 0xC0C0) || (cni_8_30_1 == 0xA101))
          cni_8_30_1 = 0; // known wrong values.

        if (cni_8_30_1 && GetCniNameFormat1() && CNI_DBG)
          dsyslog("cni_8_30_1 = 0x%.4x -> %s", cni_8_30_1, GetCniNameFormat1());

        if (++hits > MAXHITS)
          m_bStopped = true;
      }
      break;
      case 2:
      case 3:
      /* ETS 300 706: Table 19: Coding of Packet 8/30 Format 2 */
      {
        uint8_t Data[256];
        unsigned CNI_0, CNI_1, CNI_2, CNI_3;

        for (i = 0; i < 33; i++)
          Data[i] = RevertNibbles(DeHamming_8_4(MsbFirst[13+i]));

        CNI_0 = (Data[2] & 0xF) >> 0;
        CNI_2 = (Data[3] & 0xC) >> 2;
        CNI_1 = (Data[8] & 0x3) << 2 | (Data[9]  & 0xC) >> 2;
        CNI_3 = (Data[9] & 0x3) << 4 | (Data[10] & 0xF) >> 0;

        cni_8_30_2 = CNI_0 << 12 | CNI_1 << 8 | CNI_2 << 6 | CNI_3;

        if (cni_8_30_2 && GetCniNameFormat2() && CNI_DBG)
          dsyslog("cni_8_30_2 = 0x%.4x -> %s", cni_8_30_2, GetCniNameFormat2());

        if (++hits > MAXHITS)
          m_bStopped = true;
      }
      break;
      default:;
      } // end switch (DesignationCode)
    }
    break; // end 8/30/{1,2}

    default:
    break;
    }
  }
  break; // end EBU teletext

  default: // ignore wss && closed_caption
    break;
  } // end switch data_unit_id
}

void cSwReceiver::Decode(uint8_t* data, int count)
{
  if (count < 184)
    return;

  while (count >= 46 && !m_bStopped)
  {
    switch (*data) // EN 300472 Table 4 data_unit_id
    {
    case 0x02:    // EBU teletext
    case 0xC3:    // VPS line 16
      DecodePacket(data);
      break;
    default:      // wss, closed_caption, stuffing
      break;
    }

    data += 46;
    count -= 46;
    buffer->Del(46);
  }
}

void* cSwReceiver::Process()
{
  uint8_t* bp;
  int count;

  while (!IsStopped() & !m_bStopped)
  {
    count = 184;

    if ((bp = buffer->Get(count)))
    {
      Decode(bp, count);      // always N * 46bytes.
      TsCount++;
    }
    else
    {
      Sleep(10); // wait for new data.
    }
    if (CDateTime::GetUTCDateTime() > m_timeout)
      m_bStopped = true;
  }

  if (hits < MAXHITS)
  {
    // we're unshure about result.
    if ((hits < MINHITS) || (TsCount < 3))
    {
      // probably garbage. clear it.
      cni_8_30_1 = cni_8_30_2 = cni_X_26 = cni_vps = cni_cr_idx = 0;
      fuzzy = false;
    }
    else
      dsyslog("   fuzzy result, hits = %d, TsCount = %lu", hits, TsCount);
  }

  return NULL;
}

const char* cSwReceiver::GetCniNameFormat1()
{
  unsigned i;

  if (cni_8_30_1)
  {
    for (i = 0; i < CniCodes::GetCniCodeCount(); i++)
    {
      if (CniCodes::GetCniCode(i).ni_8_30_1 == cni_8_30_1)
        return CniCodes::GetCniCode(i).network;
    }
    if (cni_8_30_2 || cni_X_26 || cni_vps)
    {
      dsyslog("unknown 8/30/1 cni 0x%.4x (8/30/2 = 0x%.4x; X/26 = 0x%.4x, VPS = 0x%.4x; cr_idx = 0x%.4x) %s",
          cni_8_30_1, cni_8_30_2, cni_X_26, cni_vps, cni_cr_idx, "PLEASE REPORT TO WIRBELSCAN AUTHOR.");
    }
  }

  return NULL;
}

const char* cSwReceiver::GetCniNameFormat2()
{
  uint8_t  c = cni_8_30_2 >> 8;
  uint8_t  n = cni_8_30_2 & 0xFF;

  if (cni_8_30_2)
  {
    for (unsigned i = 0; i < CniCodes::GetCniCodeCount(); i++)
    {
      if ((CniCodes::GetCniCode(i).c_8_30_2 == c) && (CniCodes::GetCniCode(i).ni_8_30_2 == n))
        return CniCodes::GetCniCode(i).network;
    }
    if (cni_8_30_1 || cni_X_26 || cni_vps)
    {
      dsyslog("unknown 8/30/2 cni 0x%.4x (8/30/1 = 0x%.4x; X/26 = 0x%.4x, VPS = 0x%.4x; cr_idx = 0x%.4x) %s",
          cni_8_30_2, cni_8_30_1, cni_X_26, cni_vps, cni_cr_idx, "PLEASE REPORT TO WIRBELSCAN AUTHOR.");
    }
  }

  return NULL;
}

const char* cSwReceiver::GetCniNameVPS()
{
  if (cni_vps)
  {
    for (unsigned i = 0; i < CniCodes::GetCniCodeCount(); i++)
    {
      if (CniCodes::GetCniCode(i).vps__cni == cni_vps)
        return CniCodes::GetCniCode(i).network;
    }
    if (cni_8_30_1 || cni_8_30_2 || cni_X_26)
    {
      dsyslog("unknown VPS cni 0x%.4x (8/30/1 = 0x%.4x; 8/30/2 = 0x%.4x, X/26 = 0x%.4x; cr_idx = 0x%.4x) %s",
          cni_vps, cni_8_30_1, cni_8_30_2, cni_X_26, cni_cr_idx, "PLEASE REPORT TO WIRBELSCAN AUTHOR.");
    }
  }

  return NULL;
}

const char* cSwReceiver::GetCniNameX26()
{
  uint8_t a = cni_X_26 >> 8, b = cni_X_26 & 0xff;
 
  if (cni_X_26)
  {
    for (unsigned i = 0; i < CniCodes::GetCniCodeCount(); i++)
    {
      if ((CniCodes::GetCniCode(i).a_X_26 == a) && (CniCodes::GetCniCode(i).b_X_26 == b))
        return CniCodes::GetCniCode(i).network;
    }
    if (cni_8_30_1 || cni_8_30_2 || cni_vps)
    {
      dsyslog("unknown X/26 cni 0x%.4x (8/30/1 = 0x%.4x; 8/30/2 = 0x%.4x, VPS = 0x%.4x; cr_idx = 0x%.4x) %s",
          cni_X_26, cni_8_30_1, cni_8_30_2, cni_vps, cni_cr_idx, "PLEASE REPORT TO WIRBELSCAN AUTHOR.");
    }
  }

  return NULL;
}

const char * cSwReceiver::GetCniNameCrIdx()
{
  uint8_t c = cni_cr_idx >> 8, idx = cni_cr_idx & 0xff;
 
  if (cni_cr_idx)
  {
    for (unsigned i = 0; i < CniCodes::GetCniCodeCount(); i++)
    {
      if ((CniCodes::GetCniCode(i).c_8_30_2 == c) && (CniCodes::GetCniCode(i).cr_idx == idx))
        return CniCodes::GetCniCode(i).network;
    }
    if (cni_8_30_1 || cni_8_30_2 || cni_vps || cni_X_26)
    {
      if ((cni_cr_idx&255) >= 100) // Invalid anyway otherwise
      {
        dsyslog("unknown cr_idx %.2X%.3d (8/30/1 = 0x%.4x; 8/30/2 = 0x%.4x, VPS = 0x%.4x; X/26 = 0x%.4x) %s",
            cni_cr_idx>>8, cni_cr_idx&255, cni_8_30_1, cni_8_30_2, cni_vps, cni_X_26, "PLEASE REPORT TO WIRBELSCAN AUTHOR.");
      }
    }
  }
  
  return NULL;
}

void cSwReceiver::UpdatefromName(const char * name)
{
  uint8_t len = strlen(name);
  uint16_t cni_vps_id = 0;

  if (!len)
    return;

  for (unsigned i = 0; i < CniCodes::GetCniCodeCount(); i++)
  {
    if (strlen(CniCodes::GetCniCode(i).network) == len)
    {
      if (! strcasecmp(CniCodes::GetCniCode(i).network, name))
      {
        if (CniCodes::GetCniCode(i).vps__cni)
        {
          cni_vps_id = CniCodes::GetCniCode(i).vps__cni;
          if (! cni_vps_id)
            dsyslog("%s: missing cni_vps for ""%s""", __FUNCTION__, name);
          break;
        }
      }
    }
  }

  if (cni_vps_id)
  {
    cni_vps = cni_vps_id;
    fuzzy = true;
    len = strlen(GetCniNameVPS());
    strncpy(fuzzy_network, GetCniNameVPS(), len);
    fuzzy_network[len] = 0;
  }
  else
  {
    dsyslog("%s: unknown network name ""%s""", __FUNCTION__, name);
    fuzzy = true;
    strncpy(fuzzy_network, name, len);
    fuzzy_network[len] = 0;
  }

  hits++;
  if (++hits > MAXHITS)
    m_bStopped = true;
}

}
