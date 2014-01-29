/*
 * config.h: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.h 2.76.1.4 2013/09/07 10:25:10 kls Exp $
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include "utils/StringUtils.h"
#include "utils/I18N.h"
#include "filesystem/File.h"
//#include "font.h"
#include "utils/Tools.h"

// VDR's own version number:

#define VDRVERSION  "2.0.4"
#define VDRVERSNUM   20004  // Version * 10000 + Major * 100 + Minor

// The plugin API's version number:

#define APIVERSION  "2.0.0"
#define APIVERSNUM   20000  // Version * 10000 + Major * 100 + Minor

// When loading plugins, VDR searches them by their APIVERSION, which
// may be smaller than VDRVERSION in case there have been no changes to
// VDR header files since the last APIVERSION. This allows compiled
// plugins to work with newer versions of the core VDR as long as no
// VDR header files have changed.

#define MAXPRIORITY       99
#define MINPRIORITY       (-MAXPRIORITY)
#define LIVEPRIORITY      0                  // priority used when selecting a device for live viewing
#define TRANSFERPRIORITY  (LIVEPRIORITY - 1) // priority used for actual local Transfer Mode
#define IDLEPRIORITY      (MINPRIORITY - 1)  // priority of an idle device
#define MAXLIFETIME       99
#define DEFINSTRECTIME    180 // default instant recording time (minutes)

#define TIMERMACRO_TITLE    "TITLE"
#define TIMERMACRO_EPISODE  "EPISODE"

#define MINOSDWIDTH   480
#define MAXOSDWIDTH  1920
#define MINOSDHEIGHT  324
#define MAXOSDHEIGHT 1200

#define MaxFileName NAME_MAX // obsolete - use NAME_MAX directly instead!
#define MaxSkinName 16
#define MaxThemeName 16

// Error flags
#define ERROR_PES_GENERAL   0x01
#define ERROR_PES_SCRAMBLE  0x02
#define ERROR_PES_STARTCODE 0x04
#define ERROR_DEMUX_NODATA  0x10

// TODO: Define these in a better place
#ifndef INSTALL_PATH
#define INSTALL_PATH    "/usr/share/vdr"
#endif

#ifndef BIN_INSTALL_PATH
#define BIN_INSTALL_PATH "/usr/lib/vdr"
#endif

#define VDR_HOME_ENV_VARIABLE  "VDR_HOME"

// Basically VDR works according to the DVB standard, but there are countries/providers
// that use other standards, which in some details deviate from the DVB standard.
// This makes it necessary to handle things differently in some areas, depending on
// which standard is actually used. The following macros are used to distinguish
// these cases (make sure to adjust cMenuSetupDVB::standardComplianceTexts accordingly
// when adding a new standard):

#define STANDARD_DVB       0
#define STANDARD_ANSISCTE  1

typedef uint32_t in_addr_t; //XXX from /usr/include/netinet/in.h (apparently this is not defined on systems with glibc < 2.2)

class cSVDRPhost : public cListObject {
private:
  struct in_addr addr;
  in_addr_t mask;
public:
  cSVDRPhost(void);
  bool Parse(const char *s);
  bool IsLocalhost(void);
  bool Accepts(in_addr_t Address);
  };

class cSatCableNumbers {
private:
  int size;
  int *array;
public:
  cSatCableNumbers(int Size, const std::string& bindings);
  ~cSatCableNumbers();
  int Size(void) const { return size; }
  int *Array(void) { return array; }
  bool FromString(const char *s);
  cString ToString(void);
  int FirstDeviceIndex(int DeviceIndex) const;
      ///< Returns the first device index (starting at 0) that uses the same
      ///< sat cable number as the device with the given DeviceIndex.
      ///< If the given device does not use the same sat cable as any other device,
      ///< or if the resulting value would be the same as DeviceIndex,
      ///< or if DeviceIndex is out of range, -1 is returned.
  };

template<class T> class cConfig : public cList<T> {
private:
  char *fileName;
  bool allowComments;
  void Clear(void)
  {
    free(fileName);
    fileName = NULL;
    cList<T>::Clear();
  }
public:
  cConfig(void)
  {
    fileName = NULL;
    allowComments = true;
  }

  virtual ~cConfig()
  {
    free(fileName);
  }

  const char *FileName(void)
  {
    return fileName;
  }

  bool Load(const char *FileName = NULL, bool AllowComments = false, bool MustExist = false)
  {
    bool result = !MustExist;
    cConfig<T>::Clear();

    if (FileName)
    {
      free(fileName);
      fileName = strdup(FileName);
      allowComments = AllowComments;
    }

    isyslog("loading %s", fileName);
    CFile file;
    if (file.Open(fileName))
    {
      std::string strLine;
      dsyslog("opened %s", fileName);
      result = true;
      unsigned int line = 0; // For error logging
      while (file.ReadLine(strLine))
      {
        // Strip comments and spaces
        if (allowComments)
        {
          size_t comment_pos = strLine.find('#');
          strLine = strLine.substr(0, comment_pos);
        }

        StringUtils::Trim(strLine);

        if (strLine.empty())
          continue;

        T *l = new T;
        if (l->Parse(strLine.c_str()))
        {
          this->Add(l);
        }
        else
        {
          esyslog("ERROR: error in %s, line %d", fileName, line);
          delete l;
          result = false;
        }

        line++;
      }
    }
    else
    {
      LOG_ERROR_STR(fileName);
      result = false;
    }

    if (!result)
       fprintf(stderr, "vdr: error while reading '%s'\n", fileName);
    return result;
  }

  bool Save(void)
  {
    bool result = true;
    T *l = (T *)this->First();
    cSafeFile f(fileName);
    if (f.Open()) {
       while (l) {
             if (!l->Save(f)) {
                result = false;
                break;
                }
             l = (T *)l->Next();
             }
       if (!f.Close())
          result = false;
       }
    else
       result = false;
    return result;
  }
  };

class cNestedItem : public cListObject {
private:
  char *text;
  cList<cNestedItem> *subItems;
public:
  cNestedItem(const char *Text, bool WithSubItems = false);
  virtual ~cNestedItem();
  virtual int Compare(const cListObject &ListObject) const;
  const char *Text(void) const { return text; }
  cList<cNestedItem> *SubItems(void) { return subItems; }
  void AddSubItem(cNestedItem *Item);
  void SetText(const char *Text);
  void SetSubItems(bool On);
  };

class cNestedItemList : public cList<cNestedItem> {
private:
  char *fileName;
  bool Parse(FILE *f, cList<cNestedItem> *List, int &Line);
  bool Write(FILE *f, cList<cNestedItem> *List, int Indent = 0);
public:
  cNestedItemList(void);
  virtual ~cNestedItemList();
  void Clear(void);
  bool Load(const char *FileName);
  bool Save(void);
  };

class cSVDRPhosts : public cConfig<cSVDRPhost> {
public:
  bool LocalhostOnly(void);
  bool Acceptable(in_addr_t Address);
  };

extern cNestedItemList Folders;
extern cNestedItemList Commands;
extern cNestedItemList RecordingCommands;
extern cSVDRPhosts SVDRPhosts;

class cSetupLine : public cListObject {
private:
  char *plugin;
  char *name;
  char *value;
public:
  cSetupLine(void);
  cSetupLine(const char *Name, const char *Value, const char *Plugin = NULL);
  virtual ~cSetupLine();
  virtual int Compare(const cListObject &ListObject) const;
  const char *Plugin(void) { return plugin; }
  const char *Name(void) { return name; }
  const char *Value(void) { return value; }
  bool Parse(const char *s);
  bool Save(FILE *f);
  };

class cSetup : public cConfig<cSetupLine> {
  friend class cPlugin; // needs to be able to call Store()
private:
  void StoreLanguages(const char *Name, int *Values);
  bool ParseLanguages(const char *Value, int *Values);
  bool Parse(const char *Name, const char *Value);
  cSetupLine *Get(const char *Name, const char *Plugin = NULL);
  void Store(const char *Name, const char *Value, const char *Plugin = NULL, bool AllowMultiple = false);
  void Store(const char *Name, int Value, const char *Plugin = NULL);
  void Store(const char *Name, double &Value, const char *Plugin = NULL);
public:
  // Also adjust cMenuSetup (menu.c) when adding parameters here!
  int __BeginData__;
  int MarkInstantRecord;
  char NameInstantRecord[NAME_MAX + 1];
  int InstantRecordTime;
  int LnbSLOF;
  int LnbFrequLo;
  int LnbFrequHi;
  int DiSEqC;
  int SetSystemTime;
  int TimeSource;
  int TimeTransponder;
  int StandardCompliance;
  int MarginStart, MarginStop;
  int EPGLanguages[I18N_MAX_LANGUAGES + 1];
  int EPGScanTimeout;
  int EPGBugfixLevel;
  int EPGLinger;
  int DefaultPriority, DefaultLifetime;
  int PausePriority, PauseLifetime;
  int UseSubtitle;
  int UseVps;
  int VpsMargin;
  int AlwaysSortFoldersFirst;
  int VideoDisplayFormat;//XXX remove?
  int VideoFormat;//XXX remove?
  int UpdateChannels;
  int UseDolbyDigital;//XXX remove?
  int MaxVideoFileSize;
  int SplitEditedFiles;
  int MinEventTimeout, MinUserInactivity;
  time_t NextWakeupTime;
  int ResumeID;//XXX remove?
  int EmergencyExit;
  int __EndData__;
  std::string DeviceBondings;
  std::string EPGDirectory;

  static cSetup& Get(void);
  cSetup& operator= (const cSetup &s);
  bool Load(const char *FileName);
  bool Save(void);

private:
  cSetup(void);
};

#endif //__CONFIG_H
