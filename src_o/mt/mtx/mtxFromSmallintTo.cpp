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
 * $Id: mtxFromSmallintTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromSmallintTo;

static mtxSerialExecuteFunc mtxFromSmallintToGetSerialExecute( UInt aMtd1Type,
                                                               UInt /* aMtd2Type */ );

static IDE_RC mtxFromSmallintToInteger( mtxEntry ** aEntry );
static IDE_RC mtxFromSmallintToBigint( mtxEntry ** aEntry );
static IDE_RC mtxFromSmallintToReal( mtxEntry ** aEntry );
static IDE_RC mtxFromSmallintToDouble( mtxEntry ** aEntry );
static IDE_RC mtxFromSmallintToFloat( mtxEntry ** aEntry );
static IDE_RC mtxFromSmallintToNumeric( mtxEntry ** aEntry );
static IDE_RC mtxFromSmallintToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromSmallintToVarchar( mtxEntry ** aEntry );

mtxModule mtxFromSmallintTo = {
    MTX_EXECUTOR_ID_SMALLINT,
    mtx::calculateNA,
    mtxFromSmallintToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromSmallintToGetSerialExecute( UInt aMtd1Type,
                                                               UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromSmallintToChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxFromSmallintToNumeric;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxFromSmallintToInteger;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxFromSmallintToFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxFromSmallintToReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxFromSmallintToDouble;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxFromSmallintToVarchar;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxFromSmallintToBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromSmallintToInteger( mtxEntry ** aEntry )
{
    mtdIntegerType * sReturn   = (mtdIntegerType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType  sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
    {
        MTX_SET_NULL_INTEGER( sReturn );
    }
    else
    {
        *sReturn = sArg1Val;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromSmallintToInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromSmallintToBigint( mtxEntry ** aEntry )
{
    mtdBigintType * sReturn   = (mtdBigintType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
    {
        MTX_SET_NULL_BIGINT( sReturn );
    }
    else
    {
        *sReturn = sArg1Val;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromSmallintToBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromSmallintToReal( mtxEntry ** aEntry )
{
    mtdRealType   * sReturn   = (mtdRealType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;
    mtdDoubleType   sDouble   = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
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
                                  "mtxFromSmallintToReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromSmallintToDouble( mtxEntry ** aEntry )
{
    mtdDoubleType  * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType  sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
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
                                  "mtxFromSmallintToDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromSmallintToFloat( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType  sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType    sBigint   = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
    {
        MTX_SET_NULL_FLOAT( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Numeric( sBigint,
                                        sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromSmallintToFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromSmallintToNumeric( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType  sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType    sBigint   = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
    {
        MTX_SET_NULL_NUMERIC( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Numeric( sBigint,
                                        sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromSmallintToNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromSmallintToChar( mtxEntry ** aEntry )
{
    mtdCharType   * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType   sBigint   = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
    {
        MTX_SET_NULL_CHAR( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Character( sBigint,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromSmallintToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromSmallintToVarchar( mtxEntry ** aEntry )
{
    mtdCharType   * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType   sBigint   = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
    {
        MTX_SET_NULL_VARCHAR( sReturn );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Character( sBigint,
                                          sReturn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromSmallintToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
