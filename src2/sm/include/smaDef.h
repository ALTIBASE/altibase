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
 * $Id: smaDef.h 88423 2020-08-26 04:06:55Z emlee $
 **********************************************************************/

#ifndef _O_SMA_DEF_H_
# define _O_SMA_DEF_H_ 1


# include <smDef.h>
# include <smx.h>
# include <smnDef.h>

# define SMA_NODE_POOL_LIST_COUNT  (4)
# define SMA_NODE_POOL_SIZE        (1024)

# define SMA_TRANSINFO_DEF_COUNT (1024)

# define SMA_NODE_POOL_MAXIMUM (100)
# define SMA_NODE_POOL_CACHE   (25)

# define SMA_GC_NAME_LEN              (128)

/* BUG-35179 Add property for parallel logical ager */
# define SMA_PARALLEL_LOGICAL_AGER_OFF (0)
# define SMA_PARALLEL_LOGICAL_AGER_ON (1)

/* BUG-47367 */
# define SMA_PARALLEL_DELETE_THREAD_OFF (0)
# define SMA_PARALLEL_DELETE_THREAD_ON  (1)


typedef struct smaOidList
{
    smSCN       mSCN;
    // row의 free와  index node free간의
    // sync를 마추기위하여 사용.
    // oid에 대응하는  인덱스 key slot들을
    // free 완료한 시점의 system view scn. 
    smSCN       mKeyFreeSCN;  
    idBool      mErasable;
    idBool      mFinished;
    UInt        mCondition;
    smTID       mTransID;
    SInt        mListN;
    smaOidList* mNext;
    smxOIDNode* mHead;
    smxOIDNode* mTail;
}smaOidList;

// for fixed table .
typedef struct smaGCStatus
{
    //GC name :smaLogicalAger, smaDeleteThread.
    SChar  *mGCName;
    // 현재 systemViewSCN.
    smSCN  mSystemViewSCN;
    // Active Transaction 중 최소 memory view SCN
    smSCN  mMiMemSCNInTxs;
    // BUG-24821 V$TRANSACTION에 LOB관련 MinSCN이 출력되어야 합니다. 
    // mMiMemSCNInTxs을 소유한 트랜잭션 아이디(가장 오래된 트랜잭션)
    smTID  mOldestTx;
    // LogicalAger, DeleteThred의 Tail의 commitSCN
    smSCN  mSCNOfTail;
    // OIDList가 비어 있는지 boolean
    idBool mIsEmpty;
    // add된 OID 갯수.
    ULong  mAddOIDCnt;
    // GC처리가된 OID갯수.
    ULong  mGcOIDCnt;

    /* Transaction이 Commmit/Abort시 OID List에 Aging을 수행해야할
     * OID갯수가 더해진다.*/
    ULong mAgingRequestOIDCnt;

    /* Ager가 OID List의 OID를 하나씩 처리하는데 이때 1식증가 */
    ULong mAgingProcessedOIDCnt;

    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부화 및 Aging이
    //                 밀리는 현상이 더 심화됨.
    // getMinSCN했을때, MinSCN때문에 작업하지 못한 횟수
    ULong mSleepCountOnAgingCondition;

    /* 현재 수행중인 Logical Ager Thread, 혹은 Delete Thread의 갯수 */
    UInt  mThreadCount;
} smaGCStatus;

/*
    Instant Aging의 조건 Filter

    Instant Aging을 수행할 조건을 저장한다.
    
    [ 가능한 Filter들 ]
      1. mTBSID가 SC_NULL_SPACEID가 아닌 경우
         => 특정 Tablespace에 속한 OID들만 Instant Aging실시
      2. mTableOID가 SM_NULL_OID가 아닌 경우
         => 특정 Table에 속한 OID들만 Instant Aging 실시
 */
typedef struct smaInstantAgingFilter
{
    scSpaceID mTBSID;    
    smOID     mTableOID;
} smaInstantAgingFilter ;


#endif /* _O_SMA_DEF_H_ */
