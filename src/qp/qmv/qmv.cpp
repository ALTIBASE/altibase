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
 * $Id: qmv.cpp 91235 2021-07-16 00:41:05Z donovan.seo $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <cm.h>
#include <qcg.h>
#include <qmmParseTree.h>
#include <qmsParseTree.h>
#include <qmv.h>
#include <qcmCache.h>
#include <qcmUser.h>
#include <qmvQuerySet.h>
#include <qmvQTC.h>
#include <qmvOrderBy.h>
#include <qmvWith.h>
#include <qtc.h>
#include <qtcDef.h>
#include <qcmTableInfo.h>
#include <qcmFixedTable.h>
#include <qcmPerformanceView.h>
#include <qcuSqlSourceInfo.h>
#include <qdParseTree.h>
#include <qcpManager.h>
#include <qdn.h>
#include <qdv.h>
#include <qcm.h>
#include <smi.h>
#include <smiMisc.h>
#include <qdpPrivilege.h>
#include <qcmSynonym.h>
#include <qmo.h>
#include <smiTableSpace.h>
#include <qmoPartition.h>
#include <qcgPlan.h>
#include <qcsModule.h>
#include <qsv.h>
#include <qsxEnv.h>
#include <qdnCheck.h>
#include <qdnTrigger.h>
#include <qmsDefaultExpr.h>
#include <qmsPreservedTable.h>
#include <qcmView.h>
#include <qmr.h>
#include <qdpRole.h>
#include <qmvGBGSTransform.h> // PROJ-2415 Grouping Sets Clause
#include <qmvShardTransform.h>
#include <qsvProcVar.h>

extern mtdModule mtdBigint;
extern mtfModule mtfDecrypt;
extern mtfModule mtfAnd;
extern mtdModule mtdInteger;

// PROJ-1705
// update column을
// 테이블생성컬럼순서로 정렬하기 위한 임시 구조체
// 레코드 구조가 바뀜에 따라
// sm에서 column을 테이블생성컬럼순서로 처리할 수 있도록
// update column을 테이블생성컬럼순서로 정렬한다.
typedef struct qmvUpdateColumnIdx
{
    qcmColumn      * column;
    qmmValueNode   * value;
} qmvUpdateColumnIdx;

extern "C" SInt
compareUpdateColumnID( const void* aElem1, const void* aElem2 )
{
/***********************************************************************
 *
 * Description :
 *    compare columnID
 *
 * Implementation :
 *
 ***********************************************************************/
    
    IDE_DASSERT( (qmvUpdateColumnIdx*)aElem1 != NULL );
    IDE_DASSERT( (qmvUpdateColumnIdx*)aElem2 != NULL );

    //--------------------------------
    // compare columnID
    //--------------------------------

    if( ((qmvUpdateColumnIdx*)aElem1)->column->basicInfo->column.id >
        ((qmvUpdateColumnIdx*)aElem2)->column->basicInfo->column.id )
    {
        return 1;
    }
    else if( ((qmvUpdateColumnIdx*)aElem1)->column->basicInfo->column.id <
             ((qmvUpdateColumnIdx*)aElem2)->column->basicInfo->column.id )
    {
        return -1;
    }
    else
    {
        // 동일한 컬럼을 중복 업데이트 할 수 없다.
        // 예: update t1 set i1 = 1, i1 = 1;
        IDE_ASSERT(0);
    }
}


IDE_RC qmv::insertCommon( qcStatement       * aStatement,
                          qmsTableRef       * aTableRef,
                          qdConstraintSpec ** aCheckConstrSpec,
                          qcmColumn        ** aDefaultExprColumns,
                          qmsTableRef      ** aDefaultTableRef )
{
    qcmTableInfo       * sInsertTableInfo;
    qmsTableRef        * sDefaultTableRef;
    UInt                 sFlag = 0;

    IDU_FIT_POINT_FATAL( "qmv::insertCommon::__FT__" );
    
    sInsertTableInfo = aTableRef->tableInfo;

    // check grant
    IDE_TEST( qdpRole::checkDMLInsertTablePriv(
                  aStatement,
                  sInsertTableInfo->tableHandle,
                  sInsertTableInfo->tableOwnerID,
                  sInsertTableInfo->privilegeCount,
                  sInsertTableInfo->privilegeInfo,
                  ID_FALSE,
                  NULL,
                  NULL )
              != IDE_SUCCESS );

    // environment의 기록
    IDE_TEST( qcgPlan::registerPlanPrivTable(
                  aStatement,
                  QCM_PRIV_ID_OBJECT_INSERT_NO,
                  sInsertTableInfo )
              != IDE_SUCCESS );

    /* PROJ-1107 Check Constraint 지원 */
    IDE_TEST( qdnCheck::makeCheckConstrSpecRelatedToTable(
                  aStatement,
                  aCheckConstrSpec,
                  sInsertTableInfo->checks,
                  sInsertTableInfo->checkCount )
              != IDE_SUCCESS );

    /* PROJ-1107 Check Constraint 지원 */
    IDE_TEST( qdnCheck::setMtcColumnToCheckConstrList(
                  aStatement,
                  sInsertTableInfo,
                  *aCheckConstrSpec )
              != IDE_SUCCESS );

    /* PROJ-1090 Function-based Index */
    IDE_TEST( qmsDefaultExpr::makeDefaultExpressionColumnsRelatedToTable(
                  aStatement,
                  aDefaultExprColumns,
                  sInsertTableInfo )
              != IDE_SUCCESS );

    if ( *aDefaultExprColumns != NULL )
    {
        /* PROJ-2204 Join Update, Delete */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmsTableRef, &sDefaultTableRef )
                  != IDE_SUCCESS );

        QCP_SET_INIT_QMS_TABLE_REF( sDefaultTableRef );

        sDefaultTableRef->userName.stmtText = sInsertTableInfo->tableOwnerName;
        sDefaultTableRef->userName.offset   = 0;
        sDefaultTableRef->userName.size     = idlOS::strlen(sInsertTableInfo->tableOwnerName);

        sDefaultTableRef->tableName.stmtText = sInsertTableInfo->name;
        sDefaultTableRef->tableName.offset   = 0;
        sDefaultTableRef->tableName.size     = idlOS::strlen(sInsertTableInfo->name);

        sDefaultTableRef->aliasName.stmtText = sInsertTableInfo->name;
        sDefaultTableRef->aliasName.offset   = 0;
        sDefaultTableRef->aliasName.size     = idlOS::strlen(sInsertTableInfo->name);

        IDE_TEST( qmvQuerySet::validateQmsTableRef( aStatement,
                                                    NULL,
                                                    sDefaultTableRef,
                                                    sFlag,
                                                    MTC_COLUMN_NOTNULL_TRUE ) /* PR-13597 */
                  != IDE_SUCCESS );

        /* BUG-17409 */
        sDefaultTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
        sDefaultTableRef->flag |=  QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

        /*
         * PROJ-1090, PROJ-2429
         * Variable column, Compressed column을
         * Fixed Column으로 변환한 TableRef를 만든다.
         */
        IDE_TEST( qtc::nextTable( &(sDefaultTableRef->table),
                                  aStatement,
                                  NULL,     /* Tuple ID만 얻는다. */
                                  QCM_TABLE_TYPE_IS_DISK( sDefaultTableRef->tableInfo->tableFlag ),
                                  MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                  != IDE_SUCCESS );

        IDE_TEST( qmvQuerySet::makeTupleForInlineView( aStatement,
                                                       sDefaultTableRef,
                                                       sDefaultTableRef->table,
                                                       MTC_COLUMN_NOTNULL_TRUE )
                  != IDE_SUCCESS );

        *aDefaultTableRef = sDefaultTableRef;
    }
    else
    {
        *aDefaultTableRef = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::parseInsertValues( qcStatement * aStatement )
{
    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseInsertValuesInternal( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::parseInsertValuesInternal(qcStatement * aStatement)
{
    qmmInsParseTree * sParseTree;
    qmsTableRef     * sTableRef;
    qmsTableRef     * sInsertTableRef = NULL;
    qcmTableInfo    * sInsertTableInfo;
    qcmColumn       * sColumn;
    qcmColumn       * sColumnInfo;
    qmmValueNode    * sValue;
    qmmValueNode    * sFirstValue;
    qmmValueNode    * sPrevValue;
    qmmValueNode    * sCurrValue;
    qmmMultiRows    * sMultiRows;
    UChar             sSeqName[QC_MAX_SEQ_NAME_LEN];
    qcuSqlSourceInfo  sqlInfo;
    qmsFrom           sFrom;
    idBool            sTriggerExist = ID_FALSE;
    UInt              sFlag = 0;
    qmsParseTree    * sViewParseTree = NULL;
    qcmColumn       * sPrevColumn;
    mtcTemplate     * sMtcTemplate;
    qmsTarget       * sTarget;
    qcmColumn       * sInsertColumn;
    mtcTuple        * sMtcTuple;
    mtcColumn       * sMtcColumn;
    UShort            sInsertTupleId = ID_USHORT_MAX;
    idBool            sFound;
    qcmViewReadOnly   sReadOnly = QCM_VIEW_NON_READ_ONLY;

    /* BUG-46124 */
    qmsTarget       * sReturnTarget;
    qmsSFWGH        * sViewSFWGH;

    IDU_FIT_POINT_FATAL( "qmv::parseInsertValues::__FT__" );

    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;
    sTableRef  = sParseTree->tableRef;

    /* PROJ-2204 Join Update, Delete */
    QCP_SET_INIT_QMS_FROM( (&sFrom) );
    sFrom.tableRef = sTableRef;

    IDE_TEST( parseViewInFromClause( aStatement,
                                     &sFrom,
                                     sParseTree->hints )
              != IDE_SUCCESS );

    // check existence of table and get table META Info.
    sFlag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sFlag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sFlag &= ~(QMV_VIEW_CREATION_MASK);
    sFlag |= (QMV_VIEW_CREATION_FALSE);

    // PROJ-2204 join update, delete
    // updatable view에 사용되는 SFWGH임을 표시한다.
    if ( sTableRef->view != NULL )
    {
        sViewParseTree = (qmsParseTree*) sTableRef->view->myPlan->parseTree;

        if ( sViewParseTree->querySet->SFWGH != NULL )
        {
            sViewParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
            sViewParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
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

    IDE_TEST(qmvQuerySet::validateQmsTableRef( aStatement,
                                               NULL,
                                               sTableRef,
                                               sFlag,
                                               MTC_COLUMN_NOTNULL_TRUE )
             != IDE_SUCCESS); // PR-13597

    /******************************
     * PROJ-2204 Join Update, Delete
     ******************************/

    /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
    IDE_TEST( checkInsteadOfTrigger( sTableRef,
                                     QCM_TRIGGER_EVENT_INSERT,
                                     &sTriggerExist ) );

    /* PROJ-1888 INSTEAD OF TRIGGER */
    if ( sTriggerExist == ID_TRUE )
    {
        sParseTree->insteadOfTrigger = ID_TRUE;

        sParseTree->insertTableRef = sTableRef;
        sParseTree->insertColumns  = sParseTree->columns;
    }
    else
    {
        sParseTree->insteadOfTrigger = ID_FALSE;

        sPrevColumn = NULL;

        sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

        // created view, inline view , not QCM_PERFORMANCE_VIEW
        if (( ( sMtcTemplate->rows[sTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_TRUE ) &&
            ( sParseTree->tableRef->tableType != QCM_PERFORMANCE_VIEW ))
        {
            sViewSFWGH = sViewParseTree->querySet->SFWGH; /* BUG-46124 */

            /* BUG-49136 */
            IDE_TEST_RAISE( sViewSFWGH == NULL, ERR_NOT_KEY_PRESERVED_TABLE );

            if (sParseTree->columns != NULL)
            {
                /* insert column list 있는 경우 insertColumns 생성 */
                for (sColumn = sParseTree->columns;
                     sColumn != NULL;
                     sColumn = sColumn->next )
                {
                    sFound = ID_FALSE;

                    for ( sTarget = sViewParseTree->querySet->target;
                          sTarget != NULL;
                          sTarget = sTarget->next )
                    {
                        if ( sTarget->aliasColumnName.size != QC_POS_EMPTY_SIZE )
                        {
                            if ( idlOS::strMatch( sTarget->aliasColumnName.name,
                                                  sTarget->aliasColumnName.size,
                                                  sColumn->namePos.stmtText +
                                                  sColumn->namePos.offset,
                                                  sColumn->namePos.size ) == 0 )
                            {
                                // 이미 한번 찾았다.
                                IDE_TEST_RAISE( sFound == ID_TRUE, ERR_DUP_ALIAS_NAME );
                                sFound = ID_TRUE;

                                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                                        qcmColumn,
                                                        &sInsertColumn)
                                          != IDE_SUCCESS );

                                QCM_COLUMN_INIT( sInsertColumn );

                                /* BUG-46124 */
                                IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                               sViewSFWGH,
                                                                                               sViewSFWGH->from,
                                                                                               sTarget,
                                                                                               & sReturnTarget )
                                          != IDE_SUCCESS );

                                sInsertColumn->namePos.stmtText = sReturnTarget->columnName.name;
                                sInsertColumn->namePos.offset = 0;
                                sInsertColumn->namePos.size = sReturnTarget->columnName.size;
                                sInsertColumn->next = NULL;

                                if( sPrevColumn == NULL )
                                {
                                    sParseTree->insertColumns = sInsertColumn;
                                    sPrevColumn               = sInsertColumn;
                                }
                                else
                                {
                                    sPrevColumn->next = sInsertColumn;
                                    sPrevColumn       = sInsertColumn;
                                }

                                // key-preserved column 검사
                                sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                                sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                                IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                                  != MTC_TUPLE_VIEW_FALSE ) ||
                                                ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                                  != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                                ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                                  != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                                ERR_NOT_KEY_PRESERVED_TABLE );

                                if( sInsertTupleId == ID_USHORT_MAX )
                                {
                                    // first
                                    sInsertTupleId = sReturnTarget->targetColumn->node.baseTable;
                                }
                                else
                                {
                                    // insert할 태이블이 여러개임
                                    IDE_TEST_RAISE( sInsertTupleId != sReturnTarget->targetColumn->node.baseTable,
                                                    ERR_DUP_BASE_TABLE );
                                }

                                sInsertTableRef =
                                    QC_SHARED_TMPLATE(aStatement)->tableMap[sInsertTupleId].from->tableRef;
                            }
                            else
                            {
                                /* Nothing To Do */
                            }
                        }
                        else
                        {
                            /* aliasColumnName이 empty인 경우 어차피 column이 아닌 경우이므로
                             * insert 되지 않는다.
                             *
                             * SELECT C1+1 FROM T1
                             *
                             * tableName       = NULL
                             * aliasTableName  = NULL
                             * columnName      = NULL
                             * aliasColumnName = NULL
                             * displayName     = C1+1
                             */
                        }
                    }

                    if ( sFound == ID_FALSE )
                    {
                        sqlInfo.setSourceInfo(aStatement,
                                              &sColumn->namePos);
                        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                /* column list 없는 경우 insert되는 테이블의 columns 만큼
                 * insertColumns 생성 */
                for ( sTarget = sViewParseTree->querySet->target;
                      sTarget != NULL;
                      sTarget = sTarget->next )
                {
                    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                            qcmColumn,
                                            &sInsertColumn)
                              != IDE_SUCCESS );

                    QCM_COLUMN_INIT( sInsertColumn );

                    /* BUG-46124 */
                    IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                   sViewSFWGH,
                                                                                   sViewSFWGH->from,
                                                                                   sTarget,
                                                                                   & sReturnTarget )
                              != IDE_SUCCESS );

                    sInsertColumn->namePos.stmtText = sReturnTarget->columnName.name;
                    sInsertColumn->namePos.offset = 0;
                    sInsertColumn->namePos.size = sReturnTarget->columnName.size;
                    sInsertColumn->next = NULL;

                    if( sPrevColumn == NULL )
                    {
                        sParseTree->insertColumns = sInsertColumn;
                        sPrevColumn               = sInsertColumn;
                    }
                    else
                    {
                        sPrevColumn->next = sInsertColumn;
                        sPrevColumn       = sInsertColumn;
                    }

                    // key-preserved column 검사
                    sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                    sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                    IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                      != MTC_TUPLE_VIEW_FALSE ) ||
                                    ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                      != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                    ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                      != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                    ERR_NOT_KEY_PRESERVED_TABLE );

                    if( sInsertTupleId == ID_USHORT_MAX )
                    {
                        // first
                        sInsertTupleId = sReturnTarget->targetColumn->node.baseTable;
                    }
                    else
                    {
                        // insert할 태이블이 여러개임
                        IDE_TEST_RAISE( sInsertTupleId != sReturnTarget->targetColumn->node.baseTable,
                                        ERR_NOT_ENOUGH_INSERT_VALUES );
                    }

                    sInsertTableRef =
                        QC_SHARED_TMPLATE(aStatement)->tableMap[sInsertTupleId].from->tableRef;
                }
            }

            /* BUG-36350 Updatable Join DML WITH READ ONLY*/
            if ( sParseTree->tableRef->tableInfo->tableID != 0 )
            {
                /* view read only */
                IDE_TEST( qcmView::getReadOnlyOfViews(
                              QC_SMI_STMT( aStatement ),
                              sParseTree->tableRef->tableInfo->tableID,
                              &sReadOnly )
                          != IDE_SUCCESS);
            }
            else
            {
                /* Nothing To Do */
            }

            /* read only insert error */
            IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY,
                            ERR_NOT_DNL_READ_ONLY_VIEW );

            sParseTree->insertTableRef = sInsertTableRef;
        }
        else
        {
            sParseTree->insertTableRef = sTableRef;
            sParseTree->insertColumns  = sParseTree->columns;
        }
    }

    sInsertTableRef  = sParseTree->insertTableRef;
    sInsertTableInfo = sInsertTableRef->tableInfo;

    // BUG-17409
    sInsertTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
    sInsertTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

    IDE_TEST( insertCommon( aStatement,
                            sInsertTableRef,
                            &(sParseTree->checkConstrList),      /* PROJ-1107 Check Constraint 지원 */
                            &(sParseTree->defaultExprColumns),   /* PROJ-1090 Function-based Index */
                            &(sParseTree->defaultTableRef) )
              != IDE_SUCCESS );

    if (sInsertTableInfo->tableType == QCM_QUEUE_TABLE)
    {
        // Enqueue 문이 아닌 경우, queue table에 대한 Insert문은 오류.
        IDE_TEST_RAISE(
            (sParseTree->flag & QMM_QUEUE_MASK) != QMM_QUEUE_TRUE,
            ERR_DML_ON_QUEUE);
    }

    if ( (sParseTree->flag & QMM_QUEUE_MASK) == QMM_QUEUE_TRUE )
    {
        // msgid를 위한 sequence의 table 정보를 얻어온다.
        IDE_TEST_RAISE(
            sInsertTableInfo->tableType != QCM_QUEUE_TABLE,
            ERR_ENQUEUE_ON_TABLE);
        idlOS::snprintf( (SChar*)sSeqName, QC_MAX_SEQ_NAME_LEN,
                         "%s_NEXT_MSG_ID",
                         sInsertTableRef->tableInfo->name);

        IDE_TEST(qcm::getSequenceHandleByName(
                     QC_SMI_STMT( aStatement ),
                     sInsertTableRef->userID,
                     (sSeqName),
                     idlOS::strlen((SChar*)sSeqName),
                     &(sParseTree->queueMsgIDSeq))
                 != IDE_SUCCESS);
    }

    // BUG-46174
    if ( sParseTree->spVariable != NULL )
    {
        IDE_TEST( parseSPVariableValue( aStatement,
                                        sParseTree->spVariable,
                                        &(sParseTree->rows->values) )
                  != IDE_SUCCESS);
    }

    // The possible cases
    //             number of column   number of value
    //  - case 1 :        m                  n        (m = n and n > 0)
    //          => sTableInfo->columns 순으로 column과 values를 구성
    //          => 명시되지 않은 column의 value는 NULL 이거나 DEFAULT value
    //  - case 2 :        m                  n        (m = 0 and n > 0)
    //          => sTableInfo->columns를 가지고 옴.
    //  - case 3 :        m                  n        (m < n and n > 0)
    //          => too many values error
    //  - case 4 :        m                  n        (m > n and n > 0)
    //          => not enough values error
    // The other cases cannot pass parser.

    if (sParseTree->insertColumns != NULL)
    {
        for ( sMultiRows = sParseTree->rows;
              sMultiRows != NULL;
              sMultiRows = sMultiRows->next )
        {
            for ( sColumn = sParseTree->insertColumns, sCurrValue = sMultiRows->values;
                  sColumn != NULL;
                  sColumn = sColumn->next, sCurrValue = sCurrValue->next )
            {
                IDE_TEST_RAISE(sCurrValue == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);

                // The Parser checked duplicated name in column list.
                // check column existence
                if ( QC_IS_NULL_NAME(sColumn->tableNamePos) != ID_TRUE )
                {
                    if ( idlOS::strMatch( sTableRef->aliasName.stmtText + sTableRef->aliasName.offset,
                                          sTableRef->aliasName.size,
                                          sColumn->tableNamePos.stmtText + sColumn->tableNamePos.offset,
                                          sColumn->tableNamePos.size ) != 0 )
                    {
                        sqlInfo.setSourceInfo(aStatement,
                                              &sColumn->namePos);
                        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                    }
                }

                IDE_TEST(qcmCache::getColumn(aStatement,
                                             sInsertTableInfo,
                                             sColumn->namePos,
                                             &sColumnInfo) != IDE_SUCCESS);
                QMV_SET_QCM_COLUMN( sColumn, sColumnInfo );

                /* PROJ-1090 Function-based Index */
                if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                     == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &(sColumn->namePos) );
                    IDE_RAISE( ERR_CANNOT_USE_HIDDEN_COLUMN );
                }
                else
                {
                    /* Nothing to do */
                }

                //--------- validation of inserting value  ---------//
                if (sCurrValue->value == NULL)
                {
                    // insert into t1 values ( DEFAULT )
                    IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumnInfo, sCurrValue)
                             != IDE_SUCCESS);

                    if ( ( sColumnInfo->basicInfo->flag
                           & MTC_COLUMN_TIMESTAMP_MASK )
                         == MTC_COLUMN_TIMESTAMP_TRUE )
                    {
                        sCurrValue->timestamp = ID_TRUE;
                    }
                }
                else
                {
                    IDE_TEST(parseViewInExpression( aStatement, sCurrValue->value )
                             != IDE_SUCCESS);
                }
            }

            IDE_TEST_RAISE(sCurrValue != NULL, ERR_TOO_MANY_INSERT_VALUES);
        }

        // The number of sParseTree->columns = The number of sParseTree->values

        for ( sMultiRows = sParseTree->rows;
              sMultiRows != NULL;
              sMultiRows = sMultiRows->next )
        {
            sFirstValue = NULL;
            sPrevValue = NULL;
            for (sColumn = sInsertTableInfo->columns;
                 sColumn != NULL;
                 sColumn = sColumn->next)
            {
                IDE_TEST(getValue(sParseTree->insertColumns,
                                  sMultiRows->values,
                                  sColumn,
                                  &sValue)
                         != IDE_SUCCESS);

                // make current value
                IDE_TEST(STRUCT_ALLOC(
                             QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue)
                         != IDE_SUCCESS);

                sCurrValue->next = NULL;
                sCurrValue->msgID = ID_FALSE;

                if (sValue == NULL)
                {
                    // make current value
                    sCurrValue->value = NULL;

                    IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumn, sCurrValue)
                             != IDE_SUCCESS);

                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                         == MTC_COLUMN_TIMESTAMP_TRUE )
                    {
                        sCurrValue->timestamp = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->timestamp = ID_FALSE;
                    }
                    // Proj-1360 Queue
                    // messageid 칼럼인 경우 해당 칼럼에 대한 value값에 flag설정.
                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_QUEUE_MSGID_MASK )
                         == MTC_COLUMN_QUEUE_MSGID_TRUE )
                    {
                        sCurrValue->msgID = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->msgID = ID_FALSE;
                    }
                }
                else
                {
                    sCurrValue->value     = sValue->value;
                    sCurrValue->timestamp = sValue->timestamp;
                    sCurrValue->msgID = sValue->msgID;
                }

                // make value list
                if (sFirstValue == NULL)
                {
                    sFirstValue = sCurrValue;
                    sPrevValue  = sCurrValue;
                }
                else
                {
                    sPrevValue->next = sCurrValue;
                    sPrevValue       = sCurrValue;
                }
            }
            sMultiRows->values  = sFirstValue;          // full values list
        }

        sParseTree->insertColumns = sInsertTableInfo->columns;  // full columns list
    }
    else
    {
        sParseTree->insertColumns = sInsertTableInfo->columns;

        for ( sMultiRows = sParseTree->rows;
              sMultiRows != NULL;
              sMultiRows = sMultiRows->next )
        {
            sPrevValue = NULL;

            for ( sColumn = sParseTree->insertColumns, sCurrValue = sMultiRows->values;
                  sColumn != NULL;
                  sColumn = sColumn->next, sCurrValue = sCurrValue->next)
            {
                /* PROJ-1090 Function-based Index */
                if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                     == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
                {
                    // make value
                    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmmValueNode, &sValue )
                              != IDE_SUCCESS );

                    sValue->value     = NULL;
                    sValue->validate  = ID_TRUE;
                    sValue->calculate = ID_TRUE;
                    sValue->timestamp = ID_FALSE;
                    sValue->msgID     = ID_FALSE;
                    sValue->expand    = ID_FALSE;

                    // connect value list
                    sValue->next      = sCurrValue;
                    sCurrValue        = sValue;
                    if ( sPrevValue == NULL )
                    {
                        sMultiRows->values = sValue;
                    }
                    else
                    {
                        sPrevValue->next = sValue;
                    }
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST_RAISE(sCurrValue == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);

                //--------- validation of inserting value  ---------//
                if (sCurrValue->value == NULL)
                {
                    IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumn, sCurrValue)
                             != IDE_SUCCESS);

                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                         == MTC_COLUMN_TIMESTAMP_TRUE )
                    {
                        sCurrValue->timestamp = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->timestamp = ID_FALSE;
                    }

                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_QUEUE_MSGID_MASK )
                         == MTC_COLUMN_QUEUE_MSGID_TRUE )
                    {
                        sCurrValue->msgID = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->msgID = ID_FALSE;
                    }
                }
                else
                {
                    IDE_TEST(parseViewInExpression( aStatement, sCurrValue->value )
                             != IDE_SUCCESS);
                }

                sPrevValue = sCurrValue;
            }
            IDE_TEST_RAISE(sCurrValue != NULL, ERR_TOO_MANY_INSERT_VALUES);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_USE_HIDDEN_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ENOUGH_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TOO_MANY_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_DML_ON_QUEUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_QUEUE_DML_DENIED));
    }
    IDE_EXCEPTION(ERR_ENQUEUE_ON_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_TABLE_ENQUEUE_DENIED));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION( ERR_DUP_ALIAS_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME));
    }
    IDE_EXCEPTION( ERR_DUP_BASE_TABLE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ONE_BASE_TABLE));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateInsertValues(qcStatement * aStatement)
{
    qmmInsParseTree     * sParseTree;
    qmsQuerySet         * sOuterQuerySet;
    qmsSFWGH            * sOuterSFWGH;
    qmsTableRef         * sInsertTableRef;
    qcmTableInfo        * sInsertTableInfo;
    qcmColumn           * sCurrColumn;
    qmmValueNode        * sCurrValue;
    qmmMultiRows        * sMultiRows;
    qcuSqlSourceInfo      sqlInfo;
    const mtdModule    ** sModules;
    qcmColumn           * sQcmColumn;
    mtcColumn           * sMtcColumn;
    qmsFrom               sFrom;
    qdConstraintSpec    * sConstr;

    IDU_FIT_POINT_FATAL( "qmv::validateInsertValues::__FT__" );

    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;

    sInsertTableRef  = sParseTree->insertTableRef;
    sInsertTableInfo = sInsertTableRef->tableInfo;

    /* PROJ-1988 MERGE statement
     * insert values에서 merge target column이 outer column으로 참조된다. */
    sOuterQuerySet = sParseTree->outerQuerySet;
    if ( sOuterQuerySet != NULL )
    {
        sOuterSFWGH = sOuterQuerySet->SFWGH;
    }
    else
    {
        sOuterSFWGH = NULL;
    }

    /* TASK-7307 DML Data Consistency in Shard */
    IDE_TEST( checkUsableTable( aStatement,
                                sInsertTableInfo ) != IDE_SUCCESS );

    // PR-13725
    // CHECK OPERATABLE
    IDE_TEST( checkInsertOperatable( aStatement,
                                     sParseTree,
                                     sInsertTableInfo )
              != IDE_SUCCESS );

    /* PROJ-2211 Materialized View */
    if ( sInsertTableInfo->tableType == QCM_MVIEW_TABLE )
    {
        IDE_TEST_RAISE( sParseTree->hints == NULL,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );

        IDE_TEST_RAISE( sParseTree->hints->refreshMView != ID_TRUE,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(makeInsertRow( aStatement, sParseTree ) != IDE_SUCCESS);

    for ( sMultiRows = sParseTree->rows;
          sMultiRows != NULL;
          sMultiRows = sMultiRows->next )
    {
        sModules = sParseTree->columnModules;

        for( sCurrColumn = sParseTree->insertColumns, sCurrValue = sMultiRows->values;
             sCurrValue != NULL;
             sCurrColumn = sCurrColumn->next, sCurrValue = sCurrValue->next, sModules++ )
        {
            if( sCurrValue->value != NULL )
            {
                /* PROJ-2197 PSM Renewal */
                sCurrValue->value->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

                if ( qtc::estimate(sCurrValue->value,
                                   QC_SHARED_TMPLATE(aStatement),
                                   aStatement,
                                   sOuterQuerySet,
                                   sOuterSFWGH,
                                   NULL )
                     != IDE_SUCCESS )
                {
                    // default value인 경우 별도 에러처리
                    IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                    == QTC_NODE_DEFAULT_VALUE_TRUE,
                                    ERR_INVALID_DEFAULT_VALUE );

                    // default value가 아닌 경우 에러pass
                    IDE_RAISE( ERR_ESTIMATE );
                }
                else
                {
                    // Nothing to do.
                }

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                // PROJ-1988 Merge Query
                // 아래와 같은 쿼리에서 t1.i1과 t2.i1이 encrypted column이라면
                // policy가 다를 수 있으므로 t2.i1에 decrypt func을 생성한다.
                //
                // merge into t1 using t2 on (t1.i1 = t2.i1)
                // when not matched then
                // insert (t1.i1, t1.i2) values (t2.i1, t2.i2);
                //
                // BUG-32303
                // insert into t1(i1) values (echar'abc');
                sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );

                if ( (sMtcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                     == MTD_ENCRYPT_TYPE_TRUE )
                {
                    // add decrypt func
                    IDE_TEST( addDecryptFunc4ValueNode( aStatement,
                                                        sCurrValue )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                   aStatement,
                                                   QC_SHARED_TMPLATE(aStatement),
                                                   *sModules )
                          != IDE_SUCCESS );

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                // BUG-15746
                IDE_TEST( describeParamInfo( aStatement,
                                             sCurrColumn->basicInfo,
                                             sCurrValue->value )
                          != IDE_SUCCESS );

                if( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                    == MTC_NODE_DML_UNUSABLE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &sCurrValue->value->position );
                    IDE_RAISE( ERR_USE_CURSOR_ATTR );
                }
            }
        }
    }
    sParseTree->parentConstraints = NULL;

    // PROJ-1436
    if ( sParseTree->hints != NULL )
    {
        IDE_TEST( validatePlanHints( aStatement,
                                     sParseTree->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1584 DML Return Clause */
    if( sParseTree->returnInto != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sInsertTableRef;

        IDE_TEST( validateReturnInto( aStatement,
                                      sParseTree->returnInto,
                                      NULL,
                                      &sFrom,
                                      ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    /* PROJ-1107 Check Constraint 지원 */
    if ( sParseTree->checkConstrList != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sInsertTableRef;

        for ( sConstr = sParseTree->checkConstrList;
              sConstr != NULL;
              sConstr = sConstr->next )
        {
            IDE_TEST( qdbCommon::validateCheckConstrDefinition(
                          aStatement,
                          sConstr,
                          NULL,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-1090 Function-based Index */
    if ( sParseTree->defaultExprColumns != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sParseTree->defaultTableRef;

        for ( sQcmColumn = sParseTree->defaultExprColumns;
              sQcmColumn != NULL;
              sQcmColumn = sQcmColumn->next)
        {
            IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                          aStatement,
                          sQcmColumn->defaultValue,
                          NULL,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(getParentInfoList(aStatement,
                               sInsertTableInfo,
                               &(sParseTree->parentConstraints))
             != IDE_SUCCESS);

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 처리시 포린키 정보수집
    //---------------------------------------------

    IDE_TEST( setFetchColumnInfo4ParentTable(
                  aStatement,
                  sInsertTableRef ) != IDE_SUCCESS );

    // PROJ-2205 DML trigger에 의한 컬럼참조
    IDE_TEST( setFetchColumnInfo4Trigger(
                  aStatement,
                  sInsertTableRef )
              != IDE_SUCCESS );

    /*
     * BUG-39441
     * need a interface which returns whether DML on replication table or not
     */
    if (sInsertTableInfo->replicationCount > 0)
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_FALSE;
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DML_MVIEW_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GRANT_DML_PRIV_OF_MVIEW_TABLE ) );
    }
    IDE_EXCEPTION(ERR_ESTIMATE);
    {
        // BUG-19871
        if(ideGetErrorCode() == qpERR_ABORT_QMV_NOT_EXISTS_COLUMN)
        {
            sqlInfo.setSourceInfo(aStatement,
                                  &sCurrValue->value->position);
            (void)sqlInfo.init(aStatement->qmeMem);
            IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_COLUMN_NOT_ALLOWED_HERE,
                                    sqlInfo.getErrMessage()));
            (void)sqlInfo.fini();
        }
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_DEFAULT_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::parseInsertAllDefault(qcStatement * aStatement)
{
    qmmInsParseTree * sParseTree;
    qmsTableRef     * sTableRef;
    qmsTableRef     * sInsertTableRef = NULL;
    qcmTableInfo    * sInsertTableInfo;
    qcmColumn       * sColumn;
    qmmValueNode    * sPrevValue = NULL;
    qmmValueNode    * sCurrValue;
    qmmMultiRows    * sMultiRows;
    qmsFrom           sFrom;
    idBool            sTriggerExist = ID_FALSE;
    UInt              sFlag = 0;
    qmsParseTree    * sViewParseTree = NULL;
    mtcTemplate     * sMtcTemplate;
    mtcTuple        * sMtcTuple;
    mtcColumn       * sMtcColumn;
    qmsTarget       * sTarget;
    UShort            sInsertTupleId = ID_USHORT_MAX;
    qcmViewReadOnly   sReadOnly = QCM_VIEW_NON_READ_ONLY;

    /* BUG-46124 */
    qmsTarget       * sReturnTarget;
    qmsSFWGH        * sViewSFWGH;

    IDU_FIT_POINT_FATAL( "qmv::parseInsertAllDefault::__FT__" );

    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;
    sTableRef  = sParseTree->tableRef;

    /* PROJ-2204 Join Update, Delete */
    QCP_SET_INIT_QMS_FROM( (&sFrom) );
    sFrom.tableRef = sTableRef;

    IDE_TEST( parseViewInFromClause( aStatement,
                                     &sFrom,
                                     sParseTree->hints )
              != IDE_SUCCESS );

    // check existence of table and get table META Info.
    sFlag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sFlag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sFlag &= ~(QMV_VIEW_CREATION_MASK);
    sFlag |= (QMV_VIEW_CREATION_FALSE);

    // PROJ-2204 join update, delete
    // updatable view에 사용되는 SFWGH임을 표시한다.
    if ( sTableRef->view != NULL )
    {
        sViewParseTree = (qmsParseTree*) sTableRef->view->myPlan->parseTree;

        if ( sViewParseTree->querySet->SFWGH != NULL )
        {
            sViewParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
            sViewParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
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

    IDE_TEST(qmvQuerySet::validateQmsTableRef( aStatement,
                                               NULL,
                                               sTableRef,
                                               sFlag,
                                               MTC_COLUMN_NOTNULL_TRUE )
             != IDE_SUCCESS); // PR-13597

    /******************************
     * PROJ-2204 Join Update, Delete
     ******************************/

    /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
    IDE_TEST( checkInsteadOfTrigger( sTableRef,
                                     QCM_TRIGGER_EVENT_INSERT,
                                     &sTriggerExist ) );

    // insert default values에는 column을 명시할 수 없다.
    IDE_DASSERT( sParseTree->columns == NULL );

    /* PROJ-1888 INSTEAD OF TRIGGER */
    if ( sTriggerExist == ID_TRUE )
    {
        sParseTree->insteadOfTrigger = ID_TRUE;

        sParseTree->insertTableRef = sTableRef;
        sParseTree->insertColumns  = NULL;
    }
    else
    {
        sParseTree->insteadOfTrigger = ID_FALSE;

        sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

        // created view, inline view
        if( ( ( sMtcTemplate->rows[sTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_TRUE ) &&
            ( sParseTree->tableRef->tableType != QCM_PERFORMANCE_VIEW ) )
        {
            sViewSFWGH = sViewParseTree->querySet->SFWGH; /* BUG-46124 */

            /* BUG-49136 */
            IDE_TEST_RAISE( sViewSFWGH == NULL, ERR_NOT_KEY_PRESERVED_TABLE );

            /* column list 없는 경우 insert되는 테이블의 columns 만큼
             * insertColumns 생성 */
            for ( sTarget = sViewParseTree->querySet->target;
                  sTarget != NULL;
                  sTarget = sTarget->next )
            {
                /* BUG-46124 */
                IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                               sViewSFWGH,
                                                                               sViewSFWGH->from,
                                                                               sTarget,
                                                                               & sReturnTarget )
                          != IDE_SUCCESS );

                // key-preserved column 검사
                sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                  != MTC_TUPLE_VIEW_FALSE ) ||
                                ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                  != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                  != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                ERR_NOT_KEY_PRESERVED_TABLE );

                if( sInsertTupleId == ID_USHORT_MAX )
                {
                    // first
                    sInsertTupleId = sReturnTarget->targetColumn->node.baseTable;
                }
                else
                {
                    // insert할 태이블이 여러개임
                    IDE_TEST_RAISE( sInsertTupleId != sReturnTarget->targetColumn->node.baseTable,
                                    ERR_NOT_ONE_BASE_TABLE );
                }

                sInsertTableRef =
                    QC_SHARED_TMPLATE(aStatement)->tableMap[sInsertTupleId].from->tableRef;
            }

            /* BUG-36350 Updatable Join DML WITH READ ONLY*/
            if( sParseTree->tableRef->tableInfo->tableID != 0 )
            {
                /* view read only */
                IDE_TEST( qcmView::getReadOnlyOfViews(
                              QC_SMI_STMT( aStatement ),
                              sParseTree->tableRef->tableInfo->tableID,
                              &sReadOnly )
                          != IDE_SUCCESS);
            }
            else
            {
                /* Nothing To Do */
            }

            /* read only insert error */
            IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY,
                            ERR_NOT_DNL_READ_ONLY_VIEW );


            sParseTree->insertTableRef = sInsertTableRef;
            sParseTree->insertColumns  = NULL;
        }
        else
        {
            sParseTree->insertTableRef = sTableRef;
            sParseTree->insertColumns  = NULL;
        }
    }

    sInsertTableRef  = sParseTree->insertTableRef;
    sInsertTableInfo = sInsertTableRef->tableInfo;

    IDE_TEST( insertCommon( aStatement,
                            sInsertTableRef,
                            &(sParseTree->checkConstrList),      /* PROJ-1107 Check Constraint 지원 */
                            &(sParseTree->defaultExprColumns),   /* PROJ-1090 Function-based Index */
                            &(sParseTree->defaultTableRef) )
              != IDE_SUCCESS );

    // Enqueue 문이 아닌 경우, queue table에 대한 Insert문은 오류.
    IDE_TEST_RAISE(sInsertTableInfo->tableType ==
                   QCM_QUEUE_TABLE,
                   ERR_DML_ON_QUEUE);

    sParseTree->insertColumns = sInsertTableRef->tableInfo->columns;

    // make current rows value
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmMultiRows, &sMultiRows )
             != IDE_SUCCESS);
    sMultiRows->values       = NULL;
    sMultiRows->next         = NULL;
    sParseTree->rows         = sMultiRows;

    for (sColumn = sParseTree->insertColumns;
         sColumn != NULL;
         sColumn = sColumn->next)
    {
        // make current value
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue)
                 != IDE_SUCCESS);

        sCurrValue->value     = NULL;
        sCurrValue->timestamp = ID_FALSE;
        sCurrValue->msgID     = ID_FALSE;
        sCurrValue->next      = NULL;

        IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumn, sCurrValue)
                 != IDE_SUCCESS);

        if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
             == MTC_COLUMN_TIMESTAMP_TRUE )
        {
            sCurrValue->timestamp = ID_TRUE;
        }
        if ( ( sColumn->basicInfo->flag & MTC_COLUMN_QUEUE_MSGID_MASK )
             == MTC_COLUMN_QUEUE_MSGID_TRUE )
        {
            sCurrValue->msgID = ID_TRUE;
        }

        // connect
        if (sPrevValue == NULL)
        {
            sParseTree->rows->values = sCurrValue;
            sPrevValue               = sCurrValue;
        }
        else
        {
            sPrevValue->next = sCurrValue;
            sPrevValue       = sCurrValue;
        }
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DML_ON_QUEUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_QUEUE_DML_DENIED));
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION(ERR_NOT_ONE_BASE_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ONE_BASE_TABLE));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateInsertAllDefault(qcStatement * aStatement)
{
    qmmInsParseTree     * sParseTree;
    qmsTableRef         * sInsertTableRef;
    qcmTableInfo        * sInsertTableInfo;
    qmmValueNode        * sCurrValue;
    qcuSqlSourceInfo      sqlInfo;
    const mtdModule    ** sModules;
    qmsFrom               sFrom;
    qdConstraintSpec    * sConstr;
    qcmColumn           * sQcmColumn;

    IDU_FIT_POINT_FATAL( "qmv::validateInsertAllDefault::__FT__" );

    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;

    sInsertTableRef  = sParseTree->insertTableRef;
    sInsertTableInfo = sInsertTableRef->tableInfo;

    /* TASK-7307 DML Data Consistency in Shard */
    IDE_TEST( checkUsableTable( aStatement,
                                sInsertTableInfo ) != IDE_SUCCESS );

    // PR-13725
    // CHECK OPERATABLE
    IDE_TEST( checkInsertOperatable( aStatement,
                                     sParseTree,
                                     sInsertTableInfo )
              != IDE_SUCCESS );

    /* PROJ-2211 Materialized View */
    if ( sInsertTableInfo->tableType == QCM_MVIEW_TABLE )
    {
        IDE_TEST_RAISE( sParseTree->hints == NULL,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );

        IDE_TEST_RAISE( sParseTree->hints->refreshMView != ID_TRUE,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(makeInsertRow( aStatement, sParseTree ) != IDE_SUCCESS);

    sModules = sParseTree->columnModules;

    for (sCurrValue = sParseTree->rows->values;
         sCurrValue != NULL;
         sCurrValue = sCurrValue->next, sModules++ )
    {
        if (sCurrValue->value != NULL)
        {
            if ( qtc::estimate( sCurrValue->value,
                                QC_SHARED_TMPLATE(aStatement),
                                aStatement,
                                NULL,
                                NULL,
                                NULL )
                 != IDE_SUCCESS )
            {
                // default value인 경우 별도 에러처리
                IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                == QTC_NODE_DEFAULT_VALUE_TRUE,
                                ERR_INVALID_DEFAULT_VALUE );

                // default value가 아닌 경우 에러pass
                IDE_TEST( 1 );
            }
            else
            {
                // Nothing to do.
            }

            // PROJ-1502 PARTITIONED DISK TABLE
            IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                              QC_SHARED_TMPLATE(aStatement),
                                              aStatement )
                      != IDE_SUCCESS );

            IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                               aStatement,
                                               QC_SHARED_TMPLATE(aStatement),
                                               *sModules )
                      != IDE_SUCCESS );

            // PROJ-1502 PARTITIONED DISK TABLE
            IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                              QC_SHARED_TMPLATE(aStatement),
                                              aStatement )
                      != IDE_SUCCESS );

            if ( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrValue->value->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }
        }
    }

    // PROJ-1436
    if ( sParseTree->hints != NULL )
    {
        IDE_TEST( validatePlanHints( aStatement,
                                     sParseTree->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1584 DML Return Clause */
    if( sParseTree->returnInto != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sInsertTableRef;

        IDE_TEST( validateReturnInto( aStatement,
                                      sParseTree->returnInto,
                                      NULL,
                                      &sFrom,
                                      ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    /* PROJ-1107 Check Constraint 지원 */
    if ( sParseTree->checkConstrList != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sInsertTableRef;

        for ( sConstr = sParseTree->checkConstrList;
              sConstr != NULL;
              sConstr = sConstr->next )
        {
            IDE_TEST( qdbCommon::validateCheckConstrDefinition(
                          aStatement,
                          sConstr,
                          NULL,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-1090 Function-based Index */
    if ( sParseTree->defaultExprColumns != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sParseTree->defaultTableRef;

        for ( sQcmColumn = sParseTree->defaultExprColumns;
              sQcmColumn != NULL;
              sQcmColumn = sQcmColumn->next)
        {
            IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                          aStatement,
                          sQcmColumn->defaultValue,
                          NULL,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(getParentInfoList(aStatement,
                               sInsertTableInfo,
                               &(sParseTree->parentConstraints))
             != IDE_SUCCESS);

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 처리시 포린키 정보수집
    //---------------------------------------------

    IDE_TEST( setFetchColumnInfo4ParentTable(
                  aStatement,
                  sInsertTableRef )
              != IDE_SUCCESS );

    // PROJ-2205 DML trigger에 의한 컬럼참조
    IDE_TEST( setFetchColumnInfo4Trigger(
                  aStatement,
                  sInsertTableRef )
              != IDE_SUCCESS );

    /*
     * BUG-39441
     * need a interface which returns whether DML on replication table or not
     */
    if (sInsertTableInfo->replicationCount > 0)
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_FALSE;
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DML_MVIEW_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GRANT_DML_PRIV_OF_MVIEW_TABLE ) );
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_DEFAULT_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::parseInsertSelect(qcStatement * aStatement)
{
    qmmInsParseTree * sParseTree;
    qmsTableRef     * sTableRef;
    qmsTableRef     * sInsertTableRef = NULL;
    qcmTableInfo    * sInsertTableInfo;
    qcmColumn       * sColumn;
    qcmColumn       * sColumnInfo;
    qcmColumn       * sColumnParse;
    qcmColumn       * sPrevColumn;
    qcmColumn       * sCurrColumn;
    qcmColumn       * sHiddenColumn;
    qcmColumn       * sPrevHiddenColumn;
    qmmValueNode    * sPrevValue;
    qmmValueNode    * sCurrValue;
    qmmMultiRows    * sMultiRows;
    qcuSqlSourceInfo  sqlInfo;
    qmsFrom           sFrom;
    idBool            sTriggerExist = ID_FALSE;
    UInt              sFlag = 0;
    qmsParseTree    * sViewParseTree = NULL;
    mtcTemplate     * sMtcTemplate;
    mtcTuple        * sMtcTuple;
    mtcColumn       * sMtcColumn;
    qcmColumn       * sInsertColumn;
    qmsTarget       * sTarget;
    idBool            sFound;
    UShort            sInsertTupleId = ID_USHORT_MAX;
    qcmViewReadOnly   sReadOnly = QCM_VIEW_NON_READ_ONLY;

    /* BUG-46124 */
    qmsTarget       * sReturnTarget;
    qmsSFWGH        * sViewSFWGH;

    IDU_FIT_POINT_FATAL( "qmv::parseInsertSelect::__FT__" );

    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;
    sTableRef  = sParseTree->tableRef;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseSelectInternal( sParseTree->select )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                             sParseTree->select->mFlag )
                      != IDE_SUCCESS );

    /* PROJ-2204 Join Update, Delete */
    QCP_SET_INIT_QMS_FROM( (&sFrom) );
    sFrom.tableRef = sTableRef;

    IDE_TEST( parseViewInFromClause( aStatement,
                                     &sFrom,
                                     sParseTree->hints )
              != IDE_SUCCESS );

    // check existence of table and get table META Info.
    sFlag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sFlag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sFlag &= ~(QMV_VIEW_CREATION_MASK);
    sFlag |= (QMV_VIEW_CREATION_FALSE);

    // PROJ-2204 join update, delete
    // updatable view에 사용되는 SFWGH임을 표시한다.
    if ( sTableRef->view != NULL )
    {
        sViewParseTree = (qmsParseTree*) sTableRef->view->myPlan->parseTree;

        if ( sViewParseTree->querySet->SFWGH != NULL )
        {
            sViewParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
            sViewParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
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

    IDE_TEST(qmvQuerySet::validateQmsTableRef( aStatement,
                                               NULL,
                                               sTableRef,
                                               sFlag,
                                               MTC_COLUMN_NOTNULL_TRUE )
             != IDE_SUCCESS); // PR-13597

    /******************************
     * PROJ-2204 Join Update, Delete
     ******************************/

    /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
    IDE_TEST( checkInsteadOfTrigger( sTableRef,
                                     QCM_TRIGGER_EVENT_INSERT,
                                     &sTriggerExist ) );

    /* PROJ-1888 INSTEAD OF TRIGGER */
    if ( sTriggerExist == ID_TRUE )
    {
        sParseTree->insteadOfTrigger = ID_TRUE;

        sParseTree->insertTableRef = sTableRef;
        sParseTree->insertColumns  = sParseTree->columns;
    }
    else
    {
        sParseTree->insteadOfTrigger = ID_FALSE;

        sPrevColumn = NULL;

        sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

        // created view, inline view
        if (( ( sMtcTemplate->rows[sTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_TRUE ) &&
            ( sParseTree->tableRef->tableType != QCM_PERFORMANCE_VIEW ))
        {
            sViewSFWGH = sViewParseTree->querySet->SFWGH; /* BUG-46124 */

            /* BUG-49136 */
            IDE_TEST_RAISE( sViewSFWGH == NULL, ERR_NOT_KEY_PRESERVED_TABLE );

            if (sParseTree->columns != NULL)
            {
                /* insert column list 있는 경우 insertColumns 생성 */
                for (sColumn = sParseTree->columns;
                     sColumn != NULL;
                     sColumn = sColumn->next )
                {
                    sFound = ID_FALSE;

                    for ( sTarget = sViewParseTree->querySet->target;
                          sTarget != NULL;
                          sTarget = sTarget->next )
                    {
                        if ( sTarget->aliasColumnName.size != QC_POS_EMPTY_SIZE )
                        {
                            if ( idlOS::strMatch( sTarget->aliasColumnName.name,
                                                  sTarget->aliasColumnName.size,
                                                  sColumn->namePos.stmtText +
                                                  sColumn->namePos.offset,
                                                  sColumn->namePos.size ) == 0 )
                            {
                                // 이미 한번 찾았다.
                                IDE_TEST_RAISE( sFound == ID_TRUE, ERR_DUP_ALIAS_NAME );
                                sFound = ID_TRUE;

                                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                                        qcmColumn,
                                                        &sInsertColumn)
                                          != IDE_SUCCESS );

                                QCM_COLUMN_INIT( sInsertColumn );

                                /* BUG-46124 */
                                IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                               sViewSFWGH,
                                                                                               sViewSFWGH->from,
                                                                                               sTarget,
                                                                                               & sReturnTarget )
                                          != IDE_SUCCESS );

                                sInsertColumn->namePos.stmtText = sReturnTarget->columnName.name;
                                sInsertColumn->namePos.offset = 0;
                                sInsertColumn->namePos.size = sReturnTarget->columnName.size;
                                sInsertColumn->next = NULL;

                                if( sPrevColumn == NULL )
                                {
                                    sParseTree->insertColumns = sInsertColumn;
                                    sPrevColumn               = sInsertColumn;
                                }
                                else
                                {
                                    sPrevColumn->next = sInsertColumn;
                                    sPrevColumn       = sInsertColumn;
                                }

                                // key-preserved column 검사
                                sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                                sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                                IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                                  != MTC_TUPLE_VIEW_FALSE ) ||
                                                ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                                  != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                                ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                                  != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                                ERR_NOT_KEY_PRESERVED_TABLE );

                                if( sInsertTupleId == ID_USHORT_MAX )
                                {
                                    // first
                                    sInsertTupleId = sReturnTarget->targetColumn->node.baseTable;
                                }
                                else
                                {
                                    // insert할 태이블이 여러개임
                                    IDE_TEST_RAISE( sInsertTupleId != sReturnTarget->targetColumn->node.baseTable,
                                                    ERR_NOT_ONE_BASE_TABLE );
                                }

                                sInsertTableRef =
                                    QC_SHARED_TMPLATE(aStatement)->tableMap[sInsertTupleId].from->tableRef;
                            }
                            else
                            {
                                /* Nothing To Do */
                            }
                        }
                        else
                        {
                            /* aliasColumnName이 empty인 경우 어차피 column이 아닌 경우이므로
                             * insert 되지 않는다.
                             *
                             * SELECT C1+1 FROM T1
                             *
                             * tableName       = NULL
                             * aliasTableName  = NULL
                             * columnName      = NULL
                             * aliasColumnName = NULL
                             * displayName     = C1+1
                             */
                        }
                    }

                    if ( sFound == ID_FALSE )
                    {
                        sqlInfo.setSourceInfo(aStatement,
                                              &sColumn->namePos);
                        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                /* column list 없는 경우 insert되는 테이블의 columns 만큼
                 * insertColumns 생성 */
                for ( sTarget = sViewParseTree->querySet->target;
                      sTarget != NULL;
                      sTarget = sTarget->next )
                {
                    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                            qcmColumn,
                                            &sInsertColumn)
                              != IDE_SUCCESS );

                    QCM_COLUMN_INIT( sInsertColumn );
                
                    /* BUG-46124 */
                    IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                   sViewSFWGH,
                                                                                   sViewSFWGH->from,
                                                                                   sTarget,
                                                                                   & sReturnTarget )
                              != IDE_SUCCESS );

                    sInsertColumn->namePos.stmtText = sReturnTarget->columnName.name;
                    sInsertColumn->namePos.offset = 0;
                    sInsertColumn->namePos.size = sReturnTarget->columnName.size;
                    sInsertColumn->next = NULL;

                    if( sPrevColumn == NULL )
                    {
                        sParseTree->insertColumns = sInsertColumn;
                        sPrevColumn               = sInsertColumn;
                    }
                    else
                    {
                        sPrevColumn->next = sInsertColumn;
                        sPrevColumn       = sInsertColumn;
                    }

                    // key-preserved column 검사
                    sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                    sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                    IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                      != MTC_TUPLE_VIEW_FALSE ) ||
                                    ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                      != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                    ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                      != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                    ERR_NOT_KEY_PRESERVED_TABLE );

                    if( sInsertTupleId == ID_USHORT_MAX )
                    {
                        // first
                        sInsertTupleId = sReturnTarget->targetColumn->node.baseTable;
                    }
                    else
                    {
                        // insert할 태이블이 여러개임
                        IDE_TEST_RAISE( sInsertTupleId != sReturnTarget->targetColumn->node.baseTable,
                                        ERR_NOT_ONE_BASE_TABLE );
                    }

                    sInsertTableRef =
                        QC_SHARED_TMPLATE(aStatement)->tableMap[sInsertTupleId].from->tableRef;
                }
            }

            /* BUG-36350 Updatable Join DML WITH READ ONLY*/
            if ( sParseTree->tableRef->tableInfo->tableID != 0 )
            {
                /* view read only */
                IDE_TEST( qcmView::getReadOnlyOfViews(
                              QC_SMI_STMT( aStatement ),
                              sParseTree->tableRef->tableInfo->tableID,
                              &sReadOnly )
                          != IDE_SUCCESS);
            }
            else
            {
                /* Nothing To Do */
            }

            /* read only insert error */
            IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY,
                            ERR_NOT_DNL_READ_ONLY_VIEW );

            sParseTree->insertTableRef = sInsertTableRef;
        }
        else
        {
            sParseTree->insertTableRef = sTableRef;
            sParseTree->insertColumns  = sParseTree->columns;
        }
    }

    sInsertTableRef  = sParseTree->insertTableRef;
    sInsertTableInfo = sInsertTableRef->tableInfo;

    // BUG-17409
    sInsertTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
    sInsertTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

    IDE_TEST( insertCommon( aStatement,
                            sInsertTableRef,
                            &(sParseTree->checkConstrList),      /* PROJ-1107 Check Constraint 지원 */
                            &(sParseTree->defaultExprColumns),   /* PROJ-1090 Function-based Index */
                            &(sParseTree->defaultTableRef) )
              != IDE_SUCCESS );

    // Enqueue 문이 아닌 경우, queue table에 대한 Insert문은 오류.
    IDE_TEST_RAISE(sInsertTableInfo->tableType ==
                   QCM_QUEUE_TABLE,
                   ERR_DML_ON_QUEUE);

    // The possible cases
    //             number of column   number of value
    //  - case 1 :        m                  n        (m = n and n > 0)
    //          => 명시되지 않은 column은 sParseTree->columns 뒤에 덧붙여 연결
    //          => 명시되지 않은 column의 value는 NULL이거나 DEFAULT value로
    //             sParseTree->values에 연결
    //  - case 2 :        m                  n        (m = 0 and n > 0)
    //          => sTableInfo->columns를 가지고 옴.
    //  - case 3 :        m                  n        (m < n and n > 0)
    //          => too many values error
    //  - case 4 :        m                  n        (m > n and n > 0)
    //          => not enough values error
    // The other cases cannot pass parser.

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmMultiRows, &sMultiRows )
             != IDE_SUCCESS);
    sMultiRows->values       = NULL;
    sMultiRows->next         = NULL;
    sParseTree->rows         = sMultiRows;

    if (sParseTree->insertColumns != NULL)
    {
        for (sColumn = sParseTree->insertColumns;
             sColumn != NULL;
             sColumn = sColumn->next)
        {
            // The Parser checked duplicated name in column list.
            // check column existence
            if ( QC_IS_NULL_NAME(sColumn->tableNamePos) != ID_TRUE )
            {
                if ( idlOS::strMatch( sParseTree->tableRef->aliasName.stmtText + sParseTree->tableRef->aliasName.offset,
                                      sParseTree->tableRef->aliasName.size,
                                      sColumn->tableNamePos.stmtText + sColumn->tableNamePos.offset,
                                      sColumn->tableNamePos.size ) != 0 )
                {
                    sqlInfo.setSourceInfo(aStatement,
                                          &sColumn->namePos);
                    IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                }
            }

            IDE_TEST(qcmCache::getColumn(aStatement,
                                         sInsertTableInfo,
                                         sColumn->namePos,
                                         &sColumnInfo) != IDE_SUCCESS);
            QMV_SET_QCM_COLUMN(sColumn, sColumnInfo);

            /* PROJ-1090 Function-based Index */
            if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &(sColumn->namePos) );
                IDE_RAISE( ERR_CANNOT_USE_HIDDEN_COLUMN );
            }
            else
            {
                /* Nothing to do */
            }
        }

        //BUG-40060
        sParseTree->columnsForValues = NULL;

        // The number of sParseTree->columns = The number of sParseTree->values
        sPrevColumn = NULL;
        sPrevValue = NULL;
        for (sColumn = sInsertTableInfo->columns;
             sColumn != NULL;
             sColumn = sColumn->next)
        {
            // search NOT specified column
            for (sColumnParse = sParseTree->insertColumns;
                 sColumnParse != NULL;
                 sColumnParse = sColumnParse->next)
            {
                if (sColumnParse->basicInfo->column.id ==
                    sColumn->basicInfo->column.id)
                {
                    break;
                }
            }

            if (sColumnParse == NULL)
            {
                // make column node for NOT specified column
                IDE_TEST(
                    STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sCurrColumn)
                    != IDE_SUCCESS);

                QMV_SET_QCM_COLUMN(sCurrColumn, sColumn);
                sCurrColumn->next = NULL;

                // connect column list
                if (sPrevColumn == NULL)
                {
                    sParseTree->columnsForValues = sCurrColumn;
                    sPrevColumn                  = sCurrColumn;
                }
                else
                {
                    sPrevColumn->next = sCurrColumn;
                    sPrevColumn       = sCurrColumn;
                }

                // make value node for NOT specified column
                IDE_TEST(
                    STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue)
                    != IDE_SUCCESS);

                sCurrValue->value     = NULL;
                sCurrValue->timestamp = ID_FALSE;
                sCurrValue->msgID     = ID_FALSE;
                sCurrValue->next      = NULL;

                IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumn, sCurrValue)
                         != IDE_SUCCESS);

                if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                     == MTC_COLUMN_TIMESTAMP_TRUE )
                {
                    sCurrValue->timestamp = ID_TRUE;
                }

                if ( ( sColumn->basicInfo->flag & MTC_COLUMN_QUEUE_MSGID_MASK )
                     == MTC_COLUMN_QUEUE_MSGID_TRUE )
                {
                    sCurrValue->msgID = ID_TRUE;
                }

                // make value list
                if (sPrevValue == NULL)
                {
                    sParseTree->rows->values = sCurrValue;
                    sPrevValue               = sCurrValue;
                }
                else
                {
                    sPrevValue->next = sCurrValue;
                    sPrevValue       = sCurrValue;
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
        /* PROJ-1090 Function-based Index
         *  Hidden Column이 있는지 확인한다.
         */
        for ( sHiddenColumn = sInsertTableInfo->columns;
              sHiddenColumn != NULL;
              sHiddenColumn = sHiddenColumn->next )
        {
            if ( (sHiddenColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                break;
            }
            else
            {
                /* Nohting to do */
            }
        }

        /* PROJ-1090 Function-based Index
         *  Hidden Column이 있으면, Hidden Column을 제외한 Column을 모두 지정한다.
         */
        if ( sHiddenColumn != NULL )
        {
            sPrevColumn       = NULL;
            sPrevHiddenColumn = NULL;
            sPrevValue        = NULL;

            for ( sColumn = sInsertTableInfo->columns;
                  sColumn != NULL;
                  sColumn = sColumn->next )
            {
                // make column node for NOT specified column
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qcmColumn, &sCurrColumn )
                          != IDE_SUCCESS );
                QMV_SET_QCM_COLUMN( sCurrColumn, sColumn );
                sCurrColumn->next = NULL;

                // connect column list
                if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                     == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
                {
                    if ( sPrevHiddenColumn == NULL )
                    {
                        sParseTree->columnsForValues = sCurrColumn;
                        sPrevHiddenColumn            = sCurrColumn;
                    }
                    else
                    {
                        sPrevHiddenColumn->next = sCurrColumn;
                        sPrevHiddenColumn       = sCurrColumn;
                    }

                    // make value node for NOT specified column
                    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue )
                              != IDE_SUCCESS );

                    sCurrValue->value     = NULL;
                    sCurrValue->timestamp = ID_FALSE;
                    sCurrValue->msgID     = ID_FALSE;
                    sCurrValue->next      = NULL;

                    IDE_TEST( setDefaultOrNULL( aStatement, sInsertTableInfo, sColumn, sCurrValue )
                              != IDE_SUCCESS );

                    if ( (sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK)
                         == MTC_COLUMN_TIMESTAMP_TRUE )
                    {
                        sCurrValue->timestamp = ID_TRUE;
                    }
                    else
                    {
                        /* Nohting to do */
                    }

                    if ( (sColumn->basicInfo->flag & MTC_COLUMN_QUEUE_MSGID_MASK)
                         == MTC_COLUMN_QUEUE_MSGID_TRUE )
                    {
                        sCurrValue->msgID = ID_TRUE;
                    }
                    else
                    {
                        /* Nohting to do */
                    }

                    // make value list
                    if ( sPrevValue == NULL )
                    {
                        sParseTree->rows->values = sCurrValue;
                        sPrevValue               = sCurrValue;
                    }
                    else
                    {
                        sPrevValue->next = sCurrValue;
                        sPrevValue       = sCurrValue;
                    }
                }
                else
                {
                    if ( sPrevColumn == NULL )
                    {
                        sParseTree->insertColumns = sCurrColumn;
                        sPrevColumn         = sCurrColumn;
                    }
                    else
                    {
                        sPrevColumn->next = sCurrColumn;
                        sPrevColumn       = sCurrColumn;
                    }
                }
            }
        }
        else
        {
            /* Nohting to do */
        }
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_USE_HIDDEN_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_DML_ON_QUEUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_QUEUE_DML_DENIED));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DUP_ALIAS_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME));
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION(ERR_NOT_ONE_BASE_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ONE_BASE_TABLE));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::parseMultiInsertSelect(qcStatement * aStatement)
{
    qmmInsParseTree * sParseTree;
    qmsParseTree    * sViewParseTree;
    qmsTableRef     * sTableRef;
    qmsTableRef     * sInsertTableRef;
    qcmTableInfo    * sInsertTableInfo;
    qcmColumn       * sColumn;
    qcmColumn       * sColumnInfo;
    qmmValueNode    * sPrevValue;
    qmmValueNode    * sCurrValue;
    qmmValueNode    * sFirstValue;
    qmmValueNode    * sValue;
    qcuSqlSourceInfo  sqlInfo;
    qmsFrom           sFrom;
    idBool            sTriggerExist;
    UShort            sInsertTupleId;
    UInt              sFlag;
    qcmViewReadOnly   sReadOnly;
    mtcTemplate     * sMtcTemplate;
    mtcTuple        * sMtcTuple;
    mtcColumn       * sMtcColumn;
    qcmColumn       * sInsertColumn;
    qcmColumn       * sPrevColumn;
    qmsTarget       * sTarget;
    idBool            sFound;

    /* BUG-46124 */
    qmsTarget       * sReturnTarget;
    qmsSFWGH        * sViewSFWGH;

    IDU_FIT_POINT_FATAL( "qmv::parseMultiInsertSelect::__FT__" );

    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseSelectInternal( sParseTree->select )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                             sParseTree->select->mFlag )
                      != IDE_SUCCESS );

    //-------------------------------------------
    // BUG-36596 multi-table insert
    //-------------------------------------------

    for ( ;
          sParseTree != NULL;
          sParseTree = sParseTree->next )
    {
        IDE_DASSERT( (sParseTree->flag & QMM_MULTI_INSERT_MASK) == QMM_MULTI_INSERT_TRUE );

        sViewParseTree  = NULL;
        sInsertTableRef = NULL;
        sTriggerExist   = ID_FALSE;
        sInsertTupleId  = ID_USHORT_MAX;
        sFlag           = 0;
        sReadOnly       = QCM_VIEW_NON_READ_ONLY;
        sTableRef       = sParseTree->tableRef;

        /* PROJ-2204 Join Update, Delete */
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sTableRef;

        IDE_TEST( parseViewInFromClause( aStatement,
                                         &sFrom,
                                         sParseTree->hints )
                  != IDE_SUCCESS );

        // check existence of table and get table META Info.
        sFlag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
        sFlag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
        sFlag &= ~(QMV_VIEW_CREATION_MASK);
        sFlag |= (QMV_VIEW_CREATION_FALSE);

        // PROJ-2204 join update, delete
        // updatable view에 사용되는 SFWGH임을 표시한다.
        if ( sTableRef->view != NULL )
        {
            sViewParseTree = (qmsParseTree*) sTableRef->view->myPlan->parseTree;
        
            if ( sViewParseTree->querySet->SFWGH != NULL )
            {
                sViewParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
                sViewParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
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

        IDE_TEST(qmvQuerySet::validateQmsTableRef( aStatement,
                                                   NULL,
                                                   sTableRef,
                                                   sFlag,
                                                   MTC_COLUMN_NOTNULL_TRUE )
                 != IDE_SUCCESS); // PR-13597

        /******************************
         * PROJ-2204 Join Update, Delete
         ******************************/

        /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
        IDE_TEST( checkInsteadOfTrigger( sTableRef,
                                         QCM_TRIGGER_EVENT_INSERT,
                                         &sTriggerExist ) );

        /* PROJ-1888 INSTEAD OF TRIGGER */
        if ( sTriggerExist == ID_TRUE )
        {
            sParseTree->insteadOfTrigger = ID_TRUE;

            sParseTree->insertTableRef = sTableRef;
            sParseTree->insertColumns  = sParseTree->columns;
        }
        else
        {
            sParseTree->insteadOfTrigger = ID_FALSE;

            sPrevColumn = NULL;

            sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

            // created view, inline view
            if (( ( sMtcTemplate->rows[sTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
                  == MTC_TUPLE_VIEW_TRUE ) &&
                ( sParseTree->tableRef->tableType != QCM_PERFORMANCE_VIEW ))
            {
                sViewSFWGH = sViewParseTree->querySet->SFWGH; /* BUG-46124 */

                /* BUG-49136 */
                IDE_TEST_RAISE( sViewSFWGH == NULL, ERR_NOT_KEY_PRESERVED_TABLE );

                if (sParseTree->columns != NULL)
                {
                    /* insert column list 있는 경우 insertColumns 생성 */
                    for (sColumn = sParseTree->columns;
                         sColumn != NULL;
                         sColumn = sColumn->next )
                    {
                        sFound = ID_FALSE;

                        for ( sTarget = sViewParseTree->querySet->target;
                              sTarget != NULL;
                              sTarget = sTarget->next )
                        {
                            if ( sTarget->aliasColumnName.size != QC_POS_EMPTY_SIZE )
                            {
                                if ( idlOS::strMatch( sTarget->aliasColumnName.name,
                                                      sTarget->aliasColumnName.size,
                                                      sColumn->namePos.stmtText +
                                                      sColumn->namePos.offset,
                                                      sColumn->namePos.size ) == 0 )
                                {
                                    // 이미 한번 찾았다.
                                    IDE_TEST_RAISE( sFound == ID_TRUE, ERR_DUP_ALIAS_NAME );
                                    sFound = ID_TRUE;

                                    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                                            qcmColumn,
                                                            &sInsertColumn)
                                              != IDE_SUCCESS );

                                    QCM_COLUMN_INIT( sInsertColumn );

                                    /* BUG-46124 */
                                    IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                                   sViewSFWGH,
                                                                                                   sViewSFWGH->from,
                                                                                                   sTarget,
                                                                                                   & sReturnTarget )
                                              != IDE_SUCCESS );

                                    sInsertColumn->namePos.stmtText = sReturnTarget->columnName.name;
                                    sInsertColumn->namePos.offset = 0;
                                    sInsertColumn->namePos.size = sReturnTarget->columnName.size;
                                    sInsertColumn->next = NULL;

                                    if( sPrevColumn == NULL )
                                    {
                                        sParseTree->insertColumns = sInsertColumn;
                                        sPrevColumn               = sInsertColumn;
                                    }
                                    else
                                    {
                                        sPrevColumn->next = sInsertColumn;
                                        sPrevColumn       = sInsertColumn;
                                    }

                                    // key-preserved column 검사
                                    sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                                    sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                                    IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                                      != MTC_TUPLE_VIEW_FALSE ) ||
                                                    ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                                      != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                                    ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                                      != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                                    ERR_NOT_KEY_PRESERVED_TABLE );

                                    if( sInsertTupleId == ID_USHORT_MAX )
                                    {
                                        // first
                                        sInsertTupleId = sReturnTarget->targetColumn->node.baseTable;
                                    }
                                    else
                                    {
                                        // insert할 태이블이 여러개임
                                        IDE_TEST_RAISE( sInsertTupleId != sReturnTarget->targetColumn->node.baseTable,
                                                        ERR_NOT_ONE_BASE_TABLE );
                                    }

                                    sInsertTableRef =
                                        QC_SHARED_TMPLATE(aStatement)->tableMap[sInsertTupleId].from->tableRef;
                                }
                                else
                                {
                                    /* Nothing To Do */
                                }
                            }
                            else
                            {
                                /* aliasColumnName이 empty인 경우 어차피 column이 아닌 경우이므로
                                 * insert 되지 않는다.
                                 *
                                 * SELECT C1+1 FROM T1
                                 *
                                 * tableName       = NULL
                                 * aliasTableName  = NULL
                                 * columnName      = NULL
                                 * aliasColumnName = NULL
                                 * displayName     = C1+1
                                 */
                            }
                        }

                        if ( sFound == ID_FALSE )
                        {
                            sqlInfo.setSourceInfo(aStatement,
                                                  &sColumn->namePos);
                            IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    /* column list 없는 경우 insert되는 테이블의 columns 만큼
                     * insertColumns 생성 */
                    for ( sTarget = sViewParseTree->querySet->target;
                          sTarget != NULL;
                          sTarget = sTarget->next )
                    {
                        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                                qcmColumn,
                                                &sInsertColumn)
                                  != IDE_SUCCESS );

                        QCM_COLUMN_INIT( sInsertColumn );
                
                        /* BUG-46124 */
                        IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                       sViewSFWGH,
                                                                                       sViewSFWGH->from,
                                                                                       sTarget,
                                                                                       & sReturnTarget )
                                  != IDE_SUCCESS );

                        sInsertColumn->namePos.stmtText = sReturnTarget->columnName.name;
                        sInsertColumn->namePos.offset = 0;
                        sInsertColumn->namePos.size = sReturnTarget->columnName.size;
                        sInsertColumn->next = NULL;

                        if( sPrevColumn == NULL )
                        {
                            sParseTree->insertColumns = sInsertColumn;
                            sPrevColumn               = sInsertColumn;
                        }
                        else
                        {
                            sPrevColumn->next = sInsertColumn;
                            sPrevColumn       = sInsertColumn;
                        }

                        // key-preserved column 검사
                        sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                        sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                        IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                          != MTC_TUPLE_VIEW_FALSE ) ||
                                        ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                          != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                        ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                          != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                        ERR_NOT_KEY_PRESERVED_TABLE );

                        if( sInsertTupleId == ID_USHORT_MAX )
                        {
                            // first
                            sInsertTupleId = sReturnTarget->targetColumn->node.baseTable;
                        }
                        else
                        {
                            // insert할 태이블이 여러개임
                            IDE_TEST_RAISE( sInsertTupleId != sReturnTarget->targetColumn->node.baseTable,
                                            ERR_NOT_ONE_BASE_TABLE );
                        }

                        sInsertTableRef =
                            QC_SHARED_TMPLATE(aStatement)->tableMap[sInsertTupleId].from->tableRef;
                    }
                }

                /* BUG-36350 Updatable Join DML WITH READ ONLY*/
                if ( sParseTree->tableRef->tableInfo->tableID != 0 )
                {
                    /* view read only */
                    IDE_TEST( qcmView::getReadOnlyOfViews(
                                  QC_SMI_STMT( aStatement ),
                                  sParseTree->tableRef->tableInfo->tableID,
                                  &sReadOnly )
                              != IDE_SUCCESS);
                }
                else
                {
                    /* Nothing To Do */
                }

                /* read only insert error */
                IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY,
                                ERR_NOT_DNL_READ_ONLY_VIEW );

                sParseTree->insertTableRef = sInsertTableRef;
            }
            else
            {
                sParseTree->insertTableRef = sTableRef;
                sParseTree->insertColumns  = sParseTree->columns;
            }
        }

        sInsertTableRef  = sParseTree->insertTableRef;
        sInsertTableInfo = sInsertTableRef->tableInfo;

        // BUG-17409
        sInsertTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
        sInsertTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

        IDE_TEST( insertCommon( aStatement,
                                sInsertTableRef,
                                &(sParseTree->checkConstrList),      /* PROJ-1107 Check Constraint 지원 */
                                &(sParseTree->defaultExprColumns),   /* PROJ-1090 Function-based Index */
                                &(sParseTree->defaultTableRef) )
                  != IDE_SUCCESS );

        // Enqueue 문이 아닌 경우, queue table에 대한 Insert문은 오류.
        IDE_TEST_RAISE(sInsertTableInfo->tableType ==
                       QCM_QUEUE_TABLE,
                       ERR_DML_ON_QUEUE);

        // The possible cases
        //             number of column   number of value
        //  - case 1 :        m                  n        (m = n and n > 0)
        //          => 명시되지 않은 column은 sParseTree->columns 뒤에 덧붙여 연결
        //          => 명시되지 않은 column의 value는 NULL이거나 DEFAULT value로
        //             sParseTree->values에 연결
        //  - case 2 :        m                  n        (m = 0 and n > 0)
        //          => sTableInfo->columns를 가지고 옴.
        //  - case 3 :        m                  n        (m < n and n > 0)
        //          => too many values error
        //  - case 4 :        m                  n        (m > n and n > 0)
        //          => not enough values error
        // The other cases cannot pass parser.

        if (sParseTree->insertColumns != NULL)
        {
            for (sColumn = sParseTree->insertColumns, sCurrValue = sParseTree->rows->values;
                 sColumn != NULL;
                 sColumn = sColumn->next, sCurrValue = sCurrValue->next)
            {
                IDE_TEST_RAISE(sCurrValue == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);

                // The Parser checked duplicated name in column list.
                // check column existence
                if ( QC_IS_NULL_NAME(sColumn->tableNamePos) != ID_TRUE )
                {
                    if ( idlOS::strMatch( sTableRef->aliasName.stmtText + sTableRef->aliasName.offset,
                                          sTableRef->aliasName.size,
                                          sColumn->tableNamePos.stmtText + sColumn->tableNamePos.offset,
                                          sColumn->tableNamePos.size ) != 0 )
                    {
                        sqlInfo.setSourceInfo(aStatement,
                                              &sColumn->namePos);
                        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                    }
                }

                IDE_TEST(qcmCache::getColumn(aStatement,
                                             sInsertTableInfo,
                                             sColumn->namePos,
                                             &sColumnInfo) != IDE_SUCCESS);
                QMV_SET_QCM_COLUMN( sColumn, sColumnInfo );

                /* PROJ-1090 Function-based Index */
                if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                     == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &(sColumn->namePos) );
                    IDE_RAISE( ERR_CANNOT_USE_HIDDEN_COLUMN );
                }
                else
                {
                    /* Nothing to do */
                }

                //--------- validation of inserting value  ---------//
                if (sCurrValue->value == NULL)
                {
                    // insert into t1 values ( DEFAULT )
                    IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumnInfo, sCurrValue)
                             != IDE_SUCCESS);

                    if ( ( sColumnInfo->basicInfo->flag
                           & MTC_COLUMN_TIMESTAMP_MASK )
                         == MTC_COLUMN_TIMESTAMP_TRUE )
                    {
                        sCurrValue->timestamp = ID_TRUE;
                    }
                }
                else
                {
                    IDE_TEST(parseViewInExpression( aStatement, sCurrValue->value )
                             != IDE_SUCCESS);
                }
            }

            IDE_TEST_RAISE(sCurrValue != NULL, ERR_TOO_MANY_INSERT_VALUES);

            // The number of sParseTree->columns = The number of sParseTree->values

            sFirstValue = NULL;
            sPrevValue = NULL;
            for (sColumn = sInsertTableInfo->columns;
                 sColumn != NULL;
                 sColumn = sColumn->next)
            {
                IDE_TEST(getValue(sParseTree->insertColumns,
                                  sParseTree->rows->values,
                                  sColumn,
                                  &sValue)
                         != IDE_SUCCESS);

                // make current value
                IDE_TEST(STRUCT_ALLOC(
                             QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue)
                         != IDE_SUCCESS);

                sCurrValue->next = NULL;
                sCurrValue->msgID = ID_FALSE;

                if (sValue == NULL)
                {
                    // make current value
                    sCurrValue->value = NULL;

                    IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumn, sCurrValue)
                             != IDE_SUCCESS);

                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                         == MTC_COLUMN_TIMESTAMP_TRUE )
                    {
                        sCurrValue->timestamp = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->timestamp = ID_FALSE;
                    }
                    // Proj-1360 Queue
                    // messageid 칼럼인 경우 해당 칼럼에 대한 value값에 flag설정.
                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_QUEUE_MSGID_MASK )
                         == MTC_COLUMN_QUEUE_MSGID_TRUE )
                    {
                        sCurrValue->msgID = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->msgID = ID_FALSE;
                    }
                }
                else
                {
                    sCurrValue->value     = sValue->value;
                    sCurrValue->timestamp = sValue->timestamp;
                    sCurrValue->msgID = sValue->msgID;
                }

                // make value list
                if (sFirstValue == NULL)
                {
                    sFirstValue = sCurrValue;
                    sPrevValue  = sCurrValue;
                }
                else
                {
                    sPrevValue->next = sCurrValue;
                    sPrevValue       = sCurrValue;
                }
            }

            sParseTree->insertColumns = sInsertTableInfo->columns;  // full columns list
            sParseTree->rows->values  = sFirstValue;          // full values list
        }
        else
        {
            sParseTree->insertColumns = sInsertTableInfo->columns;
            sPrevValue = NULL;

            for ( sColumn = sParseTree->insertColumns, sCurrValue = sParseTree->rows->values;
                  sColumn != NULL;
                  sColumn = sColumn->next, sCurrValue = sCurrValue->next )
            {
                /* PROJ-1090 Function-based Index */
                if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                     == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
                {
                    // make value
                    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmmValueNode, &sValue )
                              != IDE_SUCCESS );

                    sValue->value     = NULL;
                    sValue->validate  = ID_TRUE;
                    sValue->calculate = ID_TRUE;
                    sValue->timestamp = ID_FALSE;
                    sValue->msgID     = ID_FALSE;
                    sValue->expand    = ID_FALSE;

                    // connect value list
                    sValue->next      = sCurrValue;
                    sCurrValue        = sValue;
                    if ( sPrevValue == NULL )
                    {
                        sParseTree->rows->values = sValue;
                    }
                    else
                    {
                        sPrevValue->next = sValue;
                    }
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST_RAISE(sCurrValue == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);

                //--------- validation of inserting value  ---------//
                if (sCurrValue->value == NULL)
                {
                    IDE_TEST(setDefaultOrNULL( aStatement, sInsertTableInfo, sColumn, sCurrValue)
                             != IDE_SUCCESS);

                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                         == MTC_COLUMN_TIMESTAMP_TRUE )
                    {
                        sCurrValue->timestamp = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->timestamp = ID_FALSE;
                    }

                    if ( ( sColumn->basicInfo->flag & MTC_COLUMN_QUEUE_MSGID_MASK )
                         == MTC_COLUMN_QUEUE_MSGID_TRUE )
                    {
                        sCurrValue->msgID = ID_TRUE;
                    }
                    else
                    {
                        sCurrValue->msgID = ID_FALSE;
                    }
                }
                else
                {
                    IDE_TEST(parseViewInExpression( aStatement, sCurrValue->value )
                             != IDE_SUCCESS);
                }

                sPrevValue = sCurrValue;
            }
            IDE_TEST_RAISE(sCurrValue != NULL, ERR_TOO_MANY_INSERT_VALUES);
        }
        // 초기화
        sParseTree->columnsForValues = NULL;
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_USE_HIDDEN_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ENOUGH_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TOO_MANY_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_DML_ON_QUEUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_QUEUE_DML_DENIED));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DUP_ALIAS_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME));
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION(ERR_NOT_ONE_BASE_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ONE_BASE_TABLE));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateInsertSelect(qcStatement * aStatement)
{
    qmmInsParseTree   * sParseTree;
    qmsTableRef       * sInsertTableRef;
    qcmTableInfo      * sInsertTableInfo;
    qcmColumn         * sColumn;
    qmsTarget         * sTarget;
    qmmValueNode      * sCurrValue;
    qcuSqlSourceInfo    sqlInfo;
    SInt                sTargetCount    = 0;
    SInt                sInsColCount    = 0;
    qmsQuerySet       * sSelectQuerySet = NULL;
    qmsFrom             sFrom;
    qdConstraintSpec  * sConstr;
    qcmColumn         * sQcmColumn;

    IDU_FIT_POINT_FATAL( "qmv::validateInsertSelect::__FT__" );

    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;

    sInsertTableRef  = sParseTree->insertTableRef;
    sInsertTableInfo = sInsertTableRef->tableInfo;

    /* TASK-7307 DML Data Consistency in Shard */
    IDE_TEST( checkUsableTable( aStatement,
                                sInsertTableInfo ) != IDE_SUCCESS );

    IDE_TEST( makeInsertRow( aStatement, sParseTree ) != IDE_SUCCESS );

    // PROJ-1502 PARTITIONED DISK TABLE
    // insert ~ select의 경우는 partition pruning이 일어나지 않으므로
    // validation단계에서 partition만 구한다.
    if( sInsertTableInfo->tablePartitionType
        == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qmoPartition::makePartitions( aStatement,
                                                sInsertTableRef )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockPartitions(
                      aStatement,
                      sInsertTableRef,
                      SMI_TABLE_LOCK_IS )
                  != IDE_SUCCESS );

        /* PROJ-2464 hybrid partitioned table 지원 */
        IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sInsertTableRef )
                  != IDE_SUCCESS );
    }

    // PR-13725
    // CHECK OPERATABLE
    IDE_TEST( checkInsertOperatable( aStatement,
                                     sParseTree,
                                     sInsertTableInfo )
              != IDE_SUCCESS );

    /* PROJ-2211 Materialized View */
    if ( sInsertTableInfo->tableType == QCM_MVIEW_TABLE )
    {
        IDE_TEST_RAISE( sParseTree->hints == NULL,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );

        IDE_TEST_RAISE( sParseTree->hints->refreshMView != ID_TRUE,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    // set member of qcStatement
    sParseTree->select->myPlan->planEnv = aStatement->myPlan->planEnv;
    sParseTree->select->spvEnv          = aStatement->spvEnv;
    sParseTree->select->mRandomPlanInfo = aStatement->mRandomPlanInfo;

    sSelectQuerySet = ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet;
    // validation of SELECT statement
    sSelectQuerySet->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sSelectQuerySet->lflag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sSelectQuerySet->lflag &= ~(QMV_VIEW_CREATION_MASK);
    sSelectQuerySet->lflag |= (QMV_VIEW_CREATION_FALSE);

    /* PROJ-2197 PSM Renewal
     * Select Target을 cast하기 위해서 PSM에서 불렸음을 표시한다. */
    sParseTree->select->calledByPSMFlag = aStatement->calledByPSMFlag;

    /* Insert Select 의 Select 는 Subqeury 가 아니라 View 이다.
     *  이곳에서 shardStmtType 를 전달해준다.
     *   다른 Subquery 의 경우 qtcSubqueryEstimate 시점에 전달한다.
     */
    IDE_TEST( sdi::setShardStmtType( aStatement,
                                     sParseTree->select )
              != IDE_SUCCESS );

    IDE_TEST(validateSelect(sParseTree->select) != IDE_SUCCESS);

    /* PROJ-2197 PSM Renewal
     * validate 전으로 원복한다. */
    sParseTree->select->calledByPSMFlag = ID_FALSE;

    // fix BUG-18752
    aStatement->myPlan->parseTree->currValSeqs =
        sParseTree->select->myPlan->parseTree->currValSeqs;

    if (sParseTree->insertColumns != NULL)
    {
        for ( sTarget = sSelectQuerySet->target;
              sTarget != NULL;
              sTarget = sTarget->next)
        {
            ++sTargetCount;
        }

        for (sColumn = sParseTree->insertColumns;
             sColumn != NULL;
             sColumn = sColumn->next)
        {
            ++sInsColCount;
        }

        IDE_TEST_RAISE(sTargetCount < sInsColCount,
                       ERR_NOT_ENOUGH_INSERT_VALUES);
        IDE_TEST_RAISE(sTargetCount > sInsColCount,
                       ERR_TOO_MANY_INSERT_VALUES);

        for (sColumn    = sParseTree->columnsForValues,
                 sCurrValue = sParseTree->rows->values;
             sCurrValue != NULL;
             sColumn    = sColumn->next,
                 sCurrValue = sCurrValue->next )
        {
            if (sCurrValue->value != NULL)
            {
                if ( qtc::estimate( sCurrValue->value,
                                    QC_SHARED_TMPLATE(aStatement),
                                    aStatement,
                                    NULL,
                                    NULL,
                                    NULL )
                     != IDE_SUCCESS )
                {
                    // default value인 경우 별도 에러처리
                    IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                    == QTC_NODE_DEFAULT_VALUE_TRUE,
                                    ERR_INVALID_DEFAULT_VALUE );
                    
                    // default value가 아닌 경우 에러pass
                    IDE_TEST( 1 );
                }
                else
                {
                    // Nothing to do.
                }

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                // fix PR-4390 : sModule => sColumn->basicInfo->module
                IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                   aStatement,
                                                   QC_SHARED_TMPLATE(aStatement),
                                                   sColumn->basicInfo->module )
                          != IDE_SUCCESS );

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                if ( ( sCurrValue->value->node.lflag
                       & MTC_NODE_DML_MASK )
                     == MTC_NODE_DML_UNUSABLE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrValue->value->position );
                    IDE_RAISE( ERR_USE_CURSOR_ATTR );
                }
            }
        }
    }
    else
    {
        sParseTree->insertColumns = sInsertTableInfo->columns;
        sParseTree->columnsForValues = NULL;

        for (sColumn = sParseTree->insertColumns,
                 sTarget = ((qmsParseTree *)(sParseTree->select->myPlan->parseTree))
                 ->querySet->target;
             sColumn != NULL;
             sColumn = sColumn->next,
                 sTarget = sTarget->next)
        {
            IDE_TEST_RAISE(sTarget == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);
        }

        IDE_TEST_RAISE(sTarget != NULL, ERR_TOO_MANY_INSERT_VALUES);
    }

    IDE_TEST(getParentInfoList(aStatement,
                               sInsertTableInfo,
                               &(sParseTree->parentConstraints))
             != IDE_SUCCESS);

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 처리시 포린키 정보수집
    //---------------------------------------------

    IDE_TEST( setFetchColumnInfo4ParentTable(
                  aStatement,
                  sInsertTableRef )
              != IDE_SUCCESS );

    // PROJ-2205 DML trigger에 의한 컬럼참조
    IDE_TEST( setFetchColumnInfo4Trigger(
                  aStatement,
                  sInsertTableRef )
              != IDE_SUCCESS );

    // PROJ-1665
    // Parallel 대상 테이블과 Insert 대상 Table이 동일한지 검사
    // 동일하지 않은 경우, Hint를 무시하도록 설정
    if ( sParseTree->hints != NULL )
    {
        if ( sParseTree->hints->parallelHint != NULL )
        {
            // BUG-46286
            if ( sParseTree->hints->parallelHint->table != NULL )
            {
                if (idlOS::strMatch(
                        sParseTree->hints->parallelHint->table->tableName.stmtText +
                        sParseTree->hints->parallelHint->table->tableName.offset,
                        sParseTree->hints->parallelHint->table->tableName.size,
                        sInsertTableRef->aliasName.stmtText +
                        sInsertTableRef->aliasName.offset,
                        sInsertTableRef->aliasName.size) == 0 ||
                    idlOS::strMatch(
                        sParseTree->hints->parallelHint->table->tableName.stmtText +
                        sParseTree->hints->parallelHint->table->tableName.offset,
                        sParseTree->hints->parallelHint->table->tableName.size,
                        sInsertTableRef->tableName.stmtText +
                        sInsertTableRef->tableName.offset,
                        sInsertTableRef->tableName.size) == 0)
                {
                    // 동일 테이블인 경우, nothing to do
                }
                else
                {
                    // Hint 무시
                    sParseTree->hints->parallelHint = NULL;
                }
            }
            else
            {
                sParseTree->hints->parallelHint = NULL;
            }
        }
        else
        {
            // Parallel Hint 없음
        }

        IDE_TEST( validatePlanHints( aStatement,
                                     sParseTree->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Hint 없음
    }

    /* PROJ-1584 DML Return Clause */
    if( sParseTree->returnInto != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sInsertTableRef;

        IDE_TEST( validateReturnInto( aStatement,
                                      sParseTree->returnInto,
                                      NULL,
                                      &sFrom,
                                      ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    /* PROJ-1107 Check Constraint 지원 */
    if ( sParseTree->checkConstrList != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sInsertTableRef;

        for ( sConstr = sParseTree->checkConstrList;
              sConstr != NULL;
              sConstr = sConstr->next )
        {
            IDE_TEST( qdbCommon::validateCheckConstrDefinition(
                          aStatement,
                          sConstr,
                          NULL,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-1090 Function-based Index */
    if ( sParseTree->defaultExprColumns != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sParseTree->defaultTableRef;

        for ( sQcmColumn = sParseTree->defaultExprColumns;
              sQcmColumn != NULL;
              sQcmColumn = sQcmColumn->next)
        {
            IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                          aStatement,
                          sQcmColumn->defaultValue,
                          NULL,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DML_MVIEW_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GRANT_DML_PRIV_OF_MVIEW_TABLE ) );
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ENOUGH_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TOO_MANY_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_DEFAULT_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateMultiInsertSelect(qcStatement * aStatement)
{
    qmmInsParseTree   * sParseTree;
    qmsTableRef       * sInsertTableRef;
    qcmTableInfo      * sInsertTableInfo = NULL;
    qcmColumn         * sCurrColumn;
    mtcColumn         * sMtcColumn;
    qmmValueNode      * sCurrValue;
    qcuSqlSourceInfo    sqlInfo;
    qmsQuerySet       * sSelectQuerySet = NULL;
    qmsFrom             sFrom;
    qdConstraintSpec  * sConstr;
    qcmColumn         * sQcmColumn;
    const mtdModule  ** sModules;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmv::validateMultiInsertSelect::__FT__" );
    
    sParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;

    // set member of qcStatement
    sParseTree->select->myPlan->planEnv = aStatement->myPlan->planEnv;
    sParseTree->select->spvEnv          = aStatement->spvEnv;
    sParseTree->select->mRandomPlanInfo = aStatement->mRandomPlanInfo;

    sSelectQuerySet = ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet;
    // validation of SELECT statement
    sSelectQuerySet->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sSelectQuerySet->lflag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sSelectQuerySet->lflag &= ~(QMV_VIEW_CREATION_MASK);
    sSelectQuerySet->lflag |= (QMV_VIEW_CREATION_FALSE);

    /* PROJ-2197 PSM Renewal
     * Select Target을 cast하기 위해서 PSM에서 불렸음을 표시한다. */
    sParseTree->select->calledByPSMFlag = aStatement->calledByPSMFlag;

    IDE_TEST(validateSelect(sParseTree->select) != IDE_SUCCESS);

    /* PROJ-2197 PSM Renewal
     * validate 전으로 원복한다. */
    sParseTree->select->calledByPSMFlag = ID_FALSE;

    // fix BUG-18752
    aStatement->myPlan->parseTree->currValSeqs =
        sParseTree->select->myPlan->parseTree->currValSeqs;

    //-------------------------------------------
    // BUG-36596 multi-table insert
    //-------------------------------------------

    for ( i = 0;
          sParseTree != NULL;
          sParseTree = sParseTree->next, i++ )
    {
        IDE_DASSERT( (sParseTree->flag & QMM_MULTI_INSERT_MASK) == QMM_MULTI_INSERT_TRUE );

        sInsertTableRef  = sParseTree->insertTableRef;
        sInsertTableInfo = sInsertTableRef->tableInfo;

        IDE_TEST( makeInsertRow( aStatement, sParseTree ) != IDE_SUCCESS );

        // PROJ-1502 PARTITIONED DISK TABLE
        // insert ~ select의 경우는 partition pruning이 일어나지 않으므로
        // validation단계에서 partition만 구한다.
        if( sInsertTableInfo->tablePartitionType
            == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST( qmoPartition::makePartitions( aStatement,
                                                    sInsertTableRef )
                      != IDE_SUCCESS );

            IDE_TEST( qcmPartition::validateAndLockPartitions(
                          aStatement,
                          sInsertTableRef,
                          SMI_TABLE_LOCK_IS )
                      != IDE_SUCCESS );

            /* PROJ-2464 hybrid partitioned table 지원 */
            IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sInsertTableRef )
                      != IDE_SUCCESS );
        }

        // PR-13725
        // CHECK OPERATABLE
        IDE_TEST( checkInsertOperatable( aStatement,
                                         sParseTree,
                                         sInsertTableInfo )
                  != IDE_SUCCESS );

        /* PROJ-2211 Materialized View */
        if ( sInsertTableInfo->tableType == QCM_MVIEW_TABLE )
        {
            IDE_TEST_RAISE( sParseTree->hints == NULL,
                            ERR_NO_GRANT_DML_MVIEW_TABLE );

            IDE_TEST_RAISE( sParseTree->hints->refreshMView != ID_TRUE,
                            ERR_NO_GRANT_DML_MVIEW_TABLE );
        }
        else
        {
            /* Nothing to do */
        }

        sModules = sParseTree->columnModules;

        for( sCurrColumn = sParseTree->insertColumns,
                 sCurrValue = sParseTree->rows->values;
             sCurrValue != NULL;
             sCurrColumn = sCurrColumn->next,
                 sCurrValue = sCurrValue->next, sModules++ )
        {
            if( sCurrValue->value != NULL )
            {
                /* PROJ-2197 PSM Renewal */
                sCurrValue->value->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

                sSelectQuerySet->processPhase = QMS_VALIDATE_INSERT;

                if ( qtc::estimate( sCurrValue->value,
                                    QC_SHARED_TMPLATE(aStatement),
                                    aStatement,
                                    sSelectQuerySet,
                                    sSelectQuerySet->SFWGH,
                                    NULL )
                     != IDE_SUCCESS )
                {
                    // default value인 경우 별도 에러처리
                    IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                    == QTC_NODE_DEFAULT_VALUE_TRUE,
                                    ERR_INVALID_DEFAULT_VALUE );

                    // default value가 아닌 경우 에러pass
                    IDE_TEST( 1 );
                }
                else
                {
                    // Nothing to do.
                }

                /* BUG-42815 multi table insert시에 subquery가 처음에 사용되는
                 * 경우는 error 를 낸다. 첫번째 insert values는 insertSelect 로
                 * 처리되기 때문에 subquery에 대한 고려가 되어있지 않기
                 * 때문이다.
                 */
                if ( ( i == 0 ) && ( QTC_HAVE_SUBQUERY( sCurrValue->value ) == ID_TRUE ) )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &sCurrValue->value->position );
                    IDE_RAISE( ERR_NOT_SUPPORT_SUBQUERY );
                }
                else
                {
                    /* Nothing to do */
                }

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                // PROJ-1988 Merge Query
                // 아래와 같은 쿼리에서 t1.i1과 t2.i1이 encrypted column이라면
                // policy가 다를 수 있으므로 t2.i1에 decrypt func을 생성한다.
                //
                // merge into t1 using t2 on (t1.i1 = t2.i1)
                // when not matched then
                // insert (t1.i1, t1.i2) values (t2.i1, t2.i2);
                //
                // BUG-32303
                // insert into t1(i1) values (echar'abc');
                sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );

                if ( (sMtcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                     == MTD_ENCRYPT_TYPE_TRUE )
                {
                    // add decrypt func
                    IDE_TEST( addDecryptFunc4ValueNode( aStatement,
                                                        sCurrValue )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                   aStatement,
                                                   QC_SHARED_TMPLATE(aStatement),
                                                   *sModules )
                          != IDE_SUCCESS );

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                // BUG-15746
                IDE_TEST( describeParamInfo( aStatement,
                                             sCurrColumn->basicInfo,
                                             sCurrValue->value )
                          != IDE_SUCCESS );

                if( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                    == MTC_NODE_DML_UNUSABLE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &sCurrValue->value->position );
                    IDE_RAISE( ERR_USE_CURSOR_ATTR );
                }
            }
        }

        IDE_TEST(getParentInfoList(aStatement,
                                   sInsertTableInfo,
                                   &(sParseTree->parentConstraints))
                 != IDE_SUCCESS);

        //---------------------------------------------
        // PROJ-1705 질의에 사용된 컬럼정보수집
        // DML 처리시 포린키 정보수집
        //---------------------------------------------

        IDE_TEST( setFetchColumnInfo4ParentTable(
                      aStatement,
                      sInsertTableRef )
                  != IDE_SUCCESS );

        // PROJ-2205 DML trigger에 의한 컬럼참조
        IDE_TEST( setFetchColumnInfo4Trigger(
                      aStatement,
                      sInsertTableRef )
                  != IDE_SUCCESS );

        // PROJ-1665
        // Parallel 대상 테이블과 Insert 대상 Table이 동일한지 검사
        // 동일하지 않은 경우, Hint를 무시하도록 설정
        if ( sParseTree->hints != NULL )
        {
            if ( sParseTree->hints->parallelHint != NULL )
            {
                // BUG-46286
                if ( sParseTree->hints->parallelHint->table != NULL )
                {
                    if (idlOS::strMatch(
                            sParseTree->hints->parallelHint->table->tableName.stmtText +
                            sParseTree->hints->parallelHint->table->tableName.offset,
                            sParseTree->hints->parallelHint->table->tableName.size,
                            sInsertTableRef->aliasName.stmtText +
                            sInsertTableRef->aliasName.offset,
                            sInsertTableRef->aliasName.size) == 0 ||
                        idlOS::strMatch(
                            sParseTree->hints->parallelHint->table->tableName.stmtText +
                            sParseTree->hints->parallelHint->table->tableName.offset,
                            sParseTree->hints->parallelHint->table->tableName.size,
                            sInsertTableRef->tableName.stmtText +
                            sInsertTableRef->tableName.offset,
                            sInsertTableRef->tableName.size) == 0)
                    {
                        // 동일 테이블인 경우, nothing to do
                    }
                    else
                    {
                        // Hint 무시
                        sParseTree->hints->parallelHint = NULL;
                    }
                }
                else
                {
                    sParseTree->hints->parallelHint = NULL;
                }
            }
            else
            {
                // Parallel Hint 없음
            }

            IDE_TEST( validatePlanHints( aStatement,
                                         sParseTree->hints )
                      != IDE_SUCCESS );
        }
        else
        {
            // Hint 없음
        }

        /* PROJ-1584 DML Return Clause */
        if( sParseTree->returnInto != NULL )
        {
            QCP_SET_INIT_QMS_FROM( (&sFrom) );
            sFrom.tableRef = sInsertTableRef;

            IDE_TEST( validateReturnInto( aStatement,
                                          sParseTree->returnInto,
                                          NULL,
                                          &sFrom,
                                          ID_TRUE )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        /* PROJ-1107 Check Constraint 지원 */
        if ( sParseTree->checkConstrList != NULL )
        {
            QCP_SET_INIT_QMS_FROM( (&sFrom) );
            sFrom.tableRef = sInsertTableRef;

            for ( sConstr = sParseTree->checkConstrList;
                  sConstr != NULL;
                  sConstr = sConstr->next )
            {
                IDE_TEST( qdbCommon::validateCheckConstrDefinition(
                              aStatement,
                              sConstr,
                              NULL,
                              &sFrom )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-1090 Function-based Index */
        if ( sParseTree->defaultExprColumns != NULL )
        {
            QCP_SET_INIT_QMS_FROM( (&sFrom) );
            sFrom.tableRef = sParseTree->defaultTableRef;

            for ( sQcmColumn = sParseTree->defaultExprColumns;
                  sQcmColumn != NULL;
                  sQcmColumn = sQcmColumn->next)
            {
                IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                              aStatement,
                              sQcmColumn->defaultValue,
                              NULL,
                              &sFrom )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    /*
     * BUG-39441
     * need a interface which returns whether DML on replication table or not
     */
    if (sInsertTableInfo->replicationCount > 0)
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_FALSE;
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DML_MVIEW_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GRANT_DML_PRIV_OF_MVIEW_TABLE ) );
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_DEFAULT_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE ) );
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORT_SUBQUERY )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_NOT_ALLOWED_SUBQUERY_SQLTEXT,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::makeInsertRow( qcStatement      * aStatement,
                           qmmInsParseTree  * aParseTree )
{
    qcmTableInfo       * sTableInfo;
    smiValue           * sInsOrUptRow;
    UInt                 sColumnCount;
    mtcColumn          * sColumns;
    UInt                 sIterator;
    UShort               sCanonizedTuple;
    UShort               sCompressedTuple;
    UInt                 sOffset;
    mtcTemplate        * sMtcTemplate;
    idBool               sHasCompressedColumn;
    ULong                sRowSize = ID_ULONG(0);

    IDU_FIT_POINT_FATAL( "qmv::makeInsertRow::__FT__" );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sTableInfo = aParseTree->insertTableRef->tableInfo;
    sColumnCount = sTableInfo->columnCount;
    sHasCompressedColumn = ID_FALSE;

    // alloc insert row
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(smiValue) * sColumnCount, (void**)&sInsOrUptRow)
             != IDE_SUCCESS);

    QC_SHARED_TMPLATE(aStatement)->insOrUptRow[ aParseTree->valueIdx ] = sInsOrUptRow ;
    QC_SHARED_TMPLATE(aStatement)->insOrUptRowValueCount[ aParseTree->valueIdx ] =
        sColumnCount ;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtdModule*) * sColumnCount,
                                            (void**)&aParseTree->columnModules)
             != IDE_SUCCESS);

    IDE_TEST( qtc::nextTable( &sCanonizedTuple, aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    aParseTree->canonizedTuple = sCanonizedTuple;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(mtcColumn) * sColumnCount,
                 (void**) & ( sMtcTemplate->rows[sCanonizedTuple].columns))
             != IDE_SUCCESS);


    for ( sIterator = 0, sOffset = 0;
          sIterator < sColumnCount;
          sIterator++ )
    {
        sColumns = sTableInfo->columns[sIterator].basicInfo;

        aParseTree->columnModules[sIterator] = sColumns->module;

        mtc::copyColumn( &(sMtcTemplate->rows[sCanonizedTuple].columns[sIterator]),
                         sColumns );

        sOffset = idlOS::align(
            sOffset,
            sColumns->module->align );

        // PROJ-1362
        if ( (sColumns->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_LOB )
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.offset
                = sOffset;
            // lob에 최대로 입력될 수 있는 값의 길이는 varchar의 최대값이다.
            sOffset += MTD_CHAR_PRECISION_MAXIMUM;
        }
        else
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.offset
                = sOffset;
            sOffset +=
                sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.size;
        }

        // PROJ-2264 Dictionary table
        if( (sColumns->column.flag & SMI_COLUMN_COMPRESSION_MASK) == SMI_COLUMN_COMPRESSION_TRUE )
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.flag |=
                SMI_COLUMN_COMPRESSION_TRUE;
            
            sHasCompressedColumn = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    sMtcTemplate->rows[sCanonizedTuple].modify = 0;
    sMtcTemplate->rows[sCanonizedTuple].lflag
        = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
    // fixAfterValidation에서 할당하지 않고 바로 할당한다.
    sMtcTemplate->rows[sCanonizedTuple].lflag
        &= ~MTC_TUPLE_ROW_SKIP_MASK;
    sMtcTemplate->rows[sCanonizedTuple].lflag
        |= MTC_TUPLE_ROW_SKIP_TRUE;
    sMtcTemplate->rows[sCanonizedTuple].columnCount
        = sColumnCount;
    sMtcTemplate->rows[sCanonizedTuple].columnMaximum
        = sColumnCount;
    sMtcTemplate->rows[sCanonizedTuple].columnLocate
        = NULL;
    sMtcTemplate->rows[sCanonizedTuple].execute
        = NULL;
    sMtcTemplate->rows[sCanonizedTuple].rowOffset
        = sOffset;
    sMtcTemplate->rows[sCanonizedTuple].rowMaximum
        = sOffset;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 sOffset,
                 (void**) &(sMtcTemplate->rows[sCanonizedTuple].row) )
             != IDE_SUCCESS);

    // PROJ-2264 Dictionary table
    if( sHasCompressedColumn == ID_TRUE )
    {
        IDE_TEST( qtc::nextTable( &sCompressedTuple, aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                  != IDE_SUCCESS );

        aParseTree->compressedTuple = sCompressedTuple;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     ID_SIZEOF(mtcColumn) * sColumnCount,
                     (void**) & ( sMtcTemplate->rows[sCompressedTuple].columns))
                 != IDE_SUCCESS);

        for( sIterator = 0; sIterator < sColumnCount; sIterator++)
        {
            sColumns = sTableInfo->columns[sIterator].basicInfo;

            mtc::copyColumn( &(sMtcTemplate->rows[sCompressedTuple].columns[sIterator]),
                             sColumns );

            if( (sColumns->column.flag & SMI_COLUMN_COMPRESSION_MASK) == SMI_COLUMN_COMPRESSION_TRUE )
            {
                // smiColumn 의 size 변경
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.size =
                    ID_SIZEOF(smOID);
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.flag |=
                    SMI_COLUMN_COMPRESSION_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }//for

        for( sIterator = 0, sOffset = 0;
             sIterator < sColumnCount;
             sIterator++ )
        {
            sColumns = &(sMtcTemplate->rows[sCompressedTuple].columns[sIterator]);

            // BUG-37460 compress column 에서 align 을 맞추어야 합니다.
            if( ( sColumns->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                == SMI_COLUMN_COMPRESSION_TRUE )
            {
                sOffset = idlOS::align( sOffset, ID_SIZEOF(smOID) );
                // BUG-47166 valgrind error
                sColumns->column.offset = sOffset;
                sRowSize                = (ULong)sOffset + (ULong)sColumns->column.size;
                IDE_TEST_RAISE( sRowSize > (ULong)ID_UINT_MAX, ERR_EXCEED_TUPLE_ROW_MAX_SIZE );

                sOffset = (UInt)sRowSize;
            }
            else
            {
                // BUG-47166 valgrind error
                // Compress column이 아닌 경우 dummy mtcColumn으로만듭니다.
                sColumns->column.offset   = 0;
                sColumns->column.size     = 0;
            }
        }

        sMtcTemplate->rows[sCompressedTuple].modify = 0;
        sMtcTemplate->rows[sCompressedTuple].lflag
            = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
        // fixAfterValidation에서 할당하지 않고 바로 할당한다.
        sMtcTemplate->rows[sCompressedTuple].lflag &= ~MTC_TUPLE_ROW_SKIP_MASK;
        sMtcTemplate->rows[sCompressedTuple].lflag |=  MTC_TUPLE_ROW_SKIP_TRUE;
        sMtcTemplate->rows[sCompressedTuple].columnCount   = sColumnCount;
        sMtcTemplate->rows[sCompressedTuple].columnMaximum = sColumnCount;
        sMtcTemplate->rows[sCompressedTuple].columnLocate  = NULL;
        sMtcTemplate->rows[sCompressedTuple].execute       = NULL;
        sMtcTemplate->rows[sCompressedTuple].rowOffset     = sOffset;
        sMtcTemplate->rows[sCompressedTuple].rowMaximum    = sOffset;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     sOffset,
                     (void**) &(sMtcTemplate->rows[sCompressedTuple].row) )
                 != IDE_SUCCESS);
    }
    else
    {
        // compressed tuple 을 사용하지 않음
        aParseTree->compressedTuple = UINT_MAX;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_TUPLE_ROW_MAX_SIZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmv::makeInsertRow",
                                  "tuple row size is larger than 4GB" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::makeUpdateRow(qcStatement * aStatement)
{
    qmmUptParseTree    * sParseTree;
    smiValue           * sInsOrUptRow;
    UInt                 sColumnCount;
    qcmColumn          * sColumn;
    UInt                 sIterator;
    UShort               sCanonizedTuple;
    UShort               sCompressedTuple;
    UInt                 sOffset;

    mtcTemplate        * sMtcTemplate;
    idBool               sHasCompressedColumn;
    ULong                sRowSize = ID_ULONG(0);

    IDU_FIT_POINT_FATAL( "qmv::makeUpdateRow::__FT__" );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sParseTree = (qmmUptParseTree*) aStatement->myPlan->parseTree;
    sHasCompressedColumn = ID_FALSE;

    // alloc updating value
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(smiValue) * sParseTree->uptColCount, (void**)&sInsOrUptRow)
             != IDE_SUCCESS);

    QC_SHARED_TMPLATE(aStatement)->insOrUptRow[ sParseTree->valueIdx ] = sInsOrUptRow;
    QC_SHARED_TMPLATE(aStatement)->insOrUptRowValueCount[ sParseTree->valueIdx ] =
        sParseTree->uptColCount;

    sColumnCount = sParseTree->uptColCount;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtdModule*) * sColumnCount,
                                            (void**)&(sParseTree->columnModules) )
             != IDE_SUCCESS);

    IDE_TEST( qtc::nextTable( &sCanonizedTuple, aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    sParseTree->canonizedTuple = sCanonizedTuple;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(mtcColumn) * sColumnCount,
                 (void**)&( sMtcTemplate->rows[sCanonizedTuple].columns))
             != IDE_SUCCESS);

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtdIsNullFunc) * sColumnCount,
                                            (void**)&(sParseTree->isNull))
             != IDE_SUCCESS);

    for (sColumn     = sParseTree->updateColumns,
             sOffset     = 0,
             sIterator   = 0;
         sColumn    != NULL;
         sColumn     = sColumn->next,
             sIterator++ )
    {
        sParseTree->columnModules[sIterator] = sColumn->basicInfo->module;

        mtc::copyColumn( &(sMtcTemplate->rows[sCanonizedTuple].columns[sIterator]),
                         sColumn->basicInfo );

        // PROJ-2264 Dictionary table
        if( (sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK)
            == SMI_COLUMN_COMPRESSION_TRUE )
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.flag |=
                SMI_COLUMN_COMPRESSION_TRUE;
            
            sHasCompressedColumn = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        sOffset = idlOS::align(
            sOffset,
            sColumn->basicInfo->module->align );

        // PROJ-1362
        if ( (sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_LOB )
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.offset
                = sOffset;
            // lob에 최대로 입력될 수 있는 값의 길이는 varchar의 최대값이다.
            sOffset += MTD_CHAR_PRECISION_MAXIMUM;
        }
        else
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.offset
                = sOffset;
            sOffset +=
                sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.size;
        }

        if( ( sColumn->basicInfo->flag & MTC_COLUMN_NOTNULL_MASK )
            == MTC_COLUMN_NOTNULL_TRUE )
        {
            sParseTree->isNull[sIterator] =
                sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].module->isNull;
        }
        else
        {
            sParseTree->isNull[sIterator] = mtd::isNullDefault;
        }
    }

    sMtcTemplate->rows[sCanonizedTuple].modify = 0;
    sMtcTemplate->rows[sCanonizedTuple].lflag
        = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
    // fixAfterValidation에서 할당하지 않고 바로 할당한다.
    sMtcTemplate->rows[sCanonizedTuple].lflag
        &= ~MTC_TUPLE_ROW_SKIP_MASK;
    sMtcTemplate->rows[sCanonizedTuple].lflag
        |= MTC_TUPLE_ROW_SKIP_TRUE;
    sMtcTemplate->rows[sCanonizedTuple].columnCount
        = sColumnCount;
    sMtcTemplate->rows[sCanonizedTuple].columnMaximum
        = sColumnCount;
    sMtcTemplate->rows[sCanonizedTuple].columnLocate
        = NULL;
    sMtcTemplate->rows[sCanonizedTuple].execute
        = NULL;
    sMtcTemplate->rows[sCanonizedTuple].rowOffset
        = sOffset;
    sMtcTemplate->rows[sCanonizedTuple].rowMaximum
        = sOffset;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 sOffset,
                 (void**) & (sMtcTemplate->rows[sCanonizedTuple].row) )
             != IDE_SUCCESS);

    // PROJ-2264 Dictionary table
    if( sHasCompressedColumn == ID_TRUE )
    {
        IDE_TEST( qtc::nextTable( &sCompressedTuple, aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                  != IDE_SUCCESS );

        sParseTree->compressedTuple = sCompressedTuple;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     ID_SIZEOF(mtcColumn) * sColumnCount,
                     (void**) & ( sMtcTemplate->rows[sCompressedTuple].columns))
                 != IDE_SUCCESS);

        for( sColumn    = sParseTree->updateColumns,
                 sOffset    = 0,
                 sIterator  = 0;
             sColumn   != NULL;
             sColumn    = sColumn->next,
                 sIterator++ )
        {
            mtc::copyColumn( &(sMtcTemplate->rows[sCompressedTuple].columns[sIterator]),
                             sColumn->basicInfo );

            // BUG-37460 compress column 에서 align 을 맞추어야 합니다.
            if( (sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK)
                == SMI_COLUMN_COMPRESSION_TRUE )
            {
                // smiColumn 의 size 변경
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.size =
                    ID_SIZEOF(smOID);
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.flag |=
                    SMI_COLUMN_COMPRESSION_TRUE;

                sOffset = idlOS::align( sOffset, ID_SIZEOF(smOID) );

                // PROJ-1362
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.offset
                    = sOffset;
                // BUG-47166 valgrind error
                sRowSize = (ULong)sOffset + (ULong)sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.size;
                IDE_TEST_RAISE( sRowSize > (ULong)ID_UINT_MAX, ERR_EXCEED_TUPLE_ROW_MAX_SIZE );

                sOffset = (UInt)sRowSize;
            }
            else
            {
                // BUG-47166 valgrind error
                // Compress column이 아닌 경우 dummy mtcColumn으로만듭니다.
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.offset
                    = 0;
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.size = 0;
            }
        }

        sMtcTemplate->rows[sCompressedTuple].modify = 0;
        sMtcTemplate->rows[sCompressedTuple].lflag
            = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
        // fixAfterValidation에서 할당하지 않고 바로 할당한다.
        sMtcTemplate->rows[sCompressedTuple].lflag &= ~MTC_TUPLE_ROW_SKIP_MASK;
        sMtcTemplate->rows[sCompressedTuple].lflag |=  MTC_TUPLE_ROW_SKIP_TRUE;
        sMtcTemplate->rows[sCompressedTuple].columnCount    = sColumnCount;
        sMtcTemplate->rows[sCompressedTuple].columnMaximum  = sColumnCount;
        sMtcTemplate->rows[sCompressedTuple].columnLocate   = NULL;
        sMtcTemplate->rows[sCompressedTuple].execute        = NULL;
        sMtcTemplate->rows[sCompressedTuple].rowOffset      = sOffset;
        sMtcTemplate->rows[sCompressedTuple].rowMaximum     = sOffset;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     sOffset,
                     (void**) &(sMtcTemplate->rows[sCompressedTuple].row) )
                 != IDE_SUCCESS);
    }
    else
    {
        // compressed tuple 을 사용하지 않음
        sParseTree->compressedTuple = UINT_MAX;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_TUPLE_ROW_MAX_SIZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmv::makeUpdateRow",
                                  "tuple row size is larger than 4GB" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateDelete(qcStatement * aStatement)
{
    qmmDelParseTree    * sParseTree;
    qmsTableRef        * sTableRef;
    qmsTableRef        * sDeleteTableRef;
    qcmTableInfo       * sDeleteTableInfo;
    idBool               sTriggerExist = ID_FALSE;
    qmsParseTree       * sViewParseTree = NULL;
    mtcTemplate        * sMtcTemplate;
    qcmViewReadOnly      sReadOnly = QCM_VIEW_NON_READ_ONLY;
    mtcTuple           * sMtcTuple;

    /* BUG-46124 */
    qmsTarget       * sTarget;
    qmsTarget       * sReturnTarget;
    qmsSFWGH        * sViewSFWGH;

    IDU_FIT_POINT_FATAL( "qmv::validateDelete::__FT__" );

    sParseTree = (qmmDelParseTree*) aStatement->myPlan->parseTree;
    sTableRef  = sParseTree->querySet->SFWGH->from->tableRef;

    qtc::dependencyClear( & sParseTree->querySet->SFWGH->depInfo );

    // check existence of table and get table META Info.
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_VIEW_CREATION_FALSE);

    // PROJ-2204 join update, delete
    // updatable view에 사용되는 SFWGH임을 표시한다.
    if ( sTableRef->view != NULL )
    {
        sViewParseTree = (qmsParseTree*) sTableRef->view->myPlan->parseTree;

        if ( sViewParseTree->querySet->SFWGH != NULL )
        {
            sViewParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
            sViewParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
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

    IDE_TEST( qmvQuerySet::validateQmsTableRef(
                  aStatement,
                  sParseTree->querySet->SFWGH,
                  sTableRef,
                  sParseTree->querySet->SFWGH->lflag,
                  MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS);

    // Table Map 설정
    QC_SHARED_TMPLATE(aStatement)->tableMap[sTableRef->table].from =
        sParseTree->querySet->SFWGH->from;

    //-------------------------------
    // To Fix PR-7786
    //-------------------------------

    // From 절에 dependency 설정
    qtc::dependencyClear( & sParseTree->querySet->SFWGH->from->depInfo );
    qtc::dependencySet( sTableRef->table,
                        & sParseTree->querySet->SFWGH->from->depInfo );

    // Query Set에 dependency 설정
    qtc::dependencyClear( & sParseTree->querySet->depInfo );
    IDE_TEST( qtc::dependencyOr( & sParseTree->querySet->depInfo,
                                 & sParseTree->querySet->SFWGH->depInfo,
                                 & sParseTree->querySet->depInfo )
              != IDE_SUCCESS );

    qtc::dependencySet( sTableRef->table,
                        & sParseTree->querySet->SFWGH->depInfo );

    /******************************
     * PROJ-2204 Join Update, Delete
     ******************************/

    /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
    IDE_TEST( checkInsteadOfTrigger( sTableRef,
                                     QCM_TRIGGER_EVENT_DELETE,
                                     &sTriggerExist ) );

    /* PROJ-1888 INSTEAD OF TRIGGER */
    if ( sTriggerExist == ID_TRUE )
    {
        sParseTree->insteadOfTrigger = ID_TRUE;

        sParseTree->deleteTableRef = sTableRef;
    }
    else
    {
        sParseTree->insteadOfTrigger = ID_FALSE;

        sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

        // created view, inline view
        if (( ( sMtcTemplate->rows[sTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_TRUE ) &&
            ( sParseTree->querySet->SFWGH->from->tableRef->tableType != QCM_PERFORMANCE_VIEW ))
        {
            sDeleteTableRef = NULL;
            sViewSFWGH      = sViewParseTree->querySet->SFWGH;  /* BUG-46124 */
            sTarget         = sViewParseTree->querySet->target; /* BUG-46124 */

            /* BUG-49136 */
            IDE_TEST_RAISE( sViewSFWGH == NULL, ERR_NOT_KEY_PRESERVED_TABLE );

            // view의 from절에서 첫번째 key preseved table을 얻어온다.
            if ( sViewParseTree->querySet->SFWGH != NULL )
            {
                // RID방식을 사용하도록 설정한다.
                sViewParseTree->querySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
                sViewParseTree->querySet->SFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;

                IDE_TEST( qmsPreservedTable::getFirstKeyPrevTable(
                              sViewParseTree->querySet->SFWGH,
                              & sDeleteTableRef )
                          != IDE_SUCCESS );

                /* BUG-46124 */
                IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                               sViewSFWGH,
                                                                               sViewSFWGH->from,
                                                                               sTarget,
                                                                               & sReturnTarget )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            // error delete할 테이블이 없음
            IDE_TEST_RAISE( sDeleteTableRef == NULL,
                            ERR_NOT_KEY_PRESERVED_TABLE );

            /* BUG-39399 remove search key preserved table  */
            sMtcTuple = &QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sDeleteTableRef->table];

            /* BUG-46124 */
            IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                              != MTC_TUPLE_VIEW_FALSE ) ||
                            ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                              != MTC_TUPLE_KEY_PRESERVED_TRUE ),
                            ERR_NOT_KEY_PRESERVED_TABLE );

            sMtcTuple->lflag &= ~MTC_TUPLE_TARGET_UPDATE_DELETE_MASK;
            sMtcTuple->lflag |= MTC_TUPLE_TARGET_UPDATE_DELETE_TRUE;
            
            /* BUG-36350 Updatable Join DML WITH READ ONLY*/
            if ( sParseTree->querySet->SFWGH->from->tableRef->tableInfo->tableID != 0 )
            {
                /* view read only */
                IDE_TEST( qcmView::getReadOnlyOfViews(
                              QC_SMI_STMT( aStatement ),
                              sParseTree->querySet->SFWGH->from->tableRef->tableInfo->tableID,
                              &sReadOnly )
                          != IDE_SUCCESS);
            }
            else
            {
                /* Nothing To Do */
            }

            /* read only insert error */
            IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY,
                            ERR_NOT_DNL_READ_ONLY_VIEW );

            sParseTree->deleteTableRef = sDeleteTableRef;
        }
        else
        {
            sParseTree->deleteTableRef = sTableRef;
        }
    }

    sDeleteTableRef = sParseTree->deleteTableRef;
    sDeleteTableInfo = sDeleteTableRef->tableInfo;

    // BUG-17409
    sDeleteTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
    sDeleteTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

    /* TASK-7307 DML Data Consistency in Shard */
    IDE_TEST( checkUsableTable( aStatement,
                                sDeleteTableInfo ) != IDE_SUCCESS );

    IDE_TEST( checkDeleteOperatable( aStatement,
                                     sParseTree->insteadOfTrigger,
                                     sDeleteTableRef )
              != IDE_SUCCESS );

    /* PROJ-2211 Materialized View */
    if ( sDeleteTableInfo->tableType == QCM_MVIEW_TABLE )
    {
        IDE_TEST_RAISE( sParseTree->querySet->SFWGH->hints == NULL,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );

        IDE_TEST_RAISE( sParseTree->querySet->SFWGH->hints->refreshMView != ID_TRUE,
                        ERR_NO_GRANT_DML_MVIEW_TABLE );
    }
    else
    {
        /* Nothing to do */
    }
    
    // check grant
    IDE_TEST( qdpRole::checkDMLDeleteTablePriv( aStatement,
                                                sDeleteTableInfo->tableHandle,
                                                sDeleteTableInfo->tableOwnerID,
                                                sDeleteTableInfo->privilegeCount,
                                                sDeleteTableInfo->privilegeInfo,
                                                ID_FALSE,
                                                NULL,
                                                NULL )
              != IDE_SUCCESS );

    // environment의 기록
    IDE_TEST( qcgPlan::registerPlanPrivTable( aStatement,
                                              QCM_PRIV_ID_OBJECT_DELETE_NO,
                                              sDeleteTableInfo )
              != IDE_SUCCESS );

    // validation of Hints
    IDE_TEST(qmvQuerySet::validateHints(aStatement, sParseTree->querySet->SFWGH)
             != IDE_SUCCESS);

    /* PROJ-1888 INSTEAD OF TRIGGER */
    sParseTree->querySet->SFWGH->validatePhase = QMS_VALIDATE_TARGET;
    IDE_TEST( qmvQuerySet::validateQmsTarget( aStatement,
                                              sParseTree->querySet,
                                              sParseTree->querySet->SFWGH )
              != IDE_SUCCESS );

    // target 생성
    sParseTree->querySet->target = sParseTree->querySet->SFWGH->target;

    sParseTree->querySet->SFWGH->validatePhase = QMS_VALIDATE_WHERE;

    // validation of WHERE clause
    if (sParseTree->querySet->SFWGH->where != NULL)
    {
        sParseTree->querySet->processPhase = QMS_VALIDATE_WHERE;

        IDE_TEST(
            qmvQuerySet::validateWhere(
                aStatement,
                NULL, // querySet : SELECT 구문만 querySet 필요
                sParseTree->querySet->SFWGH )
            != IDE_SUCCESS);
    }

    // PROJ-1436
    if ( sParseTree->querySet->SFWGH->hints != NULL )
    {
        IDE_TEST( validatePlanHints( aStatement,
                                     sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1584 DML Return Clause */
    if( sParseTree->returnInto != NULL )
    {
        IDE_TEST( validateReturnInto( aStatement,
                                      sParseTree->returnInto,
                                      sParseTree->querySet->SFWGH,
                                      NULL,
                                      ID_FALSE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // To Fix PR-12917
    // validation of LIMIT clause
    if( sParseTree->limit != NULL )
    {
        IDE_TEST( validateLimit( aStatement, sParseTree->limit )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST(getChildInfoList(aStatement,
                              sDeleteTableInfo,
                              &(sParseTree->childConstraints))
             != IDE_SUCCESS);

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 처리시 포린키 정보수집
    //---------------------------------------------

    IDE_TEST( setFetchColumnInfo4ChildTable(
                  aStatement,
                  sDeleteTableRef )
              != IDE_SUCCESS );

    // PROJ-2205 DML trigger에 의한 컬럼참조
    IDE_TEST( setFetchColumnInfo4Trigger(
                  aStatement,
                  sDeleteTableRef )
              != IDE_SUCCESS );

    /*
     * BUG-39441
     * need a interface which returns whether DML on replication table or not
     */
    if (sDeleteTableInfo->replicationCount > 0)
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_FALSE;
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DML_MVIEW_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GRANT_DML_PRIV_OF_MVIEW_TABLE ) );
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

IDE_RC qmv::parseDelete( qcStatement * aStatement )
{
    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseDeleteInternal( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::parseDeleteInternal(qcStatement * aStatement)
{
    qmmDelParseTree    * sParseTree;

    IDU_FIT_POINT_FATAL( "qmv::parseDelete::__FT__" );

    sParseTree = (qmmDelParseTree*) aStatement->myPlan->parseTree;

    /* PROJ-1888 INSTEAD OF TRIGGER */
    // FROM clause
    if ( sParseTree->querySet->SFWGH->from != NULL )
    {
        IDE_TEST( parseViewInFromClause( aStatement,
                                         sParseTree->querySet->SFWGH->from,
                                         sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );
    }

    // WHERE clause
    if (sParseTree->querySet->SFWGH->where != NULL)
    {
        IDE_TEST( parseViewInExpression( aStatement,
                                         sParseTree->querySet->SFWGH->where )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::parseUpdate( qcStatement * aStatement )
{
    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseUpdateInternal( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::parseUpdateInternal(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    UPDATE ... SET ... 의 parse 수행
 *
 * Implementation :
 *    1. check existence of table and get table META Info.
 *    2. check table type, 뷰이면 에러
 *    3. check grant
 *    4. parse VIEW in WHERE clause
 *    5. parse VIEW in SET clause
 *    6. validation of SET clause
 *       1. check column existence
 *       2. 테이블에 이중화가 걸려있고, 변경하려는 컬럼이 primary key 이면 에러
 *    7. set 에 같은 컬럼이 있으면 에러
 *
 ***********************************************************************/

    qmmUptParseTree * sParseTree;
    qmsTableRef     * sTableRef;
    qmsTableRef     * sUpdateTableRef = NULL;
    qcmTableInfo    * sTableInfo;
    qcmTableInfo    * sUpdateTableInfo;
    qcmColumn       * sColumn;
    qcmColumn       * sOtherColumn;
    qcmColumn       * sColumnInfo;
    qcmColumn       * sDefaultExprColumn;
    qmmValueNode    * sCurrValue;
    qmmSubqueries   * sCurrSubq;
    UInt              sColumnID;
    qcmIndex        * sPrimary;
    UInt              i;
    UInt              sIterator;
    UInt              sFlag = 0;
    idBool            sTimestampExistInSetClause = ID_FALSE;
    idBool            sTriggerExist = ID_FALSE;
    qcuSqlSourceInfo  sqlInfo;

    qmsParseTree    * sViewParseTree = NULL;
    qcmColumn       * sUpdateColumn;
    mtcTemplate     * sMtcTemplate;
    qmsTarget       * sTarget;
    mtcTuple        * sMtcTuple;
    mtcColumn       * sMtcColumn;
    UShort            sUpdateTupleId = ID_USHORT_MAX;
    qcmViewReadOnly   sReadOnly = QCM_VIEW_NON_READ_ONLY;

    /* BUG-46124 */
    qmsTarget       * sReturnTarget;
    qmsSFWGH        * sViewSFWGH;

    IDU_FIT_POINT_FATAL( "qmv::parseUpdate::__FT__" );

    sParseTree = (qmmUptParseTree*) aStatement->myPlan->parseTree;
    sTableRef  = sParseTree->querySet->SFWGH->from->tableRef;

    qtc::dependencyClear( & sParseTree->querySet->SFWGH->depInfo );

    /* PROJ-1888 INSTEAD OF TRIGGER */
    // FROM clause
    if ( sParseTree->querySet->SFWGH->from != NULL )
    {
        IDE_TEST( parseViewInFromClause( aStatement,
                                         sParseTree->querySet->SFWGH->from,
                                         sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );
    }

    // check existence of table and get table META Info.
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_VIEW_CREATION_FALSE);

    /* PROJ-2204 join update, delete
     * updatable view에 사용되는 SFWGH임을 표시한다. */
    if ( sTableRef->view != NULL )
    {
        sViewParseTree = (qmsParseTree*) sTableRef->view->myPlan->parseTree;

        if ( sViewParseTree->querySet->SFWGH != NULL )
        {
            sViewParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
            sViewParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
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

    IDE_TEST(qmvQuerySet::validateQmsTableRef(aStatement,
                                              sParseTree->querySet->SFWGH,
                                              sTableRef,
                                              sParseTree->querySet->SFWGH->lflag,
                                              MTC_COLUMN_NOTNULL_TRUE) // PR-13597
             != IDE_SUCCESS);

    // Table Map 설정
    QC_SHARED_TMPLATE(aStatement)->tableMap[sTableRef->table].from =
        sParseTree->querySet->SFWGH->from;

    //-------------------------------
    // To Fix PR-7786
    //-------------------------------

    // From 절에 dependency 설정
    qtc::dependencyClear( & sParseTree->querySet->SFWGH->from->depInfo );
    qtc::dependencySet( sTableRef->table,
                        & sParseTree->querySet->SFWGH->from->depInfo );

    // Query Set에 dependency 설정
    qtc::dependencyClear( & sParseTree->querySet->depInfo );
    IDE_TEST( qtc::dependencyOr( & sParseTree->querySet->depInfo,
                                 & sParseTree->querySet->SFWGH->depInfo,
                                 & sParseTree->querySet->depInfo )
              != IDE_SUCCESS );

    qtc::dependencySet( sTableRef->table,
                        & sParseTree->querySet->SFWGH->depInfo );

    sTableInfo = sTableRef->tableInfo;

    // parse VIEW in WHERE clause
    if (sParseTree->querySet->SFWGH->where != NULL)
    {
        IDE_TEST( parseViewInExpression( aStatement,
                                         sParseTree->querySet->SFWGH->where )
                  != IDE_SUCCESS);
    }

    // parse VIEW in SET clause
    for (sCurrValue = sParseTree->values;
         sCurrValue != NULL;
         sCurrValue = sCurrValue->next)
    {
        if (sCurrValue->value != NULL)
        {
            IDE_TEST(parseViewInExpression(aStatement, sCurrValue->value)
                     != IDE_SUCCESS);
        }
    }

    // parse VIEW in SET clause
    for (sCurrSubq = sParseTree->subqueries;
         sCurrSubq != NULL;
         sCurrSubq = sCurrSubq->next)
    {
        if (sCurrSubq->subquery != NULL)
        {
            IDE_TEST(parseViewInExpression(aStatement, sCurrSubq->subquery)
                     != IDE_SUCCESS);
        }
    }

    // BUG-46174
    if ( sParseTree->spVariable != NULL )
    {
        IDE_TEST( parseSPVariableValue( aStatement,
                                        sParseTree->spVariable,
                                        &(sParseTree->values) )
                  != IDE_SUCCESS);
    }

    // BUG-46174
    if ( sParseTree->columns == NULL )
    {
        // SET ROW = 인 경우
        IDE_TEST_RAISE( ( ( aStatement->spvEnv->createProc == NULL ) &&
                          ( aStatement->spvEnv->createPkg  == NULL ) &&
                          ( aStatement->calledByPSMFlag == ID_FALSE ) ),
                        ERR_NOT_SUPPORTED );

        sUpdateTableRef = sTableRef;
        sUpdateTableInfo = sUpdateTableRef->tableInfo;

        sParseTree->uptColCount = sUpdateTableInfo->columnCount;
        sParseTree->columns = sUpdateTableInfo->columns;

        for ( sColumn = sParseTree->columns,
              sCurrValue = sParseTree->values;
              sColumn != NULL;
              sColumn = sColumn->next,
              sCurrValue = sCurrValue->next )
        {
            IDE_TEST_RAISE(sCurrValue == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);
        }

        IDE_TEST_RAISE(sCurrValue != NULL, ERR_TOO_MANY_INSERT_VALUES);
    }
    else
    {
        // validation of SET clause
        // if this statement pass parser,
        //      then nummber of column = the number of value.
        sParseTree->uptColCount = 0;
        for (sColumn = sParseTree->columns;
             sColumn != NULL;
             sColumn = sColumn->next)
        {
            sParseTree->uptColCount++;

            //--------- validation of updating column ---------//
            // check column existence
            if ( (QC_IS_NULL_NAME(sColumn->tableNamePos) != ID_TRUE) )
            {
                if ( idlOS::strMatch( sTableRef->aliasName.stmtText + sTableRef->aliasName.offset,
                                      sTableRef->aliasName.size,
                                      sColumn->tableNamePos.stmtText + sColumn->tableNamePos.offset,
                                      sColumn->tableNamePos.size ) != 0 )
                {
                    sqlInfo.setSourceInfo( aStatement, &sColumn->namePos );
                    IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                }
            }

            IDE_TEST(qcmCache::getColumn(aStatement,
                                         sTableInfo,
                                         sColumn->namePos,
                                         &sColumnInfo) != IDE_SUCCESS);

            QMV_SET_QCM_COLUMN(sColumn, sColumnInfo);
        }
    }

    /******************************
     * PROJ-2204 Join Update, Delete
     ******************************/

    /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
    IDE_TEST( checkInsteadOfTrigger( sTableRef,
                                     QCM_TRIGGER_EVENT_UPDATE,
                                     &sTriggerExist ) );

    /* PROJ-1888 INSTEAD OF TRIGGER */
    if ( sTriggerExist == ID_TRUE )
    {
        sParseTree->insteadOfTrigger = ID_TRUE;

        sParseTree->updateTableRef = sTableRef;
        sParseTree->updateColumns = sParseTree->columns;
    }
    else
    {
        sParseTree->insteadOfTrigger = ID_FALSE;

        sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

        // created view, inline view
        if (( ( sMtcTemplate->rows[sTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_TRUE ) &&
            ( sParseTree->querySet->SFWGH->from->tableRef->tableType != QCM_PERFORMANCE_VIEW ))
        {
            // update할 태이블이 없음
            IDE_TEST_RAISE( sViewParseTree->querySet->SFWGH == NULL,
                            ERR_TABLE_NOT_FOUND );

            sViewSFWGH = sViewParseTree->querySet->SFWGH; /* BUG-46124 */

            // RID방식을 사용하도록 설정한다.
            sViewParseTree->querySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
            sViewParseTree->querySet->SFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;

            // make column
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                          ID_SIZEOF(qcmColumn) * sParseTree->uptColCount,
                          (void **) & sParseTree->updateColumns )
                      != IDE_SUCCESS );

            for (sColumn = sParseTree->columns,
                     sUpdateColumn = sParseTree->updateColumns;
                 sColumn != NULL;
                 sColumn = sColumn->next,
                     sUpdateColumn++ )
            {
                // find target column
                for ( sColumnInfo = sTableInfo->columns,
                          sTarget = sViewParseTree->querySet->target;
                      sColumnInfo != NULL;
                      sColumnInfo = sColumnInfo->next,
                          sTarget = sTarget->next )
                {
                    if ( sColumnInfo->basicInfo->column.id == sColumn->basicInfo->column.id )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                /* BUG-46124 */
                IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                               sViewSFWGH,
                                                                               sViewSFWGH->from,
                                                                               sTarget,
                                                                               & sReturnTarget )
                          != IDE_SUCCESS );

                // key-preserved column 검사
                sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
                sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

                IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                  != MTC_TUPLE_VIEW_FALSE ) ||
                                ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                  != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                                ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                                  != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                                ERR_NOT_KEY_PRESERVED_TABLE );

                /* BUG-39399 remove search key preserved table  */
                sMtcTuple->lflag &= ~MTC_TUPLE_TARGET_UPDATE_DELETE_MASK;
                sMtcTuple->lflag |= MTC_TUPLE_TARGET_UPDATE_DELETE_TRUE;
                
                if( sUpdateTupleId == ID_USHORT_MAX )
                {
                    // first
                    sUpdateTupleId = sReturnTarget->targetColumn->node.baseTable;
                }
                else
                {
                    // update할 태이블이 여러개임
                    IDE_TEST_RAISE( sUpdateTupleId != sReturnTarget->targetColumn->node.baseTable,
                                    ERR_NOT_ONE_BASE_TABLE );
                }

                sUpdateTableRef =
                    QC_SHARED_TMPLATE(aStatement)->tableMap[sUpdateTupleId].from->tableRef;

                sColumnInfo = sUpdateTableRef->tableInfo->columns + sReturnTarget->targetColumn->node.baseColumn;

                // copy column
                idlOS::memcpy( sUpdateColumn, sColumn, ID_SIZEOF(qcmColumn) );
                QMV_SET_QCM_COLUMN(sUpdateColumn, sColumnInfo);

                if ( sUpdateColumn->next != NULL )
                {
                    sUpdateColumn->next = sUpdateColumn + 1;
                }
                else
                {
                    // Nothing to do.
                }
            }

            /* BUG-36350 Updatable Join DML WITH READ ONLY*/
            if ( sParseTree->querySet->SFWGH->from->tableRef->tableInfo->tableID != 0 )
            {
                /* view read only */
                IDE_TEST( qcmView::getReadOnlyOfViews(
                              QC_SMI_STMT( aStatement ),
                              sParseTree->querySet->SFWGH->from->tableRef->tableInfo->tableID,
                              &sReadOnly )
                          != IDE_SUCCESS);
            }
            else
            {
                /* Nothing To Do */
            }

            /* read only update error */
            IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY,
                            ERR_NOT_DNL_READ_ONLY_VIEW );

            sParseTree->updateTableRef = sUpdateTableRef;
        }
        else
        {
            sParseTree->updateTableRef = sTableRef;
            sParseTree->updateColumns = sParseTree->columns;
        }
    }

    sUpdateTableRef = sParseTree->updateTableRef;
    sUpdateTableInfo = sUpdateTableRef->tableInfo;

    // PROJ-2219 Row-level before update trigger
    // Row-level before update trigger에서 참조하는 column을 update column에 추가한다.
    if ( sUpdateTableInfo->triggerCount > 0 )
    {
        IDE_TEST( makeNewUpdateColumnList( aStatement ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-17409
    sUpdateTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
    sUpdateTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

    for (sColumn = sParseTree->updateColumns;
         sColumn != NULL;
         sColumn = sColumn->next)
    {
        /* PROJ-1090 Function-based Index */
        if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
             == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(sColumn->namePos) );
            IDE_RAISE( ERR_CANNOT_USE_HIDDEN_COLUMN );
        }
        else
        {
            /* Nothing to do */
        }

        // If a table has timestamp column
        if ( sUpdateTableInfo->timestamp != NULL )
        {
            if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                 == MTC_COLUMN_TIMESTAMP_TRUE )
            {
                sTimestampExistInSetClause = ID_TRUE;
            }
        }
    }

    // If a table has timestamp column
    //  and there is no specified timestamp column in SET clause,
    //  then make the updating timestamp column internally.
    if ( ( sUpdateTableInfo->timestamp != NULL ) &&
         ( sTimestampExistInSetClause == ID_FALSE ) )
    {
        // make column
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sColumn)
                 != IDE_SUCCESS);

        IDE_TEST(qcmCache::getColumnByID(
                     sUpdateTableInfo,
                     sUpdateTableInfo->timestamp->constraintColumn[0],
                     &sColumnInfo )
                 != IDE_SUCCESS);

        QMV_SET_QCM_COLUMN(sColumn, sColumnInfo);

        // connect value list
        sColumn->next       = sParseTree->updateColumns;
        sParseTree->updateColumns = sColumn;
        sParseTree->uptColCount++;

        // make value
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue)
                 != IDE_SUCCESS);

        sCurrValue->value     = NULL;
        sCurrValue->validate  = ID_TRUE;
        sCurrValue->calculate = ID_TRUE;
        sCurrValue->timestamp = ID_FALSE;
        sCurrValue->msgID     = ID_FALSE;
        sCurrValue->expand    = ID_FALSE;

        // connect value list
        sCurrValue->next   = sParseTree->values;
        sParseTree->values = sCurrValue;
    }

    /* PROJ-1090 Function-based Index */
    for ( sColumn = sParseTree->updateColumns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        IDE_TEST( qmsDefaultExpr::addDefaultExpressionColumnsRelatedToColumn(
                      aStatement,
                      &(sParseTree->defaultExprColumns),
                      sUpdateTableInfo,
                      sColumn->basicInfo->column.id )
                  != IDE_SUCCESS );
    }

    /* PROJ-1090 Function-based Index */
    for ( sDefaultExprColumn = sParseTree->defaultExprColumns;
          sDefaultExprColumn != NULL;
          sDefaultExprColumn = sDefaultExprColumn->next )
    {
        // make column
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qcmColumn, &sColumn )
                  != IDE_SUCCESS );
        QMV_SET_QCM_COLUMN( sColumn, sDefaultExprColumn );

        // connect value list
        sColumn->next       = sParseTree->updateColumns;
        sParseTree->updateColumns = sColumn;
        sParseTree->uptColCount++;

        // make value
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue )
                  != IDE_SUCCESS );

        sCurrValue->value     = NULL;
        sCurrValue->validate  = ID_TRUE;
        sCurrValue->calculate = ID_TRUE;
        sCurrValue->timestamp = ID_FALSE;
        sCurrValue->msgID     = ID_FALSE;
        sCurrValue->expand    = ID_FALSE;

        // connect value list
        sCurrValue->next   = sParseTree->values;
        sParseTree->values = sCurrValue;
    }

    for (sColumn     = sParseTree->updateColumns,
             sCurrValue  = sParseTree->values,
             sIterator   = 0;
         sColumn    != NULL &&
             sCurrValue != NULL;
         sColumn     = sColumn->next,
             sCurrValue  = sCurrValue->next,
             sIterator++ )
    {
        // set order of values
        sCurrValue->order = sIterator;

        // if updating column is primary key and updating tables is replicated,
        //      then error.
        if (sUpdateTableInfo->replicationCount > 0)
        {
            sColumnID = sColumn->basicInfo->column.id;
            sPrimary  = sUpdateTableInfo->primaryKey;

            for (i = 0; i < sPrimary->keyColCount; i++)
            {
                // To fix BUG-14325
                // replication이 걸려있는 table에 대해 pk update여부 검사.
                if( QCU_REPLICATION_UPDATE_PK == 0 )
                {
                    IDE_TEST_RAISE(
                        sPrimary->keyColumns[i].column.id == sColumnID,
                        ERR_NOT_ALLOW_PK_UPDATE);
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        //--------- validation of updating value  ---------//
        if( sCurrValue->value == NULL && sCurrValue->calculate == ID_TRUE )
        {
            IDE_TEST( setDefaultOrNULL( aStatement, sUpdateTableInfo, sColumn, sCurrValue )
                      != IDE_SUCCESS);

            if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                 == MTC_COLUMN_TIMESTAMP_TRUE )
            {
                sCurrValue->timestamp = ID_TRUE;
            }
        }
    }

    /* PROJ-1107 Check Constraint 지원 */
    for ( sColumn = sParseTree->updateColumns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        IDE_TEST( qdnCheck::addCheckConstrSpecRelatedToColumn(
                      aStatement,
                      &(sParseTree->checkConstrList),
                      sUpdateTableInfo->checks,
                      sUpdateTableInfo->checkCount,
                      sColumn->basicInfo->column.id )
                  != IDE_SUCCESS );
    }

    /* PROJ-1107 Check Constraint 지원 */
    IDE_TEST( qdnCheck::setMtcColumnToCheckConstrList(
                  aStatement,
                  sUpdateTableInfo,
                  sParseTree->checkConstrList )
              != IDE_SUCCESS );

    if ( sParseTree->defaultExprColumns != NULL )
    {
        sFlag &= ~QMV_PERFORMANCE_VIEW_CREATION_MASK;
        sFlag |=  QMV_PERFORMANCE_VIEW_CREATION_FALSE;
        sFlag &= ~QMV_VIEW_CREATION_MASK;
        sFlag |=  QMV_VIEW_CREATION_FALSE;

        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmsTableRef, &sParseTree->defaultTableRef )
                  != IDE_SUCCESS );

        QCP_SET_INIT_QMS_TABLE_REF( sParseTree->defaultTableRef );

        sParseTree->defaultTableRef->userName.stmtText = sUpdateTableInfo->tableOwnerName;
        sParseTree->defaultTableRef->userName.offset   = 0;
        sParseTree->defaultTableRef->userName.size     = idlOS::strlen(sUpdateTableInfo->tableOwnerName);

        sParseTree->defaultTableRef->tableName.stmtText = sUpdateTableInfo->name;
        sParseTree->defaultTableRef->tableName.offset   = 0;
        sParseTree->defaultTableRef->tableName.size     = idlOS::strlen(sUpdateTableInfo->name);

        /* BUG-17409 */
        sParseTree->defaultTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
        sParseTree->defaultTableRef->flag |=  QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

        IDE_TEST( qmvQuerySet::validateQmsTableRef( aStatement,
                                                    NULL,
                                                    sParseTree->defaultTableRef,
                                                    sFlag,
                                                    MTC_COLUMN_NOTNULL_TRUE ) /* PR-13597 */
                  != IDE_SUCCESS );

        /*
         * PROJ-1090, PROJ-2429
         * Variable column, Compressed column을
         * Fixed Column으로 변환한 TableRef를 만든다.
         */
        IDE_TEST( qtc::nextTable( &(sParseTree->defaultTableRef->table),
                                  aStatement,
                                  NULL,     /* Tuple ID만 얻는다. */
                                  QCM_TABLE_TYPE_IS_DISK( sParseTree->defaultTableRef->tableInfo->tableFlag ),
                                  MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                  != IDE_SUCCESS );

        IDE_TEST( qmvQuerySet::makeTupleForInlineView(
                      aStatement,
                      sParseTree->defaultTableRef,
                      sParseTree->defaultTableRef->table,
                      MTC_COLUMN_NOTNULL_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // check same(duplicated) column in set clause.
    for (sColumn = sParseTree->updateColumns;
         sColumn != NULL;
         sColumn = sColumn->next)
    {
        for (sOtherColumn = sParseTree->updateColumns; sOtherColumn != NULL;
             sOtherColumn = sOtherColumn->next)
        {
            if (sColumn != sOtherColumn)
            {
                IDE_TEST_RAISE(sColumn->basicInfo == sOtherColumn->basicInfo,
                               ERR_DUP_SET_CLAUSE);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_USE_HIDDEN_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_DUP_SET_CLAUSE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_DUP_COLUMN_IN_SET));
    }
    IDE_EXCEPTION(ERR_NOT_ALLOW_PK_UPDATE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOW_PRIMARY_KEY_UPDATE));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION(ERR_NOT_ONE_BASE_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ONE_BASE_TABLE));
    }
    IDE_EXCEPTION(ERR_TABLE_NOT_FOUND);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TABLE_NOT_FOUND));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ENOUGH_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TOO_MANY_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORTED);
    {
        sqlInfo.setSourceInfo( aStatement, &sParseTree->columns->namePos );
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateUpdate(qcStatement * aStatement)
{
    qmmUptParseTree   * sParseTree;
    qcmTableInfo      * sUpdateTableInfo;
    qcmColumn         * sCurrColumn;
    qmmValueNode      * sCurrValue;
    qmmSubqueries     * sSubquery;
    qmmValuePointer   * sValuePointer;
    qmsTarget         * sTarget;
    qcStatement       * sStatement;
    qcuSqlSourceInfo    sqlInfo;
    const mtdModule  ** sModules;
    const mtdModule   * sLocatorModule;
    qmsTableRef       * sUpdateTableRef;
    qcmColumn         * sQcmColumn;
    mtcColumn         * sMtcColumn;
    smiColumnList     * sCurrSmiColumn;
    qdConstraintSpec  * sConstr;
    qmsFrom             sFrom;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmv::validateUpdate::__FT__" );

    sParseTree = (qmmUptParseTree*)aStatement->myPlan->parseTree;

    sUpdateTableRef  = sParseTree->updateTableRef;
    sUpdateTableInfo = sUpdateTableRef->tableInfo;

    /* TASK-7307 DML Data Consistency in Shard */
    IDE_TEST( checkUsableTable( aStatement,
                                sUpdateTableInfo ) != IDE_SUCCESS );

    // PR-13725
    // CHECK OPERATABLE
    IDE_TEST( checkUpdateOperatable( aStatement,
                                     sParseTree->insteadOfTrigger,
                                     sUpdateTableRef )
              != IDE_SUCCESS );
    
    // check grant
    IDE_TEST( qdpRole::checkDMLUpdateTablePriv( aStatement,
                                                sUpdateTableInfo->tableHandle,
                                                sUpdateTableInfo->tableOwnerID,
                                                sUpdateTableInfo->privilegeCount,
                                                sUpdateTableInfo->privilegeInfo,
                                                ID_FALSE,
                                                NULL,
                                                NULL )
              != IDE_SUCCESS );

    // environment의 기록
    IDE_TEST( qcgPlan::registerPlanPrivTable( aStatement,
                                              QCM_PRIV_ID_OBJECT_UPDATE_NO,
                                              sUpdateTableInfo )
              != IDE_SUCCESS );

    // PROJ-2219 Row-level before update trigger
    // Update column을 column ID순으로 정렬한다.
    IDE_TEST( sortUpdateColumn( aStatement,
                                &sParseTree->updateColumns,
                                sParseTree->uptColCount,
                                &sParseTree->values,
                                NULL )
              != IDE_SUCCESS );

    IDE_TEST( makeUpdateRow( aStatement ) != IDE_SUCCESS );

    // Update Column List 생성
    if ( sParseTree->uptColCount > 0 )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(smiColumnList) * sParseTree->uptColCount,
                      (void **) & sParseTree->uptColumnList )
                  != IDE_SUCCESS );

        for ( sCurrColumn = sParseTree->updateColumns,
                  sCurrSmiColumn = sParseTree->uptColumnList,
                  i = 0;
              sCurrColumn != NULL;
              sCurrColumn = sCurrColumn->next,
                  sCurrSmiColumn++,
                  i++ )
        {
            // smiColumnList 정보 설정
            sCurrSmiColumn->column = & sCurrColumn->basicInfo->column;

            if ( i + 1 < sParseTree->uptColCount )
            {
                sCurrSmiColumn->next = sCurrSmiColumn + 1;
            }
            else
            {
                sCurrSmiColumn->next = NULL;
            }
        }
    }
    else
    {
        sParseTree->uptColumnList = NULL;
    }

    // PROJ-1473
    // validation of Hints
    IDE_TEST( qmvQuerySet::validateHints(aStatement,
                                         sParseTree->querySet->SFWGH)
              != IDE_SUCCESS );

    // PROJ-1784 DML Without Retry
    sParseTree->querySet->processPhase = QMS_VALIDATE_SET;

    for( sSubquery = sParseTree->subqueries;
         sSubquery != NULL;
         sSubquery = sSubquery->next )
    {
        if( sSubquery->subquery != NULL)
        {
            IDE_TEST(qtc::estimate( sSubquery->subquery,
                                    QC_SHARED_TMPLATE(aStatement),
                                    aStatement,
                                    NULL,
                                    sParseTree->querySet->SFWGH,
                                    NULL )
                     != IDE_SUCCESS);

            if ( ( sSubquery->subquery->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sSubquery->subquery->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }

            if ( QTC_HAVE_AGGREGATE( sSubquery->subquery ) == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sSubquery->subquery->position );
                IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
            }

            sStatement = sSubquery->subquery->subquery;
            sTarget = ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->target;
            for( sValuePointer = sSubquery->valuePointer;
                 sValuePointer != NULL && sTarget != NULL;
                 sValuePointer = sValuePointer->next,
                     sTarget       = sTarget->next )
            {
                sValuePointer->valueNode->value = sTarget->targetColumn;
            }

            IDE_TEST_RAISE( sValuePointer != NULL || sTarget != NULL,
                            ERR_INVALID_FUNCTION_ARGUMENT );
        }
    }

    for( sCurrValue = sParseTree->lists;
         sCurrValue != NULL;
         sCurrValue = sCurrValue->next )
    {
        if (sCurrValue->value != NULL)
        {
            /* PROJ-2197 PSM Renewal */
            sCurrValue->value->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

            IDE_TEST( notAllowedAnalyticFunc( aStatement, sCurrValue->value )
                      != IDE_SUCCESS );

            if ( qtc::estimate( sCurrValue->value,
                                QC_SHARED_TMPLATE(aStatement),
                                aStatement,
                                NULL,
                                sParseTree->querySet->SFWGH,
                                NULL )
                 != IDE_SUCCESS )
            {
                // default value인 경우 별도 에러처리
                IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                == QTC_NODE_DEFAULT_VALUE_TRUE,
                                ERR_INVALID_DEFAULT_VALUE );

                // default value가 아닌 경우 에러pass
                IDE_TEST( 1 );
            }
            else
            {
                // Nothing to do.
            }

            if ( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrValue->value->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }

            if ( QTC_HAVE_AGGREGATE( sCurrValue->value ) == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrValue->value->position );
                IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
            }
        }
    }

    sModules = sParseTree->columnModules;

    for (sCurrColumn = sParseTree->updateColumns,
             sCurrValue = sParseTree->values;
         sCurrValue != NULL;
         sCurrColumn = sCurrColumn->next,
             sCurrValue = sCurrValue->next,
             sModules++)
    {
        if (sCurrValue->value != NULL)
        {
            if( sCurrValue->validate == ID_TRUE )
            {
                /* PROJ-2197 PSM Renewal */
                sCurrValue->value->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

                IDE_TEST( notAllowedAnalyticFunc( aStatement, sCurrValue->value )
                          != IDE_SUCCESS );

                if ( qtc::estimate( sCurrValue->value,
                                    QC_SHARED_TMPLATE(aStatement),
                                    aStatement,
                                    NULL,
                                    sParseTree->querySet->SFWGH,
                                    NULL )
                     != IDE_SUCCESS )
                {
                    // default value인 경우 별도 에러처리
                    IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                    == QTC_NODE_DEFAULT_VALUE_TRUE,
                                    ERR_INVALID_DEFAULT_VALUE );

                    // default value가 아닌 경우 에러pass
                    IDE_TEST( 1 );
                }
                else
                {
                    // Nothing to do.
                }

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                if ( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                     == MTC_NODE_DML_UNUSABLE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrValue->value->position );
                    IDE_RAISE( ERR_USE_CURSOR_ATTR );
                }

                if ( QTC_HAVE_AGGREGATE( sCurrValue->value ) == ID_TRUE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrValue->value->position );
                    IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
                }
            }

            // PROJ-2002 Column Security
            // update t1 set i1=i2 같은 경우 동일 암호 타입이라도 policy가
            // 다를 수 있으므로 i2에 decrypt func을 생성한다.
            sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );

            if ( (sMtcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                 == MTD_ENCRYPT_TYPE_TRUE )
            {
                // default policy의 암호 타입이라도 decrypt 함수를 생성하여
                // subquery의 결과는 항상 암호 타입이 나올 수 없게 한다.

                // add decrypt func
                IDE_TEST( addDecryptFunc4ValueNode( aStatement,
                                                    sCurrValue )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );

            // PROJ-1362
            // add Lob-Locator function
            if ( (sCurrValue->value->node.module == & qtc::columnModule) &&
                 ((sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_LOB) )
            {
                // BUG-33890
                if( ( (*sModules)->id == MTD_BLOB_ID ) ||
                    ( (*sModules)->id == MTD_CLOB_ID ) )
                {
                    IDE_TEST( mtf::getLobFuncResultModule( &sLocatorModule,
                                                           *sModules )
                              != IDE_SUCCESS );

                    IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                       aStatement,
                                                       QC_SHARED_TMPLATE(aStatement),
                                                       sLocatorModule )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( qtc::makeConversionNode(sCurrValue->value,
                                                      aStatement,
                                                      QC_SHARED_TMPLATE(aStatement),
                                                      *sModules )
                              != IDE_SUCCESS);
                }
            }
            else
            {
                IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                   aStatement,
                                                   QC_SHARED_TMPLATE(aStatement),
                                                   *sModules )
                          != IDE_SUCCESS );
            }

            // PROJ-1502 PARTITIONED DISK TABLE
            IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                              QC_SHARED_TMPLATE(aStatement),
                                              aStatement )
                      != IDE_SUCCESS );

            // BUG-15746
            IDE_TEST( describeParamInfo( aStatement,
                                         sCurrColumn->basicInfo,
                                         sCurrValue->value )
                      != IDE_SUCCESS );
        }
    }

    sParseTree->querySet->SFWGH->validatePhase = QMS_VALIDATE_WHERE;

    // validation of WHERE clause
    if (sParseTree->querySet->SFWGH->where != NULL)
    {
        sParseTree->querySet->processPhase = QMS_VALIDATE_WHERE;

        IDE_TEST(
            qmvQuerySet::validateWhere(
                aStatement,
                NULL, // querySet : SELECT 구문만 필요
                sParseTree->querySet->SFWGH )
            != IDE_SUCCESS);
    }

    // PROJ-1436
    if ( sParseTree->querySet->SFWGH->hints != NULL )
    {
        IDE_TEST( validatePlanHints( aStatement,
                                     sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1584 DML Return Clause */
    if( sParseTree->returnInto != NULL )
    {
        IDE_TEST( validateReturnInto( aStatement,
                                      sParseTree->returnInto,
                                      sParseTree->querySet->SFWGH,
                                      NULL,
                                      ID_FALSE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // To Fix PR-12917
    // validation of LIMIT clause
    if( sParseTree->limit != NULL )
    {
        IDE_TEST( validateLimit( aStatement, sParseTree->limit )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1107 Check Constraint 지원 */
    for ( sConstr = sParseTree->checkConstrList;
          sConstr != NULL;
          sConstr = sConstr->next )
    {
        IDE_TEST( qdbCommon::validateCheckConstrDefinition(
                      aStatement,
                      sConstr,
                      sParseTree->querySet->SFWGH,
                      NULL )
                  != IDE_SUCCESS );
    }

    /* PROJ-1090 Function-based Index */
    if ( sParseTree->defaultExprColumns != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sParseTree->defaultTableRef;

        for ( sQcmColumn = sParseTree->defaultExprColumns;
              sQcmColumn != NULL;
              sQcmColumn = sQcmColumn->next)
        {
            IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                          aStatement,
                          sQcmColumn->defaultValue,
                          sParseTree->querySet->SFWGH,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( getParentInfoList( aStatement,
                                 sUpdateTableInfo,
                                 &(sParseTree->parentConstraints),
                                 sParseTree->updateColumns,
                                 sParseTree->uptColCount )
              != IDE_SUCCESS);

    IDE_TEST( getChildInfoList( aStatement,
                                sUpdateTableInfo,
                                &(sParseTree->childConstraints),
                                sParseTree->updateColumns,
                                sParseTree->uptColCount )
              != IDE_SUCCESS);

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 처리시 포린키 정보수집
    //---------------------------------------------

    IDE_TEST( setFetchColumnInfo4ParentTable(
                  aStatement,
                  sUpdateTableRef )
              != IDE_SUCCESS );

    IDE_TEST( setFetchColumnInfo4ChildTable(
                  aStatement,
                  sUpdateTableRef )
              != IDE_SUCCESS );

    // PROJ-2205 DML trigger에 의한 컬럼참조
    IDE_TEST( setFetchColumnInfo4Trigger(
                  aStatement,
                  sUpdateTableRef )
              != IDE_SUCCESS );

    /*
     * BUG-39441
     * need a interface which returns whether DML on replication table or not
     */
    if (sUpdateTableInfo->replicationCount > 0)
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_FALSE;
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_AGGREGATION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_AGGREGATION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_FUNCTION_ARGUMENT);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION( ERR_INVALID_DEFAULT_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateLockTable(qcStatement * aStatement)
{
    qmmLockParseTree * sParseTree;
    qcuSqlSourceInfo   sqlInfo;
    idBool             sExist = ID_FALSE;
    qcmSynonymInfo     sSynonymInfo;
    UInt               sOperatableFlag = 0;
    UInt               sTableID = 0;
    qcmTableType       sTableType = QCM_USER_TABLE;
    
    IDU_FIT_POINT_FATAL( "qmv::validateLockTable::__FT__" );

    sParseTree = (qmmLockParseTree*) aStatement->myPlan->parseTree;

    IDE_TEST(
        qcmSynonym::resolveTableViewQueue(
            aStatement,
            sParseTree->userName,
            sParseTree->tableName,
            &(sParseTree->tableInfo),
            &(sParseTree->userID),
            &(sParseTree->tableSCN),
            &sExist,
            &sSynonymInfo,
            &(sParseTree->tableHandle))
        != IDE_SUCCESS);

    if (sExist == ID_FALSE)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->tableName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }

    // BUG-46284
    if ( ( smiGetTableFlag( sParseTree->tableHandle ) & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_FIXED )
    {
        // QCM_FIXED_TABLE, QCM_DUMP_TABLE, QCM_PERFORMANCE_VIEW
        sTableType = QCM_FIXED_TABLE;
    }
    else
    {
        if( sSynonymInfo.isSynonymName == ID_TRUE )
        {
            IDE_TEST( qcm::getTableIDAndTypeByName( QC_SMI_STMT( aStatement ),
                                                    sParseTree->userID,
                                                    (UChar *)sSynonymInfo.objectName,
                                                    (SInt)idlOS::strlen( sSynonymInfo.objectName ),
                                                    &sTableID,
                                                    &sTableType )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qcm::getTableIDAndTypeByName( QC_SMI_STMT( aStatement ),
                                                    sParseTree->userID,
                                                    (UChar *)(sParseTree->tableName.stmtText +
                                                              sParseTree->tableName.offset),
                                                    sParseTree->tableName.size,
                                                    &sTableID,
                                                    &sTableType )
                      != IDE_SUCCESS );

        }

    }

    qcm::setOperatableFlag( sTableType,
                            &sOperatableFlag );

    // BUG-45366
    if ( QCM_IS_OPERATABLE_QP_LOCK_TABLE( sOperatableFlag )
         != ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->tableName) );
        IDE_RAISE(ERR_NOT_EXIST_TABLE);
    }
    else
    {
        /* Nothing to do */
    }
    
    // check grant
    IDE_TEST( qdpRole::checkDMLLockTablePriv( aStatement,
                                              sParseTree->userID )
              != IDE_SUCCESS );

    /* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
    if ( sParseTree->untilNextDDL == ID_TRUE )
    {
        /* Lock Escalation으로 인한 Deadlock 문제를 방지하기 위해, EXCLUSIVE MODE만 지원한다. */
        IDE_TEST_RAISE( sParseTree->tableLockMode != SMI_TABLE_LOCK_X,
                        ERR_MUST_LOCK_TABLE_UNTIL_NEXT_DDL_IN_EXCLUSIVE_MODE );
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-46273 Lock Partition */
    if ( QC_IS_NULL_NAME( sParseTree->partitionName ) == ID_FALSE )
    {
        IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                  sTableID,
                                                  sParseTree->partitionName,
                                                  & sParseTree->partitionInfo,
                                                  & sParseTree->partitionSCN,
                                                  & sParseTree->partitionHandle )
                  != IDE_SUCCESS );
    }
    else
    {
        sParseTree->partitionInfo   = NULL;
        sParseTree->partitionHandle = NULL;
        SMI_INIT_SCN( & sParseTree->partitionSCN );
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    // Execution 단계에서 메타에 접근하므로 MEMORY_CURSOR Flag를 항상 켠다.
    QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_MUST_LOCK_TABLE_UNTIL_NEXT_DDL_IN_EXCLUSIVE_MODE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_CANNOT_LOCK_TABLE_UNTIL_NEXT_DDL_IN_NON_EXCLUSIVE_MODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::detectDollarTables(qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *  fixedTable또는 performanceView의 출현을 감지.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree      * sParseTree;
    qmsSortColumns    * sCurrSort;

    IDU_FIT_POINT_FATAL( "qmv::detectDollarTables::__FT__" );

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

    IDE_TEST(detectDollarInQuerySet(aStatement,
                                    sParseTree->querySet)
             != IDE_SUCCESS);

    for (sCurrSort = sParseTree->orderBy;
         sCurrSort != NULL;
         sCurrSort = sCurrSort->next)
    {
        if (sCurrSort->targetPosition <= QMV_EMPTY_TARGET_POSITION)
        {
            IDE_TEST(detectDollarInExpression(aStatement,
                                              sCurrSort->sortColumn)
                     != IDE_SUCCESS);
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::detectDollarInQuerySet(qcStatement * aStatement,
                                   qmsQuerySet * aQuerySet )
{
/***********************************************************************
 *
 * Description :
 *  fixedTable또는 performanceView의 출현을 감지.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsSFWGH                * sSFWGH;
    qmsFrom                 * sFrom;
    qmsTarget               * sTarget;
    qmsConcatElement        * sConcatElement;

    IDU_FIT_POINT_FATAL( "qmv::detectDollarInQuerySet::__FT__" );

    if (aQuerySet->setOp == QMS_NONE)
    {
        sSFWGH =  aQuerySet->SFWGH;

        // FROM clause
        for (sFrom = sSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
        {
            IDE_TEST(detectDollarInFromClause(aStatement, sFrom) != IDE_SUCCESS);
        }

        // SELECT target clause
        for (sTarget = sSFWGH->target;
             sTarget != NULL;
             sTarget = sTarget->next)
        {
            if ( ( sTarget->flag & QMS_TARGET_ASTERISK_MASK )
                 != QMS_TARGET_ASTERISK_TRUE )
            {
                IDE_TEST(detectDollarInExpression( aStatement,
                                                   sTarget->targetColumn )
                         != IDE_SUCCESS);
            }
        }

        // WHERE clause
        if (sSFWGH->where != NULL)
        {
            IDE_TEST(detectDollarInExpression(aStatement, sSFWGH->where)
                     != IDE_SUCCESS);
        }

        // hierarchical clause
        if (sSFWGH->hierarchy != NULL)
        {
            if (sSFWGH->hierarchy->startWith != NULL)
            {
                IDE_TEST(detectDollarInExpression(aStatement,
                                                  sSFWGH->hierarchy->startWith)
                         != IDE_SUCCESS);
            }

            if (sSFWGH->hierarchy->connectBy != NULL)
            {
                IDE_TEST(detectDollarInExpression(aStatement,
                                                  sSFWGH->hierarchy->connectBy)
                         != IDE_SUCCESS);
            }
        }

        // GROUP BY clause
        for( sConcatElement = sSFWGH->group;
             sConcatElement != NULL;
             sConcatElement = sConcatElement->next )
        {
            if( sConcatElement->arithmeticOrList != NULL )
            {
                IDE_TEST( detectDollarInExpression( aStatement,
                                                    sConcatElement->arithmeticOrList )
                          != IDE_SUCCESS);
            }
            else
            {
                // Nothing To Do
            }
        }

        // HAVING clause
        if (sSFWGH->having != NULL)
        {
            IDE_TEST(detectDollarInExpression(aStatement, sSFWGH->having)
                     != IDE_SUCCESS);
        }
    }
    else // UNION, UNION ALL, INTERSECT, MINUS
    {
        IDE_TEST(detectDollarInQuerySet(aStatement, aQuerySet->left)
                 != IDE_SUCCESS);

        IDE_TEST(detectDollarInQuerySet(aStatement, aQuerySet->right)
                 != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::detectDollarInExpression( qcStatement     * aStatement,
                                      qtcNode         * aExpression)
{
/***********************************************************************
 *
 * Description :
 *  fixedTable또는 performanceView의 출현을 감지.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode        * sNode;

    IDU_FIT_POINT_FATAL( "qmv::detectDollarInExpression::__FT__" );

    if ( ( aExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        IDE_TEST(detectDollarTables(aExpression->subquery) != IDE_SUCCESS);
    }
    else
    {
        // traverse child
        for (sNode = (qtcNode *)(aExpression->node.arguments);
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST(detectDollarInExpression(aStatement, sNode)
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::detectDollarInFromClause(
    qcStatement     * aStatement,
    qmsFrom         * aFrom)
{
/***********************************************************************
 *
 * Description :
 *  fixedTable또는 performanceView의 출현을 감지.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsTableRef       * sTableRef;

    IDU_FIT_POINT_FATAL( "qmv::detectDollarInFromClause::__FT__" );
    
    if (aFrom->joinType != QMS_NO_JOIN) // INNER, OUTER JOIN
    {
        IDE_TEST(detectDollarInFromClause(aStatement, aFrom->left)
                 != IDE_SUCCESS);

        IDE_TEST(detectDollarInFromClause(aStatement, aFrom->right)
                 != IDE_SUCCESS);

        // ON condition
        IDE_TEST(detectDollarInExpression(aStatement, aFrom->onCondition)
                 != IDE_SUCCESS);
    }
    else
    {
        sTableRef = aFrom->tableRef;

        if (sTableRef->view == NULL)
        {

            IDE_TEST( qcmFixedTable::validateTableName( aStatement,
                                                        sTableRef->tableName )
                      != IDE_SUCCESS );
        }
        else
        {
            // in case of "select * from (select * from v1)"
            IDE_TEST(detectDollarTables( sTableRef->view ) != IDE_SUCCESS);
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qmv::parseSelect(qcStatement * aStatement)
{
    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseSelectInternal( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::parseSelectInternal( qcStatement * aStatement )
{
    qmsParseTree      * sParseTree;
    qmsSortColumns    * sCurrSort;
    qcStmtListMgr     * sStmtListMgr;        /* BUG-45994 - 컴파일러 최적화 회피 */
    qcWithStmt        * sBackupWithStmtHead; /* BUG-45994 */

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmv::parseSelect::__FT__" );

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

    /* PROJ-2206 validate with clause */
    sStmtListMgr        = aStatement->myPlan->stmtListMgr;
    sBackupWithStmtHead = sStmtListMgr->head;

    IDU_FIT_POINT_FATAL( "qmv::parseSelect::__FT__::STAGE1" );

    IDE_TEST( qmvWith::validate( aStatement )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseViewInQuerySet( aStatement,
                                   sParseTree,
                                   sParseTree->querySet )
              != IDE_SUCCESS );

    for (sCurrSort = sParseTree->orderBy;
         sCurrSort != NULL;
         sCurrSort = sCurrSort->next)
    {
        if (sCurrSort->targetPosition <= QMV_EMPTY_TARGET_POSITION)
        {
            IDE_TEST(parseViewInExpression(aStatement, sCurrSort->sortColumn)
                     != IDE_SUCCESS);
        }
    }

    sStmtListMgr->head = sBackupWithStmtHead;

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    sStmtListMgr->head = sBackupWithStmtHead;

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;

}

IDE_RC qmv::parseViewInQuerySet( qcStatement  * aStatement,
                                 qmsParseTree * aParseTree, /* TASK-7219 Shard Transformer Refactoring */
                                 qmsQuerySet  * aQuerySet )
{
    qmsSFWGH         * sSFWGH;
    qmsFrom          * sFrom;
    qmsTarget        * sTarget;
    qmsConcatElement * sConcatElement;
    idBool             sIsChanged = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmv::parseViewInQuerySet::__FT__" );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( sdi::makeAndSetQuerySetList( aStatement,
                                           aQuerySet )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setQuerySetListState( aStatement,
                                         aParseTree,
                                         &( sIsChanged ) )
              != IDE_SUCCESS );

    if (aQuerySet->setOp == QMS_NONE)
    {
        sSFWGH =  aQuerySet->SFWGH;

        // PROJ-2415 Grouping Sets Clause
        if ( sSFWGH->group != NULL )
        {
            /* BUG-46702 */
            IDE_TEST( convertWithRoullupToRollup( aStatement, sSFWGH ) != IDE_SUCCESS );

            IDE_TEST( qmvGBGSTransform::doTransform( aStatement, aQuerySet ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }

        // FROM clause
        for (sFrom = sSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
        {
            IDE_TEST(parseViewInFromClause(aStatement, sFrom, sSFWGH->hints) != IDE_SUCCESS);
        }

        // SELECT target clause
        for (sTarget = sSFWGH->target;
             sTarget != NULL;
             sTarget = sTarget->next)
        {
            if ( ( sTarget->flag & QMS_TARGET_ASTERISK_MASK )
                 != QMS_TARGET_ASTERISK_TRUE )
            {
                IDE_TEST(parseViewInExpression( aStatement,
                                                sTarget->targetColumn )
                         != IDE_SUCCESS);
            }
        }

        // WHERE clause
        if (sSFWGH->where != NULL)
        {
            IDE_TEST(parseViewInExpression(aStatement, sSFWGH->where)
                     != IDE_SUCCESS);
        }

        // hierarchical clause
        if (sSFWGH->hierarchy != NULL)
        {
            if (sSFWGH->hierarchy->startWith != NULL)
            {
                IDE_TEST(parseViewInExpression(aStatement,
                                               sSFWGH->hierarchy->startWith)
                         != IDE_SUCCESS);
            }

            if (sSFWGH->hierarchy->connectBy != NULL)
            {
                IDE_TEST(parseViewInExpression(aStatement,
                                               sSFWGH->hierarchy->connectBy)
                         != IDE_SUCCESS);
            }
        }

        // GROUP BY clause
        for( sConcatElement = sSFWGH->group;
             sConcatElement != NULL;
             sConcatElement = sConcatElement->next )
        {
            if( sConcatElement->arithmeticOrList != NULL )
            {
                IDE_TEST( parseViewInExpression( aStatement,
                                                 sConcatElement->arithmeticOrList )
                          != IDE_SUCCESS);
            }
            else
            {
                // Nothing To Do
            }
        }

        // HAVING clause
        if (sSFWGH->having != NULL)
        {
            IDE_TEST(parseViewInExpression(aStatement, sSFWGH->having)
                     != IDE_SUCCESS);
        }
    }
    else // UNION, UNION ALL, INTERSECT, MINUS
    {
        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( parseViewInQuerySet( aStatement,
                                       aParseTree,
                                       aQuerySet->left )
              != IDE_SUCCESS );

        IDE_TEST( parseViewInQuerySet( aStatement,
                                       aParseTree,
                                       aQuerySet->right )
                  != IDE_SUCCESS );
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( sdi::unsetQuerySetListState( aStatement,
                                           sIsChanged )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::parseViewInFromClause(
    qcStatement     * aStatement,
    qmsFrom         * aFrom,
    qmsHints        * aHints )
{
    qmsTableRef       * sTableRef;
    volatile UInt       sSessionUserID;   // for fixing BUG-6096
    qcuSqlSourceInfo    sqlInfo;
    idBool              sExist = ID_FALSE;
    idBool              sIsFixedTable = ID_FALSE;
    volatile idBool     sIndirectRef;
    void              * sTableHandle = NULL;
    qcmSynonymInfo      sSynonymInfo;
    UInt                sTableType;
    qmsPivotAggr      * sPivotNode = NULL;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmv::parseViewInFromClause::__FT__" );

    // To Fix PR-11776
    sSessionUserID = QCG_GET_SESSION_USER_ID( aStatement );
    sIndirectRef   = ID_FALSE; /* BUG-45994 - 컴파일러 최적화 회피 */

    if (aFrom->joinType != QMS_NO_JOIN) // INNER, OUTER JOIN
    {
        IDE_TEST( parseViewInFromClause(aStatement, aFrom->left, aHints )
                  != IDE_SUCCESS);

        IDE_TEST( parseViewInFromClause(aStatement, aFrom->right, aHints )
                  != IDE_SUCCESS);

        // ON condition
        IDE_TEST(parseViewInExpression(aStatement, aFrom->onCondition)
                 != IDE_SUCCESS);
    }
    else
    {
        sTableRef = aFrom->tableRef;

        /* PROJ-1832 New database link */
        IDE_TEST_CONT( sTableRef->remoteTable != NULL, NORMAL_EXIT );

        /* PROJ-2206 : with stmt list로 view를 생성한다. */
        if ( sTableRef->view == NULL )
        {
            IDE_TEST( qmvWith::parseViewInTableRef( aStatement,
                                                    sTableRef )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        if(sTableRef->view == NULL)
        {
            // PROJ-2582 recursive with
            if ( ( sTableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                 == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
            {
                if ( sTableRef->recursiveView != NULL )
                {
                    /* TASK-7219 Shard Transformer Refactoring
                     *  최상위 recursive view
                     */
                    IDE_TEST( parseSelectInternal( sTableRef->recursiveView )
                              != IDE_SUCCESS );

                    IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                             sTableRef->recursiveView->mFlag )
                              != IDE_SUCCESS );

                    IDE_TEST( parseSelectInternal( sTableRef->tempRecursiveView )
                              != IDE_SUCCESS );

                }
                else
                {
                    // 하위 recursive view
                    // Nothing to do.
                }
            }
            else
            {
                if (QC_IS_NULL_NAME(sTableRef->userName) == ID_TRUE)
                {
                    sTableRef->userID = QCG_GET_SESSION_USER_ID(aStatement);
                }
                else
                {
                    IDE_TEST( qcmUser::getUserID(aStatement,
                                                 sTableRef->userName,
                                                 &(sTableRef->userID))
                              != IDE_SUCCESS );
                }

                IDE_TEST(
                    qcmSynonym::resolveTableViewQueue(
                        aStatement,
                        sTableRef->userName,
                        sTableRef->tableName,
                        &(sTableRef->tableInfo),
                        &(sTableRef->userID),
                        &(sTableRef->tableSCN),
                        &sExist,
                        &sSynonymInfo,
                        &sTableHandle)
                    != IDE_SUCCESS);

                if( sExist == ID_TRUE )
                {
                    sTableType = smiGetTableFlag( sTableHandle ) & SMI_TABLE_TYPE_MASK;

                    if( sTableType == SMI_TABLE_FIXED )
                    {
                        sIsFixedTable = ID_TRUE;
                    }
                    else
                    {
                        sIsFixedTable = ID_FALSE;
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if ( sExist == ID_TRUE )
                {
                    if ( sIsFixedTable == ID_TRUE )
                    {
                        if ( sTableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW )
                        {
                            // set flag
                            sTableRef->flag &= ~QMS_TABLE_REF_CREATED_VIEW_MASK;
                            sTableRef->flag |= QMS_TABLE_REF_CREATED_VIEW_TRUE;

                            // check status of VIEW
                            if ( sTableRef->tableInfo->status != QCM_VIEW_VALID)
                            {
                                (void)sqlInfo.setSourceInfo( aStatement,
                                                             &sTableRef->tableName );
                                IDE_RAISE( ERR_INVALID_VIEW );
                            }
                            else
                            {
                                // nothing to do
                            }

                            QCG_SET_SESSION_USER_ID( aStatement,
                                                     sTableRef->userID );

                            // PROJ-1436
                            // environment의 기록시 간접 참조 객체에 대한 user나
                            // privilege의 기록을 중지한다.
                            qcgPlan::startIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );

                            /* BUG-45994 */
                            IDU_FIT_POINT_FATAL( "qmv::parseViewInFromClause::__FT__::STAGE1" );

                            // If view is valid, then make parse tree
                            IDE_TEST(
                                qcmPerformanceView::makeParseTreeForViewInSelect(
                                    aStatement,
                                    sTableRef )
                                != IDE_SUCCESS);

                            /* TASK-7219 Shard Transformer Refactoring
                             *  for case of "select * from v1" and
                             *   v1 has another view.
                             */
                            IDE_TEST( parseSelectInternal( sTableRef->view )
                                      != IDE_SUCCESS );

                            IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                                     sTableRef->view->mFlag )
                                      != IDE_SUCCESS );

                            // for fixing BUG-6096
                            // re-set current session userID
                            QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

                            qcgPlan::endIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );
                        }
                        else
                        {
                            // nothing to do
                        }
                    }
                    else
                    {
                        // BUG-34492
                        // validation lock이면 충분하다.
                        IDE_TEST(qcm::lockTableForDMLValidation(
                                     aStatement,
                                     sTableHandle,
                                     sTableRef->tableSCN)
                                 != IDE_SUCCESS);

                        // PROJ-2646 shard analyzer enhancement
                        if ( ( sdi::isShardCoordinator( aStatement ) == ID_TRUE ) ||
                             ( sdi::isPartialCoordinator( aStatement ) == ID_TRUE ) ||
                             ( sdi::detectShardMetaChange( aStatement ) == ID_TRUE ) )
                        {
                            IDE_TEST( sdi::getTableInfo( aStatement,
                                                         sTableRef->tableInfo,
                                                         &(sTableRef->mShardObjInfo) )
                                      != IDE_SUCCESS );

                            if ( sTableRef->mShardObjInfo != NULL )
                            {
                                aStatement->mFlag &= ~QC_STMT_SHARD_OBJ_MASK;
                                aStatement->mFlag |= QC_STMT_SHARD_OBJ_EXIST;
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

                        // environment의 기록
                        IDE_TEST( qcgPlan::registerPlanTable(
                                      aStatement,
                                      sTableHandle,
                                      sTableRef->tableSCN,
                                      sTableRef->tableInfo->tableOwnerID, /* BUG-45893 */
                                      sTableRef->tableInfo->name )        /* BUG-45893 */
                                  != IDE_SUCCESS );

                        // environment의 기록
                        IDE_TEST( qcgPlan::registerPlanSynonym(
                                      aStatement,
                                      & sSynonymInfo,
                                      sTableRef->userName,
                                      sTableRef->tableName,
                                      sTableHandle,
                                      NULL )
                                  != IDE_SUCCESS );

                        if ( ( sTableRef->tableInfo->tableType == QCM_VIEW ) ||
                             ( sTableRef->tableInfo->tableType == QCM_MVIEW_VIEW ) )
                        {
                            // set flag
                            sTableRef->flag &= ~QMS_TABLE_REF_CREATED_VIEW_MASK;
                            sTableRef->flag |= QMS_TABLE_REF_CREATED_VIEW_TRUE;

                            // check status of VIEW
                            if (sTableRef->tableInfo->status != QCM_VIEW_VALID)
                            {
                                sqlInfo.setSourceInfo( aStatement,
                                                       & sTableRef->tableName );
                                IDE_RAISE( ERR_INVALID_VIEW );
                            }
                            else
                            {
                                /* Nothing to do */
                            }

                            // create view as select 인 경우에는 parseSelect를 재귀 호출 하지 않는다.
                            if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_PARSE_CREATE_VIEW_MASK )
                                 == QC_PARSE_CREATE_VIEW_FALSE )
                            {
                                // modify session userID
                                QCG_SET_SESSION_USER_ID( aStatement,
                                                         sTableRef->userID );
                                // PROJ-1436
                                // environment의 기록시 간접 참조 객체에 대한 user나
                                // privilege의 기록을 중지한다.
                                qcgPlan::startIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );

                                /* BUG-45994 */
                                IDU_FIT_POINT_FATAL( "qmv::parseViewInFromClause::__FT__::STAGE1" );

                                // If view is valid, then make parse tree
                                IDE_TEST(qdv::makeParseTreeForViewInSelect( aStatement,
                                                                            sTableRef )
                                         != IDE_SUCCESS);

                                /* TASK-7219 Shard Transformer Refactoring
                                 *  for case of "select * from v1" and v1 has another view.
                                 */
                                IDE_TEST( parseSelectInternal( sTableRef->view )
                                          != IDE_SUCCESS );

                                IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                                         sTableRef->view->mFlag )
                                          != IDE_SUCCESS );

                                // for fixing BUG-6096
                                // re-set current session userID
                                QCG_SET_SESSION_USER_ID( aStatement,
                                                         sSessionUserID );

                                qcgPlan::endIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );
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
                    }
                }
                else
                {
                    // nothing to do
                }
            }

            // For validation select, sTableRef->tableInfo must be NULL.
            sTableRef->tableInfo = NULL;
        }
        else
        {
            IDE_DASSERT( sTableRef->recursiveView == NULL );

            /* TASK-7219 Shard Transformer Refactoring
             *  in case of "select * from (select * from v1)"
             */
            IDE_TEST( parseSelectInternal( sTableRef->view )
                      != IDE_SUCCESS );

            // BUG-45443
            // 다음과 같이 node를 명시적으로 사용한 경우는 shard object가 있는 것으로 한다.
            // select * from node[data1](select * from dual);
            if ( sTableRef->view->myPlan->parseTree->stmtShard != QC_STMT_SHARD_NONE )
            {
                aStatement->mFlag |= QC_STMT_SHARD_OBJ_EXIST;

                /* TASK-7219 Shard Transformer Refactoring */
                if ( ( sTableRef->view->mFlag & QC_STMT_SHARD_KEYWORD_MASK )
                     == QC_STMT_SHARD_KEYWORD_NO_USE )
                {
                    aStatement->mFlag |= QC_STMT_SHARD_KEYWORD_USE;                    
                }
                else
                {
                    aStatement->mFlag |= QC_STMT_SHARD_KEYWORD_DUPLICATE;
                }
            }
            else
            {
                /* TASK-7219 Shard Transformer Refactoring */
                IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                         sTableRef->view->mFlag )
                          != IDE_SUCCESS );
            }
        }

        /* BUG-48544 Pivot 구문 집계 함수에 Inline View Alias, With Alias 인 Coulmn 또는 Subquery 를 사용하는 경우 오류가 발생합니다. */
        if ( sTableRef->pivot != NULL )
        {
            for ( sPivotNode  = sTableRef->pivot->aggrNodes;
                  sPivotNode != NULL;
                  sPivotNode  = sPivotNode->next )
            {
                if ( sPivotNode->node != NULL )
                {
                    IDE_TEST( parseViewInExpression( aStatement,
                                                     sPivotNode->node )
                              != IDE_SUCCESS );
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

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION(ERR_INVALID_VIEW);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDV_INVALID_VIEW,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    // To Fix PR-11776
    // Parsing 단계에서 Error 발생 시
    // User ID를 바로 잡아야 함.
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    qcgPlan::endIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;

}

IDE_RC qmv::parseViewInExpression(
    qcStatement     * aStatement,
    qtcNode         * aExpression)
{
    qcStatement    * sSubQStatement;
    qtcNode        * sNode;

    volatile SInt    sStackRemain;
    qtcOverColumn  * sOverColumn = NULL;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmv::parseViewInExpression::__FT__" );

    sStackRemain = ID_SINT_MAX; /* BUG-45994 - 컴파일러 최적화 회피 */

    // BUG-44367
    // prevent thread stack overflow
    sStackRemain = QC_SHARED_TMPLATE(aStatement)->tmplate.stackRemain;
    QC_SHARED_TMPLATE(aStatement)->tmplate.stackRemain--;

    IDE_TEST_RAISE( sStackRemain < 1, ERR_STACK_OVERFLOW );

    /* BUG-45944 */
    IDU_FIT_POINT_FATAL( "qmv::parseViewInExpression::__FT__::STAGE1" );

    if ( ( aExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        sSubQStatement = aExpression->subquery;

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( parseSelectInternal( sSubQStatement )
                  != IDE_SUCCESS );

        IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                 sSubQStatement->mFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        // traverse child
        for (sNode = (qtcNode *)(aExpression->node.arguments);
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST(parseViewInExpression(aStatement, sNode)
                     != IDE_SUCCESS);
        }

        /* TASK-7219 Analyzer/Transformer/Executor 성능개선
         * Over clause의 parseView 수행 
         */
        if ( aExpression->overClause != NULL )
        {
            for ( sOverColumn  = aExpression->overClause->overColumn;
                  sOverColumn != NULL;
                  sOverColumn  = sOverColumn->next )
            {
                IDE_TEST( parseViewInExpression( aStatement,
                                                 sOverColumn->node )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }

    QC_SHARED_TMPLATE(aStatement)->tmplate.stackRemain = sStackRemain;

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    if ( sStackRemain != ID_SINT_MAX )
    {
        QC_SHARED_TMPLATE(aStatement)->tmplate.stackRemain = sStackRemain;
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;

}

IDE_RC qmv::validateSelect(qcStatement * aStatement)
{
    qmsParseTree      * sParseTree;
    qcuSqlSourceInfo    sqlInfo;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmv::validateSelect::__FT__" );

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

    // BUG-41311
    IDE_TEST( validateLoop( aStatement ) != IDE_SUCCESS );

    // PROJ-1988 Merge Query
    // 최상위 querySet도 outerQuery를 가질 수 있다.
    // PROJ-2415 Grouping Sets Clause
    // Grouping Sts Transform으로 생성된 View는 outerQuery를 가질  수 있다.

    // validate (SELECT ... UNION SELECT ... INTERSECT ... )
    IDE_TEST( qmvQuerySet::validate(aStatement,
                                    sParseTree->querySet,
                                    sParseTree->querySet->outerSFWGH,
                                    NULL,
                                    0)
              != IDE_SUCCESS );

    // validate ORDER BY clause
    if (sParseTree->orderBy != NULL)
    {
        IDE_TEST( qmvOrderBy::validate(aStatement) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if( sParseTree->limit != NULL )
    {
        IDE_TEST( validateLimit( aStatement, sParseTree->limit ) != IDE_SUCCESS );

        /* BUG-36580 supported TOP */
        if( sParseTree->querySet->setOp == QMS_NONE )
        {
            if( sParseTree->querySet->SFWGH->top != NULL )
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sParseTree->limit->limitPos );
                IDE_RAISE( ERR_DUPLICATE_LIMIT_OPTION );
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
    }

    if (sParseTree->common.currValSeqs != NULL ||
        sParseTree->common.nextValSeqs != NULL)
    {
        // check SEQUENCE allowed position
        if (sParseTree->querySet->setOp == QMS_NONE)
        {
            // check GROUP BY
            if (sParseTree->querySet->SFWGH->group != NULL ||
                sParseTree->querySet->SFWGH->having != NULL ||
                sParseTree->querySet->SFWGH->aggsDepth1 != NULL)
            {
                if (sParseTree->common.currValSeqs != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sParseTree->common.currValSeqs->
                        sequenceNode->position );
                    IDE_RAISE( ERR_USE_SEQUENCE_WITH_GROUP_BY );
                }

                if (sParseTree->common.nextValSeqs != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sParseTree->common.nextValSeqs->
                        sequenceNode->position );
                    IDE_RAISE( ERR_USE_SEQUENCE_WITH_GROUP_BY );
                }
            }

            // check DISTINCT
            if (sParseTree->querySet->SFWGH->selectType == QMS_DISTINCT)
            {
                if (sParseTree->common.currValSeqs != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sParseTree->common.currValSeqs->
                        sequenceNode->position );
                    IDE_RAISE( ERR_USE_SEQUENCE_WITH_DISTINCT );
                }

                if (sParseTree->common.nextValSeqs != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sParseTree->common.nextValSeqs->
                        sequenceNode->position );
                    IDE_RAISE( ERR_USE_SEQUENCE_WITH_DISTINCT );
                }
            }
        }
        else
        {
            // check SET operation (UNION, UNION ALL, INTERSECT, MINUS)
            if (sParseTree->common.currValSeqs != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sParseTree->common.currValSeqs->
                    sequenceNode->position );
                IDE_RAISE( ERR_USE_SEQUENCE_WITH_SET );
            }

            if (sParseTree->common.nextValSeqs != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sParseTree->common.nextValSeqs->
                    sequenceNode->position );
                IDE_RAISE( ERR_USE_SEQUENCE_WITH_SET );
            }
        }

        // check ORDER BY
        if (sParseTree->orderBy != NULL)
        {
            if (sParseTree->common.currValSeqs != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sParseTree->common.currValSeqs->
                    sequenceNode->position );
                IDE_RAISE( ERR_USE_SEQUENCE_WITH_ORDER_BY );
            }

            if (sParseTree->common.nextValSeqs != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sParseTree->common.nextValSeqs->
                    sequenceNode->position );
                IDE_RAISE( ERR_USE_SEQUENCE_WITH_ORDER_BY );
            }
        }
    }
    // Proj - 1360 Queue
    // dequeue 문에서 subquery사용 불가능
    if (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE)
    {
        if ( sParseTree->querySet->SFWGH->where != NULL)
        {
            if ( ( sParseTree->querySet->SFWGH->where->lflag &
                   QTC_NODE_SUBQUERY_MASK ) ==
                 QTC_NODE_SUBQUERY_EXIST )
            {
                IDE_RAISE( ERR_DEQUEUE_SUBQ_EXIST );
            }
        }
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION(ERR_USE_SEQUENCE_WITH_GROUP_BY)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_WITH_GROUP_BY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_SEQUENCE_WITH_DISTINCT)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_WITH_DISTINCT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_SEQUENCE_WITH_SET)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_WITH_SET,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_SEQUENCE_WITH_ORDER_BY)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_WITH_ORDER_BY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_DEQUEUE_SUBQ_EXIST)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_QUEUE_DEQUEUE_SUBQ_EXIST));
    }
    IDE_EXCEPTION(ERR_DUPLICATE_LIMIT_OPTION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    IDE_PUSH();

    /* PROJ-1832 New database link */
    (void)qmrInvalidRemoteTableMetaCache( aStatement );

    IDE_POP();

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;

}

IDE_RC qmv::validateReturnInto( qcStatement   * aStatement,
                                qmmReturnInto * aReturnInto,
                                qmsSFWGH      * aSFWGH,
                                qmsFrom       * aFrom,
                                idBool          aIsInsert )
{
/***********************************************************************
 * Description : PROJ-1584 DML Return Clause
 *               BUG-42715
 *               qmv::validateReturnInto, qsv::validateReturnInto 함수를
 *               통합하고, return value, return into value를 validate
 *               하는 함수로 분리함.
 *
 * Implementation :
 ***********************************************************************/

    UInt sReturnCnt = 0;

    IDU_FIT_POINT_FATAL( "qmv::validateReturnInto::__FT__" );

    IDE_TEST( validateReturnValue( aStatement,
                                   aReturnInto,
                                   aSFWGH,
                                   aFrom,
                                   aIsInsert,
                                   &sReturnCnt )
              != IDE_SUCCESS );

    IDE_TEST( validateReturnIntoValue( aStatement,
                                       aReturnInto,
                                       sReturnCnt )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::validateReturnValue( qcStatement   * aStatement,
                                 qmmReturnInto * aReturnInto,
                                 qmsSFWGH      * aSFWGH,
                                 qmsFrom       * aFrom,
                                 idBool          aIsInsert,
                                 UInt          * aReturnCnt )
{
    qmmReturnValue   * sReturnValue = NULL;
    qtcNode          * sReturnNode  = NULL;
    mtcColumn        * sMtcColumn;
    qtcNode          * sNode;
    qcTemplate       * sTmplate     = QC_SHARED_TMPLATE(aStatement);
    UInt               sReturnCnt   = 0;
    qcuSqlSourceInfo   sqlInfo;

    IDU_FIT_POINT_FATAL( "qmv::validateReturnValue::__FT__" );

    /* RETURNING or RETURN .... */
    for( sReturnValue = aReturnInto->returnValue;
         sReturnValue != NULL;
         sReturnValue = sReturnValue->next)
    {
        if ( ( aStatement->spvEnv->createProc != NULL ) ||
             ( aStatement->spvEnv->createPkg  != NULL ) )
        {
            sReturnValue->returnExpr->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( qtc::estimate(
                      sReturnValue->returnExpr,
                      sTmplate,
                      aStatement,
                      NULL,
                      aSFWGH,
                      aFrom )
                  != IDE_SUCCESS );

        sReturnCnt++;

        sReturnNode = sReturnValue->returnExpr;

        /* NOT SUPPORT (semantic check) */

        if ( ( sReturnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_LIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnNode->position );
            IDE_RAISE( ERR_NOT_APPLICABLE_TYPE );
        }
        else
        {
            // Nothing to do.
        }

        /*
         * LOB
         * BUGBUG: LOB을 partition DML 수행시 LOB Cursor을 알 수 없다.
         * BUG-33189   need to add a lob interface for reading
         * the latest lob column value like getLastModifiedRow
         */
        if ( ( ( aStatement->spvEnv->createProc == NULL ) &&
               ( aStatement->spvEnv->createPkg  == NULL ) ) &&
             ( aStatement->calledByPSMFlag != ID_TRUE ) &&
             ( (sReturnNode->lflag & QTC_NODE_LOB_COLUMN_MASK)
               == QTC_NODE_LOB_COLUMN_EXIST ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnNode->position );
            IDE_RAISE( ERR_NOT_SUPPORT_LOB_COLUMN );
        }
        else
        {
            // Nothing to do.
        }

        /* TASK-7219 Shard Transformer Refactoring */
        if ( ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                            SDI_QUERYSET_LIST_STATE_DUMMY_MAKE )
               == ID_TRUE )
             &&
             ( ( sReturnNode->lflag & QTC_NODE_LOB_COLUMN_MASK )
               == QTC_NODE_LOB_COLUMN_EXIST ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &( sReturnNode->position ) );

            IDE_RAISE( ERR_NOT_SUPPORT_LOB_COLUMN );
        }
        else
        {
            /* Nothing to do */
        }

        /* SEQUENCE */
        if ( ( sReturnNode->lflag & QTC_NODE_SEQUENCE_MASK )
             == QTC_NODE_SEQUENCE_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnNode->position );
            IDE_RAISE( ERR_SEQUENCE_NOT_ALLOWED );

        }
        else
        {

        }

        /* SUBQUERY */
        if ( (sReturnNode->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnNode->position );
            IDE_RAISE( ERR_NOT_SUPPORT_SUBQUERY );
        }
        else
        {
            // Nothing to do.
        }

        /* RETURN 뒤에는 HOST Variable 오면 않됨 */
        if ( ( ( aStatement->spvEnv->createProc == NULL ) &&
               ( aStatement->spvEnv->createPkg  == NULL ) ) &&
             ( aStatement->calledByPSMFlag != ID_TRUE ) &&
             ( MTC_NODE_IS_DEFINED_TYPE( & sReturnNode->node )
               == ID_FALSE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnNode->position );
            IDE_RAISE( ERR_NOT_RETURN_ALLOW_HOSTVAR );
        }
        else
        {
            // Nothing to do.
        }

        sMtcColumn = QTC_STMT_COLUMN( aStatement, sReturnNode );

        // BUG-35195
        // return절에 암호컬럼이 오는 경우 decrypt함수를 붙인다.
        if( (sMtcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
            == MTD_ENCRYPT_TYPE_TRUE )
        {
            if ( aIsInsert == ID_TRUE )
            {
                // insert에서는 지원하지 않는다.
                sqlInfo.setSourceInfo( aStatement,
                                       & sReturnNode->position );
                IDE_RAISE( ERR_NOT_APPLICABLE_TYPE );
            }
            else
            {
                // Nothing to do.
            }

            // decrypt 함수를 만든다.
            IDE_TEST( qmvQuerySet::addDecryptFuncForNode( aStatement,
                                                          sReturnNode,
                                                          & sNode )
                      != IDE_SUCCESS );

            sReturnValue->returnExpr = sNode;
        }
        else
        {
            // Nothing to do.
        }

        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
        if ( ( ( aStatement->spvEnv->createProc == NULL ) &&
               ( aStatement->spvEnv->createPkg  == NULL ) ) &&
             ( aStatement->calledByPSMFlag == ID_TRUE ) &&
             ( QTC_IS_COLUMN( aStatement, sReturnNode ) == ID_TRUE ) )
        {
            if( sMtcColumn->module->id == MTD_BLOB_ID )
            {
                /* get_blob_locator 함수를 만든다. */
                IDE_TEST( qmvQuerySet::addBLobLocatorFuncForNode( aStatement,
                                                                  sReturnNode,
                                                                  & sNode )
                          != IDE_SUCCESS );

                sReturnValue->returnExpr = sNode;
            }
            else if( sMtcColumn->module->id == MTD_CLOB_ID )
            {
                /* get_clob_locator 함수를 만든다. */
                IDE_TEST( qmvQuerySet::addCLobLocatorFuncForNode( aStatement,
                                                                  sReturnNode,
                                                                  & sNode )
                          != IDE_SUCCESS );

                sReturnValue->returnExpr = sNode;
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

    *aReturnCnt = sReturnCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_APPLICABLE_TYPE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_RETURN,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_LOB_COLUMN)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_SUPPORT_LOB_COLUMN,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_SEQUENCE_NOT_ALLOWED)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_SEQUENCE_NOT_ALLOWED,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_SUBQUERY)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_SUPPORT_SUBQUERY,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_RETURN_ALLOW_HOSTVAR );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_RETURN_ALLOWED_HOSTVAR,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmv::validateReturnIntoValue( qcStatement   * aStatement,
                                     qmmReturnInto * aReturnInto,
                                     UInt            aReturnCnt )
{
    UInt                 sIntoCnt         = 0;
    qmmReturnIntoValue * sReturnIntoValue = NULL;
    mtcColumn          * sMtcColumn;
    qcTemplate         * sTmplate;

    // BUG-42715
    idBool               sFindVar;
    qtcNode            * sCurrIntoVar     = NULL;
    qsVariables        * sArrayVariable   = NULL;
    qcmColumn          * sRowColumn       = NULL;
    qtcModule          * sRowModule       = NULL;
    qtcModule          * sQtcModule       = NULL;
    idBool               sExistsRecordVar = ID_FALSE;
    qtcNode            * sIndexNode[2];
    qcNamePosition       sNullPosition;

    qcNamePosition       sPos;
    qcuSqlSourceInfo     sqlInfo;

    IDU_FIT_POINT_FATAL( "qmv::validateReturnIntoValue::__FT__" );

    IDE_FT_ASSERT( aStatement != NULL );
    sTmplate = QC_SHARED_TMPLATE(aStatement);
    
    SET_EMPTY_POSITION( sNullPosition );

    sPos      = aReturnInto->returnIntoValue->returningInto->position;
    sPos.size = 0;

    /* INTO .... */
    for( sReturnIntoValue = aReturnInto->returnIntoValue;
         sReturnIntoValue != NULL;
         sReturnIntoValue = sReturnIntoValue->next )
    {
        sCurrIntoVar = sReturnIntoValue->returningInto;
        sCurrIntoVar->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        sFindVar = ID_FALSE;

        sPos.size = sCurrIntoVar->position.offset + sCurrIntoVar->position.size - sPos.offset;

        // fix BUG-42521
        if ( ( (sReturnIntoValue->returningInto->node.lflag & MTC_NODE_BIND_MASK) == MTC_NODE_BIND_EXIST ) &&
             ( sReturnIntoValue->returningInto->node.module == &(qtc::valueModule) ) )
        {
            // PROJ-2653
            if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                SDI_QUERYSET_LIST_STATE_DUMMY_MAKE )
                 != ID_TRUE )
            {
                aStatement->myPlan->sBindParam[sReturnIntoValue->returningInto->node.column].param.inoutType
                    = CMP_DB_PARAM_OUTPUT;
            }
            else
            {
                IDE_FT_ASSERT( aStatement->myPlan->sBindParam == NULL );
            }
        }
        else
        {
            if ( ( aStatement->spvEnv->createProc != NULL ) ||
                 ( aStatement->spvEnv->createPkg  != NULL ) )
            {
                sCurrIntoVar->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

                IDE_TEST(qsvProcVar::searchVarAndPara( aStatement,
                                                       sCurrIntoVar,
                                                       ID_TRUE, // for OUTPUT
                                                       &sFindVar,
                                                       &sArrayVariable)
                         != IDE_SUCCESS);
            }

            if( sFindVar == ID_FALSE )
            {
                // BUG-42715
                // bind 변수가 아니면 package 변수이다.
                IDE_TEST( qsvProcVar::searchVariableFromPkg(
                              aStatement,
                              sCurrIntoVar,
                              &sFindVar,
                              &sArrayVariable )
                          != IDE_SUCCESS );
            }

            if (sFindVar == ID_FALSE)
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrIntoVar->position );

                IDE_RAISE( ERR_NOT_FOUND_VAR );
            }
            else
            {
                // Nothing to do.
            }
        }

        sReturnIntoValue->returningInto->lflag &= ~QTC_NODE_LVALUE_MASK;
        sReturnIntoValue->returningInto->lflag |= QTC_NODE_LVALUE_ENABLE;

        IDE_TEST( qtc::estimate(
                      sReturnIntoValue->returningInto,
                      sTmplate,
                      aStatement,
                      NULL,
                      NULL,
                      NULL )
                  != IDE_SUCCESS );

        if ( ( (aStatement->spvEnv->createProc == NULL) &&
               (aStatement->spvEnv->createPkg  == NULL) ) &&
             (sReturnIntoValue->returningInto->node.module == &qtc::spFunctionCallModule) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &sReturnIntoValue->returningInto->position );
            IDE_RAISE( ERR_NOT_FOUND_VAR );
        }
        else
        {
            // Nothing to do.
        }

        if ( sFindVar == ID_TRUE )
        {
            if ( ( sCurrIntoVar->lflag & QTC_NODE_OUTBINDING_MASK )
                 == QTC_NODE_OUTBINDING_DISABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrIntoVar->position );

                IDE_RAISE( ERR_INOUT_TYPE_MISMATCH );
            }

            sCurrIntoVar->lflag |= QTC_NODE_LVALUE_ENABLE;

            sMtcColumn = QTC_STMT_COLUMN( aStatement,
                                          sCurrIntoVar );

            if ( aReturnInto->bulkCollect == ID_TRUE )
            {
                if ( sMtcColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
                {
                    if ( sCurrIntoVar->node.arguments == NULL )
                    {
                        IDE_TEST( qtc::makeProcVariable( aStatement,
                                                         sIndexNode,
                                                         & sNullPosition,
                                                         NULL,
                                                         QTC_PROC_VAR_OP_NEXT_COLUMN )
                                  != IDE_SUCCESS );

                        IDE_TEST( mtc::initializeColumn(
                                      sTmplate->tmplate.
                                      rows[sIndexNode[0]->node.table].columns + sIndexNode[0]->node.column,
                                      & mtdInteger,
                                      0,
                                      0,
                                      0 )
                                  != IDE_SUCCESS );

                        sIndexNode[0]->lflag &= ~QTC_NODE_COLUMN_ESTIMATE_MASK;
                        sIndexNode[0]->lflag |= QTC_NODE_COLUMN_ESTIMATE_TRUE;

                        sCurrIntoVar->lflag &= ~QTC_NODE_PROC_VAR_ESTIMATE_MASK;
                        sCurrIntoVar->lflag |= QTC_NODE_PROC_VAR_ESTIMATE_FALSE;

                        sCurrIntoVar->lflag &= ~QTC_NODE_SP_ARRAY_INDEX_VAR_MASK;
                        sCurrIntoVar->lflag |= QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST;

                        sCurrIntoVar->lflag &= ~QTC_NODE_LVALUE_MASK;
                        sCurrIntoVar->lflag |= QTC_NODE_LVALUE_ENABLE;

                        // index node를 연결한다.
                        sCurrIntoVar->node.arguments = (mtcNode*) sIndexNode[0];
                        sCurrIntoVar->node.lflag |= 1;

                        IDE_TEST( qtc::estimate( sCurrIntoVar,
                                                 sTmplate,
                                                 aStatement,
                                                 NULL,
                                                 NULL,
                                                 NULL )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sQtcModule = (qtcModule*)sMtcColumn->module;

                    sRowColumn = sQtcModule->typeInfo->columns->next;
                    sRowModule = (qtcModule*)sRowColumn->basicInfo->module;

                    if ( ( sRowModule->module.id == MTD_ROWTYPE_ID ) ||
                         ( sRowModule->module.id == MTD_RECORDTYPE_ID ) )
                    {
                        sExistsRecordVar = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // bulk collect인 경우 associative array type만 가능
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sCurrIntoVar->position );
                    IDE_RAISE( ERR_NOT_ASSOCIATIVE );
                }
            }
            else
            {
                if ( sMtcColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
                {
                    // bulk collect이 없으면 associative array type 불가
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sCurrIntoVar->position );
                    IDE_RAISE( ERR_NOT_BULK_ASSOCIATIVE );
                }
                else
                {
                    if ( ( sMtcColumn->module->id == MTD_ROWTYPE_ID ) ||
                         ( sMtcColumn->module->id == MTD_RECORDTYPE_ID ) )
                    {
                        sRowModule = (qtcModule*)sMtcColumn->module;
                        sExistsRecordVar = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }

        sIntoCnt++;
    }

    // BUG-42715
    if( sExistsRecordVar == ID_TRUE )
    {
        // record variable이 있는 경우이므로
        // targetCount와 record var의 내부컬럼 count를 체크한다.
        // 단, 이때는 intoVarCount는 반드시 1이어야만 한다.
        if( sIntoCnt != 1 )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &sPos );
            IDE_RAISE( ERR_RECORD_COUNT );
        }
        else
        {
            // Nothing to do.
        }

        if ( sRowModule != NULL )
        {
            if( sRowModule->typeInfo->columnCount != aReturnCnt )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sPos );
                IDE_RAISE( ERR_NOT_SAME_COLUMN_RETURN_INTO_WITH_RECORD_VAR );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            IDE_DASSERT( 0 );
        }
    }
    else
    {
        // record variable이 없으면 targetCount와 intoVar count를 체크한다.
        if( sIntoCnt != aReturnCnt )
        {
            IDE_RAISE( ERR_NOT_SAME_COLUMN_RETURN_INTO );
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SAME_COLUMN_RETURN_INTO_WITH_RECORD_VAR );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSV_MISMATCHED_INTO_LIST_SQLTEXT,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_SAME_COLUMN_RETURN_INTO );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_SAME_COLUMN_RETURN_INTO,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_VAR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ASSOCIATIVE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSV_ONLY_ASSOCIATIVE,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INOUT_TYPE_MISMATCH);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSV_PROC_SELECT_INTO_NO_READONLY_VAR_SQLTEXT,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_RECORD_COUNT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSV_RECORD_WRONG,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_BULK_ASSOCIATIVE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSV_BULK_ASSOCIATIVE,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::validateLimit( qcStatement * aStatement, qmsLimit * aLimit )
{
    qtcNode* sHostNode;
    qcuSqlSourceInfo   sqlInfo;

    IDU_FIT_POINT_FATAL( "qmv::validateLimit::__FT__" );

    if( qmsLimitI::hasHostBind( qmsLimitI::getStart( aLimit ) ) == ID_TRUE )
    {
        sHostNode = qmsLimitI::getHostNode( qmsLimitI::getStart( aLimit ) );

        /* PROJ-2197 PSM Renewal */
        sHostNode->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        IDE_TEST( qtc::estimate(
                      sHostNode,
                      QC_SHARED_TMPLATE(aStatement),
                      aStatement,
                      NULL,
                      NULL,
                      NULL )
                  != IDE_SUCCESS );

        // sequence 안됨
        if ( ( sHostNode->lflag & QTC_NODE_SEQUENCE_MASK )
             == QTC_NODE_SEQUENCE_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sHostNode->position );
            IDE_RAISE( ERR_SEQUENCE_NOT_ALLOWED );
        }
        else
        {
            // Nothing to do.
        }

        // subquery 안됨
        if ( ( sHostNode->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sHostNode->position );
            IDE_RAISE( ERR_NOT_SUPPORT_SUBQUERY );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-16055
        IDE_TEST( qtc::makeConversionNode( sHostNode,
                                           aStatement,
                                           QC_SHARED_TMPLATE(aStatement),
                                           & mtdBigint )
                  != IDE_SUCCESS );
    }

    if( qmsLimitI::hasHostBind( qmsLimitI::getCount( aLimit ) ) == ID_TRUE )
    {
        sHostNode = qmsLimitI::getHostNode( qmsLimitI::getCount( aLimit ) );

        /* PROJ-2197 PSM Renewal */
        sHostNode->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        IDE_TEST( qtc::estimate(
                      sHostNode,
                      QC_SHARED_TMPLATE(aStatement),
                      aStatement,
                      NULL,
                      NULL,
                      NULL )
                  != IDE_SUCCESS );

        // sequence 안됨
        if ( ( sHostNode->lflag & QTC_NODE_SEQUENCE_MASK )
             == QTC_NODE_SEQUENCE_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sHostNode->position );
            IDE_RAISE( ERR_SEQUENCE_NOT_ALLOWED );
        }
        else
        {
            // Nothing to do.
        }

        // subquery 안됨
        if ( ( sHostNode->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sHostNode->position );
            IDE_RAISE( ERR_NOT_SUPPORT_SUBQUERY );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-16055
        IDE_TEST( qtc::makeConversionNode( sHostNode,
                                           aStatement,
                                           QC_SHARED_TMPLATE(aStatement),
                                           & mtdBigint )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SEQUENCE_NOT_ALLOWED )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QMV_SEQUENCE_NOT_ALLOWED,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORT_SUBQUERY )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_ALLOWED_SUBQUERY_SQLTEXT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

// BUG-41311
IDE_RC qmv::validateLoop( qcStatement * aStatement )
{
    qmsParseTree     * sParseTree;
    const mtcColumn  * sColumn;
    qcuSqlSourceInfo   sqlInfo;

    IDU_FIT_POINT_FATAL( "qmv::validateLoop::__FT__" );

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;
    
    if ( sParseTree->loopNode != NULL )
    {
        IDE_TEST( qtc::estimate( sParseTree->loopNode,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement,
                                 NULL,
                                 NULL,
                                 NULL )
                  != IDE_SUCCESS );

        // sequence 안됨
        if ( ( sParseTree->loopNode->lflag & QTC_NODE_SEQUENCE_MASK )
             == QTC_NODE_SEQUENCE_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sParseTree->loopNode->position );
            IDE_RAISE( ERR_SEQUENCE_NOT_ALLOWED );
        }
        else
        {
            // Nothing to do.
        }

        // subquery 안됨
        if ( ( sParseTree->loopNode->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sParseTree->loopNode->position );
            IDE_RAISE( ERR_NOT_SUPPORT_SUBQUERY );
        }
        else
        {
            // Nothing to do.
        }

        sColumn = QC_SHARED_TMPLATE(aStatement)->tmplate.stack->column;

        // loop clause는 다음 type만 가능하다.
        if ( ( sColumn->module->id == MTD_LIST_ID ) ||
             ( sColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) ||
             ( sColumn->module->id == MTD_RECORDTYPE_ID ) ||
             ( sColumn->module->id == MTD_ROWTYPE_ID ) )
        {
            // Nothing to do.
        }
        else
        {
            if ( qtc::makeConversionNode( sParseTree->loopNode,
                                          aStatement,
                                          QC_SHARED_TMPLATE(aStatement),
                                          & mtdBigint )
                 != IDE_SUCCESS )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sParseTree->loopNode->position );
                IDE_RAISE( ERR_LOOP_TYPE );
            }
            else
            {
                // Nothing to do.
            }
        }
        
        // loop_value를 위해 querySet에 전달
        sParseTree->querySet->loopNode = sParseTree->loopNode;
        sParseTree->querySet->loopStack =
            QC_SHARED_TMPLATE(aStatement)->tmplate.stack[0];
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SEQUENCE_NOT_ALLOWED )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QMV_SEQUENCE_NOT_ALLOWED,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORT_SUBQUERY )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_ALLOWED_SUBQUERY_SQLTEXT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_LOOP_TYPE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QTC_NOT_APPLICABLE_TYPE_IN_LOOP,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::setDefaultOrNULL(
    qcStatement     * aStatement,
    qcmTableInfo    * aTableInfo,
    qcmColumn       * aColumn,
    qmmValueNode    * aValue)
{
    UChar                  * sDefaultValueStr;
    qdDefaultParseTree     * sDefaultParseTree;
    qcStatement            * sDefaultStatement;
    UInt                     sOriInsOrUptStmtCount;
    UInt                   * sOriInsOrUptRowValueCount;
    smiValue              ** sOriInsOrUptRow;

    IDU_FIT_POINT_FATAL( "qmv::setDefaultOrNULL::__FT__" );

    IDE_TEST_RAISE( ( aTableInfo->tableType == QCM_VIEW ) ||
                    ( aTableInfo->tableType == QCM_MVIEW_VIEW ),
                    ERR_CANT_USE_DEFAULT_IN_VIEW );

    // The value is DEFAULT value
    if ( ( aColumn->defaultValueStr != NULL ) &&
         ( (aColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)  /* PROJ-1090 Function-based Index */
           != QCM_COLUMN_HIDDEN_COLUMN_TRUE ) )
    {
        // get DEFAULT value
        IDE_TEST(STRUCT_ALLOC(
                     QC_QMP_MEM(aStatement), qcStatement, &sDefaultStatement)
                 != IDE_SUCCESS);

        QC_SET_STATEMENT(sDefaultStatement, aStatement, NULL);

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     idlOS::strlen((SChar*)aColumn->defaultValueStr) + 9,
                     (void**)&sDefaultValueStr)
                 != IDE_SUCCESS);

        idlOS::snprintf( (SChar*)sDefaultValueStr,
                         idlOS::strlen((SChar*)aColumn->defaultValueStr) + 9,
                         "DEFAULT %s",
                         aColumn->defaultValueStr );
    }
    else
    {
        // get NULL value
        IDE_TEST(STRUCT_ALLOC(
                     QC_QMP_MEM(aStatement), qcStatement, &sDefaultStatement)
                 != IDE_SUCCESS);

        QC_SET_STATEMENT(sDefaultStatement, aStatement, NULL);

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(13, (void**)&sDefaultValueStr)
                 != IDE_SUCCESS);

        idlOS::snprintf( (SChar*) sDefaultValueStr,
                         13,
                         "DEFAULT NULL" );
    }

    sDefaultStatement->myPlan->stmtText = (SChar*)sDefaultValueStr;
    sDefaultStatement->myPlan->stmtTextLen = idlOS::strlen((SChar*)sDefaultValueStr);

    // preserve insOrUptRow
    sOriInsOrUptStmtCount = QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptStmtCount;
    sOriInsOrUptRowValueCount =
        QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRowValueCount;
    sOriInsOrUptRow = QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRow;

    IDE_TEST(qcpManager::parseIt(sDefaultStatement) != IDE_SUCCESS);

    // restore insOrUptRow
    QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptStmtCount  = sOriInsOrUptStmtCount;
    QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRowValueCount =
        sOriInsOrUptRowValueCount;
    QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRow = sOriInsOrUptRow;

    sDefaultParseTree = (qdDefaultParseTree*) sDefaultStatement->myPlan->parseTree;

    // set DEFAULT value
    aValue->value = sDefaultParseTree->defaultValue;

    // set default mask
    aValue->value->lflag &= ~QTC_NODE_DEFAULT_VALUE_MASK;
    aValue->value->lflag |= QTC_NODE_DEFAULT_VALUE_TRUE;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CANT_USE_DEFAULT_IN_VIEW);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_CANNOT_USE_DEFAULT_IN_VIEW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::getValue( qcmColumn        * aColumnList,
                      qmmValueNode     * aValueList,
                      qcmColumn        * aColumn,
                      qmmValueNode    ** aValue )
{
    qcmColumn       * sColumn;
    qmmValueNode    * sCurrValue;

    IDU_FIT_POINT_FATAL( "qmv::getValue::__FT__" );

    *aValue = NULL;

    for (sColumn = aColumnList,
             sCurrValue = aValueList;
         sColumn != NULL &&
             sCurrValue != NULL;
         sColumn = sColumn->next,
             sCurrValue = sCurrValue->next)
    {
        if (aColumn->basicInfo->column.id == sColumn->basicInfo->column.id)
        {
            *aValue = sCurrValue;
            break;
        }
    }

    return IDE_SUCCESS;
    
}

IDE_RC qmv::getParentInfoList(qcStatement     *aStatement,
                              qcmTableInfo    *aTableInfo,
                              qcmParentInfo   **aParentInfo,
                              qcmColumn       *aChangedColumn /* = NULL*/,
                              SInt             aUptColCount   /* = 0 */ )
{
    qcmParentInfo *sParentInfo;
    qcmParentInfo *sLastInfo = NULL;
    qcmColumn     *sTempColumn;

    SInt i;
    UInt *sColIds;

    IDU_FIT_POINT_FATAL( "qmv::getParentInfoList::__FT__" );

    *aParentInfo = NULL;

    if ( aUptColCount != 0 )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF(UInt) * aUptColCount,
                                                   (void**) &sColIds )
                  != IDE_SUCCESS );

        // fix BUG-16505
        for( sTempColumn = aChangedColumn, i = 0;
             ( sTempColumn != NULL ) && ( i < aUptColCount );
             sTempColumn = sTempColumn->next, i++ )
        {
            sColIds[i] = sTempColumn->basicInfo->column.id;
        }
    }
    else
    {
        sColIds = NULL;
    }

    for ( i = 0; i < (SInt) aTableInfo->foreignKeyCount; i++ )
    {
        if ( qdn::intersectColumn( aTableInfo->foreignKeys[i].referencingColumn,
                                   aTableInfo->foreignKeys[i].constraintColumnCount,
                                   sColIds,
                                   aUptColCount) == ID_TRUE )
        {
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF(qcmParentInfo ),
                                                       (void**) &sParentInfo )
                      != IDE_SUCCESS );


            IDE_TEST( qcm::getParentKey( aStatement,
                                         &aTableInfo->foreignKeys[i],
                                         sParentInfo )
                      != IDE_SUCCESS );

            if ( *aParentInfo == NULL )
            {
                *aParentInfo = sParentInfo;
                sParentInfo->next = NULL;
                sLastInfo = sParentInfo;
            }
            else
            {
                sLastInfo->next = sParentInfo;
                sParentInfo->next = NULL;
                sLastInfo = sParentInfo;
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

IDE_RC qmv::getChildInfoList(qcStatement       * aStatement,
                             qcmTableInfo      * aTableInfo,
                             qcmRefChildInfo  ** aChildInfo,
                             qcmColumn         * aChangedColumn/* = NULL*/,
                             SInt                aUptColCount  /* = 0   */ )
{
/***********************************************************************
 *
 * Description :
 *    UPDATE ... SET ... 의 parse 수행
 *
 * Implementation :
 *    1. 변경되는 컬럼으로 컬럼 ID 의 배열을 만든다
 *    2. 테이블의 uniquekey 중에서 변경되는 컬럼이 key 컬럼에 속해 있으면
 *    3. 그 key 를 참조하는 child 테이블을 찾는다.
 *    4. 2,3 을 반복해서 child 테이블의 리스트를 반환한다.
 *
 ***********************************************************************/

    qcmRefChildInfo  * sChildInfo;     // BUG-28049
    qcmRefChildInfo  * sRefChildInfo;  // BUG-28049
    qcmColumn        * sTempColumn;
    qcmIndex         * sIndexInfo;
    UInt             * sColIds;
    SInt               i;

    IDU_FIT_POINT_FATAL( "qmv::getChildInfoList::__FT__" );

    *aChildInfo = NULL;

    if (aUptColCount != 0)
    {
        //--------------------------
        // UPDATE
        //--------------------------

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(UInt) * aUptColCount,
                                               (void**)&sColIds)
                 != IDE_SUCCESS);

        // fix BUG-16505
        for( sTempColumn = aChangedColumn, i = 0;
             ( sTempColumn != NULL ) && ( i < aUptColCount );
             sTempColumn = sTempColumn->next, i++ )
        {
            sColIds[i] = sTempColumn->basicInfo->column.id;
        }
    }
    else
    {
        //--------------------------
        // DELETE
        //--------------------------

        sColIds = NULL;
    }

    for (i = 0; i < (SInt)aTableInfo->uniqueKeyCount; i++)
    {
        sIndexInfo = aTableInfo->uniqueKeys[i].constraintIndex;

        // BUG-28049
        if ( ( sColIds == NULL ) ||
             ( qdn::intersectColumn( sIndexInfo->keyColumns,
                                     sIndexInfo->keyColCount,
                                     sColIds,
                                     aUptColCount ) == ID_TRUE ) )
        {
            IDE_TEST(qcm::getChildKeys( aStatement,
                                        sIndexInfo,
                                        aTableInfo,
                                        & sChildInfo )
                     != IDE_SUCCESS);

            if ( sChildInfo != NULL )
            {
                if( *aChildInfo != NULL )
                {
                    sRefChildInfo = *aChildInfo;

                    while( sRefChildInfo->next != NULL )
                    {
                        sRefChildInfo = sRefChildInfo->next;
                    }

                    sRefChildInfo->next = sChildInfo;
                }
                else
                {
                    *aChildInfo = sChildInfo;
                }

                // PROJ-1509
                // on delete cascade인 경우,
                // 모든 하위 child table에 대한 정보를 구축해 둔다.
                for( sRefChildInfo = sChildInfo;
                     sRefChildInfo != NULL;
                     sRefChildInfo = sRefChildInfo->next )
                {
                    IDE_TEST( getChildInfoList( aStatement,
                                                *aChildInfo,
                                                sRefChildInfo )
                              != IDE_SUCCESS );
                }
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

IDE_RC qmv::getChildInfoList(qcStatement      * aStatement,
                             qcmRefChildInfo  * aTopChildInfo,
                             qcmRefChildInfo  * aRefChildInfo )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt             sCnt;
    idBool           sGetChildInfo = ID_FALSE;

    qcmForeignKey      * sForeignKey;
    qcmRefChildInfo    * sRefChildInfo;  // BUG-28049
    qcmIndex           * sIndex;
    qcmRefChildInfo    * sChildInfo;     // BUG-28049

    qmsTableRef        * sChildTableRef;
    qcmTableInfo       * sChildTableInfo;

    IDU_FIT_POINT_FATAL( "qmv::getChildInfoList:::__FT__" );

    // PROJ-1509
    // (1) foreign key의 referenceRule이 cascade이고,
    // (2) foreign key가
    //    또다른 테이블의 parent key가 될 수 있는 경우
    // child의 모든 정보 구축.

    // BUG-28049
    sForeignKey     = aRefChildInfo->foreignKey;
    sChildTableRef  = aRefChildInfo->childTableRef;
    sChildTableInfo = sChildTableRef->tableInfo;

    for( sCnt = 0;
         sCnt < sChildTableInfo->uniqueKeyCount;
         sCnt++ )
    {
        sGetChildInfo = ID_TRUE;

        sIndex = sChildTableInfo->uniqueKeys[sCnt].constraintIndex;

        // cascade cycle이 되는 경우의 처리가 필요함
        // 아니면, getChildInfoList가 무한적으로 만들어짐.
        if( checkCascadeCycle( aTopChildInfo,
                               sIndex ) == ID_TRUE )
        {
            sGetChildInfo = ID_FALSE;
        }
        else
        {
            // Nothing To Do
        }

        // BUG-29728
        // 기존 메타값을 변경하지 않기 위해 DML flag를 제거해서
        // sForeignKey->referenceRule에 option 정보만 들어가 있고,
        // QD_FOREIGN_DELETE_CASCADE에 option 정보(뒤세자리) 값만
        // 비교하기 위해 MASK를 사용한다.
        if ( (sForeignKey->referenceRule & QD_FOREIGN_OPTION_MASK)
             == (QD_FOREIGN_DELETE_CASCADE  & QD_FOREIGN_OPTION_MASK) ||
             (sForeignKey->referenceRule & QD_FOREIGN_OPTION_MASK)
             == (QD_FOREIGN_DELETE_SET_NULL  & QD_FOREIGN_OPTION_MASK) )
        {
            if( sGetChildInfo == ID_TRUE )
            {
                IDE_TEST( qcm::getChildKeys( aStatement,
                                             sIndex,
                                             sChildTableInfo,
                                             & sChildInfo )
                          != IDE_SUCCESS );

                if ( sChildInfo != NULL )
                {
                    sRefChildInfo = aRefChildInfo;

                    while( sRefChildInfo->next != NULL )
                    {
                        sRefChildInfo = sRefChildInfo->next;
                    }

                    sRefChildInfo->next = sChildInfo;

                    for( sRefChildInfo = sChildInfo;
                         sRefChildInfo != NULL;
                         sRefChildInfo = sRefChildInfo->next )
                    {
                        IDE_TEST( getChildInfoList( aStatement,
                                                    aTopChildInfo,
                                                    sRefChildInfo )
                                  != IDE_SUCCESS );
                    }
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // Nothing to do.
            }
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

idBool qmv::checkCascadeCycle(qcmRefChildInfo    * aRefChildInfo,
                              qcmIndex           * aIndex )
{
/***********************************************************************
 *
 * Description : delete cascade cycle 검사.
 *
 *     referential action이 on delete cascade 인 경우,
 *     모든 하위 child table에 대한 정보를 구축하는데,
 *     이 하위 child table이 무한히 반복적으로 구축되지 않도록 하기 위함.
 *
 *     예) create table parent ( i1 integer );
 *         alter table parent
 *         add constraint parent_pk primary key( i1 );
 *
 *         create table child ( i1 integer );
 *         alter table child
 *         add constraint child_fk
 *         foreign key( i1 ) references parent( i1 ) on delete cascade;
 *
 *         alter table child
 *         add constraint child_pk primary key( i1 );
 *
 *         alter table parent
 *         add constraint parent_fk foreign key( i1 )
 *         references child(i1) on delete cascade;
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool                sExistCycle = ID_FALSE;
    qcmRefChildInfo     * sRefChildInfo;  // BUG-28049

    // BUG-28049
    for( sRefChildInfo = aRefChildInfo;
         sRefChildInfo != NULL;
         sRefChildInfo = sRefChildInfo->next )
    {
        if( aIndex == sRefChildInfo->parentIndex )
        {
            // cycle이 존재하는 경우
            sExistCycle = ID_TRUE;

            for( sRefChildInfo = aRefChildInfo;
                 sRefChildInfo != NULL;
                 sRefChildInfo = sRefChildInfo->next )
            {
                sRefChildInfo->isCycle = ID_TRUE;
            }

            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sExistCycle;
}

IDE_RC qmv::parseMove( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    MOVE ... FROM ... 의 parse 수행
 *
 * Implementation :
 *    1. target tableRef에 대한 validation수행
 *    2. target tableRef에 대한 privilige검사(insert)
 *    3. source tableRef에 대한 validation수행
 *    4. check table type, 뷰이면 에러
 *    5. source tableRef에 대한 privilige검사(delete)
 *    6. parse VIEW in WHERE clause
 *    7. expression list존재 유무에 따라 value list 생성
 *    8. target column list & source expression list validation
 *    9. parse VIEW in expression list
 *
 ***********************************************************************/

    qmmMoveParseTree    * sParseTree;
    qcmTableInfo        * sTargetTableInfo;
    qcmTableInfo        * sSourceTableInfo;
    qmsTableRef         * sSourceTableRef;
    qcmColumn           * sColumn;
    qcmColumn           * sColumnInfo;
    qmmValueNode        * sValue;
    qmmValueNode        * sLastValue        = NULL;
    qmmValueNode        * sFirstValue;
    qmmValueNode        * sPrevValue;
    qmmValueNode        * sCurrValue;
    // To fix BUG-12318 qtc::makeColumns 함수 내에서
    // aNode[1]=NULL로 초기화 하기 때문에 2개의 포인터 배열을
    // 인자로 넘겨 주어야 함
    qtcNode             * sNode[2];
    qcuSqlSourceInfo      sqlInfo;
    UInt                  i; // fix BUG-12105
    UInt                  sSrcColCnt;
    UInt                  sFlag = 0;

    IDU_FIT_POINT_FATAL( "qmv::parseMove::__FT__" );

    sParseTree = (qmmMoveParseTree*)aStatement->myPlan->parseTree;

    // check existence of table and get table META Info.
    sFlag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sFlag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sFlag &= ~(QMV_VIEW_CREATION_MASK);
    sFlag |= (QMV_VIEW_CREATION_FALSE);

    // BUG-17409
    sParseTree->targetTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
    sParseTree->targetTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

    IDE_TEST(qmvQuerySet::validateQmsTableRef( aStatement,
                                               NULL,
                                               sParseTree->targetTableRef,
                                               sFlag,
                                               MTC_COLUMN_NOTNULL_TRUE )
             != IDE_SUCCESS); // PR-13597

    // target에 대한 부분
    IDE_TEST( insertCommon( aStatement,
                            sParseTree->targetTableRef,
                            &(sParseTree->checkConstrList),      /* PROJ-1107 Check Constraint 지원 */
                            &(sParseTree->defaultExprColumns),   /* PROJ-1090 Function-based Index */
                            &(sParseTree->defaultTableRef) )
              != IDE_SUCCESS );

    sTargetTableInfo = sParseTree->targetTableRef->tableInfo;

    // To Fix BUG-13156
    sParseTree->targetTableRef->tableHandle = sTargetTableInfo->tableHandle;

    // source에 대한 부분
    sSourceTableRef  = sParseTree->querySet->SFWGH->from->tableRef;

    qtc::dependencyClear( & sParseTree->querySet->SFWGH->depInfo );

    // check existence of table and get table META Info.
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_VIEW_CREATION_FALSE);

    IDE_TEST(qmvQuerySet::validateQmsTableRef(aStatement,
                                              sParseTree->querySet->SFWGH,
                                              sSourceTableRef,
                                              sParseTree->querySet->SFWGH->lflag,
                                              MTC_COLUMN_NOTNULL_TRUE) // PR-13597
             != IDE_SUCCESS);

    QC_SHARED_TMPLATE(aStatement)->tableMap[sSourceTableRef->table].from =
        sParseTree->querySet->SFWGH->from;

    // From 절에 dependency 설정
    qtc::dependencyClear( & sParseTree->querySet->SFWGH->from->depInfo );
    qtc::dependencySet( sSourceTableRef->table,
                        & sParseTree->querySet->SFWGH->from->depInfo );

    // Query Set에 dependency 설정
    qtc::dependencyClear( & sParseTree->querySet->depInfo );
    IDE_TEST( qtc::dependencyOr( & sParseTree->querySet->depInfo,
                                 & sParseTree->querySet->SFWGH->depInfo,
                                 & sParseTree->querySet->depInfo )
              != IDE_SUCCESS );

    // check table type
    if ( ( sSourceTableRef->tableInfo->tableType == QCM_VIEW ) ||
         ( sSourceTableRef->tableInfo->tableType == QCM_MVIEW_VIEW ) )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            & sSourceTableRef->tableName );
        IDE_RAISE( ERR_DML_ON_READ_ONLY_VIEW );
    }

    qtc::dependencySet( sSourceTableRef->table,
                        & sParseTree->querySet->SFWGH->depInfo );

    sSourceTableInfo = sSourceTableRef->tableInfo;

    IDE_TEST_RAISE(
        ( sSourceTableInfo->tableID == sTargetTableInfo->tableID ),
        ERR_CANT_MOVE_SAMETABLE );

    // check grant
    IDE_TEST( qdpRole::checkDMLDeleteTablePriv( aStatement,
                                                sSourceTableInfo->tableHandle,
                                                sSourceTableInfo->tableOwnerID,
                                                sSourceTableInfo->privilegeCount,
                                                sSourceTableInfo->privilegeInfo,
                                                ID_FALSE,
                                                NULL,
                                                NULL )
              != IDE_SUCCESS );

    // environment의 기록
    IDE_TEST( qcgPlan::registerPlanPrivTable( aStatement,
                                              QCM_PRIV_ID_OBJECT_DELETE_NO,
                                              sSourceTableInfo )
              != IDE_SUCCESS );

    // parse VIEW in WHERE clause
    if (sParseTree->querySet->SFWGH->where != NULL)
    {
        IDE_TEST( parseViewInExpression( aStatement,
                                         sParseTree->querySet->SFWGH->where )
                  != IDE_SUCCESS);
    }

    // MOVE INTO ... FROM  T2 뒤에 expression이 없는 경우
    // value node를 생성해 주어야 함.
    if( sParseTree->values == NULL)
    {
        for( sColumn = sSourceTableInfo->columns,
                 i = 0;
             i < sSourceTableInfo->columnCount;
             sColumn = sColumn->next,
                 i++ )
        {
            /* BUG-36740 Move DML이 Function-Based Index를 지원해야 합니다. */
            if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( qtc::makeColumn( aStatement,
                                       sNode,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL ) != IDE_SUCCESS );
            IDE_TEST( qtc::makeTargetColumn( sNode[0],
                                             sSourceTableRef->table,
                                             (UShort)i )
                      != IDE_SUCCESS ); // fix BUG-12105

            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmValueNode, &sValue)
                     != IDE_SUCCESS);
            sValue->value     = sNode[0];
            sValue->validate  = ID_TRUE;
            sValue->calculate = ID_TRUE;
            sValue->timestamp = ID_FALSE;
            sValue->expand    = ID_TRUE;
            sValue->msgID     = ID_FALSE;
            sValue->next      = NULL;
            if( sParseTree->values == NULL )
            {
                sParseTree->values = sValue;
                sLastValue = sParseTree->values;
            }
            else
            {
                sLastValue->next = sValue;
                sLastValue = sLastValue->next;
            }
        }
    }
    else
    {
        // Nothing to do
    }

    // The possible cases
    //             number of column   number of value
    //  - case 1 :        m                  n        (m = n and n > 0)
    //          => sTableInfo->columns 순으로 column과 values를 구성
    //          => 명시되지 않은 column의 value는 NULL 이거나 DEFAULT value
    //  - case 2 :        m                  n        (m = 0 and n > 0)
    //          => sTableInfo->columns를 가지고 옴.
    //  - case 3 :        m                  n        (m < n and n > 0)
    //          => too many values error
    //  - case 4 :        m                  n        (m > n and n > 0)
    //          => not enough values error
    // The other cases cannot pass parser.

    if (sParseTree->columns != NULL)
    {
        for (sColumn = sParseTree->columns,
                 sCurrValue = sParseTree->values;
             sColumn != NULL;
             sColumn = sColumn->next,
                 sCurrValue = sCurrValue->next)
        {
            IDE_TEST_RAISE(sCurrValue == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);

            // The Parser checked duplicated name in column list.
            // check column existence
            if ( QC_IS_NULL_NAME(sColumn->tableNamePos) != ID_TRUE )
            {
                if ( idlOS::strMatch( sParseTree->targetTableRef->aliasName.stmtText +
                                      sParseTree->targetTableRef->aliasName.offset,
                                      sParseTree->targetTableRef->aliasName.size,
                                      sColumn->tableNamePos.stmtText + sColumn->tableNamePos.offset,
                                      sColumn->tableNamePos.size ) != 0 )
                {
                    sqlInfo.setSourceInfo( aStatement, &sColumn->namePos );
                    IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                }
            }

            IDE_TEST(qcmCache::getColumn(aStatement,
                                         sTargetTableInfo,
                                         sColumn->namePos,
                                         &sColumnInfo) != IDE_SUCCESS);
            QMV_SET_QCM_COLUMN( sColumn, sColumnInfo );

            /* PROJ-1090 Function-based Index */
            if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &(sColumn->namePos) );
                IDE_RAISE( ERR_CANNOT_USE_HIDDEN_COLUMN );
            }
            else
            {
                /* Nothing to do */
            }

            //--------- validation of inserting value  ---------//
            if (sCurrValue->value == NULL)
            {
                // insert into t1 values ( DEFAULT )
                IDE_TEST(setDefaultOrNULL( aStatement, sTargetTableInfo, sColumnInfo, sCurrValue)
                         != IDE_SUCCESS);

                if ( ( sColumnInfo->basicInfo->flag
                       & MTC_COLUMN_TIMESTAMP_MASK )
                     == MTC_COLUMN_TIMESTAMP_TRUE )
                {
                    sCurrValue->timestamp = ID_TRUE;
                }
                else
                {
                    sCurrValue->timestamp = ID_FALSE;
                }
            }
            else
            {
                IDE_TEST(parseViewInExpression( aStatement, sCurrValue->value )
                         != IDE_SUCCESS);
            }
        }

        IDE_TEST_RAISE(sCurrValue != NULL, ERR_TOO_MANY_INSERT_VALUES);

        // The number of sParseTree->columns = The number of sParseTree->values

        sFirstValue = NULL;
        sPrevValue = NULL;
        for (sColumn = sTargetTableInfo->columns;
             sColumn != NULL;
             sColumn = sColumn->next)
        {
            IDE_TEST(getValue(sParseTree->columns,
                              sParseTree->values,
                              sColumn,
                              &sValue) != IDE_SUCCESS);

            // make current value
            IDE_TEST(STRUCT_ALLOC(
                         QC_QMP_MEM(aStatement), qmmValueNode, &sCurrValue)
                     != IDE_SUCCESS);

            sCurrValue->msgID = ID_FALSE;
            sCurrValue->next = NULL;

            if (sValue == NULL)
            {
                // make current value
                sCurrValue->value = NULL;
                sCurrValue->validate = ID_TRUE;
                sCurrValue->calculate = ID_TRUE;
                sCurrValue->expand = ID_FALSE;

                IDE_TEST(setDefaultOrNULL( aStatement, sTargetTableInfo, sColumn, sCurrValue)
                         != IDE_SUCCESS);

                if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                     == MTC_COLUMN_TIMESTAMP_TRUE )
                {
                    sCurrValue->timestamp = ID_TRUE;
                }
                else
                {
                    sCurrValue->timestamp = ID_FALSE;
                }
            }
            else
            {
                sCurrValue->value     = sValue->value;
                sCurrValue->validate  = sValue->validate;
                sCurrValue->calculate = sValue->calculate;
                sCurrValue->timestamp = sValue->timestamp;
                sCurrValue->msgID     = sValue->msgID;
                sCurrValue->expand    = sValue->expand;
            }

            // make value list
            if (sFirstValue == NULL)
            {
                sFirstValue = sCurrValue;
                sPrevValue  = sCurrValue;
            }
            else
            {
                sPrevValue->next = sCurrValue;
                sPrevValue       = sCurrValue;
            }
        }

        sParseTree->columns = sTargetTableInfo->columns;  // full columns list
        sParseTree->values  = sFirstValue;          // full values list
    }
    else
    {
        sParseTree->columns = sTargetTableInfo->columns;
        sPrevValue = NULL;

        for (sColumn = sParseTree->columns,
                 sCurrValue = sParseTree->values;
             sColumn != NULL;
             sColumn = sColumn->next,
                 sCurrValue = sCurrValue->next)
        {
            /* PROJ-1090 Function-based Index */
            if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                // make value
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmmValueNode, &sValue )
                          != IDE_SUCCESS );

                sValue->value     = NULL;
                sValue->validate  = ID_TRUE;
                sValue->calculate = ID_TRUE;
                sValue->timestamp = ID_FALSE;
                sValue->msgID     = ID_FALSE;
                sValue->expand    = ID_FALSE;

                // connect value list
                sValue->next      = sCurrValue;
                sCurrValue        = sValue;
                if ( sPrevValue == NULL )
                {
                    sParseTree->values = sValue;
                }
                else
                {
                    sPrevValue->next = sValue;
                }
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST_RAISE(sCurrValue == NULL, ERR_NOT_ENOUGH_INSERT_VALUES);

            //--------- validation of inserting value  ---------//
            if (sCurrValue->value == NULL)
            {
                IDE_TEST(setDefaultOrNULL( aStatement, sTargetTableInfo, sColumn, sCurrValue)
                         != IDE_SUCCESS);

                if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                     == MTC_COLUMN_TIMESTAMP_TRUE )
                {
                    sCurrValue->timestamp = ID_TRUE;
                }
            }
            else
            {
                IDE_TEST(parseViewInExpression( aStatement, sCurrValue->value )
                         != IDE_SUCCESS);
            }

            sPrevValue = sCurrValue;
        }

        IDE_TEST_RAISE(sCurrValue != NULL, ERR_TOO_MANY_INSERT_VALUES);
    }

    // PROJ-1705
    // MOVE INTO T1 FROM T2; 인 경우,
    // T2 테이블의 패치컬럼리스트구성을 위한 정보 설정.
    //
    // BUG-43722 disk table에서 move table 수행 결과가 다릅니다.
    // Target table에 column 명시 유무에 상관없이
    // source table의 패치컬럼 리스트를 구성하도록 정보를 설정해야 합니다.
    for( sSrcColCnt = 0; sSrcColCnt < sSourceTableInfo->columnCount; sSrcColCnt++ )
    {
        QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[sSourceTableRef->table].columns[sSrcColCnt].flag
            &= ~MTC_COLUMN_USE_COLUMN_MASK;
        QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[sSourceTableRef->table].columns[sSrcColCnt].flag
            |= MTC_COLUMN_USE_COLUMN_TRUE;

        QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[sSourceTableRef->table].columns[sSrcColCnt].flag
            &= ~MTC_COLUMN_USE_TARGET_MASK;
        QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[sSourceTableRef->table].columns[sSrcColCnt].flag
            |= MTC_COLUMN_USE_TARGET_TRUE;
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_USE_HIDDEN_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ENOUGH_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_INSERT_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TOO_MANY_INSERT_VALUES));
    }
    IDE_EXCEPTION(ERR_DML_ON_READ_ONLY_VIEW);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_DML_ON_READ_ONLY_VIEW,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_CANT_MOVE_SAMETABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_MOVE_SAME_TABLE));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qmv::validateMove( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    MOVE ... FROM ... 의 validate 수행
 *
 * Implementation :
 *    1. fixed table 검사
 *    2. Move를 위한 Row 생성
 *    3. value list의 validation(aggregation은 올 수 없다)
 *    4. hint validation
 *    5. where validation
 *    6. limit validation
 *    7. check constraint validation
 *    8. default expression validation
 *    9. target table의 child constraint 정보 얻음
 *   10. source table의 parent constraint 정보 얻음
 *
 ***********************************************************************/

    qmmMoveParseTree    * sParseTree;
    qcmTableInfo        * sTargetTableInfo;
    qcmTableInfo        * sSourceTableInfo;
    qmmValueNode        * sCurrValue;
    qcmColumn           * sQcmColumn;
    mtcColumn           * sMtcColumn;
    qcuSqlSourceInfo      sqlInfo;
    const mtdModule    ** sModules;
    const mtdModule     * sLocatorModule;
    qdConstraintSpec    * sConstr;
    qmsFrom               sFrom;
    IDE_RC                sRC;

    IDU_FIT_POINT_FATAL( "qmv::validateMove::__FT__" );

    sParseTree = (qmmMoveParseTree*)aStatement->myPlan->parseTree;

    sTargetTableInfo = sParseTree->targetTableRef->tableInfo;
    sSourceTableInfo = sParseTree->querySet->SFWGH->from->tableRef->tableInfo;

    // PR-13725
    // CHECK OPERATABLE
    if( QCM_IS_OPERATABLE_QP_DELETE( sSourceTableInfo->operatableFlag ) != ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->targetTableRef->tableName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }
    if( QCM_IS_OPERATABLE_QP_INSERT( sTargetTableInfo->operatableFlag ) != ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->targetTableRef->tableName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }

    // queue table에 대한 move문은 오류.
    if ( ( sSourceTableInfo->tableType == QCM_QUEUE_TABLE ) ||
         ( sTargetTableInfo->tableType == QCM_QUEUE_TABLE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->targetTableRef->tableName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2211 Materialized View */
    if ( ( sSourceTableInfo->tableType == QCM_MVIEW_TABLE ) ||
         ( sTargetTableInfo->tableType == QCM_MVIEW_TABLE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->targetTableRef->tableName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(makeMoveRow( aStatement ) != IDE_SUCCESS);

    sModules = sParseTree->columnModules;

    // PROJ-1473
    // validation of Hints
    IDE_TEST( qmvQuerySet::validateHints(aStatement,
                                         sParseTree->querySet->SFWGH)
              != IDE_SUCCESS );

    for( sCurrValue = sParseTree->values;
         sCurrValue != NULL;
         sCurrValue = sCurrValue->next, sModules++ )
    {
        if( sCurrValue->value != NULL )
        {
            IDE_TEST( notAllowedAnalyticFunc( aStatement, sCurrValue->value )
                      != IDE_SUCCESS );

            if( sCurrValue->validate == ID_TRUE )
            {
                if( sCurrValue->expand == ID_TRUE )
                {
                    sRC = qtc::estimate( sCurrValue->value,
                                         QC_SHARED_TMPLATE(aStatement),
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL );
                }
                else
                {
                    sRC = qtc::estimate( sCurrValue->value,
                                         QC_SHARED_TMPLATE(aStatement),
                                         aStatement,
                                         NULL,
                                         sParseTree->querySet->SFWGH,
                                         NULL );
                }
                
                if ( sRC != IDE_SUCCESS )
                {
                    // default value인 경우 별도 에러처리
                    IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                    == QTC_NODE_DEFAULT_VALUE_TRUE,
                                    ERR_INVALID_DEFAULT_VALUE );

                    // default value가 아닌 경우 에러pass
                    IDE_TEST( 1 );
                }
                else
                {
                    // Nothing to do.
                }
            }

            // PROJ-2002 Column Security
            // move into t1(i1) from t2(i1) 같은 경우 동일 암호 타입이라도
            // policy가 다를 수 있으므로 t2.i1에 decrypt func을 생성한다.
            sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );

            if ( (sMtcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                 == MTD_ENCRYPT_TYPE_TRUE )
            {
                // default policy의 암호 타입이라도 decrypt 함수를 생성하여
                // subquery의 결과는 항상 암호 타입이 나올 수 없게 한다.

                // add decrypt func
                IDE_TEST( addDecryptFunc4ValueNode( aStatement,
                                                    sCurrValue )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );

            // PROJ-1362
            // add Lob-Locator Conversion Node
            if ( (sCurrValue->value->node.module == & qtc::columnModule) &&
                 ((sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_LOB) )
            {
                // BUG-33890
                if( ( (*sModules)->id == MTD_BLOB_ID ) ||
                    ( (*sModules)->id == MTD_CLOB_ID ) )
                {
                    IDE_TEST( mtf::getLobFuncResultModule( &sLocatorModule,
                                                           *sModules )
                              != IDE_SUCCESS );

                    IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                       aStatement,
                                                       QC_SHARED_TMPLATE(aStatement),
                                                       sLocatorModule )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( qtc::makeConversionNode( sCurrValue->value ,
                                                       aStatement ,
                                                       QC_SHARED_TMPLATE(aStatement) ,
                                                       *sModules )
                              != IDE_SUCCESS);
                }
            }
            else
            {
                IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                   aStatement,
                                                   QC_SHARED_TMPLATE(aStatement),
                                                   *sModules )
                          != IDE_SUCCESS );
            }

            // PROJ-1502 PARTITIONED DISK TABLE
            IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                              QC_SHARED_TMPLATE(aStatement),
                                              aStatement )
                      != IDE_SUCCESS );

            if( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sCurrValue->value->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }

            if ( QTC_HAVE_AGGREGATE( sCurrValue->value ) == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrValue->value->position );
                IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    // validation of WHERE clause
    if (sParseTree->querySet->SFWGH->where != NULL)
    {
        sParseTree->querySet->processPhase = QMS_VALIDATE_WHERE;

        IDE_TEST(
            qmvQuerySet::validateWhere(
                aStatement,
                NULL, // querySet : SELECT 구문만 필요
                sParseTree->querySet->SFWGH )
            != IDE_SUCCESS);
    }

    // PROJ-1436
    if ( sParseTree->querySet->SFWGH->hints != NULL )
    {
        IDE_TEST( validatePlanHints( aStatement,
                                     sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // validation of LIMIT clause
    if( sParseTree->limit != NULL )
    {
        IDE_TEST( validateLimit( aStatement, sParseTree->limit )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1107 Check Constraint 지원 */
    for ( sConstr = sParseTree->checkConstrList;
          sConstr != NULL;
          sConstr = sConstr->next )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sParseTree->targetTableRef;

        IDE_TEST( qdbCommon::validateCheckConstrDefinition(
                      aStatement,
                      sConstr,
                      NULL,
                      &sFrom )
                  != IDE_SUCCESS );
    }

    /* PROJ-1090 Function-based Index */
    if ( sParseTree->defaultExprColumns != NULL )
    {
        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sParseTree->defaultTableRef;

        for ( sQcmColumn = sParseTree->defaultExprColumns;
              sQcmColumn != NULL;
              sQcmColumn = sQcmColumn->next)
        {
            IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                          aStatement,
                          sQcmColumn->defaultValue,
                          NULL,
                          &sFrom )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(getParentInfoList(aStatement,
                               sTargetTableInfo,
                               &(sParseTree->parentConstraints))
             != IDE_SUCCESS);

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 처리시 포린키 정보수집
    //---------------------------------------------

    IDE_TEST( setFetchColumnInfo4ParentTable(
                  aStatement,
                  sParseTree->targetTableRef )
              != IDE_SUCCESS );

    // PROJ-2205 DML trigger에 의한 컬럼참조
    IDE_TEST( setFetchColumnInfo4Trigger(
                  aStatement,
                  sParseTree->targetTableRef )
              != IDE_SUCCESS );

    IDE_TEST(getChildInfoList(aStatement,
                              sSourceTableInfo,
                              &(sParseTree->childConstraints))
             != IDE_SUCCESS);

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 처리시 포린키 정보수집
    //---------------------------------------------

    IDE_TEST( setFetchColumnInfo4ChildTable(
                  aStatement,
                  sParseTree->querySet->SFWGH->from->tableRef )
              != IDE_SUCCESS );

    // PROJ-2205 DML trigger에 의한 컬럼참조
    IDE_TEST( setFetchColumnInfo4Trigger(
                  aStatement,
                  sParseTree->querySet->SFWGH->from->tableRef )
              != IDE_SUCCESS );

    /*
     * BUG-39441
     * need a interface which returns whether DML on replication table or not
     *
     * ex) move into t2 from t1 => t2 table 이 replication 인지 검사
     */
    if (sTargetTableInfo->replicationCount > 0)
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_FALSE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_AGGREGATION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_AGGREGATION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_DEFAULT_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::makeMoveRow(qcStatement * aStatement)
{

    qmmMoveParseTree   * sParseTree;
    qcmTableInfo       * sTableInfo;
    smiValue           * sInsOrUptRow;
    UInt                 sColumnCount;
    mtcColumn          * sColumns;
    UInt                 sIterator;
    UShort               sCanonizedTuple;
    UShort               sCompressedTuple;
    UInt                 sOffset;

    mtcTemplate        * sMtcTemplate;
    idBool               sHasCompressedColumn;
    ULong                sRowSize = ID_ULONG(0);

    IDU_FIT_POINT_FATAL( "qmv::makeMoveRow::__FT__" );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sParseTree = (qmmMoveParseTree*) aStatement->myPlan->parseTree;
    sTableInfo = sParseTree->targetTableRef->tableInfo;
    sColumnCount = sTableInfo->columnCount;

    sHasCompressedColumn = ID_FALSE;

    // alloc insert row
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(smiValue) * sColumnCount, (void**)&sInsOrUptRow)
             != IDE_SUCCESS);

    QC_SHARED_TMPLATE(aStatement)->insOrUptRow[ sParseTree->valueIdx ] = sInsOrUptRow ;
    QC_SHARED_TMPLATE(aStatement)->insOrUptRowValueCount[ sParseTree->valueIdx ] =
        sColumnCount ;


    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtdModule*) * sColumnCount,
                                            (void**)&sParseTree->columnModules)
             != IDE_SUCCESS);

    IDE_TEST( qtc::nextTable( &sCanonizedTuple, aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    sParseTree->canonizedTuple = sCanonizedTuple;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(mtcColumn) * sColumnCount,
                 (void**) & (sMtcTemplate->rows[sCanonizedTuple].columns))
             != IDE_SUCCESS);

    for ( sIterator = 0, sOffset = 0;
          sIterator < sColumnCount;
          sIterator++ )
    {
        sColumns = sTableInfo->columns[sIterator].basicInfo;
        sParseTree->columnModules[sIterator] = sColumns->module;

        mtc::copyColumn( &(sMtcTemplate->rows[sCanonizedTuple].columns[sIterator]),
                         sColumns );

        sOffset = idlOS::align(
            sOffset,
            sColumns->module->align );

        // PROJ-1362
        if ( (sColumns->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_LOB )
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.offset
                = sOffset;
            // lob에 최대로 입력될 수 있는 값의 길이는 varchar의 최대값이다.
            sOffset += MTD_CHAR_PRECISION_MAXIMUM;
        }
        else
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.offset
                = sOffset;
            sOffset +=
                sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.size;
        }

        // PROJ-2264 Dictionary table
        if( (sColumns->column.flag & SMI_COLUMN_COMPRESSION_MASK) == SMI_COLUMN_COMPRESSION_TRUE )
        {
            sMtcTemplate->rows[sCanonizedTuple].columns[sIterator].column.flag |=
                SMI_COLUMN_COMPRESSION_TRUE;
            
            sHasCompressedColumn = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    sMtcTemplate->rows[sCanonizedTuple].modify = 0;
    sMtcTemplate->rows[sCanonizedTuple].lflag
        = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
    // fixAfterValidation에서 할당하지 않고 바로 할당한다.
    sMtcTemplate->rows[sCanonizedTuple].lflag
        &= ~MTC_TUPLE_ROW_SKIP_MASK;
    sMtcTemplate->rows[sCanonizedTuple].lflag
        |= MTC_TUPLE_ROW_SKIP_TRUE;
    sMtcTemplate->rows[sCanonizedTuple].columnCount
        = sColumnCount;
    sMtcTemplate->rows[sCanonizedTuple].columnMaximum
        = sColumnCount;
    sMtcTemplate->rows[sCanonizedTuple].columnLocate
        = NULL;
    sMtcTemplate->rows[sCanonizedTuple].execute
        = NULL;
    sMtcTemplate->rows[sCanonizedTuple].rowOffset
        = sOffset;
    sMtcTemplate->rows[sCanonizedTuple].rowMaximum
        = sOffset;

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 sOffset,
                 (void**) & (sMtcTemplate->rows[sCanonizedTuple].row) )
             != IDE_SUCCESS);

    // PROJ-2264 Dictionary table
    if( sHasCompressedColumn == ID_TRUE )
    {
        IDE_TEST( qtc::nextTable( &sCompressedTuple, aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                  != IDE_SUCCESS );

        sParseTree->compressedTuple = sCompressedTuple;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     ID_SIZEOF(mtcColumn) * sColumnCount,
                     (void**) & ( sMtcTemplate->rows[sCompressedTuple].columns))
                 != IDE_SUCCESS);

        for( sIterator = 0; sIterator < sColumnCount; sIterator++)
        {
            sColumns = sTableInfo->columns[sIterator].basicInfo;

            mtc::copyColumn( &(sMtcTemplate->rows[sCompressedTuple].columns[sIterator]),
                             sColumns );

            if( (sColumns->column.flag & SMI_COLUMN_COMPRESSION_MASK) == SMI_COLUMN_COMPRESSION_TRUE )
            {
                // smiColumn 의 size 변경
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.size =
                    ID_SIZEOF(smOID);
                sMtcTemplate->rows[sCompressedTuple].columns[sIterator].column.flag |=
                    SMI_COLUMN_COMPRESSION_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }//for

        for( sIterator = 0, sOffset = 0;
             sIterator < sColumnCount;
             sIterator++ )
        {
            sColumns = &(sMtcTemplate->rows[sCompressedTuple].columns[sIterator]);

            // BUG-37460 compress column 에서 align 을 맞추어야 합니다.
            if( (sColumns->column.flag & SMI_COLUMN_COMPRESSION_MASK) == SMI_COLUMN_COMPRESSION_TRUE )
            {
                sOffset = idlOS::align( sOffset, ID_SIZEOF(smOID) );
                // BUG-47166 valgrind error
                sColumns->column.offset = sOffset;
                sRowSize                = (ULong)sOffset + (ULong)sColumns->column.size;
                IDE_TEST_RAISE( sRowSize > (ULong)ID_UINT_MAX, ERR_EXCEED_TUPLE_ROW_MAX_SIZE );

                sOffset = (UInt)sRowSize;
            }
            else
            {
                // BUG-47166 valgrind error
                // Compress column이 아닌 경우 dummy mtcColumn으로만듭니다.
                sColumns->column.offset = 0;
                sColumns->column.size = 0;
            }
        }

        sMtcTemplate->rows[sCompressedTuple].modify = 0;
        sMtcTemplate->rows[sCompressedTuple].lflag
            = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
        // fixAfterValidation에서 할당하지 않고 바로 할당한다.
        sMtcTemplate->rows[sCompressedTuple].lflag &= ~MTC_TUPLE_ROW_SKIP_MASK;
        sMtcTemplate->rows[sCompressedTuple].lflag |=  MTC_TUPLE_ROW_SKIP_TRUE;
        sMtcTemplate->rows[sCompressedTuple].columnCount    = sColumnCount;
        sMtcTemplate->rows[sCompressedTuple].columnMaximum  = sColumnCount;
        sMtcTemplate->rows[sCompressedTuple].columnLocate   = NULL;
        sMtcTemplate->rows[sCompressedTuple].execute        = NULL;
        sMtcTemplate->rows[sCompressedTuple].rowOffset      = sOffset;
        sMtcTemplate->rows[sCompressedTuple].rowMaximum     = sOffset;

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     sOffset,
                     (void**) &(sMtcTemplate->rows[sCompressedTuple].row) )
                 != IDE_SUCCESS);
    }
    else
    {
        // compressed tuple 을 사용하지 않음
        sParseTree->compressedTuple = UINT_MAX;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_TUPLE_ROW_MAX_SIZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmv::makeMoveRow",
                                  "tuple row size is larger than 4GB" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

// PR-13725
IDE_RC qmv::checkInsertOperatable( qcStatement      * aStatement,
                                   qmmInsParseTree  * aParseTree,
                                   qcmTableInfo     * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    qcuSqlSourceInfo      sqlInfo;

    IDU_FIT_POINT_FATAL( "qmv::checkInsertOperatable::__FT__" );
    
    //------------------------------------------
    // instead of trigger 조사
    //------------------------------------------

    if ( aParseTree->insteadOfTrigger == ID_FALSE )
    {
        // PR-13725
        // CHECK OPERATABLE
        if( QCM_IS_OPERATABLE_QP_INSERT( aTableInfo->operatableFlag )
            != ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aParseTree->tableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::checkUpdateOperatable( qcStatement  * aStatement,
                                   idBool         aIsInsteadOfTrigger,
                                   qmsTableRef  * aTableRef )
{
    qcuSqlSourceInfo sqlInfo;

    IDU_FIT_POINT_FATAL( "qmv::checkUpdateOperatable::__FT__" );

    //------------------------------------------
    // instead of trigger 조사
    //------------------------------------------
    if ( aIsInsteadOfTrigger == ID_FALSE )
    {
        if ( QCM_IS_OPERATABLE_QP_UPDATE( aTableRef->tableInfo->operatableFlag )
             != ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement, &aTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::checkDeleteOperatable( qcStatement  * aStatement,
                                   idBool         aIsInsteadOfTrigger,
                                   qmsTableRef  * aTableRef )
{
    qcuSqlSourceInfo      sqlInfo;

    IDU_FIT_POINT_FATAL( "qmv::checkDeleteOperatable::__FT__" );

    //------------------------------------------
    // instead of trigger 조사
    //------------------------------------------
    if ( aIsInsteadOfTrigger == ID_FALSE )
    {
        if ( QCM_IS_OPERATABLE_QP_DELETE( aTableRef->tableInfo->operatableFlag )
             != ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement, &aTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmv::describeParamInfo( qcStatement * aStatement,
                               mtcColumn   * aColumn,
                               qtcNode     * aValue )
{
/***********************************************************************
 *
 * Description :
 *     BUG-15746
 *     insertValue, updateValue시 value 노드에 host변수를 bind할 경우에
 *     한해 meta column정보를 bindParamInfo에 저장한다.
 *     (SQLDescribeParam을 지원하기 위해 구현됨)
 *
 * Implementation :
 *
 ***********************************************************************/

    qciBindParam * sBindParam;
    qcTemplate   * sTemplate;
    UShort         sTuple;

    IDU_FIT_POINT_FATAL( "qmv::describeParamInfo::__FT__" );

    sTemplate = QC_SHARED_TMPLATE(aStatement);
    sTuple    = sTemplate->tmplate.variableRow;

    if( ( ( aValue->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) &&
        ( aValue->node.module == & qtc::valueModule ) &&
        ( aValue->node.table == sTuple ) &&
        ( aValue->node.column < aStatement->myPlan->sBindParamCount ) &&
        ( sTuple != ID_USHORT_MAX ) )
    {
        sBindParam = & aStatement->myPlan->sBindParam[aValue->node.column].param;

        // PROJ-2002 Column Security
        // 암호 데이타 타입은 원본 데이타 타입으로 Bind Parameter 정보 설정
        if( aColumn->type.dataTypeId == MTD_ECHAR_ID )
        {
            sBindParam->type      = MTD_CHAR_ID;
            sBindParam->language  = aColumn->type.languageId;
            sBindParam->arguments = 1;
            sBindParam->precision = aColumn->precision;
            sBindParam->scale     = 0;
        }
        else if( aColumn->type.dataTypeId == MTD_EVARCHAR_ID )
        {
            sBindParam->type      = MTD_VARCHAR_ID;
            sBindParam->language  = aColumn->type.languageId;
            sBindParam->arguments = 1;
            sBindParam->precision = aColumn->precision;
            sBindParam->scale     = 0;
        }
        else
        {
            sBindParam->type      = aColumn->type.dataTypeId;
            sBindParam->language  = aColumn->type.languageId;
            sBindParam->arguments = aColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;
            sBindParam->precision = aColumn->precision;
            sBindParam->scale     = aColumn->scale;
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC qmv::setFetchColumnInfo4ParentTable( qcStatement * aStatement,
                                            qmsTableRef * aTableRef )
{
    mtcTuple        * sTuple;
    UInt              sForeignKeyCnt;
    UInt              sColumnCnt;
    UInt              sColumnOrder;
    qcmForeignKey   * sForeignKey;
    qcmTableInfo    * sTableInfo;

    IDU_FIT_POINT_FATAL( "qmv::setFetchColumnInfo4ParentTable::__FT__" );

    sTableInfo = aTableRef->tableInfo;

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 수행시 parent table에 대한 foreign key column check
    //---------------------------------------------

    /* PROJ-2464 hybrid partitioned table 지원
     *  INSERT, MOVE, UPDATE에서 사용하는데,
     *  MTC_COLUMN_USE_COLUMN_TRUE는 여러 곳에서 사용하므로, 저장 매체에 상관없이 설정한다.
     */
    sTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table]);

    for ( sForeignKeyCnt = 0 ;
          sForeignKeyCnt < sTableInfo->foreignKeyCount;
          sForeignKeyCnt++ )
    {
        sForeignKey = &(sTableInfo->foreignKeys[sForeignKeyCnt]);

        for ( sColumnCnt = 0;
              sColumnCnt < sForeignKey->constraintColumnCount;
              sColumnCnt++ )
        {
            sColumnOrder =
                sForeignKey->referencingColumn[sColumnCnt] & SMI_COLUMN_ID_MASK;
            sTuple->columns[sColumnOrder].flag |= MTC_COLUMN_USE_COLUMN_TRUE;
        }
    }
    
    return IDE_SUCCESS;
    
}


IDE_RC qmv::setFetchColumnInfo4ChildTable( qcStatement * aStatement,
                                           qmsTableRef * aTableRef )
{
    mtcTuple        * sTuple;
    UInt              sUniqueKeyCnt;
    UInt              sColumnCnt;
    UInt              sColumnOrder;
    qcmUnique       * sUniqueKey;
    qcmTableInfo    * sTableInfo;

    IDU_FIT_POINT_FATAL( "qmv::setFetchColumnInfo4ChildTable::__FT__" );

    sTableInfo = aTableRef->tableInfo;

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 수행시 child table에 대한 foreign key column check
    //---------------------------------------------

    /* PROJ-2464 hybrid partitioned table 지원
     *  DELETE, MOVE, UPDATE에서 사용하는데,
     *  MTC_COLUMN_USE_COLUMN_TRUE는 여러 곳에서 사용하므로, 저장 매체에 상관없이 설정한다.
     */
    sTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table]);

    for ( sUniqueKeyCnt = 0 ;
          sUniqueKeyCnt < sTableInfo->uniqueKeyCount;
          sUniqueKeyCnt++ )
    {
        sUniqueKey = &(sTableInfo->uniqueKeys[sUniqueKeyCnt]);

        for ( sColumnCnt = 0;
              sColumnCnt < sUniqueKey->constraintColumnCount;
              sColumnCnt++ )
        {
            sColumnOrder =
                sUniqueKey->constraintColumn[sColumnCnt] & SMI_COLUMN_ID_MASK;
            sTuple->columns[sColumnOrder].flag |= MTC_COLUMN_USE_COLUMN_TRUE;
        }
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC qmv::setFetchColumnInfo4Trigger( qcStatement * aStatement,
                                        qmsTableRef * aTableRef )
{
    mtcTuple        * sTuple;
    qcmTableInfo    * sTableInfo;
    UInt              i;

    IDU_FIT_POINT_FATAL( "qmv::setFetchColumnInfo4Trigger::__FT__" );

    sTableInfo = aTableRef->tableInfo;

    //---------------------------------------------
    // PROJ-1705 질의에 사용된 컬럼정보수집
    // DML 수행시 trigger에 의한 column check
    //---------------------------------------------

    /* PROJ-2464 hybrid partitioned table 지원
     *  INSERT, DELETE, MOVE, UPDATE에서 사용하는데,
     *  MTC_COLUMN_USE_COLUMN_TRUE는 여러 곳에서 사용하므로, 저장 매체에 상관없이 설정한다.
     */
    if ( sTableInfo->triggerCount > 0 )
    {
        sTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table]);

        for ( i = 0; i < sTuple->columnCount; i++ )
        {
            sTuple->columns[i].flag |= MTC_COLUMN_USE_COLUMN_TRUE;
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;
    
}

IDE_RC qmv::sortUpdateColumn( qcStatement   * aStatement,
                              qcmColumn    ** aColumns,
                              UInt            aColumnCount,
                              qmmValueNode ** aValue,
                              qmmValueNode ** aValuesPos )
{
/***********************************************************************
 *
 *  Description :
 *
 *  update column을 테이블생성컬럼순서로 정렬하기 위한 함수
 *  레코드 구조가 바뀜에 따라
 *  sm에서 column을 테이블생성컬럼순서로 처리할 수 있도록
 *  update column을 테이블생성컬럼순서로 정렬한다.
 *  즉, update시 sm에 내려주는 smiValue가 테이블생성컬럼순서대로 ...
 *
 *  PROJ-2219 Row-level before update trigger
 *  기존에는 disk table인 경우에만 sort했지만,
 *  PROJ-2219 이후에는 무조건 column id 순으로 정렬한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    qmvUpdateColumnIdx * sUpdateColumnArr;
    qcmColumn          * sUpdateColumn;
    qmmValueNode       * sUpdateValue;
    qmmValueNode      ** sValuesPos;
    UInt                 sUpdateColumnCount;
    UInt                 i;
    UInt                 j;
    idBool               sIsNeedSort = ID_FALSE;
    UInt                 sOrder;

    IDU_FIT_POINT_FATAL( "qmv::sortUpdateColumn::__FT__" );

    sUpdateColumnCount = aColumnCount;
    sUpdateColumn      = *aColumns;

    for ( i = 0; sUpdateColumn != NULL; i++ )
    {
        if ( sUpdateColumn->next == NULL )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }

        if ( sUpdateColumn->basicInfo->column.id >
             sUpdateColumn->next->basicInfo->column.id )
        {
            sIsNeedSort = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }

        sUpdateColumn = sUpdateColumn->next;
    }

    // 이미 column ID순으로 정렬되어 있으면 아래의 과정을 수행하지 않는다.
    if ( sIsNeedSort == ID_TRUE )
    {
        IDE_FT_ERROR( sUpdateColumnCount > 1 );

        IDU_FIT_POINT( "qmv::sortUpdateColumn::alloc::sUpdateColumnArr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( QC_QME_MEM(aStatement)->alloc(
                      ID_SIZEOF(qmvUpdateColumnIdx) * sUpdateColumnCount,
                      (void**)&sUpdateColumnArr )
                  != IDE_SUCCESS);

        sUpdateColumn = *aColumns;
        sUpdateValue  = *aValue;

        for ( i = 0; sUpdateColumn != NULL; i++ )
        {
            IDE_FT_ERROR( sUpdateValue != NULL );

            sUpdateColumnArr[i].column = sUpdateColumn;
            sUpdateColumnArr[i].value  = sUpdateValue;

            sUpdateColumn = sUpdateColumn->next;
            sUpdateValue  = sUpdateValue->next;
        }

        idlOS::qsort( sUpdateColumnArr,
                      sUpdateColumnCount,
                      ID_SIZEOF(qmvUpdateColumnIdx),
                      compareUpdateColumnID );

        if ( aValuesPos != NULL )
        {
            IDE_TEST( QC_QME_MEM(aStatement)->alloc(
                          ID_SIZEOF(qmmValueNode *) * sUpdateColumnCount,
                          (void**)&sValuesPos )
                      != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }

        sUpdateColumnCount--;

        // 테이블생성컬럼순서대로 update column 재구성
        for ( i = 0; i < sUpdateColumnCount; i++ )
        {
            if ( aValuesPos != NULL )
            {
                sOrder = sUpdateColumnArr[i].value->order;
                sValuesPos[i] = aValuesPos[sOrder];
            }
            else
            {
                /* Nothing to do */
            }
            sUpdateColumnArr[i].column->next = sUpdateColumnArr[i+1].column;
            sUpdateColumnArr[i].value->order = i;
            sUpdateColumnArr[i].value->next  = sUpdateColumnArr[i+1].value;
        }

        if ( aValuesPos != NULL )
        {
            sOrder = sUpdateColumnArr[i].value->order;
            sValuesPos[i] = aValuesPos[sOrder];

            for ( j = 0; j < aColumnCount; j++ )
            {
                aValuesPos[j] = sValuesPos[j];
            }
        }
        else
        {
            /* Nothing to do */
        }

        // 마지막 배열의 next는 NULL
        sUpdateColumnArr[i].column->next = NULL;
        sUpdateColumnArr[i].value->order = i;
        sUpdateColumnArr[i].value->next  = NULL;

        // 정렬된 update column과 value 정보 설정.
        *aColumns = sUpdateColumnArr[0].column;
        *aValue   = sUpdateColumnArr[0].value;

    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::validatePlanHints( qcStatement * aStatement,
                               qmsHints    * aHints )
{

    IDU_FIT_POINT_FATAL( "qmv::validatePlanHints::__FT__" );

    IDE_DASSERT( aHints != NULL );

    if ( aHints->noPlanCache == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_PLAN_CACHE_IN_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_PLAN_CACHE_IN_OFF;
    }
    else
    {
        // Nothing to do.
    }

    if ( aHints->keepPlan == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_PLAN_KEEP_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_PLAN_KEEP_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41615 simple query hint
    if ( aHints->execFastHint == QMS_EXEC_FAST_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_EXEC_FAST_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_EXEC_FAST_TRUE;
    }
    else
    {
        if ( aHints->execFastHint == QMS_EXEC_FAST_FALSE )
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_EXEC_FAST_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_EXEC_FAST_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
   
    // BUG-46137
    if ( aHints->planCacheKeep == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_PLAN_CACHE_KEEP_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_PLAN_CACHE_KEEP_TRUE;
    }

    return IDE_SUCCESS;
    
}

IDE_RC qmv::addDecryptFunc4ValueNode( qcStatement  * aStatement,
                                      qmmValueNode * aValueNode )
{
    qcNamePosition      sNullPosition;
    qtcNode           * sNode[2];

    IDU_FIT_POINT_FATAL( "qmv::addDecryptFunc4ValueNode::__FT__" );

    SET_EMPTY_POSITION( sNullPosition );

    // decrypt 함수를 만든다.
    IDE_TEST( qtc::makeNode( aStatement,
                             sNode,
                             & sNullPosition,
                             & mtfDecrypt )
              != IDE_SUCCESS );

    // 함수를 연결한다.
    sNode[0]->node.arguments = (mtcNode*) aValueNode->value;
    sNode[0]->node.next = aValueNode->value->node.next;
    sNode[0]->node.arguments->next = NULL;

    // next가 없다.
    IDE_DASSERT( sNode[0]->node.next == NULL );

    // 함수만 estimate 를 수행한다.
    IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                             sNode[0] )
              != IDE_SUCCESS );

    // target 노드를 바꾼다.
    aValueNode->value = sNode[0];
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1888 INSTEAD OF TRIGGER */
IDE_RC qmv::checkInsteadOfTrigger( qmsTableRef * aTableRef,
                                   UInt          aEventType,
                                   idBool      * aTriggerExist )
{
    UInt           i;
    idBool         sTriggerExist = ID_FALSE;
    qcmTableInfo * sTableInfo;

    IDU_FIT_POINT_FATAL( "qmv::checkInsteadOfTrigger::__FT__" );

    sTableInfo = aTableRef->tableInfo;

    //------------------------------------------
    // instead of trigger 조사
    //------------------------------------------
    if ( sTableInfo->triggerCount == 0 )
    {
        /* Nothing to do. */
    }
    else
    {
        for ( i = 0;  i < sTableInfo->triggerCount; i++ )
        {
            if ( ( sTableInfo->triggerInfo[i].enable == QCM_TRIGGER_ENABLE ) &&
                 ( sTableInfo->triggerInfo[i].eventTime == QCM_TRIGGER_INSTEAD_OF ) &&
                 ( ( sTableInfo->triggerInfo[i].eventType & aEventType ) != 0 ) )
            {
                IDE_TEST_RAISE( sTableInfo->triggerInfo[i].granularity != QCM_TRIGGER_ACTION_EACH_ROW,
                                ERR_GRANULARITY );

                sTriggerExist = ID_TRUE;
                break;
            }
        }
    }

    *aTriggerExist = sTriggerExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GRANULARITY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmv::checkInsteadOfTrigger",
                                  "invalid trigger granularity" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* PROJ-1988 Implement MERGE statement */
IDE_RC qmv::parseMerge( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     MERGE INTO ... USING ... ON ...
 *         WHEN MATCHED THEN UPDATE ...
 *         WHEN NOT MATCHED THEN INSERT ...
 *         WHEN NO ROWS THEN INSERT ...
 *
 * Implementation :
 *     1. generate and complete first SELECT statement (select * from using_table)
 *     2. generate and complete second SELECT statement (select count(*) from into_table where on_expr)
 *     3. generate and complete UPDATE statement parse tree and call UPDATE statement validation
 *     4. generate and complete WHEN NOT MATCHED THEN INSERT statement parse tree and call INSERT statement validation
 *     5. generate and complete WHEN NO ROWS THEN INSERT statement parse tree and call INSERT statement validation
 *
 ***********************************************************************/

    qmmMergeParseTree   * sParseTree;

    qmsParseTree        * sSelectParseTree;
    qcStatement         * sStatement;
    qmsQuerySet         * sQuerySet;
    qmsSFWGH            * sSFWGH;
    qmsFrom             * sFrom;
    qmsTableRef         * sTableRef;
    qtcNode             * sAndNode[2];
    qtcNode             * sArgNode1;
    qtcNode             * sArgNode2;
    qcNamePosition        sNullPosition;

    IDU_FIT_POINT_FATAL( "qmv::parseMerge::__FT__" );

    sParseTree = (qmmMergeParseTree *) aStatement->myPlan->parseTree;

    SET_EMPTY_POSITION( sNullPosition );

    /*******************************************************/
    /* MAKE NEW qcStatements (SELECT2/UPDATE/DELETE/INSERT) */
    /*******************************************************/

    /***************************/
    /* selectSource statement */
    /***************************/
    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsParseTree, &sSelectParseTree)
              != IDE_SUCCESS );
    QC_SET_INIT_PARSE_TREE(sSelectParseTree, sParseTree->source->startPos);

    sParseTree->selectSourceParseTree = sSelectParseTree;

    sSelectParseTree->withClause = NULL;
    sSelectParseTree->querySet = sParseTree->source;
    sSelectParseTree->limit = NULL;
    sSelectParseTree->loopNode = NULL;
    sSelectParseTree->forUpdate = NULL;
    sSelectParseTree->queue = NULL;
    sSelectParseTree->orderBy = NULL;
    sSelectParseTree->isSiblings = ID_FALSE;
    sSelectParseTree->isTransformed = ID_FALSE;

    sSelectParseTree->common.parse = qmv::parseSelect;
    sSelectParseTree->common.validate = qmv::validateSelect;
    sSelectParseTree->common.optimize = qmo::optimizeSelect;
    sSelectParseTree->common.execute = qmx::executeSelect;

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sStatement)
             != IDE_SUCCESS);
    QC_SET_STATEMENT(sStatement, aStatement, sSelectParseTree);

    sSelectParseTree->common.stmt = sStatement;
    sSelectParseTree->common.stmtKind = QCI_STMT_SELECT;

    sStatement->myPlan->parseTree = (qcParseTree *)sSelectParseTree;

    sParseTree->selectSourceStatement = sStatement;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseSelectInternal( sParseTree->selectSourceStatement )
             != IDE_SUCCESS );

    IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                             sStatement->mFlag )
              != IDE_SUCCESS );

    /***************************/
    /* selectTarget statement */
    /***************************/
    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsParseTree, &sSelectParseTree)
              != IDE_SUCCESS );
    QC_SET_INIT_PARSE_TREE(sSelectParseTree, sParseTree->target->startPos);

    sParseTree->selectTargetParseTree = sSelectParseTree;

    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsQuerySet, &sQuerySet)
              != IDE_SUCCESS );
    idlOS::memcpy(sQuerySet, sParseTree->target, ID_SIZEOF(qmsQuerySet));

    sSelectParseTree->querySet = sQuerySet;

    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsSFWGH, &sSFWGH)
              != IDE_SUCCESS );
    idlOS::memcpy(sSFWGH, sParseTree->target->SFWGH, ID_SIZEOF(qmsSFWGH));

    sSelectParseTree->querySet->SFWGH = sSFWGH;
    sSFWGH->thisQuerySet = sQuerySet;

    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsFrom, &sFrom)
              != IDE_SUCCESS);
    idlOS::memcpy(sFrom, sParseTree->target->SFWGH->from, ID_SIZEOF(qmsFrom));

    sSFWGH->from = sFrom;

    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
              != IDE_SUCCESS );
    idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

    // partitionRef가 있는 경우 복사한다.
    IDE_TEST( qcmPartition::copyPartitionRef(
                  aStatement,
                  sParseTree->target->SFWGH->from->tableRef->partitionRef,
                  & sTableRef->partitionRef )
              != IDE_SUCCESS );

    sSFWGH->from->tableRef = sTableRef;

    sSFWGH->where = sParseTree->onExpr;

    sSelectParseTree->withClause = NULL;
    sSelectParseTree->limit = NULL;
    sSelectParseTree->loopNode = NULL;
    sSelectParseTree->forUpdate = NULL;
    sSelectParseTree->queue = NULL;
    sSelectParseTree->orderBy = NULL;
    sSelectParseTree->isSiblings = ID_FALSE;
    sSelectParseTree->isTransformed = ID_FALSE;

    sSelectParseTree->common.parse = qmv::parseSelect;
    sSelectParseTree->common.validate = qmv::validateSelect;
    sSelectParseTree->common.optimize = qmo::optimizeSelect;
    sSelectParseTree->common.execute = qmx::executeSelect;

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sStatement)
             != IDE_SUCCESS);
    QC_SET_STATEMENT(sStatement, aStatement, sSelectParseTree);

    sSelectParseTree->common.stmt = sStatement;
    sSelectParseTree->common.stmtKind = QCI_STMT_SELECT;

    sStatement->myPlan->parseTree = (qcParseTree *)sSelectParseTree;

    sParseTree->selectTargetStatement = sStatement;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( parseSelectInternal( sParseTree->selectTargetStatement )
             != IDE_SUCCESS );

    IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                             sStatement->mFlag )
              != IDE_SUCCESS );

    /***************************/
    /* update statement */
    /***************************/
    if (sParseTree->updateParseTree != NULL)
    {
        sParseTree->updateParseTree->common.stmt = sParseTree->common.stmt;
        sParseTree->updateParseTree->common.stmtKind = QCI_STMT_UPDATE;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sStatement)
                  != IDE_SUCCESS );
        QC_SET_STATEMENT(sStatement, aStatement, sParseTree->updateParseTree);

        sStatement->myPlan->parseTree = (qcParseTree *)sParseTree->updateParseTree;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsQuerySet, &sQuerySet)
                  != IDE_SUCCESS );
        idlOS::memcpy(sQuerySet, sParseTree->target, ID_SIZEOF(qmsQuerySet));

        sParseTree->updateParseTree->querySet = sQuerySet;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsSFWGH, &sSFWGH)
                  != IDE_SUCCESS );
        idlOS::memcpy(sSFWGH, sParseTree->target->SFWGH, ID_SIZEOF(qmsSFWGH));

        sParseTree->updateParseTree->querySet->SFWGH = sSFWGH;
        sSFWGH->thisQuerySet = sQuerySet;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsFrom, &sFrom)
                  != IDE_SUCCESS);
        idlOS::memcpy(sFrom, sParseTree->target->SFWGH->from, ID_SIZEOF(qmsFrom));

        sSFWGH->from = sFrom;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
                  != IDE_SUCCESS );
        idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

        // partitionRef가 있는 경우 복사한다.
        IDE_TEST( qcmPartition::copyPartitionRef(
                      aStatement,
                      sParseTree->target->SFWGH->from->tableRef->partitionRef,
                      & sTableRef->partitionRef )
                  != IDE_SUCCESS );

        sSFWGH->from->tableRef = sTableRef;

        /* PROJ-1090 Function-based Index */
        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
                  != IDE_SUCCESS );
        idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

        // partitionRef가 있는 경우 복사한다.
        IDE_TEST( qcmPartition::copyPartitionRef(
                      aStatement,
                      sParseTree->target->SFWGH->from->tableRef->partitionRef,
                      & sTableRef->partitionRef )
                  != IDE_SUCCESS );

        sParseTree->updateParseTree->defaultTableRef = sTableRef;

        sParseTree->updateStatement = sStatement;
        sParseTree->updateParseTree->common.stmt = sStatement;

        /* where clause fix (update where) */
        if (sParseTree->whereForUpdate != NULL)
        {
            IDE_TEST( qtc::copyNodeTree(aStatement, sParseTree->onExpr, &sArgNode1,
                                        ID_FALSE, ID_TRUE ) != IDE_SUCCESS);
            IDE_TEST( qtc::copyNodeTree(aStatement, sParseTree->whereForUpdate, &sArgNode2,
                                        ID_FALSE, ID_TRUE ) != IDE_SUCCESS);
            sArgNode1->node.next = (mtcNode *)(sArgNode2);

            IDE_TEST( qtc::makeNode(aStatement, sAndNode, & sNullPosition, (const UChar *)"AND", 3)
                      != IDE_SUCCESS );

            sAndNode[0]->node.arguments = (mtcNode *)sArgNode1;
            sAndNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
            sAndNode[0]->node.lflag |= 2;
        }
        else
        {
            /* BUG-39400 SQL:2003 지원 */
            IDE_TEST( qtc::copyNodeTree(aStatement, sParseTree->onExpr, &sArgNode1,
                                        ID_FALSE, ID_TRUE ) != IDE_SUCCESS);

            sAndNode[0] = sArgNode1;
        }

        sParseTree->updateParseTree->querySet->SFWGH->where = sAndNode[0];

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( parseUpdateInternal( sParseTree->updateStatement )
                  != IDE_SUCCESS );

        IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                 sStatement->mFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    /***************************/
    /* delete statement */
    /***************************/
    if (sParseTree->deleteParseTree != NULL)
    {
        sParseTree->deleteParseTree->common.stmt = sParseTree->common.stmt;
        sParseTree->deleteParseTree->common.stmtKind = QCI_STMT_DELETE;

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sStatement)
                 != IDE_SUCCESS);
        QC_SET_STATEMENT(sStatement, aStatement, sParseTree->deleteParseTree);

        sStatement->myPlan->parseTree = (qcParseTree *)sParseTree->deleteParseTree;

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsQuerySet, &sQuerySet)
                 != IDE_SUCCESS);
        idlOS::memcpy(sQuerySet, sParseTree->target, ID_SIZEOF(qmsQuerySet));

        sParseTree->deleteParseTree->querySet = sQuerySet;

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsSFWGH, &sSFWGH)
                 != IDE_SUCCESS);
        idlOS::memcpy(sSFWGH, sParseTree->target->SFWGH, ID_SIZEOF(qmsSFWGH));

        sParseTree->deleteParseTree->querySet->SFWGH = sSFWGH;
        sSFWGH->thisQuerySet = sQuerySet;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsFrom, &sFrom)
                  != IDE_SUCCESS);
        idlOS::memcpy(sFrom, sParseTree->target->SFWGH->from, ID_SIZEOF(qmsFrom));

        sSFWGH->from = sFrom;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
                  != IDE_SUCCESS );
        idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

        // partitionRef가 있는 경우 복사한다.
        IDE_TEST( qcmPartition::copyPartitionRef(
                      aStatement,
                      sParseTree->target->SFWGH->from->tableRef->partitionRef,
                      & sTableRef->partitionRef )
                  != IDE_SUCCESS );
        
        sSFWGH->from->tableRef = sTableRef;

        sParseTree->deleteStatement = sStatement;
        sParseTree->deleteParseTree->common.stmt = sStatement;

        /* where clause fix (update where + delete where) */
        if (sParseTree->whereForDelete != NULL)
        {
            IDE_TEST( qtc::copyNodeTree(aStatement,
                                        sParseTree->updateParseTree->querySet->SFWGH->where, &sArgNode1,
                                        ID_FALSE, ID_TRUE) != IDE_SUCCESS);
            IDE_TEST( qtc::copyNodeTree(aStatement,
                                        sParseTree->whereForDelete, &sArgNode2,
                                        ID_FALSE, ID_TRUE) != IDE_SUCCESS);
            sArgNode1->node.next = (mtcNode *)(sArgNode2);

            IDE_TEST( qtc::makeNode(aStatement, sAndNode, & sNullPosition, (const UChar *)"AND", 3)
                      != IDE_SUCCESS );

            sAndNode[0]->node.arguments = (mtcNode *)sArgNode1;
            sAndNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
            sAndNode[0]->node.lflag |= 2;
        }
        else
        {
            IDE_TEST( qtc::copyNodeTree(aStatement,
                                        sParseTree->updateParseTree->querySet->SFWGH->where, &sArgNode1,
                                        ID_FALSE, ID_TRUE) != IDE_SUCCESS);
            sAndNode[0] = sArgNode1;
        }
        sParseTree->deleteParseTree->querySet->SFWGH->where = sAndNode[0];

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( parseDeleteInternal( sParseTree->deleteStatement )
                  != IDE_SUCCESS );

        IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                 sStatement->mFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /***************************/
    /* insert statement */
    /***************************/
    if (sParseTree->insertParseTree != NULL)
    {
        sParseTree->insertParseTree->common.stmt = sParseTree->common.stmt;
        sParseTree->insertParseTree->common.stmtKind = QCI_STMT_INSERT;

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sStatement)
                 != IDE_SUCCESS);
        QC_SET_STATEMENT(sStatement, aStatement, sParseTree->insertParseTree);

        sStatement->myPlan->parseTree = (qcParseTree *)sParseTree->insertParseTree;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
                  != IDE_SUCCESS );
        idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

        // partitionRef가 있는 경우 복사한다.
        IDE_TEST( qcmPartition::copyPartitionRef(
                      aStatement,
                      sParseTree->target->SFWGH->from->tableRef->partitionRef,
                      & sTableRef->partitionRef )
                  != IDE_SUCCESS );

        sParseTree->insertParseTree->tableRef = sTableRef;

        /* PROJ-1090 Function-based Index */
        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
                  != IDE_SUCCESS );
        idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

        // partitionRef가 있는 경우 복사한다.
        IDE_TEST( qcmPartition::copyPartitionRef(
                      aStatement,
                      sParseTree->target->SFWGH->from->tableRef->partitionRef,
                      & sTableRef->partitionRef )
                  != IDE_SUCCESS );

        sParseTree->insertParseTree->defaultTableRef = sTableRef;

        sParseTree->insertParseTree->outerQuerySet = sParseTree->source;

        sParseTree->insertStatement = sStatement;
        sParseTree->insertParseTree->common.stmt = sStatement;
        
        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( parseInsertValuesInternal( sParseTree->insertStatement )
                  != IDE_SUCCESS );

        IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                 sStatement->mFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /***************************/
    /* insertNoRows statement */
    /***************************/
    if (sParseTree->insertNoRowsParseTree != NULL)
    {
        sParseTree->insertNoRowsParseTree->common.stmt = sParseTree->common.stmt;
        sParseTree->insertNoRowsParseTree->common.stmtKind = QCI_STMT_INSERT;

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sStatement)
                 != IDE_SUCCESS);
        QC_SET_STATEMENT(sStatement, aStatement, sParseTree->insertNoRowsParseTree);

        sStatement->myPlan->parseTree = (qcParseTree *)sParseTree->insertNoRowsParseTree;

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
                  != IDE_SUCCESS );
        idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

        // partitionRef가 있는 경우 복사한다.
        IDE_TEST( qcmPartition::copyPartitionRef(
                      aStatement,
                      sParseTree->target->SFWGH->from->tableRef->partitionRef,
                      & sTableRef->partitionRef )
                  != IDE_SUCCESS );

        sParseTree->insertNoRowsParseTree->tableRef = sTableRef;

        /* PROJ-1090 Function-based Index */
        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTableRef, &sTableRef)
                  != IDE_SUCCESS );
        idlOS::memcpy(sTableRef, sParseTree->target->SFWGH->from->tableRef, ID_SIZEOF(qmsTableRef));

        // partitionRef가 있는 경우 복사한다.
        IDE_TEST( qcmPartition::copyPartitionRef(
                      aStatement,
                      sParseTree->target->SFWGH->from->tableRef->partitionRef,
                      & sTableRef->partitionRef )
                  != IDE_SUCCESS );

        sParseTree->insertNoRowsParseTree->defaultTableRef = sTableRef;

        sParseTree->insertNoRowsStatement = sStatement;
        sParseTree->insertNoRowsParseTree->common.stmt = sStatement;

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( parseInsertValuesInternal( sParseTree->insertNoRowsStatement )
                  != IDE_SUCCESS );

        IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                 sStatement->mFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmv::validateMerge( qcStatement * aStatement )
{
    qmmMergeParseTree   * sParseTree;
    qmsTableRef         * sTargetTableRef;
    qcmTableInfo        * sTargetTableInfo;
    qcmColumn           * sColumn;
    idBool                sFound;
    qcuSqlSourceInfo      sqlInfo;
    qmsSFWGH            * sSFWGH;
    
    IDU_FIT_POINT_FATAL( "qmv::validateMerge::__FT__" );

    sParseTree = (qmmMergeParseTree *) aStatement->myPlan->parseTree;

    /* calling validate functions for SELECT/UPDATE/DELETE/INSERT */
    IDE_TEST( sParseTree->selectSourceParseTree->common.validate(
                  sParseTree->selectSourceStatement )
              != IDE_SUCCESS );

    IDE_TEST( sParseTree->selectTargetParseTree->common.validate(
                  sParseTree->selectTargetStatement )
              != IDE_SUCCESS );

    sTargetTableRef = sParseTree->selectTargetParseTree->
        querySet->SFWGH->from->tableRef;
    sTargetTableInfo = sTargetTableRef->tableInfo;

    if ( sParseTree->updateStatement != NULL )
    {
        // PR-13725
        // CHECK OPERATABLE
        if ( QCM_IS_OPERATABLE_QP_UPDATE( sTargetTableInfo->operatableFlag )
             != ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sTargetTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( sParseTree->updateParseTree->common.validate(
                      sParseTree->updateStatement )
                  != IDE_SUCCESS );

        // BUG-38398
        // updateParseTree->columns 대신
        // updateParseTree->updateColumns를 사용해야 합니다.
        for (sColumn = sParseTree->updateParseTree->updateColumns;
             sColumn != NULL;
             sColumn = sColumn->next)
        {
            sFound = ID_FALSE;

            IDE_TEST( searchColumnInExpression(sParseTree->updateStatement,
                                               sParseTree->onExpr,
                                               sColumn,
                                               &sFound )
                      != IDE_SUCCESS);

            if ( sFound == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement, & sColumn->namePos );
                IDE_RAISE( ERR_REFERENCED_COLUMNS_CANNOT_BE_UPDATED );
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* nothing to do */
    }

    if ( sParseTree->deleteParseTree != NULL )
    {
        // PR-13725
        // CHECK OPERATABLE
        if ( QCM_IS_OPERATABLE_QP_DELETE( sTargetTableInfo->operatableFlag )
             != ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sTargetTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
        else
        {
            /* nothing to do */
        }
        
        IDE_TEST( sParseTree->deleteParseTree->common.validate(
                      sParseTree->deleteStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    if ( sParseTree->insertParseTree != NULL )
    {
        // PR-13725
        // CHECK OPERATABLE
        if ( QCM_IS_OPERATABLE_QP_INSERT( sTargetTableInfo->operatableFlag )
             != ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sTargetTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( sParseTree->insertParseTree->common.validate(
                      sParseTree->insertStatement )
                  != IDE_SUCCESS );

        /* estimate WHERE for insert statement */
        if ( sParseTree->whereForInsert != NULL )
        {
            IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsSFWGH, &sSFWGH)
                      != IDE_SUCCESS );
            /* BUG-48521 insert의 where 조건은 source 를 참조해야한다 */
            idlOS::memcpy( (void*)sSFWGH,
                           (void*)sParseTree->selectSourceParseTree->querySet->SFWGH,
                           ID_SIZEOF(qmsSFWGH) );
            
            sSFWGH->where = sParseTree->whereForInsert;

            IDE_TEST( qmvQuerySet::validateWhere(
                          aStatement,
                          NULL, // querySet : SELECT 구문만 필요
                          sSFWGH )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    if ( sParseTree->insertNoRowsParseTree != NULL )
    {
        // PR-13725
        // CHECK OPERATABLE
        if ( QCM_IS_OPERATABLE_QP_INSERT( sTargetTableInfo->operatableFlag )
             != ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sTargetTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( sParseTree->insertNoRowsParseTree->common.validate(
                      sParseTree->insertNoRowsStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /*
     * BUG-39441
     * need a interface which returns whether DML on replication table or not
     */
    if (sTargetTableInfo->replicationCount > 0)
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_FALSE;
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_REFERENCED_COLUMNS_CANNOT_BE_UPDATED)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_MERGE_REFERENCED_COLUMNS_CANNOT_BE_UPDATED,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * PROJ-1988 Implement MERGE statement
 * search aColumn in aExpression, and set the result to aFound
 */
IDE_RC qmv::searchColumnInExpression( qcStatement  * aStatement,
                                      qtcNode      * aExpression,
                                      qcmColumn    * aColumn,
                                      idBool       * aFound )
{
    mtcColumn * sMtcColumn;

    IDU_FIT_POINT_FATAL( "qmv::searchColumnInExpression::__FT__" );

    if ( QTC_IS_SUBQUERY( aExpression ) == ID_FALSE )
    {
        if ( aExpression->node.module == &qtc::columnModule )
        {
            // BUG-37132
            // smiColumn.id 가 초기화되지 않은 pseudo column 에 대해서 비교를 회피한다.

            if ( ( QTC_STMT_TUPLE(aStatement, aExpression)->lflag & MTC_TUPLE_TYPE_MASK )
                 == MTC_TUPLE_TYPE_TABLE )
            {
                sMtcColumn = QTC_STMT_COLUMN(aStatement, aExpression);

                if ( aColumn->basicInfo->column.id == sMtcColumn->column.id )
                {
                    *aFound = ID_TRUE;
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }

        if ( *aFound == ID_FALSE )
        {
            if ( aExpression->node.next != NULL )
            {
                IDE_TEST( searchColumnInExpression(
                              aStatement,
                              (qtcNode *)aExpression->node.next,
                              aColumn,
                              aFound )
                          != IDE_SUCCESS);
            }
            else
            {
                /* nothing to do */
            }

            if ( aExpression->node.arguments != NULL )
            {
                IDE_TEST( searchColumnInExpression(
                              aStatement,
                              (qtcNode *)aExpression->node.arguments,
                              aColumn,
                              aFound )
                          != IDE_SUCCESS);
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2219 Row-level before update trigger
// Row-level before update trigger에서 참조하는 column을 update column에 추가한다.
IDE_RC qmv::makeNewUpdateColumnList( qcStatement * aQcStmt )
{
    qmmUptParseTree * sParseTree;
    qcmTableInfo    * sTableInfo;
    qcmColumn       * sQcmColumn;

    UChar           * sRefColumnList  = NULL;
    UInt              sRefColumnCount = 0;

    qmmValueNode    * sCurrValueNodeList;
    qcmColumn       * sCurrQcmColumn;

    qmmValueNode    * sNewValueNode = NULL;
    qcmColumn       * sNewQcmColumn = NULL;
    qcNamePosition  * sNamePosition = NULL;
    qtcNode         * sNode[2] = {NULL,NULL};

    UInt    i;

    IDU_FIT_POINT_FATAL( "qmv::makeNewUpdateColumnList::__FT__" );

    sParseTree = (qmmUptParseTree*)aQcStmt->myPlan->parseTree;
    sTableInfo = sParseTree->querySet->SFWGH->from->tableRef->tableInfo;

    // PSM load 중 PSM내의 update DML을 validation을 할 때,
    // trigger의 ref column을 고려하지 않는다.
    // Trigger를 아직 load 하지 않았기 때문이다.
    IDE_TEST_CONT( aQcStmt->spvEnv->createProc != NULL, skip_make_list );

    IDE_TEST_CONT( aQcStmt->spvEnv->createPkg != NULL, skip_make_list );

    IDE_TEST( qmv::getRefColumnList( aQcStmt,
                                     &sRefColumnList,
                                     &sRefColumnCount )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sRefColumnCount == 0, skip_make_list );

    // Update target column에 속한 것은 ref column list에서 제외한다.
    for ( sQcmColumn  = sParseTree->updateColumns;
          sQcmColumn != NULL;
          sQcmColumn  = sQcmColumn->next )
    {
        i = sQcmColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

        if ( sRefColumnList[i] == QDN_REF_COLUMN_TRUE )
        {
            sRefColumnList[i] = QDN_REF_COLUMN_FALSE;
            sRefColumnCount--;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_CONT( sRefColumnCount == 0, skip_make_list );

    IDU_FIT_POINT( "qmv::makeNewUpdateColumnList::alloc::sNewValueNode",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                  ID_SIZEOF(qmmValueNode) * sRefColumnCount,
                  (void**)&sNewValueNode )
              != IDE_SUCCESS);

    IDU_FIT_POINT( "qmv::makeNewUpdateColumnList::alloc::sNamePosition",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                  ID_SIZEOF(qcNamePosition) * sRefColumnCount,
                  (void**)&sNamePosition )
              != IDE_SUCCESS);

    IDU_FIT_POINT( "qmv::makeNewUpdateColumnList::alloc::sNewQcmColumn",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                  ID_SIZEOF(qcmColumn) * sRefColumnCount,
                  (void**)&sNewQcmColumn )
              != IDE_SUCCESS);

    sQcmColumn = sTableInfo->columns;

    sCurrValueNodeList = sParseTree->values;
    sCurrQcmColumn     = sParseTree->updateColumns;

    // Update할 value list와 column list의 마지막으로 이동하여,
    // ref column list에 있는 것을 마지막에 추가할 수 있게 한다.
    IDE_FT_ERROR( sCurrValueNodeList != NULL );
    IDE_FT_ERROR( sCurrQcmColumn     != NULL );

    while ( sCurrValueNodeList->next != NULL )
    {
        IDE_FT_ERROR( sCurrQcmColumn->next != NULL );

        sCurrValueNodeList = sCurrValueNodeList->next;
        sCurrQcmColumn     = sCurrQcmColumn->next;
    }

    for ( i = 0; i < sTableInfo->columnCount; i++ )
    {
        if ( sRefColumnList[i] == QDN_REF_COLUMN_TRUE )
        {
            QCM_COLUMN_INIT( sNewQcmColumn );

            sNewValueNode->validate  = ID_TRUE;
            sNewValueNode->calculate = ID_TRUE;
            sNewValueNode->timestamp = ID_FALSE;
            sNewValueNode->expand    = ID_FALSE;
            sNewValueNode->msgID     = ID_FALSE;
            sNewValueNode->next      = NULL;

            sNamePosition->size   = idlOS::strlen( sQcmColumn->name );
            sNamePosition->offset = 0;

            IDU_FIT_POINT( "qmv::makeNewUpdateColumnList::alloc::stmtText",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                          (sNamePosition->size + 1),
                          (void**)&sNamePosition->stmtText )
                      != IDE_SUCCESS);

            idlOS::memcpy( sNamePosition->stmtText,
                           sQcmColumn->name,
                           sNamePosition->size );
            sNamePosition->stmtText[sNamePosition->size] = '\0';

            sNewQcmColumn->basicInfo = sQcmColumn->basicInfo;
            sNewQcmColumn->namePos   = *sNamePosition;

            IDE_TEST( qtc::makeColumn( aQcStmt,
                                       sNode,
                                       NULL,          // user
                                       NULL,          // table
                                       sNamePosition, // column
                                       NULL )         // package
                      != IDE_SUCCESS );

            sNewValueNode->value = sNode[0];

            sCurrValueNodeList->next = sNewValueNode;
            sCurrQcmColumn->next     = sNewQcmColumn;

            sCurrValueNodeList = sNewValueNode;
            sCurrQcmColumn     = sNewQcmColumn;

            sNamePosition++;
            sNewValueNode++;
            sNewQcmColumn++;
        }
        else
        {
            // Nothing to do.
        }

        sQcmColumn = sQcmColumn->next;
    }

    sParseTree->uptColCount += sRefColumnCount;

    IDE_EXCEPTION_CONT( skip_make_list );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2219 Row-level before update trigger
// Update 대상 table에 대한 모든 trigger가 참조하는 column list를 얻어온다.
IDE_RC qmv::getRefColumnList( qcStatement *  aQcStmt,
                              UChar       ** aRefColumnList,
                              UInt        *  aRefColumnCount )
{
    qmmUptParseTree           * sParseTree;
    qdnTriggerCache           * sTriggerCache;
    qcmTriggerInfo            * sTriggerInfo;
    qcmTableInfo              * sTableInfo;
    qdnCreateTriggerParseTree * sTriggerParseTree;
    qdnTriggerEventTypeList   * sEventTypeList;

    UChar                     * sRefColumnList  = NULL;
    UInt                        sRefColumnCount = 0;

    UChar                     * sTriggerRefColumnList;
    UInt                        sTriggerRefColumnCount;

    qcmColumn                 * sQcmColumn;
    smiColumn                 * sSmiColumn;
    qcmColumn                 * sTriggerQcmColumn;

    UInt                        i;
    UInt                        j;
    idBool                      sNeedCheck = ID_FALSE;
    volatile UInt               sStage;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmv::getRefColumnList::__FT__" );

    sParseTree   = (qmmUptParseTree*)aQcStmt->myPlan->parseTree;
    sTableInfo   = sParseTree->querySet->SFWGH->from->tableRef->tableInfo;
    sTriggerInfo = sTableInfo->triggerInfo;
    sStage       = 0; /* BUG-45994 - 컴파일러 최적화 회피 */

    for ( i = 0; i < sTableInfo->triggerCount; i++, sTriggerInfo++ )
    {
        sNeedCheck = ID_FALSE;

        if ( ( sTriggerInfo->enable == QCM_TRIGGER_ENABLE ) &&
             ( sTriggerInfo->granularity == QCM_TRIGGER_ACTION_EACH_ROW ) &&
             ( sTriggerInfo->eventTime == QCM_TRIGGER_BEFORE ) &&
             ( ( sTriggerInfo->eventType & QCM_TRIGGER_EVENT_UPDATE ) != 0 ) )
        {
            IDE_TEST( smiObject::getObjectTempInfo( sTriggerInfo->triggerHandle,
                                                    (void**)&sTriggerCache )
                      != IDE_SUCCESS );
            IDE_TEST( sTriggerCache->latch.lockRead( NULL, NULL ) != IDE_SUCCESS );

            sStage = 1;

            /* BUG-45994 */
            IDU_FIT_POINT_FATAL( "qmv::getRefColumnList::__FT__::STAGE1" );

            // invalid 상태의 trigger는 무시한다.
            if ( sTriggerCache->isValid == ID_TRUE )
            {
                sTriggerParseTree = (qdnCreateTriggerParseTree*)sTriggerCache->triggerStatement.myPlan->parseTree;
                sEventTypeList    = sTriggerParseTree->triggerEvent.eventTypeList;

                if ( sTriggerInfo->uptCount != 0 )
                {
                    for ( sQcmColumn = sParseTree->updateColumns;
                          ( sQcmColumn != NULL ) && ( sNeedCheck == ID_FALSE );
                          sQcmColumn = sQcmColumn->next )
                    {
                        sSmiColumn = &sQcmColumn->basicInfo->column;
                        for ( sTriggerQcmColumn = sEventTypeList->updateColumns;
                              sTriggerQcmColumn != NULL;
                              sTriggerQcmColumn = sTriggerQcmColumn->next )
                        {
                            if ( sTriggerQcmColumn->basicInfo->column.id ==
                                 sSmiColumn->id )
                            {
                                sNeedCheck = ID_TRUE;
                                break;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                    }
                }
                else
                {
                    sNeedCheck = ID_TRUE;
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( sNeedCheck == ID_TRUE )
            {
                if ( sRefColumnList == NULL )
                {
                    IDU_FIT_POINT( "qmv::getRefColumnList::cralloc::sRefColumnList",
                                   idERR_ABORT_InsufficientMemory );

                    IDE_TEST( QC_QME_MEM( aQcStmt )->cralloc(
                                  ID_SIZEOF( UChar ) * sTableInfo->columnCount,
                                  (void**)&sRefColumnList )
                              != IDE_SUCCESS);
                }
                else
                {
                    // Nothing to do.
                }

                sTriggerRefColumnList  = sTriggerParseTree->refColumnList;
                sTriggerRefColumnCount = sTriggerParseTree->refColumnCount;

                if ( ( sTriggerRefColumnList != NULL ) &&
                     ( sTriggerRefColumnCount > 0 ) )
                {
                    for ( j = 0; j < sTableInfo->columnCount; j++ )
                    {
                        if ( ( sTriggerRefColumnList[j] == QDN_REF_COLUMN_TRUE ) &&
                             ( sRefColumnList[j]        == QDN_REF_COLUMN_FALSE ) )
                        {
                            sRefColumnList[j] = QDN_REF_COLUMN_TRUE;
                            sRefColumnCount++;
                        }
                        else
                        {
                            // Nothing to do.
                        }
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

            sStage = 0;
            IDE_TEST( sTriggerCache->latch.unlock() != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    *aRefColumnList  = sRefColumnList;
    *aRefColumnCount = sRefColumnCount;

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    switch ( sStage )
    {
        case 1:
            (void) sTriggerCache->latch.unlock();
        default:
            break;
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

IDE_RC qmv::notAllowedAnalyticFunc( qcStatement * aStatement,
                                    qtcNode     * aNode )
{
    qcuSqlSourceInfo    sqlInfo;

    if ( aNode->overClause != NULL )
    {
        sqlInfo.setSourceInfo( aStatement, &aNode->position );
        IDE_RAISE( ERR_NOT_ALLOWED_ANALYTIC );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.arguments != NULL )
    {
        IDE_TEST( notAllowedAnalyticFunc( aStatement, (qtcNode *)aNode->node.arguments )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.next != NULL )
    {
        IDE_TEST( notAllowedAnalyticFunc( aStatement, (qtcNode *)aNode->node.next )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_ALLOWED_ANALYTIC)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_ANALYTIC,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::parseSPVariableValue( qcStatement   * aStatement,
                                  qtcNode       * aSPVariable,
                                  qmmValueNode ** aValues)
{
    // BUG-46174
    qcuSqlSourceInfo     sqlInfo;
    mtcColumn          * sMtcColumn  = NULL;

    idBool               sFindVar;
    qsVariables        * sVariable   = NULL;
    qmmValueNode       * sLastValue  = NULL;
    qmmValueNode       * sValue      = NULL;
    qtcNode            * sRecField[2];
    qcmColumn          * sFieldColumn = NULL;
    qsProcStmtSql      * sCurrStmt    = NULL;
    qcNamePosition       sFieldName;

    IDE_TEST_RAISE( ( ( aStatement->spvEnv->createProc == NULL ) &&
                      ( aStatement->spvEnv->createPkg  == NULL ) ),
                    ERR_NOT_SUPPORTED );

    aSPVariable->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

    IDE_TEST( qsvProcVar::searchVarAndPara( aStatement,
                                            aSPVariable,
                                            ID_TRUE, // for OUTPUT
                                            &sFindVar,
                                            &sVariable)
              != IDE_SUCCESS);

    if ( sFindVar == ID_FALSE )
    {
        // BUG-42715
        // bind 변수가 아니면 package 변수이다.
        IDE_TEST( qsvProcVar::searchVariableFromPkg( aStatement,
                                                     aSPVariable,
                                                     &sFindVar,
                                                     &sVariable )
                  != IDE_SUCCESS );
    }

    IDE_TEST_RAISE( sFindVar == ID_FALSE, ERR_INVALID_VAR );

    IDE_TEST( qtc::estimate( aSPVariable,
                             QC_SHARED_TMPLATE(aStatement),
                             aStatement,
                             NULL,
                             NULL,
                             NULL )
              != IDE_SUCCESS );

    if ( ( sVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE ) &&
         ( aSPVariable->node.arguments != NULL ) )
    {
        // ARRAY[1]이 레코드타입인 경우
        sMtcColumn = sVariable->typeInfo->columns->next->basicInfo;

        IDE_TEST_RAISE( ( ( sMtcColumn->module->id != MTD_ROWTYPE_ID ) &&
                          ( sMtcColumn->module->id != MTD_RECORDTYPE_ID ) ),
                        ERR_INVALID_VAR );

        sFieldColumn = ((qtcModule*)(sMtcColumn->module))->typeInfo->columns;
    }
    else 
    {
        sMtcColumn = QTC_STMT_COLUMN( aStatement, aSPVariable ) ;

        IDE_TEST_RAISE( ( ( sMtcColumn->module->id != MTD_ROWTYPE_ID ) &&
                          ( sMtcColumn->module->id != MTD_RECORDTYPE_ID ) ),
                        ERR_INVALID_VAR );

        sFieldColumn = sVariable->typeInfo->columns;
    }

    for ( ; 
          sFieldColumn != NULL;
          sFieldColumn = sFieldColumn->next )
    {
        sFieldName.stmtText = sFieldColumn->name;
        sFieldName.offset = 0;
        sFieldName.size = idlOS::strlen((SChar*)sFieldColumn->name);

        IDE_TEST( qsvProcVar::makeRecordColumnByName( aStatement,
                                                      aSPVariable,
                                                      &sFieldName,
                                                      sRecField )
                  != IDE_SUCCESS); 

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmValueNode, &sValue)
                  != IDE_SUCCESS );

        sValue->value     = sRecField[0];
        sValue->validate  = ID_TRUE;
        sValue->calculate = ID_TRUE;
        sValue->timestamp = ID_FALSE;
        sValue->expand    = ID_FALSE;
        sValue->msgID     = ID_FALSE;
        sValue->next      = NULL;

        if ( *aValues == NULL )
        {
            *aValues = sValue;

            sCurrStmt = (qsProcStmtSql *)aStatement->spvEnv->currStmt;
            sCurrStmt->usingRecValueInsUpt = sValue->value;

            sLastValue = *aValues;
        }
        else
        {
            sLastValue->value->node.next = (mtcNode *)sValue->value;
            sLastValue->next = sValue;
            sLastValue = sLastValue->next;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_SUPPORTED);
    {
        sqlInfo.setSourceInfo( aStatement,
                               &aSPVariable->position );

        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_VAR);
    {
        sqlInfo.setSourceInfo( aStatement,
                               &aSPVariable->position );

        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSV_INVALID_IDENTIFIER,
                                 sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * BUG-46702
 * with rollup 구문이 들어오면 group by의 전체 expression을 rollup의 arguments
 * 로 transform해서 기존 rollup처럼 동작하도록 한다.
 */
IDE_RC qmv::convertWithRoullupToRollup( qcStatement * aStatement,
                                            qmsSFWGH    * aSFWGH )
{
    qmsConcatElement * sConcatElement = NULL;
    qmsConcatElement * sTemp = NULL;
    idBool             sIsWithRollup = ID_FALSE;
    qcuSqlSourceInfo   sqlInfo;

    for ( sConcatElement = aSFWGH->group;
          sConcatElement != NULL;
          sConcatElement = sConcatElement->next )
    {
        if ( sConcatElement->type == QMS_GROUPBY_WITH_ROLLUP )
        {
            sIsWithRollup = ID_TRUE;
            sConcatElement->type = QMS_GROUPBY_NORMAL;

            if ( sConcatElement->next != NULL )
            {
                sqlInfo.setSourceInfo( aStatement, &sConcatElement->next->position );
                IDE_RAISE( ERR_NOT_SUPPORT );
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

    IDE_TEST_CONT( sIsWithRollup == ID_FALSE, normal_exit );

    for ( sConcatElement = aSFWGH->group;
          sConcatElement != NULL;
          sConcatElement = sConcatElement->next )
    {
        if ( sConcatElement->type != QMS_GROUPBY_NORMAL )
        {
            sqlInfo.setSourceInfo( aStatement, &sConcatElement->position );
            IDE_RAISE( ERR_NOT_SUPPORT );
        }
        else
        {
            /* Nothing to do */
        }
    }

    sConcatElement = aSFWGH->group;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsConcatElement),
                                             (void **) &sTemp )
              != IDE_SUCCESS );

    sTemp->type = QMS_GROUPBY_ROLLUP;
    sTemp->arithmeticOrList = NULL;
    sTemp->next = NULL;
    sTemp->arguments = sConcatElement;
    SET_EMPTY_POSITION( sTemp->position );

    aSFWGH->group = sTemp;

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_SYNTAX,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::checkUpdatableView( qcStatement * aStatement,
                                qmsFrom     * aFrom )
{
    qmsParseTree     * sViewParseTree = NULL;
    qcuSqlSourceInfo   sSqlInfo;

    if ( aFrom->joinType != QMS_NO_JOIN )
    {
        if ( aFrom->joinType == QMS_FULL_OUTER_JOIN )
        {
            sSqlInfo.setSourceInfo( aStatement, &aFrom->fromPosition );
            IDE_RAISE( ERR_SYNTAX );
        }
        else
        {
            IDE_TEST( checkUpdatableView( aStatement, aFrom->left ) != IDE_SUCCESS );
            IDE_TEST( checkUpdatableView( aStatement, aFrom->right ) != IDE_SUCCESS );
        }
    }
    else
    {
        /* PROJ-2204 join update, delete
         * updatable view에 사용되는 SFWGH임을 표시한다. */
        if ( aFrom->tableRef->view != NULL )
        {
            sViewParseTree = (qmsParseTree*)aFrom->tableRef->view->myPlan->parseTree;

            if ( sViewParseTree->querySet->SFWGH != NULL )
            {
                sViewParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
                sViewParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SYNTAX )
    {
        ( void )sSqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_SYNTAX, sSqlInfo.getErrMessage() ));
        ( void )sSqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::searchColumnInFromTree( qcStatement *  aStatement,
                                    qcmColumn   *  aColumn,
                                    qmsFrom     *  aFrom,
                                    qmsTableRef ** aTableRef )
{
    qmsTableRef     * sTableRef   = NULL;
    qcmTableInfo    * sTableInfo  = NULL;
    qcmColumn       * sColumnInfo = NULL;
    UInt              sUserID;
    UShort            sColOrder;
    idBool            sIsFound = ID_FALSE;
    idBool            sIsLobType = ID_FALSE;
    qcuSqlSourceInfo  sqlInfo;

    if ( aFrom->joinType != QMS_NO_JOIN )
    {
        IDE_TEST( searchColumnInFromTree( aStatement,
                                          aColumn,
                                          aFrom->left,
                                          aTableRef )
                  != IDE_SUCCESS );

        IDE_TEST( searchColumnInFromTree( aStatement,
                                          aColumn,
                                          aFrom->right,
                                          aTableRef )
                  != IDE_SUCCESS );
    }
    else
    {
        sTableRef = aFrom->tableRef;
        sTableInfo = sTableRef->tableInfo;

        if ( QC_IS_NULL_NAME( aColumn->userNamePos ) == ID_TRUE )
        {
            sUserID  = sTableRef->userID;
            sIsFound = ID_TRUE;
        }
        else
        {
            // BUG-42494 A variable of package could not be used
            // when its type is associative array.
            if ( qcmUser::getUserID( aStatement,
                                     aColumn->userNamePos,
                                     &sUserID )
                 == IDE_SUCCESS )
            {
                if ( sTableRef->userID == sUserID )
                {
                    sIsFound = ID_TRUE;
                }
                else
                {
                    sIsFound = ID_FALSE;
                }
            }
            else
            {
                IDE_CLEAR();
                sIsFound = ID_FALSE;
            }
        }

        // check table name
        if ( sIsFound == ID_TRUE )
        {
            if ( QC_IS_NULL_NAME( aColumn->tableNamePos ) != ID_TRUE )
            {
                // BUG-38839
                if ( QC_IS_NAME_MATCHED( aColumn->tableNamePos, sTableRef->aliasName ) &&
                     ( sTableInfo != NULL ) )
                {
                    sIsFound = ID_TRUE;
                }
                else
                {
                    sIsFound = ID_FALSE;
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

        // check column name
        if ( sIsFound == ID_TRUE )
        {
            IDE_TEST( qmvQTC::searchColumnInTableInfo( sTableInfo,
                                                       aColumn->namePos,
                                                       &sColOrder,
                                                       &sIsFound,
                                                       &sIsLobType )
                      != IDE_SUCCESS);
            if ( sIsFound == ID_TRUE )
            {
                if ( *aTableRef != NULL )
                {
                    sqlInfo.setSourceInfo( aStatement, &aColumn->namePos );
                    IDE_RAISE(ERR_COLUMN_AMBIGUOUS_DEF);
                }
                else
                {
                    /* Nothing to do */
                }

                // set table and column ID
                *aTableRef = sTableRef;

                IDE_TEST( qcmCache::getColumn( aStatement,
                                               sTableInfo,
                                               aColumn->namePos,
                                               &sColumnInfo )
                          != IDE_SUCCESS );
                QMV_SET_QCM_COLUMN( aColumn, sColumnInfo );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_COLUMN_AMBIGUOUS_DEF)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_COLUMN_AMBIGUOUS_DEF,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-2714 Multiple Update Delete support
 */
IDE_RC qmv::parseMultiUpdate( qcStatement * aStatement )
{
    qmmUptParseTree * sParseTree;
    qmsTableRef     * sTableRef;
    qcmTableInfo    * sTableInfo;
    qcmColumn       * sColumn;
    qcmColumn       * sNewColumn;
    qcmColumn       * sOtherColumn;
    qcmColumn       * sColumnInfo;
    qcmColumn       * sDefaultExprColumn;
    qmmValueNode    * sCurrValue;
    qmmValueNode    * sNewValue;
    qmmSubqueries   * sCurrSubq;
    qcmIndex        * sPrimary;
    UInt              sIterator;
    UInt              sValueCount;
    UInt              sFlag = 0;
    idBool            sTimestampExistInSetClause = ID_FALSE;
    qmsFrom         * sFrom;
    qmmMultiTables  * sTmp;
    qcuSqlSourceInfo  sqlInfo;
    UInt              i;

    sParseTree = (qmmUptParseTree*) aStatement->myPlan->parseTree;
    qtc::dependencyClear( & sParseTree->querySet->SFWGH->depInfo );

    /* PROJ-1888 INSTEAD OF TRIGGER */
    // FROM clause
    for ( sFrom = sParseTree->querySet->SFWGH->from;
          sFrom != NULL;
          sFrom = sFrom->next )
    {
        IDE_TEST( parseViewInFromClause( aStatement,
                                         sFrom,
                                         sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );

        IDE_TEST( checkUpdatableView( aStatement, sFrom )
                  != IDE_SUCCESS );
    }

    // check existence of table and get table META Info.
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_VIEW_CREATION_FALSE);

    if ( sParseTree->querySet->SFWGH->hints == NULL )
    {
        IDU_FIT_POINT("qmv::parseMultiUpdate::STRUCT_ALLOC::hints",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST ( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                 qmsHints,
                                 &(sParseTree->querySet->SFWGH->hints) ) != IDE_SUCCESS);

        QCP_SET_INIT_HINTS(sParseTree->querySet->SFWGH->hints);
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qmvQuerySet::convertAnsiInnerJoin( aStatement, sParseTree->querySet->SFWGH )
              != IDE_SUCCESS );

    for ( sFrom = sParseTree->querySet->SFWGH->from;
          sFrom != NULL;
          sFrom = sFrom->next )
    {
        if ( sFrom->joinType != QMS_NO_JOIN )
        {
            IDE_TEST( qmvQuerySet::validateQmsFromWithOnCond( sParseTree->querySet,
                                                              sParseTree->querySet->SFWGH,
                                                              sFrom,
                                                              aStatement,
                                                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmvQuerySet::validateQmsTableRef( aStatement,
                                                        sParseTree->querySet->SFWGH,
                                                        sFrom->tableRef,
                                                        sParseTree->querySet->SFWGH->lflag,
                                                        MTC_COLUMN_NOTNULL_TRUE) // PR-13597
                      != IDE_SUCCESS);
            // Table Map 설정
            QC_SHARED_TMPLATE(aStatement)->tableMap[sFrom->tableRef->table].from = sFrom;

            // FROM 절에 대한 dependencies 설정
            qtc::dependencyClear( &sFrom->depInfo );
            qtc::dependencySet( sFrom->tableRef->table, &sFrom->depInfo );

            // PROJ-1718 Semi/anti join과 관련된 dependency 초기화
            qtc::dependencyClear( &sFrom->semiAntiJoinDepInfo );

            IDE_TEST( qmsPreservedTable::addTable( aStatement,
                                                   sParseTree->querySet->SFWGH,
                                                   sFrom->tableRef )
                      != IDE_SUCCESS );
        }
    }

    // Query Set에 dependency 설정
    qtc::dependencyClear( & sParseTree->querySet->depInfo );
    IDE_TEST( qtc::dependencyOr( & sParseTree->querySet->depInfo,
                                 & sParseTree->querySet->SFWGH->depInfo,
                                 & sParseTree->querySet->depInfo )
              != IDE_SUCCESS );

    // parse VIEW in WHERE clause
    if ( sParseTree->querySet->SFWGH->where != NULL )
    {
        IDE_TEST( parseViewInExpression( aStatement,
                                         sParseTree->querySet->SFWGH->where )
                  != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    // parse VIEW in SET clause
    for ( sCurrValue = sParseTree->values; sCurrValue != NULL; sCurrValue = sCurrValue->next )
    {
        if ( sCurrValue->value != NULL )
        {
            IDE_TEST(parseViewInExpression(aStatement, sCurrValue->value)
                     != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }
    }

    // parse VIEW in SET clause
    for ( sCurrSubq = sParseTree->subqueries; sCurrSubq != NULL; sCurrSubq = sCurrSubq->next)
    {
        if ( sCurrSubq->subquery != NULL )
        {
            IDE_TEST(parseViewInExpression(aStatement, sCurrSubq->subquery)
                     != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST_RAISE( sParseTree->columns == NULL, ERR_NOT_SUPPORTED );
    IDE_TEST_RAISE( sParseTree->returnInto != NULL, ERR_NOT_SUPPORTED );
    IDE_TEST_RAISE( sParseTree->limit != NULL, ERR_NOT_SUPPORTED );

    sParseTree->uptColCount = 0;
    sParseTree->updateTableRef = NULL;
    sParseTree->updateColumns = NULL;
    sParseTree->defaultExprColumns = NULL;
    sParseTree->defaultTableRef = NULL;
    sParseTree->checkConstrList = NULL;
    sParseTree->uptColumnList = NULL;
    sParseTree->insteadOfTrigger = ID_FALSE;

    for ( sCurrValue  = sParseTree->values, sValueCount = 0;
          sCurrValue != NULL;
          sCurrValue  = sCurrValue->next, sValueCount++ )
    {
        // set order of values
        sCurrValue->order = sValueCount;
    }

    for ( sColumn = sParseTree->columns, sCurrValue = sParseTree->values;
          sColumn != NULL;
          sColumn = sColumn->next, sCurrValue = sCurrValue->next )
    {
        sParseTree->uptColCount++;

        sTableRef = NULL;
        for ( sFrom = sParseTree->querySet->SFWGH->from;
              sFrom != NULL;
              sFrom = sFrom->next )
        {
            IDE_TEST( searchColumnInFromTree( aStatement,
                                              sColumn,
                                              sFrom,
                                              &sTableRef)
                      != IDE_SUCCESS);
        }

        if ( sTableRef == NULL )
        {
            sqlInfo.setSourceInfo( aStatement, &sColumn->namePos );
            IDE_RAISE( ERR_NOT_EXIST_COLUMN );
        }
        else
        {
            IDE_TEST( makeAndSetMultiTable( aStatement,
                                            sParseTree,
                                            sTableRef,
                                            sColumn,
                                            sCurrValue,
                                            sValueCount,
                                            &sParseTree->mTableList )
                      != IDE_SUCCESS );
        }
    }

    for ( sTmp = sParseTree->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        sTimestampExistInSetClause = ID_FALSE;
        sTableInfo = sTmp->mTableRef->tableInfo;
        if ( sTableInfo->triggerCount > 0 )
        {
            IDE_TEST( makeNewUpdateColumnListMultiTable( aStatement, sTmp )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        // BUG-17409
        sTmp->mTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
        sTmp->mTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

        for ( sColumn = sTmp->mColumns; sColumn != NULL; sColumn = sColumn->next )
        {
            /* PROJ-1090 Function-based Index */
            if ( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement, &(sColumn->namePos) );
                IDE_RAISE( ERR_CANNOT_USE_HIDDEN_COLUMN );
            }
            else
            {
                /* Nothing to do */
            }

            // If a table has timestamp column
            if ( sTableInfo->timestamp != NULL )
            {
                if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                     == MTC_COLUMN_TIMESTAMP_TRUE )
                {
                    sTimestampExistInSetClause = ID_TRUE;
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

        // If a table has timestamp column
        //  and there is no specified timestamp column in SET clause,
        //  then make the updating timestamp column internally.
        if ( ( sTableInfo->timestamp != NULL ) &&
             ( sTimestampExistInSetClause == ID_FALSE ) )
        {
            // make column
            IDU_FIT_POINT("qmv::parseMultiUpdate::STRUCT_ALLOC::sNewColumn",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sNewColumn)
                     != IDE_SUCCESS);

            IDE_TEST(qcmCache::getColumnByID( sTableInfo,
                                              sTableInfo->timestamp->constraintColumn[0],
                                              &sColumnInfo )
                     != IDE_SUCCESS);
            QMV_SET_QCM_COLUMN(sNewColumn, sColumnInfo);

            sNewColumn->next = sTmp->mColumns;
            sTmp->mColumns   = sNewColumn;

            // make value
            IDU_FIT_POINT("qmv::parseMultiUpdate::STRUCT_ALLOC::sNewValue",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmValueNode, &sNewValue)
                     != IDE_SUCCESS);
            sNewValue->value     = NULL;
            sNewValue->validate  = ID_TRUE;
            sNewValue->calculate = ID_TRUE;
            sNewValue->timestamp = ID_FALSE;
            sNewValue->msgID     = ID_FALSE;
            sNewValue->expand    = ID_FALSE;

            // connect value list
            sNewValue->next = sTmp->mValues;
            sTmp->mValues   = sNewValue;
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-1090 Function-based Index */
        for ( sColumn = sTmp->mColumns; sColumn != NULL; sColumn = sColumn->next )
        {
            IDE_TEST( qmsDefaultExpr::addDefaultExpressionColumnsRelatedToColumn(
                          aStatement,
                          &(sTmp->mDefaultColumns),
                          sTableInfo,
                          sColumn->basicInfo->column.id )
                      != IDE_SUCCESS );
        }

        /* PROJ-1090 Function-based Index */
        for ( sDefaultExprColumn = sTmp->mDefaultColumns;
              sDefaultExprColumn != NULL;
              sDefaultExprColumn = sDefaultExprColumn->next )
        {
            IDU_FIT_POINT("qmv::parseMultiUpdate::STRUCT_ALLOC::sNewColumn2",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sNewColumn)
                     != IDE_SUCCESS);
            QMV_SET_QCM_COLUMN( sNewColumn, sDefaultExprColumn );
            sNewColumn->next = sTmp->mColumns;
            sTmp->mColumns   = sNewColumn;
            sTmp->mColumnCount++;
            // make value
            IDU_FIT_POINT("qmv::parseMultiUpdate::STRUCT_ALLOC::sNewValue2",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmmValueNode, &sNewValue)
                     != IDE_SUCCESS);
            sNewValue->value     = NULL;
            sNewValue->validate  = ID_TRUE;
            sNewValue->calculate = ID_TRUE;
            sNewValue->timestamp = ID_FALSE;
            sNewValue->msgID     = ID_FALSE;
            sNewValue->expand    = ID_FALSE;

            // connect value list
            sNewValue->next = sTmp->mValues;
            sTmp->mValues   = sNewValue;
        }

        for ( sColumn = sTmp->mColumns, sCurrValue = sTmp->mValues, sIterator = 0;
              ( sColumn != NULL ) && ( sCurrValue != NULL );
             sColumn = sColumn->next, sCurrValue  = sCurrValue->next, sIterator++ )
        {
            // set order of values
            sCurrValue->order = sIterator;

            // if updating column is primary key and updating tables is replicated,
            //      then error.
            if ( sTableInfo->replicationCount > 0)
            {
                sPrimary  = sTableInfo->primaryKey;

                for ( i = 0; i < sPrimary->keyColCount; i++ )
                {
                    // To fix BUG-14325
                    // replication이 걸려있는 table에 대해 pk update여부 검사.
                    if ( QCU_REPLICATION_UPDATE_PK == 0 )
                    {
                        IDE_TEST_RAISE( sPrimary->keyColumns[i].column.id == sColumn->basicInfo->column.id,
                                        ERR_NOT_ALLOW_PK_UPDATE);
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                /* Nothing to do */
            }

            //--------- validation of updating value  ---------//
            if ( ( sCurrValue->value == NULL ) && ( sCurrValue->calculate == ID_TRUE ) )
            {
                IDE_TEST( setDefaultOrNULL( aStatement, sTableInfo, sColumn, sCurrValue )
                          != IDE_SUCCESS);

                if ( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
                     == MTC_COLUMN_TIMESTAMP_TRUE )
                {
                    sCurrValue->timestamp = ID_TRUE;
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

        /* PROJ-1107 Check Constraint 지원 */
        for ( sColumn = sTmp->mColumns; sColumn != NULL; sColumn = sColumn->next )
        {
            IDE_TEST( qdnCheck::addCheckConstrSpecRelatedToColumn(
                          aStatement,
                          &(sTmp->mCheckConstrList),
                          sTableInfo->checks,
                          sTableInfo->checkCount,
                          sColumn->basicInfo->column.id )
                      != IDE_SUCCESS );
        }
        /* PROJ-1107 Check Constraint 지원 */
        IDE_TEST( qdnCheck::setMtcColumnToCheckConstrList(
                      aStatement,
                      sTableInfo,
                      sTmp->mCheckConstrList )
                  != IDE_SUCCESS );

        if ( sTmp->mDefaultColumns != NULL )
        {
            sFlag = 0;
            sFlag &= ~QMV_PERFORMANCE_VIEW_CREATION_MASK;
            sFlag |=  QMV_PERFORMANCE_VIEW_CREATION_FALSE;
            sFlag &= ~QMV_VIEW_CREATION_MASK;
            sFlag |=  QMV_VIEW_CREATION_FALSE;

            IDU_FIT_POINT("qmv::parseMultiUpdate::STRUCT_ALLOC::mDefaultTableRef",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qmsTableRef, &sTmp->mDefaultTableRef )
                      != IDE_SUCCESS );

            QCP_SET_INIT_QMS_TABLE_REF( sTmp->mDefaultTableRef );

            sTmp->mDefaultTableRef->userName.stmtText = sTableInfo->tableOwnerName;
            sTmp->mDefaultTableRef->userName.offset   = 0;
            sTmp->mDefaultTableRef->userName.size     = idlOS::strlen(sTableInfo->tableOwnerName);

            sTmp->mDefaultTableRef->tableName.stmtText = sTableInfo->name;
            sTmp->mDefaultTableRef->tableName.offset   = 0;
            sTmp->mDefaultTableRef->tableName.size     = idlOS::strlen(sTableInfo->name);

            /* BUG-17409 */
            sTmp->mDefaultTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
            sTmp->mDefaultTableRef->flag |=  QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;

            IDE_TEST( qmvQuerySet::validateQmsTableRef( aStatement,
                                                        NULL,
                                                        sTmp->mDefaultTableRef,
                                                        sFlag,
                                                        MTC_COLUMN_NOTNULL_TRUE ) /* PR-13597 */
                      != IDE_SUCCESS );
            /*
             * PROJ-1090, PROJ-2429
             * Variable column, Compressed column을
             * Fixed Column으로 변환한 TableRef를 만든다.
             */
            IDE_TEST( qtc::nextTable( &(sTmp->mDefaultTableRef->table),
                                      aStatement,
                                      NULL,     /* Tuple ID만 얻는다. */
                                      QCM_TABLE_TYPE_IS_DISK( sTmp->mDefaultTableRef->tableInfo->tableFlag ),
                                      MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                      != IDE_SUCCESS );

            IDE_TEST( qmvQuerySet::makeTupleForInlineView(
                          aStatement,
                          sTmp->mDefaultTableRef,
                          sTmp->mDefaultTableRef->table,
                          MTC_COLUMN_NOTNULL_TRUE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        // check same(duplicated) column in set clause.
        for ( sColumn = sTmp->mColumns; sColumn != NULL; sColumn = sColumn->next )
        {
            for ( sOtherColumn = sTmp->mColumns; sOtherColumn != NULL; sOtherColumn = sOtherColumn->next )
            {
                if ( sColumn != sOtherColumn )
                {
                    IDE_TEST_RAISE( sColumn->basicInfo == sOtherColumn->basicInfo,
                                    ERR_DUP_SET_CLAUSE);
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUP_SET_CLAUSE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_DUP_COLUMN_IN_SET));
    }
    IDE_EXCEPTION(ERR_NOT_ALLOW_PK_UPDATE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOW_PRIMARY_KEY_UPDATE));
    }
    IDE_EXCEPTION( ERR_CANNOT_USE_HIDDEN_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN)
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::makeUpdateRowForMultiTable( qcStatement * aStatement )
{
    qmmUptParseTree    * sParseTree;
    smiValue           * sInsOrUptRow;
    qcmColumn          * sColumn;
    UShort               sCanonizedTuple;
    UInt                 sOffset;
    mtcTemplate        * sMtcTemplate;
    qmmMultiTables     * sTmp;
    UInt                 i;
    UInt                 sCount = 0;

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sParseTree = (qmmUptParseTree*) aStatement->myPlan->parseTree;

    for ( sTmp = sParseTree->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        if ( sCount < sTmp->mColumnCount )
        {
            sCount = sTmp->mColumnCount;
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDU_FIT_POINT("qmv::makeUpdateRowForMultiTable::alloc::sInsOrUptRow",
                  idERR_ABORT_InsufficientMemory);
    // alloc updating value
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(smiValue) * sCount, (void**)&sInsOrUptRow)
             != IDE_SUCCESS);

    QC_SHARED_TMPLATE(aStatement)->insOrUptRow[ sParseTree->valueIdx ] = sInsOrUptRow;
    QC_SHARED_TMPLATE(aStatement)->insOrUptRowValueCount[ sParseTree->valueIdx ] = sCount;

    for ( sTmp = sParseTree->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        IDE_TEST( qtc::nextTable( &sCanonizedTuple, aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                  != IDE_SUCCESS );

        sTmp->mCanonizedTuple = sCanonizedTuple;

        IDU_FIT_POINT("qmv::makeUpdateRowForMultiTable::alloc::columns",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     ID_SIZEOF(mtcColumn) * sTmp->mColumnCount,
                     (void**)&( sMtcTemplate->rows[sCanonizedTuple].columns))
                 != IDE_SUCCESS);

        IDU_FIT_POINT("qmv::makeUpdateRowForMultiTable::alloc::mIsNull",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtdIsNullFunc) * sTmp->mColumnCount,
                                                (void**)&(sTmp->mIsNull))
                 != IDE_SUCCESS);
        for ( sColumn = sTmp->mColumns, sOffset = 0, i = 0;
              sColumn    != NULL;
              sColumn     = sColumn->next, i++ )
        {
            mtc::copyColumn( &(sMtcTemplate->rows[sCanonizedTuple].columns[i]),
                             sColumn->basicInfo );

            IDE_TEST_RAISE( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                            == SMI_COLUMN_COMPRESSION_TRUE, ERR_NOT_SUPPORTED );

            sOffset = idlOS::align( sOffset, sColumn->basicInfo->module->align );

            // PROJ-1362
            if ( (sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK)
                 == SMI_COLUMN_TYPE_LOB )
            {
                sMtcTemplate->rows[sCanonizedTuple].columns[i].column.offset = sOffset;
                // lob에 최대로 입력될 수 있는 값의 길이는 varchar의 최대값이다.
                sOffset += MTD_CHAR_PRECISION_MAXIMUM;
            }
            else
            {
                sMtcTemplate->rows[sCanonizedTuple].columns[i].column.offset = sOffset;
                sOffset += sMtcTemplate->rows[sCanonizedTuple].columns[i].column.size;
            }

            if ( ( sColumn->basicInfo->flag & MTC_COLUMN_NOTNULL_MASK )
                 == MTC_COLUMN_NOTNULL_TRUE )
            {
                sTmp->mIsNull[i] = sMtcTemplate->rows[sCanonizedTuple].columns[i].module->isNull;
            }
            else
            {
                sTmp->mIsNull[i] = mtd::isNullDefault;
            }
        }

        sMtcTemplate->rows[sCanonizedTuple].modify = 0;
        sMtcTemplate->rows[sCanonizedTuple].lflag = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
        // fixAfterValidation에서 할당하지 않고 바로 할당한다.
        sMtcTemplate->rows[sCanonizedTuple].lflag &= ~MTC_TUPLE_ROW_SKIP_MASK;
        sMtcTemplate->rows[sCanonizedTuple].lflag |= MTC_TUPLE_ROW_SKIP_TRUE;
        sMtcTemplate->rows[sCanonizedTuple].columnCount = sTmp->mColumnCount;
        sMtcTemplate->rows[sCanonizedTuple].columnMaximum = sTmp->mColumnCount;
        sMtcTemplate->rows[sCanonizedTuple].columnLocate = NULL;
        sMtcTemplate->rows[sCanonizedTuple].execute = NULL;
        sMtcTemplate->rows[sCanonizedTuple].rowOffset = sOffset;
        sMtcTemplate->rows[sCanonizedTuple].rowMaximum = sOffset;

        IDU_FIT_POINT("qmv::makeUpdateRowForMultiTable::alloc::row",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc( sOffset,
                                                (void**) & (sMtcTemplate->rows[sCanonizedTuple].row) )
                 != IDE_SUCCESS);
    }

    // compressed tuple 을 사용하지 않음
    sParseTree->compressedTuple = UINT_MAX;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORTED );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmv::makeUpdateRowForMultiTable",
                                  "Not Support Compressed Column" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::validateMultiUpdate( qcStatement * aStatement )
{
    qmmUptParseTree   * sParseTree;
    qcmTableInfo      * sUpdateTableInfo;
    qcmColumn         * sCurrColumn;
    qmmValueNode      * sCurrValue;
    qmmSubqueries     * sSubquery;
    qmmValuePointer   * sValuePointer;
    qmsTarget         * sTarget;
    qcStatement       * sStatement;
    qcuSqlSourceInfo    sqlInfo;
    const mtdModule   * sModule;
    const mtdModule   * sLocatorModule;
    qcmColumn         * sQcmColumn;
    mtcColumn         * sMtcColumn;
    smiColumnList     * sCurrSmiColumn;
    qdConstraintSpec  * sConstr;
    qmmMultiTables    * sTmp;
    qmsFrom             sFrom;
    qmsFrom           * sFromTmp;
    qcNamePosition      sColumnName;
    qtcNode           * sNode[2] = {NULL,NULL};
    qtcNode           * sNodeTemp;
    UInt                i;

    sParseTree = (qmmUptParseTree*)aStatement->myPlan->parseTree;

    for ( sTmp = sParseTree->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        // PR-13725 CHECK OPERATABLE
        IDE_TEST( checkUpdateOperatable( aStatement,
                                         sTmp->mInsteadOfTrigger,
                                         sTmp->mTableRef )
                  != IDE_SUCCESS );

        sUpdateTableInfo = sTmp->mTableRef->tableInfo;
        // check grant
        IDE_TEST( qdpRole::checkDMLUpdateTablePriv( aStatement,
                                                    sUpdateTableInfo->tableHandle,
                                                    sUpdateTableInfo->tableOwnerID,
                                                    sUpdateTableInfo->privilegeCount,
                                                    sUpdateTableInfo->privilegeInfo,
                                                    ID_FALSE,
                                                    NULL,
                                                    NULL )
                  != IDE_SUCCESS );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanPrivTable( aStatement,
                                                  QCM_PRIV_ID_OBJECT_UPDATE_NO,
                                                  sUpdateTableInfo )
                  != IDE_SUCCESS );

        // PROJ-2219 Row-level before update trigger
        // Update column을 column ID순으로 정렬한다.
        IDE_TEST( sortUpdateColumn( aStatement,
                                    &sTmp->mColumns,
                                    sTmp->mColumnCount,
                                    &sTmp->mValues,
                                    sTmp->mValuesPos )
                  != IDE_SUCCESS );

        sQcmColumn = sTmp->mTableRef->tableInfo->columns;
        sColumnName.stmtText = sQcmColumn->name;
        sColumnName.offset = 0;
        sColumnName.size = idlOS::strlen( sQcmColumn->name );

        if ( QC_IS_NULL_NAME( sTmp->mTableRef->aliasName ) == ID_TRUE )
        {
            IDE_TEST( qtc::makeColumn( aStatement,
                                       sNode,
                                       NULL,
                                       &sTmp->mTableRef->tableName,
                                       &sColumnName,
                                       NULL )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qtc::makeColumn( aStatement,
                                       sNode,
                                       NULL,
                                       &sTmp->mTableRef->aliasName,
                                       &sColumnName,
                                       NULL )
                      != IDE_SUCCESS );
        }

        QCP_SET_INIT_QMS_FROM( (&sFrom) );
        sFrom.tableRef = sTmp->mTableRef;

        IDE_TEST ( qtc::estimate( sNode[0],
                                  QC_SHARED_TMPLATE(aStatement),
                                  aStatement,
                                  NULL,
                                  sParseTree->querySet->SFWGH,
                                  &sFrom )
                   != IDE_SUCCESS );
        if ( sTmp->mColumnList == NULL )
        {
            sTmp->mColumnList = sNode[0];
        }
        else
        {
            sNodeTemp = sTmp->mColumnList;
            sTmp->mColumnList = sNode[0];
            sTmp->mColumnList->node.next = &sNodeTemp->node;
        }
    }

    IDE_TEST( makeUpdateRowForMultiTable( aStatement )
              != IDE_SUCCESS );

    for ( sTmp = sParseTree->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        if ( sTmp->mColumnCount > 0 )
        {
            IDU_FIT_POINT("qmv::validateMultiUpdate::alloc::mUptColumnList",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                          ID_SIZEOF(smiColumnList) * sTmp->mColumnCount,
                          (void **) &sTmp->mUptColumnList )
                      != IDE_SUCCESS );

            for ( sCurrColumn = sTmp->mColumns, sCurrSmiColumn = sTmp->mUptColumnList, i = 0;
                  sCurrColumn != NULL;
                  sCurrColumn = sCurrColumn->next, sCurrSmiColumn++, i++ )
            {
                // smiColumnList 정보 설정
                sCurrSmiColumn->column = & sCurrColumn->basicInfo->column;

                if ( i + 1 < sTmp->mColumnCount )
                {
                    sCurrSmiColumn->next = sCurrSmiColumn + 1;
                }
                else
                {
                    sCurrSmiColumn->next = NULL;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    // PROJ-1473 validation of Hints
    IDE_TEST( qmvQuerySet::validateHints(aStatement, sParseTree->querySet->SFWGH)
              != IDE_SUCCESS );

    // RID방식을 사용하도록 설정한다.
    sParseTree->querySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
    sParseTree->querySet->SFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;

    // PROJ-1784 DML Without Retry
    sParseTree->querySet->processPhase = QMS_VALIDATE_SET;

    for ( sSubquery = sParseTree->subqueries;
          sSubquery != NULL;
          sSubquery = sSubquery->next )
    {
        if ( sSubquery->subquery != NULL)
        {
            IDE_TEST(qtc::estimate( sSubquery->subquery,
                                    QC_SHARED_TMPLATE(aStatement),
                                    aStatement,
                                    NULL,
                                    sParseTree->querySet->SFWGH,
                                    NULL )
                     != IDE_SUCCESS);

            if ( ( sSubquery->subquery->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sSubquery->subquery->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }

            if ( QTC_HAVE_AGGREGATE( sSubquery->subquery ) == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sSubquery->subquery->position );
                IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
            }

            sStatement = sSubquery->subquery->subquery;
            sTarget = ((qmsParseTree*)sStatement->myPlan->parseTree)->querySet->target;
            for ( sValuePointer = sSubquery->valuePointer;
                  ( sValuePointer != NULL ) && ( sTarget != NULL );
                  sValuePointer = sValuePointer->next, sTarget = sTarget->next )
            {
                sValuePointer->valueNode->value = sTarget->targetColumn;
            }

            IDE_TEST_RAISE( ( sValuePointer != NULL ) || ( sTarget != NULL ),
                            ERR_INVALID_FUNCTION_ARGUMENT );
        }
    }

    for ( sCurrValue = sParseTree->lists;
          sCurrValue != NULL;
          sCurrValue = sCurrValue->next )
    {
        if ( sCurrValue->value != NULL )
        {
            /* PROJ-2197 PSM Renewal */
            sCurrValue->value->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

            IDE_TEST( notAllowedAnalyticFunc( aStatement, sCurrValue->value )
                      != IDE_SUCCESS );

            if ( qtc::estimate( sCurrValue->value,
                                QC_SHARED_TMPLATE(aStatement),
                                aStatement,
                                NULL,
                                sParseTree->querySet->SFWGH,
                                NULL )
                 != IDE_SUCCESS )
            {
                // default value인 경우 별도 에러처리
                IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                == QTC_NODE_DEFAULT_VALUE_TRUE,
                                ERR_INVALID_DEFAULT_VALUE );

                // default value가 아닌 경우 에러pass
                IDE_TEST( 1 );
            }
            else
            {
                // Nothing to do.
            }

            if ( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrValue->value->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }

            if ( QTC_HAVE_AGGREGATE( sCurrValue->value ) == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrValue->value->position );
                IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
            }
        }
    }

    for ( sTmp = sParseTree->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        for ( sCurrColumn = sTmp->mColumns, sCurrValue = sTmp->mValues, i = 0;
              sCurrColumn != NULL;
              sCurrColumn = sCurrColumn->next, sCurrValue = sCurrValue->next, i++ )
        {
            for ( sSubquery = sParseTree->subqueries;
                  sSubquery != NULL;
                  sSubquery = sSubquery->next )
            {
                for ( sValuePointer = sSubquery->valuePointer;
                      sValuePointer != NULL;
                      sValuePointer = sValuePointer->next )
                {
                    if ( sTmp->mValuesPos[i] == sValuePointer->valueNode )
                    {
                        sCurrValue->value = sValuePointer->valueNode->value;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            if ( sCurrValue->value != NULL )
            {
                if ( sCurrValue->validate == ID_TRUE )
                {
                    /* PROJ-2197 PSM Renewal */
                    sCurrValue->value->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

                    IDE_TEST( notAllowedAnalyticFunc( aStatement, sCurrValue->value )
                              != IDE_SUCCESS );

                    if ( qtc::estimate( sCurrValue->value,
                                        QC_SHARED_TMPLATE(aStatement),
                                        aStatement,
                                        NULL,
                                        sParseTree->querySet->SFWGH,
                                        NULL )
                         != IDE_SUCCESS )
                    {
                        // default value인 경우 별도 에러처리
                        IDE_TEST_RAISE( ( sCurrValue->value->lflag & QTC_NODE_DEFAULT_VALUE_MASK )
                                        == QTC_NODE_DEFAULT_VALUE_TRUE,
                                        ERR_INVALID_DEFAULT_VALUE );

                        // default value가 아닌 경우 에러pass
                        IDE_TEST( 1 );
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    // PROJ-1502 PARTITIONED DISK TABLE
                    IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                      QC_SHARED_TMPLATE(aStatement),
                                                      aStatement )
                              != IDE_SUCCESS );

                    if ( ( sCurrValue->value->node.lflag & MTC_NODE_DML_MASK )
                         == MTC_NODE_DML_UNUSABLE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               &sCurrValue->value->position );
                        IDE_RAISE( ERR_USE_CURSOR_ATTR );
                    }

                    if ( QTC_HAVE_AGGREGATE( sCurrValue->value ) == ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               &sCurrValue->value->position );
                        IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
                    }
                }
                else
                {
                    /* Nothing to do */
                }
                // PROJ-2002 Column Security
                // update t1 set i1=i2 같은 경우 동일 암호 타입이라도 policy가
                // 다를 수 있으므로 i2에 decrypt func을 생성한다.
                sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );

                if ( (sMtcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                     == MTD_ENCRYPT_TYPE_TRUE )
                {
                    // default policy의 암호 타입이라도 decrypt 함수를 생성하여
                    // subquery의 결과는 항상 암호 타입이 나올 수 없게 한다.

                    // add decrypt func
                    IDE_TEST( addDecryptFunc4ValueNode( aStatement,
                                                        sCurrValue )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
                sMtcColumn = QTC_STMT_COLUMN( aStatement, sCurrValue->value );
                sModule = sCurrColumn->basicInfo->module;
                // PROJ-1362
                // add Lob-Locator function
                if ( (sCurrValue->value->node.module == & qtc::columnModule) &&
                     ((sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                      == SMI_COLUMN_TYPE_LOB) )
                {
                    // BUG-33890
                    if ( ( sModule->id == MTD_BLOB_ID ) ||
                         ( sModule->id == MTD_CLOB_ID ) )
                    {
                        IDE_TEST( mtf::getLobFuncResultModule( &sLocatorModule,
                                                               sModule )
                                  != IDE_SUCCESS );

                        IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                           aStatement,
                                                           QC_SHARED_TMPLATE(aStatement),
                                                           sLocatorModule )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        IDE_TEST( qtc::makeConversionNode(sCurrValue->value,
                                                          aStatement,
                                                          QC_SHARED_TMPLATE(aStatement),
                                                          sModule )
                                  != IDE_SUCCESS);
                    }
                }
                else
                {
                    IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                                       aStatement,
                                                       QC_SHARED_TMPLATE(aStatement),
                                                       sModule )
                              != IDE_SUCCESS );
                }

                // PROJ-1502 PARTITIONED DISK TABLE
                IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                                  QC_SHARED_TMPLATE(aStatement),
                                                  aStatement )
                          != IDE_SUCCESS );

                // BUG-15746
                IDE_TEST( describeParamInfo( aStatement,
                                             sCurrColumn->basicInfo,
                                             sCurrValue->value )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    for ( sFromTmp = sParseTree->querySet->SFWGH->from;
          sFromTmp != NULL;
          sFromTmp = sFromTmp->next )
    {
        IDE_TEST( qmvQuerySet::validateJoin( aStatement, sFromTmp, sParseTree->querySet->SFWGH )
                  != IDE_SUCCESS );
    }

    sParseTree->querySet->SFWGH->validatePhase = QMS_VALIDATE_WHERE;

    // validation of WHERE clause
    if ( sParseTree->querySet->SFWGH->where != NULL )
    {
        sParseTree->querySet->processPhase = QMS_VALIDATE_WHERE;

        IDE_TEST( qmvQuerySet::validateWhere( aStatement,
                                              NULL, // querySet : SELECT 구문만 필요
                                              sParseTree->querySet->SFWGH )
                  != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-1436
    if ( sParseTree->querySet->SFWGH->hints != NULL )
    {
        IDE_TEST( validatePlanHints( aStatement, sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    for ( sTmp = sParseTree->mTableList;
          sTmp != NULL;
          sTmp = sTmp->mNext )
    {
        /* PROJ-1107 Check Constraint 지원 */
        for ( sConstr = sTmp->mCheckConstrList;
              sConstr != NULL;
              sConstr = sConstr->next )
        {
            /* checkCondition에 대한 table Name 지정 */
            setTableNameForMultiTable( sConstr->checkCondition, &sTmp->mTableRef->tableName );

            IDE_TEST( qdbCommon::validateCheckConstrDefinition(
                          aStatement,
                          sConstr,
                          sParseTree->querySet->SFWGH,
                          NULL )
                      != IDE_SUCCESS );
        }

        if ( sTmp->mDefaultColumns != NULL )
        {
            QCP_SET_INIT_QMS_FROM( (&sFrom) );
            sFrom.tableRef = sTmp->mDefaultTableRef;

            for ( sQcmColumn = sTmp->mDefaultColumns;
                  sQcmColumn != NULL;
                  sQcmColumn = sQcmColumn->next)
            {
                IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                              aStatement,
                              sQcmColumn->defaultValue,
                              sParseTree->querySet->SFWGH,
                              &sFrom )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( getParentInfoList( aStatement,
                                     sTmp->mTableRef->tableInfo,
                                     &sTmp->mParentConst,
                                     sTmp->mColumns,
                                     sTmp->mColumnCount )
                  != IDE_SUCCESS );

        IDE_TEST( getChildInfoList( aStatement,
                                    sTmp->mTableRef->tableInfo,
                                    &sTmp->mChildConst,
                                    sTmp->mColumns,
                                    sTmp->mColumnCount )
                  != IDE_SUCCESS );

        //---------------------------------------------
        // PROJ-1705 질의에 사용된 컬럼정보수집
        // DML 처리시 포린키 정보수집
        //---------------------------------------------
        IDE_TEST( setFetchColumnInfo4ParentTable( aStatement,
                                                  sTmp->mTableRef )
                  != IDE_SUCCESS );

        IDE_TEST( setFetchColumnInfo4ChildTable( aStatement,
                                                 sTmp->mTableRef )
                  != IDE_SUCCESS );

        // PROJ-2205 DML trigger에 의한 컬럼참조
        IDE_TEST( setFetchColumnInfo4Trigger( aStatement,
                                              sTmp->mTableRef )
                  != IDE_SUCCESS );

        if ( sTmp->mTableRef->tableInfo->replicationCount > 0 )
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_FUNCTION_ARGUMENT);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION( ERR_INVALID_DEFAULT_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE ) );
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_AGGREGATION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_AGGREGATION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::parseMultiDelete( qcStatement * aStatement )
{
    qmmDelParseTree  * sParseTree = NULL;
    qmsFrom          * sFrom = NULL;

    sParseTree = (qmmDelParseTree*) aStatement->myPlan->parseTree;

    /* PROJ-1888 INSTEAD OF TRIGGER */
    // FROM clause
    for ( sFrom = sParseTree->querySet->SFWGH->from;
          sFrom != NULL;
          sFrom = sFrom->next )
    {
        IDE_TEST( parseViewInFromClause( aStatement,
                                         sFrom,
                                         sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );

        IDE_TEST( checkUpdatableView( aStatement, sFrom )
                  != IDE_SUCCESS );
    }

    // WHERE clause
    if ( sParseTree->querySet->SFWGH->where != NULL )
    {
        IDE_TEST( parseViewInExpression( aStatement,
                                         sParseTree->querySet->SFWGH->where )
                  != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    sParseTree->mTableList = NULL;
    sParseTree->deleteTableRef = NULL;
    sParseTree->childConstraints = NULL;
    sParseTree->insteadOfTrigger = ID_FALSE;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::validateMultiDelete( qcStatement * aStatement )
{
    qmmDelParseTree    * sParseTree;
    qmsTableRef        * sDeleteTableRef;
    qcmTableInfo       * sDeleteTableInfo;
    idBool               sTriggerExist = ID_FALSE;
    qmsParseTree       * sViewParseTree = NULL;
    mtcTemplate        * sMtcTemplate;
    qcmViewReadOnly      sReadOnly = QCM_VIEW_NON_READ_ONLY;
    mtcTuple           * sMtcTuple;
    qmsFrom            * sFrom;
    qmmDelMultiTables  * sTmp;
    qmsSFWGH           * sViewSFWGH;
    qmsTarget          * sTarget = NULL;
    qmsTarget          * sReturnTarget;
    qcmColumn          * sQcmColumn;
    qtcNode            * sNode[2] = {NULL,NULL};
    qmsFrom              sFromTmp;
    qcNamePosition       sColumnName;
    qcNamePosList      * sDelName;
    UInt                 sDelCount = 0;
    UInt                 i = 0;

    sParseTree = (qmmDelParseTree*) aStatement->myPlan->parseTree;
    qtc::dependencyClear( & sParseTree->querySet->SFWGH->depInfo );

    // check existence of table and get table META Info.
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    sParseTree->querySet->SFWGH->lflag &= ~(QMV_VIEW_CREATION_MASK);
    sParseTree->querySet->SFWGH->lflag |= (QMV_VIEW_CREATION_FALSE);

    if ( sParseTree->querySet->SFWGH->hints == NULL )
    {
        IDU_FIT_POINT("qmv::parseMultiDelete::STRUCT_ALLOC::hints",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST ( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                 qmsHints,
                                 &(sParseTree->querySet->SFWGH->hints) ) != IDE_SUCCESS);

        QCP_SET_INIT_HINTS(sParseTree->querySet->SFWGH->hints);
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qmvQuerySet::convertAnsiInnerJoin( aStatement, sParseTree->querySet->SFWGH )
              != IDE_SUCCESS );

    for ( sFrom = sParseTree->querySet->SFWGH->from;
          sFrom != NULL;
          sFrom = sFrom->next )
    {
        if ( sFrom->joinType != QMS_NO_JOIN )
        {
            IDE_TEST( qmvQuerySet::validateQmsFromWithOnCond( sParseTree->querySet,
                                                              sParseTree->querySet->SFWGH,
                                                              sFrom,
                                                              aStatement,
                                                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmvQuerySet::validateQmsTableRef( aStatement,
                                                        sParseTree->querySet->SFWGH,
                                                        sFrom->tableRef,
                                                        sParseTree->querySet->SFWGH->lflag,
                                                        MTC_COLUMN_NOTNULL_TRUE) // PR-13597
                      != IDE_SUCCESS);
            // Table Map 설정
            QC_SHARED_TMPLATE(aStatement)->tableMap[sFrom->tableRef->table].from = sFrom;

            // FROM 절에 대한 dependencies 설정
            qtc::dependencyClear( &sFrom->depInfo );
            qtc::dependencySet( sFrom->tableRef->table, &sFrom->depInfo );

            // PROJ-1718 Semi/anti join과 관련된 dependency 초기화
            qtc::dependencyClear( &sFrom->semiAntiJoinDepInfo );

            IDE_TEST( qmsPreservedTable::addTable( aStatement,
                                                   sParseTree->querySet->SFWGH,
                                                   sFrom->tableRef )
                      != IDE_SUCCESS );
        }
    }

    // Query Set에 dependency 설정
    qtc::dependencyClear( &sParseTree->querySet->depInfo );
    IDE_TEST( qtc::dependencyOr( &sParseTree->querySet->depInfo,
                                 &sParseTree->querySet->SFWGH->depInfo,
                                 &sParseTree->querySet->depInfo )
              != IDE_SUCCESS );

    for ( sFrom = sParseTree->querySet->SFWGH->from;
          sFrom != NULL;
          sFrom = sFrom->next )
    {
        IDE_TEST( makeMultiTable( aStatement,
                                  sFrom,
                                  sParseTree->mDelList,
                                  &sParseTree->mTableList )
                  != IDE_SUCCESS );
    }

    for ( sDelName = sParseTree->mDelList; sDelName != NULL; sDelName = sDelName->next )
    {
        sDelCount++;
    }

    for ( sTmp = sParseTree->mTableList; sTmp != NULL; sTmp = sTmp->mNext )
    {
        i++;
    }

    IDE_TEST_RAISE( sDelCount != i, ERR_TABLE_NOT_FOUND );

    for ( sTmp = sParseTree->mTableList; sTmp != NULL ; sTmp = sTmp->mNext )
    {
        /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
        IDE_TEST( checkInsteadOfTrigger( sTmp->mTableRef,
                                         QCM_TRIGGER_EVENT_DELETE,
                                         &sTriggerExist ) );

        /* PROJ-1888 INSTEAD OF TRIGGER */
        if ( sTriggerExist == ID_TRUE )
        {
            sTmp->mInsteadOfTrigger = ID_TRUE;
        }
        else
        {
            sTmp->mInsteadOfTrigger = ID_FALSE;
            sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

            // created view, inline view
            if (( ( sMtcTemplate->rows[sTmp->mTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
                  == MTC_TUPLE_VIEW_TRUE ) &&
                ( sTmp->mTableRef->tableType != QCM_PERFORMANCE_VIEW ))
            {
                sDeleteTableRef = NULL;
                sViewParseTree = ( qmsParseTree *)sTmp->mTableRef->view->myPlan->parseTree;
                sViewSFWGH     = sViewParseTree->querySet->SFWGH; /* BUG-46124 */

                /* BUG-49136 */
                IDE_TEST_RAISE( sViewSFWGH == NULL, ERR_NOT_KEY_PRESERVED_TABLE );

                /* BUG-36350 Updatable Join DML WITH READ ONLY*/
                if ( sTmp->mTableRef->tableInfo->tableID != 0 )
                {
                    /* view read only */
                    IDE_TEST( qcmView::getReadOnlyOfViews(
                                  QC_SMI_STMT( aStatement ),
                                  sTmp->mTableRef->tableInfo->tableID,
                                  &sReadOnly )
                              != IDE_SUCCESS);
                    /* read only insert error */
                    IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY, ERR_NOT_DNL_READ_ONLY_VIEW );
                }
                else
                {
                    /* Nothing To Do */
                }

                // view의 from절에서 첫번째 key preseved table을 얻어온다.
                if ( sViewParseTree->querySet->SFWGH != NULL )
                {
                    // RID방식을 사용하도록 설정한다.
                    sViewParseTree->querySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
                    sViewParseTree->querySet->SFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;

                    IDE_TEST( qmsPreservedTable::getFirstKeyPrevTable(
                                  sViewParseTree->querySet->SFWGH,
                                  & sDeleteTableRef )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                /* BUG-46124 */
                IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                               sViewSFWGH,
                                                                               sViewSFWGH->from,
                                                                               sTarget,
                                                                               & sReturnTarget )
                          != IDE_SUCCESS );

                // error delete할 테이블이 없음
                IDE_TEST_RAISE( sDeleteTableRef == NULL, ERR_NOT_KEY_PRESERVED_TABLE );

                /* BUG-39399 remove search key preserved table  */
                sMtcTuple = &QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sDeleteTableRef->table];

                /* BUG-46124 */
                IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                                  != MTC_TUPLE_VIEW_FALSE ) ||
                                ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                                  != MTC_TUPLE_KEY_PRESERVED_TRUE ),
                                ERR_NOT_KEY_PRESERVED_TABLE );

                sMtcTuple->lflag &= ~MTC_TUPLE_TARGET_UPDATE_DELETE_MASK;
                sMtcTuple->lflag |= MTC_TUPLE_TARGET_UPDATE_DELETE_TRUE;

                sTmp->mViewID = sTmp->mTableRef->table;
                sTmp->mTableRef = sDeleteTableRef;
            }
            else
            {
                /* Nothing to do */
            }

        }

        sTmp->mTableRef->flag &= ~QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK;
        sTmp->mTableRef->flag |= QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE;
        sDeleteTableInfo = sTmp->mTableRef->tableInfo;

        IDE_TEST( checkDeleteOperatable( aStatement,
                                         sTmp->mInsteadOfTrigger,
                                         sTmp->mTableRef )
                  != IDE_SUCCESS );

        /* PROJ-2211 Materialized View */
        if ( sDeleteTableInfo->tableType == QCM_MVIEW_TABLE )
        {
            IDE_TEST_RAISE( sParseTree->querySet->SFWGH->hints == NULL,
                            ERR_NO_GRANT_DML_MVIEW_TABLE );

            IDE_TEST_RAISE( sParseTree->querySet->SFWGH->hints->refreshMView != ID_TRUE,
                            ERR_NO_GRANT_DML_MVIEW_TABLE );
        }
        else
        {
            /* Nothing to do */
        }

        // check grant
        IDE_TEST( qdpRole::checkDMLDeleteTablePriv( aStatement,
                                                    sDeleteTableInfo->tableHandle,
                                                    sDeleteTableInfo->tableOwnerID,
                                                    sDeleteTableInfo->privilegeCount,
                                                    sDeleteTableInfo->privilegeInfo,
                                                    ID_FALSE,
                                                    NULL,
                                                    NULL )
                  != IDE_SUCCESS );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanPrivTable( aStatement,
                                                  QCM_PRIV_ID_OBJECT_DELETE_NO,
                                                  sDeleteTableInfo )
                  != IDE_SUCCESS );
    }

    // validation of Hints
    IDE_TEST(qmvQuerySet::validateHints(aStatement, sParseTree->querySet->SFWGH)
             != IDE_SUCCESS);

    // RID방식을 사용하도록 설정한다.
    sParseTree->querySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
    sParseTree->querySet->SFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;

    /* PROJ-1888 INSTEAD OF TRIGGER */
    sParseTree->querySet->SFWGH->validatePhase = QMS_VALIDATE_TARGET;
    IDE_TEST( qmvQuerySet::validateQmsTarget( aStatement,
                                              sParseTree->querySet,
                                              sParseTree->querySet->SFWGH )
              != IDE_SUCCESS );

    // target 생성
    sParseTree->querySet->target = sParseTree->querySet->SFWGH->target;

    for ( sFrom = sParseTree->querySet->SFWGH->from;
          sFrom != NULL;
          sFrom = sFrom->next )
    {
        IDE_TEST( qmvQuerySet::validateJoin( aStatement, sFrom, sParseTree->querySet->SFWGH )
                  != IDE_SUCCESS );
    }

    sParseTree->querySet->SFWGH->validatePhase = QMS_VALIDATE_WHERE;

    // validation of WHERE clause
    if (sParseTree->querySet->SFWGH->where != NULL)
    {
        sParseTree->querySet->processPhase = QMS_VALIDATE_WHERE;

        IDE_TEST( qmvQuerySet::validateWhere( aStatement,
                                              NULL, // querySet : SELECT 구문만 querySet 필요
                                              sParseTree->querySet->SFWGH )
                  != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-1436
    if ( sParseTree->querySet->SFWGH->hints != NULL )
    {
        IDE_TEST( validatePlanHints( aStatement,
                                     sParseTree->querySet->SFWGH->hints )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    for ( sTmp = sParseTree->mTableList; sTmp != NULL ; sTmp = sTmp->mNext )
    {
        sDeleteTableInfo = sTmp->mTableRef->tableInfo;
        sDeleteTableRef = sTmp->mTableRef;

        IDE_TEST(getChildInfoList(aStatement,
                                  sDeleteTableInfo,
                                  &(sTmp->mChildConstraints))
                 != IDE_SUCCESS);
        IDE_TEST( setFetchColumnInfo4ChildTable( aStatement,
                                                 sDeleteTableRef )
                  != IDE_SUCCESS );

        // PROJ-2205 DML trigger에 의한 컬럼참조
        IDE_TEST( setFetchColumnInfo4Trigger( aStatement,
                                              sDeleteTableRef )
                  != IDE_SUCCESS );

        sQcmColumn = sTmp->mTableRef->tableInfo->columns;

        sColumnName.stmtText = sQcmColumn->name;
        sColumnName.offset = 0;
        sColumnName.size = idlOS::strlen( sQcmColumn->name );

        if ( QC_IS_NULL_NAME( sTmp->mTableRef->aliasName ) == ID_TRUE )
        {
            IDE_TEST( qtc::makeColumn( aStatement,
                                       sNode,
                                       NULL,
                                       &sTmp->mTableRef->tableName,
                                       &sColumnName,
                                       NULL )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qtc::makeColumn( aStatement,
                                       sNode,
                                       NULL,
                                       &sTmp->mTableRef->aliasName,
                                       &sColumnName,
                                       NULL )
                      != IDE_SUCCESS );
        }

        QCP_SET_INIT_QMS_FROM( (&sFromTmp) );
        sFromTmp.tableRef = sTmp->mTableRef;

        IDE_TEST ( qtc::estimate( sNode[0],
                                  QC_SHARED_TMPLATE(aStatement),
                                  aStatement,
                                  NULL,
                                  sParseTree->querySet->SFWGH,
                                  &sFromTmp )
                   != IDE_SUCCESS );

        sTmp->mColumnList = sNode[0];

        /*
         * BUG-39441
         * need a interface which returns whether DML on replication table or not
         */
        if ( sDeleteTableInfo->replicationCount > 0 )
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_REPL_TABLE_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_REPL_TABLE_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* PROJ-2632 */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_DISABLE_SERIAL_FILTER_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |=  QC_TMP_DISABLE_SERIAL_FILTER_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DML_MVIEW_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GRANT_DML_PRIV_OF_MVIEW_TABLE ) );
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION(ERR_TABLE_NOT_FOUND);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TABLE_NOT_FOUND));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::makeMultiTable( qcStatement        * aStatement,
                            qmsFrom            * aFrom,
                            qcNamePosList      * aDelList,
                            qmmDelMultiTables ** aTableList )
{
    qmmDelMultiTables * sTableList;
    qmmDelMultiTables * sTmp;
    qcNamePosList     * sDelName;
    idBool              sIsFound;
    qcNamePosition    * sNamePos;

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        sIsFound = ID_FALSE;
        for ( sDelName = aDelList; sDelName != NULL; sDelName = sDelName->next )
        {
            if ( QC_IS_NULL_NAME( aFrom->tableRef->aliasName )
                 == ID_TRUE )
            {
                sNamePos = &aFrom->tableRef->tableName;
                if ( idlOS::strMatch( sNamePos->stmtText + sNamePos->offset,
                                      sNamePos->size,
                                      sDelName->namePos.stmtText + sDelName->namePos.offset,
                                      sDelName->namePos.size )
                     == 0 )
                {
                    sIsFound = ID_TRUE;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

            }
            else
            {
                sNamePos = &aFrom->tableRef->aliasName;
                if ( idlOS::strMatch( sNamePos->stmtText + sNamePos->offset,
                                      sNamePos->size,
                                      sDelName->namePos.stmtText + sDelName->namePos.offset,
                                      sDelName->namePos.size )
                     == 0 )
                {
                    sIsFound = ID_TRUE;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        if ( sIsFound == ID_TRUE )
        {
            if ( *aTableList == NULL )
            {
                IDU_FIT_POINT("qmv::makeMultiTable::STRUCT_ALLOC::sTableList",
                              idERR_ABORT_InsufficientMemory);
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                        qmmDelMultiTables,
                                        &sTableList )
                          != IDE_SUCCESS );
                QMM_INIT_DEL_MULTI_TABLES( sTableList );
                sTableList->mTableRef = aFrom->tableRef;

                *aTableList = sTableList;
            }
            else
            {
                for ( sTmp = *aTableList; sTmp != NULL; sTmp = sTmp->mNext )
                {
                    if ( sTmp->mTableRef == aFrom->tableRef )
                    {
                        break;
                    }
                    else
                    {
                        if ( sTmp->mNext == NULL )
                        {
                            IDU_FIT_POINT("qmv::makeMultiTable::STRUCT_ALLOC::sTableList2",
                                          idERR_ABORT_InsufficientMemory);
                            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                                    qmmDelMultiTables,
                                                    &sTableList )
                                      != IDE_SUCCESS );
                            QMM_INIT_DEL_MULTI_TABLES( sTableList );
                            sTableList->mTableRef = aFrom->tableRef;
                            sTmp->mNext = sTableList;
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                }
            }
        }
    }
    else
    {
        IDE_TEST( makeMultiTable( aStatement,
                                  aFrom->left,
                                  aDelList,
                                  aTableList )
                  != IDE_SUCCESS );

        IDE_TEST( makeMultiTable( aStatement,
                                  aFrom->right,
                                  aDelList,
                                  aTableList )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::makeNewUpdateColumnListMultiTable( qcStatement    * aQcStmt,
                                               qmmMultiTables * aTable )
{
    qcmTableInfo    * sTableInfo;
    qcmColumn       * sQcmColumn;

    UChar           * sRefColumnList  = NULL;
    UInt              sRefColumnCount = 0;

    qmmValueNode    * sCurrValueNodeList;
    qcmColumn       * sCurrQcmColumn;

    qmmValueNode    * sNewValueNode = NULL;
    qcmColumn       * sNewQcmColumn = NULL;
    qcNamePosition  * sNamePosition = NULL;
    qtcNode         * sNode[2] = {NULL,NULL};

    UInt    i;

    sTableInfo = aTable->mTableRef->tableInfo;

    // PSM load 중 PSM내의 update DML을 validation을 할 때,
    // trigger의 ref column을 고려하지 않는다.
    // Trigger를 아직 load 하지 않았기 때문이다.
    IDE_TEST_CONT( aQcStmt->spvEnv->createProc != NULL, skip_make_list );

    IDE_TEST_CONT( aQcStmt->spvEnv->createPkg != NULL, skip_make_list );

    IDE_TEST( qmv::getRefColumnListMultiTable( aQcStmt,
                                               aTable,
                                               &sRefColumnList,
                                               &sRefColumnCount )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sRefColumnCount == 0, skip_make_list );

    // Update target column에 속한 것은 ref column list에서 제외한다.
    for ( sQcmColumn  = aTable->mColumns;
          sQcmColumn != NULL;
          sQcmColumn  = sQcmColumn->next )
    {
        i = sQcmColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

        if ( sRefColumnList[i] == QDN_REF_COLUMN_TRUE )
        {
            sRefColumnList[i] = QDN_REF_COLUMN_FALSE;
            sRefColumnCount--;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_CONT( sRefColumnCount == 0, skip_make_list );

    IDU_FIT_POINT("qmv::makeNewUpdateColumnListMultiTable::alloc::sNewValueNode",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                  ID_SIZEOF(qmmValueNode) * sRefColumnCount,
                  (void**)&sNewValueNode )
              != IDE_SUCCESS);

    IDU_FIT_POINT("qmv::makeNewUpdateColumnListMultiTable::alloc::sNamePosition",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                  ID_SIZEOF(qcNamePosition) * sRefColumnCount,
                  (void**)&sNamePosition )
              != IDE_SUCCESS);

    IDU_FIT_POINT("qmv::makeNewUpdateColumnListMultiTable::alloc::sNewQcmColumn",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                  ID_SIZEOF(qcmColumn) * sRefColumnCount,
                  (void**)&sNewQcmColumn )
              != IDE_SUCCESS);

    sQcmColumn = sTableInfo->columns;

    sCurrValueNodeList = aTable->mValues;
    sCurrQcmColumn     = aTable->mColumns;

    // Update할 value list와 column list의 마지막으로 이동하여,
    // ref column list에 있는 것을 마지막에 추가할 수 있게 한다.
    IDE_FT_ERROR( sCurrValueNodeList != NULL );
    IDE_FT_ERROR( sCurrQcmColumn     != NULL );

    while ( sCurrValueNodeList->next != NULL )
    {
        IDE_FT_ERROR( sCurrQcmColumn->next != NULL );

        sCurrValueNodeList = sCurrValueNodeList->next;
        sCurrQcmColumn     = sCurrQcmColumn->next;
    }

    for ( i = 0; i < sTableInfo->columnCount; i++ )
    {
        if ( sRefColumnList[i] == QDN_REF_COLUMN_TRUE )
        {
            QCM_COLUMN_INIT( sNewQcmColumn );

            sNewValueNode->validate  = ID_TRUE;
            sNewValueNode->calculate = ID_TRUE;
            sNewValueNode->timestamp = ID_FALSE;
            sNewValueNode->expand    = ID_FALSE;
            sNewValueNode->msgID     = ID_FALSE;
            sNewValueNode->next      = NULL;

            sNamePosition->size   = idlOS::strlen( sQcmColumn->name );
            sNamePosition->offset = 0;

            IDU_FIT_POINT("qmv::makeNewUpdateColumnListMultiTable::alloc::sNamePosition2",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                          (sNamePosition->size + 1),
                          (void**)&sNamePosition->stmtText )
                      != IDE_SUCCESS);

            idlOS::memcpy( sNamePosition->stmtText,
                           sQcmColumn->name,
                           sNamePosition->size );
            sNamePosition->stmtText[sNamePosition->size] = '\0';

            sNewQcmColumn->basicInfo = sQcmColumn->basicInfo;
            sNewQcmColumn->namePos   = *sNamePosition;

            IDE_TEST( qtc::makeColumn( aQcStmt,
                                       sNode,
                                       NULL,          // user
                                       &aTable->mTableRef->tableName, // table
                                       sNamePosition, // column
                                       NULL )         // package
                      != IDE_SUCCESS );

            sNewValueNode->value = sNode[0];

            sCurrValueNodeList->next = sNewValueNode;
            sCurrQcmColumn->next     = sNewQcmColumn;

            sCurrValueNodeList = sNewValueNode;
            sCurrQcmColumn     = sNewQcmColumn;

            sNamePosition++;
            sNewValueNode++;
            sNewQcmColumn++;
        }
        else
        {
            // Nothing to do.
        }

        sQcmColumn = sQcmColumn->next;
    }

    aTable->mColumnCount += sRefColumnCount;

    IDE_EXCEPTION_CONT( skip_make_list );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmv::getRefColumnListMultiTable( qcStatement     * aQcStmt,
                                        qmmMultiTables  * aTable,
                                        UChar          ** aRefColumnList,
                                        UInt            * aRefColumnCount )
{
    qdnTriggerCache           * sTriggerCache;
    qcmTriggerInfo            * sTriggerInfo;
    qcmTableInfo              * sTableInfo;
    qdnCreateTriggerParseTree * sTriggerParseTree;
    qdnTriggerEventTypeList   * sEventTypeList;

    UChar                     * sRefColumnList  = NULL;
    UInt                        sRefColumnCount = 0;

    UChar                     * sTriggerRefColumnList;
    UInt                        sTriggerRefColumnCount;

    qcmColumn                 * sQcmColumn;
    smiColumn                 * sSmiColumn;
    qcmColumn                 * sTriggerQcmColumn;

    UInt                        i;
    UInt                        j;
    idBool                      sNeedCheck = ID_FALSE;
    volatile UInt               sStage;

    IDE_FT_BEGIN();

    sTableInfo   = aTable->mTableRef->tableInfo;
    sTriggerInfo = sTableInfo->triggerInfo;
    sStage       = 0; /* BUG-45994 - 컴파일러 최적화 회피 */

    for ( i = 0; i < sTableInfo->triggerCount; i++, sTriggerInfo++ )
    {
        sNeedCheck = ID_FALSE;

        if ( ( sTriggerInfo->enable == QCM_TRIGGER_ENABLE ) &&
             ( sTriggerInfo->granularity == QCM_TRIGGER_ACTION_EACH_ROW ) &&
             ( sTriggerInfo->eventTime == QCM_TRIGGER_BEFORE ) &&
             ( ( sTriggerInfo->eventType & QCM_TRIGGER_EVENT_UPDATE ) != 0 ) )
        {
            IDE_TEST( smiObject::getObjectTempInfo( sTriggerInfo->triggerHandle,
                                                    (void**)&sTriggerCache )
                      != IDE_SUCCESS );
            IDE_TEST( sTriggerCache->latch.lockRead( NULL, NULL ) != IDE_SUCCESS );

            sStage = 1;

            /* BUG-45994 */
            IDU_FIT_POINT_FATAL( "qmv::getRefColumnList::__FT__::STAGE1" );

            // invalid 상태의 trigger는 무시한다.
            if ( sTriggerCache->isValid == ID_TRUE )
            {
                sTriggerParseTree = (qdnCreateTriggerParseTree*)sTriggerCache->triggerStatement.myPlan->parseTree;
                sEventTypeList    = sTriggerParseTree->triggerEvent.eventTypeList;

                if ( sTriggerInfo->uptCount != 0 )
                {
                    for ( sQcmColumn = aTable->mColumns;
                          ( sQcmColumn != NULL ) && ( sNeedCheck == ID_FALSE );
                          sQcmColumn = sQcmColumn->next )
                    {
                        sSmiColumn = &sQcmColumn->basicInfo->column;
                        for ( sTriggerQcmColumn = sEventTypeList->updateColumns;
                              sTriggerQcmColumn != NULL;
                              sTriggerQcmColumn = sTriggerQcmColumn->next )
                        {
                            if ( sTriggerQcmColumn->basicInfo->column.id ==
                                 sSmiColumn->id )
                            {
                                sNeedCheck = ID_TRUE;
                                break;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                    }
                }
                else
                {
                    sNeedCheck = ID_TRUE;
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( sNeedCheck == ID_TRUE )
            {
                if ( sRefColumnList == NULL )
                {
                    IDU_FIT_POINT( "qmv::getRefColumnList::cralloc::sRefColumnList",
                                   idERR_ABORT_InsufficientMemory );

                    IDE_TEST( QC_QME_MEM( aQcStmt )->cralloc(
                                  ID_SIZEOF( UChar ) * sTableInfo->columnCount,
                                  (void**)&sRefColumnList )
                              != IDE_SUCCESS);
                }
                else
                {
                    // Nothing to do.
                }

                sTriggerRefColumnList  = sTriggerParseTree->refColumnList;
                sTriggerRefColumnCount = sTriggerParseTree->refColumnCount;

                if ( ( sTriggerRefColumnList != NULL ) &&
                     ( sTriggerRefColumnCount > 0 ) )
                {
                    for ( j = 0; j < sTableInfo->columnCount; j++ )
                    {
                        if ( ( sTriggerRefColumnList[j] == QDN_REF_COLUMN_TRUE ) &&
                             ( sRefColumnList[j]        == QDN_REF_COLUMN_FALSE ) )
                        {
                            sRefColumnList[j] = QDN_REF_COLUMN_TRUE;
                            sRefColumnCount++;
                        }
                        else
                        {
                            // Nothing to do.
                        }
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

            sStage = 0;
            IDE_TEST( sTriggerCache->latch.unlock() != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    *aRefColumnList  = sRefColumnList;
    *aRefColumnCount = sRefColumnCount;

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    switch ( sStage )
    {
        case 1:
            (void) sTriggerCache->latch.unlock();
        default:
            break;
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

void qmv::setTableNameForMultiTable( qtcNode        * aNode,
                                     qcNamePosition * aTableName )
{
    if ( aNode->node.module == &qtc::columnModule )
    {
        aNode->tableName = *aTableName;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.arguments != NULL )
    {
        setTableNameForMultiTable( ( qtcNode * )aNode->node.arguments, aTableName );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.next != NULL )
    {
        setTableNameForMultiTable( ( qtcNode * )aNode->node.next, aTableName );
    }
    else
    {
        /* Nothing to do */
    }
}

IDE_RC qmv::makeAndSetMultiTable( qcStatement     * aStatement,
                                  qmmUptParseTree * aParseTree,
                                  qmsTableRef     * aTableRef,
                                  qcmColumn       * aColumn,
                                  qmmValueNode    * aValue,
                                  UInt              aValueCount,
                                  qmmMultiTables ** aTableList )
{
    qmmMultiTables  * sTmp = NULL;
    qmmMultiTables  * sPrev = NULL;
    idBool            sIsExist = ID_FALSE;
    idBool            sIsView = ID_FALSE;
    mtcTemplate     * sMtcTemplate;
    qcmColumn       * sNewColumn;
    qcmColumn       * sPrevColumn;
    qcmColumn       * sColumnInfo;
    qmmValueNode    * sNewValue;
    qmmValueNode    * sPrevValue;
    qtcNode         * sPrevNode;
    UInt              sIterator;
    qtcNode         * sNode[2] = {NULL,NULL};
    qmsFrom           sFromTmp;
    qmsTableRef     * sUpdateTableRef = NULL;
    qmsParseTree    * sViewParseTree = NULL;
    qmsSFWGH        * sViewSFWGH = NULL;
    qcmViewReadOnly   sReadOnly = QCM_VIEW_NON_READ_ONLY;
    qmsTarget       * sTarget;
    qmsTarget       * sReturnTarget;
    mtcTuple        * sMtcTuple;
    mtcColumn       * sMtcColumn;
    UShort            sUpdateTupleId = ID_USHORT_MAX;
    UInt              i;

    /* 이미 생성된 것이 있는지 찾는다 */
    for ( sTmp = *aTableList; sTmp != NULL; sTmp = sTmp->mNext )
    {
        if ( sTmp->mTableRef == aTableRef )
        {
            sIsExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
        sPrev = sTmp;
    }

    if ( sIsExist == ID_FALSE )
    {
        IDU_FIT_POINT("qmv::makeAndSetMultiTable::STRUCT_ALLOC::sTmp",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qmmMultiTables,
                                &sTmp )
                  != IDE_SUCCESS );
        QMM_INIT_MULTI_TABLES( sTmp );

        if ( sPrev == NULL )
        {
            *aTableList = sTmp;
        }
        else
        {
            sPrev->mNext = sTmp;
        }

        sTmp->mTableRef = aTableRef;

        if ( aParseTree->subqueries != NULL )
        {
            IDU_FIT_POINT("qmv::makeAndSetMultiTable::alloc::mValuesPos",
                          idERR_ABORT_InsufficientMemory);
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmmValueNode *) * aValueCount,
                                                     (void **) &sTmp->mValuesPos )
                      != IDE_SUCCESS );

            for ( i = 0; i < aValueCount; i++ )
            {
                sTmp->mValuesPos[i] = NULL;
            }
        }
        else
        {
            /* Nothing to do */
        }

        /* instead of trigger 이면 view를 update하지 않는다 ( instead of trigger수행). */
        IDE_TEST( checkInsteadOfTrigger( sTmp->mTableRef,
                                         QCM_TRIGGER_EVENT_UPDATE,
                                         &sTmp->mInsteadOfTrigger ) );
    }
    else
    {
        /* Nothing to do */
    }

    /* 이전 column과 value, qtcNode를 찾는다 */
    for ( sPrevColumn = sTmp->mColumns, sIterator = 0,
            sPrevValue = sTmp->mValues, sPrevNode = sTmp->mColumnList;
          sPrevColumn != NULL;
          sPrevColumn = sPrevColumn->next, sIterator++,
            sPrevValue = sPrevValue->next, sPrevNode = (qtcNode * )sPrevNode->node.next )
    {
        if ( sPrevColumn->next == NULL )
        {
            sIterator++;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sMtcTemplate = &QC_SHARED_TMPLATE(aStatement)->tmplate;
    // created view, inline view
    if (( ( sMtcTemplate->rows[sTmp->mTableRef->table].lflag & MTC_TUPLE_VIEW_MASK )
          == MTC_TUPLE_VIEW_TRUE ) &&
        ( sTmp->mTableRef->tableType != QCM_PERFORMANCE_VIEW ) &&
        ( sTmp->mInsteadOfTrigger == ID_FALSE ))
    {
        sViewParseTree = (qmsParseTree*) sTmp->mTableRef->view->myPlan->parseTree;
        sViewSFWGH     = sViewParseTree->querySet->SFWGH;

        // update할 태이블이 없음
        IDE_TEST_RAISE( sViewParseTree->querySet->SFWGH == NULL,
                        ERR_TABLE_NOT_FOUND );

        /* BUG-36350 Updatable Join DML WITH READ ONLY */
        if ( sTmp->mTableRef->tableInfo->tableID != 0 )
        {
            /* view read only */
            IDE_TEST( qcmView::getReadOnlyOfViews( QC_SMI_STMT( aStatement ),
                                                   sTmp->mTableRef->tableInfo->tableID,
                                                   &sReadOnly )
                      != IDE_SUCCESS );
            IDE_TEST_RAISE( sReadOnly == QCM_VIEW_READ_ONLY, ERR_NOT_DNL_READ_ONLY_VIEW );
        }
        else
        {
            /* Nothing to do */
        }
        // RID방식을 사용하도록 설정한다.
        sViewParseTree->querySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
        sViewParseTree->querySet->SFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;

        for ( sColumnInfo = sTmp->mTableRef->tableInfo->columns,
                sTarget = sViewParseTree->querySet->target;
              sColumnInfo != NULL;
              sColumnInfo = sColumnInfo->next, sTarget = sTarget->next )
        {
            if ( aColumn->basicInfo->column.id == sColumnInfo->basicInfo->column.id )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        /* BUG-46124 */
        IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                       sViewSFWGH,
                                                                       sViewSFWGH->from,
                                                                       sTarget,
                                                                       &sReturnTarget )
                  != IDE_SUCCESS );

        // key-preserved column 검사
        sMtcTuple  = sMtcTemplate->rows + sReturnTarget->targetColumn->node.baseTable;
        sMtcColumn = sMtcTuple->columns + sReturnTarget->targetColumn->node.baseColumn;

        IDE_TEST_RAISE( ( ( sMtcTuple->lflag & MTC_TUPLE_VIEW_MASK )
                          != MTC_TUPLE_VIEW_FALSE ) ||
                        ( ( sMtcTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
                          != MTC_TUPLE_KEY_PRESERVED_TRUE ) ||
                        ( ( sMtcColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
                          != MTC_COLUMN_KEY_PRESERVED_TRUE ),
                        ERR_NOT_KEY_PRESERVED_TABLE );

        /* BUG-39399 remove search key preserved table  */
        sMtcTuple->lflag &= ~MTC_TUPLE_TARGET_UPDATE_DELETE_MASK;
        sMtcTuple->lflag |= MTC_TUPLE_TARGET_UPDATE_DELETE_TRUE;

        if ( sUpdateTupleId == ID_USHORT_MAX )
        {
            // first
            sUpdateTupleId = sReturnTarget->targetColumn->node.baseTable;
        }
        else
        {
            // update할 태이블이 여러개임
            IDE_TEST_RAISE( sUpdateTupleId != sReturnTarget->targetColumn->node.baseTable,
                            ERR_NOT_ONE_BASE_TABLE );
        }
        sUpdateTableRef =
            QC_SHARED_TMPLATE(aStatement)->tableMap[sUpdateTupleId].from->tableRef;

        sColumnInfo = sUpdateTableRef->tableInfo->columns + sReturnTarget->targetColumn->node.baseColumn;

        IDU_FIT_POINT("qmv::makeAndSetMultiTable::alloc::sNewColumn",
                      idERR_ABORT_InsufficientMemory);
        // make column
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcmColumn), (void **) &sNewColumn )
                  != IDE_SUCCESS );

        // copy column
        idlOS::memcpy( sNewColumn, aColumn, ID_SIZEOF(qcmColumn) );
        QMV_SET_QCM_COLUMN(sNewColumn, sColumnInfo);
        sNewColumn->next = NULL;
        sIsView = ID_TRUE;
    }
    else
    {
        IDU_FIT_POINT("qmv::makeAndSetMultiTable::alloc::sNewColumn2",
                      idERR_ABORT_InsufficientMemory);
        // make column
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcmColumn), (void **) &sNewColumn )
                  != IDE_SUCCESS );
        // copy column
        idlOS::memcpy( sNewColumn, aColumn, ID_SIZEOF(qcmColumn) );
        sNewColumn->next = NULL;
    }

    IDU_FIT_POINT("qmv::makeAndSetMultiTable::alloc::sNewValue",
                  idERR_ABORT_InsufficientMemory);
    // make value
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmmValueNode), (void **) &sNewValue )
              != IDE_SUCCESS );
    idlOS::memcpy( sNewValue, aValue, ID_SIZEOF(qmmValueNode) );
    sNewValue->next = NULL;

    if ( sTmp->mColumns == NULL )
    {
        sTmp->mColumns = sNewColumn;
        sTmp->mValues = sNewValue;

        if ( aParseTree->subqueries != NULL )
        {
            sTmp->mValuesPos[0] = aValue;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        sPrevColumn->next = sNewColumn;
        sPrevValue->next = sNewValue;

        if ( aParseTree->subqueries != NULL )
        {
            sTmp->mValuesPos[sIterator] = aValue;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sTmp->mColumnCount++;

    IDE_TEST( qtc::makeColumn( aStatement,
                               sNode,
                               QC_IS_NULL_NAME(sNewColumn->userNamePos) ? NULL : &sNewColumn->userNamePos,
                               QC_IS_NULL_NAME(sNewColumn->tableNamePos) ? NULL : &sNewColumn->tableNamePos,
                               QC_IS_NULL_NAME(sNewColumn->namePos) ? NULL : &sNewColumn->namePos,
                               NULL )
              != IDE_SUCCESS );

    QCP_SET_INIT_QMS_FROM( (&sFromTmp) );
    sFromTmp.tableRef = sTmp->mTableRef;

    if ( sIsView == ID_TRUE )
    {
        sTmp->mViewID = sTmp->mTableRef->table;
        sTmp->mTableRef = sUpdateTableRef;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST ( qtc::estimate( sNode[0],
                              QC_SHARED_TMPLATE(aStatement),
                              aStatement,
                              NULL,
                              aParseTree->querySet->SFWGH,
                              &sFromTmp )
               != IDE_SUCCESS );

    if ( sTmp->mColumnList == NULL )
    {
        sTmp->mColumnList = sNode[0];
    }
    else
    {
        sPrevNode->node.next = (mtcNode *)sNode[0];
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TABLE_NOT_FOUND);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TABLE_NOT_FOUND));
    }
    IDE_EXCEPTION( ERR_NOT_DNL_READ_ONLY_VIEW );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW));
    }
    IDE_EXCEPTION(ERR_NOT_KEY_PRESERVED_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE));
    }
    IDE_EXCEPTION(ERR_NOT_ONE_BASE_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_ONE_BASE_TABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
