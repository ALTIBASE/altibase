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
 * $Id: mtdVarbit.cpp 17938 2006-09-05 00:54:03Z leekmo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdVarbit;
extern mtdModule mtdBit;

// To Remove Warning
const mtdBitType mtdVarbitNull = { 0, {'\0',} };

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

static SInt mtdVarbitLogicalAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitLogicalDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                 mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 );

static SInt mtdVarbitStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
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
    { NULL, 6, (void*)"VARBIT" }
};

static mtcColumn mtdColumn;

mtdModule mtdVarbit = {
    mtdTypeName,
    &mtdColumn,
    MTD_VARBIT_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_VARBIT_ALIGN,
    MTD_GROUP_TEXT|
      MTD_CANON_NEED|
      MTD_CREATE_ENABLE|
      MTD_COLUMN_TYPE_VARIABLE|
      MTD_SELECTIVITY_DISABLE|
      MTD_CREATE_PARAM_PRECISION|
      MTD_SEARCHABLE_SEARCHABLE|    // BUG-17020
      MTD_LITERAL_TRUE|
      MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_TRUE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_TRUE|  // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    MTD_BIT_STORE_PRECISION_MAXIMUM,
    0,
    0,
    (void*)&mtdVarbitNull,
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
        mtdVarbitLogicalAscComp,     // Logical Comparison
        mtdVarbitLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdVarbitFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbitFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdVarbitMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbitMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdVarbitStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbitStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdVarbitStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdVarbitStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdVarbitFixedMtdFixedMtdKeyAscComp,
            mtdVarbitFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdVarbitMtdMtdKeyAscComp,
            mtdVarbitMtdMtdKeyDescComp
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
    IDE_TEST( mtd::initializeModule( &mtdVarbit, aNo ) != IDE_SUCCESS );
    
    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdVarbit,
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
        *aPrecision = MTD_VARBIT_PRECISION_DEFAULT;
    }

    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    IDE_TEST_RAISE( *aPrecision < MTD_VARBIT_PRECISION_MINIMUM ||
                    *aPrecision > MTD_VARBIT_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = ID_SIZEOF(UInt) + BIT_TO_BYTE(*aPrecision);
    
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
    UInt               sValueOffset;
    mtdBitType*        sValue;
    const UChar*       sToken;
    const UChar*       sTokenFence;
    UChar*             sIterator;

    UInt               sSubIndex;
    
    static const UChar sBinary[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         0,  1, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_VARBIT_ALIGN );
    
    sValue = (mtdBitType*)( (UChar*)aValue + sValueOffset );
    
    *aResult = IDE_SUCCESS;
    
    sIterator = sValue->value;
    
    if( BIT_TO_BYTE(aTokenLength) <= (UChar*)aValue - sIterator + aValueSize )
    {
        if( aTokenLength == 0 )
        {
            sValue->length = 0;
        }
        else
        {
            sValue->length = 0;
            
            for( sToken      = (const UChar*)aToken,
                 sTokenFence = sToken + aTokenLength;
                 sToken      < sTokenFence;
                 sIterator++, sToken += 8 )
            {
                IDE_TEST_RAISE( sBinary[sToken[0]] > 1, ERR_INVALID_LITERAL );

                // 값 넣기 전에 0으로 초기화
                idlOS::memset( sIterator,
                               0x00,
                               1 );

                *sIterator = sBinary[sToken[0]] << 7;

                sValue->length++;
                IDE_TEST_RAISE( sValue->length == 0,
                                ERR_INVALID_LITERAL );

                sSubIndex = 1;
                while( sToken + sSubIndex < sTokenFence && sSubIndex < 8 )
                {
                    IDE_TEST_RAISE( sBinary[sToken[sSubIndex]] > 1, ERR_INVALID_LITERAL );
                    *sIterator |= ( sBinary[sToken[sSubIndex]] << ( 7 - sSubIndex ) );
                    sValue->length++;
                    sSubIndex++;
                }
            }
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
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn* aColumn,
                    const void*      aRow )
{
    if( mtdVarbit.isNull( aColumn, aRow ) == ID_TRUE )
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
    if( mtdVarbit.isNull( aColumn, aRow ) == ID_TRUE )
    {
        return aHash;
    }

    return mtc::hashSkip( aHash,
                          ((mtdBitType*)aRow)->value,
                          BIT_TO_BYTE(((mtdBitType*)aRow)->length) );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return ( ((mtdBitType*)aRow)->length == 0 ) ? ID_TRUE : ID_FALSE;
}

SInt mtdVarbitLogicalAscComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sVarbitValue1 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1      = sVarbitValue1->length;

    //---------
    // value2
    //---------
    sVarbitValue2 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2      = sVarbitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) );
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

SInt mtdVarbitLogicalDescComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sVarbitValue1 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1      = sVarbitValue1->length;

    //---------
    // value2
    //---------
    sVarbitValue2 = (const mtdBitType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2      = sVarbitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) );
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

SInt mtdVarbitFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sVarbitValue1 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1      = sVarbitValue1->length;

    //---------
    // value2
    //---------
    sVarbitValue2 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2      = sVarbitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) );
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

SInt mtdVarbitFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------
    sVarbitValue1 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1      = sVarbitValue1->length;

    //---------
    // value2
    //---------
    sVarbitValue2 = (const mtdBitType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2      = sVarbitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) );
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

SInt mtdVarbitMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------        
    sVarbitValue1 = (const mtdBitType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                          aValueInfo1->value,
                                          aValueInfo1->flag,
                                          mtdVarbit.staticNull );
    
    sLength1 = sVarbitValue1->length;

    //---------
    // value2
    //---------        
    sVarbitValue2 = (const mtdBitType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) );
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

SInt mtdVarbitMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    //---------
    // value1
    //---------        
    sVarbitValue1 = (const mtdBitType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                          aValueInfo1->value,
                                          aValueInfo1->flag,
                                          mtdVarbit.staticNull );
    
    sLength1 = sVarbitValue1->length;

    //---------
    // value2
    //---------        
    sVarbitValue2 = (const mtdBitType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) );
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

SInt mtdVarbitStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    UInt                 sStoredLength1;
    const UChar        * sValue1 = NULL;
    const UChar        * sValue2;
    UInt                 sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
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

        sVarbitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength1 = sVarbitValue1->length;
        sValue1  = sVarbitValue1->value;
    }

    //---------
    // value2
    //---------        
    sVarbitValue2 = (const mtdBitType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;
    sValue2  = sVarbitValue2->value;
    
    //---------
    // compare
    //---------

    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) );
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

SInt mtdVarbitStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType   * sVarbitValue1;
    const mtdBitType   * sVarbitValue2;
    UInt                 sLength1;
    UInt                 sLength2;
    UInt                 sStoredLength1;
    const UChar        * sValue1 = NULL;
    const UChar        * sValue2;
    UInt                 sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength1 = aValueInfo1->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if  ( sStoredLength1 != 0 )
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

        sVarbitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength1 = sVarbitValue1->length;
        sValue1  = sVarbitValue1->value;
    }

    //---------
    // value2
    //---------        
    sVarbitValue2 = (const mtdBitType*)
                     mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;
    sValue2  = sVarbitValue2->value;
    
    //---------
    // compare
    //---------
    
    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) );
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

SInt mtdVarbitStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType * sVarbitValue1;
    const mtdBitType * sVarbitValue2;
    UInt               sLength1;
    UInt               sLength2;
    UInt               sStoredLength1;
    UInt               sStoredLength2;
    const UChar      * sValue1 = NULL;
    const UChar      * sValue2;
    UInt               sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
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

        sVarbitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength1 = sVarbitValue1->length;
        sValue1  = sVarbitValue1->value;
    }

    //---------
    // value2
    //---------
    if( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength2 = aValueInfo2->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo2->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if ( sStoredLength2 != 0 )
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

        sVarbitValue2 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength2 = sVarbitValue2->length;
        sValue2  = sVarbitValue2->value;
    }

    //---------
    // compare
    //---------    
    
    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength1 > sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return idlOS::memcmp( sValue1, sValue2,
                                  BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) );
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

SInt mtdVarbitStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType * sVarbitValue1;
    const mtdBitType * sVarbitValue2;
    UInt               sLength1;
    UInt               sLength2;
    UInt               sStoredLength1;
    UInt               sStoredLength2;
    const UChar      * sValue1 = NULL;
    const UChar      * sValue2;
    UInt               sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
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

        sVarbitValue1 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength1 = sVarbitValue1->length;
        sValue1  = sVarbitValue1->value;
    }

    //---------
    // value2
    //---------
    if( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sStoredLength2 = aValueInfo2->length;

        // length가 0( == NULL )이 아닌 경우에
        // aValueInfo2->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        if ( sStoredLength2 != 0 )
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

        sVarbitValue2 = (const mtdBitType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );

        sLength2 = sVarbitValue2->length;
        sValue2  = sVarbitValue2->value;
    }

    //---------
    // compare
    //---------    
    
    if( (sLength1 != 0) && (sLength2 != 0) )
    {
        if( sLength2 > sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return idlOS::memcmp( sValue2, sValue1,
                                  BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return idlOS::memcmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) );
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
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * /* aColumnInfo */,
                           mtcTemplate     * /* aTemplate */ )
{
    if( mtdVarbit.isNull( aColumn, aValue ) != ID_TRUE )
    {
        IDE_TEST_RAISE( ((mtdBitType*)aValue)->length > (UInt)aCanon->precision,
                        ERR_INVALID_LENGTH );
    }
    
    *aCanonizedValue = aValue;
    
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
                                     & mtdVarbit,
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

    mtdBitType  * sVarbitValue;

    sVarbitValue = (mtdBitType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sVarbitValue->length = 0;
    }
    else
    {
        // bit type은 mtdDataType형태로 저장되므로
        // length 에 header size가 포함되어 있어
        // 여기서 별도의 계산을 하지 않아도 된다.
        IDE_TEST_RAISE( (aDestValueOffset + aLength) > aColumnSize, ERR_INVALID_STORED_VALUE );

        idlOS::memcpy( (UChar*)sVarbitValue + aDestValueOffset, aValue, aLength );
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
 * 예 ) mtdBitType( UInt length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *******************************************************************/
    return mtdActualSize( NULL, &mtdVarbitNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdBitType( UInt length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return ID_SIZEOF(UInt);
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
