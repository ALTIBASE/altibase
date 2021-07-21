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
 * $Id: mtdReal.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#include <math.h>

#define MTD_REAL_ALIGN   (ID_SIZEOF(mtdRealType))
#define MTD_REAL_SIZE    (ID_SIZEOF(mtdRealType))

extern mtdModule mtdReal;
extern mtdModule mtdDouble;

/* PROJ-2632 */
const UInt mtdRealNull = ( MTD_REAL_EXPONENT_MASK|
                           MTD_REAL_FRACTION_MASK );

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

static SInt mtdRealLogicalAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 );

static SInt mtdRealLogicalDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 );

static SInt mtdRealFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static SInt mtdRealFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static SInt mtdRealMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdRealMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdRealStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdRealStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdRealStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdRealStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
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
                                       void            * aDestVale,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdNullValueSize();

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"REAL" },
};

static mtcColumn mtdColumn;

mtdModule mtdReal = {
    mtdTypeName,
    &mtdColumn,
    MTD_REAL_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_REAL_ALIGN,
    MTD_GROUP_NUMBER|
      MTD_CANON_NEEDLESS|
      MTD_CREATE_ENABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_ENABLE|
      MTD_NUMBER_GROUP_TYPE_DOUBLE|
      MTD_SEARCHABLE_PRED_BASIC|
      MTD_UNSIGNED_ATTR_TRUE|
      MTD_NUM_PREC_RADIX_TRUE|
      MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    7,
    0,
    0,
    (void*)&mtdRealNull,
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
        mtdRealLogicalAscComp,    // Logical Comparison
        mtdRealLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdRealFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdRealFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdRealMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdRealMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdRealStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdRealStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdRealStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdRealStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdRealFixedMtdFixedMtdKeyAscComp,
            mtdRealFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdRealMtdMtdKeyAscComp,
            mtdRealMtdMtdKeyDescComp
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
    IDE_TEST_RAISE( ID_SIZEOF(SFloat) != 4, ERR_INCOMPATIBLE_TYPE );
    
    IDE_TEST( mtd::initializeModule( &mtdReal, aNo ) != IDE_SUCCESS );    

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdReal,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INCOMPATIBLE_TYPE );
    IDE_SET(ideSetErrorCode(mtERR_FATAL_INCOMPATIBLE_TYPE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * aArguments,
                    SInt * /*aPrecision*/,
                    SInt * /*aScale*/ )
{
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_PRECISION );

     *aColumnSize = MTD_REAL_SIZE;
    
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
    UInt         sValueOffset;
    mtdRealType* sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_REAL_ALIGN );
    
    if( sValueOffset + MTD_REAL_SIZE <= aValueSize )
    {
        sValue = (mtdRealType*)( (UChar*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            mtdReal.null( mtdReal.column, sValue );
        }
        else
        {
            char  sBuffer[1024];
            IDE_TEST_RAISE( aTokenLength >= ID_SIZEOF(sBuffer),
                            ERR_INVALID_LITERAL );
            idlOS::memcpy( sBuffer, aToken, aTokenLength );
            sBuffer[aTokenLength] = 0;

            errno = 0;
            
            *sValue = idlOS::strtod( sBuffer, (char**)NULL );

            /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            if( *sValue == 0 )
            {
                *sValue = 0;
            }
            /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            IDE_TEST_RAISE( mtdIsNull( mtdReal.column,
                                       sValue ) == ID_TRUE,
                            ERR_VALUE_OVERFLOW );

            // To fix BUG-12281
            // underflow 검사
            if( ( idlOS::fabs(*sValue) < MTD_REAL_MINIMUM ) &&
                ( *sValue != 0 ) )
            {
                *sValue = 0;
            }
            else
            {
                // Nothing to do
            }
        }
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_REAL_SIZE;
        
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
    return (UInt)MTD_REAL_SIZE;
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    static UInt sNull = ( MTD_REAL_EXPONENT_MASK | MTD_REAL_FRACTION_MASK );

    if( aRow != NULL )
    {
        *((UInt*)aRow) = sNull;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* aColumn,
              const void*      aRow )
{
    if( mtdReal.isNull( aColumn, aRow ) != ID_TRUE )
    {
        aHash = mtc::hash( aHash, (const UChar*)aRow, MTD_REAL_SIZE );
    }

    return aHash;
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (( *(UInt*)aRow & MTD_REAL_EXPONENT_MASK ) == MTD_REAL_EXPONENT_MASK) ? ID_TRUE : ID_FALSE ;
}

SInt mtdRealLogicalAscComp( mtdValueInfo * aValueInfo1,
                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdRealType  * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdRealType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sNull1  = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdRealType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sNull2  = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealLogicalDescComp( mtdValueInfo * aValueInfo1,
                             mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdRealType  * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdRealType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sNull1  = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdRealType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sNull2  = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------    

    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdRealType  * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdRealType*)MTD_VALUE_FIXED( aValueInfo1 );
    sNull1  = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdRealType*)MTD_VALUE_FIXED( aValueInfo2 );
    sNull2  = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdRealType  * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdRealType*)MTD_VALUE_FIXED( aValueInfo1 );
    sNull1  = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdRealType*)MTD_VALUE_FIXED( aValueInfo2 );
    sNull2  = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------    

    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdRealType  * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;

    //---------
    // value1
    //---------    
    sValue1 = (const mtdRealType*)
                mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                     aValueInfo1->value,
                                     aValueInfo1->flag,
                                     mtdReal.staticNull );

    sNull1 = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------    
    sValue2 = (const mtdRealType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdReal.staticNull );

    sNull2 = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdRealType  * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;

    //---------
    // value1
    //---------    
    sValue1 = (const mtdRealType*)
                mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                     aValueInfo1->value,
                                     aValueInfo1->flag,
                                     mtdReal.staticNull );

    sNull1 = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------    
    sValue2 = (const mtdRealType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdReal.staticNull );
    
    sNull2 = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------    

    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdRealType          sRealValue1;
    mtdRealType        * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;
    
    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    ID_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );
    
    sNull1 = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;
    
    //---------
    // value1
    //---------
    sValue2 = (const mtdRealType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdReal.staticNull );

    sNull2 = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------    
    
    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdRealType          sRealValue1;
    mtdRealType        * sValue1;
    const mtdRealType  * sValue2;
    idBool               sNull1;
    idBool               sNull2;
    
    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    ID_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value1
    //---------
    sValue2 = (const mtdRealType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdReal.staticNull );

    sNull2 = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------

    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdRealType   sRealValue1;
    mtdRealType * sValue1;
    mtdRealType   sRealValue2;
    mtdRealType * sValue2;
    idBool        sNull1;
    idBool        sNull2;

    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    ID_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------

    sValue2 = &sRealValue2;
    
    ID_FLOAT_BYTE_ASSIGN( sValue2, aValueInfo2->value );
    
    sNull2 = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdRealStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdRealType   sRealValue1;
    mtdRealType * sValue1;
    mtdRealType   sRealValue2;
    mtdRealType * sValue2;
    idBool        sNull1;
    idBool        sNull2;

    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    ID_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );
    
    sNull1 = ( ( *(UInt*)sValue1 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------

    sValue2 = &sRealValue2;
    
    ID_FLOAT_BYTE_ASSIGN( sValue2, aValueInfo2->value );
    
    sNull2 = ( ( *(UInt*)sValue2 & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ID_TRUE : ID_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ID_TRUE) && (sNull2 != ID_TRUE) )
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
    
    if( (sNull1 == ID_TRUE) && (sNull2 != ID_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ID_TRUE) /*&& (sNull2 == ID_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

void mtdEndian( void* aValue )
{
    UInt   sCount;
    UChar* sValue;
    UChar  sBuffer[MTD_REAL_SIZE];
    
    sValue = (UChar*)aValue;
    for( sCount = 0; sCount < MTD_REAL_SIZE; sCount++ )
    {
        sBuffer[MTD_REAL_SIZE-sCount-1] = sValue[sCount];
    }
    for( sCount = 0; sCount < MTD_REAL_SIZE; sCount++ )
    {
        sValue[sCount] = sBuffer[sCount];
    }
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
    
    IDE_TEST_RAISE( aValueSize != ID_SIZEOF(mtdRealType), ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdReal,
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
    mtdRealType  sReal;
    UInt         sStringLen;

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
        sReal = *(mtdRealType*)aValue;
    
        // BUG-17025
        if ( sReal == (mtdRealType)0 )
        {
            sReal = (mtdRealType)0;
        }
        else
        {
            // Nothing to do.
        }
        
        sStringLen = idlVA::appendFormat( (SChar*) aText, 
                                          *aTextLen,
                                          "%"ID_FLOAT_G_FMT,
                                          sReal );
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
                                       void            * aDestVale,
                                       UInt           /* aDestValueOffset */,
                                       UInt              aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdRealType  * sRealValue;

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sRealValue = (mtdRealType*) aDestVale;    
    
    if( aLength == 0 )
    {
        // NULL 데이타
        *((UInt*)sRealValue) = mtdRealNull;
    }
    else
    {
        IDE_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );
        IDE_TEST_RAISE( aLength != ID_SIZEOF( mtdRealType ), ERR_INVALID_STORED_VALUE );

        ID_FLOAT_BYTE_ASSIGN( sRealValue, aValue );
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
    return mtdActualSize( NULL, &mtdRealNull );
}

