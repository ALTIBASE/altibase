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
 
#ifndef __QSX_DATA_TYPES__
#define __QSX_DATA_TYPES__

#include <qc.h>
#include <qsParseTree.h>

// system defined exception number
#define     QSX_INVALID_EXCEPTION_ID        (0)
#define     QSX_CURSOR_ALREADY_OPEN_NO      (-1)
#define     QSX_DUP_VAL_ON_INDEX_NO         (-2)
#define     QSX_INVALID_CURSOR_NO           (-3)
#define     QSX_INVALID_NUMBER_NO           (-4)
#define     QSX_NO_DATA_FOUND_NO            (-5)
#define     QSX_PROGRAM_ERROR_NO            (-6)
#define     QSX_STORAGE_ERROR_NO            (-7)
#define     QSX_TIMEOUT_ON_RESOURCE_NO      (-8)
#define     QSX_TOO_MANY_ROWS_NO            (-9)
#define     QSX_VALUE_ERROR_NO              (-10)
#define     QSX_ZERO_DIVIDE_NO              (-11)

// PROJ-1371 File handling exception number
#define     QSX_INVALID_PATH_NO             (-12)
#define     QSX_INVALID_MODE_NO             (-13)
#define     QSX_INVALID_FILEHANDLE_NO       (-14)
#define     QSX_INVALID_OPERATION_NO        (-15)
#define     QSX_READ_ERROR_NO               (-16)
#define     QSX_WRITE_ERROR_NO              (-17)
#define     QSX_ACCESS_DENIED_NO            (-18)
#define     QSX_DELETE_FAILED_NO            (-19)
#define     QSX_RENAME_FAILED_NO            (-20)

// TASK-7218 Handling shard mult-errors
#define     QSX_SHARD_MULTIPLE_ERRORS_NO    (-21)

#define     QSX_OTHER_SYSTEM_ERROR_NO       (-101)

// user-defined exception을 re-raise할 때 사용.
#define     QSX_USER_DEFINED_EXCEPTION_NO   (-998)

#define     QSX_LATEST_CURSOR_ID            (-999)

#define     QSX_IS_SQL_CURSOR(id) (id == QSX_LATEST_CURSOR_ID)

#if defined(SMALL_FOOTPRINT)
#define     QSX_MAX_CALL_DEPTH              (8)
#else
#define     QSX_MAX_CALL_DEPTH              (64)
#endif

enum qsxFlowControl
{
   QSX_FLOW_NONE = 1,
   QSX_FLOW_RETURN,
   QSX_FLOW_GOTO,
   QSX_FLOW_RAISE,
   QSX_FLOW_EXIT,
   QSX_FLOW_CONTINUE
};

#define QSX_FLOW_ID_INVALID 403

/********************************************************************
 * PROJ-1904 Extend UDT
 ********************************************************************/
typedef struct qsxArrayMemMgr
{
    iduMemListOld     mMemMgr;
    SInt              mNodeSize;
    SInt              mRefCount;
    qsxArrayMemMgr  * mNext;
} qsxArrayMemMgr;

#define QSX_MEM_EXTEND_COUNT   (8)
#define QSX_MEM_FREE_COUNT     (5)

typedef struct qsxAvlNode
{
    SInt                balance;
    SInt                reserved;
    struct qsxAvlNode * link[2];
    UChar               row[1];
} qsxAvlNode;

typedef struct qsxAvlTree
{
    iduMemListOld* memory;
    idvSQL       * statistics;
    qsxAvlNode   * root;
    mtdCompareFunc compare;
    mtcColumn    * keyCol;
    mtcColumn    * dataCol;
    SInt           dataOffset;
    SInt           rowSize;
    SInt           nodeCount;
    SInt           refCount;    // PROJ-1904 Extend UDT
} qsxAvlTree;

typedef struct qsxArrayInfo
{
    qsxAvlTree          avlTree;
    void              * row;
    qsxArrayInfo      * next;
    qcSessionSpecific * qpSpecific;
} qsxArrayInfo;

/********************************************************************
 * BUG-43158 Enhance statement list caching in PSM
 ********************************************************************/

// BUG-43158 Enhance statement list caching in PSM
#define QSX_STMT_LIST_IS_UNUSED( _list_, _idx_ )                                  \
    ( ( (_list_)[ (_idx_) / QC_UCHAR_BIT ] & (0x1 << ((_idx_) % QC_UCHAR_BIT )) ) \
      == 0x0 ? ID_TRUE : ID_FALSE )

#define QSX_STMT_LIST_SET_USED( _list_, _idx_ )                                   \
    (_list_)[ (_idx_) / QC_UCHAR_BIT ] |= (0x1 << ((_idx_) % QC_UCHAR_BIT))


// BUG-43158 Enhance statement list caching in PSM
typedef struct qsxStmtList
{
    qsProcParseTree  * mParseTree;
    void            ** mStmtPool;
    UChar            * mStmtPoolStatus;
    qsxStmtList      * mNext;
} qsxStmtList;

typedef struct qsxStmtList2
{
    qsProcParseTree  * mParseTree;
    void             * mStmtPool[QC_PSM_POOL_DEFAULT];
    UChar              mStmtPoolStatus[QC_PSM_POOL_DEFAULT_STATUS_SIZE];
    qsxStmtList2     * mNext;
} qsxStmtList2;

#endif
