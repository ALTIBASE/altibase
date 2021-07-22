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
 * $Id: smlDef.h 89120 2020-11-02 06:06:28Z donlet $
 **********************************************************************/

#ifndef _O_SML_DEF_H_
#define _O_SML_DEF_H_ 1

#include <idl.h>
#include <iduList.h>
#include <smDef.h>
#include <smu.h>

#define SML_NUMLOCKTYPES      (6)
#define SML_LOCK_POOL_SIZE    (1024)
// For Table Lock Count for Prepare Log
# define SML_MAX_LOCK_INFO ((SM_PAGE_SIZE - (SMR_LOGREC_SIZE(smrXaPrepareLog)+ID_SIZEOF(smrLogTail))) / ID_SIZEOF(smlTableLockInfo))

#define SML_LOCKMODE_STRLEN  (32)

class  smlLockPool;

struct smlLockNode;
struct smlLockItem;

typedef enum 
{
    SML_NLOCK   = 0x00000000,// 널락
    SML_SLOCK   = 0x00000001,// 공유락
    SML_XLOCK   = 0x00000002,// 배제락
    SML_ISLOCK  = 0x00000003,// 의도 공유락
    SML_IXLOCK  = 0x00000004,// 의도 배제락
    SML_SIXLOCK = 0x00000005 // 공유 의도 배제락
} smlLockMode;

struct smlLockItem
{
    iduMutex            mMutex;

    smiLockItemType     mLockItemType;  // 잠금을 획득한 Item의 타입
    scSpaceID           mSpaceID;       // 잠금을 획득한 테이블스페이스 ID
    ULong               mItemID;        // 테이블타입이라면 Table OID
                                        // 디스크 데이타파일인 경우 File ID
    smlLockMode         mGrantLockMode;
    SInt                mGrantCnt;
    SInt                mRequestCnt;
    SInt                mFlag;         // table 대표락
    SInt                mArrLockCount[SML_NUMLOCKTYPES];

    smlLockNode*        mFstLockGrant; 
    smlLockNode*        mFstLockRequest; 
    smlLockNode*        mLstLockGrant; 
    smlLockNode*        mLstLockRequest;
};

// lock node생성을 줄이기 위하여 도입된 구조.
// 테이블에 특정 transaction의 grant된 lock노드만
// 존재하고, 또 그 transaction이  다른  lock conflict
// mode를 요청할때 request list에 달 lock node를
// 생성하지 않고,
// grant된 lock노드의 lock mode를 변경하고, lockSlot
// 에 연결한다.
typedef struct smlLockSlot
{
    smlLockNode     *mLockNode;
    smlLockSlot     *mPrvLockSlot;
    smlLockSlot     *mNxtLockSlot;

    /* BUG-15906: non-autocommit모드에서 Select완료후 IS_LOCK이 해제되면
     * 좋겠습니다.
     * Partial Rollback이나 Select Stmt End할때 Lock을 어디까지 풀어야
     * 할지를 결정하기 위해 Transaction의 Lock Slot의 Sequence Number를
     * 저장해 둔다. */
    ULong            mLockSequence;
    
    SInt             mMask;
    smlLockMode      mOldMode;
    smlLockMode      mNewMode;
} smlLockSlot;

typedef struct smlLockNode
{
    SInt             mIndex;
    smiLockItemType  mLockItemType;
    scSpaceID        mSpaceID;      // 잠금을 획득한 테이블스페이스 ID
    ULong            mItemID;       // 테이블타입이라면 Table OID
                                    // 디스크 데이타파일인 경우 File ID
    smTID            mTransID;
    SInt             mSlotID;      // lock을 요청한 transaction의 slot id
    smlLockMode      mLockMode;    // transaction이 요청한 lock mode.
    smlLockNode     *mPrvLockNode; // grant또는 request list연결포인터
    smlLockNode     *mNxtLockNode; // grant또는 request list연결포인터
    smlLockNode     *mCvsLockNode;
    // mCvsLockNode-> lock conflict발생시  해당 table에 grant
    // 된 lock을 소유한 Tx에 대하여,  이전 grant된 lock node의 pointer를
    // request list에 달 lock node에 이를 저장한다.
    // ->나중에 다른 트랜잭션에서  unlock table시에
    // request list에 있는 lock node와
    // 갱신된 table grant mode와 호환될때, grant list에 별도로
    // insert하지 않고 , 기존 cvs lock node의 lock mode만 갱신시켜
    // list 연산을 줄이려는 의도에서 도입되었음.
    UInt             mLockCnt;
    idBool           mBeGrant;   // grant되었는지를 나타내는 flag, BeGranted
    smlLockItem     *mLockItem;   
    SInt             mFlag; // lock node의 대표락.
    idBool           mIsExplicitLock; // BUG-28752 implicit/explicit lock을 구분합니다.
    
    //For Transaction
    smlLockNode     *mPrvTransLockNode;
    smlLockNode     *mNxtTransLockNode; 
    iduListNode      mNode4LockNodeCache; /* BUG-43408 */
    idBool           mDoRemove;  
    smlLockSlot      mArrLockSlotList[SML_NUMLOCKTYPES];
} smlLockNode;

typedef struct smlLockMatrixItem
{
    UShort           mIndex;               // 경로 길이.
    UShort           mNxtWaitTransItem ;
    // 주로 table lock의미로
    // 사용될수 있지만,record lock에서는
    // 자신이 대기하고 있는 트랜잭션의 리스트연결로
    // 사용된다.
    UShort           mNxtWaitRecTransItem;
   // record lock이미, 자신에게 record lock을 기다리고 있는
    // 리스트로 사용된다.
} smlLockMtxItem;

typedef struct smlTableLockInfo
{
    smOID       mOidTable;
    smlLockMode mLockMode;
} smlTableLockInfo;

// sml, smx 의존성을 제거하기 위하여,
// 도입됨.
// 트랜잭션이 lock을 잡은 lock node list(mLockNodeHeader),
// lock slit list(mLockSlotHeader)의 헤더를 가지고 있다.
// 그리고 waiting table에서 트랜잭션의 가장 처음 table lock대기slot id,
// record lock 대기 slot id를 저장하고 있다. 
typedef struct smlTransLockList
{
    smlLockNode       mLockNodeHeader; // Lock Node Header
    smlLockSlot       mLockSlotHeader; // Lock Slot Header
    UShort            mFstWaitTblTransItem; // 첫번째 Table Lock Wait Transaction
    UShort            mFstWaitRecTransItem; // 첫번째 Record Lock Wait Transaction
    UShort            mLstWaitRecTransItem; // 마지막 Record Lock Wait Transaction
    iduList           mLockNodeCache; /* BUG-43408 */

} smlTransLockList;

/* PROJ-2734 
 * 분산데드락 체크대상이 되는 TX을 선별하는 과정에서 사용한다.
 * 자세한 내용은 smlLockMgr::isCycle() 주석 참조 */
typedef enum
{
    SML_DIST_DEADLOCK_TX_NON_DIST_INFO = 0, /* 분산정보없는TX. 분산데드락 체크대상은 아니나, 분산정보있는TX 나올때까지 탐색해야 한다. */
    SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO,   /* 대기관계에서 첫번째 있는 분산정보있는TX. 분산데드락 체크대상이다. */
    SML_DIST_DEADLOCK_TX_UNKNOWN            /* FIRST_DIST_INFO 이후에 있는 TX들. 분산데드락 체크대상이 아니다. */
} smlDistDeadlockTxType;

typedef struct smlDistDeadlockNode
{
    smTID                 mTransID;
    smlDistDeadlockTxType mDistTxType;
} smlDistDeadlockNode;

typedef struct smlLockMode2StrTBL
{
    smlLockMode mLockMode;
    SChar    mLockModeStr[SML_LOCKMODE_STRLEN];
} smlLockModeStr;

#endif
