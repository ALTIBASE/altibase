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
 * $Id: sdbBufferMgr.cpp 17358 2006-07-31 03:12:47Z bskim $
 *
 * Description :
 *
 * 용어설명 :
 *
 * 약어 :
 * 
 **********************************************************************/

#ifndef _O_STNMR_DEF_H_
# define _O_STNMR_DEF_H_ 1

# include <idl.h>
# include <idu.h>
# include <iduLatch.h>

# include <smm.h>
# include <smu.h>
# include <smnDef.h>
# include <stdTypes.h>

# define STNMR_NODE_LATCH_BIT       (0x00000001)

# define STNMR_NODE_POOL_LIST_COUNT (64)
# define STNMR_NODE_POOL_SIZE       (1024)

# define STNMR_SLOT_MAX             (64)
# define STNMR_SLOT_MIN             (0)
/* BUG-28319
   Memory R-Tree의 Iterator에서 내부 mStack이 Overflow날 수 있습니다.
   STNMR_STACK_DEPTH를 256에서 2048로 변경합니다. */
# define STNMR_STACK_DEPTH          (2048)
# define STNMR_NODE_TYPE_MASK       (0x00000001)
# define STNMR_NODE_TYPE_INTERNAL   (0x00000000)
# define STNMR_NODE_TYPE_LEAF       (0x00000001)

# define STNMR_AGER_COUNT           (2)
# define STNMR_AGER_MAX_COUNT       (25)

# define STNMR_LIST_COUNT_MAX       256
# define STNMR_ELEM_COUNT_MAX       (10000000)

# define STNMR_NODE_POOL_CACHE      10 
# define STNMR_NODE_POOL_MAXIMUM    100

# define STNMR_MAX(a, b)                 (((a) > (b))? (a) : (b))
# define STNMR_MIN(a, b)                 (((a) < (b))? (a) : (b))

typedef struct stnmrColumn
{
    smiCompareFunc    mCompare;
    smiColumn         mColumn;
} stnmrColumn;

struct stnmrSlot;
struct stnmrNode;

typedef struct stnmrSlot
{
    stdMBR   mMbr;
    ULong    mVersion;
    
    union
    {
        void*      mPtr;
        stnmrNode* mNodePtr;
    };
} stnmrSlot;

typedef struct stnmrNode
{
    /* For Physical Aging   */
    stnmrNode*       mPrev;
    stnmrNode*       mNext;
    smSCN            mSCN;
    stnmrNode*       mFreeNodeList;
    /* Body                 */
    IDU_LATCH        mLatch;
    ULong            mVersion;
    SInt             mSlotCount;
    SInt             mFlag;
    stnmrSlot        mSlots[STNMR_SLOT_MAX];
    stnmrNode*       mNextSPtr;
} stnmrNode;

typedef idBool (*stnmrIntersectFunc) (void *a1, 
                                      void *a2);

typedef idBool (*stnmrContainsFunc) (void *a1, 
                                     void *a2);

typedef void* (*stnmrCopyMbrFunc) (void *aMbr1, 
                                   void *aMbr2);

typedef void* (*stnmrGetExtentMBRFunc) (void *aMbr1, 
                                        void *aMbr2);

typedef SDouble (*stnmrGetAreaFunc) (void *aMbr);

typedef SDouble (*stnmrGetDeltaFunc) (void *aMbr1, 
                                      void *aMbr2);

typedef struct stnmrIndexModule
{
    stnmrIntersectFunc		mIntersect;
    stnmrContainsFunc		mContains;
    stnmrCopyMbrFunc		mCopyMbr;
    stnmrGetExtentMBRFunc	mGetExtentMBR;
    stnmrGetAreaFunc		mGetArea;
    stnmrGetDeltaFunc		mGetDelta;
} stnmrIndexModule;

#define STNMR_NODE_LATCH(aNode)  \
    IDL_MEM_BARRIER;            \
    IDE_ASSERT((((stnmrNode*)(aNode))->mLatch & STNMR_NODE_LATCH_BIT) != STNMR_NODE_LATCH_BIT); \
    ((stnmrNode*)(aNode))->mLatch |= STNMR_NODE_LATCH_BIT; \
    IDL_MEM_BARRIER;


#define STNMR_NODE_UNLATCH(aNode)  \
    IDL_MEM_BARRIER;              \
    (((stnmrNode*)(aNode))->mLatch)++; \
    IDE_ASSERT((((stnmrNode*)(aNode))->mLatch & STNMR_NODE_LATCH_BIT) != STNMR_NODE_LATCH_BIT);

typedef struct stnmrNodeList
{
    SInt      mNodeCount;
    stnmrNode mNode[STNMR_STACK_DEPTH];
} stnmrNodeList;

typedef struct stnmrStatistic
{
    ULong   mKeyCompareCount;
    ULong   mKeyValidationCount;
    ULong   mNodeSplitCount;
    ULong   mNodeDeleteCount;
} stnmrStatistic;

typedef struct stnmrHeader
{
    SMN_RUNTIME_PARAMETERS

    iduMutex     mMutex;
    stnmrNode*   mRoot;

    stnmrNode*   mFstNode;
    stnmrNode*   mLstNode;
    ULong        mVersion;
    ULong        mRootVersion;
    UInt         mNodeCount;
    IDU_LATCH    mLatch;
    SInt         mDepth;
    stdMBR       mTreeMBR;

    // BUG-18604 STMT및 AGER로 인한 통계정보 구축
    ULong          mKeyCount;
    stnmrStatistic mStmtStat;
    stnmrStatistic mAgerStat;
    
    smmSlotList  mNodePool;
    stnmrColumn* mFence;
    stnmrColumn* mColumns; //fix BUG-23023

    stnmrIndexModule mFilterFunc;
} stnmrHeader;

typedef struct stnmrStack
{
    stnmrNode*   mNodePtr;
    ULong        mVersion;
    SInt         mLstReadPos;
} stnmrStack;

typedef struct stnmrIterator
{
    smSCN       mSCN;
    smSCN       mInfinite;
    void*       mTrans;
    void*       mTable;
    SChar*      mCurRecPtr;  // MRDB scan module에서만 정의해서 쓰도록 수정?
    SChar*      mLstFetchRecPtr;
    scGRID      mRowGRID;
    smTID       mTid;
    UInt        mFlag;

    smiCursorProperties  * mProperties;
    smiStatement         * mStatement;
    /* smiIterator 공통 변수 끝 */

    idBool             mLeast;
    idBool             mHighest;
    stnmrHeader*       mIndex;
    const smiRange*    mKeyRange;
    const smiCallBack* mRowFilter;
    stnmrNode*         mNode;
    stnmrNode*         mNxtNode;
    SChar**            mSlot;
    SChar**            mLowFence;
    SChar**            mHighFence;
    ULong              mVersion;

    SInt               mDepth;
    SChar*             mRows[STNMR_SLOT_MAX];
    stnmrStack         mStack[STNMR_STACK_DEPTH];
} stnmrIterator;

typedef struct stnmrHeader4PerfV
{
    UChar      mName[SMN_MAX_INDEX_NAME_SIZE+8]; // INDEX_NAME
    UInt       mIndexID;          // INDEX_ID
    UInt       mTableTSID;        // TABLE_TBS_ID 
    stdMBR     mTreeMBR;          // TREE_MBR_MIN_X
                                  // TREE_MBR_MIN_Y
                                  // TREE_MBR_MAX_X
                                  // TREE_MBR_MAX_Y
    UInt       mUsedNodeCount;    // USED_NODE_COUNT
    UInt       mPrepareNodeCount; // PREPARE_NODE_COUNT
} stnmrHeader4PerfV;

//-------------------------------
// X$MEM_RTREE_STAT 의 구조
//-------------------------------
typedef struct stnmrStat4PerfV
{
    UChar            mName[SMN_MAX_INDEX_NAME_SIZE+8];
    UInt             mIndexID;
    iduMutexStat     mTreeLatchStat;
    ULong            mKeyCount;
    stnmrStatistic   mStmtStat;
    stnmrStatistic   mAgerStat;
    stdMBR           mTreeMBR;
} stnmrStat4PerfV;

//-------------------------------
// X$MEM_RTREE_NODEPOOL 의 구조
//-------------------------------
typedef struct stnmrNodePool4PerfV
{
    UInt             mTotalPageCount;  // TOTAL_PAGE_COUNT
    UInt             mTotalNodeCount;  // TOTAL_NODE_COUNT
    UInt             mFreeNodeCount;   // FREE_NODE_COUNT
    UInt             mUsedNodeCount;   // USED_NODE_COUNT
    UInt             mNodeSize;        // NODE_SIZE
    ULong            mTotalAllocReq;   // TOTAL_ALLOC_REQ
    ULong            mTotalFreeReq;    // TOTAL_FREE_REQ
    UInt             mFreeReqCount;    // FREE_REQ_COUNT
} stnmrNodePool4PerfV;

#endif /* _O_STNMR_DEF_H_ */
