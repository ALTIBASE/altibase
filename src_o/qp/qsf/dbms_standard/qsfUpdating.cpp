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
 * $Id: qsfUpdating.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <qsxEnv.h>

extern mtdModule mtdBoolean;

static mtcName qsfUpdatingFunctionName[1] = {
    { NULL, 9, (void*)"_UPDATING" }
};

static IDE_RC qsfUpdatingEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

IDE_RC qsfUpdatingCalculateNoArg(mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

IDE_RC qsfUpdatingCalculate1Arg(mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

mtfModule qsfUpdatingModule = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfUpdatingFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfUpdatingEstimate
};

const mtcExecute qsfExecuteNoArg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfUpdatingCalculateNoArg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute qsfExecute1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfUpdatingCalculate1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfUpdatingEstimate( mtcNode*  aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt         /* aRemain */,
                          mtcCallBack* /* aCallBack */)
{
    UInt sArgumentCount;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sArgumentCount = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    switch( sArgumentCount )
    {
        case 0:
            aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecuteNoArg;
            break;

        case 1:
            aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute1Arg;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdBoolean,
                                     0,
                                     0,
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

IDE_RC qsfUpdatingCalculateNoArg(mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
    qcStatement    * sStatement;
    mtdBooleanType * sReturnValue;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sStatement = ((qcTemplate*)aTemplate)->stmt;

    sReturnValue = (mtdBooleanType*)aStack[0].value;

    if ( sStatement->spxEnv->mTriggerEventType == QCM_TRIGGER_EVENT_UPDATE )
    {
        *sReturnValue = MTD_BOOLEAN_TRUE;
    }
    else
    {
        *sReturnValue = MTD_BOOLEAN_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qsfUpdatingCalculate1Arg(mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
    qcStatement    * sStatement;
    mtdBooleanType * sReturnValue;
    qcmColumn      * sUptColumn;
    mtdCharType    * sArgColName;
    mtdBooleanType   sRet = MTD_BOOLEAN_FALSE;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sStatement = ((qcTemplate*)aTemplate)->stmt;

    sReturnValue = (mtdBooleanType*)aStack[0].value;

    if ( sStatement->spxEnv->mTriggerEventType == QCM_TRIGGER_EVENT_UPDATE )
    {
        sArgColName = (mtdCharType*)aStack[1].value;

        for ( sUptColumn = sStatement->spxEnv->mTriggerUptColList;
              sUptColumn != NULL && sRet == MTD_BOOLEAN_FALSE;
              sUptColumn = sUptColumn->next )
        {
            if ( idlOS::strMatch( sArgColName->value,
                                  sArgColName->length,
                                  sUptColumn->namePos.stmtText + sUptColumn->namePos.offset,
                                  sUptColumn->namePos.size ) == 0 )
            {
                // offset이 0인 column은 trigger에서 변경한 column을 추가한 것이다.
                // 따라서 offset이 0인 경우는 제외한다.
                if ( sUptColumn->namePos.offset != 0 )
                {
                    sRet = MTD_BOOLEAN_TRUE;
                }
                else
                {
                    sRet = MTD_BOOLEAN_FALSE;
                }

                break;
            }
        }
    }

    *sReturnValue = sRet;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
