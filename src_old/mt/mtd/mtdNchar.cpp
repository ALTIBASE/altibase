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

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtdModule mtdNvarchar;
extern mtdModule mtdNchar;

extern mtlModule mtlUTF16;

const mtdNcharType mtdNcharNull = { 0, {'\0',} };

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

static IDE_RC mtdGetPrecision( const mtcColumn * aColumn,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale );

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static UInt mtdHash( UInt             aHash,
                     const mtcColumn* aColumn,
                     const void*      aRow );

static idBool mtdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static SInt mtdNcharLogicalAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 );

static SInt mtdNcharLogicalDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdNcharFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static SInt mtdNcharFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                 mtdValueInfo * aValueInfo2 );

static SInt mtdNcharMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdNcharMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                       mtdValueInfo * aValueInfo2 );

static SInt mtdNcharStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdNcharStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static SInt mtdNcharStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

static SInt mtdNcharStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                             mtdValueInfo * aValueInfo2 );

/* PROJ-2433 */
static SInt mtdNcharIndexKeyFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static SInt mtdNcharIndexKeyFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                 mtdValueInfo * aValueInfo2 );

static SInt mtdNcharIndexKeyMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdNcharIndexKeyMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );
/* PROJ-2433 end */

static IDE_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonized,
                           mtcEncryptInfo  * aCanonInfo,
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * aColumnInfo,
                           mtcTemplate     * aTemplate );

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
    { NULL, 5, (void*)"NCHAR"  }
};

static mtcColumn mtdColumn;

mtdModule mtdNchar = {
    mtdTypeName,
    &mtdColumn,
    MTD_NCHAR_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
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
    MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
    MTD_PSM_TYPE_ENABLE, // PROJ-1904
    0, // mtd::modifyNls4MtdModule시에 결정됨
    0,
    0,
    (void*)&mtdNcharNull,
    mtdInitialize,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdSetNull,
    mtdHash,
    mtdIsNull,
    mtd::isTrueNA,
    {
        mtdNcharLogicalAscComp,    // Logical Comparison
        mtdNcharLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdNcharFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdNcharFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
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
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdNcharIndexKeyFixedMtdKeyAscComp,
            mtdNcharIndexKeyFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdNcharIndexKeyMtdKeyAscComp,
            mtdNcharIndexKeyMtdKeyDescComp
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtd::selectivityDefault,
    mtd::encodeCharDefault,
    mtd::decodeDefault,
    mtd::compileFmtDefault,
    mtdValueFromOracle,
    mtd::makeColumnInfoDefault,

    // BUG-28934
    mtk::mergeAndRangeDefault,
    mtk::mergeOrRangeListDefault,

    {
        // PROJ-1705
        mtdStoredValue2MtdValue,
        // PROJ-2429
        mtd::mtdStoredValue2MtdValue4CompressColumn
    },
    mtdNullValueSize,
    mtdHeaderSize,

    //PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdNchar, aNo )
              != IDE_SUCCESS );
    
    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdNchar,
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
        *aPrecision = MTD_NCHAR_PRECISION_DEFAULT;
    }

    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    if( mtl::mNationalCharSet->id == MTL_UTF8_ID )
    {
        IDE_TEST_RAISE( (*aPrecision < (SInt)MTD_NCHAR_PRECISION_MINIMUM) ||
                        (*aPrecision > (SInt)MTD_UTF8_NCHAR_PRECISION_MAXIMUM),
                        ERR_INVALID_LENGTH );

        *aColumnSize = ID_SIZEOF(UShort) + (*aPrecision * MTL_UTF8_PRECISION);
    }
    else if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
    {
        IDE_TEST_RAISE( (*aPrecision < (SInt)MTD_NCHAR_PRECISION_MINIMUM) ||
                        (*aPrecision > (SInt)MTD_UTF16_NCHAR_PRECISION_MAXIMUM),
                        ERR_INVALID_LENGTH );

        *aColumnSize = ID_SIZEOF(UShort) + (*aPrecision * MTL_UTF16_PRECISION);
    }
    else
    {
        ideLog::log( IDE_ERR_0,
                     "mtl::mNationalCharSet->id : %u\n",
                     mtl::mNationalCharSet->id );

        IDE_ASSERT( 0 );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION( ERR_INVALID_SCALE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_SCALE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdValue( mtcTemplate* aTemplate,
                 mtcColumn*   aColumn,
                 void*        aValue,
                 UInt*        aValueOffset,
                 UInt         aValueSize,
                 const void*  aToken,
                 UInt         aTokenLength,
                 IDE_RC*      aResult )
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

    UInt                sValueOffset;
    mtdNcharType      * sValue;
    UChar             * sToken;
    UChar             * sTempToken;
    UChar             * sTokenFence;
    UChar             * sIterator;
    UChar             * sFence;
    const mtlModule   * sColLanguage;
    const mtlModule   * sTokenLanguage;
    idBool              sIsSame = ID_FALSE;
    UInt                sNcharCnt = 0;
    idnCharSetList      sColCharSet;
    idnCharSetList      sTokenCharSet;
    SInt                sSrcRemain = 0;
    SInt                sDestRemain = 0;
    SInt                sTempRemain = 0;
    UChar               sSize;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_NCHAR_ALIGN );
    sValue = (mtdNcharType*)( (UChar*)aValue + sValueOffset );

    *aResult = IDE_SUCCESS;

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
        sTokenLanguage = mtl::mDBCharSet;
    }
    else
    {
        sTokenLanguage = mtl::mDBCharSet;
    }

    // To fix BUG-13444
    // tokenFence와 RowFence는 별개의 검사과정이므로,
    // 먼저 RowFence검사 후 TokenFence검사를 해야 한다.
    sIterator = sValue->value;
    sFence    = (UChar*)aValue + aValueSize;

    if( sIterator >= sFence )
    {
        *aResult = IDE_FAILURE;
    }
    else
    {
        sToken = (UChar*)aToken;
        sTokenFence = sToken + aTokenLength;

        if( (aColumn->flag & MTC_COLUMN_LITERAL_TYPE_MASK) ==
            MTC_COLUMN_LITERAL_TYPE_UNICODE )
        {
            IDE_TEST( mtdNcharInterface::toNchar4UnicodeLiteral( sTokenLanguage,
                                                                 sColLanguage,
                                                                 sToken,
                                                                 sTokenFence,
                                                                 &sIterator,
                                                                 sFence,
                                                                 & sNcharCnt )
                      != IDE_SUCCESS );
        }
        else
        {
            sColCharSet = mtl::getIdnCharSet( sColLanguage );
            sTokenCharSet = mtl::getIdnCharSet( sTokenLanguage );

            sSrcRemain = aTokenLength;
            sDestRemain = aValueSize;

            while( sToken < sTokenFence )
            {
                if( sIterator >= sFence )
                {
                    *aResult = IDE_FAILURE;
                    break;
                }

                sSize = mtl::getOneCharSize( sToken,
                                             sTokenFence,
                                             sTokenLanguage );
                
                sIsSame = mtc::compareOneChar( sToken,
                                               sSize,
                                               sTokenLanguage->specialCharSet[MTL_QT_IDX],
                                               sTokenLanguage->specialCharSize );
                
                if( sIsSame == ID_TRUE )
                {
                    (void)sTokenLanguage->nextCharPtr( & sToken, sTokenFence );
                    
                    sSize = mtl::getOneCharSize( sToken,
                                                 sTokenFence,
                                                 sTokenLanguage );
                
                    sIsSame = mtc::compareOneChar( sToken,
                                                   sSize,
                                                   sTokenLanguage->specialCharSet[MTL_QT_IDX],
                                                   sTokenLanguage->specialCharSize );
                
                    IDE_TEST_RAISE( sIsSame != ID_TRUE,
                                    ERR_INVALID_LITERAL );
                }

                sTempRemain = sDestRemain;

                if( sColLanguage->id != sTokenLanguage->id )
                {
                    IDE_TEST( convertCharSet( sTokenCharSet,
                                              sColCharSet,
                                              sToken,
                                              sSrcRemain,
                                              sIterator,
                                              & sDestRemain,
                                              MTU_NLS_NCHAR_CONV_EXCP )
                              != IDE_SUCCESS );

                    sTempToken = sToken;

                    (void)sTokenLanguage->nextCharPtr( & sToken, sTokenFence );
                }
                else
                {
                    sTempToken = sToken;

                    (void)sTokenLanguage->nextCharPtr( & sToken, sTokenFence );

                    idlOS::memcpy( sIterator, sTempToken, sToken - sTempToken );

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
    
    if( *aResult == IDE_SUCCESS )
    {
        sValue->length     = sIterator - sValue->value;

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag      = 1;

        aColumn->precision = sValue->length != 0 ? sNcharCnt : 1;

        aColumn->scale     = 0;

        IDE_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != IDE_SUCCESS );

        aColumn->column.offset   = sValueOffset;

        *aValueOffset = sValueOffset + ID_SIZEOF(UShort) +
                        sValue->length;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    *aResult = IDE_FAILURE;

    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn* ,
                    const void*      aRow )
{
    return ID_SIZEOF(UShort) + ((mtdNcharType*)aRow)->length;
}

static IDE_RC mtdGetPrecision( const mtcColumn * aColumn,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale )
{
    const mtlModule    * sLanguage;
    const mtdNcharType * sValue = ((mtdNcharType*)aRow);
    UChar              * sValueIndex;
    UChar              * sValueFence;
    UShort               sValueCharCnt = 0;

    sLanguage = aColumn->language;

    // --------------------------
    // Value의 문자 개수
    // --------------------------
    
    sValueIndex = (UChar*) sValue->value;
    sValueFence = sValueIndex + sValue->length;

    while ( sValueIndex < sValueFence )
    {
        IDE_TEST_RAISE( sLanguage->nextCharPtr( & sValueIndex, sValueFence )
                        != NC_VALID,
                        ERR_INVALID_CHARACTER );

        sValueCharCnt++;
    }

    *aPrecision = sValueCharCnt;
    *aScale = 0;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_CHARACTER );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_CHARACTER));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mtdSetNull( const mtcColumn* /*aColumn*/,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        ((mtdNcharType*)aRow)->length = 0;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hashWithoutSpace( aHash, ((mtdNcharType*)aRow)->value, ((mtdNcharType*)aRow)->length );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    return (((mtdNcharType*)aRow)->length == 0) ? ID_TRUE : ID_FALSE;
}

SInt mtdNcharLogicalAscComp( mtdValueInfo * aValueInfo1,
                             mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1     = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2     = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharLogicalDescComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar*         sIterator;
    const UChar*         sFence;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sLength1     = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sLength2     = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1     = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2     = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar*         sIterator;
    const UChar*         sFence;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1     = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2     = sNcharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)
                    mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                         aValueInfo1->value,
                                         aValueInfo1->flag,
                                         mtdNchar.staticNull );

    sLength1 = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
                    mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                         aValueInfo2->value,
                                         aValueInfo2->flag,
                                         mtdNchar.staticNull );

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
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar*         sIterator;
    const UChar*         sFence;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)
                    mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                         aValueInfo1->value,
                                         aValueInfo1->flag,
                                         mtdNchar.staticNull );

    sLength1 = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
                    mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                         aValueInfo2->value,
                                         aValueInfo2->flag,
                                         mtdNchar.staticNull );

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
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;
    idBool               sExist;
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sNcharValue1 = (const mtdNcharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sNcharValue1->length;
        sValue1  = sNcharValue1->value;
    }
    
    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
                    mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                         aValueInfo2->value,
                                         aValueInfo2->flag,
                                         mtdNchar.staticNull );

    sLength2 = sNcharValue2->length;
    sValue2  = sNcharValue2->value;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;
    idBool               sExist;
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );
        
        sNcharValue1 = (const mtdNcharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sNcharValue1->length;
        sValue1  = sNcharValue1->value;
    }

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)
                    mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                         aValueInfo2->value,
                                         aValueInfo2->flag,
                                         mtdNchar.staticNull );

    sLength2 = sNcharValue2->length;
    sValue2  = sNcharValue2->value;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;    
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;
    idBool               sExist;
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sNcharValue1 = (const mtdNcharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sNcharValue1->length;
        sValue1  = sNcharValue1->value;
    }

    //---------
    // value2
    //---------    
    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength2 = (UShort)aValueInfo2->length;
        sValue2  = (UChar*)aValueInfo2->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sNcharValue2 = (const mtdNcharType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength2 = sNcharValue2->length;
        sValue2  = sNcharValue2->value;
    }

    //---------
    // compare
    //---------    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1, sValue2, sLength2 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue1 + sLength2,
                         sFence = sValue1 + sLength1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1, sValue2, sLength1 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1,
                     sFence = sValue2 + sLength2;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue2 + sLength1, sFence = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

SInt mtdNcharStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDummy;
    idBool               sExist;
    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength1 = (UShort)aValueInfo1->length;
        sValue1  = (UChar*)aValueInfo1->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sNcharValue1 = (const mtdNcharType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength1 = sNcharValue1->length;
        sValue1  = sNcharValue1->value;
    }

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        sLength2 = (UShort)aValueInfo2->length;
        sValue2  = (UChar*)aValueInfo2->value;
        
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sNcharValue2 = (const mtdNcharType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
        sLength2 = sNcharValue2->length;
        sValue2  = sNcharValue2->value;
    }

    //---------
    // compare
    //---------    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        if( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2, sValue1, sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }

            if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence - 1;
                     sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for( sIterator = sValue2 + sLength1,
                         sFence = sValue2 + sLength2;
                     sIterator < sFence;
                     sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2, sValue1, sLength2 );
        if( sCompared != 0 )
        {
            return sCompared;
        }

        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence - 1;
                 sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for( sIterator = sValue1 + sLength2, sFence = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
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

/* PROJ-2433
 * Direct key Index의 direct key와 mtd의 compare 함수
 * - partial direct key를 처리하는부분 추가 */
SInt mtdNcharIndexKeyFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 )
{
    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1     = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2     = sNcharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------
    
    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if ( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1,
                                       sValue2,
                                       sLength2 );
            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing todo */
            }

            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue1 + sLength2 ),
                      sFence = ( sValue1 + sLength1 ) ;
                      ( sIterator < ( sFence - 1 ) ) ;
                      sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue1 + sLength2 ),
                      sFence = ( sValue1 + sLength1 ) ;
                      ( sIterator < sFence ) ;
                      sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        else
        {
            /* nothing todo */
        }
        sCompared = idlOS::memcmp( sValue1,
                                   sValue2,
                                   sLength1 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        else
        {
            /* nothing todo */
        }

        if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue2 + sLength1 ),
                  sFence = ( sValue2 + sLength2 ) ;
                  ( sIterator < ( sFence - 1 ) ) ;
                  sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue2 + sLength1 ), sFence = ( sValue2 + sLength2 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
        }
        return 0;
    }
    else
    {
        /* nothing todo */
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    else
    {
        /* nothing todo */
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    else
    {
        /* nothing todo */
    }
    return 0;
}

SInt mtdNcharIndexKeyFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 )
{
    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar*         sIterator;
    const UChar*         sFence;
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo1 );
    sLength1     = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*)MTD_VALUE_FIXED( aValueInfo2 );
    sLength2     = sNcharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------
    
    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if ( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2,
                                       sValue1,
                                       sLength1 );
            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing todo */
            }

            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue2 + sLength1 ),
                      sFence = ( sValue2 + sLength2 ) ;
                      ( sIterator < ( sFence - 1 ) ) ;
                      sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue2 + sLength1 ),
                      sFence = ( sValue2 + sLength2 ) ;
                      ( sIterator < sFence ) ;
                      sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2,
                                   sValue1,
                                   sLength2 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        else
        {
            /* nothing todo */
        }

        if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
                  ( sIterator < ( sFence - 1 ) ) ;
                  sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
        }
        return 0;
    }
    else
    {
        /* nothing todo */
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    else
    {
        /* nothing todo */
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    else
    {
        /* nothing todo */
    }
    return 0;
}

SInt mtdNcharIndexKeyMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*) mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                                              aValueInfo1->value,
                                                              aValueInfo1->flag,
                                                              mtdNchar.staticNull );

    sLength1 = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*) mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                                              aValueInfo2->value,
                                                              aValueInfo2->flag,
                                                              mtdNchar.staticNull );

    sLength2 = sNcharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------
    
    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;
        
        if ( sLength1 >= sLength2 )
        {
            sCompared = idlOS::memcmp( sValue1,
                                       sValue2,
                                       sLength2 );
            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing todo */
            }

            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue1 + sLength2 ),
                      sFence = ( sValue1 + sLength1 ) ;
                      ( sIterator < ( sFence - 1 ) ) ;
                      sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue1 + sLength2 ),
                      sFence = ( sValue1 + sLength1 ) ;
                      ( sIterator < sFence ) ;
                      sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue1,
                                   sValue2,
                                   sLength1 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        else
        {
            /* nothing todo */
        }

        if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue2 + sLength1 ),
                  sFence = ( sValue2 + sLength2 ) ;
                  ( sIterator < ( sFence - 1 ) ) ;
                  sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue2 + sLength1 ), sFence = ( sValue2 + sLength2 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
        }
        return 0;
    }
    else
    {
        /* nothing todo */
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    else
    {
        /* nothing todo */
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    else
    {
        /* nothing todo */
    }
    return 0;
}

SInt mtdNcharIndexKeyMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
    const mtdNcharType * sNcharValue1;
    const mtdNcharType * sNcharValue2;
    UShort               sLength1 = 0;
    UShort               sLength2 = 0;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar*         sIterator;
    const UChar*         sFence;
    UInt                 sDirectKeyPartialSize;
    idBool               sExist;
    //---------
    // value1
    //---------
    sNcharValue1 = (const mtdNcharType*) mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                                              aValueInfo1->value,
                                                              aValueInfo1->flag,
                                                              mtdNchar.staticNull );

    sLength1 = sNcharValue1->length;

    //---------
    // value2
    //---------
    sNcharValue2 = (const mtdNcharType*) mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                                              aValueInfo2->value,
                                                              aValueInfo2->flag,
                                                              mtdNchar.staticNull );

    sLength2 = sNcharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdHeaderSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdHeaderSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------

    if( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sNcharValue1->value;
        sValue2  = sNcharValue2->value;

        if ( sLength2 >= sLength1 )
        {
            sCompared = idlOS::memcmp( sValue2,
                                       sValue1,
                                       sLength1 );
            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                /* nothing todo */
            }

            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue2 + sLength1 ),
                      sFence = ( sValue2 + sLength2 ) ;
                      ( sIterator < ( sFence - 1 ) ) ;
                      sIterator += MTL_UTF16_PRECISION )
                {
                    // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                    if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            if ( sExist == ID_FALSE )
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                        else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                        {
                            sExist = ID_TRUE;
                        }
                    }
                }
            }
            else
            {
                sExist = ID_FALSE;
                for ( sIterator = ( sValue2 + sLength1 ),
                      sFence = ( sValue2 + sLength2 ) ;
                      ( sIterator < sFence ) ;
                      sIterator++ )
                {
                    if ( *sIterator > 0x20 )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if ( *sIterator < 0x20 )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
            return 0;
        }
        sCompared = idlOS::memcmp( sValue2,
                                   sValue1,
                                   sLength2 );
        if ( sCompared != 0 )
        {
            return sCompared;
        }
        else
        {
            /* nothing todo */
        }

        if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
                  sIterator < ( sFence - 1 ) ;
                  sIterator += MTL_UTF16_PRECISION )
            {
                // BUG-41209 UTF16BE space는 { 0x00 0x20 }이다.
                if ( *sIterator > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][0] )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if ( *(sIterator + 1) > mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        if ( sExist == ID_FALSE )
                        {
                            return -1;
                        }
                        else
                        {
                            return 1;
                        }
                    }
                    else if ( *(sIterator + 1) < mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX][1] )
                    {
                        sExist = ID_TRUE;
                    }
                }
            }
        }
        else
        {
            sExist = ID_FALSE;
            for ( sIterator = ( sValue1 + sLength2 ), sFence = ( sValue1 + sLength1 ) ;
                  ( sIterator < sFence ) ;
                  sIterator++ )
            {
                if ( *sIterator > 0x20 )
                {
                    if ( sExist == ID_FALSE )
                    {
                        return -1;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if ( *sIterator < 0x20 )
                {
                    sExist = ID_TRUE;
                }
            }
        }
        return 0;
    }
    else
    {
        /* nothing todo */
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

static IDE_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonized,
                           mtcEncryptInfo  * /* aCanonInfo */,
                           const mtcColumn * /* aColumn */,
                           void*             aValue,
                           mtcEncryptInfo  * /* aColumnInfo */,
                           mtcTemplate     * /* aTemplate */ )
{
    const mtlModule * sLanguage;
    mtdNcharType    * sCanonized;
    mtdNcharType    * sValue;
    //UShort            sCanonBytePrecision;
    UChar           * sValueIndex;
    UChar           * sValueFence;
    UShort            sValueCharCnt = 0;
    UShort            i = 0;
    
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
        if( mtl::mNationalCharSet->id == MTL_UTF16_ID )
        {
            IDE_TEST_RAISE ( sValueCharCnt > aCanon->precision,
                             ERR_INVALID_LENGTH );

            sCanonized = (mtdNcharType*)*aCanonized;
            
            sCanonized->length = sValue->length +
                ( (aCanon->precision - sValueCharCnt) * 
                  MTL_UTF16_PRECISION );
            
            idlOS::memcpy( sCanonized->value, 
                           sValue->value, 
                           sValue->length );
            
            for( i = 0; 
                 i < (sCanonized->length - sValue->length);
                 i += MTL_UTF16_PRECISION )
            {
                idlOS::memcpy( sCanonized->value + sValue->length + i,
                               mtl::mNationalCharSet->specialCharSet[MTL_SP_IDX],
                               MTL_UTF16_PRECISION );
            }
        }
        else
        {
            IDE_TEST_RAISE ( sValueCharCnt > aCanon->precision,
                             ERR_INVALID_LENGTH );

            sCanonized = (mtdNcharType*)*aCanonized;
            
            sCanonized->length = sValue->length +
                ( (aCanon->precision - sValueCharCnt) ); 
            
            idlOS::memcpy( sCanonized->value, 
                           sValue->value, 
                           sValue->length );
            
            idlOS::memset( sCanonized->value + sValue->length,
                           0x20,
                           sCanonized->length - sValue->length );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void mtdEndian( void* aValue )
{
    UChar* sLength;
    UChar  sIntermediate;

    sLength = (UChar*)&(((mtdNcharType*)aValue)->length);

    sIntermediate = sLength[0];
    sLength[0]    = sLength[1];
    sLength[1]    = sIntermediate;
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
        
    mtdNcharType * sCharVal = (mtdNcharType*)aValue;
    IDE_TEST_RAISE( sCharVal == NULL, ERR_INVALID_NULL );

    IDE_TEST_RAISE( aValueSize < ID_SIZEOF(UShort), ERR_INVALID_LENGTH);
    IDE_TEST_RAISE( sCharVal->length + ID_SIZEOF(UShort) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdNchar,
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
    UInt           sValueOffset;
    mtdNcharType * sValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_NCHAR_ALIGN );
    
    if( aOracleLength < 0 )
    {
        aOracleLength = 0;
    }

    // aColumn의 초기화
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdNchar,
                                     1,
                                     ( aOracleLength > 1 ) ? aOracleLength : 1,
                                     0 )
        != IDE_SUCCESS );
    
    if( sValueOffset + aColumn->column.size <= aValueSize )
    {
        sValue = (mtdNcharType*)((UChar*)aValue+sValueOffset);
        
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

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue )
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
        IDE_TEST_RAISE( (aDestValueOffset + aLength + mtdHeaderSize()) > aColumnSize, ERR_INVALID_STORED_VALUE );

        sNcharValue->length = (UShort)(aDestValueOffset + aLength);
        idlOS::memcpy( (UChar*)sNcharValue + mtdHeaderSize() + aDestValueOffset, aValue, aLength );
    }
    
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
/*******************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 * 예 ) mtdNcharType ( UShort length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *******************************************************************/
    return mtdActualSize( NULL, &mtdNcharNull );
}

static UInt mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdNcharType ( UShort length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return ID_SIZEOF(UShort);
}

IDE_RC mtdNcharInterface::toNchar( mtcStack         * aStack,
                                   const mtlModule  * aSrcCharSet,
                                   const mtlModule  * aDestCharSet,
                                   mtdCharType      * aSource,
                                   mtdNcharType     * aResult )
{
/***********************************************************************
 *
 * Description : 
 *      CHAR 타입을 NCHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    idnCharSetList   sIdnDestCharSet;
    idnCharSetList   sIdnSrcCharSet;
    UChar          * sSourceIndex;
    UChar          * sTempIndex;
    UChar          * sSourceFence;
    UChar          * sResultValue;
    UChar          * sResultFence;
    UInt             sNcharCnt = 0;
    SInt             sSrcRemain = 0;
    SInt             sDestRemain = 0;
    SInt             sTempRemain = 0;

    // 변환 결과의 크기를 체크하기 위함
    sDestRemain = aDestCharSet->maxPrecision(aStack[0].column->precision);

    sSourceIndex = aSource->value;
    sSrcRemain   = aSource->length;
    sSourceFence = sSourceIndex + aSource->length;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      MTU_NLS_NCHAR_CONV_EXCP )
                      != IDE_SUCCESS );

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
        idlOS::memcpy( aResult->value,
                       aSource->value,
                       aSource->length );

        aResult->length = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdNcharInterface::toNchar( mtcStack         * aStack,
                                   const mtlModule  * aSrcCharSet,
                                   const mtlModule  * aDestCharSet,
                                   UChar            * aSource,
                                   UShort             aSourceLen,
                                   mtdNcharType     * aResult )
{
/***********************************************************************
 *
 * Description : 
 *      CHAR 타입을 NCHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    idnCharSetList   sIdnDestCharSet;
    idnCharSetList   sIdnSrcCharSet;
    UChar          * sSourceIndex;
    UChar          * sTempIndex;
    UChar          * sSourceFence;
    UChar          * sResultValue;
    UChar          * sResultFence;
    UInt             sNcharCnt = 0;
    SInt             sSrcRemain = 0;
    SInt             sDestRemain = 0;
    SInt             sTempRemain = 0;
    
    // 변환 결과의 크기를 체크하기 위함
    sDestRemain = aDestCharSet->maxPrecision(aStack[0].column->precision);

    sSourceIndex = aSource;
    sSrcRemain   = aSourceLen;
    sSourceFence = sSourceIndex + aSourceLen;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      MTU_NLS_NCHAR_CONV_EXCP )
                      != IDE_SUCCESS );

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
        idlOS::memcpy( aResult->value,
                       aSource,
                       aSourceLen );

        aResult->length = aSourceLen;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdNcharInterface::toChar( mtcStack         * aStack,
                                  const mtlModule  * aSrcCharSet,
                                  const mtlModule  * aDestCharSet,
                                  mtdNcharType     * aSource,
                                  mtdCharType      * aResult )
{
/***********************************************************************
 *
 * Description :
 *      NCHAR 타입을 CHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    idnCharSetList   sIdnDestCharSet;
    idnCharSetList   sIdnSrcCharSet;
    UChar          * sSourceIndex;
    UChar          * sTempIndex;
    UChar          * sSourceFence;
    UChar          * sResultValue;
    UChar          * sResultFence;
    SInt             sSrcRemain = 0;
    SInt             sDestRemain = 0;
    SInt             sTempRemain = 0;
    UShort           sSourceLen;

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
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      MTU_NLS_NCHAR_CONV_EXCP )
                      != IDE_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );
            
            sResultValue += (sTempRemain - sDestRemain);
            
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        aResult->length = sResultValue - aResult->value;
    }
    else
    {
        idlOS::memcpy( aResult->value,
                       aSource->value,
                       aSource->length );

        aResult->length = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdNcharInterface::toChar( mtcStack         * aStack,
                                  const mtlModule  * aSrcCharSet,
                                  const mtlModule  * aDestCharSet,
                                  mtdNcharType     * aSource,
                                  UChar            * aResult,
                                  UShort           * aResultLen )
{
/***********************************************************************
 *
 * Description :
 *      NCHAR 타입을 CHAR 타입으로 변환하는 함수

 * Implementation :
 *
 ***********************************************************************/

    idnCharSetList   sIdnDestCharSet;
    idnCharSetList   sIdnSrcCharSet;
    UChar          * sSourceIndex;
    UChar          * sTempIndex;
    UChar          * sSourceFence;
    UChar          * sResultValue;
    UChar          * sResultFence;
    SInt             sSrcRemain = 0;
    SInt             sDestRemain = 0;
    SInt             sTempRemain = 0;
    UShort           sSourceLen;

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
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceLen-- > 0 )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      MTU_NLS_NCHAR_CONV_EXCP )
                      != IDE_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );
            
            sResultValue += (sTempRemain - sDestRemain);
            
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        *aResultLen = sResultValue - aResult;
    }
    else
    {
        idlOS::memcpy( aResult,
                       aSource->value,
                       aSource->length );

        *aResultLen = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdNcharInterface::toNchar( SInt               aPrecision,
                                   const mtlModule  * aSrcCharSet,
                                   const mtlModule  * aDestCharSet,
                                   mtdCharType      * aSource,
                                   mtdNcharType     * aResult )
{
/***********************************************************************
 *
 * Description : 
 *      CHAR 타입을 NCHAR 타입으로 변환하는 함수
 *      PROJ-2632
 *
 * Implementation :
 *
 ***********************************************************************/

    idnCharSetList   sIdnDestCharSet;
    idnCharSetList   sIdnSrcCharSet;
    UChar          * sSourceIndex;
    UChar          * sTempIndex;
    UChar          * sSourceFence;
    UChar          * sResultValue;
    UChar          * sResultFence;
    UInt             sNcharCnt = 0;
    SInt             sSrcRemain = 0;
    SInt             sDestRemain = 0;
    SInt             sTempRemain = 0;

    sDestRemain  = aDestCharSet->maxPrecision( aPrecision );
    sSourceIndex = aSource->value;
    sSrcRemain   = aSource->length;
    sSourceFence = sSourceIndex + aSource->length;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      MTU_NLS_NCHAR_CONV_EXCP )
                      != IDE_SUCCESS );

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
        idlOS::memcpy( aResult->value,
                       aSource->value,
                       aSource->length );

        aResult->length = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdNcharInterface::toChar( SInt               aPrecision,
                                  const mtlModule  * aSrcCharSet,
                                  const mtlModule  * aDestCharSet,
                                  mtdNcharType     * aSource,
                                  mtdCharType      * aResult )
{
/***********************************************************************
 *
 * Description :
 *      NCHAR 타입을 CHAR 타입으로 변환하는 함수
 *      PROJ-2632
 *
 * Implementation :
 *
 ***********************************************************************/

    idnCharSetList   sIdnDestCharSet;
    idnCharSetList   sIdnSrcCharSet;
    UChar          * sSourceIndex;
    UChar          * sTempIndex;
    UChar          * sSourceFence;
    UChar          * sResultValue;
    UChar          * sResultFence;
    SInt             sSrcRemain = 0;
    SInt             sDestRemain = 0;
    SInt             sTempRemain = 0;
    UShort           sSourceLen;

    sDestRemain  = aPrecision;
    sSourceIndex = aSource->value;
    sSourceLen   = aSource->length;
    sSourceFence = sSourceIndex + sSourceLen;
    sSrcRemain   = sSourceLen;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      MTU_NLS_NCHAR_CONV_EXCP )
                      != IDE_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );
            
            sResultValue += (sTempRemain - sDestRemain);
            
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        aResult->length = sResultValue - aResult->value;
    }
    else
    {
        idlOS::memcpy( aResult->value,
                       aSource->value,
                       aSource->length );

        aResult->length = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdNcharInterface::toNchar4UnicodeLiteral(
    const mtlModule * aSourceCharSet,
    const mtlModule * aResultCharSet,
    UChar           * aSourceIndex,
    UChar           * aSourceFence,
    UChar          ** aResultValue,
    UChar           * aResultFence,
    UInt            * aNcharCnt )
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

    UChar             * sTempSourceIndex;
    const mtlModule   * sU16CharSet;
    idnCharSetList      sIdnSourceCharSet;
    idnCharSetList      sIdnResultCharSet;
    idnCharSetList      sIdnU16CharSet;
    idBool              sIsSame = ID_FALSE;
    SInt                sSrcRemain = 0;
    SInt                sDestRemain = 0;
    SInt                sTempRemain = 0;
    UInt                sNibbleLength = 0;
    UChar               sNibbleValue[2];
    UInt                i;
    UChar               sSize;
    
    sU16CharSet = & mtlUTF16;

    sIdnSourceCharSet = mtl::getIdnCharSet( aSourceCharSet );
    sIdnResultCharSet = mtl::getIdnCharSet( aResultCharSet );
    sIdnU16CharSet    = mtl::getIdnCharSet( sU16CharSet );

    // 캐릭터 셋 변환 시 사용하는 버퍼의 길이
    sDestRemain = aResultFence - *aResultValue;

    // 소스의 길이
    sSrcRemain = aSourceFence - aSourceIndex;

    sTempSourceIndex = aSourceIndex;

    while( aSourceIndex < aSourceFence )
    {
        IDE_TEST_RAISE( *aResultValue >= aResultFence,
                        ERR_INVALID_DATA_LENGTH );

        sSize = mtl::getOneCharSize( aSourceIndex,
                                     aSourceFence,
                                     aSourceCharSet );

        sIsSame = mtc::compareOneChar( aSourceIndex,
                                       sSize,
                                       aSourceCharSet->specialCharSet[MTL_QT_IDX],
                                       aSourceCharSet->specialCharSize );

        if( sIsSame == ID_TRUE )
        {
            (void)aSourceCharSet->nextCharPtr( & aSourceIndex, aSourceFence );
            
            sSize = mtl::getOneCharSize( aSourceIndex,
                                         aSourceFence,
                                         aSourceCharSet );
            
            sIsSame = mtc::compareOneChar( aSourceIndex,
                                           sSize,
                                           aSourceCharSet->specialCharSet[MTL_QT_IDX],
                                           aSourceCharSet->specialCharSize );
            
            IDE_TEST_RAISE( sIsSame != ID_TRUE,
                            ERR_INVALID_LITERAL );
        }


        sSize = mtl::getOneCharSize( aSourceIndex,
                                     aSourceFence,
                                     aSourceCharSet );
        
        sIsSame = mtc::compareOneChar( aSourceIndex,
                                       sSize,
                                       aSourceCharSet->specialCharSet[MTL_BS_IDX],
                                       aSourceCharSet->specialCharSize );

        if( sIsSame == ID_TRUE )
        {
            sTempSourceIndex = aSourceIndex;

            // 16진수 문자 4개를 읽는다.
            for( i = 0; i < 4; i++ )
            {
                IDE_TEST_RAISE( aSourceCharSet->nextCharPtr( & aSourceIndex, aSourceFence )
                                != NC_VALID, ERR_INVALID_U_TYPE_STRING );
            }

            (void)aSourceCharSet->nextCharPtr( & sTempSourceIndex, aSourceFence );
            
            // UTF16 값을 sNibbleValue에 받아온다.
            IDE_TEST_RAISE( mtc::makeNibble( sNibbleValue,
                                             0,
                                             sTempSourceIndex,
                                             4,
                                             & sNibbleLength )
                            != IDE_SUCCESS, ERR_INVALID_U_TYPE_STRING );

            sTempRemain = sDestRemain;

            if( aResultCharSet->id != MTL_UTF16_ID )
            {
                // UTF16 캐릭터 셋 => 내셔널 캐릭터 셋으로 변환한다.
                IDE_TEST( convertCharSet( sIdnU16CharSet,
                                          sIdnResultCharSet,
                                          sNibbleValue,
                                          MTL_UTF16_PRECISION,
                                          *aResultValue,
                                          & sDestRemain,
                                          MTU_NLS_NCHAR_CONV_EXCP )
                          != IDE_SUCCESS );
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
                IDE_TEST( convertCharSet( sIdnSourceCharSet,
                                          sIdnResultCharSet,
                                          aSourceIndex,
                                          sSrcRemain,
                                          *aResultValue,
                                          & sDestRemain,
                                          MTU_NLS_NCHAR_CONV_EXCP )
                          != IDE_SUCCESS );
            }
            // 데이터 베이스 캐릭터 셋 = 내셔널 캐릭터 셋 = UTF8
            else
            {
                sTempSourceIndex = aSourceIndex;

                (void)aSourceCharSet->nextCharPtr( & aSourceIndex, aSourceFence );
                
                idlOS::memcpy( *aResultValue,
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    }

    IDE_EXCEPTION( ERR_INVALID_U_TYPE_STRING );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_U_TYPE_STRING));
    }

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
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
