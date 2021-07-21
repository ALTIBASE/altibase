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
 * $Id: mtcDef.h 34251 2009-07-29 04:07:59Z sungminee $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>
#include <mtclCollate.h>

extern mtdModule mtcdEchar;

// To Remove Warning
const mtdEcharType mtcdEcharNull = { 0, 0, {'\0',} };

static ACI_RC mtdInitializeEchar( acp_uint32_t aNo );

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

static acp_sint32_t mtdEcharLogicalComp( mtdValueInfo* aValueInfo1,
                                         mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdEcharMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                              mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdEcharMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                               mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdEcharStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                 mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdEcharStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                  mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdEcharStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                    mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdEcharStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
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

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 5, (void*)"ECHAR" }
};

static mtcColumn mtdColumn;

mtdModule mtcdEchar = {
    mtdTypeName,
    &mtdColumn,
    MTD_ECHAR_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_ECHAR_ALIGN,
    MTD_GROUP_TEXT|
    MTD_CANON_NEED_WITH_ALLOCATION|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_DISABLE|
    MTD_CREATE_PARAM_PRECISION|
    MTD_CASE_SENS_TRUE|
    MTD_SEARCHABLE_SEARCHABLE|       // BUG-17020
    MTD_LITERAL_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|   // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_TRUE|   // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_TRUE|    // PROJ-1705
    MTD_ENCRYPT_TYPE_TRUE,           // PROJ-2002
    10000,
    0,
    0,
    (void*)&mtcdEcharNull,
    mtdInitializeEchar,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdEcharLogicalComp,    // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdEcharMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdEcharMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdEcharStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdEcharStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdEcharStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdEcharStoredStoredKeyDescComp // Descending Key Comparison
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtdSelectivityDefault,
    mtdEncodeNA,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSize
};


ACI_RC mtdInitializeEchar( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdEchar, aNo )
              != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdEchar,
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
        *aPrecision = MTD_ECHAR_PRECISION_DEFAULT;
    }
    
    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    ACI_TEST_RAISE( *aPrecision < MTD_ECHAR_PRECISION_MINIMUM ||
                    *aPrecision > MTD_ECHAR_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = sizeof(acp_uint16_t) + sizeof(acp_uint16_t) + *aPrecision;
    
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
    acp_uint32_t       sValueOffset;
    mtdEcharType*      sValue;
    const acp_uint8_t* sToken;
    const acp_uint8_t* sTokenFence;
    acp_uint8_t*       sIterator;
    acp_uint8_t*       sFence;
    acp_uint16_t       sValueLength;
    acp_uint16_t       sLength;
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_ECHAR_ALIGN );

    sValue = (mtdEcharType*)( (acp_uint8_t*)aValue + sValueOffset );

    *aResult = ACI_SUCCESS;

    // To fix BUG-13444
    // tokenFence와 RowFence는 별개의 검사과정이므로,
    // 먼저 RowFence검사 후 TokenFence검사를 해야 한다.
    sIterator = sValue->mValue;
    sFence    = (acp_uint8_t*)aValue + aValueSize;
    if( sIterator >= sFence )
    {
        *aResult = ACI_FAILURE;
    }
    else
    {    
        for( sToken      = (const acp_uint8_t*)aToken,
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
    }
    
    if( *aResult == ACI_SUCCESS )
    {
        // value에 cipher text length 셋팅
        sValue->mCipherLength = sIterator - sValue->mValue;

        //-----------------------------------------------------
        // PROJ-2002 Column Security
        //
        // [padding 제거하는 이유]
        // char type의 compare는 padding을 무시하고 비교한다.
        // 따라서 echar type의 padding을 제거하여 ecc를 생성하면 
        // ecc의 memcmp만으로 echar type의 비교가 가능하다.
        // 
        // 단, NULL과 ' ', '  '의 비교를 위하여 
        // NULL에 대해서는 ecc를 생성하지 않으며, ' ', '  '는
        // space padding 하나(' ')로 ecc를 생성한다.
        // 
        // 예제) char'NULL' => echar( encrypt(''),   ecc('')  )
        //       char' '    => echar( encrypt(' '),  ecc(' ') )
        //       char'  '   => echar( encrypt('  '), ecc(' ') )
        //       char'a'    => echar( encrypt('a'),  ecc('a') )
        //       char'a '   => echar( encrypt('a '), ecc('a') )
        //-----------------------------------------------------
        
        // padding 제거
        // sEcharValue에서 space pading을 제외한 길이를 찾는다.
        for( sLength = sValue->mCipherLength; sLength > 1; sLength-- )
        {
            if( sValue->mValue[sLength - 1] != ' ' )
            {
                break;
            }
        }

        if( sValue->mCipherLength > 0 )
        {
            // value에 ecc value & ecc length 셋팅
            ACI_TEST( aTemplate->encodeECC( sValue->mValue,
                                            sLength,
                                            sIterator,
                                            & sValue->mEccLength )
                      != ACI_SUCCESS );
        }
        else
        {
            sValue->mEccLength = 0;            
        }
        
        sValueLength = sValue->mCipherLength + sValue->mEccLength;

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag         = 1;
        aColumn->precision    = sValue->mCipherLength != 0 ? sValue->mCipherLength : 1;
        aColumn->scale        = 0;
        aColumn->mColumnAttr.mEncAttr.mEncPrecision = sValueLength != 0 ? sValueLength : 1;
        aColumn->mColumnAttr.mEncAttr.mPolicy[0]    = '\0';
        
        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->mColumnAttr.mEncAttr.mEncPrecision,
                               & aColumn->scale )
                  != ACI_SUCCESS );

        aColumn->column.offset   = sValueOffset;
            *aValueOffset            = sValueOffset
                + sizeof(acp_uint16_t) + sizeof(acp_uint16_t)
                + sValueLength;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    }    
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                            const void*      aRow,
                            acp_uint32_t     aFlag )
{
    const mtdEcharType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdEchar.staticNull );

    return sizeof(acp_uint16_t) + sizeof(acp_uint16_t)
        + sValue->mCipherLength + sValue->mEccLength; 
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdEcharType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdEcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           NULL );

    if( sValue != NULL )
    {
        sValue->mCipherLength = 0;
        sValue->mEccLength = 0;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdEcharType  * sValue;

    ACP_UNUSED( aColumn);

    sValue = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdEchar.staticNull );

    // ecc로 해시 수행
    return mtcHash( aHash, sValue->mValue + sValue->mCipherLength,
                    sValue->mEccLength );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdEcharType* sValue;

    ACP_UNUSED( aColumn);

    sValue = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdEchar.staticNull );

    if ( sValue->mEccLength == 0 )
    {
        ACE_ASSERT( sValue->mCipherLength == 0 );
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

acp_sint32_t
mtdEcharLogicalComp( mtdValueInfo* aValueInfo1,
                     mtdValueInfo* aValueInfo2 )
{
    return mtdEcharMtdMtdKeyAscComp( aValueInfo1,
                                     aValueInfo2 );
}

acp_sint32_t
mtdEcharMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                          mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdEcharType* sEcharValue1;
    const mtdEcharType* sEcharValue2;
    acp_uint16_t        sEccLength1;
    acp_uint16_t        sEccLength2;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;

    //---------
    // value1
    //---------
    sEcharValue1 = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdEchar.staticNull );
    sEccLength1 = sEcharValue1->mEccLength;

    //---------
    // value2
    //---------    
    sEcharValue2 = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdEchar.staticNull );
    sEccLength2 = sEcharValue2->mEccLength;

    //---------
    // compare
    //---------

    // ecc로 비교
    if( (sEccLength1 != 0) && (sEccLength2 != 0) )
    {
        sValue1  = sEcharValue1->mValue + sEcharValue1->mCipherLength;
        sValue2  = sEcharValue2->mValue + sEcharValue2->mCipherLength;
    
        if( sEccLength1 > sEccLength2 )
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength2 ) >= 0 ? 1 : -1 ;
        }
        else if( sEccLength1 < sEccLength2 )
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength1 ) > 0 ? 1 : -1 ;
        }
        else
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength1 );
        }
    }
    else
    {
        if( sEccLength1 < sEccLength2 )
        {
            return 1;
        }
        if( sEccLength1 > sEccLength2 )
        {
            return -1;
        }
        return 0;
    }
}

acp_sint32_t
mtdEcharMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                           mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdEcharType* sEcharValue1;
    const mtdEcharType* sEcharValue2;
    acp_uint16_t        sEccLength1;
    acp_uint16_t        sEccLength2;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;

    //---------
    // value1
    //---------    
    sEcharValue1 = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdEchar.staticNull );
    sEccLength1 = sEcharValue1->mEccLength;

    //---------
    // value2
    //---------    
    sEcharValue2 = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdEchar.staticNull );
    sEccLength2 = sEcharValue2->mEccLength;

    //---------
    // compare
    //---------

    // ecc로 비교
    if( (sEccLength1 != 0) && (sEccLength2 != 0) )
    {
        sValue1  = sEcharValue1->mValue + sEcharValue1->mCipherLength;
        sValue2  = sEcharValue2->mValue + sEcharValue2->mCipherLength;
    
        if( sEccLength2 > sEccLength1 )
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength1 ) >= 0 ? 1 : -1 ;
        }
        else if( sEccLength2 < sEccLength1 )
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength2 ) > 0 ? 1 : -1 ;
        }
        else
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength2 );
        }
    }
    else
    {
        if( sEccLength1 < sEccLength2 )
        {
            return 1;
        }
        if( sEccLength1 > sEccLength2 )
        {
            return -1;
        }
        return 0;
    }
}

acp_sint32_t
mtdEcharStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                             mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdEcharType* sEcharValue2;
    acp_uint16_t        sCipherLength1;
    acp_uint16_t        sEccLength1;
    acp_uint16_t        sEccLength2;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;    

    //---------
    // value1
    //---------
    if ( aValueInfo1->length != 0 )
    {
        MTC_SHORT_BYTE_ASSIGN( &sEccLength1,
                               (acp_uint8_t*)aValueInfo1->value + sizeof(acp_uint16_t) );
    }
    else
    {
        sEccLength1 = 0;
    }
    
    //---------
    // value2
    //---------    
    sEcharValue2 = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdEchar.staticNull );
    sEccLength2 = sEcharValue2->mEccLength;

    //---------
    // compare
    //---------

    // ecc로 비교
    if( (sEccLength1 != 0) && (sEccLength2 != 0) )
    {
        MTC_SHORT_BYTE_ASSIGN( &sCipherLength1,
                               (acp_uint8_t*)aValueInfo1->value );
        
        sValue1  = (acp_uint8_t*)aValueInfo1->value + mtdHeaderSize() + sCipherLength1;
        sValue2  = sEcharValue2->mValue + sEcharValue2->mCipherLength;
    
        if( sEccLength1 > sEccLength2 )
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength2 ) >= 0 ? 1 : -1 ;
        }
        else if( sEccLength1 < sEccLength2 )
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength1 ) > 0 ? 1 : -1 ;
        }
        else
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength1 );
        }
    }
    else
    {
        if( sEccLength1 < sEccLength2 )
        {
            return 1;
        }
        if( sEccLength1 > sEccLength2 )
        {
            return -1;
        }
        return 0;
    }
}

acp_sint32_t
mtdEcharStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdEcharType* sEcharValue2;
    acp_uint16_t        sCipherLength1;
    acp_uint16_t        sEccLength1;
    acp_uint16_t        sEccLength2;
    const acp_uint8_t*  sValue1;
    const acp_uint8_t*  sValue2;    

    //---------
    // value1
    //---------
    if ( aValueInfo1->length != 0 )
    {
        MTC_SHORT_BYTE_ASSIGN( &sEccLength1,
                               (acp_uint8_t*)aValueInfo1->value + sizeof(acp_uint16_t) );
    }
    else
    {
        sEccLength1 = 0;
    }
    
    //---------
    // value2
    //---------    
    sEcharValue2 = (const mtdEcharType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdEchar.staticNull );
    sEccLength2 = sEcharValue2->mEccLength;

    //---------
    // compare
    //---------

    // ecc로 비교
    if( (sEccLength1 != 0) && (sEccLength2 != 0) )
    {
        MTC_SHORT_BYTE_ASSIGN( &sCipherLength1,
                               (acp_uint8_t*)aValueInfo1->value );

        sValue1  = (acp_uint8_t*)aValueInfo1->value + mtdHeaderSize() + sCipherLength1;
        sValue2  = sEcharValue2->mValue + sEcharValue2->mCipherLength;
    
        if( sEccLength2 > sEccLength1 )
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength1 ) >= 0 ? 1 : -1 ;
        }
        else if( sEccLength2 < sEccLength1 )
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength2 ) > 0 ? 1 : -1 ;
        }
        else
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength2 );
        }
    }
    else
    {
        if( sEccLength1 < sEccLength2 )
        {
            return 1;
        }
        if( sEccLength1 > sEccLength2 )
        {
            return -1;
        }
        return 0;
    }
}

acp_sint32_t
mtdEcharStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t                sCipherLength1;
    acp_uint16_t                sCipherLength2;
    acp_uint16_t                sEccLength1;
    acp_uint16_t                sEccLength2;
    const acp_uint8_t         * sValue1;
    const acp_uint8_t         * sValue2;

    //---------
    // value1
    //---------
    if ( aValueInfo1->length != 0 )
    {
        MTC_SHORT_BYTE_ASSIGN( &sEccLength1,
                               (acp_uint8_t*)aValueInfo1->value + sizeof(acp_uint16_t) );
    }
    else
    {
        sEccLength1 = 0;
    }
    
    //---------
    // value2
    //---------
    if ( aValueInfo2->length != 0 )
    {
        MTC_SHORT_BYTE_ASSIGN( &sEccLength2,
                               (acp_uint8_t*)aValueInfo2->value + sizeof(acp_uint16_t) );
    }
    else
    {
        sEccLength2 = 0;
    }
    
    //---------
    // compare
    //---------

    // ecc로 비교
    if( (sEccLength1 != 0) && (sEccLength2 != 0) )
    {
        MTC_SHORT_BYTE_ASSIGN( &sCipherLength1,
                               (acp_uint8_t*)aValueInfo1->value );
        MTC_SHORT_BYTE_ASSIGN( &sCipherLength2,
                               (acp_uint8_t*)aValueInfo2->value );

        sValue1  = (acp_uint8_t*)aValueInfo1->value + mtdHeaderSize() + sCipherLength1;
        sValue2  = (acp_uint8_t*)aValueInfo2->value + mtdHeaderSize() + sCipherLength2;
        
        if( sEccLength1 > sEccLength2 )
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength2 ) >= 0 ? 1 : -1 ;
        }
        else if( sEccLength1 < sEccLength2 )
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength1 ) > 0 ? 1 : -1 ;
        }
        else
        {
            return acpMemCmp( sValue1,
                              sValue2,
                              sEccLength1 );
        }
    }
    else
    {
        if( sEccLength1 < sEccLength2 )
        {
            return 1;
        }
        if( sEccLength1 > sEccLength2 )
        {
            return -1;
        }
        return 0;
    }
}

acp_sint32_t
mtdEcharStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                 mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t                sCipherLength1;
    acp_uint16_t                sCipherLength2;
    acp_uint16_t                sEccLength1;
    acp_uint16_t                sEccLength2;
    const acp_uint8_t         * sValue1;
    const acp_uint8_t         * sValue2;

    //---------
    // value1
    //---------
    if ( aValueInfo1->length != 0 )
    {
        MTC_SHORT_BYTE_ASSIGN( &sEccLength1,
                               (acp_uint8_t*)aValueInfo1->value + sizeof(acp_uint16_t) );
    }
    else
    {
        sEccLength1 = 0;
    }
    
    //---------
    // value2
    //---------
    if ( aValueInfo2->length != 0 )
    {
        MTC_SHORT_BYTE_ASSIGN( &sEccLength2,
                               (acp_uint8_t*)aValueInfo2->value + sizeof(acp_uint16_t) );
    }
    else
    {
        sEccLength2 = 0;
    }
    
    //---------
    // compare
    //---------

    // ecc로 비교
    if( (aValueInfo1->length != 0) && (aValueInfo2->length != 0) )
    {
        MTC_SHORT_BYTE_ASSIGN( &sCipherLength1,
                               (acp_uint8_t*)aValueInfo1->value );
        MTC_SHORT_BYTE_ASSIGN( &sCipherLength2,
                               (acp_uint8_t*)aValueInfo2->value );

        sValue1  = (acp_uint8_t*)aValueInfo1->value + mtdHeaderSize() + sCipherLength1;
        sValue2  = (acp_uint8_t*)aValueInfo2->value + mtdHeaderSize() + sCipherLength2;
    
        if( sEccLength2 > sEccLength1 )
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength1 ) >= 0 ? 1 : -1 ;
        }
        else if( sEccLength2 < sEccLength1 )
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength2 ) > 0 ? 1 : -1 ;
        }
        else
        {
            return acpMemCmp( sValue2,
                              sValue1,
                              sEccLength2 );
        }
    }
    else
    {
        if( sEccLength1 < sEccLength2 )
        {
            return 1;
        }
        if( sEccLength1 > sEccLength2 )
        {
            return -1;
        }
        return 0;
    }    
}

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonized,
                           mtcEncryptInfo*  aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate )
{
    mtdEcharType* sCanonized;
    mtdEcharType* sValue;
    acp_uint8_t   sDecryptedBuf[MTD_ECHAR_DECRYPT_BUFFER_SIZE];
    acp_uint8_t * sPlain;
    acp_uint16_t  sPlainLength;
        
    sValue     = (mtdEcharType*)aValue;
    sCanonized = (mtdEcharType*)*aCanonized;
    sPlain     = sDecryptedBuf;
    
    // 컬럼의 보안정책으로 암호화
    if ( ( aColumn->mColumnAttr.mEncAttr.mPolicy[0] == '\0' ) &&
         ( aCanon->mColumnAttr.mEncAttr.mPolicy[0] == '\0' ) )
    {
        //-----------------------------------------------------
        // case 1. default policy -> default policy
        //-----------------------------------------------------
        
        if( sValue->mCipherLength == aCanon->precision )
        {
            ACI_TEST_RAISE( sValue->mCipherLength + sValue->mEccLength >
                            aCanon->mColumnAttr.mEncAttr.mEncPrecision,
                            ERR_INVALID_LENGTH );
            
            *aCanonized = aValue;
        }
        else
        {
            if( sValue->mEccLength == 0 )
            {
                ACE_ASSERT( sValue->mCipherLength == 0 );

                *aCanonized = aValue;
            }
            else
            {
                ACI_TEST_RAISE( sValue->mCipherLength > aCanon->precision,
                                ERR_INVALID_LENGTH );

                // a. copy cipher value
                if( sValue->mCipherLength > 0 )
                {
                    acpMemCpy( sCanonized->mValue,
                               sValue->mValue,
                               sValue->mCipherLength );
                }
                else
                {
                    // Nothing to do.
                }
            
                // b. padding
                if( ( aCanon->precision - sValue->mCipherLength ) > 0 )
                {
                    acpMemSet( sCanonized->mValue + sValue->mCipherLength, ' ',
                               aCanon->precision - sValue->mCipherLength );
                }
                else
                {
                    // Nothing to do.
                }
            
                // c. copy ecc value
                ACI_TEST_RAISE( aCanon->precision + sValue->mEccLength >
                                aCanon->mColumnAttr.mEncAttr.mEncPrecision,
                                ERR_INVALID_LENGTH );
            
                acpMemCpy( sCanonized->mValue + aCanon->precision,
                           sValue->mValue + sValue->mCipherLength,
                           sValue->mEccLength );
            
                sCanonized->mCipherLength = aCanon->precision;
                sCanonized->mEccLength    = sValue->mEccLength;
            }
        }
    }
    else if ( ( aColumn->mColumnAttr.mEncAttr.mPolicy[0] != '\0' ) &&
              ( aCanon->mColumnAttr.mEncAttr.mPolicy[0] == '\0' ) )
    {
        //-----------------------------------------------------
        // case 2. policy1 -> default policy
        //-----------------------------------------------------
        
        ACE_ASSERT( aColumnInfo != NULL );

        if( sValue->mEccLength == 0 )
        {
            ACE_ASSERT( sValue->mCipherLength == 0 );
            
            *aCanonized = aValue;
        }
        else
        {
            // a. copy cipher value
            if( sValue->mCipherLength > 0 )
            {            
                ACI_TEST( aTemplate->decrypt( aColumnInfo,
                                              (acp_char_t*) aColumn->mColumnAttr.mEncAttr.mPolicy,
                                              sValue->mValue,
                                              sValue->mCipherLength,
                                              sPlain,
                                              & sPlainLength )
                          != ACI_SUCCESS );
                
                ACE_ASSERT( sPlainLength <= aColumn->precision );

                ACI_TEST_RAISE( sPlainLength > aCanon->precision,
                                ERR_INVALID_LENGTH );

                acpMemCpy( sCanonized->mValue,
                           sPlain,
                           sPlainLength );
            }
            else
            {
                sPlainLength = 0;
            }
            
            // b. padding
            if( aCanon->precision - sPlainLength > 0 )
            {
                acpMemSet( sCanonized->mValue + sPlainLength, ' ',
                           aCanon->precision - sPlainLength );
            }
            else
            {
                // Nothing to do.
            }
            
            // c. copy ecc value
            ACI_TEST_RAISE( aCanon->precision + sValue->mEccLength >
                            aCanon->mColumnAttr.mEncAttr.mEncPrecision,
                            ERR_INVALID_LENGTH );
            
            acpMemCpy( sCanonized->mValue + aCanon->precision,
                       sValue->mValue + sValue->mCipherLength,
                       sValue->mEccLength );
            
            sCanonized->mCipherLength = aCanon->precision;
            sCanonized->mEccLength    = sValue->mEccLength;
        }
    }
    else if ( ( aColumn->mColumnAttr.mEncAttr.mPolicy[0] == '\0' ) &&
              ( aCanon->mColumnAttr.mEncAttr.mPolicy[0] != '\0' ) )
    {
        //-----------------------------------------------------
        // case 3. default policy -> policy2
        // 
        // ex) echar(1,def) {1,'a',ecc('a')}
        //     -> echar(3,p1) {3,enc('a  '),ecc('a')}
        //-----------------------------------------------------
        
        ACE_ASSERT( aCanonInfo != NULL );

        if( sValue->mEccLength == 0 )
        {
            ACE_ASSERT( sValue->mCipherLength == 0 );
            
            *aCanonized = aValue;
        }
        else
        {
            ACI_TEST_RAISE( sValue->mCipherLength > aCanon->precision,
                            ERR_INVALID_LENGTH );

            // a. padding
            if( sValue->mCipherLength > 0 )
            {
                acpMemCpy( sPlain,
                           sValue->mValue,
                           sValue->mCipherLength );
            }
            else
            {
                // Nothing to do.
            }

            if( aCanon->precision - sValue->mCipherLength > 0 )
            {
                acpMemSet( sPlain + sValue->mCipherLength,
                           ' ',
                           aCanon->precision - sValue->mCipherLength );
            }
            else
            {
                // Nothing to do.
            }            

            // b. copy cipher value
            ACI_TEST( aTemplate->encrypt( aCanonInfo,
                                          (acp_char_t*) aCanon->mColumnAttr.mEncAttr.mPolicy,
                                          sPlain,
                                          aCanon->precision,
                                          sCanonized->mValue,
                                          & sCanonized->mCipherLength )
                      != ACI_SUCCESS );
        
            // c. copy ecc value
            ACI_TEST_RAISE( sCanonized->mCipherLength + sValue->mEccLength >
                            aCanon->mColumnAttr.mEncAttr.mEncPrecision,
                            ERR_INVALID_LENGTH );
            
            acpMemCpy( sCanonized->mValue + sCanonized->mCipherLength,
                       sValue->mValue + sValue->mCipherLength,
                       sValue->mEccLength );
        
            sCanonized->mEccLength = sValue->mEccLength;
        }
    }
    else 
    {
        //-----------------------------------------------------
        // case 4. policy1 -> policy2
        //-----------------------------------------------------

        ACE_ASSERT( aColumnInfo != NULL );
        ACE_ASSERT( aCanonInfo != NULL );

        if( sValue->mEccLength == 0 )
        {
            ACE_ASSERT( sValue->mCipherLength == 0 );
                
            *aCanonized = aValue;
        }
        else
        {
            // a. decrypt
            if( sValue->mCipherLength > 0 )
            {            
                ACI_TEST( aTemplate->decrypt( aColumnInfo,
                                              (acp_char_t*) aColumn->mColumnAttr.mEncAttr.mPolicy,
                                              sValue->mValue,
                                              sValue->mCipherLength,
                                              sPlain,
                                              & sPlainLength )
                          != ACI_SUCCESS );
                    
                ACE_ASSERT( sPlainLength <= aColumn->precision );
                    
                ACI_TEST_RAISE( sPlainLength > aCanon->precision,
                                ERR_INVALID_LENGTH );
            }
            else
            {
                sPlainLength = 0;
            }

            // b. padding
            if( aCanon->precision - sPlainLength > 0 )
            {
                acpMemSet( sPlain + sPlainLength,
                           ' ',
                           aCanon->precision - sPlainLength );
            }
            else
            {
                // Nothing to do.
            }    
                
            // c. copy cipher value
            ACI_TEST( aTemplate->encrypt( aCanonInfo,
                                          (acp_char_t*) aCanon->mColumnAttr.mEncAttr.mPolicy,
                                          sPlain,
                                          aCanon->precision,
                                          sCanonized->mValue,
                                          & sCanonized->mCipherLength )
                      != ACI_SUCCESS );
                
            // d. copy ecc value
            ACI_TEST_RAISE( sCanonized->mCipherLength + sValue->mEccLength >
                            aCanon->mColumnAttr.mEncAttr.mEncPrecision,
                            ERR_INVALID_LENGTH );
            
            acpMemCpy( sCanonized->mValue + sCanonized->mCipherLength,
                       sValue->mValue + sValue->mCipherLength,
                       sValue->mEccLength );
            
            sCanonized->mEccLength = sValue->mEccLength;
        }
    }
    
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;
    
    sLength = (acp_uint8_t*)&(((mtdEcharType*)aValue)->mCipherLength);
    
    sIntermediate = sLength[0];
    sLength[0]    = sLength[1];
    sLength[1]    = sIntermediate;

    sLength = (acp_uint8_t*)&(((mtdEcharType*)aValue)->mEccLength);
    
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
 * Description : value의 semantic 검사 및 mtcColumn 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
        
    mtdEcharType * sEcharVal = (mtdEcharType*)aValue;
    ACI_TEST_RAISE( sEcharVal == NULL, ERR_INVALID_NULL );
    
    ACI_TEST_RAISE( aValueSize < sizeof(acp_uint16_t), ERR_INVALID_LENGTH);
    ACI_TEST_RAISE( sEcharVal->mCipherLength + sEcharVal->mEccLength
                    + sizeof(acp_uint16_t) + sizeof(acp_uint16_t) != aValueSize,
                    ERR_INVALID_LENGTH );
    
    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdEchar,
                                   1,                           // arguments
                                   sEcharVal->mCipherLength,    // precision
                                   0 )
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

    mtdEcharType* sEcharValue;

    sEcharValue = (mtdEcharType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sEcharValue->mCipherLength = 0;
        sEcharValue->mEccLength = 0;
    }
    else
    {
        ACI_TEST_RAISE( (aDestValueOffset + aLength)  > aColumnSize, ERR_INVALID_STORED_VALUE );

        acpMemCpy( (acp_uint8_t*)sEcharValue + aDestValueOffset,
                   aValue,
                   aLength );
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
 * 예 ) mtdEcharType( acp_uint16_t length; acp_uint8_t value[1] ) 에서
 *      length 타입인 acp_uint16_t의 크기를 반환
 *******************************************************************/

    return mtdActualSize( NULL,
                          & mtcdEcharNull,
                          MTD_OFFSET_USELESS );
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdEcharType( acp_uint16_t length; acp_uint8_t value[1] ) 에서
 *      length 타입인 acp_uint16_t의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint16_t) + sizeof(acp_uint16_t);
}

