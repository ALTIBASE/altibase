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
#include <svmTBSDrop.h>

/*
  생성자 (아무것도 안함)
*/
svmTBSDrop::svmTBSDrop()
{

}


/*
    사용자가 생성한 메모리 Tablespace를 drop한다.
 */
IDE_RC svmTBSDrop::dropTBS(void       * aTrans,
                              svmTBSNode * aTBSNode)
                              
{
    // aStatistics는 NULL로 들어온다.
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSNode != NULL );
    
    IDE_TEST( dropTableSpace( aTrans, aTBSNode ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
  Volatile 테이블 스페이스를 DROP한다.
  
[ PROJ-1548 User Memory Tablespace ]

Drop Tablespace 전체 알고리즘 ================================================

* Restart Recovery는 Disk Tablespace와 기본적으로 동일하게 처리한다.

(010) lock TBSNode in X

(020) DROP-TBS-LOG 로깅실시

(030) TBSNode.Status |= Dropping

(040) Drop Tablespace Pending 등록

- commit : ( pending 처리 )
           자세한 알고리즘은 svmTBSDrop::dropTableSpacePending을 참고

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
IDE_RC svmTBSDrop::dropTableSpace(void       * aTrans,
                                     svmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSNode != NULL );

    sctPendingOp * sPendingOp;

    ////////////////////////////////////////////////////////////////
    // (010) lock TBSNode in X
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                                   aTrans,
                                   &aTBSNode->mHeader,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   ID_ULONG_MAX, /* lock wait micro sec */
                                   SCT_VAL_DROP_TBS,
                                   NULL,       /* is locked */
                                   NULL )      /* lockslot */
              != IDE_SUCCESS );


    ////////////////////////////////////////////////////////////////
    // (020) DROP-TBS-LOG 로깅실시
    IDE_TEST( smLayerCallback::writeVolatileTBSDrop( NULL, /* idvSQL* */
                                                     aTrans,
                                                     aTBSNode->mHeader.mID )
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
    sPendingOp->mPendingOpFunc = svmTBSDrop::dropTableSpacePending;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   Tablespace를 DROP한 Tx가 Commit되었을 때 불리는 Pending함수
  
   PROJ-1548 User Memory Tablespace
 
   Tablespace와 관련된 모든 메모리와 리소스를 반납한다.
   - 예외 : Tablespace의 Lock정보는 다른 Tx들이 대기하면서
             참조할 수 있기 때문에 해제해서는 안된다.

   [참고] sctTableSpaceMgr::executePendingOperation 에서 호출된다.

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
   
   (c-090) Lock정보 제외한 모든 객체 파괴, 메모리 반납

   (c-100) Runtime정보 갱신 => Tablespace의 수 Counting 

   [ 참고 ]
     Commit Pending함수는 실패해서는 안되기 때문에
     빠른 에러 Detect를 위해 IDE_ASSERT로 에러처리를 실시하였다.
             
   

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
IDE_RC svmTBSDrop::dropTableSpacePending( idvSQL*             /*aStatistics*/,
                                          sctTableSpaceNode * aTBSNode,
                                          sctPendingOp      * /*aPendingOp*/ )
{
    IDE_DASSERT( aTBSNode   != NULL );

    // 여기 들어오는 Tablespace는 항상 Volatile Tablespace여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isVolatileTableSpace( aTBSNode->mID ) == ID_TRUE );
    // Tablespace의 상태를 DROPPED로 상태변경
    //
    // To Fix BUG-17323 존재하지 않는 Checkpoint Path지정하여
    //                  Tablespace생성 실패후 Log Anchor에
    //                  Log File Group Count가 0이 되어버림
    //
    // => Tablespace Node가 아직 Log Anchor에 기록되지 않은 경우
    //    Node상의 상태만 변경해준다.
    //    Log Anchor에 한번이라도 기록이 되면
    //    sSpaceNode->mAnchorOffset 가 0본다 큰 값을 가지게 된다.
    if ( ((svmTBSNode*)aTBSNode)->mAnchorOffset > 0 )  
    {
        /////////////////////////////////////////////////////////////////////
        // (c-010) TBSNode.Status := Dropped
        // (c-020) flush TBSNode  (주2)
        IDE_ASSERT( flushTBSStatusDropped(aTBSNode) == IDE_SUCCESS );
    }
    else
    {
        // Log Anchor에 한번도 기록되지 않는 경우
        aTBSNode->mState = SMI_TBS_DROPPED;
    }

    /////////////////////////////////////////////////////////////////////
    // (c-030) latch TBSNode.SyncMutex 
    // (c-040) unlatch TBSNode.SyncMutex
    //
    // 현재 Checkpoint가 Dirty Page Flush를 하고 있다면 TBSNode.SyncMutex
    // 를 잡고 있을 것이다. 
    // ==> Mutex를 잡았다가 풀어서 Dirty Page Flush가 종료되기를 기다린다.
    //
    // unlatch후 Checkpoint가 발생하면 TBSNode의 상태가 DROPPED여서
    // Dirty Page Flush를 실시하지 않고 Skip하게 된다.
    IDE_ASSERT( sctTableSpaceMgr::latchSyncMutex( aTBSNode )
                == IDE_SUCCESS );
    
    IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex( aTBSNode )
                == IDE_SUCCESS );


    /////////////////////////////////////////////////////////////////////
    // (c-050) Memory Garbage Collector를 Block한다.
    // (c-060) Memory Garbage Collector를 Unblock한다.
    //
    // 현재 Ager가 수행중이라면 blockMemGC에서 Ager의 수행이 완료되기를
    // 기다를 것이다.
    //
    // ==> Mutex를 잡았다가 풀어서 Ager가 한번 다 돌때까지 기다린다.
    //
    // unblock후 Ager가 수행되면 하면 TBSNode의 상태가 DROPPED여서
    // Aging을 실시하지 않고 Skip하게 된다.
    
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

    /* BUG-39806 Valgrind Warning
     * - DROPPED 상태로 먼저 변경하였기 때문에, 검사하지 않고 svmManager::finiTB
     *   S()를 호출합니다.
     */
    /////////////////////////////////////////////////////////////////
    // (c-090) Lock정보 제외한 모든 객체 파괴, 메모리 반납 (주4)
    //
    // 페이지 메모리 반납
    IDE_ASSERT( svmManager::finiTBS((svmTBSNode*)aTBSNode)
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/*
    Tablespace의 상태를 DROPPED로 설정하고 Log Anchor에 Flush!

    [알고리즘]
     (010) latch TableSpace Manager
     (020) TBSNode.Status := Dropped
     (030) flush TBSNode 
     (040) unlatch TableSpace Manager

   [ 참고 ]
     본 함수는 DROP TABLESPACE의 Commit Pending에서 호출된다.
     Commit Pending함수는 실패해서는 안되기 때문에
     빠른 에러 Detect를 위해 IDE_ASSERT로 에러처리를 실시하였다.
 */
IDE_RC svmTBSDrop::flushTBSStatusDropped( sctTableSpaceNode * aSpaceNode )
{
    /////////////////////////////////////////////////////////////////////
    // (010) latch TableSpace Manager
    //       다른 Tablespace가 변경되는 것을 Block하기 위함
    //       한번에 하나의 Tablespace만 변경/Flush한다.
    
    sctTableSpaceMgr::lockSpaceNode( NULL, aSpaceNode );
    
    /////////////////////////////////////////////////////////////////////
    // (020) TBSNode.Status := Dropped
    IDE_ASSERT( SMI_TBS_IS_ONLINE(aSpaceNode->mState) );
    IDE_ASSERT( SMI_TBS_IS_DROPPING(aSpaceNode->mState) );
    
    aSpaceNode->mState = SMI_TBS_DROPPED;
    
    /////////////////////////////////////////////////////////////////////
    // (c-030) flush TBSNode  (주2)
    if ( smLayerCallback::isRestart() != ID_TRUE )
    {
        IDE_ASSERT( smLayerCallback::updateTBSNodeAndFlush( aSpaceNode ) 
                    == IDE_SUCCESS );
    }
    else
    {
        // 서버구동시에는 RECOVERY 이후에 한번만 Loganchor 갱신한다.
    }

    /////////////////////////////////////////////////////////////////////
    // (040) unlatch TableSpace Manager
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    
    return IDE_SUCCESS;
}

