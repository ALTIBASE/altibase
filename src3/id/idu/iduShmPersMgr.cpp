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
#include <iduVersionDef.h>
#include <idErrorCode.h>
#include <iduShmDef.h>
#include <idwPMMgr.h>
#include <iduShmPersMgr.h>
#include <idrLogMgr.h>
#include <iduShmKeyFile.h>
#include <iduShmPersMgr.h>
#include <iduVLogShmPersPool.h>
#include <iduVLogShmPersList.h>

#if defined(ALTIBASE_PRODUCT_HDB)
# define IDU_SHM_SYS_SIGNATURE_STRING   "Altibase HDB version %s System chunk"
# define IDU_SHM_DAT_SIGNATURE_STRING   "Altibase HDB version %s Data chunk"
#elif defined(ALTIBASE_PRODUCT_XDB)
# define IDU_SHM_SYS_SIGNATURE_STRING   "Altibase XDB version %s System chunk"
# define IDU_SHM_DAT_SIGNATURE_STRING   "Altibase XDB version %s Data chunk"
#else
# error Unknown product!
#endif

idrUndoFunc iduShmPersMgr::mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_MAX];
SChar       iduShmPersMgr::mStrVLogType[IDU_VLOG_TYPE_SHM_PERS_MGR_MAX+1][100] =
{
    "IDU_VLOG_TYPE_SHM_PERS_MGR_NONE",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_ALLOC_MEM",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_CREATE_DATA_CHUNK",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_ATTACH_CHUNKS",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_INSERT_FREE_BLOCK",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_REMOVE_BLOCK",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_SPLIT_BLOCK",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_LEFT_BLOCK",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_RIGHT_BLOCK",
    "IDU_VLOG_TYPE_SHM_PERS_MGR_MAX"
};

SChar               iduShmPersMgr::mSysSignature[IDU_SHM_CHUNK_SIGSIZE];
SChar               iduShmPersMgr::mDatSignature[IDU_SHM_CHUNK_SIGSIZE];
iduShmPersSys*      iduShmPersMgr::mSChunk = NULL;
key_t               iduShmPersMgr::mSKey;
SInt                iduShmPersMgr::mSID;
#if 0
SInt                iduShmPersMgr::mSemID;
#endif
ULong               iduShmPersMgr::mChunkSize;
SInt                iduShmPersMgr::mIDs[IDU_SHM_MAX_CHUNKS];
iduShmPersChunk*    iduShmPersMgr::mAddresses[IDU_SHM_MAX_CHUNKS];

void* iduShmPersMgr::getPtrOfAddr(idShmAddr aAddr)
{
    SInt    sID;
    UInt    sOffset;
    SChar*  sPtr;

    if(aAddr == 0)
    {
        sPtr = NULL;
    }
    else
    {
        IDE_DASSERT((aAddr & IDU_SHM_PERS_ADDR) != 0);
        sPtr = NULL;

        if( aAddr != IDU_SHM_NULL_ADDR )
        {
            sID        = (SInt)((aAddr >> 32) & 0x7FFFFFFF);
            sOffset    = (UInt)(aAddr & 0xFFFFFFFF);

            if( sID >= mSChunk->mChunkCount )
            {
                ideLog::log( IDE_SERVER_0,
                             "iduShmPersMgr::getPtrOfAddr\n"
                             "aAddr                 : %"ID_UINT64_FMT"\n"
                             "Chunk ID & Offset     : "
                             "%"ID_UINT64_FMT",%"ID_UINT64_FMT"\n"
                             "Chunk Count %"ID_UINT64_FMT"\n"
                             "Max Chunk Count : %"ID_UINT64_FMT"\n",
                             aAddr,
                             sID,
                             sOffset,
                             mSChunk->mChunkCount,
                             IDU_SHM_MAX_CHUNKS );

                IDE_ASSERT(0);
            }
            else
            {
                sPtr  = (SChar*)mAddresses[sID];
                if(sPtr == NULL)
                {
                    IDE_TEST(attachOneChunk(sID) != IDE_SUCCESS);
                    IDE_TEST_RAISE(sOffset >= mAddresses[sID]->mSize, EINVALACCESS);
                    sPtr  = (SChar*)mAddresses[sID];
                }
                else
                {
                    /* fall through */
                }

                sPtr += sOffset;
            }
        }
    }

    return (void*)sPtr;

    IDE_EXCEPTION(EINVALACCESS)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERS_INVALID_ADDRESS,
                                sID, sOffset,
                                mSChunk->mChunkCount, mAddresses[sID]->mSize)
                );
    }

    IDE_EXCEPTION_END;
    IDE_ASSERT(0);
    return NULL;
}

iduShmPtr* iduShmPersMgr::getShmPtrOfAddr(idShmAddr aAddr)
{
    return (iduShmPtr*)getPtrOfAddr(aAddr);
}

idShmAddr iduShmPersMgr::getAddrOfPtr(void* aPtr)
{
    iduShmPtr* sPtr = (iduShmPtr*)aPtr - 1;
    IDE_ASSERT(sPtr->mIndex     < IDU_MEM_UPPERLIMIT);
    IDE_ASSERT(sPtr->mOffset    < mAddresses[sPtr->mID]->mSize);

    return idShmAddr(*sPtr) + sizeof(iduShmPtr);
}

IDE_RC iduShmPersMgr::initialize(idvSQL* aSQL, iduShmTxInfo* aTxInfo, iduShmProcType aProcType)
{
    idBool  sIsValid;
    idrSVP  sSavepoint;

    /* Set undo functions */
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_NONE]               = NULL;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_ALLOC_MEM]          = undoAlloc;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_CREATE_DATA_CHUNK]  = undoCreateDataChunk;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_ATTACH_CHUNKS]      = undoAttachChunks;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_INSERT_FREE_BLOCK]  = undoInsertFreeBlock;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_REMOVE_BLOCK]       = undoRemoveBlock;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_SPLIT_BLOCK]        = undoSplitBlock;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_LEFT_BLOCK]   = undoMergeLeftBlock;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_RIGHT_BLOCK]  = undoMergeRightBlock;

    idlOS::memset(mSysSignature, 0, IDU_SHM_CHUNK_SIGSIZE);
    idlOS::memset(mDatSignature, 0, IDU_SHM_CHUNK_SIGSIZE);
    idlOS::snprintf(mSysSignature, IDU_SHM_CHUNK_SIGSIZE,
                    IDU_SHM_SYS_SIGNATURE_STRING,
                    IDU_ALTIBASE_VERSION_STRING);
    idlOS::snprintf(mDatSignature, IDU_SHM_CHUNK_SIGSIZE,
                    IDU_SHM_DAT_SIGNATURE_STRING,
                    IDU_ALTIBASE_VERSION_STRING);

    mSKey       = iduProperty::getShmPersKey();
    mChunkSize  = iduProperty::getShmChunkSize();

    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

    idlOS::memset(mIDs,         0, sizeof(SInt)             * IDU_SHM_MAX_CHUNKS);
    idlOS::memset(mAddresses,   0, sizeof(iduShmPersChunk*) * IDU_SHM_MAX_CHUNKS);

    if(aProcType == IDU_PROC_TYPE_DAEMON)
    {
        if(getSysChunk(aSQL, aTxInfo) == IDE_SUCCESS)
        {
            IDE_TEST_RAISE(mSChunk->mUID != idlOS::getuid(), ENOTMINE);

            if(iduProperty::getShmMemoryPolicy() == 0)
            {
                (void)attachChunks(aSQL, aTxInfo);
                (void)cleanupChunks();
                sIsValid = ID_FALSE;
            }
            else
            {
                sIsValid = checkSysValid();
                if(sIsValid == ID_TRUE)
                {
                    if(mSChunk->mStatus != IDU_SHM_STATUS_DETACHED)
                    {
                        /*
                         * Not in detached status
                         * Previous shutdown was abnormal
                         */
                        IDE_TEST(cleanupChunks() != IDE_SUCCESS);
                        sIsValid = ID_FALSE;
                    }
                    else
                    {
                        if(attachChunks(aSQL, aTxInfo) != IDE_SUCCESS)
                        {
                            sIsValid = ID_FALSE;
                        }
                        else
                        {
                            sIsValid = ID_TRUE;
                            mSChunk->mStatus = IDU_SHM_STATUS_RUNNING;
                        }
                    }
                }
                else
                {
                    IDE_TEST(cleanupChunks() != IDE_SUCCESS);
                    sIsValid = ID_FALSE;
                }
            }
        }
        else
        {
            sIsValid = ID_FALSE;
        }

        if(sIsValid == ID_FALSE)
        {
            IDE_TEST(createSysChunk(aSQL, aTxInfo) != IDE_SUCCESS);
        }
        else
        {
            /* fall through */
        }
    }
    else
    {
        /* Only daemon can cleanup and re-create shm chunks */
        IDE_TEST(getSysChunk(aSQL, aTxInfo) != IDE_SUCCESS);
        IDE_TEST(mSChunk->mStatus != IDU_SHM_STATUS_RUNNING);
        IDE_TEST(attachChunks(aSQL, aTxInfo) != IDE_SUCCESS);
    }

    IDE_TEST(iduVLogShmPersList::initialize() != IDE_SUCCESS);
    IDE_TEST(iduVLogShmPersPool::initialize() != IDE_SUCCESS);

    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ENOTMINE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERSSYS_NOT_MINE));
    }

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::destroy(void)
{
    if((iduGetShmProcType() == IDU_PROC_TYPE_DAEMON) &&
       (iduProperty::getShmMemoryPolicy() == 0))
    {
        /* 
         * In SM code, with SHM_POLICY=0, frees all persistent blocks
         * It is no use to preserve persistent chunks when SHM_POLICY=0
         * Only daemon can cleanup chunks on destroy
         */
        IDE_TEST(cleanupChunks() != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(detachChunks() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::attachChunks(idvSQL* aSQL, iduShmTxInfo* aTxInfo)
{
    SInt    i;
    idrSVP  sSavepoint;
    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

    IDE_DASSERT(mSChunk != NULL);

    IDE_TEST(writeAttachChunks(aTxInfo) != IDE_SUCCESS);

    for(i = 1; i < mSChunk->mChunkCount; i++)
    {
        IDE_TEST(attachOneChunk(i) != IDE_SUCCESS);
    }

    for(; i < IDU_SHM_MAX_CHUNKS; i++)
    {
        mAddresses[i] = NULL;
    }

    mSChunk->mLastKey = mSChunk->mKeys[mSChunk->mChunkCount - 1];
    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::attachOneChunk(UInt aChunkID)
{
    SInt    i = aChunkID;
    idBool  sIsValid;
    SInt    sFlag = 0666;

    IDE_DASSERT(i < IDU_SHM_MAX_CHUNKS);

    mIDs[i] = idlOS::shmget(mSChunk->mKeys[i], mSChunk->mSizes[i], sFlag);
    IDE_TEST_RAISE(mIDs[i] == -1, ESHMGETFAILED);

    mAddresses[i] = (iduShmPersChunk*)idlOS::shmat(mIDs[i], NULL, sFlag);
    IDE_TEST_RAISE(mAddresses[i] == MAP_FAILED, EATTACHFAILED);

    sIsValid = checkShmValid(i);
    IDE_TEST_RAISE(sIsValid == ID_FALSE, EINVALIDSHM);
    IDE_TEST_RAISE(mAddresses[i]->mID != i, EINVALIDSHM);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ESHMGETFAILED)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERS_ATTACH_FAILED,
                                aChunkID, mSChunk->mKeys[i], mIDs[i], errno));
    }
    IDE_EXCEPTION(EATTACHFAILED)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERS_ATTACH_FAILED,
                                aChunkID, mSChunk->mKeys[i], mIDs[i], errno));
    }
    IDE_EXCEPTION(EINVALIDSHM)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERS_INVALID_CHUNK, aChunkID));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::detachChunks(void)
{
    SInt i;

    if(mSChunk != NULL)
    {
        for(i = mSChunk->mChunkCount - 1; i > 0; i--)
        {
            (void)idlOS::shmdt(mAddresses[i]);
        }
    }

    if(iduGetShmProcType() == IDU_PROC_TYPE_DAEMON)
    {
        mSChunk->mStatus        = IDU_SHM_STATUS_DETACHED;
    }

    (void)idlOS::shmdt(mAddresses[0]);

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::clearSysChunk(void)
{
    if(mSChunk != NULL)
    {
        (void)idlOS::shmdt(mSChunk);
    }
    else
    {
        /* fall through */
    }

    if((mSID != 0) && (mSID != -1))
    {
        IDE_TEST(idlOS::shmctl(mSID, IPC_RMID, NULL) == -1);
    }
    else
    {
        /*
         * Shared memory not attached
         * fall through
         */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::cleanupChunks(void)
{
    SInt i;
    if(mSChunk != NULL)
    {
        ideLog::log( IDE_SERVER_0,
                     "Cleanup persistent memory manager chunks.");

        for(i = 1; i < IDU_SHM_MAX_CHUNKS && mSChunk->mKeys[i] != 0; i++)
        {
            if(mAddresses[i] != NULL)
            {
                (void)idlOS::shmdt(mAddresses[i]);
            }
            else
            {
                /* fall through  */
            }
            (void)idlOS::shmctl(mIDs[i], IPC_RMID, NULL);
        }
    }
    else
    {
        /*
         * System chunk not attached
         * fall through
         */
    }

    IDE_TEST(clearSysChunk() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_ABORT_PERS_CLEANUP_FAILED));
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::alloc(idvSQL*                 aSQL,
                            iduShmTxInfo*           aTxInfo,
                            iduMemoryClientIndex    aIndex,
                            UInt                    aSize,
                            void**                  aPtr)
{
    iduShmPtr*  sPtr;
    iduShmPtr*  sNewRight;
    idrSVP      sSavepoint;

    SInt        sFS;
    SInt        sLS;

    aSize = idlOS::align(aSize + sizeof(iduShmPtr), 64);

    sFS = acpBitFfs32(aSize);
    sLS = acpBitFls32(aSize);

    if(sFS != sLS)
    {
        aSize = 1 << (sLS +1);
    }
    else
    {
        /* fall through */
    }

    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);
    IDE_TEST(iduShmLatchAcquire(aSQL, aTxInfo, &mSChunk->mLatch) != IDE_SUCCESS);

    IDE_TEST(removeFreeBlock(aSQL, aTxInfo, aSize, sPtr) != IDE_SUCCESS);

    if(sPtr == NULL)
    {
        IDE_TEST(createDataChunk(aSQL, aTxInfo, aSize, sPtr) != IDE_SUCCESS);
    }
    else
    {
        /* fall through */
    }

    IDE_TEST(splitBlock(aSQL, aTxInfo, sPtr, aSize, sNewRight) != IDE_SUCCESS);

    sPtr->setUsed();
    sPtr->mIndex = aIndex;

    IDE_TEST(writeAlloc(aTxInfo, sPtr->getAddr()) != IDE_SUCCESS);

    if(sNewRight == NULL)
    {
        sNewRight = sPtr->getRight();
        sNewRight->setPrevUsed();
    }
    else
    {
        sNewRight->setUsed(ID_FALSE);
        sNewRight->setPrevUsed();
        sNewRight->mIndex = IDU_MEM_RESERVED;
        IDE_TEST(insertFreeBlock(aSQL, aTxInfo, sNewRight) != IDE_SUCCESS);
    }

    IDE_ASSERT(sPtr->mID        < mSChunk->mChunkCount);
    IDE_ASSERT(sPtr->mOffset    < mAddresses[sPtr->mID]->mSize);

    *aPtr = (void*)(sPtr + 1);
    
    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::free(idvSQL*                  aSQL,
                           iduShmTxInfo*            aTxInfo,
                           void*                    aPtr)
{
    iduShmPtr* sPtr = (iduShmPtr*)aPtr - 1;

    iduShmPtr* sNewPtr;
    iduShmPtr* sNewRight;

    idrSVP      sSavepoint;

    iduMemoryClientIndex sIndex;
    SLong                sSize;

    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

    sIndex = sPtr->mIndex;
    sSize  = (SLong)sPtr->getSize();

    IDE_TEST(mergeBlock(aSQL, aTxInfo, sPtr, sNewPtr, sNewRight) != IDE_SUCCESS);
    sNewPtr->setUsed(ID_FALSE);
    sNewPtr->mIndex = IDU_MEM_RESERVED;
    IDE_TEST(insertFreeBlock(aSQL, aTxInfo, sNewPtr) != IDE_SUCCESS);
    
    sNewRight->setPrevUsed(ID_FALSE);

    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::reserve(idvSQL*               aSQL,
                              iduShmTxInfo*         aTxInfo,
                              iduShmMetaBlockType   aType,
                              iduMemoryClientIndex  aIndex,
                              UInt                  aSize,
                              void**                aPtr)
{
    IDE_ASSERT(aType < IDU_SHM_META_MAX_COUNT);
    if(mSChunk->mReserved[aType] == IDU_SHM_NULL_ADDR)
    {
        IDE_TEST(alloc(aSQL, aTxInfo, aIndex, aSize, aPtr) != IDE_SUCCESS);
        mSChunk->mReserved[aType] = getAddrOfPtr(*aPtr);
    }
    else
    {
        *aPtr = getPtrOfAddr(mSChunk->mReserved[aType]);
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC iduShmPersMgr::unreserve(idvSQL*                 aSQL,
                                iduShmTxInfo*           aTxInfo,
                                iduShmMetaBlockType     aType)
{
    idShmAddr   sAddr   = mSChunk->mReserved[aType];
    void*       sPtr;

    IDE_ASSERT(aType < IDU_SHM_META_MAX_COUNT);

    if(sAddr != IDU_SHM_NULL_ADDR)
    {
        sPtr = getPtrOfAddr(sAddr);
        IDE_TEST(free(aSQL, aTxInfo, sPtr) != IDE_SUCCESS);
        mSChunk->mReserved[aType] = IDU_SHM_NULL_ADDR;
    }
    else
    {
        /* fall through */
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void* iduShmPersMgr::getReserved(iduShmMetaBlockType   aIndex)
{
    IDE_ASSERT(aIndex < IDU_SHM_META_MAX_COUNT);
    return getPtrOfAddr(mSChunk->mReserved[aIndex]);
}


IDE_RC iduShmPersMgr::createSysChunk(idvSQL*               aSQL,
                                     iduShmTxInfo*         aTxInfo)
{
    SInt        i;
    SInt        sFlag = IPC_CREAT | IPC_EXCL | 0666;
    iduShmPtr*  sBlock;
    idShmAddr   sLatchAddr;

    idrSVP      sSavepoint;

    mSID = idlOS::shmget(mSKey, sizeof(iduShmPersSys), sFlag);
    IDE_TEST_RAISE(mSID == -1, ECRTFAILED);

    mSChunk = (iduShmPersSys*)idlOS::shmat(mSID, NULL, sFlag);
    IDE_TEST_RAISE(mSChunk == MAP_FAILED, EMAPFAILED);

    mSChunk->mKey           = mSKey;
    mSChunk->mID            = 0;
    mSChunk->mOffset        = 0;
    mSChunk->mLeftSize      = 0;
    mSChunk->mSize          = sizeof(iduShmPersSys);
    mSChunk->mIndex         = IDU_MEM_ID_CHUNK;
    mSChunk->mStatus        = IDU_SHM_STATUS_INIT;
    mSChunk->setUsed();
    mSChunk->setPrevUsed();

    mSChunk->mUID           = idlOS::getuid();
    mSChunk->mChunkCount    = 1;
    idlOS::memcpy(mSChunk->mSignature, mSysSignature, IDU_SHM_CHUNK_SIGSIZE);

    mSChunk->mKeys[0] = mSKey;
    mIDs[0] = mSID;
    mAddresses[0] = mSChunk;

    for(i = 1; i < mSChunk->mChunkCount; i++)
    {
        mSChunk->mKeys[i] = 0;
    }

    mSChunk->mLastKey = mSKey + 1;

    for(i = 0; i < IDU_SHM_META_MAX_COUNT; i++)
    {
        mSChunk->mReserved[i] = IDU_SHM_NULL_ADDR;
    }

    mSChunk->mFL = 0;
    for(i = 0; i < 25; i++)
    {
        mSChunk->mFrees[i] = IDU_SHM_NULL_ADDR;
    }

    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);
    /* Initialize latch */
    sLatchAddr = IDU_SHM_MAKE_PERS_ADDR(0, sizeof(iduShmPersChunk));
    iduShmLatchInit(sLatchAddr,
                    IDU_SHM_LATCH_SPIN_COUNT_DEFAULT,
                    &mSChunk->mLatch);

    /* Trailer Sentinel */
    sBlock = &(mSChunk->mSentinel);
    sBlock->mOffset     = mSChunk->mSize - sizeof(iduShmPtr);
    sBlock->mSize       = sizeof(iduShmPtr);
    sBlock->mLeftSize   = mSChunk->mSize - sizeof(iduShmPtr);
    sBlock->mID         = 0;
    sBlock->mIndex      = IDU_MEM_SENTINEL;
    sBlock->mPrev       = sBlock->getAddr();
    sBlock->mNext       = sBlock->getAddr();
    sBlock->setUsed();
    sBlock->setPrevUsed(ID_TRUE);

    mSChunk->mStatus        = IDU_SHM_STATUS_RUNNING;

    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ECRTFAILED)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERSSYS_CREATE_FAILED));
    }

    IDE_EXCEPTION(EMAPFAILED)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERSSYS_MAP_FAILED));
    }

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::createDataChunk(idvSQL*               aSQL,
                                      iduShmTxInfo*         aTxInfo,
                                      SInt                  aSize,
                                      iduShmPtr*&           aNewBlock)
{
    SInt                sID;
    key_t               sKey;
    SInt                sChunkIndex;
    SInt                sFlag = IPC_CREAT | IPC_EXCL | 0666;
    SInt                sSize;
    SInt                sPageSize;
    iduShmPersData*     sChunk;
    iduShmPtr*          sBlock; 
    idrSVP              sSavepoint;

    /* Code semaphore locking */

    IDE_DASSERT(mSChunk != NULL);

    if(aSize < mChunkSize)
    {
        sSize = mChunkSize;
    }
    else
    {
        sPageSize = idlOS::getpagesize();
        sSize = idlOS::align(aSize + sPageSize, sPageSize);
    }

    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

    sKey = mSChunk->mLastKey;
    sID = idlOS::shmget(sKey, sSize, sFlag);
    if(sID != -1)
    {
        sChunk = (iduShmPersData*)idlOS::shmat(sID, NULL, sFlag);
    }
    else
    {
        sChunk = (iduShmPersData*)MAP_FAILED;
    }

    while((sID == -1) || (sChunk == MAP_FAILED))
    {
        IDE_TEST_RAISE(errno == ENOMEM, EMEMFULL);
        IDE_TEST_RAISE(errno == ENFILE, EMEMFULL);
        IDE_TEST_RAISE(errno == EMFILE, EMEMFULL);

        sKey++;
        sID = idlOS::shmget(sKey, sSize, sFlag);
    
        if(sID != -1)
        {
            sChunk = (iduShmPersData*)idlOS::shmat(sID, NULL, sFlag);
        }
        else
        {
            /* continue */
        }
    }

    if(mSChunk->mUID != idlOS::getuid())
    {
        struct shmid_ds sStat;
        IDE_TEST_RAISE(idlOS::shmctl( sID, IPC_STAT, &sStat ) == -1, err_shmctl );
        IDE_ASSERT(sStat.shm_perm.uid != mSChunk->mUID);

        sStat.shm_perm.uid = mSChunk->mUID;

        IDE_TEST_RAISE( idlOS::shmctl( sID, IPC_SET, &sStat ) == -1, err_shmctl );
    }
    else
    {
        /* fall through */
    }

    sChunkIndex = mSChunk->mChunkCount;

    sChunk->mKey                = sKey;
    sChunk->mID                 = sChunkIndex;
    sChunk->mOffset             = 0;
    sChunk->mSize               = sSize;
    sChunk->mIndex              = IDU_MEM_ID_CHUNK;
    sChunk->setUsed();
    sChunk->setPrevUsed();

    /* Trailer Sentinel */
    sBlock = (iduShmPtr*)((SChar*)sChunk + sSize - sizeof(iduShmPtr));
    sBlock->mOffset     = sSize - sizeof(iduShmPtr);
    sBlock->mSize       = sizeof(iduShmPtr);
    sBlock->mLeftSize   = sSize - sizeof(iduShmPersData) + sizeof(iduShmPtr);
    sBlock->mID         = 0;
    sBlock->mIndex      = IDU_MEM_SENTINEL;
    sBlock->mPrev       = sBlock->getAddr();
    sBlock->mNext       = sBlock->getAddr();
    sBlock->setUsed();
    sBlock->setPrevUsed(ID_FALSE);

    /* Empty */
    sBlock = &(sChunk->mFirstBlock);
    sBlock->mOffset     = sizeof(iduShmPersData) - sizeof(iduShmPtr);
    sBlock->mSize       = sSize - sBlock->mOffset - sizeof(iduShmPtr);
    sBlock->mLeftSize   = sizeof(iduShmPersData) - sizeof(iduShmPtr);
    sBlock->mID         = sChunkIndex;
    sBlock->mIndex      = IDU_MEM_RESERVED;
    sBlock->mPrev       = sBlock->getAddr();
    sBlock->mNext       = sBlock->getAddr();
    sBlock->setUsed(ID_FALSE);
    sBlock->setPrevUsed();

    idlOS::memcpy(sChunk->mSignature, mDatSignature, IDU_SHM_CHUNK_SIGSIZE);

    aNewBlock = sBlock;

    IDE_TEST(writeCreateDataChunk(aTxInfo,
                                  (SLong)sChunkIndex,
                                  (SLong)sKey)
             != IDE_SUCCESS);
    mSChunk->mKeys[sChunkIndex]     = sKey;
    mSChunk->mSizes[sChunkIndex]    = sSize;
    mIDs[sChunkIndex]               = sID;
    mAddresses[sChunkIndex]         = sChunk;

    mSChunk->mChunkCount++;
    mSChunk->mLastKey = sKey + 1;

    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION(EMEMFULL)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERS_SHM_FULL));
    }

    IDE_EXCEPTION( err_shmctl );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_CHANGE_OWNER_OF_SHARED_MEMORY_SEGMENT ) );
        IDE_ASSERT(idlOS::shmdt(sChunk) == 0);
    }

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::getSysChunk(idvSQL*               aSQL,
                                  iduShmTxInfo*         aTxInfo)
{
    SInt sFlag = 0666;

    mSChunk = NULL;
    mSID = idlOS::shmget(mSKey, sizeof(iduShmPersSys), sFlag);
    IDE_TEST_RAISE(mSID == -1, EMAPFAILED);

    mSChunk = (iduShmPersSys*)idlOS::shmat(mSID, NULL, sFlag);
    IDE_TEST_RAISE(mSChunk == MAP_FAILED, EMAPFAILED);

    mAddresses[0]       = mSChunk;
    mSChunk->mKeys[0]   = mSKey;
    mSChunk->mSizes[0]  = sizeof(iduShmPersSys);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EMAPFAILED)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_PERSSYS_MAP_FAILED));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::insertFreeBlock(idvSQL*               aSQL,
                                      iduShmTxInfo*         aTxInfo,
                                      iduShmPtr*            aPtr)
{
    SInt        sFL;
    idShmAddr   sPrev;
    iduShmPtr*  sPtr;
    iduShmPtr*  sNext;

    idrSVP      sSavepoint;

    IDE_ASSERT(aPtr->getUsed() == ID_FALSE);

    sFL = calcFL(aPtr->mSize);
    sPrev = mSChunk->mFrees[sFL];

    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

    IDE_TEST(writeInsertFreeBlock(aTxInfo, sFL, sPrev)
             != IDE_SUCCESS);

    if(sPrev == IDU_SHM_NULL_ADDR)
    {
        aPtr->mPrev = aPtr->mNext = aPtr->getAddr();
        mSChunk->mFrees[sFL] = aPtr->getAddr();
        mSChunk->mFL |= 1 << sFL;
    }
    else
    {
        sPtr    = getShmPtrOfAddr(sPrev);
        sNext   = getShmPtrOfAddr(sPtr->mNext);

        aPtr->mPrev = sPrev;
        aPtr->mNext = sPtr->mNext;

        sPtr->mNext = *aPtr;
        sNext->mPrev = *aPtr;
    }

    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::removeFreeBlock(idvSQL*               aSQL,
                                      iduShmTxInfo*         aTxInfo,
                                      UInt                  aSize,
                                      iduShmPtr*&           aPtr)
{
    SInt        sFL;
    SInt        sMSB = 0xFFFFFFFF;
    idShmAddr   sNewBlock;

    iduShmPtr*  sPtr;

    sFL  = calcFL(aSize);
    sMSB = 0xFFFFFFFF << sFL;
    sMSB = mSChunk->mFL & sMSB;

    if(sMSB == 0)
    {
        sPtr = NULL;
    }
    else
    {
        sFL  = acpBitFfs32(sMSB);
        sNewBlock = mSChunk->mFrees[sFL];
        IDE_DASSERT(sNewBlock != IDU_SHM_NULL_ADDR);
        sPtr = getShmPtrOfAddr(sNewBlock);
        IDE_ASSERT(sPtr->getUsed() == ID_FALSE);

        if(sPtr->mSize < aSize)
        {
            sFL++;
            sMSB = 0xFFFFFFFF << sFL;
            sMSB = mSChunk->mFL & sMSB;

            if(sMSB == 0)
            {
                sPtr = NULL;
            }
            else
            {
                sFL  = acpBitFfs32(sMSB);
                sNewBlock = mSChunk->mFrees[sFL];
                IDE_DASSERT(sNewBlock != IDU_SHM_NULL_ADDR);
                sPtr = getShmPtrOfAddr(sNewBlock);
                IDE_ASSERT(sPtr->getUsed() == ID_FALSE);
            }
        }
    }

    if(sPtr != NULL)
    {
        IDE_TEST(removeBlock(aSQL, aTxInfo, sPtr) != IDE_SUCCESS);
        aPtr = sPtr;
    }
    else
    {
        aPtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::removeBlock(idvSQL*               aSQL,
                                  iduShmTxInfo*         aTxInfo,
                                  iduShmPtr*            aPtr)
{
    iduShmPtr*  sPrev;
    iduShmPtr*  sNext;
    SInt        sFL;
    idrSVP      sSavepoint;

    idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

    sPrev = getShmPtrOfAddr(aPtr->mPrev);
    sNext = getShmPtrOfAddr(aPtr->mNext);

    sFL = calcFL(aPtr->mSize);
    if(sPrev == aPtr)
    {
        IDE_TEST(writeRemoveBlock(aTxInfo, sFL,
                                  aPtr->getAddr(),
                                  IDU_SHM_NULL_ADDR,
                                  IDU_SHM_NULL_ADDR)
                 != IDE_SUCCESS);

        /* This free list is now empty */
        IDE_ASSERT(sNext == aPtr);
        mSChunk->mFrees[sFL] = IDU_SHM_NULL_ADDR;
        mSChunk->mFL &= ~(1 << sFL);
    }
    else
    {
        IDE_TEST(writeRemoveBlock(aTxInfo, -1,
                                  aPtr->getAddr(),
                                  aPtr->mPrev, aPtr->mNext)
                 != IDE_SUCCESS);

        if(mSChunk->mFrees[sFL] == aPtr->getAddr())
        {
            mSChunk->mFrees[sFL] = aPtr->mNext;
        }
        else
        {
            /* fall through */
        }
        sPrev->mNext = aPtr->mNext;
        sNext->mPrev = aPtr->mPrev;
    }

    IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::splitBlock(idvSQL*            aSQL,
                                 iduShmTxInfo*      aTxInfo,
                                 iduShmPtr*         aBlock,
                                 UInt               aSize,
                                 iduShmPtr*&        aNewBlock)
{
    UInt sSize = aBlock->mSize;
    UInt sNewSize = idlOS::align(aSize, 64);
    iduShmPtr* sRight;

    idrSVP      sSavepoint;


    IDE_ASSERT_MSG(sSize % 64 == 0, "Block size should be a multiple of 64 but %d", sSize);
    IDE_ASSERT_MSG(sSize >= aSize , "Block size should be larger than allocating size !(%d >= %d)", aSize, sSize);

    if(sSize < sNewSize + 128)
    {
        /*
         * Do not split when new block is smaller than 128byte
         * = least allocation size
         */
        aNewBlock = NULL;
    }
    else
    {
        sRight = aBlock->getRight();

        idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

        IDE_TEST(writeSplitBlock(aTxInfo, aBlock->getAddr(), sSize, sRight->getAddr())
                 != IDE_SUCCESS);
        aNewBlock = (iduShmPtr*)((SChar*)aBlock + sNewSize);

        aBlock->mSize           = sNewSize;
        aNewBlock->mSize        = sSize - sNewSize;
        aNewBlock->mLeftSize    = aBlock->mSize;
        aNewBlock->mOffset      = aBlock->mOffset + sNewSize;
        aNewBlock->mID          = aBlock->mID;

        sRight->mLeftSize       = aNewBlock->mSize;
    
        IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::mergeBlock(idvSQL*            aSQL,
                                 iduShmTxInfo*      aTxInfo,
                                 iduShmPtr*         aPtr,
                                 iduShmPtr*&        aNewPtr,
                                 iduShmPtr*&        aNewRight)
{
    IDE_TEST(mergeLeft(aSQL, aTxInfo, aPtr, aNewPtr) != IDE_SUCCESS);
    IDE_TEST(mergeRight(aSQL, aTxInfo, aNewPtr, aNewRight) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::mergeLeft(idvSQL*             aSQL,
                                iduShmTxInfo*       aTxInfo,
                                iduShmPtr*          aPtr,
                                iduShmPtr*&         aNewPtr)
{
    iduShmPtr* sRight = aPtr->getRight();
    idrSVP     sSavepoint;

    if(aPtr->getPrevUsed() == ID_TRUE)
    {
        aNewPtr = aPtr;
    }
    else
    {
        idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

        aNewPtr = aPtr->getLeft();

        IDE_TEST(writeMergeLeftBlock(aTxInfo,
                                     aPtr->getAddr(),
                                     aPtr->getSize(),
                                     aNewPtr->getAddr(),
                                     aNewPtr->getSize(),
                                     sRight->getAddr()) != IDE_SUCCESS);

        IDE_TEST(removeBlock(aSQL, aTxInfo, aNewPtr) != IDE_SUCCESS);
        aNewPtr->mSize += aPtr->mSize;
        sRight->mLeftSize = aNewPtr->mSize;
    
        IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduShmPersMgr::mergeRight(idvSQL*            aSQL,
                                 iduShmTxInfo*      aTxInfo,
                                 iduShmPtr*         aPtr,
                                 iduShmPtr*&        aNewRight)
{
    iduShmPtr* sRight = aPtr->getRight();
    idrSVP     sSavepoint;

    if(sRight->getUsed() == ID_TRUE)
    {
        aNewRight = sRight;
    }
    else
    {
        iduShmPtr* sNewRight = sRight->getRight();

        idrLogMgr::setSavepoint(aTxInfo, &sSavepoint);

        IDE_TEST(writeMergeRightBlock(aTxInfo,
                                      aPtr->getAddr(),
                                      aPtr->getSize(),
                                      sRight->getAddr(),
                                      sRight->getSize(),
                                      sNewRight->getAddr()) != IDE_SUCCESS);

        IDE_TEST(removeBlock(aSQL, aTxInfo, sRight) != IDE_SUCCESS);
        aPtr->mSize += sRight->mSize;
        sRight = aPtr->getRight();
        sRight->mLeftSize = aPtr->mSize;

        aNewRight = sNewRight;
    
        IDE_TEST(idrLogMgr::commit2Svp(aSQL, aTxInfo, &sSavepoint) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT(idrLogMgr::abort2Svp(aSQL, aTxInfo, &sSavepoint) == IDE_SUCCESS);
    return IDE_FAILURE;
}

idBool iduShmPersMgr::checkSysValid(void)
{
    SInt i;
    IDE_TEST(checkShmValid(0) != ID_TRUE);
    IDE_TEST(mSChunk->mKey          != mSKey);
    IDE_TEST(mSChunk->mChunkCount   < 0);
    IDE_TEST(mSChunk->mChunkCount   >= IDU_SHM_MAX_CHUNKS);
    IDE_TEST(mSChunk->mKeys[0]      != mSKey);

    for(i = 1; i < mSChunk->mChunkCount; i++)
    {
        IDE_TEST(mSChunk->mKeys[i] == 0);
    }

    for(; i < IDU_SHM_MAX_CHUNKS; i++)
    {
        IDE_TEST(mSChunk->mKeys[i] != 0);
    }

    return ID_TRUE;

    IDE_EXCEPTION_END;
    return ID_FALSE;
}

idBool iduShmPersMgr::checkShmValid(SInt aIndex)
{
    IDE_DASSERT(mSChunk != NULL);

    IDE_TEST(mAddresses[aIndex]->mSize      != mSChunk->mSizes[aIndex]);
    IDE_TEST(mAddresses[aIndex]->mOffset    != 0);
    IDE_TEST(mAddresses[aIndex]->mIndex     != IDU_MEM_ID_CHUNK);
    if(aIndex == 0)
    {
        IDE_TEST(idlOS::memcmp(mSysSignature,
                               mAddresses[aIndex]->mSignature,
                               IDU_SHM_CHUNK_SIGSIZE) != 0);
    }
    else
    {
        IDE_TEST(idlOS::memcmp(mDatSignature,
                               mAddresses[aIndex]->mSignature,
                               IDU_SHM_CHUNK_SIGSIZE) != 0);
    }

    return ID_TRUE;

    IDE_EXCEPTION_END;
    return ID_FALSE;
}

IDE_RC iduShmPersMgr::writeAlloc(iduShmTxInfo*      aTxInfo,
                                 idShmAddr          aAddr)
{
    idrUImgInfo sArrUImgInfo[1];

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &aAddr, ID_SIZEOF(idShmAddr));

    idrLogMgr::writeLog( aTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_PERS_MGR_ALLOC_MEM,
                         1,
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoAlloc(idvSQL*         aSQL,
                                iduShmTxInfo*   aTxInfo,
                                idrLSN,
                                UShort          aSize,
                                SChar*          aImage)

{
    idShmAddr   sAddr;
    iduShmPtr*  sPtr;
    iduShmPtr*  sNewPtr;
    iduShmPtr*  sNewRight;

    IDE_ASSERT(aSize == sizeof(idShmAddr));

    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sAddr, sizeof(idShmAddr));
    sPtr = getShmPtrOfAddr(sAddr);
    sPtr->setUsed(ID_FALSE);

    IDE_TEST(mergeBlock(aSQL, aTxInfo, sPtr, sNewPtr, sNewRight) != IDE_SUCCESS);
    IDE_TEST(insertFreeBlock(aSQL, aTxInfo, sNewPtr) != IDE_SUCCESS);
    
    sNewRight->setPrevUsed(ID_FALSE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC iduShmPersMgr::writeCreateDataChunk( iduShmTxInfo * aTxInfo,
                                            SInt           aChunkCount,
                                            key_t          aKey)
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &aChunkCount, ID_SIZEOF(SInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[1], &aKey, ID_SIZEOF(key_t));

    idrLogMgr::writeLog( aTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                         IDU_VLOG_TYPE_SHM_PERS_MGR_CREATE_DATA_CHUNK,
                         2, /* Undo Image Size */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoCreateDataChunk(idvSQL       * /*aStatistics*/,
                                          iduShmTxInfo * /*aTxInfo*/,
                                          idrLSN         /*aNTALsn */,
                                          UShort           aSize,
                                          SChar          * aImage)
{
    SInt            i;
    SInt            sID;
    key_t           sKey;
    SInt            sChunkCount;

    IDE_ASSERT(mSChunk != NULL);
    IDE_ASSERT(aSize == ID_SIZEOF(SInt) + ID_SIZEOF(key_t));

    idlOS::memcpy( &sChunkCount, aImage, ID_SIZEOF(SInt) );
    aImage += ID_SIZEOF(SLong);

    idlOS::memcpy( &sKey, aImage, ID_SIZEOF(key_t) );
    aImage += ID_SIZEOF(SLong);

    IDE_ASSERT(mSChunk->mChunkCount == sChunkCount);

    /* Destroy shared memory */
    sID = idlOS::shmget((key_t)sKey, mChunkSize, 0666);
    if(sID != -1)
    {
        (void)idlOS::shmctl(sID, IPC_RMID, NULL);
    }
    else
    {
        /* fall through */
    }

    mSChunk->mChunkCount = sChunkCount;

    for(i = sChunkCount; i < IDU_SHM_MAX_CHUNKS; i++)
    {
        mSChunk->mKeys[i] = 0;
    }

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::writeAttachChunks(iduShmTxInfo * aTxInfo)
{
    ULong       sDummy;
    idrUImgInfo sArrUImgInfo[1];
    sDummy = 0;

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &sDummy, ID_SIZEOF(ULong));

    idrLogMgr::writeLog( aTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                         IDU_VLOG_TYPE_SHM_PERS_MGR_ATTACH_CHUNKS,
                         1, /* Undo Image Size */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoAttachChunks(idvSQL       * /* aStatistics */,
                                       iduShmTxInfo * /* aTxInfo */,
                                       idrLSN         /* aNTALsn */,
                                       UShort            aSize,
                                       SChar        * /* aImage */)
{
    SInt            i;

    IDE_ASSERT(mSChunk != NULL);
    IDE_ASSERT(aSize == ID_SIZEOF(ULong));

    for(i = 1; i < IDU_SHM_MAX_CHUNKS; i++)
    {
        mSChunk->mKeys[i] = 0;
    }

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::writeInsertFreeBlock(iduShmTxInfo*    aTxInfo,
                                           SInt             aFL,
                                           idShmAddr        aAddr)
{
    idrUImgInfo sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &aFL, ID_SIZEOF(SInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[1], &aAddr, ID_SIZEOF(idShmAddr));

    idrLogMgr::writeLog(aTxInfo,
                        IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                        IDU_VLOG_TYPE_SHM_PERS_MGR_INSERT_FREE_BLOCK,
                        2, /* Undo Image Size */
                        sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoInsertFreeBlock(idvSQL*,
                                          iduShmTxInfo*,
                                          idrLSN,
                                          UShort        aSize,
                                          SChar*        aImage)
{
    SInt        sFL;
    idShmAddr   sAddr;
    iduShmPtr*  sPtr;
    iduShmPtr*  sPrev;
    iduShmPtr*  sNext;

    IDE_ASSERT(mSChunk != NULL);
    IDE_ASSERT(aSize == ID_SIZEOF(SInt) + ID_SIZEOF(idShmAddr));

    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sFL, sizeof(SInt));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sAddr, sizeof(idShmAddr));

    if(sAddr == IDU_SHM_NULL_ADDR)
    {
        mSChunk->mFrees[sFL] = IDU_SHM_NULL_ADDR;
        mSChunk->mFL &= ~(1 << sFL);
    }
    else
    {
        sPtr    = getShmPtrOfAddr(sAddr);
        sNext   = getShmPtrOfAddr(mSChunk->mFrees[sFL]);
        sPrev   = getShmPtrOfAddr(sNext->mPrev);

        sPtr->mNext = mSChunk->mFrees[sFL];
        sPtr->mPrev = sNext->mPrev;

        sNext->mPrev = sAddr;
        sPrev->mNext = sAddr;

        mSChunk->mFrees[sFL] = sAddr;
        mSChunk->mFL |= 1 << sFL;
    }

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::writeRemoveBlock(iduShmTxInfo*    aTxInfo,
                                       SInt             aFL,
                                       idShmAddr        aAddr,
                                       idShmAddr        aPrev,
                                       idShmAddr        aNext)
{
    idrUImgInfo sArrUImgInfo[4];

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &aFL, ID_SIZEOF(SInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[1], &aAddr, ID_SIZEOF(idShmAddr));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[2], &aPrev, ID_SIZEOF(idShmAddr));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[3], &aNext, ID_SIZEOF(idShmAddr));

    idrLogMgr::writeLog(aTxInfo,
                        IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                        IDU_VLOG_TYPE_SHM_PERS_MGR_REMOVE_BLOCK,
                        4, /* Undo Image Size */
                        sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoRemoveBlock(idvSQL*,
                                      iduShmTxInfo*,
                                      idrLSN,
                                      UShort        aSize,
                                      SChar*        aImage)
{
    SInt        sFL;
    idShmAddr   sAddr;
    idShmAddr   sPrev;
    idShmAddr   sNext;
    iduShmPtr*  sPtr;
    iduShmPtr*  sPrevPtr;
    iduShmPtr*  sNextPtr;

    IDE_ASSERT(mSChunk != NULL);
    IDE_ASSERT(aSize == ID_SIZEOF(SInt) + (ID_SIZEOF(idShmAddr) * 3));

    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sFL, sizeof(SInt));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sAddr, sizeof(idShmAddr));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sPrev, sizeof(idShmAddr));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sNext, sizeof(idShmAddr));

    IDE_ASSERT(sAddr != IDU_SHM_NULL_ADDR);
    sPtr = getShmPtrOfAddr(sAddr);

    if(sFL == -1)
    {
        /* Just insert the block between prev and next */
        sPrevPtr                    = getShmPtrOfAddr(sAddr);
        sNextPtr                    = getShmPtrOfAddr(sNext);

        sPtr->mPrev                 = sPrev;
        sPtr->mNext                 = sPrev;

        sPrevPtr->mNext             = sAddr;
        sNextPtr->mPrev             = sAddr;
    }
    else
    {
        /* Free list was empty */
        sPtr->mPrev = sPtr->mNext   = sAddr;
        
        mSChunk->mFrees[sFL]        = sAddr;
        mSChunk->mFL               |= 1 << sFL;
    }

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::writeSplitBlock(iduShmTxInfo* aTxInfo, idShmAddr aAddr, UInt aSize, idShmAddr aRight)
{
    idrUImgInfo sArrUImgInfo[3];

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &aAddr, ID_SIZEOF(idShmAddr));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[1], &aSize, ID_SIZEOF(UInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[2], &aRight, ID_SIZEOF(idShmAddr));

    idrLogMgr::writeLog(aTxInfo,
                        IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                        IDU_VLOG_TYPE_SHM_PERS_MGR_SPLIT_BLOCK,
                        2, /* Undo Image Size */
                        sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoSplitBlock(idvSQL*,
                                     iduShmTxInfo*,
                                     idrLSN,
                                     UShort         aSize,
                                     SChar*         aImage)
{
    idShmAddr   sAddr;
    iduShmPtr*  sPtr;
    UInt        sSize;

    idShmAddr   sRightAddr;
    iduShmPtr*  sRightPtr;

    IDE_ASSERT(aSize == ID_SIZEOF(idShmAddr) * 2 + (ID_SIZEOF(UInt)));

    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sAddr, sizeof(idShmAddr));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sSize, sizeof(UInt));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sRightAddr, sizeof(idShmAddr));

    sPtr = getShmPtrOfAddr(sAddr);
    sPtr->mSize = sSize;

    sRightPtr = getShmPtrOfAddr(sRightAddr);
    sRightPtr->mLeftSize = sSize;

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::writeMergeLeftBlock(iduShmTxInfo* aTxInfo,
                                          idShmAddr     aAddr,
                                          UInt          aSize,
                                          idShmAddr     aLeftAddr,
                                          UInt          aLeftSize,
                                          idShmAddr     aRightAddr)
{
    idrUImgInfo sArrUImgInfo[5];

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &aAddr,      ID_SIZEOF(idShmAddr));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[1], &aSize,      ID_SIZEOF(UInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[2], &aLeftAddr,  ID_SIZEOF(idShmAddr));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[3], &aLeftSize,  ID_SIZEOF(UInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[4], &aRightAddr,  ID_SIZEOF(UInt));

    idrLogMgr::writeLog(aTxInfo,
                        IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                        IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_LEFT_BLOCK,
                        5, /* Undo Image Size */
                        sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoMergeLeftBlock(idvSQL*,
                                         iduShmTxInfo*,
                                         idrLSN,
                                         UShort         aSize,
                                         SChar*         aImage)
{
    idShmAddr   sAddr;
    iduShmPtr*  sPtr;
    UInt        sSize;

    idShmAddr   sLeftAddr;
    iduShmPtr*  sLeftPtr;
    UInt        sLeftSize;

    idShmAddr   sRightAddr;
    iduShmPtr*  sRightPtr;

    IDE_ASSERT(aSize == ID_SIZEOF(idShmAddr) * 3 + (ID_SIZEOF(UInt) * 2));

    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sAddr,        sizeof(idShmAddr));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sSize,        sizeof(UInt));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sLeftAddr,    sizeof(idShmAddr));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sLeftSize,    sizeof(UInt));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sRightAddr,   sizeof(idShmAddr));

    sPtr                    = getShmPtrOfAddr(sAddr);
    sPtr->mSize             = sSize;
    sLeftPtr                = getShmPtrOfAddr(sLeftAddr);
    sLeftPtr->mSize         = sLeftSize;
    sRightPtr               = getShmPtrOfAddr(sRightAddr);
    sRightPtr->mLeftSize    = sSize;

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::writeMergeRightBlock(iduShmTxInfo*    aTxInfo,
                                           idShmAddr        aAddr,
                                           UInt             aSize,
                                           idShmAddr        aRightAddr,
                                           UInt             aRightSize,
                                           idShmAddr        aRightRightAddr)
{
    idrUImgInfo sArrUImgInfo[5];

    IDR_VLOG_SET_UIMG(sArrUImgInfo[0], &aAddr,              ID_SIZEOF(idShmAddr));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[1], &aSize,              ID_SIZEOF(UInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[2], &aRightAddr,         ID_SIZEOF(idShmAddr));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[3], &aRightSize,         ID_SIZEOF(UInt));
    IDR_VLOG_SET_UIMG(sArrUImgInfo[4], &aRightRightAddr,    ID_SIZEOF(idShmAddr));

    idrLogMgr::writeLog(aTxInfo,
                        IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
                        IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_RIGHT_BLOCK,
                        5, /* Undo Image Size */
                        sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduShmPersMgr::undoMergeRightBlock(idvSQL*,
                                          iduShmTxInfo*,
                                          idrLSN,
                                          UShort         aSize,
                                          SChar*         aImage)
{
    idShmAddr   sAddr;
    iduShmPtr*  sPtr;
    UInt        sSize;

    idShmAddr   sRightAddr;
    iduShmPtr*  sRightPtr;
    UInt        sRightSize;

    idShmAddr   sRightRightAddr;
    iduShmPtr*  sRightRightPtr;

    IDE_ASSERT(aSize == ID_SIZEOF(idShmAddr) * 3 + (ID_SIZEOF(UInt) * 2));

    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sAddr,        sizeof(idShmAddr));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sSize,        sizeof(UInt));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sRightAddr,    sizeof(idShmAddr));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sRightSize,    sizeof(UInt));
    IDR_VLOG_GET_UIMG_N_NEXT(aImage, &sRightRightAddr,   sizeof(idShmAddr));

    sPtr                        = getShmPtrOfAddr(sAddr);
    sPtr->mSize                 = sSize;
    sRightPtr                   = getShmPtrOfAddr(sRightAddr);
    sRightPtr->mSize            = sRightSize;
    sRightRightPtr              = getShmPtrOfAddr(sRightRightAddr);
    sRightRightPtr->mLeftSize   = sRightSize;

    return IDE_SUCCESS;
}

