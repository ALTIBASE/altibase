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
#include <ulnConv.h>
#include <ulnConvBit.h>
#include <ulnConvNumeric.h>
#include <ulnConvToNUMERIC.h>

void debug_dump_numeric(ulncNumeric *aNumeric)
{
    acp_uint32_t i;

    acpPrintf("mBase      = %d\n", aNumeric->mBase);
    acpPrintf("mSize      = %d\n", aNumeric->mSize);
    acpPrintf("mPrecision = %d\n", aNumeric->mPrecision);
    acpPrintf("mScale     = %d\n", aNumeric->mScale);
    acpPrintf("mSign      = %d\n", aNumeric->mSign);
    acpPrintf("mBuffer    = ");

    for(i = 0; i < aNumeric->mAllocSize; i++)
    {
        acpPrintf("%02X ", aNumeric->mBuffer[i]);
    }

    acpPrintf("\n");
}

ACI_RC ulncCHAR_NUMERIC(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    ulncNumeric         sNumeric;
    SQL_NUMERIC_STRUCT *sSqlNumeric;

    sSqlNumeric = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;

    ulncNumericInitialize(&sNumeric,
                          256,
                          ULNC_ENDIAN_LITTLE,
                          sSqlNumeric->val,
                          SQL_MAX_NUMERIC_LEN);

    switch(ulncCharToNumeric(&sNumeric,
                             ULNC_NUMERIC_ALLOCSIZE,
                             (const acp_uint8_t *)aColumn->mBuffer,
                             aColumn->mDataLength))
    {
        case ULNC_INVALID_LITERAL:
            ACI_RAISE(LABEL_INVALID_LITERAL);
            break;

        case ULNC_VALUE_OVERFLOW:
            ACI_RAISE(LABEL_OUT_OF_RANGE);
            break;

        case ULNC_SCALE_OVERFLOW:
            ACI_RAISE(LABEL_SCALE_OUT_OF_RANGE);
            break;

        case ULNC_SUCCESS:
            break;
    }

    ACI_TEST_RAISE(sNumeric.mSize > SQL_MAX_NUMERIC_LEN, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale < ACP_SINT8_MIN, LABEL_SCALE_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale > ACP_SINT8_MAX, LABEL_SCALE_OUT_OF_RANGE);

    sSqlNumeric->precision = sNumeric.mPrecision;
    sSqlNumeric->scale     = sNumeric.mScale;
    sSqlNumeric->sign      = sNumeric.mSign;

    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION(LABEL_SCALE_OUT_OF_RANGE)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_SCALE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncVARCHAR_NUMERIC(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber)
{
    return ulncCHAR_NUMERIC(aFnContext,
                            aAppBuffer,
                            aColumn,
                            aLength,
                            aRowNumber);
}

ACI_RC ulncVARBIT_NUMERIC(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    return ulncBIT_NUMERIC(aFnContext,
                           aAppBuffer,
                           aColumn,
                           aLength,
                           aRowNumber);
}

ACI_RC ulncBIT_NUMERIC(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    acp_uint8_t         sBitValue;
    SQL_NUMERIC_STRUCT *sSqlNumeric;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sSqlNumeric = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;
    sBitValue   = ulnConvBitToUChar(aColumn->mBuffer);

    sSqlNumeric->precision = 1;
    sSqlNumeric->scale     = 0;
    sSqlNumeric->sign      = 1;
    acpMemSet(sSqlNumeric->val, 0, SQL_MAX_NUMERIC_LEN);
    sSqlNumeric->val[0]    = sBitValue;

    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;
}

ACI_RC ulncSMALLINT_NUMERIC(ulnFnContext  *aFnContext,
                            ulnAppBuffer  *aAppBuffer,
                            ulnColumn     *aColumn,
                            ulnLengthPair *aLength,
                            acp_uint16_t   aRowNumber)
{
    acp_sint16_t        sShortValue;
    SQL_NUMERIC_STRUCT *sSqlNumeric;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sSqlNumeric = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;
    sShortValue = *(acp_sint16_t *)aColumn->mBuffer;

    ulncSLongToSQLNUMERIC(sShortValue, sSqlNumeric);

    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;
}

ACI_RC ulncINTEGER_NUMERIC(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber)
{
    acp_sint32_t        sIntValue;
    SQL_NUMERIC_STRUCT *sSqlNumeric;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sSqlNumeric = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;
    sIntValue   = *(acp_sint32_t *)aColumn->mBuffer;

    ulncSLongToSQLNUMERIC(sIntValue, sSqlNumeric);

    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;
}

ACI_RC ulncBIGINT_NUMERIC(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    acp_sint64_t        sBigIntValue;
    SQL_NUMERIC_STRUCT *sSqlNumeric;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sSqlNumeric  = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;
    sBigIntValue = *(acp_sint64_t *)aColumn->mBuffer;

    ulncSLongToSQLNUMERIC(sBigIntValue, sSqlNumeric);

    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;
}

ACI_RC ulncREAL_NUMERIC(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_float_t         sRealValue;
    ulncNumeric         sNumeric;
    SQL_NUMERIC_STRUCT *sSqlNumeric;


    /*
     * single precision floating point number 는 23(24) 비트의 significand 를 갖는다.
     * 최대 절대값은 16777216 로써, 8자리이다. 여기에 소숫점 이하 7자리, 소숫점, 부호, 
     * null term 까지 더하면 8 + 7 + 1 + 1 + 1 = 18 자리가 필요하다.
     * 그냥 넉넉히 20바이트로 할당하자.
     */
    acp_char_t          sTmpBuffer[20];

    sSqlNumeric = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;
    sRealValue  = *(acp_float_t *)aColumn->mBuffer;

    ulncNumericInitialize(&sNumeric,
                          256,
                          ULNC_ENDIAN_LITTLE,
                          sSqlNumeric->val,
                          SQL_MAX_NUMERIC_LEN);

    acpSnprintf(sTmpBuffer, 20, "%"ACI_FLOAT_G_FMT, sRealValue);

    switch(ulncCharToNumeric(&sNumeric,
                             ULNC_NUMERIC_ALLOCSIZE,
                             (const acp_uint8_t *)sTmpBuffer,
                             acpCStrLen(sTmpBuffer, 20)))
    {
        case ULNC_INVALID_LITERAL:
            ACI_RAISE(LABEL_INVALID_LITERAL);
            break;

        case ULNC_VALUE_OVERFLOW:
            ACI_RAISE(LABEL_OUT_OF_RANGE);
            break;

        case ULNC_SCALE_OVERFLOW:
            ACI_RAISE(LABEL_SCALE_OUT_OF_RANGE);
            break;

        case ULNC_SUCCESS:
            break;
    }

    ACI_TEST_RAISE(sNumeric.mSize > SQL_MAX_NUMERIC_LEN, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale < ACP_SINT8_MIN, LABEL_SCALE_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale > ACP_SINT8_MAX, LABEL_SCALE_OUT_OF_RANGE);

    sSqlNumeric->precision = sNumeric.mPrecision;
    sSqlNumeric->scale     = sNumeric.mScale;
    sSqlNumeric->sign      = sNumeric.mSign;

    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION(LABEL_SCALE_OUT_OF_RANGE)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_SCALE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncDOUBLE_NUMERIC(ulnFnContext  *aFnContext,
                            ulnAppBuffer  *aAppBuffer,
                            ulnColumn     *aColumn,
                            ulnLengthPair *aLength,
                            acp_uint16_t   aRowNumber)
{
    acp_double_t        sDoubleValue;
    ulncNumeric         sNumeric;
    SQL_NUMERIC_STRUCT *sSqlNumeric;


    /*
     * double precision floating point number 는 52(53) 비트의 significand 를 갖는다.
     * 최대 절대값은 9007199254740992 로써, 16자리이다. 여기에 소숫점 이하 15자리, 소숫점, 부호, 
     * null term 까지 더하면 16 + 15 + 1 + 1 + 1 = 34 자리가 필요하다.
     * 그냥 넉넉히 40바이트로 할당하자.
     */
    acp_char_t           sTmpBuffer[40];

    sSqlNumeric  = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;
    sDoubleValue = *(acp_double_t *)aColumn->mBuffer;

    ulncNumericInitialize(&sNumeric,
                          256,
                          ULNC_ENDIAN_LITTLE,
                          sSqlNumeric->val,
                          SQL_MAX_NUMERIC_LEN);

    acpSnprintf(sTmpBuffer, 40, "%"ACI_DOUBLE_G_FMT, sDoubleValue);

    switch(ulncCharToNumeric(&sNumeric,
                             ULNC_NUMERIC_ALLOCSIZE,
                             (const acp_uint8_t *)sTmpBuffer,
                             acpCStrLen(sTmpBuffer, 40)))
    {
        case ULNC_INVALID_LITERAL:
            ACI_RAISE(LABEL_INVALID_LITERAL);
            break;

        case ULNC_VALUE_OVERFLOW:
            ACI_RAISE(LABEL_OUT_OF_RANGE);
            break;

        case ULNC_SCALE_OVERFLOW:
            ACI_RAISE(LABEL_SCALE_OUT_OF_RANGE);
            break;

        case ULNC_SUCCESS:
            break;
    }

    ACI_TEST_RAISE(sNumeric.mSize > SQL_MAX_NUMERIC_LEN, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale < ACP_SINT8_MIN, LABEL_SCALE_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale > ACP_SINT8_MAX, LABEL_SCALE_OUT_OF_RANGE);

    sSqlNumeric->precision = sNumeric.mPrecision;
    sSqlNumeric->scale     = sNumeric.mScale;
    sSqlNumeric->sign      = sNumeric.mSign;

    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION(LABEL_SCALE_OUT_OF_RANGE)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_SCALE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncINTERVAL_NUMERIC(ulnFnContext  *aFnContext,
                            ulnAppBuffer  *aAppBuffer,
                            ulnColumn     *aColumn,
                            ulnLengthPair *aLength,
                            acp_uint16_t   aRowNumber)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aAppBuffer);
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aLength);
    ACP_UNUSED(aRowNumber);

    return ACI_SUCCESS;
}

ACI_RC ulncNUMERIC_NUMERIC(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber)
{
    cmtNumeric         *sCmNumeric;
    SQL_NUMERIC_STRUCT *sSqlNumeric;
    ulnStmt            *sStmt = aFnContext->mHandle.mStmt;
    ulnDesc            *sDescARD;
    ulnDescRec         *sDescRec;
    acp_sint16_t        sUserPrecision;
    acp_sint16_t        sValuePrecision;
    acp_sint16_t        sUserScale;
    acp_sint16_t        sScaleDiff;
    ulncNumeric         sTmpNum;
    acp_uint8_t         sTmpNumBuf[SQL_MAX_NUMERIC_LEN];
    acp_uint32_t        sTmpNumMantissaLen;
    acp_sint32_t        i;

    sCmNumeric  = (cmtNumeric *)aColumn->mBuffer;
    sSqlNumeric = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;

    /*
     * BUGBUG : cmtNumeric 의 scale 은 acp_sint16_t 인데, SQL_NUMERIC_STRUCT 의 scale 은 acp_char_t 이다.
     *          스케일 오버플로우가 나면 어떻게 하나?
     *          fractional truncation 을 내어주고, 적절히 조정해야 정답이지만,
     *          일단, 그냥 에러를 내고서 모르는체 하자 -_-;
     *
     *          그런데, 어떤 경우에 저렇게 스케일이 커지나?
     *
     *          e-90 이 numeric scale overflow 나는 한계점임.
     *          e-90 OK
     *          e-91 Overflow
     *
     *          INSERT INTO T1 VALUES('-1.2345678901234567890123456789012345678e-90',
     *                                '-1.2345678901234567890123456789012345678e+125');
     */

    ACI_TEST_RAISE(sCmNumeric->mScale < ACP_SINT8_MIN, LABEL_SCALE_OUT_OF_RANGE);
    ACI_TEST_RAISE(sCmNumeric->mScale > ACP_SINT8_MAX, LABEL_SCALE_OUT_OF_RANGE);

    sSqlNumeric->sign      = sCmNumeric->mSign;
    sSqlNumeric->precision = sCmNumeric->mPrecision;
    sSqlNumeric->scale     = sCmNumeric->mScale;
    acpMemCpy(sSqlNumeric->val, sCmNumeric->mData, SQL_MAX_NUMERIC_LEN);

    /* BUG-47384 User Type이 SQL_C_BINARY인 경우 user precision, scale은 무시한다. */
    ACI_TEST_RAISE( aAppBuffer->mCTYPE == ULN_CTYPE_BINARY, SKIP_PRECSCALE_CONV );
    /* BUG-37101 */
    sDescARD = ulnStmtGetArd(sStmt);
    ACI_TEST_RAISE(sDescARD == NULL, SKIP_PRECSCALE_CONV); /* BUG-37256 */
    sDescRec = ulnDescGetDescRec(sDescARD, aColumn->mColumnNumber);
    ACI_TEST_RAISE(sDescRec == NULL, SKIP_PRECSCALE_CONV); /* BUG-45453 */
    /* user precision, scale에 맞게 변환 */
    {
        sUserPrecision = ulnMetaGetPrecision(&sDescRec->mMeta);
        sUserScale = ulnMetaGetScale(&sDescRec->mMeta);

        if ((sUserScale != ULN_NUMERIC_UNDEF_SCALE)
         && (sUserScale != sSqlNumeric->scale))
        {
            sScaleDiff = sUserScale - sSqlNumeric->scale;
            sTmpNumMantissaLen = SQL_MAX_NUMERIC_LEN;
            for (i = SQL_MAX_NUMERIC_LEN - 1; i >= 0; i--)
            {
                if (sSqlNumeric->val[i] != 0)
                {
                    break;
                }
                sTmpNumMantissaLen--;
            }
            ulncNumericInitFromData(&sTmpNum,
                                    256,
                                    ULNC_ENDIAN_LITTLE,
                                    sSqlNumeric->val,
                                    SQL_MAX_NUMERIC_LEN,
                                    sTmpNumMantissaLen);

            /* user-scale은 value가 짤리지 않을 정도여야 한다. */
            if (sScaleDiff > 0)
            {
                for (; sScaleDiff != 0; sScaleDiff--)
                {
                    ACI_TEST_RAISE(sTmpNum.multiply(&sTmpNum, 10) != ACI_SUCCESS,
                                   LABEL_SCALE_OUT_OF_RANGE);
                }
            }
            else
            {
                for (; sScaleDiff != 0; sScaleDiff++)
                {
                    ACI_TEST_RAISE(sTmpNum.devide(&sTmpNum, 10) != ACI_SUCCESS,
                                   LABEL_SCALE_OUT_OF_RANGE);
                }
            }

            sSqlNumeric->scale = sUserScale;
        }

        if ((sUserPrecision != ULN_NUMERIC_UNDEF_PRECISION)
         || (sUserScale != ULN_NUMERIC_UNDEF_SCALE))
        {
            acpMemCpy(sTmpNumBuf, sSqlNumeric->val, SQL_MAX_NUMERIC_LEN);
            sTmpNumMantissaLen = SQL_MAX_NUMERIC_LEN;
            for (i = SQL_MAX_NUMERIC_LEN - 1; i >= 0; i--)
            {
                if (sTmpNumBuf[i] != 0)
                {
                    break;
                }
                sTmpNumMantissaLen--;
            }
            ulncNumericInitFromData(&sTmpNum,
                                    256,
                                    ULNC_ENDIAN_LITTLE,
                                    sTmpNumBuf,
                                    SQL_MAX_NUMERIC_LEN,
                                    sTmpNumMantissaLen);
            for (sValuePrecision = 0; sTmpNum.mSize > 0; sValuePrecision++)
            {
                (void) sTmpNum.devide(&sTmpNum, 10);
            }

            if (sUserPrecision == ULN_NUMERIC_UNDEF_PRECISION)
            {
                sUserPrecision = ULN_NUMERIC_MAX_PRECISION;
            }
            /* user-precision은 value를 다 담을 수 있을 정도여야 한다. */
            ACI_TEST_RAISE(sValuePrecision > sUserPrecision,
                           LABEL_PRECISION_OUT_OF_RANGE);

            sSqlNumeric->precision = sUserPrecision;
        }
    }
    ACI_EXCEPTION_CONT(SKIP_PRECSCALE_CONV);

    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_PRECISION_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_PRECISION_OUT_OF_RANGE);
    }
    ACI_EXCEPTION(LABEL_SCALE_OUT_OF_RANGE)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_SCALE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncNCHAR_NUMERIC(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    ulncNumeric         sNumeric;
    SQL_NUMERIC_STRUCT *sSqlNumeric;
    ulnDbc             *sDbc;
    ulnCharSet          sCharSet;
    acp_sint32_t        sState = 0;

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnCharSetConvert(&sCharSet,
                               aFnContext,
                               NULL,
                               (const mtlModule*)sDbc->mNcharCharsetLangModule,
                               sDbc->mClientCharsetLangModule,    //BUG-22684
                               (void*)aColumn->mBuffer,
                               aColumn->mDataLength,
                               CONV_DATA_OUT)
             != ACI_SUCCESS);

    sSqlNumeric = (SQL_NUMERIC_STRUCT *)aAppBuffer->mBuffer;

    ulncNumericInitialize(&sNumeric,
                          256,
                          ULNC_ENDIAN_LITTLE,
                          sSqlNumeric->val,
                          SQL_MAX_NUMERIC_LEN);

    switch(ulncCharToNumeric(&sNumeric,
                             ULNC_NUMERIC_ALLOCSIZE,
                             (const acp_uint8_t *)ulnCharSetGetConvertedText(&sCharSet),
                             ulnCharSetGetConvertedTextLen(&sCharSet)))
    {
        case ULNC_INVALID_LITERAL:
            ACI_RAISE(LABEL_INVALID_LITERAL);
            break;

        case ULNC_VALUE_OVERFLOW:
            ACI_RAISE(LABEL_OUT_OF_RANGE);
            break;

        case ULNC_SCALE_OVERFLOW:
            ACI_RAISE(LABEL_SCALE_OUT_OF_RANGE);
            break;

        case ULNC_SUCCESS:
            break;
    }

    ACI_TEST_RAISE(sNumeric.mSize > SQL_MAX_NUMERIC_LEN, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale < ACP_SINT8_MIN, LABEL_SCALE_OUT_OF_RANGE);
    ACI_TEST_RAISE(sNumeric.mScale > ACP_SINT8_MAX, LABEL_SCALE_OUT_OF_RANGE);

    sSqlNumeric->precision = sNumeric.mPrecision;
    sSqlNumeric->scale     = sNumeric.mScale;
    sSqlNumeric->sign      = sNumeric.mSign;

    aLength->mWritten = ACI_SIZEOF(SQL_NUMERIC_STRUCT);
    aLength->mNeeded  = ACI_SIZEOF(SQL_NUMERIC_STRUCT);

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION(LABEL_SCALE_OUT_OF_RANGE)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_SCALE_OUT_OF_RANGE);
    }

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return ACI_FAILURE;
}

ACI_RC ulncNVARCHAR_NUMERIC(ulnFnContext  *aFnContext,
                            ulnAppBuffer  *aAppBuffer,
                            ulnColumn     *aColumn,
                            ulnLengthPair *aLength,
                            acp_uint16_t   aRowNumber)
{
    return ulncNCHAR_NUMERIC(aFnContext,
                             aAppBuffer,
                             aColumn,
                             aLength,
                             aRowNumber);
}
