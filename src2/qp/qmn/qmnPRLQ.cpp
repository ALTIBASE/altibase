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
 * $Id: qmnPRLQ.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qmnPRLQ.h>
#include <qmnScan.h>
#include <qmnGroupAgg.h>
#include <qcg.h>
#include <qmcThr.h>

/***********************************************************************
 *
 * Description :
 *    Parallel Queue 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPRLQ::init( qcTemplate * aTemplate,
                      qmnPlan    * aPlan )
{
    qmncPRLQ    * sCodePlan = (qmncPRLQ *) aPlan;
    qmndPRLQ    * sDataPlan =
        (qmndPRLQ *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnPRLQ::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_PRLQ_INIT_DONE_MASK)
         == QMND_PRLQ_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sDataPlan) != IDE_SUCCESS );
    }

    // init child node
    if( aPlan->left != NULL )
    {
        sDataPlan->mCurrentNode = aPlan->left;
        IDE_TEST( sDataPlan->mCurrentNode->init( aTemplate,
                                                 sDataPlan->mCurrentNode )
                  != IDE_SUCCESS);
    }
    else
    {
        sDataPlan->mCurrentNode = NULL;
    }

    /* sDataPlan->mThrArg 는 위에 firstInit 에서 초기화 되었음 */
    sDataPlan->doIt         = qmnPRLQ::doItFirst;
    sDataPlan->mAllRowRead  = 0;
    sDataPlan->mThrObj     = NULL;
    sDataPlan->plan.mTID    = QMN_PLAN_INIT_THREAD_ID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    PRLQ node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPRLQ::firstInit( qcTemplate * aTemplate,
                           qmndPRLQ   * aDataPlan )
{
    iduMemory * sMemory = NULL;
    iduMemory * sPRLQMemory = NULL;
    iduMemory * sPRLQCacheMemory = NULL;
    qcTemplate* sTemplate = NULL;
    SInt        sStackSize;

    sMemory = aTemplate->stmt->qmxMem;

    // 적합성 검사
    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aDataPlan != NULL );

    //---------------------------------
    // PRLQ Queue 초기화
    //---------------------------------
    aDataPlan->mQueue.mHead = 0;
    aDataPlan->mQueue.mTail = 0;
    aDataPlan->mQueue.mSize = QCU_PARALLEL_QUERY_QUEUE_SIZE;

    IDU_FIT_POINT( "qmnPRLQ::firstInit::alloc::mRecord", 
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(sMemory->alloc(ID_SIZEOF(qmnPRLQRecord) *
                            aDataPlan->mQueue.mSize,
                            (void**)&(aDataPlan->mQueue.mRecord))
             != IDE_SUCCESS);

    //---------------------------------
    // Thread arguemnt 할당 및 초기화
    //---------------------------------
    IDU_FIT_POINT( "qmnPRLQ::firstInit::alloc::mThrArg", 
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( sMemory->alloc( ID_SIZEOF(qmnPRLQThrArg),
                              (void**)&(aDataPlan->mThrArg) )
              != IDE_SUCCESS);

    aDataPlan->mThrArg->mChildPlan  = NULL;
    aDataPlan->mThrArg->mPRLQQueue  = &aDataPlan->mQueue;
    aDataPlan->mThrArg->mAllRowRead = &aDataPlan->mAllRowRead;

    /*
     * --------------------------------------------------------------
     * BUG-38294 print wrong subquery plan when execute parallel query
     *
     * initialize PRLQ template
     * - alloc statement
     * - alloc memory manager
     * - alloc stack
     * --------------------------------------------------------------
     */
    sTemplate = &aDataPlan->mTemplate;

    IDU_FIT_POINT( "qmnPRLQ::firstInit::alloc::Template",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(sMemory->alloc(ID_SIZEOF(qcStatement),
                            (void**)&sTemplate->stmt)
             != IDE_SUCCESS);

    /* worker thread 에서 사용할 QMX, QXC memory 생성 */
    IDE_TEST( qcg::allocPRLQExecutionMemory( aTemplate->stmt,
                                             &sPRLQMemory,
                                             &sPRLQCacheMemory )     // PROJ-2452
              != IDE_SUCCESS);

    /*
     * 새로 생성한 메모리를 사용해 worker thread 용 template 을 생성한다.
     * 내부에서 statement 복사와 새 QMX, QXC 메모리 연결 등도 모두 처리한다.
     */
    IDE_TEST( qcg::allocAndCloneTemplate4Parallel( sPRLQMemory,
                                                   sPRLQCacheMemory, // PROJ-2452
                                                   aTemplate,
                                                   sTemplate )
              != IDE_SUCCESS );

    // PROJ-2527 WITHIN GROUP AGGR
    // mtcTemplate도 finalize를 호출하게 되었다. 이를 위해서
    // clone한 template를 기록한다.
    IDE_TEST( qcg::addPRLQChildTemplate( aTemplate->stmt,
                                         sTemplate )
              != IDE_SUCCESS );
    
    sStackSize = aTemplate->tmplate.stackCount;

    /* worker thread 용 stack 생성 */
    IDU_FIT_POINT( "qmnPRLQ::firstInit::alloc::Stack",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(sPRLQMemory->alloc(ID_SIZEOF(mtcStack) * sStackSize,
                                (void**)&(sTemplate->tmplate.stackBuffer))
             != IDE_SUCCESS);

    sTemplate->tmplate.stack       = sTemplate->tmplate.stackBuffer;
    sTemplate->tmplate.stackCount  = sStackSize;
    sTemplate->tmplate.stackRemain = sStackSize;

    aDataPlan->mTemplateCloned = ID_FALSE;

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------
    *aDataPlan->flag &= ~QMND_PRLQ_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_PRLQ_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    PRLQ의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/
IDE_RC qmnPRLQ::doIt( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * aFlag )
{
    qmncPRLQ * sCodePlan = (qmncPRLQ*)aPlan;
    qmndPRLQ * sDataPlan = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);
    UInt       sThrStatus;

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    if ( ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE ) &&
         ( ( *aFlag & QMC_ROW_QUEUE_EMPTY_MASK ) == QMC_ROW_QUEUE_EMPTY_TRUE ) &&
         ( sDataPlan->mThrObj != NULL ) )
    {
        if (sCodePlan->mParallelType == QMNC_PRLQ_PARALLEL_TYPE_SCAN)
        {
            /*
             * non-partition 인 경우
             * 마지막 doIt 직후 thread 반환
             */
            sThrStatus = acpAtomicGet32(&sDataPlan->mThrObj->mStatus);
            while (sThrStatus == QMC_THR_STATUS_RUN)
            {
                idlOS::thr_yield();
                sThrStatus = acpAtomicGet32(&sDataPlan->mThrObj->mStatus);
            }
            qmcThrReturn(aTemplate->stmt->mThrMgr, sDataPlan->mThrObj);
        }
        else
        {
            /*
             * partition 인 경우 (PPCRD 아래 PRLQ 가 있는 경우)
             *
             * PRLQ 의 child 가 1 개 이상일 경우 thread 를 재사용하기 위해
             * 여기서 thread 반환하지 않는다.
             * 대신 PRLQ 를 사용하는 노드(multi children 을 갖는 노드)가
             * PRLQ 의 thread 를 반환해야 한다. (qmnPRLQ::returnThread)
             */
        }

        // Thread 가 재사용될 수 있으므로 초기화 해준다.
        acpAtomicSet32(&sDataPlan->mAllRowRead, 0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Child에 대하여 padNull()을 호출한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPRLQ::padNull( qcTemplate* aTemplate, qmnPlan* aPlan )
{
    qmncPRLQ * sCodePlan = (qmncPRLQ *) aPlan;
    qmndPRLQ * sDataPlan =
        (qmndPRLQ *) (aTemplate->tmplate.data + aPlan->offset);

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_PRLQ_INIT_DONE_MASK)
         == QMND_PRLQ_INIT_DONE_FALSE )
    {
        // 초기화되지 않은 경우 초기화 수행
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    IDE_DASSERT( sDataPlan->mCurrentNode != NULL );

    // Child Plan에 대하여 Null Padding수행
    return sDataPlan->mCurrentNode->padNull(aTemplate, sDataPlan->mCurrentNode);

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PROJ-2402 Parallel Table Scan
 *
 * doItFirst() 에서 수행하는 작업 앞부분을 수행
 * thread 를 시작시키고 하위 SCAN 의 startIt 이 완료될때까지 대기
 * ------------------------------------------------------------------
 */
IDE_RC qmnPRLQ::startIt(qcTemplate* aTemplate, qmnPlan* aPlan, UInt* aTID)
{
    qmndPRLQ * sDataPlan;
    qmnPlan  * sChildCodePlan;
    iduMemory* sMemory;
    idBool     sIsSuccess;
    UInt       i;

    sDataPlan      = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);
    sChildCodePlan = (qmnPlan *)sDataPlan->mCurrentNode;
    sMemory        = aTemplate->stmt->qmxMem;

    //---------------------------------
    // Template 복사 (BUG-38294)
    //---------------------------------
    // 실행 전 worker thread 에서 사용할 template 을 clone 한다.

    if (sDataPlan->mTemplateCloned == ID_FALSE)
    {
        IDE_TEST(qcg::cloneTemplate4Parallel(sMemory,
                                             aTemplate,
                                             &sDataPlan->mTemplate)
                 != IDE_SUCCESS);
        sDataPlan->mTemplateCloned = ID_TRUE;
    }
    else
    {
        /* nothing to do */
    }

    /*
     * ----------------------------------------------------------------
     * Thead manager 로 부터 thread 얻어옴
     * ----------------------------------------------------------------
     */

    sDataPlan->mThrArg->mChildPlan = sChildCodePlan;
    sDataPlan->mThrArg->mTemplate  = &sDataPlan->mTemplate;
    sDataPlan->mThrObj            = NULL;

    /* PROJ-2464 hybrid partitioned table 지원
     *  - HPT 환경을 고려해서 Child 실행 시에 Memory, Disk Type를 판단하고 동작을 결정하도록 한다.
     */
    if ( sDataPlan->mExistDisk == ID_TRUE )
    {
        IDU_FIT_POINT( "qmnPRLQ::startIt::alloc::mRowBuffer",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(sMemory->alloc(ID_SIZEOF(void*) * sDataPlan->mQueue.mSize,
                                (void**)&sDataPlan->mThrArg->mRowBuffer)
                 != IDE_SUCCESS);

        for (i = 0; i < sDataPlan->mQueue.mSize; i++)
        {
            // qmc::setRowSize 에서처럼 cralloc 해야 한다.
            // 그렇지 않으면 일부 type 의 readRow 시 잘못된 값을 읽을 수 있다.
            IDU_FIT_POINT( "qmnPRLQ::startIt::cralloc::mRowBufferItem",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( sMemory->cralloc( sDataPlan->mDiskRowOffset,
                                        (void **) & (sDataPlan->mThrArg->mRowBuffer[i]) )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDU_FIT_POINT("qmnPRLQ::startIt::qmcThrGet::Disk::ERR_THREAD");
    IDE_TEST(qmcThrGet(aTemplate->stmt->mThrMgr,
                       runChildEnqueue,
                       (void*)sDataPlan->mThrArg,
                       &sDataPlan->mThrObj)
             != IDE_SUCCESS);

    if (sDataPlan->mThrObj != NULL)
    {
        // Thread 획득에 성공
        IDE_TEST(qmcThrWakeup(sDataPlan->mThrObj, &sIsSuccess) != IDE_SUCCESS);
        IDE_TEST_RAISE(sIsSuccess == ID_FALSE, ERR_IN_THREAD);

        /* PPCRD or PSCRD */
        sDataPlan->doIt = qmnPRLQ::doItNextDequeue;

        // Plan 출력을 위해 thread id 를 저장한다.
        sDataPlan->plan.mTID = sDataPlan->mThrObj->mID;

        /* child node (SCAN) 의 startIt 이 끝날때까지 대기 */
        while (acpAtomicGet32(&sDataPlan->mAllRowRead) == 0)
        {
            idlOS::thr_yield();
        }
    }
    else
    {
        // Thread 획득에 실패
        sDataPlan->doIt = qmnPRLQ::doItNextSerial;

        // Plan 출력을 위해 thread id 를 저장한다.
        // Thread 를 할당받지 못하면 TID 는 QMN_PLAN_INIT_THREAD_ID
        sDataPlan->plan.mTID = QMN_PLAN_INIT_THREAD_ID;
    }

    if (aTID != NULL)
    {
        *aTID = sDataPlan->plan.mTID;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_IN_THREAD)
    {
        while (sDataPlan->mThrObj->mErrorCode == QMC_THR_ERROR_CODE_NONE)
        {
            idlOS::thr_yield();
        }
        IDE_SET(ideSetErrorCodeAndMsg(sDataPlan->mThrObj->mErrorCode,
                                      sDataPlan->mThrObj->mErrorMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    PRLQ 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPRLQ::printPlan( qcTemplate   * aTemplate,
                           qmnPlan      * aPlan,
                           ULong          aDepth,
                           iduVarString * aString,
                           qmnDisplay     aMode )
{
    qmncPRLQ  * sCodePlan;
    qmndPRLQ  * sDataPlan;
    qcTemplate* sTemplate;
    UInt        sTID;

    sCodePlan = (qmncPRLQ*)aPlan;
    sDataPlan = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    //----------------------------
    // Display 위치 결정
    //----------------------------
    qmn::printSpaceDepth(aString, aDepth);
    iduVarStringAppendLength( aString, "PARALLEL-QUEUE", 14 );

    if ((*sDataPlan->flag & QMND_PRLQ_INIT_DONE_MASK) ==
        QMND_PRLQ_INIT_DONE_TRUE)
    {
        sTID = sDataPlan->plan.mTID;
    }
    else
    {
        sTID = 0;
    }

    if (sTID == 0)
    {
        /*
         * data plan 초기화가 안되어있거나 serial 로 실행된 경우
         * 실제 사용된 template 은 original template
         */
        sTemplate = aTemplate;
    }
    else
    {
        sTemplate = &sDataPlan->mTemplate;
    }

    //----------------------------
    // Thread ID
    //----------------------------
    if( aMode == QMN_DISPLAY_ALL )
    {
        if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
        {
            iduVarStringAppendFormat( aString,
                                      " ( TID: %"ID_UINT32_FMT" )",
                                      sTID );
        }
        else
        {
            iduVarStringAppend( aString, " ( TID: BLOCKED )" );
        }
    }

    iduVarStringAppendLength( aString, "\n", 1 );

    //----------------------------
    // Operator별 결과 정보 출력
    //----------------------------
    if ( QCU_TRCLOG_RESULT_DESC == 1 )
    {
        IDE_TEST( qmn::printResult( aTemplate,
                                    aDepth,
                                    aString,
                                    sCodePlan->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( aPlan->left != NULL )
    {
        IDE_TEST( aPlan->left->printPlan( sTemplate,
                                          aPlan->left,
                                          aDepth + 1,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }
    else
    {
        // Multi children 을 가지는 PRLQ (PPCRD 하위에 올 경우)
        // 출력하지 않는다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPRLQ::doItDefault( qcTemplate * /* aTemplate */,
                             qmnPlan    * /* aPlan */,
                             qmcRowFlag * /* aFlag */ )
{
    /* 이 함수가 수행되면 안됨. */
    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    PRLQ 의 최초 수행 함수
 *    Child를 수행하고 용도에 맞는 수행 함수를 결정한다.
 *    최초 선택된 Row는 반드시 상위 노드로 전달되기 때문에
 *    용도에 맞는 처리가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPRLQ::doItFirst( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
    qmndPRLQ* sDataPlan;

    sDataPlan = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST(startIt(aTemplate, aPlan, NULL) != IDE_SUCCESS);

    // Queue 에서 첫번째 row 를 가져온다.
    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *    PRLQ 의 doIt 실행 함수
 *    Thread 를 얻지 못했을 때 사용된다.
 *
 * Implementation :
 *    하위 노드의 doIt 함수를 호출한다.
 *
 ***********************************************************************/
IDE_RC qmnPRLQ::doItNextSerial( qcTemplate* aTemplate,
                                qmnPlan   * aPlan,
                                qmcRowFlag* aFlag )
{
    qmndPRLQ * sDataPlan        = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);
    qmndPlan * sChildDataPlan   = (qmndPlan*)(aTemplate->tmplate.data + sDataPlan->mCurrentNode->offset);

    IDE_DASSERT( sDataPlan->mCurrentNode != NULL );

    // Child를 수행
    IDE_TEST( sDataPlan->mCurrentNode->doIt( aTemplate,
                                             sDataPlan->mCurrentNode,
                                             aFlag ) != IDE_SUCCESS );

    if( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        // record가 없는 경우
        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnPRLQ::doItFirst;
    }
    else
    {
        // PROJ-2444
        // PRLQ 에 저장후 상위 플랜에서 PRLQ 의 데이터를 직접 읽는다.
        sDataPlan->mRid = sChildDataPlan->myTuple->rid;
        sDataPlan->mRow = sChildDataPlan->myTuple->row;
    }

    // PPCRD 에서 serial 실행일 경우 바로 다른 partition 을 실행하도록
    // flag 를 설정한다.
    *aFlag &= ~QMC_ROW_QUEUE_EMPTY_MASK;
    *aFlag |= QMC_ROW_QUEUE_EMPTY_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PPCRD or PSCRD 아래 PRLQ 가 왔을때의 doIt
 *
 * 상위 노드가 보다 유연하게 레코드를 가져가게 하기 위해
 * 도중에 queue 가 비어있을 경우 대기하지 않고
 * DATA_NONE + QUEUE_EMPTY 를 반환한다.
 * ------------------------------------------------------------------
 */
IDE_RC qmnPRLQ::doItNextDequeue( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag )
{
    qmndPRLQ * sDataPlan;
    UInt       sSleep;
    UInt       sAllRowRead;

    sDataPlan = (qmndPRLQ *) (aTemplate->tmplate.data + aPlan->offset);

    sSleep = 1;

    while (1)
    {
        // 다음 Record를 읽는다.
        if (dequeue( &sDataPlan->mQueue,
                     &sDataPlan->mRid,
                     &sDataPlan->mRow ) == ID_TRUE)
        {
            // queue 에서 row 를 한건 꺼내왔다.
            *aFlag = QMC_ROW_DATA_EXIST;
            break;
        }
        else
        {
            // nothing to do
        }

        sAllRowRead = sDataPlan->mAllRowRead;

        if (sAllRowRead == 0)
        {
            // mAllRowRead 가 0: 하위 노드 initialize 중 이거나
            // doItFirst 실행 중 queue 가 비어있지만 아직 row 를
            // 읽고 있는 중이므로 계속 재시도한다.
            // loop 를 계속 돈다.
            acpSleepUsec(sSleep);
            sSleep = (sSleep < QCU_PARALLEL_QUERY_QUEUE_SLEEP_MAX) ?
                (sSleep << 1) : 1;
        }
        else if (sAllRowRead == 1)
        {
            // mAllRowRead 가 1: row 를 읽고 있는 중
            // queue 가 비어있지만 아직 row 를 읽고 있는 중이다.
            // 상위 PPCRD 에서 다음 partition 을 읽도록 결과를 리턴한다.
            *aFlag = QMC_ROW_DATA_NONE | QMC_ROW_QUEUE_EMPTY_FALSE;
            break;
        }
        else if (sAllRowRead == 2)
        {
            // BUG-40598 컴파일러에 따라 결과가 틀림
            // acpAtomicGet32 를 이용하여 한번더 체크함
            sAllRowRead = acpAtomicGet32( &sDataPlan->mAllRowRead );

            if ( sAllRowRead != 2 )
            {
                continue;
            }
            else
            {
                // nothing to do
            }

            /*
             * 직전에 dequeue 실패했지만,
             * 순간적으로 enqueue 가 되고 AllRowRead = 2 가 되었을수도 있다.
             * 다시 확인해야함
             */
            if (dequeue( &sDataPlan->mQueue,
                         &sDataPlan->mRid,
                         &sDataPlan->mRow ) == ID_TRUE)
            {
                // queue 에서 row 를 한건 꺼내왔다.
                *aFlag = QMC_ROW_DATA_EXIST;
                break;
            }
            else
            {
                // mAllRowRead 가 2: row 다 읽었음
                // row 를 다 읽었고, queue 도 비어있는 상태이다.
                // 더이상 읽을 row 가 없다.
                *aFlag = QMC_ROW_DATA_NONE | QMC_ROW_QUEUE_EMPTY_TRUE;
                break;
            }
        }
        else
        {
            // Thread 에서 에러 발생
            IDE_RAISE(ERR_IN_THREAD);
        }
    }

    if ( ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE ) &&
         ( (*aFlag & QMC_ROW_QUEUE_EMPTY_MASK) == QMC_ROW_QUEUE_EMPTY_TRUE ) )
    {
        // record가 없는 경우
        // 다음 수행을 위해 재수행 함수로 설정함.
        sDataPlan->doIt = qmnPRLQ::doItResume;
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_IN_THREAD)
    {
        while (sDataPlan->mThrObj->mErrorCode == QMC_THR_ERROR_CODE_NONE)
        {
            idlOS::thr_yield();
        }
        IDE_SET(ideSetErrorCodeAndMsg(sDataPlan->mThrObj->mErrorCode,
                                      sDataPlan->mThrObj->mErrorMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *    PRLQ 의 하위노드 변경 후 첫 수행 함수
 *    Thread 에 변경할 하위 노드를 지정하고 시작하도록 한다.
 *
 *    partition 하나 처리할때마다 threadGet 하지 않기 위해
 *    PRLQ 가 하나의 scan 을 끝내고 반환되지 않고 다른 scan 을 실행한다.
 *
 *    PPCRD 아래의 아직 assign 안된 다른 scan 으로 교체될때 호출된다.
 *    partition table 인 경우만 해당
 ***********************************************************************/
IDE_RC qmnPRLQ::doItResume( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag )
{
    qmndPRLQ * sDataPlan;
    qmnPlan  * sChildCodePlan;
    idBool     sIsSuccess;

    sDataPlan       = (qmndPRLQ*)(aTemplate->tmplate.data
                                  + aPlan->offset);
    sChildCodePlan  = (qmnPlan *)sDataPlan->mCurrentNode;

    IDE_DASSERT(sDataPlan->mThrObj != NULL);

    sDataPlan->doIt = qmnPRLQ::doItNextDequeue;

    /*
     * ----------------------------------------------------------------
     * thead 에 새로운 하위 노드를 전달, 시작한다.
     * ----------------------------------------------------------------
     */

    // Template, Queue, AllRowRead, row buffer 등은 그대로 사용한다.
    // 하위 노드를 변경한다.
    sDataPlan->mThrArg->mChildPlan = sChildCodePlan;

    IDE_TEST(qmcThrWakeup(sDataPlan->mThrObj, &sIsSuccess) != IDE_SUCCESS);
    IDE_TEST_RAISE(sIsSuccess == ID_FALSE, ERR_IN_THREAD);

    // Queue 에서 첫번째 row 를 가져온다.
    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_IN_THREAD)
    {
        while (sDataPlan->mThrObj->mErrorCode == QMC_THR_ERROR_CODE_NONE)
        {
            idlOS::thr_yield();
        }
        IDE_SET(ideSetErrorCodeAndMsg(sDataPlan->mThrObj->mErrorCode,
                                      sDataPlan->mThrObj->mErrorMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPRLQ::runChildEnqueue( qmcThrObj * aThrObj )
{
    qcTemplate      * sTemplate;
    qmcRowFlag        sFlag = QMC_ROW_INITIALIZE;
    qmnPRLQThrArg   * sPRLQThrArg;

    qmnPlan         * sChildPlan;
    qmndPlan        * sChildDataPlan;
    qmnPRLQQueue    * sParallelQueue;
    void            * sOldChildTupleRow = NULL;
    UInt              sSleep;
    UInt              sQueueIdx;

    sPRLQThrArg    = (qmnPRLQThrArg*)aThrObj->mPrivateArg;
    // BUG-38294
    // Argument 로 template 을 전달받아 사용한다.
    sTemplate      = sPRLQThrArg->mTemplate;
    sChildPlan     = sPRLQThrArg->mChildPlan;
    sParallelQueue = sPRLQThrArg->mPRLQQueue;

    IDE_DASSERT( sChildPlan != NULL );

    sChildDataPlan = (qmndPlan*)(sTemplate->tmplate.data +
                                 sChildPlan->offset);

    // PROJ-2444 Parallel Aggreagtion
    IDE_TEST( sChildPlan->readyIt( sTemplate,
                                   sChildPlan,
                                   aThrObj->mID )
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - HPT 환경을 고려해서 Child 실행 시에 Memory, Disk Type를 판단하고 동작을 결정하도록 한다.
     */
    if ( ( sChildPlan->flag & QMN_PLAN_STORAGE_MASK ) == QMN_PLAN_STORAGE_DISK )
    {
        sOldChildTupleRow = sChildDataPlan->myTuple->row;

        // 자식 플랜의 myTuple->row 의 포인터를 이용하여
        // rowBuffer 을 변경한다.
        sChildDataPlan->myTuple->row = sPRLQThrArg->mRowBuffer[0];

        sQueueIdx = 0;
    }
    else
    {
        /* Nothing to do */
    }

    sSleep    = 1;

    (void)acpAtomicSet32(sPRLQThrArg->mAllRowRead, 1);

    // Child를 수행
    IDE_TEST( sChildPlan->doIt( sTemplate,
                                sChildPlan,
                                &sFlag )
              != IDE_SUCCESS );

    while (aThrObj->mStopFlag == ID_FALSE)
    {
        if ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST)
        {
            while (aThrObj->mStopFlag == ID_FALSE)
            {
                if (enqueue( sParallelQueue,
                             sChildDataPlan->myTuple->rid,
                             sChildDataPlan->myTuple->row) == ID_TRUE)
                {
                    /* PROJ-2464 hybrid partitioned table 지원
                     *  - HPT 환경을 고려해서 Child 실행 시에 Memory, Disk Type를 판단하고 동작을 결정하도록 한다.
                     */
                    if ( ( sChildPlan->flag & QMN_PLAN_STORAGE_MASK ) == QMN_PLAN_STORAGE_DISK )
                    {
                        /* row buffer 바꿔치기 */
                        sQueueIdx = ((sQueueIdx + 1) < sParallelQueue->mSize) ?
                            (sQueueIdx + 1) : 0;

                        // PROJ-2444 Parallel Aggreagtion
                        sChildDataPlan->myTuple->row = sPRLQThrArg->mRowBuffer[sQueueIdx];
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    sSleep = 1;
                    break;
                }
                else
                {
                    /* enqueue fail => wait for a while */

                    acpSleepUsec(sSleep);
                    sSleep = (sSleep < QCU_PARALLEL_QUERY_QUEUE_SLEEP_MAX) ?
                        (sSleep << 1) : 1;
                }
            }
        }
        else
        {
            // 모든 row read 완료
            acpAtomicSet32(sPRLQThrArg->mAllRowRead, 2);
            break;
        }

        // Child를 수행
        IDE_TEST( sChildPlan->doIt( sTemplate,
                                    sChildPlan,
                                    &sFlag )
                  != IDE_SUCCESS );
    }

    if ( ( sChildPlan->flag & QMN_PLAN_STORAGE_MASK ) == QMN_PLAN_STORAGE_DISK )
    {
        sChildDataPlan->myTuple->row = sOldChildTupleRow;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // Set error code to thread argument
    aThrObj->mErrorCode = ideGetErrorCode();
    aThrObj->mErrorMsg  = ideGetErrorMsg();
    (void)acpAtomicSet32(sPRLQThrArg->mAllRowRead, 3);

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PPCRD 로부터 호출됨
 * PRLQ 에 실행해야 할 SCAN 을 연결
 * ------------------------------------------------------------------
 */
void qmnPRLQ::setChildNode( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,         /* PRLQ */
                            qmnPlan    * aChildPlan )   /* SCAN */
{
    qmndPRLQ * sDataPlan = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);

    // 1. 하위 노드 변경
    sDataPlan->mCurrentNode = aChildPlan;
}

/***********************************************************************
 *
 * Description :
 *    PRLQ 의 thread 를 반환한다.
 *    현재 PRLQ 의 모든 사용이 끝났을때 PPCRD 로부터 호출된다.
 *
 * Implementation :
 *    Thread 가 wait 상태가 될 때까지 기다린 후 반환한다.
 *
 ***********************************************************************/
void qmnPRLQ::returnThread( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    qmndPRLQ* sDataPlan;

    sDataPlan = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);

    IDE_DASSERT( sDataPlan->mThrObj != NULL );
    IDE_DASSERT(((qmncPRLQ*)aPlan)->mParallelType ==
                QMNC_PRLQ_PARALLEL_TYPE_PARTITION)

    while (acpAtomicGet32(&sDataPlan->mThrObj->mStatus) ==
           QMC_THR_STATUS_RUN)
    {
        idlOS::thr_yield();
    }

    qmcThrReturn(aTemplate->stmt->mThrMgr, sDataPlan->mThrObj);

    acpAtomicSet32(&sDataPlan->mAllRowRead, 0);
}

/*
 * ------------------------------------------------------------------
 * BUG-38024 print wrong subquery plan when execute parallel query
 *
 * return the PRLQ's own template
 * if data plan is not initialized, return the original template
 * this function is called by qmnPPCRD::printPlan()
 * ------------------------------------------------------------------
 */
qcTemplate* qmnPRLQ::getTemplate(qcTemplate* aTemplate, qmnPlan* aPlan)
{
    qmncPRLQ  * sCodePlan;
    qmndPRLQ  * sDataPlan;
    qcTemplate* sResult;

    sCodePlan = (qmncPRLQ*)aPlan;
    sDataPlan = (qmndPRLQ*)(aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    if ((*sDataPlan->flag & QMND_PRLQ_INIT_DONE_MASK) ==
        QMND_PRLQ_INIT_DONE_FALSE)
    {
        sResult = aTemplate;
    }
    else
    {
        if (sDataPlan->mTemplateCloned == ID_FALSE)
        {
            sResult = aTemplate;
        }
        else
        {
            if (sDataPlan->plan.mTID == QMN_PLAN_INIT_THREAD_ID)
            {
                /* serial execution */
                sResult = aTemplate;
            }
            else
            {
                sResult = &sDataPlan->mTemplate;
            }
        }
    }
    return sResult;
}

/***********************************************************************
 *
 * Description : PROJ-2464 hybrid partitioned table 지원
 *
 *    PRLQ node의 Data 영역의 추가 정보에 대한 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
void qmnPRLQ::setPRLQInfo( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,         /* PRLQ */
                           idBool       aExistDisk,      /* INFO 1 */
                           UInt         aDiskRowOffset ) /* INFO 2 */
{
    qmndPRLQ * sDataPlan = (qmndPRLQ *)(aTemplate->tmplate.data + aPlan->offset);

    /* INFO 1 */
    sDataPlan->mExistDisk = aExistDisk;

    /* INFO 2 */
    /* BUG-43403 mExistDisk를 설정할 때, Disk Row Offset도 설정한다. */
    sDataPlan->mDiskRowOffset = aDiskRowOffset;
}
