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
 * $Id: qtcSubqueryWrapper.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 *     Subquery Wrapper Node
 *        IN Subquery의 Key Range 생성을 위한 Node로 Node Transform에
 *        의한 Key Range 생성시에만 사용되며, Subquery의 제어를 담당하며
 *        Subquery Target의 첫번째 Column을 위해서만 생성된다..
 * 
 *        - Ex) i1 IN ( SELECT a1 FROM T1 );
 *
 *          [=]
 *           |
 *           V
 *          [Indirect]-------->[Subquery Wrapper]
 *           |                  |
 *           V                  V
 *          [i1]               [Subquery]
 *                              |
 *                              V
 *                             [a1]
 *
 *     위의 그림에서와 같이 IN은 Key Range를 위해 [=] 연산자로 대체되며,
 *     [Subquery Wrapper] 는 [Subquery]를 제어한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qtc.h>
#include <qmn.h>

extern mtdModule mtdList;

//-----------------------------------------
// Subquery Wrapper 연산자의 이름에 대한 정보
//-----------------------------------------

static mtcName qtcNames[1] = {
    { NULL, 17, (void*)"SUBQUERY_WRAPPER" }
};

//-----------------------------------------
// Subquery Wrapper 연산자의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC qtcEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qtc::subqueryWrapperModule = {
    1|                      // 하나의 Column 공간
    MTC_NODE_OPERATOR_MISC| // 기타 연산자
    MTC_NODE_INDIRECT_TRUE, // Indirection됨
    ~0,                     // Indexable Mask : 의미 없음
    1.0,                    // default selectivity (비교 연산자 아님)
    qtcNames,               // 이름 정보
    NULL,                   // Counter 연산자 없음 
    mtf::initializeDefault, // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,   // 서버 종료시 종료 함수, 없음 
    qtcEstimate             // Estimate 할 함수
};

//-----------------------------------------
// Subquery Wrapper 연산자의 수행 함수의 정의
//-----------------------------------------

IDE_RC qtcCalculate_SubqueryWrapper( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static const mtcExecute qtcExecute = {
    mtf::calculateNA,             // Aggregation 초기화 함수, 없음
    mtf::calculateNA,             // Aggregation 수행 함수, 없음
    mtf::calculateNA, 
    mtf::calculateNA,             // Aggregation 종료 함수, 없음
    qtcCalculate_SubqueryWrapper, // SUBQUERY WRAPPER 연산 함수
    NULL,                         // 연산을 위한 부가 정보, 없음
    mtx::calculateNA,
    mtk::estimateRangeNA,         // Key Range 크기 추출 함수, 없음 
    mtk::extractRangeNA           // Key Range 생성 함수, 없음
};


IDE_RC qtcEstimate( mtcNode * aNode,
                    mtcTemplate* aTemplate,
                    mtcStack * aStack,
                    SInt       /* aRemain */,
                    mtcCallBack * aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    Subquery Wrapper 연산자에 대하여 Estimate 를 수행함.
 *    Node에 대한 Column 정보 및 Execute 정보를 Setting한다.
 *
 * Implementation :
 *
 *    Subquery Wrapper 노드는 별도의 Column 정보가 필요없으므로,
 *    Skip Module로 estimation을 하며, 상위 Node에서의 estimate 를
 *    위하여 하위 Node의 정보를 Stack에 설정하여 준다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcNode   * sNode;
    mtcColumn * sColumn;
    mtcTemplate * sTemplate;
    qtcCallBackInfo * sCallBackInfo;

    //------------------------------------
    // 적합성 검사
    //------------------------------------

    IDE_DASSERT( aNode->arguments->module == & qtc::subqueryModule );

    //------------------------------------
    // 노드의 Estimate
    //------------------------------------

    sCallBackInfo = (qtcCallBackInfo *) aCallBack->info;
    sTemplate = & sCallBackInfo->tmplate->tmplate;

    // Column 정보를 skipModule로 설정하고, Execute 함수를 지정한다.
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;

    /*
    IDE_TEST( qtc::skipModule.estimate( sColumn, 0, 0, 0 )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( sColumn,
                                     & qtc::skipModule,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // To Fix PR-8259
    // Subquery Node 하위의 Argument를 얻어
    // 이 정보를 상위 Node에서 사용할 수 있도록
    // Stack에 설정한다.
    sNode = aNode->arguments->arguments;
    sNode = mtf::convertedNode( sNode, sTemplate );
    aStack[0].column = aTemplate->rows[sNode->table].columns + sNode->column;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtcCalculate_SubqueryWrapper( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*,
                                     mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *    Subquery Wrapper의 연산을 수행함.
 *    하위 Subquery Node를 한 건씩 수행한다.
 *
 * Implementation :
 *
 *    Subquery의 Plan Tree를 획득한 후,
 *    Plan Tree의 초기화 여부에 따라, plan tree에 대한 수행을 하며,
 *    결과가 없을 경우에는 NULL을 Setting한다.
 *
 ***********************************************************************/

#define IDE_FN "qtcCalculate_SubqueryWrapper"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    mtcNode*     sNode;
    mtcStack*    sStack;
    SInt         sRemain;
    qmnPlan*     sPlan;
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;

    mtcColumn *  sColumn;
    void      *  sValue;
    
    sStack     = aTemplate->stack;
    sRemain    = aTemplate->stackRemain;

    // Node의 Stack과 Subquery의 Stack이 필요함.
    // List형 Subquery의 경우 Stack 검사를 Plan Tree에서 수행함.
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );
    
    // Subquery의 Plan Tree 획득.
    sNode = aNode->arguments;
    sPlan = ((qtcNode*)sNode)->subquery->myPlan->plan;

    aTemplate->stack       = aStack + 1;
    aTemplate->stackRemain = aRemain - 1;
    
    if ( aTemplate->execInfo[aNode->info] == QTC_WRAPPER_NODE_EXECUTE_FALSE )
    {
        //---------------------------------
        // 초기화가 되지 않은 경우
        //---------------------------------

        // Plan을 초기화한다.
        IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan )
                  != IDE_SUCCESS );

        // 수행되었음을 표시함.
        aTemplate->execInfo[aNode->info] = QTC_WRAPPER_NODE_EXECUTE_TRUE;
    }
    else
    {
        //---------------------------------
        // 초기화가 된 경우
        //---------------------------------

        // Nothing To Do
    }

    // Plan 을 수행
    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
              != IDE_SUCCESS );

    if ( (sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        //--------------------------------
        // 질의 결과가 없는 경우
        //--------------------------------

        aStack[0] = aStack[1];
        aStack[0].value = NULL;  // 결과가 없음을 표시

        // Plan Tree 종료
        aTemplate->execInfo[aNode->info] = QTC_WRAPPER_NODE_EXECUTE_FALSE;
    }
    else
    {
        //--------------------------------
        // 질의 결과가 있는 경우
        //--------------------------------

        // To Fix PR-8259
        // 하위 Subquery가 List형인지 One Column형인지 판단하여 처리해야 함.
        // List 형      : ( SELECT i1, i2 FROM .. )
        // One Column형 : ( SELECT i1 FROM .. )

        // Subquery의 Column획득
        sColumn = aTemplate->rows[sNode->table].columns + sNode->column;
        sValue = (void*)
            ( (SChar*) aTemplate->rows[sNode->table].row
              + sColumn->column.offset );
        
        if ( sColumn->module == & mtdList )
        {
            // List형 Subquery인 경우
            // Subquery Node의 Value정보 구성
            //
            // Subquery의 Target 정보가 생성된 Stack을
            // 통째로 지정한 영역에 복사한다.
            idlOS::memcpy( sValue,
                           & aStack[1],
                           sColumn->column.size );

            // List Subquery Node의 Column정보 및 Value정보를
            // Wrapper Node의 Stack 정보로 설정
            aStack[0].column = sColumn;
            aStack[0].value = sValue;
        }
        else
        {
            // One Column형 Subquery인 경우
            // Subquery의 결과를 복사.
            aStack[0] = aStack[1];
        }
    }

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}
