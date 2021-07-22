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
 * $Id: mtxIsNotNull.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/
#include <mtx.h>

extern mtxModule mtxIsNotNull;

static mtxSerialExecuteFunc mtxIsNotNullGetSerialExecute( UInt aMtd1Type,
                                                          UInt /* aMtd2Type */ );

static IDE_RC mtxIsNotNullBoolean( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullDate( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullSmallint( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullInteger( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullBigint( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullReal( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullDouble( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullFloat( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullNumeric( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullChar( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullVarchar( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullNchar( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullNvarchar( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullInterval( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullNibble( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullBit( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullVarbit( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullByte( mtxEntry ** aEntry );
static IDE_RC mtxIsNotNullVarbyte( mtxEntry ** aEntry );

mtxModule mtxIsNotNull = {
    MTX_EXECUTOR_ID_IS_NOT_NULL,
    mtx::calculateNA,
    mtxIsNotNullGetSerialExecute
};

static mtxSerialExecuteFunc mtxIsNotNullGetSerialExecute( UInt aMtd1Type,
                                                          UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxIsNotNullChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxIsNotNullNumeric;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxIsNotNullInteger;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxIsNotNullSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxIsNotNullFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxIsNotNullReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxIsNotNullDouble;

            break;

        case MTD_DATE_ID:
            sFunc = mtxIsNotNullDate;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxIsNotNullVarchar;

            break;

        case MTD_INTERVAL_ID:
            sFunc = mtxIsNotNullInterval;

            break;

        case MTD_BOOLEAN_ID:
            sFunc = mtxIsNotNullBoolean;

            break;

        case MTD_BYTE_ID:
            sFunc = mtxIsNotNullByte;

            break;

        case MTD_NIBBLE_ID:
            sFunc = mtxIsNotNullNibble;

            break;

        case MTD_VARBYTE_ID:
            sFunc = mtxIsNotNullVarbyte;

            break;

        case MTD_VARBIT_ID:
            sFunc = mtxIsNotNullVarbit;

            break;

        case MTD_NVARCHAR_ID:
            sFunc = mtxIsNotNullNvarchar;

            break;

        case MTD_NCHAR_ID:
            sFunc = mtxIsNotNullNchar;

            break;

        case MTD_BIT_ID:
            sFunc = mtxIsNotNullBit;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxIsNotNullBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;

    }

    return sFunc;
}

static IDE_RC mtxIsNotNullBoolean( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBooleanType   sArg1Val  = *(mtdBooleanType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BOOLEAN( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullBoolean",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullDate( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDateType    * sArg1Val  = (mtdDateType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTD_DATE_IS_NULL( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullDate",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullSmallint( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType  sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullInteger( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType   sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullBigint( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType    sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullReal( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdRealType    * sArg1Val  = (mtdRealType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_REAL( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullDouble( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType  * sArg1Val  = (mtdDoubleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullFloat( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_FLOAT( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullNumeric( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NUMERIC( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullChar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType    * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_CHAR( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullVarchar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType    * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullNchar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNcharType   * sArg1Val  = (mtdNcharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NCHAR( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullNchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullNvarchar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNcharType   * sArg1Val  = (mtdNcharType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NVARCHAR( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullNvarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullInterval( mtxEntry ** aEntry )
{
    mtdBooleanType  * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntervalType * sArg1Val  = (mtdIntervalType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTD_INTERVAL_IS_NULL( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullInterval",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullNibble( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNibbleType  * sArg1Val  = (mtdNibbleType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NIBBLE( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullNibble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullBit( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBitType     * sArg1Val  = (mtdBitType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIT( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullBit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullVarbit( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBitType     * sArg1Val  = (mtdBitType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARBIT( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullVarbit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullByte( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdByteType    * sArg1Val  = (mtdByteType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BYTE( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullByte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxIsNotNullVarbyte( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdByteType    * sArg1Val  = (mtdByteType *)( aEntry[1] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARBYTE( sArg1Val ) )
    {
        *sReturn = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *sReturn = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxIsNotNullVarbyte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
