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


/**********************************************************************
 * $Id: ulsdnExecute.c 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>

#include <ulsdnStmt.h>
#include <ulsdStmtInline.h>

static ACI_RC ulsdnStmtShardStmtPartialRollbackRequest( ulnFnContext     *aFnContext,
                                                        ulnPtContext     *aPtContext )
{
    cmiProtocol           sPacket;
    cmiProtocolContext  * sCtx = &(aPtContext->mCmiPtContext);

    sPacket.mOpID = CMP_OP_DB_ShardStmtPartialRollback;

    CMI_WRITE_CHECK( sCtx, 1 );
    
    CMI_WOP( sCtx, CMP_OP_DB_ShardStmtPartialRollback );

    ACI_TEST(ulnWriteProtocol( aFnContext, 
                               aPtContext, 
                               &sPacket) 
             != ACI_SUCCESS ) ;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulsdnStmtShardStmtPartialRollbackMain( ulnFnContext     *aFnContext,
                                                     ulnDbc           *aDbc )
{
    ULN_FLAG( sNeedFinPtContext );
    cmiProtocolContext  * sCtx = NULL;

    ACI_TEST_RAISE( aDbc->mIsConnected == ACP_FALSE, LABEL_ABORT_NO_CONNECTION );

    ACI_TEST( ulnInitializeProtocolContext( aFnContext,
                                            &(aDbc->mPtContext),
                                            &(aDbc->mSession) )
              != ACI_SUCCESS );
    ULN_FLAG_UP( sNeedFinPtContext );

    /* request */
    ACI_TEST( ulsdnStmtShardStmtPartialRollbackRequest( aFnContext,
                                                        &(aDbc->mPtContext) )
              != ACI_SUCCESS );

    ACI_TEST( ulnFlushProtocol( aFnContext, 
                                &(aDbc->mPtContext) ) 
              != ACI_SUCCESS );

    /* Waiting for response */
    sCtx = &(aDbc->mPtContext.mCmiPtContext);
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

    ULN_FLAG_DOWN( sNeedFinPtContext );
    ACI_TEST( ulnFinalizeProtocolContext( aFnContext, &(aDbc->mPtContext) ) !=
              ACI_SUCCESS );

     return ACI_SUCCESS;

     ACI_EXCEPTION( LABEL_ABORT_NO_CONNECTION )
     {
         ulnError( aFnContext, ulERR_ABORT_NO_CONNECTION, "" );
     }
     ACI_EXCEPTION_END;

     ULN_IS_FLAG_UP(sNeedFinPtContext)
     {
         ulnFinalizeProtocolContext(aFnContext,&(aDbc->mPtContext));
     }

     return ACI_FAILURE;
}

SQLRETURN ulsdnStmtShardStmtPartialRollback( ulnDbc         * aDbc )
{
    ULN_FLAG(sNeedExit);
    ulnFnContext     sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aDbc, ULN_OBJ_TYPE_DBC );

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST( ulsdnStmtShardStmtPartialRollbackMain( &sFnContext, aDbc ) != ACI_SUCCESS );

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

ACI_RC ulsdnStmtShardStmtPartialRollbackResult( cmiProtocolContext * aProtocolContext,
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

SQLRETURN ulsdnStmtSetPartialExecType( ulnStmt      * aStmt,
                                       acp_sint32_t   aPartialCoordType )
{
    ulnFnContext sFnContext;
    acp_bool_t   sNeedExit = ACP_FALSE;

    ACI_TEST_RAISE( aStmt == NULL,
                    LABEL_STMT_IS_INVALID );

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_FOR_SD, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    if ( aPartialCoordType == 1 )
    {
        ulsdStmtSetPartialExecType( aStmt, ULN_SHARD_PARTIAL_EXEC_TYPE_COORD );
    }
    else if ( aPartialCoordType == 2 )
    {
        ulsdStmtSetPartialExecType( aStmt, ULN_SHARD_PARTIAL_EXEC_TYPE_QUERY );
    }
    else
    {
        ulsdStmtSetPartialExecType( aStmt, ULN_SHARD_PARTIAL_EXEC_TYPE_NONE );
    }

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);
    
    return SQL_SUCCESS;

    ACI_EXCEPTION ( LABEL_STMT_IS_INVALID )
    {
        ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_FOR_SD, aStmt, ULN_OBJ_TYPE_STMT );

        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}
