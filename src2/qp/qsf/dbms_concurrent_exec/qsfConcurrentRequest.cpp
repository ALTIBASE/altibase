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
 * $Id: qsfConcurrentRequest.cpp 72997 2015-10-12 07:13:45Z kwsong $
 **********************************************************************/

#include <idl.h>
#include <mtc.h>
#include <mtk.h>
#include <qtc.h>
#include <qmcThr.h>
#include <qsc.h>

extern mtdModule mtdInteger;
extern mtdModule mtdChar;

static mtcName qsfFunctionName[1] = {
    { NULL, 18, (void*)"CONCURRENT_REQUEST" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfConcExecModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpConcRequest( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfRequest = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcRequest,
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
    static const mtdModule* sModules[2] = {
        &mtdInteger,
        &mtdChar
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfRequest;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcRequest( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
    qcStatement * sStatement;
    void        * sInternalSession = NULL;
    qmcThrMgr   * sThrMgr;
    qscConcMgr  * sConcMgr;
    qmcThrObj   * sThrObj = NULL;
    idBool        sIsSuccess;
    SChar       * sStr;
    UInt          sStrLen = 0;
    idBool        sIsRunStart = ID_FALSE;
    UInt          sCheckCount = 0;
    SInt          sReqID = 0;
    idBool        sIsLocked = ID_FALSE;

    UInt         sStatus;
    iduListNode* sIter;
    iduListNode* sIterNext;
    PDL_Time_Value sPDL_Time_Value;

    qscExecInfo * sExecInfo = NULL;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
    sThrMgr    = sStatement->session->mQPSpecific.mThrMgr;
    sConcMgr   = sStatement->session->mQPSpecific.mConcMgr;

    sPDL_Time_Value.initialize(0, QCU_CONC_EXEC_WAIT_INTERVAL);

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (sStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_EXEC_MASK)
                    != QC_SESSION_INTERNAL_EXEC_FALSE,
                    RECURSIVE_CALL_IS_NOT_ALLOWED);

    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
    IDU_FIT_POINT_RAISE("qsfCalculate_SpConcRequest::invalid::condition1",
                         ERR_INVALID_CONDITION);
    IDE_ERROR_RAISE( sThrMgr->mThrCnt != 0, ERR_INVALID_CONDITION );

    sReqID  = *(mtdIntegerType*)aStack[1].value;

    sStrLen = (UInt)((mtdCharType*)aStack[2].value)->length;
    sStr    = (SChar*)((mtdCharType*)aStack[2].value)->value;

    IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    // Error가 reserved thread count만큼 쌓인 경우 더이상 실행하지 않는다.
    IDE_TEST( sConcMgr->errCnt >= (sConcMgr->errMaxCnt  / 2) );

    //PROJ-2451 FIT TEST
    IDU_FIT_POINT("qsfCalculate_SpConcRequest::alloc::qscExecInfo",
                  idERR_ABORT_InsufficientMemory);

    IDE_TEST( sConcMgr->memory->alloc( ID_SIZEOF(qscExecInfo),
                                       (void**)&sExecInfo )
              != IDE_SUCCESS);
    sExecInfo->execStr = NULL;

    //PROJ-2451 FIT TEST
    IDU_FIT_POINT("qsfCalculate_SpConcRequest::alloc::execStr",
                  idERR_ABORT_InsufficientMemory);

    IDE_TEST( sConcMgr->memory->alloc( sStrLen + 1,
                                       (void**)&(sExecInfo->execStr) )
              != IDE_SUCCESS);

    sExecInfo->reqID   = sReqID;
    sExecInfo->concMgr = sConcMgr;

    sIsLocked = ID_FALSE;
    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

    idlOS::strncpy( sExecInfo->execStr, sStr, sStrLen );
    sExecInfo->execStr[sStrLen] = '\0';

    if ( qmcThrGet( sThrMgr,
                    qsc::execute,
                    sExecInfo,
                    &sThrObj )
         == IDE_SUCCESS )
    {
        // Thread를 가져온 경우
        if ( sThrObj != NULL )
        {
            sIsRunStart = ID_TRUE;

            IDE_TEST( qci::mSessionCallback.mAllocInternalSession(
                          &sInternalSession,
                          sStatement->session->mMmSession )
                      != IDE_SUCCESS );

            // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
            IDU_FIT_POINT_RAISE("qsfCalculate_SpConcRequest::invalid::condition2",
                                 ERR_INVALID_CONDITION);
            IDE_ERROR_RAISE( sInternalSession != NULL, ERR_INVALID_CONDITION );

            sExecInfo->mmSession = sInternalSession;
            sExecInfo->statistics = sStatement->mStatistics;

            IDE_TEST( qmcThrWakeup( sThrObj, &sIsSuccess ) != IDE_SUCCESS );

            IDE_TEST( sIsSuccess != ID_TRUE );
        }
        // Thread를 가져오지 못한 경우 (free list에 한 개도 없는 경우)
        else
        {
            // Nothing to do.
        }

        while ( sIsRunStart == ID_FALSE )
        {
            sCheckCount = 0;

            IDU_LIST_ITERATE_SAFE( &sThrMgr->mUseList, sIter, sIterNext )
            {
                sThrObj = (qmcThrObj*)(sIter->mObj);
                sStatus  = acpAtomicGet32(&sThrObj->mStatus);

                if ( sStatus == QMC_THR_STATUS_END )
                {
                    (void)acpAtomicSet32( &sThrObj->mStatus,
                                          QMC_THR_STATUS_WAIT );
                    sStatus = acpAtomicGet32(&sThrObj->mStatus);
                    IDE_TEST( sThrObj->join() != IDE_SUCCESS );
                    IDE_TEST( sThrObj->start() != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                if ( sStatus == QMC_THR_STATUS_WAIT )
                {
                    qmcThrReturn( sThrMgr, sThrObj );

                    IDE_ERROR_RAISE( qmcThrGet( sThrMgr,
                                                qsc::execute,
                                                sExecInfo,
                                                &sThrObj ) == IDE_SUCCESS,
                                     ERR_INVALID_CONDITION );

                    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
                    IDU_FIT_POINT_RAISE("qsfCalculate_SpConcRequest::invalid::condition3",
                                         ERR_INVALID_CONDITION);
                    IDE_ERROR_RAISE( sThrObj != NULL, ERR_INVALID_CONDITION );

                    IDE_TEST( qci::mSessionCallback.mAllocInternalSession(
                                  &sInternalSession,
                                  sStatement->session->mMmSession )
                              != IDE_SUCCESS );

                    // BUG-40281 Strength PROJ-2451 test cases for exceptional cases.
                    IDU_FIT_POINT_RAISE("qsfCalculate_SpConcRequest::invalid::condition4",
                                         ERR_INVALID_CONDITION);
                    IDE_ERROR_RAISE( sInternalSession != NULL, ERR_INVALID_CONDITION );

                    sExecInfo->mmSession = sInternalSession;
                    sExecInfo->statistics = sStatement->mStatistics;

                    IDE_TEST( qmcThrWakeup( sThrObj, &sIsSuccess ) != IDE_SUCCESS );

                    IDE_ERROR( sIsSuccess == ID_TRUE );

                    sIsRunStart = ID_TRUE;

                    break;
                }
                else
                {
                    // Nothing to do.
                }

                if ( ( sStatus == QMC_THR_STATUS_END ) ||
                     ( sStatus == QMC_THR_STATUS_JOINED ) ||
                     ( sStatus == QMC_THR_STATUS_FAIL ) )
                {
                    sCheckCount++;
                    IDE_TEST( sCheckCount == sThrMgr->mThrCnt );
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( sIsRunStart == ID_TRUE )
            {
                break;
            }
            else
            {
                idlOS::sleep( sPDL_Time_Value );
            }
        } // end of while
    }
    else
    {
        sReqID = -1;
    }

    *((mtdIntegerType*)aStack[0].value) = (mtdIntegerType)sReqID;

    return IDE_SUCCESS;

    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qsfConcurrentRequest",
                                "invliad condition"));
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        IDE_ASSERT( sConcMgr->mutex.unlock() == IDE_SUCCESS );
        sIsLocked = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    if ( sInternalSession != NULL )
    {
        qci::mSessionCallback.mFreeInternalSession( sInternalSession,
                                                    ID_FALSE /* aIsSuccess */ );
    }
    else
    {
        // Nothing to do.
    }

    // 실행에 실패한 경우 -1을 return 한다.
    *((mtdIntegerType*)aStack[0].value) = (mtdIntegerType)(-1);

    return IDE_FAILURE;
}

