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
 * $Id: mtdBinary.cpp 18998 2006-11-17 02:40:19Z leekmo $
 *
 * Description:
 *   PROJ-1583, PR-15722
 *   ODBC 표준의 SQL_BINARY에 대응하는 mtdBinary 타입 제공
 *
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdBinary;

#define MTD_BINARY_ALIGN             ( (SInt) ID_SIZEOF(SDouble) )
#define MTD_BINARY_PRECISION_DEFAULT (0)
#define MTD_BINARY_PRECISION_MINIMUM (0) 
#define MTD_BINARY_PRECISION_MAXIMUM ( ID_SINT_MAX ) // 2G

static mtdBinaryType mtdBinaryNull = { 0, {'\0'}, {'\0',} };

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

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdHeaderSize();

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"BINARY" }
};

static mtcColumn mtdColumn;

mtdModule mtdBinary = {
    mtdTypeName,
    &mtdColumn,
    MTD_BINARY_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_BINARY_ALIGN,
    MTD_GROUP_MISC|
      MTD_CANON_NEED|
      MTD_CREATE_DISABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_DISABLE|
      MTD_CREATE_PARAM_PRECISION|
      MTD_LITERAL_TRUE|
      MTD_VARIABLE_LENGTH_TYPE_TRUE| // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_TRUE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE| // PROJ-1705
      MTD_PSM_TYPE_DISABLE, // PROJ-1904
    ID_SINT_MAX,
    0,
    0,
    &mtdBinaryNull,
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
        mtd::mtdStoredValue2MtdValue4CompressColumn
    },
    mtd::mtdNullValueSizeNA,
    mtdHeaderSize,

    //PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdBinary, aNo ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdBinary,
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
                    SInt * aScale )
{
    IDE_ASSERT( aColumnSize != NULL );
    IDE_ASSERT( aArguments != NULL );
    IDE_ASSERT( aPrecision != NULL );
    IDE_ASSERT( aScale != NULL );
    
    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_BINARY_PRECISION_DEFAULT;
    }

    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_PRECISION );

    IDE_TEST_RAISE( *aPrecision < MTD_BINARY_PRECISION_MINIMUM ||
                    *aPrecision > MTD_BINARY_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = ID_SIZEOF(SDouble) + *aPrecision;
    *aScale = 0;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

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
    mtdBinaryType* sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_BINARY_ALIGN );

    sValue = (mtdBinaryType*)( (UChar*)aValue + sValueOffset );

    *aResult = IDE_SUCCESS;
    
    if( ( (aTokenLength+1) >> 1 ) <= (UChar*)aValue - sValue->mValue + aValueSize )
    {
        IDE_TEST( mtc::makeBinary( sValue,
                                   (const UChar*)aToken,
                                   aTokenLength )
                  != IDE_SUCCESS );
        
        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;
        aColumn->precision       = sValue->mLength;
        aColumn->scale           = 0;

        IDE_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != IDE_SUCCESS );

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset
                                   + ID_SIZEOF(SDouble) + sValue->mLength;
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn* ,
                    const void*      aRow )
{
    return ID_SIZEOF(SDouble) + ((mtdBinaryType*)aRow)->mLength;
}

static IDE_RC mtdGetPrecision( const mtcColumn * ,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale )
{
    *aPrecision = ((mtdBinaryType*)aRow)->mLength;
    *aScale = 0;

    return IDE_SUCCESS;
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdBinaryType*)aRow)->mLength = 0;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hashSkip( aHash, ((mtdBinaryType*)aRow)->mValue, ((mtdBinaryType*)aRow)->mLength );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (((mtdBinaryType*)aRow)->mLength == 0) ? ID_TRUE : ID_FALSE;
}


void mtdEndian( void* aValue )
{
    UChar* sLength;
    UChar  sIntermediate;

    sLength = (UChar*)&(((mtdBinaryType*)aValue)->mLength);
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
    
    mtdBinaryType * sVal = (mtdBinaryType*)aValue;
    
    IDE_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );
    
    IDE_TEST_RAISE((aValueSize < ID_SIZEOF(SDouble)) ||
                   (sVal->mLength + ID_SIZEOF(SDouble) != aValueSize),
                   ERR_INVALID_LENGTH );

    IDE_TEST_RAISE( sVal->mLength > aColumn->column.size, ERR_INVALID_VALUE );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdBinary,
                                     1,            // arguments
                                     sVal->mLength, // precision
                                     0 )           // scale
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
    IDE_EXCEPTION( ERR_INVALID_VALUE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE));
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

    mtdBinaryType* sValue;

    sValue = (mtdBinaryType*)aDestValue;

    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sValue->mLength = 0;
    }
    else
    {
        IDE_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sValue->mLength = aDestValueOffset + aLength;
        idlOS::memcpy( (UChar*)sValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 **********************************************************************/

    return offsetof(mtdBinaryType, mValue);
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
