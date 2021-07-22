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
 * $Id: qmgPartition.cpp 20233 2007-02-01 01:58:21Z shsuh $
 *
 * Description :
 *     Partition Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgPartition.h>
#include <qcg.h>
#include <qmgSelection.h>
#include <qmoMultiNonPlan.h>
#include <qmo.h>
#include <qmoCost.h>
#include <qcuProperty.h>
#include <qmoPartition.h>
#include <qcmPartition.h>
#include <qtc.h>
#include <qcgPlan.h>
#include <qmoParallelPlan.h>

/* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
#define QMG_REDUCE_PART_PREPARE_MEM_DISK_TUPLE_MASK   ( 0x00000001 )
#define QMG_REDUCE_PART_PREPARE_MEM_DISK_TUPLE_FALSE  ( 0x00000000 )
#define QMG_REDUCE_PART_PREPARE_MEM_DISK_TUPLE_TRUE   ( 0x00000001 )

#define QMG_REDUCE_PART_PREPARE_MEM_STAT_COLUMN_MASK  ( 0x00000002 )
#define QMG_REDUCE_PART_PREPARE_MEM_STAT_COLUMN_FALSE ( 0x00000000 )
#define QMG_REDUCE_PART_PREPARE_MEM_STAT_COLUMN_TRUE  ( 0x00000002 )

IDE_RC
qmgPartition::init( qcStatement * aStatement,
                    qmsQuerySet * aQuerySet,
                    qmsFrom     * aFrom,
                    qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgPartition Graph의 초기화
 *
 * Implementation :
 *    (1) qmgPartition을 위한 공간 할당 받음
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgPARTITION  * sMyGraph;
    qmsTableRef   * sTableRef;

    IDU_FIT_POINT_FATAL( "qmgPartition::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );

    //---------------------------------------------------
    // Partition Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgPartition을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPARTITION ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );


    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_PARTITION;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aFrom->depInfo );
    sMyGraph->graph.myFrom = aFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgPartition::optimize;
    sMyGraph->graph.makePlan = qmgPartition::makePlan;
    sMyGraph->graph.printGraph = qmgPartition::printGraph;

    // Disk/Memory 정보 설정
    sTableRef =  sMyGraph->graph.myFrom->tableRef;
    if ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTableRef->table].lflag
           & MTC_TUPLE_STORAGE_MASK ) == MTC_TUPLE_STORAGE_DISK )
    {
        sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
        sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
    }
    else
    {
        sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
        sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
    }

    //---------------------------------------------------
    // Partition 고유 정보의 초기화
    //---------------------------------------------------

    sMyGraph->limit = NULL;

    sMyGraph->partKeyRange = NULL;
    sMyGraph->partFilterPredicate = NULL;
    
    sMyGraph->selectedIndex = NULL;
    sMyGraph->selectedMethod = NULL;
    sMyGraph->accessMethodCnt = 0;
    sMyGraph->accessMethod = NULL;

    sMyGraph->forceIndexScan = ID_FALSE;

    // BUG-48800
    sMyGraph->mPrePruningPartRef = NULL;    

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgPartition의 최적화
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgPARTITION    * sMyGraph;
    qmsTableRef     * sTableRef;
    qmsPartitionRef * sPartitionRef;
    qmoPredicate    * sPredicate;
    qmoPredicate    * sMyPredicate;
    qmoPredicate    * sPartKeyRangePred;
    qtcNode         * sPartKeyRangeNode;
    smiRange        * sPartKeyRange;
    qmgGraph       ** sChildGraph;
    UInt              sPartitionCount;
    UInt              i;
    UInt              sFlag;    
    UShort            sColumnCnt;
    mtcTuple        * sMtcTuple;    
    UInt              sSelectedScanHint;
    idBool            sIsIndexTableScan = ID_FALSE;
    idBool            sExistIndexTable = ID_FALSE;
    idBool            sIsLocked = ID_FALSE;
    SDouble           sOutputRecordCnt;
    mtcColumn       * sColumns = NULL;
    qmoStatistics   * sStatInfo = NULL;

    IDU_FIT_POINT_FATAL( "qmgPartition::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph          = (qmgPARTITION*) aGraph;
    sTableRef         = sMyGraph->graph.myFrom->tableRef;
    sPredicate        = NULL;
    sMyPredicate      = NULL;
    sPartKeyRangePred = NULL;
    sPartKeyRange     = NULL;
    sPartitionCount   = 0;
    i                 = 0;

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        /* TASK-7219 Non-shard DML */
        IDE_TEST( qmo::removeOutRefPredPushedForce( & sMyGraph->graph.myPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // PROJ-1404
    // 불필요한 bad transitive predicate을 제거한다.
    //---------------------------------------------------
    
    // 생성된 transitive predicate이 사용자가 입력한 predicate과 중복되는
    // 경우 가능한 이를 제거하는 것이 좋다.
    // (아직은 생성하는 transitive predicate이 one table predicate이라서
    // selection graph에서 처리할 수 있다.)
    //
    // 예1)
    // select * from ( select * from t1 where t1.i1=1 ) v1, t2
    // where v1.i1=t2.i1 and t2.i1=1;
    // transitive predicate {t1.i1=1}을 생성하지만
    // v1 where절의 predicate과 중복되므로 제거하는 것이 좋다.
    //
    // 예2)
    // select * from t1 left join t2 on t1.i1=t2.i1 and t1.i1=1
    // where t2.i1=1;
    // on절에서 transitive predicate {t2.i1=1}을 생성하지만
    // 이는 right로 내려가게 되고 where절에서 내려온
    // predicate과 중복되므로 제거하는 것이 좋다.
    
    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::removeEquivalentTransitivePredicate(
                      aStatement,
                      & sMyGraph->graph.myPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    //---------------------------------------------------
    // Subquery의 Graph 생성
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST(
            qmoPred::optimizeSubqueries( aStatement,
                                         sMyGraph->graph.myPredicate,
                                         ID_TRUE ) // Use KeyRange Tip
            != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------
    // partition keyrange추출 및 검사
    //---------------------------------------------------

    // 일단 모든 파티션에 대한 정보를 구축한다.
    IDE_TEST(
        qmoPartition::makePartitions(
            aStatement,
            sTableRef )
        != IDE_SUCCESS );

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        // partition pruning을 위해 predicate를 복사.
        // fixed predicate에 대해서만 partition keyrange를 추출할
        // 수 있으므로,
        // subquery, variable predicate를 제외하고 복사한다.

        // partition key range 출력용
        IDE_TEST(qmoPred::copyPredicate4Partition(
                     aStatement,
                     sMyGraph->graph.myPredicate,
                     &sPredicate,
                     sTableRef->table,
                     sTableRef->table,
                     ID_FALSE )
                 != IDE_SUCCESS );

        // predicate을 복사하였기 때문에 intermediate tuple을 새로 할당함.
        IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                           QC_SHARED_TMPLATE(aStatement) )
                  != IDE_SUCCESS );
        
        // subquery predicate을 제외하였기 때문에 null일 수 있음.
        if( sPredicate != NULL )
        {
            // predicate의 재배치(통계정보 무시하고 그냥 재배치만 한다.)
            IDE_TEST( qmoPred::relocatePredicate4PartTable(
                          aStatement,
                          sPredicate,
                          sTableRef->tableInfo->partitionMethod,
                          & sMyGraph->graph.depInfo,
                          & sMyGraph->graph.myQuerySet->outerDepInfo,
                          & sPredicate )
                      != IDE_SUCCESS );
            
            // partition keyrange predicate 생성.
            IDE_TEST( qmoPred::makePartKeyRangePredicate(
                          aStatement,
                          sMyGraph->graph.myQuerySet,
                          sPredicate,
                          sTableRef->tableInfo->partKeyColumns,
                          sTableRef->tableInfo->partitionMethod,
                          &sPartKeyRangePred )
                      != IDE_SUCCESS );

            // partition keyrange predicate로부터 partition keyrange추출.
            IDE_TEST( extractPartKeyRange(
                          aStatement,
                          sMyGraph->graph.myQuerySet,
                          &sPartKeyRangePred,
                          sTableRef->tableInfo->partKeyColCount,
                          sTableRef->tableInfo->partKeyColBasicInfo,
                          sTableRef->tableInfo->partKeyColsFlag,
                          &sPartKeyRangeNode,
                          &sPartKeyRange )
                      != IDE_SUCCESS );

            // sPartKeyRangeNode는 출력용이다.
            // sPartKeyRange는 프루닝용.
        }
        else
        {
            // Nothing to do.
        }

        if( sPartKeyRange != NULL )
        {
            // BUG-48800 명시한 파티션이 파티션 필터링되면 플랜이 rebuild하지 않습니다.
            if ( (sTableRef->flag & QMS_TABLE_REF_PRE_PRUNING_MASK)
                 == QMS_TABLE_REF_PRE_PRUNING_TRUE ) 
            {
                sMyGraph->mPrePruningPartRef = sTableRef->partitionRef;
            }

            /* BUG-47521 hybrid partition foreignkey bug */
            if ( ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_UPDATE ) &&
                 ( sTableRef->tableInfo->rowMovement == ID_TRUE ) &&
                 ( ( sTableRef->tableInfo->foreignKeyCount > 0 ) ||
                   ( sTableRef->tableInfo->triggerCount > 0 ) ) )
            {
                IDE_TEST( qcmPartition::validateAndLockPartitions(
                              aStatement,
                              sTableRef,
                              SMI_TABLE_LOCK_IS )
                          != IDE_SUCCESS );
                sIsLocked = ID_TRUE;

                /* PROJ-2464 hybrid partitioned table 지원 */
                IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sTableRef )
                          != IDE_SUCCESS );

                if ( sTableRef->partitionSummary->isHybridPartitionedTable == ID_FALSE )
                {
                    IDE_TEST( qmoPartition::partitionPruningWithKeyRange(
                                  aStatement,
                                  sTableRef,
                                  sPartKeyRange )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                IDE_TEST( qmoPartition::partitionPruningWithKeyRange(
                              aStatement,
                              sTableRef,
                              sPartKeyRange )
                          != IDE_SUCCESS );
            }

            sMyGraph->partKeyRange = sPartKeyRangeNode;
        }
        else
        {
            // partition keyrange가 없다.
            // pruning을 할 필요가 없음.
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // partition별 통계 정보 구축
    //---------------------------------------------------
    if ( sIsLocked == ID_FALSE )
    {
        IDE_TEST( qcmPartition::validateAndLockPartitions(
                      aStatement,
                      sTableRef,
                      SMI_TABLE_LOCK_IS )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2464 hybrid partitioned table 지원 */
    IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sTableRef )
              != IDE_SUCCESS );

    // 선택된 파티션의 개수를 구하면서
    // 통계정보를 구한다.
    // 튜플 셋을 할당한다.
    /* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
    if ( ( sTableRef->partitionSummary->isHybridPartitionedTable != ID_TRUE ) &&
         ( sTableRef->tableInfo->lobColumnCount == 0 ) &&
         ( QCM_TABLE_TYPE_IS_DISK(sTableRef->tableInfo->tableFlag) == ID_TRUE ) &&
         ( ( QCG_GET_REDUCE_PART_PREPARE_MEMORY( aStatement ) & QMG_REDUCE_PART_PREPARE_MEM_DISK_TUPLE_MASK )
            == QMG_REDUCE_PART_PREPARE_MEM_DISK_TUPLE_TRUE ) &&
         ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK )
           == QCI_STMT_MASK_DML ) &&
         ( ( aStatement->mFlag & QC_STMT_NO_PART_COLUMN_REDUCE_MASK )
           != QC_STMT_NO_PART_COLUMN_REDUCE_TRUE ) )
    {
        sColumns = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTableRef->table].columns;
    }
    else
    {
        sColumns = NULL;
    }

    for ( sPartitionCount = 0, sPartitionRef = sTableRef->partitionRef;
          sPartitionRef != NULL;
          sPartitionCount++, sPartitionRef = sPartitionRef->next )
    {
        IDE_TEST( qmoStat::getStatInfo4BaseTable( aStatement,
                                                  sMyGraph->graph.myQuerySet->SFWGH->hints->optGoalType,
                                                  sPartitionRef->partitionInfo,
                                                  &(sPartitionRef->statInfo) )
                  != IDE_SUCCESS );

        IDE_TEST(qtc::nextTable( &(sPartitionRef->table),
                                 aStatement,
                                 sPartitionRef->partitionInfo,
                                 QCM_TABLE_TYPE_IS_DISK(sPartitionRef->partitionInfo->tableFlag),
                                 MTC_COLUMN_NOTNULL_TRUE )
                 != IDE_SUCCESS);

        if ( sColumns != NULL )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->free( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table].columns)
                      != IDE_SUCCESS );

            QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table].columns = sColumns;
        }
        else
        {
            /* Nothing to do */
        }

        QC_SHARED_TMPLATE(aStatement)->tableMap[sPartitionRef->table].from = sMyGraph->graph.myFrom;
    }

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_REDUCE_PART_PREPARE_MEMORY );

    // index table의 통계정보를 구한다.
    for ( i = 0; i < sTableRef->indexTableCount; i++ )
    {
        IDE_TEST( qmoStat::getStatInfo4BaseTable(
                      aStatement,
                      sMyGraph->graph.myQuerySet->SFWGH->hints->optGoalType,
                      sTableRef->indexTableRef[i].tableInfo,
                      &(sTableRef->indexTableRef[i].statInfo) )
                  != IDE_SUCCESS );
    }

    // 각각의 partition에 대한 통계 정보를 구한 후,
    // 이를 이용하여 partitioned table의 통계 정보를 구한다.
    IDE_TEST( qmoStat::getStatInfo4PartitionedTable(
                  aStatement,
                  sMyGraph->graph.myQuerySet->SFWGH->hints->optGoalType,
                  sTableRef,
                  sTableRef->statInfo )
              != IDE_SUCCESS );

    /* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
    if ( ( QCG_GET_REDUCE_PART_PREPARE_MEMORY( aStatement ) & QMG_REDUCE_PART_PREPARE_MEM_STAT_COLUMN_MASK )
         == QMG_REDUCE_PART_PREPARE_MEM_STAT_COLUMN_TRUE )
    {
        sStatInfo = sTableRef->statInfo;

        /* Partitioned에 설정된 column 정보를 초기화하고 각 partition에
         * 있는 column 은 해제한다
         */
        for ( sColumnCnt = 0; sColumnCnt < sStatInfo->columnCnt ; sColumnCnt++ )
        {
            sStatInfo->colCardInfo[sColumnCnt].isValidStat = ID_FALSE;
            sStatInfo->colCardInfo[sColumnCnt].flag &= ~QMO_STAT_MINMAX_COLUMN_SET_MASK;
            sStatInfo->colCardInfo[sColumnCnt].flag |= QMO_STAT_MINMAX_COLUMN_SET_FALSE;
        }
        for ( sPartitionCount = 0, sPartitionRef = sTableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionCount++, sPartitionRef = sPartitionRef->next )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->free( sPartitionRef->statInfo->colCardInfo )
                      != IDE_SUCCESS );

            sPartitionRef->statInfo->colCardInfo = sStatInfo->colCardInfo;
        }
    }
    //---------------------------------------------------
    // global index를 위한 predicate 최적화
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        // selectivity 계산용으로 복사
        IDE_TEST(qmoPred::deepCopyPredicate(
                     QC_QMP_MEM(aStatement),
                     sMyGraph->graph.myPredicate,
                     & sMyPredicate )
                 != IDE_SUCCESS );

        // Predicate의 재배치 및 개별 Predicate의 Selectivity 계산
        // selectivity 계산과 access method 게산용으로만 사용한다.
        IDE_TEST( qmoPred::relocatePredicate(
                      aStatement,
                      sMyPredicate,
                      & sMyGraph->graph.depInfo,
                      & sMyGraph->graph.myQuerySet->outerDepInfo,
                      sTableRef->statInfo,
                      & sMyPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------------------
    // child 최적화
    //---------------------------------------------------------------

    if( sPartitionCount >= 1 )
    {
        //선택된 파티션이 최소 한개 이상인 경우임.

        //---------------------------------------------------------------
        // child graph의 생성
        //---------------------------------------------------------------

        // graph->children구조체의 메모리 할당.
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(qmgChildren) * sPartitionCount,
                      (void**) &sMyGraph->graph.children )
                  != IDE_SUCCESS );

        // child graph pointer의 메모리 할당.
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(qmgGraph *) * sPartitionCount,
                      (void**) &sChildGraph )
                  != IDE_SUCCESS );

        for( i = 0, sPartitionRef = sTableRef->partitionRef;
             (sPartitionRef != NULL) && (i < sPartitionCount);
             i++, sPartitionRef = sPartitionRef->next )
        {
            IDE_TEST( qmgSelection::init( aStatement,
                                          sMyGraph->graph.myQuerySet,
                                          sMyGraph->graph.myFrom,
                                          sPartitionRef,
                                          &sChildGraph[i] )
                      != IDE_SUCCESS );

            // child graph에 predicate를 내림.
            // join graph에서 이미 내려올 것은 다 내려온 상태.
            // subquery predicate를 빼고 나머지 predicate은 모두 내린다.
            // 주의점 : 복사해서 내려야 함.
            // TODO1502: in subquery keyrange만 못내리는건지
            // 다 내리면 안되는건지 철저한 조사 필요.
            IDE_TEST(qmoPred::copyPredicate4Partition(
                         aStatement,
                         sMyGraph->graph.myPredicate,
                         &sChildGraph[i]->myPredicate,
                         sTableRef->table,
                         sPartitionRef->table,
                         ID_TRUE )
                     != IDE_SUCCESS );

            // PROJ-1789 PROWID
            IDE_TEST(qmoPred::copyPredicate4Partition(
                         aStatement,
                         sMyGraph->graph.ridPredicate,
                         &sChildGraph[i]->ridPredicate,
                         sTableRef->table,
                         sPartitionRef->table,
                         ID_TRUE )
                     != IDE_SUCCESS );
        }

        // predicate을 복사하였기 때문에 intermediate tuple을 새로 할당함.
        IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                           QC_SHARED_TMPLATE(aStatement) )
                  != IDE_SUCCESS );

        //---------------------------------------------------------------
        // child graph의 optimize
        //---------------------------------------------------------------

        for( i = 0, sPartitionRef = sTableRef->partitionRef;
             (sPartitionRef != NULL) && (i < sPartitionCount);
             i++, sPartitionRef = sPartitionRef->next )
        {
            // PROJ-1705
            // validation시 수집된 질의에 사용된 컬럼정보 전파

            sMtcTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table]);

            for( sColumnCnt = 0;
                 sColumnCnt <
                     QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTableRef->table].columnCount;
                 sColumnCnt++ )
            {
                sFlag = QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTableRef->table].columns[sColumnCnt].flag;

                /* PROJ-2464 hybrid partitioned table 지원
                 *  Partition별로 DML Without Retry를 지원하기 위해,
                 *  MTC_COLUMN_USE_WHERE_MASK와 MTC_COLUMN_USE_SET_MASK를 Partition Tuple에 복사한다.
                 */
                sMtcTuple->columns[sColumnCnt].flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
                sMtcTuple->columns[sColumnCnt].flag &= ~MTC_COLUMN_USE_TARGET_MASK;
                sMtcTuple->columns[sColumnCnt].flag &= ~MTC_COLUMN_USE_WHERE_MASK;
                sMtcTuple->columns[sColumnCnt].flag &= ~MTC_COLUMN_USE_SET_MASK;

                sMtcTuple->columns[sColumnCnt].flag |= sFlag & MTC_COLUMN_USE_COLUMN_MASK;
                sMtcTuple->columns[sColumnCnt].flag |= sFlag & MTC_COLUMN_USE_TARGET_MASK;
                sMtcTuple->columns[sColumnCnt].flag |= sFlag & MTC_COLUMN_USE_WHERE_MASK;
                sMtcTuple->columns[sColumnCnt].flag |= sFlag & MTC_COLUMN_USE_SET_MASK;
            }

            // 각각의 파티션 별로 optimize.
            IDE_TEST( sChildGraph[i]->optimize( aStatement,
                                                sChildGraph[i] )
                      != IDE_SUCCESS );

            sMyGraph->graph.children[i].childGraph = sChildGraph[i];

            if( i == sPartitionCount - 1 )
            {
                sMyGraph->graph.children[i].next = NULL;
            }
            else
            {
                sMyGraph->graph.children[i].next =
                    &sMyGraph->graph.children[i+1];
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // accessMethod 선택
    //---------------------------------------------------

    // rid predicate 이 있는 경우 무조건 rid scan 을 시도한다.
    // rid predicate 이 있더라도  rid scan 을 할수 없는 경우도 있다.
    // 이 경우에도 index scan 이 되지 않고 full scan 을 하게 된다.
    if ( sMyGraph->graph.ridPredicate != NULL )
    {
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmoAccessMethod),
                                               (void**)&sMyGraph->accessMethod)
                 != IDE_SUCCESS);

        sMyGraph->accessMethod->method               = NULL;
        sMyGraph->accessMethod->keyRangeSelectivity  = 0;
        sMyGraph->accessMethod->keyFilterSelectivity = 0;
        sMyGraph->accessMethod->filterSelectivity    = 0;
        sMyGraph->accessMethod->methodSelectivity    = 0;

        sMyGraph->accessMethod->totalCost = qmoCost::getTableRIDScanCost(
            sTableRef->statInfo,
            &sMyGraph->accessMethod->accessCost,
            &sMyGraph->accessMethod->diskCost );

        sMyGraph->selectedIndex   = NULL;
        sMyGraph->accessMethodCnt = 1;
        sMyGraph->selectedMethod  = &sMyGraph->accessMethod[0];

        sSelectedScanHint = QMG_NOT_USED_SCAN_HINT;
    }
    else
    {
        // 사용 가능한 access method들을 세팅한다.
        sMyGraph->accessMethodCnt = sTableRef->tableInfo->indexCount + 1;

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoAccessMethod) *
                                                 ( sMyGraph->accessMethodCnt ),
                                                 (void**) & sMyGraph->accessMethod )
                  != IDE_SUCCESS );

        IDE_TEST( qmgSelection::getBestAccessMethod( aStatement,
                                                     & sMyGraph->graph,
                                                     sTableRef->statInfo,
                                                     sMyPredicate,
                                                     sMyGraph->accessMethod,
                                                     & sMyGraph->selectedMethod,
                                                     & sMyGraph->accessMethodCnt,
                                                     & sMyGraph->selectedIndex,
                                                     & sSelectedScanHint,
                                                     1,
                                                     0 )
                  != IDE_SUCCESS);

        // BUG-37125 tpch plan optimization
        // 파티션 프루닝이 되었을때 부정확한 통계정보로 scan method 를 정하고 있다.
        // 각각의 파티션의 cost 값의 합을 가지고 scan method 를 설정해야 한다.
        IDE_TEST( reviseAccessMethodsCost( sMyGraph,
                                           sPartitionCount )
                  != IDE_SUCCESS);

        for( i = 0; i < sMyGraph->accessMethodCnt; i++ )
        {
            if( QMO_COST_IS_GREATER(sMyGraph->selectedMethod->totalCost,
                                    sMyGraph->accessMethod[i].totalCost) )
            {
                sMyGraph->selectedMethod = &(sMyGraph->accessMethod[i]);
            }
            else
            {
                // Nothing to do.
            }
        }

        if( sMyGraph->selectedMethod->method == NULL )
        {
            sMyGraph->selectedIndex = NULL;
        }
        else
        {
            sMyGraph->selectedIndex = sMyGraph->selectedMethod->method->index;
        }

        // To fix BUG-12742
        // hint를 사용한 경우는 index를 제거할 수 없다.
        if( (sSelectedScanHint == QMG_USED_SCAN_HINT) ||
            (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE) )
        {
            sMyGraph->forceIndexScan = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        //---------------------------------------------------
        // selected index가 global index인 경우
        // hint가 global index인 경우
        //---------------------------------------------------

        if ( sMyGraph->selectedIndex != NULL )
        {
            if ( sMyGraph->selectedIndex->indexPartitionType ==
                 QCM_NONE_PARTITIONED_INDEX )
            {
                // (global) index table scan
                sIsIndexTableScan = ID_TRUE;
            }
            else
            {
                // local index scan
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    // partition이 1개인 경우
    // global index가 선택된 경우
    // full scan hint가 있는 경우
    // index scan hint가 있는 경우

    if ( sIsIndexTableScan == ID_TRUE )
    {
        //---------------------------------------------------
        // INDEX TABLE SCAN이 선택된 경우
        //---------------------------------------------------

        // To Fix PR-9181
        // Index Scan이라 할 지라도
        // IN SUBQUERY KEY RANGE가 사용될 경우
        // Order가 보장되지 않는다.
        if ( ( sMyGraph->selectedMethod->method->flag &
               QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK )
             == QMO_STAT_CARD_IDX_IN_SUBQUERY_TRUE )
        {
            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
        }
        else
        {
            IDE_TEST( qmgSelection::makePreservedOrder(
                          aStatement,
                          sMyGraph->selectedMethod->method,
                          sTableRef->table,
                          & sMyGraph->graph.preservedOrder )
                      != IDE_SUCCESS );

            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
        }
    }
    else
    {
        if ( sMyGraph->selectedMethod->method == NULL )
        {
            //---------------------------------------------------
            // FULL SCAN이 선택된 경우
            //---------------------------------------------------

            if( sSelectedScanHint == QMG_USED_ONLY_FULL_SCAN_HINT )
            {
                //---------------------------------------------------
                // FULL SCAN Hint가 선택된 경우
                //---------------------------------------------------

                sMyGraph->graph.preservedOrder = NULL;

                sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
            }
            else
            {
                //---------------------------------------------------
                // cost에 의해 FULL SCAN이 선택된 경우
                //---------------------------------------------------

                if ( sMyGraph->accessMethodCnt > 1 )
                {
                    for ( i = 1; i < sMyGraph->accessMethodCnt; i++ )
                    {
                        if ( sMyGraph->accessMethod[i].method->index->indexPartitionType ==
                             QCM_NONE_PARTITIONED_INDEX )
                        {
                            sExistIndexTable = ID_TRUE;
                            break;
                        }
                        else
                        {
                            // nothing to do
                        }
                    }

                    if ( sExistIndexTable == ID_TRUE )
                    {
                        // global index가 존재하는 경우
                        sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                        sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NOT_DEFINED;
                    }
                    else
                    {
                        // global index가 없는 경우
                        sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                        sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
                    }
                }
                else
                {
                    // global index가 없는 경우
                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
                }
            }
        }
        else
        {
            //---------------------------------------------------
            // LOCAL INDEX SCAN이 선택된 경우
            //---------------------------------------------------

            // local index로는 order를 가질 수 없다.
            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
        }
    }

    //---------------------------------------------------
    // Parallel query 정보의 설정
    //---------------------------------------------------
    if (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT)
    {
        if (sTableRef->mParallelDegree > 1)
        {
            // parallel degree 가 2 이상이더라도
            // access method 가 다음과 같을 경우는
            // parallel execution 이 불가능하다.
            //
            // - GLOBAL INDEX SCAN
            if ( sMyGraph->selectedIndex != NULL )
            {
                if ( sMyGraph->selectedIndex->indexPartitionType ==
                     QCM_NONE_PARTITIONED_INDEX )
                {
                    // GLOBAL INDEX SCAN
                    sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
                    sMyGraph->graph.flag |= QMG_PARALLEL_EXEC_FALSE;
                }
                else
                {
                    // LOCAL INDEX SCAN
                    sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
                    sMyGraph->graph.flag |= QMG_PARALLEL_EXEC_TRUE;
                }
            }
            else
            {
                // FULL SCAN or RID SCAN
                sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
                sMyGraph->graph.flag |= QMG_PARALLEL_EXEC_TRUE;
            }
        }
        else
        {
            // Parallel degree = 1
            sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
            sMyGraph->graph.flag |= QMG_PARALLEL_EXEC_FALSE;
        }
    }
    else
    {
        // Not SELECT statement
        sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
        sMyGraph->graph.flag |= QMG_PARALLEL_EXEC_FALSE;
    }

    /*
     * BUG-41134
     * child 가 parallel 수행 할 수 있는지 검사
     */
    if ((sMyGraph->graph.flag & QMG_PARALLEL_EXEC_MASK) == QMG_PARALLEL_EXEC_TRUE)
    {
        for (i = 0; i < sPartitionCount; i++)
        {
            if ((sMyGraph->graph.children[i].childGraph->flag & QMG_PARALLEL_EXEC_MASK) ==
                QMG_PARALLEL_EXEC_FALSE)
            {
                sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
                sMyGraph->graph.flag |= QMG_PARALLEL_EXEC_FALSE;
                break;
            }
        }
    }
    else
    {
        /* nothing to do */
    }

    //---------------------------------------------------
    // 공통 비용 정보의 설정
    //---------------------------------------------------

    if( sPartitionCount >= 1 )
    {
        // recordSize 설정
        // 아무 child graph에서 가져오면 된다.
        sMyGraph->graph.costInfo.recordSize =
            sMyGraph->graph.children[0].childGraph->costInfo.recordSize;

        sMyGraph->graph.costInfo.selectivity =
            sMyGraph->selectedMethod->methodSelectivity;

        // inputRecordCnt는 child로부터 구한다.
        sMyGraph->graph.costInfo.inputRecordCnt = 0;

        for( i = 0, sPartitionRef = sTableRef->partitionRef;
             (sPartitionRef != NULL) && (i < sPartitionCount);
             i++, sPartitionRef = sPartitionRef->next )
        {
            sMyGraph->graph.costInfo.inputRecordCnt +=
                sPartitionRef->statInfo->totalRecordCnt;
        }
        // 레코드 갯수는 최소 한개이상으로 유지한다.
        if( sMyGraph->graph.costInfo.inputRecordCnt < 1 )
        {
            sMyGraph->graph.costInfo.inputRecordCnt = 1;
        }
        else
        {
            // Nothing to do.
        }

        // output record count 설정
        sOutputRecordCnt =
            sMyGraph->graph.costInfo.selectivity *
            sMyGraph->graph.costInfo.inputRecordCnt;

        sMyGraph->graph.costInfo.outputRecordCnt =
            ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

        // myCost, totalCost 설정
        sMyGraph->graph.costInfo.myAccessCost =
            sMyGraph->selectedMethod->accessCost;

        sMyGraph->graph.costInfo.myDiskCost =
            sMyGraph->selectedMethod->diskCost;

        sMyGraph->graph.costInfo.myAllCost =
            sMyGraph->selectedMethod->totalCost;

        // 자신의 cost는 total과 같음. 모두 child의 cost를 포함하고 있다.
        sMyGraph->graph.costInfo.totalAccessCost =
            sMyGraph->graph.costInfo.myAccessCost;
        sMyGraph->graph.costInfo.totalDiskCost =
            sMyGraph->graph.costInfo.myDiskCost;
        sMyGraph->graph.costInfo.totalAllCost =
            sMyGraph->graph.costInfo.myAllCost;
    }
    else
    {
        // TODO1502: 비용정보를 잘 세팅해주어야 한다.
        // 누적할 정보가 없음
        // 선택된 파티션이 없다!
        sMyGraph->graph.costInfo.recordSize         = 1;
        sMyGraph->graph.costInfo.selectivity        = 0;
        sMyGraph->graph.costInfo.inputRecordCnt     = 1;
        sMyGraph->graph.costInfo.outputRecordCnt    = 1;
        sMyGraph->graph.costInfo.myAccessCost       = 1;
        sMyGraph->graph.costInfo.myDiskCost         = 0;
        sMyGraph->graph.costInfo.myAllCost          = 1;
        sMyGraph->graph.costInfo.totalAccessCost    = 1;
        sMyGraph->graph.costInfo.totalDiskCost      = 0;
        sMyGraph->graph.costInfo.totalAllCost       = 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::reviseAccessMethodsCost( qmgPARTITION   * aPartitionGraph,
                                       UInt             aPartitionCount )
{
    UInt             i, j;
    qmgSELT        * sChildGrpah;
    qmgChildren    * sChildList;
    qmoIdxCardInfo * sOrgMethod;
    qmsTableRef    * sTableRef;
    SDouble          sTotalInputCount;
    SDouble          sChildPercent;
    SDouble          sOutputCount;
    qmoAccessMethod  sMerge;

    IDU_FIT_POINT_FATAL( "qmgPartition::reviseAccessMethodsCost::__FT__" );

    if( aPartitionCount >= 1 )
    {
        sChildList  = aPartitionGraph->graph.children;
        sChildGrpah = (qmgSELT*)sChildList->childGraph;

        //--------------------------------------
        // Sum Input record count
        //--------------------------------------
        for( sTotalInputCount = 0, i = 0,
                 sChildList = aPartitionGraph->graph.children;
             sChildList != NULL;
             i++, sChildList = sChildList->next )
        {
            sTotalInputCount += sChildList->childGraph->costInfo.inputRecordCnt;
        }

        //--------------------------------------
        // Add child cost
        //--------------------------------------
        for( i = 0; i < aPartitionGraph->accessMethodCnt ; i++ )
        {
            idlOS::memset( &sMerge, 0, ID_SIZEOF(qmoAccessMethod));

            //--------------------------------------
            // Full scan
            //--------------------------------------
            if( aPartitionGraph->accessMethod[i].method == NULL )
            {
                for( sChildList = aPartitionGraph->graph.children;
                     sChildList != NULL;
                     sChildList = sChildList->next )
                {
                    sChildGrpah = (qmgSELT*) sChildList->childGraph;

                    // 자식 그래프의 레코드 갯수 비율대로 selectivity 를 반영한다.
                    sChildPercent = sChildList->childGraph->costInfo.inputRecordCnt /
                        sTotalInputCount;

                    sMerge.keyRangeSelectivity  += sChildGrpah->accessMethod[0].keyRangeSelectivity  * sChildPercent;
                    sMerge.keyFilterSelectivity += sChildGrpah->accessMethod[0].keyFilterSelectivity * sChildPercent;
                    sMerge.filterSelectivity    += sChildGrpah->accessMethod[0].filterSelectivity    * sChildPercent;
                    sMerge.methodSelectivity    += sChildGrpah->accessMethod[0].methodSelectivity    * sChildPercent;
                    sMerge.keyRangeCost         += sChildGrpah->accessMethod[0].keyRangeCost;
                    sMerge.keyFilterCost        += sChildGrpah->accessMethod[0].keyFilterCost;
                    sMerge.filterCost           += sChildGrpah->accessMethod[0].filterCost;
                    sMerge.accessCost           += sChildGrpah->accessMethod[0].accessCost;
                    sMerge.diskCost             += sChildGrpah->accessMethod[0].diskCost;
                    sMerge.totalCost            += sChildGrpah->accessMethod[0].totalCost;
                }

                sOrgMethod = aPartitionGraph->accessMethod[i].method;

                idlOS::memcpy( &(aPartitionGraph->accessMethod[i]),
                               &sMerge,
                               ID_SIZEOF(qmoAccessMethod) );

                aPartitionGraph->accessMethod[i].method         = sOrgMethod;
            }
            else if( aPartitionGraph->accessMethod[i].method->index->indexPartitionType
                     != QCM_NONE_PARTITIONED_INDEX )
            {
                //--------------------------------------
                // non Global Index scan
                //--------------------------------------
                for( sChildList = aPartitionGraph->graph.children;
                     sChildList != NULL;
                     sChildList = sChildList->next )
                {
                    sChildGrpah = (qmgSELT*) sChildList->childGraph;

                    for( j = 1; j < sChildGrpah->accessMethodCnt; j++ )
                    {
                        if( aPartitionGraph->accessMethod[i].method->index->indexId ==
                            sChildGrpah->accessMethod[j].method->index->indexId )
                        {
                            // 자식 그래프의 레코드 갯수 비율대로 selectivity 를 반영한다.
                            sChildPercent = sChildList->childGraph->costInfo.inputRecordCnt /
                                sTotalInputCount;

                            sMerge.keyRangeSelectivity  += sChildGrpah->accessMethod[j].keyRangeSelectivity  * sChildPercent;
                            sMerge.keyFilterSelectivity += sChildGrpah->accessMethod[j].keyFilterSelectivity * sChildPercent;
                            sMerge.filterSelectivity    += sChildGrpah->accessMethod[j].filterSelectivity    * sChildPercent;
                            sMerge.methodSelectivity    += sChildGrpah->accessMethod[j].methodSelectivity    * sChildPercent;
                            sMerge.keyRangeCost         += sChildGrpah->accessMethod[j].keyRangeCost;
                            sMerge.keyFilterCost        += sChildGrpah->accessMethod[j].keyFilterCost;
                            sMerge.filterCost           += sChildGrpah->accessMethod[j].filterCost;
                            sMerge.accessCost           += sChildGrpah->accessMethod[j].accessCost;
                            sMerge.diskCost             += sChildGrpah->accessMethod[j].diskCost;
                            sMerge.totalCost            += sChildGrpah->accessMethod[j].totalCost;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }   // end for j
                }   // end for child

                sOrgMethod = aPartitionGraph->accessMethod[i].method;

                idlOS::memcpy( &(aPartitionGraph->accessMethod[i]),
                               &sMerge,
                               ID_SIZEOF(qmoAccessMethod) );

                aPartitionGraph->accessMethod[i].method         = sOrgMethod;
            }
            else
            {
                //--------------------------------------
                // Global Index scan
                //--------------------------------------
                sTableRef = aPartitionGraph->graph.myFrom->tableRef;

                // 인덱스 테이블을 접근하는 비용을 추가한다.
                sMerge.totalCost = qmoCost::getTableRIDScanCost(
                    sTableRef->statInfo,
                    &sMerge.accessCost,
                    &sMerge.diskCost );

                // output count
                sOutputCount = sTotalInputCount * aPartitionGraph->accessMethod[i].methodSelectivity;

                aPartitionGraph->accessMethod[i].accessCost += sMerge.accessCost * sOutputCount;
                aPartitionGraph->accessMethod[i].diskCost   += sMerge.diskCost   * sOutputCount;
                aPartitionGraph->accessMethod[i].totalCost  += sMerge.totalCost  * sOutputCount;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC
qmgPartition::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgPartition로 부터 Plan을 생성한다.
 *               partition filter 추출을 위해 여기서 predicate를 재배치한다.
 * Implementation :
 *    - qmgPartition으로 부터 생성가능한 plan
 *
 *        1. 일반 scan인 경우
 *
 *                 [PSCN]
 *                   |
 *                 [SCAN]-[SCAN]-[SCAN]-...-[SCAN]
 *
 *        2. Parallel scan인 경우
 *
 *                 [PPCRD]
 *                   |  |
 *                   | [PRLQ]-[PRLQ]-...-[PRLQ]
 *                   |
 *                 [SCAN]-[SCAN]-[SCAN]-...-[SCAN]
 *
 ***********************************************************************/

    qmgPARTITION  * sMyGraph;
    qmnPlan       * sPCRD;
    qmnPlan       * sPRLQ;
    qmgChildren   * sChildren;
    qmnChildren   * sChildrenPRLQ;
    qmnChildren   * sCurPlanChildren;
    qmsTableRef   * sTableRef;
    UInt            sPartitionCount;
    UInt            sPRLQCount;
    qmoPCRDInfo     sPCRDInfo;
    idBool          sMakePPCRD;
    UInt            i;

    IDU_FIT_POINT_FATAL( "qmgPartition::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph     != NULL );

    sMyGraph  = (qmgPARTITION *) aGraph;
    sTableRef =  sMyGraph->graph.myFrom->tableRef;

    sPartitionCount  = 0;
    sPRLQCount       = 0;
    sChildrenPRLQ    = NULL;
    sCurPlanChildren = NULL;

    //---------------------------------------------------
    // Current CNF의 등록
    //---------------------------------------------------
    if( sMyGraph->graph.myCNF != NULL )
    {
        sMyGraph->graph.myQuerySet->SFWGH->crtPath->currentCNF =
            sMyGraph->graph.myCNF;
    }
    else
    {
        // Nothing To Do
    }

    if( aParent != NULL )
    {
        sMyGraph->graph.myPlan = aParent->myPlan;

        /* PROJ-1071 Parallel Query */
        aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
        aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);
    }
    else
    {
        aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
        aGraph->flag |= QMG_PARALLEL_IMPOSSIBLE_TRUE;
    }

    //---------------------------------------------------
    // Index Table Scan인 경우 RID SCAN으로 변경
    //---------------------------------------------------
    
    // PROJ-1624 global non-partitioned index
    // selected index가 global index라면 scan을 index table scan용으로 변경한다.
    // 원래는 makeGraph 중에 수행하는 것이 맞지만
    // bottom-up으로 최적화되면서 상위 graph에서도 graph를 변경시키므로
    // makePlan 직전에 수행한다.
    if ( sMyGraph->selectedIndex != NULL )
    {
        if ( sMyGraph->selectedIndex->indexPartitionType ==
             QCM_NONE_PARTITIONED_INDEX )
        {
            IDE_TEST( alterForceRidScan( aStatement,
                                         sMyGraph )
                      != IDE_SUCCESS );

            /* BUG-38024 */
            sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
            sMyGraph->graph.flag |= QMG_PARALLEL_EXEC_FALSE;

        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1071
    // 1. Parallel degree 가 1 보다 큰 경우
    // 2. Subquery 가 아니여야 함
    // 3. Parallel 실행 가능한 graph
    // 4. Parallel SCAN 이 허용되어야 함 (BUG-38410)
    if ( ( (sMyGraph->graph.flag & QMG_PARALLEL_EXEC_MASK)
           == QMG_PARALLEL_EXEC_TRUE ) &&
         ( aGraph->myQuerySet->parentTupleID
           == ID_USHORT_MAX ) &&
         ( (aGraph->flag & QMG_PARALLEL_IMPOSSIBLE_MASK)
           == QMG_PARALLEL_IMPOSSIBLE_FALSE ) &&
         ( (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK)
           == QMG_PLAN_EXEC_REPEATED_FALSE ) )
    {
        sMakePPCRD = ID_TRUE;
    }
    else
    {
        sMakePPCRD = ID_FALSE;
    }
    
    //----------------------------------------------
    // PCRD 혹은 PPCRD 의 생성
    //----------------------------------------------
    if (sMakePPCRD == ID_FALSE)
    {
        // Parallel query 가 아님
        IDE_TEST( qmoMultiNonPlan::initPCRD( aStatement,
                                             sMyGraph->graph.myQuerySet,
                                             sMyGraph->graph.myPlan,
                                             &sPCRD )
                  != IDE_SUCCESS );

    }
    else
    {
        // Parallel query
        IDE_TEST( qmoMultiNonPlan::initPPCRD( aStatement,
                                              sMyGraph->graph.myQuerySet,
                                              sMyGraph->graph.myPlan,
                                              &sPCRD )
                  != IDE_SUCCESS );
    }

    sMyGraph->graph.myPlan = sPCRD;

    // child plan의 생성
    sTableRef->partitionRef = NULL;

    for( sChildren = sMyGraph->graph.children;
         sChildren != NULL;
         sChildren = sChildren->next )
    {
        // plan을 생성하면서 partition reference를 merge한다.
        // DNF로 된 경우 각각의 plan이 서로 다른 partition reference를
        // 가지므로 merge해 주어야 함.
        IDE_DASSERT( sChildren->childGraph->type == QMG_SELECTION );

        IDE_TEST( qmoPartition::mergePartitionRef(
                      sTableRef,
                      ((qmgSELT*)sChildren->childGraph)->partitionRef )
                  != IDE_SUCCESS );

        /* PROJ-2464 hybrid partitioned table 지원 */
        IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sTableRef )
                  != IDE_SUCCESS );

        IDE_TEST( sChildren->childGraph->makePlan( aStatement ,
                                                   &sMyGraph->graph ,
                                                   sChildren->childGraph )
                  != IDE_SUCCESS);
        sPartitionCount++;
    }

    //----------------------------------------------
    // child PRLQ plan의 생성
    //----------------------------------------------
    sPCRD->childrenPRLQ = NULL;

    if (sMakePPCRD == ID_FALSE)
    {
        // Parallel disabled
        // Nothing to do.
    }
    else
    {
        // PRLQ 는 parallel degree 만큼 만들되
        // partition 수보다 많지 않아야 한다.
        for( i = 0, sChildren = sMyGraph->graph.children;
             (i < sTableRef->mParallelDegree) && (i < sPartitionCount)
                 && (sChildren != NULL);
             i++, sChildren = sChildren->next )
     
        {
            IDE_TEST( qmoParallelPlan::initPRLQ( aStatement,
                                                 sPCRD,
                                                 &sPRLQ )
                      != IDE_SUCCESS );

            IDE_TEST( qmoParallelPlan::makePRLQ( aStatement,
                                                 sMyGraph->graph.myQuerySet,
                                                 QMO_MAKEPRLQ_PARALLEL_TYPE_PARTITION,
                                                 sChildren->childGraph->myPlan,
                                                 sPRLQ )
                      != IDE_SUCCESS );

            // PRLQ 와 SCAN 은 직접적인 상-하위 관계를 가지지 않는다.
            sPRLQ->left = NULL;

            //----------------------------------------------
            // Children PRLQ 로 plan 을 연결
            //----------------------------------------------
            IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmnChildren),
                                                   (void**)&sChildrenPRLQ)
                     != IDE_SUCCESS);

            sChildrenPRLQ->childPlan = sPRLQ;
            sChildrenPRLQ->next = NULL;

            //----------------------------------------------
            // Children PRLQ Plan의 연결 관계를 구축
            //----------------------------------------------
            if ( sCurPlanChildren == NULL )
            {
                sPCRD->childrenPRLQ = sChildrenPRLQ;
            }
            else
            {
                sCurPlanChildren->next = sChildrenPRLQ;
            }
            sCurPlanChildren = sChildrenPRLQ;

            sPRLQCount++;
        }
        ((qmncPPCRD *)sPCRD)->PRLQCount = sPRLQCount;
    }

    sPCRDInfo.flag = QMO_PCRD_INFO_INITIALIZE;

    //-----------------------------------------------------
    // To Fix BUG-8747
    // Selection Graph에 Not Null Key Range를 생성하라는 Flag가
    // 있는 경우, Leaf Info에 그 정보를 설정한다.
    // - Selection Graph에서 Not Null Key Range 생성 Flag 적용되는 조건
    //   (1) indexable Min Max가 적용된 Selection Graph
    //   (2) Merge Join 하위의 Selection Graph
    //-----------------------------------------------------

    if( (sMyGraph->graph.flag & QMG_SELT_NOTNULL_KEYRANGE_MASK ) ==
        QMG_SELT_NOTNULL_KEYRANGE_TRUE )
    {
        sPCRDInfo.flag &= ~QMO_PCRD_INFO_NOTNULL_KEYRANGE_MASK;
        sPCRDInfo.flag |= QMO_PCRD_INFO_NOTNULL_KEYRANGE_TRUE;
    }
    else
    {
        // To Fix PR-10288
        // NOTNULL KEY RANGE가 아닌 경우로 반드시 설정해 주어야 함.
        sPCRDInfo.flag &= ~QMO_PCRD_INFO_NOTNULL_KEYRANGE_MASK;
        sPCRDInfo.flag |= QMO_PCRD_INFO_NOTNULL_KEYRANGE_FALSE;
    }
    
    //---------------------------------------------------
    // partition filter 추출용 predicate 처리
    //---------------------------------------------------
    
    // non-joinable predicate이 pushdown될 수 있다.
    if ( sMyGraph->graph.myPredicate != NULL )
    {
        // partition pruning을 위해 predicate를 복사
        
        // partition filter 추출용
        IDE_TEST(qmoPred::deepCopyPredicate(
                     QC_QMP_MEM(aStatement),
                     sMyGraph->graph.myPredicate,
                     &sMyGraph->partFilterPredicate )
                 != IDE_SUCCESS );
        
        // predicate를 재배치한다.(partition filter추출용)
        IDE_TEST( qmoPred::relocatePredicate4PartTable(
                      aStatement,
                      sMyGraph->partFilterPredicate,
                      sTableRef->tableInfo->partitionMethod,
                      & sMyGraph->graph.depInfo,
                      & sMyGraph->graph.myQuerySet->outerDepInfo,
                      & sMyGraph->partFilterPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // global index를 위한 predicate 최적화
    //---------------------------------------------------

    // non-joinable predicate이 pushdown될 수 있다.
    if ( sMyGraph->graph.myPredicate != NULL )
    {
        // predicate를 재배치한다.
        IDE_TEST( qmoPred::relocatePredicate(
                      aStatement,
                      sMyGraph->graph.myPredicate,
                      & sMyGraph->graph.depInfo,
                      & sMyGraph->graph.myQuerySet->outerDepInfo,
                      sTableRef->statInfo,
                      & sMyGraph->graph.myPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    sPCRDInfo.predicate              = sMyGraph->graph.myPredicate;
    sPCRDInfo.constantPredicate      = sMyGraph->graph.constantPredicate;
    sPCRDInfo.index                  = sMyGraph->selectedIndex;
    sPCRDInfo.preservedOrder         = sMyGraph->graph.preservedOrder;
    sPCRDInfo.forceIndexScan         = sMyGraph->forceIndexScan;
    sPCRDInfo.nnfFilter              = sMyGraph->graph.nnfFilter;
    sPCRDInfo.partKeyRange           = sMyGraph->partKeyRange;
    sPCRDInfo.partFilterPredicate    = sMyGraph->partFilterPredicate;
    sPCRDInfo.selectedPartitionCount = sPartitionCount;
    sPCRDInfo.childrenGraph          = sMyGraph->graph.children;

    // BUG-48800
    if ( sMyGraph->mPrePruningPartRef != NULL )
    {
        sPCRDInfo.mPrePruningPartHandle  = sMyGraph->mPrePruningPartRef->partitionHandle;
        sPCRDInfo.mPrePruningPartSCN     = sMyGraph->mPrePruningPartRef->partitionSCN;
    }
    else
    {
        sPCRDInfo.mPrePruningPartHandle  = NULL; 
        sPCRDInfo.mPrePruningPartSCN     = SM_SCN_INIT; 
    }   

    // make PCRD or PPCRD
    if (sMakePPCRD == ID_FALSE)
    {
        IDE_TEST( qmoMultiNonPlan::makePCRD( aStatement,
                                             sMyGraph->graph.myQuerySet,
                                             sMyGraph->graph.myFrom,
                                             &sPCRDInfo,
                                             sPCRD )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmoMultiNonPlan::makePPCRD( aStatement,
                                              sMyGraph->graph.myQuerySet,
                                              sMyGraph->graph.myFrom,
                                              &sPCRDInfo,
                                              sPCRD )
                  != IDE_SUCCESS );
    }

    sMyGraph->graph.myPlan = sPCRD;

    qmg::setPlanInfo( aStatement, sPCRD, &(sMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::alterForceRidScan( qcStatement  * aStatement,
                                 qmgPARTITION * aGraph )
{
    qmgChildren  * sChild;

    IDU_FIT_POINT_FATAL( "qmgPartition::alterForceRidScan::__FT__" );

    for( sChild = aGraph->graph.children;
         sChild != NULL;
         sChild = sChild->next )
    {
        IDE_TEST( qmgSelection::alterForceRidScan( aStatement,
                                                   sChild->childGraph )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::alterSelectedIndex( qcStatement  * aStatement,
                                  qmgPARTITION * aGraph,
                                  qcmIndex     * aNewSelectedIndex )
{
    qmgChildren  * sChild;
    qmgSELT      * sChildSeltGraph;

    IDU_FIT_POINT_FATAL( "qmgPartition::alterSelectedIndex::__FT__" );

    // PROJ-1624 Global Index
    // global index는 partitioned table에만 생성되어 있다.
    if ( aNewSelectedIndex->indexPartitionType ==
         QCM_NONE_PARTITIONED_INDEX )
    {
        aGraph->selectedIndex = aNewSelectedIndex;

        // To fix BUG-12742
        // 상위 graph에 의해 결정된 경우
        // index를 없앨 수 없다.
        aGraph->forceIndexScan = ID_TRUE;
    }
    else
    {   
        for( sChild = aGraph->graph.children;
             sChild != NULL;
             sChild = sChild->next )
        {
            sChildSeltGraph = (qmgSELT*)sChild->childGraph;
            
            IDE_TEST( qmgSelection::alterSelectedIndex(
                          aStatement,
                          sChildSeltGraph,
                          aNewSelectedIndex )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::printGraph( qcStatement  * aStatement,
                          qmgGraph     * aGraph,
                          ULong          aDepth,
                          iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *    Graph를 구성하는 공통 정보를 출력한다.
 *    TODO1502: partition graph의 고유 정보를 출력해줘야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    qmoPredicate * sMorePred;
    qmgChildren  * sChildren;
    qmgPARTITION * sMyGraph;
    UInt  i;

    IDU_FIT_POINT_FATAL( "qmgPartition::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

    sMyGraph = (qmgPARTITION*)aGraph;

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
    // BUG-38192
    // Access method 별 정보 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Access Method Information ==" );

    for ( i = 0; i < sMyGraph->accessMethodCnt; i++ )
    {
        IDE_TEST( qmgSelection::printAccessMethod(
                      aStatement,
                      &sMyGraph->accessMethod[i],
                      aDepth,
                      aString )
                  != IDE_SUCCESS );
    }

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
    // Child Graph 고유 정보의 출력
    //-----------------------------------

    // BUG-38192

    if ( QCU_TRCLOG_DISPLAY_CHILDREN == 1 )
    {
        if ( aGraph->children != NULL )
        {
            for( sChildren = aGraph->children;
                 sChildren != NULL;
                 sChildren = sChildren->next )
            {
                IDE_TEST( sChildren->childGraph->printGraph( aStatement,
                                                             sChildren->childGraph,
                                                             aDepth + 1,
                                                             aString )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::extractPartKeyRange(
    qcStatement        * aStatement,
    qmsQuerySet        * aQuerySet,
    qmoPredicate      ** aPartKeyPred,
    UInt                 aPartKeyCount,
    mtcColumn          * aPartKeyColumns,
    UInt               * aPartKeyColsFlag,
    qtcNode           ** aPartKeyRangeOrg,
    smiRange          ** aPartKeyRange )
{
    qtcNode  * sKey;
    qtcNode  * sDNFKey;
    smiRange * sRange;
    smiRange * sRangeArea;
    UInt       sEstimateSize;
    UInt       sEstDNFCnt;
    UInt       sCompareType;

    IDU_FIT_POINT_FATAL( "qmgPartition::extractPartKeyRange::__FT__" );

    sKey = NULL;
    sDNFKey = NULL;
    sRange = NULL;
    sRangeArea = NULL;
    sEstimateSize = 0;
    sEstDNFCnt = 0;

    *aPartKeyRangeOrg = NULL;
    *aPartKeyRange = NULL;

    if( *aPartKeyPred == NULL )
    {
        // Nothing to do.
    }
    else
    {
        // qtcNode로의 변환
        IDE_TEST( qmoPred::linkPredicate( aStatement ,
                                          *aPartKeyPred ,
                                          &sKey ) != IDE_SUCCESS );

        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           sKey ) != IDE_SUCCESS );

        if( sKey != NULL )
        {
            // To Fix PR-9481
            // CNF로 구성된 Key Range Predicate을 DNF normalize할 경우
            // 엄청나게 많은 Node로 변환될 수 있다.
            // 이를 검사하여 지나치게 많은 변화가 필요한 경우에는
            // Default Key Range만을 생성하게 한다.
            IDE_TEST( qmoNormalForm::estimateDNF( sKey, &sEstDNFCnt )
                      != IDE_SUCCESS );

            if ( sEstDNFCnt > QCG_GET_SESSION_NORMALFORM_MAXIMUM( aStatement ) )
            {
                sDNFKey = NULL;
            }
            else
            {
                // DNF형태로 변환
                IDE_TEST( qmoNormalForm::normalizeDNF( aStatement ,
                                                       sKey ,
                                                       &sDNFKey )
                          != IDE_SUCCESS );
            }

            // environment의 기록
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_NORMAL_FORM_MAXIMUM );
        }
        else
        {
            //nothing to do
        }

        if( sDNFKey != NULL )
        {
            IDE_DASSERT( ( (*aPartKeyPred)->flag  & QMO_PRED_VALUE_MASK )
                         == QMO_PRED_FIXED );

            // KeyRange를 위해 DNF로 부터 size estimation을 한다.
            IDE_TEST( qmoKeyRange::estimateKeyRange( QC_SHARED_TMPLATE(aStatement) ,
                                                     sDNFKey ,
                                                     &sEstimateSize )
                      != IDE_SUCCESS );

            IDE_TEST( QC_QMP_MEM(aStatement)->cralloc( sEstimateSize ,
                                                       (void**)&sRangeArea )
                      != IDE_SUCCESS );

            // partition keyrange를 생성한다.

            // PROJ-1872
            // Partition Key는 Partition Table 결정할때 쓰임
            // (Partition을 나누는 기준값)과 (Predicate의 value)와
            // 비교하므로 MTD Value간의 compare를 사용함
            // ex) i1 < 10 => Partition P1
            //     i1 < 20 => Partition P2
            //     i1 < 30 => Partition P3
            // SELECT ... FROM ... WHERE i1 = 5
            // P1, P2, P3에서 P1 Partition을 선택하기 위해
            // Partition Key Range를 사용함
            sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;

            IDE_TEST(
                qmoKeyRange::makePartKeyRange( QC_SHARED_TMPLATE(aStatement),
                                               sDNFKey,
                                               aPartKeyCount,
                                               aPartKeyColumns,
                                               aPartKeyColsFlag,
                                               sCompareType,
                                               sRangeArea,
                                               sEstimateSize,
                                               &sRange )
                != IDE_SUCCESS );

            *aPartKeyRangeOrg = sKey;
            *aPartKeyRange = sRange;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::copyPARTITIONAndAlterSelectedIndex( qcStatement   * aStatement,
                                                  qmgPARTITION  * aSource,
                                                  qmgPARTITION ** aTarget,
                                                  qcmIndex      * aNewSelectedIndex,
                                                  UInt            aWhichOne )
{
/****************************************************************************
 *
 * Description : qmgJoin에서 ANTI가 선택된 경우
 *               하위 SELT graph를 복사해야 한다.
 *               하지만 복사 과정에서 predicate의 복사 유무나,
 *               복사한 후 어떤 graph의 access method를 바꿀지에 대한 처리나,
 *               scanDecisionFactor의 처리 등 복잡한 요소를 처리하기 위해서
 *               복사 알고리즘을 qmgSelection에서 구현한다.
 *               SELT graph를 복사해야할 필요가 있을 경우
 *               반드시 qmgSelection에서 구현해서 호출하도록 한다.
 *
 *               aWhich = 0 : target의 access method를 바꾼다.
 *               aWhich = 1 : source의 access method를 바꾼다.
 *
 * Implementation : aSource를 aTarget에 복사한 후,
 *                  selectedIndex가 바뀔 graph에 대해서는
 *                  sdf를 무효화 한다.
 *
 *****************************************************************************/

    qmgPARTITION * sTarget;
    qmgChildren  * sChild;
    qmgGraph     * sNewChild;
    UInt           sChildCnt = 0;
    UInt           i;

    IDU_FIT_POINT_FATAL( "qmgPartition::copyPARTITIONAndAlterSelectedIndex::__FT__" );

    for( sChild = aSource->graph.children;
         sChild != NULL;
         sChild = sChild->next )
    {
        sChildCnt++;
    }
    
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPARTITION ), (void**) &sTarget )
              != IDE_SUCCESS );

    idlOS::memcpy( sTarget, aSource, ID_SIZEOF( qmgPARTITION ) );
    
    // graph->children구조체의 메모리 할당.
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  ID_SIZEOF(qmgChildren) * sChildCnt,
                  (void**) &sTarget->graph.children )
              != IDE_SUCCESS );
    
    for( sChild = aSource->graph.children, i = 0;
         sChild != NULL;
         sChild = sChild->next, i++ )
    {
        IDE_TEST( qmgSelection::copySELTAndAlterSelectedIndex(
                      aStatement,
                      (qmgSELT*)sChild->childGraph,
                      (qmgSELT**)&sNewChild,
                      aNewSelectedIndex,
                      aWhichOne )
                  != IDE_SUCCESS );

        sTarget->graph.children[i].childGraph = sNewChild;

        if ( i + 1 < sChildCnt )
        {
            sTarget->graph.children[i].next = &(sTarget->graph.children[i + 1]);
        }
        else
        {
            sTarget->graph.children[i].next = NULL;
        }        
    }
    
    if( aWhichOne == 0 )
    {
        IDE_TEST( alterSelectedIndex( aStatement,
                                      sTarget,
                                      aNewSelectedIndex )
                  != IDE_SUCCESS );
    }
    else if( aWhichOne == 1 )
    {
        IDE_TEST( alterSelectedIndex( aStatement,
                                      aSource,
                                      aNewSelectedIndex )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    *aTarget = sTarget;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::setJoinPushDownPredicate( qcStatement   * aStatement,
                                        qmgPARTITION  * aGraph,
                                        qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                push-down join predicate를 partition graph에 연결한다.
 *
 *  Implementation :
 *                 1) partition graph에 딸린 child graph(selection)에
 *                    join predicate를 복사하여 연결시킨다.
 *                 2) partition graph에는 join predicate를 직접 연결시킨다.
 *                 3) parameter로 받은 join predicate를 null로 세팅.
 *
 ***********************************************************************/

    qmgChildren  * sChild;
    qmgSELT      * sChildSeltGraph;
    qmoPredicate * sCopiedPred = NULL;
    qmoPredicate * sJoinPredicate;

    IDU_FIT_POINT_FATAL( "qmgPartition::setJoinPushDownPredicate::__FT__" );

    // partition graph의 child graph에 복사하여 연결.
    for( sChild = aGraph->graph.children;
         sChild != NULL;
         sChild = sChild->next )
    {
        sChildSeltGraph = (qmgSELT*)sChild->childGraph;

        IDE_TEST(qmoPred::copyPredicate4Partition(
                     aStatement,
                     *aPredicate,
                     &sCopiedPred,
                     aGraph->graph.myFrom->tableRef->table,
                     sChildSeltGraph->partitionRef->table,
                     ID_TRUE )
                 != IDE_SUCCESS );

        IDE_TEST( qmgSelection::setJoinPushDownPredicate(
                      sChildSeltGraph,
                      &sCopiedPred )
                  != IDE_SUCCESS );
    }

    // partition graph에 연결.
    for( sJoinPredicate       = *aPredicate;
         sJoinPredicate->next != NULL;
         sJoinPredicate       = sJoinPredicate->next ) ;

    sJoinPredicate->next = aGraph->graph.myPredicate;
    aGraph->graph.myPredicate  = *aPredicate;
    *aPredicate       = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::setNonJoinPushDownPredicate( qcStatement   * aStatement,
                                           qmgPARTITION  * aGraph,
                                           qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                push-down join predicate를 partition graph에 연결한다.
 *
 *  Implementation :
 *                 1) partition graph에 딸린 child graph(selection)에
 *                    join predicate를 복사하여 연결시킨다.
 *                 2) partition graph에는 join predicate를 직접 연결시킨다.
 *                 3) parameter로 받은 join predicate를 null로 세팅.
 *
 ***********************************************************************/

    qmgChildren  * sChild;
    qmgSELT      * sChildSeltGraph;
    qmoPredicate * sCopiedPred;
    qmoPredicate * sJoinPredicate;
    qmoPredicate * sPredicate;
    qmoPredicate * sNextPredicate;

    IDU_FIT_POINT_FATAL( "qmgPartition::setNonJoinPushDownPredicate::__FT__" );

    // partition graph의 child graph에 복사하여 연결.
    for( sChild = aGraph->graph.children;
         sChild != NULL;
         sChild = sChild->next )
    {
        sChildSeltGraph = (qmgSELT*)sChild->childGraph;

        IDE_TEST(qmoPred::copyPredicate4Partition(
                     aStatement,
                     *aPredicate,
                     &sCopiedPred,
                     aGraph->graph.myFrom->tableRef->table,
                     sChildSeltGraph->partitionRef->table,
                     ID_TRUE )
                 != IDE_SUCCESS );

        IDE_TEST( qmgSelection::setNonJoinPushDownPredicate(
                      sChildSeltGraph,
                      &sCopiedPred )
                  != IDE_SUCCESS );
    }


    sJoinPredicate = *aPredicate;

    if( aGraph->graph.myPredicate == NULL )
    {
        // selection graph에 predicate이 하나도 없는 경우로,
        // 첫번째 non-joinable predicate 연결하고,
        // 연결된 predicate의 next 연결을 끊는다.
        aGraph->graph.myPredicate = sJoinPredicate;
        sJoinPredicate = sJoinPredicate->next;
        aGraph->graph.myPredicate->next = NULL;
        sPredicate = aGraph->graph.myPredicate;
    }
    else
    {
        // selection graph에 predicate이 있는 경우로,
        // Index Nested Loop Joinable Predicate들은 제외한다.
        // non-joinable predicate은 keyFilter or filter로 처리되어야 하므로,
        // keyRange로 추출될 Index Nested Loop Join Predicate과는 별도로 연결.

        for( sPredicate = aGraph->graph.myPredicate;
             sPredicate->next != NULL;
             sPredicate = sPredicate->next )
        {
            if( ( sPredicate->flag & QMO_PRED_INDEX_NESTED_JOINABLE_MASK )
                != QMO_PRED_INDEX_NESTED_JOINABLE_TRUE )
            {
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        // 위 처리 과정에서, selection graph에 joinable predicate만 있는 경우,
        // 마지막 joinable predicate의 next에 non-joinable predicate을 연결,
        // 연결된 non-joinable predicate의 next 연결을 끊는다.
        if( ( sPredicate->flag & QMO_PRED_INDEX_NESTED_JOINABLE_MASK )
            == QMO_PRED_INDEX_NESTED_JOINABLE_TRUE )
        {
            sPredicate->next = sJoinPredicate;
            sJoinPredicate = sJoinPredicate->next;
            sPredicate = sPredicate->next;
            sPredicate->next = NULL;
        }
        else
        {
            // Nothing To Do
        }
    }

    // Index Nested Loop Joinable Predicate을 제외한
    // selection graph predicate 리스트에서 non-joinable predicate과
    // columnID가 같은 predicate에 non-joinable predicate을 연결하고,
    // 연결된 non-joinable의 next의 연결을 끊는다.

    while( sJoinPredicate != NULL )
    {
        // joinable predicate을 제외한 predicate중에서 join predicate과
        // 같은 컬럼이 존재하는지를 검사.
        // sPredicate : index joinable predicate을 제외한
        //              첫번째 predicate을 가리킨다.

        // 컬럼별로 연결관계 만들기
        // 동일 컬럼이 있는 경우, 동일 컬럼의 마지막 predicate.more에
        // 동일 컬럼이 없는 경우, sPredicate의 마지막 predicate.next에
        // (1) 새로운 predicate(sJoinPredicate)을 연결하고,
        // (2) sJoinPredicate = sJoinPredicate->next
        // (3) 연결된 predicate의 next 연결관계를 끊음.

        sNextPredicate = sJoinPredicate->next;

        IDE_TEST( qmoPred::linkPred4ColumnID( sPredicate,
                                              sJoinPredicate ) != IDE_SUCCESS );
        sJoinPredicate = sNextPredicate;
    }

    *aPredicate = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgPartition::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Preserved Order의 direction을 결정한다.
 *                direction이 NOT_DEFINED 일 경우에만 호출하여야 한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    qmgPARTITION      * sMyGraph;
    qmgPreservedOrder * sPreservedOrder;
    UInt              * sKeyColsFlag;
    UInt                sKeyColCount;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmgPartition::finalizePreservedOrder::__FT__" );

    sMyGraph = (qmgPARTITION*) aGraph;

    // Preserved order가 존재하는 경우는 global index scan뿐이다.
    IDE_DASSERT( sMyGraph->selectedIndex->indexPartitionType ==
                 QCM_NONE_PARTITIONED_INDEX );
    
    sKeyColCount = sMyGraph->selectedIndex->keyColCount;
    sKeyColsFlag = sMyGraph->selectedIndex->keyColsFlag;
    
    // Selected index의 order로 direction을 결정한다.
    for ( sPreservedOrder = aGraph->preservedOrder,
              i = 0;
          sPreservedOrder != NULL &&
              i < sKeyColCount;
          sPreservedOrder = sPreservedOrder->next,
              i++ )
    {
        // Direction copy
        if ( ( sKeyColsFlag[i] & SMI_COLUMN_ORDER_MASK ) ==
             SMI_COLUMN_ORDER_ASCENDING )
        {
            sPreservedOrder->direction = QMG_DIRECTION_ASC;
        }
        else
        {
            sPreservedOrder->direction = QMG_DIRECTION_DESC;
        }
    }

    return IDE_SUCCESS;
}
