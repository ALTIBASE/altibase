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
 * $Id: qs.h 89835 2021-01-22 10:10:02Z andrew.shin $
 **********************************************************************/

#ifndef _O_QS_H_
#define _O_QS_H_ 1

// common structures for stored procedure and native stored procedure.
#include <qc.h>
#include <qtcDef.h>

enum qsInOutType
{
    QS_IN = 0,
    QS_OUT,
    QS_INOUT
};

enum qsNocopyType
{
    QS_COPY = 0,
    QS_NOCOPY
};

enum qsVariableType
{
    QS_NOT_AVAILABLE = 0,
    QS_PRIM_TYPE,
    QS_ROW_TYPE,
    QS_COL_TYPE,
    QS_UD_TYPE,    // PROJ-1075 record/array type추가.
    QS_RECORD_TYPE,
    QS_ASSOCIATIVE_ARRAY_TYPE,
    QS_REF_CURSOR_TYPE
};

// PROJ-1685
// PROJ-2717 Internal procedure
//  1. 기존의 QS_INTERNAL을 QS_NORMAL로 변경
//  2. 기존의 QS_EXTERNAL을 QS_EXTERNAL_C로 변경
//  3. 새로 QS_INTERNAL_C를 추가
enum qsProcType
{
    QS_NORMAL = 0,
    QS_EXTERNAL_C,
    QS_INTERNAL_C
};

typedef smOID qsOID;

#define     QS_EMPTY_OID      ( ( qsOID ) 0 )

// PROJ-1073 Package
#define     QS_INIT_SUBPROGRAM_ID  ( (UInt) 0 )
#define     QS_PSM_SUBPROGRAM_ID   ( (UInt) 0 )
#define     QS_PKG_VARIABLE_ID     ( ID_UINT_MAX )

/* PROJ-2586 PSM Parameters and return without precision
   char type의 default precision에 대해 기존과 동일한 비율로
   bit와 byte의 precision의 default precision 결정 */
#define   QS_BIT_PRECISION_DEFAULT      ( 2 * QCU_PSM_CHAR_DEFAULT_PRECISION )
#define   QS_VARBIT_PRECISION_DEFAULT   ( QS_BIT_PRECISION_DEFAULT )
#define   QS_BYTE_PRECISION_DEFAULT      ( QCU_PSM_CHAR_DEFAULT_PRECISION )
#define   QS_VARBYTE_PRECISION_DEFAULT   ( QS_BYTE_PRECISION_DEFAULT )

/*
  [ example ]
  EXEC(UTE) user_name.procedure_name (...)
  leftNode = NULL

  [ example ]
  EXEC(UTE) :host_variable := user_name.function_name (...)
  leftNode = host_variable

        callSpecNode.tableName  = user_name
        callSpecNode.columnName = procedure_name or function_name

        callSpecNode.node.lflag has MTC_NODE_OPERATOR_LIST flag
        callSpecNode.node.arguments = ...
*/
/*
  !!Caution!!!
  if you add or remove any of fields in qsExecParseTree,
  you have to change both SP_invocation_statement of qcpl.y and
  qtcEstimate of qtcSpFunctionCall.cpp
*/
typedef struct qsExecParseTree
{
    qcParseTree             common;
    qtcNode               * leftNode;
    qtcNode               * callSpecNode;

    iduList                 refCurList;
    
    // array of modules in paraTypeNode of qsParaDeclares.
    // *** Caution ***
    // Because pointer is copied from qsParseTree, Never write to
    // instances pointed by paramModules.
    UInt                    paraDeclCount;
    const mtdModule      ** paramModules;
    mtcColumn             * paramColumns;

    // for getting RETURN type information in RETURN value binding
    // See qci2::setBindColumn
    const mtdModule       * returnModule;
    mtcColumn             * returnTypeColumn;

    /* PROJ-1090 Function-based Index */
    idBool                  isDeterministic;
    
    UInt                    userID;
    /* procedure, function, package spec의 OID */
    qsOID                   procOID;
    /* BUG-38720 package body OID */
    qsOID                   pkgBodyOID;     
    UInt                    subprogramID;
    idBool                  isRootStmt;

    idBool                  isCachable;     // PROJ-2452

    // PROJ-2646 shard analyzer
    struct sdiObjectInfo  * mShardObjInfo;

    /* TASK-7219 Shard Transformer Refactoring */
    struct sdiShardAnalysis * mShardAnalysis;
} qsExecParseTree;

#define     QS_ID_INIT_VALUE   ( -1000 )

#endif /* _O_QS_H_ */
