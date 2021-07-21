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
 * $Id: smmFixedMemoryMgr.cpp 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smmShmKeyMgr.h>
#include <smmReq.h>

iduMemListOld   smmFixedMemoryMgr::mSCHMemList;

smmFixedMemoryMgr::smmFixedMemoryMgr()
{
}

IDE_RC smmFixedMemoryMgr::initializeStatic()
{
    
    IDE_TEST( smmShmKeyMgr::initializeStatic() != IDE_SUCCESS );
    
    IDE_TEST( mSCHMemList.initialize(IDU_MEM_SM_SMM,
                                     0,
                                     (SChar*) "SCH_LIST",
                                     ID_SIZEOF(smmSCH),
                                     32,
                                     8 )
             != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smmFixedMemoryMgr::destroyStatic()
{
    IDE_TEST( mSCHMemList.destroy() != IDE_SUCCESS );

    IDE_TEST( smmShmKeyMgr::destroyStatic() != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC smmFixedMemoryMgr::initialize(smmTBSNode * aTBSNode)
{

    SChar sBuffer[128];
    key_t sShmKey;
    
    sShmKey = aTBSNode->mTBSAttr.mMemAttr.mShmKey;
    aTBSNode->mBaseSCH.m_shm_id = 0;
    aTBSNode->mBaseSCH.m_header = NULL;
    aTBSNode->mBaseSCH.m_next   = NULL;
    aTBSNode->mTailSCH          = NULL;

    idlOS::memset(&aTBSNode->mTempMemBase, 0, ID_SIZEOF(smmTempMemBase));
    aTBSNode->mTempMemBase.m_first_free_temp_page  = NULL;
    aTBSNode->mTempMemBase.m_alloc_temp_page_count = 0;
    aTBSNode->mTempMemBase.m_free_temp_page_count  = 0;

    // To Fix PR-12974
    // SBUG-3 Mutex Naming
    idlOS::memset(sBuffer, 0, 128);

    idlOS::snprintf( sBuffer,
                     128,
                     "SMM_FIXED_PAGE_POOL_MUTEX_FOR_SHMKEY_%"ID_UINT32_FMT,
                     (UInt) sShmKey );
    
    IDE_TEST(aTBSNode->mTempMemBase.m_mutex.initialize(
                 sBuffer,
                 IDU_MUTEX_KIND_NATIVE,
                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);
    
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC smmFixedMemoryMgr::checkExist(key_t         aShmKey,
                                     idBool&       aExist,
                                     smmShmHeader *aHeader)
{

    smmShmHeader *sShmHeader;
    SInt          sFlag;
    PDL_HANDLE    sShmID;

#if !defined (NTO_QNX) && !defined (VC_WIN32)
    sFlag       = 0600; /*|IPC_CREAT|IPC_EXCL*/
#elif defined (USE_WIN32IPC_DAEMON)
    sFlag       = 0600;
#else
    sFlag       = IPC_CREAT | 0600;
#endif
    aExist     = ID_FALSE;

    /* [1] shmid 얻기 */
    sShmID = idlOS::shmget( aShmKey, SMM_CACHE_ALIGNED_SHM_HEADER_SIZE, sFlag );


    if (sShmID == PDL_INVALID_HANDLE)
    {
        switch(errno)
        {
        case ENOENT: // 해당 Key에 대한 ID 실패 : 없음 => 정상
            break;
        case EACCES: // 존재하지만, 권한이 없음 : 에러!
            IDE_RAISE(no_permission_error);
        default:
            IDE_RAISE(shmget_error);
        }
    }
    else
    {
        /* [2] attach 수행  */
        sShmHeader = (smmShmHeader *)idlOS::shmat(sShmID, 0, sFlag);
        IDE_TEST_RAISE(sShmHeader == (smmShmHeader *)-1, shmat_error);

        if (aHeader != NULL)
        {
        /* [3] shm header copy  */
            idlOS::memcpy(aHeader, sShmHeader, ID_SIZEOF(smmShmHeader) );
        }
#if defined (NTO_QNX) || (defined (VC_WIN32) && !defined (USE_WIN32IPC_DAEMON))
        /* [5] control(IPC_RMID) */
        IDE_TEST_RAISE(idlOS::shmctl(sShmID, IPC_RMID, NULL) < 0, shmdt_error);
#endif
        /* [4] detach */
        IDE_TEST_RAISE(idlOS::shmdt( (char*)sShmHeader ) < 0, shmdt_error);

#if !defined (NTO_QNX) && !defined (VC_WIN32)
        aExist = ID_TRUE; // 이미 공유 메모리가 존재함. OK
#elif defined (USE_WIN32IPC_DAEMON)
        aExist = ID_TRUE; // 이미 공유 메모리가 존재함. OK
#else
        aExist = ID_FALSE;
#endif
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(shmget_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_SysShmGet));
    }
    IDE_EXCEPTION(shmat_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_SysShmAt));
    }
    IDE_EXCEPTION(no_permission_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Shm_No_Permission));
    }
    IDE_EXCEPTION(shmdt_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_SysShmDt));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 ----------------------------------------------------------------------------
    checkExist() 이후에 호출되므로, 별도의 errno 검사 필요 없음!
    attach for creation whole database size
 
 ----------------------------------------------------------------------------
 -  PROJ-1548 Memory Tablespace
   - 다른 Tablespace의 공유메모리 Chunk로 Attach되는 상황 Detect
     - 실제로 다른 Tablespace의 공유메모리를 Attach할 수는 없다.
     - 하지만 확실히 체크하는 의미로 공유메모리 Attach시
       Tablespace ID를 체크한다.
     - 방법 : 공유메모리헤더(smmShmHeader)에 TBSID를 넣어 
       Attach하려는 TBSID와 공유메모리의 TBSID가 같은지 체크한다.
*/

IDE_RC smmFixedMemoryMgr::attach(smmTBSNode * aTBSNode,
                                 smmShmHeader *aRsvShmHeader)
{


    smmShmHeader *sNewShmHeader = NULL;
    SInt          sFlag;
    PDL_HANDLE    sShmID;
    smmSCH       *sSCH;
    key_t         sCurKey;

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aRsvShmHeader != NULL);
    
    sFlag       = 0600; /*|IPC_CREAT|IPC_EXCL*/

    /* smmFixedMemoryMgr_attach_alloc_SCH.tc */
    IDU_FIT_POINT("smmFixedMemoryMgr::attach::alloc::SCH");
    IDE_TEST(mSCHMemList.alloc((void **)&sSCH) != IDE_SUCCESS);

    
    /* ------------------------------------------------
     * [1] Base SHM Chunk에 대한 attach
     * ----------------------------------------------*/
    sCurKey = aTBSNode->mTBSAttr.mMemAttr.mShmKey;
    
    sShmID  = idlOS::shmget(sCurKey,
                            SMM_SHM_DB_SIZE(aRsvShmHeader->m_page_count),
                            sFlag );
    IDE_TEST_RAISE( sShmID == PDL_INVALID_HANDLE, not_exist_error);

    sNewShmHeader  = (smmShmHeader *)idlOS::shmat(sShmID, 0, sFlag);
    IDE_TEST_RAISE(sNewShmHeader == (smmShmHeader *)-1, shmat_error);

    // 공유메모리 Key관리자에게 현재 사용중인 Key를 알려준다.
    IDE_TEST( smmShmKeyMgr::notifyUsedKey( sCurKey )
              != IDE_SUCCESS );
    
    sSCH->m_header = sNewShmHeader;
    sSCH->m_shm_id = (SInt)sShmID;
    sSCH->m_next   = NULL;

    aTBSNode->mBaseSCH.m_next = sSCH;
    aTBSNode->mTailSCH        = sSCH;
    //mMemBase       = (smmMemBase *)((UChar *)sNewShmHeader + SMM_MEMBASE_OFFSET);

    IDE_TEST_RAISE(sNewShmHeader->m_versionID != smVersionID,
                   version_mismatch_error);
    IDE_TEST_RAISE(sNewShmHeader->m_valid_state != ID_TRUE,
                   invalid_shm_error);

    //  PROJ-1548 Memory Tablespace
    // 다른 Tablespace의 공유메모리 Chunk로 Attach되는 상황 Detect
    IDE_TEST_RAISE(sNewShmHeader->mTBSID != aTBSNode->mHeader.mID,
                   tbsid_mismatch_error);
    

    /* ------------------------------------------------
     * [2] Sub SHM Chunk attach
     * smmShmHeader 의 내용은 공유메모리에 존재하는 내용이고,
     * 이것에 다음 공유메모리의 key가 저장되어있다.
     * 이 키를 바탕으로 다음 공유 메모리의 위치를 정확하게 attach할 수 있다.
     * smmSCH의 내용은 동적인 내용으로 새로 setting하면서 진행한다.
     * ----------------------------------------------*/
    while (aTBSNode->mTailSCH->m_header->m_next_key != 0)
    {
        smmShmHeader  sTempShmHeader;
        idBool        sExist = ID_FALSE;
        smmSCH       *sNewSCH;

        IDE_TEST(checkExist(aTBSNode->mTailSCH->m_header->m_next_key,
                            sExist,
                            &sTempShmHeader) != IDE_SUCCESS);
        IDE_TEST_RAISE(sExist == ID_FALSE, not_exist_error);

        /* smmFixedMemoryMgr_attach_alloc_NewSCH.tc */
        IDU_FIT_POINT("smmFixedMemoryMgr::attach::alloc::NewSCH");
        IDE_TEST(mSCHMemList.alloc((void **)&sNewSCH)
                 != IDE_SUCCESS);
        // 존재함.
        sCurKey = aTBSNode->mTailSCH->m_header->m_next_key;
        sShmID  = idlOS::shmget(aTBSNode->mTailSCH->m_header->m_next_key,
                                SMM_SHM_DB_SIZE(sTempShmHeader.m_page_count),
                                sFlag );
        
        IDE_TEST_RAISE( sShmID == PDL_INVALID_HANDLE, shmget_error);

        IDE_TEST( smmShmKeyMgr::notifyUsedKey( sCurKey )
                  != IDE_SUCCESS );
        
        sNewShmHeader  = (smmShmHeader *)idlOS::shmat(sShmID, 0, sFlag);
        IDE_TEST_RAISE(sNewShmHeader == (smmShmHeader *)-1, shmat_error);

        IDE_TEST_RAISE(sNewShmHeader->m_versionID != smVersionID,
                       version_mismatch_error);

        IDE_TEST_RAISE(sNewShmHeader->m_valid_state != ID_TRUE,
                       invalid_shm_error);

        IDE_TEST_RAISE(sNewShmHeader->mTBSID != aTBSNode->mHeader.mID,
                       tbsid_mismatch_error);
        
        sSCH->m_next      = sNewSCH;
        sNewSCH->m_header = sNewShmHeader;
        sNewSCH->m_shm_id = (SInt)sShmID;
        sNewSCH->m_next   = NULL;

        aTBSNode->mTailSCH = sNewSCH;
        sSCH               = sNewSCH;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(invalid_shm_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Shm_Link_Invalid_State, (UInt)sCurKey));
    }
    IDE_EXCEPTION(version_mismatch_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Shm_Link_Invalid_Version, (UInt)sCurKey));
    }
    IDE_EXCEPTION(tbsid_mismatch_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Shm_Link_Invalid_TBSID,
                                (UInt)sNewShmHeader->mTBSID,
                                (UInt)aTBSNode->mHeader.mID));
    }
    IDE_EXCEPTION(not_exist_error); // 일부 링크가 끊어진 것임.
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Shm_Link_Not_Exist));
    }
    IDE_EXCEPTION(shmget_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_SysShmGet));
    }
    IDE_EXCEPTION(shmat_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_SysShmAt));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC smmFixedMemoryMgr::detach(smmTBSNode * aTBSNode)
{
    // To Fix PR-12974
    // SBUG-4 Conding Convention
    smmSCH *sNextSCH = NULL;

    smmSCH *sSCH = aTBSNode->mBaseSCH.m_next;

    while(sSCH != NULL)
    {
        sNextSCH = sSCH->m_next; // to prevent FMR
        IDE_TEST_RAISE(idlOS::shmdt( (char*)sSCH->m_header ) < 0, shmdt_error);
        IDE_TEST(mSCHMemList.memfree(sSCH) != IDE_SUCCESS);
        sSCH = sNextSCH;
    }

    aTBSNode->mBaseSCH.m_next = NULL;
    aTBSNode->mTailSCH        = NULL;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(shmdt_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_SysShmDt));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

inline void smmFixedMemoryMgr::linkShmPage(smmTBSNode *   aTBSNode,
                                           smmTempPage   *aPage)
{
    aPage->m_header.m_self = (smmTempPage *)SMM_SHM_LOCATION_FREE;
    aPage->m_header.m_prev = NULL;
    aPage->m_header.m_next = aTBSNode->mTempMemBase.m_first_free_temp_page;
    aTBSNode->mTempMemBase.m_first_free_temp_page = aPage;
    aTBSNode->mTempMemBase.m_free_temp_page_count++;
}


/* ------------------------------------------------
 * [] 이 함수의 목적은 공유메모리 DB에서
 *    비정상적인 종료를 했을 경우, 해당
 *    공유메모리의 링크가 올바른지를 나타내는
 *    플래그를 설정하는 것이다.
 *    restart시 attach()를 수행할 때
 *    validate flag가 검사된다.
 * ----------------------------------------------*/

void smmFixedMemoryMgr::setValidation(smmTBSNode * aTBSNode, idBool aFlag)
{


    smmSCH *sSCH = aTBSNode->mBaseSCH.m_next;

    if (sSCH != NULL)
    {
        sSCH->m_header->m_valid_state = aFlag;
    }

}


/*
   Tablespace의 첫번째 공유메모리 Chunk를 생성한다.

   [IN] aTBSNode   - 공유메모리 관리자를 가지는 Tablespace Node
   [IN] aPageCount - 첫번째 Chunk의 크기 ( 단위:Page수 )
                     - 0 이면 SHM_PAGE_COUNT_PER_KEY프로퍼티에
                       지정한 Page수만큼 할당 
   
   - Use Case
     - Startup시 공유메모리 생성후 Restore하는 경우 
     - 운영 도중 신규 Tablespace생성시
   
   - PROJ-1548 User Memory Tablespace ======================================
     - [Design]
     
       - 공유메모리 Key 관리자로부터 후보 공유메모리 Key를 할당받아
         공유메모리 생성을 시도한다.
         
       - 공유메모리 생성 성공시 공유메모리 Key를
         Log Anchor에 저장한다.
         
       - Log Anchor를 Flush는 이 함수의 호출한 함수가 처리해야한다.
 */
IDE_RC smmFixedMemoryMgr::createFirstChunk( smmTBSNode * aTBSNode,
                                            UInt         aPageCount /* = 0 */ )
{
    key_t sChunkShmKey;

    IDE_DASSERT( aTBSNode != NULL );

    // 공유메모리 Chunk를 할당받고 공유메모리 Key를 sChunkShmKey에 설정
    IDE_TEST( extendShmPage(aTBSNode,
                            aPageCount,
                            & sChunkShmKey ) != IDE_SUCCESS );

    // 첫번째 공유메모리 Chunk의 Key를 TBSNode에 설정
    aTBSNode->mTBSAttr.mMemAttr.mShmKey = sChunkShmKey ;

    // Restore마치고 Resatrt Redo/Undo까지 완료후
    // 공유메모리 Key를 Log Anchor에 Flush한다.
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smmFixedMemoryMgr::allocNewChunk(smmTBSNode * aTBSNode,
                                        key_t        aShmKey,
                                        UInt         aPageCount)
{
    scPageID      i;
    PDL_HANDLE    sShmID;
    SInt          sFlag;
    smmShmHeader *sShmHeader;
    smOID         sPersSize;
    UChar        *sPersPage;
    smmSCH       *sSCH;
    SInt          sState = 0;


    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aShmKey != 0 );
    IDE_DASSERT( aPageCount > 0 );
    
    /* [0] arg 계산 */
    sFlag   = 0600 | IPC_CREAT | IPC_EXCL;
    sPersSize = SMM_SHM_DB_SIZE(aPageCount);

    /* smmFixedMemoryMgr_allocNewChunk_alloc_SCH.tc */
    IDU_FIT_POINT("smmFixedMemoryMgr::allocNewChunk::alloc::SCH");
    IDE_TEST(mSCHMemList.alloc((void **)&sSCH) != IDE_SUCCESS);
    sState = 1;

    /* [1] shmid 얻기 */
    sShmID = idlOS::shmget(aShmKey, sPersSize, sFlag );
    if (sShmID == PDL_INVALID_HANDLE)
    {
        IDE_TEST_RAISE(errno == EEXIST, already_exist_error);
        IDE_TEST_RAISE(errno == ENOSPC, no_space_error);
        IDE_RAISE(shmget_error);
    }
    /* [1] attach 수행  */
    sShmHeader  = (smmShmHeader *)idlOS::shmat(sShmID, 0, sFlag);
    IDE_TEST_RAISE(sShmHeader == (smmShmHeader *)-1, shmat_error);
    idlOS::memset(sShmHeader, 0, sPersSize);

    /* [2] SCH assign  */
    sSCH->m_next   = NULL;
    sSCH->m_shm_id = (SInt)sShmID;
    sSCH->m_header = sShmHeader;

    sShmHeader->m_valid_state = ID_FALSE;
    sShmHeader->m_versionID   = smVersionID;
    sShmHeader->m_page_count  = aPageCount;
    // PROJ-1548 Memory Tablespace
    // 다른 Tablespace의 공유메모리 Chunk로 Attach되는 상황 Detect위해 필요
    sShmHeader->mTBSID        = aTBSNode->mHeader.mID;
    sShmHeader->m_next_key    = (key_t)0;
    sPersPage                 = ((UChar *)(sShmHeader)
                                          + SMM_CACHE_ALIGNED_SHM_HEADER_SIZE);

    // Free List로 입력
    for (i = 0; i < aPageCount; i++)
    {
        linkShmPage(aTBSNode, (smmTempPage *)(sPersPage + (i * SM_PAGE_SIZE)));
    }

    /* ------------------------------------------------
     * [링크 연결] Tail <--> New
     * ----------------------------------------------*/

    if (aTBSNode->mTailSCH == NULL)
    {
        aTBSNode->mBaseSCH.m_next = sSCH;
        aTBSNode->mTailSCH        = sSCH;
    }
    else
    {
        smmSCH       *sTempTail;

        sTempTail = aTBSNode->mTailSCH;
        sTempTail->m_header->m_valid_state = ID_FALSE;
        
        // To Fix PR-12974
        // SBUG-5
        // Code 순서가 변경될 경우, server restart 시
        // 잘못된 Shared Memory 버전의 내용을 읽을 수 있다. 
        IDL_MEM_BARRIER;
        {
            sTempTail->m_header->m_next_key = aShmKey;
            sTempTail->m_next               = sSCH;
            aTBSNode->mTailSCH              = sSCH;
        }
        IDL_MEM_BARRIER;
        sTempTail->m_header->m_valid_state = ID_TRUE;
    }

    aTBSNode->mTempMemBase.m_alloc_temp_page_count += aPageCount;
    
    // To Fix PR-12974
    // SBUG-5 kmkim 의 의견을 따름.
    IDL_MEM_BARRIER;
    aTBSNode->mTailSCH->m_header->m_valid_state    = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(shmget_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_SysShmGet));
    }
    IDE_EXCEPTION(already_exist_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_already_created));
    }
    IDE_EXCEPTION(no_space_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoMore_SHM_Page));
    }
    IDE_EXCEPTION(shmat_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_SysShmAt));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if (sState == 1)
    {
        IDE_ASSERT( mSCHMemList.memfree(sSCH) == IDE_SUCCESS );
    }
    
    IDE_POP();

    return IDE_FAILURE;

}

/*
   TBSNode의 정보중 공유메모리 관리자 관련 부분을 해제한다.
 */
IDE_RC smmFixedMemoryMgr::destroy(smmTBSNode * aTBSNode)
{
    // 이미 detach되거나 remove된 상태여야 한다.
    IDE_ASSERT( aTBSNode->mBaseSCH.m_next == NULL );
    IDE_ASSERT( aTBSNode->mTailSCH == NULL );

    IDE_TEST( aTBSNode->mTempMemBase.m_mutex.destroy()
              != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
   공유메모리를 제거한다.

   aTBSNode [IN] 공유메모리를 제거할 Tablespace

   - [ PROJ-1548 ] User Memory Tablespace -------------------------------
   
   Tablespace drop시에 Tablespace에서 사용중이던 공유메모리 Key를
   다른 Tablespace에서 재활용할 수 있어야 한다.

   문제 : Tablespace 가 drop될때 해당 Tablespace에서 사용중이던
          공유메모리 Key를 재활용하지 않을 경우
          Tablespace create/drop을 반복하면 공유메모리 Key가
          고갈되는 문제가 생김

   해결책 : Tablespace가 drop될때 해당 Tablespace에서 사용중이던
             공유메모리 Key를 재활용한다.
  
 */
IDE_RC smmFixedMemoryMgr::remove(smmTBSNode * aTBSNode)
{
    smmSCH *sSCH = NULL;

    IDE_DASSERT( aTBSNode != NULL );
    
    // PROJ-1548
    // 더 이상 사용하지 않는 공유메모리 Key가 재활용될 수 있도록
    // 공유메모리 키 관리자에게 알려주어야 한다.

    IDE_DASSERT( aTBSNode->mTBSAttr.mMemAttr.mShmKey != 0 );
    
    // 첫 번째 공유메모리 Key를 반납
    IDE_TEST( smmShmKeyMgr::notifyUnusedKey(
                  aTBSNode->mTBSAttr.mMemAttr.mShmKey )
              != IDE_SUCCESS );
    
    sSCH = aTBSNode->mBaseSCH.m_next;
    while(sSCH != NULL)
    {
        if ( sSCH->m_header->m_next_key != 0 )
        {
            IDE_TEST( smmShmKeyMgr::notifyUnusedKey(
                          sSCH->m_header->m_next_key )
                      != IDE_SUCCESS );
        }
            
        smmSCH *sNextSCH = sSCH->m_next; // to prevent FMR

        IDE_TEST_RAISE(idlOS::shmdt( (char*)sSCH->m_header ) < 0, shmdt_error);
        IDE_TEST_RAISE(idlOS::shmctl((PDL_HANDLE)sSCH->m_shm_id, IPC_RMID, NULL) == -1, shmctl_error);
        IDE_TEST(mSCHMemList.memfree(sSCH) != IDE_SUCCESS);

        sSCH = sNextSCH;
        aTBSNode->mBaseSCH.m_next = sNextSCH;
    }

    aTBSNode->mTailSCH = NULL;
    return IDE_SUCCESS;

    IDE_EXCEPTION(shmdt_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_SysShmDt));
    }
    IDE_EXCEPTION(shmctl_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_SysShmCtl));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;


}

IDE_RC smmFixedMemoryMgr::freeShmPage(smmTBSNode * aTBSNode,
                                      smmTempPage *aPage)
{

    IDE_TEST( aTBSNode->mTempMemBase.m_mutex.lock( NULL /* idvSQL* */ )
              != IDE_SUCCESS);
    
    linkShmPage(aTBSNode, aPage);

    IDE_TEST( aTBSNode->mTempMemBase.m_mutex.unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smmFixedMemoryMgr::allocShmPage(smmTBSNode *  aTBSNode,
                                       smmTempPage **aPage)
{

    UInt sState=0;

    IDE_TEST( aTBSNode->mTempMemBase.m_mutex.lock(NULL /* idvSQL* */ )
              != IDE_SUCCESS);
    sState=1;

    if (aTBSNode->mTempMemBase.m_first_free_temp_page == NULL)
    {
        // 공유메모리 Key 하나당 SHM_PAGE_COUNT_PER_KEY 프로퍼티만큼의
        // Page를 할당하도록 한다.
        IDE_TEST(extendShmPage(
                     aTBSNode,
                     (scPageID)smuProperty::getShmPageCountPerKey() )
                 != IDE_SUCCESS);
    }

    *aPage = aTBSNode->mTempMemBase.m_first_free_temp_page;

    aTBSNode->mTempMemBase.m_first_free_temp_page = (*aPage)->m_header.m_next;
    aTBSNode->mTempMemBase.m_free_temp_page_count--;

    sState=0;
    IDE_TEST( aTBSNode->mTempMemBase.m_mutex.unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if(sState==1)
    {
        IDE_ASSERT( aTBSNode->mTempMemBase.m_mutex.unlock() == IDE_SUCCESS );
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/*
     공유메모리 Chunk를 하나 새로 생성하여 페이지 메모리를 확보한다

     aTBSNode       [IN]  공유메모리 Chunk를 생성할 Tablespace
     aCount         [IN]  확보할 DB Page의 수
     aCreatedShmKey [OUT] 생성한 공유메모리 Chunk의 Key값
                          NULL이 아닌 경우에만 세팅한다.
 */
// BUGBUG : 정교한 에러처리 필요함.
IDE_RC smmFixedMemoryMgr::extendShmPage(smmTBSNode * aTBSNode,
                                        UInt         aCount,
                                        key_t      * aCreatedShmKey /* =NULL*/)
{
    IDE_RC rc;
    key_t sShmKeyCandidate;
    
    while(1)
    {
        // 공유메모리 Key 후보를 가져온다.
        IDE_TEST( smmShmKeyMgr::getShmKeyCandidate( & sShmKeyCandidate)
                  != IDE_SUCCESS );

        
        rc = allocNewChunk(aTBSNode, sShmKeyCandidate, aCount);
        if (rc == IDE_SUCCESS)
        {
            break;
        }
        switch(ideGetErrorCode())
        {
        case smERR_ABORT_already_created: // continue finding;
            break;
        case smERR_ABORT_SysShmGet:
        case smERR_ABORT_NoMore_SHM_Page:
        case smERR_ABORT_SysShmAt:
        default:
            return IDE_FAILURE;
        }
    }
    
    if ( aCreatedShmKey != NULL )
    {
        *aCreatedShmKey = sShmKeyCandidate;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smmFixedMemoryMgr::checkDBSignature(smmTBSNode * aTBSNode,
                                           UInt         aDbNumber)
{

    ULong           sBasePage[SM_PAGE_SIZE / ID_SIZEOF(ULong) ];
    smmMemBase      *sDiskMemBase;
    smmMemBase      *sSharedMemBase = aTBSNode->mMemBase;
    smmDatabaseFile *sFile;

    IDE_TEST( smmManager::openAndGetDBFile( aTBSNode,
                                            aDbNumber,
                                            0,  // aDBFileNo
                                            &sFile)
              != IDE_SUCCESS );

    IDE_TEST( sFile->readPage(aTBSNode, SMM_MEMBASE_PAGEID, (UChar*)sBasePage) 
              != IDE_SUCCESS );

    sDiskMemBase = (smmMemBase *)((UChar*)sBasePage + SMM_MEMBASE_OFFSET);

    IDE_TEST_RAISE (idlOS::strcmp(sSharedMemBase->mDBFileSignature,
                                  sDiskMemBase->mDBFileSignature) != 0,
                    signature_error);
    return IDE_SUCCESS;

    IDE_EXCEPTION(signature_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ShmDB_Signature_Mismatch,
                                sSharedMemBase->mDBFileSignature,
                                sDiskMemBase->mDBFileSignature));
        
        ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                    SM_TRC_MEMORY_DB_SIGNATURE_FATAL1,
                    sSharedMemBase->mDBFileSignature);
        ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                    SM_TRC_MEMORY_DB_SIGNATURE_FATAL2,
                    sDiskMemBase->mDBFileSignature);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
