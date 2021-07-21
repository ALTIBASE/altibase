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
 * $Id: qmnPSCRD.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <qmnPSCRD.h>
#include <qmnPRLQ.h>
#include <qcg.h>
#include <qmoUtil.h>

IDE_RC qmnPSCRD::init(qcTemplate* aTemplate, qmnPlan* aPlan)
{
    qmncPSCRD  * sCodePlan;
    qmndPSCRD  * sDataPlan;
    qmnChildren* sChild;
    qmndPlan   * sChildDataPlan = NULL;
    UInt         sDiskRowOffset = 0;
    UInt         sParallelGroupID;
    idBool       sJudge;
    idBool       sExistDisk = ID_FALSE;

    sCodePlan = (qmncPSCRD*)aPlan;
    sDataPlan = (qmndPSCRD*)(aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    if ((*sDataPlan->flag & QMND_PSCRD_INIT_DONE_MASK) ==
        QMND_PSCRD_INIT_DONE_FALSE)
    {
        IDE_TEST(firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    if (sCodePlan->mConstantFilter != NULL)
    {
        IDE_TEST(qtc::judge(&sJudge,
                            sCodePlan->mConstantFilter,
                            aTemplate)
                 != IDE_SUCCESS);
    }
    else
    {
        sJudge = ID_TRUE;
    }

    if (sJudge == ID_TRUE)
    {
        IDE_TEST(smiPrepareForParallel(aTemplate->stmt->stmtInfo->mSmiStmtForExecute->mTrans,
                                       &sParallelGroupID)
                 != IDE_SUCCESS);

        /* child PRLQ init */
        for (sChild = aPlan->childrenPRLQ; sChild != NULL; sChild = sChild->next)
        {
            IDE_TEST(sChild->childPlan->init(aTemplate,
                                             sChild->childPlan)
                     != IDE_SUCCESS);

            /* PROJ-2464 hybrid partitioned table 지원
             *  - PRLQ의 메모리 할당을 최적화하기 위해서, Disk Partition이 포함되었는지 검사한다.
             *    1. 하부 PLAN에 Disk가 포함되었는지 검사한다.
             *    2. PRLQ에 추가적인 정보를 전달한다.
             */
            /* 1. 하부 PLAN에 Disk가 포함되었는지 검사한다. */
            if ( ( sChild->childPlan->left->flag & QMN_PLAN_STORAGE_MASK ) == QMN_PLAN_STORAGE_DISK )
            {
                sExistDisk     = ID_TRUE;
                sChildDataPlan = (qmndPlan *)(aTemplate->tmplate.data + sChild->childPlan->left->offset);
                sDiskRowOffset = sChildDataPlan->myTuple->rowOffset;
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* 2. PRLQ에 추가적인 정보를 전달한다. */
        for ( sChild = aPlan->childrenPRLQ; sChild != NULL; sChild = sChild->next )
        {
            qmnPRLQ::setPRLQInfo( aTemplate,
                                  sChild->childPlan,
                                  sExistDisk,
                                  sDiskRowOffset );
        }

        // PROJ-2444
        // PSCRD는 하위 SCAN 플랜되 tuple 를 공유한다.
        // PSCRD에서 tuple->row 를 변경하기 때문에 원본을 유지해주어야 한다.
        sDataPlan->mOrgRow = sDataPlan->plan.myTuple->row;
        sDataPlan->doIt    = qmnPSCRD::doItFirst;

    }
    else
    {
        sDataPlan->doIt = qmnPSCRD::doItAllFalse;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPSCRD::firstInit(qcTemplate* aTemplate,
                           qmncPSCRD * aCodePlan,
                           qmndPSCRD * aDataPlan)
{
    qmnChildren* sChild;
    UInt         sCnt;

    /*
     * alloc PRLQ area, set PRLQ cnt
     */
    sCnt = 0;
    for (sChild = aCodePlan->plan.childrenPRLQ;
         sChild != NULL;
         sChild = sChild->next)
    {
        sCnt++;
    }
    aDataPlan->mPRLQCnt = sCnt;
    IDE_DASSERT(sCnt > 0);

    IDU_FIT_POINT("qmnPSCRD::firstInit::alloc",
                  idERR_ABORT_InsufficientMemory);

    IDE_TEST(aTemplate->stmt->qmxMem->alloc(
            sCnt * ID_SIZEOF(qmnPSCRDChildren),
            (void**)&aDataPlan->mChildrenPRLQArea)
        != IDE_SUCCESS);

    /*
     * set plan.myTuple
     */
    aDataPlan->plan.myTuple = &aTemplate->tmplate.rows[aCodePlan->mTupleRowID];

    /*
     * set row size
     */ 
    IDE_TEST(qmc::setRowSize(aTemplate->stmt->qmxMem,
                             &aTemplate->tmplate,
                             aCodePlan->mTupleRowID)
             != IDE_SUCCESS);

    aDataPlan->mNullRow = NULL;
    aDataPlan->mAccessCount = 0;
    aDataPlan->mSerialRemain = ID_FALSE;

    *aDataPlan->flag &= ~QMND_PSCRD_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_PSCRD_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPSCRD::padNull(qcTemplate* aTemplate, qmnPlan* aPlan)
{
    qmncPSCRD * sCodePlan;
    qmndPSCRD * sDataPlan;

    sCodePlan = (qmncPSCRD*)aPlan;
    sDataPlan = (qmndPSCRD*)(aTemplate->tmplate.data + aPlan->offset);

    /* 초기화가 수행되지 않은 경우 초기화를 수행 */
    if ((aTemplate->planFlag[sCodePlan->planID] & QMND_PSCRD_INIT_DONE_MASK) ==
        QMND_PSCRD_INIT_DONE_FALSE)
    {
        IDE_TEST(aPlan->init(aTemplate, aPlan) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    if ((sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK) == QMN_PLAN_STORAGE_DISK)
    {
        /* disk table */

        if (sDataPlan->mNullRow == NULL)
        {
            /* null row를 가져온 적이 없는 경우 */

            IDE_DASSERT(sDataPlan->plan.myTuple->rowOffset > 0);

            IDU_FIT_POINT("qmnPSCRD::padNull::cralloc",
                          idERR_ABORT_InsufficientMemory);

            IDE_TEST(aTemplate->stmt->qmxMem->cralloc(
                    sDataPlan->plan.myTuple->rowOffset,
                    (void**)&sDataPlan->mNullRow) != IDE_SUCCESS);

            IDE_TEST(qmn::makeNullRow(sDataPlan->plan.myTuple,
                                      sDataPlan->mNullRow)
                     != IDE_SUCCESS);

            SMI_MAKE_VIRTUAL_NULL_GRID(sDataPlan->mNullRID);
        }
        else
        {
            /* 이미 null row를 가져왔음. */
            /* nothing to do */
        }

        /* Null Row 복사 */
        idlOS::memcpy(sDataPlan->plan.myTuple->row,
                      sDataPlan->mNullRow,
                      sDataPlan->plan.myTuple->rowOffset);

        /* Null RID의 복사 */
        idlOS::memcpy(&sDataPlan->plan.myTuple->rid,
                      &sDataPlan->mNullRID,
                      ID_SIZEOF(scGRID));
    }
    else
    {
        /* Memory Table인 경우 */

        /* null row 를 가져온다. */
        IDE_TEST(smiGetTableNullRow( sCodePlan->mTableHandle,
                                     (void **)&sDataPlan->plan.myTuple->row,
                                     &sDataPlan->plan.myTuple->rid )
                 != IDE_SUCCESS);

        IDE_DASSERT(sDataPlan->plan.myTuple->row != NULL);

    }

    sDataPlan->mAccessCount++;
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPSCRD::printPlan(qcTemplate  * aTemplate,
                           qmnPlan     * aPlan,
                           ULong         aDepth,
                           iduVarString* aString,
                           qmnDisplay    aMode)
{
    qmncPSCRD  * sCodePlan;
    qmndPSCRD  * sDataPlan;
    qmnChildren* sChildren;

    sCodePlan = (qmncPSCRD*)aPlan;
    sDataPlan = (qmndPSCRD*)(aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    qmn::printSpaceDepth(aString, aDepth);

    iduVarStringAppendLength(aString,
                             "PARALLEL-SCAN-COORDINATOR ( TABLE: ",
                             35);

    /*
     * ----------------------------------------------------
     * table name 출력
     * ----------------------------------------------------
     */
    if ((sCodePlan->mTableOwnerName.name != NULL) &&
        (sCodePlan->mTableOwnerName.size > 0))
    {
        iduVarStringAppendLength( aString,
                                  sCodePlan->mTableOwnerName.name,
                                  sCodePlan->mTableOwnerName.size );
        iduVarStringAppendLength(aString, ".", 1);
    }
    else
    {
        /* nothing to do */
    }

    if ( ( sCodePlan->mTableName.size <= QC_MAX_OBJECT_NAME_LEN ) &&
         ( sCodePlan->mTableName.name != NULL ) &&
         ( sCodePlan->mTableName.size > 0 ) )
    {
        iduVarStringAppendLength( aString,
                                  sCodePlan->mTableName.name,
                                  sCodePlan->mTableName.size );
    }
    else
    {
        /* nothing to do */
    }

    if ((sCodePlan->mAliasName.name != NULL) &&
        (sCodePlan->mAliasName.size > 0) &&
        (sCodePlan->mAliasName.name != sCodePlan->mTableName.name))
    {
        iduVarStringAppendLength(aString, " ", 1);

        if (sCodePlan->mAliasName.size <= QC_MAX_OBJECT_NAME_LEN)
        {
            iduVarStringAppendLength( aString,
                                      sCodePlan->mAliasName.name,
                                      sCodePlan->mAliasName.size );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    /*
     * ----------------------------------------------------
     * access
     * ----------------------------------------------------
     */
    printAccessInfo(sDataPlan, aString, aMode);

    iduVarStringAppendLength(aString, " )\n", 3);

    //----------------------------
    // Plan ID 출력
    //----------------------------
    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        qmn::printSpaceDepth(aString, aDepth);

        // sCodePlan 의 값을 출력하기때문에 qmn::printMTRinfo을 사용하지 못한다.
        iduVarStringAppendFormat( aString,
                                  "[ SELF NODE INFO, "
                                  "SELF: %"ID_INT32_FMT" ]\n",
                                  (SInt)sCodePlan->mTupleRowID );
    }
    else
    {
        /* nothing to do */
    }
    /*
     * ----------------------------------------------------
     * detail predicate 
     * ----------------------------------------------------
     */
    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {    
        if (sCodePlan->mConstantFilter != NULL)
        {    
            qmn::printSpaceDepth(aString, aDepth);
            iduVarStringAppendLength(aString, " [ CONSTANT FILTER ]\n", 21);

            IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                              aString,
                                              aDepth + 1, 
                                              sCodePlan->mConstantFilter)
                     != IDE_SUCCESS);
        }    
        else 
        {    
            /* nothing to do */
        }
    }
    else 
    {
        /* nothing to do */
    }

    /*
     * ----------------------------------------------------
     * subquery
     * ----------------------------------------------------
     */
    if (sCodePlan->mConstantFilter != NULL)
    {
        IDE_TEST(qmn::printSubqueryPlan(aTemplate,
                                        sCodePlan->mConstantFilter,
                                        aDepth,
                                        aString,
                                        aMode)
                 != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    /*
     * ----------------------------------------------------
     * child PRLQ plan의 정보 출력
     * ----------------------------------------------------
     */
    if (QCU_TRCLOG_DISPLAY_CHILDREN == 1)
    {
        for (sChildren = sCodePlan->plan.childrenPRLQ;
             sChildren != NULL;
             sChildren = sChildren->next)
        {
            IDE_TEST(sChildren->childPlan->printPlan(aTemplate,
                                                     sChildren->childPlan,
                                                     aDepth + 1,
                                                     aString,
                                                     aMode)
                     != IDE_SUCCESS);
        }
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmnPSCRD::printAccessInfo(qmndPSCRD    * aDataPlan,
                               iduVarString * aString,
                               qmnDisplay     aMode)
{
    IDE_DASSERT(aDataPlan != NULL);
    IDE_DASSERT(aString   != NULL);

    if (aMode == QMN_DISPLAY_ALL)
    {
        /* explain plan = on */

        if ((*aDataPlan->flag & QMND_PSCRD_INIT_DONE_MASK) ==
            QMND_PSCRD_INIT_DONE_TRUE)
        {
            iduVarStringAppendFormat(aString,
                                     ", ACCESS: %"ID_UINT32_FMT"",
                                     aDataPlan->mAccessCount);
        }
        else
        {
            iduVarStringAppend(aString, ", ACCESS: 0");
        }
    }
    else
    {
        /* explain plan = only */

        iduVarStringAppend(aString, ", ACCESS: ??");
    }
}

IDE_RC qmnPSCRD::doIt(qcTemplate* aTemplate,
                      qmnPlan   * aPlan,
                      qmcRowFlag* aFlag)
{
    qmndPSCRD* sDataPlan;
    sDataPlan = (qmndPSCRD*)(aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST(sDataPlan->doIt(aTemplate, aPlan, aFlag) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PSCRD 최초 doIt 실행 함수
 * 하위 PRLQ 를 모두 시작시킨 후 row 하나를 얻어온다.
 * ------------------------------------------------------------------
 */
IDE_RC qmnPSCRD::doItFirst(qcTemplate* aTemplate,
                           qmnPlan   * aPlan,
                           qmcRowFlag* aFlag)
{
    qmndPSCRD       * sDataPlan;
    qmnPSCRDChildren* sCurChild;
    qmnPSCRDChildren* sPrevChild;
    UInt              sTID;
    UInt              i;

    sDataPlan = (qmndPSCRD*)(aTemplate->tmplate.data + aPlan->offset);

    makeChildrenPRLQArea(aTemplate, aPlan);

    /*
     * start all children PRLQs
     */
    sPrevChild = sDataPlan->mPrevPRLQ;
    sCurChild  = sDataPlan->mCurPRLQ;

    for (i = 0; i < sDataPlan->mPRLQCnt; i++)
    {
        IDE_TEST(qmnPRLQ::startIt(aTemplate, sCurChild->mPlan, &sTID)
                 != IDE_SUCCESS);
        sCurChild->mTID = sTID;

        if (sTID == 0)
        {
            sDataPlan->mSerialRemain = ID_TRUE;
            sDataPlan->mCurPRLQ      = sCurChild;
            sDataPlan->mPrevPRLQ     = sPrevChild;
        }
        else
        {
            /* nothing to do */
        }
        sPrevChild = sPrevChild->mNext;
        sCurChild  = sCurChild->mNext;
    }

    sDataPlan->doIt = qmnPSCRD::doItNext;

    /*
     * read a row
     */
    IDE_TEST(sDataPlan->doIt(aTemplate, aPlan, aFlag) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PSCRD 일반 doIt 함수
 *
 * 현재 child 를 다 읽었거나 queue 가 임시로 비어있으면 child switch
 * 1024 개 rows 읽을때마다 child switch
 * ------------------------------------------------------------------
 */
IDE_RC qmnPSCRD::doItNext(qcTemplate* aTemplate,
                          qmnPlan   * aPlan,
                          qmcRowFlag* aFlag)
{
    qmndPSCRD       * sDataPlan;
    qmnPlan         * sChildPRLQPlan;
    qmndPRLQ        * sChildPRLQDataPlan;
    qmnPSCRDChildren* sPrevPRLQ;
    qmnPSCRDChildren* sCurPRLQ;
    qmcRowFlag        sFlag;

    sDataPlan = (qmndPSCRD*)(aTemplate->tmplate.data + aPlan->offset);

    // PROJ-2444
    // PSCRD는 하위 SCAN 플랜의 tuple 를 공유한다.
    // 상위 플랜에서 SCAN 의 tuple->row 가 가르키는 메모리를 저장할수 있다.
    // PSCRD에서 tuple->row 를 변경하기 때문에 원본을 유지해주어야 한다.
    sDataPlan->plan.myTuple->row = sDataPlan->mOrgRow;

    while (1)
    {
        sChildPRLQPlan      = sDataPlan->mCurPRLQ->mPlan;
        sChildPRLQDataPlan  = (qmndPRLQ*)(aTemplate->tmplate.data + sChildPRLQPlan->offset);

        IDE_TEST(sChildPRLQPlan->doIt(aTemplate,
                                      sChildPRLQPlan,
                                      &sFlag)
                 != IDE_SUCCESS);

        if ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST)
        {
            // PROJ-2444
            // 자식 PRLQ 의 데이타를 직접 읽어온다.
            sDataPlan->plan.myTuple->rid = sChildPRLQDataPlan->mRid;
            sDataPlan->plan.myTuple->row = sChildPRLQDataPlan->mRow;

            sDataPlan->mAccessCount++;
            if (sDataPlan->mCurPRLQ->mTID != QMN_PLAN_INIT_THREAD_ID)
            {
                sDataPlan->plan.myTuple->modify++;
            }
            else
            {
                /* nothing to do */
            }

            /*
             * after read some records, switch PRLQ
             * x modulo 2^n == x bitwiseand (2^n - 1)
             */
            if ((sDataPlan->mAccessCount & 1023) == 0)
            {
                if (sDataPlan->mSerialRemain == ID_FALSE)
                {
                    sDataPlan->mPrevPRLQ = sDataPlan->mCurPRLQ;
                    sDataPlan->mCurPRLQ  = sDataPlan->mCurPRLQ->mNext;
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* nothing to do */
            }

            break;
        }
        else
        {
            /* QMC_ROW_DATA_NONE */

            if ((sFlag & QMC_ROW_QUEUE_EMPTY_MASK) == QMC_ROW_QUEUE_EMPTY_TRUE)
            {
                /*
                 * this PRLQ is done
                 * remove it from the PRLQ list
                 */

                if (sDataPlan->mCurPRLQ->mNext == sDataPlan->mCurPRLQ)
                {
                    /*
                     * this is the last PRLQ
                     * no more record to read
                     */
                    sDataPlan->mCurPRLQ  = NULL;
                    sDataPlan->mPrevPRLQ = NULL;
                    sFlag = QMC_ROW_DATA_NONE;
                    break;
                }
                else
                {
                    /*
                     * remove current PRLQ from the PRLQ list
                     * try next PRLQ
                     * if serial PRLQs remain, execute them first
                     */
                    sDataPlan->mPrevPRLQ->mNext = sDataPlan->mCurPRLQ->mNext;
                    sDataPlan->mCurPRLQ         = sDataPlan->mCurPRLQ->mNext;

                    if (sDataPlan->mSerialRemain == ID_TRUE)
                    {
                        sPrevPRLQ = sDataPlan->mPrevPRLQ;
                        sCurPRLQ  = sDataPlan->mCurPRLQ;

                        while (1)
                        {
                            if (sCurPRLQ->mTID == QMN_PLAN_INIT_THREAD_ID)
                            {
                                sDataPlan->mPrevPRLQ = sPrevPRLQ;
                                sDataPlan->mCurPRLQ  = sCurPRLQ;
                                break;
                            }
                            else
                            {
                                /* nothing to do */
                            }

                            if (sCurPRLQ == sDataPlan->mPrevPRLQ)
                            {
                                sDataPlan->mSerialRemain = ID_FALSE;
                                break;
                            }

                            sPrevPRLQ = sPrevPRLQ->mNext;
                            sCurPRLQ  = sCurPRLQ->mNext;
                        }
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }
            }
            else
            {
                /*
                 * queue is temporarily empty
                 * try next PRLQ
                 */
                if (sDataPlan->mSerialRemain == ID_FALSE)
                {
                    sDataPlan->mPrevPRLQ = sDataPlan->mCurPRLQ;
                    sDataPlan->mCurPRLQ  = sDataPlan->mCurPRLQ->mNext;
                }
                else
                {
                    /* nothing to do */
                }
            }
        }
    }

    if ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE)
    {
        /*
         * record 가 없는 경우,
         * 다음 수행을 위해 최초 수행 함수로 설정
         */
        sDataPlan->doIt = qmnPSCRD::doItFirst;
    }
    else
    {
        /* nothing to do */
    }

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPSCRD::doItAllFalse(qcTemplate* /*aTemplate*/,
                              qmnPlan   * /*aPlan*/,
                              qmcRowFlag* aFlag)
{
   *aFlag &= ~QMC_ROW_DATA_MASK;
   *aFlag |= QMC_ROW_DATA_NONE;
   
   return IDE_SUCCESS;
}

/*
 * ------------------------------------------------------------------
 * doItFirst 과정중에
 * data plan 영역에 실행할 children plan 구성
 * ------------------------------------------------------------------
 */
void qmnPSCRD::makeChildrenPRLQArea(qcTemplate* aTemplate, qmnPlan* aPlan)
{
    qmndPSCRD  * sDataPlan;
    qmnChildren* sChild;
    UInt         i;

    sDataPlan = (qmndPSCRD*)(aTemplate->tmplate.data + aPlan->offset);

    for (i = 0, sChild = aPlan->childrenPRLQ;
         sChild != NULL;
         i++, sChild = sChild->next)
    {
        sDataPlan->mChildrenPRLQArea[i].mPlan = sChild->childPlan;
        sDataPlan->mChildrenPRLQArea[i].mTID = ID_UINT_MAX;
        if (i > 0)
        {
            sDataPlan->mChildrenPRLQArea[i-1].mNext =
                &sDataPlan->mChildrenPRLQArea[i];
        }
        else
        {
            /* nothing to do */
        }
    }
    IDE_DASSERT(i == sDataPlan->mPRLQCnt);

    /* circular list */
    sDataPlan->mChildrenPRLQArea[i-1].mNext = &sDataPlan->mChildrenPRLQArea[0];

    sDataPlan->mCurPRLQ  = &sDataPlan->mChildrenPRLQArea[0];
    sDataPlan->mPrevPRLQ = &sDataPlan->mChildrenPRLQArea[i-1];
}
