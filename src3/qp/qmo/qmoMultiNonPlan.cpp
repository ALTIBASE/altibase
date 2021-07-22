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
 * $Id: qmoMultiNonPlan.cpp 90438 2021-04-02 08:20:57Z ahra.cho $
 *
 * Description :
 *     Plan Generator
 *
 *     Multi-child Non-materialized Plan을 생성하기 위한 관리자이다.
 *
 *     다음과 같은 Plan Node의 생성을 관리한다.
 *         - MultiBUNI 노드
 *         - PCRD 노드 (PROJ-1502 Partitioned Disk Table)
 *         - PPCRD 노드 (PROJ-1071 Parallel query)
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmo.h>
#include <qmoMultiNonPlan.h>
#include <qmoOneNonPlan.h>
#include <qmoDef.h>
#include <qdx.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <qmoPartition.h>
#include <qmv.h>

extern mtfModule mtfEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfOr;
extern mtfModule mtfAnd;

IDE_RC
qmoMultiNonPlan::initMultiBUNI( qcStatement  * aStatement ,
                                qmsQuerySet  * aQuerySet,
                                qmnPlan      * aParent,
                                qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : MultiBUNI 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncMultiBUNI의 할당 및 초기화
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncMultiBUNI          * sMultiBUNI;
    UInt                     sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::initMultiBUNI::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncMultiBUNI의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncMultiBUNI) ,
                                             (void **) & sMultiBUNI )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sMultiBUNI ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_MULTI_BUNI ,
                        qmnMultiBUNI ,
                        qmndMultiBUNI,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sMultiBUNI;

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sMultiBUNI->plan.resultDesc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoMultiNonPlan::makeMultiBUNI( qcStatement  * aStatement ,
                                qmsQuerySet  * aQuerySet ,
                                qmgChildren  * aChildrenGraph,
                                qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : MultiBUNI 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncMultiBUNI의 할당 및 초기화
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncMultiBUNI          * sMultiBUNI = (qmncMultiBUNI *)aPlan;
    qmgChildren            * sGraphChildren;
    qmnChildren            * sPlanChildren;
    qmnChildren            * sCurPlanChildren;
    UInt                     sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::makeMultiBUNI::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildrenGraph != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndMultiBUNI));

    //----------------------------------
    // 여러 Child Plan 연결
    //----------------------------------

    sCurPlanChildren = NULL;

    for ( sGraphChildren = aChildrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        //----------------------------------------------
        // 각 Graph의 Plan을 얻어 하나의 children plan을 구축
        //----------------------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmnChildren) ,
                                                 (void **) & sPlanChildren )
                  != IDE_SUCCESS );

        sPlanChildren->childPlan = sGraphChildren->childGraph->myPlan;
        sPlanChildren->next = NULL;

        //----------------------------------------------
        // Children Plan의 연결 관계를 구축
        //----------------------------------------------

        if ( sCurPlanChildren == NULL )
        {
            sMultiBUNI->plan.children = sPlanChildren;
        }
        else
        {
            sCurPlanChildren->next = sPlanChildren;
        }
        sCurPlanChildren = sPlanChildren;
    }

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sMultiBUNI->flag      = QMN_PLAN_FLAG_CLEAR;
    sMultiBUNI->plan.flag = QMN_PLAN_FLAG_CLEAR;

    for ( sGraphChildren = aChildrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        sMultiBUNI->plan.flag |= ( sGraphChildren->childGraph->myPlan->flag
                                   & QMN_PLAN_STORAGE_MASK );
    }

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sMultiBUNI->plan ,
                                            QMO_MULTI_BUNI_DEPENDENCY,
                                            (UShort)qtc::getPosFirstBitSet( & aQuerySet->depInfo ),
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree, PARALLEL_EXEC flag 는 children graph 의 flag 를 받는다.
     */
    for ( sGraphChildren = aChildrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        if ( sMultiBUNI->plan.mParallelDegree <
             sGraphChildren->childGraph->myPlan->mParallelDegree )
        {
            sMultiBUNI->plan.mParallelDegree =
                sGraphChildren->childGraph->myPlan->mParallelDegree;
        }
        else
        {
            /* nothing to do */
        }

        // PARALLEL_EXEC flag 도 하위 노드의 flag 를 받는다.
        sMultiBUNI->plan.flag |= ( sGraphChildren->childGraph->myPlan->flag
                                   & QMN_PLAN_NODE_EXIST_MASK);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1502 PARTITIONED DISK TABLE
IDE_RC
qmoMultiNonPlan::initPCRD( qcStatement  * aStatement ,
                           qmsQuerySet  * aQuerySet,
                           qmnPlan      * aParent,
                           qmnPlan     ** aPlan )
{

    qmncPCRD              * sPCRD;
    UInt                    sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::initPCRD::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncPCRD의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncPCRD) ,
                                             (void **) & sPCRD )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sPCRD ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_PCRD ,
                        qmnPCRD ,
                        qmndPCRD,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sPCRD;

    if( aParent != NULL )
    {
        IDE_TEST( qmc::pushResultDesc( aStatement,
                                       aQuerySet,
                                       ID_FALSE,
                                       aParent->resultDesc,
                                       &sPCRD->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1502 PARTITIONED DISK TABLE
IDE_RC
qmoMultiNonPlan::makePCRD( qcStatement  * aStatement ,
                           qmsQuerySet  * aQuerySet ,
                           qmsFrom      * aFrom ,
                           qmoPCRDInfo  * aPCRDInfo,
                           qmnPlan      * aPlan )
{
    qmsParseTree          * sParseTree;
    qmncPCRD              * sPCRD = (qmncPCRD *)aPlan;
    qmgChildren           * sGraphChildren;
    qmnChildren           * sPlanChildren;
    qmnChildren           * sCurPlanChildren;
    qmsIndexTableRef      * sIndexTable;
    qcmIndex              * sIndexTableIndex[2];
    qmoPredicate          * sPartFilterPred;
    qmncScanMethod          sMethod;
    idBool                  sInSubQueryKeyRange = ID_FALSE;
    idBool                  sScanLimit = ID_TRUE;
    idBool                  sIsDiskTable;
    UShort                  sIndexTupleRowID;
    UInt                    sDataNodeOffset;
    qtcNode               * sConstFilter = NULL;
    qtcNode               * sSubQFilter = NULL;
    qtcNode               * sRemain = NULL;
    qtcNode               * sPredicate[13];
    UInt                    i;
    qcDepInfo               sDepInfo;

    //PROJ-2249
    qmnRangeSortedChildren * sRangeSortedChildrenArray;
    qmnChildren            * sCurrChildren;

    // BUG-42387 Template에 Partitioned Table의 Tuple을 보관
    mtcTuple               * sPartitionedTuple;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::makePCRD::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aPCRDInfo != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndPCRD));

    sPCRD->tableRef      = aFrom->tableRef;
    sPCRD->tupleRowID    = aFrom->tableRef->table;
    sPCRD->table         = aFrom->tableRef->tableInfo->tableHandle;
    sPCRD->tableSCN      = aFrom->tableRef->tableSCN;

    // BUG-48800
    sPCRD->mPrePruningPartHandle = aPCRDInfo->mPrePruningPartHandle;
    sPCRD->mPrePruningPartSCN    = aPCRDInfo->mPrePruningPartSCN;

    //----------------------------------
    // 여러 Child Plan 연결
    //----------------------------------

    sCurPlanChildren = NULL;

    for ( sGraphChildren = aPCRDInfo->childrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        //----------------------------------------------
        // 각 Graph의 Plan을 얻어 하나의 children plan을 구축
        //----------------------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmnChildren) ,
                                                 (void **) & sPlanChildren )
                  != IDE_SUCCESS );

        sPlanChildren->childPlan = sGraphChildren->childGraph->myPlan;
        sPlanChildren->next = NULL;

        //----------------------------------------------
        // Children Plan의 연결 관계를 구축
        //----------------------------------------------

        if ( sCurPlanChildren == NULL )
        {
            sPCRD->plan.children = sPlanChildren;
        }
        else
        {
            sCurPlanChildren->next = sPlanChildren;
        }
        sCurPlanChildren = sPlanChildren;
    }

    //PROJ-2249 partition reference sort
    if ( ( ( aFrom->tableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE ) ||
           ( aFrom->tableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) ) &&
         ( aPCRDInfo->selectedPartitionCount > 0 ) )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmnRangeSortedChildren )
                                                 * aPCRDInfo->selectedPartitionCount,
                                                 (void **) & sRangeSortedChildrenArray )
                  != IDE_SUCCESS );

        for ( i = 0,
              sCurrChildren = sPCRD->plan.children;
              sCurrChildren != NULL;
              i++,
              sCurrChildren = sCurrChildren->next )
        {
            sRangeSortedChildrenArray[i].children = sCurrChildren;
            sRangeSortedChildrenArray[i].partKeyColumns = aFrom->tableRef->tableInfo->partKeyColumns;
        }

        if ( aPCRDInfo->selectedPartitionCount > 1 )
        {
            if ( aFrom->tableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE )
            {
                IDE_TEST( qmoPartition::sortPartitionRef( sRangeSortedChildrenArray,
                                                          aPCRDInfo->selectedPartitionCount )
                          != IDE_SUCCESS);
            }
            else
            {
                /* BUG-46065 support range using hash */
                IDE_TEST( qmoPartition::sortRangeHashPartitionRef( sRangeSortedChildrenArray,
                                                                   aPCRDInfo->selectedPartitionCount )
                          != IDE_SUCCESS);
            }
        }
        else
        {
            // Nothing to do.
        }

        for ( i = 0; i < aPCRDInfo->selectedPartitionCount; i++ )
        {
            sRangeSortedChildrenArray[i].partitionRef = ((qmncSCAN*)(sRangeSortedChildrenArray[i].children->childPlan))->partitionRef;
        }

        sPCRD->rangeSortedChildrenArray = sRangeSortedChildrenArray;
    }
    else
    {
        sPCRD->rangeSortedChildrenArray = NULL;
    }

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sPCRD->flag      = QMN_PLAN_FLAG_CLEAR;
    sPCRD->plan.flag = QMN_PLAN_FLAG_CLEAR;

    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID].lflag
        &= ~MTC_TUPLE_PARTITIONED_TABLE_MASK;

    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID].lflag
        |= MTC_TUPLE_PARTITIONED_TABLE_TRUE;

    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( sPCRD->tableRef->partitionSummary->isHybridPartitionedTable == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID].lflag
                &= ~MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID].lflag
                |= MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2464 hybrid partitioned table 지원
     *  Partitioned Table Tuple에 Partitioned Table의 정보가 있으면, Table Handle이 없다.
     */
    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID].tableHandle = (void *)sPCRD->table;

    // BUG-42387 Template에 Partitioned Table의 Tuple을 보관
    IDE_TEST( qtc::nextTable( &( sPCRD->partitionedTupleID ),
                              aStatement,
                              sPCRD->tableRef->tableInfo,
                              QCM_TABLE_TYPE_IS_DISK( sPCRD->tableRef->tableInfo->tableFlag ),
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    sPartitionedTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->partitionedTupleID]);
    sPartitionedTuple->lflag = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID].lflag;

    /* PROJ-2464 hybrid partitioned table 지원 */
    qmx::copyMtcTupleForPartitionedTable( sPartitionedTuple,
                                          &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID]) );

    sPartitionedTuple->rowOffset = qmc::getRowOffsetForTuple( &(QC_SHARED_TMPLATE(aStatement)->tmplate),
                                                              sPCRD->tupleRowID );
    sPartitionedTuple->rowMaximum = sPartitionedTuple->rowOffset;

    if( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPCRD->tupleRowID].lflag
          & MTC_TUPLE_STORAGE_MASK ) ==
        MTC_TUPLE_STORAGE_MEMORY )
    {
        sPCRD->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
        sPCRD->plan.flag |= QMN_PLAN_STORAGE_MEMORY;

        sIsDiskTable = ID_FALSE;
    }
    else
    {
        sPCRD->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
        sPCRD->plan.flag |= QMN_PLAN_STORAGE_DISK;

        sIsDiskTable = ID_TRUE;
    }

    // Previous Disable 설정
    sPCRD->flag &= ~QMNC_SCAN_PREVIOUS_ENABLE_MASK;
    sPCRD->flag |= QMNC_SCAN_PREVIOUS_ENABLE_FALSE;

    sPCRD->flag &= ~QMNC_SCAN_TABLE_FV_MASK;
    sPCRD->flag |= QMNC_SCAN_TABLE_FV_FALSE;

    //----------------------------------
    // 인덱스 설정 및 방향 설정
    //----------------------------------

    sPCRD->index           = aPCRDInfo->index;
    sPCRD->indexTupleRowID = 0;

    if( sPCRD->index != NULL )
    {
        if ( sPCRD->index->indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
        {
            // index table scan인 경우
            sPCRD->flag &= ~QMNC_SCAN_INDEX_TABLE_SCAN_MASK;
            sPCRD->flag |= QMNC_SCAN_INDEX_TABLE_SCAN_TRUE;

            // tableRef에도 기록
            for ( sIndexTable = aFrom->tableRef->indexTableRef;
                  sIndexTable != NULL;
                  sIndexTable = sIndexTable->next )
            {
                if ( sIndexTable->index->indexId == sPCRD->index->indexId )
                {
                    aFrom->tableRef->selectedIndexTable = sIndexTable;

                    sPCRD->indexTableHandle = sIndexTable->tableHandle;
                    sPCRD->indexTableSCN    = sIndexTable->tableSCN;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            IDE_DASSERT( sIndexTable != NULL );

            // key index, rid index를 찾는다.
            IDE_TEST( qdx::getIndexTableIndices( sIndexTable->tableInfo,
                                                 sIndexTableIndex )
                      != IDE_SUCCESS );

            // key index
            sPCRD->indexTableIndex = sIndexTableIndex[0];

            // index table scan용 tuple 할당
            IDE_TEST( qtc::nextTable( & sIndexTupleRowID,
                                      aStatement,
                                      sIndexTable->tableInfo,
                                      sIsDiskTable,
                                      MTC_COLUMN_NOTNULL_TRUE )
                      != IDE_SUCCESS );

            sPCRD->indexTupleRowID = sIndexTupleRowID;

            sIndexTable->table = sIndexTupleRowID;

            //index 정보 및 order by에 따른 traverse 방향 설정
            //aLeafInfo->preservedOrder를 보고 인덱스 방향과 다르면 sSCAN->flag
            //를 BACKWARD로 설정해주어야 한다.
            IDE_TEST( qmoOneNonPlan::setDirectionInfo( &(sPCRD->flag) ,
                                                       aPCRDInfo->index ,
                                                       aPCRDInfo->preservedOrder )
                      != IDE_SUCCESS );

            // To fix BUG-12742
            // index scan이 고정되어 있는 경우를 세팅한다.
            if( aPCRDInfo->forceIndexScan == ID_TRUE )
            {
                aPCRDInfo->flag &= ~QMNC_SCAN_FORCE_INDEX_SCAN_MASK;
                aPCRDInfo->flag |= QMNC_SCAN_FORCE_INDEX_SCAN_TRUE;
            }
            else
            {
                // Nothing to do.
            }
            
            if( ( aPCRDInfo->flag & QMO_PCRD_INFO_NOTNULL_KEYRANGE_MASK )
                == QMO_PCRD_INFO_NOTNULL_KEYRANGE_TRUE )
            {
                sPCRD->flag &= ~QMNC_SCAN_NOTNULL_RANGE_MASK;
                sPCRD->flag |= QMNC_SCAN_NOTNULL_RANGE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sPCRD->flag &= ~QMNC_SCAN_INDEX_TABLE_SCAN_MASK;
            sPCRD->flag |= QMNC_SCAN_INDEX_TABLE_SCAN_FALSE;
        }
    }
    else
    {
        sPCRD->flag &= ~QMNC_SCAN_INDEX_TABLE_SCAN_MASK;
        sPCRD->flag |= QMNC_SCAN_INDEX_TABLE_SCAN_FALSE;
    }

    //----------------------------------
    // Cursor Property의 설정
    //----------------------------------

    if ( (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT) ||
         (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE) ||
         (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE) )
    {
        sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
        if ( sParseTree->forUpdate != NULL )
        {
            // Proj 1360 Queue
            // dequeue문의 경우, row의 삭제를 위해 exclusive lock이 요구됨
            if (sParseTree->forUpdate->isQueue == ID_TRUE)
            {
                sPCRD->flag |= QMNC_SCAN_TABLE_QUEUE_TRUE;
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
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    IDE_TEST( qmg::setDisplayInfo( aFrom ,
                                   &(sPCRD->tableOwnerName) ,
                                   &(sPCRD->tableName) ,
                                   &(sPCRD->aliasName) ) != IDE_SUCCESS );

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    sPCRD->selectedPartitionCount = aPCRDInfo->selectedPartitionCount;
    sPCRD->partitionKeyRange = aPCRDInfo->partKeyRange;
    sPCRD->nnfFilter         = aPCRDInfo->nnfFilter;

    // BUG-47599 Partition coordinator print plan 
    if ( aFrom->tableRef->mEmptyPartRef == NULL )
    {
        sPCRD->partitionCount = aFrom->tableRef->partitionCount;
    }
    else
    {
        sPCRD->partitionCount = aFrom->tableRef->partitionCount-1;
    }

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // Predicate의 처리
    //----------------------------------

    if ( ( sPCRD->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
         == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE )
    {
        // partition filter만 추출하기 위해 subquery를 제외하고 복사한다.
        IDE_TEST( qmoPred::copyPredicate4Partition( aStatement,
                                                    aPCRDInfo->partFilterPredicate,
                                                    & sPartFilterPred,
                                                    aFrom->tableRef->table,
                                                    aFrom->tableRef->table,
                                                    ID_TRUE )
                  != IDE_SUCCESS );

        // partition filter 생성
        IDE_TEST( processPredicate( aStatement,
                                    aQuerySet,
                                    aFrom,
                                    sPartFilterPred,
                                    NULL,
                                    &sConstFilter,
                                    &sSubQFilter,
                                    &(sPCRD->partitionFilter),
                                    &sRemain )
                  != IDE_SUCCESS );

        IDE_DASSERT( sConstFilter == NULL );
        IDE_DASSERT( sSubQFilter == NULL );

        // partition filter 생성용으로만 사용했을뿐
        // sRemain은 사용하지 않는다.
        sRemain = NULL;

        // key range 생성
        IDE_TEST( qmoOneNonPlan::processPredicate( aStatement,
                                                   aQuerySet,
                                                   aPCRDInfo->predicate,
                                                   aPCRDInfo->constantPredicate,
                                                   NULL,
                                                   aPCRDInfo->index,
                                                   sPCRD->tupleRowID,
                                                   &(sMethod.constantFilter),
                                                   &(sMethod.filter),
                                                   &(sMethod.lobFilter) ,
                                                   &(sMethod.subqueryFilter),
                                                   &(sMethod.varKeyRange),
                                                   &(sMethod.varKeyFilter),
                                                   &(sMethod.varKeyRange4Filter),
                                                   &(sMethod.varKeyFilter4Filter),
                                                   &(sMethod.fixKeyRange),
                                                   &(sMethod.fixKeyFilter),
                                                   &(sMethod.fixKeyRange4Print),
                                                   &(sMethod.fixKeyFilter4Print),
                                                   &(sMethod.ridRange),
                                                   &sInSubQueryKeyRange )
                  != IDE_SUCCESS );

        IDE_TEST( qmoOneNonPlan::postProcessScanMethod( aStatement,
                                                        &sMethod,
                                                        &sScanLimit )
                  != IDE_SUCCESS );

        IDE_DASSERT( sMethod.ridRange == NULL );

        // key range, key filter, filter 초기화
        sPCRD->fixKeyRange         = sMethod.fixKeyRange;
        sPCRD->fixKeyRange4Print   = sMethod.fixKeyRange4Print;
        sPCRD->varKeyRange         = sMethod.varKeyRange;
        sPCRD->varKeyRange4Filter  = sMethod.varKeyRange4Filter;

        sPCRD->fixKeyFilter        = sMethod.fixKeyFilter;
        sPCRD->fixKeyFilter4Print  = sMethod.fixKeyFilter4Print;
        sPCRD->varKeyFilter        = sMethod.varKeyFilter;
        sPCRD->varKeyFilter4Filter = sMethod.varKeyFilter4Filter;

        sPCRD->constantFilter      = sMethod.constantFilter;
        sPCRD->filter              = sMethod.filter;
        sPCRD->lobFilter           = sMethod.lobFilter;
        sPCRD->subqueryFilter      = sMethod.subqueryFilter;
    }
    else
    {
        IDE_TEST( processPredicate( aStatement,
                                    aQuerySet,
                                    aFrom,
                                    aPCRDInfo->partFilterPredicate,
                                    aPCRDInfo->constantPredicate,
                                    &(sPCRD->constantFilter),
                                    &(sPCRD->subqueryFilter),
                                    &(sPCRD->partitionFilter),
                                    &sRemain )
                  != IDE_SUCCESS );

        // key range, key filter, filter 초기화
        sPCRD->fixKeyRange         = NULL;
        sPCRD->fixKeyRange4Print   = NULL;
        sPCRD->varKeyRange         = NULL;
        sPCRD->varKeyRange4Filter  = NULL;

        sPCRD->fixKeyFilter        = NULL;
        sPCRD->fixKeyFilter4Print  = NULL;
        sPCRD->varKeyFilter        = NULL;
        sPCRD->varKeyFilter4Filter = NULL;

        sPCRD->filter              = NULL;
        sPCRD->lobFilter           = NULL;
    }

    // queue에는 nnf, lob, subquery filter를 지원하지 않는다.
    if ( (sPCRD->flag & QMNC_SCAN_TABLE_QUEUE_MASK) == QMNC_SCAN_TABLE_QUEUE_TRUE )
    {
        IDE_TEST_RAISE( ( sPCRD->nnfFilter != NULL ) ||
                        ( sPCRD->lobFilter != NULL ) ||
                        ( sPCRD->subqueryFilter != NULL ),
                        ERR_NOT_SUPPORT_FILTER );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // Host 변수를 포함한
    // Constant Expression의 최적화
    //----------------------------------

    if ( sPCRD->nnfFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    sPCRD->nnfFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------
    // Predicate 관련 Flag 정보 설정
    //----------------------------------

    if( sInSubQueryKeyRange == ID_TRUE )
    {
        sPCRD->flag &= ~QMNC_SCAN_INSUBQ_KEYRANGE_MASK;
        sPCRD->flag |= QMNC_SCAN_INSUBQ_KEYRANGE_TRUE;
    }
    else
    {
        sPCRD->flag &= ~QMNC_SCAN_INSUBQ_KEYRANGE_MASK;
        sPCRD->flag |= QMNC_SCAN_INSUBQ_KEYRANGE_FALSE;
    }

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] = sPCRD->partitionFilter;
    sPredicate[1] = sPCRD->nnfFilter;
    sPredicate[2] = sPCRD->fixKeyRange;
    sPredicate[3] = sPCRD->varKeyRange;
    sPredicate[4] = sPCRD->varKeyRange4Filter;;
    sPredicate[5] = sPCRD->fixKeyFilter;
    sPredicate[6] = sPCRD->varKeyFilter;
    sPredicate[7] = sPCRD->varKeyFilter4Filter;
    sPredicate[8] = sPCRD->constantFilter;
    sPredicate[9] = sPCRD->filter;
    sPredicate[10] = sPCRD->lobFilter;
    sPredicate[11] = sPCRD->subqueryFilter;
    sPredicate[12] = sRemain; // 분류되고 남은 노드들임.

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    for( i = 0; i <= 12; i++ )
    {
        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aQuerySet,
                                           sPredicate[i],
                                           ID_USHORT_MAX,
                                           ID_TRUE )
                  != IDE_SUCCESS );
    }

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sPCRD->plan ,
                                            QMO_PCRD_DEPENDENCY,
                                            sPCRD->tupleRowID,
                                            NULL ,
                                            13 ,
                                            sPredicate ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    // Join predicate이 push down된 경우, SCAN의 depInfo에는 dependency가 여러개일 수 있다.
    // Result descriptor를 정학히 구성하기 위해 SCAN의 ID로 filtering한다.
    qtc::dependencySet( sPCRD->tupleRowID, &sDepInfo );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sPCRD->plan.resultDesc,
                                     &sDepInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    IDE_TEST( checkSimplePCRD( aStatement,
                               sPCRD )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     * PCRD 는 하위에 PRLQ 가 올 수 없으므로 PARALLEL_EXEC_FALSE 이다.
     */
    sPCRD->plan.mParallelDegree = aFrom->tableRef->mParallelDegree;
    sPCRD->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sPCRD->plan.flag |= QMN_PLAN_PRLQ_EXIST_FALSE;
    sPCRD->plan.flag |= QMN_PLAN_MTR_EXIST_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_SUPPORT_FILTER)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMO_NOT_ALLOWED_LOB_FILTER));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoMultiNonPlan::initMRGE( qcStatement  * aStatement ,
                           qmnPlan     ** aPlan )
{
    qmncMRGE          * sMRGE;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::initMRGE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncMRGE) , (void **)&sMRGE )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sMRGE ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_MRGE ,
                        qmnMRGE ,
                        qmndMRGE,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sMRGE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoMultiNonPlan::makeMRGE( qcStatement  * aStatement ,
                           qmoMRGEInfo  * aMRGEInfo,
                           qmgChildren  * aChildrenGraph,
                           qmnPlan      * aPlan )
{
    qmncMRGE          * sMRGE = (qmncMRGE *)aPlan;
    qmgChildren       * sChildrenGraph;
    qmnChildren       * sPlanChildren;
    UInt                sDataNodeOffset;
    qmsFrom             sFrom;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::makeMRGE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndMRGE) );

    //----------------------------------
    // 여러 Child Plan 연결
    //----------------------------------

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF(qmnChildren) * QMO_MERGE_IDX_MAX,
                                               (void **) & sPlanChildren )
              != IDE_SUCCESS );

    sMRGE->plan.children = sPlanChildren;

    sChildrenGraph = aChildrenGraph;

    for ( i = 0; i < QMO_MERGE_IDX_MAX; i++ )
    {
        //----------------------------------------------
        // 각 Graph의 Plan을 얻어 하나의 children plan을 구축
        //----------------------------------------------

        if ( sChildrenGraph->childGraph != NULL )
        {
            sPlanChildren->childPlan = sChildrenGraph->childGraph->myPlan;
        }
        else
        {
            // plan을 index로 접근하므로 childPlan이 없는 경우 null로 채워둔다.
            sPlanChildren->childPlan = NULL;
        }

        //----------------------------------------------
        // Children Plan의 연결 관계를 구축
        //----------------------------------------------

        if ( i + 1 < QMO_MERGE_IDX_MAX )
        {
            sPlanChildren->next = sPlanChildren + 1;
        }
        else
        {
            sPlanChildren->next = NULL;
        }

        sPlanChildren++;
        sChildrenGraph++;
    }

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sMRGE->flag = QMN_PLAN_FLAG_CLEAR;
    sMRGE->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    // tableRef만 있으면 됨
    sFrom.tableRef = aMRGEInfo->tableRef;

    IDE_TEST( qmg::setDisplayInfo( & sFrom ,
                                   &(sMRGE->tableOwnerName) ,
                                   &(sMRGE->tableName) ,
                                   &(sMRGE->aliasName) )
              != IDE_SUCCESS );

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    //----------------------------------
    // insert 관련 정보
    //----------------------------------

    // insert target 설정
    sMRGE->tableRef = aMRGEInfo->tableRef;

    // child statements
    sMRGE->selectSourceStatement = aMRGEInfo->selectSourceStatement;
    sMRGE->selectTargetStatement = aMRGEInfo->selectTargetStatement;

    sMRGE->updateStatement = aMRGEInfo->updateStatement;
    sMRGE->deleteStatement = aMRGEInfo->deleteStatement;    
    sMRGE->insertStatement = aMRGEInfo->insertStatement;
    sMRGE->insertNoRowsStatement = aMRGEInfo->insertNoRowsStatement;

    // insert where clause
    sMRGE->whereForInsert = aMRGEInfo->whereForInsert;
    
    // reset plan index
    sMRGE->resetPlanFlagStartIndex = aMRGEInfo->resetPlanFlagStartIndex;
    sMRGE->resetPlanFlagEndIndex   = aMRGEInfo->resetPlanFlagEndIndex;
    sMRGE->resetExecInfoStartIndex = aMRGEInfo->resetExecInfoStartIndex;
    sMRGE->resetExecInfoEndIndex   = aMRGEInfo->resetExecInfoEndIndex;

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    qtc::dependencyClear( & sMRGE->plan.depInfo );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     * MERGE .. INTO ... 구문은 지원하지 않는다.
     */
    sMRGE->plan.mParallelDegree = aMRGEInfo->tableRef->mParallelDegree;
    sMRGE->plan.flag &= ~QMN_PLAN_PRLQ_EXIST_MASK;
    sMRGE->plan.flag |= QMN_PLAN_PRLQ_EXIST_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoMultiNonPlan::initMTIT( qcStatement  * aStatement ,
                           qmnPlan     ** aPlan )
{
    qmncMTIT          * sMTIT;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::initMTIT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncMTIT ),
                                               (void **)& sMTIT )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sMTIT ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_MTIT ,
                        qmnMTIT ,
                        qmndMTIT,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sMTIT;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoMultiNonPlan::makeMTIT( qcStatement  * aStatement ,
                           qmgChildren  * aChildrenGraph,
                           qmnPlan      * aPlan )
{
    qmncMTIT          * sMTIT = (qmncMTIT *)aPlan;
    qmgChildren       * sGraphChildren;
    qmnChildren       * sPlanChildren;
    qmnChildren       * sCurPlanChildren;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::makeMTIT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildrenGraph != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndMTIT) );

    //----------------------------------
    // 여러 Child Plan 연결
    //----------------------------------

    sCurPlanChildren = NULL;

    for ( sGraphChildren = aChildrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        //----------------------------------------------
        // 각 Graph의 Plan을 얻어 하나의 children plan을 구축
        //----------------------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmnChildren) ,
                                                 (void **) & sPlanChildren )
                  != IDE_SUCCESS );

        sPlanChildren->childPlan = sGraphChildren->childGraph->myPlan;
        sPlanChildren->next = NULL;

        //----------------------------------------------
        // Children Plan의 연결 관계를 구축
        //----------------------------------------------

        if ( sCurPlanChildren == NULL )
        {
            sMTIT->plan.children = sPlanChildren;
        }
        else
        {
            sCurPlanChildren->next = sPlanChildren;
        }
        sCurPlanChildren = sPlanChildren;
    }

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sMTIT->flag      = QMN_PLAN_FLAG_CLEAR;
    sMTIT->plan.flag = QMN_PLAN_FLAG_CLEAR;

    for ( sGraphChildren = aChildrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        sMTIT->plan.flag |= ( sGraphChildren->childGraph->myPlan->flag
                              & QMN_PLAN_STORAGE_MASK );
    }

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    qtc::dependencyClear( & sMTIT->plan.depInfo );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree, PARALLEL_EXEC flag 는 children graph 의 flag 를 받는다.
     */
    for ( sGraphChildren = aChildrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        if ( sMTIT->plan.mParallelDegree <
             sGraphChildren->childGraph->myPlan->mParallelDegree )
        {
            sMTIT->plan.mParallelDegree =
                sGraphChildren->childGraph->myPlan->mParallelDegree;
        }
        else
        {
            /* nothing to do */
        }

        sMTIT->plan.flag |= ( sGraphChildren->childGraph->myPlan->flag
                              & QMN_PLAN_NODE_EXIST_MASK);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PROJ-1071 Parallel Query
 *
 * initPPCRD()
 * makePPCRD()
 * ------------------------------------------------------------------
 */

IDE_RC qmoMultiNonPlan::initPPCRD( qcStatement  * aStatement,
                                   qmsQuerySet  * aQuerySet,
                                   qmnPlan      * aParent,
                                   qmnPlan     ** aPlan )
{
    qmncPPCRD* sPPCRD;
    UInt       sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::initPPCRD::__FT__" );

    IDE_DASSERT( aStatement != NULL );

    // qmncPPCRD의 할당및 초기화

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmncPPCRD),
                                           (void **)&sPPCRD)
             != IDE_SUCCESS);

    QMO_INIT_PLAN_NODE( sPPCRD,
                        QC_SHARED_TMPLATE(aStatement),
                        QMN_PPCRD,
                        qmnPPCRD,
                        qmndPPCRD,
                        sDataNodeOffset );

    *aPlan = (qmnPlan*)sPPCRD;

    if ( aParent != NULL )
    {
        IDE_TEST( qmc::pushResultDesc( aStatement,
                                       aQuerySet,
                                       ID_FALSE,
                                       aParent->resultDesc,
                                       &sPPCRD->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoMultiNonPlan::makePPCRD( qcStatement  * aStatement,
                                   qmsQuerySet  * aQuerySet,
                                   qmsFrom      * aFrom,
                                   qmoPCRDInfo  * aPCRDInfo,
                                   qmnPlan      * aPlan )
{

    qmncPPCRD       * sPPCRD = (qmncPPCRD*)aPlan;
    qmgChildren     * sGraphChildren;
    qmnChildren     * sPlanChildren;
    qmnChildren     * sCurPlanChildren;
    qmsIndexTableRef* sIndexTable;
    qcmIndex        * sIndexTableIndex[2];
    idBool            sIsDiskTable;
    UShort            sIndexTupleRowID;
    UInt              sDataNodeOffset;
    qtcNode         * sRemain = NULL;
    qtcNode         * sPredicate[7];
    UInt              i;
    UShort            sTupleID;

    //PROJ-2249
    qmnRangeSortedChildren * sRangeSortedChildrenArray;
    qmnChildren            * sCurrChildren;

    // BUG-42387 Template에 Partitioned Table의 Tuple을 보관
    mtcTuple               * sPartitionedTuple;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::makePPCRD::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aFrom->tableRef != NULL );
    IDE_DASSERT( aFrom->tableRef->tableInfo != NULL );
    IDE_DASSERT( aPCRDInfo != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    aPlan->offset      = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset    = idlOS::align8(aPlan->offset + ID_SIZEOF(qmndPPCRD));

    sPPCRD->tableRef   = aFrom->tableRef;
    sPPCRD->tupleRowID = aFrom->tableRef->table;
    sPPCRD->table      = aFrom->tableRef->tableInfo->tableHandle;
    sPPCRD->tableSCN   = aFrom->tableRef->tableSCN;

    // BUG-48800
    sPPCRD->mPrePruningPartHandle = aPCRDInfo->mPrePruningPartHandle;
    sPPCRD->mPrePruningPartSCN    = aPCRDInfo->mPrePruningPartSCN;

    //----------------------------------
    // 여러 Child Plan 연결
    //----------------------------------

    sCurPlanChildren = NULL;

    for ( sGraphChildren = aPCRDInfo->childrenGraph;
          sGraphChildren != NULL;
          sGraphChildren = sGraphChildren->next )
    {
        //----------------------------------------------
        // 각 Graph의 Plan을 얻어 하나의 children plan을 구축
        //----------------------------------------------

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmnChildren),
                                               (void**)&sPlanChildren)
                 != IDE_SUCCESS);

        sPlanChildren->childPlan = sGraphChildren->childGraph->myPlan;
        sPlanChildren->next = NULL;

        //----------------------------------------------
        // Children Plan의 연결 관계를 구축
        //----------------------------------------------

        if ( sCurPlanChildren == NULL )
        {
            sPPCRD->plan.children = sPlanChildren;
        }
        else
        {
            sCurPlanChildren->next = sPlanChildren;
        }
        sCurPlanChildren = sPlanChildren;
    }

    //PROJ-2249 partition reference sort
    if ( ( (aFrom->tableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE) ||
           (aFrom->tableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH) ) &&
         (aPCRDInfo->selectedPartitionCount > 0) )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmnRangeSortedChildren ) *
                                                   aPCRDInfo->selectedPartitionCount,
                                                   (void**)&sRangeSortedChildrenArray )
                  != IDE_SUCCESS);

        for ( i = 0, sCurrChildren = sPPCRD->plan.children;
              sCurrChildren != NULL;
              i++, sCurrChildren = sCurrChildren->next )
        {
            sRangeSortedChildrenArray[i].children = sCurrChildren;
            sRangeSortedChildrenArray[i].partKeyColumns =
                aFrom->tableRef->tableInfo->partKeyColumns;
        }

        if ( aPCRDInfo->selectedPartitionCount > 1 )
        {
            if ( aFrom->tableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE )
            {
                IDE_TEST( qmoPartition::sortPartitionRef( sRangeSortedChildrenArray,
                                                          aPCRDInfo->selectedPartitionCount )
                          != IDE_SUCCESS);
            }
            else
            {
                /* BUG-46065 support range using hash */
                IDE_TEST( qmoPartition::sortRangeHashPartitionRef( sRangeSortedChildrenArray,
                                                                   aPCRDInfo->selectedPartitionCount )
                          != IDE_SUCCESS);
            }
        }
        else
        {
            // Nothing to do.
        }

        for ( i = 0; i < aPCRDInfo->selectedPartitionCount; i++ )
        {
            sRangeSortedChildrenArray[i].partitionRef =
                ((qmncSCAN*)(sRangeSortedChildrenArray[i].children->childPlan))->partitionRef;
        }

        sPPCRD->rangeSortedChildrenArray = sRangeSortedChildrenArray;
    }
    else
    {
        sPPCRD->rangeSortedChildrenArray = NULL;
    }

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sPPCRD->flag      = QMN_PLAN_FLAG_CLEAR;
    sPPCRD->plan.flag = QMN_PLAN_FLAG_CLEAR;

    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID].lflag
        &= ~MTC_TUPLE_PARTITIONED_TABLE_MASK;

    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID].lflag
        |= MTC_TUPLE_PARTITIONED_TABLE_TRUE;

    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( sPPCRD->tableRef->partitionSummary->isHybridPartitionedTable == ID_TRUE )
    {
        /* BUG-45737 hybrid patitioned table에 parallel grouping 연산 결과 오류 및 FATAL 발생합니다.
         *  - Partitioned Table과 Partition에 동일한 Materialized Function을 사용하도록 조정합니다.
         *  - 본래 MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE은 Hybrid Patitioned Table의 Key Column에만 사용하지만,
         *  - Parallel Plan이고 Grouping 함수가 있다면, 모두 동일한 Materialized Function을 사용해야만 결과를 Merge할 수 있습니다.
         *  - 따라서 Partition의 Plan인 sPPCRD->plan.children 를 순회하면서, 해당 Tuple의 lflag를 조정합니다.
         */
        for ( sCurrChildren  = sPPCRD->plan.children;
              sCurrChildren != NULL;
              sCurrChildren  = sCurrChildren->next )
        {
            sTupleID = sCurrChildren->childPlan->depInfo.depend[0];
            QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTupleID].lflag
                &= ~MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK;
            QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTupleID].lflag
                |= MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE;
        }

        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID].lflag
                &= ~MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID].lflag
                |= MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2464 hybrid partitioned table 지원
     *  Partitioned Table Tuple에 Partitioned Table의 정보가 있으면, Table Handle이 없다.
     */
    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID].tableHandle = (void *)sPPCRD->table;

    // BUG-42387 Template에 Partitioned Table의 Tuple을 보관
    IDE_TEST( qtc::nextTable( &( sPPCRD->partitionedTupleID ),
                              aStatement,
                              sPPCRD->tableRef->tableInfo,
                              QCM_TABLE_TYPE_IS_DISK( sPPCRD->tableRef->tableInfo->tableFlag ),
                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    sPartitionedTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->partitionedTupleID]);
    sPartitionedTuple->lflag = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID].lflag;

    /* PROJ-2464 hybrid partitioned table 지원 */
    qmx::copyMtcTupleForPartitionedTable( sPartitionedTuple,
                                          &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID]) );

    sPartitionedTuple->rowOffset = qmc::getRowOffsetForTuple( &(QC_SHARED_TMPLATE(aStatement)->tmplate),
                                                                    sPPCRD->tupleRowID );
    sPartitionedTuple->rowMaximum = sPartitionedTuple->rowOffset;

    if (( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPPCRD->tupleRowID].lflag
          & MTC_TUPLE_STORAGE_MASK ) ==
        MTC_TUPLE_STORAGE_MEMORY )
    {
        sPPCRD->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
        sPPCRD->plan.flag |= QMN_PLAN_STORAGE_MEMORY;

        sIsDiskTable = ID_FALSE;
    }
    else
    {
        sPPCRD->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
        sPPCRD->plan.flag |= QMN_PLAN_STORAGE_DISK;

        sIsDiskTable = ID_TRUE;
    }

    // Previous Disable 설정
    sPPCRD->flag &= ~QMNC_SCAN_PREVIOUS_ENABLE_MASK;
    sPPCRD->flag |= QMNC_SCAN_PREVIOUS_ENABLE_FALSE;

    sPPCRD->flag &= ~QMNC_SCAN_TABLE_FV_MASK;
    sPPCRD->flag |= QMNC_SCAN_TABLE_FV_FALSE;

    //----------------------------------
    // 인덱스 설정 및 방향 설정
    //----------------------------------

    sPPCRD->index           = aPCRDInfo->index;
    sPPCRD->indexTupleRowID = 0;

    if ( sPPCRD->index != NULL )
    {
        if ( sPPCRD->index->indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
        {
            // index table scan인 경우
            sPPCRD->flag &= ~QMNC_SCAN_INDEX_TABLE_SCAN_MASK;
            sPPCRD->flag |= QMNC_SCAN_INDEX_TABLE_SCAN_TRUE;

            // tableRef에도 기록
            for ( sIndexTable = aFrom->tableRef->indexTableRef;
                  sIndexTable != NULL;
                  sIndexTable = sIndexTable->next )
            {
                if ( sIndexTable->index->indexId == sPPCRD->index->indexId )
                {
                    aFrom->tableRef->selectedIndexTable = sIndexTable;

                    sPPCRD->indexTableHandle = sIndexTable->tableHandle;
                    sPPCRD->indexTableSCN    = sIndexTable->tableSCN;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            IDE_DASSERT( sIndexTable != NULL );

            // key index, rid index를 찾는다.
            IDE_TEST( qdx::getIndexTableIndices( sIndexTable->tableInfo,
                                                 sIndexTableIndex )
                      != IDE_SUCCESS );

            // key index
            sPPCRD->indexTableIndex = sIndexTableIndex[0];

            // index table scan용 tuple 할당
            IDE_TEST( qtc::nextTable( & sIndexTupleRowID,
                                      aStatement,
                                      sIndexTable->tableInfo,
                                      sIsDiskTable,
                                      MTC_COLUMN_NOTNULL_TRUE )
                      != IDE_SUCCESS );

            sPPCRD->indexTupleRowID = sIndexTupleRowID;

            sIndexTable->table = sIndexTupleRowID;

            //index 정보 및 order by에 따른 traverse 방향 설정
            //aLeafInfo->preservedOrder를 보고 인덱스 방향과 다르면 sSCAN->flag
            //를 BACKWARD로 설정해주어야 한다.
            IDE_TEST( qmoOneNonPlan::setDirectionInfo( &( sPPCRD->flag ),
                                                       aPCRDInfo->index,
                                                       aPCRDInfo->preservedOrder )
                      != IDE_SUCCESS );

            // To fix BUG-12742
            // index scan이 고정되어 있는 경우를 세팅한다.
            if ( aPCRDInfo->forceIndexScan == ID_TRUE )
            {
                aPCRDInfo->flag &= ~QMNC_SCAN_FORCE_INDEX_SCAN_MASK;
                aPCRDInfo->flag |= QMNC_SCAN_FORCE_INDEX_SCAN_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            if ( ( aPCRDInfo->flag & QMO_PCRD_INFO_NOTNULL_KEYRANGE_MASK )
                 == QMO_PCRD_INFO_NOTNULL_KEYRANGE_TRUE )
            {
                sPPCRD->flag &= ~QMNC_SCAN_NOTNULL_RANGE_MASK;
                sPPCRD->flag |= QMNC_SCAN_NOTNULL_RANGE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sPPCRD->flag &= ~QMNC_SCAN_INDEX_TABLE_SCAN_MASK;
            sPPCRD->flag |= QMNC_SCAN_INDEX_TABLE_SCAN_FALSE;
        }
    }
    else
    {
        sPPCRD->flag &= ~QMNC_SCAN_INDEX_TABLE_SCAN_MASK;
        sPPCRD->flag |= QMNC_SCAN_INDEX_TABLE_SCAN_FALSE;
    }

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    IDE_TEST( qmg::setDisplayInfo( aFrom ,
                                   &(sPPCRD->tableOwnerName) ,
                                   &(sPPCRD->tableName) ,
                                   &(sPPCRD->aliasName) )
              != IDE_SUCCESS );

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    sPPCRD->selectedPartitionCount = aPCRDInfo->selectedPartitionCount;
    sPPCRD->partitionKeyRange = aPCRDInfo->partKeyRange;
    sPPCRD->nnfFilter         = aPCRDInfo->nnfFilter;

    // BUG-47599 Partition coordinator print plan 
    if ( aFrom->tableRef->mEmptyPartRef == NULL )
    {
        sPPCRD->partitionCount = aFrom->tableRef->partitionCount;
    }
    else
    {
        sPPCRD->partitionCount = aFrom->tableRef->partitionCount-1;
    }


    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // Predicate의 처리
    //----------------------------------

    IDE_DASSERT((sPPCRD->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK)
                == QMNC_SCAN_INDEX_TABLE_SCAN_FALSE);

    IDE_TEST( processPredicate( aStatement,
                                aQuerySet,
                                aFrom,
                                aPCRDInfo->partFilterPredicate,
                                aPCRDInfo->constantPredicate,
                                &(sPPCRD->constantFilter),
                                &(sPPCRD->subqueryFilter),
                                &(sPPCRD->partitionFilter),
                                &sRemain )
              != IDE_SUCCESS );

    //----------------------------------
    // Host 변수를 포함한
    // Constant Expression의 최적화
    //----------------------------------

    if ( sPPCRD->nnfFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    sPPCRD->nnfFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------
    // Predicate 관련 Flag 정보 설정
    //----------------------------------

    sPPCRD->flag &= ~QMNC_SCAN_INSUBQ_KEYRANGE_MASK;
    sPPCRD->flag |= QMNC_SCAN_INSUBQ_KEYRANGE_FALSE;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] = sPPCRD->partitionFilter;
    sPredicate[1] = sPPCRD->nnfFilter;
    sPredicate[2] = sPPCRD->constantFilter;
    sPredicate[3] = sPPCRD->subqueryFilter;
    sPredicate[4] = sRemain; // 분류되고 남은 노드들임.

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    for ( i = 0; i <= 4; i++ )
    {
        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aQuerySet,
                                           sPredicate[i],
                                           ID_USHORT_MAX,
                                           ID_TRUE )
                  != IDE_SUCCESS );
    }

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            &sPPCRD->plan,
                                            QMO_PCRD_DEPENDENCY,
                                            sPPCRD->tupleRowID,
                                            NULL,
                                            5,
                                            sPredicate,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sPPCRD->plan.mParallelDegree = aFrom->tableRef->mParallelDegree;

    sPPCRD->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sPPCRD->plan.flag |= QMN_PLAN_PRLQ_EXIST_TRUE;
    sPPCRD->plan.flag |= QMN_PLAN_MTR_EXIST_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmoMultiNonPlan::processPredicate( qcStatement   * aStatement,
                                          qmsQuerySet   * aQuerySet,
                                          qmsFrom       * aFrom,
                                          qmoPredicate  * aPredicate ,
                                          qmoPredicate  * aConstantPredicate,
                                          qtcNode      ** aConstantFilter,
                                          qtcNode      ** aSubqueryFilter,
                                          qtcNode      ** aPartitionFilter,
                                          qtcNode      ** aRemain )
{
    qtcNode      * sKey;
    qtcNode      * sDNFKey;
    qtcNode      * sNode;
    qtcNode      * sOptimizedNode     = NULL;

    qmoPredicate * sSubqueryFilter    = NULL;
    qmoPredicate * sPartFilter        = NULL;
    qmoPredicate * sRemain            = NULL;
    qmoPredicate * sLastRemain        = NULL;
    UInt           sEstDNFCnt;

    IDU_FIT_POINT_FATAL( "qmoMultiNonPlan::processPredicate::__FT__" );

    *aConstantFilter  = NULL;
    *aSubqueryFilter  = NULL;
    *aPartitionFilter = NULL;
    *aRemain          = NULL;
    sKey              = NULL;
    sDNFKey           = NULL;

    if( aConstantPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aConstantPredicate,
                                          & sNode )
                  != IDE_SUCCESS );

        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        *aConstantFilter = sOptimizedNode;
    }
    else
    {
        // Nothing to do.
    }

    if( aPredicate != NULL )
    {
        IDE_TEST( qmoPred::makePartFilterPredicate( aStatement,
                                                    aQuerySet,
                                                    aPredicate,
                                                    aFrom->tableRef->tableInfo->partKeyColumns,
                                                    aFrom->tableRef->tableInfo->partitionMethod,
                                                    & sPartFilter,
                                                    & sRemain,
                                                    & sSubqueryFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        sPartFilter = NULL;
        sRemain = NULL;
        sSubqueryFilter = NULL;
    }

    if( sPartFilter != NULL )
    {
        // fixed가 되면 remain으로 빠진다.
        if( ( sPartFilter->flag & QMO_PRED_VALUE_MASK )
            == QMO_PRED_VARIABLE )
        {
            IDE_TEST( qmoPred::linkPredicate( aStatement ,
                                              sPartFilter ,
                                              &sKey ) != IDE_SUCCESS );

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

            if ( sDNFKey == NULL )
            {
                if( sRemain == NULL )
                {
                    sRemain = sPartFilter;
                }
                else
                {
                    for( sLastRemain = sRemain;
                         sLastRemain->next != NULL;
                         sLastRemain = sLastRemain->next ) ;

                    sLastRemain->next = sPartFilter;
                }
            }
            else
            {
                *aPartitionFilter = sDNFKey;
            }
        }
        else
        {
            if( sRemain == NULL )
            {
                sRemain = sPartFilter;
            }
            else
            {
                for( sLastRemain = sRemain;
                     sLastRemain->next != NULL;
                     sLastRemain = sLastRemain->next ) ;

                sLastRemain->next = sPartFilter;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    if( sRemain != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          sRemain,
                                          aRemain )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( sSubqueryFilter != NULL )
    {
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement,
                                                sSubqueryFilter,
                                                & sNode )
                  != IDE_SUCCESS );

        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        *aSubqueryFilter = sOptimizedNode;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoMultiNonPlan::checkSimplePCRD( qcStatement * aStatement,
                                         qmncPCRD    * aPCRD )
{
    qmncPCRD      * sPCRD     = aPCRD;
    idBool          sIsSimple = ID_FALSE;
    qtcNode       * sORNode;
    qtcNode       * sCompareNode;
    qtcNode       * sColumnNode;
    qtcNode       * sValueNode;
    qmnValueInfo  * sValueInfo = NULL;
    UInt            sCompareOpCount = 0;
    mtcColumn     * sColumn;
    mtcColumn     * sConstColumn;
    mtcColumn     * sValueColumn;

    QMN_INIT_VALUE_INFO( &(sPCRD->mSimpleValues) );

    if ( ( sPCRD->fixKeyFilter != NULL ) ||
         ( sPCRD->varKeyFilter != NULL ) ||
         ( sPCRD->constantFilter != NULL ) ||
         ( sPCRD->filter != NULL ) ||
         ( sPCRD->subqueryFilter != NULL ) ||
         ( sPCRD->nnfFilter != NULL ) )
    {
        sIsSimple = ID_FALSE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // full scan/index full scan인 경우
        if ( ( sPCRD->fixKeyRange == NULL ) &&
             ( sPCRD->varKeyRange == NULL ) )
        {
            // 레코드가 많은 경우 simple로 처리하지 않는다.
            // (일단은 1024000개 까지만)
            if ( sPCRD->tableRef->statInfo->totalRecordCnt 
                 <= ( QMO_STAT_TABLE_RECORD_COUNT * 100 ) )
            {
                sIsSimple = ID_TRUE;
            }
            else
            {
                sIsSimple = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }
        }
        else
        {
            sIsSimple = ID_TRUE;
        }
    }

    if ( sPCRD->tableRef->partitionSummary != NULL )
    {
        if ( sPCRD->tableRef->partitionSummary->diskPartitionCount > 0 )
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
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

    if ( sPCRD->partitionFilter != NULL )
    {
        sORNode = sPCRD->partitionFilter;

        if ( ( sORNode->node.module != &mtfOr ) ||
             ( sORNode->node.arguments->module != &mtfAnd ) ||
             ( sORNode->node.arguments->next != NULL ) )
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }

        sIsSimple = ID_TRUE; 
        sCompareNode = (qtcNode *)(sORNode->node.arguments->arguments);

        if ( sCompareNode->node.next != NULL )
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
        sValueInfo = &sPCRD->mSimpleValues;

        if ( sCompareNode->node.module == &mtfEqual )
        {
            sValueInfo->op = QMN_VALUE_OP_EQUAL;
        }
        else
        {
            // <, <=, >, >=인 경우
            if ( sCompareNode->node.module == &mtfLessThan )
            {
                if ( sCompareNode->indexArgument == 0 )
                {
                    // set type
                    sValueInfo->op = QMN_VALUE_OP_LT;
                }
                else
                {
                    sValueInfo->op = QMN_VALUE_OP_GT;
                }
            }
            else if ( sCompareNode->node.module == &mtfLessEqual )
            {
                if ( sCompareNode->indexArgument == 0 )
                {
                    // set type
                    sValueInfo->op = QMN_VALUE_OP_LE;
                }
                else
                {
                    sValueInfo->op = QMN_VALUE_OP_GE;
                }
            }
            else if ( sCompareNode->node.module == &mtfGreaterThan )
            {
                if ( sCompareNode->indexArgument == 0 )
                {
                    // set type
                    sValueInfo->op = QMN_VALUE_OP_GT;
                }
                else
                {
                    sValueInfo->op = QMN_VALUE_OP_LT;
                }
            }
            else if ( sCompareNode->node.module == &mtfGreaterEqual )
            {
                if ( sCompareNode->indexArgument == 0 )
                {
                    // set type
                    sValueInfo->op = QMN_VALUE_OP_GE;
                }
                else
                {
                    sValueInfo->op = QMN_VALUE_OP_LE;
                }
            }
            else
            {
                sIsSimple = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }

            sCompareOpCount++;
        }

        if ( sCompareNode->indexArgument == 0 )
        {
            sColumnNode = (qtcNode*)(sCompareNode->node.arguments);
            sValueNode  = (qtcNode*)(sCompareNode->node.arguments->next);
        }
        else
        {
            sColumnNode = (qtcNode*)(sCompareNode->node.arguments->next);
            sValueNode  = (qtcNode*)(sCompareNode->node.arguments);
        }

        // 순수컬럼과 값이어야 한다.
        if ( QTC_IS_COLUMN( aStatement, sColumnNode ) == ID_TRUE )
        {
            // Nothing to do.
        }
        else
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }

        sColumn = QTC_STMT_COLUMN( aStatement, sColumnNode );

        // 정수형과 문자형만 가능하다.
        if ( ( sColumn->module->id == MTD_SMALLINT_ID ) ||
             ( sColumn->module->id == MTD_BIGINT_ID ) ||
             ( sColumn->module->id == MTD_INTEGER_ID ) ||
             ( sColumn->module->id == MTD_CHAR_ID ) ||
             ( sColumn->module->id == MTD_VARCHAR_ID ) ||
             ( sColumn->module->id == MTD_NUMERIC_ID ) ||
             ( sColumn->module->id == MTD_FLOAT_ID ) )
        {
            // Nothing to do.
        }
        else
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }

        sValueInfo->column = *sColumn;

        // 컬럼, 상수나 호스트변수만 가능하다.
        if ( QTC_IS_COLUMN( aStatement, sValueNode ) == ID_TRUE )
        {
            // 컬럼인 경우 column node와 value node에 conversion이
            // 없어야 한다.
            if ( ( sColumnNode->node.conversion != NULL ) ||
                 ( sValueNode->node.conversion != NULL ) )
            {
                sIsSimple = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                // Nothing to do.
            }

            sValueColumn = QTC_STMT_COLUMN( aStatement, sValueNode );

            // 동일 타입이어야 한다.
            if ( sColumn->module->id == sValueColumn->module->id )
            {
                sValueInfo->type = QMN_VALUE_TYPE_COLUMN;

                sValueInfo->value.columnVal.table  = sValueNode->node.table;
                sValueInfo->value.columnVal.column = *sValueColumn;
            }
            else
            {
                sIsSimple = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                                sValueNode )
             == ID_TRUE )
        {
            // 상수인 경우 column node에 conversion이 없어야 한다.
            if ( sColumnNode->node.conversion != NULL )
            {
                sIsSimple = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                // Nothing to do.
            }

            sConstColumn = QTC_STMT_COLUMN( aStatement, sValueNode );

            // 상수는 동일 타입이어야 한다.
            if ( ( sColumn->module->id == sConstColumn->module->id ) ||
                 ( ( sColumn->module->id == MTD_NUMERIC_ID ) &&
                   ( sConstColumn->module->id == MTD_FLOAT_ID ) ) ||
                 ( ( sColumn->module->id == MTD_FLOAT_ID ) &&
                   ( sConstColumn->module->id == MTD_NUMERIC_ID ) ) )
            {
                sValueInfo->type = QMN_VALUE_TYPE_CONST_VALUE;
                sValueInfo->value.constVal =
                    QTC_STMT_FIXEDDATA(aStatement, sValueNode);
            }
            else
            {
                sIsSimple = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }

        // post process를 수행했으므로 host const warpper가 달릴 수 있다.
        if ( sValueNode->node.module == &qtc::hostConstantWrapperModule )
        {
            sValueNode = (qtcNode*) sValueNode->node.arguments;
        }
        else
        {
            // Nothing to do.
        }

        if ( qtc::isHostVariable( QC_SHARED_TMPLATE(aStatement),
                                  sValueNode )
             == ID_TRUE )
        {
            // 호스트 변수인 경우 column node와 value node의
            // conversion node는 동일 타입으로만 bind한다면
            // 무시할 수 있다.
            sValueInfo->type = QMN_VALUE_TYPE_HOST_VALUE;
            sValueInfo->value.id = sValueNode->node.column;

            // param 등록
            IDE_TEST( qmv::describeParamInfo( aStatement,
                                              sColumn,
                                              sValueNode )
                      != IDE_SUCCESS );

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        // 여기까지 왔다면 simple이 아니다.
        sIsSimple = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    sPCRD->mIsSimple = sIsSimple;
    sPCRD->mSimpleCompareOpCount = sCompareOpCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

