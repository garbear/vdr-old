/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "channels/Channel.h"
#include "channels/ChannelDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include "gtest/gtest.h"

#include <string>

using namespace std;

TEST(Channel, Channel)
{
  {
    cChannel channel;
    string serializedChannel;
    EXPECT_TRUE(channel.SerialiseConf(serializedChannel));
    EXPECT_STREQ(serializedChannel.c_str(), ":0:::0:0:0:0:0:0:0:0:0\n");

    EXPECT_TRUE(channel.DeserialiseConf(serializedChannel));
    EXPECT_EQ(channel.Frequency(), 0);
    EXPECT_EQ(channel.Source(), 0);
    EXPECT_EQ(channel.Srate(), 0);
    EXPECT_EQ(channel.Vpid(), 0);
    EXPECT_EQ(channel.Ppid(), 0);
    EXPECT_NE(channel.Apids(), (const int*)NULL);
    EXPECT_NE(channel.Dpids(), (const int*)NULL);
    EXPECT_NE(channel.Spids(), (const int*)NULL);
    EXPECT_EQ(channel.Apid(0), 0);
    EXPECT_EQ(channel.Dpid(0), 0);
    EXPECT_EQ(channel.Spid(0), 0);
    EXPECT_NE(channel.Alang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Alang(0), "");
    EXPECT_NE(channel.Dlang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Dlang(0), "");
    EXPECT_NE(channel.Slang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Slang(0), "");
    EXPECT_EQ(channel.Atype(0), 0);
    EXPECT_EQ(channel.Dtype(0), 0);
    EXPECT_EQ(channel.SubtitlingType(0), (uchar)0);
    EXPECT_EQ(channel.CompositionPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.AncillaryPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.Tpid(), 0);
    EXPECT_NE(channel.Caids(), (const int*)NULL);
    EXPECT_EQ(channel.Ca(0), 0);
    EXPECT_EQ(channel.Nid(), 0);
    EXPECT_EQ(channel.Tid(), 0);
    EXPECT_EQ(channel.Sid(), 0);
    EXPECT_EQ(channel.Rid(), 0);
    EXPECT_EQ(channel.Number(), 0);
    //EXPECT_EQ(channel.GroupSep(), false); // Fails because first character is a :
  }

  TiXmlElement channelElement(CHANNEL_XML_ELM_CHANNEL);
  {
    cChannel channel;
    EXPECT_TRUE(channel.DeserialiseConf("KABC-DT;(null):177000:M10:A:0:49=2:0;52=eng@106,53=esl@106:0:0:1:0:0:0\n"));
    EXPECT_EQ(channel.Frequency(), 177000);
    EXPECT_EQ(channel.Source(), 1090519040);
    EXPECT_EQ(channel.Srate(), 0);
    EXPECT_EQ(channel.Vpid(), 49);
    EXPECT_EQ(channel.Ppid(), 49);
    EXPECT_NE(channel.Apids(), (const int*)NULL);
    EXPECT_NE(channel.Dpids(), (const int*)NULL);
    EXPECT_NE(channel.Spids(), (const int*)NULL);
    EXPECT_EQ(channel.Apid(0), 0);
    EXPECT_EQ(channel.Dpid(0), 52);
    EXPECT_EQ(channel.Spid(0), 0);
    EXPECT_NE(channel.Alang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Alang(0), "");
    EXPECT_EQ(channel.Atype(0), 0);
    EXPECT_NE(channel.Dlang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Dlang(0), "eng");
    EXPECT_EQ(channel.Dtype(0), 106);
    EXPECT_NE(channel.Slang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Slang(0), "");
    EXPECT_EQ(channel.SubtitlingType(0), (uchar)0);
    EXPECT_EQ(channel.CompositionPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.AncillaryPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.Tpid(), 0);
    EXPECT_NE(channel.Caids(), (const int*)NULL);
    EXPECT_EQ(channel.Ca(0), 0);
    EXPECT_EQ(channel.Nid(), 0);
    EXPECT_EQ(channel.Tid(), 0);
    EXPECT_EQ(channel.Sid(), 1);
    EXPECT_EQ(channel.Rid(), 0);
    EXPECT_EQ(channel.Number(), 0);

    string serializedChannel;
    EXPECT_TRUE(channel.SerialiseConf(serializedChannel));
    EXPECT_STREQ(serializedChannel.c_str(), "KABC-DT;(null):177000:M10:A:0:49=2:0;52=eng@106,53=esl@106:0:0:1:0:0:0\n");

    EXPECT_TRUE(channel.SerialiseChannel(&channelElement));

    const char *name = channelElement.Attribute(CHANNEL_XML_ATTR_NAME);
    EXPECT_STREQ(name, "KABC-DT");

    const char *shortName = channelElement.Attribute(CHANNEL_XML_ATTR_SHORTNAME);
    EXPECT_STREQ(shortName, "");

    const char *provider = channelElement.Attribute(CHANNEL_XML_ATTR_PROVIDER);
    EXPECT_STREQ(shortName, "");

    const char *frequency = channelElement.Attribute(CHANNEL_XML_ATTR_FREQUENCY);
    EXPECT_EQ(StringUtils::IntVal(frequency), 177000);

    const char *parameters = channelElement.Attribute(CHANNEL_XML_ATTR_PARAMETERS);
    EXPECT_STREQ(parameters, "M10");

    const char *source = channelElement.Attribute(CHANNEL_XML_ATTR_SOURCE);
    EXPECT_EQ(cSource::FromString(source), 1090519040);

    const char *srate = channelElement.Attribute(CHANNEL_XML_ATTR_SRATE);
    EXPECT_EQ(StringUtils::IntVal(srate), 0);

    const char *vpid = channelElement.Attribute(CHANNEL_XML_ATTR_VPID);
    EXPECT_EQ(StringUtils::IntVal(vpid), 49);

    const char *ppid = channelElement.Attribute(CHANNEL_XML_ATTR_PPID);
    EXPECT_EQ(StringUtils::IntVal(ppid), 49);

    const char *vtype = channelElement.Attribute(CHANNEL_XML_ATTR_VTYPE);
    EXPECT_EQ(StringUtils::IntVal(vtype), 2);

    const char *tpid = channelElement.Attribute(CHANNEL_XML_ATTR_TPID);
    EXPECT_EQ(StringUtils::IntVal(tpid), 0);

    /*
    const TiXmlNode *apidsNode = channelElement.FirstChild(CHANNEL_XML_ELM_APIDS);
    ASSERT_TRUE(apidsNode != NULL);
    const TiXmlNode *apidNode = apidsNode->FirstChild(CHANNEL_XML_ELM_APID);
    ASSERT_TRUE(apidNode != NULL);
    const TiXmlElement *apidElem = apidNode->ToElement();
    ASSERT_TRUE(apidElem != NULL);
    EXPECT_EQ(StringUtils::IntVal(apidElem->GetText()), 0);
    const char *alang = apidElem->Attribute(CHANNEL_XML_ATTR_ALANG);
    EXPECT_STREQ(alang, "eng");
    const char *atype = apidElem->Attribute(CHANNEL_XML_ATTR_ATYPE);
    EXPECT_EQ(StringUtils::IntVal(atype), 0);
    */

    const TiXmlNode *dpidsNode = channelElement.FirstChild(CHANNEL_XML_ELM_DPIDS);
    ASSERT_TRUE(dpidsNode != NULL);
    const TiXmlNode *dpidNode = dpidsNode->FirstChild(CHANNEL_XML_ELM_DPID);
    ASSERT_TRUE(dpidNode != NULL);
    const TiXmlElement *dpidElem = dpidNode->ToElement();
    ASSERT_TRUE(dpidElem != NULL);
    EXPECT_EQ(StringUtils::IntVal(dpidElem->GetText()), 52);
    const char *dlang = dpidElem->Attribute(CHANNEL_XML_ATTR_DLANG);
    EXPECT_STREQ(dlang, "eng");
    const char *dtype = dpidElem->Attribute(CHANNEL_XML_ATTR_DTYPE);
    EXPECT_EQ(StringUtils::IntVal(dtype), 106);

    /*
    const TiXmlNode *spidsNode = channelElement.FirstChild(CHANNEL_XML_ELM_SPIDS);
    ASSERT_TRUE(spidsNode != NULL);
    const TiXmlNode *spidNode = spidsNode->FirstChild(CHANNEL_XML_ELM_SPID);
    ASSERT_TRUE(spidNode != NULL);
    const TiXmlElement *spidElem = spidNode->ToElement();
    ASSERT_TRUE(spidElem != NULL);
    EXPECT_EQ(StringUtils::IntVal(spidElem->GetText()), 0);
    const char *slang = spidElem->Attribute(CHANNEL_XML_ATTR_SLANG);
    EXPECT_STREQ(slang, "eng");
    */

    /*
    const TiXmlNode *caidsNode = channelElement.FirstChild(CHANNEL_XML_ELM_CAIDS);
    ASSERT_TRUE(caidsNode != NULL);
    const TiXmlNode *caidNode = caidsNode->FirstChild(CHANNEL_XML_ELM_CAID);
    ASSERT_TRUE(caidNode != NULL);
    const TiXmlElement *caidElem = caidNode->ToElement();
    ASSERT_TRUE(caidElem != NULL);
    EXPECT_EQ(StringUtils::IntVal(caidElem->GetText()), 0);
    */

    const char *sid = channelElement.Attribute(CHANNEL_XML_ATTR_SID);
    EXPECT_EQ(StringUtils::IntVal(sid), 1);

    const char *nid = channelElement.Attribute(CHANNEL_XML_ATTR_NID);
    EXPECT_EQ(StringUtils::IntVal(nid), 0);

    const char *tid = channelElement.Attribute(CHANNEL_XML_ATTR_TID);
    EXPECT_EQ(StringUtils::IntVal(tid), 0);

    const char *rid = channelElement.Attribute(CHANNEL_XML_ATTR_RID);
    EXPECT_EQ(StringUtils::IntVal(rid), 0);
  }
  {
    cChannel channel;
    EXPECT_TRUE(channel.Deserialise(&channelElement));
    EXPECT_EQ(channel.Frequency(), 177000);
    EXPECT_EQ(channel.Source(), 1090519040);
    EXPECT_EQ(channel.Srate(), 0);
    EXPECT_EQ(channel.Vpid(), 49);
    EXPECT_EQ(channel.Ppid(), 49);
    EXPECT_NE(channel.Apids(), (const int*)NULL);
    EXPECT_NE(channel.Dpids(), (const int*)NULL);
    EXPECT_NE(channel.Spids(), (const int*)NULL);
    EXPECT_EQ(channel.Apid(0), 0);
    EXPECT_EQ(channel.Dpid(0), 52);
    EXPECT_EQ(channel.Spid(0), 0);
    EXPECT_NE(channel.Alang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Alang(0), "");
    EXPECT_EQ(channel.Atype(0), 0);
    EXPECT_NE(channel.Dlang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Dlang(0), "eng");
    EXPECT_EQ(channel.Dtype(0), 106);
    EXPECT_NE(channel.Slang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Slang(0), "");
    EXPECT_EQ(channel.SubtitlingType(0), (uchar)0);
    EXPECT_EQ(channel.CompositionPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.AncillaryPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.Tpid(), 0);
    EXPECT_NE(channel.Caids(), (const int*)NULL);
    EXPECT_EQ(channel.Ca(0), 0);
    EXPECT_EQ(channel.Nid(), 0);
    EXPECT_EQ(channel.Tid(), 0);
    EXPECT_EQ(channel.Sid(), 1);
    EXPECT_EQ(channel.Rid(), 0);
    EXPECT_EQ(channel.Number(), 0);
  }

  {
    cChannel channel;
    EXPECT_FALSE(channel.DeserialiseConf(""));
  }

  TiXmlElement channelElement2(CHANNEL_XML_ELM_CHANNEL);
  {
    cChannel channel;
    EXPECT_TRUE(channel.DeserialiseConf("RedZone;(null):617000:M10:A:0:647=27:648=eng@15:0:1863:88:0:0:0\n"));
    EXPECT_EQ(channel.Frequency(), 617000);
    EXPECT_EQ(channel.Source(), 1090519040);
    EXPECT_EQ(channel.Srate(), 0);
    EXPECT_EQ(channel.Vpid(), 647);
    EXPECT_EQ(channel.Ppid(), 647);
    EXPECT_NE(channel.Apids(), (const int*)NULL);
    EXPECT_NE(channel.Dpids(), (const int*)NULL);
    EXPECT_NE(channel.Spids(), (const int*)NULL);
    EXPECT_EQ(channel.Apid(0), 648);
    EXPECT_EQ(channel.Dpid(0), 0);
    EXPECT_EQ(channel.Spid(0), 0);
    EXPECT_NE(channel.Alang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Alang(0), "eng");
    EXPECT_EQ(channel.Atype(0), 15);
    EXPECT_NE(channel.Dlang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Dlang(0), "");
    EXPECT_EQ(channel.Dtype(0), 0);
    EXPECT_NE(channel.Slang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Slang(0), "");
    EXPECT_EQ(channel.SubtitlingType(0), (uchar)0);
    EXPECT_EQ(channel.CompositionPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.AncillaryPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.Tpid(), 0);
    EXPECT_NE(channel.Caids(), (const int*)NULL);
    EXPECT_EQ(channel.Ca(0), 0x1863);
    EXPECT_EQ(channel.Nid(), 0);
    EXPECT_EQ(channel.Tid(), 0);
    EXPECT_EQ(channel.Sid(), 88);
    EXPECT_EQ(channel.Rid(), 0);
    EXPECT_EQ(channel.Number(), 0);

    string serializedChannel;
    EXPECT_TRUE(channel.SerialiseConf(serializedChannel));
    EXPECT_STREQ(serializedChannel.c_str(), "RedZone;(null):617000:M10:A:0:647=27:648=eng@15:0:1863:88:0:0:0\n");

    EXPECT_TRUE(channel.SerialiseChannel(&channelElement2));

    const TiXmlNode *caidsNode = channelElement2.FirstChild(CHANNEL_XML_ELM_CAIDS);
    ASSERT_TRUE(caidsNode != NULL);
    const TiXmlNode *caidNode = caidsNode->FirstChild(CHANNEL_XML_ELM_CAID);
    ASSERT_TRUE(caidNode != NULL);
    const TiXmlElement *caidElem = caidNode->ToElement();
    ASSERT_TRUE(caidElem != NULL);
    EXPECT_EQ(StringUtils::IntVal(caidElem->GetText()), 0x1863);
  }
  {
    cChannel channel;
    EXPECT_TRUE(channel.Deserialise(&channelElement2));
    EXPECT_EQ(channel.Frequency(), 617000);
    EXPECT_EQ(channel.Source(), 1090519040);
    EXPECT_EQ(channel.Srate(), 0);
    EXPECT_EQ(channel.Vpid(), 647);
    EXPECT_EQ(channel.Ppid(), 647);
    EXPECT_NE(channel.Apids(), (const int*)NULL);
    EXPECT_NE(channel.Dpids(), (const int*)NULL);
    EXPECT_NE(channel.Spids(), (const int*)NULL);
    EXPECT_EQ(channel.Apid(0), 648);
    EXPECT_EQ(channel.Dpid(0), 0);
    EXPECT_EQ(channel.Spid(0), 0);
    EXPECT_NE(channel.Alang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Alang(0), "eng");
    EXPECT_EQ(channel.Atype(0), 15);
    EXPECT_NE(channel.Dlang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Dlang(0), "");
    EXPECT_EQ(channel.Dtype(0), 0);
    EXPECT_NE(channel.Slang(0), (const char*)NULL);
    EXPECT_STREQ(channel.Slang(0), "");
    EXPECT_EQ(channel.SubtitlingType(0), (uchar)0);
    EXPECT_EQ(channel.CompositionPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.AncillaryPageId(0), (uint16_t)0);
    EXPECT_EQ(channel.Tpid(), 0);
    EXPECT_NE(channel.Caids(), (const int*)NULL);
    EXPECT_EQ(channel.Ca(0), 0x1863);
    EXPECT_EQ(channel.Nid(), 0);
    EXPECT_EQ(channel.Tid(), 0);
    EXPECT_EQ(channel.Sid(), 88);
    EXPECT_EQ(channel.Rid(), 0);
    EXPECT_EQ(channel.Number(), 0);
  }

  unsigned int frequency = 1000 * 1000 * 1000; // 1GHz
  EXPECT_EQ(cChannel::Transponder(frequency, 'H'), frequency + 100 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'V'), frequency + 200 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'L'), frequency + 300 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'R'), frequency + 400 * 1000);
  EXPECT_EQ(cChannel::Transponder(frequency, 'X'), frequency);
}
