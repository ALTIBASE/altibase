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
extern mtlModule* mtlNationalCharSet;
extern mtlModule* mtlDBCharSet;

static ACI_RC mtdInitializeNvarchar( acp_uint32_t aNo );

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

static acp_sint32_t mtdNvarcharLogicalComp( mtdValueInfo* aValueInfo1,
                                            mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNvarcharMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                 mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNvarcharMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                  mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNvarcharStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                    mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNvarcharStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                     mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNvarcharStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                       mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdNvarcharStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                                        mtdValueInfo* aValueInfo2 );

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonizedValue,
                           mtcEncryptInfo*  aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*    aColumn,
                           void*         aValue,
                           acp_uint32_t  aValueSize);

static ACI_RC mtdValueFromOracle( mtcColumn*    aColumn,
                                  void*         aValue,
                                  acp_uint32_t* aValueOffset,
                                  acp_uint32_t  aValueSize,
                                  const void*   aOracleValue,
                                  acp_sint32_t  aOracleLength,
                                  ACI_RC*       aResult );

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void*             aDestVale,
                                       acp_uint32_t      aDestValueOffset,
                                       acp_uint32_t      aLength,
                                       const void*       aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 8, (void*)"NVARCHAR"  }
};

static mtcColumn mtdColumn;

mtdModule mtcdNvarchar = {
    mtdTypeName,
    &mtdColumn,
    MTD_NVARCHAR_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_NCHAR_ALIGN,
    MTD_GROUP_TEXT|
    MTD_CANON_NEED|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_VARIABLE|
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
    mtdInitializeNvarchar,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdNvarcharLogicalComp,    // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdNvarcharMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdNvarcharMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdNvarcharStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdNvarcharStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdNvarcharStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdNvarcharStoredStoredKeyDescComp // Descending Key Comparison
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

ACI_RC mtdInitializeNvarchar( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdNvarchar, aNo )
              != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdNvarchar,
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
    ACP_UNUSED( aScale);
    
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
    acp_uint32_t              sValueOffset;
    mtdNcharType*             sValue;
    acp_uint8_t*              sToken;
    acp_uint8_t*              sTokenFence;
    acp_uint8_t*              sIterator;
    acp_uint8_t*              sTempToken;
    acp_uint8_t*              sFence;
    const mtlModule*          sColLanguage;
    const mtlModule*          sTokenLanguage;
    acp_bool_t                sIsSame   = ACP_FALSE;
    acp_uint32_t              sNcharCnt = 0;
    aciConvCharSetList        sColCharSet;
    aciConvCharSetList        sTokenCharSet;
    acp_sint32_t              sSrcRemain;
    acp_sint32_t              sDestRemain;
    acp_sint32_t              sTempRemain;
    acp_uint8_t               sSize;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_NCHAR_ALIGN );

    sValue = (mtdNcharType*)( (acp_uint8_t*)aValue + sValueOffset );

    *aResult = ACI_SUCCESS;

    sColLanguage = aColumn->language;

    // NCHAR 리터럴을 사용했다면 NVARCHAR 타입으로는 인식되지 않는다.
    // 무조건 DB 캐릭터 셋이다.
    sTokenLanguage = mtlDBCharSet;

    // To fix BUG-13444
    // tokenFence와 RowFence는 별개의 검사과정이므로,
    // 먼저 RowFence검사 후 TokenFence검사를 해야 한다.
    sIterator  = sValue->value;
    sSrcRemain = sTokenLanguage->maxPrecision( aTokenLength );
    sFence     = (acp_uint8_t*)aValue + aValueSize;

    if( sIterator >= sFence )
    {
        *aResult = ACI_FAILURE;
    }
    else
    {    
        sColCharSet = mtlGetIdnCharSet( sColLanguage );
        sTokenCharSet = mtlGetIdnCharSet( sTokenLanguage );

        sDestRemain = aTokenLength * MTL_MAX_PRECISION;

        for( sToken      = (acp_uint8_t *)aToken,
                 sTokenFence = sToken + aTokenLength;
             sToken      < sTokenFence;
             )
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
                ACI_TEST(aciConvConvertCharSet( sTokenCharSet,
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

            sSrcRemain -= ( sToken - sTempToken );
        }
    }

    if( *aResult == ACI_SUCCESS )
    {
        sValue->length           = sIterator - sValue->value;

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;

        aColumn->precision   = sValue->length != 0 ?
            (sNcharCnt * MTL_NCHAR_PRECISION(mtlNationalCharSet)) : 1;

        aColumn->precision = sValue->length != 0 ? sNcharCnt : 1;

        aColumn->scale           = 0;

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
                           mtcdNvarchar.staticNull );

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
                           mtcdNvarchar.staticNull );

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
                           mtcdNvarchar.staticNull );

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
                           mtcdNvarchar.staticNull );

    return (sValue->length == 0) ? ACP_TRUE : ACP_FALSE;
}

acp_sint32_t
mtdNvarcharLogicalComp(  mtdValueInfo* aValueInfo1,
                         mtdValueInfo* aValueInfo2  )
{
    return mtdNvarcharMtdMtdKeyAscComp( aValueInfo1,
                                        aValueInfo2 );
}

acp_sint32_t
mtdNvarcharMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                             mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType* sNvarcharValue1;
    const mtdNcharType* sNvarcharValue2;
    acp_uint16_t        sLength1 = 0;
    acp_uint16_t        sLength2 = 0;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;

    //---------
    // value1
    //---------    
    sNvarcharValue1 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNvarchar.staticNull );

    sLength1 = sNvarcharValue1->length;
    sValue1  = sNvarcharValue1->value;

    //---------
    // value2
    //---------    
    sNvarcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNvarchar.staticNull );

    sLength2 = sNvarcharValue2->length;
    sValue2  = sNvarcharValue2->value;

    //---------
    // compare
    //---------    

    if( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        if( sLength1 > sLength2 )
        {
            return acpMemCmp( sValue1, sValue2, sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return acpMemCmp( sValue1, sValue2, sLength1 ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue1, sValue2, sLength1 );
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
mtdNvarcharMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType* sNvarcharValue1;
    const mtdNcharType* sNvarcharValue2;
    acp_uint16_t        sLength1 = 0;
    acp_uint16_t        sLength2 = 0;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;

    //---------
    // value1
    //---------
    sNvarcharValue1 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdNvarchar.staticNull );

    sLength1 = sNvarcharValue1->length;
    sValue1  = sNvarcharValue1->value;

    //---------
    // value2
    //---------    
    sNvarcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNvarchar.staticNull );

    sLength2 = sNvarcharValue2->length;
    sValue2  = sNvarcharValue2->value;

    //---------
    // compare
    //---------    

    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength2 > sLength1 )
        {
            return acpMemCmp( sValue2, sValue1, sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return acpMemCmp( sValue2, sValue1, sLength2 ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue2, sValue1, sLength2 );
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
mtdNvarcharStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType* sNvarcharValue2;
    acp_uint16_t        sLength1 = 0;
    acp_uint16_t        sLength2 = 0;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;

    //---------
    // value1
    //---------    
    sLength1 = (acp_uint16_t)aValueInfo1->length;

    //---------
    // value2
    //---------    
    sNvarcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNvarchar.staticNull );

    sLength2 = sNvarcharValue2->length;

    //---------
    // compare
    //---------    

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (acp_uint8_t*)aValueInfo1->value;
        sValue2  = sNvarcharValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return acpMemCmp( sValue1, sValue2, sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return acpMemCmp( sValue1, sValue2, sLength1 ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue1, sValue2, sLength1 );
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
mtdNvarcharStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                 mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType* sNvarcharValue2;
    acp_uint16_t        sLength1 = 0;
    acp_uint16_t        sLength2 = 0;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;
    
    //---------
    // value1
    //---------    
    sLength1 = (acp_uint16_t)aValueInfo1->length;

    //---------
    // value2
    //---------    
    sNvarcharValue2 = (const mtdNcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdNvarchar.staticNull );
    
    sLength2 = sNvarcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (acp_uint8_t*)aValueInfo1->value;
        sValue2  = sNvarcharValue2->value;
        
        if( sLength2 > sLength1 )
        {
            return acpMemCmp( sValue2, sValue1, sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return acpMemCmp( sValue2, sValue1, sLength2 ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue2, sValue1, sLength2 );
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
mtdNvarcharStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                   mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t       sLength1 = 0;
    acp_uint16_t       sLength2 = 0;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;
    
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
        
        if( sLength1 > sLength2 )
        {
            return acpMemCmp( sValue1, sValue2, sLength2 ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return acpMemCmp( sValue1, sValue2, sLength1 ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue1, sValue2, sLength1 );
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
mtdNvarcharStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
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
    const acp_uint8_t*         sValue1;
    const acp_uint8_t*         sValue2;

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
        sValue1 = (acp_uint8_t*)aValueInfo1->value;
        sValue2 = (acp_uint8_t*)aValueInfo2->value;
        
        if( sLength2 > sLength1 )
        {
            return acpMemCmp( sValue2, sValue1, sLength1 ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return acpMemCmp( sValue2, sValue1, sLength2 ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue2, sValue1, sLength2 );
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
                           void**           aCanonizedValue,
                           mtcEncryptInfo*  aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate  )
{
    const mtlModule* sLanguage;
    mtdNcharType*    sValue;
    acp_uint8_t*     sValueIndex;
    acp_uint8_t*     sValueFence;
    acp_uint16_t     sValueCharCnt = 0;

    ACP_UNUSED( aCanonInfo);
    ACP_UNUSED( aColumn);
    ACP_UNUSED( aColumnInfo);
    ACP_UNUSED( aTemplate);
    
    sValue = (mtdNcharType*)aValue;

    sLanguage = aCanon->language;

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

    ACI_TEST_RAISE( sValueCharCnt > (acp_uint32_t)aCanon->precision,
                    ERR_INVALID_LENGTH );

    *aCanonizedValue = aValue;
    
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
        
    mtdNcharType * sCharVal = (mtdNcharType*)aValue;
    ACI_TEST_RAISE( sCharVal == NULL, ERR_INVALID_NULL );

    ACI_TEST_RAISE( aValueSize < sizeof(acp_uint16_t), ERR_INVALID_LENGTH );

    ACI_TEST_RAISE( sCharVal->length + sizeof(acp_uint16_t) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdNvarchar,
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
                                   & mtcdNvarchar,
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

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestVale,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdNcharType* sNvarcharValue;

    sNvarcharValue = (mtdNcharType*)aDestVale;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sNvarcharValue->length = 0;
    }
    else
    {
        ACI_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sNvarcharValue->length = (acp_uint16_t)(aDestValueOffset + aLength);
        acpMemCpy( (acp_uint8_t*)sNvarcharValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
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

