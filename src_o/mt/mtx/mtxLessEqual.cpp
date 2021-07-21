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
 * $Id: mtxLessEqual.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/
#include <mtx.h>

extern mtlModule mtlUTF16;

extern mtxModule mtxLessEqual;

static mtxSerialExecuteFunc mtxLessEqualGetSerialExecute( UInt aMtd1Type,
                                                          UInt /* aMtd2Type */ );

static IDE_RC mtxLessEqualBoolean( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualDate( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualSmallint( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualInteger( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualBigint( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualReal( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualDouble( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualFloat( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualNumeric( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualChar( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualVarchar( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualNchar( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualNcharUTF16( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualNvarchar( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualInterval( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualNibble( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualBit( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualVarbit( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualByte( mtxEntry ** aEntry );
static IDE_RC mtxLessEqualVarbyte( mtxEntry ** aEntry );

mtxModule mtxLessEqual = {
    MTX_EXECUTOR_ID_LESS_EQUAL,
    mtx::calculateNA,
    mtxLessEqualGetSerialExecute
};

static mtxSerialExecuteFunc mtxLessEqualGetSerialExecute( UInt aMtd1Type,
                                                          UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxLessEqualChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxLessEqualNumeric;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxLessEqualInteger;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxLessEqualSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxLessEqualFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxLessEqualReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxLessEqualDouble;

            break;

        case MTD_DATE_ID:
            sFunc = mtxLessEqualDate;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxLessEqualVarchar;

            break;

        case MTD_INTERVAL_ID:
            sFunc = mtxLessEqualInterval;

            break;

        case MTD_BOOLEAN_ID:
            sFunc = mtxLessEqualBoolean;

            break;

        case MTD_BYTE_ID:
            sFunc = mtxLessEqualByte;

            break;

        case MTD_NIBBLE_ID:
            sFunc = mtxLessEqualNibble;

            break;

        case MTD_VARBYTE_ID:
            sFunc = mtxLessEqualVarbyte;

            break;

        case MTD_VARBIT_ID:
            sFunc = mtxLessEqualVarbit;

            break;

        case MTD_NVARCHAR_ID:
            sFunc = mtxLessEqualNvarchar;

            break;

        case MTD_NCHAR_ID:
            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sFunc = mtxLessEqualNcharUTF16;
            }
            else
            {
                sFunc = mtxLessEqualNchar;
            }

            break;

        case MTD_BIT_ID:
            sFunc = mtxLessEqualBit;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxLessEqualBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxLessEqualBoolean( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBooleanType   sArg1Val  = *(mtdBooleanType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdBooleanType   sArg2Val  = *(mtdBooleanType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BOOLEAN( sArg1Val ) || MTX_IS_NULL_BOOLEAN( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val <= sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualBoolean",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualDate( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDateType    * sArg1Val  = (mtdDateType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdDateType    * sArg2Val  = (mtdDateType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTD_DATE_IS_NULL( sArg1Val ) || MTD_DATE_IS_NULL( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->year > sArg2Val->year )
        {
            sOrder = 1;
        }
        else if ( sArg1Val->year < sArg2Val->year )
        {
            sOrder = -1;
        }
        else
        {
            sOrder = 0;
        }

        IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

        if ( sArg1Val->mon_day_hour > sArg2Val->mon_day_hour )
        {
            sOrder = 1;
        }
        else if ( sArg1Val->mon_day_hour < sArg2Val->mon_day_hour )
        {
            sOrder = -1;
        }
        else
        {
            sOrder = 0;
        }

        IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

        if ( sArg1Val->min_sec_mic > sArg2Val->min_sec_mic )
        {
            sOrder = 1;
        }
        else if ( sArg1Val->min_sec_mic < sArg2Val->min_sec_mic )
        {
            sOrder = -1;
        }
        else
        {
            sOrder = 0;
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualDate",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualSmallint( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdSmallintType  sArg1Val  = *(mtdSmallintType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdSmallintType  sArg2Val  = *(mtdSmallintType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_SMALLINT( sArg1Val ) || MTX_IS_NULL_SMALLINT( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val <= sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualInteger( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntegerType   sArg1Val  = *(mtdIntegerType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdIntegerType   sArg2Val  = *(mtdIntegerType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_INTEGER( sArg1Val ) || MTX_IS_NULL_INTEGER( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val <= sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualBigint( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBigintType    sArg1Val  = *(mtdBigintType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdBigintType    sArg2Val  = *(mtdBigintType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIGINT( sArg1Val ) || MTX_IS_NULL_BIGINT( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val <= sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualReal( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdRealType    * sArg1Val  = (mtdRealType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdRealType    * sArg2Val  = (mtdRealType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_REAL( sArg1Val ) || MTX_IS_NULL_REAL( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( *sArg1Val <= *sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualDouble( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdDoubleType  * sArg1Val  = (mtdDoubleType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdDoubleType  * sArg2Val  = (mtdDoubleType *)( aEntry[2] + 1 )->mAddress;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_DOUBLE( sArg1Val ) || MTX_IS_NULL_DOUBLE( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( *sArg1Val <= *sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualFloat( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdNumericType * sArg2Val  = (mtdNumericType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_FLOAT( sArg1Val ) || MTX_IS_NULL_FLOAT( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length > sArg2Val->length )
        {
            sOrder = idlOS::memcmp( &( sArg1Val->signExponent ),
                                    &( sArg2Val->signExponent ),
                                    sArg2Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            if ( sArg1Val->signExponent >= 0x80 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else if ( sArg1Val->length < sArg2Val->length )
        {
            sOrder = idlOS::memcmp( &( sArg1Val->signExponent ),
                                    &( sArg2Val->signExponent ),
                                    sArg1Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            if ( sArg1Val->signExponent >= 0x80 )
            {
                sOrder = -1;
            }
            else
            {
                sOrder = 1;
            }
        }
        else
        {
            sOrder = idlOS::memcmp( &( sArg1Val->signExponent ),
                                    &( sArg2Val->signExponent ),
                                    sArg2Val->length );
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualNumeric( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNumericType * sArg1Val  = (mtdNumericType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdNumericType * sArg2Val  = (mtdNumericType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NUMERIC( sArg1Val ) || MTX_IS_NULL_NUMERIC( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length > sArg2Val->length )
        {
            sOrder = idlOS::memcmp( &( sArg1Val->signExponent ),
                                    &( sArg2Val->signExponent ),
                                    sArg2Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            if ( sArg1Val->signExponent >= 0x80 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else if ( sArg1Val->length < sArg2Val->length )
        {
            sOrder = idlOS::memcmp( &( sArg1Val->signExponent ),
                                    &( sArg2Val->signExponent ),
                                    sArg1Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            if ( sArg1Val->signExponent >= 0x80 )
            {
                sOrder = -1;
            }
            else
            {
                sOrder = 1;
            }
        }
        else
        {
            sOrder = idlOS::memcmp( &( sArg1Val->signExponent ),
                                    &( sArg2Val->signExponent ),
                                    sArg2Val->length );
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualChar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType    * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdCharType    * sArg2Val  = (mtdCharType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;
    UChar          * sIterator = NULL;
    UChar          * sFence    = NULL;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_CHAR( sArg1Val ) || MTX_IS_NULL_CHAR( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length >= sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg1Val->value + sArg2Val->length,
                      sFence = sArg1Val->value + sArg1Val->length;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    sOrder = 1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg2Val->value + sArg1Val->length,
                      sFence = sArg2Val->value + sArg2Val->length;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    sOrder = -1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualVarchar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdCharType    * sArg1Val  = (mtdCharType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdCharType    * sArg2Val  = (mtdCharType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARCHAR( sArg1Val ) || MTX_IS_NULL_VARCHAR( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length > sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            if ( sOrder >= 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else if ( sArg1Val->length < sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length );

            if ( sOrder > 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );
        }

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualNchar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNcharType   * sArg1Val  = (mtdNcharType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdNcharType   * sArg2Val  = (mtdNcharType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;
    UChar          * sIterator = NULL;
    UChar          * sFence    = NULL;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NCHAR( sArg1Val ) || MTX_IS_NULL_NCHAR( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length >= sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg1Val->value + sArg2Val->length,
                      sFence = sArg1Val->value + sArg1Val->length;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    sOrder = 1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg2Val->value + sArg1Val->length,
                      sFence = sArg2Val->value + sArg2Val->length;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    sOrder = -1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualNchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualNcharUTF16( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNcharType   * sArg1Val  = (mtdNcharType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdNcharType   * sArg2Val  = (mtdNcharType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;
    UChar          * sIterator = NULL;
    UChar          * sFence    = NULL;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NCHAR( sArg1Val ) || MTX_IS_NULL_NCHAR( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length >= sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg1Val->value + sArg2Val->length,
                      sFence = sArg1Val->value + sArg1Val->length;
                  sIterator < sFence - 1;
                  sIterator += MTL_UTF16_PRECISION )
            {
                if ( ( *( sIterator + 0 ) <= mtlUTF16.specialCharSet[MTL_SP_IDX][0] ) &&
                     ( *( sIterator + 1 ) <= mtlUTF16.specialCharSet[MTL_SP_IDX][1] ) )
                {
                    /* Nothing to do */
                }
                else
                {
                    sOrder = 1;

                    break;
                }
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg2Val->value + sArg1Val->length,
                      sFence = sArg2Val->value + sArg2Val->length;
                  sIterator < sFence - 1;
                  sIterator += MTL_UTF16_PRECISION )
            {
                if ( ( *( sIterator + 0 ) <= mtlUTF16.specialCharSet[MTL_SP_IDX][0] ) &&
                     ( *( sIterator + 1 ) <= mtlUTF16.specialCharSet[MTL_SP_IDX][1] ) )
                {
                    /* Nothing to do */
                }
                else
                {
                    sOrder = -1;

                    break;
                }
            }
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualNcharUTF16",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualNvarchar( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNcharType   * sArg1Val  = (mtdNcharType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdNcharType   * sArg2Val  = (mtdNcharType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NVARCHAR( sArg1Val ) || MTX_IS_NULL_NVARCHAR( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length > sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            if ( sOrder >= 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else if ( sArg1Val->length < sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length );

            if ( sOrder > 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );
        }

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualNvarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualInterval( mtxEntry ** aEntry )
{
    mtdBooleanType  * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar             sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdIntervalType * sArg1Val  = (mtdIntervalType *)( aEntry[1] + 1 )->mAddress;
    UChar             sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdIntervalType * sArg2Val  = (mtdIntervalType *)( aEntry[2] + 1 )->mAddress;
    SInt              sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTD_INTERVAL_IS_NULL( sArg1Val ) || MTD_INTERVAL_IS_NULL( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->second > sArg2Val->second )
        {
            sOrder = 1;
        }
        else if ( sArg1Val->second < sArg2Val->second )
        {
            sOrder = -1;
        }
        else
        {
            sOrder = 0;
        }

        IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

        if ( sArg1Val->microsecond > sArg2Val->microsecond )
        {
            sOrder = 1;
        }
        else if ( sArg1Val->microsecond < sArg2Val->microsecond )
        {
            sOrder = -1;
        }
        else
        {
            sOrder = 0;
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualInterval",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualNibble( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdNibbleType  * sArg1Val  = (mtdNibbleType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdNibbleType  * sArg2Val  = (mtdNibbleType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_NIBBLE( sArg1Val ) || MTX_IS_NULL_NIBBLE( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length > sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length >> 1 );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            if ( sArg2Val->length & 1 )
            {
                if ( ( sArg1Val->value[ sArg2Val->length >> 1 ] & 0xF0 )
                     < ( sArg2Val->value[ sArg2Val->length >> 1 ] & 0xF0 ) )
                {
                    sOrder = -1;
                }
                else
                {
                    sOrder = 1;
                }
            }
            else
            {
                sOrder = 1;
            }
        }
        else if ( sArg1Val->length < sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length >> 1 );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            if ( sArg1Val->length & 1 )
            {
                if ( ( sArg1Val->value[ sArg1Val->length >> 1 ] & 0xF0 )
                     > ( sArg2Val->value[ sArg1Val->length >> 1 ] & 0xF0 ) )
                {
                    sOrder = 1;
                }
                else
                {
                    sOrder = -1;
                }
            }
            else
            {
                sOrder = -1;
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length >> 1 );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            if ( sArg1Val->length & 1 )
            {
                if ( ( sArg1Val->value[ sArg1Val->length >> 1 ] & 0xF0 )
                     > ( sArg2Val->value[ sArg1Val->length >> 1 ] & 0xF0 ) )
                {
                    sOrder = 1;
                }
                else
                {
                    sOrder = 0;
                }

                IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

                if ( ( sArg1Val->value[ sArg1Val->length >> 1 ] & 0xF0 )
                     < ( sArg2Val->value[ sArg1Val->length >> 1 ] & 0xF0 ) )
                {
                    sOrder = -1;
                }
                else
                {
                    sOrder = 0;
                }
            }
            else
            {
                sOrder = 0;
            }
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualNibble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualBit( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBitType     * sArg1Val  = (mtdBitType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdBitType     * sArg2Val  = (mtdBitType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;
    UChar          * sIterator = NULL;
    UChar          * sFence    = NULL;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BIT( sArg1Val ) || MTX_IS_NULL_BIT( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length >= sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    BIT_TO_BYTE( sArg2Val->length ) );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg1Val->value + BIT_TO_BYTE( sArg2Val->length ),
                      sFence = sArg1Val->value + BIT_TO_BYTE( sArg1Val->length );
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator != 0x00 )
                {
                    sOrder = 1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    BIT_TO_BYTE( sArg1Val->length ) );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg2Val->value + BIT_TO_BYTE( sArg1Val->length ),
                      sFence = sArg2Val->value + BIT_TO_BYTE( sArg2Val->length );
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator != 0x00 )
                {
                    sOrder = -1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualBit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualVarbit( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdBitType     * sArg1Val  = (mtdBitType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdBitType     * sArg2Val  = (mtdBitType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARBIT( sArg1Val ) || MTX_IS_NULL_VARBIT( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length > sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    BIT_TO_BYTE( sArg2Val->length ) );

            if ( sOrder >= 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else if ( sArg1Val->length < sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    BIT_TO_BYTE( sArg1Val->length ) );

            if ( sOrder > 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    BIT_TO_BYTE( sArg1Val->length ) );
        }

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualVarbit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualByte( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdByteType    * sArg1Val  = (mtdByteType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdByteType    * sArg2Val  = (mtdByteType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;
    UChar          * sIterator = NULL;
    UChar          * sFence    = NULL;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_BYTE( sArg1Val ) || MTX_IS_NULL_BYTE( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length >= sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg1Val->value + sArg2Val->length,
                      sFence = sArg1Val->value + sArg1Val->length;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator != 0x00 )
                {
                    sOrder = 1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length );

            IDE_TEST_CONT( sOrder != 0, NORMAL_EXIT );

            for ( sIterator = sArg2Val->value + sArg1Val->length,
                      sFence = sArg2Val->value + sArg2Val->length;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator != 0x00 )
                {
                    sOrder = -1;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        IDE_EXCEPTION_CONT( NORMAL_EXIT );

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualByte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessEqualVarbyte( mtxEntry ** aEntry )
{
    mtdBooleanType * sReturn   = (mtdBooleanType *)aEntry[0]->mAddress;
    UChar            sArg1Flag = aEntry[1]->mHeader.mFlag;
    mtdByteType    * sArg1Val  = (mtdByteType *)( aEntry[1] + 1 )->mAddress;
    UChar            sArg2Flag = aEntry[2]->mHeader.mFlag;
    mtdByteType    * sArg2Val  = (mtdByteType *)( aEntry[2] + 1 )->mAddress;
    SInt             sOrder;

    IDE_TEST_RAISE( MTX_IS_NOT_CALCULATED( sArg1Flag ) || MTX_IS_NOT_CALCULATED( sArg2Flag ),
                    INVALID_ENTRY_ORDER );

    if ( MTX_IS_NULL_VARBYTE( sArg1Val ) || MTX_IS_NULL_VARBYTE( sArg2Val ) )
    {
        MTX_SET_NULL_BOOLEAN( sReturn );
    }
    else
    {
        if ( sArg1Val->length > sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            if ( sOrder >= 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else if ( sArg1Val->length < sArg2Val->length )
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg2Val->length );

            if ( sOrder > 0 )
            {
                sOrder = 1;
            }
            else
            {
                sOrder = -1;
            }
        }
        else
        {
            sOrder = idlOS::memcmp( sArg1Val->value,
                                    sArg2Val->value,
                                    sArg1Val->length );
        }

        if ( sOrder <= 0 )
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxLessEqualVarbyte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
