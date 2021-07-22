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
 * $Id: mtcdBit.cpp 17938 2006-09-05 00:54:03Z leekmo $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdBit;

// To Remove Warning
const mtdBitType mtcdBitNull = { 0, {'\0',} };

static ACI_RC mtdInitializeBit( acp_uint32_t aNo );

static ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                           acp_uint32_t* aArguments,
                           acp_sint32_t* aPrecision,
                           acp_sint32_t* aScale );

static ACI_RC mtdValue( mtcTemplate*  aTemplate,
                        mtcColumn*    aColumn,
                        void*         aValue,
                        acp_uint32_t* aValueOffset,
                        acp_uint32_t  aValueSize,
                        const void*   aToken,
                        acp_uint32_t  aTokenLength,
                        ACI_RC*       aResult );

static acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                                   const void*      aRow,
                                   acp_uint32_t     aFlag );

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale );

static void mtdNull( const mtcColumn* aColumn,
                     void*            aRow,
                     acp_uint32_t     aFlag );

static acp_uint32_t mtdHash( acp_uint32_t     aHash,
                             const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static acp_sint32_t mtdBitLogicalComp( mtdValueInfo* aValueInfo1,
                                       mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBitMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                            mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBitMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                             mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBitStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                               mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBitStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBitStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                  mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBitStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                                   mtdValueInfo* aValueInfo2 );

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonized,
                           mtcEncryptInfo*  aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 3, (void*)"BIT" }
};

static mtcColumn mtdColumn;

mtdModule mtcdBit = {
    mtdTypeName,
    &mtdColumn,
    MTD_BIT_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_BIT_ALIGN,
    MTD_GROUP_TEXT|
    MTD_CANON_NEED_WITH_ALLOCATION|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_DISABLE|
    MTD_CREATE_PARAM_PRECISION|
    MTD_SEARCHABLE_SEARCHABLE|    // BUG-17020
    MTD_LITERAL_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_TRUE,   // PROJ-1705
    128000,
    0,
    0,
    (void*)&mtcdBitNull,
    mtdInitializeBit,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdBitLogicalComp,     // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdBitMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdBitMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdBitStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdBitStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdBitStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdBitStoredStoredKeyDescComp // Descending Key Comparison
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtdSelectivityNA,
    mtdEncodeNA,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSize
};

ACI_RC mtdInitializeBit( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdBit, aNo ) != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdBit,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                    acp_uint32_t* aArguments,
                    acp_sint32_t* aPrecision,
                    acp_sint32_t* aScale)
{
    ACP_UNUSED(aScale);

    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_BIT_PRECISION_DEFAULT;
    }

    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    ACI_TEST_RAISE( *aPrecision < MTD_BIT_PRECISION_MINIMUM ||
                    *aPrecision > MTD_BIT_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = sizeof(acp_uint32_t) + ( BIT_TO_BYTE(*aPrecision) );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);

    ACI_EXCEPTION( ERR_INVALID_SCALE );
    aciSetErrorCode(mtERR_ABORT_INVALID_SCALE);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdValue( mtcTemplate*  aTemplate,
                 mtcColumn*    aColumn,
                 void*         aValue,
                 acp_uint32_t* aValueOffset,
                 acp_uint32_t  aValueSize,
                 const void*   aToken,
                 acp_uint32_t  aTokenLength,
                 ACI_RC*       aResult )
{
    acp_uint32_t sValueOffset;
    mtdBitType*  sValue;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_BIT_ALIGN );
    
    sValue = (mtdBitType*)( (acp_uint8_t*)aValue + sValueOffset );
    
    *aResult = ACI_SUCCESS;
    
    if( BIT_TO_BYTE(aTokenLength) <= (acp_uint8_t*)aValue - sValue->value + aValueSize )
    {
        if( aTokenLength == 0 )
        {
            sValue->length = 0;
        }
        else
        {
            ACI_TEST( mtcMakeBit( sValue,
                                  (const acp_uint8_t*)aToken,
                                  aTokenLength )
                      != ACI_SUCCESS );
        }

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;
        aColumn->precision       = sValue->length != 0 ? sValue->length : 1;
        aColumn->scale           = 0;
        
        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != ACI_SUCCESS );
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset +
            sizeof(acp_uint32_t) + BIT_TO_BYTE(sValue->length);
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                            const void*      aRow,
                            acp_uint32_t     aFlag )
{
    const mtdBitType* sValue;
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdBit.staticNull );

    if( mtcdBit.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        return sizeof(acp_uint32_t);
    }
    else
    {
        return sizeof(acp_uint32_t) + BIT_TO_BYTE(sValue->length);
    }
}

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale )
{
    const mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdBit.staticNull );

    *aPrecision = sValue->length;
    *aScale = 0;
    
    return ACI_SUCCESS;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           NULL );

    if( sValue != NULL )
    {
        sValue->length = 0;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdBit.staticNull );

    if( mtcdBit.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        return aHash;
    }
    
    return mtcHashSkip( aHash, sValue->value, BIT_TO_BYTE(sValue->length) );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdBit.staticNull );

    return ( sValue->length == 0 ) ? ACP_TRUE : ACP_FALSE;
}

acp_sint32_t
mtdBitLogicalComp( mtdValueInfo* aValueInfo1,
                   mtdValueInfo* aValueInfo2 )
{
    return mtdBitMtdMtdKeyAscComp( aValueInfo1,
                                   aValueInfo2 );
}

acp_sint32_t
mtdBitMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                        mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*  sBitValue1;
    const mtdBitType*  sBitValue2;
    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint32_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;
    
    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdBit.staticNull );
    sLength1 = sBitValue1->length;

    //---------
    // value1
    //---------
    sBitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdBit.staticNull );
    sLength2 = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   BIT_TO_BYTE(sLength2) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + BIT_TO_BYTE(sLength2),
                     sFence = sValue1 + BIT_TO_BYTE(sLength1);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return 1;
                }
            }
            return 0;
        }
        else
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   BIT_TO_BYTE(sLength1) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + BIT_TO_BYTE(sLength1),
                     sFence = sValue2 + BIT_TO_BYTE(sLength2);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return -1;
                }
            }
            return 0;
        }
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdBitMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                         mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*  sBitValue1;
    const mtdBitType*  sBitValue2;
    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint32_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;
    
    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdBit.staticNull );
    sLength1 = sBitValue1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdBit.staticNull );
    sLength2 = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   BIT_TO_BYTE(sLength1) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + BIT_TO_BYTE(sLength1),
                     sFence = sValue2 + BIT_TO_BYTE(sLength2);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return 1;
                }
            }
            return 0;
        }
        else
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   BIT_TO_BYTE(sLength2) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + BIT_TO_BYTE(sLength2),
                     sFence = sValue1 + BIT_TO_BYTE(sLength1);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return -1;
                }
            }
            return 0;
        }
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    
    return 0;
}

acp_sint32_t
mtdBitStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                           mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*  sBitValue2;
    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint32_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;
    
    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdBit.staticNull );
    sLength2 = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        // length가 0( == NULL )이 아닌 경우에 
        // aValueInfo1->value에는 varbit value가 저장되어 있음 
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 ) 
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();
        
        sValue2  = sBitValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   BIT_TO_BYTE(sLength2) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + BIT_TO_BYTE(sLength2),
                     sFence = sValue1 + BIT_TO_BYTE(sLength1);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return 1;
                }
            }
            return 0;
        }
        else
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   BIT_TO_BYTE(sLength1) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + BIT_TO_BYTE(sLength1),
                     sFence = sValue2 + BIT_TO_BYTE(sLength2);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return -1;
                }
            }
            return 0;
        }
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdBitStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                            mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*  sBitValue2;
    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint32_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;
    
    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdBit.staticNull );
    sLength2 = sBitValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        // length가 0( == NULL )이 아닌 경우에 
        // aValueInfo1->value에는 varbit value가 저장되어 있음 
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 ) 
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();
        
        sValue2  = sBitValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   BIT_TO_BYTE(sLength1) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + BIT_TO_BYTE(sLength1),
                     sFence = sValue2 + BIT_TO_BYTE(sLength2);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return 1;
                }
            }
        }
        else
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   BIT_TO_BYTE(sLength2) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + BIT_TO_BYTE(sLength2),
                     sFence = sValue1 + BIT_TO_BYTE(sLength1);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return -1;
                }
            }
            return 0;
        }
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    
    return 0;
}

acp_sint32_t
mtdBitStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint32_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;
    
    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------

    sLength2 = aValueInfo2->length;

    //---------
    // compare
    //---------    

    if( sLength1 != 0 && sLength2 != 0 )
    {
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();

        MTC_INT_BYTE_ASSIGN( &sLength2, aValueInfo2->value );
        sValue2  = ((acp_uint8_t*)aValueInfo2->value) + mtdHeaderSize();
        
        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   BIT_TO_BYTE(sLength2) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + BIT_TO_BYTE(sLength2),
                     sFence = sValue1 + BIT_TO_BYTE(sLength1);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return 1;
                }
            }
            return 0;
        }
        else
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   BIT_TO_BYTE(sLength1) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + BIT_TO_BYTE(sLength1),
                     sFence = sValue2 + BIT_TO_BYTE(sLength2);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return -1;
                }
            }
            return 0;
        }
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdBitStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                               mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint32_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;
    
    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------

    sLength2 = aValueInfo2->length;    

    //---------
    // compare
    //---------    

    if( sLength1 != 0 && sLength2 != 0 )
    {
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();

        MTC_INT_BYTE_ASSIGN( &sLength2, aValueInfo2->value );
        sValue2  = ((acp_uint8_t*)aValueInfo2->value) + mtdHeaderSize();
        
        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   BIT_TO_BYTE(sLength1) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + BIT_TO_BYTE(sLength1),
                     sFence = sValue2 + BIT_TO_BYTE(sLength2);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return 1;
                }
            }
            return 0;
        }
        else
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   BIT_TO_BYTE(sLength2) );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + BIT_TO_BYTE(sLength2),
                     sFence = sValue1 + BIT_TO_BYTE(sLength1);
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != 0x00 )
                {
                    return -1;
                }
            }
            return 0;
        }
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    
    return 0;

}

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonized,
                           mtcEncryptInfo*  aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo ,
                           mtcTemplate*     aTemplate  )
{
    mtdBitType* sCanonized;
    mtdBitType* sValue;

    ACP_UNUSED(aCanonInfo);
    ACP_UNUSED(aColumnInfo);
    ACP_UNUSED(aTemplate);
    
    if( aCanon->precision == aColumn->precision )
    {
        *aCanonized = aValue;
    }
    else
    {
        ACI_TEST_RAISE( ((mtdBitType*)aValue)->length > (acp_uint32_t)aCanon->precision,
                        ERR_INVALID_LENGTH );

        sCanonized = (mtdBitType*)*aCanonized;
        sValue     = (mtdBitType*)aValue;

        if( mtcdBit.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
        {
            sCanonized->length = 0;
        }
        else
        {
            acpMemSet( sCanonized->value,
                       0x00,
                       BIT_TO_BYTE( aCanon->precision ) );
            acpMemCpy( sCanonized->value,
                       sValue->value,
                       BIT_TO_BYTE( sValue->length ) );
            sCanonized->length = aCanon->precision;
        }
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH );
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;

    sLength = (acp_uint8_t*)&(((mtdBitType*)aValue)->length);
    sIntermediate = sLength[0];
    sLength[0]    = sLength[3];
    sLength[3]    = sIntermediate;
    sIntermediate = sLength[1];
    sLength[1]    = sLength[2];
    sLength[2]    = sIntermediate;
}

ACI_RC mtdValidate( mtcColumn*   aColumn,
                    void*        aValue,
                    acp_uint32_t aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
        
    mtdBitType * sVal = (mtdBitType *)aValue;
    
    ACI_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL);

    ACI_TEST_RAISE( aValueSize == 0, ERR_INVALID_LENGTH );
    ACI_TEST_RAISE( BIT_TO_BYTE(sVal->length) + sizeof(acp_uint32_t) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdBit,
                                   1,              // arguments
                                   sVal->length,   // precision
                                   0 )             // scale
              != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_NULL);
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE);
    }
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                void*        aDestValue,
                                acp_uint32_t aDestValueOffset,
                                acp_uint32_t aLength,
                                const void*  aValue )
{
/***********************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 **********************************************************************/

    mtdBitType* sBitValue;

    sBitValue = (mtdBitType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sBitValue->length = 0;
    }
    else
    {
        // bit type은 mtdDataType형태로 저장되므로
        // length 에 header size가 포함되어 있어 
        // 여기서 별도의 계산을 하지 않아도 된다. 
        ACI_TEST_RAISE( (aDestValueOffset + aLength) > aColumnSize, ERR_INVALID_STORED_VALUE );

        acpMemCpy( (acp_uint8_t*)sBitValue + aDestValueOffset, aValue, aLength );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


acp_uint32_t mtdNullValueSize()
{
/***********************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 * 예 ) mtdBitType( acp_uint32_t length; acp_uint8_t value[1] ) 에서
 *      length 타입인  acp_uint32_t의 크기를 반환
 **********************************************************************/

    return mtdActualSize( NULL,
                          & mtcdBitNull,
                          MTD_OFFSET_USELESS );   
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdBitType( acp_uint32_t length; acp_uint8_t value[1] ) 에서
 *      length 타입인  acp_uint32_t의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint32_t);
}
