/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: section.h 2.1 2012/02/26 13:58:26 kls Exp $
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_SECTION_H
#define LIBSI_SECTION_H

#include <time.h>

#include "si.h"
#include "headers.h"
#include "multiple_string.h"

namespace SI {

class PAT : public NumberedSection {
public:
   PAT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), s(NULL) {}
   PAT() : s(NULL) {}
   class Association : public LoopElement {
   public:
      int getServiceId() const;
      int getPid() const;
      bool isNITPid() const { return getServiceId()==0; }
      virtual int getLength() { return int(sizeof(pat_prog)); }
   protected:
      virtual void Parse();
   private:
      const pat_prog *s;
   };
   int getTransportStreamId() const;
   StructureLoop<Association> associationLoop;
protected:
   virtual void Parse();
private:
   const pat *s;
};

class CAT : public NumberedSection {
public:
   CAT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   CAT() {}
   DescriptorLoop loop;
protected:
   virtual void Parse();
};

class PMT : public NumberedSection {
public:
   PMT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), s(NULL) {}
   PMT() : s(NULL) {}
   class Stream : public LoopElement {
   public:
      int getPid() const;
      int getStreamType() const;
      DescriptorLoop streamDescriptors;
      virtual int getLength() { return int(sizeof(pmt_info)+streamDescriptors.getLength()); }
   protected:
      virtual void Parse();
   private:
      const pmt_info *s;
   };
   DescriptorLoop commonDescriptors;
   StructureLoop<Stream> streamLoop;
   int getServiceId() const;
   int getPCRPid() const;
protected:
   virtual void Parse();
private:
   const pmt *s;
};

class TSDT : public NumberedSection {
public:
   TSDT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), s(NULL) {}
   TSDT() : s(NULL) {}
   DescriptorLoop transportStreamDescriptors;
protected:
   virtual void Parse();
private:
   const tsdt *s;
};

class NIT : public NumberedSection {
public:
   NIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), s(NULL) {}
   NIT() : s(NULL) {}
   class TransportStream : public LoopElement {
   public:
      int getTransportStreamId() const;
      int getOriginalNetworkId() const;
      virtual int getLength() { return int(sizeof(ni_ts)+transportStreamDescriptors.getLength()); }
      DescriptorLoop transportStreamDescriptors;
   protected:
      virtual void Parse();
   private:
      const ni_ts *s;
   };
   DescriptorLoop commonDescriptors;
   StructureLoop<TransportStream> transportStreamLoop;
   int getNetworkId() const;
protected:
   virtual void Parse();
private:
   const nit *s;
};

//BAT has the same structure as NIT but different allowed descriptors
class BAT : public NIT {
public:
   BAT(const unsigned char *data, bool doCopy=true) : NIT(data, doCopy) {}
   BAT() {}
   int getBouquetId() const { return getNetworkId(); }
};

class SDT : public NumberedSection {
public:
   SDT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), s(NULL) {}
   SDT() : s(NULL) {}
   class Service : public LoopElement {
   public:
      int getServiceId() const;
      int getEITscheduleFlag() const;
      int getEITpresentFollowingFlag() const;
      RunningStatus getRunningStatus() const;
      int getFreeCaMode() const;
      virtual int getLength() { return int(sizeof(sdt_descr)+serviceDescriptors.getLength()); }
      DescriptorLoop serviceDescriptors;
   protected:
      virtual void Parse();
   private:
      const sdt_descr *s;
   };
   int getTransportStreamId() const;
   int getOriginalNetworkId() const;
   StructureLoop<Service> serviceLoop;
protected:
   virtual void Parse();
private:
   const sdt *s;
};

class EIT : public NumberedSection {
public:
   EIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), s(NULL) {}
   EIT() : s(NULL) {}
   class Event : public LoopElement {
   public:
      int getEventId() const;
      time_t getStartTime() const; //UTC
      time_t getDuration() const;

      int getMJD() const;
      int getStartTimeHour() const; //UTC
      int getStartTimeMinute() const; //UTC
      int getStartTimeSecond() const; //UTC
      int getDurationHour() const;
      int getDurationMinute() const;
      int getDurationSecond() const;
      RunningStatus getRunningStatus() const;
      int getFreeCaMode() const;

      DescriptorLoop eventDescriptors;
      virtual int getLength() { return int(sizeof(eit_event)+eventDescriptors.getLength()); }
   protected:
      virtual void Parse();
   private:
      const eit_event *s;
   };
   int getServiceId() const;
   int getTransportStreamId() const;
   int getOriginalNetworkId() const;
   int getSegmentLastSectionNumber() const;
   int getLastTableId() const;
   StructureLoop<Event> eventLoop;

   //true if table conveys present/following information, false if it conveys schedule information
   bool isPresentFollowing() const;
   //true if table describes TS on which it is broadcast, false if it describes other TS
   bool isActualTS() const;
protected:
   virtual void Parse();
private:
   const eit *s;
};

class TDT : public Section {
public:
   TDT(const unsigned char *data, bool doCopy=true) : Section(data, doCopy), s(NULL) {}
   TDT() : s(NULL) {}
   time_t getTime() const; //UTC
protected:
   virtual void Parse();
private:
   const tdt *s;
};

class TOT : public CRCSection {
public:
   TOT(const unsigned char *data, bool doCopy=true) : CRCSection(data, doCopy), s(NULL) {}
   TOT() : s(NULL) {}
   time_t getTime() const;
   DescriptorLoop descriptorLoop;
protected:
   virtual void Parse();
private:
   const tot *s;
};

class RST : public Section {
public:
   RST(const unsigned char *data, bool doCopy=true) : Section(data, doCopy) {}
   RST() {}
   class RunningInfo : public LoopElement {
   public:
      int getTransportStreamId() const;
      int getOriginalNetworkId() const;
      int getServiceId() const;
      int getEventId() const;
      RunningStatus getRunningStatus() const;
      virtual int getLength() { return int(sizeof(rst_info)); }
   protected:
      virtual void Parse();
   private:
      const rst_info *s;
   };
   StructureLoop<RunningInfo> infoLoop;
protected:
   virtual void Parse();
};

class AIT : public NumberedSection {
public:
   AIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), first(NULL) {}
   AIT() : first(NULL) {}
   class Application : public LoopElement {
   public:
      virtual int getLength() { return int(sizeof(ait_app)+applicationDescriptors.getLength()); }
      long getOrganisationId() const;
      int getApplicationId() const;
      int getControlCode() const;
      MHP_DescriptorLoop applicationDescriptors;
   protected:
      virtual void Parse();
      const ait_app *s;
   };
   MHP_DescriptorLoop commonDescriptors;
   StructureLoop<Application> applicationLoop;
   int getApplicationType() const;
   int getAITVersion() const;
protected:
   const ait *first;
   virtual void Parse();
};

/* Premiere Content Information Table */

class PremiereCIT : public NumberedSection {
public:
   PremiereCIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy), s(NULL) {}
   PremiereCIT() : s(NULL) {}
   int getContentId() const;
   time_t getDuration() const;
   PCIT_DescriptorLoop eventDescriptors;
protected:
   virtual void Parse();
private:
   const pcit *s;
};

/* Master Guide Table */

class PSIP_MGT : public VersionedSection {
public:
   PSIP_MGT(const unsigned char *data, bool doCopy=true) : VersionedSection(data, doCopy), s(NULL) {}
   PSIP_MGT() : s(NULL) {}
   class TableInfo : public VariableLengthPart {
   public:
      void setData(CharArray d);
      void setDataAndOffset(CharArray d, int &offset) { setData(d); offset += getLength(); }
      int getTableType() const;
      int getPid() const; // TODO: Rename to getTablePid()
      int getVersion() const; // TODO: Rename to getTableVersion()
      virtual int getLength() { return int(sizeof(mgt_table_info)+tableDescriptors.getLength()); }
      static int getTableInfoLength(const unsigned char *data);
      PSIP_DescriptorLoop tableDescriptors;
   protected:
      virtual void Parse();
   private:
      const mgt_table_info *s;
   };
   StructureLoop<TableInfo> tableInfoLoop;
   PSIP_DescriptorLoop descriptorLoop;
protected:
   virtual void Parse();
private:
   // Calculate size of table info loop
   static int getTableInfoLoopLength(const CharArray &data, int tableCount);
   const mgt *s;
};

/* Virtual Channel Table */

#define SHORT_NAME_LENGTH  7

class PSIP_VCT : public VersionedSection {
public:
   PSIP_VCT(const unsigned char *data, bool doCopy=true) : VersionedSection(data, doCopy), s(NULL) {}
   PSIP_VCT() : s(NULL) {}
   class ChannelInfo : public VariableLengthPart {
   public:
      void setData(CharArray d);
      void setDataAndOffset(CharArray d, int &offset) { setData(d); offset += getLength(); }
      void getShortName(char buffer[22]) const; // UTF-8, 22 == SHORT_NAME_LENGTH * sizeof(codepoint U+FFFF) + 1
      int getMajorNumber() const;
      int getMinorNumber() const;
      int getModulationMode() const;
      int getTSID() const;
      int getServiceId() const;
      int getETMLocation() const;
      bool isHidden() const;
      bool isGuideHidden() const; // Should this channel's EPG events be hidden
      int getSourceID() const;
      virtual int getLength() { return int(sizeof(vct_channel_info)+channelDescriptors.getLength()); }
      int getDescriptorLength() { return channelDescriptors.getLength(); }
      static int getChannelInfoLength(const unsigned char *data);
      PSIP_DescriptorLoop channelDescriptors;
   protected:
      virtual void Parse();
   private:
      const vct_channel_info *s;
   };
   StructureLoop<ChannelInfo> channelInfoLoop;
   PSIP_DescriptorLoop descriptorLoop;
protected:
   virtual void Parse();
private:
   // Calculate size of table info loop
   static int getChannelInfoLoopLength(const CharArray &data, int channelCount);
   const vct *s;
};

/* Event Information Table (PSIP) */

class PSIP_EIT : public VersionedSection {
public:
   PSIP_EIT(const unsigned char *data, bool doCopy=true) : VersionedSection(data, doCopy), s(NULL) {}
   PSIP_EIT() : s(NULL) {}
   class Event : public VariableLengthPart {
   public:
      void setData(CharArray d);
      void setDataAndOffset(CharArray d, int &offset) { setData(d); offset += getLength(); }
      int getEventId() const;
      time_t getStartTime() const; // GPS time
      time_t getLengthInSeconds() const;
      static int getEventLength(const psip_eit_event *data);
      PSIPStringLoop textLoop;
      PSIP_DescriptorLoop eventDescriptors;
   protected:
      virtual void Parse();
   private:
      const psip_eit_event *s;
   };
   StructureLoop<Event> eventLoop;
   int getSourceId() const;
protected:
   virtual void Parse();
private:
   const psip_eit *s;
};

/* System Time Table */

class PSIP_STT : public VersionedSection {
public:
   PSIP_STT(const unsigned char *data, bool doCopy=true) : VersionedSection(data, doCopy), s(NULL) {}
   PSIP_STT() : s(NULL) {}
   int getGpsUtcOffset() const;
protected:
   virtual void Parse();
private:
   const stt *s;
};

} //end of namespace

#endif //LIBSI_TABLE_H
