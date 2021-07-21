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

#ifndef _O_MMD_DEF_H_
#define _O_MMD_DEF_H_ 1

#include <iduLatch.h>
#include <iduList.h>

typedef enum mmdXaAssocState
{
    MMD_XA_ASSOC_STATE_NOTASSOCIATED = 0,
    MMD_XA_ASSOC_STATE_ASSOCIATED,
    MMD_XA_ASSOC_STATE_SUSPENDED
} mmdXaAssocState;

typedef enum mmdXaOperation
{
    MMD_XA_OP_OPEN = 0,
    MMD_XA_OP_CLOSE,
    MMD_XA_OP_START,
    MMD_XA_OP_END,
    MMD_XA_OP_PREPARE,
    MMD_XA_OP_COMMIT,
    MMD_XA_OP_ROLLBACK,
    MMD_XA_OP_FORGET,
    MMD_XA_OP_RECOVER,
    MMD_XA_OP_MAX
} mmdXaOperation;

/***************************************************************
 * < state transition table > 
 * S0(Non-exist Transaction)
 * S1(Active)
 * S2(Idle)
 * S3(Prepared)
 * S4(Heuristic commit/rollback)
 *                              S0    S1    S2      S3     S4
 * xa_start                     S1          S1
 * xa_end                             S2
 * xa_prepare                               S3
 * xa_commit/xa_rollback                    S0      S0     S4
 * xa_forget                                               S0
 * 그 외 경우에는 XAER_PROTO 에러 반환
 ***************************************************************/
typedef enum mmdXaState
{
    MMD_XA_STATE_IDLE = 0,
    MMD_XA_STATE_ACTIVE,
    MMD_XA_STATE_PREPARED,
    MMD_XA_STATE_HEURISTICALLY_COMMITTED,
    MMD_XA_STATE_HEURISTICALLY_ROLLBACKED,
    MMD_XA_STATE_NO_TX,
    MMD_XA_STATE_ROLLBACK_ONLY,
    MMD_XA_STATE_MAX
} mmdXaState;

//fix BUG-21794.
typedef struct mmdIdXidNode
{
    ID_XID        mXID;
    iduListNode   mLstNode;
}mmdIdXidNode;

//fix BUG-21889
typedef enum mmdXaLatchMode
{
    MMD_XA_S_LATCH = 0,
    MMD_XA_X_LATCH
}mmdXaLatchMode ;

//fix BUG-22033
typedef enum mmdXaWaitMode
{
    MMD_XA_NO_WAIT = 0,
    MMD_XA_WAIT = 1
}mmdXaWaitMode ;

//fix BUG-22033
typedef enum mmdXaLogFlag
{
    MMD_XA_DO_NOT_LOG = 0,
    MMD_XA_DO_LOG = 1
}mmdXaLogFlag ;

 //fix BUG-27218 XA Load Heurisitc Transaction함수 내용을 명확히 해야 한다.
typedef enum mmdXaLoadHeuristicXidFlag
{
    MMD_LOAD_HEURISTIC_XIDS_AT_STARTUP = 0,
    MMD_LOAD_HEURISTIC_XIDS_AT_XA_RECOVER = 1
}mmdXaLoadHeuristicXidFlag ;

/*
 * Flag definition for the RM switch
 */
#define TMNOFLAGS       0x00000000L     /* no resource manager features
                                           selected */
#define TMREGISTER      0x00000001L     /* resource manager dynamically
                                           registers */
#define TMNOMIGRATE     0x00000002L     /* resource manager does not support
                                           association migration */
#define TMUSEASYNC      0x00000004L     /* resource manager supports
                                           asynchronous operations */
/*
 * Flag definitions for xa_ and ax_ routines
 */
/* Use TMNOFLAGS, defined above, when not specifying other flags */
#define TMASYNC         0x80000000L     /* perform routine asynchronously */
#define TMONEPHASE      0x40000000L     /* caller is using one-phase commit
                                        optimisation */
#define TMFAIL          0x20000000L     /* dissociates caller and marks
                                           transaction branch rollback-only */
#define TMNOWAIT        0x10000000L     /* return if blocking condition
                                           exists */
#define TMRESUME        0x08000000L     /* caller is resuming association
                                           with suspended transaction branch */
#define TMSUCCESS       0x04000000L     /* dissociate caller from transaction
                                        branch */
#define TMSUSPEND       0x02000000L     /* caller is suspending, not ending,
                                           association */
#define TMSTARTRSCAN    0x01000000L     /* start a recovery scan */
#define TMENDRSCAN      0x00800000L     /* end a recovery scan */
#define TMMULTIPLE      0x00400000L     /* wait for any asynchronous
                                           operation */
#define TMJOIN          0x00200000L     /* caller is joining existing
                                        transaction branch */
#define TMMIGRATE       0x00100000L     /* caller intends to perform
                                        migration */

/*
 * ax_() return codes (transaction manager reports to resource manager)
 */
#define TM_JOIN         2       /* caller is joining existing transaction
                                branch */
#define TM_RESUME       1       /* caller is resuming association with
                                   suspended transaction branch */
#define TM_OK   0               /* normal execution */
#define TMER_TMERR      -1      /* an error occurred in the transaction
                                manager */
#define TMER_INVAL      -2      /* invalid arguments were given */
#define TMER_PROTO      -3      /* routine invoked in an improper context */

/*
 * xa_() return codes (resource manager reports to transaction manager)
 */
#define XA_RBBASE       100             /* The inclusive lower bound of the
                                           rollback codes */
#define XA_RBROLLBACK   XA_RBBASE       /* The rollback was caused by an
                                           unspecified reason */
#define XA_RBCOMMFAIL   XA_RBBASE+1     /* The rollback was caused by a
                                           communication failure */
#define XA_RBDEADLOCK   XA_RBBASE+2     /* A deadlock was detected */
#define XA_RBINTEGRITY  XA_RBBASE+3     /* A condition that violates the
                                           integrity of the resources was
                                           detected */
#define XA_RBOTHER      XA_RBBASE+4     /* The resource manager rolled back the
                                           transaction for a reason not on this
                                           list */
#define XA_RBPROTO      XA_RBBASE+5     /* A protocal error occurred in the
                                           resource manager */
#define XA_RBTIMEOUT    XA_RBBASE+6     /* A transaction branch took too long*/
#define XA_RBTRANSIENT  XA_RBBASE+7     /* May retry the transaction branch */
#define XA_RBEND        XA_RBTRANSIENT  /* The inclusive upper bound of the
                                           rollback codes */

#define XA_NOMIGRATE    9               /* resumption must occur where
                                           suspension occurred */
#define XA_HEURHAZ      8               /* the transaction branch may have been
                                           heuristically completed */
#define XA_HEURCOM      7               /* the transaction branch has been
                                           heuristically comitted */
#define XA_HEURRB       6               /* the transaction branch has been
                                           heuristically rolled back */
#define XA_HEURMIX      5               /* the transaction branch has been
                                           heuristically committed and rolled
                                           back */
#define XA_RETRY        4               /* routine returned with no effect
                                           and may be re-issued */
#define XA_RDONLY       3               /* the transaction was read-only
                                           and has been committed */
#define XA_OK           0               /* normal execution */
#define XAER_ASYNC      -2              /* asynchronous operation already
                                           outstanding */
#define XAER_RMERR      -3              /* a resource manager error occurred
                                        in the transaction branch */
#define XAER_NOTA       -4              /* the XID is not valid */
#define XAER_INVAL      -5              /* invalid arguments were given */
#define XAER_PROTO      -6              /* routine invoked in an improper
                                           context */
#define XAER_RMFAIL     -7              /* resource manager unavailable */
#define XAER_DUPID      -8              /* the XID already exists */
#define XAER_OUTSIDE    -9              /* resource manager doing work */
                                        /* outside global transaction */

//fix BUG-22306 XA ROLLBACK시 해당 XID가 ACTIVE일때
//대기 완료후 이에 대한 rollback처리 정책이 잘못됨.
#define XA_ROLLBACK_DEFAULT_WAIT_TIME (600)  // default query timeout value.

//fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
//그들간의 관계를 정확히 유지해야 함.
#define XID_DATA_MAX_LEN   ID_XID_DATA_MAX_LEN

//fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
//Bug Fix를  mmdManager::checkXATimeOut에도 적용해야 합니다.
#define MMD_XA_RETRY_SPIN_TIMEOUT  100000
#define MMD_XA_NONE                0
#define MMD_XA_ADD_IF_NOT_FOUND    0x01
#define MMD_XA_REMOVE_FROM_XIDLIST 0x10
#define MMD_XA_REMOVE_FROM_SESSION 0x40

#define MMD_XA_XID_RESTART_FLAG(aFlag) (((aFlag) & TMJOIN) || ((aFlag) & TMRESUME))
//----------------
// X$XID 의 구조
//----------------
typedef struct mmdXidInfo4PerfV
{
    ID_XID      mXIDValue;           //XID_VALUE
    mmcSessID   mAssocSessionID;     //ASSOC_SESSION_ID
    mmcTransID  mTransID;            //TRANS_ID
    mmdXaState  mState;              //STATE
    UInt        mStateStartTime;     //STATE_START_TIME         //BUG-25078   State가 변경된 시각
    ULong       mStateDuration;      //STATE_DURATION
    idBool      mBeginFlag;          //GLOBAL_TX_BEGIN_FLAG
    /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다. FixCount으로 용어 통일*/
    UInt        mFixCount;            //REF_COUNT
} mmdXidInfo4PerfV;
/* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now.
  that is to say , chanage the granularity from global to bucket level.
 */
typedef struct mmdXidHashBucket
{
    iduList     mChain;
    // PROJ-2408 : iduLatchObject 제거
    iduLatch mBucketLatch;
    // bug-35382: mutex optimization during alloc and dealloc
    // 전역 iduMutexMgr 에서의 xid mutex 할당 병목을 해소하기 위해
    // mutex를 hash에 미리 준비해 두고, hash크기만큼 병목을 분산시킴.
    iduList     mXidMutexChain;
    iduLatch mXidMutexBucketLatch;
}mmdXidHashBucket;

// mutex를 hash에 매달기 위해 node 있는 구조체 추가
typedef struct mmdXidMutex
{
    iduMutex     mMutex;
    iduList      mListNode;
}mmdXidMutex;

#endif
