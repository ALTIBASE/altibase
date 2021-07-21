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
 * $Id: qtcPass.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 *     Pass Node
 *        - 하위 노드의 결과를 그대로 전달하는 Node
 *        - 연산의 중복 수행을 방지하기 위해 사용된다.
 *     Ex) SELECT i1 + 1 FROM T1 ORDER BY 1;
 *
 *     [PROJ]--------->[Assign]
 *        |              |
 *        |              |
 *        |              |
 *        |            [Pass Node]
 *        |              |
 *        |              V
 *        |        dst [I1 + 1]
 *        |              |
 *     [SORT]-------------
 *                       |
 *                 src [I1 + 1]
 *
 *     위에서와 같이 [Pass Node]를 중간에 연결함으로서 [i1 + 1]의
 *     반복 수행을 방지할 수 있다.
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
// Pass 연산자의 이름에 대한 정보
//-----------------------------------------

static mtcName mtfFunctionName[1] = {
    { NULL, 4, (void*)"PASS" }
};

//-----------------------------------------
// Pass 연산자의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC qtcEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qtc::passModule = {
    1|                          // 하나의 Column 공간
    MTC_NODE_OPERATOR_MISC,     // 기타 연산자
    ~0,                         // Indexable Mask : 의미 없음
    1.0,                        // default selectivity (비교 연산자 아님)
    mtfFunctionName,            // 이름 정보
    NULL,                       // Counter 연산자 없음
    mtf::initializeDefault,     // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,       // 서버 종료시 종료 함수, 없음
    qtcEstimate                 // Estimate 할 함수
};


//-----------------------------------------
// Pass 연산자의 수행 함수의 정의
//-----------------------------------------

IDE_RC qtcCalculate_Pass( mtcNode*  aNode,
                          mtcStack* aStack,
                          SInt      aRemain,
                          void*     aInfo,
                          mtcTemplate* aTemplate );

static const mtcExecute qtcExecute = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qtcCalculate_Pass,    // PASS 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtx::calculateNA,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

IDE_RC qtcEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    /* aStack */,
                    SInt         /* aRemain */,
                    mtcCallBack* /* aCallBack */)
{
/***********************************************************************
 *
 * Description :
 *    Pass 연산자에 대하여 Estimate 를 수행함.
 *    Pass Node에 대한 Column 정보 및 Execute 정보를 Setting한다.
 *
 * Implementation :
 *
 *    호스트 변수가 존재할 경우, 상위에서의 처리를 위해
 *    Argument자체의 정보를 Setting한다.
 *    이 때 결정된 정보는 Estimation을 위해 사용될 뿐
 *    Execution시 아무 영향을 미치지 않는다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtcEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcNode*         sNode;
    mtcColumn *      sColumn;

    if ( ( aNode->lflag & MTC_NODE_INDIRECT_MASK )
         == MTC_NODE_INDIRECT_TRUE )
    {
        // To Fix PR-8192
        // 일반 Expression으로부터 생성된 Pass Node임.

        // Column 정보로 skipModule을 사용함.
        /*
        IDE_TEST( qtc::skipModule.estimate( aTuple[aNode->table].columns +
                                            aNode->column,
                                            0, 0, 0 )
                  != IDE_SUCCESS );
        */
        IDE_TEST(
            mtc::initializeColumn(
                aTemplate->rows[aNode->table].columns + aNode->column,
                & qtc::skipModule,
                0,
                0,
                0 )
            != IDE_SUCCESS );

        //sNode = aNode->arguments;

        aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;
    }
    else
    {
        // Group By Expression으로부터 생성된 Pass Node임.
        // 나 자신의 Column 정보를 상위로 설정한다.

        // Converted Node를 획득해서는 안된다.
        sNode = aNode->arguments;

        // Argument의 Column 정보를 복사 및 Execute 정보를 setting한다.
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

        // BUG-23102
        // mtcColumn으로 초기화한다.
        mtc::initializeColumn( sColumn,
                               aTemplate->rows[sNode->table].columns
                               + sNode->column );

        aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtcCalculate_Pass( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*,
                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Argument를 이용하여 정보를 Setting한다.
 *
 * Implementation :
 *
 *    Stack에 column정보와 Value 정보를 Setting한다.
 *
 ***********************************************************************/

    mtcNode   * sNode;
    mtcColumn * sPassNodeColumn;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    // Argument 획득.
    sNode = mtf::convertedNode( aNode->arguments,
                                aTemplate );

    IDE_DASSERT(sNode->column != MTC_RID_COLUMN_ID);

    // Argument를 이용한 Column 및 Value 정보 Setting
    
    aStack->column = aTemplate->rows[sNode->table].columns + sNode->column;

    /* BUG-44762 case when subquery
     *
     * Case When에서 Subquery로 인해 사용된 Pass Node의 경우에는 Node를 통해
     * Value를 계산하지 않는다. SubQuery의 결과는 이미 Case when에서
     * Stack에 미리 지정해놓았다
     */
    if ( ( ( ((qtcNode*)(aNode->arguments))->lflag & QTC_NODE_SUBQUERY_MASK )
           == QTC_NODE_SUBQUERY_EXIST ) &&
        ( ( aNode->lflag & MTC_NODE_CASE_EXPRESSION_PASSNODE_MASK )
            == MTC_NODE_CASE_EXPRESSION_PASSNODE_TRUE ) )
    {
        /* Nothing to do */
    }
    else
    {
        aStack->value  = (void*)mtc::value( aStack->column,
                                            aTemplate->rows[sNode->table].row,
                                            MTD_OFFSET_USE );
    }

    if ( ( aNode->lflag & MTC_NODE_INDIRECT_MASK )
         == MTC_NODE_INDIRECT_FALSE )
    {
        // BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생
        // BUG-28446 [valgrind], BUG-38133
        // qtcCalculate_Pass(mtcNode*, mtcStack*, int, void*, mtcTemplate*) (qtcPass.cpp:333)
        if( ( aNode->lflag & MTC_NODE_CASE_EXPRESSION_PASSNODE_MASK )
            == MTC_NODE_CASE_EXPRESSION_PASSNODE_FALSE )
        {            
            // To Fix PR-10033
            // GROUP BY 컬럼을 참조하는 Pass Node는 Direct Node이다.
            // Pass Node가 Indirect Node가 아닌 경우
            // 해당 영역에 값을 저장하고 있어야 한다.
            // 이는 Key Range등을 생성할 때, indirect node인 경우
            // 해당 Node를 찾아 그 영역에 있는 Data를 이용하여
            // Key Range를 구성하는 반면, direct node인 경우에는
            // Pass Node로부터 값을 얻어 Key Range를 생성하기 때문이다.

            sPassNodeColumn = aTemplate->rows[aNode->table].columns
                + aNode->column;
        
            idlOS::memcpy(
                (SChar*) aTemplate->rows[aNode->table].row
                + sPassNodeColumn->column.offset,
                aStack->value,
                aStack->column->module->actualSize( aStack->column,
                                                    aStack->value ) );
        }
        else
        {
            // BUG-28446 [valgrind]
            // qtcCalculate_Pass(mtcNode*, mtcStack*, int, void*, mtcTemplate*) (qtcPass.cpp:333)
            // SIMPLE CASE처리를 위해 필요에 의해 생성된 PASS NODE임을 표시
            // skipModule이며 size는 0이다.
            // 따라서, 복사될 수 없다.
        }
    }
    else
    {
        // Indirect Pass Node의 경우 Argument만을 사용하기 때문에
        // 실제 값을 저장할 필요가 없다.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
