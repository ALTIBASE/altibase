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
 * $Id: mtdClob.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtdModule mtdClob;

#define MTD_CLOB_ALIGN             (MTD_LOB_ALIGN)

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
#define MTD_CLOB_PRECISION_DEFAULT (MTU_LOB_OBJECT_BUFFER_SIZE)

static mtdClobType mtdClobNull = { MTD_LOB_NULL_LENGTH, {'\0',} };

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

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtdCanonize( const mtcColumn  * aCanon,
                           void            ** aCanonizedValue,
                           mtcEncryptInfo   * aCanonInfo,
                           const mtcColumn  * aColumn,
                           void             * aValue,
                           mtcEncryptInfo   * aColumnInfo,
                           mtcTemplate      * aTemplate );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

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
    { NULL, 4, (void*)"CLOB" }
};

static mtcColumn mtdColumn;

mtdModule mtdClob = {
    mtdTypeName,
    &mtdColumn,
    MTD_CLOB_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_CLOB_ALIGN,
    MTD_GROUP_MISC|
      MTD_CANON_NEED|
      MTD_CREATE_ENABLE|
      MTD_COLUMN_TYPE_LOB|
      MTD_SELECTIVITY_DISABLE|
      MTD_SEARCHABLE_PRED_CHAR|    // BUG-17020
      MTD_CASE_SENS_TRUE|
      MTD_LITERAL_TRUE|
      MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_TRUE|  // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    ID_SINT_MAX,  // BUG-16493 실제로는 4G를 지원하지만
                  // odbc만을 위한 조회목적(V$DATATYPE)으로만 사용됨
    0,
    0,
    &mtdClobNull,
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
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtd::selectivityNA,
    mtd::encodeNA,
    mtd::decodeDefault,
    mtd::compileFmtDefault,
    mtdValueFromOracle,
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
    IDE_TEST( mtd::initializeModule( &mtdClob, aNo )
              != IDE_SUCCESS );
    
    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdClob,
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
        *aPrecision = MTD_CLOB_PRECISION_DEFAULT;
    }
    
    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_PRECISION );

    IDE_TEST_RAISE( *aPrecision < MTD_CLOB_PRECISION_MINIMUM ||
                    *aPrecision > MTD_CLOB_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = ID_SIZEOF(SLong) + *aPrecision;
    
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
    UInt         sValueOffset;
    UInt         sValueSize;
    mtdClobType* sValue;
    const UChar* sToken;
    const UChar* sTokenFence;
    UChar*       sIterator;
    UChar*       sFence;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_CLOB_ALIGN );

    sValue = (mtdClobType*)( (UChar*)aValue + sValueOffset );

    *aResult = IDE_SUCCESS;
    
    for( sIterator   = sValue->value,
         sFence      = (UChar*)aValue + aValueSize,
         sToken      = (const UChar*)aToken,
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
    
    if( *aResult == IDE_SUCCESS )
    {
        sValueSize = sIterator - sValue->value;
        if ( sValueSize == 0 )
        {
            // null
            sValue->length = MTD_LOB_NULL_LENGTH;
        }
        else
        {
            sValue->length = sValueSize;
        }

        IDE_TEST(
            mtc::initializeColumn( aColumn,
                                   & mtdClob,
                                   1,           // arguments
                                   sValueSize,
                                   0 )          // scale
                  != IDE_SUCCESS );

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset
                                   + ID_SIZEOF(SLong) + sValueSize;
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
    if ( ((mtdClobType *)aRow)->length == MTD_LOB_NULL_LENGTH )
    {
        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
        return ID_SIZEOF(SLong);
    }
    else
    {
        return ID_SIZEOF(SLong) + ((mtdClobType*)aRow)->length;
    }
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdClobType*)aRow)->length = MTD_LOB_NULL_LENGTH;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hashWithoutSpace( aHash, ((mtdClobType*)aRow)->value, ((mtdClobType*)aRow)->length );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (((mtdClobType*)aRow)->length == MTD_LOB_NULL_LENGTH) ? ID_TRUE : ID_FALSE;
}

void mtdEndian( void* aValue )
{
    UChar* sLength;
    UChar  sIntermediate;
    
    sLength = (UChar*)&(((mtdClobType*)aValue)->length);
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
        
    mtdClobType * sCharVal = (mtdClobType*)aValue;
    IDE_TEST_RAISE( sCharVal == NULL, ERR_INVALID_NULL );
    
    IDE_TEST_RAISE( aValueSize < ID_SIZEOF(SLong), ERR_INVALID_LENGTH);
    IDE_TEST_RAISE( sCharVal->length + ID_SIZEOF(SLong) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdClob,
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

IDE_RC mtdValueFromOracle( mtcColumn*  aColumn,
                           void*       aValue,
                           UInt*       aValueOffset,
                           UInt        aValueSize,
                           const void* aOracleValue,
                           SInt        aOracleLength,
                           IDE_RC*     aResult )
{
    UInt         sValueOffset;
    mtdClobType* sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_CLOB_ALIGN );
    
    if( aOracleLength < 0 )
    {
        aOracleLength = 0;
    }

    // aColumn의 초기화
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdClob,
                                     1,
                                     ( aOracleLength > 1 ) ? aOracleLength : 1,
                                     0 )
        != IDE_SUCCESS );
    
    if( sValueOffset + aColumn->column.size <= aValueSize )
    {
        sValue = (mtdClobType*)((UChar*)aValue+sValueOffset);
        
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

static IDE_RC mtdStoredValue2MtdValue( UInt           /* aColumnSize */, 
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue )
{
/***********************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 **********************************************************************/

    // LOB 컬럼은 레코드에 LOB컬럼헤더가 저장됨. 
    // 컬럼 패치시에는 레코드에 저장된 LOB컬럼헤더만 읽고
    // 실제데이타는 참조하는 부분에서 읽어온다.
    
    // LOB컬럼헤더가 여러페이지에 나누어 저장되는 경우는 없다. 
    IDE_TEST_RAISE( !(( aDestValueOffset == 0 ) && ( aLength > 0 )),
                    ERR_INVALID_STORED_VALUE );

    idlOS::memcpy( aDestValue, aValue, aLength );
    
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
/***********************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환    
 * 예 ) mtdLobType( UInt length; UChar value[1] ) 에서
 *      length타입인 UInt의 크기를 반환
 **********************************************************************/

    return mtdActualSize( NULL, &mtdClobNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdLobType( UInt length; UChar value[1] ) 에서
 *      length타입인 UInt의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return ID_SIZEOF(SLong);
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

static IDE_RC mtdCanonize( const mtcColumn  * aCanon,
                           void            ** aCanonizedValue,
                           mtcEncryptInfo   * /* aCanonInfo */,
                           const mtcColumn  * /* aColumn */,
                           void             * aValue,
                           mtcEncryptInfo   * /* aColumnInfo */,
                           mtcTemplate      * /* aTemplate */ )
{
    /* BUG-36429 LOB Column에 대해서는 precision을 검사하지 않는다. */
    if ( aCanon->precision != 0 )
    {
        IDE_TEST_RAISE( ((mtdClobType *)aValue)->length > (SLong)aCanon->precision,
                        ERR_INVALID_LENGTH );
    }
    else
    {
        /* Nothing to do */
    }

    *aCanonizedValue = aValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LENGTH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
