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

#include <iduHashUtil.h>
#include <mmcSession.h>
#include <mmcTrans.h>
#include <mmdXid.h>

/* BUG-18981 */
IDE_RC mmdXid::initialize(ID_XID           *aUserXid,
                          mmcTransObj      *aTrans,
                          mmdXidHashBucket *aBucket)
{
    iduListNode       *sNode;
    mmdXidMutex       *sMutex = NULL;

    /* PROJ-2436 ADO.NET MSDTC */
    if (aTrans == NULL)
    {
        IDE_TEST(mmcTrans::alloc( NULL, &mTrans ) != IDE_SUCCESS);
    }
    else
    {
        mTrans = aTrans;
    }

    idlOS::memcpy(&mXid, aUserXid, ID_SIZEOF(ID_XID));

    setState(MMD_XA_STATE_NO_TX);

    mBeginFlag   = ID_FALSE;
    mFixCount = 0;
    //fix BUG-22669 XID list에 대한 performance view 필요.
    mAssocSessionID = 0;
    mAssocSession   = NULL;         //BUG-25020
    mHeuristicXaEnd = ID_FALSE;     //BUG-29351
    /* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
       that is to say , chanage the granularity from global to bucket level.
    */
    IDU_LIST_INIT_OBJ(&mListNode,this);
    
    /* PROJ-1381 FAC */
    IDU_LIST_INIT(&mAssocFetchList);

    // bug-35382: mutex optimization during alloc and dealloc
    // mutex 를 pool (hash chain)에서 하나 가져와서 mmdXid에 등록
    mMutex     = NULL;

    // PROJ-2408
    IDE_ASSERT( aBucket->mXidMutexBucketLatch.lockWrite(NULL, NULL) == IDE_SUCCESS);
    // mutex가 없으면 새로 할당하여 사용. latch는 바로 풀어준다
    if (IDU_LIST_IS_EMPTY(&(aBucket->mXidMutexChain)))
    {
        IDE_ASSERT( aBucket->mXidMutexBucketLatch.unlock() == IDE_SUCCESS);

        IDU_FIT_POINT( "mmdXid::initialize::alloc::Mutex" );

        IDE_TEST(mmdXidManager::mXidMutexPool.alloc((void **)&sMutex) != IDE_SUCCESS);
        IDE_TEST(sMutex->mMutex.initialize((SChar *)"MMD_XID_MUTEX",
                    IDU_MUTEX_KIND_POSIX,
                    IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
        mMutex = sMutex;
        mMutexNeedDestroy = ID_TRUE;
    }
    // mutex가 있으면 mutex pool 보호용 latch 잡고 하나 가져온다
    else
    {
        sNode = IDU_LIST_GET_FIRST(&(aBucket->mXidMutexChain));
        sMutex = (mmdXidMutex*)sNode->mObj;
        IDU_LIST_REMOVE(&(sMutex->mListNode));
        mMutex = sMutex;
        mMutexNeedDestroy = ID_FALSE;

        IDE_ASSERT( aBucket->mXidMutexBucketLatch.unlock() == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmdXid::finalize(mmdXidHashBucket *aBucket, idBool aFreeTrans)
{
    // bug-35382: mutex optimization during alloc and dealloc
    // mutex pool에서 가져온 것이 아니라면 바로 free시킨다
    if (mMutexNeedDestroy == ID_TRUE)
    {
        IDE_TEST(mMutex->mMutex.destroy() != IDE_SUCCESS);
        IDE_TEST(mmdXidManager::mXidMutexPool.memfree(mMutex) != IDE_SUCCESS);
    }
    // mutex pool에서 가져온 것이면 latch 잡고 반환한다
    else
    {
        IDE_ASSERT( aBucket->mXidMutexBucketLatch.lockWrite(NULL, NULL) == IDE_SUCCESS );
        IDU_LIST_ADD_LAST(&(aBucket->mXidMutexChain), &(mMutex->mListNode));
        IDE_ASSERT( aBucket->mXidMutexBucketLatch.unlock() == IDE_SUCCESS );
    }
    mMutex = NULL;
    mMutexNeedDestroy = ID_FALSE;

    if (mBeginFlag == ID_TRUE)
    {
        switch (getState())
        {
            case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
            case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
            case MMD_XA_STATE_NO_TX:
                break;

            case MMD_XA_STATE_IDLE:
            case MMD_XA_STATE_ACTIVE:
            case MMD_XA_STATE_PREPARED:
                //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
                rollbackTrans(NULL);
                break;

            default:
                break;
        }
    }

    /* PROJ-2436 ADO.NET MSDTC */
    if (aFreeTrans == ID_TRUE)
    {
        IDE_TEST(mmcTrans::free(NULL, mTrans) != IDE_SUCCESS);
        mTrans = NULL;
    }
    else
    {
        /* nothing to do */
    }

    /* PROJ-1381 FAC : 동작중인건 rollback한다.
     * 그러므로 이 시점에는 AssocFetchList가 모두 정리되어 있어야 한다. */
    IDE_ASSERT( IDU_LIST_IS_EMPTY(&mAssocFetchList) == ID_TRUE );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmdXid::attachTrans(UInt aSlotID)
{
    /* BUG-46644 attach 시점에 mTrans는 초기화 되어 있기에 초기화는 제거한다. */
    mmcTrans::attachXA(mTrans, aSlotID);
    setState(MMD_XA_STATE_PREPARED);

    return IDE_SUCCESS;
}

void mmdXid::beginTrans(mmcSession *aSession)
{
    mmcTrans::beginXA(mTrans, aSession->getStatSQL(), aSession->getSessionInfoFlagForTx());
    mBeginFlag = ID_TRUE;
}

IDE_RC mmdXid::prepareTrans(mmcSession *aSession)
{
    IDE_TEST(mmcTrans::prepareXA(mTrans, &mXid, aSession) != IDE_SUCCESS);

    setState(MMD_XA_STATE_PREPARED);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmdXid::commitTrans(mmcSession *aSession)
{
    //PROJ-1677 DEQ
    smiTrans      *sTrans = mmcTrans::getSmiTrans(mTrans);
    /* PROJ-1381 FAC */
    iduListNode         *sIterator = NULL;
    iduListNode         *sNodeNext = NULL;
    AssocFetchListItem  *sItem;

    //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
    if( aSession != NULL )
    {
        sTrans->setStatistics(aSession->getStatSQL());
    }
    else
    {
        sTrans->setStatistics(NULL);
    }

    /* PROJ-1381 FAC : Commit된 FetchList를 mmcSession으로 옮긴다. */
    IDU_LIST_ITERATE_SAFE(&mAssocFetchList, sIterator, sNodeNext)
    {
        sItem = (AssocFetchListItem *)sIterator->mObj;

        if ( IDU_LIST_IS_EMPTY(&sItem->mFetchList) == ID_FALSE )
        {
            /* 방어코드: finalize된 Session은 AssocFetchList에 남아있으면 안된다. */
            IDE_ASSERT(sItem->mSession->getSessionState() != MMC_SESSION_STATE_INIT);

            sItem->mSession->lockForFetchList();
            IDU_LIST_JOIN_LIST(sItem->mSession->getCommitedFetchList(),
                               &sItem->mFetchList);
            sItem->mSession->unlockForFetchList();
        }

        IDU_LIST_REMOVE( sIterator );
        IDE_TEST( iduMemMgr::free(sItem) != IDE_SUCCESS );
    }

    //fix BUG-30343 Committing a xid can be failed in replication environment.
    IDE_TEST( mmcTrans::commitXA( mTrans, aSession, SMI_RELEASE_TRANSACTION ) != IDE_SUCCESS);
    setState(MMD_XA_STATE_NO_TX);

    mBeginFlag = ID_FALSE;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

void mmdXid::rollbackTrans(mmcSession *aSession)
{
    UInt           sErrorCode;
    SChar         *sErrorMsg;
    UChar          sxidString[XID_DATA_MAX_LEN];

    /* PROJ-1381 FAC */
    iduListNode         *sIterator = NULL;
    iduListNode         *sNodeNext = NULL;
    AssocFetchListItem  *sItem;
    smiTrans            *sTrans = mmcTrans::getSmiTrans(mTrans);
    //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
    if( aSession != NULL )
    {
        sTrans->setStatistics(aSession->getStatSQL());
    }
    else
    {
        sTrans->setStatistics(NULL);
    }

    /* PROJ-1381 FAC : Rollback된 FetchList를 Close한다. */
    IDU_LIST_ITERATE_SAFE(&mAssocFetchList, sIterator, sNodeNext)
    {
        sItem = (AssocFetchListItem *)sIterator->mObj;

        if ( IDU_LIST_IS_EMPTY(&sItem->mFetchList) == ID_FALSE )
        {
            /* 방어코드: finalize된 Session은 AssocFetchList에 남아있으면 안된다. */
            IDE_ASSERT(sItem->mSession->getSessionState() != MMC_SESSION_STATE_INIT);

            sItem->mSession->closeAllCursorByFetchList( &sItem->mFetchList, ID_FALSE );
        }

        IDU_LIST_REMOVE( sIterator );
        IDE_TEST_RAISE( iduMemMgr::free(sItem) != IDE_SUCCESS, RollbackError );
    }

    // BUG-24887 rollback 실패시 정보를 남기고 죽을것
    IDE_TEST( mmcTrans::rollbackXA( mTrans, aSession, SMI_RELEASE_TRANSACTION ) != IDE_SUCCESS);
    setState(MMD_XA_STATE_NO_TX);

    mBeginFlag = ID_FALSE;

    return ;

    // BUG-24887 rollback 실패시 정보를 남기고 죽을것
    IDE_EXCEPTION(RollbackError);
    {
        sErrorCode = ideGetErrorCode();
        sErrorMsg =  ideGetErrorMsg(sErrorCode);
        (void)idaXaConvertXIDToString(NULL, &mXid, sxidString, XID_DATA_MAX_LEN);

        if(sErrorMsg != NULL)
        {
            ideLog::logLine(IDE_XA_0, "XA-ERROR !!! XA ROLLBACK ERROR(%d)\n"
                                      "ERR-%u : %s \n"
                                      "Trans ID : %u\n"
                                      "XID : %s\n",
                        ((aSession != NULL) ? aSession->getSessionID() : 0),
                        sErrorCode,
                        sErrorMsg,
                        mmcTrans::getTransID(mTrans),
                        sxidString);
        }
        else
        {
            ideLog::logLine(IDE_XA_0, "XA-ERROR !!! XA ROLLBACK ERROR(%d)\n"
                                      "Trans ID : %u\n"
                                      "XID : %s\n",
                        ((aSession != NULL) ? aSession->getSessionID() : 0),
                        mmcTrans::getTransID(mTrans),
                        sxidString);
        }
        IDE_ASSERT(0);
    }
    IDE_EXCEPTION_END;
}

UInt mmdXid::hashFunc(void *aKey)
{
    /* BUG-18981 */        
    return iduHashUtil::hashBinary(aKey, ID_SIZEOF(ID_XID));
}

SInt mmdXid::compFunc(void *aLhs, void *aRhs)
{
    /* BUG-18981 */        
    return idlOS::memcmp(aLhs, aRhs, ID_SIZEOF(ID_XID));
}

/***********************************************************************
 * Description:
XID와 session과 관계를 성립한다.
그리고 XID에 있는 SM 트랜잭션에게  session정보를 갱신한다.

***********************************************************************/
//fix BUG-22669 need to XID List performance view. 
void   mmdXid::associate(mmcSession * aSession)
{
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.

    //XID 와 associate된 session정보 갱신.
    mAssocSessionID     = aSession->getSessionID();
    mAssocSession       = aSession;         //BUG-25020
    
    // SM에게 transaction을사용하는 session 이변경됨을 알림.
    if(mTrans != NULL)
    {
        mmcTrans::getSmiTrans(mTrans)->setStatistics(aSession->getStatSQL());
    }

}

/***********************************************************************
 * Description:
XID와 session과 관계를 단절한다.
그리고 XID에 있는 SM 트랜잭션에게  session정보를 clearn한다.

***********************************************************************/
//fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
//그들간의 관계를 정확히 유지해야 함.
void   mmdXid::disAssociate(mmdXaState aState)
{
    mAssocSessionID     = ID_NULL_SESSION_ID;
    mAssocSession       = NULL;             //BUG-25020
    
    /* BUG-26164
     * Heuristic하게 처리되었거나 NO_TX인 경우에는
     * 해당 Transaction이 이미 없으므로, Transaction 처리를 할 필요가 없다.
     */
    switch (aState)
    {
        case MMD_XA_STATE_IDLE:
        case MMD_XA_STATE_ACTIVE:
        case MMD_XA_STATE_ROLLBACK_ONLY:
            if(mTrans != NULL)
            {
                // SM에게 transaction을사용하는 session이 없음을 알림.
                mmcTrans::getSmiTrans(mTrans)->setStatistics(NULL);
            }
            break;
        case MMD_XA_STATE_PREPARED:
        case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
        case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
        case MMD_XA_STATE_NO_TX:
            //Nothing To Do.
            break;
        default:
            break;
    }
}

/* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
 XA fix 시에 xid list latch를 shared 걸때 xid fix count 정합성을 위한 함수 */
void mmdXid::fixWithLatch()
{
    IDE_ASSERT(mMutex->mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    mFixCount++;
    IDE_ASSERT(mMutex->mMutex.unlock() == IDE_SUCCESS);
}

/* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
  XA Unfix 시에 latch duaration을 줄이기위하여 xid fix-Count를 xid list latch release전에
  구한다.*/
void mmdXid::unfix(UInt *aFixCount)
{
    // prvent minus 
    IDE_ASSERT(mFixCount != 0);
    // instruction ordering 
    ID_SERIAL_BEGIN(mFixCount--);
    ID_SERIAL_END(*aFixCount = mFixCount);
}

/* PROJ-1381 Fetch Across Commits */

/**
 * Session의 Non-Commited FetchList를 Xid와 관련된 FetchList로 추가한다.
 *
 * @param aSession[IN] FetchList와 연관된 Session
 * @return 성공하면 IDE_SUCCESS, 아니면 IDE_FAILURE
 */
IDE_RC mmdXid::addAssocFetchListItem(mmcSession *aSession)
{
    AssocFetchListItem *sItem;

    IDU_FIT_POINT_RAISE( "mmdXid::addAssocFetchListItem::malloc::Item",
                          InsufficientMemory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_MMT,
                                      ID_SIZEOF(AssocFetchListItem),
                                      (void **)&sItem,
                                      IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

    IDU_LIST_INIT(&sItem->mFetchList);
    IDU_LIST_INIT_OBJ(&sItem->mListNode, sItem);

    aSession->lockForFetchList();
    IDU_LIST_JOIN_LIST(&sItem->mFetchList, aSession->getFetchList());
    sItem->mSession = aSession;
    aSession->unlockForFetchList();

    IDU_LIST_ADD_LAST(&mAssocFetchList, &sItem->mListNode);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
