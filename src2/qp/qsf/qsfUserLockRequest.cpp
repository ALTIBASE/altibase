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
 * $Id$
 *
 * Description : BUG-41307 User Lock 지원
 *
 * Syntax :
 *     USER_LOCK_REQUEST( id IN INTEGER )
 *     RETURN INTEGER;
 **********************************************************************/

#include <qc.h>
#include <qsf.h>
#include <qsxEnv.h>
#include <qcuSessionObj.h>

extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 17, (void *)"USER_LOCK_REQUEST" }
};

static IDE_RC qsfEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

mtfModule qsfUserLockRequestModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_UserLockRequest( mtcNode     * aNode,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     void        * aInfo,
                                     mtcTemplate * aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_UserLockRequest,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode     * aNode,
                    mtcTemplate * aTemplate,
                    mtcStack    * aStack,
                    SInt          /* aRemain */,
                    mtcCallBack * aCallBack )
{
    const mtdModule * sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        &sModule )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_UserLockRequest( mtcNode     * aNode,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     void        * aInfo,
                                     mtcTemplate * aTemplate )
{
    qcSession        * sSession;
    qcSessionObjInfo * sSessionObjInfo;
    SInt               sUserLockID;
    SInt               sResult;

    sSession = ((qcTemplate *)aTemplate)->stmt->spxEnv->mSession;
    sSessionObjInfo = sSession->mQPSpecific.mSessionObj;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value )
         == ID_TRUE )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        sUserLockID = *(mtdIntegerType *)aStack[1].value;

        IDE_TEST( qcuSessionObj::requestUserLock( sSessionObjInfo,
                                                  sUserLockID,
                                                  &sResult )
                  != IDE_SUCCESS );

        *(mtdIntegerType *)aStack[0].value = sResult;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
