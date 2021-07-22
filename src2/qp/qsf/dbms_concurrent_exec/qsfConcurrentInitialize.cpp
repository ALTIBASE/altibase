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
 * $Id: qsfConcurrentInitialize.cpp 70895 2015-05-21 04:45:32Z ksjall $
 **********************************************************************/

#include <idl.h>
#include <mtc.h>
#include <mtk.h>
#include <qtc.h>
#include <qmcThr.h>
#include <qsc.h>

extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 21, (void*)"CONCURRENT_INITIALIZE" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfConcInitializeModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpConcInitialize( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcInitialize,
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
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
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

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcInitialize( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    qcStatement * sStatement;
    qmcThrMgr   * sThrMgr;
    qscConcMgr  * sConcMgr;
    UInt          sThrCnt = 0;
    UInt          sStage = 0;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
    sThrMgr    = sStatement->session->mQPSpecific.mThrMgr;
    sConcMgr   = sStatement->session->mQPSpecific.mConcMgr;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (sStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_EXEC_MASK)
                    != QC_SESSION_INTERNAL_EXEC_FALSE,
                    RECURSIVE_CALL_IS_NOT_ALLOWED );

    sThrCnt = (UInt)(*((mtdIntegerType*)aStack[1].value));

    if ( sThrCnt == 0 )
    { 
        // If thread count is zero, then set DEFAULT DEGREE OF CONCURRENCY(=4).
        sThrCnt = QCU_CONC_EXEC_DEGREE_DEFAULT;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qcg::reserveConcThr( sThrCnt,
                                  &sThrCnt )
              != IDE_SUCCESS );
    sStage = 1;

    if ( sThrCnt > 0 )
    {
        IDE_TEST( qsc::prepare( sThrMgr,
                                sConcMgr,
                                sThrCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    *((mtdIntegerType*)aStack[0].value) = (mtdIntegerType)sThrCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION_END;

    if ( ( sStage == 1 ) && ( sThrCnt > 0 ) )
    {
        (void) qcg::releaseConcThr( sThrCnt );
    }
    else
    {
        // Nothing to do.
    }
    
    *((mtdIntegerType*)aStack[0].value) = (mtdIntegerType)-1;

    return IDE_FAILURE;
}

