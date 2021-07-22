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
 * $Id: mtxFromIntegerTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromIntegerTo;

static mtxSerialExecuteFunc mtxFromIntegerToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ );

static IDE_RC mtxFromIntegerToSmallint( mtxEntry ** aEntry );
static IDE_RC mtxFromIntegerToBigint( mtxEntry ** aEntry );
static IDE_RC mtxFromIntegerToReal( mtxEntry ** aEntry );
static IDE_RC mtxFromIntegerToDouble( mtxEntry ** aEntry );
static IDE_RC mtxFromIntegerToFloat( mtxEntry ** aEntry );
static IDE_RC mtxFromIntegerToNumeric( mtxEntry ** aEntry );
static IDE_RC mtxFromIntegerToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromIntegerToVarchar( mtxEntry ** aEntry );

mtxModule mtxFromIntegerTo = {
    MTX_EXECUTOR_ID_INTEGER,
    mtx::calculateNA,
    mtxFromIntegerToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromIntegerToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromIntegerToChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxFromIntegerToNumeric;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxFromIntegerToSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxFromIntegerToFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxFromIntegerToReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxFromIntegerToDouble;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxFromIntegerToVarchar;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxFromIntegerToBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromIntegerToSmallint( mtxEntry ** aEntry )
{
    mtdSmallintType * sReturn   = (mtdSmallintType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType    sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromIntegerToBigint( mtxEntry ** aEntry )
{
    mtdBigintType * sReturn   = (mtdBigintType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType  sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromIntegerToReal( mtxEntry ** aEntry )
{
    mtdRealType  * sReturn   = (mtdRealType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;
    mtdDoubleType  sDouble   = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromIntegerToDouble( mtxEntry ** aEntry )
{
    mtdDoubleType * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar           sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType  sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromIntegerToFloat( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType   sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType    sBigint   = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromIntegerToNumeric( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType   sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType    sBigint   = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromIntegerToChar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType  sBigint   = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromIntegerToVarchar( mtxEntry ** aEntry )
{
    mtdCharType  * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar          sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType  sBigint   = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
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
                                  "mtxFromIntegerToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
