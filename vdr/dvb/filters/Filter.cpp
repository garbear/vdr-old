/*
 * filter.c: Section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: filter.c 2.0 2004/01/11 13:31:34 kls Exp $
 */

#include "Filter.h"
#include "devices/subsystems/DeviceChannelSubsystem.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"

#include <assert.h>
#include <vector>

using namespace std;

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

// --- cFilter ----------------------------------------------------------------

cFilter::cFilter(cDevice* device)
: m_device(device)
{
  assert(m_device);
  m_device->SectionFilter()->RegisterFilter(this);
}

cFilter::~cFilter(void)
{
  m_device->SectionFilter()->UnregisterFilter(this);
}

void cFilter::OpenResource(u_short pid, u_char tid, u_char mask /* = 0xFF */)
{
  FilterResourcePtr newResource = m_device->SectionFilter()->OpenResource(pid, tid, mask);

  if (newResource)
    m_resources.insert(newResource);
}

bool cFilter::GetSection(uint16_t& pid, std::vector<uint8_t>& data)
{
  return m_device->SectionFilter()->GetSection(m_resources, pid, data);
}

const cChannel* cFilter::GetCurrentlyTunedTransponder(void) const
{
  return m_device->Channel()->GetCurrentlyTunedTransponder();
}

}
