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
 * $Id: mtdBlobLocator.cpp 16552 2006-06-07 02:29:08Z mhjeong $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdBlobLocator;

#define MTD_BLOB_LOCATOR_ALIGN             ( (SInt) ID_SIZEOF(mtdBlobLocatorType) )
#define MTD_BLOB_LOCATOR_PRECISION_DEFAULT (1)


static mtdBlobLocatorType mtdBlobLocatorNull = MTD_BLOB_LOCATOR_NULL;

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


static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcName mtdTypeName[1] = {
    { NULL, 12, (void*)"BLOB_LOCATOR" }
};

static mtcColumn mtdColumn;

mtdModule mtdBlobLocator = {
    mtdTypeName,
    &mtdColumn,
    MTD_BLOB_LOCATOR_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_BLOB_LOCATOR_ALIGN,
    MTD_GROUP_MISC|
      MTD_CANON_NEEDLESS|
      MTD_CREATE_DISABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_DISABLE|
      MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
      MTD_PSM_TYPE_DISABLE, // PROJ-1904
    0,
    0,
    0,
    &mtdBlobLocatorNull,
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
    {
        // Key Comparison
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
    mtd::encodeNA,
    mtd::decodeDefault,
    mtd::compileFmtDefault,
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
    mtd::mtdNullValueSizeNA,
    mtd::mtdHeaderSizeDefault,

    // PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdBlobLocator, aNo ) != IDE_SUCCESS );
    
    // IDE_TEST( mtdEstimate( &mtdColumn, 0, 0, 0 ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdBlobLocator,
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
    
    *aColumnSize = ID_SIZEOF(mtdBlobLocatorType);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));
    
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
                 IDE_RC*     aResult )
{
    *aResult = IDE_FAILURE;
    
    return IDE_SUCCESS;
}

UInt mtdActualSize( const mtcColumn* ,
                    const void* )
{
    return ID_SIZEOF(mtdBlobLocatorType);
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    if ( aRow != NULL )
    {
        *(mtdBlobLocatorType*)aRow = MTD_BLOB_LOCATOR_NULL;
    }
}

UInt mtdHash( UInt,
              const mtcColumn*,
              const void* )
{
    return 0;
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return ( *(mtdBlobLocatorType*)aRow == MTD_BLOB_LOCATOR_NULL ) ? ID_TRUE : ID_FALSE;
}

void mtdEndian( void* aValue )
{
    UChar* sLength;
    UChar  sIntermediate;

    sLength = (UChar*)(mtdBlobLocatorType*)aValue;  // smLobLocaotr is ULong
    sIntermediate = sLength[0];
    sLength[0]    = sLength[7];
    sLength[7]    = sIntermediate;
    sIntermediate = sLength[1];
    sLength[1]    = sLength[6];
    sLength[6]    = sIntermediate;
    sIntermediate = sLength[2];
    sLength[2]    = sLength[5];
    sLength[5]    = sIntermediate;
    sIntermediate = sLength[3];
    sLength[3]    = sLength[4];
    sLength[4]    = sIntermediate;
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
    
    IDE_TEST_RAISE( aValueSize != ID_SIZEOF(mtdBlobLocatorType),
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdBlobLocator,
                                     0,     // arguments
                                     0,     // precision
                                     0 )    // scale
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
                                       UInt           /* aDestValueOffset */,
                                       UInt              aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdBlobLocatorType* sValue;

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sValue = (mtdBlobLocatorType*)aDestValue;

    if( aLength == 0 )
    {
        // NULL 데이타
        *sValue = MTD_LOCATOR_NULL;
    }
    else
    {
        IDE_TEST_RAISE( aLength != aColumnSize , ERR_INVALID_STORED_VALUE);

        idlOS::memcpy( sValue, aValue, aLength );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
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
