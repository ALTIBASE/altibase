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
 * $Id: smaDeleteThread.h 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#ifndef _O_SMA_DELETE_THREAD_H_
#define _O_SMA_DELETE_THREAD_H_ 1

#include <idl.h>
#include <idu.h>
#include <idu.h>
#include <idtBaseThread.h>
#include <smxTrans.h>
#include <smaLogicalAger.h>
#include <smaDef.h>

class smaDeleteThread : public idtBaseThread
{
//For Operation
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();
    static IDE_RC shutdownAll();

    // 특정 Table안의 OID나 특정 Tablespace안의 OID에 대해서만
    // 즉시 Aging을 실시한다.
    static IDE_RC deleteInstantly( smaInstantAgingFilter * aAgingFilter );

    static IDE_RC processJob( smxTrans    * aTransPtr,
                              smxOIDInfo  * aOIDInfoPtr,
                              idBool        aDeleteAll,
                              idBool      * aIsProcessed,
                              smSCN         aSCN );

    IDE_RC initialize( UInt aThreadID );
    IDE_RC destroy();

    virtual void run();
    IDE_RC  shutdown();
    IDE_RC  realDelete( idBool aDeleteAll, SInt aListN );

    static IDE_RC  lock() { return mMutex.lock( NULL ); }
    static IDE_RC  unlock() { return mMutex.unlock(); }

    static IDE_RC  lockAll();
    static IDE_RC  unlockAll();

    IDE_RC  lockCheckMtx() { return mCheckMutex.lock( NULL ); }
    IDE_RC  unlockCheckMtx() { return mCheckMutex.unlock(); }
    
    static void waitForNoAccessAftDropTbl();
    static IDE_RC processFreeSlotPending(
        smxTrans   *aTrans,
        smxOIDList *aOIDList);

    smaDeleteThread();

private:
    static inline idBool isAgingTarget( smaOidList  *aOIDHead,
                                        idBool       aDeleteAll,
                                        smSCN       *aMinViewSCN );

    IDE_RC processAgingOIDNodeList( smaOidList *aOIDHead,
                                    idBool      aDeleteAll );

     inline void beginATrans();
     inline void commitATrans();
     inline void commitAndBeginATransIfNeed();
    static inline IDE_RC freeOIDNodeListHead( smaOidList *aOIDHead );

//For Member
public:
    static ULong            mHandledCnt;
    static UInt             mThreadCnt;
    static smaDeleteThread *mDeleteThreadList;
    static smxTrans       **mTrans;
    /* BUG-47367 Instant Aging을 위한 Self Transaction */
    static smxTrans        *mTrans4Self;
    ULong                   mAgingCntAfterATransBegin;
    
    /* BUG-17417 V$Ager정보의 Add OID갯수는 실제 Ager가
     *           해야할 작업의 갯수가 아니다.
     *
     * mAgingProcessedOIDCnt 추가함.  */

    /* Ager가 OID List의 OID를 하나씩 처리하는데 이때 1식증가 */
    static ULong  mAgingProcessedOIDCnt;

    UInt                mTimeOut;
    idBool              mFinished;
    idBool              mHandled;

    UInt                mThreadID;

    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부하 및 Aging이
    //                  밀리는 현상이 더 심화됨
    // getMinSCN했을때, MinSCN때문에 작업하지 못한 횟수
    static ULong        mSleepCountOnAgingCondition;
private:
    static iduMutex         mMutex;
    static iduMutex         mCheckMutex4Self;

    iduMutex                mCheckMutex;

    static iduMutex *       mListLock;
    static UInt             mIsParallelMode;
};

/*
 * aOIDHead가 Aging대상인지 조사한다. 
 *
 * aOIDHead     - [IN] Aging대상 OID List의 Head
 * aDeleteAll   - [IN] ID_TRUE이면 무조건 Aging 대상으로 처리한다.
 * aMinViewSCN  - [IN] Transaction Minimum SCN
 */
idBool smaDeleteThread::isAgingTarget( smaOidList  *aOIDHead,
                                       idBool       aDeleteAll,
                                       smSCN       *aMinViewSCN )
{
    if( aOIDHead != NULL )
    {
        if( aDeleteAll == ID_FALSE )
        {
            if( aOIDHead->mErasable == ID_FALSE )
            {
                return ID_FALSE;
            }

            /* Aging Node List에 있는 Aging대상 Row를 다른 Transaction이
               볼수 있는지 조사한다. Logical Ager가 Index에서 해당 Row를
               지우는데, 해당 Row가 Index에서 지웠다고 해서 바로 Row를 Table에서
               지울수가 없다. 왜냐하면 Index 탐색시 Latch를 잡지 않고 하기 때문에
               Row를 Index Node를 통해서 접근할수 있기때문이다.
               이때문에 Node Header에 Index에서 삭제작업을 완료후에 mKeyFreeSCN을
               설정하고 Transaction의 Minimum View SCN과 비교해서 결코 접근하지
               않는다는 것을 조사하기 위해 두가지 조건을 검사한다.

               1. mKeyFreeSCN < MinViewSCN

               위 조건을 만족한다면 Aging을 수행할 수가 있다.

            */
            /* BUG-18343 읽고 있는 Row가 Ager에 의해서 삭제되고 있습니다.
             *
             * mKeyFreeSCN: OID List Header의 Key Free SCN
             *
             * smxTransMgr::getMemoryMinSCN에서 구해진
             * MinViewSCN:  Minimum View SCN
             * MinViewTID:  MinViewSCN을 가지는 Transaction ID.
             *
             * Delete Thread에서는 OID List Header의 Key Free SCN이 MIN SCN보다
             * 작으면 Aging을 수행하도록 고쳤다. 이전에는
             *    1. mKeyFreeSCN < MinViewSCN
             *    2. mKeyFreeSCN = MinViewSCN, MinViewTID = 0
             * 두가지 조건중 하나만 만족하면 Aging을 수행하도록 하였다. 그러나
             * MinViewTID가 0일때 임의의 트랜잭션도 MinViewSCN을 자신의 ViewSCN을 가질수 있다.
             * 왜냐면 smxTransMgr::getMemoryMinSCN에서 현재 system scn과 Transaction의
             * min scn중 작은 것을 선택하는데 만약 같으면 MinViewTID가 0이 된다. 이때 Aging을
             * 수행하게 되면 Transaction이 check하고 있는 Row에 대해서 Aging이 발생하게 된다.
             * 때문에 위 조건을
             *    1. mKeyFreeSCN < MinViewSCN
             * 인 경우로만 변경한다.
             */
            if( SM_SCN_IS_LT( &( aOIDHead->mKeyFreeSCN ), aMinViewSCN ) )
            {
                return ID_TRUE;
            }
            else
            {
                /* BUG-17371  [MMDB] Aging이 밀릴경우 System에 과부하 및 Aging이
                   밀리는 현상이 더 심화됨.getMinSCN했을때, MinSCN때문에 작업하지
                   못한 횟수 */
                mSleepCountOnAgingCondition++;
                return ID_FALSE;
            }
        }

        return ID_TRUE;
    }

    return ID_FALSE;
}

/*
 * Aging Transaction을 시작한다.
 *
 */
void smaDeleteThread::beginATrans()
{
    IDE_ASSERT(
        mTrans[mThreadID]->begin( NULL,
                       ( SMI_TRANSACTION_REPL_NONE |
                         SMI_COMMIT_WRITE_NOWAIT ),
                       SMX_NOT_REPL_TX_ID )
        == IDE_SUCCESS);

    smxTrans::setRSGroupID((void*)mTrans[mThreadID], 0);

    mAgingCntAfterATransBegin = 0;

}

/*
 * Aging Transaction을 Commit한다.
 *
 */
void smaDeleteThread::commitATrans()
{
    IDE_ASSERT( mTrans[mThreadID]->commit() == IDE_SUCCESS );
}

/*
 * Aging을 처리한 횟수가 smuProperty::getAgerCommitInterval
 * 보다 크면 Aging Transaction을 Commit하고 새로 시작한다.
 *
 */
void smaDeleteThread::commitAndBeginATransIfNeed()
{
    if( mAgingCntAfterATransBegin >
        smuProperty::getAgerCommitInterval() )
    {
        commitATrans();
        beginATrans();
    }
}

/*
 * aOIDHead가 가리키는 OID List Head를 Free한다.
 *
 * aOIDHead - [IN] OID List Header
 */
IDE_RC smaDeleteThread::freeOIDNodeListHead( smaOidList *aOIDHead )
{
    smmSlot *sSlot;
    SInt     sListN;

    sSlot = (smmSlot*)aOIDHead;
    sSlot->next = sSlot;
    sSlot->prev = sSlot;
    sListN      = aOIDHead->mListN;

    IDE_TEST( smaLogicalAger::mSlotList[sListN].releaseSlots(
                  1,
                  sSlot,
                  SMM_SLOT_LIST_MUTEX_ACQUIRE )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif
