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
 * $Id: qtcRid.cpp 36474 2009-11-02 05:48:43Z sungminee $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <qtc.h>
#include <smi.h>
#include <qmvQTC.h>
#include <qtcRid.h>
#include <qcuSqlSourceInfo.h>

extern mtxModule mtxRid; /* PROJ-2632 */

static IDE_RC qtcRidEstimate(mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             mtcCallBack* aCallBack);
static IDE_RC qtcRidCalculate(mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aCalcInfo,
                              mtcTemplate* aTemplate);

static mtcName gQtcRidNames[1] = {
    { NULL, 3, (void*)"RID" }
};

mtfModule gQtcRidModule = {
    1|                        // 하나의 Column 공간
    MTC_NODE_INDEX_UNUSABLE|  // Index를 사용할 수 있음
    MTC_NODE_OPERATOR_MISC,   // 기타 연산자
    ~0,                       // Indexable Mask : 의미 없음
    1.0,                      // default selectivity (비교 연산자 아님)
    gQtcRidNames,             // 이름 정보
    NULL,                     // Counter 연산자 없음
    mtf::initializeDefault,   // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,     // 서버 종료시 종료 함수, 없음
    qtcRidEstimate,           // Estimate 할 함수
};

mtcColumn gQtcRidColumn;

mtcExecute gQtcRidExecute = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcRidCalculate,      // calculate
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtxRid.mCommon,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

/*
 * -----------------------------------------
 * select _prowid from t1
 *
 * _prowid 를 위한 qtcNode 를 생성
 * parse 과정에서 호출된다
 * -----------------------------------------
 */
IDE_RC qtcRidMakeColumn(qcStatement*    aStatement,
                        qtcNode**       aNode,
                        qcNamePosition* aUserPosition,
                        qcNamePosition* aTablePosition,
                        qcNamePosition* aColumnPosition)
{
    IDE_RC   sRc;
    qtcNode* sNode;

    IDU_LIMITPOINT("qtcRidMakeColumn::malloc");
    sRc = STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode , &sNode);
    IDE_TEST(sRc != IDE_SUCCESS);

    QTC_NODE_INIT(sNode);

    if ((aUserPosition != NULL) &&
        (aTablePosition != NULL) &&
        (aColumnPosition != NULL))
    {
        // -------------------------
        // | USER | TABLE | COLUMN |
        // -------------------------
        // |  O   |   O   |   O    |
        // -------------------------
        sNode->position.stmtText = aUserPosition->stmtText;
        sNode->position.offset   = aUserPosition->offset;
        sNode->position.size     = aColumnPosition->offset
            + aColumnPosition->size
            - aUserPosition->offset;
        sNode->userName          = *aUserPosition;
        sNode->tableName         = *aTablePosition;
        sNode->columnName        = *aColumnPosition;
    }
    else if ((aTablePosition != NULL) && (aColumnPosition != NULL))
    {
        // -------------------------
        // | USER | TABLE | COLUMN |
        // -------------------------
        // |  X   |   O   |   O    |
        // -------------------------
        sNode->position.stmtText = aTablePosition->stmtText;
        sNode->position.offset   = aTablePosition->offset;
        sNode->position.size     = aColumnPosition->offset
            + aColumnPosition->size
            - aTablePosition->offset;
        sNode->tableName         = *aTablePosition;
        sNode->columnName        = *aColumnPosition;
    }
    else if (aColumnPosition != NULL )
    {
        // -------------------------
        // | USER | TABLE | COLUMN |
        // -------------------------
        // |  X   |   X   |   O    |
        // -------------------------
        sNode->position        = *aColumnPosition;
        sNode->columnName      = *aColumnPosition;
    }
    else
    {
        IDE_ASSERT(0);
    }

    sNode->node.lflag = gQtcRidModule.lflag & (~MTC_NODE_COLUMN_COUNT_MASK);
    sNode->node.module = &gQtcRidModule;

    *aNode = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC qtcRidCalculate(mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        /*aCalcInfo*/,
                              mtcTemplate* aTemplate)
{
    IDE_TEST_RAISE(aRemain < 1, ERR_STACK_OVERFLOW);

    aStack->column = &gQtcRidColumn;
    aStack->value  = (void*)&aTemplate->rows[aNode->table].rid;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_STACK_OVERFLOW)
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static IDE_RC qtcRidEstimate(mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             mtcCallBack* aCallBack)
{
    qtcNode         * sNode         = (qtcNode *)aNode;
    qtcCallBackInfo * sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);

    qmsSFWGH        * sSFWGH;
    IDE_RC            sRc = IDE_SUCCESS;

    sSFWGH = sCallBackInfo->SFWGH;

    // 최초 estimate 시에만 Column ID를 할당받게 하고,
    // 이후의 estimate 호출시에는 Column ID를 할당받지 않도록 한다.
    // 따라서, estimate() 에서만 CallBackInfo에 statement를 설정한다.
    if (sCallBackInfo->statement != NULL)
    {
        // 실제 Column인 경우 Column ID를 Setting한다.
        // Column이 아닌 경우에는 해당 Node에 적합한 Module로 변경된다.
        // 예를 들어, 다음과 같은 질의를 살펴 보자.
        //     SELECT f1 FROM T1;
        // Parsing 단계에서는 [f1]을 Column으로 판단하지만,
        // 이는 Column일수도 Function일 수도 있다.
        // 만약 Module이 변경된 경우라면, 내부에서 estimate가 수행된다.

        if ((sNode->lflag & QTC_NODE_COLUMN_ESTIMATE_MASK) ==
            QTC_NODE_COLUMN_ESTIMATE_TRUE)
        {
            // partition column id를 지정하였으므로 column id를 새로 구하는 것은
            // 하지 않는다.
        }
        else
        {
            sRc = qmvQTC::setColumnID4Rid(sNode, aCallBack);
        }
    }

    if( sRc == IDE_FAILURE )
    {
        // BUG-38507
        // setColumnID4Rid에서 NOT EXISTS COLUMN 오류가 발생한 경우
        // 일반 column으로 estimate 한다.
        if( ideGetErrorCode() == qpERR_ABORT_QMV_NOT_EXISTS_COLUMN )
        {
            aNode->module = &qtc::columnModule;
            IDE_TEST( aNode->module->estimate( aNode,
                                               aTemplate,
                                               aStack,
                                               aRemain,
                                               aCallBack )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( 1 );
        }
    }
    else
    {
        IDE_TEST_RAISE((aTemplate->rows[aNode->table].lflag & MTC_TUPLE_TYPE_MASK)
                        != MTC_TUPLE_TYPE_TABLE,
                        ERR_PROWID_NOT_SUPPORTED);

        //-----------------------------------------------
        // PROJ-1473
        // 질의에 사용된 컬럼정보를 수집한다.
        // 레코드저장방식의 처리인 경우,
        // 디스크테이블과 뷰의 질의에 사용된 컬럼정보 수집.
        //-----------------------------------------------
        IDE_TEST(qtc::setColumnExecutionPosition(aTemplate,
                                                 sNode,
                                                 sSFWGH,
                                                 sCallBackInfo->SFWGH)
                 != IDE_SUCCESS);

        IDE_TEST_RAISE((sNode->lflag & QTC_NODE_PRIOR_MASK) == QTC_NODE_PRIOR_EXIST,
                       ERR_PROWID_NOT_SUPPORTED);

        qtc::dependencySet(sNode->node.table, & sNode->depInfo);

        aTemplate->rows[aNode->table].lflag &= ~MTC_TUPLE_TARGET_RID_MASK;
        aTemplate->rows[aNode->table].lflag |= MTC_TUPLE_TARGET_RID_EXIST;

        aStack->column = &gQtcRidColumn;
        aTemplate->rows[aNode->table].ridExecute = &gQtcRidExecute;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

