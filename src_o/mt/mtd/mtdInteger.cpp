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
 * $Id: mtdInteger.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#define MTD_INTEGER_ALIGN (ID_SIZEOF(mtdIntegerType))
#define MTD_INTEGER_SIZE  (ID_SIZEOF(mtdIntegerType))

extern mtdModule mtdInteger;
extern mtdModule mtdDouble;

static mtdIntegerType mtdIntegerNull = MTD_INTEGER_NULL;

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


static SInt mtdIntegerLogicalAscComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerLogicalDescComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                              mtdValueInfo * aValueInfo2 );

static SInt mtdIntegerStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
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

static mtcName mtdTypeName[2] = {
    { mtdTypeName+1, 7, (void*)"INTEGER" },
    { NULL, 3, (void*)"INT" } // To fix BUG-17649
};

static mtcColumn mtdColumn;

mtdModule mtdInteger = {
    mtdTypeName,
    &mtdColumn,
    MTD_INTEGER_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_INTEGER_ALIGN,
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
    10,
    0,
    0,
    &mtdIntegerNull,
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
        mtdIntegerLogicalAscComp,    // Logical Comparison
        mtdIntegerLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdIntegerFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdIntegerFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdIntegerMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdIntegerMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdIntegerStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdIntegerStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdIntegerStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdIntegerStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdIntegerFixedMtdFixedMtdKeyAscComp,
            mtdIntegerFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdIntegerMtdMtdKeyAscComp,
            mtdIntegerMtdMtdKeyDescComp
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
    IDE_TEST( mtd::initializeModule( &mtdInteger, aNo ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdInteger,
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

    *aColumnSize = MTD_INTEGER_SIZE;
    
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
    UInt            sValueOffset;
    mtdIntegerType* sValue;
    const UChar*    sToken;
    const UChar*    sFence;
    SLong           sIntermediate;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_INTEGER_ALIGN );
    
    sToken = (const UChar*)aToken;
    
    if( sValueOffset + MTD_INTEGER_SIZE <= aValueSize )
    {
        sValue = (mtdIntegerType*)( (UChar*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            *sValue = MTD_INTEGER_NULL;
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
                    IDE_TEST_RAISE( sIntermediate < MTD_INTEGER_MINIMUM,
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
                    IDE_TEST_RAISE( sIntermediate > MTD_INTEGER_MAXIMUM,
                                    ERR_VALUE_OVERFLOW );
                }
                *sValue = sIntermediate;
            }
        }

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_INTEGER_SIZE;
        
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
    return (UInt)MTD_INTEGER_SIZE;
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        *(mtdIntegerType*)aRow = MTD_INTEGER_NULL;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hash( aHash, (const UChar*)aRow, MTD_INTEGER_SIZE );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (*(mtdIntegerType*)aRow == MTD_INTEGER_NULL) ? ID_TRUE : ID_FALSE ;
}

SInt mtdIntegerLogicalAscComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntegerType * sValue1;
    const mtdIntegerType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdIntegerType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdIntegerType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );

    //---------
    // compare
    //---------
    if( *sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerLogicalDescComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2  )
{
    /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntegerType * sValue1;
    const mtdIntegerType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdIntegerType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdIntegerType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );

    //---------
    // compare
    //---------    
    if( *sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntegerType * sValue1;
    const mtdIntegerType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdIntegerType*)MTD_VALUE_FIXED( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdIntegerType*)MTD_VALUE_FIXED( aValueInfo2 );

    //---------
    // compare
    //---------
    if( *sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2  )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntegerType * sValue1;
    const mtdIntegerType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdIntegerType*)MTD_VALUE_FIXED( aValueInfo1 );

    //---------
    // value2
    //---------
    sValue2 = (const mtdIntegerType*)MTD_VALUE_FIXED( aValueInfo2 );

    //---------
    // compare
    //---------    
    if( *sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntegerType * sValue1;
    const mtdIntegerType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdIntegerType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdInteger.staticNull );

    //---------
    // value2
    //---------    
    sValue2 = (const mtdIntegerType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdInteger.staticNull );

    //---------
    // compare
    //---------
    if( *sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2  )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntegerType * sValue1;
    const mtdIntegerType * sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdIntegerType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdInteger.staticNull );

    //---------
    // value2
    //---------    
    sValue2 = (const mtdIntegerType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdInteger.staticNull );

    //---------
    // compare
    //---------    
    if( *sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdIntegerType         sValue1;
    const mtdIntegerType * sValue2;
    
    //---------
    // value1
    //---------    
    ID_INT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );

    //---------
    // value2
    //---------    
    sValue2 = (const mtdIntegerType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdInteger.staticNull );

    //---------
    // compare
    //---------    
    if( sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2  )
{
    /***********************************************************************
     *
     * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
     *
 * Implementation :
 *
 ***********************************************************************/

    mtdIntegerType         sValue1;
    const mtdIntegerType * sValue2;

    //---------
    // value1
    //---------    
    ID_INT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );


    //---------
    // value2
    //---------    
    sValue2 = (const mtdIntegerType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdInteger.staticNull );


    //---------
    // compare
    //---------
    
    if( sValue1 != MTD_INTEGER_NULL && *sValue2 != MTD_INTEGER_NULL )
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

SInt mtdIntegerStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdIntegerType   sValue1;
    mtdIntegerType   sValue2;

    //---------
    // value1
    //---------    
    ID_INT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );

    //---------
    // value2
    //---------    
    ID_INT_BYTE_ASSIGN( &sValue2, aValueInfo2->value );

    //---------
    // compare
    //---------
    
    if( ( sValue1 != MTD_INTEGER_NULL ) && ( sValue2 != MTD_INTEGER_NULL ) )
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

SInt mtdIntegerStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2  )
{
    /***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdIntegerType   sValue1;
    mtdIntegerType   sValue2;

    //---------
    // value1
    //---------    
    ID_INT_BYTE_ASSIGN( &sValue1, aValueInfo1->value );

    //---------
    // value2
    //---------    
    ID_INT_BYTE_ASSIGN( &sValue2, aValueInfo2->value );

    //---------
    // compare
    //---------
    
    if( sValue1 != MTD_INTEGER_NULL && sValue2 != MTD_INTEGER_NULL )
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
    sValue[0]     = sValue[3];
    sValue[3]     = sIntermediate;
    sIntermediate = sValue[1];
    sValue[1]     = sValue[2];
    sValue[2]     = sIntermediate;
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
    
    IDE_TEST_RAISE( aValueSize != ID_SIZEOF(mtdIntegerType),
                    ERR_INVALID_LENGTH );

    // IDE_TEST( mtdEstimate( aColumn, 0, 0, 0 ) != IDE_SUCCESS);
    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdInteger,
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
                                          *(mtdIntegerType*)aValue);
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
                                       UInt           /* aDestValueOffset */,
                                       UInt              aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdIntegerType* sIntegerValue;

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sIntegerValue = (mtdIntegerType*)aDestValue;
        
    if( aLength == 0 )
    {
        // NULL 데이타
        *sIntegerValue = mtdIntegerNull;        
    }
    else
    {
        IDE_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );
        IDE_TEST_RAISE( aLength != ID_SIZEOF( mtdIntegerType ), ERR_INVALID_STORED_VALUE );

        ID_INT_BYTE_ASSIGN( sIntegerValue, aValue );
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
    return mtdActualSize( NULL, &mtdIntegerNull );
}

