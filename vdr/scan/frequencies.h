/*
 * frequencies.h: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */


#ifndef __WIRBELSCAN_FREQUENCIES_H_
#define __WIRBELSCAN_FREQUENCIES_H_


typedef struct cFreqlist {
    const char       * channelname;
    const char       * shortname;
    int                channelfreq;
} _chanlist;

typedef struct cFreqlists {
    const char       * freqlist_name;
    struct cFreqlist * freqlist;
    int                freqlist_count;
} _chanlists;


typedef struct cCountry {
    int                freqlist_id;
    int                videonorm;
    int                is_Offset;
    int                freq_Offset;
} _country;


#define FREQ_COUNT(x) (sizeof(x)/sizeof(struct cFreqlist))

extern const struct cFreqlists   freqlists[];
extern const struct cCountry     CountryProperties[];
extern const char             *  CountryNames[];

int choose_country_analog (const char * country, int * channellist);

int choose_country_analog_fm (const char * country, int * channellist);

#endif
