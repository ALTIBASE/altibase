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
 * $Id: qmgSelection.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Selection Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcg.h>
#include <qmgSelection.h>
#include <qmgProjection.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoParallelPlan.h>
#include <qmo.h>
#include <qmoCost.h>
#include <qmoSelectivity.h>
#include <qmoPushPred.h>
#include <qmoRownumPredToLimit.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmgSetRecursive.h>

IDE_RC
qmgSelection::init( qcStatement * aStatement,
                    qmsQuerySet * aQuerySet,
                    qmsFrom     * aFrom,
                    qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgSelection Graph의 초기화
 *
 * Implementation :
 *    (1) qmgSelection을 위한 공간 할당 받음
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgSELT       * sMyGraph;
    qmsTableRef   * sTableRef;

    IDU_FIT_POINT_FATAL( "qmgSelection::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );

    //---------------------------------------------------
    // Selection Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgSelection을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgSELT ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );


    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_SELECTION;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aFrom->depInfo );
    sMyGraph->graph.myFrom = aFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgSelection::optimize;
    sMyGraph->graph.makePlan = qmgSelection::makePlan;
    sMyGraph->graph.printGraph = qmgSelection::printGraph;

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

    sMyGraph->graph.flag &= ~QMG_ROWNUM_PUSHED_MASK;
    sMyGraph->graph.flag |= QMG_ROWNUM_PUSHED_TRUE;

    //---------------------------------------------------
    // Selection 고유 정보의 초기화
    //---------------------------------------------------

    sMyGraph->limit = NULL;
    sMyGraph->selectedIndex = NULL;
    sMyGraph->selectedMethod = NULL;
    sMyGraph->accessMethodCnt = 0;
    sMyGraph->accessMethod = NULL;
    sMyGraph->sdf = NULL;

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition에 대한 selection graph임을 명시
    sMyGraph->partitionRef = NULL;

    sMyGraph->mFlag = QMG_SELT_FLAG_CLEAR;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::init( qcStatement     * aStatement,
                    qmsQuerySet     * aQuerySet,
                    qmsFrom         * aFrom,
                    qmsPartitionRef * aPartitionRef,
                    qmgGraph       ** aGraph )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *               qmgSelection Graph의 초기화
 *
 * Implementation :
 *    (1) qmgSelection을 위한 공간 할당 받음
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) reference partition을 세팅
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgSELT       * sMyGraph;
    qmsTableRef   * sTableRef;

    IDU_FIT_POINT_FATAL( "qmgSelection::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aPartitionRef != NULL );

    //---------------------------------------------------
    // Selection Graph를 위한 기본 초기화
    //---------------------------------------------------
    sTableRef = aFrom->tableRef;

    // qmgSelection을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgSELT ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );


    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_SELECTION;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aFrom->depInfo );

    // partition의 dependency로 변경.
    qtc::dependencyChange( sTableRef->table,
                           aPartitionRef->table,
                           &sMyGraph->graph.depInfo,
                           &sMyGraph->graph.depInfo );

    sMyGraph->graph.myFrom = aFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgSelection::optimizePartition;
    sMyGraph->graph.makePlan = qmgSelection::makePlanPartition;
    sMyGraph->graph.printGraph = qmgSelection::printGraphPartition;

    // Disk/Memory 정보 설정
    if ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aPartitionRef->table].lflag
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
    // Selection 고유 정보의 초기화
    //---------------------------------------------------

    sMyGraph->limit = NULL;
    sMyGraph->selectedIndex = NULL;
    sMyGraph->selectedMethod = NULL;
    sMyGraph->accessMethodCnt = 0;
    sMyGraph->accessMethod = NULL;
    sMyGraph->sdf = NULL;

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition에 대한 selection graph임을 명시
    sMyGraph->partitionRef = aPartitionRef;

    /* BUG-44659 미사용 Partition의 통계 정보를 출력하다가,
     *           Graph의 Partition/Column/Index Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  qmgSELT에서 Partition Name을 보관하도록 수정합니다.
     */
    (void)idlOS::memcpy( sMyGraph->partitionName,
                         aPartitionRef->partitionInfo->name,
                         idlOS::strlen( aPartitionRef->partitionInfo->name ) + 1 );

    sMyGraph->graph.flag &= ~QMG_SELT_PARTITION_MASK;
    sMyGraph->graph.flag |= QMG_SELT_PARTITION_TRUE;

    sMyGraph->mFlag = QMG_SELT_FLAG_CLEAR;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgSelection::optimize(qcStatement * aStatement, qmgGraph * aGraph)
{
/***********************************************************************
 *
 * Description : qmgSelection의 최적화
 *
 * Implementation :
 *    (1) View Graph 생성
 *        - BUG-9736 수정 시에 View Graph 생성과정을 init 과정으로 옮겼으나
 *          init 과정에서 View Graph를 생성할 경우, Push Selection을 적용할 수
 *          없다. BUG-9736은 left outer, full outer join의 subquery graph 생성
 *          위치 변경으로 해결해야 함
 *    (2) bad transitive predicate을 제거
 *    (3) Subquery Graph 생성
 *    (4) 공통 비용 정보 설정( recordSize, inputRecordCnt, pageCnt )
 *    (5) Predicate 재배치 및 개별 Predicate의 selectivity 계산
 *    (6) 전체 selectivity 계산 및 공통 비용 정보의 selectivity에 저장
 *    (7) Access Method 선택
 *    (8) Preserved Order 설정
 *    (9) 공통 비용 정보 설정( outputRecordCnt, myCost, totalCost )
 *
 ***********************************************************************/

    UInt              sColumnCnt;
    UInt              sIndexCnt;
    UInt              sSelectedScanHint;
    UInt              sParallelDegree;

    SDouble           sOutputRecordCnt;
    SDouble           sRecordSize;

    UInt              i;

    qmgSELT         * sMyGraph;
    qmsTarget       * sTarget;
    qmsTableRef     * sTableRef;
    qcmColumn       * sColumns;
    mtcColumn       * sTargetColumn;

    IDU_FIT_POINT_FATAL( "qmgSelection::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgSELT*) aGraph;
    sTableRef = sMyGraph->graph.myFrom->tableRef;
    sRecordSize = 0;

    //---------------------------------------------------
    // View Graph의 생성 및 통계 정보 구축
    //   ( 일반 Table의 통계 정보는 Validation 과정에서 설정됨 )
    //---------------------------------------------------

    // PROJ-2582 recursive with
    // recursive view를 view 처럼 처리 하기 위해 view 복사..
    if ( sTableRef->recursiveView != NULL )
    {
        sTableRef->view = sTableRef->recursiveView;
    }
    else
    {
        // Nothing To Do
    }

    if( sTableRef->view != NULL )
    {
        // View Graph의 생성
        IDE_TEST( makeViewGraph( aStatement, & sMyGraph->graph )
                  != IDE_SUCCESS );

        // 통계 정보의 구축
        IDE_TEST( qmoStat::getStatInfo4View( aStatement,
                                             & sMyGraph->graph,
                                             & sTableRef->statInfo )
                  != IDE_SUCCESS );

        // fix BUG-11209
        // selection graph는 하위에 view가 올 수 있으므로
        // view가 있을 때는 view의 projection graph의 타입으로 flag를 보정한다.
        sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
        sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MASK &
            sMyGraph->graph.left->flag;
    }
    else
    {
        //---------------------------------------------------
        // ::init에서 이미 flag 초기화가 이루어 졌으므로, View가 아닌경우에는
        // 아무처리도 하지 않으면 된다.
        // PROJ-2582 recursive with
        // recursive view의 통계 정보의 구축
        // validate 단계에서 view는 statInfo 생성 하지 않기 때문에
        // right의 recursive view는 tableRef의 statInfo가 없다.
        // right child는 left의 VMTR로 통계정보를 구축
        //---------------------------------------------------

        if ( ( sTableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
             == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
        {
            // right query의 recursive view에 임시로 연결한 left의 graph연결
            sMyGraph->graph.left = aStatement->myPlan->graph;

            // 다시 right query의 recursive view graph를 반환
            aStatement->myPlan->graph = & sMyGraph->graph;
            
            // 통계 정보의 구축
            IDE_TEST( qmoStat::getStatInfo4View( aStatement,
                                                 & sMyGraph->graph,
                                                 & sTableRef->statInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothing to do
        }
    }

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
    
    if ( ( sMyGraph->graph.myPredicate != NULL ) &&
         ( sTableRef->view == NULL ) )
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
        // To Fix PR-11461, PR-11460
        // 일반 Table인 경우에만 Subquery KeyRange 최적화를 적용할 수 있다.
        if ( sTableRef->view == NULL )
        {
            // 일반 테이블인 경우
            IDE_TEST(
                qmoPred::optimizeSubqueries( aStatement,
                                             sMyGraph->graph.myPredicate,
                                             ID_TRUE ) // Use KeyRange Tip
                != IDE_SUCCESS );
        }
        else
        {
            // View인 경우
            IDE_TEST(
                qmoPred::optimizeSubqueries( aStatement,
                                             sMyGraph->graph.myPredicate,
                                             ID_FALSE ) // No KeyRange Tip
                != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------
    // 공통 비용 정보의 설정
    //---------------------------------------------------
    
    // recordSize 설정
    if ( sTableRef->view == NULL )
    {
        // 일반 Table인 경우
        sColumns = sTableRef->tableInfo->columns;
        sColumnCnt = sTableRef->tableInfo->columnCount;

        for ( i = 0; i < sColumnCnt; i++ )
        {
            sRecordSize += sColumns[i].basicInfo->column.size;
        }
    }
    else
    {
        // View인 경우,
        // To Fix BUG-8241
        for ( sTarget = ((qmgPROJ*)(sMyGraph->graph.left))->target;
              sTarget != NULL;
              sTarget = sTarget->next )
        {
            sTargetColumn = QTC_STMT_COLUMN( aStatement, sTarget->targetColumn );

            sRecordSize += sTargetColumn->column.size;
        }
    }
    // BUG-36463 sRecordSize 는 0이 되어서는 안된다.
    sRecordSize = IDL_MAX( sRecordSize, 1 );

    sMyGraph->graph.costInfo.recordSize = sRecordSize;

    // inputRecordCnt 설정
    sMyGraph->graph.costInfo.inputRecordCnt =
        sTableRef->statInfo->totalRecordCnt;

    //---------------------------------------------------
    // Predicate의 재배치 및 개별 Predicate의 Selectivity 계산
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST(
            qmoPred::relocatePredicate(
                aStatement,
                sMyGraph->graph.myPredicate,
                & sMyGraph->graph.depInfo,
                & sMyGraph->graph.myQuerySet->outerDepInfo,
                sMyGraph->graph.myFrom->tableRef->statInfo,
                & sMyGraph->graph.myPredicate )
            != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    /* BUG-43006 FixedTable Indexing Filter
     * Fixed Table에 대해optimizer_formance_view 프로퍼티가 꺼져있다면 index가 없다고
     * 설정해줘야한다.
     */
    if ( ( ( sTableRef->tableInfo->tableType == QCM_FIXED_TABLE ) ||
           ( sTableRef->tableInfo->tableType == QCM_DUMP_TABLE ) ||
           ( sTableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW ) ) &&
         ( QCG_GET_SESSION_OPTIMIZER_PERFORMANCE_VIEW(aStatement) == 0 ) )
    {
        sIndexCnt = 0;
    }
    else
    {
        sIndexCnt = sTableRef->tableInfo->indexCount;
    }

    /* PROJ-2402 Parallel Table Scan */
    setParallelScanFlag(aStatement, aGraph);

    //---------------------------------------------------------------
    // accessMethod 선택
    // 
    // rid predicate 이 있는 경우 무조건 rid scan 을 시도한다.
    // rid predicate 이 있더라도  rid scan 을 할수 없는 경우도 있다.
    // 이 경우에도 index scan 이 되지 않고 full scan 을 하게 된다.
    //---------------------------------------------------------------

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
        sMyGraph->forceIndexScan  = ID_FALSE;
        sMyGraph->forceRidScan    = ID_FALSE;

        sSelectedScanHint = QMG_NOT_USED_SCAN_HINT;
    }
    else
    {
        // TASK-6699 TPC-H 성능 개선
        // Cost 계산에 필요한 ParallelDegree를 계산한다.
        // 단 ParallelDegree 높인다고해서 비례해서 성능이 증가하지 않으므로 최대값은 4
        if ( (sMyGraph->mFlag & QMG_SELT_PARALLEL_SCAN_MASK) == QMG_SELT_PARALLEL_SCAN_TRUE )
        {
            if ( sMyGraph->graph.myFrom->tableRef->mParallelDegree > 1 )
            {
                sParallelDegree = IDL_MIN( sMyGraph->graph.myFrom->tableRef->mParallelDegree, 4 );
            }
            else
            {
                sParallelDegree = 1;
            }
        }
        else
        {
            sParallelDegree = 1;
        }

        sMyGraph->accessMethodCnt = sIndexCnt + 1;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmoAccessMethod) *
                                               (sIndexCnt + 1),
                                               (void**)&sMyGraph->accessMethod)
                 != IDE_SUCCESS );

        IDE_TEST( getBestAccessMethod( aStatement,
                                       & sMyGraph->graph,
                                       sTableRef->statInfo,
                                       sMyGraph->graph.myPredicate,
                                       sMyGraph->accessMethod,
                                       & sMyGraph->selectedMethod,
                                       & sMyGraph->accessMethodCnt,
                                       & sMyGraph->selectedIndex,
                                       & sSelectedScanHint,
                                       sParallelDegree,
                                       0 )
                  != IDE_SUCCESS);

        // TASK-6699 TPC-H 성능 개선
        // Index 가 선택되면 Parallel 수행이 안됨
        if (sMyGraph->selectedIndex != NULL)
        {
            sMyGraph->mFlag &= ~QMG_SELT_PARALLEL_SCAN_MASK;
            sMyGraph->mFlag |= QMG_SELT_PARALLEL_SCAN_FALSE;
        }
        else
        {
            // nothing to do
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
            sMyGraph->forceIndexScan = ID_FALSE;
        }

        sMyGraph->forceRidScan = ID_FALSE;
    }

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    // preserved order 초기화
    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NOT_DEFINED;

    if ( sMyGraph->selectedMethod->method == NULL )
    {
        //---------------------------------------------------
        // FULL SCAN이 선택된 경우
        //---------------------------------------------------

        if ( sMyGraph->graph.left != NULL )
        {
            //---------------------------------------------------
            // View인 경우, 하위의 Preserved Order를 따름
            //---------------------------------------------------

            if ( sMyGraph->graph.left->preservedOrder != NULL )
            {
                // BUG-43692,BUG-44004
                // recursive with (recursive view) 는 preserved order가질수 없다.
                if ( ( sTableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                     == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
                {
                    sMyGraph->graph.preservedOrder = NULL;
                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
                }
                else
                {
                    // 하위 view의 preserved order 복사 후,
                    // table ID 변경
                    IDE_TEST( copyPreservedOrderFromView(
                                  aStatement,
                                  sMyGraph->graph.left,
                                  sTableRef->table,
                                  & sMyGraph->graph.preservedOrder )
                              != IDE_SUCCESS );

                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |= ( sMyGraph->graph.left->flag &
                                              QMG_PRESERVED_ORDER_MASK );
                }
            }
            else
            {
                sMyGraph->graph.preservedOrder = NULL;
                sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;

                if ( ( ( sMyGraph->graph.left->flag & QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK ) ==
                       QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE ) ||
                     ( ( sMyGraph->graph.left->flag & QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK ) ==
                       QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE ) )
                {
                    // view 최적화 type이 View Materialization인 경우
                    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
                    sMyGraph->graph.left->flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.left->flag |= QMG_PRESERVED_ORDER_NEVER;
                }
                else
                {
                    //view 최적화 type이 Push Selection인 경우
                    sMyGraph->graph.flag |= ( sMyGraph->graph.left->flag &
                                              QMG_PRESERVED_ORDER_MASK );
                }
            }
        }
        else
        {
            // nothing to do
        }

        if ( ( sMyGraph->graph.flag & QMG_PRESERVED_ORDER_MASK )
             == QMG_PRESERVED_ORDER_NOT_DEFINED )
        {
            if( sSelectedScanHint == QMG_USED_ONLY_FULL_SCAN_HINT )
            {
                //---------------------------------------------------
                // FULL SCAN Hint가 선택된 경우
                //---------------------------------------------------

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
                    // index가 존재하는 경우
                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |=
                        QMG_PRESERVED_ORDER_NOT_DEFINED;
                }
                else
                {
                    // index가 없는 경우
                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
                }
            }
        }
    }
    else
    {
        //---------------------------------------------------
        // INDEX SCAN이 선택된 경우
        //---------------------------------------------------

        // To Fix PR-9181
        // Index Scan이라 할 지라도
        // IN SUBQUERY KEY RANGE가 사용될 경우
        // Order가 보장되지 않는다.
        if ( ( sMyGraph->selectedMethod->method->flag &
               QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK )
             == QMO_STAT_CARD_IDX_IN_SUBQUERY_TRUE )
        {
            sMyGraph->graph.preservedOrder = NULL;

            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
        }
        else
        {
            IDE_TEST( makePreservedOrder( aStatement,
                                          sMyGraph->selectedMethod->method,
                                          sTableRef->table,
                                          & sMyGraph->graph.preservedOrder )
                      != IDE_SUCCESS );

            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
        }
    }

    /* BUG-43006 실제 Index가 아니고 FixedTable의 Filter 역활을
     * 수행하기 때문에 ordering이 없다. 따라서 presevered order 가
     * 없다고 설정해줘야한다.
     */
    /* BUG-43498 fixed table에서 internal index 사용시 정렬이 않됨 */
    if ( ( sTableRef->tableInfo->tableType == QCM_FIXED_TABLE ) ||
         ( sTableRef->tableInfo->tableType == QCM_DUMP_TABLE ) ||
         ( sTableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW ) )
    {
        sMyGraph->graph.preservedOrder = NULL;
        sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
        sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
    }
    else
    {
        /* Nothing to do */
    }

    //---------------------------------------------------
    // 공통 비용 정보의 설정
    //---------------------------------------------------

    sMyGraph->graph.costInfo.selectivity = 
        sMyGraph->selectedMethod->methodSelectivity;

    // output record count 설정
    sOutputRecordCnt = sMyGraph->graph.costInfo.selectivity *
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

    //---------------------------------------
    // 총 비용 정보 설정
    //---------------------------------------

    // 0 ( Child의 Total Cost) + My Cost
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->selectedMethod->accessCost;

    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->selectedMethod->diskCost;

    sMyGraph->graph.costInfo.totalAllCost =
        sMyGraph->selectedMethod->totalCost;

    if ( sMyGraph->graph.left != NULL )
    {
        // Child가 존재하는 경우

        if ( ( (sMyGraph->graph.left->flag & QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK)
               == QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE ) ||
             ( (sMyGraph->graph.left->flag & QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK)
               == QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE ) )
        {
            // View Materialization 된 경우라면
            // Child의 Cost를 누적하지 않는다.
        }
        else
        {
            // Child의 비용정보를 누적한다.
            sMyGraph->graph.costInfo.totalAccessCost +=
                sMyGraph->graph.left->costInfo.totalAccessCost;

            sMyGraph->graph.costInfo.totalDiskCost +=
                sMyGraph->graph.left->costInfo.totalDiskCost;

            sMyGraph->graph.costInfo.totalAllCost +=
                sMyGraph->graph.left->costInfo.totalAllCost;
        }
    }
    else
    {
        // 순수한 Selection 그래프임.
        // 누적할 정보가 없음
    }

    //---------------------------------------------------
    // PROJ-1446 Host variable을 포함한 질의 최적화
    // host variable에 대한 최적화를 위한 준비과정
    //---------------------------------------------------
    if ((sSelectedScanHint == QMG_NOT_USED_SCAN_HINT) &&
        (sMyGraph->accessMethodCnt > 1))
    {
        /*
         * BUG-38863 result wrong: host variable
         * table scan parallel or vertical parallel 에서는
         * host 변수 최적화를 수행하지 않는다.
         */
        if ((sTableRef->mParallelDegree == 1) ||
            ((aGraph->flag & QMG_PARALLEL_IMPOSSIBLE_MASK) ==
             QMG_PARALLEL_IMPOSSIBLE_TRUE))
        {
            if ( (QCU_HOST_OPTIMIZE_ENABLE == 1) &&
                 (QCU_PLAN_RANDOM_SEED == 0 ) )
            {
                IDE_TEST( prepareScanDecisionFactor( aStatement,
                                                     sMyGraph )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }

            // environment의 기록
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_HOST_OPTIMIZE_ENABLE );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        // scan hint가 사용되면 호스트 변수에 대한 최적화를 수행하지 않는다.
        // index가 없을 경우에도 호스트 변수에 대한 최적화를 수행하지 않는다.
        // rid predicate 이 있을때도 index scan 하지 않으므로 수행하지 않는다.
        // table scan parallel 또는 vertical parallel 에서 수행하지 않는다.
        // Nothing to do...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PROJ-2402 Parallel Table Scan
 * ------------------------------------------------------------------
 * 일반 table 에 parallel 을 할지 말지를 결정하는 flag
 *
 * 1. parallel degree > 1
 * 2. select for update 의 경우 불가능
 * 3. index scan 이 아니어야 함
 * 4. subquery filter 가 없어야 함
 * 5. predicate 에 outer column 이 없어야 함
 * ------------------------------------------------------------------
 */
void qmgSelection::setParallelScanFlag(qcStatement* aStatement,
                                       qmgGraph   * aGraph)
{
    qmgSELT     * sMyGraph;
    qmoPredicate* sPredicate;
    qmoPredicate* sNextIter;
    qmoPredicate* sMoreIter;
    idBool        sIsParallel;

    sMyGraph   = (qmgSELT*)aGraph;
    sPredicate = sMyGraph->graph.myPredicate;

    sIsParallel = ID_TRUE;

    if ( aGraph->myFrom->tableRef->mParallelDegree > 1 )
    {
        if ( aGraph->myFrom->tableRef->tableAccessHints != NULL )
        {
            /* BUG-43757 Partial Full Scan Hint 사용시 Parallel Plan
             * 은 사용되지 않아야한다
             */
            if ( aGraph->myFrom->tableRef->tableAccessHints->count > 1 )
            {
                aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
                aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
                sIsParallel = ID_FALSE;
                IDE_CONT(LABEL_EXIT);
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
        aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
        aGraph->flag |= QMG_PARALLEL_EXEC_TRUE;
    }
    else
    {
        aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
        aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
        sIsParallel = ID_FALSE;
        IDE_CONT(LABEL_EXIT);
    }

    // BUG-42317
    // 상위 qmgGraph 의 parallel 수행에 영향을 미치는
    // QMG_PARALLEL_EXEC_MASK 를 결정짓는 검사를 우선 check 해야 한다.
    if ( sPredicate != NULL )
    {
        for (sNextIter = sPredicate;
             sNextIter != NULL;
             sNextIter = sNextIter->next)
        {
            for (sMoreIter = sNextIter;
                 sMoreIter != NULL;
                 sMoreIter = sMoreIter->more)
            {
                if ((sMoreIter->node->lflag & QTC_NODE_SUBQUERY_MASK) ==
                    QTC_NODE_SUBQUERY_EXIST)
                {
                    /* 상위 graph 도 parallel 할 수 없음 */
                    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
                    aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
                    sIsParallel = ID_FALSE;
                    IDE_CONT(LABEL_EXIT);
                }
                else
                {
                    /* nothing to do */
                }

                /* BUG-39769
                   node가 function일 경우, parallel scan하면 안 된다. */
                if ( (sMoreIter->node->lflag & QTC_NODE_PROC_FUNCTION_MASK) ==
                     QTC_NODE_PROC_FUNCTION_TRUE )
                {
                    /* 상위 graph 도 parallel 할 수 없음 */
                    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
                    aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
                    sIsParallel = ID_FALSE;
                    IDE_CONT(LABEL_EXIT);
                }
                else
                {
                    /* Nothing to do */
                }

                /* BUG-39770
                   package를 참조할 경우, parallel scan하면 안 된다. */
                if ( (sMoreIter->node->lflag & QTC_NODE_PKG_MEMBER_MASK) ==
                     QTC_NODE_PKG_MEMBER_EXIST )
                {
                    /* 상위 graph 도 parallel 할 수 없음 */
                    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
                    aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
                    sIsParallel = ID_FALSE;
                    IDE_CONT(LABEL_EXIT);
                }
                else
                {
                    /* Nothing to do. */
                }

                /* BUG-41932
                   lob를 참조할 경우, parallel scan하면 안 된다. */
                if ( (sMoreIter->node->lflag & QTC_NODE_LOB_COLUMN_MASK) ==
                     QTC_NODE_LOB_COLUMN_EXIST )
                {
                    /* 상위 graph 도 parallel 할 수 없음 */
                    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
                    aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
                    sIsParallel = ID_FALSE;
                    IDE_CONT(LABEL_EXIT);
                }
                else
                {
                    /* Nothing to do. */
                }

                /*
                 * BUG-38763
                 * push_pred 에 의해 t1.i1 = t2.i1 와 같은
                 * predicate 이 들어온 경우
                 * t1.i1 과 t2.i1 이 서로 다른 template 에 있으면
                 * filter 가 제대로 동작하지 않는다.
                 * 이를 방지하기위해 parallel plan 을 생성하지 않는다.
                 */
                if (qtc::dependencyContains(&aGraph->depInfo,
                                            &sMoreIter->node->depInfo) == ID_FALSE)
                {
                    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
                    aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
                    sIsParallel = ID_FALSE;
                    IDE_CONT(LABEL_EXIT);
                }
                else
                {
                    /* nothing to do */
                } 
            }
        }
    }
    else
    {
        /* nothing to do */
    }

    // BUG-39567 Lateral View 내부에서는 Parallel Scan을 금지한다.
    if ( qtc::haveDependencies( & sMyGraph->graph.myQuerySet->lateralDepInfo )
         == ID_TRUE )
    {
        sIsParallel = ID_FALSE;
        IDE_CONT(LABEL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    if (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE)
    {
        /*
         * BUG-38803
         * select for update 구문은 SCAN->init 에서 cursor open 하므로
         * parallel 불가능
         */
        sIsParallel = ID_FALSE;
        IDE_CONT(LABEL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    if ( sMyGraph->graph.ridPredicate != NULL )
    {
        // BUG-43756 _prowid 사용시 parallel 플랜을 생성하지 않습니다.
        sIsParallel = ID_FALSE;
        IDE_CONT(LABEL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    if ( aGraph->myFrom->tableRef->remoteTable != NULL )
    {
        // BUG-43819 remoteTable 사용시 parallel 플랜을 생성하지 않습니다.
        sIsParallel = ID_FALSE;
        IDE_CONT(LABEL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    IDE_EXCEPTION_CONT(LABEL_EXIT);

    if (sIsParallel == ID_TRUE)
    {
        sMyGraph->mFlag &= ~QMG_SELT_PARALLEL_SCAN_MASK;
        sMyGraph->mFlag |= QMG_SELT_PARALLEL_SCAN_TRUE;
    }
    else
    {
        sMyGraph->mFlag &= ~QMG_SELT_PARALLEL_SCAN_MASK;
        sMyGraph->mFlag |= QMG_SELT_PARALLEL_SCAN_FALSE;
    }

}

IDE_RC
qmgSelection::prepareScanDecisionFactor( qcStatement * aStatement,
                                         qmgSELT     * aGraph )
{
/***********************************************************************
 *
 * Description : 호스트 변수가 있는 질의문을 최적화하기 위해
 *               qmoScanDecisionFactor 자료 구조를 구성한다.
 *               predicate 중에서 호스트 변수가 존재하면,
 *               qmoScanDecisionFactor에 자료를 구축한다.
 *               이 자료는 data가 바인딩된 후, selectivity를 다시
 *               계산하고 access method를 재설정할 때 사용된다.
 *               하나의 selection graph에 대해
 *               하나의 qmoScanDecisionFactor가 생성되거나 혹은
 *               생성되지 않는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoScanDecisionFactor * sSDF;

    IDU_FIT_POINT_FATAL( "qmgSelection::prepareScanDecisionFactor::__FT__" );

    if( qmoPred::checkPredicateForHostOpt( aGraph->graph.myPredicate )
        == ID_TRUE )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoScanDecisionFactor ),
                                                 (void **)&sSDF )
                  != IDE_SUCCESS );

        sSDF->next = NULL;
        sSDF->baseGraph = aGraph;
        sSDF->basePlan = NULL;
        sSDF->candidateCount = 0;

        // sSDF->predicate 세팅
        IDE_TEST( qmoPred::deepCopyPredicate( QC_QMP_MEM(aStatement),
                                              aGraph->graph.myPredicate,
                                              &sSDF->predicate )
                  != IDE_SUCCESS );

        // sSDF->predicate->mySelectivityOffset, totalSelectivityOffset 세팅
        IDE_TEST( setSelectivityOffset( aStatement,
                                        sSDF->predicate )
                  != IDE_SUCCESS );

        // sSDF->accessMethodsOffset 세팅
        IDE_TEST( qtc::getDataOffset( aStatement,
                                      ID_SIZEOF(qmoAccessMethod)
                                      * aGraph->accessMethodCnt,
                                      &sSDF->accessMethodsOffset )
                  != IDE_SUCCESS );

        // sSDF->selectedMethodOffset 세팅
        IDE_TEST( qtc::getDataOffset( aStatement,
                                      ID_SIZEOF(UInt),
                                      &sSDF->selectedMethodOffset )
                  != IDE_SUCCESS );

        // sSDF 연결
        IDE_TEST( qtc::addSDF( aStatement, sSDF ) != IDE_SUCCESS );

        aGraph->sdf = sSDF;
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::setSelectivityOffset( qcStatement  * aStatement,
                                    qmoPredicate * aPredicate )
{
/********************************************************************
 *
 * Description : predicate의 mySelectivity와 totalSelectivity를
 *               세팅한다.
 *
 *               이 함수는 checkPredicateForHostOpt() 함수가 호출된 후에
 *               불려야 한다.
 *
 * Implementation :
 *
 *               모든 predicate들에 대해 host 최적화가 가능하면
 *               mySelectivityOffset에 data 위치를 세팅한다.
 *               그렇지 않으면 QMO_SELECTIVITY_OFFSET_NOT_USED를
 *               세팅한다.
 *
 *               more list의 첫번째 predicate에 대해
 *               host 최적화가 가능하면 totalSelectivity에
 *               data 위치를 세팅하고 그렇지 않으면
 *               QMO_SELECTIVITY_OFFSET_NOT_USED를 세팅한다.
 *
 ********************************************************************/

    qmoPredicate * sNextIter;
    qmoPredicate * sMoreIter;

    IDU_FIT_POINT_FATAL( "qmgSelection::setSelectivityOffset::__FT__" );

    for( sNextIter = aPredicate;
         sNextIter != NULL;
         sNextIter = sNextIter->next )
    {
        IDE_TEST( setMySelectivityOffset( aStatement, sNextIter )
                  != IDE_SUCCESS );

        IDE_TEST( setTotalSelectivityOffset( aStatement, sNextIter )
                  != IDE_SUCCESS );

        for( sMoreIter = sNextIter->more;
             sMoreIter != NULL;
             sMoreIter = sMoreIter->more )
        {
            IDE_TEST( setMySelectivityOffset( aStatement, sMoreIter )
                      != IDE_SUCCESS );

            sMoreIter->totalSelectivityOffset =
                QMO_SELECTIVITY_OFFSET_NOT_USED;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::setMySelectivityOffset( qcStatement  * aStatement,
                                      qmoPredicate * aPredicate )
{

    IDU_FIT_POINT_FATAL( "qmgSelection::setMySelectivityOffset::__FT__" );

    if( ( aPredicate->flag & QMO_PRED_HOST_OPTIMIZE_MASK )
        == QMO_PRED_HOST_OPTIMIZE_TRUE )
    {
        IDE_TEST( qtc::getDataOffset( aStatement,
                                      ID_SIZEOF(aPredicate->mySelectivity),
                                      &aPredicate->mySelectivityOffset )
                  != IDE_SUCCESS );
    }
    else
    {
        aPredicate->mySelectivityOffset = QMO_SELECTIVITY_OFFSET_NOT_USED;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::setTotalSelectivityOffset( qcStatement  * aStatement,
                                         qmoPredicate * aPredicate )
{

    IDU_FIT_POINT_FATAL( "qmgSelection::setTotalSelectivityOffset::__FT__" );

    if( ( aPredicate->flag & QMO_PRED_HEAD_HOST_OPTIMIZE_MASK )
        == QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE )
    {
        IDE_TEST( qtc::getDataOffset( aStatement,
                                      ID_SIZEOF(aPredicate->totalSelectivity),
                                      &aPredicate->totalSelectivityOffset )
                  != IDE_SUCCESS );
    }
    else
    {
        aPredicate->totalSelectivityOffset = QMO_SELECTIVITY_OFFSET_NOT_USED;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgSelection로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgSelection으로 부터 생성가능한 plan
 *
 *        1. Base Table을 위한 Selection Graph인 경우
 *
 *           - 모든 Predicate 정보는 [SCAN]노드에 포함된다.
 *
 *                 [SCAN]
 *
 *        2. View를 위한 Selection Graph인 경우
 *
 *           - 이 경우는 qmgSelection의 Child가 반드시 qmgProjection이다.
 *           - Predicate의 존재 유무에 따라 [FILT] 노드가 생성된다.
 *
 *           2.1 Child Graph qmgProjection이 View Materialization된 경우
 *
 *               ( [FILT] )
 *                   |
 *                 [VSCN]
 *
 *           2.2 Child Graph qmgProjection이 View Materialization이 아닌 경우
 *
 *               ( [FILT] )
 *                   |
 *                 [VIEW]
 *
 *        3. recursive View를 위한 Selection Graph인 경우
 *
 *           - 이 경우는 qmgSelection의 Child가 반드시 qmgProjection이다.
 *
 *                 [VSCN]
 *
 *
 ***********************************************************************/

    qmgSELT         * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgSelection::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph       = (qmgSELT *) aGraph;

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

    // PROJ-2179
    // UPDATE 구문의 경우 parent가 NULL일 수 있다.
    if( aParent != NULL )
    {
        sMyGraph->graph.myPlan = aParent->myPlan;

        /* PROJ-1071 Parallel Query */
        aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
        aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);
    }
    else
    {
        sMyGraph->graph.myPlan = NULL;

        /* PROJ-1071 Parallel Query */
        aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
        aGraph->flag |= QMG_PARALLEL_IMPOSSIBLE_TRUE;
    }

    if( (sMyGraph->graph.left == NULL) )
    {
        /*
         * 1. DML 이 아니어야 함
         * 2. keyrange 가 없어야 함
         * 3. subquery 아니어야 함
         * 4. parallel table scan 가 가능한 graph
         * 5. 반복수행되는 join 오른쪽이 아니어야 함
         */
        if (((aGraph->flag & QMG_PARALLEL_IMPOSSIBLE_MASK) ==
             QMG_PARALLEL_IMPOSSIBLE_FALSE) &&
            ((aGraph->flag & QMG_SELT_NOTNULL_KEYRANGE_MASK) ==
             QMG_SELT_NOTNULL_KEYRANGE_FALSE) &&
            (aGraph->myQuerySet->parentTupleID == ID_USHORT_MAX) &&
            ((sMyGraph->mFlag & QMG_SELT_PARALLEL_SCAN_MASK) ==
             QMG_SELT_PARALLEL_SCAN_TRUE) &&
            ((aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK) ==
             QMG_PLAN_EXEC_REPEATED_FALSE))
        {
            IDE_TEST(makeParallelScan(aStatement, sMyGraph) != IDE_SUCCESS);
        }
        else
        {
            // PROJ-2582 recursive with
            if( ( sMyGraph->graph.myFrom->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
            {
                IDE_TEST( makeRecursiveViewScan( aStatement,
                                                 sMyGraph )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( makeTableScan( aStatement,
                                         sMyGraph )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        IDE_TEST( makeViewScan( aStatement,
                                sMyGraph )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::makeTableScan( qcStatement * aStatement,
                             qmgSELT     * aMyGraph )
{
    qmsTableRef     * sTableRef;
    qmoSCANInfo       sSCANInfo;
    qtcNode         * sLobFilter; // BUG-25916
    qmnPlan         * sFILT;
    qmnPlan         * sSCAN;

    IDU_FIT_POINT_FATAL( "qmgSelection::makeTableScan::__FT__" );

    sSCANInfo.flag = QMO_SCAN_INFO_INITIALIZE;

    //-----------------------------------------------------
    //        [*FILT]
    //           |
    //        [SCAN]
    // * Option
    //-----------------------------------------------------

    //-----------------------------------------------------
    // 1. Base Table을 위한 Selection Graph인 경우
    //-----------------------------------------------------

    // 하위 노드가 없는 leaf인 base일 경우 SCAN을 생성한다.

    // To Fix PR-11562
    // Indexable MIN-MAX 최적화가 적용된 경우
    // Preserved Order는 방향성을 가짐, 따라서 해당 정보를
    // 설정해줄 필요가 없음.
    // INDEXABLE Min-Max의 설정
    // 관련 코드 제거

    //-----------------------------------------------------
    // To Fix BUG-8747
    // Selection Graph에 Not Null Key Range를 생성하라는 Flag가
    // 있는 경우, Leaf Info에 그 정보를 설정한다.
    // - Selection Graph에서 Not Null Key Range 생성 Flag 적용되는 조건
    //   (1) indexable Min Max가 적용된 Selection Graph
    //   (2) Merge Join 하위의 Selection Graph
    //-----------------------------------------------------

    if( (aMyGraph->graph.flag & QMG_SELT_NOTNULL_KEYRANGE_MASK ) ==
        QMG_SELT_NOTNULL_KEYRANGE_TRUE )
    {
        sSCANInfo.flag &= ~QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_NOTNULL_KEYRANGE_TRUE;
    }
    else
    {
        // To Fix PR-10288
        // NOTNULL KEY RANGE가 아닌 경우로 반드시 설정해 주어야 함.
        sSCANInfo.flag &= ~QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_NOTNULL_KEYRANGE_FALSE;
    }

    if (aMyGraph->forceIndexScan == ID_TRUE)
    {
        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_INDEX_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_INDEX_SCAN_TRUE;
    }
    else
    {
        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_INDEX_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_INDEX_SCAN_FALSE;
    }

    sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_RID_SCAN_MASK;
    sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_RID_SCAN_FALSE;

    sSCANInfo.predicate         = aMyGraph->graph.myPredicate;
    sSCANInfo.constantPredicate = aMyGraph->graph.constantPredicate;
    sSCANInfo.ridPredicate      = aMyGraph->graph.ridPredicate;
    sSCANInfo.limit             = aMyGraph->limit;
    sSCANInfo.index             = aMyGraph->selectedIndex;
    sSCANInfo.preservedOrder    = aMyGraph->graph.preservedOrder;
    sSCANInfo.sdf               = aMyGraph->sdf;
    sSCANInfo.nnfFilter         = aMyGraph->graph.nnfFilter;

    /* BUG-39306 partial scan */
    sTableRef = aMyGraph->graph.myFrom->tableRef;
    if ( sTableRef->tableAccessHints != NULL )
    {
        sSCANInfo.mParallelInfo.mDegree = sTableRef->tableAccessHints->count;
        sSCANInfo.mParallelInfo.mSeqNo  = sTableRef->tableAccessHints->id;
    }
    else
    {
        sSCANInfo.mParallelInfo.mDegree = 1;
        sSCANInfo.mParallelInfo.mSeqNo  = 1;
    }

    //SCAN생성
    //생성된 노드의 위치는 반드시 graph.myPlan에 세팅을 하도록 한다.
    //이 정보를 다시 child로 삼고 다음 노드 만들때 연결하도록 한다.
    IDE_TEST( qmoOneNonPlan::makeSCAN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myFrom ,
                                       &sSCANInfo,
                                       aMyGraph->graph.myPlan,
                                       &sSCAN )
              != IDE_SUCCESS);

    qmg::setPlanInfo( aStatement, sSCAN, &(aMyGraph->graph) );

    // BUG-25916 : clob을 select for update 하던 도중 assert 발생
    // clob locator의 제약으로 lobfilter로 분류된 것이 존재하면
    // SCAN 노드 상위에 FILTER 노드로 처리해야 한다.
    sLobFilter = ((qmncSCAN*)sSCAN)->method.lobFilter;

    if ( sLobFilter != NULL )
    {
        // BUGBUG
        // Lob filter의 경우 SCAN을 생성한 후에 유무를 알 수 있다.
        // 따라서 FILT의 생성 여부도 SCAN의 생성 후에 결정된다.
        // BUG-25916의 문제 해결 방법을 수정해야 한다.
        IDE_TEST( qmoOneNonPlan::initFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sLobFilter ,
                      aMyGraph->graph.myPlan ,
                      &sFILT) != IDE_SUCCESS);

        IDE_TEST( qmoOneNonPlan::makeFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sLobFilter ,
                      NULL,
                      sSCAN ,
                      sFILT) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo( aStatement, sFILT, &(aMyGraph->graph) );
    }
    else
    {
        aMyGraph->graph.myPlan = sSCAN;
    }

    // fix BUG-13482
    // SCAN 노드 생성시,
    // filter존재등으로 SCAN Limit 최적화를 적용하지 못한 경우,
    // selection graph의 limit도 NULL로 설정한다.
    // 이는 상위 PROJ 노드 생성시, limit start value 조정의 정보가 됨.
    if( sSCANInfo.limit == NULL )
    {
        aMyGraph->limit = NULL;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::makeViewScan( qcStatement * aStatement,
                            qmgSELT     * aMyGraph )
{
    UInt              sFlag   = 0;
    qtcNode         * sFilter = NULL;
    qtcNode         * sTmp    = NULL;
    qmnPlan         * sFILT   = NULL;
    qmnPlan         * sVSCN   = NULL;
    qmnPlan         * sVIEW   = NULL;

    IDU_FIT_POINT_FATAL( "qmgSelection::makeViewScan::__FT__" );

    //-----------------------------------------------------
    // 1. Non-materialized view
    // 2. Materialized view
    // 
    //     1. [*FILT]        2. [*FILT]
    //           |       OR        |
    //        [VIEW]            [VSCN]
    //           |                 |
    //        [LEFT]            [LEFT]
    // * Option
    //-----------------------------------------------------


    //---------------------------------------
    // predicate의 존재 유무에 따라
    // FILT 생성
    //---------------------------------------

    if( ( aMyGraph->graph.myPredicate != NULL ) ||
        ( aMyGraph->graph.constantPredicate != NULL ) ||
        ( aMyGraph->graph.nnfFilter != NULL ) )
    {
        if( aMyGraph->graph.myPredicate != NULL )
        {
            IDE_TEST( qmoPred::linkFilterPredicate(
                          aStatement ,
                          aMyGraph->graph.myPredicate ,
                          &sFilter ) != IDE_SUCCESS );

            // BUG-35155 Partial CNF
            if ( aMyGraph->graph.nnfFilter != NULL )
            {
                IDE_TEST( qmoPred::addNNFFilter4linkedFilter(
                              aStatement,
                              aMyGraph->graph.nnfFilter,
                              & sFilter ) != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( aMyGraph->graph.nnfFilter != NULL )
            {
                sFilter = aMyGraph->graph.nnfFilter;
            }
            else
            {
                sFilter = NULL;
            }
        }

        if ( sFilter != NULL )
        {
            IDE_TEST( qmoPred::setPriorNodeID(
                          aStatement ,
                          aMyGraph->graph.myQuerySet ,
                          sFilter ) != IDE_SUCCESS );

            /* BUG-47752 left outer join의 on절이 OR 조건으로 구성되고있고
             * 오른쪽 view에 aggregaion이 존재할경우 fatal
             */
            if ( ( ( aMyGraph->graph.flag & QMG_JOIN_RIGHT_MASK )
                   == QMG_JOIN_RIGHT_TRUE ) &&
                 ( ( aMyGraph->graph.flag & QMG_VIEW_PUSH_MASK )
                   == QMG_VIEW_PUSH_TRUE ) )
            {
                sTmp = sFilter;

                IDE_TEST( qtc::copyNodeTree( aStatement, 
                                             sTmp,
                                             &sFilter,
                                             ID_FALSE,
                                             ID_TRUE )
                           != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            // Nothing To Do
        }

        //make FILT
        IDE_TEST( qmoOneNonPlan::initFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sFilter ,
                      aMyGraph->graph.myPlan ,
                      &sFILT ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sFILT;
    }
    else
    {
        // Nothing To Do
    }

    if( (aMyGraph->graph.left->flag &
         QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK)
        == QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE )
    {
        IDE_TEST( qmoOneNonPlan::initVSCN( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           aMyGraph->graph.myFrom ,
                                           aMyGraph->graph.myPlan ,
                                           &sVSCN ) != IDE_SUCCESS );
        aMyGraph->graph.myPlan = sVSCN;
    }
    else
    {
        IDE_TEST( qmoOneNonPlan::initVIEW( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           aMyGraph->graph.myPlan ,
                                           &sVIEW ) != IDE_SUCCESS );
        aMyGraph->graph.myPlan = sVIEW;
    }

    //-----------------------------------------------------
    // 2. View를 위한 Selection Graph인 경우
    //-----------------------------------------------------

    IDE_DASSERT( aMyGraph->graph.left->type == QMG_PROJECTION );

    //----------------------
    // 하위 Plan의 생성
    //----------------------

    // To Fix BUG-8241
    if( aMyGraph->graph.left->myPlan == NULL )
    {
        // To Fix PR-8470
        // View에 대한 Plan Tree 생성 시에는 View의 Statement를
        // 사용하여야 함
        IDE_TEST( aMyGraph->graph.left->makePlan(
                      aMyGraph->graph.myFrom->tableRef->view,
                      &aMyGraph->graph,
                      aMyGraph->graph.left )
                  != IDE_SUCCESS);
    }
    else
    {
        // nothing to do
    }

    if( (aMyGraph->graph.left->flag &
         QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK)
        == QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE )
    {
        //---------------------------------------
        // VSCN생성
        //     - Materialization이 있는 경우
        //---------------------------------------

        IDE_TEST( qmoOneNonPlan::makeVSCN( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           aMyGraph->graph.myFrom ,
                                           aMyGraph->graph.left->myPlan ,
                                           sVSCN ) != IDE_SUCCESS );
        aMyGraph->graph.myPlan = sVSCN;

        qmg::setPlanInfo( aStatement, sVSCN, &(aMyGraph->graph) );
    }
    else
    {
        //---------------------------------------
        // VIEW 생성
        //     - Materialization이 없는 경우
        //---------------------------------------

        sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
        sFlag |= QMO_MAKEVIEW_FROM_SELECTION;

        //VIEW생성
        IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           aMyGraph->graph.myFrom ,
                                           sFlag ,
                                           aMyGraph->graph.left->myPlan ,
                                           sVIEW ) != IDE_SUCCESS );
        aMyGraph->graph.myPlan = sVIEW;

        qmg::setPlanInfo( aStatement, sVIEW, &(aMyGraph->graph) );
    }

    //---------------------------------------
    // predicate의 존재 유무에 따라
    // FILT 생성
    //---------------------------------------

    if( ( aMyGraph->graph.myPredicate != NULL ) ||
        ( aMyGraph->graph.constantPredicate != NULL ) ||
        ( aMyGraph->graph.nnfFilter != NULL ) )
    {
        //make FILT
        IDE_TEST( qmoOneNonPlan::makeFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sFilter ,
                      aMyGraph->graph.constantPredicate ,
                      aMyGraph->graph.myPlan ,
                      sFILT ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo( aStatement, sFILT, &(aMyGraph->graph) );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
  
/*
 * ------------------------------------------------------------------
 * PROJ-2402 Parallel Table Scan
 * ------------------------------------------------------------------
 *     [*FILT]
 *        |
 *     [PSCRD]
 *        |   
 *     [PRLQ]-[PRLQ]-[PRLQ]
 *       |      |      |
 *     [SCAN] [SCAN] [SCAN]
 * ------------------------------------------------------------------
 */
IDE_RC qmgSelection::makeParallelScan(qcStatement* aStatement,
                                      qmgSELT    * aMyGraph)
{
    qmnPlan     * sPSCRD        = NULL;
    qmnPlan     * sPRLQ         = NULL;
    qmnPlan     * sSCAN         = NULL;
    qmnPlan     * sFILT         = NULL;
    qmnPlan     * sParentPlan   = NULL;
    qmnChildren * sChildrenPRLQ = NULL;
    qmnChildren * sLastChildren = NULL;
    qtcNode     * sLobFilter    = NULL;
    qmoSCANInfo   sSCANInfo;
    qmoPSCRDInfo  sPSCRDInfo;
    UInt          sDegree;
    UInt          sPRLQCount;
    UInt          i;

    IDU_FIT_POINT_FATAL( "qmgSelection::makeParallelScan::__FT__" );

    sDegree = aMyGraph->graph.myFrom->tableRef->mParallelDegree;
    IDE_DASSERT(sDegree > 1);

    sParentPlan = aMyGraph->graph.myPlan;

    sPSCRD = NULL;
    IDE_TEST(qmoParallelPlan::initPSCRD(aStatement,
                                        aMyGraph->graph.myQuerySet,
                                        sParentPlan,
                                        &sPSCRD)
             != IDE_SUCCESS);

    // 초기화
    sPRLQCount      = 0;
    sSCANInfo.limit = NULL;

    for (i = 0; i < sDegree; i++)
    {
        sPRLQ = NULL;

        IDE_TEST(qmoParallelPlan::initPRLQ(aStatement,
                                           sPSCRD,
                                           &sPRLQ)
                 != IDE_SUCCESS);
        sPRLQ->left = NULL;

        IDU_FIT_POINT("qmgSelection::makeParallelScan::alloc", 
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmnChildren),
                                               (void**)&sChildrenPRLQ)
                 != IDE_SUCCESS);

        sChildrenPRLQ->childPlan = sPRLQ;
        sChildrenPRLQ->next = NULL;

        if (sLastChildren == NULL)
        {
            sPSCRD->childrenPRLQ = sChildrenPRLQ;
        }
        else
        {
            sLastChildren->next = sChildrenPRLQ;
        }
        sLastChildren = sChildrenPRLQ;
        sPRLQCount++;

        /*
         * ----------------------------------------------------------
         * make PRLQ
         * ----------------------------------------------------------
         */

        IDE_DASSERT((aMyGraph->graph.flag & QMG_SELT_NOTNULL_KEYRANGE_MASK) ==
                    QMG_SELT_NOTNULL_KEYRANGE_FALSE);
        IDE_DASSERT(aMyGraph->selectedIndex == NULL);
        IDE_DASSERT(aMyGraph->forceIndexScan == ID_FALSE);

        sSCANInfo.flag = QMO_SCAN_INFO_INITIALIZE;
        sSCANInfo.flag &= ~QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_NOTNULL_KEYRANGE_FALSE;

        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_INDEX_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_INDEX_SCAN_FALSE;

        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_RID_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_RID_SCAN_FALSE;

        IDE_TEST(qmoPred::deepCopyPredicate(QC_QMP_MEM(aStatement),
                                            aMyGraph->graph.myPredicate,
                                            &sSCANInfo.predicate)
                 != IDE_SUCCESS);

        IDE_TEST(qmoPred::deepCopyPredicate(QC_QMP_MEM(aStatement),
                                            aMyGraph->graph.ridPredicate,
                                            &sSCANInfo.ridPredicate)
                 != IDE_SUCCESS);

        /*
         * BUG-38823
         * constant filter 는 PSCRD 가 갖는다
         */
        sSCANInfo.constantPredicate = NULL;

        sSCANInfo.limit                 = aMyGraph->limit;
        sSCANInfo.index                 = aMyGraph->selectedIndex;
        sSCANInfo.preservedOrder        = aMyGraph->graph.preservedOrder;
        sSCANInfo.sdf                   = aMyGraph->sdf;
        sSCANInfo.nnfFilter             = aMyGraph->graph.nnfFilter;
        sSCANInfo.mParallelInfo.mDegree = sDegree;
        sSCANInfo.mParallelInfo.mSeqNo  = i + 1;

        sSCAN = NULL;
        IDE_TEST(qmoOneNonPlan::makeSCAN(aStatement,
                                         aMyGraph->graph.myQuerySet,
                                         aMyGraph->graph.myFrom,
                                         &sSCANInfo,
                                         sPRLQ,
                                         &sSCAN)
                 != IDE_SUCCESS);
        qmg::setPlanInfo(aStatement, sSCAN, &aMyGraph->graph);

        /*
         * ----------------------------------------------------------
         * make PRLQ
         * ----------------------------------------------------------
         */
        IDE_TEST(qmoParallelPlan::makePRLQ(aStatement,
                                           aMyGraph->graph.myQuerySet,
                                           QMO_MAKEPRLQ_PARALLEL_TYPE_SCAN,
                                           sSCAN,
                                           sPRLQ)
                 != IDE_SUCCESS);
    }

    /*
     * ----------------------------------------------------------
     * make PSCRD 
     * ----------------------------------------------------------
     */
    sPSCRDInfo.mConstantPredicate = aMyGraph->graph.constantPredicate;

    IDE_TEST(qmoParallelPlan::makePSCRD(aStatement,
                                        aMyGraph->graph.myQuerySet,
                                        aMyGraph->graph.myFrom,
                                        &sPSCRDInfo,
                                        sPSCRD)
             != IDE_SUCCESS);
    qmg::setPlanInfo(aStatement, sPSCRD, &aMyGraph->graph);

    aMyGraph->graph.myPlan = sPSCRD;

    sLobFilter = ((qmncSCAN*)sSCAN)->method.lobFilter;
    if (sLobFilter != NULL)
    {
        sFILT = NULL;
        IDE_TEST(qmoOneNonPlan::initFILT(aStatement,
                                         aMyGraph->graph.myQuerySet,
                                         sLobFilter,
                                         sParentPlan,
                                         &sFILT)
                 != IDE_SUCCESS);

        IDE_TEST(qmoOneNonPlan::makeFILT(aStatement,
                                         aMyGraph->graph.myQuerySet,
                                         sLobFilter,
                                         NULL,
                                         sPSCRD,
                                         sFILT)
                 != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo(aStatement, sFILT, &aMyGraph->graph);
    }
    else
    {
        /* nothing to do */
    }

    if (sSCANInfo.limit == NULL)
    {
        aMyGraph->limit = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::alterSelectedIndex( qcStatement * aStatement,
                                  qmgSELT     * aGraph,
                                  qcmIndex    * aNewSelectedIndex )
{
    qcmIndex * sNewSelectedIndex = NULL;
    UInt       i;

    IDU_FIT_POINT_FATAL( "qmgSelection::alterSelectedIndex::__FT__" );

    IDE_DASSERT( aGraph != NULL );

    if( aNewSelectedIndex != NULL )
    {
        // PROJ-1502 PARTITIONED DISK TABLE
        // 선택된 index가 local index라면 각 파티션에 맞는
        // local index partition을 찾아야 한다.
        if( ( aNewSelectedIndex->indexPartitionType ==
              QCM_LOCAL_PREFIXED_PARTITIONED_INDEX ) ||
            ( aNewSelectedIndex->indexPartitionType ==
              QCM_LOCAL_NONE_PREFIXED_PARTITIONED_INDEX ) )
        {
            IDE_DASSERT( aGraph->partitionRef != NULL );

            for( i = 0;
                 i < aGraph->partitionRef->partitionInfo->indexCount;
                 i++ )
            {
                if( aNewSelectedIndex->indexId ==
                    aGraph->partitionRef->partitionInfo->indices[i].indexId )
                {
                    sNewSelectedIndex = &aGraph->partitionRef->partitionInfo->indices[i];
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            IDE_FT_ASSERT( sNewSelectedIndex != NULL );
        }
        else
        {
            sNewSelectedIndex = aNewSelectedIndex;
        }
    }
    else
    {
        // Nothing to do.
    }

    aGraph->selectedIndex = sNewSelectedIndex;

    // To fix BUG-12742
    // 상위 graph에 의해 결정된 경우
    // index를 없앨 수 없다.
    aGraph->forceIndexScan = ID_TRUE;
    
    if( aGraph->sdf != NULL )
    {
        // 현재 selection graph의 selectedIndex가
        // 상위 graph에 의해 다시 결정된 경우
        // host optimization을 해서는 안된다.
        // 이 경우 sdf를 disable한다.
        IDE_TEST( qtc::removeSDF( aStatement, aGraph->sdf ) != IDE_SUCCESS );

        aGraph->sdf = NULL;
    }
    else
    {
        // Nothing to do...
    }

    /* PROJ-2402 Parallel Table Scan */
    aGraph->mFlag &= ~QMG_SELT_PARALLEL_SCAN_MASK;
    aGraph->mFlag |= QMG_SELT_PARALLEL_SCAN_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::printGraph( qcStatement  * aStatement,
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

    qmoPredicate * sPredicate;
    qmoPredicate * sMorePred;

    qmgSELT * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgSelection::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

    sMyGraph = (qmgSELT*)aGraph;

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
    // Access method 별 정보 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Access Method Information ==" );

    for ( i = 0; i < sMyGraph->accessMethodCnt; i++ )
    {
        IDE_TEST( printAccessMethod( aStatement,
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

    if ( aGraph->myFrom->tableRef->view != NULL )
    {
        // View에 대한 그래프 정보 출력
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "::VIEW STATEMENT GRAPH BEGIN" );

        // To Fix BUG-9586
        // view에 대한 graph 출력시 qcStatement는
        // aGraph->myFrom->tableRef->view 가 되어야 함
        IDE_TEST( aGraph->left->printGraph( aGraph->myFrom->tableRef->view,
                                            aGraph->left,
                                            aDepth + 1,
                                            aString )
                  != IDE_SUCCESS );

        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "::VIEW STATEMENT GRAPH END" );

    }
    else
    {
        // Nothing To Do
    }

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
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::printAccessMethod( qcStatement     *,
                                 qmoAccessMethod * aMethod,
                                 ULong             aDepth,
                                 iduVarString    * aString )
{
    UInt    i;

    IDU_FIT_POINT_FATAL( "qmgSelection::printAccessMethod::__FT__" );

    if( aMethod->method == NULL )
    {
        // FULL SCAN
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "FULL SCAN" );
    }
    else
    {
        // INDEX SCAN
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "INDEX SCAN[" );
        iduVarStringAppend( aString,
                            aMethod->method->index->name );
        iduVarStringAppend( aString,
                            "]" );
    }

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

IDE_RC
qmgSelection::makeViewGraph( qcStatement * aStatement,
                             qmgGraph    * aGraph )
{
/***********************************************************************
 *
 * Description : View Graph의 생성
 *
 * Implementation :
 *    (1) 첫번째 View이거나, 동일 View가 존재하지 않는 경우
 *        A. View 최적화 Hint가 없는 경우
 *           i    Push Selection 적용
 *           ii   View Graph 생성
 *           iii. View Graph 연결
 *           iv.  View 최적화 type 결정
 *           v.   View Materialization 여부를 하위 Projection Graph에 연결
 *        B. View 최적화 Hint가 VMTR인 경우
 *           ii   View Graph 생성
 *           iii. View Graph 연결
 *           iv.  View 최적화 type 결정 :  view 최적화 type을 Not Defined
 *                ( 두번째 view에서 View Materialization으로 설정됨 )
 *           v.   View Materialization 여부를 하위 Projection Graph에 연결
 *        C. View 최적화 Hint가 Push Selection인 경우
 *          i    Push Selection 적용
 *           ii   View Graph 생성
 *           iii. View Graph 연결
 *           iv.  View 최적화 type 결정 :
 *                성공 유무에 상관없이 view 최적화 type을 push selection
 *               ( view materialization으로 수행하지 않도록 하기 위함 )
 *           v.   View Materialization 여부를 하위 Projection Graph에 연결
 *    (2) 두번째 이상의 View인 경우
 *        A. 첫번째 View 최적화 type이 결정되지 않은 경우,
 *           View 최적화 type을 View Materialization으로 결정
 *        B. 첫번째 View 최적화 type이 결정된 경우,
 *           첫번째 View 최적화 type을 따름
 *
 ***********************************************************************/

    qmsTableRef  * sTableRef;
    qmoPredicate * sPredicate;
    idBool         sIsPushed;

    IDU_FIT_POINT_FATAL( "qmgSelection::makeViewGraph::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sTableRef          = aGraph->myFrom->tableRef;
    sIsPushed          = ID_FALSE;

    //---------------------------------------------------
    // BUG-40355 rownum predicate to limit
    //---------------------------------------------------

    // BUG-45296 rownum Pred 을 left outer 의 오른쪽으로 내리면 안됩니다.
    if ( (aGraph->flag & QMG_ROWNUM_PUSHED_MASK) == QMG_ROWNUM_PUSHED_TRUE )
    {
        IDE_TEST ( qmoRownumPredToLimit::rownumPredToLimitTransform(
                    aStatement,
                    aGraph->myQuerySet,
                    sTableRef,
                    &(aGraph->myPredicate) )
                != IDE_SUCCESS );
    }
    else
    {
        // nothing to do.
    }

    // doRownumPredToLimit 함수에서 myPredicate 값이 변경될수 있다.
    sPredicate         = aGraph->myPredicate;

    //---------------------------------------------------
    // View의 index hint 적용 PROJ-1495
    //---------------------------------------------------

    if( sTableRef->tableAccessHints != NULL )
    {
        IDE_TEST( setViewIndexHints( aStatement,
                                     sTableRef )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------
    // PROJ-1473 VIEW 내부로의 push projection 전파
    //---------------------------------------------------

    if( aGraph->myQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        IDE_TEST( doViewPushProjection(
                      aStatement,
                      sTableRef,
                      ((qmsParseTree*)(sTableRef->view->myPlan->parseTree))->querySet )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do 
    }

    //---------------------------------------------------
    // View 최적화 Type 결정
    //---------------------------------------------------

    if ( sTableRef->sameViewRef == NULL )
    {
        //---------------------------------------------------
        // 첫번째 View 이거나, 동일 View가 존재하지 않는 경우
        //---------------------------------------------------

        switch( sTableRef->viewOptType )
        {
            case QMO_VIEW_OPT_TYPE_NOT_DEFINED :

                //---------------------------------------------------
                // Hint 가 없는 경우
                //---------------------------------------------------

                // push selection 가능한지 시도
                IDE_TEST( doViewPushSelection( aStatement,
                                               sTableRef,
                                               sPredicate,
                                               aGraph,
                                               & sIsPushed )
                          != IDE_SUCCESS );

                // View Graph 생성
                IDE_TEST( qmo::makeGraph( sTableRef->view ) != IDE_SUCCESS );

                // View Graph 연결
                aGraph->left = sTableRef->view->myPlan->graph;

                // View 최적화 type 결정
                if ( sIsPushed == ID_TRUE )
                {
                    // push selection이 성공한 경우
                    sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_PUSH;
                }
                else
                {
                    // push selection 실패한 경우
                    sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_NOT_DEFINED;
                }

                // View Materialization 여부 설정
                aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_FALSE;
                break;
            case QMO_VIEW_OPT_TYPE_VMTR :

                //---------------------------------------------------
                // View NO_PUSH_SELECT_VIEW Hint가 있는 경우
                //    - 최초 View : Not Defined 로 설정
                //    - 그 이후 : 다음 View가 참조할때, Materialization 설정
                //               다음 View가 없는 경우, Materialization 설정
                //---------------------------------------------------

                // To Fix BUG-8400
                if ( sTableRef->view->myPlan->graph == NULL )
                {
                    // View Graph 생성
                    IDE_TEST( qmo::makeGraph( sTableRef->view )
                              != IDE_SUCCESS );

                    // View 최적화 type 결정
                    sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_NOT_DEFINED;

                    // To Fix PR-11558
                    // 생성된 View의 최상위 Graph를 연결시켜야 함.
                    aGraph->left = sTableRef->view->myPlan->graph;

                    // View Materialization 여부 설정
                    aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                    aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_FALSE;
                }
                else
                {
                    // View Graph 연결
                    aGraph->left = sTableRef->view->myPlan->graph;
                }

                break;
            case QMO_VIEW_OPT_TYPE_POTENTIAL_VMTR :

                //---------------------------------------------------
                // view를 지금까지 한번만 사용해서 VMTR을 보류했던 경우
                //---------------------------------------------------
                
                IDE_DASSERT( sTableRef->view->myPlan->graph != NULL );

                // View Graph 연결
                aGraph->left = sTableRef->view->myPlan->graph;

                // View Materialization 여부 설정
                aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE;

                // View 최적화 type 결정
                sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_VMTR;

                break;
            case QMO_VIEW_OPT_TYPE_PUSH :

                //---------------------------------------------------
                // Push Selection Hint가 있는 경우
                //    - Push Selection 성공 여부에 상관없이 ViewOptType을
                //      Push Selection으로 설정하여 다음 View 참조시
                //      View Materialization 하지 못하도록 한다.
                //---------------------------------------------------

                // push selection 가능한지 시도
                IDE_TEST( doViewPushSelection( aStatement,
                                               sTableRef,
                                               sPredicate,
                                               aGraph,
                                               & sIsPushed )
                          != IDE_SUCCESS );

                // View Graph 생성
                IDE_TEST( qmo::makeGraph( sTableRef->view ) != IDE_SUCCESS );

                // View Graph 연결
                aGraph->left = sTableRef->view->myPlan->graph;

                // View 최적화 type 결정
                sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_PUSH;

                // View Materialization 여부 설정
                aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_FALSE;
                break;
            case QMO_VIEW_OPT_TYPE_FORCE_VMTR :

                //---------------------------------------------------
                // View materialze Hint가 있는 경우
                //    Materialization 설정
                //---------------------------------------------------

                if ( sTableRef->view->myPlan->graph == NULL )
                {
                    // View Graph 생성
                    IDE_TEST( qmo::makeGraph( sTableRef->view )
                              != IDE_SUCCESS );

                    // View 최적화 type 결정
                    sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_FORCE_VMTR;

                    // 생성된 View의 최상위 Graph를 연결시켜야 함.
                    aGraph->left = sTableRef->view->myPlan->graph;

                    // View Materialization 여부 설정
                    aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                    aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE;
                }
                else
                {
                    // View Graph 연결
                    aGraph->left = sTableRef->view->myPlan->graph;
                }

                break;
            case QMO_VIEW_OPT_TYPE_CMTR :
                // To Fix BUG-8400
                if ( sTableRef->view->myPlan->graph == NULL )
                {
                    // View Graph 생성
                    IDE_TEST( qmo::makeGraph( sTableRef->view )
                              != IDE_SUCCESS );

                    // To Fix PR-11558
                    // 생성된 View의 최상위 Graph를 연결시켜야 함.
                    aGraph->left = sTableRef->view->myPlan->graph;

                    // View Materialization 여부 설정
                    aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK;
                    aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE;
                }
                else
                {
                    // View Graph 연결
                    aGraph->left = sTableRef->view->myPlan->graph;
                }
                break;
            default :
                IDE_DASSERT( 0 );
                break;

        }
    }
    else
    {
        //---------------------------------------------------
        // 두 번째 이상의 View인 경우
        //---------------------------------------------------

        switch ( sTableRef->sameViewRef->viewOptType )
        {
            case QMO_VIEW_OPT_TYPE_NOT_DEFINED:

                // PROJ-2582 recursive with
                // 첫번째 view의 Statement로 연결
                if ( ( sTableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                     == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
                {
                    sTableRef->view = sTableRef->sameViewRef->recursiveView;
                }
                else
                {
                    // BUG-43659 view 최적화시 결과오류
                    IDE_TEST( mergeViewTargetFlag( sTableRef->sameViewRef->view,
                                                   sTableRef->view ) != IDE_SUCCESS );

                    sTableRef->view = sTableRef->sameViewRef->view;
                }

                // BUG-38022
                // sameViewRef가 있지만 실제로 쿼리에서 한번만 사용하는 경우는
                // VMTR을 생성하지 않도록 한다.
                if ( sTableRef->view->myPlan->graph == NULL )
                {
                    IDE_TEST( qmo::makeGraph( sTableRef->view )
                              != IDE_SUCCESS );

                    // Graph를 연결한다.
                    aGraph->left = sTableRef->view->myPlan->graph;

                    // View Materialization 여부 설정
                    aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                    aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_FALSE;

                    // 첫번째 view의 최적화 type을 POTENTIAL_VMTR로 설정
                    sTableRef->sameViewRef->viewOptType = QMO_VIEW_OPT_TYPE_POTENTIAL_VMTR;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                // 실제로 쿼리에서 두번 이상 사용하는 경우
                // 현재 view와 첫번째 view의 최적화 type을 모두 VMTR로 설정
                sTableRef->sameViewRef->viewOptType = QMO_VIEW_OPT_TYPE_VMTR;
                /* fall through */
            case QMO_VIEW_OPT_TYPE_VMTR :
            case QMO_VIEW_OPT_TYPE_FORCE_VMTR :
            case QMO_VIEW_OPT_TYPE_POTENTIAL_VMTR :
                sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_VMTR;

                // PROJ-2582 recursive with
                // 첫번째 view의 Statement로 연결
                if ( ( sTableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                     == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
                {
                    sTableRef->view = sTableRef->sameViewRef->recursiveView;
                }
                else
                {
                    // BUG-43659 view 최적화시 결과오류
                    IDE_TEST( mergeViewTargetFlag( sTableRef->sameViewRef->view,
                                                   sTableRef->view ) != IDE_SUCCESS );

                    sTableRef->view = sTableRef->sameViewRef->view;
                }

                // To Fix BUG-8400
                // View Graph가 생성되지 않은 경우,
                // View Graph를 생성한다.
                if ( sTableRef->view->myPlan->graph == NULL )
                {
                    IDE_TEST( qmo::makeGraph( sTableRef->view )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do
                }

                // Graph를 연결한다.
                aGraph->left = sTableRef->view->myPlan->graph;

                // 하위의 Projection Graph에 VMTR 정보를 설정
                aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE;

                // To Fix PR-11562
                // View Materialization의 preserverd order를 무조건 ASC 으로
                // 설정하면 안됨.
                // 관련 코드 제거.
                break;

            case QMO_VIEW_OPT_TYPE_PUSH :
                // 첫번째 view의 최적화 type을 따른다.
                sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_PUSH;

                // push selection 가능한지 시도해 가능하면
                // push selection 수행

                IDE_TEST( doViewPushSelection( aStatement,
                                               sTableRef,
                                               sPredicate,
                                               aGraph,
                                               & sIsPushed )
                          != IDE_SUCCESS );

                // View Graph 생성
                IDE_TEST( qmo::makeGraph( sTableRef->view ) != IDE_SUCCESS );

                // View Graph 연결
                aGraph->left = sTableRef->view->myPlan->graph;

                // 하위의 Projection Graph에 VMTR 정보를 설정
                aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
                aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_FALSE;
                break;
            case QMO_VIEW_OPT_TYPE_CMTR :
                // BUG-37237 hierarchy query는 same view를 처리하지 않는다.
                if ( sTableRef->view->myPlan->graph == NULL )
                {
                    // View Graph 생성
                    IDE_TEST( qmo::makeGraph( sTableRef->view )
                              != IDE_SUCCESS );

                    aGraph->left = sTableRef->view->myPlan->graph;

                    // View Materialization 여부 설정
                    aGraph->left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK;
                    aGraph->left->flag |= QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE;
                }
                else
                {
                    // View Graph 연결
                    aGraph->left = sTableRef->view->myPlan->graph;
                }
                break;
            default:
                IDE_DASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgSelection::mergeViewTargetFlag( qcStatement * aDesView,
                                          qcStatement * aSrcView )
{
    qmsTarget  * sDesViewTarget;
    qmsTarget  * sSrcViewTarget;
    mtcColumn  * sDesColumn;
    mtcColumn  * sSrcColumn;

    sDesViewTarget = ((qmsParseTree*)(aDesView->myPlan->parseTree))->querySet->target;
    sSrcViewTarget = ((qmsParseTree*)(aSrcView->myPlan->parseTree))->querySet->target;

    for (  ;
           sSrcViewTarget != NULL;
           sDesViewTarget = sDesViewTarget->next,
               sSrcViewTarget = sSrcViewTarget->next )
    {
        sDesColumn = QTC_STMT_COLUMN( aDesView, sDesViewTarget->targetColumn );
        sSrcColumn = QTC_STMT_COLUMN( aSrcView, sSrcViewTarget->targetColumn );

        // makeSCAN 에서 사용되는 flag 를 유지시킴
        sDesColumn->flag |= sSrcColumn->flag & (
            MTC_COLUMN_USE_COLUMN_MASK |
            MTC_COLUMN_USE_TARGET_MASK |
            MTC_COLUMN_USE_NOT_TARGET_MASK |
            MTC_COLUMN_VIEW_COLUMN_PUSH_MASK |
            MTC_COLUMN_OUTER_REFERENCE_MASK );
    }

    return IDE_SUCCESS;
}

IDE_RC
qmgSelection::setViewIndexHints( qcStatement         * aStatement,
                                 qmsTableRef         * aTableRef )
{
//----------------------------------------------------------------------
//
// Description : VIEW index hint를 VIEW를 구성하는 해당 base table에 내린다.
//
// Implementation : PROJ-1495
//
//
//   예) create table t1( i1 integer, i2 integer );
//       create index t1_i1 on t1( i1 );
//
//       create table t2( i1 integer, i2 integer );
//       create index t2_i1 on t2( i1 );
//
//       create view v1 ( a1, a2 ) as
//       select * from t1
//       union all
//       select * from t2;
//
//       select /*+ index( v1, t1_i1, t2_i1 ) */
//              *
//       from v1;
//
//----------------------------------------------------------------------

    qmsTableAccessHints  * sAccessHint;
    qmsQuerySet          * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgSelection::setViewIndexHints::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aTableRef->tableAccessHints != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sQuerySet = ((qmsParseTree *)aTableRef->view->myPlan->parseTree)->querySet;

    //---------------------------------------------------
    // 각 table Access hint를 VIEW 내부 해당 base table로 내린다.
    //---------------------------------------------------

    for( sAccessHint = aTableRef->tableAccessHints;
         sAccessHint != NULL;
         sAccessHint = sAccessHint->next )
    {
        IDE_TEST( findQuerySet4ViewIndexHints( aStatement,
                                               sQuerySet,
                                               sAccessHint )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgSelection::findQuerySet4ViewIndexHints( qcStatement         * aStatement,
                                           qmsQuerySet         * aQuerySet,
                                           qmsTableAccessHints * aAccessHint )
{
/***********************************************************************
 *
 * Description : VIEW index hint를 VIEW를 구성하는 해당 base table에 내린다.
 *
 * Implementation : PROJ-1495
 *
 *  view 내부 해당 querySet을 찾는다.
 *
 ***********************************************************************/

    qmsFrom     * sFrom;

    IDU_FIT_POINT_FATAL( "qmgSelection::findQuerySet4ViewIndexHints::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aAccessHint != NULL );

    //---------------------------------------------------
    // set이 아닌 querySet을 찾아 view index hint를 적용
    //---------------------------------------------------

    if( aQuerySet->setOp == QMS_NONE )
    {
        for( sFrom = aQuerySet->SFWGH->from;
             sFrom != NULL;
             sFrom = sFrom->next )
        {
            //----------------------------------
            // view index hint를 해당 base table에 정보 설정
            //----------------------------------
            if( sFrom->joinType == QMS_NO_JOIN )
            {
                if( sFrom->tableRef->view == NULL )
                {
                    //---------------------------------------
                    // 현재 테이블이 base table로 index hint 적용
                    //---------------------------------------

                    IDE_TEST( setViewIndexHintInBaseTable( aStatement,
                                                           sFrom,
                                                           aAccessHint )
                              != IDE_SUCCESS );
                }
                else
                {
                    // VIEW 인 경우로,
                    // index hint 적용대상이 아님.
                }
            }
            else
            {
                //-----------------------------------------
                // 현재 테이블이 base table이 아니므로,
                // left, right from을 순회하면서
                // base table을 찾아서 index hint 정보 설정
                //-----------------------------------------

                IDE_TEST(
                    findBaseTableNSetIndexHint( aStatement,
                                                sFrom,
                                                aAccessHint )
                    != IDE_SUCCESS );
            }
        }
    }
    else
    {
        if( aQuerySet->left != NULL )
        {
            IDE_TEST( findQuerySet4ViewIndexHints( aStatement,
                                                   aQuerySet->left,
                                                   aAccessHint )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        if( aQuerySet->right != NULL )
        {
            IDE_TEST( findQuerySet4ViewIndexHints( aStatement,
                                                   aQuerySet->right,
                                                   aAccessHint )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgSelection::findBaseTableNSetIndexHint( qcStatement         * aStatement,
                                          qmsFrom             * aFrom,
                                          qmsTableAccessHints * aAccessHint )
{
/***********************************************************************
 *
 * Description : VIEW에 대한 index hint 적용
 *
 * Implementation : PROJ-1495
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgSelection::findBaseTableNSetIndexHint::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aAccessHint != NULL );

    //---------------------------------------------------
    // left From에 대한 처리
    //---------------------------------------------------

    if( aFrom->left->joinType == QMS_NO_JOIN )
    {
        if( aFrom->left->tableRef->view == NULL )
        {
            IDE_TEST( setViewIndexHintInBaseTable( aStatement,
                                                   aFrom->left,
                                                   aAccessHint )
                      != IDE_SUCCESS );
        }
        else
        {
            // VIEW인 경우로,
            // index hint 적용대상이 아님.
        }
    }
    else
    {
        IDE_TEST( findBaseTableNSetIndexHint( aStatement,
                                              aFrom->left,
                                              aAccessHint )
                  != IDE_SUCCESS );
    }

    //---------------------------------------------------
    // right From에 대한 처리
    //---------------------------------------------------

    if( aFrom->right->joinType == QMS_NO_JOIN )
    {
        if( aFrom->right->tableRef->view == NULL )
        {
            IDE_TEST( setViewIndexHintInBaseTable( aStatement,
                                                   aFrom->right,
                                                   aAccessHint )
                      != IDE_SUCCESS );
        }
        else
        {
            // VIEW인 경우로,
            // index hint 적용대상이 아님.
        }
    }
    else
    {
        IDE_TEST( findBaseTableNSetIndexHint( aStatement,
                                              aFrom->right,
                                              aAccessHint )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgSelection::setViewIndexHintInBaseTable( qcStatement * aStatement,
                                           qmsFrom     * aFrom,
                                           qmsTableAccessHints * aAccessHint )
{
/***********************************************************************
 *
 * Description : VIEW에 대한 index hint 적용
 *
 * Implementation : PROJ-1495
 *
 *  view내부의 각 querySet의 base table로
 *  view index hint에 해당하는 index를 찾아서 TableAccess hint를 만든다.
 *
 *     1. hint index name에 해당하는 index가 존재하는지 검사
 *     2. 존재하면,
 *        indexHint정보를 만들어서 base table에 연결한다.
 *
 ***********************************************************************/

    qmsTableAccessHints   * sTableAccessHint;
    qmsTableAccessHints   * sCurAccess;
    qmsHintTables         * sHintTable;
    qmsHintIndices        * sHintIndex;
    qmsHintIndices        * sHintIndices = NULL;
    qmsHintIndices        * sLastHintIndex;
    qmsHintIndices        * sNewHintIndex;
    qcmIndex              * sIndex;
    UInt                    sCnt;

    IDU_FIT_POINT_FATAL( "qmgSelection::setViewIndexHintInBaseTable::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aAccessHint != NULL );

    //---------------------------------------------------
    // hint index name에 해당하는 index가 존재하는지를 검사해서
    // 존재하면, base table에 연결할 indexHint정보를 만든다.
    //---------------------------------------------------

    for( sHintIndex = aAccessHint->indices;
         sHintIndex != NULL;
         sHintIndex = sHintIndex->next )
    {
        for( sCnt = 0; sCnt < aFrom->tableRef->tableInfo->indexCount; sCnt++ )
        {
            sIndex = &(aFrom->tableRef->tableInfo->indices[sCnt]);

            if( idlOS::strMatch(
                    sHintIndex->indexName.stmtText + sHintIndex->indexName.offset,
                    sHintIndex->indexName.size,
                    sIndex->name,
                    idlOS::strlen(sIndex->name) ) == 0 )
            {
                IDE_TEST(
                    QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsHintIndices),
                                                   (void **)&sNewHintIndex )
                    != IDE_SUCCESS );

                idlOS::memcpy( &(sNewHintIndex->indexName),
                               &(sHintIndex->indexName),
                               ID_SIZEOF(qcNamePosition) );

                sNewHintIndex->indexPtr = sIndex;
                sNewHintIndex->next = NULL;

                if( sHintIndices == NULL )
                {
                    sHintIndices = sNewHintIndex;
                    sLastHintIndex = sHintIndices;
                }
                else
                {
                    sLastHintIndex->next = sNewHintIndex;
                    sLastHintIndex = sLastHintIndex->next;
                }

                break;
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    //----------------------------------------------
    // VIEW index hint 중 해당 base table에 인덱스가 존재하는 경우
    // base table의 tableRef에 그 정보 설정
    //----------------------------------------------

    if( sHintIndices != NULL )
    {
        //--------------------------------------
        // qmsHintTable 정보 구성
        //--------------------------------------

        IDE_TEST(
            QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsHintTables),
                                           (void **)&sHintTable )
            != IDE_SUCCESS );

        idlOS::memcpy( &(sHintTable->userName),
                       &(aFrom->tableRef->userName),
                       ID_SIZEOF( qcNamePosition ) );
        idlOS::memcpy( &(sHintTable->tableName),
                       &(aFrom->tableRef->tableName),
                       ID_SIZEOF( qcNamePosition ) );
        sHintTable->table = aFrom;
        sHintTable->next  = NULL;

        //--------------------------------------
        // qmsTableAccessHints 정보 구성
        //--------------------------------------

        IDE_TEST(
            QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsTableAccessHints),
                                           (void **)&sTableAccessHint )
            != IDE_SUCCESS );

        idlOS::memcpy( sTableAccessHint,
                       aAccessHint,
                       ID_SIZEOF( qmsTableAccessHints ) );

        sTableAccessHint->table   = sHintTable;
        sTableAccessHint->indices = sHintIndices;
        sTableAccessHint->next    = NULL;

        //--------------------------------------
        // 해당 table의 tableRef에 tableAccessHint 정보 연결
        //--------------------------------------

        if( aFrom->tableRef->tableAccessHints == NULL )
        {
            aFrom->tableRef->tableAccessHints = sTableAccessHint;
        }
        else
        {
            // 이미 해당 table에 tableAccessHint가 있는 경우,
            // tableAccessHint연결리스트의 맨마지막에 연결.
            for( sCurAccess = aFrom->tableRef->tableAccessHints;
                 sCurAccess->next != NULL;
                 sCurAccess = sCurAccess->next ) ;

            sCurAccess->next = sTableAccessHint;
        }
    }
    else
    {
        // 해당 base table에 적용된 index가 없음.
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::doViewPushProjection( qcStatement  * aViewStatement,
                                    qmsTableRef  * aViewTableRef,
                                    qmsQuerySet  * aQuerySet )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1473 
 *     view에 대한 push projection을 수행한다.
 *
 * Implementation :
 *
 *     1. SET절은 union all만 가능
 *     2. distinct (X)
 *     3. group by (X)
 *     4. target Column에 aggr (X)
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgSelection::doViewPushProjection::__FT__" );

    if( aQuerySet->setOp == QMS_NONE )
    {
        if( ( aQuerySet->SFWGH->selectType == QMS_ALL ) &&
            ( aQuerySet->SFWGH->group == NULL ) &&
            ( aQuerySet->SFWGH->aggsDepth1 == NULL ) )
        {
            IDE_TEST( doPushProjection( aViewStatement,
                                        aViewTableRef,
                                        aQuerySet )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do 
        }
    }
    else
    {
        if( aQuerySet->setOp == QMS_UNION_ALL )
        {
            IDE_TEST( doViewPushProjection( aViewStatement,
                                            aViewTableRef,
                                            aQuerySet->left )
                      != IDE_SUCCESS );

            IDE_TEST( doViewPushProjection( aViewStatement,
                                            aViewTableRef,
                                            aQuerySet->right )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do 
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::doPushProjection( qcStatement  * aViewStatement,
                                qmsTableRef  * aViewTableRef,
                                qmsQuerySet  * aQuerySet )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1473
 *     view에 대한 push projection을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcTemplate    * sMtcTemplate;
    mtcTuple       * sViewTuple;
    qmsTarget      * sTarget;
    UShort           sColumn;

    IDU_FIT_POINT_FATAL( "qmgSelection::doPushProjection::__FT__" );

    sMtcTemplate = &(QC_SHARED_TMPLATE(aViewStatement)->tmplate);
    sViewTuple   = &(sMtcTemplate->rows[aViewTableRef->table]);

    for( sColumn = 0, sTarget = aQuerySet->target;
         ( sColumn < sViewTuple->columnCount ) && ( sTarget != NULL );
         sColumn++, sTarget = sTarget->next )
    {
        if( ( sViewTuple->columns[sColumn].flag
              & MTC_COLUMN_USE_COLUMN_MASK )
            == MTC_COLUMN_USE_COLUMN_TRUE )
        {
            // fix BUG-20939
            if( ( QTC_IS_COLUMN( aViewStatement, sTarget->targetColumn ) 
                  == ID_TRUE ) )
            {
                if( ( sMtcTemplate->rows[sTarget->targetColumn->node.table].lflag
                      & MTC_TUPLE_MATERIALIZE_MASK )
                    == MTC_TUPLE_MATERIALIZE_VALUE )
                {
                    // view내부로 push projection
                    sMtcTemplate->rows[sTarget->targetColumn->node.table].lflag
                        &= ~MTC_TUPLE_VIEW_PUSH_PROJ_MASK;
                    sMtcTemplate->rows[sTarget->targetColumn->node.table].lflag
                        |= MTC_TUPLE_VIEW_PUSH_PROJ_TRUE;

                    sMtcTemplate->rows[sTarget->targetColumn->node.table].
                        columns[sTarget->targetColumn->node.column].flag
                        &= ~MTC_COLUMN_VIEW_COLUMN_PUSH_MASK;
                    sMtcTemplate->rows[sTarget->targetColumn->node.table].
                        columns[sTarget->targetColumn->node.column].flag
                        |= MTC_COLUMN_VIEW_COLUMN_PUSH_TRUE;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // fix BUG-20939
                setViewPushProjMask( aViewStatement,
                                     sTarget->targetColumn );
            }
        }
        else
        {
            // Nothing To Do
        }
    }

    return IDE_SUCCESS;
}

void
qmgSelection::setViewPushProjMask( qcStatement * aStatement,
                                   qtcNode     * aNode )
{
    qtcNode * sNode;

    for( sNode = (qtcNode*)aNode->node.arguments;
         sNode != NULL;
         sNode = (qtcNode*)sNode->node.next )
    {
        if( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
        {
            if( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNode->node.table].lflag
                  & MTC_TUPLE_MATERIALIZE_MASK )
                == MTC_TUPLE_MATERIALIZE_VALUE )
            {
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNode->node.table].lflag
                    &= ~MTC_TUPLE_VIEW_PUSH_PROJ_MASK;
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNode->node.table].lflag
                    |= MTC_TUPLE_VIEW_PUSH_PROJ_TRUE;

                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNode->node.table].
                    columns[sNode->node.column].flag
                    &= ~MTC_COLUMN_VIEW_COLUMN_PUSH_MASK;
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNode->node.table].
                    columns[sNode->node.column].flag
                    |= MTC_COLUMN_VIEW_COLUMN_PUSH_TRUE;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            setViewPushProjMask( aStatement, sNode );
        }
    }
}

IDE_RC
qmgSelection::doViewPushSelection( qcStatement  * aStatement,
                                   qmsTableRef  * aTableRef,
                                   qmoPredicate * aPredicate,
                                   qmgGraph     * aGraph,
                                   idBool       * aIsPushed )
{
/***********************************************************************
 *
 * Description :
 *     BUG-18367 view push selection
 *     view에 대한 push selection을 수행한다.
 *
 * Implementation :
 *     view에 대한 다음 두 가지 형태를 처리한다.
 *     (1) view에 대한 one table predicate
 *     (2) PUSH_PRED 힌트로 내려온 view에 대한 join predicate
 *
 ***********************************************************************/

    qcStatement   * sView;
    qmsParseTree  * sViewParseTree;
    qmoPredicate  * sPredicate;
    qmoPredicate  * sRemainPredicate = NULL;
    qmoPredicate  * sLast;
    UShort          sViewTupleId;
    UInt            sColumnID;
    idBool          sIsIndexable;
    idBool          sIsPushed;         // view의 하위 set절에 하나라도 내려감
    idBool          sIsPushedAll;      // view의 하위 set절에 모두 내려감
    idBool          sRemainPushedPredicate;
    idBool          sRemovePredicate;

    /* TASK-7219 Non-shard DML */
    idBool          sIsPushedForce  = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmgSelection::doViewPushSelection::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aIsPushed != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sView = aTableRef->view;
    sViewTupleId = aTableRef->table;
    sViewParseTree = (qmsParseTree*) sView->myPlan->parseTree;

    for ( sPredicate = aPredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        sIsPushed              = ID_FALSE;
        sIsPushedAll           = ID_TRUE;
        sRemainPushedPredicate = ID_FALSE;

        /* TASK-7219 Non-shard DML */
        sIsPushedForce         = ID_FALSE;

        if ( (sPredicate->flag & QMO_PRED_PUSH_PRED_HINT_MASK)
             == QMO_PRED_PUSH_PRED_HINT_TRUE )
        {
            //---------------------------------------------------
            // PUSH_PRED 힌트에 의해 내려온 join predicate인 경우
            //---------------------------------------------------

            if ( ( sPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
                 != QTC_NODE_SUBQUERY_EXIST )
            {
                //---------------------------------------------------
                // 각 Predicate이 Subquery가 아닌 경우
                //---------------------------------------------------

                IDE_TEST( qmoPred::isIndexable( aStatement,
                                                sPredicate,
                                                & aGraph->myFrom->depInfo,
                                                & qtc::zeroDependencies,
                                                & sIsIndexable )
                          != IDE_SUCCESS );

                if ( sIsIndexable == ID_TRUE )
                {
                    //---------------------------------------------------
                    // Indexable 한 경우
                    //---------------------------------------------------

                    IDE_TEST( qmoPred::getColumnID( aStatement,
                                                    sPredicate->node,
                                                    ID_TRUE,
                                                    & sColumnID )
                              != IDE_SUCCESS );

                    if ( sColumnID != QMO_COLUMNID_LIST )
                    {
                        //---------------------------------------------------
                        // Indexable Predicate이 List가 아닌 경우
                        //---------------------------------------------------

                        IDE_TEST( qmoPushPred::doPushDownViewPredicate(
                                      aStatement,
                                      sViewParseTree,
                                      sViewParseTree->querySet,
                                      sViewTupleId,
                                      aGraph->myQuerySet->SFWGH,
                                      aGraph->myFrom,
                                      sPredicate,
                                      & sIsPushed,
                                      & sIsPushedAll,
                                      & sRemainPushedPredicate )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // List인 경우, Push Selection 하지 않음 : 추후 확장
                    }
                }
                else
                {
                    // indexable predicate이 아닌 경우
                    // nothing to do
                }
            }
            else
            {
                // Subquery인 경우
                // nothing to do
            }
        }
        else
        {
            //---------------------------------------------------
            // view의 one table predicate인 경우
            //---------------------------------------------------

            if ( ( sPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
                 != QTC_NODE_SUBQUERY_EXIST )
            {
                //---------------------------------------------------
                // 각 Predicate이 Subquery가 아닌 경우
                //---------------------------------------------------

                if ( qtc::getPosNextBitSet( & sPredicate->node->depInfo,
                                            qtc::getPosFirstBitSet(
                                                & sPredicate->node->depInfo ) )
                     == QTC_DEPENDENCIES_END )
                {
                    //---------------------------------------------------
                    // 각 Predicate이 외부참조 컬럼이 없는 경우
                    //---------------------------------------------------

                    IDE_TEST( qmoPushPred::doPushDownViewPredicate(
                                  aStatement,
                                  sViewParseTree,
                                  sViewParseTree->querySet,
                                  sViewTupleId,
                                  aGraph->myQuerySet->SFWGH,
                                  aGraph->myFrom,
                                  sPredicate,
                                  & sIsPushed,
                                  & sIsPushedAll,
                                  & sRemainPushedPredicate )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* TASK-7219 Non-shard DML */
                    // 외부참조 컬럼을 가지는 경우, property 에 따라 동작
                    if ( ( SDU_SHARD_TRANSFORM_MODE & SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_MASK  )
                         == SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_ENABLE )
                    {
                        IDE_TEST( qmoPred::getColumnID( aStatement,
                                                        sPredicate->node,
                                                        ID_TRUE,
                                                        & sColumnID )
                                  != IDE_SUCCESS );

                        if ( sColumnID != QMO_COLUMNID_LIST )
                        {
                            IDE_TEST( qmoPushPred::doPushDownViewPredicate(
                                          aStatement,
                                          sViewParseTree,
                                          sViewParseTree->querySet,
                                          sViewTupleId,
                                          aGraph->myQuerySet->SFWGH,
                                          aGraph->myFrom,
                                          sPredicate,
                                          & sIsPushed,
                                          & sIsPushedAll,
                                          & sRemainPushedPredicate )
                                      != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                // Subquery인 경우
                // nothing to do
            }
        }

        /* TASK-7219 Non-shard DML */
        if ( ( SDU_SHARD_TRANSFORM_MODE & SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_MASK  )
             == SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_ENABLE )
        {
            /* TASK-7219 Non-shard DML */
            isForcePushedPredForShardView( &sPredicate->node->node,
                                           &sIsPushedForce );

            /* TASK-7219 Non-shard DML */
            if ( ( sPredicate->flag & QMO_PRED_PUSHED_FORCE_PRED_MASK )
                 == QMO_PRED_PUSHED_FORCE_PRED_TRUE )
            {
                sIsPushed = ID_FALSE;
            }
            else
            {
                /* Nothing to do. */
            }

        }
        else
        {
            /* Nothing to do. */
        }

        // fix BUG-12515
        // view의 레코드갯수는 push selection된
        // predicate의 selectivity로 계산되므로
        // where절에 남아 있는 predicate의 selectivity가
        // 추가적으로 계산되지 않도록 하기 위해
        if( sIsPushed == ID_TRUE )
        {
            *aIsPushed = ID_TRUE;

            sPredicate->flag &= ~QMO_PRED_PUSH_REMAIN_MASK;
            sPredicate->flag |= QMO_PRED_PUSH_REMAIN_TRUE;
        }
        else
        {
            sPredicate->flag &= ~QMO_PRED_PUSH_REMAIN_MASK;
            sPredicate->flag |= QMO_PRED_PUSH_REMAIN_FALSE;
        }

        if ( ( ( sPredicate->flag & QMO_PRED_TRANS_PRED_MASK )
               == QMO_PRED_TRANS_PRED_TRUE ) ||
             ( sIsPushedForce == ID_TRUE ) )
        {
            //---------------------------------------------------
            // TRANSITIVE PREDICATE 최적화에 의해 생성된 predicate을
            // predicate list에서 제거한다.
            // PROJ-1404
            // ( 남겨둘 경우 filter로 처리되며 이는 bad transitive predicate 이므로 )
            //---------------------------------------------------
            sRemovePredicate = ID_TRUE;
        }
        else
        {
            //---------------------------------------------------
            // Pushed Predicate을
            // 해당 predicate을 predicate list에서 제거한다. ( BUG-18367 )
            // 
            // < 제외 상황 : 즉, predicate을 남겨두어야 하는 경우 >
            // (1) 특정 구문 특성 때문에 pushed predicate을 삭제하면
            //     결과가 틀린 경우
            // (2) view의 set절 중 일부에만 push predicate된 경우
            //---------------------------------------------------
            
            if ( sIsPushed == ID_TRUE )
            {
                if ( sIsPushedAll == ID_TRUE )
                {
                    if ( sRemainPushedPredicate == ID_TRUE )
                    {
                        // 특정 구문은 그 특성 때문에 모두 push 되어도
                        // predicate을 삭제하면 그 결과가 틀림
                        // 이 정보는 doPushDownViewPredicate()에서 설정해줌
                        sRemovePredicate = ID_FALSE;
                    }
                    else
                    {
                        // view의 모든 queryset에 push predicate 했으므로
                        // predicate을 삭제시켜도됨
                        sRemovePredicate = ID_TRUE;
                    }
                }
                else
                {
                    // 일부만 push 되었으므로 predicate을 남겨두어야 함
                    sRemovePredicate = ID_FALSE;
                }
            }
            else
            {
                // Push 되지 않았으므로 predicate을 남겨두어야 함
                sRemovePredicate = ID_FALSE;
            }
        }

        if ( sRemovePredicate == ID_TRUE )
        {
            // 연결관계 제거
        }
        else
        {
            // 연결관계 유지
            if( sRemainPredicate == NULL )
            {
                sRemainPredicate = sPredicate;
                sLast = sRemainPredicate;
            }
            else
            {
                sLast->next = sPredicate;
                sLast = sLast->next;
            }
        }
    } // for
    
    if( sRemainPredicate == NULL )
    {
        // Nothing To Do
    }
    else
    {
        sLast->next = NULL;

        /* BUG-47752 left outer join의 on절이 OR 조건으로 구성되고있고
         * 오른쪽 view에 aggregaion이 존재할경우 fatal
         */
        if ( ( ( aGraph->flag & QMG_JOIN_RIGHT_MASK )
               == QMG_JOIN_RIGHT_TRUE ) &&
             ( sIsPushed == ID_TRUE ) )
        {
            aGraph->flag &= ~QMG_VIEW_PUSH_MASK;
            aGraph->flag |= QMG_VIEW_PUSH_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    aGraph->myPredicate = sRemainPredicate;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::makePreservedOrder( qcStatement        * aStatement,
                                  qmoIdxCardInfo     * aIdxCardInfo,
                                  UShort               aTable,
                                  qmgPreservedOrder ** aPreservedOrder)
{
/***********************************************************************
 *
 * Description : Preserved Order의 생성
 *
 * Implementation :
 *    (1) direction hint에 따른 첫번째 칼럼 direction 설정 및 forward 여부 설정
 *        - direction hint가 없는 경우 : 첫번째 칼럼 direction은 not defined
 *        - direction hint가 있는 경우 : 첫번째 칼럼 direction은 hint를 따름
 *                                     & forward 또는 backward로 읽을지 결정
 *    (2) Index의 각 칼럼에 대하여 preserved order 생성 및 정보 설정
 *
 ***********************************************************************/

    mtcColumn         * sKeyCols;
    UInt                sKeyColCnt;
    UInt              * sKeyColsFlag;
    qmgPreservedOrder * sPreservedOrder;
    qmgPreservedOrder * sFirst;
    qmgPreservedOrder * sCurrent;
    qmgDirectionType    sFirstDirection;
    idBool              sIsForward;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmgSelection::makePreservedOrder::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aIdxCardInfo != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sFirst       = NULL;
    sCurrent     = NULL;
    sKeyCols     = aIdxCardInfo->index->keyColumns;
    sKeyColCnt   = aIdxCardInfo->index->keyColCount;
    sKeyColsFlag = aIdxCardInfo->index->keyColsFlag;

    //---------------------------------------------------
    //  direction hint에 따른 첫번째 칼럼 direction 및 Forward 여부 설정
    //---------------------------------------------------

    switch( aIdxCardInfo->flag & QMO_STAT_CARD_IDX_HINT_MASK )
    {
        case  QMO_STAT_CARD_IDX_HINT_NONE :
        case  QMO_STAT_CARD_IDX_INDEX :
            sFirstDirection = QMG_DIRECTION_NOT_DEFINED;
            break;
        case  QMO_STAT_CARD_IDX_INDEX_ASC :
            if ( ( sKeyColsFlag[0] & SMI_COLUMN_ORDER_MASK ) ==
                 SMI_COLUMN_ORDER_ASCENDING )
            {
                sIsForward = ID_TRUE;
            }
            else
            {
                sIsForward = ID_FALSE;
            }
            sFirstDirection = QMG_DIRECTION_ASC;
            break;
        case  QMO_STAT_CARD_IDX_INDEX_DESC :
            if ( ( sKeyColsFlag[0] & SMI_COLUMN_ORDER_MASK ) ==
                 SMI_COLUMN_ORDER_ASCENDING )
            {
                sIsForward = ID_FALSE;
            }
            else
            {
                sIsForward = ID_TRUE;
            }
            sFirstDirection = QMG_DIRECTION_DESC;
            break;
        default :
            IDE_DASSERT( 0 );
            sFirstDirection = QMG_DIRECTION_NOT_DEFINED;
            break;
    }

    //---------------------------------------------------
    // Index의 각 칼럼에 대하여 preserved order 생성 및 정보 설정
    //---------------------------------------------------

    for ( i = 0; i < sKeyColCnt; i++ )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                                 (void **) & sPreservedOrder )
                  != IDE_SUCCESS );

        sPreservedOrder->table = aTable;
        sPreservedOrder->column =
            sKeyCols[i].column.id % SMI_COLUMN_ID_MAXIMUM;
        sPreservedOrder->next = NULL;

        if ( sFirst == NULL )
        {
            // direction 설정
            sPreservedOrder->direction = sFirstDirection;
            // preserved order 연결
            sFirst = sCurrent = sPreservedOrder;
        }
        else
        {
            //---------------------------------------------------
            // 방향 설정
            //---------------------------------------------------

            if ( sFirstDirection == QMG_DIRECTION_NOT_DEFINED )
            {
                //---------------------------------------------------
                // 방향이 Not Defined 인 경우
                //---------------------------------------------------

                if ( ( sKeyColsFlag[i-1] & SMI_COLUMN_ORDER_MASK ) ==
                     ( sKeyColsFlag[i] & SMI_COLUMN_ORDER_MASK ) )
                {
                    sPreservedOrder->direction = QMG_DIRECTION_SAME_WITH_PREV;
                }
                else
                {
                    sPreservedOrder->direction = QMG_DIRECTION_DIFF_WITH_PREV;
                }
            }
            else
            {
                //---------------------------------------------------
                // 방향이 정해진 경우
                //---------------------------------------------------

                if ( sIsForward == ID_TRUE )
                {
                    if ( ( sKeyColsFlag[i] & SMI_COLUMN_ORDER_MASK )
                         == SMI_COLUMN_ORDER_ASCENDING )
                    {
                        sPreservedOrder->direction = QMG_DIRECTION_ASC;
                    }
                    else
                    {
                        sPreservedOrder->direction = QMG_DIRECTION_DESC;
                    }
                }
                else
                {
                    if ( ( sKeyColsFlag[i] & SMI_COLUMN_ORDER_MASK )
                         == SMI_COLUMN_ORDER_ASCENDING )
                    {
                        sPreservedOrder->direction = QMG_DIRECTION_DESC;
                    }
                    else
                    {
                        sPreservedOrder->direction = QMG_DIRECTION_ASC;
                    }
                }
            }

            sCurrent->next = sPreservedOrder;
            sCurrent = sCurrent->next;
        }
    }

    *aPreservedOrder = sFirst;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::getBestAccessMethodInExecutionTime(
    qcStatement      * aStatement,
    qmgGraph         * aGraph,
    qmoStatistics    * aStatInfo,
    qmoPredicate     * aPredicate,
    qmoAccessMethod  * aAccessMethod,
    qmoAccessMethod ** aSelectedAccessMethod )
{
    qmoAccessMethod     * sAccessMethod;
    qmoIdxCardInfo      * sIdxCardInfo;
    UInt                  sIdxCnt;
    UInt                  i;
    UInt                  sAccessMethodCnt;
    qmoIdxCardInfo      * sCurIdxCardInfo;
    idBool                sIsMemory = ID_FALSE;
    qmoSystemStatistics * sSysStat; // BUG-36958

    IDU_FIT_POINT_FATAL( "qmgSelection::getBestAccessMethodInExecutionTime::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aAccessMethod != NULL );

    sAccessMethod    = aAccessMethod;
    sAccessMethodCnt = 0;

    // PROJ-1436
    // execution time에는 index card info를 복사해서 사용하여
    // shared plan을 변경시키지 않게 한다.
    sIdxCnt = aStatInfo->indexCnt;

    // execution time이므로 qmxMem을 사용한다.
    IDE_TEST( QC_QMX_MEM(aStatement)->alloc(
                  ID_SIZEOF(qmoIdxCardInfo) * sIdxCnt,
                  (void**) & sIdxCardInfo )
              != IDE_SUCCESS );

    idlOS::memcpy( (void*) sIdxCardInfo,
                   (void*) aStatInfo->idxCardInfo,
                   ID_SIZEOF(qmoIdxCardInfo) * sIdxCnt );
    
    // BUG-36958
    // reprepare 시 qmeMem 이 해제되면서 cost 계산에 필요한 system 통계정보를 얻을 수 없다.
    // 따라서 execution time 에 qmxMem 을 사용하여 system 통계정보를 다시 얻어온다.

    IDE_TEST( QC_QMX_MEM(aStatement)->alloc(
                  ID_SIZEOF(qmoSystemStatistics),
                  (void**) & sSysStat)
              != IDE_SUCCESS );

    aStatement->mSysStat = sSysStat;

    if( aGraph->myQuerySet->SFWGH->hints->optGoalType != QMO_OPT_GOAL_TYPE_RULE )
    {
        IDE_TEST( qmoStat::getSystemStatistics( aStatement->mSysStat ) != IDE_SUCCESS );
    }
    else
    {
        qmoStat::getSystemStatistics4Rule( aStatement->mSysStat );
    }

    //---------------------------------------------------
    // Access Method의 구축 단계
    //---------------------------------------------------

    if( ( aGraph->flag & QMG_GRAPH_TYPE_MASK ) == QMG_GRAPH_TYPE_DISK )
    {
        // Nothing To Do
    }
    else
    {
        sIsMemory = ID_TRUE;
    }

    // FULL SCAN Access Method 설정
    setFullScanMethod( aStatement,
                       aStatInfo,
                       aPredicate,
                       &sAccessMethod[sAccessMethodCnt],
                       1,
                       ID_TRUE ); // execution time

    sAccessMethodCnt++;

    // INDEX SCAN Access Method 설정
    for ( i = 0; i < sIdxCnt; i++ )
    {
        sCurIdxCardInfo = & sIdxCardInfo[i];

        if ( ( sCurIdxCardInfo->flag & QMO_STAT_CARD_IDX_HINT_MASK )
             != QMO_STAT_CARD_IDX_NO_INDEX )
        {
            if ( sCurIdxCardInfo->index->isOnlineTBS == ID_TRUE )
            {
                IDE_TEST( setIndexScanMethod( aStatement,
                                              aGraph,
                                              aStatInfo,
                                              sCurIdxCardInfo,
                                              aPredicate,
                                              &sAccessMethod[sAccessMethodCnt],
                                              sIsMemory,
                                              ID_TRUE ) // execution time
                          != IDE_SUCCESS );

                sAccessMethodCnt++;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // NO INDEX Hint가 있는 경우, 구축하지 않음
        }
    }

    IDE_TEST( selectBestMethod( aStatement,
                                aGraph->myFrom->tableRef->tableInfo,
                                &sAccessMethod[0],
                                sAccessMethodCnt,
                                aSelectedAccessMethod )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmgSelection::getBestAccessMethod(qcStatement      *aStatement,
                                         qmgGraph         *aGraph,
                                         qmoStatistics    *aStatInfo,
                                         qmoPredicate     *aPredicate,
                                         qmoAccessMethod  *aAccessMethod,
                                         qmoAccessMethod **aSelectedAccessMethod,
                                         UInt             *aAccessMethodCnt,
                                         qcmIndex        **aSelectedIndex,
                                         UInt             *aSelectedScanHint,
                                         UInt              aParallelDegree,
                                         UInt              aFlag )
{
/***********************************************************************
 *
 * Description : AccessMethod 의 설정 및 cost 비교 후 가장 좋은
 *               AccessMethod를 선택
 *
 * Implementation :
 *    (1) Access Method 정보 구축
 *    (2) 구축된 Access Method에서 가장 cost가 좋은 access method 선택
 *
 ***********************************************************************/

    qmoAccessMethod     * sAccessMethod;
    qmsTableAccessHints * sAccessHint;
    qmoIdxCardInfo      * sIdxCardInfo;
    UInt                  sIdxCnt;
    qmoIdxCardInfo      * sSelectedIdxInfo = NULL;
    qmoIdxCardInfo      * sCurIdxCardInfo;
    qmsHintIndices      * sHintIndex;
    idBool                sUsableHint = ID_FALSE;
    idBool                sIsMemory = ID_FALSE;
    idBool                sIsFullScan = ID_FALSE;
    UInt                  i;

    IDU_FIT_POINT_FATAL( "qmgSelection::getBestAccessMethod::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aAccessMethod != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sAccessMethod        = aAccessMethod;
    sAccessHint          = aGraph->myFrom->tableRef->tableAccessHints;
    sIdxCardInfo         = aStatInfo->idxCardInfo;
    sIdxCnt              = aStatInfo->indexCnt;
    *aAccessMethodCnt    = 0;

    for( i = 0; i < sIdxCnt; i++ )
    {
        // fix BUG-12888
        // 이 flag는 graph생성시만 참조하기때문에,
        // clear해도 이미 생성된 graph에는 영향을 미치지 않음.
        sIdxCardInfo[i].flag = QMO_STAT_CLEAR;
    }

    //---------------------------------------------------
    // Access Method의 구축 단계
    //---------------------------------------------------

    if( ( aGraph->flag & QMG_GRAPH_TYPE_MASK ) == QMG_GRAPH_TYPE_DISK )
    {
        // Nothing To Do
    }
    else
    {
        sIsMemory = ID_TRUE;
    }

    for ( ; sAccessHint != NULL; sAccessHint = sAccessHint->next )
    {
        //---------------------------------------------------
        // Hint가 존재하는 경우, Hint에 따라 Access Method 설정
        //---------------------------------------------------

        if ( sAccessHint->accessType ==
             QMO_ACCESS_METHOD_TYPE_FULLACCESS_SCAN )
        {
            setFullScanMethod( aStatement,
                               aStatInfo,
                               aPredicate,
                               &sAccessMethod[*aAccessMethodCnt],
                               aParallelDegree,
                               ID_FALSE ); // prepare time
            sIsFullScan = ID_TRUE;
            (*aAccessMethodCnt)++;

            // To Fix PR-11937
            // Index Nested Loop Join 의 사용 가능성 여부를 판단하기 위하여
            // Full Scan Hint가 적용되었음을 설정한다.
            // Join Graph에서 이값의 설정 여부를 보고 판단하게 된다.
            aGraph->flag &= ~QMG_SELT_FULL_SCAN_HINT_MASK;
            aGraph->flag |= QMG_SELT_FULL_SCAN_HINT_TRUE;
        }
        else
        {
            //---------------------------------------------------
            // INDEX SCAN Hint 또는 NO INDEX SCAN Hint 인 경우,
            // 나열된 Index를 따라가면서 사용 가능한 index hint인 경우
            // access method를 설정한다.
            //---------------------------------------------------

            // BUG-43534 index ( table ) 힌트를 지원해야 합니다.
            if ( sAccessHint->indices == NULL )
            {
                for ( i = 0; i < sIdxCnt; i++ )
                {
                    IDE_TEST( qmg::usableIndexScanHint(
                                  sIdxCardInfo[i].index,
                                  sAccessHint->accessType,
                                  sIdxCardInfo,
                                  sIdxCnt,
                                  & sSelectedIdxInfo,
                                  & sUsableHint )
                              != IDE_SUCCESS );

                    if ( sUsableHint == ID_TRUE )
                    {
                        IDE_TEST( setIndexScanMethod(
                                      aStatement,
                                      aGraph,
                                      aStatInfo,
                                      sSelectedIdxInfo,
                                      aPredicate,
                                      &sAccessMethod[*aAccessMethodCnt],
                                      sIsMemory,
                                      ID_FALSE ) // prepare time
                                  != IDE_SUCCESS );

                        (*aAccessMethodCnt)++;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            }
            else
            {
                // nothing to do
            }

            for ( sHintIndex = sAccessHint->indices;
                  sHintIndex != NULL;
                  sHintIndex = sHintIndex->next )
            {
                IDE_TEST(
                    qmg::usableIndexScanHint( sHintIndex->indexPtr,
                                              sAccessHint->accessType,
                                              sIdxCardInfo,
                                              sIdxCnt,
                                              & sSelectedIdxInfo,
                                              & sUsableHint )
                    != IDE_SUCCESS );

                if ( sUsableHint == ID_TRUE )
                {
                    IDE_TEST( setIndexScanMethod( aStatement,
                                                  aGraph,
                                                  aStatInfo,
                                                  sSelectedIdxInfo,
                                                  aPredicate,
                                                  &sAccessMethod[*aAccessMethodCnt],
                                                  sIsMemory,
                                                  ID_FALSE ) // prepare time
                              != IDE_SUCCESS );

                    (*aAccessMethodCnt)++;
                }
                else
                {
                    // nothing to do
                }
            }
        }
    }

    if( *aAccessMethodCnt == 0 )
    {
        *aSelectedScanHint = QMG_NOT_USED_SCAN_HINT;
    }
    else if( *aAccessMethodCnt == 1 )
    {
        if( sAccessMethod[0].method == NULL )
        {
            //---------------------------------------------------
            // FULL SCAN Hint 만이 존재하는 경우,
            // - Preserved Order를 Never Preserved Order로 설정하기 위해
            //---------------------------------------------------
            *aSelectedScanHint = QMG_USED_ONLY_FULL_SCAN_HINT;
        }
        else
        {
            // hint에 의해서 index scan이 결정된 경우
            // 나중에 host 변수의 최적화를 시도하지 않기 위해
            *aSelectedScanHint = QMG_USED_SCAN_HINT;
        }
    }
    else
    {
        // BUG-13800
        // Scan hint가 여러개 주어졌을 경우
        // 이중에 하나가 선택될 것이기 때문에
        // 마찬가지로 host 최적화를 취소해야 한다.
        *aSelectedScanHint = QMG_USED_SCAN_HINT;
    }

    if ( *aAccessMethodCnt == 0 )
    {
        //---------------------------------------------------
        // (1) Hint가 존재하지 않는 경우
        // (2) Index Hint가 주어졌으나 해당 Hint를 모두 사용할 수 없는 경우
        // (3) No Index Hint 만이 존재할 경우
        //---------------------------------------------------

        // FULL SCAN Access Method 설정
        setFullScanMethod( aStatement,
                           aStatInfo,
                           aPredicate,
                           &sAccessMethod[*aAccessMethodCnt],
                           aParallelDegree,
                           ID_FALSE ); // prepare time
        sIsFullScan = ID_TRUE;
        (*aAccessMethodCnt)++;

        // INDEX SCAN Access Method 설정
        for ( i = 0; i < sIdxCnt; i++ )
        {
            sCurIdxCardInfo = & sIdxCardInfo[i];

            if ( ( sCurIdxCardInfo->flag & QMO_STAT_CARD_IDX_HINT_MASK )
                 != QMO_STAT_CARD_IDX_NO_INDEX )
            {
                if ( sCurIdxCardInfo->index->isOnlineTBS == ID_TRUE )
                {
                    IDE_TEST( setIndexScanMethod( aStatement,
                                                  aGraph,
                                                  aStatInfo,
                                                  sCurIdxCardInfo,
                                                  aPredicate,
                                                  &sAccessMethod[*aAccessMethodCnt],
                                                  sIsMemory,
                                                  ID_FALSE ) // prepare time
                              != IDE_SUCCESS );

                    (*aAccessMethodCnt)++;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // NO INDEX Hint가 있는 경우, 구축하지 않음
            }
        }
    }
    else
    {
        // nothing to do
    }

    /**
     * PROJ-2641 Hierarchy Query Index
     * Hierarchy query 의 경우 full scan보다 index scan이 유리하고
     * 개념상 index scan시 index 가 memory에 load되면 disk io가
     * 없다고 보고 full scan시에는 level에 따라 disk io가 전반적으로
     * 크게 증가하므로 full scan시 disk io 값을 증가시켜 준다.
     */
    if ( ( ( aFlag & QMG_BEST_ACCESS_METHOD_HIERARCHY_MASK )
           == QMG_BEST_ACCESS_METHOD_HIERARCHY_TRUE ) &&
         ( sIsFullScan == ID_TRUE ) )
    {
        sAccessMethod[0].diskCost   = sAccessMethod[0].diskCost * QMG_HIERARCHY_QUERY_DISK_IO_ADJUST_VALUE;
        sAccessMethod[0].totalCost  = sAccessMethod[0].accessCost + sAccessMethod[0].diskCost;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( selectBestMethod( aStatement,
                                aGraph->myFrom->tableRef->tableInfo,
                                &sAccessMethod[0],
                                *aAccessMethodCnt,
                                aSelectedAccessMethod )
              != IDE_SUCCESS );

    if ((*aSelectedAccessMethod)->method == NULL )
    {
        *aSelectedIndex = NULL;
    }
    else
    {
        *aSelectedIndex = (*aSelectedAccessMethod)->method->index;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmgSelection::setFullScanMethod( qcStatement      * aStatement,
                                      qmoStatistics    * aStatInfo,
                                      qmoPredicate     * aPredicate,
                                      qmoAccessMethod  * aAccessMethod,
                                      UInt               aParallelDegree,
                                      idBool             aInExecutionTime )
{
    SDouble sCost;
    SDouble sSelectivity;

    // filter selectivity
    qmoSelectivity::getTotalSelectivity4PredList( aStatement,
                                                  aPredicate,
                                                  &sSelectivity,
                                                  aInExecutionTime );

    // filter cost
    aAccessMethod->filterCost = qmoCost::getFilterCost(
        aStatement->mSysStat,
        aPredicate,
        aStatInfo->totalRecordCnt );

    sCost = qmoCost::getTableFullScanCost( aStatement->mSysStat,
                                           aStatInfo,
                                           aAccessMethod->filterCost,
                                           &(aAccessMethod->accessCost),
                                           &(aAccessMethod->diskCost) );

    aAccessMethod->method               = NULL;
    aAccessMethod->keyRangeSelectivity  = 1;
    aAccessMethod->keyFilterSelectivity = 1;
    aAccessMethod->filterSelectivity    = sSelectivity;
    aAccessMethod->methodSelectivity    = sSelectivity;
    aAccessMethod->keyRangeCost         = 0;
    aAccessMethod->keyFilterCost        = 0;

    // TASK-6699 TPC-H 성능 개선
    // ParallelDegree 를 고려하여 Full scan cost 를 계산한다.
    aAccessMethod->accessCost = aAccessMethod->accessCost / aParallelDegree;
    aAccessMethod->diskCost   = aAccessMethod->diskCost / aParallelDegree;
    aAccessMethod->totalCost  = sCost / aParallelDegree;

}

IDE_RC qmgSelection::setIndexScanMethod( qcStatement      * aStatement,
                                         qmgGraph         * aGraph,
                                         qmoStatistics    * aStatInfo,
                                         qmoIdxCardInfo   * aIdxCardInfo,
                                         qmoPredicate     * aPredicate,
                                         qmoAccessMethod  * aAccessMethod,
                                         idBool             aIsMemory,
                                         idBool             aInExecutionTime )
{
    SDouble            sCost;
    SDouble            sKeyRangeCost;
    SDouble            sKeyFilterCost;
    SDouble            sFilterCost;
    SDouble            sLoopCount;
    qcTemplate       * sTemplate;
    mtcColumn        * sColumns;
    qmoAccessMethod  * sAccessMethod = aAccessMethod;

    qmoPredWrapper   * sKeyRange     = NULL;
    qmoPredWrapper   * sKeyFilter    = NULL;
    qmoPredWrapper   * sFilter       = NULL;
    qmoPredWrapper   * sLobFilter    = NULL;
    qmoPredWrapper   * sSubQFilter   = NULL;
    qmoPredWrapper   * sWrapperIter;
    qmoPredWrapperPool sWrapperPool;

    SDouble            sKeyRangeSelectivity;
    SDouble            sKeyFilterSelectivity;
    SDouble            sFilterSelectivity;
    SDouble            sMethodSelectivity;
    UShort             sTempArr[SMI_COLUMN_ID_MAXIMUM];
    UInt               i;

    IDU_FIT_POINT_FATAL( "qmgSelection::setIndexScanMethod::__FT__" );

    if( aInExecutionTime == ID_TRUE )
    {
        sTemplate = QC_PRIVATE_TMPLATE(aStatement);
    }
    else
    {
        sTemplate = QC_SHARED_TMPLATE(aStatement);
    }
    sColumns = sTemplate->tmplate.rows[aGraph->myFrom->tableRef->table].columns;

    if( aPredicate != NULL )
    {
        //--------------------------------------
        // range, filter 추출
        //--------------------------------------
        IDE_TEST( qmoPred::extractRangeAndFilter( aStatement,
                                                  sTemplate,
                                                  aIsMemory,
                                                  aInExecutionTime,
                                                  aIdxCardInfo->index,
                                                  aPredicate,
                                                  &sKeyRange,
                                                  &sKeyFilter,
                                                  &sFilter,
                                                  &sLobFilter,
                                                  &sSubQFilter,
                                                  &sWrapperPool )
                  != IDE_SUCCESS );

        //--------------------------------------
        // range, filter selectivity
        //--------------------------------------
        IDE_TEST( qmoSelectivity::getSelectivity4KeyRange( sTemplate,
                                                           aStatInfo,
                                                           aIdxCardInfo,
                                                           sKeyRange,
                                                           &sKeyRangeSelectivity,
                                                           aInExecutionTime )
                  != IDE_SUCCESS );

        // fix BUG-42752
        if ( QCU_OPTIMIZER_ESTIMATE_KEY_FILTER_SELECTIVITY == 1 )
        {
            IDE_TEST( qmoSelectivity::getSelectivity4PredWrapper( sTemplate,
                                                                  sKeyFilter,
                                                                  &sKeyFilterSelectivity,
                                                                  aInExecutionTime )
                      != IDE_SUCCESS );
        }
        else
        {
            sKeyFilterSelectivity = 1;
        }

        IDE_TEST( qmoSelectivity::getSelectivity4PredWrapper( sTemplate,
                                                              sFilter,
                                                              &sFilterSelectivity,
                                                              aInExecutionTime )
                  != IDE_SUCCESS );

        //--------------------------------------
        // range, filter cost
        //--------------------------------------
        sKeyRangeCost  = qmoCost::getKeyRangeCost(  aStatement->mSysStat,
                                                    aStatInfo,
                                                    aIdxCardInfo,
                                                    sKeyRange,
                                                    sKeyRangeSelectivity );


        sLoopCount     = aStatInfo->totalRecordCnt * sKeyRangeSelectivity;

        sKeyFilterCost = qmoCost::getKeyFilterCost( aStatement->mSysStat,
                                                    sKeyFilter,
                                                    sLoopCount );


        sLoopCount     = sLoopCount * sKeyFilterSelectivity;

        sFilterCost    = qmoCost::getFilterCost4PredWrapper(
            aStatement->mSysStat,
            sFilter,
            sLoopCount );

        sFilterCost   += qmoCost::getFilterCost4PredWrapper(
            aStatement->mSysStat,
            sSubQFilter,
            sLoopCount );

        sMethodSelectivity = sKeyRangeSelectivity  *
            sKeyFilterSelectivity *
            sFilterSelectivity;
    }
    else
    {
        sKeyRangeCost          = 0;
        sKeyFilterCost         = 0;
        sFilterCost            = 0;

        sKeyRangeSelectivity   = QMO_SELECTIVITY_NOT_EXIST;
        sKeyFilterSelectivity  = QMO_SELECTIVITY_NOT_EXIST;
        sFilterSelectivity     = QMO_SELECTIVITY_NOT_EXIST;
        sMethodSelectivity     = QMO_SELECTIVITY_NOT_EXIST;
    }

    //--------------------------------------
    // Index cost
    //--------------------------------------
    sCost = qmoCost::getIndexScanCost( aStatement,
                                       sColumns,
                                       aPredicate,
                                       aStatInfo,
                                       aIdxCardInfo,
                                       sKeyRangeSelectivity,
                                       sKeyFilterSelectivity,
                                       sKeyRangeCost,
                                       sKeyFilterCost,
                                       sFilterCost,
                                       &(aAccessMethod->accessCost),
                                       &(aAccessMethod->diskCost) );

    //--------------------------------------
    // qmoAccessMethod 설정
    //--------------------------------------
    sAccessMethod->method               = aIdxCardInfo;

    // To Fix PR-9181
    // Index가 Key Range를 위한 Predicate을 가지고 있는 지
    // 가지고 있을 경우 IN SUBQUERY KEY RANGE를 사용하는 지를
    // 설정함.
    if( sKeyRange == NULL )
    {
        sAccessMethod->method->flag &= ~QMO_STAT_CARD_IDX_EXIST_PRED_MASK;
        sAccessMethod->method->flag |= QMO_STAT_CARD_IDX_EXIST_PRED_FALSE;

        sAccessMethod->method->flag &= ~QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK;
        sAccessMethod->method->flag |= QMO_STAT_CARD_IDX_IN_SUBQUERY_FALSE;

        /* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
        if ( (( QCU_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY == 0) ||
              ( QCU_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY == 2) ) &&
             (aGraph->myFrom->tableRef->tableInfo->primaryKey != NULL) )
        {
            if ( sAccessMethod->method->index->indexId
                 == aGraph->myFrom->tableRef->tableInfo->primaryKey->indexId )
            {
                // Primary Index임
                sAccessMethod->method->flag &= ~QMO_STAT_CARD_IDX_PRIMARY_MASK;
                sAccessMethod->method->flag |= QMO_STAT_CARD_IDX_PRIMARY_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        sAccessMethod->method->flag &= ~QMO_STAT_CARD_IDX_EXIST_PRED_MASK;
        sAccessMethod->method->flag |= QMO_STAT_CARD_IDX_EXIST_PRED_TRUE;

        // To Fix PR-9181
        // IN SUBQUERY KEYRANGE가 설정되어 있는 지를 설정함
        if ( ( sKeyRange->pred->flag & QMO_PRED_INSUBQUERY_MASK )
             == QMO_PRED_INSUBQUERY_ABSENT )
        {
            sAccessMethod->method->flag &= ~QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK;
            sAccessMethod->method->flag |= QMO_STAT_CARD_IDX_IN_SUBQUERY_FALSE;
        }
        else
        {
            sAccessMethod->method->flag &= ~QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK;
            sAccessMethod->method->flag |= QMO_STAT_CARD_IDX_IN_SUBQUERY_TRUE;
        }

        if ( aGraph->myFrom->tableRef->tableInfo->primaryKey != NULL )
        {
            if( sAccessMethod->method->index->indexId
                == aGraph->myFrom->tableRef->tableInfo->primaryKey->indexId )
            {
                // Primary Index임
                sAccessMethod->method->flag &= ~QMO_STAT_CARD_IDX_PRIMARY_MASK;
                sAccessMethod->method->flag |= QMO_STAT_CARD_IDX_PRIMARY_TRUE;
            }
        }
    }

    sAccessMethod->keyRangeSelectivity  = sKeyRangeSelectivity;
    sAccessMethod->keyFilterSelectivity = sKeyFilterSelectivity;
    sAccessMethod->filterSelectivity    = sFilterSelectivity;

    // BUG-48120 
    if ( sAccessMethod->method != NULL )
    {  
        if ( (sAccessMethod->method->flag & QMO_STAT_CARD_ALL_EQUAL_IDX_MASK)
             == QMO_STAT_CARD_ALL_EQUAL_IDX_TRUE )
        {
            // BUG-48120 output record를 계산할 때 사용하는 
            // methodSelectivity는index의 keyNDV 로 계산해야한다.
            // (__optimizer_scan_cost_mode 1이 포함된 경우)
            // 이 경우 key filter는 없다.
            sAccessMethod->methodSelectivity    = (1/aIdxCardInfo->KeyNDV) * sFilterSelectivity;
        }
        else
        {
            // BUG-48120 수정전과 동일
            sAccessMethod->methodSelectivity    = sMethodSelectivity;
        }
    }
    else
    {
        // full scan 인 경우 기존과 동일
        sAccessMethod->methodSelectivity    = sMethodSelectivity;
    }

    sAccessMethod->keyRangeCost         = sKeyRangeCost;
    sAccessMethod->keyFilterCost        = sKeyFilterCost;
    sAccessMethod->filterCost           = sFilterCost;
    sAccessMethod->totalCost            = sCost;

    /*
     * BUG-39298 improve preserved order
     *
     * create index .. on t1 (i1, i2, i3)
     * select .. from .. where t1.i1 = 1 and t1.i2 = 2 order by i3;
     *
     * 고정된 값을 갖는 column (i1, i2) 를 저장해둔다.
     * 후에 preserved order 비교시 i1, i2 를 제외하고 비교한다.
     */
    if ((sKeyRange != NULL) && (aInExecutionTime == ID_FALSE))
    {
        i = 0;
        for (sWrapperIter = sKeyRange;
             sWrapperIter != NULL;
             sWrapperIter = sWrapperIter->next)
        {
            if ((sWrapperIter->pred->flag & QMO_PRED_INDEXABLE_EQUAL_MASK) ==
                QMO_PRED_INDEXABLE_EQUAL_TRUE)
            {
                sTempArr[i] = (sWrapperIter->pred->id % SMI_COLUMN_ID_MAXIMUM);
                i++;
            }
        }

        if (i > 0)
        {
            IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                         ID_SIZEOF(UShort) * i,
                         (void**)&sAccessMethod->method->mKeyRangeColumn.mColumnArray)
                     != IDE_SUCCESS);

            sAccessMethod->method->mKeyRangeColumn.mColumnCount = i;

            for (i = 0; i < sAccessMethod->method->mKeyRangeColumn.mColumnCount; i++)
            {
                sAccessMethod->method->mKeyRangeColumn.mColumnArray[i] = sTempArr[i];
            }
        }
        else
        {
            sAccessMethod->method->mKeyRangeColumn.mColumnCount = 0;
            sAccessMethod->method->mKeyRangeColumn.mColumnArray = NULL;
        }
    }
    else
    {
        sAccessMethod->method->mKeyRangeColumn.mColumnCount = 0;
        sAccessMethod->method->mKeyRangeColumn.mColumnArray = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmgSelection::selectBestMethod( qcStatement      * aStatement,
                                       qcmTableInfo     * aTableInfo,
                                       qmoAccessMethod  * aAccessMethod,
                                       UInt               aAccessMethodCnt,
                                       qmoAccessMethod ** aSelected )
{
    UInt   i;
    idBool sIndexChange;
    UInt   sSelectedMethod = 0;

    IDU_FIT_POINT_FATAL( "qmgSelection::selectBestMethod::__FT__" );

    //---------------------------------------------------
    // Accees Method 선택 단계
    //   - cost가 가장 낮은 table access method를 선택
    //---------------------------------------------------

    // sSelected 초기화
    *aSelected = aAccessMethod;

    for ( i = 0; i < aAccessMethodCnt; i++ )
    {
        if (QMO_COST_IS_GREATER((*aSelected)->totalCost,
                                aAccessMethod[i].totalCost) == ID_TRUE)
        {
            //---------------------------------------------------
            // cost가 좋은 access method 선택
            //---------------------------------------------------

            // To Fix PR-8134
            // Cost 가 좋은 인덱스일 경우라 하더라도 실제 사용하는
            // Predicate이 존재하여야 함.
            if ( aAccessMethod[i].method != NULL )
            {
                if ( ( aAccessMethod[i].method->index->indexTypeId == SMI_ADDITIONAL_RTREE_INDEXTYPE_ID ) && 
                     ( ( aAccessMethod[i].method->flag & QMO_STAT_CARD_IDX_EXIST_PRED_MASK )
                       == QMO_STAT_CARD_IDX_EXIST_PRED_FALSE ) )
                {
                    // BUG-38082, BUG-40269
                    // predicate 이 존재하더라도 R-tree의 KeyRange를 가지고 있지 않은 predicate이라면
                    // index(R-tree) 선택을 배제한다. 그래서 predicate 유무에 관계없이 flag를 검사하도록 한다.

                    sIndexChange = ID_FALSE;
                }
                else
                {
                    // BUG-37861 / BUG-39666 메모리 테이블에서만 Predicate을 검사함
                    /* PROJ-2464 hybrid partitioned table 지원 */
                    if ( ( aTableInfo->tableFlag & SMI_TABLE_TYPE_MASK ) != SMI_TABLE_DISK )
                    {
                        if( (aAccessMethod[i].method->flag &
                             QMO_STAT_CARD_IDX_EXIST_PRED_MASK)
                            ==  QMO_STAT_CARD_IDX_EXIST_PRED_TRUE )
                        {
                            sIndexChange = ID_TRUE;
                        }
                        else
                        {
                            sIndexChange = ID_FALSE;
                        }
                    }
                    else
                    {
                        sIndexChange = ID_TRUE;
                    }
                }
            }
            else
            {
                sIndexChange = ID_FALSE;
            }
        }
        else if (QMO_COST_IS_EQUAL((*aSelected)->totalCost,
                                   aAccessMethod[i].totalCost) == ID_TRUE)
        {
            //---------------------------------------------------
            // cost가 동일한 경우
            //---------------------------------------------------

            sIndexChange = ID_FALSE;

            // Access Method를 변경할 것인지를 결정
            while (1)
            {
                //-------------------------------------------------
                // 현재 Method가 Full Scan이라면 변경하지 않음
                //-------------------------------------------------

                if (aAccessMethod[i].method == NULL)
                {
                    sIndexChange = ID_FALSE;
                    break;
                }
                else
                {
                    // Go Go
                }

                if ((aAccessMethod[i].method->flag & QMO_STAT_CARD_IDX_PRIMARY_MASK) ==
                    QMO_STAT_CARD_IDX_PRIMARY_TRUE)
                {
                    sIndexChange = ID_TRUE;
                    break;
                }
                else
                {
                    // Go Go
                }

                //----------------------------------------------------------------------------------------
                // BUG-38082, BUG-40269
                // predicate 이 존재하더라도 R-tree의 KeyRange를 가지고 있지 않은 predicate이라면
                // index(R-tree) 선택을 배제한다. 그래서 predicate 유무에 관계없이 flag를 검사하도록 한다.
                //----------------------------------------------------------------------------------------

                if ( ( aAccessMethod[i].method->index->indexTypeId == SMI_ADDITIONAL_RTREE_INDEXTYPE_ID ) && 
                     ( ( aAccessMethod[i].method->flag & QMO_STAT_CARD_IDX_EXIST_PRED_MASK )
                       == QMO_STAT_CARD_IDX_EXIST_PRED_FALSE ) )
                {
                    sIndexChange = ID_FALSE;
                    break;
                }
                else
                {
                    // Go Go
                }

                //-------------------------------------------------
                // 현재 Method가 Predicate이 없는
                // Index라면 변경하지 않음
                //-------------------------------------------------

                if ((aAccessMethod[i].method->flag &
                     QMO_STAT_CARD_IDX_EXIST_PRED_MASK) !=
                    QMO_STAT_CARD_IDX_EXIST_PRED_TRUE)
                {
                    sIndexChange = ID_FALSE;
                    break;
                }
                else
                {
                    // Go Go
                }

                //-------------------------------------------------
                // 선택된 Method가 Full Scan이고
                // 현재 Method가 Predicate을 가진 Index라면
                // Method를 변경함.
                //-------------------------------------------------

                if ((*aSelected)->method == NULL)
                {
                    sIndexChange = ID_TRUE;
                    break;
                }
                else
                {
                    // Go Go
                }

                //-------------------------------------------------
                // 현재 Method는 Predicate이 있는 index이고
                // 선택된 Method는 Predicate이 없는 index일 때
                // Method 변경함.
                //------------------------------------------------

                if (((*aSelected)->method->flag &
                     QMO_STAT_CARD_IDX_EXIST_PRED_MASK)
                    == QMO_STAT_CARD_IDX_EXIST_PRED_FALSE)
                {
                    sIndexChange = ID_TRUE;
                    break;
                }
                else
                {
                    // Go Go
                }

                //-------------------------------------------------
                // 둘 모두 해당 index를 사용하는 predicate이 있는 경우,
                // keyFilterSelecitivity가 좋은 method를 선택
                //------------------------------------------------

                if ((*aSelected)->keyFilterSelectivity >
                    aAccessMethod[i].keyFilterSelectivity)
                {
                    sIndexChange = ID_TRUE;
                    break;
                }
                else
                {
                }

                /* BUG-48120 : index의 keyRangeSelectivity를 비교하기 전에 
                 * 현재 Method는ALL EQUAL INDEX 이고,
                 * 선택된 Method는 ALL EQUAL INDEX일 때 Method 변경함
                 * (__optimizer_scan_cost_mode 1이 포함된 경우) */
                if ( ( ((*aSelected)->method->flag & QMO_STAT_CARD_ALL_EQUAL_IDX_MASK)
                       == QMO_STAT_CARD_ALL_EQUAL_IDX_FALSE ) &&
                     ( (aAccessMethod[i].method->flag & QMO_STAT_CARD_ALL_EQUAL_IDX_MASK)
                       == QMO_STAT_CARD_ALL_EQUAL_IDX_TRUE ) )
                {
                    sIndexChange = ID_TRUE;
                    break;
                }

                /*
                 * BUG-30307: index 선택시 예상비용이 같을때
                 *            비효율적인 index 를 타는 경우가 있습니다
                 *
                 * 해당 index 의 selectivity 도 비교해본다
                 *
                 */
                if ((*aSelected)->keyRangeSelectivity > aAccessMethod[i].keyRangeSelectivity)
                {
                    sIndexChange = ID_TRUE;
                    break;
                }
                else
                {
                }

                break;
            } // while
        }
        else
        {
            sIndexChange = ID_FALSE;
        }

        // Access Method 변경
        if ( sIndexChange == ID_TRUE )
        {
            *aSelected = & aAccessMethod[i];
        }
        else
        {
            // Nothing to do.
        }
    } // for

    /* TASK-6744 */
    if ( (QCU_PLAN_RANDOM_SEED != 0) &&
         (aStatement->mRandomPlanInfo != NULL) &&
         (smiGetStartupPhase() == SMI_STARTUP_SERVICE) )
    {
        aStatement->mRandomPlanInfo->mTotalNumOfCases
            = aStatement->mRandomPlanInfo->mTotalNumOfCases + aAccessMethodCnt;
        sSelectedMethod =
            QCU_PLAN_RANDOM_SEED % (aStatement->mRandomPlanInfo->mTotalNumOfCases - aStatement->mRandomPlanInfo->mWeightedValue);

        if ( sSelectedMethod >= aAccessMethodCnt )
        {
            sSelectedMethod = (QCU_PLAN_RANDOM_SEED + sSelectedMethod) % aAccessMethodCnt;
        }
        else
        {
            // Nothing to do.
        }

        IDE_DASSERT( sSelectedMethod < aAccessMethodCnt ); 

        aStatement->mRandomPlanInfo->mWeightedValue++;

        *aSelected = & aAccessMethod[sSelectedMethod];

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_PLAN_RANDOM_SEED );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC
qmgSelection::copyPreservedOrderFromView( qcStatement        * aStatement,
                                          qmgGraph           * aChildGraph,
                                          UShort               aTable,
                                          qmgPreservedOrder ** aNewOrder )
{
/***********************************************************************
 *
 * Description : View의 preserved order를 Copy하여 table, column 변경
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgPreservedOrder * sPreservedOrder;
    qmgPreservedOrder * sViewOrder;
    qmgPreservedOrder * sCurOrder;
    qmgPreservedOrder * sFirstOrder;
    qmgPROJ           * sPROJ;
    qmsTarget         * sTarget;
    qtcNode           * sTargetNode;
    UShort              sPredPos;

    IDU_FIT_POINT_FATAL( "qmgSelection::copyPreservedOrderFromView::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sCurOrder   = NULL;
    sFirstOrder = NULL;
    sPROJ       = (qmgPROJ *)aChildGraph;
    sPredPos    = 0;

    sViewOrder  = aChildGraph->preservedOrder;
    for ( ; sViewOrder != NULL; sViewOrder = sViewOrder->next )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                                 (void**)&sPreservedOrder )
                  != IDE_SUCCESS );

        // table 변경
        sPreservedOrder->table = aTable;

        // column 변경
        for ( sTarget = sPROJ->target, sPredPos = 0;
              sTarget != NULL;
              sTarget = sTarget->next, sPredPos++ )
        {
            // BUG-38193 target의 pass node 를 고려해야 합니다.
            if ( sTarget->targetColumn->node.module == &qtc::passModule )
            {
                sTargetNode = (qtcNode*)(sTarget->targetColumn->node.arguments);
            }
            else
            {
                sTargetNode = sTarget->targetColumn;
            }

            if ( ( sViewOrder->table  == sTargetNode->node.table  ) &&
                 ( sViewOrder->column == sTargetNode->node.column ) )
            {
                break;
            }
            else
            {
                // nothing to do
            }
        }
        sPreservedOrder->column = sPredPos;

        sPreservedOrder->direction = sViewOrder->direction;
        sPreservedOrder->next = NULL;

        if ( sFirstOrder == NULL )
        {
            sFirstOrder = sCurOrder = sPreservedOrder;
        }
        else
        {
            sCurOrder->next = sPreservedOrder;
            sCurOrder = sCurOrder->next;
        }
    }

    *aNewOrder = sFirstOrder;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::copySELTAndAlterSelectedIndex( qcStatement * aStatement,
                                             qmgSELT     * aSource,
                                             qmgSELT    ** aTarget,
                                             qcmIndex    * aNewSelectedIndex,
                                             UInt          aWhichOne )
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

    IDU_FIT_POINT_FATAL( "qmgSelection::copySELTAndAlterSelectedIndex::__FT__" );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgSELT ), (void**)aTarget )
              != IDE_SUCCESS );

    idlOS::memcpy( *aTarget, aSource, ID_SIZEOF( qmgSELT ) );

    if( aWhichOne == 0 )
    {
        (*aTarget)->sdf = NULL;
        IDE_TEST( alterSelectedIndex( aStatement,
                                      *aTarget,
                                      aNewSelectedIndex )
                  != IDE_SUCCESS );
    }
    else if( aWhichOne == 1 )
    {
        if( aSource->sdf != NULL )
        {
            aSource->sdf->baseGraph = *aTarget;
            aSource->sdf = NULL;
        }
        else
        {
            // Nothing to do...
        }
        IDE_TEST( alterSelectedIndex( aStatement,
                                      aSource,
                                      aNewSelectedIndex )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::optimizePartition( qcStatement * aStatement, qmgGraph * aGraph )
{
    qmgSELT         * sMyGraph;
    qmsPartitionRef * sPartitionRef;
    qcmColumn       * sColumns;
    UInt              sColumnCnt;
    qmoPredicate    * sRidPredicate;

    UInt              sIndexCnt;
    UInt              sSelectedScanHint;

    SDouble           sOutputRecordCnt;
    SDouble           sRecordSize;

    UInt              i;

    IDU_FIT_POINT_FATAL( "qmgSelection::optimizePartition::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgSELT*) aGraph;
    sPartitionRef = sMyGraph->partitionRef;
    sRecordSize = 0;

    //---------------------------------------------------
    // 공통 비용 정보의 설정
    //---------------------------------------------------

    // recordSize 설정
    sColumns = sPartitionRef->partitionInfo->columns;
    sColumnCnt = sPartitionRef->partitionInfo->columnCount;

    for ( i = 0; i < sColumnCnt; i++ )
    {
        sRecordSize += sColumns[i].basicInfo->column.size;
    }
    // BUG-36463 sRecordSize 는 0이 되어서는 안된다.
    sRecordSize = IDL_MAX( sRecordSize, 1 );

    sMyGraph->graph.costInfo.recordSize = sRecordSize;

    // inputRecordCnt 설정
    sMyGraph->graph.costInfo.inputRecordCnt =
        sPartitionRef->statInfo->totalRecordCnt;

    //---------------------------------------------------
    // Predicate의 재배치 및 개별 Predicate의 Selectivity 계산
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST(
            qmoPred::relocatePredicate(
                aStatement,
                sMyGraph->graph.myPredicate,
                & sMyGraph->graph.depInfo,
                & sMyGraph->graph.myQuerySet->outerDepInfo,
                sPartitionRef->statInfo,
                & sMyGraph->graph.myPredicate )
            != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    sIndexCnt = sPartitionRef->partitionInfo->indexCount;
    sRidPredicate = sMyGraph->graph.ridPredicate;

    //---------------------------------------------------
    // accessMethod 선택
    //---------------------------------------------------

    if (sRidPredicate != NULL)
    {
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     ID_SIZEOF(qmoAccessMethod),
                     (void**)&sMyGraph->accessMethod)
                 != IDE_SUCCESS);

        sMyGraph->accessMethod->method               = NULL;
        sMyGraph->accessMethod->keyRangeSelectivity  = 0;
        sMyGraph->accessMethod->keyFilterSelectivity = 0;
        sMyGraph->accessMethod->filterSelectivity    = 0;
        sMyGraph->accessMethod->methodSelectivity    = 0;

        sMyGraph->accessMethod->totalCost = qmoCost::getTableRIDScanCost(
            sPartitionRef->statInfo,
            &sMyGraph->accessMethod->accessCost,
            &sMyGraph->accessMethod->diskCost );

        sMyGraph->selectedIndex   = NULL;
        sMyGraph->accessMethodCnt = 1;
        sMyGraph->selectedMethod  = &sMyGraph->accessMethod[0];
        sMyGraph->forceIndexScan  = ID_FALSE;
        sMyGraph->forceRidScan    = ID_FALSE;

        sSelectedScanHint = QMG_NOT_USED_SCAN_HINT;
    }
    else
    {
        sMyGraph->accessMethodCnt = sIndexCnt + 1;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     ID_SIZEOF(qmoAccessMethod) * (sIndexCnt + 1),
                     (void**) & sMyGraph->accessMethod)
                 != IDE_SUCCESS);

        IDE_TEST(getBestAccessMethod(aStatement,
                                     &sMyGraph->graph,
                                     sPartitionRef->statInfo,
                                     sMyGraph->graph.myPredicate,
                                     sMyGraph->accessMethod,
                                     &sMyGraph->selectedMethod,
                                     &sMyGraph->accessMethodCnt,
                                     &sMyGraph->selectedIndex,
                                     &sSelectedScanHint,
                                     1,
                                     0)
                 != IDE_SUCCESS);

        // To fix BUG-12742
        // hint를 사용한 경우는 index를 제거할 수 없다.
        if( sSelectedScanHint == QMG_USED_SCAN_HINT )
        {
            sMyGraph->forceIndexScan = ID_TRUE;
        }
        else
        {
            sMyGraph->forceIndexScan = ID_FALSE;
        }
        
        sMyGraph->forceRidScan = ID_FALSE;
    }

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    // preserved order 초기화
    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NOT_DEFINED;

    if ( sMyGraph->selectedMethod->method == NULL )
    {
        //---------------------------------------------------
        // FULL SCAN이 선택된 경우
        //---------------------------------------------------

        if ( ( sMyGraph->graph.flag & QMG_PRESERVED_ORDER_MASK )
             == QMG_PRESERVED_ORDER_NOT_DEFINED )
        {
            if( sSelectedScanHint == QMG_USED_ONLY_FULL_SCAN_HINT )
            {
                //---------------------------------------------------
                // FULL SCAN Hint가 선택된 경우
                //---------------------------------------------------

                sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
            }
            else
            {
                //---------------------------------------------------
                // cost에 의해 FULL SCAN이 선택된 경우 :
                //---------------------------------------------------

                if ( sMyGraph->accessMethodCnt > 1)
                {
                    // index가 존재하는 경우
                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |=
                        QMG_PRESERVED_ORDER_NOT_DEFINED;
                }
                else
                {
                    // index가 없는 경우
                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
                }
            }
        }
    }
    else
    {
        //---------------------------------------------------
        // INDEX SCAN이 선택된 경우
        //---------------------------------------------------

        IDE_TEST( makePreservedOrder( aStatement,
                                      sMyGraph->selectedMethod->method,
                                      sPartitionRef->table,
                                      & sMyGraph->graph.preservedOrder )
                  != IDE_SUCCESS );

        sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
        sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
    }

    //---------------------------------------------------
    // 공통 비용 정보의 설정
    //---------------------------------------------------

    sMyGraph->graph.costInfo.selectivity = 
        sMyGraph->selectedMethod->methodSelectivity;

    // output record count 설정
    sOutputRecordCnt =  sMyGraph->accessMethod->methodSelectivity *
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

    //---------------------------------------
    // 총 비용 정보 설정
    //---------------------------------------

    // 0 ( Child의 Total Cost) + My Cost
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->selectedMethod->accessCost;

    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->selectedMethod->diskCost;

    sMyGraph->graph.costInfo.totalAllCost =
        sMyGraph->selectedMethod->totalCost;

    //---------------------------------------------------
    // PROJ-1446 Host variable을 포함한 질의 최적화
    // host variable에 대한 최적화를 위한 준비과정
    //---------------------------------------------------
    if( (QCU_HOST_OPTIMIZE_ENABLE == 1) &&
        (sSelectedScanHint == QMG_NOT_USED_SCAN_HINT) &&
        (sMyGraph->accessMethodCnt > 1) &&
        (QCU_PLAN_RANDOM_SEED == 0) )
    {
        IDE_TEST( prepareScanDecisionFactor( aStatement,
                                             sMyGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        // scan hint가 사용되면 호스트 변수에 대한 최적화를 수행하지 않는다.
        // index가 없을 경우에도 호스트 변수에 대한 최적화를 수행하지 않는다.
        // Nothing to do...
    }

    // environment의 기록
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_HOST_OPTIMIZE_ENABLE );
    
    /* BUG-41134 */
    setParallelScanFlag(aStatement, aGraph);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::makePlanPartition( qcStatement * aStatement, const qmgGraph * /*aParent*/, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *               qmgSelection로 부터 Plan을 생성한다.
 *               partition하나에 대한 SCAN을 생성한다.
 *
 * Implementation :
 *    - qmgSelection으로 부터 생성가능한 plan
 *
 *           - 모든 Predicate 정보는 [SCAN]노드에 포함된다.
 *
 *                 [SCAN]
 *
 ***********************************************************************/

    qmgSELT         * sMyGraph;
    qmnPlan         * sPlan;
    qmsTableRef     * sTableRef;
    qmoSCANInfo       sSCANInfo;

    IDU_FIT_POINT_FATAL( "qmgSelection::makePlanPartition::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sSCANInfo.flag = QMO_SCAN_INFO_INITIALIZE;
    sMyGraph       = (qmgSELT *) aGraph;

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

    //-----------------------------------------------------
    // 1. Base Table을 위한 Selection Graph인 경우
    //-----------------------------------------------------

    // 하위 노드가 없는 leaf인 base일 경우 SCAN을 생성한다.

    // To Fix PR-11562
    // Indexable MIN-MAX 최적화가 적용된 경우
    // Preserved Order는 방향성을 가짐, 따라서 해당 정보를
    // 설정해줄 필요가 없음.
    // INDEXABLE Min-Max의 설정
    // 관련 코드 제거

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
        sSCANInfo.flag &= ~QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_NOTNULL_KEYRANGE_TRUE;
    }
    else
    {
        // To Fix PR-10288
        // NOTNULL KEY RANGE가 아닌 경우로 반드시 설정해 주어야 함.
        sSCANInfo.flag &= ~QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_NOTNULL_KEYRANGE_FALSE;
    }

    if (sMyGraph->forceIndexScan == ID_TRUE)
    {
        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_INDEX_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_INDEX_SCAN_TRUE;
    }
    else
    {
        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_INDEX_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_INDEX_SCAN_FALSE;
    }

    if (sMyGraph->forceRidScan == ID_TRUE)
    {
        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_RID_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_RID_SCAN_TRUE;
    }
    else
    {
        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_RID_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_RID_SCAN_FALSE;
    }

    sSCANInfo.predicate         = sMyGraph->graph.myPredicate;
    sSCANInfo.constantPredicate = sMyGraph->graph.constantPredicate;
    sSCANInfo.ridPredicate      = sMyGraph->graph.ridPredicate;
    sSCANInfo.limit             = sMyGraph->limit;
    sSCANInfo.index             = sMyGraph->selectedIndex;
    sSCANInfo.preservedOrder    = sMyGraph->graph.preservedOrder;
    sSCANInfo.sdf               = sMyGraph->sdf;
    sSCANInfo.nnfFilter         = sMyGraph->graph.nnfFilter;

    /* BUG-39306 partial scan */
    sTableRef = sMyGraph->graph.myFrom->tableRef;
    if ( sTableRef->tableAccessHints != NULL )
    {
        sSCANInfo.mParallelInfo.mDegree = sTableRef->tableAccessHints->count;
        sSCANInfo.mParallelInfo.mSeqNo  = sTableRef->tableAccessHints->id;
    }
    else
    {
        sSCANInfo.mParallelInfo.mDegree = 1;
        sSCANInfo.mParallelInfo.mSeqNo  = 1;
    }
    
    //SCAN생성
    //생성된 노드의 위치는 반드시 graph.myPlan에 세팅을 하도록 한다.
    //이 정보를 다시 child로 삼고 다음 노드 만들때 연결하도록 한다.
    // partition에 대한 scan
    // partitionRef를 인자로 넘겨주어야 한다.
    IDE_TEST( qmoOneNonPlan::makeSCAN4Partition(
                  aStatement,
                  sMyGraph->graph.myQuerySet,
                  sMyGraph->graph.myFrom,
                  &sSCANInfo,
                  sMyGraph->partitionRef,
                  &sPlan )
              != IDE_SUCCESS);

    // fix BUG-13482
    // SCAN 노드 생성시,
    // filter존재등으로 SCAN Limit 최적화를 적용하지 못한 경우,
    // selection graph의 limit도 NULL로 설정한다.
    // 이는 상위 PROJ 노드 생성시, limit start value 조정의 정보가 됨.
    if( sSCANInfo.limit == NULL )
    {
        sMyGraph->limit = NULL;
    }
    else
    {
        // Nothing To Do
    }

    sMyGraph->graph.myPlan = sPlan;

    qmg::setPlanInfo( aStatement, sPlan, &(sMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::printGraphPartition( qcStatement  * aStatement,
                                   qmgGraph     * aGraph,
                                   ULong          aDepth,
                                   iduVarString * aString )
{
    qmgSELT * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgSelection::printGraphPartition::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );
    IDE_DASSERT( aGraph->myFrom->tableRef->view == NULL );
    IDE_DASSERT( aGraph->type == QMG_SELECTION );

    sMyGraph = (qmgSELT*) aGraph;

    IDE_DASSERT( sMyGraph->partitionRef != NULL );

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

    IDE_TEST( qmoStat::printStat4Partition( sMyGraph->graph.myFrom->tableRef,
                                            sMyGraph->partitionRef,
                                            sMyGraph->partitionName,
                                            aDepth,
                                            aString )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::setJoinPushDownPredicate( qmgSELT       * aGraph,
                                        qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 *  Description : push-down join predicate를 맨 앞에 연결한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    qmoPredicate * sJoinPredicate;

    IDU_FIT_POINT_FATAL( "qmgSelection::setJoinPushDownPredicate::__FT__" );

    IDE_FT_ASSERT( aPredicate != NULL );

    IDE_TEST_RAISE( *aPredicate == NULL,
                    ERR_NOT_EXIST_PREDICATE );

    //--------------------------------------
    // join predicate의 연결리스트를
    // selection graph의 predicate 연결리스트 맨 처음에 연결시킨다.
    //--------------------------------------

    for( sJoinPredicate       = *aPredicate;
         sJoinPredicate->next != NULL;
         sJoinPredicate       = sJoinPredicate->next ) ;

    sJoinPredicate->next = aGraph->graph.myPredicate;
    aGraph->graph.myPredicate  = *aPredicate;
    *aPredicate       = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_PREDICATE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgSelection::setJoinPushDownPredicate",
                                  "not exist predicate" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::setNonJoinPushDownPredicate( qmgSELT       * aGraph,
                                           qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 *  Description : non-join push predicate를 selection graph에 연결.
 *
 *  Implementation :
 *     인자로 받은 non-joinable predicate은 keyFilter or filter로
 *     처리되어야 한다.
 *     따라서, keyRange로 추출될 joinable predicate에는 연결되지 않도록 한다.
 *
 *     1. joinable predicate은 제외
 *        joinable predicate은 selection graph predicate의 처음부분에
 *        연결되어 있기 때문에, masking 정보를 이용해서, joinable predicate을
 *        제외한다.
 *     2. non-joinable predicate과 동일한 컬럼을 찾아서 연결.
 ***********************************************************************/

    qmoPredicate * sJoinPredicate;
    qmoPredicate * sPredicate;
    qmoPredicate * sNextPredicate;

    IDU_FIT_POINT_FATAL( "qmgSelection::setNonJoinPushDownPredicate::__FT__" );

    //--------------------------------------
    // selection graph predicate의 연결리스트 중
    // non-joinable predicate의 columnID와 동일한 predicate의 연결리스트에
    // non-joinable predicate을 연결한다.
    //--------------------------------------

    // sPredicate : index joinable predicate을 제외한
    //              첫번째 predicate을 가리키게 된다.

    sJoinPredicate = *aPredicate;

    if( sJoinPredicate == NULL )
    {
        // PROJ-1502 PARTITIONED DISK TABLE
        // subquery가 포함된 predicate는 partition graph에서 처리하므로,
        // non-join push down predicate는 NULL일 수 있다.
        // Nothing to do.
    }
    else
    {
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

        //---------------------------------------------------
        // To Fix BUG-13292
        // Subquery는 다른 Predicate과 함께 key range를 구성할 수 없다.
        // 그러나 다음 예제와 같은 경우, 함꼐 구성되게 된다.
        // ex) 선택된 Join Method : Full Nested Loop Join
        //     right Graph        : selection graph
        //     ==> full nested loop join의 join predicate을 right에 내려줄때
        //         in subquery가 key range로 선택되면 단독으로 key range를
        //         구성할 수 있도록 처리해주어야 한다.
        //         이와 같은 작업을 해주는 함수가 processIndexableSubQ()이다.
        //---------------------------------------------------

        IDE_TEST( qmoPred::processIndexableInSubQ( &aGraph->graph.myPredicate )
                  != IDE_SUCCESS );


        *aPredicate = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::alterForceRidScan( qcStatement * aStatement,
                                 qmgGraph    * aGraph )
{
/***********************************************************************
 *
 * Description : PROJ-1624 global non-partitioned index
 *               partition graph의 최적화완료 후 global index scan이
 *               선택되면 global index scan용 graph로 변경한다.
 *
 * Implementation :
 *      - predicate 등 모든 정보 초기화
 *      - sdf 제거
 *
 ***********************************************************************/

    qmgSELT         * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgSelection::alterForceRidScan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgSELT *) aGraph;
    
    //---------------------------------------------------
    // 초기화
    //---------------------------------------------------
    
    // Selection Graph에 Not Null Key Range 생성 Flag 초기화
    sMyGraph->graph.flag &= ~QMG_SELT_NOTNULL_KEYRANGE_MASK;
    sMyGraph->graph.flag |= QMG_SELT_NOTNULL_KEYRANGE_FALSE;

    sMyGraph->graph.myPredicate       = NULL;
    sMyGraph->graph.constantPredicate = NULL;
    sMyGraph->graph.ridPredicate      = NULL;
    sMyGraph->limit                   = NULL;
    sMyGraph->selectedIndex           = NULL;
    sMyGraph->graph.preservedOrder    = NULL;
    sMyGraph->graph.nnfFilter         = NULL;
    sMyGraph->forceIndexScan          = ID_FALSE;
    sMyGraph->forceRidScan            = ID_TRUE;   // 강제 rid scan 설정
    
    if( sMyGraph->sdf != NULL )
    {
        // 현재 selection graph의 selectedIndex가
        // 상위 graph에 의해 다시 결정된 경우
        // host optimization을 해서는 안된다.
        // 이 경우 sdf를 disable한다.
        IDE_TEST( qtc::removeSDF( aStatement, sMyGraph->sdf ) != IDE_SUCCESS );

        sMyGraph->sdf = NULL;
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSelection::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Preserved Order의 direction을 결정한다.
 *                direction이 NOT_DEFINED 일 경우에만 호출하여야 한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    qmgSELT           * sMyGraph;
    qmgPreservedOrder * sPreservedOrder;
    qmgPreservedOrder * sChildOrder;
    UInt              * sKeyColsFlag;
    UInt                sKeyColCount;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmgSelection::finalizePreservedOrder::__FT__" );

    sMyGraph = (qmgSELT*) aGraph;

    if ( aGraph->left != NULL )
    {
        // View인 경우, 하위의 Preserved Order를 따름
        // View의 table, column으로 변환하기 때문에
        //  두 Preserved order의 table과 column은 다를 수 있다.
        sPreservedOrder = aGraph->preservedOrder;
        sChildOrder     = aGraph->left->preservedOrder;
        for ( ; sPreservedOrder != NULL && sChildOrder != NULL;
              sPreservedOrder = sPreservedOrder->next,
                  sChildOrder = sChildOrder->next )
        {
            sPreservedOrder->direction= sChildOrder->direction;
        }
    }
    else
    {
        // View가 아니고 Preserved order가 존재하는 경우는 Index scan뿐이다.
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
    }

    return IDE_SUCCESS;
}

IDE_RC qmgSelection::makeRecursiveViewScan( qcStatement * aStatement,
                                            qmgSELT     * aMyGraph )
{
    /***********************************************************************
     *
     * Description : PROJ-2582 recursive with
     *
     * Implementation :
     *   VSCAN 생성.
     *
     *   right query
     *     [FILT]
     *       |
     *     [VSCN]
     *
     ***********************************************************************/

    qtcNode  * sFilter = NULL;
    qmnPlan  * sFILT   = NULL;
    qmnPlan  * sVSCN   = NULL;

    IDU_FIT_POINT_FATAL( "qmgSelection::makeRecursiveViewScan::__FT__" );

    //---------------------------------------
    // predicate의 존재 유무에 따라
    // FILT 생성
    //---------------------------------------

    if( ( aMyGraph->graph.myPredicate != NULL ) ||
        ( aMyGraph->graph.constantPredicate != NULL ) ||
        ( aMyGraph->graph.nnfFilter != NULL ) )
    {
        if( aMyGraph->graph.myPredicate != NULL )
        {
            IDE_TEST( qmoPred::linkFilterPredicate(
                          aStatement ,
                          aMyGraph->graph.myPredicate ,
                          &sFilter ) != IDE_SUCCESS );

            // BUG-35155 Partial CNF
            if ( aMyGraph->graph.nnfFilter != NULL )
            {
                IDE_TEST( qmoPred::addNNFFilter4linkedFilter(
                              aStatement,
                              aMyGraph->graph.nnfFilter,
                              & sFilter ) != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( aMyGraph->graph.nnfFilter != NULL )
            {
                sFilter = aMyGraph->graph.nnfFilter;
            }
            else
            {
                sFilter = NULL;
            }
        }

        //make FILT
        IDE_TEST( qmoOneNonPlan::initFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sFilter ,
                      aMyGraph->graph.myPlan ,
                      &sFILT ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sFILT;
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( qmoOneNonPlan::initVSCN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myFrom ,
                                       aMyGraph->graph.myPlan ,
                                       &sVSCN ) != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sVSCN;    

    //---------------------------------------
    // VSCN생성
    //---------------------------------------

    // codesonar::Null Pointer Dereference
    IDE_FT_ERROR( aMyGraph->graph.left != NULL );
    
    // right VSCN은 LEFT 의 최상위 VMTR을 child로 가진다.
    IDE_TEST( qmoOneNonPlan::makeVSCN( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       aMyGraph->graph.myFrom,
                                       aMyGraph->graph.left->myPlan, // child
                                       sVSCN ) != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sVSCN;

    qmg::setPlanInfo( aStatement, sVSCN, &(aMyGraph->graph) );
    
    //---------------------------------------
    // predicate의 존재 유무에 따라
    // FILT 생성
    //---------------------------------------

    if( ( aMyGraph->graph.myPredicate != NULL ) ||
        ( aMyGraph->graph.constantPredicate != NULL ) ||
        ( aMyGraph->graph.nnfFilter != NULL ) )
    {
        //make FILT
        IDE_TEST( qmoOneNonPlan::makeFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sFilter ,
                      aMyGraph->graph.constantPredicate ,
                      aMyGraph->graph.myPlan ,
                      sFILT ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo( aStatement, sFILT, &(aMyGraph->graph) );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* TASK-7219 Non-shard DML */
void qmgSelection::isForcePushedPredForShardView( mtcNode * aNode,
                                                  idBool  * aIsFound )
{
    if ( ( aNode != NULL ) &&
         ( *aIsFound == ID_FALSE ) )
    {
        if ( ( aNode->lflag  & MTC_NODE_PUSHED_PRED_FORCE_MASK )
             == MTC_NODE_PUSHED_PRED_FORCE_TRUE )
        {
            *aIsFound = ID_TRUE;
        }
        else
        {
            isForcePushedPredForShardView( aNode->arguments,
                                           aIsFound );

            isForcePushedPredForShardView( aNode->next,
                                           aIsFound );
        }
    }
}
