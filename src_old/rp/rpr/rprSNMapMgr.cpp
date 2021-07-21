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
 * $Id: rprSNMapMgr.cpp $
 **********************************************************************/
#include <idu.h>
#include <rprSNMapMgr.h>

IDE_RC rprSNMapMgr::initialize( const SChar * aRepName,
                                idBool        aNeedLock )
{
    SChar          sName[IDU_MUTEX_NAME_LEN];

    mMaxReplicatedSN       = SM_SN_NULL;
    mMaxMasterCommitSN = SM_SN_NULL;

    idlOS::memset(mRepName, 0, QC_MAX_OBJECT_NAME_LEN + 1);
    idlOS::strncpy(mRepName, aRepName, QC_MAX_OBJECT_NAME_LEN);

    idlOS::snprintf(sName, IDU_MUTEX_NAME_LEN, "RP_%s_SN_MAP_MGR_MUTEX",
                    aRepName);

    IDE_TEST_RAISE(mSNMapMgrMutex.initialize(sName,
                                             IDU_MUTEX_KIND_NATIVE,
                                             IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    IDU_LIST_INIT(&mSNMap);

    /* pool 초기화
     * 서버가 동작중에 메모리 할당은 receiver만 수행하며, 해제도 receiver가 한다.
     * 서버가 시작하거나, 종료할 때에는 executor가 할당 해제를 한다.
     * 즉 메모리 할당 해제는 특정 시점에 하나의 스레드가 담당하므로,
     * mutex를 사용할 필요가 없다.
     */
    idlOS::snprintf(sName, IDU_MUTEX_NAME_LEN, "RP_%s_SN_MAP_POOL", aRepName);

    IDE_TEST( mEntryPool.initialize(IDU_MEM_RP_RPR,
                                    sName,
                                    1,
                                    ID_SIZEOF(rprSNMapEntry),
                                    (1024 * 4),
                                    IDU_AUTOFREE_CHUNK_LIMIT, //chunk max(default)
                                    aNeedLock,                //use mutex(no use)
                                    8,                        //align byte(default)
                                    ID_FALSE,				  //ForcePooling
                                    ID_TRUE,				  //GarbageCollection
                                    ID_TRUE,                  /* HWCacheLine */
                                    IDU_MEMPOOL_TYPE_LEGACY   /* mempool type*/) 
              != IDE_SUCCESS );			

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MUTEX_INIT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_ThrMutexInit));
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Mutex initialization error");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/* replicated Tx의 begin SN 순서대로 정렬됨,
 * insertNewTx가 호출될 때 들어오는 master begin SN이 순서대로 들어오지
 * 않기 때문에 master begin SN은 정렬되지 않음
 */
IDE_RC rprSNMapMgr::insertNewTx(smSN aMasterBeginSN, rprSNMapEntry **aNewEntry)
{
    rprSNMapEntry *sNewEntry;

    IDE_TEST(mEntryPool.alloc((void**)&sNewEntry) != IDE_SUCCESS);

    sNewEntry->mMasterBeginSN      = aMasterBeginSN;
    sNewEntry->mMasterCommitSN     = SM_SN_NULL;
    sNewEntry->mReplicatedBeginSN  = SM_SN_NULL;
    sNewEntry->mReplicatedCommitSN = SM_SN_NULL;

    IDU_LIST_INIT_OBJ(&(sNewEntry->mNode), sNewEntry);

    IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);

    IDU_LIST_ADD_LAST(&mSNMap, &(sNewEntry->mNode));

    IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);

    *aNewEntry = sNewEntry;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*call by rpcExecutor::loadRecoveryInfos*/
IDE_RC rprSNMapMgr::insertEntry(rpdRecoveryInfo * aRecoveryInfo)
{
    rprSNMapEntry *sNewEntry;
    IDE_TEST( insertNewTx( aRecoveryInfo->mMasterBeginSN, &sNewEntry ) != IDE_SUCCESS );
    setSNs( sNewEntry,
            aRecoveryInfo->mMasterCommitSN,
            aRecoveryInfo->mReplicatedBeginSN,
            aRecoveryInfo->mReplicatedCommitSN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*call by rpxReceiverApply::applyTrCommit for recovery support normal receiver*/
void rprSNMapMgr::deleteTxByEntry(rprSNMapEntry* aEntry)
{
    IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);

    IDU_LIST_REMOVE(&(aEntry->mNode));

    IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);

    (void)mEntryPool.memfree(aEntry);

    return;
}

/*call by rpxSender::sendTrCommit for recovery sender*/
void rprSNMapMgr::deleteTxByReplicatedCommitSN(smSN    aReplicatedCommitSN,
                                               idBool* aIsExist)
{
    iduListNode   *  sNode;
    iduListNode   *  sDummy;
    rprSNMapEntry *  sEntry;

    *aIsExist = ID_FALSE;

    IDU_LIST_ITERATE_SAFE(&mSNMap, sNode, sDummy)
    {
        sEntry = (rprSNMapEntry*)sNode->mObj;
        if(sEntry->mReplicatedCommitSN == aReplicatedCommitSN)
        {
            IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);
            IDU_LIST_REMOVE(sNode);
            IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);
            (void)mEntryPool.memfree(sEntry);
            *aIsExist = ID_TRUE;
        }
    }

    return;
}

/* Local에서 디스크에 flush된 시점의 SN과 대응되는 Remote SN을 구함
 * 함수 내에서 하는 작업은 세 가지임
 * 1. Local에서 디스크에 flush된 시점의 SN과 대응되는 Remote SN을 반환 
 * 2. SN Map의 최소 SN을 구하여 설정 함
 * 3. local과 remote에서 모두 디스크에 flush된 엔트리를 삭제 함
 *                         sn map structure
 *                  master                 replicated
 *  tx1|tx1'  begin sn | commit sn | begin sn | commit sn
 *  tx2|tx2'  begin sn | commit sn | begin sn | commit sn
 */
void rprSNMapMgr::getLocalFlushedRemoteSN(smSN  aLocalFlushSN, 
                                          smSN  aRemoteFlushSN,
                                          smSN  aRestartSN,
                                          smSN *aLocalFlushedRemoteSN)
{
    smSN             sLFRSN     = SM_SN_NULL; //local flushed remote sn
    iduListNode   *  sNode      = NULL;
    iduListNode   *  sDummy     = NULL;
    rprSNMapEntry *  sEntry     = NULL;

    if(aLocalFlushSN != SM_SN_NULL)
    {
        if(IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE)
        {
            IDU_LIST_ITERATE_SAFE(&mSNMap, sNode, sDummy)
            {
                sEntry = (rprSNMapEntry*)sNode->mObj;

                /* 1.처음 엔트리 보다 Flush된 SN이 작은경우
                 *   아무것도 Flush되지 않았으므로 sLFRSN은 SM_SN_NULL을 반환
                 * 2.처음 엔트리가 아닌 경우
                 *   이전 commit sn과 현재 begin sn사이에 sLFRSN이 있으므로
                 *   이전 commit sn을 sLFRSN으로 반환
                 */
                if(aLocalFlushSN < sEntry->mReplicatedBeginSN)
                {
                    break;
                }

                //replicated begin보다는 크거나 같고, commit보다는 작음. master의 begin sn으로 설정
                if(aLocalFlushSN < sEntry->mReplicatedCommitSN)
                {
                    sLFRSN = sEntry->mMasterBeginSN;
                    break;
                }

                //replicated commit보다 크거나 같음. master의 commit sn으로 설정 후, 다음 엔트리 확인
                //local에서는 flush되었음
                sLFRSN = sEntry->mMasterCommitSN;
                //아직 aLocalFlushSN보다 큰 값이 나오지 않았으므로, 다음 노드를 확인해야 함
                //remote에서 flush되었으므로 제거 (모두 flush되었음)
                if(aRemoteFlushSN != SM_SN_NULL)
                {
                    if ( ( aRemoteFlushSN >= sEntry->mMasterCommitSN ) &&
                         ( aLocalFlushSN >= sEntry->mReplicatedCommitSN ) )
                    {
                        IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);
                        IDU_LIST_REMOVE(sNode);
                        IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);

                        (void)mEntryPool.memfree(sEntry);
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
            }
        }
        else
        {
            /*
             * BUG-41331
             * SN Mapping table이 비어있다는 것은
             * 1. Replication Log가 다 처리되었다는 것과
             * 2. Active와 Standby가 Replication 데이터가 같다는
             * 을 뜻하므로 Aciver server의 restartSN을 LocalFlushRemoteSN을 설정한다.
             */
            sLFRSN = aRestartSN;
        }
    }

    *aLocalFlushedRemoteSN = sLFRSN;

    return;
}

/* Recoery Sender가 로그를 읽은 후 해당 트랜잭션이 Recovery해야하는지 확인 함
 * SN Map은 replicated tx의 begin sn으로 정렬되어있으므로, aBeginLogSN
 * 보다 큰 값이 나오면 scan을 중단 하고 recovery가 불필요한 트랜잭션으로 판단 함
 */
idBool rprSNMapMgr::needRecoveryTx(smSN aBeginLogSN)
{
    iduListNode   *  sNode      = NULL;
    rprSNMapEntry *  sEntry     = NULL;

    if(IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE)
    {
        IDU_LIST_ITERATE(&mSNMap, sNode)
        {
            sEntry = (rprSNMapEntry*)sNode->mObj;
            if(sEntry->mReplicatedBeginSN == aBeginLogSN)
            { //Recovery 해야 함
                return ID_TRUE;
            }
            if(sEntry->mReplicatedBeginSN > aBeginLogSN)
            {
                break;
            }
        }
    }
    //검색 실패
    return ID_FALSE;
}

/*call by rpcExecutor::checkAndGiveupReplication for checkpoint thread*/
smSN rprSNMapMgr::getMinReplicatedSN()
{
    smSN sMinSN = SM_SN_NULL;
    iduListNode   *  sNode      = NULL;
    rprSNMapEntry *  sEntry     = NULL;

    IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);

    if(IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE)
    {
        IDU_LIST_ITERATE(&mSNMap, sNode)
        {
            sEntry = (rprSNMapEntry*)sNode->mObj;
            sMinSN = sEntry->mReplicatedBeginSN;
            break;
        }
    }

    IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);

    return sMinSN;
}

/*call by rpcExecutor::saveAllRecoveryInfos*/
void rprSNMapMgr::getFirstEntrySNsNDelete(rpdRecoveryInfo * aRecoveryInfos)
{
    iduListNode   * sNode;
    rprSNMapEntry * sEntry;

    aRecoveryInfos->mMasterBeginSN      = SM_SN_NULL;
    aRecoveryInfos->mMasterCommitSN     = SM_SN_NULL;
    aRecoveryInfos->mReplicatedBeginSN  = SM_SN_NULL;
    aRecoveryInfos->mReplicatedCommitSN = SM_SN_NULL;

    if(IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE)
    {
        sNode = IDU_LIST_GET_FIRST(&mSNMap);
        sEntry = (rprSNMapEntry*)sNode->mObj;

        IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);
        IDU_LIST_REMOVE(sNode);
        IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);

        aRecoveryInfos->mMasterBeginSN      = sEntry->mMasterBeginSN;
        aRecoveryInfos->mMasterCommitSN     = sEntry->mMasterCommitSN;
        aRecoveryInfos->mReplicatedBeginSN  = sEntry->mReplicatedBeginSN;
        aRecoveryInfos->mReplicatedCommitSN = sEntry->mReplicatedCommitSN;

        (void)mEntryPool.memfree(sEntry);
    }

    return;

}

void rprSNMapMgr::getFirstEntrySN(rpdRecoveryInfo * aRecoveryInfos)
{
    iduListNode   * sNode;
    rprSNMapEntry * sEntry;

    aRecoveryInfos->mMasterBeginSN      = SM_SN_NULL;
    aRecoveryInfos->mMasterCommitSN     = SM_SN_NULL;
    aRecoveryInfos->mReplicatedBeginSN  = SM_SN_NULL;
    aRecoveryInfos->mReplicatedCommitSN = SM_SN_NULL;

    if(IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE)
    {
        sNode = IDU_LIST_GET_FIRST(&mSNMap);

        sEntry = (rprSNMapEntry*)sNode->mObj;
        
        aRecoveryInfos->mMasterBeginSN      = sEntry->mMasterBeginSN;
        aRecoveryInfos->mMasterCommitSN     = sEntry->mMasterCommitSN;
        aRecoveryInfos->mReplicatedBeginSN  = sEntry->mReplicatedBeginSN;
        aRecoveryInfos->mReplicatedCommitSN = sEntry->mReplicatedCommitSN;
    }
    return;
}


void rprSNMapMgr::destroy()
{
    mMaxReplicatedSN   = SM_SN_NULL;
    mMaxMasterCommitSN = SM_SN_NULL;

    IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);
    IDU_LIST_INIT(&mSNMap);
    IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);

    if(mEntryPool.destroy(ID_FALSE) != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if(mSNMapMgrMutex.destroy() != 0)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    return;
}

void rprSNMapMgr::setSNs(rprSNMapEntry* aEntry,
                         smSN           aMasterCommitSN,
                         smSN           aReplicatedBeginSN,
                         smSN           aReplicatedCommitSN)
{
    aEntry->mMasterCommitSN     = aMasterCommitSN;
    aEntry->mReplicatedBeginSN  = aReplicatedBeginSN;
    aEntry->mReplicatedCommitSN = aReplicatedCommitSN;
}

/*delete Master Commit SN <= Active's RP Recovery SN Entry for recovery sender*/
UInt rprSNMapMgr::refineSNMap(smSN aActiveRPRecoverySN)
{
    iduListNode   *  sNode;
    iduListNode   *  sDummy;
    rprSNMapEntry *  sEntry;
    UInt             sCount = 0;

    if(IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE)
    {
        IDU_LIST_ITERATE_SAFE(&mSNMap, sNode, sDummy)
        {
            sEntry = (rprSNMapEntry*)sNode->mObj;
            sCount ++;

            if((sEntry->mMasterCommitSN == SM_SN_NULL) ||
               (sEntry->mMasterCommitSN <= aActiveRPRecoverySN))
            {
                IDE_ASSERT(mSNMapMgrMutex.lock(NULL) == IDE_SUCCESS);
                IDU_LIST_REMOVE(sNode);
                IDE_ASSERT(mSNMapMgrMutex.unlock() == IDE_SUCCESS);
                (void)mEntryPool.memfree(sEntry);
                sCount --;
            }
        }
    }

    return sCount;
}


/*call by Receiver finalize*/
void rprSNMapMgr::setMaxSNs()
{
    iduListNode   *  sNode      = NULL;
    rprSNMapEntry *  sEntry     = NULL;
    smSN     sMaxMasterCommitSN = 0;
    smSN     sMaxReplicatedSN   = 0;
    
    if(IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE)
    {
        IDU_LIST_ITERATE(&mSNMap, sNode)
        {
            sEntry = (rprSNMapEntry*)sNode->mObj;
            
            //max master commit sn
            if(sEntry->mMasterCommitSN != SM_SN_NULL)
            {
                if(sMaxMasterCommitSN < sEntry->mMasterCommitSN)
                {
                    sMaxMasterCommitSN = sEntry->mMasterCommitSN;
                }
            } //max master commit sn

            //max replicated sn
            if((sEntry->mReplicatedBeginSN != SM_SN_NULL) && 
               (sEntry->mReplicatedCommitSN != SM_SN_NULL))
            {
                if(sMaxReplicatedSN < sEntry->mReplicatedCommitSN)
                {
                    sMaxReplicatedSN = sEntry->mReplicatedCommitSN;
                }
            } //max replicated sn
        } // list iterate
    } //is empty
    
    if(sMaxMasterCommitSN != 0)
    {
        mMaxMasterCommitSN = sMaxMasterCommitSN;
    }
    else
    {
        mMaxMasterCommitSN = SM_SN_NULL;
    }
    
    if(sMaxReplicatedSN != 0)
    {
        mMaxReplicatedSN = sMaxReplicatedSN;
    }
    else
    {
        mMaxReplicatedSN = SM_SN_NULL;
    }
}

smSN rprSNMapMgr::getMaxMasterCommitSN()
{
    if((mMaxMasterCommitSN == SM_SN_NULL) && 
       (IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE))
    {
        setMaxSNs();
    }
    return mMaxMasterCommitSN;
}

smSN rprSNMapMgr::getMaxReplicatedSN()
{ 
    if((mMaxReplicatedSN == SM_SN_NULL) && 
       (IDU_LIST_IS_EMPTY(&mSNMap) != ID_TRUE))
    {
        setMaxSNs();
    }
    return mMaxMasterCommitSN;
}
