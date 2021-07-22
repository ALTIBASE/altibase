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
 * $Id: mtdNumeric.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdNumeric;
extern mtdModule mtdFloat;
extern mtdModule mtdDouble;

// To Remove Warning
const mtdNumericType mtdNumericNull = { 0, 0, {'\0',} };

static IDE_RC mtdInitialize( UInt aNo );

static IDE_RC mtdEstimate( UInt * aColumnSize,
                           UInt * aArguments,
                           SInt * aPrecision,
                           SInt * aScale );

static IDE_RC mtdValue( mtcTemplate* aTemplate,
                        mtcColumn*   aColumn,
                        void*        aValue,
                        UInt*        aValueOffset,
                        UInt         aValueSize,
                        const void*  aToken,
                        UInt         aTokenLength,
                        IDE_RC*      aResult );

static UInt mtdActualSize( const mtcColumn* aColumn,
                           const void*      aRow );

static IDE_RC mtdGetPrecision( const mtcColumn * aColumn,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale );

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static UInt mtdHash( UInt             aHash,
                     const mtcColumn* aColumn,
                     const void*      aRow );

static idBool mtdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static SInt mtdNumericLogicalAscComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdNumericLogicalDescComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdNumericFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 );

static SInt mtdNumericFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

static SInt mtdNumericMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdNumericMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdNumericStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdNumericStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

static SInt mtdNumericStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                              mtdValueInfo * aValueInfo2 );

static SInt mtdNumericStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static IDE_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonizedValue,
                           mtcEncryptInfo  * aCanonInfo,
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * aColumnInfo,
                           mtcTemplate     * aTemplate );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static IDE_RC mtdValueFromOracle( mtcColumn*  aColumn,
                                  void*       aValue,
                                  UInt*       aValueOffset,
                                  UInt        aValueSize,
                                  const void* aOracleValue,
                                  SInt        aOracleLength,
                                  IDE_RC*     aResult );

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdNullValueSize();

static UInt mtdHeaderSize();

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcName mtdTypeName[2] = {
    { mtdTypeName+1, 7, (void*)"NUMERIC" },
    { NULL,          7, (void*)"DECIMAL" }
};

static mtcColumn mtdColumn;

mtdModule mtdNumeric = {
    mtdTypeName,
    &mtdColumn,
    MTD_NUMERIC_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
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
      MTD_DATA_STORE_MTDVALUE_FALSE|   // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    MTD_NUMERIC_PRECISION_MAXIMUM,
    MTD_NUMERIC_SCALE_MINIMUM,
    MTD_NUMERIC_SCALE_MAXIMUM,
    (void*)&mtdNumericNull,
    mtdInitialize,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdSetNull,
    mtdHash,
    mtdIsNull,
    mtd::isTrueNA,
    {
        mtdNumericLogicalAscComp,    // Logical Comparison
        mtdNumericLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdNumericFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdNumericFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
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
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdNumericFixedMtdFixedMtdKeyAscComp,
            mtdNumericFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdNumericMtdMtdKeyAscComp,
            mtdNumericMtdMtdKeyDescComp
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtdDouble.selectivity,    
    mtd::encodeNumericDefault,
    mtd::decodeDefault,
    mtd::compileFmtDefault,
    mtdValueFromOracle,
    mtd::makeColumnInfoDefault,

    // BUG-28934
    mtk::mergeAndRangeDefault,
    mtk::mergeOrRangeListDefault,

    {
        // PROJ-1705
        mtdStoredValue2MtdValue,
        // PROJ-2429
        mtd::mtdStoredValue2MtdValue4CompressColumn
    },
    mtdNullValueSize,
    mtdHeaderSize,

    //PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdNumeric, aNo ) != IDE_SUCCESS );
    
     // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdNumeric,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * aArguments,
                    SInt * aPrecision,
                    SInt * aScale )
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
    
    IDE_TEST_RAISE( *aPrecision < MTD_NUMERIC_PRECISION_MINIMUM ||
                    *aPrecision > MTD_NUMERIC_PRECISION_MAXIMUM,
                    ERR_INVALID_PRECISION );

    IDE_TEST_RAISE( *aScale < MTD_NUMERIC_SCALE_MINIMUM ||
                    *aScale > MTD_NUMERIC_SCALE_MAXIMUM,
                    ERR_INVALID_SCALE );

    *aColumnSize = MTD_NUMERIC_SIZE( *aPrecision, *aScale );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));
    
    IDE_EXCEPTION( ERR_INVALID_SCALE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_SCALE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdValue( mtcTemplate* /* aTemplate */,
                 mtcColumn*   aColumn,
                 void*        aValue,
                 UInt*        aValueOffset,
                 UInt         aValueSize,
                 const void*  aToken,
                 UInt         aTokenLength,
                 IDE_RC*      aResult )
{
    mtdNumericType* sValue;
    
    if( *aValueOffset + MTD_NUMERIC_SIZE_MAXIMUM <= aValueSize )
    {
        sValue = (mtdNumericType*)( (UChar*)aValue + *aValueOffset );
        IDE_TEST( mtc::makeNumeric( sValue,
                                    MTD_NUMERIC_MANTISSA_MAXIMUM,
                                    (const UChar*)aToken,
                                    aTokenLength )
                  != IDE_SUCCESS );

        // To Fix BUG-12612
        // precision, scale 재 설정 후, estimate로 semantic 검사

        IDE_TEST( mtc::getPrecisionScaleFloat( sValue,
                                               &aColumn->precision,
                                               &aColumn->scale )
                  != IDE_SUCCESS );

        aColumn->flag = 2;

        IDE_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != IDE_SUCCESS );
        
        aColumn->column.offset   = *aValueOffset;
        *aValueOffset         += aColumn->column.size;
        
        *aResult = IDE_SUCCESS;
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn* ,
                    const void*      aRow )
{
    return ID_SIZEOF(UChar) + ((mtdNumericType*)aRow)->length;
}

static IDE_RC mtdGetPrecision( const mtcColumn * ,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale )
{
    (void)mtc::getPrecisionScaleFloat( ((mtdNumericType*)aRow),
                                       aPrecision,
                                       aScale );
    return IDE_SUCCESS;
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdNumericType*)aRow)->length = 0;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    // fix BUG-9496
    return mtc::hashWithExponent( aHash, &((mtdNumericType*)aRow)->signExponent, ((mtdNumericType*)aRow)->length );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (((mtdNumericType*)aRow)->length == 0) ? ID_TRUE : ID_FALSE ;
}

SInt mtdNumericLogicalAscComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType * sNumericValue1;
    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1       = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2       = sNumericValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = &(sNumericValue1->signExponent);
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa1,
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

SInt mtdNumericLogicalDescComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType * sNumericValue1;
    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1       = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2       = sNumericValue2->length;

    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = &(sNumericValue1->signExponent);
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength2 > sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa2,
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

SInt mtdNumericFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType * sNumericValue1;
    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1       = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2       = sNumericValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = &(sNumericValue1->signExponent);
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa1,
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

SInt mtdNumericFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType * sNumericValue1;
    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1       = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2       = sNumericValue2->length;

    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = &(sNumericValue1->signExponent);
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength2 > sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa2,
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

SInt mtdNumericMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType * sNumericValue1;
    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                           aValueInfo1->value,
                                           aValueInfo1->flag,
                                           mtdNumeric.staticNull );
    
    sLength1 = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdNumeric.staticNull );

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
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa1,
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

SInt mtdNumericMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType * sNumericValue1;
    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sNumericValue1 = (const mtdNumericType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                           aValueInfo1->value,
                                           aValueInfo1->flag,
                                           mtdNumeric.staticNull );
    
    sLength1 = sNumericValue1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdNumeric.staticNull );

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
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa2,
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

SInt mtdNumericStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sLength1 = (UChar)aValueInfo1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdNumeric.staticNull );
    
    sLength2 = sNumericValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (UChar*)aValueInfo1->value;
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa1,
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

SInt mtdNumericStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/    

    const mtdNumericType * sNumericValue2;
    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sLength1 = (UChar)aValueInfo1->length;

    //---------
    // value2
    //---------
    sNumericValue2 = (const mtdNumericType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdNumeric.staticNull );
    
    sLength2       = sNumericValue2->length;
  
    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (UChar*)aValueInfo1->value;
        sSignExponentMantissa2 = &(sNumericValue2->signExponent);
        
        if( sLength2 > sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa2,
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

SInt mtdNumericStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;

    //---------
    // value1
    //---------
    sLength1 = (UChar)aValueInfo1->length;

    //---------
    // value2
    //---------
    sLength2 = (UChar)aValueInfo2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (UChar*)aValueInfo1->value;
        sSignExponentMantissa2 = (UChar*)aValueInfo2->value;
        
        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? 1 : -1;
        }
        if( sLength1 < sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa1,
                                          sSignExponentMantissa2,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa1 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa1,
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

SInt mtdNumericStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar                  sLength1;
    UChar                  sLength2;
    const UChar          * sSignExponentMantissa1;
    const UChar          * sSignExponentMantissa2;
    SInt                   sOrder;    
    
    //---------
    // value1
    //---------
    sLength1 = (UChar)aValueInfo1->length;
    
    //---------
    // value2
    //---------
    sLength2 = (UChar)aValueInfo2->length;
    
    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sSignExponentMantissa1 = (UChar*)aValueInfo1->value;
        sSignExponentMantissa2 = (UChar*)aValueInfo2->value;
        
        if( sLength2 > sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength1 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? 1 : -1;
        }
        if( sLength2 < sLength1 )
        {
            if( ( sOrder = idlOS::memcmp( sSignExponentMantissa2,
                                          sSignExponentMantissa1,
                                          sLength2 ) ) != 0 )
            {
                return sOrder;
            }
            return *sSignExponentMantissa2 >= 0x80 ? -1 : 1;
        }
        return idlOS::memcmp( sSignExponentMantissa2,
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

static IDE_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonizedValue,
                           mtcEncryptInfo  * /* aCanonInfo */,
                           const mtcColumn * /* aColumn */,
                           void*             aValue,
                           mtcEncryptInfo  * /* aColumnInfo */,
                           mtcTemplate     * /* aTemplate */ )
{
    idBool sCanonized = ID_TRUE;
    
    if( ( aCanon->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK ) == 1 )
    {
        IDE_TEST( mtc::floatCanonize( (mtdNumericType*)aValue,
                                      (mtdNumericType*)*aCanonizedValue,
                                      aCanon->precision,
                                      &sCanonized )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mtc::numericCanonize( (mtdNumericType*)aValue,
                                        (mtdNumericType*)*aCanonizedValue,
                                        aCanon->precision,
                                        aCanon->scale,
                                        &sCanonized )
                  != IDE_SUCCESS );
    }
    
    if( sCanonized == ID_FALSE )
    {
        *aCanonizedValue = aValue;
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void mtdEndian( void* )
{
    /* nothing to do */
    return;    
}


IDE_RC mtdValidate( mtcColumn * aColumn,
                    void      * aValue,
                    UInt        aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
        
    SInt             i;
    mtdNumericType * sVal = (mtdNumericType*)aValue;
    UInt             sPrecision;
    
    IDE_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL );

    IDE_TEST_RAISE((aValueSize == 0) ||
                   (ID_SIZEOF(UChar) + sVal->length != aValueSize),
                   ERR_INVALID_LENGTH);

    if(sVal->length > 0) // not null
    {
        sPrecision = (sVal->length -1) * 2;
    
        if(sVal->length == 1) // 0
        {
            IDE_TEST(sVal->signExponent != 0x80);
            //sPrecision = 0;
        }
        else if(sVal->signExponent >= 0x80) // positive value
        {
            IDE_TEST(sVal->mantissa[0] == 0 ||
                     sVal->mantissa[sVal->length - 2] == 0 );
            for(i = 0; i < sVal->length - 1; i++)
            {
                IDE_TEST(sVal->mantissa[i] >= 100);
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
            IDE_TEST(sVal->mantissa[0] == 99 ||
                     sVal->mantissa[sVal->length - 2] == 99 );
            for(i = 0; i < sVal->length - 1; i++)
            {
                IDE_TEST(sVal->mantissa[i] >= 100);
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
    
    // IDE_TEST( mtdEstimate( aColumn, 1, sPrecision, 0 ) != IDE_SUCCESS);
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdNumeric,
                                     1,            // arguments
                                     sPrecision,   // precision
                                     0 )           // scale
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NULL);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE));
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdValueFromOracle( mtcColumn*  aColumn,
                           void*       aValue,
                           UInt*       aValueOffset,
                           UInt        aValueSize,
                           const void* aOracleValue,
                           SInt        aOracleLength,
                           IDE_RC*     aResult )
{
    mtdNumericType* sValue;
    const UChar*    sOracleValue;
    SInt            sIterator;
    
    if( *aValueOffset + MTD_FLOAT_SIZE_MAXIMUM <= aValueSize )
    {
        sValue       = (mtdNumericType*)( (UChar*)aValue + *aValueOffset );
        sOracleValue = (const UChar*)aOracleValue;
        
        if( aOracleLength <= 0 )
        {
            sValue->length = 0;
        }
        else if( aOracleLength == 1 )
        {
            IDE_TEST_RAISE( sOracleValue[0] != 0x80, ERR_INVALID_LITERAL );
            sValue->length       = 1;
            sValue->signExponent = 0x80;
        }
        else
        {
            if( sOracleValue[0] >= 0x80  )
            {
                IDE_TEST_RAISE( aOracleLength > MTD_FLOAT_SIZE_MAXIMUM - 1,
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
                IDE_TEST_RAISE( aOracleLength > MTD_FLOAT_SIZE_MAXIMUM,
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
                    IDE_TEST_RAISE( sOracleValue[sIterator] != 102,
                                    ERR_INVALID_LITERAL );
                }
                else
                {
                    sValue->length       = 1;
                    sValue->signExponent = 0x80;
                }
            }
        }
        
        IDE_TEST( mtdValidate( aColumn, sValue, sValue->length + 1 )
                  != IDE_SUCCESS );
        
        *aValueOffset += aColumn->column.size;
        
        *aResult = IDE_SUCCESS;
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue )
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
        IDE_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sNumericValue->length = (UChar)aLength;        
        idlOS::memcpy( (UChar*)sNumericValue + mtdHeaderSize(), aValue, aLength );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


UInt mtdNullValueSize()
{
/*******************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 * 예 ) mtdNumericType( UChar length; UChar signExponent; UChar mantissa[1] ) 에서
 *      length 타입인 UChar의 크기를 반환
 *******************************************************************/
    return mtdActualSize( NULL, &mtdNumericNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdNumericType( UChar length; UChar signExponent; UChar mantissa[1] ) 에서
 *      length 타입인 UChar의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return ID_SIZEOF(UChar);
}

static UInt mtdStoreSize( const smiColumn * /*aColumn*/ ) 
{
/***********************************************************************
 * PROJ-2399 row tmaplate 
 * sm에 저장되는 데이터의 크기를 반환한다.
 * variable 타입의 데이터 타입은 ID_UINT_MAX를 반환
 * mtheader가 sm에 저장된경우가 아니면 mtheader크기를 빼서 반환
 **********************************************************************/

    return ID_UINT_MAX;
}
