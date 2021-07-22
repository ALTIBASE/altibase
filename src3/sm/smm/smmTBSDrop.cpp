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
* $Id: smmTBSDrop.cpp 19201 2006-11-30 00:54:40Z kmkim $
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
#include <smmTBSDrop.h>
#include <smmTBSMultiPhase.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSDrop::smmTBSDrop()
{

}


/*
    사용자가 생성한 메모리 Tablespace를 drop한다.

  [IN] aTrans      - Tablespace를 Drop하려는 Transaction
  [IN] aTBSNode    - Drop하려는 Tablespace node
  [IN] aTouchMode  - Drop시 Checkpoint Image File삭제 여부 
 */
IDE_RC smmTBSDrop::dropTBS(void         * aTrans,
                              smmTBSNode   * aTBSNode,
                              smiTouchMode   aTouchMode)
                              
{
    // aStatistics는 NULL로 들어온다.
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( dropTableSpace( aTrans, aTBSNode, aTouchMode ) != IDE_SUCCESS );
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
  메모리 테이블 스페이스를 DROP한다.

[IN] aTrans      - Tablespace를 Drop하려는 Transaction
[IN] aTBSNode    - Drop하려는 Tablespace node
[IN] aTouchMode  - Drop시 Checkpoint Image File삭제 여부 
  
[ PROJ-1548 User Memory Tablespace ]

Drop Tablespace 전체 알고리즘 ================================================

* Restart Recovery는 Disk Tablespace와 기본적으로 동일하게 처리한다.

(010) lock TBSNode in X

(020) DROP-TBS-LOG 로깅실시

(030) TBSNode.Status |= Dropping

(040) Drop Tablespace Pending 등록

- commit : ( pending 처리 )
           자세한 알고리즘은 smmTBSDrop::dropTableSpacePending을 참고

- abort  : Log를 따라가며 Undo실시

           (a-010) DROP-TBS-LOG의 UNDO 실시 
              - 수행되는 내용 : TBSNode.Status &= ~Dropping
           // Log Anchor에 TBSNode를 Flush할 필요가 없다
           // Status |= Dropping 설정된 채로 Flush를 안했기 때문

- restart recover종료후 : # (주3)
           if TBSNode.Status == Dropped then
              remove TBSNode from TBS List
           fi

-(주3) : redo/undo끝나고 시스템의 모든 TBS를 Log Anchor에 flush하면서 
         Status가 Dropped인 TBS를 기록하지 않는다.
         또한, 시스템의 TBS List에서도 제거한다.
 */
IDE_RC smmTBSDrop::dropTableSpace(void             * aTrans,
                                     smmTBSNode       * aTBSNode,
                                     smiTouchMode       aTouchMode)
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSNode != NULL );

    sctPendingOp * sPendingOp;


    ////////////////////////////////////////////////////////////////
    // (010) lock TBSNode in X
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                  aTrans,
                  & aTBSNode->mHeader,
                  ID_FALSE,   /* intent */
                  ID_TRUE,    /* exclusive */
                  SCT_VAL_DROP_TBS) /* validation */
              != IDE_SUCCESS );

    
    ////////////////////////////////////////////////////////////////
    // (020) DROP-TBS-LOG 로깅실시
    IDE_TEST( smLayerCallback::writeMemoryTBSDrop( NULL, /* idvSQL* */
                                                   aTrans,
                                                   aTBSNode->mHeader.mID,
                                                   aTouchMode )
              != IDE_SUCCESS );

    ////////////////////////////////////////////////////////////////
    // (030) TBSNode.Status |= Dropping
    // Tx Commit이전에는 DROPPING으로 설정하고
    // Commit시에 Pending으로 DROPPED로 설정된다.
    //
    aTBSNode->mHeader.mState |= SMI_TBS_DROPPING;

    
    ////////////////////////////////////////////////////////////////
    // (040) Drop Tablespace Pending 등록
    //
    // Transaction Commit시에 수행할 Pending Operation등록 
    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                  aTrans,
                  aTBSNode->mHeader.mID,
                  ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                  SCT_POP_DROP_TBS,
                  & sPendingOp )
              != IDE_SUCCESS );

    // Commit시 sctTableSpaceMgr::executePendingOperation에서
    // 수행할 Pending함수 설정
    sPendingOp->mPendingOpFunc = smmTBSDrop::dropTableSpacePending;
    sPendingOp->mTouchMode     = aTouchMode;
    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   Tablespace를 DROP한 Tx가 Commit되었을 때 불리는 Pending함수
   
   [참고] sctTableSpaceMgr::executePendingOperation 에서 호출된다.
 */
IDE_RC smmTBSDrop::dropTableSpacePending( idvSQL*        /* aStatistics */,
                                          sctTableSpaceNode * aTBSNode,
                                          sctPendingOp      * aPendingOp )
{
    IDE_DASSERT( aTBSNode   != NULL );
    IDE_DASSERT( aPendingOp != NULL );
    IDE_DASSERT( aPendingOp->mPendingOpParam == NULL );
    
    IDE_TEST( doDropTableSpace( aTBSNode, aPendingOp->mTouchMode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    실제 Drop Tablespace수행 코드 
  
   PROJ-1548 User Memory Tablespace
 
   Tablespace와 관련된 모든 메모리와 리소스를 반납한다.
   - 예외 : Tablespace의 Lock정보는 다른 Tx들이 대기하면서
             참조할 수 있기 때문에 해제해서는 안된다.

   - 호출되는 경우           
      - Drop Tablespace의 Commit Pending Operation
      - Create Tablespace의 NTA Undo
             
   [ 알고리즘 ] ======================================================
   
   (c-010) TBSNode.Status := Dropped (주3)
   (c-020) flush TBSNode  (주2)

   
   (c-030) latch TBSNode.SyncMutex // Checkpoint와 경합
   (c-040) unlatch TBSNode.SyncMutex
   
   (c-050) Memory Garbage Collector를 Block한다. // Ager와 경합
   (c-060) Memory Garbage Collector를 Unblock한다.
  
   (c-070) close all Checkpoint Image Files
   
   # (주1)
   # DROP TABLESPACE INCLUDING CONTENTS 
   # AND CHECKPOINT IMAGES
   if DropCheckpointImages then
   (c-080) delete Checkpoint Image Files
   fi
   
   (c-090) Lock정보(STATE단계) 제외한 모든 객체 파괴, 메모리 반납

             
   (c-100) Runtime정보 갱신 => Tablespace의 수 Counting 


-(주1) : Checkpoint Image는 지워지면 다시 원복이 불가하므로
         Commit시 Pending처리한다.
         
-(주2) : TBSNode의 Status를 Dropped로 하여 Log Anchor에 내린다.
         - Normal Processing시 checkpoint는 
           Drop된 TBS의 Page를 Flush하지 않는다.
         - Restart Recovery시 Drop된 TBS안의 Page에 Redo/Undo를 무시한다.
         
-(주3) : Checkpoint Image File Node와 Checkpoint Path Node의 경우
         Log Anchor에 그대로 남아있게 된다.
         Server기동후 Tablespace의 상태가 DROPPED이면
         로그앵커안에 존재하는 Checkpoint Image File Node와
         Checkpoint Path Node를 모두 무시한다.
         
*/

IDE_RC smmTBSDrop::doDropTableSpace( sctTableSpaceNode * aTBSNode,
                                     smiTouchMode        aTouchMode )
{
    // BUG-27609 CodeSonar::Null Pointer Dereference (8)
    IDE_ASSERT( aTBSNode   != NULL );

    idBool                      sPageCountLocked = ID_FALSE;
    

    // 여기 들어오는 Tablespace는 항상 Memory Tablespace여야 한다.
    IDE_ERROR( sctTableSpaceMgr::isMemTableSpace( aTBSNode ) == ID_TRUE );
    

    // Checkpoint Thread가 이 Tablespace에 대해 Checkpoint하지 않도록
    // 하기 위해 FLAG를 설정한다.
    IDE_TEST( smmTBSDrop::flushTBSStatus(
                  aTBSNode,
                  aTBSNode->mState | SMI_TBS_DROP_PENDING )
              != IDE_SUCCESS );
    
    
    /////////////////////////////////////////////////////////////////////
    // - latch TBSNode.SyncMutex 
    // - unlatch TBSNode.SyncMutex
    //
    // 현재 Checkpoint가 Dirty Page Flush를 하고 있다면 TBSNode.SyncMutex
    // 를 잡고 있을 것이다. 
    // ==> Mutex를 잡았다가 풀어서 Dirty Page Flush가 종료되기를 기다린다.
    //
    // unlatch후 Checkpoint가 발생하면 TBSNode의 상태가
    // EXECUTING_DROP_PENDING
    // Dirty Page Flush를 실시하지 않고 Skip하게 된다.
    //
    // ( 위의 사항이 Checkpoint Image Header를 변경하는
    //   다음 코드에도 똑같이 적용된다.
    //   smmTBSMediaRecovery::doActUpdateAllDBFileHdr )
    IDE_TEST( sctTableSpaceMgr::latchSyncMutex( aTBSNode )
              != IDE_SUCCESS );
    
    IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( aTBSNode )
              != IDE_SUCCESS );

    /////////////////////////////////////////////////////////////////////
    // - Memory Garbage Collector를 Block한다.
    // - Memory Garbage Collector를 Unblock한다.
    //
    // 현재 Ager가 수행중이라면 blockMemGC에서 Ager의 수행이 완료되기를
    // 기다를 것이다.
    //
    // ==> Mutex를 잡았다가 풀어서 Ager가 한번 다 돌때까지 기다린다.
    //
    // unblock후 Ager가 수행되면 하면 TBSNode의 상태가 DROPPED여서
    // Aging을 실시하지 않고 Skip하게 된다.

    // META이전 단계에서는 AGER가 초기화조차 되어있지 않은 상태이다
    // AGER가 초기화되었는지 확인후 BLOCK/UNBLOCK한다.
    
    // BUG-32237 Free lock node when dropping table.
    //
    // DropTablespacePending 연산에서는 Ager를 잠시 block했다가 Unlock을 합니다.
    // 이는 Ager쪽에서 Tablespace가 Drop 안됐다고 생각하고 작업을 하는 것을
    // 방지하기 위함입니다.
    //  
    // 그런데 이 로직이 없어도, 현재 이미 DropTable 연산과 Aging과 관련된
    // 동시성 처리 로직이 있습니다.
    //   
    // 그것은 smaDeleteThread::waitForNoAccessAftDropTbl 함수입니다.
    // 이것은 DropTablespacePending연산에 있는 Ager Block 연산과 동일한
    // 역할입니다. 즉 실제로 Drop하기 전에 ager와의 동시성을 제어해주는 역할을
    // 합니다.
    //    
    // 즉 다음과 같이 수행됩니다.
    // a. Tx.commit
    // b. Tx.ProcessOIDList => TableHeader에 DeleteSCN 설정
    // c. waitForNoAccessAftDropTbl
    // d. DropTablePending
    // e. Tx.End
    //     
    // b, c번의 순서 때문에 Ager가 Table이 Drop되었다는 것을 확인 못하고
    // Row를 Aging하려는데 DropTablePending에 의해 Page가 날라가는 일은
    // 없습니다.
    //  
    // 1) Table이 Drop되었다는 것을 Ager가 봤을 경우 => 문제 없습니다.
    // 2) Table이 Drop되었다는 것을 Ager가 못봤을 경우 => Transaction이 c번
    // 과정을 통해 대기합니다. 
    //  
    // 또한 e번, Tx.End가 되어야만 Ager가 해당 Transaction도 Aging하기
    // 때문에, DropTablePending의 후속작업인 LockItem을 지우는 작업이
    // 먼저될 까닭도 없습니다.
    // 위와 같이 되기 때문입니다.
    
    

    /////////////////////////////////////////////////////////////////
    // close all Checkpoint Image Files
    // delete Checkpoint Image Files
    IDE_TEST( smmManager::closeAndRemoveChkptImages(
                    (smmTBSNode *) aTBSNode,
                    ( (aTouchMode == SMI_ALL_TOUCH ) ?
                      ID_TRUE /* REMOVE */ :
                      ID_FALSE /* 삭제안함 */ ) )
                != IDE_SUCCESS );



    // To Fix BUG-17267
    //    Tablespace Drop도중 Dirty Page가 존재하여 서버사망
    // => 문제 : finiToStatePhase에서 PCH Entry를 destroy하는데,
    //            Page의 Dirty Stat이 SMM_PCH_DIRTY_STAT_INIT가
    //            아니어서 사망 
    // => 해결 : Drop Tablespace시 모든 Page에 대해 PCH의 Dirty Stat을 
    //            SMM_PCH_DIRTY_STAT_INIT으로 설정한다.
    if ( sctTableSpaceMgr::isStateInSet( aTBSNode->mState,
                                         SCT_SS_NEED_PAGE_PHASE )
         == ID_TRUE )
    {
        IDE_TEST( smmManager::clearDirtyFlag4AllPages( (smmTBSNode*) aTBSNode )
                  != IDE_SUCCESS );
    }


    // Chunk확장시 모든 Tablespace의 Total Page Count를 세는데
    // 이 와중에 Page단계를 내려서는 안된다.
    // 이유 : Total Page Count를 세는 smpTBSAlterOnOff::alterTBSonline가
    //         들어있는 Alloc Page Count를 얻기위해 Membase를 접근한다.
    //         그런데, Membase는 Page단계를 내리면서 NULL로 변한다.
    IDE_TEST( smmFPLManager::lockGlobalPageCountCheckMutex() != IDE_SUCCESS );
    sPageCountLocked = ID_TRUE;
    
    /////////////////////////////////////////////////////////////////
    // Lock정보(STATE단계) 제외한 모든 객체 파괴, 메모리 반납
    IDE_TEST( smmTBSMultiPhase::finiToStatePhase( (smmTBSNode*) aTBSNode  )
              != IDE_SUCCESS );


    sPageCountLocked = ID_FALSE;
    IDE_TEST( smmFPLManager::unlockGlobalPageCountCheckMutex() != IDE_SUCCESS );
    
    
    // To Fix BUG-17258     [PROJ-1548-test] Drop Tablespace도중 사망할 경우
    //                      Checkpoint Image를 제거하지 않을 수 있음
    //
    // => Checkpoint Image제거 후에 Tablespace상태를 DROPPED로 설정한다.
    /////////////////////////////////////////////////////////////////////
    // - TBSNode.Status := Dropped
    // - flush TBSNode  (주2)
    IDE_TEST( flushTBSStatusDropped(aTBSNode) != IDE_SUCCESS );

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sPageCountLocked == ID_TRUE )
    {
        IDE_ASSERT( smmFPLManager::unlockGlobalPageCountCheckMutex()
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
}

/*
    Tablespace의 상태를 DROPPED로 설정하고 Log Anchor에 Flush!
 */
IDE_RC smmTBSDrop::flushTBSStatusDropped( sctTableSpaceNode * aSpaceNode )
{
    IDE_TEST( smmTBSDrop::flushTBSStatus( aSpaceNode,
                                          SMI_TBS_DROPPED )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/*
    Tablespace의 상태를 변경하고 Log Anchor에 Flush한다.
    
    aSpaceNode     [IN] Tablespace의 Node
    aNewSpaceState [IN] Tablespace의 새로운 상태
    
    [알고리즘]
     (010) latch TableSpace Manager
     (020) TBSNode.Status := 새로운 상태
     (030) flush TBSNode 
     (040) unlatch TableSpace Manager

   [ 참고 ]
     본 함수는 DROP TABLESPACE의 Commit Pending에서 호출된다.
     Commit Pending함수는 실패해서는 안되기 때문에
     빠른 에러 Detect를 위해 IDE_ASSERT로 에러처리를 실시하였다.
 */
IDE_RC smmTBSDrop::flushTBSStatus( sctTableSpaceNode * aSpaceNode,
                                   UInt                aNewSpaceState)
{
    /////////////////////////////////////////////////////////////////////
    // (010) latch TableSpace Manager
    //       다른 Tablespace가 변경되는 것을 Block하기 위함
    //       한번에 하나의 Tablespace만 변경/Flush한다.
    sctTableSpaceMgr::lockSpaceNode( NULL, aSpaceNode ) ;
    
    /////////////////////////////////////////////////////////////////////
    // (020) TBSNode.Status := 새로운 상태
    aSpaceNode->mState = aNewSpaceState;
    
    /////////////////////////////////////////////////////////////////////
    // (c-030) flush TBSNode  (주2)
    /* PROJ-2386 DR
     * standby 에서도 active와 동일한 시점에 loganchor를 남긴다. */
    IDE_ASSERT( smLayerCallback::updateTBSNodeAndFlush( aSpaceNode ) 
                == IDE_SUCCESS );


    /////////////////////////////////////////////////////////////////////
    // (040) unlatch TableSpace Manager
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    
    return IDE_SUCCESS;
}


