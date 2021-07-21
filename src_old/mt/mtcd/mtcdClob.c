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
 * $Id: mtcdClob.cpp 37082 2009-12-03 00:30:31Z sparrow $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdClob;

#define MTD_CLOB_ALIGN             ( (acp_sint32_t) sizeof(acp_uint32_t) )
#define MTD_CLOB_PRECISION_DEFAULT (0)

static mtdClobType mtdClobNull = { 0, {'\0',} };

static ACI_RC mtdInitializeClob( acp_uint32_t aNo );

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

static ACI_RC mtdValueFromOracle( mtcColumn*    aColumn,
                                  void*         aValue,
                                  acp_uint32_t* aValueOffset,
                                  acp_uint32_t  aValueSize,
                                  const void*   aOracleValue,
                                  acp_sint32_t  aOracleLength,
                                  ACI_RC*       aResult );

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"CLOB" }
};

static mtcColumn mtdColumn;

mtdModule mtcdClob = {
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
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    ACP_SINT32_MAX,  // BUG-16493 실제로는 4G를 지원하지만
    // odbc만을 위한 조회목적(V$DATATYPE)으로만 사용됨
    0,
    0,
    &mtdClobNull,
    mtdInitializeClob,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    NULL,           // Logical Comparison
    {                         // Key Comparison
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
    mtdValueFromOracle,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSize
};

ACI_RC mtdInitializeClob( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdClob, aNo )
              != ACP_RC_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdClob,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
              != ACP_RC_SUCCESS );
    
    return ACP_RC_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                    acp_uint32_t* aArguments,
                    acp_sint32_t* aPrecision,
                    acp_sint32_t* aScale )
{
    ACP_UNUSED(aScale);

    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_CLOB_PRECISION_DEFAULT;
    }
    
    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_PRECISION );

    ACI_TEST_RAISE( *aPrecision < MTD_CLOB_PRECISION_MINIMUM ||
                    *aPrecision > MTD_CLOB_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = sizeof(acp_uint32_t) + *aPrecision;
    
    return ACP_RC_SUCCESS;
    
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
    acp_uint32_t       sValueOffset;
    mtdClobType*       sValue;
    const acp_uint8_t* sToken;
    const acp_uint8_t* sTokenFence;
    acp_uint8_t*       sIterator;
    acp_uint8_t*       sFence;

    ACP_UNUSED( aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_CLOB_ALIGN );

    sValue = (mtdClobType*)( (acp_uint8_t*)aValue + sValueOffset );

    *aResult = ACP_RC_SUCCESS;
    
    for( sIterator   = sValue->value,
             sFence      = (acp_uint8_t*)aValue + aValueSize,
             sToken      = (const acp_uint8_t*)aToken,
             sTokenFence = sToken + aTokenLength;
         sToken      < sTokenFence;
         sIterator++, sToken++ )
    {
        if( sIterator >= sFence )
        {
            *aResult = ACI_FAILURE;
            break;
        }
        if( *sToken == '\'' )
        {
            sToken++;
            ACI_TEST_RAISE( sToken >= sTokenFence || *sToken != '\'',
                            ERR_INVALID_LITERAL );
        }
        *sIterator = *sToken;
    }
    
    if( *aResult == ACP_RC_SUCCESS )
    {
        sValue->length           = sIterator - sValue->value;

        ACI_TEST(
            mtcInitializeColumn( aColumn,
                                 & mtcdClob,
                                 1,           // arguments
                                 sValue->length,
                                 0 )          // scale
            != ACP_RC_SUCCESS );

        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset
            + sizeof(acp_uint32_t) + sValue->length;
    }
    
    return ACP_RC_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                            const void*      aRow,
                            acp_uint32_t     aFlag )
{
    const mtdClobType* sValue;

    ACP_UNUSED( aColumn);

    sValue = (const mtdClobType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdClob.staticNull );

    return sizeof(acp_uint32_t) + sValue->length;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdClobType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdClobType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           NULL );

    if( sValue != NULL )
    {
        sValue->length = 0;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdClobType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdClobType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdClob.staticNull );

    return mtcHashWithoutSpace( aHash, sValue->value, sValue->length );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdClobType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdClobType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdClob.staticNull );

    return (sValue->length == 0) ? ACP_TRUE : ACP_FALSE;
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;
    
    sLength = (acp_uint8_t*)&(((mtdClobType*)aValue)->length);
    
    sIntermediate = sLength[0];
    sLength[0]    = sLength[3];
    sLength[3]    = sIntermediate;
    sIntermediate = sLength[1];
    sLength[1]    = sLength[2];
    sLength[2]    = sIntermediate;
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
        
    mtdClobType * sCharVal = (mtdClobType*)aValue;
    ACI_TEST_RAISE( sCharVal == NULL, ERR_INVALID_NULL );
    
    ACI_TEST_RAISE( aValueSize < sizeof(acp_uint32_t), ERR_INVALID_LENGTH);
    ACI_TEST_RAISE( sCharVal->length + sizeof(acp_uint32_t) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdClob,
                                   1,                // arguments
                                   sCharVal->length, // precision
                                   0 )               // scale
              != ACP_RC_SUCCESS );

    return ACP_RC_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_NULL);
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE);
    }
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdValueFromOracle( mtcColumn*    aColumn,
                           void*         aValue,
                           acp_uint32_t* aValueOffset,
                           acp_uint32_t  aValueSize,
                           const void*   aOracleValue,
                           acp_sint32_t  aOracleLength,
                           ACI_RC*       aResult )
{
    acp_uint32_t sValueOffset;
    mtdClobType* sValue;
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_CLOB_ALIGN );
    
    if( aOracleLength < 0 )
    {
        aOracleLength = 0;
    }

    // aColumn의 초기화
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdClob,
                                   1,
                                   ( aOracleLength > 1 ) ? aOracleLength : 1,
                                   0 )
              != ACP_RC_SUCCESS );
    
    if( sValueOffset + aColumn->column.size <= aValueSize )
    {
        sValue = (mtdClobType*)((acp_uint8_t*)aValue+sValueOffset);
        
        sValue->length = aOracleLength;
        
        acpMemCpy( sValue->value, aOracleValue, aOracleLength );
        
        aColumn->column.offset = sValueOffset;
        
        *aValueOffset = sValueOffset + aColumn->column.size;
      
        *aResult = ACP_RC_SUCCESS;
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACP_RC_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize, 
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue )
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
    ACP_UNUSED( aColumnSize);

    ACI_TEST_RAISE( !( ( aDestValueOffset == 0 ) && ( aLength > 0 ) ) , ERR_INVALID_STORED_VALUE );

    acpMemCpy( aDestValue, aValue, aLength );

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;    
}


acp_uint32_t mtdNullValueSize()
{
/***********************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환    
 * 예 ) mtdLobType( acp_uint32_t length; acp_uint8_t value[1] ) 에서
 *      length타입인 acp_uint32_t의 크기를 반환
 **********************************************************************/

    return mtdActualSize( NULL,
                          & mtdClobNull,
                          MTD_OFFSET_USELESS );   
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdLobType( acp_uint32_t length; acp_uint8_t value[1] ) 에서
 *      length타입인 acp_uint32_t의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint32_t);
}

