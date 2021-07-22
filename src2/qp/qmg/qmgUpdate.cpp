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
 * $Id: qmgUpdate.cpp 53774 2012-06-15 04:53:31Z eerien $
 *
 * Description :
 *     Update Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgUpdate.h>
#include <qmo.h>
#include <qmoOneNonPlan.h>
#include <qmgSelection.h>
#include <qmgPartition.h>
#include <qmoPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE
#include <qcmPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE
#include <qmsDefaultExpr.h>

IDE_RC
qmgUpdate::init( qcStatement * aStatement,
                 qmsQuerySet * aQuerySet,
                 qmgGraph    * aChildGraph,
                 qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgUpdate Graph의 초기화
 *
 * Implementation :
 *    (1) qmgUpdate을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgUPTE         * sMyGraph;
    qmmUptParseTree * sParseTree;
    qmsQuerySet     * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgUpdate::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Update Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgUpdate을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgUPTE ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_UPDATE;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgUpdate::optimize;
    sMyGraph->graph.makePlan = qmgUpdate::makePlan;
    sMyGraph->graph.printGraph = qmgUpdate::printGraph;

    // Disk/Memory 정보 설정
    for ( sQuerySet = aQuerySet;
          sQuerySet->left != NULL;
          sQuerySet = sQuerySet->left ) ;

    switch(  sQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED :
            // 중간 결과 Type Hint가 없는 경우, 하위의 Type을 따른다.
            if ( ( aChildGraph->flag & QMG_GRAPH_TYPE_MASK )
                 == QMG_GRAPH_TYPE_DISK )
            {
                sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
                sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
            }
            else
            {
                sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
                sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
            }
            break;
        case QMO_INTER_RESULT_TYPE_DISK :
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
            break;
        case QMO_INTER_RESULT_TYPE_MEMORY :
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
            break;
        default :
            IDE_DASSERT( 0 );
            break;
    }

    //---------------------------------------------------
    // Update Graph 만을 위한 초기화
    //---------------------------------------------------

    sParseTree = (qmmUptParseTree *)aStatement->myPlan->parseTree;

    /* PROJ-2204 JOIN UPDATE, DELETE */
    sMyGraph->updateTableRef    = sParseTree->updateTableRef;
    
    // 최상위 graph인 update graph에 update columns 정보를 설정
    sMyGraph->columns           = sParseTree->updateColumns;
    sMyGraph->updateColumnList  = sParseTree->uptColumnList;
    sMyGraph->updateColumnCount = sParseTree->uptColCount;
    
    // 최상위 graph인 update graph에 update values 정보를 설정
    sMyGraph->values         = sParseTree->values;
    sMyGraph->subqueries     = sParseTree->subqueries;
    sMyGraph->lists          = sParseTree->lists;
    sMyGraph->valueIdx       = sParseTree->valueIdx;
    sMyGraph->canonizedTuple = sParseTree->canonizedTuple;
    sMyGraph->compressedTuple= sParseTree->compressedTuple;     // PROJ-2264
    sMyGraph->isNull         = sParseTree->isNull;

    // 최상위 graph인 update graph에 sequence 정보를 설정
    sMyGraph->nextValSeqs = sParseTree->common.nextValSeqs;
    
    // 최상위 graph인 update graph에 instead of trigger정보를 설정
    sMyGraph->insteadOfTrigger = sParseTree->insteadOfTrigger;

    // 최상위 graph인 update graph에 limit을 연결
    sMyGraph->limit = sParseTree->limit;

    // 최상위 graph인 update graph에 constraint를 연결
    sMyGraph->parentConstraints = sParseTree->parentConstraints;
    sMyGraph->childConstraints  = sParseTree->childConstraints;
    sMyGraph->checkConstrList   = sParseTree->checkConstrList;

    // 최상위 graph인 update graph에 return into를 연결
    sMyGraph->returnInto = sParseTree->returnInto;

    // 최상위 graph인 insert graph에 Default Expr을 연결
    sMyGraph->defaultExprTableRef = sParseTree->defaultTableRef;
    sMyGraph->defaultExprColumns  = sParseTree->defaultExprColumns;

    /* PROJ-2714 Multiple update, delete support */
    sMyGraph->mTableList = sParseTree->mTableList;
    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgUpdate::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgUpdate의 최적화
 *
 * Implementation :
 *    (1) CASE 2 : UPDATE...SET column = (subquery)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) CASE 3 : UPDATE...SET (column list) = (subquery)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (3) SCAN Limit 최적화
 *    (4) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgUPTE           * sMyGraph;
    qmgGraph          * sChildGraph;
    qmsTableRef       * sTableRef;
    idBool              sIsIntersect = ID_FALSE;
    qtcNode           * sNode;
    qmsLimit          * sLimit;
    idBool              sIsScanLimit;
    idBool              sIsRowMovementUpdate = ID_FALSE;
    idBool              sHasMemory = ID_FALSE;

    SDouble             sOutputRecordCnt;
    SDouble             sUpdateSelectivity = 1;    // BUG-17166

    IDU_FIT_POINT_FATAL( "qmgUpdate::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgUPTE*) aGraph;
    sChildGraph = aGraph->left;

    sTableRef = sMyGraph->updateTableRef;

    //---------------------------------------------------
    // update hidden column 최적화
    //---------------------------------------------------

    /* PROJ-1090 Function-based Index */
    IDE_TEST( qmsDefaultExpr::makeBaseColumn(
                  aStatement,
                  sTableRef->tableInfo,
                  sMyGraph->defaultExprColumns,
                  &(sMyGraph->defaultExprBaseColumns) )
              != IDE_SUCCESS );

    // Disk Table에 Default Expr을 적용
    /* PROJ-2464 hybrid partitioned table 지원
     *  qmoPartition::optimizeInto()에서 Partition의 Tuple에 설정하므로,
     *  qmoPartition::optimizeInto()를 호출하기 전에 수행해야 한다.
     */
    qmsDefaultExpr::setUsedColumnToTableRef(
        &(QC_SHARED_TMPLATE(aStatement)->tmplate),
        sTableRef,
        sMyGraph->defaultExprBaseColumns,
        ID_TRUE );

    //---------------------------------------------------
    // Row Movement 최적화
    //---------------------------------------------------

    // PROJ-1502 PARTITIONED DISK TABLE
    // partitioned table의 경우 set절에 partition key가 포함되어 있고,
    // table의 row movement가 활성화 되어 있다면,
    // insert table reference를 사용해야 한다.
    if ( sTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qmoPartition::isIntersectPartKeyColumn(
                      sTableRef->tableInfo->partKeyColumns,
                      sMyGraph->columns,
                      & sIsIntersect )
                  != IDE_SUCCESS );

        if ( sIsIntersect == ID_TRUE )
        {
            if ( sTableRef->tableInfo->rowMovement == ID_TRUE )
            {
                sIsRowMovementUpdate = ID_TRUE;
            
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                              ID_SIZEOF(qmsTableRef),
                              (void**)&sMyGraph->insertTableRef )
                          != IDE_SUCCESS );

                idlOS::memcpy( sMyGraph->insertTableRef,
                               sTableRef,
                               ID_SIZEOF(qmsTableRef) );

                /* PROJ-2464 hybrid partitioned table 지원
                 *  Partition Summary는 공유하지 않는다.
                 */
                sMyGraph->insertTableRef->partitionSummary = NULL;

                // row movement가 활성화 되어 있다면
                // pre pruning을 무시하고 다른 파티션에도
                // row가 옮겨갈 수 있어야 한다.
                sMyGraph->insertTableRef->partitionRef = NULL;

                sMyGraph->insertTableRef->flag &=
                    ~QMS_TABLE_REF_PRE_PRUNING_MASK;
                sMyGraph->insertTableRef->flag |=
                    QMS_TABLE_REF_PRE_PRUNING_FALSE;

                // optimizeInto를 사용.
                // insert(row movement)될 수 있는 컬럼이 속하여 있기 때문.
                // values노드에 대해서만 pruning을 한다.
                // list형도 결국 values노드로 구성되며,
                // subquery형의 경우 pruning대상에서 원래부터 제외된다.
                IDE_TEST( qmoPartition::optimizeInto(
                              aStatement,
                              sMyGraph->insertTableRef )
                          != IDE_SUCCESS );
            
                // insert가 일어나는 partition list에서
                // update가 일어나는 partition list와 같은 것이 있으면 제거한다.
                // insert/delete/update를 하나의 커서만으로 해야 하기 때문임.
                IDE_TEST( qmoPartition::minusPartitionRef(
                              sMyGraph->insertTableRef,
                              sTableRef )
                          != IDE_SUCCESS );

                /* PROJ-2464 hybrid partitioned table 지원 */
                IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sMyGraph->insertTableRef )
                          != IDE_SUCCESS );

                // row movement가 발생하면 delete-insert로 처리하는 update
                sMyGraph->updateType = QMO_UPDATE_ROWMOVEMENT;    
                // insert/update/delete가 모두 발생할 수 있는 cursor
                sMyGraph->cursorType = SMI_COMPOSITE_CURSOR;
            }
            else
            {
                sMyGraph->insertTableRef = NULL;
                
                // row movement가 발생하면 에러내는 update
                sMyGraph->updateType = QMO_UPDATE_CHECK_ROWMOVEMENT;
                // update cursor
                sMyGraph->cursorType = SMI_UPDATE_CURSOR;
            }
        }
        else
        {
            sMyGraph->insertTableRef = NULL;
            
            // row movement가 일어나지 않는 update
            sMyGraph->updateType = QMO_UPDATE_NO_ROWMOVEMENT;
            // update cursor
            sMyGraph->cursorType = SMI_UPDATE_CURSOR;
        }
    }
    else
    {
        sMyGraph->insertTableRef = NULL;
        
        // 일반 테이블의 update
        sMyGraph->updateType = QMO_UPDATE_NORMAL;
        // update cursor
        sMyGraph->cursorType = SMI_UPDATE_CURSOR;
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    // rowmovement가 일어날 가능성이 있는지를 parsetree에 저장.
    sMyGraph->isRowMovementUpdate = sIsRowMovementUpdate;

    //---------------------------------------------------
    // Cursor 설정
    //---------------------------------------------------
    
    //----------------------------------
    // PROJ-1509
    // INPLACE UPDATE 여부 설정
    // MEMORY table에서는,
    // trigger or foreign key가 존재하는 경우,
    // 변경 이전/이후 값을 읽기 위해서는
    // inplace update가 되지 않도록 flag정보를 sm에 내려준다.
    //----------------------------------

    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( sTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if ( ( sTableRef->partitionSummary->memoryPartitionCount +
               sTableRef->partitionSummary->volatilePartitionCount ) > 0 )
        {
            sHasMemory = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        if ( ( ( sTableRef->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_MEMORY ) ||
             ( ( sTableRef->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_VOLATILE ) )
        {
            sHasMemory = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    // inplace update
    if ( ( sHasMemory == ID_TRUE ) &&
         ( ( sTableRef->tableInfo->foreignKeys != NULL ) ||
           ( sTableRef->tableInfo->triggerInfo != NULL ) ) )
    {
        sMyGraph->inplaceUpdate = ID_FALSE;
    }
    else
    {
        sMyGraph->inplaceUpdate = ID_TRUE;
    }

    //---------------------------------------------------
    // SCAN Limit 최적화
    //---------------------------------------------------

    sIsScanLimit = ID_FALSE;
    if ( sMyGraph->limit != NULL )
    {
        if ( sChildGraph->type == QMG_SELECTION )
        {
            //---------------------------------------------------
            // 하위가 일반 qmgSelection인 경우
            // 즉, Set, Order By, Group By, Aggregation, Distinct, Join이
            //  없는 경우
            //---------------------------------------------------
            if ( sChildGraph->myFrom->tableRef->view == NULL )
            {
                // View 가 아닌 경우, SCAN Limit 적용
                sNode = (qtcNode *)sChildGraph->myQuerySet->SFWGH->where;

                if ( sNode != NULL )
                {
                    // where가 존재하는 경우, subquery 존재 유무 검사
                    if ( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK )
                         != QTC_NODE_SUBQUERY_EXIST )
                    {
                        sIsScanLimit = ID_TRUE;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                else
                {
                    // where가 존재하지 않는 경우,SCAN Limit 적용
                    sIsScanLimit = ID_TRUE;
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // Set, Order By, Group By, Distinct, Aggregation, Join, View가
            // 있는 경우 :
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }

    //---------------------------------------------------
    // SCAN Limit Tip이 적용된 경우
    //---------------------------------------------------

    if ( sIsScanLimit == ID_TRUE )
    {
        ((qmgSELT *)sChildGraph)->limit = sMyGraph->limit;

        // To Fix BUG-9560
        IDE_TEST(
            QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmsLimit ),
                                           ( void**) & sLimit )
            != IDE_SUCCESS );

        // fix BUG-13482
        // parse tree의 limit정보를 현재 가지며,
        // UPTE 노드 생성시,
        // 하위 SCAN 노드에서 SCAN Limit 적용이 확정되면,
        // UPTE 노드의 limit start를 1로 변경한다.

        qmsLimitI::setStart( sLimit,
                             qmsLimitI::getStart( sMyGraph->limit ) );

        qmsLimitI::setCount( sLimit,
                             qmsLimitI::getCount( sMyGraph->limit ) );

        SET_EMPTY_POSITION( sLimit->limitPos );

        sMyGraph->limit = sLimit;
    }
    else
    {
        // nothing to do
    }

    //---------------------------------------------------
    // 공통 비용 정보 설정
    //---------------------------------------------------

    // recordSize
    sMyGraph->graph.costInfo.recordSize =
        sChildGraph->costInfo.recordSize;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = sUpdateSelectivity;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sChildGraph->costInfo.outputRecordCnt;

    // outputRecordCnt
    sOutputRecordCnt = sChildGraph->costInfo.outputRecordCnt * sUpdateSelectivity;
    sMyGraph->graph.costInfo.outputRecordCnt = sOutputRecordCnt;

    // myCost
    sMyGraph->graph.costInfo.myAccessCost = 0;
    sMyGraph->graph.costInfo.myDiskCost = 0;
    sMyGraph->graph.costInfo.myAllCost = 0;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost =
        sChildGraph->costInfo.totalAccessCost +
        sMyGraph->graph.costInfo.myAccessCost;

    sMyGraph->graph.costInfo.totalDiskCost =
        sChildGraph->costInfo.totalDiskCost +
        sMyGraph->graph.costInfo.myDiskCost;

    sMyGraph->graph.costInfo.totalAllCost =
        sChildGraph->costInfo.totalAllCost +
        sMyGraph->graph.costInfo.myAllCost;

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |=
        ( sChildGraph->flag & QMG_PRESERVED_ORDER_MASK );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgUpdate::makePlan( qcStatement     * aStatement,
                     const qmgGraph  * aParent,
                     qmgGraph        * aGraph )
{
/***********************************************************************
 *
 * Description : qmgUpdate으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgUpdate으로 생성가능한 Plan
 *
 *           [UPTE]
 *
 ***********************************************************************/

    qmgUPTE         * sMyGraph;
    qmnPlan         * sPlan;
    qmnPlan         * sChildPlan;
    qmmSubqueries   * sSubquery;
    qmmValueNode    * sValue;
    qmoUPTEInfo       sUPTEInfo;

    qmsPartitionRef * sPartitionRef = NULL;
    UInt              sRowOffset    = 0;
    qmmMultiTables  * sTmp;

    IDU_FIT_POINT_FATAL( "qmgUpdate::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgUPTE*) aGraph;

    //---------------------------
    // Current CNF의 등록
    //---------------------------

    if ( sMyGraph->graph.myCNF != NULL )
    {
        sMyGraph->graph.myQuerySet->SFWGH->crtPath->currentCNF =
            sMyGraph->graph.myCNF;
    }
    else
    {
        // Nothing To Do
    }

    /* PROJ-1071 Parallel Query */
    sMyGraph->graph.flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    sMyGraph->graph.flag |= QMG_PARALLEL_IMPOSSIBLE_TRUE;

    //---------------------------
    // Plan의 생성
    //---------------------------

    // 최상위 plan이다.
    IDE_DASSERT( aParent == NULL );

    if ( sMyGraph->mTableList == NULL )
    {
        IDE_TEST( qmoOneNonPlan::initUPTE( aStatement ,
                                           &sPlan )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( qmoOneNonPlan::initMultiUPTE( aStatement,
                                                sMyGraph->graph.myQuerySet,
                                                sMyGraph->mTableList,
                                                &sPlan )
                  != IDE_SUCCESS);
    }
    sMyGraph->graph.myPlan = sPlan;

    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( sMyGraph->graph.left->makePlan( aStatement ,
                                              &sMyGraph->graph,
                                              sMyGraph->graph.left )
              != IDE_SUCCESS);

    // fix BUG-13482
    // SCAN Limit 최적화 적용에 따른 UPTE plan의 start value 결정유무
    if( sMyGraph->graph.left->type == QMG_SELECTION )
    {
        // projection 하위가 SCAN이고,
        // SCAN limit 최적화가 적용된 경우이면,
        // UPTE의 limit start value를 1로 조정한다.
        if( ((qmgSELT*)(sMyGraph->graph.left))->limit != NULL )
        {
            qmsLimitI::setStartValue( sMyGraph->limit, 1 );
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    // child가져오기
    sChildPlan = sMyGraph->graph.left->myPlan;

    //---------------------------------------------------
    // Process 상태 설정 
    //---------------------------------------------------
    sMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_UPDATE;

    //----------------------------
    // UPTE의 생성
    //----------------------------
    
    sUPTEInfo.updateTableRef      = sMyGraph->updateTableRef;
    sUPTEInfo.columns             = sMyGraph->columns;
    sUPTEInfo.updateColumnList    = sMyGraph->updateColumnList;
    sUPTEInfo.updateColumnCount   = sMyGraph->updateColumnCount;
    sUPTEInfo.values              = sMyGraph->values;
    sUPTEInfo.subqueries          = sMyGraph->subqueries;
    sUPTEInfo.valueIdx            = sMyGraph->valueIdx;
    sUPTEInfo.canonizedTuple      = sMyGraph->canonizedTuple;
    sUPTEInfo.compressedTuple     = sMyGraph->compressedTuple;  // PROJ-2264
    sUPTEInfo.isNull              = sMyGraph->isNull;
    sUPTEInfo.nextValSeqs         = sMyGraph->nextValSeqs;
    sUPTEInfo.insteadOfTrigger    = sMyGraph->insteadOfTrigger;
    sUPTEInfo.updateType          = sMyGraph->updateType;
    sUPTEInfo.cursorType          = sMyGraph->cursorType;
    sUPTEInfo.inplaceUpdate       = sMyGraph->inplaceUpdate;
    sUPTEInfo.insertTableRef      = sMyGraph->insertTableRef;
    sUPTEInfo.isRowMovementUpdate = sMyGraph->isRowMovementUpdate;
    sUPTEInfo.limit               = sMyGraph->limit;
    sUPTEInfo.parentConstraints   = sMyGraph->parentConstraints;
    sUPTEInfo.childConstraints    = sMyGraph->childConstraints;
    sUPTEInfo.checkConstrList     = sMyGraph->checkConstrList;
    sUPTEInfo.returnInto          = sMyGraph->returnInto;

    /* PROJ-1090 Function-based Index */
    sUPTEInfo.defaultExprTableRef    = sMyGraph->defaultExprTableRef;
    sUPTEInfo.defaultExprColumns     = sMyGraph->defaultExprColumns;
    sUPTEInfo.defaultExprBaseColumns = sMyGraph->defaultExprBaseColumns;
    sUPTEInfo.mTableList             = sMyGraph->mTableList;

    if ( sMyGraph->mTableList == NULL )
    {
        IDE_TEST( qmoOneNonPlan::makeUPTE( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           & sUPTEInfo ,
                                           sChildPlan ,
                                           sPlan )
                  != IDE_SUCCESS);

        sMyGraph->graph.myPlan = sPlan;

        /* PROJ-2464 hybrid partitioned table 지원 */
        // qmnUPTE::updateOneRowForRowmovement()에서 rowOffset을 사용한다.
        if ( sMyGraph->insertTableRef != NULL )
        {
            for ( sPartitionRef = sMyGraph->insertTableRef->partitionRef;
                  sPartitionRef != NULL;
                  sPartitionRef = sPartitionRef->next )
            {
                sRowOffset = qmc::getRowOffsetForTuple( &(QC_SHARED_TMPLATE(aStatement)->tmplate),
                                                        sPartitionRef->table );
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table].rowOffset  = sRowOffset;
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table].rowMaximum = sRowOffset;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        IDE_TEST( qmoOneNonPlan::makeMultiUPTE( aStatement,
                                                sMyGraph->graph.myQuerySet,
                                                &sUPTEInfo,
                                                sChildPlan,
                                                sPlan )
                  != IDE_SUCCESS);

        sMyGraph->graph.myPlan = sPlan;

        for ( sTmp = sMyGraph->mTableList; sTmp != NULL; sTmp = sTmp->mNext )
        {
            if ( sTmp->mInsertTableRef != NULL )
            {
                for ( sPartitionRef = sTmp->mInsertTableRef->partitionRef;
                      sPartitionRef != NULL;
                      sPartitionRef = sPartitionRef->next )
                {
                    sRowOffset = qmc::getRowOffsetForTuple( &(QC_SHARED_TMPLATE(aStatement)->tmplate),
                                                            sPartitionRef->table );
                    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table].rowOffset  = sRowOffset;
                    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table].rowMaximum = sRowOffset;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    //------------------------------------------
    // MAKE GRAPH
    // BUG-32584
    // 모든 서브쿼리에 대해서 MakeGraph 한후에 MakePlan을 해야 한다.
    //------------------------------------------

    //------------------------------------------
    // SET 구문 내의 Subquery 최적화
    // ex) SET i1 = (subquery)
    //------------------------------------------

    for ( sSubquery = sMyGraph->subqueries;
          sSubquery != NULL;
          sSubquery = sSubquery->next )
    {
        if ( sSubquery->subquery != NULL )
        {
            // Subquery 최적화
            IDE_TEST( qmoSubquery::optimizeExprMakeGraph( aStatement,
                                                          ID_UINT_MAX,
                                                          sSubquery->subquery )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    //------------------------------------------
    // SET 구문의 LIST 내의 Subquery 최적화
    // ex) SET (i1,i2) = (SELECT a1, a2 ... )
    //------------------------------------------

    for ( sValue = sMyGraph->lists;
          sValue != NULL;
          sValue = sValue->next )
    {
        if ( sValue->value != NULL )
        {
            // Subquery 존재할 경우 Subquery 최적화
            if ( (sValue->value->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                IDE_TEST( qmoSubquery::optimizeExprMakeGraph( aStatement,
                                                              ID_UINT_MAX,
                                                              sValue->value )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nohting To Do
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    //------------------------------------------
    // VALUE 내의 Subquery 최적화
    // ex) SET i1 = (1 + subquery)
    //------------------------------------------

    for ( sValue = sMyGraph->values;
          sValue != NULL;
          sValue = sValue->next )
    {
        if ( sValue->value != NULL )
        {
            // Subquery 존재할 경우 Subquery 최적화
            if ( (sValue->value->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                IDE_TEST( qmoSubquery::optimizeExprMakeGraph( aStatement,
                                                              ID_UINT_MAX,
                                                              sValue->value )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nohting To Do
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    //------------------------------------------
    // MAKE PLAN
    // BUG-32584
    // 모든 서브쿼리에 대해서 MakeGraph 한후에 MakePlan을 해야 한다.
    //------------------------------------------

    //------------------------------------------
    // SET 구문 내의 Subquery 최적화
    // ex) SET i1 = (subquery)
    //------------------------------------------

    for ( sSubquery = sMyGraph->subqueries;
          sSubquery != NULL;
          sSubquery = sSubquery->next )
    {
        if ( sSubquery->subquery != NULL )
        {
            // Subquery 최적화
            IDE_TEST( qmoSubquery::optimizeExprMakePlan( aStatement,
                                                         ID_UINT_MAX,
                                                         sSubquery->subquery )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    //------------------------------------------
    // SET 구문의 LIST 내의 Subquery 최적화
    // ex) SET (i1,i2) = (SELECT a1, a2 ... )
    //------------------------------------------

    for ( sValue = sMyGraph->lists;
          sValue != NULL;
          sValue = sValue->next )
    {
        if ( sValue->value != NULL )
        {
            // Subquery 존재할 경우 Subquery 최적화
            if ( (sValue->value->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                IDE_TEST( qmoSubquery::optimizeExprMakePlan( aStatement,
                                                             ID_UINT_MAX,
                                                             sValue->value )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nohting To Do
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    //------------------------------------------
    // VALUE 내의 Subquery 최적화
    // ex) SET i1 = (1 + subquery)
    //------------------------------------------

    for ( sValue = sMyGraph->values;
          sValue != NULL;
          sValue = sValue->next )
    {
        if ( sValue->value != NULL )
        {
            // Subquery 존재할 경우 Subquery 최적화
            if ( (sValue->value->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                IDE_TEST( qmoSubquery::optimizeExprMakePlan( aStatement,
                                                             ID_UINT_MAX,
                                                             sValue->value )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nohting To Do
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgUpdate::printGraph( qcStatement  * aStatement,
                       qmgGraph     * aGraph,
                       ULong          aDepth,
                       iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *    Graph를 구성하는 공통 정보를 출력한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  i;

    IDU_FIT_POINT_FATAL( "qmgUpdate::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

    //-----------------------------------
    // Graph의 시작 출력
    //-----------------------------------

    if ( aDepth == 0 )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "----------------------------------------------------------" );
    }
    else
    {
        // Nothing To Do
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


    //-----------------------------------
    // Child Graph 고유 정보의 출력
    //-----------------------------------

    IDE_TEST( aGraph->left->printGraph( aStatement,
                                        aGraph->left,
                                        aDepth + 1,
                                        aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Graph의 마지막 출력
    //-----------------------------------

    if ( aDepth == 0 )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "----------------------------------------------------------\n\n" );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Description : qmgMultiUpdate의 최적화
 *
 * Implementation :
 *    (1) CASE 2 : UPDATE...SET column = (subquery)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) CASE 3 : UPDATE...SET (column list) = (subquery)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (3) 공통 비용 정보 설정
 */
IDE_RC qmgUpdate::optimizeMultiUpdate( qcStatement * aStatement, qmgGraph * aGraph )
{
    qmgUPTE           * sMyGraph;
    qmgGraph          * sChildGraph;
    qmmMultiTables    * sTmp;
    idBool              sIsIntersect = ID_FALSE;
    idBool              sHasMemory = ID_FALSE;
    SDouble             sOutputRecordCnt;
    SDouble             sUpdateSelectivity = 1;    // BUG-17166

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------
    sMyGraph = (qmgUPTE*) aGraph;
    sChildGraph = aGraph->left;

    //---------------------------------------------------
    // update hidden column 최적화
    //---------------------------------------------------
    for ( sTmp = sMyGraph->mTableList; sTmp != NULL; sTmp = sTmp->mNext )
    {
        /* PROJ-1090 Function-based Index */
        IDE_TEST( qmsDefaultExpr::makeBaseColumn( aStatement,
                                                  sTmp->mTableRef->tableInfo,
                                                  sTmp->mDefaultColumns,
                                                  &(sTmp->mDefaultBaseColumns) )
                  != IDE_SUCCESS );

        // Disk Table에 Default Expr을 적용
        /* PROJ-2464 hybrid partitioned table 지원
         *  qmoPartition::optimizeInto()에서 Partition의 Tuple에 설정하므로,
         *  qmoPartition::optimizeInto()를 호출하기 전에 수행해야 한다.
         */
        qmsDefaultExpr::setUsedColumnToTableRef( &(QC_SHARED_TMPLATE(aStatement)->tmplate),
                                                 sTmp->mTableRef,
                                                 sTmp->mDefaultBaseColumns,
                                                 ID_TRUE );

        //---------------------------------------------------
        // Row Movement 최적화
        //---------------------------------------------------
        if ( sTmp->mTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST( qmoPartition::isIntersectPartKeyColumn( sTmp->mTableRef->tableInfo->partKeyColumns,
                                                              sTmp->mColumns,
                                                              &sIsIntersect )
                      != IDE_SUCCESS );

            if ( sIsIntersect == ID_TRUE )
            {
                if ( sTmp->mTableRef->tableInfo->rowMovement == ID_TRUE )
                {
                    IDU_FIT_POINT("qmgUpdate::optimizeMultiUpdate::alloc::mInsertTableRef",
                                  idERR_ABORT_InsufficientMemory);
                    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsTableRef),
                                                             (void**)&sTmp->mInsertTableRef )
                              != IDE_SUCCESS );
                    idlOS::memcpy( sTmp->mInsertTableRef,
                                   sTmp->mTableRef,
                                   ID_SIZEOF(qmsTableRef) );
                    /* PROJ-2464 hybrid partitioned table 지원
                     *  Partition Summary는 공유하지 않는다.
                     */
                    sTmp->mInsertTableRef->partitionSummary = NULL;

                    // row movement가 활성화 되어 있다면
                    // pre pruning을 무시하고 다른 파티션에도
                    // row가 옮겨갈 수 있어야 한다.
                    sTmp->mInsertTableRef->partitionRef = NULL;

                    sTmp->mInsertTableRef->flag &= ~QMS_TABLE_REF_PRE_PRUNING_MASK;
                    sTmp->mInsertTableRef->flag |= QMS_TABLE_REF_PRE_PRUNING_FALSE;

                    // optimizeInto를 사용.
                    // insert(row movement)될 수 있는 컬럼이 속하여 있기 때문.
                    // values노드에 대해서만 pruning을 한다.
                    // list형도 결국 values노드로 구성되며,
                    // subquery형의 경우 pruning대상에서 원래부터 제외된다.
                    IDE_TEST( qmoPartition::optimizeInto( aStatement,
                                                          sTmp->mInsertTableRef )
                              != IDE_SUCCESS );

                    // insert가 일어나는 partition list에서
                    // update가 일어나는 partition list와 같은 것이 있으면 제거한다.
                    // insert/delete/update를 하나의 커서만으로 해야 하기 때문임.
                    IDE_TEST( qmoPartition::minusPartitionRef( sTmp->mInsertTableRef,
                                                               sTmp->mTableRef )
                              != IDE_SUCCESS );

                    /* PROJ-2464 hybrid partitioned table 지원 */
                    IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sTmp->mInsertTableRef )
                              != IDE_SUCCESS );

                    // row movement가 발생하면 delete-insert로 처리하는 update
                    sTmp->mUpdateType = QMO_UPDATE_ROWMOVEMENT;
                    // insert/update/delete가 모두 발생할 수 있는 cursor
                    sTmp->mCursorType = SMI_COMPOSITE_CURSOR;
                }
                else
                {
                    sTmp->mInsertTableRef = NULL;
                    // row movement가 발생하면 에러내는 update
                    sTmp->mUpdateType = QMO_UPDATE_CHECK_ROWMOVEMENT;
                    // update cursor
                    sTmp->mCursorType = SMI_UPDATE_CURSOR;
                }
            }
            else
            {
                sTmp->mInsertTableRef = NULL;
                // row movement가 일어나지 않는 update
                sTmp->mUpdateType = QMO_UPDATE_NO_ROWMOVEMENT;
                // update cursor
                sTmp->mCursorType = SMI_UPDATE_CURSOR;
            }
        }
        else
        {
            sTmp->mInsertTableRef = NULL;
            // row movement가 일어나지 않는 update
            sTmp->mUpdateType = QMO_UPDATE_NO_ROWMOVEMENT;
            // update cursor
            sTmp->mCursorType = SMI_UPDATE_CURSOR;
        }
    }
    sMyGraph->insertTableRef = NULL;
    // row movement가 일어나지 않는 update
    sMyGraph->updateType = QMO_UPDATE_NO_ROWMOVEMENT;
    // update cursor
    sMyGraph->cursorType = SMI_UPDATE_CURSOR;
    // PROJ-1502 PARTITIONED DISK TABLE
    // rowmovement가 일어날 가능성이 있는지를 parsetree에 저장.
    sMyGraph->isRowMovementUpdate = ID_FALSE;

    //---------------------------------------------------
    // Cursor 설정
    //---------------------------------------------------

    //----------------------------------
    // PROJ-1509
    // INPLACE UPDATE 여부 설정
    // MEMORY table에서는,
    // trigger or foreign key가 존재하는 경우,
    // 변경 이전/이후 값을 읽기 위해서는
    // inplace update가 되지 않도록 flag정보를 sm에 내려준다.
    //----------------------------------

    for ( sTmp = sMyGraph->mTableList; sTmp != NULL; sTmp = sTmp->mNext )
    {
        /* PROJ-2464 hybrid partitioned table 지원 */
        if ( sTmp->mTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            if ( ( sTmp->mTableRef->partitionSummary->memoryPartitionCount +
                   sTmp->mTableRef->partitionSummary->volatilePartitionCount ) > 0 )
            {
                sHasMemory = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( ( ( sTmp->mTableRef->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_MEMORY ) ||
                 ( ( sTmp->mTableRef->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_VOLATILE ) )
            {
                sHasMemory = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        // inplace update
        if ( ( sHasMemory == ID_TRUE ) &&
             ( ( sTmp->mTableRef->tableInfo->foreignKeys != NULL ) ||
               ( sTmp->mTableRef->tableInfo->triggerInfo != NULL ) ) )
        {
            sTmp->mInplaceUpdate = ID_FALSE;
        }
        else
        {
            sTmp->mInplaceUpdate = ID_TRUE;
        }
    }

    //---------------------------------------------------
    // 공통 비용 정보 설정
    //---------------------------------------------------

    // recordSize
    sMyGraph->graph.costInfo.recordSize = sChildGraph->costInfo.recordSize;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = sUpdateSelectivity;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt = sChildGraph->costInfo.outputRecordCnt;

    // outputRecordCnt
    sOutputRecordCnt = sChildGraph->costInfo.outputRecordCnt * sUpdateSelectivity;
    sMyGraph->graph.costInfo.outputRecordCnt = sOutputRecordCnt;

    // myCost
    sMyGraph->graph.costInfo.myAccessCost = 0;
    sMyGraph->graph.costInfo.myDiskCost = 0;
    sMyGraph->graph.costInfo.myAllCost = 0;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost = sChildGraph->costInfo.totalAccessCost +
                                                sMyGraph->graph.costInfo.myAccessCost;

    sMyGraph->graph.costInfo.totalDiskCost = sChildGraph->costInfo.totalDiskCost +
                                                sMyGraph->graph.costInfo.myDiskCost;

    sMyGraph->graph.costInfo.totalAllCost = sChildGraph->costInfo.totalAllCost +
                                            sMyGraph->graph.costInfo.myAllCost;

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |= ( sChildGraph->flag & QMG_PRESERVED_ORDER_MASK );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

