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
 * $Id: smxTouchPageList.cpp 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

# include <smxTrans.h>
# include <smxTouchPageList.h>

# include <sdnIndexCTL.h>
# include <sdcTableCTL.h>

iduMemPool  smxTouchPageList::mMemPool;
UInt        smxTouchPageList::mTouchNodeSize;
UInt        smxTouchPageList::mTouchPageCntByNode;


/***********************************************************************
 *
 * Description : 트랜잭션들간에 공유자료구조 초기화
 *
 * 트랜잭션 관리자 초기화는 PreProcess 단계에서 초기화되며, 이때 Touch Page List의
 * MemPool 및 해쉬테이블을 초기화한다. 또한, 버퍼크기를 고려하여 Touch Page List
 * 에서 트랜잭션당 최대 Cache할 페이지 개수를 구한다.
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::initializeStatic()
{
    UInt sPowerOfTwo;

    mTouchPageCntByNode = smuProperty::getTransTouchPageCntByNode();

    mTouchNodeSize = ( ID_SIZEOF( smxTouchInfo ) * ( mTouchPageCntByNode - 1 ) ) +
                     ID_SIZEOF( smxTouchNode );

    /* BUG-46434
       mempool에서 IDU_MEMPOOL_TYPE_TIGHT를 사용하기 위해서는
       size, align 값을 2^n 값으로 주어야한다. 
       2^n 값이 되도록 mTouchNodeSize를 수정한다. */

    /* 1. mTouchNodeSize 보다 같거나 큰 2^n 값 구하기 */
    sPowerOfTwo = smuUtility::getPowerofTwo( mTouchNodeSize );

    /* 2. smxTouchInfo를 더 추가할수 있는지 계산 */
    mTouchPageCntByNode += ( ( sPowerOfTwo - mTouchNodeSize ) / ID_SIZEOF( smxTouchInfo ) );

    /* 3. mTouchNodeSize 값을 갱신한다. */
    mTouchNodeSize = sPowerOfTwo;

    IDE_TEST( mMemPool.initialize(
                  IDU_MEM_SM_TRANSACTION_DISKPAGE_TOUCHED_LIST,
                  (SChar*)"TRANSACTION_DISKPAGE_TOUCH_LIST",
                  ID_SCALABILITY_SYS, /* List Count */
                  mTouchNodeSize,
                  1024,    /* itemcount per a chunk */
                  1,       /* xxxx */
                  ID_TRUE, /* Use Mutex */
                  mTouchNodeSize,	/* AlignByte */
                  ID_FALSE,			/* ForcePooling */
                  ID_TRUE,			/* GarbageCollection */
                  ID_TRUE,          /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_TIGHT  /* mempool type */ ) 
              != IDE_SUCCESS );			

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description : 트랜잭션들간에 공유할 자료구조 해제
 *
 * MemPool 및 해쉬테이블을 해제한다.
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::destroyStatic()
{
    IDE_ASSERT( mMemPool.destroy() == IDE_SUCCESS );

    return IDE_SUCCESS;
}


/***********************************************************************
 *
 * Description : 트랜잭션의 Touch Page 리스트 개체 초기화
 *
 * 양방향 리스트인 NodeList를 초기화 및 Hash 초기화하며, Hash 자료구조는 중복
 * 페이지 정보를 제거하기 위해서 사용한다.
 *
 * aTrans - [IN] Owner 트랜잭션 포인터
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::initialize( smxTrans * aTrans )
{
    mTrans      = aTrans;
    mItemCnt    = 0;
    mNodeList   = NULL;

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : 트랜잭션의 Touch Page List 해제
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::destroy()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : 트랜잭션의 Touch Page List 해제
 *
 * 트랜잭션 완료시에 트랜잭션의 Touch Page List를 모두 메모리 해제한다.
 *
 * (TASK-6950)
 * smxTouchPageList::runFastStamping() 에서 할당된 NODE를 모두 free하지만,
 * 혹시나 free 안될경우를 대비해 NODE free하는 코드를 남겨둔다.
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::reset()
{
    smxTouchNode *sCurNode;
    smxTouchNode *sNxtNode = NULL;

    sCurNode = mNodeList;

    while ( sNxtNode != mNodeList )
    {
        sNxtNode = sCurNode->mNxtNode;
        IDE_TEST( mMemPool.memfree( (void *)sCurNode ) != IDE_SUCCESS );
        sCurNode = sNxtNode;
    }

    mNodeList = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션의 Touch Page List의 노드 메모리 할당
 *
 * 트랜잭션이 갱신한 데이타 페이지 정보는 하나의 Touch Page 노드마다
 * TRANSACTION_TOUCH_PAGE_COUNT_BY_NODE_ 프로퍼티만큼만 저장할 수 있으며
 * OverFlow가 발생하면 새로운 Touch Page 노드를 메모리할당하여 Touch Page List
 * 에 연결한다.
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::alloc()
{
    smxTouchNode *sNewNode;

    /* smxTouchPageList_alloc_alloc_NewNode.tc */
    IDU_FIT_POINT("smxTouchPageList::alloc::alloc::NewNode");
    IDE_TEST( mMemPool.alloc( (void**)&sNewNode ) != IDE_SUCCESS );

    initNode( sNewNode );

    if ( mNodeList != NULL )
    {
        sNewNode->mNxtNode = mNodeList->mNxtNode;

        mNodeList->mNxtNode = sNewNode;
    }
    else
    {
        /* nothing to do */
    }

    mNodeList = sNewNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description : 트랜잭션의 Touch Page List의 노드 재사용 (TASK-6950)
 *
 * Touch Page List에 등록된 페이지 정보 갯수가
 * 최대치(mMaxCachePageCnt)에 도달하면
 * 더이상 노드를 할당하지 않고 가장 오래된 노드를 재사용한다.
 *
 ***********************************************************************/
void smxTouchPageList::reuse()
{
    IDE_DASSERT( mNodeList != NULL );

    mNodeList = mNodeList->mNxtNode;

    mItemCnt -= mNodeList->mPageCnt;

    mNodeList->mPageCnt = 0;

    return;
}

/***********************************************************************
 *
 * Description : 페이지 정보 추가
 *
 * 트랜잭션이 갱신한 데이타 페이지를 Touch Page 노드에 추가한다.
 * 갱신한 데이타 페이지 정보는 SpaceID, PageID, CTS 순번이 저장된다.
 *
 * aSpaceID   - [IN] 테이블스페이스 ID
 * aPageID    - [IN] 데이타페이지의 PID
 * aCTSlotIdx - [IN] 바인딩한 CTS 번호
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::add( scSpaceID     aSpaceID,
                              scPageID      aPageID,
                              SShort        aCTSlotIdx )
{
    smxTouchNode * sCurNode;
    UInt           sItemCnt;

    IDE_ASSERT( aPageID != SC_NULL_PID );
    IDE_ASSERT( (aCTSlotIdx & SDP_CTS_IDX_MASK)
                != SDP_CTS_MAX_IDX );

    sCurNode = mNodeList;

    IDE_TEST_CONT( mMaxCachePageCnt == 0,
                   do_not_use_touch_page_list );

    if ( ( sCurNode == NULL ) ) 
    {
        IDE_TEST( alloc() != IDE_SUCCESS );
        sCurNode = mNodeList;
    }
    else
    {
        if ( sCurNode->mPageCnt >= mTouchPageCntByNode )
        {
            if ( mItemCnt < mMaxCachePageCnt )
            {
                IDE_TEST( alloc() != IDE_SUCCESS );
                sCurNode = mNodeList;
            }
            else
            {
                /* 저장된 page수가 상한에 도달하면,
                   가장 오랜된 node를 재사용한다. */
                reuse();
                sCurNode = mNodeList;
            }
        }
        else
        {
            /* nothing to do  */
        }
    }

    sItemCnt = sCurNode->mPageCnt;

    sCurNode->mArrTouchInfo[ sItemCnt ].mSpaceID    = aSpaceID;
    sCurNode->mArrTouchInfo[ sItemCnt ].mPageID     = aPageID;
    sCurNode->mArrTouchInfo[ sItemCnt ].mCTSlotIdx  = aCTSlotIdx;

    sCurNode->mPageCnt++;
    mItemCnt++;

    IDE_EXCEPTION_CONT( do_not_use_touch_page_list );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션 Fast TimeStamping 수행
 *
 * 트랜잭션 커밋과정에서 수행되는 Fast Stamping의 조건은 페이지가 버퍼상에 없거나
 * 다른 트랜잭션이나 Flusher에 의해서 X-Latch가 획득되어 있는 경우에는 실패한다.
 * 즉, 버퍼상에서 Hit되며 X-Latch가 바로 획득될 수 있는 인덱스 페이지와 데이타
 * 페이지에 대해서 구분지어 각각 수행한다.
 *
 *  aCommitSCN - [IN] 트랜잭션 CommitSCN
 *
 ***********************************************************************/
IDE_RC smxTouchPageList::runFastStamping( smSCN * aCommitSCN )
{
    UInt            i;
    sdpPhyPageHdr * sPage     = NULL;
    UInt            sFixState = 0;
    smxTouchNode  * sCurNode;
    smxTouchInfo  * sTouchInfo;
    idBool          sSuccess;
    sdSID           sTransTSSlotSID;
    smSCN           sFstDskViewSCN;
    smxTouchNode  * sNxtNode = NULL;

    sTransTSSlotSID = smxTrans::getTSSlotSID((void*)mTrans);
    sFstDskViewSCN  = smxTrans::getFstDskViewSCN( (void*)mTrans );

    sCurNode = mNodeList;

    /* (TASK-6950) 아래 loop 이해를 돕기위한 설명

       - mNodeList에 할당된 NODE가 없다면(=NULL), loop에 들어가지 않는다.
       - mNodeList가 NULL이 아닌경우, sNxtNode는 초기값이 NULL이므로 항상 loop에 들어간다. 
       - mNodeList에 하나의 NODE가 할당되어 있다면 그 NODE의 mNxtNode는 자기자신을 가르킨다. */

    while ( sNxtNode != mNodeList )
    {
        for( i = 0; i < sCurNode->mPageCnt; i++ )
        {
            sPage = NULL;
            sTouchInfo = &(sCurNode->mArrTouchInfo[i]);

            IDE_TEST( sdbBufferMgr::fixPageWithoutIO(
                                         mStatistics,
                                         sTouchInfo->mSpaceID,
                                         sTouchInfo->mPageID,
                                         (UChar**)&sPage ) 
                      != IDE_SUCCESS );
            sFixState = 1;

            if( sPage == NULL )
            {
                continue;
            }

            sdbBufferMgr::latchPage( mStatistics,
                                     (UChar*)sPage,
                                     SDB_X_LATCH,
                                     SDB_WAIT_NO,
                                     &sSuccess );
            if( sSuccess == ID_FALSE )
            {
                sFixState = 0;
                IDE_TEST( sdbBufferMgr::unfixPage( mStatistics,
                                                   (UChar*)sPage )
                          != IDE_SUCCESS );
                continue;
            }

            sFixState = 2;

            if( sPage->mPageType == SDP_PAGE_DATA )
            {
                sdcTableCTL::runFastStamping( &sTransTSSlotSID,
                                              &sFstDskViewSCN,
                                              sPage,
                                              sTouchInfo->mCTSlotIdx,
                                              aCommitSCN );
            }
            else
            {
                /* BUG-29280 - non-auto commit D-Path Insert 과정중
                 *             rollback 발생한 경우 commit 시 서버 죽는 문제
                 *
                 * DATA 페이지가 아니면 Index 페이지여야 한다. */
                IDE_ASSERT( (sPage->mPageType == SDP_PAGE_INDEX_BTREE) ||
                            (sPage->mPageType == SDP_PAGE_INDEX_RTREE) );

                sdnIndexCTL::fastStamping( mTrans,
                                           (UChar*)sPage,
                                           sTouchInfo->mCTSlotIdx,
                                           aCommitSCN );
            }

            sdbBufferMgr::setDirtyPageToBCB( mStatistics,
                                             (UChar*)sPage );

            sFixState = 1;
            sdbBufferMgr::unlatchPage( (UChar*)sPage );

            sFixState = 0;
            IDE_TEST( sdbBufferMgr::unfixPage( mStatistics,
                                               (UChar*)sPage )
                      != IDE_SUCCESS );
        }

        sNxtNode = sCurNode->mNxtNode;

        /* FAST STAMPING을 수행한 NODE는 해제한다. (TASK-6950) */
        IDE_TEST( mMemPool.memfree( (void *)sCurNode ) != IDE_SUCCESS );

        sCurNode = sNxtNode;
    }

    mNodeList = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sFixState )
    {
        case 2:
            sdbBufferMgr::unlatchPage( (UChar*)sPage );
        case 1:
            IDE_ASSERT( sdbBufferMgr::unfixPage( mStatistics,
                                                 (UChar*)sPage )
                        == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}
