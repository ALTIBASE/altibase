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
 
#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <svmReq.h>
#include <sctTableSpaceMgr.h>
#include <svmDatabase.h>
#include <svmManager.h>
#include <svmTBSStartupShutdown.h>

/*
  생성자 (아무것도 안함)
*/
svmTBSStartupShutdown::svmTBSStartupShutdown()
{

}


/*
    Server startup시 Log Anchor의 Tablespace Attribute를 바탕으로
    Tablespace Node를 구축한다.

    [IN] aTBSAttr      - Log Anchor에 저장된 Tablespace의 Attribute
    [IN] aAnchorOffset - Log Anchor상에 Tablespace Attribute가 저장된 Offset
*/
IDE_RC svmTBSStartupShutdown::loadTableSpaceNode(
           smiTableSpaceAttr   * aTBSAttr,
           UInt                  aAnchorOffset )
{
    UInt         sStage = 0;
    svmTBSNode * sTBSNode;

    IDE_DASSERT( aTBSAttr != NULL );
    IDE_DASSERT( aTBSAttr->mAttrType == SMI_TBS_ATTR );

    // sTBSNode를 초기화 한다.
    IDE_TEST(svmManager::allocTBSNode(&sTBSNode, aTBSAttr)
             != IDE_SUCCESS);

    sStage = 1;

    // Volatile Tablespace를 초기화한다.
    IDE_TEST(svmManager::initTBS(sTBSNode) != IDE_SUCCESS);

    sStage = 2;

    // Log Anchor상의 Offset초기화
    sTBSNode->mAnchorOffset = aAnchorOffset;

    /* 동일한 tablespace명이 존재하는지 검사한다. */
    // BUG-26695 TBS Node가 없는것이 정상이므로 없을 경우 오류 메시지를 반환하지 않도록 수정
    IDE_ASSERT( sctTableSpaceMgr::checkExistSpaceNodeByName(
                    sTBSNode->mHeader.mName ) == ID_FALSE );

    // 생성한 Tablespace Node를 Tablespace관리자에 추가
    sctTableSpaceMgr::addTableSpaceNode((sctTableSpaceNode*)sTBSNode);
    sStage = 3;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 3:
            sctTableSpaceMgr::removeTableSpaceNode(
                                  (sctTableSpaceNode*) sTBSNode );
        case 2:
            /* BUG-39806 Valgrind Warning
             * - svmTBSDrop::dropTableSpacePending() 의 처리를 위해서, 먼저 검사
             *   하고 svmManager::finiTBS()를 호출합니다.
             */
            if ( ( sTBSNode->mHeader.mState & SMI_TBS_DROPPED )
                 != SMI_TBS_DROPPED )
            {
                IDE_ASSERT( svmManager::finiTBS( sTBSNode ) == IDE_SUCCESS );
            }
            else
            {
                // Drop된 TBS는  이미 자원이 해제되어 있다.
            }
        case 1:
            IDE_ASSERT(svmManager::destroyTBSNode(sTBSNode)
                       == IDE_SUCCESS);
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 * svmTBSStartupShutdown::prepareAllTBS를 위한 Action함수
 * 하나의 Volatile TBS에 대해 TBS 초기화 과정을 수행한다.
 * 이미 TBSNode는 초기화가 되어 있어야 한다.
 * Volatile TBS 초기화 과정은 다음과 같다.
 *  1. TBS page 생성
 */
IDE_RC svmTBSStartupShutdown::prepareTBSAction(idvSQL*            /*aStatistics*/,
                                               sctTableSpaceNode *aTBSNode,
                                               void              */* aArg */)
{
    svmTBSNode *sTBSNode = (svmTBSNode*)aTBSNode;

    IDE_DASSERT(sTBSNode != NULL);

    if ( sctTableSpaceMgr::isVolatileTableSpace(aTBSNode->mID) == ID_TRUE )
    {
        IDE_TEST(svmManager::createTBSPages(
                                 sTBSNode,
                                 sTBSNode->mTBSAttr.mName,
                                 sTBSNode->mTBSAttr.mVolAttr.mInitPageCount )
              != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * 모든 TBSNode를 순회하면서 Volatile TBS에 대해
 * 초기화 작업을 한다.
 * 이 함수가 불려지기 전에 smrLogAnchorMgr이
 * 모든 TBSNode들을 초기화 과정을 마친 상태이어야 한다.
 */
IDE_RC svmTBSStartupShutdown::prepareAllTBS()
{
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  prepareTBSAction,
                                                  NULL,
                                                  SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 모든 Volatile Tablespace를 destroy한다.
 */
IDE_RC svmTBSStartupShutdown::destroyAllTBSNode()
{

    sctTableSpaceNode * sNextSpaceNode;
    sctTableSpaceNode * sCurrSpaceNode;
    svmTBSNode        * sTBSNode;

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurrSpaceNode != NULL )
    {
        sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNodeIncludingDropped( sCurrSpaceNode->mID );

        if ( sctTableSpaceMgr::isVolatileTableSpace(sCurrSpaceNode->mID) == ID_TRUE )
        {

            sctTableSpaceMgr::removeTableSpaceNode( sCurrSpaceNode );

            /* BUG-39806 Valgrind Warning
             * - svmTBSDrop::dropTableSpacePending() 의 처리를 위해서, 먼저 검사
             *   하고 svmManager::finiTBS()를 호출합니다.
             */
            sTBSNode = (svmTBSNode*)sCurrSpaceNode;

            if ( ( sTBSNode->mHeader.mState & SMI_TBS_DROPPED )
                 != SMI_TBS_DROPPED )
            {
                IDE_TEST( svmManager::finiTBS( sTBSNode ) != IDE_SUCCESS);
            }
            else
            {
                // Drop된 TBS는  이미 자원이 해제되어 있다.
            }

            // 이 안에서 sCurrSpaceNode의 메모리까지 해제한다.
            IDE_TEST(svmManager::destroyTBSNode((svmTBSNode*)sCurrSpaceNode)
                     != IDE_SUCCESS );
        }

        sCurrSpaceNode = sNextSpaceNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Volatile Tablespace 관리자의 초기화
 */
IDE_RC svmTBSStartupShutdown::initializeStatic()
{
    // 아무것도 하지 않는다.

    return IDE_SUCCESS;
}



/*
    Volatile Tablespace관리자의 해제
 */
IDE_RC svmTBSStartupShutdown::destroyStatic()
{
    // 모든 Memory Tablespace를 destroy한다.
    IDE_TEST( destroyAllTBSNode() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
