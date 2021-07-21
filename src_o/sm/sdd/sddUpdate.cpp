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
 * $Id: sddUpdate.cpp 88020 2020-07-10 09:34:13Z et16 $
 *
 * Description :
 *
 * 본 파일은 FILE 연산관련 redo/undo 함수에 대한 구현파일이다.
 *
 **********************************************************************/

#include <smDef.h>
#include <sddDef.h>
#include <smErrorCode.h>
#include <sddDiskMgr.h>
#include <sddTableSpace.h>
#include <sddDataFile.h>
#include <sddUpdate.h>
#include <sddReq.h>
#include <sctTableSpaceMgr.h>
#include <sdptbSpaceDDL.h>

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_CREATE_TBS 로그 재수행

트랜잭션 Commit Pending List: [POP_DBF]->[POP_TBS] // 순서 무관

순서 :   (1)           (2)            (3)      (4)        (5)       (6)       (7)        (8)
연산 : [CREATE_TBS]->[CREATE_DBF]->[ANCHOR]->[COMMIT]->[POP_DBF]->[ANCHOR]->[POP_TBS]->[ANCHOR]
상태 :  |CREATEING     |CREATING     저장      커밋      ONLINE     저장     ONLINE     저장
        |ONLINE        |ONLINE                          ~CREATING           ~CREATING

알고리즘

가. (3) 이전에 실패하면 커밋이 안된 트랜잭션이고, Loganchor에도 저장되지 않았기 때문에
    TBS List에서 검색이 안되며, 재수행할 것이 없다.

나. (3)과 (4) 사이에 실패하면 커밋이 안된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태로
    저장되었으므로, Rollback Pending 연산을 통해 DROPPED로 변경해주어야 한다.

다. (7)과 (8) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태로
    저장되었으므로, ONLINE|CREATING 상태일 경우에만 Commit Pending 연산을 등록하여
    ONLINE 상태로 변경해주어야 한다.

라. (8) 이후에 실패하면 커밋된 트랜잭션이고, 로그앵커에 ONLINE된 상태로 저장되었으므로
    재수행 할 것이 없다.

PROJ-1923 ALTIBASE HDB Disaster Recovery
위 기능을 위해 무조건 redo로 수정한다.

    => 변경 됨
가. (3) 이전에 실패하면 커밋 안된 트랜잭션이고, 로그앵커에도 저장되어 있지 않다.
    따라서 처음부터 Create TBS를 수행한다. (커밋이 없으므로 추후 Rollback 된다.)
*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_CREATE_TBS(
                                            idvSQL    * /* aStatistics */,
                                            void      * aTrans,
                                            smLSN       /* aCurLSN */,
                                            scSpaceID   aSpaceID,
                                            UInt        /* aFileID */,
                                            UInt        aValueSize,
                                            SChar     * aValuePtr,
                                            idBool      /* aIsRestart */ )
{
    sddTableSpaceNode * sSpaceNode;
    smiTableSpaceAttr   sTableSpaceAttr;
    scSpaceID           sNewSpaceID = 0;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR_MSG( aValueSize == ID_SIZEOF(smiTableSpaceAttr),
                   "aValuesSize : %"ID_UINT32_FMT,
                   aValueSize );

    /* Loganchor로부터 초기화된 TBS List를 검색한다. */
    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if( sSpaceNode != NULL )
    {
        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED)
                     != SMI_TBS_DROPPED );

        if ( SMI_TBS_IS_CREATING(sSpaceNode->mHeader.mState) )
        {
            /* 알고리즘 (다)에 해당하는 CREATINIG 상태일 경우에만 있으므로
             * 상태를 ONLINE으로 변경할 수 있게 Commit Pending 연산을 등록한다. */
            if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
            {
                IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                                aTrans,
                                                aSpaceID,
                                                ID_TRUE, /* commit시에 동작 */
                                                SCT_POP_CREATE_TBS )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Active Tx가 아닌경우 Pendig 등록하지 않는다. */
            }

            /* 알고리즘 (나)에 해당하는 것은 Rollback Pending 연산이기 때문에
             * undo_SCT_UPDATE_DRDB_CREATE_TBS()에서 POP_DROP_TBS 에서 등록한다. */
        }
        else
        {
            /* 알고리즘 (라) 에 해당하므로 재수행하지 않는다. */
        }
    }
    else
    {
        sNewSpaceID = sctTableSpaceMgr::getNewTableSpaceID();

        /* 로그에서 읽어온 spaceID와 메타에서 읽어온 newSpaceID가
         * 같으면 (가)에 해당하는 경우 중 redo를 수행해야 한다. */
        if( aSpaceID == sNewSpaceID )
        {
            /* PROJ-1923 ALTIBASE HDB Disaster Recovery
             * 위 기능을 지원하기 위해 알고리즘 (가) 에 해당하는 경우에도
             * 재수행을 수행한다. */
            idlOS::memcpy( (void *)&sTableSpaceAttr,
                           aValuePtr,
                           ID_SIZEOF(smiTableSpaceAttr) );

            // sdptbSpaceDDL::createTBS() 을 참고하여 redo 한다.
            IDE_TEST( sdptbSpaceDDL::createTBS4Redo( aTrans,
                                                     &sTableSpaceAttr )
                      != IDE_SUCCESS );
        }
        else
        {
            // do nothing
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_CREATE_TBS 로그 UNDO

트랜잭션 Rollback Pending List: [POP_DBF]->[POP_TBS] // 순서만족


순서 :   (1)           (2)            (3)      (4)        (5)        (6)
연산 : [CREATE_TBS]->[CREATE_DBF]->[ANCHOR]->[CLR_DBF]->[CLR_TBS]->[ROLLBACK]->
상태 :  |CREATING     |CREATING      저장      |DROPPING |DROPPING
        |ONLINE       |ONLINE                  |ONLINE   |ONLINE
                                               |CREATING |CREATING

순서 :  (7)          (8)       (9)      (10)
연산 : [POP_DBF]->[ANCHOR]->[POP_TBS]->[ANCHOR]
상태 :  DROPPED    저장     DROPPED     저장
        ~ONLINE  (DBF삭제)  ~ONLINE
        ~CREATING           ~CREATING
        ~DROPPING           ~DROPPING

복구 알고리즘

RESTART시

가. (3)undo를 수행하면 완료가 안된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태이기
    때문에, ONLINE|CREATING|DROPPING 상태로 변경하고, Rollback Pending 연산을 등록하여 DROPPED로
    변경해주어야 한다.

나. (9)과 (10) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태로
    저장되었으므로, Rollback Pending 연산을 등록하여 DROPPED 상태로 변경해주어야 한다.

다. (10) 이후에 실패하면 커밋된 트랜잭션이고, 로그앵커에 DROPPED된 상태로 저장되었으므로
    검색이 되지 않으며, undo할 것도 없다.

RUNTIME시

가. (1)과 (2) 사이에서 실패하면 TBS List에서 검색이 안되는 경우에는 재수행할 것이 없다.

나. (3)에서 실패하면 TBS List에서 검색이 되므로, ONLINE|CREATING|DROPPING 변경하고
    Rollback Pending 연산을 등록하여 DROPPED로 변경한다.

*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_CREATE_TBS(
                    idvSQL    * /* aStatistics */,
                    void      * aTrans,
                    smLSN       /* aCurLSN */,
                    scSpaceID   aSpaceID,
                    UInt        /* aFileID */,
                    UInt        aValueSize,
                    SChar     * /* aValuePtr */,
                    idBool      aIsRestart )
{
    sddTableSpaceNode * sSpaceNode;

    IDE_ERROR_MSG( aValueSize == 0,
                   "aValuesSize : %"ID_UINT32_FMT,
                   aValueSize );
    IDE_ERROR( (aTrans      != NULL) ||
               (aIsRestart  == ID_TRUE) );

    /* TBS List를 검색한다. */
    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    /* RUNTIME시에는 sSpaceNode 자체에 대해서 (X) 잠금이 잡혀있기 때문에
     * sctTableSpaceMgr::lock을 획득할 필요가 없다. */
    if( sSpaceNode != NULL )
    {
        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED)
                   != SMI_TBS_DROPPED );

        if ( SMI_TBS_IS_CREATING(sSpaceNode->mHeader.mState) )
        {
            /* CREATE TBS 연산에서는 어느 과정에서 실패하던지
             * Loganchor에 DROPPING상태가 저장될 수 없으므로
             * RESTART시에는 DROPPING 상태가 있을 수 없음. */
            IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPING)
                       != SMI_TBS_DROPPING );

            /* RESTART 알고리즘 (가),(나)에 해당한다.
             * RUNTIME 알고리즘 (나)에 해당한다. */
            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                          aTrans,
                                          sSpaceNode->mHeader.mID,
                                          ID_FALSE, /* abort 시 동작 */
                                          SCT_POP_DROP_TBS) != IDE_SUCCESS );

            sSpaceNode->mHeader.mState |= SMI_TBS_DROPPING;
        }
        else
        {
            /* 알고리즘 RESTART (다)에 위배된다.
             * nothing to do ... */
            IDE_ERROR( 0 );
        }

    }
    else
    {
        /* RESTART 알고리즘 (다) 해당
         * RUNTIME 알고리즘 (가) 해당
         * nothing to do ... */
    }

    /* RUNTIME시에 변경이 발생했다면 Rollback Pending이 등록되었을 것이고
     * Rollback Pending시 Loganchor를 갱신한다. */
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_DROP_TBS 로그 재수행

트랜잭션 Pending List: [POP_DBF]->[POP_TBS] // 순서만족

순서 :   (1)           (2)      (3)        (4)       (5)       (6)        (7)
연산 : [DROP_DBF]->[DROP_TBS]->[COMMIT]->[POP_DBF]->[ANCHOR]->[POP_TBS]->[ANCHOR]
상태 :  |DROPPING  |DROPPING   커밋     |DROPPED     저장      DROPPED     저장
        |ONLINE    |ONLINE              |ONLINE     (dbf삭제)

복구 알고리즘

가. (3) 이전에 실패하면 커밋이 안된 트랜잭션이고, Loganchor에도 저장되지 않았기 때문에
    TBS List에서 검색이 되면 ONLINE|DROPPING 상태로 설정하고, Commit Pending
    (DROPPING이 아니면 죽음) 연산을 등록하여 DROPPED 상태로 변경해주어야 한다.

나. (6)과 (7) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE 상태로
    저장되어 있기 때문에 ONLINE|DROPPING상태로 변경하고, Commit Pending 연산을 등록하여
    DROPPED 상태로 변경해주어야 한다.

다. (7) 이후에 실패하면 커밋된 트랜잭션이고, 상태가 DROPPED가 되어 Loganchor에
    저장되지 않으므로, TBS List에서 검색되지 않는다.

*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_DROP_TBS( idvSQL       * /* aStatistics */,
                                                 void         * aTrans,
                                                 smLSN          /* aCurLSN */,
                                                 scSpaceID      aSpaceID,
                                                 UInt           /* aFileID */,
                                                 UInt           aValueSize,
                                                 SChar        * /* aValuePtr */,
                                                 idBool         /* aIsRestart */ )
{
    sddTableSpaceNode     * sSpaceNode;

    IDE_ERROR( aTrans       != NULL );
    IDE_ERROR( aValueSize   == 0 );

    // TBS List에서 검색한다.
    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if( sSpaceNode != NULL )
    {
        // PRJ-1548 User Memory Tablespace
        // DROP TBS 연산이 commit이 아니기 때문에 DROPPED로
        // 설정하면 관련된 로그레코드를 재수행할 수 없다.
        // RESTART RECOVERY시 Commit Pending Operation을 적용하여
        // 본 버그를 수정한다.
        // SCT_UPDATE_DRDB_DROP_TBS 재수행을 할 경우에는 DROPPING
        // 상태로 설정하고, 해당 트랜잭션의 COMMIT 로그를 재수행할 때
        // Commit Pending Operation으로 DROPPED 상태로 설정한다.

        if ( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPING)
             != SMI_TBS_DROPPING )
        {
            if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
            {
                sSpaceNode->mHeader.mState |= SMI_TBS_DROPPING;

                // 알고리즘 (가), (나)에 해당하는 경우 Commit Pending 연산 등록
                IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                            aTrans,
                                            aSpaceID,
                                            ID_TRUE, /* commit시에 동작 */
                                            SCT_POP_DROP_TBS )
                          != IDE_SUCCESS );
            }
            else
            {
                // Active Tx가 아닌 경우에는 Pending 연산을 추가하지 않는다.
            }
        }
        else
        {
            // nothing to do ..
        }
    }
    else
    {
        // 알고리즘 (다)에 해당하는 경우 TBS List에서 검색이 되지 않으며
        // 재수행하지 않는다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_DROP_TBS 로그 UNDO

순서 :   (1)           (2)     (3)         (4)        (5)
연산 : [DROP_DBF]->[DROP_TBS]->[CLR_TBS]->[CLR_DBF]->[ROLLBACK]
상태 :  |ONLINE    |ONLINE     ~DROPPING   ~DROPPING
        |DROPPING  |DROPPING

복구 알고리즘

RESTART시

가. (2)이후에 실패한 경우, TBS List에서 검색된다면 (2)를 재수행하여
    ONLINE|DROPPING 상태이기 때문에, ~DROPPING 연산을 하여 ONLINE 상태로 변경한다.

RUNTIME시

가. (2)이후에 실패하면 TBS List에서 검색하여 ~DROPPING 연산을 수행하여 ONLINE 상태로
    변경한다.

*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_DROP_TBS(
                       idvSQL *             /*aStatistics*/,
                       void*                aTrans,
                       smLSN                /* aCurLSN */,
                       scSpaceID            aSpaceID,
                       UInt               /*aFileID */,
                       UInt                 aValueSize,
                       SChar*             /*aValuePtr */,
                       idBool               aIsRestart )
{
    sddTableSpaceNode*  sSpaceNode;

    IDE_ERROR( (aTrans != NULL) || (aIsRestart == ID_TRUE) );
    IDE_ERROR( aValueSize == 0 );

    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    // RUNTIME시에는 sSpaceNode 자체에 대해서 (X) 잠금이 잡혀있기 때문에
    // sctTableSpaceMgr::lock을 획득할 필요가 없다.
    if( sSpaceNode != NULL )
    {
        if( SMI_TBS_IS_DROPPING(sSpaceNode->mHeader.mState) )
        {
            // 알고리즘 RESTART (가), RUNTIME (가) 에 해당하는 경우이다.
            // DROPPING을 끄고, ONLINE 상태로 변경한다.
            sSpaceNode->mHeader.mState &= ~SMI_TBS_DROPPING;
        }

        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED )
                   != SMI_TBS_DROPPED );
        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_CREATING)
                   != SMI_TBS_CREATING );
    }
    else
    {
        // TBS List에서 검색이 되지 않으면 아무것도 하지 않는다.
        // nothing to do...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_CREATE_DBF 로그 재수행

트랜잭션 Commit Pending List: [POP_DBF]->[POP_TBS]

순서 :   (1)           (2)            (3)      (4)        (5)       (6)       (7)        (8)
연산 : [CREATE_TBS]->[CREATE_DBF]->[ANCHOR]->[COMMIT]->[POP_DBF]->[ANCHOR]->[POP_TBS]->[ANCHOR]
상태 :  |CREATEING    |CREATING     저장      커밋      ONLINE     저장     ONLINE     저장
        |ONLINE       |ONLINE                          ~CREATING           ~CREATING

알고리즘

가. (3) 이전에 실패하면 커밋이 안된 트랜잭션이고, Loganchor에도 저장되지 않았기 때문에
    TBS List에서 검색이 안되며, 재수행할 것이 없다.

나. (3)과 (4) 사이에 실패하면 커밋이 안된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태로
    저장되었으므로, Rollback Pending 연산을 통해 DROPPED로 변경해주어야 한다.

다. (5)과 (6) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태로
    저장되었으므로, ONLINE|CREATING 상태일 경우에만 Commit Pending 연산을 등록하여
    ONLINE 상태로 변경해주어야 한다.

라. (6) 이후에 실패하면 커밋된 트랜잭션이고, 로그앵커에 ONLINE된 상태로 저장되었으므로
    재수행 할 것이 없다.

# CREATE_TBS의 복구알고리즘과 거의 동일하다.

*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_CREATE_DBF( idvSQL     * /* aStatistics */,
                                                   void       * aTrans,
                                                   smLSN        aCurLSN,
                                                   scSpaceID    aSpaceID,
                                                   UInt         aFileID,
                                                   UInt         aValueSize,
                                                   SChar      * aValuePtr,
                                                   idBool       /* aIsRestart */)
{
    sctPendingOp      * sPendingOp;
    smiTouchMode        sTouchMode;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    smiDataFileAttr     sDataFileAttr;

    IDE_ERROR( aTrans       != NULL );
    IDE_ERROR( aValuePtr    != NULL );
    IDE_ERROR_MSG( aValueSize == ( ID_SIZEOF(smiTouchMode) +
                                   ID_SIZEOF(smiDataFileAttr) ),
                   "aValueSize : %"ID_UINT32_FMT,
                   aValueSize );

    /* Loganchor로부터 초기화된 TBS List를 검색한다. */
    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    /* PROJ-1923 ALTIBASE HDB Disaster Recovery
     * 알고리즘 (가)에 해당하는 경우이나,
     * SCT_UPDATE_DRDB_CREATE_TBS를 항상 Redo 하므로,
     * SCT_UPDATE_DRDB_CREATE_DBF를 Redo 할때 해당 TBS를 못찾으면 오류 상황 */
    IDE_TEST_RAISE( sSpaceNode == NULL, err_tablespace_does_not_exist );

    IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED)
               != SMI_TBS_DROPPED );

    if ( sFileNode != NULL )
    {
        if ( SMI_FILE_STATE_IS_CREATING( sFileNode->mState ) )
        {
            if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
            {
                idlOS::memcpy( &sTouchMode,
                               aValuePtr,
                               ID_SIZEOF(smiTouchMode) );

                /* 알고리즘 (다)에 해당하는 CREATINIG 상태일 경우에만
                 * 있으므로 상태를 ONLINE으로 변경할 수 있게
                 * Commit Pending 연산을 등록한다. */
                IDE_TEST( sddDataFile::addPendingOperation(
                        aTrans,
                        sFileNode,
                        ID_TRUE, /* commit시에 동작 */
                        SCT_POP_CREATE_DBF,
                        &sPendingOp ) != IDE_SUCCESS );

                sPendingOp->mTouchMode = sTouchMode;
            }
            else
            {
                /* Active Tx가 아닌 경우에는 Pending 연산을
                 * 추가하지 않는다. */
            }

            /* 알고리즘 (나)에 해당하는 것은 Rollback Pending연산이기 때문에
             * undo_SCT_UPDATE_DRDB_CREATE_DBF()에서 POP_DROP_DBF에서 등록한다. */
        }
        else
        {
            /* 알고리즘 (라) 에 해당하므로 재수행하지 않는다. */
        }
    }
    else
    {
        /* PROJ-1923 ALTIBASE HDB Disaster Recovery
         * 위 기능을 지원하기 위해 알고리즘 (가) 에 해당하는 경우 이지만
         * 재수행을 수행한다.
         * 기존에 DBF 파일이 있다면 삭제하고 새로 만들어도 된다.
         * 왜냐하면 로그앵커에 기록되어 있지 않으므로 없는 파일과 같다. */
        idlOS::memcpy( (void *)&sDataFileAttr,
                       aValuePtr + ID_SIZEOF(smiTouchMode) ,
                       ID_SIZEOF(smiDataFileAttr) );
        
        // sdptbSpaceDDL::createTBS() 을 참고하여 redo 한다.
        IDE_TEST( sdptbSpaceDDL::createDBF4Redo( aTrans,
                                                 aCurLSN,
                                                 aSpaceID,
                                                 &sDataFileAttr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_tablespace_does_not_exist );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_TablespaceDoesNotExist, aSpaceID ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_CREATE_DBF 로그 UNDO

트랜잭션 Pending List: [POP_TBS]->[POP_DBF]


순서 :   (1)           (2)            (3)      (4)        (5)        (6)
연산 : [CREATE_TBS]->[CREATE_DBF]->[ANCHOR]->[CLR_DBF]->[CLR_TBS]->[ROLLBACK]->
상태 :  CREATING      |CREATING     저장      |DROPPING  |DROPPING
                      |ONLINE                 |ONLINE    |CREATING
                                              |CREATING
순서 :  (7)          (8)       (9)      (10)
연산 : [POP_DBF]->[ANCHOR]->[POP_TBS]->[ANCHOR]
상태 :  |DROPPED    DBF 저장  DROPPED   TBS 저장
        |ONLINE    (파일삭제)

복구 알고리즘

RESTART시

가. (3)에서 undo를 수행하면 완료가 안된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태이기
    때문에, ONLINE|CREATING|DROPPING 상태로 변경하고, Rollback Pending 연산을 등록하여 DROPPED로
    변경해주어야 한다.

나. (7)과 (8) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE|CREATING 상태로
    저장되었으므로, Rollback Pending 연산을 등록하여 DROPPED 상태로 변경해주어야 한다.

다. (8) 이후에 실패하면 커밋된 트랜잭션이고, 로그앵커에 DROPPED된 상태로 저장되었으므로
    검색이 되지 않으며, undo할 것도 없다.

RUNTIME시

가. (2)과 (3) 사이에서 실패하면 TBS List에서 검색이 안되는 경우에는 재수행할 것이 없다.

나. (3)에서 실패하면 TBS List에서 검색이 되므로, ONLINE|CREATING|DROPPING 변경하고
    Rollback Pending 연산을 등록하여 DROPPED로 변경한다.

# CREATE_TBS의 복구알고리즘과 거의 동일하다.
*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_CREATE_DBF( idvSQL     * aStatistics,
                                                   void       * aTrans,
                                                   smLSN        /* aCurLSN */,
                                                   scSpaceID    aSpaceID,
                                                   UInt         aFileID,
                                                   UInt         aValueSize,
                                                   SChar      * aValuePtr,
                                                   idBool       aIsRestart )
{
    UInt                sState = 0;
    smiTouchMode        sTouchMode;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    sctPendingOp      * sPendingOp;

    IDE_ERROR( aValuePtr    != NULL );
    IDE_ERROR( (aTrans      != NULL) ||
               (aIsRestart  == ID_TRUE) );
    IDE_ERROR_MSG( aValueSize == ID_SIZEOF(smiTouchMode),
                   "aValueSize : %"ID_UINT32_FMT,
                   aValueSize );

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    /* TBS Node에 (X) 잠금을 획득했거나, DBF Node에 (X)잠금을 획득한 이후라서
     * sctTableSpaceMgr::lock()을 획득하지 않는다. */
    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        /* creating 중에 나 말고 다른이가 수정하지 않음
         * 반대로 그렇기 때문에 동시성 병목도 없다.
         * 안전하게 잡는다.*/
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;

        /* 알고리즘 RESTART (다)에 위배된다. nothing to do ... */
        IDE_ERROR( SMI_FILE_STATE_IS_CREATING( sFileNode->mState ) );

        idlOS::memcpy( &sTouchMode,
                       aValuePtr,
                       ID_SIZEOF(smiTouchMode) );

        /* CREATE TBS 연산에서는 어느 과정에서 실패하던지
         * Loganchor에 DROPPING상태가 저장될 수 없으므로
         * RESTART시에는 DROPPING 상태가 있을 수 없음. */
        IDE_ERROR( SMI_FILE_STATE_IS_NOT_DROPPING( sFileNode->mState ) );

        /* RESTART 알고리즘 (가),(나)에 해당한다.
         * RUNTIME 알고리즘 (나)에 해당한다. */
        IDE_TEST( sddDataFile::addPendingOperation(
                      aTrans,
                      sFileNode,
                      ID_FALSE, /* abort 시 동작 */
                      SCT_POP_DROP_DBF,
                      &sPendingOp ) != IDE_SUCCESS );

        sPendingOp->mPendingOpFunc  = sddDiskMgr::removeFilePending;
        sPendingOp->mPendingOpParam = (void*)sFileNode;
        sPendingOp->mTouchMode      = sTouchMode;

        sFileNode->mState |= SMI_FILE_DROPPING;

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    else
    {
        /* RESTART 알고리즘 (다) 해당
         * RUNTIME 알고리즘 (가) 해당
         * nothing to do ... */
    }

    /* RUNTIME시에 변경이 발생했다면 Rollback Pending이 등록되었을 것이고
     * Rollback Pending시 Loganchor를 갱신한다. */
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        /* BUG-47982 예외 처리에서 엉뚱한 mutex를 unlock 합니다. */
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_DROP_DBF 로그 재수행

트랜잭션 Pending List: [POP_DBF]->[POP_TBS]

순서 :   (1)           (2)      (3)        (4)       (5)       (6)        (7)
연산 : [DROP_DBF]->[DROP_TBS]->[COMMIT]->[POP_DBF]->[ANCHOR]->[POP_TBS]->[ANCHOR]
상태 :  |DROPPING  |DROPPING   커밋     |DROPPED     저장      DROPPED     저장
        |ONLINE    |ONLINE              |ONLINE

복구 알고리즘

가. (3) 이전에 실패하면 커밋이 안된 트랜잭션이고, Loganchor에도 저장되지 않았기 때문에
    TBS List에서 검색이 되면 ONLINE|DROPPING 상태로 설정하고, Commit Pending 연산을 등록하여
    DROPPED 상태로 변경해주어야 한다.

나. (4)과 (5) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE 상태로
    저장되어 있기 때문에 ONLINE|DROPPING상태로 변경하고, Commit Pending 연산을 등록하여
    DROPPED 상태로 변경해주어야 한다.

다. (7) 이후에 실패하면 커밋된 트랜잭션이고, 상태가 DROPPED가 되어 Loganchor에
    저장되지 않으므로, TBS List에서 검색되지 않는다.

# DROP_TBS의 복구알고리즘과 거의 동일하다.
*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_DROP_DBF( idvSQL       * /* aStatistics */,
                                                 void         * aTrans,
                                                 smLSN          /* aCurLSN */,
                                                 scSpaceID      aSpaceID,
                                                 UInt           aFileID,
                                                 UInt           aValueSize,
                                                 SChar        * aValuePtr,
                                                 idBool         /* aIsRestart */ )
{
    sctPendingOp      * sPendingOp;
    SChar             * sValuePtr;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    smiTouchMode        sTouchMode;

    IDE_ERROR( aTrans       != NULL );
    IDE_ERROR( aValuePtr    != NULL );
    IDE_ERROR_MSG( aValueSize == ID_SIZEOF(smiTouchMode),
                   "aValueSize : %"ID_UINT32_FMT,
                   aValueSize );

    sValuePtr = aValuePtr;
    idlOS::memcpy(&sTouchMode, sValuePtr, ID_SIZEOF(smiTouchMode));

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        // PRJ-1548 User Memory Tablespace
        // DROP DBF 연산이 commit이 아니기 때문에 DROPPED로
        // 설정하면 관련 로그레코드를 재수행할 수 없다.
        // RESTART RECOVERY시 Commit Pending Operation을 적용하여
        // 본 버그를 수정한다.
        // SCT_UPDATE_DRDB_DROP_DBF 재수행을 할 경우에는 DROPPING
        // 상태로 설정하고, 해당 트랜잭션의 COMMIT 로그를 재수행할 때
        // Commit Pending Operation으로 DROPPED 상태로 설정한다.
        // for fix BUG-14978

        // Commit Pending Operation 등록한다.
        if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        {
            IDE_TEST( sddDataFile::addPendingOperation(
                          aTrans,
                          sFileNode,
                          ID_TRUE, /* commit시 동작 */
                          SCT_POP_DROP_DBF,
                          &sPendingOp )
                      != IDE_SUCCESS );

            sPendingOp->mPendingOpFunc  = sddDiskMgr::removeFilePending;
            sPendingOp->mPendingOpParam = (void*)sFileNode;
            sPendingOp->mTouchMode      = sTouchMode;

            // DBF Node의 상태를 DROPPING으로 설정한다.
            sFileNode->mState |= SMI_FILE_DROPPING;
        }
        else
        {
            // Active Tx가 아닌 경우에는 Pending 연산을
            // 추가하지 않는다.
        }
    }
    else
    {
        // noting to do..
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_DROP_DBF 로그 UNDO

순서 :   (1)           (2)     (3)         (4)        (5)
연산 : [DROP_DBF]->[DROP_TBS]->[CLR_TBS]->[CLR_DBF]->[ROLLBACK]
상태 :  |ONLINE    |ONLINE     ~DROPPING   ~DROPPING
        |DROPPING  |DROPPING

복구 알고리즘

RESTART시

가. (1)이후에 실패한 경우, TBS List에서 검색된다면 (1)를 재수행하여
    ONLINE|DROPPING 상태이기 때문에, ~DROPPING 연산을 하여 ONLINE 상태로 변경한다.

RUNTIME시

가. (1)이후에 실패하면 TBS List에서 검색하여 ~DROPPING 연산을 수행하여 ONLINE 상태로
    변경한다.

# DROP_TBS의 복구알고리즘과 거의 동일하다.

*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_DROP_DBF(
                       idvSQL *             aStatistics,
                       void*                aTrans,
                       smLSN                /* aCurLSN */,
                       scSpaceID            aSpaceID,
                       UInt                 aFileID,
                       UInt                 aValueSize,
                       SChar*               /* aValuePtr */,
                       idBool               aIsRestart )
{
    UInt                sState = 0;
    sddDataFileNode*    sFileNode = NULL;
    sddTableSpaceNode*  sSpaceNode = NULL;

    IDE_ERROR( aValueSize == 0 );
    IDE_ERROR( (aTrans != NULL) || (aIsRestart == ID_TRUE) );

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    // RUNTIME시에는 sSpaceNode 자체에 대해서 (X) 잠금이 잡혀있기 때문에
    // sctTableSpaceMgr::lock을 획득할 필요가 없다.
    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        // FileNode수정 시 lock을 잡아야 한다.
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;

        if( SMI_FILE_STATE_IS_DROPPING( sFileNode->mState ) )
        {
            // 알고리즘 RESTART (가), RUNTIME (가) 에 해당하는 경우이다.
            // DROPPING을 끄고, ONLINE 상태로 변경한다.
            sFileNode->mState &= ~SMI_FILE_DROPPING;
        }

        IDE_ERROR( SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ) );
        IDE_ERROR( SMI_FILE_STATE_IS_NOT_CREATING( sFileNode->mState ) );

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    else
    {
        // TBS List에서 검색이 되지 않으면 아무것도 하지 않는다.
        // nothing to do...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * DESCRIPTION : tbs 노드와 dbf 노드를 반환한다.
 ***********************************************************************/
IDE_RC sddUpdate::getTBSDBF( scSpaceID            aSpaceID,
                             UInt                 aFileID,
                             sddTableSpaceNode**  aSpaceNode,
                             sddDataFileNode**    aFileNode )
{

    sddTableSpaceNode*  sSpaceNode = NULL;
    sddDataFileNode*    sFileNode = NULL;

    IDE_ERROR( aSpaceNode != NULL );
    IDE_ERROR( aFileNode  != NULL );

    sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if (sSpaceNode != NULL)
    {
        IDE_ERROR( sSpaceNode->mHeader.mID == aSpaceID );

        sddTableSpace::getDataFileNodeByIDWithoutException(
                              sSpaceNode,
                              aFileID,
                              &sFileNode );

        if (sFileNode != NULL)
        {
            IDE_ERROR_MSG( aFileID == sFileNode->mID,
                           "aFileID       : %"ID_UINT32_FMT"\n"
                           "sFileNode->mID : %"ID_UINT32_FMT,
                           aFileID,
                           sFileNode->mID );
            IDE_ERROR_MSG( sSpaceNode->mNewFileID > sFileNode->mID,
                           "sSpaceNode->mNewFileID : %"ID_UINT32_FMT"\n"
                           "sFileNode->mID         : %"ID_UINT32_FMT,
                           sSpaceNode->mNewFileID,
                           sFileNode->mID );
            IDE_ERROR_MSG( SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ), 
                           "sFileNode->mState        : %"ID_UINT32_FMT,
                           sFileNode->mState );
        }
    }
    else
    {
         // nothing to do..
    }

    *aSpaceNode = sSpaceNode;
    *aFileNode  = sFileNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_EXTEND_DBF 로그 재수행

트랜잭션 Commit Pending List: [POP_DBF]

순서 :   (1)                 (2)       (3)      (4)        (5)
연산 : [RESIZE_DBF]-------->[ANCHOR]->[COMMIT]->[POP_DBF]->[ANCHOR]
상태 :  |ONLINE     (확장)    저장      커밋      ~RESIZING  저장
        |RESIZING

알고리즘

가. (1) 이전에 실패하면 커밋이 안된 트랜잭션이고, Loganchor에도 저장되지 않았기 때문에
    재수행할 것이 없다.

나. (2)과 (3) 사이에 실패하면 커밋이 안된 트랜잭션이지만, 로그앵커에 ONLINE|RESIZING 상태로
    저장되었으므로, Rollback 연산시 RESIZING인경우에만 물리적 변경량 취소와 ~RESIZING을 해주어야 한다.
    즉, RESIZING인 경우는 이미 확장이 완료된 상태이기 때문이다.

다. (3)과 (5) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE|RESIZING 상태로
    저장되었으므로, ONLINE|RESIZING 상태일 경우에만 Commit Pending 연산을 등록하여
    ONLINE 상태로 변경해주어야 한다.
    미디어복구를 고려해서 실제파일 크기와 AfterSize를 고려하여 확장을 한다

라. (5) 이후에 실패하면 커밋된 트랜잭션이고, 로그앵커에 ONLINE된 상태로 저장되었으므로
    재수행 할 것이 없다.

*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_EXTEND_DBF( idvSQL     * aStatistics,
                                                   void       * aTrans,
                                                   smLSN        /* aCurLSN */,
                                                   scSpaceID    aSpaceID,
                                                   UInt         aFileID,
                                                   UInt         aValueSize,
                                                   SChar      * aValuePtr,
                                                   idBool       /* aIsRestart */ )
{
    sddTableSpaceNode * sSpaceNode = NULL;
    sddDataFileNode   * sFileNode  = NULL;
    ULong               sFileSize  = 0;
    ULong               sCurrSize  = 0;
    ULong               sDiffSize  = 0;
    ULong               sAfterSize = 0;
    idBool              sIsPreparedIO = ID_FALSE;
    idBool              sIsNeedLogAnchorFlush = ID_FALSE;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aValuePtr != NULL );
    IDE_ERROR_MSG( aValueSize == (ID_SIZEOF(ULong)*2),
                   "aValueSize : %"ID_UINT32_FMT,
                   aValueSize );

    // AfterSize : 확장된 CURRSIZE
    idlOS::memcpy( &sAfterSize, aValuePtr, ID_SIZEOF(ULong) );

    // AfterSize : 확장된 페이지 개수
    idlOS::memcpy( &sDiffSize,
                   aValuePtr+ID_SIZEOF(ULong),
                   ID_SIZEOF(ULong) );

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (sFileNode != NULL)
    {
        // [중요]
        // 실제파일크기와 After크기를 비교하여 AFTER 크기가 크면
        // 무조건 CURRSIZE값을 변경한다.
        // SHRINK에서도 물리적 파일은 선별적으로 조정하지만,
        // CURRSIZE 값은 변경한다.

        // 현재 실제 파일의 page 개수를 구한다.
        IDE_TEST(sddDiskMgr::prepareIO( aStatistics,
                                        sFileNode ) != IDE_SUCCESS);
        sIsPreparedIO = ID_TRUE;

        IDE_TEST(sFileNode->mFile.getFileSize(&sFileSize) != IDE_SUCCESS);

        sCurrSize = (sFileSize-SM_DBFILE_METAHDR_PAGE_SIZE) /
                    SD_PAGE_SIZE;

        /* PROJ-1923 아래와 같이 중첩 if 의 순서를 변경한다.
         * 하지만, 코드이해를 위해 주석으로 유지하도록 한다.
         *
         * PRJ-1548 RESIZE 중인 경우
         * BUGBUG - Media Recovery 시에도 sFileNode의 상태가 Resize일 수 있다.
         *
         */
        /*
        * if ( SMI_FILE_STATE_IS_RESIZING( sFileNode->mState ) )
        * {
        *     // 알고리즘 (다)에 해당하는 RESIZING 상태일 경우에만 있으므로
        *     // 상태를 ONLINE으로 변경할 수 있게 Commit Pending 연산을 등록한다.
        *     if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        *     {
        *         IDE_TEST( sddDataFile::addPendingOperation(
        *                 aTrans,
        *                 sFileNode,
        *                 ID_TRUE, // commit시에 동작 
        *                 SCT_POP_ALTER_DBF_RESIZE )
        *             != IDE_SUCCESS );
        *     }
        *     else
        *     {
        *         // ActiveTx가 아닌 경우 Pending 등록하지 않는다.
        *     }
        *     // 알고리즘 (나) Rollback 연산시 물리적 변경량 취소(RESIZING인 경우에만)와
        *     // ~RESIZING을 해주어야 한다.
        * }
        * else
        * {
        *     // Pending 등록을 할 필요가 없다.
        * }
        */

        if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        {
            if( SMI_FILE_STATE_IS_RESIZING( sFileNode->mState ) )
            {
                IDE_TEST( sddDataFile::addPendingOperation(
                        aTrans,
                        sFileNode,
                        ID_TRUE, // commit시에 동작 
                        SCT_POP_ALTER_DBF_RESIZE )
                    != IDE_SUCCESS );
            }
            else
            {
                // PROJ-1923 무조건 redo로 변경한다.
                // 로그앵커의 size < log에 기록된 size가 크다면,
                // 로그만 기록하고 extend가 실행되지 않은것이므로 redo한다.
                // 그 외에는 하지 않는다.
                if( sCurrSize < sAfterSize )
                {
                    sFileNode->mState |= SMI_FILE_RESIZING;

                    IDE_TEST( sddDataFile::addPendingOperation(
                            aTrans,
                            sFileNode,
                            ID_TRUE, // commit시에 동작 
                            SCT_POP_ALTER_DBF_RESIZE )
                        != IDE_SUCCESS );

                    sIsNeedLogAnchorFlush   = ID_TRUE;
                }
                else
                {
                    // do nothing
                }
            }
        }
        else
        {
            // do nothing
        }

        // 앵커가 aftersize보다 작고, 실제크기가 aftersize보다 작은 경우는
        // 무조건 extend 한다.
        if (sCurrSize < sAfterSize)
        {
            sddDataFile::setCurrSize(sFileNode, (sAfterSize - sDiffSize));
            IDE_TEST( sddDataFile::extend(NULL, sFileNode, sDiffSize)
                      != IDE_SUCCESS );
        }

        sIsPreparedIO = ID_FALSE;
        IDE_TEST(sddDiskMgr::completeIO( aStatistics,
                                         sFileNode,
                                         SDD_IO_WRITE )
                 != IDE_SUCCESS);

        sddDataFile::setCurrSize(sFileNode, sAfterSize);

        if( sIsNeedLogAnchorFlush == ID_TRUE )
        {
            // loganchor flush
            IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                        == IDE_SUCCESS );
        }
        else
        {
            // do nothing
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsPreparedIO == ID_TRUE )
    {
        IDE_ASSERT( sddDiskMgr::completeIO( aStatistics,
                                            sFileNode,
                                            SDD_IO_WRITE ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_EXTEND_DBF 로그 UNDO

트랜잭션 Commit Pending List: [POP_DBF]

순서 :   (1)                 (2)         (3)          (4)        (5)
연산 : [EXTEND_DBF]-------->[ANCHOR]->[CLR_EXTEND]->[ANCHOR]->[ROLLBACK]
상태 :  |ONLINE     (확장)    저장     ~RESIZING      저장     완료
        |RESIZING                      (변경량 취소)

복구 알고리즘

RESTART시

가. (1)과 (2) 사이에서 실패하면 로그앵커에 ONLINE 상태이고, RESTART 상황이므로 확장량에 대해
    취소하지 않는다. (개선사항)

나. (2)에서 undo를 수행하면 완료가 안된 트랜잭션이지만, 로그앵커에 ONLINE|RESIZING 상태이기
    때문에, ONLINE(~RESIZING) 상태로 변경하고, 변경량을 취소(축소)한다.

RUNTIME시

가. (1)과 (2)에서 실패하면 ONLINE|RESIZING 상태 ~RESIZING 하고 변경량 취소한다.

*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_EXTEND_DBF(
                       idvSQL *             aStatistics,
                       void*                aTrans,
                       smLSN                /* aCurLSN */,
                       scSpaceID            aSpaceID,
                       UInt                 aFileID,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart )
{

    UInt                sState    = 0;
    UInt                sPrepared = 0;
    sddTableSpaceNode*  sSpaceNode;
    sddDataFileNode*    sFileNode = NULL;
    ULong               sBeforeSize;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aValuePtr != NULL );
    IDE_ERROR_MSG( aValueSize == ID_SIZEOF(ULong),
                   "aValueSize : %"ID_UINT32_FMT,
                   aValueSize );

    sSpaceNode = NULL;
    sFileNode  = NULL;

    idlOS::memcpy(&sBeforeSize, aValuePtr, ID_SIZEOF(ULong));

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;

        while ( SMI_FILE_STATE_IS_BACKUP( sFileNode->mState ) )
        {
            IDE_ERROR( aIsRestart != ID_TRUE );

            // fix BUG-11337.
            // 데이타 파일이 백업중이면 완료할때까지 대기 한다.
            sctTableSpaceMgr::wait4Backup( sSpaceNode );
        }

        // 운영중에 Rollback이 발생하거나  RESIZING 상태가 Loganchor에 저장된 경우
        // 실제 파일 변경량을 취소해준다.
        if ( SMI_FILE_STATE_IS_RESIZING( sFileNode->mState ) )
        {
            // 현재 실제 파일의 page 개수를 구한다.
            IDE_TEST(sddDiskMgr::prepareIO( aStatistics,
                                            sFileNode ) != IDE_SUCCESS);
            sPrepared = 1;

            IDE_TEST( sddDataFile::truncate(sFileNode, sBeforeSize) != IDE_SUCCESS );

            sPrepared = 0;
            IDE_TEST( sddDiskMgr::completeIO( aStatistics,
                                              sFileNode,
                                              SDD_IO_WRITE ) != IDE_SUCCESS );

            sFileNode->mState &= ~SMI_FILE_RESIZING;
        }
        else
        {
            // RESTART시 (3)이전에 실패한 경우 이므로 알고리즘 (가)에 해당한다.
            // NOTHING TO DO ...
        }

        sddDataFile::setCurrSize(sFileNode, sBeforeSize);

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    else
    {
        // NOTHING TO DO ...
    }

    if ( sFileNode != NULL )
    {
        /* BUG-24086: [SD] Restart시에도 File이나 TBS에 대한 상태가 바뀌었을 경우
         * LogAnchor에 상태를 반영해야 한다.
         *
         * Restart Recovery시에는 updateDBFNodeAndFlush하지 않던것을 하도록 변경.
         * */

        IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                    == IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sPrepared != 0 )
        {
            IDE_ASSERT( sddDiskMgr::completeIO( aStatistics,
                                                sFileNode,
                                                SDD_IO_WRITE )
                        == IDE_SUCCESS );
        }

        if ( sState != 0 )
        {
            sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_SHRINK_DBF 로그 재수행

트랜잭션 Commit Pending List: [POP_DBF]

순서 :   (1)                 (2)       (3)      (4)        (5)
연산 : [SHRINK_DBF]-------->[ANCHOR]->[COMMIT]->[POP_DBF]->[ANCHOR]
상태 :  |ONLINE     (축소)    저장      커밋    ~RESIZING  저장
        |RESIZING

알고리즘

가. (1) 이전에 실패하면 커밋이 안된 트랜잭션이고, Loganchor에도 저장되지 않았기 때문에
    재수행할 것이 없다.

나. (2)과 (3) 사이에 실패하면 커밋이 안된 트랜잭션이지만, 로그앵커에 ONLINE|RESIZING 상태로
    저장되었으므로, Rollback 연산시 RESIZING인경우에만 물리적 변경량 취소와 ~RESIZING을 해주어야 한다.
    즉, RESIZING인 경우는 이미 축소가 완료된 상태이기 때문이다.

다. (3)과 (5) 사이에서 실패하면 커밋된 트랜잭션이지만, 로그앵커에 ONLINE|RESIZING 상태로
    저장되었으므로, ONLINE|RESIZING 상태일 경우에만 Commit Pending 연산을 등록하여
    ONLINE 상태로 변경해주어야 한다.
    미디어복구를 고려해서 실제파일 크기와 AfterSize를 고려하여 확장을 한다

라. (5) 이후에 실패하면 커밋된 트랜잭션이고, 로그앵커에 ONLINE된 상태로 저장되었으므로
    재수행 할 것이 없다.

*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_SHRINK_DBF(
                                            idvSQL    * aStatistics,
                                            void      * aTrans,
                                            smLSN       /* aCurLSN */,
                                            scSpaceID   aSpaceID,
                                            UInt        aFileID,
                                            UInt        aValueSize,
                                            SChar     * aValuePtr,
                                            idBool      /* aIsRestart */ )
{
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    ULong               sAfterInitSize;
    ULong               sAfterCurrSize;
    ULong               sDiffSize;
    ULong               sFileSize   = 0;
    ULong               sCurrSize   = 0;
    SLong               sResizePageSize = 0;
    idBool              sIsNeedLogAnchorFlush   = ID_FALSE;
    sctPendingOp      * sPendingOp;
    UInt                sState = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aValuePtr != NULL );
    IDE_DASSERT( aValueSize == (ID_SIZEOF(ULong)*3));
    ACP_UNUSED( aValueSize );

    idlOS::memcpy( &sAfterInitSize, aValuePtr, ID_SIZEOF(ULong) );

    idlOS::memcpy( &sAfterCurrSize, aValuePtr + ID_SIZEOF(ULong),
                   ID_SIZEOF(ULong) );

    idlOS::memcpy( &sDiffSize, aValuePtr + (ID_SIZEOF(ULong)*2),
                   ID_SIZEOF(ULong) );

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        // 현재 실제 파일의 page 개수를 구한다.
        IDE_TEST( sddDiskMgr::prepareIO( aStatistics,
                                         sFileNode ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST(sFileNode->mFile.getFileSize(&sFileSize) != IDE_SUCCESS);

        // BUG-47364 prepareIO 후에 completeIO 를 하지 않는 문제 해결
        sState = 0;
        IDE_TEST( sddDiskMgr::completeIO( aStatistics,
                                          sFileNode,
                                          SDD_IO_READ ) != IDE_SUCCESS );

        sCurrSize = (sFileSize-SM_DBFILE_METAHDR_PAGE_SIZE) /
            SD_PAGE_SIZE;

        /* PROJ-1923 아래와 같이 중첩 if 의 순서를 변경한다.
         * 하지만, 코드이해를 위해 주석으로 유지하도록 한다. */
        /*
         * // PRJ-1548 RESIZE 중인 경우
         * if ( SMI_FILE_STATE_IS_RESIZING( sFileNode->mState ) )
         * {
         *     // 알고리즘 (다)에 해당하는 RESIZING 상태일 경우에만 있으므로
         *     // 상태를 ONLINE으로 변경할 수 있게 Commit Pending 연산을
         *     // 등록한다.
         *     if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
         *     {
         *         IDE_TEST( sddDataFile::addPendingOperation(
         *                                   aTrans,
         *                                   sFileNode,
         *                                   ID_TRUE, // commit시에 동작
         *                                   SCT_POP_ALTER_DBF_RESIZE )
         *                   != IDE_SUCCESS );
         *     }
         *     else
         *     {
         *         // ActiveTx가 아닌 경우 Pending 등록하지 않는다.
         *     }
         * 
         *     // 만약 Commit하지 못한 트랜잭션이라면 Rollback Pending
         *     // 연산이기 때문에
         *     // undo_SCT_UPDATE_DRDB_EXTEND_DBF()에서 POP_DROP_DBF 에서
         *     // 등록한다.
         * }
         * else
         * {
         *     // Pending 등록을 할 필요가 없다.
         * }
         */

        if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        {
            if ( SMI_FILE_STATE_IS_RESIZING( sFileNode->mState ) )
            {
                IDE_TEST( sddDataFile::addPendingOperation(
                              aTrans,
                              sFileNode,
                              ID_TRUE, // commit시에 동작
                              SCT_POP_ALTER_DBF_RESIZE )
                          != IDE_SUCCESS );
            }
            else
            {
                // PROJ-1923 무조건 redo로 변경한다.
                // 로그앵커의 size > log 에 기록된 size가 작다면,
                // 로그만 기록하고 shrink 가 실행되지 않은것이므로
                // redo 한다. 그 외에는 하지 않는다.
                if( sCurrSize > sAfterCurrSize )
                {
                    sFileNode->mState |= SMI_FILE_RESIZING;

                    IDE_TEST( sddDataFile::addPendingOperation(
                                  aTrans,
                                  sFileNode,
                                  ID_TRUE, // commit시에 동작
                                  SCT_POP_ALTER_DBF_RESIZE,
                                  &sPendingOp )
                              != IDE_SUCCESS );

                    sResizePageSize = sDiffSize * -1;

                    sPendingOp->mPendingOpFunc    = sddDiskMgr::shrinkFilePending;
                    sPendingOp->mPendingOpParam   = (void *)sFileNode;
                    sPendingOp->mResizePageSize   = sResizePageSize;
                    sPendingOp->mResizeSizeWanted = sAfterInitSize; // aSizeWanted;

                    sIsNeedLogAnchorFlush   = ID_TRUE;
                }
            }
        }
        else
        {
            // do nothing
        }

        // [중요]
        // 무조건 CURRSIZE값을 변경한다.
        // EXTEND에서도 물리적 파일은 선별적으로 조정하지만,
        // CURRSIZE 값은 변경한다.

        sddDataFile::setInitSize(sFileNode, sAfterInitSize);
        sddDataFile::setCurrSize(sFileNode, sAfterCurrSize);

        if( sIsNeedLogAnchorFlush == ID_TRUE )
        {
            // loganchor flush
            IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                        == IDE_SUCCESS );
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        // nothing to do..
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sddDiskMgr::completeIO( aStatistics,
                                            sFileNode,
                                            SDD_IO_READ ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/*
PRJ-1548 User Memory Tablespace

SCT_UPDATE_DRDB_SHRINK_DBF 로그 UNDO

트랜잭션 Commit Pending List: [POP_DBF]

순서 :   (1)                 (2)         (3)          (4)        (5)
연산 : [SHRINK_DBF]-------->[ANCHOR]->[CLR_SHRINK]->[ANCHOR]->[ROLLBACK]
상태 :  |ONLINE               저장     ~RESIZING (축소)      저장     완료
        |RESIZING                      (변경량 확장)

복구 알고리즘

RESTART시

가. (1)과 (2) 사이에서 실패하면 로그앵커에 ONLINE 상태이고, RESTART 상황이므로 축소량에 대해
    취소하지 않는다. (개선사항)

나. (2)에서 undo를 수행하면 완료가 안된 트랜잭션이지만, 로그앵커에 ONLINE|RESIZING 상태이기
    때문에, ONLINE(~RESIZING) 상태로 변경하고, 변경량을 취소한다.

RUNTIME시

가. (1)과 (2) 사이에서는 before 이미지로 변경해준다.

나. (3)에서 ONLINE|RESIZING 상태 ~RESIZING 하고 변경량 취소한다.

*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_SHRINK_DBF(
                                       idvSQL *             aStatistics,
                                       void*                aTrans,
                                       smLSN                /* aCurLSN */,
                                       scSpaceID            aSpaceID,
                                       UInt                 aFileID,
                                       UInt                 aValueSize,
                                       SChar*               aValuePtr,
                                       idBool               /* aIsRestart */)
{
    UInt                sState = 0;
    ULong               sBeforeInitSize;
    ULong               sBeforeCurrSize;
    ULong               sDiffSize;
    sddTableSpaceNode*  sSpaceNode;
    sddDataFileNode*    sFileNode;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aValueSize == (ID_SIZEOF(ULong)*3) );
    IDE_DASSERT( aValuePtr != NULL );
    
    ACP_UNUSED( aTrans );
    ACP_UNUSED( aValueSize );

    idlOS::memcpy( &sBeforeInitSize, aValuePtr, ID_SIZEOF(ULong) );

    idlOS::memcpy( &sBeforeCurrSize,
                   aValuePtr + ID_SIZEOF(ULong),
                   ID_SIZEOF(ULong) );

    idlOS::memcpy( &sDiffSize,
                   aValuePtr + (ID_SIZEOF(ULong)*2),
                   ID_SIZEOF(ULong) );

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;
        // fix BUG-11337.
        // 데이타 파일이 백업중이면 완료할때까지 대기 한다.
        while ( SMI_FILE_STATE_IS_BACKUP( sFileNode->mState ) )
        {
            // 아래함수에서 SpaceNode::lock 을 해제했다가
            // 다시 획득해서 return 됨.

            sctTableSpaceMgr::wait4Backup( sSpaceNode );
        }

        // 운영중에 Rollback이 발생하거나  RESIZING 상태가 Loganchor에 저장된 경우
        // 실제 파일에 대해서 변경된것이 없기 때문에 상태값만 뺀다.
        if ( SMI_FILE_STATE_IS_RESIZING( sFileNode->mState ) )
        {
            // RESTART시 (3)이전에 실패한 경우 이므로 알고리즘 (가)에 해당한다.
            // NOTHING TO DO ...
        }

        // TBS Node에 X 잠금을 획득한 상태이므로 lock을 획득할 필요없다.
        sddDataFile::setInitSize(sFileNode, sBeforeInitSize);
        sddDataFile::setCurrSize(sFileNode, sBeforeCurrSize);

        sFileNode->mState &= ~SMI_FILE_RESIZING;

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    else
    {
        // 서버구동시에는 Nothing To do...
    }

    if ( sFileNode != NULL )
    {
        /* BUG-24086: [SD] Restart시에도 File이나 TBS에 대한 상태가 바뀌었을 경우
         * LogAnchor에 상태를 반영해야 한다.
         *
         * Restart Recovery시에는 updateDBFNodeAndFlush하지 않던것을 하도록 변경.
         * */

        IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                    == IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * DESCRIPTION : datafile autoextend mode에 대한 redo 수행
 * SMR_LT_TBS_UPDATE : SCT_UPDATE_DRDB_AUTOEXTEND_DBF
 * After  image : datafile attribute
 **********************************************************************/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_AUTOEXTEND_DBF(
                                   idvSQL             * /* aStatistics */,
                                   void               * aTrans,
                                   smLSN                /* aCurLSN */,
                                   scSpaceID            aSpaceID,
                                   UInt                 aFileID,
                                   UInt                 aValueSize,
                                   SChar              * aValuePtr,
                                   idBool               /* aIsRestart */ )
{

    idBool              sAutoExtMode;
    ULong               sMaxSize;
    ULong               sNextSize;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;

    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aValuePtr  != NULL );
    IDE_DASSERT( aValueSize == ID_SIZEOF(idBool) + ID_SIZEOF(ULong) * 2);

    ACP_UNUSED( aTrans );
    ACP_UNUSED( aValueSize );

    idlOS::memcpy(&sAutoExtMode, aValuePtr, ID_SIZEOF(idBool));
    aValuePtr += ID_SIZEOF(idBool);

    idlOS::memcpy(&sNextSize, aValuePtr, ID_SIZEOF(ULong));
    aValuePtr += ID_SIZEOF(ULong);

    idlOS::memcpy(&sMaxSize, aValuePtr, ID_SIZEOF(ULong));

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        sddDataFile::setAutoExtendProp(sFileNode, sAutoExtMode, sNextSize, sMaxSize);

        // loganchor flush
        IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                    == IDE_SUCCESS );
    }
    else
    {
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * DESCRIPTION : datafile autoextend mode에 대한 undo 수행
 * SMR_LT_TBS_UPDATE : SCT_UPDATE_DRDB_AUTOEXTEND_DBF
 * before image : datafile attribute
 **********************************************************************/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_AUTOEXTEND_DBF(
                       idvSQL *             aStatistics,
                       void*                aTrans,
                       smLSN                /* aCurLSN */,
                       scSpaceID            aSpaceID,
                       UInt                 aFileID,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart )
{

    UInt               sState = 0;
    idBool             sAutoExtMode;
    ULong              sNextSize;
    ULong              sMaxSize;
    sddTableSpaceNode* sSpaceNode;
    sddDataFileNode*   sFileNode;

    IDE_ERROR( (aTrans != NULL) || (aIsRestart == ID_TRUE) );
    IDE_DASSERT( aValuePtr != NULL );
    IDE_DASSERT( aValueSize == ID_SIZEOF(idBool) + ID_SIZEOF(ULong) * 2);

    ACP_UNUSED( aValueSize );

    idlOS::memcpy(&sAutoExtMode, aValuePtr, ID_SIZEOF(idBool));
    aValuePtr += ID_SIZEOF(idBool);

    idlOS::memcpy(&sNextSize, aValuePtr, ID_SIZEOF(ULong));
    aValuePtr += ID_SIZEOF(ULong);

    idlOS::memcpy(&sMaxSize, aValuePtr, ID_SIZEOF(ULong));

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;

        sddDataFile::setAutoExtendProp(sFileNode, sAutoExtMode, sNextSize, sMaxSize);

        /* BUG-24086: [SD] Restart시에도 File이나 TBS에 대한 상태가 바뀌었을 경우
         * LogAnchor에 상태를 반영해야 한다.
         *
         * Restart Recovery시에는 updateDBFNodeAndFlush하지 않던것을 하도록 변경.
         * */

        IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode ) == IDE_SUCCESS );

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    else
    {
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}


/*
    ALTER TABLESPACE TBS1 OFFLINE .... 에 대한 REDO 수행

    [ 로그 구조 ]
    After Image  --------------------------------------------
      UInt                aAState

    [ ALTER TABLESPACE OFFLINE 의 REDO 처리 ]
     Offline에 대한 REDO로 TBSNode.Status 에 대한
     Commit Pending Operation 등록
     (note-1) TBSNode를 loganchor에 flush하지 않음
              -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문
     (note-2) Commit Pending시 Resource 해제를 수행하지 않음
              -> Restart Recovery완료후 OFFLINE TBS에 대한 Resource해제를 한다
*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE( idvSQL      * /* aStatistics */,
                                                          void        * aTrans,
                                                          smLSN         /* aCurLSN */,
                                                          scSpaceID     aSpaceID,
                                                          UInt          /*aFileID*/,
                                                          UInt          aValueSize,
                                                          SChar       * aValuePtr,
                                                          idBool        /*aIsRestart*/ )
{
    UInt                sTBSState;
    sddTableSpaceNode * sTBSNode;
    sctPendingOp      * sPendingOp;

    sTBSNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        IDE_TEST( getAlterTBSOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sTBSState ) != IDE_SUCCESS );

        if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        {
            // Commit Pending등록
            // Transaction Commit시에 수행할 Pending Operation등록
            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                        aTrans,
                        sTBSNode->mHeader.mID,
                        ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                        SCT_POP_ALTER_TBS_OFFLINE,
                        & sPendingOp ) != IDE_SUCCESS );

            // Commit시 sctTableSpaceMgr::executePendingOperation에서
            // (note-2) Commit Pending시 Resource 해제를 수행하지 않는다.
            sPendingOp->mPendingOpFunc =
                         smLayerCallback::alterTBSOfflineCommitPending;
            sPendingOp->mNewTBSState   = sTBSState;

            sTBSNode->mHeader.mState |= SMI_TBS_SWITCHING_TO_OFFLINE;
        }
        else
        {
            // ActiveTx가 아닌 경우 Pending 등록하지 않는다.
        }
    }
    else
    {
        // 이미 Drop된 Tablespace인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    ALTER TABLESPACE TBS1 OFFLINE .... 에 대한 UNDO 수행

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      UInt                aBState

    [ ALTER TABLESPACE OFFLINE 의 UNDO 처리 ]
      (u-010) (020)에 대한 UNDO로 TBSNode.Status := Before Image(ONLINE)
      (note-1) TBSNode를 loganchor에 flush하지 않음
                => Restart Recovery 이후에 처리됨.
*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE( idvSQL      * /* aStatistics */,
                                                          void        * /*aTrans*/,
                                                          smLSN         /* aCurLSN */,
                                                          scSpaceID     aSpaceID,
                                                          UInt          /*aFileID*/,
                                                          UInt          aValueSize,
                                                          SChar       * aValuePtr,
                                                          idBool        /*aIsRestart*/)
{
    UInt                 sTBSState;
    sddTableSpaceNode  * sTBSNode;

    sTBSNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        IDE_TEST( getAlterTBSOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sTBSState ) != IDE_SUCCESS );

        // 운영중이거나 Restart Recovery시 모두
        // Switch_to_Offline -> Before State로 복원한다.
        sTBSNode->mHeader.mState = sTBSState;
    }
    else
    {
        // 이미 Drop된 Tablespace인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    ALTER TABLESPACE TBS1 ONLINE .... 에 대한 REDO 수행

    [ 로그 구조 ]
    After Image  --------------------------------------------
      UInt                aAState

    [ ALTER TABLESPACE ONLINE 의 REDO 처리 ]
    (r-010) TBSNode.Status := After Image(SW)
      (note-1) TBSNode를 loganchor에 flush하지 않음
               -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문
*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_TBS_ONLINE( idvSQL       * /* aStatistics */,
                                                         void         * aTrans,
                                                         smLSN          /* aCurLSN */,
                                                         scSpaceID      aSpaceID,
                                                         UInt           /*aFileID*/,
                                                         UInt           aValueSize,
                                                         SChar        * aValuePtr,
                                                         idBool         /*aIsRestart*/ )
{
    UInt                 sTBSState;
    sddTableSpaceNode  * sTBSNode;
    sctPendingOp       * sPendingOp;

    sTBSNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        IDE_TEST( getAlterTBSOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sTBSState ) != IDE_SUCCESS );

        if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        {
            // Commit Pending등록
            // Transaction Commit시에 수행할 Pending Operation등록
            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                        aTrans,
                        sTBSNode->mHeader.mID,
                        ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                        SCT_POP_ALTER_TBS_ONLINE,
                        & sPendingOp ) != IDE_SUCCESS );

            // Commit시 sctTableSpaceMgr::executePendingOperation
            // (note-2) Commit Pending시 Resource 해제를 수행하지 않는다.
            sPendingOp->mPendingOpFunc =
                        smLayerCallback::alterTBSOnlineCommitPending;
            sPendingOp->mNewTBSState   = sTBSState;

            sTBSNode->mHeader.mState |= SMI_TBS_SWITCHING_TO_ONLINE;
        }
        else
        {
            // ActiveTx가 아닌 경우 Pending 등록하지 않는다.
        }
    }
    else
    {
        // 이미 Drop된 Tablespace인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    ALTER TABLESPACE TBS1 ONLINE .... 에 대한 UNDO 수행

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      UInt                aBState

    [ ALTER TABLESPACE ONLINE 의 UNDO 처리 ]
      (u-050)  TBSNode.Status := Before Image(OFFLINE)
      (note-1) TBSNode를 loganchor에 flush하지 않음
               -> ALTER TBS ONLINEE의 Commit Pending을 통해
                  COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문
*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_TBS_ONLINE( idvSQL       * /* aStatistics */,
                                                         void         * /*aTrans*/,
                                                         smLSN          /* aCurLSN */,
                                                         scSpaceID      aSpaceID,
                                                         UInt           /*aFileID*/,
                                                         UInt           aValueSize,
                                                         SChar        * aValuePtr,
                                                         idBool         /*aIsRestart*/ )
{
    UInt                sTBSState;
    sddTableSpaceNode * sTBSNode;

    sTBSNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        IDE_TEST( getAlterTBSOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sTBSState ) != IDE_SUCCESS );

        sTBSNode->mHeader.mState = sTBSState;
    }
    else
    {
        // 이미 Drop된 Tablespace인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    ALTER TABLESPACE TBS1 ONLINE/OFFLINE .... 에 대한 Log Image를 분석한다.

    [IN]  aValueSize     - Log Image 의 크기
    [IN]  aValuePtr      - Log Image
    [OUT] aState         - Tablespace의 상태
 */
IDE_RC sddUpdate::getAlterTBSOnOffImage( UInt       aValueSize,
                                         SChar    * aValuePtr,
                                         UInt     * aState )
{
    IDE_DASSERT( aValuePtr != NULL );
    IDE_DASSERT( aValueSize == (UInt)( ID_SIZEOF(*aState)));
    IDE_DASSERT( aState   != NULL );
    
    ACP_UNUSED( aValueSize );

    idlOS::memcpy(aState, aValuePtr, ID_SIZEOF(*aState));
    aValuePtr += ID_SIZEOF(*aState);

    return IDE_SUCCESS;
}

/*
    DRDB_ALTER_OFFLINE_DBF 에 대한 REDO 수행

    [ 로그 구조 ]
    After Image  --------------------------------------------
      UInt                aAState

    [ OFFLINE DBF의 REDO 처리 ]
     Offline에 대한 REDO로 DBFNode.Status 에 대한 Commit Pending Operation 등록

     (note-1) DBFNode를 loganchor에 flush하지 않음
              -> Restart Recovery완료후 모든 DBF를 loganchor에 flush하기 때문
     (note-2) Commit Pending시 Resource 해제를 수행하지 않음
              -> DBF 레벨에서는 처리할 것이 없음. ( Pending 함수 불필요 )
*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE( idvSQL      * /* aStatistics */,
                                                          void        * aTrans,
                                                          smLSN         /* aCurLSN */,
                                                          scSpaceID     aSpaceID,
                                                          UInt          aFileID,
                                                          UInt          aValueSize,
                                                          SChar       * aValuePtr,
                                                          idBool        /*aIsRestart*/ )
{
    UInt                 sDBFState;
    sddTableSpaceNode  * sSpaceNode;
    sddDataFileNode    * sFileNode;
    sctPendingOp       * sPendingOp;

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        IDE_TEST( getAlterDBFOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sDBFState ) != IDE_SUCCESS );

        if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        {
            // Commit Pending등록
            // Transaction Commit시에 수행할 Pending Operation등록
            IDE_TEST( sddDataFile::addPendingOperation(
                          aTrans,
                          sFileNode,
                          ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                          SCT_POP_ALTER_DBF_OFFLINE,
                          & sPendingOp ) != IDE_SUCCESS );

            // Commit시 sctTableSpaceMgr::executePendingOperation에서
            // (note-2) Commit Pending시 Resource 해제를 수행하지 않는다.
            sPendingOp->mNewDBFState = sDBFState;
            // pending 시 처리할 함수가 없다.
            sPendingOp->mPendingOpFunc = NULL;
            
            // loganchor flush
            IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode )
                        == IDE_SUCCESS );
        }
        else
        {
            // ActiveTx가 아닌 경우 Pending 등록하지 않는다.
        }
    }
    else
    {
        // 이미 Drop된 DBF인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    DRDB_ALTER_OFFLINE_DBF 에 대한 UNDO 수행

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      UInt                aBState

    [ ALTER TABLESPACE OFFLINE 의 UNDO 처리 ]
      (u-010) (020)에 대한 UNDO로 DBFNode.Status := Before Image
      (note-1) DBFNode를 loganchor에 flush하지 않음
               commit 되지 않은 offline 연산은 loganchor에 offline 상태가
               내려갈수 없기 때문에 undo시에는 loganchor에 이전 상태를
               flush할 필요없다.
*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE(
                        idvSQL    * /* aStatistics */,
                        void      * /*aTrans*/,
                        smLSN       /* aCurLSN */,
                        scSpaceID   aSpaceID,
                        UInt        aFileID,
                        UInt        aValueSize,
                        SChar     * aValuePtr,
                        idBool      /*aIsRestart*/)
{
    UInt                 sDBFState;
    sddTableSpaceNode  * sSpaceNode;
    sddDataFileNode    * sFileNode;

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        IDE_TEST( getAlterDBFOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sDBFState ) != IDE_SUCCESS );

        // (u-010)
        // 운영중이거나 Restart Recovery시 모두 Before State로 복원한다.
        sFileNode->mState = sDBFState;
    }
    else
    {
        // 이미 Drop된 DBF인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    DRDB_ALTER_ONLINE_DBF 에 대한 REDO 수행

    [ 로그 구조 ]
    After Image  --------------------------------------------
      UInt                aAState

    [ ONLINE DBF의 REDO 처리 ]
    Online 에 대한 REDO로 DBFNode.Status에 대한 Commit Pending Operation 등록

    (note-1) DBFNode를 loganchor에 flush하지 않음
             -> Restart Recovery완료후 모든 DBF를 loganchor에 flush하기 때문
     (note-2) Commit Pending시 Resource 해제를 수행하지 않음
              -> DBF 레벨에서는 처리할 것이 없음. ( Pending 함수 불필요 )

*/
IDE_RC sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_DBF_ONLINE(
                        idvSQL    * /* aStatistics */,
                        void      * aTrans,
                        smLSN       /* aCurLSN */,
                        scSpaceID   aSpaceID,
                        UInt        aFileID,
                        UInt        aValueSize,
                        SChar     * aValuePtr,
                        idBool      /*aIsRestart*/ )
{
    UInt                 sDBFState;
    sddTableSpaceNode  * sSpaceNode;
    sddDataFileNode    * sFileNode;
    sctPendingOp       * sPendingOp;

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        IDE_TEST( getAlterTBSOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sDBFState ) != IDE_SUCCESS );

        if ( smLayerCallback::isBeginTrans( aTrans ) == ID_TRUE )
        {
            // Commit Pending등록
            // Transaction Commit시에 수행할 Pending Operation등록
            IDE_TEST( sddDataFile::addPendingOperation(
                          aTrans,
                          sFileNode,
                          ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                          SCT_POP_ALTER_DBF_ONLINE,
                          & sPendingOp ) != IDE_SUCCESS );

            // Commit시 sctTableSpaceMgr::executePendingOperation
            // (note-2) Commit Pending시 Resource 해제를 수행하지 않는다.
            sPendingOp->mNewDBFState   = sDBFState;
            // pending 시 처리할 함수가 없다.
            sPendingOp->mPendingOpFunc = NULL;
        }
        else
        {
            // ActiveTx가 아닌 경우 Pending 등록하지 않는다.
        }
    }
    else
    {
        // 이미 Drop된 DBF인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    ALTER TABLESPACE TBS1 ONLINE .... 에 대한 UNDO 수행

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      UInt                aBState

    [ ALTER TABLESPACE ONLINE 의 UNDO 처리 ]
      (u-050)  TBSNode.Status := Before Image(OFFLINE)
      (note-1) TBSNode를 loganchor에 flush하지 않음
               -> ALTER TBS ONLINEE의 Commit Pending을 통해
                  COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문
*/
IDE_RC sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_DBF_ONLINE(
                        idvSQL        * /* aStatistics */,
                        void          * /*aTrans*/,
                        smLSN           /* aCurLSN */,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /*aIsRestart*/ )
{
    UInt                sDBFState;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;

    IDE_TEST( getTBSDBF( aSpaceID, aFileID, &sSpaceNode, &sFileNode )
              != IDE_SUCCESS );

    if (( sSpaceNode != NULL ) &&
        ( sFileNode  != NULL ))
    {
        IDE_TEST( getAlterDBFOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sDBFState ) != IDE_SUCCESS );

        // (u-010)
        // 운영중이거나 Restart Recovery시 모두 Before State로 복원한다.
        sFileNode->mState = sDBFState;
    }
    else
    {
        // 이미 Drop된 DBF인 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    DRDB_ALTER_ONLINE_DBF/OFFLINE_DBF에 대한 Log Image를 분석한다.

    [IN]  aValueSize     - Log Image 의 크기
    [IN]  aValuePtr      - Log Image
    [OUT] aState         - DBF의 상태
*/
IDE_RC sddUpdate::getAlterDBFOnOffImage( UInt       aValueSize,
                                         SChar    * aValuePtr,
                                         UInt     * aState )
{
    IDE_DASSERT( aValuePtr != NULL );
    IDE_DASSERT( aValueSize == (UInt)( ID_SIZEOF(*aState)));
    IDE_DASSERT( aState   != NULL );

    ACP_UNUSED( aValueSize );

    idlOS::memcpy(aState, aValuePtr, ID_SIZEOF(*aState));
    aValuePtr += ID_SIZEOF(*aState);

    return IDE_SUCCESS;
}
