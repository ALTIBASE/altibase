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
 * $Id: mtcdByte.cpp 36231 2009-10-22 04:07:06Z kumdory $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdByte;

// To Remove Warning
const mtdByteType mtcdByteNull = { 0, {'\0',} };

static ACI_RC mtdInitializeByte( acp_uint32_t aNo );

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

static acp_sint32_t mtdByteLogicalComp( mtdValueInfo* aValueInfo1,
                                        mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdByteMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                             mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdByteMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                              mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdByteStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdByteStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                 mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdByteStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                   mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdByteStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
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
    { NULL, 4, (void*)"BYTE" }
};

static mtcColumn mtdColumn;

mtdModule mtcdByte = {
    mtdTypeName,
    &mtdColumn,
    MTD_BYTE_ID,
    0,
    { /*SMI_BUILTIN_B_TREE_INDEXTYPE_ID*/0,
      /*SMI_BUILTIN_B_TREE2_INDEXTYPE_ID*/0,
      0, 0, 0, 0, 0 },
    MTD_BYTE_ALIGN,
    MTD_GROUP_MISC|
    MTD_CANON_NEED_WITH_ALLOCATION|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_DISABLE|
    MTD_CREATE_PARAM_PRECISION|
    MTD_SEARCHABLE_SEARCHABLE|    // BUG-17020
    MTD_LITERAL_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    32000,
    0,
    0,
    (void*)&mtcdByteNull,
    mtdInitializeByte,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdByteLogicalComp,     // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdByteMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdByteMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdByteStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdByteStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdByteStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdByteStoredStoredKeyDescComp // Descending Key Comparison
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

ACI_RC mtdInitializeByte( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdByte, aNo ) != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdByte,
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
                    acp_sint32_t* aScale )
{
    ACP_UNUSED( aScale);
    
    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_BYTE_PRECISION_DEFAULT;
    }

    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    ACI_TEST_RAISE( *aPrecision < MTD_BYTE_PRECISION_MINIMUM ||
                    *aPrecision > MTD_BYTE_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = sizeof(acp_uint16_t) + *aPrecision;
    
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
    mtdByteType* sValue;

    ACP_UNUSED( aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_BYTE_ALIGN );

    sValue = (mtdByteType*)( (acp_uint8_t*)aValue + sValueOffset );

    *aResult = ACI_SUCCESS;
    
    if( ( (aTokenLength + 1) >> 1 ) <=
        (acp_uint8_t*)aValue - sValue->value + aValueSize )
    {
        ACI_TEST( mtcMakeByte( sValue,
                               ( acp_uint8_t*)aToken,
                               aTokenLength )
                  != ACI_SUCCESS );

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;
        aColumn->precision       = sValue->length;
        aColumn->scale           = 0;

        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != ACI_SUCCESS );

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + 
            sizeof(acp_uint16_t) + sValue->length; 
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
    const mtdByteType* sValue;

    ACP_UNUSED( aColumn);

    sValue = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdByte.staticNull );

    return sizeof(acp_uint16_t) + sValue->length;
}

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale )
{
    const mtdByteType* sValue;

    ACP_UNUSED( aColumn);

    sValue = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdByte.staticNull );

    *aPrecision = sValue->length;
    *aScale = 0;
    
    return ACI_SUCCESS;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdByteType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdByteType*)
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
    const mtdByteType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdByte.staticNull );

    return mtcHashSkip( aHash, sValue->value, sValue->length );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdByteType* sValue;

    ACP_UNUSED( aColumn);

    sValue = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdByte.staticNull );

    return (sValue->length == 0) ? ACP_TRUE : ACP_FALSE;
}

acp_sint32_t
mtdByteLogicalComp( mtdValueInfo* aValueInfo1,
                    mtdValueInfo* aValueInfo2 )
{
    return mtdByteMtdMtdKeyAscComp( aValueInfo1,
                                    aValueInfo2 );
}

acp_sint32_t
mtdByteMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                         mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType* sByteValue1;
    const mtdByteType* sByteValue2;
    acp_uint16_t       sLength1;
    acp_uint16_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint16_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdByte.staticNull );
    sLength1 = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdByte.staticNull );
    sLength2 = sByteValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;

        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + sLength2,
                     sFence = sValue1 + sLength1;
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
                                   sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
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
mtdByteMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                          mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType* sByteValue1;
    const mtdByteType* sByteValue2;
    acp_uint8_t        sLength1;
    acp_uint8_t        sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint16_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdByte.staticNull );
    sLength1 = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdByte.staticNull );
    sLength2 = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;

        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
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
                                   sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + sLength2,
                     sFence = sValue1 + sLength1;
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
mtdByteStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                            mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType* sByteValue2;
    acp_uint16_t       sLength1;
    acp_uint16_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;
    
    acp_sint16_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint8_t)aValueInfo1->length;

    //---------
    // value1
    //---------
    sByteValue2 = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdByte.staticNull );
    sLength2 = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (acp_uint8_t*)aValueInfo1->value;
        sValue2  = sByteValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + sLength2,
                     sFence = sValue1 + sLength1;
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
                                   sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
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
mtdByteStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                             mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType* sByteValue2;
    acp_uint16_t       sLength1;
    acp_uint16_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint16_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;

    //---------
    // value1
    //---------

    sLength1 = (acp_uint16_t)aValueInfo1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdByte.staticNull );
    sLength2 = sByteValue2->length;

    //---------
    // compare
    //---------    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = aValueInfo1->value;
        sValue2  = sByteValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
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
                                   sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + sLength2,
                     sFence = sValue1 + sLength1;
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
mtdByteStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                               mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t       sLength1;
    acp_uint16_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint16_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint16_t)aValueInfo1->length;
    
    //---------
    // value2
    //---------
    sLength2 = (acp_uint16_t)aValueInfo2->length;

    //---------
    // compare
    //---------    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = aValueInfo1->value;
        sValue2  = aValueInfo2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2,
                                   sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + sLength2,
                     sFence = sValue1 + sLength1;
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
                                   sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
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
mtdByteStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description :  Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t       sLength1;
    acp_uint16_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    acp_sint16_t       sCompared;
    const acp_uint8_t* sIterator;
    const acp_uint8_t* sFence;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint16_t)aValueInfo1->length;

    //---------
    // value2
    //---------
    sLength2 = (acp_uint16_t)aValueInfo2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = aValueInfo1->value;
        sValue2  = aValueInfo2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1,
                                   sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
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
                                   sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            for( sIterator = sValue1 + sLength2,
                     sFence = sValue1 + sLength1;
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
                           mtcEncryptInfo * aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate )
{
    mtdByteType* sCanonized;
    mtdByteType* sValue;

    ACP_UNUSED(aCanonInfo);
    ACP_UNUSED(aColumnInfo);
    ACP_UNUSED(aTemplate);
    
    if( aCanon->precision == aColumn->precision )
    {
        *aCanonized = aValue;
    }
    else
    {
        ACI_TEST_RAISE( ((mtdByteType*)aValue)->length > (acp_uint32_t)aCanon->precision,
                        ERR_INVALID_LENGTH );

        sCanonized = (mtdByteType*)*aCanonized;
        sValue     = (mtdByteType*)aValue;

        if( mtcdByte.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
        {
            sCanonized->length = 0;
        }
        else
        {
            acpMemCpy( sCanonized->value,
                       sValue->value,
                       sValue->length );
            acpMemSet( sCanonized->value + sValue->length,
                       0x00,
                       aCanon->precision - sValue->length );
            sCanonized->length = aCanon->precision; 
        }
    }
 
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);

    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;

    sLength = (acp_uint8_t*)&(((mtdByteType*)aValue)->length);
    sIntermediate = sLength[0];
    sLength[0]    = sLength[1];
    sLength[1]    = sIntermediate;
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
        
    mtdByteType * sVal = (mtdByteType*)aValue;

    ACI_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );

    ACI_TEST_RAISE((aValueSize < sizeof(acp_uint16_t)) ||
                   (sVal->length + sizeof(acp_uint16_t) != aValueSize),
                   ERR_INVALID_LENGTH );

    ACI_TEST_RAISE( sVal->length > aColumn->column.size, ERR_INVALID_VALUE );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdByte,
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

    ACI_EXCEPTION( ERR_INVALID_VALUE );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void            * aDestValue,
                                       acp_uint32_t      aDestValueOffset,
                                       acp_uint32_t      aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdByteType* sByteValue;

    sByteValue = (mtdByteType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sByteValue->length = 0;
    }
    else
    {
        ACI_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sByteValue->length = (acp_uint16_t)(aDestValueOffset + aLength);
        acpMemCpy( (acp_uint8_t*)sByteValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
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
/*******************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환    
 * 예 ) mtdByteType( acp_uint16_t length; acp_uint16_t value[1] ) 에서
 *      length타입의 acp_uint16_t의 크기를 반환
 *******************************************************************/

    return mtdActualSize( NULL,
                          & mtcdByteNull,
                          MTD_OFFSET_USELESS );   
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdByteType( acp_uint16_t length; acp_uint16_t value[1] ) 에서
 *      length타입의 acp_uint16_t의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint16_t);
}

