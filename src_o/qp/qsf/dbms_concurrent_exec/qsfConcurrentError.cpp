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
 * $Id: qsfConcurrentError.cpp 72995 2015-10-12 06:37:15Z kwsong $
 **********************************************************************/

#include <idl.h>
#include <mtc.h>
#include <mtk.h>
#include <qtc.h>
#include <qmcThr.h>
#include <qsc.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName qsfFunctionName1[1] = {
    { NULL, 22, (void*)"CONCURRENT_GET_ERRCODE" }
};

static mtcName qsfFunctionName2[1] = {
    { NULL, 21, (void*)"CONCURRENT_GET_ERRMSG" }
};

static mtcName qsfFunctionName3[1] = {
    { NULL, 23, (void*)"CONCURRENT_GET_ERRCOUNT" }
};

static mtcName qsfFunctionName4[1] = {
    { NULL, 19, (void*)"CONCURRENT_GET_TEXT" }
};

static mtcName qsfFunctionName5[1] = {
    { NULL, 21, (void*)"CONCURRENT_GET_ERRSEQ" }
};

static IDE_RC qsfEstimateGetErrSeq( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static IDE_RC qsfEstimateGetErrCode( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

static IDE_RC qsfEstimateGetErrMsg( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static IDE_RC qsfEstimateGetErrCnt( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static IDE_RC qsfEstimateGetText( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule qsfConcGetTextModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName4,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimateGetText
};

mtfModule qsfConcGetErrCntModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName3,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimateGetErrCnt
};

mtfModule qsfConcGetErrMsgModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName2,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimateGetErrMsg
};

mtfModule qsfConcGetErrSeqModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName5,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimateGetErrSeq
};

mtfModule qsfConcGetErrCodeModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName1,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimateGetErrCode
};


IDE_RC qsfCalculate_SpConcGetErrSeq( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC qsfCalculate_SpConcGetErrCode( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

IDE_RC qsfCalculate_SpConcGetErrMsg( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC qsfCalculate_SpConcGetErrCnt( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC qsfCalculate_SpConcGetText( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute qsfGetErrSeq = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcGetErrSeq,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute qsfGetErrCode = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcGetErrCode,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


static const mtcExecute qsfGetErrMsg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcGetErrMsg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute qsfGetText = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcGetText,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute qsfGetErrCnt = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpConcGetErrCnt,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimateGetText( mtcNode*     aNode,
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
                                     &mtdVarchar,
                                     0,
                                     8192, 
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfGetText;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsfEstimateGetErrCnt( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         /* aRemain */,
                             mtcCallBack* /* aCallBack */)
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0, 
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfGetErrCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfEstimateGetErrMsg( mtcNode*     aNode,
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
                                     &mtdVarchar,
                                     0,
                                     8192, 
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfGetErrMsg;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsfEstimateGetErrSeq( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfGetErrSeq;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfEstimateGetErrCode( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfGetErrCode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcGetErrCnt( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    qcStatement * sStatement;
    qscConcMgr  * sConcMgr;
    UInt          sErrCnt;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
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

    IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

    sErrCnt = sConcMgr->errCnt;

    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

    *((mtdIntegerType*)aStack[0].value) = sErrCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION_END;

    *((mtdIntegerType*)aStack[0].value) = 0;

    return IDE_FAILURE;
}


IDE_RC qsfCalculate_SpConcGetErrSeq( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    UInt          sReqID;
    qcStatement * sStatement;
    qscConcMgr  * sConcMgr;
    UInt          sErrSeq = ID_SINT_MAX;
    UInt          i;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
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

    sReqID = *(mtdIntegerType*)aStack[1].value;

    IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

    for ( i = 0; i < sConcMgr->errCnt; i++ )
    {
        if ( sConcMgr->errArr[i].reqID == sReqID )
        {
            sErrSeq = i;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

    *((mtdIntegerType*)aStack[0].value) = sErrSeq;

    return IDE_SUCCESS;
  
    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION_END;
   
    *((mtdIntegerType*)aStack[0].value) = sErrSeq;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcGetErrCode( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    UInt          sErrSeq;
    qcStatement * sStatement;
    qscConcMgr  * sConcMgr;
    UInt          sErrCode = 0;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
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

    sErrSeq = *(mtdIntegerType*)aStack[1].value;

    IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

    if ( sErrSeq < sConcMgr->errCnt )
    {
        sErrCode = sConcMgr->errArr[sErrSeq].errCode;
    }
    else
    {
        sErrCode = 0;
    }

    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

    *((mtdIntegerType*)aStack[0].value) = sErrCode;

    return IDE_SUCCESS;
  
    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION_END;
   
    *((mtdIntegerType*)aStack[0].value) = 0;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcGetText( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
    UInt          sErrSeq;
    qcStatement * sStatement;
    qscConcMgr  * sConcMgr;
    SChar       * sText;
    UInt          sTextLen = 0;
    mtdCharType * sReturnMsg;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
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

    sErrSeq = *(mtdIntegerType*)aStack[1].value;

    sReturnMsg = (mtdCharType*)aStack[0].value;

    sReturnMsg->length = 0;
    sReturnMsg->value[0] = '\0';

    IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

    if ( sErrSeq < sConcMgr->errCnt )
    {
        sText = sConcMgr->errArr[sErrSeq].execStr;

        if ( sText != NULL )
        {
            sTextLen = idlOS::strlen(sText);
            idlOS::strncpy( ((SChar*)sReturnMsg->value), sText, sTextLen );
        }
        else
        {
            sTextLen = 0;
        }
    }
    else
    {
        sTextLen = 0;
    }

    sReturnMsg->length = sTextLen;
    sReturnMsg->value[sTextLen] = '\0';

    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;
  
    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpConcGetErrMsg( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    UInt          sErrSeq;
    qcStatement * sStatement;
    qscConcMgr  * sConcMgr;
    SChar       * sErrMsg;
    UInt          sMsgLen = 0;
    mtdCharType * sReturnMsg;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;
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

    sErrSeq = *(mtdIntegerType*)aStack[1].value;

    sReturnMsg = (mtdCharType*)aStack[0].value;

    sReturnMsg->length = 0;
    sReturnMsg->value[0] = '\0';

    IDE_TEST( sConcMgr->mutex.lock(NULL) != IDE_SUCCESS );

    if ( sErrSeq < sConcMgr->errCnt )
    {
        sErrMsg = sConcMgr->errArr[sErrSeq].errMsg;

        if ( sErrMsg != NULL )
        {
            sMsgLen = idlOS::strlen(sErrMsg);
            idlOS::strncpy( ((SChar*)sReturnMsg->value), sErrMsg, sMsgLen );
        }
        else
        {
            sMsgLen = 0;
        }
    }
    else
    {
        sMsgLen = 0;
    }

    sReturnMsg->length = sMsgLen;
    sReturnMsg->value[sMsgLen] = '\0';

    IDE_TEST( sConcMgr->mutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;
 
    IDE_EXCEPTION(RECURSIVE_CALL_IS_NOT_ALLOWED)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

