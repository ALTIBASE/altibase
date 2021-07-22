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
 * $Id: smlLockMgr.cpp 90597 2021-04-15 01:17:16Z emlee $
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
#include <smErrorCode.h>
#include <smDef.h>
#include <smr.h>
#include <smc.h>
#include <sml.h>
#include <smlReq.h>
#include <smu.h>
#include <sct.h>
#include <smx.h>

/*
   락 호환성 행렬
   가로 - 현재 걸려있는 락타입
   세로 - 새로운 락타입
*/
idBool smlLockMgr::mCompatibleTBL[SML_NUMLOCKTYPES][SML_NUMLOCKTYPES] = {
/*                   SML_NLOCK SML_SLOCK SML_XLOCK SML_ISLOCK SML_IXLOCK SML_SIXLOCK */
/* for SML_NLOCK  */{ID_TRUE,  ID_TRUE,  ID_TRUE,  ID_TRUE,   ID_TRUE,   ID_TRUE},
/* for SML_SLOCK  */{ID_TRUE,  ID_TRUE,  ID_FALSE, ID_TRUE,   ID_FALSE,  ID_FALSE},
/* for SML_XLOCK  */{ID_TRUE,  ID_FALSE, ID_FALSE, ID_FALSE,  ID_FALSE,  ID_FALSE},
/* for SML_ISLOCK */{ID_TRUE,  ID_TRUE,  ID_FALSE, ID_TRUE,   ID_TRUE,   ID_TRUE},
/* for SML_IXLOCK */{ID_TRUE,  ID_FALSE, ID_FALSE, ID_TRUE,   ID_TRUE,   ID_FALSE},
/* for SML_SIXLOCK*/{ID_TRUE,  ID_FALSE, ID_FALSE, ID_TRUE,   ID_FALSE,  ID_FALSE}
};
/*
   락 변환 행렬
   가로 - 현재 걸려있는 락타입
   세로 - 새로운 락타입
*/
smlLockMode smlLockMgr::mConversionTBL[SML_NUMLOCKTYPES][SML_NUMLOCKTYPES] = {
/*                   SML_NLOCK    SML_SLOCK    SML_XLOCK  SML_ISLOCK   SML_IXLOCK   SML_SIXLOCK */
/* for SML_NLOCK  */{SML_NLOCK,   SML_SLOCK,   SML_XLOCK, SML_ISLOCK,  SML_IXLOCK,  SML_SIXLOCK},
/* for SML_SLOCK  */{SML_SLOCK,   SML_SLOCK,   SML_XLOCK, SML_SLOCK,   SML_SIXLOCK, SML_SIXLOCK},
/* for SML_XLOCK  */{SML_XLOCK,   SML_XLOCK,   SML_XLOCK, SML_XLOCK,   SML_XLOCK,   SML_XLOCK},
/* for SML_ISLOCK */{SML_ISLOCK,  SML_SLOCK,   SML_XLOCK, SML_ISLOCK,  SML_IXLOCK,  SML_SIXLOCK},
/* for SML_IXLOCK */{SML_IXLOCK,  SML_SIXLOCK, SML_XLOCK, SML_IXLOCK,  SML_IXLOCK,  SML_SIXLOCK},
/* for SML_SIXLOCK*/{SML_SIXLOCK, SML_SIXLOCK, SML_XLOCK, SML_SIXLOCK, SML_SIXLOCK, SML_SIXLOCK}
};

/*
   락 Mode결정 테이블
*/
smlLockMode smlLockMgr::mDecisionTBL[SML_DECISION_TBL_SIZE] = {
    SML_NLOCK,   SML_SLOCK,   SML_XLOCK, SML_XLOCK,
    SML_ISLOCK,  SML_SLOCK,   SML_XLOCK, SML_XLOCK,
    SML_IXLOCK,  SML_SIXLOCK, SML_XLOCK, SML_XLOCK,
    SML_IXLOCK,  SML_SIXLOCK, SML_XLOCK, SML_XLOCK,
    SML_SIXLOCK, SML_SIXLOCK, SML_XLOCK, SML_XLOCK,
    SML_SIXLOCK, SML_SIXLOCK, SML_XLOCK, SML_XLOCK,
    SML_SIXLOCK, SML_SIXLOCK, SML_XLOCK, SML_XLOCK,
    SML_SIXLOCK, SML_SIXLOCK, SML_XLOCK, SML_XLOCK
};

/*
   락 Mode에 따른 Lock Mask결정 테이블
*/
SInt smlLockMgr::mLockModeToMask[SML_NUMLOCKTYPES] = {
    /* for SML_NLOCK  */ 0x00000000,
    /* for SML_SLOCK  */ 0x00000001,
    /* for SML_XLOCK  */ 0x00000002,
    /* for SML_ISLOCK */ 0x00000004,
    /* for SML_IXLOCK */ 0x00000008,
    /* for SML_SIXLOCK*/ 0x00000010
};

smlLockMode2StrTBL smlLockMgr::mLockMode2StrTBL[SML_NUMLOCKTYPES] ={
    {SML_NLOCK,"NO_LOCK"},
    {SML_SLOCK,"S_LOCK"},
    {SML_XLOCK,"X_LOCK"},
    {SML_ISLOCK,"IS_LOCK"},
    {SML_IXLOCK,"IX_LOCK"},
    {SML_SIXLOCK,"SIX_LOCK"}
};

/* PROJ-2734
   mDistDeadlockRisk[WAITER TX][HOLDER TX]

   자원대기TX(waiter)와 자원보유TX(holder)의 분산LEVEL 조합별 분산데드락 실제 발생 위험률.
   분산데드락이 탐지되었을때 실제 분산데드락일 확률이 높을수록 HIGH로 설정하였다.
 */
smlDistDeadlockRiskType smlLockMgr::mDistDeadlockRisk[SMI_DIST_LEVEL_MAX][SMI_DIST_LEVEL_MAX] =
{
    /* WAITER TX : SMI_DIST_LEVEL_NONE     */
    { SML_DIST_DEADLOCK_RISK_NONE, SML_DIST_DEADLOCK_RISK_NONE, SML_DIST_DEADLOCK_RISK_NONE, SML_DIST_DEADLOCK_RISK_NONE },

    /* WAITER TX : SMI_DIST_LEVEL_SINGLE   */
    { SML_DIST_DEADLOCK_RISK_NONE, SML_DIST_DEADLOCK_RISK_LOW, SML_DIST_DEADLOCK_RISK_LOW,  SML_DIST_DEADLOCK_RISK_LOW },

    /* WAITER TX : SMI_DIST_LEVEL_MULTI    */
    { SML_DIST_DEADLOCK_RISK_NONE, SML_DIST_DEADLOCK_RISK_LOW, SML_DIST_DEADLOCK_RISK_LOW,  SML_DIST_DEADLOCK_RISK_MID },

    /* WAITER TX : SMI_DIST_LEVEL_PARALLEL */
    { SML_DIST_DEADLOCK_RISK_NONE, SML_DIST_DEADLOCK_RISK_MID, SML_DIST_DEADLOCK_RISK_MID,  SML_DIST_DEADLOCK_RISK_HIGH }
};

SInt                    smlLockMgr::mTransCnt;
iduMemPool              smlLockMgr::mLockPool;
smlLockMatrixItem   **  smlLockMgr::mWaitForTable;


smiLockWaitFunc         smlLockMgr::mLockWaitFunc;
smiLockWakeupFunc       smlLockMgr::mLockWakeupFunc;
smlTransLockList     *  smlLockMgr::mArrOfLockList;
iduMutex             *  smlLockMgr::mArrOfLockListMutex;

smlAllocLockNodeFunc    smlLockMgr::mAllocLockNodeFunc;
smlFreeLockNodeFunc     smlLockMgr::mFreeLockNodeFunc;

smlLockNode         **  smlLockMgr::mNodeCache;
smlLockNode          *  smlLockMgr::mNodeCacheArray;
ULong                *  smlLockMgr::mNodeAllocMap;

/* PROJ-2734 */
smlDistDeadlockNode  ** smlLockMgr::mTxList4DistDeadlock; /* 분산데드락 체크 대상이 될 Tx */

static IDE_RC smlLockWaitNAFunction( ULong, idBool * )
{
    return IDE_SUCCESS;
}

static IDE_RC smlLockWakeupNAFunction()
{
    return IDE_SUCCESS;
}


IDE_RC smlLockMgr::initialize( UInt              aTransCnt,
                               smiLockWaitFunc   aLockWaitFunc,
                               smiLockWakeupFunc aLockWakeupFunc )
{
    SInt               i;
    SInt               j;
    smlTransLockList * sTransLockList; /* BUG-43408 */
    SChar sBuffer[128];

    IDE_ASSERT( aTransCnt > 0 );

    mTransCnt = aTransCnt;

    /* TC/FIT/Limit/sm/sml/smlLockMgr_initialize_calloc1.sql */
    IDU_FIT_POINT_RAISE( "smlLockMgr::initialize::calloc1",
                          insufficient_memory );

    IDE_TEST( mLockPool.initialize( IDU_MEM_SM_SML,
                                    (SChar*)"LOCK_MEMORY_POOL",
                                    ID_SCALABILITY_SYS,
                                    sizeof(smlLockNode),
                                    SML_LOCK_POOL_SIZE,
                                    IDU_AUTOFREE_CHUNK_LIMIT,           /* ChunkLimit */
                                    ID_TRUE,                            /* UseMutex */
                                    IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                    ID_FALSE,                           /* ForcePooling */
                                    ID_TRUE,                            /* GarbageCollection */
                                    ID_TRUE,                            /* HWCacheLine */
                                    IDU_MEMPOOL_TYPE_LEGACY             /* mempool type */)
              != IDE_SUCCESS);

    // allocate transLock List array.
    mArrOfLockList = NULL;

    /* TC/FIT/Limit/sm/sml/smlLockMgr_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "smlLockMgr::initialize::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       (ULong)sizeof(smlTransLockList) * mTransCnt,
                                       (void**)&mArrOfLockList ) != IDE_SUCCESS,
                    insufficient_memory );

    /* PROJ-2734 */
    mTxList4DistDeadlock = NULL;
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       ID_SIZEOF(smlDistDeadlockNode *) * mTransCnt,
                                       (void**)&mTxList4DistDeadlock ) != IDE_SUCCESS,
                    insufficient_memory );
    for ( i = 0; i < mTransCnt; i++ )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                           ID_SIZEOF(smlDistDeadlockNode) * mTransCnt,
                                           (void**)&(mTxList4DistDeadlock[i]) ) != IDE_SUCCESS,
                        insufficient_memory );
    }

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       (ULong)sizeof(iduMutex) * mTransCnt,
                                       (void**)&mArrOfLockListMutex ) != IDE_SUCCESS,
                    insufficient_memory );

    mLockWaitFunc = ( (aLockWaitFunc == NULL) ?
                      smlLockWaitNAFunction : aLockWaitFunc );

    mLockWakeupFunc = ( (aLockWakeupFunc == NULL) ?
                        smlLockWakeupNAFunction : aLockWakeupFunc );

    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        idlOS::snprintf( sBuffer, 128, "TRANS_LOCK_NODE_LIST_MUTEX_%"ID_INT32_FMT"\0", i );
        IDE_TEST( mArrOfLockListMutex[i].initialize( sBuffer,
                                                     IDU_MUTEX_KIND_NATIVE,
                                                     IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
        initTransLockList(i);
    }

    switch( smuProperty::getLockMgrCacheNode() )
    {
    case 0:
        mAllocLockNodeFunc  = allocLockNodeNormal;
        mFreeLockNodeFunc   = freeLockNodeNormal;
        break;

    case 1:
        for ( i = 0 ; i < mTransCnt ; i++ )
        {
            sTransLockList = (mArrOfLockList+i);
            IDE_TEST_RAISE( initTransLockNodeCache( i,
                                                    &( sTransLockList->mLockNodeCache ) )
                            != IDE_SUCCESS,
                            insufficient_memory );
        }

        mAllocLockNodeFunc  = allocLockNodeList;
        mFreeLockNodeFunc   = freeLockNodeList;
        break;

    case 2:
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                           sizeof(smlLockNode) * mTransCnt * 64,
                                           (void**)&mNodeCacheArray ) != IDE_SUCCESS,
                        insufficient_memory );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                           sizeof(smlLockNode*) * mTransCnt,
                                           (void**)&mNodeCache ) != IDE_SUCCESS,
                        insufficient_memory );
        IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SML,
                                           mTransCnt,
                                           sizeof(ULong),
                                           (void**)&mNodeAllocMap ) != IDE_SUCCESS,
                        insufficient_memory );

        for ( i = 0 ; i < mTransCnt ; i++ )
        {
            mNodeCache[i] = &(mNodeCacheArray[i * 64]);
        }

        mAllocLockNodeFunc  = allocLockNodeBitmap;
        mFreeLockNodeFunc   = freeLockNodeBitmap;

        break;

    }

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

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-43408 */
IDE_RC smlLockMgr::initTransLockNodeCache( SInt      aSlotID,
                                           iduList * aTxSlotLockNodeBitmap )
{
    smlLockNode * sLockNode;
    UInt          i;
    UInt          sLockNodeCacheCnt;

    /* BUG-43408 Property로 초기 할당 갯수를 결정 */
    sLockNodeCacheCnt = smuProperty::getLockNodeCacheCount();

    IDU_LIST_INIT( aTxSlotLockNodeBitmap );

    for ( i = 0 ; i < sLockNodeCacheCnt ; i++ )
    {
        IDE_TEST( mLockPool.alloc( (void**)&sLockNode )
                  != IDE_SUCCESS );
        sLockNode->mSlotID = aSlotID;
        IDU_LIST_INIT_OBJ( &(sLockNode->mNode4LockNodeCache), sLockNode );
        IDU_LIST_ADD_AFTER( aTxSlotLockNodeBitmap, &(sLockNode->mNode4LockNodeCache) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smlLockMgr::destroy()
{
    iduListNode*        sIterator = NULL;
    iduListNode*        sNodeNext = NULL;
    iduList*            sLockNodeCache;
    smlLockNode*        sLockNode;
    smlTransLockList*   sTransLockList;
    SInt                i;

    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        IDE_TEST( iduMemMgr::free(mWaitForTable[i]) != IDE_SUCCESS );
    }

    IDE_TEST( iduMemMgr::free(mWaitForTable) != IDE_SUCCESS );

    switch(smuProperty::getLockMgrCacheNode())
    {
    case 0:
        /* do nothing */
        break;

    case 1:
        for ( i = 0 ; i < mTransCnt ; i++ )
        {
            sTransLockList = (mArrOfLockList+i);
            sLockNodeCache = &(sTransLockList->mLockNodeCache);
            IDU_LIST_ITERATE_SAFE( sLockNodeCache, sIterator, sNodeNext )
            {
                sLockNode = (smlLockNode*) (sIterator->mObj);
                IDE_TEST( mLockPool.memfree(sLockNode) != IDE_SUCCESS );
            }
        }
        break;

    case 2:
        IDE_TEST( iduMemMgr::free(mNodeCacheArray) != IDE_SUCCESS );
        IDE_TEST( iduMemMgr::free(mNodeCache) != IDE_SUCCESS );
        IDE_TEST( iduMemMgr::free(mNodeAllocMap) != IDE_SUCCESS );
        break;
    }

    IDE_TEST( mLockPool.destroy() != IDE_SUCCESS );

    mLockWaitFunc = NULL;

    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        IDE_TEST( mArrOfLockListMutex[i].destroy() != IDE_SUCCESS );
    }
    IDE_TEST( iduMemMgr::free( mArrOfLockListMutex ) != IDE_SUCCESS );
    IDE_TEST( iduMemMgr::free( mArrOfLockList ) != IDE_SUCCESS );

    /* PROJ-2734 */
    for ( i = 0 ; i < mTransCnt ; i++ )
    {
        IDE_TEST( iduMemMgr::free(mTxList4DistDeadlock[i]) != IDE_SUCCESS );
    }

    IDE_TEST( iduMemMgr::free( mTxList4DistDeadlock ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: initTransLockList
  Transaction lock list array에서 aSlot에 해당하는
  smlTransLockList의 초기화를 한다.
  - Tx의 맨처음 table lock 대기 item
  - Tx의 맨처음 record lock대기 item.
  - Tx의 lock node의 list 초기화.
  - Tx의 lock slot list초기화.
***********************************************************/
void  smlLockMgr::initTransLockList( SInt aSlot )
{
    smlTransLockList * sTransLockList = (mArrOfLockList+aSlot);

    sTransLockList->mFstWaitTblTransItem  = SML_END_ITEM;
    sTransLockList->mFstWaitRecTransItem  = SML_END_ITEM;
    sTransLockList->mLstWaitRecTransItem  = SML_END_ITEM;

    sTransLockList->mLockSlotHeader.mPrvLockSlot  = &(sTransLockList->mLockSlotHeader);
    sTransLockList->mLockSlotHeader.mNxtLockSlot  = &(sTransLockList->mLockSlotHeader);

    sTransLockList->mLockSlotHeader.mLockNode     = NULL;
    sTransLockList->mLockSlotHeader.mLockSequence = 0;

    sTransLockList->mLockNodeHeader.mPrvTransLockNode = &(sTransLockList->mLockNodeHeader);
    sTransLockList->mLockNodeHeader.mNxtTransLockNode = &(sTransLockList->mLockNodeHeader);
}


void smlLockMgr::getTxLockInfo( SInt aSlot, smTID *aOwnerList, UInt *aOwnerCount )
{

    UInt      i;
    void*   sTrans;

    if ( aSlot >= 0 )
    {

        for ( i = mArrOfLockList[aSlot].mFstWaitTblTransItem;
              (i != SML_END_ITEM) && (i != ID_USHORT_MAX);
              i = mWaitForTable[aSlot][i].mNxtWaitTransItem )
        {
            if ( mWaitForTable[aSlot][i].mIndex == 1 )
            {
                sTrans = smLayerCallback::getTransBySID( i );
                aOwnerList[*aOwnerCount] = smLayerCallback::getTransID( sTrans );
                (*aOwnerCount)++;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
}

/*********************************************************
  function description: addLockNode
  트랜잭션이 table에 대하여 lock을 쥐게 되는 경우,
  그 table의 grant list  lock node를  자신의 트랜잭션
  lock list에 추가한다.

  lock을 잡게 되는 경우는 아래의 두가지 경우이다.
  1. 트랜잭션이 table에 대하여 lock이 grant되어
     lock을 잡게 되는 경우.
  2. lock waiting하고 있다가 lock을 잡고 있었던  다른 트랜잭션
     이 commit or rollback을 수행하여,  wakeup되는 경우.
***********************************************************/
void smlLockMgr::addLockNode( smlLockNode *aLockNode, SInt aSlot )
{
    smlLockNode*  sTransLockNodeHdr = &(mArrOfLockList[aSlot].mLockNodeHeader);

    IDE_ASSERT( NULL == aLockNode->mPrvTransLockNode );
    IDE_ASSERT( NULL == aLockNode->mNxtTransLockNode );
    IDE_ASSERT( ID_FALSE == aLockNode->mDoRemove );
    IDE_ASSERT( aLockNode->mBeGrant  == ID_TRUE );

    IDE_DASSERT( isLockNodeExist( aLockNode ) == ID_FALSE );

    aLockNode->mNxtTransLockNode = sTransLockNodeHdr;
    aLockNode->mPrvTransLockNode = sTransLockNodeHdr->mPrvTransLockNode;

    sTransLockNodeHdr->mPrvTransLockNode->mNxtTransLockNode = aLockNode;
    sTransLockNodeHdr->mPrvTransLockNode  = aLockNode;

    IDE_DASSERT( isLockNodeExist( aLockNode ) == ID_TRUE );
    IDE_ASSERT( NULL != aLockNode->mPrvTransLockNode );
    IDE_ASSERT( NULL != aLockNode->mNxtTransLockNode );
    IDE_ASSERT( aLockNode != aLockNode->mPrvTransLockNode );
    IDE_ASSERT( aLockNode != aLockNode->mNxtTransLockNode );
    IDE_ASSERT( aLockNode->mNxtTransLockNode->mPrvTransLockNode == aLockNode );
    IDE_ASSERT( aLockNode->mPrvTransLockNode->mNxtTransLockNode == aLockNode );
    IDE_ASSERT( aLockNode->mNxtTransLockNode->mPrvTransLockNode != aLockNode->mPrvTransLockNode );
    IDE_ASSERT( aLockNode->mPrvTransLockNode->mNxtTransLockNode != aLockNode->mNxtTransLockNode );
}

/*********************************************************
  function description: removeLockNode
  트랜잭션 lock list array에서  transaction의 slotid에
  해당하는 list에서 lock node를 제거한다.
***********************************************************/
void smlLockMgr::removeLockNode( smlLockNode *aLockNode )
{
    IDE_DASSERT( isLockNodeExist( aLockNode ) == ID_TRUE );
    IDE_ASSERT( aLockNode->mDoRemove == ID_FALSE );
    IDE_ASSERT( aLockNode->mBeGrant  == ID_TRUE );
    IDE_ASSERT( NULL != aLockNode->mPrvTransLockNode );
    IDE_ASSERT( NULL != aLockNode->mNxtTransLockNode );
    IDE_ASSERT( aLockNode != aLockNode->mPrvTransLockNode );
    IDE_ASSERT( aLockNode != aLockNode->mNxtTransLockNode );
    IDE_ASSERT( aLockNode->mNxtTransLockNode->mPrvTransLockNode == aLockNode );
    IDE_ASSERT( aLockNode->mPrvTransLockNode->mNxtTransLockNode == aLockNode );

    IDE_ASSERT( aLockNode->mNxtTransLockNode->mPrvTransLockNode != aLockNode->mPrvTransLockNode );
    IDE_ASSERT( aLockNode->mPrvTransLockNode->mNxtTransLockNode != aLockNode->mNxtTransLockNode );

    aLockNode->mNxtTransLockNode->mPrvTransLockNode = aLockNode->mPrvTransLockNode;
    aLockNode->mPrvTransLockNode->mNxtTransLockNode = aLockNode->mNxtTransLockNode;

    aLockNode->mPrvTransLockNode = NULL;
    aLockNode->mNxtTransLockNode = NULL;

    IDE_DASSERT( isLockNodeExist( aLockNode ) == ID_FALSE );
}

/*********************************************************
  PROJ-1381 Fetch Across Commits
  function description: freeAllItemLockExceptIS
  aSlot id에 해당하는 트랜잭션이 grant되어 lock을 잡고
  있는 lock중 IS lock을 제외하고 해제한다.

  가장 마지막에 잡았던 lock부터 lock을 해제한다.
***********************************************************/
IDE_RC smlLockMgr::freeAllItemLockExceptIS( SInt aSlot )
{
    smlLockSlot *sCurLockSlot;
    smlLockSlot *sPrvLockSlot;
    smlLockSlot *sHeadLockSlot;
    static SInt  sISLockMask = smlLockMgr::mLockModeToMask[SML_ISLOCK];

    sHeadLockSlot = &(mArrOfLockList[aSlot].mLockSlotHeader);

    sCurLockSlot = (smlLockSlot*)getLastLockSlotPtr(aSlot);

    while ( sCurLockSlot != sHeadLockSlot )
    {
        sPrvLockSlot = sCurLockSlot->mPrvLockSlot;

        if ( sISLockMask != sCurLockSlot->mMask ) /* IS Lock이 아니면 */
        {
            IDE_TEST( smlLockMgr::unlockTable(aSlot, NULL, sCurLockSlot)
                      != IDE_SUCCESS );
        }
        else
        {
            /* IS Lock인 경우 계속 fetch를 수행하기 위해서 남겨둔다. */
        }

        sCurLockSlot = sPrvLockSlot;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: freeAllItemLock
  aSlot id에 해당하는 트랜잭션이 grant되어 lock을 잡고
  있는 lock들 을 해제한다.

  lock 해제 순서는 가장 마지막에 잡았던 lock부터 풀기
  시작한다.
*******************m***************************************/
IDE_RC smlLockMgr::freeAllItemLock( SInt aSlot )
{

    smlLockNode *sCurLockNode;
    smlLockNode *sPrvLockNode;

    smlLockNode * sTransLockNodeHdr = &(mArrOfLockList[aSlot].mLockNodeHeader);

    sCurLockNode = sTransLockNodeHdr->mPrvTransLockNode;

    while( sCurLockNode != sTransLockNodeHdr )
    {
        sPrvLockNode = sCurLockNode->mPrvTransLockNode;


        IDE_TEST( smlLockMgr::unlockTable( aSlot, sCurLockNode, NULL )
                  != IDE_SUCCESS );
        sCurLockNode = sPrvLockNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: partialTableUnlock
  트랜잭션 aSlot에 해당하는 lockSlot list에서
  가장 마지막 lock slot부터  aLockSlot까지 꺼꾸로 올라가면서
  aLockSlot이 속해있는 lock node를 이용하여 table unlock을
  수행한다. 이때 Lock Slot의 Sequence 가 aLockSequence보다
  클때까지만 Unlock을 수행한다.

  aSlot          - [IN] aSlot에 해당하는 Transaction지정
  aLockSequence  - [IN] LockSlot의 mSequence의 값이 aLockSequence
                        보다 작을때까지  Unlock을 수행한다.
  aIsSeveralLock  - [IN] ID_FALSE:모든 Lock을 해제.
                         ID_TRUE :Implicit Savepoint까지 모든 테이블에 대해
                                  IS락을 해제하고,temp tbs일경우는 IX락도해제.

***********************************************************/
IDE_RC smlLockMgr::partialItemUnlock( SInt   aSlot,
                                      ULong  aLockSequence,
                                      idBool aIsSeveralLock )
{
    smlLockSlot *sCurLockSlot;
    smlLockSlot *sPrvLockNode;
    static SInt  sISLockMask = smlLockMgr::mLockModeToMask[SML_ISLOCK];
    static SInt  sIXLockMask = smlLockMgr::mLockModeToMask[SML_IXLOCK];
    scSpaceID    sSpaceID;
    IDE_RC       sRet = IDE_SUCCESS;

    sCurLockSlot = (smlLockSlot*)getLastLockSlotPtr(aSlot);

    while ( sCurLockSlot->mLockSequence > aLockSequence )
    {
        sPrvLockNode = sCurLockSlot->mPrvLockSlot;

        if ( aIsSeveralLock == ID_FALSE )
        {
            // Abort Savepoint등의 일반적인 경우 Partial Unlock
            // Lock의 종류와 상관없이 LockSequence로 범위를 측정하여 모두 unlock
            IDE_TEST( smlLockMgr::unlockTable(aSlot, NULL, sCurLockSlot)
                      != IDE_SUCCESS );
        }
        else
        {
            // Statement End시 호출되는 경우
            // Implicit IS Lock와, TableTable의 IX Lock을 Unlock

            if ( sISLockMask == sCurLockSlot->mMask ) // IS Lock의 경우
            {
                /* BUG-15906: non-autocommit모드에서 select완료후 IS_LOCK이 해제되면
                   좋겠습니다.
                   aPartialLock가 ID_TRUE이면 IS_LOCK만을 해제하도록 함. */
                // BUG-28752 lock table ... in row share mode 구문이 먹히지 않습니다.
                // Implicit IS lock만 풀어줍니다.

                if ( sCurLockSlot->mLockNode->mIsExplicitLock != ID_TRUE )
                {
                    IDE_TEST( smlLockMgr::unlockTable(aSlot, NULL, sCurLockSlot)
                              != IDE_SUCCESS );
                }
            }
            else if ( sIXLockMask == sCurLockSlot->mMask ) //IX Lock의 경우
            {
                /* BUG-21743
                 * Select 연산에서 User Temp TBS 사용시 TBS에 Lock이 안풀리는 현상 */
                sSpaceID = sCurLockSlot->mLockNode->mSpaceID;

                if ( sctTableSpaceMgr::isTempTableSpace( sSpaceID ) == ID_TRUE )
                {
                    IDE_TEST( smlLockMgr::unlockTable(aSlot, NULL, sCurLockSlot)
                              != IDE_SUCCESS );
                }
            }
        }

        sCurLockSlot = sPrvLockNode;
    }

    return sRet;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* BUG-18981 */
IDE_RC  smlLockMgr::logLocks( smxTrans * aTrans, ID_XID * aXID )
{
    smrXaPrepareLog  *sPrepareLog;
    smlTableLockInfo  sLockInfo;
    SChar            *sLogBuffer;
    SChar            *sLog;
    UInt              sLockCount = 0;
    idBool            sIsLogged = ID_FALSE;
    smlLockNode      *sCurLockNode;
    smlLockNode      *sPrvLockNode;
    PDL_Time_Value    sTmv;
    smLSN             sEndLSN;
    SInt              sNeedBufferSize;

#ifdef DEBUG
    UChar             sXidString[SMR_XID_DATA_MAX_LEN];

    (void)idaXaConvertXIDToString(NULL, aXID, sXidString, SMR_XID_DATA_MAX_LEN);

    ideLog::log( IDE_SD_19,
                 "\n<SMR_LT_XA_PREPARE Logging> "
                 "LocalID: %"ID_UINT32_FMT", "
                 "XID: %s, "
                 "GCTX: %"ID_INT32_FMT"\n",
                 aTrans->mTransID,
                 sXidString,
                 aTrans->mIsGCTx );
#endif

    /* -------------------------------------------------------------------
       BUG-48307 : LOG BUFFER 크기가 충분한지 확인하고, 부족시 증가시킨다.
       ------------------------------------------------------------------- */
    /* 1. LOCK의 총갯수를 먼저 구한다. */
    sCurLockNode = mArrOfLockList[aTrans->mSlotN].mLockNodeHeader.mPrvTransLockNode;
    while ( sCurLockNode != &(mArrOfLockList[aTrans->mSlotN].mLockNodeHeader) )
    {
        sPrvLockNode = sCurLockNode->mPrvTransLockNode;

        // 테이블 스페이스 관련 DDL은 XA를 지원하지 않는다.
        if ( sCurLockNode->mLockItemType != SMI_LOCK_ITEM_TABLE )
        {
            sCurLockNode = sPrvLockNode;
            continue;
        }

        sCurLockNode = sPrvLockNode;
        sLockCount++;
    }
    /* 2. 필요한 LOG BUFFER SIZE를 계산한다. */
    if ( sLockCount >= SML_MAX_LOCK_INFO )
    {
        sNeedBufferSize = SMR_LOGREC_SIZE(smrXaPrepareLog)
                          + (SML_MAX_LOCK_INFO * sizeof(smlTableLockInfo))
                          + sizeof(smrLogTail);
    }
    else
    {
        sNeedBufferSize = SMR_LOGREC_SIZE(smrXaPrepareLog)
                          + (sLockCount * sizeof(smlTableLockInfo))
                          + sizeof(smrLogTail);
    }
    /* 3. 필요한 LOG BUFER SIZE가 더 큰경우 LOG BUFFER SIZE를 늘린다. */
    if ( smxTrans::getLogBufferSize( aTrans ) < sNeedBufferSize )
    {
        aTrans->setLogBufferSize( sNeedBufferSize );
    }

    /* -------------------------------------------------------------------
       xid를 로그데이타로 기록
       ------------------------------------------------------------------- */
    sLockCount = 0;
    sLogBuffer = aTrans->getLogBuffer();
    sPrepareLog = (smrXaPrepareLog*)sLogBuffer;
    smrLogHeadI::setTransID(&sPrepareLog->mHead, aTrans->mTransID);
    smrLogHeadI::setFlag(&sPrepareLog->mHead, aTrans->mLogTypeFlag);
    smrLogHeadI::setType(&sPrepareLog->mHead, SMR_LT_XA_PREPARE);

    // BUG-27024 XA에서 Prepare 후 Commit되지 않은 Disk Row를
    //           Server Restart 시 고려햐여야 합니다.
    // XA Trans의 FstDskViewSCN을 Log에 기록하여,
    // Restart시 XA Prepare Trans 재구축에 사용
    SM_SET_SCN( &sPrepareLog->mFstDskViewSCN, &aTrans->mFstDskViewSCN );

    if ( (smrLogHeadI::getFlag(&sPrepareLog->mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth( &sPrepareLog->mHead,
                                       smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sPrepareLog->mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    // prepared된 시점을 로깅함
    // 왜냐하면 heuristic commit/rollback 지원위해 timeout 기법을 사용하는데
    // system failure 이후에도 prepared된 정확한 시점을 보장하기 위함임.
    /* BUG-18981 */
    idlOS::memcpy(&(sPrepareLog->mXaTransID), aXID, sizeof(ID_XID));
    sTmv                       = idlOS::gettimeofday();
    sPrepareLog->mPreparedTime = (timeval)sTmv;
    sPrepareLog->mIsGCTx       = aTrans->mIsGCTx;
    /* -------------------------------------------------------------------
       table lock을 prepare log의 데이타로 로깅
       record lock과 OID 정보는 재시작 회복의 재수행 단계에서 수집해야 함
       ------------------------------------------------------------------- */
    sLog         = sLogBuffer + SMR_LOGREC_SIZE(smrXaPrepareLog);
    sCurLockNode = mArrOfLockList[aTrans->mSlotN].mLockNodeHeader.mPrvTransLockNode;

    while ( sCurLockNode != &(mArrOfLockList[aTrans->mSlotN].mLockNodeHeader) )
    {
        sPrvLockNode = sCurLockNode->mPrvTransLockNode;

        // 테이블 스페이스 관련 DDL은 XA를 지원하지 않는다.
        if ( sCurLockNode->mLockItemType != SMI_LOCK_ITEM_TABLE )
        {
            sCurLockNode = sPrvLockNode;
            continue;
        }

        sLockInfo.mOidTable = (smOID)sCurLockNode->mLockItem->mItemID;
        sLockInfo.mLockMode = sCurLockNode->mLockMode;

        idlOS::memcpy(sLog, &sLockInfo, sizeof(smlTableLockInfo));
        sLog        += sizeof( smlTableLockInfo );
        sCurLockNode = sPrvLockNode;
        sLockCount++;

        if ( sLockCount >= SML_MAX_LOCK_INFO )
        {
            smrLogHeadI::setSize( &sPrepareLog->mHead,
                                  SMR_LOGREC_SIZE(smrXaPrepareLog) +
                                   + (sLockCount * sizeof(smlTableLockInfo))
                                   + sizeof(smrLogTail));

            sPrepareLog->mLockCount = sLockCount;

            smrLogHeadI::copyTail( (SChar*)sLogBuffer +
                                   smrLogHeadI::getSize(&sPrepareLog->mHead) -
                                   sizeof(smrLogTail),
                                   &(sPrepareLog->mHead) );

            IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                           aTrans,
                                           (SChar*)sLogBuffer,
                                           NULL,  // Previous LSN Ptr
                                           NULL,  // Log LSN Ptr
                                           NULL,  // End LSN Ptr
                                           SM_NULL_OID )
                     != IDE_SUCCESS );

            sLockCount = 0;
            sIsLogged  = ID_TRUE;
            sLog       = sLogBuffer + SMR_LOGREC_SIZE(smrXaPrepareLog);
        }
    }

    if ( !( (sIsLogged == ID_TRUE) && (sLockCount == 0) ) )
    {
        smrLogHeadI::setSize(&sPrepareLog->mHead,
                             SMR_LOGREC_SIZE(smrXaPrepareLog)
                                + sLockCount * sizeof(smlTableLockInfo)
                                + sizeof(smrLogTail));

        sPrepareLog->mLockCount = sLockCount;

        smrLogHeadI::copyTail( (SChar*)sLogBuffer +
                               smrLogHeadI::getSize(&sPrepareLog->mHead) -
                               sizeof(smrLogTail),
                               &(sPrepareLog->mHead) );

        IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                       aTrans,
                                       (SChar*)sLogBuffer,
                                       NULL,      // Previous LSN Ptr
                                       NULL,      // Log LSN Ptr
                                       &sEndLSN,  // End LSN Ptr
                                       SM_NULL_OID )
                  != IDE_SUCCESS );

        if ( smLayerCallback::isNeedLogFlushAtCommitAPrepare( aTrans )
             == ID_TRUE )
        {
            IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_TRX,
                                               &sEndLSN )
                      != IDE_SUCCESS );
        }
    }

    IDE_TEST( smrRecoveryMgr::mIsReplWaitGlobalTxAfterPrepareFunc( NULL, /* idvSQL* */
                                                                   ID_FALSE, /* isRequestNode */
                                                                   aTrans->mTransID,
                                                                   SM_MAKE_SN(sEndLSN) ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void  smlLockMgr::lockTableByPreparedLog( void  * aTrans,
                                          SChar * aLogPtr,
                                          UInt    aLockCnt,
                                          UInt  * aOffset )
{

    UInt i;
    smlTableLockInfo  sLockInfo;
    smOID sTableOID;
    smcTableHeader   *sTableHeader;
    smlLockItem      *sLockItem    = NULL;

    for ( i = 0 ; i < aLockCnt ; i++ )
    {

        idlOS::memcpy( &sLockInfo,
                       (SChar *)((aLogPtr) + *aOffset),
                       sizeof(smlTableLockInfo) );
        sTableOID = sLockInfo.mOidTable;
        IDE_ASSERT( smcTable::getTableHeaderFromOID( sTableOID,
                                                     (void**)&sTableHeader )
                    == IDE_SUCCESS );

        sLockItem = (smlLockItem *)SMC_TABLE_LOCK( sTableHeader );
        IDE_ASSERT( sLockItem->mLockItemType == SMI_LOCK_ITEM_TABLE );

        smlLockMgr::lockTable( smLayerCallback::getTransSlot( aTrans ),
                               sLockItem,
                               sLockInfo.mLockMode );

        *aOffset += sizeof(smlTableLockInfo);
    }
}

void smlLockMgr::initLockNode( smlLockNode *aLockNode )
{
    SInt          i;
    smlLockSlot  *sLockSlotList;

    aLockNode->mLockMode          = SML_NLOCK;
    aLockNode->mPrvLockNode       = NULL;
    aLockNode->mNxtLockNode       = NULL;
    aLockNode->mCvsLockNode       = NULL;

    aLockNode->mPrvTransLockNode  = NULL;
    aLockNode->mNxtTransLockNode  = NULL;

    aLockNode->mDoRemove          = ID_FALSE;

    sLockSlotList = aLockNode->mArrLockSlotList;

    for ( i = 0 ; i < SML_NUMLOCKTYPES ; i++ )
    {
        sLockSlotList[i].mLockNode      = aLockNode;
        sLockSlotList[i].mMask          = mLockModeToMask[i];
        sLockSlotList[i].mPrvLockSlot   = NULL;
        sLockSlotList[i].mNxtLockSlot   = NULL;
        sLockSlotList[i].mLockSequence  = 0;
        sLockSlotList[i].mOldMode       = SML_NLOCK;
        sLockSlotList[i].mNewMode       = SML_NLOCK;
    }
}

IDE_RC smlLockMgr::allocLockNodeNormal(SInt /*aSlot*/, smlLockNode** aNewNode)
{
    IDE_TEST( mLockPool.alloc((void**)aNewNode) != IDE_SUCCESS );
    (*aNewNode)->mIndex = -1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smlLockMgr::allocLockNodeList( SInt aSlot, smlLockNode** aNewNode )
{
    /* BUG-43408 */
    smlLockNode*        sLockNode;
    smlTransLockList*   sTransLockList;
    iduList*            sLockNodeCache;
    iduListNode*        sNode;

    sTransLockList = (mArrOfLockList+aSlot);
    sLockNodeCache = &(sTransLockList->mLockNodeCache);

    if ( IDU_LIST_IS_EMPTY( sLockNodeCache ) )
    {
        IDE_TEST( mLockPool.alloc((void**)aNewNode)
                  != IDE_SUCCESS );
        sLockNode = *aNewNode;
        IDU_LIST_INIT_OBJ( &(sLockNode->mNode4LockNodeCache), sLockNode );
    }
    else
    {
        sNode = IDU_LIST_GET_FIRST( sLockNodeCache );
        sLockNode = (smlLockNode*)sNode->mObj;
        IDU_LIST_REMOVE( sNode );
        *aNewNode = sLockNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smlLockMgr::allocLockNodeBitmap( SInt aSlot, smlLockNode** aNewNode )
{
    SInt            sIndex;
    smlLockNode*    sLockNode = NULL;

    IDE_DASSERT( smuProperty::getLockMgrCacheNode() == 1 );

    sIndex = acpBitFfs64(~(mNodeAllocMap[aSlot]));

    if ( sIndex != -1 )
    {
        mNodeAllocMap[aSlot]   |= ID_ULONG(1) << sIndex;
        sLockNode               = &(mNodeCache[aSlot][sIndex]);
        sLockNode->mIndex       = sIndex;

        *aNewNode               = sLockNode;
    }
    else
    {
        IDE_TEST( allocLockNodeNormal(aSlot, aNewNode) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*********************************************************
  function description: allocLockNodeAndInit
  lock node를 할당하고, 초기화를 한다.
***********************************************************/
IDE_RC  smlLockMgr::allocLockNodeAndInit( SInt           aSlot,
                                          smlLockMode    aLockMode,
                                          smlLockItem  * aLockItem,
                                          smlLockNode ** aNewLockNode,
                                          idBool         aIsExplicitLock )
{
    smlLockNode*    sLockNode = NULL;

    IDE_TEST( mAllocLockNodeFunc(aSlot, &sLockNode) != IDE_SUCCESS );
    IDE_DASSERT( sLockNode != NULL );
    initLockNode( sLockNode );

    // table oid added for perfv
    sLockNode->mLockItemType    = aLockItem->mLockItemType;
    sLockNode->mSpaceID         = aLockItem->mSpaceID;
    sLockNode->mItemID          = aLockItem->mItemID;
    sLockNode->mSlotID          = aSlot;
    sLockNode->mLockCnt         = 0;
    sLockNode->mLockItem        = aLockItem;
    sLockNode->mLockMode        = aLockMode;
    sLockNode->mFlag            = mLockModeToMask[aLockMode];
    // BUG-28752 implicit/explicit 구분합니다.
    sLockNode->mIsExplicitLock  = aIsExplicitLock;

    sLockNode->mTransID = smLayerCallback::getTransID( smLayerCallback::getTransBySID( aSlot ) );

    *aNewLockNode = sLockNode;

    IDE_DASSERT( isLockNodeExist( sLockNode ) == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smlLockMgr::freeLockNodeNormal( smlLockNode* aLockNode )
{
    return mLockPool.memfree(aLockNode);
}

IDE_RC smlLockMgr::freeLockNodeList( smlLockNode* aLockNode )
{
    SInt                sSlot;
    smlTransLockList*   sTransLockList;
    iduList*            sLockNodeCache;

    sSlot           = aLockNode->mSlotID;
    sTransLockList  = (mArrOfLockList+sSlot);
    sLockNodeCache  = &(sTransLockList->mLockNodeCache);
    IDU_LIST_ADD_AFTER( sLockNodeCache, &(aLockNode->mNode4LockNodeCache) );

    return IDE_SUCCESS;
}

IDE_RC smlLockMgr::freeLockNodeBitmap( smlLockNode* aLockNode )
{
    SInt            sSlot;
    SInt            sIndex;
    ULong           sDelta;

    if ( aLockNode->mIndex != -1 )
    {
        IDE_DASSERT( smuProperty::getLockMgrCacheNode() == 1 );

        sSlot   = aLockNode->mSlotID;
        sIndex  = aLockNode->mIndex;
        sDelta  = ID_ULONG(1) << sIndex;

        IDE_DASSERT( (mNodeAllocMap[sSlot] & sDelta) != 0 );
        mNodeAllocMap[sSlot] &= ~sDelta;
    }
    else
    {
        IDE_TEST( mLockPool.memfree(aLockNode) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*********************************************************
  function description: findLockNode
  트랜잭션이 이전 statement에 의하여,
  현재  table A에 대하여 lock을 잡고 있는
  lock node  를 찾는다.
***********************************************************/
smlLockNode * smlLockMgr::findLockNode( smlLockItem * aLockItem, SInt aSlot )
{
    smlLockNode *sCurLockNode;
    smlLockNode* sLockNodeHeader = &(mArrOfLockList[aSlot].mLockNodeHeader);
    sCurLockNode = mArrOfLockList[aSlot].mLockNodeHeader.mPrvTransLockNode;

    while ( sCurLockNode != sLockNodeHeader )
    {
        IDE_ASSERT( sCurLockNode->mDoRemove == ID_FALSE );
        IDE_ASSERT( sCurLockNode->mBeGrant  == ID_TRUE );
        if ( sCurLockNode->mLockItem == aLockItem )
        {
            return sCurLockNode;
        }

        sCurLockNode = sCurLockNode->mPrvTransLockNode;
    }

    return NULL;
}

/*********************************************************
  function description: addLockSlot
  transation lock slot list에 insert하며,
  lock node추가 대신, node안에 lock slot을 이용하려할때
  불린다.
***********************************************************/
void smlLockMgr::addLockSlot( smlLockSlot * aLockSlot, SInt aSlot )
{
    smlLockSlot * sTransLockSlotHdr = &(mArrOfLockList[aSlot].mLockSlotHeader);
    //Add Lock Slot To Tail of Lock Slot List
    IDE_DASSERT( aLockSlot->mNxtLockSlot == NULL );
    IDE_DASSERT( aLockSlot->mPrvLockSlot == NULL );
    IDE_DASSERT( aLockSlot->mLockSequence == 0 );

    aLockSlot->mNxtLockSlot = sTransLockSlotHdr;
    aLockSlot->mPrvLockSlot = sTransLockSlotHdr->mPrvLockSlot;

    /* BUG-15906: non-autocommit모드에서 Select완료후 IS_LOCK이 해제되면
     * 좋겠습니다. Statement시작시 Transaction의 마지막 Lock Slot의
     * Lock Sequence Number를 저장해 두고 Statement End시에 Transaction의
     * Lock Slot List를 역으로 가면서 저장해둔 Lock Sequence Number보다
     * 해당 Lock Slot의 Lock Sequence Number가 작거나 같을때까지 Lock을 해제한다.
     */
    IDE_ASSERT( sTransLockSlotHdr->mPrvLockSlot->mLockSequence !=
                ID_ULONG_MAX );
    IDE_ASSERT( aLockSlot->mLockSequence == 0 );

    aLockSlot->mLockSequence =
        sTransLockSlotHdr->mPrvLockSlot->mLockSequence + 1;

    sTransLockSlotHdr->mPrvLockSlot->mNxtLockSlot = aLockSlot;
    sTransLockSlotHdr->mPrvLockSlot = aLockSlot;
}

/*********************************************************
  function description: removeLockSlot
  transaction의 slotid에
  해당하는 lock slot list에서 lock slot을  제거한다.
***********************************************************/
void smlLockMgr::removeLockSlot( smlLockSlot * aLockSlot )
{
    aLockSlot->mNxtLockSlot->mPrvLockSlot = aLockSlot->mPrvLockSlot;
    aLockSlot->mPrvLockSlot->mNxtLockSlot = aLockSlot->mNxtLockSlot;

    aLockSlot->mPrvLockSlot  = NULL;
    aLockSlot->mNxtLockSlot  = NULL;
    aLockSlot->mLockSequence = 0;

    aLockSlot->mOldMode      = SML_NLOCK;
    aLockSlot->mNewMode      = SML_NLOCK;
}

/*********************************************************
  function description: decTblLockModeAndTryUpdate
  table lock 정보인 aLockItem에서
  Lock node의 lock mode에 해당하는 lock mode의 갯수를 줄이고,
  만약 0이 된다면, table의 대표락을 변경한다.
***********************************************************/
void smlLockMgr::decTblLockModeAndTryUpdate( smlLockItem     * aLockItem,
                                             smlLockMode       aLockMode )
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
void smlLockMgr::incTblLockModeAndUpdate( smlLockItem     *  aLockItem,
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
}

IDE_RC smlLockMgr::allocLockItem( void ** aLockItem )
{
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SML,
                                       ID_SIZEOF(smlLockItem),
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

IDE_RC smlLockMgr::initLockItem( scSpaceID       aSpaceID,
                                 ULong           aItemID,
                                 smiLockItemType aLockItemType,
                                 void          * aLockItem )
{
    smlLockItem       * sLockItem;
    SChar               sBuffer[128];

    sLockItem = (smlLockItem*) aLockItem;

    /* LockItem Type에 따라서 mSpaceID와 mItemID를 설정한다.
     *                   mSpaceID    mItemID
     * TableSpace Lock :  SpaceID     N/A
     * Table Lock      :  SpaceID     TableOID
     * DataFile Lock   :  SpaceID     FileID */
    sLockItem->mLockItemType    = aLockItemType;
    sLockItem->mSpaceID         = aSpaceID;
    sLockItem->mItemID          = aItemID;

    idlOS::sprintf( sBuffer,
                    "LOCKITEM_MUTEX_%"ID_UINT32_FMT"_%"ID_UINT64_FMT,
                    aSpaceID,
                    (ULong)aItemID );

    IDE_TEST_RAISE( sLockItem->mMutex.initialize( sBuffer,
                                                  IDU_MUTEX_KIND_NATIVE,
                                                  IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS,
                    mutex_init_error );

    clearLockItem( sLockItem );

    return IDE_SUCCESS;

    IDE_EXCEPTION(mutex_init_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smlLockMgr::clearLockItem( smlLockItem   * aLockItem )
{
    SInt   sLockType;

    aLockItem->mGrantLockMode   = SML_NLOCK;
    aLockItem->mFstLockGrant    = NULL;
    aLockItem->mFstLockRequest  = NULL;
    aLockItem->mLstLockGrant    = NULL;
    aLockItem->mLstLockRequest  = NULL;
    aLockItem->mGrantCnt        = 0;
    aLockItem->mRequestCnt      = 0;
    aLockItem->mFlag            = SML_FLAG_LIGHT_MODE;

    for ( sLockType = 0 ; sLockType < SML_NUMLOCKTYPES ; sLockType++ )
    {
        aLockItem->mArrLockCount[sLockType] = 0;
    }
}



/*********************************************************
  function description: isCycle
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

 *********************************************************

  <PROJ-2734 : 'aIsReadyDistDeadlock = ID_TRUE' 인경우 >
  분산 트랜젝션 데드락 체크할 트랜젝션을 여기서 선정한다.

  WAITER TX와 대기관계 중 처음 나오는 <분산정보있는TX>가 대상이 되며,
  mTxList4DistDeadlock[WaitSlot][HoldLlot] 에 세팅된다.
  대상 선정기준은 아래와 같다.

  1. WAITER TX를 기준으로 한다. (WAITER TX도 <분산정보있는TX>이다.)
  2. WAITER TX가 대기하고 있는 <분산정보있는TX>가 대상이 된다.
  3. WAITER TX가 대기하고 있는 것이 <분산정보없는TX>라면,
     <분산정보없는TX>가 대기중인 또 다른 TX(X)를 확인한다.
     3.1 TX(X)가 <분산정보있는TX>라면 대상이 된다.
     3.2 TX(X)가 <분산정보없는TX>라면, <분산정보있는TX>가 나올때까지 계속 대기관계를 따라간다.

  예제) TX가 A, B, ~ H 까지 있다.
        WAITER TX A가 아래와 같은 대기관계를 갖는다면.
        각각의 TX B ~ H 까지의 값은 다음과 같이 정해진다.
    
  No는 <분산정보없는TX>, Yes는 <분산정보있는TX>를 나타낸다.

  A(WAITER) -> B(No)  -> C(Yes) -> D(No)
  A(WAITER) -> E(Yes) -> F(Yes)
  A(WAITER) -> G(Yes) -> H(No)

  위와 같을경우 TX 각각의 TYPE은 아래와 같이 정해지고,
  SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO 타입의 TX가 분산 트랜젝션 데드락 체크 대상으로 선정된다.

  SML_DIST_DEADLOCK_TX_NON_DIST_INFO   : B
  SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO : C, E, G
  SML_DIST_DEADLOCK_TX_UNKNOWN         : D, F, H

***********************************************************/
idBool smlLockMgr::isCycle( SInt aSlot, idBool aIsReadyDistDeadlock )
{
    UShort     i, j;
    // node와 node간에 대기경로 길이.
    UShort sPathLen;
    // node간에 대기 경로가 있는가 에대한 flag
    idBool     sExitPathFlag;
    smxTrans * sTrans = NULL;

    sExitPathFlag = ID_TRUE;
    sPathLen= 1;
    IDL_MEM_BARRIER;

    if ( aIsReadyDistDeadlock == ID_TRUE )
    {
        /* PROJ-2734 : 자신의 mTxList4DistDeadlock을 초기화 */
        smlLockMgr::clearTxList4DistDeadlock( aSlot );
    }

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

                /* PROJ-2734 */
                /* path Len = 1인 TX에 대한 mTxList4DistDeadlock 설정은 여기서한다. */
                if ( ( sPathLen == 1 ) &&
                     ( aIsReadyDistDeadlock == ID_TRUE ) )
                {
                    sTrans = (smxTrans *)(smLayerCallback::getTransBySID( i ));

                    if ( SMI_DIST_LEVEL_IS_VALID( sTrans->mDistTxInfo.mDistLevel ) )
                    {
                        /*
                           PROJ-2734
                           Trans가 재활용되었는지 확인하기 위해 TransID를 같이 저장한다.

                           적은 확률이지만, TransID설정 직전에 TX가 재활용될수있으므로,
                           mWaitForTable[aSlot][i].mIndex == 1 이 그대로 유지되는지 확인해서
                           TX 재활용여부를 재확인한다.
                         */

                        ID_SERIAL_BEGIN( mTxList4DistDeadlock[aSlot][i].mDistTxType = SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO;
                                         mTxList4DistDeadlock[aSlot][i].mTransID    = sTrans->mTransID; );

                        ID_SERIAL_END( if ( mWaitForTable[aSlot][i].mIndex != 1 )
                                       {
                                           /* 대기관계가 사라졌다면, TX가 재활용된것이다. 저장한 정보를 없앤다. */
                                           mTxList4DistDeadlock[aSlot][i].mDistTxType = SML_DIST_DEADLOCK_TX_UNKNOWN;
                                           mTxList4DistDeadlock[aSlot][i].mTransID    = SM_NULL_TID;
                                       } );
                    }
                    else
                    {
                        mTxList4DistDeadlock[aSlot][i].mDistTxType = SML_DIST_DEADLOCK_TX_NON_DIST_INFO;
                        mTxList4DistDeadlock[aSlot][i].mTransID    = SM_NULL_TID;
                    }
                }

                for ( j  = mArrOfLockList[i].mFstWaitTblTransItem;
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

                        /* PROJ-2734 */
                        if ( aIsReadyDistDeadlock == ID_TRUE )
                        {
                            switch( mTxList4DistDeadlock[aSlot][i].mDistTxType )
                            {
                                case SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO :
                                case SML_DIST_DEADLOCK_TX_UNKNOWN :

                                    mTxList4DistDeadlock[aSlot][j].mDistTxType = SML_DIST_DEADLOCK_TX_UNKNOWN;
                                    mTxList4DistDeadlock[aSlot][j].mTransID    = SM_NULL_TID;
                                    break;

                                case SML_DIST_DEADLOCK_TX_NON_DIST_INFO :

                                    sTrans = (smxTrans *)(smLayerCallback::getTransBySID( j ));

                                    if ( SMI_DIST_LEVEL_IS_VALID( sTrans->mDistTxInfo.mDistLevel ) )
                                    {
                                        /*
                                           PROJ-2734
                                           Trans가 재활용되었는지 확인하기 위해 TransID를 같이 저장한다.

                                           작은 확률이지만, TransID설정 직전에 TX가 재활용될수있으므로,
                                           mWaitForTable[i][j].mIndex == 1 이 그대로 유지되는지 확인해서
                                           TX 재활용여부를 재확인한다.
                                         */

                                        ID_SERIAL_BEGIN( mTxList4DistDeadlock[aSlot][j].mDistTxType
                                                             = SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO;
                                                         mTxList4DistDeadlock[aSlot][j].mTransID
                                                             = sTrans->mTransID ; );

                                        ID_SERIAL_END( if ( mWaitForTable[i][j].mIndex != 1 )
                                                       {
                                                           /* TX가 재활용된것이다. 저장한 정보를 없앤다. */
                                                           mTxList4DistDeadlock[aSlot][j].mDistTxType
                                                               = SML_DIST_DEADLOCK_TX_UNKNOWN;
                                                           mTxList4DistDeadlock[aSlot][j].mTransID
                                                               = SM_NULL_TID;
                                                       } );
                                    }
                                    break;

                                default :
                                    IDE_DASSERT(0);
                            }
                        }

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

ULong smlLockMgr::getDistDeadlockWaitTime( smlDistDeadlockRiskType aRisk )
{
    ULong sWaitTime;

    switch ( aRisk )
    {
        case SML_DIST_DEADLOCK_RISK_LOW :
            sWaitTime = smuProperty::getDistributionDeadlockRiskLowWaitTime();
            break;

        case SML_DIST_DEADLOCK_RISK_MID :
            sWaitTime = smuProperty::getDistributionDeadlockRiskMidWaitTime();
            break;

        case SML_DIST_DEADLOCK_RISK_HIGH :
            sWaitTime = smuProperty::getDistributionDeadlockRiskHighWaitTime();
            break;

        default :
            sWaitTime = 0;
            IDE_DASSERT(0);
    }

    return sWaitTime;
}

/* PROJ-2734
   분산데드락 체크대상 : WAITER와 대기관계 중 처음 나오는 분산정보있는TX
   분산레벨 확인 대상  : WAITER와 대기관계에 있는 모든 분산정보있는TX */
smxDistDeadlockDetection smlLockMgr::detectDistDeadlock( SInt    aWaitSlot,
                                                         ULong * aWaitTime )
{
    smxTrans * sHoldTrans          = NULL;
    smxTrans * sWaitTrans          = NULL;
    SInt i                         = 0;
    SInt sSlotCount                = 0;
    smiDistLevel sMaxDistLevel     = SMI_DIST_LEVEL_INIT;
    smiDistLevel sLastMaxDistLevel = SMI_DIST_LEVEL_INIT;
    ULong sWaitTime                = 0;
    smxDistDeadlockDetection sDetected       = SMX_DIST_DEADLOCK_DETECTION_NONE;
    smxDistDeadlockDetection sFirstDetection = SMX_DIST_DEADLOCK_DETECTION_NONE;

    sWaitTrans = (smxTrans *) smLayerCallback::getTransBySID( aWaitSlot );
    sSlotCount = (SInt)smlLockMgr::getSlotCount();

    /* isCycle() 함수에서
       mTxList4DistDeadlock 변수에 분산데드락 체크할 TX를 설정해두었다.
       (mTxList4DistDeadlock[WaitSlot][HoldSlot].mDistTxType = SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO 인 TX이다.) */

    for ( i = 0;
          i < sSlotCount;
          i++ )
    {
        /* 대기TX 자기자신이면 패스 */
        if ( i == aWaitSlot )
        {
            continue;
        }

        /* 대기TX와 대기관계가 없다면 패스 */
        if ( mWaitForTable[aWaitSlot][i].mIndex == 0 )
        {
            continue;
        }

        sHoldTrans = (smxTrans *)smLayerCallback::getTransBySID( i );

        /* 보유TX의 분산레벨이 VALID 하지 않으면 패스 */
        if ( SMI_DIST_LEVEL_IS_NOT_VALID( sHoldTrans->mDistTxInfo.mDistLevel ) )
        {
            continue;
        }

        /* 대기TX를 시작으로 대기관계를 그리는 모든 분산TX에 대해서
           분산레벨을 확인한다. */
        sLastMaxDistLevel = sMaxDistLevel;
        if ( sHoldTrans->mDistTxInfo.mDistLevel > sMaxDistLevel )
        {
            sMaxDistLevel = sHoldTrans->mDistTxInfo.mDistLevel;
        }

        /*
           분산데드락 체크 대상이 아니면 패스한다.
           => 여기서 주의할 점은,
              분산레벨을 확인할 TX와 분산데드락 체크할 TX는 다를수있다는 점이다.

           1) 분산레벨 확인할 TX : 대기TX와 대기관계에 있는 모든 보유TX
           2) 분산데드락 체크할 TX : 대기TX와 대기관계 중 처음 나오는 보유TX
         */
        if ( mTxList4DistDeadlock[aWaitSlot][i].mDistTxType != SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO )
        {
            continue;
        }

        sDetected = compareTx4DistDeadlock( sWaitTrans, sHoldTrans );

        if ( ( sHoldTrans->mStatus == SMX_TX_END ) ||
             ( mTxList4DistDeadlock[aWaitSlot][i].mTransID != sHoldTrans->mTransID ) )
        {
            /* 비교중에 HOLDER TX가 재활용 되었다.
               SKIP 하자. */
#ifdef DEBUG
            ideLog::log( IDE_SD_19,
                         "Distribution Deadlock - REUSE Trans ID : "
                         "%"ID_UINT32_FMT" -> %"ID_UINT32_FMT", Status : %"ID_INT32_FMT,
                         mTxList4DistDeadlock[aWaitSlot][i].mTransID,
                         sHoldTrans->mTransID,
                         sHoldTrans->mStatus );
#endif

            /* BUG-48445 : MAX분산레벨이 변경된 경우만 원복합니다. */
            if ( sLastMaxDistLevel != sMaxDistLevel )
            {
                sMaxDistLevel = sLastMaxDistLevel;
            }

            continue;
        }

        /* 분산데드락이 탐지되었다면, */
        if ( sDetected != SMX_DIST_DEADLOCK_DETECTION_NONE )
        {
            if ( sFirstDetection == SMX_DIST_DEADLOCK_DETECTION_NONE )
            {
                sFirstDetection = sDetected;
            }

            /* 분산LEVEL PARALLEL 라면, 더이상 TX를 비교할 필요없다. 비교를 종료한다. */
            if ( sMaxDistLevel == SMI_DIST_LEVEL_PARALLEL )
            {
                break;
            }
        }
    }

    if ( sFirstDetection != SMX_DIST_DEADLOCK_DETECTION_NONE )
    {
        /* 죽이기전 TIMEOUT 설정 */
        sWaitTime = getDistDeadlockWaitTime( 
                    mDistDeadlockRisk[ sWaitTrans->mDistTxInfo.mDistLevel ][ sMaxDistLevel ] );

    }

#ifdef DEBUG
    ideLog::log( IDE_SD_19,
                 "WaiterTx(%"ID_UINT32_FMT") DistLevel : %"ID_INT32_FMT", HolderTx Max DistLevel : %"ID_INT32_FMT,
                 sWaitTrans->mTransID,
                 sWaitTrans->mDistTxInfo.mDistLevel,
                 sMaxDistLevel );
#endif

    *aWaitTime = sWaitTime;

    return sFirstDetection;
}

/* PROJ-2734 : Distributed TX를 비교한다.
   비교순서는 아래와 같다.
   1. Distribution TX First Stmt View SCN
   2. Distribution TX First Stmt Time
   3. Shard PIN sequence
   4. Shard PIN Node ID */
smxDistDeadlockDetection smlLockMgr::compareTx4DistDeadlock( smxTrans * aWaitTx, smxTrans * aHoldTx )
{
    smxDistDeadlockDetection sDetected = SMX_DIST_DEADLOCK_DETECTION_NONE;

    UShort sWaitTxNodeID;
    UShort sHoldTxNodeID;

    UInt sWaitTxSequence;
    UInt sHoldTxSequence;

    while ( 1 ) /* no loop */
    {
        /* 1. Distribution TX First Stmt View SCN COMPARE */
#ifdef DEBUG
        ideLog::log( IDE_SD_19,
                     "compare ViewSCN (Must be Wait(%"ID_INT32_FMT") > Hold(%"ID_INT32_FMT")) : "
                     "%"ID_UINT64_FMT", %"ID_UINT64_FMT,
                     aWaitTx->mTransID,
                     aHoldTx->mTransID,
                     aWaitTx->mDistTxInfo.mFirstStmtViewSCN,
                     aHoldTx->mDistTxInfo.mFirstStmtViewSCN );
#endif

        if ( aWaitTx->mDistTxInfo.mFirstStmtViewSCN > aHoldTx->mDistTxInfo.mFirstStmtViewSCN )
        {
            break;
        }
        else if ( aWaitTx->mDistTxInfo.mFirstStmtViewSCN < aHoldTx->mDistTxInfo.mFirstStmtViewSCN )
        {
            sDetected = SMX_DIST_DEADLOCK_DETECTION_VIEWSCN;
            break;
        }

        /* 2. Distribution TX First Stmt Time COMPARE */
#ifdef DEBUG
        ideLog::log( IDE_SD_19,
                     "compare Time (Must be Wait(%"ID_INT32_FMT") > Hold(%"ID_INT32_FMT")) : "
                     "[%"ID_INT64_FMT"][%"ID_INT64_FMT"], [%"ID_INT64_FMT"][%"ID_INT64_FMT"]",
                     aWaitTx->mTransID,
                     aHoldTx->mTransID,
                     aWaitTx->mDistTxInfo.mFirstStmtTime.tv_.tv_sec,
                     aWaitTx->mDistTxInfo.mFirstStmtTime.tv_.tv_usec,
                     aHoldTx->mDistTxInfo.mFirstStmtTime.tv_.tv_sec,
                     aHoldTx->mDistTxInfo.mFirstStmtTime.tv_.tv_usec );
#endif

        if ( aWaitTx->mDistTxInfo.mFirstStmtTime > aHoldTx->mDistTxInfo.mFirstStmtTime )
        {
            break;
        }
        else if ( aWaitTx->mDistTxInfo.mFirstStmtTime < aHoldTx->mDistTxInfo.mFirstStmtTime )
        {
            sDetected = SMX_DIST_DEADLOCK_DETECTION_TIME;
            break;
        }

        SMI_DIVIDE_SHARD_PIN( aWaitTx->mDistTxInfo.mShardPin,
                              NULL, /* version */
                              &sWaitTxNodeID,
                              &sWaitTxSequence );

        SMI_DIVIDE_SHARD_PIN( aHoldTx->mDistTxInfo.mShardPin,
                              NULL, /* version */
                              &sHoldTxNodeID,
                              &sHoldTxSequence );

        /* 3. SHARD-PIN SEQUENCE COMPARE */
#ifdef DEBUG
        ideLog::log( IDE_SD_19,
                     "compare Sequence (Must be Wait(%"ID_INT32_FMT") > Hold(%"ID_INT32_FMT")) : "
                     "%"ID_UINT32_FMT", %"ID_UINT32_FMT,
                     aWaitTx->mTransID,
                     aHoldTx->mTransID,
                     sWaitTxSequence,
                     sHoldTxSequence );
#endif

        if ( sWaitTxSequence > sHoldTxSequence )
        {
            break;
        }
        else if ( sWaitTxSequence < sHoldTxSequence )
        {
            sDetected = SMX_DIST_DEADLOCK_DETECTION_SHARD_PIN_SEQ;
            break;
        }

        /* 4. SHARD-PIN NODE-ID COMPARE */
#ifdef DEBUG
        ideLog::log( IDE_SD_19,
                     "compare Node-ID (Must be Wait(%"ID_INT32_FMT") > Hold(%"ID_INT32_FMT")) : "
                     "%"ID_UINT32_FMT", %"ID_UINT32_FMT,
                     aWaitTx->mTransID,
                     aHoldTx->mTransID,
                     sWaitTxNodeID,
                     sHoldTxNodeID );
#endif

        if ( sWaitTxNodeID > sHoldTxNodeID )
        {
            break;
        }
        else if ( sWaitTxNodeID < sHoldTxNodeID )
        {
            sDetected = SMX_DIST_DEADLOCK_DETECTION_SHARD_PIN_NODE_ID;
            break;
        }

        sDetected = SMX_DIST_DEADLOCK_DETECTION_ALL_EQUAL;
        break;
    }

#ifdef DEBUG
    ideLog::log( IDE_SD_19,
                 "compare end (Must be 0) : result (%"ID_INT32_FMT")", sDetected );
#endif

    return sDetected;
}

/*********************************************************
  function description:

  aSlot에 해당하는 트랜잭션이 대기하고 있었던
  트랜잭션들의 대기경로 정보를 0으로 clear한다.
  -> waitTable에서 aSlot행에 대기하고 있는 컬럼들에서
    경로길이를 0로 한다.

  aDoInit에서 ID_TRUE이며 대기연결 정보도 끊어버린다.
***********************************************************/
void smlLockMgr::clearWaitItemColsOfTrans( idBool aDoInit, SInt aSlot )
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

/* PROJ-2734
 * isCycle() 함수가 수행되기 전으로 aSlot 트랜젝션의 대기상태 정보를 되돌린다.
 *
 * => isCycle() 함수 수행으로
 *    0 -> 2 or 3 or 4 or ... 로 증가된 mIndex값을 0으로 되돌린다. */
void smlLockMgr::revertWaitItemColsOfTrans( SInt aSlot )
{
    UInt    sCurTargetSlot;
    UInt    sNxtTargetSlot;

    /* isCycle을 거친 후의 리스트는
       mIndex값의 내림차순으로 구성되어있다. (큰값이 앞에 있다.) */

    for ( sCurTargetSlot = mArrOfLockList[aSlot].mFstWaitTblTransItem ;
          ( ( sCurTargetSlot != SML_END_ITEM ) &&
            ( mWaitForTable[aSlot][sCurTargetSlot].mIndex > 1 ) ) ;
          sCurTargetSlot = sNxtTargetSlot )
    {
        mWaitForTable[aSlot][sCurTargetSlot].mIndex = 0;

        sNxtTargetSlot = mWaitForTable[aSlot][sCurTargetSlot].mNxtWaitTransItem;

        mWaitForTable[aSlot][sCurTargetSlot].mNxtWaitTransItem = ID_USHORT_MAX;
    }

    if ( sCurTargetSlot != SML_END_ITEM )
    {
        mArrOfLockList[aSlot].mFstWaitTblTransItem = sCurTargetSlot;
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
void   smlLockMgr::registRecordLockWait( SInt aSlot, SInt aWaitSlot )
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

    mWaitForTable[aSlot][aWaitSlot].mNxtWaitTransItem = mArrOfLockList[aSlot].mFstWaitTblTransItem;
    mArrOfLockList[aSlot].mFstWaitTblTransItem = aWaitSlot;
}

/*********************************************************
  function description: freeAllRecordLock
  aSlot에 해당하는 트랜잭션을 record lock으로 waiting하고
  있는 트랜잭션들간의 대기 경로를 0으로 clear한다.
  record lock을 waiting하고 있는 트랜잭션을 resume시킨다.
***********************************************************/
IDE_RC  smlLockMgr::freeAllRecordLock( SInt aSlot )
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
void smlLockMgr::setLockModeAndAddLockSlot( SInt           aSlot,
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

/***********************************************************
 * BUG-47388 Lock Table 개선
 * Lock Node가 Transaction Lock Node List에 있는지 확인한다.
 ***********************************************************/
idBool smlLockMgr::isLockNodeExist( smlLockNode *aLockNode )
{
    UInt aSlot = aLockNode->mSlotID;
    smlLockNode *sCurLockNode;
    smlLockNode* sLockNodeHeader = &(mArrOfLockList[ aSlot ].mLockNodeHeader);

    IDE_ASSERT( aLockNode->mSlotID < mTransCnt );

    sCurLockNode = mArrOfLockList[aSlot].mLockNodeHeader.mPrvTransLockNode;

    while ( sCurLockNode != sLockNodeHeader )
    {
        if ( sCurLockNode == aLockNode )
        {
            return ID_TRUE;
        }

        sCurLockNode = sCurLockNode->mPrvTransLockNode;
    }

    return ID_FALSE;
}

/***********************************************************
 * BUG-47388 Lock Table 개선
 * Lock Node가 Lock Item의 Grant List에 있는지 확인한다.
 ***********************************************************/
idBool smlLockMgr::isLockNodeExistInGrant( smlLockItem * aLockItem,
                                           smlLockNode *aLockNode )
{
    smlLockNode   * sCurLockNode = aLockItem->mFstLockGrant;

    while ( sCurLockNode != NULL )
    {
        IDE_ASSERT( sCurLockNode->mLockItem == aLockItem );
        IDE_ASSERT( sCurLockNode->mItemID == aLockItem->mItemID );

        if ( sCurLockNode == aLockNode )
        {
            return ID_TRUE;
        }
        if ( aLockItem->mLstLockGrant == sCurLockNode )
        {
            break;
        }

        sCurLockNode = sCurLockNode->mNxtLockNode;
    }

    return ID_FALSE;
}

/***********************************************************
 * BUG-47388 Lock Table 개선
 * Lock Item의 Grant List와 Request List를 검증한다.
 ***********************************************************/
void smlLockMgr::validateNodeListInLockItem( smlLockItem * aLockItem )
{
    smlLockNode * sCurLockNode = aLockItem->mFstLockGrant;

    while ( sCurLockNode != NULL )
    {
        IDE_ASSERT( sCurLockNode->mLockItem == aLockItem );
        IDE_ASSERT( sCurLockNode->mItemID == aLockItem->mItemID );

        if ( aLockItem->mLstLockGrant == sCurLockNode )
        {
            break;
        }

        sCurLockNode = sCurLockNode->mNxtLockNode;
    }

    sCurLockNode = aLockItem->mFstLockRequest;
    while ( sCurLockNode != NULL )
    {
        IDE_ASSERT( sCurLockNode->mLockItem == aLockItem );
        IDE_ASSERT( sCurLockNode->mItemID == aLockItem->mItemID );

        if ( aLockItem->mLstLockRequest == sCurLockNode )
        {
            break;
        }

        sCurLockNode = sCurLockNode->mNxtLockNode;
    }

    return ;
}


void smlLockMgr::updateStatistics( idvSQL*      sStat,
                                   idvStatIndex aStatIdx )
{
    idvSession * sSession;

    sSession = ( sStat == NULL ? NULL : sStat->mSess );

    IDV_SESS_ADD( sSession, aStatIdx, 1 );
}

/***********************************************************
 * BUG-47388 Lock Table 개선으로 Light Mode 추가
 *
 * 0 Table, Tablespace Disable 이면 Lock Table을 잡지 않는다.
 *   DDL은 받을 수 없다. S, X, SIX 가 들어오면 예외처리
 *
 * 1. 이미 잡은 Lock Table의 하위모두 Lock 이 올 경우 통과
 *    단, IS Lock 은 FAC를 이유로 요청을 Skip하지 않는다.
 *
 * 2. BUG-47388에서 추가된 Light Mode로 Lock Table 을 잡는다.
 *    Lock Item에 S,X,SIX가 없고 내가 IS or IX를 잡는 경우
 *
 * 3. Lock Item에 S,X,SIX 가 있거나 내가 S,X,SIX 인 경우
 *    Mutex Mode로 Lock Table을 잡는다.
 ***********************************************************/
IDE_RC smlLockMgr::lockTable( SInt          aSlot,
                              smlLockItem  *aLockItem,
                              smlLockMode   aLockMode,
                              ULong         aLockWaitMicroSec,
                              smlLockMode   *aCurLockMode,
                              idBool       *aLocked,
                              smlLockNode **aLockNode,
                              smlLockSlot **aLockSlot,
                              idBool        aIsExplicit )
{
    smlLockNode       * sCurTransLockNode = NULL;
    UInt                sLockEnable = 1;
    idvSQL            * sStatistics = smLayerCallback::getStatisticsBySID( aSlot );

    /* BUG-32237 [sm_transaction] Free lock node when dropping table.
     * DropTablePending 에서 연기해둔 freeLockNode를 수행합니다. */
    /* 의도한 상황은 아니기에 Debug모드에서는 DASSERT로 종료시킴.
     * 하지만 release 모드에서는 그냥 rebuild 하면 문제 없음. */
    IDE_ERROR_RAISE( aLockItem != NULL, error_table_modified );

    if ( aLocked != NULL )
    {
        *aLocked = ID_TRUE;
    }

    if ( aLockSlot != NULL )
    {
        *aLockSlot = NULL;
    }

    // To fix BUG-14951
    // smuProperty::getTableLockEnable은 TABLE에만 적용되어야 한다.
    // (TBSLIST, TBS 및 DBF에는 해당 안됨)
    /* BUG-35453 -  add TABLESPACE_LOCK_ENABLE property
     * TABLESPACE_LOCK_ENABLE 도 TABLE_LOCK_ENABLE 과 동일하게
     * tablespace lock에 대해 처리한다. */
    if ( aLockItem->mLockItemType == SMI_LOCK_ITEM_TABLE )
    {
        sLockEnable = smuProperty::getTableLockEnable();
    }
    else if ( aLockItem->mLockItemType == SMI_LOCK_ITEM_TABLESPACE )
    {
        sLockEnable = smuProperty::getTablespaceLockEnable();
    }

    if ( sLockEnable == 0 )
    {
        IDE_TEST_RAISE( (( aLockMode == SML_SLOCK ) ||
                         ( aLockMode == SML_XLOCK ) ||
                         ( aLockMode == SML_SIXLOCK )),  error_lock_table_use );

        if ( aCurLockMode != NULL )
        {
            *aCurLockMode = aLockMode;
        }

        return IDE_SUCCESS;
    }

    // 트랜잭션이 이전 statement에 의하여,
    // 현재 table A에 대하여 lock을 잡았던 lock node를 찾는다.
    // Trans Node List는 나만 수정한다. 그러므로 lock을 잡을 필요는 없다.
    // 이 때 다른 DDL이 접근 할 수도 있으나 Trans Node List를 수정하지는 않는다..
    // Grant, Req List는 DDL이 수정 할 수도 있다.
    sCurTransLockNode = findLockNode( aLockItem, aSlot );
    // case 1: 이전에 트랜잭션이  table A에  lock을 잡았고,
    // 이전에 잡은 lock mode와 지금 잡고자 하는 락모드 변환결과가
    // 같으면 바로 return!
    if ( sCurTransLockNode != NULL )
    {
        if ( mConversionTBL[sCurTransLockNode->mLockMode][aLockMode]
             == sCurTransLockNode->mLockMode )
        {
            if (( aLockMode == SML_ISLOCK ) &&
                ( sCurTransLockNode->mArrLockSlotList[aLockMode].mLockSequence == 0 ))
            {
                /* PROJ-1381 Fetch Across Commits
                 * IS Lock을 잡지 않았던 경우, Lock Mode가 호환이 되더라도
                 * FAC Fetch Cursor를 위해 IS Lock을 잡아 줘야 한다. */       
            }
            else
            {
                /* 그렇지 않은 경우는(IS 가 아닌 경우이거나 이미 IS를 잡은 경우)
                 * 이미 잡은 Lock Mode가 새로 잡을 Lock의 상위 Lock이면 바로 Return */
                if ( aCurLockMode != NULL )
                {
                    *aCurLockMode = sCurTransLockNode->mLockMode;
                }
                IDE_CONT( lock_SUCCESS );
            }
        }
    }

    if (( aLockMode == SML_ISLOCK ) ||
        ( aLockMode == SML_IXLOCK ))
    {
        // 다른 DDL과 동시성 제어를 위해서 Lock을 잡아야한다.
        lockTransNodeList( sStatistics, aSlot );

        if ( aLockItem->mFlag == SML_FLAG_LIGHT_MODE )
        {
            if ( sCurTransLockNode != NULL ) /* 이전 statement 수행시 lock을 잡음 */
            {
                /* Lock에 대해서 Conversion을 수행한다. */
                sCurTransLockNode->mLockMode =
                    mConversionTBL[sCurTransLockNode->mLockMode][aLockMode];
            }
            else                      /* 이전 statement 수행 시 lock을 잡지 못함 */
            {
                /* allocate lock node and initialize */
                IDE_TEST( allocLockNodeAndInit( aSlot,
                                                aLockMode,
                                                aLockItem,
                                                &sCurTransLockNode,
                                                aIsExplicit )
                          != IDE_SUCCESS );
                sCurTransLockNode->mBeGrant  = ID_TRUE;

                /* Add Lock Node to a transaction */
                addLockNode( sCurTransLockNode, aSlot );
            }

            sCurTransLockNode->mLockCnt++;

            // 트랜잭션의 Lock node가 있었고,Lock을 잡았다면
            // lock slot을 추가한다
            // XXX 기존 코드에서 무조건 호출된다. 무조건 lockslot을 추가 해야 하는지 확인 해야 한다.
            setLockModeAndAddLockSlot( aSlot,
                                       sCurTransLockNode,
                                       aCurLockMode,
                                       aLockMode,
                                       ID_TRUE,
                                       aLockSlot );

            IDE_DASSERT( sCurTransLockNode->mArrLockSlotList[aLockMode].mLockSequence != 0 );
            // 내것에만 추가하거나 갱신하고 종료
            unlockTransNodeList( aSlot );

            IDE_ASSERT( sCurTransLockNode->mLockMode != SML_XLOCK );

            IDE_CONT(lock_SUCCESS);
        }
        else
        {
            unlockTransNodeList( aSlot );
        }
    }

    IDE_TEST( lockTableInternal( sStatistics,
                                 aSlot,
                                 aLockItem,
                                 aLockMode,
                                 aLockWaitMicroSec,
                                 aCurLockMode,
                                 aLocked ,
                                 sCurTransLockNode,
                                 aLockSlot,
                                 aIsExplicit ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT(lock_SUCCESS);

    if ( aLockNode != NULL )
    {
        *aLockNode = sCurTransLockNode;
    }
    else
    {
        /* nothing to do */
    }

    updateStatistics( sStatistics,
                      IDV_STAT_INDEX_LOCK_ACQUIRED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_table_modified );
    {
        IDE_SET( ideSetErrorCode( smERR_REBUILD_smiTableModified ) );
    }
    IDE_EXCEPTION( error_lock_table_use );
    {
        if ( aLockItem->mLockItemType == SMI_LOCK_ITEM_TABLE )
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_TableLockUse ));
        }
        else
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_TablespaceLockUse ));
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************
  function description: lockTable
  table에 lock을 걸때 다음과 같은 case에 따라 각각 처리한다.

  0. Lock Item Mutex를 잡고 보니 Light Mode 인 경우
     Light Mode의 Lock table을 잡고 바로 빠져 나간다.(BUG-47388)

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
     (BUG-47388 cvs node 를 대기하는 임시 Lock Node 이면 Free)

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
IDE_RC smlLockMgr::lockTableInternal( idvSQL           * aStatistics,
                                      SInt               aSlot,
                                      smlLockItem      * aLockItem,
                                      smlLockMode        aLockMode,
                                      ULong              aLockWaitMicroSec,
                                      smlLockMode      * aCurLockMode,
                                      idBool           * aLocked,
                                      smlLockNode      * aCurTransLockNode,
                                      smlLockSlot     ** aLockSlot,
                                      idBool             aIsExplicit )
{
    smlLockNode*        sNewTransLockNode = NULL;
    idBool              sLocked           = ID_TRUE;
    UInt                sState            = 0;
    SInt                i;
    smlTransLockList  * sCurSlot;
    smlLockNode       * sGrantLockNode;
    idBool              sIsFirstDDL = ID_FALSE;

    aLockItem->mMutex.lock( aStatistics );
    sState = 1;

    /***************************************************************************************/
    /* BUG-47388에서 추가된 영역 Start
     * 1. IS, IX lockItem을 잡아야 해서 잡았는데 그 사이에 모두 빠져나갔다, Light Mode로 추가
     * 2. X, S, SIX : Transaction을 모두 순회하며 Grant List를 구축 */
    if ( aLockItem->mFlag == SML_FLAG_LIGHT_MODE )
    {
        if (( aLockMode == SML_ISLOCK ) ||
            ( aLockMode == SML_IXLOCK ))
        {
            // lockItem을 잡아야 해서 잡았는데,그 사이에 모두 빠져나갔다,
            // 내것에만 추가하거나 갱신하고 빠진다.

            if ( aCurTransLockNode != NULL ) /* 이전 statement 수행시 lock을 잡음 */
            {
                /* Lock에 대해서 Conversion을 수행한다. */
                aCurTransLockNode->mLockMode =
                    mConversionTBL[aCurTransLockNode->mLockMode][aLockMode];
            }
            else                      /* 이전 statement 수행 시 lock을 잡지 못함 */
            {
                /* allocate lock node and initialize */
                IDE_TEST( allocLockNodeAndInit( aSlot,
                                                aLockMode,
                                                aLockItem,
                                                &aCurTransLockNode,
                                                aIsExplicit )
                          != IDE_SUCCESS );
                aCurTransLockNode->mBeGrant  = ID_TRUE;

                /* Add Lock Node to a transaction */
                lockTransNodeList( aStatistics, aSlot );
                addLockNode( aCurTransLockNode, aSlot );
                unlockTransNodeList( aSlot );
            }

            aCurTransLockNode->mLockCnt++;

            IDE_CONT(lock_SUCCESS);
        }
        else
        {
            // S,X,SIX 를 처음 잡은 경우, 모두 순회하며 이미 잡은 I lock을 찾아본다.
            /* 1. Transaction 탐색 하여 Grant List를 구축한다.
             * 2. 구버전 Lock Item과 동일하게 lock을 잡는다.
             * 여기에서는 Grant List만 구축 되면 나머지는 이후에서 한다.*/

            // mFlag를 사용하기 위해 초기화 한다.
            // SML_FLAG_LIGHT_MODE 에서 다른값으로 변경하면
            // 다른 Transaction 들이 Light Mode에서 벗어났다는 것을 알게 된다.
            aLockItem->mFlag = 0 ;
            sIsFirstDDL = ID_TRUE;

            // 내 mutex을 잡으면 dead lock 발생 할 수 있다.
            // 나의 mutex은 잡지 않고 타른 trans 의 mutex만 잡는다.
            for( i = 0 ; i < mTransCnt ; i++ )
            {
                sCurSlot = mArrOfLockList + i;

                // Node List만 보호하면 되지만 여기에서는 DML과의 동작을 통제하기 위하는 역할도 한다.
                // Pointer가 Null인것 같아 보이더라도, 해당 Transaction이 Lock을 잡고 아직 달지 않은 것일수도 있다.
                // 그러므로 Mutex lock을 잡고 확인 해 봐야 한다.
                lockTransNodeList( aStatistics, i );

                if ( (void*)sCurSlot->mLockNodeHeader.mPrvTransLockNode != (void*)sCurSlot )
                {
                    sGrantLockNode = findLockNode( aLockItem, i );
                    if( sGrantLockNode != NULL )
                    {
                        IDE_ASSERT( mCompatibleTBL[aLockItem->mGrantLockMode][sGrantLockNode->mLockMode] == ID_TRUE );
                        /* add node to grant list */
                        addLockNodeToTail( aLockItem->mFstLockGrant,
                                           aLockItem->mLstLockGrant,
                                           sGrantLockNode );

                        aLockItem->mGrantCnt++;

                        incTblLockModeAndUpdate( aLockItem, sGrantLockNode->mLockMode );
                        aLockItem->mGrantLockMode = mConversionTBL[aLockItem->mGrantLockMode][ sGrantLockNode->mLockMode ];
                    }
                }
                unlockTransNodeList( i );
            }
        }
    }
    /* BUG-47388에서 추가된 영역 End */
    /***************************************************************************************/

    //---------------------------------------
    // table의 대표락과 지금 잡고자 하는 락이 호환가능하는지 확인.
    //---------------------------------------
    if ( mCompatibleTBL[aLockItem->mGrantLockMode][aLockMode] == ID_TRUE )
    {
        if ( aCurTransLockNode != NULL ) /* 이전 statement 수행시 lock을 잡음 */
        {
            // 이전에 잡았던 값을 빼고 잠시 후에 새 값을 넣는다.
            decTblLockModeAndTryUpdate( aLockItem,
                                        aCurTransLockNode->mLockMode );
        }
        else                      /* 이전 statement 수행 시 lock을 잡지 못함 */
        {
            /* 기다리는 lock이 있는 경우 */
            /* BUGBUG: BUG-16471, BUG-17522
             * 기다리는 lock이 있는 경우 무조건 lock conflict 처리한다. */
            IDE_TEST_CONT( aLockItem->mRequestCnt != 0, lock_conflict );

            /* 기다리는 lock이 없는 경우 Lock을 grant 함 */
            /* BUG-47363 미리 alloc 해둔 LockNode를 사용 */
            IDE_TEST( allocLockNodeAndInit( aSlot,
                                            aLockMode,
                                            aLockItem,
                                            &aCurTransLockNode,
                                            aIsExplicit )
                      != IDE_SUCCESS );

            /* add node to grant list */
            addLockNodeToTail( aLockItem->mFstLockGrant,
                               aLockItem->mLstLockGrant,
                               aCurTransLockNode );

            aCurTransLockNode->mBeGrant = ID_TRUE;
            aLockItem->mGrantCnt++;

            // 수정은 나 혼자 하지만, 다른 Transaction이 다른 Table에 DDL을 잡으면서 나를 참조 할 수도 있다.
            // Transaction Lock Node List 변경 할 때 에는 반드시 Lock을 잡아야 한다.
            lockTransNodeList( aStatistics, aSlot );
            /* Add Lock Node to a transaction */
            addLockNode( aCurTransLockNode, aSlot );
            unlockTransNodeList( aSlot );
        }

        /* Lock에 대해서 Conversion을 수행한다. */
        aCurTransLockNode->mLockMode =
            mConversionTBL[aCurTransLockNode->mLockMode][aLockMode];

        incTblLockModeAndUpdate(aLockItem, aCurTransLockNode->mLockMode);
        aLockItem->mGrantLockMode =
            mConversionTBL[aLockItem->mGrantLockMode][aLockMode];

        aCurTransLockNode->mLockCnt++;
        IDE_CONT(lock_SUCCESS);
    }

    //---------------------------------------
    // Lock Conflict 처리
    //---------------------------------------

    IDE_EXCEPTION_CONT(lock_conflict);

    if ( ( aLockItem->mGrantCnt == 1 ) && ( aCurTransLockNode != NULL ) )
    {
        //---------------------------------------
        // lock conflict이지만, grant된 lock node가 1개이고,
        // 그것이 바로 그  트랙잭션일 경우에는 request list에 달지 않고
        // 기존 grant된  lock node의 lock mode와 table의 대표락,
        // grant lock mode를  갱신한다.
        //---------------------------------------

        decTblLockModeAndTryUpdate( aLockItem,
                                    aCurTransLockNode->mLockMode );

        aCurTransLockNode->mLockMode =
            mConversionTBL[aCurTransLockNode->mLockMode][aLockMode];

        aLockItem->mGrantLockMode =
            mConversionTBL[aLockItem->mGrantLockMode][aLockMode];

        incTblLockModeAndUpdate( aLockItem, aCurTransLockNode->mLockMode );
    }
    else if ( aLockWaitMicroSec != 0 )
    {
        IDE_TEST( allocLockNodeAndInit( aSlot,
                                        aLockMode,
                                        aLockItem,
                                        &sNewTransLockNode,
                                        aIsExplicit )
                  != IDE_SUCCESS );

        sNewTransLockNode->mBeGrant = ID_FALSE;

        if ( aCurTransLockNode != NULL )
        {
            sNewTransLockNode->mCvsLockNode = aCurTransLockNode;

            //Lock node를 Lock request 리스트의 헤더에 추가한다.
            // 왜냐하면 Conversion이기 때문이다.
            addLockNodeToHead( aLockItem->mFstLockRequest,
                               aLockItem->mLstLockRequest,
                               sNewTransLockNode );
        }
        else
        {
            aCurTransLockNode = sNewTransLockNode;

            // waiting table에서   트랜잭션의 slot id 행에
            // request list에서  대기하고 있는트랙잭션들 의
            // slot id를  table lock waiting 리스트에 등록한다.
            registTblLockWaitListByReq(aSlot,aLockItem->mFstLockRequest);
            //Lock node를 Lock request 리스트의 Tail에 추가한다.
            addLockNodeToTail( aLockItem->mFstLockRequest,
                               aLockItem->mLstLockRequest,
                               sNewTransLockNode );
        }
        // waiting table에서   트랜잭션의 slot id 행에
        // grant list에서     있는트랙잭션들 의
        // slot id를  table lock waiting 리스트에 등록한다.
        registTblLockWaitListByGrant( aSlot,aLockItem->mFstLockGrant );
        aLockItem->mRequestCnt++;

        IDE_TEST_RAISE( smLayerCallback::waitForLock(
                                        smLayerCallback::getTransBySID( aSlot ),
                                        &(aLockItem->mMutex),
                                        aLockWaitMicroSec )
                        != IDE_SUCCESS, err_wait_lock );

        if ( sNewTransLockNode->mCvsLockNode != NULL )
        {
            // 이미 잡고 있는 lock을 갱신하고, 대기하던 임시 lock node는 정리함,
            // 트랜잭션의 lock list에는 연결한 적이 없음 (BUG-47388)
            IDE_ASSERT(( sNewTransLockNode->mPrvTransLockNode == NULL ) &&
                       ( sNewTransLockNode->mNxtTransLockNode == NULL ));
            IDE_ASSERT(( sNewTransLockNode->mPrvLockNode == NULL ) &&
                       ( sNewTransLockNode->mNxtLockNode == NULL ) );
            IDE_ASSERT(  sNewTransLockNode->mDoRemove == ID_TRUE );
            IDE_ASSERT(  sNewTransLockNode->mBeGrant  == ID_FALSE );

            IDE_TEST( freeLockNode( sNewTransLockNode ) != IDE_SUCCESS );
            sNewTransLockNode = NULL;

            updateStatistics( aStatistics,
                              IDV_STAT_INDEX_LOCK_RELEASED );
        }
    }
    else
    {
        // 최초로 Heavy Mode로 변경 시도한 DDL이 Lock을 잡는데 실패한 경우
        // 바로 Light 모드로 다시 변경한다.
        if ( sIsFirstDDL == ID_TRUE )
        {
            IDE_TEST( toLightMode( aStatistics,
                                   aLockItem ) != IDE_SUCCESS );
        }

        sLocked = ID_FALSE;

        if ( aLocked != NULL )
        {
            *aLocked = ID_FALSE;
        }
    }

    IDE_EXCEPTION_CONT(lock_SUCCESS);

    //트랜잭션의 Lock node가 있었고,Lock을 잡았다면
    // lock slot을 추가한다
    setLockModeAndAddLockSlot( aSlot,
                               aCurTransLockNode,
                               aCurLockMode,
                               aLockMode,
                               sLocked,
                               aLockSlot );

    sState = 0;
    (void)aLockItem->mMutex.unlock();

    IDE_TEST_RAISE(sLocked == ID_FALSE, err_exceed_wait_time);
    IDE_DASSERT( aCurTransLockNode->mArrLockSlotList[aLockMode].mLockSequence != 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_wait_lock);
    {
        // fix BUG-10202하면서,
        // dead lock , time out을 튕겨 나온 트랜잭션들의
        // waiting table row의 열을 clear하고,
        // request list의 transaction이깨어나도 되는지 체크한다.

        // TimeOut 이지만, 타이밍 적으로 Lock을 Grant 한 상태 일 수도 있고,
        // 바로 정리 하면 되는 CVS Node 일 수도 있다.
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
    clearWaitItemColsOfTrans( ID_TRUE, aSlot );

    if ( sState != 0 )
    {
        (void)aLockItem->mMutex.unlock();
    }

    return IDE_FAILURE;
}

/*********************************************************
 * BUG-47388 Lock Table 개선으로 Light Mode 추가
 *
 * 1. light mode 일 경우 trans lock node list에서만 제거한다.
 * 2. 아닐 경우 unlockTableInternal() 호출
 *    2-1 mutex mode unlock Table
 *    2-2 Timeout 예외처리 ( aDoMutexLock == False )
 *       2-2-1 Timeout직전에 이미 Grant 해버린 경우
 *       2-2-2 CVS Node인 경우 DoRemove == ID_TRUE
 *       2-2-3 Request List에 연결되어 있는 경우
 *********************************************************/
IDE_RC smlLockMgr::unlockTable( SInt           aSlot,
                                smlLockNode  * aLockNode,
                                smlLockSlot  * aLockSlot,
                                idBool         aDoMutexLock )
{
    idvSQL       * sStatistics = smLayerCallback::getStatisticsBySID( aSlot );
    smlLockItem  * sLockItem ;
    UInt           sState = 0;

    if ( aLockNode == NULL )
    {
        IDE_DASSERT( aLockSlot->mLockNode != NULL );

        aLockNode = aLockSlot->mLockNode;
    }

    sLockItem = aLockNode->mLockItem;

    /* DoRemove도 내가 변경해서 lock을 if 안에 잡아도 되지만,
     * 1. Trans lock은 병목이 없다.
     * 2. 대부분의 경우 Light Mode 일 것이고 그렇다면 DoRemove는 False이다.
     * 밖에 잡는다고 해서 문제가 없으므로 만일의 사태를 대비해서 밖에서 잡는다.
     * */
    lockTransNodeList( sStatistics, aSlot );
    sState = 1;

    if ( aLockNode->mDoRemove == ID_FALSE )
    {
        // 아니 이것보다 내 lock item list prev, next를 보자
        if ( sLockItem->mFlag == SML_FLAG_LIGHT_MODE )
        {
            IDE_ASSERT( aLockNode->mNxtLockNode == NULL );
            IDE_ASSERT( aLockNode->mPrvLockNode == NULL );
            IDE_ASSERT( aLockNode->mLockMode != SML_XLOCK );
            IDE_ASSERT( aLockNode->mLockMode != SML_SLOCK );
            IDE_ASSERT( aLockNode->mLockMode != SML_SIXLOCK );
            IDE_ASSERT( aLockNode->mBeGrant == ID_TRUE );

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

                removeLockSlot(aLockSlot);

                if ( aLockNode->mFlag != 0 )
                {
                    aLockNode->mLockMode = getDecision( aLockNode->mFlag );

                    sState = 0;
                    unlockTransNodeList( aSlot );
                
                    updateStatistics( sStatistics,
                                      IDV_STAT_INDEX_LOCK_RELEASED );
                    return IDE_SUCCESS;
                }//if aLockNode
            } // if aLockSlot != NULL

            if ( aLockNode->mPrvTransLockNode != NULL )
            {
                // 트랜잭션 lock list array에서
                // transaction의 slot id에 해당하는
                // list에서 lock node를 제거한다.
                removeLockNode(aLockNode);
            }

            sState = 0;
            unlockTransNodeList( aSlot );

            IDE_TEST( freeLockNode( aLockNode ) != IDE_SUCCESS );
            aLockNode = NULL;

            updateStatistics( sStatistics,
                              IDV_STAT_INDEX_LOCK_RELEASED );

            // 내것만 제거하거나 갱신하고 종료
            return IDE_SUCCESS;
        }
    }
    else // ( aLockNode->mDoRemove == ID_TRUE )
    {
        /* 주로 lock escalation 용 임시 node이다
         * 호출 빈도가 낮다. unlockTableInternal에서 처리한다.*/
    }
    sState = 0;        
    unlockTransNodeList( aSlot );

    IDE_TEST( unlockTableInternal( sStatistics,
                                   aSlot,
                                   sLockItem,
                                   aLockNode,
                                   aLockSlot,
                                   aDoMutexLock ) != IDE_SUCCESS );

    updateStatistics( sStatistics,
                      IDV_STAT_INDEX_LOCK_RELEASED );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        unlockTransNodeList( aSlot );
    }

    return IDE_FAILURE;

}

/*********************************************************
  function description: unlockTable

 unlockTableInternal 이 호출되는 경우

  1 mutex mode unlock Table
  2 Timeout 예외처리
    2-1 Timeout직전에 이미 Grant 해버린 경우
    2-2 CVS Node인 경우 DoRemove == ID_TRUE
    2-3 Request List에 연결되어 있는 경우

 **************************************************    
 
  아래와 같이 다양한 case에 따라 각각 처리한다.
  0. lock node가 이미table의 grant or wait list에서
     이미 제거 되어 있는 경우.
     - lock node가 트랜잭션의 lock list에서 달려있으면,
       lock node를 트랜잭션의 lock list에서 제거한다.

  1. Lock Item Mutex를 잡고 보니 Light 모드 인 경우
     Light Mode Lock Table을 해제하고 바로 빠져나간다.

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

  4-1. Lock Item에 더이상 S,X,SIX 가 없는 경우
     Lock Node를 정리하고 Light Mode로 변경한다. BUG-47388

  4-2. lock을 wait하고 있는 Transaction을 깨운다.
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
IDE_RC smlLockMgr::unlockTableInternal( idvSQL      * aStatistics,
                                        SInt          aSlot,
                                        smlLockItem * aLockItem,
                                        smlLockNode * aLockNode,
                                        smlLockSlot * aLockSlot,
                                        idBool        aDoMutexLock )
{
    smlLockNode  * sCurLockNode;
    idBool         sDoFreeLockNode = ID_TRUE;
    idBool         sUnlinkAll;
    UInt           sState = 0;

    // lock node가 이미 request or grant list에서 제거된 상태.
    if ( aLockNode->mDoRemove == ID_TRUE )
    {
        // 주로 lock escalation 에 사용된 임시 lock node
        IDE_ASSERT( aDoMutexLock == ID_FALSE );
        IDE_ASSERT( aLockNode->mCvsLockNode != NULL );
        IDE_ASSERT( ( aLockNode->mPrvLockNode == NULL ) &&
                    ( aLockNode->mNxtLockNode == NULL ) );

        IDE_CONT( unlock_COMPLETE );
    }

    if ( aDoMutexLock == ID_TRUE )
    {
        // lockTable 중에 TimeOut으로 unlock하는 경우
        (void)aLockItem->mMutex.lock( aStatistics );
        sState = 1;
    }

    // LockItem을 잡고 들어왔는데 마침 풀어진 경우
    if ( aLockItem->mFlag == SML_FLAG_LIGHT_MODE )
    {
        IDE_ASSERT( aDoMutexLock == ID_TRUE );
        IDE_ASSERT( aLockNode->mNxtLockNode == NULL );
        IDE_ASSERT( aLockNode->mPrvLockNode == NULL );
        IDE_ASSERT( aLockNode->mLockMode != SML_XLOCK );
        IDE_ASSERT( aLockNode->mLockMode != SML_SLOCK );
        IDE_ASSERT( aLockNode->mLockMode != SML_SIXLOCK );
        IDE_ASSERT( aLockNode->mBeGrant == ID_TRUE );

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
            removeLockSlot( aLockSlot );

            if ( aLockNode->mFlag != 0 )
            {
                aLockNode->mLockMode = getDecision( aLockNode->mFlag );

                sDoFreeLockNode = ID_FALSE;
            }
        }

        IDE_CONT( unlock_COMPLETE );
    }

    // 남은 경우
    // Grant가 X, S, SIX 인 경우,
    // 혹은 X,S,SIX가 대기중이고 IS, IX 인 경우
    if ( aLockNode->mBeGrant == ID_TRUE )
    {
        decTblLockModeAndTryUpdate( aLockItem, aLockNode->mLockMode );

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
                    aLockNode->mLockMode = getDecision( aLockNode->mFlag );
                    incTblLockModeAndUpdate( aLockItem,
                                             aLockNode->mLockMode );
                    sDoFreeLockNode = ID_FALSE;
                    break;
                }//if aLockNode
            } // if aLockSlot != NULL

            //Remove lock node from lock Grant list
            removeLockNode( aLockItem->mFstLockGrant,
                            aLockItem->mLstLockGrant,
                            aLockNode );
            aLockItem->mGrantCnt--;
            break;
        }
        //새로운 Grant Lock Mode를 결정한다.
        aLockItem->mGrantLockMode = getDecision( aLockItem->mFlag );
    }//if aLockNode->mBeGrant == ID_TRUE
    else
    {
        // Grant 되어있지 않는 상태.
        //remove lock node from lock request list
        removeLockNode( aLockItem->mFstLockRequest,
                        aLockItem->mLstLockRequest, 
                        aLockNode );
        aLockItem->mRequestCnt--;

    }//else aLockNode->mBeGrant == ID_TRUE.

    if ( ( sDoFreeLockNode == ID_TRUE ) && ( aLockNode->mCvsLockNode == NULL ) )
    {
        // grant lock node이거나, request list에 있었던 lock node
        //가  완전히 Free될 경우 이 Transaction을 기다리고 있는
        //Transaction의  waiting table에서 갱신하여 준다.
        //하지만 request에 있었던 lock node의 경우,
        // Grant List에 Lock Node가
        //남아 있는경우는 하지 않는다.
        clearWaitTableRows( aLockItem->mFstLockRequest,
                            aSlot );
    }

    // ( 내가 X, S SIX 일때 ) && ( Grant에서 빠져나온다. )
    if (( aLockItem->mGrantLockMode == SML_ISLOCK ) ||
        ( aLockItem->mGrantLockMode == SML_IXLOCK ) ||
        ( aLockItem->mGrantLockMode == SML_NLOCK ))
    {
        sUnlinkAll = ID_TRUE;
        for ( sCurLockNode = aLockItem->mFstLockRequest ;
              sCurLockNode != NULL ;
              sCurLockNode = sCurLockNode->mNxtLockNode )
        {
            // 모두 IS, IX 만 있다면 Lock Item을 모두 해체한다
            if (( sCurLockNode->mLockMode == SML_XLOCK ) ||
                ( sCurLockNode->mLockMode == SML_SLOCK ) ||
                ( sCurLockNode->mLockMode == SML_SIXLOCK ))
            {
                sUnlinkAll = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        sUnlinkAll = ID_FALSE;
    }

    if ( sUnlinkAll == ID_TRUE )
    {
        /* 기다리는 Lock이 전부 DML 인 경우
         * lock item을 모두 unlink하고, i lock 체제로 변경한다.
         * 만약 내가 SIX였다면 Grant List도 unlink 해야 한다.*/

        IDE_TEST( toLightMode( aStatistics,
                               aLockItem ) != IDE_SUCCESS );
    }
    else // 기다리는 Lock중에 DDL 이 있는 경우
         // or 아직 X, S,SIX 가 해제되지 않은 경우
    {
        //lock을 기다리는 Transaction을 깨운다.
        IDE_TEST( wakeupRequestLockNodeInLockItem( aStatistics,
                                                   aLockItem ) != IDE_SUCCESS );
    }

    if ( aLockSlot != NULL )
    {
        removeLockSlot( aLockSlot );
    }

    IDE_EXCEPTION_CONT( unlock_COMPLETE );

    if ( sDoFreeLockNode == ID_TRUE )
    {
        // Trans List는 나만 갱신 할 수 있으므로
        // lock을 잡지 않고 확인해도 된다.
        if ( aLockNode->mPrvTransLockNode != NULL )
        {
            // 트랜잭션 lock list array에서
            // transaction의 slot id에 해당하는
            // list에서 lock node를 제거한다.
            lockTransNodeList( aStatistics, aSlot );
            removeLockNode(aLockNode);
            unlockTransNodeList( aSlot );
        }
    }

    if ( sState == 1 )
    {
        sState = 0;
        (void)aLockItem->mMutex.unlock();
    }

    if ( sDoFreeLockNode == ID_TRUE )
    {
        IDE_TEST( freeLockNode( aLockNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( aLockItem->mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/*********************************************************
 * BUG-47388 Lock Table 개선으로 Light Mode 추가
 *
 * S, X, SIX --> IS, IX, N 으로 변경되는 경우
 * Lock Item의 Grant, Request List의 모든 Lock Node를
 * Light Mode로 변경해주고 Lock Item을 초기화 한다.
 *********************************************************/
IDE_RC smlLockMgr::toLightMode( idvSQL      * aStatistics,
                                smlLockItem * aLockItem )
{
    smlLockNode  * sCurLockNode;

    if ( aLockItem->mGrantCnt > 0 )
    {
        sCurLockNode = aLockItem->mFstLockGrant;
        while ( sCurLockNode != NULL )
        {
            IDE_ASSERT(( sCurLockNode->mLockMode != SML_XLOCK ) &&
                       ( sCurLockNode->mLockMode != SML_SLOCK ) &&
                       ( sCurLockNode->mLockMode != SML_SIXLOCK ));
            removeLockNode( aLockItem->mFstLockGrant,
                            aLockItem->mLstLockGrant,
                            sCurLockNode );

            aLockItem->mGrantCnt--;
            clearWaitTableRows( aLockItem->mFstLockRequest,
                                sCurLockNode->mSlotID );

            sCurLockNode = aLockItem->mFstLockGrant;
        }
    }
    IDE_ASSERT( aLockItem->mGrantCnt == 0 );

    //lock을 기다리는 Transaction을 깨운다.
    sCurLockNode = aLockItem->mFstLockRequest;
    while ( sCurLockNode != NULL )
    {
        IDE_ASSERT(( sCurLockNode->mLockMode != SML_XLOCK ) &&
                   ( sCurLockNode->mLockMode != SML_SLOCK ) &&
                   ( sCurLockNode->mLockMode != SML_SIXLOCK ));

        //Request 리스트에서 Lock Node를 제거한다.
        removeLockNode( aLockItem->mFstLockRequest,
                        aLockItem->mLstLockRequest,
                        sCurLockNode );

        // Lock Item을 Light Mode로 변경하기 전에(mFlag를 변경하기 전에)
        // Transaction Lock Node List에 연결하고 Lock Mode를 설정 해 주어야 한다.
        // 다시 to Heavy 로 변경 하는 Transaction이 아직 연결되지 못한 Node를 찾지 못할 수도 있다.
        if ( sCurLockNode->mCvsLockNode != NULL )
        {
            //cvs node의 lock mode를 갱신한다.
            sCurLockNode->mCvsLockNode->mLockMode = mConversionTBL[sCurLockNode->mCvsLockNode->mLockMode][sCurLockNode->mLockMode];
            sCurLockNode->mDoRemove = ID_TRUE;
        }// if sCurLockNode->mCvsLockNode != NULL
        else
        {
            sCurLockNode->mBeGrant = ID_TRUE;
            IDE_ASSERT( sCurLockNode->mPrvTransLockNode == NULL );

            lockTransNodeList( aStatistics, sCurLockNode->mSlotID );
            addLockNode( sCurLockNode, sCurLockNode->mSlotID );
            unlockTransNodeList( sCurLockNode->mSlotID );
        }
        //waiting table에서 aSlot의 행에서  대기열을 clear한다.
        clearWaitItemColsOfTrans( ID_FALSE, sCurLockNode->mSlotID );

        // waiting하고 있는 트랜잭션을 resume시킨다.
        IDE_TEST( smLayerCallback::resumeTrans( smLayerCallback::getTransBySID( sCurLockNode->mSlotID ) ) != IDE_SUCCESS );
        sCurLockNode = aLockItem->mFstLockRequest;
    }/* while */
    // aLockItem 정리;

    // flag는 최소한 grant List는 해제 된 다음에 SML_FLAG_LIGHT_MODE를 set 해야 한다.
    // unlock하러 들어왔는데 flag는 0인데 아직 grant list에 연결 되어 있는 상황이 발생 할 수 있다.
    clearLockItem( aLockItem );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************
 * Mutex Mode Lock Table에서 Request List 상의 Lock Node 중
 * Grant List로 옮길 수 있는 Lock Node는 옮기고 wakeup한다.
 ***********************************************************/
IDE_RC smlLockMgr::wakeupRequestLockNodeInLockItem( idvSQL      * aStatistics,
                                                    smlLockItem * aLockItem )
{
    smlLockNode  * sCurLockNode;

    //lock을 기다리는 Transaction을 깨운다.
    sCurLockNode = aLockItem->mFstLockRequest;

    while ( sCurLockNode != NULL )
    {
        //Wake up requestors
        //현재 table의 grant mode와 현재 lock node의 lock mode가 호환?
        if ( mCompatibleTBL[sCurLockNode->mLockMode][aLockItem->mGrantLockMode] == ID_TRUE )
        {
            //Request 리스트에서 Lock Node를 제거한다.
            removeLockNode( aLockItem->mFstLockRequest,
                            aLockItem->mLstLockRequest,
                            sCurLockNode );

            aLockItem->mGrantLockMode =
                mConversionTBL[aLockItem->mGrantLockMode][sCurLockNode->mLockMode];
            if ( sCurLockNode->mCvsLockNode != NULL )
            {
                //table lock 정보에서 현재 lock node의
                //cvs node의  lock mode를 1감소시키고,대표락 갱신시도.
                decTblLockModeAndTryUpdate( aLockItem,
                                            sCurLockNode->mCvsLockNode->mLockMode );
                //cvs node의 lock mode를 갱신한다.
                sCurLockNode->mCvsLockNode->mLockMode =
                    mConversionTBL[sCurLockNode->mCvsLockNode->mLockMode][sCurLockNode->mLockMode];
                //table lock 정보에서 cvs node의
                //lock mode를 1증가 시키고 대표락을 갱신.
                incTblLockModeAndUpdate( aLockItem,
                                         sCurLockNode->mCvsLockNode->mLockMode );
                sCurLockNode->mDoRemove = ID_TRUE;
            }// if sCurLockNode->mCvsLockNode != NULL
            else
            {
                incTblLockModeAndUpdate( aLockItem, sCurLockNode->mLockMode );
                //Grant리스트에 새로운 Lock Node를 추가한다.
                addLockNodeToTail( aLockItem->mFstLockGrant,
                                   aLockItem->mLstLockGrant,
                                   sCurLockNode );
                sCurLockNode->mBeGrant = ID_TRUE;
                aLockItem->mGrantCnt++;

                IDE_ASSERT( sCurLockNode->mPrvTransLockNode == NULL );

                lockTransNodeList( aStatistics, sCurLockNode->mSlotID );
                addLockNode( sCurLockNode, sCurLockNode->mSlotID );
                unlockTransNodeList( sCurLockNode->mSlotID );
            }//else sCurLockNode->mCvsLockNode가 NULL
        }
        // 현재 table의 grant lock mode와 lock node의 lockmode
        // 가 호환하지 않는 경우.
        else
        {
            if ( aLockItem->mGrantCnt == 1 )
            {
                if ( sCurLockNode->mCvsLockNode != NULL )
                {
                    // cvs node가 바로 grant된 1개 node이다.
                    //현재 lock은 현재 table에 대한 lock의 Converion 이다.
                    decTblLockModeAndTryUpdate( aLockItem,
                                                sCurLockNode->mCvsLockNode->mLockMode );

                    aLockItem->mGrantLockMode =
                        mConversionTBL[aLockItem->mGrantLockMode][sCurLockNode->mLockMode];
                    sCurLockNode->mCvsLockNode->mLockMode = aLockItem->mGrantLockMode;
                    incTblLockModeAndUpdate( aLockItem, aLockItem->mGrantLockMode );
                    //Request 리스트에서 Lock Node를 제거한다.
                    removeLockNode( aLockItem->mFstLockRequest,
                                    aLockItem->mLstLockRequest, 
                                    sCurLockNode );
                    sCurLockNode->mDoRemove = ID_TRUE;
                }
                else
                {
                    break;
                } // sCurLockNode->mCvsLockNode가 null
            }//aLockItem->mGrantCnt == 1
            else
            {
                break;
            }//aLockItem->mGrantCnt != 1
        }//mCompatibleTBL

        aLockItem->mRequestCnt--;
        //waiting table에서 aSlot의 행에서  대기열을 clear한다.
        clearWaitItemColsOfTrans( ID_FALSE, sCurLockNode->mSlotID );
        // waiting하고 있는 트랜잭션을 resume시킨다.
        IDE_TEST( smLayerCallback::resumeTrans( smLayerCallback::getTransBySID( sCurLockNode->mSlotID ) )
                  != IDE_SUCCESS );
        sCurLockNode = aLockItem->mFstLockRequest;
    }/* while */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void smlLockMgr::dumpLockWait()
{
    SInt    i;
    SInt    j;
    SInt    k; /* for preventing from infinite loop */
    smTID   sTID;         // Current Waiting Tx
    smTID   sWaitForTID;  // Wait For Target Tx
    idBool  sWaited = ID_FALSE;

    SInt   sTransCnt = smLayerCallback::getCurTransCnt();

    ideLogEntry sLog( IDE_DUMP_0 );
    sLog.appendFormat( "LOCK WAIT INFO:\n"
                       "%8s,%16s \n",
                       "TRANS_ID",
                       "WAIT_FOR_TRANS_ID" );

    for ( j = 0; j < sTransCnt; j++ )
    {
        if ( smLayerCallback::isActiveBySID(j) == ID_TRUE)
        {
            for ( k = 0, i = smlLockMgr::mArrOfLockList[j].mFstWaitTblTransItem;
                  (i != SML_END_ITEM) && (i != ID_USHORT_MAX) && (k < sTransCnt);
                  i = smlLockMgr::mWaitForTable[j][i].mNxtWaitTransItem, k++ )
            {
                if (smlLockMgr::mWaitForTable[j][i].mIndex == 1)
                {
                    sWaited = ID_TRUE;
                    sTID        = smLayerCallback::getTIDBySID(j);
                    sWaitForTID = smLayerCallback::getTIDBySID(i);

                    sLog.appendFormat( "%8u,%16u \n",
                                       sTID,
                                       sWaitForTID );
                }
            }
        }
    }

    if( sWaited == ID_TRUE )
    {
        sLog.write();
    }
}

IDE_RC smlLockMgr::dumpLockTBL()
{
    smcTableHeader *sCatTblHdr;
    smcTableHeader *sTableHeader;
    smpSlotHeader  *sPtr;
    SChar          *sCurPtr;
    SChar          *sNxtPtr;

    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr    = NULL;

    // [1] 테이블 헤더를 순회하면서 현재 lock node list를 구한다.
    while(1)
    {
        IDE_TEST( smcRecord::nextOIDall( sCatTblHdr,
                                         sCurPtr,
                                         &sNxtPtr )
                  != IDE_SUCCESS );

        if ( sNxtPtr == NULL )
        {
            break;
        }

        sPtr = (smpSlotHeader *)sNxtPtr;

        // To fix BUG-14681
        if ( SM_SCN_IS_INFINITE(sPtr->mCreateSCN) )
        {
            /* BUG-14974: 무한 Loop발생.*/
            sCurPtr = sNxtPtr;
            continue;
        }

        sTableHeader = (smcTableHeader *)( sPtr + 1 );

        // 1. temp table은 skip ( PROJ-2201 TempTable은 이제 여기 없음 */
        // 2. drop된 table은 skip
        // 3. meta  table은 skip

        if( ( SMI_TABLE_TYPE_IS_META( sTableHeader ) == ID_TRUE ) ||
            ( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) )
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        getLockItemNodes( (smlLockItem*)sTableHeader->mLock );
        sCurPtr = sNxtPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smlLockMgr::getLockItemNodes( smlLockItem * aLockItem )
{
    smlLockNode * sCurLockNode;
    idBool        sGranted   = ID_FALSE;
    idBool        sRequested = ID_FALSE;
    SChar         sLockModeStr[][100] = {"SML_NLOCK",
                                         "SML_SLOCK",
                                         "SML_XLOCK",
                                         "SML_ISLOCK",
                                         "SML_IXLOCK",
                                         "SML_SIXLOCK"};

    aLockItem->mMutex.lock(NULL /* idvSQL* */);

    ideLogEntry sTitleLog( IDE_DUMP_0 );
    sTitleLog.appendFormat( "LOCK INFO:\n"
                            "SPACEID    : %"ID_UINT32_FMT"\n"
                            "TABLEOID   : %"ID_UINT64_FMT"\n"
                            "LOCK TYPE  : %"ID_UINT32_FMT"\n"
                            "LOCK MODE  : %s (%"ID_INT32_FMT")\n"
                            "GRANT CNT  : %"ID_INT32_FMT"\n"
                            "REQUEST CNT: %"ID_INT32_FMT"\n",
                            aLockItem->mSpaceID,
                            aLockItem->mItemID,
                            aLockItem->mLockItemType,
                            sLockModeStr[aLockItem->mGrantLockMode],
                            aLockItem->mGrantLockMode,
                            aLockItem->mGrantCnt,
                            aLockItem->mRequestCnt );


    ideLogEntry sGrantLog( IDE_DUMP_0 );
    sGrantLog.appendFormat( "LOCK GRANT INFO:\n"
                            "%8s, %8s, %8s, %8s, %8s, %8s, %8s \n",
                            "SLOTID",
                            "TABLEOID",
                            "TRANSID",
                            "TRANSID",
                            "LOCKMODE",
                            "LOCKCNT",
                            "IS_GRANT" );
    // Grant Lock Node list
    sCurLockNode = aLockItem->mFstLockGrant;

    while ( sCurLockNode != NULL )
    {
        sGranted = ID_TRUE;
        sGrantLog.appendFormat( "%8u, %8lu, %8u, %8u, %8s, %8u, %8u \n",
                                sCurLockNode->mSlotID,
                                sCurLockNode->mItemID,
                                smLayerCallback::getTransID(smLayerCallback::getTransBySID(sCurLockNode->mSlotID)),
                                sCurLockNode->mTransID,
                                sLockModeStr[sCurLockNode->mLockMode],
                                sCurLockNode->mLockCnt,
                                sCurLockNode->mBeGrant );

        sCurLockNode = sCurLockNode->mNxtLockNode;
    }


    ideLogEntry sRequestLog( IDE_DUMP_0 );
    sRequestLog.appendFormat( "LOCK REQUEST INFO:\n"
                              "%8s, %8s, %8s, %8s, %8s, %8s, %8s \n",
                              "SLOTID",
                              "TABLEOID",
                              "TRANSID",
                              "TRANSID",
                              "LOCKMODE",
                              "LOCKCNT",
                              "IS_GRANT" );
    // Request Lock Node list
    sCurLockNode = aLockItem->mFstLockRequest;

    while ( sCurLockNode != NULL )
    {
        sRequested = ID_TRUE;
        sRequestLog.appendFormat( "%8u, %8lu, %8u, %8u, %8s, %8u, %8u \n",
                                  sCurLockNode->mSlotID,
                                  sCurLockNode->mItemID,
                                  smLayerCallback::getTransID(smLayerCallback::getTransBySID(sCurLockNode->mSlotID)),
                                  sCurLockNode->mTransID,
                                  sLockModeStr[sCurLockNode->mLockMode],
                                  sCurLockNode->mLockCnt,
                                  sCurLockNode->mBeGrant );

        sCurLockNode = sCurLockNode->mNxtLockNode;
    }

    aLockItem->mMutex.unlock();

    if( ( sGranted == ID_TRUE ) || (sRequested == ID_TRUE ) )
    {
        sTitleLog.write();
    }

    if( sGranted == ID_TRUE )
    {
        sGrantLog.write();
    }

    if( sRequested == ID_TRUE )
    {
        sRequestLog.write();
    }
    return;
}
