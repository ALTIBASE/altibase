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
 * $Id: qmv.h 90434 2021-04-02 06:56:50Z khkwak $
 **********************************************************************/

#ifndef _Q_QMV_H_
#define _Q_QMV_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmmParseTree.h>
#include <qcg.h>
#include <qsv.h>

// qmv::validateSelect에 인자로 넘기는 flag 값에 대한 정의
// 향후에 특정 동작을 위하여 flag를 설정하여야 하는 경우
// 기능에 따라서 확장하면 됨.
#define QMV_PERFORMANCE_VIEW_CREATION_MASK           ID_ULONG( 0x0000000000000001 )
#define QMV_PERFORMANCE_VIEW_CREATION_FALSE          ID_ULONG( 0x0000000000000000 )
#define QMV_PERFORMANCE_VIEW_CREATION_TRUE           ID_ULONG( 0x0000000000000001 )

#define QMV_VIEW_CREATION_MASK                       ID_ULONG( 0x0000000000000002 )
#define QMV_VIEW_CREATION_FALSE                      ID_ULONG( 0x0000000000000000 )
#define QMV_VIEW_CREATION_TRUE                       ID_ULONG( 0x0000000000000002 )

// BUG-20272
// subquery의 query set인지를 기록한다.
#define QMV_QUERYSET_SUBQUERY_MASK                   ID_ULONG( 0x0000000000000004 )
#define QMV_QUERYSET_SUBQUERY_FALSE                  ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_SUBQUERY_TRUE                   ID_ULONG( 0x0000000000000004 )

// proj-2188
#define QMV_SFWGH_PIVOT_MASK                         ID_ULONG( 0x0000000000000008 )
#define QMV_SFWGH_PIVOT_FALSE                        ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_PIVOT_TRUE                         ID_ULONG( 0x0000000000000008 )

// BUG-34295
#define QMV_SFWGH_JOIN_MASK                          ID_ULONG( 0x00000000000000F0 )
#define QMV_SFWGH_JOIN_INNER                         ID_ULONG( 0x0000000000000010 )
#define QMV_SFWGH_JOIN_FULL_OUTER                    ID_ULONG( 0x0000000000000020 )
#define QMV_SFWGH_JOIN_LEFT_OUTER                    ID_ULONG( 0x0000000000000040 )
#define QMV_SFWGH_JOIN_RIGHT_OUTER                   ID_ULONG( 0x0000000000000080 )

// PROJ-2204 join update, delete
// key preserved table을 계산해야하는 SFWGH인지 설정한다.
// create view, update view, delete view, insert view
#define QMV_SFWGH_UPDATABLE_VIEW_MASK                ID_ULONG( 0x0000000000000100 )
#define QMV_SFWGH_UPDATABLE_VIEW_FALSE               ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_UPDATABLE_VIEW_TRUE                ID_ULONG( 0x0000000000000100 )

// PROJ-2394 Bulk Pivot Aggregation
// pivot transformation으로 생성되는 첫번째 view임을 설정한다.
#define QMV_SFWGH_PIVOT_FIRST_VIEW_MASK              ID_ULONG( 0x0000000000000200 )
#define QMV_SFWGH_PIVOT_FIRST_VIEW_FALSE             ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_PIVOT_FIRST_VIEW_TRUE              ID_ULONG( 0x0000000000000200 )

#define QMV_SFWGH_CONNECT_BY_FUNC_MASK               ID_ULONG( 0x0000000000000400 )
#define QMV_SFWGH_CONNECT_BY_FUNC_FALSE              ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_CONNECT_BY_FUNC_TRUE               ID_ULONG( 0x0000000000000400 )

// BUG-48128
// having clause 또는 target caluse에 scalar subquery가 outer column을 갖는경우
// outer QUERYSET에 flag
#define QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_MASK      ID_ULONG( 0x0000000000000800 )
#define QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_FALSE     ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_TRUE      ID_ULONG( 0x0000000000000800 )

// PROJ-2415 Grouping Sets Transform
#define QMV_SFWGH_GBGS_TRANSFORM_MASK                ID_ULONG( 0x0000000000007000 )
#define QMV_SFWGH_GBGS_TRANSFORM_NONE                ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_GBGS_TRANSFORM_TOP                 ID_ULONG( 0x0000000000001000 )
#define QMV_SFWGH_GBGS_TRANSFORM_MIDDLE              ID_ULONG( 0x0000000000002000 )
#define QMV_SFWGH_GBGS_TRANSFORM_BOTTOM              ID_ULONG( 0x0000000000003000 )
#define QMV_SFWGH_GBGS_TRANSFORM_BOTTOM_MTR          ID_ULONG( 0x0000000000004000 )

// BUG-41311 table function
// table function transformation으로 생성되는 view임을 설정한다.
#define QMV_SFWGH_TABLE_FUNCTION_VIEW_MASK           ID_ULONG( 0x0000000000008000 )
#define QMV_SFWGH_TABLE_FUNCTION_VIEW_FALSE          ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_TABLE_FUNCTION_VIEW_TRUE           ID_ULONG( 0x0000000000008000 )

// BUG-41573 Lateral View with GROUPING SETS
// QuerySet이 Lateral View인지 설정한다.
#define QMV_QUERYSET_LATERAL_MASK                    ID_ULONG( 0x0000000000010000 )
#define QMV_QUERYSET_LATERAL_FALSE                   ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_LATERAL_TRUE                    ID_ULONG( 0x0000000000010000 )

// PROJ-2523 Unpivot clause
#define QMV_SFWGH_UNPIVOT_MASK                       ID_ULONG( 0x0000000000020000 )
#define QMV_SFWGH_UNPIVOT_FALSE                      ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_UNPIVOT_TRUE                       ID_ULONG( 0x0000000000020000 )

// PROJ-2462 Result Cache
#define QMV_QUERYSET_RESULT_CACHE_MASK               ID_ULONG( 0x00000000000C0000 )
#define QMV_QUERYSET_RESULT_CACHE_NONE               ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_RESULT_CACHE                    ID_ULONG( 0x0000000000040000 )
#define QMV_QUERYSET_RESULT_CACHE_NO                 ID_ULONG( 0x0000000000080000 )

// PROJ-2462 Result Cache
#define QMV_QUERYSET_RESULT_CACHE_INVALID_MASK       ID_ULONG( 0x0000000000100000 )
#define QMV_QUERYSET_RESULT_CACHE_INVALID_FALSE      ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE       ID_ULONG( 0x0000000000100000 )

// PROJ-2582 recursive with
#define QMV_QUERYSET_RECURSIVE_VIEW_MASK             ID_ULONG( 0x0000000000600000 )
#define QMV_QUERYSET_RECURSIVE_VIEW_NONE             ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_RECURSIVE_VIEW_LEFT             ID_ULONG( 0x0000000000200000 )
#define QMV_QUERYSET_RECURSIVE_VIEW_RIGHT            ID_ULONG( 0x0000000000400000 )
#define QMV_QUERYSET_RECURSIVE_VIEW_TOP              ID_ULONG( 0x0000000000600000 )

/* BUG-48405 */
#define QMV_QUERYSET_ANSI_JOIN_ORDERING_MASK         ID_ULONG( 0x0000000000800000 )
#define QMV_QUERYSET_ANSI_JOIN_ORDERING_TRUE         ID_ULONG( 0x0000000000800000 )
#define QMV_QUERYSET_ANSI_JOIN_ORDERING_FALSE        ID_ULONG( 0x0000000000000000 )

// BUG-43059 Target subquery unnest/removal disable
#define QMV_QUERYSET_TARGET_SUBQUERY_UNNEST_MASK     ID_ULONG( 0x0000000001000000 )
#define QMV_QUERYSET_TARGET_SUBQUERY_UNNEST_TRUE     ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_TARGET_SUBQUERY_UNNEST_FALSE    ID_ULONG( 0x0000000001000000 )

#define QMV_QUERYSET_TARGET_SUBQUERY_REMOVAL_MASK    ID_ULONG( 0x0000000002000000 )
#define QMV_QUERYSET_TARGET_SUBQUERY_REMOVAL_TRUE    ID_ULONG( 0x0000000000000000 )
#define QMV_QUERYSET_TARGET_SUBQUERY_REMOVAL_FALSE   ID_ULONG( 0x0000000002000000 )

// PROJ-2687 Shard aggregation transform
#define QMV_SFWGH_SHARD_TRANS_VIEW_MASK              ID_ULONG( 0x0000000004000000 )
#define QMV_SFWGH_SHARD_TRANS_VIEW_FALSE             ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_SHARD_TRANS_VIEW_TRUE              ID_ULONG( 0x0000000004000000 )

// BUG-46932
#define QMV_QUERYSET_FROM_RECURSIVE_WITH_MASK        ID_ULONG( 0x0000000008000000 )
#define QMV_QUERYSET_FROM_RECURSIVE_WITH_TRUE        ID_ULONG( 0x0000000008000000 )
#define QMV_QUERYSET_FROM_RECURSIVE_WITH_FALSE       ID_ULONG( 0x0000000000000000 )

/* BUG-47786 Unnest 결과 오류 */
#define QMV_SFWGH_UNNEST_OR_STOP_MASK                ID_ULONG( 0x0000000010000000 )
#define QMV_SFWGH_UNNEST_OR_STOP_FALSE               ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_UNNEST_OR_STOP_TRUE                ID_ULONG( 0x0000000010000000 )

/* BUG-47786 Unnest 결과 오류 */
#define QMV_SFWGH_UNNEST_LEFT_DISK_MASK              ID_ULONG( 0x0000000020000000 )
#define QMV_SFWGH_UNNEST_LEFT_DISK_FALSE             ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_UNNEST_LEFT_DISK_TRUE              ID_ULONG( 0x0000000020000000 )

/* TASK-7219 */
#define QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK          ID_ULONG( 0x00000000C0000000 )
#define QMV_SFWGH_SHARD_ORDER_BY_TRANS_FALSE         ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_SHARD_ORDER_BY_TRANS_TRUE          ID_ULONG( 0x0000000040000000 )
#define QMV_SFWGH_SHARD_ORDER_BY_TRANS_ERROR         ID_ULONG( 0x0000000080000000 )

/* TASK-7219 Shard Transformer Refactoring */
#define QMV_SFWGH_SHARD_ANSI_TRANSFORM_MASK          ID_ULONG( 0x0000000100000000 )
#define QMV_SFWGH_SHARD_ANSI_TRANSFORM_FALSE         ID_ULONG( 0x0000000000000000 )
#define QMV_SFWGH_SHARD_ANSI_TRANSFORM_TRUE          ID_ULONG( 0x0000000100000000 )

#define QMV_SET_QCM_COLUMN(_dest_, _src_)                       \
    {                                                           \
        _dest_->basicInfo       = _src_->basicInfo;             \
        _dest_->defaultValueStr = _src_->defaultValueStr;       \
        _dest_->flag            = _src_->flag;                  \
    }

#define QMV_SET_QMM_VALUE_NODE(_dest_, _src_)   \
    {                                           \
        _dest_->value    = _src_->value;        \
        _dest_->position = _src_->position;     \
    }

class qmv
{

    // BUGBUG: jhseong, FT,PV 혼용가능한 코드로 변경.
public:
    // parse DEFAULT values of INSERT
    static IDE_RC parseInsertValues( qcStatement * aStatement );
    static IDE_RC parseInsertAllDefault( qcStatement * aStatement );
    static IDE_RC parseInsertSelect( qcStatement * aStatement );
    static IDE_RC parseMultiInsertSelect( qcStatement * aStatement );

    // validate INSERT
    static IDE_RC validateInsertValues( qcStatement * aStatement );
    static IDE_RC validateInsertAllDefault( qcStatement * aStatement );
    static IDE_RC validateInsertSelect( qcStatement * aStatement );
    static IDE_RC validateMultiInsertSelect( qcStatement * aStatement );

    // validate DELETE
    static IDE_RC validateDelete( qcStatement * aStatement );

    // parse VIEW of DELETE where
    static IDE_RC parseDelete( qcStatement * aStatement );

    // parse DEFAULT values of UPDATE
    static IDE_RC parseUpdate( qcStatement * aStatement );

    // parse VIEW of SELECT
    static IDE_RC parseSelect( qcStatement * aStatement );

    /* TASK-7219 Shard Transformer Refactoring */
    static IDE_RC parseInsertValuesInternal( qcStatement * aStatement );
    static IDE_RC parseSelectInternal( qcStatement * aStatement );
    static IDE_RC parseUpdateInternal( qcStatement * aStatement );
    static IDE_RC parseDeleteInternal( qcStatement * aStatement );

    // PROJ-1382, jhseong
    // parse function for detect & branch-fy of validation ptr.
    // implementation for Fixed Table, Performance View.
    static IDE_RC detectDollarTables(
        qcStatement * aStatement );
    static IDE_RC detectDollarInQuerySet(
        qcStatement * aStatement,
        qmsQuerySet * aQuerySet );
    static IDE_RC detectDollarInExpression(
        qcStatement * aStatement,
        qtcNode     * aExpression );
    static IDE_RC detectDollarInFromClause(
        qcStatement * aStatement,
        qmsFrom     * aTableRef );

    // PROJ-1068 DBLink
    static IDE_RC isDBLinkSupportQuery(
        qcStatement       *aStatement);

    // validate UPDATE
    static IDE_RC validateUpdate( qcStatement * aStatement );

    // validate LOCK TABLE
    static IDE_RC validateLockTable( qcStatement * aStatement );

    // validate SELECT
    static IDE_RC validateSelect( qcStatement * aStatement );

    /* PROJ-1584 DML Return Clause */
    static IDE_RC validateReturnInto( qcStatement   * aStatement,
                                      qmmReturnInto * aReturnInto, 
                                      qmsSFWGH      * aSFWGH,
                                      qmsFrom       * aFrom,
                                      idBool          aIsInsert );

    // BUG-42715
    static IDE_RC validateReturnValue( qcStatement   * aStatement,
                                       qmmReturnInto * aReturnInto, 
                                       qmsSFWGH      * aSFWGH,
                                       qmsFrom       * aFrom,
                                       idBool          aIsInsert,
                                       UInt          * aReturnCnt );

    // BUG-42715
    static IDE_RC validateReturnIntoValue( qcStatement   * aStatement,
                                           qmmReturnInto * aReturnInto, 
                                           UInt            aReturnCnt );

    // validate LIMIT
    static IDE_RC validateLimit( qcStatement * aStatement,
                                 qmsLimit    * aLimit );

    // validate LOOP clause
    static IDE_RC validateLoop( qcStatement  * aStatement );

    // parse VIEW in expression
    static IDE_RC parseViewInExpression(
        qcStatement     * aStatement,
        qtcNode         * aExpression);

    // for MOVE DML
    static IDE_RC parseMove( qcStatement * aStatement );
    static IDE_RC validateMove( qcStatement * aStatement );

    /* PROJ-1988 Implement MERGE statement */
    static IDE_RC parseMerge( qcStatement * aStatement );
    static IDE_RC validateMerge( qcStatement * aStatement);

    // PROJ-1436
    static IDE_RC validatePlanHints( qcStatement * aStatement,
                                     qmsHints    * aHints );
    
    /* PROJ-2219 Row-level before update trigger */
    static IDE_RC getRefColumnList( qcStatement  * aQcStmt,
                                    UChar       ** aRefColumnList,
                                    UInt         * aRefColumnCount );

    // BUG-15746
    static IDE_RC describeParamInfo(
        qcStatement * aStatement,
        mtcColumn   * aColumn,
        qtcNode     * aValue );

    
    static IDE_RC getParentInfoList(qcStatement     * aStatement,
                                    qcmTableInfo    * aTableInfo,
                                    qcmParentInfo  ** aParentInfo,
                                    qcmColumn       * aChangedColumn = NULL,
                                    SInt              aUptColCount   = 0);

    static IDE_RC getChildInfoList(qcStatement       * aStatement,
                                   qcmTableInfo      * aTableInfo,
                                   qcmRefChildInfo  ** aChildInfo,  // BUG-28049
                                   qcmColumn         * aChangedColumn = NULL,
                                   SInt                aUptColCount   = 0);

    /* PROJ-2714 Multiple Update Delete support */
    static IDE_RC parseMultiUpdate( qcStatement * aStatement );
    static IDE_RC validateMultiUpdate( qcStatement * aStatement );

    static IDE_RC parseMultiDelete( qcStatement * aStatement );
    static IDE_RC validateMultiDelete( qcStatement * aStatement );

private:
    static IDE_RC insertCommon(
        qcStatement       * aStatement,
        qmsTableRef       * aTableRef,
        qdConstraintSpec ** aCheckConstrSpec,      /* PROJ-1107 Check Constraint 지원 */
        qcmColumn        ** aDefaultExprColumns,   /* PROJ-1090 Function-based Index */
        qmsTableRef      ** aDefaultTableRef );

    static IDE_RC setDefaultOrNULL(
        qcStatement     * aStatement,
        qcmTableInfo    * aTableInfo,    
        qcmColumn       * aColumn,
        qmmValueNode    * aValue);

    static IDE_RC getValue( qcmColumn        * aColumnList,
                            qmmValueNode     * aValueList,
                            qcmColumn        * aColumn,
                            qmmValueNode    ** aValue );


    static IDE_RC makeInsertRow( qcStatement      * aStatement,
                                 qmmInsParseTree  * aParseTree );

    static IDE_RC makeUpdateRow( qcStatement * aStatement );

    static IDE_RC makeMoveRow( qcStatement * aStatement );

    // PROJ-1509
    static IDE_RC getChildInfoList( qcStatement       * aStatement,
                                    qcmRefChildInfo   * aTopChildInfo,    // BUG-28049
                                    qcmRefChildInfo   * aRefChildInfo );  // BUG-28049 

    // PROJ-1509
    static idBool checkCascadeCycle( qcmRefChildInfo  * aChildInfo,  // BUG-28049
                                     qcmIndex         * aIndex );

    // parse VIEW of SELECT
    static IDE_RC parseViewInQuerySet( qcStatement  * aStatement,
                                       qmsParseTree * aParseTree, /* TASK-7219 Shard Transformer Refactoring */
                                       qmsQuerySet  * aQuerySet );

    // parse VIEW of FROM clause
    static IDE_RC parseViewInFromClause(
        qcStatement * aStatement,
        qmsFrom     * aTableRef,
        qmsHints    * aHints );
    
    // BUG-13725
    static IDE_RC checkInsertOperatable(
        qcStatement      * aStatement,
        qmmInsParseTree  * aParseTree,
        qcmTableInfo     * aTableInfo );

    static IDE_RC checkUpdateOperatable( qcStatement  * aStatement,
                                         idBool         aIsInsteadOfTrigger,
                                         qmsTableRef  * aTableRef );

    static IDE_RC checkDeleteOperatable( qcStatement  * aStatement,
                                         idBool         aIsInsteadOfTrigger,
                                         qmsTableRef  * aTableRef );

    static IDE_RC checkInsteadOfTrigger(
        qmsTableRef * aTableRef,
        UInt          aEventType,
        idBool      * aTriggerExist );

    // PROJ-1705
    static IDE_RC setFetchColumnInfo4ParentTable( qcStatement * aStatement,
                                                  qmsTableRef * aTableRef );

    // PROJ-1705
    static IDE_RC setFetchColumnInfo4ChildTable( qcStatement * aStatement,
                                                 qmsTableRef * aTableRef );

    // PROJ-2205
    static IDE_RC setFetchColumnInfo4Trigger( qcStatement * aStatement,
                                              qmsTableRef * aTableRef );
    
    // PROJ-1705
    static IDE_RC sortUpdateColumn( qcStatement   * aStatement,
                                    qcmColumn    ** aColumns,
                                    UInt            aColumnCount,
                                    qmmValueNode ** aValue,
                                    qmmValueNode ** aValuesPos );

    // PROJ-2002 Column Security
    static IDE_RC addDecryptFunc4ValueNode( qcStatement  * aStatement,
                                            qmmValueNode * aValueNode );

    /* PROJ-1988 Implement MERGE statement */
    static IDE_RC searchColumnInExpression(	qcStatement  * aStatement,
                                            qtcNode      * aExpression,
                                            qcmColumn    * aColumn,
                                            idBool       * aFound );

    /* PROJ-2219 Row-level before update trigger */
    static IDE_RC makeNewUpdateColumnList( qcStatement * aQcStmt );

    /* BUG-45825 */
    static IDE_RC notAllowedAnalyticFunc( qcStatement * aStatement,
                                          qtcNode     * aNode );

    /* BUG-46174 */
    static IDE_RC parseSPVariableValue( qcStatement   * aStatement,
                                        qtcNode       * aSPVariable,
                                        qmmValueNode ** aValues);

    /* BUG-46702 */
    static IDE_RC convertWithRoullupToRollup( qcStatement * aStatement,
                                              qmsSFWGH    * aSFWGH );

    static IDE_RC checkUpdatableView( qcStatement * aStatement,
                                      qmsFrom     * aFrom );

    static IDE_RC searchColumnInFromTree( qcStatement *  aStatement,
                                          qcmColumn   *  aColumn,
                                          qmsFrom     *  aFrom,
                                          qmsTableRef ** aTableRef );

    static IDE_RC makeUpdateRowForMultiTable( qcStatement * aStatement );

    static IDE_RC makeMultiTable( qcStatement        * aStatement,
                                  qmsFrom            * aFrom,
                                  qcNamePosList      * aDelList,
                                  qmmDelMultiTables ** aTableList );

    static IDE_RC makeNewUpdateColumnListMultiTable( qcStatement    * aQcStmt,
                                                     qmmMultiTables * aTable );

    static IDE_RC getRefColumnListMultiTable( qcStatement     * aQcStmt,
                                              qmmMultiTables  * aTable,
                                              UChar          ** aRefColumnList,
                                              UInt            * aRefColumnCount );

    static void setTableNameForMultiTable( qtcNode        * aNode,
                                           qcNamePosition * aTableName );

    static IDE_RC makeAndSetMultiTable( qcStatement     * aStatement,
                                        qmmUptParseTree * aParseTree,
                                        qmsTableRef     * aTableRef,
                                        qcmColumn       * aColumn,
                                        qmmValueNode    * aValue,
                                        UInt              aValueCount,
                                        qmmMultiTables ** aTableList );

    inline static IDE_RC checkUsableTable( qcStatement     * aStatement,
                                           qcmTableInfo    * aTableInfo );
};

IDE_RC qmv::checkUsableTable( qcStatement     * aStatement,
                              qcmTableInfo    * aTableInfo )
{
/***********************************************************************
 *
 * TASK-7307 DML Data Consistency in Shard
 *   Shard Meta, Backup, Clone 테이블은 세션별로 readonly가 다르게 처리됨.
 *
 *   META, BACKUP 테이블
 *     사용자는 write 불가.
 *     shard_internal_local_operation >= 1 이면, 위의 조건들과 관련없이 write 허용
 *
 *   CLONE 테이블
 *     node added/joined 이 아닌 상태에서는 write 불가
 *     global_transaction_level != 3 에서는 write 불가
 *     shard_internal_local_operation >= 1 이면, 위의 조건들과 관련없이 write 허용
 *
 ***********************************************************************/
    IDE_TEST_CONT( SDU_SHARD_ENABLE != 1, normal_exit );

    IDE_TEST_CONT( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( aStatement ) == ID_TRUE, normal_exit );

    if ( QCM_TABLE_IS_SHARD_META(aTableInfo->mShardFlag) == ID_TRUE )
    {
        IDE_TEST_RAISE( ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_ALTER_META_MASK )
                           != QC_SESSION_ALTER_META_ENABLE ) &&
                        //( aStatement->session->mMmSession != NULL ) &&
                        ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_ALLOC_MASK )
                           != QC_SESSION_INTERNAL_ALLOC_TRUE ) &&
                        ( QCG_GET_SESSION_SHARD_SESSION_TYPE( aStatement )
                          != SDI_SESSION_TYPE_COORD ), ERR_NO_GRANT_DML_META_TABLE );
    }
    else if ( QCM_TABLE_IS_SHARD_BACKUP(aTableInfo->mShardFlag) == ID_TRUE )
    {
        IDE_TEST_RAISE( QCG_GET_SESSION_SHARD_SESSION_TYPE( aStatement )
                        != SDI_SESSION_TYPE_COORD, ERR_UNABLE_TO_ACCESS );
    }
    else if ( QCM_TABLE_IS_SHARD_CLONE(aTableInfo->mShardFlag) == ID_TRUE )
    {
        IDE_TEST_RAISE( QCG_GET_SESSION_IS_GCTX( aStatement ) != ID_TRUE,
                        ERR_NOT_GLOBAL_CONSISTENT_TRANSACTION );

        IDE_TEST_RAISE( sdi::getShardStatus() != 1,
                        ERR_NOT_JOINED_SHARD );

        // BUG-48700
        // Local 실행중에는 clone table에 write를 허용하지 않는다.
        // PSM을 compile 중에는 check하지 않는다.
        IDE_TEST_RAISE( (QCG_GET_SESSION_SHARD_IN_PSM_ENABLE(aStatement) == ID_FALSE) &&
                        ((aStatement->spvEnv->createProc == NULL) &&
                         (aStatement->spvEnv->createPkg  == NULL)),
                        ERR_DML_CLONE_TABLE_IN_LOCAL );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DML_META_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NO_GRANT_DML_PRIV_OF_META_TABLE ) );
    }
    IDE_EXCEPTION( ERR_UNABLE_TO_ACCESS );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_BAK_TABLE_WRITE_DENIED ) );
    }
    IDE_EXCEPTION( ERR_NOT_GLOBAL_CONSISTENT_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_TABLE_WRITE_DENIED,
                                  aTableInfo->name,
                                  "No GLOBAL_CONSISTENT_TRANSACTION" ) );
    }
    IDE_EXCEPTION( ERR_NOT_JOINED_SHARD );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_TABLE_WRITE_DENIED,
                                  aTableInfo->name,
                                  "Disjoined Shard node" ) );
    }
    IDE_EXCEPTION( ERR_DML_CLONE_TABLE_IN_LOCAL );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_CLONE_TABLE_WRITE_DENIED,
                                  aTableInfo->name ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif  // _Q_QMV_H_
