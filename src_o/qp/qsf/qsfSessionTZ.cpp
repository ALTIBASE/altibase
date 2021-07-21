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
 * $Id:
 **********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <mtc.h>

extern mtdModule mtdVarchar;

static mtcName qsfSessionTimezoneFunctionName[1] = {
    { NULL, 16, (void*)"SESSION_TIMEZONE" }
};

static IDE_RC qsfSessionTimezoneEstimate( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

IDE_RC qsfSessionTimezoneCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

mtfModule qsfSessionTimezoneModule = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfSessionTimezoneFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfSessionTimezoneEstimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfSessionTimezoneCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfSessionTimezoneEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         /* aRemain */,
                                   mtcCallBack* /* aCallBack */ )
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     MTC_TIMEZONE_NAME_LEN,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfSessionTimezoneCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    qcStatement    * sStatement;
    SChar          * sSessionTimezoneString;
    mtdCharType    * sResult;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sStatement = ((qcTemplate*)aTemplate)->stmt;
    
    sResult = (mtdCharType*)aStack[0].value;

    sSessionTimezoneString = QCG_GET_SESSION_TIMEZONE_STRING( sStatement );

    sResult->length = idlOS::strlen( sSessionTimezoneString );

    if ( sResult->length == 0 )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {   
        idlOS::memcpy( sResult->value, sSessionTimezoneString, sResult->length );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
