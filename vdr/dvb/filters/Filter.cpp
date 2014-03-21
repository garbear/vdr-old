/*
 * filter.c: Section filter
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: filter.c 2.0 2004/01/11 13:31:34 kls Exp $
 */

#include "Filter.h"
#include "FilterData.h"
#include "devices/subsystems/DeviceSectionFilterSubsystem.h"

#include <assert.h>
#include <vector>

using namespace std;

namespace VDR
{

// --- cSectionSyncer --------------------------------------------------------

cSectionSyncer::cSectionSyncer(void)
{
  Reset();
}

void cSectionSyncer::Reset(void)
{
  lastVersion = 0xFF;
  synced = false;
}

bool cSectionSyncer::Sync(uchar version, int number, int lastNumber)
{
  if (version == lastVersion)
    return false;

  if (!synced)
  {
    if (number != 0)
      return false; // sync on first section
    synced = true;
  }

  if (number == lastNumber)
    lastVersion = version;

  return synced;
}

// --- cFilter ---------------------------------------------------------------

cFilter::cFilter(cDevice* device)
 : m_device(device),
   m_bEnabled(false)
{
  assert(m_device);
}

cFilter::cFilter(cDevice* device, u_short pid, u_char tid, u_char mask /* = 0xFF */)
: m_device(device),
  m_bEnabled(false)
{
  Set(pid, tid, mask);
}

cFilter::~cFilter(void)
{
  Enable(false);
}

bool cFilter::Matches(u_short pid, u_char tid)
{
  if (m_bEnabled)
  {
    for (vector<cFilterData>::const_iterator it = m_data.begin(); it != m_data.end(); ++it)
    {
      if (it->Matches(pid, tid))
        return true;
    }
  }
  return false;
}

int cFilter::Source(void)
{
  return m_device->SectionFilter()->GetSource();
}

int cFilter::Transponder(void)
{
  return m_device->SectionFilter()->GetTransponder();
}

ChannelPtr cFilter::Channel(void)
{
  return m_device->SectionFilter()->GetChannel();
}

void cFilter::Enable(bool bEnabled)
{
  if (bEnabled != m_bEnabled)
  {
    if (bEnabled)
    {
      // Attach ourselves to our device. The device will query our filter data
      // and open the necessary handles.
      m_device->SectionFilter()->RegisterFilter(this);
      m_bEnabled = true;
    }
    else
    {
      m_bEnabled = false;
      // Remove ourselves from our device.
      m_device->SectionFilter()->UnregisterFilter(this);
    }
  }
}

void cFilter::Set(u_short pid, u_char tid, u_char mask /* = 0xFF */, bool bSticky /* = true */)
{
  cFilterData filterData(pid, tid, mask, bSticky);
  m_data.push_back(filterData);

  // Update the open handles for this filter
  if (m_bEnabled)
    m_device->SectionFilter()->RegisterFilter(this);
}

void cFilter::Del(u_short pid, u_char tid, u_char mask)
{
  cFilterData filterData(pid, tid, mask, true);
  vector<cFilterData>::iterator it = std::find(m_data.begin(), m_data.end(), filterData);
  if (it != m_data.end())
    m_data.erase(it);

  // Update the open handles for this filter (expired ones will be removed)
  if (m_bEnabled)
    m_device->SectionFilter()->RegisterFilter(this);
}

}
