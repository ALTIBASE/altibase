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
 * $Id: mtdSmallint.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#define MTD_SMALLINT_ALIGN (ID_SIZEOF(mtdSmallintType))
#define MTD_SMALLINT_SIZE  (ID_SIZEOF(mtdSmallintType))

extern mtdModule mtdSmallint;
extern mtdModule mtdDouble;

static const mtdSmallintType mtdSmallintNull = MTD_SMALLINT_NULL;

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

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static UInt mtdHash( UInt             aHash,
                     const mtcColumn* aColumn,
                     const void*      aRow );

static idBool mtdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static SInt mtdSmallintLogicalAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintLogicalDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                    mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static SInt mtdSmallintStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static IDE_RC mtdEncode( mtcColumn  * aColumn,
                         void       * aValue,
                         UInt         aValueSize,
                         UChar      * aCompileFmt,
                         UInt         aCompileFmtLen,
                         UChar      * aText,
                         UInt       * aTextLen,
                         IDE_RC     * aRet );

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdNullValueSize();

static mtcName mtdTypeName[1] = {
    { NULL, 8, (void*)"SMALLINT" },
};

static mtcColumn mtdColumn;

mtdModule mtdSmallint = {
    mtdTypeName,
    &mtdColumn,
    MTD_SMALLINT_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_SMALLINT_ALIGN,
    MTD_GROUP_NUMBER|
      MTD_CANON_NEEDLESS|
      MTD_CREATE_ENABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_ENABLE|
      MTD_NUMBER_GROUP_TYPE_BIGINT|
      MTD_SEARCHABLE_PRED_BASIC|
      MTD_UNSIGNED_ATTR_TRUE|
      MTD_NUM_PREC_RADIX_TRUE|
      MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    5,
    0,
    0,
    (void*)&mtdSmallintNull,
    mtdInitialize,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtd::getPrecisionNA,
    mtdSetNull,
    mtdHash,
    mtdIsNull,
    mtd::isTrueNA,
    {
        mtdSmallintLogicalAscComp,  // Logical Comparison
        mtdSmallintLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdSmallintFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdSmallintFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdSmallintMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdSmallintMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdSmallintStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdSmallintStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdSmallintStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdSmallintStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdSmallintFixedMtdFixedMtdKeyAscComp,
            mtdSmallintFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdSmallintMtdMtdKeyAscComp,
            mtdSmallintMtdMtdKeyDescComp
        }
    },
    mtd::canonizeDefault,
    mtdEndian,
    mtdValidate,
    mtdDouble.selectivity,
    mtdEncode,
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
        NULL 
    }, 
    mtdNullValueSize,
    mtd::mtdHeaderSizeDefault,

    // PROJ-2399
    mtd::mtdStoreSizeDefault
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdSmallint, aNo ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdSmallint,
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
                    SInt * /*aPrecision*/,
                    SInt * /*aScale*/ )
{
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_PRECISION );
    
    *aColumnSize = MTD_SMALLINT_SIZE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));
    
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
    UInt             sValueOffset;
    mtdSmallintType* sValue;
    const UChar*     sToken;
    const UChar*     sFence;
    SInt             sIntermediate;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_SMALLINT_ALIGN );
    
    sToken = (const UChar*)aToken;
    
    if( sValueOffset + MTD_SMALLINT_SIZE <= aValueSize )
    {
        sValue = (mtdSmallintType*)( (UChar*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            *sValue = MTD_SMALLINT_NULL;
        }
        else
        {
            sFence = sToken + aTokenLength;
            if( *sToken == '-' )
            {
                sToken++;
                IDE_TEST_RAISE( sToken >= sFence, ERR_INVALID_LITERAL );
                for( sIntermediate = 0; sToken < sFence; sToken++ )
                {
                    IDE_TEST_RAISE( *sToken < '0' || *sToken > '9',
                                    ERR_INVALID_LITERAL );
                    sIntermediate = sIntermediate * 10 - ( *sToken - '0' );
                    IDE_TEST_RAISE( sIntermediate < MTD_SMALLINT_MINIMUM,
                                    ERR_VALUE_OVERFLOW );
                }
                *sValue = sIntermediate;
            }
            else
            {
                if( *sToken == '+' )
                {
                    sToken++;
                }
                IDE_TEST_RAISE( sToken >= sFence, ERR_INVALID_LITERAL );
                for( sIntermediate = 0; sToken < sFence; sToken++ )
                {
                    IDE_TEST_RAISE( *sToken < '0' || *sToken > '9',
                                    ERR_INVALID_LITERAL );
                    sIntermediate = sIntermediate * 10 + ( *sToken - '0' );
                    IDE_TEST_RAISE( sIntermediate > MTD_SMALLINT_MAXIMUM,
                                    ERR_VALUE_OVERFLOW );
                }
                *sValue = sIntermediate;
            }
        }

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_SMALLINT_SIZE;
        
        *aResult = IDE_SUCCESS;
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
    return (UInt)MTD_SMALLINT_SIZE;
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        *(mtdSmallintType*)aRow = MTD_SMALLINT_NULL;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hash( aHash, (const UChar*)aRow, MTD_SMALLINT_SIZE );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (*(mtdSmallintType*)aRow == MTD_SMALLINT_NULL) ? ID_TRUE : ID_FALSE ;
}

SInt mtdSmallintLogicalAscComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdSmallintType * sValue1;
    const mtdSmallintType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdSmallintType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdSmallintType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );

    //---------
    // compare
    //---------
    
    if( ( *sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( *sValue1 > *sValue2 )
        {
            return 1;
        }
        if( *sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintLogicalDescComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdSmallintType * sValue1;
    const mtdSmallintType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdSmallintType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdSmallintType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );

    //---------
    // compare
    //---------

    if( ( *sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( *sValue1 < *sValue2 )
        {
            return 1;
        }
        if( *sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdSmallintType * sValue1;
    const mtdSmallintType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdSmallintType*)MTD_VALUE_FIXED( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdSmallintType*)MTD_VALUE_FIXED( aValueInfo2 );

    //---------
    // compare
    //---------
    
    if( ( *sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( *sValue1 > *sValue2 )
        {
            return 1;
        }
        if( *sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdSmallintType * sValue1;
    const mtdSmallintType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdSmallintType*)MTD_VALUE_FIXED( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdSmallintType*)MTD_VALUE_FIXED( aValueInfo2 );

    //---------
    // compare
    //---------

    if( ( *sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( *sValue1 < *sValue2 )
        {
            return 1;
        }
        if( *sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdSmallintType * sValue1;
    const mtdSmallintType * sValue2;

    //---------
    // value1
    //---------        
    sValue1 = (const mtdSmallintType*)
                mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                     aValueInfo1->value,
                                     aValueInfo1->flag,
                                     mtdSmallint.staticNull );

    //---------
    // value2
    //---------        
    sValue2 = (const mtdSmallintType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdSmallint.staticNull );

    //---------
    // compare
    //---------
    
    if( ( *sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( *sValue1 > *sValue2 )
        {
            return 1;
        }
        if( *sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdSmallintType * sValue1;
    const mtdSmallintType * sValue2;

    //---------
    // value1
    //---------        
    sValue1 = (const mtdSmallintType*)
                mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                     aValueInfo1->value,
                                     aValueInfo1->flag,
                                     mtdSmallint.staticNull );

    //---------
    // value2
    //---------        
    sValue2 = (const mtdSmallintType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdSmallint.staticNull );

    //---------
    // compare
    //---------

    if( ( *sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( *sValue1 < *sValue2 )
        {
            return 1;
        }
        if( *sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdSmallintType         sValue1;
    const mtdSmallintType * sValue2;
    
    //---------
    // value1
    //---------        
    ID_SHORT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );

    //---------
    // value2
    //---------        
    sValue2 = (const mtdSmallintType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdSmallint.staticNull );

    //---------
    // compare
    //---------

    if( ( sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( sValue1 > *sValue2 )
        {
            return 1;
        }
        if( sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < *sValue2 )
    {
        return 1;
    }
    if( sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdSmallintType         sValue1;
    const mtdSmallintType * sValue2;

    //---------
    // value1
    //---------        
    ID_SHORT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );


    //---------
    // value2
    //---------        
    sValue2 = (const mtdSmallintType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdSmallint.staticNull );
    
    //---------
    // compare
    //---------

    if( ( sValue1 != MTD_SMALLINT_NULL ) &&
        ( *sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( sValue1 < *sValue2 )
        {
            return 1;
        }
        if( sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < *sValue2 )
    {
        return 1;
    }
    if( sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}


SInt mtdSmallintStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdSmallintType   sValue1;
    mtdSmallintType   sValue2;

    //---------
    // value1
    //---------        
    ID_SHORT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );

    //---------
    // value2
    //---------        
    ID_SHORT_BYTE_ASSIGN( &sValue2, aValueInfo2->value );

    //---------
    // compare
    //---------

    if( ( sValue1 != MTD_SMALLINT_NULL ) &&
        ( sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( sValue1 > sValue2 )
        {
            return 1;
        }
        if( sValue1 < sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < sValue2 )
    {
        return 1;
    }
    if( sValue1 > sValue2 )
    {
        return -1;
    }
    return 0;
}

SInt mtdSmallintStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdSmallintType   sValue1;
    mtdSmallintType   sValue2;

    //---------
    // value1
    //---------        
    ID_SHORT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );

    //---------
    // value2
    //---------        
    ID_SHORT_BYTE_ASSIGN( &sValue2, aValueInfo2->value );

    //---------
    // compare
    //---------

    if( ( sValue1 != MTD_SMALLINT_NULL ) &&
        ( sValue2 != MTD_SMALLINT_NULL ) )
    {
        if( sValue1 < sValue2 )
        {
            return 1;
        }
        if( sValue1 > sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < sValue2 )
    {
        return 1;
    }
    if( sValue1 > sValue2 )
    {
        return -1;
    }
    return 0;
}

void mtdEndian( void* aValue )
{
    UChar* sValue;
    UChar  sIntermediate;
    
    sValue        = (UChar*)aValue;
    sIntermediate = sValue[0];
    sValue[0]     = sValue[1];
    sValue[1]     = sIntermediate;
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
        
    IDE_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );
    
    IDE_TEST_RAISE( aValueSize != ID_SIZEOF(mtdSmallintType),
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdSmallint,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
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


IDE_RC mtdEncode( mtcColumn  * /* aColumn */,
                  void       * aValue,
                  UInt         /* aValueSize */,
                  UChar      * /* aCompileFmt */,
                  UInt         /* aCompileFmtLen */,
                  UChar      * aText,
                  UInt       * aTextLen,
                  IDE_RC     * aRet )
{
    UInt sStringLen;

    //----------------------------------
    // Parameter Validation
    //----------------------------------

    IDE_ASSERT( aValue != NULL );
    IDE_ASSERT( aText != NULL );
    IDE_ASSERT( *aTextLen > 0 );
    IDE_ASSERT( aRet != NULL );
    
    //----------------------------------
    // Initialization
    //----------------------------------

    aText[0] = '\0';
    sStringLen = 0;

    //----------------------------------
    // Set String
    //----------------------------------
    
    // To Fix BUG-16801
    if ( mtdIsNull( NULL, aValue ) == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sStringLen = idlVA::appendFormat( (SChar*) aText, 
                                          *aTextLen,
                                          "%"ID_INT32_FMT,
                                          *(mtdSmallintType*) aValue );
    }

    //----------------------------------
    // Finalization
    //----------------------------------
    
    aText[sStringLen] = '\0';
    *aTextLen = sStringLen;

    *aRet = IDE_SUCCESS;

    return IDE_SUCCESS;
}

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt          /*  aDestValueOffset */,
                                       UInt              aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdSmallintType  * sSmallintValue;

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sSmallintValue = (mtdSmallintType*)aDestValue;
    
    if( aLength == 0 )
    {
        // NULL 데이타
        *sSmallintValue = mtdSmallintNull;
    }
    else
    {
        IDE_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );
        IDE_TEST_RAISE( aLength != ID_SIZEOF( mtdSmallintType ), ERR_INVALID_STORED_VALUE ); 

        ID_SHORT_BYTE_ASSIGN( sSmallintValue, aValue );
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
 *******************************************************************/
    return mtdActualSize( NULL, &mtdSmallintNull );
}

