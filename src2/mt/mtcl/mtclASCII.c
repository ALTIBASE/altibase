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
 * $Id: mtlASCII.cpp 32401 2009-04-29 06:01:36Z mhjeong $
 **********************************************************************/

#include <mtce.h>
#include <mtcl.h>
#include <mtcdTypes.h>

#define mtlASCII_1BYTE_TYPE(c)        ( (c)<=0x7F )

extern mtlModule mtclAscii;

extern acp_uint8_t* mtcl1BYTESpecialCharSet[];

// PROJ-1755
static mtlNCRet mtlAsciiNextChar( acp_uint8_t ** aSource, acp_uint8_t * /**/ );

static acp_sint32_t mtlAsciiMaxPrecision( acp_sint32_t aLength );

static mtcName mtlNames[3] = {
    { mtlNames+1, 5, (void*)"ASCII"    },
    { mtlNames+2, 8, (void*)"US7ASCII" },
    { NULL      , 7, (void*)"ENGLISH"  }
};

static acp_sint32_t mtlAsciiExtract_MatchCentury( acp_uint8_t* aSource,
                                                  acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchYear( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchQuarter( acp_uint8_t* aSource,
                                                  acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchMonth( acp_uint8_t* aSource,
                                                acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchWeek( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchWeekOfMonth( acp_uint8_t* aSource,
                                                      acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchDay( acp_uint8_t* aSource,
                                              acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchDayOfYear( acp_uint8_t* aSource,
                                                    acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchDayOfWeek( acp_uint8_t* aSource,
                                                    acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchHour( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchMinute( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchSecond( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiExtract_MatchMicroSec( acp_uint8_t* aSource,
                                                   acp_uint32_t aSourceLen );

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
static acp_sint32_t mtlAsciiExtract_MatchISOWeek( acp_uint8_t  * aSource,
                                                  acp_uint32_t   aSourceLen );

mtlExtractFuncSet mtclAsciiExtractSet = {
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

static acp_sint32_t mtlAsciiNextDay_MatchSunDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiNextDay_MatchMonDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiNextDay_MatchTueDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiNextDay_MatchWedDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiNextDay_MatchThuDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiNextDay_MatchFriDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

static acp_sint32_t mtlAsciiNextDay_MatchSatDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen );

mtlNextDayFuncSet mtclAsciiNextDaySet = {
    mtlAsciiNextDay_MatchSunDay,
    mtlAsciiNextDay_MatchMonDay,
    mtlAsciiNextDay_MatchTueDay,
    mtlAsciiNextDay_MatchWedDay,
    mtlAsciiNextDay_MatchThuDay,
    mtlAsciiNextDay_MatchFriDay,
    mtlAsciiNextDay_MatchSatDay
};


mtlModule mtclAscii = {
    mtlNames,
    MTL_ASCII_ID,
    mtlAsciiNextChar,
    mtlAsciiMaxPrecision,
    &mtclAsciiExtractSet,
    &mtclAsciiNextDaySet,
    mtcl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlAsciiNextChar( acp_uint8_t ** aSource, acp_uint8_t * aTemp )
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
    
    ACP_UNUSED(aTemp);
    
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

static acp_sint32_t mtlAsciiMaxPrecision( acp_sint32_t aLength )
{
/***********************************************************************
x *
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

static acp_sint32_t mtlAsciiExtract_MatchCentury( acp_uint8_t* aSource,
                                                  acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource,aSourceLen,"CENTURY", 7 );
}

static acp_sint32_t mtlAsciiExtract_MatchYear( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "YEAR", 4 );
}

static acp_sint32_t mtlAsciiExtract_MatchQuarter( acp_uint8_t* aSource,
                                                  acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource,aSourceLen, "QUARTER", 7 );
}

static acp_sint32_t mtlAsciiExtract_MatchMonth( acp_uint8_t* aSource,
                                                acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MONTH", 5 );
}

static acp_sint32_t mtlAsciiExtract_MatchWeek( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WEEK", 4 );
}

static acp_sint32_t mtlAsciiExtract_MatchWeekOfMonth( acp_uint8_t* aSource,
                                                      acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WEEKOFMONTH", 11 );
}

static acp_sint32_t mtlAsciiExtract_MatchDay( acp_uint8_t* aSource,
                                              acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "DAY", 3 );
}

static acp_sint32_t mtlAsciiExtract_MatchDayOfYear( acp_uint8_t* aSource,
                                                    acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "DAYOFYEAR", 9 );
}

static acp_sint32_t mtlAsciiExtract_MatchDayOfWeek( acp_uint8_t* aSource,
                                                    acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "DAYOFWEEK", 9 );
}

static acp_sint32_t mtlAsciiExtract_MatchHour( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "HOUR", 4 );
}

static acp_sint32_t mtlAsciiExtract_MatchMinute( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MINUTE", 6 );
}

static acp_sint32_t mtlAsciiExtract_MatchSecond( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SECOND", 6 );
}

static acp_sint32_t mtlAsciiExtract_MatchMicroSec( acp_uint8_t* aSource,
                                                   acp_uint32_t aSourceLen )
{
    return mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MICROSECOND", 11 );
}

static acp_sint32_t mtlAsciiNextDay_MatchSunDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SUNDAY", 6 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SUN", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static acp_sint32_t mtlAsciiNextDay_MatchMonDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MONDAY", 6 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MON", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static acp_sint32_t mtlAsciiNextDay_MatchTueDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "TUESDAY", 7 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "TUE", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static acp_sint32_t mtlAsciiNextDay_MatchWedDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WEDNESDAY", 9 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WED", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static acp_sint32_t mtlAsciiNextDay_MatchThuDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "THURSDAY", 8 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "THU", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static acp_sint32_t mtlAsciiNextDay_MatchFriDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "FRIDAY", 6 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "FRI", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

static acp_sint32_t mtlAsciiNextDay_MatchSatDay( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SATURDAY", 8 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SAT", 3 ) == 0 )
    {
        return 0;
    }
    return 1;
}

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
static acp_sint32_t mtlAsciiExtract_MatchISOWeek( acp_uint8_t  * aSource,
                                                  acp_uint32_t   aSourceLen )
{
    if ( mtcStrCaselessMatch( (acp_char_t*) aSource, aSourceLen, "IW", 2 ) == 0  )
    {
        return 0;
    }

    return 1;
}

