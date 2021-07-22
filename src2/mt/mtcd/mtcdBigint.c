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
 * $Id: mtdBigint.cpp 36567 2009-11-06 02:27:56Z sungminee $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

#define MTD_BIGINT_ALIGN    (sizeof(acp_uint64_t))
#define MTD_BIGINT_SIZE     (sizeof(mtdBigintType))
#define MTD_BIGINT_MAXIMUM1 ((mtdBigintType)ACP_SINT64_LITERAL(922337203685477580))
#define MTD_BIGINT_MAXIMUM2 ((mtdBigintType)ACP_SINT64_LITERAL(99999999999999999))
#define MTD_BIGINT_MINIMUM1 ((mtdBigintType)ACP_SINT64_LITERAL(-922337203685477580))
#define MTD_BIGINT_MINIMUM2 ((mtdBigintType)ACP_SINT64_LITERAL(-99999999999999999))

extern mtdModule mtcdBigint;

static mtdBigintType mtdBigintNull = MTD_BIGINT_NULL;

static ACI_RC mtdInitializeBigint( acp_uint32_t aNo );

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

static acp_sint32_t mtdBigintLogicalComp( mtdValueInfo* aValueInfo1,
                                          mtdValueInfo* aValueInfo2  );

static acp_sint32_t mtdBigintMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                               mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBigintMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBigintStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                  mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBigintStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                   mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBigintStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                     mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdBigintStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                                      mtdValueInfo* aValueInfo2 );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static acp_double_t mtdSelectivityBigint( void* aColumnMax,
                                          void* aColumnMin,
                                          void* aValueMax,
                                          void* aValueMin );

static ACI_RC mtdEncode( mtcColumn*    aColumn,
                         void*         aValue,
                         acp_uint32_t  aValueSize,
                         acp_uint8_t*  aCompileFmt,
                         acp_uint32_t  aCompileFmtLen,
                         acp_uint8_t*  aText,
                         acp_uint32_t* aTextLen,
                         ACI_RC*       aRet );

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static acp_uint32_t mtdNullValueSize();

static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"BIGINT" },
};

static mtcColumn mtdColumn;

mtdModule mtcdBigint = {
    mtdTypeName,
    &mtdColumn,
    MTD_BIGINT_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_BIGINT_ALIGN,
    MTD_GROUP_NUMBER|
    MTD_CANON_NEEDLESS|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_ENABLE|
    MTD_NUMBER_GROUP_TYPE_BIGINT|
    MTD_SEARCHABLE_PRED_BASIC|
    MTD_UNSIGNED_ATTR_TRUE|
    MTD_NUM_PREC_RADIX_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    19,
    0,
    0,
    &mtdBigintNull,
    mtdInitializeBigint,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdBigintLogicalComp,  // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdBigintMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdBigintMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdBigintStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdBigintStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdBigintStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdBigintStoredStoredKeyDescComp // Descending Key Comparison
        }
    },
    mtdCanonizeDefault,
    mtdEndian,
    mtdValidate,
    mtdSelectivityBigint,
    mtdEncode,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSizeDefault
};

ACI_RC mtdInitializeBigint( acp_uint32_t aNo )
{ 
    ACI_TEST( mtdInitializeModule( &mtcdBigint, aNo ) != ACI_SUCCESS );
    
    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdBigint,
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
/***********************************************************************
 *
 * Description : data type module의 semantic 검사 및 column size 설정 
 *
 * Implementation :
 *
 ***********************************************************************/

    ACP_UNUSED(aPrecision);
    ACP_UNUSED(aScale);
    
    ACI_TEST_RAISE( *aArguments != 0, ERR_INVALID_PRECISION );
 
    *aColumnSize = MTD_BIGINT_SIZE;
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_PRECISION );
    aciSetErrorCode(mtERR_ABORT_INVALID_PRECISION);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdValue( mtcTemplate*  aTemplate ,
                 mtcColumn*    aColumn ,
                 void*         aValue,
                 acp_uint32_t* aValueOffset,
                 acp_uint32_t  aValueSize,
                 const void*   aToken,
                 acp_uint32_t  aTokenLength,
                 ACI_RC*       aResult )
{
    acp_uint32_t       sValueOffset;
    mtdBigintType*     sValue;
    const acp_uint8_t* sToken;
    const acp_uint8_t* sFence;
    acp_sint64_t       sIntermediate;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_BIGINT_ALIGN );
    
    sToken = (const acp_uint8_t*)aToken;
    
    if( sValueOffset + MTD_BIGINT_SIZE <= aValueSize )
    {
        sValue = (mtdBigintType*)( (acp_uint8_t*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            *sValue = MTD_BIGINT_NULL;
        }
        else
        {
            sFence = sToken + aTokenLength;
            if( *sToken == '-' )
            {
                sToken++;
                ACI_TEST_RAISE( sToken >= sFence, ERR_INVALID_LITERAL );
                for( sIntermediate = 0; sToken < sFence; sToken++ )
                {
                    ACI_TEST_RAISE( *sToken < '0' || *sToken > '9',
                                    ERR_INVALID_LITERAL );
                    if( *sToken <= '7' )
                    {
                        ACI_TEST_RAISE( sIntermediate < MTD_BIGINT_MINIMUM1,
                                        ERR_VALUE_OVERFLOW );
                    }
                    else
                    {
                        ACI_TEST_RAISE( sIntermediate <=  MTD_BIGINT_MINIMUM1,
                                        ERR_VALUE_OVERFLOW );
                    }
                    sIntermediate = sIntermediate * 10 - ( *sToken - '0' );
                }
                *sValue = sIntermediate;
            }
            else
            {
                if( *sToken == '+' )
                {
                    sToken++;
                }
                ACI_TEST_RAISE( sToken >= sFence, ERR_INVALID_LITERAL );
                for( sIntermediate = 0; sToken < sFence; sToken++ )
                {
                    ACI_TEST_RAISE( *sToken < '0' || *sToken > '9',
                                    ERR_INVALID_LITERAL );
                    if( *sToken <= '7' )
                    {
                        ACI_TEST_RAISE( sIntermediate > MTD_BIGINT_MAXIMUM1,
                                        ERR_VALUE_OVERFLOW );
                    }
                    else
                    {
                        ACI_TEST_RAISE( sIntermediate >= MTD_BIGINT_MAXIMUM1,
                                        ERR_VALUE_OVERFLOW );
                    }
                    sIntermediate = sIntermediate * 10 + ( *sToken - '0' );
                }
                *sValue = sIntermediate;
            }
        }
       
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_BIGINT_SIZE;
        
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
    return (acp_uint32_t)MTD_BIGINT_SIZE;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdBigintType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdBigintType*)mtdValueForModule( NULL,
                                                aRow,
                                                aFlag,
                                                NULL );
    
    if( sValue != NULL )
    {
        *sValue = MTD_BIGINT_NULL;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBigintType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBigint.staticNull );

    return mtcHash( aHash, (const acp_uint8_t*)sValue, MTD_BIGINT_SIZE );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBigintType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBigint.staticNull );
    
    return *sValue == MTD_BIGINT_NULL ? ACP_TRUE : ACP_FALSE ;
}

acp_sint32_t
mtdBigintLogicalComp( mtdValueInfo* aValueInfo1,
                      mtdValueInfo* aValueInfo2 )
{
    return mtdBigintMtdMtdKeyAscComp( aValueInfo1, aValueInfo2 );
}

acp_sint32_t
mtdBigintMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                           mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBigintType* sValue1;
    const mtdBigintType* sValue2;

    //---------
    // value1
    //---------        
    sValue1 = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value, 
                           aValueInfo1->flag,
                           mtcdBigint.staticNull );
    
    //---------
    // value2
    //---------        
    sValue2 = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value, 
                           aValueInfo2->flag,
                           mtcdBigint.staticNull );
    
    //---------
    // compare 
    //---------
    
    if( *sValue1 != MTD_BIGINT_NULL && *sValue2 != MTD_BIGINT_NULL )
    {
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
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
    
}

acp_sint32_t
mtdBigintMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                            mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBigintType* sValue1;
    const mtdBigintType* sValue2;

    //---------
    // value1
    //---------        
    sValue1 = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value, 
                           aValueInfo1->flag,
                           mtcdBigint.staticNull );

    //---------
    // value2
    //---------        
    sValue2 = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value, 
                           aValueInfo2->flag,
                           mtcdBigint.staticNull );

    //---------
    // compare 
    //---------
    
    if( *sValue1 != MTD_BIGINT_NULL && *sValue2 != MTD_BIGINT_NULL )
    {
        if( *sValue1 < *sValue2 )
        {
            return 1;
        }
        if( *sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( *sValue1 < *sValue2 )
    {
        return 1;
    }
    if( *sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdBigintStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdBigintType        sValue1;
    const mtdBigintType* sValue2;

    //---------
    // value1
    //---------        
    MTC_LONG_BYTE_ASSIGN( &sValue1, aValueInfo1->value );
    

    //---------
    // value2
    //---------        
    sValue2 = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value, 
                           aValueInfo2->flag,
                           mtcdBigint.staticNull );
    //---------
    // compare
    //---------
    
    if( sValue1 != MTD_BIGINT_NULL && *sValue2 != MTD_BIGINT_NULL )
    {
        if( sValue1 > *sValue2 )
        {
            return 1;
        }
        if( sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < *sValue2 )
    {
        return 1;
    }
    if( sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
    
}

acp_sint32_t
mtdBigintStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                               mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdBigintType        sValue1;
    const mtdBigintType* sValue2;
    
    //---------
    // value1
    //---------        
    MTC_LONG_BYTE_ASSIGN( &sValue1, aValueInfo1->value );

    //---------
    // value2
    //---------        
    sValue2 = (const mtdBigintType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value, 
                           aValueInfo2->flag,
                           mtcdBigint.staticNull );
    
    //---------
    // compare
    //---------    
    
    if( sValue1 != MTD_BIGINT_NULL && *sValue2 != MTD_BIGINT_NULL )
    {
        if( sValue1 < *sValue2 )
        {
            return 1;
        }
        if( sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < *sValue2 )
    {
        return 1;
    }
    if( sValue1 > *sValue2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdBigintStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                 mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdBigintType sValue1;
    mtdBigintType sValue2;

    //---------
    // value1
    //---------
    MTC_LONG_BYTE_ASSIGN( &sValue1, aValueInfo1->value );
    
    //---------
    // value2
    //---------        
    MTC_LONG_BYTE_ASSIGN( &sValue2, aValueInfo2->value );
    
    //---------
    // compare
    //---------
    
    if( ( sValue1 != MTD_BIGINT_NULL ) && ( sValue2 != MTD_BIGINT_NULL ) )
    {
        if( sValue1 > sValue2 )
        {
            return 1;
        }
        if( sValue1 < sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < sValue2 )
    {
        return 1;
    }
    if( sValue1 > sValue2 )
    {
        return -1;
    }
    return 0;
    
}

acp_sint32_t
mtdBigintStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                  mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdBigintType sValue1;
    mtdBigintType sValue2;

    //---------
    // value1
    //---------
    MTC_LONG_BYTE_ASSIGN( &sValue1, aValueInfo1->value );
    
    //---------
    // value2
    //---------        
    MTC_LONG_BYTE_ASSIGN( &sValue2, aValueInfo2->value );
    
    //---------
    // compare
    //---------
    
    if( ( sValue1 != MTD_BIGINT_NULL ) && ( sValue2 != MTD_BIGINT_NULL ) )
    {
        if( sValue1 < sValue2 )
        {
            return 1;
        }
        if( sValue1 > sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( sValue1 < sValue2 )
    {
        return 1;
    }
    if( sValue1 > sValue2 )
    {
        return -1;
    }
    return 0;
}

void mtcdPartialKeyAscending( acp_uint64_t*    aPartialKey,
                             const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag )
{
    mtdBigintType sValue;

    ACP_UNUSED( aColumn);
    
    sValue = *(const mtdBigintType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBigint.staticNull );
    
    if( sValue > 0 )
    {
        *aPartialKey = (acp_uint64_t)sValue + ACP_UINT64_LITERAL(9223372036854775807);
    }
    else if( sValue != MTD_BIGINT_NULL )
    {
        *aPartialKey = sValue + ACP_UINT64_LITERAL(9223372036854775807);
    }
    else
    {
        *aPartialKey = MTD_PARTIAL_KEY_MAXIMUM;
    }
}

void mtdPartialKeyDescending( acp_uint64_t*    aPartialKey,
                              const mtcColumn* aColumn,
                              const void*      aRow,
                              acp_uint32_t     aFlag )
{
    mtdBigintType sValue;

    ACP_UNUSED( aColumn);
    
    sValue = *(const mtdBigintType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           mtcdBigint.staticNull );
    
    if( sValue > 0 )
    {
        *aPartialKey = ACP_UINT64_LITERAL(9223372036854775807) - (acp_uint64_t)sValue;
    }
    else if( sValue != MTD_BIGINT_NULL )
    {
        *aPartialKey = ACP_SINT64_LITERAL(9223372036854775807) + (acp_uint64_t)(-sValue);
    }
    else
    {
        *aPartialKey = MTD_PARTIAL_KEY_MAXIMUM;
    }
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sValue;
    acp_uint8_t  sIntermediate;
    
    sValue        = (acp_uint8_t*)aValue;
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
    
    ACI_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );
    
    ACI_TEST_RAISE( aValueSize != sizeof(mtdBigintType),
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdBigint,
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

acp_double_t mtdSelectivityBigint( void* aColumnMax, 
                                   void* aColumnMin, 
                                   void* aValueMax,  
                                   void* aValueMin )
{
/***********************************************************************
 *
 * Description :
 *    BIGINT의 Selectivity 추출 함수
 *
 * Implementation :
 *
 *    Selectivity = (aValueMax - aValueMin) / (aColumnMax - aColumnMin)
 *    0 < Selectivity <= 1 의 값을 리턴함
 *
 ***********************************************************************/
    
    mtdBigintType * sColumnMax;
    mtdBigintType * sColumnMin;
    mtdBigintType * sValueMax;
    mtdBigintType * sValueMin;
    acp_double_t    sSelectivity;
    acp_double_t    sDenominator;  // 분모값
    acp_double_t    sNumerator;    // 분자값
    mtdValueInfo    sValueInfo1;
    mtdValueInfo    sValueInfo2;
    mtdValueInfo    sValueInfo3;
    mtdValueInfo    sValueInfo4;
    
    sColumnMax = (mtdBigintType*) aColumnMax;
    sColumnMin = (mtdBigintType*) aColumnMin;
    sValueMax  = (mtdBigintType*) aValueMax;
    sValueMin  = (mtdBigintType*) aValueMin;

    //------------------------------------------------------
    // Data의 유효성 검사
    //     NULL 검사 : 계산할 수 없음
    //------------------------------------------------------

    if ( ( mtdIsNull( NULL, aColumnMax, MTD_OFFSET_USELESS ) == ACP_TRUE ) ||
         ( mtdIsNull( NULL, aColumnMin, MTD_OFFSET_USELESS ) == ACP_TRUE ) ||
         ( mtdIsNull( NULL, aValueMax, MTD_OFFSET_USELESS ) == ACP_TRUE )  ||
         ( mtdIsNull( NULL, aValueMin, MTD_OFFSET_USELESS ) == ACP_TRUE ) )
    {
        // Data중 NULL 이 있을 경우
        // 부등호의 Default Selectivity인 1/3을 Setting함
        sSelectivity = MTD_DEFAULT_SELECTIVITY;
    }
    else
    {
        //------------------------------------------------------
        // 유효성 검사
        // 다음의 경우는 조건을 잘못된 통계 정보이거나 입력 정보임.
        // Column의 Min값보다 Value의 Max값이 작은 경우
        // Column의 Max값보다 Value의 Min값이 큰 경우
        //------------------------------------------------------

        sValueInfo1.column = NULL;
        sValueInfo1.value  = sColumnMin;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = sValueMax;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sValueInfo3.column = NULL;
        sValueInfo3.value  = sColumnMax;
        sValueInfo3.flag   = MTD_OFFSET_USELESS;

        sValueInfo4.column = NULL;
        sValueInfo4.value  = sValueMin;
        sValueInfo4.flag   = MTD_OFFSET_USELESS;
        
        if ( ( mtdBigintLogicalComp( &sValueInfo1,
                                     &sValueInfo2 ) > 0 )
             ||
             ( mtdBigintLogicalComp( &sValueInfo3,
                                     &sValueInfo4 ) < 0 ) )
        {
            // To Fix PR-11858
            sSelectivity = 0;
        }
        else
        {
            //------------------------------------------------------
            // Value값 보정
            // Value의 Min값이 Column의 Min값보다 작다면 보정
            // Value의 Max값이 Column의 Max값보다 크다면 보정
            //------------------------------------------------------

            sValueInfo1.column = NULL;
            sValueInfo1.value  = sValueMin;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = sColumnMin;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( mtdBigintLogicalComp( &sValueInfo1,
                                       &sValueInfo2 ) < 0 )
            {
                sValueMin = sColumnMin;
            }
            else
            {
                // Nothing To Do
            }
            
            sValueInfo1.column = NULL;
            sValueInfo1.value  = sValueMax;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = sColumnMax;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( mtdBigintLogicalComp( &sValueInfo1,
                                       &sValueInfo2 ) > 0 )
            {
                sValueMax = sColumnMax;
            }
            else
            {
                // Nothing To Do
            }

            //------------------------------------------------------
            // 분모값 (aColumnMax - aColumnMin) 값 획득
            //------------------------------------------------------
        
            sDenominator = (acp_double_t)(*sColumnMax - *sColumnMin);
    
            if ( sDenominator <= 0.0 )
            {
                // 잘못된 통계 정보의 사용한 경우
                sSelectivity = MTD_DEFAULT_SELECTIVITY;
            }
            else
            {
                //------------------------------------------------------
                // 분자값 (aValueMax - aValueMin) 값 획득
                //------------------------------------------------------
            
                sNumerator = (acp_double_t) (*sValueMax - *sValueMin);
            
                if ( sNumerator <= 0.0 )
                {
                    // 잘못된 입력 정보인 경우
                    // To Fix PR-11858
                    sSelectivity = 0;
                }
                else
                {
                    //------------------------------------------------------
                    // Selectivity 계산
                    //------------------------------------------------------
                
                    sSelectivity = sNumerator / sDenominator;

                    if ( sSelectivity > 1.0 )
                    {
                        sSelectivity = 1.0;
                    }
                }
            }
        }
    }
    
    return sSelectivity;
}


ACI_RC mtdEncode( mtcColumn*    aColumn ,
                  void*         aValue,
                  acp_uint32_t  aValueSize ,
                  acp_uint8_t*  aCompileFmt ,
                  acp_uint32_t  aCompileFmtLen,
                  acp_uint8_t*  aText,
                  acp_uint32_t* aTextLen,
                  ACI_RC*       aRet )
{
    acp_uint32_t sStringLen;

    ACP_UNUSED(aColumn);
    ACP_UNUSED(aValueSize);
    ACP_UNUSED(aCompileFmt);
    ACP_UNUSED(aCompileFmtLen);
    
    //----------------------------------
    // Parameter Validation
    //----------------------------------

    ACE_ASSERT( aValue != NULL );
    ACE_ASSERT( aText != NULL );
    ACE_ASSERT( *aTextLen > 0 );
    ACE_ASSERT( aRet != NULL );
    
    //----------------------------------
    // Initialization
    //----------------------------------

    aText[0] = '\0';
    sStringLen = 0;

    //----------------------------------
    // Set String
    //----------------------------------
    
    // To Fix BUG-16801
    if ( mtdIsNull( NULL, aValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sStringLen = aciVaAppendFormat( (acp_char_t*) aText,
                                        (acp_size_t) aTextLen,
                                        "%"ACI_INT64_FMT,
                                        *(mtdBigintType*)aValue );

    }

    //----------------------------------
    // Finalization
    //----------------------------------
    
    aText[sStringLen] = '\0';
    *aTextLen = sStringLen;
    
    *aRet = ACI_SUCCESS;
    
    return ACI_SUCCESS;
}

ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                void*        aDestValue,
                                acp_uint32_t aDestValueOffset ,
                                acp_uint32_t aLength,
                                const void*  aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdBigintType* sBigintValue;

    ACP_UNUSED(aDestValueOffset);
    
    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다. 

    sBigintValue = (mtdBigintType*)aDestValue;
    
    if( aLength == 0 )
    {
        // NULL 데이타
        *sBigintValue = mtdBigintNull;        
    }
    else
    {
        ACI_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );        

        acpMemCpy( sBigintValue, aValue, aLength );
    }

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
/*******************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 *******************************************************************/

    return mtdActualSize( NULL,
                          & mtdBigintNull,
                          MTD_OFFSET_USELESS );
}

