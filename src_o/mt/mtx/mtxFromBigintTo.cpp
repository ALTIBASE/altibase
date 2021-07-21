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
 * $Id: mtxFromBigintTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromBigintTo;

static mtxSerialExecuteFunc mtxFromBigintToGetSerialExecute( UInt aMtd1Type,
                                                             UInt /* aMtd2Type */ );

static IDE_RC mtxFromBigintToSmallint( mtxEntry ** aEntry );
static IDE_RC mtxFromBigintToInteger( mtxEntry ** aEntry );
static IDE_RC mtxFromBigintToReal( mtxEntry ** aEntry );
static IDE_RC mtxFromBigintToDouble( mtxEntry ** aEntry );
static IDE_RC mtxFromBigintToFloat( mtxEntry ** aEntry );
static IDE_RC mtxFromBigintToNumeric( mtxEntry ** aEntry );
static IDE_RC mtxFromBigintToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromBigintToVarchar( mtxEntry ** aEntry );

mtxModule mtxFromBigintTo = {
    MTX_EXECUTOR_ID_BIGINT,
    mtx::calculateNA,
    mtxFromBigintToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromBigintToGetSerialExecute( UInt aMtd1Type,
                                                             UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromBigintToChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxFromBigintToNumeric;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxFromBigintToInteger;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxFromBigintToSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxFromBigintToFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxFromBigintToReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxFromBigintToDouble;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxFromBigintToVarchar;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromBigintToSmallint( mtxEntry ** aEntry )
{
    mtdSmallintType * sReturn   = (mtdSmallintType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType     sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_SMALLINT( sReturn );
    }
    else
    {
        IDE_TEST_RAISE( ( sArg1Val > MTD_SMALLINT_MAXIMUM ) ||
                        ( sArg1Val < MTD_SMALLINT_MINIMUM ),
                        ERR_VALUE_OVERFLOW );

        *sReturn = sArg1Val;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromBigintToSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromBigintToInteger( mtxEntry ** aEntry )
{
    mtdIntegerType * sReturn   = (mtdIntegerType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType    sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_INTEGER( sReturn );
    }
    else
    {
        IDE_TEST_RAISE( ( sArg1Val > MTD_INTEGER_MAXIMUM ) ||
                        ( sArg1Val < MTD_INTEGER_MINIMUM ),
                        ERR_VALUE_OVERFLOW );

        *sReturn = sArg1Val;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromBigintToInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromBigintToReal( mtxEntry ** aEntry )
{
    mtdRealType * sReturn   = (mtdRealType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;
    mtdDoubleType sDouble   = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_REAL( sReturn );
    }
    else
    {
        *sReturn = sDouble;

        IDE_TEST_RAISE( MTX_IS_NULL_REAL( sReturn ),
                        ERR_VALUE_OVERFLOW );

        /* To fix BUG-12281 - underflow °Ë»ç */
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
                                  "mtxFromBigintToReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromBigintToDouble( mtxEntry ** aEntry )
{
    mtdDoubleType * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType   sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_DOUBLE( sReturn );
    }
    else
    {
        *sReturn = sArg1Val;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromBigintToDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromBigintToFloat( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType    sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_FLOAT( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Numeric( sArg1Val,
                                        sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromBigintToFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromBigintToNumeric( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType    sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_NUMERIC( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Numeric( sArg1Val,
                                        sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromBigintToNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromBigintToChar( mtxEntry ** aEntry )
{
    mtdCharType * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_CHAR( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Character( sArg1Val,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromBigintToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromBigintToVarchar( mtxEntry ** aEntry )
{
    mtdCharType * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar         sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        MTX_SET_NULL_VARCHAR( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Character( sArg1Val,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromBigintToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
