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

#include "EPGStringifier.h"
#include "EPGDefinitions.h"

#include <string.h>

namespace VDR
{

const char* EPGStringifier::GenreToString(EPG_GENRE genre)
{
  switch (genre)
  {
  case EPG_GENRE_UNDEFINED:                return EPG_STRING_GENRE_UNDEFINED;
  case EPG_GENRE_MOVIEDRAMA:               return EPG_STRING_GENRE_MOVIEDRAMA;
  case EPG_GENRE_NEWSCURRENTAFFAIRS:       return EPG_STRING_GENRE_NEWSCURRENTAFFAIRS;
  case EPG_GENRE_SHOW:                     return EPG_STRING_GENRE_SHOW;
  case EPG_GENRE_SPORTS:                   return EPG_STRING_GENRE_SPORTS;
  case EPG_GENRE_CHILDRENYOUTH:            return EPG_STRING_GENRE_CHILDRENYOUTH;
  case EPG_GENRE_MUSICBALLETDANCE:         return EPG_STRING_GENRE_MUSICBALLETDANCE;
  case EPG_GENRE_ARTSCULTURE:              return EPG_STRING_GENRE_ARTSCULTURE;
  case EPG_GENRE_SOCIALPOLITICALECONOMICS: return EPG_STRING_GENRE_SOCIALPOLITICALECONOMICS;
  case EPG_GENRE_EDUCATIONALSCIENCE:       return EPG_STRING_GENRE_EDUCATIONALSCIENCE;
  case EPG_GENRE_LEISUREHOBBIES:           return EPG_STRING_GENRE_LEISUREHOBBIES;
  case EPG_GENRE_SPECIAL:                  return EPG_STRING_GENRE_SPECIAL;
  case EPG_GENRE_USERDEFINED:              return EPG_STRING_GENRE_USERDEFINED;
  case EPG_GENRE_CUSTOM:                   return EPG_STRING_GENRE_CUSTOM;
  default:                                 return EPG_STRING_GENRE_UNDEFINED;
  }
}

EPG_GENRE EPGStringifier::StringToGenre(const char* strGenre)
{
  if (strGenre)
  {
    if (strcmp(strGenre, EPG_STRING_GENRE_UNDEFINED)                == 0) return EPG_GENRE_UNDEFINED;
    if (strcmp(strGenre, EPG_STRING_GENRE_MOVIEDRAMA)               == 0) return EPG_GENRE_MOVIEDRAMA;
    if (strcmp(strGenre, EPG_STRING_GENRE_NEWSCURRENTAFFAIRS)       == 0) return EPG_GENRE_NEWSCURRENTAFFAIRS;
    if (strcmp(strGenre, EPG_STRING_GENRE_SHOW)                     == 0) return EPG_GENRE_SHOW;
    if (strcmp(strGenre, EPG_STRING_GENRE_SPORTS)                   == 0) return EPG_GENRE_SPORTS;
    if (strcmp(strGenre, EPG_STRING_GENRE_CHILDRENYOUTH)            == 0) return EPG_GENRE_CHILDRENYOUTH;
    if (strcmp(strGenre, EPG_STRING_GENRE_MUSICBALLETDANCE)         == 0) return EPG_GENRE_MUSICBALLETDANCE;
    if (strcmp(strGenre, EPG_STRING_GENRE_ARTSCULTURE)              == 0) return EPG_GENRE_ARTSCULTURE;
    if (strcmp(strGenre, EPG_STRING_GENRE_SOCIALPOLITICALECONOMICS) == 0) return EPG_GENRE_SOCIALPOLITICALECONOMICS;
    if (strcmp(strGenre, EPG_STRING_GENRE_EDUCATIONALSCIENCE)       == 0) return EPG_GENRE_EDUCATIONALSCIENCE;
    if (strcmp(strGenre, EPG_STRING_GENRE_LEISUREHOBBIES)           == 0) return EPG_GENRE_LEISUREHOBBIES;
    if (strcmp(strGenre, EPG_STRING_GENRE_SPECIAL)                  == 0) return EPG_GENRE_SPECIAL;
    if (strcmp(strGenre, EPG_STRING_GENRE_USERDEFINED)              == 0) return EPG_GENRE_USERDEFINED;
    if (strcmp(strGenre, EPG_STRING_GENRE_CUSTOM)                   == 0) return EPG_GENRE_CUSTOM;
  }
  return EPG_GENRE_UNDEFINED;
}

const char* EPGStringifier::SubGenreToString(EPG_GENRE genre, EPG_SUB_GENRE subGenre)
{
  switch (genre)
  {
  // Movie/Drama
  case EPG_GENRE_MOVIEDRAMA:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_DRAMA_OTHER:                return EPG_STRING_SUB_GENRE_DRAMA_OTHER;
    case EPG_SUB_GENRE_DETECTIVE_THRILLER:         return EPG_STRING_SUB_GENRE_DETECTIVE_THRILLER;
    case EPG_SUB_GENRE_ADVENTURE_WESTERN_WAR:      return EPG_STRING_SUB_GENRE_ADVENTURE_WESTERN_WAR;
    case EPG_SUB_GENRE_SCIFI_FANTASY_HORROR:       return EPG_STRING_SUB_GENRE_SCIFI_FANTASY_HORROR;
    case EPG_SUB_GENRE_COMEDY:                     return EPG_STRING_SUB_GENRE_COMEDY;
    case EPG_SUB_GENRE_SOAP_OPERA:                 return EPG_STRING_SUB_GENRE_SOAP_OPERA;
    case EPG_SUB_GENRE_ROMANCE:                    return EPG_STRING_SUB_GENRE_ROMANCE;
    case EPG_SUB_GENRE_RELIGIOUS_HISTORICAL:       return EPG_STRING_SUB_GENRE_RELIGIOUS_HISTORICAL;
    case EPG_SUB_GENRE_ADULT:                      return EPG_STRING_SUB_GENRE_ADULT;
    }
    break;

  // News/Current Affairs
  case EPG_GENRE_NEWSCURRENTAFFAIRS:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_CURRENT_AFFAIRS:            return EPG_STRING_SUB_GENRE_CURRENT_AFFAIRS;
    case EPG_SUB_GENRE_WEATHER:                    return EPG_STRING_SUB_GENRE_WEATHER;
    case EPG_SUB_GENRE_NEWS_MAGAZINE:              return EPG_STRING_SUB_GENRE_NEWS_MAGAZINE;
    case EPG_SUB_GENRE_DOCUMENTARY:                return EPG_STRING_SUB_GENRE_DOCUMENTARY;
    case EPG_SUB_GENRE_DISCUSSION:                 return EPG_STRING_SUB_GENRE_DISCUSSION;
    }
    break;

  // Show
  case EPG_GENRE_SHOW:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_GAME_SHOW:                  return EPG_STRING_SUB_GENRE_GAME_SHOW;
    case EPG_SUB_GENRE_CONTEST:                    return EPG_STRING_SUB_GENRE_CONTEST;
    case EPG_SUB_GENRE_VARIETY:                    return EPG_STRING_SUB_GENRE_VARIETY;
    case EPG_SUB_GENRE_TALK_SHOW:                  return EPG_STRING_SUB_GENRE_TALK_SHOW;
    }
    break;

  // Sports
  case EPG_GENRE_SPORTS:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_SPORTS:                     return EPG_STRING_SUB_GENRE_SPORTS;
    case EPG_SUB_GENRE_SPECIAL_EVENT:              return EPG_STRING_SUB_GENRE_SPECIAL_EVENT;
    case EPG_SUB_GENRE_SPORTS_MAGAZINE:            return EPG_STRING_SUB_GENRE_SPORTS_MAGAZINE;
    case EPG_SUB_GENRE_FOOTBALL:                   return EPG_STRING_SUB_GENRE_FOOTBALL;
    case EPG_SUB_GENRE_TENNIS:                     return EPG_STRING_SUB_GENRE_TENNIS;
    case EPG_SUB_GENRE_TEAM_SPORTS:                return EPG_STRING_SUB_GENRE_TEAM_SPORTS;
    case EPG_SUB_GENRE_ATHLETICS:                  return EPG_STRING_SUB_GENRE_ATHLETICS;
    case EPG_SUB_GENRE_MOTOR_SPORT:                return EPG_STRING_SUB_GENRE_MOTOR_SPORT;
    case EPG_SUB_GENRE_WATER_SPORT:                return EPG_STRING_SUB_GENRE_WATER_SPORT;
    case EPG_SUB_GENRE_WINTER_SPORTS:              return EPG_STRING_SUB_GENRE_WINTER_SPORTS;
    case EPG_SUB_GENRE_EQUESTRIAN:                 return EPG_STRING_SUB_GENRE_EQUESTRIAN;
    case EPG_SUB_GENRE_MARTIAL_SPORTS:             return EPG_STRING_SUB_GENRE_MARTIAL_SPORTS;
    }
    break;

  // Children/Youth
  case EPG_GENRE_CHILDRENYOUTH:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_CHILDREN:                   return EPG_STRING_SUB_GENRE_CHILDREN;
    case EPG_SUB_GENRE_PRESCHOOL:                  return EPG_STRING_SUB_GENRE_PRESCHOOL;
    case EPG_SUB_GENRE_AGES_6_TO_14:               return EPG_STRING_SUB_GENRE_AGES_6_TO_14;
    case EPG_SUB_GENRE_AGES_10_TO_16:              return EPG_STRING_SUB_GENRE_AGES_10_TO_16;
    case EPG_SUB_GENRE_EDUCATIONAL:                return EPG_STRING_SUB_GENRE_EDUCATIONAL;
    case EPG_SUB_GENRE_CARTOONS:                   return EPG_STRING_SUB_GENRE_CARTOONS;
    }
    break;

  // Music/Ballet/Dance
  case EPG_GENRE_MUSICBALLETDANCE:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_MUSIC_DANCE:                return EPG_STRING_SUB_GENRE_MUSIC_DANCE;
    case EPG_SUB_GENRE_ROCK_POP:                   return EPG_STRING_SUB_GENRE_ROCK_POP;
    case EPG_SUB_GENRE_CLASSICAL_MUSIC:            return EPG_STRING_SUB_GENRE_CLASSICAL_MUSIC;
    case EPG_SUB_GENRE_FOLK_MUSIC:                 return EPG_STRING_SUB_GENRE_FOLK_MUSIC;
    case EPG_SUB_GENRE_MUSICAL_OPERA:              return EPG_STRING_SUB_GENRE_MUSICAL_OPERA;
    case EPG_SUB_GENRE_BALLET:                     return EPG_STRING_SUB_GENRE_BALLET;
    }
    break;

  // Arts/Culture
  case EPG_GENRE_ARTSCULTURE:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_ARTS_CULTURE:               return EPG_STRING_SUB_GENRE_ARTS_CULTURE;
    case EPG_SUB_GENRE_PERFORMING_ARTS:            return EPG_STRING_SUB_GENRE_PERFORMING_ARTS;
    case EPG_SUB_GENRE_FINE_ARTS:                  return EPG_STRING_SUB_GENRE_FINE_ARTS;
    case EPG_SUB_GENRE_RELIGION:                   return EPG_STRING_SUB_GENRE_RELIGION;
    case EPG_SUB_GENRE_POP_CULTURE:                return EPG_STRING_SUB_GENRE_POP_CULTURE;
    case EPG_SUB_GENRE_LITERATURE:                 return EPG_STRING_SUB_GENRE_LITERATURE;
    case EPG_SUB_GENRE_FILM_CINEMA:                return EPG_STRING_SUB_GENRE_FILM_CINEMA;
    case EPG_SUB_GENRE_EXPERIMENTAL:               return EPG_STRING_SUB_GENRE_EXPERIMENTAL;
    case EPG_SUB_GENRE_BROADCASTING_PRESS:         return EPG_STRING_SUB_GENRE_BROADCASTING_PRESS;
    case EPG_SUB_GENRE_NEW_MEDIA:                  return EPG_STRING_SUB_GENRE_NEW_MEDIA;
    case EPG_SUB_GENRE_ARTS_MAGAZINES:             return EPG_STRING_SUB_GENRE_ARTS_MAGAZINES;
    case EPG_SUB_GENRE_FASHION:                    return EPG_STRING_SUB_GENRE_FASHION;
    }
    break;

  // Social/Political/Economics
  case EPG_GENRE_SOCIALPOLITICALECONOMICS:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_SOCIOPOLITICAL_ECONOMICS:   return EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_ECONOMICS;
    case EPG_SUB_GENRE_SOCIOPOLITICAL_DOCUMENTARY: return EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_DOCUMENTARY;
    case EPG_SUB_GENRE_SOCIOPOLITICAL_ADVISORY:    return EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_ADVISORY;
    case EPG_SUB_GENRE_REMARKABLE_PEOPLE:          return EPG_STRING_SUB_GENRE_REMARKABLE_PEOPLE;
    }
    break;

  // Educational Science
  case EPG_GENRE_EDUCATIONALSCIENCE:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_EDUCATION_SCIENCE:          return EPG_STRING_SUB_GENRE_EDUCATION_SCIENCE;
    case EPG_SUB_GENRE_NATURE:                     return EPG_STRING_SUB_GENRE_NATURE;
    case EPG_SUB_GENRE_SCITECH:                    return EPG_STRING_SUB_GENRE_SCITECH;
    case EPG_SUB_GENRE_MEDICINE_BIO_PSYCH:         return EPG_STRING_SUB_GENRE_MEDICINE_BIO_PSYCH;
    case EPG_SUB_GENRE_FOREIGN:                    return EPG_STRING_SUB_GENRE_FOREIGN;
    case EPG_SUB_GENRE_SOCIAL_SCIENCES:            return EPG_STRING_SUB_GENRE_SOCIAL_SCIENCES;
    case EPG_SUB_GENRE_FURTHER_EDUCATION:          return EPG_STRING_SUB_GENRE_FURTHER_EDUCATION;
    case EPG_SUB_GENRE_LANGUAGES:                  return EPG_STRING_SUB_GENRE_LANGUAGES;
    }
    break;

  // Leisure/Hobbies
  case EPG_GENRE_LEISUREHOBBIES:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_HOBBIES:                    return EPG_STRING_SUB_GENRE_HOBBIES;
    case EPG_SUB_GENRE_TRAVEL:                     return EPG_STRING_SUB_GENRE_TRAVEL;
    case EPG_SUB_GENRE_HANDICRAFT:                 return EPG_STRING_SUB_GENRE_HANDICRAFT;
    case EPG_SUB_GENRE_MOTORING:                   return EPG_STRING_SUB_GENRE_MOTORING;
    case EPG_SUB_GENRE_HEALTH:                     return EPG_STRING_SUB_GENRE_HEALTH;
    case EPG_SUB_GENRE_COOKING:                    return EPG_STRING_SUB_GENRE_COOKING;
    case EPG_SUB_GENRE_SHOPPING:                   return EPG_STRING_SUB_GENRE_SHOPPING;
    case EPG_SUB_GENRE_GARDENING:                  return EPG_STRING_SUB_GENRE_GARDENING;
    }
    break;

  // Special
  case EPG_GENRE_SPECIAL:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_SPECIAL_CHARACTERISTICS:    return EPG_STRING_SUB_GENRE_SPECIAL_CHARACTERISTICS;
    case EPG_SUB_GENRE_ORIGINAL_LANGUAGE:          return EPG_STRING_SUB_GENRE_ORIGINAL_LANGUAGE;
    case EPG_SUB_GENRE_BLACK_AND_WHITE:            return EPG_STRING_SUB_GENRE_BLACK_AND_WHITE;
    case EPG_SUB_GENRE_UNPUBLISHED:                return EPG_STRING_SUB_GENRE_UNPUBLISHED;
    case EPG_SUB_GENRE_LIVE_BROADCAST:             return EPG_STRING_SUB_GENRE_LIVE_BROADCAST;
    }
    break;

  // TODO: User Defined (XBMC)
  case EPG_GENRE_USERDEFINED:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_USER_DRAMA:                 return EPG_STRING_SUB_GENRE_USER_DRAMA;
    case EPG_SUB_GENRE_USER_THRILLER:              return EPG_STRING_SUB_GENRE_USER_THRILLER;
    case EPG_SUB_GENRE_USER_ADVENTURE:             return EPG_STRING_SUB_GENRE_USER_ADVENTURE;
    case EPG_SUB_GENRE_USER_SCIFI_FANTASY_HORROR:  return EPG_STRING_SUB_GENRE_USER_SCIFI_FANTASY_HORROR;
    case EPG_SUB_GENRE_USER_COMEDY:                return EPG_STRING_SUB_GENRE_USER_COMEDY;
    case EPG_SUB_GENRE_USER_SOAP_OPERA:            return EPG_STRING_SUB_GENRE_USER_SOAP_OPERA;
    case EPG_SUB_GENRE_USER_ROMANCE:               return EPG_STRING_SUB_GENRE_USER_ROMANCE;
    case EPG_SUB_GENRE_USER_RELIGIOUS_HISTORICAL:  return EPG_STRING_SUB_GENRE_USER_RELIGIOUS_HISTORICAL;
    case EPG_SUB_GENRE_USER_ADULT:                 return EPG_STRING_SUB_GENRE_USER_ADULT;
    }
    break;

  /*
  // TODO: User Defined (VDR)
  case EPG_GENRE_USERDEFINED:
    switch (subGenre)
    {
    case EPG_SUB_GENRE_USER_UNKNOWN:               return EPG_STRING_SUB_GENRE_USER_UNKNOWN;
    case EPG_SUB_GENRE_USER_MOVIE:                 return EPG_STRING_SUB_GENRE_USER_MOVIE;
    case EPG_SUB_GENRE_USER_SPORTS:                return EPG_STRING_SUB_GENRE_USER_SPORTS;
    case EPG_SUB_GENRE_USER_NEWS:                  return EPG_STRING_SUB_GENRE_USER_NEWS;
    case EPG_SUB_GENRE_USER_CHILDREN:              return EPG_STRING_SUB_GENRE_USER_CHILDREN;
    case EPG_SUB_GENRE_USER_EDUCATION:             return EPG_STRING_SUB_GENRE_USER_EDUCATION;
    case EPG_SUB_GENRE_USER_SERIES:                return EPG_STRING_SUB_GENRE_USER_SERIES;
    case EPG_SUB_GENRE_USER_MUSIC:                 return EPG_STRING_SUB_GENRE_USER_MUSIC;
    case EPG_SUB_GENRE_USER_RELIGIOUS:             return EPG_STRING_SUB_GENRE_USER_RELIGIOUS;
    }
    break;
  */

  default:
    break;
  }

  return "";
}

EPG_SUB_GENRE EPGStringifier::StringToSubGenre(const char* strSubGenre)
{
  if (strSubGenre)
  {
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_DRAMA_OTHER)                == 0) return EPG_SUB_GENRE_DRAMA_OTHER;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_DETECTIVE_THRILLER)         == 0) return EPG_SUB_GENRE_DETECTIVE_THRILLER;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ADVENTURE_WESTERN_WAR)      == 0) return EPG_SUB_GENRE_ADVENTURE_WESTERN_WAR;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SCIFI_FANTASY_HORROR)       == 0) return EPG_SUB_GENRE_SCIFI_FANTASY_HORROR;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_COMEDY)                     == 0) return EPG_SUB_GENRE_COMEDY;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SOAP_OPERA)                 == 0) return EPG_SUB_GENRE_SOAP_OPERA;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ROMANCE)                    == 0) return EPG_SUB_GENRE_ROMANCE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_RELIGIOUS_HISTORICAL)       == 0) return EPG_SUB_GENRE_RELIGIOUS_HISTORICAL;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ADULT)                      == 0) return EPG_SUB_GENRE_ADULT;

    // News/Current Affairs
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_CURRENT_AFFAIRS)            == 0) return EPG_SUB_GENRE_CURRENT_AFFAIRS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_WEATHER)                    == 0) return EPG_SUB_GENRE_WEATHER;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_NEWS_MAGAZINE)              == 0) return EPG_SUB_GENRE_NEWS_MAGAZINE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_DOCUMENTARY)                == 0) return EPG_SUB_GENRE_DOCUMENTARY;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_DISCUSSION)                 == 0) return EPG_SUB_GENRE_DISCUSSION;

    // Show
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_GAME_SHOW)                  == 0) return EPG_SUB_GENRE_GAME_SHOW;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_CONTEST)                    == 0) return EPG_SUB_GENRE_CONTEST;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_VARIETY)                    == 0) return EPG_SUB_GENRE_VARIETY;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_TALK_SHOW)                  == 0) return EPG_SUB_GENRE_TALK_SHOW;

    // Sports
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SPORTS)                     == 0) return EPG_SUB_GENRE_SPORTS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SPECIAL_EVENT)              == 0) return EPG_SUB_GENRE_SPECIAL_EVENT;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SPORTS_MAGAZINE)            == 0) return EPG_SUB_GENRE_SPORTS_MAGAZINE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_FOOTBALL)                   == 0) return EPG_SUB_GENRE_FOOTBALL;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_TENNIS)                     == 0) return EPG_SUB_GENRE_TENNIS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_TEAM_SPORTS)                == 0) return EPG_SUB_GENRE_TEAM_SPORTS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ATHLETICS)                  == 0) return EPG_SUB_GENRE_ATHLETICS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_MOTOR_SPORT)                == 0) return EPG_SUB_GENRE_MOTOR_SPORT;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_WATER_SPORT)                == 0) return EPG_SUB_GENRE_WATER_SPORT;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_WINTER_SPORTS)              == 0) return EPG_SUB_GENRE_WINTER_SPORTS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_EQUESTRIAN)                 == 0) return EPG_SUB_GENRE_EQUESTRIAN;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_MARTIAL_SPORTS)             == 0) return EPG_SUB_GENRE_MARTIAL_SPORTS;

    // Children/Youth
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_CHILDREN)                   == 0) return EPG_SUB_GENRE_CHILDREN;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_PRESCHOOL)                  == 0) return EPG_SUB_GENRE_PRESCHOOL;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_AGES_6_TO_14)               == 0) return EPG_SUB_GENRE_AGES_6_TO_14;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_AGES_10_TO_16)              == 0) return EPG_SUB_GENRE_AGES_10_TO_16;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_EDUCATIONAL)                == 0) return EPG_SUB_GENRE_EDUCATIONAL;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_CARTOONS)                   == 0) return EPG_SUB_GENRE_CARTOONS;

    // Music/Ballet/Dance
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_MUSIC_DANCE)                == 0) return EPG_SUB_GENRE_MUSIC_DANCE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ROCK_POP)                   == 0) return EPG_SUB_GENRE_ROCK_POP;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_CLASSICAL_MUSIC)            == 0) return EPG_SUB_GENRE_CLASSICAL_MUSIC;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_FOLK_MUSIC)                 == 0) return EPG_SUB_GENRE_FOLK_MUSIC;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_MUSICAL_OPERA)              == 0) return EPG_SUB_GENRE_MUSICAL_OPERA;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_BALLET)                     == 0) return EPG_SUB_GENRE_BALLET;

    // Arts/Culture
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ARTS_CULTURE)               == 0) return EPG_SUB_GENRE_ARTS_CULTURE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_PERFORMING_ARTS)            == 0) return EPG_SUB_GENRE_PERFORMING_ARTS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_FINE_ARTS)                  == 0) return EPG_SUB_GENRE_FINE_ARTS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_RELIGION)                   == 0) return EPG_SUB_GENRE_RELIGION;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_POP_CULTURE)                == 0) return EPG_SUB_GENRE_POP_CULTURE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_LITERATURE)                 == 0) return EPG_SUB_GENRE_LITERATURE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_FILM_CINEMA)                == 0) return EPG_SUB_GENRE_FILM_CINEMA;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_EXPERIMENTAL)               == 0) return EPG_SUB_GENRE_EXPERIMENTAL;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_BROADCASTING_PRESS)         == 0) return EPG_SUB_GENRE_BROADCASTING_PRESS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_NEW_MEDIA)                  == 0) return EPG_SUB_GENRE_NEW_MEDIA;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ARTS_MAGAZINES)             == 0) return EPG_SUB_GENRE_ARTS_MAGAZINES;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_FASHION)                    == 0) return EPG_SUB_GENRE_FASHION;

    // Social/Political/Economics
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_ECONOMICS)   == 0) return EPG_SUB_GENRE_SOCIOPOLITICAL_ECONOMICS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_DOCUMENTARY) == 0) return EPG_SUB_GENRE_SOCIOPOLITICAL_DOCUMENTARY;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_ADVISORY)    == 0) return EPG_SUB_GENRE_SOCIOPOLITICAL_ADVISORY;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_REMARKABLE_PEOPLE)          == 0) return EPG_SUB_GENRE_REMARKABLE_PEOPLE;

    // Educational Science
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_EDUCATION_SCIENCE)          == 0) return EPG_SUB_GENRE_EDUCATION_SCIENCE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_NATURE)                     == 0) return EPG_SUB_GENRE_NATURE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SCITECH)                    == 0) return EPG_SUB_GENRE_SCITECH;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_MEDICINE_BIO_PSYCH)         == 0) return EPG_SUB_GENRE_MEDICINE_BIO_PSYCH;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_FOREIGN)                    == 0) return EPG_SUB_GENRE_FOREIGN;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SOCIAL_SCIENCES)            == 0) return EPG_SUB_GENRE_SOCIAL_SCIENCES;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_FURTHER_EDUCATION)          == 0) return EPG_SUB_GENRE_FURTHER_EDUCATION;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_LANGUAGES)                  == 0) return EPG_SUB_GENRE_LANGUAGES;

    // Leisure/Hobbies
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_HOBBIES)                    == 0) return EPG_SUB_GENRE_HOBBIES;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_TRAVEL)                     == 0) return EPG_SUB_GENRE_TRAVEL;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_HANDICRAFT)                 == 0) return EPG_SUB_GENRE_HANDICRAFT;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_MOTORING)                   == 0) return EPG_SUB_GENRE_MOTORING;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_HEALTH)                     == 0) return EPG_SUB_GENRE_HEALTH;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_COOKING)                    == 0) return EPG_SUB_GENRE_COOKING;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SHOPPING)                   == 0) return EPG_SUB_GENRE_SHOPPING;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_GARDENING)                  == 0) return EPG_SUB_GENRE_GARDENING;

    // Special
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_SPECIAL_CHARACTERISTICS)    == 0) return EPG_SUB_GENRE_SPECIAL_CHARACTERISTICS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_ORIGINAL_LANGUAGE)          == 0) return EPG_SUB_GENRE_ORIGINAL_LANGUAGE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_BLACK_AND_WHITE)            == 0) return EPG_SUB_GENRE_BLACK_AND_WHITE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_UNPUBLISHED)                == 0) return EPG_SUB_GENRE_UNPUBLISHED;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_LIVE_BROADCAST)             == 0) return EPG_SUB_GENRE_LIVE_BROADCAST;

    // TODO) User Defined (XBMC)
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_DRAMA)                 == 0) return EPG_SUB_GENRE_USER_DRAMA;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_THRILLER)              == 0) return EPG_SUB_GENRE_USER_THRILLER;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_ADVENTURE)             == 0) return EPG_SUB_GENRE_USER_ADVENTURE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_SCIFI_FANTASY_HORROR)  == 0) return EPG_SUB_GENRE_USER_SCIFI_FANTASY_HORROR;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_COMEDY)                == 0) return EPG_SUB_GENRE_USER_COMEDY;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_SOAP_OPERA)            == 0) return EPG_SUB_GENRE_USER_SOAP_OPERA;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_ROMANCE)               == 0) return EPG_SUB_GENRE_USER_ROMANCE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_RELIGIOUS_HISTORICAL)  == 0) return EPG_SUB_GENRE_USER_RELIGIOUS_HISTORICAL;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_ADULT)                 == 0) return EPG_SUB_GENRE_USER_ADULT;

    // TODO) User Defined (VDR)
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_UNKNOWN)               == 0) return EPG_SUB_GENRE_USER_UNKNOWN;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_MOVIE)                 == 0) return EPG_SUB_GENRE_USER_MOVIE;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_SPORTS)                == 0) return EPG_SUB_GENRE_USER_SPORTS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_NEWS)                  == 0) return EPG_SUB_GENRE_USER_NEWS;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_CHILDREN)              == 0) return EPG_SUB_GENRE_USER_CHILDREN;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_EDUCATION)             == 0) return EPG_SUB_GENRE_USER_EDUCATION;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_SERIES)                == 0) return EPG_SUB_GENRE_USER_SERIES;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_MUSIC)                 == 0) return EPG_SUB_GENRE_USER_MUSIC;
    if (strcmp(strSubGenre, EPG_STRING_SUB_GENRE_USER_RELIGIOUS)             == 0) return EPG_SUB_GENRE_USER_RELIGIOUS;
  }

  return (EPG_SUB_GENRE)0x00; // Default to the first category of the genre
}

}
