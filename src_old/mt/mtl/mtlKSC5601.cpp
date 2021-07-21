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
 * $Id: mtlKSC5601.cpp 82146 2018-01-29 06:47:57Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtl.h>
#include <mtdTypes.h>

//----------------------------------
// EUC-KR Encoding : KS_C_5601 - 1987
//----------------------------------

#define mtlKSC5601_1BYTE_TYPE(c)       ( (c)<=0x7F )
    
#define mtlKSC5601_2BYTE_TYPE(c)       ( (c)>=0xA1 && (c)<=0xFE )

#define mtlKSC5601_MULTI_BYTE_TAIL(c)  ( (c)>=0xA1 && (c)<=0xFE )

extern mtlModule mtlKSC5601;

extern UChar * mtl1BYTESpecialCharSet[];

// PROJ-1755
static mtlNCRet mtlKSC5601NextChar( UChar ** aSource, UChar * aFence );

static SInt mtlKSC5601MaxPrecision( SInt aLength );

static mtcName mtlNames[3] = {
    { mtlNames+1,  7, (void*)"KSC5601"     },
    { mtlNames+2, 11, (void*)"KO16KSC5601" },
    { NULL      ,  6, (void*)"KOREAN"      }
};

static SInt mtlKSC5601Extract_MatchCentury( UChar* aSource,
                                            UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchYear( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchQuarter( UChar* aSource,
                                            UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchMonth( UChar* aSource,
                                          UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchWeek( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchWeekOfMonth( UChar* aSource,
                                                UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchDay( UChar* aSource,
                                        UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchDayOfYear( UChar* aSource,
                                              UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchDayOfWeek( UChar* aSource,
                                              UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchHour( UChar* aSource,
                                         UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchMinute( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchSecond( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601Extract_MatchMicroSec( UChar* aSource,
                                             UInt   aSourceLen );

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
static SInt mtlKSC5601Extract_MatchISOWeek( UChar * aSource,
                                            UInt    aSourceLen );

mtlExtractFuncSet mtlKSC5601ExtractSet = {
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

static SInt mtlKSC5601NextDay_MatchSunDay( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601NextDay_MatchMonDay( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601NextDay_MatchTueDay( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601NextDay_MatchWedDay( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601NextDay_MatchThuDay( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601NextDay_MatchFriDay( UChar* aSource,
                                           UInt   aSourceLen );

static SInt mtlKSC5601NextDay_MatchSatDay( UChar* aSource,
                                           UInt   aSourceLen );


mtlNextDayFuncSet mtlKSC5601NextDaySet = {
    mtlKSC5601NextDay_MatchSunDay,
    mtlKSC5601NextDay_MatchMonDay,
    mtlKSC5601NextDay_MatchTueDay,
    mtlKSC5601NextDay_MatchWedDay,
    mtlKSC5601NextDay_MatchThuDay,
    mtlKSC5601NextDay_MatchFriDay,
    mtlKSC5601NextDay_MatchSatDay
};

mtlModule mtlKSC5601 = {
    mtlNames,
    MTL_KSC5601_ID,
    mtlKSC5601NextChar,
    mtlKSC5601MaxPrecision,
    &mtlKSC5601ExtractSet,
    &mtlKSC5601NextDaySet,
    mtl1BYTESpecialCharSet,
    1
};

mtlNCRet mtlKSC5601NextChar( UChar ** aSource, UChar * aFence )
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

static SInt mtlKSC5601MaxPrecision( SInt aLength )
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

SInt mtlKSC5601Extract_MatchCentury( UChar* aSource,
                                     UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "CENTURY", 7 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "세기", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "SCC", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "CC",2 ) == 0 ) 
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchYear( UChar* aSource,
                                  UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "YEAR", 4 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "년", 2 ) == 0 ||
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

SInt mtlKSC5601Extract_MatchQuarter( UChar* aSource,
                                     UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "QUARTER", 7 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "분기", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "Q", 1 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchMonth( UChar* aSource,
                                   UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "MONTH", 5 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "월", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "MON", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "MM", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "RM", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchWeek( UChar* aSource,
                                  UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "WEEK", 4 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "주", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "WW", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchWeekOfMonth( UChar* aSource,
                                         UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "WEEKOFMONTH", 11 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "월중주", 6 ) == 0  ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "W", 1 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchDay( UChar* aSource,
                                 UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "DAY", 3 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "일", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "DDD", 3 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "DD", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "J", 1 ) == 0  )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchDayOfYear( UChar* aSource,
                                       UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "DAYOFYEAR", 9 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "년중일", 6 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchDayOfWeek( UChar* aSource,
                                       UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "DAYOFWEEK", 9 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "주중일", 6 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "DY", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "D", 1 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchHour( UChar* aSource,
                                  UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "HOUR", 4 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "시", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "HH", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "HH12", 4 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "HH24", 4 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchMinute( UChar* aSource,
                                    UInt   aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "MINUTE", 6 ) == 0 ||
         idlOS::strMatch( aSource, aSourceLen, "분", 2 ) == 0 ||
         idlOS::strCaselessMatch( aSource, aSourceLen, "MI", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchSecond( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "SECOND", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "초", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601Extract_MatchMicroSec( UChar* aSource,
                                      UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "MICROSECOND", 11 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "마이크로초", 10 ) == 0 )
    {
    
        return 0;
    }
    return 1;
}

SInt mtlKSC5601NextDay_MatchSunDay( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "SUNDAY", 6 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "SUN", 3 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "일요일", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "일", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601NextDay_MatchMonDay( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "MONDAY", 6 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "MON", 3 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "월요일", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "월", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601NextDay_MatchTueDay( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "TUESDAY", 7 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "TUE", 3 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "화요일", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "화", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601NextDay_MatchWedDay( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "WEDNESDAY", 9 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "WED", 3 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "수요일", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "수", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601NextDay_MatchThuDay( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "THURSDAY", 8 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "THU", 3 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "목요일", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "목", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}


SInt mtlKSC5601NextDay_MatchFriDay( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "FRIDAY", 6 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "FRI", 3 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "금요일", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "금", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

SInt mtlKSC5601NextDay_MatchSatDay( UChar* aSource,
                                    UInt   aSourceLen )
{
    if( idlOS::strCaselessMatch( aSource, aSourceLen, "SATURDAY", 8 ) == 0 ||
        idlOS::strCaselessMatch( aSource, aSourceLen, "SAT", 3 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "토요일", 6 ) == 0 ||
        idlOS::strMatch( aSource, aSourceLen, "토", 2 ) == 0 )
    {
        return 0;
    }
    return 1;
}

/* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
SInt mtlKSC5601Extract_MatchISOWeek( UChar * aSource,
                                     UInt    aSourceLen )
{
    if ( idlOS::strCaselessMatch( aSource, aSourceLen, "IW", 2 ) == 0 )
    {
        return 0;
    }

    return 1;
}
