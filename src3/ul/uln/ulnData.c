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

/*
 * 이 파일은 아래의 두가지를 구현하고 있다.
 * 가히 ul 의 핵심부들 중의 하나라고 할 수 있다.
 * 그렇지만, 지저분하다. 어쩔 수 없다.
 *
 *      1. 서버로부터 넘겨받은 데이터를 사용자 버퍼 혹은 ulnCache 의 버퍼에 쓰는 루틴들
 *      2. 사용자 버퍼에서 서버로 넘겨줄 데이터를 cmtAny 에 쓰는 루틴들
 *
 * 즉, Data 의 입/출력에 관련된 루틴들을 구현하고 있다.
 *
 * =================================================================
 *      서버로 데이터를 전송하기 위해서
 *      사용자 버퍼에서 읽어서 cmtAny 에 쓰는 루틴들
 *
 *       ------------+   +---------- ulnData ---------+  +--------
 *       Buffer      |   |                            |  |
 *       in the      ------> Write Buffer to Packet ---->| CM
 *       Application |   |                            |  | cmtAny
 *       or          <------ Write Packet to Buffer <----|
 *       ulnCache    |   |                            |  |
 *       ------------+   +----------------------------+  +--------
 *
 *      서버로부터 전달되어 온 데이터를 사용자의 버퍼
 *      혹은 ulnCache 의 버퍼에 쓰는 루틴들
 * =================================================================
 */

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnTypes.h>
#include <ulnData.h>
#include <ulnConv.h>
#include <ulnConvNumeric.h>

/*
 * =====================================================================
 * SQLBIGINT 와 관련해서
 * 컴파일 타임에 구조체인지, 64비트integer 인지 판단해서 처리하는 함수들
 *
 * Note : 아래 함수들의 prototype 은 uln.h 에 존재한다. uln 이외의 모듈에서도
 *        호출해야 하는 함수들이기 때문이다.
 * =====================================================================
 */

void ulnTypeConvertSLongToBIGINT(acp_sint64_t aInputLongValue, SQLBIGINT *aOutputLongValuePtr)
{
#if (SIZEOF_LONG == 8)
    *(acp_uint64_t *)aOutputLongValuePtr = aInputLongValue;
#elif defined(HAVE_LONG_LONG)
    *(acp_uint64_t *)aOutputLongValuePtr = aInputLongValue;
#else
    aOutputLongValuePtr->hiword =
        (acp_sint32_t)((acp_uint64_t)aInputLongValue / ACP_UINT64_LITERAL(0x100000000));

    aOutputLongValuePtr->loword =
        (acp_uint32_t)((acp_uint64_t)aInputLongValue -
               (((acp_uint64_t)aInputLongValue / ACP_UINT64_LITERAL(0x100000000)) * ACP_UINT64_LITERAL(0x100000000)));
#endif /* SIZEOF_LONG == 8 */
}

void ulnTypeConvertULongToUBIGINT(acp_uint64_t aInputLongValue, SQLUBIGINT *aOutputLongValuePtr)
{
#if (SIZEOF_LONG == 8)
    *(acp_uint64_t *)aOutputLongValuePtr = aInputLongValue;
#elif defined(HAVE_LONG_LONG)
    *(acp_uint64_t *)aOutputLongValuePtr = aInputLongValue;
#else
    /*
     * Note : bitwise shift 연산을 쓰지 않고 아래와 같이 arithmetic 연산을 쓴 이유는
     *        byte order 에 대한 차이를 없애기 위해서이다.
     */
    aOutputLongValuePtr->hiword =
        (acp_uint32_t)(aInputLongValue / ACP_UINT64_LITERAL(0x100000000));

    aOutputLongValuePtr->loword =
        (acp_uint32_t)(aInputLongValue -
               ((aInputLongValue / ACP_UINT64_LITERAL(0x100000000)) * ACP_UINT64_LITERAL(0x100000000)));
#endif /* SIZEOF_LONG == 8 */
}

acp_sint64_t ulnTypeConvertBIGINTtoSLong(SQLBIGINT aInput)
{
    acp_sint64_t sOutputValue;

#if (SIZEOF_LONG == 8)
    sOutputValue = aInput;
#elif defined(HAVE_LONG_LONG)
    sOutputValue = aInput;
#else
    sOutputValue = aInput.hiword * ACP_UINT64_LITERAL(0x100000000) + aInput.loword;
#endif /* SIZEOF_LONG == 8 */

    return sOutputValue;
}

acp_uint64_t ulnTypeConvertUBIGINTtoULong(SQLUBIGINT aInput)
{
    acp_sint64_t sOutputValue;

#if (SIZEOF_LONG == 8)
    sOutputValue = aInput;
#elif defined(HAVE_LONG_LONG)
    sOutputValue = aInput;
#else
    sOutputValue = aInput.hiword * ACP_UINT64_LITERAL(0x100000000) + aInput.loword;
#endif /* SIZEOF_LONG == 8 */

    return sOutputValue;
}

void ulnDataBuildColumnZero( ulnFnContext *aFnContext,
                             ulnRow       *aRow,
                             ulnColumn    *aColumn )
{
    ulnStmt      *sStmt       = aFnContext->mHandle.mStmt;

    /* PROJ-1789 Updatable Scrollable Cursor: (Impl.) 북마크 == Position */

    /* memory access violation 문제를 피하기위해 align을 맞춰준다.
        * mBuffer의 크기(ULN_CACHE_MAX_SIZE_FOR_FIXED_TYPE)는
        * ACI_SIZEOF(acp_sint64_t) * 2 보다 크므로 이렇게 해도 문제 없다. */
    aColumn->mBuffer = (acp_uint8_t *) ACP_ALIGN8_PTR(aColumn->mBuffer);

    /* VARIABLE일 때의 최대값을 64bit signed int로 제한하는 이유는
        * CursorPosition이 sint64라 그 이상은 의미가 없기 때문. */
    if (ulnStmtGetAttrUseBookMarks(sStmt) == SQL_UB_VARIABLE)
    {
        *((acp_sint64_t *) aColumn->mBuffer) = aRow->mRowNumber;
        aColumn->mDataLength = ACI_SIZEOF(acp_sint64_t);
        aColumn->mMtype = ULN_MTYPE_BIGINT;
    }
    else /* is SQL_UB_FIXED (== SQL_UB_ON) */
    {
        *((acp_sint32_t *) aColumn->mBuffer) = aRow->mRowNumber;
        aColumn->mDataLength = ACI_SIZEOF(acp_sint32_t);
        aColumn->mMtype = ULN_MTYPE_INTEGER;
    }

    aColumn->mPrecision  = 0;
}

ACI_RC ulnCopyToUserBufferForSimpleQuery(ulnFnContext     *aFnContext,
                                         ulnStmt          *aKeysetStmt,
                                         acp_uint8_t      *aSrc,
                                         ulnDescRec       *aDescRecArd,
                                         ulnDescRec       *aDescRecIrd)
{
    ulnDbc           *sDbc = NULL;
    ACI_RC            sRet = ACI_FAILURE;
    acp_uint16_t      sLen16;
    acp_uint8_t       sLen8;
    ulnIndLenPtrPair  sUserIndLenPair = {NULL, NULL};
    ulnConvFunction  *sFilter = NULL;
    ulnLengthPair     sLengthPair     = {ULN_vLEN(0), ULN_vLEN(0)};
    ulnAppBuffer      sAppBuff;
    ulnColumn         sColumn = {0,};

    cmtNumeric        sNumericBuf;
    acp_uint8_t       sShortBuf[2]  = {0,0};
    acp_uint8_t       sIntBuf[4]    = {0,0,0,0};
    acp_uint8_t       sBigIntBuf[8] = {0,0,0,0,0,0,0,0};
    mtdDateType       sTimeBuf    = {0,0,0};
    acp_uint8_t      *sCharBuf    = NULL;
    acp_bool_t        sConversion = ACP_TRUE;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aDescRecArd != NULL)
    {
        ulnBindCalcUserIndLenPair(aDescRecArd, 0, &sUserIndLenPair);

        ACI_TEST_RAISE((aDescRecArd->mDataPtr == NULL), LABEL_SKIP_CONVERSION);

        sAppBuff.mCTYPE        = ulnMetaGetCTYPE(&aDescRecArd->mMeta);
        sAppBuff.mBuffer       = (acp_uint8_t *)ulnBindCalcUserDataAddr(aDescRecArd,0);
        sAppBuff.mBufferSize   = ulnMetaGetOctetLength(&aDescRecArd->mMeta);
        sAppBuff.mColumnStatus = ULN_ROW_SUCCESS;

        /* BUG-45258 */
        sColumn.mColumnNumber = aDescRecIrd->mIndex;
        sColumn.mMtype = (acp_uint16_t)aDescRecIrd->mMeta.mMTYPE;
        sColumn.mDataLength = aDescRecIrd->mMaxByteSize;

        sFilter = ulnConvGetFilter(sAppBuff.mCTYPE, (ulnMTypeID)sColumn.mMtype);
        ACI_TEST_RAISE(sFilter == NULL, LABEL_CONV_NOT_APPLICABLE);

        switch(sColumn.mMtype)
        {
            case ULN_MTYPE_CHAR :
            case ULN_MTYPE_VARCHAR :
                sLen16 = *((acp_uint16_t*)aSrc);
                ACI_TEST(sLen16 > aDescRecIrd->mMaxByteSize);
                ACI_TEST_RAISE((sLen16 == 0), LABEL_NULL_DATA);

                if ((sAppBuff.mCTYPE == ULN_CTYPE_CHAR) &&
                    (sDbc->mCharsetLangModule->id == sDbc->mClientCharsetLangModule->id))
                {
                    /*
                     * BUG-45568 ulnConvFunction을 사용하지 않기(성능때문인가?)에
                     *           사용자 버퍼 크기를 고려해야 한다.
                     */
                    if (sLen16 >= sAppBuff.mBufferSize)
                    {
                        ulnErrorExtended(aFnContext,
                                         1,
                                         sColumn.mColumnNumber,
                                         ulERR_IGNORE_RIGHT_TRUNCATED);
                        sAppBuff.mColumnStatus = ULN_ROW_SUCCESS_WITH_INFO;

                        acpMemCpy(sAppBuff.mBuffer, (aSrc + 2), sAppBuff.mBufferSize - 1);
                        sAppBuff.mBuffer[sAppBuff.mBufferSize - 1] = '\0';
                        sLengthPair.mWritten = sAppBuff.mBufferSize - 1;
                    }
                    else
                    {
                        acpMemCpy(sAppBuff.mBuffer, (aSrc + 2), sLen16);
                        sAppBuff.mBuffer[sLen16] = '\0';
                        sLengthPair.mWritten = sLen16;
                    }

                    sConversion         = ACP_FALSE;
                    sColumn.mBuffer     = sAppBuff.mBuffer;
                    sLengthPair.mNeeded = sLen16;
                }
                else
                {
                    ACI_TEST(acpMemAlloc((void**)&sCharBuf, (acp_size_t)(sLen16 + 1)));
                    acpMemCpy(sCharBuf, (aSrc + 2), sLen16);
                    sCharBuf[sLen16] = '\0';  /* BUG-45568 */
                    sColumn.mDataLength = sLen16;
                    sColumn.mBuffer     = sCharBuf;
                    sColumn.mMTLength   = sLen16+2;
                    sColumn.mPrecision  = 0;
                }
                break;
            case ULN_MTYPE_FLOAT :
            case ULN_MTYPE_NUMERIC :
                sLen8                = *((acp_uint8_t*)aSrc);
                sColumn.mMTLength   = 1;
                sColumn.mPrecision  = 0;
                
                ACI_TEST_RAISE((sLen8 == 0), LABEL_NULL_DATA);

                ACI_TEST( ulncMtNumericToCmNumeric(&sNumericBuf, (mtdNumericType*)aSrc)
                          != ACI_SUCCESS );
                          
                sColumn.mDataLength = ACI_SIZEOF(cmtNumeric);
                sColumn.mMTLength   = sLen8 + 1;
                sColumn.mPrecision  = 0;
                sColumn.mBuffer     = (acp_uint8_t*)&sNumericBuf;
                break;
            case ULN_MTYPE_SMALLINT :
                if ((sAppBuff.mCTYPE == ULN_CTYPE_SSHORT) ||
                    (sAppBuff.mCTYPE == ULN_CTYPE_USHORT))
                {
                    cmNoEndianAssign2((acp_uint16_t*)sAppBuff.mBuffer, (acp_uint16_t*)aSrc);
                    ACI_TEST_RAISE((*((acp_sint16_t*)sAppBuff.mBuffer) == MTD_SMALLINT_NULL),
                                   LABEL_NULL_DATA);
                    sLengthPair.mWritten = 2;
                    sLengthPair.mNeeded  = 2;
                    sConversion          = ACP_FALSE;
                }
                else
                {
                    cmNoEndianAssign2((acp_uint16_t*)&sShortBuf, (acp_uint16_t*)aSrc);
                    sColumn.mBuffer   = (acp_uint8_t*)&sShortBuf;
                    ACI_TEST_RAISE((*((acp_sint16_t*)sColumn.mBuffer) == MTD_SMALLINT_NULL),
                                   LABEL_NULL_DATA);
                    sColumn.mDataLength = 2;
                    sColumn.mMTLength   = 2;
                    sColumn.mPrecision  = 0;
                }
                break;
            case ULN_MTYPE_INTEGER :
                if ((sAppBuff.mCTYPE == ULN_CTYPE_SLONG) ||
                     sAppBuff.mCTYPE == ULN_CTYPE_ULONG)
                {
                    cmNoEndianAssign4((acp_uint32_t*)sAppBuff.mBuffer, (acp_uint32_t*)aSrc);
                    ACI_TEST_RAISE((*((acp_sint32_t*)sAppBuff.mBuffer) == MTD_INTEGER_NULL),
                                   LABEL_NULL_DATA);
                    sLengthPair.mWritten = 4;
                    sLengthPair.mNeeded  = 4;
                    sConversion          = ACP_FALSE;
                }
                else
                {
                    cmNoEndianAssign4((acp_uint32_t*)&sIntBuf, (acp_uint32_t*)aSrc);
                    sColumn.mBuffer   = (acp_uint8_t*)&sIntBuf;
                    ACI_TEST_RAISE((*((acp_sint32_t*)sColumn.mBuffer) == MTD_INTEGER_NULL),
                                   LABEL_NULL_DATA);
                    sColumn.mDataLength = 4;
                    sColumn.mMTLength   = 4;
                    sColumn.mPrecision  = 0;
                }
                break;
            case ULN_MTYPE_BIGINT :
                if ((sAppBuff.mCTYPE == ULN_CTYPE_UBIGINT) ||
                    (sAppBuff.mCTYPE == ULN_CTYPE_SBIGINT))
                {
                    cmNoEndianAssign8((acp_uint64_t*)sAppBuff.mBuffer, (acp_uint64_t*)aSrc);
                    ACI_TEST_RAISE((*((acp_sint64_t*)sAppBuff.mBuffer) == MTD_BIGINT_NULL),
                                   LABEL_NULL_DATA);
                    sLengthPair.mWritten = 8;
                    sLengthPair.mNeeded  = 8;
                    sConversion          = ACP_FALSE;
                }
                else
                {
                    cmNoEndianAssign8((acp_uint64_t*)&sBigIntBuf, (acp_uint64_t*)aSrc);
                    sColumn.mBuffer   = (acp_uint8_t*)&sBigIntBuf;
                    ACI_TEST_RAISE((*((acp_sint64_t*)sColumn.mBuffer) == MTD_BIGINT_NULL),
                                   LABEL_NULL_DATA);
                    sColumn.mDataLength = 8;
                    sColumn.mMTLength   = 8;
                    sColumn.mPrecision  = 0;
                }
                break;
            case ULN_MTYPE_TIMESTAMP :
                cmNoEndianAssign2((acp_uint16_t*)&sTimeBuf.year        , (acp_uint16_t*)aSrc  );
                cmNoEndianAssign2((acp_uint16_t*)&sTimeBuf.mon_day_hour, (acp_uint16_t*)(aSrc+2));
                cmNoEndianAssign4((acp_uint32_t*)&sTimeBuf.min_sec_mic , (acp_uint32_t*)(aSrc+4));
                sColumn.mBuffer     = (acp_uint8_t*)&sTimeBuf;
                sColumn.mDataLength = 8;
                sColumn.mMTLength   = 8;
                sColumn.mPrecision  = 0;

                ACI_TEST_RAISE((MTD_DATE_IS_NULL((mtdDateType*)sColumn.mBuffer) == ACP_TRUE), LABEL_NULL_DATA);
                break;
            case ULN_MTYPE_BYTE :
            case ULN_MTYPE_VARBYTE :
                sLen16 = *((acp_uint16_t*)aSrc);

                ACI_TEST(sLen16 > aDescRecIrd->mMaxByteSize);
                ACI_TEST_RAISE((sLen16 == 0), LABEL_NULL_DATA);

                sColumn.mBuffer     = (aSrc + 2);
                sColumn.mDataLength = sLen16;
                sColumn.mMTLength   = aDescRecIrd->mMaxByteSize;
                sColumn.mPrecision  = 0;

                break;
            default :
                ACE_ASSERT(0);
                break;
        }

        sColumn.mGDPosition = 0;
        sColumn.mRemainTextLen = 0;

        ACP_TEST_RAISE((sConversion==ACP_FALSE),LABEL_SKIP_CONVERSION);

        if (sFilter(aFnContext,
                    &sAppBuff,
                    &sColumn,
                    &sLengthPair,
                    1) == ACI_SUCCESS)
        {
            ACI_EXCEPTION_CONT(LABEL_SKIP_CONVERSION);
            /*
             * 사용자에게 리턴하는 길이
             */
            if (ulnBindSetUserIndLenValue(&sUserIndLenPair, sLengthPair.mNeeded) != ACI_SUCCESS)
            {
                /*
                 * 22002 :
                 *
                 * NULL 이 컬럼에 fetch 되어 와서, SQL_NULL_DATA 를 사용자가 지정한
                 * StrLen_or_IndPtr 에 써 줘야 하는데, 이녀석이 NULL 포인터이다.
                 * 그럴때에 발생시켜 주는 에러.
                 */
                ulnErrorExtended(aFnContext,
                                 1,
                                 sColumn.mColumnNumber,
                                 ulERR_ABORT_INDICATOR_REQUIRED_BUT_NOT_SUPPLIED_ERR);

                sAppBuff.mColumnStatus = ULN_ROW_ERROR;
            }
        }
        else
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, 0, SQL_ROW_ERROR);
        }

        if (sCharBuf != NULL)
        {
            acpMemFree(sColumn.mBuffer);
            sColumn.mBuffer = NULL;
        }
        else
        {
            /* do nothing.*/
        }
    }
    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_NULL_DATA)
    {
        if (ulnBindSetUserIndLenValue(&sUserIndLenPair, SQL_NULL_DATA) != ACI_SUCCESS)
        {
            ulnErrorExtended(aFnContext,
                             1,
                             sColumn.mColumnNumber,
                             ulERR_ABORT_INDICATOR_REQUIRED_BUT_NOT_SUPPLIED_ERR);
        }
        sRet = ACI_SUCCESS;
    }
    ACI_EXCEPTION(LABEL_CONV_NOT_APPLICABLE)
    {
        /*
         * 07006 : Restricted data type attribute violation
         */
        ulnErrorExtended(aFnContext,
                         1,
                         sColumn.mColumnNumber,
                         ulERR_ABORT_INVALID_CONVERSION);

        sAppBuff.mColumnStatus = ULN_ROW_ERROR;

    }
    ACI_EXCEPTION_END;

    if (sCharBuf != NULL)
    {
        acpMemFree(sColumn.mBuffer);
        sColumn.mBuffer = NULL;
    }
    else
    {
        /* do nothing.*/
    }

    return sRet;
}

/*
 * =================================================================
 *
 * ulnCache(MT data) ---> ulnColumn  ---> User Buffer
 *    |                     ^    |             ^
 *    |_____________________|    |_____________|
 *             |                        |
 *  ulnDataBuildColumnFromMT            |
 *                                      |
 *                                      |
 *                                 ulnConvert
 *
 * =================================================================
 */
ACI_RC ulnDataBuildColumnFromMT(ulnFnContext *aFnContext,
                                acp_uint8_t  *aSrc,
                                ulnColumn    *aColumn)
{
    acp_uint32_t      sLen32;
    acp_uint16_t      sLen16;
    acp_uint8_t       sLen8;
    ulnLob           *sLob;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    acp_uint64_t      sLobLocatorId;
    acp_uint64_t      sLobSize;

    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;

    switch(aColumn->mMtype)
    {
        case ULN_MTYPE_NULL :
            *(aColumn->mBuffer + 0) = *(aSrc + 0);

            aColumn->mDataLength = SQL_NULL_DATA;
            aColumn->mMTLength   = 1;
            aColumn->mPrecision  = 0;
            break;

        case ULN_MTYPE_BINARY :
            CM_ENDIAN_ASSIGN4(&sLen32, aSrc);

            aColumn->mBuffer     = (aSrc + 8);
            aColumn->mDataLength = sLen32;
            aColumn->mMTLength   = (aColumn->mDataLength) + 8;
            aColumn->mPrecision  = 0;

            if( aColumn->mDataLength == 0)
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_CHAR :
        case ULN_MTYPE_VARCHAR :
        case ULN_MTYPE_NCHAR :
        case ULN_MTYPE_NVARCHAR :
            CM_ENDIAN_ASSIGN2(&sLen16, aSrc);

            aColumn->mBuffer     = (aSrc + 2);
            aColumn->mDataLength = sLen16;
            aColumn->mMTLength   = (aColumn->mDataLength) + 2;
            aColumn->mPrecision  = 0;

            if( aColumn->mDataLength == 0)
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_FLOAT :
        case ULN_MTYPE_NUMERIC :

            sLen8 = *((acp_uint8_t*)aSrc);

            if( sLen8 != 0 )
            {
                ACI_TEST( ulncMtNumericToCmNumeric(
                                    (cmtNumeric*)aColumn->mBuffer,
                                    (mtdNumericType*)aSrc)
                          != ACI_SUCCESS );

                aColumn->mDataLength = ACI_SIZEOF(cmtNumeric);
                aColumn->mMTLength   = sLen8 + 1;
                aColumn->mPrecision  = 0;
            }
            else
            {
                aColumn->mDataLength = SQL_NULL_DATA;
                aColumn->mMTLength   = 1;
                aColumn->mPrecision  = 0;
            }
            break;

        case ULN_MTYPE_SMALLINT :
            CM_ENDIAN_ASSIGN2(aColumn->mBuffer, aSrc);

            aColumn->mDataLength = 2;
            aColumn->mMTLength   = 2;
            aColumn->mPrecision  = 0;

            if (*((acp_sint16_t*)aColumn->mBuffer) == MTD_SMALLINT_NULL )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_INTEGER :
            CM_ENDIAN_ASSIGN4(aColumn->mBuffer, aSrc);

            aColumn->mDataLength = 4;
            aColumn->mMTLength   = 4;
            aColumn->mPrecision  = 0;

            if (*((acp_sint32_t*)aColumn->mBuffer) == MTD_INTEGER_NULL )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_BIGINT :
            CM_ENDIAN_ASSIGN8(aColumn->mBuffer, aSrc);

            aColumn->mDataLength = 8;
            aColumn->mMTLength   = 8;
            aColumn->mPrecision  = 0;

            if (*((acp_sint64_t*)aColumn->mBuffer) == MTD_BIGINT_NULL )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_REAL :
            CM_ENDIAN_ASSIGN4(aColumn->mBuffer, aSrc);

            aColumn->mDataLength = 4;
            aColumn->mMTLength   = 4;
            aColumn->mPrecision  = 0;

            if( ( *(acp_uint32_t*)(aColumn->mBuffer) & MTD_REAL_EXPONENT_MASK )
                == MTD_REAL_EXPONENT_MASK )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_DOUBLE :
            CM_ENDIAN_ASSIGN8(aColumn->mBuffer, aSrc);

            aColumn->mDataLength = 8;
            aColumn->mMTLength   = 8;
            aColumn->mPrecision  = 0;

            if( ( *(acp_uint64_t*)(aColumn->mBuffer) & MTD_DOUBLE_EXPONENT_MASK )
                == MTD_DOUBLE_EXPONENT_MASK )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_BLOB :
        case ULN_MTYPE_CLOB :
        case ULN_MTYPE_BLOB_LOCATOR :
        case ULN_MTYPE_CLOB_LOCATOR :

            sLob = (ulnLob *)aColumn->mBuffer;
            ulnLobInitialize(sLob, (ulnMTypeID)aColumn->mMtype);

            CM_ENDIAN_ASSIGN8(&sLobLocatorId, aSrc);
            /* 
             * PROJ-2047 Strengthening LOB - LOBCACHE
             *
             * LOB_LOCATOR(8) + lobsize(8) + HasData(1) + LobData(?)
             */
            CM_ENDIAN_ASSIGN8(&sLobSize, aSrc + 8);

            sLob->mOp->mSetLocator(aFnContext, sLob, sLobLocatorId);

            /* PROJ-2728 Sharding LOB */
            if ( sLobSize == ACP_ULONG_MAX )
            {
                sLob->mIsNull = ACP_TRUE;
                sLob->mSize = 0;
            }
            else
            {
                sLob->mIsNull = ACP_FALSE;;
                sLob->mSize = sLobSize;
            }

            aColumn->mDataLength = ACI_SIZEOF(ulnLob);
            aColumn->mMTLength   = LOB_MT_SIZE;
            aColumn->mPrecision  = 0;

            if (sLobLocatorId == MTD_LOCATOR_NULL )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            else
            {
                /* 
                 * PROJ-2047 Strengthening LOB - LOBCACHE
                 *
                 * HasData가 True이면 LOB Caching을 한다.
                 */
                if (aSrc[LOB_MT_HASDATA_OFFSET] == ACP_TRUE)
                {
                    sLob->mData = aSrc + LOB_MT_SIZE;
                    aColumn->mMTLength += sLob->mSize;
                }
                else
                {
                    sLob->mData = NULL;
                }

                /* BUG-36966 */
                ACI_TEST(ulnLobCacheAdd(sStmt->mLobCache,
                                        sLobLocatorId,
                                        sLob->mData,
                                        sLob->mSize,
                                        sLob->mIsNull)
                         != ACI_SUCCESS);
            }
            break;

        case ULN_MTYPE_TIMESTAMP :
            CM_ENDIAN_ASSIGN2(aColumn->mBuffer, aSrc);
            CM_ENDIAN_ASSIGN2(aColumn->mBuffer+2, aSrc+2);
            CM_ENDIAN_ASSIGN4(aColumn->mBuffer+4, aSrc+4);

            aColumn->mDataLength = 8;
            aColumn->mMTLength   = 8;
            aColumn->mPrecision  = 0;

            if( MTD_DATE_IS_NULL((mtdDateType*)aColumn->mBuffer) )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_INTERVAL :
            CM_ENDIAN_ASSIGN8(aColumn->mBuffer, aSrc);
            CM_ENDIAN_ASSIGN8(aColumn->mBuffer+8, aSrc+8);

            aColumn->mDataLength = 16;
            aColumn->mMTLength   = 16;
            aColumn->mPrecision  = 0;

            if( MTD_INTERVAL_IS_NULL((mtdIntervalType*)aColumn->mBuffer) )
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_BIT :
        case ULN_MTYPE_VARBIT :
            CM_ENDIAN_ASSIGN4(&sLen32, aSrc);

            aColumn->mBuffer     = (aSrc + 4);
            aColumn->mDataLength = BIT_TO_BYTE(sLen32);
            aColumn->mMTLength   = (aColumn->mDataLength) + 4;
            aColumn->mPrecision  = sLen32;

            if( aColumn->mDataLength == 0)
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        case ULN_MTYPE_NIBBLE :
            sLen8 = *(aSrc + 0);

            if( sLen8 == MTD_NIBBLE_NULL_LENGTH )
            {
                aColumn->mBuffer     = aSrc;
                aColumn->mDataLength = SQL_NULL_DATA;
                aColumn->mMTLength   = 1;
                aColumn->mPrecision  = 0;
            }
            else
            {
                aColumn->mBuffer     = (aSrc + 1);
                aColumn->mDataLength = (sLen8 + 1) >> 1;
                aColumn->mMTLength   = (aColumn->mDataLength) + 1;
                aColumn->mPrecision  = sLen8;
            }
            break;

        case ULN_MTYPE_BYTE :
        case ULN_MTYPE_VARBYTE :
            CM_ENDIAN_ASSIGN2(&sLen16, aSrc);

            aColumn->mBuffer     = (aSrc + 2);
            aColumn->mDataLength = sLen16;
            aColumn->mMTLength   = (aColumn->mDataLength) + 2;
            aColumn->mPrecision  = 0;

            if( sLen16 == 0)
            {
                aColumn->mDataLength = SQL_NULL_DATA;
            }
            break;

        default :
            ACE_ASSERT(0);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulnDataWriteStringToUserBuffer(ulnFnContext *aFnContext,
                                    acp_char_t   *aSourceString,
                                    acp_uint32_t  aSourceStringLength,
                                    acp_char_t   *aTargetBuffer,
                                    acp_uint32_t  aTargetBufferSize,
                                    acp_sint16_t *aSourceStringSizePtr)
{
    acp_uint32_t sLengthToCopy;

    if (aTargetBuffer == NULL)
    {
        return;
    }

    /*
     * 원래 길이를 리턴한다.
     */
    if (aSourceStringSizePtr != NULL)
    {
        *aSourceStringSizePtr = aSourceStringLength;
    }

    if (aTargetBufferSize >= ULN_SIZE_OF_NULLTERMINATION_CHARACTER)
    {
        if (aSourceStringLength >= aTargetBufferSize)
        {
            sLengthToCopy = aTargetBufferSize - ULN_SIZE_OF_NULLTERMINATION_CHARACTER;

            /*
             * 01004 : String Data, right truncated
             */
            ulnError(aFnContext, ulERR_IGNORE_RIGHT_TRUNCATED);
        }
        else
        {
            sLengthToCopy = aSourceStringLength;
        }

        /*
         * 이름을 복사하고, NULL Terminate 를 한다.
         */
        if (aSourceString == NULL)
        {
            *aTargetBuffer = ULN_NULL_TERMINATION_CHARACTER;
        }
        else
        {
            acpMemCpy(aTargetBuffer, aSourceString, sLengthToCopy);
            *(aTargetBuffer + sLengthToCopy) = ULN_NULL_TERMINATION_CHARACTER;
        }
    }
    else
    {
        /*
         * 01004 : String Data, right truncated
         */
        ulnError(aFnContext, ulERR_IGNORE_RIGHT_TRUNCATED);
    }
}

ACI_RC ulnDataGetNextColumnOffset(ulnColumn    *aColumn,
                                  acp_uint8_t  *aSrc,
                                  acp_uint32_t *aOffset)
{
    acp_uint8_t       sLen[8];
    acp_uint32_t      sOffset = *aOffset;

    switch(aColumn->mMtype)
    {
        case ULN_MTYPE_NULL :
            sOffset += 1;
            break;

        case ULN_MTYPE_BINARY :
            CM_ENDIAN_ASSIGN4(sLen, aSrc);

            sOffset += 8 + (*(acp_uint32_t*)(&sLen[0]));
            break;

        case ULN_MTYPE_CHAR :
        case ULN_MTYPE_VARCHAR :
        case ULN_MTYPE_NCHAR :
        case ULN_MTYPE_NVARCHAR :
            CM_ENDIAN_ASSIGN2(sLen, aSrc);

            sOffset += 2 + (*(acp_uint16_t*)(&sLen[0]));
            break;

        case ULN_MTYPE_FLOAT :
        case ULN_MTYPE_NUMERIC :

            sLen[0] = ((mtdNumericType*)aSrc)->length;
            sOffset += sLen[0] + 1;
            break;

        case ULN_MTYPE_SMALLINT :
            sOffset += 2;
            break;

        case ULN_MTYPE_INTEGER :
        case ULN_MTYPE_REAL :
            sOffset += 4;
            break;

        case ULN_MTYPE_BIGINT :
        case ULN_MTYPE_DOUBLE :
        case ULN_MTYPE_TIMESTAMP :
            sOffset += 8;
            break;

        case ULN_MTYPE_BLOB :
        case ULN_MTYPE_CLOB :
        case ULN_MTYPE_BLOB_LOCATOR :
        case ULN_MTYPE_CLOB_LOCATOR :
            /* 
             * PROJ-2047 Strengthening LOB - LOBCACHE
             *
             * sHasData가 True이면 Data 길이만큼 sOffset을 더해줘야 한다.
             */
            sOffset += LOB_MT_SIZE;

            if (aSrc[LOB_MT_HASDATA_OFFSET] == ACP_TRUE)
            {
                CM_ENDIAN_ASSIGN8(sLen, aSrc + 8);
                sOffset += *(acp_uint64_t *)sLen;
            }
            else
            {
                /* Nothing */
            }
            break;

        case ULN_MTYPE_INTERVAL :
            sOffset += 16;
            break;

        case ULN_MTYPE_BIT :
        case ULN_MTYPE_VARBIT :
            CM_ENDIAN_ASSIGN4(sLen, aSrc);

            sOffset += 4 + BIT_TO_BYTE(*(acp_uint32_t*)(&sLen[0]));
            break;

        case ULN_MTYPE_NIBBLE :
            sLen[0] = *(aSrc + 0);
            if( sLen[0] == MTD_NIBBLE_NULL_LENGTH )
            {
                sOffset += 1;
            }
            else
            {
                sOffset += 1 + ((sLen[0] + 1) >> 1);
            }
            break;

        case ULN_MTYPE_BYTE :
        case ULN_MTYPE_VARBYTE :
            CM_ENDIAN_ASSIGN2(sLen, aSrc);

            sOffset += 2 + (*(acp_uint16_t*)(&sLen[0]));
            break;

        default :
            ACI_TEST(ACP_TRUE);
            break;
    }

    *aOffset = sOffset;
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

