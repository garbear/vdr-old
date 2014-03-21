/*
 * nit.h: NIT section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: nit.h 2.0 2007/06/10 08:50:21 kls Exp $
 */

#pragma once

#include "Types.h"
#include "Filter.h"

#include <vector>

// From UTF8Utils.h
#define Utf8BufSize(s) ((s) * 4)

//#define MAXNITS 16
#define MAXNETWORKNAME Utf8BufSize(256)

namespace VDR
{
class cNitFilter : public cFilter
{
public:
  cNitFilter(cDevice* device);
  virtual ~cNitFilter(void) { }

  virtual void Enable(bool bEnabled);

protected:
  virtual void ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data);

private:
  class cNit
  {
  public:
    u_short networkId;
    char    name[MAXNETWORKNAME];
    bool    hasTransponder;
  };

  cSectionSyncer    m_sectionSyncer;
  u_short           m_networkId;
  std::vector<cNit> m_nits;
};

}
