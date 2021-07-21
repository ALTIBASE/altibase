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
 * $Id: mtcdBinary.cpp 18998 2006-11-17 02:40:19Z leekmo $
 *
 * Description:
 *   PROJ-1583, PR-15722
 *   ODBC 표준의 SQL_BINARY에 대응하는 mtcdBinary 타입 제공
 *
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdBinary;

#define MTD_BINARY_ALIGN             ( (acp_sint32_t) sizeof(acp_double_t) )

mtdBinaryType mtcdBinaryNull = { 0, {'\0'}, {'\0',} };

static ACI_RC mtdInitializeBinary( acp_uint32_t aNo );

static ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                           acp_uint32_t* aArguments,
                           acp_sint32_t* aPrecision,
                           acp_sint32_t* aScale );

static ACI_RC mtdValue( mtcTemplate*  aTemplate,
                        mtcColumn*    aColumn,
                        void*         aValue,
                        acp_uint32_t* aValueOffset,
                        acp_uint32_t  aValueSize,
                        const void*   aToken,
                        acp_uint32_t  aTokenLength,
                        ACI_RC*       aResult );

static acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                                   const void*      aRow,
                                   acp_uint32_t     aFlag );

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale );

static void mtdNull( const mtcColumn* aColumn,
                     void*            aRow,
                     acp_uint32_t     aFlag );

static acp_uint32_t mtdHash( acp_uint32_t     aHash,
                             const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"BINARY" }
};

static mtcColumn mtdColumn;

mtdModule mtcdBinary = {
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
    MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    ACP_SINT32_MAX,
    0,
    0,
    &mtcdBinaryNull,
    mtdInitializeBinary,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    NULL,           // Logical Comparison
    {                       // Key Comparison
        {
            NULL,         // Ascending Key Comparison
            NULL          // Descending Key Comparison
        }
        ,
        {
            NULL,         // Ascending Key Comparison
            NULL          // Descending Key Comparison
        }
        ,
        {
            NULL,         // Ascending Key Comparison
            NULL          // Descending Key Comparison
        }
    },
    mtdCanonizeDefault,
    mtdEndian,
    mtdValidate,
    mtdSelectivityNA,
    mtdEncodeNA,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,  
    mtdNullValueSizeNA,
    mtdHeaderSize
};

ACI_RC mtdInitializeBinary( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdBinary, aNo ) != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdBinary,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                    acp_uint32_t* aArguments,
                    acp_sint32_t* aPrecision,
                    acp_sint32_t* aScale )
{
    ACE_ASSERT( aColumnSize != NULL );
    ACE_ASSERT( aArguments != NULL );
    ACE_ASSERT( aPrecision != NULL );
    ACE_ASSERT( aScale != NULL );
    
    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_BINARY_PRECISION_DEFAULT;
    }

    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_PRECISION );

    ACI_TEST_RAISE( *aPrecision < MTD_BINARY_PRECISION_MINIMUM ||
                    *aPrecision > MTD_BINARY_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = sizeof(acp_double_t) + *aPrecision;
    *aScale = 0;
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);

    ACI_EXCEPTION( ERR_INVALID_PRECISION );
    aciSetErrorCode(mtERR_ABORT_INVALID_PRECISION);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdValue( mtcTemplate*  aTemplate,
                 mtcColumn*    aColumn,
                 void*         aValue,
                 acp_uint32_t* aValueOffset,
                 acp_uint32_t  aValueSize,
                 const void*   aToken,
                 acp_uint32_t  aTokenLength,
                 ACI_RC*       aResult )
{
    acp_uint32_t   sValueOffset;
    mtdBinaryType* sValue;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_BINARY_ALIGN );

    sValue = (mtdBinaryType*)( (acp_uint8_t*)aValue + sValueOffset );

    *aResult = ACI_SUCCESS;
    
    if( ( (aTokenLength+1) >> 1 ) <= (acp_uint8_t*)aValue - sValue->mValue + aValueSize )
    {
        ACI_TEST( mtcMakeBinary( sValue,
                                 (const acp_uint8_t*)aToken,
                                 aTokenLength )
                  != ACI_SUCCESS );
        
        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;
        aColumn->precision       = sValue->mLength;
        aColumn->scale           = 0;

        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != ACI_SUCCESS );

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset
            + sizeof(acp_double_t) + sValue->mLength;
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                            const void*      aRow,
                            acp_uint32_t     aFlag )
{
    const mtdBinaryType* sValue;

    ACP_UNUSED(aColumn);
    
    sValue = (const mtdBinaryType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBinary.staticNull );
    
    return sizeof(acp_double_t) + sValue->mLength;
}

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale )
{
    const mtdBinaryType* sValue;

    ACP_UNUSED(aColumn);
    
    sValue = (const mtdBinaryType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBinary.staticNull );

    *aPrecision = sValue->mLength;
    *aScale = 0;
    
    return ACI_SUCCESS;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdBinaryType* sValue;

    ACP_UNUSED(aColumn);
    
    sValue = (mtdBinaryType*)
        mtdValueForModule( NULL, 
                           aRow, 
                           aFlag,
                           NULL );
    
    if( sValue != NULL )
    {
        sValue->mLength = 0;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBinaryType* sValue;

    ACP_UNUSED(aColumn);
    
    sValue = (const mtdBinaryType*)
        mtdValueForModule( NULL, 
                           aRow, 
                           aFlag,
                           mtcdBinary.staticNull );

    return mtcHashSkip( aHash, sValue->mValue, sValue->mLength );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBinaryType* sValue;

    ACP_UNUSED(aColumn);
    
    sValue = (const mtdBinaryType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBinary.staticNull );
    
    return (sValue->mLength == 0) ? ACP_TRUE : ACP_FALSE;
}


void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;

    sLength = (acp_uint8_t*)&(((mtdBinaryType*)aValue)->mLength);
    sIntermediate = sLength[0];
    sLength[0]    = sLength[1];
    sLength[1]    = sIntermediate;
}


ACI_RC mtdValidate( mtcColumn*   aColumn,
                    void*        aValue,
                    acp_uint32_t aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdBinaryType * sVal = (mtdBinaryType*)aValue;
    
    ACI_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );
    
    ACI_TEST_RAISE((aValueSize < sizeof(acp_double_t)) ||
                   (sVal->mLength + sizeof(acp_double_t) != aValueSize),
                   ERR_INVALID_LENGTH );

    ACI_TEST_RAISE( sVal->mLength > aColumn->column.size, ERR_INVALID_VALUE );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdBinary,
                                   1,            // arguments
                                   sVal->mLength, // precision
                                   0 )           // scale
              != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_NULL);
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE);
    }
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }

    ACI_EXCEPTION( ERR_INVALID_VALUE );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
 
static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdBinaryType* sValue;

    ACP_UNUSED(aColumnSize);
    
    sValue = (mtdBinaryType*)aDestValue;

    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sValue->mLength = 0;
    }
    else
    {
        ACI_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sValue->mLength = aDestValueOffset + aLength;
        acpMemCpy( (acp_uint8_t*)sValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 **********************************************************************/

    return offsetof(mtdBinaryType, mValue);
}

