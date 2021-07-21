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
 * $Id: mtcdInterval.cpp 36231 2009-10-22 04:07:06Z kumdory $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

#include <math.h>

extern mtdModule mtcdInterval;

extern mtdModule mtcdDouble;

#define MTD_INTERVAL_ALIGN (sizeof(acp_uint64_t))
#define MTD_INTERVAL_SIZE  (sizeof(mtdIntervalType))

const mtdIntervalType mtcdIntervalNull = {
    -ACP_SINT64_LITERAL(9223372036854775807)-1,
    -ACP_SINT64_LITERAL(9223372036854775807)-1
};

static ACI_RC mtdInitializeInterval( acp_uint32_t aNo );

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

static acp_sint32_t mtdIntervalLogicalComp( mtdValueInfo* aValueInfo1,
                                            mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdIntervalMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                 mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdIntervalMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                  mtdValueInfo* aValueInfo2  );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*    aColumn,
                           void*         aValue,
                           acp_uint32_t  aValueSize);

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void*             aDestValue,
                                       acp_uint32_t      aDestValueOffset ,
                                       acp_uint32_t      aLength,
                                       const void*       aValue );

static mtcName mtdTypeName[1] = {
    { NULL, 8, (void*)"INTERVAL" }
};

static mtcColumn mtdColumn;

mtdModule mtcdInterval = {
    mtdTypeName,
    &mtdColumn,
    MTD_INTERVAL_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_INTERVAL_ALIGN,
    MTD_GROUP_INTERVAL|
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
    (void*)&mtcdIntervalNull,
    mtdInitializeInterval,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdIntervalLogicalComp,   // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdIntervalMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdIntervalMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // 저장되지 않는 타입이므로 이 부분으로 들어올 수 없음 
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

ACI_RC mtdInitializeInterval( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdInterval, aNo ) != ACI_SUCCESS );
    
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdInterval,
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

    *aColumnSize = MTD_INTERVAL_SIZE;
    
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
    acp_uint32_t     sValueOffset;
    mtdIntervalType* sValue;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_INTERVAL_ALIGN );

    if( sValueOffset + MTD_INTERVAL_SIZE <= aValueSize )
    {
        sValue = (mtdIntervalType*)( (acp_uint8_t*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            *sValue = mtcdIntervalNull;
        }
        else
        {
            /* BUGBUG: rewrite */
            acp_char_t   sBuffer[1024];
            acp_double_t sDouble;
            acp_double_t sIntegerPart;
            acp_double_t sFractionalPart;
            ACI_TEST_RAISE( aTokenLength >= sizeof(sBuffer),
                            ERR_INVALID_LITERAL );
            acpMemCpy( sBuffer, aToken, aTokenLength );
            sBuffer[aTokenLength] = 0;

            acpCStrToDouble( sBuffer,
                             acpCStrLen(sBuffer, ACP_UINT32_MAX),
                             &sDouble,
                             (char**)NULL);

            ACI_TEST_RAISE( mtcdDouble.isNull( mtcdDouble.column,
                                              &sDouble,
                                              MTD_OFFSET_USELESS ) == ACP_TRUE,
                            ERR_VALUE_OVERFLOW );
            /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            if( sDouble == 0 )
            {
                sDouble = 0;
            }
            /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            sDouble *= 864e2;
            ACI_TEST_RAISE( mtcdDouble.isNull( mtcdDouble.column,
                                              &sDouble,
                                              MTD_OFFSET_USELESS ) == ACP_TRUE,
                            ERR_VALUE_OVERFLOW );
            sFractionalPart     = modf( sDouble, &sIntegerPart );
            sValue->second      = (acp_sint64_t)sIntegerPart;
            sValue->microsecond = (acp_sint64_t)( sFractionalPart * 1E6 );
        }
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_INTERVAL_SIZE;
        
        *aResult = ACI_SUCCESS;
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    
    ACI_EXCEPTION( ERR_VALUE_OVERFLOW );
    aciSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW);
    
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
    
    return MTD_INTERVAL_SIZE;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdIntervalType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdIntervalType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           NULL );

    if( sValue != NULL )
    {
        *sValue = mtcdIntervalNull;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdIntervalType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdIntervalType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdInterval.staticNull );

    return mtcHash( aHash, (const acp_uint8_t*)sValue, MTD_INTERVAL_SIZE );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdIntervalType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdIntervalType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdInterval.staticNull );

    return  MTD_INTERVAL_IS_NULL( sValue ) ? ACP_TRUE : ACP_FALSE ;
}

acp_sint32_t
mtdIntervalLogicalComp( mtdValueInfo* aValueInfo1,
                        mtdValueInfo* aValueInfo2 )
{
    return mtdIntervalMtdMtdKeyAscComp( aValueInfo1,
                                        aValueInfo2 );
}

acp_sint32_t
mtdIntervalMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                             mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    const mtdIntervalType* sIntervalValue1;
    const mtdIntervalType* sIntervalValue2;
    acp_sint64_t           sSecond1;
    acp_sint64_t           sSecond2;
    acp_sint64_t           sMicrosecond1;
    acp_sint64_t           sMicrosecond2;
    acp_bool_t             sNull1;
    acp_bool_t             sNull2;

    //---------
    // value1
    //---------
    sIntervalValue1 = (const mtdIntervalType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdInterval.staticNull );

    sSecond1      = sIntervalValue1->second;
    sMicrosecond1 = sIntervalValue1->microsecond;

    sNull1 = ( ( sSecond1 == mtcdIntervalNull.second ) &&
               ( sMicrosecond1 == mtcdIntervalNull.microsecond ) )
        ? ACP_TRUE : ACP_FALSE;

    //---------
    // value2
    //---------
    sIntervalValue2 = (const mtdIntervalType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdInterval.staticNull );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtcdIntervalNull.second ) &&
               ( sMicrosecond2 == mtcdIntervalNull.microsecond ) )
        ? ACP_TRUE : ACP_FALSE;

    //---------
    // compare
    //---------

    if( ( sNull1 == ACP_FALSE ) && ( sNull2 == ACP_FALSE ) )
    {
        if( sSecond1      > sSecond2     ) return  1;
        if( sSecond1      < sSecond2      ) return -1;
        if( sMicrosecond1 > sMicrosecond2 ) return  1;
        if( sMicrosecond1 < sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ACP_TRUE ) && ( sNull2 == ACP_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ACP_FALSE ) && ( sNull2 == ACP_TRUE ) )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdIntervalMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdIntervalType* sIntervalValue1;
    const mtdIntervalType* sIntervalValue2;
    acp_sint64_t           sSecond1;
    acp_sint64_t           sSecond2;
    acp_sint64_t           sMicrosecond1;
    acp_sint64_t           sMicrosecond2;
    acp_bool_t             sNull1;
    acp_bool_t             sNull2;

    //---------
    // value1
    //---------
    sIntervalValue1 = (const mtdIntervalType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdInterval.staticNull );

    sSecond1      = sIntervalValue1->second;
    sMicrosecond1 = sIntervalValue1->microsecond;

    sNull1 = ( ( sSecond1 == mtcdIntervalNull.second ) &&
               ( sMicrosecond1 == mtcdIntervalNull.microsecond ) )
        ? ACP_TRUE : ACP_FALSE;

    //---------
    // value2
    //---------
    sIntervalValue2 = (const mtdIntervalType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdInterval.staticNull );

    sSecond2      = sIntervalValue2->second;
    sMicrosecond2 = sIntervalValue2->microsecond;

    sNull2 = ( ( sSecond2 == mtcdIntervalNull.second ) &&
               ( sMicrosecond2 == mtcdIntervalNull.microsecond ) )
        ? ACP_TRUE : ACP_FALSE;

    //---------
    // compare
    //---------

    if( ( sNull1 == ACP_FALSE ) && ( sNull2 == ACP_FALSE ) )
    {
        if( sSecond1      < sSecond2      ) return  1;
        if( sSecond1      > sSecond2      ) return -1;
        if( sMicrosecond1 < sMicrosecond2 ) return  1;
        if( sMicrosecond1 > sMicrosecond2 ) return -1;
        return 0;
    }
    
    if( ( sNull1 == ACP_TRUE ) && ( sNull2 == ACP_FALSE ) )
    {
        return 1;
    }
    if( ( sNull1 == ACP_FALSE ) && ( sNull2 == ACP_TRUE ) )
    {
        return -1;
    }
    return 0;
}

    
void mtdEndian( void* aValue )
{
    acp_uint8_t* sValue;
    acp_uint8_t  sIntermediate;
    
    sValue = (acp_uint8_t*)&(((mtdIntervalType*)aValue)->second);

    sIntermediate = sValue[0];
    sValue[0]     = sValue[7];
    sValue[7]     = sIntermediate;
    sIntermediate = sValue[1];
    sValue[1]     = sValue[6];
    sValue[6]     = sIntermediate;
    sIntermediate = sValue[2];
    sValue[2]     = sValue[5];
    sValue[5]     = sIntermediate;
    sIntermediate = sValue[3];
    sValue[3]     = sValue[4];
    sValue[4]     = sIntermediate;
        
    sValue = (acp_uint8_t*)&(((mtdIntervalType*)aValue)->microsecond);

    sIntermediate = sValue[0];
    sValue[0]     = sValue[7];
    sValue[7]     = sIntermediate;
    sIntermediate = sValue[1];
    sValue[1]     = sValue[6];
    sValue[6]     = sIntermediate;
    sIntermediate = sValue[2];
    sValue[2]     = sValue[5];
    sValue[5]     = sIntermediate;
    sIntermediate = sValue[3];
    sValue[3]     = sValue[4];
    sValue[4]     = sIntermediate;
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

    ACI_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL);

    ACI_TEST_RAISE( aValueSize != sizeof(mtdIntervalType),
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdInterval,
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

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t    aColumnSize,
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

    mtdIntervalType *sValue;

    ACP_UNUSED(aDestValueOffset);
    
    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sValue = (mtdIntervalType*)aDestValue;

    if( aLength == 0 )
    {
        // NULL 데이타
        *sValue = mtcdIntervalNull;
    }
    else
    {
        ACI_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );

        acpMemCpy( sValue, aValue, aLength );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

