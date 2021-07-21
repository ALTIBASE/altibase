/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtxModule mtxLnnvl; /* PROJ-2632 */

//-----------------------------------------
// LNNVL 연산자의 이름에 대한 정보
//-----------------------------------------

static mtcName mtfFunctionName[1] = {
    { NULL, 5, (void*)"LNNVL" }
};

//-----------------------------------------
// LNNVL 연산자의 Module 에 대한 정보
//-----------------------------------------

static IDE_RC mtfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule mtfLnnvl = {
    1|                                // 하나의 Column 공간
    MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_COMPARISON_TRUE|MTC_NODE_EAT_NULL_TRUE|
    MTC_NODE_PRINT_FMT_PREFIX_PA,     // 출력 format
    ~(MTC_NODE_INDEX_MASK),           // Indexable Mask : Index 사용 불가
    1.0 / 2.0,                        // default selectivity
    mtfFunctionName,                  // 이름 정보
    NULL,                             // Counter 연산자 없음
    mtf::initializeDefault,           // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,             // 서버 종료시 종료 함수, 없음
    mtfEstimate                       // Estimate 할 함수
};

//-----------------------------------------
// Assign 연산자의 수행 함수의 정의
//-----------------------------------------

IDE_RC mtfLnnvlCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    mtfLnnvlCalculate,    // LNNVL 연산을 위한 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtxLnnvl.mCommon,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음 
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

IDE_RC mtfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /* aRemain */,
                    mtcCallBack* /* aCallback */)
{
/***********************************************************************
 *
 * Description :
 *    LNNVL 연산자에 대하여 Estimate 를 수행함.
 *    LNNVL Node에 대한 Column 정보 및 Execute 정보를 Setting한다.
 *
 * Implementation :
 *
 *    LNNVL Node의 Column정보를 Boolean Type으로 설정하고,
 *    LNNVL Node의 Execute 정보를 Setting함
 ***********************************************************************/

    extern mtdModule mtdBoolean;

    mtcNode* sNode;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    // Argument가 하나인지를 검사
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // Argument가 (논리, 비교) 연산자인지 검사
    sNode  = aNode->arguments;

    // LNNVL의 argument는 LNNVL 또는 비교연산자만 가능(AND, OR 등 불가)
    IDE_TEST_RAISE( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) != MTC_NODE_COMPARISON_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );

    sNode->lflag &= ~(MTC_NODE_INDEX_MASK);

    // Node의 Execute를 설정
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    // Node의 Column 정보를 Boolean으로 설정
    /*
    IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLnnvlCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    LNNVL 연산을 수행함.
 *
 *    LNNVL과 일반 논리 연산자인 NOT과는 미묘한 차이가 있다.
 *
 *    NOT의 특징이 다음과 같은 논리 Matrix를 갖는 반면, LNNVL은 Unknown
 *    에 대한 처리가 NOT과 다르다.
 *
 *    ----------------------------------------------------------
 *          A         |    NOT ( A )       |   LNNVL ( A )
 *    ----------------------------------------------------------
 *         TRUE       |    FALSE           |   FALSE
 *    ----------------------------------------------------------
 *         FALSE      |    TRUE            |   TRUE
 *    ----------------------------------------------------------
 *         UNKNOWN    |    UNKNOWN         |   TRUE
 *    ----------------------------------------------------------
 *
 *    위의 Matrix에서 보듯이, LNNVL은 UNKNOWN에 대한 처리가 다르다.
 *    이는 LNNVL의 목적이 NOT과 달리 중복 Data를 제거하기 위함이기
 *    때문이다.
 *    예를 들어, 다음과 같은 논리식이 있다고 하자.
 *        Ex)    A OR B
 *        ==>    [A] concatenation [B AND LNNVL(A)]
 *    [A OR B]는 DNF로 처리 시 LNNVL을 이용한 변환이 이루어진다.
 *    먼저, [A] 부분에서는 [A]의 결과가 TRUE인 결과만 생성되고,
 *    FALSE및 UNKNOWN인 결과는 생성되지 않는다.
 *    [B] 부분에서는 [B]가 TRUE인 결과 중에서 [A]에 포함된 결과를
 *    제거해야 한다.  즉, [LNNVL(A)]에서는 [A]에서 선택된 결과들을
 *    제거하려면, (A)의 결과가 TRUE인 것만을 제거하여야 한다.
 *    즉, (A)의 결과가 FALSE이거나 UNKNOWN인 결과를 포함하여야 한다.
 *
 * Implementation :
 *
 *    Argument를 수행하고, Argument의 논리값에 따라
 *    Matrix에 의거한 값을 설정한다.
 *
 ***********************************************************************/

    // LNNVL 논리식 Matrix
    static const mtdBooleanType sMatrix[3] = {
        MTD_BOOLEAN_FALSE,   // TRUE인 경우, FALSE를 설정
        MTD_BOOLEAN_TRUE,    // FALSE인 경우, TRUE를 설정
        MTD_BOOLEAN_TRUE     // UNKNOWN인 경우, TRUE를 설정
    };

    // Argument를 수행
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    // Argument의 값과 Matrix를 이용하여 LNNVL의 값을 결정
    *(mtdBooleanType*)aStack[0].value =
                                    sMatrix[*(mtdBooleanType*)aStack[1].value];
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
