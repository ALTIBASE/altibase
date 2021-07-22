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
 * $Id: qmvQuerySet.cpp 90409 2021-04-01 04:19:46Z donovan.seo $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <mtc.h>
#include <qcg.h>
#include <qmvQuerySet.h>
#include <qtc.h>
#include <qtcDef.h>
#include <qcmUser.h>
#include <qcmFixedTable.h>
#include <qmv.h>
#include <qmvQTC.h>
#include <qsvEnv.h>
#include <qsvProcVar.h>
#include <qsvProcStmts.h>
#include <qcuSqlSourceInfo.h>
#include <qcuProperty.h>
#include <qdpPrivilege.h>
#include <qdv.h>
#include <qmoStatMgr.h>
#include <qcmSynonym.h>
#include <qcmDump.h>
#include <qcmPartition.h> // PROJ-1502 PARTITIONED DISK TABLE
#include <qcgPlan.h>
#include <qcsModule.h>
#include <qmvGBGSTransform.h> // PROJ-2415 Grouping Sets Clause
#include <qmvPivotTransform.h>
#include <qmvHierTransform.h>
#include <qmvTableFuncTransform.h>
#include <qmsIndexTable.h>
#include <qmsDefaultExpr.h>
#include <qmsPreservedTable.h>
#include <qmr.h>
#include <qsvPkgStmts.h>
#include <qmvAvgTransform.h> /* PROJ-2361 */
#include <qdpRole.h>
#include <qmvUnpivotTransform.h> /* PROJ-2523 */
#include <qmvShardTransform.h>
#include <sdi.h>

extern mtfModule mtfAnd;
extern mtdModule mtdBinary;
extern mtdModule mtdVarchar;

extern mtfModule mtfGetBlobLocator;
extern mtfModule mtfGetClobLocator;
extern mtfModule mtfDecrypt;
extern mtfModule mtfCast;
extern mtfModule mtfList;

extern mtfModule mtfDecodeOffset;

#define QMV_QUERYSET_PARALLEL_DEGREE_LIMIT (1024)

IDE_RC qmvQuerySet::validate(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aOuterQuerySFWGH,
    qmsFrom         * aOuterQueryFrom,
    SInt              aDepth)
{
    qmsParseTree    * sParseTree;
    qmsTarget       * sLeftTarget;
    qmsTarget       * sRightTarget;
    qmsTarget       * sPrevTarget = NULL;
    qmsTarget       * sCurrTarget;
    mtcColumn       * sMtcColumn;
    UShort            sTable;
    UShort            sColumn;
    mtcNode         * sLeftNode;
    mtcNode         * sRightNode;
    qcuSqlSourceInfo  sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validate::__FT__" );

    IDE_TEST_RAISE(aDepth >= QCU_PROPERTY(mMaxSetOpRecursionDepth),
                   ERR_REACHED_MAX_RECURSION_DEPTH);

    IDE_DASSERT( aQuerySet != NULL );

    if (aQuerySet->setOp == QMS_NONE)
    {
        // SFWGH must be exist
        IDE_DASSERT( aQuerySet->SFWGH != NULL );
        
        // set Quter Query Pointer

        // PROJ-2418
        if ( aQuerySet->SFWGH->outerQuery != NULL )
        {
            // outerQuery가 이미 지정되어 있다면, Lateral View이다.
            // 이미 지정된 outerQuery를 재설정하지 않는다.

            // Nothing to do.
        }
        else
        {
            aQuerySet->SFWGH->outerQuery = aOuterQuerySFWGH;
        }

        // PROJ-2418
        if ( aQuerySet->SFWGH->outerFrom != NULL )
        {
            // outerFrom가 이미 지정되어 있다면, Lateral View이다.
            // 이미 지정된 outerFrom을 재설정하지 않는다.

            // Nothing to do.
        }
        else
        {
            aQuerySet->SFWGH->outerFrom = aOuterQueryFrom;
        }

        aQuerySet->SFWGH->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
        aQuerySet->SFWGH->lflag |= (aQuerySet->lflag&QMV_PERFORMANCE_VIEW_CREATION_MASK);
        aQuerySet->SFWGH->lflag &= ~(QMV_VIEW_CREATION_MASK);
        aQuerySet->SFWGH->lflag |= (aQuerySet->lflag&QMV_VIEW_CREATION_MASK);

        IDE_TEST( qmvQuerySet::validateQmsSFWGH( aStatement,
                                                 aQuerySet,
                                                 aQuerySet->SFWGH )
                  != IDE_SUCCESS );

        // set dependencies
        IDE_TEST( qtc::dependencyOr( & aQuerySet->SFWGH->depInfo,
                                     & aQuerySet->depInfo,
                                     & aQuerySet->depInfo )
                  != IDE_SUCCESS );

        // PROJ-1413
        // set outer column dependencies
        // PROJ-2415 Grouping Sets Cluase
        // setOuterDependencies() -> addOuterDependencies()
        IDE_TEST( qmvQTC::addOuterDependencies( aQuerySet->SFWGH,
                                                & aQuerySet->SFWGH->outerDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & aQuerySet->SFWGH->outerDepInfo,
                                     & aQuerySet->outerDepInfo,
                                     & aQuerySet->outerDepInfo )
                  != IDE_SUCCESS );

        // PROJ-2418
        // lateral View의 dependencies (lateralDepInfo) 설정
        IDE_TEST( qmvQTC::setLateralDependencies( aQuerySet->SFWGH,
                                                  & aQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );

        // set target
        aQuerySet->target = aQuerySet->SFWGH->target;

        aQuerySet->materializeType = aQuerySet->SFWGH->hints->materializeType;
    }
    else // UNION, UNION ALL, INTERSECT, MINUS
    {
        // FOR UPDATE clause
        sParseTree = (qmsParseTree *) aStatement->myPlan->parseTree;
        IDE_TEST_RAISE(sParseTree->forUpdate != NULL,
                       ERR_DISTINCT_SETFUNCTION_ON_FORUPDATE);

        aQuerySet->left->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
        aQuerySet->left->lflag |= (aQuerySet->lflag&QMV_PERFORMANCE_VIEW_CREATION_MASK);
        aQuerySet->right->lflag &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
        aQuerySet->right->lflag |= (aQuerySet->lflag&QMV_PERFORMANCE_VIEW_CREATION_MASK);
        aQuerySet->left->lflag &= ~(QMV_VIEW_CREATION_MASK);
        aQuerySet->left->lflag |= (aQuerySet->lflag&QMV_VIEW_CREATION_MASK);
        aQuerySet->right->lflag &= ~(QMV_VIEW_CREATION_MASK);
        aQuerySet->right->lflag |= (aQuerySet->lflag&QMV_VIEW_CREATION_MASK);

        // PROJ-2582 recursive with
        if ( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
             == QMV_QUERYSET_RECURSIVE_VIEW_TOP )
        {
            // recursive query 구분을 위해 right querySet flag설정
            aQuerySet->right->lflag &= ~QMV_QUERYSET_RECURSIVE_VIEW_MASK;
            aQuerySet->right->lflag |= QMV_QUERYSET_RECURSIVE_VIEW_RIGHT;
            aQuerySet->right->lflag &= ~QMV_QUERYSET_RESULT_CACHE_MASK;
            aQuerySet->right->lflag |= QMV_QUERYSET_RESULT_CACHE_NO;

            aQuerySet->left->lflag &= ~QMV_QUERYSET_RECURSIVE_VIEW_MASK;
            aQuerySet->left->lflag |= QMV_QUERYSET_RECURSIVE_VIEW_LEFT;
            aQuerySet->left->lflag &= ~QMV_QUERYSET_RESULT_CACHE_MASK;
            aQuerySet->left->lflag |= QMV_QUERYSET_RESULT_CACHE_NO;
        }
        else
        {
            // nothing to do
        }
        
        IDE_TEST(qmvQuerySet::validate(aStatement,
                                       aQuerySet->left,
                                       aOuterQuerySFWGH,
                                       aOuterQueryFrom,
                                       aDepth + 1) != IDE_SUCCESS);

        IDE_TEST(qmvQuerySet::validate(aStatement,
                                       aQuerySet->right,
                                       aOuterQuerySFWGH,
                                       aOuterQueryFrom,
                                       aDepth + 1) != IDE_SUCCESS);

        if( ( aQuerySet->left->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
            ||
            ( aQuerySet->right->materializeType == QMO_MATERIALIZE_TYPE_VALUE ) )
        {
            aQuerySet->materializeType = QMO_MATERIALIZE_TYPE_VALUE;
        }
        else
        {
            aQuerySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
        }

        if (aQuerySet->right->setOp == QMS_NONE &&
            aQuerySet->right->SFWGH->intoVariables != NULL)
        {
            IDE_DASSERT( aQuerySet->right->SFWGH->intoVariables->intoNodes != NULL );
            
            sqlInfo.setSourceInfo(
                aStatement,
                & aQuerySet->right->SFWGH->intoVariables->intoNodes->position );
            IDE_RAISE( ERR_INAPPROPRIATE_INTO );
        }

        // for optimizer
        if (aQuerySet->setOp == QMS_UNION)
        {
            if (aQuerySet->left->setOp == QMS_UNION)
            {
                aQuerySet->left->setOp = QMS_UNION_ALL;
            }

            if (aQuerySet->right->setOp == QMS_UNION)
            {
                aQuerySet->right->setOp = QMS_UNION_ALL;
            }
        }

        /* match count of seleted items */
        sLeftTarget  = aQuerySet->left->target;
        sRightTarget = aQuerySet->right->target;

        // add tuple set
        IDE_TEST(qtc::nextTable( &(sTable), aStatement, NULL, ID_TRUE,
                                 MTC_COLUMN_NOTNULL_TRUE ) != IDE_SUCCESS); // PR-13597

        sColumn = 0;
        aQuerySet->target = NULL;
        while(sLeftTarget != NULL && sRightTarget != NULL)
        {
            if (((sLeftTarget->targetColumn->lflag & QTC_NODE_COLUMN_RID_MASK) ==
                 QTC_NODE_COLUMN_RID_EXIST) ||
                ((sRightTarget->targetColumn->lflag & QTC_NODE_COLUMN_RID_MASK) ==
                 QTC_NODE_COLUMN_RID_EXIST))
            {
                IDE_RAISE( ERR_PROWID_NOT_SUPPORTED );
            }
            
            // 리스트 타입의 사용 여부 확인
            sMtcColumn = QTC_STMT_COLUMN( aStatement, sLeftTarget->targetColumn );
            if ( sMtcColumn->module->id == MTD_LIST_ID )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sLeftTarget->targetColumn->position );
                IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
            }
            else
            {
                // Nothing to do.
            }
            
            sMtcColumn = QTC_STMT_COLUMN( aStatement, sRightTarget->targetColumn );
            if ( sMtcColumn->module->id == MTD_LIST_ID )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sRightTarget->targetColumn->position );
                IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
            }
            else
            {
                // Nothing to do.
            }
            
            // BUG-16000
            // compare연산이 불가능한 타입은 에러.
            if (aQuerySet->setOp != QMS_UNION_ALL)
            {
                // UNION, INTERSECT, MINUS
                // left
                if ( qtc::isEquiValidType( sLeftTarget->targetColumn,
                                           & QC_SHARED_TMPLATE(aStatement)->tmplate )
                     == ID_FALSE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sLeftTarget->targetColumn->position );
                    IDE_RAISE( ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_SET );
                }
                else
                {
                    // Nothing to do.
                }

                // right
                if ( qtc::isEquiValidType( sRightTarget->targetColumn,
                                           & QC_SHARED_TMPLATE(aStatement)->tmplate )
                     == ID_FALSE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sRightTarget->targetColumn->position );
                    IDE_RAISE( ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_SET );
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

            // PROJ-2582 recursive with
            if ( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                 == QMV_QUERYSET_RECURSIVE_VIEW_TOP )
            {
                // recursive view인 경우 target type은 left의 type, precision으로 맞춘다.
                IDE_TEST(
                    qtc::makeLeftDataType4TwoNode( aStatement,
                                                   sLeftTarget->targetColumn,
                                                   sRightTarget->targetColumn )
                    != IDE_SUCCESS );
            }
            else
            {   
                // Query Set의 양쪽 Target을 동일한 Data Type으로 생성함.
                IDE_TEST(
                    qtc::makeSameDataType4TwoNode( aStatement,
                                                   sLeftTarget->targetColumn,
                                                   sRightTarget->targetColumn )
                    != IDE_SUCCESS );
            }

            // make new target for SET
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsTarget, &sCurrTarget)
                     != IDE_SUCCESS);

            sCurrTarget->userName         = sLeftTarget->userName;
            sCurrTarget->tableName        = sLeftTarget->tableName;
            sCurrTarget->aliasTableName   = sLeftTarget->aliasTableName;
            sCurrTarget->columnName       = sLeftTarget->columnName;
            sCurrTarget->aliasColumnName  = sLeftTarget->aliasColumnName;
            sCurrTarget->displayName      = sLeftTarget->displayName;
            sCurrTarget->flag             = sLeftTarget->flag;
            sCurrTarget->flag            &= ~QMS_TARGET_IS_NULLABLE_MASK;
            sCurrTarget->flag            |= QMS_TARGET_IS_NULLABLE_TRUE;
            sCurrTarget->next             = NULL;

            // set member of qtcNode
            IDE_TEST(qtc::makeInternalColumn(
                         aStatement,
                         sTable,
                         sColumn,
                         &(sCurrTarget->targetColumn))
                     != IDE_SUCCESS);
            
            // PROJ-2415 Grouping Sets Clause
            // left와 right Node의 flag를 Set의 Target Node로 전달한다.
            if ( ( ( sLeftTarget->targetColumn->lflag & QTC_NODE_GBGS_ORDER_BY_NODE_MASK )
                   == QTC_NODE_GBGS_ORDER_BY_NODE_TRUE ) ||
                 ( ( sRightTarget->targetColumn->lflag & QTC_NODE_GBGS_ORDER_BY_NODE_MASK )
                   == QTC_NODE_GBGS_ORDER_BY_NODE_TRUE ) )
            {
                sCurrTarget->targetColumn->lflag &= ~QTC_NODE_GBGS_ORDER_BY_NODE_MASK;
                sCurrTarget->targetColumn->lflag |= QTC_NODE_GBGS_ORDER_BY_NODE_TRUE;
            }
            else
            {
                /* Nothing to do */
            }

            if (aQuerySet->target == NULL)
            {
                aQuerySet->target = sCurrTarget;
                sPrevTarget       = sCurrTarget;
            }
            else
            {
                sPrevTarget->next = sCurrTarget;
                sPrevTarget->targetColumn->node.next =
                    (mtcNode *)(sCurrTarget->targetColumn);

                sPrevTarget       = sCurrTarget;
            }

            // next
            sColumn++;
            sLeftTarget  = sLeftTarget->next;
            sRightTarget = sRightTarget->next;
        }

        IDE_TEST_RAISE( aQuerySet->target == NULL, ERR_EMPTY_TARGET );

        IDE_TEST_RAISE( (sLeftTarget != NULL && sRightTarget == NULL) ||
                        (sLeftTarget == NULL && sRightTarget != NULL),
                        ERR_NOT_MATCH_TARGET_COUNT_IN_QUERYSET);

        IDE_TEST(qtc::allocIntermediateTuple( aStatement,
                                              & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                              sTable,
                                              sColumn )
                 != IDE_SUCCESS);

        // set mtcColumn
        for( sColumn = 0,
                 sCurrTarget = aQuerySet->target,
                 sLeftTarget = aQuerySet->left->target,
                 sRightTarget = aQuerySet->right->target;
             sColumn < QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable].columnCount;
             sColumn++,
                 sCurrTarget = sCurrTarget->next,
                 sLeftTarget = sLeftTarget->next,
                 sRightTarget = sRightTarget->next )
        {
            sLeftNode = mtf::convertedNode(
                (mtcNode *)sLeftTarget->targetColumn,
                &QC_SHARED_TMPLATE(aStatement)->tmplate);
            sRightNode = mtf::convertedNode(
                (mtcNode *)sRightTarget->targetColumn,
                &QC_SHARED_TMPLATE(aStatement)->tmplate);      
            
            // To Fix Case-2175
            // Left와 Right중 보다 Size가 큰 Column의 정보를
            // View의 Column 정보로 사용한다.
            if ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sLeftNode->table].
                 columns[sLeftNode->column].column.size
                 >=
                 QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sRightNode->table].
                 columns[sRightNode->column].column.size )
            {
                // Left쪽의 Column 정보를 복사
                // copy size, type, module
                mtc::copyColumn(
                    &( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable]
                       .columns[sColumn]),
                    &( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sLeftNode->table]
                       .columns[sLeftNode->column]) );
            }
            else
            {
                // Right쪽의 Column 정보를 복사
                // copy size, type, module
                mtc::copyColumn(
                    &( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable]
                       .columns[sColumn]),
                    &( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sRightNode->table]
                       .columns[sRightNode->column]) );
            }

            // set execute
            IDE_TEST( qtc::estimateNodeWithoutArgument(
                          aStatement, sCurrTarget->targetColumn )
                      != IDE_SUCCESS );
        }

        // set offset
        qtc::resetTupleOffset( & QC_SHARED_TMPLATE(aStatement)->tmplate, sTable );

        // nothing to do for tuple
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable].lflag
            &= ~MTC_TUPLE_ROW_SKIP_MASK;
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable].lflag
            |= MTC_TUPLE_ROW_SKIP_TRUE;

        // SET에 의해 생성된 암시적 View임을 표시함
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable].lflag &= ~MTC_TUPLE_VIEW_MASK;
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable].lflag |= MTC_TUPLE_VIEW_TRUE;

        // set dependencies
        qtc::dependencyClear( & aQuerySet->depInfo );

        //     (1) view dependency
        qtc::dependencySet( sTable, & aQuerySet->depInfo );

        // PROJ-1358
        // SET 은 하위의 Dependency를 OR-ing 하지 않는다.
        //     (2) left->dependencies
        // qtc::dependencyOr(aQuerySet->left->dependencies,
        //                   aQuerySet->dependencies,
        //                   aQuerySet->dependencies);
        //     (3) right->dependencies
        // qtc::dependencyOr(aQuerySet->right->dependencies,
        //                   aQuerySet->dependencies,
        //                  aQuerySet->dependencies);

        // set outer column dependencies

        // PROJ-1413
        // outer column dependency는 하위 dependency를 OR-ing한다.
        //     (1) left->dependencies
        IDE_TEST( qtc::dependencyOr( & aQuerySet->left->outerDepInfo,
                                     & aQuerySet->outerDepInfo,
                                     & aQuerySet->outerDepInfo )
                  != IDE_SUCCESS );
        //     (2) right->dependencies
        IDE_TEST( qtc::dependencyOr( & aQuerySet->right->outerDepInfo,
                                     & aQuerySet->outerDepInfo,
                                     & aQuerySet->outerDepInfo )
                  != IDE_SUCCESS );

        // PROJ-2418
        // lateral view dependency는 하위 dependency를 OR-ing한다.
        IDE_TEST( qtc::dependencyOr( & aQuerySet->left->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & aQuerySet->right->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_MATCH_TARGET_COUNT_IN_QUERYSET)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_MISMATCH_TARGET_COUNT));
    }
    IDE_EXCEPTION(ERR_DISTINCT_SETFUNCTION_ON_FORUPDATE)
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QMV_DISTINCT_SETFUNCTION_ON_FORUPDATE));
    }
    IDE_EXCEPTION(ERR_INAPPROPRIATE_INTO);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_INAPPROPRIATE_INTO_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_SET);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_SET,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_EMPTY_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvQuerySet::validate",
                                  "Target is empty" ));
    }
    IDE_EXCEPTION(ERR_REACHED_MAX_RECURSION_DEPTH)
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_REACHED_MAX_SET_OP_RECURSION_DEPTH));
    }
    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateQmsSFWGH(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aSFWGH)
{
    qmsParseTree     * sParseTree = NULL;
    qmsFrom          * sFrom = NULL;
    qmsTarget        * sTarget = NULL;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateQmsSFWGH::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSFWGH != NULL );

    //---------------------------------------------------
    // initialize
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        // BUG-21985 : replication에서 WHERE에 대한 validation 할때,
        //             querySet 정보가 없어 넘기지 않음
        aQuerySet->processPhase = QMS_VALIDATE_INIT;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_INIT;

    sParseTree = (qmsParseTree *) aStatement->myPlan->parseTree;

    IDE_TEST_RAISE( aSFWGH->selectType == QMS_DISTINCT &&
                    sParseTree->forUpdate != NULL,
                    ERR_DISTINCT_SETFUNCTION_ON_FORUPDATE);

    // codesonar::Null Pointer Dereference
    IDE_FT_ERROR( aQuerySet != NULL );

    // PROJ-2582 recursive with
    IDE_TEST_RAISE( ( aSFWGH->selectType == QMS_DISTINCT ) &&
                    ( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                      == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ),
                    ERR_DISTINCT_NOT_ALLOWED_RECURSIVE_VIEW );

    // PROJ-2204 Join Update, Delete
    // create view, insert view, update view, delete view의
    // 최상위 SFWGH만 key preserved table을 검사한다.
    if ( ( aSFWGH->lflag & QMV_SFWGH_UPDATABLE_VIEW_MASK )
         == QMV_SFWGH_UPDATABLE_VIEW_TRUE )
    {
        IDE_TEST( qmsPreservedTable::initialize( aStatement,
                                                 aSFWGH )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
              
    qtc::dependencyClear( & aSFWGH->depInfo );

    //-------------------------------------
    // 1473TODO 확인필요
    // PROJ-1473
    // 컬럼정보수집시 hint정보가 필요해서 초기화 작업을 앞으로 당김.
    //-------------------------------------

    if ( aSFWGH->hints == NULL )
    {
        // 사용자가 힌트를 명시하지 않은 경우 초기값으로 힌트 구조체 할당

        IDE_TEST ( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                 qmsHints,
                                 &(aSFWGH->hints) ) != IDE_SUCCESS);

        QCP_SET_INIT_HINTS(aSFWGH->hints);
    }
    else
    {
        // Nothing To Do 
    }

    // PROJ-1473
    // 질의에 사용된 컬럼정보를 얻기위해
    // processType hint에 대해서만 일부 선구축한다.
    // (예: on 절에 사용된 컬럼처리)
    
    if( aSFWGH->hints->materializeType == QMO_MATERIALIZE_TYPE_NOT_DEFINED )
    {
        if( QCU_OPTIMIZER_PUSH_PROJECTION == 1 )
        {
            aSFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_VALUE;
        }
        else
        {
            aSFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;
        }
    }
    else
    {
        // Nothing To Do
    }

    /* BUG-36580 supported TOP */
    if ( aSFWGH->top != NULL )
    {
        IDE_TEST( qmv::validateLimit( aStatement, aSFWGH->top )
                  != IDE_SUCCESS );
    }
    else
    {		  
        // Nothing To Do
    }
    
    //---------------------------------------------------
    // validation of FROM clause
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        aQuerySet->processPhase = QMS_VALIDATE_FROM;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_FROM;

    // fix BUG-14606
    IDE_TEST_RAISE( ( ( aSFWGH->from->next != NULL ) ||
                      ( aSFWGH->from->joinType != QMS_NO_JOIN ) )
                    && ( sParseTree->forUpdate != NULL ),
                    ERR_JOIN_ON_FORUPDATE);

    /* PROJ-1715 oneTable transform inlineview */
    if ( aSFWGH->hierarchy != NULL )
    {
        sFrom = aSFWGH->from;
        if ( ( sFrom->joinType == QMS_NO_JOIN  ) &&
             ( sFrom->next == NULL ) )
        {
            if ( sFrom->tableRef->view == NULL )
            {
                IDE_TEST( qmvHierTransform::doTransform( aStatement,
                                                         sFrom->tableRef )
                          != IDE_SUCCESS );
            }
            else
            {
                // PROJ-2638
                if ( ( sFrom->tableRef->view->myPlan->parseTree->stmtShard !=
                       QC_STMT_SHARD_NONE ) &&
                     ( sFrom->tableRef->view->myPlan->parseTree->stmtShard !=
                       QC_STMT_SHARD_META ) )
                {
                    IDE_TEST( qmvHierTransform::doTransform( aStatement,
                                                             sFrom->tableRef )
                              != IDE_SUCCESS );
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
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( convertAnsiInnerJoin( aStatement, aSFWGH )
              != IDE_SUCCESS );

    for (sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
    {
        if (sFrom->joinType != QMS_NO_JOIN) // INNER, OUTER JOIN
        {
            IDE_TEST(validateQmsFromWithOnCond( aQuerySet,
                                                aSFWGH,
                                                sFrom,
                                                aStatement,
                                                MTC_COLUMN_NOTNULL_TRUE) // PR-13597
                     != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(validateQmsTableRef( aStatement,
                                          aSFWGH,
                                          sFrom->tableRef,
                                          aSFWGH->lflag,
                                          MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                     != IDE_SUCCESS);

            // Table Map 설정
            QC_SHARED_TMPLATE(aStatement)->tableMap[sFrom->tableRef->table].from = sFrom;

            // FROM 절에 대한 dependencies 설정
            qtc::dependencyClear( & sFrom->depInfo );
            qtc::dependencySet( sFrom->tableRef->table, & sFrom->depInfo );
            
            // PROJ-1718 Semi/anti join과 관련된 dependency 초기화
            qtc::dependencyClear( & sFrom->semiAntiJoinDepInfo );

            IDE_TEST( qmsPreservedTable::addTable( aStatement,
                                                   aSFWGH,
                                                   sFrom->tableRef )
                      != IDE_SUCCESS );

            if( (sFrom->tableRef->tableInfo->tableType == QCM_FIXED_TABLE) ||
                (sFrom->tableRef->tableInfo->tableType == QCM_DUMP_TABLE) ||
                (sFrom->tableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW) ||
                ( sFrom->tableRef->remoteTable != NULL ) || /* PROJ-1832 */
                ( sFrom->tableRef->mShardObjInfo != NULL ) )
            {
                // for FT/PV, no privilege validation, only X$, V$ name validation
                /* for database link */

                /* PROJ-2462 Result Cache */
                if ( aQuerySet != NULL )
                {
                    aQuerySet->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
                    aQuerySet->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                //check grant
                if ( (sFrom->tableRef->view == NULL) ||
                     ((sFrom->tableRef->flag) & QMS_TABLE_REF_CREATED_VIEW_MASK)
                     == QMS_TABLE_REF_CREATED_VIEW_TRUE)
                {
                    IDE_TEST( qdpRole::checkDMLSelectTablePriv(
                                  aStatement,
                                  sFrom->tableRef->tableInfo->tableOwnerID,
                                  sFrom->tableRef->tableInfo->privilegeCount,
                                  sFrom->tableRef->tableInfo->privilegeInfo,
                                  ID_FALSE,
                                  NULL,
                                  NULL )
                              != IDE_SUCCESS );
                        
                    // environment의 기록
                    IDE_TEST( qcgPlan::registerPlanPrivTable(
                                  aStatement,
                                  QCM_PRIV_ID_OBJECT_SELECT_NO,
                                  sFrom->tableRef->tableInfo )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                /* PROJ-2462 Result Cache */
                if ( ( sFrom->tableRef->tableInfo->temporaryInfo.type != QCM_TEMPORARY_ON_COMMIT_NONE ) &&
                     ( aQuerySet != NULL ) )
                {
                    aQuerySet->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
                    aQuerySet->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }

    //---------------------------------------------------
    // validation of Hints
    //---------------------------------------------------

    IDE_TEST(validateHints(aStatement, aSFWGH) != IDE_SUCCESS);

    //---------------------------------------------------
    // validation of target
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        aQuerySet->processPhase = QMS_VALIDATE_TARGET;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_TARGET;

    // BUG-41311 table function
    if ( ( aSFWGH->lflag & QMV_SFWGH_TABLE_FUNCTION_VIEW_MASK )
         == QMV_SFWGH_TABLE_FUNCTION_VIEW_TRUE )
    {
        IDE_TEST( qmvTableFuncTransform::expandTarget( aStatement,
                                                       aSFWGH )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2188 Support pivot clause 
    if ( ( aSFWGH->lflag & QMV_SFWGH_PIVOT_MASK )
         == QMV_SFWGH_PIVOT_TRUE )
    {
        IDE_TEST(qmvPivotTransform::expandPivotDummy( aStatement,
                                                      aSFWGH )
                 != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2523 Unpivot clause
    // Unpivot target을 확장한다.
    if ( ( aSFWGH->lflag & QMV_SFWGH_UNPIVOT_MASK )
         == QMV_SFWGH_UNPIVOT_TRUE )
    {
        IDE_TEST( qmvUnpivotTransform::expandUnpivotTarget( aStatement,
                                                            aSFWGH )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(validateQmsTarget(aStatement, aQuerySet, aSFWGH) != IDE_SUCCESS);

    // right outer join을 left outer join으로 변환
    // (expand asterisk 때문에 validate target후에 위치한다.)
    for (sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
    {
        IDE_TEST(validateJoin(aStatement, sFrom, aSFWGH) != IDE_SUCCESS);
    }

    //---------------------------------------------------
    // validation of WHERE clause
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        aQuerySet->processPhase = QMS_VALIDATE_WHERE;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_WHERE;

    if (aSFWGH->where != NULL)
    {
        IDE_TEST(validateWhere(aStatement, aQuerySet, aSFWGH) != IDE_SUCCESS);
    }

    //---------------------------------------------------
    // validation of Hierarchical clause
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        aQuerySet->processPhase = QMS_VALIDATE_HIERARCHY;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_HIERARCHY;

    if (aSFWGH->hierarchy != NULL)
    {
        IDE_TEST(validateHierarchy(aStatement, aQuerySet, aSFWGH)
                 != IDE_SUCCESS);
    }

    //---------------------------------------------------
    // validation of GROUP clause
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        aQuerySet->processPhase = QMS_VALIDATE_GROUPBY;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_GROUPBY;

    if (aSFWGH->group != NULL)
    {
        IDE_TEST(validateGroupBy(aStatement, aQuerySet, aSFWGH) != IDE_SUCCESS);
    }

    //---------------------------------------------------
    // validation of HAVING clause
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        aQuerySet->processPhase = QMS_VALIDATE_HAVING;;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_HAVING;

    if (aSFWGH->having != NULL)
    {
        IDE_TEST(validateHaving(aStatement, aQuerySet, aSFWGH) != IDE_SUCCESS);
    }

    // PROJ-2415 Grouping Sets Clause
    if ( ( aSFWGH->group != NULL ) &&
         ( ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
             == QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) ||
           ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
             == QMV_SFWGH_GBGS_TRANSFORM_BOTTOM ) ) )
    {
        // PROJ-2415 Grouping Sets Clause
        IDE_TEST( qmvGBGSTransform::removeNullGroupElements( aSFWGH ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    //---------------------------------------------------
    // finalize
    //---------------------------------------------------

    if ( aQuerySet != NULL )
    {
        aQuerySet->processPhase = QMS_VALIDATE_FINAL;
    }
    else
    {
        // nothing to do 
    }
    aSFWGH->validatePhase = QMS_VALIDATE_FINAL;

    // Because aggsDepth1 and aggsDepth2 is made in estimate,
    // the following code is checked after validation all SFWGH estimate.
    if (aSFWGH->group == NULL)
    {
        // If a SELECT statement contains SET functions without aGROUP BY, or
        // If a SELECT statement contains HAVING without aGROUP BY,
        //     the select list can't include any references to columns
        //     unless those references are used with a set function.
        if ( ( aSFWGH->aggsDepth1 != NULL ) || ( aSFWGH->having != NULL ) )
        {
            for (sTarget = aSFWGH->target;
                 sTarget != NULL;
                 sTarget = sTarget->next)
            {
                IDE_TEST(qmvQTC::isAggregateExpression( aStatement,
                                                        aSFWGH,
                                                        sTarget->targetColumn)
                         != IDE_SUCCESS);
            }
        }
    }

    if (aSFWGH->aggsDepth2 != NULL)
    {
        IDE_TEST_RAISE(aSFWGH->group == NULL, ERR_NESTED_AGGR_WITHOUT_GROUP);

        if (aSFWGH->having != NULL)
        {
            IDE_TEST(qmvQTC::haveNotNestedAggregate( aStatement,
                                                     aSFWGH,
                                                     aSFWGH->having)
                     != IDE_SUCCESS);
        }
    }

    // check SELECT FOR UPDATE
    if ( aSFWGH->aggsDepth1 != NULL )
    {
        IDE_TEST_RAISE(sParseTree->forUpdate != NULL,
                       ERR_AGGREGATE_ON_FORUPDATE);

        // PROJ-2582 recursive with
        IDE_TEST_RAISE( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                        == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT,
                        ERR_AGGREGATE_NOT_ALLOWED_RECURSIVE_VIEW );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2361 */
    if ( ( aSFWGH->aggsDepth1 != NULL ) && ( aSFWGH->aggsDepth2 == NULL ) )
    {
        IDE_TEST( qmvAvgTransform::doTransform( aStatement,
                                                aQuerySet,
                                                aSFWGH )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2204 Join Update, Delete
    IDE_TEST( qmsPreservedTable::find( aStatement,
                                       aSFWGH )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_AGGREGATE_ON_FORUPDATE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_AGGREGATE_ON_FORUPDATE));
    }
    IDE_EXCEPTION(ERR_NESTED_AGGR_WITHOUT_GROUP)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NESTED_AGGR_WITHOUT_GROUP));
    }
    IDE_EXCEPTION(ERR_DISTINCT_SETFUNCTION_ON_FORUPDATE)
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QMV_DISTINCT_SETFUNCTION_ON_FORUPDATE));
    }
    IDE_EXCEPTION(ERR_JOIN_ON_FORUPDATE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_JOIN_ON_FORUPDATE));
    }
    IDE_EXCEPTION( ERR_DISTINCT_NOT_ALLOWED_RECURSIVE_VIEW )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMV_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW,
                                 "DISTINCT") );
    }
    IDE_EXCEPTION( ERR_AGGREGATE_NOT_ALLOWED_RECURSIVE_VIEW )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMV_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW,
                                 "aggregate Function") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateGroupOneColumn( qcStatement     * aStatement,
                                            qmsQuerySet     * aQuerySet,
                                            qmsSFWGH        * aSFWGH,
                                            qtcNode         * aNode )
{
    mtcColumn         * sColumn;
    qcuSqlSourceInfo    sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateGroupOneColumn::__FT__" );

    /* PROJ-2197 PSM Renewal */
    aNode->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
    IDE_TEST(qtc::estimate(
                 aNode,
                 QC_SHARED_TMPLATE(aStatement),
                 aStatement,
                 aQuerySet,
                 aSFWGH,
                 NULL )
             != IDE_SUCCESS);

    if ( ( aNode->node.lflag & MTC_NODE_DML_MASK )
         == MTC_NODE_DML_UNUSABLE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_USE_CURSOR_ATTR );
    }

    if ( QTC_HAVE_AGGREGATE( aNode ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
    }

    // PROJ-1492
    // BUG-42041
    if ( ( MTC_NODE_IS_DEFINED_TYPE( & aNode->node ) == ID_FALSE ) &&
         ( aStatement->calledByPSMFlag == ID_FALSE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_ALLOW_HOSTVAR );
    }

    // subquery not allowed here.
    if ( ( aNode->lflag & QTC_NODE_SUBQUERY_MASK )
         == QTC_NODE_SUBQUERY_EXIST )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_ALLOW_SUBQUERY );
    }

    // BUG-14094
    if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_LIST )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_GROUP_BY );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-14094
    if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_GROUP_BY );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-14094
    if ( ( aNode->node.lflag & MTC_NODE_COMPARISON_MASK )
         == MTC_NODE_COMPARISON_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_GROUP_BY );
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1362
    if ( ( aNode->lflag & QTC_NODE_BINARY_MASK )
         == QTC_NODE_BINARY_EXIST )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-15896
    // BUG-24133
    sColumn = QTC_TMPL_COLUMN( QC_SHARED_TMPLATE(aStatement),
                               aNode );
                
    if ( ( sColumn->module->id == MTD_BOOLEAN_ID ) ||
         ( sColumn->module->id == MTD_ROWTYPE_ID ) ||
         ( sColumn->module->id == MTD_RECORDTYPE_ID ) ||
         ( sColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) || 
         ( sColumn->module->id == MTD_REF_CURSOR_ID ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aNode->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_GROUP_BY );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_ALLOWED_AGGREGATION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_AGGREGATION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_HOSTVAR );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOWED_HOSTVAR,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_SUBQUERY );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_NOT_ALLOWED_SUBQUERY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE_TYPE_IN_GROUP_BY );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_GROUP_BY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmvQuerySet::validateHints( qcStatement* aStatement,
                                   qmsSFWGH   * aSFWGH)
{
/***********************************************************************
 *
 * Description :
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree            * sParseTree;
    qmsViewOptHints         * sViewOpt;
    qmsJoinMethodHints      * sJoin;
    qmsLeadingHints         * sLeading;
    qmsTableAccessHints     * sAccess;
    qmsTableAccessHints     * sCurAccess = NULL; // To Fix BUG-10465, 10449
    qmsHintTables           * sHintTable;
    qmsHintIndices          * sHintIndex;
    qmsPushPredHints        * sPushPredHint;
    qmsPushPredHints        * sValidPushPredHint = NULL;
    qmsPushPredHints        * sLastPushPredHint  = NULL;
    qmsNoMergeHints         * sNoMergeHint;
    qmsFrom                 * sFrom;
    qcmTableInfo            * sTableInfo;
    qcmIndex                * sIndex;
    UInt                      i;
    idBool                    sIsValid;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateHints::__FT__" );

    IDE_FT_ASSERT( aSFWGH != NULL );

    if ( aSFWGH->hints == NULL )
    {
        // 사용자가 힌트를 명시하지 않은 경우 초기값으로 힌트 구조체 할당

        IDE_TEST ( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                 qmsHints,
                                 &(aSFWGH->hints) ) != IDE_SUCCESS);

        QCP_SET_INIT_HINTS(aSFWGH->hints);
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-2638
    // shard 의 경우 별도의 hint 가 없는한 memory temp 를 사용한다.
    if ( aSFWGH->hints->interResultType == QMO_INTER_RESULT_TYPE_NOT_DEFINED )
    {
        for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
        {
            if ( sFrom->tableRef != NULL )
            {
                if ( sFrom->tableRef->view != NULL )
                {
                    if ( ( sFrom->tableRef->view->myPlan->parseTree->stmtShard !=
                           QC_STMT_SHARD_NONE ) &&
                         ( sFrom->tableRef->view->myPlan->parseTree->stmtShard !=
                           QC_STMT_SHARD_META ) )
                    {
                        aSFWGH->hints->interResultType = QMO_INTER_RESULT_TYPE_MEMORY;
                        break;
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
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    /*
     *  PROJ-2206 viewMaterialize
     *  하위 from에 view가 존재 하고 view에 MATERIALIZE, NO_MATERIALIZE 힌트가
     *  적용되었는지 판단해서 QMO_VIEW_OPT_TYPE_FORCE_VMTR,
     *  QMO_VIEW_OPT_TYPE_PUSH 를 적용해 준다.
     */
    for (sFrom = aSFWGH->from;
         sFrom != NULL;
         sFrom = sFrom->next)
    {
        if ( sFrom->tableRef != NULL )
        {
            if ( sFrom->tableRef->view != NULL )
            {
                // PROJ-2638
                // shard 의 경우 view materialize hint 를 배제한다.
                if ( ( sFrom->tableRef->view->myPlan->parseTree->stmtShard
                       == QC_STMT_SHARD_NONE ) ||
                     ( sFrom->tableRef->view->myPlan->parseTree->stmtShard
                       == QC_STMT_SHARD_META ) )
                {
                    sParseTree = (qmsParseTree *)(sFrom->tableRef->view->myPlan->parseTree);
                    IDE_FT_ASSERT( sParseTree != NULL );

                    if ( sParseTree->querySet != NULL )
                    {
                        if ( sParseTree->querySet->SFWGH != NULL )
                        {
                            if ( sParseTree->querySet->SFWGH->hints != NULL )
                            {
                                switch ( sParseTree->querySet->SFWGH->hints->viewOptMtrType )
                                {
                                    case QMO_VIEW_OPT_MATERIALIZE:
                                        sFrom->tableRef->viewOptType = QMO_VIEW_OPT_TYPE_FORCE_VMTR;
                                        sFrom->tableRef->noMergeHint = ID_TRUE;
                                        break;

                                    case QMO_VIEW_OPT_NO_MATERIALIZE:
                                        sFrom->tableRef->viewOptType = QMO_VIEW_OPT_TYPE_PUSH;
                                        sFrom->tableRef->noMergeHint = ID_TRUE;
                                        break;

                                    default:
                                        break;
                                }
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
    }

    // validation of VIEW OPT
    for (sViewOpt = aSFWGH->hints->viewOptHint;
         sViewOpt != NULL;
         sViewOpt = sViewOpt->next)
    {
        sHintTable = sViewOpt->table;

        sHintTable->table = NULL;
        
        // search table
        for (sFrom = aSFWGH->from;
             (sFrom != NULL) && (sHintTable->table == NULL);
             sFrom = sFrom->next)
        {
            findHintTableInFromClause(aStatement, sFrom, sHintTable);

            // PROJ-2638
            // shard 의 경우 view optimize hint 를 배제한다.
            if ( sHintTable->table != NULL )
            {
                if ( sHintTable->table->tableRef->view != NULL )
                {
                    if ( ( sHintTable->table->tableRef->view->myPlan->parseTree->stmtShard
                           != QC_STMT_SHARD_NONE ) &&
                         ( sHintTable->table->tableRef->view->myPlan->parseTree->stmtShard
                           != QC_STMT_SHARD_META ) )
                    {
                        sHintTable->table = NULL;
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

            // Table is found.
            if ( sHintTable->table != NULL )
            {
                sHintTable->table->tableRef->viewOptType = sViewOpt->viewOptType;

                if( sHintTable->table->tableRef->view != NULL )
                {
                    // PROJ-1413
                    // hint에서 지정된 view는 merge하지 않음
                    sHintTable->table->tableRef->noMergeHint = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }

    // PROJ-1495
    // validation of PUSH_PRED hint

    for( sPushPredHint = aSFWGH->hints->pushPredHint;
         sPushPredHint != NULL;
         sPushPredHint = sPushPredHint->next )
    {
        sHintTable = sPushPredHint->table;

        sHintTable->table = NULL;
        
        // 1. search table
        for( sFrom = aSFWGH->from;
             ( sFrom != NULL ) && ( sHintTable->table == NULL );
             sFrom = sFrom->next )
        {
            findHintTableInFromClause(aStatement, sFrom, sHintTable);

            if( sHintTable->table != NULL )
            {
                if( sHintTable->table->tableRef->view != NULL )
                {
                    // 해당 view table을 찾음

                    // PROJ-1413
                    // hint에서 지정된 view는 merge하지 않음
                    sHintTable->table->tableRef->noMergeHint = ID_TRUE;

                    break;
                }
                else
                {
                    // view가 아닌 base table로 잘못된 hint
                    sHintTable->table = NULL;
                }
            }
            else
            {
                // 해당 view table이 없음의 잘못된 hint
                // Nothing To Do
            }
        }

        // 2. PUSH_PRED hint view의 적합성 검사
        if( sHintTable->table != NULL )
        {
            sIsValid = ID_TRUE;

            IDE_TEST(
                validatePushPredView( aStatement,
                                      sHintTable->table->tableRef,
                                      & sIsValid ) != IDE_SUCCESS );

            if( sIsValid == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                // VIEW가 hint를 적용하기에 부적합
                sHintTable->table = NULL;
            }
        }
        else
        {
            // Nothing To Do
        }

        if( sHintTable->table != NULL )
        {
            if( sValidPushPredHint == NULL )
            {
                sValidPushPredHint = sPushPredHint;
                sLastPushPredHint  = sValidPushPredHint;
            }
            else
            {
                sLastPushPredHint->next = sPushPredHint;
                sLastPushPredHint = sLastPushPredHint->next;
            }
        }
        else
        {
            // Nothing To Do
        }
    }

    if( sValidPushPredHint != NULL )
    {
        sLastPushPredHint->next = NULL;
    }
    else
    {
        // Nothing To Do
    }
    
    // PROJ-2582 recursive with
    // recursive view가 있는 right subquery인 경우 push pred hint는 무시한다.
    if ( aSFWGH->recursiveViewID != ID_USHORT_MAX )
    {
        aSFWGH->hints->pushPredHint = NULL;
    }
    else
    {
        aSFWGH->hints->pushPredHint = sValidPushPredHint;
    }
    
    // PROJ-2582 recursive with    
    // recursive view가 있는 right subquery인 경우 ordered hint는 무시한다.
    if ( ( aSFWGH->recursiveViewID != ID_USHORT_MAX ) &&
         ( aSFWGH->hints->joinOrderType == QMO_JOIN_ORDER_TYPE_ORDERED ) )
    {
        aSFWGH->hints->joinOrderType = QMO_JOIN_ORDER_TYPE_OPTIMIZED;
    }
    else
    {
        // Nothing to do.
    }
    
    // validation of JOIN METHOD
    for (sJoin = aSFWGH->hints->joinMethod;
         sJoin != NULL;
         sJoin = sJoin->next)
    {
        for (sHintTable = sJoin->joinTables;
             sHintTable != NULL;
             sHintTable = sHintTable->next)
        {
            sHintTable->table = NULL;
            
            // search table
            for (sFrom = aSFWGH->from;
                 (sFrom != NULL) && (sHintTable->table == NULL);
                 sFrom = sFrom->next)
            {
                findHintTableInFromClause(aStatement, sFrom, sHintTable);
            }

            // To Fix PR-8045
            // Join Method에 dependencies를 설정해야 함
            if ( sHintTable->table != NULL )
            {
                IDE_TEST( qtc::dependencyOr( & sJoin->depInfo,
                                             & sHintTable->table->depInfo,
                                             & sJoin->depInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                // 잘못된 Hint임
                // Nothing To Do
            }
        }
    }

    // BUG-42447 leading hint support
    // validation leading hints
    sLeading = aSFWGH->hints->leading;
    if ( sLeading != NULL )
    {
        for ( sHintTable = sLeading->mLeadingTables;
              sHintTable != NULL;
              sHintTable = sHintTable->next )
        {
            sHintTable->table = NULL;

            // search table
            for ( sFrom = aSFWGH->from;
                  (sFrom != NULL) && (sHintTable->table == NULL);
                  sFrom = sFrom->next )
            {
                findHintTableInFromClause(aStatement, sFrom, sHintTable);
            }

            // BUG-47410 leading 힌트에 lateral view가 있는 경우 무시한다.
            if ( sHintTable->table != NULL )
            {
                if ( (sHintTable->table->tableRef->flag & QMS_TABLE_REF_LATERAL_VIEW_MASK) ==
                     QMS_TABLE_REF_LATERAL_VIEW_TRUE )
                {
                    aSFWGH->hints->leading = NULL;
                    aSFWGH->hints->joinOrderType = QMO_JOIN_ORDER_TYPE_OPTIMIZED; 
                    break;
                }
            }
        }
    }

    // validation of TABLE ACCESS
    // To Fix BUG-9525
    sAccess = aSFWGH->hints->tableAccess;
    while ( sAccess != NULL )
    {
        sHintTable = sAccess->table;

        if (sHintTable != NULL)
        {
            sHintTable->table = NULL;
            
            // search table
            for (sFrom = aSFWGH->from;
                 (sFrom != NULL) && (sHintTable->table == NULL);
                 sFrom = sFrom->next)
            {
                findHintTableInFromClause(aStatement, sFrom, sHintTable);

                if( sHintTable->table != NULL )
                {
                    if( sHintTable->table->tableRef->view != NULL )
                    {
                        // PROJ-1413
                        // hint에서 지정된 view는 merge하지 않음
                        sHintTable->table->tableRef->noMergeHint = ID_TRUE;
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

            if ( sHintTable->table != NULL )
            {
                // To Fix BUG-9525
                if ( sHintTable->table->tableRef->tableAccessHints == NULL )
                {
                    sHintTable->table->tableRef->tableAccessHints =
                        sCurAccess = sAccess;
                }
                else
                {
                    for ( sCurAccess =
                              sHintTable->table->tableRef->tableAccessHints;
                          sCurAccess->next != NULL;
                          sCurAccess = sCurAccess->next ) ;
                    
                    sCurAccess->next = sAccess;
                    sCurAccess = sAccess;
                }
                
                for (sHintIndex = sAccess->indices;
                     sHintIndex != NULL;
                     sHintIndex = sHintIndex->next)
                {
                    // search index
                    sTableInfo = sHintTable->table->tableRef->tableInfo;
                    
                    if (sTableInfo != NULL)
                    {
                        for (i = 0; i < sTableInfo->indexCount; i++)
                        {
                            sIndex = &(sTableInfo->indices[i]);
                            
                            if (idlOS::strMatch(
                                    sHintIndex->indexName.stmtText +
                                    sHintIndex->indexName.offset,
                                    sHintIndex->indexName.size,
                                    sIndex->name,
                                    idlOS::strlen(sIndex->name) ) == 0)
                            {
                                sHintIndex->indexPtr = sIndex;
                                break;
                            }
                        }
                    }
                }

                if ( sAccess->count < 1 )
                {
                    sAccess->count = 1;
                }
                else
                {
                    // Nothing to do.
                }

                if ( ( sAccess->id < 1 ) || ( sAccess->id > sAccess->count ) )
                {
                    sAccess->id = 1;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        else // parsing error
        {
            break;
        }

        // To Fix BUG-9525
        sAccess = sAccess->next;
        if ( sCurAccess != NULL )
        {
            sCurAccess->next = NULL;
        }
        else
        {
            // nothing to do
        }
    }

    /* PROJ-1071 Parallel Query */
    validateParallelHint(aStatement, aSFWGH);

    if (aSFWGH->hints->optGoalType == QMO_OPT_GOAL_TYPE_UNKNOWN)
    {
        if ( QCG_GET_SESSION_OPTIMIZER_MODE( aStatement ) == 0 ) // COST
        {
            aSFWGH->hints->optGoalType = QMO_OPT_GOAL_TYPE_ALL_ROWS;
        }
        else // RULE
        {
            aSFWGH->hints->optGoalType = QMO_OPT_GOAL_TYPE_RULE;
        }

        // environment의 기록
        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_MODE );
    }

    // PROJ-1473
    if( aSFWGH->hints->materializeType == QMO_MATERIALIZE_TYPE_NOT_DEFINED )
    {
        if( QCU_OPTIMIZER_PUSH_PROJECTION == 1 )
        {
            aSFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_VALUE;
        }
        else
        {
            aSFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;
        }
    }
    else
    {
        // Nothing To Do
    }

    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    if( aSFWGH->hints->interResultType == QMO_INTER_RESULT_TYPE_NOT_DEFINED )
    {
        if( QCG_GET_SESSION_OPTIMIZER_DEFAULT_TEMP_TBS_TYPE( aStatement )  == 1 )
        {
            aSFWGH->hints->interResultType = QMO_INTER_RESULT_TYPE_MEMORY;
        }
        else if( QCG_GET_SESSION_OPTIMIZER_DEFAULT_TEMP_TBS_TYPE( aStatement )  == 2 )
        {
            aSFWGH->hints->interResultType = QMO_INTER_RESULT_TYPE_DISK;
            
        }
        else
        {
            // Nothing To Do
        }

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_DEFAULT_TEMP_TBS_TYPE );
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-1413
    // validation of NO_MERGE hint
    for( sNoMergeHint = aSFWGH->hints->noMergeHint;
         sNoMergeHint != NULL;
         sNoMergeHint = sNoMergeHint->next )
    {
        sHintTable = sNoMergeHint->table;

        // BUG-43536 no_merge() 힌트 지원
        if ( sHintTable == NULL )
        {
            continue;
        }
        else
        {
            // nothing to do.
        }

        // no_merge( view ) hints
        sHintTable->table = NULL;

        for( sFrom = aSFWGH->from;
             ( sFrom != NULL ) && ( sHintTable->table == NULL );
             sFrom = sFrom->next )
        {
            findHintTableInFromClause(aStatement, sFrom, sHintTable);

            if( sHintTable->table != NULL )
            {
                if( sHintTable->table->tableRef->view != NULL )
                {
                    // hint에서 지정된 view는 merge하지 않음
                    sHintTable->table->tableRef->noMergeHint = ID_TRUE;
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
    }

    // PROJ-2462 Result Cache
    if ( aSFWGH->hints->topResultCache == ID_TRUE )
    {
        QC_SHARED_TMPLATE(aStatement)->resultCache.flag &= ~QC_RESULT_CACHE_TOP_MASK;
        QC_SHARED_TMPLATE(aStatement)->resultCache.flag |= QC_RESULT_CACHE_TOP_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2462 ResultCache */
    if ( aSFWGH->thisQuerySet != NULL )
    {
        // PROJ-2582 recursive with
        if ( ( ( aSFWGH->thisQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
               == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
             ( ( aSFWGH->thisQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
               == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) )
        {
            // nothing to do
        }
        else
        {
            aSFWGH->thisQuerySet->lflag &= ~QMV_QUERYSET_RESULT_CACHE_MASK;

            if ( aSFWGH->hints->resultCacheType == QMO_RESULT_CACHE_NO )
            {
                aSFWGH->thisQuerySet->lflag |= QMV_QUERYSET_RESULT_CACHE_NO;
            }
            else if ( aSFWGH->hints->resultCacheType == QMO_RESULT_CACHE )
            {
                aSFWGH->thisQuerySet->lflag |= QMV_QUERYSET_RESULT_CACHE;
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

    // PROJ-1436
    IDE_TEST( qmv::validatePlanHints( aStatement,
                                      aSFWGH->hints )
              != IDE_SUCCESS );

    //------------------------------------------------
    // 힌트에 대한 정보 구축이 완료되면, 이를 바탕으로
    // 일반 Table에 대한 통계 정보를 구축
    // qmsSFWGH->from을 따라가면서, 모든 일반  Table에 대한 통계 정보 구축
    // JOIN인 경우, 하위 일반 테이블도 모두 검사...
    // 일반 Table : qmsSFWGH->from->joinType = QMS_NO_JOIN 이고,
    //              qmsSFWGH->from->tableRef->view == NULL 인지를 검사.
    //------------------------------------------------

    IDE_TEST( qmoStat::getStatInfo4AllBaseTables( aStatement,
                                                  aSFWGH )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmvQuerySet::validateParallelHint(qcStatement* aStatement,
                                       qmsSFWGH   * aSFWGH)
{
    qmsParallelHints* sParallelHint;
    qmsHintTables   * sHintTable;
    qmsFrom         * sFrom;
    UInt              sDegree;

    IDE_DASSERT(aSFWGH != NULL);
    IDE_DASSERT(aSFWGH->hints != NULL);

    for (sParallelHint = aSFWGH->hints->parallelHint;
         sParallelHint != NULL;
         sParallelHint = sParallelHint->next)
    {
        if ((sParallelHint->parallelDegree == 0) ||
            (sParallelHint->parallelDegree > 65535))
        {
            /* invalid hint => ignore */
            continue;
        }
        else
        {
            /*
             * valid hint
             * adjust degree
             */
            sDegree = IDL_MIN(sParallelHint->parallelDegree,
                              QMV_QUERYSET_PARALLEL_DEGREE_LIMIT);
        }

        sHintTable = sParallelHint->table;

        if (sHintTable == NULL)
        {
            /*
             * parallel hint without table name
             * ex) PARALLEL(4)
             */
            setParallelDegreeOnAllTables(aSFWGH->from, sDegree);
        }
        else
        {
            sHintTable->table = NULL;

            for (sFrom = aSFWGH->from;
                 (sFrom != NULL) && (sHintTable->table == NULL);
                 sFrom = sFrom->next)
            {
                findHintTableInFromClause(aStatement, sFrom, sHintTable);

                if (sHintTable->table != NULL)
                {
                    /* found */
                    break;
                }
                else
                {
                    /* keep going */
                }
            }

            if (sHintTable->table != NULL)
            {
                /*
                 * BUG-39173
                 * USER_TABLE 에 대해서만 parallel 적용
                 */
                if ((sHintTable->table->tableRef->view == NULL) &&
                    (sHintTable->table->tableRef->tableType == QCM_USER_TABLE))
                {
                    /* view 가 아닌 base table 에 대해서만 hint 적용 */
                    sHintTable->table->tableRef->mParallelDegree = sDegree;
                }
                else
                {
                    /* View 는 parallel degree 에 영향받지 않는다. */
                }
            }
            else
            {
                /* No matching table */
            }
        }
    }

}

void qmvQuerySet::setParallelDegreeOnAllTables(qmsFrom* aFrom, UInt aDegree)
{
    qmsFrom* sFrom;

    for (sFrom = aFrom; sFrom != NULL; sFrom = sFrom->next)
    {
        if (sFrom->joinType == QMS_NO_JOIN)
        {
            /*
             * BUG-39173
             * USER_TABLE 에 대해서만 parallel 적용
             */
            if ((sFrom->tableRef->view == NULL) &&
                (sFrom->tableRef->tableType == QCM_USER_TABLE))
            {
                sFrom->tableRef->mParallelDegree = aDegree;
            }
            else
            {
                /* view 인 경우 parallel hint 무시 */
            }
        }
        else
        {
            /* ansi style join */
            setParallelDegreeOnAllTables(sFrom->left, aDegree);
            setParallelDegreeOnAllTables(sFrom->right, aDegree);
        }
    }

}

void qmvQuerySet::findHintTableInFromClause(
    qcStatement     * aStatement,
    qmsFrom         * aFrom,
    qmsHintTables   * aHintTable)
{
    IDE_RC            sRC;
    qmsFrom         * sTable;
    UInt              sUserID;

    if (aFrom->joinType != QMS_NO_JOIN)
    {
        if (aHintTable->table == NULL)
        {
            findHintTableInFromClause(aStatement, aFrom->left, aHintTable);
        }

        if (aHintTable->table == NULL)
        {
            findHintTableInFromClause(aStatement, aFrom->right, aHintTable);
        }
    }
    else
    {
        sTable = aFrom;

        if (QC_IS_NULL_NAME(aHintTable->userName) == ID_TRUE)
        {
            // search alias table name
            if (idlOS::strMatch(
                    aHintTable->tableName.stmtText + aHintTable->tableName.offset,
                    aHintTable->tableName.size,
                    sTable->tableRef->aliasName.stmtText + sTable->tableRef->aliasName.offset,
                    sTable->tableRef->aliasName.size) == 0 ||
                idlOS::strMatch(
                    aHintTable->tableName.stmtText + aHintTable->tableName.offset,
                    aHintTable->tableName.size,
                    sTable->tableRef->tableName.stmtText + sTable->tableRef->tableName.offset,
                    sTable->tableRef->tableName.size) == 0)
            {
                aHintTable->table = sTable;
            }
        }
        else
        {
            // search real table name
            sRC = qcmUser::getUserID(
                aStatement, aHintTable->userName, &(sUserID));

            if (sRC == IDE_SUCCESS && sTable->tableRef->userID == sUserID)
            {
                if (idlOS::strMatch(
                        aHintTable->tableName.stmtText + aHintTable->tableName.offset,
                        aHintTable->tableName.size,
                        sTable->tableRef->aliasName.stmtText + sTable->tableRef->aliasName.offset,
                        sTable->tableRef->aliasName.size) == 0 ||
                    idlOS::strMatch(
                        aHintTable->tableName.stmtText + aHintTable->tableName.offset,
                        aHintTable->tableName.size,
                        sTable->tableRef->tableName.stmtText + sTable->tableRef->tableName.offset,
                        sTable->tableRef->tableName.size) == 0)
                {
                    aHintTable->table = sTable;
                }
            }
        }
    }

}

IDE_RC qmvQuerySet::validateWhere(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aSFWGH)
{
    qcuSqlSourceInfo   sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateWhere::__FT__" );

    /* PROJ-2197 PSM Renewal */
    aSFWGH->where->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
    IDE_TEST(qtc::estimate(
                 aSFWGH->where,
                 QC_SHARED_TMPLATE(aStatement),
                 aStatement,
                 aQuerySet,
                 aSFWGH,
                 NULL )
             != IDE_SUCCESS);

    if ( ( aSFWGH->where->node.lflag &
           ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_COMPARISON_MASK ) )
         == ( MTC_NODE_LOGICAL_CONDITION_FALSE | MTC_NODE_COMPARISON_FALSE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aSFWGH->where->position );
        IDE_RAISE( ERR_NOT_PREDICATE );
    }

    if ( ( aSFWGH->where->node.lflag & MTC_NODE_DML_MASK )
         == MTC_NODE_DML_UNUSABLE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aSFWGH->where->position );
        IDE_RAISE( ERR_USE_CURSOR_ATTR );
    }

    if ( ( aSFWGH->outerHavingCase == ID_FALSE ) &&
         ( QTC_HAVE_AGGREGATE(aSFWGH->where) == ID_TRUE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aSFWGH->where->position );
        IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
    }

    if ( QTC_HAVE_AGGREGATE2(aSFWGH->where) )
    {
        IDE_RAISE( ERR_TOO_DEEPLY_NESTED_AGGR );
    }

    if ( ( aSFWGH->where->lflag & QTC_NODE_SEQUENCE_MASK )
         == QTC_NODE_SEQUENCE_EXIST )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            & aSFWGH->where->position );
        IDE_RAISE( ERR_USE_SEQUENCE_IN_WHERE );
    }

    if (((aSFWGH->where->lflag & QTC_NODE_SUBQUERY_MASK) ==
         QTC_NODE_SUBQUERY_EXIST) &&
        (aSFWGH->where->lflag & QTC_NODE_COLUMN_RID_MASK) ==
        QTC_NODE_COLUMN_RID_EXIST)
    {
        IDE_RAISE( ERR_PROWID_NOT_SUPPORTED );
    }

    /* Plan Property에 등록 */
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_QUERY_REWRITE_ENABLE );
    
    /* PROJ-1090 Function-based Index */
    if ( QCG_GET_SESSION_QUERY_REWRITE_ENABLE(aStatement) == 1 )
    {
        IDE_TEST( qmsDefaultExpr::applyFunctionBasedIndex(
                      aStatement,
                      aSFWGH->where,
                      aSFWGH->from,
                      &(aSFWGH->where) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-41289 subquery 의 where 절에도 DescribeParamInfo 적용
    if ( QC_SHARED_TMPLATE(aStatement)->stmt->myPlan->sBindParamCount > 0 )
    {
        // BUG-15746 DescribeParamInfo 설정
        IDE_TEST( qtc::setDescribeParamInfo4Where( QC_SHARED_TMPLATE(aStatement)->stmt,
                                                   aSFWGH->where )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_PREDICATE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_PREDICATE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_SEQUENCE_IN_WHERE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_IN_WHERE,
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
    IDE_EXCEPTION(ERR_TOO_DEEPLY_NESTED_AGGR)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TOO_DEEPLY_NESTED_AGGR));
    }
    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateGroupBy(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aSFWGH)
{
    qmsParseTree            * sParseTree;
    qtcNode                 * sExpression;
    qmsConcatElement        * sConcatElement;
    qmsConcatElement        * sTemp;
    qmsTarget               * sTarget;
    qtcNode                 * sNode[2];
    qtcNode                 * sListNode;
    qcNamePosition            sPosition;
    UInt                      sGroupExtCount = 0;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateGroupBy::__FT__" );

    sParseTree = (qmsParseTree *) aStatement->myPlan->parseTree;

    // FOR UPDATE clause
    IDE_TEST_RAISE(sParseTree->forUpdate != NULL, ERR_GROUP_ON_FORUPDATE);

    // PROJ-2582 recursive with
    IDE_TEST_RAISE( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                    == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT,
                    ERR_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW );

    // PROJ-2415 Grouping Sets Clause    
    if ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
         != QMV_SFWGH_GBGS_TRANSFORM_NONE )
    {
        // Nested Aggregate Function 사용불가
        IDE_TEST_RAISE( aSFWGH->aggsDepth2 != NULL, ERR_NOT_NESTED_AGGR );

        // Over Cluase 사용불가
        IDE_TEST_RAISE( aQuerySet->analyticFuncList != NULL, ERR_NOT_WINDOWING );            
    }
    else
    {
        /* Nothing to do */
    }
    
    for (sConcatElement = aSFWGH->group;
         sConcatElement != NULL;
         sConcatElement = sConcatElement->next )
    {
        switch ( sConcatElement->type )
        {
            // PROJ-2415 Grouping Sets Clause
            case QMS_GROUPBY_NULL:
            case QMS_GROUPBY_NORMAL:
                IDE_TEST_RAISE( sConcatElement->arithmeticOrList == NULL,
                                ERR_NOT_SUPPORT_COMPLICATE_GROUP_EXT );
                sExpression = sConcatElement->arithmeticOrList;

                IDE_TEST_RAISE((sExpression->node.lflag & MTC_NODE_FUNCTON_GROUPING_MASK)
                               == MTC_NODE_FUNCTON_GROUPING_TRUE,
                               ERR_NOT_SUPPORT_GROUPING_SETS);

                IDE_TEST( validateGroupOneColumn( aStatement,
                                                  aQuerySet,
                                                  aSFWGH,
                                                  sExpression )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE((sExpression->lflag & QTC_NODE_COLUMN_RID_MASK) ==
                               QTC_NODE_COLUMN_RID_EXIST,
                               ERR_PROWID_NOT_SUPPORTED);
                break;
            case QMS_GROUPBY_ROLLUP:
            case QMS_GROUPBY_CUBE:
                /* PROJ-1353 Rollup, Cube는 Group by 절에 1번만 가능하다 */
                IDE_TEST_RAISE( sGroupExtCount > 0, ERR_NOT_SUPPORT_COMPLICATE_GROUP_EXT );

                for ( sTemp = sConcatElement->arguments;
                      sTemp != NULL;
                      sTemp = sTemp->next )
                {
                    sExpression = sTemp->arithmeticOrList;

                    IDE_TEST_RAISE((sExpression->node.lflag & MTC_NODE_FUNCTON_GROUPING_MASK)
                                   == MTC_NODE_FUNCTON_GROUPING_TRUE,
                                   ERR_NOT_SUPPORT_GROUPING_SETS);

                    if ( ( sExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
                         == MTC_NODE_OPERATOR_LIST )
                    {
                        for ( sListNode = ( qtcNode * )sExpression->node.arguments;
                              sListNode != NULL;
                              sListNode = ( qtcNode * )sListNode->node.next )
                        {
                            IDE_TEST( validateGroupOneColumn( aStatement,
                                                              aQuerySet,
                                                              aSFWGH,
                                                              sListNode )
                                      != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        IDE_TEST( validateGroupOneColumn( aStatement,
                                                          aQuerySet,
                                                          aSFWGH,
                                                          sExpression )
                                  != IDE_SUCCESS );
                    }
                    IDE_TEST_RAISE((sExpression->lflag & QTC_NODE_COLUMN_RID_MASK) ==
                                   QTC_NODE_COLUMN_RID_EXIST,
                                   ERR_PROWID_NOT_SUPPORTED);
                }
                sGroupExtCount++;
                break;
            case QMS_GROUPBY_GROUPING_SETS:
                /* PROJ-2415 Grouping Sets Clause로 해당 에러는 더이상 발생하지 않게 된다.
                 * 실제로 Group의 Validation 시에 GROUPING SETS는 존재 하지 않으며
                 * 만약 이 CASE에 접근 된다면 Internal error를 발생 시켜야 한다.
                 */
                IDE_RAISE( ERR_NOT_SUPPORT_GROUPING_SETS );
                break;
            default:
                IDE_RAISE( ERR_NOT_SUPPORT_COMPLICATE_GROUP_EXT );
        }
    }

    // If a query contains the GROUP BY clause,
    // the select list can contain only the following types of expressions.
    //   - constants (including SYSDATE)
    //   - group functions
    //   - expressions identical to those in the GROUP BY clause
    //   - expressions invlving the above expressions that evaluate to
    //    the same value for all rows in a group
    if ( aSFWGH->aggsDepth2 == NULL )
    {
        for ( sTarget = aSFWGH->target;
              sTarget != NULL;
              sTarget = sTarget->next )
        {
            IDE_TEST( qmvQTC::isGroupExpression( aStatement,
                                                 aSFWGH,
                                                 sTarget->targetColumn,
                                                 ID_TRUE ) // make pass node
                      != IDE_SUCCESS);
        }
    }
    else
    {
        for ( sTarget = aSFWGH->target;
              sTarget != NULL;
              sTarget = sTarget->next )
        {
            IDE_TEST(qmvQTC::isNestedAggregateExpression( aStatement,
                                                          aQuerySet,
                                                          aSFWGH,
                                                          sTarget->targetColumn )
                     != IDE_SUCCESS);
        }
    }

    /* PROJ-1353 Grouping Function Data Address Pseudo Column */
    if ( ( sGroupExtCount > 0 ) && ( aSFWGH->groupingInfoAddr == NULL ) )
    {
        SET_EMPTY_POSITION( sPosition );

        IDE_TEST( qtc::makeColumn( aStatement,
                                   sNode,
                                   NULL,
                                   NULL,
                                   &sPosition,
                                   NULL ) != IDE_SUCCESS );
        aSFWGH->groupingInfoAddr = sNode[0];

        /* make tuple for Connect By Stack Address */
        IDE_TEST(qmvQTC::makeOneTupleForPseudoColumn( aStatement,
                                                      aSFWGH->groupingInfoAddr,
                                                      (SChar *)"BIGINT",
                                                      6 )
                 != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_GROUP_ON_FORUPDATE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_GROUP_ON_FORUPDATE));
    }
    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_GROUPING_SETS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_SUPPORT_GROUPING_SETS));
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_COMPLICATE_GROUP_EXT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_SUPPORT_COMPLICATE_GROUP_EXT));
    }
    IDE_EXCEPTION(ERR_NOT_NESTED_AGGR)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_SUPPORT_NESTED_AGGR));
    }
    IDE_EXCEPTION(ERR_NOT_WINDOWING)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_WINDOW_FUNCTION,
                                "ROLLUP, CUBE, GROUPING SETS"));
    }
    IDE_EXCEPTION( ERR_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMV_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW,
                                 "GROUP BY" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateHaving(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aSFWGH)
{
    qmsParseTree            * sParseTree;
    qcuSqlSourceInfo          sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateHaving::__FT__" );

    sParseTree = (qmsParseTree *) aStatement->myPlan->parseTree;

    // FOR UPDATE clause
    IDE_TEST_RAISE(sParseTree->forUpdate != NULL, ERR_GROUP_ON_FORUPDATE);

    /* PROJ-2197 PSM Renewal */
    aSFWGH->having->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
    IDE_TEST(qtc::estimate(
                 aSFWGH->having,
                 QC_SHARED_TMPLATE(aStatement),
                 aStatement,
                 aQuerySet,
                 aSFWGH,
                 NULL )
             != IDE_SUCCESS);

    if ( ( aSFWGH->having->node.lflag &
           ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_COMPARISON_MASK ) )
         == ( MTC_NODE_LOGICAL_CONDITION_FALSE | MTC_NODE_COMPARISON_FALSE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aSFWGH->having->position );
        IDE_RAISE( ERR_NOT_PREDICATE );
    }

    if ( ( aSFWGH->having->node.lflag & MTC_NODE_DML_MASK )
         == MTC_NODE_DML_UNUSABLE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aSFWGH->having->position );
        IDE_RAISE( ERR_USE_CURSOR_ATTR );
    }

    if (aSFWGH->group != NULL)
    {
        IDE_TEST(qmvQTC::isGroupExpression( aStatement,
                                            aSFWGH,
                                            aSFWGH->having,
                                            ID_TRUE ) // make pass node
                 != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(qmvQTC::isAggregateExpression( aStatement, aSFWGH, aSFWGH->having )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_GROUP_ON_FORUPDATE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_GROUP_ON_FORUPDATE));
    }
    IDE_EXCEPTION(ERR_NOT_PREDICATE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_PREDICATE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateHierarchy(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aSFWGH)
{
    qcuSqlSourceInfo sqlInfo;
    qtcNode        * sStart;
    qtcNode        * sConnect;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateHierarchy::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_FT_ASSERT( aSFWGH != NULL );
    IDE_FT_ASSERT( aSFWGH->from != NULL );
    IDE_FT_ASSERT( aSFWGH->hierarchy != NULL );

    sStart    = aSFWGH->hierarchy->startWith;
    sConnect  = aSFWGH->hierarchy->connectBy;
    
    //------------------------------------------
    // validate hierarchy
    //------------------------------------------
    
    // PROJ-2582 recursive with
    IDE_TEST_RAISE( ( aQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                    == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT,
                    ERR_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW );

    // validation start with clause
    if (sStart != NULL)
    {
        IDE_TEST( searchStartWithPrior( aStatement, sStart )
                  != IDE_SUCCESS );
        /* PROJ-2197 PSM Renewal */
        sStart->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        IDE_TEST(qtc::estimate(
                     sStart,
                     QC_SHARED_TMPLATE(aStatement),
                     aStatement,
                     aQuerySet,
                     aSFWGH,
                     NULL )
                 != IDE_SUCCESS);

        if ( ( sStart->node.lflag &
               ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_COMPARISON_MASK ) )
             == ( MTC_NODE_LOGICAL_CONDITION_FALSE | MTC_NODE_COMPARISON_FALSE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sStart->position );
            IDE_RAISE( ERR_NOT_PREDICATE );
        }

        // PROJ-1362
        if ( ( sStart->lflag & QTC_NODE_BINARY_MASK )
             == QTC_NODE_BINARY_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sStart->position );
            IDE_RAISE( ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE );
        }

        /* PROJ-1715 */
        if ( ( sStart->lflag & QTC_NODE_ISLEAF_MASK )
             == QTC_NODE_ISLEAF_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sStart->position );
            IDE_RAISE( ERR_NOT_USE_ISLEAF_IN_START_WITH );
        }
        else
        {
            /* Nothing to do */
        }
    }

    // validation connect by
    if (sConnect != NULL)
    {
        /* PROJ-2197 PSM Renewal */
        sConnect->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        IDE_TEST(qtc::estimate(
                     sConnect,
                     QC_SHARED_TMPLATE(aStatement),
                     aStatement,
                     NULL,
                     aSFWGH,
                     NULL )
                 != IDE_SUCCESS);

        if ( ( sConnect->node.lflag &
               ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_COMPARISON_MASK ) )
             == ( MTC_NODE_LOGICAL_CONDITION_FALSE | MTC_NODE_COMPARISON_FALSE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sConnect->position );
            IDE_RAISE( ERR_NOT_PREDICATE );
        }
        
        // PROJ-1362
        if ( ( sConnect->lflag & QTC_NODE_BINARY_MASK )
             == QTC_NODE_BINARY_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sConnect->position );
            IDE_RAISE( ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE );
        }

        /* PROJ-1715 */
        if ( ( sConnect->lflag & QTC_NODE_ISLEAF_MASK )
             == QTC_NODE_ISLEAF_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sConnect->position );
            IDE_RAISE( ERR_NOT_USE_ISLEAF_IN_CONNECT_BY );
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( searchConnectByAggr( aStatement, sConnect )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_NOT_PREDICATE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_PREDICATE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_USE_ISLEAF_IN_START_WITH)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_ISLEAF_IN_START_WITH,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_USE_ISLEAF_IN_CONNECT_BY)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_ISLEAF_IN_CONNECT_BY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMV_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW,
                                 "HIERARCHICAL query") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateJoin(
    qcStatement     * aStatement,
    qmsFrom         * aFrom,
    qmsSFWGH        * aSFWGH)
{
    qmsFrom           * sFromTemp;


    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateJoin::__FT__" );
    
    if (aFrom->joinType != QMS_NO_JOIN)
    {
        IDE_TEST(validateJoin(aStatement, aFrom->left, aSFWGH)
                 != IDE_SUCCESS);

        IDE_TEST(validateJoin(aStatement, aFrom->right, aSFWGH)
                 != IDE_SUCCESS);

        // BUG-34295 Join ordering ANSI style query
        switch( aFrom->joinType )
        {
            case QMS_NO_JOIN :
                break;
            case QMS_INNER_JOIN :
                aSFWGH->lflag |= QMV_SFWGH_JOIN_INNER;
                break;
            case QMS_FULL_OUTER_JOIN :
                aSFWGH->lflag |= QMV_SFWGH_JOIN_FULL_OUTER;
                break;
            case QMS_LEFT_OUTER_JOIN :
                aSFWGH->lflag |= QMV_SFWGH_JOIN_LEFT_OUTER;
                break;
            case QMS_RIGHT_OUTER_JOIN :
                aSFWGH->lflag |= QMV_SFWGH_JOIN_RIGHT_OUTER;
                break;
            default:
                break;
        }

        if (aFrom->joinType == QMS_RIGHT_OUTER_JOIN)
        {
            aFrom->joinType = QMS_LEFT_OUTER_JOIN;
            sFromTemp = aFrom->right;
            aFrom->right = aFrom->left;
            aFrom->left = sFromTemp;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::addLobLocatorFunc(
    qcStatement     * aStatement,
    qmsTarget       * aTarget )
{
    qmsTarget         * sCurrTarget;
    qtcNode           * sNode[2];
    qtcNode           * sPrevNode = NULL;
    qtcNode           * sExpression;
    mtcColumn         * sMtcColumn;    
    qcuSqlSourceInfo    sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::addLobLocatorFunc::__FT__" );

    // add Lob-Locator function
    for ( sCurrTarget = aTarget;
          sCurrTarget != NULL;
          sCurrTarget = sCurrTarget->next)
    {
        sMtcColumn = QTC_STMT_COLUMN( aStatement,
                                      sCurrTarget->targetColumn );
            
        if ( QTC_IS_COLUMN( aStatement, sCurrTarget->targetColumn ) == ID_TRUE )
        {
            if( sMtcColumn->module->id == MTD_BLOB_ID )
            {
                // get_blob_locator 함수를 만든다.
                IDE_TEST( qtc::makeNode(aStatement,
                                        sNode,
                                        &sCurrTarget->targetColumn->columnName,
                                        &mtfGetBlobLocator )
                          != IDE_SUCCESS );

                // 함수를 연결한다.
                sNode[0]->node.arguments = (mtcNode*)sCurrTarget->targetColumn;
                sNode[0]->node.next = sCurrTarget->targetColumn->node.next;
                sNode[0]->node.arguments->next = NULL;

                if ( sPrevNode != NULL )
                {
                    sPrevNode->node.next = (mtcNode*)sNode[0];
                }
                else
                {
                    // Nothing to do.
                }

                // 함수만 estimate 를 수행한다.
                IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                         sNode[0] )
                          != IDE_SUCCESS );

                // target 노드를 바꾼다.
                sCurrTarget->targetColumn = sNode[0];

                sPrevNode = sNode[0];
            }
            else if( sMtcColumn->module->id == MTD_CLOB_ID )
            {
                // get_clob_locator 함수를 만든다.
                IDE_TEST( qtc::makeNode(aStatement,
                                        sNode,
                                        &sCurrTarget->targetColumn->columnName,
                                        &mtfGetClobLocator )
                          != IDE_SUCCESS );

                // 함수를 연결한다.
                sNode[0]->node.arguments = (mtcNode*)sCurrTarget->targetColumn;
                sNode[0]->node.next = sCurrTarget->targetColumn->node.next;
                sNode[0]->node.arguments->next = NULL;

                if ( sPrevNode != NULL )
                {
                    sPrevNode->node.next = (mtcNode*)sNode[0];
                }
                else
                {
                    // Nothing to do.
                }

                // 함수만 estimate 를 수행한다.
                IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                         sNode[0] )
                          != IDE_SUCCESS );

                // target 노드를 바꾼다.
                sCurrTarget->targetColumn = sNode[0];

                sPrevNode = sNode[0];
            }
            else
            {
                sPrevNode = sCurrTarget->targetColumn;
            }
        }
        else
        {
            // SP외에서 select target에 lob value가 오는 경우 에러.
            if ( ( aStatement->spvEnv->createProc == NULL ) &&
                 ( aStatement->calledByPSMFlag != ID_TRUE ) &&
                 ( ( sMtcColumn->module->id == MTD_BLOB_ID ) ||
                   ( sMtcColumn->module->id == MTD_CLOB_ID ) ) )
            {
                sExpression = sCurrTarget->targetColumn;

                sqlInfo.setSourceInfo( aStatement,
                                       & sExpression->position );
                IDE_RAISE( ERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT );
            }
            else
            {
                sPrevNode = sCurrTarget->targetColumn;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateQmsTarget(qcStatement* aStatement,
                                      qmsQuerySet* aQuerySet,
                                      qmsSFWGH   * aSFWGH)
{
    qmsTarget        * sPrevTarget;
    qmsTarget        * sCurrTarget;
    qmsTarget        * sFirstTarget;
    qmsTarget        * sLastTarget;
    qmsFrom          * sFrom;
    qcuSqlSourceInfo   sqlInfo;
    mtcColumn        * sMtcColumn;

    /* PROJ-2197 PSM Renewal */
    qsProcStmtSql    * sSql;

    /* PROJ-2523 Unpivot clause */
    mtcNode * sMtcNode = NULL;
    UShort    sTupleID = ID_USHORT_MAX;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateQmsTarget::__FT__" );

    // 기본 초기화
    sPrevTarget  = NULL;
    sCurrTarget  = NULL;
    sFirstTarget = NULL;
    sLastTarget  = NULL;

    // PROJ-2469 Optimize View Materialization
    // Initialization For Re-Validation
    aSFWGH->currentTargetNum = 0; 

    if (aSFWGH->target == NULL) 
    {
        //---------------------
        // SELECT * FROM ...
        // expand asterisk
        //---------------------
        for (sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
        {
            IDE_TEST(expandAllTarget( aStatement,
                                      aQuerySet,
                                      aSFWGH,
                                      sFrom,
                                      NULL,
                                      & sFirstTarget,
                                      & sLastTarget )
                     != IDE_SUCCESS);

            if ( sPrevTarget == NULL )
            {
                aSFWGH->target = sFirstTarget;
            }
            else
            {
                sPrevTarget->next = sFirstTarget;
                sPrevTarget->targetColumn->node.next =
                    (mtcNode *)sFirstTarget->targetColumn;
            }
            sPrevTarget = sLastTarget;
        }
    }
    else
    {
        // BUG-34234
        // target절에 사용된 호스트 변수를 varchar type으로 강제 고정한다.
        IDE_TEST( addCastOper( aStatement,
                               aSFWGH->target )
                  != IDE_SUCCESS );
        
        for ( sCurrTarget = aSFWGH->target, sPrevTarget = NULL;
              sCurrTarget != NULL;
              sCurrTarget = sCurrTarget->next)
        {
            if ( ( sCurrTarget->flag & QMS_TARGET_ASTERISK_MASK )
                 == QMS_TARGET_ASTERISK_TRUE )
            {
                if ( QC_IS_NULL_NAME( sCurrTarget->targetColumn->tableName ) == ID_TRUE )
                {
                    //---------------------
                    // in case of *
                    // expand target
                    //---------------------

                    for (sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
                    {
                        IDE_TEST(expandAllTarget( aStatement,
                                                  aQuerySet,
                                                  aSFWGH,
                                                  sFrom,
                                                  sCurrTarget,
                                                  & sFirstTarget,
                                                  & sLastTarget )
                                 != IDE_SUCCESS);

                        if ( sPrevTarget == NULL )
                        {
                            aSFWGH->target = sFirstTarget;
                        }
                        else
                        {
                            sPrevTarget->next = sFirstTarget;
                            sPrevTarget->targetColumn->node.next =
                                (mtcNode *)sFirstTarget->targetColumn;
                        }
                        sPrevTarget = sLastTarget;
                    }
                }
                else
                {
                    //---------------------
                    // in case of t1.*, u1.t1.*
                    // expand target & get userID
                    //---------------------
                    IDE_TEST( expandTarget( aStatement,
                                            aQuerySet,
                                            aSFWGH,
                                            sCurrTarget,
                                            & sFirstTarget,
                                            & sLastTarget )
                              != IDE_SUCCESS );

                    if ( sPrevTarget == NULL )
                    {
                        aSFWGH->target = sFirstTarget;
                    }
                    else
                    {
                        sPrevTarget->next = sFirstTarget;
                        sPrevTarget->targetColumn->node.next =
                            (mtcNode *)sFirstTarget->targetColumn;
                    }
                    sPrevTarget = sLastTarget;
                }
            }
            else 
            {
                //---------------------
                // expression
                //---------------------
                IDE_TEST( validateTargetExp( aStatement,
                                             aQuerySet,
                                             aSFWGH,
                                             sCurrTarget )
                          != IDE_SUCCESS );
                // PROJ-2469 Optimize View Materialization
                // SFWGH에 현재 Validation중인 Target이 몇 번 째 Target인지 기록한다.
                // ( viewColumnRefList 생성 시 활용 )
                aSFWGH->currentTargetNum++;
                sPrevTarget = sCurrTarget;
            }
        }
    }

    /*
     * BUG-16000
     * lob or binary type은 distinct를 사용할 수 없다.
     *
     * PROJ-1789 PROWID
     * SELECT DISTINCT (_PROWID) FROM .. 지원 X
     *
     * BUG-41047 distinct list type X
     */
    if ( aSFWGH->selectType == QMS_DISTINCT )
    {
        for ( sCurrTarget = aSFWGH->target;
              sCurrTarget != NULL;
              sCurrTarget = sCurrTarget->next)
        {
            IDE_TEST_RAISE(
                qtc::isEquiValidType( sCurrTarget->targetColumn,
                                      & QC_SHARED_TMPLATE(aStatement)->tmplate )
                == ID_FALSE,
                ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT );

            IDE_TEST_RAISE((sCurrTarget->targetColumn->lflag &
                            QTC_NODE_COLUMN_RID_MASK) ==
                           QTC_NODE_COLUMN_RID_EXIST,
                           ERR_PROWID_NOT_SUPPORTED);

            if (sCurrTarget->targetColumn->node.module == &mtfList)
            {
                sqlInfo.setSourceInfo(aStatement, &sCurrTarget->targetColumn->position );
                IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
            }
            else
            {
                /* nothing to do */
            }

        }
    }
    else
    {
        // Nothing to do.
    }

    // set isNullable
    for ( sCurrTarget = aSFWGH->target;
          sCurrTarget != NULL;
          sCurrTarget = sCurrTarget->next)
    {
        sMtcColumn = QTC_STMT_COLUMN(aStatement, sCurrTarget->targetColumn);

        if ((sMtcColumn->flag & MTC_COLUMN_NOTNULL_MASK) == MTC_COLUMN_NOTNULL_TRUE)
        {
            // BUG-20928
            if( QTC_IS_AGGREGATE( sCurrTarget->targetColumn ) == ID_TRUE )
            {
                sCurrTarget->flag &= ~QMS_TARGET_IS_NULLABLE_MASK;
                sCurrTarget->flag |= QMS_TARGET_IS_NULLABLE_TRUE;
            }
            else
            {
                sCurrTarget->flag &= ~QMS_TARGET_IS_NULLABLE_MASK;
                sCurrTarget->flag |= QMS_TARGET_IS_NULLABLE_FALSE;
            }
        }
        else
        {
            sCurrTarget->flag &= ~QMS_TARGET_IS_NULLABLE_MASK;
            sCurrTarget->flag |= QMS_TARGET_IS_NULLABLE_TRUE;
        }
    }

    // PROJ-2002 Column Security
    // subquery의 target에 암호 컬럼이 오는 경우 decrypt 함수를 생성한다.
    IDE_TEST( addDecryptFunc( aStatement, aSFWGH->target )
              != IDE_SUCCESS );
    
    // PROJ-1362
    IDE_TEST( addLobLocatorFunc( aStatement, aSFWGH->target )
              != IDE_SUCCESS );

    // in case of stored procedure
    if (aSFWGH->intoVariables != NULL)
    {
        if (aSFWGH->outerQuery != NULL)
        {
            IDE_DASSERT( aSFWGH->intoVariables->intoNodes != NULL );
            
            sqlInfo.setSourceInfo(
                aStatement,
                & aSFWGH->intoVariables->intoNodes->position );
            IDE_RAISE( ERR_INAPPROPRIATE_INTO );
        }

        // in case of SELECT in procedure
        if ( ( aStatement->spvEnv->createProc != NULL ) ||
             ( aStatement->spvEnv->createPkg != NULL ) )
        {
            sSql = (qsProcStmtSql*)(aStatement->spvEnv->currStmt);

            // BUG-38099
            IDE_TEST_CONT( sSql == NULL, SKIP_VALIDATE_TARGET );

            IDE_TEST_CONT( qsvProcStmts::isFetchType(sSql->common.stmtType) != ID_TRUE, 
                           SKIP_VALIDATE_TARGET );

            IDE_TEST( qsvProcStmts::validateIntoClause(
                          aStatement,
                          aSFWGH->target,
                          aSFWGH->intoVariables )
                      != IDE_SUCCESS );

            sSql->intoVariables = aSFWGH->intoVariables;
            sSql->from          = aSFWGH->from;
        }
    }

    /* PROJ-2523 Unpivot clause
     *  Unpivot transform에 사용된 column이 실제 해당 table의 column인지 확인한다.
     *  Pseudo column, PSM variable등 이 통과되는 것 을 방지한다.
     */
    if ( ( aSFWGH->lflag & QMV_SFWGH_UNPIVOT_MASK ) == QMV_SFWGH_UNPIVOT_TRUE )
    {
        sTupleID =  aSFWGH->from->tableRef->table;

        for ( sCurrTarget = aSFWGH->target;
              sCurrTarget != NULL;
              sCurrTarget = sCurrTarget->next)
        {
            if ( ( sCurrTarget->flag & QMS_TARGET_UNPIVOT_COLUMN_MASK )
                 == QMS_TARGET_UNPIVOT_COLUMN_TRUE )
            {
                IDE_DASSERT( sCurrTarget->targetColumn->node.module == &mtfDecodeOffset );

                for ( sMtcNode  = sCurrTarget->targetColumn->node.arguments->next;
                      sMtcNode != NULL;
                      sMtcNode  = sMtcNode->next )
                {
                    if ( sMtcNode->table != sTupleID )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & ((qtcNode*)sMtcNode)->columnName );

                        IDE_RAISE( ERR_COLUMN_NOT_IN_UNPIVOT_TABLE );
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
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( SKIP_VALIDATE_TARGET );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_INAPPROPRIATE_INTO);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_INAPPROPRIATE_INTO_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT));
    }
    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }
    IDE_EXCEPTION(ERR_NOT_APPLICABLE_TYPE_IN_TARGET)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_COLUMN_NOT_IN_UNPIVOT_TABLE );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_INVALID_UNPIVOT_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::expandAllTarget(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aSFWGH,
    qmsFrom         * aFrom,
    qmsTarget       * aCurrTarget,
    qmsTarget      ** aFirstTarget,
    qmsTarget      ** aLastTarget)
{
/***********************************************************************
 *
 * Description : 전체 target 확장
 *
 * Implementation :
 *    SELECT * FROM ...과 같은 경우, parsing 과정에서 target에
 *    해당하는 칼럼 노드가 생성되지 않는다.
 *    이를 이 함수에서 생성한다.
 ***********************************************************************/
    qmsTableRef       * sTableRef;
    qmsTarget         * sFirstTarget;
    qmsTarget         * sLastTarget;
    qcuSqlSourceInfo    sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::expandAllTarget::__FT__" );

    if (aFrom->joinType != QMS_NO_JOIN)
    {
        IDE_TEST(expandAllTarget( aStatement, 
                                  aQuerySet, 
                                  aSFWGH, 
                                  aFrom->left, 
                                  aCurrTarget, 
                                  aFirstTarget, 
                                  & sLastTarget )
                 != IDE_SUCCESS);

        IDE_TEST(expandAllTarget( aStatement, 
                                  aQuerySet, 
                                  aSFWGH, 
                                  aFrom->right, 
                                  aCurrTarget, 
                                  & sFirstTarget,
                                  aLastTarget )
                 != IDE_SUCCESS);

        sLastTarget->next = sFirstTarget;
        sLastTarget->targetColumn->node.next
            = & sFirstTarget->targetColumn->node;
    }
    else
    {
        // check duplicated specified table or alias name
        sTableRef = NULL;

        if (aFrom->tableRef->aliasName.size > 0)
        {
            IDE_TEST(getTableRef( aStatement,
                                  aSFWGH->from,
                                  aFrom->tableRef->userID,
                                  aFrom->tableRef->aliasName,
                                  &sTableRef)
                     != IDE_SUCCESS);
        }
        else
        {
            sTableRef = aFrom->tableRef;
        }

        if (aFrom->tableRef != sTableRef)
        {
            sqlInfo.setSourceInfo(
                aStatement,
                & aFrom->tableRef->aliasName );
            IDE_RAISE( ERR_INVALID_TABLE_NAME );
        }

        // make target list for current table
        IDE_TEST(makeTargetListForTableRef( aStatement,
                                            aQuerySet,
                                            aSFWGH,
                                            sTableRef,
                                            aFirstTarget,
                                            aLastTarget)
                 != IDE_SUCCESS);

        if ( aCurrTarget != NULL )
        {
            (*aLastTarget)->next = aCurrTarget->next;
            (*aLastTarget)->targetColumn->node.next
                = aCurrTarget->targetColumn->node.next;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_TABLE_NAME)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_INVALID_TABLE_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::expandTarget(
    qcStatement  * aStatement,
    qmsQuerySet  * aQuerySet,
    qmsSFWGH     * aSFWGH,
    qmsTarget    * aCurrTarget,
    qmsTarget   ** aFirstTarget,
    qmsTarget   ** aLastTarget )
{
/***********************************************************************
 *
 * Description : target 확장
 *
 * Implementation :
 *    targe 중 'T1.*', 'U1,T1.*' 과 같은 경우는 Parsing 과정에서 target에
 *    해당하는 칼럼 노드가 생성되지 않는다.
 *    이를 이 함수에서 생성한다.
 ***********************************************************************/
    UInt               sUserID;
    qmsTableRef      * sTableRef;
    qcuSqlSourceInfo   sqlInfo;
    qtcNode          * sExpression;
    qmsFrom          * sFrom;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::expandTarget::__FT__" );
    
    sExpression = aCurrTarget->targetColumn;

    if (QC_IS_NULL_NAME(sExpression->userName) == ID_TRUE)
    {
        sUserID = QC_EMPTY_USER_ID;
    }
    else
    {
        IDE_TEST(qcmUser::getUserID( aStatement,
                                     sExpression->userName,
                                     &(sUserID))
                 != IDE_SUCCESS);
    }

    // search table in specified table list
    sTableRef = NULL;

    // PROJ-2415 Grouping Sets Clause
    if ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
         == QMV_SFWGH_GBGS_TRANSFORM_MIDDLE )
    {
        sFrom = ( ( qmsParseTree * )aSFWGH->from->tableRef->view->myPlan->parseTree )->querySet->SFWGH->from;
    }
    else
    {
        sFrom = aSFWGH->from;
    }

    IDE_TEST(getTableRef( aStatement,
                          sFrom,
                          sUserID,
                          sExpression->tableName,
                          &sTableRef)
             != IDE_SUCCESS);

    if (sTableRef == NULL)
    {
        sqlInfo.setSourceInfo( aStatement, & sExpression->tableName );
        IDE_RAISE( ERR_INVALID_TABLE_NAME );
    }

    // make target list for current table
    IDE_TEST(makeTargetListForTableRef( aStatement,
                                        aQuerySet,
                                        aSFWGH,
                                        sTableRef,
                                        aFirstTarget,
                                        aLastTarget)
             != IDE_SUCCESS);

    (*aLastTarget)->next = aCurrTarget->next;
    (*aLastTarget)->targetColumn->node.next
        = aCurrTarget->targetColumn->node.next;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_TABLE_NAME)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_INVALID_TABLE_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateTargetExp(
    qcStatement  * aStatement,
    qmsQuerySet  * aQuerySet,
    qmsSFWGH     * aSFWGH,
    qmsTarget    * aCurrTarget )
{
/***********************************************************************
 *
 * Description : target expression에 대하여 validation 검사
 *
 * Implementation :
 *
 ***********************************************************************/
    qcuSqlSourceInfo   sqlInfo;
    qtcNode          * sExpression;
    mtcColumn        * sColumn;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateTargetExp::__FT__" );

    sExpression = aCurrTarget->targetColumn;

    /* PROJ-2197 PSM Renewal */
    sExpression->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

    // BUG-42790
    // target에 집계함수가 오면 변수일 수 없다.
    if ( ( sExpression->node.module != &qtc::columnModule ) &&
         ( sExpression->node.module != &qtc::spFunctionCallModule ) &&
         ( ( ( sExpression->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) ==
           QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST ) )
    {
        sExpression->lflag &= ~QTC_NODE_SP_ARRAY_INDEX_VAR_MASK;
        sExpression->lflag |= QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST(qtc::estimate( sExpression,
                            QC_SHARED_TMPLATE(aStatement),
                            aStatement,
                            aQuerySet,
                            aSFWGH,
                            NULL )
             != IDE_SUCCESS);

    // BUG-43001
    if ( ( sExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_LIST )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sExpression->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
    }

    //fix BUG-16538
    IDE_TEST( qtc::estimateConstExpr( sExpression,
                                      QC_SHARED_TMPLATE(aStatement),
                                      aStatement ) != IDE_SUCCESS );

    if ( ( sExpression->node.lflag & MTC_NODE_DML_MASK )
         == MTC_NODE_DML_UNUSABLE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sExpression->position );
        IDE_RAISE( ERR_USE_CURSOR_ATTR );
    }

    // PROJ-1492
    if( ( aStatement->calledByPSMFlag == ID_FALSE ) &&
        ( MTC_NODE_IS_DEFINED_TYPE( & sExpression->node ) )
        == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sExpression->position );
        IDE_RAISE( ERR_NOT_ALLOW_HOSTVAR );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-14094
    if ( ( sExpression->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sExpression->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-14094
    if ( ( sExpression->node.lflag & MTC_NODE_COMPARISON_MASK )
         == MTC_NODE_COMPARISON_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sExpression->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-15897
    if ( ( sExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        if ( (sExpression->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) > 1 )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sExpression->position );
            IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
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

    // BUG-15896
    // BUG-24133
    sColumn = QTC_TMPL_COLUMN( QC_SHARED_TMPLATE(aStatement),
                               sExpression );
                
    if ( ( sColumn->module->id == MTD_BOOLEAN_ID ) ||
         ( sColumn->module->id == MTD_ROWTYPE_ID ) ||
         ( sColumn->module->id == MTD_RECORDTYPE_ID ) ||
         ( sColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) || 
         ( sColumn->module->id == MTD_REF_CURSOR_ID ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sExpression->position );
        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1789 Updatable Scrollable Cursor (a.k.a. PROWID)
    IDE_TEST(setTargetColumnInfo(aStatement, aCurrTarget) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_HOSTVAR );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOWED_HOSTVAR,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::getTableRef(
    qcStatement       * aStatement,
    qmsFrom           * aFrom,
    UInt                aUserID,
    qcNamePosition      aTableName,
    qmsTableRef      ** aTableRef)
{
    qmsTableRef     * sTableRef;
    qcuSqlSourceInfo  sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::getTableRef::__FT__" );

    if (aFrom->joinType != QMS_NO_JOIN)
    {
        IDE_TEST(getTableRef(
                     aStatement, aFrom->left, aUserID, aTableName, aTableRef)
                 != IDE_SUCCESS);

        IDE_TEST(getTableRef(
                     aStatement, aFrom->right, aUserID, aTableName, aTableRef)
                 != IDE_SUCCESS);
    }
    else
    {
        sTableRef = aFrom->tableRef;

        // compare table name
        if ( ( sTableRef->userID == aUserID || aUserID == QC_EMPTY_USER_ID ) &&
             sTableRef->aliasName.size > 0 )
        {
            if ( QC_IS_NAME_MATCHED( sTableRef->aliasName, aTableName ) )
            {
                if (*aTableRef != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sTableRef->aliasName );
                    IDE_RAISE( ERR_AMBIGUOUS_DEFINED_COLUMN );
                }
                *aTableRef = sTableRef;
            }
        }
    }

    if (aFrom->next != NULL)
    {
        IDE_TEST(getTableRef(
                     aStatement, aFrom->next, aUserID, aTableName, aTableRef)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_AMBIGUOUS_DEFINED_COLUMN)
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

IDE_RC qmvQuerySet::makeTargetListForTableRef(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qmsSFWGH        * aSFWGH,
    qmsTableRef     * aTableRef,
    qmsTarget      ** aFirstTarget,
    qmsTarget      ** aLastTarget)
{
    qcmTableInfo    * sTableInfo;
    qcmColumn       * sCurrColumn;
    qmsTarget       * sPrevTarget;
    qmsTarget       * sCurrTarget;
    UShort            sColumn;
    SChar             sDisplayName[ QC_MAX_OBJECT_NAME_LEN*2 + ID_SIZEOF(UChar) + 1 ];
    UInt              sDisplayNameMaxSize = QC_MAX_OBJECT_NAME_LEN * 2;
    UInt              sNameLen = 0;

    // PROJ-2415 Grouping Sets Clause
    qmsParseTree    * sChildParseTree;
    qmsTarget       * sChildTarget;
    UShort            sStartColumn = ID_USHORT_MAX;
    UShort            sEndColumn   = ID_USHORT_MAX;
    UShort            sCount;
    qmsTableRef     * sTableRef = NULL;

    /* TASK-7219 */
    UShort            sOrderStart = ID_USHORT_MAX;
    UShort            sOrderEnd   = ID_USHORT_MAX;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::makeTargetListForTableRef::__FT__" );

    *aFirstTarget = NULL;
    *aLastTarget  = NULL;

    // PROJ-2415 Gropuing Sets Clause
    // 하위 inLineView의 Target에서 해당하는 Column을 찾는다.
    if ( ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
           == QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) &&
         ( aSFWGH->from->tableRef != aTableRef ) )
    {
        sChildParseTree  = ( qmsParseTree * )aSFWGH->from->tableRef->view->myPlan->parseTree;
        
        sTableRef = aSFWGH->from->tableRef;

        for ( sChildTarget  = sChildParseTree->querySet->SFWGH->target, sCount = 0;
              sChildTarget != NULL;
              sChildTarget  = sChildTarget->next, sCount++ )
        {
            if ( aTableRef->table == sChildTarget->targetColumn->node.table )
            {
                if ( sStartColumn == ID_USHORT_MAX )
                {
                    sStartColumn = sCount;
                    sEndColumn = sCount;
                }
                else
                {
                    sEndColumn++;
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
        sTableRef = aTableRef;
    }

    /* TASK-7219
     *   Implicit Order By Transform 로 추가된 Target 이 Shard View 생성에
     *   Select * 형태로 인해서 최상위 결과에 노출될 수 있다.
     *   해당 Target 은 추가된 것으로 사용자 쿼리 내에 없는 Target 이고
     *   결과에서 제외해야 하므로, 미리 위치를 검사한다.
     *
     *   주의할 점은 Implicit Order By Transform 된 Target 사이에
     *   다른 Transform 으로 생성된 Target 이 있으면 안된다.
     */
    if ( ( aSFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK )
         == QMV_SFWGH_SHARD_ORDER_BY_TRANS_TRUE )
    {
        sChildParseTree = (qmsParseTree *)aSFWGH->from->tableRef->view->myPlan->parseTree;

        for ( sChildTarget  = sChildParseTree->querySet->SFWGH->target, sCount = 0;
              sChildTarget != NULL;
              sChildTarget  = sChildTarget->next, sCount++ )
        {
            if ( ( sChildTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                 != QMS_TARGET_SHARD_ORDER_BY_TRANS_NONE )
            {
                if ( sOrderStart == ID_USHORT_MAX )
                {
                    sOrderStart = sCount;
                    sOrderEnd   = sCount + 1;
                }
                else
                {
                    sOrderEnd++;
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
        /* Nothing to do */
    }

    sTableInfo = sTableRef->tableInfo;

    for (sPrevTarget = NULL,
             sColumn = 0,
             sCurrColumn = sTableInfo->columns;
         sCurrColumn != NULL;
         sCurrColumn = sCurrColumn->next, sColumn++)
    {
        /* PROJ-1090 Function-based Index */
        if ( ( (sCurrColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
               == QCM_COLUMN_HIDDEN_COLUMN_TRUE ) ||
             ( (sCurrColumn->flag & QCM_COLUMN_GBGS_ORDER_BY_NODE_MASK) // PROJ-2415 Grouping Sets Clause 
               == QCM_COLUMN_GBGS_ORDER_BY_NODE_TRUE ) 
             )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        
        // PROJ-2415 Grouping Sets Clause
        if ( ( ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                 == QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) &&
               ( ( sColumn < sStartColumn ) || ( sColumn > sEndColumn ) ) ) &&
             ( sTableRef != aTableRef ) )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        /* TASK-7219
         *   추가된 Target 은 사용자 쿼리 내에 없는 Target 이고
         *   결과에서 제외해야 한다.
         */
        if ( ( ( aSFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK )
               == QMV_SFWGH_SHARD_ORDER_BY_TRANS_TRUE )
               &&
               ( ( sColumn >= sOrderStart ) && ( sColumn < sOrderEnd ) ) )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                              qmsTarget,
                              &sCurrTarget)
                 != IDE_SUCCESS);
        QMS_TARGET_INIT(sCurrTarget);

        // make expression for u1.t1.c1
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                              qtcNode,
                              &(sCurrTarget->targetColumn))
                 != IDE_SUCCESS);

        // set member of qtcNode
        IDE_TEST(qtc::makeTargetColumn(
                     sCurrTarget->targetColumn,
                     sTableRef->table,
                     sColumn)
                 != IDE_SUCCESS);

        IDE_TEST(qtc::estimate(
                     sCurrTarget->targetColumn,
                     QC_SHARED_TMPLATE(aStatement),
                     NULL,
                     aQuerySet,
                     aSFWGH,
                     NULL )
                 != IDE_SUCCESS);

        // PROJ-1413
        // view 컬럼 참조를 기록한다.
        IDE_TEST( qmvQTC::addViewColumnRefListForTarget(
                      aStatement,
                      sCurrTarget->targetColumn,
                      aSFWGH->currentTargetNum,
                      sCurrTarget->targetColumn->node.column )
                  != IDE_SUCCESS );

        /* TASK-7219 Shard Transformer Refactoring */

        /* BUG-37658 : meta table info 가 free 될 수 있으므로 다음 항목을
         *             alloc -> memcpy 한다.
         * - userName
         * - tableName
         * - aliasTableName
         * - columnName
         * - aliasColumnName
         * - displayName
         */

        // set user name
        sNameLen = idlOS::strlen(sTableInfo->tableOwnerName);

        if ( sNameLen > 0 )
        {
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              sNameLen + 1,
                                              &(sCurrTarget->userName.name) )
                      != IDE_SUCCESS );
            idlOS::memcpy( sCurrTarget->userName.name,
                           sTableInfo->tableOwnerName,
                           sNameLen );
            sCurrTarget->userName.name[sNameLen] = '\0';
            sCurrTarget->userName.size = sNameLen;
        }
        else
        {
            sCurrTarget->userName.name = NULL;
            sCurrTarget->userName.size = QC_POS_EMPTY_SIZE;

            SET_EMPTY_POSITION( sCurrTarget->targetColumn->userName );
        }

        // set table name
        sNameLen = idlOS::strlen(sTableInfo->name);

        if ( sNameLen > 0 )
        {
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              sNameLen + 1,
                                              &(sCurrTarget->tableName.name) )
                      != IDE_SUCCESS );
            idlOS::memcpy( sCurrTarget->tableName.name,
                           sTableInfo->name,
                           sNameLen );
            sCurrTarget->tableName.name[sNameLen] = '\0';
            sCurrTarget->tableName.size = sNameLen;

            sCurrTarget->targetColumn->tableName.stmtText = sCurrTarget->tableName.name;
            sCurrTarget->targetColumn->tableName.offset   = 0;
            sCurrTarget->targetColumn->tableName.size     = sCurrTarget->tableName.size;
        }
        else
        {
            sCurrTarget->tableName.name = NULL;
            sCurrTarget->tableName.size = QC_POS_EMPTY_SIZE;

            SET_EMPTY_POSITION( sCurrTarget->targetColumn->tableName );
        }

        // set alias table name
        if ( QC_IS_NULL_NAME( sTableRef->aliasName ) == ID_TRUE )
        {
            sCurrTarget->aliasTableName.name = sCurrTarget->tableName.name;
            sCurrTarget->aliasTableName.size = sCurrTarget->tableName.size;
        }
        else
        {
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM(aStatement),
                                              SChar,
                                              sTableRef->aliasName.size + 1,
                                              &(sCurrTarget->aliasTableName.name))
                      != IDE_SUCCESS );
            idlOS::memcpy( sCurrTarget->aliasTableName.name,
                           sTableRef->aliasName.stmtText +
                           sTableRef->aliasName.offset,
                           sTableRef->aliasName.size );
            sCurrTarget->aliasTableName.name[ sTableRef->aliasName.size ] = '\0';
            sCurrTarget->aliasTableName.size = sTableRef->aliasName.size;
        }

        // set base column name
        sNameLen = idlOS::strlen(sCurrColumn->name);

        if ( sNameLen > 0 )
        {
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              sNameLen + 1,
                                              &(sCurrTarget->columnName.name) )
                      != IDE_SUCCESS );
            idlOS::memcpy( sCurrTarget->columnName.name,
                           sCurrColumn->name,
                           sNameLen );
            sCurrTarget->columnName.name[sNameLen] = '\0';
            sCurrTarget->columnName.size = sNameLen;

            sCurrTarget->targetColumn->columnName.stmtText = sCurrTarget->columnName.name;
            sCurrTarget->targetColumn->columnName.offset   = 0;
            sCurrTarget->targetColumn->columnName.size     = sCurrTarget->columnName.size;
        }
        else
        {
            sCurrTarget->columnName.name = NULL;
            sCurrTarget->columnName.size = QC_POS_EMPTY_SIZE;

            SET_EMPTY_POSITION( sCurrTarget->targetColumn->columnName );
        }

        // set alias column name
        if ( QC_IS_NULL_NAME( sCurrTarget->columnName ) == ID_TRUE )
        {
            sCurrTarget->aliasColumnName.name = sCurrTarget->columnName.name;
            sCurrTarget->aliasColumnName.size = sCurrTarget->columnName.size;
        }
        else
        {
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              sCurrTarget->columnName.size + 1,
                                              &(sCurrTarget->aliasColumnName.name) )
                      != IDE_SUCCESS);
            idlOS::memcpy( sCurrTarget->aliasColumnName.name,
                           sCurrTarget->columnName.name,
                           sCurrTarget->columnName.size );
            sCurrTarget->aliasColumnName.name[sCurrTarget->columnName.size] = '\0';
            sCurrTarget->aliasColumnName.size = sCurrTarget->columnName.size;
        }

        // set display name
        sNameLen = 0;
        sCurrTarget->displayName.size = 0;

        // set display name : table name
        if ( QCG_GET_SESSION_SELECT_HEADER_DISPLAY( aStatement ) == 0 )
        {
            if ( QC_IS_NULL_NAME( sTableRef->aliasName ) == ID_FALSE )
            {
                if ( sDisplayNameMaxSize > (UInt)(sTableRef->aliasName.size + 1) )
                {
                    // table name
                    idlOS::memcpy(
                        sDisplayName + sNameLen,
                        sTableRef->aliasName.stmtText + sTableRef->aliasName.offset,
                        sTableRef->aliasName.size);
                    sNameLen += sTableRef->aliasName.size;

                    // .
                    idlOS::memcpy(
                        sDisplayName + sNameLen,
                        ".",
                        1);
                    sNameLen += 1;
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
            /* Noting to do */
        }        

        // environment의 기록
        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_SELECT_HEADER_DISPLAY );

        // set display name - column name
        if (sCurrTarget->aliasColumnName.size > 0)
        {
            if (sDisplayNameMaxSize >
                sNameLen + sCurrTarget->aliasColumnName.size )
            {
                idlOS::memcpy(
                    sDisplayName + sNameLen,
                    sCurrTarget->aliasColumnName.name,
                    sCurrTarget->aliasColumnName.size);
                sNameLen += sCurrTarget->aliasColumnName.size;
            }
            else
            {
                idlOS::memcpy(
                    sDisplayName + sNameLen,
                    sCurrTarget->aliasColumnName.name,
                    sDisplayNameMaxSize - sNameLen );
                sNameLen = sDisplayNameMaxSize;
            }
        }
        else
        {
            if (sCurrColumn->namePos.size > 0)
            {
                if (sDisplayNameMaxSize >
                    sNameLen + sCurrColumn->namePos.size)
                {
                    idlOS::memcpy(
                        sDisplayName + sNameLen,
                        sCurrColumn->namePos.stmtText + sCurrColumn->namePos.offset,
                        sCurrColumn->namePos.size);
                    sNameLen += sCurrColumn->namePos.size;
                }
                else
                {
                    // BUG-42775 Invalid read of size 1
                    idlOS::memcpy(
                        sDisplayName + sNameLen,
                        sCurrColumn->namePos.stmtText + sCurrColumn->namePos.offset,
                        sDisplayNameMaxSize - sNameLen );
                    sNameLen = sDisplayNameMaxSize;
                }
            }
        }

        if (sNameLen > 0)
        {
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            sNameLen + 1,
                                            &(sCurrTarget->displayName.name))
                     != IDE_SUCCESS);

            idlOS::memcpy(sCurrTarget->displayName.name,
                          sDisplayName,
                          sNameLen);
            sCurrTarget->displayName.name[sNameLen] = '\0';
            sCurrTarget->displayName.size = sNameLen;
        }
        else
        {
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            sNameLen + 1,
                                            &(sCurrTarget->displayName.name))
                     != IDE_SUCCESS);

            sCurrTarget->displayName.name[sNameLen] = '\0';
            sCurrTarget->displayName.size = sNameLen;
        }

        // set flag
        if ( ( sCurrColumn->basicInfo->flag & MTC_COLUMN_NOTNULL_MASK )
             == MTC_COLUMN_NOTNULL_TRUE )
        {
            // BUG-20928
            if( QTC_IS_AGGREGATE( sCurrTarget->targetColumn ) == ID_TRUE )
            {
                sCurrTarget->flag &= ~QMS_TARGET_IS_NULLABLE_MASK;
                sCurrTarget->flag |= QMS_TARGET_IS_NULLABLE_TRUE;
            }
            else
            {
                sCurrTarget->flag &= ~QMS_TARGET_IS_NULLABLE_MASK;
                sCurrTarget->flag |= QMS_TARGET_IS_NULLABLE_FALSE;
            }
        }
        else
        {
            sCurrTarget->flag &= ~QMS_TARGET_IS_NULLABLE_MASK;
            sCurrTarget->flag |= QMS_TARGET_IS_NULLABLE_TRUE;
        }

        if ( sTableRef->view == NULL )
        {
            sCurrTarget->flag &= ~QMS_TARGET_IS_UPDATABLE_MASK;
            sCurrTarget->flag |= QMS_TARGET_IS_UPDATABLE_TRUE;
        }
        else
        {
            sCurrTarget->flag &= ~QMS_TARGET_IS_UPDATABLE_MASK;
            sCurrTarget->flag |= QMS_TARGET_IS_UPDATABLE_FALSE;
        }

        // link
        if (sPrevTarget == NULL)
        {
            *aFirstTarget = sCurrTarget;
            sPrevTarget   = sCurrTarget;
        }
        else
        {
            sPrevTarget->next = sCurrTarget;
            sPrevTarget->targetColumn->node.next =
                (mtcNode *)(sCurrTarget->targetColumn);

            sPrevTarget       = sCurrTarget;
        }

        // PROJ-2469 Optimize View Materialization
        aSFWGH->currentTargetNum++;        
    }

    IDE_TEST_RAISE( *aFirstTarget == NULL, ERR_COLUMN_NOT_FOUND );

    *aLastTarget = sPrevTarget;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvQuerySet::makeTargetListForTableRef",
                                  "Column not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateQmsTableRef(
    qcStatement * aStatement,
    qmsSFWGH    * aSFWGH,
    qmsTableRef * aTableRef,
    UInt          aFlag,
    UInt          aIsNullableFlag ) // PR-13597
{
    qcDepInfo    sDepInfo;
    UInt         sCount;
    UInt         sNameLen = 0;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateQmsTableRef::__FT__" );

    // BUG-41311 table function
    if ( aTableRef->funcNode != NULL )
    {
        IDE_TEST( qmvTableFuncTransform::doTransform( aStatement, aTableRef )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    
    /* BUG-33691 PROJ-2188 Support pivot clause */
    if ( aTableRef->pivot != NULL )
    {
        IDE_TEST(qmvPivotTransform::doTransform( aStatement, aSFWGH, aTableRef )
                 != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2523 Unpivot clause */
    if ( aTableRef->unpivot != NULL )
    {
        IDE_TEST( qmvUnpivotTransform::doTransform( aStatement,
                                                    aSFWGH,
                                                    aTableRef )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if( aTableRef->view == NULL )
    {
        // PROJ-2582 recursive with
        if ( ( aTableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
             == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
        {
            // PROJ-2582 recursive with
            // recursive view인 경우
            IDE_TEST( validateRecursiveView( aStatement,
                                             aSFWGH,
                                             aTableRef,
                                             aIsNullableFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2418
            // View가 없다면 Lateral Flag를 초기화한다.
            aTableRef->flag &= ~QMS_TABLE_REF_LATERAL_VIEW_MASK;
            aTableRef->flag |= QMS_TABLE_REF_LATERAL_VIEW_FALSE;

            /* PROJ-1832 New database link */
            if ( aTableRef->remoteTable != NULL )
            {
                /* remote table */
                IDE_TEST( validateRemoteTable( aStatement,
                                               aTableRef )
                          != IDE_SUCCESS );
            }
            else
            {
                // 테이블인 경우
                IDE_TEST( validateTable(aStatement,
                                        aTableRef,
                                        aFlag,
                                        aIsNullableFlag)
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        // Create View 또는 Inline View인 경우
        IDE_TEST(validateView( aStatement, aSFWGH, aTableRef, aIsNullableFlag )
                 != IDE_SUCCESS);
    }

    // BUG-41230 SYS_USERS_ => DBA_USERS_
    if ( aTableRef->tableInfo->tableOwnerID == QC_SYSTEM_USER_ID )
    {
        if ( ( QCG_GET_SESSION_USER_ID(aStatement) == QC_SYS_USER_ID ) ||
             ( QCG_GET_SESSION_USER_ID(aStatement) == QC_SYSTEM_USER_ID ) )
        {
            // Nothing To Do
        }
        else
        {
            IDE_TEST_RAISE ( ( idlOS::strlen( aTableRef->tableInfo->name ) >= 4 ) &&
                             ( idlOS::strMatch( aTableRef->tableInfo->name,
                                                4,
                                                (SChar *)"DBA_",
                                                4 ) == 0 ),
                             ERR_NOT_EXIST_TABLE );
        }
    }
    else
    {
        // Nothing To Do
    }
    
    // PROJ-1413
    // tuple variable 등록
    if ( QC_IS_NULL_NAME( (aTableRef->aliasName) ) == ID_FALSE )
    {
        IDE_TEST( qtc::registerTupleVariable( aStatement,
                                              & aTableRef->aliasName )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-31570
     * DDL이 빈번한 환경에서 plan text를 안전하게 보여주는 방법이 필요하다.
     */
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( (QC_MAX_OBJECT_NAME_LEN + 1) *
                                             (aTableRef->tableInfo->columnCount),
                                             (void**)&( aTableRef->columnsName ) )
              != IDE_SUCCESS );

    for(sCount= 0; sCount < (aTableRef->tableInfo->columnCount); ++sCount)
    {
        idlOS::memcpy( aTableRef->columnsName[sCount],
                       aTableRef->tableInfo->columns[sCount].name,
                       (QC_MAX_OBJECT_NAME_LEN + 1) );
    }

    /* PROJ-1090 Function-based Index */
    if ( QCG_GET_SESSION_QUERY_REWRITE_ENABLE(aStatement) == 1 )
    {
        IDE_TEST( qmsDefaultExpr::makeNodeListForFunctionBasedIndex(
                      aStatement,
                      aTableRef,
                      &(aTableRef->defaultExprList) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-38983
    // DDL 이 빈번한 환경에서 plan text 의 userName (tableOwnerName) 을 안전하게 출력.
    if ( aTableRef->tableInfo != NULL )
    {
        sNameLen = idlOS::strlen( aTableRef->tableInfo->tableOwnerName );

        if ( sNameLen > 0 )
        {
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM(aStatement),
                                              SChar,
                                              sNameLen + 1,
                                              &(aTableRef->userName.stmtText) )
                      != IDE_SUCCESS );

            idlOS::memcpy( aTableRef->userName.stmtText,
                           aTableRef->tableInfo->tableOwnerName,
                           sNameLen );
            aTableRef->userName.stmtText[sNameLen] = '\0';
            aTableRef->userName.offset = 0;
            aTableRef->userName.size = sNameLen;
        }
        else
        {
            SET_EMPTY_POSITION( aTableRef->userName );
        }
    }
    else
    {
        // Nothing to do.
    }

    // set dependency
    if (aSFWGH != NULL)
    {
        qtc::dependencySet( aTableRef->table, & sDepInfo );
        IDE_TEST( qtc::dependencyOr( & aSFWGH->depInfo,
                                     & sDepInfo,
                                     & aSFWGH->depInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                                  aTableRef->tableInfo->name ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */ 

#define QUERY_PREFIX "SELECT * FROM "

/* Length of "SELECT * FROM " string is 14. */
#define QUERY_PREFIX_LENGTH (14)

/*
 * BUG-36967
 *
 * Copy and convert double single quotation('') to single quotation('). 
 */
void qmvQuerySet::copyRemoteQueryString( SChar * aDest, qcNamePosition * aSrc )
{
    SInt i = 0;
    SInt j = 0;
    SChar * sSrcString = NULL;

    sSrcString = aSrc->stmtText + aSrc->offset;

    for ( i = 0; i < aSrc->size; i++, j++ )
    {
        if ( sSrcString[i] == '\'' )
        {
            i++;
        }
        else
        {
            /* nothing to do */
        }
        aDest[j] = sSrcString[i];
    }
    aDest[j] = '\0';

}

IDE_RC qmvQuerySet::validateRemoteTable( qcStatement * aStatement,
                                         qmsTableRef * aTableRef )
{
    SInt sLength = 0;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateRemoteTable::__FT__" );

    if ( QC_IS_NULL_NAME( aTableRef->remoteTable->tableName ) != ID_TRUE )
    {
        sLength = QUERY_PREFIX_LENGTH;
        sLength += aTableRef->remoteTable->tableName.size + 1;
          
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc(
                      sLength,
                      (void **)&(aTableRef->remoteQuery) )
                  != IDE_SUCCESS);

        idlOS::memcpy( aTableRef->remoteQuery,
                       QUERY_PREFIX, QUERY_PREFIX_LENGTH );

        QC_STR_COPY( aTableRef->remoteQuery + QUERY_PREFIX_LENGTH, aTableRef->remoteTable->tableName );
    }
    else
    {
        sLength = aTableRef->remoteTable->remoteQuery.size + 1;

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc(
                      sLength,
                      (void **)&(aTableRef->remoteQuery) )
                  != IDE_SUCCESS);

        copyRemoteQueryString( aTableRef->remoteQuery,
                               &(aTableRef->remoteTable->remoteQuery) );
    }

    IDE_TEST( qmrGetRemoteTableMeta( aStatement, aTableRef )
              != IDE_SUCCESS );
    IDE_TEST( qcgPlan::registerPlanTable( aStatement,
                                          aTableRef->tableHandle,
                                          aTableRef->tableSCN,
                                          aTableRef->tableInfo->tableOwnerID, /* BUG-45893 */
                                          aTableRef->tableInfo->name )        /* BUG-45893 */
              != IDE_SUCCESS );

    IDE_TEST( qtc::nextTable( &(aTableRef->table ),
                              aStatement,
                              aTableRef->tableInfo,
                              ID_FALSE,
                              MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    /* emulate as disk table */
    QC_SHARED_TMPLATE( aStatement )->tmplate.rows[ aTableRef->table ].lflag
        &= ~MTC_TUPLE_STORAGE_MASK;
    QC_SHARED_TMPLATE( aStatement )->tmplate.rows[ aTableRef->table ].lflag
        |= MTC_TUPLE_STORAGE_DISK;

    /* BUG-36468 */
    QC_SHARED_TMPLATE( aStatement )->tmplate.rows[ aTableRef->table ].lflag
        &= ~MTC_TUPLE_STORAGE_LOCATION_MASK;
    QC_SHARED_TMPLATE( aStatement )->tmplate.rows[ aTableRef->table ].lflag
        |= MTC_TUPLE_STORAGE_LOCATION_REMOTE;
    
    /* no plan cache */
    QC_SHARED_TMPLATE( aStatement )->flag &= ~QC_TMP_PLAN_CACHE_IN_MASK;
    QC_SHARED_TMPLATE( aStatement )->flag |= QC_TMP_PLAN_CACHE_IN_OFF;

    /* BUG-42639 Monitoring query */
    QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
    QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmvQuerySet::validateTable(
    qcStatement * aStatement,
    qmsTableRef * aTableRef,
    UInt          aFlag,
    UInt          aIsNullableFlag)
{
    qcuSqlSourceInfo    sqlInfo;
    idBool              sExist = ID_FALSE;
    idBool              sIsFixedTable = ID_FALSE;
    void               *sTableHandle = NULL;
    qcmSynonymInfo      sSynonymInfo;
    UInt                sTableType;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateTable::__FT__" );

    IDE_TEST(
        qcmSynonym::resolveTableViewQueue(
            aStatement,
            aTableRef->userName,
            aTableRef->tableName,
            &(aTableRef->tableInfo),
            &(aTableRef->userID),
            &(aTableRef->tableSCN),
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
        sqlInfo.setSourceInfo(aStatement, &aTableRef->tableName);
        // 테이블이 존재하지 않음. 에러 발생시킴
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }

    // FixedTable or PerformanceView
    if( sIsFixedTable == ID_TRUE )
    {
        // PROJ-1350 - Plan Tree 자동 재구성
        // Execution 시점에 table handle의 접근을 허용하기 위하여
        // Table Meta Cache의 정보의 table handle정보를 설정한다.
        // 이는 Table Meta Cache정보가 DDL 등에 의해 변경될 수 있기
        // 때문이다.
        aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;
        aTableRef->tableType   = aTableRef->tableInfo->tableType;
        aTableRef->tableFlag   = aTableRef->tableInfo->tableFlag;
        aTableRef->tableOID    = aTableRef->tableInfo->tableOID;

        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_REF_FIXED_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_REF_FIXED_TABLE_TRUE;

        // BUG-45522 fixed table 을 가르키는 시노밈은 plan cache 에 등록해야 합니다.
        IDE_TEST( qcgPlan::registerPlanSynonym(
                      aStatement,
                      &sSynonymInfo,
                      aTableRef->userName,
                      aTableRef->tableName,
                      aTableRef->tableHandle,
                      NULL )
                  != IDE_SUCCESS );

        if ( aTableRef->tableInfo->tableType == QCM_DUMP_TABLE )
        {
            IDE_TEST( qcmDump::getDumpObjectInfo( aStatement, aTableRef )
                      != IDE_SUCCESS );

            // PROJ-1436
            // D$ table이 있으면 plan cache하지 않는다.
            // X$,V$,D$모두 질의참조 객체가 DB객체가 아니다.
            // 그러나 X$,V$는 객체변경이 없는반면,
            // D$는 객체변경이 발생한다.
            //따라서 D$는 Cache비대상입니다.
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_PLAN_CACHE_IN_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_PLAN_CACHE_IN_OFF;
        }
        else
        {
            /* Nothing to do */
        }

        /* BUG-42639 Monitoring query */
        if ( ( QCG_GET_SESSION_OPTIMIZER_PERFORMANCE_VIEW(aStatement) == 1 ) &&
             ( iduProperty::getQueryProfFlag() == 0 ) )
        {
            if ( QCM_IS_OPERATABLE_QP_FT_TRANS( aTableRef->tableInfo->operatableFlag )
                 == ID_FALSE )
            {
                QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
                QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;
        }
    }
    else  // 일반 테이블
    {
        // BUG-42639 Monitoring query
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;
        // 테이블이 존재하면 처리를 계속 진행함
        // BUG-34492
        // validation lock이면 충분하다.
        IDE_TEST(qcm::lockTableForDMLValidation(
                     aStatement,
                     sTableHandle,
                     aTableRef->tableSCN)
                 != IDE_SUCCESS);

        // CREATE VIEW 구문 수행 중 SELECT 안의 VIEW가 INVALID한 상태면
        //  에러를 발생시킨다.
        if ( ( (aFlag & QMV_VIEW_CREATION_MASK) == QMV_VIEW_CREATION_TRUE ) &&
             ( ( aTableRef->tableInfo->tableType == QCM_VIEW ) ||
               ( aTableRef->tableInfo->tableType == QCM_MVIEW_VIEW ) ) &&
             ( aTableRef->tableInfo->status == QCM_VIEW_INVALID ) )
        {
            sqlInfo.setSourceInfo(aStatement, &aTableRef->tableName);
            IDE_RAISE( ERR_INVALID_VIEW );
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-1350 - Plan Tree 자동 재구성
        // Execution 시점에 table handle의 접근을 허용하기 위하여
        // Table Meta Cache의 정보의 table handle정보를 설정한다.
        // 이는 Table Meta Cache정보가 DDL 등에 의해 변경될 수 있기
        // 때문이다.
        aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;
        aTableRef->tableType   = aTableRef->tableInfo->tableType;
        aTableRef->tableFlag   = aTableRef->tableInfo->tableFlag;
        aTableRef->tableOID    = aTableRef->tableInfo->tableOID;

        // PROJ-2646 shard analyzer enhancement
        if ( ( sdi::isShardCoordinator( aStatement ) == ID_TRUE ) ||
             ( sdi::isPartialCoordinator( aStatement ) == ID_TRUE ) ||
             ( sdi::detectShardMetaChange( aStatement ) == ID_TRUE ) )
        {
            IDE_TEST( sdi::getTableInfo( aStatement,
                                         aTableRef->tableInfo,
                                         &(aTableRef->mShardObjInfo) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanTable(
                      aStatement,
                      aTableRef->tableHandle,
                      aTableRef->tableSCN,
                      aTableRef->tableInfo->tableOwnerID, /* BUG-45893 */
                      aTableRef->tableInfo->name )        /* BUG-45893 */
                  != IDE_SUCCESS );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanSynonym(
                      aStatement,
                      & sSynonymInfo,
                      aTableRef->userName,
                      aTableRef->tableName,
                      aTableRef->tableHandle,
                      NULL )
                  != IDE_SUCCESS );

        // PROJ-1502 PARTITIONED DISK TABLE
        if ( aTableRef->partitionRef != NULL )
        {
            // 파티션을 지정한 경우임.
            if ( aTableRef->tableInfo->tablePartitionType !=
                 QCM_PARTITIONED_TABLE )
            {
                sqlInfo.setSourceInfo(aStatement,
                                      &aTableRef->tableName);
                // 파티션드 테이블이 아닌 경우. 에러 발생시킴
                IDE_RAISE( ERR_NOT_PART_TABLE );
            }
            else
            {
                /* nothing to do */
            }

            // 파티션드 테이블인 경우 파티션 정보 얻어옴
            IDE_TEST( qcmPartition::getPartitionInfo(
                          aStatement,
                          aTableRef->tableInfo->tableID,
                          aTableRef->partitionRef->partitionName,
                          &aTableRef->partitionRef->partitionInfo,
                          &aTableRef->partitionRef->partitionSCN,
                          &aTableRef->partitionRef->partitionHandle )
                      != IDE_SUCCESS );

            // BUG-34492
            // validation lock이면 충분하다.
            IDE_TEST( qmx::lockPartitionForDML( QC_SMI_STMT( aStatement ),
                                                aTableRef->partitionRef,
                                                SMI_TABLE_LOCK_IS )
                      != IDE_SUCCESS );

            // To fix BUG-17479
            IDE_TEST( qcmPartition::getPartitionOrder(
                          QC_SMI_STMT( aStatement ),
                          aTableRef->tableInfo->tableID,
                          (UChar*)(aTableRef->partitionRef->partitionName.stmtText +
                                   aTableRef->partitionRef->partitionName.offset),
                          aTableRef->partitionRef->partitionName.size,
                          & aTableRef->partitionRef->partOrder )
                      != IDE_SUCCESS );

            aTableRef->partitionRef->partitionOID =
                aTableRef->partitionRef->partitionInfo->tableOID;

            aTableRef->partitionRef->partitionID =
                aTableRef->partitionRef->partitionInfo->partitionID;

            aTableRef->selectedPartitionID =
                aTableRef->partitionRef->partitionID;

            // set flag
            // 선행 프루닝이 되었다는 flag세팅.
            aTableRef->flag &= ~QMS_TABLE_REF_PRE_PRUNING_MASK;
            aTableRef->flag |= QMS_TABLE_REF_PRE_PRUNING_TRUE;

        } // if( aTableRef->partitionRef != NULL )
        else
        {
            /* nothing to do */
        }

        // make index table reference
        if ( aTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST( qmsIndexTable::makeIndexTableRef(
                          aStatement,
                          aTableRef->tableInfo,
                          & aTableRef->indexTableRef,
                          & aTableRef->indexTableCount )
                      != IDE_SUCCESS );

            // BUG-34492
            // validation lock이면 충분하다.
            IDE_TEST( qmsIndexTable::validateAndLockIndexTableRefList( aStatement,
                                                                       aTableRef->indexTableRef,
                                                                       SMI_TABLE_LOCK_IS )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // shard table은 shard view에서만 참조할 수 있다.
        if ( ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                              SDI_QUERYSET_LIST_STATE_MAIN_ANALYZE_DONE )
               == ID_TRUE )
             &&
             ( aStatement->myPlan->parseTree->stmtShard == QC_STMT_SHARD_NONE )
             &&
             ( aTableRef->mShardObjInfo != NULL ) )
        {
            if ( aTableRef->mShardObjInfo->mIsLocalForce == ID_FALSE )
            {
                // shard transform 에러를 출력한다.
                IDE_TEST( qmvShardTransform::raiseInvalidShardQuery( aStatement ) != IDE_SUCCESS );

                sqlInfo.setSourceInfo( aStatement,
                                       &(aTableRef->tableName) );
                IDE_RAISE( ERR_EXIST_SHARD_TABLE_OUTSIDE_SHARD_VIEW );
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            // Nothing to do.
        }

        if( aStatement->spvEnv->createPkg != NULL )
        {
            IDE_TEST( qsvPkgStmts::makeRelatedObjects(
                          aStatement,
                          & aTableRef->userName,
                          & aTableRef->tableName,
                          & sSynonymInfo,
                          aTableRef->tableInfo->tableID,
                          QS_TABLE)
                      != IDE_SUCCESS );
        }
        else
        {
            if ( (aStatement->spvEnv->createProc != NULL) || // PSM
                 (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT) ) // VIEW
            {
                // search or make related object list
                IDE_TEST(qsvProcStmts::makeRelatedObjects(
                             aStatement,
                             & aTableRef->userName,
                             & aTableRef->tableName,
                             & sSynonymInfo,
                             aTableRef->tableInfo->tableID,
                             QS_TABLE )
                         != IDE_SUCCESS);
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    // set alias name
    if (QC_IS_NULL_NAME(aTableRef->aliasName) == ID_TRUE)
    {
        SET_POSITION( aTableRef->aliasName, aTableRef->tableName );
    }
    
    /* PROJ-2441 flashback */
    if( sIsFixedTable == ID_TRUE ) // BUG-16651
    {
        if( smiFixedTable::canUse( aTableRef->tableInfo->tableHandle )
            == ID_FALSE &&
            ((aFlag & QMV_PERFORMANCE_VIEW_CREATION_MASK) ==
             QMV_PERFORMANCE_VIEW_CREATION_FALSE ) &&
            ((aFlag & QMV_VIEW_CREATION_MASK) ==
             QMV_VIEW_CREATION_FALSE )  )
        {
            sqlInfo.setSourceInfo(
                aStatement,
                &aTableRef->tableName);
            IDE_RAISE( ERR_DML_ON_CLOSED_TABLE );
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

    // add tuple set
    IDE_TEST(qtc::nextTable(
                 &(aTableRef->table),
                 aStatement,
                 aTableRef->tableInfo,
                 QCM_TABLE_TYPE_IS_DISK(aTableRef->tableInfo->tableFlag),
                 aIsNullableFlag ) // PR-13597
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( aTableRef->tableInfo->tablePartitionType ==
        QCM_PARTITIONED_TABLE )
    {
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table].lflag
            &= ~MTC_TUPLE_PARTITIONED_TABLE_MASK;

        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table].lflag
            |= MTC_TUPLE_PARTITIONED_TABLE_TRUE;
    }

    IDE_TEST_RAISE(
        ((aTableRef->tableInfo->tableType != QCM_QUEUE_TABLE) &&
         (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE)),
        ERR_DEQUEUE_ON_TABLE);

    aTableRef->mParallelDegree = IDL_MIN(aTableRef->tableInfo->parallelDegree,
                                         QMV_QUERYSET_PARALLEL_DEGREE_LIMIT);

    IDE_DASSERT( aTableRef->mParallelDegree != 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_SHARD_TABLE_OUTSIDE_SHARD_VIEW)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(sdERR_ABORT_EXIST_SHARD_TABLE_OUTSIDE_SHARD_VIEW,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_DML_ON_CLOSED_TABLE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_DML_ON_CLOSED_TABLE,
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
    IDE_EXCEPTION(ERR_DEQUEUE_ON_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_TABLE_DEQUEUE_DENIED));
    }
    IDE_EXCEPTION(ERR_NOT_PART_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_PARTITIONED_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
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

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateView(
    qcStatement * aStatement,
    qmsSFWGH    * aSFWGH,
    qmsTableRef * aTableRef,
    UInt          aIsNullableFlag)
{
    qmsParseTree      * sParseTree;
    qmsQuerySet       * sQuerySet;
    volatile UInt       sSessionUserID;   // for fixing BUG-6096
    qcuSqlSourceInfo    sqlInfo;
    idBool              sExist = ID_FALSE;
    idBool              sIsFixedTable = ID_FALSE;
    volatile idBool     sIndirectRef; /* BUG-45994 - 컴파일러 최적화 회피 */
    idBool              sIsShardView  = ID_FALSE;
    void              * sTableHandle  = NULL;
    qmsQuerySet       * sLateralViewQuerySet = NULL;
    qcmColumn         * sColumnAlias = NULL;
    qcmSynonymInfo      sSynonymInfo;
    volatile idBool     sCalledByPSMFlag;
    UInt                sTableType;
    UInt                i;
    UInt                sDepTupleID;
    qmsTableRef       * sTableRef = NULL;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateView::__FT__" );

    idlOS::memset( &sSynonymInfo, 0x00, ID_SIZEOF( qcmSynonymInfo ) );
    sSynonymInfo.isSynonymName = ID_FALSE;

    sCalledByPSMFlag = aTableRef->view->calledByPSMFlag;
    sIndirectRef     = ID_FALSE; /* BUG-45994 - 컴파일러 최적화 회피 */

    // To Fix PR-1176
    sSessionUserID = QCG_GET_SESSION_USER_ID( aStatement );

    // A parse tree of real view is set in parser.
    IDE_TEST_RAISE(aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE,
                   ERR_DEQUEUE_ON_TABLE);
    
    /* BUG-46124 */
    IDE_TEST( qmsPreservedTable::checkAndSetPreservedInfo( aSFWGH,
                                                           aTableRef )
              != IDE_SUCCESS );

    // get aTableRef->tableInfo for CREATED VIEW
    // PROJ-2083 DUAL Table
    if ( ( aTableRef->flag & QMS_TABLE_REF_CREATED_VIEW_MASK )
         == QMS_TABLE_REF_CREATED_VIEW_TRUE )
    {
        IDE_TEST(
            qcmSynonym::resolveTableViewQueue(
                aStatement,
                aTableRef->userName,
                aTableRef->tableName,
                &(aTableRef->tableInfo),
                &(aTableRef->userID),
                &(aTableRef->tableSCN),
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
            sqlInfo.setSourceInfo( aStatement,
                                   & aTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
        
        if( sIsFixedTable == ID_TRUE )  // BUG-16651
        {
            // userID is set in qmv::parseViewInFromClause
            // BUG-17452
            aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;
            aTableRef->tableType   = aTableRef->tableInfo->tableType;
            aTableRef->tableFlag   = aTableRef->tableInfo->tableFlag;
            aTableRef->tableOID    = aTableRef->tableInfo->tableOID;

            // BUG-45522 fixed table 을 가르키는 시노밈은 plan cache 에 등록해야 합니다.
            IDE_TEST( qcgPlan::registerPlanSynonym( aStatement,
                                                    &sSynonymInfo,
                                                    aTableRef->userName,
                                                    aTableRef->tableName,
                                                    aTableRef->tableHandle,
                                                    NULL )
                      != IDE_SUCCESS );

            if (QC_IS_NULL_NAME(aTableRef->aliasName) == ID_TRUE)
            {
                SET_POSITION( aTableRef->aliasName, aTableRef->tableName );
            }

            // set aTableRef->sameViewRef
            IDE_TEST( setSameViewRef(aStatement, aSFWGH, aTableRef)
                      != IDE_SUCCESS );
        }
        else
        {
            /* BUG-42639 Monitoring query
             * 일반 View일 경우 recreate 가 될수 있어서 Lock을 잡아야함으로
             * Transaction이 필요하다
             */
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;

            // BUG-34492
            // validation lock이면 충분하다.
            IDE_TEST( qcm::lockTableForDMLValidation(
                          aStatement,
                          sTableHandle,
                          aTableRef->tableSCN )
                      != IDE_SUCCESS );

            // BUG-17452
            aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;
            aTableRef->tableType   = aTableRef->tableInfo->tableType;
            aTableRef->tableFlag   = aTableRef->tableInfo->tableFlag;
            aTableRef->tableOID    = aTableRef->tableInfo->tableOID;

            // environment의 기록
            IDE_TEST( qcgPlan::registerPlanTable( aStatement,
                                                  aTableRef->tableHandle,
                                                  aTableRef->tableSCN,
                                                  aTableRef->tableInfo->tableOwnerID, /* BUG-45893 */
                                                  aTableRef->tableInfo->name )        /* BUG-45893 */
                      != IDE_SUCCESS );
                
            // environment의 기록
            IDE_TEST( qcgPlan::registerPlanSynonym( aStatement,
                                                    & sSynonymInfo,
                                                    aTableRef->userName,
                                                    aTableRef->tableName,
                                                    aTableRef->tableHandle,
                                                    NULL )
                      != IDE_SUCCESS );
                    
            if (QC_IS_NULL_NAME(aTableRef->aliasName) == ID_TRUE)
            {
                SET_POSITION( aTableRef->aliasName, aTableRef->tableName );
            }

            // set aTableRef->sameViewRef
            IDE_TEST( setSameViewRef(aStatement, aSFWGH, aTableRef)
                      != IDE_SUCCESS );
        }
    }

    // PROJ-1889 INSTEAD OF TRIGGER : select 외에 DML도 올수 있다.
    // select 일경우만 forUpdate를 할당한다.
    if (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT )
    {
        // set forUpdate ptr
        sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

        ((qmsParseTree *)(aTableRef->view->myPlan->parseTree))->forUpdate =
            sParseTree->forUpdate;
    }
    
    // set member of qcStatement
    aTableRef->view->myPlan->planEnv = aStatement->myPlan->planEnv;
    aTableRef->view->spvEnv          = aStatement->spvEnv;
    aTableRef->view->mRandomPlanInfo = aStatement->mRandomPlanInfo;

    aTableRef->view->mFlag &= ~QC_STMT_VIEW_MASK;
    aTableRef->view->mFlag |= QC_STMT_VIEW_TRUE;

    /* PROJ-2197 PSM Renewal
     * view의 target에 있는 PSM 변수를 host 변수로 사용하는 경우
     * 강제로 casting 하기 위해서 flag를 변경한다.
     * (qmvQuerySet::addCastOper 에서 사용한다.) */
    aTableRef->view->calledByPSMFlag = aStatement->calledByPSMFlag;

    if (aTableRef->tableInfo != NULL)
    {
        //-----------------------------------------
        // CREATED VIEW
        //-----------------------------------------
        if ( ( aTableRef->tableInfo->tableType == QCM_VIEW ) ||
             ( aTableRef->tableInfo->tableType == QCM_MVIEW_VIEW ) )
        {
            // for fixing BUG-6096
            // get current session userID
            // sSessionUserID = aStatement->session->userInfo->userID;
            // modify session userID
            QCG_SET_SESSION_USER_ID( aStatement, aTableRef->userID );

            // PROJ-1436
            // environment의 기록시 간접 참조 객체에 대한 user나
            // privilege의 기록을 중지한다.
            qcgPlan::startIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );

            /* BUG-45994 */
            IDU_FIT_POINT_FATAL( "qmvQuerySet::validateView::__FT__::STAGE1" );

            /* TASK-7219 Shard Transformer Refactoring
             *  PROJ-2646 shard analyzer
             *   shard view의 하위 statement에서는 shard table이 올 수 있다.
             */
            IDE_TEST( sdi::setShardStmtType( aStatement,
                                             aTableRef->view )
                      != IDE_SUCCESS );

            // inline view validation
            IDE_TEST(qmv::validateSelect(aTableRef->view) != IDE_SUCCESS);

            // for fixing BUG-6096
            // re-set current session userID
            QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

            qcgPlan::endIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );

            if( aStatement->spvEnv->createPkg != NULL )
            {
                IDE_TEST( qsvPkgStmts::makeRelatedObjects(
                              aStatement,
                              & aTableRef->userName,
                              & aTableRef->tableName,
                              & sSynonymInfo,
                              aTableRef->tableInfo->tableID,
                              QS_TABLE)
                          != IDE_SUCCESS );
            }
            else 
            {
                if( (aStatement->spvEnv->createProc != NULL) || // PSM
                    (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT) ) // VIEW
                {
                    // search or make related object list
                    IDE_TEST(qsvProcStmts::makeRelatedObjects(
                                 aStatement,
                                 & aTableRef->userName,
                                 & aTableRef->tableName,
                                 & sSynonymInfo,
                                 aTableRef->tableInfo->tableID,
                                 QS_TABLE )
                             != IDE_SUCCESS);
                }
                else
                {
                    // Nothing to do.
                } 
            }
        }

        else if( aTableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW )
        {
            // for fixing BUG-6096
            // get current session userID
            // sSessionUserID = aStatement->session->userInfo->userID;
            // modify session userID
            QCG_SET_SESSION_USER_ID( aStatement, aTableRef->userID );

            // PROJ-1436
            // environment의 기록시 간접 참조 객체에 대한 user나
            // privilege의 기록을 중지한다.
            qcgPlan::startIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );

            /* BUG-45994 */
            IDU_FIT_POINT_FATAL( "qmvQuerySet::validateView::__FT__::STAGE1" );

            /* TASK-7219 Shard Transformer Refactoring
             *  PROJ-2646 shard analyzer
             *   shard view의 하위 statement에서는 shard table이 올 수 있다.
             */
            IDE_TEST( sdi::setShardStmtType( aStatement,
                                             aTableRef->view )
                      != IDE_SUCCESS );

            // inline view validation
            // BUG-34390
            if ( qmv::validateSelect(aTableRef->view) != IDE_SUCCESS )
            {
                if ( ( ( aTableRef->flag & QMS_TABLE_REF_CREATED_VIEW_MASK )
                       == QMS_TABLE_REF_CREATED_VIEW_TRUE ) &&
                     ( ideGetErrorCode() == qpERR_ABORT_QMV_DML_ON_CLOSED_TABLE ) )
                {
                    sqlInfo.setSourceInfo( aStatement, & aTableRef->tableName );
                    IDE_RAISE( ERR_DML_ON_CLOSED_TABLE );
                }
                else
                {
                    IDE_TEST(1);
                }
            }
            else
            {
                // Nothing to do.
            }

            // for fixing BUG-6096
            // re-set current session userID
            QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

            qcgPlan::endIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aTableRef->tableName );
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }

        // PROJ-2418
        // Inline View가 아니므로 Lateral Flag를 초기화한다.
        aTableRef->flag &= ~QMS_TABLE_REF_LATERAL_VIEW_MASK;
        aTableRef->flag |= QMS_TABLE_REF_LATERAL_VIEW_FALSE;
    }
    else
    {
        //-----------------------------------------
        // inline view
        //-----------------------------------------
        sQuerySet = ( ( qmsParseTree* )( aTableRef->view->myPlan->parseTree ) )->querySet;

        /* insert aSFWGH == NULL 이다. */
        if ( aSFWGH != NULL )
        {
            // To fix BUG-24213
            // performance view creation flag를 전달
            sQuerySet->lflag
                &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
            sQuerySet->lflag
                |= (aSFWGH->lflag & QMV_PERFORMANCE_VIEW_CREATION_MASK);
            sQuerySet->lflag
                &= ~(QMV_VIEW_CREATION_MASK);
            sQuerySet->lflag
                |= (aSFWGH->lflag & QMV_VIEW_CREATION_MASK);

            // PROJ-2415 Grouping Sets Clause
            // Transform에 의해 생성된 View가 Dependency를 가질 수 있도록 Outer Query를 세팅
            if ( ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                   == QMV_SFWGH_GBGS_TRANSFORM_TOP ) ||
                 ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                   == QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) )
            {
                sQuerySet->outerSFWGH = aSFWGH;
            }
            else
            {
                /* Nothing to do */
            }

            if ( ( ( aSFWGH->thisQuerySet->lflag & QMV_QUERYSET_LATERAL_MASK )
                   == QMV_QUERYSET_LATERAL_TRUE ) &&
                 ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                   != QMV_SFWGH_GBGS_TRANSFORM_NONE ) )
            {
                // BUG-41573 Lateral View WITH GROUPING SETS
                // 상위가 Lateral View라면 GBGS로 인해 파생된 Inline View는
                // 모두 Lateral View가 되어야 한다.
                aTableRef->flag &= ~QMS_TABLE_REF_LATERAL_VIEW_MASK;
                aTableRef->flag |= QMS_TABLE_REF_LATERAL_VIEW_TRUE;
            }
        }
        else
        {
            /* Nothing To Do */
        }
        
        // PROJ-2418
        if ( ( aTableRef->flag & QMS_TABLE_REF_LATERAL_VIEW_MASK ) == 
             QMS_TABLE_REF_LATERAL_VIEW_TRUE )
        {
            if ( aSFWGH != NULL )
            {
                sLateralViewQuerySet =
                    ((qmsParseTree*) aTableRef->view->myPlan->parseTree)->querySet;

                // Lateral View SFWGH의 outerQuery / outerFrom 지정
                setLateralOuterQueryAndFrom( sLateralViewQuerySet,
                                             aTableRef,
                                             aSFWGH );

                // BUG-41573 Lateral View Flag 지정
                sLateralViewQuerySet->lflag &= ~QMV_QUERYSET_LATERAL_MASK;
                sLateralViewQuerySet->lflag |= QMV_QUERYSET_LATERAL_TRUE;
            }
            else
            {
                /*******************************************************************
                 *
                 *  INSERT ... SELECT 에서는 아래와 같이 된다.
                 *  - aTableRef는 SELECT Main Query
                 *  - aSFWGH (TableRef 관점에서의 외부 SFWGH)는 NULL
                 *
                 *  그러나 SELECT Main Query는 LATERAL Flag을 가질 수 없으므로
                 *  aSFWGH가 NULL이면서 aTableRef가 LATERAL Flag를 가지는 경우는 없다.
                 *
                 *******************************************************************/
                IDE_DASSERT(0);
            }
        }
        else
        {
            // Lateral View가 아닌 경우
            // Nothing to do.
        }

        /* TASK-7219 Shard Transformer Refactoring
         *  PROJ-2646 shard analyzer
         *   shard view의 하위 statement에서는 shard table이 올 수 있다.
         */
        IDE_TEST( sdi::setShardStmtType( aStatement,
                                         aTableRef->view )
                  != IDE_SUCCESS );

        // inline view validation
        IDE_TEST(qmv::validateSelect(aTableRef->view) != IDE_SUCCESS);

        /* PROJ-2641 Hierarchy Query Index */
        if ( ( ( aTableRef->flag & QMS_TABLE_REF_HIER_VIEW_MASK )
               == QMS_TABLE_REF_HIER_VIEW_TRUE ) &&
             ( ( aStatement->myPlan->parseTree->stmtShard == QC_STMT_SHARD_NONE ) ||
               ( aStatement->myPlan->parseTree->stmtShard == QC_STMT_SHARD_META ) ) )
        {
            sParseTree = (qmsParseTree*) aTableRef->view->myPlan->parseTree;

            sTableRef = sParseTree->querySet->SFWGH->from->tableRef;

            sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

            // PROJ-2654 shard view 는 배제한다.
            if ( sTableRef->view != NULL )
            {
                if ( ( sTableRef->view->myPlan->parseTree->stmtShard != QC_STMT_SHARD_NONE ) &&
                     ( sTableRef->view->myPlan->parseTree->stmtShard != QC_STMT_SHARD_META ) )
                {
                    sIsShardView = ID_TRUE;
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

            /* Hierarchy Query는 무조건 select * from t1 --> select * from (
             * select * from t1 )으로 transform 해서 처리하는데 일반 테이블 과
             * dual table일  경우에 대해서 transform된 view를 없애고 하위
             * tableRef 를 현재로 복사한다
             */

            if ( ( sTableRef->remoteTable == NULL ) &&
                 ( sTableRef->tableInfo->temporaryInfo.type == QCM_TEMPORARY_ON_COMMIT_NONE ) &&
                 ( sTableRef->tableInfo->tablePartitionType != QCM_PARTITIONED_TABLE ) &&
                 ( sTableRef->tableInfo->tableType != QCM_DUMP_TABLE ) &&
                 ( sTableRef->tableInfo->tableType != QCM_PERFORMANCE_VIEW ) &&
                 ( sParseTree->isSiblings == ID_FALSE ) &&
                 ( sIsShardView == ID_FALSE ) &&
                 ( QCU_OPTIMIZER_HIERARCHY_TRANSFORMATION == 0 ) )
            {
                if ( sTableRef->tableInfo->tableType == QCM_FIXED_TABLE )
                {
                    if ( ( ( sTableRef->tableInfo->tableOwnerID == QC_SYSTEM_USER_ID ) &&
                           ( idlOS::strMatch( sTableRef->tableInfo->name,
                                              idlOS::strlen(sTableRef->tableInfo->name),
                                              "SYS_DUMMY_",
                                              10 ) == 0 ) ) ||
                         ( idlOS::strMatch( sTableRef->tableInfo->name,
                                            idlOS::strlen(sTableRef->tableInfo->name),
                                            "X$DUAL",
                                            6 ) == 0 ) )
                    {
                        sTableRef->pivot     = aTableRef->pivot;
                        sTableRef->unpivot   = aTableRef->unpivot;
                        sTableRef->aliasName = aTableRef->aliasName;
                        idlOS::memcpy( aTableRef, sTableRef, sizeof( qmsTableRef ) );

                        IDE_CONT( NORMAL_EXIT );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    sTableRef->pivot     = aTableRef->pivot;
                    sTableRef->unpivot   = aTableRef->unpivot;
                    sTableRef->aliasName = aTableRef->aliasName;
                    idlOS::memcpy( aTableRef, sTableRef, sizeof( qmsTableRef ) );

                    IDE_CONT( NORMAL_EXIT );
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

        // PROJ-2418 Lateral View 후처리
        if ( sLateralViewQuerySet != NULL )
        {
            // View QuerySet에 outerDepInfo가 존재하는 경우에는
            // lateralDepInfo에 outerDepInfo를 ORing 한다.
            IDE_TEST( qmvQTC::setLateralDependenciesLast( sLateralViewQuerySet )
                      != IDE_SUCCESS );
                      
            if ( qtc::haveDependencies( & sLateralViewQuerySet->lateralDepInfo ) == ID_FALSE )
            {
                // LATERAL / APPLY Keyword는 있지만 실제로 Lateral View는 아닌 경우
                // Flag를 원상 복구한다.
                aTableRef->flag &= ~QMS_TABLE_REF_LATERAL_VIEW_MASK;
                aTableRef->flag |= QMS_TABLE_REF_LATERAL_VIEW_FALSE;
            }
            else
            {
                // 실제 Lateral View가 정의된 경우
                // BUG-43705 lateral view를 simple view merging을 하지않으면 결과가 다릅니다.
                for ( i = 0; i < sLateralViewQuerySet->lateralDepInfo.depCount; i++ )
                {
                    sDepTupleID = sLateralViewQuerySet->lateralDepInfo.depend[i];
                    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sDepTupleID].lflag |= MTC_TUPLE_LATERAL_VIEW_REF_TRUE;
                }
            }
        }
        else
        {
            // LATERAL / APPLY Keyword가 없는 View
            // Nothing to do.
        }

        // PROJ-2415 Grouping Sets Clause 
        // Tranform에 의해 생성된 View의 Outer Dependency를 자신의 Outer Dependency에 Or한다.
        if ( ( aSFWGH != NULL ) &&
             ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
               != QMV_SFWGH_GBGS_TRANSFORM_NONE ) )
        {
            IDE_TEST( qtc::dependencyOr( & sQuerySet->outerDepInfo,
                                         & aSFWGH->outerDepInfo,
                                         & aSFWGH->outerDepInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        /* TASK-7217 Sharded sequence
           Shard transform에 의해서 sequence가 shard view안으로 들어가는 경우는 허용한다. */
        if ( (aTableRef->view->myPlan->parseTree->stmtShard != QC_STMT_SHARD_DATA) &&
             (((qmsParseTree*)aTableRef->view->myPlan->parseTree)->isShardView == ID_FALSE) &&
             (aTableRef->view->myPlan->mShardAnalysis == NULL) )
        {
            if (aTableRef->view->myPlan->parseTree->currValSeqs != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & aTableRef->view->myPlan->parseTree->
                    currValSeqs->sequenceNode->position );
                IDE_RAISE( ERR_USE_SEQUENCE_IN_VIEW );
            }

            if (aTableRef->view->myPlan->parseTree->nextValSeqs != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & aTableRef->view->myPlan->parseTree->
                    nextValSeqs->sequenceNode->position );
                IDE_RAISE( ERR_USE_SEQUENCE_IN_VIEW );
            }
        }

        // set USER ID
        aTableRef->userID = QCG_GET_SESSION_USER_ID(aStatement);

        // PROJ-2582 recursive with
        // with절의 view이고 column alias가 있는 경우 컬럼이름은 column alias를 이용한다.
        if ( aTableRef->withStmt != NULL )
        {
            sColumnAlias = aTableRef->withStmt->columns;
        }
        else
        {
            // Nothing to do.
        }
        
        // make qcmTableInfo for IN-LINE VIEW
        IDE_TEST(makeTableInfo(
                     aStatement,
                     ((qmsParseTree *)aTableRef->view->myPlan->parseTree)->querySet,
                     sColumnAlias,
                     &aTableRef->tableInfo,
                     QS_EMPTY_OID)
                 != IDE_SUCCESS);

        // BUG-37136
        aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;  // NULL이다
        aTableRef->tableType   = aTableRef->tableInfo->tableType;
        aTableRef->tableFlag   = aTableRef->tableInfo->tableFlag;
        aTableRef->tableOID    = aTableRef->tableInfo->tableOID;

        // with 절에 의해 생성된 inline 뷰에 경우 제너레이트된 tableID를 할당한다.
        if ( aTableRef->withStmt != NULL )
        {
            aTableRef->tableInfo->tableID = aTableRef->withStmt->tableID;
        }
        else
        {
            aTableRef->tableInfo->tableID = 0;
        }

        // set aTableRef->sameViewRef
        IDE_TEST( setSameViewRef( aStatement, aSFWGH, aTableRef )
                  != IDE_SUCCESS );
    }

    // add tuple set
    IDE_TEST(qtc::nextTable( &(aTableRef->table), aStatement, NULL, ID_TRUE, aIsNullableFlag ) // PR-13597
             != IDE_SUCCESS);

    // make mtcColumn and mtcExecute for inline view tuple
    IDE_TEST(makeTupleForInlineView(aStatement,
                                    aTableRef,
                                    aTableRef->table,
                                    aIsNullableFlag )
             != IDE_SUCCESS);

    aTableRef->view->calledByPSMFlag = sCalledByPSMFlag;

    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DEQUEUE_ON_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_TABLE_DEQUEUE_DENIED));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_SEQUENCE_IN_VIEW)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_IN_VIEW,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DML_ON_CLOSED_TABLE )
    {
        // BUG-34390
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_DML_ON_CLOSED_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    // To Fix PR-11776
    // View 에 대한 Validation error 발생 시
    // 원래의 User ID로 복원해 주어야 한다.
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    qcgPlan::endIndirectRefFlag( aStatement, (idBool *) & sIndirectRef );

    aTableRef->view->calledByPSMFlag = sCalledByPSMFlag;

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateInlineView( qcStatement * aStatement,
                                        qmsSFWGH    * aSFWGH,
                                        qmsTableRef * aTableRef,
                                        UInt          aIsNullableFlag )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     Subquery에서 transform된 inline view를 위해 기존 validateView에서
 *     inline view에 대한 logic을 분리한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateInlineView::__FT__" );

    aTableRef->userID = QCG_GET_SESSION_USER_ID(aStatement);

    IDE_TEST(makeTableInfo(
                 aStatement,
                 ((qmsParseTree *)aTableRef->view->myPlan->parseTree)->querySet,
                 NULL,
                 &aTableRef->tableInfo,
                 QS_EMPTY_OID)
             != IDE_SUCCESS);

    // BUG-38264
    aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;
    aTableRef->tableType   = aTableRef->tableInfo->tableType;

    if ( aTableRef->withStmt != NULL )
    {
        aTableRef->tableInfo->tableID = aTableRef->withStmt->tableID;
    }
    else
    {
        aTableRef->tableInfo->tableID = 0;
    }

    IDE_TEST( setSameViewRef( aStatement, aSFWGH, aTableRef )
              != IDE_SUCCESS );

    IDE_TEST(qtc::nextTable( &(aTableRef->table), aStatement, NULL, ID_TRUE, aIsNullableFlag )
             != IDE_SUCCESS);

    IDE_TEST(makeTupleForInlineView(aStatement,
                                    aTableRef,
                                    aTableRef->table,
                                    aIsNullableFlag )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateRecursiveView( qcStatement * aStatement,
                                           qmsSFWGH    * aSFWGH,
                                           qmsTableRef * aTableRef,
                                           UInt          aIsNullableFlag )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *
 * Implementation :
 * 
 *     with q1(i1,pid) as (
 *       select id, pid from t1
 *       union all
 *       select q1.id, '/'||q1.pid from q1, t1 where q1.pid = t1.id
 *                                      ^^
 *                                      하위 recursive view
 *     )
 *     select * from q1;
 *                   ^^
 *                   최상위 recursive view
 *
 *    q1의 i1은 union all의 left subquery의 target column type으로 고정하되
 *    precision은 right subquery의 target에 의해 커질 수 있다.
 *
 ***********************************************************************/

    qmsParseTree      * sParseTree = NULL;
    qmsWithClause     * sWithClause = NULL;
    qcmColumn         * sColumnAlias = NULL;
    idBool              sIsFound = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateRecursiveView::__FT__" );
    
    IDE_DASSERT( aTableRef->withStmt != NULL );

    sParseTree = (qmsParseTree*)aStatement->myPlan->parseTree;
    
    // set USER ID
    aTableRef->userID = QCG_GET_SESSION_USER_ID(aStatement);
    
    // with절의 view이고 column alias가 있는 경우 컬럼이름은 column alias를 이용한다.
    sColumnAlias = aTableRef->withStmt->columns;

    // query_name에 alias 존재 해야 한다.
    IDE_TEST_RAISE( sColumnAlias == NULL, ERR_NO_COLUMN_ALIAS_RECURSIVE_VIEW );

    // query_name 중복 체크
    for ( sWithClause = sParseTree->withClause;
          sWithClause != NULL;
          sWithClause = sWithClause->next )
    {
        if ( QC_IS_NAME_MATCHED( aTableRef->tableName,
                                 sWithClause->stmtName ) == ID_TRUE )
        {
            IDE_TEST_RAISE( sIsFound == ID_TRUE,
                            ERR_DUPLICATE_QUERY_NAME_RECURSIVE_VIEW );
            
            sIsFound = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    
    if ( aTableRef->recursiveView != NULL )
    {
        IDE_TEST( validateTopRecursiveView( aStatement,
                                            aSFWGH,
                                            aTableRef,
                                            aIsNullableFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( validateBottomRecursiveView( aStatement,
                                               aSFWGH,
                                               aTableRef,
                                               aIsNullableFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_COLUMN_ALIAS_RECURSIVE_VIEW )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMV_NO_COLUMN_ALIAS_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION( ERR_DUPLICATE_QUERY_NAME_RECURSIVE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_DUPLICATION_QUERY_NAME_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateTopRecursiveView( qcStatement * aStatement,
                                              qmsSFWGH    * aSFWGH,
                                              qmsTableRef * aTableRef,
                                              UInt          aIsNullableFlag )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *    상위recrusive view valdiate.
 *
 * Implementation :
 * 
 *     with q1(i1,pid) as (
 *       select id, pid from t1
 *       union all
 *       select q1.id, '/'||q1.pid from q1, t1 where q1.pid = t1.id
 *                                      ^^
 *                                      하위 recursive view
 *     )
 *     select * from q1;
 *                   ^^
 *                   최상위 recursive view
 *
 *    q1의 i1은 union all의 left subquery의 target column type으로 고정하되
 *    precision은 right subquery의 target에 의해 커질 수 있다.
 *
 ***********************************************************************/
    
    qmsParseTree      * sWithParseTree = NULL;
    qmsParseTree      * sViewParseTree = NULL;
    qcmColumn         * sColumnAlias = NULL;
    qcmColumn         * sColumnInfo = NULL;
    qmsTarget         * sWithTarget = NULL;
    qmsTarget         * sViewTarget = NULL;
    qmsTarget         * sTarget = NULL;
    mtcColumn         * sMtcColumn = NULL;
    qtcNode           * sTargetColumn = NULL;
    qtcNode           * sNode = NULL;
    qtcNode           * sPrevNode = NULL;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateTopRecursiveView::__FT__" );
    
    IDE_DASSERT( aTableRef->withStmt != NULL );
    
    sWithParseTree = (qmsParseTree *)aTableRef->withStmt->stmt->myPlan->parseTree;
    
    // with절의 view이고 column alias가 있는 경우 컬럼이름은 column alias를 이용한다.
    sColumnAlias = aTableRef->withStmt->columns;

    //------------------------------
    // 최상위 recursive view
    // 본 쿼리에서 사용할 recursiveView (parsing만 되어 있다.)
    //------------------------------
        
    // 다시 validation을 수행하기 위해 임시 tableInfo를 생성한다.
    IDE_TEST( makeTableInfo( aStatement,
                             sWithParseTree->querySet,
                             sColumnAlias,
                             &aTableRef->tableInfo,
                             QS_EMPTY_OID )
              != IDE_SUCCESS );
    
    // BUG-43768 recursive with 권한 체크 에러
    aTableRef->tableInfo->tableOwnerID = QCG_GET_SESSION_USER_ID(aStatement);        

    // BUG-44156 임시 tableInfo에 tableID설정
    aTableRef->tableInfo->tableID = aTableRef->withStmt->tableID;
    
    aTableRef->withStmt->tableInfo = aTableRef->tableInfo;

    //------------------------------
    // 먼저 recursive view의 target column이 발산하는지 혹은
    // 수렴하는지 알아보고 tableInfo를 변경한다.
    //------------------------------
        
    sViewParseTree = (qmsParseTree*) aTableRef->tempRecursiveView->myPlan->parseTree;
        
    sViewParseTree->querySet->lflag &= ~QMV_QUERYSET_RECURSIVE_VIEW_MASK;
    sViewParseTree->querySet->lflag |= QMV_QUERYSET_RECURSIVE_VIEW_TOP;

    IDE_TEST( qmv::validateSelect( aTableRef->tempRecursiveView )
              != IDE_SUCCESS );

    // tableInfo 컬럼의 precision을 보정한다.
    for ( sColumnInfo = aTableRef->tableInfo->columns,
              sWithTarget = sWithParseTree->querySet->target,
              sViewTarget = sViewParseTree->querySet->target;
          ( sColumnInfo != NULL ) &&
              ( sWithTarget != NULL ) &&
              ( sViewTarget != NULL );
          sColumnInfo = sColumnInfo->next,
              sWithTarget = sWithTarget->next,
              sViewTarget = sViewTarget->next )
    {
        IDE_TEST( qtc::makeRecursiveTargetInfo( aStatement,
                                                sWithTarget->targetColumn,
                                                sViewTarget->targetColumn,
                                                sColumnInfo )
                  != IDE_SUCCESS );
    }

    // 이미 aTableRef->withStmt->tableInfo에 달려있다.
        
    //------------------------------
    // 변경된 tableInfo로 다시 validation을 수행한다.
    //------------------------------
        
    sViewParseTree = (qmsParseTree*) aTableRef->recursiveView->myPlan->parseTree;
        
    sViewParseTree->querySet->lflag &= ~QMV_QUERYSET_RECURSIVE_VIEW_MASK;
    sViewParseTree->querySet->lflag |= QMV_QUERYSET_RECURSIVE_VIEW_TOP;

    IDE_TEST( qmv::validateSelect( aTableRef->recursiveView )
              != IDE_SUCCESS );
        
    //------------------------------
    // 변경된 tableInfo로 recursive view의 left와 right의 target을 고정한다.
    // 최대 precision까지 발산하는 경우 cast함수에서 에러가 발생한다.
    //------------------------------

    sPrevNode = NULL;
        
    // left target의 고정
    for ( sColumnInfo = aTableRef->tableInfo->columns,
              sTarget = sViewParseTree->querySet->left->SFWGH->target;
          ( sColumnInfo != NULL ) && ( sTarget != NULL );
          sColumnInfo = sColumnInfo->next,
              sTarget = sTarget->next )
    {
        sTargetColumn = sTarget->targetColumn;

        // conversion이 없다.
        IDE_DASSERT( sTargetColumn->node.conversion == NULL );

        sMtcColumn = QTC_STMT_COLUMN( aStatement, sTargetColumn );

        // type,precision이 다르다면 cast를 붙여 맞춘다.
        if ( ( sMtcColumn->type.dataTypeId !=
               sColumnInfo->basicInfo->type.dataTypeId ) ||
             ( sMtcColumn->column.size !=
               sColumnInfo->basicInfo->column.size ) )
        {
            IDE_TEST( addCastFuncForNode( aStatement,
                                          sTargetColumn,
                                          sColumnInfo->basicInfo,
                                          & sNode )
                      != IDE_SUCCESS );

            if ( sPrevNode != NULL )
            {
                sPrevNode->node.next = (mtcNode*)sNode;
            }
            else
            {
                // Nothing to do.
            }

            // target 노드를 바꾼다.
            sTarget->targetColumn = sNode;

            sPrevNode = sNode;
        }
        else
        {
            sPrevNode = sTargetColumn;
        }
    }
        
    sPrevNode = NULL;
        
    // right target의 고정
    for ( sColumnInfo = aTableRef->tableInfo->columns,
              sTarget = sViewParseTree->querySet->right->SFWGH->target;
          ( sColumnInfo != NULL ) && ( sTarget != NULL );
          sColumnInfo = sColumnInfo->next,
              sTarget = sTarget->next )
    {
        sTargetColumn = sTarget->targetColumn;
            
        // union all시 conversion을 달아놓았다.
        sMtcColumn =
            QTC_STMT_COLUMN( aStatement,
                             (qtcNode*) mtf::convertedNode(
                                 (mtcNode*) sTargetColumn,
                                 &QC_SHARED_TMPLATE(aStatement)->tmplate ) );
        
        // type,precision이 다르다면 cast를 붙여 맞춘다.
        if ( ( sMtcColumn->type.dataTypeId !=
               sColumnInfo->basicInfo->type.dataTypeId ) ||
             ( sMtcColumn->column.size !=
               sColumnInfo->basicInfo->column.size ) )
        {
            // target column의 conversion을 삭제하고
            // target column에 새로 cast함수를 붙인다.
            sTargetColumn->node.conversion = NULL;
                
            IDE_TEST( addCastFuncForNode( aStatement,
                                          sTargetColumn,
                                          sColumnInfo->basicInfo,
                                          & sNode )
                      != IDE_SUCCESS );

            if ( sPrevNode != NULL )
            {
                sPrevNode->node.next = (mtcNode*)sNode;
            }
            else
            {
                // Nothing to do.
            }

            // target 노드를 바꾼다.
            sTarget->targetColumn = sNode;

            sPrevNode = sNode;
        }
        else
        {
            sPrevNode = sTargetColumn;
        }
    }
        
    //------------------------------
    // 변경된 tableInfo로 inline view tuple을 생성한다.
    //------------------------------
        
    // BUG-37136
    aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;  // NULL이다
    aTableRef->tableType   = aTableRef->tableInfo->tableType;
    aTableRef->tableFlag   = aTableRef->tableInfo->tableFlag;
    aTableRef->tableOID    = aTableRef->tableInfo->tableOID;
        
    if (QC_IS_NULL_NAME(aTableRef->aliasName) == ID_TRUE)
    {
        SET_POSITION( aTableRef->aliasName, aTableRef->tableName );
    }
    else
    {
        // nothing to do 
    }
        
    IDE_TEST( setSameViewRef( aStatement, aSFWGH, aTableRef )
              != IDE_SUCCESS );

    // add tuple set
    IDE_TEST( qtc::nextTable( &(aTableRef->table),
                              aStatement,
                              NULL,
                              ID_TRUE,
                              aIsNullableFlag ) // PR-13597
              != IDE_SUCCESS );

    // make mtcColumn and mtcExecute for inline view tuple
    IDE_TEST( makeTupleForInlineView( aStatement,
                                      aTableRef,
                                      aTableRef->table,
                                      aIsNullableFlag )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateBottomRecursiveView( qcStatement * aStatement,
                                                 qmsSFWGH    * aSFWGH,
                                                 qmsTableRef * aTableRef,
                                                 UInt          aIsNullableFlag )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *    하위 recrusive view valdiate.
 *    
 * Implementation :
 * 
 *     with q1(i1,pid) as (
 *       select id, pid from t1
 *       union all
 *       select q1.id, '/'||q1.pid from q1, t1 where q1.pid = t1.id
 *                                      ^^
 *                                      하위 recursive view
 *     )
 *     select * from q1;
 *                   ^^
 *                   최상위 recursive view
 *
 *    q1의 i1은 union all의 left subquery의 target column type으로 고정하되
 *    precision은 right subquery의 target에 의해 커질 수 있다.
 *
 ***********************************************************************/

    qmsParseTree      * sWithParseTree = NULL;
    qcmColumn         * sColumnAlias = NULL;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateBottomRecursiveView::__FT__" );
    
    IDE_DASSERT( aTableRef->withStmt != NULL );

    sWithParseTree = (qmsParseTree *)
        aTableRef->withStmt->stmt->myPlan->parseTree;
    
    // with절의 view이고 column alias가 있는 경우 컬럼이름은
    // column alias를 이용한다.
    sColumnAlias = aTableRef->withStmt->columns;
    
    //------------------------------
    // 하위 recursive view
    // withStmt->stmt에서 validation에 사용된다.
    //------------------------------

    if ( aTableRef->withStmt->tableInfo == NULL )
    {
        // left subquery에 recursive view 오면 안됨
        IDE_TEST_RAISE( aSFWGH == sWithParseTree->querySet->left->SFWGH,
                        ERR_ORDER_RECURSIVE_VIEW );
        
        // union all의 right subquery SFWGH에서만 가능
        // 이외의 view, subquery에서는 안됨
        IDE_TEST_RAISE( aSFWGH != sWithParseTree->querySet->right->SFWGH,
                        ERR_ITSELF_DIRECTLY_RECURSIVE_VIEW );
        
        // make qcmTableInfo for IN-LINE VIEW
        IDE_TEST( makeTableInfo( aStatement,
                                 sWithParseTree->querySet->left,
                                 sColumnAlias,
                                 &aTableRef->tableInfo,
                                 QS_EMPTY_OID )
                  != IDE_SUCCESS );

        // BUG-43768 recursive with 권한 체크 에러
        aTableRef->tableInfo->tableOwnerID = QCG_GET_SESSION_USER_ID(aStatement);

        // 하위 tableInfo를 달아둔다.
        aTableRef->withStmt->tableInfo = aTableRef->tableInfo;
            
        // with 절에 의해 생성된 inline 뷰에 경우 제너레이트된 tableID를 할당한다.
        aTableRef->tableInfo->tableID = aTableRef->withStmt->tableID;
    }
    else
    {
        // right query에 subquery로 query_name이 오는 경우는
        // 이미 tableInfo를 생성해서 withStmt에 달아 놓은 상태로
        // 이부분에 들어 오게 되는데 isTop인 경우는 tableInfo가
        // withStmt에 달려 있지 않다.
        IDE_TEST_RAISE( aTableRef->withStmt->isTop == ID_TRUE,
                        ERR_DUPLICATE_RECURSIVE_VIEW );

        aTableRef->tableInfo = aTableRef->withStmt->tableInfo;

        // BUG-44156 임시 tableInfo에 tableID설정
        aTableRef->tableInfo->tableID = aTableRef->withStmt->tableID;
    }

    // BUG-37136
    aTableRef->tableHandle = aTableRef->tableInfo->tableHandle;  // NULL이다
    aTableRef->tableType   = aTableRef->tableInfo->tableType;
    aTableRef->tableFlag   = aTableRef->tableInfo->tableFlag;
    aTableRef->tableOID    = aTableRef->tableInfo->tableOID;

    if (QC_IS_NULL_NAME(aTableRef->aliasName) == ID_TRUE)
    {
        SET_POSITION( aTableRef->aliasName, aTableRef->tableName );
    }
    else
    {
        // nothing to do
    }
        
    IDE_TEST( setSameViewRef( aStatement, aSFWGH, aTableRef )
              != IDE_SUCCESS );

    // add tuple set
    IDE_TEST( qtc::nextTable( &(aTableRef->table),
                              aStatement,
                              NULL,
                              ID_TRUE,
                              aIsNullableFlag ) // PR-13597
              != IDE_SUCCESS );

    // make mtcColumn and mtcExecute for inline view tuple
    IDE_TEST( makeTupleForInlineView( aStatement,
                                      aTableRef,
                                      aTableRef->table,
                                      aIsNullableFlag )
              != IDE_SUCCESS );

    // recursive view reference id 저장
    aSFWGH->recursiveViewID = aTableRef->table;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ORDER_RECURSIVE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_ORDER_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION( ERR_ITSELF_DIRECTLY_RECURSIVE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_ITSELF_DIRECTLY_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION( ERR_DUPLICATE_RECURSIVE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_DUPLICATE_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::setSameViewRef(
    qcStatement * aStatement,
    qmsSFWGH    * aSFWGH,
    qmsTableRef * aTableRef)
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 *     기존 same view를 찾는 방식은 querySet에의 from절에서 찾거나
 *     outerQuery(SFWGH)를 따라 올라가며 from절에서 찾는 방식이었다.
 *     이런 방식은 from절과 subquery에서 참조하는 동일한 view만을
 *     검색할 수 있을뿐 (querySet내의 view만을 검색한다.)
 *
 *     ex) select * from v1 a, v1 b;
 *         select * from v1 where i1 in (select i1 from v1);
 *
 *     created view 혹은 inline view안에서 사용한 동일한 view나
 *     set절에 사용된 동일한 view를 검색할 수 없었다.
 *     (querySet 밖의 view를 검색할 수 없다.)
 *
 *     ex) select * from v1 a, (select * from v1) b;
 *         select * from v1 union select * from v1;
 *
 *     이 문제를 해결하기 위해 outerQuery를 따라가는 방식이 아닌
 *     tableMap을 검색하는 방식으로 변경한다.
 *
 *     그러나 이런 방식은 다음과 같은 view에 대해서도 동일한 view를
 *     검색하고 불필요한 materialize가 발생하는 문제가 있다.
 *     하지만 본 함수는 동일한 view를 찾는 것이지, materialize를
 *     결정하는 것은 아니므로 이런 문제는 optimizer가 풀어야 한다.
 *
 *     ex) select * from v1 union all select * from v1;
 *
 *     다행인 것은 view에 predicate이 pushdown되는 경우는 materialize를
 *     수행하지 않으므로 특별한 경우를 제외하고는 위와 같은 쿼리는
 *     자주 사용되지 않으리라 예상한다. 이런 경우를 특별하게
 *     취급한다면 hint를 사용하는 것이 맞다.
 *
 *     ex) select /+ push_select_view(v1) / * from v1
 *         union all
 *         select * from v1;
 *
 *    PROJ-2582 recursive with
 *      top query에 대한 same view 처리
 *
 *     ex> with test_cte ( cte_i1 ) as
 *        (    select * from t1
 *             union all
 *             select * from test_cte )
 *        select * from test_cte a, test_cte b;
 *
 * Implementation :
 *
 ***********************************************************************/

    qcTableMap      * sTableMap;
    qmsTableRef     * sTableRef;
    UShort            i;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::setSameViewRef::__FT__" );

    if ( aSFWGH != NULL )
    {
        if ( aSFWGH->hierarchy != NULL )
        {
            // BUG-37237 hierarchy query는 same view를 처리하지 않는다.
            aTableRef->flag &= ~QMS_TABLE_REF_HIER_VIEW_MASK;
            aTableRef->flag |= QMS_TABLE_REF_HIER_VIEW_TRUE;
            
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
    
    sTableMap = QC_SHARED_TMPLATE(aStatement)->tableMap;

    for ( i = 0; i < QC_SHARED_TMPLATE(aStatement)->tmplate.rowCount; i++ )
    {
        if ( sTableMap[i].from != NULL )
        {
            sTableRef = sTableMap[i].from->tableRef;

            // PROJ-2582 recursive with
            if ( ( aTableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                 == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE )
            {
                if ( ( sTableRef->recursiveView != NULL ) &&
                     ( sTableRef->tableInfo != NULL ) &&
                     ( sTableRef->tableInfo->tableID == aTableRef->tableInfo->tableID ) )
                {
                    if ( sTableRef->sameViewRef == NULL )
                    {
                        aTableRef->recursiveView = sTableRef->recursiveView;
                        aTableRef->sameViewRef = sTableRef;
                    }
                    else
                    {
                        aTableRef->sameViewRef = sTableRef->sameViewRef;
                    }

                    break;
                }
                else
                {
                    // Nothing to do.
                }                
            }
            else
            {
                /* TASK-7219
                 *   qmv::parseSelect 에서 qmvWith::validate 을 With Shard Trasnfrom 단계를
                 *   이후로 연기하였고, 이 시점에 With View 는 Transform 이전 것과 이후 것이
                 *   존재하게 된다.
                 *
                 *   이때에 Shard Transform 된 View는 Transform 이전의 View와 Same View Ref
                 *   처리하면 안된다. qmvShardTransform::processTransformForFrom 에서
                 *   Shard Transform 된 View 여부를 sTableRef->flag 에 남겼다.
                 *
                 *   여기서 여부를 확인하여 적절한 View 를 Same View Ref 처리하도록 유도한다.
                 */
                if ( ( sTableRef->flag & QMS_TABLE_REF_SHARD_TRANSFROM_MASK )
                     != ( aTableRef->flag & QMS_TABLE_REF_SHARD_TRANSFROM_MASK ) )
                {
                    continue;
                }
                else
                {
                    /* Nothing to do */
                }

                if ( ( sTableRef->view != NULL ) &&
                     ( sTableRef->tableInfo != NULL ) &&
                     ( ( sTableRef->tableInfo->tableID == aTableRef->tableInfo->tableID ) &&
                       ( aTableRef->tableInfo->tableID > 0 ) && // with에의한 inline view일 경우
                       ( ( sTableRef->flag & QMS_TABLE_REF_HIER_VIEW_MASK )
                         != QMS_TABLE_REF_HIER_VIEW_TRUE ) ) ) // hierarchy에 사용된 view가 아닌 경우
                {
                    if ( sTableRef->sameViewRef == NULL )
                    {
                        aTableRef->sameViewRef = sTableRef;
                    }
                    else
                    {
                        aTableRef->sameViewRef = sTableRef->sameViewRef;
                    }

                    break;
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

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;
}

IDE_RC qmvQuerySet::validateQmsFromWithOnCond(
    qmsQuerySet * aQuerySet,
    qmsSFWGH    * aSFWGH,
    qmsFrom     * aFrom,
    qcStatement * aStatement,
    UInt          aIsNullableFlag) // PR-13597
{
/***********************************************************************
 *
 * Description :
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qcuSqlSourceInfo      sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::setSameViewRef::__FT__" );

    if (aFrom->joinType == QMS_NO_JOIN) // leaf node
    {
        IDE_TEST(validateQmsTableRef(aStatement,
                                     aSFWGH,
                                     aFrom->tableRef,
                                     aSFWGH->lflag,
                                     aIsNullableFlag)  // PR-13597
                 != IDE_SUCCESS);

        // Table Map 설정
        QC_SHARED_TMPLATE(aStatement)->tableMap[aFrom->tableRef->table].from = aFrom;

        // FROM 절에 대한 dependencies 설정
        qtc::dependencyClear( & aFrom->depInfo );
        qtc::dependencySet( aFrom->tableRef->table, & aFrom->depInfo );

        // PROJ-1718 Semi/anti join과 관련된 dependency 초기화
        qtc::dependencyClear( & aFrom->semiAntiJoinDepInfo );

        IDE_TEST( qmsPreservedTable::addTable( aStatement,
                                               aSFWGH,
                                               aFrom->tableRef )
                  != IDE_SUCCESS );
            
        /* PROJ-2462 Result Cache */
        if ( aQuerySet != NULL )
        {
            if ( ( aFrom->tableRef->tableInfo->tableType == QCM_FIXED_TABLE) ||
                 ( aFrom->tableRef->tableInfo->tableType == QCM_DUMP_TABLE) ||
                 ( aFrom->tableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW) ||
                 ( aFrom->tableRef->remoteTable != NULL ) || /* PROJ-1832 */
                 ( aFrom->tableRef->mShardObjInfo != NULL ) ||
                 ( aFrom->tableRef->tableInfo->temporaryInfo.type != QCM_TEMPORARY_ON_COMMIT_NONE ) )
            {
                aQuerySet->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
                aQuerySet->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
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
    else // non-leaf node
    {
        // PR-13597 를 위해서 outer column인 경우에 해당 테이블을 NULLABLE로 설정
        if( aFrom->joinType == QMS_INNER_JOIN )
        {
            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->left,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_TRUE)
                     != IDE_SUCCESS);

            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->right,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_TRUE)
                     != IDE_SUCCESS);
        }
        else if( aFrom->joinType == QMS_FULL_OUTER_JOIN )
        {
            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->left,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_FALSE)
                     != IDE_SUCCESS);

            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->right,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_FALSE)
                     != IDE_SUCCESS);
        }
        else if( aFrom->joinType == QMS_LEFT_OUTER_JOIN )
        {
            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->left,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_TRUE)
                     != IDE_SUCCESS);

            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->right,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_FALSE)
                     != IDE_SUCCESS);
        }
        else if( aFrom->joinType == QMS_RIGHT_OUTER_JOIN )
        {
            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->left,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_FALSE)
                     != IDE_SUCCESS);

            IDE_TEST(validateQmsFromWithOnCond(aQuerySet,
                                               aSFWGH,
                                               aFrom->right,
                                               aStatement,
                                               MTC_COLUMN_NOTNULL_TRUE)
                     != IDE_SUCCESS);
        }
        else
        {
            // 이런 경우는 있을 수 없음.
        }

        /* PROJ-2197 PSM Renewal */
        aFrom->onCondition->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        IDE_TEST(qtc::estimate( aFrom->onCondition,
                                QC_SHARED_TMPLATE(aStatement),
                                aStatement,
                                aQuerySet,
                                aSFWGH,
                                aFrom )
                 != IDE_SUCCESS);

        if ( ( aFrom->onCondition->node.lflag &
               ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_COMPARISON_MASK ) )
             == ( MTC_NODE_LOGICAL_CONDITION_FALSE | MTC_NODE_COMPARISON_FALSE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aFrom->onCondition->position );
            IDE_RAISE( ERR_NOT_PREDICATE );
        }

        if ( ( aFrom->onCondition->node.lflag & MTC_NODE_DML_MASK )
             == MTC_NODE_DML_UNUSABLE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aFrom->onCondition->position );
            IDE_RAISE( ERR_USE_CURSOR_ATTR );
        }

        // BUG-16652
        if ( ( aSFWGH->outerHavingCase == ID_FALSE ) &&
             ( QTC_HAVE_AGGREGATE(aFrom->onCondition) == ID_TRUE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aFrom->onCondition->position );
            IDE_RAISE( ERR_NOT_ALLOWED_AGGREGATION );
        }

        // BUG-16652
        if ( ( aFrom->onCondition->lflag & QTC_NODE_SEQUENCE_MASK )
             == QTC_NODE_SEQUENCE_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aFrom->onCondition->position );
            IDE_RAISE( ERR_USE_SEQUENCE_IN_ON_JOIN_CONDITION );
        }

        /* Plan Property에 등록 */
        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_QUERY_REWRITE_ENABLE );

        /* PROJ-1090 Function-based Index */
        if ( QCG_GET_SESSION_QUERY_REWRITE_ENABLE(aStatement) == 1 )
        {
            IDE_TEST( qmsDefaultExpr::applyFunctionBasedIndex(
                          aStatement,
                          aFrom->onCondition,
                          aSFWGH->from,
                          &(aFrom->onCondition) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        // FROM 절에 대한 dependencies 설정
        //    - Left From의 depedencies 포함
        //    - Right From의 dependencies 포함
        qtc::dependencyClear( & aFrom->depInfo );

        IDE_TEST( qtc::dependencyOr( & aFrom->depInfo,
                                     & aFrom->left->depInfo,
                                     & aFrom->depInfo )
                  != IDE_SUCCESS );
        IDE_TEST( qtc::dependencyOr( & aFrom->depInfo,
                                     & aFrom->right->depInfo,
                                     & aFrom->depInfo )
                  != IDE_SUCCESS );

        // PROJ-1718 Semi/anti join과 관련된 dependency 초기화
        qtc::dependencyClear( & aFrom->semiAntiJoinDepInfo );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_PREDICATE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_PREDICATE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_USE_SEQUENCE_IN_ON_JOIN_CONDITION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_USE_SEQUENCE_IN_ON_JOIN_CONDITION,
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

IDE_RC qmvQuerySet::makeTableInfo(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet,
    qcmColumn       * aColumnAlias,
    qcmTableInfo   ** aTableInfo,
    smOID             aObjectID)
{
    qcmTableInfo    * sTableInfo;
    qmsTarget       * sTarget;
    qtcNode         * sNode;
    mtcColumn       * sTargetColumn;
    qcmColumn       * sPrevColumn = NULL;
    qcmColumn       * sCurrColumn;
    qcuSqlSourceInfo  sqlInfo;
    UInt              i;
    // BUG-37981
    qcTemplate      * sTemplate;
    mtcTuple        * sMtcTuple;
    qsxPkgInfo      * sPkgInfo    = NULL;
    qcNamePosition    sPosition;
    qmsNamePosition   sNamePosition;
    qcmColumn       * sAlias = aColumnAlias;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::makeTableInfo::__FT__" );

    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), qcmTableInfo, &sTableInfo)
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    sTableInfo->tablePartitionType = QCM_NONE_PARTITIONED_TABLE;
    sTableInfo->tableType = QCM_USER_TABLE;

    // PROJ-1407 Temporary Table
    sTableInfo->temporaryInfo.type = QCM_TEMPORARY_ON_COMMIT_NONE;

    /* PROJ-2359 Table/Partition Access Option */
    sTableInfo->accessOption = QCM_ACCESS_OPTION_READ_WRITE;

    /* TASK-7307 DML Data Consistency in Shard */
    sTableInfo->mIsUsable = ID_TRUE;
    sTableInfo->mShardFlag = QCM_TABLE_SHARD_FLAG_TABLE_NONE;

    /* 기타 정보 */
    sTableInfo->tableOID        = SMI_NULL_OID;
    sTableInfo->tableFlag       = 0;
    sTableInfo->isDictionary    = ID_FALSE;
    sTableInfo->viewReadOnly    = QCM_VIEW_NON_READ_ONLY;

    for (sTarget = aQuerySet->target;
         sTarget != NULL;
         sTarget = sTarget->next)
    {
        sTableInfo->columnCount++;
    }

    // To Fix PR-8331
    IDE_TEST(STRUCT_CRALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                      qcmColumn,
                                      ID_SIZEOF(qcmColumn) * sTableInfo->columnCount,
                                      (void**) & sCurrColumn)
             != IDE_SUCCESS);

    for (sTarget = aQuerySet->target, i = 0;
         sTarget != NULL;
         sTarget = sTarget->next, i++)
    {
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                              mtcColumn,
                              (void**) & (sCurrColumn[i].basicInfo))
                 != IDE_SUCCESS);

        if (sTarget->aliasColumnName.size > QC_MAX_OBJECT_NAME_LEN)
        {
            if (sTarget->targetColumn != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sTarget->targetColumn->position);
            }
            else
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sCurrColumn->namePos);
            }
            IDE_RAISE( ERR_MAX_COLUMN_NAME_LENGTH );
        }

        if ( (sTarget->targetColumn->node.lflag & MTC_NODE_OPERATOR_MASK) ==
             MTC_NODE_OPERATOR_SUBQUERY )
        {
            // BUG-16928
            IDE_TEST( getSubqueryTarget(
                          aStatement,
                          (qtcNode *)sTarget->targetColumn->node.arguments,
                          & sNode )
                      != IDE_SUCCESS );
        }
        else
        {
            sNode = sTarget->targetColumn;
        }

        /* BUG-37981
           cursor for loop에서 사용되는 cursor의 경우,
           package spec에 있는 cursor 일 수도 있기 때문에
           template이 다를 수도 있다. */
        /* BUG-44716
         * Package 생성중에는 자신의 OID로 getPkgInfo를 호출하면 안된다. */
        if( (aObjectID == QS_EMPTY_OID) ||
            ((aStatement->spvEnv->createPkg != NULL) &&
             (aObjectID == aStatement->spvEnv->createPkg->pkgOID)) ) 
        {
            sTargetColumn = QTC_STMT_COLUMN(aStatement, sNode);
        }
        else
        {
            IDE_TEST( qsxPkg::getPkgInfo( aObjectID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            sTemplate = sPkgInfo->tmplate;

            sMtcTuple = ( sTemplate->tmplate.rows ) + (sNode->node.table);

            sTargetColumn = QTC_TUPLE_COLUMN( sMtcTuple, sNode );
        }

        // BUG-35172
        // Inline View에서 BINARY 타입의 컬럼을 생성하면 안된다.
        IDE_TEST_RAISE( sTargetColumn->module->id == MTD_BINARY_ID, ERR_INVALID_TYPE );

        mtc::copyColumn(sCurrColumn[i].basicInfo, sTargetColumn);

        // PROJ-1473
        sCurrColumn[i].basicInfo->flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
        sCurrColumn[i].basicInfo->flag &= ~MTC_COLUMN_USE_TARGET_MASK;
        sCurrColumn[i].basicInfo->flag &= ~MTC_COLUMN_USE_NOT_TARGET_MASK;
        sCurrColumn[i].basicInfo->flag &= ~MTC_COLUMN_VIEW_COLUMN_PUSH_MASK;

        // PROJ-2415 Grouping Sets Clause
        // Flag 전달
        if ( ( sTarget->targetColumn->lflag & QTC_NODE_GBGS_ORDER_BY_NODE_MASK )
             == QTC_NODE_GBGS_ORDER_BY_NODE_TRUE )
        {
            sCurrColumn[i].flag &= ~QCM_COLUMN_GBGS_ORDER_BY_NODE_MASK;
            sCurrColumn[i].flag |= QCM_COLUMN_GBGS_ORDER_BY_NODE_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        // PROJ-2582 recursive with
        // with절의 view이고 column alias가 있는 경우 컬럼이름은 column alias를 이용한다.
        if ( sAlias != NULL )
        {
            QC_STR_COPY( sCurrColumn[i].name, sAlias->namePos );
            
            SET_EMPTY_POSITION(sCurrColumn[i].namePos);

            // next
            sAlias = sAlias->next;
        }
        else
        {
            /* TASK-7219
             *  Inline View 가 Shard View Transform 으로 변경되는 경우에
             *   1. qmvQuerySet::makeTableInfo
             *   2. qmvQuerySet::makeTargetListForTableRef
             *  위 함수의 순서를 중첩해서 호출하게 되고,
             *  2 에서 메모리 할당으로 생성된 Target Name 정보를 1번에서 재이용한다.
             *
             *  2 에서 aliasColumnName 이 존재하던 하지않던 할당하므로,
             *  1 에서 재이용될 때에는 aliasColumnName.size 로 제어하는 내용이 적절하지 않다.
             *
             *  2 에서 aliasColumnName.size 를 조정하는 방안도 있겠지만,
             *  문제가 되는 비슷한 작업이 많으므로,
             *  Shard 에서 오작동하는 부분만 1 에서 수정한다. 
             */
            if ( ( sTarget->aliasColumnName.size == QC_POS_EMPTY_SIZE ) ||
                 ( sTarget->aliasColumnName.size == 0 ) )
            {
                // 다음과 같은 경우 union all에 의해 생성된 internal column은 null name을 가진다.
                // select * from (select 'A' from dual union all select 'A' from dual);
                if ( QC_IS_NULL_NAME( sTarget->targetColumn->position ) == ID_TRUE )
                {
                    if ( sTarget->displayName.size > 0 )
                    {
                        sPosition.stmtText = sTarget->displayName.name;
                        sPosition.offset   = 0;
                        sPosition.size     = sTarget->displayName.size;
                    }
                    else
                    {
                        // 아무것도 할 수 없다.
                        SET_EMPTY_POSITION( sPosition );
                    }
                }
                else
                {
                    // natc에서 수행하는 경우도 기존 display name으로 출력한다.
                    if ( ( QCU_COMPAT_DISPLAY_NAME == 1 ) ||
                         ( QCU_DISPLAY_PLAN_FOR_NATC == 1 ) )
                    {
                        // BUG-38946 기존 display name과 동일하게 설정해야한다.
                        getDisplayName( sTarget->targetColumn, &sPosition );
                    }
                    else
                    {
                        IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                                        SChar,
                                                        sTarget->targetColumn->position.size + 1,
                                                        &(sNamePosition.name))
                                 != IDE_SUCCESS);

                        // O사와 유사하게 설정한다.
                        copyDisplayName( sTarget->targetColumn,
                                         &sNamePosition );
                    
                        sPosition.stmtText = sNamePosition.name;
                        sPosition.offset   = 0;
                        sPosition.size     = sNamePosition.size;
                    }
                }
            
                sCurrColumn[i].name[0] = '\0';
                sCurrColumn[i].namePos = sPosition;
            }
            else
            {
                idlOS::memcpy(sCurrColumn[i].name,
                              sTarget->aliasColumnName.name,
                              sTarget->aliasColumnName.size);
                sCurrColumn[i].name[sTarget->aliasColumnName.size] = '\0';

                SET_EMPTY_POSITION(sCurrColumn[i].namePos);
            }
        }

        sCurrColumn[i].defaultValue = NULL;
        sCurrColumn[i].next         = NULL;

        // connect
        if (sPrevColumn == NULL)
        {
            sTableInfo->columns = &sCurrColumn[i];
            sPrevColumn         = &sCurrColumn[i];
        }
        else
        {
            sPrevColumn->next = &sCurrColumn[i];
            sPrevColumn       = &sCurrColumn[i];
        }
    }

    *aTableInfo = sTableInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MAX_COLUMN_NAME_LENGTH);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_TYPE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CREATE_DISABLE_DATA_TYPE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::makeTupleForInlineView(
    qcStatement * aStatement,
    qmsTableRef * aTableRef,
    UShort        aTupleID,
    UInt          aIsNullableFlag)
{
    qcmTableInfo    * sTableInfo;
    qcmColumn       * sCurrColumn;
    UShort            sColumn;
    qtcNode         * sNode[2];
    qcNamePosition    sColumnPos;
    qtcNode         * sCurrNode;

    mtcTemplate     * sMtcTemplate;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::makeTupleForInlineView::__FT__" );

    sTableInfo = aTableRef->tableInfo;
    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sMtcTemplate->rows[aTupleID].lflag
        = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
    sMtcTemplate->rows[aTupleID].columnCount   = sTableInfo->columnCount;
    sMtcTemplate->rows[aTupleID].columnMaximum = sTableInfo->columnCount;

    // View를 위해 생성된 Tuple임을 표시함.
    sMtcTemplate->rows[aTupleID].lflag &= ~MTC_TUPLE_VIEW_MASK;
    sMtcTemplate->rows[aTupleID].lflag |= MTC_TUPLE_VIEW_TRUE;

    /* BUG-44382 clone tuple 성능개선 */
    // 복사가 필요함
    qtc::setTupleColumnFlag( &(sMtcTemplate->rows[aTupleID]),
                             ID_TRUE,
                             ID_FALSE );

    // memory alloc for columns and execute
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(mtcColumn) * sMtcTemplate->rows[aTupleID].columnCount,
                 (void**)&(sMtcTemplate->rows[aTupleID].columns))
             != IDE_SUCCESS);

    // PROJ-1473
    IDE_TEST( qtc::allocAndInitColumnLocateInTuple(
                  QC_QMP_MEM(aStatement),
                  sMtcTemplate,
                  aTupleID )
              != IDE_SUCCESS );

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(mtcExecute) * sMtcTemplate->rows[aTupleID].columnCount,
                 (void**)&(sMtcTemplate->rows[aTupleID].execute))
             != IDE_SUCCESS);

    // make dummy qtcNode
    SET_EMPTY_POSITION( sColumnPos );
    IDE_TEST(qtc::makeColumn(aStatement, sNode, NULL, NULL, &sColumnPos, NULL )
             != IDE_SUCCESS);
    sNode[0]->node.table = aTupleID;
    sCurrNode = sNode[0];

    // set mtcColumn, mtcExecute
    for( sColumn = 0, sCurrColumn = sTableInfo->columns;
         sColumn < sMtcTemplate->rows[aTupleID].columnCount;
         sColumn++, sCurrColumn = sCurrColumn->next)
    {
        sCurrNode->node.column = sColumn;

        // PROJ-1362
        // lob은 select시 lob-locator로 변환되므로 임의로 바꿔준다.
        if (sCurrColumn->basicInfo->module->id == MTD_BLOB_ID)
        {
            IDE_TEST(mtc::initializeColumn(
                         &(sMtcTemplate->rows[aTupleID].columns[sColumn]),
                         MTD_BLOB_LOCATOR_ID,
                         0,
                         0,
                         0)
                     != IDE_SUCCESS);
        }
        else if (sCurrColumn->basicInfo->module->id == MTD_CLOB_ID)
        {
            IDE_TEST(mtc::initializeColumn(
                         &(sMtcTemplate->rows[aTupleID].columns[sColumn]),
                         MTD_CLOB_LOCATOR_ID,
                         0,
                         0,
                         0)
                     != IDE_SUCCESS);
        }
        else
        {
            // copy size, type, module
            mtc::copyColumn( &(sMtcTemplate->rows[aTupleID].columns[sColumn]),
                             sCurrColumn->basicInfo );
        }

        // fix BUG-13597
        if( aIsNullableFlag == MTC_COLUMN_NOTNULL_FALSE )
        {
            sMtcTemplate->rows[aTupleID].columns[sColumn].flag &=
                ~(MTC_COLUMN_NOTNULL_MASK);
            sMtcTemplate->rows[aTupleID].columns[sColumn].flag |=
                (MTC_COLUMN_NOTNULL_FALSE);
        }
        else
        {
            // Nothing To Do
        }

        IDE_TEST(qtc::estimateNodeWithoutArgument(
                     aStatement, sCurrNode )
                 != IDE_SUCCESS );
    }

    // set offset
    qtc::resetTupleOffset( sMtcTemplate, aTupleID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::estimateTargetCount(
    qcStatement * aStatement,
    SInt        * aTargetCount)
{
    qmsParseTree    * sParseTree = (qmsParseTree *) aStatement->myPlan->parseTree;
    qmsQuerySet     * sQuerySet;
    qmsSFWGH        * sSFWGH;
    qmsFrom         * sFrom;
    qmsTarget       * sTarget;

    SInt              sTargetCount = 0;
    UInt              sUserID      = 0;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::estimateTargetCount::__FT__" );

    // initialize
    *aTargetCount = 0;

    // get Left Leaf SFWGH
    sQuerySet = sParseTree->querySet;
    while (sQuerySet->setOp != QMS_NONE)
    {
        sQuerySet = sQuerySet->left;
    }
    sSFWGH = sQuerySet->SFWGH;

    // estimate
    if (sSFWGH->target == NULL) // SELECT * FROM ...
    {
        for (sFrom = sSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
        {
            IDE_TEST(estimateOneFromTreeTargetCount(
                         aStatement, sFrom, &sTargetCount)
                     != IDE_SUCCESS);

            (*aTargetCount) = (*aTargetCount) + sTargetCount;
        }
    }
    else
    {
        for (sTarget = sSFWGH->target;
             sTarget != NULL;
             sTarget = sTarget->next)
        {
            if ( ( sTarget->flag & QMS_TARGET_ASTERISK_MASK )
                 == QMS_TARGET_ASTERISK_TRUE )
            { // in case of t1.*, u1.t1.*
                // get userID
                if (QC_IS_NULL_NAME(sTarget->targetColumn->userName) == ID_TRUE)
                {
                    sUserID = QC_EMPTY_USER_ID;
                }
                else
                {
                    IDE_TEST(qcmUser::getUserID(
                                 aStatement,
                                 sTarget->targetColumn->userName,
                                 &(sUserID))
                             != IDE_SUCCESS);
                }

                IDE_TEST(estimateOneTableTargetCount(
                             aStatement, sSFWGH->from,
                             sUserID, sTarget->targetColumn->tableName,
                             &sTargetCount)
                         != IDE_SUCCESS);

                (*aTargetCount) = (*aTargetCount) + sTargetCount;
            }
            else // expression
            {
                (*aTargetCount)++;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::estimateOneFromTreeTargetCount(
    qcStatement       * aStatement,
    qmsFrom           * aFrom,
    SInt              * aTargetCount)
{
    SInt        sLeftTargetCount  = 0;
    SInt        sRightTargetCount = 0;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::estimateOneFromTreeTargetCount::__FT__" );

    if (aFrom->joinType != QMS_NO_JOIN)
    {
        IDE_TEST(estimateOneFromTreeTargetCount(
                     aStatement, aFrom->left, &sLeftTargetCount)
                 != IDE_SUCCESS);

        IDE_TEST(estimateOneFromTreeTargetCount(
                     aStatement, aFrom->right, &sRightTargetCount)
                 != IDE_SUCCESS);

        *aTargetCount = sLeftTargetCount + sRightTargetCount;
    }
    else
    {
        IDE_TEST(getTargetCountFromTableRef(
                     aStatement, aFrom->tableRef, aTargetCount)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::estimateOneTableTargetCount(
    qcStatement       * aStatement,
    qmsFrom           * aFrom,
    UInt                aUserID,
    qcNamePosition      aTableName,
    SInt              * aTargetCount)
{
    qmsFrom         * sFrom;
    qmsTableRef     * sTableRef = NULL;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::estimateOneTableTargetCount::__FT__" );

    *aTargetCount = 0;

    for (sFrom = aFrom;
         sFrom != NULL && *aTargetCount == 0;
         sFrom = sFrom->next)
    {
        if (sFrom->joinType != QMS_NO_JOIN)
        {
            IDE_TEST(estimateOneTableTargetCount(
                         aStatement, sFrom->left,
                         aUserID, aTableName,
                         aTargetCount)
                     != IDE_SUCCESS);

            if (*aTargetCount == 0)
            {
                IDE_TEST(estimateOneTableTargetCount(
                             aStatement, sFrom->right,
                             aUserID, aTableName,
                             aTargetCount)
                         != IDE_SUCCESS);
            }
        }
        else
        {
            sTableRef = aFrom->tableRef;

            // compare table name
            if ( ( sTableRef->userID == aUserID ||
                   aUserID == QC_EMPTY_USER_ID ) &&
                 sTableRef->aliasName.size > 0 )
            {
                if ( QC_IS_NAME_MATCHED( sTableRef->aliasName, aTableName ) )
                {
                    IDE_TEST(getTargetCountFromTableRef(
                                 aStatement, sTableRef, aTargetCount)
                             != IDE_SUCCESS);
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::getTargetCountFromTableRef(
    qcStatement * aStatement,
    qmsTableRef * aTableRef,
    SInt        * aTargetCount)
{
    qcuSqlSourceInfo  sqlInfo;
    idBool            sExist = ID_FALSE;
    idBool            sIsFixedTable = ID_FALSE;
    void             *sTableHandle  = NULL;
    qcmSynonymInfo    sSynonymInfo;
    UInt              sTableType;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::getTargetCountFromTableRef::__FT__" );

    //일반 테이블인 경우
    if (aTableRef->view == NULL)
    {
        IDE_TEST(
            qcmSynonym::resolveTableViewQueue(
                aStatement,
                aTableRef->userName,
                aTableRef->tableName,
                &(aTableRef->tableInfo),
                &(aTableRef->userID),
                &(aTableRef->tableSCN),
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
            
        if( sExist == ID_FALSE )
        {
            if ( QC_IS_NULL_NAME(aTableRef->tableName) == ID_FALSE)
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & aTableRef->tableName );
                IDE_RAISE( ERR_NOT_EXIST_TABLE );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if( sIsFixedTable == ID_FALSE )
            {
                // BUG-34492
                // validation lock이면 충분하다.
                IDE_TEST( qcm::lockTableForDMLValidation(
                              aStatement,
                              sTableHandle,
                              aTableRef->tableSCN )
                          != IDE_SUCCESS );
                
                // environment의 기록
                IDE_TEST( qcgPlan::registerPlanTable(
                              aStatement,
                              sTableHandle,
                              aTableRef->tableSCN,
                              aTableRef->tableInfo->tableOwnerID, /* BUG-45893 */
                              aTableRef->tableInfo->name )        /* BUG-45893 */
                          != IDE_SUCCESS );
                
                // environment의 기록
                IDE_TEST( qcgPlan::registerPlanSynonym(
                              aStatement,
                              & sSynonymInfo,
                              aTableRef->userName,
                              aTableRef->tableName,
                              sTableHandle,
                              NULL )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            // get column count
            *aTargetCount = aTableRef->tableInfo->columnCount;
        }
    }
    //View인 경우
    else
    {
        IDE_TEST(estimateTargetCount( aTableRef->view, aTargetCount)
                 != IDE_SUCCESS);
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

IDE_RC qmvQuerySet::validatePushPredView( qcStatement  * aStatement,
                                          qmsTableRef  * aTableRef,
                                          idBool       * aIsValid )
{
/***********************************************************************
 *
 * Description : join predicate을 내릴 수 있는지  VIEW 조건 검사
 *
 * Implementation :
 *
 *    PROJ-1495
 *
 *    조건 1. VIEW에 LIMIT절이 없고 shard table 이 아니어야 함.
 *    조건 2. UNION ALL로만 구성되어야 함.
 *    조건 3. VIEW를 구성하는 각 select문들은
 *            (1) target column은 모두 순수 컬럼이어야 한다.
 *            (1) FROM절에 오는 table이 base table이어야 한다.
 *            (2) WHERE절은 올 수 있으나,
 *                WHERE절에 subquery는 올 수 없다.
 *            (3) GROUP BY              (X)
 *            (4) AGGREGATION           (X)
 *            (5) DISTINCT              (X)
 *            (6) LIMIT                 (X)
 *            (7) START WITH/CONNECT BY (X)
 *
 **********************************************************************/

    qmsParseTree   * sViewParseTree;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validatePushPredView::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sViewParseTree = (qmsParseTree *)aTableRef->view->myPlan->parseTree;

    //---------------------------------------------------
    // 조건 검사 : LIMIT절이 없어야 함.
    //---------------------------------------------------

    if ( ( sViewParseTree->limit == NULL ) &&
         ( ( sViewParseTree->common.stmtShard == QC_STMT_SHARD_NONE ) ||
           ( sViewParseTree->common.stmtShard == QC_STMT_SHARD_META ) ) ) // PROJ-2638
    {
        //---------------------------------------------------
        // 조건 검사 : UNION ALL
        //---------------------------------------------------

        IDE_TEST( validateUnionAllQuerySet( aStatement,
                                            sViewParseTree->querySet,
                                            aIsValid )
                  != IDE_SUCCESS );
    }
    else
    {
        *aIsValid = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::validateUnionAllQuerySet( qcStatement  * aStatement,
                                              qmsQuerySet  * aQuerySet,
                                              idBool       * aIsValid )
{
/***********************************************************************
 *
 * Description : VIEW가 하나의 select문 또는 UNION ALL로만 구성되었는지 등의
 *               querySet 구성요건을 만족하는지 검사
 *
 * Implementation :
 *
 *    PROJ-1495
 *
 **********************************************************************/

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validateUnionAllQuerySet::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    //---------------------------------------------------
    // 조건 검사 : UNION ALL
    //---------------------------------------------------

    if( *aIsValid == ID_TRUE )
    {
        if( aQuerySet->setOp == QMS_NONE )
        {
            //-------------------------------------------
            // querySet 검사
            //-------------------------------------------

            IDE_TEST(
                validatePushPredHintQuerySet( aStatement,
                                              aQuerySet,
                                              aIsValid )
                != IDE_SUCCESS );
        }
        else
        {
            if( aQuerySet->left != NULL )
            {
                if( aQuerySet->setOp == QMS_UNION_ALL )
                {
                    // left validate
                    IDE_TEST( validateUnionAllQuerySet( aStatement,
                                                        aQuerySet->left,
                                                        aIsValid )
                              != IDE_SUCCESS );
                }
                else
                {
                    *aIsValid = ID_FALSE;
                }
            }
            else
            {
                // Nothing To Do
            }

            if( aQuerySet->right != NULL )
            {
                if( aQuerySet->setOp == QMS_UNION_ALL )
                {
                    // left validate
                    IDE_TEST( validateUnionAllQuerySet( aStatement,
                                                        aQuerySet->right,
                                                        aIsValid )
                              != IDE_SUCCESS );
                }
                else
                {
                    *aIsValid = ID_FALSE;
                }
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmvQuerySet::validatePushPredHintQuerySet( qcStatement  * aStatement,
                                                  qmsQuerySet  * aQuerySet,
                                                  idBool       * aIsValid )
{
/***********************************************************************
 *
 * Description : VIEW를 구성하는 querySet의 조건검사
 *
 * Implementation :
 *
 *    PROJ-1495
 *
 *    조건 VIEW를 구성하는 각 select문들은
 *         (1) target column은 모두 순수 컬럼이어야 한다.
 *         (2) FROM절에 오는 table이 base table이어야 한다.
 *         (3) WHERE절은 올 수 있으나,
 *             WHERE절에 subquery는 올 수 없다.
 *         (4) GROUP BY              (X)
 *         (5) AGGREGATION           (X)
 *         (6) DISTINCT              (X)
 *         (7) START WITH/CONNECT BY (X)
 *
 **********************************************************************/

    qmsTarget    * sTarget;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::validatePushPredHintQuerySet::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    //---------------------------------------------------
    // 조건 검사 : target column이 순수 컬럼인지,
    //             from절의 table이 base table인지
    //---------------------------------------------------

    if( ( aQuerySet->SFWGH->from->joinType == QMS_NO_JOIN ) &&
        ( aQuerySet->SFWGH->from->tableRef->view == NULL ) )
    {
        for( sTarget = aQuerySet->target;
             sTarget != NULL;
             sTarget = sTarget->next )
        {
            if( QTC_IS_COLUMN( aStatement, sTarget->targetColumn ) == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                *aIsValid = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        *aIsValid = ID_FALSE;
    }

    //---------------------------------------------------
    // 조건 검사 : where절 검사
    //---------------------------------------------------

    if( *aIsValid == ID_TRUE )
    {
        if( aQuerySet->SFWGH->where != NULL )
        {
            if( ( aQuerySet->SFWGH->where->lflag & QTC_NODE_SUBQUERY_MASK )
                == QTC_NODE_SUBQUERY_EXIST )
            {
                *aIsValid = ID_FALSE;
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
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------
    // 조건 검사 :  GROUP BY              (X)
    //              AGGREGATION           (X)
    //              DISTINCT              (X)
    //              START WITH/CONNECT BY (X)
    //---------------------------------------------------

    if( *aIsValid == ID_TRUE )
    {
        if( ( aQuerySet->SFWGH->group == NULL ) &&
            ( aQuerySet->SFWGH->aggsDepth1 == NULL ) &&
            ( aQuerySet->SFWGH->selectType == QMS_ALL ) &&
            ( aQuerySet->SFWGH->hierarchy == NULL ) )
        {
            // Nothing To Do
        }
        else
        {
            *aIsValid = ID_FALSE;
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;
}

IDE_RC qmvQuerySet::getSubqueryTarget( qcStatement  * aStatement,
                                       qtcNode      * aNode,
                                       qtcNode     ** aTargetNode )
{

    IDU_FIT_POINT_FATAL( "qmvQuerySet::getSubqueryTarget::__FT__" );

    if ( (aNode->node.lflag & MTC_NODE_OPERATOR_MASK) ==
         MTC_NODE_OPERATOR_SUBQUERY )
    {
        IDE_TEST( getSubqueryTarget( aStatement,
                                     (qtcNode *)aNode->node.arguments,
                                     aTargetNode )
                  != IDE_SUCCESS );
    }
    else
    {
        (*aTargetNode) = aNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::changeTargetForCommunication(
    qcStatement     * aStatement,
    qmsQuerySet     * aQuerySet )
{
    qmsTarget         * sTarget;
    qtcNode           * sTargetColumn;
    mtcColumn         * sMtcColumn;
    qcuSqlSourceInfo    sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::changeTargetForCommunication::__FT__" );
    
    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // BUG-20652
    // client로의 전송을 위해 taget의 geometry type을
    // binary type으로 변환
    //------------------------------------------

    // set절이 있든 없든 이미 새로운 target을 구성했음
    for ( sTarget = aQuerySet->target;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        sTargetColumn = sTarget->targetColumn;

        if( aStatement->calledByPSMFlag != ID_TRUE )
        {
            // target은 항상 type이 정의되어있음
            IDE_DASSERT( MTC_NODE_IS_DEFINED_TYPE( & sTargetColumn->node )
                         == ID_TRUE );
        }
        else
        {
            // Nothing to do.
        }
        
        sMtcColumn = QTC_STMT_COLUMN( aStatement, sTargetColumn );
        
        if ( sMtcColumn->module->id == MTD_GEOMETRY_ID )
        {            
            // subquery가 아니라면 target은 항상 conversion이 없음
            IDE_DASSERT( sTargetColumn->node.conversion == NULL );
            
            // target에 conversion이 생성되는 예외상황.
            // get_blob_locator처럼 함수를 이용한 변환도 생각할 수 있으나
            // 이 경우 비용이 많이 드는 memcpy를 수행하게 되므로
            // conversion을 이용한다.
            IDE_TEST( qtc::makeConversionNode( sTargetColumn,
                                               aStatement,
                                               QC_SHARED_TMPLATE(aStatement),
                                               & mtdBinary )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // 최종 target column이 list type이어서는 안된다.
        if ( sMtcColumn->module->id == MTD_LIST_ID )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sTargetColumn->position );
            IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE_TYPE_IN_TARGET );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::addDecryptFunc( qcStatement  * aStatement,
                                    qmsTarget    * aTarget )
{
    qmsTarget         * sCurrTarget;
    qtcNode           * sTargetColumn;
    qtcNode           * sNode;
    qtcNode           * sPrevNode = NULL;
    mtcColumn         * sMtcColumn;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::changeTargetForCommunication::__FT__" );

    // add Decrypt function
    for ( sCurrTarget = aTarget;
          sCurrTarget != NULL;
          sCurrTarget = sCurrTarget->next)
    {
        sTargetColumn = sCurrTarget->targetColumn;
        
        if( aStatement->calledByPSMFlag == ID_FALSE )
        {
            // target은 항상 type이 정의되어있음
            IDE_DASSERT( MTC_NODE_IS_DEFINED_TYPE( & sTargetColumn->node )
                         == ID_TRUE );
        }
        else
        {
            // Nothing to do.
        }
        
        sMtcColumn = QTC_STMT_COLUMN( aStatement, sTargetColumn );

        if( (sMtcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
            == MTD_ENCRYPT_TYPE_TRUE )
        {
            // default policy의 암호 타입이라도 decrypt 함수를 생성하여
            // subquery의 결과는 항상 암호 타입이 나올 수 없게 한다.
                
            // decrypt 함수를 만든다.
            IDE_TEST( addDecryptFuncForNode( aStatement,
                                             sTargetColumn,
                                             & sNode )
                      != IDE_SUCCESS );

            if ( sPrevNode != NULL )
            {
                sPrevNode->node.next = (mtcNode*)sNode;
            }
            else
            {
                // Nothing to do.
            }

            // target 노드를 바꾼다.
            sCurrTarget->targetColumn = sNode;

            sPrevNode = sNode;
        }
        else 
        {
            sPrevNode = sTargetColumn;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::addDecryptFuncForNode( qcStatement  * aStatement,
                                           qtcNode      * aNode,
                                           qtcNode     ** aNewNode )
{
    qtcNode  * sNode[2];

    IDU_FIT_POINT_FATAL( "qmvQuerySet::addDecryptFuncForNode::__FT__" );
    
    // decrypt 함수를 만든다.
    IDE_TEST( qtc::makeNode( aStatement,
                             sNode,
                             & aNode->columnName,
                             & mtfDecrypt )
              != IDE_SUCCESS );

    // 함수를 연결한다.
    sNode[0]->node.arguments = (mtcNode*) aNode;
    sNode[0]->node.next = aNode->node.next;
    sNode[0]->node.arguments->next = NULL;

    // 함수만 estimate를 수행한다.
    IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                             sNode[0] )
              != IDE_SUCCESS );

    *aNewNode = sNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::addBLobLocatorFuncForNode( qcStatement  * aStatement,
                                               qtcNode      * aNode,
                                               qtcNode     ** aNewNode )
{
    qtcNode  * sNode[2];

    IDU_FIT_POINT_FATAL( "qmvQuerySet::addDecryptFuncForNode::__FT__" );

    /* get_blob_locator 함수를 만든다. */
    IDE_TEST( qtc::makeNode( aStatement,
                             sNode,
                             & aNode->columnName,
                             & mtfGetBlobLocator )
              != IDE_SUCCESS );

    /* 함수를 연결한다. */
    sNode[0]->node.arguments = (mtcNode*) aNode;
    sNode[0]->node.next = aNode->node.next;
    sNode[0]->node.arguments->next = NULL;

    /* 함수만 estimate를 수행한다. */
    IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                             sNode[0] )
              != IDE_SUCCESS );

    *aNewNode = sNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::addCLobLocatorFuncForNode( qcStatement  * aStatement,
                                               qtcNode      * aNode,
                                               qtcNode     ** aNewNode )
{
    qtcNode  * sNode[2];

    IDU_FIT_POINT_FATAL( "qmvQuerySet::addCLobLocatorFuncForNode::__FT__" );

    /* get_clob_locator 함수를 만든다. */
    IDE_TEST( qtc::makeNode( aStatement,
                             sNode,
                             & aNode->columnName,
                             & mtfGetClobLocator )
              != IDE_SUCCESS );

    /* 함수를 연결한다. */
    sNode[0]->node.arguments = (mtcNode*) aNode;
    sNode[0]->node.next = aNode->node.next;
    sNode[0]->node.arguments->next = NULL;

    /* 함수만 estimate를 수행한다. */
    IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                             sNode[0] )
              != IDE_SUCCESS );

    *aNewNode = sNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ----------------------------------------------------------------------------
 * PROJ-1789: Updatable Scrollable Cursor (a.k.a. PROWID)
 *
 * qmsTarget에 들어가는 column 정보를 setting 한다
 *
 * 기존에 aliasName과 displayName만 setting 했는데
 * userName, tableName, isUpdatable 등의 정보를 추가하였다
 *
 * 예)
 *
 * SELECT C1 FROM T1
 *
 * tableName       = T1
 * aliasTableName  = T1
 * columnName      = C1
 * aliasColumnName = C1
 * displayName     = C1
 *
 *
 * SELECT C1 AS C FROM T1 AS T
 *
 * tableName       = T1
 * aliasTableName  = T
 * columnName      = C1
 * aliasColumnName = C
 * displayName     = C
 *
 *
 * SELECT C1+1 AS C FROM T1
 *
 * tableName       = NULL
 * aliasTableName  = NULL
 * columnName      = NULL
 * aliasColumnName = C 
 * displayName     = C
 * 
 *
 * SELECT C1+1 FROM T1
 *
 * tableName       = NULL
 * aliasTableName  = NULL
 * columnName      = NULL
 * aliasColumnName = NULL
 * displayName     = C1+1
 *
 *
 * SELECT C1 FROM (SELECT * FROM T1);
 *
 * tableName       = NULL
 * aliasTableName  = NULL
 * columnName      = C1
 * aliasColumnName = C1
 * displayName     = C1
 * ----------------------------------------------------------------------------
 */
IDE_RC qmvQuerySet::setTargetColumnInfo(qcStatement* aStatement,
                                        qmsTarget  * aTarget)
{
    qtcNode        * sExpression;
    qmsFrom        * sFrom;
    qcNamePosition * sNamePosition;
    qcNamePosition   sPosition;
    UInt             sCurOffset;
    idBool           sIsAliasExist;
    idBool           sIsBaseColumn;
    qcuSqlSourceInfo sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::setTargetColumnInfo::__FT__" );

    sExpression = aTarget->targetColumn;
    sIsBaseColumn = QTC_IS_COLUMN(aStatement, sExpression);

    // PROJ-1653 Outer Join Operator (+)
    // Outer Join Operator (+) 는 target 절에는 올 수 없다.
    if ( ( sExpression->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
         == QTC_NODE_JOIN_OPERATOR_EXIST )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sExpression->position );

        IDE_RAISE( ERR_NOT_APPLICABLE_OPERATOR_IN_TARGET );
    }

    if ( aTarget->aliasColumnName.size != QC_POS_EMPTY_SIZE )
    {
        sIsAliasExist = ID_TRUE;
    }
    else
    {
        sIsAliasExist = ID_FALSE;
    }

    if ( sIsBaseColumn == ID_TRUE )
    {
        sFrom = QC_SHARED_TMPLATE(aStatement)->
            tableMap[(sExpression)->node.table].from;

        /* BUG-37658 : meta table info 가 free 될 수 있으므로 다음 항목을
         *             alloc -> memcpy 한다.
         * - userName
         * - tableName
         * - aliasTableName
         * - columnName
         * - aliasColumnName
         * - displayName
         */

        // set user name
        sNamePosition = &sFrom->tableRef->userName;
        if ( sNamePosition->size != QC_POS_EMPTY_SIZE )
        {
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM(aStatement),
                                              SChar,
                                              sNamePosition->size + 1,
                                              &(aTarget->userName.name) )
                      != IDE_SUCCESS );
            aTarget->userName.size = sNamePosition->size;

            idlOS::strncpy( aTarget->userName.name,
                            sNamePosition->stmtText + sNamePosition->offset,
                            aTarget->userName.size );
            aTarget->userName.name[aTarget->userName.size] = '\0';
        }
        else
        {
            aTarget->userName.name = NULL;
            aTarget->userName.size = QC_POS_EMPTY_SIZE;
        }

        // set base table name
        sNamePosition = &sFrom->tableRef->tableName;
        if ( sNamePosition->size != QC_POS_EMPTY_SIZE )
        {
            // BUG-30344
            // plan cache check-in이 실패하여 non-cache mode로 수행될 때
            // query text는 더 이상 존재하지 않으므로 복사 생성한다.
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM(aStatement),
                                              SChar,
                                              sNamePosition->size + 1,
                                              &(aTarget->tableName.name) )
                      != IDE_SUCCESS );
            aTarget->tableName.size = sNamePosition->size;

            idlOS::strncpy( aTarget->tableName.name,
                            sNamePosition->stmtText +
                            sNamePosition->offset,
                            aTarget->tableName.size );
            aTarget->tableName.name[aTarget->tableName.size] = '\0';
        }
        else
        {
            aTarget->tableName.name = NULL;
            aTarget->tableName.size = QC_POS_EMPTY_SIZE;
        }

        // set alias table name
        sNamePosition = &sFrom->tableRef->aliasName;
        if ( sNamePosition->size != QC_POS_EMPTY_SIZE )
        {
            // BUG-30344
            // plan cache check-in이 실패하여 non-cache mode로 수행될 때
            // query text는 더 이상 존재하지 않으므로 복사 생성한다.
            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM(aStatement),
                                              SChar,
                                              sNamePosition->size + 1,
                                              &(aTarget->aliasTableName.name) )
                      != IDE_SUCCESS);

            idlOS::strncpy( aTarget->aliasTableName.name,
                            sNamePosition->stmtText + sNamePosition->offset,
                            sNamePosition->size );

            aTarget->aliasTableName.name[sNamePosition->size] = '\0';
            aTarget->aliasTableName.size = sNamePosition->size;
        }
        else
        {
            // alias table name이 없으면 그냥 table name
            aTarget->aliasTableName.name = aTarget->tableName.name;
            aTarget->aliasTableName.size = aTarget->tableName.size;
        }

        // set base column name
        sNamePosition = &sExpression->columnName;

        if ( ( sExpression->lflag & QTC_NODE_PRIOR_MASK )
             == QTC_NODE_PRIOR_EXIST )
        {
            // 'PRIOR '를 앞에 덧붙인다.
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            sNamePosition->size + 7,
                                            &(aTarget->columnName.name))
                     != IDE_SUCCESS);

            idlOS::strncpy(aTarget->columnName.name,
                           "PRIOR ",
                           6);

            idlOS::strncpy(aTarget->columnName.name + 6,
                           sNamePosition->stmtText + sNamePosition->offset,
                           sNamePosition->size);

            aTarget->columnName.size = sNamePosition->size + 6;
        }
        else
        {
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            sNamePosition->size + 1,
                                            &(aTarget->columnName.name))
                     != IDE_SUCCESS);

            idlOS::strncpy(aTarget->columnName.name,
                           sNamePosition->stmtText + sNamePosition->offset,
                           sNamePosition->size);

            aTarget->columnName.size = sNamePosition->size;
        }

        aTarget->columnName.name[aTarget->columnName.size] = '\0';

        // set alias column name
        if ( sIsAliasExist == ID_FALSE )
        {
            // alias가 없으면 column name

            sNamePosition = &sExpression->columnName;
            // BUG-30344
            // plan cache check-in이 실패하여 non-cache mode로 수행될 때
            // query text는 더 이상 존재하지 않으므로 복사 생성한다.
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            sNamePosition->size + 1,
                                            &(aTarget->aliasColumnName.name))
                     != IDE_SUCCESS);

            idlOS::strncpy(aTarget->aliasColumnName.name,
                           sNamePosition->stmtText + sNamePosition->offset,
                           sNamePosition->size);
            aTarget->aliasColumnName.name[sNamePosition->size] = '\0';
            aTarget->aliasColumnName.size = sNamePosition->size;
        }
        else
        {
            // parse 때 이미 setting 되었음
        }

        // set updatability
        if (sFrom->tableRef->view == NULL)
        {
            aTarget->flag &= ~QMS_TARGET_IS_UPDATABLE_MASK;
            aTarget->flag |= QMS_TARGET_IS_UPDATABLE_TRUE;
        }
        else
        {
            aTarget->flag &= ~QMS_TARGET_IS_UPDATABLE_MASK;
            aTarget->flag |= QMS_TARGET_IS_UPDATABLE_FALSE;
        }
    }
    else
    {
        aTarget->userName.name        = NULL;
        aTarget->userName.size        = QC_POS_EMPTY_SIZE;
        aTarget->tableName.name       = NULL;
        aTarget->tableName.size       = QC_POS_EMPTY_SIZE;
        aTarget->aliasTableName.name  = NULL;
        aTarget->aliasTableName.size  = QC_POS_EMPTY_SIZE;
        aTarget->columnName.name      = NULL;
        aTarget->columnName.size      = QC_POS_EMPTY_SIZE;
        aTarget->flag                &= ~QMS_TARGET_IS_UPDATABLE_MASK;
        aTarget->flag                |= QMS_TARGET_IS_UPDATABLE_FALSE;
    }

    // set display name
    if (sIsBaseColumn == ID_FALSE)
    {
        if (sIsAliasExist == ID_FALSE)
        {
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            sExpression->position.size + 1,
                                            &(aTarget->displayName.name))
                     != IDE_SUCCESS);

            // BUG-38946 display name의 설정
            // natc에서 수행하는 경우도 기존 display name으로 출력한다.
            if ( ( QCU_COMPAT_DISPLAY_NAME == 1 ) ||
                 ( QCU_DISPLAY_PLAN_FOR_NATC == 1 ) )
            {
                // 기존과 유사하게 설정한다.
                getDisplayName( sExpression, &sPosition );
                
                idlOS::strncpy(aTarget->displayName.name,
                               sPosition.stmtText +
                               sPosition.offset,
                               sPosition.size);
                aTarget->displayName.name[sPosition.size] = '\0';
                aTarget->displayName.size = sPosition.size;
            }
            else
            {
                // O사와 유사하게 설정한다.
                copyDisplayName( sExpression,
                                 &(aTarget->displayName) );
            }
        }
        else
        {
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            aTarget->aliasColumnName.size + 1,
                                            &(aTarget->displayName.name))
                     != IDE_SUCCESS);

            idlOS::strncpy(aTarget->displayName.name,
                           aTarget->aliasColumnName.name,
                           aTarget->aliasColumnName.size);
            aTarget->displayName.name[aTarget->aliasColumnName.size] = '\0';
            aTarget->displayName.size = aTarget->aliasColumnName.size;
        }
    }
    else
    {
        if (sIsAliasExist == ID_FALSE)
        {
            sCurOffset = 0;

            /*
             * TODO:
             * BUG-33546 SELECT_HEADER_DISPLAY property does not work properly
             * 
             * 이 bug 를 해결하려면 if 조건을 아래 주석과 같이 하면 된다.
             */

            /*
              if ((QCG_GET_SESSION_SELECT_HEADER_DISPLAY(aStatement) == 0) &&
              (aTarget->aliasTableName.size > 0))
            */
            if (1 == 0)
            {
                // tableName.columnName
                IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                                SChar,
                                                aTarget->aliasTableName.size +
                                                aTarget->columnName.size + 2,
                                                &(aTarget->displayName.name))
                         != IDE_SUCCESS);

                idlOS::strncpy(aTarget->displayName.name,
                               aTarget->aliasTableName.name,
                               aTarget->aliasTableName.size);
                aTarget->displayName.name[aTarget->aliasTableName.size] = '.';
                sCurOffset = sCurOffset + aTarget->aliasTableName.size + 1;
            }
            else
            {
                // columnName
                IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                                SChar,
                                                aTarget->columnName.size + 1,
                                                &(aTarget->displayName.name))
                         != IDE_SUCCESS);
            }
            idlOS::strncpy(aTarget->displayName.name + sCurOffset,
                           aTarget->columnName.name,
                           aTarget->columnName.size);
            sCurOffset = sCurOffset + aTarget->columnName.size;

            aTarget->displayName.name[sCurOffset] = '\0';
            aTarget->displayName.size = sCurOffset;

            qcgPlan::registerPlanProperty(aStatement,
                                          PLAN_PROPERTY_SELECT_HEADER_DISPLAY);
        }
        else
        {
            // alias가 있으면 SELECT_HEADER_DISPLAY 무시
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(QC_QMP_MEM(aStatement),
                                            SChar,
                                            aTarget->aliasColumnName.size + 1,
                                            &(aTarget->displayName.name))
                     != IDE_SUCCESS);

            idlOS::strncpy(aTarget->displayName.name,
                           aTarget->aliasColumnName.name,
                           aTarget->aliasColumnName.size);
            aTarget->displayName.name[aTarget->aliasColumnName.size] = '\0';
            aTarget->displayName.size = aTarget->aliasColumnName.size;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_APPLICABLE_OPERATOR_IN_TARGET)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_OPERATOR_IN_TARGET,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::addCastOper( qcStatement  * aStatement,
                                 qmsTarget    * aTarget )
{
    qmsTarget         * sCurrTarget;
    qtcNode           * sNewNode;
    qtcNode           * sPrevNode = NULL;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::addCastOper::__FT__" );

    if( ( QCU_COERCE_HOST_VAR_TO_VARCHAR > 0 ) &&
        ( aStatement->calledByPSMFlag == ID_FALSE ) )
    {
        for ( sCurrTarget = aTarget;
              sCurrTarget != NULL;
              sCurrTarget = sCurrTarget->next)
        {
            IDE_TEST( searchHostVarAndAddCastOper( aStatement,
                                                   sCurrTarget->targetColumn,
                                                   & sNewNode,
                                                   ID_FALSE )
                      != IDE_SUCCESS );

            if ( sNewNode != NULL )
            {
                sCurrTarget->targetColumn = sNewNode;

                if ( sPrevNode != NULL )
                {
                    sPrevNode->node.next = (mtcNode*)sNewNode;
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

            sPrevNode = sCurrTarget->targetColumn;
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

IDE_RC qmvQuerySet::searchHostVarAndAddCastOper( qcStatement   * aStatement,
                                                 qtcNode       * aNode,
                                                 qtcNode      ** aNewNode,
                                                 idBool          aContainRootsNext )
{
    mtcTemplate    * sTemplate;
    mtcColumn        sTypeColumn;
    qtcNode        * sNewNode;
    qtcNode        * sCastNode[2];
    qtcNode        * sTypeNode[2];
    UShort           sVariableRow;
    qcNamePosition   sEmptyPos;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::searchHostVarAndAddCastOper::__FT__" );

    sTemplate    = & QC_SHARED_TMPLATE(aStatement)->tmplate;
    sVariableRow = sTemplate->variableRow;
    *aNewNode    = NULL;

    if ( ( aNode->node.module == & mtfCast ) ||
         ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
           == MTC_NODE_OPERATOR_SUBQUERY ) )
    {
        /* BUG-42522 A COERCE_HOST_VAR_IN_SELECT_LIST_TO_VARCHAR is sometimes
         * wrong.
         */
        if ( ( aNode->node.next != NULL ) &&
             ( aContainRootsNext == ID_TRUE ) )
        {
            IDE_TEST( searchHostVarAndAddCastOper(
                          aStatement,
                          (qtcNode *) aNode->node.next,
                          & sNewNode,
                          ID_TRUE )
                      != IDE_SUCCESS );

            if ( sNewNode != NULL )
            {
                aNode->node.next = (mtcNode*) sNewNode;
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
        if ( aNode->node.arguments != NULL )
        {
            IDE_TEST( searchHostVarAndAddCastOper(
                          aStatement,
                          (qtcNode *) aNode->node.arguments,
                          & sNewNode,
                          ID_TRUE )
                      != IDE_SUCCESS );

            if ( sNewNode != NULL )
            {
                aNode->node.arguments = (mtcNode*) sNewNode;
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
        
        if ( ( aNode->node.next != NULL ) && ( aContainRootsNext == ID_TRUE ) )
        {
            IDE_TEST( searchHostVarAndAddCastOper(
                          aStatement,
                          (qtcNode *) aNode->node.next,
                          & sNewNode,
                          ID_TRUE )
                      != IDE_SUCCESS );

            if ( sNewNode != NULL )
            {
                aNode->node.next = (mtcNode*) sNewNode;
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
        
        if ( ( (aNode->node.lflag & MTC_NODE_BIND_MASK) == MTC_NODE_BIND_EXIST )
             &&
             ( aNode->node.table == sVariableRow )
             &&
             ( aNode->node.arguments == NULL ) )
        {
            // host variable, terminal node
            SET_EMPTY_POSITION( sEmptyPos );
            
            // cast operator를 만든다.
            IDE_TEST( qtc::makeNode( aStatement,
                                     sCastNode,
                                     & aNode->columnName,
                                     & mtfCast )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::initializeColumn( & sTypeColumn,
                                             & mtdVarchar,
                                             1,
                                             QCU_COERCE_HOST_VAR_TO_VARCHAR,
                                             0 )
                      != IDE_SUCCESS );
            
            // column정보를 이용하여 qtcNode를 생성한다.
            IDE_TEST( qtc::makeProcVariable( aStatement,
                                             sTypeNode,
                                             & sEmptyPos,
                                             & sTypeColumn,
                                             QTC_PROC_VAR_OP_NEXT_COLUMN )
                      != IDE_SUCCESS );
            
            sCastNode[0]->node.funcArguments = (mtcNode *) sTypeNode[0];
            
            sCastNode[0]->node.lflag &= ~MTC_NODE_COLUMN_COUNT_MASK;
            sCastNode[0]->node.lflag |= 1;
            
            // 함수를 연결한다.
            sCastNode[0]->node.arguments = (mtcNode*) aNode;
            sCastNode[0]->node.next = aNode->node.next;
            sCastNode[0]->node.arguments->next = NULL;
            
            *aNewNode = sCastNode[0];
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


// BUG-38273
// ansi 스타일 inner join 을 일반 스타일의 조인으로 변경한다.
idBool qmvQuerySet::checkInnerJoin( qmsFrom * aFrom )
{
    qmsFrom  * sFrom = aFrom;
    idBool     sOnlyInnerJoin;

    if( sFrom->joinType == QMS_NO_JOIN )
    {
        sOnlyInnerJoin = ID_TRUE;
    }
    else if( sFrom->joinType == QMS_INNER_JOIN )
    {
        if( checkInnerJoin( sFrom->left ) == ID_TRUE )
        {
            sOnlyInnerJoin = checkInnerJoin( sFrom->right );
        }
        else
        {
            sOnlyInnerJoin = ID_FALSE;
        }
    }
    else
    {
        sOnlyInnerJoin = ID_FALSE;
    }

    return sOnlyInnerJoin;
}

// BUG-38273
// ansi 스타일 inner join 을 일반 스타일의 조인으로 변경한다.
IDE_RC qmvQuerySet::innerJoinToNoJoin( qcStatement * aStatement,
                                       qmsSFWGH    * aSFWGH,
                                       qmsFrom     * aFrom )
{
    qmsFrom         * sFrom = aFrom;
    qmsFrom         * sTempFrom;
    qtcNode         * sNode[2];
    qcNamePosition    sNullPosition;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::innerJoinToNoJoin::__FT__" );

    if ( sFrom->left->joinType == QMS_INNER_JOIN )
    {
        IDE_TEST( innerJoinToNoJoin( aStatement,
                                     aSFWGH,
                                     sFrom->left ) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    if ( sFrom->right->joinType == QMS_INNER_JOIN )
    {
        IDE_TEST( innerJoinToNoJoin( aStatement,
                                     aSFWGH,
                                     sFrom->right ) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    if( aSFWGH->where != NULL )
    {
        //------------------------------------------------------
        // 기존 where 절에 on 절의 프리디킷을 연결한다.
        // and 연산자를 생성하여 연결하되
        // 기존에 and 연산자가 있으면 and 연산자의 인자로 연결한다.
        //------------------------------------------------------
        if ( aSFWGH->where->node.module != &mtfAnd )
        {
            SET_EMPTY_POSITION(sNullPosition);

            IDE_TEST(qtc::makeNode(aStatement,
                                   sNode,
                                   &sNullPosition,
                                   &mtfAnd ) != IDE_SUCCESS);

            sNode[0]->node.arguments       = (mtcNode *)sFrom->onCondition;
            sNode[0]->node.arguments->next = (mtcNode *)aSFWGH->where;
            sNode[0]->node.lflag += 2;

            aSFWGH->where = sNode[0];
        }
        else
        {
            //------------------------------------------------------
            // 기존 and 노드의 마지막에 연결한다.
            //------------------------------------------------------
            for( sNode[0] = (qtcNode *)aSFWGH->where->node.arguments;
                 sNode[0]->node.next != NULL;
                 sNode[0] = (qtcNode*)sNode[0]->node.next);

            sNode[0]->node.next = (mtcNode *)sFrom->onCondition;
        }
    }
    else
    {
        //------------------------------------------------------
        // 기존 where 절이 없을 경우 바로 연결한다.
        //------------------------------------------------------
        aSFWGH->where = sFrom->onCondition;
    }

    //------------------------------------------------------
    // left 의 마지막에 right 를 연결한다.
    //------------------------------------------------------
    for( sTempFrom = sFrom->left;
         sTempFrom->next != NULL;
         sTempFrom = sTempFrom->next );
    sTempFrom->next = sFrom->right;

    //------------------------------------------------------
    // right 의 마지막에 부모의 next 를 연결한다.
    //------------------------------------------------------
    for( sTempFrom = sFrom->right;
         sTempFrom->next != NULL;
         sTempFrom = sTempFrom->next );
    sTempFrom->next = sFrom->next;

    // BUG-38347 fromPosition 을 유지해야함
    SET_POSITION( sFrom->left->fromPosition, sFrom->fromPosition );

    idlOS::memcpy( sFrom, sFrom->left, ID_SIZEOF(qmsFrom));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-38946
 * 기존 display name과 동일하게 설정하기 위한 함수
 *
 * select (1) from dual               --> (1)
 * select count(1) from dual          --> COUNT(1)
 * select count(*) from dual          --> COUNT
 * select count(*) over () from dual  --> COUNT             <===(*)
 * select count(i1) over () from dual --> COUNT(I1) over ()
 * select user_name() from dual       --> USER_NAME
 * select func1 from dual             --> FUNC1
 * select func1() from dual           --> FUNC1
 * select func1(1) from dual          --> FUNC1(1)
 * select pkg1.func1 from dual        --> PKG1.FUNC1        <===(*)
 * select pkg1.func1() from dual      --> FUNC1
 * select pkg1.func1(1) from dual     --> FUNC1(1)
 * select user1.func1 from dual       --> USER1.FUNC1       <===(*)
 * select user1.func1() from dual     --> FUNC1
 * select user1.func1(1) from dual    --> FUNC1(1)
 * select seq1.nextval from dual      --> SEQ1.NEXTVAL
 *
 * (*)같은 몇몇 경우에 따라 display name이 달라지는 버그가 있어
 * 항상 display되는 것으로 수정한다.
 * select count(*) over () from dual  --> COUNT(*) over ()
 * select pkg1.func1 from dual        --> FUNC1
 * select user1.func1 from dual       --> FUNC1
 */
void qmvQuerySet::getDisplayName( qtcNode        * aNode,
                                  qcNamePosition * aNamePos )
{
    if ( ( ( aNode->node.lflag & MTC_NODE_REMOVE_ARGUMENTS_MASK ) ==
           MTC_NODE_REMOVE_ARGUMENTS_TRUE ) ||
         ( aNode->overClause != NULL ) )
    {
        // count(*), (1), (func1())
        SET_POSITION( *aNamePos, aNode->position );
    }
    else
    {
        if ( ( aNode->node.lflag & MTC_NODE_COLUMN_COUNT_MASK ) > 0 )
        {
            if ( aNode->node.module == &qtc::spFunctionCallModule )
            {
                // func1(1), pkg1.func(1)
                if ( QC_IS_NULL_NAME(aNode->pkgName) == ID_TRUE )
                {
                    aNamePos->stmtText = aNode->columnName.stmtText;
                    aNamePos->offset   = aNode->columnName.offset;
                    aNamePos->size     = aNode->position.size +
                        aNode->position.offset - aNode->columnName.offset;
                }
                else
                {
                    aNamePos->stmtText = aNode->pkgName.stmtText;
                    aNamePos->offset   = aNode->pkgName.offset;
                    aNamePos->size     = aNode->position.size +
                        aNode->position.offset - aNode->pkgName.offset;
                }
            }
            else
            {
                // count(1)
                SET_POSITION( *aNamePos, aNode->position );
            }
        }
        else
        {
            if ( aNode->node.module == &qtc::spFunctionCallModule )
            {
                // func1, func1(), pkg1.func1, pkg1.func1()
                if ( QC_IS_NULL_NAME(aNode->pkgName) == ID_TRUE )
                {
                    SET_POSITION( *aNamePos, aNode->columnName );
                }
                else
                {
                    SET_POSITION( *aNamePos, aNode->pkgName );
                }
            }
            else
            {
                // user_name()
                if ( ( QC_IS_NULL_NAME(aNode->position) == ID_FALSE ) &&
                     ( QC_IS_NULL_NAME(aNode->columnName) == ID_FALSE ) )
                {
                    aNamePos->stmtText = aNode->position.stmtText;
                    aNamePos->offset   = aNode->position.offset;
                    aNamePos->size     = aNode->columnName.offset +
                        aNode->columnName.size - aNode->position.offset;
                }
                else
                {
                    SET_EMPTY_POSITION( *aNamePos );
                }
            }
        }
    }

}

/* BUG-38946
 * O사와 유사하게 display name을 설정하기 위한 함수
 *
 * select (1) from dual               --> (1)
 * select 'abc' from dual             --> 'ABC'
 * select count(1) from dual          --> COUNT(1)
 * select count(*) from dual          --> COUNT(*)
 * select count(*) over () from dual  --> COUNT(*)OVER()
 * select count(i1) over () from dual --> COUNT(I1)OVER()
 * select user_name() from dual       --> USER_NAME()
 * select func1 from dual             --> FUNC1
 * select func1() from dual           --> FUNC1()
 * select func1(1) from dual          --> FUNC1(1)
 * select pkg1.func1 from dual        --> PKG1.FUNC1
 * select pkg1.func1() from dual      --> PKG1.FUNC1()
 * select pkg1.func1(1) from dual     --> PKG1.FUNC1(1)
 * select user1.func1 from dual       --> USER1.FUNC1
 * select user1.func1() from dual     --> USER1.FUNC1()
 * select user1.func1(1) from dual    --> USER1.FUNC1(1)
 * select seq1.nextval from dual      --> NEXTVAL           <===(*)
 */
void qmvQuerySet::copyDisplayName( qtcNode         * aNode,
                                   qmsNamePosition * aNamePos )
{
    qcNamePosition   sPosition;
    SChar          * sName;
    SChar          * sPos;
    SInt             i;

    // BUG-41072
    // pseudo column인 경우 columnName만 표시한다.
    if ( aNode->node.module == &qtc::columnModule )
    {
        SET_POSITION( sPosition, aNode->columnName );
    }
    else
    {
        SET_POSITION( sPosition, aNode->position );
    }
    
    sName = aNamePos->name;
    sPos  = sPosition.stmtText + sPosition.offset;
    
    for ( i = 0; i < sPosition.size; i++, sPos++ )
    {
        if ( idlOS::idlOS_isspace( *sPos ) == 0 )
        {
            *sName = idlOS::toupper(*sPos);
            sName++;
        }
        else
        {
            // Nothing to do.
        }
    }
    *sName = '\0';
    
    aNamePos->size = sName - aNamePos->name;

}

void qmvQuerySet::setLateralOuterQueryAndFrom( qmsQuerySet * aViewQuerySet, 
                                               qmsTableRef * aViewTableRef,
                                               qmsSFWGH    * aOuterSFWGH )
{
/***********************************************************************
 *
 * Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 * 
 *  Lateral View의 outerQuery를 상위 레벨의 SFWGH로 지정한다.
 *  ANSI Join Tree 내부에 Lateral View가 존재하면 outerFrom도 지정한다.
 *
 *  outerQuery는 Lateral View (aViewQuerySet)의 상위 SFWGH를 말하고,
 *  outerFrom은 Lateral View가 속한 Join Tree를 나타낸다.
 *  당연히 outerQuery->from 은 outerFrom을 포함하며, 
 *  outerFrom이 더 작은 범위를 나타낸다.
 *
 *  aViewQuerySet이 validation 될 때, 외부 참조 컬럼의 위치를 파악할 때,
 *
 *   - outerFrom이 존재하면 outerFrom에서만 탐색을 진행한다.
 *   - outerFrom이 존재하지 않으면 outerQuery에서만 탐색을 진행한다.
 *
 *  =====================================================================
 *
 *  [ outerFrom? ]
 *  ANSI-Join Tree 내부에서 외부 참조할 수 있는 범위가 outerQuery보다
 *  제한되어야 하는데, 이를 나타내는 것이 outerFrom이다.
 *
 *  예를 들어, ON 절에 이런 Subquery가 사용된다면 outerQuery/outerFrom 은,
 *  
 *  SELECT * 
 *  FROM   T1, 
 *         T2 LEFT JOIN T3 ON T2.i1 = T3.i1 
 *            LEFT JOIN T4 ON 1 = ( SELECT T1.i1 FROM DUAL ) -- Subquery
 *            LEFT JOIN T5 ON T2.i2 = T5.i2;
 *
 *         [T1]                (JOIN)
 *                            ／    \
 *                           ／     [T5]
 *                          ／  
 *                      (JOIN)---SubqueryFilter   
 *                     ／    \
 *                 (JOIN)    [T4]
 *                ／    \
 *              [T2]    [T3]
 *  
 *   - Subquery의 outerQuery->from = { 1, { { { 2, 3 }, 4 }, 5 } }
 *   - Subquery의 outerFrom = { { 2, 3 }, 4 }
 *
 *  이 된다. 따라서, 위의 Subquery는 T1.i1이 잘못 참조되고 있으므로 에러로 종료.
 *
 *  쉽게 이야기하면, 'Subquery가 속한 바로 그 Join Tree까지만' outerFrom이 된다. 
 *  상위 Join은 outerFrom이 될 수 없다는 것이다. (예제에서, T5는 속하지 않는다)
 *
 *  =====================================================================
 *
 *  outerFrom 기법을 Lateral View에도 그대로 적용해야 한다.
 *  
 *   - Lateral View가 단독으로 쓰인다면, outerQuery를 설정한다.
 *   - Lateral View가 ANSI-Join Tree 안에 존재한다면 
 *     'Lateral View가 속한 바로 그 Join Tree 까지만' outerFrom로 설정한다.
 *
 *  (참고) outerQuery는 상위 outerQuery를 탐색하기 위한 용도로도 사용되므로,
 *         outerFrom만 탐색한다고 해서 outerQuery를 지정하지 않으면 안 된다.  
 *
 *
 * Implementation :
 *
 *  - Set Operator가 존재하는 경우에는 LEFT/RIGHT 재귀호출한다.
 *  - 단일 QuerySet인 경우, outerQuery를 설정하고 outerFrom이 될 qmsFrom을 찾는다.
 *  - 찾은 qmsFrom을 outerFrom으로 지정한다. (찾은 값이 NULL일 수도 있다.)
 *
 ***********************************************************************/

    qmsFrom * sFrom             = NULL;
    qmsFrom * sOuterFrom        = NULL;

    IDE_DASSERT( aViewQuerySet != NULL );

    if ( aViewQuerySet->setOp == QMS_NONE )
    {
        // Set Operation이 없는 경우
        if ( aViewQuerySet->SFWGH != NULL )
        {
            // outerQuery 설정
            IDE_DASSERT( aViewQuerySet->SFWGH->outerQuery == NULL );
            aViewQuerySet->SFWGH->outerQuery = aOuterSFWGH;

            // outerFrom 설정
            for ( sFrom = aOuterSFWGH->from;
                  ( sFrom != NULL ) && ( sOuterFrom == NULL );
                  sFrom = sFrom->next )
            {
                if ( sFrom->joinType != QMS_NO_JOIN )
                {
                    getLateralOuterFrom( aViewQuerySet,
                                         aViewTableRef,
                                         sFrom,
                                         & sOuterFrom );
                }
                else
                {
                    // Nothing to do.
                }
            }
            aViewQuerySet->SFWGH->outerFrom = sOuterFrom;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Set Operation인 경우
        setLateralOuterQueryAndFrom( aViewQuerySet->left,  
                                     aViewTableRef, 
                                     aOuterSFWGH );

        setLateralOuterQueryAndFrom( aViewQuerySet->right,  
                                     aViewTableRef, 
                                     aOuterSFWGH );
    }

}

void qmvQuerySet::getLateralOuterFrom( qmsQuerySet  * aViewQuerySet, 
                                       qmsTableRef  * aViewTableRef, 
                                       qmsFrom      * aFrom,
                                       qmsFrom     ** aOuterFrom )
{
/***********************************************************************
 *
 * Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 * 
 *  Lateral View가 ANSI Join Tree에 속한 경우,
 *  Lateral View가 속한 바로 그 Join Tree만 찾아 outerFrom으로 반환한다.
 *
 * Implementation :
 *  
 *  Lateral View를 나타내는 TableRef가 속한 qmsFrom이 
 *  현재 관심을 두고 있는 qmsFrom의 LEFT/RIGHT에 속해 있다면, 
 *  현재 qmsFrom이 aOuterFrom이 된다.
 *  
 *  Join Tree를 만날 때마다 LEFT/RIGHT에 대해 재귀호출한다.
 *
 ***********************************************************************/

    IDE_DASSERT( aFrom->left  != NULL );
    IDE_DASSERT( aFrom->right != NULL );

    // Left에서 탐색
    if ( aFrom->left->joinType != QMS_NO_JOIN )
    {
        getLateralOuterFrom( aViewQuerySet,
                             aViewTableRef,
                             aFrom->left,
                             aOuterFrom );
    }
    else
    {
        if ( aFrom->left->tableRef == aViewTableRef )
        {
            *aOuterFrom = aFrom;
        }
        else
        {
            // Nothing to do.
        }
    }

    // Left에서 찾지 못했다면 Right에서 탐색
    if ( *aOuterFrom == NULL ) 
    {
        if ( aFrom->right->joinType != QMS_NO_JOIN )
        {
            getLateralOuterFrom( aViewQuerySet,
                                 aViewTableRef,
                                 aFrom->right,
                                 aOuterFrom );
        }
        else
        {
            if ( aFrom->right->tableRef == aViewTableRef )
            {
                *aOuterFrom = aFrom;
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

IDE_RC qmvQuerySet::addCastFuncForNode( qcStatement  * aStatement,
                                        qtcNode      * aNode,
                                        mtcColumn    * aCastType,
                                        qtcNode     ** aNewNode )
{
    qtcNode    * sNode[2];
    qtcNode    * sArgNode[2];
    mtcColumn  * sMtcColumn = NULL;

    IDU_FIT_POINT_FATAL( "qmvQuerySet::addCastFuncForNode::__FT__" );

    // cast 함수를 만든다.
    IDE_TEST( qtc::makeNode( aStatement,
                             sNode,
                             & aNode->columnName,
                             & mtfCast )
              != IDE_SUCCESS );

    // cast 인자를 만든다.
    IDE_TEST( qtc::makeNode( aStatement,
                             sArgNode,
                             & aNode->columnName,
                             & qtc::valueModule )
              != IDE_SUCCESS );

    sMtcColumn = QTC_STMT_COLUMN( aStatement, sArgNode[0] );

    mtc::copyColumn( sMtcColumn, aCastType );

    // 함수를 연결한다.
    sNode[0]->node.arguments = (mtcNode*) aNode;
    sNode[0]->node.funcArguments = (mtcNode*) sArgNode[0];
    sNode[0]->node.next = aNode->node.next;
    sNode[0]->node.arguments->next = NULL;

    // 함수만 estimate를 수행한다.
    IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                             sNode[0] )
              != IDE_SUCCESS );

    *aNewNode = sNode[0];

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::searchStartWithPrior( qcStatement * aStatement,
                                          qtcNode     * aNode )
{
    qcuSqlSourceInfo  sSqlInfo;

    if ( ( ( aNode->lflag & QTC_NODE_PRIOR_MASK ) ==
           QTC_NODE_PRIOR_EXIST ) ||
         ( QTC_IS_AGGREGATE( aNode ) == ID_TRUE ) )
    {
        sSqlInfo.setSourceInfo( aStatement,
                                &aNode->position );
        IDE_RAISE( ERR_NOT_SUPPORTED );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.arguments != NULL )
    {
        if ( QTC_IS_SUBQUERY( (qtcNode *)aNode->node.arguments ) == ID_FALSE )
        {
            IDE_TEST( searchStartWithPrior( aStatement, (qtcNode *)aNode->node.arguments )
                      != IDE_SUCCESS );
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

    if ( aNode->node.next != NULL )
    {
        if ( QTC_IS_SUBQUERY( (qtcNode *)aNode->node.next ) == ID_FALSE )
        {
            IDE_TEST( searchStartWithPrior( aStatement, (qtcNode *)aNode->node.next )
                      != IDE_SUCCESS );
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
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORTED );
    {
        (void)sSqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT,
                            sSqlInfo.getErrMessage() ));
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-45742 connect by aggregation function fatal */
IDE_RC qmvQuerySet::searchConnectByAggr( qcStatement * aStatement,
                                         qtcNode     * aNode )
{
    qcuSqlSourceInfo  sSqlInfo;

    if ( QTC_IS_AGGREGATE(aNode) == ID_TRUE )
    {
        sSqlInfo.setSourceInfo( aStatement,
                                &aNode->position );
        IDE_RAISE( ERR_NOT_SUPPORTED );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNode->node.arguments != NULL )
    {
        if ( QTC_IS_SUBQUERY( (qtcNode *)aNode->node.arguments ) == ID_FALSE )
        {
            IDE_TEST( searchConnectByAggr( aStatement, (qtcNode *)aNode->node.arguments )
                      != IDE_SUCCESS );
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

    if ( aNode->node.next != NULL )
    {
        if ( QTC_IS_SUBQUERY( (qtcNode *)aNode->node.next ) == ID_FALSE )
        {
            IDE_TEST( searchConnectByAggr( aStatement, (qtcNode *)aNode->node.next )
                      != IDE_SUCCESS );
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
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORTED );
    {
        (void)sSqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT,
                            sSqlInfo.getErrMessage() ));
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQuerySet::convertAnsiInnerJoin( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH )
{
    qmsFrom  * sFrom = NULL;
    idBool     sOnlyInnerJoin = ID_FALSE;

    /* TASK-7219
     *   Shard Transform 은 사용자의 Query Text 기반으로 수행되는데,
     *   ANSI 스타일와 일반 스타일의 차이가 매우 크므로,
     *   해당 변환을 수행하지 않도록 한다.
     *
     *   그리고 원격지로 Query가 보내지면, 원격지에서 해당 변환을 수행할 것으로 기대한다.
     */
    IDE_TEST_CONT( ( ( aStatement->myPlan->parseTree->stmtShard != QC_STMT_SHARD_NONE )
                     &&
                     ( aStatement->myPlan->parseTree->stmtShard != QC_STMT_SHARD_META ) ),
                   NORMAL_EXIT );

    // BUG-38402
    if ( QCU_OPTIMIZER_ANSI_INNER_JOIN_CONVERT == 1 )
    {
        // BUG-38273
        // ansi 스타일 inner join 을 일반 스타일의 조인으로 변경한다.
        for (sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next)
        {
            if ( sFrom->joinType == QMS_NO_JOIN )
            {
                sOnlyInnerJoin = ID_TRUE;
            }
            else if ( sFrom->joinType == QMS_INNER_JOIN )
            {
                sOnlyInnerJoin = checkInnerJoin( sFrom );
            }
            else
            {
                sOnlyInnerJoin = ID_FALSE;
            }

            if ( sOnlyInnerJoin == ID_FALSE )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sOnlyInnerJoin == ID_TRUE )
        {
            qcgPlan::registerPlanProperty(
                aStatement,
                PLAN_PROPERTY_OPTIMIZER_ANSI_INNER_JOIN_CONVERT );

            for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
            {
                if ( sFrom->joinType == QMS_INNER_JOIN )
                {
                    IDE_TEST( innerJoinToNoJoin( aStatement,
                                                 aSFWGH,
                                                 sFrom ) != IDE_SUCCESS);
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

    /* TASK-7219 */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

