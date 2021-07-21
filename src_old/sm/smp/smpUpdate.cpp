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
 * $Id: smpUpdate.cpp 86110 2019-09-02 04:52:04Z et16 $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smpDef.h>
#include <smpFixedPageList.h>
#include <smpVarPageList.h>
#include <smpFreePageList.h>
#include <smpUpdate.h>
#include <sctTableSpaceMgr.h>
#include <smmTBSMultiPhase.h>
#include <smpTBSAlterOnOff.h>
#include <smpReq.h>

/* Update type:  SMR_SMM_PERS_UPDATE_LINK  */
/* before image: Prev PageID | Next PageID */
/* after  image: Prev PageID | Next PageID */
IDE_RC smpUpdate::redo_undo_SMM_PERS_UPDATE_LINK(smTID       /*a_tid*/,
                                                 scSpaceID     aSpaceID,
                                                 scPageID      a_pid,
                                                 scOffset    /*a_offset*/,
                                                 vULong      /*a_data*/,
                                                 SChar        *a_pImage,
                                                 SInt        /*a_nSize*/,
                                                 UInt        /*aFlag*/ )
{

    smpPersPage *s_pPersPage;

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            a_pid,
                                            (void**)&s_pPersPage )
                == IDE_SUCCESS );

    s_pPersPage->mHeader.mSelfPageID = a_pid;

    idlOS::memcpy(&(s_pPersPage->mHeader.mPrevPageID),
                  a_pImage,
                  ID_SIZEOF(scPageID));

    a_pImage += ID_SIZEOF(scPageID);

    idlOS::memcpy(&(s_pPersPage->mHeader.mNextPageID),
                  a_pImage,
                  ID_SIZEOF(scPageID));


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, a_pid) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/* Update type: SMR_SMC_PERS_INIT_FIXED_PAGE           */
/* Redo Only                                           */
/* After Image: SlotSize | SlotCount | AllocPageListID */
IDE_RC smpUpdate::redo_SMC_PERS_INIT_FIXED_PAGE(smTID        /*a_tid*/,
                                                scSpaceID      aSpaceID,
                                                scPageID       a_pid,
                                                scOffset     /*a_offset*/,
                                                vULong       /*a_data*/,
                                                SChar         *a_pImage,
                                                SInt         /*a_nSize*/,
                                                UInt        /*aFlag*/ )
{
    smpPersPage    *s_pPersPage;
    vULong          s_slotSize;
    vULong          s_cSlot;
    UInt            sPageListID;
    smOID           sTableOID;

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            a_pid,
                                            (void**)&s_pPersPage )
                == IDE_SUCCESS );

    /* BUG-15710: Redo시에 특정영역의 데이타가 올바른지 ASSERT를 걸고 있습니다.
       Redo시에 해당데이타에 redo하기전까지는  Valid하다는 것을 보장하지 못합니다.
       IDE_ASSERT( s_pPersPage->mHeader.mSelfPageID == a_pid );
    */

    idlOS::memcpy(&s_slotSize, a_pImage, ID_SIZEOF(vULong));
    a_pImage  += ID_SIZEOF(vULong);
    idlOS::memcpy(&s_cSlot, a_pImage, ID_SIZEOF(vULong));
    a_pImage  += ID_SIZEOF(vULong);
    idlOS::memcpy(&sPageListID, a_pImage, ID_SIZEOF(UInt));
    a_pImage  += ID_SIZEOF(UInt);
    idlOS::memcpy(&sTableOID, a_pImage, ID_SIZEOF(smOID));

    smpFixedPageList::initializePage(s_slotSize,
                                     s_cSlot,
                                     sPageListID,
                                     sTableOID,
                                     s_pPersPage );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, a_pid) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Update type: SMR_SMC_PERS_INIT_VAR_PAGE                      */
/* Redo Only                                                    */
/* After Image: VarIdx | SlotSize | SlotCount | AllocPageListID */
IDE_RC smpUpdate::redo_SMC_PERS_INIT_VAR_PAGE(smTID       /*a_tid*/,
                                              scSpaceID     aSpaceID,
                                              scPageID      a_pid,
                                              scOffset    /*a_offset*/,
                                              vULong      /*a_data*/,
                                              SChar       * a_pAfterImage,
                                              SInt        /*a_nSize*/,
                                              UInt        /*aFlag*/ )
{

    smpPersPage    *s_pPersPage;
    vULong          s_slotSize;
    vULong          s_cSlot;
    vULong          s_idx;
    UInt            sPageListID;
    smOID           sTableOID;

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            a_pid,
                                            (void**)&s_pPersPage )
                == IDE_SUCCESS );

    /* BUG-15710: Redo시에 특정영역의 데이타가 올바른지 ASSERT를 걸고 있습니다.
       Redo시에 해당데이타에 redo하기전까지는  Valid하다는 것을 보장하지 못합니다.
       IDE_ASSERT( s_pPersPage->mHeader.mSelfPageID == a_pid );
    */

    idlOS::memcpy(&s_idx, a_pAfterImage, ID_SIZEOF(vULong));
    a_pAfterImage  += ID_SIZEOF(vULong);
    idlOS::memcpy(&s_slotSize, a_pAfterImage, ID_SIZEOF(vULong));
    a_pAfterImage  += ID_SIZEOF(vULong);
    idlOS::memcpy(&s_cSlot, a_pAfterImage, ID_SIZEOF(vULong));
    a_pAfterImage  += ID_SIZEOF(vULong);
    idlOS::memcpy(&sPageListID, a_pAfterImage, ID_SIZEOF(UInt));
    a_pAfterImage  += ID_SIZEOF(UInt);
    idlOS::memcpy(&sTableOID, a_pAfterImage, ID_SIZEOF(smOID));

    smpVarPageList::initializePage(s_idx,
                                   sPageListID,
                                   s_slotSize,
                                   s_cSlot,
                                   sTableOID,
                                   s_pPersPage );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, a_pid) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smpUpdate::redo_SMP_NTA_ALLOC_FIXED_ROW( scSpaceID    aSpaceID,
                                                scPageID     aPageID,
                                                scOffset     aOffset,
                                                idBool       aIsSetDeleteBit )
{
    smpSlotHeader     *sSlotHeader;
    smSCN              sSCN;
    smOID              sOID;

    sOID = SM_MAKE_OID(aPageID, aOffset);
    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       sOID,
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );

    SM_INIT_SCN(&sSCN);

    if(aIsSetDeleteBit == ID_TRUE)
    {
        SM_SET_SCN_DELETE_BIT( &sSCN );
    }

    SMP_SLOT_SET_OFFSET( sSlotHeader, aOffset );
    sSlotHeader->mCreateSCN = sSCN;
    SM_SET_SCN_FREE_ROW( &(sSlotHeader->mLimitSCN) );
    SMP_SLOT_INIT_POSITION( sSlotHeader );
    SMP_SLOT_SET_USED( sSlotHeader );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID, aPageID) != IDE_SUCCESS );

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
IDE_RC smpUpdate::getAlterTBSOnOffImage( UInt       aValueSize,
                                         SChar    * aValuePtr,
                                         UInt     * aState )
{
    IDE_DASSERT( aValuePtr != NULL );

    IDE_ASSERT( aValueSize == (UInt)( ID_SIZEOF(*aState)));

    IDE_DASSERT( aState   != NULL );

    idlOS::memcpy(aState, aValuePtr, ID_SIZEOF(*aState));
    aValuePtr += ID_SIZEOF(*aState);

    return IDE_SUCCESS;
}



/*
    ALTER TABLESPACE TBS1 OFFLINE .... 에 대한 REDO 수행

    [ 로그 구조 ]
    After Image  --------------------------------------------
      UInt                aAState

    [ ALTER TABLESPACE OFFLINE 의 REDO 처리 ]
      (u-010) (020)에 대한 REDO로 TBSNode.Status := After Image(OFFLINE)
      (note-1) TBSNode를 loganchor에 flush하지 않음
               -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문
      (note-2) Commit Pending을 수행하지 않음
               -> Restart Recovery완료후 OFFLINE TBS에 대한 Resource해제를 한다
*/
IDE_RC smpUpdate::redo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE(
                    idvSQL        * /* aStatistics */,
                    void          * aTrans,
                    smLSN           /* aCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /*aFileID*/,
                    UInt            aValueSize,
                    SChar         * aValuePtr,
                    idBool          /*aIsRestart*/ )
{
    UInt           sTBSState;
    smmTBSNode   * sTBSNode;
    sctPendingOp * sPendingOp;

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        IDE_TEST( getAlterTBSOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sTBSState ) != IDE_SUCCESS );

        // Tablespace의 상태를 즉시 변경할 경우,
        // Tablespace를 OFFLINE으로 만드는 도중에
        // 발생한 Log들을 Redo하지 못하게 된다.
        // ( Offline Tablespace에 대해서는 Redo를 하지 않음 )
        //
        // => Transaction Commit시에 수행할 Pending Operation등록
        IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                      aTrans,
                      aSpaceID,
                      ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                      SCT_POP_ALTER_TBS_OFFLINE,
                      & sPendingOp )
                  != IDE_SUCCESS );

        // Commit시 sctTableSpaceMgr::executePendingOperation에서
        // 수행할 Pending함수 설정
        sPendingOp->mPendingOpFunc = smpTBSAlterOnOff::alterOfflineCommitPending;
        sPendingOp->mNewTBSState   = sTBSState;

        // Pending함수에서 상태체크시 ASSERT죽지않게 하기 위함
        sTBSNode->mHeader.mState |= SMI_TBS_SWITCHING_TO_OFFLINE ;

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
               -> ALTER TBS OFFLINE의 Commit Pending을 통해
                  COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문
*/
IDE_RC smpUpdate::undo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE(
                    idvSQL        * /*aStatistics*/,
                    void          * /*aTrans*/,
                    smLSN           /* aCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /*aFileID*/,
                    UInt            aValueSize,
                    SChar         * aValuePtr,
                    idBool          /*aIsRestart*/ )
{
    UInt          sTBSState;
    smmTBSNode  * sTBSNode;

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

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
    ALTER TABLESPACE TBS1 ONLINE .... 에 대한 REDO 수행

    [ 로그 구조 ]
    After Image  --------------------------------------------
      UInt                aAState

    [ ALTER TABLESPACE ONLINE 의 REDO 처리 ]
      (r-010) TBSNode.Status := After Image(ONLINE)
      (note-1) TBSNode를 loganchor에 flush하지 않음
               -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문

*/
IDE_RC smpUpdate::redo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE(
                    idvSQL        * /*aStatistics*/,
                    void          * /*aTrans*/,
                    smLSN           /* aCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /*aFileID*/,
                    UInt            aValueSize,
                    SChar         * aValuePtr,
                    idBool          /*aIsRestart*/ )
{
    UInt          sTBSState;
    smmTBSNode  * sTBSNode;

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        if ( SMI_TBS_IS_ONLINE(sTBSNode->mHeader.mState) )
        {
            // 이미 Online상태라면, Page단계까지 초기화되고
            // Restore가 완료된 상태이다.
            //
            // 아무런 처리도 필요하지 않다.
        }
        else // Online상태가 아니라면
        {
            if ( sTBSNode->mRestoreType ==
                 SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET )
            {
                // PAGE 단계까지 초기화를 시키고
                // Restore를 실시한다.
                IDE_TEST( smmTBSMultiPhase::initPagePhase( sTBSNode )
                          != IDE_SUCCESS );
                IDE_TEST( smmManager::prepareAndRestore( sTBSNode )
                          != IDE_SUCCESS );
            }
            else
            {
                // 1. ONLINE인 채로 서버 기동
                //      => Page단계까지 초기화, Restore실시
                // 2. REDO도중 ALTER OFFLINE로그를 만나서 OFFLINE으로 전이
                //      => 상태만 OFFLINE으로 바꾸고,
                //         Tablespace는 그대로 둠
                // 3. REDO도중 ALTER ONLINE로그 만남
                //      => 1에서 Restore까지 모두 마친 상태
                //         여기에서는 아무런 처리도 수행하지 않음
            }

        }

        IDE_TEST( getAlterTBSOnOffImage( aValueSize,
                                         aValuePtr,
                                         & sTBSState ) != IDE_SUCCESS );

        // Online상태로 전이
        sTBSNode->mHeader.mState = sTBSState;

        IDE_ERROR( SMI_TBS_IS_ONLINE(sTBSState) );
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
               -> ALTER TBS ONLINE의 Commit Pending을 통해
                  COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문


*/
IDE_RC smpUpdate::undo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE(
                    idvSQL        * aStatistics,
                    void          * /*aTrans*/,
                    smLSN           /* aCurLSN */,
                    scSpaceID       aSpaceID,
                    UInt            /*aFileID*/,
                    UInt            aValueSize,
                    SChar         * aValuePtr,
                    idBool          aIsRestart )
{
    UInt          sTBSState;
    smmTBSNode  * sTBSNode;

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sTBSNode != NULL )
    {
        if ( aIsRestart == ID_TRUE )
        {
            // Restart Recovery중에는
            // Tablespace의 상태만 변경하고
            // Resource는 그대로 둔다.
            //
            // Restart Recovery완료후에 일괄처리한다.
        }
        else
        {
            // Alter Tablespace Online 수행도중 에러발생 여부 체크
            if ( ( sTBSNode->mHeader.mState & SMI_TBS_SWITCHING_TO_ONLINE )
                 == SMI_TBS_SWITCHING_TO_ONLINE )
            {
                // Alter Tablespace Online 수행은 완료하였으나,
                // 이후에 Transaction이 ABORT된 경우

                //////////////////////////////////////////////////////////
                //  Free All Index Memory of TBS
                //  Destroy/Free Runtime Info At Table Header
                IDE_TEST( smLayerCallback::alterTBSOffline4Tables(
                              aStatistics,
                              aSpaceID )
                          != IDE_SUCCESS );


                //////////////////////////////////////////////////////////
                // Page단계를 해제한다.
                IDE_TEST( smmTBSMultiPhase::finiPagePhase( sTBSNode )
                          != IDE_SUCCESS );

            }
            else
            {
                // Alter Tablespace Online 수행중 에러 발생하여
                // ABORT된 경우

                // Do Nothing
                // => Alter Tablespace Online 수행시 에러처리 루틴에서
                //    위의 작업을 이미 다 처리한 상태이다.
            }
        }


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

/* Update type: SMR_SMC_PERS_SET_INCONSISTENCY         */
IDE_RC smpUpdate::redo_undo_SMC_PERS_SET_INCONSISTENCY(
    smTID        /*aTid*/,
    scSpaceID      aSpaceID,
    scPageID       aPid,
    scOffset     /*aOffset*/,
    vULong       /*aData*/,
    SChar         *aImage,
    SInt         /*aSize*/,
    UInt         /*aFlag*/ )
{
    smpPersPage    * sPersPage;

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            aPid,
                                            (void**)&sPersPage )
                == IDE_SUCCESS );
    idlOS::memcpy( &sPersPage->mHeader.mType,
                   aImage,
                   ID_SIZEOF( smpPageType ) );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPid) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


