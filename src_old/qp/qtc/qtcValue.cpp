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
 * $Id: qtcValue.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 *     Value를 의미하는 Node
 *     Ex) 'ABC'
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <qtc.h>
#include <mte.h>

//-----------------------------------------
// Value 연산자의 이름에 대한 정보
//-----------------------------------------

static mtcName qtcNames[1] = {
    { NULL, 5, (void*)"VALUE" }
};

//-----------------------------------------
// Value 연산자의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC qtcValueEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule qtc::valueModule = {
    1|                      // 하나의 Column 공간
    MTC_NODE_OPERATOR_MISC, // 기타 연산자
    ~0,                     // Indexable Mask : 의미 없음
    1.0,                    // default selectivity (비교 연산자 아님)
    qtcNames,               // 이름 정보
    NULL,                   // Counter 연산자 없음
    mtf::initializeDefault, // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,   // 서버 종료시 종료 함수, 없음
    qtcValueEstimate        // Estimate 할 함수
};

//-----------------------------------------
// Value 연산자의 수행 함수의 정의
//-----------------------------------------

IDE_RC qtcCalculate_Value(  mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qtcExecute = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcCalculate_Value,   // VALUE 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtx::calculateEmpty,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

IDE_RC qtcValueEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt         /* aRemain */,
                         mtcCallBack* /* aCallback */ )
{
/***********************************************************************
 *
 * Description :
 *    Value 연산자에 대하여 Estimate 를 수행함.
 *    Value Node에 대한 Execute 정보를 설정함
 *
 * Implementation :
 *
 *    Stack에 Value Node에 대한 Column 정보를 설정하고
 *    Value Node에 대한 Execute 정보를 Setting
 ***********************************************************************/

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;

    return IDE_SUCCESS;
}

IDE_RC qtcCalculate_Value( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*,
                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Value의 연산을 수행한다.
 *
 * Implementation :
 *
 *    Stack에 column정보와 Value 정보를 Setting한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcCalculate_Value"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
