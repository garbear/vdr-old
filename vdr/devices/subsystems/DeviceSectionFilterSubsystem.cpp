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

#include "DeviceSectionFilterSubsystem.h"
#include "dvb/EIT.h"
#include "dvb/NIT.h"
#include "dvb/PAT.h"
#include "dvb/SDT.h"
#include "dvb/Sections.h"

#include <unistd.h>

cDeviceSectionFilterSubsystem::cDeviceSectionFilterSubsystem(cDevice *device)
 : cDeviceSubsystem(device),
   m_sectionHandler(NULL),
   m_eitFilter(NULL),
   m_patFilter(NULL),
   m_sdtFilter(NULL),
   m_nitFilter(NULL)
{
}


void cDeviceSectionFilterSubsystem::StartSectionHandler()
{
  if (!m_sectionHandler)
  {
    m_sectionHandler = new cSectionHandler(Device());
    AttachFilter(m_eitFilter = new cEitFilter);
    AttachFilter(m_patFilter = new cPatFilter);
    AttachFilter(m_sdtFilter = new cSdtFilter(m_patFilter));
    AttachFilter(m_nitFilter = new cNitFilter);
  }
}

void cDeviceSectionFilterSubsystem::StopSectionHandler()
{
  if (m_sectionHandler)
  {
    delete m_nitFilter;
    delete m_sdtFilter;
    delete m_patFilter;
    delete m_eitFilter;
    delete m_sectionHandler;
    m_nitFilter = NULL;
    m_sdtFilter = NULL;
    m_patFilter = NULL;
    m_eitFilter = NULL;
    m_sectionHandler = NULL;
  }
}

int cDeviceSectionFilterSubsystem::ReadFilter(int handle, void *buffer, size_t length)
{
  return safe_read(handle, buffer, length);
}

void cDeviceSectionFilterSubsystem::CloseFilter(int handle)
{
  close(handle);
}

void cDeviceSectionFilterSubsystem::AttachFilter(cFilter *filter)
{
  if (m_sectionHandler)
    m_sectionHandler->Attach(filter);
}

void cDeviceSectionFilterSubsystem::Detach(cFilter *filter)
{
  if (m_sectionHandler)
    m_sectionHandler->Detach(filter);
}
