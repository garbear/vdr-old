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

#include "xbmc/xbmc_epg_types.h"

#include <shared_ptr/shared_ptr.hpp>
#include <sys/types.h>
#include <vector>

namespace VDR
{

class cSchedule;
typedef VDR::shared_ptr<cSchedule> SchedulePtr;
typedef std::vector<SchedulePtr>   ScheduleVector;

class cEvent;
typedef VDR::shared_ptr<cEvent> EventPtr;
typedef std::vector<EventPtr>   EventVector;

typedef u_int32_t tEventID;

/*!
 * EPG entry content event types
 *
 * These IDs come from the DVB-SI EIT table "content descriptor". Also known
 * under the name "E-book genre assignments".
 *
 * See xbmc_epg_types.h
*/
enum EPG_GENRE
{
  EPG_GENRE_UNDEFINED                = EPG_EVENT_CONTENTMASK_UNDEFINED,
  EPG_GENRE_MOVIEDRAMA               = EPG_EVENT_CONTENTMASK_MOVIEDRAMA,
  EPG_GENRE_NEWSCURRENTAFFAIRS       = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS,
  EPG_GENRE_SHOW                     = EPG_EVENT_CONTENTMASK_SHOW,
  EPG_GENRE_SPORTS                   = EPG_EVENT_CONTENTMASK_SPORTS,
  EPG_GENRE_CHILDRENYOUTH            = EPG_EVENT_CONTENTMASK_CHILDRENYOUTH,
  EPG_GENRE_MUSICBALLETDANCE         = EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE,
  EPG_GENRE_ARTSCULTURE              = EPG_EVENT_CONTENTMASK_ARTSCULTURE,
  EPG_GENRE_SOCIALPOLITICALECONOMICS = EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS,
  EPG_GENRE_EDUCATIONALSCIENCE       = EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE,
  EPG_GENRE_LEISUREHOBBIES           = EPG_EVENT_CONTENTMASK_LEISUREHOBBIES,
  EPG_GENRE_SPECIAL                  = EPG_EVENT_CONTENTMASK_SPECIAL,
  EPG_GENRE_USERDEFINED              = EPG_EVENT_CONTENTMASK_USERDEFINED,
  EPG_GENRE_CUSTOM                   = EPG_GENRE_USE_STRING, // Indicates custom genre string
};

enum EPG_SUB_GENRE
{
  // Movie/Drama
  EPG_SUB_GENRE_DRAMA_OTHER                = 0x00, // Other/Unknown Drama
  EPG_SUB_GENRE_DETECTIVE_THRILLER         = 0x01, // Detective/Thriller
  EPG_SUB_GENRE_ADVENTURE_WESTERN_WAR      = 0x02, // Adventure/Western/War
  EPG_SUB_GENRE_SCIFI_FANTASY_HORROR       = 0x03, // Science Fiction/Fantasy/Horror
  EPG_SUB_GENRE_COMEDY                     = 0x04, // Comedy
  EPG_SUB_GENRE_SOAP_OPERA                 = 0x05, // Soap/Melodrama/Folkloric
  EPG_SUB_GENRE_ROMANCE                    = 0x06, // Romance
  EPG_SUB_GENRE_RELIGIOUS_HISTORICAL       = 0x07, // Serious/Classical/Religious/Historical Movie/Drama
  EPG_SUB_GENRE_ADULT                      = 0x08, // Adult

  // News/Current Affairs
  EPG_SUB_GENRE_CURRENT_AFFAIRS            = 0x00, // News/Current Affairs
  EPG_SUB_GENRE_WEATHER                    = 0x01, // News/Weather Report
  EPG_SUB_GENRE_NEWS_MAGAZINE              = 0x02, // News Magazine
  EPG_SUB_GENRE_DOCUMENTARY                = 0x03, // Documentary
  EPG_SUB_GENRE_DISCUSSION                 = 0x04, // Discussion/Interview/Debate

  // Show
  EPG_SUB_GENRE_GAME_SHOW                  = 0x00, // Show/Game Show
  EPG_SUB_GENRE_CONTEST                    = 0x01, // Game Show/Quiz/Contest
  EPG_SUB_GENRE_VARIETY                    = 0x02, // Variety Show
  EPG_SUB_GENRE_TALK_SHOW                  = 0x03, // Talk Show

  // Sports
  EPG_SUB_GENRE_SPORTS                     = 0x00, // Sports
  EPG_SUB_GENRE_SPECIAL_EVENT              = 0x01, // Special Event
  EPG_SUB_GENRE_SPORTS_MAGAZINE            = 0x02, // Sport Magazine
  EPG_SUB_GENRE_FOOTBALL                   = 0x03, // Football
  EPG_SUB_GENRE_TENNIS                     = 0x04, // Tennis/Squash
  EPG_SUB_GENRE_TEAM_SPORTS                = 0x05, // Team Sports
  EPG_SUB_GENRE_ATHLETICS                  = 0x06, // Athletics
  EPG_SUB_GENRE_MOTOR_SPORT                = 0x07, // Motor Sport
  EPG_SUB_GENRE_WATER_SPORT                = 0x08, // Water Sport
  EPG_SUB_GENRE_WINTER_SPORTS              = 0x09, // Winter Sports
  EPG_SUB_GENRE_EQUESTRIAN                 = 0x0A, // Equestrian
  EPG_SUB_GENRE_MARTIAL_SPORTS             = 0x0B, // Martial Sports

  // Children's Shows
  EPG_SUB_GENRE_CHILDREN                   = 0x00, // Children's/Youth Programmes
  EPG_SUB_GENRE_PRESCHOOL                  = 0x01, // Pre-school Children's Programmes
  EPG_SUB_GENRE_AGES_6_TO_14               = 0x02, // Entertainment Programmes for 6 to 14
  EPG_SUB_GENRE_AGES_10_TO_16              = 0x03, // Entertainment Programmes for 10 to 16
  EPG_SUB_GENRE_EDUCATIONAL                = 0x04, // Informational/Educational/School Programme
  EPG_SUB_GENRE_CARTOONS                   = 0x05, // Cartoons/Puppets

  // Music/Ballet/Dance
  EPG_SUB_GENRE_MUSIC_DANCE                = 0x00, // Music/Ballet/Dance
  EPG_SUB_GENRE_ROCK_POP                   = 0x01, // Rock/Pop
  EPG_SUB_GENRE_CLASSICAL_MUSIC            = 0x02, // Serious/Classical Music
  EPG_SUB_GENRE_FOLK_MUSIC                 = 0x03, // Folk/Traditional Music
  EPG_SUB_GENRE_MUSICAL_OPERA              = 0x04, // Musical/Opera
  EPG_SUB_GENRE_BALLET                     = 0x05, // Ballet

  // Arts/Culture
  EPG_SUB_GENRE_ARTS_CULTURE               = 0x00, // Arts/Culture
  EPG_SUB_GENRE_PERFORMING_ARTS            = 0x01, // Performing Arts
  EPG_SUB_GENRE_FINE_ARTS                  = 0x02, // Fine Arts
  EPG_SUB_GENRE_RELIGION                   = 0x03, // Religion
  EPG_SUB_GENRE_POP_CULTURE                = 0x04, // Popular Culture/Traditional Arts
  EPG_SUB_GENRE_LITERATURE                 = 0x05, // Literature
  EPG_SUB_GENRE_FILM_CINEMA                = 0x06, // Film/Cinema
  EPG_SUB_GENRE_EXPERIMENTAL               = 0x07, // Experimental Film/Video
  EPG_SUB_GENRE_BROADCASTING_PRESS         = 0x08, // Broadcasting/Press
  EPG_SUB_GENRE_NEW_MEDIA                  = 0x09, // New Media
  EPG_SUB_GENRE_ARTS_MAGAZINES             = 0x0A, // Arts/Culture Magazines
  EPG_SUB_GENRE_FASHION                    = 0x0B, // Fashion

  // Social/Political/Economics
  EPG_SUB_GENRE_SOCIOPOLITICAL_ECONOMICS   = 0x00, // Social/Political/Economics
  EPG_SUB_GENRE_SOCIOPOLITICAL_DOCUMENTARY = 0x01, // Magazines/Reports/Documentary
  EPG_SUB_GENRE_SOCIOPOLITICAL_ADVISORY    = 0x02, // Economics/Social Advisory
  EPG_SUB_GENRE_REMARKABLE_PEOPLE          = 0x03, // Remarkable People

  // Educational Science
  EPG_SUB_GENRE_EDUCATION_SCIENCE          = 0x00, // Education/Science/Factual
  EPG_SUB_GENRE_NATURE                     = 0x01, // Nature/Animals/Environment
  EPG_SUB_GENRE_SCITECH                    = 0x02, // Technology/Natural Sciences
  EPG_SUB_GENRE_MEDICINE_BIO_PSYCH         = 0x03, // Medicine/Physiology/Psychology
  EPG_SUB_GENRE_FOREIGN                    = 0x04, // Foreign Countries/Expeditions
  EPG_SUB_GENRE_SOCIAL_SCIENCES            = 0x05, // Social/Spiritual Sciences
  EPG_SUB_GENRE_FURTHER_EDUCATION          = 0x06, // Further Education
  EPG_SUB_GENRE_LANGUAGES                  = 0x07, // Languages

  // Leisure/Hobbies
  EPG_SUB_GENRE_HOBBIES                    = 0x00, // Leisure/Hobbies
  EPG_SUB_GENRE_TRAVEL                     = 0x01, // Tourism/Travel
  EPG_SUB_GENRE_HANDICRAFT                 = 0x02, // Handicraft
  EPG_SUB_GENRE_MOTORING                   = 0x03, // Motoring
  EPG_SUB_GENRE_HEALTH                     = 0x04, // Fitness & Health
  EPG_SUB_GENRE_COOKING                    = 0x05, // Cooking
  EPG_SUB_GENRE_SHOPPING                   = 0x06, // Advertisement/Shopping
  EPG_SUB_GENRE_GARDENING                  = 0x07, // Gardening

  // Special
  EPG_SUB_GENRE_SPECIAL_CHARACTERISTICS    = 0x00, // Special Characteristics
  EPG_SUB_GENRE_ORIGINAL_LANGUAGE          = 0x01, // Original Language
  EPG_SUB_GENRE_BLACK_AND_WHITE            = 0x02, // Black & White
  EPG_SUB_GENRE_UNPUBLISHED                = 0x03, // Unpublished
  EPG_SUB_GENRE_LIVE_BROADCAST             = 0x04, // Live Broadcast

  // TODO: User Defined (XBMC)
  EPG_SUB_GENRE_USER_DRAMA                 = 0x00, // Drama
  EPG_SUB_GENRE_USER_THRILLER              = 0x01, // Detective/Thriller
  EPG_SUB_GENRE_USER_ADVENTURE             = 0x02, // Adventure/Western/War
  EPG_SUB_GENRE_USER_SCIFI_FANTASY_HORROR  = 0x03, // Science Fiction/Fantasy/Horror
  EPG_SUB_GENRE_USER_COMEDY                = 0x04, // Comedy
  EPG_SUB_GENRE_USER_SOAP_OPERA            = 0x05, // Soap/Melodrama/Folkloric
  EPG_SUB_GENRE_USER_ROMANCE               = 0x06, // Romance
  EPG_SUB_GENRE_USER_RELIGIOUS_HISTORICAL  = 0x07, // Serious/ClassicalReligion/Historical
  EPG_SUB_GENRE_USER_ADULT                 = 0x08, // Adult

  // TODO: User Defined (VDR)
  EPG_SUB_GENRE_USER_UNKNOWN               = 0x00, // Unknown
  EPG_SUB_GENRE_USER_MOVIE                 = 0x01, // Movie
  EPG_SUB_GENRE_USER_SPORTS                = 0x02, // Sports
  EPG_SUB_GENRE_USER_NEWS                  = 0x03, // News
  EPG_SUB_GENRE_USER_CHILDREN              = 0x04, // Children
  EPG_SUB_GENRE_USER_EDUCATION             = 0x05, // Education
  EPG_SUB_GENRE_USER_SERIES                = 0x06, // Series
  EPG_SUB_GENRE_USER_MUSIC                 = 0x07, // Music
  EPG_SUB_GENRE_USER_RELIGIOUS             = 0x08, // Religious
};

}
