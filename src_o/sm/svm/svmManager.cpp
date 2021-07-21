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
* $Id: svmManager.cpp 37235 2009-12-11 01:56:06Z elcarim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smiFixedTable.h>
#include <sctTableSpaceMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smu.h>
#include <smuUtility.h>
#include <svmReq.h>
#include <svmDef.h>
#include <svmDatabase.h>
#include <svmFPLManager.h>
#include <svmExpandChunk.h>
#include <svmManager.h>
#include <svrLogMgr.h>
#include <svpVarPageList.h>
#include <smiMain.h>


svmManager::svmManager()
{
}

/************************************************************************
 * Description : svmManager를 초기화 한다.
 *
 *   svm 모듈 중에 초기화되어야 하는 모듈들은 모두 여기서 초기화된다.
 ************************************************************************/
IDE_RC svmManager::initializeStatic()
{
    /* Volatile log manager를 초기화한다. */
    /* volatile log를 위한 로그 버퍼 메모리를 일부 할당받는다. */
    IDE_TEST( svrLogMgr::initializeStatic() != IDE_SUCCESS );

    IDE_TEST( svmFPLManager::initializeStatic() != IDE_SUCCESS );

    svpVarPageList::initAllocArray();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/************************************************************************
 * Description : svm 모듈이 사용했던 자원을 해제한다.
 ************************************************************************/
IDE_RC svmManager::destroyStatic()
{
    IDE_TEST( svrLogMgr::destroyStatic() != IDE_SUCCESS );

    IDE_TEST( svmFPLManager::destroyStatic() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Tablespace Node를 할당하고 초기화한다.
 *
 *  [IN] aTBSNode    - 초기화할 Tablespace Node
 *  [IN] aTBSAttr    - log anchor로부터 읽은 TBSAttr
 ************************************************************************/
IDE_RC svmManager::allocTBSNode(svmTBSNode        **aTBSNode,
                                smiTableSpaceAttr  *aTBSAttr)
{
    IDE_DASSERT( aTBSAttr != NULL );

    /* svmManager_allocTBSNode_calloc_TBSNode.tc */
    IDU_FIT_POINT("svmManager::allocTBSNode::calloc::TBSNode");
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SVM,
                               1,
                               ID_SIZEOF(svmTBSNode),
                               (void**)aTBSNode)
                 != IDE_SUCCESS);

    // Memory, Volatile, Disk TBS 공통 초기화
    IDE_TEST(sctTableSpaceMgr::initializeTBSNode(&((*aTBSNode)->mHeader),
                                                 aTBSAttr)
             != IDE_SUCCESS);

    // TBSNode의 TBSAttr에 값 복사
    idlOS::memcpy(&((*aTBSNode)->mTBSAttr),
                  aTBSAttr,
                  ID_SIZEOF(smiTableSpaceAttr));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Tablespace Node를 파괴한다.
 *
 *   aTBSNode [IN] 해제할 svmTBSNode
 ************************************************************************/
IDE_RC svmManager::destroyTBSNode(svmTBSNode *aTBSNode)
{
    IDE_DASSERT(aTBSNode != NULL );

    // Lock정보를 포함한 TBSNode의 모든 정보를 파괴
    IDE_TEST(sctTableSpaceMgr::destroyTBSNode(&aTBSNode->mHeader)
             != IDE_SUCCESS);

    IDE_TEST( iduMemMgr::free( aTBSNode ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description :
 *    Volatile Tablespace를 초기화한다.
 *    aTBSNode는 initTBSNode()를 통해 초기화되어 있어야 한다.
 *
 ************************************************************************/
IDE_RC svmManager::initTBS(svmTBSNode *aTBSNode)
{
    if ( SMI_TBS_IS_DROPPED(aTBSNode->mHeader.mState) )
    {
        // Drop된 TBS는 자원을 초기화할 필요가 없다.
    }
    else
    {
        aTBSNode->mDBMaxPageCount =
            calculateDbPageCount(smuProperty::getVolMaxDBSize(),
                                 smuProperty::getExpandChunkPageCount());

        aTBSNode->mAnchorOffset = SCT_UNSAVED_ATTRIBUTE_OFFSET;

        // Free Page List관리자 초기화
        IDE_TEST( svmFPLManager::initialize( aTBSNode ) != IDE_SUCCESS );

        // Tablespace확장 ChunK관리자 초기화
        IDE_TEST( svmExpandChunk::initialize( aTBSNode ) != IDE_SUCCESS );

        // PCH Array초기화
        // PCH Array는 실제 alloc될 page 개수보다 SVM_TBS_FIRST_PAGE_ID개 더 많아야 한다.
        // mPCHArray[TBS_ID][0]은 사용되지 않는 PCH이다.
        // 즉 m_page가 alloc되지 않는다.
        /* svmManager_initTBS_calloc_PCHArray.tc */
        IDU_FIT_POINT("svmManager::initTBS::calloc::PCHArray");
        IDE_TEST( smmManager::allocPCHArray( aTBSNode->mHeader.mID,
                                             aTBSNode->mDBMaxPageCount +
                                             SVM_TBS_FIRST_PAGE_ID ) != IDE_SUCCESS );

        /*
         * BUG-24292 create/drop volatile tablespace 수행 반복시 메모리 증가가 꾸준히 발생!!
         *
         * Database 확장은 ExpandGlobal Mutex를 잡고 하기 때문에
         * 병렬로 수행되는 경우가 없다. 때문에 Free Page List를 1로 하면된다.
         */
        
        // BUG-47487: FLI 페이지 MemPool 초기화 ( Volatile ) 
        IDE_TEST(aTBSNode->mFLIMemPagePool.initialize(
                    IDU_MEM_SM_SVM,
                    (SChar*)"TEMP_MEMORY_POOL",
                    1,
                    SM_PAGE_SIZE,
                    smuProperty::getTempPageChunkCount(),
                    IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                    ID_TRUE,							/* UseMutex */
                    IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                    ID_FALSE,							/* ForcePooling */
                    ID_TRUE,							/* GarbageCollection */
                    ID_TRUE,                           /* HWCacheLine */
                    IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
                != IDE_SUCCESS);			

        // MemPagePool 초기화
        IDE_TEST(aTBSNode->mMemPagePool.initialize(
                     IDU_MEM_SM_SVM,
                     (SChar*)"TEMP_MEMORY_POOL",
                     1,
                     SM_PAGE_SIZE,
                     smuProperty::getTempPageChunkCount(),
                     IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                     ID_TRUE,							/* UseMutex */
                     IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                     ID_FALSE,							/* ForcePooling */
                     ID_TRUE,							/* GarbageCollection */
                     ID_TRUE,                           /* HWCacheLine */
                     IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
                 != IDE_SUCCESS);			

        // PCH Memory Pool초기화
        IDE_TEST(aTBSNode->mPCHMemPool.initialize(
                     IDU_MEM_SM_SVM,
                     (SChar*)"PCH_MEM_POOL",
                     1,    // 다중화 하지 않는다.
                     ID_SIZEOF(svmPCH),
                     1024, // 한번에 1024개의 PCH저장할 수 있는 크기로 메모리를 확장한다.
                     IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                     ID_TRUE,							/* UseMutex */
                     IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                     ID_FALSE,							/* ForcePooling */
                     ID_TRUE,							/* GarbageCollection */
                     ID_TRUE,                           /* HWCacheLine */
                     IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
                != IDE_SUCCESS);			
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Tablespace의 자원들을 해제한다.
 *               Drop TBS시 호출된다.
 ************************************************************************/
IDE_RC svmManager::finiTBS( svmTBSNode *aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    /* BUG-39806 Valgrind Warning
     * - svmTBSDrop::dropTableSpacePending() 의 처리를 위해서, DROPPED 검사를 함
     *   수 외부에서 합니다.
     */

    // Free Page List 관리자 해제
    IDE_TEST( svmFPLManager::destroy( aTBSNode ) != IDE_SUCCESS );

    // Expand Chunk 관리자 해제
    IDE_TEST( svmExpandChunk::destroy( aTBSNode ) != IDE_SUCCESS );

    // BUGBUG-1548 일반메모리일때도 해제 안해도 상관없다.
    //             페이지 메모리 Pool자체를 destroy하기 때문
    IDE_TEST( freeAll( aTBSNode ) != IDE_SUCCESS );

    IDE_TEST( aTBSNode->mPCHMemPool.destroy() != IDE_SUCCESS );

    // BUG-47487: FLI 페이지 memPool 파괴 ( Volatile ) 
    IDE_TEST( aTBSNode->mFLIMemPagePool.destroy() != IDE_SUCCESS );

    // Page Pool 관리자를 파괴
    IDE_TEST( aTBSNode->mMemPagePool.destroy() != IDE_SUCCESS );

    IDE_TEST( smmManager::freePCHArray( aTBSNode->mHeader.mID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 사용자가 생성하려는 데이터베이스 크기에 근접하는
 * 데이터베이스 생성을 위해 생성할 Page 수를 계산한다.
 *
 * 사용자가 지정한 크기와 정확히 일치하는 데이터베이스를 생성할 수 없는
 * 이유는, 하나의 데이터베이스는 항상 Expand Chunk크기의 배수로
 * 생성되기 때문이다.
 *
 * aDbSize         [IN] 생성하려는 데이터베이스 크기
 * aChunkPageCount [IN] 하나의 Expand Chunk가 지니는 Page의 수
 *
 */
ULong svmManager::calculateDbPageCount( ULong aDbSize, ULong aChunkPageCount )
{
    vULong sCalculPageCount;
    vULong sRequestedPageCount;

    IDE_DASSERT( aDbSize > 0 );
    IDE_DASSERT( aChunkPageCount > 0  );

    sRequestedPageCount = aDbSize  / SM_PAGE_SIZE;

    // Expand Chunk Page수의 배수가 되도록 설정.
    // BUG-15288 단 Max DB SIZE를 넘을 수 없다.
    sCalculPageCount =
        aChunkPageCount * (sRequestedPageCount / aChunkPageCount);

    return sCalculPageCount ;
}

/*
   Tablespace의 Meta Page를 초기화하고 Free Page들을 생성한다.

   Chunk확장에 대한 로깅을 실시한다.
   
   aCreatePageCount [IN] 생성할 데이터베이스가 가질 Page의 수
                         Membase가 기록되는 데이터베이스
                         Meta Page 수도 포함한다.
                         이 값은 smiMain::smiCalculateDBSize()를 통해서 
                         구해진 값이어야 하고 smiGetMaxDBPageCount()보다
                         작은 값임을 확인한 후의 값이어야 한다.
   aDbFilePageCount [IN] 하나의 데이터베이스 파일이 가질 Page의 수
   aChunkPageCount  [IN] 하나의 Expand Chunk가 가질 Page의 수


   [ 알고리즘 ]
   - (010) PCH(Page Control Header) Memory 관리자(Pool)를 초기화
   - (020) 0번 Meta Page (Membase 존재)를 초기화
   - (030) Expand Chunk관리자 초기화
   - (040) 초기 Tablespace크기만큼 Tablespace 확장(Expand Chunk 할당)
 */

IDE_RC svmManager::createTBSPages(
    svmTBSNode      * aTBSNode,
    SChar           * aDBName,
    scPageID          aCreatePageCount)
{
    UInt         sState = 0;
    scPageID     sTotalPageCount;
    
    vULong       sNewChunks;
    scPageID     sChunkPageCount =
                     (scPageID)smuProperty::getExpandChunkPageCount();

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aCreatePageCount > 0 );
    
    ////////////////////////////////////////////////////////////////
    // aTBSNode의 membase 초기화
    IDE_TEST( svmDatabase::initializeMembase(
                  aTBSNode,
                  aDBName,
                  sChunkPageCount )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (020) Expand Chunk관리자 초기화
    IDE_TEST( svmExpandChunk::setChunkPageCnt( aTBSNode,
                                               sChunkPageCount )
              != IDE_SUCCESS );

    // Expand Chunk와 관련된 Property 값 체크
    IDE_TEST( svmDatabase::checkExpandChunkProps(&aTBSNode->mMemBase)
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////////
    // (030) 초기 Tablespace크기만큼 Tablespace 확장(Expand Chunk 할당)
    // 생성할 데이터베이스 Page수를 토대로 새로 할당할 Expand Chunk 의 수를 결정
    sNewChunks = svmExpandChunk::getExpandChunkCount(
                     aTBSNode,
                     aCreatePageCount );


    // To Fix BUG-17381     create tablespace가
    //                      VOLATILE_MAX_DB_SIZE이상으로 됩니다.
    
    // 시스템에서 오직 하나의 Tablespace만이
    // Chunk확장을 하도록 하는 Mutex
    // => 두 개의 Tablespace가 동시에 Chunk확장하는 상황에서는
    //    모든 Tablespace의 할당한 Page 크기가 VOLATILE_MAX_DB_SIZE보다
    //    작은 지 검사할 수 없기 때문
    
    IDE_TEST( svmFPLManager::lockGlobalAllocChunkMutex()
              != IDE_SUCCESS );
    sState = 1;
        
    IDE_TEST( svmFPLManager::getTotalPageCount4AllTBS( & sTotalPageCount )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( ( sTotalPageCount +
                      ( sNewChunks * sChunkPageCount ) ) >
                    ( smuProperty::getVolMaxDBSize() / SM_PAGE_SIZE ),
                    error_unable_to_create_cuz_vol_max_db_size );
    
    // 트랜잭션을 NULL로 넘겨서 로깅을 하지 않도록 한다.
    // 최대 Page수를 넘어서는지 이 속에서 체크한다.
    IDE_TEST( allocNewExpandChunks( aTBSNode,
                                    sNewChunks )
              != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( svmFPLManager::unlockGlobalAllocChunkMutex()
              != IDE_SUCCESS );
    
        
    IDE_ASSERT( aTBSNode->mMemBase.mAllocPersPageCount <=
                aTBSNode->mDBMaxPageCount );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( error_unable_to_create_cuz_vol_max_db_size );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_UNABLE_TO_CREATE_CUZ_VOL_MAX_DB_SIZE,
                     aTBSNode->mHeader.mName,
                     (ULong) ( ( aCreatePageCount * SM_PAGE_SIZE) / 1024),
                     (ULong) (smuProperty::getVolMaxDBSize()/1024),
                     (ULong) ( (sTotalPageCount * SM_PAGE_SIZE ) / 1024 )
                ));
    }
    // fix BUG-29682 IDE_EXCEPTION_END가 잘못되어 무한루프가 있습니다.
    IDE_EXCEPTION_END;
    IDE_PUSH();
    {
        switch( sState )
        {
            case 1:
                
                IDE_ASSERT( svmFPLManager::unlockGlobalAllocChunkMutex()
                            == IDE_SUCCESS );
                break;
            
            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/* 여러개의 Expand Chunk를 추가하여 데이터베이스를 확장한다.
 * aExpandChunkCount [IN] 확장하고자 하는 Expand Chunk의 수
 */
IDE_RC svmManager::allocNewExpandChunks( svmTBSNode *  aTBSNode,
                                         UInt          aExpandChunkCount )
{
    UInt i;
    scPageID sChunkFirstPID;
    scPageID sChunkLastPID;

    // 이 함수는 Normal Processing때에만 불리우므로
    // Expand Chunk확장 후 데이터베이스 Page수가
    // 최대 Page갯수를 넘어서는지 체크한다.
    IDE_TEST_RAISE(aTBSNode->mMemBase.mAllocPersPageCount
                     + aExpandChunkCount * aTBSNode->mMemBase.mExpandChunkPageCnt
                   > aTBSNode->mDBMaxPageCount,
                   max_page_error);

    // Expand Chunk를 데이터베이스에 추가하여 데이터베이스를 확장한다.
    for (i=0; i<aExpandChunkCount; i++ )
    {
        // 새로 추가할 Chunk의 첫번째 Page ID를 계산한다.
        // 지금까지 할당한 모든 Chunk의 총 Page수가 새 Chunk의 첫번째 Page ID가 된다.
        sChunkFirstPID = aTBSNode->mMemBase.mCurrentExpandChunkCnt *
                         aTBSNode->mMemBase.mExpandChunkPageCnt +
                         SVM_TBS_FIRST_PAGE_ID;
        // 새로 추가할 Chunk의 마지막 Page ID를 계산한다.
        sChunkLastPID  = sChunkFirstPID +
                         aTBSNode->mMemBase.mExpandChunkPageCnt - 1;

        IDE_TEST( allocNewExpandChunk( aTBSNode,
                                       sChunkFirstPID,
                                       sChunkLastPID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(max_page_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TooManyPage,
                                (ULong)aTBSNode->mDBMaxPageCount ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 새로 할당된 Expand Chunk안에 속하는 Page들의 PCH Entry를 할당한다.
 * Chunk안의 Free List Info Page의 Page Memory도 할당한다.
 *
 * 주의! 1. Alloc Chunk는 Logical Redo대상이므로, Physical 로깅하지 않는다.
 *          Chunk내의 Free Page들에 대해서는 Page Memory를 할당하지 않음
 *
 * aNewChunkFirstPID [IN] Chunk안의 첫번째 Page
 * aNewChunkLastPID  [IN] Chunk안의 마지막 Page
 */
IDE_RC svmManager::fillPCHEntry4AllocChunk(svmTBSNode * aTBSNode,
                                           scPageID     aNewChunkFirstPID,
                                           scPageID     aNewChunkLastPID )
{
    UInt       sFLIPageCnt = 0 ;
    scSpaceID  sSpaceID;
    scPageID   sPID = 0 ;

    sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aNewChunkFirstPID )
                 == ID_TRUE );
    IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aNewChunkLastPID )
                 == ID_TRUE );
    
    for ( sPID = aNewChunkFirstPID ;
          sPID <= aNewChunkLastPID ;
          sPID ++ )
    {
        if ( smmManager::getPCHBase( sSpaceID, sPID ) == NULL )
        {
            // PCH Entry를 할당한다.
            IDE_TEST( allocPCHEntry( aTBSNode, sPID ) != IDE_SUCCESS );
        }

        // Free List Info Page의 Page 메모리를 할당한다.
        if ( sFLIPageCnt < svmExpandChunk::getChunkFLIPageCnt(aTBSNode) )
        {
            sFLIPageCnt ++ ;

            // Restart Recovery중에는 해당 Page메모리가 이미 할당되어
            // 있을 수 있다.
            //
            // allocAndLinkPageMemory 에서 이를 고려한다.
            // 자세한 내용은 allocPageMemory의 주석을 참고
            
            IDE_TEST( allocAndLinkPageMemory( aTBSNode,
                                              sPID,          // PID
                                              SM_NULL_PID,   // prev PID
                                              SM_NULL_PID )  // next PID
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 특정 페이지 범위만큼 데이터베이스를 확장한다.
 *
 * 모든 Free Page List에 대해 Latch가 잡히지 않은 채로 이 함수가 호출된다.
 *
 * aNewChunkFirstPID [IN] 확장할 데이터베이스 Expand Chunk의 첫번째 Page ID
 * aNewChunkFirstPID [IN] 확장할 데이터베이스 Expand Chunk의 마지막 Page ID
 */
IDE_RC svmManager::allocNewExpandChunk(svmTBSNode * aTBSNode,
                                       scPageID     aNewChunkFirstPID,
                                       scPageID     aNewChunkLastPID )
{
    UInt              sStage = 0;
    svmPageList       sArrFreeList[SVM_MAX_FPL_COUNT];

    IDE_DASSERT( aTBSNode != NULL );

    // Page ID는 SVM_TBS_FIRST_PAGE_ID부터 출발한 숫자이기 때문에
    // mDBMaxPageCount(page 개수)와 비교할 땐 SVM_TBS_FIRST_PAGE_ID만큼
    // 뺀 수와 비교해야 한다.
    IDE_DASSERT( aNewChunkFirstPID - SVM_TBS_FIRST_PAGE_ID
                 < aTBSNode->mDBMaxPageCount );
    IDE_DASSERT( aNewChunkLastPID - SVM_TBS_FIRST_PAGE_ID
                 < aTBSNode->mDBMaxPageCount );

    // 모든 Free Page List의 Latch획득
    IDE_TEST( svmFPLManager::lockAllFPLs(aTBSNode) != IDE_SUCCESS );
    sStage = 1;

    // 하나의 Expand Chunk에 속하는 Page들의 PCH Entry들을 구성한다.
    IDE_TEST( fillPCHEntry4AllocChunk( aTBSNode,
                                       aNewChunkFirstPID,
                                       aNewChunkLastPID )
              != IDE_SUCCESS );

    // Logical Redo 될 것이므로 Physical Update( Next Free Page ID 세팅)
    // 에 대한 로깅을 하지 않음.
    IDE_TEST( svmFPLManager::distributeFreePages(
                  aTBSNode,
                  // Chunk내의 첫번째 Free Page
                  // Chunk의 앞부분은 Free List Info Page들이 존재하므로,
                  // Free List Info Page만큼 건너뛰어야 Free Page가 나온다.
                  aNewChunkFirstPID + 
                  svmExpandChunk::getChunkFLIPageCnt(aTBSNode),
                  // Chunk내의 마지막 Free Page
                  aNewChunkLastPID,
                  ID_TRUE, // set next free page, PRJ-1548
                  sArrFreeList  )
              != IDE_SUCCESS );

    // 지금까지 데이터베이스에 할당된 총 페이지수 변경
    aTBSNode->mMemBase.mAllocPersPageCount = aNewChunkLastPID - SVM_TBS_FIRST_PAGE_ID + 1;
    aTBSNode->mMemBase.mCurrentExpandChunkCnt++;

    IDE_TEST( svmFPLManager::appendPageLists2FPLs( aTBSNode, sArrFreeList )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( svmFPLManager::unlockAllFPLs(aTBSNode) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sStage )
    {
        case 1 :
            IDE_ASSERT( svmFPLManager::unlockAllFPLs(aTBSNode) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 * Page들의 메모리를 할당한다.
 *
 * FLI Page에 Next Free Page ID로 링크된 페이지들에 대해
 * PCH안의 Page 메모리를 할당하고 Page Header의 Prev/Next포인터를 연결한다.
 *
 * Free List Info Page안의 Next Free Page ID를 기반으로
 * PCH의 Page들을 Page Header의 Prev/Next링크로 연결한다.
 *
 * Free List Info Page에 특별한 값을 설정하여 할당된 페이지임을 표시한다.
 *
 * Free Page가 새로 할당되기 전에 불리우는 루틴으로, 기존 Free Page들의
 * PCH및 Page 메모리를 할당한다.
 *
 * aHeadPID   [IN] 연결하고자 하는 첫번째 Free Page
 * aTailPID   [IN] 연결하고자 하는 마지막 Free Page
 * aPageCount [OUT] 연결된 총 페이지 수
 */
IDE_RC svmManager::allocFreePageMemoryList( svmTBSNode * aTBSNode,
                                            scPageID     aHeadPID,
                                            scPageID     aTailPID,
                                            vULong     * aPageCount )
{
    scPageID   sPrevPID = SM_NULL_PID;
    scPageID   sNextPID;
    scPageID   sPID;

    IDE_DASSERT( smmManager::isValidPageID( aTBSNode->mTBSAttr.mID, aHeadPID )
                 == ID_TRUE );
    IDE_DASSERT( smmManager::isValidPageID( aTBSNode->mTBSAttr.mID, aTailPID )
                 == ID_TRUE );
    IDE_DASSERT( aPageCount != NULL );

    vULong   sProcessedPageCnt = 0;

    // BUGBUG kmkim 각 Page에 Latch를 걸어야 하는지 고민 필요.
    // 살짝쿵 생각해본 결과로는 Latch잡을 필요 없음..

    // sHeadPage부터 sTailPage사이의 모든 Page에 대해
    sPID = aHeadPID;
    while ( sPID != SM_NULL_PID )
    {
        // Next Page ID 결정
        if ( sPID == aTailPID )
        {
            // 마지막으로 link될 Page라면 다음 Page는 NULL
            sNextPID = SM_NULL_PID ;
        }
        else
        {
            // 마지막이 아니라면 다음 Page Free Page ID를 얻어온다.
            IDE_TEST( svmExpandChunk::getNextFreePage( aTBSNode,
                                                       sPID,
                                                       & sNextPID )
                      != IDE_SUCCESS );
        }

        // Free Page이더라도 PCH는 할당되어 있어야 한다.
        IDE_ASSERT( smmManager::getPCHBase( aTBSNode->mTBSAttr.mID, sPID )  != NULL );

        // 페이지 메모리를 할당하고 초기화
        IDE_TEST( allocAndLinkPageMemory( aTBSNode,
                                          sPID,
                                          sPrevPID,
                                          sNextPID ) != IDE_SUCCESS );
        
        // 테이블에 할당되었다는 의미로
        // Page의 Next Free Page로 특별한 값을 기록해둔다.
        // 서버 기동시 Page가 테이블에 할당된 Page인지,
        // Free Page인지 여부를 결정하기 위해 사용된다.
        IDE_TEST( svmExpandChunk::setNextFreePage(
                      aTBSNode,
                      sPID,
                      SVM_FLI_ALLOCATED_PID )
                  != IDE_SUCCESS );

        sProcessedPageCnt ++ ;


        sPrevPID = sPID ;

        // sPID 가 aTailPID일 때,
        // 여기에서 sPID가 SM_NULL_PID 로 설정되어 loop 종료
        sPID = sNextPID ;
    }


    * aPageCount = sProcessedPageCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PCH의 Page속의 Page Header의 Prev/Next포인터를 기반으로
 * FLI Page에 Next Free Page ID를 설정한다.
 *
 * 테이블에 할당되었던 Page가 Free Page로 반납되기 전에 전에
 * 불리우는 루틴이다.
 *
 * aHeadPage  [IN] 연결하고자 하는 첫번째 Free Page
 * aTailPage  [IN] 연결하고자 하는 마지막 Free Page
 * aPageCount [OUT] 연결된 총 페이지 수
 */
IDE_RC svmManager::linkFreePageList( svmTBSNode * aTBSNode,
                                     void       * aHeadPage,
                                     void       * aTailPage,
                                     vULong     * aPageCount )
{
    scPageID    sPID, sTailPID, sNextPID;
    vULong      sProcessedPageCnt = 0;
    void      * sPagePtr;

    IDE_DASSERT( aHeadPage != NULL );
    IDE_DASSERT( aTailPage != NULL );
    IDE_DASSERT( aPageCount != NULL );

    sPID = smLayerCallback::getPersPageID( aHeadPage );
    sTailPID = smLayerCallback::getPersPageID( aTailPage );
    // sHeadPage부터 sTailPage사이의 모든 Page에 대해

    do
    {
        if ( sPID == sTailPID ) // 마지막 페이지인 경우
        {
            sNextPID = SM_NULL_PID ;
        }
        else  // 마지막 페이지가 아닌 경우
        {
            // Free List Info Page에 Next Free Page ID를 기록한다.
            IDE_ASSERT( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
                                                    sPID,
                                                    &sPagePtr )
                        == IDE_SUCCESS );
            sNextPID = smLayerCallback::getNextPersPageID( sPagePtr );

        }

        // Free List Info Page에 Next Free Page ID 세팅
        IDE_TEST( svmExpandChunk::setNextFreePage( aTBSNode,
                                                   sPID,
                                                   sNextPID )
                  != IDE_SUCCESS );

        sProcessedPageCnt ++ ;

        sPID = sNextPID ;
    } while ( sPID != SM_NULL_PID );

    * aPageCount = sProcessedPageCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* PCH의 Page속의 Page Header의 Prev/Next포인터를 기반으로
 * PCH 의 Page 메모리를 반납한다.
 *
 * 테이블에 할당되었던 Page가 Free Page로 반납된 후에
 * 불리우는 루틴으로, Page들의 PCH및 Page 메모리를 해제한다.
 *
 * aHeadPage  [IN] 연결하고자 하는 첫번째 Free Page
 * aTailPage  [IN] 연결하고자 하는 마지막 Free Page
 * aPageCount [OUT] 연결된 총 페이지 수
 */
IDE_RC svmManager::freeFreePageMemoryList( svmTBSNode * aTBSNode,
                                           void       * aHeadPage,
                                           void       * aTailPage,
                                           vULong     * aPageCount )
{
    scPageID    sPID, sTailPID, sNextPID;
    vULong      sProcessedPageCnt = 0;
    void      * sPagePtr;

    IDE_DASSERT( aHeadPage != NULL );
    IDE_DASSERT( aTailPage != NULL );
    IDE_DASSERT( aPageCount != NULL );

    sPID = smLayerCallback::getPersPageID( aHeadPage );
    sTailPID = smLayerCallback::getPersPageID( aTailPage );
    // sHeadPage부터 sTailPage사이의 모든 Page에 대해

    do
    {
        if ( sPID == sTailPID ) // 마지막 페이지인 경우
        {
            sNextPID = SM_NULL_PID ;
        }
        else  // 마지막 페이지가 아닌 경우
        {
            // Free List Info Page에 Next Free Page ID를 기록한다.
            IDE_ASSERT( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
                                                    sPID,
                                                    &sPagePtr )
                        == IDE_SUCCESS );
            sNextPID = smLayerCallback::getNextPersPageID( sPagePtr );
        }

        IDE_TEST( freePageMemory( aTBSNode, sPID ) != IDE_SUCCESS );

        sProcessedPageCnt ++ ;

        sPID = sNextPID ;
    } while ( sPID != SM_NULL_PID );

    * aPageCount = sProcessedPageCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


#if 0  //not used
/** DB로부터 하나의 Page를 할당받는다.
 *
 * aAllocatedPage  [OUT] 할당받은 페이지
 */
IDE_RC svmManager::allocatePersPage (void        *aTrans,
                                     scSpaceID    aSpaceID,
                                     void       **aAllocatedPage)
{
    IDE_ASSERT( aAllocatedPage != NULL);

    IDE_TEST( allocatePersPageList( aTrans,
                                    aSpaceID,
                                    1,
                                    aAllocatedPage,
                                    aAllocatedPage )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
#endif

/** DB로부터 Page를 여러개 할당받는다.
 *
 * 여러개의 Page를 동시에 할당받으면 DB Page할당 횟수를 줄일 수 있으며,
 * 이를 통해 DB Free Page List 로의 동시성을 향상시킬 수 있다.
 *
 * 여러 Page들을 서로 연결하기 위해 aHeadPage부터 aTailPage까지
 * Page Header의 Prev/Next포인터로 연결해준다.
 *
 * aPageCount [IN] 할당받을 페이지의 수
 * aHeadPage  [OUT] 할당받은 페이지 중 첫번째 페이지
 * aTailPage  [OUT] 할당받은 페이지 중 마지막 페이지
 */
IDE_RC svmManager::allocatePersPageList (void      *  aTrans,
                                         scSpaceID    aSpaceID,
                                         UInt         aPageCount,
                                         void     **  aHeadPage,
                                         void     **  aTailPage,
                                         UInt      *  aAllocPageCnt )
{

    scPageID  sHeadPID = SM_NULL_PID;
    scPageID  sTailPID = SM_NULL_PID;
    vULong    sLinkedPageCount;
    UInt      sPageListID;
    svmTBSNode * sTBSNode;
    UInt      sPageCount = 0;
    UInt      sTotalPageCount = 0;
    UInt      sState = 0;

    IDE_DASSERT( aPageCount != 0 );
    IDE_DASSERT( aHeadPage != NULL );
    IDE_DASSERT( aTailPage != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );

    // 트랜잭션의 RSGroupID를 이용하여 여러개의 Free Page List중 하나를 선택한다.
    smLayerCallback::allocRSGroupID( aTrans, &sPageListID );

    if( aPageCount != 1 )
    {
        /* BUG-46861 TABLE_ALLOC_PAGE_COUNT 프로퍼티를 사용해 다수의 페이지를 할당할 경우 다수의 페이지 할당으로
           할당된 페이지가 MEM_MAX_DB_SIZE를 넘어서지 않도록 주의해야 한다.
           또한, TBS lock을 잡지 않고 현재 사용중인 DB의 크기를 체크하기 때문에
           현재 사용중인 공간+새로 할당한 공간이 최대 사용 가능 공간에 도달하지 않아도
           그에 근접할 경우(1개 Chunk이하의 공간이 차이날 경우)에는 기존처럼 1개씩 할당한다. */
        IDE_TEST( svmFPLManager::lockGlobalAllocChunkMutex() != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( svmFPLManager::getTotalPageCount4AllTBS( & sTotalPageCount ) != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( svmFPLManager::unlockGlobalAllocChunkMutex() != IDE_SUCCESS );

        if( ( smiGetStartupPhase() == SMI_STARTUP_SERVICE ) &&
            ( ( sTotalPageCount + aPageCount ) < 
              ( ( smuProperty::getVolMaxDBSize() / SM_PAGE_SIZE ) - sTBSNode->mMemBase.mExpandChunkPageCnt ) ) )
        {
            sPageCount = aPageCount;
        }
        else
        {
            /* 할당한 공간+할당할 공간이 VOLATILE_MAX_DB_SIZE에 근접하거나 서버 시작 중에는 기존처럼 하나만 할당한다. */
            sPageCount = 1;
        }
    }
    else
    {
        sPageCount = 1;
    }

    // sPageListID에 해당하는 Free Page List에 최소한 aPageCount개의
    // Free Page가 존재함을 보장하면서 latch를 획득한다.
    //
    // aPageCount만큼 Free Page List에 있음을 보장하기 위해서
    //
    // 1. Free Page List간에 Free Page들을 이동시킬 수 있다. => Physical 로깅
    // 2. Expand Chunk를 할당할 수 있다.
    //     => Chunk할당을 Logical 로깅.
    //       -> Recovery시 svmManager::allocNewExpandChunk호출하여 Logical Redo
    IDE_TEST( svmFPLManager::lockListAndPreparePages( sTBSNode,
                                                      aTrans,
                                                      (svmFPLNo)sPageListID,
                                                      sPageCount )
              != IDE_SUCCESS );

    // 트랜잭션이 사용하는 Free Page List에서 Free Page들을 떼어낸다.
    // DB Free Page List에 대한 로깅이 여기에서 이루어진다.
    IDE_TEST( svmFPLManager::removeFreePagesFromList( sTBSNode,
                                                      aTrans,
                                                      (svmFPLNo)sPageListID,
                                                      sPageCount,
                                                      & sHeadPID,
                                                      & sTailPID )
              != IDE_SUCCESS );

    // Head부터 Tail까지 모든 Page에 대해
    // Page Header의 Prev/Next 링크를 서로 연결시킨다.
    IDE_TEST( allocFreePageMemoryList ( sTBSNode,
                                        sHeadPID,
                                        sTailPID,
                                        & sLinkedPageCount )
              != IDE_SUCCESS );

    IDE_ASSERT( sLinkedPageCount == sPageCount );

    IDE_ASSERT( smmManager::getPersPagePtr( sTBSNode->mTBSAttr.mID, 
                                            sHeadPID,
                                            aHeadPage )
                == IDE_SUCCESS );
    IDE_ASSERT( smmManager::getPersPagePtr( sTBSNode->mTBSAttr.mID, 
                                            sTailPID,
                                            aTailPage )
                == IDE_SUCCESS );


    // 페이지를 할당받은 Free Page List의 Latch를 풀어준다.
    IDE_TEST( svmFPLManager::unlockFreePageList( sTBSNode,
                                                 (svmFPLNo)sPageListID )
              != IDE_SUCCESS );

    *aAllocPageCnt = sPageCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1: 
            IDE_ASSERT( svmFPLManager::unlockGlobalAllocChunkMutex()
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

#if 0
/*
 * 하나의 Page를 데이터베이스로 반납한다
 *
 * aToBeFreePage [IN] 반납할 Page
 */
IDE_RC svmManager::freePersPage (void       * aTrans,
                                 scSpaceID    aSpaceID,
                                 void        *aToBeFreePage )
{
    IDE_DASSERT( aToBeFreePage != NULL );

    IDE_TEST( freePersPageList( aTrans,
                                aSpaceID,
                                aToBeFreePage,
                                aToBeFreePage )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

/*
 * 여러개의 Page를 한꺼번에 데이터베이스로 반납한다.
 *
 * aHeadPage부터 aTailPage까지
 * Page Header의 Prev/Next포인터로 연결되어 있어야 한다.
 *
 * 현재 Free Page수가 가장 작은 Free Page List 에 Page를 Free 한다.
 *
 * aTrans    [IN] Page를 반납하려는 트랜잭션
 * aHeadPage [IN] 반납할 첫번째 Page
 * aHeadPage [IN] 반납할 마지막 Page
 *
 */
IDE_RC svmManager::freePersPageList (void       * aTrans,
                                     scSpaceID    aSpaceID,
                                     void       * aHeadPage,
                                     void       * aTailPage)
{
    UInt       sPageListID;
    vULong     sLinkedPageCount    ;
    vULong     sFreePageCount    ;
    scPageID   sHeadPID, sTailPID ;
    UInt       sStage = 0;
    svmTBSNode * sTBSNode;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aHeadPage != NULL );
    IDE_DASSERT( aTailPage != NULL );

    IDE_TEST(sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                       (void**)&sTBSNode )
             != IDE_SUCCESS);

    sHeadPID = smLayerCallback::getPersPageID( aHeadPage );
    sTailPID = smLayerCallback::getPersPageID( aTailPage );

    // Free List Info Page안에 Free Page들의 Link를 기록한다.
    IDE_TEST( linkFreePageList( sTBSNode,
                                aHeadPage,
                                aTailPage,
                                & sLinkedPageCount )
              != IDE_SUCCESS );

    // Free Page를 반납한다.
    // 이렇게 해도 allocFreePage에서 Page가장 많은 Free Page List에서
    // 절반을 떼어오는 루틴때문에 Free Page List간의 밸런싱이 된다.

    // 트랜잭션의 RSGroupID를 이용하여 여러개의 Free Page List중 하나를 선택한다.
    smLayerCallback::allocRSGroupID( aTrans, &sPageListID );

    // Page를 반납할 Free Page List에 Latch획득
    IDE_TEST( svmFPLManager::lockFreePageList(sTBSNode, (svmFPLNo)sPageListID)
              != IDE_SUCCESS );
    sStage = 1;

    // Free Page List 에 Page를 반납한다.
    // DB Free Page List에 대한 로깅이 발생한다.
    IDE_TEST( svmFPLManager::appendFreePagesToList( sTBSNode,
                                                    aTrans,
                                                    sPageListID,
                                                    sLinkedPageCount,
                                                    sHeadPID,
                                                    sTailPID )
              != IDE_SUCCESS );

    IDE_TEST( freeFreePageMemoryList( sTBSNode,
                                      aHeadPage,
                                      aTailPage,
                                      & sFreePageCount )
              != IDE_SUCCESS );

    IDE_ASSERT( sFreePageCount == sLinkedPageCount );

    // 주의! Page 반납 연산 Logging을 하는
    // Page Free를 하기 때문에 Flush를 할 필요가 없다.

    sStage = 0;
    // Free Page List에서 Latch푼다
    IDE_TEST( svmFPLManager::unlockFreePageList( sTBSNode,
                                                 (svmFPLNo)sPageListID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sStage )
    {
        case 1:
            IDE_ASSERT( svmFPLManager::unlockFreePageList(
                            sTBSNode,
                            (svmFPLNo)sPageListID )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }
    sStage = 0;

    IDE_POP();

    return IDE_FAILURE;
}


/* --------------------------------------------------------------------------
 *  SECTION : Latch Control
 * -------------------------------------------------------------------------*/

/*
 * 특정 Page에 S래치를 획득한다. ( 현재는 X래치로 구현되어 있다 )
 */
IDE_RC
svmManager::holdPageSLatch(scSpaceID aSpaceID,
                           scPageID  aPageID )
{
    svmPCH    * sPCH ;

    IDE_DASSERT( smmManager::isValidPageID( aSpaceID, aPageID ) == ID_TRUE );

    sPCH = (svmPCH*)smmManager::getPCH( aSpaceID, aPageID );

    IDE_DASSERT( sPCH != NULL );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sPCH->mPageMemLatch.lockRead( NULL,/* idvSQL* */
                                  NULL ) /* sWeArgs*/
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 특정 Page에 X래치를 획득한다.
 */
IDE_RC
svmManager::holdPageXLatch(scSpaceID aSpaceID,
                           scPageID  aPageID)
{
    svmPCH    * sPCH;

    IDE_DASSERT( smmManager::isValidPageID( aSpaceID, aPageID ) == ID_TRUE );

    sPCH = (svmPCH*)smmManager::getPCH( aSpaceID, aPageID );

    IDE_DASSERT( sPCH != NULL );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sPCH->mPageMemLatch.lockWrite( NULL,/* idvSQL* */
                                   NULL ) /* sWeArgs*/
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * 특정 Page에서 래치를 풀어준다.
 */
IDE_RC
svmManager::releasePageLatch(scSpaceID aSpaceID,
                             scPageID  aPageID)
{
    svmPCH * sPCH;

    IDE_DASSERT( smmManager::isValidPageID( aSpaceID, aPageID ) == ID_TRUE );

    sPCH = (svmPCH*)smmManager::getPCH( aSpaceID, aPageID );

    IDE_DASSERT( sPCH );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sPCH->mPageMemLatch.unlock( ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* ------------------------------------------------
 *  alloc & free PCH + Page
 * ----------------------------------------------*/

IDE_RC svmManager::allocPCHEntry(svmTBSNode *  aTBSNode,
                                 scPageID      aPageID)
{

    SChar       sMutexName[128];
    svmPCH    * sCurPCH;
    smPCSlot * sPCHSlot;
    scSpaceID   sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_ASSERT(sSpaceID < SC_MAX_SPACE_COUNT);
    
    IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aPageID ) == ID_TRUE );

    sPCHSlot = smmManager::getPCHSlot( sSpaceID, aPageID );
    IDE_ASSERT( sPCHSlot->mPCH == NULL);

    /* svmManager_allocPCHEntry_alloc_CurPCH.tc */
    IDU_FIT_POINT("svmManager::allocPCHEntry::alloc::CurPCH");
    IDE_TEST( aTBSNode->mPCHMemPool.alloc((void **)&sCurPCH) != IDE_SUCCESS);
    sPCHSlot->mPCH = sCurPCH;

    /* ------------------------------------------------
     * [] mutex 초기화
     * ----------------------------------------------*/

    idlOS::snprintf( sMutexName,
                     128,
                     "SVMPCH_%"ID_UINT32_FMT"_%"ID_UINT32_FMT"_PAGE_MEMORY_LATCH",
                     (UInt) sSpaceID,
                     (UInt) aPageID );

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_ASSERT( sCurPCH->mPageMemLatch.initialize(  
                                      sMutexName,
                                      IDU_LATCH_TYPE_NATIVE )
                == IDE_SUCCESS );

    sPCHSlot->mPagePtr = NULL;
    sCurPCH->mNxtScanPID       = SM_NULL_PID;
    sCurPCH->mPrvScanPID       = SM_NULL_PID;
    sCurPCH->mModifySeqForScan = 0;
    sCurPCH->mFreePageHeader   = NULL ;
    
    // svmPCH.mFreePageHeader 초기화
    IDE_TEST( smLayerCallback::initializeFreePageHeader( sSpaceID, aPageID )
              != IDE_SUCCESS );
    IDE_ASSERT( sCurPCH->mFreePageHeader != NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/* PCH 해제
 *
 * aPID      [IN] PCH를 해제하고자 하는 Page ID
 * aPageFree [IN] PCH뿐만 아니라 그 안의 Page 메모리도 해제할 것인지 여부
 */

IDE_RC svmManager::freePCHEntry(svmTBSNode * aTBSNode,
                                scPageID     aPID)
{
    svmPCH    * sCurPCH;
    smPCSlot * sPCHSlot;
    scSpaceID   sSpaceID = aTBSNode->mTBSAttr.mID;

    IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aPID ) == ID_TRUE );

    sPCHSlot = smmManager::getPCHSlot( sSpaceID, aPID );
    sCurPCH = sPCHSlot->mPCH;

    IDE_ASSERT(sCurPCH != NULL);

    // svmPCH.mFreePageHeader 해제
    IDE_TEST( smLayerCallback::destroyFreePageHeader( sSpaceID, aPID )
              != IDE_SUCCESS );

    // Free Page라면 Page메모리가 이미 반납되어
    // 메모리를 Free하지 않아도 된다.
    // Page 메모리가 있는 경우에만 메모리를 반납한다.
    if ( sPCHSlot->mPagePtr != NULL )
    {
        IDE_TEST( freePageMemory( aTBSNode, aPID ) != IDE_SUCCESS );
    }

    /* BUG-31569 [sm-mem-page] When executing full scan, hold page X Latch
     * in MMDB */
    IDE_TEST( sCurPCH->mPageMemLatch.destroy() != IDE_SUCCESS);

    sCurPCH->mNxtScanPID       = SM_NULL_PID;
    sCurPCH->mPrvScanPID       = SM_NULL_PID;
    sCurPCH->mModifySeqForScan = 0;

    IDE_TEST( aTBSNode->mPCHMemPool.memfree(sCurPCH) != IDE_SUCCESS);

    sPCHSlot->mPCH = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 데이터베이스의 PCH, Page Memory를 모두 Free한다.
 */
IDE_RC svmManager::freeAll(svmTBSNode * aTBSNode)
{
    scPageID i;

    for (i = 0; i < aTBSNode->mDBMaxPageCount; i++)
    {
        svmPCH *sPCH = (svmPCH*)smmManager::getPCH(aTBSNode->mTBSAttr.mID, i);

        if (sPCH != NULL)
        {
            IDE_TEST( freePCHEntry(aTBSNode, i) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* 데이터베이스 타입에 따라 공유메모리나 일반 메모리를 페이지 메모리로 할당한다
 *
 * aPage [OUT] 할당된 Page 메모리
 */
IDE_RC svmManager::allocPage( svmTBSNode*   aTBSNode, 
                              svmTempPage** aPage,
                              idBool        aIsDataPage )
{
    IDE_DASSERT( aPage != NULL );

    /* svmManager_allocPage_alloc_Page.tc */
    IDU_FIT_POINT("svmManager::allocPage::alloc::Page");
    
    // BUG-47487: DATA / FLI 페이지 메모리 alloc 분리 ( Volatile ) 
    if ( aIsDataPage == ID_TRUE )
    {
        IDE_TEST( aTBSNode->mMemPagePool.alloc( (void **)aPage )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( aTBSNode->mFLIMemPagePool.alloc( (void **)aPage )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 데이터베이스타입에따라 Page메모리를 공유메모리나 일반메모리로 해제한다.
 *
 * aPage [IN] 해제할 Page 메모리
 */
IDE_RC svmManager::freePage( svmTBSNode*  aTBSNode, 
                             svmTempPage* aPage,
                             idBool       aIsDataPage )
{
    IDE_DASSERT( aPage != NULL );

    // BUG-47487: DATA / FLI 페이지 메모리 free 분리 ( Volatile )
    if ( aIsDataPage == ID_TRUE )
    {
        IDE_TEST( aTBSNode->mMemPagePool.memfree( aPage )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( aTBSNode->mFLIMemPagePool.memfree( aPage )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 특정 Page의 PCH안의 Page Memory를 할당한다.
 *
 * aPID [IN] Page Memory를 할당할 Page의 ID
 */
IDE_RC svmManager::allocPageMemory( svmTBSNode * aTBSNode, scPageID aPID )
{
    scSpaceID sSpaceID = aTBSNode->mTBSAttr.mID;
    smPCSlot * sPCHSlot;

    IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aPID ) == ID_TRUE );
   
    sPCHSlot = smmManager::getPCHSlot( sSpaceID, aPID );
    
    IDE_ASSERT( sPCHSlot->mPCH != NULL );

    // Page Memory가 할당되어 있지 않아야 한다.
    IDE_ASSERT( sPCHSlot->mPagePtr == NULL );

    if ( sPCHSlot->mPagePtr == NULL )
    {
        // BUG-47487: DATA / FLI 페이지 경우 나누기 alloc ( Volatile )
        if ( ( svmExpandChunk::isFLIPageID ( aTBSNode, aPID ) == ID_TRUE ) )
        {
            //FLI
            IDE_TEST( allocPage( aTBSNode, 
                                 (svmTempPage **) & sPCHSlot->mPagePtr,
                                 ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            //DATA
            IDE_TEST( allocPage( aTBSNode, 
                                 (svmTempPage **) & sPCHSlot->mPagePtr )
                      != IDE_SUCCESS );
        }   
    }

#ifdef DEBUG_SVM_FILL_GARBAGE_PAGE
    idlOS::memset( sPCH->m_page, 0x43, SM_PAGE_SIZE );
#endif // DEBUG

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * 페이지 메모리를 할당하고, 해당 Page를 초기화한다.
 * 필요한 경우, 페이지 초기화에 대한 로깅을 실시한다
 *
 * aPID     [IN] 페이지 메모리를 할당하고 초기화할 페이지 ID
 * aPrevPID [IN] 할당할 페이지의 이전 Page ID
 * aNextPID [IN] 할당할 페이지의 다음 Page ID
 */
IDE_RC svmManager::allocAndLinkPageMemory( svmTBSNode * aTBSNode,
                                           scPageID     aPID,
                                           scPageID     aPrevPID,
                                           scPageID     aNextPID )
{
    scSpaceID  sSpaceID = aTBSNode->mTBSAttr.mID;
    smPCSlot * sPCHSlot;
    IDE_DASSERT( aPID != SM_NULL_PID );
    IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aPID ) == ID_TRUE );

#ifdef DEBUG
    if ( aPrevPID != SM_NULL_PID )
    {
        IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aPrevPID ) == ID_TRUE );
    }

    if ( aNextPID != SM_NULL_PID )
    {
        IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aNextPID ) == ID_TRUE );
    }
#endif
    sPCHSlot = smmManager::getPCHSlot( sSpaceID, aPID );
    IDE_ASSERT( sPCHSlot->mPCH != NULL );

    // Page Memory를 할당한다.
    IDE_TEST( allocPageMemory( aTBSNode, aPID ) != IDE_SUCCESS );

    smLayerCallback::linkPersPage( sPCHSlot->mPagePtr,
                                   aPID,
                                   aPrevPID,
                                   aNextPID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 특정 Page의 PCH안의 Page Memory를 해제한다.
 *
 * aPID [IN] Page Memory를 반납할 Page의 ID
 */
IDE_RC svmManager::freePageMemory( svmTBSNode * aTBSNode, scPageID aPID )
{
    scSpaceID sSpaceID = aTBSNode->mTBSAttr.mID;
    smPCSlot * sPCHSlot;

    IDE_DASSERT( smmManager::isValidPageID( sSpaceID, aPID ) == ID_TRUE );
    
    sPCHSlot = smmManager::getPCHSlot( sSpaceID, aPID );
    
    IDE_ASSERT( sPCHSlot->mPCH != NULL );

    // Page Memory가 할당되어 있어야 한다.
    IDE_ASSERT( sPCHSlot->mPagePtr != NULL );

    // BUG-47487: DATA / FLI 페이지 경우 나누기 free ( Volatile )
    if ( ( svmExpandChunk::isFLIPageID( aTBSNode, aPID ) == ID_TRUE ) )
    {
        //FLI
        IDE_TEST( freePage( aTBSNode, 
                            (svmTempPage*) sPCHSlot->mPagePtr, 
                            ID_FALSE )
                  != IDE_SUCCESS );
    }
    else
    {
        //DATA
        IDE_TEST( freePage( aTBSNode, 
                            (svmTempPage*) sPCHSlot->mPagePtr )
                  != IDE_SUCCESS );
    }

    sPCHSlot->mPagePtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC svmManager::allocPageAlignedPtr( UInt    a_nSize,
                                        void**  a_pMem,
                                        void**  a_pAlignedMem )
{
    static UInt s_nPageSize = idlOS::getpagesize();
    void  * s_pMem;
    void  * s_pAlignedMem;
    UInt    sState  = 0;

    s_pMem = NULL;

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SVM,
                               a_nSize + s_nPageSize - 1,
                               &s_pMem)
             != IDE_SUCCESS);
    sState = 1;

    idlOS::memset(s_pMem, 0, a_nSize + s_nPageSize - 1);

    s_pAlignedMem = idlOS::align(s_pMem, s_nPageSize);

    *a_pMem = s_pMem;
    *a_pAlignedMem = s_pAlignedMem;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( s_pMem ) == IDE_SUCCESS );
            s_pMem = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

// Base Page ( 0번 Page ) 에 Latch를 건다
// 0번 Page를 변경하는 Transaction들이 없음을 보장한다.
IDE_RC svmManager::lockBasePage(svmTBSNode * aTBSNode)
{
    IDE_TEST( svmFPLManager::lockAllocChunkMutex(aTBSNode) != IDE_SUCCESS );

    IDE_TEST( svmFPLManager::lockAllFPLs(aTBSNode) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Fatal Error at svmManager::lockBasePage");

    return IDE_FAILURE;
}

// Base Page ( 0번 Page ) 에서 Latch를 푼다.
// lockBasePage로 잡은 Latch를 모두 해제한다
IDE_RC svmManager::unlockBasePage(svmTBSNode * aTBSNode)
{
    IDE_TEST( svmFPLManager::unlockAllFPLs(aTBSNode) != IDE_SUCCESS);
    IDE_TEST( svmFPLManager::unlockAllocChunkMutex(aTBSNode) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Fatal Error at svmManager::unlockBasePage");

    return IDE_FAILURE;
}
