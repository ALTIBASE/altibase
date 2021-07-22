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
 * $Id: qmgInsert.cpp 53774 2012-06-15 04:53:31Z eerien $
 *
 * Description :
 *     Insert Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgInsert.h>
#include <qmo.h>
#include <qmoOneNonPlan.h>
#include <qmoPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE
#include <qcmPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE

IDE_RC
qmgInsert::init( qcStatement      * aStatement,
                 qmmInsParseTree  * aParseTree,
                 qmgGraph         * aChildGraph,
                 qmgGraph        ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgInsert Graph의 초기화
 *
 * Implementation :
 *    (1) qmgInsert을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgINST         * sMyGraph;
    qmsTableRef     * sInsertTableRef;

    IDU_FIT_POINT_FATAL( "qmgInsert::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //---------------------------------------------------
    // Insert Graph를 위한 기본 초기화
    //---------------------------------------------------
    
    // qmgInsert을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgINST ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_INSERT;
    sMyGraph->graph.left = aChildGraph;
    if ( aChildGraph != NULL )
    {
        qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                                   & aChildGraph->depInfo );
    }
    else
    {
        // Nothing to do.
    }

    // insert는 querySet이 없음
    sMyGraph->graph.myQuerySet = NULL;

    sMyGraph->graph.optimize = qmgInsert::optimize;
    sMyGraph->graph.makePlan = qmgInsert::makePlan;
    sMyGraph->graph.printGraph = qmgInsert::printGraph;

    sInsertTableRef = aParseTree->insertTableRef;
    
    // Disk/Memory 정보 설정
    if ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sInsertTableRef->table].lflag
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
    // Insert Graph 만을 위한 초기화
    //---------------------------------------------------

    // 최상위 graph인 insert graph에 insert table 정보를 설정
    sMyGraph->tableRef = sInsertTableRef;
    
    // 최상위 graph인 insert graph에 insert columns 정보를 설정
    sMyGraph->columns          = aParseTree->insertColumns;
    sMyGraph->columnsForValues = aParseTree->columnsForValues;
    
    // 최상위 graph인 insert graph에 insert values 정보를 설정
    sMyGraph->rows           = aParseTree->rows;
    sMyGraph->valueIdx       = aParseTree->valueIdx;
    sMyGraph->canonizedTuple = aParseTree->canonizedTuple;
    sMyGraph->compressedTuple= aParseTree->compressedTuple;     // PROJ-2264

    // 최상위 graph인 insert graph에 queue 정보를 설정
    sMyGraph->queueMsgIDSeq = aParseTree->queueMsgIDSeq;

    // 최상위 graph인 insert graph에 hint 정보를 설정
    sMyGraph->hints = aParseTree->hints;

    // 최상위 graph인 insert graph에 sequence 정보를 설정
    sMyGraph->nextValSeqs = aParseTree->common.nextValSeqs;
    
    // 최상위 graph인 insert graph에 multi-insert 정보를 설정
    if ( (aParseTree->flag & QMM_MULTI_INSERT_MASK) == QMM_MULTI_INSERT_TRUE )
    {
        sMyGraph->multiInsertSelect = ID_TRUE;
    }
    else
    {
        sMyGraph->multiInsertSelect = ID_FALSE;
    }
    
    // 최상위 graph인 insert graph에 instead of trigger정보를 설정
    sMyGraph->insteadOfTrigger = aParseTree->insteadOfTrigger;

    // 최상위 graph인 insert graph에 constraint를 연결
    sMyGraph->parentConstraints = aParseTree->parentConstraints;
    sMyGraph->checkConstrList   = aParseTree->checkConstrList;

    // 최상위 graph인 insert graph에 return into를 연결
    sMyGraph->returnInto = aParseTree->returnInto;

    // 최상위 graph인 insert graph에 Default Expr을 연결
    sMyGraph->defaultExprTableRef = aParseTree->defaultTableRef;
    sMyGraph->defaultExprColumns  = aParseTree->defaultExprColumns;

    // BUG-43063 insert nowait
    sMyGraph->lockWaitMicroSec = aParseTree->lockWaitMicroSec;
    
    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgInsert::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgInsert의 최적화
 *
 * Implementation :
 *    (1) CASE 1 : INSERT...VALUE(...(subquery)...)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgINST         * sMyGraph;
    qcmTableInfo    * sTableInfo;

    IDU_FIT_POINT_FATAL( "qmgInsert::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgINST*) aGraph;
    sTableInfo = sMyGraph->tableRef->tableInfo;
    
    //-----------------------------------------
    // PROJ-1566
    // INSERT APPEND Hint 처리
    // 아래 제약 조건을 만족하지 않는 경우, append hint를 제거
    //-----------------------------------------

    if ( sMyGraph->hints != NULL )
    {
        if ( ( sMyGraph->hints->directPathInsHintFlag &
               SMI_INSERT_METHOD_MASK )
             == SMI_INSERT_METHOD_APPEND )
        {
            // APPEND Hint가 주어진 경우

            while(1)
            {
                if ( sTableInfo->indexCount != 0 )
                {
                    // (2) 대상 table은 index를 가질 수 없다.
                    sMyGraph->hints->directPathInsHintFlag
                        = SMI_INSERT_METHOD_NORMAL;
                    break;
                }

                if ( sTableInfo->triggerCount != 0 )
                {
                    // (3) 대상 table은 trigger를 가질 수 없다.
                    sMyGraph->hints->directPathInsHintFlag
                        = SMI_INSERT_METHOD_NORMAL;
                    break;
                }

                if ( sTableInfo->foreignKeyCount != 0 )
                {
                    // (4) 대상 table은 foreign key를 가질 수 없다.
                    sMyGraph->hints->directPathInsHintFlag
                        = SMI_INSERT_METHOD_NORMAL;
                    break;
                }

                /* BUG-29753 Direct-Path INSERT 제약 조건에서
                   NOT NULL constraint 제약 삭제
                if ( sTableInfo->notNullCount != 0 )
                {
                    // (5) 대상 table은 not null constraint를 가질 수 없다.
                    sMyGraph->hints->directPathInsHintFlag
                        = SMI_INSERT_METHOD_NORMAL;
                    break;
                }
                */

                /* PROJ-1107 Check Constraint 지원 */
                if ( sTableInfo->checkCount != 0 )
                {
                    /* (6) 대상 table은 check constraint를 가질 수 없다. */
                    sMyGraph->hints->directPathInsHintFlag
                        = SMI_INSERT_METHOD_NORMAL;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

                if ( sTableInfo->replicationCount != 0 )
                {
                    // (7) replication 할 수 없다.
                    sMyGraph->hints->directPathInsHintFlag
                        = SMI_INSERT_METHOD_NORMAL;
                    break;
                }

                /* PROJ-2464 hybrid partitioned table 지원
                 *  APPEND Hint는 Partitioned Table에 상관없이 Disk Partition이 있는 경우에만 지원한다.
                 *  여기에서 Partition 정보를 알 수 없다. qmgInsert::makePlan()에서 처리한다.
                 */
                // if ( sTableFlag != SMI_TABLE_DISK )
                // {
                //     // (8) 대상 table은 disk table이어야 한다.
                //     //     memory table은 APPEND 방식으로 insert 할 수 없음
                //     sMyGraph->hints->directPathInsHintFlag
                //         = SMI_INSERT_METHOD_NORMAL;
                //     break;
                // }
                // else
                // {
                //     /* Nothing to do */
                // }

                if ( sTableInfo->lobColumnCount != 0 )
                {
                    // (9) 대상 table에 lob column이 존재하면 안됨
                    sMyGraph->hints->directPathInsHintFlag
                        = SMI_INSERT_METHOD_NORMAL;
                    break;
                }

                break;
            }
        }
        else
        {
            // APPEND Hint가 주어지지 않은 경우
            // nothing to do
        }
    }

    //---------------------------------------------------
    // 공통 비용 정보 설정
    //---------------------------------------------------

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt = 0;

    // recordSize
    sMyGraph->graph.costInfo.recordSize = 1;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = 1;

    // outputRecordCnt
    sMyGraph->graph.costInfo.outputRecordCnt = 0;

    // myCost
    sMyGraph->graph.costInfo.myAccessCost = 0;
    sMyGraph->graph.costInfo.myDiskCost = 0;
    sMyGraph->graph.costInfo.myAllCost = 0;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost = 0;
    sMyGraph->graph.costInfo.totalDiskCost = 0;
    sMyGraph->graph.costInfo.totalAllCost = 0;

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;

    return IDE_SUCCESS;
}

IDE_RC
qmgInsert::makePlan( qcStatement     * aStatement,
                     const qmgGraph  * aParent,
                     qmgGraph        * aGraph )
{
/***********************************************************************
 *
 * Description : qmgInsert으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgInsert으로 생성가능한 Plan
 *
 *           [INST]
 *
 ***********************************************************************/

    qmmInsParseTree * sInsParseTree;
    qcStatement     * sSubQStatement;
    qmgINST         * sMyGraph;
    qmnPlan         * sPlan;
    qmnPlan         * sChildPlan;
    qmmValueNode    * sValueNode;
    qmmMultiRows    * sMultiRows;
    qmoINSTInfo       sINSTInfo;
    qmncINST        * sINST;
    idBool            sIsConst = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmgInsert::makePlan::__FT__" );

    sInsParseTree = (qmmInsParseTree *)aStatement->myPlan->parseTree;
    
    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgINST*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= QMG_PARALLEL_IMPOSSIBLE_FALSE;

    // BUG-38410
    // 최상위 plan 이므로 기본값을 세팅한다.
    aGraph->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aGraph->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    //---------------------------
    // Plan의 생성
    //---------------------------

    // 최상위 plan이다.
    IDE_DASSERT( aParent == NULL );
    
    IDE_TEST( qmoOneNonPlan::initINST( aStatement ,
                                       &sPlan )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;
    
    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    if ( sMyGraph->graph.left != NULL )
    {
        // BUG-38410
        // SCAN parallel flag 를 자식 노드로 물려준다.
        aGraph->left->flag  |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

        // subquery의 statement로 makePlan한다.
        sSubQStatement = sInsParseTree->select;
        
        IDE_TEST( sMyGraph->graph.left->makePlan( sSubQStatement ,
                                                  &sMyGraph->graph,
                                                  sMyGraph->graph.left )
                  != IDE_SUCCESS);

        // child가져오기
        sChildPlan = sMyGraph->graph.left->myPlan;
    }
    else
    {
        sChildPlan = NULL;
    }

    //----------------------------
    // INST의 생성
    //----------------------------

    sINSTInfo.tableRef          = sMyGraph->tableRef;
    sINSTInfo.columns           = sMyGraph->columns;
    sINSTInfo.columnsForValues  = sMyGraph->columnsForValues;
    sINSTInfo.rows              = sMyGraph->rows;
    sINSTInfo.valueIdx          = sMyGraph->valueIdx;
    sINSTInfo.canonizedTuple    = sMyGraph->canonizedTuple;
    sINSTInfo.compressedTuple   = sMyGraph->compressedTuple;    // PROJ-2264
    sINSTInfo.queueMsgIDSeq     = sMyGraph->queueMsgIDSeq;
    sINSTInfo.hints             = sMyGraph->hints;
    sINSTInfo.nextValSeqs       = sMyGraph->nextValSeqs;
    sINSTInfo.multiInsertSelect = sMyGraph->multiInsertSelect;
    sINSTInfo.insteadOfTrigger  = sMyGraph->insteadOfTrigger;
    sINSTInfo.parentConstraints = sMyGraph->parentConstraints;
    sINSTInfo.checkConstrList   = sMyGraph->checkConstrList;
    sINSTInfo.returnInto        = sMyGraph->returnInto;
    sINSTInfo.lockWaitMicroSec  = sMyGraph->lockWaitMicroSec;

    /* PROJ-1090 Function-based Index */
    sINSTInfo.defaultExprTableRef = sMyGraph->defaultExprTableRef;
    sINSTInfo.defaultExprColumns  = sMyGraph->defaultExprColumns;

    IDE_TEST( qmoOneNonPlan::makeINST( aStatement ,
                                       & sINSTInfo ,
                                       sChildPlan ,
                                       sPlan )
                 != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;

    if ( sMyGraph->graph.left == NULL )
    {
        /* BUG-42764 Multi Row */
        for ( sMultiRows = sMyGraph->rows;
              sMultiRows != NULL;
              sMultiRows = sMultiRows->next )
        {
            //------------------------------------------
            // INSERT ... VALUES 구문 내의 Subquery 최적화
            //------------------------------------------
            // BUG-32584
            // 모든 서브쿼리에 대해서 MakeGraph 한후에 MakePlan을 해야 한다.
            for ( sValueNode = sMultiRows->values;
                  sValueNode != NULL;
                  sValueNode = sValueNode->next )
            {
                // Subquery 존재할 경우 Subquery 최적화
                if ( (sValueNode->value->lflag & QTC_NODE_SUBQUERY_MASK )
                     == QTC_NODE_SUBQUERY_EXIST )
                {
                    IDE_TEST( qmoSubquery::optimizeExprMakeGraph( aStatement,
                                                                  ID_UINT_MAX,
                                                                  sValueNode->value )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nohting To Do
                }
            }

            for ( sValueNode = sMultiRows->values;
                  sValueNode != NULL;
                  sValueNode = sValueNode->next )
            {
                // Subquery 존재할 경우 Subquery 최적화
                if ( (sValueNode->value->lflag & QTC_NODE_SUBQUERY_MASK )
                     == QTC_NODE_SUBQUERY_EXIST )
                {
                    IDE_TEST( qmoSubquery::optimizeExprMakePlan( aStatement,
                                                                 ID_UINT_MAX,
                                                                 sValueNode->value )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nohting To Do
                }
            }
        }
    }
    else
    {
        // insert select의 경우 validation시 partitionRef를 구성했다.

        // Nothing to do.
    }

    //----------------------------
    // into절에 대한 파티션드 테이블 최적화
    //----------------------------

    /* BUG-47614 partition table insert시 optimize 성능 개선 */
    if ( sMyGraph->parentConstraints != NULL )
    {
        sIsConst = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }
    // PROJ-1502 PARTITIONED DISK TABLE
    /* PROJ-2464  hybrid partitioned table 지원
     *  - Insert Select시 Partition의 Tuple을 구성하지 않았다.
     *  - HPT의 추가로 실제 Insert가 되는 Partition의 Tuple를 사용해야 하므로 무조건 호출한다.
     *  - Move DML의 호출 순서와 동일하게 변경한다.
     * BUG-47614 partition table insert시 optimize 성능 개선
     */
    IDE_TEST( qmoPartition::optimizeIntoForInsert( aStatement,
                                                   sMyGraph->tableRef,
                                                   sIsConst )
              != IDE_SUCCESS );

    sINST = (qmncINST *)sPlan;

    /* BUG-47625 Insert execution 성능향상 */
    if ( ( sINST->isMultiInsertSelect == ID_TRUE ) ||
         ( sMyGraph->tableRef->tableInfo->lobColumnCount > 0 ) ||
         ( sMyGraph->tableRef->tableInfo->triggerCount > 0 ) ||
         ( sINST->returnInto != NULL ) ||
         ( sINST->checkConstrList != NULL ) ||
         ( sINST->rows->next != NULL ) ||
         ( sINST->compressedTuple != UINT_MAX ) ||
         ( sINST->isAppend == ID_TRUE ) ||
         ( sINST->insteadOfTrigger == ID_TRUE ) ||
         ( sINST->defaultExprColumns != NULL ) ||
         ( ( sINST->flag & QMNC_INST_EXIST_ENCRYPT_COLUMN_MASK )
           == QMNC_INST_EXIST_ENCRYPT_COLUMN_TRUE ) ||
         ( ( sINST->flag & QMNC_INST_EXIST_TIMESTAMP_MASK )
           == QMNC_INST_EXIST_TIMESTAMP_TRUE ) ||
         ( ( sINST->flag & QMNC_INST_EXIST_QUEUE_MASK )
           == QMNC_INST_EXIST_QUEUE_TRUE ) )
    {
        sINST->flag &= ~QMNC_INST_COMPLEX_MASK;
        sINST->flag |= QMNC_INST_COMPLEX_TRUE;
    }
    else
    {
        sINST->flag &= ~QMNC_INST_COMPLEX_MASK;
        sINST->flag |= QMNC_INST_COMPLEX_FALSE;
    }

    if ( sMyGraph->tableRef->partitionSummary != NULL )
    {
        if ( sMyGraph->tableRef->partitionSummary->diskPartitionCount > 0 )
        {
            sINST->isSimple = ID_FALSE;
        }
        else
        {
            /* Nothing to do */
        }

        /* BUG-47625 Insert execution 성능향상 */
        if ( sMyGraph->tableRef->partitionSummary->isHybridPartitionedTable == ID_TRUE )
        {
            sINST->flag &= ~QMNC_INST_COMPLEX_MASK;
            sINST->flag |= QMNC_INST_COMPLEX_TRUE;
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

    /* PROJ-2464 hybrid partitioned table 지원
     *  APPEND Hint는 Partitioned Table에 상관없이 Disk Partition이 있는 경우에만 지원한다.
     */
    fixHint( sPlan );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgInsert::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgInsert::printGraph::__FT__" );

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

    if ( aGraph->left != NULL )
    {
        IDE_TEST( aGraph->left->printGraph( aStatement,
                                            aGraph->left,
                                            aDepth + 1,
                                            aString )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

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

/***********************************************************************
 *
 * Description : Disk Partition을 고려하여 INSERT 구문의 HInt를 처리한다.
 *
 * Implementation :
 *
 ***********************************************************************/
void qmgInsert::fixHint( qmnPlan * aPlan )
{
    qmncINST    * sINST           = (qmncINST *)aPlan;
    qmsTableRef * sTableRef       = NULL;

    sINST->isAppend = ID_FALSE;

    if ( sINST->hints != NULL )
    {
        if ( ( sINST->hints->directPathInsHintFlag & SMI_INSERT_METHOD_MASK )
                                                  == SMI_INSERT_METHOD_APPEND )
        {
            sTableRef = sINST->tableRef;

            if ( sTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                if ( sTableRef->partitionSummary->diskPartitionCount > 0 )
                {
                    sINST->isAppend = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                if ( QCM_TABLE_TYPE_IS_DISK( sTableRef->tableInfo->tableFlag ) == ID_TRUE )
                {
                    sINST->isAppend = ID_TRUE;
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
    else
    {
        /* Nothing to do */
    }

    return;
}
