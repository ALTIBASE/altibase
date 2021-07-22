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
#include <ulnPDContext.h>

/*
 * =========================
 * Prepare Buffer
 * =========================
 */

static ACI_RC ulnPDContextPrepareBufferUSER(ulnPDContext *aPDContext, void *aArgument)
{
    aPDContext->mBuffer = (acp_uint8_t *)aArgument;

    return ACI_SUCCESS;
}

static ACI_RC ulnPDContextPrepareBufferALLOC(ulnPDContext *aPDContext, void *aArgument)
{
    ACP_UNUSED(aArgument);

    switch (aPDContext->mBufferType)
    {
        case ULN_PD_BUFFER_TYPE_MAX:
        case ULN_PD_BUFFER_TYPE_UNDETERMINED:
            ACE_ASSERT(0);
            break;

        case ULN_PD_BUFFER_TYPE_ALLOC:
            if (aPDContext->mBuffer != NULL)
            {
                acpMemFree(aPDContext->mBuffer);
            }

        case ULN_PD_BUFFER_TYPE_USER:
            aPDContext->mBuffer = NULL;
            ACI_TEST(acpMemAlloc((void**)&aPDContext->mBuffer, ULN_PD_BUFFER_SIZE)
                     != ACP_RC_SUCCESS);

            break;
    }

    aPDContext->mBufferSize = ULN_PD_BUFFER_SIZE;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * =========================
 * Finalize Buffer
 * =========================
 */

static void ulnPDContextFinalizeUSER(ulnPDContext *aPDContext)
{
    // idlOS::printf("#### <<<<<< finalize user buffer\n");

    aPDContext->mState      = ULN_PD_ST_CREATED;
    aPDContext->mBuffer     = NULL;
    aPDContext->mBufferSize = 0;

    aPDContext->mIndicator  = 0;
    aPDContext->mDataLength = 0;

    aPDContext->mBufferType = ULN_PD_BUFFER_TYPE_UNDETERMINED;

    aPDContext->mOp         = NULL;
}

static void ulnPDContextFinalizeALLOC(ulnPDContext *aPDContext)
{
    // idlOS::printf("#### <<<<<< finalize alloced buffer\n");

    aPDContext->mState      = ULN_PD_ST_CREATED;

    if (aPDContext->mBuffer != NULL)
    {
        acpMemFree(aPDContext->mBuffer);
    }

    aPDContext->mBuffer     = NULL;
    aPDContext->mBufferSize = 0;

    aPDContext->mBufferType = ULN_PD_BUFFER_TYPE_UNDETERMINED;

    aPDContext->mOp         = NULL;
}

/*
 * =========================
 * Accumulate Data
 * =========================
 */

static ACI_RC ulnPDContextAccumulateDataALLOC(ulnPDContext *aPDContext,
                                              acp_uint8_t  *aBuffer,
                                              acp_uint32_t  aLength)
{
    if (aLength > 0)
    {
        /*
         * 쌓을려는 데이터가 버퍼 크기 초과
         */
        ACI_TEST(aPDContext->mDataLength + aLength > aPDContext->mBufferSize);

        acpMemCpy(aPDContext->mBuffer + aPDContext->mDataLength, aBuffer, aLength);

        aPDContext->mDataLength += aLength;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnPDContextAccumulateDataUSER(ulnPDContext *aPDContext,
                                             acp_uint8_t  *aBuffer,
                                             acp_uint32_t  aLength)
{
    aPDContext->mBuffer     = aBuffer;
    aPDContext->mDataLength = aLength;

    return ACI_SUCCESS;
}

/*
 * =========================
 * Set / Get State
 * =========================
 */

void ulnPDContextSetState(ulnPDContext *aPDContext, ulnPDState aState)
{
    aPDContext->mState = aState;
}

ulnPDState ulnPDContextGetState(ulnPDContext *aPDContext)
{
    return aPDContext->mState;
}

static const ulnPDOpSet gUlnPDContextOpSet[ULN_PD_BUFFER_TYPE_MAX] =
{
    {
        NULL,
        NULL,
        NULL
    },

    {
        ulnPDContextPrepareBufferUSER,
        ulnPDContextFinalizeUSER,
        ulnPDContextAccumulateDataUSER
    },

    {
        ulnPDContextPrepareBufferALLOC,
        ulnPDContextFinalizeALLOC,
        ulnPDContextAccumulateDataALLOC
    }
};

void ulnPDContextCreate(ulnPDContext *aPDContext)
{
    /*
     * ulnDescRec 가 생성될 때 호출됨.
     * 함수 명과는 다르게, 실제 Create 가 아니라, 인스턴스를 초기화하기만 함.
     * ulnDescRec 에 ulnPDContext 가 static 하게 들어가 있어서 그러함.
     */

    acpListInitObj(&aPDContext->mList, aPDContext);

    aPDContext->mOp         = NULL;

    aPDContext->mBufferType = ULN_PD_BUFFER_TYPE_UNDETERMINED;

    aPDContext->mBuffer     = NULL;
    aPDContext->mBufferSize = 0;

    aPDContext->mDataLength = 0;
    aPDContext->mIndicator  = 0;
    aPDContext->mState      = ULN_PD_ST_CREATED;
}

void ulnPDContextInitialize(ulnPDContext *aPDContext, ulnPDBufferType aBufferType)
{
    /*
     * 진짜 초기화하는 함수.
     */

    acpListInitObj(&aPDContext->mList, aPDContext);

    aPDContext->mBuffer     = NULL;
    aPDContext->mBufferSize = 0;

    aPDContext->mDataLength = 0;
    aPDContext->mIndicator  = 0;
    aPDContext->mState      = ULN_PD_ST_INITIAL;

    aPDContext->mBufferType = aBufferType;

    aPDContext->mOp         = (ulnPDOpSet *)&gUlnPDContextOpSet[aBufferType];
}

acp_uint32_t ulnPDContextGetDataLength(ulnPDContext *aPDContext)
{
    return aPDContext->mDataLength;
}

acp_uint32_t ulnPDContextGetBufferSize(ulnPDContext *aPDContext)
{
    return aPDContext->mBufferSize;
}

acp_uint8_t *ulnPDContextGetBuffer(ulnPDContext *aPDContext)
{
    return aPDContext->mBuffer;
}
