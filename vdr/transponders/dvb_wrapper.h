/*
 * dvb_wrapper.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * wirbelscan-0.0.5
 *
 * $Id$ v20101210
 */

/* ARGHS! I hate that unsolved API problems with DVB..
 * Three different API approaches and one additional
 * wrapper to the old one for vdr-1.7.x 
 */
#pragma once

#include "transponders/TransponderTypes.h"

namespace VDR
{

/*!
 * Wrapper to convert types to Linux DVB API.
 */
fe_code_rate_t          CableSatCodeRates(eCableSatCodeRates cr);
fe_modulation_t         CableModulations(eCableModulations cm);
fe_modulation_t         AtscModulations(eAtscModulations am);
fe_polarization_t       SatPolarizations(eSatPolarizations sp);
fe_rolloff_t            SatRollOffs(eSatRollOffs ro);
fe_delivery_system_t    SatSystems(eSatSystems ss);
fe_modulation_t         SatModulationTypes(eSatModulationTypes mt);
int                     TerrBandwidths(eTerrBandwidths tb);
fe_modulation_t         TerrConstellations(eTerrConstellations tc);
fe_hierarchy_t          TerrHierarchies(eTerrHierarchies th);
fe_code_rate_t          TerrCodeRates(eTerrCodeRates cr);
fe_guard_interval_t     TerrGuardIntervals(eTerrGuardIntervals gi);
fe_transmit_mode_t      TerrTransmissionModes(eTerrTransmissionModes tm);
fe_spectral_inversion_t CableTerrInversions(eCableTerrInversions in);

}
