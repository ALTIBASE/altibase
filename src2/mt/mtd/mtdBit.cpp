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
 * $Id: mtdBit.cpp 17938 2006-09-05 00:54:03Z leekmo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdBit;

// To Remove Warning
const mtdBitType mtdBitNull = { 0, {'\0',} }; /* PROJ-2632 */

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

static SInt mtdBitLogicalAscComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 );

static SInt mtdBitLogicalDescComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 );

static SInt mtdBitFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                              mtdValueInfo * aValueInfo2 );

static SInt mtdBitFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static SInt mtdBitMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 );

static SInt mtdBitMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdBitStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdBitStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdBitStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static SInt mtdBitStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
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

static mtcName mtdTypeName[1] = {
    { NULL, 3, (void*)"BIT" }
};

static mtcColumn mtdColumn;

mtdModule mtdBit = {
    mtdTypeName,
    &mtdColumn,
    MTD_BIT_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
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
      MTD_DATA_STORE_MTDVALUE_TRUE|   // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    MTD_BIT_STORE_PRECISION_MAXIMUM,
    0,
    0,
    (void*)&mtdBitNull,
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
        mtdBitLogicalAscComp,     // Logical Comparison
        mtdBitLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdBitFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdBitFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
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
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdBitFixedMtdFixedMtdKeyAscComp,
            mtdBitFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdBitMtdMtdKeyAscComp,
            mtdBitMtdMtdKeyDescComp
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
    mtd::mtdStoreSizeDefault
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdBit, aNo ) != IDE_SUCCESS );
    
    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdBit,
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
        *aPrecision = MTD_BIT_PRECISION_DEFAULT;
    }

    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    IDE_TEST_RAISE( *aPrecision < MTD_BIT_PRECISION_MINIMUM ||
                    *aPrecision > MTD_BIT_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = ID_SIZEOF(UInt) + ( BIT_TO_BYTE(*aPrecision) );
    
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
    UInt        sValueOffset;
    mtdBitType* sValue;

    sValueOffset = idlOS::align( *aValueOffset, MTD_BIT_ALIGN );
    
    sValue = (mtdBitType*)( (UChar*)aValue + sValueOffset );
    
    *aResult = IDE_SUCCESS;
    
    if( BIT_TO_BYTE(aTokenLength) <= (UChar*)aValue - sValue->value + aValueSize )
    {
        if( aTokenLength == 0 )
        {
            sValue->length = 0;
        }
        else
        {
            IDE_TEST( mtc::makeBit( sValue,
                                    (const UChar*)aToken,
                                    aTokenLength )
                      != IDE_SUCCESS );
        }

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;
        aColumn->precision       = sValue->length != 0 ? sValue->length : 1;
        aColumn->scale           = 0;
        
        IDE_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != IDE_SUCCESS );
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset +
                                   ID_SIZEOF(UInt) + BIT_TO_BYTE(sValue->length);
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
    if( mtdBit.isNull( aColumn, aRow ) == ID_TRUE )
    {
        return ID_SIZEOF(UInt);
    }
    else
    {
        return ID_SIZEOF(UInt) + BIT_TO_BYTE(((mtdBitType*)aRow)->length);
    }
}

static IDE_RC mtdGetPrecision( const mtcColumn * ,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale )
{
    *aPrecision = ((mtdBitType*)aRow)->length;
    *aScale = 0;

    return IDE_SUCCESS;
}

void mtdSetNull( const mtcColumn* /*aColumn*/,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdBitType*)aRow)->length = 0;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* aColumn,
              const void*      aRow )
{
    if( mtdBit.isNull( aColumn, aRow ) == ID_TRUE )
    {
        return aHash;
    }

    return mtc::hashSkipWithoutZero( aHash,
                                     ((mtdBitType*)aRow)->value,
                                     BIT_TO_BYTE(((mtdBitType*)aRow)->length) );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return ( ((mtdBitType*)aRow)->length == 0 ) ? ID_TRUE : ID_FALSE;
}

SInt mtdBitLogicalAscComp( mtdValueInfo * aValueInfo1,
                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1   = sBitValue1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2   = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2,
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
            sCompared = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdBitLogicalDescComp( mtdValueInfo * aValueInfo1,
                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1   = sBitValue1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2   = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1,
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
            sCompared = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdBitFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1   = sBitValue1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2   = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2,
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
            sCompared = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdBitFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1   = sBitValue1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2   = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1,
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
            sCompared = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdBitMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                             mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdBit.staticNull );

    sLength1    = sBitValue1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdBit.staticNull );

    sLength2    = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2,
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
            sCompared = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdBitMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sBitValue1 = (const mtdBitType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdBit.staticNull );

    sLength1    = sBitValue1->length;

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdBit.staticNull );

    sLength2    = sBitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sBitValue1->value;
        sValue2  = sBitValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1,
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
            sCompared = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdBitStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    UInt                 sStoredLength1;
    const UChar        * sValue1 = NULL;
    const UChar        * sValue2;
    UInt                 sDummy;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength1 = aValueInfo1->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if ( sStoredLength1 != 0 )
        {
            ID_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
            sValue1  = ((UChar*)aValueInfo1->value) + mtdHeaderSize();
        }
        else
        {
            sLength1 = 0;
        }
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sBitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sBitValue1->length;
        sValue1  = sBitValue1->value;
    }

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)
                  mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                       aValueInfo2->value,
                                       aValueInfo2->flag,
                                       mtdBit.staticNull );
    sLength2 = sBitValue2->length;
    sValue2  = sBitValue2->value;

    //---------
    // compare
    //---------

    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2,
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
            sCompared = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdBitStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    UInt                 sStoredLength1;
    const UChar        * sValue1 = NULL;
    const UChar        * sValue2;
    UInt                 sDummy;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength1 = aValueInfo1->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if ( sStoredLength1 != 0 )
        {
            ID_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
            sValue1  = ((UChar*)aValueInfo1->value) + mtdHeaderSize();
        }
        else
        {
            sLength1 = 0;
        }
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sBitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sBitValue1->length;
        sValue1  = sBitValue1->value;
    }

    //---------
    // value2
    //---------
    sBitValue2 = (const mtdBitType*)
                  mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                       aValueInfo2->value,
                                       aValueInfo2->flag,
                                       mtdBit.staticNull );
    sLength2 = sBitValue2->length;
    sValue2  = sBitValue2->value;

    //---------
    // compare
    //---------
    
    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1,
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
            sCompared = idlOS::memcmp( sValue2, sValue1,
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

SInt mtdBitStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    UInt                 sStoredLength1;
    UInt                 sStoredLength2;
    const UChar        * sValue1 = NULL;
    const UChar        * sValue2;
    UInt                 sDummy;

    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength1 = aValueInfo1->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if( sStoredLength1 != 0 )
        {
            ID_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
            sValue1  = ((UChar*)aValueInfo1->value) + mtdHeaderSize();
        }
        else
        {
            sLength1 = 0;
        }
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sBitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sBitValue1->length;
        sValue1  = sBitValue1->value;
    }

    //---------
    // value2
    //---------
    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength2 = aValueInfo2->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo2->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if( sStoredLength2 != 0 )
        {
            ID_INT_BYTE_ASSIGN( &sLength2, aValueInfo2->value );
            sValue2  = ((UChar*)aValueInfo2->value) + mtdHeaderSize();
        }
        else
        {
            sLength2 = 0;
        }
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sBitValue2 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength2 = sBitValue2->length;
        sValue2  = sBitValue2->value;
    }

    //---------
    // compare
    //---------    

    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2,
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
            sCompared = idlOS::memcmp( sValue1, sValue2,
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

SInt mtdBitStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sBitValue1;
    const mtdBitType   * sBitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    UInt                 sStoredLength1;
    UInt                 sStoredLength2;
    const UChar        * sValue1 = NULL;
    const UChar        * sValue2;
    UInt                 sDummy;
    
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength1 = aValueInfo1->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if( sStoredLength1 != 0 )
        {
            ID_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
            sValue1  = ((UChar*)aValueInfo1->value) + mtdHeaderSize();
        }
        else
        {
            sLength1 = 0;
        }
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sBitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sBitValue1->length;
        sValue1  = sBitValue1->value;
    }

    //---------
    // value2
    //---------
    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength2 = aValueInfo2->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo2->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if( sStoredLength2 != 0 )
        {
            ID_INT_BYTE_ASSIGN( &sLength2, aValueInfo2->value );
            sValue2  = ((UChar*)aValueInfo2->value) + mtdHeaderSize();
        }
        else
        {
            sLength2 = 0;
        }
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );
    
        sBitValue2 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength2 = sBitValue2->length;
        sValue2  = sBitValue2->value;
    }

    //---------
    // compare
    //---------    

    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1,
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
            sCompared = idlOS::memcmp( sValue2, sValue1,
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

static IDE_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonized,
                           mtcEncryptInfo  * /* aCanonInfo */,
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * /* aColumnInfo */,
                           mtcTemplate     * /* aTemplate */ )
{
    mtdBitType* sCanonized;
    mtdBitType* sValue;

    sValue = (mtdBitType*)aValue;
    
    if( sValue->length == (UInt)aCanon->precision )
    {
        *aCanonized = aValue;
    }
    else
    {
        IDE_TEST_RAISE( sValue->length > (UInt)aCanon->precision,
                        ERR_INVALID_LENGTH );

        sCanonized = (mtdBitType*)*aCanonized;

        if( mtdBit.isNull( aColumn, sValue ) == ID_TRUE )
        {
            sCanonized->length = 0;
        }
        else
        {
            idlOS::memset( sCanonized->value,
                           0x00,
                           BIT_TO_BYTE( aCanon->precision ) );
            idlOS::memcpy( sCanonized->value,
                           sValue->value,
                           BIT_TO_BYTE( sValue->length ) );
            sCanonized->length = aCanon->precision;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH ));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void mtdEndian( void* aValue )
{
    UChar* sLength;
    UChar  sIntermediate;

    sLength = (UChar*)&(((mtdBitType*)aValue)->length);
    sIntermediate = sLength[0];
    sLength[0]    = sLength[3];
    sLength[3]    = sIntermediate;
    sIntermediate = sLength[1];
    sLength[1]    = sLength[2];
    sLength[2]    = sIntermediate;
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
        
    mtdBitType * sVal = (mtdBitType *)aValue;
    
    IDE_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL);

    IDE_TEST_RAISE( aValueSize == 0, ERR_INVALID_LENGTH );
    IDE_TEST_RAISE( BIT_TO_BYTE(sVal->length) + ID_SIZEOF(UInt) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdBit,
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

IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                void            * aDestValue,
                                UInt              aDestValueOffset,
                                UInt              aLength,
                                const void      * aValue )
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
        IDE_TEST_RAISE( (aDestValueOffset + aLength) > aColumnSize, ERR_INVALID_STORED_VALUE );

        idlOS::memcpy( (UChar*)sBitValue + aDestValueOffset, aValue, aLength );
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
/***********************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 * 예 ) mtdBitType( UInt length; UChar value[1] ) 에서
 *      length 타입인  UInt의 크기를 반환
 **********************************************************************/

    return mtdActualSize( NULL, &mtdBitNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdBitType( UInt length; UChar value[1] ) 에서
 *      length 타입인  UInt의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return ID_SIZEOF(UInt);
}
