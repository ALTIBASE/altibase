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
 * $Id: qmnPRLQ.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QMN_PRLQ_H_
#define _O_QMN_PRLQ_H_ 1

#include <qmnDef.h>
#include <qmcThr.h>

//-----------------
// Data Node Flags
//-----------------

// qmndPRLQ.flag
// First Initialization Done
#define QMND_PRLQ_INIT_DONE_MASK  (0x00000001)
#define QMND_PRLQ_INIT_DONE_FALSE (0x00000000)
#define QMND_PRLQ_INIT_DONE_TRUE  (0x00000001)

#define QMND_PRLQ_PRL_EXEC_MASK   (0x00000002)
#define QMND_PRLQ_PRL_EXEC_FALSE  (0x00000000)
#define QMND_PRLQ_PRL_EXEC_TRUE   (0x00000002)

/*
 * qmndPRLQ.mParallelType
 */
#define QMNC_PRLQ_PARALLEL_TYPE_SCAN      (1)
#define QMNC_PRLQ_PARALLEL_TYPE_PARTITION (2)

typedef struct qmnPRLQRecord
{
    scGRID  rid;
    void  * row;
} qmnPRLQRecord;

typedef struct qmnPRLQQueue 
{
    UInt           mHead;
    UInt           mTail;
    UInt           mSize;
    qmnPRLQRecord* mRecord;
} qmnPRLQQueue;

typedef struct qmnPRLQThrArg
{
    qcTemplate   * mTemplate;   /* BUG-38294 */
    qmnPlan      * mChildPlan;
    qmnPRLQQueue * mPRLQQueue;
    void        ** mRowBuffer;
    UInt         * mAllRowRead;
} qmnPRLQThrArg;

typedef struct qmncPRLQ
{
    qmnPlan     plan;
    UInt        planID;

    qtcNode   * myNode;
    UInt        mParallelType;
} qmncPRLQ;

typedef struct qmndPRLQ
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan         plan;
    doItFunc         doIt;
    UInt           * flag;

    //---------------------------------
    // PRLQ 고유 정보
    //---------------------------------

    // PROJ-2444
    // 기존에는 dequeue 한 데이타를 하위 자식 노드에 다시 세팅해 주었다.
    // 이를 변경하여 PRLQ가 데이타를 저장하고
    // 상위 PPCRD, PSCRD 에서 이를 접근하여 가져가도록 변경한다.
    scGRID          mRid;
    void          * mRow;

    qmnPRLQQueue    mQueue;
    qmnPRLQThrArg * mThrArg;
    qmcThrObj     * mThrObj;
    UInt            mAllRowRead;

    void          * nullRow;    // Null Row
    qtcNode       * myNode;

    qmnPlan       * mCurrentNode;   // 동적 실행을 위한 left

    /* BUG-38294 */
    idBool          mTemplateCloned;
    qcTemplate      mTemplate;

    /* PROJ-2464 hybrid partitioned table 지원 */
    idBool         mExistDisk;
    UInt           mDiskRowOffset;
} qmndPRLQ;

class qmnPRLQ
{
public:
    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    static IDE_RC padNull( qcTemplate * aTemplate,
                           qmnPlan    * aPlan );

    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );

    static void setChildNode( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmnPlan    * aChildPlan );

    static void returnThread( qcTemplate * aTemplate,
                              qmnPlan    * aPlan );

    /* BUG-38294 */
    static qcTemplate* getTemplate(qcTemplate* aTemplate, qmnPlan* aPlan);

    static IDE_RC startIt(qcTemplate* aTemplate, qmnPlan* aPlan, UInt* aTID);

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void setPRLQInfo( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             idBool       aExistDisk,
                             UInt         aDiskRowOffset );
private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmndPRLQ   * aDataPlan );

    static IDE_RC doItResume( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag );

    static IDE_RC doItNextSerial(qcTemplate* aTemplate,
                                 qmnPlan   * aPlan,
                                 qmcRowFlag* aFlag);

    static IDE_RC doItNextDequeue( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan,
                                   qmcRowFlag * aFlag );

    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 최초 PRLQ를 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    static inline idBool enqueue(qmnPRLQQueue * aQueue,
                                 scGRID         aRid,
                                 void         * aRow);

    static inline idBool dequeue(qmnPRLQQueue * aQueue,
                                 scGRID       * aRid,
                                 void        ** aRow);

    /* enqueue thread function */
    static IDE_RC runChildEnqueue( qmcThrObj * aThrObj );
};

inline idBool qmnPRLQ::enqueue(qmnPRLQQueue* aQueue,
                               scGRID        aRid,
                               void        * aRow)
{
    idBool sIsSuccess;
    UInt   sHead;
    UInt   sTail;

    sHead = aQueue->mHead;
    sTail = aQueue->mTail;

    if (sTail + 2 < aQueue->mSize)
    {
        if (sTail + 2 != sHead)
        {
            aQueue->mRecord[sTail].rid = aRid;
            aQueue->mRecord[sTail].row = aRow;

            acpAtomicInc32( & aQueue->mTail );
            sIsSuccess = ID_TRUE;
        }
        else
        {
            sIsSuccess = ID_FALSE;
        }
    }
    else
    {
        if (sTail + 2 - aQueue->mSize != sHead)
        {
            aQueue->mRecord[sTail].rid = aRid;
            aQueue->mRecord[sTail].row = aRow;

            if (sTail + 1 < aQueue->mSize)
            {
                acpAtomicInc32( & aQueue->mTail );
            }
            else
            {
                acpAtomicSet32( & aQueue->mTail, 0 );
            }
            sIsSuccess = ID_TRUE;
        }
        else
        {
            sIsSuccess = ID_FALSE;
        }
    }

    return sIsSuccess;
}

inline idBool qmnPRLQ::dequeue(qmnPRLQQueue * aQueue,
                               scGRID       * aRid,
                               void        ** aRow)
{
    idBool sIsSuccess;
    UInt   sHead;
    UInt   sTail;

    sHead = aQueue->mHead;
    sTail = aQueue->mTail;

    if (sHead != sTail)
    {
        *aRid = aQueue->mRecord[sHead].rid;
        *aRow = aQueue->mRecord[sHead].row;

        if (sHead + 1 < aQueue->mSize)
        {
            (aQueue->mHead)++;
        }
        else
        {
            aQueue->mHead = 0;
        }
        sIsSuccess = ID_TRUE;
    }
    else
    {
        sIsSuccess = ID_FALSE;
    }

    return sIsSuccess;
}

#endif /* _O_QMN_PRLQ_H_ */
