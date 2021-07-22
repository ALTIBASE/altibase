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
 * $Id: mtfTo_char.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>
#include <mtdTypes.h>
#include <mtddlLexer.h>
#include <mtlTerritory.h>

extern mtfModule mtfTo_char;

extern mtdModule mtdDate;
extern mtdModule mtdNumeric;
extern mtdModule mtdFloat;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdByte;
extern mtdModule mtdVarbyte;
extern mtdModule mtdNibble;
extern mtdModule mtdBit;
extern mtdModule mtdVarbit;
extern mtdModule mtdClob;
extern mtdModule mtdClobLocator;

static mtcName mtfTo_charFunctionName[1] = {
    { NULL, 7, (void*)"TO_CHAR" }
};

static IDE_RC mtfTo_charEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

static IDE_RC compXXXXandRN( UChar  * aNumFmt,
                             UInt     aNumFmtLen,
                             UChar  * aResultValue,
                             UShort * aResultLen,
                             SInt     aIntNum );

static IDE_RC convertToRoman( SInt     aIntNum,
                              UShort * aRNCharCnt,
                              SChar  * aTemp );

static IDE_RC checkFormat( UChar * aFmt,
                           UInt    aLength,
                           UChar * aToken );

static IDE_RC convertToString( SInt    aLength,
                               SInt    aSignExp,
                               UChar * aMantissa,
                               SChar * aTemp,
                               SInt  * aTempLen );

static IDE_RC applyFormat( SChar       * aString,
                           SInt          aStringLen,
                           UChar       * aFormat,
                           UInt          aFormatLen,
                           UChar       * aResult,
                           UInt        * aResultLen,
                           UChar       * aToken,
                           mtlCurrency * aCurrency,
                           idBool        aIsMinus );

// EEEE format이 없을 경우, format 적용
static IDE_RC applyNonEEEEFormat( SChar       * aString,
                                  SInt          aStringLen,
                                  UChar       * aResult,
                                  UInt        * aResultLen,
                                  UChar       * aToken,
                                  mtlCurrency * aCurrency,
                                  idBool        aIsMinus );

// EEEE format이 있을 경우, format 적용
static IDE_RC applyEEEEFormat( SChar       * aString,
                               SInt          aStringLen,
                               UChar       * aResult,
                               UInt        * aResultLen,
                               UChar       * aToken,
                               mtlCurrency * aCurrency,
                               idBool        aIsMinus );

static IDE_RC removeLeadingBlank( SChar  * aResult,
                                  UInt     aResultLen,
                                  UInt   * aBlankCnt );

inline UInt getIntegerLength( UInt aInteger )
{
    if( aInteger < 10 )
    {
        return 1;
    }
    else if( aInteger < 100 )
    {
        return 2;
    }
    else if( aInteger < 1000 )
    {
        return 3;
    }
    else if( aInteger < 10000 )
    {
        return 4;
    }
    else if( aInteger < 100000 )
    {
        return 5;
    }
    else if( aInteger < 1000000 )
    {
        return 6;
    }
    else if( aInteger < 10000000 )
    {
        return 7;
    }
    else if( aInteger < 100000000 )
    {
        return 8;
    }
    else if( aInteger < 1000000000 )
    {
        return 9;
    }
    else
    {
        return 10;
    }
}

inline UInt mtfFastUInt2Str( SChar * aBuffer,
                             UInt    aValue,
                             UInt    aLength )
{
    SChar * sBuffer = aBuffer;
    UInt    sHighValue;
    UInt    sLowValue;

    switch( aLength )
    {
    case 1:
        *sBuffer++ = '0' + ((aValue = aValue * 268435456) >> 28);
        break;

    case 2:
        *sBuffer++ = '0' + ((aValue = aValue * 26843546) >> 28);
        *sBuffer++ = '0' + ((aValue = (aValue & 0x0FFFFFFF) * 10) >> 28);
        break;

    case 3:
        *sBuffer++ = '0' + ((aValue = aValue * 2684355) >> 28);
        *sBuffer++ = '0' + ((aValue = (aValue & 0x0FFFFFFF) * 10) >> 28);
        *sBuffer++ = '0' + ((aValue = (aValue & 0x0FFFFFFF) * 10) >> 28);
        break;

    case 4:
        *sBuffer++ = '0' + ((aValue = aValue * 268436) >> 28);
        *sBuffer++ = '0' + ((aValue = (aValue & 0x0FFFFFFF) * 10) >> 28);
        *sBuffer++ = '0' + ((aValue = (aValue & 0x0FFFFFFF) * 10) >> 28);
        *sBuffer++ = '0' + ((aValue = (aValue & 0x0FFFFFFF) * 10) >> 28);
        break;

    case 5:
        sHighValue = aValue / 10000;
        sLowValue  = aValue % 10000;
        *sBuffer++ = '0' + ((sHighValue = sHighValue * 268435456) >> 28);
        *sBuffer++ = '0' + ((sLowValue = sLowValue * 268436) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        break;

    case 6:
        sHighValue = aValue / 10000;
        sLowValue  = aValue % 10000;
        *sBuffer++ = '0' + ((sHighValue = sHighValue * 26843546) >> 28);
        *sBuffer++ = '0' + ((sHighValue = ((sHighValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = sLowValue * 268436) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        break;

    case 7:
        sHighValue = aValue / 10000;
        sLowValue  = aValue % 10000;
        *sBuffer++ = '0' + ((sHighValue = sHighValue * 2684355) >> 28);
        *sBuffer++ = '0' + ((sHighValue = ((sHighValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sHighValue = ((sHighValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = sLowValue * 268436) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        break;

    case 8:
        sHighValue = aValue / 10000;
        sLowValue  = aValue % 10000;
        *sBuffer++ = '0' + ((sHighValue = sHighValue * 268436) >> 28);
        *sBuffer++ = '0' + ((sHighValue = ((sHighValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sHighValue = ((sHighValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sHighValue = ((sHighValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = sLowValue * 268436) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        *sBuffer++ = '0' + ((sLowValue = ((sLowValue & 0x0FFFFFFF)) * 10) >> 28);
        break;

    default:
        aLength = idlOS::snprintf( sBuffer, aLength, "%"ID_UINT32_FMT, aValue );
    }

    return aLength;
}

typedef struct mtfNumberInfo
{
    UChar          sToken[MTD_NUMBER_MAX];
    mtlCurrency    sCurrency;
} mtfNumberInfo;

mtfModule mtfTo_char = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfTo_charFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfTo_charEstimate
};

static IDE_RC mtfTo_charCalculateFor1Arg( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

// PROJ-1579 NCHAR
static IDE_RC mtfTo_charCalculateNcharFor1Arg( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_charCalculateFor1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_charCalculateNcharFor1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNumberFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfToCharInterface::mtfTo_charCalculateNumberFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteDateFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfToCharInterface::mtfTo_charCalculateDateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static
    const char* gMONTHName[12] = {
        "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
        "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };
static
    const char* gMonthName[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
static
    const char* gmonthName[12] = {
        "january", "february", "march", "april", "may", "june",
        "july", "august", "september", "october", "november", "december"
    };

static
    const char* gMONName[12] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
static
    const char* gMonName[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
static
    const char* gmonName[12] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };

static
    const char* gDAYName[7] = {
        "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
        "THURSDAY", "FRIDAY", "SATURDAY"
    };
static
    const char* gDayName[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };
static
    const char* gdayName[7] = {
        "sunday", "monday", "tuesday", "wednesday",
        "thursday", "friday", "saturday"
    };

static
    const char* gDYName[7] = {
        "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
    };
static
    const char* gDyName[7] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
static
    const char* gdyName[7] = {
        "sun", "mon", "tue", "wed", "thu", "fri", "sat"
    };

static
    const char* gRMMonth[12] = {
        "I", "II", "III", "IV", "V", "VI",
        "VII", "VIII", "IX", "X", "XI", "XII"
    };
static
    const char* grmMonth[12] = {
        "i", "ii", "iii", "iv", "v", "vi",
        "vii", "viii", "ix", "x", "xi", "xii"
    };

static IDE_RC applyNONEFormat( mtdDateType* /* aDate */,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       aString,
                               idBool       /* aIsFillMode */ )
{
    SInt sLen;
    SInt sBufferLen;
    sLen = idlOS::strlen((SChar*)aString);

    if ( sLen < (*aBufferFence)-(*aBufferCur) )
    {
        sBufferLen = sLen;
    } 
    else
    {
        sBufferLen = (*aBufferFence)-(*aBufferCur);
    }

    idlOS::strncpy( aBuffer+(*aBufferCur), 
                    aString,
                    sBufferLen );

    (*aBufferCur) += sBufferLen;

    return IDE_SUCCESS;
}

static IDE_RC applyAM_UFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    SInt sHour   = mtdDateInterface::hour( aDate );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

    if ( sHour < 12 )
    {
        aBuffer[(*aBufferCur)++] = 'A';
        aBuffer[(*aBufferCur)++] = 'M';
    }
    else
    {
        aBuffer[(*aBufferCur)++] = 'P';
        aBuffer[(*aBufferCur)++] = 'M';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyAM_ULFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       /* aIsFillMode */ )
{
    SInt sHour = mtdDateInterface::hour( aDate );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

    if ( sHour < 12 )
    {
        aBuffer[(*aBufferCur)++] = 'A';
        aBuffer[(*aBufferCur)++] = 'm';
    }
    else
    {
        aBuffer[(*aBufferCur)++] = 'P';
        aBuffer[(*aBufferCur)++] = 'm';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyAM_LFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    SInt sHour = mtdDateInterface::hour( aDate );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

    if ( sHour < 12 )
    {
        aBuffer[(*aBufferCur)++] = 'a';
        aBuffer[(*aBufferCur)++] = 'm';
    }
    else
    {
        aBuffer[(*aBufferCur)++] = 'p';
        aBuffer[(*aBufferCur)++] = 'm';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-36296 SCC Format 지원 */
static IDE_RC applySCCFormat( mtdDateType * aDate,
                              SChar       * aBuffer,
                              SInt        * aBufferCur,
                              SInt        * aBufferFence,
                              SChar       * /* aString */,
                              idBool        aIsFillMode )
{
    SShort sYear     = mtdDateInterface::year( aDate );
    UInt   sValue    = 0;
    UInt   sValueLen = 0;

    IDE_TEST_RAISE( (*aBufferFence) - (*aBufferCur) < 1,
                    ERR_BUFFER_NOT_ENOUGH );

    if ( aIsFillMode == ID_FALSE )
    {
        if ( sYear <= 0 )
        {
            sValue = ( ( abs( sYear - 1 ) + 99 ) / 100 ) % 100; // ceil

            aBuffer[(*aBufferCur)++] = '-';
        }
        else
        {
            sValue = ( ( sYear + 99 ) / 100 ) % 100; // ceil

            aBuffer[(*aBufferCur)++] = ' ';
        }

        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          2 );
    }
    else
    {
        if ( sYear <= 0 )
        {
            sValue = ( ( abs( sYear - 1 ) + 99 ) / 100 ) % 100; // ceil

            aBuffer[(*aBufferCur)++] = '-';
        }
        else
        {
            sValue = ( ( sYear + 99 ) / 100 ) % 100; // ceil
        }

        sValueLen = getIntegerLength( sValue );

        IDE_TEST( (*aBufferFence) - (*aBufferCur) < (SInt)sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BUFFER_NOT_ENOUGH )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "applySCCFormat",
                                  "Buffer not enough" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyCCFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    SShort sYear  = mtdDateInterface::year( aDate );
    UInt   sValue = 0;
    UInt   sValueLen;

    if ( sYear <= 0 )
    {
        /* Year 0은 BC -1년이다. 절대값을 구하기 전에 보정한다.
         * 음수일 때, 부호를 제거한다. (Oracle)
         */
        sValue = ( ( abs( sYear - 1 ) + 99 ) / 100 ) % 100; // ceil
    }
    else
    {
        sValue = ( ( sYear + 99 ) / 100 ) % 100; // ceil
    }

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyDAY_UFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       /* aIsFillMode */ )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gDAYName[mtc::dayOfWeek( sYear, sMonth, sDay )] );

    return IDE_SUCCESS;
}

static IDE_RC applyDAY_ULFormat( mtdDateType* aDate,
                                 SChar*       aBuffer,
                                 SInt*        aBufferCur,
                                 SInt*        aBufferFence,
                                 SChar*       /* aString */,
                                 idBool       /* aIsFillMode */ )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gDayName[mtc::dayOfWeek( sYear, sMonth, sDay )] );

    return IDE_SUCCESS;
}
static IDE_RC applyDAY_LFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       /* aIsFillMode */ )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gdayName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
    return IDE_SUCCESS;
}

static IDE_RC applyDDDFormat( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    SShort       sYear      = mtdDateInterface::year( aDate );
    UChar        sMonth     = mtdDateInterface::month( aDate );
    SInt         sDay       = mtdDateInterface::day( aDate );

    SInt         sDayOfYear = mtc::dayOfYear( sYear, sMonth, sDay );

    UInt         sValue     = sDayOfYear;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 3 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 3 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyDDFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    SInt sDay    = mtdDateInterface::day( aDate );

    UInt sValue  = sDay;
    UInt sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );
        
        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyDY_UFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gDYName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
    return IDE_SUCCESS;
}

static IDE_RC applyDY_ULFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       /* aIsFillMode */ )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gDyName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
    return IDE_SUCCESS;
}

static IDE_RC applyDY_LFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gdyName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
    return IDE_SUCCESS;
}

static IDE_RC applyDFormat( mtdDateType* aDate,
                            SChar*       aBuffer,
                            SInt*        aBufferCur,
                            SInt*        aBufferFence,
                            SChar*       /* aString */,
                            idBool       /* aIsFillMode */ )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%1"ID_INT32_FMT,
                                      mtc::dayOfWeek( sYear, sMonth, sDay ) + 1 );
    return IDE_SUCCESS;
}

static IDE_RC applyFFFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );
    UInt         sValue = sMicro;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 6 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 6 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyFF1Format( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );
    UInt         sValue = sMicro / 100000;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 1 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 1 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );
        
        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyFF2Format( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );
    UInt         sValue = sMicro / 10000;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyFF3Format( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );
    UInt         sValue = sMicro / 1000;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 3 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 3 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyFF4Format( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );
    UInt         sValue = sMicro / 100;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 4 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 4 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyFF5Format( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );
    UInt         sValue = sMicro / 10;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 5 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 5 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyFF6Format( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );
    UInt         sValue = sMicro;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 6 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 6 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyHHFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    SInt         sHour = mtdDateInterface::hour( aDate );

    UInt         sValue = sHour;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2);
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyHH12Format( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       aIsFillMode )
{
    SInt         sHour = mtdDateInterface::hour( aDate );
    UInt         sValue = sHour % 12;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        if ( sHour == 0 || sHour == 12 )
        {
            IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 12, 2 );
        }
        else
        {
            IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValue, 
                                              2 );
        }
    }
    else
    {
        if ( sHour == 0 || sHour == 12 )
        {
            IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 12, 2 );
        }
        else
        {
            sValueLen      = getIntegerLength( sValue );

            IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValue, 
                                              sValueLen );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyHH24Format( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       aIsFillMode )
{
    SInt         sHour = mtdDateInterface::hour( aDate );
    UInt         sValue = sHour;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyMIFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    SInt         sMin = mtdDateInterface::minute( aDate );

    UInt         sValue = sMin;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyMMFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    UInt         sValue = sMonth;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyMON_UFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gMONName[sMonth-1] );
    return IDE_SUCCESS;
}

static IDE_RC applyMON_ULFormat( mtdDateType* aDate,
                                 SChar*       aBuffer,
                                 SInt*        aBufferCur,
                                 SInt*        aBufferFence,
                                 SChar*       /* aString */,
                                 idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gMonName[sMonth-1] );
    return IDE_SUCCESS;
}

static IDE_RC applyMON_LFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gmonName[sMonth-1] );
    return IDE_SUCCESS;
}

static IDE_RC applyMONTH_UFormat( mtdDateType* aDate,
                                  SChar*       aBuffer,
                                  SInt*        aBufferCur,
                                  SInt*        aBufferFence,
                                  SChar*       /* aString */,
                                  idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gMONTHName[sMonth-1] );
    return IDE_SUCCESS;
}

static IDE_RC applyMONTH_ULFormat( mtdDateType* aDate,
                                   SChar*       aBuffer,
                                   SInt*        aBufferCur,
                                   SInt*        aBufferFence,
                                   SChar*       /* aString */,
                                   idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gMonthName[sMonth-1] );
    return IDE_SUCCESS;
}

static IDE_RC applyMONTH_LFormat( mtdDateType* aDate,
                                  SChar*       aBuffer,
                                  SInt*        aBufferCur,
                                  SInt*        aBufferFence,
                                  SChar*       /* aString */,
                                  idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gmonthName[sMonth-1] );
    return IDE_SUCCESS;
}

static IDE_RC applyPM_UFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    SInt         sHour = mtdDateInterface::hour( aDate );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

    if ( sHour < 12 )
    {
        aBuffer[(*aBufferCur)++] = 'A';
        aBuffer[(*aBufferCur)++] = 'M';
    }
    else
    {
        aBuffer[(*aBufferCur)++] = 'P';
        aBuffer[(*aBufferCur)++] = 'M';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyPM_ULFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       /* aIsFillMode */ )
{
    SInt         sHour = mtdDateInterface::hour( aDate );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

    if ( sHour < 12 )
    {
        aBuffer[(*aBufferCur)++] = 'A';
        aBuffer[(*aBufferCur)++] = 'm';
    }
    else
    {
        aBuffer[(*aBufferCur)++] = 'P';
        aBuffer[(*aBufferCur)++] = 'm';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyPM_LFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    SInt         sHour = mtdDateInterface::hour( aDate );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

    if ( sHour < 12 )
    {
        aBuffer[(*aBufferCur)++] = 'a';
        aBuffer[(*aBufferCur)++] = 'm';
    }
    else
    {
        aBuffer[(*aBufferCur)++] = 'p';
        aBuffer[(*aBufferCur)++] = 'm';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyQFormat( mtdDateType* aDate,
                            SChar*       aBuffer,
                            SInt*        aBufferCur,
                            SInt*        aBufferFence,
                            SChar*       /* aString */,
                            idBool       /* aIsFillMode */ )
{
    UChar sMonth = mtdDateInterface::month( aDate );
    UInt  sValue = ( sMonth + 2 ) / 3; //ceil( (SDouble) sMonth / 3 );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 1 );

    (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyRM_UFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      gRMMonth[sMonth-1] );
    return IDE_SUCCESS;
}

static IDE_RC applyRM_LFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       /* aIsFillMode */ )
{
    UChar        sMonth = mtdDateInterface::month( aDate );

    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      grmMonth[sMonth-1] );
    return IDE_SUCCESS;
}

/* BUG-36296 SYYYY Format 지원 */
static IDE_RC applySYYYYFormat( mtdDateType * aDate,
                                SChar       * aBuffer,
                                SInt        * aBufferCur,
                                SInt        * aBufferFence,
                                SChar       * /* aString */,
                                idBool        aIsFillMode )
{
    SShort sYear     = mtdDateInterface::year( aDate );
    UInt   sValue    = 0;
    UInt   sValueLen = 0;

    IDE_TEST_RAISE( (*aBufferFence) - (*aBufferCur) < 1,
                    ERR_BUFFER_NOT_ENOUGH );

    if ( aIsFillMode == ID_FALSE )
    {
        if ( sYear < 0 )
        {
            sValue = abs( sYear ) % 10000;

            aBuffer[(*aBufferCur)++] = '-';
        }
        else
        {
            sValue = sYear % 10000;

            aBuffer[(*aBufferCur)++] = ' ';
        }

        IDE_TEST_RAISE( (*aBufferFence) - (*aBufferCur) < 4,
                        ERR_BUFFER_NOT_ENOUGH );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 4 );
    }
    else
    {
        if ( sYear < 0 )
        {
            sValue = abs( sYear ) % 10000;

            aBuffer[(*aBufferCur)++] = '-';
        }
        else
        {
            sValue = sYear % 10000;
        }

        sValueLen      = getIntegerLength( sValue );

        IDE_TEST_RAISE( (*aBufferFence) - (*aBufferCur) < (SInt)sValueLen,
                        ERR_BUFFER_NOT_ENOUGH );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BUFFER_NOT_ENOUGH )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "applySYYYYFormat",
                                  "Buffer not enough" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyYYYYFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       aIsFillMode )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UInt         sValue = 0;
    UInt         sValueLen;

    if ( sYear < 0 )
    {
        /* 음수일 때, 부호를 제거한다. (Oracle) */
        sValue = abs( sYear ) % 10000;
    }
    else
    {
        sValue = sYear % 10000;
    }

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 4 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 4 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyYYYFormat( mtdDateType* aDate,
                              SChar*       aBuffer,
                              SInt*        aBufferCur,
                              SInt*        aBufferFence,
                              SChar*       /* aString */,
                              idBool       aIsFillMode )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UInt         sValue = 0;
    UInt         sValueLen;

    if ( sYear < 0 )
    {
        /* 음수일 때, 부호를 제거한다. (Oracle) */
        sValue = abs( sYear ) % 1000;
    }
    else
    {
        sValue = sYear % 1000;
    }

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 3 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 3 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyYYFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UInt         sValue = 0;
    UInt         sValueLen;

    if ( sYear < 0 )
    {
        /* 음수일 때, 부호를 제거한다. (Oracle) */
        sValue = abs( sYear ) % 100;
    }
    else
    {
        sValue = sYear % 100;
    }

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyYFormat( mtdDateType* aDate,
                            SChar*       aBuffer,
                            SInt*        aBufferCur,
                            SInt*        aBufferFence,
                            SChar*       /* aString */,
                            idBool       aIsFillMode )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UInt         sValue = 0;
    UInt         sValueLen;

    if ( sYear < 0 )
    {
        /* 음수일 때, 부호를 제거한다. (Oracle) */
        sValue = abs( sYear ) % 10;
    }
    else
    {
        sValue = sYear % 10;
    }

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 1 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 1 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyRRRRFormat( mtdDateType* aDate,
                               SChar*       aBuffer,
                               SInt*        aBufferCur,
                               SInt*        aBufferFence,
                               SChar*       /* aString */,
                               idBool       aIsFillMode )
{
    return applyYYYYFormat( aDate,
                            aBuffer,
                            aBufferCur,
                            aBufferFence,
                            NULL,
                            aIsFillMode );
}

static IDE_RC applyRRFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    return applyYYFormat( aDate,
                          aBuffer,
                          aBufferCur,
                          aBufferFence,
                          NULL,
                          aIsFillMode );

    return IDE_SUCCESS;
}

static IDE_RC applySSSSSSSSFormat( mtdDateType* aDate,
                                   SChar*       aBuffer,
                                   SInt*        aBufferCur,
                                   SInt*        aBufferFence,
                                   SChar*       /* aString */,
                                   idBool       aIsFillMode )
{
    SInt         sSec = mtdDateInterface::second( aDate );
    UInt         sMicro = mtdDateInterface::microSecond( aDate );

    UInt         sValueSec = sSec;
    UInt         sValueMicro = sMicro;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < ( 2 + 6 ) );
        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValueSec, 2 );
        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValueMicro, 6 );
    }
    else
    {
        if( sSec != 0 )
        {
            sValueLen      = getIntegerLength( sValueSec );

            IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < ( sValueLen + 6 ) );

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValueSec, 
                                              sValueLen );
            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValueMicro, 
                                              6 );
        }
        else
        {
            sValueLen      = getIntegerLength( sValueMicro );

            IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValueMicro, 
                                              sValueLen );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applySSSSSSFormat( mtdDateType* aDate,
                                 SChar*       aBuffer,
                                 SInt*        aBufferCur,
                                 SInt*        aBufferFence,
                                 SChar*       /* aString */,
                                 idBool       aIsFillMode )
{
    UInt         sMicro = mtdDateInterface::microSecond( aDate );

    UInt         sValue = sMicro;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 6 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 6 );

    }
    else
    {
        sValueLen      = getIntegerLength( sValue );
        
        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applySSSSSFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       aIsFillMode )
{
    SInt         sHour = mtdDateInterface::hour( aDate );
    SInt         sMin = mtdDateInterface::minute( aDate );
    SInt         sSec = mtdDateInterface::second( aDate );

    UInt         sValue = ( sHour * 60 * 60 ) + ( sMin * 60 ) + sSec;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 5 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 5 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applySSFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    SInt         sSec = mtdDateInterface::second( aDate );

    UInt         sValue = sSec;
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyWWFormat( mtdDateType* aDate,
                             SChar*       aBuffer,
                             SInt*        aBufferCur,
                             SInt*        aBufferFence,
                             SChar*       /* aString */,
                             idBool       aIsFillMode )
{
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    UInt         sValue = mtc::weekOfYear( sYear, sMonth, sDay);
    UInt         sValueLen;

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen      = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen);

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                          sValue, 
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-42926 TO_CHAR()에 IW 추가 */
static IDE_RC applyIWFormat( mtdDateType * aDate,
                             SChar       * aBuffer,
                             SInt        * aBufferCur,
                             SInt        * aBufferFence,
                             SChar       * /* aString */,
                             idBool        aIsFillMode )
{
    SShort  sYear     = mtdDateInterface::year( aDate );
    UChar   sMonth    = mtdDateInterface::month( aDate );
    SInt    sDay      = mtdDateInterface::day( aDate );

    UInt    sValue    = mtc::weekOfYearForStandard( sYear, sMonth, sDay );
    UInt    sValueLen = 0;

    if ( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-42941 TO_CHAR()에 WW2(Oracle Version WW) 추가 */
static IDE_RC applyWW2Format( mtdDateType * aDate,
                              SChar       * aBuffer,
                              SInt        * aBufferCur,
                              SInt        * aBufferFence,
                              SChar       * /* aString */,
                              idBool        aIsFillMode )
{
    SShort  sYear     = mtdDateInterface::year( aDate );
    UChar   sMonth    = mtdDateInterface::month( aDate );
    SInt    sDay      = mtdDateInterface::day( aDate );

    UInt    sValue    = mtc::weekOfYearForOracle( sYear, sMonth, sDay );
    UInt    sValueLen = 0;

    if ( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyWFormat( mtdDateType* aDate,
                            SChar*       aBuffer,
                            SInt*        aBufferCur,
                            SInt*        aBufferFence,
                            SChar*       /* aString */,
                            idBool       /* aIsFillMode */ )
{
    UInt         sWeekOfMonth;
    SShort       sYear = mtdDateInterface::year( aDate );
    UChar        sMonth = mtdDateInterface::month( aDate );
    SInt         sDay = mtdDateInterface::day( aDate );

    IDE_TEST( (*aBufferFence) - (*aBufferCur) < 1 );

    sWeekOfMonth = (UInt) idlOS::ceil( (SDouble) ( sDay +
                                                   mtc::dayOfWeek( sYear, sMonth, 1 )
                                                   ) / 7 );

    (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sWeekOfMonth, 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC applyYCYYYFormat( mtdDateType* aDate,
                                SChar*       aBuffer,
                                SInt*        aBufferCur,
                                SInt*        aBufferFence,
                                SChar*       /* aString */,
                                idBool       aIsFillMode )
{
    SShort       sYear   = mtdDateInterface::year( aDate );
    UInt         sValue1 = 0;
    UInt         sValue2 = 0;
    UInt         sValueLen;

    if ( sYear < 0 )
    {
        /* 음수일 때, 부호를 제거한다. (Oracle) */
        sValue1 = ( abs( sYear ) / 1000 ) % 10;
        sValue2 = abs( sYear ) % 1000;
    }
    else
    {
        sValue1 = ( sYear / 1000 ) % 10;
        sValue2 = sYear % 1000;
    }

    if( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < ( 1 + 1 + 3 ) );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue1, 1 );
        aBuffer[(*aBufferCur)++] = ',';
        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue2, 3 );
    }
    else
    {
        if( sValue1 != 0 )
        {
            sValueLen      = getIntegerLength( sValue1 );

            IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < ( sValueLen + 1 + 3 ) );

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValue1, 
                                              sValueLen );

            aBuffer[(*aBufferCur)++] = ',';

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValue2, 
                                              3 );
        }
        else
        {
            sValueLen      = getIntegerLength( sValue2 );

            IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen );

            (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), 
                                              sValue2, 
                                              sValueLen );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


//static IDE_RC applyYYCYYFormat( mtdDateType* aDate,
//                                SChar*       aBuffer,
//                                SInt*        aBufferCur,
//                                SInt*        aBufferFence,
//                                SChar*       /* aString */,
//                                idBool       aIsFillMode )
//{
//    SShort       sYear = mtdDateInterface::year( aDate );
//
//    if( aIsFillMode == ID_FALSE )
//    {
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%02"ID_INT32_FMT,
//                                             sYear / 100 );
//
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%s",
//                                             "," );
//
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%02"ID_INT32_FMT,
//                                             sYear % 100 );
//    }
//    else
//    {
//        if( (SShort)(sYear / 100) != 0 )
//        {
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%"ID_INT32_FMT,
//                                                 sYear / 100 );
//
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%s",
//                                                 "," );
//
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%02"ID_INT32_FMT,
//                                                 sYear % 100 );
//        }
//        else
//        {
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%"ID_INT32_FMT,
//                                                 sYear % 100 );
//        }
//    }
//
//    return IDE_SUCCESS;
//}
//
//static IDE_RC applyYYYCYFormat( mtdDateType* aDate,
//                                SChar*       aBuffer,
//                                SInt*        aBufferCur,
//                                SInt*        aBufferFence,
//                                SChar*       /* aString */,
//                                idBool       aIsFillMode )
//{
//    SShort       sYear = mtdDateInterface::year( aDate );
//
//    if( aIsFillMode == ID_FALSE )
//    {
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%03"ID_INT32_FMT,
//                                             sYear / 10 );
//
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%s",
//                                             "," );
//
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%01"ID_INT32_FMT,
//                                             sYear % 10 );
//    }
//    else
//    {
//        if( (SShort)(sYear / 10) != 0 )
//        {
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%"ID_INT32_FMT,
//                                                 sYear / 10 );
//
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%s",
//                                                 "," );
//
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%01"ID_INT32_FMT,
//                                                 sYear % 10 );
//        }
//        else
//        {
//            (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                                 (*aBufferFence)-(*aBufferCur),
//                                                 "%"ID_INT32_FMT,
//                                                 sYear % 10 );
//        }
//    }
//
//    return IDE_SUCCESS;
//}
//
//static IDE_RC applyYYYYCFormat( mtdDateType* aDate,
//                                SChar*       aBuffer,
//                                SInt*        aBufferCur,
//                                SInt*        aBufferFence,
//                                SChar*       /* aString */,
//                                idBool       aIsFillMode )
//{
//    SShort       sYear = mtdDateInterface::year( aDate );
//
//    if( aIsFillMode == ID_FALSE )
//    {
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%04"ID_INT32_FMT,
//                                             sYear );
//    }
//    else
//    {
//        (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                             (*aBufferFence)-(*aBufferCur),
//                                             "%"ID_INT32_FMT,
//                                             sYear );
//    }
//
//    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
//                                         (*aBufferFence)-(*aBufferCur),
//                                         "%s",
//                                         "," );
//
//    return IDE_SUCCESS;
//}

static IDE_RC applyDOUBLE_QUOTE_STRINGFormat( mtdDateType* /* aDate */,
                                              SChar*       aBuffer,
                                              SInt*        aBufferCur,
                                              SInt*        aBufferFence,
                                              SChar*       aString,
                                              idBool       /* aIsFillMode */ )
{
    (*aBufferCur) += idlOS::snprintf( aBuffer+(*aBufferCur),
                                      (*aBufferFence)-(*aBufferCur),
                                      "%s",
                                      aString );
    return IDE_SUCCESS;
}

// To fix BUG-17693
static IDE_RC applyFMFormat( mtdDateType* /* aDate */,
                             SChar*       /* aBuffer */,
                             SInt*        /* aBufferCur */,
                             SInt*        /* aBufferFence */,
                             SChar*       /* aString */,
                             idBool       /* aIsFillMode */ )
{
    // Nothing to do

    return IDE_SUCCESS;
}

/* BUG-46727 TO_CHAR()에 IYYY 추가 */
static IDE_RC applyIYYYFormat( mtdDateType * aDate,
                               SChar       * aBuffer,
                               SInt        * aBufferCur,
                               SInt        * aBufferFence,
                               SChar       * /* aString */,
                               idBool        aIsFillMode )
{
    SShort  sYear     = mtdDateInterface::year( aDate );
    UChar   sMonth    = mtdDateInterface::month( aDate );
    SInt    sDay      = mtdDateInterface::day( aDate );

    UInt    sValue    = mtc::yearForStandard( sYear, sMonth, sDay );
    UInt    sValueLen = 0;

    if ( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 4 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 4 );
    }
    else
    {
        sValueLen = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-46727 TO_CHAR()에 IYY 추가 */
static IDE_RC applyIYYFormat( mtdDateType * aDate,
                              SChar       * aBuffer,
                              SInt        * aBufferCur,
                              SInt        * aBufferFence,
                              SChar       * /* aString */,
                              idBool        aIsFillMode )
{
    SShort  sYear     = mtdDateInterface::year( aDate );
    UChar   sMonth    = mtdDateInterface::month( aDate );
    SInt    sDay      = mtdDateInterface::day( aDate );

    UInt    sValue    = mtc::yearForStandard( sYear, sMonth, sDay ) % 1000;
    UInt    sValueLen = 0;

    if ( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 3 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 3 );
    }
    else
    {
        sValueLen = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-46727 TO_CHAR()에 IY 추가 */
static IDE_RC applyIYFormat( mtdDateType * aDate,
                             SChar       * aBuffer,
                             SInt        * aBufferCur,
                             SInt        * aBufferFence,
                             SChar       * /* aString */,
                             idBool        aIsFillMode )
{
    SShort  sYear     = mtdDateInterface::year( aDate );
    UChar   sMonth    = mtdDateInterface::month( aDate );
    SInt    sDay      = mtdDateInterface::day( aDate );

    UInt    sValue    = mtc::yearForStandard( sYear, sMonth, sDay ) % 100;
    UInt    sValueLen = 0;

    if ( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 2 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 2 );
    }
    else
    {
        sValueLen = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* BUG-46727 TO_CHAR()에 I 추가 */
static IDE_RC applyIFormat( mtdDateType * aDate,
                            SChar       * aBuffer,
                            SInt        * aBufferCur,
                            SInt        * aBufferFence,
                            SChar       * /* aString */,
                            idBool        aIsFillMode )
{
    SShort  sYear     = mtdDateInterface::year( aDate );
    UChar   sMonth    = mtdDateInterface::month( aDate );
    SInt    sDay      = mtdDateInterface::day( aDate );

    UInt    sValue    = mtc::yearForStandard( sYear, sMonth, sDay ) % 10;
    UInt    sValueLen = 0;

    if ( aIsFillMode == ID_FALSE )
    {
        IDE_TEST( (*aBufferFence) - (*aBufferCur) < 1 );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur), sValue, 1 );
    }
    else
    {
        sValueLen = getIntegerLength( sValue );

        IDE_TEST( (UInt)(*aBufferFence) - (*aBufferCur) < sValueLen );

        (*aBufferCur) += mtfFastUInt2Str( aBuffer + (*aBufferCur),
                                          sValue,
                                          sValueLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static mtfFormatModuleFunc applySEPARATORFormat = &applyNONEFormat;
static mtfFormatModuleFunc applyWHITESPACEFormat = &applyNONEFormat;

static mtfFormatModuleFunc gFormatFuncSet[MTC_TO_CHAR_MAX_PRECISION] = { NULL,
    &applyNONEFormat,         &applyAM_UFormat,         &applyAM_ULFormat,
    &applyAM_LFormat,         &applyCCFormat,           &applyDAY_UFormat,
    &applyDAY_ULFormat,       &applyDAY_LFormat,        &applyDDDFormat,
    &applyDDFormat,           &applyDY_UFormat,         &applyDY_ULFormat,
    &applyDY_LFormat,         &applyDFormat,            &applyFFFormat,
    &applyFF1Format,          &applyFF2Format,          &applyFF3Format,
    &applyFF4Format,          &applyFF5Format,          &applyFF6Format,
    &applyHHFormat,           &applyHH12Format,         &applyHH24Format,
    &applyMIFormat,           &applyMMFormat,           &applyMON_UFormat,
    &applyMON_ULFormat,       &applyMON_LFormat,        &applyMONTH_UFormat,
    &applyMONTH_ULFormat,     &applyMONTH_LFormat,      &applyPM_UFormat,
    &applyPM_ULFormat,        &applyPM_LFormat,         &applyQFormat,
    &applyRM_UFormat,         &applyRM_LFormat,         &applyRRRRFormat,
    &applyRRFormat,           &applySSSSSSSSFormat,     &applySSSSSSFormat,
    &applySSSSSFormat,        &applySSFormat,           &applyWWFormat,
    &applyWFormat,            &applyYCYYYFormat,        &applyYYYYFormat,
    &applyYYYFormat,          &applyYYFormat,           &applyYFormat,
    &applyDOUBLE_QUOTE_STRINGFormat, &applyFMFormat,    applySEPARATORFormat,
    &applyIWFormat,           &applyWW2Format,          &applySYYYYFormat,
    &applySCCFormat,          &applyIYYYFormat,         &applyIYYFormat,  
    &applyIYFormat,           &applyIFormat,            applyWHITESPACEFormat, 
};

IDE_RC mtfTo_charEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt       /* aRemain */,
                           mtcCallBack * aCallBack )
{
    const mtdModule* sModules[2];
    SInt             sPrecision;

    mtcNode        * sCharNode;
    mtcColumn      * sCharColumn;
    mtdCharType    * sFormat;

    mtfNumberInfo  * sNumberInfo;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
    {
        IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                                aStack[1].column->module )
                  != IDE_SUCCESS );

        switch( aStack[1].column->module->flag & MTD_GROUP_MASK )
        {
            case MTD_GROUP_TEXT:
                sPrecision = aStack[1].column->precision;
                break;
            case MTD_GROUP_DATE:
            case MTD_GROUP_NUMBER:
            case MTD_GROUP_INTERVAL:
                sPrecision = MTC_TO_CHAR_MAX_PRECISION;
                break;
            case MTD_GROUP_MISC:
                // BUG-16741
                if ( ( aStack[1].column->module == &mtdByte ) ||
                     ( aStack[1].column->module == &mtdVarbyte ) )
                {
                    sPrecision = aStack[1].column->precision * 2;
                }
                else if ( (aStack[1].column->module == &mtdNibble) ||
                          (aStack[1].column->module == &mtdBit)    ||
                          (aStack[1].column->module == &mtdVarbit) )
                {
                    sPrecision = aStack[1].column->precision;
                }
                /* BUG-42666 To_char function is not considered clob locator */
                else if ( ( aStack[1].column->module == &mtdClob ) ||
                          ( aStack[1].column->module == &mtdClobLocator ) )
                {
                    /* BUG-36219 TO_CHAR, TO_NCHAR에서 LOB 지원 */
                    if ( aStack[1].column->precision != 0 )
                    {
                        sPrecision = IDL_MIN( aStack[1].column->precision,
                                              MTD_VARCHAR_PRECISION_MAXIMUM );
                    }
                    else
                    {
                        sPrecision = MTD_VARCHAR_PRECISION_MAXIMUM;
                    }
                }
                else
                {
                    // mtdNull인 경우도 포함
                    sPrecision = 1;
                }
                break;
            default:
                ideLog::log( IDE_ERR_0, 
                             "( aStack[1].column->module->flag & MTD_GROUP_MASK ) : %x\n",
                             aStack[1].column->module->flag & MTD_GROUP_MASK );

                IDE_ASSERT( 0 );
        }

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        // PROJ-1579 NCHAR
        if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
            (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecuteNcharFor1Arg;

            // NCHAR의 precisoin은 문자의 개수이므로 
            // 최대로 늘어날 수 있는 CHAR의 precision을 구한다.
            sPrecision = aStack[1].column->language->maxPrecision(sPrecision);
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                            mtfExecuteFor1Arg;
        }


        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdVarchar,  // BUG-16501
                                         1,
                                         sPrecision,
                                         0 )
              != IDE_SUCCESS );
    }
    else if ( ( aStack[1].column->module->flag & MTD_GROUP_MASK ) == 
              MTD_GROUP_NUMBER )
    {
        if( (aNode->lflag & MTC_NODE_REESTIMATE_MASK) == 
            MTC_NODE_REESTIMATE_FALSE )
        {
            sModules[0] = &mtdNumeric;
            sModules[1] = &mtdChar;

            // number format의 최대 길이를 64로 제한한다.
            IDE_TEST_RAISE( aStack[2].column->precision >
                            MTC_TO_CHAR_MAX_PRECISION,
                            ERR_TO_CHAR_MAX_PRECISION );

            // 'fmt'가 'rn'일 경우 15으로 설정해야 함.
            // 'fmt'가 'xxxx'일 경우 최대 8임.
            sPrecision = IDL_MAX( 15, aStack[2].column->precision + 3 );
        
            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
            
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                mtfExecuteNumberFor2Args;
            
            IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                             &mtdVarchar,  // BUG-16501
                                             1,
                                             sPrecision,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            sCharNode = mtf::convertedNode( aNode->arguments->next,
                                            aTemplate );

            sCharColumn = &(aTemplate->rows[sCharNode->table].
                              columns[sCharNode->column]);

            if( ( sCharNode == aNode->arguments->next ) &&
                ( ( aTemplate->rows[sCharNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                    == MTC_TUPLE_TYPE_CONSTANT ) )
            {
                sFormat =
                    (mtdCharType *)mtd::valueForModule(
                        (smiColumn*)&(sCharColumn->column),
                        aTemplate->rows[sCharNode->table].row,
                        MTD_OFFSET_USE,
                        mtdChar.staticNull );

                // format이 널일 경우 makeFormatInfo를 호출하지 않는다.
                if ( sFormat->length != 0 )
                {
                    // 'fmt'가 'rn'일 경우 15으로 설정해야 함.
                    // 'fmt'가 'xxxx'일 경우 최대 8임.
                    sPrecision = IDL_MAX( 15, sFormat->length + 3 );
                    
                    // calculateInfo에 format정보를 (sFormatInfo) 저장할 공간을 할당
                    IDE_TEST( aCallBack->alloc( aCallBack->info,
                                                ID_SIZEOF(mtfNumberInfo),
                                                (void**) &sNumberInfo )
                              != IDE_SUCCESS );
                    idlOS::memset( sNumberInfo, 0, ID_SIZEOF(mtfNumberInfo) );
                    
                    IDE_TEST( checkFormat( sFormat->value,
                                           sFormat->length,
                                           sNumberInfo->sToken )
                              != IDE_SUCCESS );
                    
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        (void*) sNumberInfo;
                    
                    /* PROJ-2208 Multi Currency */
                    if ( ( sNumberInfo->sToken[MTD_NUMBER_FORMAT_C] > 0 ) ||
                         ( sNumberInfo->sToken[MTD_NUMBER_FORMAT_D] > 0 ) ||
                         ( sNumberInfo->sToken[MTD_NUMBER_FORMAT_G] > 0 ) ||
                         ( sNumberInfo->sToken[MTD_NUMBER_FORMAT_L] > 0 ) )
                    {
                        IDE_TEST( aTemplate->nlsCurrency( aTemplate, &(sNumberInfo->sCurrency) )
                                  != IDE_SUCCESS );
                        aTemplate->nlsCurrencyRef = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    if ( sNumberInfo->sToken[MTD_NUMBER_FORMAT_C] > 0 )
                    {
                        sPrecision += MTL_TERRITORY_ISO_LEN;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    if ( sNumberInfo->sToken[MTD_NUMBER_FORMAT_L] > 0 )
                    {
                        sPrecision += idlOS::strlen( sNumberInfo->sCurrency.L );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                     &mtdVarchar,  // BUG-16501
                                                     1,
                                                     sPrecision,
                                                     0 )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }
    }
    else
    {
        sModules[0] = &mtdDate;

        if( aStack[2].column->language != NULL )
        {
            // PROJ-1579 NCHAR
            IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[1],
                                                    aStack[2].column->module )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-1579 NCHAR
            IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[1],
                                                    aStack[1].column->module )
                      != IDE_SUCCESS );
        }

        IDE_TEST_RAISE( aStack[2].column->precision >
                        MTC_TO_CHAR_MAX_PRECISION,
                        ERR_TO_CHAR_MAX_PRECISION );

        if( (aNode->lflag & MTC_NODE_REESTIMATE_MASK) == 
            MTC_NODE_REESTIMATE_FALSE )
        {
            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecuteDateFor2Args;

            // date fmt가 'day'일 경우 최대 9자리(wednesday)까지 올 수 있다.
            // 따라서 precision을 (aStack[2].column->precision) * 3로 설정한다.
            // toChar 함수에서 sBuffer에 쓰지 않고 바로 aStack[0]에 쓰기 위해서
            // precision을 최대값 + 1로 잡는다. 마지막에 NULL이 들어갈 수 
            // 있기 때문이다.
            IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdVarchar,  // BUG-16501
                                         1,
                                         (aStack[2].column->precision) * 3 + 1,
                                         0 )
                      != IDE_SUCCESS );
        }
        // REESTIMATE가 TRUE일 때에만 format 정보를 구성한다.
        else
        {
            sCharNode = mtf::convertedNode( aNode->arguments->next,
                                            aTemplate );

            sCharColumn = &(aTemplate->rows[sCharNode->table].
                              columns[sCharNode->column]);

            if( ( sCharNode == aNode->arguments->next ) &&
                ( ( aTemplate->rows[sCharNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                  == MTC_TUPLE_TYPE_CONSTANT ) )
            {
                sFormat =
                    (mtdCharType *)mtd::valueForModule(
                        (smiColumn*)&(sCharColumn->column),
                        aTemplate->rows[sCharNode->table].row,
                        MTD_OFFSET_USE,
                        mtdChar.staticNull );

                // format이 널일 경우 makeFormatInfo를 호출하지 않는다.
                if ( sFormat->length != 0 )
                {
                    IDE_TEST( mtfToCharInterface::makeFormatInfo( aNode,
                                                                  aTemplate,
                                                                  sFormat->value,
                                                                  sFormat->length,
                                                                  aCallBack )
                             != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }
    }

    if ( ( ( ( aStack[1].column->module->flag & MTD_GROUP_MASK ) == MTD_GROUP_NUMBER ) ||
           ( ( aStack[1].column->module->flag & MTD_GROUP_MASK ) == MTD_GROUP_DATE ) )
         &&
         ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 ) )
    {
        if( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next ) == ID_TRUE )
            &&
            ( ( ( aTemplate->rows[aNode->arguments->next->table].lflag
                  & MTC_TUPLE_TYPE_MASK )
                == MTC_TUPLE_TYPE_CONSTANT ) ||
              ( ( aTemplate->rows[aNode->arguments->next->table].lflag
                  & MTC_TUPLE_TYPE_MASK )
                == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
        {
            aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
            aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;

            // BUG-38070 undef type으로 re-estimate하지 않는다.
            if ( ( aTemplate->variableRow != ID_USHORT_MAX ) &&
                 ( ( aNode->lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) )
            {
                if ( aTemplate->rows[aTemplate->variableRow].
                     columns->module->id == MTD_UNDEF_ID )
                {
                    aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                    aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
            aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
        }
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_TO_CHAR_MAX_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_TO_CHAR_MAX_PRECISION,
                            MTC_TO_CHAR_MAX_PRECISION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTo_charCalculateFor1Arg( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : To_Char Calculate for CHAR
 *
 * Implementation :
 *    TO_CHAR( date )
 *
 *    aStack[0] : 입력된 날짜 형식을 문자형으로 변환하여 출력
 *    aStack[1] : date
 *
 *    ex) TO_CHAR( join_date ) ==> '09-JUN-2005'
 *
 ***********************************************************************/
    UInt    sStackSize;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        /* BUG-42666 To_char function is not considered clob locator */
        sStackSize = aStack[1].column->module->actualSize( aStack[1].column,
                                                           aStack[1].value );
        IDE_TEST_RAISE( aStack[0].column->column.size < sStackSize,
                        ERR_BUFFER_OVERFLOW );

        // CHAR => CHAR이므로 그냥 복사한다.
        idlOS::memcpy( aStack[0].value, aStack[1].value, sStackSize );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                 "mtfTo_charCalculateFor1Arg",
                                 "buffer overflow" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTo_charCalculateNcharFor1Arg( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : To_Char Calculate for NCHAR
 *
 * Implementation :
 *    TO_CHAR( date )
 *
 *    aStack[0] : 입력된 날짜 형식을 문자형으로 변환하여 출력
 *    aStack[1] : date
 *
 *    ex) TO_CHAR( join_date ) ==> '09-JUN-2005'
 *
 ***********************************************************************/

    mtdNcharType     * sSource;
    mtdCharType      * sResult;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // NCHAR => CHAR인 경우
        sSource = (mtdNcharType*)aStack[1].value;
        sResult = (mtdCharType*)aStack[0].value;

        IDE_TEST( mtdNcharInterface::toChar( aStack,
                                (const mtlModule *) mtl::mNationalCharSet,
                                (const mtlModule *) mtl::mDBCharSet,
                                sSource,
                                sResult )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfToCharInterface::mtfTo_charCalculateNumberFor2Args( mtcNode     * aNode,
                                                              mtcStack    * aStack,
                                                              SInt          aRemain,
                                                              void        * aInfo,
                                                              mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : To_Char Calculate
 *
 * Implementation :
 *    TO_CHAR( number, 'fmt' )
 *
 *    aStack[0] : 입력된 숫자를 number format에 따라 변환하여 문자로 출력
 *    aStack[1] : number
 *    aStack[2] : number format

 *    ex) TO_CHAR( 123.4567, '999999' )
 *       ==>   123
 *
 ***********************************************************************/

    mtdCharType*    sResult;
    mtdNumericType* sNumber;
    mtdNumericType* sNumberTemp;
    UChar           sNumberTempBuffer[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdCharType*    sVarchar;
    mtlCurrency     sCurrencyBuf;
    mtlCurrency*    sCurrency = &sCurrencyBuf;

    SInt            sIntNum     = 0;
    SLong           sRnNumCheck = 0;
    SChar*          sNumericString;
    UChar*          sNumFmt;
    UInt            sNumFmtLen  = 0;

    // Numeric을 문자열로 변환할 때 필요한 최대 버퍼 크기
    // Scale'의 범위가 -84 ~ 128 이므로, '+.' 또는 '-.'와 Scale 128 으로 계산
    SChar           sTemp[2 + MTD_NUMERIC_SCALE_MAXIMUM + 1];
    SInt            sTempLen = 0;

    UInt            sIterator = 0;
    idBool          sIsMinus = ID_FALSE;
    UInt            sResultLen = 0;
    UChar           sString[MTD_NUMBER_FORMAT_BUFFER_LEN] = {0,};     // applyFormat()를 호출하기 전에 초기화
    idBool          sIsV = ID_FALSE;
    SInt            sNumCnt = 0;
    SInt            sIntNumCnt = 0;
    SInt            sNineCnt = 0;
    SInt            sAfterPeriodNumCnt = 0;
    SInt            sAfterPeriodNineCnt = 0;
    SInt            sAfterPeriodInvalidNumCnt = 0;
    idBool          sIsPeriod = ID_FALSE;
    UChar           sRoundPos[4];
    SInt            sRoundNum = 0;
    UShort          sRoundPosLen = 0;
    UChar           sTokenBuf[MTD_NUMBER_MAX];
    UChar         * sToken = sTokenBuf;
    UInt            sBlankCnt = 0;
    mtdNumericType* sMtdZeroValue;
    UChar           sMtdZeroBuff[20] = { 1, 128, 0, };
    mtdNumericType* sArgument2;
    UChar           sArgument2Buff[MTD_NUMERIC_SIZE_MAXIMUM];
    mtfNumberInfo * sNumberInfo;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdCharType*)aStack[0].value;
        sNumber = (mtdNumericType*)aStack[1].value;
        sVarchar = (mtdCharType*)aStack[2].value;

        sNumberTemp = (mtdNumericType*)sNumberTempBuffer;
        sNumberTemp->length = 0;

        sNumFmt = sVarchar->value;
        sNumFmtLen = sVarchar->length;

        if ( aInfo != NULL )
        {
            sNumberInfo = (mtfNumberInfo*) aInfo;

            idlOS::memcpy( sToken,
                           sNumberInfo->sToken,
                           ID_SIZEOF(UChar) * MTD_NUMBER_MAX );
            sCurrency = &(sNumberInfo->sCurrency);
        }
        else
        {
            //fix BUG-18162
            idlOS::memset( sToken,
                           0,
                           ID_SIZEOF(UChar) * MTD_NUMBER_MAX );
            
            IDE_TEST( checkFormat( sNumFmt, sNumFmtLen, sToken )
                      != IDE_SUCCESS );
            
            /* PROJ-2208 Multi Currency */
            if ( ( sToken[MTD_NUMBER_FORMAT_C] > 0 ) ||
                 ( sToken[MTD_NUMBER_FORMAT_D] > 0 ) ||
                 ( sToken[MTD_NUMBER_FORMAT_G] > 0 ) ||
                 ( sToken[MTD_NUMBER_FORMAT_L] > 0 ) )
            {
                idlOS::memset( sCurrency, 0x0, sizeof( mtlCurrency ));
                IDE_TEST( aTemplate->nlsCurrency( aTemplate, sCurrency )
                          != IDE_SUCCESS );
                aTemplate->nlsCurrencyRef = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        
        if( sToken[MTD_NUMBER_FORMAT_FM] == 1 )
        {
            sNumFmt += 2;
            sNumFmtLen -= 2;
        }
        else
        {
            // Nothing to do
        }

        if ( sNumber->signExponent < 128 )
        {
            sIsMinus = ID_TRUE;
        }
        else
        {
            sIsMinus = ID_FALSE;
        }

        if ( ( idlOS::strCaselessMatch( sNumFmt, 2, "RN", 2 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sNumFmt, 4, "XXXX", 4 ) == 0 ) )
        {
            // 소수가 입력될 경우 정수로 반올림한다.
            sMtdZeroValue = (mtdNumericType*)sMtdZeroBuff;
            IDE_TEST( mtc::roundFloat( sNumberTemp,
                                       sNumber,
                                       sMtdZeroValue )
                      != IDE_SUCCESS );

            // numeric 타입을 정수로 변환하는 과정. integer 범위 내에서 변환한다.
            // *(sNumberTemp)는 numeric의 length이다.
            if ( sNumberTemp->length > 0 )
            {
                for ( sIterator = 0; sIterator < ((UInt)sNumberTemp->length) - 1; sIterator++ )
                {
                    if ( sNumberTemp->signExponent > 128 )
                    {
                        sRnNumCheck += (SLong) ( sNumberTemp->mantissa[sIterator] *
                                                 idlOS::pow( 100, ( sNumberTemp->signExponent -
                                                                    ( 193 + sIterator ) ) ) );
                    }
                    else
                    {
                        sRnNumCheck -= (SLong) ( ( 99 - sNumberTemp->mantissa[sIterator] ) *
                                                 idlOS::pow( 100, ( 63 - sNumberTemp->signExponent -
                                                                    sIterator ) ) );
                    }
                    
                    // 결과값은 Integer이므로, numeric을 Integer 크기 이하로 제한해야 한다.
                    if ( sRnNumCheck > ID_SINT_MAX || sRnNumCheck < ID_SINT_MIN )
                    {
                        IDE_RAISE( ERR_INVALID_LENGTH );
                    }
                    else
                    {
                        sIntNum = (SInt) sRnNumCheck;
                    }
                }
            }
            else
            {
                // Nothing to do.
            }

            // XXXX 또는 RN format을 계산한다.
            IDE_TEST( compXXXXandRN( sNumFmt,
                                     sNumFmtLen,
                                     sResult->value,
                                     &(sResult->length),
                                     sIntNum )
                      != IDE_SUCCESS );
        }
        // RN, XXXX가 아닌  fmt일 경우 sNumber를 그대로 string으로 변환한다.
        else
        {
            sNineCnt = sToken[MTD_NUMBER_FORMAT_ZERO] +
                       sToken[MTD_NUMBER_FORMAT_NINE];
            sAfterPeriodNineCnt = sToken[MTD_COUNT_FLOAT_ZERO] +
                                  sToken[MTD_COUNT_FLOAT_NINE];
            if ( sToken[MTD_NUMBER_FORMAT_EEEE] <= 0 )
            {
                // format의 . 뒤에 나온 9 또는 0의 개수에 맞춰서 반올림
                sRoundNum = sAfterPeriodNineCnt;

                if ( sRoundNum >= 100 )
                {
                    sRoundPos[0] = sRoundNum / 100 + '0';
                    sRoundPos[1] = ( sRoundNum / 10 ) -
                                   ( ( sRoundNum / 100 ) * 10 ) + '0';
                    sRoundPos[2] = sRoundNum % 10 + '0';
                    sRoundPosLen = 3;
                }
                else if ( sRoundNum >= 10 )
                {
                    sRoundPos[0] = sRoundNum / 10 + '0';
                    sRoundPos[1] = sRoundNum % 10 + '0';
                    sRoundPosLen = 2;
                }
                else if ( sRoundNum > 0 )
                {
                    sRoundPos[0] = sRoundNum % 10 + '0';
                    sRoundPosLen = 1;
                }
                else if ( sRoundNum == 0 )
                {
                    sRoundPos[0] = '0';
                    sRoundPosLen = 1;
                }

                sArgument2 = (mtdNumericType*)sArgument2Buff;
                IDE_TEST( mtc::makeNumeric( sArgument2,
                                            MTD_FLOAT_MANTISSA_MAXIMUM,
                                            sRoundPos,
                                            sRoundPosLen )
                          != IDE_SUCCESS );

                IDE_TEST( mtc::roundFloat( sNumberTemp,
                                           sNumber,
                                           sArgument2 )
                          != IDE_SUCCESS );

                IDE_TEST( convertToString( sNumberTemp->length,
                                           sNumberTemp->signExponent,
                                           sNumberTemp->mantissa,
                                           sTemp,
                                           &sTempLen )
                          != IDE_SUCCESS );

                sNumericString = sTemp;
            }
            else
            {
                IDE_TEST( convertToString( sNumber->length,
                                           sNumber->signExponent,
                                           sNumber->mantissa,
                                           sTemp,
                                           &sTempLen )
                          != IDE_SUCCESS );

                sNumericString = sTemp;

                for ( sIterator = 0; sIterator < (UInt) sTempLen; sIterator++ )
                {
                    if ( *( sNumericString + sIterator ) == '.' )
                    {
                        sIsPeriod = ID_TRUE;
                    }

                    if ( *( sNumericString + sIterator ) >= '0' &&
                         *( sNumericString + sIterator ) <= '9' )
                    {
                        sNumCnt++;

                        if ( sIsPeriod == ID_FALSE )
                        {
                            sIntNumCnt++;
                        }
                        else
                        {
                            sAfterPeriodNumCnt++;
                        }
                    }
                }

                sIsPeriod = ID_FALSE;

                for ( sIterator = 0; sIterator < (UInt) sTempLen; sIterator++ )
                {
                    if ( *( sNumericString + sIterator ) >= '1' &&
                         *( sNumericString + sIterator ) <= '9' &&
                         sIsPeriod == ID_TRUE )
                    {
                        break;
                    }

                    if ( sIsPeriod == ID_TRUE && *( sNumericString + sIterator ) == '0' )
                    {
                        sAfterPeriodInvalidNumCnt++;
                    }

                    if ( *( sNumericString + sIterator ) == '.' )
                    {
                        sIsPeriod = ID_TRUE;
                    }
                }

                // sString의 0~9의 개수보다 format의 9의 개수가 많을 경우
                // 반올림한다.
                // 몇 번째 자리에서 반올림할 것인지 결정
                if ( sIntNumCnt > 0 &&
                     ( sNumCnt > sNineCnt ||
                     ( sAfterPeriodNineCnt + 1 ) != sNumCnt ) )
                {
                    sRoundNum = ( sAfterPeriodNineCnt + 1 ) - sIntNumCnt;
                }
                else if ( sIntNumCnt == 0 )
                {
                    sRoundNum = sAfterPeriodNineCnt + 1 + sAfterPeriodInvalidNumCnt;
                }
                else
                {
                    sRoundNum = sAfterPeriodNumCnt;
                }

                if ( sIsV == ID_TRUE )
                {
                    if ( sIntNumCnt == 0 )
                    {
                        sRoundNum = sAfterPeriodNineCnt + 1 + sAfterPeriodInvalidNumCnt;
                    }
                    else
                    {
                        sRoundNum = sAfterPeriodNineCnt - sIntNumCnt + 1;
                    }
                }

                if ( sRoundNum >= 100 )
                {
                    sRoundPos[0] = sRoundNum / 100 + '0';
                    sRoundPos[1] = ( sRoundNum / 10 ) -
                                   ( ( sRoundNum / 100 ) * 10 ) + '0';
                    sRoundPos[2] = sRoundNum % 10 + '0';
                    sRoundPosLen = 3;
                }
                else if ( sRoundNum >= 10 )
                {
                    sRoundPos[0] = sRoundNum / 10 + '0';
                    sRoundPos[1] = sRoundNum % 10 + '0';
                    sRoundPosLen = 2;
                }
                else if ( sRoundNum > 0 )
                {
                    sRoundPos[0] = sRoundNum % 10 + '0';
                    sRoundPosLen = 1;
                }
                else if ( sRoundNum == 0 )
                {
                    sRoundPos[0] = '0';
                    sRoundPosLen = 1;
                }
                else if ( sRoundNum < 0 && sRoundNum > -10 )
                {
                    sRoundPos[0] = '-';
                    sRoundPos[1] = abs( sRoundNum ) % 10 + '0';
                    sRoundPosLen = 2;
                }
                else if ( sRoundNum <= -10 && sRoundNum > -100 )
                {
                    sRoundPos[0] = '-';
                    sRoundPos[1] = abs( sRoundNum ) / 10 + '0';
                    sRoundPos[2] = abs( sRoundNum ) % 10 + '0';
                    sRoundPosLen = 3;
                }
                else if ( sRoundNum <= -100 )
                {
                    sRoundPos[0] = '-';
                    sRoundPos[1] = abs( sRoundNum ) / 100 + '0';
                    sRoundPos[2] = ( abs( sRoundNum ) / 10 ) -
                                   ( ( abs( sRoundNum ) / 100 ) * 10 ) + '0';
                    sRoundPos[3] = sRoundNum % 10 + '0';
                    sRoundPosLen = 4;
                }
                else
                {
                    // nothing to do
                }

                sArgument2 = (mtdNumericType*)sArgument2Buff;
                IDE_TEST( mtc::makeNumeric( sArgument2,
                                            MTD_FLOAT_MANTISSA_MAXIMUM,
                                            sRoundPos,
                                            sRoundPosLen )
                          != IDE_SUCCESS );

                IDE_TEST( mtc::roundFloat( sNumberTemp,
                                           sNumber,
                                           sArgument2 )
                          != IDE_SUCCESS );

                IDE_TEST( convertToString( sNumberTemp->length,
                                           sNumberTemp->signExponent,
                                           sNumberTemp->mantissa,
                                           sTemp,
                                          &sTempLen )
                          != IDE_SUCCESS );

                sNumericString = sTemp;
            }

            IDE_TEST( applyFormat( sNumericString,
                                   sTempLen,
                                   sNumFmt,
                                   sNumFmtLen,
                                   sString,
                                   &sResultLen,
                                   sToken,
                                   sCurrency,
                                   sIsMinus )
                      != IDE_SUCCESS );

            // BUG-37357
            IDE_TEST_RAISE( (UInt)aStack[0].column->precision < sResultLen,
                            ERR_INVALID_LENGTH );
            
            idlOS::memcpy( sResult->value,
                           sString,
                           sResultLen );
            sResult->length = sResultLen;
        }

        // To fix BUG-17693
        if( sToken[MTD_NUMBER_FORMAT_FM] == 1 )
        {
            // BUG-19149
            IDE_TEST( removeLeadingBlank( (SChar *) sResult->value,
                                          (UInt) sResult->length,
                                          & sBlankCnt )
                      != IDE_SUCCESS );
            
            sResult->length = (sResult->length) - sBlankCnt;
        }
        else
        {
            // Nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC convertToRoman( SInt aIntNum, UShort* aRNCharCnt, SChar* aTemp )
{
/***********************************************************************
 *
 * Description : 로마 숫자로 변환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar* sRNChar = aTemp;

    sRNChar[0] = '\0';
    *aRNCharCnt = 0;

    while ( aIntNum > 0 )
    {
        if ( aIntNum > 999 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "M" );
            (*aRNCharCnt) += 1;
            aIntNum -= 1000;
        }
        else if ( aIntNum > 899 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "CM" );
            (*aRNCharCnt) += 2;
            aIntNum -= 900;
        }
        else if ( aIntNum > 499 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "D" );
            (*aRNCharCnt) += 1;
            aIntNum -= 500;
        }
        else if ( aIntNum > 399 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "CD" );
            (*aRNCharCnt) += 2;
            aIntNum -= 400;
        }
        else if ( aIntNum > 99 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "C" );
            (*aRNCharCnt) += 1;
            aIntNum -= 100;
        }
        else if ( aIntNum > 89 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "XC" );
            (*aRNCharCnt) += 2;
            aIntNum -= 90;
        }
        else if ( aIntNum > 49 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "L" );
            (*aRNCharCnt) += 1;
            aIntNum -= 50;
        }
        else if ( aIntNum > 39 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "XL" );
            (*aRNCharCnt) += 2;
            aIntNum -= 40;
        }
        else if ( aIntNum > 9 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "X" );
            (*aRNCharCnt) += 1;
            aIntNum -= 10;
        }
        else if ( aIntNum == 9 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "IX" );
            (*aRNCharCnt) += 2;
            aIntNum = 0;
        }
        else if ( aIntNum > 4 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "V" );
            (*aRNCharCnt) += 1;
            aIntNum -= 5;
        }
        else if ( aIntNum == 4 )
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "IV" );
            (*aRNCharCnt) += 2;
            aIntNum = 0;
        }
        else /*if ( aIntNum > 0 )*/
        {
            idlVA::appendFormat( sRNChar, 16, "%s", "I" );
            (*aRNCharCnt) += 1;
            aIntNum -= 1;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC convertToString( SInt   aLength,
                        SInt   aSignExp,
                        UChar* aMantissa,
                        SChar* aTemp,
                        SInt*  aTempLen )
{
/***********************************************************************
 *
 * Description : numeric type의 숫자를 to_char(number, number_format)
 *               에서 변환이 쉽도록 string으로 변환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar*   sTemp = aTemp;
    SInt     sBufferCur = 0;
    // Numeric을 문자열로 변환할 때 필요한 최대 버퍼 크기
    // Scale'의 범위가 -84 ~ 128 이므로, '+.' 또는 '-.'와 Scale 128 으로 계산
    SInt     sBufferFence = 2 + MTD_NUMERIC_SCALE_MAXIMUM + 1;

    idBool   sIsMinus = ID_FALSE;
    idBool   sIsFloat = ID_FALSE;
    idBool   sIsPoint = ID_FALSE;

    SInt     sNumber;
    SInt     sZeroCount;
    SInt     sIterator;
    SInt     sIterator2 = 0;
    SInt     sCharCnt = 0;

    sTemp[0] = '\0';

    if ( aSignExp > 128 )
    {
        sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "+" );
    }
    // 0일 경우에는 +. 으로 반환한다.
    else if ( aSignExp == 128 )
    {
        sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "+" );
        sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "." );
        *aTempLen = IDL_MIN( sBufferCur, sBufferFence - 1 );

        return IDE_SUCCESS;
    }
    else
    {
        sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "-" );
    }

    // 음수일 경우 양수로 변환
    if ( aSignExp < 128 )
    {
        aSignExp = 128 - aSignExp;
        aSignExp += 128;
        sIsMinus = ID_TRUE;
    }

    // 소수점 아래 부분을 가질 때는 0 붙는 위치가 다름
    if ( aSignExp - 193 < aLength - 2 )
    {
        sIsFloat = ID_TRUE;
    }

    for ( sIterator = 0; sIterator < aLength - 1; sIterator++ )
    {
        sNumber = *( aMantissa + sIterator );

        // 음수이면
        if ( sIsMinus == ID_TRUE )
        {
            sNumber = 99 - sNumber;
        }

        // 소수 부분을 가질 경우, 일의 자리의 위치를 저장
        if ( sIsPoint == ID_FALSE && sIsFloat == ID_TRUE &&
             sIterator > ( aSignExp - 193 ) )
        {
            sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "." );
            sIsPoint = ID_TRUE;

            // 소수인 경우 .다음에 붙는 0의 개수 계산
            if ( ( sIsMinus == ID_TRUE &&
                 ( 99 - *aMantissa ) >= 10 ) ||
                 ( sIsMinus == ID_FALSE &&
                 *aMantissa >= 10 ) )
            {
                sZeroCount = ( 193 - aSignExp ) * 2 - 2;
            }
            else
            {
                sZeroCount = ( 193 - aSignExp ) * 2 - 1;
            }

            for ( sIterator2 = 0; sIterator2 < sZeroCount; sIterator2++ )
            {
                sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "0" );
            }
        }

        if ( sNumber >= 10 )
        {
             sBufferCur = idlVA::appendFormat( sTemp,
                                 sBufferFence,
                                 "%d",
                                 sNumber / 10 );
            sCharCnt++;

            if ( sNumber % 10 != 0 )
            {
                 sBufferCur = idlVA::appendFormat( sTemp,
                                     sBufferFence,
                                     "%d",
                                     sNumber % 10 );
                sCharCnt++;
            }
            else
            {
                // 소수 부분이 있을 경우,
                // 맨 마지막 mantissa가 10의 배수일 때 맨끝의 0을 버린다.
                if  ( sIterator == aLength - 2 && sIsFloat == ID_TRUE )
                {
                    // nothing to do
                }
                else
                {
                     sBufferCur = idlVA::appendFormat( sTemp,
                                         sBufferFence,
                                         "%s",
                                         "0" );
                    sCharCnt++;
                }
            }
        }
        else if ( sNumber >= 0 && sNumber < 10 )
        {
            // 첫번째 mantissa 값이 한자리수이면
            if ( sIterator == 0 )
            {
                sBufferCur = idlVA::appendFormat( sTemp,
                                     sBufferFence,
                                     "%d",
                                     sNumber );
                sCharCnt++;
            }
            // 중간이나 맨 끝의 mantissa 값이 한자리수이면
            else
            {
                 sBufferCur = idlVA::appendFormat( sTemp,
                                      sBufferFence,
                                      "%s",
                                      "0" );
                 sBufferCur = idlVA::appendFormat( sTemp,
                                  sBufferFence,
                                  "%d",
                                  sNumber );
                sCharCnt += 2;
            }
        }
        else
        {
            break;
        }
    }

    // 정수인 경우 입력된 수 이외에 0이 추가로 붙는 경우 0의 개수 계산
    if ( sIsFloat == ID_TRUE )
    {
        sZeroCount = 0;
    }
    else if ( aSignExp >= 193 && sIsFloat == ID_FALSE &&
         ( sCharCnt - aLength >= aSignExp - 193 ) )
    {
        sZeroCount = 0;
    }
    else
    {
        sZeroCount = ( ( aSignExp - 193 ) -
                       ( aLength - 2 ) ) * 2;
    }

    for ( sIterator = 0; sIterator < sZeroCount; sIterator++ )
    {
        sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "0" );
    }

    if ( sIsFloat == ID_FALSE )
    {
        sBufferCur = idlVA::appendFormat( sTemp, sBufferFence, "%s", "." );
    }
    else
    {
        // nothing to do
    }

    *aTempLen = IDL_MIN( sBufferCur, sBufferFence - 1 );

    return IDE_SUCCESS;
}

IDE_RC checkFormat( UChar* aFmt, UInt aLength, UChar* aToken )
{
/***********************************************************************
 *
 * Description : number format을 체크한다.
 *              다른 함수에서 사용될 number format token을 aToken에 저장한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UShort sCommaCnt  = 0;
    UShort sPeriodCnt = 0;
    UShort sDollarCnt = 0;
    UShort sZeroCnt   = 0;
    UShort sNineCnt   = 0;
    UShort sBCnt      = 0;
    UShort sEEEECnt   = 0;
    UShort sMICnt     = 0;
    UShort sPRCnt     = 0;
    UShort sRNCnt     = 0;
    UShort sSCnt      = 0;
    UShort sVCnt      = 0;
    UShort sXXXXCnt   = 0;
    UShort sFMCnt     = 0;
    UShort sCCnt      = 0;
    UShort sLCnt      = 0;
    UShort sGCnt      = 0;
    UShort sDCnt      = 0;

    SInt   sIntNineCnt   = 0;
    SInt   sIntZeroCnt   = 0;
    SInt   sFloatNineCnt = 0;
    SInt   sFloatZeroCnt = 0;
    SInt   sAfterVNineZeroCnt = 0;

    idBool sIsFirstS      = ID_FALSE;
    idBool sIsFirstPeriod = ID_FALSE;

    UShort sFormatIndex   = 0;
    UChar* sFormat        = aFmt;
    UInt   sFormatLeftLen = aLength;  // 처리하고 남은 format string의 length
    UInt   sFormatLen     = aLength;   // format string의 length

    // To fix BUG-17693,28199
    // 'FM' format을 format string에서 찾는다.
    // (결과 문자열의 앞쪽 공백제거의 의미이고 맨 앞에 한번밖에 나올 수 없다.)
    if ( idlOS::strCaselessMatch( sFormat, 2, "FM", 2 ) == 0 )
    {
        sFMCnt++;

        // 'FM' format을 빼고 sFormat을 설정한다.
        sFormat += 2;
        sFormatLeftLen -= 2;
        sFormatLen -= 2;
        // sFormatIndex = 0;
    }
    else
    {
        // Nothing to do.
    }
    
    while ( sFormatIndex < sFormatLen )
    {
        if ( sFormatLeftLen >= 4 )
        {
            if ( idlOS::strCaselessMatch( sFormat, 4, "EEEE", 4 ) == 0 )
            {
                // eeee format이 있을 경우 9나 0이 나오기 전에 .이 나오면 안됨.
                IDE_TEST_RAISE( ( sEEEECnt != 0 ) || ( sCommaCnt != 0) ||
                                ( sGCnt != 0 )    ||
                                ( sIsFirstPeriod == ID_TRUE ),
                                ERR_INVALID_LITERAL );

                if ( sFormatLeftLen >= 6 )
                {
                    if ( (idlOS::strCaselessMatch( sFormat+4, 2, "MI", 2 ) == 0) ||
                         (idlOS::strCaselessMatch( sFormat+4, 2, "PR", 2 ) == 0) )
                    {
                        IDE_TEST_RAISE( sFormatIndex != sFormatLen - 6,
                                        ERR_INVALID_LITERAL );
                    }
                    else
                    {
                        IDE_RAISE( ERR_INVALID_LITERAL );
                    }
                }
                else if ( sFormatLeftLen >= 5 )
                {
                    if ( idlOS::strCaselessMatch( sFormat+4, 1, "S", 1 ) == 0 )
                    {
                        IDE_TEST_RAISE( sFormatIndex != sFormatLen - 5,
                                        ERR_INVALID_LITERAL );
                    }
                    else
                    {
                        IDE_RAISE( ERR_INVALID_LITERAL );
                    }
                }
                else
                {
                    IDE_TEST_RAISE( ( sFormatIndex != sFormatLen - 4 ) ||
                                    ( sFormatLen   == 4 ),
                                    ERR_INVALID_LITERAL );
                }

                sEEEECnt++;
                sFormat += 4;
                sFormatLeftLen -= 4;
                sFormatIndex += 3;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 4, "XXXX", 4 ) == 0 )
            {
                IDE_TEST_RAISE( sFormatLen != 4, ERR_INVALID_LITERAL );

                sXXXXCnt++;
                sFormat += 4;
                sFormatLeftLen -= 4;
                sFormatIndex += 3;
                goto break_out;
            }
        }
        if ( sFormatLeftLen >= 2 )
        {
            if ( idlOS::strCaselessMatch( sFormat, 2, "MI", 2 ) == 0 )
            {
                IDE_TEST_RAISE( ( sFormatIndex != sFormatLen -2 ) ||
                                ( sSCnt != 0 ) || ( sPRCnt != 0 ) ||
                                ( sMICnt != 0 ),
                                ERR_INVALID_LITERAL );

                sMICnt++;
                sFormat += 2;
                sFormatLeftLen -= 2;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 2, "PR", 2 ) == 0 )
            {
                IDE_TEST_RAISE( ( sFormatIndex != sFormatLen -2 ) ||
                                ( sSCnt != 0 ) || ( sPRCnt != 0 ) ||
                                ( sMICnt != 0 ),
                                ERR_INVALID_LITERAL );
                sPRCnt++;
                sFormat += 2;
                sFormatLeftLen -= 2;
                sFormatIndex++;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 2, "RN", 2 ) == 0 )
            {
                IDE_TEST_RAISE( sFormatLen != 2, ERR_INVALID_LITERAL );

                sRNCnt++;
                sFormat += 2;
                sFormatLeftLen -= 2;
                sFormatIndex++;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 2, "FM", 2 ) == 0 )
            {
                // 'FM' format은 format string 중간에 나올 수 없다.
                IDE_RAISE( ERR_INVALID_LITERAL );
            }
        }
        if ( sFormatLeftLen >= 1 )
        {
            if ( idlOS::strMatch( sFormat, 1, ",", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sDCnt != 0 )              ||
                                ( sPeriodCnt != 0 )         ||
                                ( sEEEECnt   != 0 )         ||
                                ( sVCnt      != 0 )         ||
                                ( sGCnt      != 0 )         ||
                                (( sNineCnt + sZeroCnt ) == 0 ),
                                ERR_INVALID_LITERAL );
                sCommaCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strMatch( sFormat, 1, ".", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sDCnt      != 0 )   ||
                                ( sPeriodCnt != 0 )   ||
                                ( sEEEECnt   != 0 )   ||
                                ( sGCnt      != 0 )   ||
                                ( sVCnt      != 0 ),
                                ERR_INVALID_LITERAL );

                if ( (sFormatIndex == sFormatLen - 1) ||
                     (( sNineCnt + sZeroCnt ) == 0) )
                {
                    sIsFirstPeriod = ID_TRUE;
                }
                sPeriodCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strMatch( sFormat, 1, "$", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sLCnt != 0 )      ||
                                ( sCCnt != 0 )      ||
                                ( sDollarCnt != 0 ),
                                ERR_INVALID_LITERAL );
                sDollarCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strMatch( sFormat, 1, "0", 1 ) == 0 )
            {
                if ( (sPeriodCnt == 0) && (sVCnt == 0) && (sDCnt == 0))
                {
                    sIntZeroCnt++;
                }

                if ( sVCnt == 1 )
                {
                    sAfterVNineZeroCnt++;
                    sFloatZeroCnt++;
                }

                if ( sPeriodCnt == 1 || sDCnt == 1 )
                {
                    sFloatZeroCnt++;
                }

                sZeroCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strMatch( sFormat, 1, "9", 1 ) == 0 )
            {
                if ( (sPeriodCnt == 0) && (sVCnt == 0) && (sDCnt == 0) )
                {
                    sIntNineCnt++;
                }

                if ( sVCnt == 1 )
                {
                    sAfterVNineZeroCnt++;
                    sFloatNineCnt++;
                }

                if ( sPeriodCnt == 1 || sDCnt == 1 )
                {
                    sFloatNineCnt++;
                }

                sNineCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 1, "B", 1 ) == 0 )
            {
                IDE_TEST_RAISE( sBCnt != 0, ERR_INVALID_LITERAL );

                sBCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 1, "S", 1 ) == 0 )
            {
                if ( sFormatLeftLen == sFormatLen )
                {
                    sIsFirstS = ID_TRUE;
                }

                IDE_TEST_RAISE( ( sFormatIndex != 0 ) &&
                                ( sFormatIndex != sFormatLen -1 ),
                                ERR_INVALID_LITERAL );
                IDE_TEST_RAISE( ( sSCnt != 0 ) || ( sPRCnt != 0 ) ||
                                ( sMICnt != 0 ),
                                ERR_INVALID_LITERAL );

                sSCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 1, "V", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sPeriodCnt != 0 ) ||
                                ( sDCnt      != 0 ) ||
                                ( sVCnt      != 0 ),
                                ERR_INVALID_LITERAL );

                sVCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 1, "C", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sLCnt != 0 )      ||
                                ( sCCnt != 0 )      ||
                                ( sDollarCnt != 0 ),
                                ERR_INVALID_LITERAL );

                if ( ( sFormatIndex > 0 ) &&
                     ( sFormatIndex < sFormatLen - 1 ))
                {
                    if ( sIsFirstS == ID_FALSE )
                    {
                        if ( sFormatLeftLen > 3 )
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                        else if ( sFormatLeftLen == 3 )
                        {
                            if ( (idlOS::strCaselessMatch( sFormat + 1, 2,
                                                           "PR", 2 ) != 0 ) &&
                                 ( idlOS::strCaselessMatch( sFormat + 1, 2,
                                                           "MI", 2 ) != 0 ) )
                            {
                                IDE_RAISE( ERR_INVALID_LITERAL );
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        } else if ( sFormatLeftLen == 2 )
                        {
                            if ( idlOS::strCaselessMatch( sFormat + 1, 1,
                                                          "S", 1 ) != 0 )
                            {
                                IDE_RAISE( ERR_INVALID_LITERAL );
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }
                        else
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                    }
                    else
                    {
                        IDE_TEST_RAISE( sFormatIndex != 1, ERR_INVALID_LITERAL );
                    }
                }

                sCCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 1, "L", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sLCnt != 0 )      ||
                                ( sCCnt != 0 )      ||
                                ( sDollarCnt != 0 ),
                                ERR_INVALID_LITERAL );

                if ( ( sFormatIndex > 0 ) &&
                     ( sFormatIndex < sFormatLen - 1 ))
                {
                    if ( sIsFirstS == ID_FALSE )
                    {
                        if ( sFormatLeftLen > 3 )
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                        else if ( sFormatLeftLen == 3 )
                        {
                            if ( (idlOS::strCaselessMatch( sFormat + 1, 2,
                                                           "PR", 2 ) != 0 ) &&
                                 ( idlOS::strCaselessMatch( sFormat + 1, 2,
                                                           "MI", 2 ) != 0 ) )
                            {
                                IDE_RAISE( ERR_INVALID_LITERAL );
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        } else if ( sFormatLeftLen == 2 )
                        {
                            if ( idlOS::strCaselessMatch( sFormat + 1, 1,
                                                          "S", 1 ) != 0 )
                            {
                                IDE_RAISE( ERR_INVALID_LITERAL );
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }
                        else
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                    }
                    else
                    {
                        IDE_TEST_RAISE( sFormatIndex != 1, ERR_INVALID_LITERAL );
                    }
                }
                sLCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 1, "G", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sDCnt != 0 )              ||
                                ( sPeriodCnt != 0 )         ||
                                ( sEEEECnt   != 0 )         ||
                                ( sVCnt      != 0 )         ||
                                ( sCommaCnt  != 0 )         ||
                                ( sPeriodCnt != 0 )         ||
                                (( sNineCnt + sZeroCnt ) == 0 ),
                                ERR_INVALID_LITERAL );

                sGCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
            if ( idlOS::strCaselessMatch( sFormat, 1, "D", 1 ) == 0 )
            {
                IDE_TEST_RAISE( ( sDCnt      != 0 )   ||
                                ( sPeriodCnt != 0 )   ||
                                ( sEEEECnt   != 0 )   ||
                                ( sCommaCnt  != 0 )   ||
                                ( sPeriodCnt != 0 )   ||
                                ( sVCnt      != 0 ),
                                ERR_INVALID_LITERAL );
                if ( (sFormatIndex == sFormatLen - 1) ||
                     (( sNineCnt + sZeroCnt ) == 0) )
                {
                    sIsFirstPeriod = ID_TRUE;
                }
                sDCnt++;
                sFormat++;
                sFormatLeftLen--;
                goto break_out;
            }
        }
break_out:
        sFormatIndex++;
    }

    // 지원히지 않는 format이 있는 경우, ERROR
    IDE_TEST_RAISE( sFormatLeftLen != 0, ERR_INVALID_LITERAL )

    aToken[MTD_NUMBER_FORMAT_FM]     = sFMCnt;
    aToken[MTD_NUMBER_FORMAT_COMMA]  = sCommaCnt;
    aToken[MTD_NUMBER_FORMAT_PERIOD] = sPeriodCnt;
    aToken[MTD_NUMBER_FORMAT_DOLLAR] = sDollarCnt;
    aToken[MTD_NUMBER_FORMAT_ZERO]   = sZeroCnt;
    aToken[MTD_NUMBER_FORMAT_NINE]   = sNineCnt;
    aToken[MTD_NUMBER_FORMAT_B]      = sBCnt;
    aToken[MTD_NUMBER_FORMAT_EEEE]   = sEEEECnt;
    aToken[MTD_NUMBER_FORMAT_MI]     = sMICnt;
    aToken[MTD_NUMBER_FORMAT_PR]     = sPRCnt;
    aToken[MTD_NUMBER_FORMAT_RN]     = sRNCnt;
    aToken[MTD_NUMBER_FORMAT_S]      = sSCnt;
    aToken[MTD_NUMBER_FORMAT_V]      = sVCnt;
    aToken[MTD_NUMBER_FORMAT_XXXX]   = sXXXXCnt;
    aToken[MTD_NUMBER_FORMAT_C]      = sCCnt;
    aToken[MTD_NUMBER_FORMAT_L]      = sLCnt;
    aToken[MTD_NUMBER_FORMAT_G]      = sGCnt;
    aToken[MTD_NUMBER_FORMAT_D]      = sDCnt;
    aToken[MTD_COUNT_NINE]           = sIntNineCnt;
    aToken[MTD_COUNT_ZERO]           = sIntZeroCnt;
    aToken[MTD_COUNT_FLOAT_NINE]     = sFloatNineCnt;
    aToken[MTD_COUNT_FLOAT_ZERO]     = sFloatZeroCnt;
    aToken[MTD_COUNT_VNINE_ZERO]     = sAfterVNineZeroCnt;
    aToken[MTD_COUNT_FIRST_S]        = sIsFirstS;
    aToken[MTD_COUNT_FIRST_PERIOD]   = sIsFirstPeriod;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC applyFormat( SChar       * aString,
                    SInt          aStringLen,
                    UChar       * aFormat,
                    UInt          aFormatLen,
                    UChar       * aResult,
                    UInt        * aResultLen,
                    UChar       * aToken,
                    mtlCurrency * aCurrency,
                    idBool        aIsMinus )
{
/***********************************************************************
 *
 * Description : number format의 형태로 string을 변환한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UShort sDollarCnt         = aToken[MTD_NUMBER_FORMAT_DOLLAR];
    UShort sEEEECnt           = aToken[MTD_NUMBER_FORMAT_EEEE];
    UShort sMICnt             = aToken[MTD_NUMBER_FORMAT_MI];
    UShort sPRCnt             = aToken[MTD_NUMBER_FORMAT_PR];
    UShort sSCnt              = aToken[MTD_NUMBER_FORMAT_S];

    SInt   sIntNineCnt        = aToken[MTD_COUNT_NINE];
    SInt   sIntZeroCnt        = aToken[MTD_COUNT_ZERO];
    SInt   sFloatNineCnt      = aToken[MTD_COUNT_FLOAT_NINE];
    SInt   sFloatZeroCnt      = aToken[MTD_COUNT_FLOAT_ZERO];

    idBool sIsPeriod          = ID_FALSE;
    idBool sIsFirstS          = (idBool) aToken[MTD_COUNT_FIRST_S];
    idBool sIsFloatZero       = ID_FALSE;

    SInt sIntNumCnt           = 0;
    SInt sFloatNumCnt         = 0;
    SInt sFloatInvalidNumCnt  = 0;

    UShort sIterator          = 0;
    UChar  sTemp[MTD_NUMBER_FORMAT_BUFFER_LEN];
    UChar* sResult            = aResult;
    SInt   sResultLen         = 0;
    SChar* sString            = NULL;
    SInt   sStringLen         = 0;

    UInt   sResultIndex       = 0;
    UInt   sFormatIndex       = 0;
    UInt   sStringIndex       = 0;

    sString                   = aString;
    sStringLen                = aStringLen;

    UChar* sFormat            = NULL;
    UInt   sFormatLen         = 0;

    sFormat                   = aFormat;
    sFormatLen                = aFormatLen;
    sResultLen                = sFormatLen;

    // string에서 정수 부분의 숫자 개수를 센다.
    for ( sIterator = 0; sIterator < aStringLen; sIterator++ )
    {
        if ( *( sString + sIterator ) >= '0' &&
             *( sString + sIterator ) <= '9' )
        {
            if ( sIsPeriod == ID_FALSE )
            {
                sIntNumCnt++;
            }
            else
            {
                sFloatNumCnt++;
                if ( *( sString + sIterator ) != '0' )
                {
                    sIsFloatZero = ID_FALSE;
                }
                else
                {
                    if ( sIsFloatZero == ID_TRUE )
                    {
                        // 정수부분이 없을 경우 유효숫자가 나오기 전의 0의 개수
                        // 0.00234같은 경우, 2개
                        sFloatInvalidNumCnt++;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
        }

        if ( *( sString + sIterator ) == '.' )
        {
            sIsPeriod = ID_TRUE;
            sIsFloatZero = ID_TRUE;
        }
    }

    // format string을 result string에 맞게 변형시킨다.
    if ( sEEEECnt == 0 )
    {
        // fmt의 유효숫자(소수점 위)가 더 적으면 #으로 채워서 return
        if ( sIntNumCnt > ( sIntNineCnt + sIntZeroCnt ) )
        {
            idlOS::memset( sResult,
                           '#',
                           aFormatLen + 1 );
            *aResultLen = aFormatLen + 1;

            return IDE_SUCCESS;
        }
    }
    else
    {
        idlOS::memset( sResult,
                       ' ',
                       1 );
        sResultIndex++;
        sResultLen++;
    }

    aToken[MTD_COUNT_INTEGER]       = sIntNumCnt;
    aToken[MTD_COUNT_FLOAT]         = sFloatNumCnt;
    aToken[MTD_COUNT_INVALID_FLOAT] = sFloatInvalidNumCnt;

    // format string을 result string에 맞게 변형시킨다.
    // 부호 기호가 하나도 없을 경우 맨 앞에 임시로 문자를 하나
    // 넣어놓음.
    if ( ( sSCnt + sMICnt + sPRCnt) == 0 )
    {
        idlOS::memset( sResult + sResultIndex,
                       '@',
                       1 );
        sResultIndex++;
        sResultLen++;
    }
    else
    {
        if ( sSCnt == 1 && sIsFirstS == ID_FALSE )
        {
            idlOS::memset( sResult + sResultIndex,
                           'O',
                           1 );
            sResultIndex++;
            sResultLen++;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sPRCnt == 1 )
        {
            idlOS::memcpy( sResult + sResultIndex,
                           sFormat + sFormatLen - 2,
                           1 );
            sResultIndex++;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sMICnt == 1 )
        {
            idlOS::memcpy( sResult + sResultIndex,
                           sFormat + sFormatLen - 2,
                           1 );
            sResultIndex++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    // '$'를 부호 다음에 추가
    // S가 맨앞에 나오는 format 형태는 나중에 $기호를 처리한다.
    if ( sDollarCnt == 1 )
    {
        if ( sIsFirstS == ID_FALSE )
        {
            idlOS::memset( sResult + sResultIndex,
                           '$',
                           1 );
            sResultIndex++;
        }
        else
        {
            // nothing to do
        }
    }

    for ( ; sFormatIndex < sFormatLen; sFormatIndex++ )
    {
        if ( *( sFormat + sFormatIndex ) == '$' )
        {
            if ( sIsFirstS == ID_FALSE )
            {
                // Nothing to do
            }
            else
            {
                if ( sFormatIndex == 1 )
                {
                    // S는 숫자 표현 형식의 맨 앞이나 맨 뒤에만 올 수 있다.
                    // $는 숫자 앞에만 올 수 있다.
                    // 따라서, S$로 시작하는 경우는 정상이다.
                    idlOS::memcpy( sResult + sResultIndex,
                                   sFormat + sFormatIndex,
                                   1 );
                    sResultIndex++;
                }
                else
                {
                    // S와 $ 사이의 format을 $ 뒤로 보내어, S$로 시작하게 만든다.

                    if ( sEEEECnt == 0 )
                    {
                        IDE_ASSERT_MSG( sFormatIndex >= 2,
                                        "sFormatIndex : %"ID_UINT32_FMT"\n",
                                        sFormatIndex );

                        // 예를 들면, 'S999$'를 'S$999'로 변환한다.
                        idlOS::memcpy( sTemp, sResult, sFormatIndex );
                        idlOS::memcpy( sResult, sTemp, 1 );
                        idlOS::memset( sResult + 1, '$', 1 );
                        idlOS::memcpy( sResult + 2, sTemp + 1, sFormatIndex - 1 );
                    }
                    else
                    {
                        IDE_ASSERT_MSG( sResultIndex >= 3,
                                        "sResultIndex : %"ID_UINT32_FMT"\n",
                                        sResultIndex );

                        // EEEE format이 있을 경우, 맨 앞이 공백임.
                        // 예를 들면, ' S999$'를 ' S$999'로 변환한다.
                        idlOS::memcpy( sTemp, sResult, sResultIndex );
                        idlOS::memcpy( sResult, sTemp, 2 );
                        idlOS::memset( sResult + 2, '$', 1 );
                        idlOS::memcpy( sResult + 3, sTemp + 2, sResultIndex - 2 );
                    }
                    sResultIndex++;
                }
            }
        }
        else if ( ( idlOS::strCaselessMatch( sFormat + sFormatIndex, 1,
                                             "O", 1 ) == 0 ) ||
                  ( idlOS::strCaselessMatch( sFormat + sFormatIndex, 1,
                                             "P", 1 ) == 0 ) ||
                  ( idlOS::strCaselessMatch( sFormat + sFormatIndex, 1,
                                             "M", 1 ) == 0 ) )
        {
            // Nothing to do
        }
        else if ( idlOS::strCaselessMatch( sFormat + sFormatIndex, 1,
                                           "B", 1 ) == 0 )
        {
            sResultLen--;
        }
        else
        {
            idlOS::memcpy( sResult + sResultIndex,
                           sFormat + sFormatIndex,
                           1 );
            sResultIndex++;
        }
    }
    sResultIndex = 0;

    if ( sIntNumCnt == 0 )
    {
        if ( ( sFloatNineCnt + sFloatZeroCnt ) == 0 )
        {
            sIntNumCnt = 1;
        }
    }

    aToken[MTD_INDEX_STRING]     = sStringIndex;
    aToken[MTD_INDEX_STRING_LEN] = sStringLen;
    aToken[MTD_INDEX_FORMAT]     = sFormatIndex;
    aToken[MTD_INDEX_FORMAT_LEN] = sFormatLen;
    aToken[MTD_INDEX_RESULT]     = sResultIndex;
    aToken[MTD_INDEX_RESULT_LEN] = sResultLen;

    if ( sEEEECnt == 0 )
    {
        // EEEE format이 없을 경우, format 적용
        IDE_TEST( applyNonEEEEFormat( aString,
                                      aStringLen,
                                      aResult,
                                      aResultLen,
                                      aToken,
                                      aCurrency,
                                      aIsMinus )
                  != IDE_SUCCESS );
    }
    else
    {
        // EEEE format이 있을 경우, format 적용
        IDE_TEST( applyEEEEFormat( aString,
                                   aStringLen,
                                   aResult,
                                   aResultLen,
                                   aToken,
                                   aCurrency,
                                   aIsMinus )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC applyNonEEEEFormat( SChar       * aString,
                           SInt          aStringLen,
                           UChar       * aResult,
                           UInt        * aResultLen,
                           UChar       * aToken,
                           mtlCurrency * aCurrency,
                           idBool        aIsMinus )
{
/***********************************************************************
 *
 * Description : EEEE format이 없는 경우 aString을 aFormat에 맞게
                 변환한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UShort  sBCnt              = aToken[MTD_NUMBER_FORMAT_B];
    UShort  sDCnt              = aToken[MTD_NUMBER_FORMAT_D];

    SInt    sIntNineCntTemp    = aToken[MTD_COUNT_NINE];
    SInt    sIntZeroCnt        = aToken[MTD_COUNT_ZERO];
    SInt    sIntZeroCntTemp    = aToken[MTD_COUNT_ZERO];

    idBool  sIsStart           = ID_FALSE;
    idBool  sIsStartV          = ID_FALSE;

    SInt    sIntNumCnt         = aToken[MTD_COUNT_INTEGER];
    SInt    sFloatCnt          = 0;
    UShort  sIterator          = 0;
    UChar   sTemp[MTD_NUMBER_FORMAT_BUFFER_LEN];
    UChar * sResult            = aResult;
    SInt    sResultLen         = 0;
    SChar * sString            = NULL;
    SInt    sStringLen         = 0;
    UChar * sChar              = NULL;

    SInt   sResultIndex       = 0;
    SInt   sStringIndex       = 0;
    SInt   sTempIndex         = 0;
    SInt   sTempLen           = 0;
    SInt   sCount             = 0;

    sString                   = aString;
    sStringLen                = aStringLen;

    sStringIndex              = aToken[MTD_INDEX_STRING];
    sStringLen                = aToken[MTD_INDEX_STRING_LEN];
    sResultIndex              = aToken[MTD_INDEX_RESULT];
    sResultLen                = aToken[MTD_INDEX_RESULT_LEN];

    sFloatCnt     = aToken[MTD_COUNT_FLOAT_NINE] + aToken[MTD_COUNT_FLOAT_ZERO];

    // str1이 0일 경우, 0을 나타내기 위해서 sIntNumCnt을 1로 한다.
    if ( sBCnt == 0 && sStringLen == 2 && sFloatCnt == 0 )
    {
        sIntNumCnt = 1;
    }

    // 정수 부분을 쓰기 이전의 숫자 중, 0이 나오기
    // 전까지의 9와 ,를 맨 앞으로 보낸다.
    sCount = sIntNineCntTemp + sIntZeroCnt;
    sChar = sResult + sResultIndex;

    while ( sCount >= sIntNumCnt )
    {
        if ( sCount > sIntNumCnt )
        {
            if ( ( *sChar == '0' ) || ( *sChar == '.' ) ||
                 ( idlOS::strCaselessMatch( sChar, 1, "D", 1 ) == 0 ) )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( ( *sChar != ',' ) &&
                 ( idlOS::strCaselessMatch( sChar, 1, "G", 1 ) != 0 ) )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( ( sIntNineCntTemp == 0 ) &&
             ( *sChar != ',' ) &&
             ( idlOS::strCaselessMatch( sChar, 1, "G", 1 ) != 0 ) )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( *sChar == '9' ) || ( *sChar == ',' ) ||
             ( idlOS::strCaselessMatch( sChar, 1, "G", 1 ) == 0 ))

        {
            if ( *sChar == '9' )
            {
                sIntNineCntTemp--;
                sCount = sIntNineCntTemp + sIntZeroCnt;
            }
            idlOS::memcpy( sTemp,
                           sResult,
                           sResultLen );

            idlOS::memcpy( sResult,
                           sResult + sResultIndex,
                           1 );

            idlOS::memcpy( sResult + 1,
                           sTemp,
                           sResultIndex );
        }

        if ( sResultIndex < sResultLen - 1 )
        {
            sResultIndex++;
            sChar = sResult + sResultIndex;
        }
        else
        {
            break;
        }
    }

    sResultIndex = 0;
    sStringIndex++;

    while ( sResultIndex < sResultLen )
    {
        if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "@", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "S", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "O", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "P", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "M", 1 ) == 0 ) )
        {
            sIsStart = ID_TRUE;
        }

        if ( sStringLen == 0 &&
             ( ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                          "@", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "S", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "O", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "P", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "M", 1 ) == 0 ) ) )
        {
            sIsStart = ID_TRUE;
        }

        if ( sIsStart == ID_FALSE )
        {
            idlOS::memset( sResult + sResultIndex,
                           ' ',
                           1 );
        }
        else
        {
            if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                            "@", 1 ) == 0 ) )
            {
                if ( sBCnt == 0 || sStringLen != 2 )
                {
                    if ( *aString == '+' )
                    {
                        if ( aIsMinus == ID_TRUE )
                        {
                            idlOS::memset( sResult + sResultIndex,
                                           '-',
                                           1 );
                        }
                        else
                        {
                            idlOS::memset( sResult + sResultIndex,
                                           ' ',
                                           1 );
                        }
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '-',
                                       1 );
                    }
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "S", 1 ) == 0 )
            {
                if ( sBCnt == 0 || sStringLen != 2 )
                {
                    if ( *aString == '+' && aIsMinus == ID_FALSE )
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '+',
                                       1 );
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '-',
                                       1 );
                    }
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "P", 1 ) == 0 )
            {
                if ( sBCnt == 0 || sStringLen != 2 )
                {
                    if ( *aString == '+' && aIsMinus == ID_FALSE )
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       ' ',
                                       1 );
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '<',
                                       1 );
                    }
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "R", 1 ) == 0 )
            {
                if ( sBCnt == 0 || sStringLen != 2 )
                {
                    if ( *aString == '+' && aIsMinus == ID_FALSE )
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       ' ',
                                       1 );
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '>',
                                       1 );
                    }
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "I", 1 ) == 0 )
            {
                if ( sBCnt == 0 || sStringLen != 2 )
                {
                    if ( *aString == '+' && aIsMinus == ID_FALSE )
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       ' ',
                                       1 );
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '-',
                                       1 );
                    }
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "O", 1 ) == 0 ) ||
                      ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "M", 1 ) == 0 ) )
            {
                for ( sIterator = 0;
                      sIterator < ( sResultLen - sResultIndex - 1 );
                      sIterator++ )
                {
                    idlOS::memcpy( sResult + sResultIndex + sIterator,
                                   sResult + sResultIndex + sIterator + 1,
                                   1 );
                }
                sResultLen--;
                continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "V", 1 ) == 0 ) )
            {
                sIsStartV = ID_TRUE;

                for ( sIterator = 0;
                      sIterator < ( sResultLen - sResultIndex - 1 );
                      sIterator++ )
                {
                    idlOS::memcpy( sResult + sResultIndex + sIterator,
                                   sResult + sResultIndex + sIterator + 1,
                                   1 );
                }
                sResultLen--;
                continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "C", 1 ) == 0 ) )
            {
                idlOS::memcpy( sTemp, sResult, sResultLen );
                idlOS::memcpy( sResult + sResultIndex,
                               aCurrency->C,
                               3 );
                sTempIndex = sResultIndex + 1;
                sResultIndex += 3;

                if ( sResultLen > sTempIndex )
                {
                    idlOS::memcpy( sResult + sResultIndex,
                                   sTemp + sTempIndex,
                                   sResultLen - sTempIndex );
                }
                else
                {
                    /* Nothing to do */
                }
                sResultLen += 2 ;
                continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "L", 1 ) == 0 ) )
            {
                sTempLen = idlOS::strlen( aCurrency->L );
                idlOS::memcpy( sTemp, sResult, sResultLen );
                idlOS::memcpy( sResult + sResultIndex,
                               aCurrency->L,
                               sTempLen );
                sTempIndex = sResultIndex + 1;
                sResultIndex += sTempLen;

                if ( sResultLen > sTempIndex )
                {
                    idlOS::memcpy( sResult + sResultIndex,
                                   sTemp + sTempIndex,
                                   sResultLen - sTempIndex );
                }
                else
                {
                    /* Nothing to do */
                }
                sResultLen += sTempLen - 1 ;
                continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "G", 1 ) == 0 ) )
            {
                idlOS::memcpy( sResult + sResultIndex,
                               &aCurrency->G,
                               1 );
            }
            else if ( sIsStartV == ID_TRUE &&
                      ( *( sResult + sResultIndex ) == '9' ||
                        *( sResult + sResultIndex ) == '0' ) )
            {
                if ( sBCnt == 1 && sStringLen == 2 )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
                else
                {
                    if ( *( sString + sStringIndex ) == '.' )
                    {
                        sStringIndex++;
                        continue;
                    }

                    if ( sStringIndex >= aStringLen )
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '0',
                                       1 );
                    }
                    else
                    {
                        idlOS::memcpy( sResult + sResultIndex,
                                       sString + sStringIndex,
                                       1 );
                        sStringIndex++;
                    }
                }
            }
            else if ( *( sResult + sResultIndex ) == '9' ||
                      *( sResult + sResultIndex ) == '0' ||
                      *( sResult + sResultIndex ) == '.' ||
                       ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                  "D", 1 ) == 0 ) )
            {
                // B format이 있고, 숫자가 0일 경우
                if ( sBCnt == 1 && sStringLen == 2 )
                {
                    if (((*( sString + sStringIndex ) == '.' &&
                          *( sResult + sResultIndex ) == '.' )) ||
                         (*( sString + sStringIndex ) == '.' &&
                           ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                   "D", 1 ) == 0 ) ))
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       ' ',
                                       1 );
                        sStringIndex++;
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       ' ',
                                       1 );
                    }
                }
                else if ( sBCnt == 0 && sStringLen == 2 )
                {
                    if (((*( sString + sStringIndex ) == '.' &&
                          *( sResult + sResultIndex ) == '.' )) ||
                         (*( sString + sStringIndex ) == '.' &&
                           ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                   "D", 1 ) == 0 ) ))
                    {
                        if ( sDCnt == 0 )
                        {
                            idlOS::memset( sResult + sResultIndex, '.', 1 );
                        }
                        else
                        {
                            idlOS::memcpy( sResult + sResultIndex,
                                           &aCurrency->D, 1 );
                        }
                        sStringIndex++;
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '0',
                                       1 );
                    }
                }
                else if ( sStringIndex >= aStringLen )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '0',
                                   1 );
                }
                else if ( ( *( sResult + sResultIndex ) == '0' ||
                            *( sResult + sResultIndex ) == '9' ) &&
                          ( sIntZeroCntTemp + sIntNineCntTemp ) > sIntNumCnt )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '0',
                                   1 );
                    sIntZeroCntTemp--;
                }
                else
                {
                    if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                   "D", 1 ) == 0 )
                    {
                        idlOS::memcpy( sResult + sResultIndex,
                                       &aCurrency->D,
                                       1 );
                    }
                    else
                    {
                        idlOS::memcpy( sResult + sResultIndex,
                                       sString + sStringIndex,
                                       1 );
                    }
                    sStringIndex++;
                }
            }
            else
            {
                // nothing to do
            }
        }
        sResultIndex++;
    }
    *aResultLen = sResultLen;

    return IDE_SUCCESS;
}

IDE_RC applyEEEEFormat( SChar       * aString,
                        SInt          aStringLen,
                        UChar       * aResult,
                        UInt        * aResultLen,
                        UChar       * aToken,
                        mtlCurrency * aCurrency,
                        idBool        aIsMinus )
{
/***********************************************************************
 *
 * Description : EEEE format이 있는 경우 aString을 aFormat에 맞게
                 변환한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UShort sBCnt = aToken[MTD_NUMBER_FORMAT_B];

    SInt   sIntNineCnt        = aToken[MTD_COUNT_NINE];
    SInt   sIntZeroCnt        = aToken[MTD_COUNT_ZERO];
    SInt   sAfterVNineZeroCnt = aToken[MTD_COUNT_VNINE_ZERO];

    idBool sIsStart = ID_FALSE;
    idBool sIsStartV = ID_FALSE;

    SInt   sIntNumCnt          = aToken[MTD_COUNT_INTEGER];
    SInt   sFloatInvalidNumCnt = aToken[MTD_COUNT_INVALID_FLOAT];

    SInt   sExp = 0;
    UShort sIterator = 0;
    UShort sZeroIterator = 0;
    UChar  sTemp[MTD_NUMBER_FORMAT_BUFFER_LEN];
    UChar* sResult = aResult;
    SInt   sResultLen = 0;
    SChar* sString;
    SInt   sStringLen;

    SInt   sResultIndex = 0;
    SInt   sStringIndex = 0;
    SInt   sTempIndex   = 0;
    SInt   sTempLen     = 0;

    sString = aString;
    sStringLen = aStringLen;

    sStringIndex = aToken[MTD_INDEX_STRING];
    sStringLen   = aToken[MTD_INDEX_STRING_LEN];
    sResultIndex = aToken[MTD_INDEX_RESULT];
    sResultLen   = aToken[MTD_INDEX_RESULT_LEN];

    sZeroIterator = sAfterVNineZeroCnt;

    // str1이 0일 경우, 0을 나타내기 위해서 sIntNumCnt을 1로 한다.
    if ( sStringLen == 2 )
    {
        sIntNumCnt = 1;
    }

    // 지수 표기로 나타낼 때, 지수를 계산한다.
    // 0이 아니고, 0~1 사이의 소수일 때
    if ( sStringLen != 2 && *( sString + 1 ) == '.' )
    {
        sExp = sFloatInvalidNumCnt * (-1) - 1;
    }
    else
    {
        sExp = sIntNumCnt - 1;
    }

    // EEEE format이 있을 경우, V앞의 9 또는 0이 여러개일 경우
    // 하나만 남겨두고 삭제한다.
    idlOS::memcpy( sTemp,
                   sResult,
                   sResultLen );

    idlOS::memcpy( sResult,
                   sTemp,
                   3 );

    idlOS::memcpy( sResult + 3,
                   sTemp + sIntNineCnt + sIntZeroCnt + 2,
                   sResultLen - sIntNineCnt - sIntZeroCnt - 2 );

    sResultLen = sResultLen - sIntNineCnt - sIntZeroCnt + 1;

    sResultIndex = 0;
    sStringIndex++;

    while ( sStringIndex <= sStringLen &&
            ( *( sString + sStringIndex ) < '1' ||
              *( sString + sStringIndex ) > '9' ) )
    {
        sStringIndex++;
    }

    // B가 있고, str1이 0일 때는 sResultLen 만큼의 공백을 출력한다.
    // loop돌지 않음.
    while ( sResultIndex < sResultLen &&
            ( sBCnt == 0 || sStringLen != 2 ) )
    {
        if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "@", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "S", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "O", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "P", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                        "M", 1 ) == 0 ) )
        {
            sIsStart = ID_TRUE;
        }

        if ( sStringLen == 0 &&
             ( ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                          "@", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "S", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "O", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "P", 1 ) == 0 ) ||
             ( idlOS::strCaselessMatch( sResult + sResultIndex + 1, 1,
                                        "M", 1 ) == 0 ) ) )
        {
            sIsStart = ID_TRUE;
        }

        if ( sIsStart == ID_FALSE )
        {
            idlOS::memset( sResult + sResultIndex,
                           ' ',
                           1 );
        }
        else
        {
            if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                            "@", 1 ) == 0 ) )
            {
                if ( *aString == '+' )
                {
                    if ( aIsMinus == ID_TRUE )
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '-',
                                       1 );
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       ' ',
                                       1 );
                    }
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '-',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "S", 1 ) == 0 )
            {
                if ( *aString == '+' && aIsMinus == ID_FALSE )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '+',
                                   1 );
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '-',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "P", 1 ) == 0 )
            {
                if ( *aString == '+' && aIsMinus == ID_FALSE )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '<',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "R", 1 ) == 0 )
            {
                if ( *aString == '+' && aIsMinus == ID_FALSE )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
                else
                {

                    idlOS::memset( sResult + sResultIndex,
                                   '>',
                                   1 );
                }
            }
            else if ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "I", 1 ) == 0 )
            {
                if ( *aString == '+' && aIsMinus == ID_FALSE )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   ' ',
                                   1 );
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '-',
                                   1 );
                }
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "O", 1 ) == 0 ) ||
                      ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                               "M", 1 ) == 0 ) )
            {
                // L, M 제거
                for ( sIterator = 0;
                      sIterator < ( sResultLen - sResultIndex - 1 );
                      sIterator++ )
                {
                    idlOS::memcpy( sResult + sResultIndex + sIterator,
                                   sResult + sResultIndex + sIterator + 1,
                                   1 );
                }
                sResultLen--;
                continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "V", 1 ) == 0 ) )
            {
                sIsStartV = ID_TRUE;

                // V제거
                for ( sIterator = 0;
                      sIterator < ( sResultLen - sResultIndex - 1 );
                      sIterator++ )
                {
                    idlOS::memcpy( sResult + sResultIndex + sIterator,
                                   sResult + sResultIndex + sIterator + 1,
                                   1 );
                }
                sResultLen--;
                continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "C", 1 ) == 0 ) )
            {
                idlOS::memcpy( sTemp, sResult, sResultLen );
                idlOS::memcpy( sResult + sResultIndex,
                               aCurrency->C,
                               3 );
                sTempIndex = sResultIndex + 1;
                sResultIndex += 3;
                if ( sResultLen > sTempIndex )
                {
                    idlOS::memcpy( sResult + sResultIndex,
                                   sTemp + sTempIndex,
                                   sResultLen - sTempIndex );
                }
                else
                {
                    /* Nothing to do */
                }
                sResultLen += 2 ;
                continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "L", 1 ) == 0 ) )
            {
                 sTempLen = idlOS::strlen( aCurrency->L );
                 idlOS::memcpy( sTemp, sResult, sResultLen );
                 idlOS::memcpy( sResult + sResultIndex,
                                aCurrency->L,
                                sTempLen );
                 sTempIndex = sResultIndex + 1;
                 sResultIndex += sTempLen;

                 if ( sResultLen > sTempIndex )
                 {
                     idlOS::memcpy( sResult + sResultIndex,
                                    sTemp + sTempIndex,
                                    sResultLen - sTempIndex );
                 }
                 else
                 {
                     /* Nothing to do */
                 }
                 sResultLen += sTempLen - 1 ;
                 continue;
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                 "D", 1 ) == 0 ) )
            {
                idlOS::memcpy( sResult + sResultIndex,
                               &aCurrency->D, 1 );
            }
            else if ( sIsStartV == ID_TRUE &&
                      ( *( sResult + sResultIndex ) == '9' ||
                        *( sResult + sResultIndex ) == '0' ) )
            {
                if ( sStringLen - sStringIndex >= 1 )
                {
                    if ( ( *( sString + sStringIndex) == '.' ) ||
                         ( idlOS::strCaselessMatch( sResult + sResultIndex, 1,
                                                    "D", 1 ) == 0 ) )
                    {
                        sStringIndex++;
                    }
                }

                // 나타낼 수 있는 유효숫자의 개수를 초과한 경우
                // 초과한 V의 개수만큼 0을 추가한다.
                // 초과하지 않을 때까지는 string의 숫자를 그대로 가져온다.
                if ( sIntNumCnt != 0 &&
                     sZeroIterator <= sAfterVNineZeroCnt - ( aStringLen - 2 ) + 1 )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '0',
                                   1 );
                    sZeroIterator--;
                    sStringIndex++;
                }
                else if ( sIntNumCnt == 0 )
                {
                    if ( sZeroIterator > 0 &&
                         sStringIndex < aStringLen )

                    {
                        idlOS::memcpy( sResult + sResultIndex,
                                       sString + sStringIndex,
                                       1 );
                        sZeroIterator--;
                        sStringIndex++;
                    }
                    else
                    {
                        idlOS::memset( sResult + sResultIndex,
                                       '0',
                                       1 );
                        sZeroIterator--;
                        sStringIndex++;
                    }
                }
                else
                {
                    idlOS::memcpy( sResult + sResultIndex,
                                   sString + sStringIndex,
                                   1 );
                    sZeroIterator--;
                    sStringIndex++;
                }
            }
            else if ( ( idlOS::strCaselessMatch( sResult + sResultIndex, 4,
                                                 "EEEE", 4 ) == 0 ) )
            {
                idlOS::memset( sResult + sResultIndex,
                               'E',
                               1 );

                if ( sExp >= 0 )
                {
                    idlOS::memset( sResult + sResultIndex + 1,
                                   '+',
                                   1 );
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex + 1,
                                   '-',
                                   1 );
                    sExp = sExp * (-1);
                }

                // exponent가 세 자리일 경우
                // result string의 크기를 하나 늘려야 한다.
                if ( sExp > 99 )
                {
                    sResultIndex += 2;

                    idlOS::memcpy( sTemp,
                                   sResult,
                                   sResultLen );

                    idlOS::memcpy( sResult,
                                   sTemp,
                                   sResultIndex );

                    idlOS::memset( sResult + sResultIndex,
                                   'E',
                                   1 );

                    idlOS::memcpy( sResult + sResultIndex + 1,
                                   sTemp + sResultIndex,
                                   sResultLen - sResultIndex );

                    sResultLen++;

                    idlOS::memset( sResult + sResultIndex,
                                   ( sExp / 100 ) + '0',
                                   1 );

                    idlOS::memset( sResult + sResultIndex + 1,
                                   ( sExp / 10 ) - ( ( sExp / 100 ) * 10 ) + '0',
                                   1 );

                    idlOS::memset( sResult + sResultIndex + 2,
                                   ( sExp % 10 ) + '0',
                                   1 );
                    sResultIndex += 2;
                }
                else
                {
                    idlOS::memset( sResult + sResultIndex + 2,
                                   sExp / 10 + '0',
                                   1 );

                    idlOS::memset( sResult + sResultIndex + 3,
                                   sExp % 10 + '0',
                                   1 );
                    sResultIndex += 3;
                }
            }
            else if ( *( sResult + sResultIndex ) == '9' ||
                      *( sResult + sResultIndex ) == '0' )
            {
                if ( sBCnt == 0 && sStringLen == 2 )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '0',
                                   1 );
                }
                else if ( sStringIndex >= aStringLen )
                {
                    idlOS::memset( sResult + sResultIndex,
                                   '0',
                                   1 );
                }
                else
                {

                    if ( ( *( sString + sStringIndex) == '.' ) ||
                         ( idlOS::strCaselessMatch( sString + sStringIndex, 1,
                                                    "D", 1 ) == 0 ) )
                    {
                        if ( sStringIndex != aStringLen )
                        {
                            sStringIndex++;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    idlOS::memcpy( sResult + sResultIndex,
                                   sString + sStringIndex,
                                   1 );
                    sStringIndex++;
                }
            }
            else
            {
                // nothing to do
            }
        }
        sResultIndex++;
    }

    if ( sBCnt == 1 && sStringLen == 2 )
    {
        idlOS::memset( sResult,
                       ' ',
                       sResultLen );
    }
    *aResultLen = sResultLen;

    return IDE_SUCCESS;
}

IDE_RC compXXXXandRN( UChar* aNumFmt,
                      UInt aNumFmtLen,
                      UChar* aResultValue,
                      UShort* aResultLen,
                      SInt aIntNum )
{
/***********************************************************************
 *
 * Description : compXXXXandRN
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort          sRNCharCnt = 0;
    SChar           sRnTemp[MTD_NUMBER_FORMAT_BUFFER_LEN];
    SInt            sIterator;
    UChar*          sNumFmt = aNumFmt;
    UInt            sNumFmtLen = aNumFmtLen;
    SInt            sIntNum = aIntNum;
    SInt            sIndex = 0;
    UChar           sChr = 0;
    UChar           sIsEmpty = 1;

    if ( sNumFmtLen >= 4 )
    {
        if ( idlOS::strMatch( sNumFmt, 4, "XXXX", 4 ) == 0 )
        {
            for( sIterator = 0 ; sIterator < 8 ; sIterator++ )
            {
                sChr = ( ( sIntNum << (sIterator*4)) >> 28 ) & 0xF;

                if( sChr != 0 || sIsEmpty == 0)
                {
                    sIsEmpty = 0;
                    if( sChr < 10 )
                    {
                        aResultValue[sIndex] = sChr + '0';
                    }
                    else
                    {
                        aResultValue[sIndex] = sChr + 'A' - 10;
                    }
                    sIndex++;
                }
            }
            *aResultLen = sIndex;
        }
        else if ( idlOS::strCaselessMatch( sNumFmt, 4, "XXXX", 4 ) == 0 )
        {
            for( sIterator = 0 ; sIterator < 8 ; sIterator++ )
            {
                sChr = ( ( sIntNum << (sIterator*4)) >> 28 ) & 0xF;

                if( sChr != 0 || sIsEmpty == 0)
                {
                    sIsEmpty = 0;
                    if( sChr < 10 )
                    {
                        aResultValue[sIndex] = sChr + '0';
                    }
                    else
                    {
                        aResultValue[sIndex] = sChr + 'a' - 10;
                    }
                    sIndex++;
                }
            }
            *aResultLen = sIndex;
        }
    }
    else if ( sNumFmtLen >= 2 )
    {
        if ( idlOS::strMatch( sNumFmt, 2, "RN", 2 ) == 0 )
        {
            if ( sIntNum > 3999 || sIntNum <= 0 )
            {
                IDE_RAISE ( ERR_INVALID_LENGTH );
            }

            IDE_TEST( convertToRoman( sIntNum, &sRNCharCnt, sRnTemp )
                      != IDE_SUCCESS );

            idlOS::memcpy( aResultValue,
                           sRnTemp,
                           sRNCharCnt );

            *aResultLen = sRNCharCnt;
        }
        else if ( idlOS::strCaselessMatch( sNumFmt, 2, "RN", 2 ) == 0 )
        {
            if ( sIntNum > 3999 || sIntNum <= 0 )
            {
                IDE_RAISE ( ERR_INVALID_LENGTH );
            }

            IDE_TEST( convertToRoman( sIntNum, &sRNCharCnt, sRnTemp )
                      != IDE_SUCCESS );

            for ( sIterator = 0; sIterator < sRNCharCnt; sIterator++ )
            {
                if( *( sRnTemp + sIterator ) >= 'A' &&
                    *( sRnTemp + sIterator ) <= 'Z' )
                {
                    // 대문자인 경우, 소문자로 변환하여 결과에 저장
                    *( sRnTemp + sIterator ) = *( sRnTemp + sIterator ) + 0x20;
                }
                else
                {
                    *( sRnTemp + sIterator ) = *( sRnTemp + sIterator );
                }
            }

            idlOS::memcpy( aResultValue,
                           sRnTemp,
                           sRNCharCnt );

            *aResultLen = sRNCharCnt;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfToCharInterface::mtfTo_charCalculateDateFor2Args(
    mtcNode*     aNode,
    mtcStack*    aStack,
    SInt         aRemain,
    void*        aInfo,
    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : To_Char Calculate
 *
 * Implementation :
 *    TO_CHAR( date, 'fmt' )
 *
 *    aStack[0] : 입력된 날짜 형식을 문자형으로 변환하여 출력
 *    aStack[1] : date
 *    aStack[2] : fmt ( char의 날짜 형식 )
 *
 *    ex) TO_CHAR( join_date, 'YYYY-MM-DD HH:MI::SS' )
 *       ==> '2005-JUN-09'
 *
 ***********************************************************************/

    mtdCharType*        sValue;
    mtdDateType*        sDate;
    mtdCharType*        sVarchar;
    UInt                sLength;

    SInt                sStringMaxLen;
    mtdFormatInfo*      sFormatInfo;
    mtfTo_charCalcInfo* sCalcInfo;
    UShort              sFormatCount;
    UShort              sIterator;
    SInt                sBufferCur;
    SChar*              sResult;
    //SInt                sResultLen;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue   = (mtdCharType*)aStack[0].value;
        sDate    = (mtdDateType*)aStack[1].value;
        sVarchar = (mtdCharType*)aStack[2].value;

        // Estimate 단계에서 format 정보를 알고 있을 경우
        // format 정보 대신 aInfo 정보를 이용한다.
        if ( aInfo != NULL )
        {
            sStringMaxLen = aStack[0].column->precision;
            sFormatInfo = (mtdFormatInfo*) aInfo;
            sFormatCount = sFormatInfo->count;
            sBufferCur = 0;

            sResult = (SChar*) sValue->value;
            //sResultLen = sValue->length;

            for ( sIterator = 0, sCalcInfo = sFormatInfo->format;
                  sIterator < sFormatCount;
                  sIterator++, sCalcInfo++ )
            {
                IDE_TEST( sCalcInfo->applyDateFormat( sDate,
                                                      sResult,
                                                      &sBufferCur,
                                                      &sStringMaxLen,
                                                      sCalcInfo->string,
                                                      sCalcInfo->isFillMode )
                          != IDE_SUCCESS );
            }

            sValue->length = IDL_MIN( sBufferCur, sStringMaxLen - 1 );

            // 간혹 snprintf에서 null termination을 하지 않을
            // 경우가 있을 수 있다하여
            sResult[sValue->length] = '\0';
        }
        else
        {
            IDE_TEST( mtdDateInterface::toChar(sDate,
                                               sValue->value,
                                               &sLength,
                                               aStack[0].column->precision,
                                               sVarchar->value,
                                               sVarchar->length )
                      != IDE_SUCCESS );

            sValue->length = (UShort)sLength;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfToCharInterface::makeFormatInfo( mtcNode*     aNode,
                                           mtcTemplate* aTemplate,
                                           UChar*       aFormat,
                                           UInt         aFormatLen,
                                           mtcCallBack* aCallBack )
{
    mtcExecute*         sExecute;
    SInt                sToken = -1;
    SChar*              sString;
    UInt                sStringLen;
    mtdFormatInfo*      sFormatInfo;
    yyscan_t            sScanner;
    idBool              sInitScanner = ID_FALSE;
    idBool              sIsFillMode = ID_FALSE;
    UChar              *sFormat;  //  에러 출력을 위해 현재 포맷의 위치를 추적함
    UInt                sFormatLen;
    UChar              *sErrorFormat;

    sExecute = &(aTemplate->rows[aNode->table].execute[aNode->column]);

    // calculateInfo에 format정보를 (sFormatInfo) 저장할 공간을 할당
    IDE_TEST(aCallBack->alloc( aCallBack->info,
                               ID_SIZEOF(mtdFormatInfo),
                               (void**)&( sExecute->calculateInfo ) )
                               != IDE_SUCCESS );

    sFormatInfo = (mtdFormatInfo*)(sExecute->calculateInfo);
    sFormatInfo->count = 0;

    // calculateInfo에 필요한 format의 개수만큼 공간을 할당
    // format의 개수는 최대 aFormatLen이다.
    IDE_TEST(aCallBack->alloc( aCallBack->info,
                               ID_SIZEOF(mtfTo_charCalcInfo) * aFormatLen,
                               (void**)&( sExecute->calculateInfo ) )
                               != IDE_SUCCESS );

    sFormatInfo->format = (mtfTo_charCalcInfo*) (sExecute->calculateInfo);

    IDE_TEST_RAISE( mtddllex_init ( &sScanner ) != 0, ERR_LEX_INIT_FAILED );
    sInitScanner = ID_TRUE;

    sFormat = aFormat;
    mtddl_scan_bytes ( (const char*)aFormat, aFormatLen, sScanner );

    // get first token
    sToken = mtddllex( sScanner );
    while( sToken != 0 )
    {
        sFormatInfo->format[sFormatInfo->count].applyDateFormat =
            gFormatFuncSet[sToken];

        // To fix BUG-17693
        sFormatInfo->format[sFormatInfo->count].isFillMode = ID_FALSE;

        if ( sToken == MTD_DATE_FORMAT_SEPARATOR )
        {
            // Get string length
            sStringLen = mtddlget_leng( sScanner );
            
            // calculateInfo에 format정보를 (sFormatInfo) 저장할 공간을 할당
            // SEPARATOR 혹은 공백의 크기 만큼 할당
            IDE_TEST(aCallBack->alloc( aCallBack->info,
                                       sStringLen + 1,
                                       (void**)&( sExecute->calculateInfo ) )
                                       != IDE_SUCCESS );

            sFormatInfo->format[sFormatInfo->count].string =
                (SChar*) sExecute->calculateInfo;
            sString = sFormatInfo->format[sFormatInfo->count].string;

            // MAX_PRECISION을 초과하지 않는 범위에서 문자열 복사
            sStringLen = IDL_MIN(MTC_TO_CHAR_MAX_PRECISION-1, sStringLen);
            idlOS::memcpy(sString,
                          mtddlget_text(sScanner),
                          sStringLen);
            sString[sStringLen] = 0;
        }
        else if ( sToken == MTD_DATE_FORMAT_DOUBLE_QUOTE_STRING )
        {
            // Get string length
            // "***"의 경우 양쪽에 double quote를 삭제할 것을 가정한 길이
            sStringLen = mtddlget_leng( sScanner ) - 2;
            
            // calculateInfo에 format정보를 (sFormatInfo) 저장할 공간을 할당
            IDE_TEST(aCallBack->alloc( aCallBack->info,
                                       sStringLen + 1,
                                       (void**)&( sExecute->calculateInfo ) )
                                       != IDE_SUCCESS );

            sFormatInfo->format[sFormatInfo->count].string =
                (SChar*) sExecute->calculateInfo;
            sString = sFormatInfo->format[sFormatInfo->count].string;

            // "***"의 경우 양쪽에 double quote를 삭제한다.
            // MAX_PRECISION을 초과하지 않는 범위에서 문자열 복사
            sStringLen = IDL_MIN(MTC_TO_CHAR_MAX_PRECISION-1, sStringLen);
            idlOS::memcpy(sString,
                          mtddlget_text(sScanner)+1,
                          sStringLen);
            sString[sStringLen] = 0;
        }
        else if ( sToken == MTD_DATE_FORMAT_NONE )
        {
            // BUG-19753
            // separator 혹은 quoted string이 아닌 인식되지 않은 포맷은 에러

            sFormatLen = aFormatLen - ( sFormat - aFormat );
            IDE_TEST(aCallBack->alloc( aCallBack->info,
                                       sFormatLen + 1,
                                       (void**)&sErrorFormat )
                                       != IDE_SUCCESS );            
            
            idlOS::memcpy( sErrorFormat,
                           sFormat,
                           sFormatLen );
            sErrorFormat[sFormatLen] = '\0';
            
            IDE_RAISE( ERR_DATE_NOT_RECOGNIZED_FORMAT );
        }
        else
        {
            // NONE, DOUBLE_QUOTE 포맷이 아닐때는 string이 필요없지만
            // 메모리 할당을 해주지 않으면 함수를 호출할 때, UMR 발생
            IDE_TEST(aCallBack->alloc( aCallBack->info,
                                       1,
                                       (void**)&( sExecute->calculateInfo ) )
                                       != IDE_SUCCESS );

            sFormatInfo->format[sFormatInfo->count].string =
                (SChar*) sExecute->calculateInfo;
            sFormatInfo->format[sFormatInfo->count].string[0] = 0;

            sFormatInfo->format[sFormatInfo->count].isFillMode = sIsFillMode;

            // To fix BUG-17693
            if ( sToken == MTD_DATE_FORMAT_FM )
            {
                sIsFillMode = ID_TRUE;
            }
            else
            {
                // Nothing to do
            }
        }
        
        sFormatInfo->count++;

        // 다음 포맷의 시작점을 가리키도록 포인터 이동
        sFormat += mtddlget_leng( sScanner );
        
        // get next token
        sToken = mtddllex( sScanner );
    }

    mtddllex_destroy ( sScanner );
    sInitScanner = ID_FALSE;

    // calculateInfo에는 mtdFormatInfo의 처음 주소를 다시 세팅해줘야 한다.
    (sExecute->calculateInfo) = sFormatInfo;

    if ( sToken == -1 )
    {
        IDE_RAISE( ERR_INVALID_LITERAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_DATE));
    }
    IDE_EXCEPTION( ERR_DATE_NOT_RECOGNIZED_FORMAT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_DATE_NOT_RECOGNIZED_FORMAT,
                                sErrorFormat));
    }
    IDE_EXCEPTION( ERR_LEX_INIT_FAILED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtfToCharInterface::makeFormatInfo",
                                  "Lex init failed" ));
    }
    IDE_EXCEPTION_END;

    if ( sInitScanner == ID_TRUE )
    {
        mtddllex_destroy ( sScanner );
    }

    return IDE_FAILURE;
}


IDE_RC removeLeadingBlank( SChar  * aResult,
                           UInt     aResultLen,
                           UInt   * aBlankCnt )
{
/***********************************************************************
 *
 * Description :
 *    BUG-17693
 *
 * Implementation :
 *
 *    TO_CHAR( number_type, 'number_format_model' )
 *
 *    'FM' format이 왔을 경우 왼쪽에 오는 공백을 모두 제거한다.
 *
 *
 ***********************************************************************/

    UInt    sIndex;
    UInt    i;

    for( sIndex = 0; sIndex < aResultLen; sIndex++ )
    {
        if( aResult[sIndex] == ' ' )
        {
            (*aBlankCnt)++;
        }
        else
        {
            break;
        }
    }

    if (*aBlankCnt != 0)
    {
        for( sIndex = (*aBlankCnt), i = 0;
             sIndex < aResultLen;
             sIndex++, i++ )
        {
            aResult[i] = aResult[sIndex];
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}
