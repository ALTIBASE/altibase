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
 * $Id: mtxEqual.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/
#include <mtx.h>

extern mtlModule mtlUTF16;

extern mtxModule mtxEqual;

static mtxSerialExecuteFunc mtxEqualGetSerialExecute( UInt aMtd1Type,
                                                      UInt /* aMtd2Type */ );

static IDE_RC mtxEqualBoolean( mtxEntry ** aEntry );
static IDE_RC mtxEqualDate( mtxEntry ** aEntry );
static IDE_RC mtxEqualSmallint( mtxEntry ** aEntry );
static IDE_RC mtxEqualInteger( mtxEntry ** aEntry );
static IDE_RC mtxEqualBigint( mtxEntry ** aEntry );
static IDE_RC mtxEqualReal( mtxEntry ** aEntry );
static IDE_RC mtxEqualDouble( mtxEntry ** aEntry );
static IDE_RC mtxEqualFloat( mtxEntry ** aEntry );
static IDE_RC mtxEqualNumeric( mtxEntry ** aEntry );
static IDE_RC mtxEqualChar( mtxEntry ** aEntry );
static IDE_RC mtxEqualVarchar( mtxEntry ** aEntry );
static IDE_RC mtxEqualNchar( mtxEntry ** aEntry );
static IDE_RC mtxEqualNcharUTF16( mtxEntry ** aEntry );
static IDE_RC mtxEqualNvarchar( mtxEntry ** aEntry );
static IDE_RC mtxEqualInterval( mtxEntry ** aEntry );
static IDE_RC mtxEqualNibble( mtxEntry ** aEntry );
static IDE_RC mtxEqualBit( mtxEntry ** aEntry );
static IDE_RC mtxEqualVarbit( mtxEntry ** aEntry );
static IDE_RC mtxEqualByte( mtxEntry ** aEntry );
static IDE_RC mtxEqualVarbyte( mtxEntry ** aEntry );

mtxModule mtxEqual = {
    MTX_EXECUTOR_ID_EQUAL,
    mtx::calculateNA,
    mtxEqualGetSerialExecute
};

static mtxSerialExecuteFunc mtxEqualGetSerialExecute( UInt aMtd1Type,
                                                      UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxEqualChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxEqualNumeric;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxEqualInteger;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxEqualSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxEqualFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxEqualReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxEqualDouble;

            break;

        case MTD_DATE_ID:
            sFunc = mtxEqualDate;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxEqualVarchar;

            break;

        case MTD_INTERVAL_ID:
            sFunc = mtxEqualInterval;

            break;

        case MTD_BOOLEAN_ID:
            sFunc = mtxEqualBoolean;

            break;

        case MTD_BYTE_ID:
            sFunc = mtxEqualByte;

            break;

        case MTD_NIBBLE_ID:
            sFunc = mtxEqualNibble;

            break;

        case MTD_VARBYTE_ID:
            sFunc = mtxEqualVarbyte;

            break;

        case MTD_VARBIT_ID:
            sFunc = mtxEqualVarbit;

            break;

        case MTD_NVARCHAR_ID:
            sFunc = mtxEqualNvarchar;

            break;

        case MTD_NCHAR_ID:
            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sFunc = mtxEqualNcharUTF16;
            }
            else
            {
                sFunc = mtxEqualNchar;
            }

            break;

        case MTD_BIT_ID:
            sFunc = mtxEqualBit;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxEqualBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxEqualBoolean( mtxEntry ** aEntry )
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
        if ( sArg1Val == sArg2Val )
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
                                  "mtxEqualBoolean",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualDate( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualDate",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualSmallint( mtxEntry ** aEntry )
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
        if ( sArg1Val == sArg2Val )
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
                                  "mtxEqualSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualInteger( mtxEntry ** aEntry )
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
        if ( sArg1Val == sArg2Val )
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
                                  "mtxEqualInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualBigint( mtxEntry ** aEntry )
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
        if ( sArg1Val == sArg2Val )
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
                                  "mtxEqualBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualReal( mtxEntry ** aEntry )
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
        if ( *sArg1Val < *sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
        else if ( *sArg1Val > *sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxEqualReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualDouble( mtxEntry ** aEntry )
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
        if ( *sArg1Val < *sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
        else if ( *sArg1Val > *sArg2Val )
        {
            *sReturn = MTD_BOOLEAN_FALSE;
        }
        else
        {
            *sReturn = MTD_BOOLEAN_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ORDER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtxEqualDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualFloat( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualNumeric( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualChar( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualVarchar( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualNchar( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualNchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualNcharUTF16( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualNcharUTF16",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualNvarchar( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualNvarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualInterval( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualInterval",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualNibble( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualNibble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualBit( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualBit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualVarbit( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualVarbit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualByte( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualByte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxEqualVarbyte( mtxEntry ** aEntry )
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

        if ( sOrder == 0 )
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
                                  "mtxEqualVarbyte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
