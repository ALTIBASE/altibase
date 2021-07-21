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
 
#ifndef _O_QSX_CURSOR_H_
#define _O_QSX_CURSOR_H_ 1

#include <qc.h>
#include <iduMemory.h>
#include <qsParseTree.h>
#include <qsxDef.h>
#include <qsxEnv.h>
#include <qmcCursor.h>
#include <qcd.h>

typedef struct qsxCursorInfo
{
    QCD_HSTMT         hStmt;
    qsVariableItems * mCursorParaDecls;
    vSLong            mRowCount;

    // mRowCountIsNull, mIsOpen, mIsNextRecordExist, mIsEndOfCursor, mIsSQLCursor, mIsNeedStmtFree
    // 를 mFlag로 통합함.
    //
    // * mIsSQLCursor 
    //    BUG-3756
    //      ID_TRUE  : SQL (ex. SQL%ROWCOUNT)
    //      ID_FALSE : USER DEFINED CURSOR
    // * mIsNeedStmtFree
    //    BUG-38767 
    //      statement free가 필요한지 여부
    // * mIsNextRecordExist
    //    next row가 존재하는지 여부
    UInt              mFlag;

    // BUG-44536 Affected row count과 fetched row count를 분리합니다. 
    qciStmtType       mStmtType;

    // for tracking all cursors that is in use
    // to finalize/close all the cursors before transaction commit.
    qsxCursorInfo   * mNext;

    qsCursors       * mCursor;
} qsxCursorInfo;

#define QSX_CURSOR_ROWCOUNT_NULL_MASK      (0x00000001)
#define QSX_CURSOR_ROWCOUNT_NULL_TRUE      (0x00000000)    // default
#define QSX_CURSOR_ROWCOUNT_NULL_FALSE     (0x00000001)

#define QSX_CURSOR_OPEN_MASK               (0x00000002)
#define QSX_CURSOR_OPEN_TRUE               (0x00000002)
#define QSX_CURSOR_OPEN_FALSE              (0x00000000)    // default

#define QSX_CURSOR_NEXT_RECORD_EXIST_MASK  (0x00000004)
#define QSX_CURSOR_NEXT_RECORD_EXIST_TRUE  (0x00000004)
#define QSX_CURSOR_NEXT_RECORD_EXIST_FALSE (0x00000000)    // default

#define QSX_CURSOR_END_OF_CURSOR_MASK      (0x00000008)
#define QSX_CURSOR_END_OF_CURSOR_TRUE      (0x00000008)
#define QSX_CURSOR_END_OF_CURSOR_FALSE     (0x00000000)    // default

#define QSX_CURSOR_SQL_CURSOR_MASK         (0x00000010)
#define QSX_CURSOR_SQL_CURSOR_TRUE         (0x00000010)
#define QSX_CURSOR_SQL_CURSOR_FALSE        (0x00000000)    // default

#define QSX_CURSOR_NEED_STMT_FREE_MASK     (0x00000020)
#define QSX_CURSOR_NEED_STMT_FREE_TRUE     (0x00000020)
#define QSX_CURSOR_NEED_STMT_FREE_FALSE    (0x00000000)    // default

#define QSX_CURSOR_FLAG_INIT  ( QSX_CURSOR_ROWCOUNT_NULL_TRUE |                     \
                                QSX_CURSOR_OPEN_FALSE |                             \
                                QSX_CURSOR_NEXT_RECORD_EXIST_FALSE |                \
                                QSX_CURSOR_END_OF_CURSOR_FALSE |                    \
                                QSX_CURSOR_NEED_STMT_FREE_FALSE )

#define QSX_CURSOR_SET_ROWCOUNT_NULL_TRUE( _cur_ )                                  \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_ROWCOUNT_NULL_MASK ) |       \
                      QSX_CURSOR_ROWCOUNT_NULL_TRUE )                               \
    )

#define QSX_CURSOR_SET_ROWCOUNT_NULL_FALSE( _cur_ )                                 \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_ROWCOUNT_NULL_MASK ) |       \
                      QSX_CURSOR_ROWCOUNT_NULL_FALSE )                              \
    )

#define QSX_CURSOR_SET_NEXT_RECORD_EXIST_TRUE( _cur_ )                              \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_NEXT_RECORD_EXIST_MASK ) |   \
                      QSX_CURSOR_NEXT_RECORD_EXIST_TRUE )                           \
    )

#define QSX_CURSOR_SET_NEXT_RECORD_EXIST_FALSE( _cur_ )                             \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_NEXT_RECORD_EXIST_MASK ) |   \
                      QSX_CURSOR_NEXT_RECORD_EXIST_FALSE )                          \
    )

#define QSX_CURSOR_SET_OPEN_TRUE( _cur_ )                                           \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_OPEN_MASK ) |                \
                      QSX_CURSOR_OPEN_TRUE )                                        \
    )

#define QSX_CURSOR_SET_OPEN_FALSE( _cur_ )                                          \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_OPEN_MASK ) |                \
                      QSX_CURSOR_OPEN_FALSE )                                       \
    )

#define QSX_CURSOR_SET_END_OF_CURSOR_TRUE( _cur_ )                                  \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_END_OF_CURSOR_MASK ) |       \
                      QSX_CURSOR_END_OF_CURSOR_TRUE )                               \
    )

#define QSX_CURSOR_SET_END_OF_CURSOR_FALSE( _cur_ )                                 \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_END_OF_CURSOR_MASK ) |       \
                      QSX_CURSOR_END_OF_CURSOR_FALSE )                              \
    )

#define QSX_CURSOR_SET_SQL_CURSOR_TRUE( _cur_ )                                     \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_SQL_CURSOR_MASK ) |          \
                      QSX_CURSOR_SQL_CURSOR_TRUE )                                  \
    )

#define QSX_CURSOR_SET_SQL_CURSOR_FALSE( _cur_ )                                    \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_SQL_CURSOR_MASK ) |          \
                      QSX_CURSOR_SQL_CURSOR_FALSE )                                 \
    )

#define QSX_CURSOR_SET_NEED_STMT_FREE_TRUE( _cur_ )                                 \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_NEED_STMT_FREE_MASK ) |      \
                      QSX_CURSOR_NEED_STMT_FREE_TRUE )                              \
    )

#define QSX_CURSOR_SET_NEED_STMT_FREE_FALSE( _cur_ )                                \
    (                                                                               \
     (_cur_)->mFlag = ( ( (_cur_)->mFlag & ~QSX_CURSOR_NEED_STMT_FREE_MASK ) |      \
                      QSX_CURSOR_NEED_STMT_FREE_FALSE )                             \
    )

#define QSX_CURSOR_IS_ROWCOUNT_NULL( _cur_ )                                        \
    ( ( ((_cur_)->mFlag) & (QSX_CURSOR_ROWCOUNT_NULL_MASK) ) ==                     \
      QSX_CURSOR_ROWCOUNT_NULL_TRUE ? ID_TRUE : ID_FALSE )

#define QSX_CURSOR_IS_OPEN( _cur_ )                                                 \
    ( ( ((_cur_)->mFlag) & (QSX_CURSOR_OPEN_MASK) ) ==                              \
      QSX_CURSOR_OPEN_TRUE ? ID_TRUE : ID_FALSE )

#define QSX_CURSOR_IS_END_OF_CURSOR( _cur_ )                                        \
    ( ( ((_cur_)->mFlag) & (QSX_CURSOR_END_OF_CURSOR_MASK) ) ==                     \
      QSX_CURSOR_END_OF_CURSOR_TRUE ? ID_TRUE : ID_FALSE )

#define QSX_CURSOR_IS_NEXT_RECORD_EXIST( _cur_ )                                    \
    ( ( ((_cur_)->mFlag) & (QSX_CURSOR_NEXT_RECORD_EXIST_MASK) ) ==                 \
      QSX_CURSOR_NEXT_RECORD_EXIST_TRUE ? ID_TRUE : ID_FALSE )

#define QSX_CURSOR_IS_NEED_STMT_FREE( _cur_ )                                       \
    ( ( ((_cur_)->mFlag) & (QSX_CURSOR_NEED_STMT_FREE_MASK) ) ==                    \
      QSX_CURSOR_NEED_STMT_FREE_TRUE ? ID_TRUE : ID_FALSE )

#define QSX_CURSOR_IS_SQL_CURSOR( _cur_ )                                           \
    ( ( ((_cur_)->mFlag) & (QSX_CURSOR_SQL_CURSOR_MASK) ) ==                        \
      QSX_CURSOR_SQL_CURSOR_TRUE ? ID_TRUE : ID_FALSE )

#define QSX_CURSOR_IS_FOUND( cur )                                                  \
  ( ( (QSX_CURSOR_IS_ROWCOUNT_NULL( (cur) ) == ID_TRUE) ||                          \
      ((cur)->mRowCount <= 0) ||                                                    \
      (QSX_CURSOR_IS_END_OF_CURSOR( (cur) ) == ID_TRUE) ) ?                         \
          ID_FALSE:ID_TRUE                                                          \
  )

#define QSX_CURSOR_SET_ROWCOUNT( cur, count )                                       \
    {                                                                               \
        (cur)->mRowCount = (count);                                                 \
        QSX_CURSOR_SET_ROWCOUNT_NULL_FALSE( (cur) );                                \
    }

#define QSX_CURSOR_SET_ROWCOUNT_NULL( cur )                                         \
    {                                                                               \
        (cur)->mRowCount = 0;                                                       \
        QSX_CURSOR_SET_ROWCOUNT_NULL_TRUE( (cur) );                                 \
    }

#define QSX_CURSOR_INFO_INIT( _cur_ )                                               \
    {                                                                               \
        (_cur_)->hStmt = NULL;                                                      \
        (_cur_)->mCursorParaDecls = NULL;                                           \
        (_cur_)->mRowCount = 0;                                                     \
        (_cur_)->mNext = NULL;                                                      \
        (_cur_)->mCursor = NULL;                                                    \
        (_cur_)->mFlag = QSX_CURSOR_FLAG_INIT;                                      \
        (_cur_)->mStmtType = QCI_STMT_MASK_MAX;                                     \
    }

class qsxCursor
{
public:
    static IDE_RC initialize( qsxCursorInfo * aCurInfo,
                              idBool          aIsSQLCursor );

    static IDE_RC finalize( qsxCursorInfo * aCurInfo,
                            qcStatement   * aQcStmt );

    static void reset( qsxCursorInfo * aCurInfo );

    static IDE_RC setCursorSpec( qsxCursorInfo   * aCurInfo,
                                 qsVariableItems * aCursorParaDecls );

    static IDE_RC open( qsxCursorInfo   * aCurInfo,
                        qcStatement     * aQcStmt,
                        qtcNode         * aOpenArgNodes,
                        qcTemplate      * aCursorTemplate,
                        UInt              aSqlIdx /* BUG-38767 */,
                        idBool            aIsDefiner   /* BUG-45306 PSM AUTHID */ );

    static IDE_RC close( qsxCursorInfo * aCurInfo,
                         qcStatement   * aQcStmt );

    static IDE_RC fetchInto( qsxCursorInfo * aCurInfo,
                             qcStatement   * aQcStmt,
                             qmsInto       * aIntoVarNodes, 
                             qmsLimit      * aLimit /* BUG-41242 */ );
};

#endif /* _O_QSX_CURSOR_H_ */
