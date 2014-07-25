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

#include "FrontendCapabilities.h"
#include "utils/log/Log.h"
#include "utils/Tools.h"

namespace VDR
{

cFrontendCapabilities::cFrontendCapabilities(fe_caps_t caps, fe_spectral_inversion_t fallback)
 : caps_inversion(INVERSION_AUTO),
   caps_qam(QAM_AUTO),
   caps_transmission_mode(TRANSMISSION_MODE_AUTO),
   caps_guard_interval(GUARD_INTERVAL_AUTO),
   caps_hierarchy(HIERARCHY_AUTO),
   caps_fec(FEC_AUTO),
   caps_s2(true)
{
  if (!(caps & FE_CAN_INVERSION_AUTO))
  {
    dsyslog("INVERSION_AUTO not supported, trying %s", fallback == INVERSION_ON ? "INVERSION_ON" : "INVERSION_OFF");
    caps_inversion = fallback;
  }
  if (!(caps & FE_CAN_QAM_AUTO))
  {
    dsyslog("QAM_AUTO not supported, trying QAM_64 (and possibly other modulations)");
    caps_qam = QAM_64;
  }
  if (!(caps & FE_CAN_TRANSMISSION_MODE_AUTO))
  {
    caps_transmission_mode = TRANSMISSION_MODE_8K;
    dsyslog("TRANSMISSION_MODE not supported, trying TRANSMISSION_MODE_8K");
  }
  if (!(caps & FE_CAN_GUARD_INTERVAL_AUTO))
  {
    dsyslog("GUARD_INTERVAL_AUTO not supported, trying GUARD_INTERVAL_1_8");
    caps_guard_interval = GUARD_INTERVAL_1_8;
  }
  if (!(caps & FE_CAN_HIERARCHY_AUTO))
  {
    dsyslog("HIERARCHY_AUTO not supported, trying HIERARCHY_NONE");
    caps_hierarchy = HIERARCHY_NONE;
  }
  if (!(caps & FE_CAN_FEC_AUTO))
  {
    dsyslog("FEC_AUTO not supported, trying FEC_NONE");
    caps_fec = FEC_NONE;
  }
  if (!(caps & FE_CAN_2G_MODULATION))
  {
    dsyslog("DVB S2 not supported");
    caps_s2 = false;
  }
}

}
