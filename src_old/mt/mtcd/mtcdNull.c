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
 * $Id: mtcdNull.cpp 36319 2009-10-26 07:00:21Z sungminee $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

#define MTD_NULL_ALIGN (sizeof(acp_uint8_t))
#define MTD_NULL_SIZE  (sizeof(acp_uint8_t))
#define MTD_NULL_VALUE (0xFF)

extern mtdModule mtcdNull;

// for JDBC
const acp_uint8_t mtdNullNull = 0;

static ACI_RC mtdInitializeNull( acp_uint32_t aNo );

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

static void mtdNull_( const mtcColumn* aColumn,
                      void*            aRow,
                      acp_uint32_t     aFlag );

static acp_uint32_t mtdHash( acp_uint32_t     aHash,
                             const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static acp_sint32_t mtdNullComp( mtdValueInfo* aValueInfo1,
                                 mtdValueInfo* aValueInfo2 );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset ,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"NULL" },
};

static mtcColumn mtdColumn;

mtdModule mtcdNull = {
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
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    0,
    0,
    0,
    (void*)&mtdNullNull,
    mtdInitializeNull,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull_,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdNullComp,       // Logical Comparison (NULL > NULL)
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
    },
    mtdCanonizeDefault,
    mtdEndian,
    mtdValidate,
    mtdSelectivityNA,
    NULL,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue, 
    mtdNullValueSizeNA,
    mtdHeaderSizeDefault
};

ACI_RC mtdInitializeNull( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdNull, aNo ) != ACI_SUCCESS );    

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdNull,
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
                    acp_sint32_t* aScale  )
{
    ACP_UNUSED(aPrecision);
    ACP_UNUSED(aScale);
    
    ACI_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );

    *aColumnSize = MTD_NULL_SIZE;
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);
    
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
    const acp_uint8_t*    sToken;

    ACP_UNUSED(aTemplate);
    
    sToken = (const acp_uint8_t*)aToken;
    
    if( aTokenLength == 4 || aTokenLength == 0 )
    {
        if( aTokenLength == 4 )
        {
            ACI_TEST_RAISE( ( sToken[0] != 'N' && sToken[0] != 'n' ) ||
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
            *((acp_uint8_t*)aValue + *aValueOffset) = MTD_NULL_VALUE;
            *aValueOffset += MTD_NULL_SIZE;
            
            *aResult = ACI_SUCCESS;
        }
        else
        {    
            *aResult = ACI_FAILURE;
        }
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aTemp,
                            const void*      aTemp2,
                            acp_uint32_t     aTemp3)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    
    return (acp_uint32_t)MTD_NULL_SIZE;
}

void mtdNull_( const mtcColumn* aTemp,
               void*            aTemp2,
               acp_uint32_t     aTemp3 )
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
}

acp_uint32_t mtdHash( acp_uint32_t             aHash,
                      const mtcColumn*         aTemp,
                      const void*              aTemp2,
                      acp_uint32_t             aTemp3)
{
    ACP_UNUSED(aHash);
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    
    return aHash;
}

acp_bool_t mtdIsNull( const mtcColumn* aTemp,
                      const void*      aTemp2,
                      acp_uint32_t     aTemp3)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);

    return ACP_TRUE;
}

acp_sint32_t
mtdNullComp( mtdValueInfo*  aValueInfo1 ,
             mtdValueInfo*  aValueInfo2  )
{
    ACP_UNUSED(aValueInfo1);
    ACP_UNUSED(aValueInfo2);
    
    // ORDER BY NULL DESC 등에서 사용됨.
    return 0;
}

void mtdEndian( void* aTemp)
{
    ACP_UNUSED(aTemp);
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
        
    ACI_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );
    
    ACI_TEST_RAISE( aValueSize != MTD_NULL_SIZE, ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdNull,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
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
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t    aColumnSize ,
                                       void*           aDestValue,
                                       acp_uint32_t    aDestValueOffset ,
                                       acp_uint32_t    aLength,
                                       const void*     aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.
    ACP_UNUSED(aColumnSize);
    ACP_UNUSED(aDestValueOffset);
    ACP_UNUSED(aLength);
    
    ACI_TEST_RAISE(aLength != MTD_NULL_SIZE, ERR_INVALID_STORED_VALUE);

    *((acp_uint8_t*)aDestValue) = *((acp_uint8_t*)aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

