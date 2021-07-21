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
 * $Id$
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>

#include <ulsd.h>

SQLRETURN ulsdDisconnect(ulnDbc *aDbc)
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_DISCONNECT, aDbc, ULN_OBJ_TYPE_DBC );

    // BUG-47312
    ACI_TEST_RAISE( aDbc == NULL, LABEL_INVALID_HANDLE);
    ACI_TEST_RAISE( aDbc->mObj.mType != ULN_OBJ_TYPE_DBC, LABEL_INVALID_HANDLE );

    /* PROJ-2739 Client-side Sharding LOB */
    (void) ulsdLobLocatorFreeAll( aDbc );

    ACI_TEST(!SQL_SUCCEEDED(ulsdNodeDisconnect(aDbc->mShardDbcCxt.mShardDbc)));
 
    return ulnDisconnect(aDbc);

    ACI_EXCEPTION( LABEL_INVALID_HANDLE )
    {
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

void ulsdSilentDisconnect(ulnDbc *aDbc)
{
    SQLRETURN    sRet;

    // BUG-47312
    if ( aDbc != NULL )
    {
        if ( aDbc->mObj.mType == ULN_OBJ_TYPE_DBC )
        {
            ulsdNodeSilentDisconnect(aDbc->mShardDbcCxt.mShardDbc);
     
            if ( ulnDbcIsConnected( aDbc ) == ACP_TRUE )
            {
                sRet = ulnDisconnect( aDbc );
            }

            ACP_UNUSED(sRet);
        }
    }

    return;
}

SQLRETURN ulsdNodeDisconnect(ulsdDbc *aShard)
{
    ACI_RC        sRet1 = ACI_SUCCESS;
    SQLRETURN     sRet2 = SQL_ERROR;
    SQLRETURN     sRet3 = SQL_ERROR;
    SQLRETURN     sRet4 = SQL_ERROR;
    ulnFnContext  sFnContext;
    acp_bool_t    sHasFailNode = ACP_FALSE;
    acp_bool_t    sResultFail[ULSD_SD_NODE_MAX_COUNT] = {ACP_FALSE,};
    acp_uint16_t  sCnt = 0;

    ACI_TEST( aShard->mNodeCount > ULSD_SD_NODE_MAX_COUNT );

    for ( sCnt = 0; sCnt < aShard->mNodeCount; sCnt++ )
    {
        SHARD_LOG("(Disconnect) NodeId=%d, Server=%s:%d\n",
                  aShard->mNodeInfo[sCnt]->mNodeId,
                  aShard->mNodeInfo[sCnt]->mServerIP,
                  aShard->mNodeInfo[sCnt]->mPortNo);

        ulnDbcSetIsConnected(aShard->mNodeInfo[sCnt]->mNodeDbc, ACP_FALSE);
        sRet1 = ACI_SUCCESS;

        sRet2 = ulnDisconnect(aShard->mNodeInfo[sCnt]->mNodeDbc);

        sRet3 = ulnFreeHandle(SQL_HANDLE_DBC, aShard->mNodeInfo[sCnt]->mNodeDbc);

        sRet4 = ulsdNodeInfoFree(aShard->mNodeInfo[sCnt]);

        if ( sRet1 != ACI_SUCCESS
             || !SQL_SUCCEEDED(sRet2)
             || !SQL_SUCCEEDED(sRet3)
             || !SQL_SUCCEEDED(sRet4) )
        {
            sHasFailNode = ACP_TRUE;
            sResultFail[sCnt] = ACP_TRUE;
        }
        else
        {
            sResultFail[sCnt] = ACP_FALSE;
        }
    }

    ulsdNodeFree(aShard);

    ACI_TEST_RAISE(sHasFailNode == ACP_TRUE, LABEL_NODE_DISCONNECTION_FAIL);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_NODE_DISCONNECTION_FAIL)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_DISCONNECT, aShard->mMetaDbc, ULN_OBJ_TYPE_DBC);

        for ( sCnt = 0; sCnt < aShard->mNodeCount; sCnt++ )
        {
            if ( sResultFail[sCnt] == ACP_TRUE )
            {
                ulsdNativeErrorToUlnError(&sFnContext,
                                          SQL_HANDLE_DBC,
                                          (ulnObject *)aShard->mNodeInfo[sCnt]->mNodeDbc,
                                          aShard->mNodeInfo[sCnt],
                                          "Disconnect");
            }
            else
            {
                /* Do Nothing */
            }
        }
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdNodeSilentDisconnect(ulsdDbc *aShard)
{
    acp_uint16_t sCnt;

    for ( sCnt = 0; sCnt < aShard->mNodeCount; sCnt++ )
    {
        if ( aShard->mNodeInfo[sCnt]->mNodeDbc != NULL )
        {
            SHARD_LOG("(Silent Disconnect) NodeId=%d, Server=%s:%d\n",
                      aShard->mNodeInfo[sCnt]->mNodeId,
                      aShard->mNodeInfo[sCnt]->mServerIP,
                      aShard->mNodeInfo[sCnt]->mPortNo);

            ulnDbcSetIsConnected(aShard->mNodeInfo[sCnt]->mNodeDbc, ACP_FALSE);

            ulnDisconnect(aShard->mNodeInfo[sCnt]->mNodeDbc);

            ulnFreeHandle(SQL_HANDLE_DBC, aShard->mNodeInfo[sCnt]->mNodeDbc);

            ulsdNodeInfoFree(aShard->mNodeInfo[sCnt]);
        }
        else
        {
            /* Do Nothing */
        }
    }

    ulsdNodeFree(aShard);

    return SQL_SUCCESS;
}

void ulsdNodeFree(ulsdDbc *aShard)
{
    if ( aShard->mNodeInfo != NULL )
    {
        acpMemFree(aShard->mNodeInfo);
        aShard->mNodeInfo = NULL;
    }
    else
    {
        /* Do Nothing */
    }

    aShard->mNodeCount = 0;
    aShard->mIsTestEnable = ACP_FALSE;
    aShard->mTouched = ACP_FALSE;
}
