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

#if !defined(_O_IDU_SHM_PERS_MGR_H_)
#define _O_IDU_SHM_PERS_MGR_H_ 1

#include <idTypes.h>
#include <idrDef.h>
#include <iduMemDefs.h>
#include <iduShmDef.h>
#include <iduProperty.h>

typedef enum iduVLogShmPersMgrType
{
    IDU_VLOG_TYPE_SHM_PERS_MGR_NONE,
    IDU_VLOG_TYPE_SHM_PERS_MGR_ALLOC_MEM,
    IDU_VLOG_TYPE_SHM_PERS_MGR_CREATE_DATA_CHUNK,
    IDU_VLOG_TYPE_SHM_PERS_MGR_ATTACH_CHUNKS,
    IDU_VLOG_TYPE_SHM_PERS_MGR_INSERT_FREE_BLOCK,
    IDU_VLOG_TYPE_SHM_PERS_MGR_REMOVE_BLOCK,
    IDU_VLOG_TYPE_SHM_PERS_MGR_SPLIT_BLOCK,
    IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_LEFT_BLOCK,
    IDU_VLOG_TYPE_SHM_PERS_MGR_MERGE_RIGHT_BLOCK,
    IDU_VLOG_TYPE_SHM_PERS_MGR_MAX
} iduVLogShmPersMgrType;

#define IDU_SHM_CHUNK_SIGSIZE   (128 - sizeof(key_t))
#define IDU_SHM_MAX_CHUNKS      (4096)

#define IDU_SHM_STATUS_INIT     (0xDEAFBEEF)
#define IDU_SHM_STATUS_RUNNING  (0x105ECAFE)
#define IDU_SHM_STATUS_DETACHED (0xFADE0670)

typedef struct iduShmPtr
{
    ULong                   mSize;
    ULong                   mOffset;
    ULong                   mLeftSize;
    /* 24 */

    SInt                    mID;
    iduMemoryClientIndex    mIndex;
    /* 32 */

    SInt                    mUsed;
    SInt                    mPrevUsed;
    SChar                   mFiller1[8];
    /* 48 */

    idShmAddr               mPrev;
    idShmAddr               mNext;
    /* 64 */

    inline void setUsed(idBool aUsed = ID_TRUE)     { mUsed = (aUsed == ID_TRUE)? 1 : 0; }
    inline void setPrevUsed(idBool aUsed = ID_TRUE) { mPrevUsed = (aUsed == ID_TRUE)? 1 : 0; }
    inline idBool getUsed(void)         { return (mUsed == 0)? ID_FALSE:ID_TRUE; }
    inline idBool getPrevUsed(void)     { return (mPrevUsed == 0)? ID_FALSE:ID_TRUE; }

    inline idShmAddr    getAddr()       {return IDU_SHM_MAKE_PERS_ADDR(mID, mOffset);}
    inline operator     idShmAddr()     {return getAddr();}

    inline iduShmPtr*   getLeft(void)   { return (iduShmPtr*)((SChar*)this - mLeftSize); }
    inline iduShmPtr*   getRight(void)  { return (iduShmPtr*)((SChar*)this + mSize); }
    inline UInt         getSize(void)   { return mSize; }
} iduShmPtr;

typedef struct iduShmPersChunk : public iduShmPtr
{
    key_t                   mKey;
    SChar                   mSignature[IDU_SHM_CHUNK_SIGSIZE];
} iduShmPersChunk;

typedef struct iduShmPersSys : public iduShmPersChunk
{
    iduShmLatch             mLatch;

    SInt                    mKeys[IDU_SHM_MAX_CHUNKS];
    UInt                    mSizes[IDU_SHM_MAX_CHUNKS];
    SLong                   mChunkCount;
    SInt                    mStatus;
    SInt                    mUID;
    SInt                    mLastKey;
    SChar                   mFiller4[64 - (sizeof(SInt) * 3 + sizeof(SLong))];

    /* Reserved Addresses */
    idShmAddr               mReserved[IDU_SHM_META_MAX_COUNT];

    /*
     * Free list - 25 elements
     * 2^7 = 128 ~ 2 ^ 31 = 2G
     */
    SInt                    mFL;
    idShmAddr               mFrees[25];

    iduShmPtr               mSentinel;
} iduShmPersSys;

typedef struct iduShmPersData : public iduShmPersChunk
{
    iduShmPtr               mFirstBlock;
} iduShmPersData;

class iduShmPersMgr
{
public:
    static IDE_RC   initialize(idvSQL*, iduShmTxInfo*, iduShmProcType);
    static IDE_RC   destroy(void);
    static IDE_RC   clear(void);

    static IDE_RC   alloc(idvSQL*, iduShmTxInfo*,
                          iduMemoryClientIndex, UInt, void**);
    static IDE_RC   free(idvSQL*, iduShmTxInfo*, void*);

    static IDE_RC   reserve(idvSQL*, iduShmTxInfo*,
                            iduShmMetaBlockType,
                            iduMemoryClientIndex,
                            UInt, void**);
    static IDE_RC   unreserve(idvSQL*, iduShmTxInfo*, iduShmMetaBlockType);
    static void*    getReserved(iduShmMetaBlockType);

    static void*        getPtrOfAddr(idShmAddr);
    static idShmAddr    getAddrOfPtr(void*);
    static iduShmPtr*   getShmPtrOfAddr(idShmAddr);

    static inline SInt  calcFL(UInt);

    static iduShmPersSys*   mSChunk;

private:
    /* Sys Segment Header */
    static key_t            mSKey;
    static SInt             mSID;
    static ULong            mChunkSize;
    static SChar            mSysSignature[IDU_SHM_CHUNK_SIGSIZE];
    static SChar            mDatSignature[IDU_SHM_CHUNK_SIGSIZE];

    static SInt             mIDs[IDU_SHM_MAX_CHUNKS];
    static iduShmPersChunk* mAddresses[IDU_SHM_MAX_CHUNKS];

    static IDE_RC           getSysChunk(idvSQL*, iduShmTxInfo*);
    static IDE_RC           createSysChunk(idvSQL*, iduShmTxInfo*);
    static IDE_RC           createDataChunk(idvSQL*, iduShmTxInfo*, SInt, iduShmPtr*&);

    static IDE_RC           attachChunks(idvSQL*, iduShmTxInfo*);
    static IDE_RC           detachChunks(void);
    static IDE_RC           clearSysChunk(void);
    static IDE_RC           cleanupChunks(void);

    static IDE_RC           attachOneChunk(UInt);

    static IDE_RC           insertFreeBlock(idvSQL*, iduShmTxInfo*, iduShmPtr*);
    static IDE_RC           removeFreeBlock(idvSQL*, iduShmTxInfo*, UInt, iduShmPtr*&);
    static IDE_RC           removeBlock(idvSQL*, iduShmTxInfo*, iduShmPtr*);
    static IDE_RC           splitBlock(idvSQL*, iduShmTxInfo*, iduShmPtr*, UInt, iduShmPtr*&);
    static IDE_RC           mergeBlock(idvSQL*, iduShmTxInfo*, iduShmPtr*, iduShmPtr*&, iduShmPtr*&);
    static IDE_RC           mergeLeft(idvSQL*, iduShmTxInfo*, iduShmPtr*, iduShmPtr*&);
    static IDE_RC           mergeRight(idvSQL*, iduShmTxInfo*, iduShmPtr*, iduShmPtr*&);

    static idBool           checkSysValid(void);
    static idBool           checkShmValid(SInt);

public:
    /* Redo and Undo functions */
    static IDE_RC writeAlloc(iduShmTxInfo*, idShmAddr);
    static IDE_RC writeCreateDataChunk(iduShmTxInfo*, SInt, key_t);
    static IDE_RC writeAttachChunks(iduShmTxInfo*);
    static IDE_RC writeInsertFreeBlock(iduShmTxInfo*, SInt, idShmAddr);
    static IDE_RC writeRemoveBlock(iduShmTxInfo*, SInt, idShmAddr, idShmAddr, idShmAddr);
    static IDE_RC writeSplitBlock(iduShmTxInfo*, idShmAddr, UInt, idShmAddr);
    static IDE_RC writeMergeLeftBlock(iduShmTxInfo*, idShmAddr, UInt, idShmAddr, UInt, idShmAddr);
    static IDE_RC writeMergeRightBlock(iduShmTxInfo*, idShmAddr, UInt, idShmAddr, UInt, idShmAddr);

    static IDE_RC undoAlloc(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);
    static IDE_RC undoCreateDataChunk(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);
    static IDE_RC undoAttachChunks(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);
    static IDE_RC undoInsertFreeBlock(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);
    static IDE_RC undoRemoveBlock(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);
    static IDE_RC undoSplitBlock(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);
    static IDE_RC undoMergeLeftBlock(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);
    static IDE_RC undoMergeRightBlock(idvSQL*, iduShmTxInfo*, idrLSN, UShort, SChar*);

public:
    static idrUndoFunc mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERS_MGR_MAX];
    static SChar       mStrVLogType[IDU_VLOG_TYPE_SHM_PERS_MGR_MAX+1][100];
};

inline SInt iduShmPersMgr::calcFL(UInt aSize)
{
    IDE_DASSERT(aSize >= 64);
    return acpBitFls32(aSize) - 6;
}
    
#endif

