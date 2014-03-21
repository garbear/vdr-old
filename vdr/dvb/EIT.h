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
class cEitFilter : public cFilter {
private:
  static CDateTime disableUntil;
protected:
  virtual void ProcessData(u_short Pid, u_char Tid, const u_char *Data, int Length);
public:
  cEitFilter(void);
  static void SetDisableUntil(const CDateTime& Time);
  };
}

#endif //__EIT_H
