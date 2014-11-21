/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
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
#pragma once

#include <memory>
#include <vector>

namespace VDR
{

// Imported from si_ext.h at 20bc1686fcaf016e590950acdf0369d7ef5a343c
// TODO: This should be moved to vdr/streams or similar
enum STREAM_TYPE
{
  STREAM_TYPE_UNDEFINED                         = 0x00, // ITU-T | ISO/IEC Reserved
  STREAM_TYPE_11172_VIDEO                       = 0x01, // ISO/IEC 11172 Video
  STREAM_TYPE_13818_VIDEO                       = 0x02, // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
  STREAM_TYPE_11172_AUDIO                       = 0x03, // ISO/IEC 11172 Audio
  STREAM_TYPE_13818_AUDIO                       = 0x04, // ISO/IEC 13818-3 Audio
  STREAM_TYPE_13818_PRIVATE                     = 0x05, // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections
  STREAM_TYPE_13818_PES_PRIVATE                 = 0x06, // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data
  STREAM_TYPE_13522_MHEG                        = 0x07, // ISO/IEC 13522 MHEG
  STREAM_TYPE_H222_0_DSMCC                      = 0x08, // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A DSM CC
  STREAM_TYPE_H222_1                            = 0x09, // ITU-T Rec. H.222.1
  STREAM_TYPE_13818_6_TYPE_A                    = 0x0A, // ISO/IEC 13818-6 type A
  STREAM_TYPE_13818_6_TYPE_B                    = 0x0B, // ISO/IEC 13818-6 type B
  STREAM_TYPE_13818_6_TYPE_C                    = 0x0C, // ISO/IEC 13818-6 type C
  STREAM_TYPE_13818_6_TYPE_D                    = 0x0D, // ISO/IEC 13818-6 type D
  STREAM_TYPE_13818_1_AUX                       = 0x0E, // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 auxiliary
  STREAM_TYPE_13818_AUDIO_ADTS                  = 0x0F, // ISO/IEC 13818-7 Audio with ADTS transport syntax
  STREAM_TYPE_14496_VISUAL                      = 0x10, // ISO/IEC 14496-2 Visual
  STREAM_TYPE_14496_AUDIO_LATM                  = 0x11, // ISO/IEC 14496-3 Audio with the LATM transport syntax as defined in ISO/IEC 14496-3 / AMD 1
  STREAM_TYPE_14496_FLEX_PES                    = 0x12, // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets
  STREAM_TYPE_14496_FLEX_SECT                   = 0x13, // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC14496_sections.
  STREAM_TYPE_13818_6_DOWNLOAD                  = 0x14, // ISO/IEC 13818-6 Synchronized Download Protocol
  STREAM_TYPE_META_PES                          = 0x15, // Metadata carried in PES packets using the Metadata Access Unit Wrapper defined in 2.12.4.1
  STREAM_TYPE_META_SECT                         = 0x16, // Metadata carried in metadata_sections
  STREAM_TYPE_DSMCC_DATA                        = 0x17, // Metadata carried in ISO/IEC 13818-6 (DSM-CC) Data Carousel
  STREAM_TYPE_DSMCC_OBJ                         = 0x18, // Metadata carried in ISO/IEC 13818-6 (DSM-CC) Object Carousel
  STREAM_TYPE_META_DOWNLOAD                     = 0x19, // Metadata carried in ISO/IEC 13818-6 Synchronized Download Protocol using the Metadata Access Unit Wrapper defined in 2.12.4.1
  STREAM_TYPE_14496_H264_VIDEO                  = 0x1B, // 0x1A-0x7F -> ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Reserved
  STREAM_TYPE_13818_USR_PRIVATE_80              = 0x80, // 0x80-0xFF User Private
  STREAM_TYPE_13818_USR_PRIVATE_81              = 0x81,
  STREAM_TYPE_13818_USR_PRIVATE_82              = 0x82,
};

class cChannel;
typedef std::shared_ptr<cChannel> ChannelPtr;
typedef std::vector<ChannelPtr>   ChannelVector;

}
