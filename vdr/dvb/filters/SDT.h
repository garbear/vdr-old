/*
 * sdt.h: SDT section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sdt.h 2.0 2004/01/05 14:30:14 kls Exp $
 */

#ifndef __SDT_H
#define __SDT_H

#include "Types.h"
#include "Filter.h"
#include "PAT.h"

namespace VDR
{
class cSdtFilter : public cFilter
{
public:
  cSdtFilter(cDevice* device, cPatFilter* patFilter);
  virtual ~cSdtFilter() { }

  virtual void Enable(bool bEnabled);

protected:
  virtual void ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data);

private:
  cSectionSyncer    m_sectionSyncer;
  cPatFilter* const m_patFilter;
};

}

#endif //__SDT_H
