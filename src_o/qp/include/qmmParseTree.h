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
 * $Id: qmmParseTree.h 89835 2021-01-22 10:10:02Z andrew.shin $
 **********************************************************************/

#ifndef _O_QMM_PARSE_TREE_H_
#define _O_QMM_PARSE_TREE_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qdParseTree.h>
#include <qcmTableInfo.h>

// Proj-1360 Queue
#define QMM_QUEUE_MASK         (0x00000001)
#define QMM_QUEUE_FALSE        (0x00000000)
#define QMM_QUEUE_TRUE         (0x00000001)

#define QMM_MULTI_INSERT_MASK  (0x00000002)
#define QMM_MULTI_INSERT_FALSE (0x00000000)
#define QMM_MULTI_INSERT_TRUE  (0x00000002)

typedef struct qmmValueNode
{
    qtcNode*      value;
    qmmValueNode* next;
    UShort        order;
    idBool        validate;
    idBool        calculate;
    idBool        timestamp;
    idBool        expand;
// Proj-1360 Queue
    idBool        msgID;
} qmmValueNode;

typedef struct qmmValuePointer
{
    qmmValueNode*    valueNode;
    qmmValuePointer* next;
} qmmValuePointer;

typedef struct qmmSubqueries
{
    qtcNode*         subquery;
    qmmValuePointer* valuePointer;
    qmmSubqueries*   next;
} qmmSubqueries;

/* PROJ-1584 DML Returning Clause */
typedef struct qmmReturnValue
{
    qtcNode         * returnExpr;
    qcNamePosition    returningPos;
    qmmReturnValue  * next;
} qmmReturnValue;

typedef struct qmmReturnIntoValue
{
    qtcNode            * returningInto;
    qcNamePosition       returningIntoPos;
    qmmReturnIntoValue * next;
} qmmReturnIntoValue;

typedef struct qmmReturnInto
{
    qmmReturnValue     * returnValue;
    qmmReturnIntoValue * returnIntoValue;
    idBool               bulkCollect;
    qcNamePosition       returnIntoValuePos;
} qmmReturnInto;

/* BUG-42764 Multi Row */
typedef struct qmmMultiRows
{
    qmmValueNode * values;
    qmmMultiRows * next;
} qmmMultiRows;

typedef struct qmmInsParseTree
{
    qcParseTree          common;

    struct qmsTableRef * tableRef;
    void               * tableHandle;

    qcmColumn          * columns;    // insert columns
    qmmMultiRows       * rows;       // insert values BUG-42764 multi row insert

    qcStatement        * select;            // for INSERT ... SELECT ...
    qcmColumn          * columnsForValues;  // for INSERT ... SELECT ...
    qcmParentInfo      * parentConstraints;
    UInt                 valueIdx;          // index to qcTemplate.insOrUptRow
    qtcNode            * spVariable;        // BUG-46174 sp variable in VALUES clause

    /* PROJ-2204 Join Update, Delete */
    struct qmsTableRef * insertTableRef;
    qcmColumn          * insertColumns; 
    
    const mtdModule   ** columnModules;
    UInt                 canonizedTuple;
    // PROJ-2264 Dictionary table
    UInt                 compressedTuple;
    UInt                 flag;
    // Proj - 1360 Queue
    void               * queueMsgIDSeq;

    // PROJ-1566
    // Insert 방식을 APPEND로 할것인지 말것인지에 대한 Hint
    // APPEND 방식 일 경우, Page에 insert될때 마지막에 insert된
    // record 다음에 append되는 방식
    // ( 이때 내부적으로 direct-path INSERT 방식으로 처리됨 )
    qmsHints           * hints;

    // instead of trigger
    idBool               insteadOfTrigger;

    /* PROJ-1584 DML Return Clause */
    qmmReturnInto      * returnInto;

    /* PROJ-1988 MERGE statement */
    qmsQuerySet        * outerQuerySet;

    /* PROJ-1107 Check Constraint 지원 */
    qdConstraintSpec   * checkConstrList;
    
    /* PROJ-1090 Function-based Index */
    struct qmsTableRef * defaultTableRef;
    qcmColumn          * defaultExprColumns;

    // BUG-43063 insert nowait
    ULong                lockWaitMicroSec;

    // BUG-36596 multi-table insert
    struct qmmInsParseTree * next;

    /* TASK-7219 Shard Transformer Refactoring */
    struct sdiShardAnalysis * mShardAnalysis;
} qmmInsParseTree;

typedef struct qmmDelMultiTables
{
    qmsTableRef       * mTableRef;
    qtcNode           * mColumnList;
    qcmRefChildInfo   * mChildConstraints;
    idBool              mInsteadOfTrigger;
    SInt                mViewID;
    smiColumnList     * mWhereColumnList;
    smiColumnList    ** mWherePartColumnList;
    qmmDelMultiTables * mNext;
} qmmDelMultiTables;

typedef struct qmmDelParseTree
{
    qcParseTree          common;

    qmsQuerySet        * querySet;   // table information and where clause

    /* PROJ-2204 Join Update, Delete */
    struct qmsTableRef * deleteTableRef;     // delete target tableRef

    qcmRefChildInfo    * childConstraints;  // BUG-28049

    /* PROJ-1584 DML Return Clause */
    qmmReturnInto      * returnInto; 

    // To Fix PR-12917
    qmsLimit           * limit;

    // PROJ-1888 INSTEAD OF TRIGGER 
    idBool               insteadOfTrigger;

    qcNamePosList      * mDelList;      // lists in multi delete clause
    qmmDelMultiTables  * mTableList;

    /* TASK-7219 Shard Transformer Refactoring */
    struct sdiShardAnalysis * mShardAnalysis;
} qmmDelParseTree;

typedef struct qmmSetClause // temporary struct for parser
{
    qcmColumn          * columns;    // update columns
    qmmValueNode       * values;     // update values
    qmmSubqueries      * subqueries; // subqueries in set clause
    qmmValueNode       * lists;      // lists in set clause
    qtcNode            * spVariable; // BUG-46174 sp variable in set clause
} qmmSetClause;

typedef struct qmmMultiTables
{
    qmsTableRef      * mTableRef;
    qmsTableRef      * mInsertTableRef;
    qcmColumn        * mColumns;
    qtcNode          * mColumnList;
    UInt               mColumnCount;
    UInt             * mColumnIDs;
    qcmColumn        * mDefaultColumns;
    qcmColumn        * mDefaultBaseColumns;
    qmsTableRef      * mDefaultTableRef;
    qmmValueNode     * mValues;     // update values
    qmmValueNode    ** mValuesPos;
    smiColumnList    * mUptColumnList;
    idBool             mInsteadOfTrigger;
    qdConstraintSpec * mCheckConstrList;
    UInt               mCanonizedTuple;
    mtdIsNullFunc    * mIsNull;
    qcmParentInfo    * mParentConst;
    qcmRefChildInfo  * mChildConst;  // BUG-28049
    idBool             mInplaceUpdate;
    SInt               mViewID;
    qmoUpdateType      mUpdateType;
    smiCursorType      mCursorType;
    smiColumnList   ** mPartColumnList;
    smiColumnList    * mWhereColumnList;
    smiColumnList   ** mWherePartColumnList;
    smiColumnList    * mSetColumnList;
    smiColumnList   ** mSetPartColumnList;
    qmmMultiTables   * mNext;
} qmmMultiTables;

typedef struct qmmUptParseTree
{
    qcParseTree          common;

    UInt                 uptColCount;
    qcmColumn          * columns;    // update columns
    qmmValueNode       * values;     // update values
    qmmSubqueries      * subqueries; // subqueries in set clause
    qmmValueNode       * lists;      // lists in set clause
    qtcNode            * spVariable; // BUG-46174 sp variable in set clause
    qmsQuerySet        * querySet;   // table information and where clause
    qcmParentInfo      * parentConstraints;
    qcmRefChildInfo    * childConstraints;  // BUG-28049
    UInt                 valueIdx;   // index to qcTemplate.insOrUptRow
    const mtdModule   ** columnModules;
    UInt                 canonizedTuple;
    // PROJ-2264 Dictionary table
    UInt                 compressedTuple;
    mtdIsNullFunc      * isNull;

    /* PROJ-2204 join update, delete */
    struct qmsTableRef * updateTableRef;
    qcmColumn          * updateColumns;

    /* PROJ-1584 DML Return Clause */
    qmmReturnInto      * returnInto; 

    // To Fix PR-12917
    qmsLimit           * limit;

    // update column list
    smiColumnList      * uptColumnList;

    // PROJ-1888 INSTEAD OF TRIGGER
    idBool               insteadOfTrigger;

    /* PROJ-1107 Check Constraint 지원 */
    qdConstraintSpec   * checkConstrList;

    /* PROJ-1090 Function-based Index */
    struct qmsTableRef * defaultTableRef;
    qcmColumn          * defaultExprColumns;

    /* PROJ-2714 Multiple update, delete support */
    struct qmmMultiTables * mTableList;

    /* TASK-7219 Shard Transformer Refactoring */
    struct sdiShardAnalysis * mShardAnalysis;
} qmmUptParseTree;

typedef struct qmmMoveParseTree
{
    qcParseTree          common;
    struct qmsTableRef * targetTableRef;    // Move Target TableRef
    qcmColumn          * columns;           // Target Table Columns
    qmmValueNode       * values;            // insert values
    qcmParentInfo      * parentConstraints; // parent constraints of target table
    qcmRefChildInfo    * childConstraints;  // child constraints of source table, BUG-28049
    UInt                 valueIdx;
    const mtdModule   ** columnModules;
    UInt                 canonizedTuple;
    // PROJ-2264 Dictionary table
    UInt                 compressedTuple;
    qmsQuerySet        * querySet;          // querySet of source table
   
    struct qmsLimit    * limit;

    /* PROJ-1107 Check Constraint 지원 */
    qdConstraintSpec   * checkConstrList;
    
    /* PROJ-1090 Function-based Index */
    struct qmsTableRef * defaultTableRef;
    qcmColumn          * defaultExprColumns;
    
} qmmMoveParseTree;

typedef struct qmmLockParseTree
{
    qcParseTree          common;

    qcNamePosition       userName;
    qcNamePosition       tableName;
    qcNamePosition       partitionName;

    smiTableLockMode     tableLockMode;
    ULong                lockWaitMicroSec;

    /* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
    idBool               untilNextDDL;

    // validation information
    UInt                 userID;
    qcmTableInfo       * tableInfo;
    void               * tableHandle;
    smSCN                tableSCN;

    /* BUG-46273 Lock Partition */
    qcmTableInfo       * partitionInfo;
    void               * partitionHandle;
    smSCN                partitionSCN;
} qmmLockParseTree;

/* PROJ-1988 Implement MERGE statement */
typedef struct qmmMergeParseTree
{
    qcParseTree           common;

    qcStatement         * updateStatement;
    qcStatement         * deleteStatement;    
    qcStatement         * insertStatement;
    qcStatement         * insertNoRowsStatement;   // BUG-37535
    qcStatement         * selectSourceStatement;
    qcStatement         * selectTargetStatement;

    struct qmsQuerySet  * target;
    struct qmsQuerySet  * source;
    struct qtcNode      * onExpr;

    struct qtcNode      * whereForUpdate;
    struct qtcNode      * whereForDelete;
    struct qtcNode      * whereForInsert;

    qmmUptParseTree     * updateParseTree;
    qmmDelParseTree     * deleteParseTree;
    qmmInsParseTree     * insertParseTree;
    qmmInsParseTree     * insertNoRowsParseTree;   // BUG-37535
    qmsParseTree        * selectSourceParseTree;
    qmsParseTree        * selectTargetParseTree;

} qmmMergeParseTree;

typedef struct qmmMergeActions
{
    struct qtcNode      * whereForUpdate;
    struct qtcNode      * whereForDelete;
    struct qtcNode      * whereForInsert;

    qmmUptParseTree     * updateParseTree;
    qmmDelParseTree     * deleteParseTree;
    qmmInsParseTree     * insertParseTree;
    qmmInsParseTree     * insertNoRowsParseTree;   // BUG-37535

} qmmMergeActions;

/* PROJ-1812 ROLE */
typedef struct qmmJobOrRoleParseTree
{
    qdJobParseTree      * jobParseTree;
    qdUserParseTree     * userParseTree;
    qdDropParseTree     * dropParseTree;
} qmmJobOrRoleParseTree;

#define QMM_INIT_MERGE_ACTIONS(_dst_)         \
{                                             \
    (_dst_)->whereForUpdate        = NULL;    \
    (_dst_)->whereForDelete        = NULL;    \
    (_dst_)->whereForInsert        = NULL;    \
    (_dst_)->updateParseTree       = NULL;    \
    (_dst_)->deleteParseTree       = NULL;    \
    (_dst_)->insertParseTree       = NULL;    \
    (_dst_)->insertNoRowsParseTree = NULL;    \
}

#define QMM_INIT_MULTI_TABLES( _dst_ )         \
{                                              \
    (_dst_)->mTableRef            = NULL;      \
    (_dst_)->mInsertTableRef      = NULL;      \
    (_dst_)->mColumns             = NULL;      \
    (_dst_)->mColumnList          = NULL;      \
    (_dst_)->mColumnCount         = 0;         \
    (_dst_)->mColumnIDs           = NULL;      \
    (_dst_)->mDefaultColumns      = NULL;      \
    (_dst_)->mDefaultBaseColumns  = NULL;      \
    (_dst_)->mDefaultTableRef     = NULL;      \
    (_dst_)->mValues              = NULL;      \
    (_dst_)->mValuesPos           = NULL;      \
    (_dst_)->mUptColumnList       = NULL;      \
    (_dst_)->mInsteadOfTrigger    = ID_FALSE;  \
    (_dst_)->mCheckConstrList     = NULL;      \
    (_dst_)->mCanonizedTuple      = 0;         \
    (_dst_)->mIsNull              = NULL;      \
    (_dst_)->mParentConst         = NULL;      \
    (_dst_)->mChildConst          = NULL;      \
    (_dst_)->mInplaceUpdate       = ID_FALSE;  \
    (_dst_)->mViewID              = -1;        \
    (_dst_)->mUpdateType          = QMO_UPDATE_NORMAL;\
    (_dst_)->mCursorType          = SMI_SELECT_CURSOR;\
    (_dst_)->mPartColumnList      = NULL;      \
    (_dst_)->mWhereColumnList     = NULL;      \
    (_dst_)->mWherePartColumnList = NULL;      \
    (_dst_)->mSetColumnList       = NULL;      \
    (_dst_)->mSetPartColumnList   = NULL;      \
    (_dst_)->mNext                = NULL;      \
}

#define QMM_INIT_DEL_MULTI_TABLES( _dst_ )     \
{                                              \
    (_dst_)->mTableRef            = NULL;      \
    (_dst_)->mColumnList          = NULL;      \
    (_dst_)->mChildConstraints    = NULL;      \
    (_dst_)->mInsteadOfTrigger    = ID_FALSE;  \
    (_dst_)->mViewID              = -1;        \
    (_dst_)->mWhereColumnList     = NULL;      \
    (_dst_)->mWherePartColumnList = NULL;      \
    (_dst_)->mNext                = NULL;      \
}

#endif /* _O_QMM_PARSE_TREE_H_ */

