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
 * $Id: qmcThr.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <ideErrorMgr.h>
#include <iduMemory.h>
#include <qcg.h>
#include <qmcThr.h>

/*
 * ------------------------------------------------------------------
 * PROJ-1071 Parallel Query
 * ------------------------------------------------------------------
 * thread manager 관련 interface
 *
 * - qmcThrObjCreate()
 * - qmcThrObjFinal()
 * - qmcThrGet()
 * - qmcThrReturn()
 * - qmcThrWakeup()
 * - qmcThrJoin()
 * ------------------------------------------------------------------
 */
static IDE_RC qmcThrJoinInternal(qmcThrObj* aThrObj);

static IDE_RC qmcThrObjCreateInternal(qmcThrMgr * aMgr,
                                      qmcThrObj** aThrObj,
                                      UInt        aThrID)
{
    IDE_RC      sRc;
    qmcThrObj* sThrObj;

    /*
     * thrInfo create and init
     */
    IDU_FIT_POINT( "qmcThr::qmcThrObjCreateInternal::alloc::sThrObj",
                    idERR_ABORT_InsufficientMemory );

    sRc = aMgr->mMemory->alloc(ID_SIZEOF(qmcThrObj), (void**)&sThrObj);
    IDE_TEST(sRc != IDE_SUCCESS);
    sThrObj = new(sThrObj) qmcThrObj;

    sThrObj->mPrivateArg   = NULL;
    sThrObj->mStopFlag     = ID_TRUE;
    sThrObj->mStatus       = QMC_THR_STATUS_NONE;
    sThrObj->mID           = aThrID;
    sThrObj->mErrorCode    = QMC_THR_ERROR_CODE_NONE;
    sThrObj->mErrorMsg     = NULL;

    *aThrObj = sThrObj;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * reserve cnt 만큼 thread info 할당 및 초기화
 * called by top projection firstInit()
 *
 * thread 생성은 여기서 하지 않고 qmcThrGet() 에서 한다.
 * ------------------------------------------------------------------
 */
IDE_RC qmcThrObjCreate(qmcThrMgr* aMgr, UInt aReserveCnt)
{
    IDE_RC      sRc;
    UInt        i;
    qmcThrObj* sThrObj;

    IDE_DASSERT(aMgr != NULL);
    IDE_DASSERT(aReserveCnt >= 1);

    for (i = 0; i < aReserveCnt; i++)
    {
        sRc = qmcThrObjCreateInternal(aMgr, &sThrObj, aMgr->mThrCnt + i + 1);
        IDE_TEST(sRc != IDE_SUCCESS);
        
        IDU_LIST_ADD_LAST(&aMgr->mFreeList, &(sThrObj->mNode));
    }

    aMgr->mThrCnt += aReserveCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (IDU_LIST_IS_EMPTY(&aMgr->mFreeList) != ID_TRUE)
    {
        iduListNode* sIter;
        iduListNode* sIterNext;

        IDU_LIST_ITERATE_SAFE(&aMgr->mFreeList, sIter, sIterNext)
        {
            IDU_LIST_REMOVE(sIter);
            (void)qmcThrJoinInternal((qmcThrObj*)(sIter->mObj));
        }
    }

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * thread manager 안에 있는 thread info 해제
 * ------------------------------------------------------------------
 */
IDE_RC qmcThrObjFinal(qmcThrMgr* aMgr)
{
    UInt         i;
    iduListNode* sIter;
    iduListNode* sIterNext;
    IDE_RC       sRc;

    i = 0;
    IDU_LIST_ITERATE_SAFE(&aMgr->mUseList, sIter, sIterNext)
    {
        IDU_LIST_REMOVE(sIter);

        // BUG-40148, BUG-40860
        // Ignore an error from qmcThrJoinInternal except system errors.
        sRc = qmcThrJoinInternal((qmcThrObj*)(sIter->mObj));
        IDE_TEST((sRc != IDE_SUCCESS) && (ideIsIgnore() != IDE_SUCCESS));
        i++;
    }

    IDU_LIST_ITERATE_SAFE(&aMgr->mFreeList, sIter, sIterNext)
    {
        IDU_LIST_REMOVE(sIter);

        // BUG-40148, BUG-40860
        // Ignore an error from qmcThrJoinInternal except system errors.
        sRc = qmcThrJoinInternal((qmcThrObj*)(sIter->mObj));
        IDE_TEST((sRc != IDE_SUCCESS) && (ideIsIgnore() != IDE_SUCCESS));
        i++;
    }
    IDE_DASSERT(i == aMgr->mThrCnt);

    aMgr->mThrCnt = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * qmcThrGet() 함수로부터 호출된다.
 *
 * 실제 thread 생성
 * => qmcThrRun() 함수가 새로운 thread 로 실행된다.
 * ------------------------------------------------------------------
 */
static IDE_RC qmcThrCreate(qmcThrObj* aThrObj)
{
    aThrObj->mStatus = QMC_THR_STATUS_CREATED;

    IDU_FIT_POINT( "qmcThr::qmcThrCreate::thread::start",
                    idERR_ABORT_THR_CREATE_FAILED );

    IDE_TEST(aThrObj->start() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aThrObj->mStatus = QMC_THR_STATUS_FAIL;

    return IDE_FAILURE;
}

static IDE_RC qmcThrJoinInternal(qmcThrObj* aThrObj)
{
    UInt     sStatus;
    idBool   sIsSuccess;

    sStatus = acpAtomicGet32(&aThrObj->mStatus);

    if (sStatus == QMC_THR_STATUS_WAIT)
    {
        /* WAIT */
        aThrObj->mStopFlag = ID_TRUE;

        IDE_TEST(qmcThrWakeupForJoin(aThrObj, &sIsSuccess) != IDE_SUCCESS);
    }
    else
    {
        if (sStatus == QMC_THR_STATUS_RUN)
        {
            aThrObj->mStopFlag = ID_TRUE;

            while (aThrObj->mStatus == QMC_THR_STATUS_RUN)
            {
                idlOS::thr_yield();
            }
            if (aThrObj->mStatus == QMC_THR_STATUS_WAIT)
            {
                /* WAIT */
                IDE_TEST(qmcThrWakeupForJoin(aThrObj, &sIsSuccess) != IDE_SUCCESS);
            } 
            else
            {
                /* END */
            }
        }
        else
        {
            /* NONE or END or JOINED or FAIL */
        }
    }

    if ((aThrObj->mStatus != QMC_THR_STATUS_NONE) &&
        (aThrObj->mStatus != QMC_THR_STATUS_JOINED) &&
        (aThrObj->mStatus != QMC_THR_STATUS_FAIL))
    {
        IDE_TEST(aThrObj->join() != IDE_SUCCESS);
        aThrObj->mStatus = QMC_THR_STATUS_JOINED;
    }
    else
    {
        /* noting to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * 현재 thread manager 가 가지고 있는 모든 thread 를 종료
 * thrInfo 를 free 하지는 않는다 (cursor close 이후에 해야함)
 * ------------------------------------------------------------------
 */
IDE_RC qmcThrJoin(qmcThrMgr* aMgr)
{
    UInt         i;
    iduListNode* sIter;
    iduListNode* sIterNext;
    IDE_RC       sRc;

    i = 0;
    IDU_LIST_ITERATE_SAFE(&aMgr->mUseList, sIter, sIterNext)
    {
        // BUG-40148, BUG-40860
        // Ignore an error from qmcThrJoinInternal except system errors.
        sRc = qmcThrJoinInternal((qmcThrObj*)(sIter->mObj));
        IDE_TEST((sRc != IDE_SUCCESS) && (ideIsIgnore() != IDE_SUCCESS));
        i++;
    }

    IDU_LIST_ITERATE_SAFE(&aMgr->mFreeList, sIter, sIterNext)
    {
        // BUG-40148, BUG-40860
        // Ignore an error from qmcThrJoinInternal except system errors.
        sRc = qmcThrJoinInternal((qmcThrObj*)(sIter->mObj));
        IDE_TEST((sRc != IDE_SUCCESS) && (ideIsIgnore() != IDE_SUCCESS));
        i++;
    }
    IDE_DASSERT(i == aMgr->mThrCnt);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ----------------------------------------------------------------------
 * PRLQ 의 요청에 의하여 free thread 하나를 PRLQ 에 assign
 * qmnPRLQ::doItFirst() 로부터 호출됨
 *
 * 여러개의 PRLQ 가 동시에 thread 를 요청하는 일은 없다.
 * 그런 경우가 생긴다면 mutex lock 이 필요함
 * ----------------------------------------------------------------------
 */
IDE_RC qmcThrGet(qmcThrMgr  * aMgr,
                 qmcThrFunc   aFunc,
                 void       * aPrivateArg,
                 qmcThrObj ** aThrObj)
{
    IDE_RC       sRc;
    UInt         sStatus;
    iduListNode* sNode;
    qmcThrObj * sThrObj;

    if (IDU_LIST_IS_EMPTY(&aMgr->mFreeList) != ID_TRUE)
    {
        sNode = IDU_LIST_GET_FIRST(&aMgr->mFreeList);
        IDU_LIST_REMOVE(sNode);
        IDU_LIST_ADD_LAST(&aMgr->mUseList, sNode);
        sThrObj = (qmcThrObj*)(sNode->mObj);

        sStatus = acpAtomicGet32(&sThrObj->mStatus);
        switch (sStatus)
        {
            case QMC_THR_STATUS_NONE:
                /* thread 사용된적 없음 => 최초 생성 */
                sThrObj->mPrivateArg  = aPrivateArg;
                sThrObj->mStopFlag    = ID_FALSE;
                sThrObj->mRun         = aFunc;
                sRc = qmcThrCreate(sThrObj);
                IDE_TEST(sRc != IDE_SUCCESS);
                break;
            case QMC_THR_STATUS_WAIT:
                /* free thread 재사용 */
                sThrObj->mPrivateArg = aPrivateArg;
                sThrObj->mStopFlag   = ID_FALSE;
                sThrObj->mRun        = aFunc;
                break;
            default:
                IDE_ERROR(0);
        }
    }
    else
    {
        sThrObj = NULL;
    }

    *aThrObj = sThrObj;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * thread 반납
 * use list -> free list 로 옮김
 * ------------------------------------------------------------------
 */
void qmcThrReturn(qmcThrMgr* aMgr, qmcThrObj* aThrObj)
{
    iduListNode* sNode;
    sNode = &(aThrObj->mNode);
    IDU_LIST_REMOVE(sNode);
    IDU_LIST_ADD_FIRST(&aMgr->mFreeList, sNode);
}

/*
 * ------------------------------------------------------------------
 * cond_signal 을 호출해서 thread 를 깨운다.
 *
 * thread status 가 CREATED -> WAIT 로 바뀌기를 기다린다.
 * thread 가 비정상 종료 한 경우 (END) 는
 * signal 을 보낼 수 없으므로 ID_FALSE 를 return
 * ------------------------------------------------------------------
 */
IDE_RC qmcThrWakeup(qmcThrObj* aThrObj, idBool* aIsSuccess)
{
    IDE_RC sRc;
    UInt   sStatus;
    idBool sIsSuccess;

    sIsSuccess = ID_TRUE;

    while (1)
    {
        sStatus = acpAtomicGet32(&aThrObj->mStatus);
        if (sStatus == QMC_THR_STATUS_WAIT)
        {
            sIsSuccess = ID_TRUE;
            break;
        }
        else if ( (sStatus == QMC_THR_STATUS_END)    ||
                  (sStatus == QMC_THR_STATUS_NONE)   ||
                  (sStatus == QMC_THR_STATUS_JOINED) ||
                  (sStatus == QMC_THR_STATUS_FAIL)   ||
                  (sStatus == QMC_THR_STATUS_RUN_FOR_JOIN) )
        {
            sIsSuccess = ID_FALSE;
            goto LABEL_EXIT;
        }
        else
        {
            /*
             * RUN -> WAIT 로 가는 중
             * CREATED -> WAIT 로 가는 중
             *
             * wait for a while and try again
             */
            idlOS::thr_yield();
        }
    }

    sRc = aThrObj->mMutex.lock(NULL);
    IDE_TEST(sRc != IDE_SUCCESS);

    (void)acpAtomicSet32(&aThrObj->mStatus, QMC_THR_STATUS_RUN);

    IDE_TEST(aThrObj->mCV.signal() != IDE_SUCCESS);

    sRc = aThrObj->mMutex.unlock();
    IDE_TEST(sRc != IDE_SUCCESS);

LABEL_EXIT:

    *aIsSuccess = sIsSuccess;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-41627 */
IDE_RC qmcThrWakeupForJoin(qmcThrObj* aThrObj, idBool* aIsSuccess)
{
    IDE_RC sRc;
    UInt   sStatus;
    idBool sIsSuccess;

    sIsSuccess = ID_TRUE;

    while (1)
    {
        sStatus = acpAtomicGet32(&aThrObj->mStatus);
        if (sStatus == QMC_THR_STATUS_WAIT)
        {
            sIsSuccess = ID_TRUE;
            break;
        }
        else if ( (sStatus == QMC_THR_STATUS_END)    ||
                  (sStatus == QMC_THR_STATUS_NONE)   ||
                  (sStatus == QMC_THR_STATUS_JOINED) ||
                  (sStatus == QMC_THR_STATUS_FAIL)   ||
                  (sStatus == QMC_THR_STATUS_RUN_FOR_JOIN) )

        {
            sIsSuccess = ID_FALSE;
            goto LABEL_EXIT;
        }
        else
        {
            /*
             * RUN -> WAIT 로 가는 중
             * CREATED -> WAIT 로 가는 중
             *
             * wait for a while and try again
             */
            idlOS::thr_yield();
        }
    }

    sRc = aThrObj->mMutex.lock(NULL);
    IDE_TEST(sRc != IDE_SUCCESS);

    (void)acpAtomicSet32(&aThrObj->mStatus, QMC_THR_STATUS_RUN_FOR_JOIN);

    IDE_TEST(aThrObj->mCV.signal() != IDE_SUCCESS);

    sRc = aThrObj->mMutex.unlock();
    IDE_TEST(sRc != IDE_SUCCESS);

LABEL_EXIT:

    *aIsSuccess = sIsSuccess;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcThrObj::initializeThread()
{
    IDE_RC sRC;

    /*
     * thrArg create and init
     */

    sRC = mMutex.initialize((SChar*)"QMC_THRARG_MUTEX",
                            IDU_MUTEX_KIND_POSIX,
                            IDV_WAIT_INDEX_NULL);
    IDE_TEST(sRC != IDE_SUCCESS);

    IDE_TEST(mCV.initialize((SChar *)"QMC_THRARG_CV")
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ----------------------------------------------------------------
 * create thread 할때 생성되는 기본 thread 함수
 * argument 안에 run 함수가 존재하고 최종적으로 run 이 호출된다.
 * ----------------------------------------------------------------
 */
void qmcThrObj::run()
{
    IDE_RC     sRc;
    UInt       sStatus = 0;

    while (1)
    {
        sRc = mMutex.lock(NULL);
        IDE_TEST(sRc != IDE_SUCCESS);

        (void)acpAtomicSet32(&mStatus, QMC_THR_STATUS_WAIT);

        /*
         * BUGBUG1071
         * cond_timedwait
         */
        IDE_TEST(mCV.wait(&(mMutex)) != IDE_SUCCESS);

        sStatus   = acpAtomicGet32(&mStatus);

        sRc = mMutex.unlock();
        IDE_TEST(sRc != IDE_SUCCESS);

        if ( (mStopFlag == ID_TRUE) && ( sStatus == QMC_THR_STATUS_RUN_FOR_JOIN) )
        {
            break;
        }
        // BUG-41046
        // Sometimes spurious wake-up could be occurred in multi-threading program.
        else if ( sStatus == QMC_THR_STATUS_WAIT )
        {
            continue;
        }
        else
        {
            // Nothing to do.
        }

        /*
         * 호출되는 함수:
         * qmnPRLQ::runChildEnqueueMemory
         * qmnPRLQ::runChildEnqueueDisk
         *
         * PROJ-2451
         * qsxConcurrentExecute::execute
         */
        IDE_TEST(mRun(this) != IDE_SUCCESS);

        if (mStopFlag == ID_TRUE)
        {
            break;
        }
    }

    (void)acpAtomicSet32(&mStatus, QMC_THR_STATUS_END);

    return;

    IDE_EXCEPTION_END;    

    /* Set error code to thread argument */
    mErrorCode = ideGetErrorCode();
    mErrorMsg  = ideGetErrorMsg();
    (void)acpAtomicSet32(&mStatus, QMC_THR_STATUS_END);

    return;
}

void qmcThrObj::finalizeThread()
{
    (void)mMutex.destroy();
    (void)mCV.destroy();
}

