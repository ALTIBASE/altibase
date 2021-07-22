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
 * $Id: mtcdBoolean.cpp 36231 2009-10-22 04:07:06Z kumdory $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

#define MTD_BOOLEAN_ALIGN   (sizeof(mtdBooleanType))
#define MTD_BOOLEAN_SIZE    (sizeof(mtdBooleanType))

extern mtdModule mtcdBoolean;

static mtdBooleanType mtdBooleanNull = MTD_BOOLEAN_NULL;

static ACI_RC mtdInitializeBoolean( acp_uint32_t aNo );

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

static ACI_RC mtdIsTrue( acp_bool_t*      aResult,
                         const mtcColumn* aColumn,
                         const void*      aRow,
                         acp_uint32_t     aFlag );

static acp_sint32_t mtdBooleanMtdMtdKeyComp( mtdValueInfo* aValueInfo1,
                                             mtdValueInfo* aValueInfo2 );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static mtcName mtdTypeName[1] = {
    { NULL, 7, (void*)"BOOLEAN" },
};

static mtcColumn mtdColumn;

static acp_uint32_t mtdBooleanHeaderSize();

static acp_uint32_t mtdBooleanNullValueSize();

static ACI_RC mtdBooleanStoredValue2MtdValue(acp_uint32_t aColumnSize,
                                             void*        aDestValue,
                                             acp_uint32_t aDestValueOffset,
                                             acp_uint32_t aLength,
                                             const void*  aValue);

mtdModule mtcdBoolean = {
    mtdTypeName,
    &mtdColumn,
    MTD_BOOLEAN_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_BOOLEAN_ALIGN,
    MTD_GROUP_MISC|
    MTD_CANON_NEEDLESS|
    MTD_CREATE_DISABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_DISABLE|
    MTD_SEARCHABLE_PRED_BASIC|
    MTD_UNSIGNED_ATTR_TRUE|
    MTD_NUM_PREC_RADIX_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    1,
    0,
    0,
    &mtdBooleanNull,
    mtdInitializeBoolean,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrue,
    mtdBooleanMtdMtdKeyComp,           // Logical Comparison
    {
        // Key Comparison (GROUP BY TRUE)
        {
            mtdBooleanMtdMtdKeyComp,         // Ascending Key Comparison
            mtdBooleanMtdMtdKeyComp          // Descending Key Comparison
        }
        ,
        {
            // 저장되지 않는 type이므로 이 부분으로 들어올수 없음
            NULL,
            NULL
        }
        ,
        {
            // 저장되지 않는 type이므로 이 부분으로 들어올수 없음
            NULL,
            NULL
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
    mtdBooleanStoredValue2MtdValue, 
    mtdBooleanNullValueSize,
    mtdBooleanHeaderSize
};

ACI_RC mtdInitializeBoolean( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdBoolean, aNo ) != ACI_SUCCESS );

//    ACI_TEST( mtdEstimate( &mtdColumn, 0, 0, 0 ) != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdBoolean,
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
    ACP_UNUSED(aPrecision);
    ACP_UNUSED(aScale);
    
    ACI_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );

    *aColumnSize = MTD_BOOLEAN_SIZE;
    
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
    acp_uint32_t       sValueOffset;
    mtdBooleanType*    sValue;
    const acp_uint8_t* sToken;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_BOOLEAN_ALIGN );

    *aResult = ACI_FAILURE;

    if( sValueOffset + MTD_BOOLEAN_SIZE <= aValueSize )
    {
        sValue = (mtdBooleanType*)( (acp_uint8_t*)aValue + sValueOffset );
        sToken = (const acp_uint8_t*)aToken;

        if( aTokenLength == 5                        &&
            ( sToken[0] == 'F' || sToken[0] == 'f' ) &&
            ( sToken[1] == 'A' || sToken[1] == 'a' ) &&
            ( sToken[2] == 'L' || sToken[2] == 'l' ) &&
            ( sToken[3] == 'S' || sToken[3] == 's' ) &&
            ( sToken[4] == 'E' || sToken[4] == 'e' )  )
        {
            *sValue  = MTD_BOOLEAN_FALSE;
            *aResult = ACI_SUCCESS;
        }
        else if( aTokenLength == 4                        &&
                 ( sToken[0] == 'T' || sToken[0] == 't' ) &&
                 ( sToken[1] == 'R' || sToken[1] == 'r' ) &&
                 ( sToken[2] == 'U' || sToken[2] == 'u' ) &&
                 ( sToken[3] == 'E' || sToken[3] == 'e' )  )
        {
            *sValue  = MTD_BOOLEAN_TRUE;
            *aResult = ACI_SUCCESS;
        }
        else if( ( aTokenLength == 0 )                      ||
                 ( aTokenLength == 4                        &&
                   ( sToken[0] == 'N' || sToken[0] == 'n' ) &&
                   ( sToken[1] == 'U' || sToken[1] == 'u' ) &&
                   ( sToken[2] == 'L' || sToken[2] == 'l' ) &&
                   ( sToken[3] == 'L' || sToken[3] == 'l' ) ) )
        {
            *sValue  = MTD_BOOLEAN_NULL;
            *aResult = ACI_SUCCESS;
        }
        else
        {
            ACI_RAISE( ERR_INVALID_LITERAL );
        }
        if( *aResult == ACI_SUCCESS )
        {
            aColumn->column.offset   = sValueOffset;
            *aValueOffset            = sValueOffset + MTD_BOOLEAN_SIZE;
        }
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

    return MTD_BOOLEAN_SIZE;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdBooleanType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdBooleanType*)
        mtdValueForModule( NULL, 
                           aRow, 
                           aFlag,
                           NULL );

    if( sValue != NULL)
    {
        *sValue = MTD_BOOLEAN_NULL;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBooleanType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBooleanType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBoolean.staticNull );
    
    return mtcHash( aHash, (const acp_uint8_t*)sValue, MTD_BOOLEAN_SIZE );

}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBooleanType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBooleanType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBoolean.staticNull );
    
    return (*sValue == MTD_BOOLEAN_NULL) ? ACP_TRUE : ACP_FALSE ;
}

ACI_RC mtdIsTrue( acp_bool_t*      aResult,
                  const mtcColumn* aColumn,
                  const void*      aRow,
                  acp_uint32_t     aFlag )
{
    const mtdBooleanType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBooleanType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBoolean.staticNull );

    *aResult = *sValue == MTD_BOOLEAN_TRUE ? ACP_TRUE : ACP_FALSE ;
    
    return ACI_SUCCESS;
}

acp_sint32_t
mtdBooleanMtdMtdKeyComp( mtdValueInfo* aValueInfo1,
                         mtdValueInfo* aValueInfo2 )
{
    const mtdBooleanType* sValue1;
    const mtdBooleanType* sValue2;

    //---------
    // value1
    //---------
    sValue1 = (const mtdBooleanType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value, 
                           aValueInfo1->flag,
                           mtcdBoolean.staticNull );

    //---------
    // value2
    //---------
    sValue2 = (const mtdBooleanType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value, 
                           aValueInfo2->flag,
                           mtcdBoolean.staticNull );

    //---------
    // compare
    //---------
    
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

void mtdEndian( void* aTemp )
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
        
    mtdBooleanType * sVal = (mtdBooleanType*)aValue;
    ACI_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL );
    
    ACI_TEST_RAISE( aValueSize != sizeof(mtdBooleanType),
                    ERR_INVALID_LENGTH);

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdBoolean,
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

static acp_uint32_t mtdBooleanHeaderSize()
{
    return 0;
}

static acp_uint32_t mtdBooleanNullValueSize()
{
    return 0;
}

static ACI_RC mtdBooleanStoredValue2MtdValue(acp_uint32_t aColumnSize,
                                             void*        aDestValue,
                                             acp_uint32_t aDestValueOffset,
                                             acp_uint32_t aLength,
                                             const void*  aValue)
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdBooleanType* sValue;

    ACP_UNUSED(aDestValueOffset);
    
    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sValue = (mtdBooleanType*)aDestValue;

    if( aLength == 0 )
    {
        // NULL 데이타
        *sValue = mtdBooleanNull;
    }
    else
    {
        ACI_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );

        *sValue = *(mtdBooleanType*)aValue;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;    
}

