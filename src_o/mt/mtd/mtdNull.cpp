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
 * $Id: mtdNull.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#define MTD_NULL_ALIGN (ID_SIZEOF(UChar))
#define MTD_NULL_SIZE  (ID_SIZEOF(UChar))
#define MTD_NULL_VALUE (0xFF)

extern mtdModule mtdNull;

// for JDBC
const UChar mtdNullNull = 0;

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

static SInt mtdNullComp( mtdValueInfo * aValueInfo1,
                         mtdValueInfo * aValueInfo2 );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt           /* aDestValueOffset */,
                                       UInt              aLength,
                                       const void      * aValue );

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"NULL" },
};

static mtcColumn mtdColumn;

mtdModule mtdNull = {
    mtdTypeName,
    &mtdColumn,
    MTD_NULL_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_NULL_ALIGN,
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
    (void*)&mtdNullNull,
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
        mtdNullComp,       // Logical Comparison (NULL > NULL)
        mtdNullComp
    },
    {
        // Key Comparison  (ORDER BY NULL DESC)
        {
            mtdNullComp,      // Ascending Key Comparison
            mtdNullComp       // Descending Key Comparison
        }
        ,
        {
            mtdNullComp,      // Ascending Key Comparison
            mtdNullComp       // Descending Key Comparison
        }
        ,
        {
            mtdNullComp,      // Ascending Key Comparison
            mtdNullComp       // Descending Key Comparison
        }
        ,
        {
            mtdNullComp,      // Ascending Key Comparison
            mtdNullComp       // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 */
            mtdNullComp,
            mtdNullComp
        }
        ,
        {
            /* PROJ-2433 */
            mtdNullComp,
            mtdNullComp
        }
    },
    mtd::canonizeDefault,
    mtdEndian,
    mtdValidate,
    mtd::selectivityNA,
    NULL,
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
        mtd::mtdStoredValue2MtdValue4CompressColumn
    },
    mtd::mtdNullValueSizeNA,
    mtd::mtdHeaderSizeDefault,

    // PROJ-2399
    mtd::mtdStoreSizeDefault
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdNull, aNo ) != IDE_SUCCESS );    

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdNull,
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
                    SInt * /*aScale*/  )
{
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );

    *aColumnSize = MTD_NULL_SIZE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
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
    const UChar*    sToken;
    
    sToken = (const UChar*)aToken;
    
    if( aTokenLength == 4 || aTokenLength == 0 )
    {
        if( aTokenLength == 4 )
        {
            IDE_TEST_RAISE( ( sToken[0] != 'N' && sToken[0] != 'n' ) ||
                            ( sToken[1] != 'U' && sToken[1] != 'u' ) ||
                            ( sToken[2] != 'L' && sToken[2] != 'l' ) ||
                            ( sToken[3] != 'L' && sToken[3] != 'l' ),
                            ERR_INVALID_LITERAL );
        }
        
        // BUG-25739
        // 적어도 1byte의 공간이 필요하다.
        if( *aValueOffset < aValueSize )
        {
            aColumn->column.offset   = *aValueOffset;
            // BUG-20754
            *((UChar*)aValue + *aValueOffset) = MTD_NULL_VALUE;
            *aValueOffset += MTD_NULL_SIZE;
            
            *aResult = IDE_SUCCESS;
        }
        else
        {    
            *aResult = IDE_FAILURE;
        }
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

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
    return (UInt)MTD_NULL_SIZE;
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
    return ID_TRUE;
}

SInt mtdNullComp( mtdValueInfo * /* aValueInfo1 */,
                  mtdValueInfo * /* aValueInfo2 */ )
{
    // ORDER BY NULL DESC 등에서 사용됨.
    return 0;
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
        
    IDE_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );
    
    IDE_TEST_RAISE( aValueSize != MTD_NULL_SIZE, ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdNull,
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
 
static IDE_RC mtdStoredValue2MtdValue( UInt           /* aColumnSize */,
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

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    IDE_TEST_RAISE(aLength != MTD_NULL_SIZE, ERR_INVALID_STORED_VALUE);

    *((UChar*)aDestValue) = *((UChar*)aValue);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

