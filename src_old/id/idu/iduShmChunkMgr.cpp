/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idErrorCode.h>
#include <iduShmDef.h>
//#include <smDef.h>
#include <iduShmLatch.h>
//#include <smmDef.h>
#include <iduShmKeyMgr.h>
#include <iduShmChunkMgr.h>

#define IDU_SHM_ID_OBJECT_COUNT_IN_CHUNK  (32)

iduSCH   ** iduShmChunkMgr::mArrSCH;
iduMemListOld iduShmChunkMgr::mSCHPool;

IDE_RC iduShmChunkMgr::initialize( ULong aMaxSegCnt )
{
    IDE_TEST( iduMemMgr::malloc(
                  IDU_MEM_ID_IDU,
                  ID_SIZEOF(iduSCH*) * aMaxSegCnt,
                  (void**)&mArrSCH )
              != IDE_SUCCESS );

    IDE_TEST( mSCHPool.initialize( IDU_MEM_ID_IDU,
                                   0,  /* aSeqNumber */
                                   (SChar*) "SCH_LIST",
                                   ID_SIZEOF(iduSCH),
                                   32, /* aElemCnt */
                                   8 ) /* aAutofreeChunkLimit */
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::destroy()
{
    IDE_ASSERT( mSCHPool.destroy( ID_TRUE /* Check Validation */ )
                == IDE_SUCCESS );

    IDE_ASSERT( iduMemMgr::free( mArrSCH ) == IDE_SUCCESS );
    return IDE_SUCCESS;
}

// System Chunk를 생성한다.
IDE_RC iduShmChunkMgr::createSysChunk( UInt      aSize,
                                       iduSCH ** aNewSCH )
{
    IDE_TEST( allocSHMChunk( aSize,
                             IDU_SHM_TYPE_SYSTEM_SEGMENT,
                             aNewSCH )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::allocSHMChunk( UInt          aSize,
                                      iduShmType    aShmType,
                                      iduSCH     ** aNewSCH )
{
    IDE_RC         sRC;
    key_t          sShmKeyCandidate;
    PDL_HANDLE     sShmID;
    void         * sChunkPtr;
    iduSCH       * sNewSCH;
    UInt           sState = 0;

    IDE_TEST( mSCHPool.alloc( (void**)&sNewSCH ) != IDE_SUCCESS );
    sState = 1;

    while(1)
    {
        if( aShmType == IDU_SHM_TYPE_SYSTEM_SEGMENT )
        {
            sShmKeyCandidate = iduProperty::getShmDBKey();
        }
        else
        {
            // 공유메모리 Key 후보를 가져온다.
            IDE_TEST( iduShmKeyMgr::getShmKeyCandidate( &sShmKeyCandidate )
                      != IDE_SUCCESS );
        }

        sRC = createSHMChunkFromOS( sShmKeyCandidate,
                                    aSize,
                                    &sShmID,
                                    &sChunkPtr );

        if( sRC == IDE_SUCCESS )
        {
            break;
        }
        else
        {
            /* System Chunk는 ShmKey가 지정되어 있기 때문에 attach가
             * 실패하면 바로 error를 리턴한다. */
            IDE_TEST( aShmType == IDU_SHM_TYPE_SYSTEM_SEGMENT );
        }

        switch( ideGetErrorCode() )
        {
        case idERR_ABORT_already_created: // continue finding;
            break;
        case idERR_ABORT_SysShmGet:
        case idERR_ABORT_NoMore_SHM_Page:
        case idERR_FATAL_idc_SHM_ATTACH:
        case idERR_FATAL_Invalid_SHM_DB_KEY:
        default:
            IDE_RAISE( err_shm_create );
            break;
        }
    }

    sNewSCH->mShmID    = sShmID;
    sNewSCH->mShmKey   = sShmKeyCandidate;
    sNewSCH->mSize     = aSize;
    sNewSCH->mChunkPtr = sChunkPtr;

    *aNewSCH = sNewSCH;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shm_create );
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( mSCHPool.memfree( sNewSCH ) == IDE_SUCCESS );
    }

    *aNewSCH = NULL;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::freeShmChunk( UInt aSegID )
{
    iduSCH *sSCH;

    sSCH = mArrSCH[aSegID];

    IDE_ASSERT( sSCH != NULL );

    ideLog::log(IDE_SERVER_0,
                "[RMV-SHM] SHM KEY: %"ID_INT32_FMT", "
                "ID: %"ID_INT32_FMT","
                "SZ: %"ID_UINT32_FMT"",
                sSCH->mShmKey,
                sSCH->mShmID,
                sSCH->mSize );

    IDE_TEST_RAISE( idlOS::shmctl( (PDL_HANDLE)sSCH->mShmID,
                                   IPC_RMID,
                                   NULL) == -1, err_shmctl );

    if( sSCH->mChunkPtr != NULL )
    {
        (void)idlOS::shmdt( (char*)sSCH->mChunkPtr );
        sSCH->mChunkPtr = NULL;
    }

    IDE_TEST( mSCHPool.memfree( sSCH ) != IDE_SUCCESS );
    mArrSCH[aSegID] = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmctl );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SHM_CTL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void iduShmChunkMgr::registerShmChunk( UInt     aIndex,
                                       iduSCH * aSCHPtr )
{
    mArrSCH[aIndex] = aSCHPtr;
}

void iduShmChunkMgr::unregisterShmChunk( UInt aIndex )
{
    mArrSCH[aIndex] = NULL;
}

/* OS로부터 Shared Memory System Chunk를 생성한다. */
IDE_RC iduShmChunkMgr::createSHMChunkFromOS( key_t           aShmKey,
                                             ULong           aSize,
                                             PDL_HANDLE    * aShmID,
                                             void         ** aNewChunkPtr )
{
    SInt           sFlag;
    PDL_HANDLE     sShmID;
    SInt           sState = 0;
    void         * sNewChunk;

    IDE_TEST_RAISE( aShmKey == 0, err_invalid_key );

    /* [0] Arg 계산 */
    sFlag = 0666 | IPC_CREAT | IPC_EXCL;

    /* [1] ShmID 얻기 */
    sShmID = idlOS::shmget( aShmKey, aSize, sFlag );

    if( sShmID == PDL_INVALID_HANDLE )
    {
        IDE_TEST_RAISE( errno == EEXIST, err_already_exist );
        IDE_TEST_RAISE( errno == ENOSPC, err_no_space );
        IDE_RAISE( err_shmget );
    }

    /* [2] Attach 수행  */
    sNewChunk  = idlOS::shmat( sShmID, 0, sFlag );
    IDE_TEST_RAISE( sNewChunk == (void *)-1, err_shmat );
    sState = 1;
   
#if defined(INTEL_LINUX) || defined(ALPHA_LINUX) || defined(NTO_QNX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(AMD64_LINUX) || defined(X86_64_LINUX) || defined(XEON_LINUX)
    if( iduProperty::getShmLock() == 1 )
    { 
        IDE_TEST_RAISE( idlOS::shmctl( sShmID,
                                       SHM_LOCK,
                                       NULL) == -1, err_shmctl );
    }
#endif

    idlOS::memset( sNewChunk, 0, aSize );

    *aShmID       = sShmID;
    *aNewChunkPtr = sNewChunk;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmget );
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_SysShmGet) );
    }
    IDE_EXCEPTION( err_already_exist );
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_already_created) );
    }
    IDE_EXCEPTION( err_no_space );
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_NoMore_SHM_Page) );
    }
    IDE_EXCEPTION( err_shmat );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_idc_SHM_ATTACH) );
    }
    IDE_EXCEPTION( err_shmctl );
    {
        ideLog::log( IDE_SERVER_0, "## errno:%d", errno );
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SHM_CTL ) );
    }
    IDE_EXCEPTION( err_invalid_key );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_Invalid_SHM_DB_KEY) );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sState == 1 )
    {
        IDE_ASSERT( idlOS::shmdt( sNewChunk ) == 0 );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/* BUG-40895 Cannot destroy the shared memory segments which is created as other user */
IDE_RC iduShmChunkMgr::changeSHMChunkOwner( PDL_HANDLE aShmID,
                                            uid_t      aOwnerUID )
{
    struct shmid_ds sStat;

    IDE_TEST_RAISE( idlOS::shmctl( aShmID, IPC_STAT, &sStat ) == -1, err_shmctl );

    IDE_TEST_CONT( sStat.shm_perm.uid == aOwnerUID, cont_finish );

    sStat.shm_perm.uid = aOwnerUID;

    IDE_TEST_RAISE( idlOS::shmctl( aShmID, IPC_SET, &sStat ) == -1, err_shmctl );

    IDE_EXCEPTION_CONT( cont_finish );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmctl );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_CHANGE_OWNER_OF_SHARED_MEMORY_SEGMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::attachAndGetSSegHdr( iduShmSSegment * aSSegHdr )
{
    return attachAndGetSSegHdr( aSSegHdr, iduProperty::getShmDBKey() );
}

IDE_RC iduShmChunkMgr::attachAndGetSSegHdr( iduShmSSegment * aSSegHdr,
                                            key_t            aShmKey )
{
    SInt           sFlag;
    PDL_HANDLE     sSysShmID;
    void         * sAttachSysShmPtr;
    SInt           sState = 0;
    key_t          sShmKey4SSeg;

    IDE_TEST_RAISE( aShmKey == 0, err_invalid_key );

    sShmKey4SSeg = aShmKey;

    sFlag = 0666;

    sSysShmID = idlOS::shmget( sShmKey4SSeg,
                               ID_SIZEOF(iduShmSSegment),
                               sFlag );
    IDE_TEST_RAISE( sSysShmID == PDL_INVALID_HANDLE, err_shmget );

    sAttachSysShmPtr = idlOS::shmat( sSysShmID,
                                     NULL, /* Recommend Attach Addr */
                                     sFlag );
    IDE_TEST_RAISE( sAttachSysShmPtr == (void*)-1, err_shmat );
    sState = 1;

    idlOS::memcpy( aSSegHdr, sAttachSysShmPtr, ID_SIZEOF(iduShmSSegment) );

    sState = 0;
    IDE_TEST_RAISE( idlOS::shmdt( sAttachSysShmPtr ) < 0, err_shmdt );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmget );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_SysShmGet ) );
    }
    IDE_EXCEPTION( err_shmat );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SHM_ATTACH ) );
    }
    IDE_EXCEPTION( err_shmdt );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_SysShmDt) );
    }
    IDE_EXCEPTION( err_invalid_key );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_Invalid_SHM_DB_KEY) );
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( idlOS::shmdt( sAttachSysShmPtr ) == 0 );
    }

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::attachChunk( key_t      aShmKey4SSeg,
                                    UInt       aSize,
                                    iduSCH   * aNewSCH )
{
    SInt           sFlag;
    PDL_HANDLE     sShmID;
    void         * sAttachShmPtr;
    UInt           sState = 0;

    sFlag = 0666;

    sShmID = idlOS::shmget( aShmKey4SSeg, aSize, sFlag );
    IDE_TEST_RAISE( sShmID == PDL_INVALID_HANDLE, err_shmget );

    sAttachShmPtr = idlOS::shmat( sShmID,
                                  NULL/* Recommend Attach Addr */,
                                  sFlag );
    IDE_TEST_RAISE( sAttachShmPtr == (void*)-1, err_shmat );
    sState = 1;

    aNewSCH->mShmKey   = aShmKey4SSeg;
    aNewSCH->mShmID    = sShmID;
    aNewSCH->mSize     = aSize;
    aNewSCH->mChunkPtr = sAttachShmPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmget );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_SysShmGet ) );
    }
    IDE_EXCEPTION( err_shmat );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SHM_ATTACH ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 1:
        IDE_ASSERT( idlOS::shmdt( sAttachShmPtr ) == 0 );
    default:
        break;
    }

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::attachChunk( key_t      aShmKey4SSeg,
                                    UInt       aSize,
                                    iduSCH  ** aNewSCH )
{
    iduSCH       * sNewSCH;
    UInt           sState = 0;

    IDE_TEST( mSCHPool.alloc( (void**)&sNewSCH ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( attachChunk( aShmKey4SSeg,
                           aSize,
                           sNewSCH ) != IDE_SUCCESS );
    sState = 2;

    *aNewSCH = sNewSCH;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 2:
        IDE_ASSERT( idlOS::shmdt( sNewSCH->mChunkPtr ) == 0 );
    case 1:
        IDE_ASSERT( mSCHPool.memfree( sNewSCH ) == 0 );
    default:
        break;
    }

    *aNewSCH = NULL;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::detachChunk( UInt aSegID )
{
    iduSCH  * sCurSCH = mArrSCH[aSegID];

    IDE_TEST_RAISE( idlOS::shmdt( sCurSCH->mChunkPtr ) < 0, err_shmdt );

    IDE_TEST( mSCHPool.memfree( sCurSCH ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmdt );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_SysShmDt) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::removeChunck( UInt aSegID )
{
    iduSCH  * sCurSCH = mArrSCH[aSegID];

    IDE_TEST( removeChunck( (SChar*)sCurSCH->mChunkPtr,
                            sCurSCH->mShmID )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( idlOS::shmdt( sCurSCH->mChunkPtr ) < 0, err_shmdt );

    IDE_TEST( mSCHPool.memfree( sCurSCH ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmdt );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_SysShmDt) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::detachChunk( SChar * aChunkPtr )
{
    IDE_TEST_RAISE( idlOS::shmdt( aChunkPtr ) < 0, err_shmdt );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmdt );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_SysShmDt) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::removeChunck( SChar     * aChunkPtr,
                                     PDL_HANDLE  aShmID )
{
#if defined(INTEL_LINUX) || defined(ALPHA_LINUX) || defined(NTO_QNX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(AMD64_LINUX) || defined(X86_64_LINUX) || defined(XEON_LINUX)
    if( iduProperty::getShmLock() == 1 )
    {
        (void)idlOS::shmctl( aShmID,
                             SHM_UNLOCK,
                             NULL);
    }
#endif

    IDE_TEST_RAISE( idlOS::shmctl( aShmID, IPC_RMID, NULL) == -1,
                    err_shmctl );
    IDE_TEST_RAISE( idlOS::shmdt( aChunkPtr ) < 0, err_shmdt );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_shmdt );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_SysShmDt ) );
    }
    IDE_EXCEPTION( err_shmctl );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SHM_CTL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmChunkMgr::checkExist( key_t     aShmKey,
                                   idBool&   aExist )
{
    iduShmSSegment * sSSegHeader;
    SInt             sFlag;
    PDL_HANDLE       sShmID;

#if !defined (NTO_QNX) && !defined (VC_WIN32)
    sFlag       = 0666; /*|IPC_CREAT|IPC_EXCL*/
#elif defined (USE_WIN32IPC_DAEMON)
    sFlag       = 0666;
#else
    sFlag       = IPC_CREAT | 0666;
#endif
    aExist = ID_FALSE;

    sShmID = idlOS::shmget( aShmKey, ID_SIZEOF(iduShmSSegment), sFlag );

    if( sShmID == PDL_INVALID_HANDLE )
    {
        switch(errno)
        {
        case ENOENT:
            break;
        case EACCES:
            IDE_RAISE( no_permission_error );
        default:
            IDE_RAISE( shmget_error );
        }
    }
    else
    {
        sSSegHeader = (iduShmSSegment *)idlOS::shmat(sShmID, 0, sFlag);
        IDE_TEST_RAISE(sSSegHeader == (iduShmSSegment *)-1, shmat_error);
#if defined(INTEL_LINUX) || defined(ALPHA_LINUX) || defined(NTO_QNX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(AMD64_LINUX) || defined(X86_64_LINUX) || defined(XEON_LINUX)

        if( iduProperty::getShmLock() == 1 )
        {
            (void)idlOS::shmctl( sShmID,
                                 SHM_UNLOCK,
                                 NULL);
        }
#endif

#if defined (NTO_QNX) || (defined (VC_WIN32) && !defined (USE_WIN32IPC_DAEMON))

        IDE_TEST_RAISE(idlOS::shmctl(sShmID, IPC_RMID, NULL) < 0, shmdt_error);
#endif

        IDE_TEST_RAISE(idlOS::shmdt( (char*)sSSegHeader ) < 0, shmdt_error);

#if !defined (NTO_QNX) && !defined (VC_WIN32)
        aExist = ID_TRUE;
#elif defined (USE_WIN32IPC_DAEMON)
        aExist = ID_TRUE;
#else
        aExist = ID_FALSE;
#endif
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(shmget_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_SysShmGet));
    }
    IDE_EXCEPTION(shmat_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idc_SHM_ATTACH));
    }
    IDE_EXCEPTION(no_permission_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_Shm_No_Permission));
    }
    IDE_EXCEPTION(shmdt_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_SysShmDt));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
