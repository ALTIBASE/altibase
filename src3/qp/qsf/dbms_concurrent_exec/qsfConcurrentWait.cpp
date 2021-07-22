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
 * $Id: qsfConcurrentWait.cpp 72997 2015-10-12 07:13:45Z kwsong $
 **********************************************************************/

#include <idl.h>
#include <mtc.h>
#include <mtk.h>
#include <qtc.h>
#include <qmcThr.h>
#include <qsc.h>

extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 15, (void*)"CONCURRENT_WAIT" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfConcWaitModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpConcWait( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC qsfCalculate_SpConcWaitAll( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfWait = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcWait,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


static const mtcExecute qsfWaitAll= {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcWaitAll,
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

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0, 
                                     0 )
              != IDE_SUCCESS );

    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 0 )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = qsfWaitAll;
    }
    else
    {
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        aTemplate->rows[aNode->table].execute[aNode->column] = qsfWait;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcWait( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
    UInt          sReqID;
    SInt          sRc = 1;
    iduListNode * sIter;
    iduListNode * sIterNext;
    UInt          sStatus;
    qmcThrMgr   * sThrMgr;
    qmcThrObj  * sThrObj = NULL;
    qscExecInfo * sExecInfo = NULL;
    qscConcMgr  * sConcMgr = NULL;
    qcStatement * sStatement;
    UInt          sCurrReqID = ID_UINT_MAX;

    PDL_Time_Value sWaitTime;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
    sThrMgr    = sStatement->session->mQPSpecific.mThrMgr;
    sConcMgr   = sStatement->session->mQPSpecific.mConcMgr;
    sWaitTime.initialize(0, QCU_CONC_EXEC_WAIT_INTERVAL);

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (sStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_EXEC_MASK)
                    != QC_SESSION_INTERNAL_EXEC_FALSE,
                    RECURSIVE_CALL_IS_NOT_ALLOWED);

    sReqID = *(mtdIntegerType*)aStack[1].value;

    IDU_LIST_ITERATE_SAFE( &sThrMgr->mUseList, sIter, sIterNext )
    {
        IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

        sThrObj   = (qmcThrObj*)(sIter->mObj);
        sExecInfo  = (qscExecInfo*)(sThrObj->mPrivateArg);

        if ( sExecInfo != NULL )
        {
            sCurrReqID = sExecInfo->reqID;
        }
        else
        {
            sCurrReqID = ID_UINT_MAX;
        }

        IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

        if ( sCurrReqID == sReqID )
        {
            do
            {
                IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

                sThrObj  = (qmcThrObj*)(sIter->mObj);
                sExecInfo = (qscExecInfo*)(sThrObj->mPrivateArg);

                if ( sExecInfo == NULL )
                {
                    // Execution is already finished.
                    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );
                    break;
                }
                else
                {
                    sCurrReqID = sExecInfo->reqID;
                    sStatus = acpAtomicGet32(&sThrObj->mStatus);

                    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );
                }

                if ( sCurrReqID != sReqID )
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                switch ( sStatus )
                {
                case QMC_THR_STATUS_RUN:
                    idlOS::sleep( sWaitTime );
                    break;
                case QMC_THR_STATUS_WAIT:
                case QMC_THR_STATUS_END:
                case QMC_THR_STATUS_JOINED:
                    break;
                case QMC_THR_STATUS_NONE:
                case QMC_THR_STATUS_CREATED:
                case QMC_THR_STATUS_FAIL:
                    sRc = -1;
                    break;
                default :
                    // invalid value
                    IDE_ERROR_RAISE(0, ERR_INVALID_CONDITION);
                    break;
                }
            } while ( sStatus == QMC_THR_STATUS_RUN );

            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    *((mtdIntegerType*)aStack[0].value) = sRc;

    return IDE_SUCCESS;
 
    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qsfConcurrentWait",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;
    
    *((mtdIntegerType*)aStack[0].value) = -1;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcWaitAll( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
    qcStatement * sStatement;
    qmcThrMgr   * sThrMgr;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
    sThrMgr    = sStatement->session->mQPSpecific.mThrMgr;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (sStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_EXEC_MASK)
                    != QC_SESSION_INTERNAL_EXEC_FALSE,
                    RECURSIVE_CALL_IS_NOT_ALLOWED);

    // 실패하면 FATAL 발생한다.
    IDE_TEST( qmcThrJoin(sThrMgr) != IDE_SUCCESS );

    *((mtdIntegerType*)aStack[0].value) = (mtdIntegerType)1;

    return IDE_SUCCESS;

    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION_END;

    *((mtdIntegerType*)aStack[0].value) = (mtdIntegerType)-1;

    return IDE_FAILURE;
}

