/*
 * common.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __WIRBELSCAN_COMMON_H_
#define __WIRBELSCAN_COMMON_H_

#include <linux/types.h>
#include <sys/ioctl.h>
#include "dvb_wrapper.h"


#define SCAN_TV        ( 1 << 0 )
#define SCAN_RADIO     ( 1 << 1 )
#define SCAN_FTA       ( 1 << 2 )
#define SCAN_SCRAMBLED ( 1 << 3 )
#define SCAN_HD        ( 1 << 4 )

#define STDOUT                  1
#define SYSLOG                  2

#define ADAPTER_AUTO            0

#define DVBC_INVERSION_AUTO     0
#define DVBC_QAM_AUTO           0
#define DVBC_QAM_64             1
#define DVBC_QAM_128            2
#define DVBC_QAM_256            3

#define CHANNEL_SYNTAX_PVRINPUT      0
#define CHANNEL_SYNTAX_PVRINPUT_IPTV 1

#define MAXSIGNALSTRENGTH       65535
#define MINSIGNALSTRENGTH       16383

class cMySetup {
 private:
 public:
  int verbosity;
  int logFile;
  scantype_t DVB_Type;
  int DVBT_Inversion;
  int DVBC_Inversion;
  int DVBC_Symbolrate;
  int DVBC_QAM;
  int CountryIndex;
  int SatIndex;
  int enable_s2;
  int ATSC_type;
  uint32_t scanflags;
  bool update;
  uint32_t user[3];
  int systems[6];
  bool initsystems;
  bool enable_pvrinput;
  cMySetup(void);
  void InitSystems();
};
extern cMySetup wSetup;


/* generic functions */
void  dlog(const int level, const char *fmt, ...);
void  hexdump(const char * intro, const unsigned char * buf, int len);
int   IOCTL(int fd, int cmd, void * data);
bool  FileExists(const char * aFile);

#define closeIfOpen(aFile)      if (aFile    >= 0)    { close(aFile);      aFile = -1;    }
#define verbose(x...) dlog(4, x)
#define fatal(x...)   dlog(0, x); return -1

const char * vdr_inversion_name(int inversion);
const char * vdr_fec_name(int fec);
const char * vdr17_modulation_name(int modulation);
const char * vdr_modulation_name(int modulation);
const char * vdr_bandwidth_name(int bandwidth);
const char * vdr_transmission_mode_name(int transmission_mode);
const char * vdr_guard_name(int guard_interval);
const char * vdr_hierarchy_name(int hierarchy);
const char * vdr_name_to_short_name(const char * satname);
const char * xine_transmission_mode_name (int transmission_mode);
const char * xine_modulation_name(int modulation);
const char * atsc_mod_to_txt ( int id );
int dvbc_modulation(int index);
int dvbc_symbolrate(int index);
void InitSystems(void);

#endif
