/*
 * dvb_wrapper.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * wirbelscan-0.0.5
 *
 * $Id$ v20101210
 */

/* ARGHS! I hate that unsolved API problems with DVB..
 * Three different API approaches and one additional
 * wrapper to the old one for vdr-1.7.x 
 */

#ifndef __WIRBELSCAN_DVB_WRAPPER_H_
#define __WIRBELSCAN_DVB_WRAPPER_H_

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>
#include <vdr/dvbdevice.h>
#include <vdr/channels.h>
#include "extended_frontend.h"

typedef enum scantype {
  DVB_TERR    = 0,
  DVB_CABLE   = 1,
  DVB_SAT     = 2,
  PVRINPUT    = 3,
  PVRINPUT_FM = 4,
  DVB_ATSC    = 5,
  NO_DEVICE   = 6,
  TRANSPONDER = 999
} scantype_t;

//-------------------------------------------------------------------

// API independend enums
enum eCableSatCodeRates {
 eCoderateAuto = 0,
 eCoderate12   = 1,
 eCoderate23   = 2,
 eCoderate34   = 3,
 eCoderate56   = 4,
 eCoderate78   = 5,
 eCoderate89   = 6,
 eCoderate35   = 7,
 eCoderate45   = 8,
 eCoderate910  = 9,
 eCoderateNone = 15
};

enum eCableModulations {
 eModulationQamAuto = 0,
 eModulationQam16   = 1,
 eModulationQam32   = 2,
 eModulationQam64   = 3,
 eModulationQam128  = 4,
 eModulationQam256  = 5,
 eModulationQam512  = 6,
 eModulationQam1024 = 7
};

enum eAtscModulations {
 eVsb8   = 0,
 eVsb16  = 1,
 eQam64  = 2,
 eQam256 = 3,
 eQamAuto= 4,
};

enum eSatPolarizations {
 eHorizontal = 0,
 eVertical   = 1,
 eLeft       = 2,
 eRight      = 3
};

enum eSatRollOffs {
 eRolloff35   = 0,
 eRolloff25   = 1,
 eRolloff20   = 2,
 eRolloffAuto = 3
};

enum eSatSystems {
 eDvbs  = 0,
 eDvbs2 = 1
};

enum eSatModulationTypes {
 eSatModulationAuto  = 0,
 eSatModulationQpsk  = 1,
 eSatModulation8psk  = 2,
 eSatModulationQam16 = 3
};

enum eTerrBandwidths {
 eBandwidth8Mhz = 0,
 eBandwidth7Mhz = 1,
 eBandwidth6Mhz = 2,
 eBandwidth5Mhz = 3,
 eBandwidthAuto = 4
};

enum eTerrConstellations {
 eModulationQpsk = 0,
 eModulation16   = 1,
 eModulation64   = 2,
 eModulationAuto = 3
};

enum eTerrHierarchies {
 eHierarchyNone = 0,
 eHierarchy1    = 1,
 eHierarchy2    = 2,
 eHierarchy4    = 3,
 eHierarchyAuto = 8
};

enum eTerrCodeRates {
 eTerrCoderate12   = 0,
 eTerrCoderate23   = 1,
 eTerrCoderate34   = 2,
 eTerrCoderate56   = 3,
 eTerrCoderate78   = 4,
 eTerrCoderateAuto = 5,
 eTerrCoderateNone = 6
};

enum eTerrGuardIntervals {
 eGuardinterval32   = 0,
 eGuardinterval16   = 1,
 eGuardinterval8    = 2,
 eGuardinterval4    = 3,
 eGuardintervalAuto = 4
};

enum eTerrTransmissionModes {
 eTransmissionmode2K   = 0,
 eTransmissionmode8K   = 1,
 eTransmissionmode4K   = 2,
 eTransmissionmodeAuto = 3
};

enum eCableTerrInversions {
 eInversionOff  = 0,
 eInversionOn   = 1,
 eInversionAuto = 2
};

// wirbelscan enums -> dvb api
fe_code_rate_t          CableSatCodeRates(eCableSatCodeRates cr);
fe_modulation_t         CableModulations(eCableModulations cm);
fe_modulation_t         AtscModulations(eAtscModulations am);
fe_polarization_t       SatPolarizations(eSatPolarizations sp);
fe_rolloff_t            SatRollOffs(eSatRollOffs ro);
fe_delivery_system_t    SatSystems(eSatSystems ss);
fe_modulation_t         SatModulationTypes(eSatModulationTypes mt);
#if VDRVERSNUM >= 10700
int                     TerrBandwidths(eTerrBandwidths tb);
#else
fe_bandwidth_t          TerrBandwidths(eTerrBandwidths tb);
#endif
fe_modulation_t         TerrConstellations(eTerrConstellations tc);
fe_hierarchy_t          TerrHierarchies(eTerrHierarchies th);
fe_code_rate_t          TerrCodeRates(eTerrCodeRates cr);
fe_guard_interval_t     TerrGuardIntervals(eTerrGuardIntervals gi);
fe_transmit_mode_t      TerrTransmissionModes(eTerrTransmissionModes tm);
fe_spectral_inversion_t CableTerrInversions(eCableTerrInversions in);

// dvb api -> vdr channels.conf
int  GetVDRInversionFromDVB(fe_spectral_inversion_t Inversion);
int  GetVDRBandwidthFromDVB(fe_bandwidth_t Bandwidth);
int  GetVDRCoderateFromDVB(fe_code_rate_t Coderate);
int  GetVDRModulationtypeFromDVB(fe_modulation_t Modulationtype);
int  GetVDRSatSystemFromDVB(fe_delivery_system_t SatSystem);
int  GetVDRTransmissionModeFromDVB(fe_transmit_mode_t TransmissionMode);
int  GetVDRGuardFromDVB(fe_guard_interval_t Guard);
int  GetVDRHierarchyFromDVB(fe_hierarchy_t Hierarchy);
int  GetVDRRollOffFromDVB(fe_rolloff_t RollOff);
char GetVDRPolarizationFromDVB(fe_polarization_t Polarization);

// use with dvb api 
bool SetSatTransponderDataFromDVB(cChannel * channel, int Source, int Frequency, char Polarization, int Srate, int CoderateH, int Modulation, int System, int RollOff);
bool SetCableTransponderDataFromDVB(cChannel * channel, int Source, int Frequency, int Modulation, int Srate, int CoderateH, int Inversion);
bool SetTerrTransponderDataFromDVB(cChannel * channel, int Source, int Frequency, int Bandwidth, int Modulation, int Hierarchy, int CodeRateH, int CodeRateL, int Guard, int Transmission, int Inversion);

// use with wirbelscan enums
bool SetSatTransponderDataFromVDR(cChannel * channel, int Source, int Frequency, eSatPolarizations Polarization, int Srate, eCableSatCodeRates CoderateH, eSatModulationTypes Modulation, eSatSystems System, eSatRollOffs RollOff);
bool SetCableTransponderDataFromVDR(cChannel * channel, int Source, int Frequency, eCableModulations Modulation, int Srate, eCableSatCodeRates CoderateH, eCableTerrInversions Inversion);
bool SetTerrTransponderDataFromVDR(cChannel * channel, int Source, int Frequency, eTerrBandwidths Bandwidth, eTerrConstellations Modulation, eTerrHierarchies Hierarchy, eTerrCodeRates CodeRateH, eTerrCodeRates CodeRateL, eTerrGuardIntervals Guard, eTerrTransmissionModes Transmission, eCableTerrInversions Inversion);

void SetPids(cChannel * channel, int Vpid, int Ppid, int Vtype, int *Apids, int *Atypes, char ALangs[][MAXLANGCODE2], int *Dpids, int *Dtypes, char DLangs[][MAXLANGCODE2], int *Spids, char SLangs[][MAXLANGCODE2], int Tpid);

// debug print
cString PrintTransponder(const cChannel * Transponder);
cString PrintChannel(const cChannel * Channel);
cString PrintDvbApi(void);
cString PrintDvbApiUsed(int cardIndex);

// DVB frontend capabilities
unsigned int  GetFrontendStatus  (int cardIndex = 0);
unsigned int  GetFrontendStrength(int cardIndex = 0);
unsigned int  GetCapabilities    (int cardIndex = 0);
unsigned int  GetFeType          (int cardIndex = 0);
cString GetFeName(int cardIndex);
bool GetTerrCapabilities (int cardIndex, bool *CodeRate, bool *Modulation, bool *Inversion, bool *Bandwidth, bool *Hierarchy, bool *TransmissionMode, bool *GuardInterval);
bool GetCableCapabilities(int cardIndex, bool *CodeRate, bool *Modulation, bool *Inversion);
bool GetAtscCapabilities (int cardIndex, bool *CodeRate, bool *Modulation, bool *Inversion, bool *VSB, bool * QAM);
bool GetSatCapabilities  (int cardIndex, bool *CodeRate, bool *Modulation, bool *RollOff, bool *DvbS2); //DvbS2: true if supported.
bool ValidSatfreq        (int f, const cChannel * Channel);

// plugin && vdr capabilities/properties
bool IsPvrinput(const cChannel * Channel);
bool IsScantype(scantype_t type, const cChannel * c);
bool ActiveTimers(scantype_t type);
bool PendingTimers(scantype_t type, int Margin);

#endif
