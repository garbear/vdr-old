/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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
#pragma once

#include <linux/dvb/frontend.h>

class cFrontendCapabilities
{
public:
  cFrontendCapabilities(fe_caps caps, fe_spectral_inversion fallback = INVERSION_OFF);

  fe_spectral_inversion caps_inversion;
  fe_modulation         caps_qam;
  fe_transmit_mode      caps_transmission_mode;
  fe_guard_interval     caps_guard_interval;
  fe_hierarchy          caps_hierarchy;
  fe_code_rate          caps_fec;
  bool                  caps_s2;
};