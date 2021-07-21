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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnCommunication.h>
#include <ulnBindParamDataIn.h>
#include <ulnConv.h>
#include <ulnConvNumeric.h>
#include <mtErrorCodeClient.h>

// proj_2160 cm_type removal
// used to get default c_type when the user uses SQL_C_DEFAULT type
// : char_char, char_nchar, to_smallint, to_integer, to_bigint
ulnCTypeID ulnGetDefaultUlCType(ulnMeta* aIpdMeta)
{
    ulnMTypeID           sMType;
    acp_sint16_t         sDefSQLCType;

    sMType       = aIpdMeta->mMTYPE;
    sDefSQLCType = ulnTypeGetDefault_SQL_C_TYPE(sMType);
    return ulnTypeMap_SQLC_CTYPE(sDefSQLCType);

}

// from c_char, c_wchar to char, varchar
ACI_RC ulnParamDataInBuildAny_CHAR_CHAR(ulnFnContext *aFnContext,
                                        ulnDescRec   *aDescRecApd,
                                        ulnDescRec   *aDescRecIpd,
                                        void         *aUserDataPtr,
                                        acp_sint32_t  aUserOctetLength,
                                        acp_uint32_t  aRowNo0Based,
                                        acp_uint8_t  *aConversionBuffer,
                                        ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    acp_sint32_t         sTransferSize;
    acp_bool_t           sIsNull = ACP_FALSE;
    ulnCTypeID           sCType;
    acp_uint16_t         sCopyLen = 0;
    const mtlModule*     sCliCharset;
    acp_sint32_t         sSrcLen = 0;

    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    if (sCType == ULN_CTYPE_DEFAULT)
    {
        sCType = ulnGetDefaultUlCType(&(aDescRecIpd->mMeta));
    }
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sTransferSize,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
    }
    else
    {
        if (sCType == ULN_CTYPE_CHAR)
        {
            sCliCharset = sDbc->mClientCharsetLangModule;
        }
        else // ctype_wchar
        {
            sCliCharset = sDbc->mWcharCharsetLangModule;
        }

        /*
         * BUG-44205 The data were bound with WCHAR type are truncated its precision.
         *
         * http://nok.altibase.com/x/eRYqAg
         */
        if ((sCliCharset->id              == MTL_UTF16_ID) ||
            (sDbc->mCharsetLangModule->id == MTL_UTF16_ID))
        {
            sSrcLen = sTransferSize;
        }
        else if (sCliCharset != sDbc->mCharsetLangModule)
        {
            sSrcLen = ACP_MIN(sTransferSize, aDescRecApd->mBindInfo.mPrecision);
        }
        else
        {
            /* A obsolete convention */
        }

        if ((sCliCharset      != sDbc->mCharsetLangModule) ||
            (sCliCharset->id              == MTL_UTF16_ID) ||
            (sDbc->mCharsetLangModule->id == MTL_UTF16_ID))
        {
            // 데이터가 손실되지 않는 경우에 한해서 PRECISION만큼만 보낸다.
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sCliCharset,  //BUG-22684
                                       sDbc->mCharsetLangModule,
                                       aUserDataPtr,
                                       sSrcLen,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sCopyLen = aCharSet->mDestLen;

            ULN_CHUNK_WR2(sStmt, &sCopyLen);
            ULN_CHUNK_WCP(sStmt, ulnCharSetGetConvertedText(aCharSet), sCopyLen);
        }
        else
        {
            sCopyLen = ACP_MIN(sTransferSize, aDescRecApd->mBindInfo.mPrecision);

            ULN_CHUNK_WR2(sStmt, &sCopyLen);
            ULN_CHUNK_WCP(sStmt, aUserDataPtr, sCopyLen);
        }
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_NUMERIC(ulnFnContext *aFnContext,
                                           ulnDescRec   *aDescRecApd,
                                           ulnDescRec   *aDescRecIpd,
                                           void         *aUserDataPtr,
                                           acp_sint32_t  aUserOctetLength,
                                           acp_uint32_t  aRowNo0Based,
                                           acp_uint8_t  *aConversionBuffer,
                                           ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    acp_uint8_t          sNumericBuf[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType      *sNumeric = (mtdNumericType*)sNumericBuf;

    acp_uint8_t          sNumericBuf2[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType      *sNumeric2 = (mtdNumericType*)sNumericBuf2;

    acp_bool_t           sCanonized;
    ulnBindInfo*         sBindInfo = &(aDescRecApd->mBindInfo);

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);
    
    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    /* typedef struct mtdNumericType {
       acp_uint8_t length;
       acp_uint8_t signExponent;
       acp_uint8_t mantissa[1];
       } mtdNumericType;
       */
    if( sIsNull == ACP_TRUE )
    {
        //const mtdNumericType mtdNumericNull = { 0, 0, {'\0',} };
        ULN_CHUNK_WR1(sStmt, mtcdNumericNull.length);
    }
    else
    {
        // WCHAR 데이터인 경우 문자셋변환을 해주어야 한다.
        // ex) '37': 0x00 0x37 (2bytes) => 0x37 (1byte)
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( mtcMakeNumeric( sNumeric,
                                        MTD_NUMERIC_MANTISSA_MAXIMUM,
                                        sUserData,
                                        sDataSize )
                        != ACI_SUCCESS, LABEL_MTCONV_ERROR );

        ACI_TEST_RAISE( mtcNumericCanonize( sNumeric,
                                            sNumeric2,
                                            sBindInfo->mPrecision,
                                            // 정보가 있을까?
                                            sBindInfo->mScale,
                                            &sCanonized )
                        != ACI_SUCCESS, LABEL_MTCONV_ERROR );

        // bbae
        if( sCanonized == ACP_FALSE )
        {
            acpMemCpy( sNumeric2, sNumeric, sNumeric->length + ACI_SIZEOF(acp_uint8_t));
        }

        ULN_CHUNK_WR1(sStmt, sNumeric2->length);
        if (sNumeric2->length > 0)
        {
            ULN_CHUNK_WR1(sStmt, sNumeric2->signExponent);
            ULN_CHUNK_WCP(sStmt, sNumeric2->mantissa, sNumeric2->length - 1);
        }
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_MTCONV_ERROR)
    {
        if (aciGetErrorCode() == mtERR_ABORT_VALUE_OVERFLOW)
        {
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1, 
                             aDescRecApd->mIndex,
                             ulERR_ABORT_Data_Value_Overflow_Error,
                             "Numeric");
        }
        else
        {
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1,
                             aDescRecApd->mIndex,
                             ulERR_ABORT_INVALID_NUMBER_STRING,
                             sDataSize,
                             sUserData);
        }
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_BIT(ulnFnContext *aFnContext,
                                       ulnDescRec   *aDescRecApd,
                                       ulnDescRec   *aDescRecIpd,
                                       void         *aUserDataPtr,
                                       acp_sint32_t  aUserOctetLength,
                                       acp_uint32_t  aRowNo0Based,
                                       acp_uint8_t  *aConversionBuffer,
                                       ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    // 8200 = ushort_max/8 + 7 = 65535/8 + 7 = 8191 + 7
    acp_uint8_t          sTargetBuffer[8200];
    mtdBitType*          sTarget = (mtdBitType*)sTargetBuffer;
    acp_uint32_t         sStructSize;
    acp_uint32_t         sByteLen  = 0;
    ulnBindInfo*         sBindInfo = &(aDescRecApd->mBindInfo);

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        //const mtdBitType mtdBitNull = { 0, {'\0',} };
        ULN_CHUNK_WR4(sStmt, &(mtcdBitNull.length));
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        sStructSize = MTD_BIT_TYPE_STRUCT_SIZE(sDataSize);
        ACI_TEST_RAISE( sStructSize > sizeof(sTargetBuffer),
                        LABEL_INVALID_BUFF_LEN);

        ACI_TEST_RAISE( mtcMakeBit(sTarget, sUserData, sDataSize)
                        != ACI_SUCCESS, LABEL_INVALID_LITERAL );
        ACI_TEST_RAISE( sTarget->length > (acp_uint32_t)sBindInfo->mPrecision,
                        LABEL_INVALID_LENGTH );

        /* typedef struct mtdBitType {
           acp_uint32_t  length;
           acp_uint8_t   value[1];
           } mtdBitType;
           */
        sByteLen = BIT_TO_BYTE(sTarget->length);
        ULN_CHUNK_WR4(sStmt, &(sTarget->length));
        ULN_CHUNK_WCP(sStmt, sTarget->value, sByteLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_NUMBER_STRING,
                         sDataSize,
                         sUserData);
    }
    ACI_EXCEPTION(LABEL_INVALID_LENGTH)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1, 
                         aDescRecApd->mIndex,
                         ulERR_ABORT_Data_Value_Overflow_Error,
                         "Bit");
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFF_LEN)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_BUFFER_LEN, sDataSize);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}



ACI_RC ulnParamDataInBuildAny_CHAR_SMALLINT(ulnFnContext *aFnContext,
                                            ulnDescRec   *aDescRecApd,
                                            ulnDescRec   *aDescRecIpd,
                                            void         *aUserDataPtr,
                                            acp_sint32_t  aUserOctetLength,
                                            acp_uint32_t  aRowNo0Based,
                                            acp_uint8_t  *aConversionBuffer,
                                            ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    acp_uint8_t         *sEndPtr = NULL;
    acp_double_t         sTemp;
    acp_sint32_t         sSign;
    acp_sint16_t         sSmallIntValue = MTD_SMALLINT_NULL;
    acp_sint64_t         sBigIntValue  = 0;
    acp_uint64_t         sTempValue    = 0;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( acpCStrToInt64( (const acp_char_t*)sUserData,
                                        sDataSize,
                                        &sSign,
                                        &sTempValue,
                                        10,    /* base */
                                        NULL )
                        != ACP_RC_SUCCESS, LABEL_INVALID_LITERAL );

        if( sEndPtr != (sUserData + sDataSize) )
        {
            ACI_TEST_RAISE( acpCStrToDouble( (const acp_char_t*)sUserData,
                                             sDataSize,
                                             &sTemp,
                                             (acp_char_t**)&sEndPtr)
                            != ACP_RC_SUCCESS, LABEL_INVALID_LITERAL );

            ACI_TEST_RAISE( sEndPtr != (sUserData + sDataSize), LABEL_INVALID_LITERAL );

            sBigIntValue = sTemp;
        }
        else
        {
            sBigIntValue = sTempValue * sSign;
        }

        ACI_TEST_RAISE( sBigIntValue > MTD_SMALLINT_MAXIMUM ||
                        sBigIntValue < MTD_SMALLINT_MINIMUM,
                        LABEL_OUT_OF_RANGE );

        sSmallIntValue = sBigIntValue;

        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_NUMBER_STRING,
                         sDataSize,
                         sUserData);
    }
    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_INTEGER(ulnFnContext *aFnContext,
                                           ulnDescRec   *aDescRecApd,
                                           ulnDescRec   *aDescRecIpd,
                                           void         *aUserDataPtr,
                                           acp_sint32_t  aUserOctetLength,
                                           acp_uint32_t  aRowNo0Based,
                                           acp_uint8_t  *aConversionBuffer,
                                           ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    acp_uint8_t         *sEndPtr = NULL;
    acp_double_t         sTemp;
    acp_sint32_t         sSign;
    acp_sint32_t         sIntegerValue = MTD_INTEGER_NULL;
    acp_sint64_t         sBigIntValue  = 0;
    acp_uint64_t         sTempValue    = 0;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( acpCStrToInt64( (const acp_char_t*)sUserData,
                                        sDataSize,
                                        &sSign,
                                        &sTempValue,
                                        10,    /* base */
                                        (acp_char_t**)&sEndPtr )
                        != ACP_RC_SUCCESS, LABEL_INVALID_LITERAL );

        if( sEndPtr != (sUserData + sDataSize) )
        {
            ACI_TEST_RAISE( acpCStrToDouble( (const acp_char_t*)sUserData,
                                             sDataSize,
                                             &sTemp,
                                             (acp_char_t**)&sEndPtr)
                            != ACP_RC_SUCCESS, LABEL_INVALID_LITERAL );

            ACI_TEST_RAISE( sEndPtr != (sUserData + sDataSize), LABEL_INVALID_LITERAL );

            sBigIntValue = sTemp;
        }
        else
        {
            sBigIntValue = sTempValue * sSign;
        }

        ACI_TEST_RAISE( sBigIntValue > MTD_INTEGER_MAXIMUM ||
                        sBigIntValue < MTD_INTEGER_MINIMUM,
                        LABEL_OUT_OF_RANGE );

        sIntegerValue = sBigIntValue;

        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_NUMBER_STRING,
                         sDataSize,
                         sUserData);
    }
    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_BIGINT(ulnFnContext *aFnContext,
                                          ulnDescRec   *aDescRecApd,
                                          ulnDescRec   *aDescRecIpd,
                                          void         *aUserDataPtr,
                                          acp_sint32_t  aUserOctetLength,
                                          acp_uint32_t  aRowNo0Based,
                                          acp_uint8_t  *aConversionBuffer,
                                          ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;
    ulncConvResult       sConvResult;

    ulncNumeric  sNumeric;
    acp_bool_t   sIsOverflow  = ACP_FALSE;
    acp_sint64_t sBigIntValue  = MTD_BIGINT_NULL;
    acp_uint64_t sUBigIntValue = MTD_BIGINT_NULL;
    acp_uint8_t  sBuffer[ULNC_NUMERIC_ALLOCSIZE];

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ulncNumericInitialize(&sNumeric, 10, ULNC_ENDIAN_BIG, sBuffer, ULNC_NUMERIC_ALLOCSIZE);

        sConvResult = ulncCharToNumeric(
                           &sNumeric,
                           ULNC_NUMERIC_ALLOCSIZE,
                           sUserData,
                           sDataSize);
        ACI_TEST_RAISE( sConvResult == ULNC_VALUE_OVERFLOW, LABEL_OUT_OF_RANGE );
        ACI_TEST_RAISE( sConvResult != ULNC_SUCCESS, LABEL_INVALID_LITERAL );

        sUBigIntValue = ulncDecimalToSLong(&sNumeric, &sIsOverflow);

        ACI_TEST_RAISE(sIsOverflow == ACP_TRUE, LABEL_OUT_OF_RANGE);

        if (sNumeric.mSign == 0)
        {
            sBigIntValue = (acp_sint64_t)sUBigIntValue * (-1);
        }
        else
        {
            sBigIntValue = (acp_sint64_t)sUBigIntValue;
        }

        ACI_TEST_RAISE( sBigIntValue > MTD_BIGINT_MAXIMUM ||
                        sBigIntValue < MTD_BIGINT_MINIMUM,
                        LABEL_OUT_OF_RANGE );

        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_NUMBER_STRING,
                         sDataSize,
                         sUserData);
    }
    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_REAL(ulnFnContext *aFnContext,
                                        ulnDescRec   *aDescRecApd,
                                        ulnDescRec   *aDescRecIpd,
                                        void         *aUserDataPtr,
                                        acp_sint32_t  aUserOctetLength,
                                        acp_uint32_t  aRowNo0Based,
                                        acp_uint8_t  *aConversionBuffer,
                                        ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;
    mtdRealType sTarget;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        //static const acp_uint32_t mtdRealNull =
        //( MTD_REAL_EXPONENT_MASK| MTD_REAL_FRACTION_MASK );
        ULN_CHUNK_WR4(sStmt, &mtcdRealNull);
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( mtcMakeReal( &sTarget, sUserData, sDataSize)
                        != ACI_SUCCESS, LABEL_MTCONV_ERROR );
        ULN_CHUNK_WR4(sStmt, &sTarget);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_MTCONV_ERROR)
    {
        if (aciGetErrorCode() == mtERR_ABORT_VALUE_OVERFLOW)
        {    
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1, 
                             aDescRecApd->mIndex,
                             ulERR_ABORT_Data_Value_Overflow_Error,
                             "Real");
        }
        else
        {
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1,
                             aDescRecApd->mIndex,
                             ulERR_ABORT_INVALID_NUMBER_STRING,
                             sDataSize,
                             sUserData);
        }
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// similar to CHAR_NUMERIC except precision and canonize
ACI_RC ulnParamDataInBuildAny_CHAR_FLOAT(ulnFnContext *aFnContext,
                                           ulnDescRec   *aDescRecApd,
                                           ulnDescRec   *aDescRecIpd,
                                           void         *aUserDataPtr,
                                           acp_sint32_t  aUserOctetLength,
                                           acp_uint32_t  aRowNo0Based,
                                           acp_uint8_t  *aConversionBuffer,
                                           ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    acp_uint8_t          sNumericBuf[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType      *sNumeric = (mtdNumericType*)sNumericBuf;
    ulnBindInfo*         sBindInfo = &(aDescRecApd->mBindInfo);

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    /* typedef struct mtdNumericType {
       acp_uint8_t length;
       acp_uint8_t signExponent;
       acp_uint8_t mantissa[1];
       } mtdNumericType;
       */
    if( sIsNull == ACP_TRUE )
    {
        //const mtdNumericType mtdNumericNull = { 0, 0, {'\0',} };
        ULN_CHUNK_WR1(sStmt, mtcdNumericNull.length);
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( mtcMakeNumeric( sNumeric,
                                        MTD_FLOAT_MANTISSA( sBindInfo->mPrecision ),
                                        sUserData,
                                        sDataSize )
                        != ACI_SUCCESS, LABEL_MTCONV_ERROR );

        ULN_CHUNK_WR1(sStmt, sNumeric->length);
        if (sNumeric->length > 0)
        {
            ULN_CHUNK_WR1(sStmt, sNumeric->signExponent);
            ULN_CHUNK_WCP(sStmt, sNumeric->mantissa, sNumeric->length - 1);
        }
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_MTCONV_ERROR)
    {
        if (aciGetErrorCode() == mtERR_ABORT_VALUE_OVERFLOW)
        {    
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1, 
                             aDescRecApd->mIndex,
                             ulERR_ABORT_Data_Value_Overflow_Error,
                             "Float");
        }
        else
        {
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1,
                             aDescRecApd->mIndex,
                             ulERR_ABORT_INVALID_NUMBER_STRING,
                             sDataSize,
                             sUserData);
        }
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_DOUBLE(ulnFnContext *aFnContext,
                                          ulnDescRec   *aDescRecApd,
                                          ulnDescRec   *aDescRecIpd,
                                          void         *aUserDataPtr,
                                          acp_sint32_t  aUserOctetLength,
                                          acp_uint32_t  aRowNo0Based,
                                          acp_uint8_t  *aConversionBuffer,
                                          ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull   = ACP_FALSE;
    mtdDoubleType        sTarget;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        // acp_uint64_t mtdDoubleNull =
        // ( MTD_DOUBLE_EXPONENT_MASK | MTD_DOUBLE_FRACTION_MASK );
        ULN_CHUNK_WR8(sStmt, &mtcdDoubleNull);
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( mtcMakeDouble( &sTarget, sUserData, sDataSize)
                        != ACI_SUCCESS, LABEL_MTCONV_ERROR );
        ULN_CHUNK_WR8(sStmt, &sTarget);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_MTCONV_ERROR)
    {
        if (aciGetErrorCode() == mtERR_ABORT_VALUE_OVERFLOW)
        {    
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1, 
                             aDescRecApd->mIndex,
                             ulERR_ABORT_Data_Value_Overflow_Error,
                             "Double");
        }
        else
        {
            ulnErrorExtended(aFnContext,
                             aRowNo0Based + 1,
                             aDescRecApd->mIndex,
                             ulERR_ABORT_INVALID_NUMBER_STRING,
                             sDataSize,
                             sUserData);
        }
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_BINARY(ulnFnContext *aFnContext,
                                          ulnDescRec   *aDescRecApd,
                                          ulnDescRec   *aDescRecIpd,
                                          void         *aUserDataPtr,
                                          acp_sint32_t  aUserOctetLength,
                                          acp_uint32_t  aRowNo0Based,
                                          acp_uint8_t  *aConversionBuffer,
                                          ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull   = ACP_FALSE;
    mtdBinaryType*       sTarget   = NULL;
    acp_uint32_t         sPadding  = 0;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        // static mtdBinaryType mtdBinaryNull = { 0, {'\0'}, {'\0',} };
        ULN_CHUNK_WR4(sStmt, &(mtcdBinaryNull.mLength));
        ULN_CHUNK_WR4(sStmt, &sPadding); // meaningless
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE(sDataSize > MTD_BINARY_PRECISION_MAXIMUM,
                       LABEL_INVALID_BUFF_LEN); // 2G
        ACI_TEST(acpMemAlloc((void**) &sTarget, 8+sDataSize) != ACI_SUCCESS);

        ACI_TEST_RAISE(mtcMakeBinary(sTarget, sUserData, sDataSize )
                 != ACI_SUCCESS, LABEL_INVALID_LITERAL);

        /* typedef struct mtdBinaryType
           {
           acp_uint32_t mLength;
           acp_uint8_t  mPadding[4];
           acp_uint8_t  mValue[1];
           } mtdBinaryType;
           */
        ULN_CHUNK_WR4(sStmt, &(sTarget->mLength));
        ULN_CHUNK_WR4(sStmt, &sPadding); // meaningless
        ULN_CHUNK_WCP(sStmt, sTarget->mValue, sTarget->mLength);
    }

    if (sTarget != NULL)
    {
        acpMemFree(sTarget);
        sTarget = NULL;
    }
    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFF_LEN)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_BUFFER_LEN, sDataSize);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_NUMBER_STRING,
                         sDataSize,
                         sUserData);
    }
    ACI_EXCEPTION_END;

    if (sTarget != NULL)
    {
        acpMemFree(sTarget);
        sTarget = NULL;
    }
    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_NIBBLE(ulnFnContext *aFnContext,
                                          ulnDescRec   *aDescRecApd,
                                          ulnDescRec   *aDescRecIpd,
                                          void         *aUserDataPtr,
                                          acp_sint32_t  aUserOctetLength,
                                          acp_uint32_t  aRowNo0Based,
                                          acp_uint8_t  *aConversionBuffer,
                                          ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    acp_uint8_t          sTargetBuffer[256];
    mtdNibbleType*       sTarget = (mtdNibbleType*)sTargetBuffer;
    acp_uint32_t         sByteLen  = 0;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        // #define MTD_NIBBLE_NULL_LENGTH       (255)
        // const mtdNibbleType mtdNibbleNull =
        // { MTD_NIBBLE_NULL_LENGTH, {'\0',} };
        ULN_CHUNK_WR1(sStmt, mtcdNibbleNull.length);
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE(sDataSize > MTD_NIBBLE_PRECISION_MAXIMUM, 
                       LABEL_INVALID_BUFF_LEN); // 254
        ACI_TEST(mtcMakeNibble(sTarget, sUserData, sDataSize) != ACI_SUCCESS);

        /* typedef struct mtdNibbleType {
           acp_uint8_t length;
           acp_uint8_t value[1];
           } mtdNibbleType;
           */
        sByteLen  = (sTarget->length + 1)/2;
        ULN_CHUNK_WR1(sStmt, sTarget->length);
        ULN_CHUNK_WCP(sStmt, sTarget->value, sByteLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFF_LEN)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_BUFFER_LEN, sDataSize);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_BYTE(ulnFnContext *aFnContext,
                                        ulnDescRec   *aDescRecApd,
                                        ulnDescRec   *aDescRecIpd,
                                        void         *aUserDataPtr,
                                        acp_sint32_t  aUserOctetLength,
                                        acp_uint32_t  aRowNo0Based,
                                        acp_uint8_t  *aConversionBuffer,
                                        ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    acp_uint8_t          sTargetBuffer[65535];
    mtdByteType*         sTarget = (mtdByteType*)sTargetBuffer;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        // const mtdByteType mtdByteNull = { 0, {'\0',} };
        ULN_CHUNK_WR2(sStmt, &(mtcdByteNull.length));
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE(sDataSize > MTD_BYTE_PRECISION_MAXIMUM, 
                       LABEL_INVALID_BUFF_LEN); // 65536 - 2
        ACI_TEST( mtcMakeByte( (mtdByteType *)sTarget,
                               sUserData,
                               sDataSize )
                  != ACI_SUCCESS );

        /* typedef struct mtdByteType {
           acp_uint16_t length;
           acp_uint8_t  value[1];
           } mtdByteType;
           */
        ULN_CHUNK_WR2(sStmt, &(sTarget->length));
        ULN_CHUNK_WCP(sStmt, sTarget->value, sTarget->length);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFF_LEN)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_BUFFER_LEN, sDataSize);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_TIMESTAMP(ulnFnContext *aFnContext,
                                             ulnDescRec   *aDescRecApd,
                                             ulnDescRec   *aDescRecIpd,
                                             void         *aUserDataPtr,
                                             acp_sint32_t  aUserOctetLength,
                                             acp_uint32_t  aRowNo0Based,
                                             acp_uint8_t  *aConversionBuffer,
                                             ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;
    mtdDateType          sMtdDate = mtcdDateNull;
    acp_uint8_t          sErrMsgBuf[32] = {'\0', };

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        // nothing todo
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( mtdDateInterfaceToDate(&sMtdDate,
                                               sUserData,
                                               sDataSize,
                                               (acp_uint8_t*)sDbc->mDateFormat,
                                               acpCStrLen(sDbc->mDateFormat, ACP_SINT32_MAX))
                        != ACI_SUCCESS, INVALID_DATE_STR_ERROR );
    }

    ULN_CHUNK_WR2(sStmt, &(sMtdDate.year));
    ULN_CHUNK_WR2(sStmt, &(sMtdDate.mon_day_hour));
    ULN_CHUNK_WR4(sStmt, &(sMtdDate.min_sec_mic));

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION( INVALID_DATE_STR_ERROR )
    {
        acpMemCpy( sErrMsgBuf,
                   sUserData,
                   ACP_MIN(31, sDataSize) );
        ulnErrorExtended( aFnContext,
                          aRowNo0Based + 1,
                          aDescRecApd->mIndex,
                          ulERR_ABORT_INVALID_DATE_STRING,
                          sErrMsgBuf,
                          sDbc->mDateFormat );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_INTERVAL(ulnFnContext *aFnContext,
                                            ulnDescRec   *aDescRecApd,
                                            ulnDescRec   *aDescRecIpd,
                                            void         *aUserDataPtr,
                                            acp_sint32_t  aUserOctetLength,
                                            acp_uint32_t  aRowNo0Based,
                                            acp_uint8_t  *aConversionBuffer,
                                            ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_uint8_t         *sUserData = (acp_uint8_t*)aUserDataPtr;
    acp_sint32_t         sDataSize = 0;
    acp_bool_t           sIsNull = ACP_FALSE;

    mtdIntervalType      sTarget;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    sUserData,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    if( sIsNull == ACP_TRUE )
    {
        // const mtdIntervalType mtdIntervalNull = {
        //   -ACP_SINT64_LITERAL(9223372036854775807)-1,
        //   -ACP_SINT64_LITERAL(9223372036854775807)-1 };
        ULN_CHUNK_WR8(sStmt, &(mtcdIntervalNull.second));
        ULN_CHUNK_WR8(sStmt, &(mtcdIntervalNull.microsecond));
    }
    else
    {
        if (sCType == ULN_CTYPE_WCHAR)
        {
            ACI_TEST(ulnCharSetConvert(aCharSet,
                                       aFnContext,
                                       NULL,
                                       sDbc->mWcharCharsetLangModule,
                                       sDbc->mCharsetLangModule,
                                       sUserData,
                                       sDataSize,
                                       CONV_DATA_IN)
                     != ACI_SUCCESS);

            sUserData = ulnCharSetGetConvertedText(aCharSet);
            sDataSize = aCharSet->mDestLen;
        }

        ACI_TEST_RAISE( mtcMakeInterval(&sTarget, sUserData, sDataSize)
                        != ACI_SUCCESS, LABEL_INVALID_LITERAL );

        /* typedef struct mtdIntervalType {
           acp_sint64_t second;
           acp_sint64_t microsecond;
           } mtdIntervalType;
           */
        ULN_CHUNK_WR8(sStmt, &(sTarget.second));
        ULN_CHUNK_WR8(sStmt, &(sTarget.microsecond));
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_NUMBER_STRING,
                         sDataSize,
                         sUserData);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_CHAR_NCHAR(ulnFnContext *aFnContext,
                                             ulnDescRec   *aDescRecApd,
                                             ulnDescRec   *aDescRecIpd,
                                             void         *aUserDataPtr,
                                             acp_sint32_t  aUserOctetLength,
                                             acp_uint32_t  aRowNo0Based,
                                             acp_uint8_t  *aConversionBuffer,
                                             ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;
    acp_sint32_t         sTransferSize;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_uint16_t         sCopyLen = 0;
    const mtlModule*     sCliCharset;

    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCType = aDescRecApd->mMeta.mCTYPE;
    if (sCType == ULN_CTYPE_DEFAULT)
    {
        sCType = ulnGetDefaultUlCType(&(aDescRecIpd->mMeta));
    }
    ACI_TEST(ulnBindCalcUserDataLen(sCType,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sTransferSize,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
    }
    else
    {
        if (sCType == ULN_CTYPE_CHAR)
        {
            sCliCharset = sDbc->mClientCharsetLangModule;
        }
        else // ctype_wchar
        {
            sCliCharset = sDbc->mWcharCharsetLangModule;
        }
        // 데이터가 손실되지 않는 경우에 한해서 PRECISION만큼만 보낸다.
        ACI_TEST(ulnCharSetConvert(aCharSet,
                                   aFnContext,
                                   NULL,
                                   sCliCharset, //BUG-22684
                                   (const mtlModule*)sDbc->mNcharCharsetLangModule,
                                   (void*)aUserDataPtr,
                                   sTransferSize,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);

        // BUG-23645 캐릭터셋 변환시 클라이언트에서 데이타를 잘라서 보냅니다.
        // 서버로 보낼때는 컨버젼된 데이타를 전부 보낸다.
        sCopyLen = aCharSet->mDestLen;
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
        ULN_CHUNK_WCP(sStmt, ulnCharSetGetConvertedText(aCharSet), sCopyLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// this referred to convertCmNumeric()
ACI_RC ulnParamDataInBuildAny_NUMERIC_NUMERIC(ulnFnContext *aFnContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnDescRec   *aDescRecIpd,
                                              void         *aUserDataPtr,
                                              acp_sint32_t  aUserOctetLength,
                                              acp_uint32_t  aRowNo0Based,
                                              acp_uint8_t  *aConversionBuffer,
                                              ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    acp_sint32_t         sDataSize;
    acp_bool_t           sIsNull = ACP_FALSE;

    SQL_NUMERIC_STRUCT  *sUserVal = (SQL_NUMERIC_STRUCT *)aUserDataPtr;
    acp_uint32_t         sUserValLen;

    acp_uint8_t          sConvBuffer[ULN_NUMERIC_BUFFER_SIZE]; // 20
    acp_uint8_t         *sConvMantissa;
    ulncNumeric          sSrc;
    ulncNumeric          sDst;
    acp_uint8_t          sNumericBuf[MTD_NUMERIC_SIZE_MAXIMUM]; // 23
    mtdNumericType      *sNumeric = (mtdNumericType *) sNumericBuf;
    acp_sint8_t          sSign;
    acp_sint32_t         sScale     = 0;
    acp_sint32_t         sPrecision = 0;
    acp_sint32_t         sSize;
    acp_sint32_t         i;
    acp_uint32_t         sStructSize;

    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_NUMERIC,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sDataSize,
                                    &sIsNull) != ACI_SUCCESS);

    ACI_TEST_RAISE(sDataSize != ACI_SIZEOF(SQL_NUMERIC_STRUCT),
                   LABEL_INVALID_DATA_SIZE);

    /* typedef struct mtdNumericType {
       acp_uint8_t length;
       acp_uint8_t signExponent;
       acp_uint8_t mantissa[1];
       } mtdNumericType;
       */
    if( sIsNull == ACP_TRUE )
    {
        //const mtdNumericType mtdNumericNull = { 0, 0, {'\0',} };
        ULN_CHUNK_WR1(sStmt, mtcdNumericNull.length);
    }
    else
    {
        sUserValLen = SQL_MAX_NUMERIC_LEN;
        for (i = SQL_MAX_NUMERIC_LEN - 1; i >= 0; i--)
        {
            if (sUserVal->val[i] != 0)
            {
                break;
            }
            sUserValLen--;
        }

        if (sUserValLen > 0)
        {
            if( aDescRecApd->mMeta.mCTYPE == ULN_CTYPE_BINARY )
            {
                sScale = sUserVal->scale;
            }
            else
            {
                sScale = aDescRecApd->mMeta.mOdbcScale;
            }

            sSign  = (acp_sint8_t)(sUserVal->sign);

            // buffer를 0으로 초기화 하지 않는다
            ulncNumericInitFromData(&sSrc,
                                    256,
                                    // see changeSessionStateService()
                                    ULNC_ENDIAN_LITTLE,
                                    sUserVal->val,
                                    SQL_MAX_NUMERIC_LEN, // 16
                                    sUserValLen );

            // buffer를 0으로 초기화 한다
            ulncNumericInitialize(&sDst,
                                  100,
                                  ULNC_ENDIAN_BIG,
                                  sConvBuffer,
                                  MTD_NUMERIC_MANTISSA_MAXIMUM); // 20

            ACI_TEST(ulncNumericToNumeric(&sDst, &sSrc) != ACI_SUCCESS);
            if ((sScale % 2) == 1)
            {
                ACI_TEST(ulncShiftLeft(&sDst) != ACI_SUCCESS);

                sScale++;
            }
            else if ((sScale % 2) == -1)
            {
                ACI_TEST(ulncShiftLeft(&sDst) != ACI_SUCCESS);
            }

            if (sDst.mEndian == ULNC_ENDIAN_BIG)
            {
                sConvMantissa = sDst.mBuffer + sDst.mAllocSize - sDst.mSize;
            }
            else
            {
                sConvMantissa = sDst.mBuffer;
            }

            sSize = sDst.mSize;
            for (i = sSize - 1; (i >= 0) && (sConvMantissa[i] == 0); i--)
            {
                sSize  -= 1;
                sScale -= 2;
            }

            sPrecision = sSize * 2;

            if (sConvMantissa[0] < 10)
            {
                sPrecision--;
            }

            if ((sConvMantissa[sSize - 1] % 10) == 0)
            {
                sPrecision--;
            }

            if (sSign != 1)
            {
                for (i = 0; i < sSize; i++)
                {
                    sConvMantissa[i] = 99 - sConvMantissa[i];
                }
            }

            ACI_TEST_RAISE((sPrecision < MTD_NUMERIC_PRECISION_MINIMUM) ||
                           (sPrecision > MTD_NUMERIC_PRECISION_MAXIMUM),
                           LABEL_INVALID_LITERAL);
            ACI_TEST_RAISE((sScale < MTD_NUMERIC_SCALE_MINIMUM) ||
                           (sScale > MTD_NUMERIC_SCALE_MAXIMUM),
                           LABEL_INVALID_LITERAL);

            // BUG-32571: TargetType이 FLOAT인 경우 scale이 0
            if(aDescRecIpd->mMeta.mMTYPE != ULN_MTYPE_FLOAT)
            {
                // ex) -88888.44444 (10, 5): scale 6가 되어 error 
                ACI_TEST_RAISE(sScale > aDescRecIpd->mMeta.mOdbcScale, InvalidScaleSize );
            }

            sStructSize = MTD_NUMERIC_SIZE(sPrecision, 0);
            ACI_TEST_RAISE(sStructSize > ACI_SIZEOF(sNumericBuf),
                            LABEL_INVALID_LITERAL);

            sNumeric->length        = sSize + 1;
            sNumeric->signExponent  = (sSign == 1) ? 0x80 : 0;
            sNumeric->signExponent |= (sSize - sScale / 2) * ((sSign == 1) ? 1 : -1) + 64;

            // max len: 20
            acpMemCpy(sNumeric->mantissa, sConvMantissa, sSize);
        }
        else
        {
            sNumeric->length       = 1;
            sNumeric->signExponent = 128;
            sNumeric->mantissa[0] = '\0';
        }

        ULN_CHUNK_WR1(sStmt, sNumeric->length);
        if (sNumeric->length > 0)
        {
            ULN_CHUNK_WR1(sStmt, sNumeric->signExponent);
            ULN_CHUNK_WCP(sStmt, sNumeric->mantissa, sNumeric->length - 1);
        }
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BINARY_SIZE_NOT_MATCH);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }
    ACI_EXCEPTION(InvalidScaleSize);
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_NUMERIC_SCALE_OUT_OF_RANGE);
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_OLD(ulnFnContext *aFnContext,
                                  ulnDescRec   *aDescRecApd,
                                  ulnDescRec   *aDescRecIpd,
                                  void         *aUserDataPtr,
                                  acp_sint32_t  aUserOctetLength,
                                  acp_uint32_t  aRowNo0Based,
                                  acp_uint8_t  *aConversionBuffer,
                                  ulnCharSet   *aCharSet)
{
    ulnCTypeID  sCTYPE;
    ulnMeta    *sIpdMeta;
    ulnMeta    *sApdMeta;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aUserDataPtr);
    ACP_UNUSED(aUserOctetLength);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    sApdMeta = &aDescRecApd->mMeta;
    sIpdMeta = &aDescRecIpd->mMeta;

    sCTYPE = sApdMeta->mCTYPE;

    // proj_2160 cm_type removal
    // 이 함수를 타면 안된다. 일단 assert로 막아둔다
    /* BUG-35242 ALTI-PCM-002 Coding Convention Violation in UL module */
    acpPrintf( "parameter binding of [%d to %d] is not supported\n",
               sCTYPE,
               sIpdMeta->mMTYPE );

    ACE_ASSERT(0);

    return ACI_SUCCESS;
}

/*
 * =======================================
 * SQL_C_BIT --> SQL_TYPES
 * =======================================
 */

ACI_RC ulnParamDataInBuildAny_BIT_CHAR(ulnFnContext *aFnContext,
                                           ulnDescRec   *aDescRecApd,
                                           ulnDescRec   *aDescRecIpd,
                                           void         *aUserDataPtr,
                                           acp_sint32_t  aUserOctetLength,
                                           acp_uint32_t  aRowNo0Based,
                                           acp_uint8_t  *aConversionBuffer,
                                           ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    acp_char_t           sCharValue;
    ulnDbc              *sDbc;
    acp_uint16_t         sCopyLen = 1;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);



    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        //const mtdCharType mtdCharNull = { 0, {'\0',} };
        ULN_CHUNK_WR2(sStmt, &(mtcdCharNull.length));
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        switch (sBitValue)
        {
            case 0:
            case 1:
                sCharValue = sBitValue + '0';
                break;

            default:
                ACI_RAISE(LABEL_BIT_VALUE_OUT_OF_RANGE);
                break;
        }
        //*aConversionBuffer = (acp_uint8_t)sCharValue;
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
        ULN_CHUNK_WR1(sStmt, sCharValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_NCHAR(ulnFnContext *aFnContext,
                                            ulnDescRec   *aDescRecApd,
                                            ulnDescRec   *aDescRecIpd,
                                            void         *aUserDataPtr,
                                            acp_sint32_t  aUserOctetLength,
                                            acp_uint32_t  aRowNo0Based,
                                            acp_uint8_t  *aConversionBuffer,
                                            ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulWChar              sCharValue;
    ulnDbc              *sDbc;
    acp_uint16_t         sCopyLen = 1;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        //const mtdNcharType mtdNcharNull = { 0, {'\0',} };
        ULN_CHUNK_WR2(sStmt, &(mtcdNcharNull.length));
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        switch (sBitValue)
        {
            case 0:
            case 1:
                *(acp_uint8_t*)(&sCharValue) = (acp_uint8_t)0;
                *(((acp_uint8_t*)(&sCharValue)) + 1) = (acp_uint8_t)(sBitValue + 0x30);
                break;

            default:
                ACI_RAISE(LABEL_BIT_VALUE_OUT_OF_RANGE);
                break;
        }
        sCopyLen = ACI_SIZEOF(ulWChar);
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
        ULN_CHUNK_WCP(sStmt, &(sCharValue), sCopyLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_NUMERIC(ulnFnContext *aFnContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnDescRec   *aDescRecIpd,
                                              void         *aUserDataPtr,
                                              acp_sint32_t  aUserOctetLength,
                                              acp_uint32_t  aRowNo0Based,
                                              acp_uint8_t  *aConversionBuffer,
                                              ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulnDbc              *sDbc;
    acp_uint8_t          sNumericBuf[MTD_NUMERIC_SIZE_MAXIMUM]; // 23
    mtdNumericType      *sNumeric = (mtdNumericType *) sNumericBuf;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        //const mtdNumericType mtdNumericNull = { 0, 0, {'\0',} };
        ULN_CHUNK_WR1(sStmt, mtcdNumericNull.length);
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        ACI_TEST_RAISE(sBitValue > 1, LABEL_BIT_VALUE_OUT_OF_RANGE);
        /*
        sCmtNumeric->mPrecision = 1;
        sCmtNumeric->mScale     = 0;
        sCmtNumeric->mSign      = 1;
        sCmtNumeric->mData[0]   = sBitValue;
        */
        sNumeric->length       = 2;
        sNumeric->signExponent = 128;
        // (sSize - sScale / 2) * ((sSign == 1) ? 1 : -1) + 64;
        sNumeric->signExponent |= 1 + 64;
        sNumeric->mantissa[0] = sBitValue;

        ULN_CHUNK_WR1(sStmt, sNumeric->length);
        if (sNumeric->length > 0)
        {
            ULN_CHUNK_WR1(sStmt, sNumeric->signExponent);
            ULN_CHUNK_WCP(sStmt, sNumeric->mantissa, sNumeric->length - 1);
        }
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_BIT(ulnFnContext *aFnContext,
                                      ulnDescRec   *aDescRecApd,
                                      ulnDescRec   *aDescRecIpd,
                                      void         *aUserDataPtr,
                                      acp_sint32_t  aUserOctetLength,
                                      acp_uint32_t  aRowNo0Based,
                                      acp_uint8_t  *aConversionBuffer,
                                      ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulnDbc              *sDbc;
    mtdBitType           sTarget;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /* typedef struct mtdBitType {
       acp_uint32_t  length;
       acp_uint8_t   value[1];
       } mtdBitType;
       */
    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        //const mtdBitType mtdBitNull = { 0, {'\0',} };
        ULN_CHUNK_WR4(sStmt, &(mtcdBitNull.length));
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        ACI_TEST_RAISE(sBitValue > 1, LABEL_BIT_VALUE_OUT_OF_RANGE);

        sBitValue = sBitValue << 7;
        sTarget.length = 1;
        ULN_CHUNK_WR4(sStmt, &(sTarget.length));
        ULN_CHUNK_WR1(sStmt, sBitValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_SMALLINT(ulnFnContext *aFnContext,
                                               ulnDescRec   *aDescRecApd,
                                               ulnDescRec   *aDescRecIpd,
                                               void         *aUserDataPtr,
                                               acp_sint32_t  aUserOctetLength,
                                               acp_uint32_t  aRowNo0Based,
                                               acp_uint8_t  *aConversionBuffer,
                                               ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulnDbc              *sDbc;
    acp_sint16_t         sSmallIntValue = MTD_SMALLINT_NULL;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        ACI_TEST_RAISE(sBitValue > 1, LABEL_BIT_VALUE_OUT_OF_RANGE);
        sSmallIntValue = sBitValue;
        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_INTEGER(ulnFnContext *aFnContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnDescRec   *aDescRecIpd,
                                              void         *aUserDataPtr,
                                              acp_sint32_t  aUserOctetLength,
                                              acp_uint32_t  aRowNo0Based,
                                              acp_uint8_t  *aConversionBuffer,
                                              ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulnDbc              *sDbc;
    acp_sint32_t         sIntegerValue = MTD_INTEGER_NULL;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        ACI_TEST_RAISE(sBitValue > 1, LABEL_BIT_VALUE_OUT_OF_RANGE);
        sIntegerValue = sBitValue;
        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_BIGINT(ulnFnContext *aFnContext,
                                             ulnDescRec   *aDescRecApd,
                                             ulnDescRec   *aDescRecIpd,
                                             void         *aUserDataPtr,
                                             acp_sint32_t  aUserOctetLength,
                                             acp_uint32_t  aRowNo0Based,
                                             acp_uint8_t  *aConversionBuffer,
                                             ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulnDbc              *sDbc;
    acp_sint64_t         sBigIntValue = MTD_BIGINT_NULL;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        ACI_TEST_RAISE(sBitValue > 1, LABEL_BIT_VALUE_OUT_OF_RANGE);
        sBigIntValue = sBitValue;
        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_REAL(ulnFnContext *aFnContext,
                                           ulnDescRec   *aDescRecApd,
                                           ulnDescRec   *aDescRecIpd,
                                           void         *aUserDataPtr,
                                           acp_sint32_t aUserOctetLength,
                                           acp_uint32_t  aRowNo0Based,
                                           acp_uint8_t  *aConversionBuffer,
                                           ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulnDbc              *sDbc;
    acp_float_t          sRealValue = mtcdRealNull; // acp_uint32_t

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR4(sStmt, &sRealValue);
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        ACI_TEST_RAISE(sBitValue > 1, LABEL_BIT_VALUE_OUT_OF_RANGE);
        sRealValue = sBitValue;
        ULN_CHUNK_WR4(sStmt, &sRealValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BIT_DOUBLE(ulnFnContext *aFnContext,
                                             ulnDescRec   *aDescRecApd,
                                             ulnDescRec   *aDescRecIpd,
                                             void         *aUserDataPtr,
                                             acp_sint32_t aUserOctetLength,
                                             acp_uint32_t  aRowNo0Based,
                                             acp_uint8_t  *aConversionBuffer,
                                             ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t          sBitValue = 0;
    ulnDbc              *sDbc;
    acp_double_t         sDoubleValue = mtcdDoubleNull; // acp_uint64_t

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR8(sStmt, &sDoubleValue);
    }
    else
    {
        /* BUG-46052 codesonar null test after dereference */
        sBitValue = *(acp_uint8_t *)aUserDataPtr;
        ACI_TEST_RAISE(sBitValue > 1, LABEL_BIT_VALUE_OUT_OF_RANGE);
        sDoubleValue = sBitValue;
        ULN_CHUNK_WR8(sStmt, &sDoubleValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_BIT_VALUE_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/* proj_2160 cm_type removal
 * c_type: stinyint, utinyint, sshort, ushort,
 *         slong, ulong, sbigint, ubigint
 * target: smallint */
ACI_RC ulnParamDataInBuildAny_TO_SMALLINT(ulnFnContext *aFnContext,
                                          ulnDescRec   *aDescRecApd,
                                          ulnDescRec   *aDescRecIpd,
                                          void         *aUserDataPtr,
                                          acp_sint32_t  aUserOctetLength,
                                          acp_uint32_t  aRowNo0Based,
                                          acp_uint8_t  *aConversionBuffer,
                                          ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;

    acp_sint8_t          sSint8;
    acp_uint8_t          sUint8;
    acp_sint16_t         sSint16;
    acp_uint16_t         sUint16;
    acp_sint32_t         sSint32;
    acp_uint32_t         sUint32;
    acp_sint64_t         sSint64;
    acp_uint64_t         sUint64;
    acp_sint16_t         sSmallIntValue = MTD_SMALLINT_NULL;

    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }
    else
    {
        sCType = aDescRecApd->mMeta.mCTYPE;
        if (sCType == ULN_CTYPE_DEFAULT)
        {
            sCType = ulnGetDefaultUlCType(&(aDescRecIpd->mMeta));
        }
        switch(sCType)
        {
            case ULN_CTYPE_STINYINT:
                sSint8 = *(acp_sint8_t*)aUserDataPtr;
                sSmallIntValue = sSint8;
                break;

            case ULN_CTYPE_UTINYINT:
                sUint8 = *(acp_uint8_t*)aUserDataPtr;
                sSmallIntValue = sUint8;
                break;
 
            case ULN_CTYPE_SSHORT:
                sSint16 = *(acp_sint16_t*)aUserDataPtr;
                sSmallIntValue = sSint16;
                break;

            case ULN_CTYPE_USHORT:
                sUint16 = *(acp_uint16_t*)aUserDataPtr;
                // ex) 0x8000(32768) > 0x7fff(32767, max)
                ACI_TEST_RAISE(sUint16 > MTD_SMALLINT_MAXIMUM, DataOverflow);
                sSmallIntValue = sUint16;
                break;

            case ULN_CTYPE_SLONG:
                sSint32 = *(acp_sint32_t*)aUserDataPtr;
                ACI_TEST_RAISE(sSint32 > MTD_SMALLINT_MAXIMUM, DataOverflow);
                sSmallIntValue = sSint32;
                break;

            case ULN_CTYPE_ULONG:
                sUint32 = *(acp_uint32_t*)aUserDataPtr;
                ACI_TEST_RAISE(sUint32 > MTD_SMALLINT_MAXIMUM, DataOverflow);
                sSmallIntValue = sUint32;
                break;

            case ULN_CTYPE_SBIGINT:
                sSint64 = *(acp_sint64_t*)aUserDataPtr;
                ACI_TEST_RAISE(sSint64 > MTD_SMALLINT_MAXIMUM, DataOverflow);
                sSmallIntValue = sSint64;
                break;

            case ULN_CTYPE_UBIGINT:
                sUint64 = *(acp_uint64_t*)aUserDataPtr;
                ACI_TEST_RAISE(sUint64 > MTD_SMALLINT_MAXIMUM, DataOverflow);
                sSmallIntValue = sUint64;
                break;

            default:
                ACE_ASSERT(0);
                break;
        }
        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(DataOverflow)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_Data_Value_Overflow_Error,
                         "Smallint");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_TO_INTEGER(ulnFnContext *aFnContext,
                                         ulnDescRec   *aDescRecApd,
                                         ulnDescRec   *aDescRecIpd,
                                         void         *aUserDataPtr,
                                         acp_sint32_t  aUserOctetLength,
                                         acp_uint32_t  aRowNo0Based,
                                         acp_uint8_t  *aConversionBuffer,
                                         ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;

    acp_uint16_t         sUint16;
    acp_sint32_t         sSint32;
    acp_uint32_t         sUint32;
    acp_sint64_t         sSint64;
    acp_uint64_t         sUint64;
    acp_sint32_t         sIntegerValue = MTD_INTEGER_NULL;

    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }
    else
    {
        sCType = aDescRecApd->mMeta.mCTYPE;
        if (sCType == ULN_CTYPE_DEFAULT)
        {
            sCType = ulnGetDefaultUlCType(&(aDescRecIpd->mMeta));
        }
        switch(sCType)
        {
            case ULN_CTYPE_USHORT:
                sUint16 = *(acp_uint16_t*)aUserDataPtr;
                sIntegerValue = sUint16;
                break;

            case ULN_CTYPE_SLONG:
                sSint32 = *(acp_sint32_t*)aUserDataPtr;
                ACI_TEST_RAISE(sSint32 > MTD_INTEGER_MAXIMUM, DataOverflow);
                sIntegerValue = sSint32;
                break;

            case ULN_CTYPE_ULONG:
                sUint32 = *(acp_uint32_t*)aUserDataPtr;
                ACI_TEST_RAISE(sUint32 > MTD_INTEGER_MAXIMUM, DataOverflow);
                sIntegerValue = sUint32;
                break;

            case ULN_CTYPE_SBIGINT:
                sSint64 = *(acp_sint64_t*)aUserDataPtr;
                ACI_TEST_RAISE(sSint64 > MTD_INTEGER_MAXIMUM, DataOverflow);
                sIntegerValue = sSint64;
                break;

            case ULN_CTYPE_UBIGINT:
                sUint64 = *(acp_uint64_t*)aUserDataPtr;
                ACI_TEST_RAISE(sUint64 > MTD_INTEGER_MAXIMUM, DataOverflow);
                sIntegerValue = sUint64;
                break;

            default:
                ACE_ASSERT(0);
                break;
        }
        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(DataOverflow)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_Data_Value_Overflow_Error,
                         "Integer");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_TO_BIGINT(ulnFnContext *aFnContext,
                                        ulnDescRec   *aDescRecApd,
                                        ulnDescRec   *aDescRecIpd,
                                        void         *aUserDataPtr,
                                        acp_sint32_t  aUserOctetLength,
                                        acp_uint32_t  aRowNo0Based,
                                        acp_uint8_t  *aConversionBuffer,
                                        ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    ulnCTypeID           sCType;

    acp_uint32_t         sUint32;
    acp_sint64_t         sSint64;
    acp_uint64_t         sUint64;
    acp_sint64_t         sBigIntValue = MTD_BIGINT_NULL;

    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }
    else
    {
        sCType = aDescRecApd->mMeta.mCTYPE;
        if (sCType == ULN_CTYPE_DEFAULT)
        {
            sCType = ulnGetDefaultUlCType(&(aDescRecIpd->mMeta));
        }
        switch(sCType)
        {
            case ULN_CTYPE_ULONG:
                sUint32 = *(acp_uint32_t*)aUserDataPtr;
                sBigIntValue = sUint32;
                break;

            case ULN_CTYPE_SBIGINT:
                sSint64 = *(acp_sint64_t*)aUserDataPtr;
                ACI_TEST_RAISE(sSint64 > MTD_BIGINT_MAXIMUM, DataOverflow);
                sBigIntValue = sSint64;
                break;

            case ULN_CTYPE_BLOB_LOCATOR:
            case ULN_CTYPE_CLOB_LOCATOR:
#ifdef COMPILE_SHARDCLI
                /* PROJ-2739 Client-side Sharding LOB */
                sUint64 = *(acp_uint64_t*)aUserDataPtr;
                ulsdLobLocator *sShardLocator = (ulsdLobLocator *)sUint64;
                sSint64 = sShardLocator->mLobLocator;
#else
                sSint64 = *(acp_sint64_t*)aUserDataPtr;
#endif
                ACI_TEST_RAISE(sSint64 > MTD_BIGINT_MAXIMUM, DataOverflow);
                sBigIntValue = sSint64;
                break;

            case ULN_CTYPE_UBIGINT:
                sUint64 = *(acp_uint64_t*)aUserDataPtr;
                ACI_TEST_RAISE(sUint64 > MTD_BIGINT_MAXIMUM, DataOverflow);
                sBigIntValue = sUint64;
                break;

            default:
                ACE_ASSERT(0);
                break;
        }
        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(DataOverflow)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_Data_Value_Overflow_Error,
                         "Bigint");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_FLOAT_REAL(ulnFnContext *aFnContext,
                                         ulnDescRec   *aDescRecApd,
                                         ulnDescRec   *aDescRecIpd,
                                         void         *aUserDataPtr,
                                         acp_sint32_t  aUserOctetLength,
                                         acp_uint32_t  aRowNo0Based,
                                         acp_uint8_t  *aConversionBuffer,
                                         ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    acp_float_t          sRealValue = mtcdRealNull; // acp_uint32_t

    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        // mtdRealNull(acp_uint32_t)을 sRealValue로 casting하면 안됨
        // 값이 변한다. cf) 0x7fffffff -> 0x80000000
        ULN_CHUNK_WR4(sStmt, &mtcdRealNull);
    }
    else
    {
        sRealValue = *(acp_float_t*)aUserDataPtr;
        ULN_CHUNK_WR4(sStmt, &sRealValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_DOUBLE_DOUBLE(ulnFnContext *aFnContext,
                                            ulnDescRec   *aDescRecApd,
                                            ulnDescRec   *aDescRecIpd,
                                            void         *aUserDataPtr,
                                            acp_sint32_t  aUserOctetLength,
                                            acp_uint32_t  aRowNo0Based,
                                            acp_uint8_t  *aConversionBuffer,
                                            ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    acp_double_t         sDoubleValue;

    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        // mtdDoubleNull(acp_uint64_t)을 sDoubleValue로 casting하면 안됨
        // 값이 변한다. cf) 0x7fffffffffffffff -> 0x43e0000000000000
        ULN_CHUNK_WR8(sStmt, &mtcdDoubleNull);
    }
    else
    {
        sDoubleValue = *(acp_double_t*)aUserDataPtr;
        ULN_CHUNK_WR8(sStmt, &sDoubleValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

/*
 * =======================================
 * SQL_C_BINARY --> SQL_TYPES
 * =======================================
 */

ACI_RC ulnParamDataInBuildAny_BINARY_CHAR(ulnFnContext *aFnContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnDescRec   *aDescRecIpd,
                                              void         *aUserDataPtr,
                                              acp_sint32_t  aUserOctetLength,
                                              acp_uint32_t  aRowNo0Based,
                                              acp_uint8_t  *aConversionBuffer,
                                              ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sTransferSize;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_uint16_t         sCopyLen = 0;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sTransferSize,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
    }
    else
    {
        sCopyLen = ACP_MIN(sTransferSize, aDescRecApd->mBindInfo.mPrecision);
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
        ULN_CHUNK_WCP(sStmt, aUserDataPtr, sCopyLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_NCHAR(ulnFnContext *aFnContext,
                                               ulnDescRec   *aDescRecApd,
                                               ulnDescRec   *aDescRecIpd,
                                               void         *aUserDataPtr,
                                               acp_sint32_t  aUserOctetLength,
                                               acp_uint32_t  aRowNo0Based,
                                               acp_uint8_t  *aConversionBuffer,
                                               ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sTransferSize;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_uint16_t         sCopyLen = 0;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_CHAR,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sTransferSize,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
    }
    else
    {
        ACI_TEST(ulnCharSetConvert(aCharSet,
                                   aFnContext,
                                   NULL,
                                   sDbc->mClientCharsetLangModule, //BUG-22684
                                   (const mtlModule*)sDbc->mNcharCharsetLangModule,
                                   (void*)aUserDataPtr,
                                   sTransferSize,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);

        // BUG-23645 캐릭터셋 변환시 클라이언트에서 데이타를 잘라서 보냅니다.
        // 서버로 보낼때는 컨버젼된 데이타를 전부 보낸다.
        sCopyLen = aCharSet->mDestLen;
        ULN_CHUNK_WR2(sStmt, &sCopyLen);
        ULN_CHUNK_WCP(sStmt, ulnCharSetGetConvertedText(aCharSet), sCopyLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_BINARY(ulnFnContext *aFnContext,
                                                ulnDescRec   *aDescRecApd,
                                                ulnDescRec   *aDescRecIpd,
                                                void         *aUserDataPtr,
                                                acp_sint32_t  aUserOctetLength,
                                                acp_uint32_t  aRowNo0Based,
                                                acp_uint8_t  *aConversionBuffer,
                                                ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sTransferSize;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_uint32_t         sCopyLen32 = 0;
    acp_uint32_t         sPadding = 0;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sTransferSize,
                                    &sIsNull) != ACI_SUCCESS);

    /* typedef struct mtdBinaryType
       {
       acp_uint32_t mLength;
       acp_uint8_t  mPadding[4];
       acp_uint8_t  mValue[1];
       } mtdBinaryType;
       */
    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR4(sStmt, &sCopyLen32); // length 0 means null
        ULN_CHUNK_WR4(sStmt, &sPadding);   // meaningless
    }
    else
    {
        sCopyLen32 = ACP_MIN(sTransferSize, aDescRecApd->mBindInfo.mPrecision);
        ULN_CHUNK_WR4(sStmt, &sCopyLen32);
        ULN_CHUNK_WR4(sStmt, &sPadding); // meaningless
        ULN_CHUNK_WCP(sStmt, aUserDataPtr, sCopyLen32);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

#include <ulnConvToCHAR.h>

ACI_RC ulnParamDataInBuildAny_BINARY_NUMERIC(ulnFnContext *aFnContext,
                                             ulnDescRec   *aDescRecApd,
                                             ulnDescRec   *aDescRecIpd,
                                             void         *aUserDataPtr,
                                             acp_sint32_t  aUserOctetLength,
                                             acp_uint32_t  aRowNo0Based,
                                             acp_uint8_t  *aConversionBuffer,
                                             ulnCharSet   *aCharSet)
{
    return ulnParamDataInBuildAny_NUMERIC_NUMERIC(aFnContext,
                                                  aDescRecApd,
                                                  aDescRecIpd,
                                                  aUserDataPtr,
                                                  aUserOctetLength,
                                                  aRowNo0Based,
                                                  aConversionBuffer,
                                                  aCharSet);
}

ACI_RC ulnParamDataInBuildAny_BINARY_BIT(ulnFnContext *aFnContext,
                                             ulnDescRec   *aDescRecApd,
                                             ulnDescRec   *aDescRecIpd,
                                             void         *aUserDataPtr,
                                             acp_sint32_t  aUserOctetLength,
                                             acp_uint32_t  aRowNo0Based,
                                             acp_uint8_t  *aConversionBuffer,
                                             ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_sint32_t         sMinRequiredBytes;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_uint32_t         sLen32  = 0;
    acp_uint8_t*         sTarget = (acp_uint8_t*)aUserDataPtr;
    acp_uint32_t         sByteLen  = 0;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);
    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR4(sStmt, &(mtcdBitNull.length));
    }
    else
    {
        sLen32   = *((acp_uint32_t*)aUserDataPtr);
        // precision 값으로부터 실제 byte 길이를 구한다
        sByteLen = BIT_TO_BYTE(sLen32);
        sMinRequiredBytes = ACI_SIZEOF(acp_uint32_t) + sByteLen;
        ACI_TEST_RAISE(sUserDataLength < sMinRequiredBytes, LABEL_INVALID_DATA_SIZE);

        /* typedef struct mtdBitType {
           acp_uint32_t  length;
           acp_uint8_t   value[1];
           } mtdBitType;
           */
        ULN_CHUNK_WR4(sStmt, &(sLen32));
        ULN_CHUNK_WCP(sStmt, (4+sTarget), sByteLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INCONSISTENT_BIT_PRECISION_AND_BUFFER_SIZE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_NIBBLE(ulnFnContext *aFnContext,
                                                ulnDescRec   *aDescRecApd,
                                                ulnDescRec   *aDescRecIpd,
                                                void         *aUserDataPtr,
                                                acp_sint32_t  aUserOctetLength,
                                                acp_uint32_t  aRowNo0Based,
                                                acp_uint8_t  *aConversionBuffer,
                                                ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_sint32_t         sMinRequiredBytes;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_uint8_t          sLen8 = 0;
    acp_uint8_t*         sTarget   = (acp_uint8_t*)aUserDataPtr;
    acp_uint32_t         sByteLen  = 0;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR1(sStmt, mtcdNibbleNull.length);
    }
    else
    {
        sLen8 = *((acp_uint8_t*)aUserDataPtr);
        // BUG-23061 nibble 의 최대길이가 254입니다.
        ACI_TEST_RAISE(sLen8 > MTD_NIBBLE_PRECISION_MAXIMUM,
                       LABEL_INVALID_DATA_SIZE);
        // precision 값으로부터 실제 byte 길이를 구한다
        sByteLen  = (sLen8 + 1)/2;
        sMinRequiredBytes = ACI_SIZEOF(acp_uint8_t) + sByteLen;
        ACI_TEST_RAISE(sUserDataLength < sMinRequiredBytes, LABEL_INVALID_DATA_SIZE);

        /* typedef struct mtdNibbleType {
           acp_uint8_t length;
           acp_uint8_t value[1];
           } mtdNibbleType;
           */
        ULN_CHUNK_WR1(sStmt, sLen8); // set precision
        ULN_CHUNK_WCP(sStmt, (1+sTarget), sByteLen);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INCONSISTENT_NIBBLE_PRECISION_AND_BUFFER_SIZE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_BYTE(ulnFnContext *aFnContext,
                                          ulnDescRec   *aDescRecApd,
                                          ulnDescRec   *aDescRecIpd,
                                          void         *aUserDataPtr,
                                          acp_sint32_t  aUserOctetLength,
                                          acp_uint32_t  aRowNo0Based,
                                          acp_uint8_t  *aConversionBuffer,
                                          ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_sint16_t         sTransferSize16;
    acp_bool_t           sIsNull = ACP_FALSE;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aRowNo0Based);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR2(sStmt, &(mtcdByteNull.length));
    }
    else
    {
        sTransferSize16 = ACP_MIN(sUserDataLength, aDescRecApd->mBindInfo.mPrecision);
        ULN_CHUNK_WR2(sStmt, &sTransferSize16);
        ULN_CHUNK_WCP(sStmt, aUserDataPtr, sTransferSize16);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulnParamDataInBuildAny_BINARY_SMALLINT(ulnFnContext *aFnContext,
                                                  ulnDescRec   *aDescRecApd,
                                                  ulnDescRec   *aDescRecIpd,
                                                  void         *aUserDataPtr,
                                                  acp_sint32_t  aUserOctetLength,
                                                  acp_uint32_t  aRowNo0Based,
                                                  acp_uint8_t  *aConversionBuffer,
                                                  ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_sint16_t         sSmallIntValue = MTD_SMALLINT_NULL;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }
    else
    {
        ACI_TEST_RAISE(sUserDataLength != ACI_SIZEOF(acp_sint16_t), LABEL_INVALID_DATA_SIZE);
        sSmallIntValue = *(acp_sint16_t *)aUserDataPtr;
        ULN_CHUNK_WR2(sStmt, &sSmallIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BINARY_SIZE_NOT_MATCH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulnParamDataInBuildAny_BINARY_INTEGER(ulnFnContext *aFnContext,
                                                 ulnDescRec   *aDescRecApd,
                                                 ulnDescRec   *aDescRecIpd,
                                                 void         *aUserDataPtr,
                                                 acp_sint32_t  aUserOctetLength,
                                                 acp_uint32_t  aRowNo0Based,
                                                 acp_uint8_t  *aConversionBuffer,
                                                 ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_sint32_t         sIntegerValue = MTD_INTEGER_NULL;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }
    else
    {
        ACI_TEST_RAISE(sUserDataLength != ACI_SIZEOF(acp_sint32_t), LABEL_INVALID_DATA_SIZE);
        sIntegerValue = *(acp_sint32_t *)aUserDataPtr;
        ULN_CHUNK_WR4(sStmt, &sIntegerValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BINARY_SIZE_NOT_MATCH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_BIGINT(ulnFnContext *aFnContext,
                                                ulnDescRec   *aDescRecApd,
                                                ulnDescRec   *aDescRecIpd,
                                                void         *aUserDataPtr,
                                                acp_sint32_t  aUserOctetLength,
                                                acp_uint32_t  aRowNo0Based,
                                                acp_uint8_t  *aConversionBuffer,
                                                ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_sint64_t         sBigIntValue = MTD_BIGINT_NULL;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }
    else
    {
        ACI_TEST_RAISE(sUserDataLength != ACI_SIZEOF(acp_sint64_t), LABEL_INVALID_DATA_SIZE);
        sBigIntValue = *(acp_sint64_t *)aUserDataPtr;
        ULN_CHUNK_WR8(sStmt, &sBigIntValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BINARY_SIZE_NOT_MATCH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_DOUBLE(ulnFnContext *aFnContext,
                                                ulnDescRec   *aDescRecApd,
                                                ulnDescRec   *aDescRecIpd,
                                                void         *aUserDataPtr,
                                                acp_sint32_t  aUserOctetLength,
                                                acp_uint32_t  aRowNo0Based,
                                                acp_uint8_t  *aConversionBuffer,
                                                ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_double_t         sDoubleValue = mtcdDoubleNull; // acp_uint64_t

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR8(sStmt, &sDoubleValue);
    }
    else
    {
        ACI_TEST_RAISE(sUserDataLength != ACI_SIZEOF(acp_double_t), LABEL_INVALID_DATA_SIZE);
        sDoubleValue = *(acp_double_t *)aUserDataPtr;
        ULN_CHUNK_WR8(sStmt, &sDoubleValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BINARY_SIZE_NOT_MATCH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_REAL(ulnFnContext *aFnContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnDescRec   *aDescRecIpd,
                                              void         *aUserDataPtr,
                                              acp_sint32_t  aUserOctetLength,
                                              acp_uint32_t  aRowNo0Based,
                                              acp_uint8_t  *aConversionBuffer,
                                              ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc*              sDbc;
    acp_sint32_t         sUserDataLength;
    acp_bool_t           sIsNull = ACP_FALSE;
    acp_float_t          sRealValue = mtcdRealNull; // acp_uint32_t

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    if (sIsNull == ACP_TRUE)
    {
        ULN_CHUNK_WR4(sStmt, &sRealValue);
    }
    else
    {
        ACI_TEST_RAISE(sUserDataLength != ACI_SIZEOF(acp_float_t), LABEL_INVALID_DATA_SIZE);
        sRealValue = *(acp_float_t *)aUserDataPtr;
        ULN_CHUNK_WR4(sStmt, &sRealValue);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BINARY_SIZE_NOT_MATCH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulnParamDataInBuildAny_BINARY_INTERVAL(ulnFnContext *aFnContext,
                                                  ulnDescRec   *aDescRecApd,
                                                  ulnDescRec   *aDescRecIpd,
                                                  void         *aUserDataPtr,
                                                  acp_sint32_t  aUserOctetLength,
                                                  acp_uint32_t  aRowNo0Based,
                                                  acp_uint8_t  *aConversionBuffer,
                                                  ulnCharSet   *aCharSet)
{
    /*
     * NOTE : Altibase 에는 interval 타입으로 컬럼을 만들 수 없다.
     *        따라서, interval 타입은 fetch 혹은 outbinding 만 가능하며,
     *        insert 는 불가능하다.
     */
    // proj_2160 cm_type removal
    // this callback never be called.
    // cuz it is not registered in the mapping table.
    // so, this is useless? I don't care.
    
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aUserDataPtr);
    ACP_UNUSED(aUserOctetLength);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ulnErrorExtended(aFnContext,
                     aRowNo0Based + 1,
                     aDescRecApd->mIndex,
                     ulERR_ABORT_CANNOT_INSERT_INTERVAL);

    return ACI_FAILURE;
}


ACI_RC ulnParamDataInBuildAny_BINARY_TIMESTAMP(ulnFnContext *aFnContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnDescRec   *aDescRecIpd,
                                              void         *aUserDataPtr,
                                              acp_sint32_t  aUserOctetLength,
                                              acp_uint32_t  aRowNo0Based,
                                              acp_uint8_t  *aConversionBuffer,
                                              ulnCharSet   *aCharSet)
{
    ulnStmt              *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc               *sDbc;
    acp_sint32_t          sUserDataLength;
    acp_bool_t            sIsNull = ACP_FALSE;
    SQL_TIMESTAMP_STRUCT *sUserDate;
    mtdDateType           sDate;

    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnBindCalcUserDataLen(ULN_CTYPE_BINARY,
                                    aDescRecApd->mMeta.mOctetLength,
                                    (acp_uint8_t *)aUserDataPtr,
                                    aUserOctetLength,
                                    &sUserDataLength,
                                    &sIsNull) != ACI_SUCCESS);

    /* typedef struct mtdDateType
       {
       acp_sint16_t year;
       acp_uint16_t mon_day_hour;
       acp_uint32_t min_sec_mic;
       } mtdDateType;
       */
    if (sIsNull == ACP_TRUE)
    {
        // mtdDateType mtdDateNull = { -32768, 0, 0 };
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.year));
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(mtcdDateNull.min_sec_mic));
    }
    else
    {
        ACI_TEST_RAISE(sUserDataLength != ACI_SIZEOF(SQL_TIMESTAMP_STRUCT), LABEL_INVALID_DATA_SIZE);

        /* typedef struct tagTIMESTAMP_STRUCT
           {
           SQLSMALLINT    year;
           SQLUSMALLINT   month;
           SQLUSMALLINT   day;
           SQLUSMALLINT   hour;
           SQLUSMALLINT   minute;
           SQLUSMALLINT   second;
           SQLUINTEGER    fraction;
           } TIMESTAMP_STRUCT;
           */
        sUserDate = (SQL_TIMESTAMP_STRUCT*) aUserDataPtr;
        ACI_TEST_RAISE(mtdDateInterfaceMakeDate(&sDate,
                                                sUserDate->year,
                                                sUserDate->month,
                                                sUserDate->day,
                                                sUserDate->hour,
                                                sUserDate->minute,
                                                sUserDate->second,
                                                (sUserDate->fraction/1000))
                 != ACI_SUCCESS, LABEL_INVALID_DATE);

        ULN_CHUNK_WR2(sStmt, &(sDate.year));
        ULN_CHUNK_WR2(sStmt, &(sDate.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(sDate.min_sec_mic));
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_BINARY_SIZE_NOT_MATCH);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_DateTime_Field_Overflow_Error,
                         "ulnParamDataInBuildAny_BINARY_TIMESTAMP");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_BINARY_DATE(ulnFnContext *aFnContext,
                                          ulnDescRec   *aDescRecApd,
                                          ulnDescRec   *aDescRecIpd,
                                          void         *aUserDataPtr,
                                          acp_sint32_t  aUserOctetLength,
                                          acp_uint32_t  aRowNo0Based,
                                          acp_uint8_t  *aConversionBuffer,
                                          ulnCharSet   *aCharSet)
{
    // binary_timestamp: consider user data as structure
    // binary_date     : consider user data as text string
    return ulnParamDataInBuildAny_CHAR_TIMESTAMP(aFnContext,
                                                 aDescRecApd,
                                                 aDescRecIpd,
                                                 aUserDataPtr,
                                                 aUserOctetLength,
                                                 aRowNo0Based,
                                                 aConversionBuffer,
                                                 aCharSet);
}

/*
 * =======================================================
 * SQL_C_DATE, SQL_C_TIME, SQL_C_TIMESTAMP --> SQL_TYPES
 * =======================================================
 */

ACI_RC ulnParamDataInBuildAny_DATE_DATE(ulnFnContext *aFnContext,
                                            ulnDescRec   *aDescRecApd,
                                            ulnDescRec   *aDescRecIpd,
                                            void         *aUserDataPtr,
                                            acp_sint32_t  aUserOctetLength,
                                            acp_uint32_t  aRowNo0Based,
                                            acp_uint8_t  *aConversionBuffer,
                                            ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    SQL_DATE_STRUCT     *sUserDate;
    mtdDateType          sDate;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.year));
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(mtcdDateNull.min_sec_mic));
    }
    else
    {
        sUserDate = (SQL_DATE_STRUCT*) aUserDataPtr;
        ACI_TEST_RAISE(mtdDateInterfaceMakeDate(&sDate,
                                                sUserDate->year,
                                                sUserDate->month,
                                                sUserDate->day,
                                                0, // hour
                                                0, // minute
                                                0, // second
                                                0) // microsec
                 != ACI_SUCCESS, LABEL_INVALID_DATE);

        ULN_CHUNK_WR2(sStmt, &(sDate.year));
        ULN_CHUNK_WR2(sStmt, &(sDate.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(sDate.min_sec_mic));
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_DateTime_Field_Overflow_Error, "Parameter");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_TIME_TIME(ulnFnContext *aFnContext,
                                            ulnDescRec   *aDescRecApd,
                                            ulnDescRec   *aDescRecIpd,
                                            void         *aUserDataPtr,
                                            acp_sint32_t  aUserOctetLength,
                                            acp_uint32_t  aRowNo0Based,
                                            acp_uint8_t  *aConversionBuffer,
                                            ulnCharSet   *aCharSet)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc              *sDbc;
    SQL_TIME_STRUCT     *sUserDate;
    mtdDateType          sDate;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.year));
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(mtcdDateNull.min_sec_mic));
    }
    else
    {
        sUserDate = (SQL_TIME_STRUCT*) aUserDataPtr;
        ACI_TEST_RAISE(mtdDateInterfaceMakeDate(&sDate,
                                                1111, // year
                                                11,   // mon
                                                11,   // day
                                                sUserDate->hour,
                                                sUserDate->minute,
                                                sUserDate->second,
                                                0)    // microsec
                 != ACI_SUCCESS, LABEL_INVALID_DATE);

        ULN_CHUNK_WR2(sStmt, &(sDate.year));
        ULN_CHUNK_WR2(sStmt, &(sDate.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(sDate.min_sec_mic));
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_DateTime_Field_Overflow_Error, "Parameter");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_TIMESTAMP_DATE(ulnFnContext *aFnContext,
                                                 ulnDescRec   *aDescRecApd,
                                                 ulnDescRec   *aDescRecIpd,
                                                 void         *aUserDataPtr,
                                                 acp_sint32_t  aUserOctetLength,
                                                 acp_uint32_t  aRowNo0Based,
                                                 acp_uint8_t  *aConversionBuffer,
                                                 ulnCharSet   *aCharSet)
{
    ulnStmt              *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc               *sDbc;
    SQL_TIMESTAMP_STRUCT *sUserDate;
    mtdDateType           sDate;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.year));
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(mtcdDateNull.min_sec_mic));
    }
    else
    {
        sUserDate = (SQL_TIMESTAMP_STRUCT*) aUserDataPtr;
        ACI_TEST_RAISE(mtdDateInterfaceMakeDate(&sDate,
                                                sUserDate->year,
                                                sUserDate->month,
                                                sUserDate->day,
                                                0, // hour
                                                0, // minute
                                                0, // second
                                                0) // microsec
                 != ACI_SUCCESS, LABEL_INVALID_DATE);

        ULN_CHUNK_WR2(sStmt, &(sDate.year));
        ULN_CHUNK_WR2(sStmt, &(sDate.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(sDate.min_sec_mic));
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_DateTime_Field_Overflow_Error, "Parameter");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_TIMESTAMP_TIME(ulnFnContext *aFnContext,
                                             ulnDescRec   *aDescRecApd,
                                             ulnDescRec   *aDescRecIpd,
                                             void         *aUserDataPtr,
                                             acp_sint32_t  aUserOctetLength,
                                             acp_uint32_t  aRowNo0Based,
                                             acp_uint8_t  *aConversionBuffer,
                                             ulnCharSet   *aCharSet)
{
    ulnStmt              *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc               *sDbc;
    SQL_TIMESTAMP_STRUCT *sUserDate;
    mtdDateType           sDate;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.year));
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(mtcdDateNull.min_sec_mic));
    }
    else
    {
        sUserDate = (SQL_TIMESTAMP_STRUCT*) aUserDataPtr;
        ACI_TEST_RAISE(mtdDateInterfaceMakeDate(&sDate,
                                                1111, // year
                                                11,   // month
                                                11,   // day
                                                sUserDate->hour,
                                                sUserDate->minute,
                                                sUserDate->second,
                                                0)
                 != ACI_SUCCESS, LABEL_INVALID_DATE);

        ULN_CHUNK_WR2(sStmt, &(sDate.year));
        ULN_CHUNK_WR2(sStmt, &(sDate.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(sDate.min_sec_mic));
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_DateTime_Field_Overflow_Error, "Parameter");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataInBuildAny_TIMESTAMP_TIMESTAMP(ulnFnContext *aFnContext,
                                                  ulnDescRec   *aDescRecApd,
                                                  ulnDescRec   *aDescRecIpd,
                                                  void         *aUserDataPtr,
                                                  acp_sint32_t  aUserOctetLength,
                                                  acp_uint32_t  aRowNo0Based,
                                                  acp_uint8_t  *aConversionBuffer,
                                                  ulnCharSet   *aCharSet)
{
    ulnStmt              *sStmt = aFnContext->mHandle.mStmt;
    ulnDbc               *sDbc;
    SQL_TIMESTAMP_STRUCT *sUserDate;
    mtdDateType           sDate;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aConversionBuffer);
    ACP_UNUSED(aCharSet);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aUserDataPtr == NULL || aUserOctetLength == SQL_NULL_DATA)
    {
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.year));
        ULN_CHUNK_WR2(sStmt, &(mtcdDateNull.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(mtcdDateNull.min_sec_mic));
    }
    else
    {
        sUserDate = (SQL_TIMESTAMP_STRUCT*) aUserDataPtr;
        ACI_TEST_RAISE(mtdDateInterfaceMakeDate(&sDate,
                                                sUserDate->year,
                                                sUserDate->month,
                                                sUserDate->day,
                                                sUserDate->hour,
                                                sUserDate->minute,
                                                sUserDate->second,
                                                (sUserDate->fraction/1000))
                 != ACI_SUCCESS, LABEL_INVALID_DATE);

        ULN_CHUNK_WR2(sStmt, &(sDate.year));
        ULN_CHUNK_WR2(sStmt, &(sDate.mon_day_hour));
        ULN_CHUNK_WR4(sStmt, &(sDate.min_sec_mic));
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_DATE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNo0Based + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_DateTime_Field_Overflow_Error, "Parameter");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

