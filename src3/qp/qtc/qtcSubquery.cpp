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
 * $Id: qtcSubquery.cpp 90527 2021-04-09 04:25:41Z jayce.park $
 *
 * Description :
 *
 *     Subquery 연산을 수행하는 Node
 *     One Column 형 Subquery 와 List(Multi-Column)형 Subquery를
 *     구분하여 처리하여야 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <qtc.h>
#include <qmv.h>
#include <qmvQuerySet.h>
#include <qmvQTC.h>
#include <qmn.h>
#include <qcuSqlSourceInfo.h>
#include <qcuProperty.h>
#include <qtcCache.h>
#include <qcgPlan.h>

extern mtdModule mtdList;

static IDE_RC qtcSubqueryEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

/* one column, single-row unknown */
static IDE_RC qtcSubqueryCalculateUnknown( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

/* one column, single-row sure */
static IDE_RC qtcSubqueryCalculateSure( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

/* list subquery, single-row sure */
static IDE_RC qtcSubqueryCalculateListSure( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

/* list subquery, single-row unknown, too many targets */
static IDE_RC qtcSubqueryCalculateListTwice( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

static idBool qtcSubqueryIs1RowSure(qcStatement* aStatement);

/* Subquery 연산자의 이름에 대한 정보 */
static mtcName mtfFunctionName[1] = {
    { NULL, 8, (void*)"SUBQUERY" }
};

/* Subquery 연산자의 Module 에 대한 정보 */
mtfModule qtc::subqueryModule = {
    1|                          // 하나의 Column 공간
    MTC_NODE_OPERATOR_SUBQUERY | MTC_NODE_HAVE_SUBQUERY_TRUE, // Subquery 연산자
    ~(MTC_NODE_INDEX_MASK),     // Indexable Mask : Column 정보가 아님.
    1.0,                        // default selectivity (비교 연산자 아님)
    mtfFunctionName,            // 이름 정보
    NULL,                       // Counter 연산자 없음
    mtf::initializeDefault,     // 서버 구동시 초기화 함수, 없음
    mtf::finalizeDefault,       // 서버 종료시 종료 함수, 없음
    qtcSubqueryEstimate         // Estimate 할 함수
};

static IDE_RC qtcSubqueryEstimate( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    Subquery 연산자에 대하여 Estimate 를 수행함.
 *    Subquery Node에 대한 Column 정보 및 Execute 정보를 Setting한다.
 *
 * Implementation :
 *
 *    Subquery의 Target Column의 개수를 세어,
 *    One-Column / List Subquery 의 여부를 판단하고, 이에 대한
 *    처리를 수행한다.
 *
 ***********************************************************************/

    UInt                 sCount;
    UInt                 sFence;
    SInt                 sRemain;
    idBool               sIs1RowSure;

    mtcStack*            sStack;
    mtcStack*            sStack2;
    mtcNode*             sNode;
    mtcNode*             sConvertedNode;

    qcTemplate         * sTemplate;
    qcStatement        * sStatement;
    qtcCallBackInfo    * sCallBackInfo;
    qtcNode            * sQtcNode;
    qmsTarget*           sTarget;
    qcuSqlSourceInfo     sqlInfo;
    qmsQuerySet        * sQuerySet = NULL;

    sCallBackInfo    = (qtcCallBackInfo*)((mtcCallBack*)aCallBack)->info;
    sTemplate        = sCallBackInfo->tmplate;
    sStack           = sTemplate->tmplate.stack;
    sRemain          = sTemplate->tmplate.stackRemain;
    sStatement       = ((qtcNode*)aNode)->subquery;

    sTemplate->tmplate.stack       = aStack + 1;
    sTemplate->tmplate.stackRemain = aRemain - 1;

    IDE_TEST_RAISE(sTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW);

    // set member of qcStatement
    sStatement->myPlan->parseTree->stmtKind = QCI_STMT_SELECT;

    // BUG-41104 statement 가 null 일때는 skip 해도 된다.
    // validate 단계에서 이미 설정되어 있다.
    if ( sCallBackInfo->statement != NULL )
    {
        sStatement->myPlan->planEnv = sCallBackInfo->statement->myPlan->planEnv;
        sStatement->spvEnv          = sCallBackInfo->statement->spvEnv;
        sStatement->mRandomPlanInfo = sCallBackInfo->statement->mRandomPlanInfo;

        /* PROJ-2197 PSM Renewal */
        sStatement->calledByPSMFlag = sCallBackInfo->statement->calledByPSMFlag;

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( sdi::setShardStmtType( sCallBackInfo->statement,
                                         sStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing todo
    }

    if( sCallBackInfo->SFWGH != NULL )
    {
        ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->lflag =
            sCallBackInfo->SFWGH->lflag;
    }
    else
    {
        ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->lflag = 0;
    }

    // BUG-20272
    ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->lflag &=
        ~QMV_QUERYSET_SUBQUERY_MASK;
    ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->lflag |=
        QMV_QUERYSET_SUBQUERY_TRUE;

    // BUG-41311
    IDE_TEST( qmv::validateLoop( sStatement ) != IDE_SUCCESS );

    // Query Set에 대한 Validation 수행
    IDE_TEST( qmvQuerySet::validate(
                  sStatement,
                  ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet,
                  sCallBackInfo->SFWGH, sCallBackInfo->from,
                  0)
              != IDE_SUCCESS );

    sStatement->calledByPSMFlag = ID_FALSE;

    // PROJ-2462 Result Cache
    // SubQuery의 Reulst Cache가않쓰였는지를 상위 QuerySet에 표시한다.
    if ( sCallBackInfo->querySet != NULL )
    {
        sCallBackInfo->querySet->lflag |= ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->lflag
                                         & QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-33225
    if( ((qmsParseTree*)sStatement->myPlan->parseTree)->limit != NULL )
    {
        IDE_TEST( qmv::validateLimit( sStatement,
                                      ((qmsParseTree*)sStatement->myPlan->parseTree)->limit )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // Subquery에는 Sequence를 사용할 수 없음
    if (sStatement->myPlan->parseTree->currValSeqs != NULL)
    {
        sqlInfo.setSourceInfo(
            sStatement,
            & sStatement->myPlan->parseTree->
            currValSeqs->sequenceNode->position );
        IDE_RAISE(ERR_USE_SEQUENCE_IN_SUBQUERY);
    }

    if (sStatement->myPlan->parseTree->nextValSeqs != NULL)
    {
        sqlInfo.setSourceInfo(
            sStatement,
            & sStatement->myPlan->parseTree->
            nextValSeqs->sequenceNode->position );
        IDE_RAISE(ERR_USE_SEQUENCE_IN_SUBQUERY);
    }

    // Subquery Node 에 대한 dependencies를 설정함
    // set dependencies
    qtc::dependencySetWithDep(
        & ((qtcNode *)aNode)->depInfo,
        & ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->outerDepInfo );

    // Subquery Node에 대한 Argument를 연결함
    // 즉, Subquery Target이 Subquery Node의 argument가 된다.
    sTarget = ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->target;

    aNode->arguments = (mtcNode*)sTarget->targetColumn;

    // Target Column의 개수를 세어
    // one-column subquery인지, List Subquery인지를 판단한다.
    for ( sNode   = aNode->arguments, sFence  = 0;
          sNode  != NULL;
          sNode   = sNode->next, sFence++ )
    {
        // BUG-36258
        if ((((qtcNode*)sNode)->lflag & QTC_NODE_COLUMN_RID_MASK) ==
            QTC_NODE_COLUMN_RID_EXIST)
        {
            IDE_RAISE(ERR_PROWID_NOT_SUPPORTED);
        }
    }

    IDE_TEST_RAISE( sFence > MTC_NODE_ARGUMENT_COUNT_MAXIMUM,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // Subquery Node에 Argument의 개수를 설정함
    aNode->lflag &= ~(MTC_NODE_ARGUMENT_COUNT_MASK);
    aNode->lflag |= sFence;

    // To fix PR-7904
    // Subquery가 존재함을 설정함
    sQtcNode = (qtcNode*) aNode;
    sQtcNode->lflag &= ~QTC_NODE_SUBQUERY_MASK;
    sQtcNode->lflag |= QTC_NODE_SUBQUERY_EXIST;

    /* PROJ-2283 Single-Row Subquery 개선 */
    sIs1RowSure = qtcSubqueryIs1RowSure(sStatement);

    //---------------------------------------------------------
    // Argument의 개수에 따라 처리 방식을 달리한다.
    // 즉, One-Column Subquery (sFence == 1)와
    // List Subquery (sFence > 1)인 경우에 따라 처리 방식이 다르다.
    // One-Column Subquery의 경우,
    //    - Indirection 방식으로 처리
    // List Subquery의 경우,
    //    - List 방식으로 처리
    //---------------------------------------------------------

    if( sFence == 1 )
    {
        //---------------------------------------------------------
        // One Column Subquery 에 대한 처리를 수행함.
        //---------------------------------------------------------

        // Subquery Node를 Indirection이 가능하도록 함.
        aNode->lflag |= MTC_NODE_INDIRECT_TRUE;

        // Column 정보로 skipModule을 사용함.
        IDE_TEST( mtc::initializeColumn( aTemplate->rows[aNode->table].columns
                                         + aNode->column,
                                         & qtc::skipModule,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

        // Argument의 Column 정보를 Stack에 설정하여
        // 상위 Node에서의 estimate 가 가능하도록 함.
        sConvertedNode = aNode->arguments;
        sConvertedNode = mtf::convertedNode( sConvertedNode,
                                             &sTemplate->tmplate );

        aStack[0].column = aTemplate->rows[sConvertedNode->table].columns
            + sConvertedNode->column;

        // One Column형 Subquery의 수행 함수를 설정함.
        aTemplate->rows[aNode->table].execute[aNode->column].initialize    = mtf::calculateNA;
        aTemplate->rows[aNode->table].execute[aNode->column].aggregate     = mtf::calculateNA;
        aTemplate->rows[aNode->table].execute[aNode->column].finalize      = mtf::calculateNA;
        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = NULL;
        aTemplate->rows[aNode->table].execute[aNode->column].estimateRange = mtk::estimateRangeNA;
        aTemplate->rows[aNode->table].execute[aNode->column].extractRange  = mtk::extractRangeNA;

        if (sIs1RowSure == ID_TRUE)
        {
            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                qtcSubqueryCalculateSure;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                qtcSubqueryCalculateUnknown;
        }
    }
    else
    {
        //---------------------------------------------------------
        // List Subquery 에 대한 처리를 수행함.
        //---------------------------------------------------------

        aStack[0].column =
            aTemplate->rows[aNode->table].columns + aNode->column;

        //---------------------------------------------------------
        // Subquery Node의 Column 정보를 list형 Data Type으로 설정함.
        // List형 타입의 estimation을 위한 인자 정보는 다음과 같이 결정된다.
        //
        // Ex) (I1, I2) IN ( (1,1), (2,1), (3,2) )
        //    Argument 개수 : 3
        //    Arguemnt의 Column 개수 : 2
        //
        // Subquery의 List 정보
        //    (I1, I2) IN ( SELECT A1, A2 FROM ... )
        //    Argument 개수 : 1
        //    Arguemnt의 Column 개수 : sFence
        //---------------------------------------------------------
        //IDE_TEST( mtdList.estimate( aStack[0].column, 1, sFence, 0 )
        //          != IDE_SUCCESS );

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdList,
                                         1,
                                         sFence,
                                         0 )
                  != IDE_SUCCESS );

        //-------------------------------------------------------------
        // 상위에서의 estimate 를 위해 Target 개수만큼의 처리를 수행한다.
        //-------------------------------------------------------------

        // 여러 Column의 처리를 위한 별도의 Stack 공간 할당
        IDU_FIT_POINT( "qtcSubquery::qtcSubqueryEstimate::alloc::Stack" );
        IDE_TEST(aCallBack->alloc( aCallBack->info,
                                   aStack[0].column->column.size,
                                   (void**)&(aStack[0].value))
                 != IDE_SUCCESS);

        // 별도로 할당된 공간에 여러 Column의 정보를 설정한다.
        sStack2 = (mtcStack*)aStack[0].value;

        for( sCount = 0, sNode = aNode->arguments;
             sCount < sFence;
             sCount++,   sNode = sNode->next )
        {
            sConvertedNode = sNode;
            sConvertedNode = mtf::convertedNode( sConvertedNode,
                                                 &sTemplate->tmplate );

            sStack2[sCount].column =
                aTemplate->rows[sConvertedNode->table].columns
                + sConvertedNode->column;
        }

        // List 형 Subquery의 수행 함수를 설정함.
        aTemplate->rows[aNode->table].execute[aNode->column].initialize    = mtf::calculateNA;
        aTemplate->rows[aNode->table].execute[aNode->column].aggregate     = mtf::calculateNA;
        aTemplate->rows[aNode->table].execute[aNode->column].finalize      = mtf::calculateNA;
        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = NULL;
        aTemplate->rows[aNode->table].execute[aNode->column].estimateRange = mtk::estimateRangeNA;
        aTemplate->rows[aNode->table].execute[aNode->column].extractRange  = mtk::extractRangeNA;

        if (sIs1RowSure == ID_TRUE)
        {
            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                qtcSubqueryCalculateListSure;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                qtcSubqueryCalculateListTwice;
        }
    }

    sTemplate->tmplate.stack       = sStack;
    sTemplate->tmplate.stackRemain = sRemain;

    /* PROJ-2448 Subquery caching */
    IDE_TEST( qtcCache::validateSubqueryCache( (qcTemplate *)aTemplate, sQtcNode )
              != IDE_SUCCESS );

    // BUG-43059 Target subquery unnest/removal disable
    if ( ( sCallBackInfo->querySet != NULL ) &&
         ( sCallBackInfo->querySet->processPhase == QMS_VALIDATE_TARGET ) )
    {
        sQuerySet = ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet;

        if ( QCU_OPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE == 1 )
        {
            sQuerySet->lflag &= ~QMV_QUERYSET_TARGET_SUBQUERY_UNNEST_MASK;
            sQuerySet->lflag |=  QMV_QUERYSET_TARGET_SUBQUERY_UNNEST_FALSE;
        }
        else
        {
            // Nothing to do.
        }

        if ( QCU_OPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE == 1 )
        {
            sQuerySet->lflag &= ~QMV_QUERYSET_TARGET_SUBQUERY_REMOVAL_MASK;
            sQuerySet->lflag |=  QMV_QUERYSET_TARGET_SUBQUERY_REMOVAL_FALSE;
        }
        else
        {
            // Nothing to do.
        }

        IDE_DASSERT( sCallBackInfo->statement != NULL );
        qcgPlan::registerPlanProperty( sCallBackInfo->statement,
                                       PLAN_PROPERTY_OPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE );
        qcgPlan::registerPlanProperty( sCallBackInfo->statement,
                                       PLAN_PROPERTY_OPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_USE_SEQUENCE_IN_SUBQUERY );
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_IN_SUBQUERY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {    
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }  
    IDE_EXCEPTION_END;

    sTemplate->tmplate.stack       = sStack;
    sTemplate->tmplate.stackRemain = sRemain;

    return IDE_FAILURE;
}

static IDE_RC qtcSubqueryCalculateSure( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * /* aInfo */,
                                        mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    One Column Subquery의 연산을 수행한다.
 *    본 함수는 One Row를 생성하는 Subquery의 수행에서만 호출된다.
 *    즉, 다음과 같은 Subquery의 수행에서만 호출된다.
 *       - ex1) INSERT INTO T1 VALUES ( one_row_subquery );
 *       - ex2) SELECT * FROM T1 WHERE i1 = one_row_subquery;
 *    반대되는 예로는 다음과 같은 것들이 있다.
 *       - ex1) INSERT INTO T1 multi_row_subquery;
 *       - ex2) SELECT * FROM T1 WHERE I1 in ( multi_row_subquery );
 *
 *    <PROJ-2283 Single-Row Subquery 개선>
 *    single-row sure subquery 인 경우만 호출된다.
 *    subquery의 결과는 항상 1개 이하이다.
 *    따라서 해당 subquery를 한번만 수행한다.
 *
 ***********************************************************************/

    qcStatement* sStatement;
    qmnPlan*     sPlan;
    mtcStack*    sStack;
    SInt         sRemain;
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;

    qtcCacheObj   * sCacheObj = NULL;
    qtcCacheState   sState    = QTC_CACHE_STATE_BEGIN;
    UInt sParamCnt = 0;

    sStack     = aTemplate->stack;
    sRemain    = aTemplate->stackRemain;
    sStatement = ((qtcNode*)aNode)->subquery;
    sPlan      = sStatement->myPlan->plan;

    /* PROJ-2448 Subquery caching */

    // BUG-43696 outerColumn 을 얻어오기 위한 stack 조정
    aTemplate->stack       = aStack  + 1;
    aTemplate->stackRemain = aRemain - 1;
    IDE_TEST_RAISE(aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW);

    // 해당 Subquery의 plan을 초기화한다.
    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan ) != IDE_SUCCESS );

    IDE_TEST( qtcCache::searchCache( (qcTemplate *)aTemplate,
                                     (qtcNode*)aNode,
                                     aStack,
                                     QTC_CACHE_TYPE_SCALAR_SUBQUERY_SURE,
                                     &sCacheObj,
                                     &sParamCnt,
                                     &sState )
              != IDE_SUCCESS );

    // 결과값을 얻어오기 위한 stack 재조정
    aTemplate->stack       = aStack  + 1 + sParamCnt;
    aTemplate->stackRemain = aRemain - 1 - sParamCnt;
    IDE_TEST_RAISE( aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW );

    switch ( sState )
    {
        case QTC_CACHE_STATE_INVOKE_MAKE_RECORD:
        case QTC_CACHE_STATE_RETURN_INVOKE:

            /* subquery 수행 */
            IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                // Nothing to do.
            }
            else
            {
                /* 결과가 없을 경우 null padding */
                IDE_TEST( sPlan->padNull( (qcTemplate*)aTemplate, sPlan )
                          != IDE_SUCCESS );
            }

            // Make cacheObj->currRecord and cache
            IDE_TEST( qtcCache::executeCache( QC_QXC_MEM( ((qcTemplate *)aTemplate)->stmt ),
                                              aNode,
                                              aStack,
                                              sCacheObj,
                                              ((qcTemplate *)aTemplate)->cacheBucketCnt,
                                              sState,
                                              ((qcTemplate *)aTemplate)->mSubqueryMode )
                      != IDE_SUCCESS );

            // Subquery의 결과를 나의 Stack에 쌓고, Subquery의 Plan을 종료한다.
            aStack[0] = aStack[1+sParamCnt];
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

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcSubquery::qtcSubqueryCalculateSure",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_FAILURE;
}

static IDE_RC qtcSubqueryCalculateUnknown( mtcNode     * aNode,
                                           mtcStack    * aStack,
                                           SInt          aRemain,
                                           void        * /*aInfo*/,
                                           mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    One Column Subquery의 연산을 수행한다.
 *    본 함수는 One Row를 생성하는 Subquery의 수행에서만 호출된다.
 *    즉, 다음과 같은 Subquery의 수행에서만 호출된다.
 *       - ex1) INSERT INTO T1 VALUES ( one_row_subquery );
 *       - ex2) SELECT * FROM T1 WHERE i1 = one_row_subquery;
 *    반대되는 예로는 다음과 같은 것들이 있다.
 *       - ex1) INSERT INTO T1 multi_row_subquery;
 *       - ex2) SELECT * FROM T1 WHERE I1 in ( multi_row_subquery );
 *
 * Implementation :
 *
 *    Subquery의 결과가 one row인지를 확인하여야 한다.
 *    따라서, 해당 Subquery를 두 번 수행하게 된다.
 *
 ***********************************************************************/

    qcStatement* sStatement;
    qmnPlan*     sPlan;
    mtcStack*    sStack;
    SInt         sRemain;
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;

    qtcCacheObj   * sCacheObj = NULL;
    qtcCacheState   sState    = QTC_CACHE_STATE_BEGIN;
    UInt sParamCnt = 0;

    /* BUG-48776 */
    qmncPROJ * sCodePlan     = NULL;
    qmndPROJ * sDataPlan     = NULL;
    qmcRowFlag sFlag2        = QMC_ROW_INITIALIZE;
    UInt       sActualSize   = 0;
    UInt       sMaxSize      = 0;

    sStack     = aTemplate->stack;
    sRemain    = aTemplate->stackRemain;
    sStatement = ((qtcNode*)aNode)->subquery;
    sPlan      = sStatement->myPlan->plan;

    /* PROJ-2448 Subquery caching */

    // BUG-43696 outerColumn 을 얻어오기 위한 stack 조정
    aTemplate->stack       = aStack  + 1;
    aTemplate->stackRemain = aRemain - 1;
    IDE_TEST_RAISE(aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW);

    // 해당 Subquery의 plan을 초기화한다.
    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan ) != IDE_SUCCESS );

    IDE_TEST( qtcCache::searchCache( (qcTemplate *)aTemplate,
                                     (qtcNode*)aNode,
                                     aStack,
                                     QTC_CACHE_TYPE_SCALAR_SUBQUERY_UNKNOWN,
                                     &sCacheObj,
                                     &sParamCnt,
                                     &sState )
              != IDE_SUCCESS );

    // 결과값을 얻어오기 위한 stack 재조정
    aTemplate->stack       = aStack  + 1 + sParamCnt;
    aTemplate->stackRemain = aRemain - 1 - sParamCnt;
    IDE_TEST_RAISE(aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW);

    switch ( sState )
    {
        case QTC_CACHE_STATE_INVOKE_MAKE_RECORD:
        case QTC_CACHE_STATE_RETURN_INVOKE:

            // 해당 Subquery의 plan을 수행한다.
            IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                      != IDE_SUCCESS );

            // 결과가 있을 경우, One Row Subquery인지 확인하여야 하며,
            // 결과가 없을 경우, Null Padding한다.
            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                if ( ( ((qcTemplate *)aTemplate)->mSubqueryMode == 1 ) &&
                     ( sPlan->type == QMN_PROJ ) )
                {
                    /* BUG-48776
                     * 기존 수행 ( QCU_PRESERVE_SCALAR_SUBQUERY_RESULT_DISABLE == 1 ) 의 경우
                     * Scalar sub-query unknown의 수행 시
                     * One row return인지 확인 하기 위해 다음 레코드를 doIt 해본 후
                     * 다시 리턴하여야 하는 원래의 record를 회복하기 위해 초기화 후 재수행 ( init -> doit ) 을 하고 있었다.
                     * 이는 sub-query가 outer query의 record만큼 최대 2번씩 수행되는 오버헤드가 있었고,
                     * 수행을 완료한 stack 으로부터
                     * First sub-query return value 를 백업, 복원하여 scalar sub-query의 본래 결과를 리턴하는 방식으로 로직을 변경한다.
                     */

                    sCodePlan = (qmncPROJ*)sPlan;
                    sDataPlan = (qmndPROJ*)(aTemplate->data + sPlan->offset);

                    if ( sDataPlan->mKeepColumn == NULL )
                    {
                        // 최초 수행인 경우 column keeping memory allocation
                        IDE_TEST( ((qcTemplate*)aTemplate)->stmt->qmxMem->alloc( ID_SIZEOF(mtcColumn),
                                                                                 (void**) & sDataPlan->mKeepColumn )
                                  != IDE_SUCCESS);
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    if ( sDataPlan->mKeepValue == NULL )
                    {
                        // Row의 최대 Size 계산
                        IDE_TEST( qmnPROJ::getMaxRowSize( (qcTemplate*)aTemplate,
                                                          sCodePlan,
                                                          &sMaxSize ) != IDE_SUCCESS );

                        // 최초 수행인 경우 value keeping memory allocation
                        IDE_TEST( ((qcTemplate*)aTemplate)->stmt->qmxMem->alloc( sMaxSize,
                                                                                 (void**) & sDataPlan->mKeepValue )
                                  != IDE_SUCCESS);
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    // column keeping memory에 backup
                    idlOS::memcpy( sDataPlan->mKeepColumn,
                                   aTemplate->stack->column,
                                   ID_SIZEOF(mtcColumn) );

                    // keeping 대상 value ( subquery return value )의 실제 크기를 구한다.
                    sActualSize = aTemplate->stack->column->module->actualSize( aTemplate->stack->column,
                                                                                aTemplate->stack->value );

                    // value keeping memory에 backup
                    idlOS::memcpy( sDataPlan->mKeepValue,
                                   aTemplate->stack->value,
                                   sActualSize );

                    // 결과가 One Row인지를 확인한다.
                    // 즉, 두 번째 수행해서 결과가 없는 지를 확인한다.
                    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag2 )
                              != IDE_SUCCESS );
                    IDE_TEST_RAISE( ( sFlag2 & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST,
                                    ERR_MULTIPLE_ROWS );

                    // keep column restore
                    aTemplate->stack->column = sDataPlan->mKeepColumn;
                    // keep value restore
                    aTemplate->stack->value = sDataPlan->mKeepValue;
                }
                else
                {
                    /*
                     * BUG-41784 subquery 수행방식을 PROJ-2283 이전으로 복원
                     */

                    // 결과가 One Row인지를 확인한다.
                    // 즉, 두 번째 수행해서 결과가 없는 지를 확인한다.
                    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                              != IDE_SUCCESS );
                    IDE_TEST_RAISE( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST,
                                    ERR_MULTIPLE_ROWS );

                    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan )
                              != IDE_SUCCESS );
                    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                              != IDE_SUCCESS );

                    IDE_ERROR_RAISE( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST,
                                     ERR_SUBQUERY_RETURN_VALUE_CHANGED );

                }
            }
            else
            {
                IDE_TEST( sPlan->padNull( (qcTemplate*)aTemplate, sPlan )
                          != IDE_SUCCESS );
            }

            // Make cacheObj->currRecord and cache
            IDE_TEST( qtcCache::executeCache( QC_QXC_MEM( ((qcTemplate *)aTemplate)->stmt ),
                                              aNode,
                                              aStack,
                                              sCacheObj,
                                              ((qcTemplate *)aTemplate)->cacheBucketCnt,
                                              sState,
                                              ((qcTemplate *)aTemplate)->mSubqueryMode )
                          != IDE_SUCCESS );

            // Subquery의 결과를 나의 Stack에 쌓고, Subquery의 Plan을 종료한다.
            aStack[0] = aStack[1+sParamCnt];
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

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MULTIPLE_ROWS )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_MULTIPLE_ROWS));
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcSubquery::qtcSubqueryCalculateUnknown",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION( ERR_SUBQUERY_RETURN_VALUE_CHANGED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_SUBQUERY_RETURN_VALUE_CHANGED ) );
    }
    IDE_EXCEPTION_END;

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_FAILURE;
}

static IDE_RC qtcSubqueryCalculateListSure( mtcNode     * aNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * /* aInfo */,
                                            mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    List Subquery의 연산을 수행한다.
 *    본 함수는 One Row를 생성하는 Subquery의 수행에서만 호출된다.
 *
 *    <PROJ-2283 Single-Row Subquery 개선>
 *    single-row sure subquery 인 경우만 호출된다.
 *    subquery의 결과는 항상 1개 이하이다.
 *    따라서 해당 subquery를 한번만 수행한다.
 *
 ***********************************************************************/

    qcStatement* sStatement;
    qmnPlan*     sPlan;
    mtcStack*    sStack;
    SInt         sRemain;
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;

    qtcCacheObj  * sCacheObj = NULL;
    qtcCacheState  sState = QTC_CACHE_STATE_BEGIN;
    UInt sParamCnt = 0;

    sStack     = aTemplate->stack;
    sRemain    = aTemplate->stackRemain;
    sStatement = ((qtcNode*)aNode)->subquery;
    sPlan      = sStatement->myPlan->plan;

    /* PROJ-2448 Subquery caching */

    // BUG-43696 outerColumn 을 얻어오기 위한 stack 조정
    aTemplate->stack       = aStack  + 1;
    aTemplate->stackRemain = aRemain - 1;
    IDE_TEST_RAISE(aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW);

    // 해당 Subquery의 plan을 초기화한다.
    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan ) != IDE_SUCCESS );

    IDE_TEST( qtcCache::searchCache( (qcTemplate *)aTemplate,
                                     (qtcNode*)aNode,
                                     aStack,
                                     QTC_CACHE_TYPE_LIST_SUBQUERY_SURE,
                                     &sCacheObj,
                                     &sParamCnt,
                                     &sState )
              != IDE_SUCCESS );

    // 결과값을 얻어오기 위한 stack 재조정
    aTemplate->stack       = aStack  + 1 + sParamCnt;
    aTemplate->stackRemain = aRemain - 1 - sParamCnt;
    IDE_TEST_RAISE( aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW );

    // 여러 Column의 데이터를 얻기 위해
    // estimate 시점에 할당된 영역을 Stack에 지정한다.
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)((UChar*) aTemplate->rows[aNode->table].row
                               + aStack[0].column->column.offset);

    switch ( sState )
    {
        case QTC_CACHE_STATE_INVOKE_MAKE_RECORD:
        case QTC_CACHE_STATE_RETURN_INVOKE:

            // 해당 Subquery의 plan을 수행한다.
            IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
            }
            else
            {
                IDE_TEST( sPlan->padNull( (qcTemplate*)aTemplate, sPlan )
                          != IDE_SUCCESS );
            }

            // Make cacheObj->currRecord and cache
            IDE_TEST( qtcCache::executeCache( QC_QXC_MEM( ((qcTemplate *)aTemplate)->stmt ),
                                              aNode,
                                              aStack,
                                              sCacheObj,
                                              ((qcTemplate *)aTemplate)->cacheBucketCnt,
                                              sState,
                                              ((qcTemplate *)aTemplate)->mSubqueryMode )
                      != IDE_SUCCESS );

            // Subquery의 Target 정보가 생성된 Stack을 통째로 지정한 영역에 복사한다.
            idlOS::memcpy( aStack[0].value,
                           aStack + 1 + sParamCnt,
                           aStack[0].column->column.size );
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

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcSubquery::qtcSubqueryCalculateListSure",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_FAILURE;
}

/*
 * subquery 의 target 개수가 너무 많아서
 * single-row tuple 을 할당받을 수 없는 경우
 * 어쩔수 없이 옛날 방식대로 subquery 를 두번 수행한다.
 */
IDE_RC qtcSubqueryCalculateListTwice( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * /*aInfo*/,
                                      mtcTemplate * aTemplate )
{
    qcStatement* sStatement;
    qmnPlan*     sPlan;
    mtcStack*    sStack;
    SInt         sRemain;
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;

    qtcCacheObj   * sCacheObj = NULL;
    qtcCacheState   sState = QTC_CACHE_STATE_BEGIN;
    UInt sParamCnt = 0;

    /* BUG-48776 */
    qmncPROJ  * sCodePlan   = NULL;
    qmndPROJ  * sDataPlan   = NULL;
    qmcRowFlag  sFlag2      = QMC_ROW_INITIALIZE;
    UInt        sFence      = 0;
    UInt        i           = 0;
    UInt        sOffset     = 0;
    UInt        sActualSize = 0;
    UInt        sMaxSize    = 0;

    sStack     = aTemplate->stack;
    sRemain    = aTemplate->stackRemain;
    sStatement = ((qtcNode*)aNode)->subquery;
    sPlan      = sStatement->myPlan->plan;

    /* PROJ-2448 Subquery caching */

    // BUG-43696 outerColumn 을 얻어오기 위한 stack 조정
    aTemplate->stack       = aStack  + 1;
    aTemplate->stackRemain = aRemain - 1;
    IDE_TEST_RAISE(aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW);

    // 해당 Subquery의 plan을 초기화한다.
    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan ) != IDE_SUCCESS );

    IDE_TEST( qtcCache::searchCache( (qcTemplate *)aTemplate,
                                     (qtcNode*)aNode,
                                     aStack,
                                     QTC_CACHE_TYPE_LIST_SUBQUERY_TWICE,
                                     &sCacheObj,
                                     &sParamCnt,
                                     &sState )
              != IDE_SUCCESS );

    // 결과값을 얻어오기 위한 stack 재조정
    aTemplate->stack       = aStack  + 1 + sParamCnt;
    aTemplate->stackRemain = aRemain - 1 - sParamCnt;
    IDE_TEST_RAISE( aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW );

    // 여러 Column의 데이터를 얻기 위해
    // Unbound 시점에 할당된 영역을 Stack에 지정한다.
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)((UChar*) aTemplate->rows[aNode->table].row
                               + aStack[0].column->column.offset);

    switch ( sState )
    {
        case QTC_CACHE_STATE_INVOKE_MAKE_RECORD:
        case QTC_CACHE_STATE_RETURN_INVOKE:

            // 해당 Subquery의 plan을 수행한다.
            IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                if ( ( ((qcTemplate *)aTemplate)->mSubqueryMode == 1 ) &&
                     ( sPlan->type == QMN_PROJ ) )
                {
                    /* BUG-48776
                     * 기존 수행 ( QCU_PRESERVE_SCALAR_SUBQUERY_RESULT_DISABLE == 1 ) 의 경우
                     * Scalar sub-query unknown의 수행 시
                     * One row return인지 확인 하기 위해 다음 레코드를 doIt 해본 후
                     * 다시 리턴하여야 하는 원래의 record를 회복하기 위해 초기화 후 재수행 ( init -> doit ) 을 하고 있었다.
                     * 이는 sub-query가 outer query의 record만큼 최대 2번씩 수행되는 오버헤드가 있었고,
                     * 수행을 완료한 stack 으로부터
                     * First sub-query return value 를 백업, 복원하여 scalar sub-query의 본래 결과를 리턴하는 방식으로 로직을 변경한다.
                     */

                    sCodePlan = (qmncPROJ*) sPlan;
                    sDataPlan = (qmndPROJ*) (aTemplate->data + sPlan->offset);

                    // List의 argument 갯수를 구한다. ( = subquery return value count )
                    sFence = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

                    if ( sDataPlan->mKeepColumn == NULL )
                    {
                        // 최초 수행인 경우 column keeping memory allocation
                        IDE_TEST( ((qcTemplate*)aTemplate)->stmt->qmxMem->alloc( sFence * ID_SIZEOF(mtcColumn),
                                                                                 (void**) & sDataPlan->mKeepColumn )
                                  != IDE_SUCCESS);
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    if ( sDataPlan->mKeepValue == NULL )
                    {
                        // Row의 최대 Size 계산
                        IDE_TEST( qmnPROJ::getMaxRowSize( (qcTemplate*)aTemplate,
                                                          sCodePlan,
                                                          &sMaxSize ) != IDE_SUCCESS );

                        // 최초 수행인 경우 value keeping memory allocation
                        IDE_TEST( ((qcTemplate*)aTemplate)->stmt->qmxMem->alloc( sMaxSize,
                                                                                 (void**) & sDataPlan->mKeepValue )
                                  != IDE_SUCCESS);
                    }

                    for ( i = 0, sOffset = 0;
                          i < sFence;
                          i++ )
                    {
                        // column keeping memory에 backup
                        idlOS::memcpy( sDataPlan->mKeepColumn + i,
                                       (aStack + 1 + sParamCnt + i)->column,
                                       ID_SIZEOF(mtcColumn) );

                        sActualSize = (aStack + 1 + sParamCnt + i)->column->module->actualSize( (aStack + 1 + sParamCnt + i)->column,
                                                                                                (aStack + 1 + sParamCnt + i)->value );

                        // value keeping memory에 backup
                        idlOS::memcpy( (SChar*)sDataPlan->mKeepValue + sOffset,
                                       (aStack + 1 + sParamCnt + i)->value,
                                       sActualSize );

                        sOffset = idlOS::align(sOffset, (aStack + 1 + sParamCnt + i)->column->module->align);
                        sOffset += (aStack + 1 + sParamCnt + i)->column->column.size;
                    }

                    // 결과가 One Row인지를 확인한다.
                    // 즉, 두 번째 수행해서 결과가 없는 지를 확인한다.
                    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag2 )
                              != IDE_SUCCESS );
                    IDE_TEST_RAISE( ( sFlag2 & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST,
                                    ERR_MULTIPLE_ROWS );

                    for ( i = 0, sOffset = 0;
                          i < sFence;
                          i++ )
                    {
                        // keep column restore
                        (aStack + 1 + sParamCnt + i)->column = (sDataPlan->mKeepColumn + i);
                        // keep value restore
                        (aStack + 1 + sParamCnt + i)->value = (void*)((SChar*)sDataPlan->mKeepValue + sOffset);

                        sOffset = idlOS::align(sOffset, (aStack + 1 + sParamCnt + i)->column->module->align);
                        sOffset += (aStack + 1 + sParamCnt + i)->column->column.size;
                    }
                }
                else
                {
                    // 결과가 One Row인지를 확인한다.
                    // 즉, 두 번째 수행해서 결과가 없는 지를 확인한다.
                    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                              != IDE_SUCCESS );
                    IDE_TEST_RAISE( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST,
                                    ERR_MULTIPLE_ROWS );

                    // 두 번째 수행으로 Tuple Set이 변경되므로,
                    // 다시 한 번 수행하여야 한다.

                    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan  )
                              != IDE_SUCCESS );

                    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
                              != IDE_SUCCESS );
                    IDE_TEST_RAISE( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE,
                                    ERR_SUBQUERY_RETURN_VALUE_CHANGED );
                }
            }
            else
            {
                IDE_TEST( sPlan->padNull( (qcTemplate*)aTemplate, sPlan )
                          != IDE_SUCCESS );
            }

            // Make cacheObj->currRecord and cache
            IDE_TEST( qtcCache::executeCache( QC_QXC_MEM( ((qcTemplate *)aTemplate)->stmt ),
                                              aNode,
                                              aStack,
                                              sCacheObj,
                                              ((qcTemplate *)aTemplate)->cacheBucketCnt,
                                              sState,
                                              ((qcTemplate *)aTemplate)->mSubqueryMode )
                      != IDE_SUCCESS );

            // Subquery의 Target 정보가 생성된 Stack을 통째로 지정한 영역에 복사한다.
            idlOS::memcpy( aStack[0].value,
                           aStack + 1 + sParamCnt,
                           aStack[0].column->column.size );
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

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MULTIPLE_ROWS )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_MULTIPLE_ROWS));
    }
    IDE_EXCEPTION( ERR_SUBQUERY_RETURN_VALUE_CHANGED )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_SUBQUERY_RETURN_VALUE_CHANGED));
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcSubquery::qtcSubqueryCalculateTwice",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_FAILURE;
}

static idBool qtcSubqueryIs1RowSure(qcStatement* aStatement)
{
    qmsQuerySet* sQuerySet;

    IDE_DASSERT(aStatement != NULL);
    IDE_DASSERT(aStatement->myPlan != NULL);
    IDE_DASSERT(aStatement->myPlan->parseTree != NULL);

    sQuerySet = ((qmsParseTree*)aStatement->myPlan->parseTree)->querySet;

    IDE_DASSERT(sQuerySet != NULL);

    if (sQuerySet->analyticFuncList != NULL)
    {
        /* analytic function 존재하면 항상 false */
        return ID_FALSE;
    }

    if (sQuerySet->SFWGH == NULL)
    {
        /* subquery 가 set operation 으로 연결된 경우 항상 false */
        return ID_FALSE;
    }

    if (sQuerySet->SFWGH->group == NULL)
    {
        if (sQuerySet->SFWGH->aggsDepth1 != NULL)
        {
            /* group by 가 없고 aggregation 이 있는 경우 */
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }
    else
    {
        if (sQuerySet->SFWGH->aggsDepth2 != NULL)
        {
            /* group by 가 있고 nested aggregation 이 있는 경우 */
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }
}

IDE_RC qtcSubqueryCalculateExists( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * /* aInfo */,
                                   mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-2448 Subquery caching
 *
 *               EXISTS 의 인자로 사용된 subquery 의 calculation
 *
 * Implementation :
 *
 *               aStack[0] 은 EXISTS 의 stack 으로 true/false 를 담는다.
 *
 ***********************************************************************/

    qcStatement * sStatement = NULL;
    qmnPlan     * sPlan = NULL;
    mtcStack    * sStack = NULL;
    SInt          sRemain = 0;
    qmcRowFlag    sFlag = QMC_ROW_INITIALIZE;

    qtcCacheObj   * sCacheObj = NULL;
    qtcCacheState   sState    = QTC_CACHE_STATE_BEGIN;
    UInt sParamCnt = 0;

    sStack  = aTemplate->stack;
    sRemain = aTemplate->stackRemain;

    sStatement = ((qtcNode*)aNode)->subquery;
    sPlan      = sStatement->myPlan->plan;

    /* PROJ-2448 Subquery caching */

    // BUG-43696 outerColumn 을 얻어오기 위한 stack 조정
    aTemplate->stack       = aStack  + 1;
    aTemplate->stackRemain = aRemain - 1;
    IDE_TEST_RAISE(aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW);

    // 해당 Subquery의 plan을 초기화한다.
    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan ) != IDE_SUCCESS );

    IDE_TEST( qtcCache::searchCache( (qcTemplate *)aTemplate,
                                     (qtcNode*)aNode,
                                     aStack,
                                     QTC_CACHE_TYPE_EXISTS_SUBQUERY,
                                     &sCacheObj,
                                     &sParamCnt,
                                     &sState )
              != IDE_SUCCESS );

    // 결과값을 얻어오기 위한 stack 재조정
    aTemplate->stack       = aStack  + 1 + sParamCnt;
    aTemplate->stackRemain = aRemain - 1 - sParamCnt;
    IDE_TEST_RAISE( aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW );

    switch ( sState )
    {
        case QTC_CACHE_STATE_INVOKE_MAKE_RECORD:
        case QTC_CACHE_STATE_RETURN_INVOKE:

            IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag ) != IDE_SUCCESS );

            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
            }
            else
            {
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
            }

            // Make cacheObj->currRecord and cache
            IDE_TEST( qtcCache::executeCache( QC_QXC_MEM( ((qcTemplate *)aTemplate)->stmt ),
                                              aNode,
                                              aStack,
                                              sCacheObj,
                                              ((qcTemplate *)aTemplate)->cacheBucketCnt,
                                              sState,
                                              ((qcTemplate *)aTemplate)->mSubqueryMode )
                      != IDE_SUCCESS );
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

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcSubquery::qtcSubqueryCalculateExists",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtcSubqueryCalculateNotExists( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * /* aInfo */,
                                      mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-2448 Subquery caching
 *
 *               NOT EXISTS 의 인자로 사용된 subquery 의 calculation
 *
 * Implementation :
 *
 *               aStack[0] 은 NOT EXISTS 의 stack 으로 true/false 를 담는다.
 *
 ***********************************************************************/

    qcStatement * sStatement = NULL;
    qmnPlan     * sPlan = NULL;
    mtcStack    * sStack = NULL;
    SInt          sRemain = 0;
    qmcRowFlag    sFlag = QMC_ROW_INITIALIZE;

    qtcCacheObj   * sCacheObj = NULL;
    qtcCacheState   sState    = QTC_CACHE_STATE_BEGIN;
    UInt sParamCnt = 0;

    sStack  = aTemplate->stack;
    sRemain = aTemplate->stackRemain;

    sStatement = ((qtcNode*)aNode)->subquery;
    sPlan      = sStatement->myPlan->plan;

    /* PROJ-2448 Subquery caching */

    // BUG-43696 outerColumn 을 얻어오기 위한 stack 조정
    aTemplate->stack       = aStack  + 1;
    aTemplate->stackRemain = aRemain - 1;
    IDE_TEST_RAISE(aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW);

    // 해당 Subquery의 plan을 초기화한다.
    IDE_TEST( sPlan->init( (qcTemplate*)aTemplate, sPlan ) != IDE_SUCCESS );

    IDE_TEST( qtcCache::searchCache( (qcTemplate *)aTemplate,
                                     (qtcNode*)aNode,
                                     aStack,
                                     QTC_CACHE_TYPE_NOT_EXISTS_SUBQUERY,
                                     &sCacheObj,
                                     &sParamCnt,
                                     &sState )
              != IDE_SUCCESS );

    // 결과값을 얻어오기 위한 stack 재조정
    aTemplate->stack       = aStack  + 1 + sParamCnt;
    aTemplate->stackRemain = aRemain - 1 - sParamCnt;
    IDE_TEST_RAISE( aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW );

    switch ( sState )
    {
        case QTC_CACHE_STATE_INVOKE_MAKE_RECORD:
        case QTC_CACHE_STATE_RETURN_INVOKE:

            IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag ) != IDE_SUCCESS );

            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
            }
            else
            {
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
            }

            // Make cacheObj->currRecord and cache
            IDE_TEST( qtcCache::executeCache( QC_QXC_MEM( ((qcTemplate *)aTemplate)->stmt ),
                                              aNode,
                                              aStack,
                                              sCacheObj,
                                              ((qcTemplate *)aTemplate)->cacheBucketCnt,
                                              sState,
                                              ((qcTemplate *)aTemplate)->mSubqueryMode )
                      != IDE_SUCCESS );
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

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcSubquery::qtcSubqueryCalculateNotExists",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
