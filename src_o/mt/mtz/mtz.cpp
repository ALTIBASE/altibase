/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtz.cpp 55070 2012-08-16 23:16:52Z sparrow $
 **********************************************************************/

#include <mte.h>
#include <idl.h>
#include <mtz.h>

/*
 * timezone names table
 *
 * region name | utc_offset | second value
 * http://en.wikipedia.org/wiki/List_of_tz_database_time_zones
 *
 * abbreviation | utc_offset | second value
 * http://en.wikipedia.org/wiki/Time_zone_abbreviations
 */
static mtzTimezoneNamesTable mTimezoneNamesTable[] =
{
    {"Africa/Abidjan",      "+00:00", 0},
    {"Africa/Accra",        "+00:00", 0},
    {"Africa/Addis_Ababa",  "+03:00", 0},
    {"Africa/Algiers",      "+01:00", 0},
    {"Africa/Asmara",       "+03:00", 0},
    {"Africa/Asmera",       "+03:00", 0},
    {"Africa/Bamako",       "+00:00", 0},
    {"Africa/Bangui",       "+01:00", 0},
    {"Africa/Banjul",       "+00:00", 0},
    {"Africa/Bissau",       "+00:00", 0},
    {"Africa/Blantyre",     "+02:00", 0},
    {"Africa/Brazzaville",  "+01:00", 0},
    {"Africa/Bujumbura",    "+02:00", 0},
    {"Africa/Cairo",        "+02:00", 0},
    {"Africa/Casablanca",   "+00:00", 0},
    {"Africa/Ceuta",        "+01:00", 0},
    {"Africa/Conakry",      "+00:00", 0},
    {"Africa/Dakar",        "+00:00", 0},
    {"Africa/Dar_es_Salaam", "+03:00", 0},
    {"Africa/Djibouti",     "+03:00", 0},
    {"Africa/Douala",       "+01:00", 0},
    {"Africa/El_Aaiun",     "+00:00", 0},
    {"Africa/Freetown",     "+00:00", 0},
    {"Africa/Gaborone",     "+02:00", 0},
    {"Africa/Harare",       "+02:00", 0},
    {"Africa/Johannesburg", "+02:00", 0},
    {"Africa/Juba",         "+03:00", 0},
    {"Africa/Kampala",      "+03:00", 0},
    {"Africa/Khartoum",     "+03:00", 0},
    {"Africa/Kigali",       "+02:00", 0},
    {"Africa/Kinshasa",     "+01:00", 0},
    {"Africa/Lagos",        "+01:00", 0},
    {"Africa/Libreville",   "+01:00", 0},
    {"Africa/Lome",         "+00:00", 0},
    {"Africa/Luanda",       "+01:00", 0},
    {"Africa/Lubumbashi",   "+02:00", 0},
    {"Africa/Lusaka",       "+02:00", 0},
    {"Africa/Malabo",       "+01:00", 0},
    {"Africa/Maputo",       "+02:00", 0},
    {"Africa/Maseru",       "+02:00", 0},
    {"Africa/Mbabane",      "+02:00", 0},
    {"Africa/Mogadishu",    "+03:00", 0},
    {"Africa/Monrovia",     "+00:00", 0},
    {"Africa/Nairobi",      "+03:00", 0},
    {"Africa/Ndjamena",     "+01:00", 0},
    {"Africa/Niamey",       "+01:00", 0},
    {"Africa/Nouakchott",   "+00:00", 0},
    {"Africa/Ouagadougou",  "+00:00", 0},
    {"Africa/Porto-Novo",   "+01:00", 0},
    {"Africa/Sao_Tome",     "+00:00", 0},
    {"Africa/Timbuktu",     "+00:00", 0},
    {"Africa/Tripoli",      "+02:00", 0},
    {"Africa/Tunis",        "+01:00", 0},
    {"Africa/Windhoek",     "+01:00", 0},
    {"AKST9AKDT",           "-09:00", 0},
    {"America/Adak",        "-10:00", 0},
    {"America/Anchorage",   "-09:00", 0},
    {"America/Anguilla",    "-04:00", 0},
    {"America/Antigua",     "-04:00", 0},
    {"America/Araguaina",   "-03:00", 0},
    {"America/Argentina/Buenos_Aires",  "-03:00", 0},
    {"America/Argentina/Catamarca",     "-03:00", 0},
    {"America/Argentina/ComodRivadavia", "-03:00", 0},
    {"America/Argentina/Cordoba",       "-03:00", 0},
    {"America/Argentina/Jujuy",         "-03:00", 0},
    {"America/Argentina/La_Rioja",      "-03:00", 0},
    {"America/Argentina/Mendoza",       "-03:00", 0},
    {"America/Argentina/Rio_Gallegos",  "-03:00", 0},
    {"America/Argentina/Salta",         "-03:00", 0},
    {"America/Argentina/San_Juan",      "-03:00", 0},
    {"America/Argentina/San_Luis",      "-03:00", 0},
    {"America/Argentina/Tucuman",       "-03:00", 0},
    {"America/Argentina/Ushuaia",       "-03:00", 0},
    {"America/Aruba",       "-04:00", 0},
    {"America/Asuncion",    "-04:00", 0},
    {"America/Atikokan",    "-05:00", 0},
    {"America/Atka",        "-10:00", 0},
    {"America/Bahia",       "-03:00", 0},
    {"America/Bahia_Banderas", "-06:00", 0},
    {"America/Barbados",    "-04:00", 0},
    {"America/Belem",       "-03:00", 0},
    {"America/Belize",      "-06:00", 0},
    {"America/Blanc-Sablon", "-04:00", 0},
    {"America/Boa_Vista",   "-04:00", 0},
    {"America/Bogota",      "-05:00", 0},
    {"America/Boise",       "-07:00", 0},
    {"America/Buenos_Aires", "-03:00", 0},
    {"America/Cambridge_Bay", "-07:00", 0},
    {"America/Campo_Grande", "-04:00", 0},
    {"America/Cancun",      "-06:00", 0},
    {"America/Caracas",     "-04:30", 0},
    {"America/Catamarca",   "-03:00", 0},
    {"America/Cayenne",     "-03:00", 0},
    {"America/Cayman",      "-05:00", 0},
    {"America/Chicago",     "-06:00", 0},
    {"America/Chihuahua",   "-07:00", 0},
    {"America/Coral_Harbour", "-05:00", 0},
    {"America/Cordoba",     "-03:00", 0},
    {"America/Costa_Rica",  "-06:00", 0},
    {"America/Creston",     "-07:00", 0},
    {"America/Cuiaba",      "-04:00", 0},
    {"America/Curacao",     "-04:00", 0},
    {"America/Danmarkshavn", "+00:00", 0},
    {"America/Dawson",      "-08:00", 0},
    {"America/Dawson_Creek", "-07:00", 0},
    {"America/Denver",      "-07:00", 0},
    {"America/Detroit",     "-05:00", 0},
    {"America/Dominica",    "-04:00", 0},
    {"America/Edmonton",    "-07:00", 0},
    {"America/Eirunepe",    "-04:00", 0},
    {"America/El_Salvador", "-06:00", 0},
    {"America/Ensenada",    "-08:00", 0},
    {"America/Fort_Wayne",  "-05:00", 0},
    {"America/Fortaleza",   "-03:00", 0},
    {"America/Glace_Bay",   "-04:00", 0},
    {"America/Godthab",     "-03:00", 0},
    {"America/Goose_Bay",   "-04:00", 0},
    {"America/Grand_Turk",  "-05:00", 0},
    {"America/Grenada",     "-04:00", 0},
    {"America/Guadeloupe",  "-04:00", 0},
    {"America/Guatemala",   "-06:00", 0},
    {"America/Guayaquil",   "-05:00", 0},
    {"America/Guyana",      "-04:00", 0},
    {"America/Halifax",     "-04:00", 0},
    {"America/Havana",      "-05:00", 0},
    {"America/Hermosillo",  "-07:00", 0},
    {"America/Indiana/Indianapolis", "-05:00", 0},
    {"America/Indiana/Knox", "-06:00", 0},
    {"America/Indiana/Marengo", "-05:00", 0},
    {"America/Indiana/Petersburg", "-05:00", 0},
    {"America/Indiana/Tell_City", "-06:00", 0},
    {"America/Indiana/Vevay", "-05:00",  0},
    {"America/Indiana/Vincennes", "-05:00", 0},
    {"America/Indiana/Winamac", "-05:00", 0},
    {"America/Indianapolis", "-05:00", 0},
    {"America/Inuvik",      "-07:00", 0},
    {"America/Iqaluit",     "-05:00", 0},
    {"America/Jamaica",     "-05:00", 0},
    {"America/Jujuy",       "-03:00", 0},
    {"America/Juneau",      "-09:00", 0},
    {"America/Kentucky/Louisville", "-05:00", 0},
    {"America/Kentucky/Monticello", "-05:00", 0},
    {"America/Knox_IN",     "-06:00", 0},
    {"America/Kralendijk",  "-04:00", 0},
    {"America/La_Paz",      "-04:00", 0},
    {"America/Lima",        "-05:00", 0},
    {"America/Los_Angeles", "-08:00", 0},
    {"America/Louisville",  "-05:00", 0},
    {"America/Lower_Princes", "-04:00", 0},
    {"America/Maceio",      "-03:00", 0},
    {"America/Managua",     "-06:00", 0},
    {"America/Manaus",      "-04:00", 0},
    {"America/Marigot",     "-04:00", 0},
    {"America/Martinique",  "-04:00", 0},
    {"America/Matamoros",   "-06:00", 0},
    {"America/Mazatlan",    "-07:00", 0},
    {"America/Mendoza",     "-03:00", 0},
    {"America/Menominee",   "-06:00", 0},
    {"America/Merida",      "-06:00", 0},
    {"America/Metlakatla",  "-08:00", 0},
    {"America/Mexico_City", "-06:00", 0},
    {"America/Miquelon",    "-03:00", 0},
    {"America/Moncton",     "-04:00", 0},
    {"America/Monterrey",   "-06:00", 0},
    {"America/Montevideo",  "-03:00", 0},
    {"America/Montreal",    "-05:00", 0},
    {"America/Montserrat",  "-04:00", 0},
    {"America/Nassau",      "-05:00", 0},
    {"America/New_York",    "-05:00", 0},
    {"America/Nipigon",     "-05:00", 0},
    {"America/Nome",        "-09:00", 0},
    {"America/Noronha",     "-02:00", 0},
    {"America/North_Dakota/Beulah", "-06:00", 0},
    {"America/North_Dakota/Center", "-06:00", 0},
    {"America/North_Dakota/New_Salem", "-06:00", 0},
    {"America/Ojinaga",     "-07:00", 0},
    {"America/Panama",      "-05:00", 0},
    {"America/Pangnirtung", "-05:00", 0},
    {"America/Paramaribo",  "-03:00", 0},
    {"America/Phoenix",     "-07:00", 0},
    {"America/Port_of_Spain", "-04:00", 0},
    {"America/Port-au-Prince", "-05:00", 0},
    {"America/Porto_Acre",  "-04:00", 0},
    {"America/Porto_Velho", "04:00", 0},
    {"America/Puerto_Rico", "-04:00", 0},
    {"America/Rainy_River", "-06:00", 0},
    {"America/Rankin_Inlet", "-06:00", 0},
    {"America/Recife",      "-03:00", 0},
    {"America/Regina",      "-06:00", 0},
    {"America/Resolute",    "-06:00", 0},
    {"America/Rio_Branco",  "-04:00", 0},
    {"America/Rosario",     "-03:00", 0},
    {"America/Santa_Isabel", "-08:00", 0},
    {"America/Santarem",    "-03:00", 0},
    {"America/Santiago",    "-04:00", 0},
    {"America/Santo_Domingo", "-04:00", 0},
    {"America/Sao_Paulo",   "-03:00", 0},
    {"America/Scoresbysund", "-01:00", 0},
    {"America/Shiprock",    "-07:00", 0},
    {"America/Sitka",       "-09:00", 0},
    {"America/St_Barthelemy", "-04:00", 0},
    {"America/St_Johns",    "-03:30", 0},
    {"America/St_Kitts",    "-04:00", 0},
    {"America/St_Lucia",    "-04:00", 0},
    {"America/St_Thomas",   "-04:00", 0},
    {"America/St_Vincent",  "-04:00", 0},
    {"America/Swift_Current", "-06:00", 0},
    {"America/Tegucigalpa", "-06:00", 0},
    {"America/Thule",       "-04:00", 0},
    {"America/Thunder_Bay", "-05:00", 0},
    {"America/Tijuana",     "-08:00", 0},
    {"America/Toronto",     "-05:00", 0},
    {"America/Tortola",     "-04:00", 0},
    {"America/Vancouver",   "-08:00", 0},
    {"America/Virgin",      "-04:00", 0},
    {"America/Whitehorse",  "-08:00", 0},
    {"America/Winnipeg",    "-06:00", 0},
    {"America/Yakutat",     "-09:00", 0},
    {"America/Yellowknife", "-07:00", 0},
    {"Antarctica/Casey",    "+11:00", 0},
    {"Antarctica/Davis",    "+05:00", 0},
    {"Antarctica/DumontDUrville", "+10:00", 0},
    {"Antarctica/Macquarie", "+11:00", 0},
    {"Antarctica/Mawson",   "+05:00", 0},
    {"Antarctica/McMurdo",  "+12:00", 0},
    {"Antarctica/Palmer",   "-04:00", 0},
    {"Antarctica/Rothera",  "-03:00", 0},
    {"Antarctica/South_Pole", "+12:00", 0},
    {"Antarctica/Syowa",    "+03:00", 0},
    {"Antarctica/Vostok",   "+06:00", 0},
    {"Arctic/Longyearbyen", "+01:00", 0},
    {"Asia/Aden",           "+03:00", 0},
    {"Asia/Almaty",         "+06:00", 0},
    {"Asia/Amman",          "+02:00", 0},
    {"Asia/Anadyr",         "+12:00", 0},
    {"Asia/Aqtau",          "+05:00", 0},
    {"Asia/Aqtobe",         "+05:00", 0},
    {"Asia/Ashgabat",       "+05:00", 0},
    {"Asia/Ashkhabad",      "+05:00", 0},
    {"Asia/Baghdad",        "+03:00", 0},
    {"Asia/Bahrain",        "+03:00", 0},
    {"Asia/Baku",           "+04:00", 0},
    {"Asia/Bangkok",        "+07:00", 0},
    {"Asia/Beirut",         "+02:00", 0},
    {"Asia/Bishkek",        "+06:00", 0},
    {"Asia/Brunei",         "+08:00", 0},
    {"Asia/Calcutta",       "+05:30", 0},
    {"Asia/Choibalsan",     "+08:00", 0},
    {"Asia/Chongqing",      "+08:00", 0},
    {"Asia/Chungking",      "+08:00", 0},
    {"Asia/Colombo",        "+05:30", 0},
    {"Asia/Dacca",          "+06:00", 0},
    {"Asia/Damascus",       "+02:00", 0},
    {"Asia/Dhaka",          "+06:00", 0},
    {"Asia/Dili",           "+09:00", 0},
    {"Asia/Dubai",          "+04:00", 0},
    {"Asia/Dushanbe",       "+05:00", 0},
    {"Asia/Gaza",           "+02:00", 0},
    {"Asia/Harbin",         "+08:00", 0},
    {"Asia/Hebron",         "+02:00", 0},
    {"Asia/Ho_Chi_Minh",    "+07:00", 0},
    {"Asia/Hong_Kong",      "+08:00", 0},
    {"Asia/Hovd",           "+07:00", 0},
    {"Asia/Irkutsk",        "+09:00", 0},
    {"Asia/Istanbul",       "+02:00", 0},
    {"Asia/Jakarta",        "+07:00", 0},
    {"Asia/Jayapura",       "+09:00", 0},
    {"Asia/Jerusalem",      "+02:00", 0},
    {"Asia/Kabul",          "+04:30", 0},
    {"Asia/Kamchatka",      "+12:00", 0},
    {"Asia/Karachi",        "+05:00", 0},
    {"Asia/Kashgar",        "+08:00", 0},
    {"Asia/Kathmandu",      "+05:45", 0},
    {"Asia/Katmandu",       "+05:45", 0},
    {"Asia/Kolkata",        "+05:30", 0},
    {"Asia/Krasnoyarsk",    "+08:00", 0},
    {"Asia/Kuala_Lumpur",   "+08:00", 0},
    {"Asia/Kuching",        "+08:00", 0},
    {"Asia/Kuwait",         "+03:00", 0},
    {"Asia/Macao",          "+08:00", 0},
    {"Asia/Macau",          "+08:00", 0},
    {"Asia/Magadan",        "+12:00", 0},
    {"Asia/Makassar",       "+08:00", 0},
    {"Asia/Manila",         "+08:00", 0},
    {"Asia/Muscat",         "+04:00", 0},
    {"Asia/Nicosia",        "+02:00", 0},
    {"Asia/Novokuznetsk",   "+07:00", 0},
    {"Asia/Novosibirsk",    "+07:00", 0},
    {"Asia/Omsk",           "+07:00", 0},
    {"Asia/Oral",           "+05:00", 0},
    {"Asia/Phnom_Penh",     "+07:00", 0},
    {"Asia/Pontianak",      "+07:00", 0},
    {"Asia/Pyongyang",      "+09:00", 0},
    {"Asia/Qatar",          "+03:00", 0},
    {"Asia/Qyzylorda",      "+06:00", 0},
    {"Asia/Rangoon",        "+06:30", 0},
    {"Asia/Riyadh",         "+03:00", 0},
    {"Asia/Saigon",         "+07:00", 0},
    {"Asia/Sakhalin",       "+11:00", 0},
    {"Asia/Samarkand",      "+05:00", 0},
    {"Asia/Seoul",          "+09:00", 0},
    {"Asia/Shanghai",       "+08:00", 0},
    {"Asia/Singapore",      "+08:00", 0},
    {"Asia/Taipei",         "+08:00", 0},
    {"Asia/Tashkent",       "+05:00", 0},
    {"Asia/Tbilisi",        "+04:00", 0},
    {"Asia/Tehran",         "+03:30", 0},
    {"Asia/Tel_Aviv",       "+02:00", 0},
    {"Asia/Thimbu",         "+06:00", 0},
    {"Asia/Thimphu",        "+06:00", 0},
    {"Asia/Tokyo",          "+09:00", 0},
    {"Asia/Ujung_Pandang",  "+08:00", 0},
    {"Asia/Ulaanbaatar",    "+08:00", 0},
    {"Asia/Ulan_Bator",     "+08:00", 0},
    {"Asia/Urumqi",         "+08:00", 0},
    {"Asia/Vientiane",      "+07:00", 0},
    {"Asia/Vladivostok",    "+11:00", 0},
    {"Asia/Yakutsk",        "+10:00", 0},
    {"Asia/Yekaterinburg",  "+06:00", 0},
    {"Asia/Yerevan",        "+04:00", 0},
    {"Atlantic/Azores",     "-01:00", 0},
    {"Atlantic/Bermuda",    "-04:00", 0},
    {"Atlantic/Canary",     "+00:00", 0},
    {"Atlantic/Cape_Verde", "-01:00", 0},
    {"Atlantic/Faeroe",     "+00:00", 0},
    {"Atlantic/Faroe",      "+00:00", 0},
    {"Atlantic/Jan_Mayen",  "+01:00", 0},
    {"Atlantic/Madeira",    "+00:00", 0},
    {"Atlantic/Reykjavik",  "+00:00", 0},
    {"Atlantic/South_Georgia", "-02:00", 0},
    {"Atlantic/St_Helena",  "+00:00", 0},
    {"Atlantic/Stanley",    "-03:00", 0},
    {"Australia/ACT",       "+10:00", 0},
    {"Australia/Adelaide",  "+09:30", 0},
    {"Australia/Brisbane",  "+10:00", 0},
    {"Australia/Broken_Hill", "+09:30", 0},
    {"Australia/Canberra",  "+10:00", 0},
    {"Australia/Currie",    "+10:00", 0},
    {"Australia/Darwin",    "+09:30", 0},
    {"Australia/Eucla",     "+08:45", 0},
    {"Australia/Hobart",    "+10:00", 0},
    {"Australia/LHI",       "+10:30", 0},
    {"Australia/Lindeman",  "+10:00", 0},
    {"Australia/Lord_Howe", "+10:30", 0},
    {"Australia/Melbourne", "+10:00", 0},
    {"Australia/North",     "+09:30", 0},
    {"Australia/NSW",       "+10:00", 0},
    {"Australia/Perth",     "+08:00", 0},
    {"Australia/Queensland", "+10:00", 0},
    {"Australia/South",     "+09:30", 0},
    {"Australia/Sydney",    "+10:00", 0},
    {"Australia/Tasmania",  "+10:00", 0},
    {"Australia/Victoria",  "+10:00", 0},
    {"Australia/West",      "+08:00", 0},
    {"Australia/Yancowinna", "+09:30", 0},
    {"Brazil/Acre",         "-04:00", 0},
    {"Brazil/DeNoronha",    "-02:00", 0},
    {"Brazil/East",         "-03:00", 0},
    {"Brazil/West",         "-04:00", 0},
    {"Canada/Atlantic",     "-04:00", 0},
    {"Canada/Central",      "-06:00", 0},
    {"Canada/Eastern",      "-05:00", 0},
    {"Canada/East-Saskatchewan", "-06:00", 0},
    {"Canada/Mountain",     "-07:00", 0},
    {"Canada/Newfoundland", "-03:30", 0},
    {"Canada/Pacific",      "-08:00", 0},
    {"Canada/Saskatchewan", "-06:00", 0},
    {"Canada/Yukon",        "-08:00", 0},
    {"CET",                 "+01:00", 0},
    {"Chile/Continental",   "-04:00", 0},
    {"Chile/EasterIsland",  "-06:00", 0},
    {"CST6CDT",             "-06:00", 0},
    {"Cuba",                "-05:00", 0},
    {"EET",                 "+02:00", 0},
    {"Egypt",               "+02:00", 0},
    {"Eire",                "+00:00", 0},
    {"EST",                 "-05:00", 0},
    {"EST5EDT",             "-05:00", 0},
    {"Etc/GMT",             "+00:00", 0},
    {"Etc/GMT+0",           "+00:00", 0},
    {"Etc/UCT",             "+00:00", 0},
    {"Etc/Universal",       "+00:00", 0},
    {"Etc/UTC",             "+00:00", 0},
    {"Etc/Zulu",            "+00:00", 0},
    {"Europe/Amsterdam",    "+01:00", 0},
    {"Europe/Andorra",      "+01:00", 0},
    {"Europe/Athens",       "+02:00", 0},
    {"Europe/Belfast",      "+00:00", 0},
    {"Europe/Belgrade",     "+01:00", 0},
    {"Europe/Berlin",       "+01:00", 0},
    {"Europe/Bratislava",   "+01:00", 0},
    {"Europe/Brussels",     "+01:00", 0},
    {"Europe/Bucharest",    "+02:00", 0},
    {"Europe/Budapest",     "+01:00", 0},
    {"Europe/Chisinau",     "+02:00", 0},
    {"Europe/Copenhagen",   "+01:00", 0},
    {"Europe/Dublin",       "+00:00", 0},
    {"Europe/Gibraltar",    "+01:00", 0},
    {"Europe/Guernsey",     "+00:00", 0},
    {"Europe/Helsinki",     "+02:00", 0},
    {"Europe/Isle_of_Man",  "+00:00", 0},
    {"Europe/Istanbul",     "+02:00", 0},
    {"Europe/Jersey",       "+00:00", 0},
    {"Europe/Kaliningrad",  "+03:00", 0},
    {"Europe/Kiev",         "+02:00", 0},
    {"Europe/Lisbon",       "+00:00", 0},
    {"Europe/Ljubljana",    "+01:00", 0},
    {"Europe/London",       "+00:00", 0},
    {"Europe/Luxembourg",   "+01:00", 0},
    {"Europe/Madrid",       "+01:00", 0},
    {"Europe/Malta",        "+01:00", 0},
    {"Europe/Mariehamn",    "+02:00", 0},
    {"Europe/Minsk",        "+03:00", 0},
    {"Europe/Monaco",       "+01:00", 0},
    {"Europe/Moscow",       "+04:00", 0},
    {"Europe/Nicosia",      "+02:00", 0},
    {"Europe/Oslo",         "+01:00", 0},
    {"Europe/Paris",        "+01:00", 0},
    {"Europe/Podgorica",    "+01:00", 0},
    {"Europe/Prague",       "+01:00", 0},
    {"Europe/Riga",         "+02:00", 0},
    {"Europe/Rome",         "+01:00", 0},
    {"Europe/Samara",       "+04:00", 0},
    {"Europe/San_Marino",   "+01:00", 0},
    {"Europe/Sarajevo",     "+01:00", 0},
    {"Europe/Simferopol",   "+02:00", 0},
    {"Europe/Skopje",       "+01:00", 0},
    {"Europe/Sofia",        "+02:00", 0},
    {"Europe/Stockholm",    "+01:00", 0},
    {"Europe/Tallinn",      "+02:00", 0},
    {"Europe/Tirane",       "+01:00", 0},
    {"Europe/Tiraspol",     "+02:00", 0},
    {"Europe/Uzhgorod",     "+02:00", 0},
    {"Europe/Vaduz",        "+01:00", 0},
    {"Europe/Vatican",      "+01:00", 0},
    {"Europe/Vienna",       "+01:00", 0},
    {"Europe/Vilnius",      "+02:00", 0},
    {"Europe/Volgograd",    "+04:00", 0},
    {"Europe/Warsaw",       "+01:00", 0},
    {"Europe/Zagreb",       "+01:00", 0},
    {"Europe/Zaporozhye",   "+02:00", 0},
    {"Europe/Zurich",       "+01:00", 0},
    {"GB",                  "+00:00", 0},
    {"GB-Eire",             "+00:00", 0},
    {"GMT",                 "+00:00", 0},
    {"GMT+0",               "+00:00", 0},
    {"GMT0",                "+00:00", 0},
    {"GMT-0",               "+00:00", 0},
    {"Greenwich",           "+00:00", 0},
    {"Hongkong",            "+08:00", 0},
    {"HST",                 "-10:00", 0},
    {"Iceland",             "+00:00", 0},
    {"Indian/Antananarivo", "+03:00", 0},
    {"Indian/Chagos",       "+06:00", 0},
    {"Indian/Christmas",    "+07:00", 0},
    {"Indian/Cocos",        "+06:30", 0},
    {"Indian/Comoro",       "+03:00", 0},
    {"Indian/Kerguelen",    "+05:00", 0},
    {"Indian/Mahe",         "+04:00", 0},
    {"Indian/Maldives",     "+05:00", 0},
    {"Indian/Mauritius",    "+04:00", 0},
    {"Indian/Mayotte",      "+03:00", 0},
    {"Indian/Reunion",      "+04:00", 0},
    {"Iran",                "+03:30", 0},
    {"Israel",              "+02:00", 0},
    {"Jamaica",             "-05:00", 0},
    {"Japan",               "+09:00", 0},
    {"JST-9",               "+09:00", 0},
    {"Kwajalein",           "+12:00", 0},
    {"Libya",               "+02:00", 0},
    {"MET",                 "+01:00", 0},
    {"Mexico/BajaNorte",    "-08:00", 0},
    {"Mexico/BajaSur",      "-07:00", 0},
    {"Mexico/General",      "-06:00", 0},
    {"MST",                 "-07:00", 0},
    {"MST7MDT",             "-07:00", 0},
    {"Navajo",              "-07:00", 0},
    {"NZ",                  "+12:00", 0},
    {"NZ-CHAT",             "+12:45", 0},
    {"Pacific/Apia",        "+13:00", 0},
    {"Pacific/Auckland",    "+12:00", 0},
    {"Pacific/Chatham",     "+12:45", 0},
    {"Pacific/Chuuk",       "+10:00", 0},
    {"Pacific/Easter",      "-06:00", 0},
    {"Pacific/Efate",       "+11:00", 0},
    {"Pacific/Enderbury",   "+13:00", 0},
    {"Pacific/Fakaofo",     "+14:00", 0},
    {"Pacific/Fiji",        "+12:00", 0},
    {"Pacific/Funafuti",    "+12:00", 0},
    {"Pacific/Galapagos",   "-06:00", 0},
    {"Pacific/Gambier",     "-09:00", 0},
    {"Pacific/Guadalcanal", "+11:00", 0},
    {"Pacific/Guam",        "+10:00", 0},
    {"Pacific/Honolulu",    "-10:00", 0},
    {"Pacific/Johnston",    "-10:00", 0},
    {"Pacific/Kiritimati",  "+14:00", 0},
    {"Pacific/Kosrae",      "+11:00", 0},
    {"Pacific/Kwajalein",   "+12:00", 0},
    {"Pacific/Majuro",      "+12:00", 0},
    {"Pacific/Marquesas",   "-09:30", 0},
    {"Pacific/Midway",      "-11:00", 0},
    {"Pacific/Nauru",       "+12:00", 0},
    {"Pacific/Niue",        "-11:00", 0},
    {"Pacific/Norfolk",     "+11:30", 0},
    {"Pacific/Noumea",      "+11:00", 0},
    {"Pacific/Pago_Pago",   "-11:00", 0},
    {"Pacific/Palau",       "+09:00", 0},
    {"Pacific/Pitcairn",    "-08:00", 0},
    {"Pacific/Pohnpei",     "+11:00", 0},
    {"Pacific/Ponape",      "+11:00", 0},
    {"Pacific/Port_Moresby", "+10:00", 0},
    {"Pacific/Rarotonga",   "-10:00", 0},
    {"Pacific/Saipan",      "+10:00", 0},
    {"Pacific/Samoa",       "-11:00", 0},
    {"Pacific/Tahiti",      "-10:00", 0},
    {"Pacific/Tarawa",      "+12:00", 0},
    {"Pacific/Tongatapu",   "+13:00", 0},
    {"Pacific/Truk",        "+10:00", 0},
    {"Pacific/Wake",        "+12:00", 0},
    {"Pacific/Wallis",      "+12:00", 0},
    {"Pacific/Yap",         "+10:00", 0},
    {"Poland",              "+01:00", 0},
    {"Portugal",            "+00:00", 0},
    {"PRC",                 "+08:00", 0},
    {"PST8PDT",             "-08:00", 0},
    {"ROC",                 "+08:00", 0},
    {"ROK",                 "+09:00", 0},
    {"Singapore",           "+08:00", 0},
    {"Turkey",              "+02:00", 0},
    {"UCT",                 "+00:00", 0},
    {"Universal",           "+00:00", 0},
    {"US/Alaska",           "-09:00", 0},
    {"US/Aleutian",         "-10:00", 0},
    {"US/Arizona",          "-07:00", 0},
    {"US/Central",          "-06:00", 0},
    {"US/Eastern",          "-05:00", 0},
    {"US/East-Indiana",     "-05:00", 0},
    {"US/Hawaii",           "-10:00", 0},
    {"US/Indiana-Starke",   "-06:00", 0},
    {"US/Michigan",         "-05:00", 0},
    {"US/Mountain",         "-07:00", 0},
    {"US/Pacific",          "-08:00", 0},
    {"US/Pacific-New",      "-08:00", 0},
    {"US/Samoa",            "-11:00", 0},
    {"UTC",                 "+00:00", 0},
    {"WET",                 "+00:00", 0},
    {"W-SU",                "+04:00", 0},
    {"Zulu",                "+00:00", 0},
    {"ACDT",                "+10:30", 0},
    {"ACST",                "+09:30", 0},
    {"ACT",                 "+08:00", 0},
    {"ADT",                 "-03:00", 0},
    {"AEDT",                "+11:00", 0},
    {"AEST",                "+10:00", 0},
    {"AFT",                 "+04:30", 0},
    {"AKDT",                "-08:00", 0},
    {"AKST",                "-09:00", 0},
    {"AMST",                "+05:00", 0},
    {"AMT",                 "+04:00", 0},
    {"ART",                 "-03:00", 0},
    {"AST",                 "+03:00", 0},
    {"AST",                 "+04:00", 0},
    {"AST",                 "+03:00", 0},
    {"AST",                 "-04:00", 0},
    {"AWDT",                "+09:00", 0},
    {"AWST",                "+08:00", 0},
    {"AZOST",               "-01:00", 0},
    {"AZT",                 "+04:00", 0},
    {"BDT",                 "+08:00", 0},
    {"BIOT",                "+06:00", 0},
    {"BIT",                 "-12:00", 0},
    {"BOT",                 "-04:00", 0},
    {"BRT",                 "-03:00", 0},
    {"BST",                 "+06:00", 0},
    {"BST",                 "+01:00", 0},
    {"BTT",                 "+06:00", 0},
    {"CAT",                 "+02:00", 0},
    {"CCT",                 "+06:30", 0},
    {"CDT",                 "-05:00", 0},
    {"CEDT",                "+02:00", 0},
    {"CEST",                "+02:00", 0},
    {"CET",                 "+01:00", 0},
    {"CHADT",               "+13:45", 0},
    {"CHAST",               "+12:45", 0},
    {"CHOT",                "-08:00", 0},
    {"ChST",                "+10:00", 0},
    {"ChST",                "+10:00", 0},
    {"CHUT",                "+10:00", 0},
    {"CIST",                "-08:00", 0},
    {"CIT",                 "+08:00", 0},
    {"CKT",                 "-10:00", 0},
    {"CLST",                "-03:00", 0},
    {"CLT",                 "-04:00", 0},
    {"COST",                "-04:00", 0},
    {"COT",                 "-05:00", 0},
    {"CST",                 "-06:00", 0},
    {"CST",                 "+08:00", 0},
    {"CST",                 "+09:30", 0},
    {"CST",                 "+10:30", 0},
    {"CST",                 "-05:00", 0},
    {"CT",                  "+08:00", 0},
    {"CVT",                 "-01:00", 0},
    {"CWST",                "+08:45", 0},
    {"CXT",                 "+07:00", 0},
    {"DAVT",                "+07:00", 0},
    {"DDUT",                "+07:00", 0},
    {"DFT",                 "+01:00", 0},
    {"EASST",               "-05:00", 0},
    {"EAST",                "-06:00", 0},
    {"EAT",                 "+03:00", 0},
    {"ECT",                 "-04:00", 0},
    {"ECT",                 "-05:00", 0},
    {"EDT",                 "-04:00", 0},
    {"EEDT",                "+03:00", 0},
    {"EEST",                "+03:00", 0},
    {"EET",                 "+02:00", 0},
    {"EGST",                "+00:00", 0},
    {"EGT",                 "-01:00", 0},
    {"EIT",                 "+09:00", 0},
    {"EST",                 "-05:00", 0},
    {"EST",                 "-05:00", 0},
    {"FET",                 "+03:00", 0},
    {"FJT",                 "+12:00", 0},
    {"FKST",                "-03:00", 0},
    {"FKT",                 "-04:00", 0},
    {"FNT",                 "-02:00", 0},
    {"GALT",                "-06:00", 0},
    {"GAMT",                "-09:00", 0},
    {"GET",                 "+04:00", 0},
    {"GFT",                 "-03:00", 0},
    {"GILT",                "+12:00", 0},
    {"GIT",                 "-09:00", 0},
    {"GMT",                 "+00:00", 0},
    {"GST",                 "-02:00", 0},
    {"GST",                 "+04:00", 0},
    {"GYT",                 "-04:00", 0},
    {"HADT",                "-09:00", 0},
    {"HAEC",                "+02:00", 0},
    {"HAST",                "-10:00", 0},
    {"HKT",                 "+08:00", 0},
    {"HMT",                 "+05:00", 0},
    {"HOVT",                "+07:00", 0},
    {"HST",                 "-10:00", 0},
    {"ICT",                 "+07:00", 0},
    {"IDT",                 "+03:00", 0},
    {"I0T",                 "+03:00", 0},
    {"IRDT",                "+08:00", 0},
    {"IRKT",                "+08:00", 0},
    {"IRST",                "+03:30", 0},
    {"IST",                 "+05:30", 0},
    {"IST",                 "+01:00", 0},
    {"IST",                 "+02:00", 0},
    {"JST",                 "+09:00", 0},
    {"KGT",                 "+07:00", 0},
    {"KOST",                "+11:00", 0},
    {"KRAT",                "+07:00", 0},
    {"KST",                 "+09:00", 0},
    {"LHST",                "+10:30", 0},
    {"LHST",                "+11:00", 0},
    {"LINT",                "+14:00", 0},
    {"MAGT",                "+12:00", 0},
    {"MART",                "-09:30", 0},
    {"MAWT",                "+05:00", 0},
    {"MDT",                 "-06:00", 0},
    {"MET",                 "+01:00", 0},
    {"MEST",                "+02:00", 0},
    {"MHT",                 "+12:00", 0},
    {"MIST",                "+11:00", 0},
    {"MIT",                 "-09:30", 0},
    {"MMT",                 "+06:30", 0},
    {"MSK",                 "+04:00", 0},
    {"MST",                 "+08:00", 0},
    {"MST",                 "-07:00", 0},
    {"MST",                 "+06:30", 0},
    {"MUT",                 "+04:00", 0},
    {"MVT",                 "+05:00", 0},
    {"MYT",                 "+08:00", 0},
    {"NCT",                 "+11:00", 0},
    {"NDT",                 "-02:30", 0},
    {"NFT",                 "+11:30", 0},
    {"NPT",                 "+05:45", 0},
    {"NST",                 "-03:30", 0},
    {"NT",                  "-03:30", 0},
    {"NUT",                 "-11:30", 0},
    {"NZDT",                "+13:00", 0},
    {"NZST",                "+12:00", 0},
    {"OMST",                "+06:00", 0},
    {"ORAT",                "-05:00", 0},
    {"PDT",                 "-07:00", 0},
    {"PET",                 "-05:00", 0},
    {"PETT",                "+12:00", 0},
    {"PGT",                 "+10:00", 0},
    {"PHOT",                "+13:00", 0},
    {"PHT",                 "+08:00", 0},
    {"PKT",                 "+05:00", 0},
    {"PMDT",                "+08:00", 0},
    {"PMST",                "+08:00", 0},
    {"PONT",                "+11:00", 0},
    {"PST",                 "-08:00", 0},
    {"PST",                 "+08:00", 0},
    {"RET",                 "+04:00", 0},
    {"ROTT",                "-03:00", 0},
    {"SAKT",                "+11:00", 0},
    {"SAMT",                "+04:00", 0},
    {"SAST",                "+02:00", 0},
    {"SBT",                 "+11:00", 0},
    {"SCT",                 "+04:00", 0},
    {"SGT",                 "+08:00", 0},
    {"SLT",                 "+05:30", 0},
    {"SRT",                 "-03:00", 0},
    {"SST",                 "-11:00", 0},
    {"SST",                 "+08:00", 0},
    {"SYOT",                "+03:00", 0},
    {"TAHT",                "-10:00", 0},
    {"THA",                 "+07:00", 0},
    {"TFT",                 "+05:00", 0},
    {"TJT",                 "+05:00", 0},
    {"TKT",                 "+14:00", 0},
    {"TLT",                 "+09:00", 0},
    {"TMT",                 "+05:00", 0},
    {"TOT",                 "+13:00", 0},
    {"TVT",                 "+12:00", 0},
    {"UCT",                 "+00:00", 0},
    {"ULAT",                "+08:00", 0},
    {"UTC",                 "+00:00", 0},
    {"UYST",                "-02:00", 0},
    {"UYT",                 "-03:00", 0},
    {"UZT",                 "+05:00", 0},
    {"VET",                 "-04:30", 0},
    {"VLAT",                "+10:00", 0},
    {"VOLT",                "+04:00", 0},
    {"VOST",                "+06:00", 0},
    {"VUT",                 "+11:00", 0},
    {"WAKT",                "+12:00", 0},
    {"WAST",                "+02:00", 0},
    {"WAT",                 "+01:00", 0},
    {"WEDT",                "+01:00", 0},
    {"WEST",                "+01:00", 0},
    {"WET",                 "+00:00", 0},
    {"WST",                 "+08:00", 0},
    {"YAKT",                "+09:00", 0},
    {"YEKT",                "+05:00", 0}
};

IDE_RC mtz::initialize( void )
{
    IDE_TEST( buildSecondOffset() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtz::finalize( void )
{
    return IDE_SUCCESS;
}

IDE_RC mtz::buildSecondOffset( void )
{
    UInt  sCount;
    SLong sSecondOffset;
    UInt  i;
    
    sCount = getTimezoneElementCount();

    for ( i = 0; i < sCount; i++ )
    {
        IDE_TEST( timezoneStr2Second( getTimezoneUTCOffset( i ), &sSecondOffset )
                  != IDE_SUCCESS );

        mTimezoneNamesTable[i].mSecond = sSecondOffset;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt mtz::getTimezoneElementCount( void )
{
    return ID_SIZEOF( mTimezoneNamesTable ) / ID_SIZEOF( mtzTimezoneNamesTable );
}

const SChar * mtz::getTimezoneName( UInt idx )
{
    return mTimezoneNamesTable[idx].mName;
}

const SChar * mtz::getTimezoneUTCOffset( UInt idx )
{
    return mTimezoneNamesTable[idx].mUTCOffset;
}

SLong mtz::getTimezoneSecondWithIndex( UInt idx )
{
    return mTimezoneNamesTable[idx].mSecond;
}

IDE_RC mtz::timezoneStr2Second( const SChar * aTimezoneStr,
                                SLong * aOutTimezoneSecond )
{
     const SChar *sPos;
     SLong  sRes;
     SLong  sSign  = 1;
     SInt   sState = 0;  // 0 'sign', 1 'hh', 2 ':', 3 'mi', 4 end
     SLong  sHH    = 0;  // 24h
     UInt   sDigitsCount = 0;
     SLong  sMI    = 0;

     sPos = aTimezoneStr;

     while ( *sPos != '\0' )
     {
         if ( sState == 0 ) //sign
         {
             if ( *sPos == '+' )
             {
                 sSign  = 1;
                 sState = 1;
                 sPos ++;
                 continue;
             }
             else
             {
                 /* nothing to do. */
             }

             if ( *sPos == '-' )
             {
                 sSign  = -1;
                 sState = 1;
                 sPos ++;
                 continue;
             }
             else
             {
                 /* nothing to do. */
             }

             if ( ( *sPos >= '0' ) && ( *sPos <= '9' ) ) //sign 없이
             {
                 sState =1 ;
             }
             else
             {
                 //첫바이트로 +/- 숫자가 아니면 예외
                 IDE_RAISE( ERR_LITERAL_DOES_NOT_MATCH_FORMAT_STRING );
             }
         }
         else
         {
             /* nothing to do. */
         }

         if ( sState == 1 ) //hh
         {
             if ( ( *sPos >='0' ) && ( *sPos <='9' ) )
             {
                 if ( sHH != 0 )
                 {
                     sHH = sHH * 10;
                     sHH = sHH + *sPos - '0';
                     sState = 2;
                     sDigitsCount = 0;
                     sPos ++;
                     continue;
                 }
                 else
                 {
                     /* nothing to do. */
                 }

                 if ( sHH == 0 ) //처음 또는 09 같이 0 이후
                 {
                     sHH = *sPos - '0';
                     sDigitsCount ++;
                 }
                 else
                 {
                     /* nothing to do. */
                 }

                 if ( sDigitsCount == 2 )
                 {
                     sState = 2;
                     sDigitsCount = 0;
                     sPos ++;
                     continue;
                 }
                 else
                 {
                     /* nothing to do. */
                 }
             }
             else
             {
                 IDE_TEST_RAISE( *sPos != ':', ERR_LITERAL_DOES_NOT_MATCH_FORMAT_STRING );
                 sState =3;
                 sDigitsCount = 0;
                 sPos ++;
                 continue;
             }
         }
         else
         {
             /* nothing to do. */
         }

         if ( sState == 2 ) // :
         {
             IDE_TEST_RAISE( *sPos != ':', ERR_LITERAL_DOES_NOT_MATCH_FORMAT_STRING );
             sState =3;
             sPos ++;
             continue;
         }
         else
         {
             /* nothing to do. */
         }

         if ( sState == 3 ) //min
         {
             if ( ( *sPos >='0' ) && ( *sPos <='9' ) )
             {

                 if ( sMI != 0 )
                 {
                     sMI = sMI * 10;
                     sMI = sMI + *sPos - '0';
                     sDigitsCount ++;

                 }
                 else
                 {
                     /* nothing to do. */
                 }

                 if ( sMI == 0 )
                 {
                     sMI = *sPos -'0';
                     sDigitsCount ++;
                 }
                 else
                 {
                     /* nothing to do. */
                 }

                 if ( sDigitsCount == 2 )
                 {
                     sState = 4;
                     break;
                 }
                 else
                 {
                     /* nothing to do. */
                 }

                 IDE_TEST_RAISE( sDigitsCount > 2 , ERR_MIN_MUST_59 );
             }
             else
             {
                 IDE_RAISE( ERR_LITERAL_DOES_NOT_MATCH_FORMAT_STRING );
             }
         }
         else
         {
             /* nothing to do. */
         }

         sPos ++;
     }

     IDE_TEST_RAISE( sState != 4, ERR_LITERAL_DOES_NOT_MATCH_FORMAT_STRING );

     IDE_TEST_RAISE( sMI > 59, ERR_MIN_MUST_59 );

     sRes = ( sHH * 60 * 60 + sMI * 60 ) * sSign;

     IDE_TEST_RAISE( ( sRes < 12 * 60 * 60 * -1 ) || ( sRes > 14 * 60 * 60 ) , ERR_HOUR_MUST_12_TO_14 );

     *aOutTimezoneSecond = sRes;

     return IDE_SUCCESS;

     IDE_EXCEPTION ( ERR_LITERAL_DOES_NOT_MATCH_FORMAT_STRING )
     {
         //Time zone format string mismatch
         IDE_SET ( ideSetErrorCode ( mtERR_ABORT_TZ_FMT_MISMATCH ) );
     }

     IDE_EXCEPTION ( ERR_MIN_MUST_59 )
     {
         //time zone minute must be between -59 and 59
         IDE_SET ( ideSetErrorCode ( mtERR_ABORT_INVALID_MINUTE ) );
     }

     IDE_EXCEPTION ( ERR_HOUR_MUST_12_TO_14 )
     {
         // Time zone offset out of range
         IDE_SET ( ideSetErrorCode ( mtERR_ABORT_TZ_HOUR_OUT_OF_RANGE ) );
     }

     IDE_EXCEPTION_END;

     return IDE_FAILURE;
}

IDE_RC mtz::getTimezoneSecondAndString( SChar * aTimezoneString,
                                        SLong * aOutTimezoneSecond,
                                        SChar * aOutTimezoneString )
{
    UInt   sCount;
    SChar  sTmpTimezoneString[MTC_TIMEZONE_NAME_LEN + 1];
    SChar *sTimezoneString;
    SChar *sEndString;
    const SChar *sTmpStr;
    UInt   sLen;
    SLong  sAbsSecond;
    UInt   sFindIdx = 0;
    idBool sFound   = ID_FALSE;
    UInt   i;

    *aOutTimezoneSecond = 0;

    sCount = getTimezoneElementCount();

    //trim
    sTimezoneString = aTimezoneString;
    while( idlOS::idlOS_isspace( *sTimezoneString ) )
    {
        sTimezoneString ++;
    }

    sEndString = sTimezoneString + idlOS::strlen( sTimezoneString ) -1;
    while ( ( sEndString > sTimezoneString ) && idlOS::idlOS_isspace(*sEndString) )
    {
        sEndString --;
    }
    sEndString ++;
    sLen = ( sEndString - sTimezoneString );
    IDE_TEST_RAISE( sLen > MTC_TIMEZONE_NAME_LEN, ERR_TZ_REGION_NOT_FOUND );

    idlOS::memcpy( sTmpTimezoneString, sTimezoneString, sLen );
    sTmpTimezoneString[sLen] = 0;
    sTimezoneString = sTmpTimezoneString;

    if ( ( (*sTimezoneString >='A') && (*sTimezoneString <='Z') ) ||
         ( (*sTimezoneString >='a') && (*sTimezoneString <='z') ) )
    {
        for ( i = 0; i < sCount; i++ )
        {
            sTmpStr = getTimezoneName( i );
            if ( idlOS::strCaselessMatch( sTmpStr, sTimezoneString ) == 0 )
            {
                sFindIdx = i;
                sFound = ID_TRUE;
                if ( aOutTimezoneString != NULL )
                {
                    idlOS::memcpy( aOutTimezoneString, sTmpStr, idlOS::strlen( sTmpStr ) );
                    aOutTimezoneString[idlOS::strlen( sTmpStr )] = 0;
                }
                else
                {
                    /* nothing to do. */
                }
                break;
            }
            else
            {
                /* nothing to do. */
            }
        }

        IDE_TEST_RAISE( sFound == ID_FALSE, ERR_TZ_REGION_NOT_FOUND );
        *aOutTimezoneSecond = getTimezoneSecondWithIndex( sFindIdx );
    }
    else
    {
        IDE_TEST( timezoneStr2Second( sTimezoneString, aOutTimezoneSecond )
                  != IDE_SUCCESS );

        if ( aOutTimezoneString != NULL )
        {
            if ( *aOutTimezoneSecond < 0 )
            {
                sAbsSecond = *aOutTimezoneSecond * -1;
                idlOS::snprintf( aOutTimezoneString,
                                 MTC_TIMEZONE_VALUE_LEN + 1,
                                 "-%02"ID_INT64_FMT":%02"ID_INT64_FMT,
                                 sAbsSecond / (3600), (sAbsSecond % 3600)/60 );
            }
            else
            {
                idlOS::snprintf( aOutTimezoneString,
                                 MTC_TIMEZONE_VALUE_LEN + 1,
                                 "+%02"ID_INT64_FMT":%02"ID_INT64_FMT,
                                 *aOutTimezoneSecond / (3600), (*aOutTimezoneSecond % 3600)/60 );
            }
        }
        else
        {
            /* nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION ( ERR_TZ_REGION_NOT_FOUND )
    {
        // Time zone region not found
        IDE_SET ( ideSetErrorCode ( mtERR_ABORT_TZ_REGION_NOT_FOUND ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
