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
 * $Id: qsfGetErrorMessage.cpp 89988 2021-02-15 02:01:46Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <mtc.h>
#include <mtk.h>
#include <qtc.h>
#include <qsxEnv.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName qsfFunctionName[1] = {
    { NULL, 20, (void*)"SP_GET_ERROR_MESSAGE" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfGetErrorMessageModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpGetErrorMessage( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute qsfGetErrorMessage= {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpGetErrorMessage,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /* aRemain */,
                    mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdInteger
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_CODET_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     MAX_LAST_ERROR_MSG_LEN,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfGetErrorMessage;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CODET_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpGetErrorMessage( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
    SInt          sIndex;
    UInt          sStrLen;
    iduList     * sErrorList;
    iduList     * sNode;
    qcStatement * sStatement;
    mtdCharType * sReturnMsg;

    sStatement = ((qcTemplate*)aTemplate)->stmt;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sIndex = *(mtdIntegerType*)aStack[1].value - 1;

    IDE_TEST_RAISE( ((sIndex >= sStatement->spxEnv->mErrorListCount) ||
                     (sIndex < 0)),
                    ERR_INDEX_OVERFLOW );

    sReturnMsg = (mtdCharType*)aStack[0].value;
    sReturnMsg->length = 0;
    sReturnMsg->value[0] = '\0';

    sErrorList = &sStatement->spxEnv->mErrorList;

    if ( IDU_LIST_IS_EMPTY( sErrorList ) == ID_FALSE )
    {
        sNode = IDU_LIST_GET_FIRST( sErrorList );

        for ( ; (sIndex > 0) && (sNode != NULL); sIndex--, sNode = sNode->mNext )
            ;

        if ( (sIndex == 0) && (sNode != NULL) )
        {
            if ( sNode->mObj != NULL )
            {
                sStrLen = idlOS::strlen(((ideErrorCollectionStack*)sNode->mObj)->mErrorMessage);
                sStrLen = (sStrLen > (MAX_LAST_ERROR_MSG_LEN-1)) ? (MAX_LAST_ERROR_MSG_LEN-1):sStrLen;
                idlOS::strncpy( (SChar*)sReturnMsg->value,
                                ((ideErrorCollectionStack*)sNode->mObj)->mErrorMessage,
                                sStrLen );
                ((SChar*)sReturnMsg->value)[sStrLen] = '\0';
                sReturnMsg->length = sStrLen;
            }
            else
            {
                IDE_RAISE(ERR_UNEXPECTED);
            }
        }
        else
        {
            // Error list count가 0이 아닌데 error list가 비어있을 수 없다.
            IDE_RAISE(ERR_UNEXPECTED);
        }
    }

    return IDE_SUCCESS;
 
    IDE_EXCEPTION( ERR_INDEX_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_ARRAY_INDEX_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                 "qsfGetErrorMessage",
                                 "Error collection is unexpected status" ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

