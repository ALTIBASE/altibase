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
 * $Id: mtcdNibble.cpp 36850 2009-11-19 04:57:38Z raysiasd $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdNibble;

#define MTD_NIBBLE_ALIGN             (sizeof(acp_uint8_t))

// To Remove Warning
const mtdNibbleType mtcdNibbleNull = { MTD_NIBBLE_NULL_LENGTH, {'\0',} };

static ACI_RC mtdInitializeNibble( acp_uint32_t aNo );

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

static acp_sint32_t mtdNibbleLogicalComp( mtdValueInfo* aValueInfo1,
                                          mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNibbleMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                               mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNibbleMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNibbleStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                  mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNibbleStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                   mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNibbleStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                     mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNibbleStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                                      mtdValueInfo* aValueInfo2 );

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonizedValue,
                           mtcEncryptInfo*  aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void*             aDestValue,
                                       acp_uint32_t      aDestValueOffset,
                                       acp_uint32_t      aLength,
                                       const void*       aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"NIBBLE" }
};

static mtcColumn mtdColumn;

mtdModule mtcdNibble = {
    mtdTypeName,
    &mtdColumn,
    MTD_NIBBLE_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_NIBBLE_ALIGN,
    MTD_GROUP_MISC|
    MTD_CANON_NEED|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_VARIABLE|
    MTD_SELECTIVITY_DISABLE|
    MTD_CREATE_PARAM_PRECISION|
    MTD_SEARCHABLE_SEARCHABLE|    // BUG-17020
    MTD_LITERAL_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_TRUE,   // PROJ-1705
    MTD_NIBBLE_PRECISION_MAXIMUM,
    0,
    0,
    (void*)&mtcdNibbleNull,
    mtdInitializeNibble,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdNibbleLogicalComp,     // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdNibbleMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdNibbleMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdNibbleStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdNibbleStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdNibbleStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdNibbleStoredStoredKeyDescComp // Descending Key Comparison
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

ACI_RC mtdInitializeNibble( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdNibble, aNo ) != ACI_SUCCESS );
    
    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdNibble,
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
    ACP_UNUSED(aScale);

    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_NIBBLE_PRECISION_DEFAULT;
    }

    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    ACI_TEST_RAISE( (*aPrecision < MTD_NIBBLE_PRECISION_MINIMUM) ||
                    (*aPrecision > MTD_NIBBLE_PRECISION_MAXIMUM),
                    ERR_INVALID_LENGTH );

    *aColumnSize = sizeof(acp_uint8_t) + ( (*aPrecision + 1) >> 1);
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);

    ACI_EXCEPTION( ERR_INVALID_SCALE );
    aciSetErrorCode(mtERR_ABORT_INVALID_SCALE);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdValue( mtcTemplate*  aTemplate ,
                 mtcColumn*    aColumn,
                 void*         aValue,
                 acp_uint32_t* aValueOffset,
                 acp_uint32_t  aValueSize,
                 const void*   aToken,
                 acp_uint32_t  aTokenLength,
                 ACI_RC*       aResult )
{
    acp_uint32_t   sValueOffset;
    mtdNibbleType* sValue;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_NIBBLE_ALIGN );
    
    sValue = (mtdNibbleType*)( (acp_uint8_t*)aValue + sValueOffset );
    
    *aResult = ACI_SUCCESS;
    
    if( (aTokenLength + 1) >> 1 <= (acp_uint8_t*)aValue - sValue->value + aValueSize )
    {
        if( aTokenLength == 0 )
        {
            sValue->length     = MTD_NIBBLE_NULL_LENGTH;

            // precision, scale 재 설정 후, estimate로 semantic 검사
            aColumn->flag      = 1;
            aColumn->precision = MTD_NIBBLE_PRECISION_DEFAULT;
            aColumn->scale     = 0;
        }
        else
        {
            ACI_TEST( mtcMakeNibble( sValue,
                                     (const acp_uint8_t*)aToken,
                                     aTokenLength )
                      != ACI_SUCCESS );

            // precision, scale 재 설정 후, estimate로 semantic 검사
            aColumn->flag      = 1;
            aColumn->precision = sValue->length != 0
                ? sValue->length
                : MTD_NIBBLE_PRECISION_DEFAULT;
            aColumn->scale     = 0;
        }
       
        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != ACI_SUCCESS );

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset +
            sizeof(acp_uint8_t) + ( (sValue->length+1) >> 1 );
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
    const mtdNibbleType* sValue;
    
    sValue = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNibble.staticNull );

    if( mtcdNibble.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        return sizeof(acp_uint8_t);
    }
    else
    {
        return sizeof(acp_uint8_t) + ( (sValue->length + 1) >> 1 );
    }
}

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale )
{
    const mtdNibbleType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNibble.staticNull );
    
    *aPrecision = sValue->length;
    *aScale = 0;
    
    return ACI_SUCCESS;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdNibbleType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdNibbleType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           NULL );

    if( sValue != NULL )
    {
        sValue->length = MTD_NIBBLE_NULL_LENGTH;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdNibbleType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNibble.staticNull );

    if( mtcdNibble.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        return aHash;
    }
    
    return mtcHashSkip( aHash, sValue->value, (sValue->length + 1) >> 1 );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdNibbleType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNibble.staticNull );

    return ( sValue->length == MTD_NIBBLE_NULL_LENGTH ) ? ACP_TRUE : ACP_FALSE;
}

acp_sint32_t
mtdNibbleLogicalComp( mtdValueInfo* aValueInfo1,
                      mtdValueInfo* aValueInfo2 )
{
    return mtdNibbleMtdMtdKeyAscComp( aValueInfo1,
                                      aValueInfo2 );
}

acp_sint32_t
mtdNibbleMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                           mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType* sNibbleValue1;
    const mtdNibbleType* sNibbleValue2;
    acp_uint8_t          sLength1;
    acp_uint8_t          sLength2;
    const acp_uint8_t*   sValue1;
    const acp_uint8_t*   sValue2;
    acp_sint32_t         sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNibble.staticNull );
    sLength1 = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNibble.staticNull );

    sLength2 = sNibbleValue2->length;

    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sValue1  = sNibbleValue1->value;
        sValue2  = sNibbleValue2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                      sLength2 >> 1  ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength2 & 1 )
            {
                if( ( sValue1[sLength2>>1] & 0xF0 ) <
                    ( sValue2[sLength2>>1] & 0xF0 ) )
                {
                    return -1;
                }
            }
            return 1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                      sLength1 >> 1 ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength1 & 1 )
            {
                if( ( sValue1[sLength1>>1] & 0xF0 ) >
                    ( sValue2[sLength1>>1] & 0xF0 ) )
                {
                    return 1;
                }
            }
            return -1;
        }
        if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                  sLength1 >> 1 ) ) != 0 )
        {
            return sOrder;
        }
        if( sLength1 & 1 )
        {
            if( ( sValue1[sLength1>>1] & 0xF0 ) >
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return 1;
            }
            if( ( sValue1[sLength1>>1] & 0xF0 ) <
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 > sLength2 )
    {
        return 1;
    }
    if( sLength1 < sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNibbleMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                            mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType* sNibbleValue1;
    const mtdNibbleType* sNibbleValue2;
    acp_uint8_t          sLength1;
    acp_uint8_t          sLength2;
    const acp_uint8_t*   sValue1;
    const acp_uint8_t*   sValue2;
    acp_sint32_t         sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNibble.staticNull );

    sLength1 = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNibble.staticNull );

    sLength2 = sNibbleValue2->length;

    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sValue1  = sNibbleValue1->value;
        sValue2  = sNibbleValue2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                      sLength2 >> 1  ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength2 & 1 )
            {
                if( ( sValue1[sLength2>>1] & 0xF0 ) <
                    ( sValue2[sLength2>>1] & 0xF0 ) )
                {
                    return 1;
                }
            }
            return -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                      sLength1 >> 1 ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength1 & 1 )
            {
                if( ( sValue1[sLength1>>1] & 0xF0 ) >
                    ( sValue2[sLength1>>1] & 0xF0 ) )
                {
                    return -1;
                }
            }
            return 1;
        }
        if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                  sLength1 >> 1 ) ) != 0 )
        {
            return sOrder;
        }
        if( sLength1 & 1 )
        {
            if( ( sValue1[sLength1>>1] & 0xF0 ) >
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return -1;
            }
            if( ( sValue1[sLength1>>1] & 0xF0 ) <
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return 1;
            }
        }
        return 0;
    }
    
    if( sLength1 > sLength2 )
    {
        return 1;
    }
    if( sLength1 < sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNibbleStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType* sNibbleValue1;
    const mtdNibbleType* sNibbleValue2;
    acp_uint8_t          sLength1;
    acp_uint8_t          sLength2;
    const acp_uint8_t*   sValue1;
    const acp_uint8_t*   sValue2;
    acp_sint32_t         sOrder;

    //---------
    // value1
    //---------

    sLength1 = ( aValueInfo1->length == 0 )
        ? MTD_NIBBLE_NULL_LENGTH
        : aValueInfo1->length;

    //---------
    // value2
    //---------
    
    sNibbleValue2 = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNibble.staticNull );

    sLength2 = sNibbleValue2->length;

    //---------
    // compare
    //---------

    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sNibbleValue1 = (const mtdNibbleType*)aValueInfo1->value;
        sLength1 = sNibbleValue1->length;
        sValue1  = sNibbleValue1->value;
        
        sValue2  = sNibbleValue2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                      sLength2 >> 1  ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength2 & 1 )
            {
                if( ( sValue1[sLength2>>1] & 0xF0 ) <
                    ( sValue2[sLength2>>1] & 0xF0 ) )
                {
                    return -1;
                }
            }
            return 1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                      sLength1 >> 1 ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength1 & 1 )
            {
                if( ( sValue1[sLength1>>1] & 0xF0 ) >
                    ( sValue2[sLength1>>1] & 0xF0 ) )
                {
                    return 1;
                }
            }
            return -1;
        }
        if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                  sLength1 >> 1 ) ) != 0 )
        {
            return sOrder;
        }
        if( sLength1 & 1 )
        {
            if( ( sValue1[sLength1>>1] & 0xF0 ) >
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return 1;
            }
            if( ( sValue1[sLength1>>1] & 0xF0 ) <
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 > sLength2 )
    {
        return 1;
    }
    if( sLength1 < sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNibbleStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                               mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/
  
    const mtdNibbleType* sNibbleValue1;
    const mtdNibbleType* sNibbleValue2;
    acp_uint8_t          sLength1;
    acp_uint8_t          sLength2;
    const acp_uint8_t*   sValue1;
    const acp_uint8_t*   sValue2;
    acp_sint32_t         sOrder;

    //---------
    // value1
    //---------

    sLength1 = ( aValueInfo1->length == 0 )
        ? MTD_NIBBLE_NULL_LENGTH
        : aValueInfo1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNibble.staticNull );

    sLength2 = sNibbleValue2->length;

    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sNibbleValue1 = (const mtdNibbleType*)aValueInfo1->value;
        sLength1 = sNibbleValue1->length;
        sValue1  = sNibbleValue1->value;

        sValue2  = sNibbleValue2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                      sLength2 >> 1  ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength2 & 1 )
            {
                if( ( sValue1[sLength2>>1] & 0xF0 ) <
                    ( sValue2[sLength2>>1] & 0xF0 ) )
                {
                    return 1;
                }
            }
            return -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                      sLength1 >> 1 ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength1 & 1 )
            {
                if( ( sValue1[sLength1>>1] & 0xF0 ) >
                    ( sValue2[sLength1>>1] & 0xF0 ) )
                {
                    return -1;
                }
            }
            return 1;
        }
        if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                  sLength1 >> 1 ) ) != 0 )
        {
            return sOrder;
        }
        if( sLength1 & 1 )
        {
            if( ( sValue1[sLength1>>1] & 0xF0 ) >
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return -1;
            }
            if( ( sValue1[sLength1>>1] & 0xF0 ) <
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return 1;
            }
        }
        return 0;
    }
    
    if( sLength1 > sLength2 )
    {
        return 1;
    }
    if( sLength1 < sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNibbleStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                 mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType* sNibbleValue1;
    const mtdNibbleType* sNibbleValue2;
    acp_uint8_t          sLength1;
    acp_uint8_t          sLength2;
    const acp_uint8_t*   sValue1;
    const acp_uint8_t*   sValue2;
    acp_sint32_t         sOrder;

    //---------
    // value1
    //---------

    sLength1 = ( aValueInfo1->length == 0 )
        ? MTD_NIBBLE_NULL_LENGTH
        : aValueInfo1->length;
    
    //---------
    // value2
    //---------

    sLength2 = ( aValueInfo2->length == 0 )
        ? MTD_NIBBLE_NULL_LENGTH
        : aValueInfo2->length;

    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sNibbleValue1 = (const mtdNibbleType*)aValueInfo1->value;
        sLength1 = sNibbleValue1->length;
        sValue1  = sNibbleValue1->value;

        sNibbleValue2 = (const mtdNibbleType*)aValueInfo2->value;
        sLength2 = sNibbleValue2->length;
        sValue2  = sNibbleValue2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                      sLength2 >> 1  ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength2 & 1 )
            {
                if( ( sValue1[sLength2>>1] & 0xF0 ) <
                    ( sValue2[sLength2>>1] & 0xF0 ) )
                {
                    return -1;
                }
            }
            return 1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                      sLength1 >> 1 ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength1 & 1 )
            {
                if( ( sValue1[sLength1>>1] & 0xF0 ) >
                    ( sValue2[sLength1>>1] & 0xF0 ) )
                {
                    return 1;
                }
            }
            return -1;
        }
        if( ( sOrder = acpMemCmp( sValue1, sValue2,
                                  sLength1 >> 1 ) ) != 0 )
        {
            return sOrder;
        }
        if( sLength1 & 1 )
        {
            if( ( sValue1[sLength1>>1] & 0xF0 ) >
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return 1;
            }
            if( ( sValue1[sLength1>>1] & 0xF0 ) <
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 > sLength2 )
    {
        return 1;
    }
    if( sLength1 < sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNibbleStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                  mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType* sNibbleValue1;
    const mtdNibbleType* sNibbleValue2;
    acp_uint8_t          sLength1;
    acp_uint8_t          sLength2;
    const acp_uint8_t*   sValue1;
    const acp_uint8_t*   sValue2;
    acp_sint32_t         sOrder;

    //---------
    // value1
    //---------

    sLength1 = ( aValueInfo1->length == 0 )
        ? MTD_NIBBLE_NULL_LENGTH
        : aValueInfo1->length;

    //---------
    // value2
    //---------

    sLength2 = ( aValueInfo2->length == 0 )
        ? MTD_NIBBLE_NULL_LENGTH
        : aValueInfo2->length;
    
    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sNibbleValue1 = (const mtdNibbleType*)aValueInfo1->value;
        sLength1 = sNibbleValue1->length;
        sValue1  = sNibbleValue1->value;

        sNibbleValue2 = (const mtdNibbleType*)aValueInfo2->value;
        sLength2 = sNibbleValue2->length;
        sValue2  = sNibbleValue2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                      sLength2 >> 1  ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength2 & 1 )
            {
                if( ( sValue1[sLength2>>1] & 0xF0 ) <
                    ( sValue2[sLength2>>1] & 0xF0 ) )
                {
                    return 1;
                }
            }
            return -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                      sLength1 >> 1 ) ) != 0 )
            {
                return sOrder;
            }
            if( sLength1 & 1 )
            {
                if( ( sValue1[sLength1>>1] & 0xF0 ) >
                    ( sValue2[sLength1>>1] & 0xF0 ) )
                {
                    return -1;
                }
            }
            return 1;
        }
        if( ( sOrder = acpMemCmp( sValue2, sValue1,
                                  sLength1 >> 1 ) ) != 0 )
        {
            return sOrder;
        }
        if( sLength1 & 1 )
        {
            if( ( sValue1[sLength1>>1] & 0xF0 ) >
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return -1;
            }
            if( ( sValue1[sLength1>>1] & 0xF0 ) <
                ( sValue2[sLength1>>1] & 0xF0 ) )
            {
                return 1;
            }
        }
        return 0;
    }
    
    if( sLength1 > sLength2 )
    {
        return 1;
    }
    if( sLength1 < sLength2 )
    {
        return -1;
    }
    return 0;
}

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonizedValue,
                           mtcEncryptInfo*  aCanonInfo ,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo ,
                           mtcTemplate*     aTemplate  )
{
    ACP_UNUSED(aCanonInfo);
    ACP_UNUSED(aColumnInfo);
    ACP_UNUSED(aTemplate);
    
    if( mtcdNibble.isNull( aColumn, aValue, MTD_OFFSET_USELESS ) != ACP_TRUE )
    {
        ACI_TEST_RAISE( ((mtdNibbleType*)aValue)->length >
                        (acp_uint32_t)aCanon->precision,
                        ERR_INVALID_PRECISION );
    }
    
    *aCanonizedValue = aValue;
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_PRECISION );
    aciSetErrorCode(mtERR_ABORT_INVALID_PRECISION);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

void mtdEndian( void* aTemp)
{
    ACP_UNUSED( aTemp);
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

    mtdNibbleType * sVal = (mtdNibbleType *)aValue;
    
    ACI_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL);

    ACI_TEST_RAISE( aValueSize == 0, ERR_INVALID_LENGTH );
    ACI_TEST_RAISE( sVal->length + sizeof(acp_uint8_t) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdNibble,
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

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t    aColumnSize,
                                       void*           aDestValue,
                                       acp_uint32_t    aDestValueOffset,
                                       acp_uint32_t    aLength,
                                       const void*     aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdNibbleType* sNibbleValue;

    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다. 

    sNibbleValue = (mtdNibbleType*)aDestValue;
    
    if( aLength == 0 )
    {
        // NULL 데이타
        sNibbleValue->length = MTD_NIBBLE_NULL_LENGTH;
    }
    else
    {
        // bit type은 mtdDataType형태로 저장되므로
        // length 에 header size가 포함되어 있어 
        // 여기서 별도의 계산을 하지 않아도 된다. 
        ACI_TEST_RAISE( (aDestValueOffset + aLength) > aColumnSize, ERR_INVALID_STORED_VALUE );

        acpMemCpy( sNibbleValue, aValue, aLength );
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
 * 예 ) mtdNibbleType( acp_uint8_t length; acp_uint8_t value[1] ) 에서
 *      length 타입인 acp_uint8_t의 크기를 반환
 *******************************************************************/
    
    return mtdActualSize( NULL,
                          & mtcdNibbleNull,
                          MTD_OFFSET_USELESS );   
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdNibbleType( acp_uint8_t length; acp_uint8_t value[1] ) 에서
 *      length 타입인 acp_uint8_t의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint8_t);
}

