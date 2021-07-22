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
 * $Id: smmUpdate.cpp 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smmReq.h>
#include <smmUpdate.h>
#include <smmExpandChunk.h>
#include <sctTableSpaceMgr.h>
#include <smmTBSMultiPhase.h>

/* Update type:  SMR_PHYSICAL                      */
IDE_RC smmUpdate::redo_undo_PHYSICAL(smTID       /*a_tid*/,
                                     scSpaceID   a_spaceid,
                                     scPageID    a_pid,
                                     scOffset    a_offset,
                                     vULong      /*a_data*/,
                                     SChar     * a_pImage,
                                     SInt        a_nSize,
                                     UInt        /*aFlag*/ )
{

    SChar *s_pDataPtr;
    
    if ( a_nSize != 0 )
    {
        IDE_ASSERT( smmManager::getOIDPtr( a_spaceid,
                                           SM_MAKE_OID( a_pid, a_offset ),
                                           (void**)&s_pDataPtr )
                    == IDE_SUCCESS );

        idlOS::memcpy( s_pDataPtr, a_pImage, a_nSize );

        IDE_TEST( smmDirtyPageMgr::insDirtyPage(a_spaceid, a_pid)
                  != IDE_SUCCESS );

    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

/* Update type:  SMR_SMM_MEMBASE_INFO                      */
IDE_RC smmUpdate::redo_SMM_MEMBASE_INFO(smTID       /*a_tid*/,
                                        scSpaceID   a_spaceid,
                                        scPageID    a_pid,
                                        scOffset    a_offset,
                                        vULong      /*a_data*/,
                                        SChar     * a_pImage,
                                        SInt        a_nSize,
                                        UInt        /*aFlag*/ )
{

    smmMemBase *sMemBase;

    IDE_ERROR_MSG( (a_pid == SMM_MEMBASE_PAGEID) && (a_nSize == ID_SIZEOF(smmMemBase)),
                   "a_pid : %"ID_UINT32_FMT, a_pid  );
    
    IDE_ASSERT( smmManager::getOIDPtr( a_spaceid,
                                       SM_MAKE_OID( a_pid, a_offset ),
                                       (void**)&sMemBase )
                == IDE_SUCCESS );
    idlOS::memcpy( sMemBase, a_pImage, ID_SIZEOF(smmMemBase) );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(a_spaceid, a_pid)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

/* Update type:  SMR_SMM_MEMBASE_SET_SYSTEM_SCN                      */
IDE_RC smmUpdate::redo_SMM_MEMBASE_SET_SYSTEM_SCN(smTID       /*a_tid*/,
                                                  scSpaceID   a_spaceid,
                                                  scPageID    a_pid,
                                                  scOffset    a_offset,
                                                  vULong      /*a_data*/,
                                                  SChar     * a_pImage,
                                                  SInt        a_nSize,
                                                  UInt        /*aFlag*/ )
{

    smmMemBase *sMemBase;
    smSCN       s_systemSCN;

    IDE_ERROR_MSG( a_nSize == ID_SIZEOF(smSCN),
                   "a_nSize : %"ID_UINT32_FMT, a_nSize  );
    
    IDE_ASSERT( smmManager::getOIDPtr( a_spaceid,
                                       SM_MAKE_OID( a_pid, a_offset ),
                                       (void**)&sMemBase )
                == IDE_SUCCESS );

    idlOS::memcpy( &s_systemSCN, a_pImage, ID_SIZEOF(smSCN) );

    sMemBase->mSystemSCN = s_systemSCN;
    
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(a_spaceid, a_pid) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

/* Update type:  SMR_SMM_MEMBASE_ALLOC_PERS
 *
 *   Membase의 다중화된 Free Page List에 변경한 내용에 대해 REDO/UNDO 실시
 *
 *   before image :
 *                  aFPLNo
 *                  aBeforeMembase-> mFreePageLists[ aFPLNo ].mFirstFreePageID
 *                  aBeforeMembase-> mFreePageLists[ aFPLNo ].mFreePageCount
 *   after  image :
 *                  aFPLNo
 *                  aAfterPageID
 *                  aAfterPageCount
 */
IDE_RC smmUpdate::redo_undo_SMM_MEMBASE_ALLOC_PERS_LIST(smTID          /*a_tid*/,
                                                        scSpaceID      a_spaceid,
                                                        scPageID       a_pid,
                                                        scOffset       a_offset,
                                                        vULong         /*a_data*/,
                                                        SChar        * a_pImage,
                                                        SInt           /*a_nSize*/,
                                                        UInt           /*aFlag*/ )
{

    smmMemBase * sMembase;
    UInt         sFPLNo ;
    scPageID     sFirstFreePageID ;
    vULong       sFreePageCount ;
    
    IDE_ASSERT( smmManager::getOIDPtr( a_spaceid,
                                       SM_MAKE_OID( a_pid, a_offset ),
                                       (void**)&sMembase )
                == IDE_SUCCESS );

    idlOS::memcpy( & sFPLNo, 
                   a_pImage, 
                   ID_SIZEOF(UInt) );
    a_pImage += ID_SIZEOF(UInt);

    idlOS::memcpy( & sFirstFreePageID,
                   a_pImage,
                   ID_SIZEOF(scPageID) );
    a_pImage += ID_SIZEOF(scPageID);

    idlOS::memcpy( & sFreePageCount,
                   a_pImage,
                   ID_SIZEOF(vULong) );
    a_pImage += ID_SIZEOF(vULong);


    // Redo하는 Page외의 다른 Page의 Validity를 보장할 수 없기 때문에,
    // Redo중에는 Free Page List의 Page수를 체크하지 않는다.
    IDE_DASSERT( smmFPLManager::isValidFPL( a_spaceid,
                                            & sMembase->mFreePageLists[ sFPLNo ] )
                 == ID_TRUE );
    
    sMembase->mFreePageLists[ sFPLNo ].mFirstFreePageID = sFirstFreePageID ;
    sMembase->mFreePageLists[ sFPLNo ].mFreePageCount = sFreePageCount;

    // Redo하는 Page외의 다른 Page의 Validity를 보장할 수 없기 때문에,
    // Redo중에는 Free Page List의 Page수를 체크하지 않는다.
    IDE_DASSERT( smmFPLManager::isValidFPL( a_spaceid,
                                            & sMembase->mFreePageLists[ sFPLNo ] )
                 == ID_TRUE );
    

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(a_spaceid, a_pid) != IDE_SUCCESS );


    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}




/* Update type:  SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK

   Expand Chunk할당에 대해 logical redo 실시 
   
   smmManager::allocNewExpandChunk의 Logical Redo를 위해
   Membase 일부 멤버의 저장된 Before 이미지를 이용한다.           
   
   after  image: aBeforeMembase->m_alloc_pers_page_count
                 aBeforeMembase->mCurrentExpandChunkCnt
                 aBeforeMembase->mExpandChunkPageCnt
                 aBeforeMembase->m_nDBFileCount[0]
                 aBeforeMembase->m_nDBFileCount[1]
                 aBeforeMembase->mFreePageListCount
                 [
                      aBeforeMembase-> mFreePageLists[i].mFirstFreePageID
                      aBeforeMembase-> mFreePageLists[i].mFreePageCount
                 ] ( aBeforeMembase->mFreePageListCount 만큼 )
                 aExpandPageListID
                 aAfterChunkFirstPID                          
                 aAfterChunkLastPID
                 
   aExpandPageListID : 확장된 Chunk의 Page가 매달릴 Page List의 ID
                       UINT_MAX일 경우 모든 Page List에 골고루 분배된다
*/                                                                   
IDE_RC smmUpdate::redo_SMM_MEMBASE_ALLOC_EXPAND_CHUNK( smTID       /*a_tid*/,
                                                       scSpaceID     a_spaceid,
                                                       scPageID      a_pid,
                                                       scOffset      a_offset,
                                                       vULong      /*a_data*/,
                                                       SChar        *a_pImage,
                                                       SInt        /*a_nSize*/,
                                                       UInt        /*aFlag*/ )
{

    smmMemBase    * sMembase;
    smmMemBase    * sOrgMembase;
    UInt            sOrgDBFileCount;
    
    // Log에서 읽어온 멤버들로만 Membase를 구성하여
    // Logical Redo를 수행한다.
    // 그리고 Logical Redo를 수행하고 난 결과로 생긴 Membase안의 필드값으로 
    // 실제 Membase의 필드값을 세팅한다.
    //
    // 이렇게 하는 이유는, Restart Recovery도중에는 Membase가 깨져있을 수도
    // 있기 때문에, Membase안의 필드중 Log에서 읽어온 필드 외의 다른
    // 필드들을 모두 0으로 세팅해놓고 Logical Redo를 수행하여,
    // 예상치 못한 Membase 필드 참조를 미연에 Detect하기 위함이다.
    smmMemBase      sLogicalRedoMembase;
    
    UInt            sFreePageListCount;
    scPageID        sNewChunkFirstPID;
    scPageID        sNewChunkLastPID;
    scPageID        sFirstFreePageID;
    vULong          sFreePageCount;
    UInt            sExpandPageListID;    
    UInt            sDbFileNo;
    UInt            i;
    smmTBSNode    * sTBSNode;
    SInt            sWhichDB;
    idBool          sIsCreatedDBFile;

    IDE_ERROR_MSG( a_pid == SMM_MEMBASE_PAGEID,
                   "a_pid : %"ID_UINT32_FMT, a_pid  );
    
    
    idlOS::memset( & sLogicalRedoMembase,
                   0,
                   ID_SIZEOF( smmMemBase ) );
    
    // Logical Redo가 여러번 되어도 문제없도록 하기 위해
    // Logical Redo전에 Membase의 정보를 로그 기록 당시로 돌려놓는다.
    
    // Log분석 시작 ++++++++++++++++++++++++++++++++++++++++++++++
    idlOS::memcpy( & sLogicalRedoMembase.mAllocPersPageCount,
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mAllocPersPageCount ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mAllocPersPageCount );
    
    idlOS::memcpy( & sLogicalRedoMembase.mCurrentExpandChunkCnt,
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mCurrentExpandChunkCnt ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mCurrentExpandChunkCnt );

    idlOS::memcpy( & sLogicalRedoMembase.mExpandChunkPageCnt,
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mExpandChunkPageCnt ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mExpandChunkPageCnt );
    
    idlOS::memcpy( & sLogicalRedoMembase.mDBFileCount[0],
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[0] ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[0] );

    idlOS::memcpy( & sLogicalRedoMembase.mDBFileCount[1],
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[1] ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[1] );

    
    idlOS::memcpy( & sFreePageListCount,
                   a_pImage,
                   ID_SIZEOF(UInt) );
    a_pImage += ID_SIZEOF(UInt);
    sLogicalRedoMembase.mFreePageListCount = sFreePageListCount;

    for ( i = 0 ;
          i < (UInt) sFreePageListCount ;
          i ++ )
    {
        idlOS::memcpy( & sFirstFreePageID,
                       a_pImage,
                       ID_SIZEOF(scPageID) );
        a_pImage += ID_SIZEOF(scPageID);
        
        idlOS::memcpy( & sFreePageCount,
                       a_pImage,
                       ID_SIZEOF(vULong) );
        a_pImage += ID_SIZEOF(vULong);

        
        sLogicalRedoMembase.mFreePageLists[i].mFirstFreePageID = sFirstFreePageID;
        sLogicalRedoMembase.mFreePageLists[i].mFreePageCount = sFreePageCount;

        // Redo하는 Page외의 다른 Page의 Validity를 보장할 수 없기 때문에,
        // Redo중에는 Free Page List의 Page수를 체크하지 않는다.
        IDE_DASSERT( smmFPLManager::isValidFPL( a_spaceid,
                                                & sLogicalRedoMembase.mFreePageLists[i] )
                     == ID_TRUE );
    } // end of for

    idlOS::memcpy( & sExpandPageListID, 
                   a_pImage, 
                   ID_SIZEOF(UInt) );
    a_pImage += ID_SIZEOF(UInt);
    
    idlOS::memcpy( & sNewChunkFirstPID, 
                   a_pImage, 
                   ID_SIZEOF(scPageID) );
    a_pImage += ID_SIZEOF(scPageID);

    idlOS::memcpy( & sNewChunkLastPID, 
                   a_pImage, 
                   ID_SIZEOF(scPageID) );
    a_pImage += ID_SIZEOF(scPageID);
    // Log분석 끝 ------------------------------------------------

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( a_spaceid,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );

    sLogicalRedoMembase.mDBFilePageCount = sTBSNode->mMemBase->mDBFilePageCount;
    
    // Logical Redo 수행을 위해 Membase를 바꿔친다.
    sOrgMembase = sTBSNode->mMemBase;
    sTBSNode->mMemBase = & sLogicalRedoMembase;

    // Logical Redo 이전의 DB File수 
    sOrgDBFileCount = sLogicalRedoMembase.mDBFileCount[0];

    // Logical Redo로 Expand Chunk를 할당한다.
    IDE_TEST( smmManager::allocNewExpandChunk( sTBSNode,
                                               // aTrans를 NULL로 넘겨서 Redo중임을 알리고
                                               // Expand Chunk할당중에 Log기록및 Page범위 체크를 하지 않는다.
                                               NULL,
                                               sNewChunkFirstPID,
                                               sNewChunkLastPID )
              != IDE_SUCCESS );
 
    // Chunk할당 도중 새로 생겨난 DB파일에 대한 처리
    //
    // sOrgDBFileCount 
    //    logical redo이전의 DB File수  => 새로 생성된 첫번째 DB File 번호
    // sLogicalRedoMembase.m_nDBFileCount[0]-1
    //    새 Membase의 DB File수 -1 => 새로 생성된 마지막 DB File 번호
    //
    // ( m_nDBFileCount[0], [1] 모두 항상 같은 값으로 설정되므로,
    //   아무 값이나 사용해도 된다. )
    for ( sDbFileNo  = sOrgDBFileCount; 
          sDbFileNo <= sLogicalRedoMembase.mDBFileCount[0]-1 ;
          sDbFileNo ++ )
    {
        // DB File이 존재하면 open하고 없으면 생성한다.
        // 만약 파일이 없어서 생성을 해야하는 경우에는 
        // CreateLSN은 이미 위에서 allocNewExpandChunk에서 
        // Stable/Unstable Chkpt image의 Hdr에 설정을 해놓았기 때문에
        // 여기서 그냥 파일만 생성하면 된다. 
        IDE_TEST( smmManager::openOrCreateDBFileinRecovery( sTBSNode,
                                                            sDbFileNo,
                                                            &sIsCreatedDBFile )
                  != IDE_SUCCESS );

        /* PROJ-2386 DR
         * DR Standby의 checkpoint파일의 ChkptImageHdr.mSpaceID가 설정되지 않아
         * TBS DROP 되지 않는 문제 수정. */
        if ( sIsCreatedDBFile == ID_TRUE )
        {
            for ( sWhichDB = 0;
                  sWhichDB < SMM_PINGPONG_COUNT;
                  sWhichDB++ )
            {
                IDE_TEST( smmManager::flushTBSMetaPage( sTBSNode, sWhichDB )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* nothing to do */
        }

        // 모든 DB File에 존재하는 토탈 Page Count 계산.
        // Performance View용이며 정확히 일치하지 않아도 된다.
        IDE_TEST( smmManager::calculatePageCountInDisk( sTBSNode )
                  != IDE_SUCCESS );
    }

    // To Fix BUG-15112
    // Restart Recovery중에 Page Memory가 NULL인 Page에 대한 Redo시
    // 해당 페이지를 그때 그때 필요할 때마다 할당한다.
    // 여기에서 페이지 메모리를 미리 할당해둘 필요가 없다
    
    // Membase를 원래대로 돌려놓는다.
    sTBSNode->mMemBase = sOrgMembase;
    
    IDE_ASSERT( smmManager::getOIDPtr( a_spaceid,
                                       SM_MAKE_OID( a_pid, a_offset ),
                                       (void**)&sMembase )
                == IDE_SUCCESS );
    IDE_ERROR_MSG( sMembase == sOrgMembase,
                   "sMembase    : %"ID_vULONG_FMT"\n"
                   "sOrgMembase : %"ID_vULONG_FMT"\n",
                   sMembase,
                   sOrgMembase );

    // Logical Redo의 결과로 만들어진 Membase의 정보를 실제 Membase에 세팅
    sMembase->mAllocPersPageCount = sLogicalRedoMembase.mAllocPersPageCount;
    sMembase->mCurrentExpandChunkCnt = sLogicalRedoMembase.mCurrentExpandChunkCnt;
    
    
    IDE_DASSERT( sMembase->mExpandChunkPageCnt == 
                 sLogicalRedoMembase.mExpandChunkPageCnt );
    
    sMembase->mFreePageListCount = sLogicalRedoMembase.mFreePageListCount;

    sMembase->mDBFileCount[0] = sLogicalRedoMembase.mDBFileCount[0];
    
    sMembase->mDBFileCount[1] = sLogicalRedoMembase.mDBFileCount[1];
    
    IDE_DASSERT( sMembase->mFreePageListCount == sFreePageListCount );

    for ( i = 0 ;
          i < sFreePageListCount ;
          i ++ )
    {
        // Redo하는 Page외의 다른 Page의 Validity를 보장할 수 없기 때문에,
        // Redo중에는 Free Page List의 Page수를 체크하지 않는다.
        IDE_DASSERT( smmFPLManager::isValidFPL( a_spaceid,
                                                & sLogicalRedoMembase.mFreePageLists[i] )
                     == ID_TRUE );
        
        sMembase->mFreePageLists[i].mFirstFreePageID = 
            sLogicalRedoMembase.mFreePageLists[i].mFirstFreePageID ;
        
        sMembase->mFreePageLists[i].mFreePageCount =
            sLogicalRedoMembase.mFreePageLists[i].mFreePageCount ;
    }
    
    // 0 번 Page를 Dirty 페이지로 추가
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(a_spaceid, a_pid) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Update type : SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK
 *
 * Free Page List Info Page 에 Free Page의 Next Free Page기록한 것에 대해
 * REDO / UNDO 실시
 *
 * before image: Next Free PageID 
 * after  image: Next Free PageID 
 */
IDE_RC smmUpdate::redo_undo_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK(
                                                          smTID          /*a_tid*/,
                                                          scSpaceID      a_spaceid,
                                                          scPageID       a_pid,
                                                          scOffset       a_offset,
                                                          vULong         /*a_data*/,
                                                          SChar        * a_pImage,
                                                          SInt           /*a_nSize*/,
                                                          UInt        /*aFlag*/ )
{

    smmFLISlot * sSlotAddr ;

    IDE_ASSERT( smmManager::getOIDPtr( a_spaceid,
                                       SM_MAKE_OID( a_pid, a_offset ),
                                       (void**)&sSlotAddr )
                == IDE_SUCCESS );

    idlOS::memcpy( & sSlotAddr->mNextFreePageID,
                   a_pImage,
                   ID_SIZEOF(scPageID) );
    

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(a_spaceid, a_pid) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

/*
 * Update type : SCT_UPDATE_MRDB_CREATE_TBS
 *
 * tablespace 생성에 대한 redo 수행
 *
 * Commit전에 생성된 Log Anchor에 Tablespace를 Flush하였기 때문에
 * Tablespace의 Attribute나 Checkpoint Path와 같은 정보들에 대해
 * 별도의 Redo를 수행할 필요가 없다.
 *
 * Disk Tablespace와 동일하게 처리한다.
 *  - sddUpdate::redo_SCT_UPDATE_DRDB_CREATE_TBS 의 주석 참고
 *    ( 여기에 주석의 알고리즘 가,나,다,...와 같은 기호가 기술되어 있음)
 */
IDE_RC smmUpdate::redo_SMM_UPDATE_MRDB_CREATE_TBS( idvSQL     * /*aStatistics*/,
                                                   void       * aTrans,
                                                   smLSN        /* aCurLSN */,
                                                   scSpaceID    aSpaceID,
                                                   UInt         /* aFileID */,
                                                   UInt         aValueSize,
                                                   SChar      * aValuePtr,
                                                   idBool       /* aIsRestart */  )
{
    smmTBSNode            * sSpaceNode;
    sctPendingOp          * sPendingOp;
    smiTableSpaceAttr       sTableSpaceAttr;
    smiChkptPathAttrList  * sChkptPathList  = NULL;
    smiChkptPathAttrList  * sChkptPath      = NULL;
    UInt                    sChkptPathCount = 0;
    UInt                    sReadOffset     = 0;
    UInt                    i               = 0;
    UInt                    sMemAllocState  = 0;
    scSpaceID               sNewSpaceID     = 0;

    IDE_DASSERT( aTrans != NULL );

    // PROJ-1923으로 After Image를 기록한다. 가변 길이의 After Image 임.
    IDE_ERROR_MSG( aValueSize >= (ID_SIZEOF(smiTableSpaceAttr) + ID_SIZEOF(UInt)),
                   "aValueSize : %"ID_UINT32_FMT,
                   aValueSize );

    // Loganchor로부터 초기화된 TBS List를 검색한다. 
    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    if ( sSpaceNode != NULL )
    {
        if ( SMI_TBS_IS_CREATING(sSpaceNode->mHeader.mState) ) 
        {
            /* 알고리즘 (다)에 해당하는 CREATINIG 상태일 경우에만 있으므로 
             * 상태를 ONLINE으로 변경할 수 있게 Commit Pending 연산을 등록한다. */
            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                                        aTrans,
                                                        aSpaceID,
                                                        ID_TRUE, /* commit시에 동작 */
                                                        SCT_POP_CREATE_TBS,
                                                        & sPendingOp )
                      != IDE_SUCCESS );

            sPendingOp->mPendingOpFunc = smmTBSCreate::createTableSpacePending;
            
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
        if ( aSpaceID == sNewSpaceID )
        {
            /* PROJ-1923 ALTIBASE HDB Disaster Recovery.
             * 위 기능을 지원하기 위해 알고리즘 (가) 에 해당하는 경우에도
             * 재수행을 수행한다. */

            // 로그를 읽어 옴
            sReadOffset = 0;

            idlOS::memcpy( (void *)&sTableSpaceAttr,
                           aValuePtr + sReadOffset,
                           ID_SIZEOF(smiTableSpaceAttr) );
            sReadOffset += ID_SIZEOF(smiTableSpaceAttr);

            idlOS::memcpy( (void *)&sChkptPathCount,
                           aValuePtr + sReadOffset,
                           ID_SIZEOF(UInt) );
            sReadOffset += ID_SIZEOF(UInt);

            // Check point path 가 기록되어 있다면.
            if ( sChkptPathCount != 0 )
            {
                // Check point path 용 메모리 할당
                /* smmUpdate_redo_SMM_UPDATE_MRDB_CREATE_TBS_calloc_ChkptPathList.tc */
                IDU_FIT_POINT("smmUpdate::redo_SMM_UPDATE_MRDB_CREATE_TBS::calloc::ChkptPathList");
                IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMM,
                                             sChkptPathCount,
                                             ID_SIZEOF(smiChkptPathAttrList),
                                             (void **)&sChkptPathList )
                          != IDE_SUCCESS );
                sMemAllocState = 1;

                // sChkptPathCount 만큼 smiChkptPathAttrList 를 memcpy
                idlOS::memcpy( (void *)sChkptPathList,
                               aValuePtr + sReadOffset,
                               (size_t)ID_SIZEOF(smiChkptPathAttrList) * sChkptPathCount );
                sReadOffset += ID_SIZEOF(smiChkptPathAttrList) * sChkptPathCount;

                // ChkptPathAttrList의 next 값 수정
                sChkptPath = sChkptPathList;
                for ( i = 0 ; i < sChkptPathCount ; i++ )
                {
                    if ( i == (sChkptPathCount -1) )
                    {
                        // 마지막의 Next 는 NULL로 설정해야 한다.
                        sChkptPath->mNext = NULL;
                    }
                    else
                    {
                        // next 주소값을 새로 설정한다.
                        sChkptPath->mNext = sChkptPath + 1;
                        sChkptPath = sChkptPath->mNext;
                    }
                } // end of for
            }
            else
            {
                // check point path가 입력되지 않은 로그라면 NULL로 처리
                sChkptPathList = NULL;
            }

            // create tbs 4 redo 실행
            IDE_TEST( smmTBSCreate::createTBS4Redo( aTrans,
                                                    &sTableSpaceAttr,
                                                    sChkptPathList )
                      != IDE_SUCCESS );

            if ( sMemAllocState != 0 )
            {
                sMemAllocState = 0;
                IDE_TEST( iduMemMgr::free( sChkptPathList ) != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            // do nothing
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sMemAllocState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sChkptPathList ) == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/*
 * Update type : SCT_UPDATE_MRDB_CREATE_TBS
 *
 * Memory tablespace 생성에 대한 undo 수행.
 *
 *  - sddUpdate::redo_SCT_UPDATE_DRDB_CREATE_TBS 의 주석 참고
 *    ( 여기에 주석의 알고리즘 가,나,다,...와 같은 기호가 기술되어 있음)
 */
IDE_RC smmUpdate::undo_SMM_UPDATE_MRDB_CREATE_TBS(
                                          idvSQL*            /*aStatistics*/,
                                          void*              /* aTrans*/,
                                          smLSN              /* aCurLSN */,
                                          scSpaceID          aSpaceID,
                                          UInt               /* aFileID */,
                                          UInt               /* aValueSize */,
                                          SChar*             /* aValuePtr */,
                                          idBool             /* aIsRestart */  )
{
    smmTBSNode         * sSpaceNode;

    // TBS List를 검색한다. 
    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );

    // RUNTIME시에는 sSpaceNode 자체에 대해서 (X) 잠금이 잡혀있기 때문에 
    // sctTableSpaceMgr::lock을 획득할 필요가 없다. 
    if ( sSpaceNode != NULL ) // 이미 Undo완료되어 Drop처리된 경우 SKIP
    {
        // Drop 된 Tablespace의 경우 sSpaceNode가 NULL로 나온다
        // 절대 Drop된 Tablespace가 올 수 없다.
        IDE_DASSERT( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED) 
                     != SMI_TBS_DROPPED );

        // Create Tablespace NTA가 찍히기 전에 Abort된 경우에만
        // 이 함수에 의해 Create Tablespace가 Undo된다.
        IDE_ERROR(  SMI_TBS_IS_CREATING(sSpaceNode->mHeader.mState) )

        // Lock정보(STATE단계) 제외한 모든 객체 파괴, 메모리 반납
        IDE_TEST( smmTBSMultiPhase::finiToStatePhase( sSpaceNode )
                  != IDE_SUCCESS );

        // Tablespace의 상태를 DROPPED로 상태변경
        //
        // To Fix BUG-17323 존재하지 않는 Checkpoint Path지정하여
        //                  Tablespace생성 실패후 Log Anchor에
        //                  Log File Group Count가 0이 되어버림
        //
        // => Tablespace Node가 아직 Log Anchor에 기록되지 않은 경우
        //    Node상의 상태만 변경해준다.
        //    Log Anchor에 한번이라도 기록이 되면
        //    sSpaceNode->mAnchorOffset 가 0보다 큰 값을 가지게 된다.
        if ( sSpaceNode->mAnchorOffset > 0 )  
        {
            // Log Anchor에 기록된 적이 있는 경우 
            // - Restart Recovery가 아닌경우에만 Log Anchor에 Flush
            IDE_TEST( smmTBSDrop::flushTBSStatusDropped( & sSpaceNode->mHeader )
                      != IDE_SUCCESS );
        }
        else
        {
            // Log Anchor에 한번도 기록되지 않는 경우
            sSpaceNode->mHeader.mState = SMI_TBS_DROPPED;
        }
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
    
    return IDE_FAILURE;    
}

/*
 * Update type : SCT_UPDATE_MRDB_DROP_TBS
 *
 * Memory tablespace Drop에 대한 redo 수행.
 *
 *   [로그 구조]
 *   After Image 기록 ----------------------------------------
 *      smiTouchMode  aTouchMode
 */
IDE_RC smmUpdate::redo_SMM_UPDATE_MRDB_DROP_TBS(
                                      idvSQL*            /*aStatistics*/,
                                      void*              aTrans,
                                      smLSN              /* aCurLSN */,
                                      scSpaceID          aSpaceID,
                                      UInt               /* aFileID */,
                                      UInt               aValueSize,
                                      SChar*             aValuePtr,
                                      idBool             /* aIsRestart */  )
{
    smmTBSNode        * sSpaceNode;
    sctPendingOp      * sPendingOp;
    smiTouchMode        sTouchMode;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aValueSize == ID_SIZEOF(smiTouchMode) );

    ACP_UNUSED( aValueSize );

    idlOS::memcpy( &sTouchMode, aValuePtr, ID_SIZEOF(smiTouchMode) );
    
    // 해당 Tablespace의 Node를 찾는다.
    // Drop된 경우에도 노드를 리턴한다.
    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aSpaceID );

    if ( sSpaceNode == NULL )
    {
        // 이미 DROP된 경우
        // Do Nothing
    }
    else
    {
        if ( SMI_TBS_IS_DROPPED(sSpaceNode->mHeader.mState) )
        {
            // 이미 DROP된 경우
            // Do Nothing
        }
        else
        {
            // Drop Tablespace Pending 수행하다가 사망한 경우도 Pending수행
            // - 이 경우 SMI_TBS_DROP_PENDING의 상태를 가진다.
            // - 이 경우에도 Drop Tablespace Pending을 다시 수행한다.
            
            /* Transaction의 Commit시에 Tablespace를 Drop 하도록
               Pending함수를 달아준다. */
            
            // Pending함수 수행 직전 상태로 Tablespace의 STATE를 복원한다
            sSpaceNode->mHeader.mState |= SMI_TBS_DROPPING;
            sSpaceNode->mHeader.mState &= ~SMI_TBS_DROP_PENDING;
            
            // 알고리즘 (가), (나)에 해당하는 경우 Commit Pending 연산 등록
            IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                                      aTrans,
                                                      sSpaceNode->mHeader.mID,
                                                      ID_TRUE, /* commit시에 동작 */
                                                      SCT_POP_DROP_TBS,
                                                      &sPendingOp )
                      != IDE_SUCCESS );
            
            // Pending 함수 설정.
            // 처리 : Tablespace와 관련된 모든 메모리와 리소스를 반납한다.
            sPendingOp->mPendingOpFunc = smmTBSDrop::dropTableSpacePending;
            sPendingOp->mTouchMode     = sTouchMode;
        }
        
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * Update type : SCT_UPDATE_MRDB_DROP_TBS
 *
 * tablespace 제거에 대한 undo 수행
 *
 * Disk Tablespace와 동일하게 처리한다.
 *  - sddUpdate::undo_SCT_UPDATE_DRDB_DROP_TBS 의 주석 참고
 * 
 * before image : tablespace attribute
 */
IDE_RC smmUpdate::undo_SMM_UPDATE_MRDB_DROP_TBS(
                                  idvSQL*            /*aStatistics*/,
                                  void*              /* aTrans */,
                                  smLSN              /* aCurLSN */,
                                  scSpaceID          aSpaceID,
                                  UInt               /* aFileID */,
                                  UInt               aValueSize,
                                  SChar*             /* aValuePtr */,
                                  idBool             /* aIsRestart */  )
{
    smmTBSNode       *  sSpaceNode;

    IDE_DASSERT( aValueSize == 0 );
    ACP_UNUSED( aValueSize );

    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    // RUNTIME시에는 sSpaceNode 자체에 대해서 (X) 잠금이 잡혀있기 때문에 
    // sctTableSpaceMgr::lock을 획득할 필요가 없다. 
    if ( sSpaceNode != NULL )
    {
        if ( SMI_TBS_IS_DROPPING(sSpaceNode->mHeader.mState) )
        {
            // 알고리즘 RESTART (가), RUNTIME (가) 에 해당하는 경우이다. 
            // DROPPING을 끄고, ONLINE 상태로 변경한다. 
            sSpaceNode->mHeader.mState &= ~SMI_TBS_DROPPING;
        }
        
        IDE_ERROR( (sSpaceNode->mHeader.mState & SMI_TBS_CREATING) 
                    != SMI_TBS_CREATING );
    }
    else
    {
        // TBS List에서 검색이 되지 않으면 이미 Drop된 Tablespace이다.
        // 아무것도 하지 않는다.
        // nothing to do...
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
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
IDE_RC smmUpdate::getAlterAutoExtendImage( UInt       aValueSize,
                                           SChar    * aValuePtr,
                                           idBool   * aAutoExtMode,
                                           scPageID * aNextPageCount,
                                           scPageID * aMaxPageCount )
{
    IDE_DASSERT( aValuePtr != NULL );
    IDE_DASSERT( aValueSize == (UInt)( ID_SIZEOF(*aAutoExtMode) +
                                       ID_SIZEOF(*aNextPageCount) +
                                       ID_SIZEOF(*aMaxPageCount) ) );
    IDE_DASSERT( aAutoExtMode   != NULL );
    IDE_DASSERT( aNextPageCount != NULL );
    IDE_DASSERT( aMaxPageCount  != NULL );

    ACP_UNUSED( aValueSize );
    
    idlOS::memcpy( aAutoExtMode, aValuePtr, ID_SIZEOF(*aAutoExtMode) );
    aValuePtr += ID_SIZEOF(*aAutoExtMode);

    idlOS::memcpy( aNextPageCount, aValuePtr, ID_SIZEOF(*aNextPageCount) );
    aValuePtr += ID_SIZEOF(*aNextPageCount);

    idlOS::memcpy( aMaxPageCount, aValuePtr, ID_SIZEOF(*aMaxPageCount) );
    aValuePtr += ID_SIZEOF(*aMaxPageCount);

    return IDE_SUCCESS;
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
IDE_RC smmUpdate::redo_SCT_UPDATE_MRDB_ALTER_AUTOEXTEND(
                                       idvSQL             * /*aStatistics*/,    
                                       void               * aTrans,
                                       smLSN                /* aCurLSN */,
                                       scSpaceID            aSpaceID,
                                       UInt                 /*aFileID*/,
                                       UInt                 aValueSize,
                                       SChar              * aValuePtr,
                                       idBool               /* aIsRestart */ )
{

    smmTBSNode       * sTBSNode;
    idBool             sAutoExtMode;
    scPageID           sNextPageCount;
    scPageID           sMaxPageCount;

    IDE_DASSERT( aTrans != NULL );

    ACP_UNUSED( aTrans );
    
    // aValueSize, aValuePtr 에 대한 인자 DASSERTION은
    // getAlterAutoExtendImage 에서 실시.
    IDE_TEST( getAlterAutoExtendImage( aValueSize,
                                       aValuePtr,
                                       & sAutoExtMode,
                                       & sNextPageCount,
                                       & sMaxPageCount ) != IDE_SUCCESS );

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sTBSNode != NULL )
    {
        sTBSNode->mTBSAttr.mMemAttr.mIsAutoExtend  = sAutoExtMode;
        sTBSNode->mTBSAttr.mMemAttr.mNextPageCount = sNextPageCount;
        sTBSNode->mTBSAttr.mMemAttr.mMaxPageCount  = sMaxPageCount;
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
IDE_RC smmUpdate::undo_SCT_UPDATE_MRDB_ALTER_AUTOEXTEND(
                                       idvSQL*            /*aStatistics*/,
                                       void*                aTrans,
                                       smLSN                /* aCurLSN */,
                                       scSpaceID            aSpaceID,
                                       UInt                 /*aFileID*/,
                                       UInt                 aValueSize,
                                       SChar*               aValuePtr,
                                       idBool               aIsRestart )
{
    smmTBSNode       * sTBSNode;
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

    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sTBSNode != NULL )
    {
        sTBSNode->mTBSAttr.mMemAttr.mIsAutoExtend  = sAutoExtMode;
        sTBSNode->mTBSAttr.mMemAttr.mNextPageCount = sNextPageCount;
        sTBSNode->mTBSAttr.mMemAttr.mMaxPageCount  = sMaxPageCount;
        
        if ( aIsRestart == ID_FALSE )
        {
            // Log Anchor에 flush.
            // TableSpace 에 대한 DDL의 undo여서 Table Space에 Lock Item이 잡혀 있다.
            IDE_TEST( smLayerCallback::updateTBSNodeAndFlush( (sctTableSpaceNode*) sTBSNode ) 
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

/*
    Create Tablespace도중 Checkpoint Image File의 생성에 대한 Redo
    type : SMM_UPDATE_MRDB_CREATE_CIMAGE_FILE
 */
IDE_RC smmUpdate::redo_SMM_UPDATE_MRDB_CREATE_CIMAGE_FILE(
                                      idvSQL*            /*aStatistics*/,
                                      void*              /* aTrans */,
                                      smLSN              /* aCurLSN */,
                                      scSpaceID          /* aSpaceID */,
                                      UInt               /* aFileID */,
                                      UInt               /* aValueSize */,
                                      SChar*             /* aValuePtr */,
                                      idBool             /* aIsRestart */  )
{
    // Do Nothing ===============================================
    // Create Tablespace도중 생성된 Checkpoint Image File은
    // Create Tablespace가 Undo될 때 지워주기만 하고,
    // Redo때는 아무것도 할 필요가 없다.
    return IDE_SUCCESS;
}


/*
    Create Tablespace도중 Checkpoint Image File의 생성에 대한 Undo
    type : SMM_UPDATE_MRDB_CREATE_CIMAGE_FILE

    해당 File을 제거한다.
    
    [로그 구조]
    Before Image 기록 ----------------------------------------
      UInt - Ping Pong Number
      UInt - DB File Number
 */
IDE_RC smmUpdate::undo_SMM_UPDATE_MRDB_CREATE_CIMAGE_FILE(
                                          idvSQL*            /*aStatistics*/,
                                          void*              aTrans,
                                          smLSN              /* aCurLSN */,
                                          scSpaceID          aSpaceID,
                                          UInt               /* aFileID */,
                                          UInt               aValueSize,
                                          SChar*             aValuePtr,
                                          idBool             /* aIsRestart */  )
{
    smmTBSNode        * sSpaceNode;
    UInt                sPingPongNo;
    UInt                sDBFileNo;
    
    smmDatabaseFile   * sDBFilePtr;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aValueSize == ID_SIZEOF(UInt) * 2 );

    ACP_UNUSED( aTrans );
    ACP_UNUSED( aValueSize );
    
    idlOS::memcpy( &sPingPongNo, aValuePtr, ID_SIZEOF(UInt) );
    aValuePtr += ID_SIZEOF(UInt);

    idlOS::memcpy( &sDBFileNo, aValuePtr, ID_SIZEOF(UInt) );
    aValuePtr += ID_SIZEOF(UInt);
    
    
    // TBS List에서 검색한다. 
    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aSpaceID );
    
    if ( sSpaceNode != NULL )
    {
        // DROPPED상태의 TBS는 findSpaceNodeWithoutException에서 건너뛴다
        IDE_DASSERT( (sSpaceNode->mHeader.mState & SMI_TBS_DROPPED)
                     != SMI_TBS_DROPPED );

        // DISCARD, OFFLINE, ONLINE Tablespace모두 MEDIA System까지
        // 초기화가 되어 있으므로, DB File객체에 접근이 가능하다.
        if ( smmManager::getDBFile( sSpaceNode,
                                    sPingPongNo,
                                    sDBFileNo,
                                    SMM_GETDBFILEOP_SEARCH_FILE,
                                    &sDBFilePtr ) == IDE_SUCCESS )
        {
            // 파일이 존재하는 경우 
            IDE_TEST( sDBFilePtr->closeAndRemoveDbFile( aSpaceID,
                                                        ID_TRUE, /* Remove File */
                                                        sSpaceNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // To Fix BUG-18272   TC/Server/sm4/PRJ-1548/dynmem/../suites/
            //                    restart/rt_ctdt_aa.sql 수행도중 죽습니다.
            //
            // SMM_UPDATE_MRDB_CREATE_CIMAGE_FILE 로깅까지만 되고
            // 파일이 아직 생성되지 않은 경우이다 => SKIP
            IDE_ERROR( ideGetErrorCode() == smERR_ABORT_NoExistFile );
            
            IDE_CLEAR();
        }
    }
    else
    {
        // 이미 Drop처리된 경우이다. 아무것도 안한다.
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 
   PRJ-1548 User Memory Tablespace 개념도입 
   
   Update type:  SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK

   Media Recovery 시에 Expand Chunk할당에 대해 logical redo 실시 
   
   1. Membase를 포함하는 0번 데이타파일이 오류가 있을 경우 
      Membase를 복구해주어야 한다.
      (Logical Membase를 사용하여야 하고 Membase도 Update해야함 )
   2. 0번외의 데이타파일이 오류가 있는 경우  
      Membase의 복구는 필요없고 Chunk 만 복구해주면 된다. 
     (Logical Membase 사용하여야 하지만 Membase는 update하면 안됨  )

   smmManager::allocNewExpandChunk의 Logical Redo를 위해
   Membase 일부 멤버의 저장된 Before 이미지를 이용한다.           
   
   after  image: aBeforeMembase->m_alloc_pers_page_count
                 aBeforeMembase->mCurrentExpandChunkCnt
                 aBeforeMembase->mExpandChunkPageCnt
                 aBeforeMembase->m_nDBFileCount[0]
                 aBeforeMembase->m_nDBFileCount[1]
                 aBeforeMembase->mFreePageListCount
                 [
                      aBeforeMembase-> mFreePageLists[i].mFirstFreePageID
                      aBeforeMembase-> mFreePageLists[i].mFreePageCount
                 ] ( aBeforeMembase->mFreePageListCount 만큼 )
                 aExpandPageListID
                 aAfterChunkFirstPID                          
                 aAfterChunkLastPID                        
*/                                                                   
IDE_RC smmUpdate::redo4MR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK( scSpaceID     a_spaceid,
                                                          scPageID      a_pid,
                                                          scOffset      a_offset,
                                                          SChar        *a_pImage )
{

    smmMemBase * sMembase;
    smmMemBase * sOrgMembase;
    UInt         sOrgDBFileCount;
    
    // Log에서 읽어온 멤버들로만 Membase를 구성하여
    // Logical Redo를 수행한다.
    // 그리고 Logical Redo를 수행하고 난 결과로 생긴 Membase안의 필드값으로 
    // 실제 Membase의 필드값을 세팅한다.
    //
    // 이렇게 하는 이유는, Restart Recovery도중에는 Membase가 깨져있을 수도
    // 있기 때문에, Membase안의 필드중 Log에서 읽어온 필드 외의 다른
    // 필드들을 모두 0으로 세팅해놓고 Logical Redo를 수행하여,
    // 예상치 못한 Membase 필드 참조를 미연에 Detect하기 위함이다.
    smmMemBase   sLogicalRedoMembase;
    
    UInt         sFreePageListCount;
    scPageID     sNewChunkFirstPID ;
    scPageID     sNewChunkLastPID  ;
    scPageID     sFirstFreePageID ;
    vULong       sFreePageCount ;
    UInt         sExpandPageListID;    
    UInt         sDbFileNo;
    UInt         i;
    smmTBSNode  *sTBSNode;
    idBool       sIsExistTBS;
    idBool       sRecoverMembase;
    idBool       sRecoverNewChunk;
    idBool       sDummy;

    IDE_DASSERT( a_pid == (scPageID)0 );
    IDE_DASSERT( a_pImage != NULL );

    // [ 1 ] 0번 페이지를 포함하는 데이타파일이 오류가 있는지 
    // 검사하여 Membase를 복구할 것인지를 결정한다. 
    IDE_TEST( smmTBSMediaRecovery::findMatchFailureDBF( a_spaceid,
                                                        a_pid,
                                                        &sIsExistTBS,
                                                        &sRecoverMembase ) 
              != IDE_SUCCESS );

    // filter에서 존재하지 않는 테이블스페이스의 로그는 걸러냈다
    IDE_ERROR( sIsExistTBS == ID_TRUE );
    
    // Logical Membase 초기화 
    idlOS::memset( & sLogicalRedoMembase,
                   0,
                   ID_SIZEOF( smmMemBase ) );

    // Logical Redo가 여러번 되어도 문제없도록 하기 위해
    // Logical Redo전에 Membase의 정보를 로그 기록 당시로 돌려놓는다.

    // Log분석 시작 ++++++++++++++++++++++++++++++++++++++++++++++
    idlOS::memcpy( & sLogicalRedoMembase.mAllocPersPageCount,
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mAllocPersPageCount ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mAllocPersPageCount );

    idlOS::memcpy( & sLogicalRedoMembase.mCurrentExpandChunkCnt,
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mCurrentExpandChunkCnt ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mCurrentExpandChunkCnt );

    idlOS::memcpy( & sLogicalRedoMembase.mExpandChunkPageCnt,
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mExpandChunkPageCnt ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mExpandChunkPageCnt );

    idlOS::memcpy( & sLogicalRedoMembase.mDBFileCount[0],
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[0] ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[0] );

    idlOS::memcpy( & sLogicalRedoMembase.mDBFileCount[1],
                   a_pImage,
                   ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[1] ) );
    a_pImage += ID_SIZEOF( sLogicalRedoMembase.mDBFileCount[1] );

    idlOS::memcpy( & sFreePageListCount,
                   a_pImage,
                   ID_SIZEOF(UInt) );
    a_pImage += ID_SIZEOF(UInt);

    sLogicalRedoMembase.mFreePageListCount = sFreePageListCount;

    for ( i = 0 ;
          i < (UInt) sFreePageListCount ;
          i ++ )
    {
        idlOS::memcpy( &sFirstFreePageID,
                       a_pImage,
                       ID_SIZEOF(scPageID) );
        a_pImage += ID_SIZEOF( scPageID );

        idlOS::memcpy( &sFreePageCount,
                       a_pImage,
                       ID_SIZEOF(vULong) );
        a_pImage += ID_SIZEOF( vULong );


        sLogicalRedoMembase.mFreePageLists[i].mFirstFreePageID = sFirstFreePageID;
        sLogicalRedoMembase.mFreePageLists[i].mFreePageCount = sFreePageCount;

        // Redo하는 Page외의 다른 Page의 Validity를 보장할 수 없기 때문에,
        // Redo중에는 Free Page List의 Page수를 체크하지 않는다.
        IDE_DASSERT( smmFPLManager::isValidFPL( a_spaceid,
                                                & sLogicalRedoMembase.mFreePageLists[i] )
                     == ID_TRUE );
    }
    
    idlOS::memcpy( &sExpandPageListID,      
                   a_pImage, 
                   ID_SIZEOF(UInt) );
    a_pImage += ID_SIZEOF(UInt);    

    idlOS::memcpy( &sNewChunkFirstPID, 
                   a_pImage, 
                   ID_SIZEOF(scPageID) );
    a_pImage += ID_SIZEOF(scPageID);

    idlOS::memcpy( &sNewChunkLastPID, 
                   a_pImage, 
                   ID_SIZEOF(scPageID) );
    a_pImage += ID_SIZEOF(scPageID);
    // Log분석 끝 ------------------------------------------------

    // [ 2 ] Chunk 확장범위의 페이지를 포함하는 데이타파일이 
    // 오류가 있는지 검사하여 복구할 것인지를 결정한다. 
    // Chunk는 데이타파일간에 걸쳐서 존재하지 않는다. 
    IDE_TEST( smmTBSMediaRecovery::findMatchFailureDBF( a_spaceid,
                                                        sNewChunkFirstPID,
                                                        &sIsExistTBS,
                                                        &sRecoverNewChunk ) 
              != IDE_SUCCESS );

    if ( (sRecoverMembase == ID_TRUE) || 
         (sRecoverNewChunk == ID_TRUE) )
    {
        IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( a_spaceid,
                                                            (void**)&sTBSNode )
                  != IDE_SUCCESS );

        // BUG-27456 Klocwork SM (4)
        IDE_ERROR( sTBSNode->mMemBase != NULL );

        sLogicalRedoMembase.mDBFilePageCount = sTBSNode->mMemBase->mDBFilePageCount;

        // Membase를 복구하거나 0번외의 데이타파일을 복구하더라도 
        // sLogical
        // Logical Redo 수행을 위해 Membase를 바꿔친다.
        sOrgMembase        = sTBSNode->mMemBase;
        sTBSNode->mMemBase = &sLogicalRedoMembase;

        // Logical Redo 이전의 DB File수 
        sOrgDBFileCount = sLogicalRedoMembase.mDBFileCount[0];

        // Logical Redo로 Expand Chunk를 할당한다.
        IDE_TEST( smmManager::allocNewExpandChunk4MR( sTBSNode,
                                                      sNewChunkFirstPID,
                                                      sNewChunkLastPID,
                                                      sRecoverMembase,
                                                      sRecoverNewChunk ) 
                  != IDE_SUCCESS );

        // Chunk할당 도중 새로 생겨난 DB파일에 대한 처리
        //
        // sOrgDBFileCount 
        //    logical redo이전의 DB File수  => 새로 생성된 첫번째 DB File 번호
        // sLogicalRedoMembase.m_nDBFileCount[0]-1
        //    새 Membase의 DB File수 -1 => 새로 생성된 마지막 DB File 번호
        //
        // ( m_nDBFileCount[0], [1] 모두 항상 같은 값으로 설정되므로,
        //   아무 값이나 사용해도 된다. )
        for ( sDbFileNo  = sOrgDBFileCount ; 
              sDbFileNo <= sLogicalRedoMembase.mDBFileCount[0]-1 ;
              sDbFileNo ++ )
        {
            // BUGBUG 
            // 미디어복구시에는 반드시 파일이 존재한다는 전제가 있다. 
            // 그렇기 때문에 다음 함수를 호출할 필요없이 파일을 찾아
            // Open하는 함수로 대체하면 된다. 

            // DB File이 존재하면 open하고 없으면 생성한다.
            IDE_TEST( smmManager::openOrCreateDBFileinRecovery( sTBSNode,
                                                                sDbFileNo,
                                                                &sDummy )
                      != IDE_SUCCESS );

            // 모든 DB File에 존재하는 토탈 Page Count 계산.
            // Performance View용이며 정확히 일치하지 않아도 된다.
            IDE_TEST( smmManager::calculatePageCountInDisk( sTBSNode )
                      != IDE_SUCCESS );
        }

        // To Fix BUG-15112
        // Restart Recovery중에 Page Memory가 NULL인 Page에 대한 Redo시
        // 해당 페이지를 그때 그때 필요할 때마다 할당한다.
        // 여기에서 페이지 메모리를 미리 할당해둘 필요가 없다

        // Membase를 원래대로 돌려놓는다.
        sTBSNode->mMemBase = sOrgMembase;

        if ( sRecoverMembase == ID_TRUE )
        {
            // Membase의 미디어 복구가 필요한 경우 
            IDE_ASSERT( smmManager::getOIDPtr( a_spaceid,
                                               SM_MAKE_OID( a_pid, a_offset ),
                                               (void**)&sMembase )
                        == IDE_SUCCESS );

            IDE_ERROR_MSG( sMembase == sOrgMembase,
                           "sMembase    : %"ID_vULONG_FMT"\n"
                           "sOrgMembase : %"ID_vULONG_FMT"\n",
                           sMembase,
                           sOrgMembase );

            // Logical Redo의 결과로 만들어진 Membase의 정보를
            // 실제 Membase에 세팅
            sMembase->mAllocPersPageCount = sLogicalRedoMembase.mAllocPersPageCount;
            sMembase->mCurrentExpandChunkCnt = sLogicalRedoMembase.mCurrentExpandChunkCnt;


            IDE_DASSERT( sMembase->mExpandChunkPageCnt == 
                         sLogicalRedoMembase.mExpandChunkPageCnt );

            sMembase->mFreePageListCount = sLogicalRedoMembase.mFreePageListCount;

            sMembase->mDBFileCount[0] = sLogicalRedoMembase.mDBFileCount[0];

            sMembase->mDBFileCount[1] = sLogicalRedoMembase.mDBFileCount[1];

            IDE_DASSERT( sMembase->mFreePageListCount ==  sFreePageListCount );

            for ( i = 0;
                  i < sFreePageListCount ;
                  i ++ )
            {
                // Redo하는 Page외의 다른 Page의 Validity를 보장할 수 
                // 없기 때문에, Redo중에는 Free Page List의 Page수를 
                // 체크하지 않는다.
                IDE_DASSERT( smmFPLManager::isValidFPL( a_spaceid,
                                                        & sLogicalRedoMembase.mFreePageLists[i] )
                             == ID_TRUE );

                sMembase->mFreePageLists[i].mFirstFreePageID = 
                    sLogicalRedoMembase.mFreePageLists[i].mFirstFreePageID ;

                sMembase->mFreePageLists[i].mFreePageCount =
                    sLogicalRedoMembase.mFreePageLists[i].mFreePageCount ;
            }

            // 0 번 Page를 Dirty 페이지로 추가
            IDE_TEST( smmDirtyPageMgr::insDirtyPage(a_spaceid, a_pid) 
                      != IDE_SUCCESS );
        }
        else
        {
            // Membase 복구가 필요없는 경우
        }
    }
    else
    {
        // 미디어 복구가 필요없는 Chunk 인 경우 
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

