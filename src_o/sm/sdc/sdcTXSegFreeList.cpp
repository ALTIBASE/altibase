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
 * $Id: sdcTXSegFreeList.cpp 89495 2020-12-14 05:19:22Z emlee $
 **********************************************************************/

# include <idl.h>
# include <ideErrorMgr.h>
# include <ideMsgLog.h>
# include <iduMutex.h>

# include <sdcTXSegMgr.h>
# include <sdcTXSegFreeList.h>

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 FreeList 초기화
 *
 * FreeList의 동시성제어를 위한 Mutex를 초기화하고,
 * 트랜잭션 세그먼트 엔트리들을 모두 FreeList에 연결한다.
 *
 * 한가지 고려할 점은 Prepare 트랜잭션을 고려해야한다.
 *
 * 서버복구과정에서 UndoAll 수행후 Prepare 트랜잭션들이 사용하던
 * 트랜잭션 세그먼트 엔트리는 Online 상태이기 때문에 이를 제외한
 * 나머지들을 가지고 FreeList로 구성해야한다.
 *
 * aArrEntry        - [IN] 트랜잭션 세그먼트 테이블 Pointer
 * aEntryIdx        - [IN] FreeList 순번
 * aFstEntry        - [IN] FreeList가 관리할 첫번째 Entry 순번
 * aLstEntry        - [IN] FreeList가 관리할 첫번째 Entry 순번
 *
 ***********************************************************************/
IDE_RC sdcTXSegFreeList::initialize( sdcTXSegEntry   * aArrEntry,
                                     UInt              aEntryIdx,
                                     SInt              aFstEntry,
                                     SInt              aLstEntry )
{
    SInt              i;
    SChar             sBuffer[128];

    IDE_DASSERT( aFstEntry <= aLstEntry );

    idlOS::memset( sBuffer, 0, 128 );

    idlOS::snprintf( sBuffer, 128, "TXSEG_FREELIST_MUTEX_%"ID_UINT32_FMT,
                     aEntryIdx );

    IDE_TEST( mMutex.initialize( 
                     sBuffer,
                     IDU_MUTEX_KIND_NATIVE,
                     IDV_WAIT_INDEX_LATCH_FREE_DRDB_TXSEG_FREELIST )
              != IDE_SUCCESS );

    mEntryCnt     = aLstEntry - aFstEntry + 1;
    mFreeEntryCnt = mEntryCnt;

    SMU_LIST_INIT_BASE( &mBase );

    for(i = aFstEntry; i <= aLstEntry; i++)
    {
        initEntry4Runtime( &aArrEntry[i], this );

        SMU_LIST_ADD_LAST( &mBase, &aArrEntry[i].mListNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 FreeList 해제
 *
 ***********************************************************************/
IDE_RC sdcTXSegFreeList::destroy()
{
    IDE_ASSERT( SMU_LIST_IS_EMPTY( &mBase ) );
    return mMutex.destroy();
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 할당
 *
 * 트랜잭션 세그먼트 엔트리를 할당한다.
 *
 * aEntry           - [OUT] 트랜잭션 세그먼트 Entry Reference Pointer
 *
 ***********************************************************************/
void sdcTXSegFreeList::allocEntry( sdcTXSegEntry ** aEntry )
{
    smuList *sNode;

    IDE_ASSERT( aEntry != NULL );

    *aEntry = NULL;

    IDE_TEST_CONT( mFreeEntryCnt == 0, CONT_NO_FREE_ENTRY );

    IDE_ASSERT( lock() == IDE_SUCCESS );

    sNode = SMU_LIST_GET_FIRST( &mBase );

    if( sNode != &mBase )
    {
        IDE_ASSERT( mFreeEntryCnt != 0 );

        SMU_LIST_DELETE( sNode );
        mFreeEntryCnt--;
        SMU_LIST_INIT_NODE( sNode );

        *aEntry = (sdcTXSegEntry*)sNode->mData;
        IDE_ASSERT( (*aEntry)->mStatus == SDC_TXSEG_OFFLINE );

        (*aEntry)->mStatus = SDC_TXSEG_ONLINE;
    }

    IDE_ASSERT( unlock() == IDE_SUCCESS );

    IDE_EXCEPTION_CONT( CONT_NO_FREE_ENTRY );

    return;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 할당
 *
 * BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
 * transaction에 특정 segment entry를 binding하는 기능 추가
 * 
 * aEntryID         - [IN]  할당받을 트랜잭션 세그먼트 Entry ID
 * aEntry           - [OUT] 트랜잭션 세그먼트 Entry Reference Pointer
 *
 ***********************************************************************/
void sdcTXSegFreeList::allocEntryByEntryID( UInt             aEntryID,
                                            sdcTXSegEntry ** aEntry )
{
    smuList       * sNode;
    sdcTXSegEntry * sEntry;

    IDE_ASSERT( aEntry != NULL );

    *aEntry = NULL;

    IDE_TEST_CONT( mFreeEntryCnt == 0, CONT_NO_FREE_ENTRY );

    IDE_ASSERT( lock() == IDE_SUCCESS );

    sNode = SMU_LIST_GET_FIRST( &mBase );

    while( 1 )
    {
        if( SMU_LIST_IS_EMPTY( &mBase ) || (sNode == &mBase) )
        {
            // aEntryID의 freeList가 empty이거나
            // list 순회가 끝났을 경우 종료
            break;
        }
            
        sEntry = (sdcTXSegEntry*)sNode->mData;

        // 특정 entry ID의 segment entry를 찾는다.
        if( sEntry->mEntryIdx == aEntryID )
        {
            // 할당할 segment entry가 맞으면
            SMU_LIST_DELETE( sNode );
            mFreeEntryCnt--;
            SMU_LIST_INIT_NODE( sNode );

            *aEntry = (sdcTXSegEntry*)sNode->mData;
            IDE_ASSERT( (*aEntry)->mStatus == SDC_TXSEG_OFFLINE );

            (*aEntry)->mStatus = SDC_TXSEG_ONLINE;
            break;
        }
        else
        {
            // 할당할 segment entry를 찾을 때까지 list 순회
            sNode = SMU_LIST_GET_NEXT( sNode );
        }
    }

    IDE_ASSERT( unlock() == IDE_SUCCESS );

    IDE_EXCEPTION_CONT( CONT_NO_FREE_ENTRY );

    return;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 해제
 *
 * 트랜잭션 세그먼트 엔트리를 해제한다.
 *
 * aEntry        - [IN] 트랜잭션 세그먼트 Entry 포인터
 *
 ***********************************************************************/
void sdcTXSegFreeList::freeEntry( sdcTXSegEntry * aEntry,
                                  idBool          aMoveToFirst )
{
    IDE_ASSERT( aEntry->mStatus == SDC_TXSEG_ONLINE );

    IDE_ASSERT( lock() == IDE_SUCCESS );

    aEntry->mStatus = SDC_TXSEG_OFFLINE;

    initEntry4Runtime( aEntry, this );

    if( aMoveToFirst == ID_TRUE )
    {
        /*
         * 일반적인 경우의 freeEntry는 Buffer Hit를 고려하여 재사용 확률을 높인다.
         */
        SMU_LIST_ADD_FIRST( &mBase, &aEntry->mListNode );
    }
    else
    {
        /*
         * STEAL로 인한 freeEntry는 뒤로 이동하여 재사용 확률을 줄인다
         */
        SMU_LIST_ADD_LAST( &mBase, &aEntry->mListNode );
    }
    
    mFreeEntryCnt++;

    IDE_ASSERT( unlock() == IDE_SUCCESS );

    return;
}

/***********************************************************************
 *
 * Description : 지정된 트랜잭션 세그먼트가 오프라인이면서
 *               Expired되었는지 확인하고 리스트에서 제거
 *
 * aEntry           - [IN] 트랜잭션 세그먼트 Entry 포인터
 * aSegType         - [IN] Segment Type
 * aOldestTransBSCN - [IN] 가장 오래된 트랜잭션의 Stmt의 SCN
 * aTrySuccess      - [OUT] 할당여부
 *
 ***********************************************************************/
void sdcTXSegFreeList::tryAllocExpiredEntryByIdx(
                               UInt             aEntryIdx,
                               sdpSegType       aSegType,
                               smSCN          * aOldestTransBSCN,
                               sdcTXSegEntry ** aEntry )
{
    sdcTXSegFreeList      * sFreeList;
    sdcTXSegEntry         * sEntry;
    UInt                    sState = 0;

    IDE_ASSERT( aOldestTransBSCN != NULL );
    IDE_ASSERT( aEntry           != NULL );

    *aEntry   = NULL;
    sEntry    = sdcTXSegMgr::getEntryByIdx( aEntryIdx );
    sFreeList = sEntry->mFreeList;

    IDE_TEST_CONT( sFreeList->mFreeEntryCnt == 0, CONT_NO_ENTRY );
    IDE_TEST_CONT( sEntry->mStatus == SDC_TXSEG_ONLINE, CONT_NO_ENTRY );

    IDE_ASSERT( sFreeList->lock() == IDE_SUCCESS );
    sState = 1;

    IDE_TEST_CONT( sEntry->mStatus == SDC_TXSEG_ONLINE, CONT_NO_ENTRY );

    IDE_TEST_CONT( isEntryExpired( sEntry, 
                                   aSegType, 
                                   aOldestTransBSCN ) == ID_FALSE,
                    CONT_NO_ENTRY );

    IDE_ASSERT( sFreeList->mFreeEntryCnt != 0 );

    SMU_LIST_DELETE( &sEntry->mListNode );
    sFreeList->mFreeEntryCnt--;
    SMU_LIST_INIT_NODE( &sEntry->mListNode );

    sEntry->mStatus = SDC_TXSEG_ONLINE;

    sState = 0;
    IDE_ASSERT( sFreeList->unlock() == IDE_SUCCESS );

    *aEntry = sEntry;

    IDE_EXCEPTION_CONT( CONT_NO_ENTRY );

    if ( sState != 0 )
    {
        IDE_ASSERT( sFreeList->unlock() == IDE_SUCCESS );
    }

    return;
}


/***********************************************************************
 *
 * Description : 지정된 트랜잭션 세그먼트가 오프라인이면 리스트에서 제거
 *
 * aEntryIdx     - [IN]
 * aEntry        - [OUT] 트랜잭션 세그먼트 Entry 포인터
 *
 ***********************************************************************/
void sdcTXSegFreeList::tryAllocEntryByIdx( UInt               aEntryIdx,
                                           sdcTXSegEntry   ** aEntry )
{
    smuList               * sNode;
    sdcTXSegFreeList      * sFreeList;
    sdcTXSegEntry         * sEntry;
    UInt                    sState = 0;

    IDE_ASSERT( aEntry != NULL );

    *aEntry   = NULL;
    sEntry    = sdcTXSegMgr::getEntryByIdx( aEntryIdx );
    sFreeList = sEntry->mFreeList;

    IDE_TEST_CONT( sFreeList->mFreeEntryCnt == 0, CONT_NO_ENTRY );
    IDE_TEST_CONT( sEntry->mStatus == SDC_TXSEG_ONLINE, CONT_NO_ENTRY );

    IDE_ASSERT( sFreeList->lock() == IDE_SUCCESS );
    sState = 1;

    IDE_TEST_CONT( sEntry->mStatus == SDC_TXSEG_ONLINE, CONT_NO_ENTRY );
    IDE_ASSERT( sFreeList->mFreeEntryCnt != 0 );

    sNode = &sEntry->mListNode;
    SMU_LIST_DELETE( sNode );
    sFreeList->mFreeEntryCnt--;
    SMU_LIST_INIT_NODE( sNode );

    sEntry->mStatus = SDC_TXSEG_ONLINE;

    sState = 0;
    IDE_ASSERT( sFreeList->unlock() == IDE_SUCCESS );

    *aEntry = sEntry;

    IDE_EXCEPTION_CONT( CONT_NO_ENTRY );

    if ( sState != 0 )
    {
        IDE_ASSERT( sFreeList->unlock() == IDE_SUCCESS );
    }

    return;
}
