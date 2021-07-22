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
* $Id: smmTBSStartupShutdown.cpp 19201 2006-11-30 00:54:40Z kmkim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <smm.h>
#include <smmReq.h>
#include <sctTableSpaceMgr.h>
#include <smmTBSMultiPhase.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSStartupShutdown::smmTBSStartupShutdown()
{

}



/*
    Server startup시 Log Anchor의 Tablespace Attribute를 바탕으로
    Tablespace Node를 구축한다.

    [IN] aTBSAttr      - Log Anchor에 저장된 Tablespace의 Attribute
    [IN] aAnchorOffset - Log Anchor상에 Tablespace Attribute가 저장된 Offset
*/
IDE_RC smmTBSStartupShutdown::loadTableSpaceNode(
                                           smiTableSpaceAttr   * aTBSAttr,
                                           UInt                  aAnchorOffset )
{
    smmTBSNode    * sTBSNode;
    UInt            sState  = 0;

    IDE_DASSERT( aTBSAttr != NULL );
    IDE_DASSERT( aTBSAttr->mAttrType == SMI_TBS_ATTR );

    /* smmTBSStartupShutdown_loadTableSpaceNode_calloc_TBSNode.tc */
    IDU_FIT_POINT("smmTBSStartupShutdown::loadTableSpaceNode::calloc::TBSNode");
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMM,
                               1,
                               ID_SIZEOF(smmTBSNode),
                               (void**)&(sTBSNode))
                 != IDE_SUCCESS);
    sState  = 1;

    // Tablespace의 상태에 따른 다단계 초기화 수행
    // 우선 STATE단게까지만 초기화한다.
    //
    // MEDIA단계 초기화를 위해서는 체크포인트 경로를 필요로 한다.
    // Log Anchor로부터 모든 체크포인트 패스 노드 가 전부 로드된 후에
    // MEDIA단계 이후의 초기화를 진행한다.

    // 여기에서 TBSNode.mState에 TBSAttr.mTBSStateOnLA를 복사
    IDE_TEST( smmTBSMultiPhase::initStatePhase( sTBSNode,
                                                aTBSAttr )
              != IDE_SUCCESS );

    // Log Anchor상의 Offset초기화
    sTBSNode->mAnchorOffset = aAnchorOffset;


    /* 동일한 tablespace명이 존재하는지 검사한다. */
    // BUG-26695 TBS Node가 없는것이 정상이므로 없을 경우 오류 메시지를 반환하지 않도록 수정
    IDE_ASSERT( sctTableSpaceMgr::checkExistSpaceNodeByName(
                    sTBSNode->mHeader.mName ) == ID_FALSE );

    // 생성한 Tablespace Node를 Tablespace관리자에 추가
    sctTableSpaceMgr::addTableSpaceNode( (sctTableSpaceNode*)sTBSNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sTBSNode ) == IDE_SUCCESS );
            sTBSNode = NULL;
        default:
            break;
    }

    // 서버구동중 Tablespace Load도중 실패시
    // 에러처리하지 않고 FATAL로 죽인다.

    IDE_CALLBACK_FATAL("Failed to initialize Tablespace reading from loganchor ");

    return IDE_FAILURE;
}

/*
  로그앵커로 부터 판독된 Checkpoint Image Attribute를
  런타임 헤더에 설정한다. 파일을 오픈하지는 않는다.

  [IN] aChkptImageAttr - 로그앵커에 판독된 메모리 데이타파일 속성
  [IN] aMemRedoLSN  - 로그앵커에 저장된 메모리 Redo LSN
  [IN] aAnchorOffset   - 속성이 저장된 LogAnchor Offset
 */
IDE_RC smmTBSStartupShutdown::initializeChkptImageAttr(
                               smmChkptImageAttr * aChkptImageAttr,
                               smLSN             * aMemRedoLSN,
                               UInt                aAnchorOffset )
{
    UInt              sLoop;
    UInt              sSmVersion;
    smmTBSNode      * sSpaceNode;
    smmDatabaseFile * sDatabaseFile;

    IDE_DASSERT( aMemRedoLSN   != NULL );
    IDE_DASSERT( aMemRedoLSN != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace(
                 aChkptImageAttr->mSpaceID ) == ID_TRUE );

    // 테이블스페이스 노드 검색
    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aChkptImageAttr->mSpaceID );

    if ( sSpaceNode != NULL )
    {
        if ( SMI_TBS_IS_DROPPED(sSpaceNode->mHeader.mState) )
        {
            // Drop된 Tablespace의 경우 무시
            // 주의 ! DROP_PENDING인 경우
            //        아직 처리하지 못한 DROP작업을 완료해야 하기 때문에
            //        아래 else를 타야 함.
        }
        else
        {
            // 테이블스페이스 노드가 존재하는 경우

            // 데이타파일 객체를 검색한다.
            // 실제 파일이 존재하는지는 identifyDatabase에서
            // 처리한다.

            // 로그앵커로부터 저장된 바이너리 버전을 얻는다.
            sSmVersion = smLayerCallback::getSmVersionIDFromLogAnchor();

            for ( sLoop = 0; sLoop < SMM_PINGPONG_COUNT; sLoop ++ )
            {
                IDE_TEST( smmManager::getDBFile( sSpaceNode,
                                                 sLoop,
                                                 aChkptImageAttr->mFileNum,
                                                 SMM_GETDBFILEOP_NONE,
                                                 &sDatabaseFile )
                          != IDE_SUCCESS );

                sDatabaseFile->setChkptImageHdr(
                                        aMemRedoLSN,
                                        &aChkptImageAttr->mMemCreateLSN,
                                        &sSpaceNode->mHeader.mID,
                                        &sSmVersion,
                                        &aChkptImageAttr->mDataFileDescSlotID );
            }

            // 로그앵커에 저장된 파일들은 모두 생성된 파일이므로
            // crtdbfile 플래그를 true로 설정하여 로그앵커에
            // 중복 추가되지 않도록 한다.
            for ( sLoop = 0; sLoop < SMM_PINGPONG_COUNT; sLoop++ )
            {
                IDE_ASSERT ( smmManager::getCreateDBFileOnDisk(
                                                 sSpaceNode,
                                                 sLoop,      // pingpong no
                                                 aChkptImageAttr->mFileNum )
                             == ID_FALSE );
            }

            // 데이타베이스 검증과정과 미디어복구과정에서
            // 로그앵커에 저장된 데이타파일을 대상으로 검증하고
            // 복구도 하기 위해 LstCreatedDBFile 을 설정한다.
            if ( aChkptImageAttr->mFileNum > 0 )
            {
                IDE_ASSERT( aChkptImageAttr->mFileNum >
                            sSpaceNode->mLstCreatedDBFile );
            }

            sSpaceNode->mLstCreatedDBFile = aChkptImageAttr->mFileNum;

            for ( sLoop = 0; sLoop < SMM_PINGPONG_COUNT; sLoop++ )
            {
                smmManager::setCreateDBFileOnDisk(
                                sSpaceNode,
                                sLoop, // pingpong no.
                                aChkptImageAttr->mFileNum,
                                aChkptImageAttr->mCreateDBFileOnDisk[ sLoop ] );
            }

            smmManager::setAnchorOffsetToCrtDBFileInfo( sSpaceNode,
                                                        aChkptImageAttr->mFileNum,
                                                        aAnchorOffset );
        }

    }
    else
    {
        // DROPPED이 되었거나 존재하지 않는 경우에는
        // NOTHING TO DO...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * Loganchor로부터 읽어들인 Checkpoint Path Attribute로 Node를 생성한다.
 *
 * aChkptPathAttr [IN] - 추가할 smiChkptPathAttr* 타입의 Attribute
 * aAnchorOffset  [IN] - Loganchor의 메모리버퍼상의 ChkptPath Attribute 오프셋
 */
IDE_RC smmTBSStartupShutdown::createChkptPathNode(
                                 smiChkptPathAttr *  aChkptPathAttr,
                                 UInt                aAnchorOffset )
{
    smmTBSNode        * sTBSNode;
    smmChkptPathNode  * sCPathNode;
    UInt                sState  = 0;

    IDE_DASSERT( aChkptPathAttr != NULL );
    IDE_DASSERT( aChkptPathAttr->mAttrType == SMI_CHKPTPATH_ATTR );
    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace(
                                   aChkptPathAttr->mSpaceID ) == ID_TRUE );
    IDE_DASSERT( aAnchorOffset != 0 );

    /* smmTBSStartupShutdown_createChkptPathNode_calloc_CPathNode.tc */
    IDU_FIT_POINT("smmTBSStartupShutdown::createChkptPathNode::calloc::CPathNode");
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMM,
                               1,
                               ID_SIZEOF(smmChkptPathNode),
                               (void**)&(sCPathNode))
             != IDE_SUCCESS);
    sState = 1;

    idlOS::memcpy(& sCPathNode->mChkptPathAttr,
                  aChkptPathAttr,
                  ID_SIZEOF(smiChkptPathAttr));

    // set attibute loganchor offset
    sCPathNode->mAnchorOffset = aAnchorOffset;

    // 테이블스페이스 노드 검색
    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aChkptPathAttr->mSpaceID );

    if ( sTBSNode != NULL )
    {
        sctTableSpaceMgr::lockSpaceNode( NULL /* idvSQL * */,
                                         sTBSNode );
        sState = 2;

        if ( SMI_TBS_IS_DROPPED(sTBSNode->mHeader.mState) )
        {
            // Drop된 Tablespace의 경우 무시
            // 주의 ! DROP_PENDING인 경우
            //        아직 처리하지 못한 DROP작업을 완료해야 하기 때문에
            //        아래 else를 타야 함.
        }
        else
        {
            IDE_TEST( smmTBSChkptPath::addChkptPathNode( sTBSNode,
                                                         sCPathNode )
                      != IDE_SUCCESS );
        }
        sState = 1;
        sctTableSpaceMgr::unlockSpaceNode( sTBSNode );
    }
    else
    {
        // Drop된 Tablespace의 경우 무시
    }


    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sState)
    {
        case 2:
            sctTableSpaceMgr::unlockSpaceNode( sTBSNode );
        case 1:
            IDE_ASSERT( iduMemMgr::free( sCPathNode ) == IDE_SUCCESS );
            sCPathNode = NULL;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}



/* smmTBSStartupShutdown::initFromStatePhase4AllTBS 의 액션 함수
 */
IDE_RC smmTBSStartupShutdown::initFromStatePhaseAction(
           idvSQL            * /*aStatistics */,
           sctTableSpaceNode * aTBSNode,
           void * /* aActionArg */ )
{
    IDE_DASSERT( aTBSNode != NULL );

    if ( sctTableSpaceMgr::isMemTableSpace( aTBSNode ) == ID_TRUE )
    {
        IDE_TEST( smmTBSMultiPhase::initFromStatePhase (
                                          (smmTBSNode *) aTBSNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 모든 Tablespace의 다단계 초기화를 수행한다.
 *
 * 메모리 Tablespace는 STATE => MEDIA => PAGE 의 순서로
 * 단계별로 초기화 된다.
 *
 * STATE단계는 Log Anchor에서 Tablespace Node를 읽어들일때 초기화되고,
 * MEDIA, PAGE단계는 Log Anchor로부터 모든 Tablespace Node와
 * 관련 정보를 다 읽어들인후에 초기화 된다.
 *
 * MEDIA Phase를 초기화하기 위해서는 Tablespace Node에
 * 체크포인트 경로가 세팅되어 있어야 한다.
 * 그런데, 체크포인트 경로 Attribute가 저장된 내용이 Log Anchor의
 * 중간 중간에 나타날 수 있기 때문에,
 * Log Anchor의 로드가 모두 완료된 후에 MEDIA단계를 초기화한다.
 *
 */
IDE_RC smmTBSStartupShutdown::initFromStatePhase4AllTBS()
{
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                                  NULL,  /* idvSQL* */
                                  initFromStatePhaseAction,
                                  NULL, /* Action Argument*/
                                  SCT_ACT_MODE_NONE)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
 * 모든 Tablespace를 destroy한다.
 */
IDE_RC smmTBSStartupShutdown::destroyAllTBSNode()
{
    sctTableSpaceNode *sNextSpaceNode;
    sctTableSpaceNode *sCurrSpaceNode;

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurrSpaceNode != NULL )
    {
        sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNodeIncludingDropped( sCurrSpaceNode->mID );

        if ( sctTableSpaceMgr::isMemTableSpace( sCurrSpaceNode ) == ID_TRUE )
        {

            sctTableSpaceMgr::removeTableSpaceNode( sCurrSpaceNode );

            /* BUG-40451 Valgrind Warning
             * - smmTBSDrop::dropTableSpacePending()의 처리를 위해서, 먼저 검사
             *   하고 svmManager::finiTBS()를 호출합니다.
             */ 
            if ( ( sCurrSpaceNode->mState & SMI_TBS_DROPPED )
                 != SMI_TBS_DROPPED )
            {
                IDE_TEST( smmTBSMultiPhase::finiTBS (
                              (smmTBSNode *) sCurrSpaceNode )
                          != IDE_SUCCESS );
            }
            else
            {
                // Drop된 TBS는  이미 자원이 해제되어 있다.
            }

            // To Fix BUG-18178
            //    Shutdown시 Tablespace Node Memory를 해제하지 않음
            IDE_TEST( iduMemMgr::free( sCurrSpaceNode ) != IDE_SUCCESS );
        }

        sCurrSpaceNode = sNextSpaceNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Memory Tablespace 관리자의 초기화
 */
IDE_RC smmTBSStartupShutdown::initializeStatic()
{
    // 아무것도 하지 않는다.

    return IDE_SUCCESS;
}



/*
    Memory Tablespace관리자의 해제
 */
IDE_RC smmTBSStartupShutdown::destroyStatic()
{
    // 모든 Memory Tablespace를 destroy한다.
    IDE_TEST( destroyAllTBSNode() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




