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
* $Id: smmTBSCreate.cpp 19201 2006-11-30 00:54:40Z kmkim $
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
#include <smmTBSCreate.h>
#include <smmTBSMultiPhase.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSCreate::smmTBSCreate()
{

}


/*
     Tablespace Attribute를 초기화 한다.

     [IN] aTBSAttr        - 초기화 될 Tablespace Attribute
     [IN] aSpaceID        - 새로 생성할 Tablespace가 사용할 Space ID
     [IN] aType           - Tablespace 종류
                            User||System, Memory, Data || Temp
     [IN] aName           - Tablespace의 이름
     [IN] aAttrFlag       - Tablespace의 속성 Flag
     [IN] aSplitFileSize  - Checkpoint Image File의 크기
     [IN] aInitSize       - Tablespace의 초기크기

     [ 알고리즘 ]
       (010) 기본 필드 초기화
       (020) mSplitFilePageCount 초기화
       (030) mInitPageCount 초기화

     [ 에러처리 ]
       (e-010) aSplitFileSize 가 Expand Chunk크기의 배수가 아니면 에러
       (e-020) aInitSize 가 Expand Chunk크기의 배수가 아니면 에러

*/
IDE_RC smmTBSCreate::initializeTBSAttr( smiTableSpaceAttr    * aTBSAttr,
                                         scSpaceID              aSpaceID,
                                         smiTableSpaceType      aType,
                                         SChar                * aName,
                                         UInt                   aAttrFlag,
                                         ULong                  aSplitFileSize,
                                         ULong                  aInitSize)
{
    ULong sChunkSize = smuProperty::getExpandChunkPageCount() * SM_PAGE_SIZE;

    IDE_DASSERT( aTBSAttr != NULL );
    IDE_DASSERT( aName != NULL );

    // checkErrorOfTBSAttr 에서 이미 체크한 내용들이므로 ASSERT로 검사
    IDE_ASSERT ( ( aSplitFileSize % sChunkSize ) == 0 );
    IDE_ASSERT ( ( aInitSize % sChunkSize ) == 0 );

    IDE_ASSERT( sChunkSize > 0 );


    idlOS::memset( aTBSAttr, 0, ID_SIZEOF( *aTBSAttr ) );

    aTBSAttr->mMemAttr.mShmKey             = 0;
    aTBSAttr->mMemAttr.mCurrentDB          = 0;

    aTBSAttr->mAttrType                    = SMI_TBS_ATTR;
    aTBSAttr->mID                          = aSpaceID;
    aTBSAttr->mType                        = aType;
    aTBSAttr->mAttrFlag                    = aAttrFlag;
    aTBSAttr->mMemAttr.mSplitFilePageCount = aSplitFileSize / SM_PAGE_SIZE ;
    aTBSAttr->mMemAttr.mInitPageCount      = aInitSize / SM_PAGE_SIZE;
    // Membase를 저장할 Page만큼 추가
    aTBSAttr->mMemAttr.mInitPageCount     += SMM_DATABASE_META_PAGE_CNT ;

    idlOS::strncpy( aTBSAttr->mName,
                    aName,
                    SMI_MAX_TABLESPACE_NAME_LEN );
    aTBSAttr->mName[SMI_MAX_TABLESPACE_NAME_LEN] = '\0';

    // BUGBUG-1548 Refactoring mName이 '\0'으로 끝나는 문자열이므로
    //             mNameLength는 필요가 없다. 제거할것.
    aTBSAttr->mNameLength                  = idlOS::strlen( aName );

    return IDE_SUCCESS;
}

/*
    사용자가 Tablespace생성을 위한 옵션을 지정하지 않은 경우
    기본값을 설정하는 함수

     [IN/OUT] aSplitFileSize  - Checkpoint Image File의 크기
     [IN/OUT] aInitSize       - Tablespace의 초기크기
 */
IDE_RC smmTBSCreate::makeDefaultArguments( ULong  * aSplitFileSize,
                                           ULong  * aInitSize)
{
    ULong sChunkSize = smuProperty::getExpandChunkPageCount() * SM_PAGE_SIZE;

    // 사용자가 Checkpoint Image Split크기(DB File크기)를
    // 지정하지 않은 경우
    if ( *aSplitFileSize == 0 )
    {
        // 프로퍼티로 지정한 기본 DB File크기로 설정
        *aSplitFileSize = smuProperty::getDefaultMemDBFileSize();
    }

    // 사용자가 Tablespace초기 크기를 지정하지 않은 경우
    if ( *aInitSize == 0 )
    {
        // 최소 확장단위인 Chunk크기만큼 생성
        *aInitSize = sChunkSize;
    }

    return IDE_SUCCESS;
}


/*
    Tablespace Attribute에 대한 에러체크를 실시한다.

     [IN] aName           - Tablespace의 이름
     [IN] aSplitFileSize  - Checkpoint Image File의 크기
     [IN] aInitSize       - Tablespace의 초기크기

 */
IDE_RC smmTBSCreate::checkErrorOfTBSAttr( SChar * aTBSName,
                                          ULong   aSplitFileSize,
                                          ULong   aInitSize)
{
    ULong sChunkSize = smuProperty::getExpandChunkPageCount() * SM_PAGE_SIZE;

    IDE_TEST_RAISE(idlOS::strlen(aTBSName) > (SM_MAX_DB_NAME -1),
                   too_long_tbsname);

    IDE_TEST_RAISE ( ( aSplitFileSize % sChunkSize ) != 0,
                     error_db_file_split_size_not_aligned_to_chunk_size );

    IDE_TEST( smuProperty::checkFileSizeLimit( "the split size of checkpoint image",
                                               aSplitFileSize )
              != IDE_SUCCESS );

    IDE_TEST_RAISE ( ( aInitSize % sChunkSize ) != 0,
                     error_tbs_init_size_not_aligned_to_chunk_size );

    return IDE_SUCCESS;

    IDE_EXCEPTION(too_long_tbsname);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_TooLongTBSName,
                                  (UInt)SM_MAX_DB_NAME ) );
    }
    IDE_EXCEPTION( error_tbs_init_size_not_aligned_to_chunk_size );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TBSInitSizeNotAlignedToChunkSize,
                                // KB 단위의 Expand Chunk Page 크기
                                ( sChunkSize / 1024 )));
    }
    IDE_EXCEPTION( error_db_file_split_size_not_aligned_to_chunk_size );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_SplitSizeNotAlignedToChunkSize,
                                // KB 단위의 Expand Chunk Page 크기
                                ( sChunkSize / 1024 )));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    새로운  Tablespace가 사용할 Tablespace ID를 할당받는다.

    [OUT] aSpaceID - 할당받은 Tablespace의 ID
 */
IDE_RC smmTBSCreate::allocNewTBSID( scSpaceID * aSpaceID )
{
    UInt  sStage = 0;

    IDE_TEST( sctTableSpaceMgr::lockForCrtTBS() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sctTableSpaceMgr::allocNewTableSpaceID( aSpaceID )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sctTableSpaceMgr::unlockForCrtTBS() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sStage )
    {
        case 1 :
            IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );
            break;
        default:
            break;
    }

    IDE_POP();


    return IDE_FAILURE;
}


/*
 * 사용자가 지정한, 혹은 시스템의 기본 Checkpoint Path를
 * Tablespace에 추가한다.
 *
 * aTBSNode           [IN] - Checkpoint Path가 추가될 Tabelspace의 Node
 * aChkptPathAttrList [IN] - 추가할 Checkpoint Path의 List
 */
IDE_RC smmTBSCreate::createDefaultOrUserChkptPaths(
                         smmTBSNode           * aTBSNode,
                         smiChkptPathAttrList * aChkptPathAttrList )
{
    UInt                  i;
    UInt                  sChkptPathCount;
    UInt                  sChkptPathSize;
    smiChkptPathAttrList  sCPAttrLists[ SM_DB_DIR_MAX_COUNT + 1];
    smiChkptPathAttrList *sCPAttrList;

    IDE_DASSERT( aTBSNode != NULL );
    // 사용자가 Checkpoint Path를 지정하지 않은 경우
    // aChkptPathAttrList는 NULL이다.

    // 사용자가 Checkpoint Path를 지정하지 않은 경우
    if ( aChkptPathAttrList == NULL )
    {
        // 사용자가 지정해둔 하나 이상의 DB_DIR 프로퍼티를 읽어서
        // Checkpoint Path Attribute들의 List로 연결한다.
        sChkptPathCount = smuProperty::getDBDirCount();

        for(i = 0; i < sChkptPathCount; i++)
        {
            sCPAttrLists[i].mCPathAttr.mAttrType = SMI_CHKPTPATH_ATTR;

            IDE_TEST( smmTBSChkptPath::setChkptPath(
                          & sCPAttrLists[i].mCPathAttr,
                          (SChar*)smuProperty::getDBDir(i) )
                      != IDE_SUCCESS );

            sCPAttrLists[i].mNext = & sCPAttrLists[i+1] ;
        }

        sCPAttrLists[ sChkptPathCount - 1 ].mNext = NULL ;
        aChkptPathAttrList = & sCPAttrLists[0];
    }
    // BUG-29812
    // 사용자가 Checkpoint Path를 지정한 경우
    // 절대경로이면 그대로 사용하고,
    // 상대경로이면 절대경로로 변경하여 사용한다.
    else
    {
        sCPAttrList = aChkptPathAttrList;

        while( sCPAttrList != NULL )
        {
            // BUG-29812
            // Memory TBS의 Checkpoint Path를 절대경로로 변환한다.
            sChkptPathSize = idlOS::strlen(sCPAttrList->mCPathAttr.mChkptPath);

            IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                        ID_FALSE,
                                        sCPAttrList->mCPathAttr.mChkptPath,
                                        &sChkptPathSize,
                                        SMI_TBS_MEMORY )
                      != IDE_SUCCESS);

            sCPAttrList = sCPAttrList->mNext;
        }
    }

    IDE_TEST( createChkptPathNodes( aTBSNode, aChkptPathAttrList )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * 하나 혹은 그 이상의 Checkpoint Path를 TBSNode의 Tail에 추가한다.
 *
 * aTBSNode       [IN] - Checkpoint Path가 추가될 Tabelspace의 Node
 * aChkptPathList [IN] - 추가할 Checkpoint Path의 List
 */
IDE_RC smmTBSCreate::createChkptPathNodes( smmTBSNode           * aTBSNode,
                                           smiChkptPathAttrList * aChkptPathList )
{

    SChar                * sChkptPath;
    smiChkptPathAttrList * sCPAttrList;
    smmChkptPathNode     * sCPathNode = NULL;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aChkptPathList != NULL );


    for ( sCPAttrList = aChkptPathList;
          sCPAttrList != NULL;
          sCPAttrList = sCPAttrList->mNext )
    {
        sChkptPath = sCPAttrList->mCPathAttr.mChkptPath;

        //////////////////////////////////////////////////////////////////
        // (e-010) NewPath가 없는 디렉토리이면 에러
        // (e-020) NewPath가 디렉토리가 아닌 파일이면 에러
        // (e-030) NewPath에 read/write/execute permission이 없으면 에러
        IDE_TEST( smmTBSChkptPath::checkAccess2ChkptPath( sChkptPath )
                  != IDE_SUCCESS );

        //////////////////////////////////////////////////////////////////
        // (e-040) 이미 Tablespace에 존재하는 Checkpoint Path가 있으면 에러
        IDE_TEST( smmTBSChkptPath::findChkptPathNode( aTBSNode,
                                                      sChkptPath,
                                                      & sCPathNode )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sCPathNode != NULL, err_chkpt_path_already_exists );

        //////////////////////////////////////////////////////////////////
        // (010) CheckpointPathNode 할당
        IDE_TEST( smmTBSChkptPath::makeChkptPathNode(
                      aTBSNode->mTBSAttr.mID,
                      sChkptPath,
                      & sCPathNode ) != IDE_SUCCESS );

        // (020) TBSNode 에 CheckpointPathNode 추가
        IDE_TEST( smmTBSChkptPath::addChkptPathNode( aTBSNode,
                                                     sCPathNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_chkpt_path_already_exists );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CPATH_ALREADY_EXISTS,
                                sChkptPath));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    // aChkptPathList중 존재하는 모든 Checkpoint Path Node를 제거한다.
    IDE_ASSERT( smmTBSChkptPath::removeChkptPathNodesIfExist(
                    aTBSNode, aChkptPathList )
                == IDE_SUCCESS );

    IDE_POP();

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : tablespace 생성 및 초기화
 * PROJ-1923 ALTIBASE HDB Disaster Recovery
 *****************************************************************************/
IDE_RC smmTBSCreate::createTBS4Redo( void                 * aTrans,
                                     smiTableSpaceAttr    * aTBSAttr,
                                     smiChkptPathAttrList * aChkptPathList )
{
    scSpaceID       sSpaceID;
    sctPendingOp  * sPendingOp;

    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aTBSAttr   != NULL );
    // aChkptPathList는 사용자가 지정하지 않은 경우 NULL이다.
    // aSplitFileSize는 사용자가 지정하지 않은 경우 0이다.
    // aInitSize는 사용자가 지정하지 않은 경우 0이다.
    // aNextSize는 사용자가 지정하지 않은 경우 0이다.
    // aMaxSize는 사용자가 지정하지 않은 경우 0이다.

    /*******************************************************************/
    // Tablespace 생성 실시

    // Tablespace의 ID할당 =======================================
    sSpaceID = sctTableSpaceMgr::getNewTableSpaceID();

    IDE_TEST( sSpaceID != aTBSAttr->mID );

    IDE_TEST( allocNewTBSID( & sSpaceID ) != IDE_SUCCESS );

    // 실제 Tablespace생성 수행 ==================================
    // Tablespace와 그 안의 Checkpoint Path를 생성한다.
    IDE_TEST( createTBSWithNTA4Redo( aTrans,
                                     aTBSAttr,
                                     aChkptPathList )
              != IDE_SUCCESS );

    // Commit시 동작할 Pending Operation등록  ======================
    // Commit시 동작 : Tablespace상태에서 SMI_TBS_CREATING을 제거
    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
            aTrans,
            aTBSAttr->mID,
            ID_TRUE,
            SCT_POP_CREATE_TBS,
            & sPendingOp)
        != IDE_SUCCESS );
    sPendingOp->mPendingOpFunc = smmTBSCreate::createTableSpacePending;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : 
 * PROJ-1923 ALTIBASE HDB Disaster Recovery
 *****************************************************************************/
IDE_RC smmTBSCreate::createTBSWithNTA4Redo( void                  * aTrans,
                                            smiTableSpaceAttr     * aTBSAttr,
                                            smiChkptPathAttrList  * aChkptPathList )
{
    IDE_DASSERT( aTrans                 != NULL );
    IDE_DASSERT( aTBSAttr->mName[0]     != '\0' );
    IDE_DASSERT( aTBSAttr->mAttrType    == SMI_TBS_ATTR );
    // NO ASSERT : aChkptPathList의 경우 NULL일 수 있다.

    idBool          sChkptBlocked   = ID_FALSE;
    idBool          sNodeCreated    = ID_FALSE;
    idBool          sNodeAdded      = ID_FALSE;

    smmTBSNode    * sTBSNode;
    sctLockHier     sLockHier;

    SCT_INIT_LOCKHIER(&sLockHier);

    // Tablespace Node를 할당, 다단계 초기화를 수행한다.
    // - 처리내용
    //   - Tablespace Node를 할당
    //   - Tablespace를 PAGE단계까지 초기화를 수행
    IDE_TEST( allocAndInitTBSNode( aTrans,
                                   aTBSAttr,
                                   & sTBSNode )
              != IDE_SUCCESS );
    sNodeCreated = ID_TRUE;

    // TBSNode에 계층 Lock을 X로 잡는다. ( 일반 DML,DDL과 경합 )
    //   - Lock획득시 Lock Slot을 sLockHier->mTBSNodeSlot에 넘겨준다.
    //   - 이 함수 호출전에 잡혀있는 Latch가 있어서는 안된다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                  aTrans,
                  & sTBSNode->mHeader,
                  ID_FALSE,   /* intent */
                  ID_TRUE,    /* exclusive */
                  sctTableSpaceMgr::getDDLLockTimeOut((smxTrans*)aTrans),
                  SCT_VAL_DDL_DML,/* validation */
                  NULL,       /* is locked */
                  & sLockHier )      /* lockslot */
              != IDE_SUCCESS );

    // - Checkpoint를 Block시킨다.
    //   - Checkpoint중 Dirty Page를 flush하는 작업과 경합
    //   - Checkpoint중 DB File Header를 flush하는 작업과 경합
    IDE_TEST( smLayerCallback::blockCheckpoint() != IDE_SUCCESS );
    sChkptBlocked=ID_TRUE;

    ///////////////////////////////////////////////////////////////
    // 로깅실시 => SCT_UPDATE_MRDB_CREATE_TBS
    //  - redo시 : Tablespace상태에서 SMI_TBS_CREATING 상태를 빼준다
    //  - undo시 : Tablespace상태를 SMI_TBS_DROPPED로 변경

    // Tablespace Node를 TBS Node List에 추가한다.
    IDE_TEST( registerTBS( sTBSNode ) != IDE_SUCCESS );
    sNodeAdded = ID_TRUE;

    IDE_TEST( createTBSInternal4Redo( aTrans,
                                      sTBSNode,
                                      aChkptPathList ) != IDE_SUCCESS );

    sChkptBlocked = ID_FALSE;
    IDE_TEST( smLayerCallback::unblockCheckpoint() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sChkptBlocked == ID_TRUE )
    {
        IDE_ASSERT( smLayerCallback::unblockCheckpoint() == IDE_SUCCESS );
    }

    if ( sNodeCreated == ID_TRUE )
    {
        if ( sNodeAdded == ID_TRUE )
        {
            // Tablespace Node가 시스템에 추가된 경우
            // SCT_UPDATE_MRDB_CREATE_TBS 의 Undo수행시
            // Tablespace의 Phase를 STATE단계까지 내려준다.
            // 노드는 해제되지 않는다.
        }
        else
        {
            // 잡혀있는 Lock이 있다면 해제
            if ( sLockHier.mTBSNodeSlot != NULL )
            {
                IDE_ASSERT( smLayerCallback::unlockItem(
                                aTrans,
                                sLockHier.mTBSNodeSlot )
                            == IDE_SUCCESS );
            }

            // 노드를 해제
            IDE_ASSERT( finiAndFreeTBSNode( sTBSNode ) == IDE_SUCCESS );
        }
    }

    IDE_POP();

    return IDE_FAILURE;
}

/******************************************************************************
 * PROJ-1923 ALTIBASE HDB Disaster Recovery
 *
 ******************************************************************************/
IDE_RC smmTBSCreate::createTBSInternal4Redo(
                          void                 * aTrans,
                          smmTBSNode           * aTBSNode,
                          smiChkptPathAttrList * aChkptPathAttrList )
{
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aTBSNode   != NULL );

    // NO ASSERT : aChkptPathAttrList의 경우 NULL일 수 있다.

    ///////////////////////////////////////////////////////////////
    // (070) Checkpoint Path Node들을 생성
    IDE_TEST( createDefaultOrUserChkptPaths( aTBSNode,
                                             aChkptPathAttrList )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (080) 테이블 스페이스의 페이지 메모리로 사용될
    //       Page Pool을 초기화한다.
    IDE_TEST( smmManager::initializePagePool( aTBSNode )
              != IDE_SUCCESS );

    // (100) initialize Membase on Page 0
    // (110) alloc ExpandChunks
    IDE_TEST( smmManager::createTBSPages4Redo(
                  aTBSNode,
                  aTrans,
                  smmDatabase::getDBName(), // aDBName,
                  aTBSNode->mTBSAttr.mMemAttr.mSplitFilePageCount,
                  aTBSNode->mTBSAttr.mMemAttr.mInitPageCount, // aInitPageCount,
                  smmDatabase::getDBCharSet(), // aDBCharSet,
                  smmDatabase::getNationalCharSet() // aNationalCharSet
                  )
              != IDE_SUCCESS );

    IDE_TEST( smmTBSCreate::flushTBSAndCPaths(aTBSNode) != IDE_SUCCESS );

    /* PROJ-2386 DR 
     * - 여기서는 checkpoint file 의 create LSN 을 알수없으므로 checkpoint file을 만들지 않도록 한다.
     * - checkpoint file은 SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK 로그 redo시 생성한다. */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * 시스템/사용자 메모리 Tablespace를 생성한다.
 *
 * [IN] aTrans          - Tablespace를 생성하려는 Transaction
 * [IN] aDBName         - Tablespace가 생성될 Database의 이름
 * [IN] aType           - Tablespace 종류
 *                  User||System, Memory, Data || Temp
 * [IN] aName           - Tablespace의 이름
 * [IN] aAttrFlag       - Tablespace의 속성 Flag
 * [IN] aChkptPathList  - Tablespace의 Checkpoint Image들을 저장할 Path들
 * [IN] aSplitFileSize  - Checkpoint Image File의 크기
 * [IN] aInitSize       - Tablespace의 초기크기
 *                  Meta Page(0번 Page)는 포함하지 않는 크기이다.
 * [IN] aIsAutoExtend   - Tablespace의 자동확장 여부
 * [IN] aNextSize       - Tablespace의 자동확장 크기
 * [IN] aMaxSize        - Tablespace의 최대크기
 * [IN] aIsOnline       - Tablespace의 초기 상태 (ONLINE이면 ID_TRUE)
 * [IN] aDBCharSet      - 데이터베이스 캐릭터 셋(PROJ-1579 NCHAR)
 * [IN] aNationalCharSet- 내셔널 캐릭터 셋(PROJ-1579 NCHAR)
 * [OUT] aTBSID         - 생성한 Tablespace의 ID
 *
 * - Latch간 Deadlock회피를 위한 Latch잡는 순서
 *   1. sctTableSpaceMgr::lock()           // TBS LIST
 *   2. sctTableSpaceMgr::latchSyncMutex() // TBS NODE
 *   3. smmPCH.mPageMemMutex.lock()        // PAGE
 *
 * - Lock과 Latch간의 Deadlock회피를 위한 Lock/Latch잡는 순서
 *   1. Lock를  먼저 잡는다.
 *   2. Latch를 나중에 잡는다.
 *****************************************************************************/
IDE_RC smmTBSCreate::createTBS( void                 * aTrans,
                                SChar                * aDBName,
                                SChar                * aTBSName,
                                UInt                   aAttrFlag,
                                smiTableSpaceType      aType,
                                smiChkptPathAttrList * aChkptPathList,
                                ULong                  aSplitFileSize,
                                ULong                  aInitSize,
                                idBool                 aIsAutoExtend,
                                ULong                  aNextSize,
                                ULong                  aMaxSize,
                                idBool                 aIsOnline,
                                SChar                * aDBCharSet,
                                SChar                * aNationalCharSet,
                                scSpaceID            * aTBSID )
{
    scSpaceID            sSpaceID;
    smiTableSpaceAttr    sTBSAttr;
    smmTBSNode         * sCreatedTBSNode;
    sctPendingOp       * sPendingOp;

    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aDBName    != NULL );
    IDE_DASSERT( aTBSName   != NULL );
    // aChkptPathList는 사용자가 지정하지 않은 경우 NULL이다.
    // aSplitFileSize는 사용자가 지정하지 않은 경우 0이다.
    // aInitSize는 사용자가 지정하지 않은 경우 0이다.
    // aNextSize는 사용자가 지정하지 않은 경우 0이다.
    // aMaxSize는 사용자가 지정하지 않은 경우 0이다.

    /*******************************************************************/
    // Tablespace 생성을 위한 준비단계
    {
        // 사용자가 Tablespace생성을 위한 옵션을 지정하지 않은 경우
        // 기본값을 설정
        IDE_TEST( makeDefaultArguments( & aSplitFileSize,
                                        & aInitSize ) != IDE_SUCCESS );


        // 사용자가 지정한 Tablespace생성을 위한 내용에 대해
        // 에러체크 실시
        IDE_TEST( checkErrorOfTBSAttr( aTBSName,
                                       aSplitFileSize,
                                       aInitSize) != IDE_SUCCESS );
    }

    /*******************************************************************/
    // Tablespace 생성 실시
    {
        // Tablespace의 ID할당 =======================================
        IDE_TEST( allocNewTBSID( & sSpaceID ) != IDE_SUCCESS );

        // Tablespace생성을 위한 Attribute초기화 ====================
        IDE_TEST( initializeTBSAttr( & sTBSAttr,
                                     sSpaceID,
                                     aType,
                                     aTBSName,
                                     aAttrFlag,
                                     aSplitFileSize,
                                     aInitSize) != IDE_SUCCESS );


        // 실제 Tablespace생성 수행 ==================================
        // Tablespace와 그 안의 Checkpoint Path를 생성한다.
        IDE_TEST( createTBSWithNTA(aTrans,
                                   aDBName,
                                   & sTBSAttr,
                                   aChkptPathList,
                                   sTBSAttr.mMemAttr.mInitPageCount,
                                   aDBCharSet,
                                   aNationalCharSet,
                                   & sCreatedTBSNode)
                  != IDE_SUCCESS );


        IDE_TEST( smmTBSAlterAutoExtend::alterTBSsetAutoExtend(
                                                  aTrans,
                                                  sTBSAttr.mID,
                                                  aIsAutoExtend,
                                                  aNextSize,
                                                  aMaxSize ) != IDE_SUCCESS );

        IDU_FIT_POINT( "3.PROJ-1548@smmTBSCreate::createTBS" );

        // Offline설정 =================================================
        // 위에서 생성한 Tablespace는 기본적으로 Online상태를 지닌다.
        // 사용자가 Tablespace를 Offline으로 생성하려는 경우
        // 상태를 Offline으로 바꾸어준다.
        if ( aIsOnline == ID_FALSE )
        {
            IDE_TEST( smLayerCallback::alterTBSStatus( aTrans,
                                                       sCreatedTBSNode,
                                                       SMI_TBS_OFFLINE )
                      != IDE_SUCCESS );
        }

        // Commit시 동작할 Pending Operation등록  ======================
        // Commit시 동작 : Tablespace상태에서 SMI_TBS_CREATING을 제거
        IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                                  aTrans,
                                                  sCreatedTBSNode->mHeader.mID,
                                                  ID_TRUE, /* commit시에 동작 */
                                                  SCT_POP_CREATE_TBS,
                                                  & sPendingOp)
                  != IDE_SUCCESS );
        sPendingOp->mPendingOpFunc = smmTBSCreate::createTableSpacePending;
    }

    if ( aTBSID != NULL )
    {
        *aTBSID = sSpaceID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Tablespace를 생성하고 생성이 완료되면 NTA로 묶는다.
 * 생성이 실패할 경우 NTA의 시작까지 Physical Undo한다.
 *
 * [IN] aTrans          - Tablespace를 생성하려는 Transaction
 * [IN] aDBName         - Tablespace가 생성될 Database의 이름
 * [IN] aTBSAttr        - Tablespace의 Attribute
 *                  ( 사용자가 지정한 내용으로 초기화되어 있음)
 * [IN] aChkptPathList  - Tablespace의 Checkpoint Image들을 저장할 Path들
 * [IN] aInitPageCount  - Tablespace의 초기크기 ( Page수 )
 * [IN] aDBCharSet      - 데이터베이스 캐릭터 셋(PROJ-1579 NCHAR)
 * [IN] aNationalCharSet- 내셔널 캐릭터 셋(PROJ-1579 NCHAR)
 * [OUT] aCreatedTBSNode - 생성된 Tablespace의 Node
 *****************************************************************************/
IDE_RC smmTBSCreate::createTBSWithNTA(
           void                  * aTrans,
           SChar                 * aDBName,
           smiTableSpaceAttr     * aTBSAttr,
           smiChkptPathAttrList  * aChkptPathList,
           scPageID                aInitPageCount,
           SChar                 * aDBCharSet,
           SChar                 * aNationalCharSet,
           smmTBSNode           ** aCreatedTBSNode )
{
    IDE_DASSERT( aTrans                 != NULL );
    IDE_DASSERT( aDBName                != NULL );
    IDE_DASSERT( aCreatedTBSNode        != NULL );
    IDE_DASSERT( aInitPageCount          > 0 );
    IDE_DASSERT( aTBSAttr->mName[0]     != '\0' );
    IDE_DASSERT( aTBSAttr->mAttrType    == SMI_TBS_ATTR );
    // NO ASSERT : aChkptPathAttrList의 경우 NULL일 수 있다.

    idBool         sNTAMarked    = ID_TRUE;
    idBool         sChkptBlocked = ID_FALSE;
    idBool         sNodeCreated  = ID_FALSE;
    idBool         sNodeAdded    = ID_FALSE;

    smLSN          sNTALSN;
    smmTBSNode   * sTBSNode;
    sctLockHier    sLockHier;

    SCT_INIT_LOCKHIER(&sLockHier);

    sNTALSN = smLayerCallback::getLstUndoNxtLSN( aTrans );
    sNTAMarked = ID_TRUE;

    // Tablespace Node를 할당, 다단계 초기화를 수행한다.
    // - 처리내용
    //   - Tablespace Node를 할당
    //   - Tablespace를 PAGE단계까지 초기화를 수행
    IDE_TEST( allocAndInitTBSNode( aTrans,
                                   aTBSAttr,
                                   & sTBSNode )
              != IDE_SUCCESS );
    sNodeCreated = ID_TRUE;

    // TBSNode에 계층 Lock을 X로 잡는다. ( 일반 DML,DDL과 경합 )
    //   - Lock획득시 Lock Slot을 sLockHier->mTBSNodeSlot에 넘겨준다.
    //   - 이 함수 호출전에 잡혀있는 Latch가 있어서는 안된다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                  aTrans,
                  & sTBSNode->mHeader,
                  ID_FALSE,   /* intent */
                  ID_TRUE,    /* exclusive */
                  sctTableSpaceMgr::getDDLLockTimeOut((smxTrans*)aTrans),
                  SCT_VAL_DDL_DML,/* validation */
                  NULL,       /* is locked */
                  & sLockHier )      /* lockslot */
              != IDE_SUCCESS );

    // - Checkpoint를 Block시킨다.
    //   - Checkpoint중 Dirty Page를 flush하는 작업과 경합
    //   - Checkpoint중 DB File Header를 flush하는 작업과 경합
    IDE_TEST( smLayerCallback::blockCheckpoint() != IDE_SUCCESS );
    sChkptBlocked=ID_TRUE;

    ///////////////////////////////////////////////////////////////
    // 로깅실시 => SCT_UPDATE_MRDB_CREATE_TBS
    //  - redo시 : Tablespace상태에서 SMI_TBS_CREATING 상태를 빼준다
    //  - undo시 : Tablespace상태를 SMI_TBS_DROPPED로 변경
    IDE_TEST( smLayerCallback::writeMemoryTBSCreate(
                                    NULL, /* idvSQL* */
                                    aTrans,
                                    aTBSAttr,
                                    aChkptPathList )
              != IDE_SUCCESS );

    // Tablespace Node를 TBS Node List에 추가한다.
    IDE_TEST( registerTBS( sTBSNode ) != IDE_SUCCESS );
    sNodeAdded = ID_TRUE;

    IDE_TEST( createTBSInternal(aTrans,
                                sTBSNode,
                                aDBName,
                                aChkptPathList,
                                aInitPageCount,
                                aDBCharSet,
                                aNationalCharSet) != IDE_SUCCESS );

    // ====== write NTA ======
    // - Redo : do nothing
    // - Undo : drop Tablespace수행
    IDE_TEST( smLayerCallback::writeCreateTbsNTALogRec(
                                   NULL, /* idvSQL* */
                                   aTrans,
                                   &sNTALSN,
                                   sTBSNode->mHeader.mID )
              != IDE_SUCCESS );

    sChkptBlocked = ID_FALSE;
    IDE_TEST( smLayerCallback::unblockCheckpoint() != IDE_SUCCESS );

    * aCreatedTBSNode = sTBSNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sNTAMarked == ID_TRUE )
    {
        // Fatal 오류가 아니라면, NTA까지 롤백
        IDE_ASSERT( smLayerCallback::undoTrans( NULL, /* idvSQL* */
                                                aTrans,
                                                &sNTALSN )
                    == IDE_SUCCESS );
    }

    if ( sChkptBlocked == ID_TRUE )
    {
        IDE_ASSERT( smLayerCallback::unblockCheckpoint() == IDE_SUCCESS );
    }

    if ( sNodeCreated == ID_TRUE )
    {
        if ( sNodeAdded == ID_TRUE )
        {
            // Tablespace Node가 시스템에 추가된 경우
            // SCT_UPDATE_MRDB_CREATE_TBS 의 Undo수행시
            // Tablespace의 Phase를 STATE단계까지 내려준다.
            // 노드는 해제되지 않는다.
        }
        else
        {
            // 잡혀있는 Lock이 있다면 해제
            if ( sLockHier.mTBSNodeSlot != NULL )
            {
                IDE_ASSERT( smLayerCallback::unlockItem( aTrans,
                                                         sLockHier.mTBSNodeSlot )
                            == IDE_SUCCESS );
            }

            // 노드를 해제
            IDE_ASSERT( finiAndFreeTBSNode( sTBSNode ) == IDE_SUCCESS );
        }
    }

    IDE_POP();

    return IDE_FAILURE;
}

/******************************************************************************
 * Tablespace를 시스템에 등록하고 내부 자료구조를 생성한다.
 *
 * aTrans              [IN] Tablespace를 생성하려는 Transaction
 * aTBSNode            [IN] Tablespace Node
 * aDBName             [IN] Tablespace가 속하는 Database의 이름
 * aChkptPathAttrList  [IN] Checkpoint Path Attribute의 List
 * aCreatedTBSNode     [OUT] 생성한 Tablespace의 Node
 *
 * [ PROJ-1548 User Memory Tablespace ]
 ******************************************************************************/
IDE_RC smmTBSCreate::createTBSInternal(
                          void                 * aTrans,
                          smmTBSNode           * aTBSNode,
                          SChar                * aDBName,
                          smiChkptPathAttrList * aChkptPathAttrList,
                          scPageID               aInitPageCount,
                          SChar                * aDBCharSet,
                          SChar                * aNationalCharSet)
{
    IDE_DASSERT( aTrans         != NULL );
    IDE_DASSERT( aDBName        != NULL );
    IDE_DASSERT( aTBSNode       != NULL );
    IDE_DASSERT( aInitPageCount > 0 );

    UInt sWhichDB   = 0;
    // NO ASSERT : aChkptPathAttrList의 경우 NULL일 수 있다.

    ///////////////////////////////////////////////////////////////
    // (070) Checkpoint Path Node들을 생성
    IDE_TEST( createDefaultOrUserChkptPaths( aTBSNode,
                                             aChkptPathAttrList )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (080) 테이블 스페이스의 페이지 메모리로 사용될
    //       Page Pool을 초기화한다.
    IDE_TEST( smmManager::initializePagePool( aTBSNode )
              != IDE_SUCCESS );

    // (100) initialize Membase on Page 0
    // (110) alloc ExpandChunks
    IDE_TEST( smmManager::createTBSPages(
                  aTBSNode,
                  aTrans,
                  aDBName,
                  aTBSNode->mTBSAttr.mMemAttr.mSplitFilePageCount,
                  aInitPageCount,
                  aDBCharSet,
                  aNationalCharSet)
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (120) Log Anchor Flush
    //
    // Commit 로그만 기록되고 Pending처리 안될 경우에 대비하여
    // Log Anchor에 CREATE중인 Tablespace를 Flush
    // SpaceNode Attr 가 먼저 로그앵커에 내려간 후
    // Chkpt Image Attr 가 내려가야 한다.  -> (130)
    //
    // - SMI_TBS_INCONSISTENT 상태로 Log Anchor에 내린다.
    //   - Tablespace가 온전히 생성되지 않은 상태
    //   - Membase와 DB File간의 Consistency를 보장할 수 없는 상태
    //   - Restart시 Prepare/Restore를 Skip하여야 한다.

    aTBSNode->mHeader.mState |= SMI_TBS_INCONSISTENT ;
    IDE_TEST( smmTBSCreate::flushTBSAndCPaths(aTBSNode) != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////
    // (130) create Checkpoint Image Files (init size만큼)
    // 테이블 스페이스의 모든 파일을 생성한다.
    // 동시에 로그앵커에 Chkpt Image Attr 이 저장된다.
    // - File생성 전에 Logging을 실시한다.
    //  - redo시 : do nothing
    //  - undo시 : file제거
    IDE_TEST( smmManager::createDBFile( aTrans,
                                        aTBSNode,
                                        aTBSNode->mHeader.mName,
                                        aInitPageCount,
                                        ID_TRUE /* aIsNeedLogging */ )

              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////////
    // (140) flush Page 0 to Checkpoint Image
    //
    // To Fix BUG-17227 create tablespace후 server kill; server start안됨
    //  - Restart Redo/Undo이전에 0번 Page를 읽어서 sm version check를
    //    하기 때문에, 0번 Page를 Flush해야함.
    for ( sWhichDB = 0; sWhichDB <= 1; sWhichDB++ )
    {
        IDE_TEST( smmManager::flushTBSMetaPage( aTBSNode, sWhichDB )
                  != IDE_SUCCESS );
    }

    // Membase와 Tablespace의 모든 Resource가 consistent하게
    // 생성 되었으므로, Tablespace의 상태에서
    // SMI_TBS_INCONSISTENT 상태를 제거한다.
    aTBSNode->mHeader.mState &= ~SMI_TBS_INCONSISTENT;
    IDE_TEST( smLayerCallback::updateTBSNodeAndFlush( & aTBSNode->mHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Tablespace Node와 그에 속한 Checkpoint Path Node들을 모두
    Log Anchor에 Flush한다.

    [IN] aTBSNode - Flush하려는 Tablespace Node
 */
IDE_RC smmTBSCreate::flushTBSAndCPaths(smmTBSNode * aTBSNode)
{
    smuList    * sNode;
    smuList    * sBaseNode;
    smmChkptPathNode  * sCPathNode;

    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smLayerCallback::addTBSNodeAndFlush( (sctTableSpaceNode*)aTBSNode )
              != IDE_SUCCESS );

    sBaseNode = &aTBSNode->mChkptPathBase;

    for (sNode = SMU_LIST_GET_FIRST(sBaseNode);
         sNode != sBaseNode;
         sNode = SMU_LIST_GET_NEXT(sNode))
    {
        sCPathNode = (smmChkptPathNode*)(sNode->mData);

        IDE_ASSERT( aTBSNode->mHeader.mID ==
                    sCPathNode->mChkptPathAttr.mSpaceID );

        IDE_TEST( smLayerCallback::addChkptPathNodeAndFlush( sCPathNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   Tablespace를 Create한 Tx가 Commit되었을 때 불리는 Pending함수

   [ 알고리즘 ]
     (010) latch TableSpace Manager
     (020) TBSNode.State 에서 CREATING상태 제거
     (030) TBSNode를 Log Anchor에 Flush
     (040) Runtime정보 갱신 => Tablespace의 수 Counting
     (050) unlatch TableSpace Manager
   [ 참고 ]
     Commit Pending함수는 실패해서는 안되기 때문에
     빠른 에러 Detect를 위해 IDE_ASSERT로 에러처리를 실시하였다.
 */

IDE_RC smmTBSCreate::createTableSpacePending(  idvSQL            * aStatistics,
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

    // Tablespace에 SMI_TBS_CREATING의 모든 비트가 켜져 있는지 ASSERT를
    // 걸어서는 안된다.
    //
    // 이유 :
    //   CREATE TABLESPACE .. OFFLINE 시에
    //   Alter Tablespace Offline의 COMMIT PENDING이 수행되어
    //   SMI_TBS_SWITCHING_TO_OFFLINE을 제거된 채로 들어올 수 있다.
    //
    //   그런데 SMI_TBS_CREATING 과 SMI_TBS_SWITCHING_TO_OFFLINE이
    //   일부 비트를 공유하기 때문에, Tablespace에 SMI_TBS_CREATING의
    //   모든 비트가 켜져 있지 않게 된다.

    aSpaceNode->mState &= ~SMI_TBS_CREATING;


    ///////////////////////////////////////////////////////////////
    // (030) TBSNode를 Log Anchor에 Flush
    /* PROJ-2386 DR
     * standby recovery도 동일한 시점에 loganchor update 한다. */
    IDE_ASSERT( smLayerCallback::updateTBSNodeAndFlush( aSpaceNode )
                == IDE_SUCCESS );



    /////////////////////////////////////////////////////////////////////
    // (050) unlatch TableSpace Manager
    sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );


    return IDE_SUCCESS;
}


/*
    smmTBSNode를 생성하고 X Lock을 획득한 후  sctTableSpaceMgr에 등록한다.

    [ Lock / Latch간의 deadlock 회피 ]
    - Lock을 먼저 잡고 Latch를 나중에 잡아서 Deadlock을 회피한다.
    - 이 함수가 불리기 전에 latch가 잡힌 상태여서는 안된다.


    [IN] aTrans      - Tablespace 에 계층 Lock을 잡고자 하는 경우
                       Lock을 잡을 Transaction
    [IN] aTBSAttr    - 생성할 Tablespace Node의 속성
    [IN] aCreatedTBSNode - 생성한 Tablespace Node
*/
IDE_RC smmTBSCreate::allocAndInitTBSNode(
                                   void                * aTrans,
                                   smiTableSpaceAttr   * aTBSAttr,
                                   smmTBSNode         ** aCreatedTBSNode )

{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTBSAttr != NULL );
    IDE_DASSERT( aTBSAttr->mAttrType == SMI_TBS_ATTR );
    IDE_DASSERT( aCreatedTBSNode != NULL );

    ACP_UNUSED( aTrans );

    UInt                 sStage = 0;
    smmTBSNode         * sTBSNode = NULL;

    /* TC/FIT/Limit/sm/smm/smmTBSCreate_allocAndInitTBSNode_calloc.sql */
    IDU_FIT_POINT_RAISE( "smmTBSCreate::allocAndInitTBSNode::calloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_SM_SMM,
                               1,
                               ID_SIZEOF(smmTBSNode),
                               (void**)&(sTBSNode)) != IDE_SUCCESS,
                   insufficient_memory );
    sStage = 1;


    ///////////////////////////////////////////////////////////////
    // (080) Tablespace를 다단계 초기화
    // 여기에서 TBSNode.mState에 TBSAttr.mTBSStateOnLA를 복사
    // - 처음에는 ONLINE상태로 Tablespace를 다단계 초기화
    // - 사용자가 OFFLINE으로 생성한 Tablespace의 경우
    //   Create Tablespace의 마지막 단계에서 OFFLINE으로 전환
    //
    // - Create중인 Tablespace의 상태
    //   - SMI_TBS_CREATING
    //     - BACKUP과의 동시성 제어를 위해를 넣어줌
    //     - Commit Pending시 CREATING상태가 제거됨
    aTBSAttr->mTBSStateOnLA = SMI_TBS_CREATING | SMI_TBS_ONLINE;

    IDE_TEST( smmTBSMultiPhase::initTBS( sTBSNode,
                                         aTBSAttr )
              != IDE_SUCCESS );
    sStage = 2;


    * aCreatedTBSNode = sTBSNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 2:
        {
            IDE_ASSERT( smmTBSMultiPhase::finiTBS( sTBSNode )
                        == IDE_SUCCESS );
        }

        case 1:
        {
            IDE_ASSERT( iduMemMgr::free(sTBSNode) == IDE_SUCCESS );
        }

        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
    Tablespace Node를 다단계 해제수행후 free한다.
 */
IDE_RC smmTBSCreate::finiAndFreeTBSNode( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST( smmTBSMultiPhase::finiTBS( aTBSNode ) != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::free(aTBSNode) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    다른 Transaction들이 Tablespace이름으로 Tablespace를 찾을 수 있도록
    시스템에 등록한다.

    - 같은 이름의 Tablespace가 이미 존재하는지 여기에서 체크한다.

    [IN] aTBSNode - 등록하려는 Tablespace의 Node
 */
IDE_RC smmTBSCreate::registerTBS( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    UInt    sStage = 0;
    idBool  sIsExistNode;

    // 동일한 Tablespace가 동시에 생성되는 것을 방지하게 위해
    // global tablespace latch를 잡는다.
    IDE_TEST( sctTableSpaceMgr::lockForCrtTBS() != IDE_SUCCESS );
    sStage = 1;

    /* 동일한 tablespace명이 존재하는지 검사한다. */
    // BUG-26695 TBS Node가 없는것이 정상이므로 없을 경우 오류 메시지를 반환하지 않도록 수정
    sIsExistNode = sctTableSpaceMgr::checkExistSpaceNodeByName( aTBSNode->mHeader.mName );

    IDE_TEST_RAISE( sIsExistNode == ID_TRUE, err_already_exist_tablespace_name );

    sctTableSpaceMgr::addTableSpaceNode( (sctTableSpaceNode*)aTBSNode );

    sStage = 0;
    IDE_TEST( sctTableSpaceMgr::unlockForCrtTBS() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_already_exist_tablespace_name )
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistTableSpaceName,
                                aTBSNode->mHeader.mName));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 1:
        {
            IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );
        }
        break;
        default :
            break;

    }

    IDE_POP();

    return IDE_FAILURE;
}






