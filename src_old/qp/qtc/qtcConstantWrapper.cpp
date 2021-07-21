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
 * $Id: qtcConstantWrapper.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 *     Host Constant Wrapper Node
 *        - 항상 동일한 결과를 생성하는 연산을 한 번만 수행하고
 *        - 그 결과를 반복적으로 사용할 때 사용한다.
 *        - Host 변수가 있는 부분에 대해서만 처리하고,
 *        - Host 변수가 없는 영역은 Validation 과정 중에
 *          Pre-Processing Constant Expression에 의하여 처리된다.
 * 
 *        - Ex) 4 + ?
 *
 *          [Host Constant Wrapper Node]
 *                     |
 *                     V
 *                    [+]
 *                     |
 *                     V
 *                    [4]------>[?]
 *
 *     위의 그림에서와 같이 [4+?]을 한 번만 수행한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qtc.h>
#include <qci.h>

//-----------------------------------------
// Host Constant Wrapper 연산자의 이름에 대한 정보
//-----------------------------------------

static mtcName qtcNames[1] = {
    { NULL, 22, (void*)"HOST_CONSTANT_WRAPPER" }
};

//-----------------------------------------
// Constant Wrapper 연산자의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC qtcEstimate_HostConstantWrapper( mtcNode*     aNode,
                                               mtcTemplate* aTemplate,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               mtcCallBack* aCallBack );

mtfModule qtc::hostConstantWrapperModule = {
    1|                      // 하나의 Column 공간
    MTC_NODE_OPERATOR_MISC| // 기타 연산자
    MTC_NODE_INDIRECT_TRUE, // Indirection됨
    ~0,                     // Indexable Mask : 의미 없음
    0.0,                    // BUG-39036 ( 1 -> 0 변경 )
    qtcNames,               // 이름 정보
    NULL,                   // Counter 연산자 없음
    mtf::initializeDefault, // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,   // 서버 종료시 종료 함수, 없음
    qtcEstimate_HostConstantWrapper     // Estimate 할 함수
};

//-----------------------------------------
// Constant Wrapper 연산자의 수행 함수의 정의
//-----------------------------------------

IDE_RC qtcCalculate_HostConstantWrapper( 
                            mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qtcExecute = {
    mtf::calculateNA,             // Aggregation 초기화 함수, 없음
    mtf::calculateNA,             // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,             // Aggregation 종료 함수, 없음
    qtcCalculate_HostConstantWrapper, // CONSTANT WRAPPER 연산 함수
    NULL,                         // 연산을 위한 부가 정보, 없음
    mtx::calculateEmpty,
    mtk::estimateRangeNA,         // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA           // Key Range 생성 함수, 없음
};

IDE_RC qtcEstimate_HostConstantWrapper( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt      /* aRemain */,
                                        mtcCallBack* /*aCallBack*/ )
{
/***********************************************************************
 *
 * Description :
 *    Constant Wrapper 연산자에 대하여 Estimate 를 수행함.
 *    Node에 대한 Column 정보 및 Execute 정보를 Setting한다.
 *
 * Implementation :
 *
 *    Constant Wrapper Node는
 *    Plan Node 생성 후 최종적으로 처리되는 노드이다.
 *    따라서, 상위 Node에서 Estimate 를 위해 호출하는 경우가 없다.
 *
 *    Constant Wrapper 노드는 별도의 Column 정보가 필요없으므로,
 *    Skip Module로 estimation을 하며, 상위 Node에서의 estimate 를
 *    위하여 하위 Node의 정보를 Stack에 설정하여 준다.
 *
 *    PROJ-1492로 CAST연산자가 추가되어 호스트 변수를 사용하더라도
 *    그 타입이 정의되어 Validation시 Estimate 를 호출할 수 있다.
 *
 ***********************************************************************/

#define IDE_FN "qtcEstimate_HostConstantWrapper"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcNode   * sNode;
    mtcColumn * sColumn;

    // Column 정보를 skipModule로 설정하고, Execute 함수를 지정한다.
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;

    IDE_TEST( mtc::initializeColumn( sColumn,
                                     & qtc::skipModule,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Argument를 얻어 이 정보를 상위 Node에서 사용할 수 있도록
    // Stack에 설정한다.
    sNode = aNode->arguments;

    aStack[0].column = aTemplate->rows[sNode->table].columns + sNode->column;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtcCalculate_HostConstantWrapper( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*,
                                         mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *    Constant Wrapper의 연산을 수행함.
 *    최초 한 번만 Argument를 수행하고, 이 후는 Argument를 수행하지
 *    않는다.
 *
 * Implementation :
 *
 *    Template내 Execution 정보를 이용하여, 이미 수행되었는지의
 *    여부를 판단한다.
 *
 *    수행되지 않은 경우,
 *        - Argument를 수행하고, 그 결과 Stack을 복사한다.
 *    수행된 경우,
 *        - Argument 정보를 이용하여 Stack에 설정한다.
 *
 ***********************************************************************/

#define IDE_FN "qtcCalculate_HostConstantWrapper"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    mtcNode*  sNode;
    mtcStack * sStack;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    if ( aTemplate->execInfo[aNode->info] == QTC_WRAPPER_NODE_EXECUTE_FALSE )
    {
        //---------------------------------
        // 한 번도 수행되지 않은 경우
        //---------------------------------

        // Arguemnt를 획득한다.
        sNode  = aNode->arguments;
        sStack = aStack;
        sStack++;
        aRemain--;

        // Argument를 수행한다.
        IDE_TEST( aTemplate->rows[sNode->table].
                  execute[sNode->column].calculate(                     sNode,
                                                                       sStack,
                                                                      aRemain,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );
    
        if( sNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sNode,
                                             sStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // Argument의 수행 결과를 Stack에 복사.
        aStack[0] = aStack[1];
        aTemplate->execInfo[aNode->info] = QTC_WRAPPER_NODE_EXECUTE_TRUE;
    }
    else
    {
        //---------------------------------
        // 이미 수행된 경우
        //---------------------------------

        // 결과값이 있는 Argument를 획득함.
        sNode = aNode->arguments;
        sNode = mtf::convertedNode( sNode, aTemplate );

        // Column 정보를 Stack에 설정함.
        aStack[0].column = aTemplate->rows[sNode->table].columns
            + sNode->column;

        // Value 정보를 획득하여 Stack에 설정함.
        aStack[0].value = (void *) mtc::value( aStack[0].column, 
                                               aTemplate->rows[sNode->table].row, 
                                               MTD_OFFSET_USE );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}
 
