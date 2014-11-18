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

#include "SectionSyncer.h"

#define INVALID_VERSION  0xFF

namespace VDR
{

// --- cSectionSyncer --------------------------------------------------------

void cSectionSyncer::Reset(void)
{
  m_previousVersion = INVALID_VERSION;
  m_bSynced = false;
}

cSectionSyncer::SYNC_STATUS cSectionSyncer::Sync(uint8_t version, int sectionNumber, int endSectionNumber)
{
  // Wait for version change
  if (version == m_previousVersion)
    return SYNC_STATUS_OLD_VERSION; // No change in version

  // If we're not already synced, then sync on first section
  if (!m_bSynced)
  {
    if (sectionNumber == 0)
      m_bSynced = true;
  }

  if (!m_bSynced)
    return SYNC_STATUS_NOT_SYNCED;

  // If all sections have passed by, record the new version for next time
  if (sectionNumber == endSectionNumber)
    m_previousVersion = version;

  return SYNC_STATUS_NEW_VERSION;
}

}
