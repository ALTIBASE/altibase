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
 * $Id: mtdVarbyte.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdVarbyte;

// To Remove Warning
const mtdByteType mtdVarbyteNull = { 0, {'\0',} };

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

static SInt mtdVarbyteLogicalAscComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteLogicalDescComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                              mtdValueInfo * aValueInfo2 );

static SInt mtdVarbyteStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static IDE_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonized,
                           mtcEncryptInfo  * aCanonInfo,
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * aColumnInfo,
                           mtcTemplate     * aTemplate );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdNullValueSize();

static UInt mtdHeaderSize();

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcName mtdTypeName[2] = {
    { mtdTypeName + 1, 7, (void*)"VARBYTE" },
    { NULL,            3, (void*)"RAW" }
};

static mtcColumn mtdColumn;

mtdModule mtdVarbyte = {
    mtdTypeName,
    &mtdColumn,
    MTD_VARBYTE_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_VARBYTE_ALIGN,
    MTD_GROUP_MISC|
      MTD_CANON_NEED|
      MTD_CREATE_ENABLE|
      MTD_COLUMN_TYPE_VARIABLE|
      MTD_SELECTIVITY_DISABLE|
      MTD_CREATE_PARAM_PRECISION|
      MTD_SEARCHABLE_SEARCHABLE|    // BUG-17020
      MTD_LITERAL_TRUE|
      MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_TRUE|  // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    MTD_BYTE_STORE_PRECISION_MAXIMUM,
    0,
    0,
    (void*)&mtdVarbyteNull,
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
        mtdVarbyteLogicalAscComp,     // Logical Comparison
        mtdVarbyteLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdVarbyteFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbyteFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdVarbyteMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbyteMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdVarbyteStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbyteStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdVarbyteStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdVarbyteStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdVarbyteFixedMtdFixedMtdKeyAscComp,
            mtdVarbyteFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdVarbyteMtdMtdKeyAscComp,
            mtdVarbyteMtdMtdKeyDescComp
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtd::selectivityNA,
    mtd::encodeNA,
    mtd::decodeDefault,
    mtd::compileFmtDefault,
    mtd::valueFromOracleDefault,
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

    // PROJ-2399
    mtdStoreSize 
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdVarbyte, aNo ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdVarbyte,
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
                    SInt * /*aScale*/ )
{
    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_VARBYTE_PRECISION_DEFAULT;
    }

    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    IDE_TEST_RAISE( *aPrecision < MTD_VARBYTE_PRECISION_MINIMUM ||
                    *aPrecision > MTD_VARBYTE_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = ID_SIZEOF(UShort) + *aPrecision;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

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
    UInt         sValueOffset;
    mtdByteType* sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_VARBYTE_ALIGN );

    sValue = (mtdByteType*)( (UChar*)aValue + sValueOffset );

    *aResult = IDE_SUCCESS;
    
    if( ( (aTokenLength + 1) >> 1 ) <=
        (UChar*)aValue - sValue->value + aValueSize )
    {
        IDE_TEST( mtc::makeByte( sValue,
                                 (const UChar*)aToken,
                                 aTokenLength )
                  != IDE_SUCCESS );
        
        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;
        aColumn->precision       = sValue->length;
        aColumn->scale           = 0;

        IDE_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != IDE_SUCCESS );
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + 
                                   ID_SIZEOF(UShort) + sValue->length; 
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
    return ID_SIZEOF(UShort) + ((mtdByteType*)aRow)->length;
}

static IDE_RC mtdGetPrecision( const mtcColumn * ,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale )
{
    *aPrecision = ((mtdByteType*)aRow)->length;
    *aScale = 0;

    return IDE_SUCCESS;
}

void mtdSetNull( const mtcColumn* /*aColumn*/,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdByteType*)aRow)->length = 0;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hashSkip( aHash, ((mtdByteType*)aRow)->value, ((mtdByteType*)aRow)->length );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (((mtdByteType*)aRow)->length == 0) ? ID_TRUE : ID_FALSE;
}

SInt mtdVarbyteLogicalAscComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1    = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2    = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength1 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
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

SInt mtdVarbyteLogicalDescComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1    = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2    = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;

        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength2 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
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

SInt mtdVarbyteFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1    = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2    = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength1 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
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

SInt mtdVarbyteFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1    = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2    = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;

        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength2 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
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

SInt mtdVarbyteMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdVarbyte.staticNull );

    sLength1    = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdVarbyte.staticNull );

    sLength2    = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength1 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
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

SInt mtdVarbyteMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sByteValue1 = (const mtdByteType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdVarbyte.staticNull );

    sLength1    = sByteValue1->length;

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdVarbyte.staticNull );

    sLength2    = sByteValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sByteValue1->value;
        sValue2  = sByteValue2->value;

        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength2 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
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

SInt mtdVarbyteStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    UInt                 sLength;
    
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sByteValue1 = (const mtdByteType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );
        sLength1 = sByteValue1->length;
        sValue1  = sByteValue1->value;
    }

    //---------
    // value1
    //---------
    sByteValue2 = (const mtdByteType*)
                  mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                       aValueInfo2->value,
                                       aValueInfo2->flag,
                                       mtdVarbyte.staticNull );
    sLength2 = sByteValue2->length;
    sValue2  = sByteValue2->value;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength1 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
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

SInt mtdVarbyteStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key 와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    UInt                 sLength;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sByteValue1 = (const mtdByteType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );
        sLength1 = sByteValue1->length;
        sValue1  = sByteValue1->value;
    }

    //---------
    // value2
    //---------
    sByteValue2 = (const mtdByteType*)
                  mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                       aValueInfo2->value,
                                       aValueInfo2->flag,
                                       mtdVarbyte.staticNull );
    sLength2 = sByteValue2->length;
    sValue2  = sByteValue2->value;

    //---------
    // compare
    //---------    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength2 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
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

SInt mtdVarbyteStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    UInt                 sLength;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sByteValue1 = (const mtdByteType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );
        sLength1 = sByteValue1->length;
        sValue1  = sByteValue1->value;
    }
    
    //---------
    // value2
    //---------
    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength2 = (UShort)aValueInfo2->length;
        sValue2  = (UChar*)aValueInfo2->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sByteValue2 = (const mtdByteType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );
        sLength2= sByteValue2->length;
        sValue2  = sByteValue2->value;
    }

    //---------
    // compare
    //---------    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  sLength1 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
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

SInt mtdVarbyteStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description :  Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdByteType  * sByteValue1;
    const mtdByteType  * sByteValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    UInt                 sLength;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sByteValue1 = (const mtdByteType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );
        sLength1 = sByteValue1->length;
        sValue1  = sByteValue1->value;
    }
    
    //---------
    // value2
    //---------
    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength2 = (UShort)aValueInfo2->length;
        sValue2  = (UChar*)aValueInfo2->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sByteValue2 = (const mtdByteType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );
        sLength2= sByteValue2->length;
        sValue2  = sByteValue2->value;
    }

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  sLength2 ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
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
                           void**            aCanonized,
                           mtcEncryptInfo  * /* aCanonInfo */,
                           const mtcColumn * /* aColumn */,
                           void*             aValue,
                           mtcEncryptInfo  * /* aColumnInfo */,
                           mtcTemplate     * /* aTemplate */ )
{

    IDE_TEST_RAISE( ((mtdByteType*)aValue)->length > (UInt)aCanon->precision,
                   ERR_INVALID_LENGTH );
    
    *aCanonized = aValue;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void mtdEndian( void* aValue )
{
    UChar* sLength;
    UChar  sIntermediate;

    sLength = (UChar*)&(((mtdByteType*)aValue)->length);
    sIntermediate = sLength[0];
    sLength[0]    = sLength[1];
    sLength[1]    = sIntermediate;
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
        
    mtdByteType * sVal = (mtdByteType*)aValue;

    IDE_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );

    IDE_TEST_RAISE((aValueSize < ID_SIZEOF(UShort)) ||
                   (sVal->length + ID_SIZEOF(UShort) != aValueSize),
                   ERR_INVALID_LENGTH );

    IDE_TEST_RAISE( sVal->length > aColumn->column.size, ERR_INVALID_VALUE );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdVarbyte,
                                     1,              // arguments
                                     sVal->length,   // precision
                                     0 )             // scale
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
    IDE_EXCEPTION( ERR_INVALID_VALUE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE));
    }
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

    mtdByteType* sByteValue;

    sByteValue = (mtdByteType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sByteValue->length = 0;
    }
    else
    {
         IDE_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sByteValue->length = (UShort)(aDestValueOffset + aLength);
        idlOS::memcpy( (UChar*)sByteValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
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
 * 예 ) mtdByteType( UShort length; UChar value[1] ) 에서
 *      length타입의 UShort의 크기를 반환
 *******************************************************************/

    return mtdActualSize( NULL, &mtdVarbyteNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdByteType( UShort length; UChar value[1] ) 에서
 *      length타입의 UShort의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return ID_SIZEOF(UShort);
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
