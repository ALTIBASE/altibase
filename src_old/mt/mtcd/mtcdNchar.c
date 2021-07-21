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
 * $Id: mtdVarchar.cpp 21249 2007-04-06 01:39:26Z leekmo $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>
#include <mtcdTypes.h>

extern mtdModule mtcdNvarchar;
extern mtdModule mtcdNchar;

extern mtlModule mtclUTF16;

extern mtlModule* mtlDBCharSet;
extern mtlModule* mtlNationalCharSet;

const mtdNcharType mtcdNcharNull = { 0, {'\0',} };

static ACI_RC mtdInitializeNchar( acp_uint32_t aNo );

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

static acp_sint32_t mtdNcharLogicalComp( mtdValueInfo* aValueInfo1,
                                         mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNcharMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                              mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNcharMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                               mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNcharStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                 mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNcharStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                  mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNcharStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                    mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNcharStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                                     mtdValueInfo* aValueInfo2 );

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonized,
                           mtcEncryptInfo*  aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate );

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

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void*             aDestValue,
                                       acp_uint32_t      aDestValueOffset,
                                       acp_uint32_t      aLength,
                                       const void*       aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 5, (void*)"NCHAR"  }
};

static mtcColumn mtdColumn;

mtdModule mtcdNchar = {
    mtdTypeName,
    &mtdColumn,
    MTD_NCHAR_ID,
    0,
    { /*SMI_BUILTIN_B_TREE_INDEXTYPE_ID*/0,
      /*SMI_BUILTIN_B_TREE2_INDEXTYPE_ID*/0,
      0, 0, 0, 0, 0 },
    MTD_NCHAR_ALIGN,
    MTD_GROUP_TEXT|
    MTD_CANON_NEED_WITH_ALLOCATION|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_DISABLE|
    MTD_CREATE_PARAM_PRECISION|
    MTD_CASE_SENS_TRUE|
    MTD_SEARCHABLE_SEARCHABLE|
    MTD_LITERAL_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    32000,
    0,
    0,
    (void*)&mtcdNcharNull,
    mtdInitializeNchar,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdNcharLogicalComp,    // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdNcharMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdNcharMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdNcharStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdNcharStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdNcharStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdNcharStoredStoredKeyDescComp // Descending Key Comparison
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtdSelectivityDefault,
    mtdEncodeCharDefault,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracle,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSize
};

ACI_RC mtdInitializeNchar( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdNchar, aNo )
              != ACI_SUCCESS );
    
    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdNchar,
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
    ACP_UNUSED(aScale);
    
    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_NCHAR_PRECISION_DEFAULT;
    }

    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    if( mtlNationalCharSet->id == MTL_UTF8_ID )
    {
        ACI_TEST_RAISE( (*aPrecision < (acp_sint32_t)MTD_NCHAR_PRECISION_MINIMUM) ||
                        (*aPrecision > (acp_sint32_t)MTD_UTF8_NCHAR_PRECISION_MAXIMUM),
                        ERR_INVALID_LENGTH );

        *aColumnSize = sizeof(acp_uint16_t) + (*aPrecision * MTL_UTF8_PRECISION);
    }
    else if( mtlNationalCharSet->id == MTL_UTF16_ID )
    {
        ACI_TEST_RAISE( (*aPrecision < (acp_sint32_t)MTD_NCHAR_PRECISION_MINIMUM) ||
                        (*aPrecision > (acp_sint32_t)MTD_UTF16_NCHAR_PRECISION_MAXIMUM),
                        ERR_INVALID_LENGTH );

        *aColumnSize = sizeof(acp_uint16_t) + (*aPrecision * MTL_UTF16_PRECISION);
    }
    else
    {
        ACE_ASSERT( 0 );
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);
    
    ACI_EXCEPTION( ERR_INVALID_SCALE );
    aciSetErrorCode(mtERR_ABORT_INVALID_SCALE);
    
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
/***********************************************************************
 *
 * Description : PROJ-1579 NCHAR
 *
 * Implementation :
 *      NCHAR 리터럴, 유니코드 리터럴, 일반 리터럴인지에 따라
 *      다르게 value를 구한다.
 *
 ***********************************************************************/

    acp_uint32_t           sValueOffset;
    mtdNcharType*          sValue;
    acp_uint8_t*           sToken;
    acp_uint8_t*           sTempToken;
    acp_uint8_t*           sTokenFence;
    acp_uint8_t*           sIterator;
    acp_uint8_t*           sFence;
    const mtlModule*       sColLanguage;
    const mtlModule*       sTokenLanguage;
    acp_bool_t             sIsSame     = ACP_FALSE;
    acp_uint32_t           sNcharCnt   = 0;
    aciConvCharSetList     sColCharSet;
    aciConvCharSetList     sTokenCharSet;
    acp_sint32_t           sSrcRemain  = 0;
    acp_sint32_t           sDestRemain = 0;
    acp_sint32_t           sTempRemain = 0;
    acp_uint8_t            sSize;
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_NCHAR_ALIGN );
    sValue = (mtdNcharType*)( (acp_uint8_t*)aValue + sValueOffset );

    *aResult = ACI_SUCCESS;

    // -------------------------------------------------------------------
    // N'안'과 같이 NCHAR 리터럴을 사용한 경우,
    // NLS_NCHAR_LITERAL_REPLACE가 TRUE일 경우에는 서버로
    // 클라이언트 캐릭터 셋이 그대로 전송된다.
    //
    // 따라서 클라이언트가 전송해준 환경 변수를 보고 SrcCharSet을 결정한다.
    // 1. TRUE:  클라이언트 캐릭터 셋   =>  내셔널 캐릭터 셋
    // 2. FALSE: 데이터베이스 캐릭터 셋 =>  내셔널 캐릭터 셋
    //
    // 파싱 단계에서 이 환경 변수가 TRUE일 경우에만 NCHAR 리터럴로 처리되므로 
    // 여기서 환경 변수를 다시 체크할 필요는 없다.
    // 
    // aTemplate->nlsUse는 클라이언트 캐릭터 셋이다.(ALTIBASE_NLS_USE)
    // -------------------------------------------------------------------

    sColLanguage = aColumn->language;

    if( (aColumn->flag & MTC_COLUMN_LITERAL_TYPE_MASK) ==
        MTC_COLUMN_LITERAL_TYPE_NCHAR )
    {
        sTokenLanguage = aTemplate->nlsUse;
    }
    else if( (aColumn->flag & MTC_COLUMN_LITERAL_TYPE_MASK) ==
             MTC_COLUMN_LITERAL_TYPE_UNICODE )
    {
        sTokenLanguage = mtlDBCharSet;
    }
    else
    {
        sTokenLanguage = mtlDBCharSet;
    }

    // To fix BUG-13444
    // tokenFence와 RowFence는 별개의 검사과정이므로,
    // 먼저 RowFence검사 후 TokenFence검사를 해야 한다.
    sIterator = sValue->value;
    sFence    = (acp_uint8_t*)aValue + aValueSize;

    if( sIterator >= sFence )
    {
        *aResult = ACI_FAILURE;
    }
    else
    {
        sToken = (acp_uint8_t*)aToken;
        sTokenFence = sToken + aTokenLength;

        if( (aColumn->flag & MTC_COLUMN_LITERAL_TYPE_MASK) ==
            MTC_COLUMN_LITERAL_TYPE_UNICODE )
        {
            ACI_TEST( mtdNcharInterfaceToNchar4UnicodeLiteral( sTokenLanguage,
                                                               sColLanguage,
                                                               sToken,
                                                               sTokenFence,
                                                               &sIterator,
                                                               sFence,
                                                               & sNcharCnt )
                      != ACI_SUCCESS );
        }
        else
        {
            sColCharSet = mtlGetIdnCharSet( sColLanguage );
            sTokenCharSet = mtlGetIdnCharSet( sTokenLanguage );

            sSrcRemain = aTokenLength;
            sDestRemain = aValueSize;

            while( sToken < sTokenFence )
            {
                if( sIterator >= sFence )
                {
                    *aResult = ACI_FAILURE;
                    break;
                }

                sSize = mtlGetOneCharSize( sToken,
                                           sTokenFence,
                                           sTokenLanguage );
                
                sIsSame = mtcCompareOneChar( sToken,
                                             sSize,
                                             sTokenLanguage->specialCharSet[MTL_QT_IDX],
                                             sTokenLanguage->specialCharSize );
                
                if( sIsSame == ACP_TRUE )
                {
                    (void)sTokenLanguage->nextCharPtr( & sToken, sTokenFence );
                    
                    sSize = mtlGetOneCharSize( sToken,
                                               sTokenFence,
                                               sTokenLanguage );
                    
                    sIsSame = mtcCompareOneChar( sToken,
                                                 sSize,
                                                 sTokenLanguage->specialCharSet[MTL_QT_IDX],
                                                 sTokenLanguage->specialCharSize );
                
                    ACI_TEST_RAISE( sIsSame != ACP_TRUE,
                                    ERR_INVALID_LITERAL );
                }

                sTempRemain = sDestRemain;

                if( sColLanguage->id != sTokenLanguage->id )
                {
                    ACI_TEST( aciConvConvertCharSet( sTokenCharSet,
                                                     sColCharSet,
                                                     sToken,
                                                     sSrcRemain,
                                                     sIterator,
                                                     & sDestRemain,
                                                     0 )
                              != ACI_SUCCESS );

                    sTempToken = sToken;

                    (void)sTokenLanguage->nextCharPtr( & sToken, sTokenFence );
                }
                else
                {
                    sTempToken = sToken;

                    (void)sTokenLanguage->nextCharPtr( & sToken, sTokenFence );

                    acpMemCpy( sIterator, sTempToken, sToken - sTempToken );

                    sDestRemain -= (sToken - sTempToken);
                }

                if( sTempRemain - sDestRemain > 0 )
                {
                    sIterator += (sTempRemain - sDestRemain);
                    sNcharCnt++;
                }
                else
                {
                    // Nothing to do.
                }
                
                sSrcRemain -= ( sToken - sTempToken);
            }
        }
    }
    
    if( *aResult == ACI_SUCCESS )
    {
        sValue->length     = sIterator - sValue->value;

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag      = 1;

        aColumn->precision = sValue->length != 0 ? sNcharCnt : 1;

        aColumn->scale     = 0;

        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != ACI_SUCCESS );

        aColumn->column.offset   = sValueOffset;

        *aValueOffset = sValueOffset + sizeof(acp_uint16_t) +
            sValue->length;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);

    ACI_EXCEPTION_END;

    *aResult = ACI_FAILURE;

    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                            const void*      aRow,
                            acp_uint32_t     aFlag )
{
    const mtdNcharType * sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNchar.staticNull );

    return sizeof(acp_uint16_t) + sValue->length;
}

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale )
{
    const mtlModule*    sLanguage;
    const mtdNcharType* sValue;
    acp_uint8_t*        sValueIndex;
    acp_uint8_t*        sValueFence;
    acp_uint16_t        sValueCharCnt = 0;
    
    sValue = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNchar.staticNull );

    sLanguage = aColumn->language;

    // --------------------------
    // Value의 문자 개수
    // --------------------------
    
    sValueIndex = (acp_uint8_t*) sValue->value;
    sValueFence = sValueIndex + sValue->length;

    while ( sValueIndex < sValueFence )
    {
        ACI_TEST_RAISE( sLanguage->nextCharPtr( & sValueIndex, sValueFence )
                        != NC_VALID,
                        ERR_INVALID_CHARACTER );

        sValueCharCnt++;
    }

    *aPrecision = sValueCharCnt;
    *aScale = 0;
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_CHARACTER );
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_CHARACTER);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdNcharType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdNcharType*)
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
    const mtdNcharType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNchar.staticNull );

    return mtcHashWithoutSpace( aHash, sValue->value, sValue->length );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdNcharType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdNchar.staticNull );

    return (sValue->length == 0) ? ACP_TRUE : ACP_FALSE;
}

acp_sint32_t
mtdNcharLogicalComp( mtdValueInfo* aValueInfo1,
                     mtdValueInfo* aValueInfo2 )
{
    return mtdNcharMtdMtdKeyAscComp( aValueInfo1,
                                     aValueInfo2);
}

acp_sint32_t
mtdNcharMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                          mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType* sNcharValue1;
    const mtdNcharType* sNcharValue2;
    acp_uint16_t        sLength1 = 0;
    acp_uint16_t        sLength2 = 0;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;
    acp_sint32_t        sCompared;
    const acp_uint8_t*  sIterator;
    const acp_uint8_t*  sFence;
    acp_bool_t          sIsSame;

    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNchar.staticNull );

    sLength1 = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNchar.staticNull );
    
    sLength2 = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtlNationalCharSet->id == MTL_UTF16_ID )
            {
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    sIsSame = mtcCompareOneChar(
                        (acp_uint8_t*)sIterator,
                        MTL_UTF16_PRECISION,
                        mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                        mtlNationalCharSet->specialCharSize );

                    if( sIsSame == ACP_FALSE )
                    {
                        return 1;
                    }
                }
            }
            else
            {
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if( *sIterator != ' ' )
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
        sCompared = acpMemCmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtlNationalCharSet->id == MTL_UTF16_ID )
        {
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                sIsSame = mtcCompareOneChar(
                    (acp_uint8_t*)sIterator,
                    MTL_UTF16_PRECISION,
                    mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                    mtlNationalCharSet->specialCharSize );
                        
                if( sIsSame == ACP_FALSE )
                {
                    return -1;
                }
            }
        }
        else
        {
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return -1;
                }
            }
        }
        return 0;
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNcharMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                           mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType* sNcharValue1;
    const mtdNcharType* sNcharValue2;
    acp_uint16_t        sLength1 = 0;
    acp_uint16_t        sLength2 = 0;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;
    acp_sint32_t        sCompared;
    const acp_uint8_t*  sIterator;
    const acp_uint8_t*  sFence;
    acp_bool_t          sIsSame;

    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNchar.staticNull );

    sLength1 = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNchar.staticNull );
    
    sLength2 = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtlNationalCharSet->id == MTL_UTF16_ID )
            {
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    sIsSame = mtcCompareOneChar(
                        (acp_uint8_t*)sIterator,
                        MTL_UTF16_PRECISION,
                        mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                        mtlNationalCharSet->specialCharSize );
                    if( sIsSame == ACP_FALSE )
                    {
                        return 1;
                    }
                }
            }
            else
            {
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if( *sIterator != ' ' )
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
        sCompared = acpMemCmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtlNationalCharSet->id == MTL_UTF16_ID )
        {
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                sIsSame = mtcCompareOneChar(
                    (acp_uint8_t*)sIterator,
                    MTL_UTF16_PRECISION,
                    mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                    mtlNationalCharSet->specialCharSize );

                if( sIsSame == ACP_FALSE )
                {
                    return -1;
                }
            }
        }
        else
        {
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return -1;
                }
            }
        }
        return 0;
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNcharStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                             mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType*        sNcharValue2;
    acp_uint16_t               sLength1 = 0;
    acp_uint16_t               sLength2 = 0;
    const acp_uint8_t*         sValue1;
    const acp_uint8_t*         sValue2;
    acp_sint32_t               sCompared;
    const acp_uint8_t*         sIterator;
    const acp_uint8_t*         sFence;
    acp_bool_t                 sIsSame;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint16_t)aValueInfo1->length;
    
    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNchar.staticNull );

    sLength2 = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (acp_uint8_t*)aValueInfo1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtlNationalCharSet->id == MTL_UTF16_ID )
            {
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    sIsSame = mtcCompareOneChar(
                        (acp_uint8_t*)sIterator,
                        MTL_UTF16_PRECISION,
                        mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                        mtlNationalCharSet->specialCharSize );

                    if( sIsSame == ACP_FALSE )
                    {
                        return 1;
                    }
                }
            }
            else
            {
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if( *sIterator != ' ' )
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
        sCompared = acpMemCmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtlNationalCharSet->id == MTL_UTF16_ID )
        {
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                sIsSame = mtcCompareOneChar(
                    (acp_uint8_t*)sIterator,
                    MTL_UTF16_PRECISION,
                    mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                    mtlNationalCharSet->specialCharSize );
                        
                if( sIsSame == ACP_FALSE )
                {
                    return -1;
                }
            }
        }
        else
        {
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return -1;
                }
            }
        }
        return 0;
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNcharStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType* sNcharValue2;
    acp_uint16_t        sLength1 = 0;
    acp_uint16_t        sLength2 = 0;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;
    acp_sint32_t        sCompared;
    const acp_uint8_t*  sIterator;
    const acp_uint8_t*  sFence;
    acp_bool_t          sIsSame;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint16_t)aValueInfo1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNchar.staticNull );

    sLength2 = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (acp_uint8_t*)aValueInfo1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtlNationalCharSet->id == MTL_UTF16_ID )
            {
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    sIsSame = mtcCompareOneChar(
                        (acp_uint8_t*)sIterator,
                        MTL_UTF16_PRECISION,
                        mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                        mtlNationalCharSet->specialCharSize );

                    if( sIsSame == ACP_FALSE )
                    {
                        return 1;
                    }
                }
            }
            else
            {
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if( *sIterator != ' ' )
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
        sCompared = acpMemCmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtlNationalCharSet->id == MTL_UTF16_ID )
        {
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                sIsSame = mtcCompareOneChar(
                    (acp_uint8_t*)sIterator,
                    MTL_UTF16_PRECISION,
                    mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                    mtlNationalCharSet->specialCharSize );
                        
                if( sIsSame == ACP_FALSE )
                {
                    return -1;
                }
            }
        }
        else
        {
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return -1;
                }
            }
        }
        return 0;
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNcharStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t               sLength1 = 0;
    acp_uint16_t               sLength2 = 0;
    const acp_uint8_t*         sValue1;
    const acp_uint8_t*         sValue2;
    acp_sint32_t               sCompared;
    const acp_uint8_t*         sIterator;
    const acp_uint8_t*         sFence;
    acp_bool_t                 sIsSame;

    //---------
    // value1
    //---------
    sLength1 = (acp_uint16_t)aValueInfo1->length;

    //---------
    // value2
    //---------
    sLength2 = (acp_uint16_t)aValueInfo2->length;

    //---------
    // compare
    //---------
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (acp_uint8_t*)aValueInfo1->value;
        sValue2  = (acp_uint8_t*)aValueInfo2->value;

        if( sLength1 >= sLength2 )
        {
            sCompared = acpMemCmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtlNationalCharSet->id == MTL_UTF16_ID )
            {
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    sIsSame = mtcCompareOneChar(
                        (acp_uint8_t*)sIterator,
                        MTL_UTF16_PRECISION,
                        mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                        mtlNationalCharSet->specialCharSize );

                    if( sIsSame == ACP_FALSE )
                    {
                        return 1;
                    }
                }
            }
            else
            {
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if( *sIterator != ' ' )
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
        sCompared = acpMemCmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtlNationalCharSet->id == MTL_UTF16_ID )
        {
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                sIsSame = mtcCompareOneChar(
                    (acp_uint8_t*)sIterator,
                    MTL_UTF16_PRECISION,
                    mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                    mtlNationalCharSet->specialCharSize );

                if( sIsSame == ACP_FALSE )
                {
                    return -1;
                }
            }
        }
        else
        {
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return -1;
                }
            }
        }
        return 0;
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdNcharStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                 mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t               sLength1 = 0;
    acp_uint16_t               sLength2 = 0;
    const acp_uint8_t        * sValue1;
    const acp_uint8_t        * sValue2;
    acp_sint32_t               sCompared;
    const acp_uint8_t        * sIterator;
    const acp_uint8_t        * sFence;
    acp_bool_t                 sIsSame;

    //---------
    // value1
    //---------    
    sLength1 = (acp_uint16_t)aValueInfo1->length;

    //---------
    // value2
    //---------    
    sLength2 = (acp_uint16_t)aValueInfo2->length;

    //---------
    // compare
    //---------    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (acp_uint8_t*)aValueInfo1->value;
        sValue2  = (acp_uint8_t*)aValueInfo2->value;

        if( sLength2 >= sLength1 )
        {
            sCompared = acpMemCmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtlNationalCharSet->id == MTL_UTF16_ID )
            {
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    sIsSame = mtcCompareOneChar(
                        (acp_uint8_t*)sIterator,
                        MTL_UTF16_PRECISION,
                        mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                        mtlNationalCharSet->specialCharSize );
                                            
                    if( sIsSame == ACP_FALSE )
                    {
                        return 1;
                    }
                }
            }
            else
            {
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if( *sIterator != ' ' )
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
        sCompared = acpMemCmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtlNationalCharSet->id == MTL_UTF16_ID )
        {
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                sIsSame = mtcCompareOneChar(
                    (acp_uint8_t*)sIterator,
                    MTL_UTF16_PRECISION,
                    mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                    mtlNationalCharSet->specialCharSize );
                        
                if( sIsSame == ACP_FALSE )
                {
                    return -1;
                }
            }
        }
        else
        {
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return -1;
                }
            }
        }
        return 0;
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonized,
                           mtcEncryptInfo*  aCanonInfo ,
                           const mtcColumn* aColumn ,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo ,
                           mtcTemplate*     aTemplate  )
{
    const mtlModule*        sLanguage;
    mtdNcharType*           sCanonized;
    mtdNcharType*           sValue;
    //acp_uint16_t            sCanonBytePrecision;
    acp_uint8_t           * sValueIndex;
    acp_uint8_t           * sValueFence;
    acp_uint16_t            sValueCharCnt = 0;
    acp_uint16_t            i = 0;
    
    ACP_UNUSED( aCanonInfo);
    ACP_UNUSED( aColumn);
    ACP_UNUSED( aColumnInfo);
    ACP_UNUSED( aTemplate);
    
    sValue = (mtdNcharType*)aValue;

    sLanguage = aCanon->language;

    //sCanonBytePrecision = sLanguage->maxPrecision(aCanon->precision);

    // --------------------------
    // Value의 문자 개수
    // --------------------------
    sValueIndex = sValue->value;
    sValueFence = sValueIndex + sValue->length;

    while( sValueIndex < sValueFence )
    {
        (void)sLanguage->nextCharPtr( & sValueIndex, sValueFence );
        sValueCharCnt++;
    }

    if( ( sValueCharCnt == aCanon->precision ) || (sValue->length == 0) )
    {
        *aCanonized = aValue;
    }
    else
    {
        if( mtlNationalCharSet->id == MTL_UTF16_ID )
        {
            ACI_TEST_RAISE ( sValueCharCnt > aCanon->precision,
                             ERR_INVALID_LENGTH );

            sCanonized = (mtdNcharType*)*aCanonized;
            
            sCanonized->length = sValue->length +
                ( (aCanon->precision - sValueCharCnt) * 
                  MTL_UTF16_PRECISION );
            
            acpMemCpy( sCanonized->value, 
                       sValue->value, 
                       sValue->length );
            
            for( i = 0; 
                 i < (sCanonized->length - sValue->length);
                 i += MTL_UTF16_PRECISION )
            {
                acpMemCpy( sCanonized->value + sValue->length + i,
                           mtlNationalCharSet->specialCharSet[MTL_SP_IDX],
                           MTL_UTF16_PRECISION );
            }
        }
        else
        {
            ACI_TEST_RAISE ( sValueCharCnt > aCanon->precision,
                             ERR_INVALID_LENGTH );

            sCanonized = (mtdNcharType*)*aCanonized;
            
            sCanonized->length = sValue->length +
                ( (aCanon->precision - sValueCharCnt) ); 
            
            acpMemCpy( sCanonized->value, 
                       sValue->value, 
                       sValue->length );
            
            acpMemSet( sCanonized->value + sValue->length,
                       ' ',
                       sCanonized->length - sValue->length );
        }
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);

    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;

    sLength = (acp_uint8_t*)&(((mtdNcharType*)aValue)->length);

    sIntermediate = sLength[0];
    sLength[0]    = sLength[1];
    sLength[1]    = sIntermediate;
}

ACI_RC mtdValidate( mtcColumn*    aColumn,
                    void*         aValue,
                    acp_uint32_t  aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
        
    mtdNcharType * sCharVal = (mtdNcharType*)aValue;
    ACI_TEST_RAISE( sCharVal == NULL, ERR_INVALID_NULL );

    ACI_TEST_RAISE( aValueSize < sizeof(acp_uint16_t), ERR_INVALID_LENGTH);
    ACI_TEST_RAISE( sCharVal->length + sizeof(acp_uint16_t) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdNchar,
                                   1,                // arguments
                                   sCharVal->length, // precision
                                   0 )               // scale
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

ACI_RC mtdValueFromOracle( mtcColumn*    aColumn,
                           void*         aValue,
                           acp_uint32_t* aValueOffset,
                           acp_uint32_t  aValueSize,
                           const void*   aOracleValue,
                           acp_sint32_t  aOracleLength,
                           ACI_RC*       aResult )
{
    acp_uint32_t  sValueOffset;
    mtdNcharType* sValue;
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_NCHAR_ALIGN );
    
    if( aOracleLength < 0 )
    {
        aOracleLength = 0;
    }

    // aColumn의 초기화
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdNchar,
                                   1,
                                   ( aOracleLength > 1 ) ? aOracleLength : 1,
                                   0 )
              != ACI_SUCCESS );
    
    if( sValueOffset + aColumn->column.size <= aValueSize )
    {
        sValue = (mtdNcharType*)((acp_uint8_t*)aValue+sValueOffset);
        
        sValue->length = aOracleLength;
        
        acpMemCpy( sValue->value, aOracleValue, aOracleLength );
        
        aColumn->column.offset = sValueOffset;
        
        *aValueOffset = sValueOffset + aColumn->column.size;
        
        *aResult = ACI_SUCCESS;
    }
    else
    {
        *aResult = ACI_FAILURE;
    }

    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void*             aDestValue,
                                       acp_uint32_t      aDestValueOffset,
                                       acp_uint32_t      aLength,
                                       const void*       aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdNcharType* sNcharValue;

    sNcharValue = (mtdNcharType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sNcharValue->length = 0;
    }
    else
    {
        ACI_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sNcharValue->length = (acp_uint16_t)(aDestValueOffset + aLength);
        acpMemCpy( (acp_uint8_t*)sNcharValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
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
 * 예 ) mtdNcharType ( acp_uint16_t length; acp_uint8_t value[1] ) 에서
 *      length타입인 acp_uint16_t의 크기를 반환
 *******************************************************************/

    return mtdActualSize( NULL,
                          & mtcdNcharNull,
                          MTD_OFFSET_USELESS );   
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdNcharType ( acp_uint16_t length; acp_uint8_t value[1] ) 에서
 *      length타입인 acp_uint16_t의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint16_t);
}

ACI_RC mtdNcharInterfaceToNchar( mtcStack*        aStack,
                                 const mtlModule* aSrcCharSet,
                                 const mtlModule* aDestCharSet,
                                 mtdCharType*     aSource,
                                 mtdNcharType*    aResult )
{
/***********************************************************************
 *
 * Description : 
 *      CHAR 타입을 NCHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    aciConvCharSetList   sIdnDestCharSet;
    aciConvCharSetList   sIdnSrcCharSet;
    acp_uint8_t*         sSourceIndex;
    acp_uint8_t*         sTempIndex;
    acp_uint8_t*         sSourceFence;
    acp_uint8_t*         sResultValue;
    acp_uint8_t*         sResultFence;
    acp_uint32_t         sNcharCnt = 0;
    acp_sint32_t         sSrcRemain = 0;
    acp_sint32_t         sDestRemain = 0;
    acp_sint32_t         sTempRemain = 0;

    // 변환 결과의 크기를 체크하기 위함
    sDestRemain = aDestCharSet->maxPrecision(aStack[0].column->precision);

    sSourceIndex = aSource->value;
    sSrcRemain   = aSource->length;
    sSourceFence = sSourceIndex + aSource->length;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtlGetIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtlGetIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            ACI_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            ACI_TEST( aciConvConvertCharSet( sIdnSrcCharSet,
                                             sIdnDestCharSet,
                                             sSourceIndex,
                                             sSrcRemain,
                                             sResultValue,
                                             & sDestRemain,
                                             0 )
                      != ACI_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );

            if( sTempRemain - sDestRemain > 0 )
            {
                sResultValue += (sTempRemain - sDestRemain);
                sNcharCnt++;

            }
            else
            {
                // Nothing to do.
            }
                        
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        aResult->length = sResultValue - aResult->value;
    }
    else
    {
        acpMemCpy( aResult->value,
                   aSource->value,
                   aSource->length );

        aResult->length = aSource->length;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdNcharInterfaceToNchar2( mtcStack*        aStack,
                                  const mtlModule* aSrcCharSet,
                                  const mtlModule* aDestCharSet,
                                  acp_uint8_t*     aSource,
                                  acp_uint16_t     aSourceLen,
                                  mtdNcharType*    aResult )
{
/***********************************************************************
 *
 * Description : 
 *      CHAR 타입을 NCHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    aciConvCharSetList     sIdnDestCharSet;
    aciConvCharSetList     sIdnSrcCharSet;
    acp_uint8_t*           sSourceIndex;
    acp_uint8_t*           sTempIndex;
    acp_uint8_t*           sSourceFence;
    acp_uint8_t*           sResultValue;
    acp_uint8_t*           sResultFence;
    acp_uint32_t           sNcharCnt = 0;
    acp_sint32_t           sSrcRemain = 0;
    acp_sint32_t           sDestRemain = 0;
    acp_sint32_t           sTempRemain = 0;

    // 변환 결과의 크기를 체크하기 위함
    sDestRemain = aDestCharSet->maxPrecision(aStack[0].column->precision);

    sSourceIndex = aSource;
    sSrcRemain   = aSourceLen;
    sSourceFence = sSourceIndex + aSourceLen;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtlGetIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtlGetIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            ACI_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;

            ACI_TEST( aciConvConvertCharSet( sIdnSrcCharSet,
                                             sIdnDestCharSet,
                                             sSourceIndex,
                                             sSrcRemain,
                                             sResultValue,
                                             & sDestRemain,
                                             0 )
                      != ACI_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );

            if( sTempRemain - sDestRemain > 0 )
            {
                sResultValue += (sTempRemain - sDestRemain);
                sNcharCnt++;
            }
            else
            {
                // Nothing to do.
            }

            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        aResult->length = sResultValue - aResult->value;
    }
    else
    {
        acpMemCpy( aResult->value,
                   aSource,
                   aSourceLen );

        aResult->length = aSourceLen;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdNcharInterfaceToChar( mtcStack*        aStack,
                                const mtlModule* aSrcCharSet,
                                const mtlModule* aDestCharSet,
                                mtdNcharType*    aSource,
                                mtdCharType*     aResult )
{
/***********************************************************************
 *
 * Description :
 *      NCHAR 타입을 CHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    aciConvCharSetList   sIdnDestCharSet;
    aciConvCharSetList   sIdnSrcCharSet;
    acp_uint8_t*         sSourceIndex;
    acp_uint8_t*         sTempIndex;
    acp_uint8_t*         sSourceFence;
    acp_uint8_t*         sResultValue;
    acp_uint8_t*         sResultFence;
    acp_sint32_t         sSrcRemain  = 0;
    acp_sint32_t         sDestRemain = 0;
    acp_sint32_t         sTempRemain = 0;
    acp_uint16_t         sSourceLen;

    // 변환 결과의 크기를 체크하기 위함
    sDestRemain = aStack[0].column->precision;

    sSourceIndex = aSource->value;
    sSourceLen   = aSource->length;
    sSourceFence = sSourceIndex + sSourceLen;
    sSrcRemain   = sSourceLen;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtlGetIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtlGetIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            ACI_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            ACI_TEST( aciConvConvertCharSet( sIdnSrcCharSet,
                                             sIdnDestCharSet,
                                             sSourceIndex,
                                             sSrcRemain,
                                             sResultValue,
                                             & sDestRemain,
                                             0 )
                      != ACI_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );
            
            sResultValue += (sTempRemain - sDestRemain);
            
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        aResult->length = sResultValue - aResult->value;
    }
    else
    {
        acpMemCpy( aResult->value,
                   aSource->value,
                   aSource->length );

        aResult->length = aSource->length;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdNcharInterfaceToChar2( mtcStack*        aStack,
                                 const mtlModule* aSrcCharSet,
                                 const mtlModule* aDestCharSet,
                                 mtdNcharType*    aSource,
                                 acp_uint8_t*     aResult,
                                 acp_uint16_t*    aResultLen )
{
/***********************************************************************
 *
 * Description :
 *      NCHAR 타입을 CHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    aciConvCharSetList   sIdnDestCharSet;
    aciConvCharSetList   sIdnSrcCharSet;
    acp_uint8_t*         sSourceIndex;
    acp_uint8_t*         sTempIndex;
    acp_uint8_t*         sSourceFence;
    acp_uint8_t*         sResultValue;
    acp_uint8_t*         sResultFence;
    acp_sint32_t         sSrcRemain = 0;
    acp_sint32_t         sDestRemain = 0;
    acp_sint32_t         sTempRemain = 0;
    acp_uint16_t         sSourceLen;

    // 변환 결과의 크기를 체크하기 위함
    sDestRemain = aStack[0].column->precision;

    sSourceIndex = aSource->value;
    sSourceLen   = aSource->length;
    sSourceFence = sSourceIndex + sSourceLen;
    sSrcRemain   = sSourceLen;

    sResultValue = aResult;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtlGetIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtlGetIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceLen-- > 0 )
        {
            ACI_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            ACI_TEST( aciConvConvertCharSet( sIdnSrcCharSet,
                                             sIdnDestCharSet,
                                             sSourceIndex,
                                             sSrcRemain,
                                             sResultValue,
                                             & sDestRemain,
                                             0 )
                      != ACI_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );
            
            sResultValue += (sTempRemain - sDestRemain);
            
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        *aResultLen = sResultValue - aResult;
    }
    else
    {
        acpMemCpy( aResult,
                   aSource->value,
                   aSource->length );

        *aResultLen = aSource->length;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdNcharInterfaceToNchar4UnicodeLiteral(
    const mtlModule* aSourceCharSet,
    const mtlModule* aResultCharSet,
    acp_uint8_t*     aSourceIndex,
    acp_uint8_t*     aSourceFence,
    acp_uint8_t**    aResultValue,
    acp_uint8_t*     aResultFence,
    acp_uint32_t*    aNcharCnt )
{
/***********************************************************************
 *
 * Description :
 *
 *      PROJ-1579 NCHAR
 *      U 타입 문자열을 NCHAR 타입으로 변경한다.
 *      U 타입 문자열에는 데이터베이스 캐릭터 셋과 '\'로 시작하는
 *      유니코드 포인트가 올 수 있다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t*            sTempSourceIndex;
    const mtlModule*        sU16CharSet;
    aciConvCharSetList      sIdnSourceCharSet;
    aciConvCharSetList      sIdnResultCharSet;
    aciConvCharSetList      sIdnU16CharSet;
    acp_bool_t              sIsSame       = ACP_FALSE;
    acp_sint32_t            sSrcRemain    = 0;
    acp_sint32_t            sDestRemain   = 0;
    acp_sint32_t            sTempRemain   = 0;
    acp_uint32_t            sNibbleLength = 0;
    acp_uint8_t             sNibbleValue[2];
    acp_uint32_t            i;
    acp_uint8_t             sSize;
    
    sU16CharSet = & mtclUTF16;

    sIdnSourceCharSet = mtlGetIdnCharSet( aSourceCharSet );
    sIdnResultCharSet = mtlGetIdnCharSet( aResultCharSet );
    sIdnU16CharSet    = mtlGetIdnCharSet( sU16CharSet );

    // 캐릭터 셋 변환 시 사용하는 버퍼의 길이
    sDestRemain = aResultFence - *aResultValue;

    // 소스의 길이
    sSrcRemain = aSourceFence - aSourceIndex;

    sTempSourceIndex = aSourceIndex;

    while( aSourceIndex < aSourceFence )
    {
        ACI_TEST_RAISE( *aResultValue >= aResultFence,
                        ERR_INVALID_DATA_LENGTH );

        sSize = mtlGetOneCharSize( aSourceIndex,
                                   aSourceFence,
                                   aSourceCharSet );

        sIsSame = mtcCompareOneChar( aSourceIndex,
                                     sSize,
                                     aSourceCharSet->specialCharSet[MTL_QT_IDX],
                                     aSourceCharSet->specialCharSize );

        if( sIsSame == ACP_TRUE )
        {
            (void)aSourceCharSet->nextCharPtr( & aSourceIndex, aSourceFence );

            sSize = mtlGetOneCharSize( aSourceIndex,
                                       aSourceFence,
                                       aSourceCharSet );

            sIsSame = mtcCompareOneChar( aSourceIndex,
                                         sSize,
                                         aSourceCharSet->specialCharSet[MTL_QT_IDX],
                                         aSourceCharSet->specialCharSize );

            ACI_TEST_RAISE( sIsSame != ACP_TRUE,
                            ERR_INVALID_LITERAL );
        }


        sSize = mtlGetOneCharSize( aSourceIndex,
                                   aSourceFence,
                                   aSourceCharSet );

        sIsSame = mtcCompareOneChar( aSourceIndex,
                                     sSize,
                                     aSourceCharSet->specialCharSet[MTL_BS_IDX],
                                     aSourceCharSet->specialCharSize );
        
        if( sIsSame == ACP_TRUE )
        {
            sTempSourceIndex = aSourceIndex;

            // 16진수 문자 4개를 읽는다.
            for( i = 0; i < 4; i++ )
            {
                ACI_TEST_RAISE( aSourceCharSet->nextCharPtr( & aSourceIndex, aSourceFence )
                                != NC_VALID, ERR_INVALID_U_TYPE_STRING );
            }

            (void)aSourceCharSet->nextCharPtr( & sTempSourceIndex, aSourceFence );
            
            // UTF16 값을 sNibbleValue에 받아온다.
            ACI_TEST_RAISE( mtcMakeNibble2( sNibbleValue,
                                            0,
                                            sTempSourceIndex,
                                            4,
                                            & sNibbleLength )
                            != ACI_SUCCESS, ERR_INVALID_U_TYPE_STRING );

            sTempRemain = sDestRemain;

            if( aResultCharSet->id != MTL_UTF16_ID )
            {
                // UTF16 캐릭터 셋 => 내셔널 캐릭터 셋으로 변환한다.
                ACI_TEST( aciConvConvertCharSet ( sIdnU16CharSet,
                                                  sIdnResultCharSet,
                                                  sNibbleValue,
                                                  MTL_UTF16_PRECISION,
                                                  *aResultValue,
                                                  & sDestRemain,
                                                  0 )
                          != ACI_SUCCESS );
            }
            else
            {
                (*aResultValue)[0] = sNibbleValue[0];
                (*aResultValue)[1] = sNibbleValue[1];

                sDestRemain -= 2;
            }
        }
        else
        {
            sTempRemain = sDestRemain;

            if( sIdnSourceCharSet != sIdnResultCharSet )
            {
                // DB 캐릭터 셋 => 내셔널 캐릭터 셋으로 변환한다.
                ACI_TEST( aciConvConvertCharSet( sIdnSourceCharSet,
                                                 sIdnResultCharSet,
                                                 aSourceIndex,
                                                 sSrcRemain,
                                                 *aResultValue,
                                                 & sDestRemain,
                                                 0 )
                          != ACI_SUCCESS );
            }
            // 데이터 베이스 캐릭터 셋 = 내셔널 캐릭터 셋 = UTF8
            else
            {
                sTempSourceIndex = aSourceIndex;

                (void)aSourceCharSet->nextCharPtr( & aSourceIndex, aSourceFence );
                
                acpMemCpy( *aResultValue,
                           sTempSourceIndex,
                           aSourceIndex - sTempSourceIndex );
                aSourceIndex = sTempSourceIndex;

                sDestRemain -= (aSourceIndex - sTempSourceIndex);
            }
        }

        sTempSourceIndex = aSourceIndex;

        (void)aSourceCharSet->nextCharPtr( & aSourceIndex, aSourceFence );
        
        (*aResultValue) += (sTempRemain - sDestRemain);

        sSrcRemain -= ( aSourceIndex - sTempSourceIndex );

        (*aNcharCnt)++;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    }

    ACI_EXCEPTION( ERR_INVALID_U_TYPE_STRING );
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_U_TYPE_STRING);
    }

    ACI_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }
    
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
