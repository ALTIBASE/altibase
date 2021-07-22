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
 

#ifndef _O_RPR_SNMAPMGR_H_
#define _O_RPR_SNMAPMGR_H_

#include <smDef.h>
#include <rp.h>
#include <qci.h>
#include <rpdMeta.h>

typedef struct rprSNMapEntry
{
    smSN        mMasterBeginSN;
    smSN        mMasterCommitSN;
    smSN        mReplicatedBeginSN;
    smSN        mReplicatedCommitSN;
    iduListNode mNode;
} rprSNMapEntry;

class rprSNMapMgr;
class rpxSender;

typedef struct rprRecoveryItem
{
    rpRecoveryStatus   mStatus;
    rprSNMapMgr      * mSNMapMgr;
    rpxSender        * mRecoverySender;
} rprRecoveryItem;

class rprSNMapMgr
{
public:

    rprSNMapMgr(){};
    ~rprSNMapMgr() {};

    IDE_RC initialize( const SChar * aRepName,
                       idBool        aNeedLock );

    void   destroy();

    /* 1.Local에서 디스크에 flush된 시점의 SN과 대응되는 Remote의 SN을 구해야 하며,
     *   이때, sn map을 스캔해야한다.
     * 2.양쪽 모두에서 디스크에 flush된 엔트리를 주기적으로 삭제해야하며, 이 때에도 스캔이 필요하다.
     * 스캔 횟수를 줄이기 위해 이 두 가지 작업을 한번 스캔시 함께 하도록 한다.
     * 1번 작업을 위해, local flush SN이 필요하며, 2번 작업을 위해 local/remote flush sn이 필요하다.
     * 그래서 양쪽 모두에서 디스크에 flush된 SN을 인자로 받는다.
     */
    void   getLocalFlushedRemoteSN(smSN   aLocalFlushSN, 
                                   smSN   aRemoteFlushSN,
                                   smSN   aRestartSN,
                                   smSN * aLocalFlushedRemoteSN);
    /*Recoery Sender가 로그를 읽은 후 해당 트랜잭션이 Recovery해야하는지 확인 함*/
    idBool needRecoveryTx(smSN aBeginLogSN);

    void   setMaxSNs();
    smSN   getMaxMasterCommitSN();
    smSN   getMinReplicatedSN();
    smSN   getMaxReplicatedSN();

    void   getFirstEntrySNsNDelete(rpdRecoveryInfo * aRecoveryInfos);
    void   getFirstEntrySN(rpdRecoveryInfo * aRecoveryInfos);
    IDE_RC insertNewTx(smSN aMasterBeginSN, rprSNMapEntry **aNewEntry);
    IDE_RC insertEntry(rpdRecoveryInfo * aRecoveryInfo);
    void   deleteTxByEntry(rprSNMapEntry * aEntry);
    void   deleteTxByReplicatedCommitSN(smSN     aReplicatedCommitSN,
                                        idBool * aIsExist);

    void   setSNs(rprSNMapEntry * aEntry,
                  smSN            aMasterCommitSN,
                  smSN            aReplicatedBeginSN,
                  smSN            aReplicatedCommitSN);

    UInt   refineSNMap(smSN aActiveRPRecoverySN);

    inline idBool isYou(const SChar * aRepName )
    {
        return ( idlOS::strcmp( mRepName, aRepName ) == 0 )
            ? ID_TRUE : ID_FALSE;
    };

    inline idBool isEmpty(){ return IDU_LIST_IS_EMPTY(&mSNMap); };

    SChar           mRepName[QC_MAX_OBJECT_NAME_LEN + 1];

    smSN            mMaxReplicatedSN;
private:

    iduMemPool      mEntryPool;
    iduList         mSNMap;
    /* SN Map Mgr Mutex는 check point thread와 receiver가 동시에 리스트에 접근하는 것을 방지 하기 위해 유지한다.
     * checkpoint thread는 checkpoint시 리스트의 처음 엔트리를 검색하며, receiver는 리스트를 수정 및 검색한다. 
     * 리스트를 수정하는 것은 receiver와 executor가 배타적으로 수행하므로, 
     * receiver/executor는 리스트를 갱신 할 때에만 lock을 잡도록 하여 불필요한 lock을 잡지 않도록 한다.
     * checkpoint thread는 검색 시 lock을 잡아서, checkpoint thread가 잘못된 값을 읽지 않도록 한다.
     */
    iduMutex        mSNMapMgrMutex;
    smSN            mMaxMasterCommitSN; //recovery해야 하는 지에 대한 여부를 빠르게 감지하기 위해 유지함
};

#endif //_O_RPR_SNMAPMGR_H_
