/*
 * frequencies.c: wirbelscan - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */


/* see VIDEO::FREQUENCIES or other ressources from net
   there are a lot of equivalent files around
 */

#include <stdlib.h>
#include <string.h>
#include <linux/types.h>

#include "frequencies.h"
#include "countries.h"
#include "common.h"
#include <linux/videodev2.h>

using namespace COUNTRY;

/* US NTSC broadcast */

#define FREQ_NTSC_BCAST \
    { "NTSC 2",  "ch 2",      55250 }, \
    { "NTSC 3",  "ch 3",      61250 }, \
    { "NTSC 4",  "ch 4",      67250 }, \
    { "NTSC 5",  "ch 5",      77250 }, \
    { "NTSC 6",  "ch 6",      83250 }, \
    { "NTSC 7",  "ch 7",     175250 }, \
    { "NTSC 8",  "ch 8",     181250 }, \
    { "NTSC 9",  "ch 9",     187250 }, \
    { "NTSC 10", "ch 10",    193250 }, \
    { "NTSC 11", "ch 11",    199250 }, \
    { "NTSC 12", "ch 12",    205250 }, \
    { "NTSC 13", "ch 13",    211250 }, \
    { "NTSC 14", "ch 14",    471250 }, \
    { "NTSC 15", "ch 15",    477250 }, \
    { "NTSC 16", "ch 16",    483250 }, \
    { "NTSC 17", "ch 17",    489250 }, \
    { "NTSC 18", "ch 18",    495250 }, \
    { "NTSC 19", "ch 19",    501250 }, \
    { "NTSC 20", "ch 20",    507250 }, \
    { "NTSC 21", "ch 21",    513250 }, \
    { "NTSC 22", "ch 22",    519250 }, \
    { "NTSC 23", "ch 23",    525250 }, \
    { "NTSC 24", "ch 24",    531250 }, \
    { "NTSC 25", "ch 25",    537250 }, \
    { "NTSC 26", "ch 26",    543250 }, \
    { "NTSC 27", "ch 27",    549250 }, \
    { "NTSC 28", "ch 28",    555250 }, \
    { "NTSC 29", "ch 29",    561250 }, \
    { "NTSC 30", "ch 30",    567250 }, \
    { "NTSC 31", "ch 31",    573250 }, \
    { "NTSC 32", "ch 32",    579250 }, \
    { "NTSC 33", "ch 33",    585250 }, \
    { "NTSC 34", "ch 34",    591250 }, \
    { "NTSC 35", "ch 35",    597250 }, \
    { "NTSC 36", "ch 36",    603250 }, \
    { "NTSC 37", "ch 37",    609250 }, \
    { "NTSC 38", "ch 38",    615250 }, \
    { "NTSC 39", "ch 39",    621250 }, \
    { "NTSC 40", "ch 40",    627250 }, \
    { "NTSC 41", "ch 41",    633250 }, \
    { "NTSC 42", "ch 42",    639250 }, \
    { "NTSC 43", "ch 43",    645250 }, \
    { "NTSC 44", "ch 44",    651250 }, \
    { "NTSC 45", "ch 45",    657250 }, \
    { "NTSC 46", "ch 46",    663250 }, \
    { "NTSC 47", "ch 47",    669250 }, \
    { "NTSC 48", "ch 48",    675250 }, \
    { "NTSC 49", "ch 49",    681250 }, \
    { "NTSC 50", "ch 50",    687250 }, \
    { "NTSC 51", "ch 51",    693250 }, \
    { "NTSC 52", "ch 52",    699250 }, \
    { "NTSC 53", "ch 53",    705250 }, \
    { "NTSC 54", "ch 54",    711250 }, \
    { "NTSC 55", "ch 55",    717250 }, \
    { "NTSC 56", "ch 56",    723250 }, \
    { "NTSC 57", "ch 57",    729250 }, \
    { "NTSC 58", "ch 58",    735250 }, \
    { "NTSC 59", "ch 59",    741250 }, \
    { "NTSC 60", "ch 60",    747250 }, \
    { "NTSC 61", "ch 61",    753250 }, \
    { "NTSC 62", "ch 62",    759250 }, \
    { "NTSC 63", "ch 63",    765250 }, \
    { "NTSC 64", "ch 64",    771250 }, \
    { "NTSC 65", "ch 65",    777250 }, \
    { "NTSC 66", "ch 66",    783250 }, \
    { "NTSC 67", "ch 67",    789250 }, \
    { "NTSC 68", "ch 68",    795250 }, \
    { "NTSC 69", "ch 69",    801250 }, \
    { "NTSC 70", "ch 70",    807250 }, \
    { "NTSC 71", "ch 71",    813250 }, \
    { "NTSC 72", "ch 72",    819250 }, \
    { "NTSC 73", "ch 73",    825250 }, \
    { "NTSC 74", "ch 74",    831250 }, \
    { "NTSC 75", "ch 75",    837250 }, \
    { "NTSC 76", "ch 76",    843250 }, \
    { "NTSC 77", "ch 77",    849250 }, \
    { "NTSC 78", "ch 78",    855250 }, \
    { "NTSC 79", "ch 79",    861250 }, \
    { "NTSC 80", "ch 80",    867250 }, \
    { "NTSC 81", "ch 81",    873250 }, \
    { "NTSC 82", "ch 82",    879250 }, \
    { "NTSC 83", "ch 83",    885250 }

/* US NTSC cable */

#define FREQ_NTSC_CABLE \
    { "NTSC 1",   "ch 1",     73250 }, \
    { "NTSC 2",   "ch 2",     55250 }, \
    { "NTSC 3",   "ch 3",     61250 }, \
    { "NTSC 4",   "ch 4",     67250 }, \
    { "NTSC 5",   "ch 5",     77250 }, \
    { "NTSC 6",   "ch 6",     83250 }, \
    { "NTSC 7",   "ch 7",    175250 }, \
    { "NTSC 8",   "ch 8",    181250 }, \
    { "NTSC 9",   "ch 9",    187250 }, \
    { "NTSC 10",  "ch 10",   193250 }, \
    { "NTSC 11",  "ch 11",   199250 }, \
    { "NTSC 12",  "ch 12",   205250 }, \
    { "NTSC 13",  "ch 13",   211250 }, \
    { "NTSC 14",  "ch 14",   121250 }, \
    { "NTSC 15",  "ch 15",   127250 }, \
    { "NTSC 16",  "ch 16",   133250 }, \
    { "NTSC 17",  "ch 17",   139250 }, \
    { "NTSC 18",  "ch 18",   145250 }, \
    { "NTSC 19",  "ch 19",   151250 }, \
    { "NTSC 20",  "ch 20",   157250 }, \
    { "NTSC 21",  "ch 21",   163250 }, \
    { "NTSC 22",  "ch 22",   169250 }, \
    { "NTSC 23",  "ch 23",   217250 }, \
    { "NTSC 24",  "ch 24",   223250 }, \
    { "NTSC 25",  "ch 25",   229250 }, \
    { "NTSC 26",  "ch 26",   235250 }, \
    { "NTSC 27",  "ch 27",   241250 }, \
    { "NTSC 28",  "ch 28",   247250 }, \
    { "NTSC 29",  "ch 29",   253250 }, \
    { "NTSC 30",  "ch 30",   259250 }, \
    { "NTSC 31",  "ch 31",   265250 }, \
    { "NTSC 32",  "ch 32",   271250 }, \
    { "NTSC 33",  "ch 33",   277250 }, \
    { "NTSC 34",  "ch 34",   283250 }, \
    { "NTSC 35",  "ch 35",   289250 }, \
    { "NTSC 36",  "ch 36",   295250 }, \
    { "NTSC 37",  "ch 37",   301250 }, \
    { "NTSC 38",  "ch 38",   307250 }, \
    { "NTSC 39",  "ch 39",   313250 }, \
    { "NTSC 40",  "ch 40",   319250 }, \
    { "NTSC 41",  "ch 41",   325250 }, \
    { "NTSC 42",  "ch 42",   331250 }, \
    { "NTSC 43",  "ch 43",   337250 }, \
    { "NTSC 44",  "ch 44",   343250 }, \
    { "NTSC 45",  "ch 45",   349250 }, \
    { "NTSC 46",  "ch 46",   355250 }, \
    { "NTSC 47",  "ch 47",   361250 }, \
    { "NTSC 48",  "ch 48",   367250 }, \
    { "NTSC 49",  "ch 49",   373250 }, \
    { "NTSC 50",  "ch 50",   379250 }, \
    { "NTSC 51",  "ch 51",   385250 }, \
    { "NTSC 52",  "ch 52",   391250 }, \
    { "NTSC 53",  "ch 53",   397250 }, \
    { "NTSC 54",  "ch 54",   403250 }, \
    { "NTSC 55",  "ch 55",   409250 }, \
    { "NTSC 56",  "ch 56",   415250 }, \
    { "NTSC 57",  "ch 57",   421250 }, \
    { "NTSC 58",  "ch 58",   427250 }, \
    { "NTSC 59",  "ch 59",   433250 }, \
    { "NTSC 60",  "ch 60",   439250 }, \
    { "NTSC 61",  "ch 61",   445250 }, \
    { "NTSC 62",  "ch 62",   451250 }, \
    { "NTSC 63",  "ch 63",   457250 }, \
    { "NTSC 64",  "ch 64",   463250 }, \
    { "NTSC 65",  "ch 65",   469250 }, \
    { "NTSC 66",  "ch 66",   475250 }, \
    { "NTSC 67",  "ch 67",   481250 }, \
    { "NTSC 68",  "ch 68",   487250 }, \
    { "NTSC 69",  "ch 69",   493250 }, \
    { "NTSC 70",  "ch 70",   499250 }, \
    { "NTSC 71",  "ch 71",   505250 }, \
    { "NTSC 72",  "ch 72",   511250 }, \
    { "NTSC 73",  "ch 73",   517250 }, \
    { "NTSC 74",  "ch 74",   523250 }, \
    { "NTSC 75",  "ch 75",   529250 }, \
    { "NTSC 76",  "ch 76",   535250 }, \
    { "NTSC 77",  "ch 77",   541250 }, \
    { "NTSC 78",  "ch 78",   547250 }, \
    { "NTSC 79",  "ch 79",   553250 }, \
    { "NTSC 80",  "ch 80",   559250 }, \
    { "NTSC 81",  "ch 81",   565250 }, \
    { "NTSC 82",  "ch 82",   571250 }, \
    { "NTSC 83",  "ch 83",   577250 }, \
    { "NTSC 84",  "ch 84",   583250 }, \
    { "NTSC 85",  "ch 85",   589250 }, \
    { "NTSC 86",  "ch 86",   595250 }, \
    { "NTSC 87",  "ch 87",   601250 }, \
    { "NTSC 88",  "ch 88",   607250 }, \
    { "NTSC 89",  "ch 89",   613250 }, \
    { "NTSC 90",  "ch 90",   619250 }, \
    { "NTSC 91",  "ch 91",   625250 }, \
    { "NTSC 92",  "ch 92",   631250 }, \
    { "NTSC 93",  "ch 93",   637250 }, \
    { "NTSC 94",  "ch 94",   643250 }, \
    { "NTSC 95",  "ch 95",    91250 }, \
    { "NTSC 96",  "ch 96",    97250 }, \
    { "NTSC 97",  "ch 97",   103250 }, \
    { "NTSC 98",  "ch 98",   109250 }, \
    { "NTSC 99",  "ch 99",   115250 }, \
    { "NTSC 100", "ch 100",  649250 }, \
    { "NTSC 101", "ch 101",  655250 }, \
    { "NTSC 102", "ch 102",  661250 }, \
    { "NTSC 103", "ch 103",  667250 }, \
    { "NTSC 104", "ch 104",  673250 }, \
    { "NTSC 105", "ch 105",  679250 }, \
    { "NTSC 106", "ch 106",  685250 }, \
    { "NTSC 107", "ch 107",  691250 }, \
    { "NTSC 108", "ch 108",  697250 }, \
    { "NTSC 109", "ch 109",  703250 }, \
    { "NTSC 110", "ch 110",  709250 }, \
    { "NTSC 111", "ch 111",  715250 }, \
    { "NTSC 112", "ch 112",  721250 }, \
    { "NTSC 113", "ch 113",   27250 }, \
    { "NTSC 114", "ch 114",  733250 }, \
    { "NTSC 115", "ch 115",  739250 }, \
    { "NTSC 116", "ch 116",  745250 }, \
    { "NTSC 117", "ch 117",  751250 }, \
    { "NTSC 118", "ch 118",  757250 }, \
    { "NTSC 119", "ch 119",  763250 }, \
    { "NTSC 120", "ch 120",  769250 }, \
    { "NTSC 121", "ch 121",  775250 }, \
    { "NTSC 122", "ch 122",  781250 }, \
    { "NTSC 123", "ch 123",  787250 }, \
    { "NTSC 124", "ch 124",  793250 }, \
    { "NTSC 125", "ch 125",  799250 }, \
    { "NTSC T7",  "ch T7",     8250 }, \
    { "NTSC T8",  "ch T8",    14250 }, \
    { "NTSC T9",  "ch T9",    20250 }, \
    { "NTSC T10", "ch T10",   26250 }, \
    { "NTSC T11", "ch T11",   32250 }, \
    { "NTSC T12", "ch T12",   38250 }, \
    { "NTSC T13", "ch T13",   44250 }, \
    { "NTSC T14", "ch T14",   50250 }

    
    
/* US NTSC HRC */

#define FREQ_NTSC_HRC \
    { "NTSC HRC 1",   "HRC 1",      72000 }, \
    { "NTSC HRC 2",   "HRC 2",      54000 }, \
    { "NTSC HRC 3",   "HRC 3",      60000 }, \
    { "NTSC HRC 4",   "HRC 4",      66000 }, \
    { "NTSC HRC 5",   "HRC 5",      78000 }, \
    { "NTSC HRC 6",   "HRC 6",      84000 }, \
    { "NTSC HRC 7",   "HRC 7",     174000 }, \
    { "NTSC HRC 8",   "HRC 8",     180000 }, \
    { "NTSC HRC 9",   "HRC 9",     186000 }, \
    { "NTSC HRC 10",  "HRC 10",    192000 }, \
    { "NTSC HRC 11",  "HRC 11",    198000 }, \
    { "NTSC HRC 12",  "HRC 12",    204000 }, \
    { "NTSC HRC 13",  "HRC 13",    210000 }, \
    { "NTSC HRC 14",  "HRC 14",    120000 }, \
    { "NTSC HRC 15",  "HRC 15",    126000 }, \
    { "NTSC HRC 16",  "HRC 16",    132000 }, \
    { "NTSC HRC 17",  "HRC 17",    138000 }, \
    { "NTSC HRC 18",  "HRC 18",    144000 }, \
    { "NTSC HRC 19",  "HRC 19",    150000 }, \
    { "NTSC HRC 20",  "HRC 20",    156000 }, \
    { "NTSC HRC 21",  "HRC 21",    162000 }, \
    { "NTSC HRC 22",  "HRC 22",    168000 }, \
    { "NTSC HRC 23",  "HRC 23",    216000 }, \
    { "NTSC HRC 24",  "HRC 24",    222000 }, \
    { "NTSC HRC 25",  "HRC 25",    228000 }, \
    { "NTSC HRC 26",  "HRC 26",    234000 }, \
    { "NTSC HRC 27",  "HRC 27",    240000 }, \
    { "NTSC HRC 28",  "HRC 28",    246000 }, \
    { "NTSC HRC 29",  "HRC 29",    252000 }, \
    { "NTSC HRC 30",  "HRC 30",    258000 }, \
    { "NTSC HRC 31",  "HRC 31",    264000 }, \
    { "NTSC HRC 32",  "HRC 32",    270000 }, \
    { "NTSC HRC 33",  "HRC 33",    276000 }, \
    { "NTSC HRC 34",  "HRC 34",    282000 }, \
    { "NTSC HRC 35",  "HRC 35",    288000 }, \
    { "NTSC HRC 36",  "HRC 36",    294000 }, \
    { "NTSC HRC 37",  "HRC 37",    300000 }, \
    { "NTSC HRC 38",  "HRC 38",    306000 }, \
    { "NTSC HRC 39",  "HRC 39",    312000 }, \
    { "NTSC HRC 40",  "HRC 40",    318000 }, \
    { "NTSC HRC 41",  "HRC 41",    324000 }, \
    { "NTSC HRC 42",  "HRC 42",    330000 }, \
    { "NTSC HRC 43",  "HRC 43",    336000 }, \
    { "NTSC HRC 44",  "HRC 44",    342000 }, \
    { "NTSC HRC 45",  "HRC 45",    348000 }, \
    { "NTSC HRC 46",  "HRC 46",    354000 }, \
    { "NTSC HRC 47",  "HRC 47",    360000 }, \
    { "NTSC HRC 48",  "HRC 48",    366000 }, \
    { "NTSC HRC 49",  "HRC 49",    372000 }, \
    { "NTSC HRC 50",  "HRC 50",    378000 }, \
    { "NTSC HRC 51",  "HRC 51",    384000 }, \
    { "NTSC HRC 52",  "HRC 52",    390000 }, \
    { "NTSC HRC 53",  "HRC 53",    396000 }, \
    { "NTSC HRC 54",  "HRC 54",    402000 }, \
    { "NTSC HRC 55",  "HRC 55",    408000 }, \
    { "NTSC HRC 56",  "HRC 56",    414000 }, \
    { "NTSC HRC 57",  "HRC 57",    420000 }, \
    { "NTSC HRC 58",  "HRC 58",    426000 }, \
    { "NTSC HRC 59",  "HRC 59",    432000 }, \
    { "NTSC HRC 60",  "HRC 60",    438000 }, \
    { "NTSC HRC 61",  "HRC 61",    444000 }, \
    { "NTSC HRC 62",  "HRC 62",    450000 }, \
    { "NTSC HRC 63",  "HRC 63",    456000 }, \
    { "NTSC HRC 64",  "HRC 64",    462000 }, \
    { "NTSC HRC 65",  "HRC 65",    468000 }, \
    { "NTSC HRC 66",  "HRC 66",    474000 }, \
    { "NTSC HRC 67",  "HRC 67",    480000 }, \
    { "NTSC HRC 68",  "HRC 68",    486000 }, \
    { "NTSC HRC 69",  "HRC 69",    492000 }, \
    { "NTSC HRC 70",  "HRC 70",    498000 }, \
    { "NTSC HRC 71",  "HRC 71",    504000 }, \
    { "NTSC HRC 72",  "HRC 72",    510000 }, \
    { "NTSC HRC 73",  "HRC 73",    516000 }, \
    { "NTSC HRC 74",  "HRC 74",    522000 }, \
    { "NTSC HRC 75",  "HRC 75",    528000 }, \
    { "NTSC HRC 76",  "HRC 76",    534000 }, \
    { "NTSC HRC 77",  "HRC 77",    540000 }, \
    { "NTSC HRC 78",  "HRC 78",    546000 }, \
    { "NTSC HRC 79",  "HRC 79",    552000 }, \
    { "NTSC HRC 80",  "HRC 80",    558000 }, \
    { "NTSC HRC 81",  "HRC 81",    564000 }, \
    { "NTSC HRC 82",  "HRC 82",    570000 }, \
    { "NTSC HRC 83",  "HRC 83",    576000 }, \
    { "NTSC HRC 84",  "HRC 84",    582000 }, \
    { "NTSC HRC 85",  "HRC 85",    588000 }, \
    { "NTSC HRC 86",  "HRC 86",    594000 }, \
    { "NTSC HRC 87",  "HRC 87",    600000 }, \
    { "NTSC HRC 88",  "HRC 88",    606000 }, \
    { "NTSC HRC 89",  "HRC 89",    612000 }, \
    { "NTSC HRC 90",  "HRC 90",    618000 }, \
    { "NTSC HRC 91",  "HRC 91",    624000 }, \
    { "NTSC HRC 92",  "HRC 92",    630000 }, \
    { "NTSC HRC 93",  "HRC 93",    636000 }, \
    { "NTSC HRC 94",  "HRC 94",    642000 }, \
    { "NTSC HRC 95",  "HRC 95",     90000 }, \
    { "NTSC HRC 96",  "HRC 96",     96000 }, \
    { "NTSC HRC 97",  "HRC 97",    102000 }, \
    { "NTSC HRC 98",  "HRC 98",    108000 }, \
    { "NTSC HRC 99",  "HRC 99",    114000 }, \
    { "NTSC HRC 100", "HRC 100",   648000 }, \
    { "NTSC HRC 101", "HRC 101",   654000 }, \
    { "NTSC HRC 102", "HRC 102",   660000 }, \
    { "NTSC HRC 103", "HRC 103",   666000 }, \
    { "NTSC HRC 104", "HRC 104",   672000 }, \
    { "NTSC HRC 105", "HRC 105",   678000 }, \
    { "NTSC HRC 106", "HRC 106",   684000 }, \
    { "NTSC HRC 107", "HRC 107",   690000 }, \
    { "NTSC HRC 108", "HRC 108",   696000 }, \
    { "NTSC HRC 109", "HRC 109",   702000 }, \
    { "NTSC HRC 110", "HRC 110",   708000 }, \
    { "NTSC HRC 111", "HRC 111",   714000 }, \
    { "NTSC HRC 112", "HRC 112",   720000 }, \
    { "NTSC HRC 113", "HRC 113",   726000 }, \
    { "NTSC HRC 114", "HRC 114",   732000 }, \
    { "NTSC HRC 115", "HRC 115",   738000 }, \
    { "NTSC HRC 116", "HRC 116",   744000 }, \
    { "NTSC HRC 117", "HRC 117",   750000 }, \
    { "NTSC HRC 118", "HRC 118",   756000 }, \
    { "NTSC HRC 119", "HRC 119",   762000 }, \
    { "NTSC HRC 120", "HRC 120",   768000 }, \
    { "NTSC HRC 121", "HRC 121",   774000 }, \
    { "NTSC HRC 122", "HRC 122",   780000 }, \
    { "NTSC HRC 123", "HRC 123",   786000 }, \
    { "NTSC HRC 124", "HRC 124",   792000 }, \
    { "NTSC HRC 125", "HRC 125",   798000 }, \
    { "NTSC HRC T7",  "HRC T7",      7000 }, \
    { "NTSC HRC T8",  "HRC T8",     13000 }, \
    { "NTSC HRC T9",  "HRC T9",     19000 }, \
    { "NTSC HRC T10", "HRC T10",    25000 }, \
    { "NTSC HRC T11", "HRC T11",    31000 }, \
    { "NTSC HRC T12", "HRC T12",    37000 }, \
    { "NTSC HRC T13", "HRC T13",    43000 }, \
    { "NTSC HRC T14", "HRC T14",    49000 }

/* US NTSC IRC */

#define FREQ_NTSC_IRC \
    { "NTSC IRC 1",   "IRC 1",      73250 }, \
    { "NTSC IRC 2",   "IRC 2",      55250 }, \
    { "NTSC IRC 3",   "IRC 3",      61250 }, \
    { "NTSC IRC 4",   "IRC 4",      67250 }, \
    { "NTSC IRC 5",   "IRC 5",      79250 }, \
    { "NTSC IRC 6",   "IRC 6",      85250 }, \
    { "NTSC IRC 7",   "IRC 7",     175250 }, \
    { "NTSC IRC 8",   "IRC 8",     181250 }, \
    { "NTSC IRC 9",   "IRC 9",     187250 }, \
    { "NTSC IRC 10",  "IRC 10",    193250 }, \
    { "NTSC IRC 11",  "IRC 11",    199250 }, \
    { "NTSC IRC 12",  "IRC 12",    205250 }, \
    { "NTSC IRC 13",  "IRC 13",    211250 }, \
    { "NTSC IRC 14",  "IRC 14",    121150 }, \
    { "NTSC IRC 15",  "IRC 15",    127150 }, \
    { "NTSC IRC 16",  "IRC 16",    133150 }, \
    { "NTSC IRC 17",  "IRC 17",    139150 }, \
    { "NTSC IRC 18",  "IRC 18",    145150 }, \
    { "NTSC IRC 19",  "IRC 19",    151150 }, \
    { "NTSC IRC 20",  "IRC 20",    157150 }, \
    { "NTSC IRC 21",  "IRC 21",    163150 }, \
    { "NTSC IRC 22",  "IRC 22",    169150 }, \
    { "NTSC IRC 23",  "IRC 23",    217250 }, \
    { "NTSC IRC 24",  "IRC 24",    223250 }, \
    { "NTSC IRC 25",  "IRC 25",    229250 }, \
    { "NTSC IRC 26",  "IRC 26",    235250 }, \
    { "NTSC IRC 27",  "IRC 27",    241250 }, \
    { "NTSC IRC 28",  "IRC 28",    247250 }, \
    { "NTSC IRC 29",  "IRC 29",    253250 }, \
    { "NTSC IRC 30",  "IRC 30",    259250 }, \
    { "NTSC IRC 31",  "IRC 31",    265250 }, \
    { "NTSC IRC 32",  "IRC 32",    271250 }, \
    { "NTSC IRC 33",  "IRC 33",    277250 }, \
    { "NTSC IRC 34",  "IRC 34",    283250 }, \
    { "NTSC IRC 35",  "IRC 35",    289250 }, \
    { "NTSC IRC 36",  "IRC 36",    295250 }, \
    { "NTSC IRC 37",  "IRC 37",    301250 }, \
    { "NTSC IRC 38",  "IRC 38",    307250 }, \
    { "NTSC IRC 39",  "IRC 39",    313250 }, \
    { "NTSC IRC 40",  "IRC 40",    319250 }, \
    { "NTSC IRC 41",  "IRC 41",    325250 }, \
    { "NTSC IRC 42",  "IRC 42",    331250 }, \
    { "NTSC IRC 43",  "IRC 43",    337250 }, \
    { "NTSC IRC 44",  "IRC 44",    343250 }, \
    { "NTSC IRC 45",  "IRC 45",    349250 }, \
    { "NTSC IRC 46",  "IRC 46",    355250 }, \
    { "NTSC IRC 47",  "IRC 47",    361250 }, \
    { "NTSC IRC 48",  "IRC 48",    367250 }, \
    { "NTSC IRC 49",  "IRC 49",    373250 }, \
    { "NTSC IRC 50",  "IRC 50",    379250 }, \
    { "NTSC IRC 51",  "IRC 51",    385250 }, \
    { "NTSC IRC 52",  "IRC 52",    391250 }, \
    { "NTSC IRC 53",  "IRC 53",    397250 }, \
    { "NTSC IRC 54",  "IRC 54",    403250 }, \
    { "NTSC IRC 55",  "IRC 55",    409250 }, \
    { "NTSC IRC 56",  "IRC 56",    415250 }, \
    { "NTSC IRC 57",  "IRC 57",    421250 }, \
    { "NTSC IRC 58",  "IRC 58",    427250 }, \
    { "NTSC IRC 59",  "IRC 59",    433250 }, \
    { "NTSC IRC 60",  "IRC 60",    439250 }, \
    { "NTSC IRC 61",  "IRC 61",    445250 }, \
    { "NTSC IRC 62",  "IRC 62",    451250 }, \
    { "NTSC IRC 63",  "IRC 63",    457250 }, \
    { "NTSC IRC 64",  "IRC 64",    463250 }, \
    { "NTSC IRC 65",  "IRC 65",    469250 }, \
    { "NTSC IRC 66",  "IRC 66",    475250 }, \
    { "NTSC IRC 67",  "IRC 67",    481250 }, \
    { "NTSC IRC 68",  "IRC 68",    487250 }, \
    { "NTSC IRC 69",  "IRC 69",    493250 }, \
    { "NTSC IRC 70",  "IRC 70",    499250 }, \
    { "NTSC IRC 71",  "IRC 71",    505250 }, \
    { "NTSC IRC 72",  "IRC 72",    511250 }, \
    { "NTSC IRC 73",  "IRC 73",    517250 }, \
    { "NTSC IRC 74",  "IRC 74",    523250 }, \
    { "NTSC IRC 75",  "IRC 75",    529250 }, \
    { "NTSC IRC 76",  "IRC 76",    535250 }, \
    { "NTSC IRC 77",  "IRC 77",    541250 }, \
    { "NTSC IRC 78",  "IRC 78",    547250 }, \
    { "NTSC IRC 79",  "IRC 79",    553250 }, \
    { "NTSC IRC 80",  "IRC 80",    559250 }, \
    { "NTSC IRC 81",  "IRC 81",    565250 }, \
    { "NTSC IRC 82",  "IRC 82",    571250 }, \
    { "NTSC IRC 83",  "IRC 83",    577250 }, \
    { "NTSC IRC 84",  "IRC 84",    583250 }, \
    { "NTSC IRC 85",  "IRC 85",    589250 }, \
    { "NTSC IRC 86",  "IRC 86",    595250 }, \
    { "NTSC IRC 87",  "IRC 87",    601250 }, \
    { "NTSC IRC 88",  "IRC 88",    607250 }, \
    { "NTSC IRC 89",  "IRC 89",    613250 }, \
    { "NTSC IRC 90",  "IRC 90",    619250 }, \
    { "NTSC IRC 91",  "IRC 91",    625250 }, \
    { "NTSC IRC 92",  "IRC 92",    631250 }, \
    { "NTSC IRC 93",  "IRC 93",    637250 }, \
    { "NTSC IRC 94",  "IRC 94",    643250 }, \
    { "NTSC IRC 95",  "IRC 95",     91250 }, \
    { "NTSC IRC 96",  "IRC 96",     97250 }, \
    { "NTSC IRC 97",  "IRC 97",    103250 }, \
    { "NTSC IRC 98",  "IRC 98",    109250 }, \
    { "NTSC IRC 99",  "IRC 99",    115250 }, \
    { "NTSC IRC 100", "IRC 100",   649250 }, \
    { "NTSC IRC 101", "IRC 101",   655250 }, \
    { "NTSC IRC 102", "IRC 102",   661250 }, \
    { "NTSC IRC 103", "IRC 103",   667250 }, \
    { "NTSC IRC 104", "IRC 104",   673250 }, \
    { "NTSC IRC 105", "IRC 105",   679250 }, \
    { "NTSC IRC 106", "IRC 106",   685250 }, \
    { "NTSC IRC 107", "IRC 107",   691250 }, \
    { "NTSC IRC 108", "IRC 108",   697250 }, \
    { "NTSC IRC 109", "IRC 109",   703250 }, \
    { "NTSC IRC 110", "IRC 110",   709250 }, \
    { "NTSC IRC 111", "IRC 111",   715250 }, \
    { "NTSC IRC 112", "IRC 112",   721250 }, \
    { "NTSC IRC 113", "IRC 113",   727250 }, \
    { "NTSC IRC 114", "IRC 114",   733250 }, \
    { "NTSC IRC 115", "IRC 115",   739250 }, \
    { "NTSC IRC 116", "IRC 116",   745250 }, \
    { "NTSC IRC 117", "IRC 117",   751250 }, \
    { "NTSC IRC 118", "IRC 118",   757250 }, \
    { "NTSC IRC 119", "IRC 119",   763250 }, \
    { "NTSC IRC 120", "IRC 120",   769250 }, \
    { "NTSC IRC 121", "IRC 121",   775250 }, \
    { "NTSC IRC 122", "IRC 122",   781250 }, \
    { "NTSC IRC 123", "IRC 123",   787250 }, \
    { "NTSC IRC 124", "IRC 124",   793250 }, \
    { "NTSC IRC 125", "IRC 125",   799250 }, \
    { "NTSC IRC T7",  "IRC T7",      8250 }, \
    { "NTSC IRC T8",  "IRC T8",     14250 }, \
    { "NTSC IRC T9",  "IRC T9",     20250 }, \
    { "NTSC IRC T10", "IRC T10",    26250 }, \
    { "NTSC IRC T11", "IRC T11",    32250 }, \
    { "NTSC IRC T12", "IRC T12",    38250 }, \
    { "NTSC IRC T13", "IRC T13",    44250 }, \
    { "NTSC IRC T14", "IRC T14",    50250 }


static struct cFreqlist ntsc[] = {
   FREQ_NTSC_BCAST,
   FREQ_NTSC_CABLE,
   FREQ_NTSC_HRC,
   FREQ_NTSC_IRC
};


/* JP NTSC broadcast */

#define FREQ_NTSC_BCAST_JP \
    { "NTSC JP 1",  "JP 1",      91250 }, \
    { "NTSC JP 2",  "JP 2",      97250 }, \
    { "NTSC JP 3",  "JP 3",     103250 }, \
    { "NTSC JP 4",  "JP 4",     171250 }, \
    { "NTSC JP 5",  "JP 5",     177250 }, \
    { "NTSC JP 6",  "JP 6",     183250 }, \
    { "NTSC JP 7",  "JP 7",     189250 }, \
    { "NTSC JP 8",  "JP 8",     193250 }, \
    { "NTSC JP 9",  "JP 9",     199250 }, \
    { "NTSC JP 10", "JP 10",    205250 }, \
    { "NTSC JP 11", "JP 11",    211250 }, \
    { "NTSC JP 12", "JP 12",    217250 }, \
    { "NTSC JP 13", "JP 13",    471250 }, \
    { "NTSC JP 14", "JP 14",    477250 }, \
    { "NTSC JP 15", "JP 15",    483250 }, \
    { "NTSC JP 16", "JP 16",    489250 }, \
    { "NTSC JP 17", "JP 17",    495250 }, \
    { "NTSC JP 18", "JP 18",    501250 }, \
    { "NTSC JP 19", "JP 19",    507250 }, \
    { "NTSC JP 20", "JP 20",    513250 }, \
    { "NTSC JP 21", "JP 21",    519250 }, \
    { "NTSC JP 22", "JP 22",    525250 }, \
    { "NTSC JP 23", "JP 23",    531250 }, \
    { "NTSC JP 24", "JP 24",    537250 }, \
    { "NTSC JP 25", "JP 25",    543250 }, \
    { "NTSC JP 26", "JP 26",    549250 }, \
    { "NTSC JP 27", "JP 27",    555250 }, \
    { "NTSC JP 28", "JP 28",    561250 }, \
    { "NTSC JP 29", "JP 29",    567250 }, \
    { "NTSC JP 30", "JP 30",    573250 }, \
    { "NTSC JP 31", "JP 31",    579250 }, \
    { "NTSC JP 32", "JP 32",    585250 }, \
    { "NTSC JP 33", "JP 33",    591250 }, \
    { "NTSC JP 34", "JP 34",    597250 }, \
    { "NTSC JP 35", "JP 35",    603250 }, \
    { "NTSC JP 36", "JP 36",    609250 }, \
    { "NTSC JP 37", "JP 37",    615250 }, \
    { "NTSC JP 38", "JP 38",    621250 }, \
    { "NTSC JP 39", "JP 39",    627250 }, \
    { "NTSC JP 40", "JP 40",    633250 }, \
    { "NTSC JP 41", "JP 41",    639250 }, \
    { "NTSC JP 42", "JP 42",    645250 }, \
    { "NTSC JP 43", "JP 43",    651250 }, \
    { "NTSC JP 44", "JP 44",    657250 }, \
    { "NTSC JP 45", "JP 45",    663250 }, \
    { "NTSC JP 46", "JP 46",    669250 }, \
    { "NTSC JP 47", "JP 47",    675250 }, \
    { "NTSC JP 48", "JP 48",    681250 }, \
    { "NTSC JP 49", "JP 49",    687250 }, \
    { "NTSC JP 50", "JP 50",    693250 }, \
    { "NTSC JP 51", "JP 51",    699250 }, \
    { "NTSC JP 52", "JP 52",    705250 }, \
    { "NTSC JP 53", "JP 53",    711250 }, \
    { "NTSC JP 54", "JP 54",    717250 }, \
    { "NTSC JP 55", "JP 55",    723250 }, \
    { "NTSC JP 56", "JP 56",    729250 }, \
    { "NTSC JP 57", "JP 57",    735250 }, \
    { "NTSC JP 58", "JP 58",    741250 }, \
    { "NTSC JP 59", "JP 59",    747250 }, \
    { "NTSC JP 60", "JP 60",    753250 }, \
    { "NTSC JP 61", "JP 61",    759250 }, \
    { "NTSC JP 62", "JP 62",    765250 }
 
/* JP NTSC cable */
#define FREQ_NTSC_CABLE_JP \
    { "NTSC JP 13", "JP 13",    109250 }, \
    { "NTSC JP 14", "JP 14",    115250 }, \
    { "NTSC JP 15", "JP 15",    121250 }, \
    { "NTSC JP 16", "JP 16",    127250 }, \
    { "NTSC JP 17", "JP 17",    133250 }, \
    { "NTSC JP 18", "JP 18",    139250 }, \
    { "NTSC JP 19", "JP 19",    145250 }, \
    { "NTSC JP 20", "JP 20",    151250 }, \
    { "NTSC JP 21", "JP 21",    157250 }, \
    { "NTSC JP 22", "JP 22",    165250 }, \
    { "NTSC JP 23", "JP 23",    223250 }, \
    { "NTSC JP 24", "JP 24",    231250 }, \
    { "NTSC JP 25", "JP 25",    237250 }, \
    { "NTSC JP 26", "JP 26",    243250 }, \
    { "NTSC JP 27", "JP 27",    249250 }, \
    { "NTSC JP 28", "JP 28",    253250 }, \
    { "NTSC JP 29", "JP 29",    259250 }, \
    { "NTSC JP 30", "JP 30",    265250 }, \
    { "NTSC JP 31", "JP 31",    271250 }, \
    { "NTSC JP 32", "JP 32",    277250 }, \
    { "NTSC JP 33", "JP 33",    283250 }, \
    { "NTSC JP 34", "JP 34",    289250 }, \
    { "NTSC JP 35", "JP 35",    295250 }, \
    { "NTSC JP 36", "JP 36",    301250 }, \
    { "NTSC JP 37", "JP 37",    307250 }, \
    { "NTSC JP 38", "JP 38",    313250 }, \
    { "NTSC JP 39", "JP 39",    319250 }, \
    { "NTSC JP 40", "JP 40",    325250 }, \
    { "NTSC JP 41", "JP 41",    331250 }, \
    { "NTSC JP 42", "JP 42",    337250 }, \
    { "NTSC JP 43", "JP 43",    343250 }, \
    { "NTSC JP 44", "JP 44",    349250 }, \
    { "NTSC JP 45", "JP 45",    355250 }, \
    { "NTSC JP 46", "JP 46",    361250 }, \
    { "NTSC JP 47", "JP 47",    367250 }, \
    { "NTSC JP 48", "JP 48",    373250 }, \
    { "NTSC JP 49", "JP 49",    379250 }, \
    { "NTSC JP 50", "JP 50",    385250 }, \
    { "NTSC JP 51", "JP 51",    391250 }, \
    { "NTSC JP 52", "JP 52",    397250 }, \
    { "NTSC JP 53", "JP 53",    403250 }, \
    { "NTSC JP 54", "JP 54",    409250 }, \
    { "NTSC JP 55", "JP 55",    415250 }, \
    { "NTSC JP 56", "JP 56",    421250 }, \
    { "NTSC JP 57", "JP 57",    427250 }, \
    { "NTSC JP 58", "JP 58",    433250 }, \
    { "NTSC JP 59", "JP 59",    439250 }, \
    { "NTSC JP 60", "JP 60",    445250 }, \
    { "NTSC JP 61", "JP 61",    451250 }, \
    { "NTSC JP 62", "JP 62",    457250 }, \
    { "NTSC JP 63", "JP 63",    463250 }
     
static struct cFreqlist ntsc_japan[] = {
 FREQ_NTSC_BCAST_JP,
 FREQ_NTSC_CABLE_JP
};

   
/* PAL australia */
static struct cFreqlist pal_australia[] = {
    { "PAL 0",   "ch 0",     46250 },
    { "PAL 1",   "ch 1",     57250 },
    { "PAL 2",   "ch 2",     64250 },
    { "PAL 3",   "ch 3",     86250 },
    { "PAL 4",   "ch 4",     95250 },
    { "PAL 5",   "ch 5",    102250 },
    { "PAL 5A",  "ch 5A",   138250 },
    { "PAL 6",   "ch 6",    175250 },
    { "PAL 7",   "ch 7",    182250 },
    { "PAL 8",   "ch 8",    189250 },
    { "PAL 9",   "ch 9",    196250 },
    { "PAL 10",  "ch 10",   209250 },
    { "PAL 11",  "ch 11",   216250 },
    { "PAL 28",  "ch 28",   527250 },
    { "PAL 29",  "ch 29",   534250 },
    { "PAL 30",  "ch 30",   541250 },
    { "PAL 31",  "ch 31",   548250 },
    { "PAL 32",  "ch 32",   555250 },
    { "PAL 33",  "ch 33",   562250 },
    { "PAL 34",  "ch 34",   569250 },
    { "PAL 35",  "ch 35",   576250 },
    { "PAL 36",  "ch 36",   591250 },
    { "PAL 39",  "ch 39",   604250 },
    { "PAL 40",  "ch 40",   611250 },
    { "PAL 41",  "ch 41",   618250 },
    { "PAL 42",  "ch 42",   625250 },
    { "PAL 43",  "ch 43",   632250 },
    { "PAL 44",  "ch 44",   639250 },
    { "PAL 45",  "ch 45",   646250 },
    { "PAL 46",  "ch 46",   653250 },
    { "PAL 47",  "ch 47",   660250 },
    { "PAL 48",  "ch 48",   667250 },
    { "PAL 49",  "ch 49",   674250 },
    { "PAL 50",  "ch 50",   681250 },
    { "PAL 51",  "ch 51",   688250 },
    { "PAL 52",  "ch 52",   695250 },
    { "PAL 53",  "ch 53",   702250 },
    { "PAL 54",  "ch 54",   709250 },
    { "PAL 55",  "ch 55",   716250 },
    { "PAL 56",  "ch 56",   723250 },
    { "PAL 57",  "ch 57",   730250 },
    { "PAL 58",  "ch 58",   737250 },
    { "PAL 59",  "ch 59",   744250 },
    { "PAL 60",  "ch 60",   751250 },
    { "PAL 61",  "ch 61",   758250 },
    { "PAL 62",  "ch 62",   765250 },
    { "PAL 63",  "ch 63",   772250 },
    { "PAL 64",  "ch 64",   779250 },
    { "PAL 65",  "ch 65",   786250 },
    { "PAL 66",  "ch 66",   793250 },
    { "PAL 67",  "ch 67",   800250 },
    { "PAL 68",  "ch 68",   807250 },
    { "PAL 69",  "ch 69",   814250 },
};
 
static struct cFreqlist pal_australia_optus[] = {
    { "PAL 1",   "ch 1",    138250 },
    { "PAL 2",   "ch 2",    147250 },
    { "PAL 3",   "ch 3",    154250 },
    { "PAL 4",   "ch 4",    161250 },
    { "PAL 5",   "ch 5",    168250 },
    { "PAL 6",   "ch 6",    175250 },
    { "PAL 7",   "ch 7",    182250 },
    { "PAL 8",   "ch 8",    189250 },
    { "PAL 9",   "ch 9",    196250 },
    { "PAL 10",  "ch 10",   209250 },
    { "PAL 11",  "ch 11",   216250 },
    { "PAL 12",  "ch 12",   224250 },
    { "PAL 13",  "ch 13",   231250 },
    { "PAL 14",  "ch 14",   238250 },
    { "PAL 15",  "ch 15",   245250 },
    { "PAL 16",  "ch 16",   252250 },
    { "PAL 17",  "ch 17",   259250 },
    { "PAL 18",  "ch 18",   266250 },
    { "PAL 19",  "ch 19",   273250 },
    { "PAL 20",  "ch 20",   280250 },
    { "PAL 21",  "ch 21",   287250 },
    { "PAL 22",  "ch 22",   294250 },
    { "PAL 23",  "ch 23",   303250 },
    { "PAL 24",  "ch 24",   310250 },
    { "PAL 25",  "ch 25",   317250 },
    { "PAL 26",  "ch 26",   324250 },
    { "PAL 27",  "ch 27",   338250 },
    { "PAL 28",  "ch 28",   345250 },
    { "PAL 29",  "ch 29",   352250 },
    { "PAL 30",  "ch 30",   359250 },
    { "PAL 31",  "ch 31",   366250 },
    { "PAL 32",  "ch 32",   373250 },
    { "PAL 33",  "ch 33",   380250 },
    { "PAL 34",  "ch 34",   387250 },
    { "PAL 35",  "ch 35",   394250 },
    { "PAL 36",  "ch 36",   401250 },
    { "PAL 37",  "ch 37",   408250 },
    { "PAL 38",  "ch 38",   415250 },
    { "PAL 39",  "ch 39",   422250 },
    { "PAL 40",  "ch 40",   429250 },
    { "PAL 41",  "ch 41",   436250 },
    { "PAL 42",  "ch 42",   443250 },
    { "PAL 43",  "ch 43",   450250 },
    { "PAL 44",  "ch 44",   457250 },
    { "PAL 45",  "ch 45",   464250 },
    { "PAL 46",  "ch 46",   471250 },
    { "PAL 47",  "ch 47",   478250 },
    { "PAL 48",  "ch 48",   485250 },
    { "PAL 49",  "ch 49",   492250 },
    { "PAL 50",  "ch 50",   499250 },
    { "PAL 51",  "ch 51",   506250 },
    { "PAL 52",  "ch 52",   513250 },
    { "PAL 53",  "ch 53",   520250 },
    { "PAL 54",  "ch 54",   527250 },
    { "PAL 55",  "ch 55",   534250 },
};


/* europe west and east              */

/* CCIR frequencies */

#define FREQ_CCIR_I_III   \
    { "CCIR I k2",    "k2",    48250 }, \
    { "CCIR I k3",    "k3",    55250 }, \
    { "CCIR I k4",    "k4",    62250 }, \
    { "CCIR I S01",   "S01",   69250 }, \
    { "CCIR I S02",   "S02",   76250 }, \
    { "CCIR I S03",   "S03",   83250 }, \
    { "CCIR III k5",  "k5",   175250 }, \
    { "CCIR III k6",  "k6",   182250 }, \
    { "CCIR III k7",  "k7",   189250 }, \
    { "CCIR III k8",  "k8",   196250 }, \
    { "CCIR III k9",  "k9",   203250 }, \
    { "CCIR III k10", "k10",  210250 }, \
    { "CCIR III k11", "k11",  217250 }, \
    { "CCIR III k12", "k12",  224250 }


/*  CCIR SL/SH SE1 = 105.250MHz removed,
 *  since it overlaps with FM radio 87.5-108.0MHz
 */
#define FREQ_CCIR_SL_SH \
    { "CCIR SL/SH SE2",  "SE2",   112250 }, \
    { "CCIR SL/SH SE3",  "SE3",   119250 }, \
    { "CCIR SL/SH SE4",  "SE4",   126250 }, \
    { "CCIR SL/SH SE5",  "SE5",   133250 }, \
    { "CCIR SL/SH SE6",  "SE6",   140250 }, \
    { "CCIR SL/SH SE7",  "SE7",   147250 }, \
    { "CCIR SL/SH SE8",  "SE8",   154250 }, \
    { "CCIR SL/SH SE9",  "SE9",   161250 }, \
    { "CCIR SL/SH SE10", "SE10",  168250 }, \
    { "CCIR SL/SH SE11", "SE11",  231250 }, \
    { "CCIR SL/SH SE12", "SE12",  238250 }, \
    { "CCIR SL/SH SE13", "SE13",  245250 }, \
    { "CCIR SL/SH SE14", "SE14",  252250 }, \
    { "CCIR SL/SH SE15", "SE15",  259250 }, \
    { "CCIR SL/SH SE16", "SE16",  266250 }, \
    { "CCIR SL/SH SE17", "SE17",  273250 }, \
    { "CCIR SL/SH SE18", "SE18",  280250 }, \
    { "CCIR SL/SH SE19", "SE19",  287250 }, \
    { "CCIR SL/SH SE20", "SE20",  294250 }

#define FREQ_CCIR_H \
    { "CCIR H S21",  "S21",  303250 }, \
    { "CCIR H S22",  "S22",  311250 }, \
    { "CCIR H S23",  "S23",  319250 }, \
    { "CCIR H S24",  "S24",  327250 }, \
    { "CCIR H S25",  "S25",  335250 }, \
    { "CCIR H S26",  "S26",  343250 }, \
    { "CCIR H S27",  "S27",  351250 }, \
    { "CCIR H S28",  "S28",  359250 }, \
    { "CCIR H S29",  "S29",  367250 }, \
    { "CCIR H S30",  "S30",  375250 }, \
    { "CCIR H S31",  "S31",  383250 }, \
    { "CCIR H S32",  "S32",  391250 }, \
    { "CCIR H S33",  "S33",  399250 }, \
    { "CCIR H S34",  "S34",  407250 }, \
    { "CCIR H S35",  "S35",  415250 }, \
    { "CCIR H S36",  "S36",  423250 }, \
    { "CCIR H S37",  "S37",  431250 }, \
    { "CCIR H S38",  "S38",  439250 }, \
    { "CCIR H S39",  "S39",  447250 }, \
    { "CCIR H S40",  "S40",  455250 }, \
    { "CCIR H S41",  "S41",  463250 }
 
/* OIRT frequencies (europe east) */
#define FREQ_OIRT_I_III \
    { "OIRT I/III R1",  "R1",     49750 }, \
    { "OIRT I/III R2",  "R2",     59250 }, \
    { "OIRT I/III R3",  "R3",     77250 }, \
    { "OIRT I/III R4",  "R4",     85250 }, \
    { "OIRT I/III R5",  "R5",     93250 }, \
    { "OIRT I/III R6",  "R6",    175250 }, \
    { "OIRT I/III R7",  "R7",    183250 }, \
    { "OIRT I/III R8",  "R8",    191250 }, \
    { "OIRT I/III R9",  "R9",    199250 }, \
    { "OIRT I/III R10", "R10",   207250 }, \
    { "OIRT I/III R11", "R11",   215250 }, \
    { "OIRT I/III R12", "R12",   223250 }

#define FREQ_OIRT_SL_SH \
    { "OIRT SL/SH SR1",  "SR1",   111250 }, \
    { "OIRT SL/SH SR2",  "SR2",   119250 }, \
    { "OIRT SL/SH SR3",  "SR3",   127250 }, \
    { "OIRT SL/SH SR4",  "SR4",   135250 }, \
    { "OIRT SL/SH SR5",  "SR5",   143250 }, \
    { "OIRT SL/SH SR6",  "SR6",   151250 }, \
    { "OIRT SL/SH SR7",  "SR7",   159250 }, \
    { "OIRT SL/SH SR8",  "SR8",   167250 }, \
    { "OIRT SL/SH SR11", "SR11",  231250 }, \
    { "OIRT SL/SH SR12", "SR12",  239250 }, \
    { "OIRT SL/SH SR13", "SR13",  247250 }, \
    { "OIRT SL/SH SR14", "SR14",  255250 }, \
    { "OIRT SL/SH SR15", "SR15",  263250 }, \
    { "OIRT SL/SH SR16", "SR16",  271250 }, \
    { "OIRT SL/SH SR17", "SR17",  279250 }, \
    { "OIRT SL/SH SR18", "SR18",  287250 }, \
    { "OIRT SL/SH SR19", "SR19",  295250 }

/* uhf frequencies, east and west */
#define FREQ_UHF \
    { "UHF 21",  "k21",   471250 }, \
    { "UHF 22",  "k22",   479250 }, \
    { "UHF 23",  "k23",   487250 }, \
    { "UHF 24",  "k24",   495250 }, \
    { "UHF 25",  "k25",   503250 }, \
    { "UHF 26",  "k26",   511250 }, \
    { "UHF 27",  "k27",   519250 }, \
    { "UHF 28",  "k28",   527250 }, \
    { "UHF 29",  "k29",   535250 }, \
    { "UHF 30",  "k30",   543250 }, \
    { "UHF 31",  "k31",   551250 }, \
    { "UHF 32",  "k32",   559250 }, \
    { "UHF 33",  "k33",   567250 }, \
    { "UHF 34",  "k34",   575250 }, \
    { "UHF 35",  "k35",   583250 }, \
    { "UHF 36",  "k36",   591250 }, \
    { "UHF 37",  "k37",   599250 }, \
    { "UHF 38",  "k38",   607250 }, \
    { "UHF 39",  "k39",   615250 }, \
    { "UHF 40",  "k40",   623250 }, \
    { "UHF 41",  "k41",   631250 }, \
    { "UHF 42",  "k42",   639250 }, \
    { "UHF 43",  "k43",   647250 }, \
    { "UHF 44",  "k44",   655250 }, \
    { "UHF 45",  "k45",   663250 }, \
    { "UHF 46",  "k46",   671250 }, \
    { "UHF 47",  "k47",   679250 }, \
    { "UHF 48",  "k48",   687250 }, \
    { "UHF 49",  "k49",   695250 }, \
    { "UHF 50",  "k50",   703250 }, \
    { "UHF 51",  "k51",   711250 }, \
    { "UHF 52",  "k52",   719250 }, \
    { "UHF 53",  "k53",   727250 }, \
    { "UHF 54",  "k54",   735250 }, \
    { "UHF 55",  "k55",   743250 }, \
    { "UHF 56",  "k56",   751250 }, \
    { "UHF 57",  "k57",   759250 }, \
    { "UHF 58",  "k58",   767250 }, \
    { "UHF 59",  "k59",   775250 }, \
    { "UHF 60",  "k60",   783250 }, \
    { "UHF 61",  "k61",   791250 }, \
    { "UHF 62",  "k62",   799250 }, \
    { "UHF 63",  "k63",   807250 }, \
    { "UHF 64",  "k64",   815250 }, \
    { "UHF 65",  "k65",   823250 }, \
    { "UHF 66",  "k66",   831250 }, \
    { "UHF 67",  "k67",   839250 }, \
    { "UHF 68",  "k68",   847250 }, \
    { "UHF 69",  "k69",   855250 }
    
static struct cFreqlist europe_west[] = {
     FREQ_CCIR_I_III,
     FREQ_CCIR_SL_SH,
     FREQ_CCIR_H,
     FREQ_UHF
};
    
static struct cFreqlist europe_east[] = {
     FREQ_OIRT_I_III,
     FREQ_OIRT_SL_SH,
     FREQ_CCIR_I_III,
     FREQ_CCIR_SL_SH,
     FREQ_CCIR_H,
     FREQ_UHF
};

static struct cFreqlist pal_italy[] = {
    { "PAL A",   "A",     53750 },
    { "PAL B",   "B",     62250 },
    { "PAL C",   "C",     82250 },
    { "PAL D",   "D",    175250 },
    { "PAL E",   "E",    183750 },
    { "PAL F",   "F",    192250 },
    { "PAL G",   "G",    201250 },
    { "PAL H",   "H",    210250 },
    { "PAL H1",  "H1",   217250 },
    { "PAL H2",  "H2",   224250 },
    FREQ_UHF
};
    
static struct cFreqlist pal_ireland[] = {
    { "PAL A0",   "A0",    45750 },
    { "PAL A1",   "A1",    48000 },
    { "PAL A2",   "A2",    53750 },
    { "PAL A3",   "A3",    56000 },
    { "PAL A4",   "A4",    61750 },
    { "PAL A5",   "A5",    64000 },
    { "PAL A6",   "A6",   175250 },
    { "PAL A7",   "A7",   176000 },
    { "PAL A8",   "A8",   183250 },
    { "PAL A9",   "A9",   184000 },
    { "PAL A10",  "A10",  191250 },
    { "PAL A11",  "A11",  192000 },
    { "PAL A12",  "A12",  199250 },
    { "PAL A13",  "A13",  200000 },
    { "PAL A14",  "A14",  207250 },
    { "PAL A15",  "A15",  208000 },
    { "PAL A16",  "A16",  215250 },
    { "PAL A17",  "A17",  216000 },
    { "PAL A18",  "A18",  224000 },
    { "PAL A19",  "A19",  232000 },
    { "PAL A20",  "A20",  248000 },
    { "PAL A21",  "A21",  256000 },
    { "PAL A22",  "A22",  264000 },
    { "PAL A23",  "A23",  272000 },
    { "PAL A24",  "A24",  280000 },
    { "PAL A25",  "A25",  288000 },
    { "PAL A26",  "A26",  296000 },
    { "PAL A27",  "A27",  304000 },
    { "PAL A28",  "A28",  312000 },
    { "PAL A29",  "A29",  320000 },
    { "PAL A30",  "A30",  344000 },
    { "PAL A31",  "A31",  352000 },
    { "PAL A32",  "A32",  408000 },
    { "PAL A33",  "A33",  416000 },
    { "PAL A34",  "A34",  448000 },
    { "PAL A35",  "A35",  480000 },
    { "PAL A36",  "A36",  520000 },
    FREQ_UHF,
};

static struct cFreqlist secam_france[] = {
    { "SECAM K01",  "K01",   47750 },
    { "SECAM K02",  "K02",   55750 },
    { "SECAM K03",  "K03",   60500 },
    { "SECAM K04",  "K04",   63750 },
    { "SECAM K05",  "K05",  176000 },
    { "SECAM K06",  "K06",  184000 },
    { "SECAM K07",  "K07",  192000 },
    { "SECAM K08",  "K08",  200000 },
    { "SECAM K09",  "K09",  208000 },
    { "SECAM K10",  "K10",  216000 },
    { "SECAM KB",   "KB",   116750 },
    { "SECAM KC",   "KC",   128750 },
    { "SECAM KD",   "KD",   140750 },
    { "SECAM KE",   "KE",   159750 },
    { "SECAM KF",   "KF",   164750 },
    { "SECAM KG",   "KG",   176750 },
    { "SECAM KH",   "KH",   188750 },
    { "SECAM KI",   "KI",   200750 },
    { "SECAM KJ",   "KJ",   212750 },
    { "SECAM KK",   "KK",   224750 },
    { "SECAM KL",   "KL",   236750 },
    { "SECAM KM",   "KM",   248750 },
    { "SECAM KN",   "KN",   260750 },
    { "SECAM KO",   "KO",   272750 },
    { "SECAM KP",   "KP",   284750 },
    { "SECAM KQ",   "KQ",   296750 },
    { "SECAM H01",  "H01",  303250 },
    { "SECAM H02",  "H02",  311250 },
    { "SECAM H03",  "H03",  319250 },
    { "SECAM H04",  "H04",  327250 },
    { "SECAM H05",  "H05",  335250 },
    { "SECAM H06",  "H06",  343250 },
    { "SECAM H07",  "H07",  351250 },
    { "SECAM H08",  "H08",  359250 },
    { "SECAM H09",  "H09",  367250 },
    { "SECAM H10",  "H10",  375250 },
    { "SECAM H11",  "H11",  383250 },
    { "SECAM H12",  "H12",  391250 },
    { "SECAM H13",  "H13",  399250 },
    { "SECAM H14",  "H14",  407250 },
    { "SECAM H15",  "H15",  415250 },
    { "SECAM H16",  "H16",  423250 },
    { "SECAM H17",  "H17",  431250 },
    { "SECAM H18",  "H18",  439250 },
    { "SECAM H19",  "H19",  447250 },
    FREQ_UHF,
};


static struct cFreqlist pal_newzealand[] = {
    { "PAL 1",   "ch 1",     45250 }, 
    { "PAL 2",   "ch 2",     55250 }, 
    { "PAL 3",   "ch 3",     62250 },
    { "PAL 4",   "ch 4",    175250 },
    { "PAL 5",   "ch 5",    182250 },
    { "PAL 6",   "ch 6",    189250 },
    { "PAL 7",   "ch 7",    196250 },
    { "PAL 8",   "ch 8",    203250 },
    { "PAL 9",   "ch 9",    210250 },
    { "PAL 10",  "ch 10",   217250 },
    { "PAL 11",  "ch 11",   224250 },
    FREQ_UHF,
};

/* China PAL broadcast */
static struct cFreqlist pal_bcast_cn[] = {
    { "PAL 1",   "ch 1",     49750 },
    { "PAL 2",   "ch 2",     57750 },
    { "PAL 3",   "ch 3",     65750 },
    { "PAL 4",   "ch 4",     77250 },
    { "PAL 5",   "ch 5",     85250 },
    { "PAL 6",   "ch 6",    112250 },
    { "PAL 7",   "ch 7",    120250 },
    { "PAL 8",   "ch 8",    128250 },
    { "PAL 9",   "ch 9",    136250 },
    { "PAL 10",  "ch 10",   144250 },
    { "PAL 11",  "ch 11",   152250 },
    { "PAL 12",  "ch 12",   160250 },
    { "PAL 13",  "ch 13",   168250 },
    { "PAL 14",  "ch 14",   176250 },
    { "PAL 15",  "ch 15",   184250 },
    { "PAL 16",  "ch 16",   192250 },
    { "PAL 17",  "ch 17",   200250 },
    { "PAL 18",  "ch 18",   208250 },
    { "PAL 19",  "ch 19",   216250 },
    { "PAL 20",  "ch 20",   224250 },
    { "PAL 21",  "ch 21",   232250 },
    { "PAL 22",  "ch 22",   240250 },
    { "PAL 23",  "ch 23",   248250 },
    { "PAL 24",  "ch 24",   256250 },
    { "PAL 25",  "ch 25",   264250 },
    { "PAL 26",  "ch 26",   272250 },
    { "PAL 27",  "ch 27",   280250 },
    { "PAL 28",  "ch 28",   288250 },
    { "PAL 29",  "ch 29",   296250 },
    { "PAL 30",  "ch 30",   304250 },
    { "PAL 31",  "ch 31",   312250 },
    { "PAL 32",  "ch 32",   320250 },
    { "PAL 33",  "ch 33",   328250 },
    { "PAL 34",  "ch 34",   336250 },
    { "PAL 35",  "ch 35",   344250 },
    { "PAL 36",  "ch 36",   352250 },
    { "PAL 37",  "ch 37",   360250 },
    { "PAL 38",  "ch 38",   368250 },
    { "PAL 39",  "ch 39",   376250 },
    { "PAL 40",  "ch 40",   384250 },
    { "PAL 41",  "ch 41",   392250 },
    { "PAL 42",  "ch 42",   400250 },
    { "PAL 43",  "ch 43",   408250 },
    { "PAL 44",  "ch 44",   416250 },
    { "PAL 45",  "ch 45",   424250 },
    { "PAL 46",  "ch 46",   432250 },
    { "PAL 47",  "ch 47",   440250 },
    { "PAL 48",  "ch 48",   448250 },
    { "PAL 49",  "ch 49",   456250 },
    { "PAL 50",  "ch 50",   463250 },
    { "PAL 51",  "ch 51",   471250 },
    { "PAL 52",  "ch 52",   479250 },
    { "PAL 53",  "ch 53",   487250 },
    { "PAL 54",  "ch 54",   495250 },
    { "PAL 55",  "ch 55",   503250 },
    { "PAL 56",  "ch 56",   511250 },
    { "PAL 57",  "ch 57",   519250 },
    { "PAL 58",  "ch 58",   527250 },
    { "PAL 59",  "ch 59",   535250 },
    { "PAL 60",  "ch 60",   543250 },
    { "PAL 61",  "ch 61",   551250 },
    { "PAL 62",  "ch 62",   559250 },
    { "PAL 63",  "ch 63",   607250 },
    { "PAL 64",  "ch 64",   615250 },
    { "PAL 65",  "ch 65",   623250 },
    { "PAL 66",  "ch 66",   631250 },
    { "PAL 67",  "ch 67",   639250 },
    { "PAL 68",  "ch 68",   647250 },
    { "PAL 69",  "ch 69",   655250 },
    { "PAL 70",  "ch 70",   663250 },
    { "PAL 71",  "ch 71",   671250 },
    { "PAL 72",  "ch 72",   679250 },
    { "PAL 73",  "ch 73",   687250 },
    { "PAL 74",  "ch 74",   695250 },
    { "PAL 75",  "ch 75",   703250 },
    { "PAL 76",  "ch 76",   711250 },
    { "PAL 77",  "ch 77",   719250 },
    { "PAL 78",  "ch 78",   727250 },
    { "PAL 79",  "ch 79",   735250 },
    { "PAL 80",  "ch 80",   743250 },
    { "PAL 81",  "ch 81",   751250 },
    { "PAL 82",  "ch 82",   759250 },
    { "PAL 83",  "ch 83",   767250 },
    { "PAL 84",  "ch 84",   775250 },
    { "PAL 85",  "ch 85",   783250 },
    { "PAL 86",  "ch 86",   791250 },
    { "PAL 87",  "ch 87",   799250 },
    { "PAL 88",  "ch 88",   807250 },
    { "PAL 89",  "ch 89",   815250 },
    { "PAL 90",  "ch 90",   823250 },
    { "PAL 91",  "ch 91",   831250 },
    { "PAL 92",  "ch 92",   839250 },
    { "PAL 93",  "ch 93",   847250 },
    { "PAL 94",  "ch 94",   855250 },
};

/* South Africa PAL Broadcast */

static struct cFreqlist pal_bcast_za[] ={
    { "PAL 4",   "ch 4",    175250 },
    { "PAL 5",   "ch 5",    183250 },
    { "PAL 6",   "ch 6",    191250 },
    { "PAL 7",   "ch 7",    199250 },
    { "PAL 8",   "ch 8",    207250 },
    { "PAL 9",   "ch 9",    215250 },
    { "PAL 10",  "ch 10",   223250 },
    { "PAL 11",  "ch 11",   231250 },
    { "PAL 12",  "ch 12",   239250 },
    { "PAL 13",  "ch 13",   247250 },
    FREQ_UHF
};

static struct cFreqlist argentina[] = {
    { "PAL 001",  "ch 001",   56250 },
    { "PAL 002",  "ch 002",   62250 },
    { "PAL 003",  "ch 003",   68250 },
    { "PAL 004",  "ch 004",   78250 },
    { "PAL 005",  "ch 005",   84250 },
    { "PAL 006",  "ch 006",  176250 },
    { "PAL 007",  "ch 007",  182250 },
    { "PAL 008",  "ch 008",  188250 },
    { "PAL 009",  "ch 009",  194250 },
    { "PAL 010",  "ch 010",  200250 },
    { "PAL 011",  "ch 011",  206250 },
    { "PAL 012",  "ch 012",  212250 },
    { "PAL 013",  "ch 013",  122250 },
    { "PAL 014",  "ch 014",  128250 },
    { "PAL 015",  "ch 015",  134250 },
    { "PAL 016",  "ch 016",  140250 },
    { "PAL 017",  "ch 017",  146250 },
    { "PAL 018",  "ch 018",  152250 },
    { "PAL 019",  "ch 019",  158250 },
    { "PAL 020",  "ch 020",  164250 },
    { "PAL 021",  "ch 021",  170250 },
    { "PAL 022",  "ch 022",  218250 },
    { "PAL 023",  "ch 023",  224250 },
    { "PAL 024",  "ch 024",  230250 },
    { "PAL 025",  "ch 025",  236250 },
    { "PAL 026",  "ch 026",  242250 },
    { "PAL 027",  "ch 027",  248250 },
    { "PAL 028",  "ch 028",  254250 },
    { "PAL 029",  "ch 029",  260250 },
    { "PAL 030",  "ch 030",  266250 },
    { "PAL 031",  "ch 031",  272250 },
    { "PAL 032",  "ch 032",  278250 },
    { "PAL 033",  "ch 033",  284250 },
    { "PAL 034",  "ch 034",  290250 },
    { "PAL 035",  "ch 035",  296250 },
    { "PAL 036",  "ch 036",  302250 },
    { "PAL 037",  "ch 037",  308250 },
    { "PAL 038",  "ch 038",  314250 },
    { "PAL 039",  "ch 039",  320250 },
    { "PAL 040",  "ch 040",  326250 },
    { "PAL 041",  "ch 041",  332250 },
    { "PAL 042",  "ch 042",  338250 },
    { "PAL 043",  "ch 043",  344250 },
    { "PAL 044",  "ch 044",  350250 },
    { "PAL 045",  "ch 045",  356250 },
    { "PAL 046",  "ch 046",  362250 },
    { "PAL 047",  "ch 047",  368250 },
    { "PAL 048",  "ch 048",  374250 },
    { "PAL 049",  "ch 049",  380250 },
    { "PAL 050",  "ch 050",  386250 },
    { "PAL 051",  "ch 051",  392250 },
    { "PAL 052",  "ch 052",  398250 },
    { "PAL 053",  "ch 053",  404250 },
    { "PAL 054",  "ch 054",  410250 },
    { "PAL 055",  "ch 055",  416250 },
    { "PAL 056",  "ch 056",  422250 },
    { "PAL 057",  "ch 057",  428250 },
    { "PAL 058",  "ch 058",  434250 },
    { "PAL 059",  "ch 059",  440250 },
    { "PAL 060",  "ch 060",  446250 },
    { "PAL 061",  "ch 061",  452250 },
    { "PAL 062",  "ch 062",  458250 },
    { "PAL 063",  "ch 063",  464250 },
    { "PAL 064",  "ch 064",  470250 },
    { "PAL 065",  "ch 065",  476250 },
    { "PAL 066",  "ch 066",  482250 },
    { "PAL 067",  "ch 067",  488250 },
    { "PAL 068",  "ch 068",  494250 },
    { "PAL 069",  "ch 069",  500250 },
    { "PAL 070",  "ch 070",  506250 },
    { "PAL 071",  "ch 071",  512250 },
    { "PAL 072",  "ch 072",  518250 },
    { "PAL 073",  "ch 073",  524250 },
    { "PAL 074",  "ch 074",  530250 },
    { "PAL 075",  "ch 075",  536250 },
    { "PAL 076",  "ch 076",  542250 },
    { "PAL 077",  "ch 077",  548250 },
    { "PAL 078",  "ch 078",  554250 },
    { "PAL 079",  "ch 079",  560250 },
    { "PAL 080",  "ch 080",  566250 },
    { "PAL 081",  "ch 081",  572250 },
    { "PAL 082",  "ch 082",  578250 },
    { "PAL 083",  "ch 083",  584250 },
    { "PAL 084",  "ch 084",  590250 },
    { "PAL 085",  "ch 085",  596250 },
    { "PAL 086",  "ch 086",  602250 },
    { "PAL 087",  "ch 087",  608250 },
    { "PAL 088",  "ch 088",  614250 },
    { "PAL 089",  "ch 089",  620250 },
    { "PAL 090",  "ch 090",  626250 },
    { "PAL 091",  "ch 091",  632250 },
    { "PAL 092",  "ch 092",  638250 },
    { "PAL 093",  "ch 093",  644250 },
};

static struct cFreqlist fm_europe[] = {
/*    { "FM  87.5",   "87.5MHz",  87500  },
    { "FM  87.6",   "87.6MHz",  87625  },
    { "FM  87.8",   "87.8MHz",  87750  },
    { "FM  87.9",   "87.9MHz",  87875  },
    { "FM  88.0",   "88.0MHz",  88000  },
    { "FM  88.1",   "88.1MHz",  88125  },
    { "FM  88.3",   "88.3MHz",  88250  },
    { "FM  88.4",   "88.4MHz",  88375  },
    { "FM  88.5",   "88.5MHz",  88500  },
    { "FM  88.6",   "88.6MHz",  88625  },
    { "FM  88.8",   "88.8MHz",  88750  },
    { "FM  88.9",   "88.9MHz",  88875  },
    { "FM  89.0",   "89.0MHz",  89000  },
    { "FM  89.1",   "89.1MHz",  89125  },
    { "FM  89.3",   "89.3MHz",  89250  },
    { "FM  89.4",   "89.4MHz",  89375  },
    { "FM  89.5",   "89.5MHz",  89500  },
    { "FM  89.6",   "89.6MHz",  89625  },
    { "FM  89.8",   "89.8MHz",  89750  },
    { "FM  89.9",   "89.9MHz",  89875  },
    { "FM  90.0",   "90.0MHz",  90000  },
    { "FM  90.1",   "90.1MHz",  90125  },
    { "FM  90.3",   "90.3MHz",  90250  },
    { "FM  90.4",   "90.4MHz",  90375  },
    { "FM  90.5",   "90.5MHz",  90500  },
    { "FM  90.6",   "90.6MHz",  90625  },
    { "FM  90.8",   "90.8MHz",  90750  },
    { "FM  90.9",   "90.9MHz",  90875  },
    { "FM  91.0",   "91.0MHz",  91000  },
    { "FM  91.1",   "91.1MHz",  91125  },
    { "FM  91.3",   "91.3MHz",  91250  },
    { "FM  91.4",   "91.4MHz",  91375  },
    { "FM  91.5",   "91.5MHz",  91500  },
    { "FM  91.6",   "91.6MHz",  91625  },
    { "FM  91.8",   "91.8MHz",  91750  },
    { "FM  91.9",   "91.9MHz",  91875  },
    { "FM  92.0",   "92.0MHz",  92000  },
    { "FM  92.1",   "92.1MHz",  92125  },
    { "FM  92.3",   "92.3MHz",  92250  },
    { "FM  92.4",   "92.4MHz",  92375  },
    { "FM  92.5",   "92.5MHz",  92500  },
    { "FM  92.6",   "92.6MHz",  92625  },
    { "FM  92.8",   "92.8MHz",  92750  },
    { "FM  92.9",   "92.9MHz",  92875  },
    { "FM  93.0",   "93.0MHz",  93000  },
    { "FM  93.1",   "93.1MHz",  93125  },
    { "FM  93.3",   "93.3MHz",  93250  },
    { "FM  93.4",   "93.4MHz",  93375  },
    { "FM  93.5",   "93.5MHz",  93500  },
    { "FM  93.6",   "93.6MHz",  93625  },
    { "FM  93.8",   "93.8MHz",  93750  },
    { "FM  93.9",   "93.9MHz",  93875  },
    { "FM  94.0",   "94.0MHz",  94000  },
    { "FM  94.1",   "94.1MHz",  94125  },
    { "FM  94.3",   "94.3MHz",  94250  },
    { "FM  94.4",   "94.4MHz",  94375  },
    { "FM  94.5",   "94.5MHz",  94500  },
    { "FM  94.6",   "94.6MHz",  94625  },
    { "FM  94.8",   "94.8MHz",  94750  },
    { "FM  94.9",   "94.9MHz",  94875  },
    { "FM  95.0",   "95.0MHz",  95000  },
    { "FM  95.1",   "95.1MHz",  95125  },
    { "FM  95.3",   "95.3MHz",  95250  },
    { "FM  95.4",   "95.4MHz",  95375  },
    { "FM  95.5",   "95.5MHz",  95500  },
    { "FM  95.6",   "95.6MHz",  95625  },
    { "FM  95.8",   "95.8MHz",  95750  },
    { "FM  95.9",   "95.9MHz",  95875  },
    { "FM  96.0",   "96.0MHz",  96000  },
    { "FM  96.1",   "96.1MHz",  96125  },
    { "FM  96.3",   "96.3MHz",  96250  },
    { "FM  96.4",   "96.4MHz",  96375  },
    { "FM  96.5",   "96.5MHz",  96500  },
    { "FM  96.6",   "96.6MHz",  96625  },
    { "FM  96.8",   "96.8MHz",  96750  },
    { "FM  96.9",   "96.9MHz",  96875  },
    { "FM  97.0",   "97.0MHz",  97000  },
    { "FM  97.1",   "97.1MHz",  97125  },
    { "FM  97.3",   "97.3MHz",  97250  },
    { "FM  97.4",   "97.4MHz",  97375  },
    { "FM  97.5",   "97.5MHz",  97500  },
    { "FM  97.6",   "97.6MHz",  97625  },
    { "FM  97.8",   "97.8MHz",  97750  },
    { "FM  97.9",   "97.9MHz",  97875  },
    { "FM  98.0",   "98.0MHz",  98000  },
    { "FM  98.1",   "98.1MHz",  98125  },
    { "FM  98.3",   "98.3MHz",  98250  },
    { "FM  98.4",   "98.4MHz",  98375  },
    { "FM  98.5",   "98.5MHz",  98500  },
    { "FM  98.6",   "98.6MHz",  98625  },
    { "FM  98.8",   "98.8MHz",  98750  },
    { "FM  98.9",   "98.9MHz",  98875  },
    { "FM  99.0",   "99.0MHz",  99000  },
    { "FM  99.1",   "99.1MHz",  99125  },
    { "FM  99.3",   "99.3MHz",  99250  },
    { "FM  99.4",   "99.4MHz",  99375  },
    { "FM  99.5",   "99.5MHz",  99500  },
    { "FM  99.6",   "99.6MHz",  99625  },
    { "FM  99.8",   "99.8MHz",  99750  },
    { "FM  99.9",   "99.9MHz",  99875  },
    { "FM 100.0",  "100.0MHz", 100000  },
    { "FM 100.1",  "100.1MHz", 100125  },
    { "FM 100.3",  "100.3MHz", 100250  },
    { "FM 100.4",  "100.4MHz", 100375  },
    { "FM 100.5",  "100.5MHz", 100500  },
    { "FM 100.6",  "100.6MHz", 100625  },
    { "FM 100.8",  "100.8MHz", 100750  },
    { "FM 100.9",  "100.9MHz", 100875  },
    { "FM 101.0",  "101.0MHz", 101000  },
    { "FM 101.1",  "101.1MHz", 101125  },
    { "FM 101.3",  "101.3MHz", 101250  },
    { "FM 101.4",  "101.4MHz", 101375  },
    { "FM 101.5",  "101.5MHz", 101500  },
    { "FM 101.6",  "101.6MHz", 101625  },
    { "FM 101.8",  "101.8MHz", 101750  },
    { "FM 101.9",  "101.9MHz", 101875  },
    { "FM 102.0",  "102.0MHz", 102000  },
    { "FM 102.1",  "102.1MHz", 102125  },
    { "FM 102.3",  "102.3MHz", 102250  },
    { "FM 102.4",  "102.4MHz", 102375  },
    { "FM 102.5",  "102.5MHz", 102500  },
    { "FM 102.6",  "102.6MHz", 102625  },
    { "FM 102.8",  "102.8MHz", 102750  },
    { "FM 102.9",  "102.9MHz", 102875  },
    { "FM 103.0",  "103.0MHz", 103000  },
    { "FM 103.1",  "103.1MHz", 103125  },
    { "FM 103.3",  "103.3MHz", 103250  },
    { "FM 103.4",  "103.4MHz", 103375  },
    { "FM 103.5",  "103.5MHz", 103500  },
    { "FM 103.6",  "103.6MHz", 103625  },
    { "FM 103.8",  "103.8MHz", 103750  },
    { "FM 103.9",  "103.9MHz", 103875  },
    { "FM 104.0",  "104.0MHz", 104000  },
    { "FM 104.1",  "104.1MHz", 104125  },
    { "FM 104.3",  "104.3MHz", 104250  },
    { "FM 104.4",  "104.4MHz", 104375  },
    { "FM 104.5",  "104.5MHz", 104500  },
    { "FM 104.6",  "104.6MHz", 104625  },
    { "FM 104.8",  "104.8MHz", 104750  },
    { "FM 104.9",  "104.9MHz", 104875  },
    { "FM 105.0",  "105.0MHz", 105000  },
    { "FM 105.1",  "105.1MHz", 105125  },
    { "FM 105.3",  "105.3MHz", 105250  },
    { "FM 105.4",  "105.4MHz", 105375  },
    { "FM 105.5",  "105.5MHz", 105500  },
    { "FM 105.6",  "105.6MHz", 105625  },
    { "FM 105.8",  "105.8MHz", 105750  },
    { "FM 105.9",  "105.9MHz", 105875  },
    { "FM 106.0",  "106.0MHz", 106000  },
    { "FM 106.1",  "106.1MHz", 106125  },
    { "FM 106.3",  "106.3MHz", 106250  },
    { "FM 106.4",  "106.4MHz", 106375  },
    { "FM 106.5",  "106.5MHz", 106500  },
    { "FM 106.6",  "106.6MHz", 106625  },
    { "FM 106.8",  "106.8MHz", 106750  },
    { "FM 106.9",  "106.9MHz", 106875  },
    { "FM 107.0",  "107.0MHz", 107000  },
    { "FM 107.1",  "107.1MHz", 107125  },
    { "FM 107.3",  "107.3MHz", 107250  },
    { "FM 107.4",  "107.4MHz", 107375  },
    { "FM 107.5",  "107.5MHz", 107500  },
    { "FM 107.6",  "107.6MHz", 107625  },
    { "FM 107.8",  "107.8MHz", 107750  },
    { "FM 107.9",  "107.9MHz", 107875  },
    { "FM 108.0",  "108.0MHz", 108000  },*/

    { "FM 87.5",   " 87.5MHz",   87500 },
    { "FM 87.6",   " 87.6MHz",   87563 },
    { "FM 87.6",   " 87.6MHz",   87625 },
    { "FM 87.7",   " 87.7MHz",   87688 },
    { "FM 87.8",   " 87.8MHz",   87750 },
    { "FM 87.8",   " 87.8MHz",   87813 },
    { "FM 87.9",   " 87.9MHz",   87875 },
    { "FM 87.9",   " 87.9MHz",   87938 },
    { "FM 88.0",   " 88.0MHz",   88000 },
    { "FM 88.1",   " 88.1MHz",   88063 },
    { "FM 88.1",   " 88.1MHz",   88125 },
    { "FM 88.2",   " 88.2MHz",   88188 },
    { "FM 88.3",   " 88.3MHz",   88250 },
    { "FM 88.3",   " 88.3MHz",   88313 },
    { "FM 88.4",   " 88.4MHz",   88375 },
    { "FM 88.4",   " 88.4MHz",   88438 },
    { "FM 88.5",   " 88.5MHz",   88500 },
    { "FM 88.6",   " 88.6MHz",   88563 },
    { "FM 88.6",   " 88.6MHz",   88625 },
    { "FM 88.7",   " 88.7MHz",   88688 },
    { "FM 88.8",   " 88.8MHz",   88750 },
    { "FM 88.8",   " 88.8MHz",   88813 },
    { "FM 88.9",   " 88.9MHz",   88875 },
    { "FM 88.9",   " 88.9MHz",   88938 },
    { "FM 89.0",   " 89.0MHz",   89000 },
    { "FM 89.1",   " 89.1MHz",   89063 },
    { "FM 89.1",   " 89.1MHz",   89125 },
    { "FM 89.2",   " 89.2MHz",   89188 },
    { "FM 89.3",   " 89.3MHz",   89250 },
    { "FM 89.3",   " 89.3MHz",   89313 },
    { "FM 89.4",   " 89.4MHz",   89375 },
    { "FM 89.4",   " 89.4MHz",   89438 },
    { "FM 89.5",   " 89.5MHz",   89500 },
    { "FM 89.6",   " 89.6MHz",   89563 },
    { "FM 89.6",   " 89.6MHz",   89625 },
    { "FM 89.7",   " 89.7MHz",   89688 },
    { "FM 89.8",   " 89.8MHz",   89750 },
    { "FM 89.8",   " 89.8MHz",   89813 },
    { "FM 89.9",   " 89.9MHz",   89875 },
    { "FM 89.9",   " 89.9MHz",   89938 },
    { "FM 90.0",   " 90.0MHz",   90000 },
    { "FM 90.1",   " 90.1MHz",   90063 },
    { "FM 90.1",   " 90.1MHz",   90125 },
    { "FM 90.2",   " 90.2MHz",   90188 },
    { "FM 90.3",   " 90.3MHz",   90250 },
    { "FM 90.3",   " 90.3MHz",   90313 },
    { "FM 90.4",   " 90.4MHz",   90375 },
    { "FM 90.4",   " 90.4MHz",   90438 },
    { "FM 90.5",   " 90.5MHz",   90500 },
    { "FM 90.6",   " 90.6MHz",   90563 },
    { "FM 90.6",   " 90.6MHz",   90625 },
    { "FM 90.7",   " 90.7MHz",   90688 },
    { "FM 90.8",   " 90.8MHz",   90750 },
    { "FM 90.8",   " 90.8MHz",   90813 },
    { "FM 90.9",   " 90.9MHz",   90875 },
    { "FM 90.9",   " 90.9MHz",   90938 },
    { "FM 91.0",   " 91.0MHz",   91000 },
    { "FM 91.1",   " 91.1MHz",   91063 },
    { "FM 91.1",   " 91.1MHz",   91125 },
    { "FM 91.2",   " 91.2MHz",   91188 },
    { "FM 91.3",   " 91.3MHz",   91250 },
    { "FM 91.3",   " 91.3MHz",   91313 },
    { "FM 91.4",   " 91.4MHz",   91375 },
    { "FM 91.4",   " 91.4MHz",   91438 },
    { "FM 91.5",   " 91.5MHz",   91500 },
    { "FM 91.6",   " 91.6MHz",   91563 },
    { "FM 91.6",   " 91.6MHz",   91625 },
    { "FM 91.7",   " 91.7MHz",   91688 },
    { "FM 91.8",   " 91.8MHz",   91750 },
    { "FM 91.8",   " 91.8MHz",   91813 },
    { "FM 91.9",   " 91.9MHz",   91875 },
    { "FM 91.9",   " 91.9MHz",   91938 },
    { "FM 92.0",   " 92.0MHz",   92000 },
    { "FM 92.1",   " 92.1MHz",   92063 },
    { "FM 92.1",   " 92.1MHz",   92125 },
    { "FM 92.2",   " 92.2MHz",   92188 },
    { "FM 92.3",   " 92.3MHz",   92250 },
    { "FM 92.3",   " 92.3MHz",   92313 },
    { "FM 92.4",   " 92.4MHz",   92375 },
    { "FM 92.4",   " 92.4MHz",   92438 },
    { "FM 92.5",   " 92.5MHz",   92500 },
    { "FM 92.6",   " 92.6MHz",   92563 },
    { "FM 92.6",   " 92.6MHz",   92625 },
    { "FM 92.7",   " 92.7MHz",   92688 },
    { "FM 92.8",   " 92.8MHz",   92750 },
    { "FM 92.8",   " 92.8MHz",   92813 },
    { "FM 92.9",   " 92.9MHz",   92875 },
    { "FM 92.9",   " 92.9MHz",   92938 },
    { "FM 93.0",   " 93.0MHz",   93000 },
    { "FM 93.1",   " 93.1MHz",   93063 },
    { "FM 93.1",   " 93.1MHz",   93125 },
    { "FM 93.2",   " 93.2MHz",   93188 },
    { "FM 93.3",   " 93.3MHz",   93250 },
    { "FM 93.3",   " 93.3MHz",   93313 },
    { "FM 93.4",   " 93.4MHz",   93375 },
    { "FM 93.4",   " 93.4MHz",   93438 },
    { "FM 93.5",   " 93.5MHz",   93500 },
    { "FM 93.6",   " 93.6MHz",   93563 },
    { "FM 93.6",   " 93.6MHz",   93625 },
    { "FM 93.7",   " 93.7MHz",   93688 },
    { "FM 93.8",   " 93.8MHz",   93750 },
    { "FM 93.8",   " 93.8MHz",   93813 },
    { "FM 93.9",   " 93.9MHz",   93875 },
    { "FM 93.9",   " 93.9MHz",   93938 },
    { "FM 94.0",   " 94.0MHz",   94000 },
    { "FM 94.1",   " 94.1MHz",   94063 },
    { "FM 94.1",   " 94.1MHz",   94125 },
    { "FM 94.2",   " 94.2MHz",   94188 },
    { "FM 94.3",   " 94.3MHz",   94250 },
    { "FM 94.3",   " 94.3MHz",   94313 },
    { "FM 94.4",   " 94.4MHz",   94375 },
    { "FM 94.4",   " 94.4MHz",   94438 },
    { "FM 94.5",   " 94.5MHz",   94500 },
    { "FM 94.6",   " 94.6MHz",   94563 },
    { "FM 94.6",   " 94.6MHz",   94625 },
    { "FM 94.7",   " 94.7MHz",   94688 },
    { "FM 94.8",   " 94.8MHz",   94750 },
    { "FM 94.8",   " 94.8MHz",   94813 },
    { "FM 94.9",   " 94.9MHz",   94875 },
    { "FM 94.9",   " 94.9MHz",   94938 },
    { "FM 95.0",   " 95.0MHz",   95000 },
    { "FM 95.1",   " 95.1MHz",   95063 },
    { "FM 95.1",   " 95.1MHz",   95125 },
    { "FM 95.2",   " 95.2MHz",   95188 },
    { "FM 95.3",   " 95.3MHz",   95250 },
    { "FM 95.3",   " 95.3MHz",   95313 },
    { "FM 95.4",   " 95.4MHz",   95375 },
    { "FM 95.4",   " 95.4MHz",   95438 },
    { "FM 95.5",   " 95.5MHz",   95500 },
    { "FM 95.6",   " 95.6MHz",   95563 },
    { "FM 95.6",   " 95.6MHz",   95625 },
    { "FM 95.7",   " 95.7MHz",   95688 },
    { "FM 95.8",   " 95.8MHz",   95750 },
    { "FM 95.8",   " 95.8MHz",   95813 },
    { "FM 95.9",   " 95.9MHz",   95875 },
    { "FM 95.9",   " 95.9MHz",   95938 },
    { "FM 96.0",   " 96.0MHz",   96000 },
    { "FM 96.1",   " 96.1MHz",   96063 },
    { "FM 96.1",   " 96.1MHz",   96125 },
    { "FM 96.2",   " 96.2MHz",   96188 },
    { "FM 96.3",   " 96.3MHz",   96250 },
    { "FM 96.3",   " 96.3MHz",   96313 },
    { "FM 96.4",   " 96.4MHz",   96375 },
    { "FM 96.4",   " 96.4MHz",   96438 },
    { "FM 96.5",   " 96.5MHz",   96500 },
    { "FM 96.6",   " 96.6MHz",   96563 },
    { "FM 96.6",   " 96.6MHz",   96625 },
    { "FM 96.7",   " 96.7MHz",   96688 },
    { "FM 96.8",   " 96.8MHz",   96750 },
    { "FM 96.8",   " 96.8MHz",   96813 },
    { "FM 96.9",   " 96.9MHz",   96875 },
    { "FM 96.9",   " 96.9MHz",   96938 },
    { "FM 97.0",   " 97.0MHz",   97000 },
    { "FM 97.1",   " 97.1MHz",   97063 },
    { "FM 97.1",   " 97.1MHz",   97125 },
    { "FM 97.2",   " 97.2MHz",   97188 },
    { "FM 97.3",   " 97.3MHz",   97250 },
    { "FM 97.3",   " 97.3MHz",   97313 },
    { "FM 97.4",   " 97.4MHz",   97375 },
    { "FM 97.4",   " 97.4MHz",   97438 },
    { "FM 97.5",   " 97.5MHz",   97500 },
    { "FM 97.6",   " 97.6MHz",   97563 },
    { "FM 97.6",   " 97.6MHz",   97625 },
    { "FM 97.7",   " 97.7MHz",   97688 },
    { "FM 97.8",   " 97.8MHz",   97750 },
    { "FM 97.8",   " 97.8MHz",   97813 },
    { "FM 97.9",   " 97.9MHz",   97875 },
    { "FM 97.9",   " 97.9MHz",   97938 },
    { "FM 98.0",   " 98.0MHz",   98000 },
    { "FM 98.1",   " 98.1MHz",   98063 },
    { "FM 98.1",   " 98.1MHz",   98125 },
    { "FM 98.2",   " 98.2MHz",   98188 },
    { "FM 98.3",   " 98.3MHz",   98250 },
    { "FM 98.3",   " 98.3MHz",   98313 },
    { "FM 98.4",   " 98.4MHz",   98375 },
    { "FM 98.4",   " 98.4MHz",   98438 },
    { "FM 98.5",   " 98.5MHz",   98500 },
    { "FM 98.6",   " 98.6MHz",   98563 },
    { "FM 98.6",   " 98.6MHz",   98625 },
    { "FM 98.7",   " 98.7MHz",   98688 },
    { "FM 98.8",   " 98.8MHz",   98750 },
    { "FM 98.8",   " 98.8MHz",   98813 },
    { "FM 98.9",   " 98.9MHz",   98875 },
    { "FM 98.9",   " 98.9MHz",   98938 },
    { "FM 99.0",   " 99.0MHz",   99000 },
    { "FM 99.1",   " 99.1MHz",   99063 },
    { "FM 99.1",   " 99.1MHz",   99125 },
    { "FM 99.2",   " 99.2MHz",   99188 },
    { "FM 99.3",   " 99.3MHz",   99250 },
    { "FM 99.3",   " 99.3MHz",   99313 },
    { "FM 99.4",   " 99.4MHz",   99375 },
    { "FM 99.4",   " 99.4MHz",   99438 },
    { "FM 99.5",   " 99.5MHz",   99500 },
    { "FM 99.6",   " 99.6MHz",   99563 },
    { "FM 99.6",   " 99.6MHz",   99625 },
    { "FM 99.7",   " 99.7MHz",   99688 },
    { "FM 99.8",   " 99.8MHz",   99750 },
    { "FM 99.8",   " 99.8MHz",   99813 },
    { "FM 99.9",   " 99.9MHz",   99875 },
    { "FM 99.9",   " 99.9MHz",   99938 },
    { "FM 100.0",  "100.0MHz",  100000 },
    { "FM 100.1",  "100.1MHz",  100063 },
    { "FM 100.1",  "100.1MHz",  100125 },
    { "FM 100.2",  "100.2MHz",  100188 },
    { "FM 100.3",  "100.3MHz",  100250 },
    { "FM 100.3",  "100.3MHz",  100313 },
    { "FM 100.4",  "100.4MHz",  100375 },
    { "FM 100.4",  "100.4MHz",  100438 },
    { "FM 100.5",  "100.5MHz",  100500 },
    { "FM 100.6",  "100.6MHz",  100563 },
    { "FM 100.6",  "100.6MHz",  100625 },
    { "FM 100.7",  "100.7MHz",  100688 },
    { "FM 100.8",  "100.8MHz",  100750 },
    { "FM 100.8",  "100.8MHz",  100813 },
    { "FM 100.9",  "100.9MHz",  100875 },
    { "FM 100.9",  "100.9MHz",  100938 },
    { "FM 101.0",  "101.0MHz",  101000 },
    { "FM 101.1",  "101.1MHz",  101063 },
    { "FM 101.1",  "101.1MHz",  101125 },
    { "FM 101.2",  "101.2MHz",  101188 },
    { "FM 101.3",  "101.3MHz",  101250 },
    { "FM 101.3",  "101.3MHz",  101313 },
    { "FM 101.4",  "101.4MHz",  101375 },
    { "FM 101.4",  "101.4MHz",  101438 },
    { "FM 101.5",  "101.5MHz",  101500 },
    { "FM 101.6",  "101.6MHz",  101563 },
    { "FM 101.6",  "101.6MHz",  101625 },
    { "FM 101.7",  "101.7MHz",  101688 },
    { "FM 101.8",  "101.8MHz",  101750 },
    { "FM 101.8",  "101.8MHz",  101813 },
    { "FM 101.9",  "101.9MHz",  101875 },
    { "FM 101.9",  "101.9MHz",  101938 },
    { "FM 102.0",  "102.0MHz",  102000 },
    { "FM 102.1",  "102.1MHz",  102063 },
    { "FM 102.1",  "102.1MHz",  102125 },
    { "FM 102.2",  "102.2MHz",  102188 },
    { "FM 102.3",  "102.3MHz",  102250 },
    { "FM 102.3",  "102.3MHz",  102313 },
    { "FM 102.4",  "102.4MHz",  102375 },
    { "FM 102.4",  "102.4MHz",  102438 },
    { "FM 102.5",  "102.5MHz",  102500 },
    { "FM 102.6",  "102.6MHz",  102563 },
    { "FM 102.6",  "102.6MHz",  102625 },
    { "FM 102.7",  "102.7MHz",  102688 },
    { "FM 102.8",  "102.8MHz",  102750 },
    { "FM 102.8",  "102.8MHz",  102813 },
    { "FM 102.9",  "102.9MHz",  102875 },
    { "FM 102.9",  "102.9MHz",  102938 },
    { "FM 103.0",  "103.0MHz",  103000 },
    { "FM 103.1",  "103.1MHz",  103063 },
    { "FM 103.1",  "103.1MHz",  103125 },
    { "FM 103.2",  "103.2MHz",  103188 },
    { "FM 103.3",  "103.3MHz",  103250 },
    { "FM 103.3",  "103.3MHz",  103313 },
    { "FM 103.4",  "103.4MHz",  103375 },
    { "FM 103.4",  "103.4MHz",  103438 },
    { "FM 103.5",  "103.5MHz",  103500 },
    { "FM 103.6",  "103.6MHz",  103563 },
    { "FM 103.6",  "103.6MHz",  103625 },
    { "FM 103.7",  "103.7MHz",  103688 },
    { "FM 103.8",  "103.8MHz",  103750 },
    { "FM 103.8",  "103.8MHz",  103813 },
    { "FM 103.9",  "103.9MHz",  103875 },
    { "FM 103.9",  "103.9MHz",  103938 },
    { "FM 104.0",  "104.0MHz",  104000 },
    { "FM 104.1",  "104.1MHz",  104063 },
    { "FM 104.1",  "104.1MHz",  104125 },
    { "FM 104.2",  "104.2MHz",  104188 },
    { "FM 104.3",  "104.3MHz",  104250 },
    { "FM 104.3",  "104.3MHz",  104313 },
    { "FM 104.4",  "104.4MHz",  104375 },
    { "FM 104.4",  "104.4MHz",  104438 },
    { "FM 104.5",  "104.5MHz",  104500 },
    { "FM 104.6",  "104.6MHz",  104563 },
    { "FM 104.6",  "104.6MHz",  104625 },
    { "FM 104.7",  "104.7MHz",  104688 },
    { "FM 104.8",  "104.8MHz",  104750 },
    { "FM 104.8",  "104.8MHz",  104813 },
    { "FM 104.9",  "104.9MHz",  104875 },
    { "FM 104.9",  "104.9MHz",  104938 },
    { "FM 105.0",  "105.0MHz",  105000 },
    { "FM 105.1",  "105.1MHz",  105063 },
    { "FM 105.1",  "105.1MHz",  105125 },
    { "FM 105.2",  "105.2MHz",  105188 },
    { "FM 105.3",  "105.3MHz",  105250 },
    { "FM 105.3",  "105.3MHz",  105313 },
    { "FM 105.4",  "105.4MHz",  105375 },
    { "FM 105.4",  "105.4MHz",  105438 },
    { "FM 105.5",  "105.5MHz",  105500 },
    { "FM 105.6",  "105.6MHz",  105563 },
    { "FM 105.6",  "105.6MHz",  105625 },
    { "FM 105.7",  "105.7MHz",  105688 },
    { "FM 105.8",  "105.8MHz",  105750 },
    { "FM 105.8",  "105.8MHz",  105813 },
    { "FM 105.9",  "105.9MHz",  105875 },
    { "FM 105.9",  "105.9MHz",  105938 },
    { "FM 106.0",  "106.0MHz",  106000 },
    { "FM 106.1",  "106.1MHz",  106063 },
    { "FM 106.1",  "106.1MHz",  106125 },
    { "FM 106.2",  "106.2MHz",  106188 },
    { "FM 106.3",  "106.3MHz",  106250 },
    { "FM 106.3",  "106.3MHz",  106313 },
    { "FM 106.4",  "106.4MHz",  106375 },
    { "FM 106.4",  "106.4MHz",  106438 },
    { "FM 106.5",  "106.5MHz",  106500 },
    { "FM 106.6",  "106.6MHz",  106563 },
    { "FM 106.6",  "106.6MHz",  106625 },
    { "FM 106.7",  "106.7MHz",  106688 },
    { "FM 106.8",  "106.8MHz",  106750 },
    { "FM 106.8",  "106.8MHz",  106813 },
    { "FM 106.9",  "106.9MHz",  106875 },
    { "FM 106.9",  "106.9MHz",  106938 },
    { "FM 107.0",  "107.0MHz",  107000 },
    { "FM 107.1",  "107.1MHz",  107063 },
    { "FM 107.1",  "107.1MHz",  107125 },
    { "FM 107.2",  "107.2MHz",  107188 },
    { "FM 107.3",  "107.3MHz",  107250 },
    { "FM 107.3",  "107.3MHz",  107313 },
    { "FM 107.4",  "107.4MHz",  107375 },
    { "FM 107.4",  "107.4MHz",  107438 },
    { "FM 107.5",  "107.5MHz",  107500 },
    { "FM 107.6",  "107.6MHz",  107563 },
    { "FM 107.6",  "107.6MHz",  107625 },
    { "FM 107.7",  "107.7MHz",  107688 },
    { "FM 107.8",  "107.8MHz",  107750 },
    { "FM 107.8",  "107.8MHz",  107813 },
    { "FM 107.9",  "107.9MHz",  107875 },
    { "FM 107.9",  "107.9MHz",  107938 },
    { "FM 108.0",  "108.0MHz",  108000 },
};


const struct cFreqlists freqlists[] = {
    { "ntsc",            ntsc,                FREQ_COUNT(ntsc)                },
    { "ntsc-japan",      ntsc_japan,          FREQ_COUNT(ntsc_japan)          },  
    { "europe-west",     europe_west,         FREQ_COUNT(europe_west)         },
    { "europe-east",     europe_east,         FREQ_COUNT(europe_east)         },
    { "italy",           pal_italy,           FREQ_COUNT(pal_italy)           },
    { "newzealand",      pal_newzealand,      FREQ_COUNT(pal_newzealand)      },
    { "australia",       pal_australia,       FREQ_COUNT(pal_australia)       },
    { "ireland",         pal_ireland,         FREQ_COUNT(pal_ireland)         },
    { "france",          secam_france,        FREQ_COUNT(secam_france)        },
    { "china-bcast",     pal_bcast_cn,        FREQ_COUNT(pal_bcast_cn)        },
    { "southafrica",     pal_bcast_za,        FREQ_COUNT(pal_bcast_za)        },
    { "argentina",       argentina,           FREQ_COUNT(argentina)           },
    { "australia-optus", pal_australia_optus, FREQ_COUNT(pal_australia_optus) },
    { "fm_europe",       fm_europe,           FREQ_COUNT(fm_europe)           },
    { NULL,              NULL,                0                               } /* EOF */
};

#define    FREQLIST_NTSC                 0
#define    FREQLIST_NTSC_JAPAN           1
#define    FREQLIST_PAL_EUROPE_WEST      2
#define    FREQLIST_PAL_EUROPE_EAST      3
#define    FREQLIST_PAL_ITALY            4
#define    FREQLIST_PAL_NEWZEALAND       5
#define    FREQLIST_PAL_AUSTRALIA        6
#define    FREQLIST_PAL_IRELAND          7
#define    FREQLIST_SECAM_FRANCE         8
#define    FREQLIST_PAL_CHINA            9
#define    FREQLIST_PAL_SOUTHAFRIKA     10
#define    FREQLIST_PAL_ARGENTINIA      11
#define    FREQLIST_PAL_AUSTRALIA_OPUS  12
#define    FREQLIST_FM_EUROPE           13

int choose_country_analog_fm (const char * country, int * channellist) {
        if (strcasecmp(country_to_short_name(txt_to_country(country)), country)) {
            dlog(0,"\n\nCOUNTRY CODE IS NOT DEFINED. FALLING BACK TO \"DE\"\n");
            }
            // default pal europe west
            *channellist = FREQLIST_FM_EUROPE;

        dlog(1, "using settings for %s", country_to_full_name(txt_to_country(country)));

        /*
         * choose channellist name
         */
        switch(txt_to_country(country)) {
                   case AL:      //ALBANIA
                   case DZ:      //ALGERIA
                   case AO:      //ANGOLA
                   case AE:      //UNITED ARAB EMIRATES
                   case AT:      //AUSTRIA
                   case BH:      //BAHRAIN
                   case BD:      //BANGLADESH
                   case BE:      //BELGIUM
                   case BA:      //BOSNIA AND HERZEGOVINA
                   case BW:      //BOTSWANA
                   case BR:      //BRAZIL
                   case BN:      //BRUNEI DARUSSALAM
                   case KH:      //CAMBODIA
                   case CM:      //CAMEROON
                   case HR:      //CROATIA
                   case CY:      //CYPRUS
                   case DK:      //DENMARK
                   case EG:      //EGYPT
                   case GQ:      //EQUATORIAL GUINEA
                   case ET:      //ETHIOPIA
                   case FI:      //FINLAND
                   case GM:      //GAMBIA
                   case DE:      //GERMANY
                   case GH:      //GHANA
                   case GI:      //GIBRALTAR
                   case GL:      //GREENLAND
                   case GW:      //GUINEA-BISSAU
                   case HK:      //HONG KONG
                   case IS:      //ICELAND
                   case IN:      //INDIA
                   case ID:      //INDONESIA
                   case IL:      //ISRAEL
                   case JO:      //JORDAN
                   case KE:      //KENYA
                   case KW:      //KUWAIT
                   case LS:      //LESOTHO
                   case LR:      //LIBERIA
                   case LY:      //LIBYAN ARAB JAMAHIRIYA
                   case LU:      //LUXEMBOURG
                   case MW:      //MALAWI
                   case MY:      //MALAYSIA
                   case MV:      //MALDIVES
                   case MT:      //MALTA
                   case MC:      //MONACO
                   case MZ:      //MOZAMBIQUE
                   case NA:      //NAMIBIA
                   case NP:      //NEPAL
                   case NL:      //NETHERLANDS
                   case NG:      //NIGERIA
                   case KP:      //NORTH KOREA"
                   case NO:      //NORWAY
                   case OM:      //OMAN
                   case PK:      //PAKISTAN
                   case PG:      //PAPUA NEW GUINEA
                   case PY:      //PARAGUAY
                   case PT:      //PORTUGAL
                   case QA:      //QATA
                   case ST:      //SAO TOME AND PRINCIPE
                   case SC:      //SEYCHELLES
                   case SL:      //SIERRA LEONE
                   case SG:      //SINGAPORE
                   case SI:      //SLOVENIA
                   case SO:      //SOMALIA
                   case ES:      //SPAIN
                   case LK:      //SRI LANKA
                   case SD:      //SUDAN
                   case SZ:      //SWAZILAND
                   case SE:      //SWEDEN
                   case CH:      //SWITZERLAND
                   case SY:      //SYRIAN ARAB REPUBLIC
                   case TZ:      //TANZANIA
                   case TH:      //THAILAND
                   case TN:      //TUNISIA
                   case TR:      //TURKEY
                   case UG:      //UGANDA
                   case GB:      //UNITED KINGDOM
                   case UY:      //URUGUAY
                   case ZM:      //ZAMBIA
                   case YE:      //YEMEN
                   case AU:      //AUSTRALIA
                   case ZA:      //SOUTH AFRICA
                   case CZ:      //CZECH REPUBLIC
                   case RO:      //ROMANIA
                   case IE:      //IRELAND
                   case IT:      //ITALY
                   case CN:      //CHINA
                   case AR:      //ARGENTINA
                   case NZ:      //NEW ZEALAND
                        *channellist = FREQLIST_FM_EUROPE;
                        dlog(1, "fm europe.");
                        break;
                  // TODO, check all and implement fm scan lists for other countries here 
                   default:
                        dlog(1, "Country identifier %s not defined. Using default freq lists.", country);
                        return -1;
                        break;
                   }
        return 0; // success
}

int choose_country_analog (const char * country, int * channellist) {
        if (strcasecmp(country_to_short_name(txt_to_country(country)), country)) {
            dlog(0,"\n\nCOUNTRY CODE IS NOT DEFINED. FALLING BACK TO \"DE\"\n");
            }
        // default pal europe west
        *channellist = FREQLIST_PAL_EUROPE_WEST;

        dlog(1, "using settings for %s", country_to_full_name(txt_to_country(country)));

        /*
         * choose channellist name
         */
        switch(txt_to_country(country)) {
        //**********Analogue freq lists*******************************************//
                   case AL:      //ALBANIA
                   case DZ:      //ALGERIA
                   case AO:      //ANGOLA
                   case AE:      //UNITED ARAB EMIRATES
                   case AT:      //AUSTRIA
                   case BH:      //BAHRAIN
                   case BD:      //BANGLADESH
                   case BE:      //BELGIUM
                   case BA:      //BOSNIA AND HERZEGOVINA
                   case BW:      //BOTSWANA
                   case BR:      //BRAZIL
                   case BN:      //BRUNEI DARUSSALAM
                   case KH:      //CAMBODIA
                   case CM:      //CAMEROON
                   case HR:      //CROATIA
                   case CY:      //CYPRUS
                   case DK:      //DENMARK
                   case EG:      //EGYPT
                   case GQ:      //EQUATORIAL GUINEA
                   case ET:      //ETHIOPIA
                   case FI:      //FINLAND
                   case GM:      //GAMBIA
                   case DE:      //GERMANY
                   case GH:      //GHANA
                   case GI:      //GIBRALTAR
                   case GL:      //GREENLAND
                   case GW:      //GUINEA-BISSAU
                   case HK:      //HONG KONG
                   case IS:      //ICELAND
                   case IN:      //INDIA
                   case ID:      //INDONESIA
                   case IL:      //ISRAEL
                   case JO:      //JORDAN
                   case KE:      //KENYA
                   case KW:      //KUWAIT
                   case LS:      //LESOTHO
                   case LR:      //LIBERIA
                   case LY:      //LIBYAN ARAB JAMAHIRIYA
                   case LU:      //LUXEMBOURG
                   case MW:      //MALAWI
                   case MY:      //MALAYSIA
                   case MV:      //MALDIVES
                   case MT:      //MALTA
                   case MC:      //MONACO
                   case MZ:      //MOZAMBIQUE
                   case NA:      //NAMIBIA
                   case NP:      //NEPAL
                   case NL:      //NETHERLANDS
                   case NG:      //NIGERIA
                   case KP:      //NORTH KOREA"
                   case NO:      //NORWAY
                   case OM:      //OMAN
                   case PK:      //PAKISTAN
                   case PG:      //PAPUA NEW GUINEA
                   case PY:      //PARAGUAY
                   case PT:      //PORTUGAL
                   case QA:      //QATA
                   case ST:      //SAO TOME AND PRINCIPE
                   case SC:      //SEYCHELLES
                   case SL:      //SIERRA LEONE
                   case SG:      //SINGAPORE
                   case SI:      //SLOVENIA
                   case SO:      //SOMALIA
                   case ES:      //SPAIN
                   case LK:      //SRI LANKA
                   case SD:      //SUDAN
                   case SZ:      //SWAZILAND
                   case SE:      //SWEDEN
                   case CH:      //SWITZERLAND
                   case SY:      //SYRIAN ARAB REPUBLIC
                   case TZ:      //TANZANIA
                   case TH:      //THAILAND
                   case TN:      //TUNISIA
                   case TR:      //TURKEY
                   case UG:      //UGANDA
                   case GB:      //UNITED KINGDOM
                   case UY:      //URUGUAY
                   case ZM:      //ZAMBIA
                   case YE:      //YEMEN
                        *channellist = FREQLIST_PAL_EUROPE_WEST;
                        dlog(1, "pal europe-west.");
                        break;
                   case CZ:      //CZECH REPUBLIC
                   case RO:      //ROMANIA
                        *channellist = FREQLIST_PAL_EUROPE_EAST;
                        dlog(1, "pal europe-east.");
                        break;
                   case IE:      //IRELAND
                        *channellist = FREQLIST_PAL_IRELAND;
                        dlog(1, "pal Ireland.");
                        break;
                   case IT:      //ITALY
                        *channellist = FREQLIST_PAL_ITALY;
                        dlog(1, "pal Italy.");
                        break;
                   case CN:      //CHINA
                        *channellist = FREQLIST_PAL_CHINA;
                        dlog(1, "pal China.");
                        break;
                   case AR:      //ARGENTINA
                        *channellist = FREQLIST_PAL_ARGENTINIA;
                        dlog(1, "pal argentinia.");
                        break;
                   case NZ:      //NEW ZEALAND
                        *channellist = FREQLIST_PAL_NEWZEALAND;
                        dlog(1, "pal New Zealand.");
                        break;
                   case AU:      //AUSTRALIA
                        *channellist = FREQLIST_PAL_AUSTRALIA;
                        dlog(1, "pal australia.");
                        break;
                   case ZA:      //SOUTH AFRICA
                        *channellist = FREQLIST_PAL_SOUTHAFRIKA;
                        dlog(1, "pal South Africa.");
                        break;
                   case AF:      //AFGHANISTAN
                   case AM:      //ARMENIA
                   case AZ:      //AZERBAIJAN
                   case BY:      //BELARUS
                   case BJ:      //BENIN
                   case BG:      //BULGARIA
                   case BF:      //BURKINA FASO
                   case BI:      //BURUNDI
                   case CV:      //CAPE VERDE
                   case CF:      //CENTRAL AFRICAN REPUBLIC
                   case TD:      //CHAD
                   case CG:      //CONGO
                   case DJ:      //DJIBOUTI
                   case EE:      //ESTONIA
                   case FR:      //FRANCE
                   case GA:      //GABON
                   case GE:      //GEORGIA
                   case GR:      //GREECE
                   case HU:      //HUNGARY
                   case IR:      //IRAN
                   case IQ:      //IRAQ
                   case KZ:      //KAZAKHSTAN
                   case LB:      //LEBANON
                   case LT:      //LITHUANIA
                   case MG:      //MADAGASCAR
                   case ML:      //MALI
                   case MR:      //MAURITANIA
                   case MU:      //MAURITIUS
                   case MD:      //MOLDOVA
                   case MN:      //MONGOLIA
                   case MA:      //MOROCCO
                   case NE:      //NIGER
                   case PL:      //POLAND
                   case RU:      //RUSSIAN FEDERATION
                   case RW:      //RWANDA
                   case SA:      //SAUDI ARABIA
                   case SN:      //SENEGAL
                   case SK:      //SLOVAKIA
                   case TG:      //TOGO
                   case UA:      //UKRAINE
                   case VN:      //VIET NAM
                        *channellist = FREQLIST_SECAM_FRANCE;
                        dlog(1, "secam france.");
                        break;
                   case AG:      //ANTIGUA AND BARBUDA
                   case AW:      //ARUBA
                   case BS:      //BAHAMAS
                   case BB:      //BARBADOS
                   case BZ:      //BELIZE
                   case BM:      //BERMUDA
                   case BO:      //BOLIVIA
                   case CA:      //CANADA
                   case CL:      //CHILE
                   case CO:      //COLOMBIA
                   case CR:      //COSTA RICA
                   case CU:      //CUBA
                   case DO:      //DOMINICAN REPUBLIC
                   case EC:      //ECUADOR
                   case SV:      //EL SALVADOR
                   case GT:      //GUATEMALA
                   case HN:      //HONDURAS
                   case JM:      //JAMAICA
                   case KR:      //SOUTH KOREA
                   case MX:      //MEXICO
                   case MS:      //MONTSERRAT
                   case MM:      //MYANMAR
                   case NI:      //NICARAGUA
                   case PA:      //PANAMA
                   case PE:      //PERU
                   case PH:      //PHILIPPINES
                   case PR:      //PUERTO RICO
                   case WS:      //SAMOA
                   case SR:      //SURINAME
                   case TW:      //TAIWAN
                   case TT:      //TRINIDAD AND TOBAGO
                   case US:      //UNITED STATES
                   case VE:      //VENEZUELA
                   case VI:      //VIRGIN ISLANDS
                        *channellist = FREQLIST_NTSC;
                        dlog(1, "ntsc.");
                        break; 
                   case JP:      //JAPAN
                        *channellist = FREQLIST_NTSC_JAPAN;
                        dlog(1, "ntsc jp.");
                        break; 
 
        //******************************************************************//
        // UNKNOWN / t.b.d.
                   case AX:      //ÅLAND ISLANDS
                   case AS:      //AMERICAN SAMOA
                   case AD:      //ANDORRA
                   case AI:      //ANGUILLA
                   case AQ:      //ANTARCTICA
                   case BT:      //BHUTAN
                   case BV:      //BOUVET ISLAND
                   case IO:      //BRITISH INDIAN OCEAN TERRITORY
                   case KY:      //CAYMAN ISLANDS
                   case CX:      //CHRISTMAS ISLAND
                   case CC:      //COCOS (KEELING) ISLANDS
                   case KM:      //COMOROS
                   case CD:      //CONGO
                   case CK:      //COOK ISLANDS
                   case CI:      //CÔTE D'IVOIRE
                   case DM:      //DOMINICA
                   case ER:      //ERITREA
                   case FK:      //FALKLAND ISLANDS (MALVINAS)
                   case FO:      //FAROE ISLANDS
                   case FJ:      //FIJI
                   case GF:      //FRENCH GUIANA
                   case PF:      //FRENCH POLYNESIA
                   case TF:      //FRENCH SOUTHERN TERRITORIES
                   case GD:      //GRENADA
                   case GP:      //GUADELOUPE
                   case GU:      //GUAM
                   case GG:      //GUERNSEY
                   case GN:      //GUINEA
                   case GY:      //GUYANA
                   case HT:      //HAITI
                   case HM:      //HEARD ISLAND AND MCDONALD ISLANDS
                   case VA:      //HOLY SEE (VATICAN CITY STATE)
                   case IM:      //ISLE OF MAN
                   case JE:      //JERSEY
                   case KI:      //KIRIBATI
                   case KG:      //KYRGYZSTAN
                   case LA:      //LAO PEOPLE'S DEMOCRATIC REPUBLIC
                   case LV:      //LATVIA
                   case LI:      //LIECHTENSTEIN
                   case MO:      //MACAO
                   case MK:      //MACEDONIA
                   case MH:      //MARSHALL ISLANDS
                   case MQ:      //MARTINIQUE
                   case YT:      //MAYOTTE
                   case FM:      //MICRONESIA
                   case ME:      //MONTENEGRO
                   case NR:      //NAURU
                   case AN:      //NETHERLANDS ANTILLES
                   case NC:      //NEW CALEDONIA
                   case NU:      //NIUE
                   case NF:      //NORFOLK ISLAND
                   case MP:      //NORTHERN MARIANA ISLANDS
                   case PW:      //PALAU
                   case PS:      //PALESTINIAN TERRITORY
                   case PN:      //PITCAIRN
                   case RE:      //RÉUNION
                   case BL:      //SAINT BARTHÉLEMY
                   case SH:      //SAINT HELENA
                   case KN:      //SAINT KITTS AND NEVIS
                   case LC:      //SAINT LUCIA
                   case MF:      //SAINT MARTIN
                   case PM:      //SAINT PIERRE AND MIQUELON
                   case VC:      //SAINT VINCENT AND THE GRENADINES
                   case SM:      //SAN MARINO
                   case RS:      //SERBIA
                   case SB:      //SOLOMON ISLANDS
                   case GS:      //SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS
                   case SJ:      //SVALBARD AND JAN MAYEN
                   case TJ:      //TAJIKISTAN
                   case TL:      //TIMOR-LESTE
                   case TK:      //TOKELAU
                   case TO:      //TONGA
                   case TM:      //TURKMENISTAN
                   case TC:      //TURKS AND CAICOS ISLANDS
                   case TV:      //TUVALU
                   case UM:      //UNITED STATES MINOR OUTLYING ISLANDS
                   case UZ:      //UZBEKISTAN
                   case VU:      //VANUATU
                   case VG:      //VIRGIN ISLANDS
                   case WF:      //WALLIS AND FUTUNA
                   case EH:      //WESTERN SAHARA
                   default:
                        dlog(1, "Country identifier %s not defined. Using default freq lists.", country);
                        return -1;
                        break;
                }
        return 0; // success
}
