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
 * $Id: qmoOneNonPlan.cpp 91445 2021-08-10 01:09:08Z andrew.shin $
 *
 * Description :
 *     Plan Generator
 *
 *     One-child Non-materialized Plan을 생성하기 위한 관리자이다.
 *
 *     다음과 같은 Plan Node의 생성을 관리한다.
 *         - SCAN 노드
 *         - FILT 노드
 *         - PROJ 노드
 *         - GRBY 노드
 *         - AGGR 노드
 *         - CUNT 노드
 *         - VIEW 노드
 *         - VSCN 노드
 *         - SCAN(for Partition) 노드 PROJ-1502 PARTITIONED DISK TABLE
 *         - CNTR 노드 PROJ-1405 ROWNUM
 *         - SDSE 노드
 *         - SDEX 노드
 *         - SDIN 노드
 *
 *     모든 NODE들은 초기화 작업 , 메인 작업 , 마무리 작업 순으로 이루어
 *     진다. 초기화 작업에서는 NODE의 코드 영역의 할당 및 초기화 작업을
 *     하며, 메인작업에는 각 NODE별로 이루어져야 할 작업 , 그리고 마지막
 *     으로 마무리 작업에서는 data영역의 크기, dependency처리, subquery의
 *     처리 등이 이루어진다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcg.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmo.h>
#include <qmoOneNonPlan.h>
#include <qmoRidPred.h>
#include <qdnForeignKey.h>
#include <qmoPartition.h>
#include <qcuTemporaryObj.h>
#include <qmv.h>
#include <qmxResultCache.h>

extern mtfModule mtfEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfOr;
extern mtfModule mtfAnd;
extern mtfModule mtfAdd2;
extern mtfModule mtfSubtract2;
extern mtfModule mtfTo_char;

/*
 * PROJ-1832 New database link
 */
IDE_RC qmoOneNonPlan::allocAndCopyRemoteTableInfo(
    qcStatement             * aStatement,
    struct qmsTableRef      * aTableRef,
    SChar                  ** aDatabaseLinkName,
    SChar                  ** aRemoteQuery )
{
    UInt sLength = 0;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::allocAndCopyRemoteTableInfo::__FT__" );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( aTableRef->remoteTable->linkName.size + 1,
                                               (void **)aDatabaseLinkName )
              != IDE_SUCCESS );

    QC_STR_COPY( *aDatabaseLinkName, aTableRef->remoteTable->linkName );

    sLength = idlOS::strlen( aTableRef->remoteQuery );
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sLength + 1,
                                             (void **)aRemoteQuery )
              != IDE_SUCCESS );

    idlOS::strncpy( *aRemoteQuery,
                    aTableRef->remoteQuery,
                    sLength );
    (*aRemoteQuery)[ sLength ] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qmoOneNonPlan::setCursorPropertiesForRemoteTable(
    qcStatement         * aStatement,
    smiCursorProperties * aCursorProperties,
    idBool                aIsStore,
    SChar               * aDatabaseLinkName,
    SChar               * aRemoteQuery,
    UInt                  aColumnCount,
    qcmColumn           * aColumns )
{
    UInt             i = 0;
    smiRemoteTable * sRemoteTable = NULL;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::setCursorPropertiesForRemoteTable::__FT__" );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( *sRemoteTable ),
                                               (void **)& sRemoteTable )
              != IDE_SUCCESS );

    /* BUG-37077 REMOTE_TABLE_STORE */
    sRemoteTable->mIsStore = aIsStore;

    sRemoteTable->mLinkName = aDatabaseLinkName;
    sRemoteTable->mRemoteQuery = aRemoteQuery;

    sRemoteTable->mColumnCount = aColumnCount;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( *sRemoteTable->mColumns ) * aColumnCount,
                                             (void **)&( sRemoteTable->mColumns ) )
              != IDE_SUCCESS );

    for ( i  = 0;
          i < aColumnCount;
          i++ )
    {
        sRemoteTable->mColumns[i].mName = aColumns[i].name;
        sRemoteTable->mColumns[i].mTypeId = aColumns[i].basicInfo->module->id;
        sRemoteTable->mColumns[i].mPrecision = aColumns[i].basicInfo->precision;
        sRemoteTable->mColumns[i].mScale = aColumns[i].basicInfo->scale;
    }

    aCursorProperties->mRemoteTable = sRemoteTable;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmoOneNonPlan::makeSCAN( qcStatement  * aStatement ,
                                qmsQuerySet  * aQuerySet ,
                                qmsFrom      * aFrom ,
                                qmoSCANInfo  * aSCANInfo ,
                                qmnPlan      * aParent,
                                qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : SCAN 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - data 영역의 크기 계산
 *         - qmncSCAN의 할당 및 초기화 (display 정보 , index 정보)
 *         - limit 구절의 확인
 *         - select for update를 위한 처리
 *     + 메인 작업
 *         - Predicate의 분류
 *             - constant의 처리
 *             - 입력Predicate정보와 index 정보로 부터 Predicate 분류
 *             - fixed , variable의 구분
 *             - qtcNode로 의 변환
 *             - smiRange로의 변환
 *             - indexable min-max의 처리
 *     + 마무리 작업
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncSCAN          * sSCAN;
    UInt                sDataNodeOffset;
    qmsParseTree      * sParseTree;
    qtcNode           * sPredicate[11];
    qcmIndex          * sIndices;
    UInt                i;
    UInt                sIndexCnt;
    qmoPredicate      * sCopiedPred;
    idBool              sScanLimit          = ID_TRUE;
    idBool              sInSubQueryKeyRange = ID_FALSE;
    mtcTuple          * sMtcTuple;
    idBool              sExistLobFilter     = ID_FALSE;
    qcDepInfo           sDepInfo;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeSCAN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aSCANInfo != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDU_FIT_POINT("qmoOneNonPlan::makeSCAN::alloc",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmncSCAN),
                                           (void**)& sSCAN)
             != IDE_SUCCESS);

    QMO_INIT_PLAN_NODE( sSCAN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_SCAN ,
                        qmnSCAN ,
                        qmndSCAN,
                        sDataNodeOffset );

    // PROJ-2444
    sSCAN->plan.readyIt = qmnSCAN::readyIt;

    // BUG-15816
    // data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    // PROJ-1446 Host variable을 포함한 질의 최적화
    sIndices  = aFrom->tableRef->tableInfo->indices;
    sIndexCnt = aFrom->tableRef->tableInfo->indexCount;

    //----------------------------------
    // Table 관련 정보 설정
    //----------------------------------

    sSCAN->tupleRowID = aFrom->tableRef->table;
    sSCAN->table    = aFrom->tableRef->tableInfo->tableHandle;
    sSCAN->tableSCN = aFrom->tableRef->tableSCN;

    /* PROJ-2359 Table/Partition Access Option */
    sSCAN->accessOption = aFrom->tableRef->tableInfo->accessOption;

    // PROJ-1502 PARTITIONED DISK TABLE
    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sSCAN->tupleRowID].tableHandle =
        aFrom->tableRef->tableInfo->tableHandle;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sSCAN->flag = QMN_PLAN_FLAG_CLEAR;
    sSCAN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    if ( qcuTemporaryObj::isTemporaryTable( aFrom->tableRef->tableInfo ) == ID_TRUE )
    {
        sSCAN->flag &= ~QMNC_SCAN_TEMPORARY_TABLE_MASK;
        sSCAN->flag |= QMNC_SCAN_TEMPORARY_TABLE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &(sSCAN->cursorProperty), NULL );

    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement ,
                                     sSCAN->tupleRowID ,
                                     &( sSCAN->plan.flag ) )
              != IDE_SUCCESS );

    // Previous Disable 설정
    sSCAN->flag &= ~QMNC_SCAN_PREVIOUS_ENABLE_MASK;
    sSCAN->flag |= QMNC_SCAN_PREVIOUS_ENABLE_FALSE;

    // fix BUG-12167
    // 참조하는 테이블이 fixed or performance view인지의 정보를 저장
    // 참조하는 테이블에 대한 IS LOCK을 걸지에 대한 판단 기준.
    if ( ( aFrom->tableRef->tableInfo->tableType == QCM_FIXED_TABLE ) ||
         ( aFrom->tableRef->tableInfo->tableType == QCM_DUMP_TABLE ) ||
         ( aFrom->tableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW ) )
    {
        sSCAN->flag &= ~QMNC_SCAN_TABLE_FV_MASK;
        sSCAN->flag |= QMNC_SCAN_TABLE_FV_TRUE;

        if ( QCG_GET_SESSION_OPTIMIZER_PERFORMANCE_VIEW(aStatement) == 1 )
        {
            sSCAN->flag &= ~QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK;
            sSCAN->flag |= QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE;
        }
        else
        {
            /* BUG-43006 FixedTable Indexing Filter
             * optimizer formance view propery 가 0이라면
             * FixedTable 의 index는 없다고 설정해줘야한다
             */
            sIndices  = NULL;
            sIndexCnt = 0;
        }
    }
    else
    {
        sSCAN->flag &= ~QMNC_SCAN_TABLE_FV_MASK;
        sSCAN->flag |= QMNC_SCAN_TABLE_FV_FALSE;
    }

    /* PROJ-1832 New database link */
    if ( aFrom->tableRef->remoteTable != NULL )
    {
        if ( aFrom->tableRef->remoteTable->mIsStore != ID_TRUE )
        {
            sSCAN->flag &= ~QMNC_SCAN_REMOTE_TABLE_MASK;
            sSCAN->flag |= QMNC_SCAN_REMOTE_TABLE_TRUE;
        }
        else
        {
            /* BUG-37077 REMOTE_TABLE_STORE */
            sSCAN->flag &= ~QMNC_SCAN_REMOTE_TABLE_STORE_MASK;
            sSCAN->flag |= QMNC_SCAN_REMOTE_TABLE_STORE_TRUE;
        }
    }
    else
    {
        sSCAN->flag &= ~QMNC_SCAN_REMOTE_TABLE_MASK;
        sSCAN->flag |= QMNC_SCAN_REMOTE_TABLE_FALSE;

        sSCAN->flag &= ~QMNC_SCAN_REMOTE_TABLE_STORE_MASK;
        sSCAN->flag |= QMNC_SCAN_REMOTE_TABLE_STORE_FALSE;
    }

    // PROJ-1705
    // partition table인 경우, tableRef->tableInfo->rowMovement 정보가 필요한데,
    // 이 정보만 따로 관리하지 않고,
    // 모든 테이블에 대해 code node에 tableRef정보를 달아준다.
    sSCAN->tableRef = aFrom->tableRef;

    // PROJ-1618 Online Dump
    if ( aFrom->tableRef->tableInfo->tableType == QCM_DUMP_TABLE )
    {
        IDE_TEST_RAISE( aFrom->tableRef->mDumpObjList == NULL,
                        ERR_EMPTY_DUMP_OBJECT );
        sSCAN->dumpObject = aFrom->tableRef->mDumpObjList->mObjInfo;
    }
    else
    {
        sSCAN->dumpObject = NULL;
    }

    //----------------------------------
    // 인덱스 설정 및 방향 설정
    //----------------------------------

    sSCAN->method.index = aSCANInfo->index;

    // Proj-1360 Queue
    // dequeue 문에서 수행된 경우, 색인이 설정되지 않으면
    // primary key에 해당하는 msgid 색인을 사용
    if ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE )
    {
        sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;
        sParseTree->queue->tableID = aFrom->tableRef->tableInfo->tableID;

        if ( aSCANInfo->index == NULL )
        {
            sSCAN->method.index = aFrom->tableRef->tableInfo->primaryKey;
        }
        else
        {
            /* nothing to do */
        }

        // fifo 옵션이 설정되어 있으면 탐색 순위를 변경한다.
        if ( sParseTree->queue->isFifo == ID_TRUE )
        {
            sSCAN->flag &= ~QMNC_SCAN_TRAVERSE_MASK;
            sSCAN->flag |= QMNC_SCAN_TRAVERSE_FORWARD;
        }
        else
        {
            sSCAN->flag &= ~QMNC_SCAN_TRAVERSE_MASK;
            sSCAN->flag |= QMNC_SCAN_TRAVERSE_BACKWARD;
        }
    }
    else
    {
        // do nothing.
    }

    // BUG-36760 dequeue 시에 무조건 인덱스를 타야함
    if ( sSCAN->method.index != NULL )
    {
        // To Fix PR-11562
        // Indexable MIN-MAX 최적화가 적용된 경우
        // Preserved Order는 방향성을 가짐, 따라서 해당 정보를
        // 설정해줄 필요가 없음.
        // 관련 코드 제거

        //index 정보 및 order by에 따른 traverse 방향 설정
        //aSCANInfo->preservedOrder를 보고 인덱스 방향과 다르면 sSCAN->flag
        //를 BACKWARD로 설정해주어야 한다.

        // fix BUG-31907
        // queue table에 대한 traverse direction은 sParseTree->queue->isFifo에 의해 이미 결정되어 있다.
        if ( aStatement->myPlan->parseTree->stmtKind != QCI_STMT_DEQUEUE )
        {
            IDE_TEST( setDirectionInfo( &( sSCAN->flag ),
                                        aSCANInfo->index,
                                        aSCANInfo->preservedOrder )
                      != IDE_SUCCESS );
        }

        // To fix BUG-12742
        // index scan이 고정되어 있는 경우를 세팅한다.
        if ( ( aSCANInfo->flag & QMO_SCAN_INFO_FORCE_INDEX_SCAN_MASK ) ==
             QMO_SCAN_INFO_FORCE_INDEX_SCAN_TRUE)
        {
            sSCAN->flag &= ~QMNC_SCAN_FORCE_INDEX_SCAN_MASK;
            sSCAN->flag |= QMNC_SCAN_FORCE_INDEX_SCAN_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // nothing to do
    }

    if( ( aSCANInfo->flag & QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK )
        == QMO_SCAN_INFO_NOTNULL_KEYRANGE_TRUE )
    {
        sSCAN->flag &= ~QMNC_SCAN_NOTNULL_RANGE_MASK;
        sSCAN->flag |= QMNC_SCAN_NOTNULL_RANGE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // Cursor Property의 설정
    //----------------------------------

    sSCAN->lockMode = SMI_LOCK_READ;
    sSCAN->cursorProperty.mLockWaitMicroSec = ID_ULONG_MAX;
    
    if ( (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT) ||
         (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE) ||
         (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE) )
    {
        sParseTree      = (qmsParseTree *)aStatement->myPlan->parseTree;
        if( sParseTree->forUpdate != NULL )
        {
            sSCAN->lockMode                         = SMI_LOCK_REPEATABLE;
            sSCAN->cursorProperty.mLockWaitMicroSec =
                sParseTree->forUpdate->lockWaitMicroSec;
            // Proj 1360 Queue
            // dequeue문의 경우, row의 삭제를 위해 exclusive lock이 요구됨
            if (sParseTree->forUpdate->isQueue == ID_TRUE)
            {
                sSCAN->flag   |= QMNC_SCAN_TABLE_QUEUE_TRUE;
                sSCAN->lockMode = SMI_LOCK_WRITE;
            }

            /* BUG-49127 */
            if ( sParseTree->forUpdate->isMoveAndDelete == ID_TRUE )
            {
                sSCAN->flag &= ~QMNC_SCAN_MOVE_AND_DELETE_MASK;
                sSCAN->flag |= QMNC_SCAN_MOVE_AND_DELETE_TRUE;
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

    /* PROJ-1832 New database link */
    if  ( ( ( sSCAN->flag & QMNC_SCAN_REMOTE_TABLE_MASK ) ==
            QMNC_SCAN_REMOTE_TABLE_TRUE ) ||
          ( ( sSCAN->flag & QMNC_SCAN_REMOTE_TABLE_STORE_MASK ) ==
            QMNC_SCAN_REMOTE_TABLE_STORE_TRUE ) )
    {
        IDE_TEST( allocAndCopyRemoteTableInfo( aStatement,
                                               aFrom->tableRef,
                                               &( sSCAN->databaseLinkName ),
                                               &( sSCAN->remoteQuery ) )
                  != IDE_SUCCESS );

        IDE_TEST( setCursorPropertiesForRemoteTable( aStatement,
                                                     &( sSCAN->cursorProperty ),
                                                     aFrom->tableRef->remoteTable->mIsStore,
                                                     sSCAN->databaseLinkName,
                                                     sSCAN->remoteQuery,
                                                     aFrom->tableRef->tableInfo->columnCount,
                                                     aFrom->tableRef->tableInfo->columns )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* PROJ-2402 Parallel Table Scan */
    sSCAN->cursorProperty.mParallelReadProperties.mThreadCnt =
        aSCANInfo->mParallelInfo.mDegree;
    sSCAN->cursorProperty.mParallelReadProperties.mThreadID =
        aSCANInfo->mParallelInfo.mSeqNo;
    
    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    IDE_TEST( qmg::setDisplayInfo( aFrom ,
                                   &( sSCAN->tableOwnerName ),
                                   &( sSCAN->tableName ),
                                   &( sSCAN->aliasName ) )
              != IDE_SUCCESS );

    /* BUG-44520 미사용 Disk Partition의 SCAN Node를 출력하다가,
     *           Partition Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  SCAN Node에서 Partition Name을 보관하도록 수정합니다.
     */
    sSCAN->partitionName[0] = '\0';

    /* BUG-44633 미사용 Disk Partition의 SCAN Node를 출력하다가,
     *           Index Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  SCAN Node에서 Index ID를 보관하도록 수정합니다.
     */
    sSCAN->partitionIndexId = 0;

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // Predicate의 처리
    //----------------------------------

    IDE_TEST( processPredicate( aStatement,
                                aQuerySet,
                                aSCANInfo->predicate,
                                aSCANInfo->constantPredicate,
                                aSCANInfo->ridPredicate,
                                aSCANInfo->index,
                                sSCAN->tupleRowID,
                                &( sSCAN->method.constantFilter ),
                                &( sSCAN->method.filter ),
                                &( sSCAN->method.lobFilter ),
                                &( sSCAN->method.subqueryFilter ),
                                &( sSCAN->method.varKeyRange ),
                                &( sSCAN->method.varKeyFilter ),
                                &( sSCAN->method.varKeyRange4Filter ),
                                &( sSCAN->method.varKeyFilter4Filter ),
                                &( sSCAN->method.fixKeyRange ),
                                &( sSCAN->method.fixKeyFilter ),
                                &( sSCAN->method.fixKeyRange4Print ),
                                &( sSCAN->method.fixKeyFilter4Print ),
                                &( sSCAN->method.ridRange ),
                                & sInSubQueryKeyRange )
              != IDE_SUCCESS );

    IDE_TEST( postProcessScanMethod( aStatement,
                                     & sSCAN->method,
                                     & sScanLimit )
              != IDE_SUCCESS );

    // BUG-20403 : table scan상위에 FILT가 존재하는지
    if ( sSCAN->method.lobFilter != NULL )
    {
        sExistLobFilter = ID_TRUE;
    }

    // queue에는 nnf, lob, subquery filter를 지원하지 않는다.
    if ( (sSCAN->flag & QMNC_SCAN_TABLE_QUEUE_MASK) == QMNC_SCAN_TABLE_QUEUE_TRUE )
    {
        IDE_TEST_RAISE( ( aSCANInfo->nnfFilter != NULL ) ||
                        ( sSCAN->method.lobFilter != NULL ) ||
                        ( sSCAN->method.subqueryFilter != NULL ),
                        ERR_NOT_SUPPORT_FILTER );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // Predicate 관련 Flag 정보 설정
    //----------------------------------
    if ( sInSubQueryKeyRange == ID_TRUE )
    {
        sSCAN->flag &= ~QMNC_SCAN_INSUBQ_KEYRANGE_MASK;
        sSCAN->flag |= QMNC_SCAN_INSUBQ_KEYRANGE_TRUE;
    }
    else
    {
        sSCAN->flag &= ~QMNC_SCAN_INSUBQ_KEYRANGE_MASK;
        sSCAN->flag |= QMNC_SCAN_INSUBQ_KEYRANGE_FALSE;
    }

    // PROJ-1446 Host variable을 포함한 질의 최적화
    sSCAN->sdf = aSCANInfo->sdf;

    if ( aSCANInfo->sdf != NULL )
    {
        IDE_DASSERT( sIndexCnt > 0 );

        // sdf에 basePlan을 단다.
        aSCANInfo->sdf->basePlan = &sSCAN->plan;

        // sdf에 index 개수만큼 후보를 생성한다.
        // 각각의 후보에 filter/key range/key filter 정보를 생성한다.

        aSCANInfo->sdf->candidateCount = sIndexCnt;

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmncScanMethod ) * sIndexCnt,
                                                 (void**)& aSCANInfo->sdf->candidate )
                  != IDE_SUCCESS );

        for ( i =0;
              i <sIndexCnt;
              i++ )
        {
            // selected index에 대해서는 앞에서 sSCAN에 대해 작업을 했으므로,
            // 다시 작업할 필요가 없다.
            // 대신 그자리엔 full scan에 대해서 작업을 한다.
            if ( &sIndices[i] != aSCANInfo->index )
            {
                aSCANInfo->sdf->candidate[i].index = &sIndices[i];
            }
            else
            {
                aSCANInfo->sdf->candidate[i].index = NULL;
            }

            IDE_TEST( qmoPred::deepCopyPredicate( QC_QMP_MEM(aStatement),
                                                  aSCANInfo->sdf->predicate,
                                                  & sCopiedPred )
                      != IDE_SUCCESS );

            IDE_TEST( processPredicate( aStatement,
                                        aQuerySet,
                                        sCopiedPred,
                                        NULL, // aSCANInfo->constantPredicate,
                                        NULL, // aSCANInfo->ridPredicate
                                        aSCANInfo->sdf->candidate[i].index,
                                        sSCAN->tupleRowID,
                                        &( aSCANInfo->sdf->candidate[i].constantFilter ),
                                        &( aSCANInfo->sdf->candidate[i].filter ),
                                        &( aSCANInfo->sdf->candidate[i].lobFilter ),
                                        &( aSCANInfo->sdf->candidate[i].subqueryFilter ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyRange ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyFilter ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyRange4Filter ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyFilter4Filter ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyRange ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyFilter ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyRange4Print ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyFilter4Print ),
                                        &( aSCANInfo->sdf->candidate[i].ridRange ),
                                        & sInSubQueryKeyRange ) // 의미없는 정보임.
                      != IDE_SUCCESS );

            IDE_TEST( postProcessScanMethod( aStatement,
                                             & aSCANInfo->sdf->candidate[i],
                                             & sScanLimit )
                      != IDE_SUCCESS );

            // BUG-20403 : table scan상위에 FILT가 존재하는지
            if( aSCANInfo->sdf->candidate[i].lobFilter != NULL )
            {
                sExistLobFilter = ID_TRUE;
            }
            else
            {
                // Nothing to do...
            }

            // queue에는 nnf, lob, subquery filter를 지원하지 않는다.
            if ( (sSCAN->flag & QMNC_SCAN_TABLE_QUEUE_MASK) == QMNC_SCAN_TABLE_QUEUE_TRUE )
            {
                IDE_TEST_RAISE( ( aSCANInfo->nnfFilter != NULL ) ||
                                ( aSCANInfo->sdf->candidate[i].lobFilter != NULL ) ||
                                ( aSCANInfo->sdf->candidate[i].subqueryFilter != NULL ),
                                ERR_NOT_SUPPORT_FILTER );
            }
            else
            {
                // Nothing to do.
            }

            aSCANInfo->sdf->candidate[i].constantFilter =
                sSCAN->method.constantFilter;

            // fix BUG-19074
            //----------------------------------
            // sdf의 dependency 처리
            //----------------------------------

            sPredicate[0] = aSCANInfo->sdf->candidate[i].fixKeyRange;
            sPredicate[1] = aSCANInfo->sdf->candidate[i].varKeyRange;
            sPredicate[2] = aSCANInfo->sdf->candidate[i].varKeyRange4Filter;
            sPredicate[3] = aSCANInfo->sdf->candidate[i].fixKeyFilter;
            sPredicate[4] = aSCANInfo->sdf->candidate[i].varKeyFilter;
            sPredicate[5] = aSCANInfo->sdf->candidate[i].varKeyFilter4Filter;
            sPredicate[6] = aSCANInfo->sdf->candidate[i].filter;
            sPredicate[7] = aSCANInfo->sdf->candidate[i].subqueryFilter;

            IDE_TEST( qmoDependency::setDependency( aStatement,
                                                    aQuerySet,
                                                    & sSCAN->plan,
                                                    QMO_SCAN_DEPENDENCY,
                                                    sSCAN->tupleRowID,
                                                    NULL,
                                                    8,
                                                    sPredicate,
                                                    0,
                                                    NULL )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do...
    }

    //----------------------------------
    // NNF Filter 구성
    //----------------------------------
    sSCAN->nnfFilter = aSCANInfo->nnfFilter;

    //----------------------------------
    // SCAN limit의 적용
    // bug-7792,20403 , 상위 filter가 존재하면 default값을 세팅하고
    // 그렇지 않은 경우엔 limit정보를 세팅한다.
    //----------------------------------
    // fix BUG-25151 filter가 있는 경우에도 SCAN LIMIT 적용
    // NNF FILTER가 있는 경우에도 scan limit적용하지 않는다.
    if( (sScanLimit == ID_FALSE) ||
        (sExistLobFilter == ID_TRUE) ||
        (sSCAN->nnfFilter != NULL) )
    {
        sSCAN->limit = NULL;

        // PR-13482
        // SCAN Limit을 적용하지 못한경우,
        // selection graph의 limit도 NULL로 만들기 위한 정보
        aSCANInfo->limit = NULL;
    }
    else
    {
        sSCAN->limit = aSCANInfo->limit;
    }

    // 아래는 의미 없는 정보임.
    // 어차피 qmnScan의 firstInit에서 qmncSCAN의 limit 정보를 참조해,
    // qmnd의 cursorProperty의 두 맴버를 세팅함.
    sSCAN->cursorProperty.mFirstReadRecordPos = 0;
    sSCAN->cursorProperty.mIsUndoLogging = ID_TRUE;
    sSCAN->cursorProperty.mReadRecordCount = ID_ULONG_MAX;

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    //----------------------------------
    // PROJ-1473
    // tuple의 column 사용정보 재조정. (예:view내부로의 push projection적용)
    //----------------------------------

    sMtcTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sSCAN->tupleRowID]);

    // BUG-43705 lateral view를 simple view merging을 하지않으면 결과가 다릅니다.
    if ( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_PUSH_PROJ_MASK )
           == MTC_TUPLE_VIEW_PUSH_PROJ_TRUE ) &&
         ( ( sMtcTuple->lflag & MTC_TUPLE_LATERAL_VIEW_REF_MASK )
           == MTC_TUPLE_LATERAL_VIEW_REF_FALSE ) )
    {
        for ( i = 0;
              i < sMtcTuple->columnCount;
              i++ )
        {
            if ( ( sMtcTuple->columns[i].flag &
                   MTC_COLUMN_VIEW_COLUMN_PUSH_MASK )
                 == MTC_COLUMN_VIEW_COLUMN_PUSH_FALSE )
            {
                // BUG-25470
                // OUTER COLUMN REFERENCE가 있는 컬럼은
                // 컬럼정보를 제거하지 않는다.
                if ( ( ( sMtcTuple->columns[i].flag &
                         MTC_COLUMN_USE_TARGET_MASK )
                       == MTC_COLUMN_USE_TARGET_TRUE )
                     &&
                     ( ( sMtcTuple->columns[i].flag &
                         MTC_COLUMN_USE_NOT_TARGET_MASK )
                       == MTC_COLUMN_USE_NOT_TARGET_FALSE )
                     &&
                     ( ( sMtcTuple->columns[i].flag &
                         MTC_COLUMN_OUTER_REFERENCE_MASK )
                       == MTC_COLUMN_OUTER_REFERENCE_FALSE ))
                {
                    // 질의에 사용되지 않는 컬럼으로
                    // view내부로 push projection된 경우임.
                    // 질의에 사용된 컬럼정보제거
                    sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
                    sMtcTuple->columns[i].flag |= MTC_COLUMN_USE_COLUMN_FALSE;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // 질의에 사용된 컬럼
                // Nothing To Do
            }
        }
    }

    //----------------------------------
    // Host 변수를 포함한
    // Constant Expression의 최적화
    //----------------------------------
    if ( sSCAN->nnfFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    sSCAN->nnfFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] = sSCAN->method.fixKeyRange;
    sPredicate[1] = sSCAN->method.varKeyRange;
    sPredicate[2] = sSCAN->method.varKeyRange4Filter;
    sPredicate[3] = sSCAN->method.fixKeyFilter;
    sPredicate[4] = sSCAN->method.varKeyFilter;
    sPredicate[5] = sSCAN->method.varKeyFilter4Filter;
    sPredicate[6] = sSCAN->method.constantFilter;
    sPredicate[7] = sSCAN->method.filter;
    sPredicate[8] = sSCAN->method.subqueryFilter;
    sPredicate[9] = sSCAN->nnfFilter;
    sPredicate[10] = sSCAN->method.ridRange;

    //----------------------------------
    // PROJ-1473
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    for ( i  = 0;
          i <= 10;
          i++ )
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
                                            & sSCAN->plan,
                                            QMO_SCAN_DEPENDENCY,
                                            sSCAN->tupleRowID,
                                            NULL,
                                            11,
                                            sPredicate,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    if ( aParent != NULL )
    {
        IDE_TEST( qmc::pushResultDesc( aStatement,
                                       aQuerySet,
                                       ID_FALSE,
                                       aParent->resultDesc,
                                       & sSCAN->plan.resultDesc )
                  != IDE_SUCCESS );

        // Join predicate이 push down된 경우, SCAN의 depInfo에는 dependency가 여러개일 수 있다.
        // Result descriptor를 정학히 구성하기 위해 SCAN의 ID로 filtering한다.
        qtc::dependencySet( sSCAN->tupleRowID, &sDepInfo );

        IDE_TEST( qmc::filterResultDesc( aStatement,
                                         & sSCAN->plan.resultDesc,
                                         & sDepInfo,
                                         &( aQuerySet->depInfo ) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
        // UPDATE 구문 등은 parent가 NULL일 수 있다.
    }

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sSCAN->plan.mParallelDegree = aFrom->tableRef->mParallelDegree;

    // PROJ-2551 simple query 최적화
    // simple index scan인 경우 fast execute를 수행한다.
    IDE_TEST( checkSimpleSCAN( aStatement, sSCAN ) != IDE_SUCCESS );

    /* PROJ-2632 */
    sSCAN->mSerialFilterOffset = QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize;
    sSCAN->mSerialFilterSize   = 0;
    sSCAN->mSerialFilterCount  = 0;
    sSCAN->mSerialFilterInfo   = NULL;

    IDE_TEST( buildSerialFilterInfo( aStatement,
                                     aQuerySet->SFWGH->hints,
                                     sSCAN->method.filter,
                                     &( sSCAN->mSerialFilterSize ),
                                     &( sSCAN->mSerialFilterCount ),
                                     &( sSCAN->mSerialFilterInfo ) )
              != IDE_SUCCESS );

    if ( sSCAN->mSerialFilterInfo != NULL)
    {
        QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize
            += QTC_GET_SERIAL_EXECUTE_DATA_SIZE( sSCAN->mSerialFilterSize );
    }
    else
    {
        /* Nothing to do */
    }

    *aPlan = (qmnPlan *)sSCAN;

    /* PROJ-2462 Result Cache */
    qmo::addTupleID2ResultCacheStack( aStatement,
                                      sSCAN->tupleRowID );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_DUMP_OBJECT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMO_EMPTY_DUMP_OBJECT));
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_FILTER)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMO_NOT_ALLOWED_LOB_FILTER));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeSCAN4Partition( qcStatement     * aStatement,
                                   qmsQuerySet     * aQuerySet,
                                   qmsFrom         * aFrom,
                                   qmoSCANInfo     * aSCANInfo,
                                   qmsPartitionRef * aPartitionRef,
                                   qmnPlan        ** aPlan )
{
/***********************************************************************
 *
 * Description : SCAN 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - data 영역의 크기 계산
 *         - qmncSCAN의 할당 및 초기화 (display 정보 , index 정보)
 *         - limit 구절의 확인
 *         - select for update를 위한 처리
 *     + 메인 작업
 *         - Predicate의 분류
 *             - constant의 처리
 *             - 입력Predicate정보와 index 정보로 부터 Predicate 분류
 *             - fixed , variable의 구분
 *             - qtcNode로 의 변환
 *             - smiRange로의 변환
 *             - indexable min-max의 처리
 *     + 마무리 작업
 *         - dependency의 처리
 *
 ***********************************************************************/

    qmncSCAN          * sSCAN;
    UInt                sDataNodeOffset;
    qmsParseTree      * sParseTree;
    qtcNode           * sPredicate[10];
    qcmIndex          * sIndices;
    UInt                i;
    UInt                sIndexCnt;
    qmoPredicate      * sCopiedPred;
    idBool              sScanLimit          = ID_TRUE;
    idBool              sInSubQueryKeyRange = ID_FALSE;
    mtcTuple          * sMtcTuple;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeSCAN4Partition::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aSCANInfo!= NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDU_FIT_POINT("qmoOneNonPlan::makeSCAN4Partition::alloc");
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncSCAN ),
                                               (void **)& sSCAN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSCAN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_SCAN ,
                        qmnSCAN ,
                        qmndSCAN,
                        sDataNodeOffset );
    // PROJ-2444
    sSCAN->plan.readyIt         = qmnSCAN::readyIt;

    // BUG-15816
    // data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    // PROJ-1446 Host variable을 포함한 질의 최적화
    sIndices = aPartitionRef->partitionInfo->indices;
    sIndexCnt = aPartitionRef->partitionInfo->indexCount;

    //----------------------------------
    // Table / Partition 관련 정보 설정
    //----------------------------------

    sSCAN->tupleRowID       = aPartitionRef->table;
    sSCAN->table            = aPartitionRef->partitionHandle;
    sSCAN->tableSCN         = aPartitionRef->partitionSCN;
    sSCAN->partitionRef     = aPartitionRef;
    sSCAN->tableRef         = aFrom->tableRef;

    /* PROJ-2359 Table/Partition Access Option */
    sSCAN->accessOption = aPartitionRef->partitionInfo->accessOption;

    // PROJ-1502 PARTITIONED DISK TABLE
    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sSCAN->tupleRowID].tableHandle =
        aPartitionRef->partitionHandle;

    /* PROJ-2462 Result Cache */
    qmo::addTupleID2ResultCacheStack( aStatement,
                                      sSCAN->tupleRowID );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sSCAN->flag = QMN_PLAN_FLAG_CLEAR;
    sSCAN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement ,
                                     sSCAN->tupleRowID ,
                                     &( sSCAN->plan.flag ) )
              != IDE_SUCCESS );


    // tuple의 flag에 partition을 위한 tuple이라고 세팅.
    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sSCAN->tupleRowID].lflag
        &= ~MTC_TUPLE_PARTITION_MASK;

    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sSCAN->tupleRowID].lflag
        |= MTC_TUPLE_PARTITION_TRUE;

    // partition 을 위한 scan이라고 flag설정
    sSCAN->flag &= ~QMNC_SCAN_FOR_PARTITION_MASK;
    sSCAN->flag |= QMNC_SCAN_FOR_PARTITION_TRUE;

    // Previous Disable 설정
    sSCAN->flag &= ~QMNC_SCAN_PREVIOUS_ENABLE_MASK;
    sSCAN->flag |= QMNC_SCAN_PREVIOUS_ENABLE_FALSE;

    sSCAN->flag &= ~QMNC_SCAN_TABLE_FV_MASK;
    sSCAN->flag |= QMNC_SCAN_TABLE_FV_FALSE;

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &(sSCAN->cursorProperty), NULL );

    //----------------------------------
    // 인덱스 설정 및 방향 설정
    //----------------------------------

    sSCAN->method.index        = aSCANInfo->index;

    if( aSCANInfo->index != NULL )
    {
        // To Fix PR-11562
        // Indexable MIN-MAX 최적화가 적용된 경우
        // Preserved Order는 방향성을 가짐, 따라서 해당 정보를
        // 설정해줄 필요가 없음.
        // 관련 코드 제거

        //index 정보 및 order by에 따른 traverse 방향 설정
        //aSCANInfo->preservedOrder를 보고 인덱스 방향과 다르면 sSCAN->flag
        //를 BACKWARD로 설정해주어야 한다.
        IDE_TEST( setDirectionInfo( &( sSCAN->flag ) ,
                                    aSCANInfo->index ,
                                    aSCANInfo->preservedOrder )
                  != IDE_SUCCESS );

        // To fix BUG-12742
        // index scan이 고정되어 있는 경우를 세팅한다.
        if ( ( aSCANInfo->flag & QMO_SCAN_INFO_FORCE_INDEX_SCAN_MASK )
             == QMO_SCAN_INFO_FORCE_INDEX_SCAN_TRUE )
        {
            sSCAN->flag &= ~QMNC_SCAN_FORCE_INDEX_SCAN_MASK;
            sSCAN->flag |= QMNC_SCAN_FORCE_INDEX_SCAN_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // nothing to do
    }

    // rid scan이 고정되어 있는 경우를 세팅한다.
    if ( ( aSCANInfo->flag & QMO_SCAN_INFO_FORCE_RID_SCAN_MASK )
         == QMO_SCAN_INFO_FORCE_RID_SCAN_TRUE)
    {
        sSCAN->flag &= ~QMNC_SCAN_FORCE_RID_SCAN_MASK;
        sSCAN->flag |= QMNC_SCAN_FORCE_RID_SCAN_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    if ( ( aSCANInfo->flag & QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK )
         == QMO_SCAN_INFO_NOTNULL_KEYRANGE_TRUE )
    {
        sSCAN->flag &= ~QMNC_SCAN_NOTNULL_RANGE_MASK;
        sSCAN->flag |= QMNC_SCAN_NOTNULL_RANGE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // Cursor Property의 설정
    //----------------------------------

    SMI_CURSOR_PROP_INIT(&sSCAN->cursorProperty, NULL, NULL);

    sSCAN->lockMode = SMI_LOCK_READ;
    sSCAN->cursorProperty.mLockWaitMicroSec = ID_ULONG_MAX;

    if ( ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT ) ||
         ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE ) ||
         ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE ) )
    {
        sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
        if ( sParseTree->forUpdate != NULL )
        {
            sSCAN->lockMode                         = SMI_LOCK_REPEATABLE;
            sSCAN->cursorProperty.mLockWaitMicroSec =
                sParseTree->forUpdate->lockWaitMicroSec;
            // Proj 1360 Queue
            // dequeue문의 경우, row의 삭제를 위해 exclusive lock이 요구됨
            if (sParseTree->forUpdate->isQueue == ID_TRUE)
            {
                sSCAN->flag   |= QMNC_SCAN_TABLE_QUEUE_TRUE;
                sSCAN->lockMode = SMI_LOCK_WRITE;
            }
            else
            {
                /* Nothing to do */
            }

            /* BUG-49127 */
            if ( sParseTree->forUpdate->isMoveAndDelete == ID_TRUE )
            {
                sSCAN->flag &= ~QMNC_SCAN_MOVE_AND_DELETE_MASK;
                sSCAN->flag |= QMNC_SCAN_MOVE_AND_DELETE_TRUE;
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

    /* PROJ-2402 Parallel Table Scan */
    sSCAN->cursorProperty.mParallelReadProperties.mThreadCnt =
        aSCANInfo->mParallelInfo.mDegree;
    sSCAN->cursorProperty.mParallelReadProperties.mThreadID =
        aSCANInfo->mParallelInfo.mSeqNo;

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    IDE_TEST( qmg::setDisplayInfo( aFrom ,
                                   &( sSCAN->tableOwnerName ) ,
                                   &( sSCAN->tableName ) ,
                                   &( sSCAN->aliasName ) )
              != IDE_SUCCESS );

    /* BUG-44520 미사용 Disk Partition의 SCAN Node를 출력하다가,
     *           Partition Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  SCAN Node에서 Partition Name을 보관하도록 수정합니다.
     */
    (void)idlOS::memcpy( sSCAN->partitionName,
                         aPartitionRef->partitionInfo->name,
                         idlOS::strlen( aPartitionRef->partitionInfo->name ) + 1 );

    /* BUG-44633 미사용 Disk Partition의 SCAN Node를 출력하다가,
     *           Index Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  SCAN Node에서 Index ID를 보관하도록 수정합니다.
     */
    if ( aSCANInfo->index != NULL )
    {
        sSCAN->partitionIndexId = aSCANInfo->index->indexId;
    }
    else
    {
        sSCAN->partitionIndexId = 0;
    }

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // Predicate의 처리
    //----------------------------------

    IDE_TEST( processPredicate( aStatement,
                                aQuerySet,
                                aSCANInfo->predicate,
                                aSCANInfo->constantPredicate,
                                aSCANInfo->ridPredicate,
                                aSCANInfo->index,
                                sSCAN->tupleRowID,
                                &( sSCAN->method.constantFilter ),
                                &( sSCAN->method.filter ),
                                &( sSCAN->method.lobFilter ),
                                &( sSCAN->method.subqueryFilter ),
                                &( sSCAN->method.varKeyRange ),
                                &( sSCAN->method.varKeyFilter ),
                                &( sSCAN->method.varKeyRange4Filter ),
                                &( sSCAN->method.varKeyFilter4Filter ),
                                &( sSCAN->method.fixKeyRange ),
                                &( sSCAN->method.fixKeyFilter ),
                                &( sSCAN->method.fixKeyRange4Print ),
                                &( sSCAN->method.fixKeyFilter4Print ),
                                &( sSCAN->method.ridRange ),
                                & sInSubQueryKeyRange )
              != IDE_SUCCESS );

    IDE_TEST( postProcessScanMethod( aStatement,
                                     & sSCAN->method,
                                     & sScanLimit )
              != IDE_SUCCESS );

    // queue에는 nnf, lob, subquery filter를 지원하지 않는다.
    if ( (sSCAN->flag & QMNC_SCAN_TABLE_QUEUE_MASK) == QMNC_SCAN_TABLE_QUEUE_TRUE )
    {
        IDE_TEST_RAISE( ( aSCANInfo->nnfFilter != NULL ) ||
                        ( sSCAN->method.lobFilter != NULL ) ||
                        ( sSCAN->method.subqueryFilter != NULL ),
                        ERR_NOT_SUPPORT_FILTER );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // Predicate 관련 Flag 정보 설정
    //----------------------------------
    if ( sInSubQueryKeyRange == ID_TRUE )
    {
        IDE_DASSERT(0);
    }
    else
    {
        sSCAN->flag &= ~QMNC_SCAN_INSUBQ_KEYRANGE_MASK;
        sSCAN->flag |= QMNC_SCAN_INSUBQ_KEYRANGE_FALSE;
    }

    // PROJ-1446 Host variable을 포함한 질의 최적화
    sSCAN->sdf = aSCANInfo->sdf;

    if ( aSCANInfo->sdf != NULL )
    {
        IDE_DASSERT( sIndexCnt > 0 );

        // sdf에 basePlan을 단다.
        aSCANInfo->sdf->basePlan = &sSCAN->plan;

        // sdf에 index 개수만큼 후보를 생성한다.
        // 각각의 후보에 filter/key range/key filter 정보를 생성한다.

        aSCANInfo->sdf->candidateCount = sIndexCnt;

        IDU_FIT_POINT( "qmoOneNonPlan::makeSCAN4Partition::alloc::candidate" );
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncScanMethod ) * sIndexCnt,
                                                   (void**)& aSCANInfo->sdf->candidate )
                  != IDE_SUCCESS );

        for ( i = 0;
              i < sIndexCnt;
              i++ )
        {
            // selected index에 대해서는 앞에서 sSCAN에 대해 작업을 했으므로,
            // 다시 작업할 필요가 없다.
            // 대신 그자리엔 full scan에 대해서 작업을 한다.
            if ( &sIndices[i] != aSCANInfo->index )
            {
                aSCANInfo->sdf->candidate[i].index = &sIndices[i];
            }
            else
            {
                aSCANInfo->sdf->candidate[i].index = NULL;
            }

            IDE_TEST( qmoPred::deepCopyPredicate( QC_QMP_MEM(aStatement),
                                                  aSCANInfo->sdf->predicate,
                                                  & sCopiedPred )
                      != IDE_SUCCESS );

            IDE_TEST( processPredicate( aStatement,
                                        aQuerySet,
                                        sCopiedPred,
                                        NULL, //aSCANInfo->constantPredicate,
                                        NULL, // rid predicate
                                        aSCANInfo->sdf->candidate[i].index,
                                        sSCAN->tupleRowID,
                                        &( aSCANInfo->sdf->candidate[i].constantFilter ),
                                        &( aSCANInfo->sdf->candidate[i].filter ),
                                        &( aSCANInfo->sdf->candidate[i].lobFilter ),
                                        &( aSCANInfo->sdf->candidate[i].subqueryFilter ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyRange ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyFilter ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyRange4Filter ),
                                        &( aSCANInfo->sdf->candidate[i].varKeyFilter4Filter ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyRange ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyFilter ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyRange4Print ),
                                        &( aSCANInfo->sdf->candidate[i].fixKeyFilter4Print ),
                                        &( aSCANInfo->sdf->candidate[i].ridRange ),
                                        & sInSubQueryKeyRange ) // 의미없는 정보임.
                      != IDE_SUCCESS );

            IDE_TEST( postProcessScanMethod( aStatement,
                                             & aSCANInfo->sdf->candidate[i],
                                             & sScanLimit )
                      != IDE_SUCCESS );

            // queue에는 nnf, lob, subquery filter를 지원하지 않는다.
            if ( (sSCAN->flag & QMNC_SCAN_TABLE_QUEUE_MASK) == QMNC_SCAN_TABLE_QUEUE_TRUE )
            {
                IDE_TEST_RAISE( ( aSCANInfo->nnfFilter != NULL ) ||
                                ( aSCANInfo->sdf->candidate[i].lobFilter != NULL ) ||
                                ( aSCANInfo->sdf->candidate[i].subqueryFilter != NULL ),
                                ERR_NOT_SUPPORT_FILTER );
            }
            else
            {
                // Nothing to do.
            }

            aSCANInfo->sdf->candidate[i].constantFilter =
                sSCAN->method.constantFilter;

            // fix BUG-19074
            //----------------------------------
            // sdf의 dependency 처리
            //----------------------------------

            sPredicate[0] = aSCANInfo->sdf->candidate[i].fixKeyRange;
            sPredicate[1] = aSCANInfo->sdf->candidate[i].varKeyRange;
            sPredicate[2] = aSCANInfo->sdf->candidate[i].varKeyRange4Filter;
            sPredicate[3] = aSCANInfo->sdf->candidate[i].fixKeyFilter;
            sPredicate[4] = aSCANInfo->sdf->candidate[i].varKeyFilter;
            sPredicate[5] = aSCANInfo->sdf->candidate[i].varKeyFilter4Filter;
            sPredicate[6] = aSCANInfo->sdf->candidate[i].filter;
            sPredicate[7] = aSCANInfo->sdf->candidate[i].subqueryFilter;

            IDE_TEST( qmoDependency::setDependency( aStatement,
                                                    aQuerySet,
                                                    & sSCAN->plan,
                                                    QMO_SCAN_DEPENDENCY,
                                                    sSCAN->tupleRowID,
                                                    NULL,
                                                    8,
                                                    sPredicate,
                                                    0,
                                                    NULL )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do...
    }

    //----------------------------------
    // SCAN limit의 적용
    // bug-7792 , filter가 존재하면 default값을 세팅하고
    // 그렇지 않은 경우엔 limit정보를 세팅한다.
    //----------------------------------
    if ( sScanLimit == ID_FALSE )
    {
        sSCAN->limit = NULL;

        // PR-13482
        // SCAN Limit을 적용하지 못한경우,
        // selection graph의 limit도 NULL로 만들기 위한 정보
        aSCANInfo->limit = NULL;
    }
    else
    {
        sSCAN->limit = aSCANInfo->limit;
    }

    // 아래는 의미 없는 정보임.
    // 어차피 qmnScan의 firstInit에서 qmncSCAN의 limit 정보를 참조해,
    // qmnd의 cursorProperty의 두 맴버를 세팅함.
    sSCAN->cursorProperty.mFirstReadRecordPos = 0;
    sSCAN->cursorProperty.mIsUndoLogging = ID_TRUE;
    sSCAN->cursorProperty.mReadRecordCount = ID_ULONG_MAX;

    //----------------------------------
    // NNF Filter 구성
    //----------------------------------
    sSCAN->nnfFilter = aSCANInfo->nnfFilter;


    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    //----------------------------------
    // PROJ-1473
    // tuple의 column 사용정보 재조정. (예:view내부로의 push projection적용)
    //----------------------------------

    sMtcTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sSCAN->tupleRowID]);

    // BUG-43705 lateral view를 simple view merging을 하지않으면 결과가 다릅니다.
    if ( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_PUSH_PROJ_MASK )
           == MTC_TUPLE_VIEW_PUSH_PROJ_TRUE ) &&
         ( ( sMtcTuple->lflag & MTC_TUPLE_LATERAL_VIEW_REF_MASK )
           == MTC_TUPLE_LATERAL_VIEW_REF_FALSE ) )
    {
        for ( i = 0;
              i < sMtcTuple->columnCount;
              i++ )
        {
            if ( ( sMtcTuple->columns[i].flag &
                   MTC_COLUMN_VIEW_COLUMN_PUSH_MASK )
                 == MTC_COLUMN_VIEW_COLUMN_PUSH_FALSE )
            {
                // BUG-25470
                // OUTER COLUMN REFERENCE가 있는 컬럼은
                // 컬럼정보를 제거하지 않는다.
                if ( ( ( sMtcTuple->columns[i].flag &
                         MTC_COLUMN_USE_TARGET_MASK )
                       == MTC_COLUMN_USE_TARGET_TRUE )
                     &&
                     ( ( sMtcTuple->columns[i].flag &
                         MTC_COLUMN_USE_NOT_TARGET_MASK )
                       == MTC_COLUMN_USE_NOT_TARGET_FALSE )
                     &&
                     ( ( sMtcTuple->columns[i].flag &
                         MTC_COLUMN_OUTER_REFERENCE_MASK )
                       == MTC_COLUMN_OUTER_REFERENCE_FALSE ))
                {
                    // 질의에 사용되지 않는 컬럼으로
                    // view내부로 push projection된 경우임.
                    // 질의에 사용된 컬럼정보제거

                    sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
                    sMtcTuple->columns[i].flag |= MTC_COLUMN_USE_COLUMN_FALSE;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // 질의에 사용된 컬럼
                // Nothing To Do
            }
        }
    }

    //----------------------------------
    // Host 변수를 포함한
    // Constant Expression의 최적화
    //----------------------------------
    if ( sSCAN->nnfFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    sSCAN->nnfFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] = sSCAN->method.fixKeyRange;
    sPredicate[1] = sSCAN->method.varKeyRange;
    sPredicate[2] = sSCAN->method.varKeyRange4Filter;;
    sPredicate[3] = sSCAN->method.fixKeyFilter;
    sPredicate[4] = sSCAN->method.varKeyFilter;
    sPredicate[5] = sSCAN->method.varKeyFilter4Filter;
    sPredicate[6] = sSCAN->method.constantFilter;
    sPredicate[7] = sSCAN->method.filter;
    sPredicate[8] = sSCAN->method.subqueryFilter;
    sPredicate[9] = sSCAN->nnfFilter;

    //----------------------------------
    // PROJ-1473
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    for ( i  = 0;
          i <= 9;
          i++ )
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
                                            & sSCAN->plan,
                                            QMO_SCAN_DEPENDENCY,
                                            sSCAN->tupleRowID,
                                            NULL,
                                            10,
                                            sPredicate,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    sSCAN->plan.resultDesc = NULL;

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sSCAN->plan.mParallelDegree = aFrom->tableRef->mParallelDegree;

    // PROJ-2551 simple query 최적화
    // simple index scan인 경우 fast execute를 수행한다.
    IDE_TEST( checkSimpleSCAN( aStatement, sSCAN ) != IDE_SUCCESS );

    /* PROJ-2632 */
    sSCAN->mSerialFilterOffset = QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize;
    sSCAN->mSerialFilterSize   = 0;
    sSCAN->mSerialFilterCount  = 0;
    sSCAN->mSerialFilterInfo   = NULL;

    IDE_TEST( buildSerialFilterInfo( aStatement,
                                     aQuerySet->SFWGH->hints,
                                     sSCAN->method.filter,
                                     &( sSCAN->mSerialFilterSize ),
                                     &( sSCAN->mSerialFilterCount ),
                                     &( sSCAN->mSerialFilterInfo ) )
              != IDE_SUCCESS );

    if ( sSCAN->mSerialFilterInfo != NULL)
    {
        QC_SHARED_TMPLATE( aStatement )->tmplate.dataSize
            += QTC_GET_SERIAL_EXECUTE_DATA_SIZE( sSCAN->mSerialFilterSize );
    }
    else
    {
        /* Nothing to do */
    }

    *aPlan = (qmnPlan *)sSCAN;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_SUPPORT_FILTER)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMO_NOT_ALLOWED_LOB_FILTER));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::postProcessScanMethod( qcStatement    * aStatement,
                                      qmncScanMethod * aMethod,
                                      idBool         * aScanLimit )
{
    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::postProcessScanMethod::__FT__" );

    IDE_DASSERT( aMethod != NULL );

    // fix BUG-25151 filter가 있는 경우에도 SCAN LIMIT 적용
    if ( aMethod->subqueryFilter != NULL )
    {
        *aScanLimit = ID_FALSE;
    }
    else
    {
        // Nothing To Do
    }

    if ( aMethod->filter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    aMethod->filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    if ( aMethod->subqueryFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    aMethod->subqueryFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // BUG-17506
    if ( aMethod->varKeyRange4Filter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    aMethod->varKeyRange4Filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // BUG-17506
    if ( aMethod->varKeyFilter4Filter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    aMethod->varKeyFilter4Filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    if ( aMethod->ridRange != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aMethod->ridRange )
                  != IDE_SUCCESS );
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
qmoOneNonPlan::initFILT( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qtcNode      * aPredicate ,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : FILT 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncFILT의 할당 및 초기화
 *     + 메인 작업
 *         - filter , constant Predicate의 분류
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncFILT          * sFILT;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initFILT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncFILT ),
                                               (void **)& sFILT )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sFILT ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_FILT ,
                        qmnFILT ,
                        qmndFILT,
                        sDataNodeOffset );

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    if ( aPredicate != NULL )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        & sFILT->plan.resultDesc,
                                        aPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   & sFILT->plan.resultDesc )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sFILT;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeFILT( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qtcNode      * aPredicate ,
                         qmoPredicate * aConstantPredicate ,
                         qmnPlan      * aChildPlan ,
                         qmnPlan      * aPlan )
{
    qmncFILT          * sFILT = (qmncFILT *)aPlan;
    UInt                sDataNodeOffset;
    qtcNode           * sPredicate[10];
    qtcNode           * sNode;
    qtcNode           * sOptimizedNode;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeFILT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aPredicate != NULL || aConstantPredicate != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    sFILT->plan.left        = aChildPlan;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndFILT));

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sFILT->flag = QMN_PLAN_FLAG_CLEAR;
    sFILT->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sFILT->plan.flag       |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    if ( aPredicate != NULL )
    {
        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( aPredicate,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        sFILT->filter = sOptimizedNode;
    }
    else
    {
        sFILT->filter = NULL;
    }

    if ( aConstantPredicate != NULL )
    {
        // constant Predicate의 처리
        IDE_TEST( qmoPred::linkPredicate( aStatement ,
                                          aConstantPredicate ,
                                          & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           sNode )
                  != IDE_SUCCESS );

        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        sFILT->constantFilter = sOptimizedNode;
    }
    else
    {
        sFILT->constantFilter = NULL;
    }

    // TO Fix PR-10182
    // HAVING 절에 존재하는 PRIOR Column에 대해서도 ID를 변경하여야 한다.
    if ( sFILT->filter != NULL )
    {
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           sFILT->filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // Host 변수를 포함한
    // Constant Expression의 최적화
    //----------------------------------

    if ( sFILT->filter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    sFILT->filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sFILT->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    if ( sFILT->filter != NULL )
    {
        IDE_TEST( qmc::duplicateGroupExpression( aStatement,
                                                 sFILT->filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] = sFILT->constantFilter;
    sPredicate[1] = sFILT->filter;

    //----------------------------------
    // PROJ-1473
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[0],
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[1],
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sFILT->plan,
                                            QMO_FILT_DEPENDENCY,
                                            0,
                                            NULL,
                                            2,
                                            sPredicate,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sFILT->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sFILT->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sFILT->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initPROJ( qcStatement  * aStatement,
                         qmsQuerySet  * aQuerySet,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
    qmncPROJ          * sPROJ;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initPROJ::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncPROJ ),
                                               (void **)& sPROJ )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sPROJ ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_PROJ ,
                        qmnPROJ ,
                        qmndPROJ,
                        sDataNodeOffset );

    if ( aParent == NULL )
    {
        // 일반적인 query-set의 projection
        IDE_TEST( qmc::createResultFromQuerySet( aStatement,
                                                 aQuerySet,
                                                 & sPROJ->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Query-set의 최상위 projection이 아닌 경우
        // Ex) view의 하위 projection
        IDE_TEST( qmc::copyResultDesc( aStatement,
                                       aParent->resultDesc,
                                       & sPROJ->plan.resultDesc )
                  != IDE_SUCCESS );
    }

    /* PROJ-2462 Result Cache */
    if ( ( aQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_INVALID_MASK )
         == QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE )
    {
        qmo::flushResultCacheStack( aStatement );
    }
    else
    {
        /* Nothing to do */
    }

    *aPlan = (qmnPlan *)sPROJ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makePROJ( qcStatement  * aStatement,
                         qmsQuerySet  * aQuerySet,
                         UInt           aFlag,
                         qmsLimit     * aLimit,
                         qtcNode      * aLoopNode,
                         qmnPlan      * aChildPlan,
                         qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : PROJ 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncPROJ의 할당 및 초기화
 *     + 메인 작업
 *         - flag의 설정 (top , non-top || limit || indexable min-max )
 *         - Top PROJ , Non-top PROJ일때의 처리
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     - PROJ 노드는 Projection Graph 또는 Set Graph로부터 생성된다.
 *     - Non-top일때의 처리에 대한 고려가 더 필요하다.
 *
 ***********************************************************************/

    qmncPROJ          * sPROJ = (qmncPROJ *)aPlan;
    UInt                sDataNodeOffset;

    qmsTarget         * sTarget;
    qmsTarget         * sNewTarget;
    qmsTarget         * sFirstTarget       = NULL;
    qmsTarget         * sLastTarget        = NULL;

    qtcNode           * sNewNode;
    qtcNode           * sFirstNode         = NULL;
    qtcNode           * sLastNode          = NULL;

    UShort              sColumnCount;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makePROJ::__FT__" );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndPROJ));

    sPROJ->myTargetOffset = sDataNodeOffset;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sPROJ->flag = QMN_PLAN_FLAG_CLEAR;
    sPROJ->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sPROJ->plan.left  = aChildPlan;
    sPROJ->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    // PROJ-2462 Result Cache
    if ( ( aFlag & QMO_MAKEPROJ_TOP_RESULT_CACHE_MASK )
         == QMO_MAKEPROJ_TOP_RESULT_CACHE_TRUE )
    {
        sPROJ->flag &= ~QMNC_PROJ_TOP_RESULT_CACHE_MASK;
        sPROJ->flag |= QMNC_PROJ_TOP_RESULT_CACHE_TRUE;

        sPROJ->limit = NULL;
    }
    else
    {
        // limit 정보의 세팅
        sPROJ->limit = aLimit;
        if( sPROJ->limit != NULL )
        {
            sPROJ->flag &= ~QMNC_PROJ_LIMIT_MASK;
            sPROJ->flag |= QMNC_PROJ_LIMIT_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    // loop 정보의 세팅
    sPROJ->loopNode = aLoopNode;
    
    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    // indexable min-max flag세팅
    if( (aFlag & QMO_MAKEPROJ_INDEXABLE_MINMAX_MASK) ==
        QMO_MAKEPROJ_INDEXABLE_MINMAX_TRUE )
    {
        sPROJ->flag &= ~QMNC_PROJ_MINMAX_MASK;
        sPROJ->flag |= QMNC_PROJ_MINMAX_TRUE;
    }
    else
    {
        sPROJ->flag &= ~QMNC_PROJ_MINMAX_MASK;
        sPROJ->flag |= QMNC_PROJ_MINMAX_FALSE;
    }

    /* PROJ-1071 Parallel Query */
    if ((aFlag & QMO_MAKEPROJ_QUERYSET_TOP_MASK) ==
        QMO_MAKEPROJ_QUERYSET_TOP_TRUE)
    {
        sPROJ->flag &= ~QMNC_PROJ_QUERYSET_TOP_MASK;
        sPROJ->flag |= QMNC_PROJ_QUERYSET_TOP_TRUE;
    }
    else
    {
        sPROJ->flag &= ~QMNC_PROJ_QUERYSET_TOP_MASK;
        sPROJ->flag |= QMNC_PROJ_QUERYSET_TOP_FALSE;
    }

    // To Fix BUG-8026
    if ( aQuerySet->SFWGH != NULL )
    {
        sPROJ->level     = aQuerySet->SFWGH->level;
        sPROJ->isLeaf    = aQuerySet->SFWGH->isLeaf;
        sPROJ->loopLevel = aQuerySet->SFWGH->loopLevel;
    }
    else
    {
        // nothing to do
        sPROJ->level     = NULL;
        sPROJ->isLeaf    = NULL;
        sPROJ->loopLevel = NULL;
    }

    sPROJ->nextValSeqs = aStatement->myPlan->parseTree->nextValSeqs;

    if ( (aFlag & QMO_MAKEPROJ_TOP_MASK ) == QMO_MAKEPROJ_TOP_TRUE )
    {
        //----------------------------------
        // Top PROJ인 경우
        //----------------------------------

        sPROJ->flag &= ~QMNC_PROJ_TOP_MASK;
        sPROJ->flag |= QMNC_PROJ_TOP_TRUE;

        //Top인 경우에는 현재의 QuerySet의 Target을 달아준다.
        sPROJ->myTarget    = aQuerySet->target;

        for ( sItrAttr  = sPROJ->plan.resultDesc, sColumnCount = 0;
              sItrAttr != NULL ;
              sItrAttr  = sItrAttr->next , sColumnCount++ )
        {
            //----------------------------------
            // Prior Node의 처리
            //----------------------------------

            // To Fix BUG-8026
            if ( aQuerySet->SFWGH != NULL )
            {
                IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                                   aQuerySet ,
                                                   sItrAttr->expr )
                          != IDE_SUCCESS );
            }
            else
            {
                // nothing to do
            }
        }
    }
    else
    {
        //----------------------------------
        // Non-top PROJ인 경우
        //----------------------------------
        sPROJ->flag &= ~QMNC_PROJ_TOP_MASK;
        sPROJ->flag |= QMNC_PROJ_TOP_FALSE;

        for ( sTarget  = aQuerySet->target,
                  sColumnCount = 0,
                  sItrAttr = sPROJ->plan.resultDesc;
              sTarget != NULL;
              sTarget  = sTarget->next,
                  sColumnCount++,
                  sItrAttr = sItrAttr->next )
        {
            //----------------------------------
            // Prior Node의 처리
            //----------------------------------
            IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                               aQuerySet ,
                                               sItrAttr->expr )
                      != IDE_SUCCESS );

            //----------------------------------
            // Bug-7948
            // TOP-PROJ가 아닌 경우에 Target을 복사해서 연결한다.
            // 예를 들어 STORE and SEARCH인 경우
            //
            //            (sub-query)
            //               |
            //              (i1)     -    (i2)
            //               ^              ^
            //               |              |
            // target-> (qmsTarget)  -  (qmsTarget)
            //
            // 위와 같이 가리키고 있는 경우 저장을 하는 Materialize노드에서
            // target을 변경 시키므로 하위의 PROJ도 결과 target을 가리키게
            // 된다. (잘못된 상황)
            //
            // 따라서, qmsTarget과 qmsTarget->targetColum을 모두 복사해서
            // PROJ->myTraget에 달아주도록 한다.
            //
            //----------------------------------

            //기존의 Target을 복사한다.
            //alloc
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                    qmsTarget ,
                                    & sNewTarget )
                      != IDE_SUCCESS);
            //memcpy
            idlOS::memcpy( sNewTarget , sTarget , ID_SIZEOF(qmsTarget) );
            sNewTarget->next = NULL;

            //기존의 TargetColumn을 복사한다.
            //alloc
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                    qtcNode ,
                                    & sNewNode )
                      != IDE_SUCCESS);

            idlOS::memcpy( sNewNode,
                           sItrAttr->expr,
                           ID_SIZEOF(qtcNode ) );

            sItrAttr->expr = sNewNode;

            sNewTarget->targetColumn = sNewNode;

            // BUG-37204
            // proj의 result desc 에 pass 노드가 달려있는 경우
            // view 에서 잘못된 노드를 참조 하게 됩니다.
            if ( sItrAttr->expr->node.module == &qtc::passModule )
            {
                // proj 의 상위 그래프에서 pass 노드가 이미 생성되어 내려오는 경우
                // PASS 노드를 복사해야 하고
                // proj 의 하위 그래프에서 pass 노드를 생성하는 경우
                // PASS 노드의 인자를 복사해야 한다.
                // 2가지 경우를 구분할수 없으므로 PASS 노드와 인자를 모두 복사함.
                // 여기서 복사된 복사본은 proj 노드가 사용하게 되며
                // 원본은 view 노드가 내용을 변경하여 사용하게 된다.
                IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                       qtcNode ,
                                       & sNewNode )
                         != IDE_SUCCESS);

                idlOS::memcpy( sNewNode,
                               sItrAttr->expr->node.arguments,
                               ID_SIZEOF(qtcNode ) );

                sItrAttr->expr->node.arguments = (mtcNode*)sNewNode;
            }
            else
            {
                // nothing todo.
            }

            //connect : qmsTarget
            if ( sFirstTarget == NULL )
            {
                sFirstTarget = sNewTarget;
                sLastTarget  = sNewTarget;
            }
            else
            {
                sLastTarget->next = sNewTarget;
                sLastTarget       = sNewTarget;
            }

            //connect : targetColumn
            if ( sFirstNode == NULL )
            {
                sFirstNode = sNewNode;
                sLastNode  = sNewNode;
            }
            else
            {
                sLastNode->node.next = (mtcNode*)sNewNode;
                sLastNode = sNewNode;
            }
        }
        //non-Top인 경우에는 복사한 Target을 달아준다.
        sPROJ->myTarget    = sFirstTarget;
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    sPROJ->targetCount = sColumnCount;
    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdNode) );

    for ( sItrAttr  = sPROJ->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aQuerySet,
                                           sItrAttr->expr,
                                           ID_USHORT_MAX,
                                           ID_FALSE )
                  != IDE_SUCCESS );
    }

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sPROJ->plan ,
                                            QMO_PROJ_DEPENDENCY,
                                            0 ,
                                            sPROJ->myTarget ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sPROJ->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sPROJ->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sPROJ->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    // PROJ-2551 simple query 최적화
    // simple target인 경우 fast execute를 수행한다.
    IDE_TEST( checkSimplePROJ( aStatement, sPROJ )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initGRBY( qcStatement       * aStatement ,
                         qmsQuerySet       * aQuerySet ,
                         qmsAggNode        * aAggrNode ,
                         qmsConcatElement  * aGroupColumn,
                         qmnPlan          ** aPlan )
{
/***********************************************************************
 *
 * Description : GRBY 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncGRBY의 할당 및 초기화
 *     + 메인 작업
 *         - Tuple Set의 등록
 *         - GRBY컬럼의 구성 & passNode의 처리
 *         - Tupple Set의 컬럼 개수에 맞도록 할당
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     -다음의 두가지 경우로 사용이된다.
 *         - Sort-Based Grouping
 *         - Sort-Based Distinction
 *     -따라서, Sort-Based Grouping으로 사용이 될때에는 group by 정보로
 *      부터 이용이 되므로 이는 order by 및 having절에 사용이 될수 있으
 *      므로, srcNode를 복사하여 컬럼을 구성하고, group by에 dstNode또는
 *      passNode로 대체 한다.
 *
 ***********************************************************************/
    qmncGRBY          * sGRBY;
    UInt                sDataNodeOffset;
    UInt                sFlag = 0;

    qmsAggNode        * sAggrNode;
    qmsConcatElement  * sGroupColumn;
    qtcNode           * sNode;
    mtcNode           * sArg;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initGRBY::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncGRBY ),
                                               (void **)& sGRBY )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sGRBY ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_GRBY ,
                        qmnGRBY ,
                        qmndGRBY,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sGRBY;

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    for ( sGroupColumn  = aGroupColumn;
          sGroupColumn != NULL;
          sGroupColumn  = sGroupColumn->next )
    {
        IDE_TEST( qmc::appendAttribute( aStatement,
                                        aQuerySet,
                                        & sGRBY->plan.resultDesc,
                                        sGroupColumn->arithmeticOrList,
                                        sFlag,
                                        QMC_APPEND_EXPRESSION_TRUE | QMC_APPEND_ALLOW_CONST_TRUE,
                                        ID_FALSE )
                  != IDE_SUCCESS );
    }

    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sNode = sAggrNode->aggr;
        while ( sNode->node.module == &qtc::passModule )
        {
            sNode = (qtcNode *)sNode->node.arguments;
        }

        // BUGBUG
        // Aggregate function이 아닌 node가 전달되는 경우가 존재한다.
        /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
        if ( ( QTC_IS_AGGREGATE( sNode ) == ID_TRUE ) &&
             ( sNode->overClause == NULL ) )
        {
            // Argument들을 추가한다.
            for ( sArg  = sNode->node.arguments;
                  sArg != NULL;
                  sArg  = sArg->next )
            {
                // BUG-39975 group_sort improve performence
                // AGGR 노드에서 AGGR 함수 인자에 PASS 노드를 생성함
                // 따라서 PASS 노드를 skip 해야 한다.
                while ( sArg->module == &qtc::passModule )
                {
                    sArg = sArg->arguments;
                }

                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                & sGRBY->plan.resultDesc,
                                                (qtcNode *)sArg,
                                                0,
                                                0,
                                                ID_TRUE ) // BUG-46249
                          != IDE_SUCCESS );
            }
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

// Sort-based distinct용 GRBY
IDE_RC
qmoOneNonPlan::initGRBY( qcStatement       * aStatement ,
                         qmsQuerySet       * aQuerySet ,
                         qmnPlan           * aParent,
                         qmnPlan          ** aPlan )
{
    qmncGRBY          * sGRBY;
    UInt                sDataNodeOffset;
    UInt                sFlag   = 0;
    UInt                sOption = 0;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initGRBY::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncGRBY의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncGRBY ),
                                               (void **)& sGRBY )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sGRBY ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_GRBY ,
                        qmnGRBY ,
                        qmndGRBY,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sGRBY;

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    sOption &= ~QMC_APPEND_EXPRESSION_MASK;
    sOption |= QMC_APPEND_EXPRESSION_TRUE;

    sOption &= ~QMC_APPEND_ALLOW_CONST_MASK;
    sOption |= QMC_APPEND_ALLOW_CONST_TRUE;

    sOption &= ~QMC_APPEND_ALLOW_DUP_MASK;
    sOption |= QMC_APPEND_ALLOW_DUP_FALSE;

    for ( sItrAttr  = aParent->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        // Distinct 대상이 아닌 경우 제외한다.
        // Ex) order by절의 expression
        if ( ( sItrAttr->flag & QMC_ATTR_DISTINCT_MASK )
             == QMC_ATTR_DISTINCT_TRUE )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sGRBY->plan.resultDesc,
                                            sItrAttr->expr,
                                            sFlag,
                                            sOption,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST( qmc::makeReferenceResult( aStatement,
                                        ( aParent->type == QMN_PROJ ? ID_TRUE : ID_FALSE ),
                                        aParent->resultDesc,
                                        sGRBY->plan.resultDesc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeGRBY( qcStatement      * aStatement ,
                         qmsQuerySet      * aQuerySet ,
                         UInt               aFlag ,
                         qmnPlan          * aChildPlan ,
                         qmnPlan          * aPlan )
{
    qmncGRBY          * sGRBY = (qmncGRBY *)aPlan;
    UInt                sDataNodeOffset;

    qmcMtrNode        * sMtrNode[2];

    UShort              sTupleID;
    UShort              sColumnCount  = 0;

    qmcMtrNode        * sNewMtrNode;
    qmcMtrNode        * sFirstMtrNode = NULL;
    qmcMtrNode        * sLastMtrNode  = NULL;

    UInt                sFlag;

    mtcTemplate       * sMtcTemplate;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeGRBY::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndGRBY));

    sGRBY->plan.left        = aChildPlan;
    sGRBY->mtrNodeOffset    = sDataNodeOffset;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sGRBY->flag             = QMN_PLAN_FLAG_CLEAR;
    sGRBY->plan.flag        = QMN_PLAN_FLAG_CLEAR;

    sGRBY->plan.flag       |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // 튜플의 할당
    //----------------------------------
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    // To Fix PR-8493
    // GROUP BY 컬럼의 대체 여부를 결정하기 위해서는
    // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
    sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

    if ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        if ( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
             == QMN_PLAN_STORAGE_DISK )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
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

    // To Fix PR-7940
    sGRBY->myNode = NULL;

    sFlag = 0;
    sFlag &= ~QMC_MTR_GROUPING_MASK;
    sFlag |= QMC_MTR_GROUPING_TRUE;

    sGRBY->baseTableCount = 0;

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        // Grouping key인 경우
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK )
             == QMC_ATTR_KEY_TRUE )
        {
            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;

            sNewMtrNode->flag &= ~QMC_MTR_GROUPING_MASK;
            sNewMtrNode->flag |= QMC_MTR_GROUPING_TRUE;

            sNewMtrNode->flag &= ~QMC_MTR_MTR_PLAN_MASK;
            sNewMtrNode->flag |= QMC_MTR_MTR_PLAN_TRUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // GRBY,AGGR,WNST는 제외한다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
    }

    sGRBY->myNode = sFirstMtrNode;

    switch ( aFlag & QMO_MAKEGRBY_SORT_BASED_METHOD_MASK )
    {
        case QMO_MAKEGRBY_SORT_BASED_GROUPING :
            sGRBY->flag &= ~QMNC_GRBY_METHOD_MASK;
            sGRBY->flag |= QMNC_GRBY_METHOD_GROUPING;
            break;
        default:
            sGRBY->flag &= ~QMNC_GRBY_METHOD_MASK;
            sGRBY->flag |= QMNC_GRBY_METHOD_DISTINCTION;
    }

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    IDE_TEST( qtc::allocIntermediateTuple( aStatement ,
                                           & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS);

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_FALSE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

    if ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        if ( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
             == QMN_PLAN_STORAGE_DISK )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
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

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sGRBY->myNode )
              != IDE_SUCCESS);

    //----------------------------------
    // PROJ-1473 column locate 지정.
    //----------------------------------

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sGRBY->myNode )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sGRBY->myNode )
              != IDE_SUCCESS );

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    for ( sNewMtrNode  = sGRBY->myNode , sColumnCount = 0 ;
          sNewMtrNode != NULL;
          sNewMtrNode  = sNewMtrNode->next , sColumnCount++ ) ;

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    sMtrNode[0]  = sGRBY->myNode;
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sGRBY->plan ,
                                            QMO_GRBY_DEPENDENCY,
                                            sTupleID ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            sMtrNode )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sGRBY->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sGRBY->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sGRBY->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoOneNonPlan::initAGGR( qcStatement       * aStatement ,
                         qmsQuerySet       * aQuerySet ,
                         qmsAggNode        * aAggrNode ,
                         qmsConcatElement  * aGroupColumn,
                         qmnPlan           * aParent,
                         qmnPlan          ** aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncAGGR의 할당 및 초기화
 *     + 메인 작업
 *         - Tuple Set의 등록
 *         - AGGR 컬럼의 구성 & passNode의 처리
 *             - myNode의 구성
 *               (aggregation에 대한 컬럼 + grouping에 대한 컬럼)
 *             - distNode의 구성
 *               (myNode중 distinct에 대한 컬럼,
 *                qmvQTC::isEquivalentExpression()으로 중복된 expression 제거)
 *              - Tuple Set의 컬럼 개수에 맞도록 할당
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *    - Aggregation 속의 DISTINCT column에 대해서는 각각의 Temp Table이
 *      생성되므로 각각 Tuple할당이 필요하며, 같은 expresion이 있는지
 *      검사해보아야 한다. 또한, grouping 그래프로 부터 distinct column과
 *      BucketCnt를 받아 들여야 한다. (입력인자가 빠져 있음)
 *      이를 qmcMtrNode.bucketCnt에 세팅한다.
 *
 *     -Grouping의 컬럼 정보는 group by 정보로
 *      부터 이용이 되므로 이는 order by 및 having절에 사용이 될수 있으
 *      므로, srcNode를 복사하여 컬럼을 구성하고, group by에 dstNode또는
 *      passNode로 대체 한다.
 *
 ***********************************************************************/

    qmncAGGR          * sAGGR;
    UInt                sDataNodeOffset;
    UInt                sFlag = 0;

    qmsAggNode        * sAggrNode;
    qmsConcatElement  * sGroupColumn;
    qtcNode           * sNode;
    qmcAttrDesc       * sResultDesc;
    qmcAttrDesc       * sFind;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initAGGR::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aAggrNode  != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncHSDS의 할당 및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncAGGR ),
                                               (void **)& sAGGR )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sAGGR ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_AGGR ,
                        qmnAGGR ,
                        qmndAGGR,
                        sDataNodeOffset );

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    sFlag &= ~QMC_ATTR_KEY_MASK;
    sFlag |= QMC_ATTR_KEY_TRUE;

    // BUG-41565
    // AGGR 함수에 컨버젼이 달려있으면 결과가 틀려집니다.
    // 상위 플랜에서 컨버젼이 있으면 qtcNode 를 새로 생성하기 때문에
    // 상위 플랜의 result desc 의 것을 추가해 주어야 같은 qtcNode 를 공유할수 있다.
    if ( aParent->type != QMN_GRAG )
    {
        // 상위 플랜이 GRAG 이면 추가하지 않는다. 잘못된 AGGR 을 추가하게됨
        // select max(count(i1)), sum(i1) from t1 group by i1; 일때
        // GRAG1 -> max(count(i1)), sum(i1)
        // GRAG2 -> count(i1) 가 처리된다.
        for ( sResultDesc  = aParent->resultDesc;
              sResultDesc != NULL;
              sResultDesc  = sResultDesc->next )
        {
            // nested aggr x, over x
            if ( ( sResultDesc->expr->overClause == NULL ) &&
                 ( QTC_IS_AGGREGATE( sResultDesc->expr ) == ID_TRUE ) &&
                 ( QTC_HAVE_AGGREGATE2( sResultDesc->expr ) == ID_FALSE ) )
            {
                IDE_TEST( qmc::appendAttribute( aStatement,
                                                aQuerySet,
                                                & sAGGR->plan.resultDesc,
                                                sResultDesc->expr,
                                                sFlag,
                                                QMC_APPEND_EXPRESSION_TRUE,
                                                ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else
    {
        // Nothing To Do
    }

    for ( sGroupColumn  = aGroupColumn;
          sGroupColumn != NULL;
          sGroupColumn  = sGroupColumn->next )
    {
        // 하위에서 grouping된 결과를 받으므로 expression이라 하더라도
        // function mask를 씌우지 않고 결과를 참조하도록 한다.

        // BUG-43542 group by 컬럼이 상수일때도 result desc 에 넣어야 한다.
        IDE_TEST( qmc::appendAttribute( aStatement,
                                        aQuerySet,
                                        & sAGGR->plan.resultDesc,
                                        sGroupColumn->arithmeticOrList,
                                        0,
                                        QMC_APPEND_EXPRESSION_TRUE |
                                        QMC_APPEND_ALLOW_CONST_TRUE,
                                        ID_FALSE )
                     != IDE_SUCCESS );
    }

    for ( sAggrNode  = aAggrNode;
          sAggrNode != NULL;
          sAggrNode  = sAggrNode->next )
    {
        sNode = sAggrNode->aggr;
        while ( sNode->node.module == &qtc::passModule )
        {
            sNode = (qtcNode *)sNode->node.arguments;
        }

        // BUGBUG
        // Aggregate function이 아닌 node가 전달되는 경우가 존재한다.
        /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
        if ( ( QTC_IS_AGGREGATE( sNode ) == ID_TRUE ) &&
             ( sNode->overClause == NULL ) )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sAGGR->plan.resultDesc,
                                            sNode,
                                            sFlag,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-39975 group_sort improve performence
    // BUG-39857 의 수정사항은 group_sort 의 성능을 다소 떨어뜨리게 한다.
    // AGGR 함수 인자에 PASS 노드를 생성하여 성능을 향상시킨다.
    for ( sResultDesc  = sAGGR->plan.resultDesc;
          sResultDesc != NULL;
          sResultDesc  = sResultDesc->next )
    {
        // count(*) 의 경우 arguments 가 null 이다.
        if ( (QTC_IS_AGGREGATE( sResultDesc->expr ) == ID_TRUE) &&
             (sResultDesc->expr->node.arguments != NULL) )
        {
            IDE_TEST( qmc::findAttribute( aStatement,
                                          sAGGR->plan.resultDesc,
                                          (qtcNode*)sResultDesc->expr->node.arguments,
                                          ID_TRUE,
                                          & sFind )
                      != IDE_SUCCESS );

            if ( sFind != NULL )
            {
                IDE_TEST( qmc::makeReference( aStatement,
                                              ID_TRUE,
                                              sFind->expr,
                                              (qtcNode**)&( sResultDesc->expr->node.arguments ) )
                          != IDE_SUCCESS );
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

    *aPlan = (qmnPlan *)sAGGR;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeAGGR( qcStatement      * aStatement ,
                         qmsQuerySet      * aQuerySet ,
                         UInt               aFlag ,
                         qmoDistAggArg    * aDistAggArg,
                         qmnPlan          * aChildPlan ,
                         qmnPlan          * aPlan )
{
    qmncAGGR          * sAGGR          = (qmncAGGR *)aPlan;
    UInt                sDataNodeOffset;

    qmcMtrNode        * sMtrNode[2];

    UShort              sTupleID;
    UShort              sColumnCount = 0;

    UShort              sMtrNodeCount;
    UShort              sAggrNodeCount = 0;
    UShort              sDistNodeCount = 0;

    qmcMtrNode        * sNewMtrNode;
    qmcMtrNode        * sFirstMtrNode = NULL;
    qmcMtrNode        * sLastMtrNode  = NULL;
    mtcTemplate       * sMtcTemplate;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeAGGR::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndAGGR));

    sAGGR->plan.left = aChildPlan;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------
    sAGGR->flag      = QMN_PLAN_FLAG_CLEAR;
    sAGGR->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sAGGR->plan.flag       |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    //----------------------------------
    // 튜플의 할당
    //----------------------------------
    IDE_TEST( qtc::nextTable( & sTupleID,
                              aStatement,
                              NULL,
                              ID_TRUE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    // To Fix PR-8493
    // GROUP BY 컬럼의 대체 여부를 결정하기 위해서는
    // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
    sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

    if ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        if ( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
             == QMN_PLAN_STORAGE_DISK )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
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

    //GRAPH에서 지정한 저장매체를 사용한다.
    if ( ( aFlag & QMO_MAKEAGGR_TEMP_TABLE_MASK )
         == QMO_MAKEAGGR_MEMORY_TEMP_TABLE )
    {
        sAGGR->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sAGGR->plan.flag  |= QMN_PLAN_STORAGE_MEMORY;
    }
    else
    {
        sAGGR->plan.flag  &= ~QMN_PLAN_STORAGE_MASK;
        sAGGR->plan.flag  |= QMN_PLAN_STORAGE_DISK;
    }

    sAGGR->myNode = NULL;
    sAGGR->distNode = NULL;
    sAGGR->baseTableCount = 0;

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        // Aggregate function인 경우
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_TRUE )
        {
            sAggrNodeCount++;

            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // GRBY,AGGR,WNST는 제외한다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            // BUG-8076
            if ( ( sItrAttr->expr->node.lflag & MTC_NODE_DISTINCT_MASK )
                 == MTC_NODE_DISTINCT_FALSE )
            {
                //----------------------------------
                // Distinction이 존재하지 않을 경우
                //----------------------------------
                sNewMtrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                sNewMtrNode->flag |= QMC_MTR_DIST_AGGR_FALSE;
                sNewMtrNode->myDist = NULL;
            }
            else
            {
                //----------------------------------
                // Distinction이 존재 하는 경우
                //----------------------------------

                sNewMtrNode->flag &= ~QMC_MTR_DIST_AGGR_MASK;
                sNewMtrNode->flag |= QMC_MTR_DIST_AGGR_TRUE;

                //----------------------------------
                // To Fix BUG-7604
                // 동일한 Distinct Argument 존재 유무 검사
                //    - 동일한 Distinct Argument가 있을 경우,
                //      이를 통합하여 처리하기 위함
                //----------------------------------
                IDE_TEST( qmg::makeDistNode( aStatement,
                                             aQuerySet,
                                             sAGGR->plan.flag,
                                             aChildPlan,
                                             sTupleID,
                                             aDistAggArg,
                                             sItrAttr->expr,
                                             sNewMtrNode,
                                             & sAGGR->distNode,
                                             & sDistNodeCount )
                          != IDE_SUCCESS );
            }

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
    }

    sAGGR->flag &= ~QMNC_AGGR_GROUPED_MASK;
    sAGGR->flag |= QMNC_AGGR_GROUPED_FALSE;

    for ( sItrAttr  = aPlan->resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        // Grouping key인 경우
        if ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_FALSE )
        {
            sAGGR->flag &= ~QMNC_AGGR_GROUPED_MASK;
            sAGGR->flag |= QMNC_AGGR_GROUPED_TRUE;

            IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                              aQuerySet,
                                              sItrAttr->expr,
                                              ID_FALSE,
                                              sTupleID,
                                              0,
                                              & sColumnCount,
                                              & sNewMtrNode )
                      != IDE_SUCCESS );

            sNewMtrNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
            sNewMtrNode->flag |= QMC_MTR_SORT_ASCENDING;

            sNewMtrNode->flag &= ~QMC_MTR_GROUPING_MASK;
            sNewMtrNode->flag |= QMC_MTR_GROUPING_TRUE;

            sNewMtrNode->flag &= ~QMC_MTR_MTR_PLAN_MASK;
            sNewMtrNode->flag |= QMC_MTR_MTR_PLAN_TRUE;

            // PROJ-2362 memory temp 저장 효율성 개선
            // GRBY,AGGR,WNST는 제외한다.
            sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE;

            if ( sFirstMtrNode == NULL )
            {
                sFirstMtrNode       = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
            else
            {
                sLastMtrNode->next  = sNewMtrNode;
                sLastMtrNode        = sNewMtrNode;
            }
        }
    }

    sAGGR->myNode = sFirstMtrNode;

    IDE_TEST( qtc::allocIntermediateTuple( aStatement ,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                           sTupleID ,
                                           sColumnCount )
              != IDE_SUCCESS);

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_FALSE;

    //myNode를 위한 Tuple은 항상 메모리이다.
    sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

    if ( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
    {
        if ( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
             == QMN_PLAN_STORAGE_DISK )
        {
            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
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

    //컬럼 정보및 execute정보의 세팅
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sAGGR->myNode )
              != IDE_SUCCESS);

    //----------------------------------
    // PROJ-1473 column locate 지정.
    //----------------------------------

    IDE_TEST( qmg::setColumnLocate( aStatement,
                                    sAGGR->myNode )
              != IDE_SUCCESS );

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    // 저장 Column의 data 영역 지정
    sAGGR->mtrNodeOffset  = sDataNodeOffset;

    for ( sNewMtrNode  = sAGGR->myNode , sMtrNodeCount = 0 ;
          sNewMtrNode != NULL ;
          sNewMtrNode  = sNewMtrNode->next , sMtrNodeCount++ ) ;

    // aggregation column의 data 영역 지정
    sDataNodeOffset += sMtrNodeCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );
    sAGGR->aggrNodeOffset = sDataNodeOffset;

    // distinct column의 data영역 지정
    sDataNodeOffset += sAggrNodeCount * idlOS::align8( ID_SIZEOF(qmdAggrNode) );
    sAGGR->distNodeOffset = sDataNodeOffset;

    // data 영역의 크기 조정
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sDistNodeCount * idlOS::align8( ID_SIZEOF(qmdDistNode) );

    //----------------------------------
    //dependency 처리 및 subquery 처리
    //----------------------------------

    sMtrNode[0]  = sAGGR->myNode;
    sMtrNode[1]  = sAGGR->distNode;
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sAGGR->plan ,
                                            QMO_AGGR_DEPENDENCY,
                                            sTupleID ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            2 ,
                                            sMtrNode )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sAGGR->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sAGGR->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sAGGR->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initCUNT( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : CUNT 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - data 영역의 크기 계산
 *         - qmncCUNT의 할당 및 초기화 (display 정보 , index 정보)
 *     + 메인 작업
 *         - countNode의 세팅
 *         - Predicate의 분류
 *     + 마무리 작업
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *     - CUNT , HIER , SCAN의 작업은 모두 유사하므로 이를 다른 interface로
 *       제공 하도록 한다.
 *
 ***********************************************************************/

    qmncCUNT          * sCUNT;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initCUNT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement       != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    //qmncCUNT의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncCUNT ),
                                               (void **)& sCUNT )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sCUNT ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_CUNT ,
                        qmnCUNT ,
                        qmndCUNT,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sCUNT;

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   & sCUNT->plan.resultDesc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeCUNT( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmsFrom      * aFrom ,
                         qmsAggNode   * aCountNode,
                         qmoLeafInfo  * aLeafInfo ,
                         qmnPlan      * aPlan )
{
    qmncCUNT          * sCUNT = (qmncCUNT *)aPlan;
    UInt                sDataNodeOffset;
    qmsParseTree      * sParseTree;
    qtcNode           * sPredicate[10];
    idBool              sInSubQueryKeyRange = ID_FALSE;
    qcmIndex          * sIndices;
    UInt                i;
    UInt                sIndexCnt;
    qmoPredicate      * sCopiedPred;
    qmsPartitionRef   * sPartitionRef       = NULL;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeCUNT::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement       != NULL );
    IDE_DASSERT( aFrom            != NULL );
    IDE_DASSERT( aQuerySet        != NULL );
    IDE_DASSERT( aCountNode       != NULL );
    IDE_DASSERT( aCountNode->next == NULL );
    IDE_DASSERT( aLeafInfo != NULL );
    IDE_DASSERT( aLeafInfo->levelPredicate == NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // PROJ-1446 Host variable을 포함한 질의 최적화
    sIndices = aFrom->tableRef->tableInfo->indices;
    sIndexCnt = aFrom->tableRef->tableInfo->indexCount;

    sParseTree      = (qmsParseTree *)aStatement->myPlan->parseTree;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndCUNT));

    // BUG-15816
    // data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    sCUNT->tupleRowID       = aFrom->tableRef->table;
    // BUG-17483 파티션 테이블 count(*) 지원
    // tableHandel 대신에 tableRef 를 저장한다.
    sCUNT->tableRef         = aFrom->tableRef;
    sCUNT->tableSCN         = aFrom->tableRef->tableSCN;

    //----------------------------------
    // 인덱스 설정 및 방향 설정
    //----------------------------------

    sCUNT->method.index        = aLeafInfo->index;
    
    //----------------------------------
    // Cursor Property의 설정
    //----------------------------------

    sCUNT->lockMode                           = SMI_LOCK_READ;
    
    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &(sCUNT->cursorProperty), NULL );

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sCUNT->flag = QMN_PLAN_FLAG_CLEAR;
    sCUNT->plan.flag = QMN_PLAN_FLAG_CLEAR;

    if ( aLeafInfo->index != NULL )
    {
        // To fix BUG-12742
        // index scan이 고정되어 있는 경우를 세팅한다.
        if ( aLeafInfo->forceIndexScan == ID_TRUE )
        {
            sCUNT->flag &= ~QMNC_CUNT_FORCE_INDEX_SCAN_MASK;
            sCUNT->flag |= QMNC_CUNT_FORCE_INDEX_SCAN_TRUE;
        }
        else
        {
            sCUNT->flag &= ~QMNC_CUNT_FORCE_INDEX_SCAN_MASK;
            sCUNT->flag |= QMNC_CUNT_FORCE_INDEX_SCAN_FALSE;
        }
    }
    else
    {
        // Nothing to do.
    }

    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement ,
                                     sCUNT->tupleRowID ,
                                     &( sCUNT->plan.flag ) )
              != IDE_SUCCESS );

    //------------------------------------------------------------
    // 메인 작업
    //------------------------------------------------------------

    //----------------------------------
    // COUNT(*) 최적화 방법 결정
    //----------------------------------

    // bug-8337
    // A3로 부터의 포팅이 필요하여 현재 무조건 CURSOR로 부터 처리
    // sCUNT->flag        &= ~QMNC_CUNT_METHOD_MASK;
    // sCUNT->flag        |= QMNC_CUNT_METHOD_CURSOR;

    // To Fix PR-13329
    // COUNT(*) 최적화 후 Handle을 사용할 것인지 Cursor를 사용할 것인지의 여부
    // CURSOR 를 사용하는 경우
    //    - 조건이 존재, SET이 존재, Fixed Table인 경우
    if ( ( aLeafInfo->constantPredicate  != NULL ) ||
         ( aLeafInfo->predicate          != NULL ) ||
         ( aLeafInfo->ridPredicate       != NULL ) ||
         ( sParseTree->querySet->setOp   != QMS_NONE ) )
    {
        sCUNT->flag &= ~QMNC_CUNT_METHOD_MASK;
        sCUNT->flag |= QMNC_CUNT_METHOD_CURSOR;
    }
    else
    {
        sCUNT->flag &= ~QMNC_CUNT_METHOD_MASK;
        sCUNT->flag |= QMNC_CUNT_METHOD_HANDLE;
    }

    // fix BUG-12167
    // 참조하는 테이블이 fixed or performance view인지의 정보를 저장
    // 참조하는 테이블에 대한 IS LOCK을 걸지에 대한 판단 기준.
    if( ( aFrom->tableRef->tableInfo->tableType == QCM_FIXED_TABLE ) ||
        ( aFrom->tableRef->tableInfo->tableType == QCM_DUMP_TABLE ) ||
        ( aFrom->tableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW ) )
    {
        sCUNT->flag &= ~QMNC_CUNT_TABLE_FV_MASK;
        sCUNT->flag |= QMNC_CUNT_TABLE_FV_TRUE;

        // To Fix PR-13329
        // Fixed Table인 경우 Handle로부터 그 정보를 얻으면 안됨.
        sCUNT->flag &= ~QMNC_CUNT_METHOD_MASK;
        sCUNT->flag |= QMNC_CUNT_METHOD_CURSOR;

        /* BUG-42639 Monitoring query */
        if ( QCG_GET_SESSION_OPTIMIZER_PERFORMANCE_VIEW(aStatement) == 1 )
        {
            sCUNT->flag &= ~QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK;
            sCUNT->flag |= QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE;
        }
        else
        {
            /* BUG-43006 FixedTable Indexing Filter
             * optimizer formance view propery 가 0이라면
             * FixedTable 의 index는 없다고 설정해줘야한다
             */
            sIndices  = NULL;
            sIndexCnt = 0;
        }
    }
    else
    {
        sCUNT->flag &= ~QMNC_CUNT_TABLE_FV_MASK;
        sCUNT->flag |= QMNC_CUNT_TABLE_FV_FALSE;
    }

    if ( qcuTemporaryObj::isTemporaryTable( aFrom->tableRef->tableInfo ) == ID_TRUE )
    {
        sCUNT->flag &= ~QMNC_SCAN_TEMPORARY_TABLE_MASK;
        sCUNT->flag |= QMNC_SCAN_TEMPORARY_TABLE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // BUG-16651
    if ( aFrom->tableRef->tableInfo->tableType == QCM_DUMP_TABLE )
    {
        IDE_TEST_RAISE( aFrom->tableRef->mDumpObjList == NULL,
                        ERR_EMPTY_DUMP_OBJECT );
        sCUNT->dumpObject = aFrom->tableRef->mDumpObjList->mObjInfo;
    }
    else
    {
        sCUNT->dumpObject = NULL;
    }

    //count(*)노드 세팅
    //PR-8784 , 복사해서 사용하도록 한다.
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qtcNode ,
                            &( sCUNT->countNode ) )
              != IDE_SUCCESS);
    idlOS::memcpy( sCUNT->countNode ,
                   aCountNode->aggr ,
                   ID_SIZEOF(qtcNode) );

    if ( sCUNT->tableRef->partitionRef == NULL )
    {
        /* PROJ-2462 Result Cache */
        qmo::addTupleID2ResultCacheStack( aStatement,
                                          sCUNT->tupleRowID );
    }
    else
    {
        for ( sPartitionRef  = sCUNT->tableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next )
        {
            /* PROJ-2462 Result Cache */
            qmo::addTupleID2ResultCacheStack( aStatement,
                                              sPartitionRef->table );
        }
    }

    //----------------------------------
    // display 정보 세팅
    //----------------------------------

    IDE_TEST( qmg::setDisplayInfo( aFrom ,
                                   &( sCUNT->tableOwnerName ) ,
                                   &( sCUNT->tableName ) ,
                                   &( sCUNT->aliasName ) )
              != IDE_SUCCESS );

    //----------------------------------
    // Predicate의 처리
    //----------------------------------

    sCUNT->method.subqueryFilter = NULL;
    IDE_TEST( processPredicate( aStatement,
                                aQuerySet,
                                aLeafInfo->predicate,
                                aLeafInfo->constantPredicate,
                                aLeafInfo->ridPredicate,
                                aLeafInfo->index,
                                sCUNT->tupleRowID,
                                &( sCUNT->method.constantFilter ),
                                &( sCUNT->method.filter ),
                                &( sCUNT->method.lobFilter ),
                                NULL,
                                &( sCUNT->method.varKeyRange ),
                                &( sCUNT->method.varKeyFilter ),
                                &( sCUNT->method.varKeyRange4Filter ),
                                &( sCUNT->method.varKeyFilter4Filter ),
                                &( sCUNT->method.fixKeyRange ),
                                &( sCUNT->method.fixKeyFilter ),
                                &( sCUNT->method.fixKeyRange4Print ),
                                &( sCUNT->method.fixKeyFilter4Print ),
                                &( sCUNT->method.ridRange ),
                                & sInSubQueryKeyRange )
              != IDE_SUCCESS );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    sCUNT->sdf = aLeafInfo->sdf;

    if ( aLeafInfo->sdf != NULL )
    {
        IDE_DASSERT( sIndexCnt > 0 );

        // sdf에 baskPlan을 단다.
        aLeafInfo->sdf->basePlan = &sCUNT->plan;

        // sdf에 index 개수만큼 후보를 생성한다.
        // 각각의 후보에 filter/key range/key filter 정보를 생성한다.

        aLeafInfo->sdf->candidateCount = sIndexCnt;

        IDU_FIT_POINT( "qmoOneNonPlan::makeCUNT::alloc::candidate" );
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncScanMethod ) * sIndexCnt,
                                                   (void**)& aLeafInfo->sdf->candidate )
                  != IDE_SUCCESS );

        for ( i = 0;
              i < sIndexCnt;
              i++ )
        {
            // selected index에 대해서는 앞에서 sSCAN에 대해 작업을 했으므로,
            // 다시 작업할 필요가 없다.
            // 대신 그자리엔 full scan에 대해서 작업을 한다.
            if ( &sIndices[i] != aLeafInfo->index )
            {
                aLeafInfo->sdf->candidate[i].index = &sIndices[i];
            }
            else
            {
                aLeafInfo->sdf->candidate[i].index = NULL;
            }

            IDE_TEST( qmoPred::deepCopyPredicate( QC_QMP_MEM( aStatement ),
                                                  aLeafInfo->sdf->predicate,
                                                  & sCopiedPred )
                      != IDE_SUCCESS );

            IDE_TEST( processPredicate( aStatement,
                                        aQuerySet,
                                        sCopiedPred,
                                        NULL, //aLeafInfo->constantPredicate,
                                        NULL, // rid predicate
                                        aLeafInfo->sdf->candidate[i].index,
                                        sCUNT->tupleRowID,
                                        &( aLeafInfo->sdf->candidate[i].constantFilter ),
                                        &( aLeafInfo->sdf->candidate[i].filter ),
                                        &( aLeafInfo->sdf->candidate[i].lobFilter ),
                                        &( aLeafInfo->sdf->candidate[i].subqueryFilter ),
                                        &( aLeafInfo->sdf->candidate[i].varKeyRange ),
                                        &( aLeafInfo->sdf->candidate[i].varKeyFilter ),
                                        &( aLeafInfo->sdf->candidate[i].varKeyRange4Filter ),
                                        &( aLeafInfo->sdf->candidate[i].varKeyFilter4Filter ),
                                        &( aLeafInfo->sdf->candidate[i].fixKeyRange ),
                                        &( aLeafInfo->sdf->candidate[i].fixKeyFilter ),
                                        &( aLeafInfo->sdf->candidate[i].fixKeyRange4Print ),
                                        &( aLeafInfo->sdf->candidate[i].fixKeyFilter4Print ),
                                        &( aLeafInfo->sdf->candidate[i].ridRange ),
                                        & sInSubQueryKeyRange )
                      != IDE_SUCCESS );

            aLeafInfo->sdf->candidate[i].constantFilter =
                sCUNT->method.constantFilter;

            // fix BUG-19074
            IDE_TEST( postProcessCuntMethod( aStatement,
                                             & aLeafInfo->sdf->candidate[i] )
                      != IDE_SUCCESS );

            // fix BUG-19074
            //----------------------------------
            // sdf의 dependency 처리
            //----------------------------------

            sPredicate[0] = aLeafInfo->sdf->candidate[i].fixKeyRange;
            sPredicate[1] = aLeafInfo->sdf->candidate[i].varKeyRange;
            sPredicate[2] = aLeafInfo->sdf->candidate[i].varKeyRange4Filter;
            sPredicate[3] = aLeafInfo->sdf->candidate[i].fixKeyFilter;
            sPredicate[4] = aLeafInfo->sdf->candidate[i].varKeyFilter;
            sPredicate[5] = aLeafInfo->sdf->candidate[i].varKeyFilter4Filter;
            sPredicate[6] = aLeafInfo->sdf->candidate[i].constantFilter;
            sPredicate[7] = aLeafInfo->sdf->candidate[i].filter;

            //----------------------------------
            // PROJ-1473
            // dependency 설정전에 predicate들의 위치정보 변경.
            //----------------------------------
            IDE_TEST( qmoDependency::setDependency( aStatement,
                                                    aQuerySet,
                                                    & sCUNT->plan,
                                                    QMO_CUNT_DEPENDENCY,
                                                    sCUNT->tupleRowID,
                                                    NULL,
                                                    8,
                                                    sPredicate,
                                                    0,
                                                    NULL )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do...
    }

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] = sCUNT->method.fixKeyRange;
    sPredicate[1] = sCUNT->method.varKeyRange;
    sPredicate[2] = sCUNT->method.varKeyRange4Filter;
    sPredicate[3] = sCUNT->method.fixKeyFilter;
    sPredicate[4] = sCUNT->method.varKeyFilter;
    sPredicate[5] = sCUNT->method.varKeyFilter4Filter;
    sPredicate[6] = sCUNT->method.constantFilter;
    sPredicate[7] = sCUNT->method.filter;
    sPredicate[8] = sCUNT->method.ridRange;

    //----------------------------------
    // PROJ-1473
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    for ( i  = 0;
          i <= 8;
          i++ )
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
                                            & sCUNT->plan,
                                            QMO_CUNT_DEPENDENCY,
                                            sCUNT->tupleRowID,
                                            NULL,
                                            9,
                                            sPredicate,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    //----------------------------------
    // Host 변수를 포함한
    // Constant Expression의 최적화
    //----------------------------------

    // fix BUG-19074
    IDE_TEST( postProcessCuntMethod( aStatement,
                                     & sCUNT->method )
              != IDE_SUCCESS );

    // Dependent Tuple Row ID 설정
    sCUNT->depTupleRowID = (UShort)sCUNT->plan.dependency;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMO_EMPTY_DUMP_OBJECT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::postProcessCuntMethod( qcStatement    * aStatement,
                                      qmncScanMethod * aMethod )
{
    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::postProcessCuntMethod::__FT__" );

    IDE_DASSERT( aMethod != NULL );

    if ( aMethod->filter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    aMethod->filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // BUG-17506
    if ( aMethod->varKeyRange4Filter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    aMethod->varKeyRange4Filter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // BUG-17506
    if ( aMethod->varKeyFilter4Filter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    aMethod->varKeyFilter4Filter )
                  != IDE_SUCCESS );
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
qmoOneNonPlan::initVIEW( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : VIEW 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncVIEW의 할당 및 초기화
 *     + 메인 작업
 *         - Store and Search 인경우 Tuple Set의 할당
 *         - VIEW의 컬럼의 구성 (myNode)
 *         - Display를 위한 정보 세팅
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 *
 * TO DO
 *     - myNode는 qtcNode형태 이므로 qtc::makeInternalColumn()으로 할당또는
 *       알아낸 Tuple ID로 호출 한다.
 *
 ***********************************************************************/

    qmncVIEW          * sVIEW;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initVIEW::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aQuerySet  != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncVIEW의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncVIEW ),
                                               (void **)& sVIEW )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sVIEW ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_VIEW ,
                        qmnVIEW ,
                        qmndVIEW,
                        sDataNodeOffset );

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aParent->resultDesc,
                                   & sVIEW->plan.resultDesc )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sVIEW;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeVIEW( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmsFrom      * aFrom ,
                         UInt           aFlag ,
                         qmnPlan      * aChildPlan,
                         qmnPlan      * aPlan )
{
    qmncVIEW          * sVIEW = (qmncVIEW*)aPlan;
    UInt                sDataNodeOffset;

    UShort              sTupleID;
    UShort              sColumnCount;
    UInt                sViewaDependencyFlag    = 0;
    qtcNode           * sPrevNode = NULL;
    qtcNode           * sNewNode;
    qtcNode           * sConvertedNode;

    mtcTemplate       * sMtcTemplate;

    qmcAttrDesc       * sItrAttr;
    qmcAttrDesc       * sChildAttr;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeVIEW::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aQuerySet  != NULL );
    IDE_FT_ASSERT( aChildPlan != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndVIEW));

    sVIEW->plan.left        = aChildPlan;

    //----------------------------------
    // Flag 정보 설정
    //----------------------------------

    sVIEW->flag      = QMN_PLAN_FLAG_CLEAR;
    sVIEW->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sVIEW->plan.flag       |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    // BUG-37507
    // 뷰의 target 의 경우 외부 참조 컬럼이 올수 있다. 따라서 기존과 같이 처리한다.
    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sVIEW->plan.resultDesc,
                                     & aChildPlan->depInfo,
                                     & aChildPlan->depInfo )
              != IDE_SUCCESS );

    //----------------------------------
    // Flag 정보 설정 및 Tuple ID 할당
    //----------------------------------

    switch ( aFlag & QMO_MAKEVIEW_FROM_MASK )
    {
        case QMO_MAKEVIEW_FROM_PROJECTION:
            //----------------------------------
            // Projection으로 부터 생성된 경우
            // - 암시적 VIEW이므로 Tuple을 할당
            //   받는다.
            //----------------------------------

            //dependency 관리에 따른 flag세팅
            sViewaDependencyFlag    = QMO_VIEW_IMPLICIT_DEPENDENCY;

            IDE_TEST( qtc::nextTable( & sTupleID,
                                      aStatement,
                                      NULL,
                                      ID_TRUE,
                                      MTC_COLUMN_NOTNULL_TRUE )
                      != IDE_SUCCESS );

            sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
            sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

            sColumnCount = 0;

            for( sItrAttr = aPlan->resultDesc;
                 sItrAttr != NULL;
                 sItrAttr = sItrAttr->next )
            {
                sColumnCount++;
            }

            //----------------------------------
            // Tuple column의 할당
            //----------------------------------
            IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                                   & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                                   sTupleID ,
                                                   sColumnCount )
                      != IDE_SUCCESS);

            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

            sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
            sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_FALSE;

            //----------------------------------
            // mtcTuple.lflag세팅
            //----------------------------------

            // VIEW는 Intermediate이다.
            sMtcTemplate->rows[sTupleID].lflag    &= ~MTC_TUPLE_TYPE_MASK;
            sMtcTemplate->rows[sTupleID].lflag    |= MTC_TUPLE_TYPE_INTERMEDIATE;

            // To Fix PR-7992
            // Implicite VIEW에 대해서는 Depedendency 관리를 위해
            // VIEW임을 설정하지 않는다.
            //   참조) qtcColumn.cpp의 qtcEstimate() 주석 참조
            // 모든 Column의 depedency 설정 후에 VIEW임을 설정한다.
            sMtcTemplate->rows[sTupleID].lflag    &= ~MTC_TUPLE_VIEW_MASK;
            sMtcTemplate->rows[sTupleID].lflag    |= MTC_TUPLE_VIEW_FALSE;

            //----------------------------------
            // mtcColumn정보의 구축
            // - target정보로 부터 구축한다.
            //----------------------------------

            for( sItrAttr = aPlan->resultDesc, sChildAttr = aChildPlan->resultDesc, sColumnCount = 0;
                 sItrAttr != NULL;
                 sItrAttr = sItrAttr->next, sChildAttr = sChildAttr->next, sColumnCount++ )
            {
                sConvertedNode =
                    (qtcNode*)mtf::convertedNode(
                        (mtcNode*)sItrAttr->expr,
                        &QC_SHARED_TMPLATE(aStatement)->tmplate );

                mtc::copyColumn( &(sMtcTemplate->rows[sTupleID].columns[sColumnCount]),
                                 &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows
                                   [sConvertedNode->node.table].
                                   columns[sConvertedNode->node.column]) );

                // PROJ-2179
                // VIEW의 결과들은 모두 value node로 바꿔준다.
                sItrAttr->expr->node.table      = sTupleID;
                sItrAttr->expr->node.column     = sColumnCount;
                sItrAttr->expr->node.module     = &qtc::valueModule;
                sItrAttr->expr->node.conversion = NULL;
                sItrAttr->expr->node.lflag      = qtc::valueModule.lflag;
                // PROJ-1718 Constant가 아닌 expression이 value module로 변환된 경우
                //           unparsing시 tree를 순회하도록 하도록 orgNode를 설정해준다.
                sItrAttr->expr->node.orgNode    = (mtcNode *)sChildAttr->expr;
            }

            break;

        case QMO_MAKEVIEW_FROM_SELECTION:
            //----------------------------------
            // Selection으로 부터 생성된 경우
            // - Created View또는 Inline View와 같은 명시적 View이다.
            //----------------------------------

            //dependency 관리에 따른 flag세팅
            sViewaDependencyFlag    = QMO_VIEW_EXPLICIT_DEPENDENCY;

            //튜플 알아내기
            sTupleID         = aFrom->tableRef->table;

            break;

        case QMO_MAKEVIEW_FROM_SET:
            //----------------------------------
            // Set으로 부터 생성된 경우
            // - Set을 하나의 개념적 Table로 처리하기 위한 암시적 View임
            // - 그러나 optimizer가 tuple을 할당한녀석이 아니므로 explicit
            //   하게 처리 한다. (이는 PROJ-1358에 의하여 개념이 변경됨)
            //----------------------------------

            // To Fix PR-12791
            // SET 으로부터 생성되는 VIEW 노드는 암시적 View 임.
            //dependency 관리에 따른 flag세팅
            sViewaDependencyFlag    = QMO_VIEW_IMPLICIT_DEPENDENCY;

            //튜플 알아내기
            sTupleID         = aQuerySet->target->targetColumn->node.table;
            break;
        default:
            IDE_FT_ASSERT( 0 );
    }

    //----------------------------------
    // mtcTuple.lflag세팅
    //----------------------------------
    // 저장 매체의 정보 - VIEW는 하나의 memory 공간만 사용
    sMtcTemplate->rows[sTupleID].lflag    &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag    |= MTC_TUPLE_STORAGE_MEMORY;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;

    //----------------------------------
    // myNode의 생성
    //----------------------------------
    sVIEW->myNode = NULL;

    for ( sColumnCount = 0;
          sColumnCount < sMtcTemplate->rows[sTupleID].columnCount;
          sColumnCount++ )
    {
        IDE_TEST( qtc::makeInternalColumn( aStatement,
                                           sTupleID,
                                           sColumnCount,
                                           & sNewNode )
                  != IDE_SUCCESS);

        if ( sVIEW->myNode == NULL )
        {
            sVIEW->myNode  = sNewNode;
            sPrevNode      = sNewNode;
        }
        else
        {
            sPrevNode->node.next = (mtcNode *)sNewNode;
            sPrevNode            = sNewNode;
        }

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sNewNode )
                  != IDE_SUCCESS );
    }

    IDE_FT_ASSERT( sPrevNode != NULL );

    sPrevNode->node.next = NULL;

    // To Fix PR-7992
    // 모든 Column의 올바른 Depedency 설정 후에 VIEW임을 설정함.
    sMtcTemplate->rows[sTupleID].lflag    &= ~MTC_TUPLE_VIEW_MASK;
    sMtcTemplate->rows[sTupleID].lflag    |= MTC_TUPLE_VIEW_TRUE;

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    if ( aFrom != NULL )
    {
        IDE_TEST( qmg::setDisplayInfo( aFrom ,
                                       &( sVIEW->viewOwnerName ) ,
                                       &( sVIEW->viewName ) ,
                                       &( sVIEW->aliasName ) )
                  != IDE_SUCCESS );
    }
    else
    {
        // To Fix PR-7992
        sVIEW->viewName.name = NULL;
        sVIEW->viewName.size = QC_POS_EMPTY_SIZE;

        sVIEW->aliasName.name = NULL;
        sVIEW->aliasName.size = QC_POS_EMPTY_SIZE;
    }

    //-------------------------------------------------------------
    // 마무리 작업
    //-------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sVIEW->plan,
                                            sViewaDependencyFlag,
                                            sTupleID,
                                            NULL,
                                            0,
                                            NULL,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sVIEW->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sVIEW->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sVIEW->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initVSCN( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmsFrom      * aFrom ,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : VSCN 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncVSCN의 할당 및 초기화
 *     + 메인 작업
 *         - MTC_TUPLE_TYPE_TABLE 세팅
 *         - Display를 위한 정보 세팅
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncVSCN          * sVSCN;
    UInt                sDataNodeOffset;
    UShort              sTupleID;
    qcDepInfo           sDepInfo;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initVSCN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncVSCN ),
                                               (void **)& sVSCN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sVSCN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_VSCN ,
                        qmnVSCN ,
                        qmndVSCN,
                        sDataNodeOffset );

    sTupleID = aFrom->tableRef->table;

    // PROJ-2179
    // Join 시 table의 dependency가 포함될 수 있으므로
    // 이 VSCN이 fetch하는 table에 대한 dependency 정보를 별도로 설정한다.

    qtc::dependencySet(sTupleID, &sDepInfo);

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   & sVSCN->plan.resultDesc )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     & sVSCN->plan.resultDesc,
                                     & sDepInfo,
                                     &( aQuerySet->depInfo ) )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sVSCN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::makeVSCN( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                qmsFrom      * aFrom,
                                qmnPlan      * aChildPlan,
                                qmnPlan      * aPlan )
{
    qmncVSCN          * sVSCN = (qmncVSCN *)aPlan;
    UInt                sDataNodeOffset;

    UShort              sChildTupleID;
    UShort              sTupleID;
    mtcTemplate       * sMtcTemplate;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeVSCN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );
    IDE_DASSERT( aFrom      != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndVSCN));

    sVSCN->plan.left        = aChildPlan;

    //----------------------------------
    // 기본 정보 설정
    //----------------------------------

    sVSCN->tupleRowID       = aFrom->tableRef->table;
    sTupleID = sVSCN->tupleRowID;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sVSCN->flag             = QMN_PLAN_FLAG_CLEAR;
    sVSCN->plan.flag        = QMN_PLAN_FLAG_CLEAR;

    // PROJ-2582 recursive with
    if ( ( aFrom->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
         != QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
    {
        IDE_DASSERT( aChildPlan != NULL );
        
        sVSCN->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);
    }
    else
    {
        // nothing to do
    }

    //-------------------------------------------------------------
    // 메인 작업
    //-------------------------------------------------------------

    // Tuple.lflag의 세팅
    sMtcTemplate->rows[sTupleID].lflag    &= ~MTC_TUPLE_TYPE_MASK;
    sMtcTemplate->rows[sTupleID].lflag    |= MTC_TUPLE_TYPE_TABLE;

    // Tuple.lflag의 세팅
    sMtcTemplate->rows[sTupleID].lflag    &= ~MTC_TUPLE_VSCN_MASK;
    sMtcTemplate->rows[sTupleID].lflag    |= MTC_TUPLE_VSCN_TRUE;

    // To Fix PR-8385
    // VSCN이 생성되는 경우에는 in-line view라 하더라도
    // 일반테이블로 처리하여야 한다. 따라서 하위에 view라고 세팅된것을
    // false로 설정한다.
    sMtcTemplate->rows[sTupleID].lflag    &= ~MTC_TUPLE_VIEW_MASK;
    sMtcTemplate->rows[sTupleID].lflag    |=  MTC_TUPLE_VIEW_FALSE;

    // PROJ-2582 recursive with
    if ( ( aFrom->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
         != QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
    {
        // 저장 매체의 정보 - 하위의 VMTR의 정보를 이용
        sChildTupleID = ((qmncVMTR*)aChildPlan)->myNode->dstNode->node.table;

        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |=
            (sMtcTemplate->rows[sChildTupleID].lflag & MTC_TUPLE_STORAGE_MASK);
    }
    else
    {
        // recursive with는 메모리만 지원
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;
    }

    if ( ( aQuerySet->materializeType
           == QMO_MATERIALIZE_TYPE_VALUE )
         &&
         ( ( sMtcTemplate->rows[sTupleID].lflag & MTC_TUPLE_STORAGE_MASK )
           == MTC_TUPLE_STORAGE_DISK ) )
    {
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
    }
    else
    {
        sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
        sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_MATERIALIZE_RID;
    }

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    IDE_TEST( qmg::setDisplayInfo( aFrom ,
                                   &( sVSCN->viewOwnerName ) ,
                                   &( sVSCN->viewName ) ,
                                   &( sVSCN->aliasName ) )
              != IDE_SUCCESS );

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
                                            & sVSCN->plan ,
                                            QMO_VSCN_DEPENDENCY,
                                            sVSCN->tupleRowID ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL ) != IDE_SUCCESS );

    // PROJ-2582 recursive with
    if ( ( aFrom->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
         != QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
    {
        /*
         * PROJ-1071 Parallel Query
         * parallel degree
         */
        sVSCN->plan.mParallelDegree = aChildPlan->mParallelDegree;
        sVSCN->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
        sVSCN->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);
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
qmoOneNonPlan::initCNTR( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qmnPlan       * aParent ,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : CNTR 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncCNTR의 할당 및 초기화
 *     + 메인 작업
 *         - rownum의 설정
 *         - Stopkey Predicate의 처리
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 ***********************************************************************/

    qmncCNTR          * sCNTR;
    UInt                sDataNodeOffset;
    qmcAttrDesc       * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initCNTR::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncCNTR의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncCNTR ),
                                               (void **)& sCNTR )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sCNTR ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_CNTR ,
                        qmnCNTR ,
                        qmndCNTR,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sCNTR;

    /* BUG-39611 support SYS_CONNECT_BY_PATH's expression arguments
     * 상위 Plan 에서 전달 되는 result descript중 CONNECT_BY와 관련된 FUNCTION을
     * 하위로 전달하기 위해 CNTR에도 append한다.
     */
    for ( sItrAttr = aParent->resultDesc;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        if ( ( sItrAttr->expr->node.lflag & MTC_NODE_FUNCTION_CONNECT_BY_MASK )
               == MTC_NODE_FUNCTION_CONNECT_BY_TRUE )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            & sCNTR->plan.resultDesc,
                                            sItrAttr->expr,
                                            QMC_ATTR_SEALED_TRUE,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do*/
        }
    }

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   & sCNTR->plan.resultDesc )
              != IDE_SUCCESS );

    for ( sItrAttr  = sCNTR->plan.resultDesc;
          sItrAttr != NULL;
          sItrAttr  = sItrAttr->next )
    {
        if( ( sItrAttr->expr->lflag & QTC_NODE_ROWNUM_MASK )
                == QTC_NODE_ROWNUM_EXIST )
        {
            sItrAttr->flag &= ~QMC_ATTR_TERMINAL_MASK;
            sItrAttr->flag |= QMC_ATTR_TERMINAL_TRUE;
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
qmoOneNonPlan::makeCNTR( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmoPredicate * aStopkeyPredicate ,
                         qmnPlan      * aChildPlan ,
                         qmnPlan      * aPlan )
{
    qmncCNTR          * sCNTR = (qmncCNTR *)aPlan;
    UInt                sDataNodeOffset;
    qtcNode           * sNode;
    qtcNode           * sPredicate[10];
    qtcNode           * sStopFilter;
    qtcNode           * sOptimizedStopFilter;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeCNTR::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    sCNTR->plan.left        = aChildPlan;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndCNTR));

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sCNTR->flag = QMN_PLAN_FLAG_CLEAR;
    sCNTR->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sCNTR->plan.flag       |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    //----------------------------------
    // rownum의 tuple row ID 처리
    //----------------------------------

    sNode = aQuerySet->SFWGH->rownum;

    // BUG-17949
    // group by rownum인 경우 SFWGH->rownum에 passNode가 달려있다.
    if ( sNode->node.module == & qtc::passModule )
    {
        sNode = (qtcNode*) sNode->node.arguments;
    }
    else
    {
        // Nothing to do.
    }

    sCNTR->rownumRowID = sNode->node.table;

    //----------------------------------
    // stopFilter의 처리
    //----------------------------------

    if ( aStopkeyPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aStopkeyPredicate,
                                          & sStopFilter )
                  != IDE_SUCCESS );

        IDE_TEST( qmoPred::setPriorNodeID( aStatement,
                                           aQuerySet,
                                           sStopFilter )
                  != IDE_SUCCESS );

        IDE_TEST( qmoNormalForm::optimizeForm( sStopFilter,
                                               & sOptimizedStopFilter )
                  != IDE_SUCCESS );

        sCNTR->stopFilter = sOptimizedStopFilter;
    }
    else
    {
        sCNTR->stopFilter = NULL;
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    // data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // Host 변수를 포함한
    // Constant Expression의 최적화
    //----------------------------------

    if ( sCNTR->stopFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement ,
                                                    sCNTR->stopFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] = sCNTR->stopFilter;

    //----------------------------------
    // PROJ-1473
    // dependency 설정전에 predicate들의 위치정보 변경.
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[0],
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sCNTR->plan,
                                            QMO_CNTR_DEPENDENCY,
                                            0,
                                            NULL,
                                            1,
                                            sPredicate,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sCNTR->plan.mParallelDegree = aChildPlan->mParallelDegree;
    sCNTR->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sCNTR->plan.flag |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initINST( qcStatement   * aStatement ,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : INST 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncINST의 할당 및 초기화
 *
 ***********************************************************************/

    qmncINST          * sINST;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initINST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncINST ),
                                               (void **)& sINST )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sINST ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_INST ,
                        qmnINST ,
                        qmndINST,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sINST;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeINST( qcStatement   * aStatement ,
                         qmoINSTInfo   * aINSTInfo ,
                         qmnPlan       * aChildPlan ,
                         qmnPlan       * aPlan )
{
/***********************************************************************
 *
 * Description : INST 노드를 생성한다.
 *
 * Implementation :
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *
 ***********************************************************************/

    qmncINST          * sINST = (qmncINST *)aPlan;
    UInt                sDataNodeOffset;
    qmsFrom             sFrom;
    qcmColumn         * sColumn;
    qmmValueNode      * sValue;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeINST::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndINST) );

    sINST->plan.left = aChildPlan;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sINST->flag = QMN_PLAN_FLAG_CLEAR;
    sINST->plan.flag = QMN_PLAN_FLAG_CLEAR;

    if ( aChildPlan != NULL )
    {
        sINST->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);
    }
    else
    {
        // Nothing to do.
    }

    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement,
                                     aINSTInfo->tableRef->table,
                                     &( sINST->plan.flag ) )
              != IDE_SUCCESS );

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    // tableRef만 있으면 됨
    sFrom.tableRef = aINSTInfo->tableRef;

    IDE_TEST( qmg::setDisplayInfo( & sFrom,
                                   &( sINST->tableOwnerName ),
                                   &( sINST->tableName ),
                                   &( sINST->aliasName ) )
                 != IDE_SUCCESS );

    /* BUG-47625 Insert execution 성능향상 */
    for ( sColumn = aINSTInfo->tableRef->tableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        if ( ( sColumn->basicInfo->flag & MTC_COLUMN_NOTNULL_MASK )
             == MTC_COLUMN_NOTNULL_TRUE )
        {
            sINST->flag &= ~QMNC_INST_EXIST_NOTNULL_COLUMN_MASK;
            sINST->flag |= QMNC_INST_EXIST_NOTNULL_COLUMN_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK)
               == MTD_ENCRYPT_TYPE_TRUE )
        {
            sINST->flag &= ~QMNC_INST_EXIST_ENCRYPT_COLUMN_MASK;
            sINST->flag |= QMNC_INST_EXIST_ENCRYPT_COLUMN_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* BUG-47625 Insert execution 성능향상 */
    if ( aINSTInfo->rows->next == NULL )
    {
        for ( sValue = aINSTInfo->rows->values;
              sValue != NULL;
              sValue = sValue->next )
        {
            if ( sValue->timestamp == ID_TRUE )
            {
                sINST->flag &= ~QMNC_INST_EXIST_TIMESTAMP_MASK;
                sINST->flag |= QMNC_INST_EXIST_TIMESTAMP_TRUE;
            }
            else
            {
                /* Nothing to do */
            }

            if ( sValue->msgID == ID_TRUE )
            {
                sINST->flag &= ~QMNC_INST_EXIST_QUEUE_MASK;
                sINST->flag |= QMNC_INST_EXIST_QUEUE_TRUE;
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

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    //----------------------------------
    // insert 관련 정보
    //----------------------------------

    // insert target 설정
    sINST->tableRef = aINSTInfo->tableRef;

    // insert select 설정
    if ( aChildPlan != NULL )
    {
        sINST->isInsertSelect = ID_TRUE;
    }
    else
    {
        sINST->isInsertSelect = ID_FALSE;
    }

    // multi-table insert 설정
    sINST->isMultiInsertSelect = aINSTInfo->multiInsertSelect;

    // insert columns 설정
    sINST->columns          = aINSTInfo->columns;
    sINST->columnsForValues = aINSTInfo->columnsForValues;

    // insert values 설정
    sINST->rows           = aINSTInfo->rows;
    sINST->valueIdx       = aINSTInfo->valueIdx;
    sINST->canonizedTuple = aINSTInfo->canonizedTuple;
    sINST->compressedTuple= aINSTInfo->compressedTuple;
    sINST->queueMsgIDSeq  = aINSTInfo->queueMsgIDSeq;

    // insert hint 설정
    sINST->hints = aINSTInfo->hints;

    /* PROJ-2464 hybrid partitioned table 지원
     *  - Simple Query를 지원하기 위해서, Partition을 고려하지 않고 우선 설정한다.
     */
    if ( sINST->hints != NULL )
    {
        if ( ( ( sINST->hints->directPathInsHintFlag & SMI_INSERT_METHOD_MASK )
               == SMI_INSERT_METHOD_APPEND )
             &&
             ( ( sINST->plan.flag & QMN_PLAN_STORAGE_MASK )
               == QMN_PLAN_STORAGE_DISK ) )
        {
            //---------------------------------------------------
            // PROJ-1566
            // INSERT가 APPEND 방식으로 수행되어야 하는 경우,
            // SIX Lock을 획득해야 함
            //---------------------------------------------------

            sINST->isAppend = ID_TRUE;
        }
        else
        {
            sINST->isAppend = ID_FALSE;
        }

        // PROJ-2673
        if ( aINSTInfo->insteadOfTrigger == ID_FALSE )
        {
            sINST->disableTrigger = sINST->hints->disableInsTrigger;
        }
        else
        {
            sINST->disableTrigger = ID_FALSE;
        }
    }
    else
    {
        sINST->isAppend = ID_FALSE;
        sINST->disableTrigger = ID_FALSE;
    }

    // sequence 정보
    sINST->nextValSeqs = aINSTInfo->nextValSeqs;

    // instead of trigger 설정
    sINST->insteadOfTrigger = aINSTInfo->insteadOfTrigger;

    // BUG-43063 insert nowait
    sINST->lockWaitMicroSec = aINSTInfo->lockWaitMicroSec;

    //----------------------------------
    // parent constraint 설정
    //----------------------------------

    sINST->parentConstraints = aINSTInfo->parentConstraints;
    sINST->checkConstrList   = aINSTInfo->checkConstrList;

    //----------------------------------
    // return into 설정
    //----------------------------------

    sINST->returnInto = aINSTInfo->returnInto;

    //----------------------------------
    // Default Expr 설정
    //----------------------------------

    sINST->defaultExprTableRef = aINSTInfo->defaultExprTableRef;
    sINST->defaultExprColumns  = aINSTInfo->defaultExprColumns;

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    qtc::dependencyClear( & sINST->plan.depInfo );

    // PROJ-2551 simple query 최적화
    // simple insert인 경우 fast execute를 수행한다.
    IDE_TEST( checkSimpleINST( aStatement, sINST )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initUPTE( qcStatement   * aStatement ,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : UPTE 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncUPTE의 할당 및 초기화
 *
 ***********************************************************************/

    qmncUPTE          * sUPTE;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initUPTE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncUPTE ),
                                               (void **)& sUPTE )
        != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sUPTE ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_UPTE ,
                        qmnUPTE ,
                        qmndUPTE,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sUPTE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeUPTE( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qmoUPTEInfo   * aUPTEInfo ,
                         qmnPlan       * aChildPlan ,
                         qmnPlan       * aPlan )
{
/***********************************************************************
 *
 * Description : UPTE 노드를 생성한다.
 *
 * Implementation :
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *
 ***********************************************************************/

    qmncUPTE          * sUPTE = (qmncUPTE *)aPlan;
    UInt                sDataNodeOffset;
    qcmTableInfo      * sTableInfo;
    qmsPartitionRef   * sPartitionRef;
    UInt                sPartitionCount = 0;
    qcmColumn         * sColumn;
    qmsFrom             sFrom;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeUPTE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    sTableInfo = aUPTEInfo->updateTableRef->tableInfo;
    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndUPTE) );

    sUPTE->plan.left = aChildPlan;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sUPTE->flag = QMN_PLAN_FLAG_CLEAR;
    sUPTE->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sUPTE->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement,
                                     aUPTEInfo->updateTableRef->table,
                                     &( sUPTE->plan.flag ) )
              != IDE_SUCCESS );

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    QCP_SET_INIT_QMS_FROM( (&sFrom) );
    sFrom.tableRef = aUPTEInfo->updateTableRef;

    IDE_TEST( qmg::setDisplayInfo( & sFrom,
                                   &( sUPTE->tableOwnerName ),
                                   &( sUPTE->tableName ),
                                   &( sUPTE->aliasName ) )
              != IDE_SUCCESS );

    /* PROJ-2204 Join Update, Delete
     * join update임을 표시한다. */
    if ( aQuerySet->SFWGH->from->tableRef->view != NULL )
    {
        sUPTE->flag &= ~QMNC_UPTE_VIEW_MASK;
        sUPTE->flag |= QMNC_UPTE_VIEW_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // key preserved property
    if ( aQuerySet->SFWGH->preservedInfo != NULL )
    {
        if ( aQuerySet->SFWGH->preservedInfo->useKeyPreservedTable == ID_TRUE )
        {
            sUPTE->flag &= ~QMNC_UPTE_VIEW_KEY_PRESERVED_MASK;
            sUPTE->flag |= QMNC_UPTE_VIEW_KEY_PRESERVED_TRUE;
        }
    }
    
    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    //----------------------------------
    // update 관련 정보
    //----------------------------------

    // update target table 설정
    sUPTE->tableRef = aUPTEInfo->updateTableRef;

    // update column 관련 정보
    sUPTE->columns           = aUPTEInfo->columns;
    sUPTE->updateColumnList  = aUPTEInfo->updateColumnList;
    sUPTE->updateColumnCount = aUPTEInfo->updateColumnCount;
    sUPTE->mTableList        = NULL;
    sUPTE->mMultiTableCount  = 0;

    if ( aUPTEInfo->updateColumnCount > 0 )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( aUPTEInfo->updateColumnCount * ID_SIZEOF(UInt),
                                                   (void**) & sUPTE->updateColumnIDs )
                  != IDE_SUCCESS );

        // To Fix PR-10592
        // Update Column이 여러개인 경우 정확히 ID를 설정해야 함.
        for ( i = 0, sColumn = aUPTEInfo->columns;
              i < aUPTEInfo->updateColumnCount;
              i++, sColumn = sColumn->next )
        {
            sUPTE->updateColumnIDs[i] = sColumn->basicInfo->column.id;
        }
    }
    else
    {
        sUPTE->updateColumnIDs = NULL;
    }

    // update value 관련 정보
    sUPTE->values         = aUPTEInfo->values;
    sUPTE->subqueries     = aUPTEInfo->subqueries;
    sUPTE->valueIdx       = aUPTEInfo->valueIdx;
    sUPTE->canonizedTuple = aUPTEInfo->canonizedTuple;
    sUPTE->compressedTuple= aUPTEInfo->compressedTuple;
    sUPTE->isNull         = aUPTEInfo->isNull;

    // sequence 정보
    sUPTE->nextValSeqs    = aUPTEInfo->nextValSeqs;

    // instead of trigger 설정
    sUPTE->insteadOfTrigger = aUPTEInfo->insteadOfTrigger;

    //----------------------------------
    // partition 관련 정보
    //----------------------------------

    sUPTE->insertTableRef      = aUPTEInfo->insertTableRef;
    sUPTE->isRowMovementUpdate = aUPTEInfo->isRowMovementUpdate;

    // partition별 update column list 생성
    if ( sUPTE->tableRef->partitionRef != NULL )
    {
        sPartitionCount = 0;
        for ( sPartitionRef  = sUPTE->tableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next )
        {
            sPartitionCount++;
        }

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                   (void **) &( sUPTE->updatePartColumnList ) )
                  != IDE_SUCCESS );

        for ( sPartitionRef  = sUPTE->tableRef->partitionRef, i = 0;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next, i++ )
        {
            IDE_TEST( qmoPartition::makePartUpdateColumnList( aStatement,
                                                              sPartitionRef,
                                                              sUPTE->updateColumnList,
                                                              &( sUPTE->updatePartColumnList[i] ) )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sUPTE->updatePartColumnList = NULL;
    }

    // update type
    sUPTE->updateType = aUPTEInfo->updateType;

    //----------------------------------
    // cursor 관련 정보
    //----------------------------------

    sUPTE->cursorType    = aUPTEInfo->cursorType;
    sUPTE->inplaceUpdate = aUPTEInfo->inplaceUpdate;

    //----------------------------------
    // limit 정보의 세팅
    //----------------------------------

    sUPTE->limit = aUPTEInfo->limit;
    if ( sUPTE->limit != NULL )
    {
        sUPTE->flag &= ~QMNC_UPTE_LIMIT_MASK;
        sUPTE->flag |= QMNC_UPTE_LIMIT_TRUE;
    }
    else
    {
        sUPTE->flag &= ~QMNC_UPTE_LIMIT_MASK;
        sUPTE->flag |= QMNC_UPTE_LIMIT_FALSE;
    }

    //----------------------------------
    // constraint 설정
    //----------------------------------

    // Foreign Key Referencing을 위한 Master Table이 존재하는 지 검사
    if ( qdnForeignKey::haveToCheckParent( sTableInfo,
                                           sUPTE->updateColumnIDs,
                                           aUPTEInfo->updateColumnCount )
         == ID_TRUE)
    {
        sUPTE->parentConstraints = aUPTEInfo->parentConstraints;
    }
    else
    {
        sUPTE->parentConstraints = NULL;
    }

    // Child Table의 Referencing 조건 검사
    if ( qdnForeignKey::haveToOpenBeforeCursor( aUPTEInfo->childConstraints,
                                                sUPTE->updateColumnIDs,
                                                aUPTEInfo->updateColumnCount )
         == ID_TRUE )
    {
        sUPTE->childConstraints = aUPTEInfo->childConstraints;
    }
    else
    {
        sUPTE->childConstraints = NULL;
    }

    sUPTE->checkConstrList = aUPTEInfo->checkConstrList;

    //----------------------------------
    // return into 설정
    //----------------------------------

    sUPTE->returnInto = aUPTEInfo->returnInto;

    //----------------------------------
    // Default Expr 설정
    //----------------------------------

    sUPTE->defaultExprTableRef    = aUPTEInfo->defaultExprTableRef;
    sUPTE->defaultExprColumns     = aUPTEInfo->defaultExprColumns;
    sUPTE->defaultExprBaseColumns = aUPTEInfo->defaultExprBaseColumns;

    //----------------------------------
    // PROJ-1784 DML Without Retry
    //----------------------------------

    IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                    sUPTE->tableRef->table,
                                                    & sUPTE->whereColumnList )
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::makeSetClauseColumnList( aStatement,
                                                  sUPTE->tableRef->table,
                                                  & sUPTE->setColumnList )
              != IDE_SUCCESS );

    // partition별 where column list 생성
    if ( sUPTE->tableRef->partitionRef != NULL )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                   (void **) &( sUPTE->wherePartColumnList ) )
                  != IDE_SUCCESS );

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                   (void **) &( sUPTE->setPartColumnList ) )
                  != IDE_SUCCESS );

        for ( sPartitionRef  = sUPTE->tableRef->partitionRef, i = 0;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next, i++ )
        {
            /* PROJ-2464 hybrid partitioned table 지원 */
            IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                            sPartitionRef->table,
                                                            &( sUPTE->wherePartColumnList[i] ) )
                      != IDE_SUCCESS );

            IDE_TEST( qdbCommon::makeSetClauseColumnList( aStatement,
                                                          sPartitionRef->table,
                                                          &( sUPTE->setPartColumnList[i] ) )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sUPTE->wherePartColumnList = NULL;
        sUPTE->setPartColumnList   = NULL;
    }

    if ( aQuerySet->SFWGH->hints != NULL )
    {
        if( aQuerySet->SFWGH->hints->withoutRetry == ID_TRUE )
        {
            sUPTE->withoutRetry = ID_TRUE ;
        }
        else
        {
            sUPTE->withoutRetry = ( QCU_DML_WITHOUT_RETRY_ENABLE == 1 ) ? ID_TRUE : ID_FALSE ;
        }
    }
    else
    {
        sUPTE->withoutRetry = ID_FALSE;
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sUPTE->plan ,
                                            QMO_UPTE_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    // PROJ-2551 simple query 최적화
    // simple insert인 경우 fast execute를 수행한다.
    IDE_TEST( checkSimpleUPTE( aStatement, sUPTE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initDETE( qcStatement   * aStatement ,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : DETE 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncDETE의 할당 및 초기화
 *
 ***********************************************************************/

    qmncDETE          * sDETE;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initDETE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncDETE ),
                                               (void **)& sDETE )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sDETE ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_DETE ,
                        qmnDETE ,
                        qmndDETE,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sDETE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeDETE( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qmoDETEInfo   * aDETEInfo ,
                         qmnPlan       * aChildPlan ,
                         qmnPlan       * aPlan )
{
/***********************************************************************
 *
 * Description : DETE 노드를 생성한다.
 *
 * Implementation :
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *
 ***********************************************************************/

    qmncDETE          * sDETE = (qmncDETE *)aPlan;
    UInt                sDataNodeOffset;
    qmsFrom             sFrom;
    qmsPartitionRef   * sPartitionRef;
    UInt                sPartitionCount;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeDETE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------
    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndDETE) );

    sDETE->plan.left = aChildPlan;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sDETE->flag = QMN_PLAN_FLAG_CLEAR;
    sDETE->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sDETE->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);
    sDETE->mTableList = NULL;
    sDETE->mMultiTableCount  = 0;
    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement,
                                     aDETEInfo->deleteTableRef->table,
                                     &( sDETE->plan.flag ) )
              != IDE_SUCCESS );

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    QCP_SET_INIT_QMS_FROM( (&sFrom) );
    sFrom.tableRef = aDETEInfo->deleteTableRef;

    IDE_TEST( qmg::setDisplayInfo( & sFrom,
                                   &( sDETE->tableOwnerName ),
                                   &( sDETE->tableName ),
                                   &( sDETE->aliasName ) )
                 != IDE_SUCCESS );

    /* PROJ-2204 Join Update, Delete
     * join delete임을 표시한다. */
    if ( aQuerySet->SFWGH->from->tableRef->view != NULL )
    {
        sDETE->flag &= ~QMNC_DETE_VIEW_MASK;
        sDETE->flag |= QMNC_DETE_VIEW_TRUE;
    }
    else
    {
        // Nothing to do.
    }
    
    // key preserved property
    if ( aQuerySet->SFWGH->preservedInfo != NULL )
    {
        if ( aQuerySet->SFWGH->preservedInfo->useKeyPreservedTable == ID_TRUE )
        {
            sDETE->flag &= ~QMNC_DETE_VIEW_KEY_PRESERVED_MASK;
            sDETE->flag |= QMNC_DETE_VIEW_KEY_PRESERVED_TRUE;
        }
    }
        
    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    //----------------------------------
    // delete 관련 정보
    //----------------------------------

    // delete target table 설정
    sDETE->tableRef = aDETEInfo->deleteTableRef;

    // instead of trigger 설정
    sDETE->insteadOfTrigger = aDETEInfo->insteadOfTrigger;

    //----------------------------------
    // limit 정보의 세팅
    //----------------------------------

    sDETE->limit = aDETEInfo->limit;
    if ( sDETE->limit != NULL )
    {
        sDETE->flag &= ~QMNC_DETE_LIMIT_MASK;
        sDETE->flag |= QMNC_DETE_LIMIT_TRUE;
    }
    else
    {
        sDETE->flag &= ~QMNC_DETE_LIMIT_MASK;
        sDETE->flag |= QMNC_DETE_LIMIT_FALSE;
    }

    //----------------------------------
    // check constraint 설정
    //----------------------------------

    sDETE->childConstraints = aDETEInfo->childConstraints;

    //----------------------------------
    // return into 설정
    //----------------------------------

    sDETE->returnInto = aDETEInfo->returnInto;

    //----------------------------------
    // PROJ-1784 DML Without Retry
    //----------------------------------

    IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                    sDETE->tableRef->table,
                                                    & sDETE->whereColumnList )
              != IDE_SUCCESS );

    // partition별 where column list 생성
    if ( sDETE->tableRef->partitionRef != NULL )
    {
        sPartitionCount = 0;
        for ( sPartitionRef  = sDETE->tableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next )
        {
            sPartitionCount++;
        }

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                   (void **) &( sDETE->wherePartColumnList ) )
                  != IDE_SUCCESS );

        for ( sPartitionRef  = sDETE->tableRef->partitionRef, i = 0;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next, i++ )
        {
            /* PROJ-2464 hybrid partitioned table 지원 */
            IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                            sPartitionRef->table,
                                                            &( sDETE->wherePartColumnList[i] ) )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sDETE->wherePartColumnList = NULL;
    }

    if ( aQuerySet->SFWGH->hints != NULL )
    {
        if( aQuerySet->SFWGH->hints->withoutRetry == ID_TRUE )
        {
            sDETE->withoutRetry = ID_TRUE ;
        }
        else
        {
            sDETE->withoutRetry = ( QCU_DML_WITHOUT_RETRY_ENABLE == 1 ) ? ID_TRUE : ID_FALSE ;
        }
    }
    else
    {
        sDETE->withoutRetry = ID_FALSE;
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sDETE->plan ,
                                            QMO_DETE_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    // PROJ-2551 simple query 최적화
    // simple insert인 경우 fast execute를 수행한다.
    IDE_TEST( checkSimpleDETE( aStatement, sDETE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::initMOVE( qcStatement   * aStatement ,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : MOVE 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncMOVE의 할당 및 초기화
 *
 ***********************************************************************/

    qmncMOVE          * sMOVE;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initMOVE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncMOVE ),
                                               (void **)& sMOVE )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sMOVE ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_MOVE ,
                        qmnMOVE ,
                        qmndMOVE,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sMOVE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeMOVE( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qmoMOVEInfo   * aMOVEInfo ,
                         qmnPlan       * aChildPlan ,
                         qmnPlan       * aPlan )
{
/***********************************************************************
 *
 * Description : MOVE 노드를 생성한다.
 *
 * Implementation :
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *
 ***********************************************************************/

    qmncMOVE          * sMOVE = (qmncMOVE *)aPlan;
    UInt                sDataNodeOffset;

    qmsFrom             sFrom;
    qmsPartitionRef   * sPartitionRef;
    UInt                sPartitionCount;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeMOVE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------
    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndMOVE) );

    sMOVE->plan.left = aChildPlan;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sMOVE->flag = QMN_PLAN_FLAG_CLEAR;
    sMOVE->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sMOVE->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement,
                                     aQuerySet->SFWGH->from->tableRef->table,
                                     &( sMOVE->plan.flag ) )
              != IDE_SUCCESS );

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    // tableRef만 있으면 됨
    sFrom.tableRef = aMOVEInfo->targetTableRef;

    IDE_TEST( qmg::setDisplayInfo( & sFrom,
                                   &( sMOVE->tableOwnerName),
                                   &( sMOVE->tableName),
                                   &( sMOVE->aliasName) )
              != IDE_SUCCESS );

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    //----------------------------------
    // move 관련 정보
    //----------------------------------

    // source table 설정
    sMOVE->tableRef = aQuerySet->SFWGH->from->tableRef;

    // target table 설정
    sMOVE->targetTableRef = aMOVEInfo->targetTableRef;

    // insert 관련 설정
    sMOVE->columns        = aMOVEInfo->columns;
    sMOVE->values         = aMOVEInfo->values;
    sMOVE->valueIdx       = aMOVEInfo->valueIdx;
    sMOVE->canonizedTuple = aMOVEInfo->canonizedTuple;
    sMOVE->compressedTuple= aMOVEInfo->compressedTuple;

    // sequence 설정
    sMOVE->nextValSeqs    = aMOVEInfo->nextValSeqs;

    //----------------------------------
    // limit 정보의 세팅
    //----------------------------------

    sMOVE->limit = aMOVEInfo->limit;
    if ( sMOVE->limit != NULL )
    {
        sMOVE->flag &= ~QMNC_MOVE_LIMIT_MASK;
        sMOVE->flag |= QMNC_MOVE_LIMIT_TRUE;
    }
    else
    {
        sMOVE->flag &= ~QMNC_MOVE_LIMIT_MASK;
        sMOVE->flag |= QMNC_MOVE_LIMIT_FALSE;
    }

    //----------------------------------
    // constraint 설정
    //----------------------------------

    sMOVE->parentConstraints = aMOVEInfo->parentConstraints;
    sMOVE->childConstraints  = aMOVEInfo->childConstraints;
    sMOVE->checkConstrList   = aMOVEInfo->checkConstrList;

    //----------------------------------
    // Default Expr 설정
    //----------------------------------

    sMOVE->defaultExprTableRef = aMOVEInfo->defaultExprTableRef;
    sMOVE->defaultExprColumns  = aMOVEInfo->defaultExprColumns;

    //----------------------------------
    // PROJ-1784 DML Without Retry
    //----------------------------------

    IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                    sMOVE->tableRef->table,
                                                    & sMOVE->whereColumnList )
              != IDE_SUCCESS );

    // partition별 where column list 생성
    if ( sMOVE->tableRef->partitionRef != NULL )
    {
        sPartitionCount = 0;
        for ( sPartitionRef  = sMOVE->tableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next )
        {
            sPartitionCount++;
        }

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                   (void **) &( sMOVE->wherePartColumnList ) )
                  != IDE_SUCCESS );

        for ( sPartitionRef  = sMOVE->tableRef->partitionRef, i = 0;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next, i++ )
        {
            /* PROJ-2464 hybrid partitioned table 지원 */
            IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                            sPartitionRef->table,
                                                            &( sMOVE->wherePartColumnList[i] ) )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sMOVE->wherePartColumnList = NULL;
    }

    if ( aQuerySet->SFWGH->hints != NULL )
    {
        if( aQuerySet->SFWGH->hints->withoutRetry == ID_TRUE )
        {
            sMOVE->withoutRetry = ID_TRUE ;
        }
        else
        {
            sMOVE->withoutRetry = ( QCU_DML_WITHOUT_RETRY_ENABLE == 1 ) ? ID_TRUE : ID_FALSE ;
        }
    }
    else
    {
        sMOVE->withoutRetry = ID_FALSE;
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sMOVE->plan ,
                                            QMO_MOVE_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::setDirectionInfo( UInt               * aFlag ,
                                 qcmIndex           * aIndex,
                                 qmgPreservedOrder  * aPreservedOrder )
{
/***********************************************************************
 *
 * Description : index와 preserved order에 따른 traverse direction을
 *               설정한다.
 *               preserved order가 not defined인 경우 full scan을
 *               허용하는 flag를 세팅한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::setDirectionInfo::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------
    IDE_DASSERT( aFlag            != NULL );
    IDE_DASSERT( aIndex           != NULL );

    // Hierarchy의 경우처럼 preserved order가 존재하지 않을 경우
    if ( aPreservedOrder == NULL )
    {
        *aFlag &= ~QMNC_SCAN_TRAVERSE_MASK;
        *aFlag |= QMNC_SCAN_TRAVERSE_FORWARD;
    }
    else
    {
        IDE_DASSERT( aPreservedOrder->column ==
                     (UShort) (aIndex->keyColumns[0].column.id % SMI_COLUMN_ID_MAXIMUM) );

        if ( (aIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
             == SMI_COLUMN_ORDER_ASCENDING )
        {
            // index의 order가 ascending일때
            switch ( aPreservedOrder->direction )
            {
                case QMG_DIRECTION_NOT_DEFINED:
                    *aFlag &= ~QMNC_SCAN_FORCE_INDEX_SCAN_MASK;
                    *aFlag |= QMNC_SCAN_FORCE_INDEX_SCAN_FALSE;
                    /* fall through */
                case QMG_DIRECTION_ASC:
                case QMG_DIRECTION_ASC_NULLS_LAST:
                    *aFlag &= ~QMNC_SCAN_TRAVERSE_MASK;
                    *aFlag |= QMNC_SCAN_TRAVERSE_FORWARD;
                    break;
                case QMG_DIRECTION_DESC:
                case QMG_DIRECTION_DESC_NULLS_FIRST:
                    *aFlag &= ~QMNC_SCAN_TRAVERSE_MASK;
                    *aFlag |= QMNC_SCAN_TRAVERSE_BACKWARD;
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            // index의 order가 descending일때
            switch ( aPreservedOrder->direction )
            {
                case QMG_DIRECTION_NOT_DEFINED:
                    *aFlag &= ~QMNC_SCAN_FORCE_INDEX_SCAN_MASK;
                    *aFlag |= QMNC_SCAN_FORCE_INDEX_SCAN_FALSE;
                    /* fall through */
                case QMG_DIRECTION_DESC:
                case QMG_DIRECTION_DESC_NULLS_LAST:
                    *aFlag &= ~QMNC_SCAN_TRAVERSE_MASK;
                    *aFlag |= QMNC_SCAN_TRAVERSE_FORWARD;
                    break;
                case QMG_DIRECTION_ASC:
                case QMG_DIRECTION_ASC_NULLS_FIRST:
                    *aFlag &= ~QMNC_SCAN_TRAVERSE_MASK;
                    *aFlag |= QMNC_SCAN_TRAVERSE_BACKWARD;
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoOneNonPlan::setTableTypeFromTuple( qcStatement   * aStatement ,
                                      UInt            aTupleID ,
                                      UInt          * aFlag )
{
/***********************************************************************
 *
 * Description : 해당 Tuple로 부터 Storage 속성을 찾아 세팅한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::setTableTypeFromTuple::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aFlag      != NULL );


    if ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTupleID].lflag
           & MTC_TUPLE_STORAGE_MASK )
         == MTC_TUPLE_STORAGE_MEMORY )
    {
        *aFlag &= ~QMN_PLAN_STORAGE_MASK;
        *aFlag |= QMN_PLAN_STORAGE_MEMORY;
    }
    else
    {
        *aFlag &= ~QMN_PLAN_STORAGE_MASK;
        *aFlag |= QMN_PLAN_STORAGE_DISK;
    }

    return IDE_SUCCESS;
}

idBool
qmoOneNonPlan::isMemoryTableFromTuple( qcStatement   * aStatement ,
                                       UShort          aTupleID )
{
/***********************************************************************
 *
 * Description : 해당 Tuple로 부터 Storage가 메모리 테이블인지 찾는다
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    if( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTupleID].lflag
          & MTC_TUPLE_STORAGE_MASK ) ==
        MTC_TUPLE_STORAGE_MEMORY )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

IDE_RC
qmoOneNonPlan::classifyFixedNVariable( qcStatement    * aStatement ,
                                       qmsQuerySet    * aQuerySet ,
                                       qmoPredicate  ** aKeyPred ,
                                       qtcNode       ** aFixKey ,
                                       qtcNode       ** aFixKey4Print ,
                                       qtcNode       ** aVarKey ,
                                       qtcNode       ** aVarKey4Filter ,
                                       qmoPredicate  ** aFilter )
{
/***********************************************************************
 *
 * Description : FixKey(range ,filter) 와 VarKey(range ,filter)를 구분한다
 *
 *
 * Implementation :
 *    - fixed , variable의 구분
 *        - 이는 qmoPredicate의 flag로 구분을 한다.
 *    - qtcNode로의 변환
 *        - qmoPredicate -> qtcNode로의 변환
 *        - DNF 형태의 노드 변환
 *
 *    - fixed 인경우
 *        - 변화된 DNF 노드 세팅
 *        - display를 위한 fixXXX4Print에 CNF형태의 qtcNode 세팅
 *    - variable 인경우
 *        - 변화된 DNF 노드 세팅
 *        - filter로 사용될 경우를 위한 CNF형태의 qtcNode 세팅
 *
 *    PROJ-1436 SQL Plan Cache
 *    proj-1436이전에는 fixed key에 대하여 prepare 시점에서 미리
 *    smiRange를 생성하였으나 이는 plan cache에 의해 생성된
 *    smiRange를 공유하게되어 variable column에 대하여 smiValue.value를
 *    참조하므로 자칫 동시성 문제가 발생할 수 있다. 그러므로
 *    fixed key도 variable key와 마찬가지로 execute 시점에서
 *    smiRange를 생성하여 오염의 가능성이 없도록 한다.
 *
 ***********************************************************************/

    qmoPredicate      * sLastFilter = NULL;
    qtcNode           * sKey        = NULL;
    qtcNode           * sDNFKey     = NULL;
    UInt                sEstDNFCnt;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::classifyFixedNVariable::__FT__" );

    IDE_DASSERT( *aKeyPred != NULL );

    // qtcNode로의 변환
    IDE_TEST( qmoPred::linkPredicate( aStatement ,
                                      * aKeyPred ,
                                      & sKey )
              != IDE_SUCCESS );
    IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                       aQuerySet ,
                                       sKey )
              != IDE_SUCCESS );

    if ( sKey != NULL )
    {
        // To Fix PR-9481
        // CNF로 구성된 Key Range Predicate을 DNF normalize할 경우
        // 엄청나게 많은 Node로 변환될 수 있다.
        // 이를 검사하여 지나치게 많은 변화가 필요한 경우에는
        // Default Key Range만을 생성하게 한다.
        IDE_TEST( qmoNormalForm::estimateDNF( sKey, & sEstDNFCnt )
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
                                                   & sDNFKey )
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
        // To Fix PR-9481
        // Default Key Range를 생성하고 Key Range Predicate은
        // Filter가 되어야 한다.

        if ( *aFilter == NULL )
        {
            *aFilter = *aKeyPred;
        }
        else
        {
            for ( sLastFilter        = *aFilter;
                  sLastFilter->next != NULL;
                  sLastFilter        = sLastFilter->next ) ;

            sLastFilter->next = *aKeyPred;
        }

        // fix BUG-10671
        // keyRange를 생성할 수 없는 경우,
        // keyRange의 predicate들을 filter연결리스트로 옮기고
        // keyRange는 NULL로 설정해야 함.
        *aKeyPred = NULL;

        *aFixKey = NULL;
        *aVarKey = NULL;

        *aFixKey4Print = NULL;
        *aVarKey4Filter = NULL;
    }
    else
    {
        // Fixed인지 , variable인지 flag를 보고 판단한다.
        // 이는 Predicate 관리자로 부터 모두 처리되어 첫 qmoPredicate.flag에
        // 취합되어 세팅된다.
        if ( ( (*aKeyPred)->flag & QMO_PRED_VALUE_MASK ) == QMO_PRED_FIXED  )
        {
            // Fixed Key Range/Filter인 경우

            *aFixKey = sDNFKey;
            *aVarKey = NULL;

            // Fixed Key 출력을 위한 노드정보를 저장한다.
            *aFixKey4Print = sKey;
            *aVarKey4Filter = NULL;
        }
        else
        {
            // Variable Key Range/Filter인 경우

            *aFixKey = NULL;
            *aVarKey = sDNFKey;

            *aFixKey4Print = NULL;
            // Range구성을 실패할 경우 사용할 Filter정보를 저장한다.
            *aVarKey4Filter = sKey;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::processPredicate( qcStatement     * aStatement,
                                        qmsQuerySet     * aQuerySet,
                                        qmoPredicate    * aPredicate,
                                        qmoPredicate    * aConstantPredicate,
                                        qmoPredicate    * aRidPredicate,
                                        qcmIndex        * aIndex,
                                        UShort            aTupleRowID,
                                        qtcNode        ** aConstantFilter,
                                        qtcNode        ** aFilter,
                                        qtcNode        ** aLobFilter,
                                        qtcNode        ** aSubqueryFilter,
                                        qtcNode        ** aVarKeyRange,
                                        qtcNode        ** aVarKeyFilter,
                                        qtcNode        ** aVarKeyRange4Filter,
                                        qtcNode        ** aVarKeyFilter4Filter,
                                        qtcNode        ** aFixKeyRange,
                                        qtcNode        ** aFixKeyFilter,
                                        qtcNode        ** aFixKeyRange4Print,
                                        qtcNode        ** aFixKeyFilter4Print,
                                        qtcNode        ** aRidRange,
                                        idBool          * aInSubQueryKeyRange)
{
/***********************************************************************
 *
 * Description : 주어진 정보로 부터 Predicate 처리를 한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    // 1. constant filter를 처리한다.
    // 2. Predicate를 index정보와 함계 keyRange , KeyFilter , filter ,
    //    lob filter, subquery filter로 분류 한다.
    // 3. keyRange 와 keyFilter는 다시 fixed와 variable로 구분을 한다.
    // 4. 이를 qtcNode 또는 smiRange등 맞는 자료 구조로 변환 한다.

    qmoPredicate      * sKeyRangePred        = NULL;
    qmoPredicate      * sKeyFilterPred       = NULL;
    qmoPredicate      * sFilter              = NULL;
    qmoPredicate      * sLobFilter           = NULL;
    qmoPredicate      * sSubqueryFilter      = NULL;
    qmoPredicate      * sLastFilter          = NULL;
    qtcNode           * sVarKeyRange4Filter  = NULL;
    qtcNode           * sVarKeyFilter4Filter = NULL;
    qtcNode           * sNode;
    qtcNode           * sOptimizedNode;
    idBool              sIsMemory          = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::processPredicate::__FT__" );

    if ( aConstantPredicate != NULL )
    {
        // constant Predicate의 처리
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          aConstantPredicate,
                                          & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement,
                                           aQuerySet,
                                           sNode )
                  != IDE_SUCCESS );

        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        *aConstantFilter = sOptimizedNode;
    }
    else
    {
        *aConstantFilter = NULL;

    }

    if ( aRidPredicate != NULL )
    {
        IDE_TEST( makeRidRangePredicate( aRidPredicate,
                                         aPredicate,
                                         & aRidPredicate,
                                         & aPredicate,
                                         aRidRange )
                  != IDE_SUCCESS );
    }
    else
    {
        *aRidRange = NULL;
    }

    // qmoPredicate index로 부터 Predicate 분류
    sIsMemory = isMemoryTableFromTuple( aStatement , aTupleRowID );

    if ( aPredicate != NULL )
    {
        IDE_TEST( qmoPred::makeRangeAndFilterPredicate( aStatement,
                                                        aQuerySet,
                                                        sIsMemory,
                                                        aPredicate,
                                                        aIndex,
                                                        & sKeyRangePred,
                                                        & sKeyFilterPred,
                                                        & sFilter,
                                                        & sLobFilter,
                                                        & sSubqueryFilter )
                     != IDE_SUCCESS );
    }
    else
    {
        sKeyRangePred   = NULL;
        sKeyFilterPred  = NULL;
        sFilter         = NULL;
        sLobFilter      = NULL;
        sSubqueryFilter = NULL;
    }


    // fixed , variable구분
    if ( sKeyRangePred != NULL )
    {

        if ( (sKeyRangePred->flag & QMO_PRED_INSUBQ_KEYRANGE_MASK )
             == QMO_PRED_INSUBQ_KEYRANGE_TRUE )
        {
            *aInSubQueryKeyRange = ID_TRUE;
        }
        else
        {
            *aInSubQueryKeyRange = ID_FALSE;
        }

        //keyrange의 구분
        IDE_TEST( classifyFixedNVariable( aStatement,
                                          aQuerySet,
                                          & sKeyRangePred,
                                          aFixKeyRange,
                                          aFixKeyRange4Print,
                                          aVarKeyRange,
                                          & sVarKeyRange4Filter,
                                          & sFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        *aVarKeyRange       = NULL;
        *aFixKeyRange       = NULL;
        *aFixKeyRange4Print = NULL;
    }

    if ( sKeyFilterPred != NULL )
    {
        if ( sKeyRangePred != NULL )
        {
            //bug-7165
            //variable key range일때는 variable key filter만 생성한다.
            if ( ( sKeyRangePred->flag & QMO_PRED_VALUE_MASK )
                 == QMO_PRED_VARIABLE )
            {
                sKeyFilterPred->flag &= ~QMO_PRED_VALUE_MASK;
                sKeyFilterPred->flag |= QMO_PRED_VARIABLE;
            }
            else
            {
                //nothing to do
            }

            //keyFilter의 구분
            IDE_TEST( classifyFixedNVariable( aStatement,
                                              aQuerySet,
                                              & sKeyFilterPred,
                                              aFixKeyFilter,
                                              aFixKeyFilter4Print,
                                              aVarKeyFilter,
                                              & sVarKeyFilter4Filter,
                                              & sFilter )
                      != IDE_SUCCESS );
        }
        else
        {
            //nothing to do
            //key filter만 존재하는 경우

            if ( sFilter == NULL )
            {
                // predicate 분류시, keyRange 없이 keyFilter만 분류된 경우
                IDE_DASSERT(0);
            }
            else
            {
                // keyRange가 filter로 분류된 경우
                // (keyRange생성 메모리 크기 제한으로 인해)
                for ( sLastFilter        = sFilter;
                      sLastFilter->next != NULL;
                      sLastFilter        = sLastFilter->next ) ;

                sLastFilter->next = sKeyFilterPred;
            }

            *aVarKeyFilter       = NULL;
            *aFixKeyFilter       = NULL;
            *aFixKeyFilter4Print = NULL;

            sKeyFilterPred = NULL;
        }
    }
    else
    {
        *aVarKeyFilter       = NULL;
        *aFixKeyFilter       = NULL;
        *aFixKeyFilter4Print = NULL;
    }

    if ( sFilter != NULL )
    {
        // filter의 처리
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement ,
                                                sFilter ,
                                                & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           sNode )
                  != IDE_SUCCESS );

        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        *aFilter = sOptimizedNode;
    }
    else
    {
        *aFilter = NULL;
    }

    if ( sLobFilter != NULL )
    {
        IDE_FT_ASSERT( aLobFilter != NULL );

        // lobFilter의 처리
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement ,
                                                sLobFilter ,
                                                aLobFilter)
                  != IDE_SUCCESS );

        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           * aLobFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aLobFilter != NULL )
        {
            *aLobFilter = NULL;
        }
        else
        {
            // nothing to do
        }
    }

    if ( sSubqueryFilter != NULL )
    {
        // subqueryFilter의 처리
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement ,
                                                sSubqueryFilter ,
                                                & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           sNode )
                  != IDE_SUCCESS );

        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        IDE_FT_ASSERT( aSubqueryFilter != NULL );

        // BUG-38971 subQuery filter 를 정렬할 필요가 있습니다.
        if ( ( sOptimizedNode->node.lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_OR )
        {
            IDE_TEST( qmoPred::sortORNodeBySubQ( aStatement,
                                                 sOptimizedNode )
                      != IDE_SUCCESS );
        }
        else
        {
            //nothing to do
        }

        *aSubqueryFilter = sOptimizedNode;
    }
    else
    {
        if ( aSubqueryFilter != NULL )
        {
            *aSubqueryFilter = NULL;
        }
        else
        {
            //nothing to do
        }
    }

    if ( sVarKeyRange4Filter != NULL )
    {
        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sVarKeyRange4Filter,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        *aVarKeyRange4Filter = sOptimizedNode;
    }
    else
    {
        *aVarKeyRange4Filter = NULL;
    }

    if ( sVarKeyFilter4Filter != NULL )
    {
        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sVarKeyFilter4Filter,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        *aVarKeyFilter4Filter = sOptimizedNode;
    }
    else
    {
        *aVarKeyFilter4Filter = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::makeRidRangePredicate( qmoPredicate  * aInRidPred,
                                             qmoPredicate  * aInOtherPred,
                                             qmoPredicate ** aOutRidPred,
                                             qmoPredicate ** aOutOtherPred,
                                             qtcNode      ** aRidRange)
{
    IDE_RC sRc;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeRidRangePredicate::__FT__" );

    sRc = qmoRidPredExtractRangePred( aInRidPred,
                                      aInOtherPred,
                                      aOutRidPred,
                                      aOutOtherPred);
    IDE_TEST(sRc != IDE_SUCCESS);

    if ( *aOutRidPred == NULL )
    {
        *aRidRange = NULL;
    }
    else
    {
        *aRidRange = (*aOutRidPred)->node;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimplePROJ( qcStatement * aStatement,
                                qmncPROJ    * aPROJ )
{
/***********************************************************************
 *
 * Description : simple target 컬럼인지 검사한다.
 *
 * Implementation :
 *     - 정수형과 문자형 타입만 가능하다.
 *     - 순수컬럼과 상수만 가능하다.
 *     - limit이 없어야 한다.
 *
 ***********************************************************************/

    qmncPROJ      * sPROJ = aPROJ;
    qmsTarget     * sTarget;
    qtcNode       * sNode;
    mtcColumn     * sColumn;
    mtcColumn     * sColumnArg;
    void          * sInfo;
    idBool          sIsSimple = ID_FALSE;
    qmnValueInfo  * sValueInfo = NULL;
    UInt          * sValueSizes = NULL;
    UInt          * sOffsets = NULL;
    UInt            sOffset = 0;
    UInt            i;
    qmncPCRD      * sPCRD = NULL;
    qmnChildren   * sChildren = NULL;
    qmncSCAN      * sSCAN = NULL;
    qcmColumn     * sQcmColumn = NULL;
    mtcColumn     * sTemp = NULL;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimplePROJ::__FT__" );

    IDE_TEST_CONT( qciMisc::checkExecFast( aStatement, NULL ) == ID_FALSE,
                   NORMAL_EXIT );

    // loop가 없어야 한다.
    if ( sPROJ->loopNode != NULL )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    // scan limit 적용된 상수 limit만 가능
    if ( sPROJ->limit != NULL )
    {
        if ( ( sPROJ->limit->start.constant != 1 ) ||
             ( sPROJ->limit->count.constant == 0 ) ||
             ( sPROJ->limit->count.hostBindNode != NULL ) )
        {
            IDE_CONT( NORMAL_EXIT );
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

    // make simple values
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPROJ->targetCount * ID_SIZEOF( qmnValueInfo ),
                                               (void **) & sValueInfo )
              != IDE_SUCCESS );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPROJ->targetCount * ID_SIZEOF(UInt),
                                               (void **) & sValueSizes )
              != IDE_SUCCESS );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPROJ->targetCount * ID_SIZEOF(UInt),
                                               (void **) & sOffsets )
              != IDE_SUCCESS );

    sIsSimple = ID_TRUE;

    // simple target인 경우
    for ( sTarget  = sPROJ->myTarget, i = 0;
          sTarget != NULL;
          sTarget  = sTarget->next, i++ )
    {
        // target column이 순수컬럼이거나 상수이다.
        sColumn = QTC_STMT_COLUMN( aStatement, sTarget->targetColumn );

        // 정수형과 문자형만 가능하다.
        if ( ( sColumn->module->id == MTD_SMALLINT_ID ) ||
             ( sColumn->module->id == MTD_BIGINT_ID ) ||
             ( sColumn->module->id == MTD_INTEGER_ID ) ||
             ( sColumn->module->id == MTD_CHAR_ID ) ||
             ( sColumn->module->id == MTD_VARCHAR_ID ) ||
             ( sColumn->module->id == MTD_NUMERIC_ID ) ||
             ( sColumn->module->id == MTD_FLOAT_ID ) ||
             ( sColumn->module->id == MTD_DATE_ID ) )
        {
            // Nothing to do.
        }
        else
        {
            sIsSimple = ID_FALSE;
            break;
        }

        // init value
        QMN_INIT_VALUE_INFO( &(sValueInfo[i]) );

        // set column
        sValueInfo[i].column = *sColumn;

        if ( QTC_IS_COLUMN( aStatement, sTarget->targetColumn ) == ID_TRUE )
        {
            // 압축컬럼은 안된다.
            if ( ( sColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                 == SMI_COLUMN_COMPRESSION_TRUE )
            {
                sIsSimple = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }

            if ( sTarget->targetColumn->node.column == MTC_RID_COLUMN_ID )
            {
                // prowid
                sValueInfo[i].type = QMN_VALUE_TYPE_PROWID;

                sValueInfo[i].value.columnVal.table = sTarget->targetColumn->node.table;
                sValueInfo[i].value.columnVal.column = *sColumn;
            }
            else
            {
                // set target
                sValueInfo[i].type = QMN_VALUE_TYPE_COLUMN;

                sValueInfo[i].value.columnVal.table = sTarget->targetColumn->node.table;
                sValueInfo[i].value.columnVal.column = *sColumn;
            }
        }
        else
        {
            if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                                    sTarget->targetColumn )
                 == ID_TRUE )
            {
                // set target
                sValueInfo[i].type = QMN_VALUE_TYPE_CONST_VALUE;
                sValueInfo[i].value.constVal =
                    QTC_STMT_FIXEDDATA(aStatement, sTarget->targetColumn);
            }
            else
            {
                // BUG-43076 simple to_char
                if ( ( sTarget->targetColumn->node.module == &mtfTo_char ) &&
                     ( ( sTarget->targetColumn->node.lflag &
                         MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 ) )
                {
                    sNode = (qtcNode*) sTarget->targetColumn->node.arguments;
                    sInfo = QTC_STMT_EXECUTE( aStatement, sTarget->targetColumn )
                        ->calculateInfo;
                    sColumnArg = QTC_STMT_COLUMN( aStatement, sNode );

                    if ( ( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE ) &&
                         ( sColumnArg->module->id == MTD_DATE_ID ) &&
                         ( sInfo != NULL ) )
                    {
                        // set target
                        sValueInfo[i].type                   = QMN_VALUE_TYPE_TO_CHAR;
                        sValueInfo[i].value.columnVal.table  = sNode->node.table;
                        sValueInfo[i].value.columnVal.column = *sColumnArg;
                        sValueInfo[i].value.columnVal.info   = sInfo;
                    }
                    else
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
            }
        }

        // target tuple size
        sValueSizes[i] = idlOS::align8( sColumn->column.size );

        sOffsets[i]  = sOffset;
        sOffset     += sValueSizes[i];
    }

    if ( sPROJ->plan.left->type == QMN_PCRD )
    {
        sPCRD = (qmncPCRD*)sPROJ->plan.left;

        IDE_TEST_CONT( sPCRD->mIsSimple == ID_FALSE, NORMAL_EXIT );

        for ( sChildren = sPCRD->plan.children;
              sChildren != NULL;
              sChildren = sChildren->next )
        {
            sSCAN = (qmncSCAN*)sChildren->childPlan;

            IDU_FIT_POINT( "qmoOneNonPlan::checkSimplePROJ::alloc::sTemp",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPROJ->targetCount * ID_SIZEOF( mtcColumn ),
                                                       (void **)&sTemp )
                      != IDE_SUCCESS );

            for ( sTarget  = sPROJ->myTarget, i = 0;
                  sTarget != NULL;
                  sTarget  = sTarget->next, i++ )
            {
                sColumn = QTC_STMT_COLUMN( aStatement, sTarget->targetColumn );

                for ( sQcmColumn = sSCAN->partitionRef->partitionInfo->columns;
                      sQcmColumn != NULL;
                      sQcmColumn = sQcmColumn->next )
                {
                    if ( sQcmColumn->basicInfo->column.id == sColumn->column.id )
                    {
                        sTemp[i] = *sQcmColumn->basicInfo;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            sSCAN->mSimpleColumns = sTemp;
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    sPROJ->isSimple            = sIsSimple;
    sPROJ->simpleValues        = sValueInfo;
    sPROJ->simpleValueSizes    = sValueSizes;
    sPROJ->simpleResultOffsets = sOffsets;
    sPROJ->simpleResultSize    = sOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimpleSCAN( qcStatement  * aStatement,
                                qmncSCAN     * aSCAN )
{
/***********************************************************************
 *
 * Description : simple scan 노드인지 검사한다.
 *
 * Implementation :
 *     - 정수형과 문자형 타입만 가능하다.
 *     - 상수와 호스트 변수만 가능하다.
 *     - fixed key range와 variable key range만 가능하다.
 *     - full scan이나 index full scan만 가능하다.
 *     - index key를 모두 사용하지 않아도 가능하다.
 *
 ***********************************************************************/

    qmncSCAN      * sSCAN = aSCAN;
    idBool          sIsSimple = ID_FALSE;
    idBool          sIsUnique = ID_FALSE;
    idBool          sRidScan = ID_FALSE;
    UInt            sValueCount = 0;
    UInt            sCompareOpCount = 0;
    qmnValueInfo  * sValueInfo = NULL;
    UInt            sOffset = 0;
    qtcNode       * sORNode;
    qtcNode       * sCompareNode;
    qtcNode       * sColumnNode;
    qtcNode       * sValueNode;
    qcmIndex      * sIndex;
    UInt            i;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimpleSCAN::__FT__" );

    IDE_TEST_CONT( qciMisc::checkExecFast( aStatement, NULL ) == ID_FALSE,
                   NORMAL_EXIT );

    // memory table만 가능
    if ( ( sSCAN->tableRef->remoteTable != NULL ) ||
         ( qcuTemporaryObj::isTemporaryTable( sSCAN->tableRef->tableInfo )
           == ID_TRUE ) )
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    if ( sSCAN->tableRef->tableInfo->tablePartitionType
         == QCM_PARTITIONED_TABLE )
    {
        if ( smiIsDiskTable( sSCAN->partitionRef->partitionHandle ) == ID_TRUE )
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
        if ( smiIsDiskTable( sSCAN->tableRef->tableHandle ) == ID_TRUE )
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }

    // 상수 limit만 가능
    if ( sSCAN->limit != NULL )
    {
        if ( ( sSCAN->limit->start.hostBindNode != NULL ) ||
             ( sSCAN->limit->count.constant == 0 ) ||
             ( sSCAN->limit->count.hostBindNode != NULL ) )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
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

    // simple scan이 아닌 경우
    if ( ( sSCAN->method.fixKeyFilter   != NULL ) ||
         ( sSCAN->method.varKeyFilter   != NULL ) ||
         ( sSCAN->method.constantFilter != NULL ) ||
         ( sSCAN->method.filter         != NULL ) ||
         ( sSCAN->method.subqueryFilter != NULL ) ||
         ( sSCAN->nnfFilter             != NULL ) ||
         /* BUG-45312 Simple Query 에서 Index가 있을 경우 MIN, MAX값 오류 */
         ( ( sSCAN->flag & QMNC_SCAN_NOTNULL_RANGE_MASK )
           == QMNC_SCAN_NOTNULL_RANGE_TRUE ) ||
         /* BUG-45372 Simple Query FixedTable Index Bug */
         ( ( sSCAN->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
           == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE ) )
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    if ( sSCAN->method.ridRange == NULL )
    {
        // simple full scan/index full scan인 경우
        if ( ( sSCAN->method.fixKeyRange == NULL ) &&
             ( sSCAN->method.varKeyRange == NULL ) )
        {
            // 레코드가 많은 경우 simple로 처리하지 않는다.
            // (일단은 1024000개 까지만)
            if ( sSCAN->tableRef->statInfo->totalRecordCnt
                 <= QMO_STAT_TABLE_RECORD_COUNT * 100 )
            {
                sIsSimple = ID_TRUE;
            }
            else
            {
                sIsSimple = ID_FALSE;
            }

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        // index는 반드시 존재해야 한다.
        IDE_FT_ASSERT( sSCAN->method.index != NULL );

        // simple index scan인 경우
        sIndex = sSCAN->method.index;

        // ascending index만 가능
        for ( i = 0; i < sIndex->keyColCount; i++ )
        {
            if ( ( sIndex->keyColsFlag[i] & SMI_COLUMN_ORDER_MASK )
                 != SMI_COLUMN_ORDER_ASCENDING )
            {
                sIsSimple = ID_FALSE;

                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                // Nothing to do.
            }
        }

        // make simple values
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ( sIndex->keyColCount + 1 ) * ID_SIZEOF( qmnValueInfo ), // min,max
                                                   (void **) & sValueInfo )
                  != IDE_SUCCESS );

        // fixed key range와 variable key range는 동시에 존재하지 않는다.
        IDE_FT_ASSERT( ( ( sSCAN->method.fixKeyRange != NULL ) &&
                       ( sSCAN->method.varKeyRange == NULL ) ) ||
                     ( ( sSCAN->method.fixKeyRange == NULL ) &&
                       ( sSCAN->method.varKeyRange != NULL ) ) );

        if ( sSCAN->method.fixKeyRange != NULL )
        {
            sORNode = sSCAN->method.fixKeyRange;
        }
        else
        {
            sORNode = sSCAN->method.varKeyRange;
        }

        // OR와 AND로만 이루어져 있다.
        IDE_FT_ASSERT( sORNode->node.module == &mtfOr );
        IDE_FT_ASSERT( sORNode->node.arguments->module == &mtfAnd );

        // 하나의 AND만 존재해야 한다.
        if ( sORNode->node.arguments->next != NULL )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        sIsSimple = ID_TRUE;

        for ( sCompareNode  = (qtcNode*)(sORNode->node.arguments->arguments), i = 0;
              sCompareNode != NULL;
              sCompareNode  = (qtcNode*)(sCompareNode->node.next), i++ )
        {
            /* BUG-49058 */
            if ( i >= ( sIndex->keyColCount + 1 ) )
            {
                sIsSimple = ID_FALSE;
                break;
            }

            // init value
            QMN_INIT_VALUE_INFO( &(sValueInfo[i]) );

            // =인 경우
            if ( sCompareNode->node.module == &mtfEqual )
            {
                // =이전에 =이 아니었다면 simple이 아니다.
                if ( sCompareOpCount == 0 )
                {
                    // set type
                    sValueInfo[i].op = QMN_VALUE_OP_EQUAL;
                }
                else
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
            }
            else
            {
                // <, <=, >, >=인 경우
                if ( sCompareNode->node.module == &mtfLessThan )
                {
                    if ( sCompareNode->indexArgument == 0 )
                    {
                        // set type
                        sValueInfo[i].op = QMN_VALUE_OP_LT;
                    }
                    else
                    {
                        sValueInfo[i].op = QMN_VALUE_OP_GT;
                    }
                }
                else if ( sCompareNode->node.module == &mtfLessEqual )
                {
                    if ( sCompareNode->indexArgument == 0 )
                    {
                        // set type
                        sValueInfo[i].op = QMN_VALUE_OP_LE;
                    }
                    else
                    {
                        sValueInfo[i].op = QMN_VALUE_OP_GE;
                    }
                }
                else if ( sCompareNode->node.module == &mtfGreaterThan )
                {
                    if ( sCompareNode->indexArgument == 0 )
                    {
                        // set type
                        sValueInfo[i].op = QMN_VALUE_OP_GT;
                    }
                    else
                    {
                        sValueInfo[i].op = QMN_VALUE_OP_LT;
                    }
                }
                else if ( sCompareNode->node.module == &mtfGreaterEqual )
                {
                    if ( sCompareNode->indexArgument == 0 )
                    {
                        // set type
                        sValueInfo[i].op = QMN_VALUE_OP_GE;
                    }
                    else
                    {
                        sValueInfo[i].op = QMN_VALUE_OP_LE;
                    }
                }
                else
                {
                    sIsSimple = ID_FALSE;
                    break;
                }

                sCompareOpCount++;

                // 2개를 넘으면 에러
                if ( sCompareOpCount <= 2 )
                {
                    // Nothing to do.
                }
                else
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
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

            // check column and value
            IDE_TEST( checkSimpleSCANValue( aStatement,
                                            &( sValueInfo[i] ),
                                            sColumnNode,
                                            sValueNode,
                                            & sOffset,
                                            & sIsSimple )
                      != IDE_SUCCESS );

            if ( sIsSimple == ID_FALSE )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }

            // 마지막 두개의 비교연산이 동일 컬럼이어야 하고
            // 부호가 다른 비교연산이어야 한다.
            if ( sCompareOpCount == 2 )
            {
                if ( ( sValueInfo[i-1].column.column.id == sValueInfo[i].column.column.id ) &&
                     ( ( ( sValueInfo[i-1].op == QMN_VALUE_OP_LT ) &&
                         ( ( sValueInfo[i].op == QMN_VALUE_OP_GT ) ||
                           ( sValueInfo[i].op == QMN_VALUE_OP_GE ) ) )
                       ||
                       ( ( sValueInfo[i-1].op == QMN_VALUE_OP_LE ) &&
                         ( ( sValueInfo[i].op == QMN_VALUE_OP_GT ) ||
                           ( sValueInfo[i].op == QMN_VALUE_OP_GE ) ) )
                       ||
                       ( ( sValueInfo[i-1].op == QMN_VALUE_OP_GT ) &&
                         ( ( sValueInfo[i].op == QMN_VALUE_OP_LT ) ||
                           ( sValueInfo[i].op == QMN_VALUE_OP_LE ) ) )
                       ||
                       ( ( sValueInfo[i-1].op == QMN_VALUE_OP_GE ) &&
                         ( ( sValueInfo[i].op == QMN_VALUE_OP_LT ) ||
                           ( sValueInfo[i].op == QMN_VALUE_OP_LE ) ) ) ) )
                {
                    // Nothing to do.
                }
                else
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
            }
            else
            {
                // Nothing to do.
            }

            // =이전에 동일 컬럼이 올 수 없고, <,<=,>,>=인 경우 동일 컬럼만 가능하다.
            if ( sValueInfo[i].op == QMN_VALUE_OP_EQUAL )
            {
                if ( i > 0 )
                {
                    if ( sValueInfo[i - 1].column.column.id ==
                         sValueInfo[i].column.column.id )
                    {
                        sIsSimple = ID_FALSE;
                        break;
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
                // 2개인 경우 이전과 동일 컬럼이어야 한다.
                if ( sCompareOpCount == 2 )
                {
                    if ( sValueInfo[i - 1].column.column.id ==
                         sValueInfo[i].column.column.id )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        IDE_TEST_CONT( sIsSimple == ID_FALSE, NORMAL_EXIT );

        sValueCount = i;

        // 최대 1건인 경우
        if ( ( sIndex->keyColCount == sValueCount ) &&
             ( sIndex->isUnique == ID_TRUE ) &&
             ( sCompareOpCount == 0 ) )
        {
            sIsUnique = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        /* BUG-45376 Simple Query Index Bug */
        if ( sIndex->keyColCount < sValueCount )
        {
            sIsSimple = ID_FALSE;
        }
        else
        {
            for ( i = 0; i < sValueCount; i++ )
            {
                if ( sValueInfo[i].column.column.id != sIndex->keyColumns[i].column.id )
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }
    else
    {
        // rid scan인 경우
        sORNode = sSCAN->method.ridRange;

        // make simple values
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmnValueInfo ),
                                                   (void **)& sValueInfo )
                  != IDE_SUCCESS );

        sValueCount = 1;

        // OR로만 이루어져 있다.
        IDE_FT_ASSERT( sORNode->node.module == &mtfOr );

        // OR의 argument에 equal이 있다.
        sCompareNode = (qtcNode*)(sORNode->node.arguments);

        // 하나의 equal만 허용한다.
        if ( ( sCompareNode->node.module == &mtfEqual ) &&
             ( sCompareNode->node.next == NULL ) )
        {
            // Nothing to do.
        }
        else
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }

        sIsSimple = ID_TRUE;
        sIsUnique = ID_TRUE;
        sRidScan  = ID_TRUE;

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

        // init value
        QMN_INIT_VALUE_INFO( sValueInfo );

        // set type
        sValueInfo->op = QMN_VALUE_OP_EQUAL;

        // check column and value
        IDE_TEST( checkSimpleSCANValue( aStatement,
                                        sValueInfo,
                                        sColumnNode,
                                        sValueNode,
                                        & sOffset,
                                        & sIsSimple )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    sSCAN->isSimple             = sIsSimple;
    sSCAN->simpleValueCount     = sValueCount;
    sSCAN->simpleValues         = sValueInfo;
    sSCAN->simpleValueBufSize   = sOffset;
    sSCAN->simpleUnique         = sIsUnique;
    sSCAN->simpleRid            = sRidScan;
    sSCAN->simpleCompareOpCount = sCompareOpCount;
    sSCAN->mSimpleColumns       = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimpleSCANValue( qcStatement  * aStatement,
                                     qmnValueInfo * aValueInfo,
                                     qtcNode      * aColumnNode,
                                     qtcNode      * aValueNode,
                                     UInt         * aOffset,
                                     idBool       * aIsSimple )
{
    idBool      sIsSimple = ID_TRUE;
    mtcColumn * sColumn;
    mtcColumn * sConstColumn;
    mtcColumn * sValueColumn;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimpleSCANValue::__FT__" );

    // 순수컬럼과 값이어야 한다.
    if ( QTC_IS_COLUMN( aStatement, aColumnNode ) == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }

    sColumn = QTC_STMT_COLUMN( aStatement, aColumnNode );

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

    aValueInfo->column = *sColumn;

    // 컬럼, 상수나 호스트변수만 가능하다.
    if ( QTC_IS_COLUMN( aStatement, aValueNode ) == ID_TRUE )
    {
        // 컬럼인 경우 column node와 value node에 conversion이
        // 없어야 한다.
        if ( ( aColumnNode->node.conversion != NULL ) ||
             ( aValueNode->node.conversion != NULL ) )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        sValueColumn = QTC_STMT_COLUMN( aStatement, aValueNode );

        // 동일 타입이어야 한다.
        if ( sColumn->module->id == sValueColumn->module->id )
        {
            aValueInfo->type = QMN_VALUE_TYPE_COLUMN;

            aValueInfo->value.columnVal.table  = aValueNode->node.table;
            aValueInfo->value.columnVal.column = *sValueColumn;
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
                            aValueNode )
         == ID_TRUE )
    {
        // 상수인 경우 column node에 conversion이 없어야 한다.
        if ( aColumnNode->node.conversion != NULL )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        sConstColumn = QTC_STMT_COLUMN( aStatement, aValueNode );

        // 상수는 동일 타입이어야 한다.
        if ( ( sColumn->module->id == sConstColumn->module->id ) ||
             ( ( sColumn->module->id == MTD_NUMERIC_ID ) &&
               ( sConstColumn->module->id == MTD_FLOAT_ID ) ) ||
             ( ( sColumn->module->id == MTD_FLOAT_ID ) &&
               ( sConstColumn->module->id == MTD_NUMERIC_ID ) ) )
        {
            aValueInfo->type = QMN_VALUE_TYPE_CONST_VALUE;
            aValueInfo->value.constVal =
                QTC_STMT_FIXEDDATA(aStatement, aValueNode);
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

    // post process를 수행했으므로 host const warpper가 달릴 수 있다.
    if ( aValueNode->node.module == &qtc::hostConstantWrapperModule )
    {
        aValueNode = (qtcNode*) aValueNode->node.arguments;
    }
    else
    {
        // Nothing to do.
    }

    if ( qtc::isHostVariable( QC_SHARED_TMPLATE(aStatement),
                              aValueNode )
         == ID_TRUE )
    {
        // 호스트 변수인 경우 column node와 value node의
        // conversion node는 동일 타입으로만 bind한다면
        // 무시할 수 있다.
        aValueInfo->type = QMN_VALUE_TYPE_HOST_VALUE;
        aValueInfo->value.id = aValueNode->node.column;

        // value buffer size
        (*aOffset) += idlOS::align8( sColumn->column.size );

        // param 등록
        IDE_TEST( qmv::describeParamInfo( aStatement,
                                          sColumn,
                                          aValueNode )
                  != IDE_SUCCESS );

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    // 여기까지 왔다면 simple이 아니다.
    sIsSimple = ID_FALSE;

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aIsSimple = sIsSimple;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimpleINST( qcStatement * aStatement,
                                qmncINST    * aINST )
{
/***********************************************************************
 *
 * Description : simple insert 노드인지 검사한다.
 *
 * Implementation :
 *     - insert values만 가능하다.
 *     - 숫자형, 문자형, 날짜형만 가능하다.
 *     - 상수, 호스트변수, sysdate만 가능하다.
 *     - trigger, lob column, return into등은 안된다.
 *
 ***********************************************************************/

    qmncINST      * sINST = aINST;
    idBool          sIsSimple = ID_FALSE;
    UInt            sValueCount = 0;
    qmnValueInfo  * sValueInfo = NULL;
    UInt            sOffset = 0;
    qcmTableInfo  * sTableInfo = NULL;
    qmmValueNode  * sValueNode;
    qcmParentInfo * sParentInfo;
    UInt            i;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimpleINST::__FT__" );

    IDE_TEST_CONT( qciMisc::checkExecFast( aStatement, NULL ) == ID_FALSE,
                   NORMAL_EXIT );

    sIsSimple = ID_TRUE;

    // 기타 등등 검사
    if ( ( sINST->isInsertSelect == ID_TRUE ) ||
         ( sINST->isMultiInsertSelect == ID_TRUE ) ||
         ( sINST->insteadOfTrigger == ID_TRUE ) ||
         ( sINST->isAppend == ID_TRUE ) ||
         ( sINST->tableRef->tableInfo->triggerCount > 0 ) ||
         ( sINST->tableRef->tableInfo->lobColumnCount > 0 ) ||
         ( smiIsDiskTable( sINST->tableRef->tableHandle )
           == ID_TRUE ) ||
         ( qcuTemporaryObj::isTemporaryTable( sINST->tableRef->tableInfo )
           == ID_TRUE ) ||
         ( sINST->compressedTuple != UINT_MAX ) ||
         ( sINST->nextValSeqs != NULL ) ||
         ( sINST->defaultExprColumns != NULL ) ||
         ( sINST->checkConstrList != NULL ) ||
         ( sINST->returnInto != NULL ) )
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    if ( sINST->tableRef->tableInfo->tablePartitionType
         == QCM_PARTITIONED_TABLE )
    {
        sINST->flag &= ~QMNC_INST_PARTITIONED_MASK;
        sINST->flag |= QMNC_INST_PARTITIONED_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-43410 foreign key 지원
    for ( sParentInfo  = sINST->parentConstraints;
          sParentInfo != NULL;
          sParentInfo  = sParentInfo->next )
    {
        if ( smiIsDiskTable( sParentInfo->parentTableRef->tableHandle ) == ID_TRUE )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }
    }

    sTableInfo  = sINST->tableRef->tableInfo;
    sValueCount = sTableInfo->columnCount;

    // make simple values
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sValueCount * ID_SIZEOF( qmnValueInfo ),
                                               (void **) & sValueInfo )
              != IDE_SUCCESS );

    if ( sINST->rows->next != NULL )
    {
        sIsSimple = ID_FALSE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    for ( sValueNode = sINST->rows->values, i = 0;
          ( sValueNode != NULL ) && ( i < sValueCount );
          sValueNode = sValueNode->next, i++ )
    {
        // simple이 아닌 value
        if ( ( sValueNode->timestamp == ID_TRUE ) ||
             ( sValueNode->value == NULL ) )
        {
            sIsSimple = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }

        // init value
        QMN_INIT_VALUE_INFO( &(sValueInfo[i]) );

        //BUG-45715 support enqueue 
        if ( sValueNode->msgID == ID_TRUE )
        {
            sValueInfo[i].isQueue = ID_TRUE;
        }
        else
        {
            // nothing to do
        }

        // check column and value
        IDE_TEST( checkSimpleINSTValue( aStatement,
                                        &( sValueInfo[i] ),
                                        sTableInfo->columns[i].basicInfo,
                                        sValueNode,
                                        & sOffset,
                                        & sIsSimple )
                  != IDE_SUCCESS );

        if ( sIsSimple == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_CONT( sIsSimple == ID_FALSE, NORMAL_EXIT );

    if ( ( sValueNode == NULL ) && ( i == sValueCount ) )
    {
        // Nothing to do.
    }
    else
    {
        sIsSimple = ID_FALSE;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    sINST->isSimple           = sIsSimple;
    sINST->simpleValueCount   = sValueCount;
    sINST->simpleValues       = sValueInfo;
    sINST->simpleValueBufSize = sOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimpleINSTValue( qcStatement  * aStatement,
                                     qmnValueInfo * aValueInfo,
                                     mtcColumn    * aColumn,
                                     qmmValueNode * aValueNode,
                                     UInt         * aOffset,
                                     idBool       * aIsSimple )
{
    idBool          sIsSimple = ID_TRUE;
    mtcColumn     * sConstColumn;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimpleINSTValue::__FT__" );

    // 정수형과 문자형이 가능하다.
    if ( ( aColumn->module->id == MTD_SMALLINT_ID ) ||
         ( aColumn->module->id == MTD_BIGINT_ID ) ||
         ( aColumn->module->id == MTD_INTEGER_ID ) ||
         ( aColumn->module->id == MTD_CHAR_ID ) ||
         ( aColumn->module->id == MTD_VARCHAR_ID ) ||
         ( aColumn->module->id == MTD_NUMERIC_ID ) ||
         ( aColumn->module->id == MTD_FLOAT_ID ) ||
         ( aColumn->module->id == MTD_DATE_ID ) )
    {
        // Nothing to do.
    }
    else
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }

    aValueInfo->column = *aColumn;

    if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                            aValueNode->value )
         == ID_TRUE )
    {
        sConstColumn = QTC_STMT_COLUMN( aStatement, aValueNode->value );

        // 상수는 동일 타입이어야 한다.
        if ( ( aColumn->module->id == sConstColumn->module->id ) ||
             ( ( aColumn->module->id == MTD_NUMERIC_ID ) &&
               ( sConstColumn->module->id == MTD_FLOAT_ID ) ) ||
             ( ( aColumn->module->id == MTD_FLOAT_ID ) &&
               ( sConstColumn->module->id == MTD_NUMERIC_ID ) ) )
        {
            aValueInfo->type = QMN_VALUE_TYPE_CONST_VALUE;
            aValueInfo->value.constVal =
                QTC_STMT_FIXEDDATA(aStatement, aValueNode->value);

            // canonize buffer size
            (*aOffset) += idlOS::align8( aColumn->column.size );
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

    if ( qtc::isHostVariable( QC_SHARED_TMPLATE(aStatement),
                              aValueNode->value )
         == ID_TRUE )
    {
        aValueInfo->type     = QMN_VALUE_TYPE_HOST_VALUE;
        aValueInfo->value.id = aValueNode->value->node.column;

        // value buffer size
        (*aOffset) += idlOS::align8( aColumn->column.size );

        // param 등록
        IDE_TEST( qmv::describeParamInfo( aStatement,
                                          aColumn,
                                          aValueNode->value )
                  != IDE_SUCCESS );

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    // sysdate일 수 있다.
    if ( ( aColumn->module->id == MTD_DATE_ID ) &&
         ( QC_SHARED_TMPLATE(aStatement)->sysdate != NULL ) )
    {
        if ( ( aValueNode->value->node.table ==
               QC_SHARED_TMPLATE(aStatement)->sysdate->node.table ) &&
             ( aValueNode->value->node.column ==
               QC_SHARED_TMPLATE(aStatement)->sysdate->node.column ) )
        {
            aValueInfo->type = QMN_VALUE_TYPE_SYSDATE;
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

    // 여기까지 왔다면 simple이 아니다.
    sIsSimple = ID_FALSE;

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aIsSimple = sIsSimple;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimpleUPTE( qcStatement  * aStatement,
                                qmncUPTE     * aUPTE )
{
/***********************************************************************
 *
 * Description : simple update 노드인지 검사한다.
 *
 * Implementation :
 *     simple update인 경우 fast execute를 수행한다.
 *
 ***********************************************************************/

    qmncUPTE        * sUPTE = aUPTE;
    idBool            sIsSimple = ID_FALSE;
    qmnValueInfo    * sValueInfo = NULL;
    UInt              sOffset = 0;
    qcmColumn       * sColumnNode;
    qmmValueNode    * sValueNode;
    qcmParentInfo   * sParentInfo;
    qcmRefChildInfo * sChildInfo;
    UInt              i;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimpleUPTE::__FT__" );

    IDE_TEST_CONT( qciMisc::checkExecFast( aStatement, NULL ) == ID_FALSE,
                   NORMAL_EXIT );

    sIsSimple = ID_TRUE;

    // 기타 등등 검사
    if ( ( sUPTE->insteadOfTrigger == ID_TRUE ) ||
         ( sUPTE->tableRef->tableInfo->triggerCount > 0 ) ||
         ( sUPTE->compressedTuple != UINT_MAX ) ||
         ( sUPTE->subqueries != NULL ) ||
         ( sUPTE->nextValSeqs != NULL ) ||
         ( sUPTE->defaultExprColumns != NULL ) ||
         ( sUPTE->checkConstrList != NULL ) ||
         ( sUPTE->returnInto != NULL ) || 
         ( sUPTE->updateType == QMO_UPDATE_ROWMOVEMENT ) )
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    if ( sUPTE->tableRef->tableInfo->tablePartitionType
         == QCM_PARTITIONED_TABLE )
    {
        sUPTE->flag &= ~QMNC_UPTE_PARTITIONED_MASK;
        sUPTE->flag |= QMNC_UPTE_PARTITIONED_TRUE;

        if ( sUPTE->tableRef->partitionSummary != NULL )
        {
            if ( sUPTE->tableRef->partitionSummary->diskPartitionCount > 0 )
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
    }
    else
    {
        if ( smiIsDiskTable( sUPTE->tableRef->tableHandle ) == ID_TRUE )
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }

    // scan limit 적용된 상수 limit만 가능
    if ( sUPTE->limit != NULL )
    {
        if ( ( sUPTE->limit->start.constant != 1 ) ||
             ( sUPTE->limit->count.constant == 0 ) ||
             ( sUPTE->limit->count.hostBindNode != NULL ) )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
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

    // BUG-43410 foreign key 지원
    for ( sParentInfo  = sUPTE->parentConstraints;
          sParentInfo != NULL;
          sParentInfo  = sParentInfo->next )
    {
        if ( smiIsDiskTable( sParentInfo->parentTableRef->tableHandle ) == ID_TRUE )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-43410 foreign key 지원
    for ( sChildInfo  = sUPTE->childConstraints;
          sChildInfo != NULL;
          sChildInfo  = sChildInfo->next )
    {
        if ( ( sChildInfo->isSelfRef == ID_TRUE ) ||
             ( smiIsDiskTable( sChildInfo->childTableRef->tableHandle ) == ID_TRUE ) ||
             /* BUG-45370 Simple Foreignkey Child constraint trigger */
             ( sChildInfo->childTableRef->tableInfo->triggerCount > 0 ) )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }
    }

    // make simple values
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sUPTE->updateColumnCount * ID_SIZEOF( qmnValueInfo ),
                                               (void **) & sValueInfo )
              != IDE_SUCCESS );

    for ( sColumnNode = sUPTE->columns, sValueNode = sUPTE->values, i = 0;
          ( sColumnNode != NULL ) && ( sValueNode != NULL );
          sColumnNode = sColumnNode->next, sValueNode = sValueNode->next, i++ )
    {
        // init value
        QMN_INIT_VALUE_INFO( &(sValueInfo[i]) );

        IDE_TEST( checkSimpleUPTEValue( aStatement,
                                        &( sValueInfo[i] ),
                                        sColumnNode->basicInfo,
                                        sValueNode,
                                        & sOffset,
                                        & sIsSimple )
                     != IDE_SUCCESS );

        if ( sIsSimple == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_CONT( sIsSimple == ID_FALSE, NORMAL_EXIT );

    if ( ( sColumnNode == NULL ) && ( sValueNode == NULL ) )
    {
        // Nothing to do.
    }
    else
    {
        sIsSimple = ID_FALSE;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    sUPTE->isSimple           = sIsSimple;
    sUPTE->simpleValues       = sValueInfo;
    sUPTE->simpleValueBufSize = sOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimpleUPTEValue( qcStatement  * aStatement,
                                     qmnValueInfo * aValueInfo,
                                     mtcColumn    * aColumn,
                                     qmmValueNode * aValueNode,
                                     UInt         * aOffset,
                                     idBool       * aIsSimple )
{
    idBool          sIsSimple = ID_TRUE;
    mtcColumn     * sConstColumn;
    mtcColumn     * sSetMtcColumn;
    qtcNode       * sSetColumn;
    qtcNode       * sSetValue;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimpleUPTEValue::__FT__" );

    // simple value가 아님
    if ( ( aValueNode->timestamp == ID_TRUE ) ||
         ( aValueNode->msgID == ID_TRUE ) ||
         ( aValueNode->value == NULL ) )
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    aValueInfo->column = *aColumn;

    // 정수형과 문자형이 가능하다.
    if ( ( aColumn->module->id == MTD_SMALLINT_ID ) ||
         ( aColumn->module->id == MTD_BIGINT_ID ) ||
         ( aColumn->module->id == MTD_INTEGER_ID ) ||
         ( aColumn->module->id == MTD_CHAR_ID ) ||
         ( aColumn->module->id == MTD_VARCHAR_ID ) ||
         ( aColumn->module->id == MTD_NUMERIC_ID ) ||
         ( aColumn->module->id == MTD_FLOAT_ID ) ||
         ( aColumn->module->id == MTD_DATE_ID ) )
    {
        // Nothing to do.
    }
    else
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }

    sSetValue = aValueNode->value;

    // =, +, -만 가능하다.
    if ( ( sSetValue->node.module == &qtc::columnModule ) ||
         ( sSetValue->node.module == &qtc::valueModule ) )
    {
        // set i1 = 1
        aValueInfo->op = QMN_VALUE_OP_ASSIGN;
    }
    else if ( sSetValue->node.module == &mtfAdd2 )
    {
        // 숫자형만 가능하다.
        if ( ( aColumn->module->id == MTD_SMALLINT_ID ) ||
             ( aColumn->module->id == MTD_BIGINT_ID ) ||
             ( aColumn->module->id == MTD_INTEGER_ID ) ||
             ( aColumn->module->id == MTD_NUMERIC_ID ) ||
             ( aColumn->module->id == MTD_FLOAT_ID ) )
        {
            // Nothing to do.
        }
        else
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }

        // set i1 = i1 + 1
        aValueInfo->op = QMN_VALUE_OP_ADD;

        sSetColumn = (qtcNode*)(sSetValue->node.arguments);
        sSetMtcColumn = QTC_STMT_COLUMN( aStatement, sSetColumn );

        if ( aColumn->column.id == sSetMtcColumn->column.id )
        {
            sSetValue = (qtcNode*)(sSetColumn->node.next);
        }
        else
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
    }
    else if ( sSetValue->node.module == &mtfSubtract2 )
    {
        // 숫자형만 가능하다.
        if ( ( aColumn->module->id == MTD_SMALLINT_ID ) ||
             ( aColumn->module->id == MTD_BIGINT_ID ) ||
             ( aColumn->module->id == MTD_INTEGER_ID ) ||
             ( aColumn->module->id == MTD_NUMERIC_ID ) ||
             ( aColumn->module->id == MTD_FLOAT_ID ) )
        {
            // Nothing to do.
        }
        else
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }

        // set i1 = i1 - 1
        aValueInfo->op = QMN_VALUE_OP_SUB;

        sSetColumn = (qtcNode*)(sSetValue->node.arguments);
        sSetMtcColumn = QTC_STMT_COLUMN( aStatement, sSetColumn );

        if ( aColumn->column.id == sSetMtcColumn->column.id )
        {
            sSetValue = (qtcNode*)(sSetColumn->node.next);
        }
        else
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
    }
    else
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }

    if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                            sSetValue )
         == ID_TRUE )
    {
        sConstColumn = QTC_STMT_COLUMN( aStatement, sSetValue );

        // 상수는 동일 타입이어야 한다.
        if ( ( aColumn->module->id == sConstColumn->module->id ) ||
             ( ( aColumn->module->id == MTD_NUMERIC_ID ) &&
               ( sConstColumn->module->id == MTD_FLOAT_ID ) ) ||
             ( ( aColumn->module->id == MTD_FLOAT_ID ) &&
               ( sConstColumn->module->id == MTD_NUMERIC_ID ) ) )
        {
            aValueInfo->type = QMN_VALUE_TYPE_CONST_VALUE;
            aValueInfo->value.constVal =
                QTC_STMT_FIXEDDATA(aStatement, sSetValue);

            // canonize buffer size, new value buffer size
            (*aOffset) += idlOS::align8( aColumn->column.size );
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

    if ( qtc::isHostVariable( QC_SHARED_TMPLATE(aStatement),
                              sSetValue )
         == ID_TRUE )
    {
        aValueInfo->type = QMN_VALUE_TYPE_HOST_VALUE;
        aValueInfo->value.id = sSetValue->node.column;

        // value buffer size, new value buffer size
        (*aOffset) += idlOS::align8( aColumn->column.size );

        // param 등록
        IDE_TEST( qmv::describeParamInfo( aStatement,
                                          aColumn,
                                          sSetValue )
                  != IDE_SUCCESS );

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    // sysdate일 수 있다.
    if ( ( aColumn->module->id == MTD_DATE_ID ) &&
         ( QC_SHARED_TMPLATE(aStatement)->sysdate != NULL ) )
    {
        if ( ( aValueNode->value->node.table ==
               QC_SHARED_TMPLATE(aStatement)->sysdate->node.table ) &&
             ( aValueNode->value->node.column ==
               QC_SHARED_TMPLATE(aStatement)->sysdate->node.column ) )
        {
            aValueInfo->type = QMN_VALUE_TYPE_SYSDATE;
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

    // 여기까지 왔다면 simple이 아니다.
    sIsSimple = ID_FALSE;

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aIsSimple = sIsSimple;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::checkSimpleDETE( qcStatement  * aStatement,
                                qmncDETE     * aDETE )
{
/***********************************************************************
 *
 * Description : simple delete 노드인지 검사한다.
 *
 * Implementation :
 *     simple delete인 경우 fast execute를 수행한다.
 *
 ***********************************************************************/

    qmncDETE        * sDETE = aDETE;
    qcmRefChildInfo * sChildInfo;
    idBool            sIsSimple = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::checkSimpleDETE::__FT__" );

    IDE_TEST_CONT( qciMisc::checkExecFast( aStatement, NULL ) == ID_FALSE,
                   NORMAL_EXIT );

    sIsSimple = ID_TRUE;

    if ( ( sDETE->insteadOfTrigger == ID_TRUE ) ||
         ( sDETE->tableRef->tableInfo->triggerCount > 0 ) ||
         ( sDETE->returnInto != NULL ) )
    {
        sIsSimple = ID_FALSE;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    if ( sDETE->tableRef->tableInfo->tablePartitionType
         == QCM_PARTITIONED_TABLE )
    {
        sDETE->flag &= ~QMNC_DETE_PARTITIONED_MASK;
        sDETE->flag |= QMNC_DETE_PARTITIONED_TRUE;

        if ( sDETE->tableRef->partitionSummary != NULL )
        {
            if ( sDETE->tableRef->partitionSummary->diskPartitionCount > 0 )
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
    }
    else
    {
        if ( smiIsDiskTable( sDETE->tableRef->tableHandle ) == ID_TRUE )
        {
            sIsSimple = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }

    // scan limit 적용된 상수 limit만 가능
    if ( sDETE->limit != NULL )
    {
        if ( ( sDETE->limit->start.constant != 1 ) ||
             ( sDETE->limit->count.constant == 0 ) ||
             ( sDETE->limit->count.hostBindNode != NULL ) )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
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

    // BUG-43410 foreign key 지원
    for ( sChildInfo  = sDETE->childConstraints;
          sChildInfo != NULL;
          sChildInfo  = sChildInfo->next )
    {
        if ( ( sChildInfo->isSelfRef == ID_TRUE ) ||
             ( smiIsDiskTable( sChildInfo->childTableRef->tableHandle ) == ID_TRUE ) ||
             /* BUG-45370 Simple Foreignkey Child constraint trigger */
             ( sChildInfo->childTableRef->tableInfo->triggerCount > 0 ) )
        {
            sIsSimple = ID_FALSE;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    sDETE->isSimple = sIsSimple;

    return IDE_SUCCESS;
}

IDE_RC
qmoOneNonPlan::initDLAY( qcStatement  * aStatement ,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : DLAY 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncDLAY의 할당 및 초기화
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *
 ***********************************************************************/

    qmncDLAY          * sDLAY;
    UInt                sDataNodeOffset;

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParent != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncDLAY의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncDLAY ),
                                               (void **)& sDLAY )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sDLAY ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_DLAY ,
                        qmnDLAY ,
                        qmndDLAY,
                        sDataNodeOffset );

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aParent->resultDesc,
                                   & sDLAY->plan.resultDesc )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sDLAY;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOneNonPlan::makeDLAY( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmnPlan      * aChildPlan ,
                         qmnPlan      * aPlan )
{
    qmncDLAY          * sDLAY = (qmncDLAY *)aPlan;
    UInt                sDataNodeOffset;

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildPlan != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    sDLAY->plan.left = aChildPlan;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndDLAY));

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sDLAY->flag = QMN_PLAN_FLAG_CLEAR;
    sDLAY->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sDLAY->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    //dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aQuerySet,
                                            & sDLAY->plan,
                                            QMO_DLAY_DEPENDENCY,
                                            0,
                                            NULL,
                                            0,
                                            NULL,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    sDLAY->plan.mParallelDegree  = aChildPlan->mParallelDegree;
    sDLAY->plan.flag            &= ~QMN_PLAN_NODE_EXIST_MASK;
    sDLAY->plan.flag            |= (aChildPlan->flag & QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::initSDSE( qcStatement  * aStatement,
                                qmnPlan      * aParent,
                                qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : SDSE 노드를 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSDSE * sSDSE = NULL;
    UInt       sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initSDSE::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aParent    != NULL );
    IDE_FT_ASSERT( aPlan      != NULL );

    //-------------------------------------------------------------
    // 초기화 작업
    //-------------------------------------------------------------

    // qmncSDSE 의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmncSDSE ),
                                             (void**)& sSDSE )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSDSE,
                        QC_SHARED_TMPLATE(aStatement),
                        QMN_SDSE,
                        qmnSDSE,
                        qmndSDSE,
                        sDataNodeOffset );

    IDE_TEST( qmc::copyResultDesc( aStatement,
                                   aParent->resultDesc,
                                   & sSDSE->plan.resultDesc )
              != IDE_SUCCESS );

    // shard plan index
    sSDSE->shardDataIndex = QC_SHARED_TMPLATE(aStatement)->shardExecData.planCount;
    QC_SHARED_TMPLATE(aStatement)->shardExecData.planCount++;

    *aPlan = (qmnPlan *)sSDSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::makeSDSE( qcStatement       * aStatement,
                                qmnPlan           * aParent,
                                qcNamePosition    * aShardQuery,
                                sdiAnalyzeInfo    * aShardAnalysis,
                                qcShardParamInfo  * aShardParamInfo, /* TASK-7219 Non-shard DML */
                                UShort              aShardParamCount,
                                qmgGraph          * aGraph,
                                qmnPlan           * aPlan )
{
    mtcTemplate    * sTemplate = NULL;
    qmncSDSE       * sSDSE = (qmncSDSE*)aPlan;
    qtcNode        * sPredicate[4];
    qmncScanMethod   sMethod;
    idBool           sInSubQueryKeyRange = ID_FALSE;
    idBool           sScanLimit = ID_TRUE;
    UInt             sDataNodeOffset = 0;
    UShort           sTupleID = ID_USHORT_MAX;
    UInt             i = 0;
    mtcColumn      * sColumn      = NULL;
    UShort           sColumnCount = 0; /* TASK-7219 */

    UShort      sShardParamCount = 0;

    idBool sIsPushProj = ID_FALSE; /* BUG-49154 */

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aParent    != NULL );
    IDE_FT_ASSERT( aGraph     != NULL );
    IDE_FT_ASSERT( aStatement->session != NULL );

    //----------------------------------
    // 초기화 작업
    //----------------------------------

    sTemplate       = &QC_SHARED_TMPLATE(aStatement)->tmplate;
    aPlan->offset   = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset + ID_SIZEOF(qmndSDSE) );

    sSDSE->tableRef          = aGraph->myFrom->tableRef;
    sSDSE->mQueryPos         = aShardQuery;
    sSDSE->mShardAnalysis    = aShardAnalysis;
    sSDSE->mShardParamInfo   = aShardParamInfo; /* TASK-7219 Non-shard DML */
    sSDSE->mShardParamCount  = aShardParamCount;
    sSDSE->tupleRowID        = sTupleID = sSDSE->tableRef->table;

    if ( sSDSE->shardDataIndex != 0 )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.partialStmt = ID_TRUE;
    }

    if ( aStatement->calledByPSMFlag == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.globalPSM = ID_TRUE;
    }

    /* TASK-7219 Non-shard DML */
    if ( QCG_GET_SHARD_PARTIAL_EXEC_TYPE( aStatement ) == SDI_SHARD_PARTIAL_EXEC_TYPE_COORD )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.partialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_QUERY;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.partialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;
    }

    // shard exec data 설정
    sSDSE->shardDataOffset = idlOS::align8( QC_SHARED_TMPLATE(aStatement)->shardExecData.dataSize );
    sSDSE->shardDataSize = 0;

    for ( i = 0; i < SDI_NODE_MAX_COUNT; i++ )
    {
        sSDSE->mBuffer[i] = sSDSE->shardDataOffset + sSDSE->shardDataSize;
        sSDSE->shardDataSize += idlOS::align8( sTemplate->rows[sTupleID].rowOffset );
    }

    /* TASK-7219 */
    for ( i = 0, sColumn = sTemplate->rows[ sSDSE->tableRef->table ].columns, sColumnCount = 0;
          i < sTemplate->rows[ sSDSE->tableRef->table ].columnCount;
          i++, sColumn++ )
    {
        if ( ( sColumn->flag & MTC_COLUMN_NULL_TYPE_MASK ) == MTC_COLUMN_NULL_TYPE_TRUE )
        {
            /* BUG-49154 */
            sIsPushProj = ID_TRUE;
        }
        else
        {
            sColumnCount++;
        }
    }

    if ( sColumnCount == 0 )
    {
        sColumnCount = 1;
    }
    else
    {
        /* Nothing to do */
    }

    sSDSE->mOffset = sSDSE->shardDataOffset + sSDSE->shardDataSize;
    sSDSE->shardDataSize += idlOS::align8( ID_SIZEOF(UInt) * sColumnCount ); /* TASK-7219 */

    sSDSE->mMaxByteSize = sSDSE->shardDataOffset + sSDSE->shardDataSize;
    sSDSE->shardDataSize += idlOS::align8( ID_SIZEOF(UInt) * sColumnCount ); /* TASK-7219 */

    sSDSE->mBindParam = sSDSE->shardDataOffset + sSDSE->shardDataSize;
    sSDSE->shardDataSize += idlOS::align8( ID_SIZEOF(sdiBindParam) * aShardParamCount );

    /* PROJ-2728 */
    sSDSE->mOutBindParam = sSDSE->shardDataOffset + sSDSE->shardDataSize;
    sSDSE->shardDataSize += idlOS::align8( ID_SIZEOF(sdiOutBindParam) * aShardParamCount );

    sSDSE->mOutRefBindData = sSDSE->shardDataOffset + sSDSE->shardDataSize;
    sSDSE->shardDataSize += idlOS::align8( getOutRefBindDataSize( sTemplate,
                                                                  sSDSE ) );

    /* BUG-49154 */
    if ( sIsPushProj == ID_TRUE )
    {
        sSDSE->mRowForTransformed = sSDSE->shardDataOffset + sSDSE->shardDataSize;
        sSDSE->shardDataSize += idlOS::align8( sTemplate->rows[sTupleID].rowOffset );
    }
    else
    {
        sSDSE->mRowForTransformed = 0;
    }

    QC_SHARED_TMPLATE(aStatement)->shardExecData.dataSize =
        sSDSE->shardDataOffset + sSDSE->shardDataSize;

    // Flag 설정
    sSDSE->flag      = QMN_PLAN_FLAG_CLEAR;
    sSDSE->plan.flag = QMN_PLAN_FLAG_CLEAR;

    // BUGBUG
    // SDSE 자체는 disk 로 생성하나
    // SDSE 의 상위 plan 의 interResultType 은 memory temp 로 수행되어야 한다.
    sSDSE->plan.flag &= ~QMN_PLAN_STORAGE_MASK;
    sSDSE->plan.flag |=  QMN_PLAN_STORAGE_DISK;

    //PROJ-2402 Parallel Table Scan
    sSDSE->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sSDSE->plan.flag |=  QMN_PLAN_PRLQ_EXIST_FALSE;
    sSDSE->plan.flag |=  QMN_PLAN_MTR_EXIST_FALSE;

    // PROJ-1071 Parallel Query
    sSDSE->plan.mParallelDegree = 1;

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // mtcTuple.flag세팅
    //----------------------------------

    sTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
    sTemplate->rows[sTupleID].lflag |=  MTC_TUPLE_STORAGE_DISK;

    sTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sTemplate->rows[sTupleID].lflag |=  MTC_TUPLE_PLAN_TRUE;

    sTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sTemplate->rows[sTupleID].lflag |=  MTC_TUPLE_PLAN_MTR_FALSE;

    sTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_TYPE_MASK;
    sTemplate->rows[sTupleID].lflag |=  MTC_TUPLE_TYPE_INTERMEDIATE;

    sTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
    sTemplate->rows[sTupleID].lflag |=  MTC_TUPLE_MATERIALIZE_VALUE;

    //----------------------------------
    // Predicate의 처리
    //----------------------------------

    // Host 변수를 포함한 Constant Expression의 최적화
    if ( aGraph->nnfFilter != NULL )
    {
        IDE_TEST( qtc::optimizeHostConstExpression( aStatement,
                                                    aGraph->nnfFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmoOneNonPlan::processPredicate( aStatement,
                                               aGraph->myQuerySet,
                                               aGraph->myPredicate,
                                               aGraph->constantPredicate,
                                               NULL,
                                               NULL,
                                               sSDSE->tupleRowID,
                                               &( sMethod.constantFilter ),
                                               &( sMethod.filter ),
                                               &( sMethod.lobFilter ),
                                               &( sMethod.subqueryFilter ),
                                               &( sMethod.varKeyRange ),
                                               &( sMethod.varKeyFilter ),
                                               &( sMethod.varKeyRange4Filter ),
                                               &( sMethod.varKeyFilter4Filter ),
                                               &( sMethod.fixKeyRange ),
                                               &( sMethod.fixKeyFilter ),
                                               &( sMethod.fixKeyRange4Print ),
                                               &( sMethod.fixKeyFilter4Print ),
                                               &( sMethod.ridRange ),
                                               & sInSubQueryKeyRange )
              != IDE_SUCCESS );

    IDE_TEST( qmoOneNonPlan::postProcessScanMethod( aStatement,
                                                    & sMethod,
                                                    & sScanLimit )
                 != IDE_SUCCESS );

    IDE_FT_ASSERT( sInSubQueryKeyRange     == ID_FALSE );
    IDE_FT_ASSERT( sMethod.varKeyRange         == NULL );
    IDE_FT_ASSERT( sMethod.varKeyFilter        == NULL );
    IDE_FT_ASSERT( sMethod.varKeyRange4Filter  == NULL );
    IDE_FT_ASSERT( sMethod.varKeyFilter4Filter == NULL );
    IDE_FT_ASSERT( sMethod.fixKeyRange         == NULL );
    IDE_FT_ASSERT( sMethod.fixKeyFilter        == NULL );
    IDE_FT_ASSERT( sMethod.fixKeyRange4Print   == NULL );
    IDE_FT_ASSERT( sMethod.fixKeyFilter4Print  == NULL );
    IDE_FT_ASSERT( sMethod.ridRange            == NULL );

    sSDSE->constantFilter = sMethod.constantFilter;
    sSDSE->subqueryFilter = sMethod.subqueryFilter;
    sSDSE->filter         = sMethod.filter;
    sSDSE->lobFilter      = sMethod.lobFilter; // PROJ-2728, BUG-25916
    sSDSE->nnfFilter      = aGraph->nnfFilter;

    sPredicate[0] = sSDSE->constantFilter;
    sPredicate[1] = sSDSE->subqueryFilter;
    sPredicate[2] = sSDSE->nnfFilter;
    sPredicate[3] = sSDSE->filter;

    // PROJ-1437 dependency 설정전에 predicate들의 위치정보를 변경.
    for ( i = 0; i <= 3; i++ )
    {
        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aGraph->myQuerySet,
                                           sPredicate[i],
                                           ID_USHORT_MAX,
                                           ID_TRUE )
                  != IDE_SUCCESS );

        for ( sShardParamCount = 0;
              sShardParamCount < sSDSE->mShardParamCount;
              sShardParamCount++ )
        {
            if ( sSDSE->mShardParamInfo[sShardParamCount].mIsOutRefColumnBind == ID_TRUE )
            {
                setOutRefBindReassign( sPredicate[i],
                                       &sSDSE->mShardParamInfo[sShardParamCount] );
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }

    //----------------------------------
    // dependency 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement,
                                            aGraph->myQuerySet,
                                            & sSDSE->plan,
                                            QMO_SDSE_DEPENDENCY,
                                            sSDSE->tupleRowID,
                                            NULL,
                                            4,
                                            sPredicate,
                                            0,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aGraph->myQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   & sSDSE->plan.resultDesc )
              != IDE_SUCCESS );

    //----------------------------------
    // PROJ-2462 Result Cache
    //----------------------------------

    qmo::addTupleID2ResultCacheStack( aStatement, sSDSE->tupleRowID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmoOneNonPlan::setOutRefBindReassign( qtcNode          * aFilter,
                                           qcShardParamInfo * aShardParamInfo )
{
    if ( aFilter != NULL )
    {
        if ( aFilter->node.module == &qtc::columnModule )
        {
            if ( ( aFilter->node.baseTable == aShardParamInfo->mOutRefTuple ) &&
                 ( aFilter->node.baseColumn == aShardParamInfo->mOffset ) )
            {
                aShardParamInfo->mOutRefTuple = aFilter->node.table;
                aShardParamInfo->mOffset  = aFilter->node.column;
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

        setOutRefBindReassign( (qtcNode*)aFilter->node.next,
                               aShardParamInfo );

        setOutRefBindReassign( (qtcNode*)aFilter->node.arguments,
                               aShardParamInfo );
    }
    else
    {
        /* Nothing to do. */
    }
}

UInt qmoOneNonPlan::getOutRefBindDataSize( mtcTemplate * aTemplate,
                                           qmncSDSE    * aCodePlan )
{
    UInt sOutRefBindDataSize = 0;

    qcShardParamInfo * sBindParamInfo = NULL;

    UShort             i = 0;

    mtcTuple         * sTuple  = NULL;
    mtcColumn        * sColumn = NULL;

    for ( ;
          i < aCodePlan->mShardParamCount; i++ )
    {
        sBindParamInfo = aCodePlan->mShardParamInfo + i;

        if ( sBindParamInfo->mIsOutRefColumnBind == ID_TRUE )
        {
            sTuple = & aTemplate->rows[ sBindParamInfo->mOutRefTuple ];
            sColumn = sTuple->columns + sBindParamInfo->mOffset;

            sOutRefBindDataSize += sColumn->column.size;
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return sOutRefBindDataSize;
}

IDE_RC qmoOneNonPlan::initSDEX( qcStatement  * aStatement,
                                qmnPlan     ** aPlan )
{
    qmncSDEX * sSDEX = NULL;
    UInt       sDataNodeOffset = 0;

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aPlan      != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncSDEX의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmncSDEX ),
                                             (void**)& sSDEX)
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSDEX,
                        QC_SHARED_TMPLATE(aStatement),
                        QMN_SDEX,
                        qmnSDEX,
                        qmndSDEX,
                        sDataNodeOffset );

    // shard plan index
    sSDEX->shardDataIndex = QC_SHARED_TMPLATE(aStatement)->shardExecData.planCount;
    QC_SHARED_TMPLATE(aStatement)->shardExecData.planCount++;

    *aPlan = (qmnPlan*)sSDEX;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::makeSDEX( qcStatement      * aStatement,
                                qcNamePosition   * aShardQuery,
                                sdiAnalyzeInfo   * aShardAnalysis,
                                qcShardParamInfo * aShardParamInfo, /* TASK-7219 Non-shard DML */
                                UShort             aShardParamCount,
                                qmnPlan          * aPlan )
{
    qmncSDEX * sSDEX = (qmncSDEX*)aPlan;
    UInt       sDataNodeOffset = 0;

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement  != NULL );
    IDE_FT_ASSERT( aShardQuery != NULL );
    IDE_FT_ASSERT( aPlan       != NULL );

    //----------------------------------
    // 초기화 작업
    //----------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8(aPlan->offset + ID_SIZEOF(qmndSDEX));

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    sSDEX->shardQuery.stmtText = aShardQuery->stmtText;
    sSDEX->shardQuery.offset   = aShardQuery->offset;
    sSDEX->shardQuery.size     = aShardQuery->size;

    sSDEX->shardAnalysis = aShardAnalysis;
    sSDEX->shardParamInfo = aShardParamInfo; /* TASK-7219 */
    sSDEX->shardParamCount = aShardParamCount;

    if ( sSDEX->shardDataIndex != 0 )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.partialStmt = ID_TRUE;
    }

    if ( aStatement->calledByPSMFlag == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.globalPSM = ID_TRUE;
    }

    /* TASK-7219 Non-shard DML */
    if ( aShardAnalysis->mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.partialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_COORD;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.partialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;
    }

    // shard exec data 설정
    sSDEX->shardDataOffset = idlOS::align8( QC_SHARED_TMPLATE(aStatement)->shardExecData.dataSize );
    sSDEX->shardDataSize = 0;

    sSDEX->bindParam = sSDEX->shardDataOffset + sSDEX->shardDataSize;
    sSDEX->shardDataSize += idlOS::align8( ID_SIZEOF(sdiBindParam) * aShardParamCount );

    /* PROJ-2728 */
    sSDEX->outBindParam = sSDEX->shardDataOffset + sSDEX->shardDataSize;
    sSDEX->shardDataSize += idlOS::align8( ID_SIZEOF(sdiOutBindParam) * aShardParamCount );

    QC_SHARED_TMPLATE(aStatement)->shardExecData.dataSize =
        sSDEX->shardDataOffset + sSDEX->shardDataSize;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sSDEX->flag      = QMN_PLAN_FLAG_CLEAR;
    sSDEX->plan.flag = QMN_PLAN_FLAG_CLEAR;

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    qtc::dependencyClear( & sSDEX->plan.depInfo );

    return IDE_SUCCESS;
}

IDE_RC qmoOneNonPlan::initSDIN( qcStatement   * aStatement ,
                                qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : Shard INST 노드를 생성한다.
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncSDIN의 할당 및 초기화
 *
 ***********************************************************************/

    qmncSDIN          * sSDIN;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::initSDIN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    //qmncSCAN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncSDIN ),
                                               (void **)& sSDIN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSDIN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_SDIN ,
                        qmnSDIN ,
                        qmndSDIN ,
                        sDataNodeOffset );

    // shard plan index
    sSDIN->shardDataIndex = QC_SHARED_TMPLATE(aStatement)->shardExecData.planCount;
    QC_SHARED_TMPLATE(aStatement)->shardExecData.planCount++;

    *aPlan = (qmnPlan *)sSDIN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::makeSDIN( qcStatement    * aStatement ,
                                qmoINSTInfo    * aINSTInfo ,
                                qcNamePosition * aShardQuery ,
                                sdiAnalyzeInfo * aShardAnalysis ,
                                qmnPlan        * aChildPlan ,
                                qmnPlan        * aPlan )
{
/***********************************************************************
 *
 * Description : SDIN 노드를 생성한다.
 *
 * Implementation :
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *
 ***********************************************************************/

    qmncSDIN          * sSDIN = (qmncSDIN *)aPlan;
    qcmTableInfo      * sTableForInsert;
    UInt                sDataNodeOffset;
    qmsFrom             sFrom;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoOneNonPlan::makeSDIN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aINSTInfo  != NULL );
    IDE_FT_ASSERT( aPlan      != NULL );

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndSDIN) );

    sSDIN->plan.left = aChildPlan;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sSDIN->flag = QMN_PLAN_FLAG_CLEAR;
    sSDIN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    if ( aChildPlan != NULL )
    {
        sSDIN->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);
    }
    else
    {
        // Nothing to do.
    }

    //Leaf Node에 tuple로 부터 memory 인지 disk table인지를 세팅
    //from tuple정보
    IDE_TEST( setTableTypeFromTuple( aStatement ,
                                     aINSTInfo->tableRef->table ,
                                     &( sSDIN->plan.flag ) )
              != IDE_SUCCESS );

    //----------------------------------
    // Display 정보 세팅
    //----------------------------------

    // tableRef만 있으면 됨
    sFrom.tableRef = aINSTInfo->tableRef;

    IDE_TEST( qmg::setDisplayInfo( & sFrom,
                                   &( sSDIN->tableOwnerName ),
                                   &( sSDIN->tableName ),
                                   &( sSDIN->aliasName ) )
              != IDE_SUCCESS );

    //----------------------------------------------------------------
    // 메인 작업
    //----------------------------------------------------------------

    sTableForInsert = aINSTInfo->tableRef->tableInfo;

    //----------------------------------
    // insert 관련 정보
    //----------------------------------

    // insert target 설정
    sSDIN->tableRef = aINSTInfo->tableRef;

    // insert select 설정
    if ( aChildPlan != NULL )
    {
        sSDIN->isInsertSelect = ID_TRUE;
    }
    else
    {
        sSDIN->isInsertSelect = ID_FALSE;
    }

    // insert columns 설정
    sSDIN->columns          = aINSTInfo->columns;
    sSDIN->columnsForValues = aINSTInfo->columnsForValues;

    // insert values 설정
    sSDIN->rows            = aINSTInfo->rows;
    sSDIN->valueIdx        = aINSTInfo->valueIdx;
    sSDIN->canonizedTuple  = aINSTInfo->canonizedTuple;
    sSDIN->queueMsgIDSeq   = aINSTInfo->queueMsgIDSeq;

    // sequence 정보
    sSDIN->nextValSeqs = aINSTInfo->nextValSeqs;

    // shard query 정보
    sSDIN->shardQuery.stmtText = aShardQuery->stmtText;
    sSDIN->shardQuery.offset   = aShardQuery->offset;
    sSDIN->shardQuery.size     = aShardQuery->size;

    // shard analysis 정보
    sSDIN->shardAnalysis = aShardAnalysis;
    sSDIN->shardParamCount = 0;
    for ( i = 0; i < sTableForInsert->columnCount; i++ )
    {
        if ( (sTableForInsert->columns[i].flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
             == QCM_COLUMN_HIDDEN_COLUMN_FALSE )
        {
            sSDIN->shardParamCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sSDIN->shardDataIndex != 0 )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.partialStmt = ID_TRUE;
    }

    if ( aStatement->calledByPSMFlag == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->shardExecData.globalPSM = ID_TRUE;
    }

    // shard exec data 설정
    sSDIN->shardDataOffset = idlOS::align8( QC_SHARED_TMPLATE(aStatement)->shardExecData.dataSize );
    sSDIN->shardDataSize = 0;

    sSDIN->bindParam = sSDIN->shardDataOffset + sSDIN->shardDataSize;
    sSDIN->shardDataSize += idlOS::align8( ID_SIZEOF(sdiBindParam) * sSDIN->shardParamCount );

    /* PROJ-2728 */
    sSDIN->outBindParam = sSDIN->shardDataOffset + sSDIN->shardDataSize;
    sSDIN->shardDataSize += idlOS::align8( ID_SIZEOF(sdiOutBindParam) * sSDIN->shardParamCount );

    QC_SHARED_TMPLATE(aStatement)->shardExecData.dataSize =
        sSDIN->shardDataOffset + sSDIN->shardDataSize;

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    // data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    qtc::dependencyClear( & sSDIN->plan.depInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::buildSerialFilterInfo( qcStatement          * aStatement,
                                             qmsHints             * aHints,
                                             qtcNode              * aFilter,
                                             UInt                 * aFilterSize,
                                             UInt                 * aFilterCount,
                                             mtxSerialFilterInfo ** aInfo )
{
/***********************************************************************
 *
 * Description :
 *  PROJ-2632
 *
 * Implementation :
 *
 ***********************************************************************/

    mtxSerialFilterInfo * sInfo       = NULL;
    mtxSerialFilterInfo * sHead       = NULL;
    UInt                  sFiterCount = 0;

    IDE_TEST_CONT( aFilter == NULL,
                   BUILD_ERROR );

    IDE_TEST_CONT( ( QC_SHARED_TMPLATE( aStatement )->flag & QC_TMP_DISABLE_SERIAL_FILTER_MASK )
                   == QC_TMP_DISABLE_SERIAL_FILTER_TRUE,
                   BUILD_ERROR );

    if ( aHints != NULL )
    {
        IDE_TEST_RAISE( aHints->mSerialFilter == QMS_SERIAL_FILTER_FALSE,
                        BUILD_ERROR );

        IDE_TEST_RAISE( ( ( aHints->mSerialFilter == QMS_SERIAL_FILTER_NONE ) &&
                          ( QCG_GET_SERIAL_EXECUTE_MODE( aStatement ) == 0 ) ),
                        BUILD_ERROR );
    }
    else
    {
        IDE_TEST_RAISE( QCG_GET_SERIAL_EXECUTE_MODE( aStatement ) == 0,
                        BUILD_ERROR );
    }

    IDE_TEST_RAISE( qtc::estimateSerializeFilter( aStatement,
                                                  &( aFilter->node ),
                                                  &( sFiterCount ) )
                     != IDE_SUCCESS, BUILD_ERROR );

    IDE_TEST_RAISE( sFiterCount <= 0,
                    BUILD_ERROR );

    IDE_TEST( qtc::allocateSerializeFilter( aStatement,
                                            sFiterCount,
                                            &( sInfo ) )
              != IDE_SUCCESS );

    sHead = sInfo;

    IDE_TEST_RAISE( qtc::recursiveSerializeFilter( aStatement,
                                                   &( aFilter->node ),
                                                   sInfo + sFiterCount,
                                                   &( sHead ) )
                   != IDE_SUCCESS, BUILD_ERROR );

    *aFilterSize  = sHead->mOffset + sHead->mHeader.mSize;
    *aFilterCount = sFiterCount;
    *aInfo        = sInfo;

    IDE_EXCEPTION_CONT( BUILD_ERROR );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::initMultiUPTE( qcStatement     * aStatement,
                                     qmsQuerySet     * aQuerySet,
                                     qmmMultiTables  * aTables,
                                     qmnPlan        ** aPlan )
{
    qmncUPTE        * sUPTE;
    UInt              sDataNodeOffset;
    qmmMultiTables  * sTmp;
    qtcNode         * sNode;

    IDU_FIT_POINT("qmoOneNonPlan::initMultiUPTE::alloc::sUPTE",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncUPTE ),
                                               (void **)& sUPTE )
        != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sUPTE ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_UPTE ,
                        qmnUPTE ,
                        qmndUPTE,
                        sDataNodeOffset );

    for ( sTmp = aTables; sTmp != NULL; sTmp = sTmp->mNext )
    {
        for ( sNode = sTmp->mColumnList;
              sNode != NULL;
              sNode = (qtcNode *)sNode->node.next )
        {
            IDE_TEST( qmc::appendPredicate( aStatement,
                                            aQuerySet,
                                            &sUPTE->plan.resultDesc,
                                            sNode )
                      != IDE_SUCCESS );
        }
    }

    *aPlan = (qmnPlan *)sUPTE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::makeMultiUPTE( qcStatement   * aStatement,
                                     qmsQuerySet   * aQuerySet,
                                     qmoUPTEInfo   * aUPTEInfo,
                                     qmnPlan       * aChildPlan,
                                     qmnPlan       * aPlan )
{
    qmncUPTE          * sUPTE = (qmncUPTE *)aPlan;
    UInt                sDataNodeOffset;
    qmsPartitionRef   * sPartitionRef;
    UInt                sPartitionCount = 0;
    qcmColumn         * sColumn;
    qmmMultiTables    * sTmp;
    UInt                i;

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------
    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndUPTE) );

    sUPTE->plan.left = aChildPlan;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sUPTE->flag = QMN_PLAN_FLAG_CLEAR;
    sUPTE->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sUPTE->plan.flag |= (aChildPlan->flag & QMN_PLAN_STORAGE_MASK);

    sUPTE->flag &= ~QMNC_UPTE_MULTIPLE_TABLE_MASK;
    sUPTE->flag |= QMNC_UPTE_MULTIPLE_TABLE_TRUE;

    // key preserved property
    if ( aQuerySet->SFWGH->preservedInfo != NULL )
    {
        if ( aQuerySet->SFWGH->preservedInfo->useKeyPreservedTable == ID_TRUE )
        {
            sUPTE->flag &= ~QMNC_UPTE_VIEW_KEY_PRESERVED_MASK;
            sUPTE->flag |= QMNC_UPTE_VIEW_KEY_PRESERVED_TRUE;
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

    //----------------------------------
    // update 관련 정보
    //----------------------------------

    // update target table 설정
    sUPTE->tableRef = aUPTEInfo->updateTableRef;
    // update column 관련 정보
    sUPTE->columns           = aUPTEInfo->columns;
    sUPTE->updateColumnList  = aUPTEInfo->updateColumnList;
    sUPTE->updateColumnCount = aUPTEInfo->updateColumnCount;

    sUPTE->mTableList = aUPTEInfo->mTableList;

    for ( sTmp = sUPTE->mTableList, i = 0; sTmp != NULL; sTmp = sTmp->mNext )
    {
        i++;
    }
    sUPTE->mMultiTableCount = i;

    for ( sTmp = sUPTE->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        if ( sTmp->mColumnCount > 0 )
        {
            IDU_FIT_POINT("qmoOneNonPlan::makeMultiUPTE::alloc::mColumnIDs",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sTmp->mColumnCount * ID_SIZEOF(UInt),
                                                       (void**) &sTmp->mColumnIDs )
                      != IDE_SUCCESS );
            for ( i = 0, sColumn = sTmp->mColumns; i < sTmp->mColumnCount; i++, sColumn = sColumn->next )
            {
                sTmp->mColumnIDs[i] = sColumn->basicInfo->column.id;
            }
        }
        else
        {
            sTmp->mColumnIDs = NULL;
        }

        // partition별 update column list 생성
        if ( sTmp->mTableRef->partitionRef != NULL )
        {
            sPartitionCount = 0;
            for ( sPartitionRef  = sTmp->mTableRef->partitionRef;
                  sPartitionRef != NULL;
                  sPartitionRef  = sPartitionRef->next )
            {
                sPartitionCount++;
            }

            IDU_FIT_POINT("qmoOneNonPlan::makeMultiUPTE::alloc::mPartColumnList",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                       (void **) &( sTmp->mPartColumnList ) )
                      != IDE_SUCCESS );

            for ( sPartitionRef = sTmp->mTableRef->partitionRef, i = 0;
                  sPartitionRef != NULL;
                  sPartitionRef = sPartitionRef->next, i++ )
            {
                IDE_TEST( qmoPartition::makePartUpdateColumnList( aStatement,
                                                                  sPartitionRef,
                                                                  sTmp->mUptColumnList,
                                                                  &( sTmp->mPartColumnList[i] ) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            sTmp->mPartColumnList = NULL;
        }

        //----------------------------------
        // constraint 설정
        //----------------------------------

        // Foreign Key Referencing을 위한 Master Table이 존재하는 지 검사
        if ( qdnForeignKey::haveToCheckParent( sTmp->mTableRef->tableInfo,
                                               sTmp->mColumnIDs,
                                               sTmp->mColumnCount )
             == ID_TRUE )
        {
            /* Nothing to do */
        }
        else
        {
            sTmp->mParentConst = NULL;
        }

        // Child Table의 Referencing 조건 검사
        if ( qdnForeignKey::haveToOpenBeforeCursor( sTmp->mChildConst,
                                                    sTmp->mColumnIDs,
                                                    sTmp->mColumnCount )
             == ID_TRUE )
        {
            /* Nothing to do */
        }
        else
        {
            sTmp->mChildConst = NULL;
        }

        //----------------------------------
        // PROJ-1784 DML Without Retry
        //----------------------------------
        IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                        sTmp->mTableRef->table,
                                                        &sTmp->mWhereColumnList )
                  != IDE_SUCCESS );
        IDE_TEST( qdbCommon::makeSetClauseColumnList( aStatement,
                                                      sTmp->mTableRef->table,
                                                      &sTmp->mSetColumnList )
                  != IDE_SUCCESS );

        // partition별 where column list 생성
        if ( sTmp->mTableRef->partitionRef != NULL )
        {
            IDU_FIT_POINT("qmoOneNonPlan::makeMultiUPTE::alloc::mWherePartColumnList",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                       (void **) &( sTmp->mWherePartColumnList ) )
                      != IDE_SUCCESS );
            IDU_FIT_POINT("qmoOneNonPlan::makeMultiUPTE::alloc::mSetPartColumnList",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                       (void **) &( sTmp->mSetPartColumnList ) )
                      != IDE_SUCCESS );

            for ( sPartitionRef  = sTmp->mTableRef->partitionRef, i = 0;
                  sPartitionRef != NULL;
                  sPartitionRef  = sPartitionRef->next, i++ )
            {
                /* PROJ-2464 hybrid partitioned table 지원 */
                IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                                sPartitionRef->table,
                                                                &( sTmp->mWherePartColumnList[i] ) )
                          != IDE_SUCCESS );

                IDE_TEST( qdbCommon::makeSetClauseColumnList( aStatement,
                                                              sPartitionRef->table,
                                                              &( sTmp->mSetPartColumnList[i] ) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            sTmp->mWherePartColumnList = NULL;
            sTmp->mSetPartColumnList = NULL;
        }
    }

    // update value 관련 정보
    sUPTE->values         = aUPTEInfo->values;
    sUPTE->subqueries     = aUPTEInfo->subqueries;
    sUPTE->valueIdx       = aUPTEInfo->valueIdx;
    sUPTE->canonizedTuple = aUPTEInfo->canonizedTuple;
    sUPTE->compressedTuple= aUPTEInfo->compressedTuple;
    sUPTE->isNull         = aUPTEInfo->isNull;

    // sequence 정보
    sUPTE->nextValSeqs    = aUPTEInfo->nextValSeqs;

    // instead of trigger 설정
    sUPTE->insteadOfTrigger = aUPTEInfo->insteadOfTrigger;
    sUPTE->checkConstrList  = aUPTEInfo->checkConstrList;
    sUPTE->insertTableRef   = aUPTEInfo->insertTableRef;
    sUPTE->isRowMovementUpdate = aUPTEInfo->isRowMovementUpdate;
    // update type
    sUPTE->updateType = aUPTEInfo->updateType;
    sUPTE->cursorType    = aUPTEInfo->cursorType;
    sUPTE->inplaceUpdate = aUPTEInfo->inplaceUpdate;

    if ( aQuerySet->SFWGH->hints != NULL )
    {
        if( aQuerySet->SFWGH->hints->withoutRetry == ID_TRUE )
        {
            sUPTE->withoutRetry = ID_TRUE ;
        }
        else
        {
            sUPTE->withoutRetry = ( QCU_DML_WITHOUT_RETRY_ENABLE == 1 ) ? ID_TRUE : ID_FALSE ;
        }
    }
    else
    {
        sUPTE->withoutRetry = ID_FALSE;
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            &sUPTE->plan ,
                                            QMO_UPTE_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    sUPTE->isSimple = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::initMultiDETE( qcStatement        * aStatement,
                                     qmsQuerySet        * aQuerySet,
                                     qmmDelMultiTables  * aTables,
                                     qmnPlan           ** aPlan )
{
    qmncDETE           * sDETE;
    UInt                 sDataNodeOffset;
    qmmDelMultiTables  * sTmp;
    qtcNode            * sNode;

    //qmncSCAN의 할당및 초기화
    IDU_FIT_POINT("qmoOneNonPlan::makeMultiDETE::alloc::sDETE",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmncDETE ),
                                               (void **)& sDETE )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sDETE ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_DETE ,
                        qmnDETE ,
                        qmndDETE,
                        sDataNodeOffset );

    for ( sTmp = aTables; sTmp != NULL; sTmp = sTmp->mNext )
    {
        for ( sNode = sTmp->mColumnList;
              sNode != NULL;
              sNode = (qtcNode *)sNode->node.next )
        {
            IDE_TEST( qmc::appendPredicate( aStatement,
                                            aQuerySet,
                                            &sDETE->plan.resultDesc,
                                            sNode )
                      != IDE_SUCCESS );
        }
    }

    *aPlan = (qmnPlan *)sDETE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOneNonPlan::makeMultiDETE( qcStatement * aStatement,
                                     qmsQuerySet * aQuerySet,
                                     qmoDETEInfo * aDETEInfo,
                                     qmnPlan     * aChildPlan,
                                     qmnPlan     * aPlan )
{
    qmncDETE          * sDETE = (qmncDETE *)aPlan;
    UInt                sDataNodeOffset;
    qmsPartitionRef   * sPartitionRef;
    UInt                sPartitionCount;
    UInt                i;
    qmmDelMultiTables * sTmp;

    //----------------------------------------------------------------
    // 초기화 작업
    //----------------------------------------------------------------
    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset = idlOS::align8( aPlan->offset +
                                     ID_SIZEOF(qmndDETE) );

    sDETE->plan.left = aChildPlan;

    sDETE->flag &= QMNC_DETE_MULTIPLE_TABLE_MASK;
    sDETE->flag |= QMNC_DETE_MULTIPLE_TABLE_TRUE;

    // key preserved property
    if ( aQuerySet->SFWGH->preservedInfo != NULL )
    {
        if ( aQuerySet->SFWGH->preservedInfo->useKeyPreservedTable == ID_TRUE )
        {
            sDETE->flag &= ~QMNC_DETE_VIEW_KEY_PRESERVED_MASK;
            sDETE->flag |= QMNC_DETE_VIEW_KEY_PRESERVED_TRUE;
        }
    }

    sDETE->mTableList = aDETEInfo->mTableList;
    sDETE->tableRef = NULL;
    sDETE->insteadOfTrigger = ID_FALSE;
    sDETE->limit = NULL;
    sDETE->childConstraints = aDETEInfo->childConstraints;
    sDETE->returnInto = NULL;
    sDETE->isSimple = ID_FALSE;

    for ( sTmp = sDETE->mTableList, i = 0; sTmp != NULL; sTmp = sTmp->mNext )
    {
        i++;
    }
    sDETE->mMultiTableCount = i;

    for ( sTmp = sDETE->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                        sTmp->mTableRef->table,
                                                        &sTmp->mWhereColumnList )
                  != IDE_SUCCESS );

        if ( sTmp->mTableRef->partitionRef != NULL )
        {
            sPartitionCount = 0;
            for ( sPartitionRef  = sTmp->mTableRef->partitionRef;
                  sPartitionRef != NULL;
                  sPartitionRef  = sPartitionRef->next )
            {
                sPartitionCount++;
            }
            IDU_FIT_POINT("qmoOneNonPlan::makeMultiDETE::alloc::mWherePartColumnList",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sPartitionCount * ID_SIZEOF( smiColumnList* ),
                                                       (void **) &( sTmp->mWherePartColumnList ) )
                      != IDE_SUCCESS );
            for ( sPartitionRef  = sTmp->mTableRef->partitionRef, i = 0;
                  sPartitionRef != NULL;
                  sPartitionRef  = sPartitionRef->next, i++ )
            {
                /* PROJ-2464 hybrid partitioned table 지원 */
                IDE_TEST( qdbCommon::makeWhereClauseColumnList( aStatement,
                                                                sPartitionRef->table,
                                                                &( sTmp->mWherePartColumnList[i] ) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( aQuerySet->SFWGH->hints != NULL )
    {
        if( aQuerySet->SFWGH->hints->withoutRetry == ID_TRUE )
        {
            sDETE->withoutRetry = ID_TRUE ;
        }
        else
        {
            sDETE->withoutRetry = ( QCU_DML_WITHOUT_RETRY_ENABLE == 1 ) ? ID_TRUE : ID_FALSE ;
        }
    }
    else
    {
        sDETE->withoutRetry = ID_FALSE;
    }

    //----------------------------------------------------------------
    // 마무리 작업
    //----------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sDETE->plan ,
                                            QMO_DETE_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    sDETE->isSimple = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

