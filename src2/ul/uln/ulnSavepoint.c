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
#include <ulsdnExecute.h>

static ACI_RC ulnSetSavepointMain( ulnFnContext     * aFnContext,
                                   ulnDbc           * aDbc,
                                   const acp_char_t * aSavepointName,
                                   acp_uint32_t       aSavepointNameLength )
{
    ULN_FLAG(sNeedFinPtContext);

    cmiProtocolContext *sCtx      = NULL;

    ACI_TEST_RAISE( aDbc->mIsConnected == ACP_FALSE, LABEL_ABORT_NO_CONNECTION );

    ACI_TEST( ulnInitializeProtocolContext( aFnContext,
                                            &(aDbc->mPtContext),
                                            &(aDbc->mSession) )
              != ACI_SUCCESS );
    ULN_FLAG_UP(sNeedFinPtContext);

    ACI_TEST( ulnWriteSetSavepointREQ( aFnContext,
                                       &(aDbc->mPtContext),
                                       aSavepointName,
                                       aSavepointNameLength )
              != ACI_SUCCESS );

    ACI_TEST( ulnFlushProtocol( aFnContext, &(aDbc->mPtContext) ) != ACI_SUCCESS );

    ulsdDbcCallback( aDbc );

    sCtx = &aDbc->mPtContext.mCmiPtContext;
    if ( cmiGetLinkImpl( sCtx ) == CMI_LINK_IMPL_IPCDA )
    {
        ACI_TEST( ulnReadProtocolIPCDA( aFnContext,
                                        &(aDbc->mPtContext),
                                        aDbc->mConnTimeoutValue ) 
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST( ulnReadProtocol( aFnContext,
                                   &(aDbc->mPtContext),
                                   aDbc->mConnTimeoutValue ) 
                  != ACI_SUCCESS );
    }

    ULN_FLAG_DOWN(sNeedFinPtContext);
    ACI_TEST( ulnFinalizeProtocolContext( aFnContext, &(aDbc->mPtContext) ) != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION( LABEL_ABORT_NO_CONNECTION )
    {
        ulnError( aFnContext, ulERR_ABORT_NO_CONNECTION, "" );
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP( sNeedFinPtContext )
    {
        ulnFinalizeProtocolContext( aFnContext,&(aDbc->mPtContext) );
    }

    return ACI_FAILURE;
}

static ACI_RC ulnRollbackToSavepointMain( ulnFnContext     * aFnContext,
                                          ulnDbc           * aDbc,
                                          const acp_char_t * aSavepointName,
                                          acp_uint32_t       aSavepointNameLength )
{
    ULN_FLAG(sNeedFinPtContext);

    cmiProtocolContext *sCtx      = NULL;

    ACI_TEST_RAISE( aDbc->mIsConnected == ACP_FALSE, LABEL_ABORT_NO_CONNECTION );

    ACI_TEST( ulnInitializeProtocolContext(aFnContext,
                                           &(aDbc->mPtContext),
                                           &(aDbc->mSession) )
              != ACI_SUCCESS );
    ULN_FLAG_UP(sNeedFinPtContext);

    ACI_TEST( ulnWriteRollbackToSavepointREQ( aFnContext,
                                              &(aDbc->mPtContext),
                                              aSavepointName,
                                              aSavepointNameLength )
              != ACI_SUCCESS );

    ACI_TEST( ulnFlushProtocol( aFnContext, &(aDbc->mPtContext) ) != ACI_SUCCESS );

    ulsdDbcCallback( aDbc );

    sCtx = &aDbc->mPtContext.mCmiPtContext;
    if ( cmiGetLinkImpl( sCtx ) == CMI_LINK_IMPL_IPCDA )
    {
        ACI_TEST( ulnReadProtocolIPCDA( aFnContext,
                                        &(aDbc->mPtContext),
                                        aDbc->mConnTimeoutValue ) 
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST( ulnReadProtocol( aFnContext,
                                   &(aDbc->mPtContext),
                                   aDbc->mConnTimeoutValue ) 
                  != ACI_SUCCESS );
    }

    ULN_FLAG_DOWN(sNeedFinPtContext);
    ACI_TEST( ulnFinalizeProtocolContext( aFnContext, &(aDbc->mPtContext) ) != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION( LABEL_ABORT_NO_CONNECTION )
    {
        ulnError( aFnContext, ulERR_ABORT_NO_CONNECTION, "" );
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        ulnFinalizeProtocolContext( aFnContext,&(aDbc->mPtContext) );
    }

    return ACI_FAILURE;
}

SQLRETURN ulnSetSavepoint( ulnDbc               * aDbc, 
                           const acp_char_t     * aSavepointName,
                           acp_uint32_t           aSavepointNameLength )
{
    ULN_FLAG(sNeedExit);
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_FOR_SD, aDbc, ULN_OBJ_TYPE_DBC );

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST( ulnSetSavepointMain( &sFnContext,
                                   aDbc, 
                                   aSavepointName, 
                                   aSavepointNameLength ) 
              != ACI_SUCCESS );

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG( NULL, ULN_TRACELOG_MID, NULL, 0,
                   "%-18s| ulnSetSavepoint" );

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

ACI_RC ulnCallbackSetSavepointResult( cmiProtocolContext * aProtocolContext,
                                      cmiProtocol        * aProtocol,
                                      void               * aServiceSession,
                                      void               * aUserContext )
{
    ACP_UNUSED( aProtocolContext );
    ACP_UNUSED( aProtocol );
    ACP_UNUSED( aServiceSession );
    ACP_UNUSED( aUserContext );

    return ACI_SUCCESS;
}

SQLRETURN ulnRollbackToSavepoint( ulnDbc             * aDbc, 
                                  const acp_char_t   * aSavepointName,
                                  acp_uint32_t         aSavepointNameLength )
{
    ULN_FLAG(sNeedExit);
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_FOR_SD, aDbc, ULN_OBJ_TYPE_DBC );

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST( ulnRollbackToSavepointMain( &sFnContext,
                                          aDbc, 
                                          aSavepointName, 
                                          aSavepointNameLength ) 
              != ACI_SUCCESS );

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG( NULL, ULN_TRACELOG_MID, NULL, 0,
                   "%-18s| ulnRollbackToSavepoint" );

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

ACI_RC ulnCallbackRollbackToSavepointResult( cmiProtocolContext * aProtocolContext,
                                             cmiProtocol        * aProtocol,
                                             void               * aServiceSession,
                                             void               * aUserContext )
{
    ACP_UNUSED( aProtocolContext );
    ACP_UNUSED( aProtocol );
    ACP_UNUSED( aServiceSession );
    ACP_UNUSED( aUserContext );

    return ACI_SUCCESS;
}
