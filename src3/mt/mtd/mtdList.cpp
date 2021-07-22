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
 * $Id: mtdList.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#define MTD_LIST_ALIGN (ID_SIZEOF(ULong))

extern mtdModule mtdList;

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

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"LIST" },
};

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdNullValueSize();

static UInt mtdHeaderSize();

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcColumn mtdColumn;

mtdModule mtdList = {
    mtdTypeName,
    &mtdColumn,
    MTD_LIST_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_LIST_ALIGN,
    MTD_GROUP_MISC|
      MTD_CANON_NEEDLESS|
      MTD_CREATE_DISABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_DISABLE|
      MTD_VARIABLE_LENGTH_TYPE_TRUE| // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_TRUE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_TRUE|  // PROJ-1705
      MTD_PSM_TYPE_DISABLE, // PROJ-1904
    0,
    0,
    0,
    NULL, // staticNull : List type cannot have static null.
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
        mtd::compareNA,           // Logical Comparison
        mtd::compareNA
    },
    {                         // Key Comparison
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 */
            mtd::compareNA,
            mtd::compareNA
        }
        ,
        {
            /* PROJ-2433 */
            mtd::compareNA,
            mtd::compareNA
        }
    },
    mtd::canonizeDefault,
    mtdEndian,
    mtdValidate,
    mtd::selectivityNA,
    NULL,
    NULL,
    NULL,
    mtd::valueFromOracleDefault,
    mtd::makeColumnInfoDefault,

    // BUG-28934
    mtk::mergeAndRangeNA,
    mtk::mergeOrRangeListNA,

    {    
        // PROJ-1705
        mtdStoredValue2MtdValue, 
        // PROJ-2429
        NULL 
    }, 
    mtdNullValueSize,
    mtdHeaderSize,
    
    // PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdList, aNo ) != IDE_SUCCESS );    

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdList,
                                     1,   // arguments
                                     1,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * aArguments,
                    SInt * aPrecision,
                    SInt * aScale )  // 실제 value를 저장할 byte size
{
    switch( *aArguments )
    {
        case 1:
            *aColumnSize = ID_SIZEOF(mtcStack) * (*aPrecision);
            break;
        case 2:
            *aColumnSize = ID_SIZEOF(mtcStack) * (*aPrecision) + (*aScale);
            break;
        default:
            IDE_RAISE( ERR_INVALID_LENGTH );
            break;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdValue( mtcTemplate*,
                 mtcColumn*,
                 void*,
                 UInt*,
                 UInt,
                 const void*,
                 UInt,
                 IDE_RC* )
{
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_APPLICABLE));
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn* aColumn,
                    const void* )
{
    return aColumn->column.size;
}

void mtdSetNull( const mtcColumn*,
                 void* )
{
    return;
}

UInt mtdHash( UInt             aHash,
              const mtcColumn*,
              const void* )
{
    return aHash;
}

idBool mtdIsNull( const mtcColumn*,
                  const void* )
{
    return ID_FALSE;
}

void mtdEndian( void* )
{
    return;
}


IDE_RC mtdValidate( mtcColumn * /* aColumn */ ,
                    void      * /* aValue */,
                    UInt        /* aValueSize */)
{
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_APPLICABLE));
    
    return IDE_FAILURE;
}
 
static IDE_RC mtdStoredValue2MtdValue( UInt          aColumnSize,
                                       void        * aDestValue,
                                       UInt          aDestValueOffset,
                                       UInt          aLength,
                                       const void  * aValue )
{
    IDE_TEST_RAISE( aDestValueOffset + aLength > aColumnSize,
                    ERR_INVALID_STORED_VALUE );
    
    if ( aLength > 0 )
    {
        idlOS::memcpy( (UChar*)aDestValue + aDestValueOffset, aValue, aLength );
    }
    else
    {
        // Nothing to do.
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
    return 0;
}

static UInt mtdHeaderSize()
{
    return 0;
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
