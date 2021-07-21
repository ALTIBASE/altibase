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
 * $Id: mtxFromDoubleTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromDoubleTo;

static mtxSerialExecuteFunc mtxFromDoubleToGetSerialExecute( UInt aMtd1Type,
                                                             UInt /* aMtd2Type */ );

static IDE_RC mtxFromDoubleToBigint( mtxEntry ** aEntry );
static IDE_RC mtxFromDoubleToReal( mtxEntry ** aEntry );
static IDE_RC mtxFromDoubleToFloat( mtxEntry ** aEntry );
static IDE_RC mtxFromDoubleToNumeric( mtxEntry ** aEntry );
static IDE_RC mtxFromDoubleToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromDoubleToVarchar( mtxEntry ** aEntry );
static IDE_RC mtxFromDoubleToInterval( mtxEntry ** aEntry );

mtxModule mtxFromDoubleTo = {
    MTX_EXECUTOR_ID_DOUBLE,
    mtx::calculateNA,
    mtxFromDoubleToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromDoubleToGetSerialExecute( UInt aMtd1Type,
                                                             UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromDoubleToChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxFromDoubleToNumeric;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxFromDoubleToFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxFromDoubleToReal;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxFromDoubleToVarchar;

            break;

        case MTD_INTERVAL_ID:
            sFunc = mtxFromDoubleToInterval;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxFromDoubleToBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromDoubleToBigint( mtxEntry ** aEntry )
{
    mtdBigintType * sReturn   = (mtdBigintType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType * sArg1Val  = (mtdDoubleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( sArg1Val ) )
    {
        MTX_SET_NULL_BIGINT( sReturn );
    }
    else
    {
        IDE_TEST_RAISE( ( *sArg1Val > MTD_BIGINT_MAXIMUM ) ||
                        ( *sArg1Val < MTD_BIGINT_MINIMUM ),
                        ERR_VALUE_OVERFLOW );

        *sReturn = *(mtdBigintType *)sArg1Val;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDoubleToBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromDoubleToReal( mtxEntry ** aEntry )
{
    mtdRealType * sReturn   = (mtdRealType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType sArg1Val  = *(mtdDoubleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( & sArg1Val ) )
    {
        MTX_SET_NULL_REAL( sReturn );
    }
    else
    {
        *sReturn = sArg1Val;

        IDE_TEST_RAISE( MTX_IS_NULL_REAL( sReturn ),
                        ERR_VALUE_OVERFLOW );

        /* To fix BUG-12281 - underflow 검사 */
        if ( idlOS::fabs( sArg1Val ) < MTD_REAL_MINIMUM )
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
                                  "mtxFromDoubleToReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromDoubleToFloat( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType    sArg1Val   = *(mtdDoubleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( & sArg1Val ) )
    {
        MTX_SET_NULL_FLOAT( sReturn );
    }
    else
    {
        IDE_TEST( mtc::makeNumeric( sReturn,
                                    sArg1Val )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDoubleToFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromDoubleToNumeric( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType    sArg1Val  = *(mtdDoubleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( & sArg1Val ) )
    {
        MTX_SET_NULL_NUMERIC( sReturn );
    }
    else
    {
        IDE_TEST( mtc::makeNumeric( sReturn,
                                    sArg1Val )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDoubleToNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromDoubleToChar( mtxEntry ** aEntry )
{
    mtdCharType * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType sArg1Val  = *(mtdDoubleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( & sArg1Val ) )
    {
        MTX_SET_NULL_CHAR( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeR2Character( sArg1Val,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDoubleToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromDoubleToVarchar( mtxEntry ** aEntry )
{
    mtdCharType * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType sArg1Val   = *(mtdDoubleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( & sArg1Val ) )
    {
        MTX_SET_NULL_VARCHAR( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeR2Character( sArg1Val,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDoubleToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromDoubleToInterval( mtxEntry ** aEntry )
{
    mtdIntervalType * sReturn   = (mtdIntervalType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType     sArg1Val  = *(mtdDoubleType *)( aEntry[1] + 1 )->mAddress;
    SDouble           sIntegralPart;
    SDouble           sFractionalPart;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( & sArg1Val ) )
    {
        MTX_SET_NULL_INTERVAL( sReturn );
    }
    else
    {
        sArg1Val *= 864e2;

        IDE_TEST_RAISE( MTX_IS_NULL_DOUBLE( & sArg1Val ),
                        ERR_VALUE_OVERFLOW );

        sFractionalPart = modf( sArg1Val,
                                & sIntegralPart );

        IDE_TEST_RAISE( ( sIntegralPart >  9e18 ) ||
                        ( sIntegralPart < -9e18 ),
                        ERR_VALUE_OVERFLOW );

        sReturn->second      = (SLong)sIntegralPart;

        sFractionalPart = modf( sFractionalPart * 1e6,
                                & sIntegralPart );

        sReturn->microsecond = (SLong)sIntegralPart;

        /* BUG-40967 - 오차 보정 (반올림) */
        if ( sFractionalPart >= 0.5 )
        {
            sReturn->microsecond++;

            if ( sReturn->microsecond == 1000000 )
            {
                sReturn->second++;
                sReturn->microsecond = 0;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else if ( sFractionalPart <= -0.5 )
        {
            sReturn->microsecond--;

            if ( sReturn->microsecond == -1000000 )
            {
                sReturn->second--;
                sReturn->microsecond = 0;
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
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromDoubleToInterval",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
