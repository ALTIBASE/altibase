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
 * $Id: qtcAssign.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 *     ASSIGN 연산을 수행하는 Node
 *
 *     To fix BUG-20272
 *     subquery의 target에 conversion node가 생성되어서는 안되는 경우
 *     conversion을 위한 assign node를 생성한다.
 *
 *     ex) decode(:v1, 1, (select max(i1) from t1))
 *                                ^^^^^^
 *     max(i1) 같은 aggr노드들은 mtrNode를 생성하므로 indirect conversion이
 *     생성되어서는 안된다. 이런 경우 max(i1)을 인자로 갖는 assign node를
 *     생성하여 assign node에 indirect conversion node를 달아야 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <qtc.h>
#include <ide.h>
#include <mte.h>

extern mtdModule mtdList;

//-----------------------------------------
// Assign 연산자의 이름에 대한 정보
//-----------------------------------------

static mtcName qtcNames[1] = {
    { NULL, 6, (void*)"ASSIGN" }
};

//-----------------------------------------
// Assign 연산자의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC qtcEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qtc::assignModule = {
    1 |                     // 하나의 Column 공간
    MTC_NODE_OPERATOR_MISC, // 기타 연산자
    ~(MTC_NODE_INDEX_MASK), // Indexable Mask
    1.0,                    // default selectivity (비교 연산자 아님)
    qtcNames,               // 이름 정보
    NULL,                   // Counter 연산자 없음
    mtf::initializeDefault, // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,   // 서버 종료시 종료 함수, 없음
    qtcEstimate             // Estimate 할 함수
};

//-----------------------------------------
// Assign 연산자의 수행 함수의 정의
//-----------------------------------------

IDE_RC qtcCalculate_Assign( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qtcExecute = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcCalculate_Assign,  // ASSIGN 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtx::calculateNA,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

IDE_RC qtcEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    Assign 연산자에 대하여 Estimate 를 수행함.
 *    Assign Node에 대한 Column 정보 및 Execute 정보를 Setting한다.
 *
 * Implementation :
 *    TODO - Host 변수의 존재에 대한 고려가 되어 있지 않음
 *
 *    Argument의 Column정보를 이용하여 Assign Node의 Column 정보를 설정하고,
 *    Assign Node의 Execute 정보를 Setting함
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    qtcCallBackInfo * sInfo;
    mtcNode         * sNode;
    mtcColumn       * sColumn;

    sInfo = (qtcCallBackInfo*)aCallBack->info;

    // Argument 를 얻는다.
    sNode = mtf::convertedNode( aNode->arguments,
                                & sInfo->tmplate->tmplate );

    // Argument의 Column 정보를 복사 및 Execute 정보를 setting한다.
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    // BUG-23102
    // mtcColumn으로 초기화한다.
    mtc::initializeColumn( sColumn,
                           aTemplate->rows[sNode->table].columns + sNode->column );

    aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;

    // 상위에서의 Estimation을 위해 Stack에 Column 정보 Setting
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtcCalculate_Assign( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*,
                            mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Assign 연산 ( a := b ) 연산을 수행한다.
 *
 * Implementation :
 *
 *    Argument를 수행하고 그 결과를 Assign노드의 위치에 복사한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcCalculate_Assign"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcNode*  sNode;

    // BUG-33674
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );
    
    // Assign Node의 Column정보 및 Value 정보의 위치를 설정함.
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    // Argument의 정보를 획득함.
    sNode  = aNode->arguments;

    // Argument의 연산을 수행함.
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                        aStack + 1,
                                                                        aRemain - 1,
                                                                        aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                        aTemplate )
              != IDE_SUCCESS );

    // Argument가 Conversion을 가질 경우 Conversion 연산을 수행함.
    if( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
                                         aStack + 1,
                                         aRemain - 1,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    // Argument의 연산 결과가 저장된 Stack정보로부터
    // Assign Node의 Value 위치에 복사함.
    idlOS::memcpy( aStack[0].value, aStack[1].value,
                   aStack[1].column->module->actualSize(
                       aStack[1].column,
                       aStack[1].value ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
