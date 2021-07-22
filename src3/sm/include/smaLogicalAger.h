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
 * $Id: smaLogicalAger.h 88423 2020-08-26 04:06:55Z emlee $
 **********************************************************************/

#ifndef _O_SMA_LOGICAL_AGER_H_
# define _O_SMA_LOGICAL_AGER_H_ 1


# include <idu.h>
# include <idtBaseThread.h>

# include <smDef.h>
# include <smm.h>
# include <smaDef.h>

class smaLogicalAger : public idtBaseThread
{
    friend class smaDeleteThread;
    friend class smaFT;

 private:
    idBool       mContinue;
    UInt         mTimeOut;
    UInt         mThreadID;

    static smmSlotList* mSlotList;
    static iduMutex     mBlock;
    static iduMutex     mCheckMutex;
    static smaOidList** mHead;
    static smaOidList** mTailLogicalAger;
    static smaOidList** mTailDeleteThread;
    static iduMutex  *  mListLock;

    static smaLogicalAger *mLogicalAgerList;
    // 생성된 Logical Ager Thread의 갯수
    static UInt            mCreatedThreadCount;
    
    // 현재 수행중인 Logical Ager Thread의 갯수
    static UInt            mRunningThreadCount;

    // Ager가 초기화 되었는지 여부
    static idBool          mIsInitialized;

    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부화 및 Aging이
    //                 밀리는 현상이 더 심화됨
    // getMinSCN 했을때, MinSCN 때문에 작업하지 못한 횟수
    static ULong           mSleepCountOnAgingCondition;

    /* Logical Ager Thread갯수를 변경하려는 Thread가
     * 획득해야 하는 Mutex
     */
    static iduMutex        mAgerCountChangeMutex;

    /* BUG-17474 Index Free Node List에 대한 동시성 제어가 되고 있지
     *           않습니다. */
    static iduMutex        mFreeNodeListMutex;
    /* TASK-4990 changing the method of collecting index statistics
     * FreeNode를 막는다. Node재활용을 막아서 Scan을 편리하게 하기 위함이다.*/
    static SInt            mBlockFreeNodeCount;

    /* BUG-35179 Add property for parallel logical ager */
    static UInt            mIsParallelMode;

    /* BUG-35179 Add property for parallel logical ager 
     * drop table, drop tablespace와의 동시성 문제를 해결하기 위해 각 thread별로
     * lock을 추가한다. */
    iduMutex               mWaitForNoAccessAftDropTblLock;

    smSCN                  mViewSCN;

public:

    static UInt         mListCnt;

    /* Transaction이 Commmit/Abort시 Aging List를 넘기는데
     * 이때 1식증가 */
    static ULong  mAddCount;
    /* Ager가 List를 하나씩 처리하는데 이때 1식증가 */
    static ULong  mHandledCount;

    /* BUG-17417 V$Ager정보의 Add OID갯수는 실제 Ager가
     *           해야할 작업의 갯수가 아니다.
     *
     * mAgingRequestOIDCnt, mAgingProcessedOIDCnt
     * 추가함.  */

    /* Transaction이 Commmit/Abort시 OID List에 Aging을 수행해야할
     * OID갯수가 더해진다.*/
    static ULong  mAgingRequestOIDCnt;

    /* Ager가 OID List의 OID를 하나씩 처리하는데 이때 1식증가 */
    static ULong  mAgingProcessedOIDCnt;

public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();
    static IDE_RC shutdownAll();
    
    smaLogicalAger();
    
    IDE_RC initialize( UInt aThreadID );
    
    IDE_RC destroy( void );
    
    IDE_RC shutdown( void );
    
    static IDE_RC setAger( idBool aValue);

    // Ager가 초기화되었는지 여부를 리턴한다.
    static idBool isInitialized( void );
    
    static IDE_RC block( void );
    static IDE_RC unblock( void );

    static IDE_RC lock() { return mBlock.lock( NULL /* idvSQL* */); }
    static IDE_RC unlock() { return mBlock.unlock(); }
    
    static IDE_RC lockChangeMtx() { return mAgerCountChangeMutex.lock( NULL /* idvSQL* */); }
    static IDE_RC unlockChangeMtx() { return mAgerCountChangeMutex.unlock(); }

    static IDE_RC addList( smTID         aTID,
                           idBool        aIsDDL,
                           smSCN       * aSCN,
                           UInt          aCondition,
                           smxOIDList  * aList,
                           void       ** aAgerListPtr );

    static void setOIDListFinished( void* aOIDList, idBool aFlag);

    // Aging할 OID를 가리키는 Index Slot을 제거한다.
    static IDE_RC deleteIndex( smaOidList            * aOIDList,
                               UInt                  * aDidDelOldKey );
    static IDE_RC deleteIndexInstantly( smaOidList            * aOIDList,
                                        smaInstantAgingFilter * aAgingFilter,
                                        UInt                  * aDidDelOldKey );

    // Instant Aging Filter를 초기화한다 ( smaDef.h 참고 )
    static void initInstantAgingFilter( smaInstantAgingFilter * sAgingFilter );

    // Aging Filter조건에 부합하는지 체크
    static idBool isAgingFilterTrue( smaInstantAgingFilter * aAgingFilter,
                                     scSpaceID               aTBSID,
                                     smOID                   aTableOID );
    
    
    // 특정 Tablespace에 속한 OID에 대해서 즉시 Aging실시
    static IDE_RC doInstantAgingWithTBS( scSpaceID aTBSID );
    
    // 특정 Table에 속한 OID에 대해서 즉시 Aging실시
    static IDE_RC doInstantAgingWithTable( smOID aTableOID );
    
    // smaPhysicalAger제거에 따른 추가함수들.
    static void addFreeNodes(void*      aFreeNodeList,
                             smnNode*   aFreeNodes);
    /* TASK-4990 changing the method of collecting index statistics
     * FreeNode를 막는다. Node재활용을 막아서 TreeLatch 없이
     * Node를 FullScan 하기 위함이다. 
     * 다만 동시에 RebuildStat이 여럿 수행될 수 있으므로,
     * Block 여부를 Count로 조회한다. */
    static void blockFreeNode()
    {
        IDE_ASSERT( mBlockFreeNodeCount >= 0 );
        IDE_ASSERT( mFreeNodeListMutex.lock( NULL ) == IDE_SUCCESS );
        mBlockFreeNodeCount ++;
        IDE_ASSERT( mFreeNodeListMutex.unlock() == IDE_SUCCESS );
    }

    static void unblockFreeNode()
    {
        IDE_ASSERT( mFreeNodeListMutex.lock( NULL ) == IDE_SUCCESS );
        mBlockFreeNodeCount --;
        IDE_ASSERT( mFreeNodeListMutex.unlock() == IDE_SUCCESS );
        IDE_ASSERT( mBlockFreeNodeCount >= 0 );
    }

    
    static void freeNodesIfPossible();
    
    static IDE_RC freeOldKeyFromIndexes(smcTableHeader*  aTableHeader,
                                        smOID            aOldVersionRowOID,
                                        UInt*            aDidDelOldKey);
    
    static IDE_RC checkOldKeyFromIndexes(smcTableHeader * aTableHeader,
                                         smOID            aOldVersionRowOID );
    
    static void waitForNoAccessAftDropTbl();
    
    virtual void run();

    /*********************************************************************
     * Description : aOID가 dummy OID인지 묻는다.
     *
     *     BUG-15306을 위해 추가된 함수임
     *********************************************************************/
    static inline idBool isDummyOID( smaOidList* aOID )
    {
#ifdef DEBUG
        if( aOID->mCondition == 0 ) 
        {
            IDE_DASSERT( SM_SCN_IS_INIT( aOID->mSCN ) );
        }
        else
        {
            IDE_DASSERT( (aOID->mCondition == SM_OID_ACT_AGING_COMMIT)   ||
                         (aOID->mCondition == SM_OID_ACT_AGING_ROLLBACK) ); 
            IDE_DASSERT( SM_SCN_IS_NOT_INIT( aOID->mSCN ) );
        }
#endif
        return ( (aOID->mCondition == 0) ? ID_TRUE : ID_FALSE );
    };

    static void addAgingRequestCnt( ULong aAgingRequestCnt )
    {
        (void)idCore::acpAtomicAdd64( &mAgingRequestOIDCnt,
                                      aAgingRequestCnt );
    };

#ifdef DEBUG
    static idBool checkAgingProcessedCnt()
    {
        /* 순서가 중요, 둘 다 증가하는 값이므로, Request Count를 먼저 읽어버리면
         * 이 후 증가해버린 Processed Count 를 읽게 될 수 있다.
         * Processed Count를 먼저 읽어야 한다.*/
        ULong sProcessCnt = idCore::acpAtomicGet64( &mAgingProcessedOIDCnt );
        ULong sRequestCnt = idCore::acpAtomicGet64( &mAgingRequestOIDCnt );

        return ( sProcessCnt <= sRequestCnt ) ? ID_TRUE : ID_FALSE ;
    }
#endif

    // Ager Thread의 갯수를 특정 갯수가 되도록 생성,제거한다.
    static IDE_RC changeAgerCount( UInt aAgerCount );

private:
    // Ager Thread를 하나 추가한다.
    static IDE_RC addOneAger();
    
    // Ager Thread를 하나 제거한다.
    static IDE_RC removeOneAger();

    static IDE_RC addDummyOID( SInt aListN );

    // 특정 Table안의 OID나 특정 Tablespace안의 OID에 대해
    // 즉시 Aging을 실시한다.
    static IDE_RC agingInstantly( smaInstantAgingFilter * aAgingFilter );

    /* BUG-35179 Add property for parallel logical ager 
     * drop table, drop tablespace와의 동시성 문제를 해결하기 위해 각 thread별로
     * lock을 추가한다. */

    inline IDE_RC lockWaitForNoAccessAftDropTbl()
    {
        return mWaitForNoAccessAftDropTblLock.lock( NULL /* idvSQL* */ );
    }

    inline IDE_RC unlockWaitForNoAccessAftDropTbl()
    {
        return mWaitForNoAccessAftDropTblLock.unlock();
    }
};

#endif /* _O_SMA_LOGICAL_AGER_H_ */
