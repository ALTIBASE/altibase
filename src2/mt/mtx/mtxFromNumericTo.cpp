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
 * $Id: mtxFromNumericTo.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxFromNumericTo;

static mtxSerialExecuteFunc mtxFromNumericToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ );

static IDE_RC mtxFromNumericToSmallint( mtxEntry ** aEntry );
static IDE_RC mtxFromNumericToInteger( mtxEntry ** aEntry );
static IDE_RC mtxFromNumericToBigint( mtxEntry ** aEntry );
static IDE_RC mtxFromNumericToDouble( mtxEntry ** aEntry );
static IDE_RC mtxFromNumericToFloat( mtxEntry ** aEntry );
static IDE_RC mtxFromNumericToChar( mtxEntry ** aEntry );
static IDE_RC mtxFromNumericToVarchar( mtxEntry ** aEntry );

mtxModule mtxFromNumericTo = {
    MTX_EXECUTOR_ID_NUMERIC,
    mtx::calculateNA,
    mtxFromNumericToGetSerialExecute
};

static mtxSerialExecuteFunc mtxFromNumericToGetSerialExecute( UInt aMtd1Type,
                                                              UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxFromNumericToChar;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxFromNumericToInteger;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxFromNumericToSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxFromNumericToFloat;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxFromNumericToDouble;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxFromNumericToVarchar;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxFromNumericToBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxFromNumericToSmallint( mtxEntry ** aEntry )
{
    mtdSmallintType * sReturn   = (mtdSmallintType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType  * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType     sBigint;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NUMERIC( sArg1Val ) )
    {
        MTX_SET_NULL_SMALLINT( sReturn );
    }
    else
    {
        IDE_TEST( mtv::numeric2NativeN( sArg1Val,
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
                                  "mtxFromNumericToSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromNumericToInteger( mtxEntry ** aEntry )
{
    mtdIntegerType * sReturn   = (mtdIntegerType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType    sBigint;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NUMERIC( sArg1Val ) )
    {
        MTX_SET_NULL_INTEGER( sReturn );
    }
    else
    {
        IDE_TEST( mtv::numeric2NativeN( sArg1Val,
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
                                  "mtxFromNumericToInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromNumericToBigint( mtxEntry ** aEntry )
{
    mtdBigintType  * sReturn   = (mtdBigintType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    mtdBigintType    sBigint;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NUMERIC( sArg1Val ) )
    {
        MTX_SET_NULL_BIGINT( sReturn );
    }
    else
    {
        IDE_TEST( mtv::numeric2NativeN( sArg1Val,
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
                                  "mtxFromNumericToBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromNumericToDouble( mtxEntry ** aEntry )
{
    mtdDoubleType  * sReturn   = (mtdDoubleType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NUMERIC( sArg1Val ) )
    {
        MTX_SET_NULL_DOUBLE( sReturn );
    }
    else
    {
        mtc::numeric2Double( sReturn,
                             sArg1Val );

        IDE_TEST_RAISE( MTX_IS_NULL_DOUBLE( sReturn ),
                        ERR_VALUE_OVERFLOW );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromNumericToDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromNumericToFloat( mtxEntry ** aEntry )
{
    mtdNumericType * sReturn   = (mtdNumericType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    idlOS::memcpy( sReturn,
                   sArg1Val,
                   sArg1Val->length + 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromNumericToFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromNumericToChar( mtxEntry ** aEntry )
{
    mtdCharType    * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    UInt             sLength;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    /* PROJ-1365, fix BUG-12944
     *  계산된 결과값 또는 사용자에게 바로 입력받는 값은
     *  precision이 40일 수 있으므로 38일 때 보다 2자리를 더 필요로 한다.
     *  최대 45자리에서 47자리로 수정함.
     *  1(부호) + 40(precision) + 1(.) + 1(E) + 1(exponent부호) + 3(exponent값)
     *  = 47이 된다.
     */
    IDE_TEST( mtv::float2String( sReturn->value,
                                 47,
                                 & sLength,
                                 sArg1Val )
              != IDE_SUCCESS );

    sReturn->length = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromNumericToChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxFromNumericToVarchar( mtxEntry ** aEntry )
{
    mtdCharType    * sReturn   = (mtdCharType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    UInt             sLength;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    /* PROJ-1365, fix BUG-12944
     *  계산된 결과값 또는 사용자에게 바로 입력받는 값은
     *  precision이 40일 수 있으므로 38일 때 보다 2자리를 더 필요로 한다.
     *  최대 45자리에서 47자리로 수정함.
     *  1(부호) + 40(precision) + 1(.) + 1(E) + 1(exponent부호) + 3(exponent값)
     *  = 47이 된다.
     */
    IDE_TEST( mtv::float2String( sReturn->value,
                                 47,
                                 & sLength,
                                 sArg1Val )
              != IDE_SUCCESS );

    sReturn->length = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxFromNumericToVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
