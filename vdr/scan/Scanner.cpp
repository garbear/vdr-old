/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2006, 2007, 2008, 2009 Winfried Koehler
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

#include "Scanner.h"
#include "channels/ChannelManager.h"
#include "devices/Device.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "dvb/filters/PAT.h"
#include "dvb/filters/PSIP_VCT.h"
#include "transponders/TransponderFactory.h"

#include <assert.h>
#include <vector>

using namespace PLATFORM;
using namespace std;

namespace VDR
{

cScanner::cScanner(void)
 : m_frequencyHz(0),
   m_percentage(0.0f)
{
}

bool cScanner::Start(const cScanConfig& setup)
{
  assert(setup.device.get() != NULL);

  if (!IsRunning())
  {
    m_setup = setup;
    CreateThread();
    return true;
  }

  return false;
}

void* cScanner::Process()
{
  cTransponderFactory* transponders = NULL;

  fe_caps_t caps; // TODO

  switch (m_setup.dvbType)
  {
  case TRANSPONDER_ATSC:
    transponders = new cAtscTransponderFactory(caps, m_setup.atscModulation);
    break;
  case TRANSPONDER_CABLE:
    transponders = new cCableTransponderFactory(caps, m_setup.dvbcSymbolRate);
    break;
  case TRANSPONDER_SATELLITE:
    transponders = new cSatelliteTransponderFactory(caps, m_setup.satelliteIndex);
    break;
  case TRANSPONDER_TERRESTRIAL:
    transponders = new cTerrestrialTransponderFactory(caps);
    break;
  }

  if (!transponders)
    return NULL;

  //vector<cTransponder> transponders = m_setup.device->GetTransponders(m_setup);

  while (transponders->HasNext())
  {
    cTransponder transponder = transponders->GetNext();

    m_frequencyHz = transponder.FrequencyHz();

    if (m_setup.device->Channel()->SwitchTransponder(transponder))
    {
      cPat pat(m_setup.device.get());
      ChannelVector patChannels = pat.GetChannels();

      // TODO: Use SDT for non-ATSC tuners
      cPsipVct vct(m_setup.device.get());
      ChannelVector vctChannels = vct.GetChannels();

      for (ChannelVector::const_iterator it = patChannels.begin(); it != patChannels.end(); ++it)
      {
        const ChannelPtr& patChannel = *it;
        for (ChannelVector::const_iterator it2 = vctChannels.begin(); it2 != vctChannels.end(); ++it2)
        {
          const ChannelPtr& vctChannel = *it2;
          if (patChannel->Tsid() == vctChannel->Tsid() &&
              patChannel->Sid()  == vctChannel->Sid())
          {
            patChannel->SetName(vctChannel->Name(), vctChannel->ShortName(), vctChannel->Provider());
            // TODO: Copy transponder data
            // TODO: Copy major/minor channel number
          }
        }
      }

      if (!patChannels.empty())
        cChannelManager::Get().AddChannels(patChannels);
    }
  }

  m_percentage = 100.0f;

  return NULL;
}

}
