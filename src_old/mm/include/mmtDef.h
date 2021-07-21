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

#ifndef   _O_MMT_DEF_H_
#define   _O_MMT_DEF_H_  1
#include <idl.h>
#include <idvProfile.h>
#include <mmcDef.h>

/* [BUG-29592] A responsibility for updating  a global session time have better to be assigned to
   session manager    rather than load balancer*/
#define MMT_THR_CREATE_WAIT_TIME (12)

/* PROJ-2616 */
#define MMT_IPCDA_INCREASE_DATA_COUNT(aCtx)                 \
do                                                          \
{                                                           \
    if (cmiGetLinkImpl(aCtx) == CMI_LINK_IMPL_IPCDA)        \
    {                                                       \
        cmiIPCDAIncDataCount(aCtx);                         \
    }                                                       \
} while(0)

/*
 * TASK-7218 Multi-Error Handling 2nd
 *   클라이언트가 shardcli가 아닌 User 세션이면 Multiple Error를 전송.
 *   그 외 세션은 shardcli 내부에서 Multiple Error를 만든다.
 *   arrary-binding은 Multiple Error 적용이 제외된다.
 */
#define MMT_NEED_TO_SEND_MULTIPLE_ERROR(aSession, aExecuteOption)          \
  ((((aSession) != NULL) &&                                                \
    ((aSession)->isShardClient() == SDI_SHARD_CLIENT_FALSE) &&             \
    ((aSession)->getShardSessionType() == SDI_SESSION_TYPE_USER) &&        \
    (ideErrorCollectionSize() > 1) &&                                      \
    ((aExecuteOption) <= CMP_DB_EXECUTE_NORMAL_EXECUTE ))                  \
   ? ID_TRUE : ID_FALSE)

typedef enum mmtServiceThreadState
{
    MMT_SERVICE_THREAD_STATE_NONE,
    MMT_SERVICE_THREAD_STATE_POLL,
    MMT_SERVICE_THREAD_STATE_EXECUTE
} mmtServiceThreadState;

//fix BUG-19323
typedef enum mmtServiceThreadRunMode
{
    MMT_SERVICE_THREAD_RUN_SHARED =0,
    MMT_SERVICE_THREAD_RUN_DEDICATED
} mmtServiceThreadRunMode;

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   Service Thread의 현재 busy , idle여부.
*/
typedef enum mmtServiceThreadRunStatus
{
    MMT_SERVICE_THREAD_RUN_IN_IDLE =0,
    MMT_SERVICE_THREAD_RUN_IN_BUSY
} mmtServiceThreadRunStatus;

/* BUG-38384 A task in a service thread can be a orphan */
typedef enum mmtServiceThreadLock
{
    MMT_SERVICE_THREAD_NO_LOCK = 0,
    MMT_SERVICE_THREAD_LOCK    = 1
} mmtServiceThreadLock;

typedef struct mmtServiceThreadInfo
{
    UInt                  mServiceThreadID;
    UInt                  mIpcID;
    PDL_thread_t          mThreadID;

    mmcServiceThreadType  mServiceThreadType;

    mmtServiceThreadState mState;
    //fix BUG-19323    
    mmtServiceThreadRunMode mRunMode;

    UInt                  mStartTime;

    mmcSessID             mSessionID;
    mmcStmtID             mStmtID;

    UInt                  mTaskCount;
    UInt                  mReadyTaskCount;

    idvTime               mExecuteBegin;
    idvTime               mExecuteEnd;
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       load-balance 이력을 추적하기 위하여 추가한 필드임.
    */
    UInt                  mCurAddedTasks;
    UInt                  mInTaskCntFromIdle;
    UInt                  mInTaskCntFromBusy;

    UInt                  mOutTaskCntInIdle;
    UInt                  mOutTaskCntInBusy;
    
    UInt                  mBusyExperienceCnt;
    UInt                  mLifeSpan;

    /* BUG-38384 A task in a service thread can be a orphan */
    idBool                mRemoveAllTasks;
} mmtServiceThreadInfo;

/* ------------------------------------------------
 *  aBindData(cmtAny)의 내부 정보를
 *  각 타입에 맞게 DescInfo를 채운다.
 * ----------------------------------------------*/

typedef struct mmtCmsBindProfContext
{
    idvProfDescInfo *mDescInfo;
    UInt             mDescPos;
    UInt             mTotalSize;
    UInt             mMaxDesc;
} mmtCmsBindProfContext;


class mmtServiceThread;

//fix BUG-19464
typedef void (*mmtServiceThreadStartFunc)(mmtServiceThread      *aServiceThread,
                                          mmcServiceThreadType   aServiceType,
                                          UInt                  *aServiceThreadID);


IDE_RC profVariableGetDataCallback(cmtVariable *aVariable,
                                   UInt         aOffset,
                                   UInt         aSize,
                                   UChar       *aData,
                                   void        *aContext);


UInt profWriteBindCallback(idvProfBind     *aBindInfo,
                           idvProfDescInfo *aInfo,
                           UInt             aInfoBegin,
                           UInt             aCurrSize);

IDE_RC profVariableGetDataCallbackA5(cmtVariable *aVariable,
                                   UInt         aOffset,
                                   UInt         aSize,
                                   UChar       *aData,
                                   void        *aContext);


UInt profWriteBindCallbackA5(void            *aBindData,
                           idvProfDescInfo *aInfo,
                           UInt             aInfoBegin,
                           UInt             aInfoCount,
                           UInt             aCurrSize);

#endif
