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
 * $Id: mtdChar.cpp 86985 2020-03-23 02:02:28Z donovan.seo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>
#include <mtuProperty.h>
#include <mtlCollate.h>

extern mtdModule mtdChar;
extern mtdModule mtdDouble;


//------------------------------------------------------
// mtdSelectivityChar()를 위한 매크로 시작
//------------------------------------------------------

// numeric type of string
# define MTD_DECIMAL      0x00
# define MTD_HEXA_UPPER   0x01
# define MTD_HEXA_LOWER   0x02
# define MTD_ORDINARY     0x04

// MIN
# define MTD_MIN(a,b) ((a<b)?a:b)


// To Remove Warning
const mtdCharType mtdCharNull = { 0, {'\0',} };

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

static SInt mtdCharLogicalAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 );

static SInt mtdCharLogicalDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 );

static SInt mtdCharFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static SInt mtdCharFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static SInt mtdCharMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdCharMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdCharStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdCharStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdCharStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdCharStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

/* PROJ-2433 */
static SInt mtdCharIndexKeyFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static SInt mtdCharIndexKeyFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static SInt mtdCharIndexKeyMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static SInt mtdCharIndexKeyMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );
/* PROJ-2433 end */

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

static SDouble mtdSelectivityChar( void     * aColumnMax,
                                   void     * aColumnMin,
                                   void     * aValueMax,
                                   void     * aValueMin,
                                   SDouble    aBoundFactor,
                                   SDouble    aTotalRecordCnt );

static vSLong mtdStringType( const mtdCharType * aValue );

static SDouble mtdDigitsToDouble( const mtdCharType * aValue, UInt aBase );

static SDouble mtdConvertToDouble( const mtdCharType * aValue );

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

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"CHAR" }
};

static mtcColumn mtdColumn;

mtdModule mtdChar = {
    mtdTypeName,
    &mtdColumn,
    MTD_CHAR_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_CHAR_ALIGN,
    MTD_GROUP_TEXT|
    MTD_CANON_NEED_WITH_ALLOCATION|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_ENABLE|
    MTD_CREATE_PARAM_PRECISION|
    MTD_CASE_SENS_TRUE|
    MTD_SEARCHABLE_SEARCHABLE|  // BUG-17020
    MTD_LITERAL_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
    MTD_PSM_TYPE_ENABLE, // PROJ-1904
    MTD_CHAR_STORE_PRECISION_MAXIMUM,
    0,
    0,
    (void*)&mtdCharNull,
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
        mtdCharLogicalAscComp,    // Logical Comparison
        mtdCharLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdCharFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdCharFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdCharMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdCharMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdCharStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdCharStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdCharStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdCharStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdCharIndexKeyFixedMtdKeyAscComp,
            mtdCharIndexKeyFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdCharIndexKeyMtdKeyAscComp,
            mtdCharIndexKeyMtdKeyDescComp
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtdSelectivityChar,
    mtd::encodeCharDefault,
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

    // PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdChar, aNo ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdChar,
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
        *aPrecision = MTD_CHAR_PRECISION_DEFAULT;
    }

    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    IDE_TEST_RAISE( *aPrecision < MTD_CHAR_PRECISION_MINIMUM ||
                    *aPrecision > MTD_CHAR_PRECISION_MAXIMUM,
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
    mtdCharType* sValue;
    const UChar* sToken;
    const UChar* sTokenFence;
    UChar*       sIterator;
    UChar*       sFence;

    sValueOffset = idlOS::align( *aValueOffset, MTD_CHAR_ALIGN );

    sValue = (mtdCharType*)( (UChar*)aValue + sValueOffset );

    *aResult = IDE_SUCCESS;

    // To fix BUG-13444
    // tokenFence와 RowFence는 별개의 검사과정이므로,
    // 먼저 RowFence검사 후 TokenFence검사를 해야 한다.
    sIterator = sValue->value;
    sFence    = (UChar*)aValue + aValueSize;
    if( sIterator >= sFence )
    {
        *aResult = IDE_FAILURE;
    }
    else
    {
        for( sToken      = (const UChar*)aToken,
                 sTokenFence = sToken + aTokenLength;
             sToken      < sTokenFence;
             sIterator++, sToken++ )
        {
            if( sIterator >= sFence )
            {
                *aResult = IDE_FAILURE;
                break;
            }
            if( *sToken == '\'' )
            {
                sToken++;
                IDE_TEST_RAISE( sToken >= sTokenFence || *sToken != '\'',
                                ERR_INVALID_LITERAL );
            }
            *sIterator = *sToken;
        }

        // BUG-40290
        // 버퍼가 모잘라서 잘려도 length 를 기록해주면 좋다.
        sValue->length = sIterator - sValue->value;
    }

    if( *aResult == IDE_SUCCESS )
    {
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
        *aValueOffset            = sValueOffset
            + ID_SIZEOF(UShort) + sValue->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn* ,
                    const void*      aRow )
{
    return ID_SIZEOF(UShort) + ((mtdCharType*)aRow)->length;
}

static IDE_RC mtdGetPrecision( const mtcColumn * ,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale )
{
    *aPrecision = ((mtdCharType*)aRow)->length;
    *aScale = 0;

    return IDE_SUCCESS;
}

void mtdSetNull( const mtcColumn* /*aColumn*/,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdCharType*)aRow)->length = 0;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hashWithoutSpace( aHash, ((mtdCharType*)aRow)->value, ((mtdCharType*)aRow)->length );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (((mtdCharType*)aRow)->length == 0) ? ID_TRUE : ID_FALSE;
}

SInt mtdCharLogicalAscComp( mtdValueInfo * aValueInfo1,
                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2    = sCharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharLogicalDescComp( mtdValueInfo * aValueInfo1,
                             mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2    = sCharValue2->length;

    //---------
    // compare
    //---------        

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2    = sCharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2    = sCharValue2->length;

    //---------
    // compare
    //---------        

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdChar.staticNull );

    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdChar.staticNull );

    sLength2    = sCharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdChar.staticNull );

    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdChar.staticNull );

    sLength2    = sCharValue2->length;

    //---------
    // compare
    //---------        

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;
    
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

        sCharValue1 = (const mtdCharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sCharValue1->length;
        sValue1  = sCharValue1->value;
    }

    //---------
    // value2
    //---------    
    sCharValue2 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                        aValueInfo2->value,
                                        aValueInfo2->flag,
                                        mtdChar.staticNull );
    sLength2 = sCharValue2->length;
    sValue2  = sCharValue2->value;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;
    
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

        sCharValue1 = (const mtdCharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sCharValue1->length;
        sValue1  = sCharValue1->value;
    }

    //---------
    // value2
    //---------    
    sCharValue2 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                        aValueInfo2->value,
                                        aValueInfo2->flag,
                                        mtdChar.staticNull );
    sLength2 = sCharValue2->length;
    sValue2  = sCharValue2->value;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;

    //---------
    // value1
    //---------    
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE ) 
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sCharValue1 = (const mtdCharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sCharValue1->length;
        sValue1  = sCharValue1->value;
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

        sCharValue2 = (const mtdCharType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength2 = sCharValue2->length;
        sValue2  = sCharValue2->value;
    }

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

SInt mtdCharStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;

    //---------
    // value1
    //---------    
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sCharValue1 = (const mtdCharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sCharValue1->length;
        sValue1  = sCharValue1->value;
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

        sCharValue2 = (const mtdCharType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength2 = sCharValue2->length;
        sValue2  = sCharValue2->value;
    }

    //---------
    // compare
    //---------        

    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
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

/* PROJ-2433
 * Direct key Index의 direct key와 mtd의 compare 함수
 * - partial direct key를 처리하는부분 추가 */
SInt mtdCharIndexKeyFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2    = sCharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------
    
    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if ( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1,
                                       sValue2,
                                       sLength2 );
            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing to do */
            }

            sExist = ID_FALSE;
            for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1,
                                   sValue2,
                                   sLength1 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        else
        {
            /* nothing to do */
        }
        sExist = ID_FALSE;
        for ( sIterator = ( sValue2 + sLength1 ), sFence = ( sValue2 + sLength2 ) ;
              ( sIterator < sFence ) ;
              sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    else
    {
        /* nothing to do */
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    else
    {
        /* nothing to do */
    }


    return 0;
}

SInt mtdCharIndexKeyFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 )
{
    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2    = sCharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------        

    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if ( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2,
                                       sValue1,
                                       sLength1 );
            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing to do */
            }
            sExist = ID_FALSE;
            for ( sIterator = ( sValue2 + sLength1 ), sFence = ( sValue2 + sLength2 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2,
                                   sValue1,
                                   sLength2 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
              ( sIterator < sFence ) ;
              sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    else
    {
        /* nothing to do */
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    else
    {
        /* nothing to do */
    }
    return 0;
}

SInt mtdCharIndexKeyMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*) mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                                            aValueInfo1->value,
                                                            aValueInfo1->flag,
                                                            mtdChar.staticNull );

    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*) mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                                            aValueInfo2->value,
                                                            aValueInfo2->flag,
                                                            mtdChar.staticNull );

    sLength2    = sCharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------
    
    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if ( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1,
                                       sValue2,
                                       sLength2 );
            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing to do */
            }
            sExist = ID_FALSE;
            for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1,
                                   sValue2,
                                   sLength1 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        sExist = ID_FALSE;
        for ( sIterator = ( sValue2 + sLength1 ), sFence = ( sValue2 + sLength2 ) ;
              ( sIterator < sFence ) ;
              sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    else
    {
        /* nothing to do */
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    else
    {
        /* nothing to do */
    }
    return 0;
}

SInt mtdCharIndexKeyMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;    
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    //---------
    // value1
    //---------
    sCharValue1 = (const mtdCharType*) mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                                            aValueInfo1->value,
                                                            aValueInfo1->flag,
                                                            mtdChar.staticNull );

    sLength1    = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*) mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                                            aValueInfo2->value,
                                                            aValueInfo2->flag,
                                                            mtdChar.staticNull );

    sLength2    = sCharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------        

    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if ( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2,
                                       sValue1,
                                       sLength1 );
            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing to do */
            }
            sExist = ID_FALSE;
            for ( sIterator = ( sValue2 + sLength1 ), sFence = ( sValue2 + sLength2 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2,
                                   sValue1,
                                   sLength2 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        else
        {
            /* nothing to do */
        }
        sExist = ID_FALSE;
        for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
              ( sIterator < sFence ) ;
              sIterator++ )
        {
            if ( *sIterator > 0x20 )
            {
                if ( sExist == ID_FALSE )
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }
            else if ( *sIterator < 0x20 )
            {
                sExist = ID_TRUE;
            }
        }
        return 0;
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    else
    {
        /* nothing to do */
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    else
    {
        /* nothing to do */
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
    mtdCharType* sCanonized;
    mtdCharType* sValue;

    sValue = (mtdCharType*)aValue;

    if( sValue->length == aCanon->precision || sValue->length == 0 )
    {
        *aCanonized = aValue;
    }
    else
    {
        IDE_TEST_RAISE( sValue->length > aCanon->precision,
                        ERR_INVALID_LENGTH );
        
        sCanonized = (mtdCharType*)*aCanonized;
        
        sCanonized->length = aCanon->precision;
        
        idlOS::memcpy( sCanonized->value, sValue->value, sValue->length );
        
        idlOS::memset( sCanonized->value + sValue->length, 0x20,
                       sCanonized->length - sValue->length );
    }

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

    sLength = (UChar*)&(((mtdCharType*)aValue)->length);

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

    mtdCharType * sCharVal = (mtdCharType*)aValue;
    IDE_TEST_RAISE( sCharVal == NULL, ERR_INVALID_NULL );

    IDE_TEST_RAISE( aValueSize < ID_SIZEOF(UShort), ERR_INVALID_LENGTH);
    IDE_TEST_RAISE( sCharVal->length + ID_SIZEOF(UShort) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdChar,
                                     1,                // arguments
                                     sCharVal->length, // precision
                                     0 )               // scale
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


SDouble mtdSelectivityChar( void     * aColumnMax,
                            void     * aColumnMin,
                            void     * aValueMax,
                            void     * aValueMin,
                            SDouble    aBoundFactor,
                            SDouble    aTotalRecordCnt )
{
/*----------------------------------------------------------------------
  Name:
  mtdSelectivityChar()
  -- 최대, 최소 값을 이용하여 범위 값에 대한 선택도를 추정,
  -- CHAR(n),VARCHAR(n)

  Arguments:
  aColumnMax  -- 칼럼의 최대 값 (MAX)
  aColumnMin  -- 칼럼의 최소 값 (MIN)
  aValueMax   -- 범위 최소 값   (Y)
  aValueMin   -- 범위 최대 값   (X)

  Description: 최대, 최소값을 이용하여 범위 값에 대한 선택도를 추정한다.
  <, >, <=, >=, BETWEEN, NOT BETWEEN Predicate가 해당되며,
  LIKE, NOT LIKE의 경우에도 prefix match인 경우 이에 해당한다.

  예: i1 between X and Y
  ==> selectivity = ( Y - X ) / ( MAX - MIN )

  선택도를 계산하는 과정에서 문자열을 DOUBLE형으로 변환해야 한다.
  이 때, 4개의 인자가 모두 10진수로 판단되면 10진수로
  16진수로 판단되면 16진수로 변환한다.
  그렇지 않은 경우 문자열 52비트가 포함하는 값을 변환한다.

  *----------------------------------------------------------------------*/

    // 각각의 값에 대한 mtdCharType
    const mtdCharType *    sColumnMax;
    const mtdCharType *    sColumnMin;
    const mtdCharType *    sValueMax;
    const mtdCharType *    sValueMin;

    // 각각의 값에 대한 SDouble 변수
    SDouble          sColMaxDouble;
    SDouble          sColMinDouble;
    SDouble          sValMaxDouble;
    SDouble          sValMinDouble;

    // Selectivity
    SDouble          sSelectivity;

    // 10진수, 16진수, 일반문자열 여부를 표시하는 변수
    vSLong           sStringType;

    // 변수 초기화
    sStringType = 0;
    sColumnMax = (mtdCharType*) aColumnMax;
    sColumnMin = (mtdCharType*) aColumnMin;
    sValueMax  = (mtdCharType*) aValueMax;
    sValueMin  = (mtdCharType*) aValueMin;

    //------------------------------------------------------
    // Data의 유효성 검사
    //     NULL 검사 : 계산할 수 없음
    //------------------------------------------------------
    
    // BUG-22064
    IDE_DASSERT( aColumnMax == idlOS::align( aColumnMax, MTD_CHAR_ALIGN ) );
    IDE_DASSERT( aColumnMin == idlOS::align( aColumnMin, MTD_CHAR_ALIGN ) );
    IDE_DASSERT( aValueMax == idlOS::align( aValueMax, MTD_CHAR_ALIGN ) );
    IDE_DASSERT( aValueMin == idlOS::align( aValueMin, MTD_CHAR_ALIGN ) );
    
    if ( ( mtdIsNull( NULL, aColumnMax ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aColumnMin ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aValueMax  ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aValueMin  ) == ID_TRUE ) )
    {
        // Data중 NULL 이 있을 경우
        // 부등호의 Default Selectivity인 1/3을 Setting함
        sSelectivity = MTD_DEFAULT_SELECTIVITY;
    }
    else
    {
        //------------------------------------------------------------
        // 숫자를 의미하는 문자열인지 판단
        //  sStringType에 적절한 플래그 설정
        //------------------------------------------------------------
        sStringType |= mtdStringType(sColumnMax);
        sStringType |= mtdStringType(sColumnMin);
        sStringType |= mtdStringType(sValueMax);
        sStringType |= mtdStringType(sValueMin);

        //---------------------------------------------------
        // 10진수의 판단
        //   어떠한 플래그로 설정되어 있지 않아야 한다.
        // 10진수의 변환
        //   mtdDigitsToDouble함수를 이용하여 10진수로 변환한다.
        //---------------------------------------------------
        if( sStringType == MTD_DECIMAL )
        {
            sColMaxDouble = mtdDigitsToDouble( sColumnMax, 10 );
            sColMinDouble = mtdDigitsToDouble( sColumnMin, 10 );
            sValMaxDouble = mtdDigitsToDouble( sValueMax, 10 );
            sValMinDouble = mtdDigitsToDouble( sValueMin, 10 );
        }
        //---------------------------------------------------
        // 16진수의 판단
        //   MTD_HEXA_LOWER 이거나 MTD_HEXA_UPPER 인 경우
        //   둘 중의 한 플래그 값과 일치해야하며,
        //   두 플래그가 함께 설정된 경우는 일반 문자열로 판단
        // 16진수의 변환
        //   mtdDigitsToDouble함수를 이용하여 16진수로 변환한다.
        //---------------------------------------------------
        else if( ( sStringType == MTD_HEXA_LOWER ) ||
                 ( sStringType == MTD_HEXA_UPPER ) )
        {
            sColMaxDouble = mtdDigitsToDouble( sColumnMax, 16 );
            sColMinDouble = mtdDigitsToDouble( sColumnMin, 16 );
            sValMaxDouble = mtdDigitsToDouble( sValueMax, 16 );
            sValMinDouble = mtdDigitsToDouble( sValueMin, 16 );
        }
        //------------------------------------------------------------
        // 일반 문자열인 경우 변환
        //------------------------------------------------------------
        else
        {
            sColMaxDouble = mtdConvertToDouble( sColumnMax );
            sColMinDouble = mtdConvertToDouble( sColumnMin );
            sValMaxDouble = mtdConvertToDouble( sValueMax );
            sValMinDouble = mtdConvertToDouble( sValueMin );
        }

        //---------------------------------------------------------
        // selectivity 계산
        //--------------------------------------------------------
        sSelectivity = mtdDouble.selectivity( (void *)&sColMaxDouble,
                                              (void *)&sColMinDouble,
                                              (void *)&sValMaxDouble,
                                              (void *)&sValMinDouble,
                                              aBoundFactor,
                                              aTotalRecordCnt );
    }

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return sSelectivity;

}


vSLong mtdStringType( const mtdCharType * aValue )
{
/*----------------------------------------------------------------------
  Name:
  mtdStringType()
  -- 해당 문자열의 타입을 판단한다.
  10진수     : MTD_DECIMAL
  16진수     : MTD_HEXA_LOWER, MTD_HEXA_UPPER
  일반 문자열 : MTD_ORDINARY

  Arguments:
  aValue  -- 타입을 판단할 문자열

  *----------------------------------------------------------------------*/
    vSLong sLength;
    vSLong sIdx;
    vSLong sStringType;

    sStringType = 0;
    sLength     = (vSLong)(aValue->length);

    for( sIdx = 0; sIdx < sLength; sIdx++ )
    {
        if( (aValue->value[sIdx] >= '0') &&
            (aValue->value[sIdx] <= '9') )
        {
            /* nothing to do...*/
        }
        else if ( (aValue->value[sIdx] >= 'a') &&
                  (aValue->value[sIdx] <= 'f')  )
        {
            sStringType |= MTD_HEXA_LOWER;
        }
        else if( (aValue->value[sIdx] >= 'A') &&
                 (aValue->value[sIdx] <= 'F') )
        {
            sStringType |= MTD_HEXA_UPPER;
        }
        else
        {
            sStringType |= MTD_ORDINARY;
            break;
        }
    }
    return sStringType;

}

SDouble mtdDigitsToDouble( const mtdCharType * aValue, UInt aBase )
{
/*----------------------------------------------------------------------
  Name:
  BUG-16401
  mtdDigitsToDouble()
  -- 해당 문자열을 15자리 문자열로 고정후
  -- SDbouble타입으로 변환한다.

  Arguments:
  aValue  -- 변환할 문자열
  aBase   -- 진법

  *----------------------------------------------------------------------*/

    SDouble  sDoubleVal;
    ULong    sLongVal;
    UInt     sDigit;
    UInt     i;

    static const UChar sHex[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
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

    sLongVal = 0;

    if ( (aValue->length > 0) && (aBase >= 2) && (aBase <= 16) )
    {
        if ( aValue->length < MTD_CHAR_DIGIT_MAX )
        {
            for ( i = 0; i < aValue->length; i++ )
            {
                sDigit = sHex[ aValue->value[i] ];

                if ( sDigit >= aBase )
                {
                    sLongVal = 0;
                    break;
                }
                else
                {
                    sLongVal = sLongVal * aBase + sDigit;
                }
            }

            if ( sLongVal != 0 )
            {
                for ( ; i < MTD_CHAR_DIGIT_MAX; i++ )
                {
                    sLongVal = sLongVal * aBase;
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            for ( i = 0; i < MTD_CHAR_DIGIT_MAX; i++ )
            {
                sDigit = sHex[ aValue->value[i] ];

                if ( sDigit >= aBase )
                {
                    sLongVal = 0;
                    break;
                }
                else
                {
                    sLongVal = sLongVal * aBase + sDigit;
                }
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    sDoubleVal = ID_ULTODB( sLongVal );

    return sDoubleVal;
}

SDouble mtdConvertToDouble( const mtdCharType * aValue )
{
/*----------------------------------------------------------------------
  Name:
  mtdConvertToDouble()
  -- 해당 문자열을 SDbouble타입으로 변환한다.

  Arguments:
  aValue  -- 변환할 문자열

  *----------------------------------------------------------------------*/

#if defined(ENDIAN_IS_BIG_ENDIAN)
    //----------------------------------------------------
    // Big endian 인 경우
    //----------------------------------------------------
    SDouble    sDoubleVal;       // 변환될 Double 값
    ULong      sLongVal;         // ULong으로 변환 시 사용할 변수

    sLongVal = 0;                // 초기화

    // mtdCharType을 ULong으로 변환
    idlOS::memcpy( (UChar*)&sLongVal,
                   aValue->value,
                   MTD_MIN( aValue->length, ID_SIZEOF(sLongVal) ) );

    // 앞부분 52비트만 더블로 변환
    sLongVal   = sLongVal >> 12;
    sDoubleVal = ID_ULTODB( sLongVal );

    return sDoubleVal;

#else
    //----------------------------------------------------
    // Little endian 인 경우
    //----------------------------------------------------
    SDouble    sDoubleVal;       // 변환될 Double 값
    ULong      sLongVal;         // ULong으로 변환 시 사용할 변수
    ULong      sEndian;          // byte ordering 변환시 사용할 임시 변수
    UChar    * sSrc;             // byte ordering 변환을 위한 포인터
    UChar    * sDest;            // byte ordering 변환을 위한 포인터

    sLongVal = 0;                // 초기화

    // mtdCharType을 ULong으로 변환
    idlOS::memcpy( (UChar*)&sLongVal,
                   aValue->value,
                   MTD_MIN( aValue->length, ID_SIZEOF(sLongVal) ) );

    // byte ordering 조정
    sSrc     = (UChar*)&sLongVal;
    sDest    = (UChar*)&sEndian;
    sDest[0] = sSrc[7];
    sDest[1] = sSrc[6];
    sDest[2] = sSrc[5];
    sDest[3] = sSrc[4];
    sDest[4] = sSrc[3];
    sDest[5] = sSrc[2];
    sDest[6] = sSrc[1];
    sDest[7] = sSrc[0];

    // 앞부분 52비트만 더블로 변환
    sEndian    = sEndian >> 12;
    sDoubleVal = ID_ULTODB( sEndian );

    return sDoubleVal;

#endif

}



IDE_RC mtdValueFromOracle( mtcColumn*  aColumn,
                           void*       aValue,
                           UInt*       aValueOffset,
                           UInt        aValueSize,
                           const void* aOracleValue,
                           SInt        aOracleLength,
                           IDE_RC*     aResult )
{
    UInt         sValueOffset;
    mtdCharType* sValue;

    sValueOffset = idlOS::align( *aValueOffset, MTD_CHAR_ALIGN );

    if( aOracleLength < 0 )
    {
        aOracleLength = 0;
    }

    // aColumn의 초기화
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdChar,
                                     1,
                                     ( aOracleLength > 1 ) ? aOracleLength : 1,
                                     0 )
              != IDE_SUCCESS );


    if( sValueOffset + aColumn->column.size <= aValueSize )
    {
        sValue = (mtdCharType*)((UChar*)aValue+sValueOffset);

        sValue->length = aOracleLength;

        idlOS::memcpy( sValue->value, aOracleValue, aOracleLength );

        aColumn->column.offset = sValueOffset;

        *aValueOffset = sValueOffset + aColumn->column.size;

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

    mtdCharType* sCharValue;

    sCharValue = (mtdCharType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sCharValue->length = 0;
    }
    else
    {
        IDE_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sCharValue->length = (UShort)(aDestValueOffset + aLength);
        idlOS::memcpy( (UChar*)sCharValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
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
 * 예 ) mtdCharType( UShort length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *******************************************************************/
    return mtdActualSize( NULL, &mtdCharNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdCharType( UShort length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return ID_SIZEOF(UShort);
}

static UInt mtdStoreSize( const smiColumn * aColumn ) 
{
/***********************************************************************
 * PROJ-2399 row tmaplate 
 * sm에 저장되는 데이터의 크기를 반환한다.
 * variable 타입의 데이터 타입은 ID_UINT_MAX를 반환
 * mtheader가 sm에 저장된경우가 아니면 mtheader크기를 빼서 반환
 **********************************************************************/

    return aColumn->size - mtdHeaderSize();
}
