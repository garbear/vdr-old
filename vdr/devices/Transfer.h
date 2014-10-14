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

#include "Player.h"
#include "Receiver.h"
#include "Remux.h"
#include "channels/ChannelTypes.h"

#include <stdint.h>

namespace VDR
{

class cTransfer : public iReceiver, public cPlayer
{
public:
  cTransfer(const ChannelPtr& Channel);
  virtual ~cTransfer(void);

  virtual void Start(void);
  virtual void Stop(void);

  virtual void Receive(const uint16_t pid, const uint8_t* data, const size_t len);

private:
  cPatPmtGenerator patPmtGenerator;
  const ChannelPtr m_channel;
};

}
