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
 * $Id: qsfLastModifiedProwid.cpp 15573 2006-04-15 02:50:33Z sungminee $
 **********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <qtcRid.h>
#include <mtc.h>

static mtcName qsfLastModifiedProwidFunctionName[1] = {
    { NULL, 20, (void*)"LAST_MODIFIED_PROWID" }
};

static IDE_RC qsfLastModifiedProwidEstimate( mtcNode*     aNode,
                                             mtcTemplate* aTemplate,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             mtcCallBack* aCallBack );

IDE_RC qsfLastModifiedProwidCalculate(mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

mtfModule qsfLastModifiedProwidModule = {
    1 | MTC_NODE_OPERATOR_FUNCTION | MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfLastModifiedProwidFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfLastModifiedProwidEstimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfLastModifiedProwidCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfLastModifiedProwidEstimate( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         /* aRemain */,
                                      mtcCallBack* /* aCallBack */)
{
#define IDE_FN "IDE_RC qsfLastModifiedProwidEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    mtc::copyColumn( aStack[0].column, & gQtcRidColumn );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsfLastModifiedProwidCalculate(mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC qsfLastModifiedProwidCalculate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qcStatement  * sStatement = ((qcTemplate*)aTemplate)->stmt;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_DASSERT( ID_SIZEOF(mtdBigintType) == ID_SIZEOF(scGRID) );

    qcg::getLastModifiedRowGRID( sStatement,
                                 (scGRID*) aStack[0].value );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}
