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
 * $Id: smnbPointerbaseBuild.cpp 15589 2006-04-18 01:47:21Z leekmo $
 **********************************************************************/


#include <ide.h>
#include <smErrorCode.h>

#include <smc.h>
#include <smm.h>
#include <smnDef.h>
#include <smnReq.h>
#include <smnManager.h>
#include <sctTableSpaceMgr.h>
#include <smnbPointerbaseBuild.h>
#include <sgmManager.h>

extern smiStartupPhase smiGetStartupPhase();

smnbPointerbaseBuild::smnbPointerbaseBuild() : idtBaseThread()
{

}

smnbPointerbaseBuild::~smnbPointerbaseBuild()
{

}

/* ------------------------------------------------
 * Row base Memory Index Build Main Routine
 *
 * 1. Extraction
 * 2. Quick Sort with divide & conquer
 * 3. Make Tree
 * ----------------------------------------------*/
IDE_RC smnbPointerbaseBuild::buildIndex( void               * aTrans,
                                         idvSQL             * aStatistics,
                                         smcTableHeader     * aTable,
                                         smnIndexHeader     * aIndex,
                                         UInt                 aThreadCnt,
                                         idBool               aIsPersistent,
                                         idBool               aIsNeedValidation,
                                         smnGetPageFunc       aGetPageFunc,
                                         smnGetRowFunc        aGetRowFunc,
                                         SChar              * aNullPtr )
{
    smnbPointerbaseBuild  * sThreads  = NULL;
    smnIndexModule        * sIndexModules;
    smnbStatistic           sIndexStat;
    smnpJobItem             sJobInfo;
    SInt                    sState    = 0;
    UInt                    sThrState = 0;
    UInt                    sRowCount = 0;
    UInt                    i = 0;
    SInt                    sNodeAllocState = 0;

    IDE_ASSERT ( aIndex->mModule != NULL );
    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aThreadCnt > 0 );

    sIndexModules = aIndex->mModule;
    
    ideLog::log( IDE_SM_0, "\
=========================================\n\
 [MEM_IDX_CRE] 0. BUILD INDEX (Pointer)  \n\
                  NAME : %s              \n\
                  ID   : %u              \n\
=========================================\n\
    1. Extract Row Pointer               \n\
    2. Sort                              \n\
    3. Make Tree                         \n\
=========================================\n\
          BUILD_THREAD_COUNT     : %u    \n\
=========================================\n",
                 aIndex->mName,
                 aIndex->mId,
                 aThreadCnt );

    /* smnbPointerbaseBuild_buildIndex_malloc_Threads.tc */
    IDU_FIT_POINT("smnbPointerbaseBuild::buildIndex::malloc::Threads");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 (ULong)ID_SIZEOF(smnbPointerbaseBuild) * aThreadCnt,
                                 (void**)&sThreads )
              != IDE_SUCCESS);

    for( i = 0; i < aThreadCnt; i++ )
    {
        new (sThreads + i) smnbPointerbaseBuild;
    }

    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF( smnbStatistic ) );

    if ( ( (aIndex->mFlag & SMI_INDEX_PERSISTENT_MASK) == SMI_INDEX_PERSISTENT_ENABLE ) &&
         ( smrRecoveryMgr::isABShutDown() == ID_FALSE) &&
         ( smuProperty::forceIndexPersistenceMode() != SMN_INDEX_PERSISTENCE_NOUSE ) &&
         ( aIsPersistent == ID_TRUE ) )
    {
        if( smnbBTree::makeNodeListFromDiskImage( aTable, aIndex )
            == IDE_SUCCESS )
        {
            sThreads[0].finalize( aIndex );
            IDE_CONT( BUILD_DONE );
        }
    }

// ------------------------------------------------
// Phase 1. Extract Row Pointer
// ------------------------------------------------
    ideLog::log( IDE_SM_0, "\
========================================\n\
 [MEM_IDX_CRE] 1. Extract Row Pointer   \n\
========================================\n" );

    IDE_TEST( sThreads[0].extractRow( aStatistics,
                                      aTable,
                                      aIndex,
                                      aIsNeedValidation,
                                      &sRowCount,
                                      aGetPageFunc,
                                      aGetRowFunc )
              != IDE_SUCCESS );

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [MEM_IDX_CRE] 1. Extract Done %u       \n\
========================================\n", sRowCount );

    if( (sRowCount != 0) &&
        (aIndex->mModule->mFlag & SMN_BOTTOMUP_BUILD_MASK)
        == SMN_BOTTOMUP_BUILD_ENABLE )
    {
        IDE_TEST( smnManager::initializeJobInfo( &sJobInfo )
                  != IDE_SUCCESS );
        sState = 1;

        sJobInfo.mTableHeader = aTable;
        sJobInfo.mIndexHeader = aIndex;
        
// ------------------------------------------------
// Phase 2. Quick Sort
// ------------------------------------------------
    ideLog::log( IDE_SM_0, "\
========================================\n\
 [MEM_IDX_CRE] 2. Sort Row Pointer      \n\
========================================\n" );

        IDE_TEST( sThreads[0].prepareQuickSort( aTable, aIndex, &sJobInfo )
                  != IDE_SUCCESS);
        sNodeAllocState = 1;

        for(i = 0; i < aThreadCnt; i++)
        {
            IDE_TEST( sThreads[i].initialize( aTrans,
                                              aStatistics,
                                              &sJobInfo,
                                              aNullPtr )
                      != IDE_SUCCESS);

            // BUG-30426 [SM] Memory Index 생성 실패시
            // thread를 destroy 해야 합니다.
            sThrState++;
            
            IDE_TEST( sThreads[i].start() != IDE_SUCCESS);
        }

        for(i = 0; i < aThreadCnt; i++)
        {
            IDE_TEST_RAISE( sThreads[i].join() != IDE_SUCCESS,
                            err_thread_join );

            if(sThreads[i].mError != 0)
            {
                IDE_SET( ideSetErrorCode( sThreads[i].mError ) );
                IDE_TEST( ID_TRUE );
            }
        }

        // BUG-30426 [SM] Memory Index 생성 실패시
        // thread를 destroy 해야 합니다.
        while( sThrState > 0 )
        {
            IDE_TEST(sThreads[--sThrState].destroy() != IDE_SUCCESS);
        }

// ------------------------------------------------
// Phase 3. Make Tree
// ------------------------------------------------
    ideLog::log( IDE_SM_0, "\
========================================\n\
 [MEM_IDX_CRE] 3. Make Tree             \n\
========================================\n" );

        IDE_TEST( sThreads[0].finalize( aIndex ) != IDE_SUCCESS );
        sNodeAllocState = 2;

        sState = 0;
        IDE_TEST( smnManager::destroyJobInfo( &sJobInfo )
                  != IDE_SUCCESS);
    }

    IDE_EXCEPTION_CONT( BUILD_DONE );

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [MEM_IDX_CRE] 4. Set Stat              \n\
========================================\n" );
    if( ((smnbHeader*)aIndex->mHeader)->root != NULL )
    {
        /* BUG-39681 Do not need statistic information when server start */
        if( smiGetStartupPhase() == SMI_STARTUP_SERVICE )
        {
            IDE_TEST( smnbBTree::gatherStat( aStatistics,
                                             NULL, /* aTrans - nologging */
                                             1.0f, /* 100% */
                                             aThreadCnt,
                                             aTable,
                                             NULL, /*TotalTableArg*/
                                             aIndex,
                                             NULL,
                                             ID_FALSE )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* 데이터가 전혀 없는 상태면, 통계를 Invalid상태로 둠. */
    }

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [MEM_IDX_CRE] Build Completed          \n\
========================================\n" );

    // To fix BUG-20631
    if(sThreads != NULL)
    {
        (void)iduMemMgr::free(sThreads);
        
        sThreads = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thread_join);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_Systhrjoin ) );
    }
    IDE_EXCEPTION_END;

    if(sThrState != 0)
    {
        IDE_PUSH();
        for(i++; i < aThreadCnt; i++)
        {
            (void)sThreads[i].join();
        }
        IDE_POP();
    }

    IDE_PUSH();

    // BUG-30426 [SM] Memory Index 생성 실패시
    // thread를 destroy 해야 합니다.
    for(i = 0; i < sThrState; i++)
    {
        (void)sThreads[i].destroy();
    }

    if(sThreads != NULL)
    {
        (void)iduMemMgr::free(sThreads);
        
        sThreads = NULL;
    }

    if( sState != 0 )
    {
        (void)smnManager::destroyJobInfo(&sJobInfo);
    }

    switch( sNodeAllocState )
    {
        case 2:
            /* internalnode / leafnode 모두 할당이 끝난 경우 */
            (void)(sIndexModules)->mFreeAllNodeList( aStatistics,
                                                     aIndex,
                                                     aTrans );
            break;
        case 1:
            /* BUG-41421 키 중복으로 인덱스 생성이 실패할 경우
             * leafnode에 할당된 메모리를 반환해야 한다. */
            /* leafnode만 할당이 끝난 경우 */
            (void)removePrepareNodeList( aIndex );
            break;
        case 0:
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * Index build 쓰레드 초기화
 * ----------------------------------------------*/
IDE_RC smnbPointerbaseBuild::initialize( void        * aTrans,
                                         idvSQL      * aStatistics,
                                         smnpJobItem * aJobInfo,
                                         SChar       * aNullPtr )
{
    mStatistics = aStatistics;
    mJobInfo    = aJobInfo;
    mTrans      = aTrans;
    mNullPtr    = aNullPtr;
    mError      = 0;

    IDE_TEST( mStack.initialize( IDU_MEM_SM_SMN, ID_SIZEOF( smnSortStack ) )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC smnbPointerbaseBuild::destroy()
{
    IDE_TEST(mStack.destroy() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smnbPointerbaseBuild::run()
{
    IDE_TEST( quickSort( mTrans,
                         mJobInfo->mIndexHeader,
                         mNullPtr,
                         &mStack,
                         mJobInfo) != IDE_SUCCESS);

    return;

    IDE_EXCEPTION_END;
    
    mError = ideGetErrorCode();

    IDL_MEM_BARRIER;
    
}

IDE_RC smnbPointerbaseBuild::quickSort( void            * aTrans,
                                        smnIndexHeader  * aIndex,
                                        SChar           * aNullPtr,
                                        iduStackMgr     * aStack,
                                        smnpJobItem     * aJobInfo)
{
    smnbLNode*        sSlotNode;
    smnbLNode*        sRightNode;
    smnbLNode*        sLeftNode;
    UInt              sLeftPos;
    UInt              sRightPos;
    UInt              sSlotPos;
    smnbHeader*       sIndexHeader;
    smnbSortStack     sCurStack;
    idBool            sEmpty;
    UInt              sRows;
    SInt              sState = 0;
    SInt              sMutexState = 0;
    smnbStatistic     sIndexStat;      // BUG-18201
    
    sIndexHeader = (smnbHeader*)(aIndex->mHeader);

    // BUG-18201
    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF(smnbStatistic) );

    while(aJobInfo->mIsFinish == ID_FALSE)
    {
        IDE_TEST(aStack->pop(ID_FALSE, &sCurStack, &sEmpty)
                 != IDE_SUCCESS);

        if(sEmpty == ID_TRUE)
        {
            while(aJobInfo->mIsFinish == ID_FALSE)
            {
                IDE_TEST(aJobInfo->mMutex.lock(NULL /* idvSQL* */)
                         != IDE_SUCCESS);
                sMutexState = 1;
                
                IDE_TEST(aJobInfo->mStack.pop(ID_TRUE, &sCurStack, &sEmpty)
                         != IDE_SUCCESS);

                if(sEmpty == ID_FALSE)
                {
                    if(sState == 0)
                    {
                        sState = 1;
                        aJobInfo->mJobProcessCnt++;
                    }
                    sMutexState = 0;
                    IDE_TEST(aJobInfo->mMutex.unlock() != IDE_SUCCESS);
                    break;
                }

                if(sState == 1)
                {
                    sState = 0;
                    aJobInfo->mJobProcessCnt--;
                }
                
                if(aJobInfo->mJobProcessCnt == 0)
                {
                    sMutexState = 0;
                    IDE_TEST(aJobInfo->mMutex.unlock() != IDE_SUCCESS);
                    break;
                }

                sMutexState = 0;
                IDE_TEST(aJobInfo->mMutex.unlock() != IDE_SUCCESS);
                
                idlOS::thr_yield();
            }
        }

        if(sEmpty == ID_TRUE)
        {
            break;
        }
        
        sLeftNode  = (smnbLNode*)(sCurStack.leftNode);
        sLeftPos   = sCurStack.leftPos;
        sRightNode = (smnbLNode*)(sCurStack.rightNode);
        sRightPos  = sCurStack.rightPos;

        if ( sLeftPos == (UInt)( sLeftNode->mSlotCount - 1 ) )
        {
            sLeftPos  = 0;
            sLeftNode = (smnbLNode*)(sLeftNode->nextSPtr);
        }
        else
        {
            sLeftPos++;
        }

        while( sLeftNode->sequence < sRightNode->sequence ||
               (sLeftNode->sequence == sRightNode->sequence &&
                sLeftPos < sRightPos) )
        {
            IDE_TEST( partition( aTrans,
                                 sIndexHeader,
                                 &sIndexStat,
                                 aNullPtr,
                                 (smnbLNode**)(aJobInfo->mArrNode),
                                 aJobInfo->mIsUnique,
                                 sLeftNode,
                                 sLeftPos,
                                 sRightNode,
                                 sRightPos,
                                 &sSlotNode,
                                 &sSlotPos ) != IDE_SUCCESS );

            if(aTrans != NULL)
            {
                IDE_TEST( iduCheckSessionEvent( mStatistics )
                          != IDE_SUCCESS );
            }

            /* PROJ-2433 */
            sRows = ( sRightNode->sequence - sSlotNode->sequence ) *
                    SMNB_LEAF_SLOT_MAX_COUNT( sIndexHeader );

            sCurStack.leftNode  = sSlotNode;
            sCurStack.leftPos   = sSlotPos;
            sCurStack.rightNode = sRightNode;
            sCurStack.rightPos  = sRightPos;
            
            if(sRows > smuProperty::getIndexBuildMinRecordCount() )
            {
                IDE_TEST(aJobInfo->mStack.push(ID_TRUE, &sCurStack)
                         != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(aStack->push(ID_FALSE, &sCurStack)
                         != IDE_SUCCESS);
            }
            
            sRightNode = sSlotNode;
            sRightPos  = sSlotPos;

            if(aJobInfo->mIsFinish == ID_TRUE)
            {
                break;
            }
        }//While
    }

    if(aJobInfo->mIsFinish == ID_FALSE)
    {
        IDE_TEST(aJobInfo->mMutex.lock(NULL /* idvSQL* */)
                 != IDE_SUCCESS);
        aJobInfo->mIsFinish = ID_TRUE;
        IDE_TEST(aJobInfo->mMutex.unlock() != IDE_SUCCESS);
    }

    /* BUG-39681 Do not need statistic information when server start */
    if( smiGetStartupPhase() == SMI_STARTUP_SERVICE )
    {
        // BUG-18201 : Memory/Disk Index 통계치
        SMNB_ADD_STATISTIC( &(sIndexHeader->mStmtStat),
                            &sIndexStat );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aJobInfo->mIsFinish = ID_TRUE;

    if(sMutexState != 0)
    {
        IDE_PUSH();
        (void)aJobInfo->mMutex.unlock();
        IDE_POP();
    }
    
    return IDE_FAILURE;
}

/* ---------------------------------------------------------
 * Description:
 *
 * - data page들로부터 row pointer를 추출하여
 *   임시 leaf node에 저장
 * ---------------------------------------------------------*/
IDE_RC smnbPointerbaseBuild::extractRow( idvSQL           * aStatistics,
                                         smcTableHeader   * aTable,
                                         smnIndexHeader   * aIndex,
                                         idBool             aIsNeedValidation,
                                         UInt             * aRowCount,
                                         smnGetPageFunc     aGetPageFunc,
                                         smnGetRowFunc      aGetRowFunc )
{
    smnIndexModule    * sIndexModules;
    smnbHeader        * sHeader;
    smnbColumn        * sColumn;
    UInt                sRowCount = 0;
    scPageID            sPageID;
    SChar             * sFence;
    SChar             * sRow;
    idBool              sLocked = ID_FALSE;
    SChar             * sValue;
    UInt                sLength = 0;

    sIndexModules = aIndex->mModule;
    sHeader = (smnbHeader*)aIndex->mHeader;

    sPageID = SM_NULL_OID;

    while( 1 )
    {
        /* 함수 내부에서 Lock 풀고 다시 잡아줌 */
        IDE_TEST( aGetPageFunc( aTable, &sPageID, &sLocked ) != IDE_SUCCESS );

        if( sPageID == SM_NULL_OID )
        {
            break;
        }

        sRow = NULL;

        while( 1 )
        {
            IDE_TEST( aGetRowFunc( aTable,
                                   sPageID,
                                   &sFence,
                                   &sRow,
                                   aIsNeedValidation ) != IDE_SUCCESS );

            if( sRow == NULL )
            {
                break;
            }

            // check not null
            if( sHeader->mIsNotNull == ID_TRUE )
            {
                // 하나라도 Null이면 안되기 때문에
                // 모두 Null인 경우만 체크하는 isNullKey를 사용하면 안된다

                for( sColumn = &sHeader->columns[0];
                     sColumn < sHeader->fence;
                     sColumn++ )
                {
                    if( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_FIXED )
                    {
                        sValue = sRow + sColumn->column.offset;
                    }
                    else
                    {
                        sValue = sgmManager::getVarColumn(sRow, &(sColumn->column), &sLength);
                    }

                    if( (sValue == NULL) ||
                        (sColumn->isNull( &(sColumn->column), sValue ) == ID_TRUE) )
                    {
                        IDE_RAISE( ERR_NOTNULL_VIOLATION );
                    }
                }
            }

            IDE_TEST( insertRowToLeaf( aIndex, sRow )
                      != IDE_SUCCESS );
            sRowCount++;
        }
    }
    *aRowCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOTNULL_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_NULL_VIOLATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    (void)(sIndexModules)->mFreeAllNodeList( aStatistics,
                                             aIndex,
                                             NULL );

    if( sLocked == ID_TRUE )
    {
        // null일 경우 latch가 잡힌 page의 latch를 해제
        (void)smnManager::releasePageLatch( aTable, sPageID );
        sLocked = ID_FALSE;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/* ---------------------------------------------------------
 * Description:
 *
 * - key 중복으로 인한 인덱스 생성 실패시 인덱스 헤더의 root가 아닌
 *   pFstNode를 타고 들어가 노드에 할당된 메모리를 반환해야 한다.
 * ---------------------------------------------------------*/
IDE_RC smnbPointerbaseBuild::removePrepareNodeList( smnIndexHeader * aIndex )
{
    smnbHeader      * sHeader;
    smnbLNode       * sCurLNode = NULL;
    smnbLNode       * sNxtLNode = NULL;

    sHeader = (smnbHeader*)aIndex->mHeader;

    if( sHeader->pFstNode != NULL )
    {
        sCurLNode = sHeader->pFstNode;

        while( sCurLNode != NULL )
        {
            sNxtLNode = sCurLNode->nextSPtr;        
            IDE_TEST( smnbBTree::freeNode( (smnbNode*)sCurLNode ) != IDE_SUCCESS );
            sCurLNode = sNxtLNode;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smnbPointerbaseBuild::insertRowToLeaf( smnIndexHeader  * aIndex,
                                              SChar           * aRow )
{
    smnbHeader      * sHeader;
    smnbLNode       * sCurLNode;

    sHeader = (smnbHeader*)aIndex->mHeader;

    if( sHeader->pFstNode == NULL )
    {
        IDE_TEST( sHeader->mNodePool.allocateSlots(1,
                                                (smmSlot**)&(sCurLNode))
                  != IDE_SUCCESS );

        smnbBTree::initLeafNode( sCurLNode,
                                 sHeader,
                                 IDU_LATCH_UNLOCKED );
        sHeader->nodeCount++;

        sCurLNode->sequence = 0;
        sHeader->pFstNode = sCurLNode;
        sHeader->pLstNode = sCurLNode;
    }

    // BUG-27456 Klocwork SM(4)
    IDE_ERROR( sHeader->pLstNode != NULL );

    if ( sHeader->pLstNode->mSlotCount >= SMNB_LEAF_SLOT_MAX_COUNT( sHeader ) )
    {
        IDE_TEST( sHeader->mNodePool.allocateSlots( 1,
                                                    (smmSlot**)&(sCurLNode) )
                != IDE_SUCCESS );

        smnbBTree::initLeafNode( sCurLNode,
                                 sHeader,
                                 IDU_LATCH_UNLOCKED );
        sHeader->nodeCount++;

        sCurLNode->sequence = sHeader->pLstNode->sequence + 1;
        sHeader->pLstNode->nextSPtr = sCurLNode;
        sCurLNode->prevSPtr = sHeader->pLstNode;

        sHeader->pLstNode = sCurLNode;
    }
    else
    {
        sCurLNode = sHeader->pLstNode;
    }

    smnbBTree::setLeafSlot( sCurLNode,
                            sCurLNode->mSlotCount,
                            aRow,
                            NULL ); /* PROJ-2433 : direct key는 아래에서 세팅함 */

    /* PROJ-2433 direct key 세팅 */
    if ( SMNB_IS_DIRECTKEY_INDEX( sHeader ) == ID_TRUE )
    {
        IDE_TEST( smnbBTree::makeKeyFromRow( sHeader,
                                             aRow,
                                             SMNB_GET_KEY_PTR( sCurLNode, sCurLNode->mSlotCount ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    (sCurLNode->mSlotCount)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbPointerbaseBuild::prepareQuickSort( smcTableHeader * aTable,
                                               smnIndexHeader * aIndex,
                                               smnpJobItem    * aJobInfo )
{
    UInt             sNode;
    UInt             sMask  = 0x80000000;
    UInt             i      = 0;
    UInt             j;
    UInt             sState = 0;
    smnbLNode      * sCurNode;
    smnbLNode     ** sArrNode = NULL;
    smnIndexHeader * sSrcIndex;
    smnbSortStack    sStackInfo;
    smnbHeader     * sIndexHeader;
    smnbHeader     * sSrcIndexHeader;

    sIndexHeader = (smnbHeader*)(aIndex->mHeader);
    sSrcIndex    = (smnIndexHeader*)(sIndexHeader->tempPtr);

    //To fix BUG-5047
    if(aJobInfo->mIsPersIndex == ID_TRUE)
    {
        if( smnbBTree::makeNodeListFromDiskImage(aTable, aIndex)
            != IDE_SUCCESS)
        {
            aJobInfo->mIsPersIndex = ID_FALSE;
        }
    }

    if(aJobInfo->mIsPersIndex == ID_FALSE)
    {
        if(sIndexHeader->nodeCount == 0 && sSrcIndex != NULL)
        {
            sSrcIndexHeader = (smnbHeader*)(sSrcIndex->mHeader);
        
            IDE_TEST(duplicate(sSrcIndex, aIndex) != 0);

            smnbBTree::lockTree( sSrcIndexHeader );
            sSrcIndexHeader->cRef--;
            smnbBTree::unlockTree( sSrcIndexHeader );
        }

        while(1)
        {
            if(sIndexHeader->cRef == 0)
            {
                break;
            }

            idlOS::sleep(1);
        }

        sNode = sIndexHeader->nodeCount;
    
        for(i = 32; i > 0; i--) 
        {
            if((sMask & sNode) == sMask)
            {
                break;
            }

            sMask = sMask >> 1;
        }

        if(i != 0)
        {
            /* smnbPointerbaseBuild_prepareQuickSort_malloc_ArrNode.tc */
            IDU_FIT_POINT("smnbPointerbaseBuild::prepareQuickSort::malloc::ArrNode");
            IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SMN,
                                       ID_SIZEOF(smnbLNode *) * sNode,
                                       (void**)&sArrNode)
                     != IDE_SUCCESS);
            sState = 1;

            sCurNode = sIndexHeader->pFstNode;

            for(j = 0; j < sNode; j++)
            {
                sArrNode[j] = sCurNode;
                sCurNode    = (smnbLNode*)(sCurNode->nextSPtr);
            }

            aJobInfo->mArrNode   = sArrNode;

            /* BUG-42152 when adding column with default value on memory partitioned table,
             * local unique constraint is not validated.
             */
            if ( ( ( aIndex->mFlag & SMI_INDEX_UNIQUE_MASK )
                   == SMI_INDEX_UNIQUE_ENABLE ) ||
                 ( ( aIndex->mFlag & SMI_INDEX_LOCAL_UNIQUE_MASK )
                   == SMI_INDEX_LOCAL_UNIQUE_ENABLE ) )
            {
                aJobInfo->mIsUnique = ID_TRUE;
            }
            else
            {
                aJobInfo->mIsUnique = ID_FALSE;
            }

            aJobInfo->mIsFinish  = ID_FALSE;

            sStackInfo.leftNode  = sIndexHeader->pFstNode;
            sStackInfo.leftPos   = (UInt)-1;
            sStackInfo.rightNode = sIndexHeader->pLstNode;
            sStackInfo.rightPos  = sIndexHeader->pLstNode->mSlotCount - 1;

            IDE_TEST(aJobInfo->mStack.push(ID_FALSE,  &sStackInfo) != IDE_SUCCESS);
        }
        else
        {
            aJobInfo->mIsFinish = ID_TRUE;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sArrNode ) == IDE_SUCCESS );
            sArrNode = NULL;

            if(aJobInfo->mArrNode != NULL)
            {
                aJobInfo->mArrNode = NULL;
            }
        default:
            break;
    }

    aJobInfo->mIsFinish = ID_TRUE;
    
    return IDE_FAILURE;
}

IDE_RC smnbPointerbaseBuild::finalize( smnIndexHeader * aIndex )
{
    smnbHeader * sIndexHeader;

    sIndexHeader = (smnbHeader*)(aIndex->mHeader);
    if( sIndexHeader->nodeCount > 0 )
    {
        IDE_TEST( makeTree( sIndexHeader,
                            (smnbLNode*)(sIndexHeader->pFstNode),
                            (smnbNode**)&(sIndexHeader->root))
                  != IDE_SUCCESS);
    }
    else
    {
        sIndexHeader->root = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sIndexHeader->root = NULL;

    return IDE_FAILURE;
}

IDE_RC smnbPointerbaseBuild::partition( void            * aTrans,
                                        smnbHeader      * aIndexHeader,
                                        smnbStatistic   * aIndexStat,
                                        SChar           * aNull,
                                        smnbLNode      ** aArrNode,
                                        idBool            aIsUnique,
                                        smnbLNode       * aLeftNode,
                                        UInt              aLeftPos,
                                        smnbLNode       * aRightNode,
                                        UInt              aRightPos,
                                        smnbLNode      ** aOutNode,
                                        UInt            * aOutPos )
{
    SInt        i, j;
    SChar     * rowPtr;
    SChar     * sRow    = NULL;
    void      * sKey    = NULL;
    void      * sKeyBuf = NULL;
    SInt        sResult;
    smTID       sTID;
    idBool      sIsEqual;
    void      * sOIDPtr;
    SInt        sState  = 0;
    SInt        sViewNodeState = 0;

    const smnbColumn* sColumns;
    const smnbColumn* sFence;
    
    sColumns = aIndexHeader->columns;
    sFence   = aIndexHeader->fence;

    /* BUG-46955 */
    rowPtr = getPivot( aIndexHeader,
                       aIndexStat,
                       aArrNode,
                       aLeftNode,
                       aLeftPos,
                       aRightNode,
                       aRightPos );
 
    i = aLeftPos - 1;
    j = aRightPos + 1;

    if ( SMNB_IS_DIRECTKEY_INDEX( aIndexHeader ) == ID_TRUE )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                     aRightNode->mKeySize,
                                     &sKeyBuf )
                  != IDE_SUCCESS );
        sState = 1;
    }
    else
    {
        /* nothing to do */
    }

    while(1)
    {
        do
        {
            if(j == 0)
            {
                aRightNode = aRightNode->prevSPtr;
                j = aRightNode->mSlotCount - 1;
            }
            else
            {
                j--;
            }


            if ( aRightNode->mRowPtrs[j] == rowPtr )
            {
                /* 동일한 포인터라면 같은 값이기 때문에 비교할 필요없다. */
                sResult = 0;
            }
            else
            {
                sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                                         sColumns,
                                                         sFence,
                                                         aRightNode->mRowPtrs[j],
                                                         rowPtr,
                                                         &sIsEqual );

                if ( ( aIsUnique == ID_TRUE ) && ( sIsEqual == ID_TRUE ) )
                {
                    sOIDPtr = aRightNode->mRowPtrs[j];

                    if( smnbBTree::compareRows( aIndexStat,
                                                aIndexHeader->columns,
                                                aIndexHeader->fence,
                                                sOIDPtr,
                                                aNull )
                        != 0 )
                    {
                        sViewNodeState = 1;

                        IDE_TEST( smnbBTree::isRowUnique(
                                                  aTrans,
                                                  aIndexStat,
                                                  SM_SCN_INIT,
                                                  sOIDPtr,
                                                  &sTID,
                                                  NULL, 
                                                  ID_FALSE ) /* aForbiddenToRetry */
                                  != IDE_SUCCESS );
                    }
                }
            }
        }while(sResult > 0);
        
        do
        {
            if ( i == ( aLeftNode->mSlotCount - 1 ) )
            {
                aLeftNode = aLeftNode->nextSPtr;
                i = 0;
            }
            else
            {
                i++;
            }

            if ( aLeftNode->mRowPtrs[i] == rowPtr )
            {
                /* 동일한 포인터라면 같은 값이기 때문에 비교할 필요없다. */
                sResult = 0;
            }
            else
            {
                sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                                         sColumns,
                                                         sFence,
                                                         aLeftNode->mRowPtrs[i],
                                                         rowPtr,
                                                         &sIsEqual );
            
                if ( ( aIsUnique == ID_TRUE ) && ( sIsEqual == ID_TRUE ) )
                {
                    sOIDPtr = aLeftNode->mRowPtrs[i];

                    if( smnbBTree::compareRows( aIndexStat,
                                                aIndexHeader->columns,
                                                aIndexHeader->fence,
                                                sOIDPtr,
                                                aNull )
                        != 0 )
                    {
                        sViewNodeState = 2;

                        IDE_TEST( smnbBTree::isRowUnique(
                                                  aTrans,
                                                  aIndexStat,
                                                  SM_SCN_INIT,
                                                  sOIDPtr,
                                                  &sTID,
                                                  NULL,
                                                  ID_FALSE ) /* aForbiddenToRetry */
                                  != IDE_SUCCESS );
                    }
                }
            }
        }while(sResult < 0);

        if( aRightNode->sequence > aLeftNode->sequence ||
            (aRightNode->sequence == aLeftNode->sequence && i < j))
        {
            /* PROJ-2433
             * 아래 코드가 좀복잡해보이지만, 간단하다.
             *
             * NODE aRightNode의 slot j와
             * NODE aLeftNode의 slot i의 자리를 바꾸는 코드이다.
             */

            sRow = NULL;
            sKey = NULL;

            smnbBTree::getLeafSlot( &sRow,
                                    &sKey,
                                    aRightNode,
                                    (SShort)j );

            if ( sKey != NULL )
            {
                idlOS::memcpy( sKeyBuf,
                               sKey,
                               aRightNode->mKeySize );
                
                sKey = sKeyBuf;
            }
            else
            {
                /* nothing to do */
            }

            smnbBTree::copyLeafSlots( aRightNode,
                                      (SShort)j,
                                      aLeftNode,
                                      (SShort)i,
                                      (SShort)i );
            
            smnbBTree::setLeafSlot( aLeftNode,
                                    (SShort)i,
                                    sRow,
                                    sKey );
        }
        else
        {
            break;
        }
    }

    *aOutNode = aRightNode;
    *aOutPos  = j;

    if ( sState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( sKeyBuf ) == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-42169 서버 재시작으로 인한 index rebuild 과정에서 uniqueness가 깨질경우
     * 디버그를 위한 추가 정보를 출력해야 한다. */
    if ( ( ideGetErrorCode() == smERR_ABORT_smnUniqueViolation ) &&
         ( smiGetStartupPhase() != SMI_STARTUP_SERVICE ) )
    {    
        /* sm trc log에는 header 및 문제가 발생한 node의 dump 정보를 남긴다. */
        ideLog::log( IDE_SM_0, "Unique Violation Error Occurs In Restart Rebuild Index" );
        smnManager::logCommonHeader( aIndexHeader->mIndexHeader );

        switch ( sViewNodeState )
        {
            case 2:
                smnbBTree::logIndexNode( aIndexHeader->mIndexHeader, (smnbNode *)aLeftNode );
                break;
            case 1:
                smnbBTree::logIndexNode( aIndexHeader->mIndexHeader, (smnbNode *)aRightNode );
                break;
            case 0:
                IDE_ASSERT(0);
        }

        /* error trc log에는 문제가 발생한 인덱스의 id, name을 남긴다. */
        ideLog::log( IDE_ERR_0, "Unique Violation Error Occurs In Restart Rebuild Index\n"
                     "IndexOID    : %"ID_UINT32_FMT"\n"
                     "IndexName   : %s\n",
                     aIndexHeader->mIndexHeader->mId,
                     aIndexHeader->mIndexHeader->mName );
    } 

    if ( sState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( sKeyBuf ) == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC smnbPointerbaseBuild::makeTree( smnbHeader  * aIndexHeader,
                                       smnbLNode   * aFstLNode,
                                       smnbNode   ** sRootNode)
{
    smnbLNode * sCurLNode;
    smnbINode * sCurParentINode;
    smnbINode * sFstParentINode;
    smnbINode * sNewINode = NULL;
    smnbINode * sCurINode = NULL;
    SChar     * sRow = NULL;
    void      * sKey = NULL;
    SInt       i;
    
    sCurLNode = aFstLNode;

    if( sCurLNode->nextSPtr == NULL )
    {
        *sRootNode = (smnbNode*)aFstLNode;
        return IDE_SUCCESS;
    }
    
    while(1)
    {
        IDE_TEST(aIndexHeader->mNodePool.allocateSlots(1,
                                                    (smmSlot**)&(sNewINode))
                 != IDE_SUCCESS);
        
        smnbBTree::initInternalNode( sNewINode,
                                     aIndexHeader,
                                     IDU_LATCH_UNLOCKED );

        // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
        aIndexHeader->nodeCount++;
        
        sFstParentINode = sNewINode;
        sCurINode       = sFstParentINode;

        while(1)
        {
            for ( i = 0 ;
                  ( i < SMNB_INTERNAL_SLOT_MAX_COUNT( aIndexHeader ) ) && ( sCurLNode != NULL ) ;
                  i++ )
            {
                /* PROJ-2433 */
                sRow = NULL;
                sKey = NULL;

                smnbBTree::getLeafSlot( &sRow,
                                        &sKey,
                                        sCurLNode,
                                        (SShort)( sCurLNode->mSlotCount - 1 ) );

                if ( sCurLNode->nextSPtr != NULL )
                {
                    IDE_ERROR( sRow != NULL );
                }
                else
                {
                    /* nothing to do */
                }

                smnbBTree::setInternalSlot( sCurINode,
                                            (SShort)i,
                                            (smnbNode *)sCurLNode,
                                            sRow,
                                            sKey );

                sCurLNode = sCurLNode->nextSPtr;
            }

            IDE_ERROR( i != 0 );
            
            sCurINode->mSlotCount = i;
            
            if( sCurLNode == NULL )
            {
                break;
            }
            
            IDE_TEST(aIndexHeader->mNodePool.allocateSlots(1,
                                                        (smmSlot**)&(sNewINode))
                     != IDE_SUCCESS);

            smnbBTree::initInternalNode( sNewINode,
                                         aIndexHeader,
                                         IDU_LATCH_UNLOCKED );

            // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
            aIndexHeader->nodeCount++;

            sCurINode->nextSPtr = sNewINode;
            sNewINode->prevSPtr = sCurINode;

            sCurINode = sNewINode;
        }

        if( sCurINode != sFstParentINode )
        {
            sCurINode = sFstParentINode;

            IDE_TEST( aIndexHeader->mNodePool.allocateSlots(1,
                                                         (smmSlot**)&(sFstParentINode))
                     != IDE_SUCCESS);
            
            smnbBTree::initInternalNode( sFstParentINode,
                                         aIndexHeader,
                                         IDU_LATCH_UNLOCKED );
            
            // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
            aIndexHeader->nodeCount++;

            sCurParentINode = sFstParentINode;
            
            while(1)
            {
                for ( i = 0 ;
                      ( i < SMNB_INTERNAL_SLOT_MAX_COUNT( aIndexHeader ) ) && ( sCurINode != NULL ) ;
                      i++ )
                {
                    /* PROJ-2433 */
                    sRow = NULL;
                    sKey = NULL;

                    smnbBTree::getInternalSlot( NULL,
                                               &sRow,
                                               &sKey,
                                               sCurINode,
                                               (SShort)( sCurINode->mSlotCount - 1 ) );

                    smnbBTree::setInternalSlot( sCurParentINode,
                                                (SShort)i,
                                                (smnbNode *)sCurINode,
                                                sRow,
                                                sKey );

                    sCurINode = sCurINode->nextSPtr;
                }
                
                IDE_ERROR( i != 0 );
                
                sCurParentINode->mSlotCount = i;
                
                if ( ( i != SMNB_INTERNAL_SLOT_MAX_COUNT( aIndexHeader ) ) ||
                     ( sCurINode == NULL ) )
                {
                    if(sFstParentINode == sCurParentINode)
                    {
                        break;
                    }

                    sCurINode = sFstParentINode;
                    
                    IDE_TEST( aIndexHeader->mNodePool.allocateSlots(1,
                                                                 (smmSlot**)&(sFstParentINode))
                             != IDE_SUCCESS);

                    smnbBTree::initInternalNode( sFstParentINode,
                                                 aIndexHeader,
                                                 IDU_LATCH_UNLOCKED );
                    
                    // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
                    aIndexHeader->nodeCount++;

                    sCurParentINode = sFstParentINode;
                }
                else
                {
                    IDE_TEST( aIndexHeader->mNodePool.allocateSlots(1,
                                                                 (smmSlot**)&(sNewINode))
                             != IDE_SUCCESS);
                    
                    smnbBTree::initInternalNode( sNewINode,
                                                 aIndexHeader,
                                                 IDU_LATCH_UNLOCKED );
                    
                    // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
                    aIndexHeader->nodeCount++;

                    sCurParentINode->nextSPtr = sNewINode;
                    sNewINode->prevSPtr = sCurParentINode;
                    
                    sCurParentINode = sNewINode;
                }
            }
        }
        
        break;
    }
    
    sCurINode = sFstParentINode;

    while ( SMNB_IS_INTERNAL_NODE( sCurINode ) )
    {
        /* PROJ-2433
         * child pointer는 유지 */
        smnbBTree::setInternalSlot( sCurINode,
                                    (SShort)( sCurINode->mSlotCount - 1 ),
                                    sCurINode->mChildPtrs[sCurINode->mSlotCount - 1],
                                    NULL,
                                    NULL );

        sCurINode = (smnbINode*)(sCurINode->mChildPtrs[sCurINode->mSlotCount - 1]);
    }

    *sRootNode = (smnbNode*)sFstParentINode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbPointerbaseBuild::duplicate( smnIndexHeader * aSrcIndex,
                                        smnIndexHeader * aDestIndex)
{
    smnbHeader* sSrcIndexHeader;
    smnbHeader* sDestIndexHeader;
    smnbLNode*  sCurNode;
    smnbLNode*  sNewNode;

    sSrcIndexHeader  = (smnbHeader*)aSrcIndex->mHeader;
    sDestIndexHeader = (smnbHeader*)aDestIndex->mHeader;

    sDestIndexHeader->nodeCount = sSrcIndexHeader->nodeCount;
    
    sCurNode = sSrcIndexHeader->pFstNode;
    
    while(sCurNode != NULL)
    {
        IDE_TEST(sDestIndexHeader->mNodePool.allocateSlots(1,
                                                          (smmSlot**)&(sNewNode))
                 != IDE_SUCCESS);

        idlOS::memcpy(sNewNode, sCurNode, smnbBTree::mNodeSize);
        
        if(sDestIndexHeader->pFstNode == NULL)
        {
            sDestIndexHeader->pFstNode = sNewNode;
            sDestIndexHeader->pLstNode = sNewNode;
        }
        else
        {
            sDestIndexHeader->pLstNode->nextSPtr = sNewNode;
            sNewNode->prevSPtr = sDestIndexHeader->pLstNode;
            
            sDestIndexHeader->pLstNode = sNewNode;
        }

        sCurNode = sCurNode->nextSPtr;
    }//while

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbPointerbaseBuild::getPivot             *
 * ------------------------------------------------------------------*
 * BUG-46955
 *
 * QUICK SORT 에서 사용할 PIVOT 을 구해서 리턴한다.
 * 최악을 피하기 위해, 3개의 값(lo, mid, hi) 중 중간값을 선택한다.
 *
 *********************************************************************/
SChar * smnbPointerbaseBuild::getPivot( smnbHeader      * aIndexHeader,
                                        smnbStatistic   * aIndexStat,
                                        smnbLNode      ** aArrNode,
                                        smnbLNode       * aLeftNode,
                                        UInt              aLeftPos,
                                        smnbLNode       * aRightNode,
                                        UInt              aRightPos )
{
    const smnbColumn * sColumns = aIndexHeader->columns;
    const smnbColumn * sFence   = aIndexHeader->fence;
    SChar            * sLo      = NULL;
    SChar            * sMid     = NULL;
    SChar            * sHi      = NULL;
    SChar            * sTmp     = NULL;
    SInt               sResult  = 0;
    idBool             sDummy   = ID_FALSE;


    if ( ( aLeftNode->sequence + 8 ) <= aRightNode->sequence ) 
    {
        /** NODE 갯수 : 9, 10, 11, ~ **/

        sHi = getPivot4Large( aIndexHeader,
                              aIndexStat,
                              aArrNode,
                              aLeftNode,
                              aLeftPos,
                              aRightNode,
                              aRightPos );
        IDE_CONT( PIVOT_RETURN );
    }
    else if ( ( aLeftNode->sequence + 2 ) <= aRightNode->sequence )
    {
        /** NODE 갯수 : 3, 4, 5, 6, 7, 8 **/

        sLo  = aLeftNode->mRowPtrs[aLeftPos];
        sMid = (aArrNode[(aLeftNode->sequence + aRightNode->sequence) / 2])->mRowPtrs[0];
        sHi  = aRightNode->mRowPtrs[aRightPos];
    }
    else if ( ( aLeftNode->sequence + 1 ) == aRightNode->sequence ) 
    {
        /** NODE 갯수 : 2개 **/

        if ( ( aLeftNode->mSlotCount - aLeftPos ) + ( aRightPos + 1 ) <= 2 )
        {
            /* 인덱스 키가 2개 이하면 첫번째 인덱스키를 pivot으로 선택 */
            sHi = aLeftNode->mRowPtrs[aLeftPos];
            IDE_CONT( PIVOT_RETURN );
        }
        else
        {
            sLo  = aLeftNode->mRowPtrs[aLeftPos];
            sMid = ( ( aLeftNode->mSlotCount - aLeftPos ) >= 2 ) ?
                   aLeftNode->mRowPtrs[aLeftNode->mSlotCount - 1] : aRightNode->mRowPtrs[0];
            sHi  = aRightNode->mRowPtrs[aRightPos];
        }
    }
    else 
    {
        /** NODE 갯수 : 1개 **/

        if ( ( aRightPos - aLeftPos + 1 ) <= 2 )
        {
            /* 인덱스 키가 2개 이하면 첫번째 인덱스키를 pivot으로 선택 */
            sHi = aLeftNode->mRowPtrs[aLeftPos];
            IDE_CONT( PIVOT_RETURN );
        }
        else
        {
            sLo  = aLeftNode->mRowPtrs[aLeftPos];
            sMid = aLeftNode->mRowPtrs[(aLeftPos + aRightPos) / 2];
            sHi  = aLeftNode->mRowPtrs[aRightPos];
        }
    }

    /* sLo, sMid, sHi 중에 중간값을 찾는다. */

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             sMid,
                                             sLo,
                                             &sDummy );
    /* ( sMid < sLo ) */
    if ( sResult < 0 )
    {
        sTmp = sMid;
        sMid = sLo;
        sLo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             sHi,
                                             sLo,
                                             &sDummy );
    /* ( sHi < sLo ) */
    if ( sResult < 0 )
    {
        sTmp = sHi;
        sHi  = sLo;
        sLo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             sMid,
                                             sHi,
                                             &sDummy );
    /* ( sMid < sHi ) */
    if ( sResult < 0 )
    {
        sTmp = sMid;
        sMid = sHi;
        sHi  = sTmp;
    }

    IDE_EXCEPTION_CONT( PIVOT_RETURN );

    return sHi;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbPointerbaseBuild::getPivot4Large       *
 * ------------------------------------------------------------------*
 * BUG-46955
 *
 * NODE 갯수가 9개이상인 경우
 * QUICK SORT 에서 사용할 PIVOT 을 구해서 리턴한다.
 *
 * 9개의 값 중 중간값을 선택한다.
 *   
 * pivot = median( median(s1Lo, s1Mid, s1Hi),
 *                 median(s2Lo, s2Mid, s2Hi),
 *                 median(s3Lo, s3Mid, s3Hi) )
 *
 *********************************************************************/
SChar * smnbPointerbaseBuild::getPivot4Large( smnbHeader      * aIndexHeader,
                                              smnbStatistic   * aIndexStat,
                                              smnbLNode      ** aArrNode,
                                              smnbLNode       * aLeftNode,
                                              UInt              aLeftPos,
                                              smnbLNode       * aRightNode,
                                              UInt              aRightPos )
{
    const smnbColumn * sColumns = aIndexHeader->columns;
    const smnbColumn * sFence   = aIndexHeader->fence;
    SChar            * s1Lo     = NULL;
    SChar            * s1Mid    = NULL;
    SChar            * s1Hi     = NULL;
    SChar            * s2Lo     = NULL;
    SChar            * s2Mid    = NULL;
    SChar            * s2Hi     = NULL;
    SChar            * s3Lo     = NULL;
    SChar            * s3Mid    = NULL;
    SChar            * s3Hi     = NULL;
    SChar            * sTmp     = NULL;
    SInt               sResult  = 0;
    idBool             sDummy   = ID_FALSE;
    UInt               sInc     = 0;

    sInc = ( aRightNode->sequence - aLeftNode->sequence ) / 8;

    s1Lo  = aLeftNode->mRowPtrs[aLeftPos];
    s1Mid = aArrNode[(aLeftNode->sequence + ( 1 * sInc ))]->mRowPtrs[0];
    s1Hi  = aArrNode[(aLeftNode->sequence + ( 2 * sInc ))]->mRowPtrs[0];

    s2Lo  = aArrNode[(aLeftNode->sequence + ( 3 * sInc ))]->mRowPtrs[0];
    s2Mid = aArrNode[(aLeftNode->sequence + ( 4 * sInc ))]->mRowPtrs[0];
    s2Hi  = aArrNode[(aLeftNode->sequence + ( 5 * sInc ))]->mRowPtrs[0];

    s3Lo  = aArrNode[(aLeftNode->sequence + ( 6 * sInc ))]->mRowPtrs[0];
    s3Mid = aArrNode[(aLeftNode->sequence + ( 7 * sInc ))]->mRowPtrs[0];
    s3Hi  = aRightNode->mRowPtrs[aRightPos];

    /************************************
     * s1Hi = median(s1Lo, s1Mid, s1Hi)
     ************************************/
    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s1Mid,
                                             s1Lo,
                                             &sDummy );
    /* ( s1Mid < s1Lo ) */
    if ( sResult < 0 )
    {
        sTmp  = s1Mid;
        s1Mid = s1Lo;
        s1Lo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s1Hi,
                                             s1Lo,
                                             &sDummy );
    /* ( s1Hi < s1Lo ) */
    if ( sResult < 0 )
    {
        sTmp  = s1Hi;
        s1Hi  = s1Lo;
        s1Lo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s1Mid,
                                             s1Hi,
                                             &sDummy );
    /* ( s1Mid < s1Hi ) */
    if ( sResult < 0 )
    {
        sTmp  = s1Mid;
        s1Mid = s1Hi;
        s1Hi  = sTmp;
    }

    /************************************
     * s2Hi = median(s2Lo, s2Mid, s2Hi)
     ************************************/
    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s2Mid,
                                             s2Lo,
                                             &sDummy );
    /* ( s2Mid < s2Lo ) */
    if ( sResult < 0 )
    {
        sTmp  = s2Mid;
        s2Mid = s2Lo;
        s2Lo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s2Hi,
                                             s2Lo,
                                             &sDummy );
    /* ( s2Hi < s2Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s2Hi;
        s2Hi  = s2Lo;
        s2Lo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s2Mid,
                                             s2Hi,
                                             &sDummy );
    /* ( s2Mid < s2Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s2Mid;
        s2Mid = s2Hi;
        s2Hi  = sTmp;
    }

    /************************************
     * s3Hi = median(s3Lo, s3Mid, s3Hi)
     ************************************/
    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s3Mid,
                                             s3Lo,
                                             &sDummy );
    /* ( s3Mid < s3Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s3Mid;
        s3Mid = s3Lo;
        s3Lo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s3Hi,
                                             s3Lo,
                                             &sDummy );
    /* ( s3Hi < s3Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s3Hi;
        s3Hi  = s3Lo;
        s3Lo  = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s3Mid,
                                             s3Hi,
                                             &sDummy );
    /* ( s3Mid < s3Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s3Mid;
        s3Mid = s3Hi;
        s3Hi  = sTmp;
    }

    /************************************
     * s3Hi = median(s1Hi, s2Hi, s3Hi)
     ************************************/
    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s2Hi,
                                             s1Hi,
                                             &sDummy );
    /* ( s2Hi < s1Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s2Hi;
        s2Hi = s1Hi;
        s1Hi = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s3Hi,
                                             s1Hi,
                                             &sDummy );
    /* ( s3Hi < s1Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s3Hi;
        s3Hi = s1Hi;
        s1Hi = sTmp;
    }

    sResult = smnbBTree::compareRowsAndPtr2( aIndexStat,
                                             sColumns,
                                             sFence,
                                             s2Hi,
                                             s3Hi,
                                             &sDummy );
    /* ( s2Hi < s3Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s2Hi;
        s2Hi = s3Hi;
        s3Hi = sTmp;
    }

    return s3Hi;
}
