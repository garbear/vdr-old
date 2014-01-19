/*
 * wirbelscan: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __SCAN_H__
#define __SCAN_H__

#include "common.h"

#define dprintf(level, fmt...)			\
	do {					\
		if (level <= wSetup.verbosity)	{	\
			fprintf(stderr, fmt); }	\
	} while (0)

#define dpprintf(level, fmt, args...) \
	dprintf(level, "%s:%d: " fmt, __FUNCTION__, __LINE__ , ##args)

#define warning(msg...) dprintf(1, "WARNING: " msg)
#define info(msg...) dprintf(2, msg)

#endif
