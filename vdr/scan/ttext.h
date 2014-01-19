/*
 * ttext.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __WIRBELSCAN_TTEXT_H_
#define __WIRBELSCAN_TTEXT_H_

#include <vdr/receiver.h>
#include <vdr/channels.h>
#include <vdr/ringbuffer.h>

class cSwReceiver : public cReceiver, public cThread {
private:
  cChannel * channel;
  cRingBufferLinear * buffer;
  bool stopped;
  bool fuzzy;
  int hits;
  int timeout;
  uint32_t TsCount;
  uint16_t cni_8_30_1;
  uint16_t cni_8_30_2;
  uint16_t cni_X_26;
  uint16_t cni_vps;
  uint16_t cni_cr_idx;
  char fuzzy_network[48];
protected:
  virtual void Receive(uchar * Data, int Length);
  virtual void Action();
  virtual void Stop() { stopped = true; };
  void Decode(uchar * data, int count);
  void DecodePacket(uchar * data);
public:
  cSwReceiver(cChannel * Channel);
  virtual ~cSwReceiver();
  void Reset();
  bool Finished() { return stopped; }
  uint16_t CNI_8_30_1() { return cni_8_30_1; };
  uint16_t CNI_8_30_2() { return cni_8_30_2; };
  uint16_t CNI_X_26()   { return cni_X_26;   };
  uint16_t CNI_VPS()    { return cni_vps;    };
  uint16_t CNI_CR_IDX() { return cni_cr_idx; };
  bool     Fuzzy()      { return fuzzy;      };
  void UpdatefromName(const char * name);
  const char * GetCniNameFormat1();
  const char * GetCniNameFormat2();
  const char * GetCniNameX26();
  const char * GetCniNameVPS();
  const char * GetCniNameCrIdx();
  const char * GetFuzzyName() { return (const char *) fuzzy_network; };
};


#endif
