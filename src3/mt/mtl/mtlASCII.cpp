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
 * $Id: mtlASCII.cpp 82146 2018-01-29 06:47:57Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtl.h>
#include <mtdTypes.h>

#define mtlASCII_1BYTE_TYPE(c)        ( (c)<=0x7F )

extern mtlModule mtlAscii;

extern UChar* mtl1BYTESpecialCharSet[];

// PROJ-1755
static mtlNCRet mtlAsciiNextChar( UChar ** aSource, UChar * /**/ );

static SInt mtlAsciiMaxPrecision( SInt aLength );

static mtcName mtlNames[3] = {
    { mtlNames+1, 5, (void*)"ASCII"    },
    { mtlNames+2, 8, (void*)"US7ASCII" },
    { NULL      , 7, (void*)"ENGLISH"  }
};


static SInt mtlAsciiExtract_MatchCentury( UChar* aSource,
                                          UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchYear( UChar* aSource,
                                       UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchQuarter( UChar* aSource,
                                          UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchMonth( UChar* aSource,
                                        UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchWeek( UChar* aSource,
                                       UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchWeekOfMonth( UChar* aSource,
                                              UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchDay( UChar* aSource,
                                      UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchDayOfYear( UChar* aSource,
                                            UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchDayOfWeek( UChar* aSource,
                                            UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchHour( UChar* aSource,
                                       UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchMinute( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchSecond( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiExtract_MatchMicroSec( UChar* aSource,
                                           UInt   aSourceLen );

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
static SInt mtlAsciiExtract_MatchISOWeek( UChar * aSource,
                                          UInt    aSourceLen );

mtlExtractFuncSet mtlAsciiExtractSet = {
    mtlAsciiExtract_MatchCentury,
    mtlAsciiExtract_MatchYear,
    mtlAsciiExtract_MatchQuarter,
    mtlAsciiExtract_MatchMonth,
    mtlAsciiExtract_MatchWeek,
    mtlAsciiExtract_MatchWeekOfMonth,
    mtlAsciiExtract_MatchDay,
    mtlAsciiExtract_MatchDayOfYear,
    mtlAsciiExtract_MatchDayOfWeek,
    mtlAsciiExtract_MatchHour,
    mtlAsciiExtract_MatchMinute,
    mtlAsciiExtract_MatchSecond,
    mtlAsciiExtract_MatchMicroSec,
    /* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
    mtlAsciiExtract_MatchISOWeek
};

static SInt mtlAsciiNextDay_MatchSunDay( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiNextDay_MatchMonDay( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiNextDay_MatchTueDay( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiNextDay_MatchWedDay( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiNextDay_MatchThuDay( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiNextDay_MatchFriDay( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlAsciiNextDay_MatchSatDay( UChar* aSource,
                                         UInt   aSourceLen );

mtlNextDayFuncSet mtlAsciiNextDaySet = {
    mtlAsciiNextDay_MatchSunDay,
    mtlAsciiNextDay_MatchMonDay,
    mtlAsciiNextDay_MatchTueDay,
    mtlAsciiNextDay_MatchWedDay,
    mtlAsciiNextDay_MatchThuDay,
    mtlAsciiNextDay_MatchFriDay,
    mtlAsciiNextDay_MatchSatDay
};


mtlModule mtlAscii = {
    mtlNames,
    MTL_ASCII_ID,
    mtlAsciiNextChar,
    mtlAsciiMaxPrecision,
    &mtlAsciiExtractSet,
    &mtlAsciiNextDaySet,
    mtl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlAsciiNextChar( UChar ** aSource, UChar * /**/ )
{
/***********************************************************************
 *
 * Description : PROJ-1755 Next Char 최적화
 *
 * Implementation :
 *    다음 문자 위치로 pointer 이동
 *
 ***********************************************************************/

    mtlNCRet sRet;
    
    if ( mtlASCII_1BYTE_TYPE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else
    {
        sRet = NC_INVALID;
        (*aSource)++;
    }
    
    return sRet;
}

static SInt mtlAsciiMaxPrecision( SInt aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 ASCII의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    ascii 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    
    return aLength * MTL_ASCII_PRECISION;
}

static SInt mtlAsciiExtract_MatchCentury( UChar* aSource,
                                          UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "CENTURY", 7 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "SCC", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "CC",2 ) == 0 ) 
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchYear( UChar* aSource,
                                       UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "YEAR", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "SYYYY", 5 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "YYYY", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "YYY", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "YY", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "Y", 1 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchQuarter( UChar* aSource,
                                          UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "QUARTER", 7 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "Q", 1 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchMonth( UChar* aSource,
                                        UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "MONTH", 5 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "MON", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "MM", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "RM", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchWeek( UChar* aSource,
                                       UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "WEEK", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "WW", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchWeekOfMonth( UChar* aSource,
                                              UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "WEEKOFMONTH", 11 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "W", 1 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchDay( UChar* aSource,
                                      UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "DAY", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "DDD", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "DD", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "J", 1 ) == 0  )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchDayOfYear( UChar* aSource,
                                            UInt   aSourceLen )
{
    return idlOS::strCaselessMatch( aSource, aSourceLen, "DAYOFYEAR", 9 );
}

static SInt mtlAsciiExtract_MatchDayOfWeek( UChar* aSource,
                                            UInt   aSourceLen )
{
        if ( idlOS::strCaselessMatch( aSource, aSourceLen, "DAYOFWEEK", 9 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "DY", 2 ) == 0 ||
             idlOS::strCaselessMatch( aSource, aSourceLen, "D", 1 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchHour( UChar* aSource,
                                       UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "HOUR", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "HH", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "HH12", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "HH24", 4 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchMinute( UChar* aSource,
                                         UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "MINUTE", 6 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "MI", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiExtract_MatchSecond( UChar* aSource,
                                         UInt   aSourceLen )
{
    return idlOS::strCaselessMatch( aSource, aSourceLen, "SECOND", 6 );
}

static SInt mtlAsciiExtract_MatchMicroSec( UChar* aSource,
                                           UInt   aSourceLen )
{
    return idlOS::strCaselessMatch( aSource, aSourceLen, "MICROSECOND", 11 );
}

static SInt mtlAsciiNextDay_MatchSunDay( UChar* aSource,
                                         UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "SUNDAY", 6 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "SUN", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiNextDay_MatchMonDay( UChar* aSource,
                                         UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "MONDAY", 6 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "MON", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiNextDay_MatchTueDay( UChar* aSource,
                                         UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "TUESDAY", 7 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "TUE", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiNextDay_MatchWedDay( UChar* aSource,
                                         UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "WEDNESDAY", 9 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "WED", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiNextDay_MatchThuDay( UChar* aSource,
                                         UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "THURSDAY", 8 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "THU", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiNextDay_MatchFriDay( UChar* aSource,
                                         UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "FRIDAY", 6 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "FRI", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static SInt mtlAsciiNextDay_MatchSatDay( UChar* aSource,
                                         UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "SATURDAY", 8 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "SAT", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
static SInt mtlAsciiExtract_MatchISOWeek( UChar * aSource,
                                          UInt    aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "IW", 2 ) == 0 )
    {
        return 0;
    }

    return 1;
}

