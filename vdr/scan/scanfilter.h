#ifndef __WIRBELSCAN_SCANFILTER_H_
#define __WIRBELSCAN_SCANFILTER_H_

#include <libsi/si.h>
#include <vdr/receiver.h>
#include <vdr/filter.h>
#include <vdr/pat.h>
#include <linux/dvb/frontend.h>
#include "caDescriptor.h"
#include "si_ext.h"

#define _MAXNITS                              16
#define _MAXNETWORKNAME                       Utf8BufSize(256)
#define MAX_PMTS                              256 //max allowed PMTs per Transponder

#define SCANNED_CHANNEL                       0x8000
#define INVALID_CHANNEL                       0x4000
#define NO_LOCK                               0x2000

//#define DEBUGVERSION 1

#ifdef DEBUGVERSION
#define HEXDUMP(d, l)    for (int i = 0; i < l; i++) {if (!(i % 16)) {printf("%s0x%.4x:    ", i ? "\n    " : "    ", (i / 16) * 16);} printf("%.2x ", *(d + i));} printf("\n");
#else
#define HEXDUMP(d, l)
#endif

//---------cTransponders---------------------------------------------------------------------
class cTransponders : public cChannels {
private:
protected:
public:
  bool       IsUniqueTransponder(const cChannel * NewTransponder);
  cChannel * GetByParams(const cChannel * NewTransponder);
  cChannel * NextTransponder(void);
  };

extern cTransponders NewTransponders;
extern cTransponders ScannedTransponders;
extern cChannels     NewChannels;
extern int nextTransponders;

//--------------------------------------------------------------------------------------------

bool       is_known_initial_transponder(cChannel * newChannel, bool auto_allowed, cChannels * list = NULL);
bool       is_nearly_same_frequency(const cChannel * chan_a, const cChannel * chan_b, uint delta = 2001);
bool       is_different_transponder_deep_scan(const cChannel * a, const cChannel * b, bool auto_allowed);
cChannel * GetByTransponder(const cChannel * Transponder);
int        ServicesOnTransponder(const cChannel * Transponder);
int        AddChannels();
void       resetLists();

//--------cPatScanner-------------------------------------------------------------------------
class cPmtScanner;

class cPatScanner : public cFilter, public cThread {
private:
  bool          active;
  cPmtScanner * cPmtScanners[MAX_PMTS];
  cDevice *     parent;
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char * Data, int Length);
  virtual void Action(void);
public:
  cPatScanner(cDevice * Parent);
  ~cPatScanner();
  bool Active(void) {
    return (active);
    };
  };

//--------cNitScanner-------------------------------------------------------------------------

class cNitScanner : public cFilter, public cThread {
private:

  class cNit {
public:
    u_short networkId;
    char    name[MAXNETWORKNAME];
    bool    hasTransponder;
    };
  cSectionSyncer syncNit;
  cNit           nits[_MAXNITS];
  u_short        networkId;
  int            numNits;
  int            tableId;
  bool           active;
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char * Data, int Length);
  virtual void Action(void);
public:
  cNitScanner(int TableId);
  ~cNitScanner();
  bool Active(void) {
    return (active);
    };
  };

//--------cPmtScanner-------------------------------------------------------------------------

class cPmtScanner : public cFilter {
private:
  bool       active;
  int        pmtPid;
  int        pmtSid;
  cChannel * Channel;
  int        numPmtEntries;
  bool PmtVersionChanged(int PmtPid, int Sid, int Version);
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char * Data, int Length);
public:
  bool Active(void) {
    return (active);
    };
  cPmtScanner(cChannel * channel, u_short Sid, u_short PmtPid);
  };

//--------cSdtScanner-------------------------------------------------------------------------

class cSdtScanner : public cFilter, public cThread {
private:
  bool           active;
  int            tableId;
  cSectionSyncer sectionSyncer;
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char * Data, int Length);
  virtual void Action(void);
public:
  bool Active(void) { return (active); };
  cSdtScanner(int TableId);
  ~cSdtScanner();
  };

//--------cEitScanner-------------------------------------------------------------------------

class cEitScanner : public cFilter, public cThread {
private:
  bool          active;
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);
  virtual void Action(void);
public:
  bool Active(void) { return (active); };
  cEitScanner(void);
  ~cEitScanner();
  };


#endif
