/*
 * eit.h: EIT section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: eit.h 2.1 2010/01/03 15:28:34 kls Exp $
 */

#ifndef __EIT_H
#define __EIT_H

#include "Types.h"
#include "Filter.h"
#include "utils/DateTime.h"

namespace VDR
{
class cEitFilter : public cFilter
{
public:
  cEitFilter(cDevice* device);
  virtual ~cEitFilter(void) { }

protected:
  virtual void ProcessData(u_short pid, u_char tid, const std::vector<uint8_t>& data);
  static void SetDisableUntil(const CDateTime& time);

private:
  static CDateTime m_disableUntil;
};

}

#endif //__EIT_H
