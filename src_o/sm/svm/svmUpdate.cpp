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
#include <ideErrorMgr.h>
#include <svmReq.h>
#include <svmUpdate.h>
#include <svmExpandChunk.h>
#include <sctTableSpaceMgr.h>

/*
 * Update type : SCT_UPDATE_VRDB_CREATE_TBS
 *
 * Volatile tablespace 생성에 대한 redo 수행
 *
 * Commit전에 생성된 Log Anchor에 Tablespace를 Flush하였기 때문에
 * Tablespace의 Attribute나 Checkpoint Path와 같은 정보들에 대해
 * 별도의 Redo를 수행할 필요가 없다.
 *
 * Disk Tablespace와 동일하게 처리한다.
 *  - sddUpdate::redo_SCT_UPDATE_DRDB_CREATE_TBS 의 주석 참고
 *    ( 여기에 주석의 알고리즘 가,나,다,...와 같은 기호가 기술되어 있음)
 */
IDE_RC svmUpdate::redo_SCT_UPDATE_VRDB_CREATE_TBS(
                    idvSQL        * /*aStatistics*/,
                    void          * aTrans,
                    smLSN           /* sCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /* aFileID */,
                    UInt            aValueSize,
                    SChar         * /* aValuePtr */,
                    idBool          /* aIsRestart */ )
{
    svmTBSNode     *  sSpaceNode;
    sctPendingOp   *  sPendingOp;

    IDE_ERROR( aTrans != NULL );

    // After Image가 없어야 한다.
    IDE_ERROR( aValueSize == 0 );

    // Loganchor로부터 초기화된 TBS List를 검색한다. 
    sSpaceNode = (svmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sSpaceNode != NULL )
    {
        if ( SMI_TBS_IS_CREATING(sSpaceNode->mHeader.mState) )
        {
            // 알고리즘 (다)에 해당하는 CREATINIG 상태일 경우에만 있으므로 
            // 상태를 ONLINE으로 변경할 수 있게 Commit Pending 연산을 등록한다. 

            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                                    aTrans,
                                                    aSpaceID,
                                                    ID_TRUE, /* commit시에 동작 */
                                                    SCT_POP_CREATE_TBS,
                                                    & sPendingOp )
                      != IDE_SUCCESS );

            sPendingOp->mPendingOpFunc = svmTBSCreate::createTableSpacePending;
            
            // 알고리즘 (나)에 해당하는 것은 Rollback Pending 연산이기 때문에
            // undo_SCT_UPDATE_DRDB_CREATE_TBS()에서 POP_DROP_TBS 에서 등록한다.
        }
        else
        {
            // 알고리즘 (라) 에 해당하므로 재수행하지 않는다. 
        }
    }
    else
    {
        // 알고리즘 (가) 에 해당하므로 재수행하지 않는다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Update type : SCT_UPDATE_VRDB_CREATE_TBS
 *
 * Volatile tablespace 생성에 대한 undo 수행.
 *
 * Disk Tablespace와 동일하게 처리한다.
 *  - sddUpdate::undo_SCT_UPDATE_DRDB_CREATE_TBS 의 주석 참고
 *    ( 여기에 주석의 알고리즘 가,나,다,...와 같은 기호가 기술되어 있음)
 * 
 * after image : tablespace attribute
 */
IDE_RC svmUpdate::undo_SCT_UPDATE_VRDB_CREATE_TBS(
                    idvSQL        * aStatistics,
                    void          * aTrans,
                    smLSN           /* sCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /* aFileID */,
                    UInt            /* aValueSize */,
                    SChar         * /* aValuePtr */,
                    idBool          /* aIsRestart */  )
{
    UInt             sState = 0;
    svmTBSNode     * sSpaceNode;
    sctPendingOp   * sPendingOp;

    // TBS List를 검색한다. 
    sSpaceNode = (svmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sSpaceNode != NULL )
    {
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;

        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED) != SMI_TBS_DROPPED );

        if ( SMI_TBS_IS_CREATING(sSpaceNode->mHeader.mState) )
        {
            // CREATE TBS 연산에서는 
            // 어느 과정에서 실패하던지 Loganchor에 DROPPING상태가 저장될 수 없으므로 
            // RESTART시에는 DROPPING 상태가 있을 수 없음.
            IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPING) 
                       != SMI_TBS_DROPPING );

            // RESTART 알고리즘 (가),(나)에 해당한다.
            // RUNTIME 알고리즘 (나)에 해당한다.

            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                                  aTrans,
                                                  sSpaceNode->mHeader.mID,
                                                  ID_FALSE, /* abort 시 동작 */
                                                  SCT_POP_DROP_TBS,
                                                  &sPendingOp) != IDE_SUCCESS );

            // Pending 함수 설정.
            // 처리 : Tablespace와 관련된 모든 메모리와 리소스를 반납한다.
            sPendingOp->mPendingOpFunc = svmTBSDrop::dropTableSpacePending;

            // 생성중이던 Checkpoint Image모두 제거 
            sPendingOp->mTouchMode = SMI_ALL_TOUCH ;
            
            sSpaceNode->mHeader.mState |= SMI_TBS_DROPPING;
        }
        else
        {
            // 알고리즘 RESTART (다)에 위배된다.
            // nothing to do ..
            IDE_DASSERT( 0 );
        }

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    else
    {
        // RESTART 알고리즘 (다) 해당
        // RUNTIME 알고리즘 (가) 해당
        // nothing to do ...
    }

    // RUNTIME시에 변경이 발생했다면 Rollback Pending이 등록되었을 것이고
    // Rollback Pending시 Loganchor를 갱신한다. 
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    
    return IDE_FAILURE;    
}

/*
 * Update type : SCT_UPDATE_VRDB_DROP_TBS
 *
 * Volatile tablespace Drop에 대한 redo 수행.
 *
 * Disk Tablespace와 동일하게 처리한다.
 *  - sddUpdate::redo_SCT_UPDATE_DRDB_DROP_TBS 의 주석 참고
 *    ( 여기에 주석의 알고리즘 가,나,다,...와 같은 기호가 기술되어 있음)
 * 
 *   [로그 구조]
 *   After Image 기록 ----------------------------------------
 *      smiTouchMode  aTouchMode
 */
IDE_RC svmUpdate::redo_SCT_UPDATE_VRDB_DROP_TBS(
                    idvSQL        * /*aStatistics*/,
                    void          * aTrans,
                    smLSN           /* sCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /* aFileID */,
                    UInt            aValueSize,
                    SChar         * /*aValuePtr*/,
                    idBool          /* aIsRestart */  )
{
    svmTBSNode        * sSpaceNode;
    sctPendingOp      * sPendingOp;
    
    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aValueSize == 0 );

    // TBS List에서 검색한다. 
    sSpaceNode = (svmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sSpaceNode != NULL )
    {
        // DROPPED상태의 TBS는 findSpaceNodeWithoutException에서 건너뛴다
        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED)
                   != SMI_TBS_DROPPED );
        
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
            sSpaceNode->mHeader.mState |= SMI_TBS_DROPPING;
            
            // 알고리즘 (가), (나)에 해당하는 경우 Commit Pending 연산 등록
            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                            aTrans,
                                            sSpaceNode->mHeader.mID,
                                            ID_TRUE, /* commit시에 동작 */
                                            SCT_POP_DROP_TBS,
                                            &sPendingOp)
                      != IDE_SUCCESS );

            // Pending 함수 설정.
            // 처리 : Tablespace와 관련된 모든 메모리와 리소스를 반납한다.
            sPendingOp->mPendingOpFunc = svmTBSDrop::dropTableSpacePending;

            /* Volatile TBS drop에서는 touch mode를 고려하지 않는다. */
            sPendingOp->mTouchMode     = SMI_ALL_NOTOUCH;
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
 * Update type : SCT_UPDATE_VRDB_DROP_TBS
 *
 * Volatile tablespace 제거에 대한 undo 수행
 *
 * Disk Tablespace와 동일하게 처리한다.
 *  - sddUpdate::undo_SCT_UPDATE_DRDB_DROP_TBS 의 주석 참고
 * 
 * before image : tablespace attribute
 */
IDE_RC svmUpdate::undo_SCT_UPDATE_VRDB_DROP_TBS(
                    idvSQL        * aStatistics,
                    void          * /* aTrans */,
                    smLSN           /* sCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /* aFileID */,
                    UInt            aValueSize ,
                    SChar         * /* aValuePtr */,
                    idBool          /* aIsRestart */  )
{
    UInt          sState = 0;
    svmTBSNode *  sSpaceNode;

    IDE_ERROR( aValueSize == 0 );

    sSpaceNode = (svmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sSpaceNode != NULL )
    {
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sSpaceNode );
        sState = 1;

        if ( SMI_TBS_IS_DROPPING( sSpaceNode->mHeader.mState ) )
        {
            // 알고리즘 RESTART (가), RUNTIME (가) 에 해당하는 경우이다. 
            // DROPPING을 끄고, ONLINE 상태로 변경한다. 
            sSpaceNode->mHeader.mState &= ~SMI_TBS_DROPPING;
        }
        
        IDE_ERROR( SMI_TBS_IS_ONLINE(sSpaceNode->mHeader.mState) );
        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_CREATING) 
                   != SMI_TBS_CREATING );

        sState = 0;
        sctTableSpaceMgr::unlockSpaceNode( sSpaceNode );
    }
    else
    {
        // TBS List에서 검색이 되지 않으면 이미 Drop된 Tablespace이다.
        // 아무것도 하지 않는다.
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
/*
    ALTER TABLESPACE TBS1 AUTOEXTEND .... 에 대한 Log Image를 분석한다.

    [IN]  aValueSize     - Log Image 의 크기 
    [IN]  aValuePtr      - Log Image
    [OUT] aAutoExtMode   - Auto extent mode
    [OUT] aNextPageCount - Next page count
    [OUT] aMaxPageCount  - Max page count
 */
IDE_RC svmUpdate::getAlterAutoExtendImage( UInt       aValueSize,
                                           SChar    * aValuePtr,
                                           idBool   * aAutoExtMode,
                                           scPageID * aNextPageCount,
                                           scPageID * aMaxPageCount )
{
    IDE_ERROR( aValuePtr != NULL );
    IDE_ERROR( aValueSize == (UInt)( ID_SIZEOF(*aAutoExtMode) +
                                      ID_SIZEOF(*aNextPageCount) +
                                      ID_SIZEOF(*aMaxPageCount) ) );
    IDE_ERROR( aAutoExtMode   != NULL );
    IDE_ERROR( aNextPageCount != NULL );
    IDE_ERROR( aMaxPageCount  != NULL );
    
    idlOS::memcpy(aAutoExtMode, aValuePtr, ID_SIZEOF(*aAutoExtMode));
    aValuePtr += ID_SIZEOF(*aAutoExtMode);

    idlOS::memcpy(aNextPageCount, aValuePtr, ID_SIZEOF(*aNextPageCount));
    aValuePtr += ID_SIZEOF(*aNextPageCount);

    idlOS::memcpy(aMaxPageCount, aValuePtr, ID_SIZEOF(*aMaxPageCount));
    aValuePtr += ID_SIZEOF(*aMaxPageCount);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    ALTER TABLESPACE TBS1 AUTOEXTEND .... 에 대한 REDO 수행

    [ 로그 구조 ]
    After Image   --------------------------------------------
      idBool              aAIsAutoExtend
      scPageID            aANextPageCount
      scPageID            aAMaxPageCount 
    
    [ ALTER_TBS_AUTO_EXTEND 의 REDO 처리 ]
      (r-010) TBSNode.AutoExtend := AfterImage.AutoExtend
      (r-020) TBSNode.NextSize   := AfterImage.NextSize
      (r-030) TBSNode.MaxSize    := AfterImage.MaxSize
*/
IDE_RC svmUpdate::redo_SCT_UPDATE_VRDB_ALTER_AUTOEXTEND(
                    idvSQL        * /*aStatistics*/,
                    void          * /*aTrans*/,
                    smLSN           /* sCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /*aFileID*/,
                    UInt            aValueSize,
                    SChar         * aValuePtr,
                    idBool          /* aIsRestart */ )
{
    svmTBSNode       * sTBSNode;
    idBool             sAutoExtMode;
    scPageID           sNextPageCount;
    scPageID           sMaxPageCount;

    // aValueSize, aValuePtr 에 대한 인자 DASSERTION은
    // getAlterAutoExtendImage 에서 실시.
    IDE_TEST( getAlterAutoExtendImage( aValueSize,
                                       aValuePtr,
                                       & sAutoExtMode,
                                       & sNextPageCount,
                                       & sMaxPageCount ) != IDE_SUCCESS );

    sTBSNode = (svmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sTBSNode != NULL )
    {
        sTBSNode->mTBSAttr.mVolAttr.mIsAutoExtend  = sAutoExtMode;
        sTBSNode->mTBSAttr.mVolAttr.mNextPageCount = sNextPageCount;
        sTBSNode->mTBSAttr.mVolAttr.mMaxPageCount  = sMaxPageCount;
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
    ALTER TABLESPACE TBS1 AUTOEXTEND .... 에 대한 undo 수행

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      idBool              aBIsAutoExtend
      scPageID            aBNextPageCount
      scPageID            aBMaxPageCount
      
    [ ALTER_TBS_AUTO_EXTEND 의 UNDO 처리 ]
      (u-010) 로깅실시 -> CLR ( ALTER_TBS_AUTO_EXTEND )
      (u-020) TBSNode.AutoExtend := BeforeImage.AutoExtend
      (u-030) TBSNode.NextSize   := BeforeImage.NextSize
      (u-040) TBSNode.MaxSize    := BeforeImage.MaxSize
*/
IDE_RC svmUpdate::undo_SCT_UPDATE_VRDB_ALTER_AUTOEXTEND(
                    idvSQL        * /*aStatistics*/,
                    void          * aTrans,
                    smLSN           /* sCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /*aFileID*/,
                    UInt            aValueSize,
                    SChar         * aValuePtr,
                    idBool          aIsRestart )
{
    svmTBSNode       * sTBSNode;
    idBool             sAutoExtMode;
    scPageID           sNextPageCount;
    scPageID           sMaxPageCount;
    

    // BUGBUG-1548 왜 Restart도중에는 aTrans == NULL일 수 있는지?
    IDE_ERROR( (aTrans != NULL) || (aIsRestart == ID_TRUE) );


    // aValueSize, aValuePtr 에 대한 인자 DASSERTION은
    // getAlterAutoExtendImage 에서 실시.
    IDE_TEST( getAlterAutoExtendImage( aValueSize,
                                       aValuePtr,
                                       & sAutoExtMode,
                                       & sNextPageCount,
                                       & sMaxPageCount ) != IDE_SUCCESS );

    sTBSNode = (svmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        sTBSNode->mTBSAttr.mVolAttr.mIsAutoExtend  = sAutoExtMode;
        sTBSNode->mTBSAttr.mVolAttr.mNextPageCount = sNextPageCount;
        sTBSNode->mTBSAttr.mVolAttr.mMaxPageCount  = sMaxPageCount;
        
        if ( aIsRestart == ID_FALSE )
        {
            // Log Anchor에 flush.
            IDE_TEST( smLayerCallback::updateTBSNodeAndFlush( (sctTableSpaceNode*)sTBSNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // RESTART시에는 Loganchor를 flush하지 않는다.
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
