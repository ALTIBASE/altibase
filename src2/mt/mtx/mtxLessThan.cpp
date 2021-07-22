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
 * $Id: mtxLessThan.cpp 84859 2019-02-01 06:07:08Z andrew.shin $
 **********************************************************************/
#include <mtx.h>

extern mtlModule mtlUTF16;

extern mtxModule mtxLessThan;

static mtxSerialExecuteFunc mtxLessThanGetSerialExecute( UInt aMtd1Type,
                                                      UInt /* aMtd2Type */ );

static IDE_RC mtxLessThanBoolean( mtxEntry ** aEntry );
static IDE_RC mtxLessThanDate( mtxEntry ** aEntry );
static IDE_RC mtxLessThanSmallint( mtxEntry ** aEntry );
static IDE_RC mtxLessThanInteger( mtxEntry ** aEntry );
static IDE_RC mtxLessThanBigint( mtxEntry ** aEntry );
static IDE_RC mtxLessThanReal( mtxEntry ** aEntry );
static IDE_RC mtxLessThanDouble( mtxEntry ** aEntry );
static IDE_RC mtxLessThanFloat( mtxEntry ** aEntry );
static IDE_RC mtxLessThanNumeric( mtxEntry ** aEntry );
static IDE_RC mtxLessThanChar( mtxEntry ** aEntry );
static IDE_RC mtxLessThanVarchar( mtxEntry ** aEntry );
static IDE_RC mtxLessThanNchar( mtxEntry ** aEntry );
static IDE_RC mtxLessThanNcharUTF16( mtxEntry ** aEntry );
static IDE_RC mtxLessThanNvarchar( mtxEntry ** aEntry );
static IDE_RC mtxLessThanInterval( mtxEntry ** aEntry );
static IDE_RC mtxLessThanNibble( mtxEntry ** aEntry );
static IDE_RC mtxLessThanBit( mtxEntry ** aEntry );
static IDE_RC mtxLessThanVarbit( mtxEntry ** aEntry );
static IDE_RC mtxLessThanByte( mtxEntry ** aEntry );
static IDE_RC mtxLessThanVarbyte( mtxEntry ** aEntry );

mtxModule mtxLessThan = {
    MTX_EXECUTOR_ID_LESS_THAN,
    mtx::calculateNA,
    mtxLessThanGetSerialExecute
};

static mtxSerialExecuteFunc mtxLessThanGetSerialExecute( UInt aMtd1Type,
                                                      UInt /* aMtd2Type */ )
{
    mtxSerialExecuteFunc sFunc = mtx::calculateNA;

    switch ( aMtd1Type )
    {
        case MTD_CHAR_ID:
            sFunc = mtxLessThanChar;

            break;

        case MTD_NUMERIC_ID:
            sFunc = mtxLessThanNumeric;

            break;

        case MTD_INTEGER_ID:
            sFunc = mtxLessThanInteger;

            break;

        case MTD_SMALLINT_ID:
            sFunc = mtxLessThanSmallint;

            break;

        case MTD_FLOAT_ID:
            sFunc = mtxLessThanFloat;

            break;

        case MTD_REAL_ID:
            sFunc = mtxLessThanReal;

            break;

        case MTD_DOUBLE_ID:
            sFunc = mtxLessThanDouble;

            break;

        case MTD_DATE_ID:
            sFunc = mtxLessThanDate;

            break;

        case MTD_VARCHAR_ID:
            sFunc = mtxLessThanVarchar;

            break;

        case MTD_INTERVAL_ID:
            sFunc = mtxLessThanInterval;

            break;

        case MTD_BOOLEAN_ID:
            sFunc = mtxLessThanBoolean;

            break;

        case MTD_BYTE_ID:
            sFunc = mtxLessThanByte;

            break;

        case MTD_NIBBLE_ID:
            sFunc = mtxLessThanNibble;

            break;

        case MTD_VARBYTE_ID:
            sFunc = mtxLessThanVarbyte;

            break;

        case MTD_VARBIT_ID:
            sFunc = mtxLessThanVarbit;

            break;

        case MTD_NVARCHAR_ID:
            sFunc = mtxLessThanNvarchar;

            break;

        case MTD_NCHAR_ID:
            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sFunc = mtxLessThanNcharUTF16;
            }
            else
            {
                sFunc = mtxLessThanNchar;
            }

            break;

        case MTD_BIT_ID:
            sFunc = mtxLessThanBit;

            break;

        case MTD_BIGINT_ID:
            sFunc = mtxLessThanBigint;

            break;

        default:
            sFunc = mtx::calculateNA;

            break;
    }

    return sFunc;
}

static IDE_RC mtxLessThanBoolean( mtxEntry ** aEntry )
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
        if ( sArg1Val < sArg2Val )
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
                                  "mtxLessThanBoolean",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanDate( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanDate",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanSmallint( mtxEntry ** aEntry )
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
        if ( sArg1Val < sArg2Val )
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
                                  "mtxLessThanSmallint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanInteger( mtxEntry ** aEntry )
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
        if ( sArg1Val < sArg2Val )
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
                                  "mtxLessThanInteger",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanBigint( mtxEntry ** aEntry )
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
        if ( sArg1Val < sArg2Val )
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
                                  "mtxLessThanBigint",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanReal( mtxEntry ** aEntry )
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
                                  "mtxLessThanReal",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanDouble( mtxEntry ** aEntry )
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
                                  "mtxLessThanDouble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanFloat( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanFloat",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanNumeric( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanNumeric",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanChar( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanChar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanVarchar( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanVarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanNchar( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanNchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanNcharUTF16( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanNcharUTF16",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanNvarchar( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanNvarchar",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanInterval( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanInterval",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanNibble( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanNibble",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanBit( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanBit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanVarbit( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanVarbit",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanByte( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanByte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtxLessThanVarbyte( mtxEntry ** aEntry )
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

        if ( sOrder < 0 )
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
                                  "mtxLessThanVarbyte",
                                  "invalid entry order" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
