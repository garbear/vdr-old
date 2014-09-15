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

#define EPG_XML_ROOT              "epg"
#define EPG_XML_ELM_SCHEDULE      "schedule"
#define EPG_XML_ELM_EVENT         "event"
#define EPG_XML_ELM_TITLE         "title"
#define EPG_XML_ELM_PLOT_OUTLINE  "plot_outline"
#define EPG_XML_ELM_PLOT          "plot"
#define EPG_XML_ELM_COMPONENTS    "components"
#define EPG_XML_ELM_COMPONENT     "component"
#define EPG_XML_ELM_CONTENTS      "contents"
#define EPG_XML_ELM_CONTENT       "content"

#define EPG_XML_ATTR_EVENT_ID     "id"
#define EPG_XML_ATTR_START_TIME   "start"
#define EPG_XML_ATTR_END_TIME     "end"
#define EPG_XML_ATTR_GENRE        "genre"
#define EPG_XML_ATTR_SUBGENRE     "subgenre"
#define EPG_XML_ATTR_CUSTOM_GENRE "custom_genre"
#define EPG_XML_ATTR_PARENTAL     "parental"
#define EPG_XML_ATTR_STAR         "star"
#define EPG_XML_ATTR_TABLE_ID     "table"
#define EPG_XML_ATTR_VERSION      "version"
#define EPG_XML_ATTR_VPS          "vps"
#define EPG_XML_ATTR_COMPONENT_ID "id"

#define EPG_COMPONENT_XML_ATTR_STREAM      "stream"
#define EPG_COMPONENT_XML_ATTR_TYPE        "type"
#define EPG_COMPONENT_XML_ATTR_LANGUAGE    "language"
#define EPG_COMPONENT_XML_ATTR_DESCRIPTION "description"

#define EPG_STRING_GENRE_UNDEFINED                      "Undefined"
#define EPG_STRING_GENRE_MOVIEDRAMA                     "Movie/Drama"
#define EPG_STRING_GENRE_NEWSCURRENTAFFAIRS             "News/Current Affairs"
#define EPG_STRING_GENRE_SHOW                           "Show"
#define EPG_STRING_GENRE_SPORTS                         "Sports"
#define EPG_STRING_GENRE_CHILDRENYOUTH                  "Children/Youth"
#define EPG_STRING_GENRE_MUSICBALLETDANCE               "Music/Ballet/Dance"
#define EPG_STRING_GENRE_ARTSCULTURE                    "Arts/Culture"
#define EPG_STRING_GENRE_SOCIALPOLITICALECONOMICS       "Social/Political/Economics"
#define EPG_STRING_GENRE_EDUCATIONALSCIENCE             "Educational/Science"
#define EPG_STRING_GENRE_LEISUREHOBBIES                 "Leisure/Hobbies"
#define EPG_STRING_GENRE_SPECIAL                        "Special"
#define EPG_STRING_GENRE_USERDEFINED                    "User Defined"
#define EPG_STRING_GENRE_CUSTOM                         "Custom"

// Movie/Drama
#define EPG_STRING_SUB_GENRE_DRAMA_OTHER                "Movie/Drama"
#define EPG_STRING_SUB_GENRE_DETECTIVE_THRILLER         "Detective/Thriller"
#define EPG_STRING_SUB_GENRE_ADVENTURE_WESTERN_WAR      "Adventure/Western/War"
#define EPG_STRING_SUB_GENRE_SCIFI_FANTASY_HORROR       "Science Fiction/Fantasy/Horror"
#define EPG_STRING_SUB_GENRE_COMEDY                     "Comedy"
#define EPG_STRING_SUB_GENRE_SOAP_OPERA                 "Soap/Melodrama/Folkloric"
#define EPG_STRING_SUB_GENRE_ROMANCE                    "Romance"
#define EPG_STRING_SUB_GENRE_RELIGIOUS_HISTORICAL       "Classical/Religious/Historical Movie"
#define EPG_STRING_SUB_GENRE_ADULT                      "Adult"

// News/Current Affairs
#define EPG_STRING_SUB_GENRE_CURRENT_AFFAIRS            "News"
#define EPG_STRING_SUB_GENRE_WEATHER                    "News/Weather Report"
#define EPG_STRING_SUB_GENRE_NEWS_MAGAZINE              "News Magazine"
#define EPG_STRING_SUB_GENRE_DOCUMENTARY                "Documentary"
#define EPG_STRING_SUB_GENRE_DISCUSSION                 "Discussion/Interview/Debate"

// Show
#define EPG_STRING_SUB_GENRE_GAME_SHOW                  "Game Show"
#define EPG_STRING_SUB_GENRE_CONTEST                    "Game Show/Quiz/Contest"
#define EPG_STRING_SUB_GENRE_VARIETY                    "Variety Show"
#define EPG_STRING_SUB_GENRE_TALK_SHOW                  "Talk Show"

// Sports
#define EPG_STRING_SUB_GENRE_SPORTS                     "Sports"
#define EPG_STRING_SUB_GENRE_SPECIAL_EVENT              "Special Event"
#define EPG_STRING_SUB_GENRE_SPORTS_MAGAZINE            "Sport Magazine"
#define EPG_STRING_SUB_GENRE_FOOTBALL                   "Football"
#define EPG_STRING_SUB_GENRE_TENNIS                     "Tennis/Squash"
#define EPG_STRING_SUB_GENRE_TEAM_SPORTS                "Team Sports"
#define EPG_STRING_SUB_GENRE_ATHLETICS                  "Athletics"
#define EPG_STRING_SUB_GENRE_MOTOR_SPORT                "Motor Sport"
#define EPG_STRING_SUB_GENRE_WATER_SPORT                "Water Sport"
#define EPG_STRING_SUB_GENRE_WINTER_SPORTS              "Winter Sports"
#define EPG_STRING_SUB_GENRE_EQUESTRIAN                 "Equestrian"
#define EPG_STRING_SUB_GENRE_MARTIAL_SPORTS             "Martial Sports"

// Children/Youth
#define EPG_STRING_SUB_GENRE_CHILDREN                   "Children's/Youth Programmes"
#define EPG_STRING_SUB_GENRE_PRESCHOOL                  "Pre-school Children's Programmes"
#define EPG_STRING_SUB_GENRE_AGES_6_TO_14               "Entertainment Programmes for 6 to 14"
#define EPG_STRING_SUB_GENRE_AGES_10_TO_16              "Entertainment Programmes for 10 to 16"
#define EPG_STRING_SUB_GENRE_EDUCATIONAL                "Informational/Educational/School Programme"
#define EPG_STRING_SUB_GENRE_CARTOONS                   "Cartoons/Puppets"

// Music/Ballet/Dance
#define EPG_STRING_SUB_GENRE_MUSIC_DANCE                "Music/Ballet/Dance"
#define EPG_STRING_SUB_GENRE_ROCK_POP                   "Rock/Pop"
#define EPG_STRING_SUB_GENRE_CLASSICAL_MUSIC            "Serious/Classical Music"
#define EPG_STRING_SUB_GENRE_FOLK_MUSIC                 "Folk/Traditional Music"
#define EPG_STRING_SUB_GENRE_MUSICAL_OPERA              "Musical/Opera"
#define EPG_STRING_SUB_GENRE_BALLET                     "Ballet"

// Arts/Culture
#define EPG_STRING_SUB_GENRE_ARTS_CULTURE               "Arts/Culture"
#define EPG_STRING_SUB_GENRE_PERFORMING_ARTS            "Performing Arts"
#define EPG_STRING_SUB_GENRE_FINE_ARTS                  "Fine Arts"
#define EPG_STRING_SUB_GENRE_RELIGION                   "Religion"
#define EPG_STRING_SUB_GENRE_POP_CULTURE                "Popular Culture/Traditional Arts"
#define EPG_STRING_SUB_GENRE_LITERATURE                 "Literature"
#define EPG_STRING_SUB_GENRE_FILM_CINEMA                "Film/Cinema"
#define EPG_STRING_SUB_GENRE_EXPERIMENTAL               "Experimental Film/Video"
#define EPG_STRING_SUB_GENRE_BROADCASTING_PRESS         "Broadcasting/Press"
#define EPG_STRING_SUB_GENRE_NEW_MEDIA                  "New Media"
#define EPG_STRING_SUB_GENRE_ARTS_MAGAZINES             "Arts/Culture Magazines"
#define EPG_STRING_SUB_GENRE_FASHION                    "Fashion"

// Social/Political/Economics
#define EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_ECONOMICS   "Social/Political/Economics"
#define EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_DOCUMENTARY "Magazines/Reports/Documentary"
#define EPG_STRING_SUB_GENRE_SOCIOPOLITICAL_ADVISORY    "Economics/Social Advisory"
#define EPG_STRING_SUB_GENRE_REMARKABLE_PEOPLE          "Remarkable People"

// Educational Science
#define EPG_STRING_SUB_GENRE_EDUCATION_SCIENCE          "Education/Science/Factual"
#define EPG_STRING_SUB_GENRE_NATURE                     "Nature/Animals/Environment"
#define EPG_STRING_SUB_GENRE_SCITECH                    "Technology/Natural Sciences"
#define EPG_STRING_SUB_GENRE_MEDICINE_BIO_PSYCH         "Medicine/Physiology/Psychology"
#define EPG_STRING_SUB_GENRE_FOREIGN                    "Foreign Countries/Expeditions"
#define EPG_STRING_SUB_GENRE_SOCIAL_SCIENCES            "Social/Spiritual Sciences"
#define EPG_STRING_SUB_GENRE_FURTHER_EDUCATION          "Further Education"
#define EPG_STRING_SUB_GENRE_LANGUAGES                  "Languages"

// Leisure/Hobbies
#define EPG_STRING_SUB_GENRE_HOBBIES                    "Leisure/Hobbies"
#define EPG_STRING_SUB_GENRE_TRAVEL                     "Tourism/Travel"
#define EPG_STRING_SUB_GENRE_HANDICRAFT                 "Handicraft"
#define EPG_STRING_SUB_GENRE_MOTORING                   "Motoring"
#define EPG_STRING_SUB_GENRE_HEALTH                     "Fitness/Health"
#define EPG_STRING_SUB_GENRE_COOKING                    "Cooking"
#define EPG_STRING_SUB_GENRE_SHOPPING                   "Advertisement/Shopping"
#define EPG_STRING_SUB_GENRE_GARDENING                  "Gardening"

// Special
#define EPG_STRING_SUB_GENRE_SPECIAL_CHARACTERISTICS    "Special Characteristics"
#define EPG_STRING_SUB_GENRE_ORIGINAL_LANGUAGE          "Original Language"
#define EPG_STRING_SUB_GENRE_BLACK_AND_WHITE            "Black & White"
#define EPG_STRING_SUB_GENRE_UNPUBLISHED                "Unpublished"
#define EPG_STRING_SUB_GENRE_LIVE_BROADCAST             "Live Broadcast"

// Asterisk (*) marks user-defined subgenres

// TODO: User Defined (XBMC)
#define EPG_STRING_SUB_GENRE_USER_DRAMA                 "Drama*"
#define EPG_STRING_SUB_GENRE_USER_THRILLER              "Detective/Thriller*"
#define EPG_STRING_SUB_GENRE_USER_ADVENTURE             "Adventure/Western/War*"
#define EPG_STRING_SUB_GENRE_USER_SCIFI_FANTASY_HORROR  "Science Fiction/Fantasy/Horror*"
#define EPG_STRING_SUB_GENRE_USER_COMEDY                "Comedy*"
#define EPG_STRING_SUB_GENRE_USER_SOAP_OPERA            "Soap/Melodrama/Folkloric*"
#define EPG_STRING_SUB_GENRE_USER_ROMANCE               "Romance*"
#define EPG_STRING_SUB_GENRE_USER_RELIGIOUS_HISTORICAL  "Serious/ClassicalReligion/Historical*"
#define EPG_STRING_SUB_GENRE_USER_ADULT                 "Adult*"

// TODO: User Defined (VDR)
#define EPG_STRING_SUB_GENRE_USER_UNKNOWN               "Unknown*"
#define EPG_STRING_SUB_GENRE_USER_MOVIE                 "Movie*"
#define EPG_STRING_SUB_GENRE_USER_SPORTS                "Sports*"
#define EPG_STRING_SUB_GENRE_USER_NEWS                  "News*"
#define EPG_STRING_SUB_GENRE_USER_CHILDREN              "Children*"
#define EPG_STRING_SUB_GENRE_USER_EDUCATION             "Education*"
#define EPG_STRING_SUB_GENRE_USER_SERIES                "Series*"
#define EPG_STRING_SUB_GENRE_USER_MUSIC                 "Music*"
#define EPG_STRING_SUB_GENRE_USER_RELIGIOUS             "Religious*"
