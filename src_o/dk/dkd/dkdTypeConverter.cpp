/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $id$
 **********************************************************************/

#include <ide.h>
#include <idl.h>

#include <mtc.h>

#include <dkdTypeConverter.h>

struct dkdTypeConverter
{
    UInt mColumnCount;

    dkpColumn * mOriginalColumnArray;
    
    mtcColumn * mConvertedColumnArray;

    UInt mRecordLength;
    
};

/*
 *
 */ 
static IDE_RC allocTypeConveter( UInt aColumnCount,
                                 dkdTypeConverter ** aConverter )
{
    dkdTypeConverter * sConverter = NULL;
    SInt sStage = 0;

    IDU_FIT_POINT_RAISE( "allocTypeConveter::malloc::Converter",
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( *sConverter ),
                                       (void **)&sConverter,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sStage = 1;

    IDU_FIT_POINT_RAISE( "allocTypeConveter::malloc::OriginalColumnArray",
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc(
                        IDU_MEM_DK,
                        ID_SIZEOF( *(sConverter->mOriginalColumnArray) ) * aColumnCount,
                        (void **)&(sConverter->mOriginalColumnArray),
                        IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sStage = 2;

    IDU_FIT_POINT_RAISE( "allocTypeConveter::malloc::ConvertedColumnArray", 
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc(
                        IDU_MEM_DK,
                        ID_SIZEOF( *(sConverter->mConvertedColumnArray) ) * aColumnCount,
                        (void **)&(sConverter->mConvertedColumnArray),
                        IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );    
    sStage = 3;

    *aConverter = sConverter;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    IDE_PUSH();
    
    switch ( sStage )
    {
        case 3:
            (void)iduMemMgr::free( sConverter->mConvertedColumnArray );
        case 2:
            (void)iduMemMgr::free( sConverter->mOriginalColumnArray );
        case 1:
            (void)iduMemMgr::free( sConverter );
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;    
}

/*
 *
 */
static void freeTypeConverter( dkdTypeConverter * aConverter )
{
    if ( aConverter != NULL )
    {
        if ( aConverter->mConvertedColumnArray != NULL )
        {
            (void)iduMemMgr::free( aConverter->mConvertedColumnArray );
            aConverter->mConvertedColumnArray = NULL;
        }
        else
        {
            /* Nothing to do */
        }
        if ( aConverter->mOriginalColumnArray != NULL )
        {
            (void)iduMemMgr::free( aConverter->mOriginalColumnArray );
            aConverter->mOriginalColumnArray = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        (void)iduMemMgr::free( aConverter );
        aConverter = NULL;
    }
    else
    {
        /* Nothing to do */
    }
}

/*
 *
 */
static void copyColumnArray( dkpColumn * aDestination,
                             dkpColumn * aSource,
                             UInt aColumnCount )
{
    UInt i = 0;

    for ( i = 0; i < aColumnCount; i++ )
    {
        aDestination[i].mType = aSource[i].mType;
        aDestination[i].mScale = aSource[i].mScale;
        aDestination[i].mPrecision = aSource[i].mPrecision;
        aDestination[i].mNullable = aSource[i].mNullable;
        idlOS::memcpy( aDestination[i].mName,
                       aSource[i].mName,
                       ID_SIZEOF( aSource[i].mName ) );
    }
}

/*
 *
 */ 
static IDE_RC getConvertedColumnType( dkpColumn * aOriginalColumn,
                                      UInt * aConvertedTypeId,
                                      UInt * aArgumentCount,
                                      UInt * aPrecision )
{
    UInt sConvertedTypeId = MTD_NONE_ID;
    UInt sArgumentCount = 0;
    UInt sPrecision = 0;
    
    switch ( aOriginalColumn->mType )
    {
        case DKP_COL_TYPE_CHAR:
            sConvertedTypeId = MTD_CHAR_ID;
            sArgumentCount = 1;
            sPrecision = aOriginalColumn->mPrecision;
            break;
            
        case DKP_COL_TYPE_VARCHAR:
            sConvertedTypeId = MTD_VARCHAR_ID;
            sArgumentCount = 1;
            sPrecision = aOriginalColumn->mPrecision;
            break;
            
        case DKP_COL_TYPE_SMALLINT:
        case DKP_COL_TYPE_TINYINT:
            sConvertedTypeId = MTD_SMALLINT_ID;
            sArgumentCount = 0;
            break;

        case DKP_COL_TYPE_DECIMAL:
        case DKP_COL_TYPE_NUMERIC:
        case DKP_COL_TYPE_FLOAT:
            sConvertedTypeId = MTD_FLOAT_ID;
            /* BUG-41063 Don't care precision */
            sArgumentCount = 0;
            break;
            
        case DKP_COL_TYPE_INTEGER:
            sConvertedTypeId = MTD_INTEGER_ID;
            sArgumentCount = 0;
            break;
            
        case DKP_COL_TYPE_REAL:
            sConvertedTypeId = MTD_REAL_ID;
            sArgumentCount = 0;
            break;
            
        case DKP_COL_TYPE_DOUBLE:
            sConvertedTypeId = MTD_DOUBLE_ID;
            sArgumentCount = 0;
            break;
            
        case DKP_COL_TYPE_BIGINT:
            sConvertedTypeId = MTD_BIGINT_ID;
            sArgumentCount = 0;
            break;
            
        case DKP_COL_TYPE_DATE:
        case DKP_COL_TYPE_TIME:
        case DKP_COL_TYPE_TIMESTAMP:
            sConvertedTypeId = MTD_DATE_ID;
            sArgumentCount = 0;
            break;
            
        case DKP_COL_TYPE_BINARY:
        case DKP_COL_TYPE_BLOB:
        case DKP_COL_TYPE_CLOB:
	default:
            /* Unsupported type */
            IDE_RAISE( ERR_UNSUPPORTED_TYPE );
            break;
    }

    *aConvertedTypeId = sConvertedTypeId;
    *aArgumentCount = sArgumentCount;
    *aPrecision = sPrecision;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNSUPPORTED_TYPE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_UNSUPPORTED_COLUMN_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC convertColumnArray( dkpColumn * aOriginalColumnArray,
                                  UInt aColumnCount,
                                  mtcColumn * aConvertedColumnArray )
{
    UInt i = 0;
    UInt sConvertedTypeId = MTD_NONE_ID;
    UInt sArgumentCount = 0;
    UInt sPrecision = 0;
    UInt sOffset = 0;
    
    for ( i = 0; i < aColumnCount; i++ )
    {
        IDE_TEST( getConvertedColumnType( &(aOriginalColumnArray[i]),
                                          &sConvertedTypeId,
                                          &sArgumentCount,
                                          &sPrecision )
                  != IDE_SUCCESS );


        IDE_TEST( mtc::initializeColumn( &(aConvertedColumnArray[i]),
                                         sConvertedTypeId,
                                         sArgumentCount,
                                         sPrecision,
                                         aOriginalColumnArray[i].mScale )
                  != IDE_SUCCESS );

        aConvertedColumnArray[i].column.id = i;
        aConvertedColumnArray[i].column.flag =
            SMI_COLUMN_TYPE_FIXED |
            SMI_COLUMN_MODE_IN |
            SMI_COLUMN_STORAGE_DISK;
        aConvertedColumnArray[i].column.offset = sOffset;
        sOffset += idlOS::align8(
            aConvertedColumnArray[i].column.size );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static UInt calculateRecordLength( mtcColumn * aColumnArray,
                                   UInt aColumnCount )
{
    UInt i = 0;
    UInt sLength = 0;

    for ( i = 0; i < aColumnCount; i++ )
    {
        sLength += idlOS::align8( aColumnArray[i].column.size );
    }
    
    return sLength;
}
    
    
/*
 *
 */ 
IDE_RC dkdTypeConverterCreate( dkpColumn * aColumnArray,
                               UInt aColumnCount,
                               dkdTypeConverter ** aConverter )
{
    dkdTypeConverter * sConverter = NULL;
    SInt sStage = 0;

    IDE_TEST( allocTypeConveter( aColumnCount, &sConverter )
              != IDE_SUCCESS );
    sStage = 1;

    copyColumnArray( sConverter->mOriginalColumnArray,
                     aColumnArray, aColumnCount );

    sConverter->mColumnCount = aColumnCount;
    
    IDE_TEST( convertColumnArray( sConverter->mOriginalColumnArray,
                                  sConverter->mColumnCount,
                                  sConverter->mConvertedColumnArray )
              != IDE_SUCCESS );

    sConverter->mRecordLength = calculateRecordLength(
        sConverter->mConvertedColumnArray,
        sConverter->mColumnCount );
    
    *aConverter = sConverter;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sStage )
    {
        case 1:
            freeTypeConverter( sConverter );
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkdTypeConverterDestroy( dkdTypeConverter * aConverter )
{
    freeTypeConverter( aConverter );

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkdTypeConverterGetOriginalMeta( dkdTypeConverter * aConverter,
                                        dkpColumn ** aMeta )
{
    *aMeta = aConverter->mOriginalColumnArray;
    
    return IDE_SUCCESS;
}

/*
 *
 */ 
IDE_RC dkdTypeConverterGetConvertedMeta( dkdTypeConverter * aConverter,
                                         mtcColumn ** aMeta )
{
    *aMeta = aConverter->mConvertedColumnArray;
    
    return IDE_SUCCESS;
}

/*
 *
 */ 
IDE_RC dkdTypeConverterGetRecordLength( dkdTypeConverter * aConverter,
                                        UInt * aRecordLength )
{
    *aRecordLength = aConverter->mRecordLength;
    
    return IDE_SUCCESS;
}

/*
 *
 */ 
IDE_RC dkdTypeConverterGetColumnCount( dkdTypeConverter * aConverter,
                                       UInt * aColumnCount )
{
    *aColumnCount = aConverter->mColumnCount;
    
    return IDE_SUCCESS;
}

/*
 * 
 */
static void read2ByteNumber( SChar * aDest, SChar * aSrc )
{
    /*
     * value from ADLP is big endian number.
     */
#if defined( ENDIAN_IS_BIG_ENDIAN )
    aDest[0] = aSrc[0]; 
    aDest[1] = aSrc[1];
#else
    aDest[0] = aSrc[1]; 
    aDest[1] = aSrc[0];
#endif
}

/*
 *
 */
static void read4ByteNumber( SChar * aDest, SChar * aSrc )
{
    /*
     * value from ADLP is big endian number.
     */
#if defined( ENDIAN_IS_BIG_ENDIAN )
    aDest[0] = aSrc[0]; 
    aDest[1] = aSrc[1];
    aDest[2] = aSrc[2]; 
    aDest[3] = aSrc[3];
#else
    aDest[0] = aSrc[3]; 
    aDest[1] = aSrc[2];
    aDest[2] = aSrc[1]; 
    aDest[3] = aSrc[0];
#endif
}

/*
 *
 */
static void read8ByteNumber( SChar * aDest, SChar * aSrc )
{
    /*
     * value from ADLP is big endian number.
     */    
#if defined( ENDIAN_IS_BIG_ENDIAN )
    aDest[0] = aSrc[0];
    aDest[1] = aSrc[1];
    aDest[2] = aSrc[2]; 
    aDest[3] = aSrc[3];
    aDest[4] = aSrc[4];
    aDest[5] = aSrc[5];
    aDest[6] = aSrc[6];
    aDest[7] = aSrc[7];
#else
    aDest[0] = aSrc[7];
    aDest[1] = aSrc[6];
    aDest[2] = aSrc[5];
    aDest[3] = aSrc[4];
    aDest[4] = aSrc[3];
    aDest[5] = aSrc[2];
    aDest[6] = aSrc[1];
    aDest[7] = aSrc[0];
#endif
}

/*
 * ADLP defines NULL value's length is -1.
 */ 
#define NULL_VALUE_LENGTH       (-1)

/*
 *
 */ 
static IDE_RC convertNullValue( dkpColumn * aSrcColumn,
                                mtcColumn * aDestColumn,
                                SChar * aDestColumnValue )
{
    const mtdModule * sModule;
    
    /* 0 is no null */
    IDE_TEST_RAISE( aSrcColumn->mNullable == 0, ERROR_NO_NULL );

    sModule = aDestColumn->module;
    
    idlOS::memcpy( aDestColumnValue,
                   sModule->staticNull,
                   sModule->actualSize( NULL, sModule->staticNull ) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_NULL )
    {
        IDE_SET( ideSetErrorCode(
                     dkERR_ABORT_DKD_REMOTE_TABLE_COLUMN_IS_NO_NULL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
static IDE_RC convertNonNullValue( dkpColumn * aSrcColumn,
                                   SChar * aSrcColumnValue,
                                   SInt aLength,
                                   SChar * aDestColumnValue,
                                   SInt aColumnSize )
{
    mtdDateType * sDate = NULL;
    mtdNumericType * sNumeric = NULL;
    mtdCharType * sChar = NULL;
    
    /*
     * number value from ADLP is big endian.
     */

    switch ( aSrcColumn->mType )
    {
        case DKP_COL_TYPE_CHAR:
        case DKP_COL_TYPE_VARCHAR:
            IDE_TEST_RAISE( aColumnSize < aLength, ERR_COLUMN_SIZE_OVER );
            
            sChar = (mtdCharType *)aDestColumnValue;

            sChar->length = (UShort)aLength;
            idlOS::memcpy( sChar->value, aSrcColumnValue, aLength );
            break;
            
        case DKP_COL_TYPE_SMALLINT:
        case DKP_COL_TYPE_TINYINT:
            read2ByteNumber( aDestColumnValue, aSrcColumnValue );
            break;
            
        case DKP_COL_TYPE_DECIMAL:
        case DKP_COL_TYPE_NUMERIC:
        case DKP_COL_TYPE_FLOAT:
            sNumeric = (mtdNumericType *)aDestColumnValue;
            
            IDE_TEST( mtc::makeNumeric( sNumeric,
                                        (UInt)MTD_FLOAT_MANTISSA_MAXIMUM,
                                        (const UChar *)aSrcColumnValue,
                                        aLength )
                      != IDE_SUCCESS );
            break;
            
        case DKP_COL_TYPE_INTEGER:
        case DKP_COL_TYPE_REAL:
            read4ByteNumber( aDestColumnValue, aSrcColumnValue );
            break;
            
        case DKP_COL_TYPE_DOUBLE:
        case DKP_COL_TYPE_BIGINT:
            read8ByteNumber( aDestColumnValue, aSrcColumnValue );
            break;
            
        case DKP_COL_TYPE_DATE:
        case DKP_COL_TYPE_TIME:
        case DKP_COL_TYPE_TIMESTAMP:
            sDate = (mtdDateType *)aDestColumnValue;
            
            read2ByteNumber( (SChar *)&(sDate->year), aSrcColumnValue );
            aSrcColumnValue += ID_SIZEOF( UShort );
            
            read2ByteNumber( (SChar *)&(sDate->mon_day_hour),
                             aSrcColumnValue );
            aSrcColumnValue += ID_SIZEOF( UShort );
            
            read4ByteNumber( (SChar *)&(sDate->min_sec_mic), aSrcColumnValue );
            break;
            
        case DKP_COL_TYPE_BINARY:
        case DKP_COL_TYPE_BLOB:
        case DKP_COL_TYPE_CLOB:
            IDE_RAISE( ERR_UNSUPPORTED_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_SIZE_OVER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "convertNonNullValue : "
                                  "Target data size is biger than column buffer size"  ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_TYPE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_UNSUPPORTED_COLUMN_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 * 
 */
static IDE_RC convertValue( dkpColumn * aSrcColumn,
                            SChar * aSrcColumnValue,
                            SInt aLength,
                            mtcColumn * aDestColumn,
                            SChar * aDestColumnValue )
{
    if ( aLength == NULL_VALUE_LENGTH )
    {

        IDE_TEST( convertNullValue( aSrcColumn,
                                    aDestColumn,
                                    aDestColumnValue )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( convertNonNullValue( aSrcColumn,
                                       aSrcColumnValue,
                                       aLength,
                                       aDestColumnValue,
                                       ( (SInt)aDestColumn->column.size ) )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkdTypeConverterConvertRecord( dkdTypeConverter * aConverter,
                                      void * aSrcRecord,
                                      void * aDestRecord )
{
    UInt i = 0;
    SChar * sSrcRecordPoint = (SChar *)aSrcRecord;
    SChar * sDestRecordPoint = NULL;
    SInt sColumnValueLength = 0;
    
    for ( i = 0; i < aConverter->mColumnCount; i++ )
    {
        /* BUG-36519 */
        idlOS::memcpy( (SChar *)&sColumnValueLength, 
                       sSrcRecordPoint, 
                       ID_SIZEOF( UInt ) );

        sSrcRecordPoint += ID_SIZEOF( UInt );        

        sDestRecordPoint = (SChar *)aDestRecord +
            aConverter->mConvertedColumnArray[i].column.offset;
        
        IDE_TEST( convertValue( &(aConverter->mOriginalColumnArray[i]),
                                sSrcRecordPoint,
                                sColumnValueLength,
                                &(aConverter->mConvertedColumnArray[i]),
                                sDestRecordPoint )
                  != IDE_SUCCESS );

        if ( sColumnValueLength != NULL_VALUE_LENGTH )
        {
            sSrcRecordPoint += sColumnValueLength;
        }
        else
        {
            /* nothing to do */
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkdTypeConverterMakeNullRow( dkdTypeConverter * aConverter,
                                    void             * aDestRecord )
{
    UInt        i = 0;
    SChar     * sDestRecordPoint = NULL;
    mtdModule * sModule = NULL;

    for ( i = 0; i < aConverter->mColumnCount; i++ )
    {
        sDestRecordPoint = (SChar *)aDestRecord +
            aConverter->mConvertedColumnArray[i].column.offset;

        sModule = (mtdModule *)aConverter->mConvertedColumnArray[i].module;

        idlOS::memcpy( sDestRecordPoint,
                       sModule->staticNull,
                       sModule->actualSize( NULL, sModule->staticNull ) );
    }

    return IDE_SUCCESS;
}
