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
* $Id: smmTBSMultiPhase.cpp 19201 2006-11-30 00:54:40Z kmkim $
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

smmTBSMultiPhase::smmTBSMultiPhase()
{
}

/* Server Shutdown시에 Tablespace의 상태에 따라
   Tablespace의 초기화된 단계들을 파괴한다.

   [IN] aTBSNode - Tablespace의  Node
   
   ex> DISCARDED, OFFLINE상태의 TBS는 MEDIA, STATE단계를 파괴
   ex> ONLINE상태의 TBS는 PAGE,MEDIA,STATE단계를 파괴
 */
IDE_RC smmTBSMultiPhase::finiTBS( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( finiToStatePhase( aTBSNode )
              != IDE_SUCCESS );

    if ( sctTableSpaceMgr::hasState( & aTBSNode->mHeader,
                                     SCT_SS_NEED_STATE_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( finiStatePhase( aTBSNode ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/* STATE Phase 로부터 Tablespace의 상태에 따라
   필요한 단게까지 다단계 초기화

   [IN] aTBSNode - Tablespace의  Node
   [IN] aTBSAttr - Tablespace의  Attribute
   
   ex> DISCARDED, OFFLINE상태의 TBS는 MEDIA, STATE단계를 파괴
   ex> ONLINE상태의 TBS는  STATE, MEDIA, PAGE단계를 하나씩 초기화
 */
IDE_RC smmTBSMultiPhase::initTBS( smmTBSNode        * aTBSNode,
                                  smiTableSpaceAttr * aTBSAttr )
{
    IDE_DASSERT( aTBSNode != NULL );

    // STATE Phase 초기화
    if ( sctTableSpaceMgr::isStateInSet( aTBSAttr->mTBSStateOnLA,
                                         SCT_SS_NEED_STATE_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( initStatePhase( aTBSNode,
                                  aTBSAttr ) != IDE_SUCCESS );
    }

    // MEDIA, PAGE Phase 초기화
    if ( sctTableSpaceMgr::isStateInSet( aTBSNode->mHeader.mState,
                                         SCT_SS_NEED_MEDIA_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( initFromStatePhase( aTBSNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* STATE단계까지 초기화된 Tablespace의 다단계초기화

   [IN] aTBSNode - Tablespace의  Node
   
   ex> OFFLINE상태의 TBS는 MEDIA단계까지 초기화
   ex> ONLINE상태의 TBS는 PAGE,MEDIA단계까지 초기화
   
 */
  
IDE_RC smmTBSMultiPhase::initFromStatePhase( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    UInt sTBSState = aTBSNode->mHeader.mState;

    // 현재 Tablespace의 상태는 STATE단계까지의 초기화를
    // 필요로 하는 상태여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isStateInSet( sTBSState,
                                                SCT_SS_NEED_STATE_PHASE )
                == ID_TRUE );
    
    // MEDIA Phase초기화
    if ( sctTableSpaceMgr::isStateInSet( sTBSState,
                                         SCT_SS_NEED_MEDIA_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( initMediaPhase( aTBSNode ) != IDE_SUCCESS );
    }

    
    // PAGE Phase 초기화
    if ( sctTableSpaceMgr::isStateInSet( sTBSState,
                                         SCT_SS_NEED_PAGE_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( initPagePhase( aTBSNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* 다단계 해제를 통해 STATE단계로 전이

   [IN] aTBSNode - Tablespace의  Node
   
   ex> OFFLINE상태의 TBS는 MEDIA단계를 파괴
   ex> ONLINE상태의 TBS는 PAGE,MEDIA단계를 파괴

   => sTBSNode안의 State를 이용하지 않고 인자로 넘어온 State를 이용한다
 */
IDE_RC smmTBSMultiPhase::finiToStatePhase( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    UInt sTBSState = aTBSNode->mHeader.mState;
    
    
    // PAGE Phase 파괴
    if ( sctTableSpaceMgr::isStateInSet( sTBSState,
                                         SCT_SS_NEED_PAGE_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( finiPagePhase( aTBSNode ) != IDE_SUCCESS );
    }

    // MEDIA Phase파괴
    if ( sctTableSpaceMgr::isStateInSet( sTBSState,
                                         SCT_SS_NEED_MEDIA_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( finiMediaPhase( aTBSNode ) != IDE_SUCCESS );
    }

    // 현재 Tablespace의 상태는 STATE단계까지의 초기화를
    // 필요로 하는 상태여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isStateInSet( sTBSState,
                                                SCT_SS_NEED_STATE_PHASE )
                == ID_TRUE );
    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 다단계 해제를 통해 MEDIA단계로 전이

   [IN] aTBSState - Tablespace의 State
   [IN] aTBSNode - Tablespace의  Node
   
   ex> ONLINE상태의 TBS에 대해 PAGE단계를 파괴

   => sTBSNode안의 State를 이용하지 않고 인자로 넘어온 State를 이용한다
 */
IDE_RC smmTBSMultiPhase::finiToMediaPhase( UInt         aTBSState,
                                           smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    // PAGE Phase 파괴
    if ( sctTableSpaceMgr::isStateInSet( aTBSState, SCT_SS_NEED_PAGE_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( finiPagePhase( aTBSNode ) != IDE_SUCCESS );
    }

    // 현재 Tablespace의 상태는 MEDIA단계까지의 초기화를
    // 필요로 하는 상태여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isStateInSet( aTBSState,
                                                SCT_SS_NEED_MEDIA_PHASE )
                == ID_TRUE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Tablespace의 STATE단계를 초기화

   [IN] aTBSNode - Tablespace의  Node
   [IN] aTBSAttr - Tablespace의  Attribute
 */
IDE_RC smmTBSMultiPhase::initStatePhase( smmTBSNode * aTBSNode,
                                         smiTableSpaceAttr * aTBSAttr )
{
    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aTBSAttr != NULL );

    // Memory Tablespace Node의 초기화 실시
    IDE_TEST( smmManager::initMemTBSNode( aTBSNode,
                                          aTBSAttr )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Tablespace의 MEDIA단계를 초기화

   [IN] aTBSNode - Tablespace의  Node

   참고 : STATE단계까지 초기화가 되어 있어야 한다.
 */
IDE_RC smmTBSMultiPhase::initMediaPhase( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    // Memory Tablespace Node의 초기화 실시
    IDE_TEST( smmManager::initMediaSystem( aTBSNode )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Tablespace의 PAGE단계를 초기화

   [IN] aTBSNode - Tablespace의  Node

   참고 : MEDIA단계까지초기화가 되어 있어야 한다.
 */
IDE_RC smmTBSMultiPhase::initPagePhase( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    // Memory Tablespace Node의 초기화 실시
    IDE_TEST( smmManager::initPageSystem( aTBSNode )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Tablespace의 STATE단계를 파괴

   [IN] aTBSNode - Tablespace의  Node

   참고 : STATE단계가 초기화된 상태여야 한다.
   참고 : PAGE,MEDIA단계가 파괴되어 있어야 한다.
 */
IDE_RC smmTBSMultiPhase::finiStatePhase( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smmManager::finiMemTBSNode( aTBSNode )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Tablespace의 MEDIA단계를 파괴

   [IN] aTBSNode - Tablespace의  Node

   참고 : STATE, MEDIA단계가 모두 초기화된 상태여야 한다.
   참고 : PAGE단계가 파괴 되어 있어야 한다.
 */
IDE_RC smmTBSMultiPhase::finiMediaPhase( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smmManager::finiMediaSystem( aTBSNode )
              != IDE_SUCCESS );


    // To Fix BUG-18100, 18099
    //        Shutdown시 Checkpoint Path Node관련 Memory Leak발생 문제 해결
    IDE_TEST( smmTBSChkptPath::freeAllChkptPathNode( aTBSNode )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Tablespace의 PAGE단계를 파괴

   [IN] aTBSNode - Tablespace의  Node

   참고 : STATE, MEDIA, PAGE단계가 모두 초기화된 상태여야 한다.
 */
IDE_RC smmTBSMultiPhase::finiPagePhase( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smmManager::finiPageSystem( aTBSNode )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
