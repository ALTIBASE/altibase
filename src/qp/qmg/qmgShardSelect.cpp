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
 * $Id: qmgShardSELT.cpp 77650 2016-10-17 02:19:47Z timmy.kim $
 *
 * Description : Shard Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcg.h>
#include <qmo.h>
#include <qcgPlan.h>
#include <qmgShardSelect.h>
#include <qmgShardDML.h>
#include <qmgSelection.h>
#include <qmoOneNonPlan.h>
#include <qmgSorting.h>            /* TASK-7219 */
#include <qmoUtil.h>               /* TASK-7219 */
#include <qmoPushPred.h>           /* TASK-7219 */
#include <qmoCheckViewColumnRef.h> /* TASK-7219 */
#include <qmvShardTransform.h>     /* TASK-7219 Shard Transformer Refactoring */

IDE_RC qmgShardSelect::init( qcStatement  * aStatement,
                             qmsQuerySet  * aQuerySet,
                             qmsFrom      * aFrom,
                             qmgGraph    ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgShardSELT Graph의 초기화
 *
 * Implementation :
 *
 *            - alloc qmgShardSELT
 *            - init qmgShardSELT, qmgGraph
 *
 ***********************************************************************/

    qmgShardSELT * sMyGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmgShardSelect::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aQuerySet != NULL );
    IDE_FT_ASSERT( aFrom != NULL );

    //---------------------------------------------------
    // Shard Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgShardSELT을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgShardSELT ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_SHARD_SELECT;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aFrom->depInfo );
    sMyGraph->graph.myFrom = aFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgShardSelect::optimize;
    sMyGraph->graph.makePlan = qmgShardSelect::makePlan;
    sMyGraph->graph.printGraph = qmgShardSelect::printGraph;

    // BUGBUG
    // SDSE 자체는 disk 로 생성하나
    // SDSE 의 상위 plan 의 interResultType 은 memory temp 로 수행되어야 한다.
    sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
    sMyGraph->graph.flag |=  QMG_GRAPH_TYPE_DISK;

    //---------------------------------------------------
    // Shard 고유 정보의 초기화
    //---------------------------------------------------

    SET_POSITION( sMyGraph->shardQuery,
                  aFrom->tableRef->view->myPlan->parseTree->stmtPos );
    sMyGraph->shardAnalysis = aFrom->tableRef->view->myPlan->mShardAnalysis;
    sMyGraph->shardParamInfo = aFrom->tableRef->view->myPlan->mShardParamInfo; /* TASK-7219 Non-shard DML */
    sMyGraph->shardParamOffset = aFrom->tableRef->view->myPlan->mShardParamOffset;
    sMyGraph->shardParamCount = aFrom->tableRef->view->myPlan->mShardParamCount;

    sMyGraph->limit = NULL;
    sMyGraph->accessMethod = NULL;
    sMyGraph->accessMethodCnt = 0;
    sMyGraph->flag = QMG_SHARD_FLAG_CLEAR;

    /* TASK-7219 */
    sMyGraph->mShardQueryBuf = NULL;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::optimize( qcStatement * aStatement,
                                 qmgGraph    * aGraph )
{
/***********************************************************************
 *
 * Description : qmgShardSELT 의 최적화
 *
 * Implementation :
 *
 *      - 통계정보 구축
 *      - Subquery Graph 생성
 *      - 공통 비용 정보 설정 (recordSize, inputRecordCnt)
 *      - Predicate 재배치 및 개별 Predicate의 selectivity 계산
 *      - 전체 selectivity 계산 및 공통 비용 정보의 selectivity에 저장
 *      - Access Method 선택
 *      - Preserved Order 설정
 *      - 공통 비용 정보 설정 (outputRecordCnt, myCost, totalCost)
 *
 ***********************************************************************/

    qmgShardSELT   * sMyGraph  = NULL;
    qmsTarget      * sTarget   = NULL;
    qmsTableRef    * sTableRef = NULL;
    qmsQuerySet    * sQuerySet = NULL;
    qmsParseTree   * sParseTree = NULL;
    mtcColumn      * sTargetColumn = NULL;
    SDouble          sOutputRecordCnt = 0;
    SDouble          sRecordSize = 0;

    IDU_FIT_POINT_FATAL( "qmgShardSelect::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgShardSELT *)aGraph;
    sTableRef = sMyGraph->graph.myFrom->tableRef;

    IDE_FT_ASSERT( sTableRef != NULL );
    IDE_FT_ASSERT( sTableRef->view != NULL );

    //---------------------------------------------------
    // 통계 정보 구축
    //---------------------------------------------------

    IDE_TEST( qmoStat::getStatInfo4RemoteTable( aStatement,
                                                sTableRef->tableInfo,
                                                &(sTableRef->statInfo) )
              != IDE_SUCCESS );

    //---------------------------------------------------
    // Subquery의 Graph 생성
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sMyGraph->graph.myPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // 공통 비용 정보 설정 (recordSize, inputRecordCnt)
    //---------------------------------------------------

    sParseTree = (qmsParseTree *)sTableRef->view->myPlan->parseTree;

    IDE_FT_ASSERT( sParseTree != NULL );

    for ( sQuerySet = sParseTree->querySet;
          sQuerySet != NULL;
          sQuerySet = sQuerySet->left )
    {
        if ( sQuerySet->setOp == QMS_NONE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    // recordSize 설정
    // To Fix BUG-8241
    IDE_FT_ASSERT( sQuerySet != NULL );
    IDE_FT_ASSERT( sQuerySet->SFWGH != NULL );

    for ( sTarget = sQuerySet->SFWGH->target;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        sTargetColumn = QTC_STMT_COLUMN( aStatement, sTarget->targetColumn );

        IDE_FT_ASSERT( sTargetColumn != NULL );

        IDE_TEST_RAISE(
            ( sTargetColumn->module->id != MTD_BIGINT_ID   ) &&
            ( sTargetColumn->module->id != MTD_SMALLINT_ID ) &&
            ( sTargetColumn->module->id != MTD_INTEGER_ID  ) &&
            ( sTargetColumn->module->id != MTD_DOUBLE_ID   ) &&
            ( sTargetColumn->module->id != MTD_FLOAT_ID    ) &&
            ( sTargetColumn->module->id != MTD_REAL_ID     ) &&
            ( sTargetColumn->module->id != MTD_NUMBER_ID   ) &&
            ( sTargetColumn->module->id != MTD_NUMERIC_ID  ) &&
            ( sTargetColumn->module->id != MTD_BINARY_ID   ) &&
            ( sTargetColumn->module->id != MTD_BIT_ID      ) &&
            ( sTargetColumn->module->id != MTD_VARBIT_ID   ) &&
            ( sTargetColumn->module->id != MTD_BYTE_ID     ) &&
            ( sTargetColumn->module->id != MTD_VARBYTE_ID  ) &&
            ( sTargetColumn->module->id != MTD_NIBBLE_ID   ) &&
            ( sTargetColumn->module->id != MTD_DATE_ID     ) &&
            ( sTargetColumn->module->id != MTD_INTERVAL_ID ) &&
            ( sTargetColumn->module->id != MTD_CHAR_ID     ) &&
            ( sTargetColumn->module->id != MTD_VARCHAR_ID  ) &&
            ( sTargetColumn->module->id != MTD_NCHAR_ID    ) &&
            ( sTargetColumn->module->id != MTD_NVARCHAR_ID ) &&
            ( sTargetColumn->module->id != MTD_NULL_ID     ) &&
            ( sTargetColumn->module->id != MTD_GEOMETRY_ID ) &&
            ( sTargetColumn->module->id != MTD_BLOB_LOCATOR_ID ) &&
            ( sTargetColumn->module->id != MTD_CLOB_LOCATOR_ID ),
            ERR_INVALID_SHARD_QUERY );

        sRecordSize += sTargetColumn->column.size;
    }

    // BUG-36463 sRecordSize 는 0이 되어서는 안된다.
    sRecordSize = IDL_MAX( sRecordSize, 1 );
    sMyGraph->graph.costInfo.recordSize = sRecordSize;

    // inputRecordCnt 설정
    sMyGraph->graph.costInfo.inputRecordCnt = sTableRef->statInfo->totalRecordCnt;

    //---------------------------------------------------
    // Predicate의 재배치 및 개별 Predicate의 Selectivity 계산
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::relocatePredicate( aStatement,
                                              sMyGraph->graph.myPredicate,
                                              & sMyGraph->graph.depInfo,
                                              & sMyGraph->graph.myQuerySet->outerDepInfo,
                                              sMyGraph->graph.myFrom->tableRef->statInfo,
                                              & sMyGraph->graph.myPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // accessMethod (full scan)
    //---------------------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoAccessMethod),
                                             (void**)&sMyGraph->accessMethod)
              != IDE_SUCCESS );

    qmgSelection::setFullScanMethod( aStatement,
                                     sTableRef->statInfo,
                                     sMyGraph->graph.myPredicate,
                                     sMyGraph->accessMethod,
                                     1,           // parallel degree
                                     ID_FALSE );  // execute time

    sMyGraph->accessMethodCnt = 1;

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |=  QMG_PRESERVED_ORDER_NEVER;

    //---------------------------------------------------
    // 공통 비용 정보 설정 (outputRecordCnt, myCost, totalCost)
    //---------------------------------------------------

    sMyGraph->graph.costInfo.selectivity = sMyGraph->accessMethod->methodSelectivity;

    // output record count 설정
    sOutputRecordCnt = sMyGraph->graph.costInfo.selectivity *
        sMyGraph->graph.costInfo.inputRecordCnt;

    sMyGraph->graph.costInfo.outputRecordCnt =
        ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

    // my cost
    sMyGraph->graph.costInfo.myAccessCost = sMyGraph->accessMethod->accessCost;
    sMyGraph->graph.costInfo.myDiskCost = sMyGraph->accessMethod->diskCost;
    sMyGraph->graph.costInfo.myAllCost = sMyGraph->accessMethod->totalCost;

    // total cost
    sMyGraph->graph.costInfo.totalAccessCost = sMyGraph->accessMethod->accessCost;
    sMyGraph->graph.costInfo.totalDiskCost = sMyGraph->accessMethod->diskCost;
    sMyGraph->graph.costInfo.totalAllCost = sMyGraph->accessMethod->totalCost;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET,
                                  sTargetColumn->module->names->string ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::makePlan( qcStatement    * aStatement,
                                 const qmgGraph * aParent,
                                 qmgGraph       * aGraph )
{
/***********************************************************************
 *
 *  Description : qmgShardSELT 로 부터 Plan을 생성한다.
 *
 *  Implementation : SHARD가 있을 경우 아래와 같은 plan을 생성한다.
 *
 *          [PARENT]
 *            |
 *          [SDSE] (SharD SElect)
 *        ----|------------------------------------------
 *          [...]    [...]    [...] : shard data node 
 *    
 ************************************************************************/

    qmgShardSELT * sMyGraph = NULL;
    qmnPlan      * sSDSE    = NULL;
    qtcNode      * sLobFilter; // BUG-25916
    qmnPlan      * sFILT;

    IDU_FIT_POINT_FATAL( "qmgShardSelect::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aParent    != NULL );
    IDE_FT_ASSERT( aGraph     != NULL );

    sMyGraph = (qmgShardSELT*)aGraph;

    /* TASK-7219 */
    IDE_TEST( qmgShardSelect::pushSeriesOptimize( aStatement,
                                                  aParent,
                                                  sMyGraph )
              != IDE_SUCCESS );

    //---------------------------------------------------
    // Current CNF의 등록
    //---------------------------------------------------

    if ( sMyGraph->graph.myCNF != NULL )
    {
        sMyGraph->graph.myQuerySet->SFWGH->crtPath->currentCNF = sMyGraph->graph.myCNF;
    }
    else
    {
        // Nothing to do.
    }

    sMyGraph->graph.myPlan = aParent->myPlan;

    // PROJ-1071 Parallel Query
    sMyGraph->graph.flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    sMyGraph->graph.flag |=  QMG_PARALLEL_IMPOSSIBLE_TRUE;

    //-----------------------
    // init SDSE
    //-----------------------
    IDE_TEST( qmoOneNonPlan::initSDSE( aStatement,
                                       aParent->myPlan,
                                       &sSDSE )
              != IDE_SUCCESS );

    //-----------------------
    // make SDSE
    //-----------------------
    IDE_TEST( qmoOneNonPlan::makeSDSE( aStatement,
                                       aParent->myPlan,
                                       &sMyGraph->shardQuery,
                                       sMyGraph->shardAnalysis,
                                       sMyGraph->shardParamInfo, /* TASK-7219 Non-shard DML */
                                       sMyGraph->shardParamCount,
                                       &sMyGraph->graph,
                                       sSDSE )
              != IDE_SUCCESS );

    qmg::setPlanInfo( aStatement, sSDSE, &(sMyGraph->graph) );

    /* PROJ-2728 Sharding LOB
         BUG-47751 반영으로 인해 아래 BUG-25916 코드 적용 */
    // BUG-25916 : clob을 select for update 하던 도중 assert 발생
    // clob locator의 제약으로 lobfilter로 분류된 것이 존재하면
    // SCAN 노드 상위에 FILTER 노드로 처리해야 한다.
    sLobFilter = ((qmncSDSE*)sSDSE)->lobFilter;

    if ( sLobFilter != NULL )
    {
        // BUGBUG
        // Lob filter의 경우 SCAN을 생성한 후에 유무를 알 수 있다.
        // 따라서 FILT의 생성 여부도 SCAN의 생성 후에 결정된다.
        // BUG-25916의 문제 해결 방법을 수정해야 한다.
        IDE_TEST( qmoOneNonPlan::initFILT(
                      aStatement ,
                      sMyGraph->graph.myQuerySet ,
                      sLobFilter ,
                      sMyGraph->graph.myPlan ,
                      &sFILT) != IDE_SUCCESS);

        IDE_TEST( qmoOneNonPlan::makeFILT(
                      aStatement ,
                      sMyGraph->graph.myQuerySet ,
                      sLobFilter ,
                      NULL,
                      sSDSE ,
                      sFILT) != IDE_SUCCESS);
        sMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo( aStatement, sFILT, &(sMyGraph->graph) );
    }
    else
    {
        sMyGraph->graph.myPlan = sSDSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::printGraph( qcStatement  * aStatement,
                                   qmgGraph     * aGraph,
                                   ULong          aDepth,
                                   iduVarString * aString )
{
/***********************************************************************
 *
 * Description : Graph를 구성하는 공통 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate * sPredicate = NULL;
    qmoPredicate * sMorePred  = NULL;
    qmgShardSELT * sMyGraph = NULL;
    UInt  i;

    IDU_FIT_POINT_FATAL( "qmgShardSelect::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph     != NULL );
    IDE_FT_ASSERT( aString    != NULL );

    sMyGraph = (qmgShardSELT *)aGraph;

    //-----------------------------------
    // Graph의 시작 출력
    //-----------------------------------

    if (aDepth == 0)
    {
        QMG_PRINT_LINE_FEED(i, aDepth, aString);
        iduVarStringAppend(aString,
                           "----------------------------------------------------------");
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // Graph 공통 정보의 출력
    //-----------------------------------

    IDE_TEST( qmg::printGraph( aStatement,
                               aGraph,
                               aDepth,
                               aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Graph 고유 정보의 출력
    //-----------------------------------

    IDE_TEST( qmoStat::printStat( aGraph->myFrom,
                                  aDepth,
                                  aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Access method 정보 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Access Method Information ==" );

    IDE_TEST( printAccessMethod( aStatement,
                                 sMyGraph->accessMethod,
                                 aDepth,
                                 aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Subquery Graph 정보의 출력
    //-----------------------------------

    for ( sPredicate = aGraph->myPredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        for ( sMorePred = sPredicate;
              sMorePred != NULL;
              sMorePred = sMorePred->more )
        {
            IDE_TEST( qmg::printSubquery( aStatement,
                                          sMorePred->node,
                                          aDepth,
                                          aString )
                      != IDE_SUCCESS );
        }
    }

    for ( sPredicate = aGraph->constantPredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        for ( sMorePred = sPredicate;
              sMorePred != NULL;
              sMorePred = sMorePred->more )
        {
            IDE_TEST( qmg::printSubquery( aStatement,
                                          sMorePred->node,
                                          aDepth,
                                          aString )
                      != IDE_SUCCESS );
        }
    }

    //-----------------------------------
    // shard 정보 출력
    //-----------------------------------

    IDE_TEST( qmgShardDML::printShardInfo( aStatement,
                                           sMyGraph->shardAnalysis,
                                           &(sMyGraph->shardQuery),
                                           aDepth,
                                           aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Graph의 마지막 출력
    //-----------------------------------

    if (aDepth == 0)
    {
        QMG_PRINT_LINE_FEED(i, aDepth, aString);
        iduVarStringAppend(aString,
                           "----------------------------------------------------------\n\n");
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::printAccessMethod( qcStatement     * /* aStatement */ ,
                                          qmoAccessMethod * aMethod,
                                          ULong             aDepth,
                                          iduVarString    * aString )
{
    UInt i;

    IDU_FIT_POINT_FATAL( "qmgShardSelect::printAccessMethod::__FT__" );

    IDE_FT_ASSERT( aMethod->method == NULL );

    // FULL SCAN
    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "FULL SCAN" );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "  ACCESS_COST : %"ID_PRINT_G_FMT,
                              aMethod->accessCost );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "  DISK_COST   : %"ID_PRINT_G_FMT,
                              aMethod->diskCost );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "  TOTAL_COST  : %"ID_PRINT_G_FMT,
                              aMethod->totalCost );

    return IDE_SUCCESS;
    
}

/* TASK-7219 */
IDE_RC qmgShardSelect::copySelect( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 Select를 복제한다. Set 연산을 고려해서 SELECT * 를
 *               앞에 추가로 달거나, 기존의 Query Text를 그대로 이용한다.
 *
 *  QUERY   / SELECT C1 FROM ( SELECT * FROM T1 ORDER BY C1 )
 *  ANALYZE / SELECT C1 FROM ( SELECT * FROM SHARD( SELECT * FROM T1 ) ORDER BY C1 )
 *
 *  SHARD   /                                       SELECT * FROM T1
 *  COPY    /                                       SELECT
 *
 *  SET     /                 ( SELECT * FROM T1 ) UNION ALL ( SELECT * FROM T1 )
 *  WRAP    / SELECT *
 *
 *  SET     /                 ( SELECT * FROM T1 ) UNION ALL ( SELECT * FROM T1 )
 *  SKIP    /
 *
 *  HALFSET /                 ( SELECT * FROM T1 )
 *  COPY    /                 ( SELECT *
 *
 * Implementation : 1.   SELECT or ( SELECT
 *                  2.1. Set 감싸는 Transform를 수행한다.
 *                  2.2. 기본 Set 연산인 경우 copyFrom에서 작업한다.
 *                  2.3. 기존의 Query Text를 그래도 복제한다.
 *                  2.4. Bind 정보를 수집한다
 *
 ****************************************************************************************/

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mQuerySet == NULL, ERR_NULL_QUERYSET );
    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    if ( aInfo->mTarget != NULL )
    {
        /* 1.   SELECT or ( SELECT */
        if ( aInfo->mQuerySet->setOp != QMS_NONE )
        {
            /*  Set 연산에 Push Projection할 Target이 있으므로,
             *  aInfo->mNeedWrapSet == ID_TRUE인 상황.
             *  From에서 Set을 감싼다.
             */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          "SELECT " )
                      != IDE_SUCCESS );
        }
        else
        {
            /*  Set 연산이 아닌 경우, 아래의 두 형태이다.
             *  |  SELECT ...  |  ( SELECT ...  )  |
             *     ^              ^
             *      \______________\____ aInfo->mPosition.offset
             */
            if ( aInfo->mPosition.stmtText[ aInfo->mPosition.offset ] == '(' )
            {
                IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              "( SELECT " )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              "SELECT " )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        if ( aInfo->mQuerySet->setOp != QMS_NONE )
        {
            /* 2.1. Set 감싸는 Transform를 수행한다. */
            if ( aInfo->mNeedWrapSet == ID_TRUE )
            {
                /*  Push Projection를 수행하지 않지만, Push Selection을 수행해서,
                 *  aInfo->mNeedWrapSet == ID_TRUE인 상황으로
                 *  From에서 Set을 감싼다.
                 */
                IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              "SELECT * " )
                          != IDE_SUCCESS );
            }
            else
            {
                /* 2.2. 기본 Set 연산인 경우 copyFrom에서 작업한다.
                 *
                 *  Push Projection를 수행하지 않고,
                 *  Set를 감싸지도 않기 때문에,
                 *  변경할 필요가 없다.
                 */
            }
        }
        else
        {
            /* 2.3. 기존의 Query Text를 그대로 복제한다.
             *
             *  Set 연산이 아닌 경우, 아래의 두 형태이다.
             *  |  SELECT ...  |  ( SELECT ...  )  |
             *     ^              ^
             *      \______________\____ aInfo->mPosition.offset
             *
             */
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aInfo->mPosition.stmtText
                                                + aInfo->mPosition.offset,
                                                aInfo->mQuerySet->SFWGH->from->fromPosition.offset
                                                - aInfo->mPosition.offset )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( aInfo->mGraph == NULL, ERR_NULL_GRAPH );

            /* 2.4.   Bind 정보를 수집한다. */
            IDE_TEST( qmg::copyAndCollectParamOffset( aInfo->mStatement,
                                                      aInfo->mPosition.offset,
                                                      aInfo->mQuerySet->SFWGH->from->fromPosition.offset,
                                                      aInfo->mGraph->shardParamOffset,
                                                      aInfo->mGraph->shardParamCount,
                                                      aInfo->mGraph->shardParamInfo,
                                                      aInfo->mParamOffsetInfo )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copySelect",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copySelect",
                                  "query set is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copySelect",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copySelect",
                                  "graph is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::copyFrom( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 From절 Text를 복제한다.
 *               Set 연산자를 고려해서 복제하며, Push Selection 여부에 따라서, Set 연산인
 *               Query Set를 View 로 한번 감싸는 Transform으로 변경하기도 한다.
 *
 *  SET    /        ( SELECT * FROM T1 ) UNION ALL ( SELECT * FROM T1 )
 *  WRAP   / FROM ( ( SELECT * FROM T1 ) UNION ALL ( SELECT * FROM T1 ) )  <-- 3.2
 *  COPY   /        ( SELECT * FROM T1 ) UNION ALL ( SELECT * FROM T1 )    <-- 4.1
 *
 *  COMMON / SELECT * FROM T1
 *  WRAP   /        X
 *  COPY   /          FROM T1  <-- 4.2
 *
 * Implementation : 1.   Set 연산자 유무에 따라서 시작 Offset를 결정한다.
 *                  2.1. Set 연산자가 있다면, 마지막 Right 까지 복제한다.
 *                  2.2. 아니라면, 현재 From 부터 마지막 From 까지 찾는다.
 *                  3.1. Set 감싸는 Transform를 수행한다.
 *                  3.2. FROM (
 *                  3.3. 시작 Offset부터 마지막 Offset까지 복제한다.
 *                  3.4. )
 *                  4.1. 아니면 Query Set 여부에 따라서 복제한다.
 *                  4.2. FROM
 *                  4.3. 시작 Offset부터 마지막 Offset까지 복제한다.
 *                  5.   Bind 정보를 수집한다.
 *
 ****************************************************************************************/

    qmsQuerySet * sLeft   = NULL;
    qmsQuerySet * sRight  = NULL;
    qmsFrom     * sFrom   = NULL;
    SInt          sStart  = 0;
    SInt          sEnd    = 0;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1.   Set 연산자 유무에 따라서 시작 Offset를 결정한다. */
    if ( aInfo->mQuerySet->setOp != QMS_NONE )
    {
        sLeft = aInfo->mQuerySet->left;

        while ( sLeft->setOp != QMS_NONE )
        {
            sLeft = sLeft->left;
        }

        sRight = aInfo->mQuerySet->right;

        while ( sRight->setOp != QMS_NONE )
        {
            sRight = sRight->right;
        }
    }
    else
    {
        sFrom = aInfo->mQuerySet->SFWGH->from;
    }

    /* 2.1. Set 연산자가 있다면, 마지막 Right 까지 복제한다. */
    if ( sRight != NULL )
    {
        IDE_TEST_RAISE( sLeft->startPos.stmtText != aInfo->mPosition.stmtText, ERR_DIFF_LEFT );
        IDE_TEST_RAISE( sRight->startPos.stmtText != aInfo->mPosition.stmtText, ERR_DIFF_RIGHT );

        sStart = sLeft->startPos.offset;
        sEnd   = sRight->endPos.offset + sRight->endPos.size;
    }
    else
    {
        if ( sFrom->onCondition != NULL )
        {
            IDE_TEST_RAISE( sFrom->onCondition->position.stmtText != aInfo->mPosition.stmtText, ERR_DIFF_TABLE );
        }
        else
        {
            IDE_TEST_RAISE( sFrom->tableRef->position.stmtText != aInfo->mPosition.stmtText, ERR_DIFF_TABLE );
        }

        /* 2.2. 아니라면, 현재 From 부터 마지막 From 까지 찾는다.*/
        IDE_TEST( qmg::getFromOffset( sFrom,
                                      &( sStart ),
                                      &( sEnd ) )
                  != IDE_SUCCESS );
    }

    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    /* 3.1. Set 감싸는 Transform를 수행한다. */
    if ( aInfo->mNeedWrapSet == ID_TRUE )
    {
        /* 3.2. FROM ( */
        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      "FROM ( " )
                  != IDE_SUCCESS );


        /* 3.3. 시작 Offset부터 마지막 Offset까지 복제한다. */
        IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                            aInfo->mPosition.stmtText
                                            + sStart,
                                            sEnd
                                            - sStart )
                  != IDE_SUCCESS );

        /* 3.4. ) */
        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      " )" )
                  != IDE_SUCCESS );

        if ( ( sEnd < aInfo->mPosition.offset + aInfo->mPosition.size ) &&
             ( aInfo->mPosition.stmtText[ aInfo->mPosition.offset
                                          + aInfo->mPosition.size
                                          - 1 ] == ')' ) )
        {
            aInfo->mPosition.size--;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        if ( aInfo->mQuerySet->setOp != QMS_NONE )
        {
            /* 4.1. 아니면 Query Set 여부에 따라서 복제한다. */
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aInfo->mPosition.stmtText
                                                + sStart,
                                                sEnd
                                                - sStart )
                  != IDE_SUCCESS );
        }
        else
        {
            /* 4.2. FROM */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          "FROM " )
                      != IDE_SUCCESS );

            /* 4.3. 시작 Offset부터 마지막 Offset까지 복제한다. */
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aInfo->mPosition.stmtText
                                                + sStart,
                                                sEnd
                                                - sStart )
                      != IDE_SUCCESS );
        }

    }

    IDE_TEST_RAISE( aInfo->mGraph == NULL, ERR_NULL_GRAPH );

    /* 5.   Bind 정보를 수집한다. */
    IDE_TEST( qmg::copyAndCollectParamOffset( aInfo->mStatement,
                                              sStart,
                                              sEnd,
                                              aInfo->mGraph->shardParamOffset,
                                              aInfo->mGraph->shardParamCount,
                                              aInfo->mGraph->shardParamInfo,
                                              aInfo->mParamOffsetInfo )
              != IDE_SUCCESS );

    aInfo->mPosition.size  -= sEnd - aInfo->mPosition.offset;
    aInfo->mPosition.offset = sEnd;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyFrom",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyFrom",
                                  "query set is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyFrom",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION( ERR_DIFF_LEFT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyFrom",
                                  "left position is diff" ) );
    }
    IDE_EXCEPTION( ERR_DIFF_RIGHT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyFrom",
                                  "right position is diff" ) );
    }
    IDE_EXCEPTION( ERR_DIFF_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyFrom",
                                  "table position is diff" ) );
    }
    IDE_EXCEPTION( ERR_NULL_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyFrom",
                                  "graph is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::copyWhere( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 Where절 Text를 복제한다.
 *               Push Selection 여부에 따라서 Where Text 구성이 변경될 수 있다.
 *
 *  Where          /   / WHEHE C2 = 1 AND C3 > 2
 *                             *****************
 *  Push Selection / O / WHERE ( PUSH SELECTION ) AND ( C2 = 1 AND C3 > 2 )
 *                                                *************************
 *  Push Selection / X / WHEHE C2 = 1 AND C3 > 2
 *                       ***********************
 *
 * Implementation : 1.1. AND (
 *                  1.2. WHERE
 *                  2.   시작 Offset를 계산한다.
 *                  3.   마지막 Offset를 계산한다.
 *                  4.   시작부터 마지막까지 복제한다.
 *                  5.   )
 *                  6.   Bind 정보를 수집한다.
 *
 ****************************************************************************************/

    qtcNode * sNode  = NULL;
    qtcNode * sLast  = NULL;
    SInt      sStart = 0;
    SInt      sEnd   = 0;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );

    if ( aInfo->mWhere != NULL )
    {
        IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

        if ( aInfo->mPushPred != NULL )
        {
            /* 1.1. AND ( */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          " AND ( " )
                          != IDE_SUCCESS );
        }
        else
        {
            /* 1.2. WHERE */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          " WHERE " )
                      != IDE_SUCCESS );
        }

        /* 2.   시작 Offset를 계산한다. */
        IDE_TEST( qmg::getNodeOffset( aInfo->mWhere,
                                      ID_TRUE,
                                      &( sStart ),
                                      NULL )
                  != IDE_SUCCESS );

        /* 3.   마지막 Offset를 계산한다. */
        for ( sNode  = aInfo->mWhere;
              sNode != NULL;
              sNode  = (qtcNode *)sNode->node.next )
        {
            sLast = sNode;
        }

        IDE_TEST( qmg::getNodeOffset( sLast,
                                      ID_TRUE,
                                      NULL,
                                      &( sEnd ) )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sLast->position.stmtText != aInfo->mPosition.stmtText, ERR_DIFF_QUERY );

        /* 4.   시작부터 마지막까지 복제한다. */
        IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                            sLast->position.stmtText
                                            + sStart,
                                            sEnd
                                            - sStart )
                  != IDE_SUCCESS );

        if ( aInfo->mPushPred != NULL )
        {
            /* 5.   ) */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              " )" )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST_RAISE( aInfo->mGraph == NULL, ERR_NULL_GRAPH );

        /* 6.   Bind 정보를 수집한다. */
        IDE_TEST( qmg::copyAndCollectParamOffset( aInfo->mStatement,
                                                  sStart,
                                                  sEnd,
                                                  aInfo->mGraph->shardParamOffset,
                                                  aInfo->mGraph->shardParamCount,
                                                  aInfo->mGraph->shardParamInfo,
                                                  aInfo->mParamOffsetInfo )
                  != IDE_SUCCESS );

        aInfo->mPosition.size  -= sEnd - aInfo->mPosition.offset;
        aInfo->mPosition.offset = sEnd;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyWhere",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyWhere",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION( ERR_DIFF_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyWhere",
                                  "query position is diff" ) );
    }
    IDE_EXCEPTION( ERR_NULL_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::copyWhere",
                                  "graph is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::generateTargetText( qmgTransformInfo * aInfo,
                                           qmsTarget        * aTarget )
{
/****************************************************************************************
 *
 * Description : Asterisk ( * ) 를 사용한 Query에서는 qmvQuerySet::expandAllTarget 함수로
 *               Target을 확장하여 Target Node를 생성하기 때문에, Target Position이 없다.
 *
 *               이때, tableInfo->column 내용을 기반으로 작성된 userName, tableName,
 *               aliasTableName, columnName, aliasColumnName을 가지고
 *               확장된 Target의 이름을 생성한다.
 *
 * Implementation : 1.1. Set 연산이 있다면, column 생성한다.
 *                  1.2. AliasColumnName가 있다면 Text를 생성한다.
 *                  1.3. Column가 있다면 Text를 생성한다.
 *                  2.1. UserName가 있다면 Text를 생성한다.
 *                  2.2. AliasTableName가 있다면 Text를 생성한다.
 *                  2.3. TableName가 있다면 Text를 생성한다.
 *                  2.4. Column가 있다면 Text를 생성한다.
 *                  2.5. AliasColumnName가 있다면 Text를 생성한다.
 *
 ****************************************************************************************/

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aTarget == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    /* 1.1. Set 연산이 있다면, column만 생성한다. */
    if ( aInfo->mQuerySet->setOp != QMS_NONE )
    {
        /* 1.2. AliasColumnName가 있다면 Text를 생성한다. */
        if ( QC_IS_NULL_NAME( aTarget->aliasColumnName ) != ID_TRUE )
        {
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aTarget->aliasColumnName.name,
                                                aTarget->aliasColumnName.size )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( QC_IS_NULL_NAME( aTarget->columnName ) == ID_TRUE, ERR_NULL_COLUMNNAME );

            /* 1.3. Column가 있다면 Text를 생성한다. */
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aTarget->columnName.name,
                                                aTarget->columnName.size )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* 2.1. UserName가 있다면 Text를 생성한다. */
        if ( QC_IS_NULL_NAME( aTarget->userName ) != ID_TRUE )
        {
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aTarget->userName.name,
                                                aTarget->userName.size )
                      != IDE_SUCCESS );

            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          "." )
                      != IDE_SUCCESS );

        }
        else
        {
            /* Nothing to do */
        }

        /* 2.2. AliasTableName가 있다면 Text를 생성한다. */
        if ( QC_IS_NULL_NAME( aTarget->aliasTableName ) != ID_TRUE )
        {
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aTarget->aliasTableName.name,
                                                aTarget->aliasTableName.size )
                      != IDE_SUCCESS );

            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          "." )
                      != IDE_SUCCESS );
        }
        else
        {
            /* 2.3. TableName가 있다면 Text를 생성한다. */
            if ( QC_IS_NULL_NAME( aTarget->tableName ) != ID_TRUE )
            {
                IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                    aTarget->tableName.name,
                                                    aTarget->tableName.size )
                          != IDE_SUCCESS );

                IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              "." )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }

        IDE_TEST_RAISE( QC_IS_NULL_NAME( aTarget->columnName ) == ID_TRUE, ERR_NULL_COLUMNNAME );

        /* 2.4. Column가 있다면 Text를 생성한다. */
        IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                            aTarget->columnName.name,
                                            aTarget->columnName.size )
                  != IDE_SUCCESS );

        /* 2.5. AliasColumnName가 있다면 Text를 생성한다. */
        if ( QC_IS_NULL_NAME( aTarget->aliasColumnName ) != ID_TRUE )
        {
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          " AS " )
                      != IDE_SUCCESS );

            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aTarget->aliasColumnName.name,
                                                aTarget->aliasColumnName.size )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generateTargetText",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generateTargetText",
                                  "node is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generateTargetText",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_COLUMNNAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generateTargetText",
                                  "column name is null" ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::generatePredText( qmgTransformInfo * aInfo,
                                         qtcNode          * aNode )
{
/****************************************************************************************
 *
 * Description : qmoUtil::printPredInPlan 를 참고하여 작성된 Query Text 형태의 Predicate
 *               Node Text 생성 함수이다. 중위 순회로 순회하면서 한줄로 생성해낸다.
 *
 * Implementation : 1. 논리 연산자라면 재귀 호출한다.
 *                  2. 중위 순회로 논리 연산자를 중간에 작성한다.
 *                  3. 논리 연산자가 아니라면, Node Text를 생성한다.
 *
 ****************************************************************************************/

    qtcNode * sNode = NULL;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    /* 1. 논리 연산자라면 재귀 호출한다. */
    if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        for ( sNode  = (qtcNode *)( aNode->node.arguments );
              sNode != NULL;
              sNode  = (qtcNode *)( sNode->node.next ) )
        {
            IDE_TEST( generatePredText( aInfo,
                                        sNode )
                     != IDE_SUCCESS );

            /* 2. 중위 순회로 논리 연산자를 중간에 작성한다. */
            if ( sNode->node.next != NULL )
            {
                if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_AND )
                {
                    IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                        " AND ",
                                                        5 )
                              != IDE_SUCCESS );
                }
                else if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                          == MTC_NODE_OPERATOR_OR )
                {
                    IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                        " OR ",
                                                        4 )
                              != IDE_SUCCESS );
                }
                else if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                          == MTC_NODE_OPERATOR_NOT )
                {
                    IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                        " NOT ",
                                                        6 )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_RAISE( ERR_UNEXPECT );
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        QC_SHARED_TMPLATE( aInfo->mStatement )->flag &= ~QC_TMP_SHARD_OUT_REF_COL_TO_BIND_MASK;
        QC_SHARED_TMPLATE( aInfo->mStatement )->flag |= QC_TMP_SHARD_OUT_REF_COL_TO_BIND_TRUE;
        
        /* 3. 논리 연산자가 아니라면, Node Text를 생성한다. */
        IDE_TEST( qmoUtil::printExpressionInPlan( QC_SHARED_TMPLATE( aInfo->mStatement ),
                                                  aInfo->mString,
                                                  aNode,
                                                  QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );

        QC_SHARED_TMPLATE( aInfo->mStatement )->flag &= ~QC_TMP_SHARD_OUT_REF_COL_TO_BIND_MASK;
        QC_SHARED_TMPLATE( aInfo->mStatement )->flag |= QC_TMP_SHARD_OUT_REF_COL_TO_BIND_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generatePredText",
                                  "info is null" ) );
    }
        IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generatePredText",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generatePredText",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generatePredText",
                                  "node is null" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::generatePredText",
                                  "unexpected error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::pushProjectOptimize( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 Target을 Push, Text를 복제한다.
 *               QMS_TARGET_IS_USELESS_TRUE 인 Target은 제거한다. Target 제거로 Push할
 *               Target이 없어지면 Null Target를 추가한다.
 *
 *  QUERY   / SELECT C1 FROM ( SELECT * FROM T1 ORDER BY C1 )
 *  ANALYZE / SELECT C1 FROM ( SELECT * FROM SHARD( SELECT * FROM T1 ) ORDER BY C1 )
 *
 *  SHARD   /                                       SELECT * FROM T1
 *  EXPAND  /                                       SELECT C1, C2 FROM T1
 *  SELECT  /                                       SELECT
 *  USELESS /                                              F   T
 *  PROJECT /                                              C1               <-- 2.1, 2.3
 *  FROM    /                                                     FROM T1
 *
 *
 * Implementation : 1.   Push Projection 대상이 있는 경우에 최적화한다.
 *                  2.1. 모든 Target를 수색하며, 최적화한다.
 *                  2.2. QMS_TARGET_IS_USELESS_TRUE인 Target은 제거한다.
 *                  2.3. 이전 Target에 ,로 연결한다.
 *                  3.1. Query Text에서 있던 Target은 복제한다.
 *                  3.2. 시작부터 마지막까지 복제한다.
 *                  3.3. aliasColumnName는 Position이 없어 새로 Text를 생성한다.
 *                  3.4. 생성된 Target은 새로 Text를 생성한다.
 *                  4.   Push할 Target의 Bind 정보를 수집한다.
 *                  5.   Push할 Target이 없다면, NULL Target 를 추가한다.
 *                  6.   Tuple Column를 조정한다.
 *
 ****************************************************************************************/

    qmsTarget * sTarget      = NULL;
    SInt        sStart       = 0;
    SInt        sEnd         = 0;
    idBool      sIsTransform = ID_FALSE;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mQuerySet == NULL, ERR_NULL_QUERYSET );
    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    /* 1.   Push Projection 대상이 있는 경우에 최적화한다. */
    if ( aInfo->mTarget != NULL )
    {
        /* 2.1. 모든 Target를 수색하며, 최적화한다. */
        for ( sTarget  = aInfo->mTarget;
              sTarget != NULL;
              sTarget  = sTarget->next )
        {
            /* 2.2.   QMS_TARGET_IS_USELESS_TRUE 인 Target은 제거한다. */
            if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) == QMS_TARGET_IS_USELESS_TRUE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            /* 2.3.   이전 Target에 ,로 연결한다. */
            if ( sIsTransform == ID_TRUE )
            {
                IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              ", " )
                          != IDE_SUCCESS );
            }
            else
            {
                sIsTransform = ID_TRUE;
            }

            /* 3.1. Query Text에서 있던 Target은 복제한다. */
            if ( ( QC_IS_NULL_NAME( sTarget->targetColumn->position ) != ID_TRUE ) &&
                 ( sTarget->targetColumn->position.stmtText == aInfo->mPosition.stmtText ) )
            {
                IDE_TEST( qmg::getNodeOffset( sTarget->targetColumn,
                                              ID_FALSE,
                                              &( sStart ),
                                              &( sEnd ) )
                          != IDE_SUCCESS );

                /* 3.2. 시작부터 마지막까지 복제한다. */
                IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                    sTarget->targetColumn->position.stmtText
                                                    + sStart,
                                                    sEnd
                                                    - sStart )
                          != IDE_SUCCESS );

                /* 3.3. aliasColumnName는 Position이 없어 새로 Text를 생성한다. */
                if ( QC_IS_NULL_NAME( sTarget->aliasColumnName ) != ID_TRUE )
                {
                    IDE_TEST( iduVarStringAppend( aInfo->mString,
                                                  " AS " )
                              != IDE_SUCCESS );

                    IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                        sTarget->aliasColumnName.name,
                                                        sTarget->aliasColumnName.size )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* 3.4. 생성된 Target은 새로 Text를 생성한다.
                 *  - qmvQuerySet::expandAllTarget
                 *  - qmvQuerySet::expandTarget
                 *  - qmvShardTransform::appendSortNode
                 *  - qmvShardTransform::adjustTargetAndSort
                 */
                IDE_TEST( generateTargetText( aInfo,
                                              sTarget )
                          != IDE_SUCCESS );
            }

            /* 4.   Push할 Target의 Bind 정보를 수집한다. */
            IDE_TEST( qmg::findAndCollectParamOffset( aInfo->mStatement,
                                                      sTarget->targetColumn,
                                                      aInfo->mParamOffsetInfo )
                      != IDE_SUCCESS );
        }

        if ( sIsTransform == ID_TRUE )
        {
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          " " )
                      != IDE_SUCCESS );
        }
        else
        {
            /* 5.   Push할 Target이 없다면, NULL Target 를 추가한다. */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          "NULL " )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushProjectOptimize",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushProjectOptimize",
                                  "query set is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushProjectOptimize",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::pushSelectOptimize( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 Where절을 Push, Text를 복제한다.
 *               Predicate 단위를 ( )로 감싸고, Push Selection 전체도 ( )로 감싼다.
 *
 *  TARGET PREDICATE            |  Push Selection
 *                              |
 *    OR                        |   WHERE (
 *     AND                      |          (
 *      A.C2 = 1 ---  A.C3 = 2  |            T1.C2 = 1
 *                              |                      )
 *                              |                        AND
 *                              |                            (
 *                              |                              T2.C2 = 1
 *                              |                                        )
 *                              |                                          )
 *                              |
 *                              |   WHERE ( ( T1.C2 = 1 ) AND ( T2.C2 = 1 ) )
 *
 * Implementation : 1.  Push할 Predicate가 있을 때만 수행한다.
 *                  2.  WHERE (
 *                  3.  이전 Predicade에 AND로 연결한다.
 *                  4.  Shard View에 적합하게 Predicate Node 이름을 변경한다.
 *                  5.  (
 *                  6.  Predicate Node를 Text화 한다.
 *                  7.  )
 *                  8.  Push할 Predicate의 Bind 정보를 수집한다.
 *                  9.  Shard Key Columnn이라면 Shard Value를 만든다.
 *                  10. )
 *
 ****************************************************************************************/

    qmgPushPred * sPushPred    = NULL;
    idBool        sIsTransform = ID_FALSE;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );

    /* 1.  Push할 Predicate가 있을 때만 수행한다. */
    if ( aInfo->mPushPred != NULL )
    {
        IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

        /* 2.  WHERE ( */
        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      " WHERE ( " )
                  != IDE_SUCCESS );

        for ( sPushPred  = aInfo->mPushPred;
              sPushPred != NULL;
              sPushPred  = sPushPred->mNext )
        {
            if ( sIsTransform == ID_TRUE )
            {
                /* 3.  이전 Predicade에 AND로 연결한다. */
                IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              " AND " )
                          != IDE_SUCCESS );
            }
            else
            {
                sIsTransform = ID_TRUE;
            }

            /* 5.  ( */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          "( " )
                      != IDE_SUCCESS );

            /* 6.  Predicate Node를 Text화 한다. */
            IDE_TEST( generatePredText( aInfo,
                                        sPushPred->mNode )
                      != IDE_SUCCESS );

            /* 7.  ) */
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          " )" )
                      != IDE_SUCCESS );

            /* 8.  Push할 Predicate의 Bind 정보를 수집한다. */
            IDE_TEST( qmg::findAndCollectParamOffset( aInfo->mStatement,
                                                      sPushPred->mNode,
                                                      aInfo->mParamOffsetInfo )
                      != IDE_SUCCESS );
        }

        IDE_TEST_RAISE( sIsTransform == ID_FALSE, ERR_UNEXPECTED );

        /* 10. ) */
        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      " )" )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushSelectOptimize",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushSelectOptimize",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushSelectOptimize",
                                  "target predicate is none" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::pushOrderByOptimize( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 Order By절을 Push, Text를 복제한다.
 *               Limit Sort인 경우만 수행한다. 그리고 Order By절을 먼저 Push, 연속해서
 *               pushLimitOptimize를 호출해서 해야 한다. 따라서 Order By절을 Push한 다음
 *               Limit절을 Push한다.
 *
 *  ORDER BY C2, C3 ASC  |  ORDER BY C2, C3 NULL LAST
 *  *******************  |  *************************
 *
 * Implementation : 1. Push할 Order By가 있을 때만 수행한다.
 *                  2. 시작 Offset를 계산한다.
 *                  3. 마지막 Offset를 계산한다.
 *                  4. Push할 Order By의 Bind 정보를 수집한다.
 *                  5. 시작부터 마지막까지 복제한다.
 *
 ****************************************************************************************/

    qmsSortColumns * sOrderBy = NULL;
    qmsSortColumns * sLast    = NULL;
    SInt             sStart   = 0;
    SInt             sEnd     = 0;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    /*  1. Push할 Order By가 있을 때만 수행한다. */
    if ( aInfo->mOrderBy != NULL )
    {
        /* 2. 시작 Offset를 계산한다. */
        IDE_TEST( qmg::getNodeOffset( aInfo->mOrderBy->sortColumn,
                                      ID_TRUE,
                                      &( sStart ),
                                      NULL )
                  != IDE_SUCCESS );

        /* 3. 마지막 Offset를 계산한다. */
        for ( sOrderBy  = aInfo->mOrderBy;
              sOrderBy != NULL;
              sOrderBy  = sOrderBy->next )
        {
            /* 4.Push할 Order By의 Bind 정보를 수집한다. */
            IDE_TEST( qmg::findAndCollectParamOffset( aInfo->mStatement,
                                                      sOrderBy->sortColumn,
                                                      aInfo->mParamOffsetInfo )
                      != IDE_SUCCESS );

            sLast = sOrderBy;
        }

        if ( QC_IS_NULL_NAME( sLast->mNullsOptPos ) != ID_TRUE )
        {
            sEnd = sLast->mNullsOptPos.offset + sLast->mNullsOptPos.size;
        }
        else if ( QC_IS_NULL_NAME( sLast->mSortModePos ) != ID_TRUE )
        {
            sEnd = sLast->mSortModePos.offset + sLast->mSortModePos.size;
        }
        else
        {
            IDE_TEST( qmg::getNodeOffset( sLast->sortColumn,
                                          ID_TRUE,
                                          NULL,
                                          &( sEnd ) )
                      != IDE_SUCCESS );
        }

        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      " ORDER BY " )
                  != IDE_SUCCESS );

        /* 5. 시작부터 마지막까지 복제한다. */
        IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                            sLast->sortColumn->position.stmtText
                                            + sStart,
                                            sEnd
                                            - sStart )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushOrderByOptimize",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushOrderByOptimize",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::pushLimitOptimize( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 Limit절을 Push, Query Set Text를 복제한다.
 *               Push Limit 최적화로 Shard Select Plan에서 Node 마다 Limit를 수행한다.
 *
 *  BEFORE                       |  AFTER
 *  - LIMIT 2 OFFSET 2 ( COORD ) |  -- LIMIT CAST( 2 AS BIGINT ) + CAST( 2 AS BIGINT ) - 1 ( NODE1 )
 *                               |  -- LIMIT CAST( 2 AS BIGINT ) + CAST( 2 AS BIGINT ) - 1 ( NODE2 )
 *                               |  - LIMIT 2 OFFSET 2 ( COORD )
 *
 * Implementation : 1.   Push할 Limit가 있을 때만 수행한다.
 *                  2.   LIMIT FOR SHARD CAST(
 *                  3.1. START
 *                  3.2. Push할 Limit의 Bind 정보를 수집한다.
 *                  4.   AS BIGINT ) + CAST(
 *                  5.1  COUNT
 *                  5.2. Push할 Limit의 Bind 정보를 수집한다.
 *                  6.   AS BIGINT ) - 1
 *
 ****************************************************************************************/

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    /*  1.   Push할 Limit가 있을 때만 수행한다. */
    if ( aInfo->mLimit != NULL )
    {
        /* 2.   LIMIT ( */
        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      " LIMIT FOR SHARD CAST( " )
                  != IDE_SUCCESS );

        /* 3.1. START */
        if ( QC_IS_NULL_NAME( aInfo->mLimit->start.mPosition ) != ID_TRUE )
        {
            IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                                aInfo->mLimit->start.mPosition.stmtText
                                                + aInfo->mLimit->start.mPosition.offset,
                                                aInfo->mLimit->start.mPosition.size )
                      != IDE_SUCCESS );

            if ( qmsLimitI::hasHostBind( qmsLimitI::getStart( aInfo->mLimit ) ) == ID_TRUE )
            {
                /* 3.2. Push할 Limit의 Bind 정보를 수집한다. */
                IDE_TEST( qmg::findAndCollectParamOffset( aInfo->mStatement,
                                                          qmsLimitI::getHostNode(
                                                              qmsLimitI::getStart( aInfo->mLimit ) ),
                                                          aInfo->mParamOffsetInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          "1" )
                      != IDE_SUCCESS );
        }

        /* 4.   AS BIGINT ) + ( */
        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      " AS BIGINT ) + CAST( " )
                  != IDE_SUCCESS );

        /* 5.1. COUNT */
        IDE_TEST( iduVarStringAppendLength( aInfo->mString,
                                            aInfo->mLimit->count.mPosition.stmtText
                                            + aInfo->mLimit->count.mPosition.offset,
                                            aInfo->mLimit->count.mPosition.size )
                  != IDE_SUCCESS );

        if ( qmsLimitI::hasHostBind( qmsLimitI::getCount( aInfo->mLimit ) ) == ID_TRUE )
        {
            /* 5.2. Push할 Limit의 Bind 정보를 수집한다. */
            IDE_TEST( qmg::findAndCollectParamOffset( aInfo->mStatement,
                                                      qmsLimitI::getHostNode(
                                                          qmsLimitI::getCount( aInfo->mLimit ) ),
                                                      aInfo->mParamOffsetInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        /* 6.   AS BIGINT ) - 1 SHARD */
        IDE_TEST( iduVarStringAppend( aInfo->mString,
                                      " AS BIGINT ) - 1" )
                  != IDE_SUCCESS );

    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushLimitOptimize",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushLimitOptimize",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmgShardSelect::pushForUpdate( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Plan String Buffer로 FOR UPDATE절을 Push, Query Set Text를 복제한다.
 *               수행 시 Shard Select Plan에서 수행 대상 노드들에 FOR UPDATE가 전달된다.
 *
 * Implementation :
 *
 ****************************************************************************************/

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mString == NULL, ERR_NULL_PLAN_STRING );

    if ( aInfo->mForUpdate != NULL )
    {
        if ( aInfo->mForUpdate->isQueue == ID_FALSE )
        {
            IDE_TEST( iduVarStringAppend( aInfo->mString,
                                          " FOR UPDATE" )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( aInfo->mForUpdate->isMoveAndDelete == ID_TRUE  )
            {
                IDE_TEST( iduVarStringAppend( aInfo->mString,
                                              " FOR MOVE AND DELETE" )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_RAISE(ERR_UNSUPPORTED_FOR_UPDATE_TYPE);
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushForUpdate",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PLAN_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushForUpdate",
                                  "plan string is null" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_FOR_UPDATE_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushForUpdate",
                                  "Unsupported shard object type in Altibase Sharding" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::pushSeriesOptimize( qcStatement    * aStatement,
                                           const qmgGraph * aParent,
                                           qmgShardSELT   * aMyGraph )
{
/****************************************************************************************
 *
 * Description : Shard Query로 Push Projection, Push Selection, Push Limit 최적화를
 *               수행한다.
 *
 *               특히, Push Limit는 Limit절 하나만 Push하거나, Limit Sort인 경우
 *               Order By절 까지도 같이 Push해야 한다.
 *
 * Implementation : 1.  변환이 필요한 지 검사한다.
 *                  2.  변환이 필요할 때만 수행한다.
 *                  3.  Select를 복제한다.
 *                  4.  Push Projection를 수행한다.
 *                  5.  From 절을 복제한다.
 *                  6.  Push Selection를 수행한다.
 *                  7.  Where 절을 복제한다.
 *                  8.  Group By Having 절을 복제한다
 *                  9.  Bind 정보를 수집한다.
 *                  10. Push Limit를 수행한다.
 *                  11. Push FOR UPDATE를 수행한다. (BUG-48961)
 *                  12. 변환한 Query를 Buffer에 출력한다.
 *                  13. 변환한 Query로 Tuple Column를 설정한다.
 *                  14. 변환한 Query로 Shard Key Value를 설정한다.
 *                  15. 변환한 Query로 Shard Parameter를 설정
 *                  16. 변환한 Query를 Shard Query로 설정한다.
 *
 ****************************************************************************************/

    qmgTransformInfo sTransformInfo;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aParent == NULL, ERR_NULL_PARENT_GRAPH );
    IDE_TEST_RAISE( aMyGraph == NULL, ERR_NULL_MY_GRAPH );

    /* 1.  변환이 필요한 지 검사한다. */
    IDE_TEST( checkAndGetPushSeries( aStatement,
                                     aParent,
                                     aMyGraph,
                                     &( sTransformInfo ) )
              != IDE_SUCCESS );

    /* 2.  변환이 필요할 때만 수행한다. */
    if ( sTransformInfo.mIsTransform == ID_TRUE )
    {
        IDE_TEST( iduVarStringTruncate( sTransformInfo.mString,
                                        ID_TRUE )
                  != IDE_SUCCESS );

        /* 3. Select를 복제한다. */
        IDE_TEST( copySelect( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 4.  Push Projection를 수행한다. */
        IDE_TEST( pushProjectOptimize( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 5.  From 절을 복제한다. */
        IDE_TEST( copyFrom( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 6.  Push Selection를 수행한다. */
        IDE_TEST( pushSelectOptimize( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 7.  Where 절을 복제한다. */
        IDE_TEST( copyWhere( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 8.  Group By Having 절을 복제한다. */
        IDE_TEST( iduVarStringAppendLength( sTransformInfo.mString,
                                            sTransformInfo.mPosition.stmtText
                                            + sTransformInfo.mPosition.offset,
                                            sTransformInfo.mPosition.size )
                  != IDE_SUCCESS );

        /* 9.  Bind 정보를 수집한다. */
        IDE_TEST( qmg::copyAndCollectParamOffset( aStatement,
                                                  sTransformInfo.mPosition.offset,
                                                  sTransformInfo.mPosition.offset
                                                  + sTransformInfo.mPosition.size,
                                                  aMyGraph->shardParamOffset,
                                                  aMyGraph->shardParamCount,
                                                  aMyGraph->shardParamInfo,
                                                  sTransformInfo.mParamOffsetInfo )
                  != IDE_SUCCESS );

        /* 10. Push Limit를 수행한다. */
        IDE_TEST( pushOrderByOptimize( &( sTransformInfo ) ) != IDE_SUCCESS );

        IDE_TEST( pushLimitOptimize( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 11. Push FOR UPDATE를 수행한다. */
        IDE_TEST( pushForUpdate( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 12. 변환한 Query를 Buffer에 출력한다. */
        aMyGraph->mShardQueryLength = iduVarStringGetLength( sTransformInfo.mString );

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( aMyGraph->mShardQueryLength + 1,
                                                   (void**)&( aMyGraph->mShardQueryBuf ) )
                  != IDE_SUCCESS );

        IDE_TEST( iduVarStringConvToCString( sTransformInfo.mString,
                                             aMyGraph->mShardQueryLength + 1,
                                             aMyGraph->mShardQueryBuf )
                  != IDE_SUCCESS );

        IDE_TEST( iduVarStringTruncate( sTransformInfo.mString,
                                        ID_TRUE )
                  != IDE_SUCCESS );

        /* 13. 변환한 Query로 Tuple Column를 t정한다. */
        IDE_TEST( setShardTupleColumn( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 14. 변환한 Query로 Shard Key Value를 설정한다. */
        IDE_TEST( setShardKeyValue( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 15. 변환한 Query로 Shard Parameter를 설정한다. */
        IDE_TEST( setShardParameter( &( sTransformInfo ) ) != IDE_SUCCESS );

        /* 16. 변환한 Query를 Shard Query로 설정한다. */
        aMyGraph->shardQuery.stmtText = aMyGraph->mShardQueryBuf;
        aMyGraph->shardQuery.offset   = 0;
        aMyGraph->shardQuery.size     = aMyGraph->mShardQueryLength;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushSeriesOptimize",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PARENT_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushSeriesOptimize",
                                  "parent graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_MY_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::pushSeriesOptimize",
                                  "my graph is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::setShardKeyValue( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Push Selection으로 Shard Key가 내려갈 때에, Shard Key Value를 조정하는
 *               함수이다. qmvShardTransform::isShardQuery 함수 상위를 참조하였다.
 *
 *               Dummy Statement를 생성하고, Push Selection이 적용된 Query를 다시 PVO,
 *               Analyze 한다. Analyze 결과로 반환되는 Shard Key Value만 취하고
 *               나머지는 버린다.
 *
 *
 *  BEFORE / SELECT CAST( ? AS INTEGER )
 *            FROM ( SELECT CAST( ? AS INTEGER ), C1
 *                    FROM T1 WHERE C3 != ?
 *                     ORDER BY C2 )
 *             WHERE C1 != ?;
 *
 *  AFTER1 / SELECT CAST( ? AS INTEGER )
 *            FROM ( SELECT *
 *                    FROM ( SELECT CAST( ? AS INTEGER ), C1
 *                            FROM T1 WHERE C3 != ?
 *                             ORDER BY C2 ) )
 *             WHERE C1 != 1;
 *
 *  SHARD VALUE / COUNT 0 / NONE
 *
 *
 *  AFTER2 / SELECT CAST( ? AS INTEGER )
 *            FROM ( SELECT *
 *                    FROM ( SELECT CAST( ? AS INTEGER ), C1
 *                            FROM T1 WHERE C3 != ?
 *                             AND C1 != 1
 *                                 *******
 *                              ORDER BY C2 ) )
 *             WHERE C1 != 1;
 *
 *  SHARD VALUE / COUNT 1 / VALUE 1
 *
 *
 * Implementation : 1.  Push Selection이 수행된 경우만 변경한다.
 *                  2.  Dummy Statement를 생성한다.
 *                  3.  Transform된 Query를 설정한다.
 *                  4.  기존 Bind 정보를 유지하려고, Shard 구분용 Bind Offset를 설정한다.
 *                  5.  prasePartial를 수행한다.
 *                  6.  분석 대상인지 먼저 확인한다.
 *                  7.  Dummy Bind 정보를 설정한다.
 *                  8.  Validate를 수행한다.
 *                  9.  Analyze를 수행한다.
 *                  10. Shard 구분용 Bind Offset를 원복한다.
 *                  11. Shard Key Value를 복사한다.
 *                  12. Dummy Statement를 정리한다.
 *
 ****************************************************************************************/

    qcStatement    * sStatement        = NULL;
    qmsQuerySet    * sQuerySet         = NULL;
    sdiAnalyzeInfo * sAnalyzeInfo      = NULL;
    ULong            sSMN              = ID_ULONG(0);
    idBool           sIsShardParseTree = ID_FALSE;
    idBool           sIsShardQuerySet  = ID_FALSE;
    idBool           sIsTransformAble  = ID_FALSE;
    qcNamePosition * sStmtPos          = NULL;
    qcNamePosition   sStmtBuffer;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aInfo->mGraph == NULL, ERR_NULL_GRAPH );

    /* 1.  Push Selection이 수행된 경우만 변경한다. */
    IDE_TEST_CONT( aInfo->mPushPred == NULL, NORMAL_EXIT );
    IDE_TEST_CONT( aInfo->mGraph->shardAnalysis == NULL, NORMAL_EXIT );

    /* 2. */
    sStatement  = aInfo->mStatement;
    sQuerySet   = aInfo->mQuerySet;
    sStmtPos    = &( sStmtBuffer );

    if ( sdi::detectShardMetaChange( sStatement ) == ID_TRUE )
    {
        sSMN = sdi::getSMNForDataNode();
    }
    else
    {
        sSMN = QCG_GET_SESSION_SHARD_META_NUMBER( sStatement );
    }

    /* TASK-7219 Non-shard DML */
    sStatement->mShardPartialExecType = aInfo->mStatement->mShardPartialExecType;

    /* 3.  Transform된 Query를 설정한다. */
    sStmtPos->stmtText = aInfo->mGraph->mShardQueryBuf;
    sStmtPos->offset   = 0;
    sStmtPos->size     = aInfo->mGraph->mShardQueryLength;

    /* 4. */
    SDI_SET_QUERYSET_LIST_STATE( sStatement->mShardQuerySetList,
                                 SDI_QUERYSET_LIST_STATE_MAIN_PARTIAL );

    IDE_TEST( qmvShardTransform::doShardAnalyze( sStatement,
                                                 sStmtPos,
                                                 sSMN,
                                                 sQuerySet )
              != IDE_SUCCESS );

    IDE_TEST( sdi::isShardParseTree( sStatement->myPlan->parseTree,
                                     &( sIsShardParseTree ) )
              != IDE_SUCCESS );

    if ( sIsShardParseTree == ID_FALSE )
    {
        IDE_TEST( sdi::isShardQuerySet( sQuerySet,
                                        &( sIsShardQuerySet ),
                                        &( sIsTransformAble ) )
                  != IDE_SUCCESS );

        if ( ( sIsShardQuerySet == ID_FALSE )
             &&
             ( sIsTransformAble == ID_FALSE ) )
        {
            IDE_TEST_RAISE( aInfo->mUseShardKwd == ID_FALSE, ERR_UNEXPECTED );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromQuerySet( sStatement,
                                                      sQuerySet,
                                                      &( sAnalyzeInfo ) )
              != IDE_SUCCESS );

    aInfo->mGraph->shardAnalysis = sAnalyzeInfo;

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardKeyValue",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardKeyValue",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardKeyValue",
                                  "graph is null" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardKeyValue",
                                  "transformed query is non shard" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::setShardParameter( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Push Selection으로 Shard Key Bind가 내려갈 때에, Shard Key Bind
 *               정보를 조정하는 함수이다. qmvShardTransform::isShardQuery 함수 하위를
 *               참조하였다.
 *
 *               Transform 시 Bind 정보는 Transform 과정 중에 수집된 정보를 가지고, Shard
 *               Bind가 원본 Query에 연관된 Bind Offset으로 Array를 생성한다.
 *
 *               만약 qmgShardSelect::setShardKeyValue 가 수행되었다면, Transform된
 *               Query 내의 Bind Offset으로 Shard Key Bind를 설정하기 때문에,
 *               Shard Param Offset Array 정보와 상이하다.
 *
 *               Shard Param Offset Array는 Transform된 Query 내의 Shard Bind 개수와
 *               위치 해당하는 적절한 Offset이 기록되어 있으므로, 해당 정보로 변경해야 한다.
 *
 *
 *  BEFORE / SELECT CAST( ? AS INTEGER )
 *            FROM ( SELECT CAST( ? AS INTEGER ), C1
 *                    FROM T1 WHERE C3 != ?
 *                     ORDER BY C2 )
 *             WHERE C1 != ?;
 *
 *  BIND       / COUNT 4 / OFFSET 0 1 2 3
 *                                       \______________________________
 *                                                                      |
 *  AFTER1 / SELECT CAST( ? AS INTEGER )                                |
 *            FROM ( SELECT *                                           |
 *                    FROM ( SELECT CAST( ? AS INTEGER ), C1            |
 *                            FROM T1 WHERE C3 != ?                     |
 *                             ORDER BY C2 ) )                          |
 *             WHERE C1 != ?;                                           |
 *                                                                      |
 *  SHARD BIND  / COUNT 2 / OFFSET 1 2                                  |
 *  SHARD VALUE / COUNT 0 / NONE                                        |
 *                                                                      |
 *                                                                      |
 *  AFTER2 / SELECT CAST( ? AS INTEGER )                                |
 *            FROM ( SELECT *                                           |
 *                    FROM ( SELECT CAST( ? AS INTEGER ), C1 -- BIND 0  |
 *                            FROM T1 WHERE C3 != ?          -- BIND 1  |
 *                             AND C1 != ?                   -- BIND 2  |
 *                              ORDER BY C2 ) )                         |
 *             WHERE C1 != ?;            _______________________________|
 *                                      /                               |
 *  SHARD BIND  / COUNT 2 / OFFSET 1 2 3                                |
 *  SHARD VALUE / COUNT 1 / BIND 2    /                                 |
 *                                \__/                                  |
 *                                DIFF                                  |
 *                                                                      |
 *  qmgShardSelect::setShardParameter()                                 |
 *                                                                      |
 *  SHARD VALUE / COUNT 1 / BIND 3                                      |
 *                                \_____________________________________|
 *
 *
 * Implementation : 1.  Shard Param Offset Array를 생성한다.
 *                  2.  Push Selection이 수행된 경우만 변경한다.
 *                  3.  Shard Key Value를 변경한다.
 *                  4.  Shard SubKey Value를 변경한다.
 *
 ****************************************************************************************/

    UShort             sShardParamCount = ID_USHORT_MAX;
    qcShardParamInfo * sShardParamInfo  = NULL;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aInfo->mStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aInfo->mParamOffsetInfo == NULL, ERR_NULL_PARAM_INFO );
    IDE_TEST_RAISE( aInfo->mGraph == NULL, ERR_NULL_GRAPH );

    /* 1.  Shard Param Offset Array를 생성한다. */
    IDE_TEST( qmg::makeShardParamOffsetArrayForGraph( aInfo->mStatement,
                                                      aInfo->mParamOffsetInfo,
                                                      &( sShardParamCount ),
                                                      &( sShardParamInfo ) )
              != IDE_SUCCESS );

    aInfo->mGraph->shardParamCount       = sShardParamCount;
    aInfo->mGraph->shardParamInfo = sShardParamInfo;

    /* 2.  Push Selection이 수행된 경우만 변경한다. */
    IDE_TEST_CONT( aInfo->mPushPred == NULL, NORMAL_EXIT );
    IDE_TEST_CONT( aInfo->mGraph->shardAnalysis == NULL, NORMAL_EXIT );

    IDE_TEST( qmg::adjustParamOffsetForAnalyzeInfo( aInfo->mGraph->shardAnalysis,
                                                    sShardParamCount,
                                                    &sShardParamInfo )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardParameter",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardParameter",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardParameter",
                                  "graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PARAM_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardParameter",
                                  "param info is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::setShardTupleColumn( qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Push Projecion으로 Column 이 제거되는 상황을 고려해서 Tuple Column 에
 *               Flag 하는 함수이다.
 *
 *               Target이 제거된 형태의 Shard Query로 인해서, DATA NODE 부터 받아올
 *               Tuple 는 필요한 Target Column 만 연이어서 기록한다.
 *
 *               불필요한 Target Column은  Tuple Column 에 Flag 하고, 이후에
 *               Column Type Null 처리를 수행해야 한다.
 *
 *
 *  QUERY     / SELECT C2 FROM      ( SELECT C1, C2, C3, C4 FROM T1 ) ORDER BY C4
 *  ANALYZE   / SELECT C2 FROM SHARD( SELECT C1, C2, C3, C4 FROM T1 ) ORDER BY C4
 *  TRANSFROM /                SHARD( SELECT     C2,     C4 FROM T1 )
 *
 *  DATA      /                SHARD( SELECT     C2,     C4 FROM T1 )
 *   RESULT   /                     |- C2 -|- C4 -|
 *
 *
 * Implementation : 1. Push Projection 을 수행하는 경우만, 조정한다.
 *                  2. 사용중인 Target은 Tuple의 앞부분을 사용하도록 Offset를 조정한다.
 *
 ****************************************************************************************/

    qmsTarget * sTarget      = NULL;
    mtcTuple  * sTuple       = NULL;
    mtcColumn * sColumn      = NULL;
    UShort      sIdx         = 0;
    UShort      sTable       = 0;
    UShort      sColumnCount = 0;

    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );

    /* 1. Push Projection 을 수행하는 경우만, 조정한다. */
    IDE_TEST_CONT( aInfo->mTarget == NULL, NORMAL_EXIT );

    sTarget      = aInfo->mTarget;
    sTable       = aInfo->mGraph->graph.myFrom->tableRef->table;
    sTuple       = & QC_SHARED_TMPLATE( aInfo->mStatement )->tmplate.rows[ sTable ];    
    sColumnCount = sTuple->columnCount;
    sColumn      = sTuple->columns;

    /* 2. 사용중인 Target은 Tuple의 앞부분을 사용하도록 Offset를 조정한다. */
    for ( sIdx = 0, sColumn = sTuple->columns, sTarget = aInfo->mTarget;
          sIdx < sColumnCount;
          sIdx++, sColumn++, sTarget = sTarget->next )
    {
        if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) == QMS_TARGET_IS_USELESS_TRUE )
        {
            sColumn->flag &= ~MTC_COLUMN_NULL_TYPE_MASK;
            sColumn->flag |= MTC_COLUMN_NULL_TYPE_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::setShardTupleColumn",
                                  "info is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::makePushPredicate( qcStatement  * aStatement,
                                          qmgShardSELT * aMyGraph,
                                          qmsParseTree * aViewParseTree,
                                          qmsSFWGH     * aOuterQuery,
                                          SInt         * aRemain,
                                          qmoPredicate * aPredicate,
                                          qmgPushPred ** aHead,
                                          qmgPushPred ** aTail )
{
/****************************************************************************************
 *
 * Description : qmgSelection::doViewPushSelection 를 참조하여 Push Selection이 가능한
 *               경우에 해당 Predicate를 수집한다.
 *
 * Implementation : 1.   Predicate 검사한다.
 *                  2.   Predicate이 Push Down하기 Valid한 대상이어야 한다.
 *                  3.1. Push Selection을 검사한다.
 *                  3.2. 적절한 Table Predicate로 변경한다.
 *                  3.3. 해당 Predicate을 포함해서 Mtc Stack 제한을 검사한다.
 *                  3.4. 가능한 Predicate를 수집한다.
 *                  4.   aPredicate->next 먼저 검사한다.
 *                  5.   aPredicate->more 먼저 검사한다.
 *                  6.   Stack 사용량을 전달한다.
 *
 ****************************************************************************************/

    qmgPushPred  * sNewPred    = NULL;
    qtcNode      * sNode       = NULL;
    UShort         sTupleId    = ID_USHORT_MAX;
    idBool         sIsValid    = ID_FALSE;
    idBool         sIsPushPred = ID_FALSE;
    idBool         sIsOverflow = ID_FALSE;
    SInt           sRemain     = 0;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aMyGraph == NULL, ERR_NULL_MY_GRAPH );
    IDE_TEST_RAISE( aViewParseTree == NULL, ERR_NULL_PARSETREE );
    IDE_TEST_RAISE( aPredicate == NULL, ERR_NULL_PREDICATE );

    sRemain = *aRemain;

    IDE_TEST_CONT( sRemain < 1, NORMAL_EXIT );

    /* 1.   Predicate 검사한다. */
    IDE_TEST( qmoPred::isValidPushDownPredicate( aStatement,
                                                 &( aMyGraph->graph ),
                                                 aPredicate,
                                                 &( sIsValid ) )
              != IDE_SUCCESS );

    /* 2.   Predicate이 Push Down하기 Valid한 대상이어야 한다. */
    if ( sIsValid == ID_FALSE )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* 3.1. Push Selection을 검사한다. */
    sTupleId = aMyGraph->graph.myFrom->tableRef->table;

    IDE_TEST( qmoPushPred::checkPushDownPredicate( aStatement,
                                                   aViewParseTree,
                                                   aViewParseTree->querySet,
                                                   aOuterQuery,
                                                   sTupleId,
                                                   aPredicate,
                                                   &( sIsPushPred ) )
              != IDE_SUCCESS );

    if ( sIsPushPred == ID_TRUE )
    {
        /* 3.2. 적절한 Table Predicate로 변경한다. */
        IDE_TEST( qmoPushPred::changePredicateNodeName( aStatement,
                                                        aViewParseTree->querySet,
                                                        sTupleId,
                                                        aPredicate->node,
                                                        &( sNode ) )
                  != IDE_SUCCESS );

        /* 3.3. 해당 Predicate을 포함해서 Mtc Stack 제한을 검사한다.
         *   - AND의 Argument로 연결되어, 1를 감산한다.
         */
        sRemain -= 1;

        IDE_TEST( qmg::checkStackOverflow( &( sNode->node ),
                                           sRemain,
                                           &( sIsOverflow ) )
                  != IDE_SUCCESS );

        IDE_TEST_CONT( sIsOverflow == ID_TRUE, NORMAL_EXIT );

        /* 3.4. 가능한 Predicate를 수집한다. */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmgPushPred,
                                &( sNewPred ) )
                  != IDE_SUCCESS );

        sNewPred->mNode = sNode;
        sNewPred->mNext = NULL;

        if ( *aHead == NULL )
        {
            sNewPred->mId = 1;

            *aHead = sNewPred;
            *aTail = sNewPred;
        }
        else
        {
            sNewPred->mId = (*aTail)->mId + 1;

            (*aTail)->mNext = sNewPred;
            *aTail          = sNewPred;
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 4.   aPredicate->next 먼저 검사한다. */
    if ( aPredicate->next != NULL )
    {
        IDE_TEST( makePushPredicate( aStatement,
                                     aMyGraph,
                                     aViewParseTree,
                                     aOuterQuery,
                                     &( sRemain ),
                                     aPredicate->next,
                                     aHead,
                                     aTail )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 5.   aPredicate->more 먼저 검사한다. */
    if ( aPredicate->more != NULL )
    {
        IDE_TEST( makePushPredicate( aStatement,
                                     aMyGraph,
                                     aViewParseTree,
                                     aOuterQuery,
                                     &( sRemain ),
                                     aPredicate->more,
                                     aHead,
                                     aTail )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 6.   Stack 사용량을 전달한다. */
    if ( aRemain != NULL )
    {
        *aRemain = sRemain;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::makePushPredicate",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_MY_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::makePushPredicate",
                                  "my graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::makePushPredicate",
                                  "view parse tree is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PREDICATE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::makePushPredicate",
                                  "predicate tree is nul" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::checkPushProject( qcStatement  * aStatement,
                                         qmgShardSELT * aMyGraph,
                                         qmsParseTree * aParseTree,
                                         qmsTarget   ** aTarget,
                                         idBool       * aNeedWrapSet )
{
/****************************************************************************************
 *
 * Description : PROJ-2469 Optimize View Materialization의 View Target 최적화를 이용해서
 *               Push Projection 여부만 검사하는 함수이다.
 *
 *               Shard Graph 아래로 Graph 생성을 하지 않아서, View Target 최적화가
 *               Shard Graph 까지만 적용되어 있다. 따라서 Shard Graph의 View Target
 *               정보로 viewColumnRefList 를 조정하고, 이 정보로 Shard View Target 까지
 *               적용하도록 qmoCheckViewColumnRef::checkUselessViewTarget 호출한다.
 *               또 Implicit Order By Transform 정보를 전달한다.
 *
 *               이 함수로 불필요한 Target를 찾아내면, Shard Graph의
 *               qmgShardSelect::pushProjectOptimize 에서 그 Target를 제거한
 *               Shard Query으로 변환한다.
 *
 *               qmgShardSelect::pushProjectOptimize 에서 변환되는 Shard Query에 적합하게
 *               Tuple Column를 조정한다.
 *
 * Implementation : 1. 프로퍼티를 검사한다.
 *                  2. viewColumnRefList를 조정한다.
 *                  3. Shard View로 Push Projection 정보를 전달한다.
 *                  4. Shard View로 Implicit Order By Transform 정보를 전달한다.
 *
 ****************************************************************************************/

    qmsTarget        * sTarget       = NULL;
    qmsColumnRefList * sColumnRef    = NULL;
    qmsSortColumns   * sOrderBy      = NULL;
    idBool             sNeedPushProj = ID_FALSE;
    UInt               sTargetOrder  = 0;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aMyGraph == NULL, ERR_NULL_MY_GRAPH );
    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    /* 1. 프로퍼티를 검사한다. */
    IDE_TEST_CONT( ( SDU_SHARD_TRANSFORM_MODE & SDU_SHARD_TRANSFORM_PUSH_PROJECT_MASK )
                   != SDU_SHARD_TRANSFORM_PUSH_PROJECT_ENABLE,
                   NORMAL_EXIT );

    /* 2. viewColumnRefList를 조정한다. */
    for ( sTarget  = aMyGraph->graph.myQuerySet->target, sTargetOrder = 0;
          sTarget != NULL;
          sTarget  = sTarget->next, sTargetOrder++ )
    {
        if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK )
             == QMS_TARGET_IS_USELESS_FALSE )
        {
            for ( sColumnRef  = aMyGraph->graph.myFrom->tableRef->viewColumnRefList;
                  sColumnRef != NULL;
                  sColumnRef  = sColumnRef->next )
            {
                if ( sColumnRef->targetOrder == sTargetOrder )
                {
                    sColumnRef->isUsed = ID_TRUE;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    sTarget    = aParseTree->querySet->target;
    sColumnRef = aMyGraph->graph.myFrom->tableRef->viewColumnRefList;
    sOrderBy   = ( (qmsParseTree *)( aStatement->myPlan->parseTree ) )->orderBy;

    /* 3. Shard View로 Push Projection 정보를 전달한다. */
    IDE_TEST( qmoCheckViewColumnRef::checkUselessViewTarget( sTarget,
                                                             sColumnRef,
                                                             sOrderBy )
              != IDE_SUCCESS );

    /* 4. Shard View로 Implicit Order By Transform 정보를 전달한다. */
    for ( sTarget  = aParseTree->querySet->target;
          sTarget != NULL;
          sTarget  = sTarget->next )
    {
        if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK )
             == QMS_TARGET_IS_USELESS_TRUE )
        {
            sNeedPushProj = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
             != QMS_TARGET_SHARD_ORDER_BY_TRANS_NONE )
        {
            sTarget->flag &= ~QMS_TARGET_IS_USELESS_MASK;
            sTarget->flag |= QMS_TARGET_IS_USELESS_FALSE;

            sNeedPushProj = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    if ( sNeedPushProj == ID_TRUE )
    {
        if ( aParseTree->querySet->setOp != QMS_NONE )
        {
            *aNeedWrapSet = ID_TRUE;
            *aTarget      = aParseTree->querySet->target;
        }
        else
        {
            *aTarget = aParseTree->querySet->target;
        }
    }
    else
    {
        *aTarget = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushProject",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_MY_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushProject",
                                  "my graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushProject",
                                  "parse tree is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::checkPushSelect( qcStatement  * aStatement,
                                        qmgShardSELT * aMyGraph,
                                        qmsParseTree * aViewParseTree,
                                        qmsSFWGH     * aOuterQuery,
                                        qmgPushPred ** aPushPred,
                                        qtcNode     ** aWhere,
                                        idBool       * aNeedWrapSet )
{
/****************************************************************************************
 *
 * Description : 기존 Where의 Mtc Stack 제한를 고려해서 해당 Predicate를 수집한다.
 *
 * Implementation : 1.   프로퍼티를 검사한다.
 *                  2.1. Mtc Stack 사용량를 계산한다.
 *                  2.2. 기존 Where절 Mtc Stack 사용량를 감산한다
 *                  2.   Predicate를 수집한다.
 *                  3.   Push Selection 여부를 반환한다.
 *
 ****************************************************************************************/

    qmgPushPred * sPushPred = NULL;
    qmgPushPred * sTail     = NULL;
    qtcNode     * sWhere    = NULL;
    SInt          sRemain   = 0;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aMyGraph == NULL, ERR_NULL_MY_GRAPH );
    IDE_TEST_RAISE( aViewParseTree == NULL, ERR_NULL_PARSETREE );

    /* 1.   프로퍼티를 검사한다. */
    IDE_TEST_CONT( ( SDU_SHARD_TRANSFORM_MODE & SDU_SHARD_TRANSFORM_PUSH_SELECT_MASK )
                   != SDU_SHARD_TRANSFORM_PUSH_SELECT_ENABLE,
                   NORMAL_EXIT );

    if ( aMyGraph->graph.myPredicate != NULL )
    {
        /* 2.1. Mtc Stack 사용량를 계산한다. */
        sRemain = QCG_GET_SESSION_STACK_SIZE( aStatement );

        if ( aViewParseTree->querySet->setOp != QMS_NONE )
        {
            /* Nothing to do */
        }
        else
        {
            /* 2.2. 기존 Where절 Mtc Stack 사용량를 감산한다.
             *
             *   - 최상위 AND 연산자 추가를 고려해서, 1를 감산한다.
             *   - 최상위 Node가 AND인 경우에 Argument 개수만큼 감산한다.
             *
             *   - Push Selection을 AND 연산로 연결하는데,
             *   - 각각 AND 연산자는 최상위 AND 연산자의 다수 Argument로 묶인다.
             */
            sWhere = aViewParseTree->querySet->SFWGH->where;

            if ( sWhere != NULL )
            {
                sRemain -= 1;

                if ( ( sWhere->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_AND )
                {
                    sRemain -= (SInt)( sWhere->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* 3.   Predicate를 수집한다. */
        IDE_TEST( makePushPredicate( aStatement,
                                     aMyGraph,
                                     aViewParseTree,
                                     aOuterQuery,
                                     &( sRemain ),
                                     aMyGraph->graph.myPredicate,
                                     &( sPushPred ),
                                     &( sTail ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    /* 4.   Push Selection 여부를 반환한다. */
    if ( aViewParseTree->querySet->setOp != QMS_NONE )
    {
        if ( sPushPred != NULL )
        {
            *aNeedWrapSet = ID_TRUE;
            *aPushPred    = sPushPred;
            *aWhere       = NULL;
        }
        else
        {
            *aPushPred = NULL;
            *aWhere    = NULL;
        }
    }
    else
    {
        *aPushPred = sPushPred;
        *aWhere    = aViewParseTree->querySet->SFWGH->where;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushSelect",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_MY_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushSelect",
                                  "my graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushSelect",
                                  "parse tree is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::checkPushLimit( qcStatement     * aStatement,
                                       const qmgGraph  * aParent,
                                       qmgShardSELT    * aMyGraph,
                                       qmsParseTree    * aParseTree,
                                       qmsSortColumns ** aOrderBy,
                                       qmsLimit       ** aLimit )
{
/****************************************************************************************
 *
 * Description : Shard View 바로 위 Graph에서 Limit가 있는지 검사하는 함수이다.
 *               이러한 경우 원격지로 규칙에 맞게 Limit까지 Query로 변환해서,
 *               전송하면 결과가 달라지지 않는다.
 *
 * Implementation : 1. Limit가 있다면, 최적화하지 않는다.
 *                  2. 프로퍼티를 검사한다.
 *                  3. Where 절이 있다면, 최적화하지 않는다
 *                  4. Push Selection이 가능하면 Flag에 기록한다.
 *                  5. Push Limit이 가능하면 여부를 반환한다.
 *
 ****************************************************************************************/

    qmsLimit       * sLimit     = NULL;
    qmsSortColumns * sOrderBy   = NULL;

    IDE_TEST_RAISE( aParent == NULL, ERR_NULL_PARENT_GRAPH );
    IDE_TEST_RAISE( aMyGraph == NULL, ERR_NULL_MY_GRAPH );
    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    /* 1. Limit가 있다면, 최적화하지 않는다. */
    IDE_TEST_CONT( aParseTree->limit != NULL, NORMAL_EXIT );

    /* 2. 프로퍼티를 검사한다. */
    IDE_TEST_CONT( ( SDU_SHARD_TRANSFORM_MODE & SDU_SHARD_TRANSFORM_PUSH_LIMIT_MASK )
                   != SDU_SHARD_TRANSFORM_PUSH_LIMIT_ENABLE,
                   NORMAL_EXIT );

    /* 3. Where 절이 있다면, 최적화하지 않는다. */
    IDE_TEST_CONT( aMyGraph->graph.myPredicate != NULL, NORMAL_EXIT );

    /* 4. Parent Graph에서 Push할 대상을 검사한다. */
    if ( aParent->type == QMG_PROJECTION )
    {
        sLimit = ( (qmgPROJ *)aParent )->limit;
    }
    else if ( aParent->type == QMG_SORTING )
    {
        if ( ( aParent->flag & QMG_SORT_OPT_TIP_MASK ) == QMG_SORT_OPT_TIP_LMST )
        {
            sOrderBy = ( (qmgSORT *)aParent )->orderBy;
            sLimit   = ( (qmsParseTree *)( aStatement->myPlan->parseTree ) )->limit;

            IDE_TEST_RAISE( sOrderBy == NULL, ERR_NULL_ORDER_BY );
            IDE_TEST_RAISE( sLimit   == NULL, ERR_NULL_LIMIT );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    /* 5. Push Limit이 가능하면 여부를 반환한다. */
    if ( aOrderBy != NULL )
    {
        *aOrderBy = sOrderBy;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aLimit != NULL )
    {
        *aLimit = sLimit;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARENT_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushLimit",
                                  "parent graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_MY_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushLimit",
                                  "my graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushLimit",
                                  "parse tree is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ORDER_BY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushLimit",
                                  "order by is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIMIT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkPushLimit",
                                  "limit is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgShardSelect::checkAndGetPushSeries( qcStatement      * aStatement,
                                              const qmgGraph   * aParent,
                                              qmgShardSELT     * aMyGraph,
                                              qmgTransformInfo * aInfo )
{
/****************************************************************************************
 *
 * Description : Push Projection, Push Selection, Push Limit 최적화를 수행 가능한지
 *               최적화 요소를 검사한다.
 *
 * Implementation : 1. Plan String이 있을 때만 수행한다.
 *                  2. 키워드를 사용했다면, 최적화하지 않는다.
 *                  3. Parent Graph 에서 Push할 대상을 찾아온다.
 *                  4. My Graph 에서 Push할 대상을 검사한다.
 *
 ****************************************************************************************/

    qcParamOffsetInfo * sParamOffsetInfo = NULL;
    iduVarString      * sString          = NULL;
    qmsParseTree      * sViewParseTree   = NULL;
    qmsQuerySet       * sViewQuerySet    = NULL;

    /* TASK-7219 Non-shard DML */
    qmsSFWGH          * sOuterQuery      = NULL;

    qmsTarget         * sTarget          = NULL;
    qmgPushPred       * sPushPred        = NULL;
    qtcNode           * sWhere           = NULL;
    qmsSortColumns    * sOrderBy         = NULL;
    qmsLimit          * sLimit           = NULL;
    qmsForUpdate      * sForUpdate       = NULL; /* BUG-48961 */
    idBool              sNeedWrapSet     = ID_FALSE;
    idBool              sUseShardKwd     = ID_FALSE; /* TASK-7219 Shard Transformer Refactoring */
    idBool              sIsTransform     = ID_FALSE;
    idBool              sIsSupported     = ID_FALSE; /* TASK-7219 Shard Transformer Refactoring */

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aInfo == NULL, ERR_NULL_INFO );

    sString = (iduVarString *)qci::mSessionCallback.mGetPlanString( QC_MM_STMT( aStatement ) );

    /* 1. Plan String이 있을 때만 수행한다. */
    if ( sString != NULL )
    {
        IDE_TEST_RAISE( aMyGraph == NULL, ERR_NULL_MY_GRAPH );
        IDE_TEST_RAISE( aMyGraph->graph.myFrom == NULL, ERR_NULL_FROM );
        IDE_TEST_RAISE( aMyGraph->graph.myFrom->tableRef->view == NULL, ERR_NULL_VIEW );
        IDE_TEST_RAISE( aMyGraph->graph.myQuerySet->SFWGH == NULL, ERR_NULL_SFWGH );

        sViewParseTree = (qmsParseTree *)aMyGraph->graph.myFrom->tableRef->view->myPlan->parseTree;
        sViewQuerySet  = sViewParseTree->querySet;

        sOuterQuery = aMyGraph->graph.myQuerySet->SFWGH->outerQuery;

        /* BUG-48961
         * PUSH FOR UPDATE는 optimization이 아닌, 정합성을 위해 반드시 수행해야 하는 transformation이기 때문에
         * NORMAL_EXIT 검사 전에 수집한다.
         */
        if ( ((qmsParseTree*)aStatement->myPlan->parseTree)->forUpdate != NULL )
        {
            sForUpdate = ((qmsParseTree*)aStatement->myPlan->parseTree)->forUpdate;
            sIsTransform = ID_TRUE;
        }
        else
        {
            /* Nothing to do. */
        }

        /* 2. 키워드를 사용했다면, 최적화하지 않는다. */
        IDE_TEST_CONT( sViewParseTree->common.stmtShard != QC_STMT_SHARD_ANALYZE, NORMAL_EXIT );

        /* TASK-7219 Shard Transformer Refactoring */
        if ( sViewParseTree->isShardView == ID_TRUE )
        {
            sUseShardKwd = ID_TRUE;

            IDE_TEST( sdi::isSupported( sViewQuerySet,
                                        &( sIsSupported ) )
                      != IDE_SUCCESS );

            IDE_TEST_CONT( sIsSupported == ID_FALSE, NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }

        /* 3. My Graph 에서 Push할 대상을 검사한다. */
        IDE_TEST( checkPushProject( aStatement,
                                    aMyGraph,
                                    sViewParseTree,
                                    &( sTarget ),
                                    &( sNeedWrapSet ) )
                  != IDE_SUCCESS );

        if ( sTarget != NULL )
        {
            sIsTransform = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( checkPushSelect( aStatement,
                                   aMyGraph,
                                   sViewParseTree,
                                   sOuterQuery,
                                   &( sPushPred ),
                                   &( sWhere ),
                                   &( sNeedWrapSet ) )
                  != IDE_SUCCESS );

        if ( sPushPred != NULL )
        {
            sIsTransform = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        /* 4. Parent Graph 에서 Push할 대상을 찾아온다. */
        IDE_TEST( checkPushLimit( aStatement,
                                  aParent,
                                  aMyGraph,
                                  sViewParseTree,
                                  &( sOrderBy ),
                                  &( sLimit ) )
                  != IDE_SUCCESS );

        if ( sLimit != NULL )
        {
            sIsTransform = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    if ( sIsTransform == ID_TRUE )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qcParamOffsetInfo,
                                &( sParamOffsetInfo ) )
                  != IDE_SUCCESS );

        QC_INIT_PARAM_OFFSET_INFO( sParamOffsetInfo );
    }
    else
    {
        /* Nothing to do */
    }


    aInfo->mString          = sString;
    aInfo->mGraph           = aMyGraph;
    aInfo->mStatement       = aStatement;
    aInfo->mQuerySet        = sViewQuerySet;
    aInfo->mTarget          = sTarget;
    aInfo->mPushPred        = sPushPred;
    aInfo->mWhere           = sWhere;
    aInfo->mOrderBy         = sOrderBy;
    aInfo->mLimit           = sLimit;
    aInfo->mForUpdate       = sForUpdate;
    aInfo->mParamOffsetInfo = sParamOffsetInfo;
    aInfo->mNeedWrapSet     = sNeedWrapSet;
    aInfo->mUseShardKwd     = sUseShardKwd; /* TASK-7219 Shard Transformer Refactoring */
    aInfo->mIsTransform     = sIsTransform;

    SET_POSITION( aInfo->mPosition, aMyGraph->shardQuery );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkAndGetPushSeries",
                                  "statement is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkAndGetPushSeries",
                                  "info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_MY_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkAndGetPushSeries",
                                  "my graph is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_FROM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkAndGetPushSeries",
                                  "from is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkAndGetPushSeries",
                                  "shard viewis null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardSelect::checkAndGetPushSeries",
                                  "SFWGH is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
