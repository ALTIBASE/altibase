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
 * $Id: mtxFromVarcharTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromVarcharTo;

static mtxSerialExecuteFunc mtxFromVarcharToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ );

static IDE_RC mtxFromVarcharToDate( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToSmallint( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToInteger( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToBigint( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToReal( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToDouble( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToFloat( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToNumeric( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToNchar( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToNvarchar( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToNibble( mtxEntry ** aEntry );
static IDE_RC mtxFromVarcharToVarbyte( mtxEntry ** aEntry );

mtxModule mtxFromVarcharTo = {
    MTX_EXECUTOR_ID_VARCHAR,
    mtx::calculateNA,
    mtxFromVarcharToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromVarcharToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromVarcharToChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxFromVarcharToNumeric;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxFromVarcharToInteger;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxFromVarcharToSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxFromVarcharToFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxFromVarcharToReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxFromVarcharToDouble;

            break;

        case MTD_DATE_ID:
            sFunc = mtxFromVarcharToDate;

            break;

        case MTD_NIBBLE_ID:
            sFunc = mtxFromVarcharToNibble;

            break;

        case MTD_VARBYTE_ID:
            sFunc = mtxFromVarcharToVarbyte;

            break;

        case MTD_NVARCHAR_ID:
            sFunc = mtxFromVarcharToNvarchar;

            break;

        case MTD_NCHAR_ID:
            sFunc = mtxFromVarcharToNchar;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxFromVarcharToBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromVarcharToDate( mtxEntry ** aEntry )
{
    mtdDateType  * sReturn   = (mtdDateType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType  * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    SChar        * sArg2Val  = (SChar *)aEntry[2]->mAddress;
    idBool       * sArg3Val  = (idBool *)aEntry[3]->mAddress;
    PDL_Time_Value sTimevalue;
    struct tm      sLocaltime;
    time_t         sTime;
    SShort         sRealYear;
    UShort         sRealMonth;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_DATE( sReturn );
    }
    else
    {
        idlOS::memset( (void*)sReturn,
                       0x00,
                       ID_SIZEOF( mtdDateType ) );

        /* 날짜 초기화 */
        sReturn->year         = ID_SSHORT_MAX;
        sReturn->mon_day_hour = 0;
        sReturn->min_sec_mic  = 0;

        /* '일'은 1로 설정 */
        sReturn->mon_day_hour &= ~MTD_DATE_DAY_MASK;
        sReturn->mon_day_hour |= 1 << MTD_DATE_DAY_SHIFT;

        IDE_TEST( mtdDateInterface::toDate( sReturn,
                                            sArg1Val->value,
                                            sArg1Val->length,
                                            (UChar*)sArg2Val,
                                            idlOS::strlen( sArg2Val ) )
                  != IDE_SUCCESS );

        /* PROJ-1436 - dateFormat을 참조했음을 표시한다. */
        *sArg3Val = ID_TRUE;

        /* 년 또는 월이 세팅이 안된 경우에 현재 날짜로 다시 세팅해줘야 함. */
        if ( ( sReturn->year == ID_SSHORT_MAX ) ||
             ( mtdDateInterface::month( sReturn ) == 0 ) )
        {
            sTimevalue = idlOS::gettimeofday();
            sTime      = (time_t)sTimevalue.sec();

            idlOS::localtime_r( & sTime,
                                & sLocaltime );

            if ( sReturn->year == ID_SSHORT_MAX )
            {
                sRealYear = (SShort)sLocaltime.tm_year + 1900;
            }
            else
            {
                sRealYear = (SShort)sReturn->year;
            }

            if ( (UShort)(mtdDateInterface::month( sReturn ) ) == 0 )
            {
                sRealMonth = (UShort)sLocaltime.tm_mon + 1;
            }
            else
            {
                sRealMonth = (UShort)mtdDateInterface::month( sReturn );
            }

            /* year, month, day의 조합이 올바른지 체크하고, sDate에 다시 세팅해준다. */
            IDE_TEST( mtdDateInterface::checkYearMonthDayAndSetDateValue( sReturn,
                                                                          (SShort)sRealYear,
                                                                          (UChar)sRealMonth,
                                                                          mtdDateInterface::day( sReturn ) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToDate",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToSmallint( mtxEntry ** aEntry )
{
    mtdSmallintType * sReturn   = (mtdSmallintType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType     * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType     sBigint;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_SMALLINT( sReturn );
    }
    else
    {
        IDE_TEST( mtv::character2NativeN( sArg1Val,
                                          & sBigint )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( ( sBigint > MTD_SMALLINT_MAXIMUM ) ||
                        ( sBigint < MTD_SMALLINT_MINIMUM ),
                        ERR_VALUE_OVERFLOW );

        *sReturn = (mtdSmallintType)sBigint;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToInteger( mtxEntry ** aEntry )
{
    mtdIntegerType * sReturn   = (mtdIntegerType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType    * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType    sBigint;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_INTEGER( sReturn );
    }
    else
    {
        IDE_TEST( mtv::character2NativeN( sArg1Val,
                                          & sBigint )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( ( sBigint > MTD_INTEGER_MAXIMUM ) ||
                        ( sBigint < MTD_INTEGER_MINIMUM ),
                        ERR_VALUE_OVERFLOW );

        *sReturn = (mtdIntegerType)sBigint;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToBigint( mtxEntry ** aEntry )
{
    mtdBigintType * sReturn   = (mtdBigintType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType   * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType   sBigint;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_BIGINT( sReturn );
    }
    else
    {
        IDE_TEST( mtv::character2NativeN( sArg1Val,
                                          & sBigint )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( MTX_IS_NULL_BIGINT( sBigint ),
                        ERR_VALUE_OVERFLOW );

        *sReturn = sBigint;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToReal( mtxEntry ** aEntry )
{
    mtdRealType  * sReturn   = (mtdRealType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType  * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    mtdDoubleType  sDouble;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_REAL( sReturn );
    }
    else
    {
        IDE_TEST( mtv::character2NativeR( sArg1Val,
                                          & sDouble )
                  != IDE_SUCCESS );

        *sReturn = sDouble;

        IDE_TEST_RAISE( MTX_IS_NULL_REAL( sReturn ),
                        ERR_VALUE_OVERFLOW );

        /* To fix BUG-12281 - underflow 검사 */
        if ( idlOS::fabs( sDouble ) < MTD_REAL_MINIMUM )
        {
            *sReturn = 0;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToDouble( mtxEntry ** aEntry )
{
    mtdDoubleType * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType   * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_DOUBLE( sReturn );
    }
    else
    {
        IDE_TEST( mtv::character2NativeR( sArg1Val,
                                          sReturn )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( MTX_IS_NULL_DOUBLE( sReturn ),
                        ERR_VALUE_OVERFLOW );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToFloat( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType    * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_FLOAT( sReturn );
    }
    else
    {
        IDE_TEST( mtc::makeNumeric( sReturn,
                                    MTD_FLOAT_MANTISSA_MAXIMUM,
                                    sArg1Val->value,
                                    sArg1Val->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToNumeric( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType    * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_NUMERIC( sReturn );
    }
    else
    {
        IDE_TEST( mtc::makeNumeric( sReturn,
                                    MTD_FLOAT_MANTISSA_MAXIMUM,
                                    sArg1Val->value,
                                    sArg1Val->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToChar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType  * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    sReturn->length = sArg1Val->length;

    idlOS::memcpy( sReturn->value,
                   sArg1Val->value,
                   sArg1Val->length );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToNchar( mtxEntry ** aEntry )
{
    mtdNcharType * sReturn   = (mtdNcharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType  * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    mtcColumn    * sArg2Val  = (mtcColumn *)aEntry[2]->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    IDE_TEST( mtdNcharInterface::toNchar( sArg2Val->precision,
                                          (const mtlModule *) mtl::mDBCharSet,
                                          (const mtlModule *) mtl::mNationalCharSet,
                                          sArg1Val,
                                          sReturn )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToNchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToNvarchar( mtxEntry ** aEntry )
{
    mtdNcharType * sReturn   = (mtdNcharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType  * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    mtcColumn    * sArg2Val  = (mtcColumn *)aEntry[2]->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    IDE_TEST( mtdNcharInterface::toNchar( sArg2Val->precision,
                                          (const mtlModule *) mtl::mDBCharSet,
                                          (const mtlModule *) mtl::mNationalCharSet,
                                          sArg1Val,
                                          sReturn )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToNvarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToNibble( mtxEntry ** aEntry )
{
    mtdNibbleType * sReturn   = (mtdNibbleType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType   * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    SInt            sIterator;
    SInt            sCount;
    SInt            sFence;

    static const UChar sHex[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_NIBBLE( sReturn );
    }
    else
    {
        sFence = ( sArg1Val->length / 2 ) * 2;

        for ( sIterator = 0,
                  sCount = 0;
              sIterator < sFence;
              sIterator += 2,
                  sCount++ )
        {
            IDE_TEST_RAISE( ( sHex[ sArg1Val->value[ sIterator + 0 ] ] > 15 ) ||
                            ( sHex[ sArg1Val->value[ sIterator + 1 ] ] > 15 ),
                            ERR_INVALID_LITERAL );

            sReturn->value[ sCount ] =
                sHex[ sArg1Val->value[ sIterator + 0 ] ] * 16
                + sHex[ sArg1Val->value[ sIterator + 1 ] ];
        }

        if ( ( sArg1Val->length % 2 ) != 0 )
        {
            IDE_TEST_RAISE( sHex[ sArg1Val->value[ sIterator ] ] > 15,
                            ERR_INVALID_LITERAL );

            sReturn->value[ sCount ] =
                sHex[ sArg1Val->value[ sIterator ] ] * 16;
        }
        else
        {
            /* Nothing to do */
        }

        sReturn->length = sArg1Val->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToNibble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromVarcharToVarbyte( mtxEntry ** aEntry )
{
    mtdByteType * sReturn   = (mtdByteType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    SInt          sIterator;
    SInt          sCount;
    SInt          sFence;

    static const UChar sHex[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        MTX_SET_NULL_VARBYTE( sReturn );
    }
    else
    {
        sFence = ( sArg1Val->length / 2 ) * 2;

        for ( sIterator = 0,
                  sCount = 0;
              sIterator < sFence;
              sIterator += 2,
                  sCount++ )
        {
            IDE_TEST_RAISE( ( sHex[ sArg1Val->value[ sIterator + 0 ] ] > 15 ) ||
                            ( sHex[ sArg1Val->value[ sIterator + 1 ] ] > 15 ),
                            ERR_INVALID_LITERAL );

            sReturn->value[ sCount ] =
                sHex[ sArg1Val->value[ sIterator + 0 ] ] * 16
                + sHex[ sArg1Val->value[ sIterator + 1 ] ];
        }

        if ( ( sArg1Val->length % 2 ) != 0 )
        {
            IDE_TEST_RAISE( sHex[ sArg1Val->value[ sIterator ] ] > 15,
                            ERR_INVALID_LITERAL );

            sReturn->value[ sCount ] =
                sHex[ sArg1Val->value[ sIterator ] ] * 16;

            sCount++;
        }
        else
        {
            /* Nothing to do */
        }

        sReturn->length = sCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromVarcharToVarbyte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
