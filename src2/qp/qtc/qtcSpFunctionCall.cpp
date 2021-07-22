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
 * $Id: qtcSpFunctionCall.cpp 90527 2021-04-09 04:25:41Z jayce.park $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <ide.h>

#include <ida.h>
#include <qtc.h>
#include <qs.h>
#include <qsv.h>
#include <qso.h>
#include <qsx.h>
#include <qsvEnv.h>
#include <mtdTypes.h>
#include <qsxUtil.h>
#include <qsxRelatedProc.h>
#include <qc.h>
#include <qcuProperty.h>
#include <qtcCache.h>

static mtcName qtcFunctionName[1] = {
    { NULL, 10, (void*)"SPFUNCCALL" }
};

static IDE_RC qtcEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC qtcCalculateStoredProcedure( mtcNode*,
                                           mtcStack* aStack,
                                           SInt,
                                           void*,
                                           mtcTemplate*);

mtfModule qtc::spFunctionCallModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_EAT_NULL_TRUE,
    ~0,
    1.0,              // default selectivity (비교 연산자 아님)
    qtcFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qtcEstimate
};

static const mtcExecute qtcExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qtcCalculateStoredProcedure,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qtcEstimate( mtcNode     * aNode,
                    mtcTemplate * aTemplate,
                    mtcStack    * aStack,
                    SInt        /* aRemain */,
                    mtcCallBack * aCallBack )
{
    qcStatement * sStatement = NULL;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = qtcExecute;

    sStatement = ((qtcNode *) aNode )->subquery;

    if ( ((qsExecParseTree*)(sStatement->myPlan->parseTree))->paramModules != NULL )
    {
        IDE_TEST( mtf::makeConversionNodes(
                      aNode,
                      aNode->arguments,
                      aTemplate,
                      aStack + 1,
                      aCallBack,
                      ((qsExecParseTree*)
                       (sStatement->myPlan->parseTree))->paramModules) != IDE_SUCCESS );
    }

    /* BUG-44382 clone tuple 성능개선 */
    // 복사와 초기화가 필요함
    qtc::setTupleColumnFlag( &(aTemplate->rows[aNode->table]),
                             ID_TRUE,
                             ID_TRUE );

    /* PROJ-2452 Cache for DETERMINISTIC function */
    IDE_TEST( qtcCache::validateFunctionCache( (qcTemplate *)aTemplate, (qtcNode *)aNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtcCalculateStoredProcedure( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    qtcNode             * sCallSpecNode;
    qsExecParseTree     * sExecPlanTree;
    qcStatement         * sStatement;
    UInt                  sArgCount;
    qmcCursor           * sOriCursorMgr = NULL;
    qmcdTempTableMgr    * sOriTempTableMgr = NULL;
    mtcStack            * sOriStack;
    SInt                  sOriStackRemain;
    // BUG-41279
    // Prevent parallel execution while executing 'select for update' clause.
    UInt                  sOriFlag;

    mtcColumn           * sColumn;
    UInt                  sStage = 0;
    iduMemoryStatus       sQmxMemStatus;

    // PROJ-2452
    qtcCacheObj   * sCacheObj = NULL;
    qtcCacheState   sState    = QTC_CACHE_STATE_BEGIN;
    UInt sParamCnt = 0;

    sStatement    = ((qcTemplate*)aTemplate)->stmt ;
    sCallSpecNode = (qtcNode *) aNode ;
    sExecPlanTree = (qsExecParseTree *) (sCallSpecNode->subquery->myPlan->parseTree );

    // exchange cursorMgr to get the opened cursors while evaluating
    // arguments. ( eg. subquery on argument )
    sOriCursorMgr = ((qcTemplate * ) aTemplate)->cursorMgr;
    sOriTempTableMgr = ((qcTemplate *) aTemplate)->tempTableMgr;

    sStage = 1;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sStage = 2;

    ((qcTemplate * ) aTemplate)->cursorMgr = sOriCursorMgr ;
    ((qcTemplate * ) aTemplate)->tempTableMgr = sOriTempTableMgr ;

    IDE_TEST( sStatement->qmxMem->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);

    sArgCount = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    sOriStack               = aTemplate->stack;
    sOriStackRemain         = aTemplate->stackRemain;
    aTemplate->stack        = aStack ;
    aTemplate->stackRemain  = aRemain ;

    sOriFlag = sStatement->spxEnv->mFlag;

    // BUG-45990
    // 최상위 statement인 경우에만 statement flag를 설정한다.
    if ( sStatement->spxEnv->mCallDepth == 0 )
    {
        QSX_ENV_SET_STMT_FLAG( sStatement );
    }

    sStage = 3;

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    IDE_TEST( qtcCache::searchCache( (qcTemplate *)aTemplate,
                                     sCallSpecNode,
                                     aStack,
                                     QTC_CACHE_TYPE_DETERMINISTIC_FUNCTION,
                                     &sCacheObj,
                                     &sParamCnt,
                                     &sState )
              != IDE_SUCCESS );

    switch ( sState )
    {
        case QTC_CACHE_STATE_INVOKE_MAKE_RECORD:
        case QTC_CACHE_STATE_RETURN_INVOKE:
            if ( qsxEnv::invokeWithStack( sStatement->spxEnv,
                                          sStatement,
                                          sExecPlanTree->procOID,
                                          sExecPlanTree->pkgBodyOID,
                                          sExecPlanTree->subprogramID,
                                          sExecPlanTree->callSpecNode,
                                          // return value , argument
                                          aStack + sArgCount + 1,
                                          aRemain - sArgCount - 1 )
                 == IDE_SUCCESS )
            {
                /* PROJ-2452 Caching for DETERMINISTIC Function */
                IDE_TEST( qtcCache::executeCache( QC_QXC_MEM( sStatement ),
                                                  aNode,
                                                  aStack,
                                                  sCacheObj,
                                                  ((qcTemplate *)aTemplate)->cacheBucketCnt,
                                                  sState,
                                                  ((qcTemplate *)aTemplate)->mSubqueryMode )
                          != IDE_SUCCESS );
            }
            else
            {
                // BUG-35713
                // sql로 부터 invoke되는 function에서 발생하는 no_data_found 에러는
                // 에러처리하지 않고 null을 반환한다.
                if ( ( ideGetErrorCode() == qpERR_ABORT_QSX_NO_DATA_FOUND ) &&
                     ( sStatement->spxEnv->mCallDepth == 0 ) &&
                     ( QCU_PSM_IGNORE_NO_DATA_FOUND_ERROR == 1 ) )
                {
                    ideClearError();

                    sColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(sStatement),
                                               sCallSpecNode );
                    
                    // sql에서 수행하는 function은 primitive type만 리턴한다.
                    IDE_TEST_RAISE( ( sColumn->module->id >= MTD_UDT_ID_MIN ) &&
                                    ( sColumn->module->id <= MTD_UDT_ID_MAX ),
                                    ERR_RETURN_TYPE );
                    
                    IDE_TEST( qsxUtil::assignNull( sCallSpecNode,
                                                   QC_PRIVATE_TMPLATE(sStatement) )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_RAISE( ERR_PASS );
                }
            }

            break;

        case QTC_CACHE_STATE_RETURN_CURR_RECORD:

            IDE_TEST( qtcCache::getCacheValue( aNode,
                                               aStack,
                                               sCacheObj,
                                               ((qcTemplate *)aTemplate)->mSubqueryMode )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_ERROR_RAISE( 0, ERR_UNEXPECTED_CACHE_ERROR );
            break;
    }

    sState = QTC_CACHE_STATE_END;

    aTemplate->stack       = sOriStack;
    aTemplate->stackRemain = sOriStackRemain;

    sStatement->spxEnv->mFlag = sOriFlag;

    sStage = 0;

    IDE_TEST( sStatement->qmxMem->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcCalculateStoredProcedure",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION( ERR_RETURN_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcCalculateStoredProcedure",
                                  "invalid return type" ));
    }
    IDE_EXCEPTION( ERR_PASS )
    {
        // propagation current error code
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        // 0~1 : postfix calculation failure
        // 2~3 : calculation failure

        case 3:
            ((qcTemplate * ) aTemplate)->cursorMgr    = sOriCursorMgr;
            ((qcTemplate * ) aTemplate)->tempTableMgr = sOriTempTableMgr;
            aTemplate->stack       = sOriStack;
            aTemplate->stackRemain = sOriStackRemain;

            sStatement->spxEnv->mFlag = sOriFlag;
            (void) sStatement->qmxMem->setStatus( &sQmxMemStatus );
        case 2: 
            break;
        case 1:
            ((qcTemplate * ) aTemplate)->cursorMgr    = sOriCursorMgr;
            ((qcTemplate * ) aTemplate)->tempTableMgr = sOriTempTableMgr;
        case 0:
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}
