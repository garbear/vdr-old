/*
 * config.h: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.h 1.15 2000/09/03 09:37:30 kls Exp $
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "dvbapi.h"
#include "tools.h"

#define VDRVERSION "0.62"

#define MaxBuffer 10000

enum eKeys { // "Up" and "Down" must be the first two keys!
             kUp,
             kDown,
             kMenu,
             kOk,
             kBack,
             kLeft,
             kRight,
             kRed,
             kGreen,
             kYellow,
             kBlue,
             k0, k1, k2, k3, k4, k5, k6, k7, k8, k9,
             kNone
           };

struct tKey {
  eKeys type;
  char *name;
  unsigned int code;
  };

class cKeys {
private:
  char *fileName;
public:
  unsigned char code;
  unsigned short address;
  tKey *keys;
  cKeys(void);
  void Clear(void);
  void SetDummyValues(void);
  bool Load(const char *FileName = NULL);
  bool Save(void);
  unsigned int Encode(const char *Command);
  eKeys Get(unsigned int Code);
  void Set(eKeys Key, unsigned int Code);
  };

class cChannel : public cListObject {
private:
  static char *buffer;
  static const char *ToText(cChannel *Channel);
public:
  enum { MaxChannelName = 32 }; // 31 chars + terminating 0!
  char name[MaxChannelName];
  int frequency; // MHz
  char polarization;
  int diseqc;
  int srate;
  int vpid;
  int apid;
  int ca;
  int pnr;
  cChannel(void);
  cChannel(const cChannel *Channel);
  const char *ToText(void);
  bool Parse(const char *s);
  bool Save(FILE *f);
  bool Switch(cDvbApi *DvbApi = NULL);
  static bool SwitchTo(int i, cDvbApi *DvbApi = NULL);
  static const char *GetChannelName(int i);
  };

class cTimer : public cListObject {
private:
  time_t startTime, stopTime;
  static char *buffer;
  static const char *ToText(cTimer *Timer);
public:
  enum { MaxFileName = 256 };
  bool recording;
  int active;
  int channel;
  int day;
  int start;
  int stop;
//TODO VPS???
  int priority;
  int lifetime;
  char file[MaxFileName];
  char *summary;
  cTimer(bool Instant = false);
  ~cTimer();
  cTimer& operator= (const cTimer &Timer);
  const char *ToText(void);
  bool Parse(const char *s);
  bool Save(FILE *f);
  bool IsSingleEvent(void);
  bool Matches(time_t t = 0);
  time_t StartTime(void);
  time_t StopTime(void);
  void SetRecording(bool Recording);
  static cTimer *GetMatch(void);
  static int TimeToInt(int t);
  static time_t Day(time_t t);
  static int ParseDay(const char *s);
  static const char *PrintDay(int d);
  };

template<class T> class cConfig : public cList<T> {
private:
  char *fileName;
  void Clear(void)
  {
    delete fileName;
    cList<T>::Clear();
  }
public:
  bool Load(const char *FileName)
  {
    isyslog(LOG_INFO, "loading %s", FileName);
    bool result = true;
    Clear();
    fileName = strdup(FileName);
    FILE *f = fopen(fileName, "r");
    if (f) {
       int line = 0;
       char buffer[MaxBuffer];
       while (fgets(buffer, sizeof(buffer), f) > 0) {
             line++;
             T *l = new T;
             if (l->Parse(buffer))
                Add(l);
             else {
                esyslog(LOG_ERR, "error in %s, line %d\n", fileName, line);
                delete l;
                result = false;
                break;
                }
             }
       fclose(f);
       }
    else {
       esyslog(LOG_ERR, "can't open '%s'\n", fileName);
       result = false;
       }
    return result;
  }
  bool Save(void)
  {
  //TODO make backup copies???
    bool result = true;
    T *l = (T *)First();
    FILE *f = fopen(fileName, "w");
    if (f) {
       while (l) {
             if (!l->Save(f)) {
                result = false;
                break;
                }
             l = (T *)l->Next();
             }
       fclose(f);
       }
    else
       result = false;
    return result;
  }
  };

class cChannels : public cConfig<cChannel> {};

class cTimers : public cConfig<cTimer> {
public:
  cTimer *GetTimer(cTimer *Timer);
  };

extern int CurrentChannel;

extern cChannels Channels;
extern cTimers Timers;
extern cKeys Keys;

#endif //__CONFIG_H