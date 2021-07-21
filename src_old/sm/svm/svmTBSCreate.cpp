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
#include <svmTBSCreate.h>
#include <svmTBSAlterAutoExtend.h>

/*
  생성자 (아무것도 안함)
*/
svmTBSCreate::svmTBSCreate()
{

}

/*
     Tablespace Attribute를 초기화 한다.

     [IN] aTBSAttr        - 초기화 될 Tablespace Attribute
     [IN] aName           - Tablespace가 생성될 Database의 이름
     [IN] aType           - Tablespace 종류
                            User||System, Memory, Data || Temp
     [IN] aAttrFlag       - Tablespace의 속성 Flag
     [IN] aInitSize       - Tablespace의 초기크기
     [IN] aState          - Tablespace의 상태

     [ 알고리즘 ]
       (010) 기본 필드 초기화
       (020) mSplitFilePageCount 초기화
       (030) mInitPageCount 초기화

     [ 에러처리 ]
       (e-010) aSplitFileSize 가 Expand Chunk크기의 배수가 아니면 에러
       (e-020) aInitSize 가 Expand Chunk크기의 배수가 아니면 에러

*/
IDE_RC svmTBSCreate::initializeTBSAttr( smiTableSpaceAttr    * aTBSAttr,
                                         smiTableSpaceType      aType,
                                         SChar                * aName,
                                         UInt                   aAttrFlag,
                                         ULong                  aInitSize,
                                         UInt                   aState)
{
    ULong sChunkSize = smuProperty::getExpandChunkPageCount() * SM_PAGE_SIZE;

    IDE_DASSERT( aTBSAttr != NULL );
    IDE_DASSERT( aName != NULL );

    IDE_ASSERT( sChunkSize > 0 );

    // Volatile Tablespace의 경우 Log Compression사용하겠다고 하면
    // 에러를 발생
    IDE_TEST_RAISE( (aAttrFlag & SMI_TBS_ATTR_LOG_COMPRESS_MASK)
                    == SMI_TBS_ATTR_LOG_COMPRESS_TRUE,
                    err_volatile_log_compress );

    idlOS::memset( aTBSAttr, 0, ID_SIZEOF( *aTBSAttr ) );

    aTBSAttr->mAttrType   = SMI_TBS_ATTR;
    aTBSAttr->mType       = aType;
    aTBSAttr->mAttrFlag   = aAttrFlag;

    idlOS::strncpy( aTBSAttr->mName, aName, SMI_MAX_TABLESPACE_NAME_LEN );
    aTBSAttr->mName[SMI_MAX_TABLESPACE_NAME_LEN] = '\0';

    // BUGBUG-1548 Refactoring mName이 '\0'으로 끝나는 문자열이므로
    //             mNameLength는 필요가 없다. 제거할것.
    aTBSAttr->mNameLength = idlOS::strlen( aName );
    aTBSAttr->mTBSStateOnLA      = aState;

    // 사용자가 Tablespace초기 크기를 지정하지 않은 경우
    if ( aInitSize == 0 )
    {
        // 최소 확장단위인 Chunk크기만큼 생성
        aInitSize = sChunkSize;
    }

    IDE_TEST_RAISE ( ( aInitSize % sChunkSize ) != 0,
                     error_tbs_init_size_not_aligned_to_chunk_size );

    aTBSAttr->mVolAttr.mInitPageCount = aInitSize / SM_PAGE_SIZE;

    return IDE_SUCCESS;


    IDE_EXCEPTION( err_volatile_log_compress );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG));
    }
    IDE_EXCEPTION( error_tbs_init_size_not_aligned_to_chunk_size );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TBSInitSizeNotAlignedToChunkSize,
                                // KB 단위의 Expand Chunk Page 크기
                                ( sChunkSize / 1024 )));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
     사용자 메모리 Tablespace를 생성한다.
     [IN] aTrans          - Tablespace를 생성하려는 Transaction
     [IN] aDBName         - Tablespace가 생성될 Database의 이름
     [IN] aType           - Tablespace 종류
                            User||System, Memory, Data || Temp
     [IN] aAttrFlag       - Tablespace의 속성 Flag
     [IN] aName           - Tablespace의 이름
     [IN] aInitSize       - Tablespace의 초기크기
                            Meta Page(0번 Page)는 포함하지 않는 크기이다.
     [IN] aIsAutoExtend   - Tablespace의 자동확장 여부
     [IN] aNextSize       - Tablespace의 자동확장 크기
     [IN] aMaxSize        - Tablespace의 최대크기
     [IN] aState          - Tablespace의 상태
     [OUT] aTBSID         - 생성한 Tablespace의 ID
 */
IDE_RC svmTBSCreate::createTBS( void                 * aTrans,
                                 SChar                * aDBName,
                                 SChar                * aTBSName,
                                 smiTableSpaceType      aType,
                                 UInt                   aAttrFlag,
                                 ULong                  aInitSize,
                                 idBool                 aIsAutoExtend,
                                 ULong                  aNextSize,
                                 ULong                  aMaxSize,
                                 UInt                   aState,
                                 scSpaceID            * aTBSID )
{
    smiTableSpaceAttr    sTBSAttr;
    svmTBSNode         * sCreatedTBSNode;


    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aDBName != NULL );
    IDE_DASSERT( aTBSName != NULL );
    // aChkptPathList는 사용자가 지정하지 않은 경우 NULL이다.
    // aSplitFileSize는 사용자가 지정하지 않은 경우 0이다.
    // aInitSize는 사용자가 지정하지 않은 경우 0이다.
    // aNextSize는 사용자가 지정하지 않은 경우 0이다.
    // aMaxSize는 사용자가 지정하지 않은 경우 0이다.

    IDE_TEST( initializeTBSAttr( &sTBSAttr,
                                 aType,
                                 aTBSName,
                                 aAttrFlag,
                                 aInitSize,
                                 aState) != IDE_SUCCESS );

    // BUGBUG-1548-M3 mInitPageCount만큼 TBS생성시 MEM_MAX_DB_SIZE안넘어서는지 체크
    // BUGBUG-1548-M3 mNextPageCount가 EXPAND_CHUNK_PAGE_COUNT의 배수인지 체크
    // BUGBUG-1548-M3 mIsAutoExtend에 따라 청크 자동확장여부 결정하기

    IDE_TEST( createTableSpaceInternal(aTrans,
                                       aDBName,
                                       &sTBSAttr,
                                       sTBSAttr.mVolAttr.mInitPageCount,
                                       &sCreatedTBSNode )
             != IDE_SUCCESS );

    IDE_TEST( svmTBSAlterAutoExtend::alterTBSsetAutoExtend(
                  aTrans,
                  sTBSAttr.mID,
                  aIsAutoExtend,
                  aNextSize,
                  aMaxSize ) != IDE_SUCCESS );

    if ( aTBSID != NULL )
    {
        *aTBSID = sTBSAttr.mID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   메모리 Tablespace를 생성한다.

   aTrans              [IN] Tablespace를 생성하려는 Transaction
   aDBName             [IN] Tablespace가 속하는 Database의 이름
   aTBSAttr            [IN] 생성하려는 Tablespace의 이름
   aChkptPathAttrList  [IN] Checkpoint Path Attribute의 List
   aCreatedTBSNode     [OUT] 생성한 Tablespace의 Node

[ PROJ-1548 User Memory Tablespace ]

Create Tablespace 전체 알고리즘 =============================================

* Tablespace Attribute, Checkpoint Path에 대한
  Restart Recovery는 Disk Tablespace와 기본적으로 동일하게 처리한다.
  ( Commit전에 Log Anchor에 Flush해두고
    Redo시에 상태 변경을 수행하는 Pending연산만 달아준다. )

(010) Tablespace ID할당

(015) 로깅실시 => CREATE_TBS

(020) allocate TBSNode
(030) Lock TBSNode                 // Create중인 TBS에 DML/DDL을 막음
(050) TBS List에 TBSNode를 추가

(060) Checkpoint Path Node들을 생성
(070) TBSNode와 관련된 모든 리소스 초기화
(080) create Checkpoint Image Files (init size만큼)

BUGBUG-1548 같은 이름의 Checkpoint Image파일 존재시 에러처리
=> Forward Processing시, Restart Redo시로 구분하여 고려

(090) initialize PCHArray
(100) initialize Membase on Page 0
(110) alloc ExpandChunks

(120) flush Page 0 to Checkpoint Image (주5)

(130) Log Anchor Flush   // Commit 로그만 기록되고 Pending처리 안될 경우
                         // 대비하여 Log Anchor에
                         // CREATE중인 Tablespace를 Flush

(140) Commit Pending등록


- commit : (pending)
           (c-010) Status := Online(주2)
           (c-020) flush TBSNode (주3)
           <commit 완료>

- Restart Redo시 Commit Pending => Forward Processing시와 마찬가지로 처리

- CREATE_TBS 에 대한 REDO :
  - 아무런 처리도 하지 않음
  - 이유 : Create Tablespace시 Commit Pending으로 Tablespace관련 정보를
           모두 Log Anchor에 Flush했기 때문

- CREATE_TBS 에 대한 UNDO :
  - Forward Processing중 Abort발생시 호출
  - Restart Undo시 호출
  - 알고리즘
           (a-010) (로깅실시-CLR-CREATETBS-ABORT)
           (a-010) TBSID 에 해당하는 TBSNode를 찾음
           if ( TBSNode == NULL ) // TBSNode가 없음
             // Do Nothing ( Create 에 대한 Undo로 아무것도 할 필요 없음 )
           else // TBSNode가 존재
             (a-020) latch TBSNode.SyncMutex
             (a-030)   Status := DROPPED       // TBSNode상태를 DROPPED로 설정
             (a-040) unlatch TBSNode.SyncMutex  // 이후로는 Checkpoint를 안함
             (a-050) close all Checkpoint Image Files
             (a-060) delete Checkpoint Image Files
             (a-070) Lock정보 제외한 모든 객체 파괴, 메모리 반납 (주4)
             (a-080) flush TBSNode
           fi
           <abort 완료>

- CLR-CREATETBS-ABORT에 대한 REDO :
  - 메모리나 파일이 존재하는 경우에만 메모리 반납, 파일삭제 실시
    - 이유 : 이미 CLR에 해당하는 내용이 REDO되었을 수 있기 때문.
           if ( TBSNode 존재 )
              (clr-010) Status := DROPPED
              (clr-020) Lock정보 제외한 모든 객체 파괴, 메모리 반납 (주4)
              (clr-030) TBS와 관련된 파일이 있으면 제거
           else
              // Tablespace Node가 List에서 제거된 경우이며,
              // 아무것도 할 필요 없다
           fi

- checkpoint :
  - Creating중인 TBS에 대해서 Checkpoint를 허용한다.
      => (080)에서 Checkpoint Image파일 먼저 생성하고
         (100), (110)에서 Dirty Page 추가하기 때문에
         Checkpoint가 발생하여 Dirty Page Flush되어도 문제가 없다.
         단, UNDO도중 Checkpoint Image를 지우기 전에
         Sync Mutex를 잡아서 Checkpoint와의 경합을 막는다.

  - Drop된, 혹은 Create Tablespace가 UNDO되어 DROP상태로 된
    Tablespace에 대해 Checkpoint를 수행하지 않는다.
      => Checkpoint전에 TBSNode의 SyncMutex를 잡고 상태검사 실시
           latch TBSNode.SyncMutex
              if ( TBSNode.Status & DROPPED )
              {
                 // Do Nothing
              }
              else
              {
                 Checkpoint(Dirty Page Flush) 실시
              }
           unlatch TBSNode.SyncMutex


- (주1) : TBS에 X락을 잡아서 TBS생성하는 TX가 COMMIT하기 전에
         아무도 TBS에 접근하지 못하도록 막아준다.

- (주2) : TBSNode의 Status를 Creating에서 Online으로 변경해준다.

- (주3) : Commit Pending으로 Log Anchor가 Flush되고 나서
          Transction의 Commit이 완료된다.
          이후
          TBSNode를 commit이전에 Log Anchor에 바로 Flush해도 무관하다.
          단, 이 때 abort시 TBSNode를 Log Anchor에서 지워주는 추가작업이 필요하다.
- (주4) : 해당 TBSNode의 Lock을 대기하는 다른 Tx가 있을 수 있기 때문에,
          TBSNode자체를 TBS List에서 제거할 수 없다.
          ((가)Create Tablespace TBS1이 Commit되기 전에
           (나)Create Table In TBS1이 들어온 경우,
           (나)는 TBS1의 Lock을 대기한다. )

- (주5) : Membase가 들어있는 0번 Page는 Restart Recovery이전인,
          Prepare/Restore DB시에 읽어들여서 몇 개의 File을
          Restore할 지를 결정하게 된다.
          Restrart Redo가 되지 않은 상태에서도 이 값이
          Create Tablespace당시의 값임을 보장하기 위해
          0번 Page를 Flush한다.

          BUGBUG-주석더기술

- Latch간 Deadlock회피를 위한 Latch잡는 순서

    1. sctTableSpaceMgr::lock()           // TBS LIST
    2. sctTableSpaceMgr::latchSyncMutex() // TBS NODE
    3. svmPCH.mPageMemMutex.lock()        // PAGE

- Lock과 Latch간의 Deadlock회피를 위한 Lock/Latch잡는 순서
    1. Lock를  먼저 잡는다.
    2. Latch를 나중에 잡는다.

*/
IDE_RC svmTBSCreate::createTableSpaceInternal(
                          void                 * aTrans,
                          SChar                * aDBName,
                          smiTableSpaceAttr    * aTBSAttr,
                          scPageID               aCreatePageCount,
                          svmTBSNode          ** aCreatedTBSNode )
{
    svmTBSNode   * sNewTBSNode = NULL;
    idBool         sIsCreateLatched = ID_FALSE;
    sctPendingOp * sPendingOp;

    IDE_DASSERT( aTBSAttr->mName[0] != '\0' );
    IDE_DASSERT( aCreatePageCount > 0 );
    IDE_DASSERT( aTBSAttr->mAttrType == SMI_TBS_ATTR );

    IDE_TEST_RAISE(idlOS::strlen(aTBSAttr->mName) > (SM_MAX_DB_NAME -1),
                   too_long_tbsname);

    ///////////////////////////////////////////////////////////////
    // (010) Tablespace ID할당
    IDE_TEST( sctTableSpaceMgr::lockForCrtTBS() != IDE_SUCCESS );
    sIsCreateLatched = ID_TRUE;

    IDE_TEST( sctTableSpaceMgr::allocNewTableSpaceID(&aTBSAttr->mID)
              != IDE_SUCCESS );

    sIsCreateLatched = ID_FALSE;
    IDE_TEST( sctTableSpaceMgr::unlockForCrtTBS() != IDE_SUCCESS );


    ///////////////////////////////////////////////////////////////
    // (015) 로깅실시 => CREATE_TBS
    IDE_TEST( smLayerCallback::writeVolatileTBSCreate( NULL, /* idvSQL */
                                                       aTrans,
                                                       aTBSAttr->mID )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (020) allocate TBSNode
    // (030) Lock TBSNode             // DML/DDL과 Create중인 TBS간의 경합
    // (040) Latch TBSNode.SyncMutex // Checkpoint와 Create중인 TBS간의 경합
    // (050) TBS List에 TBSNode를 추가

    // createAndLockTBSNode에서
    // TBSNode->mTBSAttr.mState 가 CREATING | ONLINE 이 되도록 한다.
    // Commit시 Pending으로 CREATING을 빼주고,
    // Abort시 writeVolatileTBSLog에 대한 UNDO로 상태를 DROPPED로 설정한다.

    // createAndLockTBSNode에서 TBSNode.mState에 TBSAttr.mTBSStateOnLA를 복사
    //
    //
    // - Volatile Tablespace의 경우
    //   SMI_TBS_INCONSISTENT상태가 필요하지 않다.
    //   - 이유 : Membase, Checkpoint Image파일 모두 없음
    //   -         Tablespace가 생성되는 도중에
    //             일시적으로 INCONSISTENT한 상태가 되는 일이 없음
    aTBSAttr->mTBSStateOnLA = SMI_TBS_CREATING | SMI_TBS_ONLINE;

    // Tablespace Node를 생성하여 TBS Node List에 추가한다.
    // - TBSNode에 계층 Lock을 X로 잡는다. ( 일반 DML,DDL과 경합 )
    // - Sync Mutex에 Latch를 획득한다. ( Checkpoint와 경합 )
    IDE_TEST( createAndLockTBSNode(aTrans,
                                   aTBSAttr)
              != IDE_SUCCESS );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTBSAttr->mID,
                                                        (void**)&sNewTBSNode )
              != IDE_SUCCESS );

    // (080) initialize PCHArray
    // (100) alloc ExpandChunks
    IDE_TEST( svmManager::createTBSPages(
                  sNewTBSNode,
                  aDBName,
                  aCreatePageCount )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (110) Log Anchor Flush
    //
    // Commit 로그만 기록되고 Pending처리 안될 경우에 대비하여
    // Log Anchor에 CREATE중인 Tablespace를 Flush
    IDE_TEST( svmTBSCreate::flushTBSNode(sNewTBSNode) != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (120) Commit Pending등록
    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                  aTrans,
                  sNewTBSNode->mHeader.mID,
                  ID_TRUE, /* commit시에 동작 */
                  SCT_POP_CREATE_TBS,
                  & sPendingOp)
              != IDE_SUCCESS );

    sPendingOp->mPendingOpFunc = svmTBSCreate::createTableSpacePending;

    *aCreatedTBSNode = sNewTBSNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION(too_long_tbsname);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_TooLongTBSName,
                                  (UInt)SM_MAX_DB_NAME ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsCreateLatched == ID_TRUE )
    {
        IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );
    }

    // (030)에서 획득한 Tablespace X Lock은 UNDO완료후 자동으로 풀게된다
    // 여기서 별도 처리할 필요 없음

    IDE_POP();

    return IDE_FAILURE;
}
/*
   Tablespace를 Create한 Tx가 Commit되었을 때 불리는 Pending함수

   [ 알고리즘 ]
     (010) latch TableSpace Manager
     (020) TBSNode.State 에서 CREATING상태 제거
     (030) TBSNode를 Log Anchor에 Flush
     (040) unlatch TableSpace Manager
   [ 참고 ]
     Commit Pending함수는 실패해서는 안되기 때문에
     빠른 에러 Detect를 위해 IDE_ASSERT로 에러처리를 실시하였다.
 */

IDE_RC svmTBSCreate::createTableSpacePending( idvSQL*             aStatistics,
                                              sctTableSpaceNode * aSpaceNode,
                                              sctPendingOp      * /*aPendingOp*/ )
{
    IDE_DASSERT( aSpaceNode != NULL );

    /////////////////////////////////////////////////////////////////////
    // (010) latch TableSpace Manager
    //       다른 Tablespace가 변경되는 것을 Block하기 위함
    //       한번에 하나의 Tablespace만 변경/Flush한다.

    sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                     aSpaceNode );

    ///////////////////////////////////////////////////////////////
    // (020) TBSNode.State 에서 CREATING상태 제거

    IDE_ASSERT( SMI_TBS_IS_ONLINE(aSpaceNode->mState) );
    IDE_ASSERT( SMI_TBS_IS_CREATING(aSpaceNode->mState) );
    aSpaceNode->mState &= ~SMI_TBS_CREATING;

    ///////////////////////////////////////////////////////////////
    // (030) TBSNode를 Log Anchor에 Flush
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


/*
    svmTBSNode를 생성하여 sctTableSpaceMgr에 등록한다.

    [ Lock / Latch간의 deadlock 회피 ]
    - Lock을 먼저 잡고 Latch를 나중에 잡아서 Deadlock을 회피한다.
    - 이 함수가 불리기 전에 latch가 잡힌 상태여서는 안된다.


    [IN] aTBSAttr    - 생성할 Tablespace Node의 속성
    [IN] aOp         - Tablespace Node생성시의 Option
    [IN] aIsOnCreate - create tablespace 연산의 경우 ID_TRUE
    [IN] aTrans      - Tablespace 에 계층 Lock을 잡고자 하는 경우
                       Lock을 잡을 Transaction
*/
IDE_RC svmTBSCreate::createAndLockTBSNode(void              *aTrans,
                                           smiTableSpaceAttr *aTBSAttr)
{
    UInt                 sStage = 0;
    svmTBSNode         * sTBSNode = NULL;
    sctLockHier          sLockHier = { NULL, NULL };
    idBool               sIsLocked = ID_FALSE;


    IDE_DASSERT( aTBSAttr != NULL );
    IDE_DASSERT( aTBSAttr->mAttrType == SMI_TBS_ATTR );

    // sTBSNode를 초기화 한다.
    IDE_TEST(svmManager::allocTBSNode(&sTBSNode, aTBSAttr)
             != IDE_SUCCESS);

    sStage = 1;

    // Volatile Tablespace를 초기화한다.
    IDE_TEST(svmManager::initTBS(sTBSNode) != IDE_SUCCESS);

    sStage = 2;

    // Lock획득시 Lock Slot을 sLockHier->mTBSNodeSlot에 넘겨준다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                  aTrans,
                  & sTBSNode->mHeader,
                  ID_FALSE,   /* intent */
                  ID_TRUE,    /* exclusive */
                  ID_ULONG_MAX, /* lock wait micro sec */
                  SCT_VAL_DDL_DML, /* validation */
                  NULL,       /* is locked */
                  & sLockHier )      /* lockslot */
              != IDE_SUCCESS );

    // register it.
    /* 1. 다른 TBS를 모두 같이 확인하므로 SpaceNode Lock으로 보호가 안된다.
     * 2. 나는 아직 생성 되지 않았으므로 SpaceNode Lock은 잡지 않아도 된다.
     * 3. checkExistSpaceNodeByName() 는 SpaceNodeArray 에서 확인한다.
     *    내가 동일한 이름이 없음을 확인하고 SpaceNodeArray에 추가 할 때 까지
     *    Lock으로 보호 되어야 한다.
     */
    IDE_TEST( sctTableSpaceMgr::lockForCrtTBS() != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    /* 동일한 tablespace명이 존재하는지 검사한다. */
    // BUG-26695 TBS Node가 없는것이 정상이므로 없을 경우 오류 메시지를 반환하지 않도록 수정
    IDE_TEST_RAISE( sctTableSpaceMgr::checkExistSpaceNodeByName( sTBSNode->mHeader.mName )
                    == ID_TRUE, err_already_exist_tablespace_name );

    sctTableSpaceMgr::addTableSpaceNode( (sctTableSpaceNode*)sTBSNode );
    sStage = 3;

    sIsLocked = ID_FALSE;
    IDE_TEST( sctTableSpaceMgr::unlockForCrtTBS() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_already_exist_tablespace_name )
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistTableSpaceName,
                                sTBSNode->mHeader.mName));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sLockHier.mTBSNodeSlot != NULL )
    {
        IDE_ASSERT( smLayerCallback::unlockItem( aTrans,
                                                 sLockHier.mTBSNodeSlot )
                    == IDE_SUCCESS );
    }

    switch(sStage)
    {
        case 3:
            sctTableSpaceMgr::removeTableSpaceNode(
                                  (sctTableSpaceNode*)sTBSNode );

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
            IDE_ASSERT(svmManager::destroyTBSNode(sTBSNode) == IDE_SUCCESS);
        default:
           break;
    }

    if ( sIsLocked == ID_TRUE )
    {
        IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
    Tablespace Node를 Log Anchor에 flush한다.

    [IN] aTBSNode - Flush하려는 Tablespace Node
 */
IDE_RC svmTBSCreate::flushTBSNode(svmTBSNode * aTBSNode)
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smLayerCallback::addTBSNodeAndFlush( (sctTableSpaceNode*)aTBSNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


