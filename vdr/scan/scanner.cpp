/*
 * scanner.c: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$ v20101210
 */

#include <stdarg.h>
#include <stdlib.h>
#include <linux/dvb/frontend.h>
#include <linux/videodev2.h>
#include <vdr/sources.h>
#include <vdr/eitscan.h>
#include <vdr/channels.h>
#include <vdr/tools.h>
#include <vdr/svdrp.h>
#include <vdr/device.h>
#include "scanner.h"
#include "menusetup.h"
#include "common.h"
#include "satellites.h"
#include "dvb_wrapper.h"
#include "scanfilter.h"
#include "statemachine.h"
#include "countries.h"
#include "frequencies.h"
#include "ext_math.h"
#include "ttext.h"
#include "wirbelscan_services.h"

using namespace COUNTRY;

int initialTransponders;

static unsigned int chan_to_freq(int channel, int channellist) {
  if (channellist == 999)
       
  dlog(4, "channellist=%d, base_offset=%d, channel=%d, step=%d",
       channellist, base_offset(channel, channellist),
       channel, freq_step(channel, channellist));
  if (base_offset(channel, channellist) != -1) { // -1 == invalid
    return (base_offset(channel, channellist) + channel * freq_step(channel, channellist));
    }
  return (0);
  }

static bool IsTvChannel(int f) {
  return ((f > 108000) || (f < 87500));
  }

static int device_is_preferred(int caps, cString frontend_name) {
  int preferred = 1; // no preferrence

  /* add other good/bad cards here. */
  if (strncmp("VLSI VES1820", *frontend_name, 12) == 0) {
    /* bad working FF dvb-c card, known to have qam256 probs. */
    preferred = 0; // not preferred
    }
  if (caps & FE_CAN_2G_MODULATION) {
    /* w_scan/wirbelscan preferres devices which are DVB-{S,C,T}2 */
    preferred = 2; // preferred
    }
  return (preferred);
  }

static unsigned int FeType(cChannel * Channel) {
  if (Channel->IsCable()) return (FE_QAM);
  if (Channel->IsSat())   return (FE_QPSK);
  if (Channel->IsTerr())  return (FE_OFDM);
  #if VDRVERSNUM > 10713
  if (Channel->IsAtsc())  return (FE_ATSC);
  #endif
  return (0xffff);
  }

cDevice * GetPreferredDevice(cChannel * Channel) {
  cDevice * dev         = NULL;
  int       preferred   = 0;
  int       pref_device = -1;
  uint      caps;
  cString   name;

  for (int i = 0; i < MAXDEVICES; i++) {
    if ((dev = cDevice::GetDevice(i)) == NULL) {
      continue;
      }
    if (!dev->ProvidesChannel(Channel)) {
      continue;
      }
    if (GetFeType(dev->CardIndex()) != FeType(Channel)) {
      continue;
      }
    name = cString::sprintf("%s", *GetFeName(dev->CardIndex()));
    caps = GetCapabilities(dev->CardIndex());
    dlog(1, "device %d = %s", dev->CardIndex(), *name);
    if (device_is_preferred(caps, name) >= preferred) {
      preferred   = device_is_preferred(caps, name);
      pref_device = i;
      }
    switch (preferred) {
       case 0: // device known to have probs. usable anyway..
         dlog(1, "usable :-|");
         break;
       case 1: // device w/o problems
         dlog(1, "good :-)");
         break;
       case 2: // perfect device found. stop scanning
         dlog(1, "very good :-))");
         return (dev);
         break;
       default:;
      }
    }
  if (pref_device >= 0) {
    dev = cDevice::GetDevice(pref_device);
    return (dev);
    }
  return (NULL);
  }

int PvrHasLock(int timeout, int videodev) {
  struct v4l2_tuner tuner;
  cString           devname = cString::sprintf("/dev/video%d", videodev);
  int fd = open(*devname, O_RDWR);

  if (fd < 0) {
    return (false);
    }

  tuner.index = 0;
  tuner.type  = V4L2_TUNER_ANALOG_TV;

  if (ioctl(fd, VIDIOC_G_TUNER, &tuner) == 0) {
    while (timeout > 0) {
      cCondWait::SleepMs(10);
      timeout -= 10;
      ioctl(fd, VIDIOC_G_TUNER, &tuner);
      if (tuner.signal > MINSIGNALSTRENGTH) {
        break;
        }
      }
    }
  close(fd);
  return ((tuner.signal > MINSIGNALSTRENGTH) ? tuner.signal : 0);
  }

int PvrGetSignal(int videodev) {
  struct v4l2_tuner tuner;
  cString           devname = cString::sprintf("/dev/video%d", videodev);
  int fd = open(*devname, O_RDWR);

  if (fd < 0) {
    return (false);
    }

  tuner.index = 0;
  tuner.type  = V4L2_TUNER_ANALOG_TV;

  ioctl(fd, VIDIOC_G_TUNER, &tuner);
  close(fd);
  return (tuner.signal);
  }


cScanner::cScanner(const char * Description, scantype_t Type) {
  type       = Type;
  shouldstop = false;
  single     = false;
  aChannel   = NULL;
  dev        = NULL;
  status     = 0;
  StateMachine = NULL;
  initialTransponders = 0;
  Start();
  }

cScanner::~cScanner(void) {
  dlog(3, "destroying scanner");
  Scanner = NULL;
  }

void cScanner::Action(void) {
  bool          crAuto, modAuto, invAuto, bwAuto, hAuto, tmAuto, gAuto, roAuto, s2Support, vsbSupport, qamSupport, vbiSupport = false;
  bool          useNit = true;
  int           f = 0, devfd = -1, videodev = -1;
  int           mod_parm, modulation_min = 0, modulation_max = 1;
  int           sr_parm, dvbc_symbolrate_min = 0, dvbc_symbolrate_max = 1;
  int           channel, channel_min = 0, channel_max = 133;
  int           offs, freq_offset_min = 0, freq_offset_max = 2;
  int           this_channellist = DVBT_DE, this_bandwidth = 8000000, this_qam = QAM_AUTO, atsc = ATSC_VSB, dvb;
  uint16_t      frontend_type = FE_QPSK;
  int           qam_no_auto      = 0, this_atsc = 0;
  int           thisChannel      = 0; 

  const char *  country   = country_to_short_name(wSetup.CountryIndex);
  const char *  satellite = satellite_to_short_name(wSetup.SatIndex);
  cString       channelname, shortname;
  cSwReceiver * SwReceiver = NULL;

  int           caps_inversion = INVERSION_OFF, caps_qam = QAM_AUTO, caps_hierarchy = HIERARCHY_NONE;
  int           caps_fec       = FEC_AUTO, caps_guard_interval = GUARD_INTERVAL_AUTO, caps_transmission_mode = TRANSMISSION_MODE_AUTO;
  int           caps_s2        = 0, caps_vsb = 0;

  double *      vals = NULL;
  int           col  = 0;
  int           row  = 0;

  resetLists();
  initialTransponders = 0;
  status = 1;
  dlog(1, "Running on VDR %s (%s)", VDRVERSION,
     #ifdef PLUGINPARAMPATCHVERSNUM
       "with IPTV patch. :-)");
     #elif (VDRVERSNUM < 10713)
       "IPTV patch is missing. :-(");
     #else
       "with internal Plugin support. :-)");
     #endif
  dlog(1, "wirbelscan version %s", extVer);

  switch (type) {
     case TRANSPONDER:
       {
       using namespace WIRBELSCAN_SERVICE;
       cUserTransponder * t = new cUserTransponder(&wSetup.user[0]);
       this_channellist = USERLIST;
       useNit = t->UseNit();

       // disable all loops
       modulation_min      = modulation_max      = 0;
       dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
       channel_min         = channel_max         = 1;
       freq_offset_min     = freq_offset_max     = 0;
       single = true;
       thisChannel = -1;

       aChannel = new cChannel;

       switch ((scantype_t) t->Type()) {
          case  DVB_TERR:
               dvb = frontend_type = FE_OFDM;
               SetTerrTransponderDataFromDVB(aChannel, cSource::stTerr, t->Frequency(), t->Bandwidth(), t->Modulation(),
                                     t->Hierarchy(), t->FecHP(), t->FecLP(), t->Guard(), t->Transmission(), t->Inversion());
               break;
          case  DVB_CABLE:
               dvb = frontend_type = FE_QAM;
               SetCableTransponderDataFromDVB(aChannel, cSource::stCable, t->Frequency(), t->Modulation(),
                                    t->Symbolrate(), 0, t->Inversion());
               break;
          case  DVB_SAT:
               dvb = frontend_type = FE_QPSK;
               SetSatTransponderDataFromDVB(aChannel,
                                    cSource::FromString(sat_list[t->Id()].source_id), t->Frequency(),
                                    GetVDRPolarizationFromDVB((fe_polarization_t) t->Polarisation()), t->Symbolrate(), t->FecHP(),
                                    t->Modulation(), t->Satsystem(), t->Rolloff());
               break;
          case  DVB_ATSC:
               dvb = frontend_type = FE_ATSC;
               SetCableTransponderDataFromDVB(aChannel, cSource::stCable, t->Frequency(), t->Modulation(),
                                    t->Symbolrate(), 0, t->Inversion());
               break;
          default:
               esyslog("unsupported user transponder type.\n");
          }
       if ((dev = GetPreferredDevice(aChannel)) == NULL) {
         dlog(0, "No device available - exiting!");
         if (MenuScanning) MenuScanning->SetStatus((status = 2));
         DELETENULL(aChannel);
         return;
         }
       deviceName = cString::sprintf("%s", *GetFeName(dev->CardIndex()));
       dlog(1, "frontend %s", *deviceName);     
       if (MenuScanning) MenuScanning->SetDeviceInfo(deviceName);
       break;
       }
     case DVB_TERR:
       {
       dvb = FE_OFDM;
       frontend_type = FE_OFDM;
       choose_country(country, &atsc, &dvb, &frontend_type, &this_channellist);
       aChannel = new cChannel;
       /* find a dvb-t capable device using *some* channel */
       SetTerrTransponderDataFromVDR(aChannel, cSource::stTerr, 474000, eBandwidth8Mhz, eModulation64,
                                     eHierarchyNone, eTerrCoderate23, eTerrCoderate23, eGuardintervalAuto, eTransmissionmode8K,
                                     eInversionOff);
       if ((dev = GetPreferredDevice(aChannel)) == NULL) {
         dlog(0, "No device available - exiting!");
         if (MenuScanning) MenuScanning->SetStatus((status = 2));
         DELETENULL(aChannel);
         return;
         }
       dlog(1, "%s", *PrintDvbApi());
       dlog(1, "%s", *PrintDvbApiUsed(dev->CardIndex()));
       if (! GetTerrCapabilities(dev->CardIndex(), &crAuto, &modAuto, &invAuto, &bwAuto, &hAuto, &tmAuto, &gAuto))
          dlog(0, "ERROR: Could not query capabilites.");

       deviceName = cString::sprintf("%s", *GetFeName(dev->CardIndex()));
       dlog(1, "frontend %s supports", *deviceName);     
       if (MenuScanning) MenuScanning->SetDeviceInfo(deviceName);

       if (invAuto) {
         dlog(1, "INVERSION_AUTO");
         caps_inversion = INVERSION_AUTO;
         }
       else {
         if (wSetup.DVBT_Inversion == 0) {
            dlog(1, "INVERSION_AUTO not supported, trying INVERSION_OFF.");
            caps_inversion = INVERSION_OFF;
            }
         else {
            dlog(1, "INVERSION_AUTO not supported, trying INVERSION_ON.");
            caps_inversion = INVERSION_ON;
            }
         }
       if (modAuto) {
         dlog(1, "QAM_AUTO");
         caps_qam = QAM_AUTO;
         }
       else {
         dlog(1, "QAM_AUTO not supported, trying QAM_64.");
         caps_qam = QAM_64;
         }
       if (tmAuto) {
         dlog(1, "TRANSMISSION_MODE_AUTO");
         caps_transmission_mode = TRANSMISSION_MODE_AUTO;
         }
       else {
         caps_transmission_mode = dvbt_transmission_mode(5, this_channellist);
         dlog(1, "TRANSMISSION_MODE not supported, trying %s.",
              xine_transmission_mode_name(caps_transmission_mode));
         }
       if (gAuto) {
         dlog(1, "GUARD_INTERVAL_AUTO");
         caps_guard_interval = GUARD_INTERVAL_AUTO;
         }
       else {
         dlog(1, "GUARD_INTERVAL_AUTO not supported, trying GUARD_INTERVAL_1_8.");
         caps_guard_interval = GUARD_INTERVAL_1_8;
         }
       if (hAuto) {
         dlog(1, "HIERARCHY_AUTO");
         caps_hierarchy = HIERARCHY_AUTO;
         }
       else {
         dlog(1, "HIERARCHY_AUTO not supported, trying HIERARCHY_NONE.");
         caps_hierarchy = HIERARCHY_NONE;
         }
       if (crAuto) {
         dlog(1, "FEC_AUTO");
         caps_fec = FEC_AUTO;
         }
       else {
         dlog(1, "FEC_AUTO not supported, trying FEC_NONE.");
         caps_fec = FEC_NONE;
         }
       // disable qam loop, disable symbolrate loop
       modulation_min      = modulation_max = 0;
       dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
       break;
       }
     case DVB_CABLE:
       {
       dvb = FE_QAM;
       frontend_type = FE_QAM;
       choose_country(country, &atsc, &dvb, &frontend_type, &this_channellist);
       aChannel = new cChannel;
       /* find a dvb-c capable device using *some* channel */
       SetCableTransponderDataFromVDR(aChannel, cSource::stCable, 410000, eModulationQam64, 6900,
                                      eCoderateNone, eInversionOff);
       if ((dev = GetPreferredDevice(aChannel)) == NULL) {
         dlog(0, "No device available - exiting!");
         if (MenuScanning) MenuScanning->SetStatus((status = 2));
         DELETENULL(aChannel);
         return;
         }
       dlog(1, "%s", *PrintDvbApi());
       dlog(1, "%s", *PrintDvbApiUsed(dev->CardIndex()));
       if (! GetCableCapabilities(dev->CardIndex(), &crAuto, &modAuto, &invAuto))
          dlog(0, "ERROR: Could not query capabilites.");

       deviceName = cString::sprintf("%s", *GetFeName(dev->CardIndex()));
       dlog(1, "frontend %s supports", *deviceName);
       if (MenuScanning) MenuScanning->SetDeviceInfo(deviceName);

       if (invAuto) {
         dlog(1, "INVERSION_AUTO");
         caps_inversion = INVERSION_AUTO;
         }
       else {
         if (wSetup.DVBC_Inversion == 0) {
            dlog(1, "INVERSION_AUTO not supported, trying INVERSION_OFF.");
            caps_inversion = INVERSION_OFF;
            }
         else {
            dlog(1, "INVERSION_AUTO not supported, trying INVERSION_ON.");
            caps_inversion = INVERSION_ON;
            }
         }
       if (modAuto) {
         dlog(1, "QAM_AUTO");
         caps_qam = QAM_AUTO;
         }
       else {
         cString buf;
         for (int i = modulation_min; i <= modulation_max; i++)
           if (*buf) {
             buf = cString::sprintf("%s, %s", *buf, xine_modulation_name(dvbc_modulation(i)));
             }
           else {
             buf = cString::sprintf("%s", xine_modulation_name(dvbc_modulation(i)));
             }
         dlog(1, "QAM_AUTO not supported, trying %s.", *buf);
         caps_qam    = QAM_64;
         qam_no_auto = 1;
         }
       caps_fec = FEC_NONE;
       switch(wSetup.DVBC_Symbolrate) {
          case 0: // auto
             dvbc_symbolrate_min = 0;
             dvbc_symbolrate_max = 1;
             break;
          case 1 ... 15:
             dvbc_symbolrate_min = dvbc_symbolrate_max = wSetup.DVBC_Symbolrate - 1;
             break;
          default:// all
             dvbc_symbolrate_min = 0;
             dvbc_symbolrate_max = 14;
             break;
          }

       break;
       }
     case DVB_SAT:
       {
       bool res;
       dvb = FE_QPSK;
       frontend_type = FE_QPSK;
       choose_satellite(satellite, &this_channellist);
       aChannel = new cChannel;
       /* find a dvb-s2 capable device using *some* channel */

       res = SetSatTransponderDataFromDVB(aChannel,
                                    cSource::FromString(sat_list[this_channellist].source_id),
                                    sat_list[this_channellist].items[0].intermediate_frequency,
                                    GetVDRPolarizationFromDVB(sat_list[this_channellist].items[0].polarization),
                                    27500, FEC_5_6, 9, 6, 0); // 9 = PSK-8, 6 = DVBS2, 0 = Rolloff 0.35
       if (!res || (dev = cDevice::GetDevice(aChannel, 100, 1)) == NULL) {
         dlog(0, "No DVB-S2 device available - trying fallback to DVB-S");
         if (MenuScanning) MenuScanning->SetStatus(3);

         SetSatTransponderDataFromDVB(aChannel,
                                      cSource::FromString(sat_list[this_channellist].source_id),
                                      sat_list[this_channellist].items[0].intermediate_frequency,
                                      GetVDRPolarizationFromDVB(sat_list[this_channellist].items[0].polarization),
                                      27500, FEC_5_6, QPSK, 5, 0); // 5 = QPSK, 5 = DVBS, 0 = Rolloff 0.35

         if ((dev = GetPreferredDevice(aChannel)) == NULL) {
           dlog(0, "No device available - exiting!");
           if (MenuScanning) MenuScanning->SetStatus((status = 2));
           DELETENULL(aChannel);
           return;
           }
         }
       dlog(1, "%s", *PrintDvbApi());
       dlog(1, "%s", *PrintDvbApiUsed(dev->CardIndex()));
       if (! GetSatCapabilities(dev->CardIndex(), &crAuto, &modAuto, &roAuto, &s2Support))
          dlog(0, "ERROR: Could not query capabilites.");

       deviceName = cString::sprintf("%s", *GetFeName(dev->CardIndex()));
       dlog(1, "frontend %s supports", *deviceName);
       if (MenuScanning) MenuScanning->SetDeviceInfo(deviceName);

       caps_inversion = INVERSION_AUTO;
       if (crAuto) {
         dlog(1, "FEC_AUTO");
         caps_fec = FEC_AUTO;
         }
       dlog(1, "DVB-S");
       if (s2Support) {
         dlog(1, "DVB-S2");
         caps_s2 = 1;
         }
       else {
         // 20101210: 0.0.5-2, assume no S2 at all.
         wSetup.enable_s2 = false;
         }

       // channel means here: transponder,
       // last channel == (item_count - 1) since we're counting from 0
       channel_max = sat_list[this_channellist].item_count - 1;
       // disable qam loop
       modulation_min = modulation_max = 0;
       // disable symbolrate loop
       dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
       // disable freq offset loop
       freq_offset_min = freq_offset_max = 0;
       break;
       }
     case DVB_ATSC:
       {
       int atsc = 1 + wSetup.ATSC_type;
       frontend_type = FE_ATSC;
       choose_country(country, &atsc, &dvb, &frontend_type, &this_channellist);
       aChannel = new cChannel;

       /* TODO: distinguish between dvb-c && atsc qam */
       SetCableTransponderDataFromVDR(aChannel, cSource::stCable, 474000, eModulationQam256, 6900000, eCoderateNone, eInversionOff);
       if ((dev = GetPreferredDevice(aChannel)) == NULL) {
         dlog(0, "No device available - exiting!");
         if (MenuScanning) MenuScanning->SetStatus((status = 2));
         DELETENULL(aChannel);
         return;
         }
       dlog(1, "%s", *PrintDvbApi());
       dlog(1, "%s", *PrintDvbApiUsed(dev->CardIndex()));
       if (! GetAtscCapabilities(dev->CardIndex(), &crAuto, &modAuto, &invAuto, &vsbSupport, &qamSupport))
          dlog(0, "ERROR: Could not query capabilites.");

       deviceName = cString::sprintf("%s", *GetFeName(dev->CardIndex()));
       dlog(1, "frontend %s supports", *deviceName);
       if (MenuScanning) MenuScanning->SetDeviceInfo(deviceName);

       if (invAuto) {
         dlog(1, "INVERSION_AUTO\n");
         caps_inversion = INVERSION_AUTO;
         }
       else {
         dlog(1, "INVERSION_AUTO not supported, trying INVERSION_OFF.");
         caps_inversion = INVERSION_OFF;
         }
       if (vsbSupport) {
         dlog(1, "VSB");
         caps_vsb = 1;
         }
       if (qamSupport) {
         dlog(1, "QAM");
         caps_qam = 1;
         }
       switch (1 + wSetup.ATSC_type) {
          case ATSC_VSB:
            modulation_min = modulation_max = ATSC_VSB;
            break;
          case ATSC_QAM:
            modulation_min = modulation_max = ATSC_QAM;
            break;
          default:
            modulation_min = ATSC_VSB;
            modulation_max = ATSC_QAM;
            break;
         }
       // disable symbolrate loop
       dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
       break;
       }
     case PVRINPUT:
     case PVRINPUT_FM:
       {
       cString                devname;
       struct v4l2_capability vcap;
       struct v4l2_frequency  vf;
       struct v4l2_tuner      tuner;

       if (type == PVRINPUT) {
         choose_country_analog(country, &this_channellist);
         }
       else {
         choose_country_analog_fm(country, &this_channellist);
         vals    = (double *) malloc(freqlists[this_channellist].freqlist_count * 2 * sizeof(double));
         channel = 0;
         }
       // disable modulation loop
       modulation_min = modulation_max = 0;
       // disable offset loop
       freq_offset_min = freq_offset_max = 0;
       // disable symbolrate loop
       dvbc_symbolrate_min = dvbc_symbolrate_max = 0;
       channel_min         = 0;
       channel_max         = freqlists[this_channellist].freqlist_count - 1;

       aChannel = new cChannel;
       /* find a pvr capable device using *some* channel */
       #if VDRVERSNUM>10713
       aChannel->Parse("N24:250000:TV:V:0:301:300:305:0:1:0:8432:62");       
       #elif defined (PLUGINPARAMPATCHVERSNUM)     
       aChannel->Parse("N24:250000:PVRINPUT|TV:P:0:301:300:305:0:8436:0:0:62");
       #else
       dlog(0, "WARNING: YOUR VDR CORE MISSES THE IPTV PATCH - FUTURE VERSIONS OF PVRINPUT\n"
               "         WILL NO LONGER WORK WITHOUT THIS PATCH");
       aChannel->Parse("N24:250000:C0:C:0:301:300:305:A1:8436:0:0:62");
       #endif
       dlog(3, "searching device for channel %s", *PrintChannel(aChannel));
       if ((dev = cDevice::GetDevice(aChannel, 100, 1)) == NULL) {
         dlog(0, "No device available - exiting! (pvrinput not running?)");
         if (MenuScanning) MenuScanning->SetStatus((status = 2));
         DELETENULL(aChannel);
         if (type == PVRINPUT_FM)
            free(vals);
         return;
         }
       dev->SwitchChannel(aChannel, false);
       SwReceiver = new cSwReceiver(aChannel);
       dev->AttachReceiver(SwReceiver);
       cCondWait::SleepMs(2000);
       /*
        * searching the video device. There's no easy way to find out.. :(
        */
       for (int i = 0; i < 8; i++) {
         devname = cString::sprintf("/dev/video%d", i);
         devfd   = open(*devname, O_RDWR);
         if (devfd < 1)
           continue;
         if (ioctl(devfd, VIDIOC_QUERYCAP, &vcap)) {
           dlog(1, "quering capabilities %s failed.", *devname);
           close(devfd);
           devfd = -1;
           continue;
           }
         if (strncasecmp((char *) vcap.driver, "ivtv"   , 4) &&
             strncasecmp((char *) vcap.driver, "pvrusb2", 7) &&
             strncasecmp((char *) vcap.driver, "cx18",    4))
           continue;
         videodev = i;
         ioctl(devfd, VIDIOC_G_TUNER, &tuner);
         vf.tuner     = 0;
         vf.type      = tuner.type;
         vf.frequency = 0;
         ioctl(devfd, VIDIOC_G_FREQUENCY, &vf);
         close(devfd);
         if ((vf.frequency * 62500) == 250000000)
           dlog(1, "device %s (%s)", *devname, (char *) vcap.card);
         else
           continue;
         cCondWait::SleepMs(2000);
         vbiSupport = vcap.capabilities & (V4L2_CAP_VBI_CAPTURE || V4L2_CAP_SLICED_VBI_CAPTURE);
         if (vbiSupport)
           dlog(1, "device can capture vbi");
         }
       if (videodev < 0) {
         dlog(0, "pvr video device not found - giving up.");
         return;
         }

       deviceName = cString::sprintf("%s", vcap.card);
       if (MenuScanning) MenuScanning->SetDeviceInfo(deviceName);

       dev->DetachAllReceivers();

       break;
       }
    default:
       dlog(0, "ERROR: Unknown scan type %d", type);
       return;
    } // end switch type

  if (MenuScanning) MenuScanning->SetStatus(1);

  //count channels.

  switch(type) {
    case DVB_SAT:
    case PVRINPUT:
    case PVRINPUT_FM:
    case TRANSPONDER:
      initialTransponders = channel_max;
      break;
    default:
      // number depends on offset and symbolrates; counting in nested loops is easiest way.
      for (mod_parm = modulation_min; mod_parm <= modulation_max; mod_parm++)
        for (channel = channel_min; channel <= channel_max; channel++)
          for (offs = freq_offset_min; offs <= freq_offset_max; offs++)
            for (sr_parm = dvbc_symbolrate_min; sr_parm <= dvbc_symbolrate_max; sr_parm++) {
              if ((! chan_to_freq(channel, this_channellist)) ||
                  (freq_offset(channel, this_channellist, offs) == -1))
                continue;
              ++initialTransponders;
              }
    }


  for (mod_parm = modulation_min; mod_parm <= modulation_max; mod_parm++) {
    for (channel = channel_min; channel <= channel_max; channel++) {
      for (offs = freq_offset_min; offs <= freq_offset_max; offs++)
        for (sr_parm = dvbc_symbolrate_min; sr_parm <= dvbc_symbolrate_max; sr_parm++) {
          if (!ActionAllowed())
            goto stop;

          switch (type) {
             case DVB_TERR:
               f = chan_to_freq(channel, this_channellist);
               if (!f) {
                 continue;               //skip unused channels
                 }
               if (freq_offset(channel, this_channellist, offs) == -1) {
                 continue;        //skip this one
                 }
               f += freq_offset(channel, this_channellist, offs);
               if (this_bandwidth != bandwidth(channel, this_channellist)) {
                 dlog(1, "Scanning %sMHz frequencies...",
                      vdr_bandwidth_name(bandwidth(channel, this_channellist)));
                 }
               this_bandwidth = bandwidth(channel, this_channellist);
               SetTerrTransponderDataFromDVB(aChannel, cSource::stTerr, f / 1000,
                                             this_bandwidth,
                                             caps_qam,
                                             caps_hierarchy,
                                             caps_fec,
                                             caps_fec,
                                             caps_guard_interval,
                                             caps_transmission_mode,
                                             caps_inversion);
               dlog(1, "%s", *PrintTransponder(aChannel));
               aChannel->SetId(0, 0, 0, 0);
               if (is_known_initial_transponder(aChannel, false)) {
                 dlog(1, "%d: skipped (already known transponder)", aChannel->Frequency());
                 thisChannel++;
                 continue;
                 }
               break;
             case DVB_CABLE:
               f = chan_to_freq(channel, this_channellist);
               if (!f) {
                 continue;        //skip unused channels
                 }
               if (freq_offset(channel, this_channellist, offs) == -1) {
                 continue;        //skip this one
                 }
               f += freq_offset(channel, this_channellist, offs);
               this_qam = caps_qam;
               if (qam_no_auto > 0) {
                 this_qam = dvbc_modulation(mod_parm);
                 #if VDRVERSNUM >= 10713
                 params = new cDvbTransponderParameters(aChannel->Parameters());
                 if (params->Modulation() != this_qam) {
                   dlog(1, "searching QAM%s...", vdr_modulation_name(this_qam));
                   }
                 free(params);
                 #else
                 if (aChannel->Modulation() != this_qam) {
                   dlog(1, "searching QAM%s...", vdr_modulation_name(this_qam));
                   }
                 #endif

                 }
               SetCableTransponderDataFromDVB(aChannel, cSource::stCable, f / 1000,
                                              this_qam,
                                              dvbc_symbolrate(sr_parm) / 1000,
                                              caps_fec,
                                              caps_inversion);
               dlog(1, "%s", *PrintTransponder(aChannel));
               aChannel->SetId(0, 0, 0, 0);
               if (is_known_initial_transponder(aChannel, false)) {
                 dlog(1, "%d: skipped (already known transponder)", aChannel->Frequency());
                 thisChannel++;
                 continue;
                 }
               break;
             case DVB_SAT:
               SetSatTransponderDataFromDVB(aChannel,
                                            cSource::FromString(sat_list[this_channellist].source_id),
                                            sat_list[this_channellist].items[channel].intermediate_frequency,
                                            GetVDRPolarizationFromDVB(sat_list[this_channellist].items[channel].polarization),
                                            sat_list[this_channellist].items[channel].symbol_rate,
                                            sat_list[this_channellist].items[channel].fec_inner,
                                            sat_list[this_channellist].items[channel].modulation_type,
                                            sat_list[this_channellist].items[channel].modulation_system,
                                            sat_list[this_channellist].items[channel].rolloff);
               if (! ValidSatfreq(sat_list[this_channellist].items[channel].intermediate_frequency, aChannel))
                  continue;
 
               dlog(1, "%s", *PrintChannel(aChannel));
               aChannel->SetId(0, 0, 0, 0);
               /*
                * orbital_position = sat_list[this_channellist].orbital_position;
                * west_east_flag   = sat_list[this_channellist].west_east_flag;
                */
               if (sat_list[this_channellist].items[channel].modulation_system == SYS_DVBS2) {
                 if (!(caps_s2) || (DVB_API_VERSION < 5) || (wSetup.enable_s2 == 0)) {
                   dlog(1, "%d: skipped ()",
                        sat_list[this_channellist].items[channel].intermediate_frequency,
                        (wSetup.enable_s2 == 0)?"disabled":"no driver support");
                   thisChannel++;
                   continue;
                   }
                 }

               if (is_known_initial_transponder(aChannel, false)) {
                 dlog(1, "%d: skipped (already known transponder)", aChannel->Frequency());
                 thisChannel++;
                 continue;
                 }
               break;
             case DVB_ATSC:
               switch (mod_parm) {
                  case ATSC_VSB:
                    this_atsc = VSB_8;
                    f         = chan_to_freq(channel, ATSC_VSB);
                    if (!f) {
                      continue;          //skip unused channels
                      }
                    if (freq_offset(channel, ATSC_VSB, offs) == -1) {
                      continue;          //skip this one
                      }
                    f += freq_offset(channel, ATSC_VSB, offs);
                    break;
                  case ATSC_QAM:
                    this_atsc = QAM_256;
                    f         = chan_to_freq(channel, ATSC_QAM);
                    if (!f) {
                      continue;          //skip unused channels
                      }
                    if (freq_offset(channel, ATSC_QAM, offs) == -1) {
                      continue;          //skip this one
                      }
                    f += freq_offset(channel, ATSC_QAM, offs);
                    break;
                  default:
                    dlog(1, "unknown atsc modulation id %d", mod_parm);
                    return;
                  } // end switch mod_parm
               //fixme: vsb vs qam here
               SetCableTransponderDataFromDVB(aChannel, cSource::stCable, f / 1000,
                                              this_atsc,
                                              dvbc_symbolrate(sr_parm),
                                              caps_fec,
                                              caps_inversion);
               dlog(1, "%s", *PrintTransponder(aChannel));
               aChannel->SetId(0, 0, 0, 0);
               if (is_known_initial_transponder(aChannel, false)) {
                 dlog(1, "%d %s: skipped (already known transponder)",
                      aChannel->Frequency(), atsc_mod_to_txt(this_atsc));
                 thisChannel++;
                 continue;
                 }
               break;

             case PVRINPUT:
             case PVRINPUT_FM:
               {
               cString ch;
               f           = freqlists[this_channellist].freqlist[channel].channelfreq;
               channelname = cString::sprintf("%s", freqlists[this_channellist].freqlist[channel].channelname);
               shortname   = cString::sprintf("%s", freqlists[this_channellist].freqlist[channel].shortname);
               #ifdef PLUGINPARAMPATCHVERSNUM
               ch = cString::sprintf("%s;wirbelscan:%d:PVRINPUT|%s:P:0:301:%s:0:%d:0:0:0",
                                     *channelname, f, IsTvChannel(f) ? "TV" : "RADIO",
                                     IsTvChannel(f) ? "300:305" : "0:0", (int) 0.5 + (f * 16) / 1000);
               #elif VDRVERSNUM > 10713  
               ch = cString::sprintf("%s;wirbelscan:%d:%s:V:0:301:%s:0:1:0:%d:0",
                                     *channelname, f, IsTvChannel(f) ? "TV" : "RADIO",
                                     IsTvChannel(f) ? "300:305" : "0:0", (int) 0.5 + (f * 16) / 1000);
               #else 
               ch = cString::sprintf("%s;wirbelscan:%d:C0:C:0:301:%s:A1:%d:0:0:0",
                                     *channelname, f,
                                     IsTvChannel(f) ? "300:305" : "0:0", (int) 0.5 + (f * 16) / 1000);
               #endif
               aChannel->Parse(*ch);
               if (IsTvChannel(f)) {
                 dlog(1, "%s", *PrintTransponder(aChannel));
                 if (is_known_initial_transponder(aChannel, false, &Channels)) {
                   dlog(1, "%d: skipped (already known channel)", aChannel->Frequency());
                   thisChannel++;
                   continue;
                   }
                 }
               break;
               }
             case TRANSPONDER:
               dlog(1, "%s", *PrintTransponder(aChannel));
               break;
             default:;
             } // end switch type
          ++thisChannel;
          lProgress = (int) (thisChannel * 100) / initialTransponders;
          lTransponder = *PrintTransponder(aChannel);
          if (MenuScanning) {
            MenuScanning->SetProgress(lProgress, type, (initialTransponders - thisChannel));
            MenuScanning->SetTransponder(aChannel);
            }
          dev->SwitchChannel(aChannel, false);
          SwReceiver = new cSwReceiver(aChannel);
          dev->AttachReceiver(SwReceiver);

          switch (type) {
             case DVB_ATSC:
             case DVB_CABLE:
             case DVB_SAT:
             case DVB_TERR:
             case TRANSPONDER:
               {
               bool lock;

               cCondWait::SleepMs(1000);
               if (GetFrontendStatus(dev->CardIndex()) & FE_HAS_SIGNAL) 
                 lock = dev->HasLock(1500);
               else
                 lock = false;

               lStrength = GetFrontendStrength(dev->CardIndex());
               if (MenuScanning) MenuScanning->SetStr(lStrength, lock);
               if (lock) {
                  StateMachine = new cStateMachine(dev, aChannel, useNit);
                  while (StateMachine && StateMachine->Active())
                     cCondWait::SleepMs(100);
                  //StateMachine = NULL;
                  DELETENULL(StateMachine);
                  }
               }
               break;
             case PVRINPUT:
               { //tv scan
               int iterations = 0;
               cCondWait::SleepMs(500);
               lStrength = PvrHasLock(1500, videodev);

               if (MenuScanning) MenuScanning->SetStr(lStrength, lStrength > 0);
               if (lStrength) {
                 cCondWait::SleepMs(1000);
                 SwReceiver->Reset(); // try to cleanup old vbi cached data from pvrinput and start thread
                 while (! SwReceiver->Finished() && ++iterations < 400)
                    cCondWait::SleepMs(100);
                 cChannel *     newChannel = new cChannel;                
                 newChannel->Parse(*aChannel->ToText());
                 newChannel->SetName(*channelname, "", "");
                 if (SwReceiver) {
                     char * name = NULL;

                     if (SwReceiver->CNI_VPS() && SwReceiver->GetCniNameVPS())
                        name = (char *) SwReceiver->GetCniNameVPS();
                     else if (SwReceiver->CNI_8_30_1() && SwReceiver->GetCniNameFormat1())
                        name = (char *) SwReceiver->GetCniNameFormat1();
                     else if (SwReceiver->CNI_8_30_2() && SwReceiver->GetCniNameFormat2())
                        name = (char *) SwReceiver->GetCniNameFormat2();
                     else if (SwReceiver->CNI_X_26() && SwReceiver->GetCniNameX26())
                        name = (char *) SwReceiver->GetCniNameX26();
                     else if (SwReceiver->CNI_CR_IDX() && SwReceiver->GetCniNameCrIdx())
                        name = (char *) SwReceiver->GetCniNameCrIdx();
                     else if (SwReceiver->Fuzzy())
                        name = (char *) SwReceiver->GetFuzzyName();

                     if (name) {
                        newChannel->SetName(name, "", "");
                        }
                     }
                 dlog(1, "%s", *PrintChannel(newChannel));
                 Channels.IncBeingEdited();
                 Channels.Add(newChannel);
                 Channels.DecBeingEdited();
                 Channels.ReNumber();
                 Channels.SetModified(true);

                 if (MenuScanning)
                    MenuScanning->SetChan(1);
                 }
               dev->Detach(SwReceiver);
               delete(SwReceiver);
               break;
               }
             case PVRINPUT_FM:
               { //pvr fm radio scan using pvrinput.
               int    pll_lock  = 100;
               int    samples   = 50;
               int    sinterval = 10;
               double sig       = 0.0;

               cCondWait::SleepMs(pll_lock);
               for (int s = 0; s < samples; s++) {
                 cCondWait::SleepMs(sinterval);
                 sig += PvrGetSignal(videodev);
                 }
               sig /= samples;
               dlog(1, "spectrum scan %.2f %%", 100 * ((double) row / channel_max));
               nextpair(f, sig);

               if (channel == channel_max) {
                 int      maxcount = 0, i;
                 double * bsplines, * maxs;

                 dlog(3, "==== BSPLINE ====");
                 bsplines = (double *) malloc(bsp_reqbufsize(row) * sizeof(double));
                 bspline(vals, row, bsplines);

                 if (SPLINE_RATIO > 0.0) {
                   double * ccoeffs  = (double *) malloc(csp_reqbufsize(row) * sizeof(double));
                   double * csplines = (double *) malloc(bsp_reqbufsize(row) * sizeof(double));
                   dlog(3, "==== CSPLINE ====");
                   dlog(4, "\tcoeffs");
                   cspline(vals, row, ccoeffs);
                   dlog(4, "\tvalues");
                   for (i = 0; i < bsp_reqbufsize(row) / 2; i++) {
                     *(csplines + i * 2 + 0) = *(bsplines + i * 2 + 0);
                     *(csplines + i * 2 + 1) = csp_value(vals, row, ccoeffs, *(bsplines + i * 2 + 0));
                     *(bsplines + i * 2 + 1) = *(bsplines + i * 2 + 1) * (1.0 - SPLINE_RATIO) +
                                               *(csplines + i * 2 + 1) * SPLINE_RATIO;
                     }
                   free(ccoeffs);
                   free(csplines);
                   }

                 if (wSetup.verbosity >= 3) {
                   FILE * alog;
                   alog = fopen("data.log", "w+");
                   fprintf(alog, "==== vals ====\n");
                   printmatrix(vals, 2, row, 0, alog);
                   fprintf(alog, "==== bsplines ====\n");
                   printmatrix(bsplines, 2, bsp_reqbufsize(row) / 2, 0, alog);
                   fclose(alog);
                   }

                 dlog(3, "==== MAXIMA ====");
                 maxs     = (double *) malloc(bsp_reqbufsize(row) * sizeof(double));
                 maxcount = bsp_maxima(bsp_reqbufsize(row) / 2, bsplines, maxs);
                 if (wSetup.verbosity >= 3) {
                   printmatrix(maxs, 2, maxcount, 0);
                   }

                 if (maxcount) {
                   int i;
                   Channels.IncBeingEdited();
                   for (i = 0; i < maxcount; i++) {
                     cString ch, name;
                     // 30% strength; arbitrary choosen.
                     if (*(maxs + i * 2 + 1) < 20000) {
                       continue;
                       }

                     aChannel = new cChannel;

                     f    = bsp_round(*(maxs + i * 2 + 0), 50);
                     name = cString::sprintf("FM %.2f", (double) f / 1000.0);

                     #ifdef PLUGINPARAMPATCHVERSNUM
                     ch = cString::sprintf("%s;FM radio:%d:PVRINPUT|RADIO:P:0:0:300:0:0:%d:0:0:0",
                                           *name, f, (int) 0.5 + (f * 16) / 1000);
                     #elif VDRVERSNUM > 10713
                     ch = cString::sprintf("%s;FM radio:%d:RADIO:V:0:0:300:0:0:0:0:%d:0",
                                           *name, f, (int) 0.5 + (f * 16) / 1000);
                     #else
                     ch = cString::sprintf("%s;FM radio:%d:C0:C:0:0:300:0:A1:%d:0:0:0",
                                           *name, f, (int) 0.5 + (f * 16) / 1000);
                     #endif

                     aChannel->Parse(*ch);

                     if (is_known_initial_transponder(aChannel, false, &Channels)) {
                       dlog(1, "fm radio: %s (already known)", *PrintTransponder(aChannel));
                       }
                     else {
                       Channels.Add(aChannel);
                       dlog(1, "fm radio: %s (new)", *PrintTransponder(aChannel));
                       }
                     }
                   Channels.DecBeingEdited();
                   Channels.SetModified(true);
                   }

                 free(bsplines);
                 free(maxs);
                 free(vals);
                 vals = NULL;
                 }
               }
               break;
             default:;
             } // end switch type
          if (dev)
            dev->DetachAllReceivers();
          } // end loop sr_parm
      } // end loop channel
    } // end loop mod_parm

  if (type == PVRINPUT) {
      int d;
      cDevice * aDevice;
      cString c, node;
      const char * inputs[] = { 
            "COMPOSITE0", "COMPOSITE1", "COMPOSITE2", "COMPOSITE3", "COMPOSITE4",
            "SVIDEO0",    "SVIDEO1",    "SVIDEO2",    "SVIDEO3",
            "COMPONENT" }; 

      for (d = 0; d<8; d++) {
          if (! (aDevice = cDevice::GetDevice(d)))
             continue;

          for (int card=0; card<8; card++) {
              node = cString::sprintf("/dev/video%d", card);
              if (! FileExists(*node))
                 continue;

              c = cString::sprintf("CARD%d", card);
              for (unsigned i=0; i<10; i++) {
                  cString p = cString::sprintf("%s", inputs[i]);
                  cString ch;

                  ch = cString::sprintf("%s %s;extern:1%s%s|%s:%s:0:301:300:305:0:1:0:%d:0",
                       *c, *p, (VDRVERSNUM>10713)?":":":PVRINPUT|",
                       *p ,*c, (VDRVERSNUM>10713)?"V":"P", 9000 + 10*card+i);
                  if (!aChannel->Parse(*ch)) {
                     dlog(0, "%s %d: cannot parse channel:  %s ", __FUNCTION__, __LINE__, *ch);
                     break;
                     }
                  if (is_known_initial_transponder(aChannel, true, &Channels)) {
                     dlog(1, "%s %s skipped (already known channel)", *c, *p);
                     continue;
                     }
                  if (aDevice->ProvidesChannel(aChannel)) {
                     cChannel * newChannel = new cChannel(*aChannel);
                     dlog(1, "%s", *PrintChannel(newChannel));
                     Channels.IncBeingEdited();
                     Channels.Add(newChannel);
                     Channels.DecBeingEdited();
                     Channels.SetModified(true);
                     if (MenuScanning)
                        MenuScanning->SetChan(1);
                     }
                  } // end input loop
              } // end /dev/videoX loop
          } // end device loop
      } // end type pvrinput

stop:
  if (MenuScanning) MenuScanning->SetStatus((status = 0));
  if (dev) dev->DetachAllReceivers();
  Channels.ReNumber();
  SetShouldstop(true);
  dlog(3, "leaving scanner");
  Cancel(0);
  Scanner = NULL;
  }
