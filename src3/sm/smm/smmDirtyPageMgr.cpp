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
 * $Id: smmDirtyPageMgr.cpp 90085 2021-02-26 02:14:29Z et16 $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <sctTableSpaceMgr.h>
#include <smm.h>
#include <smmDirtyPageList.h>
#include <smmDirtyPageMgr.h>
#include <smmManager.h>
#include <smmReq.h>

IDE_RC smmDirtyPageMgr::initialize(scSpaceID aSpaceID , SInt a_listCount)
{
    
    SInt i;
    
    mSpaceID    = aSpaceID ;
    m_listCount = a_listCount;
    
    IDE_ASSERT( m_listCount != 0 );
    
    m_list = NULL;
    
    /* TC/FIT/Limit/sm/smm/smmDirtyPageMgr_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "smmDirtyPageMgr::initialize::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMM,
                                 (ULong)ID_SIZEOF(smmDirtyPageList) * m_listCount,
                                 (void**)&m_list ) != IDE_SUCCESS,
                    insufficient_memory );
    
    for( i = 0; i < m_listCount; i++ )
    {
        new (m_list + i) smmDirtyPageList;
        IDE_TEST( m_list[i].initialize(aSpaceID, i) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    모든 Dirty Page List들로부터 Dirty Page들을 제거한다.
 */
IDE_RC smmDirtyPageMgr::removeAllDirtyPages()
{
    SInt i;
    
    for (i = 0; i < m_listCount; i++)
    {
        IDE_TEST(m_list[i].open() != IDE_SUCCESS);
        IDE_TEST(m_list[i].clear() != IDE_SUCCESS);
        IDE_TEST(m_list[i].close() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}



IDE_RC smmDirtyPageMgr::destroy()
{
    SInt i;
    
    for (i = 0; i < m_listCount; i++)
    {
        IDE_TEST(m_list[i].destroy() != IDE_SUCCESS);
    }
    
    IDE_TEST(iduMemMgr::free(m_list) != IDE_SUCCESS);
    
    m_list = NULL;
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smmDirtyPageMgr::lockDirtyPageList(smmDirtyPageList **a_list)
{
    static SInt sMutexDist = 0;
    
    idBool s_break = ID_FALSE;
    idBool s_locked;
    smmDirtyPageList *s_list = NULL;
    SInt i, sStart;
    
    sStart = sMutexDist;
    do
    {
        for (i = sStart; i < m_listCount; i++)
        {
            s_list = m_list + i;
            
            IDE_TEST(s_list->m_mutex.trylock(s_locked)
                           != IDE_SUCCESS);
            
            if (s_locked == ID_TRUE)
            {
                s_break = ID_TRUE; 
                sMutexDist = i + 1;
                break; // 탈출! lock을 잡음
            }
        }
        sStart = 0;
    } while(s_break == ID_FALSE);
    
    *a_list = s_list;
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
    
}

/*
 * Dirty Page 리스트에 변경된 Page를 추가한다.
 *
 * aPageID [IN] Dirty Page로 추가할 Page의 ID
 */
IDE_RC smmDirtyPageMgr::insDirtyPage( scPageID aPageID )
{
    smmDirtyPageList *sList;

    smmPCH      *sPCH;

    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace( mSpaceID ) == ID_TRUE );
    IDE_DASSERT( smmManager::isValidPageID( mSpaceID, aPageID ) == ID_TRUE );
    
    IDL_MEM_BARRIER;
#ifdef DEBUG
    smPCSlot *sPCSlot = smmManager::getPCHSlot( mSpaceID, aPageID );
    sPCH              = (smmPCH*)sPCSlot->mPCH;
#else
    sPCH = smmManager::getPCH( mSpaceID, aPageID );
#endif    
    //IDE_DASSERT( sPCH->m_page != NULL );
    
    /*
      [tx1] Alloc Page#1
      [tx1] commit
      [tx2] Modify Page#1
      [tx2] Free Page#1
      === CHECKPOINT ===
      server dead..
      restart redo.. -> Page#1의 PID값이 제대로 된값임을 보장못함
      
      ------------------------------------------------------------
      
      Page안의 PID는 smm 단에서 테이블로 Page를 할당할 때 설정된다.
      
      만약 페이지가 할당되었다가 DROP TABLE문에 의해 DB로 반납되고,
      Checkpoint가 발생하면,해당 Page는 Free Page이어서, Page 메모리조차
      없기 때문에, Disk로 내려가지 않게 된다.
      
      이때,Page#1을 할당한 트랜잭션 tx1이 commit한 트랜잭션이라면,
      Redo시에 tx1이 기록한 log들은 redo대상으로 편입되지 않으며,
      Alloc Page#1에 대한 REDO가 실행되지 않게 된다.
      
      그리고 Page#1의 내용을 변경한 트랜잭션 tx2가 redo되면서
      Page#1을 Dirty Page로 추가하게 될 경우,
      Page#1의 PID가 엉뚱한 값으로 되어 있을 수 있다.
      
      그러나, Redo가 다 끝난 상황에서는 Page#1이 Free되기 때문에
      여기에 더이상 접근할 일이 없다.
      
    */
    // Restart Recovery중에는 Page 메모리 안의 PID값을 보장 못한다.
#ifdef DEBUG
    if ( smLayerCallback::isRestartRecoveryPhase() == ID_FALSE ) 
    {
        IDE_DASSERT( smLayerCallback::getPersPageID( sPCSlot->mPagePtr )
                     == aPageID );
    }
#endif    

    if (sPCH->m_dirty == ID_FALSE)
    {
        sPCH->m_dirty = ID_TRUE;
        
        IDE_TEST(lockDirtyPageList(&sList) != IDE_SUCCESS);
        IDE_TEST_RAISE(sList->addDirtyPage( aPageID )
                       != IDE_SUCCESS, add_error);
        IDE_TEST(sList->m_mutex.unlock() != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(add_error);
    {
        IDE_PUSH();
        IDE_ASSERT( sList->m_mutex.unlock() == IDE_SUCCESS );
        
        IDE_POP();
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

/***********************************************************************
    여러 Tablespace에 대한 연산을 관리하는 인터페이스
    Refactoring을 통해 추후 별도의 Class로 빼도록 한다.
 ***********************************************************************/


/*
    Dirty Page관리자를 생성한다.
 */
IDE_RC smmDirtyPageMgr::initializeStatic()
{

    return IDE_SUCCESS;

}

/*
    Dirty Page관리자를 파괴한다.
 */

IDE_RC smmDirtyPageMgr::destroyStatic()
{
    
    return IDE_SUCCESS;
}


/*
 * 특정 Tablespace의 Dirty Page 리스트에 변경된 Page를 추가한다.
 *
 * aSpaceID [IN] Dirty Page가 속한 Tablespace의 ID
 * aPageID  [IN] Dirty Page로 추가할 Page의 ID
 */
IDE_RC smmDirtyPageMgr::insDirtyPage( scSpaceID aSpaceID, scPageID aPageID )
{
    smmDirtyPageMgr * sDPMgr = NULL;

    IDE_TEST( findDPMgr(aSpaceID, &sDPMgr ) != IDE_SUCCESS );

    // 해당 Tablespace의 Dirty Page관리자가 없어서는 안된다.
    IDE_ASSERT( sDPMgr != NULL );

    IDE_TEST( sDPMgr->insDirtyPage( aPageID ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC smmDirtyPageMgr::insDirtyPage(scSpaceID aSpaceID, void * a_new_page)
{
    
    IDE_DASSERT( a_new_page != NULL );
    
    return insDirtyPage( aSpaceID,
                         smLayerCallback::getPersPageID( a_new_page ) );
}

/*
    특정 Tablespace를 위한 Dirty Page관리자를 생성한다.

    [IN] aSpaceID - 생성하고자 하는 Dirty Page관리자가 속한 Tablespace의 ID
 */
IDE_RC smmDirtyPageMgr::createDPMgr(smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );
    
    smmDirtyPageMgr * sDPMgr;
    
    /* TC/FIT/Limit/sm/smm/smmDirtyPageMgr_createDPMgr_malloc.sql */
    IDU_FIT_POINT_RAISE( "smmDirtyPageMgr::createDPMgr::malloc", 
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMM,
                               ID_SIZEOF(smmDirtyPageMgr),
                               (void**) & sDPMgr ) != IDE_SUCCESS,
                   insufficient_memory );
    
    (void)new ( sDPMgr ) smmDirtyPageMgr;
    
    IDE_TEST( sDPMgr->initialize(aTBSNode->mHeader.mID, ID_SCALABILITY_CPU )
              != IDE_SUCCESS);


    aTBSNode->mDirtyPageMgr = sDPMgr;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
   특정 Tablespace를 위한 Dirty Page관리자를 찾아낸다.
   아직 Dirty Page 관리자가 생성되지 않은 경우 NULL이 리턴된다.
   
   [IN]  aSpaceID - 찾고자 하는 DirtyPage관리자가 속한 Tablespace의 ID
   [OUT] aDPMgr   - 찾아낸 Dirty Page 관리자
*/

IDE_RC smmDirtyPageMgr::findDPMgr( scSpaceID aSpaceID,
                                   smmDirtyPageMgr ** aDPMgr )
{
    smmTBSNode * sTBSNode;
    
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sTBSNode)
              != IDE_SUCCESS );

    *aDPMgr = sTBSNode->mDirtyPageMgr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    특정 Tablespace의 Dirty Page관리자를 제거한다.

    [IN] aSpaceID - 제거하고자 하는 Dirty Page관리자가 속한 Tablespace ID
 */
IDE_RC smmDirtyPageMgr::removeDPMgr( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );
    
    smmDirtyPageMgr * sDPMgr;
    
    sDPMgr = aTBSNode->mDirtyPageMgr;

    IDE_TEST( sDPMgr->removeAllDirtyPages() != IDE_SUCCESS );
    
    IDE_TEST( sDPMgr->destroy() != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::free( sDPMgr ) != IDE_SUCCESS );
    
    aTBSNode->mDirtyPageMgr = NULL;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}




