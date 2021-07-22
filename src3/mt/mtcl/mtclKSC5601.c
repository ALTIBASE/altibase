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
 * $Id: mtlKSC5601.cpp 32401 2009-04-29 06:01:36Z mhjeong $
 **********************************************************************/

#include <mtce.h>
#include <mtcl.h>
#include <mtcdTypes.h>

//----------------------------------
// EUC-KR Encoding : KS_C_5601 - 1987
//----------------------------------

#define mtlKSC5601_1BYTE_TYPE(c)       ( (c)<=0x7F )

#define mtlKSC5601_2BYTE_TYPE(c)       ( (c)>=0xA1 && (c)<=0xFE )

#define mtlKSC5601_MULTI_BYTE_TAIL(c)  ( (c)>=0xA1 && (c)<=0xFE )

extern mtlModule mtclKSC5601;

extern acp_uint8_t * mtcl1BYTESpecialCharSet[];

// PROJ-1755
static mtlNCRet mtlKSC5601NextChar( acp_uint8_t ** aSource, acp_uint8_t * aFence );

static acp_sint32_t mtlKSC5601MaxPrecision( acp_sint32_t aLength );

static mtcName mtlNames[3] = {
    { mtlNames+1,  7, (void*)"KSC5601"     },
    { mtlNames+2, 11, (void*)"KO16KSC5601" },
    { NULL      ,  6, (void*)"KOREAN"      }
};

static acp_sint32_t mtlKSC5601Extract_MatchCentury( acp_uint8_t* aSource,
                                                    acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchYear( acp_uint8_t* aSource,
                                                 acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchQuarter( acp_uint8_t* aSource,
                                                    acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchMonth( acp_uint8_t* aSource,
                                                  acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchWeek( acp_uint8_t* aSource,
                                                 acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchWeekOfMonth( acp_uint8_t* aSource,
                                                        acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchDay( acp_uint8_t* aSource,
                                                acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchDayOfYear( acp_uint8_t* aSource,
                                                      acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchDayOfWeek( acp_uint8_t* aSource,
                                                      acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchHour( acp_uint8_t* aSource,
                                                 acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchMinute( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchSecond( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601Extract_MatchMicroSec( acp_uint8_t* aSource,
                                                     acp_uint32_t   aSourceLen );

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
static acp_sint32_t mtlKSC5601Extract_MatchISOWeek( acp_uint8_t  * aSource,
                                                    acp_uint32_t   aSourceLen );

mtlExtractFuncSet mtclKSC5601ExtractSet = {
    mtlKSC5601Extract_MatchCentury,
    mtlKSC5601Extract_MatchYear,
    mtlKSC5601Extract_MatchQuarter,
    mtlKSC5601Extract_MatchMonth,
    mtlKSC5601Extract_MatchWeek,
    mtlKSC5601Extract_MatchWeekOfMonth,
    mtlKSC5601Extract_MatchDay,
    mtlKSC5601Extract_MatchDayOfYear,
    mtlKSC5601Extract_MatchDayOfWeek,
    mtlKSC5601Extract_MatchHour,
    mtlKSC5601Extract_MatchMinute,
    mtlKSC5601Extract_MatchSecond,
    mtlKSC5601Extract_MatchMicroSec,
    /* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
    mtlKSC5601Extract_MatchISOWeek
};

static acp_sint32_t mtlKSC5601NextDay_MatchSunDay( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601NextDay_MatchMonDay( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601NextDay_MatchTueDay( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601NextDay_MatchWedDay( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601NextDay_MatchThuDay( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601NextDay_MatchFriDay( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );

static acp_sint32_t mtlKSC5601NextDay_MatchSatDay( acp_uint8_t* aSource,
                                                   acp_uint32_t   aSourceLen );


mtlNextDayFuncSet mtclKSC5601NextDaySet = {
    mtlKSC5601NextDay_MatchSunDay,
    mtlKSC5601NextDay_MatchMonDay,
    mtlKSC5601NextDay_MatchTueDay,
    mtlKSC5601NextDay_MatchWedDay,
    mtlKSC5601NextDay_MatchThuDay,
    mtlKSC5601NextDay_MatchFriDay,
    mtlKSC5601NextDay_MatchSatDay
};

mtlModule mtclKSC5601 = {
    mtlNames,
    MTL_KSC5601_ID,
    mtlKSC5601NextChar,
    mtlKSC5601MaxPrecision,
    &mtclKSC5601ExtractSet,
    &mtclKSC5601NextDaySet,
    mtcl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlKSC5601NextChar( acp_uint8_t ** aSource, acp_uint8_t * aFence )
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
    
    if ( mtlKSC5601_1BYTE_TYPE( *(*aSource) ) )
    {
        sRet = NC_VALID;
        (*aSource)++;
    }
    else if ( mtlKSC5601_2BYTE_TYPE( *(*aSource) ) )
    {
        if( aFence - *aSource > 1 )
        {
            if( mtlKSC5601_MULTI_BYTE_TAIL( *(*aSource+1) ) )
            {
                sRet = NC_VALID;
                (*aSource) += 2;
            }
            else
            {
                sRet = NC_MB_INVALID;
                (*aSource) += 2;
            }
        }
        else
        {
            sRet = NC_MB_INCOMPLETED;
            *aSource = aFence;
        }
    }
    else
    {
        sRet = NC_INVALID;
        (*aSource)++;
    }

    return sRet;
}

static acp_sint32_t mtlKSC5601MaxPrecision( acp_sint32_t aLength )
{
/***********************************************************************
 *
 * Description : 문자갯수(aLength)의 KSC5601의 최대 precision 계산
 *
 * Implementation :
 *
 *    인자로 받은 aLength에
 *    KSC5601 한문자의 최대 크기를 곱한 값을 리턴함.
 *
 *    aLength는 문자갯수의 의미가 있음.
 *
 ***********************************************************************/
    return aLength * MTL_KSC5601_PRECISION;
}

acp_sint32_t mtlKSC5601Extract_MatchCentury( acp_uint8_t* aSource,
                                             acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "CENTURY", 7 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "세기", 4 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchYear( acp_uint8_t* aSource,
                                          acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "YEAR", 4 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "년", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchQuarter( acp_uint8_t* aSource,
                                             acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "QUARTER", 7 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "분기", 4 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchMonth( acp_uint8_t* aSource,
                                           acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MONTH", 5 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "월", 2) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchWeek( acp_uint8_t* aSource,
                                          acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WEEK", 4 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "주", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchWeekOfMonth( acp_uint8_t* aSource,
                                                 acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WEEKOFMONTH", 11 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "월중주", 6 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchDay( acp_uint8_t* aSource,
                                         acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen,  "DAY", 3 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "일", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchDayOfYear( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "DAYOFYEAR", 9 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "년중일", 6 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchDayOfWeek( acp_uint8_t* aSource,
                                               acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "DAYOFWEEK", 9 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "주중일", 6 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchHour( acp_uint8_t* aSource,
                                          acp_uint32_t   aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "HOUR", 4 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "시", 2  ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchMinute( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MINUTE", 6 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "분", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchSecond( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SECOND", 6 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "초", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601Extract_MatchMicroSec( acp_uint8_t* aSource,
                                              acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MICROSECOND", 11 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "마이크로초", 10 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601NextDay_MatchSunDay( acp_uint8_t* aSource,
                                            acp_uint32_t   aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SUNDAY", 6 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SUN", 3 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "일요일", 6 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "일", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601NextDay_MatchMonDay( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MONDAY", 6 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "MON", 3 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "월요일", 6 ) == 0 ||
        mtcStrMatch((acp_char_t*)aSource, aSourceLen, "월", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601NextDay_MatchTueDay( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "TUESDAY", 7 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "TUE", 3 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "화요일", 6 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "화", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601NextDay_MatchWedDay( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WEDNESDAY", 9 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "WED", 3 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "수요일", 6 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "수", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601NextDay_MatchThuDay( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "THURSDAY", 8 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "THU", 3 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "목요일", 6 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "목", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}


acp_sint32_t mtlKSC5601NextDay_MatchFriDay( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "FRIDAY", 6 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "FRI", 3 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "금요일", 6 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "금", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

acp_sint32_t mtlKSC5601NextDay_MatchSatDay( acp_uint8_t* aSource,
                                            acp_uint32_t aSourceLen )
{
    if( mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SATURDAY", 8 ) == 0 ||
        mtcStrCaselessMatch((acp_char_t*) aSource, aSourceLen, "SAT", 3 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "토요일", 6 ) == 0 ||
          mtcStrMatch((acp_char_t*)aSource, aSourceLen, "토", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
acp_sint32_t mtlKSC5601Extract_MatchISOWeek( acp_uint8_t  * aSource,
                                             acp_uint32_t   aSourceLen )
{
    if ( mtcStrCaselessMatch( (acp_char_t*) aSource, aSourceLen, "IW", 2 ) == 0 )
    {
        return 0;
    }

    return 1;
}
