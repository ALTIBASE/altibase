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
 * $Id: qsParseTree.h 89013 2020-10-22 04:09:47Z donlet $
 **********************************************************************/

#ifndef _O_QS_PARSE_TREE_H_
#define _O_QS_PARSE_TREE_H_ 1

#include <qc.h>
#include <qtcDef.h>
#include <qs.h>
#include <qsxLibrary.h>

//----------------------------------------
// PROJ-1359 Trigger
//----------------------------------------

// PSM Statement의 종류
typedef enum
{
    QS_PROC_STMT_NONE = 0,
    QS_PROC_STMT_ASSIGN,
    QS_PROC_STMT_BLOCK,
    QS_PROC_STMT_CASE_WHEN,
    QS_PROC_STMT_CLOSE,
    QS_PROC_STMT_CONTINUE,
    QS_PROC_STMT_COMMIT,        // Invalid in Trigger
    QS_PROC_STMT_CURSOR_FOR,
    QS_PROC_STMT_DELETE,
    QS_PROC_STMT_ELSE,
    QS_PROC_STMT_ELSE_IF,
    QS_PROC_STMT_EXEC_PROC,     // Invalid in Trigger
    QS_PROC_STMT_EXEC_IMM,      // PROJ-1386 Dynamic-SQL
    QS_PROC_STMT_EXIT,
    QS_PROC_STMT_FETCH,
    QS_PROC_STMT_FOR_LOOP,
    QS_PROC_STMT_GOTO,
    QS_PROC_STMT_IF,
    QS_PROC_STMT_IF_ELSE,
    QS_PROC_STMT_INSERT,
    QS_PROC_STMT_LABEL,
    QS_PROC_STMT_LOOP,
    QS_PROC_STMT_OPEN,
    QS_PROC_STMT_OPEN_FOR,      // PROJ-1386 Dynamic-SQL
    QS_PROC_STMT_OPEN_FOR_STATIC_SQL, // BUG-42397 Ref Cursor Static SQL
    QS_PROC_STMT_NULL,
    QS_PROC_STMT_MOVE,
    QS_PROC_STMT_MERGE,
    QS_PROC_STMT_RAISE,
    QS_PROC_STMT_RETURN,
    QS_PROC_STMT_ROLLBACK,      // Invalid in Trigger
    QS_PROC_STMT_SAVEPOINT,     // Invalid in Trigger
    QS_PROC_STMT_SELECT,
    QS_PROC_STMT_SET_TX,        // Invalid in Trigger
    QS_PROC_STMT_THEN,
    QS_PROC_STMT_UPDATE,
    QS_PROC_STMT_WHILE_LOOP,
    QS_PROC_STMT_EXCEPTION,     // BUG-37501
    QS_PROC_STMT_PKG_BLOCK,     // PROC-1073 Package
    QS_PROC_STMT_ENQUEUE,       // BUG-24383 Support enqueue statement at PSM
    QS_PROC_STMT_DEQUEUE,       // BUG-37797 Support dequeue statement at PSM
    QS_PROC_STMT_ROLLBACK_TO_SAVEPOINT, // Invalid in Trigger
    QS_PROC_STMT_END
} qsProcStmtType;

struct qsxProcInfo;
struct qsxPkgInfo;              // PROJ-1073 Package
struct qsProcStmtSql;
struct qsPkgStmts;
struct qsVariables;
struct qsProcException;         // BUG-37501

/*
  [ REF1 ] SUFFIX for parse/plan tree of stored procedure.
  A. struct type name
  s              : linked list with next pointer

  B. variable name
  s              : linked list with next pointer
  ParseTree      : qcParseTree
  Plan           : qmnPlan
  PlanRange      : qsPlanRange which is index to qcTemplate.planFlag
  Pos            : qcNamePosition

  Node           : qtcNode
  TypeNode       : qtcNode
  possibly with MTC_NODE_OPERATOR_LIST flag for row type.
  if MTC_NODE_OPERATOR_LIST is set, "node.arguments" has
  each attributes for row type.
  RowTypeNode    : qtcNode
  This node is *always* row type node.
  MTC_NODE_OPERATOR_LIST flag is *always* set.

  SpecNode       : qtcNode
  MTC_NODE_OPERATOR_LIST flag is *always* set.
  "node.arguments" has each arguments for
  1. cursor declaration parameters.
  2. cursor open arguments.
  3. execute arguments.

  Nodes          : qtcNode with linked list using "node.next" pointer

  Stmt           : qsProcStmts
  Stmts          : qsProcStmts with linked with using "next" pointer

  ID             : SInt
  OID            : qsOID
*/

/*
  [ REF2 ] USAGE of qtcNode's qcNamePosition attributes
  (   userName,       tableName,      columnName )

  A. variable
  (         -1,       lable name,  variable name )
  (         -1,               -1,  variable name )

  B. row type variable on specific user's table.
  (  user name,      table name,   variable name )
  (         -1,      table name,   variable name )

  C. cursor open/for, %ISFOUND, %ISNOTFOUND, %ISOPEN, %ROWCOUNT
  (         -1,      lable name,     cursor name )
  (         -1,              -1,     cursor name )

  D. procedure/function call
  (         -1,       user name,  proc/func name )
  (         -1,              -1,  proc/func name )
*/

enum qsBlockItemType
{
    QS_VARIABLE,
    QS_TRIGGER_NEW_VARIABLE,         // PROJ-1359 Trigger를 위한 Variable
    QS_TRIGGER_OLD_VARIABLE,         // PROJ-1359 Trigger를 위한 Variable
    QS_CURSOR,
    QS_EXCEPTION,
    QS_TYPE,                     // PROJ-1075 UDT
    QS_PRAGMA_AUTONOMOUS_TRANS,  // BUG-38509 autonomous transaction
    QS_PRAGMA_EXCEPTION_INIT     // BUG-41240 EXCEPTION_INIT Pragma
};

/* 차후 추가되는 object type은 QS_OBJECT_MAX 위에 추가할 것 */
enum qsObjectType
{
    QS_PROC = 0,
    QS_FUNC,
    QS_TABLE,
    QS_TYPESET,
    QS_SYNONYM,
    QS_LIBRARY,     // PROJ-1685
    QS_PKG,         // PROJ-1073 Package
    QS_PKG_BODY,    // PROJ-1073 Package
    QS_TRIGGER,
    QS_OBJECT_MAX   // BUG-37293
};

// PROJ-1073 Package
enum qsPkgOption
{
    QS_PKG_ALL = 0,
    QS_PKG_SPEC_ONLY,
    QS_PKG_BODY_ONLY
};

/* BUG-37820 */
enum qsPkgSubprogramType
{
    QS_NONE_TYPE,         // 초기값
    QS_PUBLIC_SUBPROGRAM,
    QS_PRIVATE_SUBPROGRAM
};

#define QS_SET_PARENT_STMT( aCurrStmt, aParentStmt )    \
    { aCurrStmt->parent = aParentStmt; }

// BUG-37655
// PRAGMA RESTRICT_REFERENCES Option 여부
// .flag
#define QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED    (0x00000000)
#define QS_PRAGMA_RESTRICT_REFERENCES_RNDS         (0x00000001)
#define QS_PRAGMA_RESTRICT_REFERENCES_WNDS         (0x00000002)
#define QS_PRAGMA_RESTRICT_REFERENCES_RNPS         (0x00000004)
#define QS_PRAGMA_RESTRICT_REFERENCES_WNPS         (0x00000008)
#define QS_PRAGMA_RESTRICT_REFERENCES_TRUST        (0x00000010)

#define QS_PRAGMA_RESTRICT_REFERENCES_SUBPROGRAM   (0x00000020)

#define QS_CHECK_PRAGMA_RESTRICT_REFERENCES_CONDITION( a , b ) \
    ( (a & b) == b ? ID_TRUE : ID_FALSE )

/* BUG-41847 package의 subprogram을 default value로 사용가능하게 지원 */
enum qsPkgStmtsValidationState
{
    QS_PKG_STMTS_INIT_STATE = 0,
    QS_PKG_STMTS_PARSE_FINISHED,
    QS_PKG_STMTS_VALIDATE_HEADER_FINISHED,
    QS_PKG_STMTS_VALIDATE_BODY_FINISHED
};

// BUG-42322
typedef struct qsObjectNameInfo
{
    SChar * objectType;
    SChar * name;       // user_name.object_name
    SChar * userName;   // user_name
} qsObjectNameInfo;

typedef struct qsRelatedObjects
{
    UInt                  userID;
    UInt                  tableID;
    qsOID                 objectID;

    qmsNamePosition       userName;
    qmsNamePosition       objectName;
    SInt                  objectType;    // TABLE or PROCEDURE, FUNCTION
                                         // enum qsObjectType

    qcNamePosition        objectNamePos; // for checking circular view error

    qsRelatedObjects    * next;
} qsRelatedObjects;

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
typedef struct qsConstraintRelatedProc
{
    UInt                          userID;
    UInt                          tableID;
    UInt                          constraintID;

    UInt                          relatedUserID;
    qcNamePosition                relatedProcName;
} qsConstraintRelatedProc;

typedef struct qsIndexRelatedProc
{
    UInt                          userID;
    UInt                          tableID;
    UInt                          indexID;

    UInt                          relatedUserID;
    qcNamePosition                relatedProcName;
} qsIndexRelatedProc;

// PROJ-1359 Trigger
// Cycle Detection을 위해 갱신이 발생하는 Table을 관리함.
typedef struct qsModifiedTable
{
    UInt                 tableID;    // Modify 대상이 되는 Table
    qsProcStmtType       stmtType;   // Modify 연산의 종류
    qsModifiedTable *    next;
} qsModifiedTable;

typedef struct qsNames
{
    qcNamePosition      namePos;
    qsNames           * next;

    SInt                id;
} qsNames;

typedef struct qsAllVariables
{
    struct qsLabels             * labels;
    struct qsVariableItems      * variableItems;
    idBool                        inLoop;

    qsAllVariables              * next;
} qsAllVariables;

typedef struct qsLabels
{
    qcNamePosition           namePos;
    SInt                     id;           // negative value
    struct qsProcStmts     * stmt;

    qsLabels               * next;
} qsLabels;

// PROJ-1436
// synonym으로 참조되는 PSM의 정보
typedef struct qsSynonymList
{
    SChar                userName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                objectName[QC_MAX_OBJECT_NAME_LEN + 1];
    idBool               isPublicSynonym;    
    qsOID                objectID;
    qsSynonymList  * next;
} qsSynonymList;

// PROJ-1685
typedef struct qsCallSpecParam
{
    qcNamePosition       paramNamePos;
    qcNamePosition       paramPropertyPos;
    qsInOutType          inOutType;
    UShort               table;
    UShort               column;
    qsCallSpecParam    * next; 
} qsCallSpecParam;

typedef struct qsCallSpec
{
    qcNamePosition       userNamePos;
    qcNamePosition       libraryNamePos;
    SChar              * functionName;
    SChar              * fileSpec;
    qsCallSpecParam    * param;
    UInt                 paramCount;
    qsProcType           procType;
    qsxLibraryNode     * libraryNode;
    void               * functionPtr;
} qsCallSpec;

#define QS_INIT_CALL_SPEC( _callspec_ )                \
{                                                      \
    SET_EMPTY_POSITION( (_callspec_)->userNamePos);    \
    SET_EMPTY_POSITION( (_callspec_)->libraryNamePos); \
    (_callspec_)->functionName = NULL;                 \
    (_callspec_)->fileSpec = NULL;                     \
    (_callspec_)->param = NULL;                        \
    (_callspec_)->paramCount = 0;                      \
    (_callspec_)->procType = QS_EXTERNAL_C;            \
    (_callspec_)->libraryNode = NULL;                  \
    (_callspec_)->functionPtr = NULL;                 \
}

// PROJ-1359 Trigger
// PSM의 Body는 Validation시
// PSM의 Block으로 사용될 때와 Trigger의 Action Body로 사용될 때
// 서로 상이한 정보를 필요로 하며, Validation하는 내용도 다르다.

typedef enum {
    QS_PURPOSE_NONE = 0,
    QS_PURPOSE_PSM,
    QS_PURPOSE_TRIGGER,
    QS_PURPOSE_PKG            // PROJ-1073 Package
} qsValidatePurpose;

typedef IDE_RC (*qsParseFunc)  ( qcStatement *,
                                 qsProcStmts * );

typedef IDE_RC (*qsValidateFunc)  ( qcStatement *,
                                    qsProcStmts *,
                                    qsProcStmts *,
                                    qsValidatePurpose );

typedef IDE_RC (*qsOptimizeFunc)  ( qcStatement *,
                                    qsProcStmts * );

struct qsxExecutorInfo;

typedef IDE_RC (*qsExecuteFunc)   ( qsxExecutorInfo *,
                                    qcStatement *,
                                    qsProcStmts * );

// PROJ-1335, To fix PR-12475
// GOTO 에서 참조할 label을 childLabels에 저장,
// 변수 scope제어 및 exit를 위한 label을 parentLabels에 저장
typedef struct qsProcStmts
{
    qsProcStmts          * next;
    qsProcStmts          * parent;
    qcNamePosition         pos;
    qsProcStmtType         stmtType;
    SInt                   lineno;  // fix BUG-36522
    idBool                 useDate; // fix BUG-36902

    struct qsLabels      * parentLabels;
    struct qsLabels      * childLabels;

    // function pointer
    qsParseFunc            parse;
    qsValidateFunc         validate;
    qsOptimizeFunc         optimize;
    qsExecuteFunc          execute;
} qsProcStmts;

typedef struct qsProcStmtBlock
{
    qsProcStmts                   common;

    struct qsVariableItems      * variableItems;

    qsProcStmts                 * bodyStmts;
    qsProcStmts                 * exception;

    /* BUG-38509 autonomous transaction
       autonomousTrsns의 default는 ID_FALSE */
    idBool                        isAutonomousTransBlock; 
} qsProcStmtBlock;

// identification
//      - parameter   [table, column]
//      - variable    [table, column]
//      - cursor      [table, column]
//      - exception   [id]              : unique negative value in procedure
//      - label       [id]              : unique negative value in procedure

typedef struct qsVariableItems
{
    qsBlockItemType     itemType;
   
    // identification
    UShort              table;
    UShort              column;
    qsOID               objectID;        // PROJ-1073 Package

    qcNamePosition      name;

    SInt                lineno;          // BUG-42322

    qsVariableItems   * next;
} qsVariableItems;

// PROJ-1075 RECORD
// common : 공통item
// columns : record타입이나 array타입의 배열요소
// idxcolumn : indexed by ... 의 type
// name : 타입검색시 사용. package와 연관이 있으므로 이름을 직접 복사해서 보유
// tableInfo : a.b 또는 a(4).b 등 컬럼검색시 필요한 정보
// nestedType : array type이 record를 포함하는 경우 필요한 정보.
// array type인 경우 첫번째 컬럼은 index column이 된다.
// array type인 경우 columnCount는 항상 2이다.
typedef struct qsTypes
{
    qsVariableItems      common;
    qcmColumn          * columns;     // for record/row/array type
    qsVariableItems    * fields;      // BUG-36772, for record
    UInt                 columnCount; // for record/rowtype
    qsVariableType       variableType;
    SInt                 typeSize;
    UInt                 flag;        // PROJ-1904 Extend UDT
    qcmTableInfo       * tableInfo;   // for rowtype.
    qtcModule          * typeModule;
    SChar                name[ QC_MAX_OBJECT_NAME_LEN + 1];
} qsTypes;

typedef struct qsVariables
{
    qsVariableItems     common;
    qtcNode           * variableTypeNode;
    qtcNode           * defaultValueNode;
    qsVariableType      variableType;
    qsInOutType         inOutType; // PROJ-1075, for parameter
    qsNocopyType        nocopyType;
    UInt                resIdx; // PROJ-1075, for array type
    qsTypes           * typeInfo; // PROJ-1075, for row/record/array type
} qsVariables;

typedef struct qsUsingParam
{
    qtcNode              * paramNode;
    qsInOutType            inOutType;
    struct qsUsingParam  * next;
} qsUsingParam;

// BUG-37333
typedef struct qsUsingSubprograms
{
    qtcNode            * subprogramNode;
    qsUsingSubprograms * next;
} qsUsingSubprograms;

/*
  [ example ]
  cursor c1 ( p1 integer, p2 integer ) is <select statement>

  [ parse tree ]
  namePos         = c1
  paraDecls       = ( p1 integer, p2 integer )
  selectParseTree = <select statement>
*/
typedef struct qsCursors
{
    qsVariableItems       common;

    // BUG-20578 index to qcTemplate.procSqlCount
    UInt                  sqlIdx;

    qsProcStmtSql       * mCursorSql;

    // has cursor data module.
    qtcNode             * cursorTypeNode;

    qsVariableItems     * paraDecls;
    qcStatement         * statement; // consider cursor memory

    qcmTableInfo        * tableInfo;

    //fix PROJ-1596
    qcNamePosition        pos;
    UInt                  userID;    // BUG-38164
} qsCursors;

typedef struct qsExceptionDefs
{
    qsVariableItems        common;

    qsOID                  objectID;             // PROJ-1073 Package
    SInt                   id;                   // negative value
    /* BUG-41240 EXCEPTION_INIT Pragma */
    UInt                   errorCode;
    UInt                   userErrorCode;
} qsExceptionDefs;

#define QS_INIT_EXCEPTION_VAR( _stmt_ , _exceptionVar_ )        \
{                                                               \
    (_exceptionVar_)->id = qsvEnv::getNextId((_stmt_)->spvEnv); \
    (_exceptionVar_)->objectID = QS_EMPTY_OID;                  \
    (_exceptionVar_)->errorCode     = 0;                        \
    (_exceptionVar_)->userErrorCode = 0;                        \
}

typedef struct qsExceptions
{
    qcNamePosition         userNamePos;          // PROJ-1073 Package
    qcNamePosition         labelNamePos;
    qcNamePosition         exceptionNamePos;

    qsExceptions         * next;

    /* internal states */
    qsOID                  objectID;             // PROJ-1073 Package
    SInt                   id;                   // negative value
    UInt                   errorCode;
    /* BUG-41240 EXCEPTION_INIT Pragma */
    idBool                 isSystemDefinedError;
    UInt                   userErrorCode;
} qsExceptions;

typedef struct qsProcStmtSql
{
    qsProcStmts             common;

    // BUG-20578 index to qcTemplate.procSqlCount
    UInt                    sqlIdx;
    UInt                    sqlTextLen;
    SChar                 * sqlText;
    
    qcParseTree           * parseTree;
    qcStatement           * statement;
    // BUG-34288
    // if exists subquery인 경우
    idBool                  isExistsSql;

    // for select only
    qmsInto               * intoVariables;
    qmsFrom               * from;                // BUG-36988

    qsUsingParam          * usingParams;
    idBool                  isIntoVarRecordType;
    UInt                    usingParamCount;

    // BUG-37333
    qsUsingSubprograms    * usingSubprograms; 
    // BUG-46174
    qtcNode               * usingRecValueInsUpt;
} qsProcStmtSql;

// BUG-34288 if exists subquery
enum qsIfConditionType
{
    QS_CONDITION_NORMAL = 0,
    QS_CONDITION_EXISTS,        // if exists (subquery) then ...
    QS_CONDITION_NOT_EXISTS     // if not exists (subquery) then ...
};

typedef struct qsProcStmtIf
{
    qsProcStmts            common;

    // BUG-34288 if exists subquery
    qsIfConditionType      conditionType;
    qtcNode              * conditionNode;  // normal condition
    qsProcStmtSql        * existsSql;      // (not)exists subquery condition

    qsProcStmts          * thenStmt;
    qsProcStmts          * elseStmt;
} qsProcStmtIf;

// PROJ-1335, PR-12475 GOTO 문을 지원하기 위해 추가함.
typedef struct qsProcStmtThen
{
    qsProcStmts            common;
    qsProcStmts          * thenStmts;
} qsProcStmtThen;

// PROJ-1335, PR-12475 GOTO 문을 지원하기 위해 추가함.
typedef struct qsProcStmtElse
{
    qsProcStmts            common;
    qsProcStmts          * elseStmts;
} qsProcStmtElse;

typedef struct qsProcStmtWhile
{
    qsProcStmts            common;
    qtcNode              * conditionNode;
    qsProcStmts          * loopStmts;
} qsProcStmtWhile;

/*
  stepSizeNode is *always* > 0
  lowerNode is *always* <= upperNode

  conditionNode  :   lowerNode <= counterNode AND counterNode <= upperNode
  newCounterNode :   counterNode + stepSizeNode
  counterNode - stepSizeNode ( reverse )
*/
typedef struct qsProcStmtFor
{
    qsProcStmts            common;

    qtcNode              * counterVar;

    // To Fix PR-11391
    qtcNode              * lowerVar;
    qtcNode              * upperVar;
    qtcNode              * stepVar;

    idBool                 isReverse;
    qtcNode              * lowerNode;
    qtcNode              * upperNode;

    qtcNode              * stepNode;
    qsProcStmts          * loopStmts;

    /* internal states */
    // made on parse phase
    qtcNode              * isStepOkNode;       // stepNode > 0

    qtcNode              * isIntervalOkNode;   // lowerNode <= upperNode

    // counterVar + stepVar
    qtcNode              * newCounterNode;

    // counterNode BETWEEN lowerNode AND upperNode
    qtcNode              * conditionNode;
} qsProcStmtFor;

/*

  [ example ]
  for r in label_name.cursor_name (v1, v2)
  loop
  <statements>
  end loop;

  [ parse tree ]
  counterNode                   = r
  counterNode.columnName        = "r"
  see REF2.B shown above.

  cursorSpecNode                = label_name.cursor_name (v1, v2)
  cursorSpecNode.userName       = label_name
  cursorSpecNode.tableName      = cursor_name
  cursorSpecNode.node.arguments = (v1, v2)

  loopStmts = <statements>
*/

typedef struct qsProcStmtCursorFor
{
    qsProcStmts            common;

    qtcNode              * counterRowTypeNode; // RowTypeNode
    qtcNode              * openCursorSpecNode;

    qsProcStmts          * loopStmts;

    qsVariables          * rowVariable;

    UInt                   sqlIdx; // BUG-38767
} qsProcStmtCursorFor;

/*
  [ example 1 ]
  EXIT loop_label_name ;

  [ parse tree 1 ]
  labelNamePos   = loop_label_name
  conditionNode  = NULL


  [ example 2 ]
  EXIT loop_label_name WHEN < boolean expression >;

  [ parse tree 2 ]
  labelNamePos   = loop_label_name
  conditionNode  = < boolean expression >

*/


typedef struct qsProcStmtExit
{
    qsProcStmts            common;

    qcNamePosition         labelNamePos;
    qtcNode              * conditionNode;

    /* internal states */
    SInt                   labelID;
    qsProcStmts          * stmt;
} qsProcStmtExit;

typedef struct qsProcStmtContinue
{
    qsProcStmts            common;
} qsProcStmtContinue;

/*
  UNSUPPORTED !!
  [ example ]
  goto label_name;

  [ parse tree ]
  labelNamePos = label_name
*/

typedef struct qsProcStmtGoto
{
    qsProcStmts            common;

    qcNamePosition         labelNamePos;

    SInt                   labelID;
} qsProcStmtGoto;

/*
  [ example ]
  null;

*/


typedef struct qsProcStmtNull
{
    qsProcStmts            common;
} qsProcStmtNull;

/*
  [ example ]
  open label_name.cursor_name ( v1, v2 );

[ parse tree ]
    openSpecNode.userName        = label_name
    openSpecNode.tableName       = cursor_name
    openSpecNode.node.lflag     |= MTC_NODE_OPERATOR_LIST
    openSpecNode.node.arguments  = ( v1, v2 )
 */
typedef struct qsProcStmtOpen
{
    qsProcStmts            common;
    qtcNode              * openCursorSpecNode;
    UInt                   sqlIdx; // BUG-38767
} qsProcStmtOpen;

/*
  [ example ]
  FETCH label_name.cursor_name INTO v1, v2;

  [ parse tree ]
  lableNamePos      = label_name
  cursorNamePos     = cursor_name
  intoVariableNodes = v1, v2      ( linked list with "node.next" pointer )
*/
typedef struct qsProcStmtFetch
{
    qsProcStmts            common;

    qtcNode              * cursorNode;

    /* linked list with intoVariableNodes.node.next */
    qmsInto              * intoVariableNodes;
    idBool                 isIntoVarRecordType;
    idBool                 isRefCursor;
    UInt                   intoVarCount;

    qmsLimit             * limit; /* BUG-41242 */
} qsProcStmtFetch;

/*
  [ example ]
  CLOSE label_name.cursor_name;

  [ parse tree ]
  lableNamePos      = label_name
  cursorNamePos     = cursor_name
*/

typedef struct qsProcStmtClose
{
    qsProcStmts            common;

    qtcNode              * cursorNode;
    idBool                 isRefCursor;
} qsProcStmtClose;


/*
  [ example ]
  user_name1 . variable_name1  :=  user_name2 . variable_name2;

  [ parse tree ]
  leftNode            = user_name1 . variable_name1
  leftNode.userName   = user_name1
  leftNode.tableName  = variable_name1

  rightNode           = user_name2 . variable_name2
  rightNode.userName  = user_name2
  rightNode.tableName = variable_name2
*/
typedef struct qsProcStmtAssign
{
    qsProcStmts            common;

    // PROJ-1904 Extend UDT
    idBool                 copyRef;

    qtcNode              * leftNode;
    qtcNode              * rightNode;
} qsProcStmtAssign;


/*
  [ example ]
  raise lable_name . exception_name;

  [ parse tree ]
  labelNamePos       = lable_name
  exceptionNamePos   = exception_name
*/
typedef struct qsProcStmtRaise
{
    qsProcStmts         common;

    qsExceptions*       exception;
} qsProcStmtRaise;

/*
  [ example ]
  return <expression>;

  [ parse tree ]
  expressionNode     = <expression>
*/
typedef struct qsProcStmtReturn
{
    qsProcStmts            common;

    qtcNode              * returnNode;
} qsProcStmtReturn;

/*
  [ example ]
  label_name :

  [ parse tree ]
  labelNamePos       = label_name
*/
typedef struct qsProcStmtLabel
{
    qsProcStmts            common;

    qcNamePosition         labelNamePos;

    /* internal states */
    SInt                   id;
} qsProcStmtLabel;

// PROJ-1386 Dynamic-SQL
typedef struct qsProcStmtExecImm
{
    qsProcStmts            common;
    qtcNode              * sqlStringNode;
    qmsInto              * intoVariableNodes;
    qsUsingParam         * usingParams;
    idBool                 isIntoVarRecordType;
    UInt                   usingParamCount;
} qsProcStmtExecImm;

typedef struct qsProcStmtOpenFor
{
    qsProcStmts            common;
    qtcNode              * refCursorVarNode;
    qtcNode              * sqlStringNode;
    qsProcStmtSql        * staticSql; // BUG-42397 Ref Cursor Static SQL
    qsUsingParam         * usingParams;
    UInt                   usingParamCount;
    UInt                   sqlIdx; // BUG-38767
} qsProcStmtOpenFor;

/*
  [ example ]
  WHEN label_name1.exception_name1 ,  label_name2.exception_name2 THEN
  < statements >

  [ parse tree ]
  exceptions    = label_name1.exception_name1 ,  label_name2.exception_name2
  liked list with "exceptions.next" pointer

  exceptions.labelNamePos          = label_name1
  exceptions.exceptionNamePos      = exception_name1

  exceptions.next.labelNamePos     = label_name2
  exceptions.next.exceptionNamePos = exception_name2
*/

typedef struct qsExceptionHandlers
{
    qsExceptions         * exceptions;
    qsProcStmts          * actionStmt;

    qsExceptionHandlers  * next;
} qsExceptionHandlers;

// BUG-37501
typedef struct qsProcStmtException
{
    qsProcStmts            common;
    qsExceptionHandlers  * exceptionHandlers;
} qsProcStmtException;

/* BUG-40124 EXCEPTION INIT Pragma */
typedef struct qsPragmaExceptionInit
{
    qsVariableItems   common;
    qcNamePosition    exceptionName;
    UInt              userErrorCode;
} qsPragmaExceptionInit;

// CREATE (OR REPLACE) PROCEDURE, CREATE (OR REPLACE) FUNCTION
typedef struct qsProcParseTree
{
    qcParseTree                 common;

    SChar                     * stmtText;
    SInt                        stmtTextLen;

    qsObjectNameInfo            objectNameInfo;

    qcNamePosition              userNamePos;
    qcNamePosition              procNamePos;

    /* if rowtype, returnTypeNode.node.flag has MTC_NODE_OPERATOR_LIST */
    /* type + data position */
    struct qsVariables        * returnTypeVar;
    mtdModule                 * returnTypeModule;
    mtcColumn                 * returnTypeColumn;

    idBool                      paramUseDate;    // BUG-37854
    UInt                        paraDeclCount;
    struct qsVariableItems    * paraDecls;
    // array of modules in paraTypeNode of qsParaDeclares.
    mtdModule                ** paramModules;
    mtcColumn                 * paramColumns;

    struct qsProcStmtBlock    * block;

    qtcNode                   * sqlCursorTypeNode;

    // PROJ-1075 typeset추가로 parsing과정에서
    // object type을 설정함.
    qsObjectType                objType;

    UInt                        userID;
    qsOID                       procOID;
    /* BUG-38720
       package의 subprogram 일 때, package body의 OID */
    qsOID                       pkgBodyOID;

    //fix PROJ-1596
    qsxProcInfo               * procInfo;
    
    // BUG-21761
    // N타입을 U타입으로 변형시킬 때 사용
    qcNamePosList             * ncharList;
   
    // PROJ-1685
    qsCallSpec                * expCallSpec;

    qsProcType                  procType;

    /* BUG-39770
       package를 참조하거나 package를 참조하는 객체를 참조하는 경우,
       parallel로 수행되면 안 된다. */
    idBool                      referToPkg;

    idBool                      isCachable;     // PROJ-2452

    /* PROJ-1090 Function-based Index */
    idBool                      isDeterministic;

    /* BUG-45306 PSM AUTHID */
    idBool                      isDefiner;

    // BUG-43158 Enhance statement list caching in PSM
    UInt                        procSqlCount;

    // BUG-47971 Package global variable의 분산 실행을 제한합니다.
    idBool                      referToPkgVar;
} qsProcParseTree;

#define QS_PROC_PARSE_TREE_INIT( _dst_ )            \
{                                                   \
    (_dst_)->objectNameInfo.objectType = NULL;      \
    (_dst_)->objectNameInfo.name       = NULL;      \
    SET_EMPTY_POSITION( (_dst_)->userNamePos );     \
    SET_EMPTY_POSITION( (_dst_)->procNamePos );     \
    (_dst_)->isDeterministic = ID_FALSE;            \
    (_dst_)->isDefiner       = ID_TRUE;             \
    (_dst_)->paramUseDate    = ID_FALSE;            \
    (_dst_)->procOID         = QS_EMPTY_OID;        \
    (_dst_)->pkgBodyOID      = QS_EMPTY_OID;        \
    (_dst_)->procInfo        = NULL;                \
    (_dst_)->ncharList       = NULL;                \
    (_dst_)->expCallSpec     = NULL;                \
    (_dst_)->procType        = QS_NORMAL;           \
    (_dst_)->referToPkg      = ID_FALSE;            \
    (_dst_)->isCachable      = ID_FALSE;            \
    (_dst_)->procSqlCount    = 0;                   \
    (_dst_)->referToPkgVar   = ID_FALSE;            \
}


// ALTER FUNCTION, ALTER PROCEDURE
typedef struct qsAlterParseTree
{
    qcParseTree             common;

    qcNamePosition          userNamePos;
    qcNamePosition          procNamePos;

    UInt                    userID;
    qsOID                   procOID;
    qsObjectType            objectType; /* BUG-39059 */
} qsAlterParseTree;

#define QS_ALTER_PARSE_TREE_INIT( _dst_ )               \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->userNamePos);         \
        SET_EMPTY_POSITION(_dst_->procNamePos);         \
        _dst_->userID = 0;                              \
    }


/*

  [ example ]
  DROP FUNCTION  [user_name.]function_name
  namePos = function_name

  [ example ]
  DROP PROCEDURE [user_name.]procedure_name
  namePos = procedure_name

*/
typedef struct qsDropParseTree
{
    qcParseTree             common;

    qcNamePosition          userNamePos;
    qcNamePosition          procNamePos;

    UInt                    userID;
    qsOID                   procOID;
    qsObjectType            objectType; /* BUG-39059 */
} qsDropParseTree;

#define QS_DROP_PARSE_TREE_INIT( _dst_ )                \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->userNamePos);         \
        SET_EMPTY_POSITION(_dst_->procNamePos);         \
        _dst_->userID = 0;                              \
    }

// PROJ-1552
typedef struct qsRecPointParseTree
{
    qcParseTree       common;

    SChar           * recPointID;               // recovery point ID
    UInt              recPointIDLen;
    SChar           * fileName;
    UInt              fileNameLen;
    UInt              lineNo;
    SChar           * testType;               // test type
    UInt              testTypeLen;
    UInt              applyValue;             // decrease or random value
    UInt              sleepSecond;            // sleep second
    UInt              skipFlagAtStartup;      // skip flag in server startup

} qsRecPointParseTree;

// PROJ-1073 Package
typedef IDE_RC (*qsPkgParseFunc)           ( qcStatement *,
                                             qsPkgStmts * );

typedef IDE_RC (*qsPkgValidateHeaderFunc)  ( qcStatement *,
                                             qsPkgStmts * );

typedef IDE_RC (*qsPkgValidateBodyFunc)    ( qcStatement *,
                                             qsPkgStmts *,
                                             qsValidatePurpose );

typedef IDE_RC (*qsPkgOptimizeFunc)        ( qcStatement *,
                                             qsPkgStmts * );

typedef IDE_RC (*qsPkgExecuteFunc)         ( qsxExecutorInfo *,
                                             qcStatement *,
                                             qsPkgStmts * );

typedef struct qsPkgStmts
{
    qsPkgStmts                * next;
  
    qcNamePosition              pos;
    qsObjectType                stmtType;
    qsPkgStmtsValidationState   state;
    // BUG-37655
    // PRAGMA RestrictReference option flag
    UInt                        flag;
   
    qsPkgParseFunc              parse;
    qsPkgValidateHeaderFunc     validateHeader;
    qsPkgValidateBodyFunc       validateBody;
    qsPkgOptimizeFunc           optimize;
    qsPkgExecuteFunc            execute;
} qsPkgStmts;

typedef struct qsPkgSubprograms
{
    qsPkgStmts        common;

    UInt                  subprogramID;
    SChar               * subprogramText;
    UInt                  subprogramTextLen;
    qsPkgSubprogramType   subprogramType;  /* BUG-37820 */
    qsProcParseTree     * parseTree;
    qcStatement         * statement;
} qsPkgSubprograms;

// BUG-37655
typedef struct qsRestrictReferencesOption
{
    UInt   option;
    qsRestrictReferencesOption       * next;
} qsRestrictReferenceOption;

typedef struct qsRestrictReferences
{
    qsPkgStmts                  common;

    qcNamePosition              subprogramName;
    qsRestrictReferenceOption * options;
} qsRestrictReference;

typedef struct qsPkgStmtBlock
{
    qsProcStmts           common;

    qsVariableItems     * variableItems;      // variable declaration
    qsPkgStmts          * subprograms;        // subprogram declaration & defination
    qsProcStmts         * bodyStmts;          // body의 initialize section
    qsProcStmts         * exception;          // body의 initialize section
} qsPkgStmtBlock;

typedef struct  qsPkgParseTree
{
    qcParseTree           common;
    SChar               * stmtText;
    SInt                  stmtTextLen;

    UInt                  subprogramCount;
    qsPkgStmtBlock      * block;

    qsObjectNameInfo      objectNameInfo; 

    qcNamePosition        userNamePos;
    qcNamePosition        pkgNamePos;

    qsObjectType          objType;
    UInt                  userID;
    qsOID                 pkgOID;

    qcNamePosList       * ncharList;
    qtcNode             * sqlCursorTypeNode;

    /* BUG-45306 PSM AUTHID */
    idBool                isDefiner;

    qsxPkgInfo          * pkgInfo;
} qsPkgParseTree;

#define QS_PKG_PARSE_TREE_INIT( _dst_ )            \
{                                                  \
    (_dst_)->objectNameInfo.objectType = NULL;     \
    (_dst_)->objectNameInfo.name       = NULL;     \
    SET_EMPTY_POSITION( (_dst_)->userNamePos );    \
    SET_EMPTY_POSITION( (_dst_)->pkgNamePos );     \
    (_dst_)->isDefiner                 = ID_TRUE;  \
    (_dst_)->subprogramCount           = 0;        \
    (_dst_)->pkgInfo                   = NULL;     \
    (_dst_)->ncharList                 = NULL;     \
}                                               

// ALTER PACKAGE
typedef struct qsAlterPkgParseTree
{
    qcParseTree       common;

    qcNamePosition    userNamePos;
    qcNamePosition    pkgNamePos;
    qsPkgOption       option;       // NULL, PACKAGE, SPECIFICATION, BODY

    UInt              userID;
    qsOID             specOID;
    qsOID             bodyOID;
} qsAlterPkgParseTree;

#define QS_ALTER_PKG_PARSE_TREE_INIT( _dst_ )       \
{                                               \
    SET_EMPTY_POSITION( _dst_->userNamePos );   \
    SET_EMPTY_POSITION( _dst_->pkgNamePos );    \
    _dst_->userID = 0;                          \
}                                               \

// DROP PACKAGE
typedef struct qsDropPkgParseTree
{
    qcParseTree       common;

    qcNamePosition    userNamePos;
    qcNamePosition    pkgNamePos;
    qsPkgOption       option;       // NULL, BODY

    UInt              userID;
    qsOID             specOID;
    qsOID             bodyOID;
} qsDropPkgParseTree;

#define QS_DROP_PKG_PARSE_TREE_INIT( _dst_ )             \
{                                               \
    SET_EMPTY_POSITION( _dst_->userNamePos );   \
    SET_EMPTY_POSITION( _dst_->pkgNamePos );    \
    _dst_->userID = 0;                          \
}                                               \

#endif /* _O_QS_PARSE_TREE_H_ */
