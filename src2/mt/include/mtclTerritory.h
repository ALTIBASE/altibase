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
 * $Id: mtclTerritory.h 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#ifndef _O_MTCL_TERRITORY_H_
#define _O_MTCL_TERRITORY_H_ 1

//#include <idl.h>

ACP_EXTERN_C_BEGIN

typedef enum mtlTerritories {
    TERRITORY_ALBANIA            = 0,
    TERRITORY_ALGERIA,
    TERRITORY_AMERICA,
    TERRITORY_ARGENTINA,
    TERRITORY_AUSTRALIA,
    TERRITORY_AUSTRIA,
    TERRITORY_AZERBAIJAN,
    TERRITORY_BAHRAIN,
    TERRITORY_BANGLADESH,
    TERRITORY_BELARUS,
    TERRITORY_BELGIUM,            /* 10 */
    TERRITORY_BRAZIL,
    TERRITORY_BULGARIA,
    TERRITORY_CANADA,
    TERRITORY_CATALONIA,
    TERRITORY_CHILE,
    TERRITORY_CHINA,
    TERRITORY_CIS,
    TERRITORY_COLOMBIA,
    TERRITORY_COSTA_RICA,
    TERRITORY_CROATIA,            /* 20 */
    TERRITORY_CYPRUS,
    TERRITORY_CZECH_REPUBLIC,
    TERRITORY_CZECHOSLOVAKIA,
    TERRITORY_DENMARK,
    TERRITORY_DJIBOUTI,
    TERRITORY_ECUADOR,
    TERRITORY_EGYPT,
    TERRITORY_EL_SALVADOR,
    TERRITORY_ESTONIA,
    TERRITORY_FINLAND,            /* 30 */
    TERRITORY_FRANCE,
    TERRITORY_FYR_MACEDONIA,
    TERRITORY_GERMANY,
    TERRITORY_GREECE,
    TERRITORY_GUATEMALA,
    TERRITORY_HONG_KONG,
    TERRITORY_HUNGARY,
    TERRITORY_ICELAND,
    TERRITORY_INDIA,
    TERRITORY_INDONESIA,          /* 40 */
    TERRITORY_IRAQ,
    TERRITORY_IRELAND,
    TERRITORY_ISRAEL,
    TERRITORY_ITALY,
    TERRITORY_JAPAN,
    TERRITORY_JORDAN,
    TERRITORY_KAZAKHSTAN,
    TERRITORY_KOREA,
    TERRITORY_KUWAIT,
    TERRITORY_LATVIA,             /* 50 */
    TERRITORY_LEBANON,
    TERRITORY_LIBYA,
    TERRITORY_LITHUANIA,
    TERRITORY_LUXEMBOURG,
    TERRITORY_MACEDONIA,
    TERRITORY_MALAYSIA,
    TERRITORY_MAURITANIA,
    TERRITORY_MEXICO,
    TERRITORY_MOROCCO,
    TERRITORY_NEW_ZEALAND,        /* 60 */
    TERRITORY_NICARAGUA,
    TERRITORY_NORWAY,
    TERRITORY_OMAN,
    TERRITORY_PANAMA,
    TERRITORY_PERU,
    TERRITORY_PHILIPPINES,
    TERRITORY_POLAND,
    TERRITORY_PORTUGAL,
    TERRITORY_PUERTO_RICO,
    TERRITORY_QATAR,              /* 70 */
    TERRITORY_ROMANIA,
    TERRITORY_RUSSIA,
    TERRITORY_SAUDI_ARABIA,
    TERRITORY_SERBIA_AND_MONTENEGRO,
    TERRITORY_SINGAPORE,
    TERRITORY_SLOVAKIA,
    TERRITORY_SLOVENIA,
    TERRITORY_SOMALIA,
    TERRITORY_SOUTH_AFRICA,
    TERRITORY_SPAIN,              /* 80 */
    TERRITORY_SUDAN,
    TERRITORY_SWEDEN,
    TERRITORY_SWITZERLAND,
    TERRITORY_SYRIA,
    TERRITORY_TAIWAN,
    TERRITORY_THAILAND,
    TERRITORY_THE_NETHERLANDS,
    TERRITORY_TUNISIA,
    TERRITORY_TURKEY,
    TERRITORY_UKRAINE,            /* 90 */
    TERRITORY_UNITED_ARAB_EMIRATES,
    TERRITORY_UNITED_KINGDOM,
    TERRITORY_UZBEKISTAN,
    TERRITORY_VENEZUELA,
    TERRITORY_VIETNAM,
    TERRITORY_YEMEN,
    TERRITORY_YUGOSLAVIA,         /* end of oracle territories in 11g2*/
    TERRITORY_MAX
} mtlTerritories;

#define MTL_TERRITORY_DEFAULT                (TERRITORY_KOREA)
#define MTL_TERRITORY_CURRENCY_LEN           (10)
#define MTL_TERRITORY_NUMERIC_CHAR_LEN       (2)
#define MTL_TERRITORY_ISO_LEN                (3)
#define MTL_TERRITORY_NAME_LEN               (40)

typedef struct mtlCurrency
{
    acp_char_t  D;                                  /* Decimal Character */
    acp_char_t  G;                                  /* Group Character */
    acp_char_t  C[MTL_TERRITORY_ISO_LEN + 1];       /* ISO Currency Name */
    acp_char_t  L[MTL_TERRITORY_CURRENCY_LEN + 1];  /* Currency Symbol */
    acp_sint8_t len;                                /* Currency Symbol Size */
} mtlCurrency;

ACP_EXTERN_C_END

#endif /* _O_MTL_TERRITORY_H_ */

