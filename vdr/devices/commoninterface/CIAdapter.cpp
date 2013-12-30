/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
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

#include "CIAdapter.h"
#include "CamSlot.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))
#endif

cCiAdapter::cCiAdapter()// : CThread("CI adapter"),
 : m_assignedDevice(NULL)
{
  for (int i = 0; i < ARRAY_SIZE(m_camSlots); i++)
    m_camSlots[i] = NULL;
}

cCiAdapter::~cCiAdapter()
{
  StopThread(3000);
  for (int i = 0; i < ARRAY_SIZE(m_camSlots); i++)
    delete m_camSlots[i];
}

void cCiAdapter::AddCamSlot(cCamSlot *camSlot)
{
  if (camSlot)
  {
    for (int i = 0; i < ARRAY_SIZE(m_camSlots); i++)
    {
      if (!m_camSlots[i])
      {
        camSlot->slotIndex = i;
        m_camSlots[i] = camSlot;
        return;
      }
    }
    //esyslog("ERROR: no free CAM slot in CI adapter");
  }
}

bool cCiAdapter::Ready(void)
{
  for (int i = 0; i < ARRAY_SIZE(m_camSlots); i++)
  {
    if (m_camSlots[i] && !m_camSlots[i]->Ready())
      return false;
  }
  return true;
}

void *cCiAdapter::Process()
{
  cTPDU TPDU;
  while (!IsStopped())
  {
    int n = Read(TPDU.Buffer(), TPDU.MaxSize());
    if (n > 0 && TPDU.Slot() < ARRAY_SIZE(m_camSlots))
    {
      TPDU.SetSize(n);
      cCamSlot *cs = m_camSlots[TPDU.Slot()];
      TPDU.Dump(cs ? cs->SlotNumber() : 0, false);
      if (cs)
        cs->Process(&TPDU);
    }
    for (int i = 0; i < ARRAY_SIZE(m_camSlots); i++)
    {
      if (m_camSlots[i])
        m_camSlots[i]->Process();
    }
  }
  return NULL;
}
