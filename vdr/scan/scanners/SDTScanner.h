/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
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

#include "dvb/Filter.h"

class iSdtScannerCallback
{
public:
  //virtual ChannelPtr SdtGetByService(int source, int tid, int sid) = 0;
  //virtual void SdtFoundChannel(ChannelPtr channel) = 0;
  //virtual void SdtFoundTransponder(ChannelPtr transponder) = 0;
  /*!
   * If an existing channel has the service, the NID is updated and the channel
   * is returned. Otherwise, an empty channel ptr is returned.
   */
  virtual ChannelPtr SdtFoundService(ChannelPtr channel, int nid, int tid, int sid) = 0;
  virtual void SdtFoundChannel(ChannelPtr channel, int nid, int tid, int sid, char* name, char* shortName, char* provider) = 0;
  virtual ~iSdtScannerCallback() { }
};

class cSdtScanner : public cFilter
{
public:
  cSdtScanner(iSdtScannerCallback* callback, bool bUseOtherTable = false);
  virtual ~cSdtScanner();

protected:
  virtual void ProcessData(u_short Pid, u_char Tid, const u_char * Data, int Length);

private:
  iSdtScannerCallback* m_callback;
  int                  m_tableId;
  cSectionSyncer       m_sectionSyncer;
};
