/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: rpxPJChild.cpp 90170 2021-03-10 06:05:50Z lswhh $
 **********************************************************************/

#include <idl.h> // to remove win32 compile warning

#include <smxTrans.h>

#include <qci.h>

#include <rp.h>
#include <rpuProperty.h>
#include <rpcManager.h>
#include <rpxPJMgr.h>
#include <rpxPJChild.h>
#include <rpxSync.h>
#include <rpcHBT.h>
#include <mtc.h>

/* PROJ-2240 */
#include <rpdCatalog.h>

rpxPJChild::rpxPJChild() : idtBaseThread(IDT_DETACHED)
{
}

IDE_RC rpxPJChild::initialize( rpxPJMgr     * aParent,
                               rpdMeta      * aMeta,
                               RP_SOCKET_TYPE aSocketType,
                               SChar        * aIPAddress,
                               UInt           aPortNo,
                               rpIBLatency    aIBLatency, 
                               smiStatement * aStatement,
                               UInt           aChildCount,
                               UInt           aNumber,
                               iduList      * aSyncList,
                               idBool       * aExitFlag )
{
    mParent       = aParent;
    mChildCount   = aChildCount;
    mNumber       = aNumber;
    mStatus       = RPX_PJ_SIGNAL_NONE;
    mExitFlag     = aExitFlag;
    mStatement    = aStatement;
    mMeta         = aMeta;
    mSyncList     = aSyncList;
    mSocketType   = aSocketType;
    mIPAddress    = aIPAddress;
    mPortNo       = aPortNo;
    mIBLatency    = aIBLatency;

    return IDE_SUCCESS;
}

void rpxPJChild::destroy()
{
    return;
}

IDE_RC rpxPJChild::initializeThread()
{
    /* Thread의 run()에서만 사용하는 메모리를 할당한다. */

    /* mMessenger는 rpxPJChild::run()에서만 사용하므로, 여기로 옮긴다. */
    /* 
     * PJ Child 는 Sync 모드에서만 동작하기 때문에 Messager 안 SOCKET 에서 
     * Lock 을 잡을 필요가 없다
     */
    
    IDE_TEST( mMessenger.initialize( NULL,
                                     RP_MESSENGER_NONE,
                                     mSocketType,
                                     mExitFlag,
                                     &( mMeta->mReplication ),
                                     NULL,
                                     ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    return IDE_FAILURE;
}

void rpxPJChild::finalizeThread()
{
    mMessenger.destroy(RPN_RELEASE_PROTOCOL_CONTEXT);

    return;
}

iduListNode * rpxPJChild::getFirstNode()
{
    UInt i = 0;
    iduListNode *sNode = IDU_LIST_GET_FIRST( mSyncList );

    while( i < mNumber  )
    {
        sNode = getNextNode( sNode );

        i++;
    }

    return sNode;
}

iduListNode * rpxPJChild::getNextNode( iduListNode *aNode )
{
    iduListNode * sNode = IDU_LIST_GET_NEXT( aNode );
    
    if( sNode != mSyncList )
    {
        /*do nothing*/
    }
    else
    {
        /* Head Node Skip */
        sNode = IDU_LIST_GET_NEXT( sNode );
    }

    return sNode;
}

void rpxPJChild::run()
{
    idBool       sIsConnect = ID_FALSE;
    SChar        sLog[512] = { 0, };

    rpxSyncItem * sSyncItem  = NULL;
    iduListNode * sCurNode   = NULL;
    iduListNode * sFirstNode = NULL;

    IDE_CLEAR();

    if ( mSocketType == RP_SOCKET_TYPE_TCP )
    {
        IDE_TEST( mMessenger.connectThroughTcp( mIPAddress,
                                                mPortNo )
                  != IDE_SUCCESS );
        sIsConnect = ID_TRUE;
    }
    else if ( mSocketType == RP_SOCKET_TYPE_IB )
    {
        IDE_TEST( mMessenger.connectThroughIB( mIPAddress,
                                               mPortNo,
                                               mIBLatency )
                  != IDE_SUCCESS );
        sIsConnect = ID_TRUE;
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    IDE_TEST( mMessenger.handshake( mMeta ) != IDE_SUCCESS );

    idlOS::sprintf(sLog,
                   "[%"ID_UINT32_FMT"] PJ_RUN",
                   mNumber);

    // BUG-15471
    ideLog::log( IDE_RP_0, RP_TRC_PJC_NTC_MSG, sLog );

    mStatus = RPX_PJ_SIGNAL_RUNNING;

    /* mSyncList 는 rpxPJMgr 의 mSyncList 의 포인터로 rpxPJMgr 에서 관리한다.*/
    if ( IDU_LIST_IS_EMPTY( mSyncList ) != ID_TRUE )
    {
        sFirstNode = getFirstNode();
        sCurNode = sFirstNode;

        do
        {
            IDE_TEST_RAISE( *mExitFlag == ID_TRUE, ERR_EXIT );

            sSyncItem = (rpxSyncItem*)sCurNode->mObj;
            IDE_DASSERT( sSyncItem != NULL );

            IDE_TEST( doSync( sSyncItem ) != IDE_SUCCESS  );

            sCurNode = getNextNode( sCurNode );
        } while( sCurNode != sFirstNode );
    }
    else
    {
        /* nothing to do */
    }

    /* Send Replication Stop Message */
    ideLog::log(IDE_RP_0, "[PJ Child] SEND Stop Message!\n");

    IDE_TEST( mMessenger.sendStopAndReceiveAck() != IDE_SUCCESS );

    ideLog::log(IDE_RP_0, "[PJ Child] SEND Stop Message SUCCESS!!!\n");

    sIsConnect = ID_FALSE;

    mMessenger.disconnect();

    idlOS::sprintf( sLog, "[%"ID_UINT32_FMT"] PJ_EXIT", mNumber );

    // BUG-15471
    ideLog::log( IDE_RP_0, RP_TRC_PJC_NTC_MSG, sLog );

    mStatus = RPX_PJ_SIGNAL_EXIT;

    return;

    IDE_EXCEPTION( ERR_EXIT );
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_EXIT_FLAG_SET ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    /* Link가 Invalid이면 처리해주는 코드 필요함 */
    if( sIsConnect == ID_TRUE )
    {
        mMessenger.disconnect();
    }
    if( *mExitFlag != ID_TRUE )
    {
        *mExitFlag = ID_TRUE;
    }
    IDE_WARNING( IDE_RP_0,  RP_TRC_PJC_ERR_RUN );

    mStatus = RPX_PJ_SIGNAL_ERROR | RPX_PJ_SIGNAL_EXIT;

    return;
}

IDE_RC rpxPJChild::doSync( rpxSyncItem * aSyncItem )
{
    SChar        sLog[512] = { 0, };

    idlOS::snprintf(sLog, 512,
                     "[%"ID_UINT32_FMT"] TABLE %s SYNC START",
                    mNumber,
                    aSyncItem->mTable->mItem.mLocalTablename );

    // BUG-15471
    ideLog::log( IDE_RP_0, RP_TRC_PJC_NTC_MSG, sLog );

    IDE_TEST( rpxSync::syncTable( mStatement,
                                  &mMessenger,
                                  aSyncItem->mTable,
                                  mExitFlag,
                                  mChildCount,
                                  mNumber,
                                  &( aSyncItem->mSyncedTuples ), /* V$REPSYNC.SYNC_RECORD_COUNT*/
                                  ID_FALSE /* aIsALA */ ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
