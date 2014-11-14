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

#define FOLDERDELIMCHAR '~'
#define RECEXT_          "rec"
#define DELEXT_          "del"
#define MAX_LINK_LEVEL  6

#define RUC_BEFORERECORDING "before"
#define RUC_AFTERRECORDING  "after"
#define RUC_EDITEDRECORDING "edited"
#define RUC_DELETERECORDING "deleted"

#define MININDEXAGE      3600 // seconds before an index file is considered no longer to be written

/* This was the original code, which works fine in a Linux only environment.
   Unfortunately, because of Windows and its brain dead file system, we have
   to use a more complicated approach, in order to allow users who have enabled
   the --vfat command line option to see their recordings even if they forget to
   enable --vfat when restarting VDR... Gee, do I hate Windows.
   (kls 2002-07-27)
#define DATAFORMAT   "%4d-%02d-%02d.%02d:%02d.%02d.%02d" RECEXT
#define NAMEFORMAT   "%s/%s/" DATAFORMAT
*/
#define DATAFORMATPES   "%4d-%02d-%02d.%02d%*c%02d.%02d.%02d." RECEXT_
#define NAMEFORMATPES   "%s/%s/" "%4d-%02d-%02d.%02d.%02d.%02d.%02d." RECEXT_
#define DATAFORMATTS    "%4d-%02d-%02d.%02d.%02d.%d-%d." RECEXT_
#define NAMEFORMATTS    "%s/%s/" DATAFORMATTS

#define MARKSFILESUFFIX   "/marks.xml"

#define SORTMODEFILE      ".sort"

#define MINDISKSPACE    MEGABYTE(1024)

#define REMOVECHECKDELTA   60 // seconds between checks for removing deleted files
#define DELETEDLIFETIME   300 // seconds after which a deleted recording will be actually removed
#define DISKCHECKDELTA    100 // seconds between checks for free disk space
#define REMOVELATENCY      10 // seconds to wait until next check after removing a file
#define MARKSUPDATEDELTA   10 // seconds between checks for updating editing marks

// The maximum size of a single frame (up to HDTV 1920x1080):
#define MAXFRAMESIZE  (KILOBYTE(1024) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE to avoid breaking up TS packets

// The maximum file size is limited by the range that can be covered
// with a 40 bit 'unsigned int', which is 1TB. The actual maximum value
// used is 6MB below the theoretical maximum, to have some safety (the
// actual file size may be slightly higher because we stop recording only
// before the next independent frame, to have a complete Group Of Pictures):
#define MAXVIDEOFILESIZETS  1048570 // MB
#define MAXVIDEOFILESIZEPES    2000 // MB
#define MINVIDEOFILESIZE        100 // MB
#define MAXVIDEOFILESIZEDEFAULT MAXVIDEOFILESIZEPES

#define DEFAULTFRAMESPERSECOND 25.0

#define RECORDING_START_INTERVAL_SECS (60) // don't try to start the same recording more than once a minute

#define VPSLOOKAHEADTIME      24 // hours within which VPS timers will make sure their events are up to date
#define VPSUPTODATETIME     3600 // seconds before the event or schedule of a VPS timer needs to be refreshed
#define TIMERLOOKAHEADTIME    60 // seconds before a non-VPS timer starts and the channel is switched if possible
#define TIMERCHECKDELTA       10 // seconds between checks for timers that need to see their channel
#define TIMERDEVICETIMEOUT     8 // seconds before a device used for timer check may be reused
