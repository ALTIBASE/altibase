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
* $Id: smpTBSAlterOnOff.cpp 19201 2006-11-30 00:54:40Z kmkim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <smm.h>
#include <smpReq.h>
#include <smrUpdate.h>
#include <sctTableSpaceMgr.h>
#include <smpTBSAlterOnOff.h>
#include <smmTBSMultiPhase.h>

/*
    smpTBSAlterOnOff.cpp

    smpTBSAlterOnOff 클래스의 함수중 
    다음 기능의 구현이 들어있는 파일이다.

    Alter TableSpace Offline
    Alter TableSpace Online
 */


/*
  생성자 (아무것도 안함)
*/
smpTBSAlterOnOff::smpTBSAlterOnOff()
{
    
}


/*
   Memory Tablespace에 대해 Alter Tablespace Online/Offline을 수행
    
   [IN] aTrans        - 상태를 변경하려는 Transaction
   [IN] aTableSpaceID - 상태를 변경하려는 Tablespace의 ID
   [IN] aState        - 새로 전이할 상태 ( Online or Offline )
 */
IDE_RC smpTBSAlterOnOff::alterTBSStatus( void       * aTrans,
                                         smmTBSNode * aTBSNode,
                                         UInt         aState )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSNode != NULL );
    
    switch ( aState )
    {
        case SMI_TBS_ONLINE :
            IDE_TEST( smpTBSAlterOnOff::alterTBSonline( aTrans,
                                                        aTBSNode )
                      != IDE_SUCCESS );
            break;

        case SMI_TBS_OFFLINE :
            IDE_TEST( smpTBSAlterOnOff::alterTBSoffline( aTrans,
                                                         aTBSNode )
                      != IDE_SUCCESS );
            break;
    }

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Alter Tablespace Online Offline 로그를 기록 

    [IN] aTrans          - 로깅하려는 Transaction 객체
    [IN] aTBSNode        - ALTER할 Tablespace의 Node
    [IN] aUpdateType     - Alter Tablespace Online인지 Offline인지 여부 
    [IN] aStateToRemove  - 제거할 상태 (변수 &= ~상태)
    [IN] aStateToAdd     - 추가할 상태 (변수 |=  상태)
    [OUT] aNewTBSState   - 새로 전이할 Tablespace의 상태

 */
IDE_RC smpTBSAlterOnOff::writeAlterTBSStateLog(
                             void                 * aTrans,
                             smmTBSNode           * aTBSNode,
                             sctUpdateType          aUpdateType,
                             smiTableSpaceState     aStateToRemove,
                             smiTableSpaceState     aStateToAdd,
                             UInt                 * aNewTBSState )
{

    IDE_DASSERT( aTrans != 0 );
    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aNewTBSState != NULL );
    
    UInt sBeforeState;
    UInt sAfterState;

    sBeforeState = aTBSNode->mHeader.mState ;

    // 로깅하기 전에 Backup관리자와의 동시성 제어를 위한 임시 Flag를 제거
    sBeforeState &= ~SMI_TBS_SWITCHING_TO_OFFLINE;
    sBeforeState &= ~SMI_TBS_SWITCHING_TO_ONLINE;

    sAfterState  = sBeforeState;
    sAfterState &= ~aStateToRemove;
    sAfterState |=  aStateToAdd;
        
    IDE_TEST( smrUpdate::writeTBSAlterOnOff(
                             NULL, /* idvSQL* */
                             aTrans,
                             aTBSNode->mHeader.mID,
                             aUpdateType,
                             // Before Image
                             sBeforeState,
                             // After Image
                             sAfterState, 
                             NULL )
              != IDE_SUCCESS );

    *aNewTBSState = sAfterState ;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}





/*
    META, SERVICE단계에서 Tablespace를 Offline상태로 변경한다.

    [IN] aTrans   - 상태를 변경하려는 Transaction
    [IN] aTBSNode - 상태를 변경할 Tablespace의 Node

    [ 알고리즘 ]
      (010) TBSNode에 X락 획득
      (020) Tablespace를 Backup중이라면 Backup종료시까지 대기 
      (030) "TBSNode.Status := Offline" 에 대한 로깅 
      (040) TBSNode.OfflineSCN := Current System SCN
      (050) Instant Memory Aging 실시 - Aging 수행중에만 잠시 Ager 래치획득
      (060) Checkpoint Latch 획득 
      (070)    Dirty Page Flush 실시
               ( Unstable DB 부터, 0번 1번 DB 모두 실시 )
      (080) Checkpoint Latch 해제
      (090) TBSNode에 Dirty Page가 없음을 ASSERT로 확인
      (100) Commit Pending등록
      
    [ Commit시 : (Pending) ]
      (c-010) TBSNode.Status := Offline
      (c-020) latch TBSNode.SyncMutex        // Checkpoint와 경합
      (c-030) unlatch TBSNode.SyncMutex
      (c-040) Free All Page Memory of TBS
      (c-050) Free All Index Memory of TBS
      (c-060) Free Runtime Info At Table Header
      (c-070) Free Runtime Info At TBSNode ( Expcet Lock )
      (c-080) flush TBSNode to loganchor

    [ Abort시 ]
      [ UNDO ] 수행 

    [ REDO ]
      (u-010) (020)에 대한 REDO로 TBSNode.Status := After Image(OFFLINE)
      (note-1) TBSNode를 loganchor에 flush하지 않음 
               -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문
      (note-2) Commit Pending을 수행하지 않음
               -> Restart Recovery완료후 OFFLINE TBS에 대한 Resource해제를 한다
      
    [ UNDO ]
      (u-010) (020)에 대한 UNDO로 TBSNode.Status := Before Image(ONLINE)
              TBSNode.Status가 Commit Pending에서 변경되기 때문에
              굳이 undo중에 Before Image로 덮어칠 필요는 없다.
              그러나 일관성을 유지하기 위해 TBSNode.Status를
              Before Image로 원복하도록 한다.      
      (note-1) TBSNode를 loganchor에 flush하지 않음
               -> ALTER TBS OFFLINE의 Commit Pending을 통해
                  COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문

*/
IDE_RC smpTBSAlterOnOff::alterTBSoffline( void       * aTrans,
                                          smmTBSNode * aTBSNode )
{
    idvSQL       * sStatistics ;
    sctPendingOp * sPendingOp;
    UInt           sNewTBSState = 0;
    UInt           sState = 0;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSNode != NULL );

    sStatistics = smxTrans::getStatistics( aTrans );

    ///////////////////////////////////////////////////////////////////////////
    //  (010) TBSNode에 X락 획득
    //
    // Tablespace가 Offline상태여도 에러를 내지 않는다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode( 
                                   aTrans,
                                   & aTBSNode->mHeader,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   SCT_VAL_ALTER_TBS_ONOFF) /* validation */
              != IDE_SUCCESS );
    
    ///////////////////////////////////////////////////////////////////////////
    //  (e-010) Tablespace상태에 따른 에러처리
    IDE_TEST( sctTableSpaceMgr::checkError4AlterStatus( 
                                     (sctTableSpaceNode*)aTBSNode,
                                     SMI_TBS_OFFLINE  /* New State */ )
              != IDE_SUCCESS );
    
    ///////////////////////////////////////////////////////////////////////////
    //  (020) Tablespace를 Backup중이라면 Backup종료시까지 대기 
    IDE_TEST( sctTableSpaceMgr::wait4BackupAndBlockBackup( sStatistics,
                                                           (sctTableSpaceNode*)aTBSNode,
                                                           SMI_TBS_SWITCHING_TO_OFFLINE )
              != IDE_SUCCESS );
    sState = 1;

    ///////////////////////////////////////////////////////////////////////////
    //  (030) "TBSNode.Status := Offline" 에 대한 로깅 
    //
    IDE_TEST( writeAlterTBSStateLog( aTrans,
                                     aTBSNode,
                                     SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE,
                                     SMI_TBS_ONLINE,  /* State To Remove */
                                     SMI_TBS_OFFLINE, /* State To Add */
                                     & sNewTBSState )
              != IDE_SUCCESS );
    
    ///////////////////////////////////////////////////////////////////////////
    //  (050) Instant Memory Aging 실시
    //        - Aging 수행중에만 잠시 Ager 래치획득
    //
    IDE_TEST( smLayerCallback::doInstantAgingWithMemTBS(
                  aTBSNode->mHeader.mID ) != IDE_SUCCESS );
    
    ///////////////////////////////////////////////////////////////////////////
    //  (060) Checkpoint Latch 획득 
    //
    //  (070)    Dirty Page Flush 실시
    //           ( Unstable DB 부터, 0번 1번 DB 모두 실시 )
    //  (080) Checkpoint Latch 해제
    //
    IDE_TEST( smrRecoveryMgr::flushDirtyPages4AllTBS()
              != IDE_SUCCESS );


    //////////////////////////////////////////////////////////////////////
    //  (090)   TBSNode에 Dirty Page가 없음을 ASSERT로 확인
    IDE_TEST( smrRecoveryMgr::assertNoDirtyPagesInTBS( aTBSNode )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (100) Commit Pending등록
    //
    // Transaction Commit시에 수행할 Pending Operation등록 
    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                  aTrans,
                  aTBSNode->mHeader.mID,
                  ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                  SCT_POP_ALTER_TBS_OFFLINE,
                  & sPendingOp )
              != IDE_SUCCESS );

    // Commit시 sctTableSpaceMgr::executePendingOperation에서
    // 수행할 Pending함수 설정
    sPendingOp->mPendingOpFunc = smpTBSAlterOnOff::alterOfflineCommitPending;
    sPendingOp->mNewTBSState   = sNewTBSState;
    sState = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch( sState)
    {
    case 1:
        // Tablespace의 상태에서 SMI_TBS_SWITCHING_TO_OFFLINE를 제거
        aTBSNode->mHeader.mState &= ~SMI_TBS_SWITCHING_TO_OFFLINE;
        break;

    default:
        break;
    }
    IDE_POP();

    return IDE_FAILURE;
    
}


/*
   Tablespace를 OFFLINE시킨 Tx가 Commit되었을 때 불리는 Pending함수
  
   PROJ-1548 User Memory Tablespace
 
   Tablespace와 관련된 모든 메모리와 리소스를 반납한다.
   - 예외 : Tablespace의 Lock정보는 다른 Tx들이 대기하면서
             참조할 수 있기 때문에 해제해서는 안된다.
  
   [참고] sctTableSpaceMgr::executePendingOperation 에서 호출된다.

   [ 알고리즘 ] ======================================================
      (c-010) TBSNode.Status := OFFLINE
      (c-020) latch TBSNode.SyncMutex        // Checkpoint와 경합
      (c-030) unlatch TBSNode.SyncMutex
      (c-040) Free All Index Memory of TBS
      (c-050) Destroy/Free Runtime Info At Table Header
      (c-060) Free All Page Memory of TBS ( 공유메모리인 경우 모두 제거 )
      (c-070) Free Runtime Info At TBSNode ( Expcet Lock, DB File Objects )
              - Offline된 Tablespace라도 Checkpoint시
                redo LSN을 DB File Header에 기록해야 하기 때문에
                DB File 객체들을 파괴하지 않는다.
      (c-080) flush TBSNode to loganchor

*/
IDE_RC smpTBSAlterOnOff::alterOfflineCommitPending(
                          idvSQL*             aStatistics, 
                          sctTableSpaceNode * aTBSNode,
                          sctPendingOp      * aPendingOp )
{
    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aPendingOp != NULL );

    idBool sPageCountLocked = ID_FALSE;
    
    // 여기 들어오는 Tablespace는 항상 Memory Tablespace여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isMemTableSpace( aTBSNode ) == ID_TRUE );

    // Commit Pending수행 이전에는 OFFLINE으로 가는 중이라고
    // Flag가 세팅되어 있어야 한다.
    IDE_ASSERT( ( aTBSNode->mState & SMI_TBS_SWITCHING_TO_OFFLINE )
                == SMI_TBS_SWITCHING_TO_OFFLINE );
    
    /////////////////////////////////////////////////////////////////////
    // (c-010) TBSNode.Status := OFFLINE
    aTBSNode->mState = aPendingOp->mNewTBSState;

    // SMI_TBS_SWITCHING_TO_OFFLINE 이 설정되어 있으면 안된다.
    IDE_ASSERT( ( aTBSNode->mState & SMI_TBS_SWITCHING_TO_OFFLINE )
                != SMI_TBS_SWITCHING_TO_OFFLINE );
    
    /////////////////////////////////////////////////////////////////////
    // (c-020) latch TBSNode.SyncMutex
    // (c-030) unlatch TBSNode.SyncMutex
    //
    // 다음 두 Thread간의 동시성 제어
    // - Alter TBS Offline Thread
    //    - 이 함수의 finiPagePhase에서 smmDirtyPageMgr을 제거.
    // - Checkpoint Thread
    //    - 이 TBS의 smmDirtyPageMgr을 Open하고 Read시도
    //      - 이 Tablespace는 OFFLINE되었으므로 Dirty Page는 없지만
    //         Checkpoint도중에 smmDirtyPageMgr에 대해
    //         Open/Read/Close작업이 수행된다
    //
    // 현재 Checkpoint가 smmDirtyPageMgr에 접근하고 하고 있다면
    // TBSNode.SyncMutex 를 잡고 있을 것이다. 
    // ==> Mutex를 잡았다가 풀어서 smmDirtyPageMgr로의 접근이 없음을
    //     확인한다.
    //
    if ( smrRecoveryMgr::isRestart() == ID_FALSE )
    {
        // 운영중에만 
        IDE_TEST( sctTableSpaceMgr::latchSyncMutex( aTBSNode )   
                  != IDE_SUCCESS );
        IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( aTBSNode ) 
                  != IDE_SUCCESS );

        /////////////////////////////////////////////////////////////////////
        //  (c-040) Free All Index Memory of TBS
        //  (c-050) Destroy/Free Runtime Info At Table Header
        IDE_TEST( smLayerCallback::alterTBSOffline4Tables( aStatistics,
                                                           aTBSNode->mID )
                  != IDE_SUCCESS );

        // Chunk확장시 모든 Tablespace의 Total Page Count를 세는데
        // 이 와중에 Page단계를 내려서는 안된다.
        // 이유 : Total Page Count를 세는
        //         smmFPLManager::getTotalPageCount4AllTBS가
        //         들어있는 Alloc Page Count를 얻기위해 Membase를 접근한다.
        //         그런데, Membase는 Page단계를 내리면서 NULL로 변한다.
        //
        // 현재 Tablespace는 DROP_PENDING상태이지만,
        // DROP_PENDING상태 설정 이전에 Total Page Counting이 시작되어
        // 본 Tablespace에 대한 Membase에 접근하고 있을 수도 있다.
        //
        // => Page단계로 내리는 동안 GlobalPageCountCheck Mutex를 획득.
        
        IDE_TEST( smmFPLManager::lockGlobalPageCountCheckMutex() != IDE_SUCCESS );
        sPageCountLocked = ID_TRUE;

        
        /////////////////////////////////////////////////////////////////////
        //  (c-060) Free All Page Memory of TBS( 공유메모리인 경우 모두 제거 )
        //  (c-070) Free Runtime Info At TBSNode ( Expcet Lock, DB File Objects )
        IDE_TEST( smmTBSMultiPhase::finiPagePhase( (smmTBSNode*) aTBSNode )
                != IDE_SUCCESS );


        sPageCountLocked = ID_FALSE;
        IDE_TEST( smmFPLManager::unlockGlobalPageCountCheckMutex() != IDE_SUCCESS );

        /////////////////////////////////////////////////////////////////////
        //  (c-080) flush TBSNode to loganchor
        IDE_TEST( smrRecoveryMgr::updateTBSNodeToAnchor( aTBSNode )
                != IDE_SUCCESS );
    }
    else
    {
       // restart recovery시에는 상태만 변경한다. 
    }

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
    META/SERVICE단계에서 Tablespace를 Online상태로 변경한다.

    [IN] aTrans   - 상태를 변경하려는 Transaction
    [IN] aTBSNode - 상태를 변경할 Tablespace의 Node

    [ 알고리즘 ]
      (010) TBSNode에 X락 획득
      (020) Tablespace를 Backup중이라면 Backup종료시까지 대기 
      (030) "TBSNode.Status := ONLINE"에 대한 로깅
      (040)  LogAnchor의 TBSNode.stableDB에 해당하는
             Checkpoint Image를 로드한다.
      (050) Table의 Runtime정보 초기화
            - Mutex 객체, Free Page관련 정보 초기화 실시
            - 모든 Page에 대해 Free Slot을 찾아서 각 Page에 달아준다.
            - 테이블 헤더의 Runtime정보에 Free Page들을 구축해준다.
      (060) 해당 TBS에 속한 모든 Table에 대해 Index Rebuilding 실시
      (070) Commit Pending등록
      (note-1) Log anchor에 TBSNode를 flush하지 않는다. (commit pending으로 처리)

    [ Commit시 ] (pending)
      (c-010) TBSNode.Status := ONLINE    (주1)
      (c-020) Flush TBSNode To LogAnchor


    [ Abort시 ]
      [ UNDO ] 수행
      
    [ REDO ]
      (r-010) (030)에 대한 REDO로 TBSNode.Status := After Image(ONLINE)
      (note-1) TBSNode를 loganchor에 flush하지 않음 
               -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문
      (note-2) Page Redo시 PAGE가 NULL이면 해당 TBS에서 Restore를 실시하므로
               (050)의 작업이 필요하지 않다.
      (note-3) Restart Recovery완료후 (060), (070)의 작업이 수행되므로
               Redo중에 이를 처리하지 않는다.
               
    [ UNDO ]
      if RestartRecovery
         // do nothing
      else
         (u-010) (070)에서 Index Rebuilding한 메모리 해제
         (u-020) (060)에서 초기화한 Table의 Runtime 정보 해제
         (u-030) (050)에서 Restore한 Page의 메모리 해제
         (u-040) (010)에서 할당한 TBS의 Runtime정보 해제
      fi
      (u-050) (020)에 대한 UNDO로 TBSNode.Status := Before Image(OFFLINE)
              -> TBSNode.Status가 Commit Pending에서 변경되기 때문에
                 굳이 undo중에 Before Image로 덮어칠 필요는 없다.
                 그러나 일관성을 유지하기 위해 TBSNode.Status를
                 Before Image로 원복하도록 한다.
                 
      (note-1) TBSNode를 loganchor에 flush하지 않음
               -> ALTER TBS ONLINEE의 Commit Pending을 통해
                  COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문
      
    [ 전제조건 ] 
       이 함수는 META, SERVICE단계에서 ONLINE으로 올릴 경우에만 호출된다.


    (주1) TBSNode.Status가 ONLINE이 되면 Checkpoint가 Dirty Page Flush를
          할 수 있게 된다. 그러므로 TBSNode.Status는 tablespace의
          모든 resource를 준비해놓고 commit pending으로 ONLINE으로 설정한다
       
 */
IDE_RC smpTBSAlterOnOff::alterTBSonline( void       * aTrans,
                                         smmTBSNode * aTBSNode )
{
    idvSQL       *   sStatistics;
    UInt             sStage = 0;
    sctPendingOp *   sPendingOp;
    UInt             sNewTBSState = 0;
    idBool           sPageCountLocked = ID_FALSE;
    scPageID         sTotalPageCount;
    ULong            sTBSCurrentSize;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSNode != NULL );

    sStatistics = smxTrans::getStatistics( aTrans );

    ///////////////////////////////////////////////////////////////////////////
    //  (010) TBSNode에 X락 획득
    //
    // Tablespace가 Offline상태여도 에러를 내지 않는다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode( 
                                   aTrans,
                                   & aTBSNode->mHeader,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   SCT_VAL_ALTER_TBS_ONOFF) /* validation */
              != IDE_SUCCESS );

    // To Fix BUG ; ALTER TBS ONLINE시 MEM_MAX_DB_SIZE 체크를 안함
    //
    // MEM_MAX_DB_SIZE체크를 위해 GlobalPageCountCheck Mutex를 잡는다.
    // GlobalPageCountCheck를 잡는 구간은 다음과 같다.
    //
    //   - 잡기 : Tablespace상태를 SMI_TBS_SWITCHING_TO_ONLINE 으로 설정직전
    //   - 풀기 : Membase를 통해 Tablespace에 할당된  Page Count 접근이
    //            가능하며, Page단계가 해제될 가능성이 없고,
    //            Membase의 데이터가 정상적인 값으로 유지됨을
    //            보장할수 있는 상태
    //
    //   - 이유 : Chunk확장시마다
    //             smmFPLManager::aggregateTotalPageCountAction 에서
    //            Tablespace의 상태가 SMI_TBS_SWITCHING_TO_ONLINE인 경우 
    //            Tablespace의 Membase로부터 읽어낸 Page수를
    //            DB의 Total Page수에 포함시키기 때문
    //            
    IDE_TEST( smmFPLManager::lockGlobalPageCountCheckMutex() != IDE_SUCCESS );
    sPageCountLocked = ID_TRUE;

    ///////////////////////////////////////////////////////////////////////////
    //  (e-010) Tablespace상태에 따른 에러처리
    IDE_TEST( sctTableSpaceMgr::checkError4AlterStatus( 
                                      (sctTableSpaceNode*)aTBSNode,
                                      SMI_TBS_ONLINE /* New State */ ) 
              != IDE_SUCCESS );
    

    ///////////////////////////////////////////////////////////////////////////
    //  (020) Tablespace를 Backup중이라면 Backup종료시까지 대기
    //
    //  상태에 SMI_TBS_SWITCHING_TO_ONLINE 를 추가한다.
    IDE_TEST( sctTableSpaceMgr::wait4BackupAndBlockBackup( sStatistics,
                                                           (sctTableSpaceNode*)aTBSNode,
                                                           SMI_TBS_SWITCHING_TO_ONLINE )
              != IDE_SUCCESS );
    sStage = 1;
    
    ///////////////////////////////////////////////////////////////////////////
    //  (030) "TBSNode.Status := ONLINE"에 대한 로깅
    //
    IDE_TEST( writeAlterTBSStateLog( aTrans,
                                     aTBSNode,
                                     SCT_UPDATE_MRDB_ALTER_TBS_ONLINE,
                                     SMI_TBS_OFFLINE,  /* State To Remove */
                                     SMI_TBS_ONLINE,   /* State To Add */
                                     & sNewTBSState )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (040)  Tablespace 초기화
    //       - Offline상태의 TBS는 STATE, MEDIA Phase까지 초기화된 상태이다
    IDE_TEST( smmTBSMultiPhase::initPagePhase( aTBSNode )
              != IDE_SUCCESS );
    sStage = 2;

    ///////////////////////////////////////////////////////////////////////////
    //  (040)  LogAnchor의 TBSNode.stableDB에 해당하는
    //         Checkpoint Image를 로드한다.
    //
    IDE_TEST( smmManager::prepareAndRestore( aTBSNode ) != IDE_SUCCESS );

    // 지금 ONLINE시키려는 Tablespace를 포함한 모든 Tablespace들의
    // 크기를 구한다.
    //
    // BUGBUG-1548 prepareAndRestore를 하기 전에 이 체크를 수행해야함
    //             이유 : prepareAndRestore에서 Page Memory를 사용함
    //             Membase가 제거되고 할당된 Page수를 Tablespace Node에서
    //             가져올 수 있게 되면 이와같이 처리가능
    /*
     * BUG- 35443 Add Property for Excepting SYS_TBS_MEM_DIC size from
     * MEM_MAX_DB_SIZE
     */
    if( smuProperty::getSeparateDicTBSSizeEnable() == ID_TRUE )
    {
        /* 
         * SYS_TBS_MEM_DIC는 TBS online/offline을 수행할수 없기 때문에
         * SYS_TBS_MEM_DIC를 제외한 TBS의 크기만 구하여 MEM_MAX_DB_SIZE 검사를
         * 수행한다. 
         * smiTableSpace::alterStatus()에서 SYS_TBS_MEM_DIC 인지 검사됨
         */
        IDE_TEST( smmFPLManager::getTotalPageCountExceptDicTBS( 
                                                            &sTotalPageCount )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( smmFPLManager::getTotalPageCount4AllTBS( 
                                                        &sTotalPageCount ) 
                  != IDE_SUCCESS );
    }

    // 현재 Tablespace에 할당된 Page Memory크기 : 에러발생시 사용할 값
    sTBSCurrentSize = 
        smmDatabase::getAllocPersPageCount( aTBSNode->mMemBase ) *
        SM_PAGE_SIZE;
    
    IDE_TEST_RAISE( sTotalPageCount >
                    ( smuProperty::getMaxDBSize() / SM_PAGE_SIZE ),
                    error_unable_to_alter_online_cuz_mem_max_db_size );
    
    
    ///////////////////////////////////////////////////////////////////////////
    //  (050) Table의 Runtime정보 초기화
    ///////////////////////////////////////////////////////////////////////////
    //  (060) 해당 TBS에 속한 모든 Table에 대해 Index Rebuilding 실시
    //
    IDE_TEST( smLayerCallback::alterTBSOnline4Tables( sStatistics,
                                                      aTrans,
                                                      aTBSNode->mHeader.mID )
              != IDE_SUCCESS );
    sStage = 3;
    
    ///////////////////////////////////////////////////////////////////////////
    //  (070) Commit Pending등록
    //
    // Transaction Commit시에 수행할 Pending Operation등록 
    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                              aTrans,
                              aTBSNode->mHeader.mID,
                              ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                              SCT_POP_ALTER_TBS_ONLINE,
                              & sPendingOp )
              != IDE_SUCCESS );
    // Commit시 sctTableSpaceMgr::executePendingOperation에서
    // 수행할 Pending함수 설정
    sPendingOp->mPendingOpFunc = smpTBSAlterOnOff::alterOnlineCommitPending;
    sPendingOp->mNewTBSState   = sNewTBSState;

    sStage = 0;

    sPageCountLocked = ID_FALSE;
    IDE_TEST( smmFPLManager::unlockGlobalPageCountCheckMutex()
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_unable_to_alter_online_cuz_mem_max_db_size );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_UNABLE_TO_ALTER_ONLINE_CUZ_MEM_MAX_DB_SIZE,
                     aTBSNode->mHeader.mName,
                     (ULong) (sTBSCurrentSize / 1024),
                     (ULong) (smuProperty::getMaxDBSize()/1024),
                     (ULong) ( ( (sTotalPageCount * SM_PAGE_SIZE ) -
                                 sTBSCurrentSize ) / 1024 )
                ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 3:
                //////////////////////////////////////////////////////////
                //  Free All Index Memory of TBS
                //  Destroy/Free Runtime Info At Table Header
                IDE_ASSERT( smLayerCallback::alterTBSOffline4Tables( sStatistics,
                                                                     aTBSNode->mHeader.mID )
                            == IDE_SUCCESS );
                
                
            case 2:
                //////////////////////////////////////////////////////////
                // initPagePhase및 prepareAndRestore에서
                // 할당, 초기화한 내용 해제
                IDE_ASSERT( smmTBSMultiPhase::finiPagePhase( aTBSNode )
                            == IDE_SUCCESS );

            case 1:
                // GlobalPageCountCheck Lock를 풀기 전에 
                // Tablespace의 상태에서 SMI_TBS_SWITCHING_TO_ONLINE를 제거
                // 이유1 : 현재 Page단계가 해제된 상태로 Membase는 NULL이고                //         더이상 Total Page Count를 세는데 이 Tablespace를
                //         포함 시키면 안되기 때문
                
                // 이유2 : ALTER ONLINE의 UNDO시 이 FLAG가 안들어 있으면
                //         에러처리 루틴에서 Tablespace를 ONLINE시키려다
                //         할당한 리소스를 모두 해제한것으로 간주하고
                //         리소스 UNDO를 하지 않음
                aTBSNode->mHeader.mState &= ~SMI_TBS_SWITCHING_TO_ONLINE;
                break;
        }

        
        if ( sPageCountLocked == ID_TRUE )
        {
        
            IDE_ASSERT( smmFPLManager::unlockGlobalPageCountCheckMutex()
                        == IDE_SUCCESS );
        }
    }
    IDE_POP();
    
    return IDE_FAILURE;
}



/*
   Tablespace를 ONLINE시킨 Tx가 Commit되었을 때 불리는 Pending함수
  
   PROJ-1548 User Memory Tablespace
 
  
   [참고] sctTableSpaceMgr::executePendingOperation 에서 호출된다.

   [ 알고리즘 ] ======================================================
      (c-010) TBSNode.Status := ONLINE
      (c-020) Flush TBSNode To LogAnchor

*/
IDE_RC smpTBSAlterOnOff::alterOnlineCommitPending(
                          idvSQL*             /* aStatistics */, 
                          sctTableSpaceNode * aTBSNode,
                          sctPendingOp      * aPendingOp )
{
    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aPendingOp != NULL );

    // 여기 들어오는 Tablespace는 항상 Memory Tablespace여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isMemTableSpace( aTBSNode ) == ID_TRUE );
    
    // Commit Pending수행 이전에는 ONLINE으로 가는 중이라고
    // Flag가 세팅되어 있어야 한다.
    IDE_ASSERT( ( aTBSNode->mState & SMI_TBS_SWITCHING_TO_ONLINE )
                == SMI_TBS_SWITCHING_TO_ONLINE );
    
    /////////////////////////////////////////////////////////////////////
    // (c-010) TBSNode.Status := ONLINE    
    aTBSNode->mState = aPendingOp->mNewTBSState;

    // SMI_TBS_SWITCHING_TO_OFFLINE 이 설정되어 있으면 안된다.
    IDE_ASSERT( ( aTBSNode->mState & SMI_TBS_SWITCHING_TO_ONLINE )
                != SMI_TBS_SWITCHING_TO_ONLINE );
    
    /////////////////////////////////////////////////////////////////////
    //  (c-020) flush TBSNode to loganchor
    if ( smrRecoveryMgr::isRestart() == ID_FALSE )
    {
        IDE_TEST( smrRecoveryMgr::updateTBSNodeToAnchor( aTBSNode )
                  != IDE_SUCCESS );
    }
    else
    {
        // restart recovery시에는 상태만 변경한다. 
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Restart REDO, UNDO끝난 후에 Offline 상태인 Tablespace의 메모리를 해제
 *
 * Restart REDO, UNDO중에는 Alter TBS Offline로그를 만나도
 * 상태만 OFFLINE으로 바꾸고 TBS의 Memory를 해제하지 않는다.
 *  => 이유 : OFFLINE, ONLINE 전이가 빈번하게 발생할 경우
 *            매번 restore하느라 Restart Recovery성능이 저하될 수 있음
 *      
 */
IDE_RC smpTBSAlterOnOff::finiOfflineTBS()
{
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                                  NULL, /* idvSQL* */
                                  finiOfflineTBSAction,
                                  NULL, /* Action Argument*/
                                  SCT_ACT_MODE_NONE ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * smpTBSAlterOnOff::destroyAllTBSNode를 위한 Action함수
 */
IDE_RC smpTBSAlterOnOff::finiOfflineTBSAction( idvSQL*          /* aStatistics */,
                                               sctTableSpaceNode * aTBSNode,
                                               void * /* aActionArg */ )
{ 
    IDE_DASSERT( aTBSNode != NULL );

    if ( sctTableSpaceMgr::isMemTableSpace( aTBSNode ) == ID_TRUE )
    {
        // Tablespace의 상태가 Offline인데
        if ( SMI_TBS_IS_OFFLINE(aTBSNode->mState) )
        {
            // Tablespace Page가 Load되어 있는 경우 
            if ( ((smmTBSNode*)aTBSNode)->mRestoreType !=
                 SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET )
            {

                // Alter Tablespace Offline의 과정
                //   - 모든 Dirty Page를 0번, 1번 DB FIle에 Flush
                //   - on commit pending
                //     - flush to loganchor(Tablespace.state := OFFLINE)
                //
                // 즉, Alter Tablespace Offline이 commit되었음을 의미하며
                // 이는 곧, Normal Processing시 모든 Dirty Page가
                // 이미 Disk에 내려가 있음을 의미한다.
                //
                // => Redo/Undo후 OFFLINE인 Tablespace에 대한
                //    Dirty Page를 모두 제거해도 무관하다.
                IDE_TEST( smmManager::clearDirtyFlag4AllPages(
                                         (smmTBSNode*) aTBSNode )
                          != IDE_SUCCESS );
                
                //  Free All Page Memory of TBS
                //      ( 공유메모리인 경우 모두 제거 )
                //  Free Runtime Info At TBSNode
                //      ( Expcet Lock, DB File Objects )
                IDE_TEST( smmTBSMultiPhase::finiPagePhase(
                              (smmTBSNode*) aTBSNode ) != IDE_SUCCESS );
                
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
