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

#ifndef _O_ULN_BIND_COMMON_H_
#define _O_ULN_BIND_COMMON_H_ 1

#include <ulnDesc.h>
#include <ulnDescRec.h>

typedef struct ulnAppBuffer
{
    ulnCTypeID      mCTYPE;
    acp_uint8_t    *mBuffer;
    ulvULen         mBufferSize;
    acp_uint32_t   *mFileOptionPtr;
    acp_uint32_t    mColumnStatus;
} ulnAppBuffer;

struct ulnIndLenPtrPair
{
    ulvSLen *mIndicatorPtr;
    ulvSLen *mLengthPtr;
};

/*
 * 사용자가 바인드를 새로 하거나 BINDINFO GET RES 를 받아서 IRD 를 생성해야 할 필요가 있을 경우
 * 새로운(혹은 재사용할) Descriptor Record 를 지정해주는 함수
 */
ACI_RC ulnBindArrangeNewDescRec(ulnDesc       *aDesc,
                                acp_uint16_t   aIndex,
                                ulnDescRec   **aOutputDescRec);

/*
 * 사용자가 바인드한 어드레스를 계산하는 함수
 */
ACP_INLINE ACI_RC ulnBindGetBindOffsetAndElementSize(ulnDescRec   *aAppDescRec,
                                                        ulvULen      *aBindOffset,
                                                        acp_uint32_t *aElementSize)
{
    // fix BUG-20745 BIND_OFFSET_PTR 지원
    if( aAppDescRec->mParentDesc->mHeader.mBindOffsetPtr != NULL )
    {
        *aBindOffset = *(aAppDescRec->mParentDesc->mHeader.mBindOffsetPtr);
    }
    else
    {
        *aBindOffset = 0;
    }

    /*
     * Note : 이 규칙은 desc 가 APD 든, ARD 든 상관없이 적용된다.
     *        즉, SQL_ATTR_PARAM_BIND_TYPE 이건, SQL_ATTR_ROW_BIND_TYPE 이건 상관없다는 것이다.
     */
    if (aAppDescRec->mParentDesc->mHeader.mBindType != SQL_PARAM_BIND_BY_COLUMN)
    {
        /*
         * row wise binding 일 때에는 구조체의 크기가 element size.
         */
        *aElementSize = aAppDescRec->mParentDesc->mHeader.mBindType;
    }

    return ACI_SUCCESS;
}

/*
 * 사용자 버퍼의 주소를 계산하는 함수.
 *
 *      ULN_CALC_USER_BUF_ADDR()
 *      ulnBindCalcUserOctetLengthAddr()
 *      ulnBindCalcUserFileOptionAddr()
 *      ulnBindCalcUserIndLenPair()
 *
 * 함수들에서 공통으로 쓰이는 함수이다.
 *
 * BUGBUG : Execute 시에도 만약 사용자가 indicator ptr 와 strlen ptr 을 분리했을 경우
 *          각각을 모두 검사해서 SQL_NTS, SQL_NULL_DATA, length 를 판단해서
 *          계산해야 한다.
 *
 *          그러나 지금은 구현해 놓지 않았다.
 *
 *          나중에 ulnBindCalcUserOctetLengthAddr() 를 호출하는 부분들을
 *          모두 찾아서 고쳐 주어야 한다.
 *          현재 Fetch() 시에 분리된 indicator, strlen 버퍼를 고려해서 길이를 리턴하는 
 *          것은 구현이 되어 있다.
 */
#define ULN_CALC_USER_BUF_ADDR(aBoundAddr, aRowNumber, aBindOffset, aElementSize) \
    ((void *)((aBoundAddr) + (aBindOffset) + ((aRowNumber) * (aElementSize))))

ACP_INLINE acp_uint32_t *ulnBindCalcUserFileOptionAddr(ulnDescRec *aAppDescRec, acp_uint32_t aRowNumber)
{
    ulvULen      sBindOffset;
    acp_uint32_t sElementSize;

    /*
     * aRowNumber 는 0 베이스 인덱스
     */

    if (aAppDescRec->mFileOptionsPtr == NULL)
    {
        return NULL;
    }

    /*
     * 기본 column wise binding 일 때에는 사용자가 바인드한 데이터 타입에 따른 크기.
     * 가변길이를 바인드했을 때에도 마찬가지이다. 사용자가 준비해 두었다고 통보한 버퍼의 크기.
     */
    sElementSize = ACI_SIZEOF(acp_uint32_t);
    sBindOffset  = 0;

    /*
     * Note : sBindOffset 과 sElementSize 는 바꿀 필요가 있을 때,
     *        즉, row wise binding 시에만 바꾼다.
     */
    ulnBindGetBindOffsetAndElementSize(aAppDescRec, &sBindOffset, &sElementSize);

    return (acp_uint32_t *)ULN_CALC_USER_BUF_ADDR((acp_uint8_t *)(aAppDescRec->mFileOptionsPtr),
                                                  aRowNumber,
                                                  sBindOffset,
                                                  sElementSize);
}

ACP_INLINE void *ulnBindCalcUserDataAddr(ulnDescRec *aAppDescRec, acp_uint32_t aRowNumber)
{
    ulvULen      sBindOffset;
    acp_uint32_t sElementSize;

    /*
     * aRowNumber 는 0 베이스 인덱스
     */

    if (aAppDescRec->mDataPtr == NULL)
    {
        return NULL;
    }

    /*
     * 기본 column wise binding 일 때에는 사용자가 바인드한 데이터 타입에 따른 크기.
     * 가변길이를 바인드했을 때에도 마찬가지이다. 사용자가 준비해 두었다고 통보한 버퍼의 크기.
     *
     * Note : 64bit 값을 octet length 로 쓴다고 해도 설마 기가 단위일까 -_-;; 그냥 acp_uint32_t 로 캐스팅
     *        해서 써도 되리라 생각함.
     */
    sElementSize = (acp_uint32_t)(aAppDescRec->mMeta.mOctetLength);
    sBindOffset  = 0;

    /*
     * Note : sBindOffset 과 sElementSize 는 바꿀 필요가 있을 때,
     *        즉, row wise binding 시에만 바꾼다.
     */
    ulnBindGetBindOffsetAndElementSize(aAppDescRec, &sBindOffset, &sElementSize);

    return (void *)ULN_CALC_USER_BUF_ADDR((acp_uint8_t *)(aAppDescRec->mDataPtr),
                                          aRowNumber,
                                          sBindOffset,
                                          sElementSize);
}

ACP_INLINE ulvSLen *ulnBindCalcUserOctetLengthAddr(ulnDescRec *aAppDescRec, acp_uint32_t aRowNumber)
{
    ulvULen       sBindOffset;
    acp_uint32_t  sElementSize;
    ulvSLen      *sOctetLengthPtr;

    /* BUG-36518 */
    if (aAppDescRec == NULL)
    {
        return NULL;
    }

    sOctetLengthPtr = aAppDescRec->mOctetLengthPtr;

    if (sOctetLengthPtr == NULL)
    {
        return NULL;
    }

    /*
     * 기본 column wise binding 일 때에 len or ind 의 사이즈
     */
    sElementSize = ACI_SIZEOF(ulvSLen);
    sBindOffset  = 0;

    /*
     * Note : sBindOffset 과 sElementSize 는 바꿀 필요가 있을 때,
     *        즉, row wise binding 시에만 바꾼다.
     */
    ulnBindGetBindOffsetAndElementSize(aAppDescRec, &sBindOffset, &sElementSize);

    return (ulvSLen *)ULN_CALC_USER_BUF_ADDR((acp_uint8_t *)sOctetLengthPtr,
                                             aRowNumber,
                                             sBindOffset,
                                             sElementSize);
}

/*
 * 편의상 aLengthRetrievedFromServer 에 0 이 들어와도 NULL_DATA 로 취급하자.
 */
ACP_INLINE ACI_RC ulnBindSetUserIndLenValue(ulnIndLenPtrPair *aIndLenPair, ulvSLen aLengthRetrievedFromServer)
{
    if (aLengthRetrievedFromServer == ULN_vLEN(0) ||
        aLengthRetrievedFromServer == SQL_NULL_DATA)
    {
        /*
         * NULL DATA 가 수신되었을 때
         */
        if (aIndLenPair->mLengthPtr != NULL)
        {
            if (aIndLenPair->mLengthPtr != aIndLenPair->mIndicatorPtr)
            {
                *aIndLenPair->mLengthPtr = ULN_vLEN(0);
            }
        }

        ACI_TEST_RAISE(aIndLenPair->mIndicatorPtr == NULL, LABEL_ERR_22002);

        *aIndLenPair->mIndicatorPtr = SQL_NULL_DATA;
    }
    else
    {
        if (aIndLenPair->mLengthPtr != NULL)
        {
            *aIndLenPair->mLengthPtr = aLengthRetrievedFromServer;
        }

        if (aIndLenPair->mIndicatorPtr != NULL)
        {
            if (aIndLenPair->mLengthPtr != aIndLenPair->mIndicatorPtr)
            {
                *aIndLenPair->mIndicatorPtr = ULN_vLEN(0);
            }
        }
    }

    return ACI_SUCCESS;

    /*
     * 22002 : Indicator Variable Required but not supplied and
     *         the data retrieved was NULL.
     *
     *         이 함수를 호출한 곳에서 적절히 알아서 발생시켜 주면 된다.
     *
     * Note : Fetch 시에만 발생시키고, outbinding parameter 의 경우에는 조용히
     *        무시하고, 에러 발생시키지 않음에 주의!
     */
    ACI_EXCEPTION(LABEL_ERR_22002);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ulvSLen ulnBindGetUserIndLenValue(ulnIndLenPtrPair *aIndLenPair);

ACP_INLINE void ulnBindCalcUserIndLenPair(ulnDescRec       *aAppDescRec,
                                             acp_uint32_t      aRowNumber,
                                             ulnIndLenPtrPair *aUserIndLenPair)
{
    ulvSLen      *sOctetLengthPtr = aAppDescRec->mOctetLengthPtr;
    ulvSLen      *sIndicatorPtr   = aAppDescRec->mIndicatorPtr;
    ulvULen       sBindOffset     = 0;
    acp_uint32_t  sElementSize    = 0;

    if (sOctetLengthPtr != NULL || sIndicatorPtr != NULL)
    {
        /*
         * 기본 column wise binding 일 때에 len or ind 의 사이즈
         */
        sElementSize = ACI_SIZEOF(ulvSLen);
        sBindOffset  = 0;

        ulnBindGetBindOffsetAndElementSize(aAppDescRec, &sBindOffset, &sElementSize);
    }

    if (sOctetLengthPtr == NULL)
    {
        aUserIndLenPair->mLengthPtr = NULL;
    }
    else
    {
        aUserIndLenPair->mLengthPtr = (ulvSLen *)ULN_CALC_USER_BUF_ADDR((acp_uint8_t *)sOctetLengthPtr,
                                                                        aRowNumber,
                                                                        sBindOffset,
                                                                        sElementSize);
    }

    if (sIndicatorPtr == NULL)
    {
        aUserIndLenPair->mIndicatorPtr = NULL;
    }
    else
    {
        aUserIndLenPair->mIndicatorPtr = (ulvSLen *)ULN_CALC_USER_BUF_ADDR((acp_uint8_t *)sIndicatorPtr,
                                                                           aRowNumber,
                                                                           sBindOffset,
                                                                           sElementSize);
    }
}

/*
 * 사용자가 BindParameter 시에 제공한 BufferSize, StrLenOrIndPtr 에 있는값을 가지고서
 * 사용자가 실제 전송하고자 하는 데이터의 사이즈를 구한다.
 */
ACP_INLINE ACI_RC ulnBindCalcUserDataLen(ulnCTypeID    aCType,
                                            ulvSLen       aUserBufferSize,
                                            acp_uint8_t  *aUserDataPtr,
                                            ulvSLen       aUserStrLenOrInd,
                                            acp_sint32_t *aUserDataLength,
                                            acp_bool_t   *aIsNullData)
{
    ulvSLen       sUserDataLength = 0;
    acp_bool_t    sIsNullData     = ACP_FALSE;
    acp_uint16_t *sWCharData      = NULL;
    ulvSLen       sWCharMaxLen    = 0;

    /*
     * Note : 이 함수를 호출할 때 aUserDataPtr 에 저장된 데이터의 타입이 CHAR or BINARY
     *        인 것을 확인해야 한다.
     */
    if (aUserDataPtr == NULL)
    {
        /*
         * SQLBindParameter() 에서 ParamValuePtr 이 NULL 이면, NULL DATA 이다.
         */
        aUserStrLenOrInd = SQL_NULL_DATA;
    }

    switch(aUserStrLenOrInd)
    {
        case SQL_NTS:
            if (aCType == ULN_CTYPE_CHAR)
            {
                if (aUserBufferSize > ULN_vLEN(0))
                {
                    sUserDataLength = acpCStrLen((acp_char_t *)aUserDataPtr, aUserBufferSize);
                }
                else
                {
                    sUserDataLength = acpCStrLen((acp_char_t *)aUserDataPtr, ACP_SINT32_MAX);
                }
            }
            // BUG-25937 SQL_WCHAR로 CLOB을 넣으면 제대로 안들어갑니다.
            // WCHAR 데이타의 길이를 정확히 구한다.
            // 길이를 잘못구하면 서버에 잘못된 값들이 들어간다.
            else if(aCType == ULN_CTYPE_WCHAR)
            {

                /* bug-36698: wrong length when SQL_C_WCHAR and 0 buflen.
                   wchar 타입도 위의 char 타입처럼, buflen가 0인경우에
                   대해서 처리해 주도록 한다.
                   cf) Power Builder 에서 parameter가 wchar 타입으로 들어옴 */
                if (aUserBufferSize > ULN_vLEN(0))
                {
                    sWCharMaxLen = aUserBufferSize;
                }
                else
                {
                    sWCharMaxLen = (ulvSLen)ACP_SINT32_MAX;
                }

                for(sWCharData = (acp_uint16_t*)aUserDataPtr, sUserDataLength = 0;
                    (sUserDataLength < sWCharMaxLen) && (*sWCharData != 0);
                    sUserDataLength = sUserDataLength +2)
                {
                    ++sWCharData;
                }
            }
            else
            {
                /*
                 * aCType 은 ULN_CTYPE_BINARY, ULN_CTYPE_BYTE, ULN_CTYPE_BIT, ULN_CTYPE_NIBBLE
                 */
                sUserDataLength = aUserBufferSize;
            }
            break;

        case SQL_NULL_DATA:
            sUserDataLength = ULN_vLEN(0);
            sIsNullData     = ACP_TRUE;
            break;

        case SQL_DEFAULT_PARAM:
        case SQL_DATA_AT_EXEC:
            /*
             * BUGBUG : 아직 구현 안함
             */
            ACI_TEST(1);
            break;

        default:
            if (aUserStrLenOrInd <= SQL_LEN_DATA_AT_EXEC(0))
            {
                /*
                 * SQL_LEN_DATA_AT_EXEC() 매크로에 의한 값
                 */
                ACI_TEST(1);
            }
            else
            {
                if (aUserBufferSize > ULN_vLEN(0))
                {
                    sUserDataLength = ACP_MIN(aUserBufferSize, aUserStrLenOrInd);
                }
                else
                {
                    sUserDataLength = aUserStrLenOrInd;
                }
            }
            break;
    }

    *aUserDataLength = (acp_sint32_t)sUserDataLength;

    if (aIsNullData != NULL)
    {
        *aIsNullData = sIsNullData;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_bool_t ulnBindIsFixedLengthColumn(ulnDescRec *aAppDescRec, ulnDescRec *aImpDescRec);

acp_uint16_t ulnBindCalcNumberOfParamsToSend(ulnStmt *aStmt);

ACI_RC ulnBindAdjustStmtInfo(ulnStmt *aStmt);

#endif /* _O_ULN_BIND_COMMON_H_ */
