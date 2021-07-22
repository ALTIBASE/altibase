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
 * $Id: smlLockMgrSpin.cpp 83745 2018-08-21 03:20:06Z emlee $
 **********************************************************************/
/**************************************************************
 * FILE DESCRIPTION : smlLockMgr.cpp                          *
 * -----------------------------------------------------------*
 이 모듈에서 제공하는 기능은 다음과 크게 4가지이다.

 1. lock table
 2. unlock table
 3. record lock처리
 4. dead lock detection


 - lock table
  기본적으로 table의 대표락과 지금 잡고자 하는 락과 호환가능하면,
  grant list에 달고 table lock을 잡게 되고,
  lock conflict이 발생하면 table lock대기 리스트인 request list
  에 달게 되며, lock waiting table에 등록하고 dead lock검사후에
  waiting하게 된다.

  altibase에서는 lock  optimization을 다음과 같이 하여,
  코드가 복잡하게 되었다.

  : grant lock node 생성을 줄이기 위하여  lock node안의
    lock slot도입및 이용.
    -> 이전에 트랜잭션이  table에 대하여 lock을 잡았고,
      지금 요구하는 table lock mode가 호환가능하면, 새로운
      grant node를 생성하고 grant list에 달지 않고,
      기존 grant node의 lock mode만 conversion하여 갱신한다.
    ->lock conflict이지만,   grant된 lock node가 1개이고,
      그것이 바로 그  트랙잭션일 경우, 기존 grant lock node의
      lock mode를 conversion하여 갱신한다.

  : unlock table시 request list에 있는 node를
    grant list으로 move되는 비용을 줄이기 위하여 lock node안에
    cvs lock node pointer를 도입하였다.
    -> lock conflict 이고 트랜잭션이 이전에 table에 대하여 grant된
      lock node를 가지고 있는 경우, request list에 달 새로운
      lock node의 cvs lock를 이전에 grant lock node를 pointing
      하게함.
   %나중에 다른 트랜잭션의 unlock table시 request에 있었던 lock
   node가 새로 갱신된 grant mode와 호환가능할때 , 이 lock node를
   grant list으로 move하는 대신, 기존 grant lock node의 lock mode
   만 conversion한다.

 - unlock table.
   lock node가 grant되어 있는 경우에는 다음과 같이 동작한다.
    1> 새로운 table의 대표락과 grant lock mode를 갱신한다.
    2>  grant list에서 lock node를 제거시킨다.
      -> Lock node안에 lock slot이 2개 이상있는 경우에는
       제거안함.
   lock node가 request되어 있었던 경우에는 request list에서
   제거한다.

   request list에 있는 lock node중에
   현재 갱신된 grant lock mode와 호환가능한
   Transaction들 을 다음과 같이 깨운다.
   1.  request list에서 lock node제거.
   2.  table lock정보에서 grant lock mode를 갱신.
   3.  cvs lock node가 있으면,이 lock node를
      grant list으로 move하는 대신, 기존 grant lock node의
      lock mode 만 conversion한다.
   4. cvs lock node가 없으면 grant list에 lock node add.
   5.  waiting table에서 자신을 wainting 하고 있는 트랜잭션
       의 대기 상태 clear.
   6.  waiting하고 있는 트랜잭션을 resume시킨다.
   7.  lock slot, lock node 제거 시도.

 - waiting table 표현.
   waiting table은 chained matrix이고, 다음과 같이 표현된다.

     T1   T2   T3   T4   T5   T6

  T1                                        |
                                            | record lock
  T2                                        | waiting list
                                            |
  T3                6          USHORT_MAX   |
                                            |
  T4      6                                 |
                                            |
  T5                                        v

  T6     USHORT_MAX
    --------------------------------->
    table lock waiting or transaction waiting list

    T3은 T4, T6에 대하여 table lock waiting또는
    transaction waiting(record lock일 경우)하고 있다.

    T2에 대하여  T4,T6가 record lock waiting하고 있으며,
    T2가 commit or rollback시에 T4,T6 행의 T2열의 대기
    상태를 clear하고 resume시킨다.

 -  record lock처리
   recod lock grant, request list와 node는 없다.
   다만 waiting table에서  대기하려는 transaction A의 column에
   record lock 대기를 등록시키고, transaction A abort,commit이
   발생하면 자신에게 등록된 record lock 대기 list을 순회하며
   record lock대기상태를 clear하고, 트랜잭션들을 깨운다.


 - dead lock detection.
  dead lock dectioin은 다음과 같은 두가지 경우에
  대하여 수행한다.

   1. Tx A가 table lock시 conflict이 발생하여 request list에 달고,
     Tx A의 행의 열에 waiting list를 등록할때,waiting table에서
     Tx A에 대하여 cycle이 발생하면 transaction을 abort시킨다.


   2.Tx A가  record R1을 update시도하다가,
   다른 Tx B에 의하여 이미  active여서 record lock을 대기할때.
    -  Tx B의 record lock 대기열에서  Tx A를 등록하고,
       Tx A가 Tx B에 대기함을 기록하고 나서  waiting table에서
       Tx A에 대하여 cycle이 발생하면 transaction을 abort시킨다.


*************************************************************************/
#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smlDef.h>
#include <smDef.h>
#include <smr.h>
#include <smc.h>
#include <sml.h>
#include <smlReq.h>
#include <smu.h>
//#include <smrLogHeadI.h>
#include <sct.h>
#include <smx.h>
//#include <smxTransMgr.h>

smlLockMode smlLockMgr::getLockMode( const SLong aLock )
{
    SInt sMode;
    SInt sFlag = 0;

    for ( sMode = 0 ; sMode < SML_NUMLOCKTYPES ; sMode++ )
    {
        if ( (aLock & mLockMask[sMode]) != 0 )
        {
            sFlag |= mLockModeToMask[sMode];
        }
        else
        {
            /* continue */
        }
    }

    return mDecisionTBL[sFlag];
}

smlLockMode smlLockMgr::getLockMode( const smlLockItemSpin* aItem )
{
    return getLockMode(aItem->mLock);
}

smlLockMode smlLockMgr::getLockMode( const smlLockNode* aNode )
{
    return mDecisionTBL[aNode->mFlag];
}

SLong smlLockMgr::getGrantedCnt( const smlLockItemSpin* aItem, const smlLockMode aMode )
{
    SInt    i;
    SLong   sSum;

    if ( aMode == SML_NLOCK )
    {
        sSum = 0;
        for ( i = 0 ; i < mSpinSlotCnt ; i ++ )
        {
            sSum += (SLong)acpBitCountSet64( aItem->mOwner[i] );
        }
    }
    else
    {
        sSum = ( aItem->mLock & mLockMask[aMode]) >> mLockBit[aMode];
    }

    return sSum;
}



IDE_RC smlLockMgr::initializeSpin( UInt aTransCnt )
{
    SInt i;
    SInt j;
    IDE_ASSERT( smuProperty::getLockMgrType() == 1 );

    mTransCnt = aTransCnt;
    mSpinSlotCnt = (mTransCnt + 63) / 64;

    IDE_ASSERT( mTransCnt > 0 );
    IDE_ASSERT( mSpinSlotCnt > 0 );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       mTransCnt * mTransCnt * sizeof(SInt),
                                       (void**)&mPendingArray ) != IDE_SUCCESS,
                    insufficient_memory );
    IDE_TEST_RAISE(iduMemMgr::malloc( IDU_MEM_SM_SML,
                                      mTransCnt * sizeof(SInt*),
                                      (void**)&mPendingMatrix ) != IDE_SUCCESS,
                   insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       mTransCnt * sizeof(idBool),
                                       (void**)&mIsCycle ) != IDE_SUCCESS,
                    insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       mTransCnt * sizeof(idBool),
                                       (void**)&mIsChecked ) != IDE_SUCCESS,
                    insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       mTransCnt * sizeof(idBool),
                                       (void**)&mIsDetected ) != IDE_SUCCESS,
                    insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       mTransCnt * sizeof(SInt),
                                       (void**)&mDetectQueue ) != IDE_SUCCESS,
                    insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SML,
                                       mTransCnt,
                                       sizeof(ULong),
                                       (void**)&mSerialArray ) != IDE_SUCCESS,
                    insufficient_memory );

    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        mIsCycle[i]         = ID_FALSE;
        mIsChecked[i]       = ID_FALSE;
        mIsDetected[i]      = ID_FALSE;

        mPendingMatrix[i]   = &(mPendingArray[mTransCnt * i]);

        for ( j = 0 ; j < mTransCnt ; j++ )
        {
            mPendingMatrix[i][j] = SML_EMPTY;
        }
    }

    mStopDetect     = 0;
    mPendSerial     = 0;
    mTXPendCount    = 0;
    IDE_TEST( mDetectDeadlockThread.launch( detectDeadlockSpin, NULL )
              != IDE_SUCCESS );

    mLockTableFunc                  = lockTableSpin;
    mUnlockTableFunc                = unlockTableSpin;
    mInitLockItemFunc               = initLockItemSpin;
    mAllocLockItemFunc              = allocLockItemSpin;
    mDidLockReleasedFunc            = didLockReleasedSpin;
    mRegistRecordLockWaitFunc       = registLockWaitSpin;
    mFreeAllRecordLockFunc          = freeAllRecordLockSpin;
    mClearWaitItemColsOfTransFunc   = clearWaitItemColsOfTransSpin;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smlLockMgr::destroySpin()
{
    (void)acpAtomicSet32( &mStopDetect, 1 );
    IDE_TEST( mDetectDeadlockThread.join()      != IDE_SUCCESS );
    IDE_TEST( iduMemMgr::free( mPendingArray  ) != IDE_SUCCESS );
    mPendingArray = NULL;
    IDE_TEST( iduMemMgr::free( mPendingMatrix ) != IDE_SUCCESS );
    mPendingMatrix =  NULL;
    IDE_TEST( iduMemMgr::free( mIsCycle       ) != IDE_SUCCESS );
    mIsCycle = NULL;
    IDE_TEST( iduMemMgr::free( mIsChecked     ) != IDE_SUCCESS );
    mIsChecked = NULL; 
    IDE_TEST( iduMemMgr::free( mIsDetected    ) != IDE_SUCCESS );
    mIsDetected = NULL; 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smlLockMgr::allocLockItemSpin( void ** aLockItem )
{
    ssize_t sAllocSize;

    sAllocSize = sizeof(smlLockItemSpin) 
                 + ( (ID_SIZEOF(ULong) * mSpinSlotCnt) )
                 - ID_SIZEOF(ULong);

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       sAllocSize,
                                       aLockItem ) != IDE_SUCCESS,
                    insufficient_memory );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smlLockMgr::initLockItemSpin( scSpaceID          aSpaceID,
                                     ULong              aItemID,
                                     smiLockItemType    aLockItemType,
                                     void             * aLockItem )
{
    smlLockItemSpin*    sLockItem;
    SInt                i;

    IDE_TEST( initLockItemCore( aSpaceID, 
                                aItemID,
                                aLockItemType, 
                                aLockItem )
            != IDE_SUCCESS );

    sLockItem = (smlLockItemSpin*) aLockItem;

    sLockItem->mLock = 0;
    for ( i = 0 ; i < SML_NUMLOCKTYPES ; i++ )
    {
        sLockItem->mPendingCnt[i] = 0;
    }
    idlOS::memset( sLockItem->mOwner, 0, sizeof(ULong) * mSpinSlotCnt );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smlLockMgr::lockTableSpin( SInt          aSlot,
                                  smlLockItem  *aLockItem,
                                  smlLockMode   aLockMode,
                                  ULong         aLockWaitMicroSec,
                                  smlLockMode  *aCurLockMode,
                                  idBool       *aLocked,
                                  smlLockNode **aLockNode,
                                  smlLockSlot **aLockSlot,
                                  idBool        aIsExplicit )
{
    smlLockItemSpin*    sLockItem = (smlLockItemSpin*)aLockItem;
    smlLockNode*        sCurTransLockNode = NULL;
    smlLockSlot*        sNewSlot;
    idvSQL*             sStat;
    idvSession*         sSession;
    UInt                sLockEnable = 1;

    SLong       sOldLock;
    SLong       sCurLock;
    SLong       sNewLock;
    SLong       sLockDelta;
    smlLockMode sOldMode;
    smlLockMode sCurMode;
    smlLockMode sNewMode;
    idBool      sIsNewLock = ID_FALSE;
    idBool      sDone;
    SLong       sLoops = 0;
    SInt        sIndex;
    SInt        sBitIndex;
    SInt        i;
    SInt        sWaitTransSlot;
    smxTrans  * sTrans;

    SInt        sSleepTime = smuProperty::getLockMgrMinSleep();
    acp_time_t  sBeginTime = acpTimeNow();
    acp_time_t  sCurTime;

    ULong       sOwner;
    ULong       sOwnerDelta;
    SInt        sOwnerIndex;

    PDL_Time_Value      sTimeOut;

    /* BUG-32237 [sm_transaction] Free lock node when dropping table.
     * DropTablePending 에서 연기해둔 freeLockNode를 수행합니다. */
    /* 의도한 상황은 아니기에 Debug모드에서는 DASSERT로 종료시킴.
     * 하지만 release 모드에서는 그냥 rebuild 하면 문제 없음. */
    if ( sLockItem == NULL )
    {
        IDE_DASSERT( 0 );

        IDE_RAISE( error_table_modified );
    }
    else
    {
        /* nothing to do... */
    }

    sStat = smLayerCallback::getStatisticsBySID( aSlot );

    sSession = ( ( sStat == NULL ) ? NULL : sStat->mSess );

    // To fix BUG-14951
    // smuProperty::getTableLockEnable은 TABLE에만 적용되어야 한다.
    // (TBSLIST, TBS 및 DBF에는 해당 안됨)
    /* BUG-35453 -  add TABLESPACE_LOCK_ENABLE property
     * TABLESPACE_LOCK_ENABLE 도 TABLE_LOCK_ENABLE 과 동일하게
     * tablespace lock에 대해 처리한다. */
    switch ( sLockItem->mLockItemType )
    {
        case SMI_LOCK_ITEM_TABLE:
            sLockEnable = smuProperty::getTableLockEnable();
            break;

        case SMI_LOCK_ITEM_TABLESPACE:
            sLockEnable = smuProperty::getTablespaceLockEnable();
            break;

        default:
            break;
    }

    if ( sLockEnable == 0 )
    {
        if ( ( aLockMode != SML_ISLOCK ) && ( aLockMode != SML_IXLOCK ) )
        {
            if ( sLockItem->mLockItemType == SMI_LOCK_ITEM_TABLESPACE )
            {
                IDE_RAISE( error_lock_tablespace_use );
            }
            else
            {
                IDE_RAISE( error_lock_table_use );
            }
        }
        else
        {
            /* nothing to do */ 
        }

        if ( aLocked != NULL )
        {
            *aLocked = ID_TRUE;
        }
        else
        {
            /* nothing to do */ 
        }

        if ( aLockSlot != NULL )
        {
            *aLockSlot = NULL;
        }
        else
        {
            /* nothing to do */ 
        }

        if ( aCurLockMode != NULL )
        {
            *aCurLockMode = aLockMode;
        }
        else
        {
            /* nothing to do */ 
        }

        return IDE_SUCCESS;
    }
    else  /* sLockEnable != 0 */
    {
        /* nothing to do */ 
    } 

    if ( aLocked != NULL )
    {
        *aLocked = ID_TRUE;
    }
    else
    {
        /* nothing to do */ 
    }

    if ( aLockSlot != NULL )
    {
        *aLockSlot = NULL;
    }
    else
    {
        /* nothing to do */ 
    }

    // 트랜잭션이 이전 statement에 의하여,
    // 현재 table A에 대하여 lock을 잡았던 lock node를 찾는다.
    sCurTransLockNode = findLockNode( sLockItem,aSlot );
    // case 1: 이전에 트랜잭션이  table A에  lock을 잡았고,
    // 이전에 잡은 lock mode와 지금 잡고자 하는 락모드 변환결과가
    // 같으면 바로 return!
    if ( sCurTransLockNode != NULL )
    {
        sIsNewLock = ID_FALSE;

        if ( mConversionTBL[getLockMode(sCurTransLockNode)][aLockMode]
             == getLockMode( sCurTransLockNode ) )
        {
            /* PROJ-1381 Fetch Across Commits
             * 1. IS Lock이 아닌 경우
             * 2. 이전에 잡으려는 Lock Mode로 Lock을 잡은 경우
             * => Lock을 잡지 않고 바로 return 한다. */
            if ( ( aLockMode != SML_ISLOCK ) ||
                 ( sCurTransLockNode->mArrLockSlotList[aLockMode].mLockSequence != 0 ) )
            {
                if ( aCurLockMode != NULL )
                {
                    *aCurLockMode = getLockMode( sCurTransLockNode );
                }
                else
                {
                    /* nothing to do */ 
                }

                IDV_SESS_ADD( sSession, IDV_STAT_INDEX_LOCK_ACQUIRED, 1 );

                return IDE_SUCCESS;
            }
            else
            {
                /* nothing to do */ 
            }
        }
    }
    else /* sCurTransLockNode == NULL */
    {
        sIsNewLock = ID_TRUE;

        /* allocate lock node and initialize */
        IDE_TEST( allocLockNodeAndInit( aSlot,
                                        aLockMode,
                                        sLockItem,
                                        &sCurTransLockNode,
                                        aIsExplicit )
                  != IDE_SUCCESS );

        sCurTransLockNode->mBeGrant = ID_FALSE;
        sCurTransLockNode->mDoRemove = ID_FALSE;
        /* Add Lock Node to a transaction */
        addLockNode( sCurTransLockNode, aSlot );
    }

    IDE_EXCEPTION_CONT(BEGIN_LOOP);
    sCurLock = sLockItem->mLock;
    sCurMode = getLockMode(sCurLock);

    if ( mCompatibleTBL[sCurMode][aLockMode] == ID_TRUE )
    {
        IDE_TEST_CONT( getGrantedCnt( sLockItem, aLockMode ) >= mLockMax[aLockMode],
                       TRY_TIMEOUT );
        for ( i = 0 ; i < SML_NUMLOCKTYPES ; i++ )
        {
            if ( mPriority[i] == aLockMode )
            {
                break;
            }
            IDE_TEST_CONT( sLockItem->mPendingCnt[mPriority[i]] != 0, TRY_TIMEOUT );
        }

        if ( ( sIsNewLock == ID_TRUE ) || ( aLockMode == SML_ISLOCK ) )
        {
            sOldMode    = SML_NLOCK;
            sNewMode    = aLockMode;
            sLockDelta  = mLockDelta[aLockMode];
        }
        else
        {
            sOldMode    = getLockMode(sCurTransLockNode);
            sNewMode    = mConversionTBL[sCurMode][aLockMode];
            sLockDelta  = mLockDelta[sNewMode] - mLockDelta[sOldMode];
        }

        do
        {
            sNewLock = sCurLock + sLockDelta;
            sOldLock = acpAtomicCas64( &(sLockItem->mLock), sNewLock, sCurLock );

            if ( sOldLock == sCurLock )
            {
                /* CAS Successful, lock held */
                sDone = ID_TRUE;
            }
            else
            {
                sCurLock = sOldLock;
                sCurMode = getLockMode(sCurLock);

                IDE_TEST_CONT( mCompatibleTBL[sCurMode][aLockMode] == ID_FALSE,
                               TRY_TIMEOUT );
        
                sDone = ID_FALSE;
            }
        } while ( sDone == ID_FALSE );
    }
    else if ( ( getGrantedCnt(sLockItem) == 1 ) && ( sIsNewLock == ID_FALSE ) )
    {
        /* ---------------------------------------
         * lock conflict이지만, grant된 lock node가 1개이고,
         * 그것이 바로 그  트랙잭션일 경우에는 request list에 달지 않고
         * 기존 grant된  lock node의 lock mode와 table의 대표락,
         * grant lock mode를  갱신한다.
         * --------------------------------------- */
        if ( aLockMode == SML_ISLOCK )
        {
            sOldMode = SML_NLOCK;
            sNewMode = SML_ISLOCK;
            sNewLock = sCurLock + mLockDelta[SML_ISLOCK];
        }
        else
        {
            sOldMode = getLockMode(sCurTransLockNode);
            sNewMode = mConversionTBL[sCurMode][aLockMode];
            sNewLock = sCurLock + mLockDelta[sNewMode] - mLockDelta[sOldMode];
        }
        sOldLock = acpAtomicCas64( &(sLockItem->mLock), sNewLock, sCurLock );

        IDE_TEST_CONT( sOldLock != sCurLock, TRY_TIMEOUT );
    }
    else
    {
        IDE_CONT(TRY_TIMEOUT);
    }

    sCurTransLockNode->mLockCnt++;

    setLockModeAndAddLockSlotSpin( aSlot, 
                                   sCurTransLockNode, 
                                   aCurLockMode,
                                   sNewMode, 
                                   ID_TRUE, 
                                   &sNewSlot,
                                   sOldMode, 
                                   sNewMode );

    if ( aLockNode != NULL )
    {
        *aLockNode = sCurTransLockNode;
    }
    else
    {
        /* nothing to do */ 
    }

    if ( aLockSlot != NULL )
    {
        *aLockSlot = sNewSlot;
    }
    else
    {
        /* nothing to do */ 
    }

    if ( sLoops != 0 )
    {
        (void)acpAtomicDec32( &(sLockItem->mPendingCnt[aLockMode]) );
        clearWaitItemColsOfTransSpin( ID_FALSE, aSlot );
        decTXPendCount();
        smxTransMgr::setTransBegunBySID(aSlot);
    }
    else
    {
        /* nothing to do */ 
    }

    IDE_DASSERT( sLockItem->mLock != 0 );

    if ( sIsNewLock == ID_TRUE )
    {
        calcIndex(aSlot, sIndex, sBitIndex);
        (void)acpAtomicAdd64( &(sLockItem->mOwner[sIndex]),
                              ID_ULONG(1) << sBitIndex );
        sCurTransLockNode->mBeGrant = ID_TRUE;
    }
    else
    {
        /* nothing to do */ 
    }
                
    IDV_SESS_ADD(sSession, IDV_STAT_INDEX_LOCK_ACQUIRED, 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT(TRY_TIMEOUT);
    {
        if ( sLoops == 0 )
        {
            /* RP인 경우 Lock Wait 시간이 달라진다. 
             * Loop = 0일때만 Setting 하여 중복 Setting 을 피한다. */
            sTrans            = (smxTrans *)smLayerCallback::getTransBySID( aSlot );
            aLockWaitMicroSec = sTrans->getLockTimeoutByUSec( aLockWaitMicroSec );
        }

        /* check timeout */
        sCurTime = acpTimeNow();
        IDE_TEST_RAISE( (ULong)(sCurTime - sBeginTime) > aLockWaitMicroSec,
                        err_exceed_wait_time );
        IDE_TEST( iduCheckSessionEvent( sStat ) != IDE_SUCCESS );

        if ( sLoops == 0 )
        {
            /* Increase pending count */
            (void)acpAtomicInc32( &(sLockItem->mPendingCnt[aLockMode]) );
            smxTransMgr::setTransBlockedBySID( aSlot );
            beginPending( aSlot );
            incTXPendCount();
        }
        else
        {
            /* Another loop */
        }

        for ( i = 0 ; i < mSpinSlotCnt ; i++ )
        {
            sOwner = sLockItem->mOwner[i];

            while ( sOwner != 0 )
            {
                sOwnerIndex  = acpBitFfs64(sOwner);
                sWaitTransSlot = ( i * 64 ) + sOwnerIndex;

                if ( sWaitTransSlot != aSlot )
                {
                    sOwnerDelta = ID_ULONG(1) << sOwnerIndex;

                    registLockWaitSpin( aSlot, sWaitTransSlot );
                    sOwner &= ~sOwnerDelta;
                }
                else
                {
                    /* 자기가 lock을 획득하고 있더라도 lock 획득에 실패할수 있다.
                     * IS -> X의 경우 IS가 이미 여럿이 걸려있어서 내가 이미 IS를 가지고 있지만
                     * X를 걸기 위해 다른 IS를 가진 Tx가 끝나길 기다려야 한다.
                     * 여기서 sOwner에 자기 Tx의 Bit를 제거하지 않으면 무한루프를 돌기 때문에
                     * Bit를 제거해준다. */
                    sOwnerDelta = ID_ULONG(1) << sOwnerIndex;
                    sOwner &= ~sOwnerDelta;
                }
            }
        }

        /* Loop를 증가 시키지 않은 상태로 isCycle에서 예외 처리 되면
         * PendingCnt가 줄지 않아 다른 Tx가 HANG에 걸리는 경우가 생김. */
        /* loop to try again */
        sLoops++;

        /*
         * add myself to deadlock detection matrix
         * and check deadlock
         */
        IDE_TEST_RAISE( isCycle(aSlot) == ID_TRUE, err_deadlock );

        if ( ( sLoops % smuProperty::getLockMgrSpinCount() ) == 0 )
        {
            sTimeOut.set(0, sSleepTime);
            idlOS::sleep(sTimeOut);
            sSleepTime = IDL_MIN(sSleepTime * 2, smuProperty::getLockMgrMaxSleep());
            /* sLoops이 계속 증가하여 Overflow로 0이 되어 
             * 두번 PendingCnt를 증가하는 경우를 막기 위함 */
            sLoops = 1;
        }
        else
        {
            idlOS::thr_yield();
        }

        /* try again */
        IDE_CONT(BEGIN_LOOP);
    }

    IDE_EXCEPTION( error_table_modified );
    {
        IDE_SET( ideSetErrorCode( smERR_REBUILD_smiTableModified ) );
    }
    IDE_EXCEPTION(error_lock_table_use);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TableLockUse));
    }
    IDE_EXCEPTION(error_lock_tablespace_use);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TablespaceLockUse));
    }
    IDE_EXCEPTION(err_deadlock);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Aborted));
    }
    IDE_EXCEPTION(err_exceed_wait_time);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_smcExceedLockTimeWait));
    }

    IDE_EXCEPTION_END;

    clearWaitItemColsOfTransSpin( ID_FALSE, aSlot );
    if ( sLoops != 0 )
    {
        (void)acpAtomicDec32( &(sLockItem->mPendingCnt[aLockMode]) );
        smxTransMgr::setTransBegunBySID(aSlot);
        decTXPendCount();
    }
    if ( sIsNewLock == ID_TRUE )
    {
        /* tried new lock but not held */
        removeLockNode( sCurTransLockNode );
        (void)freeLockNode( sCurTransLockNode );
    }

    return IDE_FAILURE;
}

IDE_RC smlLockMgr::unlockTableSpin( SInt          aSlot,
                                    smlLockNode  *aLockNode,
                                    smlLockSlot  *aLockSlot,
                                    idBool     /* aDoMutexLock */ )
{
    smlLockItemSpin*    sLockItem;
    smlLockNode*    sLockNode;
    smlLockSlot*    sLockSlot = NULL;
    smlLockSlot*    sLockSlotAfter;
    smlLockMode     sOldMode;
    smlLockMode     sNewMode;

    SLong           sLockDelta;
    SInt            sIndex;
    SInt            sBitIndex;
    SInt            i;

    idvSQL      *sStat;
    idvSession  *sSession;

    sStat = smLayerCallback::getStatisticsBySID( aSlot );

    sSession = ( (sStat == NULL) ? NULL : sStat->mSess );

    if ( aLockNode == NULL )
    {
        sLockNode = aLockSlot->mLockNode;
        sLockSlot = aLockSlot;
    
        sOldMode    = sLockSlot->mOldMode;
        sNewMode    = sLockSlot->mNewMode;
        /* check whether future lock exists */
        sLockDelta  = 0;
        for ( i = 0 ; i < SML_NUMLOCKTYPES ; i++ )
        {
            sLockSlotAfter = &(sLockNode->mArrLockSlotList[i]);

            if ( (sLockSlotAfter != sLockSlot) &&
                 (sLockSlotAfter->mOldMode == sNewMode) )
            {
                IDE_DASSERT( sLockSlotAfter->mLockSequence != 0 );
                sLockSlotAfter->mOldMode = sOldMode;

                sNewMode = sOldMode = SML_NLOCK;
                break;
            }
            else
            {
                /* continue */
            }
        }
        sLockDelta  = mLockDelta[sOldMode] - mLockDelta[sNewMode];

        removeLockSlot(sLockSlot);
        sLockNode->mFlag &= ~(sLockSlot->mMask);
        sLockNode->mLockCnt--;
    }
    else /* aLockNode == NULL */
    {
        sLockNode   = aLockNode;
        sLockDelta  = 0;

        for ( i = 0 ; i < SML_NUMLOCKTYPES ; i++ )
        {
            sLockSlot = &(sLockNode->mArrLockSlotList[i]);
            if ( sLockSlot->mLockSequence != 0 )
            {
                sOldMode    = sLockSlot->mOldMode;
                sNewMode    = sLockSlot->mNewMode;
                sLockDelta += mLockDelta[sOldMode] - mLockDelta[sNewMode];
                removeLockSlot(sLockSlot);
        
                sLockNode->mFlag &= ~(sLockSlot->mMask);
            }
            else
            {
                /* continue */
            }
        }

        IDE_DASSERT( sLockNode->mFlag == 0 );
        sLockNode->mLockCnt = 0;
    }

    IDE_ASSERT( sLockNode != NULL );
    sLockItem = (smlLockItemSpin*)sLockNode->mLockItem;
    IDE_DASSERT( sLockItem->mLock != 0 );

    calcIndex(aSlot, sIndex, sBitIndex);
    IDE_DASSERT( (sLockItem->mOwner[sIndex] & (ID_ULONG(1) << sBitIndex)) != 0 );

    if ( sLockDelta != 0 )
    {
        (void)acpAtomicAdd64( &(sLockItem->mLock), sLockDelta );
    }

    if ( sLockNode->mLockCnt == 0 )
    {
        (void)acpAtomicSub64( &(sLockItem->mOwner[sIndex] ), ID_ULONG(1) << sBitIndex);
        removeLockNode( sLockNode );
        IDE_TEST( freeLockNode(sLockNode) != IDE_SUCCESS );
    
        IDV_SESS_ADD( sSession, IDV_STAT_INDEX_LOCK_RELEASED, 1 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void* smlLockMgr::detectDeadlockSpin( void* /* aArg */ )
{
    SInt            i;
    SInt            j;
    SInt            k;
    SInt            l;
    SInt            sQueueHead;
    SInt            sQueueTail;
    SInt            sLastDetected;
    idBool          sDetected;
    ULong           sSerialMax;
    SInt            sSlotIDMax;
    SInt            sSleepTime;
    PDL_Time_Value  sTimeVal;

    sLastDetected = -1;
    while ( mStopDetect == 0 )
    {
    
        sSleepTime = smuProperty::getLockMgrDetectDeadlockInterval();
        if ( sSleepTime != 0 )
        {
            sTimeVal.set(sSleepTime, 0);
            idlOS::sleep(sTimeVal);
        }

        if ( mTXPendCount == 0 )
        {
            continue;
        }
        else
        {
            /* fall through */
        }

        IDL_MEM_BARRIER;
        if ( sLastDetected != -1 )
        {
            if ( mIsCycle[sLastDetected] == ID_TRUE )
            {
                continue;
            }
            else
            {
                sLastDetected = -1;
            }
        }

        for ( i = 0 ; i < mTransCnt ; i++ )
        {
            mIsChecked[i] = ID_FALSE;
        }

        for ( i = 0 ; i < mTransCnt ; i++ )
        {
            if ( ( mIsChecked[i] == ID_TRUE ) ||
                 ( smxTransMgr::getTransStatus(i) != SMX_TX_BLOCKED ) )
            {
                continue;
            }
            else
            {
                /* fall through */
            }

            mDetectQueue[0] = i;
            sQueueHead = 1;
            sQueueTail = 1;

            /* peek all slot IDs that are waiting on TX[i] */
            for ( j=0; j<mTransCnt;j++)
            {
                if ( (mPendingMatrix[j][i] != -1) &&
                     (mIsChecked[j] == ID_FALSE) )
                {
                    mIsChecked[j] = ID_TRUE;
                    mDetectQueue[sQueueHead] = j;
                    sQueueHead++;
                }
            }

            /* extract graph and check cycle */
            sDetected = ID_FALSE;
            while ( sQueueHead != sQueueTail )
            {
                k = mDetectQueue[sQueueTail];
                sQueueTail++;

                /* peek all slot IDs that are waiting on TX[k] */
                for ( j=0; j < mTransCnt; j++ )
                {
                    if ( (mPendingMatrix[j][k] != -1) &&
                         (mIsChecked[j] == ID_FALSE) )
                    {
                        for ( l = 0 ; l < sQueueHead ; l++ )
                        {
                            if ( mDetectQueue[l] == j )
                            {
                                sDetected = ID_TRUE;
                                break;
                            }
                        }

                        if ( sDetected != ID_TRUE )
                        {
                            mIsChecked[j] = ID_TRUE;
                            mDetectQueue[sQueueHead] = j;
                            sQueueHead++;
                        }
                    }
 
                }

            } /* while for queue */

            if ( sDetected == ID_TRUE )
            {
                if ( mIsDetected[i] == ID_TRUE )
                {
                    /* find smallest serial in blocked transactions */
                    sSlotIDMax = -1;
                    sSerialMax = 0;
                    for ( j = 0 ; j < sQueueHead ; j++ )
                    {
                        k = mDetectQueue[j];
                        if ( sSerialMax < mSerialArray[k] )
                        {
                            sSerialMax = mSerialArray[k];
                            sSlotIDMax = k;
                        }
                    }

                    IDE_DASSERT(sSlotIDMax != -1);
                    if ( sSlotIDMax != -1 )
                    {
                        mIsCycle[sSlotIDMax] = ID_TRUE;
                        mIsDetected[i] = ID_FALSE;
                        sLastDetected = sSlotIDMax;
                    }

                    /* stop detect and loop again */
                    break;
                }
                else
                {
                    mIsDetected[i] = ID_TRUE;
                    mIsCycle[i] = ID_FALSE;
                }
            }
            else
            {
                mIsDetected[i] = ID_FALSE;
                mIsCycle[i] = ID_FALSE;
            }
        } /* for i */
    } /* thread while */

    return NULL;
}

void smlLockMgr::registLockWaitSpin( SInt aSlot, SInt aWaitSlot )
{

    if ( mPendingMatrix[aWaitSlot][aSlot] != -1 )
    {
        if ( mIsDetected[aSlot] == ID_TRUE )
        {
            mIsCycle[aSlot] = ID_TRUE;
            mIsDetected[aSlot] = ID_FALSE;
        }
        else
        {
            mIsCycle[aSlot] = ID_FALSE;
            mIsDetected[aSlot] = ID_TRUE;
        }
    }

    mPendingMatrix[aSlot][aWaitSlot] = 1;

}

idBool smlLockMgr::didLockReleasedSpin( SInt aSlot, SInt aWaitSlot )
{
    return ( mPendingMatrix[aSlot][aWaitSlot] == -1 ) ? ID_TRUE : ID_FALSE;  
}

IDE_RC smlLockMgr::freeAllRecordLockSpin( SInt aSlot )
{
    SInt i;

    for ( i = 0; i < mTransCnt; i++ )
    {
        mPendingMatrix[i][aSlot] = -1;
    }

    return IDE_SUCCESS;
}

void smlLockMgr::setLockModeAndAddLockSlotSpin( SInt             aSlot,
                                                smlLockNode  *   aTxLockNode,
                                                smlLockMode  *   aCurLockMode,
                                                smlLockMode      aLockMode,
                                                idBool           aIsLocked,
                                                smlLockSlot **   aLockSlot,
                                                smlLockMode      aOldMode,
                                                smlLockMode      aNewMode )
{
    if ( aTxLockNode != NULL )
    {
        // 트랜잭션의 lock slot list에  lock slot  추가 .
        if ( aIsLocked == ID_TRUE )
        {
            aTxLockNode->mFlag |= mLockModeToMask[aLockMode];
            addLockSlot( &(aTxLockNode->mArrLockSlotList[aLockMode]),
                         aSlot );

            if ( aLockSlot != NULL )
            {
                *aLockSlot = &(aTxLockNode->mArrLockSlotList[aLockMode]);
                (*aLockSlot)->mOldMode = aOldMode;
                (*aLockSlot)->mNewMode = aNewMode;
            }//if aLockSlot

        }//if aIsLocked
        else
        {
            /* nothing to do */
        }

        if ( aCurLockMode != NULL )
        {
            *aCurLockMode = getLockMode( aTxLockNode );
        }
        else
        {
            /* nothing to do */
        }
    }//aTxLockNode != NULL
    else
    {
        /* nothing to do */
    }
}

void smlLockMgr::clearWaitItemColsOfTransSpin( idBool aDoInit, SInt aSlot )
{
    SInt sWaitSlot;

    PDL_UNUSED_ARG(aDoInit);

    IDE_DASSERT(aSlot < mTransCnt);

    for ( sWaitSlot = 0 ; sWaitSlot < mTransCnt ; sWaitSlot++ )
    {
        mPendingMatrix[aSlot][sWaitSlot] = -1;
    }

    mSerialArray[aSlot] = ID_ULONG(0);
    mIsCycle[aSlot]     = ID_FALSE;
}


void smlLockMgr::calcIndex( const SInt aSlotNo, SInt& aIndex, SInt& aBitIndex )
{
    IDE_DASSERT( aSlotNo < mTransCnt );

    aIndex      = aSlotNo / 64;
    aBitIndex   = aSlotNo % 64;
}

void smlLockMgr::beginPending(SInt aSlot)
{
    mSerialArray[aSlot] = acpAtomicInc64( &mPendSerial );
}
