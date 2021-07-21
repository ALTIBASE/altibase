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
 * $Id: qmcThr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QMC_THR_H_
#define _O_QMC_THR_H_ 1

#include <ide.h>
#include <idTypes.h>

class qmcThrObj;

typedef IDE_RC (*qmcThrFunc)(qmcThrObj* aArg);

class qmcThrObj : public idtBaseThread
{
public:
    iduListNode     mNode;
    iduMutex        mMutex;       /* mCV 와 함께 쓰임 */
    iduCond         mCV;          /* 동기화를 위한 cond var */
    UInt            mStatus;      /* thread 상태를 나타냄 */
    idBool          mStopFlag;    /* thread 멈추기 위한 flag */
    void*           mPrivateArg;  /* 1. PRLQ 고유 data plan */
                                  /* 2. PROJ-2451 DBMS_CONCURRENT_EXEC
                                   *    qscExecInfo */
    qmcThrFunc      mRun;         /* thread 가 실행할 함수 */
    UInt            mID;          /* thread id */
    UInt            mErrorCode;   /* error 가 났을때 set */
    SChar*          mErrorMsg;

    qmcThrObj() : idtBaseThread()
    {
        mNode.mObj = this;
    }

    virtual IDE_RC  initializeThread();
    virtual void    run();
    virtual void    finalizeThread();
};

typedef struct qmcThrMgr
{
    UInt        mThrCnt;
    iduList     mUseList;
    iduList     mFreeList;
    iduMemory * mMemory;   /* original stmt->qmxMem */
} qmcThrMgr;

/* qmcThrObj.mStatus */
#define QMC_THR_STATUS_NONE           (0) /* thread start 이전 */
#define QMC_THR_STATUS_CREATED        (1) /* thread start 직후 wait 직전 */
#define QMC_THR_STATUS_WAIT           (2) /* thread start 후 대기 */
#define QMC_THR_STATUS_RUN            (3) /* thread 작업 중 */
#define QMC_THR_STATUS_END            (4) /* thread 종료 (error 발생 or stopflag) */
#define QMC_THR_STATUS_JOINED         (5) /* thread 종료, join 완료 */
#define QMC_THR_STATUS_FAIL           (6) /* thread start 실패 */
#define QMC_THR_STATUS_RUN_FOR_JOIN   (7) /* thread join하기 위해 작업 중 ( BUG-41627 ) */

/* qmcThrObj.mErrorCode */
#define QMC_THR_ERROR_CODE_NONE (0)

IDE_RC qmcThrObjCreate(qmcThrMgr* aMgr, UInt aReserveCnt);
IDE_RC qmcThrObjFinal(qmcThrMgr* aMgr);

IDE_RC qmcThrGet(qmcThrMgr  * aMgr,
                 qmcThrFunc   aFunc,
                 void       * aPrivateArg,
                 qmcThrObj ** aThrObj);
void qmcThrReturn(qmcThrMgr* aMgr, qmcThrObj* aThrObj);

IDE_RC qmcThrWakeup(qmcThrObj* aThrObj, idBool* aIsSuccess);
IDE_RC qmcThrWakeupForJoin(qmcThrObj* aThrObj, idBool* aIsSuccess);   /* BUG-41627 */
IDE_RC qmcThrJoin(qmcThrMgr* aMgr);

#endif /* _O_QMC_THR_H_ */
