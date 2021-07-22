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
 * $$Id:$
 **********************************************************************/

/************************************************************************
 * Description :
 *    버퍼풀에서 사용하는 checkpoint list set을 구현한 파일이다.
 *    checkpoint list는 BCB가 dirty되는 시점에 LRU list와 상관없이
 *    달리는 리스트이다.
 *    그런 checkpoint list는 하나의 set으로 다중 관리되는데
 *    다른 리스트들과는 달리 한 클래스에서
 *    다중 리스트가 관리되는게 특징이다.
 *    이는 checkpoint list 들 중에 가장 작은 recovery LSN을
 *    외부에서 구하기 쉽도록 하기 위해서이다.
 *
 *    + transaction thread들에 의해 setDirty시 add()가 호출된다.
 *    + flusher에 의해 flush시 minBCB(), nextBCB(), remove()가 호출된다.
 *    + checkpoint thread에 의해 checkpoint시 getRecoveryLSN()이 호출된다.
 *
 *    checkpoint flush시 다음과 같은 형식으로 사용해야 한다.
 *
 *    sdbCPListSet::minBCB(&sBCB);
 *    while (sBCB != NULL)
 *    {
 *        if (sBCB satisfies some flush condition)
 *        {
 *            flush(sBCB);
 *            sdbCPListSet::remove(&sBCB);
 *            sdbCPListSet::minBCB(&sBCB);
 *        }
 *        else
 *        {
 *            sdbCPListSet::nextBCB(&sBCB);
 *        }
 *    }
 *
 * Implementation :
 *    각 checkpoint list는 하나의 mutex에 의해 관리된다.
 *    이 mutex는 add()와 remove()에서 사용된다.
 ************************************************************************/

#include <sdbCPListSet.h>
#include <sdbReq.h>
#include <smErrorCode.h>


/************************************************************************
 * Description:
 *  초기화
 * aListCount   - [IN] 리스트 개수
 * aType        - [IN] BufferMgr or SecondaryBufferMgr 에서 호출되었는지.
 ************************************************************************/
IDE_RC sdbCPListSet::initialize( UInt aListCount, sdLayerState aType )
{
    UInt  i;
    SChar sMutexName[128];
    UInt  sState = 0;

    mListCount = aListCount;

    /* TC/FIT/Limit/sm/sdb/sdbCPListSet_initialize_malloc1.sql */
    IDU_FIT_POINT_RAISE( "sdbCPListSet::initialize::malloc1",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     (ULong)ID_SIZEOF(smuList) * mListCount,
                                     (void**)&mBase) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;

    /* TC/FIT/Limit/sm/sdb/sdbCPListSet_initialize_malloc2.sql */
    IDU_FIT_POINT_RAISE( "sdbCPListSet::initialize::malloc2",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                      (ULong)ID_SIZEOF(UInt) * mListCount,
                                      (void**)&mListLength)!= IDE_SUCCESS,
                    insufficient_memory );
    sState = 2;

    /* TC/FIT/Limit/sm/sdb/sdbCPListSet_initialize_malloc3.sql */
    IDU_FIT_POINT_RAISE( "sdbCPListSet::initialize::malloc3",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     (ULong)ID_SIZEOF(iduMutex) * mListCount,
                                     (void**)&mListMutex) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 3;

    mLayerType = aType; /* BUG-47945 cp list 디버깅 정보 보강 */
    if( aType == SD_LAYER_BUFFER_POOL )
    {
        for (i = 0; i < mListCount; i++)
        {
            idlOS::memset(sMutexName, 0, 128);
            idlOS::snprintf(
                    sMutexName, 
                    128, 
                    "CHECKPOINT_LIST_MUTEX_%"ID_UINT32_FMT, i + 1);

            IDE_ASSERT(mListMutex[i].initialize(
                        sMutexName,
                        IDU_MUTEX_KIND_NATIVE,
                        IDV_WAIT_INDEX_LATCH_FREE_DRDB_CHECKPOINT_LIST)
                    == IDE_SUCCESS);

            SMU_LIST_INIT_BASE(&mBase[i]);
            mListLength[i] = 0;
        }
    }
    else
    {
        for (i = 0; i < mListCount; i++)
        {
            idlOS::memset(sMutexName, 0, 128);
            idlOS::snprintf(
                 sMutexName, 
                 ID_SIZEOF(sMutexName), 
                 "SECONDARY_BUFFER_CHECKPOINT_LIST_MUTEX_%"ID_UINT32_FMT, i + 1);

            IDE_ASSERT(mListMutex[i].initialize(
                 sMutexName,
                 IDU_MUTEX_KIND_POSIX,
                 IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_CHECKPOINT_LIST)
                    == IDE_SUCCESS);

            SMU_LIST_INIT_BASE(&mBase[i]);
            mListLength[i] = 0;
        }
    }

    // 이 후에 IDE_TEST 구문을 사용하려면 mListMutex의 destroy처리를
    // exception 코드에서 해줘야 한다.

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 3:
            IDE_ASSERT(iduMemMgr::free(mListMutex) == IDE_SUCCESS);
            mListMutex = NULL;
        case 2:
            IDE_ASSERT(iduMemMgr::free(mListLength) == IDE_SUCCESS);
            mListLength = NULL;
        case 1:
            IDE_ASSERT(iduMemMgr::free(mBase) == IDE_SUCCESS);
            mBase = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  소멸자
 ***********************************************************************/
IDE_RC sdbCPListSet::destroy()
{
    UInt i;

    for( i = 0; i < mListCount; i++ )
    {
        IDE_ASSERT( mListMutex[i].destroy() == IDE_SUCCESS );
    }

    IDE_ASSERT(iduMemMgr::free(mListMutex) == IDE_SUCCESS);
    mListMutex = NULL;

    IDE_ASSERT(iduMemMgr::free(mListLength) == IDE_SUCCESS);
    mListLength = NULL;

    IDE_ASSERT(iduMemMgr::free(mBase) == IDE_SUCCESS);
    mBase = NULL;

    return IDE_SUCCESS;
}

/************************************************************************
 * Description :
 *    checkpoint list에 BCB를 recoveryLSN 순으로 정렬하여 추가한다.
 *    기본적으로 BCB의 mPageID에 의해 checkpoint list no가 정해지는데,
 *    해당 list의 mutex가 잠겨있으면 다음 list에 넣게 된다.
 *    만약 list 개수만큼 시도해도 mutex 잡는데 실패하면 처음에 선택했던
 *    list에서 mutex lock을 요청하고 대기하게 된다.
 *
 * Implementation :
 *    다음 3단계를 통해 add를 한다.
 *    1. 삽입할 list를 물색한다. list mutex 획득에 성공하면 해당 list에
 *       넣게된다.
 *    2. list를 찾았으면 삽입할 위치를 찾는다. tail부터 순회하여
 *       자신의 recoveryLSN보다 작은 BCB를 만날때까지 mPrev로 진행한다.
 *    3. 위치를 찾았으면 aBCB를 리스트에 끼어 넣는다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 ************************************************************************/
void sdbCPListSet::add( idvSQL * aStatistics, void * aBCB )
{
    UInt       sTryCount = 0;;
    UInt       sListNo  = ID_UINT_MAX;
    smuList  * sNode    = NULL;
    smuList  * sPrvNode = NULL;
    smuList  * sCurNode = NULL;
    smuList  * sNxtNode = NULL;
    sdBCB    * sBCB     = NULL;
    smLSN      sLSNOfAddBCB;
    idBool     isSucceed;

    IDE_DASSERT( aBCB != NULL );

    if ( mLayerType == SD_LAYER_BUFFER_POOL )
    {
        /* BUG-47945 cp list 디버깅 정보 보강 */
        IDE_ERROR( sdbBufferPool::isFixedBCB( (sdbBCB*)aBCB) == ID_TRUE );
    }
    SM_GET_LSN( sLSNOfAddBCB, SD_GET_BCB_RECOVERYLSN( aBCB ) );

    sListNo   = SD_GET_BCB_PAGEID( aBCB ) % mListCount;

    // [1 단계: list들에 대해 mutex가 잡히는 list를 찾는다.]
    // list들에 대해 lock을 시도한다.
    while (1)
    {
        if (sTryCount == mListCount)
        {
            // trylock을 mListCount만큼 시도했으면
            // lock()으로 대기를 한다.
            IDE_ASSERT(mListMutex[sListNo].lock(aStatistics)
                       == IDE_SUCCESS);
            break;
        }
        // 일반적으로 trylock을 호출하여
        // lock 획득에 실패하면 다음 list로 옮긴다.
        IDE_ASSERT(mListMutex[sListNo].trylock(isSucceed)
                   == IDE_SUCCESS);
        sTryCount++;
        
        if (isSucceed == ID_TRUE)
        {
            break;
        }
        // 다음 try할 list no를 구한다.
        // 한바퀴 돌면 다시 0부터 try한다.
        if (sListNo == mListCount - 1)
        {
            sListNo = 0;
        }
        else
        {
            sListNo++;
        }
    }

    // [2 단계: list의 tail부터 recoveryLSN을 검사하여 들어갈 위치를 찾는다.]
    // 꼬리부터 순회하여 aBCB의 recoveryLSN보다
    // 같거나 작은 첫번째 BCB를 찾는다.
    sNode = SMU_LIST_GET_LAST(&mBase[sListNo]);

    /* BUG-47945 cp list 디버깅 정보 보강,
     * CP List 연결에서 문제가 되는 상황이 발생하여
     * list 연결시 단계별로 확인합니다. */
    IDE_ERROR_MSG( sNode > (void*)512,
                   "sNode : %"ID_xPOINTER_FMT"\n",sNode );

    IDE_ERROR_MSG( sNode->mNext == &mBase[sListNo],
                   "sNode->mNext : %"ID_xPOINTER_FMT" != "
                   "mBase[%"ID_UINT32_FMT"] : %"ID_xPOINTER_FMT"\n",
                   sNode->mNext,
                   sListNo,
                   &mBase[sListNo] );

    while (sNode != &mBase[sListNo])
    {
        sBCB = (sdBCB*)sNode->mData;

        IDE_ERROR_MSG( sListNo == SD_GET_BCB_CPLISTNO( sBCB ),
                       "sListNo %"ID_UINT32_FMT" != "
                       "SD_GET_BCB_CPLISTNO( sBCB ) %"ID_UINT32_FMT"\n",
                       sListNo,
                       SD_GET_BCB_CPLISTNO( sBCB ) );

        if ( smLayerCallback::isLSNGTE( &sLSNOfAddBCB, 
                                        &(sBCB->mRecoveryLSN) )
             == ID_TRUE )
        {
            break;
        }

        sPrvNode = SMU_LIST_GET_PREV( sNode );
        IDE_ERROR_MSG( sPrvNode > (void*)512,
                       "sPrvNode : %"ID_xPOINTER_FMT"\n",sPrvNode );

        IDE_ERROR_MSG( sPrvNode->mNext == sNode,
                       "sPrvNode->mNext %"ID_xPOINTER_FMT" != "
                       "sNode %"ID_xPOINTER_FMT"\n",
                       sPrvNode->mNext, sNode );

        sNxtNode = sNode;
        sNode = sPrvNode;
    }

    IDE_ERROR_MSG( sNode > (void*)512,
                   "sNode : %"ID_xPOINTER_FMT"\n",sNode );

    sNxtNode = sNode->mNext;

    IDE_ERROR_MSG( sNxtNode > (void*)512,
                   "sNode : %"ID_xPOINTER_FMT"\n",sNxtNode );

    IDE_ERROR_MSG( sNxtNode->mPrev == sNode,
                   "sNxtNode->mPrev %"ID_xPOINTER_FMT" != "
                   "sNode %"ID_xPOINTER_FMT"\n",
                   sNxtNode->mPrev, sNode );

    // [3 단계: 찾은 위치에 BCB를 넣는다.]
    sCurNode = SD_GET_BCB_CPLISTITEM( aBCB );
    sCurNode->mPrev = sNode;
    sCurNode->mNext = sNode->mNext;
    sNxtNode->mPrev = sCurNode;
    sNode->mNext = sCurNode;

    SD_GET_BCB_CPLISTNO( aBCB ) = sListNo;
    mListLength[sListNo] += 1;

    IDE_ASSERT(mListMutex[sListNo].unlock() == IDE_SUCCESS);

    return;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_ERR_0,
                 "sdbCPListSet::add()\n"
                 "Layer   : %"ID_UINT32_FMT" (0: Buffer Pool, 1: Sedondary Buffer)\n"
                 "ListNo  : %"ID_UINT32_FMT"\n"
                 "ListCnt : %"ID_UINT32_FMT"\n"
                 "sNode   : %"ID_xPOINTER_FMT"\n"
                 "sPrvNode: %"ID_xPOINTER_FMT"\n"
                 "sNxtNode: %"ID_xPOINTER_FMT"\n"
                 "sCurNode: %"ID_xPOINTER_FMT"\n",
                 mLayerType,
                 sListNo,
                 mListCount,
                 sNode,
                 sPrvNode,
                 sNxtNode,
                 sCurNode );

    if ( sListNo < mListCount )
    {
        ideLog::log( IDE_ERR_0,
                     "mBase[%"ID_UINT32_FMT"] : %"ID_xPOINTER_FMT"\n",
                     sListNo,
                     &mBase[sListNo] );
    }

    IDE_ASSERT( mLayerType == SD_LAYER_BUFFER_POOL );

    sdbBCB::dump( aBCB );

    if ( sBCB != NULL )
    {
        sdbBCB::dump( sBCB );; 
    }

    if ( sNode > (void*)512 )
    {
        sBCB = (sdBCB*)sNode->mData;

        ideLog::log( IDE_ERR_0,
                     "sNode->mData: %"ID_xPOINTER_FMT"\n",
                     sBCB );

        if ( sBCB > (void*)512 )
        {
            sdbBCB::dump( sBCB );;
        }
    }

    if ( sNxtNode > (void*)512 )
    {
        sBCB = (sdBCB*)sNxtNode->mData;

        ideLog::log( IDE_ERR_0,
                     "sNxtNode->mData: %"ID_xPOINTER_FMT"\n",
                     sBCB );
        if ( sBCB > (void*)512 )
        {
            sdbBCB::dump( sBCB );;
        }
    }

    if ( sPrvNode > (void*)512 )
    {
        sBCB = (sdBCB*)sPrvNode->mData;

        ideLog::log( IDE_ERR_0,
                     "sPrvNode->mData: %"ID_xPOINTER_FMT"\n",
                     sBCB );
        if ( sBCB > (void*)512 )
        {
            sdbBCB::dump( sBCB );;
        }
    }

    if ( sCurNode > (void*)512 )
    {
        sBCB = (sdBCB*)sCurNode->mData;

        ideLog::log( IDE_ERR_0,
                     "sCurNode->mData: %"ID_xPOINTER_FMT"\n",
                     sBCB );
        if ( sBCB > (void*)512 )
        {
            sdbBCB::dump( sBCB );;
        }
    }

    IDE_ASSERT(0);

    return;
}

/************************************************************************
 * Description :
 *    checkpoint list들 중에 가장 작은 recovery LSN을 가진 BCB를 찾는다.
 *    찾은 BCB를 checkpoint list에서 빼진 않는다.
 *    또한 찾은 BCB는 checkpoint list에 대해 mutex를 잡지 않기 때문에
 *    반환할 시점에 checkpoint list에 달려있는지, 최소의 recovery LSN이
 *    맞는지는 보장할 수 없다.
 *    이 함수를 호출한 곳에서 결과로 얻은 BCB에 대해 stateMutex를 잡은 후
 *    state를 확인하여 dirty인지를 검사해야 한다.
 *    최소 recovery LSN이 아니어도 checkpoint flush가 문제가 되진 않는다.
 *
 * Implementation :
 *    checkpoint list를 순회하면서 가장 앞에 있는 BCB끼리 recovery LSN을
 *    비교하여 가장 작은 LSN을 가진 BCB를 반환한다.
 *    각 리스트의 BCB를 참조할 땐 mutex를 잡지 않는다.
 *    서로 다른 쓰레드에 의해 동시에 이 함수가 불린다면
 *    같은 BCB를 반환받게 될 것이다.
 *    하지만 flush할 때 BCB의 BCBMutex를 획득함으로써,
 *    같은 BCB를 두 flusher에 의해 동시에 flush되는 일은 없을 것이다.
 ***********************************************************************/
sdBCB* sdbCPListSet::getMin()
{
    UInt      i;
    smLSN     sMinLSN;
    smuList * sNode;
    sdBCB   * sFirstBCB;
    sdBCB   * sRet;

    // checkpoint list가 모두 비어있으면 NULL을 리턴하게 된다.
    sRet = NULL;

    // sMinLSN의 값을 최대값으로 초기화한다.
    SM_LSN_MAX(sMinLSN);

    for (i = 0; i < mListCount; i++)
    {
        sNode = SMU_LIST_GET_FIRST(&mBase[i]);
        if (sNode == &mBase[i])
        {
            // checkpoint list가 비어 있거나 skip list이면
            // skip한다.
            continue;
        }

        sFirstBCB = (sdBCB*)sNode->mData;
        if ( smLayerCallback::isLSNLT( &(sFirstBCB->mRecoveryLSN), &sMinLSN ) )
        {
            SM_GET_LSN(sMinLSN, sFirstBCB->mRecoveryLSN);
            sRet = sFirstBCB;
        }
    }
    return sRet;
}

/***********************************************************************
 * Description :
 *    입력받은 BCB가 속한 list 다음의 list의 첫번째 BCB를 반환한다.
 *    다음 list에 BCB가 없으면 그 다음 list의 BCB를 반환한다.
 *    모든 list가 비어있으면 NULL을 반환한다.
 *    aCurrBCB는 minBCB()를 통해서 얻은 BCB이기 때문에
 *    이 함수가 불려질 시점에 aCurrBCB가 list에서 빠졌을 수도 있다.
 *    이 경우엔 list의 minBCB를 반환한다.
 *
 * Implementation :
 *    주어진 mListNo + 1부터 mListNo - 1까지 순환하면서
 *    첫번째 BCB가 NULL이 아니면 반환한다.
 *    
 * aCurrBCB - [IN]  이 BCB가 속한 list의 다음 list중에
 *                  empty가 아닌 list의 첫번재 BCB를 찾는다.
 *                  반환되는 BCB는 찾은 BCB가 반환된다.
 *                  못찾으면 NULL이 반환된다.
 ***********************************************************************/
sdBCB* sdbCPListSet::getNextOf( void * aCurrBCB )
{
    UInt      i;
    UInt      sValidListNo;
    UInt      sBeforeListNo;
    sdBCB   * sRet     = NULL;
    
    IDE_DASSERT( aCurrBCB != NULL );

    sBeforeListNo = SD_GET_CPLIST_NO( aCurrBCB );

    if( sBeforeListNo == SDB_CP_LIST_NONE )
    {
        // ID_UINT_MAX인 경우엔 리스트에서 빠져있는 경우
        // aCurrBCB가 이미 list에서 빠져있다. 
        // 이 경우 list의 minBCB를 반환한다.
        sRet = getMin();
    }
    else
    {
        sRet = NULL;

        // aListNo값이 3이고 리스트 개수가 5라면
        // 루프는 4, 5, 6, 7를 순회하고
        // list ID는 4, 0, 1, 2를 순회한다.
        for (i = sBeforeListNo + 1; i < sBeforeListNo + mListCount; i++)
        {
            if (i >= mListCount)
            {
                sValidListNo = i - mListCount;
            }
            else
            {
                sValidListNo = i;
            }

            if (!SMU_LIST_IS_EMPTY(&mBase[sValidListNo]))
            {
                sRet = (sdBCB*)SMU_LIST_GET_FIRST(&mBase[sValidListNo])->mData;
                break;
            }
        }
    }

    return sRet;
}

/***********************************************************************
 * Description :
 *  checkpoint 리스트는 LRU, Flush, prepare List와 다르게 뮤텍스가 하나밖에
 *  존재 하지 않는다.  왜냐면, checkpoint리스트는 삽입과 삭제가 리스트 내의
 *  어느곳에서든 일어날 수 있기 때문이다.
 * 
 *    checkpoint list에서 aBCB를 제거한다.
 *    aBCB에는 어떤 checkpoint list에 자신이 달려있는지
 *    list no가 있다. 이 list no를 참조하여 list mutex를 잡은 후
 *    aBCB를 리스트에서 제거한다.
 *
 * Implementation :
 *    aBCB의 mCPListItem 정보는 checkpoint list mutex(mListMutex)에
 *    의해 보호되어야 한다. 따라서 mCPListItem의 정보를 참조,
 *    변경하기 위해서는 mListMutex를 잡아야 하는데, mListMutex를 
 *    잡기위해서는 mCPListITem.mListNo를 참조해야 한다.
 *    mCPListItem.mListNo 정보를 mutex없이 읽어야 하기 때문에
 *    dirty read이다. 이 값을 읽고 mutex를 잡은 후 다시 mListNo를
 *    검사해야 한다.
 *    
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 ***********************************************************************/
void sdbCPListSet::remove( idvSQL * aStatistics, void * aBCB )
{
    UInt      sListNo;

    IDE_DASSERT( aBCB != NULL );
    IDE_DASSERT( SD_GET_CPLIST_NO( aBCB ) != ID_UINT_MAX );
    // BCB가 속한 list no를 얻어온다.
    // list에 대해 mutex를 잡지 않은 상태이기 때문에
    // dirty read이다.
    sListNo  = SD_GET_CPLIST_NO( aBCB );

    // list에 대해 mutex를 잡는다.
    // BCB의 mCPListItem을 참조하기 위해서는 반드시
    // list mutex를 잡아야 한다.
    IDE_ASSERT( mListMutex[sListNo].lock( aStatistics ) == IDE_SUCCESS );

    // list mutex를 잡은 후 다시 list no를 검사한다.
    // 이 값이 다르다면 한 BCB에 대해서 두 쓰레드가 remove한
    // 경우이다. flush 알고리즘 상 이런 경우는 발생할 순 없다.
    IDE_ASSERT( sListNo == SD_GET_CPLIST_NO( aBCB ) );

    SMU_LIST_DELETE( SD_GET_BCB_CPLISTITEM( aBCB ) );
    mListLength[sListNo] -= 1;

    SDB_INIT_CP_LIST( aBCB );
    IDE_ASSERT( mListMutex[sListNo].unlock() == IDE_SUCCESS );
}

/***********************************************************************
 * Description :
 *  현재 checkpoint list들의 BCB의 recoveryLSN에서 가장 작은 recoveryLSN을
 *  리턴한다.
 *  
 *  aStatistics - [IN]  통계정보
 *  aMinLSN     - [OUT] 가장 작은 recoveryLSN
 ***********************************************************************/
void sdbCPListSet::getMinRecoveryLSN( idvSQL *aStatistics, smLSN *aMinLSN )
{
    UInt       i;
    smuList  * sNode;
    sdBCB    * sFirstBCB;

    IDE_DASSERT(aMinLSN != NULL);

    // checkpoint list에 BCB가 하나도 없으면 MAX값이 리턴된다.
    SM_LSN_MAX( *aMinLSN );

    for (i = 0; i < mListCount; i++)
    {
        // recoveryLSN을 읽기 위해서는 list mutex를 잡아야 한다.
        // BCB의 LSN을 얻어오는 동안 BCB의 LSN이 리스트에 빠지면서 LSN값이
        // 초기화 될 수 있기 때문에, lock을 걸어서 제대로 된 BCB의 LSN값을
        // 얻어온다.
        IDE_ASSERT(mListMutex[i].lock(aStatistics) == IDE_SUCCESS);

        sNode = SMU_LIST_GET_FIRST(&mBase[i]);

        if (sNode == &mBase[i])
        {
            // checkpoint list가 비어 있거나 skip list이면
            // skip한다.
            IDE_ASSERT(mListMutex[i].unlock() == IDE_SUCCESS);
            continue;
        }

        sFirstBCB = (sdBCB*)sNode->mData;

        if ( smLayerCallback::isLSNLT( &sFirstBCB->mRecoveryLSN, aMinLSN )
             == ID_TRUE )
        {
            SM_GET_LSN(*aMinLSN, sFirstBCB->mRecoveryLSN);
        }

        IDE_ASSERT(mListMutex[i].unlock() == IDE_SUCCESS);
    }
}

/***********************************************************************
 * Description :
 *  현재 checkpoint list들의 BCB의 recoveryLSN에서 가장 큰 recoveryLSN을
 *  리턴한다.
 *  
 *  aStatistics - [IN]  통계정보
 *  aMaxLSN     - [OUT] 가장 큰 recoveryLSN
 ***********************************************************************/
void sdbCPListSet::getMaxRecoveryLSN(idvSQL *aStatistics, smLSN *aMaxLSN)
{
    UInt       i;
    smuList  * sNode;
    sdBCB    * sLastBCB;

    IDE_DASSERT(aMaxLSN != NULL);

    SM_LSN_INIT(*aMaxLSN);

    for (i = 0; i < mListCount; i++)
    {
        // recoveryLSN을 읽기 위해서는 list mutex를 잡아야 한다.
        IDE_ASSERT(mListMutex[i].lock(aStatistics) == IDE_SUCCESS);

        sNode = SMU_LIST_GET_LAST(&mBase[i]);
        if (sNode == &mBase[i])
        {
            IDE_ASSERT(mListMutex[i].unlock() == IDE_SUCCESS);
            continue;
        }

        sLastBCB = (sdBCB*)sNode->mData;
        if ( smLayerCallback::isLSNLT( aMaxLSN, &sLastBCB->mRecoveryLSN )
             == ID_TRUE )
        {
            SM_GET_LSN(*aMaxLSN, sLastBCB->mRecoveryLSN);
        }
        IDE_ASSERT(mListMutex[i].unlock() == IDE_SUCCESS);
    }
}
 
/***********************************************************************
 * Description :
 *  현재 checkpoint list가 유지하는 모든 BCB들의 개수를 리턴한다.
 ***********************************************************************/
UInt sdbCPListSet::getTotalBCBCnt()
{
    UInt i;
    UInt sSum = 0;

    for( i = 0 ; i < mListCount ; i++)
    {
        sSum += mListLength[i];
    }

    return sSum;
}

