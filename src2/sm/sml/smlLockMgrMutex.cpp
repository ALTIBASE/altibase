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
 * $Id: smlLockMgrMutex.cpp 82075 2018-01-17 06:39:52Z jina.kim $
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
#include <smlLockMgr.h>
#include <smlReq.h>
#include <smu.h>
#include <smrLogHeadI.h>
#include <sct.h>
#include <smxTrans.h>

/*********************************************************
  function description: decTblLockModeAndTryUpdate
  table lock 정보인 aLockItem에서
  Lock node의 lock mode에 해당하는 lock mode의 갯수를 줄이고,
  만약 0이 된다면, table의 대표락을 변경한다.
***********************************************************/
void smlLockMgr::decTblLockModeAndTryUpdate( smlLockItemMutex*   aLockItem,
                                             smlLockMode         aLockMode )
{
    --(aLockItem->mArrLockCount[aLockMode]);
    IDE_ASSERT(aLockItem->mArrLockCount[aLockMode] >= 0);
    // table의 대표 락을 변경한다.
    if ( aLockItem->mArrLockCount[aLockMode] ==  0 )
    {
        aLockItem->mFlag &= ~(mLockModeToMask[aLockMode]);
    }
}

/*********************************************************
  function description: incTblLockModeUpdate
  table lock 정보인 aLockItem에서
  Lock node의 lock mode에 해당하는 lock mode의 갯수를 늘이고,
   table의 대표락을 변경한다.
***********************************************************/
void smlLockMgr::incTblLockModeAndUpdate( smlLockItemMutex*  aLockItem,
                                          smlLockMode        aLockMode )
{
    ++(aLockItem->mArrLockCount[aLockMode]);
    IDE_ASSERT(aLockItem->mArrLockCount[aLockMode] >= 0);
    aLockItem->mFlag |= mLockModeToMask[aLockMode];
}


void smlLockMgr::addLockNodeToHead( smlLockNode *&aFstLockNode, 
                                    smlLockNode *&aLstLockNode, 
                                    smlLockNode *&aNewLockNode )
{
    if ( aFstLockNode != NULL ) 
    { 
        aFstLockNode->mPrvLockNode = aNewLockNode; 
    } 
    else 
    { 
        aLstLockNode = aNewLockNode; 
    } 
    aNewLockNode->mPrvLockNode = NULL; 
    aNewLockNode->mNxtLockNode = aFstLockNode; 
    aFstLockNode = aNewLockNode;
    aNewLockNode->mDoRemove= ID_FALSE;
}

void smlLockMgr::addLockNodeToTail( smlLockNode *&aFstLockNode, 
                                    smlLockNode *&aLstLockNode, 
                                    smlLockNode *&aNewLockNode )
{
    if ( aLstLockNode != NULL ) 
    { 
        aLstLockNode->mNxtLockNode = aNewLockNode; 
    } 
    else 
    { 
        aFstLockNode = aNewLockNode; 
    } 
    aNewLockNode->mPrvLockNode = aLstLockNode;
    aNewLockNode->mNxtLockNode = NULL; 
    aLstLockNode = aNewLockNode;
    aNewLockNode->mDoRemove= ID_FALSE;
}

void smlLockMgr::removeLockNode( smlLockNode *&aFstLockNode,
                                 smlLockNode *&aLstLockNode,
                                 smlLockNode *&aLockNode ) 
{
    if ( aLockNode == aFstLockNode ) 
    { 
        aFstLockNode = aLockNode->mNxtLockNode; 
    } 
    else 
    { 
        aLockNode->mPrvLockNode->mNxtLockNode = aLockNode->mNxtLockNode; 
    } 
    if ( aLockNode == aLstLockNode ) 
    { 
        aLstLockNode = aLockNode->mPrvLockNode; 
    } 
    else 
    { 
        aLockNode->mNxtLockNode->mPrvLockNode = aLockNode->mPrvLockNode; 
    } 
    aLockNode->mNxtLockNode = NULL; 
    aLockNode->mPrvLockNode = NULL;

    aLockNode->mDoRemove= ID_TRUE;
}

IDE_RC smlLockMgr::initializeMutex( UInt aTransCnt )
{

    SInt i, j;

    IDE_ASSERT( smuProperty::getLockMgrType() == 0 );

    mTransCnt = aTransCnt;

    IDE_ASSERT(mTransCnt > 0);

    //Alloc wait table
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SML,
                                       mTransCnt,
                                       ID_SIZEOF(smlLockMatrixItem *),
                                       (void**)&mWaitForTable ) != IDE_SUCCESS,
                    insufficient_memory );

    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SML,
                                           mTransCnt,
                                           ID_SIZEOF(smlLockMatrixItem),
                                           (void**)&(mWaitForTable[i] ) ) != IDE_SUCCESS,
                        insufficient_memory );


    }

    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        for ( j = 0 ; j < mTransCnt ; j++ )
        {
            mWaitForTable[i][j].mIndex = 0;
            mWaitForTable[i][j].mNxtWaitTransItem = ID_USHORT_MAX;
            mWaitForTable[i][j].mNxtWaitRecTransItem = ID_USHORT_MAX;
        }
    }

    mLockTableFunc                  = lockTableMutex;
    mUnlockTableFunc                = unlockTableMutex;
    mInitLockItemFunc               = initLockItemMutex;
    mAllocLockItemFunc              = allocLockItemMutex;
    mDidLockReleasedFunc            = didLockReleasedMutex;
    mRegistRecordLockWaitFunc       = registRecordLockWaitMutex;
    mFreeAllRecordLockFunc          = freeAllRecordLockMutex;
    mClearWaitItemColsOfTransFunc   = clearWaitItemColsOfTransMutex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smlLockMgr::destroyMutex()
{
    SInt i;

    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        IDE_TEST( iduMemMgr::free(mWaitForTable[i]) != IDE_SUCCESS );
    }

    IDE_TEST( iduMemMgr::free(mWaitForTable) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smlLockMgr::allocLockItemMutex( void ** aLockItem )
{
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       ID_SIZEOF(smlLockItemMutex),
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

IDE_RC smlLockMgr::initLockItemMutex( scSpaceID       aSpaceID,
                                      ULong           aItemID,
                                      smiLockItemType aLockItemType,
                                      void           *aLockItem )
{
    SInt                sLockType;
    smlLockItemMutex*   sLockItem;

    IDE_TEST( initLockItemCore( aSpaceID, 
                                aItemID,
                                aLockItemType, 
                                aLockItem )
              != IDE_SUCCESS );

    sLockItem = (smlLockItemMutex*) aLockItem;

    sLockItem->mGrantLockMode   = SML_NLOCK;
    sLockItem->mFstLockGrant    = NULL;
    sLockItem->mFstLockRequest  = NULL;
    sLockItem->mLstLockGrant    = NULL;
    sLockItem->mLstLockRequest  = NULL;
    sLockItem->mGrantCnt        = 0;
    sLockItem->mRequestCnt      = 0;
    sLockItem->mFlag            = 0;

    for ( sLockType = 0 ; sLockType < SML_NUMLOCKTYPES ; sLockType++ )
    {
        sLockItem->mArrLockCount[sLockType] = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: lockTable
  table에 lock을 걸때 다음과 같은 case에 따라 각각 처리한다.

  1. 이전에 트랜잭션이  table A에  lock을 잡았고,
    이전에 잡은 lock mode와 지금 잡고자 하는 락모드 변환결과가
    같으면 바로 return.

  2. table의 grant lock mode과 지금 잡고자 하는 락과 호환가능한경우.

     2.1  table 에 lock waiting하는 트랜잭션이 하나도 없고,
          이전에 트랜잭션이 table에 대하여 lock을 잡지 않는 경우.

       -  lock node를 할당하고 초기화한다.
       -  table lock의 grant list tail에 add한다.
       -  lock node가 grant되었음을 갱신하고,
          table lock의 grant count를 1증가 시킨다.
       -  트랙잭션의 lock list에  lock node를  add한다.

     2.2  table 에 lock waiting하는 트랜잭션들이 있지만,
          이전에 트랜잭션이  table에 대하여 lock을 잡은 경우.
         -  table에서 grant된 lock mode array에서
            트랜잭션의 lock node의   lock mode를 1감소.
            table 의 대표락 갱신시도.

     2.1, 2.2의 공통적으로  다음을 수행하고 3번째 단계로 넘어감.
     - 현재 요청한 lock mode와 이전  lock mode를
      conversion하여, lock node의 lock mode를 갱신.
     - lock node의 갱신된  lock mode을 이용하여
      table에서 grant된 lock mode array에서
      새로운 lockmode 에 해당하는 lock mode를 1증가하고
      table의 대표락을 갱신.
     - table lock의 grant mode를 갱신.
     - grant lock node의 lock갯수 증가.

     2.3  이전에 트랜잭션이 table에 대하여 lock을 잡지 않았고,
          table의 request list가 비어 있지 않는 경우.
         - lock을 grant하지 않고 3.2절과 같이 lock대기처리를
           한다.
           % 형평성 문제 때문에 이렇게 처리한다.

  3. table의 grant lock mode과 지금 잡고자 하는 락과 호환불가능한 경우.
     3.1 lock conflict이지만,   grant된 lock node가 1개이고,
         그것이 바로 그  트랙잭션일 경우.
      - request list에 달지 않고
        기존 grant된  lock node의 lock mode와 table의 대표락,
        grant lock mode를   갱신한다.

     3.2 lock conflict 이고 lock 대기 시간이 0이 아닌 경우.
        -  request list에 달 lock node를 생성하고
         아래 세부 case에 대하여, 분기.
        3.2.1 트랜잭션이 이전에 table에 대하여 grant된
              lock node를 가지고 있는 경우.
           -  request list의 헤더에 생성된 lock node를 추가.
           -  request list에 매단 lock node의 cvs node를
               grant된 lock node의 주소로 assgin.
            -> 나중에 다른 트랜잭션에서  unlock table시에
                request list에 있는 lock node와
               갱신된 table grant mode와 호환될때,
               grant list에 별도로 insert하지 않고 ,
               기존 cvs lock node의 lock mode만 갱신시켜
               grant list 연산을 줄이려는 의도임.

        3.2.2 이전에 grant된 lock node가 없는 경우.
            -  table lock request list에서 있는 트랜잭션들의
               slot id들을  waitingTable에서 lock을 요구한
               트랙잭션의 slot id 행에 list로 연결한다
               %경로길이는 1로
            - request list의 tail에 생성된 lock node를 추가.

        3.2.1, 3.2.2 공통으로 마무리 스텝으로 다음과 같이한다.

        - waiting table에서   트랜잭션의 slot id 행에
        grant list에서     있는트랙잭션들 의
        slot id를  table lock waiting 리스트에 등록한다.
        - dead lock 를 검사한다(isCycle)
         -> smxTransMgr::waitForLock에서 검사하며,
          dead lock이 발생하면 트랜잭션 abort.
        - waiting하게 된다(suspend).
         ->smxTransMgr::waitForLock에서 수행한다.
        -  다른 트랜잭션이 commit/abort되면서 wakeup되면,
          waiting table에서 자신의 트랜잭션 slot id에
          대응하는 행에서  대기 컬럼들을 clear한다.
        -  트랜잭션의 lock list에 lock node를 추가한다.
          : lock을 잡게 되었기때문에.

     3.3 lock conflict 이고 lock대기 시간이 0인 경우.
        : lock flag를 ID_FALSE로 갱신.

  4. 트랜잭션의 lock slot list에  lock slot  추가 .


 BUG-28752 lock table ... in row share mode 구문이 먹히지 않습니다.

 implicit/explicit lock을 구분하여 겁니다.
 implicit is lock만 statement end시 풀어주기 위함입니다. 

 이 경우 Upgrding Lock에 대해 고려하지 않아도 됩니다.
  즉, Implicit IS Lock이 걸린 상태에서 Explicit IS Lock을 걸려 하거나
 Explicit IS Lock이 걸린 상태에서 Implicit IS Lock이 걸린 경우에 대해
 고려하지 않아도 됩니다.
  왜냐하면 Imp가 먼저 걸려있을 경우, 어차피 Statement End되는 즉시 풀
 어지기 때문에 이후 Explicit Lock을 거는데 문제 없고, Exp가 먼저 걸려
 있을 경우, 어차피 IS Lock이 걸려 있는 상태이기 때문에 Imp Lock을 또
 걸 필요는 없기 때문입니다.
***********************************************************/
IDE_RC smlLockMgr::lockTableMutex( SInt          aSlot,
                                   smlLockItem  *aLockItem,
                                   smlLockMode   aLockMode,
                                   ULong         aLockWaitMicroSec,
                                   smlLockMode   *aCurLockMode,
                                   idBool       *aLocked,
                                   smlLockNode **aLockNode,
                                   smlLockSlot **aLockSlot,
                                   idBool        aIsExplicit )
{
    smlLockItemMutex*   sLockItem = (smlLockItemMutex*)aLockItem;
    smlLockNode*        sCurTransLockNode = NULL;
    smlLockNode*        sNewTransLockNode = NULL;
    UInt                sState            = 0;
    idBool              sLocked           = ID_TRUE;
    idvSQL*             sStat;
    idvSession*         sSession;
    UInt                sLockEnable = 1;


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

    sSession = (sStat == NULL ? NULL : sStat->mSess);

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
        if ( (aLockMode != SML_ISLOCK) && (aLockMode !=SML_IXLOCK) )
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

        if ( aLocked != NULL )
        {
            *aLocked = ID_TRUE;
        }

        if ( aLockSlot != NULL )
        {
            *aLockSlot = NULL;
        }

        if ( aCurLockMode != NULL )
        {
            *aCurLockMode = aLockMode;
        }

        return IDE_SUCCESS;
    }

    if ( aLocked != NULL )
    {
        *aLocked = ID_TRUE;
    }

    if ( aLockSlot != NULL )
    {
        *aLockSlot = NULL;
    }

    // 트랜잭션이 이전 statement에 의하여,
    // 현재 table A에 대하여 lock을 잡았던 lock node를 찾는다.
    sCurTransLockNode = findLockNode(sLockItem,aSlot);
    // case 1: 이전에 트랜잭션이  table A에  lock을 잡았고,
    // 이전에 잡은 lock mode와 지금 잡고자 하는 락모드 변환결과가
    // 같으면 바로 return!
    if ( sCurTransLockNode != NULL )
    {
        if ( mConversionTBL[sCurTransLockNode->mLockMode][aLockMode]
             == sCurTransLockNode->mLockMode )
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
                    *aCurLockMode = sCurTransLockNode->mLockMode;
                }

                IDV_SESS_ADD( sSession,
                              IDV_STAT_INDEX_LOCK_ACQUIRED, 
                              1 );

                /* We don't need  this code since delayed unlock thread checks 
                 * if it's ok to unlock every time.
                 * I leave this code just for sure. 
                 *
                 *   if( aLockMode == SML_ISLOCK )
                 *   {
                 *       mDelayedUnlock.clearBitmap(aSlot); 
                 *   }
                 */

                return IDE_SUCCESS;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST_RAISE( sLockItem->mMutex.lock( NULL /* idvSQL* */)
                    != IDE_SUCCESS, err_mutex_lock );

    sState = 1;

    //---------------------------------------
    // table의 대표락과 지금 잡고자 하는 락이 호환가능하는지 확인.
    //---------------------------------------
    if ( mCompatibleTBL[sLockItem->mGrantLockMode][aLockMode] == ID_TRUE )
    {
        if ( sCurTransLockNode != NULL ) /* 이전 statement 수행시 lock을 잡음 */
        {   
            decTblLockModeAndTryUpdate( sLockItem,
                                        sCurTransLockNode->mLockMode );
        }
        else                      /* 이전 statement 수행 시 lock을 잡지 못함 */
        {
            if ( sLockItem->mRequestCnt != 0 ) /* 기다리는 lock이 있는 경우 */
            {
                /* BUGBUG: BUG-16471, BUG-17522
                 * 기다리는 lock이 있는 경우 무조건 lock conflict 처리한다. */
                IDE_CONT(lock_conflict);
            }
            else
            {
                /* 기다리는 lock이 없는 경우 Lock을 grant 함 */
            }

            /* allocate lock node and initialize */
            IDE_TEST( allocLockNodeAndInit( aSlot,
                                            aLockMode,
                                            sLockItem,
                                            &sCurTransLockNode,
                                            aIsExplicit )
                      != IDE_SUCCESS );

            /* add node to grant list */
            addLockNodeToTail( sLockItem->mFstLockGrant,
                               sLockItem->mLstLockGrant,
                               sCurTransLockNode );

            sCurTransLockNode->mBeGrant = ID_TRUE;
            sLockItem->mGrantCnt++;
            /* Add Lock Node to a transaction */
            addLockNode( sCurTransLockNode, aSlot );
        }

        /* Lock에 대해서 Conversion을 수행한다. */
        sCurTransLockNode->mLockMode =
            mConversionTBL[sCurTransLockNode->mLockMode][aLockMode];

        incTblLockModeAndUpdate(sLockItem, sCurTransLockNode->mLockMode);
        sLockItem->mGrantLockMode =
            mConversionTBL[sLockItem->mGrantLockMode][aLockMode];

        sCurTransLockNode->mLockCnt++;
        IDE_CONT(lock_SUCCESS);
    }

    //---------------------------------------
    // Lock Conflict 처리
    //---------------------------------------

    IDE_EXCEPTION_CONT(lock_conflict);

    if ( ( sLockItem->mGrantCnt == 1 ) && ( sCurTransLockNode != NULL ) )
    {
        //---------------------------------------
        // lock conflict이지만, grant된 lock node가 1개이고,
        // 그것이 바로 그  트랙잭션일 경우에는 request list에 달지 않고
        // 기존 grant된  lock node의 lock mode와 table의 대표락,
        // grant lock mode를  갱신한다.
        //---------------------------------------

        decTblLockModeAndTryUpdate( sLockItem,
                                    sCurTransLockNode->mLockMode );

        sCurTransLockNode->mLockMode =
            mConversionTBL[sCurTransLockNode->mLockMode][aLockMode];

        sLockItem->mGrantLockMode =
            mConversionTBL[sLockItem->mGrantLockMode][aLockMode];

        incTblLockModeAndUpdate( sLockItem, sCurTransLockNode->mLockMode );
    }
    else if ( aLockWaitMicroSec != 0 )
    {
        IDE_TEST( allocLockNodeAndInit( aSlot,
                                        aLockMode,
                                        sLockItem,
                                        &sNewTransLockNode,
                                        aIsExplicit ) != IDE_SUCCESS );
        sNewTransLockNode->mBeGrant    = ID_FALSE;
        if ( sCurTransLockNode != NULL )
        {
            sNewTransLockNode->mCvsLockNode = sCurTransLockNode;

            //Lock node를 Lock request 리스트의 헤더에 추가한다.
            // 왜냐하면 Conversion이기 때문이다.
            addLockNodeToHead( sLockItem->mFstLockRequest,
                               sLockItem->mLstLockRequest,
                               sNewTransLockNode );
        }
        else
        {
            sCurTransLockNode = sNewTransLockNode;
            // waiting table에서   트랜잭션의 slot id 행에
            // request list에서  대기하고 있는트랙잭션들 의
            // slot id를  table lock waiting 리스트에 등록한다.
            registTblLockWaitListByReq(aSlot,sLockItem->mFstLockRequest);
            //Lock node를 Lock request 리스트의 Tail에 추가한다.
            addLockNodeToTail( sLockItem->mFstLockRequest,
                               sLockItem->mLstLockRequest,
                               sNewTransLockNode );
        }
        // waiting table에서   트랜잭션의 slot id 행에
        // grant list에서     있는트랙잭션들 의
        // slot id를  table lock waiting 리스트에 등록한다.
        registTblLockWaitListByGrant( aSlot,sLockItem->mFstLockGrant );
        sLockItem->mRequestCnt++;

        IDE_TEST_RAISE( smLayerCallback::waitForLock(
                                        smLayerCallback::getTransBySID( aSlot ),
                                        &(sLockItem->mMutex),
                                        aLockWaitMicroSec )
                        != IDE_SUCCESS, err_wait_lock );
        // lock wait후에 wakeup되어 lock을 쥐게됨.
        // Add Lock Node to Transaction
        addLockNode( sNewTransLockNode, aSlot );
    }
    else
    {
        sLocked = ID_FALSE;

        if ( aLocked != NULL )
        {
            *aLocked = ID_FALSE;
        }
    }

    IDE_EXCEPTION_CONT(lock_SUCCESS);


    //트랜잭션의 Lock node가 있었고,Lock을 잡았다면
    // lock slot을 추가한다
    setLockModeAndAddLockSlotMutex( aSlot,
                                    sCurTransLockNode,
                                    aCurLockMode,
                                    aLockMode,
                                    sLocked,
                                    aLockSlot );
    IDE_TEST_RAISE(sLocked == ID_FALSE, err_exceed_wait_time);
    IDE_DASSERT( sCurTransLockNode->mArrLockSlotList[aLockMode].mLockSequence != 0 );

    sState = 0;
    IDE_TEST_RAISE(sLockItem->mMutex.unlock() != IDE_SUCCESS, err_mutex_unlock);

    if ( aLockNode != NULL )
    {
        *aLockNode = sCurTransLockNode;
    }
    else
    {
        /* nothing to do */
    }

    IDV_SESS_ADD(sSession, IDV_STAT_INDEX_LOCK_ACQUIRED, 1);

    return IDE_SUCCESS;

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
    IDE_EXCEPTION(err_mutex_lock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(err_mutex_unlock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION(err_wait_lock);
    {
        // fix BUG-10202하면서,
        // dead lock , time out을 튕겨 나온 트랜잭션들의
        // waiting table row의 열을 clear하고,
        // request list의 transaction이깨어나도 되는지 체크한다.

        (void)smlLockMgr::unlockTable( aSlot, 
                                       sNewTransLockNode,
                                       NULL, 
                                       ID_FALSE );
    }
    IDE_EXCEPTION(err_exceed_wait_time);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_smcExceedLockTimeWait));
    }
    IDE_EXCEPTION_END;
    //waiting table에서 aSlot의 행에 대기열을 clear한다.
    clearWaitItemColsOfTransMutex(ID_TRUE, aSlot);

    if ( sState != 0 )
    {
        IDE_ASSERT( sLockItem->mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}
/*********************************************************
  function description: unlockTable
  아래와 같이 다양한 case에 따라 각각 처리한다.
  1. lock node가 이미table의 grant or wait list에서
     이미 제거 되어 있는 경우.
     - lock node가 트랜잭션의 lock list에서 달려있으면,
       lock node를 트랜잭션의 lock list에서 제거한다.

  2. if (lock node가 grant된 상태 일경우)
     then
      table lock정보에서 lock node의 lock mode를  1감소
      시키고 table 대표락 갱신시도.
      aLockNode의 대표락  &= ~(aLockSlot의 mMask).
      if( aLock node의 대표락  != 0)
      then
        aLockNode의 lockMode를 갱신한다.
        갱신된 lock node의 mLockMode를 table lock정보에서 1증가.
        lock node삭제 하라는 flag를 off시킨다.
        --> lockTable의 2.2절에서 이전에 트랜잭션이 grant 된
        lock node에 대하여, grant lock node를 추가하는 대신에
        lock mode를 conversion하여 lock slot을
        add한 경우이다. 즉 node는 하나이지만, 그안에
        lock slot이 두개 이상이 있는 경우이다.
        4번째 단계로 넘어간다.
      fi
      grant list에서 lock node를 제거시킨다.
      table lock정보에서 grant count 1 감소.
      새로운 Grant Lock Mode를 결정한다.
    else
      // lock node가 request list에 달려 있는 경우.
       request list에서 lock node를 제거.
       request count감소.
       grant count가 0이면 5번단계로 넘어감.
    fi
  3. waiting table을 clear한다.
     grant lock node이거나, request list에 있었던 lock node
     가  완전히 Free될 경우 이 Transaction을 기다리고 있는
     Transaction의  waiting table에서 갱신하여 준다.
     하지만 request에 있었던 lock node의 경우,
     Grant List에 Lock Node가
     남아 있는경우는 하지 않는다.


  4. lock을 wait하고 있는 Transaction을 깨운다.
     현재 lock node :=  table lock정보에서 request list에서
                        맨처음 lock node.
     while( 현재 lock node != null)
     begin loop.
      if( 현재 table의 grant mode와 현재 lock node의 lock mode가 호환?)
      then
         request 리스트에서 Lock Node를 제거한다.
         table lock정보에서 grant lock mode를 갱신한다.
         if(현재 lock node의 cvs node가 null이 아니면)
         then
            table lock 정보에서 현재 lock node의
            cvs node의 lock mode를 1감소시키고,대표락 갱신시도.
            cvs node의 lock mode를 갱신한다.
            table lock 정보에서 cvs node의
            lock mode를 1증가 시키고 대표락을 갱신.
         else
            table lock 정보에서 현재 lock node의 lock mode를
            1증가 시키고 대표락을 갱신.
            Grant리스트에 새로운 Lock Node를 추가한다.
            현재 lock node의 grant flag를 on 시킨다.
            grant count를 1증가 시킨다.
         fi //현재 lock node의 cvs node가 null이 아니면
      else
      //현재 table의 grant mode와 현재 lock node의 lock mode가
      // 호환되지 않는 경우.
        if(table의 grant count == 1)
        then
           if(현재 lock node의 cvs node가 있는가?)
           then
             //cvs node가 바로  grant된 1개 node이다.
              table lock 정보에서 cvs lock node의 lock mode를
              1감소 시키고, table 대표락 갱신시도.
              table lock 정보에서 grant mode갱신.
              cvs lock node의 lock mode를 현재 table의 grant mode
              으로 갱신.
              table lock 정보에서 현재 grant mode를 1증가 시키고,
              대표락 갱신.
              Request 리스트에서 Lock Node를 제거한다.
           else
              break; // nothint to do
           fi
        else
           break; // nothing to do.
        fi
      fi// lock node의 lock mode가 호환되지 않는 경우.

      table lock정보에서 request count 1감소.
      현재 lock node의 slot id을  waiting하고 있는 row들을
      waiting table에서 clear한다.
      waiting하고 있는 트랜잭션을 resume시킨다.
     loop end.

  5. lock slot, lock node 제거 시도.
     - lock slot 이 null이 아니면,lock slot을 transaction
        의 lock slot list에서 제거한다.
     - 2번 단계에서 lock node를 제거하라는 flag가 on
     되어 있으면  lock node를  트랜잭션의 lock list에서
     제거 하고, lock node를 free한다.
***********************************************************/

IDE_RC smlLockMgr::unlockTableMutex( SInt          aSlot,
                                     smlLockNode  *aLockNode,
                                     smlLockSlot  *aLockSlot,
                                     idBool        aDoMutexLock )
{

    smlLockItemMutex*   sLockItem = NULL;
    smlLockNode*        sCurLockNode;
    UInt                sState = 0;
    idBool              sDoFreeLockNode = ID_TRUE;
    idvSQL*             sStat;
    idvSession*         sSession;

    sStat = smLayerCallback::getStatisticsBySID( aSlot );

    sSession = ((sStat == NULL) ? NULL : sStat->mSess);


    if ( aLockNode == NULL )
    {
        IDE_DASSERT( aLockSlot->mLockNode != NULL );

        aLockNode = aLockSlot->mLockNode;
    }

    // lock node가 이미 request or grant list에서 제거된 상태.
    if ( aLockNode->mDoRemove == ID_TRUE )
    {
        IDE_ASSERT( ( aLockNode->mPrvLockNode == NULL ) &&
                    ( aLockNode->mNxtLockNode == NULL ) );
        //트랜잭션의 lock list에서 아직 제거되지 않았느지확인.
        if ( aLockNode->mPrvTransLockNode != NULL )
        {
            // 트랜잭션 lock list array에서
            // transaction의 slot id에 해당하는
            // list에서 lock node를 제거한다.
            removeLockNode(aLockNode);
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( freeLockNode(aLockNode) != IDE_SUCCESS );

        IDV_SESS_ADD(sSession, IDV_STAT_INDEX_LOCK_RELEASED, 1);

        return IDE_SUCCESS;
    }
    else
    {
        /* nothing to do */
    }


    sLockItem = (smlLockItemMutex*)aLockNode->mLockItem;

    if ( aDoMutexLock == ID_TRUE )
    {
        IDE_TEST_RAISE( sLockItem->mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS,
                        err_mutex_lock );
        sState = 1;
    }
    else
    {
        /* nothing to do */
    }


    while(1)
    {
        if ( aLockNode->mBeGrant == ID_TRUE )
        {
            decTblLockModeAndTryUpdate( sLockItem, aLockNode->mLockMode );
            while ( 1 )
            {
                if ( aLockSlot != NULL )
                {
                    aLockNode->mFlag  &= ~(aLockSlot->mMask);
                    // lockTable의 2.2절에서 이전에 트랜잭션이 grant 된
                    // lock node에 대하여, grant lock node를 추가하는 대신에
                    // lock mode를 conversion하여 lock slot을
                    // add한 경우이다. 즉 node는 하나이지만, 그안에
                    // lock slot이 두개 이상이 있는 경우이다.
                    // -> 따라서 lock node를 삭제하면 안된다.
                    //   sDoFreeLockNode = ID_FALSE;
                    if ( aLockNode->mFlag != 0 )
                    {
                        aLockNode->mLockMode = mDecisionTBL[aLockNode->mFlag];
                        incTblLockModeAndUpdate( sLockItem,
                                                 aLockNode->mLockMode );
                        sDoFreeLockNode = ID_FALSE;
                        break;
                    }//if aLockNode
                } // if aLockSlot != NULL
                //Remove lock node from lock Grant list
                removeLockNode( sLockItem->mFstLockGrant,
                                sLockItem->mLstLockGrant,
                                aLockNode );
                sLockItem->mGrantCnt--;
                break;
            } // inner while loop .
            //새로운 Grant Lock Mode를 결정한다.
            sLockItem->mGrantLockMode = mDecisionTBL[sLockItem->mFlag];
        }//if aLockNode->mBeGrant == ID_TRUE
        else
        {
            // Grant 되어있지 않는 상태.
            //remove lock node from lock request list
            removeLockNode( sLockItem->mFstLockRequest,
                            sLockItem->mLstLockRequest, 
                            aLockNode );
            sLockItem->mRequestCnt--;

        }//else aLockNode->mBeGrant == ID_TRUE.

        if ( ( sDoFreeLockNode == ID_TRUE ) && ( aLockNode->mCvsLockNode == NULL ) )
        {
            // grant lock node이거나, request list에 있었던 lock node
            //가  완전히 Free될 경우 이 Transaction을 기다리고 있는
            //Transaction의  waiting table에서 갱신하여 준다.
            //하지만 request에 있었던 lock node의 경우,
            // Grant List에 Lock Node가
            //남아 있는경우는 하지 않는다.
            clearWaitTableRows( sLockItem->mFstLockRequest,
                                aSlot );
        }

        //lock을 기다리는 Transaction을 깨운다.
        sCurLockNode = sLockItem->mFstLockRequest;
        while ( sCurLockNode != NULL )
        {
            //Wake up requestors
            //현재 table의 grant mode와 현재 lock node의 lock mode가 호환?
            if ( mCompatibleTBL[sCurLockNode->mLockMode][sLockItem->mGrantLockMode] == ID_TRUE )
            {
                //Request 리스트에서 Lock Node를 제거한다.
                removeLockNode( sLockItem->mFstLockRequest,
                                sLockItem->mLstLockRequest,
                                sCurLockNode );

                sLockItem->mGrantLockMode =
                    mConversionTBL[sLockItem->mGrantLockMode][sCurLockNode->mLockMode];
                if ( sCurLockNode->mCvsLockNode != NULL )
                {
                    //table lock 정보에서 현재 lock node의
                    //cvs node의  lock mode를 1감소시키고,대표락 갱신시도.
                    decTblLockModeAndTryUpdate( sLockItem,
                                                sCurLockNode->mCvsLockNode->mLockMode );
                    //cvs node의 lock mode를 갱신한다.
                    sCurLockNode->mCvsLockNode->mLockMode =
                        mConversionTBL[sCurLockNode->mCvsLockNode->mLockMode][sCurLockNode->mLockMode];
                    //table lock 정보에서 cvs node의
                    //lock mode를 1증가 시키고 대표락을 갱신.
                    incTblLockModeAndUpdate( sLockItem,
                                             sCurLockNode->mCvsLockNode->mLockMode );
                }// if sCurLockNode->mCvsLockNode != NULL
                else
                {
                    incTblLockModeAndUpdate( sLockItem, sCurLockNode->mLockMode );
                    //Grant리스트에 새로운 Lock Node를 추가한다.
                    addLockNodeToTail( sLockItem->mFstLockGrant,
                                       sLockItem->mLstLockGrant,
                                       sCurLockNode );
                    sCurLockNode->mBeGrant = ID_TRUE;
                    sLockItem->mGrantCnt++;
                }//else sCurLockNode->mCvsLockNode가 NULL
            }
            // 현재 table의 grant lock mode와 lock node의 lockmode
            // 가 호환하지 않는 경우.
            else
            {
                if ( sLockItem->mGrantCnt == 1 )
                {
                    if ( sCurLockNode->mCvsLockNode != NULL )
                    {
                        // cvs node가 바로 grant된 1개 node이다.
                        //현재 lock은 현재 table에 대한 lock의 Converion 이다.
                        decTblLockModeAndTryUpdate( sLockItem,
                                                    sCurLockNode->mCvsLockNode->mLockMode );

                        sLockItem->mGrantLockMode =
                            mConversionTBL[sLockItem->mGrantLockMode][sCurLockNode->mLockMode];
                        sCurLockNode->mCvsLockNode->mLockMode = sLockItem->mGrantLockMode;
                        incTblLockModeAndUpdate( sLockItem, sLockItem->mGrantLockMode );
                        //Request 리스트에서 Lock Node를 제거한다.
                        removeLockNode( sLockItem->mFstLockRequest,
                                        sLockItem->mLstLockRequest, 
                                        sCurLockNode );
                    }
                    else
                    {
                        break;
                    } // sCurLockNode->mCvsLockNode가 null
                }//sLockItem->mGrantCnt == 1
                else
                {
                    break;
                }//sLockItem->mGrantCnt != 1
            }//mCompatibleTBL

            sLockItem->mRequestCnt--;
            //waiting table에서 aSlot의 행에서  대기열을 clear한다.
            clearWaitItemColsOfTransMutex( ID_FALSE, sCurLockNode->mSlotID );
            // waiting하고 있는 트랜잭션을 resume시킨다.
            IDE_TEST( smLayerCallback::resumeTrans( smLayerCallback::getTransBySID( sCurLockNode->mSlotID ) )
                      != IDE_SUCCESS );
            sCurLockNode = sLockItem->mFstLockRequest;
        }/* while */

        break;
    }/* while */

    if ( aLockSlot != NULL )
    {
        removeLockSlot(aLockSlot);
    }
    else
    {
        /* nothing to do */
    }

    if ( sDoFreeLockNode == ID_TRUE )
    {
        if ( aLockNode->mPrvTransLockNode != NULL )
        {
            // 트랜잭션 lock list array에서
            // transaction의 slot id에 해당하는
            // list에서 lock node를 제거한다.
            removeLockNode(aLockNode);
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    if ( aDoMutexLock == ID_TRUE )
    {
        sState = 0;
        IDE_TEST_RAISE( sLockItem->mMutex.unlock() != IDE_SUCCESS, err_mutex_unlock );
    }
    else
    {
        /* nothing to do */
    }

    if ( sDoFreeLockNode == ID_TRUE )
    {
        IDE_TEST( freeLockNode(aLockNode) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDV_SESS_ADD(sSession, IDV_STAT_INDEX_LOCK_RELEASED, 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_mutex_lock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(err_mutex_unlock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;

    if ( ( sState != 0 ) && ( aDoMutexLock == ID_TRUE ) )
    {
        IDE_ASSERT( sLockItem->mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/*********************************************************
  function description: detectDeadlockMutex
  SIGMOD RECORD, Vol.17, No,2 ,June,1988
  Bin Jang,
  Dead Lock Detection is Really Cheap paper의
  7 page내용과 동일.
  논문 내용과 다른점은,
  waiting table이 chained matrix이어서 알로리듬의
  복잡도를 낮추었고,
  마지막 loop에서 아래와 같은 조건을 주어
  자기자신과경로가 발생하자마자 전체 loop를 빠져나간다는
  점이다.
  if(aSlot == j)
  {
   return ID_TRUE;
  }

***********************************************************/
idBool smlLockMgr::detectDeadlockMutex( SInt aSlot )
{
    UShort     i, j;
    // node와 node간에 대기경로 길이.
    UShort sPathLen;
    // node간에 대기 경로가 있는가 에대한 flag
    idBool     sExitPathFlag;
    void       *sTargetTrans;
    SInt       sSlotN;

    sExitPathFlag = ID_TRUE;
    sPathLen= 1;
    IDL_MEM_BARRIER;

    while ( ( sExitPathFlag == ID_TRUE ) &&
            ( mWaitForTable[aSlot][aSlot].mIndex == 0 ) &&
            ( sPathLen < mTransCnt ) )
    {
        sExitPathFlag      = ID_FALSE;

        for ( i  =  mArrOfLockList[aSlot].mFstWaitTblTransItem;
              ( i != SML_END_ITEM ) &&
              ( mWaitForTable[aSlot][aSlot].mNxtWaitTransItem == ID_USHORT_MAX );
              i = mWaitForTable[aSlot][i].mNxtWaitTransItem )
        {
            IDE_ASSERT(i < mTransCnt);

            if ( mWaitForTable[aSlot][i].mIndex == sPathLen )
            {
                sTargetTrans = smLayerCallback::getTransBySID( i );
                sSlotN = smLayerCallback::getTransSlot( sTargetTrans );

                for ( j  = mArrOfLockList[sSlotN].mFstWaitTblTransItem;
                      j < SML_END_ITEM;
                      j  = mWaitForTable[i][j].mNxtWaitTransItem )
                {
                    if ( (mWaitForTable[i][j].mIndex == 1)
                         && (mWaitForTable[aSlot][j].mNxtWaitTransItem == ID_USHORT_MAX) )
                    {
                        IDE_ASSERT(j < mTransCnt);
                        mWaitForTable[aSlot][j].mIndex = sPathLen + 1;
                        mWaitForTable[aSlot][j].mNxtWaitTransItem
                            = mArrOfLockList[aSlot].mFstWaitTblTransItem;

                        mArrOfLockList[aSlot].mFstWaitTblTransItem = j;
                        // aSlot == j이면,
                        // dead lock 발생 이다..
                        // 왜냐하면
                        // mWaitForTable[aSlot][aSlot].mIndex !=0
                        // 상태가 되었기 때문이다.
                        // dead lock dection하자 마자,
                        // 나머지 loop 생락. 이점이 논문과 틀린점.
                        if ( aSlot == j )
                        {
                            return ID_TRUE;
                        }

                        sExitPathFlag = ID_TRUE;
                    }
                }//For
            }//if
        }//For
        sPathLen++;
    }//While

    return (mWaitForTable[aSlot][aSlot].mIndex == 0) ? ID_FALSE: ID_TRUE;
}

/*********************************************************
  function description:

  aSlot에 해당하는 트랜잭션이 대기하고 있었던
  트랜잭션들의 대기경로 정보를 0으로 clear한다.
  -> waitTable에서 aSlot행에 대기하고 있는 컬럼들에서
    경로길이를 0로 한다.

  aDoInit에서 ID_TRUE이며 대기연결 정보도 끊어버린다.
***********************************************************/
void smlLockMgr::clearWaitItemColsOfTransMutex( idBool aDoInit, SInt aSlot )
{

    UInt    sCurTargetSlot;
    UInt    sNxtTargetSlot;

    sCurTargetSlot = mArrOfLockList[aSlot].mFstWaitTblTransItem;

    while ( sCurTargetSlot != SML_END_ITEM )
    {
        mWaitForTable[aSlot][sCurTargetSlot].mIndex = 0;
        sNxtTargetSlot = mWaitForTable[aSlot][sCurTargetSlot].mNxtWaitTransItem;

        if ( aDoInit == ID_TRUE )
        {
            mWaitForTable[aSlot][sCurTargetSlot].mNxtWaitTransItem = ID_USHORT_MAX;
        }

        sCurTargetSlot = sNxtTargetSlot;
    }

    if ( aDoInit == ID_TRUE )
    {
        mArrOfLockList[aSlot].mFstWaitTblTransItem = SML_END_ITEM;
    }
}

/*********************************************************
  function description:
  : 트랜잭션 a(aSlot)에 대하여 waiting하고 있었던 트랜잭션들의
    대기 경로 정보를 clear한다.
***********************************************************/
void  smlLockMgr::clearWaitTableRows( smlLockNode * aLockNode,
                                      SInt          aSlot )
{

    UShort sTransSlot;

    while ( aLockNode != NULL )
    {
        sTransSlot = aLockNode->mSlotID;
        mWaitForTable[sTransSlot][aSlot].mIndex = 0;
        aLockNode = aLockNode->mNxtLockNode;
    }
}

/*********************************************************
  function description: registRecordLockWait
  다음과 같은 순서로  record락 대기와
  트랜잭션간에 waiting 관계를 연결한다.

  1. waiting table에서 aSlot 이 aWaitSlot에 waiting하고
     있슴을 표시한다.
  2.  aWaitSlot column에  aSlot을
     record lock list에 연결시킨다.
  3.  aSlot행에서 aWaitSlot을  transaction waiting
     list에 연결시킨다.
***********************************************************/
void   smlLockMgr::registRecordLockWaitMutex( SInt aSlot, SInt aWaitSlot )
{
    SInt sLstSlot;

    IDE_ASSERT( mWaitForTable[aSlot][aWaitSlot].mIndex == 0 );
    IDE_ASSERT( mArrOfLockList[aSlot].mFstWaitTblTransItem == SML_END_ITEM );

    /* BUG-24416
     * smlLockMgr::registRecordLockWait() 에서 반드시
     * mWaitForTable[aSlot][aWaitSlot].mIndex 이 1로 설정되어야 합니다. */
    mWaitForTable[aSlot][aWaitSlot].mIndex = 1;

    if ( mWaitForTable[aSlot][aWaitSlot].mNxtWaitRecTransItem
         == ID_USHORT_MAX )
    {
        /* BUG-23823: Record Lock에 대한 대기 리스트를 Waiting순서대로 깨워
         * 주어야 한다.
         *
         * Wait할 Transaction을 Target Wait Transaction List의 마지막에 연결
         * 한다. */
        mWaitForTable[aSlot][aWaitSlot].mNxtWaitRecTransItem = SML_END_ITEM;

        if ( mArrOfLockList[aWaitSlot].mFstWaitRecTransItem == SML_END_ITEM )
        {
            IDE_ASSERT( mArrOfLockList[aWaitSlot].mLstWaitRecTransItem ==
                        SML_END_ITEM );

            mArrOfLockList[aWaitSlot].mFstWaitRecTransItem = aSlot;
            mArrOfLockList[aWaitSlot].mLstWaitRecTransItem = aSlot;
        }
        else
        {
            IDE_ASSERT( mArrOfLockList[aWaitSlot].mLstWaitRecTransItem !=
                        SML_END_ITEM );

            sLstSlot = mArrOfLockList[aWaitSlot].mLstWaitRecTransItem;

            mWaitForTable[sLstSlot][aWaitSlot].mNxtWaitRecTransItem = aSlot;
            mArrOfLockList[aWaitSlot].mLstWaitRecTransItem          = aSlot;
        }
    }

    IDE_ASSERT( mArrOfLockList[aSlot].mFstWaitTblTransItem == SML_END_ITEM );

    mWaitForTable[aSlot][aWaitSlot].mNxtWaitTransItem
        = mArrOfLockList[aSlot].mFstWaitTblTransItem;
    mArrOfLockList[aSlot].mFstWaitTblTransItem = aWaitSlot;
}

/*********************************************************
  function description: didLockReleased
  트랜잭션에 대응하는  aSlot이  aWaitSlot 트랜잭션에게
  record lock wait또는 table lock wait이 의미 clear되어
  있는지 확인한다.
  *********************************************************/
idBool  smlLockMgr::didLockReleasedMutex( SInt aSlot, SInt aWaitSlot )
{

    return (mWaitForTable[aSlot][aWaitSlot].mIndex == 0) ?  ID_TRUE: ID_FALSE;
}

/*********************************************************
  function description: freeAllRecordLock
  aSlot에 해당하는 트랜잭션을 record lock으로 waiting하고
  있는 트랜잭션들간의 대기 경로를 0으로 clear한다.
  record lock을 waiting하고 있는 트랜잭션을 resume시킨다.
***********************************************************/
IDE_RC  smlLockMgr::freeAllRecordLockMutex( SInt aSlot )
{

    UShort i;
    UShort  sNxtItem;
    void * sTrans;


    i = mArrOfLockList[aSlot].mFstWaitRecTransItem;

    while ( i != SML_END_ITEM )
    {
        IDE_ASSERT( i != ID_USHORT_MAX );

        sNxtItem = mWaitForTable[i][aSlot].mNxtWaitRecTransItem;

        mWaitForTable[i][aSlot].mNxtWaitRecTransItem = ID_USHORT_MAX;

        if ( mWaitForTable[i][aSlot].mIndex == 1 )
        {
            sTrans = smLayerCallback::getTransBySID( i );
            IDE_TEST( smLayerCallback::resumeTrans( sTrans ) != IDE_SUCCESS );
        }

        mWaitForTable[i][aSlot].mIndex = 0;

        i = sNxtItem;
    }

    /* PROJ-1381 FAC
     * Record Lock을 해제한 다음 초기화를 한다. */
    mArrOfLockList[aSlot].mFstWaitRecTransItem = SML_END_ITEM;
    mArrOfLockList[aSlot].mLstWaitRecTransItem = SML_END_ITEM;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: registTblLockWaitListByReq
  waiting table에서   트랜잭션의 slot id 행의 열들에
  request list에서  대기하고 있는트랙잭션들 의
  slot id를  table lock waiting 리스트에 등록한다.
***********************************************************/
void smlLockMgr::registTblLockWaitListByReq( SInt          aSlot,
                                             smlLockNode * aLockNode )
{

    UShort       sTransSlot;

    while ( aLockNode != NULL )
    {
        sTransSlot = aLockNode->mSlotID;
        if ( aSlot != sTransSlot )
        {
            IDE_ASSERT(mWaitForTable[aSlot][sTransSlot].mNxtWaitTransItem == ID_USHORT_MAX);

            mWaitForTable[aSlot][sTransSlot].mIndex = 1;
            mWaitForTable[aSlot][sTransSlot].mNxtWaitTransItem
                = mArrOfLockList[aSlot].mFstWaitTblTransItem;
            mArrOfLockList[aSlot].mFstWaitTblTransItem = sTransSlot;
        }
        aLockNode = aLockNode->mNxtLockNode;
    }
}

/*********************************************************
  function description: registTblLockWaitListByGrant
  waiting table에서   트랜잭션의 slot id 행에
  grant list에서     있는트랙잭션들 의
  slot id를  table lock waiting 리스트에 등록한다.
***********************************************************/
void smlLockMgr::registTblLockWaitListByGrant( SInt          aSlot,
                                               smlLockNode * aLockNode )
{

    UShort       sTransSlot;

    while ( aLockNode != NULL )
    {
        sTransSlot = aLockNode->mSlotID;
        if ( (aSlot != sTransSlot) &&
             (mWaitForTable[aSlot][sTransSlot].mNxtWaitTransItem == ID_USHORT_MAX) )
        {
            mWaitForTable[aSlot][sTransSlot].mIndex = 1;
            mWaitForTable[aSlot][sTransSlot].mNxtWaitTransItem
                = mArrOfLockList[aSlot].mFstWaitTblTransItem;
            mArrOfLockList[aSlot].mFstWaitTblTransItem = sTransSlot;
        } //if
        aLockNode = aLockNode->mNxtLockNode;
    }//while
}

/*********************************************************
  function description: setLockModeAndAddLockSlot
  트랜잭션이 이전에 table에 lock을 잡은 경우에 한하여,
  현재 트랜잭션의 lock mode를 setting하고,
  이번에도 lock을 잡았다면 lock slot을 트랜잭션의 lock
  node에 추가한다.
***********************************************************/
void smlLockMgr::setLockModeAndAddLockSlotMutex( SInt           aSlot,
                                                 smlLockNode  * aTxLockNode,
                                                 smlLockMode  * aCurLockMode,
                                                 smlLockMode    aLockMode,
                                                 idBool         aIsLocked,
                                                 smlLockSlot ** aLockSlot )
{
    if ( aTxLockNode != NULL )
    {
        if ( aCurLockMode != NULL )
        {
            *aCurLockMode = aTxLockNode->mLockMode;
        }
        // 트랜잭션의 lock slot list에  lock slot  추가 .
        if ( aIsLocked == ID_TRUE )
        {
            aTxLockNode->mFlag |= mLockModeToMask[aLockMode];
            addLockSlot( &(aTxLockNode->mArrLockSlotList[aLockMode]),
                         aSlot );

            if ( aLockSlot != NULL )
            {
                *aLockSlot = &(aTxLockNode->mArrLockSlotList[aLockMode]);
            }//if aLockSlot

        }//if aIsLocked
    }//aTxLockNode != NULL
}
