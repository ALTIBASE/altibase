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
#include <ulxXaProtocol.h>
#include <ulnError.h>

ACI_RC ulxConnectionProtocol(ulnDbc       *aDbc,
                             acp_sint32_t  aOp,
                             acp_sint32_t  aRmid,
                             acp_slong_t   aFlag,
                             acp_sint32_t *aResult)
{
    /* for xa_open, xa_close */
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx = &(aDbc->mPtContext.mCmiPtContext);
    ulnFnContext         sFnContext;
    acp_uint64_t         sArgument = 0;
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_XA, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST( ulnInitializeProtocolContext( &sFnContext,
                                            &(aDbc->mPtContext),
                                            &(aDbc->mSession))
        != ACI_SUCCESS);

    sPacket.mOpID = CMP_OP_DB_XaOperation;

    CMI_WRITE_CHECK(sCtx, 22);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_XaOperation);
    CMI_WR1(sCtx, aOp);
    CMI_WR4(sCtx, (acp_uint32_t*)&aRmid);
    CMI_WR8(sCtx, (acp_uint64_t*)&aFlag);
    CMI_WR8(sCtx, &sArgument);

    ACI_TEST(ulnWriteProtocol(&sFnContext, &(aDbc->mPtContext), &sPacket)
             != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(&sFnContext,  &(aDbc->mPtContext))
             != ACI_SUCCESS);

    ACI_TEST_RAISE(ulnReadProtocol(&sFnContext,
                             &(aDbc->mPtContext),
                             aDbc->mConnTimeoutValue) != ACI_SUCCESS, read_err);

    *aResult = sFnContext.mXaResult;

    return ACI_SUCCESS;

    ACI_EXCEPTION(read_err);
    {
        *aResult = sFnContext.mXaResult;
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(&sFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulxTransactionProtocol(ulnDbc       *aDbc,
                              acp_sint32_t  aOp,
                              acp_sint32_t  aRmid,
                              acp_slong_t   aFlag,
                              XID          *aXid,
                              acp_sint32_t *aResult)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx = &(aDbc->mPtContext.mCmiPtContext);
    ulnFnContext         sFnContext;
    acp_uint64_t         sArgument = 0;
    acp_sint64_t         sFormatID;
    acp_sint64_t         sGTRIDLength;
    acp_sint64_t         sBQUALLength;
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState = 0;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_XA, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST(ulnInitializeProtocolContext( &sFnContext,
                                           &(aDbc->mPtContext),
                                           &(aDbc->mSession))
             != ACI_SUCCESS);

    sPacket.mOpID = CMP_OP_DB_XaTransaction;

    CMI_WRITE_CHECK(sCtx, 46 + XIDDATASIZE);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_XaTransaction);
    CMI_WR1(sCtx, aOp);
    CMI_WR4(sCtx, (acp_uint32_t*)&aRmid);
    CMI_WR8(sCtx, (acp_uint64_t*)&aFlag);
    CMI_WR8(sCtx, &sArgument);

    // XID 구조멤버가 가변길이 long형이어서 고정길이 버퍼가 필요
    sFormatID    = aXid->formatID;
    sGTRIDLength = aXid->gtrid_length;
    sBQUALLength = aXid->bqual_length;

    CMI_WR8(sCtx, (acp_uint64_t*)&(sFormatID));
    CMI_WR8(sCtx, (acp_uint64_t*)&(sGTRIDLength));
    CMI_WR8(sCtx, (acp_uint64_t*)&(sBQUALLength));
    CMI_WCP(sCtx, aXid->data, XIDDATASIZE);

    ACI_TEST(ulnWriteProtocol(&sFnContext, &(aDbc->mPtContext), &sPacket)
             != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(&sFnContext, &(aDbc->mPtContext)) != ACI_SUCCESS);

    ACI_TEST_RAISE(ulnReadProtocol(&sFnContext,
                             &(aDbc->mPtContext),
                             aDbc->mConnTimeoutValue) != ACI_SUCCESS, read_err);

    *aResult = sFnContext.mXaResult;

    return ACI_SUCCESS;

    ACI_EXCEPTION(read_err);
    {
        /* bug-36037: invalid xid
           invalid xid인 경우 xa result를 받지 않고
           error result를 수신하여 mXaResult가 의미 없음 */
        if (sFnContext.mXaResult != XA_OK)
        {
            *aResult = sFnContext.mXaResult;
        }
        else
        {
            *aResult = XAER_RMERR;
        }
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(&sFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulxRecoverProtocol(ulnDbc        *aDbc,
                          acp_sint32_t   aOp,
                          acp_sint32_t   aRmid,
                          acp_slong_t    aFlag,
                          XID          **aXid,
                          acp_sint32_t   aCount,
                          acp_sint32_t  *aResult)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx = &(aDbc->mPtContext.mCmiPtContext);
    ulnFnContext         sFnContext;
    acp_uint64_t         sArgument = aCount;
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_XA, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST(ulnInitializeProtocolContext( &sFnContext,
                                           &(aDbc->mPtContext),
                                           &(aDbc->mSession))
             != ACI_SUCCESS);

    /* set recover position */
    sFnContext.mXaRecoverXid = NULL;
    sFnContext.mXaXidPos     = 0;

    sPacket.mOpID = CMP_OP_DB_XaOperation;

    CMI_WRITE_CHECK(sCtx, 22);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_XaOperation);
    CMI_WR1(sCtx, aOp);
    CMI_WR4(sCtx, (acp_uint32_t*)&aRmid);
    CMI_WR8(sCtx, (acp_uint64_t*)&aFlag);
    CMI_WR8(sCtx, &sArgument);

    /*
     * 프로토콜 쓰기
     */
    ACI_TEST(ulnWriteProtocol(&sFnContext, &(aDbc->mPtContext), &sPacket)
             != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(&sFnContext, &(aDbc->mPtContext)) != ACI_SUCCESS);
    *aResult = sFnContext.mXaResult;

    ACI_TEST(ulnReadProtocol(&sFnContext,
                             &(aDbc->mPtContext),
                             aDbc->mConnTimeoutValue) != ACI_SUCCESS);

    *aResult = sFnContext.mXaResult;
    *aXid    = (XID*)sFnContext.mXaRecoverXid;

    sFnContext.mXaRecoverXid = NULL;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(&sFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulxCallbackXaGetResult(cmiProtocolContext *aCtx,
                              cmiProtocol        *aProtocol,
                              void               *aServiceSession,
                              void               *aUserContext)
{
    ulnFnContext     *sFnContext;

    acp_uint8_t       sOperation;
    acp_uint32_t      sReturnValue;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD1(aCtx, sOperation);
    CMI_RD4(aCtx, &sReturnValue);

    sFnContext  = (ulnFnContext *)aUserContext;
    sFnContext->mXaResult = sReturnValue;

    if ( (sOperation == CMP_DB_XA_RECOVER) &&
         (sFnContext->mXaResult > 0) )
    {
        ACI_TEST( acpMemAlloc((void**)&sFnContext->mXaRecoverXid, 
                              ACI_SIZEOF(XID) * sFnContext->mXaResult)
                  != ACP_RC_SUCCESS );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulxCallbackXaXid(cmiProtocolContext *aCtx,
                        cmiProtocol        *aProtocol,
                        void               *aServiceSession,
                        void               *aUserContext)
{
    ulnFnContext  *sFnContext;
    acp_sint32_t   sXidPos;
    XID           *sXid;
    acp_sint64_t   sFormatID;
    acp_sint64_t   sGTRIDLength;
    acp_sint64_t   sBQUALLength;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    sFnContext  = (ulnFnContext *)aUserContext;
    sXidPos     = sFnContext->mXaXidPos;
    sXid        = (XID*)(sFnContext->mXaRecoverXid);

    ACI_TEST_RAISE(sXid == NULL, LABEL_INVALID_PROTOCOL);

    // XID 구조멤버가 가변길이 long형이어서 고정길이 버퍼가 필요
    CMI_RD8(aCtx, (acp_uint64_t*)&(sFormatID));
    CMI_RD8(aCtx, (acp_uint64_t*)&(sGTRIDLength));
    CMI_RD8(aCtx, (acp_uint64_t*)&(sBQUALLength));

    sXid[sXidPos].formatID     = sFormatID;
    sXid[sXidPos].gtrid_length = sGTRIDLength;
    sXid[sXidPos].bqual_length = sBQUALLength;

    CMI_RCP(aCtx, sXid[sXidPos].data, XIDDATASIZE);

    sFnContext->mXaXidPos++;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_PROTOCOL);
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}
