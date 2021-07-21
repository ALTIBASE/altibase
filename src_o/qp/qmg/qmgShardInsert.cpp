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
 * $Id$
 *
 * Description :
 *     Shard Insert Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgShardInsert.h>
#include <qmgShardDML.h>
#include <qmo.h>
#include <qmoOneNonPlan.h>
#include <qcg.h>

IDE_RC
qmgShardInsert::init( qcStatement      * aStatement,
                      qmmInsParseTree  * aParseTree,
                      qmgGraph         * aChildGraph,
                      qmgGraph        ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgShardInsert Graph의 초기화
 *
 * Implementation :
 *    (1) qmgShardInsert을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgShardINST    * sMyGraph;
    qmsTableRef     * sInsertTableRef;

    IDU_FIT_POINT_FATAL( "qmgShardInsert::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );

    //---------------------------------------------------
    // Insert Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgShardInsert을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgShardINST ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_SHARD_INSERT;
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

    sMyGraph->graph.optimize = qmgShardInsert::optimize;
    sMyGraph->graph.makePlan = qmgShardInsert::makePlan;
    sMyGraph->graph.printGraph = qmgShardInsert::printGraph;

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

    // 최상위 graph인 insert graph에 queue 정보를 설정
    sMyGraph->queueMsgIDSeq = aParseTree->queueMsgIDSeq;

    // 최상위 graph인 insert graph에 sequence 정보를 설정
    sMyGraph->nextValSeqs = aParseTree->common.nextValSeqs;

    // insert position
    IDE_FT_ASSERT( QC_IS_NULL_NAME( sInsertTableRef->position ) != ID_TRUE );

    SET_POSITION( sMyGraph->insertPos, aParseTree->common.stmtPos );
    sMyGraph->insertPos.size =
        sInsertTableRef->position.offset +
        sInsertTableRef->position.size -
        sMyGraph->insertPos.offset;

    /* BUG-45865 */
    if ( sInsertTableRef->position.stmtText[sInsertTableRef->position.offset +
                                            sInsertTableRef->position.size] == '\"' )
    {
        sMyGraph->insertPos.size += 1;
    }

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgShardInsert::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgShardInsert의 최적화
 *
 * Implementation :
 *    (1) CASE 1 : INSERT...VALUE(...(subquery)...)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgShardINST    * sMyGraph;
    qcmTableInfo    * sTableInfo;
    sdiObjectInfo   * sShardObjInfo = NULL;
    sdiAnalyzeInfo  * sAnalyzeInfo;
    UInt              sParamCount = 1;
    UInt              sLen;
    UInt              i;
    ULong             sSessionSMN = ID_LONG(0);

    IDU_FIT_POINT_FATAL( "qmgShardInsert::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgShardINST*) aGraph;
    sTableInfo = sMyGraph->tableRef->tableInfo;

    /* PROJ-2701 Sharding online data rebuild */
    sSessionSMN = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );

    sdi::getShardObjInfoForSMN( sSessionSMN,
                                sMyGraph->tableRef->mShardObjInfo,
                                &sShardObjInfo );

    IDE_TEST_RAISE( sShardObjInfo == NULL, ERR_NO_SHARD_OBJECT );

    sAnalyzeInfo = &(sMyGraph->shardAnalysis);

    //---------------------------------------------------
    // insert를 위한 analyzeInfo 생성
    //---------------------------------------------------

    IDE_FT_ASSERT( sShardObjInfo != NULL );

    // analyzer를 통하지 않고 직접 analyze 정보를 생성한다.
    IDE_TEST( sdi::setAnalysisResultForInsert( aStatement,
                                               sAnalyzeInfo,
                                               sShardObjInfo )
              != IDE_SUCCESS );

    //---------------------------------------------------
    // insert query 생성
    //---------------------------------------------------

    idlOS::snprintf( sMyGraph->shardQueryBuf,
                     ID_SIZEOF(sMyGraph->shardQueryBuf),
                     "%.*s VALUES (?",
                     sMyGraph->insertPos.size,
                     sMyGraph->insertPos.stmtText + sMyGraph->insertPos.offset );
    sLen = idlOS::strlen( sMyGraph->shardQueryBuf );

    // 첫번째 컬럼은 hidden column이 아니다.
    IDE_FT_ASSERT( sTableInfo->columnCount > 0 );
    IDE_FT_ASSERT( (sTableInfo->columns[0].flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_FALSE );

    for ( i = 1; i < sTableInfo->columnCount; i++ )
    {
        if ( (sTableInfo->columns[i].flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
             == QCM_COLUMN_HIDDEN_COLUMN_FALSE )
        {
            sParamCount++;
            sLen += 2;

            IDE_TEST_RAISE( sLen + 2 >= ID_SIZEOF(sMyGraph->shardQueryBuf),
                            ERR_QUERY_BUFFER_OVERFLOW );
            idlOS::strcat( sMyGraph->shardQueryBuf, ",?" );
        }
        else
        {
            // Nothing to do.
        }
    }

    sLen += 1;
    IDE_TEST_RAISE( sLen + 1 >= ID_SIZEOF(sMyGraph->shardQueryBuf),
                    ERR_QUERY_BUFFER_OVERFLOW );
    idlOS::strcat( sMyGraph->shardQueryBuf, ")" );

    // name position으로 기록한다.
    sMyGraph->shardQuery.stmtText = sMyGraph->shardQueryBuf;
    sMyGraph->shardQuery.offset   = 0;
    sMyGraph->shardQuery.size     = sLen;

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

    IDE_EXCEPTION( ERR_QUERY_BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardInsert::optimize",
                                  "query buffer overflow" ) );
    }
    IDE_EXCEPTION( ERR_NO_SHARD_OBJECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgShardInsert::optimize",
                                  "no shard object information" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgShardInsert::makePlan( qcStatement     * aStatement,
                          const qmgGraph  * aParent,
                          qmgGraph        * aGraph )
{
/***********************************************************************
 *
 * Description : qmgShardInsert으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgShardInsert으로 생성가능한 Plan
 *
 *           [INST]
 *
 ***********************************************************************/

    qmmInsParseTree * sInsParseTree;
    qcStatement     * sSubQStatement;
    qmgShardINST    * sMyGraph;
    qmnPlan         * sPlan;
    qmnPlan         * sChildPlan;
    qmmValueNode    * sValueNode;
    qmmMultiRows    * sMultiRows;
    qmoINSTInfo       sINSTInfo;

    IDU_FIT_POINT_FATAL( "qmgShardInsert::makePlan::__FT__" );

    sInsParseTree = (qmmInsParseTree *)aStatement->myPlan->parseTree;

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );

    sMyGraph = (qmgShardINST*) aGraph;

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
    IDE_FT_ASSERT( aParent == NULL );

    IDE_TEST( qmoOneNonPlan::initSDIN( aStatement ,
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
    sINSTInfo.queueMsgIDSeq     = sMyGraph->queueMsgIDSeq;
    sINSTInfo.nextValSeqs       = sMyGraph->nextValSeqs;

    IDE_TEST( qmoOneNonPlan::makeSDIN( aStatement ,
                                       &sINSTInfo ,
                                       &(sMyGraph->shardQuery),
                                       &(sMyGraph->shardAnalysis),
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
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgShardInsert::printGraph( qcStatement  * aStatement,
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

    qmgShardINST    * sMyGraph;
    UInt              i;

    IDU_FIT_POINT_FATAL( "qmgShardInsert::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );
    IDE_FT_ASSERT( aString != NULL );

    sMyGraph = (qmgShardINST*)aGraph;

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
    // shard 정보 출력
    //-----------------------------------

    IDE_TEST( qmgShardDML::printShardInfo( aStatement,
                                           &(sMyGraph->shardAnalysis),
                                           &(sMyGraph->shardQuery),
                                           aDepth,
                                           aString )
              != IDE_SUCCESS );

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
