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
 * $Id: mtdDouble.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#include <math.h>

#define MTD_DOUBLE_ALIGN   (ID_SIZEOF(ULong))
#define MTD_DOUBLE_SIZE    (ID_SIZEOF(mtdDoubleType))

extern mtdModule mtdDouble;

/* PROJ-2632 */
const ULong mtdDoubleNull = ( MTD_DOUBLE_EXPONENT_MASK |
                              MTD_DOUBLE_FRACTION_MASK );

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

static SInt mtdDoubleLogicalAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleLogicalDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                 mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 );

static SInt mtdDoubleStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                              mtdValueInfo * aValueInfo2 );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static SDouble mtdSelectivityDouble( void     * aColumnMax,
                                     void     * aColumnMin,
                                     void     * aValueMax,
                                     void     * aValueMin,
                                     SDouble    aBoundFactor,
                                     SDouble    aTotalRecordCnt );

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
    { NULL, 6, (void*)"DOUBLE" },
};

static mtcColumn mtdColumn;

mtdModule mtdDouble = {
    mtdTypeName,
    &mtdColumn,
    MTD_DOUBLE_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_DOUBLE_ALIGN,
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
    15,
    0,
    0,
    (void*)&mtdDoubleNull, /* PROJ-2632 */
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
        mtdDoubleLogicalAscComp,      // Logical Comparison
        mtdDoubleLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdDoubleFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdDoubleFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdDoubleMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdDoubleMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdDoubleStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdDoubleStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdDoubleStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdDoubleStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdDoubleFixedMtdFixedMtdKeyAscComp,
            mtdDoubleFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdDoubleMtdMtdKeyAscComp,
            mtdDoubleMtdMtdKeyDescComp
        }
    },
    mtd::canonizeDefault,
    mtdEndian,
    mtdValidate,
    mtdSelectivityDouble,
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
    IDE_TEST_RAISE( MTD_DOUBLE_SIZE != 8, ERR_INCOMPATIBLE_TYPE );
    
    IDE_TEST( mtd::initializeModule( &mtdDouble, aNo ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdDouble,
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

    * aColumnSize = MTD_DOUBLE_SIZE;
    
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
    UInt           sValueOffset;
    mtdDoubleType* sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_DOUBLE_ALIGN );
    
    if( sValueOffset + MTD_DOUBLE_SIZE <= aValueSize )
    {
        sValue = (mtdDoubleType*)( (UChar*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            mtdDouble.null( mtdDouble.column, sValue );
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
            IDE_TEST_RAISE( mtdIsNull( mtdDouble.column,
                                       sValue ) == ID_TRUE,
                            ERR_VALUE_OVERFLOW );

            // To fix BUG-12281
            // underflow 검사
            if( ( idlOS::fabs(*sValue) < MTD_DOUBLE_MINIMUM ) &&
                ( *sValue != 0 ) )
            {
                *sValue = 0;
            }
            else
            {
                // Nothing to do
            }
        }
     
        // to fix BUG-12582
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_DOUBLE_SIZE;
        
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
    return (UInt)MTD_DOUBLE_SIZE;
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    static ULong sNull = ( MTD_DOUBLE_EXPONENT_MASK |
                           MTD_DOUBLE_FRACTION_MASK );

    if( aRow != NULL )
    {
        *((ULong*)aRow) = sNull;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* aColumn,
              const void*      aRow )
{
    if( mtdDouble.isNull( aColumn, aRow ) != ID_TRUE )
    {
        aHash = mtc::hash( aHash, (const UChar*)aRow, MTD_DOUBLE_SIZE );
    }

    return aHash;
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return ( *(ULong*)aRow & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;
}

SInt mtdDoubleLogicalAscComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdDoubleType * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdDoubleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sNull1  = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdDoubleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sNull2  = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

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

SInt mtdDoubleLogicalDescComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdDoubleType * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdDoubleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sNull1  = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdDoubleType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sNull2  = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

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

SInt mtdDoubleFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdDoubleType * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdDoubleType*)MTD_VALUE_FIXED( aValueInfo1 );
    sNull1  = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdDoubleType*)MTD_VALUE_FIXED( aValueInfo2 );
    sNull2  = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

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

SInt mtdDoubleFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdDoubleType * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdDoubleType*)MTD_VALUE_FIXED( aValueInfo1 );
    sNull1  = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (const mtdDoubleType*)MTD_VALUE_FIXED( aValueInfo2 );
    sNull2  = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

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

SInt mtdDoubleMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdDoubleType * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDoubleType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdDouble.staticNull );

    sNull1  = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (mtdDoubleType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdDouble.staticNull );

    sNull2  = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

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

SInt mtdDoubleMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdDoubleType * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDoubleType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdDouble.staticNull );

    sNull1  = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------
    sValue2 = (mtdDoubleType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdDouble.staticNull );

    sNull2  = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

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

SInt mtdDoubleStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDoubleType         sDoubleValue1;
    mtdDoubleType       * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;
    
    //---------
    // value1
    //---------
    
    sValue1 = &sDoubleValue1;

    ID_DOUBLE_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------        
    sValue2 = (const mtdDoubleType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdDouble.staticNull );

    sNull2 = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;
    
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

SInt mtdDoubleStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDoubleType         sDoubleValue1;
    mtdDoubleType       * sValue1;
    const mtdDoubleType * sValue2;
    idBool                sNull1;
    idBool                sNull2;
    
    //---------
    // value1
    //---------

    sValue1 = &sDoubleValue1;

    ID_DOUBLE_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;
    

    //---------
    // value2
    //---------        
    sValue2 = (const mtdDoubleType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdDouble.staticNull );

    sNull2 = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

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

SInt mtdDoubleStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDoubleType         sDoubleValue1;
    mtdDoubleType       * sValue1;
    mtdDoubleType         sDoubleValue2;
    mtdDoubleType       * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------

    sValue1 = &sDoubleValue1;
    
    ID_DOUBLE_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------

    sValue2 = &sDoubleValue2;

    ID_DOUBLE_BYTE_ASSIGN( sValue2, aValueInfo2->value );

    sNull2 = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;
    
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

SInt mtdDoubleStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDoubleType         sDoubleValue1;
    mtdDoubleType       * sValue1;
    mtdDoubleType         sDoubleValue2;
    mtdDoubleType       * sValue2;
    idBool                sNull1;
    idBool                sNull2;

    //---------
    // value1
    //---------

    sValue1 = &sDoubleValue1;
    
    ID_DOUBLE_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( *(ULong*)sValue1 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;

    //---------
    // value2
    //---------

    sValue2 = &sDoubleValue2;

    ID_DOUBLE_BYTE_ASSIGN( sValue2, aValueInfo2->value );

    sNull2 = ( *(ULong*)sValue2 & MTD_DOUBLE_EXPONENT_MASK )
             == MTD_DOUBLE_EXPONENT_MASK ? ID_TRUE : ID_FALSE ;
    
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
    UChar  sBuffer[MTD_DOUBLE_SIZE];
    
    sValue = (UChar*)aValue;
    for( sCount = 0; sCount < MTD_DOUBLE_SIZE; sCount++ )
    {
        sBuffer[MTD_DOUBLE_SIZE-sCount-1] = sValue[sCount];
    }
    for( sCount = 0; sCount < MTD_DOUBLE_SIZE; sCount++ )
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
        
    IDE_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL);
    
    IDE_TEST_RAISE( aValueSize != ID_SIZEOF(mtdDoubleType),
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdDouble,
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


SDouble mtdSelectivityDouble( void     * aColumnMax,
                              void     * aColumnMin,
                              void     * aValueMax,
                              void     * aValueMin,
                              SDouble    aBoundFactor,
                              SDouble    aTotalRecordCnt )
{
/***********************************************************************
 *
 * Description :
 *    DOUBLE 의 Selectivity 추출 함수
 *
 * Implementation :
 *
 *      1. NULL 검사 : S = DS
 *      2. ColumnMin > ColumnMax 검증 (DASSERT)
 *      3. ColumnMin > ValueMax 또는 ColumnMax < ValueMin 검증 :
 *         S = 1 / totalRecordCnt
 *      4. ValueMin, ValueMax 보정
 *       - ValueMin < ColumnMin 검증 : ValueMin = ColumnMin (보정)
 *       - ValueMax > ColumnMax 검증 : ValueMax = ColumnMax (보정)
 *      5. ColumnMax == ColumnMin 검증 (분모값) : S = 1
 *      6. ValueMax <= ValueMin 검증 (분자값) : S = 1 / totalRecordCnt
 *         ex) i1 < 1 and i1 > 3  -> ValueMax:1, ValueMin:3
 *      7. Etc : S = (ValueMax - ValueMin) / (ColumnMax - ColumnMin)
 *       - <=, >=, BETWEEN 에 대해 경계값 보정
 *
 ***********************************************************************/
    
    mtdDoubleType * sColumnMax;
    mtdDoubleType * sColumnMin;
    mtdDoubleType * sValueMax;
    mtdDoubleType * sValueMin;
    mtdDoubleType * sULongPtr;
    mtdValueInfo    sColumnMaxInfo;
    mtdValueInfo    sColumnMinInfo;
    mtdValueInfo    sValueMaxInfo;
    mtdValueInfo    sValueMinInfo;
    SDouble         sDenominator;   // 분모값
    SDouble         sNumerator;     // 분자값
    SDouble         sSelectivity;
    
    sColumnMax = (mtdDoubleType*) aColumnMax;
    sColumnMin = (mtdDoubleType*) aColumnMin;
    sValueMax  = (mtdDoubleType*) aValueMax;
    sValueMin  = (mtdDoubleType*) aValueMin;
    sSelectivity = MTD_DEFAULT_SELECTIVITY;

    //------------------------------------------------------
    // Selectivity 계산
    //------------------------------------------------------

    if ( ( mtdIsNull( NULL, aColumnMax ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aColumnMin ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aValueMax  ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aValueMin  ) == ID_TRUE ) )
    {
        //------------------------------------------------------
        // 1. NULL 검사 : 계산할 수 없음
        //------------------------------------------------------

        // MTD_DEFAULT_SELECTIVITY;
    }
    else
    {
        sColumnMaxInfo.column = NULL;
        sColumnMaxInfo.value  = sColumnMax;
        sColumnMaxInfo.flag   = MTD_OFFSET_USELESS;

        sColumnMinInfo.column = NULL;
        sColumnMinInfo.value  = sColumnMin;
        sColumnMinInfo.flag   = MTD_OFFSET_USELESS;

        sValueMaxInfo.column = NULL;
        sValueMaxInfo.value  = sValueMax;
        sValueMaxInfo.flag   = MTD_OFFSET_USELESS;

        sValueMinInfo.column = NULL;
        sValueMinInfo.value  = sValueMin;
        sValueMinInfo.flag   = MTD_OFFSET_USELESS;
        
        if ( mtdDoubleLogicalAscComp( &sColumnMinInfo,
                                      &sColumnMaxInfo ) > 0 )
        {
            //------------------------------------------------------
            // 2. ColumnMin > ColumnMax 검증 (잘못된 통계 정보)
            //------------------------------------------------------

            IDE_DASSERT_MSG( 1,
                             "sColumnMin : %"ID_DOUBLE_G_FMT",",
                             "sColumnMax : %"ID_DOUBLE_G_FMT"\n",
                             *((SDouble *)aColumnMin),
                             *((SDouble *)sColumnMax) );
        }
        else
        {
            if ( ( mtdDoubleLogicalAscComp( &sColumnMinInfo,
                                            &sValueMaxInfo ) > 0 )
                 ||
                 ( mtdDoubleLogicalAscComp( &sColumnMaxInfo,
                                            &sValueMinInfo ) < 0 ) )
            {
                //------------------------------------------------------
                // 3. ColumnMin > ValueMax 또는 ColumnMax < ValueMin 검증
                //------------------------------------------------------

                sSelectivity = 1 / aTotalRecordCnt;
            }
            else
            {
                //------------------------------------------------------
                // 4. 보정
                //  - ValueMin < ColumnMin 검증 : ValueMin = ColumnMin
                //  - ValueMax > ColumnMax 검증 : ValueMax = ColumnMax
                //------------------------------------------------------

                sValueMin = ( mtdDoubleLogicalAscComp(
                                  &sValueMinInfo,
                                  &sColumnMinInfo ) < 0 ) ?
                    sColumnMin: sValueMin;

                sValueMax = ( mtdDoubleLogicalAscComp(
                                  &sValueMaxInfo,
                                  &sColumnMaxInfo ) > 0 ) ?
                    sColumnMax: sValueMax;
                
                sDenominator = (SDouble)(*sColumnMax - *sColumnMin);
                sULongPtr = &sDenominator;
        
                if ( ( sDenominator == 0.0 ) ||
                     ( ( *(ULong*)sULongPtr & MTD_DOUBLE_EXPONENT_MASK )
                       == MTD_DOUBLE_EXPONENT_MASK ) )
                {
                    //------------------------------------------------------
                    // 5. ColumnMax == ColumnMin 검증 (분모값)
                    //------------------------------------------------------

                    sSelectivity = 1;
                }
                else
                {
                    sNumerator = (SDouble) (*sValueMax - *sValueMin);
                    sULongPtr = &sNumerator;

                    // jhseong, NaN check, PR-9195
                    if( ( sNumerator <= 0.0 ) ||
                        ( ( *(ULong*)sULongPtr & MTD_DOUBLE_EXPONENT_MASK )
                          == MTD_DOUBLE_EXPONENT_MASK ) )
                    {
                        //------------------------------------------------------
                        // 6. ValueMax <= ValueMin 검증 (분자값)
                        //------------------------------------------------------
                
                        // PROJ-2242
                        sSelectivity = 1 / aTotalRecordCnt;
                    }
                    else
                    {
                        //------------------------------------------------------
                        // 7. Etc : Min-Max selectivity
                        //------------------------------------------------------

                        sSelectivity = sNumerator / sDenominator;
                        sSelectivity += aBoundFactor;
                        sSelectivity = ( sSelectivity > 1 ) ? 1: sSelectivity;
                    }
                }
            }
        }
    }

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return sSelectivity;

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
    mtdDoubleType  sDouble;
    UInt           sStringLen;

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
        sDouble = *(mtdDoubleType*)aValue;
    
        // BUG-17025
        if ( sDouble == (mtdDoubleType)0 )
        {
            sDouble = (mtdDoubleType)0;
        }
        else
        {
            // Nothing to do.
        }
        
        sStringLen = idlVA::appendFormat( (SChar*) aText, 
                                          *aTextLen,
                                          "%"ID_DOUBLE_G_FMT,
                                          sDouble );
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
                                       UInt              /*aDestValueOffset*/,
                                       UInt              aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdDoubleType  * sDoubleValue;

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sDoubleValue = (mtdDoubleType*)aDestValue;
    
    if( aLength == 0 )
    {
        // NULL 데이타
        *((ULong*)sDoubleValue) = mtdDoubleNull;
    }
    else
    {
        IDE_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );
        IDE_TEST_RAISE( aLength != ID_SIZEOF( mtdDoubleType ), ERR_INVALID_STORED_VALUE );

        ID_DOUBLE_BYTE_ASSIGN( sDoubleValue, aValue );
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
    return mtdActualSize( NULL, &mtdDoubleNull );
}

