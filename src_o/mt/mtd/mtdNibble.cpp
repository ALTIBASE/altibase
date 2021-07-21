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
 * $Id: mtdNibble.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdNibble;

#define MTD_NIBBLE_ALIGN             (ID_SIZEOF(UChar))

// To Remove Warning
const mtdNibbleType mtdNibbleNull = { MTD_NIBBLE_NULL_LENGTH, {'\0',} }; /* PROJ-2632 */

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

static SInt mtdNibbleLogicalAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleLogicalDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                 mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 );

static SInt mtdNibbleStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
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

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdNullValueSize();

static UInt mtdHeaderSize();

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"NIBBLE" }
};

static mtcColumn mtdColumn;

mtdModule mtdNibble = {
    mtdTypeName,
    &mtdColumn,
    MTD_NIBBLE_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
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
      MTD_DATA_STORE_MTDVALUE_TRUE|   // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    MTD_NIBBLE_PRECISION_MAXIMUM,
    0,
    0,
    (void*)&mtdNibbleNull,
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
        mtdNibbleLogicalAscComp,     // Logical Comparison
        mtdNibbleLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdNibbleFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdNibbleFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
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
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdNibbleFixedMtdFixedMtdKeyAscComp,
            mtdNibbleFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdNibbleMtdMtdKeyAscComp,
            mtdNibbleMtdMtdKeyDescComp
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
    IDE_TEST( mtd::initializeModule( &mtdNibble, aNo ) != IDE_SUCCESS );
    
    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdNibble,
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
        *aPrecision = MTD_NIBBLE_PRECISION_DEFAULT;
    }

    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    IDE_TEST_RAISE( (*aPrecision < MTD_NIBBLE_PRECISION_MINIMUM) ||
                    (*aPrecision > MTD_NIBBLE_PRECISION_MAXIMUM),
                    ERR_INVALID_LENGTH );

    *aColumnSize = ID_SIZEOF(UChar) + ( (*aPrecision + 1) >> 1);
    
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
    UInt           sValueOffset;
    mtdNibbleType* sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_NIBBLE_ALIGN );
    
    sValue = (mtdNibbleType*)( (UChar*)aValue + sValueOffset );
    
    *aResult = IDE_SUCCESS;
    
    if( (aTokenLength + 1) >> 1 <= (UChar*)aValue - sValue->value + aValueSize )
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
            IDE_TEST( mtc::makeNibble( sValue,
                                       (const UChar*)aToken,
                                       aTokenLength )
                      != IDE_SUCCESS );

            // precision, scale 재 설정 후, estimate로 semantic 검사
            aColumn->flag      = 1;
            aColumn->precision = sValue->length != 0
                                 ? sValue->length
                                 : MTD_NIBBLE_PRECISION_DEFAULT;
            aColumn->scale     = 0;
        }
       
        IDE_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != IDE_SUCCESS );
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset +
                                ID_SIZEOF(UChar) + ( (sValue->length+1) >> 1 );
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn* aColumn,
                    const void*      aRow )
{
    if( mtdNibble.isNull( aColumn, aRow ) == ID_TRUE )
    {
        return ID_SIZEOF(UChar);
    }
    else
    {
        return ID_SIZEOF(UChar) + ( (((mtdNibbleType*)aRow)->length + 1) >> 1 );
    }
}

static IDE_RC mtdGetPrecision( const mtcColumn * aColumn,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale )
{
    if ( mtdNibble.isNull( aColumn, aRow ) == ID_TRUE )
    {
        *aPrecision = 0;
        *aScale = 0;
    }
    else
    {
        *aPrecision = ((mtdNibbleType*)aRow)->length;
        *aScale = 0;
    }

    return IDE_SUCCESS;
}

void mtdSetNull( const mtcColumn* /*aColumn*/,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdNibbleType*)aRow)->length = MTD_NIBBLE_NULL_LENGTH;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* aColumn,
              const void*      aRow )
{
    if( mtdNibble.isNull( aColumn, aRow ) == ID_TRUE )
    {
        return aHash;
    }

    return mtc::hashSkip( aHash, ((mtdNibbleType*)aRow)->value, (((mtdNibbleType*)aRow)->length + 1) >> 1 );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return ( ((mtdNibbleType*)aRow)->length == MTD_NIBBLE_NULL_LENGTH ) ? ID_TRUE : ID_FALSE;
}

SInt mtdNibbleLogicalAscComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1      = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2      = sNibbleValue2->length;

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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
        if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdNibbleLogicalDescComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1      = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2      = sNibbleValue2->length;

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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
        if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdNibbleFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1      = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2      = sNibbleValue2->length;

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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
        if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdNibbleFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1      = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2      = sNibbleValue2->length;

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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
        if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdNibbleMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                          aValueInfo1->value,
                                          aValueInfo1->flag,
                                          mtdNibble.staticNull );
    sLength1 = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdNibble.staticNull );

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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
        if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdNibbleMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;

    //---------
    // value1
    //---------
    sNibbleValue1 = (const mtdNibbleType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                          aValueInfo1->value,
                                          aValueInfo1->flag,
                                          mtdNibble.staticNull );
    sLength1 = sNibbleValue1->length;

    //---------
    // value2
    //---------
    sNibbleValue2 = (const mtdNibbleType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdNibble.staticNull );

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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
        if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdNibbleStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;
    UInt                  sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = ( aValueInfo1->length == 0 )
                   ? MTD_NIBBLE_NULL_LENGTH
                   : aValueInfo1->length;

        sNibbleValue1 = (const mtdNibbleType *)aValueInfo1->value;
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sNibbleValue1 = (const mtdNibbleType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength1 = sNibbleValue1->length;
    }

    //---------
    // value2
    //---------
    
    sNibbleValue2 = (const mtdNibbleType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdNibble.staticNull );

    sLength2 = sNibbleValue2->length;

    //---------
    // compare
    //---------

    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sLength1 = sNibbleValue1->length;
        sValue1  = sNibbleValue1->value;

        sValue2  = sNibbleValue2->value;

        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
        if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdNibbleStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/
  
    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;
    UInt                  sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = ( aValueInfo1->length == 0 )
                   ? MTD_NIBBLE_NULL_LENGTH
                   : aValueInfo1->length;

        sNibbleValue1 = (const mtdNibbleType *)aValueInfo1->value;
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sNibbleValue1 = (const mtdNibbleType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength1 = sNibbleValue1->length;
    }

    //---------
    // value2
    //---------

    sNibbleValue2 = (const mtdNibbleType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdNibble.staticNull );

    sLength2 = sNibbleValue2->length;

    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sLength1 = sNibbleValue1->length;
        sValue1  = sNibbleValue1->value;

        sValue2  = sNibbleValue2->value;

        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
        if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdNibbleStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;
    UInt                  sLength;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = ( aValueInfo1->length == 0 )
                   ? MTD_NIBBLE_NULL_LENGTH
                   : aValueInfo1->length;

        sNibbleValue1 = (const mtdNibbleType *)aValueInfo1->value;
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sNibbleValue1 = (const mtdNibbleType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );

        sLength1 = sNibbleValue1->length;
    }
    
    //---------
    // value2
    //---------

    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength2 = ( aValueInfo2->length == 0 )
                   ? MTD_NIBBLE_NULL_LENGTH
                   : aValueInfo2->length;

        sNibbleValue2 = (const mtdNibbleType *)aValueInfo2->value;
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sNibbleValue2 = (const mtdNibbleType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );

        sLength2 = sNibbleValue2->length;
    }

    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sValue1  = sNibbleValue1->value;
        sLength1 = sNibbleValue1->length;

        sValue2  = sNibbleValue2->value;
        sLength2 = sNibbleValue2->length;

        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
            if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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
        if( ( sOrder = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdNibbleStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNibbleType * sNibbleValue1;
    const mtdNibbleType * sNibbleValue2;
    UChar                 sLength1;
    UChar                 sLength2;
    const UChar         * sValue1;
    const UChar         * sValue2;
    SInt                  sOrder;
    UInt                  sLength;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = ( aValueInfo1->length == 0 )
                   ? MTD_NIBBLE_NULL_LENGTH
                   : aValueInfo1->length;

        sNibbleValue1 = (const mtdNibbleType *)aValueInfo1->value;
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sNibbleValue1 = (const mtdNibbleType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );

        sLength1 = sNibbleValue1->length;
    }
    
    //---------
    // value2
    //---------

    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength2 = ( aValueInfo2->length == 0 )
                   ? MTD_NIBBLE_NULL_LENGTH
                   : aValueInfo2->length;

        sNibbleValue2 = (const mtdNibbleType *)aValueInfo2->value;
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sNibbleValue2 = (const mtdNibbleType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sLength );

        sLength2 = sNibbleValue2->length;
    }
    
    //---------
    // compare
    //---------
    
    if( ( sLength1 != MTD_NIBBLE_NULL_LENGTH ) &&
        ( sLength2 != MTD_NIBBLE_NULL_LENGTH ) )
    {
        sValue1  = sNibbleValue1->value;
        sLength1 = sNibbleValue1->length;

        sValue2  = sNibbleValue2->value;
        sLength2 = sNibbleValue2->length;

        if( sLength1 > sLength2 )
        {
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
            if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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
        if( ( sOrder = idlOS::memcmp( sValue2, sValue1,
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

static IDE_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonizedValue,
                           mtcEncryptInfo  * /* aCanonInfo */,
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * /* aColumnInfo */,
                           mtcTemplate     * /* aTemplate */ )
{
    if( mtdNibble.isNull( aColumn, aValue ) != ID_TRUE )
    {
        IDE_TEST_RAISE( ((mtdNibbleType*)aValue)->length >
                        (UInt)aCanon->precision,
                        ERR_INVALID_PRECISION );
    }
    
    *aCanonizedValue = aValue;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void mtdEndian( void* )
{
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
        
    mtdNibbleType * sVal = (mtdNibbleType *)aValue;
    
    IDE_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL);

    IDE_TEST_RAISE( aValueSize == 0, ERR_INVALID_LENGTH );
    IDE_TEST_RAISE( sVal->length + ID_SIZEOF(UChar) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdNibble,
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
        IDE_TEST_RAISE( (aDestValueOffset + aLength) > aColumnSize, ERR_INVALID_STORED_VALUE );

        idlOS::memcpy( sNibbleValue, aValue, aLength );
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
 * 예 ) mtdNibbleType( UChar length; UChar value[1] ) 에서
 *      length 타입인 UChar의 크기를 반환
 *******************************************************************/
    return mtdActualSize( NULL, &mtdNibbleNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdNibbleType( UChar length; UChar value[1] ) 에서
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
