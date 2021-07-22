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
 * $Id: mtcdNumeric.cpp 36510 2009-11-04 03:26:46Z sungminee $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdNumeric;
extern mtdModule mtcdFloat;
extern mtdModule mtcdDouble;

// To Remove Warning
const mtdNumericType mtcdNumericNull = { 0, 0, {'\0',} };

static ACI_RC mtdInitializeNumeric( acp_uint32_t aNo );

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

static acp_sint32_t mtdNumericLogicalComp( mtdValueInfo* aValueInfo1,
                                           mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNumericMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNumericMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                 mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNumericStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                   mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNumericStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                    mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNumericStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                      mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNumericStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
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

static acp_double_t mtdSelectivityNumeric( void* aColumnMax,
                                           void* aColumnMin,
                                           void* aValueMax,
                                           void* aValueMin );

static ACI_RC mtdValueFromOracle( mtcColumn*    aColumn,
                                  void*         aValue,
                                  acp_uint32_t* aValueOffset,
                                  acp_uint32_t  aValueSize,
                                  const void*   aOracleValue,
                                  acp_sint32_t  aOracleLength,
                                  ACI_RC*       aResult );

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t  aColumnSize,
                                       void*         aDestValue,
                                       acp_uint32_t  aDestValueOffset,
                                       acp_uint32_t  aLength,
                                       const void*   aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[2] = {
    { mtdTypeName+1, 7, (void*)"NUMERIC" },
    { NULL,          7, (void*)"DECIMAL" }
};

static mtcColumn mtdColumn;

mtdModule mtcdNumeric = {
    mtdTypeName,
    &mtdColumn,
    MTD_NUMERIC_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_NUMERIC_ALIGN,
    MTD_GROUP_NUMBER|
    MTD_CANON_NEED_WITH_ALLOCATION|
    MTD_CREATE_ENABLE| 
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_ENABLE|
    MTD_NUMBER_GROUP_TYPE_NUMERIC|
    MTD_CREATE_PARAM_PRECISIONSCALE|
    MTD_SEARCHABLE_PRED_BASIC|
    MTD_UNSIGNED_ATTR_TRUE|
    MTD_NUM_PREC_RADIX_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|   // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE|  // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,   // PROJ-1705
    MTD_NUMERIC_PRECISION_MAXIMUM,
    MTD_NUMERIC_SCALE_MINIMUM,
    MTD_NUMERIC_SCALE_MAXIMUM,
    (void*)&mtcdNumericNull,
    mtdInitializeNumeric,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdNumericLogicalComp,    // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdNumericMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdNumericMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdNumericStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdNumericStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdNumericStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdNumericStoredStoredKeyDescComp // Descending Key Comparison
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtdSelectivityNumeric,    
    mtdEncodeNumericDefault,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracle,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSize
};

ACI_RC mtdInitializeNumeric( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdNumeric, aNo ) != ACI_SUCCESS );
    
    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdNumeric,
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
    switch( *aArguments )
    {
        case 0:
            *aPrecision = MTD_NUMERIC_PRECISION_DEFAULT;
        case 1:
            *aScale     = MTD_NUMERIC_SCALE_DEFAULT;
            *aArguments = 2;
        default:
            break;
    }
    
    ACI_TEST_RAISE( *aPrecision < MTD_NUMERIC_PRECISION_MINIMUM ||
                    *aPrecision > MTD_NUMERIC_PRECISION_MAXIMUM,
                    ERR_INVALID_PRECISION );

    ACI_TEST_RAISE( *aScale < MTD_NUMERIC_SCALE_MINIMUM ||
                    *aScale > MTD_NUMERIC_SCALE_MAXIMUM,
                    ERR_INVALID_SCALE );

    *aColumnSize = MTD_NUMERIC_SIZE( *aPrecision, *aScale );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_PRECISION );
    aciSetErrorCode(mtERR_ABORT_INVALID_PRECISION);
    
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
    mtdNumericType* sValue;

    ACP_UNUSED( aTemplate);
    
    if( *aValueOffset + MTD_NUMERIC_SIZE_MAXIMUM <= aValueSize )
    {
        sValue = (mtdNumericType*)( (acp_uint8_t*)aValue + *aValueOffset );
        ACI_TEST( mtcMakeNumeric( sValue,
                                  MTD_NUMERIC_MANTISSA_MAXIMUM,
                                  (const acp_uint8_t*)aToken,
                                  aTokenLength )
                  != ACI_SUCCESS );

        // To Fix BUG-12612
        // precision, scale 재 설정 후, estimate로 semantic 검사

        ACI_TEST( mtcGetPrecisionScaleFloat( sValue,
                                             &aColumn->precision,
                                             &aColumn->scale )
                  != ACI_SUCCESS );

        aColumn->flag = 2;

        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != ACI_SUCCESS );
        
        aColumn->column.offset   = *aValueOffset;
        *aValueOffset         += aColumn->column.size; 
        
        *aResult = ACI_SUCCESS;
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
    const mtdNumericType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNumeric.staticNull );

    return sizeof(acp_uint8_t) + sValue->length;
}

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale )
{
    const mtdNumericType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNumeric.staticNull );

    ACI_TEST( mtcGetPrecisionScaleFloat( sValue,
                                         aPrecision,
                                         aScale )
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdNumericType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdNumericType*)
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
    const mtdNumericType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNumeric.staticNull );

    // fix BUG-9496
    return mtcHashWithExponent( aHash, &sValue->signExponent, sValue->length );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdNumericType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNumeric.staticNull );
    
    return (sValue->length == 0) ? ACP_TRUE : ACP_FALSE ;
}

acp_sint32_t
mtdNumericLogicalComp( mtdValueInfo* aValueInfo1,
                       mtdValueInfo* aValueInfo2 )
{
    return mtdNumericMtdMtdKeyAscComp( aValueInfo1,
                                       aValueInfo2 );
}

acp_sint32_t
mtdNumericMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                            mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType* sNumericValue1;
    const mtdNumericType* sNumericValue2;
    acp_uint8_t           sLength1;
    acp_uint8_t           sLength2;
    const acp_uint8_t*    sSignExponentMantissa1;
    const acp_uint8_t*    sSignExponentMantissa2;
    acp_sint32_t          sOrder;

    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNumeric.staticNull );
    
    sLength1 = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNumeric.staticNull );
    
    sLength2 = sNumericValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = &(sNumericValue1->signExponent);
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa1,
                                      sSignExponentMantissa2,
                                      sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa1,
                                      sSignExponentMantissa2,
                                      sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return acpMemCmp( sSignExponentMantissa1,
                          sSignExponentMantissa2,
                          sLength1 );
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
mtdNumericMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                             mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType* sNumericValue1;
    const mtdNumericType* sNumericValue2;
    acp_uint8_t           sLength1;
    acp_uint8_t           sLength2;
    const acp_uint8_t*    sSignExponentMantissa1;
    const acp_uint8_t*    sSignExponentMantissa2;
    acp_sint32_t          sOrder;
    
    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNumeric.staticNull );
    
    sLength1 = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNumeric.staticNull );

    sLength2 = sNumericValue2->length;

    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = &(sNumericValue1->signExponent);
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength2 > sLength1 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa2,
                                      sSignExponentMantissa1,
                                      sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa2,
                                      sSignExponentMantissa1,
                                      sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return acpMemCmp( sSignExponentMantissa2,
                          sSignExponentMantissa1,
                          sLength2 );
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
mtdNumericStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                               mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType* sNumericValue2;
    acp_uint8_t           sLength1;
    acp_uint8_t           sLength2;
    const acp_uint8_t*    sSignExponentMantissa1;
    const acp_uint8_t*    sSignExponentMantissa2;
    acp_sint32_t          sOrder;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint8_t)aValueInfo1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNumeric.staticNull );
    
    sLength2 = sNumericValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (acp_uint8_t*)aValueInfo1->value;
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa1,
                                      sSignExponentMantissa2,
                                      sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa1,
                                      sSignExponentMantissa2,
                                      sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return acpMemCmp( sSignExponentMantissa1,
                          sSignExponentMantissa2,
                          sLength1 );
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
mtdNumericStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/    

    const mtdNumericType* sNumericValue2;
    acp_uint8_t           sLength1;
    acp_uint8_t           sLength2;
    const acp_uint8_t*    sSignExponentMantissa1;
    const acp_uint8_t*    sSignExponentMantissa2;
    acp_sint32_t          sOrder;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint8_t)aValueInfo1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNumeric.staticNull );
    
    sLength2       = sNumericValue2->length;
  
    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (acp_uint8_t*)aValueInfo1->value;
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength2 > sLength1 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa2,
                                      sSignExponentMantissa1,
                                      sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa2,
                                      sSignExponentMantissa1,
                                      sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return acpMemCmp( sSignExponentMantissa2,
                          sSignExponentMantissa1,
                          sLength2 );
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
mtdNumericStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                  mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t                  sLength1;
    acp_uint8_t                  sLength2;
    const acp_uint8_t*           sSignExponentMantissa1;
    const acp_uint8_t*           sSignExponentMantissa2;
    acp_sint32_t                 sOrder;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint8_t)aValueInfo1->length;

    //---------
    // value2
    //---------
    sLength2 = (acp_uint8_t)aValueInfo2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (acp_uint8_t*)aValueInfo1->value;
        sSignExponentMantissa2 = (acp_uint8_t*)aValueInfo2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa1,
                                      sSignExponentMantissa2,
                                      sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa1,
                                      sSignExponentMantissa2,
                                      sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return acpMemCmp( sSignExponentMantissa1,
                          sSignExponentMantissa2,
                          sLength1 );
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
mtdNumericStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                   mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t                  sLength1;
    acp_uint8_t                  sLength2;
    const acp_uint8_t*           sSignExponentMantissa1;
    const acp_uint8_t*           sSignExponentMantissa2;
    acp_sint32_t                 sOrder;
    
    //---------
    // value1
    //---------
    sLength1 = (acp_uint8_t)aValueInfo1->length;
    
    //---------
    // value2
    //---------
    sLength2 = (acp_uint8_t)aValueInfo2->length;
    
    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (acp_uint8_t*)aValueInfo1->value;
        sSignExponentMantissa2 = (acp_uint8_t*)aValueInfo2->value;
        
        if( sLength2 > sLength1 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa2,
                                      sSignExponentMantissa1,
                                      sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = acpMemCmp( sSignExponentMantissa2,
                                      sSignExponentMantissa1,
                                      sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return acpMemCmp( sSignExponentMantissa2,
                          sSignExponentMantissa1,
                          sLength2 );
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
                           void**           aCanonizedValue,
                           mtcEncryptInfo*  aCanonInfo ,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate )
{
    acp_bool_t sCanonized = ACP_TRUE;

    ACP_UNUSED(aCanonInfo);
    ACP_UNUSED(aColumnInfo);
    
    if( ( aCanon->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK ) == 1 )
    {
        ACI_TEST( mtcdFloat.canonize( aCanon,
                                     aCanonizedValue,
                                     NULL,
                                     aColumn,
                                     aValue,
                                     NULL,
                                     aTemplate )
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST( mtcNumericCanonize( (mtdNumericType*)aValue,
                                      (mtdNumericType*)*aCanonizedValue,
                                      aCanon->precision,
                                      aCanon->scale,
                                      &sCanonized )
                  != ACI_SUCCESS );

        if( sCanonized == ACP_FALSE )
        {
            *aCanonizedValue = aValue;
        }
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

void mtdEndian( void* aTemp )
{
    ACP_UNUSED( aTemp);
    
    /* nothing to do */

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
        
    acp_sint32_t     i;
    mtdNumericType * sVal = (mtdNumericType*)aValue;
    acp_uint32_t     sPrecision;
    
    ACI_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL );

    ACI_TEST_RAISE((aValueSize == 0) ||
                   (sizeof(acp_uint8_t) + sVal->length != aValueSize),
                   ERR_INVALID_LENGTH);

    if(sVal->length > 0) // not null
    {
        sPrecision = (sVal->length -1) * 2;
    
        if(sVal->length == 1) // 0
        {
            ACI_TEST(sVal->signExponent != 0x80);
            //sPrecision = 0;
        }
        else if(sVal->signExponent >= 0x80) // positive value
        {
            ACI_TEST(sVal->mantissa[0] == 0 ||
                     sVal->mantissa[sVal->length - 2] == 0 );
            for(i = 0; i < sVal->length - 1; i++)
            {
                ACI_TEST(sVal->mantissa[i] >= 100);
            }
            if(sVal->mantissa[0] < 10)
            {
                sPrecision--;
            }
            if(sVal->mantissa[sVal->length - 2] % 10 == 0)
            {
                sPrecision--;
            }
        }
        else // negative value
        {
            ACI_TEST(sVal->mantissa[0] == 99 ||
                     sVal->mantissa[sVal->length - 2] == 99 );
            for(i = 0; i < sVal->length - 1; i++)
            {
                ACI_TEST(sVal->mantissa[i] >= 100);
            }
            if(sVal->mantissa[0] > 90)
            {
                sPrecision--;
            }
            if((sVal->length > 2) &&
               (sVal->mantissa[sVal->length - 2] % 10 == 9))
            {
                sPrecision--;
            }
        }
    }
    else // NULL
    {
        sPrecision = MTD_NUMERIC_PRECISION_MINIMUM;
    }
    
    // ACI_TEST( mtdEstimate( aColumn, 1, sPrecision, 0 ) != ACI_SUCCESS);
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdNumeric,
                                   1,            // arguments
                                   sPrecision,   // precision
                                   0 )           // scale
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

acp_double_t
mtdSelectivityNumeric( void*  aColumnMax,
                       void*  aColumnMin,
                       void*  aValueMax,
                       void*  aValueMin )
{
/***********************************************************************
 *
 * Description :
 *    FLOAT, NUMERIC 의 Selectivity 추출 함수
 *
 * Implementation :
 *
 *    Selectivity = (aValueMax - aValueMin) / (aColumnMax - aColumnMin)
 *    0 < Selectivity <= 1 의 값을 리턴함
 *
 ***********************************************************************/
    
    mtdNumericType* sColumnMax;
    mtdNumericType* sColumnMin;
    mtdNumericType* sValueMax;
    mtdNumericType* sValueMin;
    acp_double_t    sSelectivity;
    acp_double_t    sDenominator;  // 분모값
    acp_double_t    sNumerator;    // 분자값

    acp_uint8_t     sNumericBuffer[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumeric;
    acp_uint8_t     sBuffer[64];
    mtaVarcharType* sVarchar;
    mtdValueInfo    sValueInfo1;
    mtdValueInfo    sValueInfo2;
    mtdValueInfo    sValueInfo3;
    mtdValueInfo    sValueInfo4;

    sNumeric = (mtdNumericType*)sNumericBuffer;
    
    sColumnMax = (mtdNumericType*) aColumnMax;
    sColumnMin = (mtdNumericType*) aColumnMin;
    sValueMax  = (mtdNumericType*) aValueMax;
    sValueMin  = (mtdNumericType*) aValueMin;

    //------------------------------------------------------
    // Data의 유효성 검사
    //     NULL 검사 : 계산할 수 없음
    //------------------------------------------------------

    if ( ( sColumnMax->length == 0 ) ||
         ( sColumnMin->length == 0 ) ||
         ( sValueMax->length == 0 )  ||
         ( sValueMin->length == 0 ) )
    {
        // Data중 NULL 이 있을 경우
        // 부등호의 Default Selectivity인 1/3을 Setting함
        sSelectivity = MTD_DEFAULT_SELECTIVITY;
    }
    else
    {
        //------------------------------------------------------
        // 유효성 검사
        // 다음의 경우는 조건을 잘못된 통계 정보이거나 입력 정보임.
        // Column의 Min값보다 Value의 Max값이 작은 경우
        // Column의 Max값보다 Value의 Min값이 큰 경우
        //------------------------------------------------------

        sValueInfo1.column = NULL;
        sValueInfo1.value  = sColumnMin;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = sValueMax;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sValueInfo3.column = NULL;
        sValueInfo3.value  = sColumnMax;
        sValueInfo3.flag   = MTD_OFFSET_USELESS;

        sValueInfo4.column = NULL;
        sValueInfo4.value  = sValueMin;
        sValueInfo4.flag   = MTD_OFFSET_USELESS;

        if ( ( mtdNumericLogicalComp( &sValueInfo1,
                                      &sValueInfo2 ) > 0 )
             ||
             ( mtdNumericLogicalComp( &sValueInfo3,
                                      &sValueInfo4 ) < 0 ) )
        {
            // To Fix PR-11858
            sSelectivity = 0;
        }
        else
        {
            //------------------------------------------------------
            // Value값 보정
            // Value의 Min값이 Column의 Min값보다 작다면 보정
            // Value의 Max값이 Column의 Max값보다 크다면 보정
            //------------------------------------------------------

            sValueInfo1.column = NULL;
            sValueInfo1.value  = sValueMin;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = sColumnMin;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            
            if ( mtdNumericLogicalComp( &sValueInfo1,
                                        &sValueInfo2 ) < 0 )
            {
                sValueMin = sColumnMin;
            }
            else
            {
                // Nothing To Do
            }

            sValueInfo1.column = NULL;
            sValueInfo1.value  = sValueMax;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = sColumnMax;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( mtdNumericLogicalComp( &sValueInfo1,
                                        &sValueInfo2 ) > 0 )
            {
                sValueMax = sColumnMax;
            }
            else
            {
                // Nothing To Do
            }

            //------------------------------------------------------
            // 분모값 (aColumnMax - aColumnMin) 값 획득
            //------------------------------------------------------

            // sColumnMax - sColumnMin
            if ( mtcSubtractFloat( sNumeric,
                                   MTD_FLOAT_PRECISION_MAXIMUM,
                                   sColumnMax,
                                   sColumnMin ) != ACI_SUCCESS )
            {
                // Value Overflow가 발생할 수 있음
                sDenominator = 0.0;
            }
            else
            {
                sVarchar = (mtaVarcharType*)sBuffer;

                // Numeric -> Varchar로 변환
                if ( mtaVarcharExact( sVarchar,
                                      60,
                                      (mtaNumericType*)
                                      & (sNumeric->signExponent),
                                      sNumeric->length ) != ACI_SUCCESS )
                {
                    // 변환 중 Error 발생할 수 있음
                    sDenominator = 0.0;
                }
                else
                {
                    // Varchar를 Double로 변환
                    sVarchar->value[MTA_GET_LENGTH(sVarchar->length)] = 0;
        
/*                    sDenominator = idlOS::strtod( (char*)sVarchar->value,
                      (char**)NULL );BUGBUG*/
                    
                    ACE_ASSERT( acpCStrToDouble((const acp_char_t*)sVarchar->value,
                                                acpCStrLen((acp_char_t*)sVarchar->value,
                                                           MTA_GET_LENGTH(sVarchar->length)),
                                                &sDenominator,
                                                (acp_char_t**)NULL)
                                == ACP_RC_SUCCESS );
        
                    if ( mtcdDouble.isNull( NULL,
                                           & sDenominator,
                                           MTD_OFFSET_USELESS ) == ACP_TRUE )
                    {
                        // 변환결과가 NULL인 경우
                        sDenominator = 0.0;
                    }
                    else
                    {
                        // 올바른 분모값 획득
                        // Nothing To do
                    }
                }
            }
        
            if ( sDenominator <= 0.0 )
            {
                // 잘못된 통계 정보의 사용한 경우
                sSelectivity = MTD_DEFAULT_SELECTIVITY;
            }
            else
            {
                //------------------------------------------------------
                // 분자값 (aValueMax - aValueMin) 값 획득
                //------------------------------------------------------

                // sValueMax - sValueMin
                if ( mtcSubtractFloat( sNumeric,
                                       MTD_FLOAT_PRECISION_MAXIMUM,
                                       sValueMax,
                                       sValueMin ) != ACI_SUCCESS )
                {
                    // Value Overflow가 발생할 수 있음
                    sNumerator = 0.0;
                }
                else
                {
                    sVarchar = (mtaVarcharType*)sBuffer;

                    // Numeric -> Varchar로 변환
                    if ( mtaVarcharExact(
                             sVarchar,
                             60,
                             (mtaNumericType*) & (sNumeric->signExponent),
                             sNumeric->length ) != ACI_SUCCESS )
                    {
                        // 변환 중 Error 발생할 수 있음
                        sNumerator = 0.0;
                    }
                    else
                    {
                        // Varchar를 Double로 변환
                        sVarchar->value[MTA_GET_LENGTH(sVarchar->length)] = 0;
                        
/*                        sNumerator = idlOS::strtod( (char*)sVarchar->value,
                          (char**)NULL );BUBUG*/

                        ACE_ASSERT( acpCStrToDouble((const acp_char_t*)sVarchar->value,
                                                    acpCStrLen((acp_char_t*)sVarchar->value,
                                                               MTA_GET_LENGTH(sVarchar->length)),
                                                    &sNumerator,
                                                    (acp_char_t**)NULL)
                                    == ACE_RC_SUCCESS );
                        
                        if ( mtcdDouble.isNull( NULL,
                                               & sNumerator,
                                               MTD_OFFSET_USELESS ) == ACP_TRUE)
                        {
                            // 변환결과가 NULL인 경우
                            sNumerator = 0.0;
                        }
                        else
                        {
                            // 올바른 분자값 획득
                            // Nothing To do
                        }
                    }
                }
            
                if ( sNumerator <= 0.0 )
                {
                    // 잘못된 입력 정보인 경우
                    // To Fix PR-11858
                    sSelectivity = 0;
                }
                else
                {
                    //------------------------------------------------------
                    // Selectivity 계산
                    //------------------------------------------------------
                
                    sSelectivity = sNumerator / sDenominator;
                    if( sSelectivity > 1.0 )
                    {
                        sSelectivity = 1.0;
                    }
                }
            }
        }
    }
    
    return sSelectivity;
}

ACI_RC mtdValueFromOracle( mtcColumn*    aColumn,
                           void*         aValue,
                           acp_uint32_t* aValueOffset,
                           acp_uint32_t  aValueSize,
                           const void*   aOracleValue,
                           acp_sint32_t  aOracleLength,
                           ACI_RC*       aResult )
{
    mtdNumericType*       sValue;
    const acp_uint8_t*    sOracleValue;
    acp_sint32_t          sIterator;
    
    if( *aValueOffset + MTD_FLOAT_SIZE_MAXIMUM <= aValueSize )
    {
        sValue       = (mtdNumericType*)( (acp_uint8_t*)aValue + *aValueOffset );
        sOracleValue = (const acp_uint8_t*)aOracleValue;
        
        if( aOracleLength <= 0 )
        {
            sValue->length = 0;
        }
        else if( aOracleLength == 1 )
        {
            ACI_TEST_RAISE( sOracleValue[0] != 0x80, ERR_INVALID_LITERAL );
            sValue->length       = 1;
            sValue->signExponent = 0x80;
        }
        else
        {
            if( sOracleValue[0] >= 0x80  )
            {
                ACI_TEST_RAISE( aOracleLength > MTD_FLOAT_SIZE_MAXIMUM - 1,
                                ERR_INVALID_LENGTH );
                if( sOracleValue[0] > 0x80 )
                {
                    sValue->length       = aOracleLength;
                    sValue->signExponent = sOracleValue[0];
                    for( sIterator = 1;
                         sIterator < aOracleLength;
                         sIterator++ )
                    {
                        sValue->mantissa[sIterator-1] =
                            sOracleValue[sIterator] - 1;
                    }
                }
                else
                {
                    sValue->length       = 1;
                    sValue->signExponent = 0x80;
                }
            }
            else
            {
                ACI_TEST_RAISE( aOracleLength > MTD_FLOAT_SIZE_MAXIMUM,
                                ERR_INVALID_LENGTH );
                if( sOracleValue[0] < 0x7F )
                {
                    sValue->length       = aOracleLength - 1;
                    sValue->signExponent = sOracleValue[0] + 1;
                    for( sIterator = 1;
                         sIterator < aOracleLength - 1;
                         sIterator++ )
                    {
                        sValue->mantissa[sIterator-1] =
                            sOracleValue[sIterator] - 2;
                    }
                    ACI_TEST_RAISE( sOracleValue[sIterator] != 102,
                                    ERR_INVALID_LITERAL );
                }
                else
                {
                    sValue->length       = 1;
                    sValue->signExponent = 0x80;
                }
            }
        }
        
        ACI_TEST( mtdValidate( aColumn, sValue, sValue->length + 1 )
                  != ACI_SUCCESS );
        
/*        *aValueOffset += aColumn->column.size;BUGBUG*/
        
        *aResult = ACI_SUCCESS;
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void*             aDestValue,
                                       acp_uint32_t      aDestValueOffset,
                                       acp_uint32_t      aLength,
                                       const void*       aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdNumericType* sNumericValue;

    // 가변길이 데이타 타입이지만,
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sNumericValue = (mtdNumericType*)aDestValue;

    if( aLength == 0 )
    {
        // NULL 데이타
        sNumericValue->length = 0;
    }
    else
    {
        ACI_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sNumericValue->length = (acp_uint8_t)aLength;
        acpMemCpy( (acp_uint8_t*)sNumericValue + mtdHeaderSize(), aValue, aLength );
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
 * 예 ) mtdNumericType( acp_uint8_t length; acp_uint8_t signExponent; acp_uint8_t mantissa[1] ) 에서
 *      length 타입인 acp_uint8_t의 크기를 반환
 *******************************************************************/

    return mtdActualSize( NULL,
                          & mtcdNumericNull,
                          MTD_OFFSET_USELESS );
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdNumericType( acp_uint8_t length; acp_uint8_t signExponent; acp_uint8_t mantissa[1] ) 에서
 *      length 타입인 acp_uint8_t의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint8_t);
}

