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
 * $Id: qsxRefCursor.h 23857 2007-10-23 02:36:53Z mycomman $
 **********************************************************************/

#ifndef _O_QSX_REF_CURSOR_H_
#define _O_QSX_REF_CURSOR_H_ 1

#include <qc.h>
#include <qsParseTree.h>
#include <qcd.h>

typedef struct qsxRefCursorInfo
{
    UInt       id;              // ref cursor의 id. 현재는 사용하지 않음
    vSLong     rowCount;        // rowcount
    idBool     rowCountIsNull;  // notfound체크용
    idBool     isOpen;          // open여부
    idBool     isEndOfCursor;   // fetch끝인지 여부
    idBool     nextRecordExist; // next row가 존재하는지 여부
    idBool     isNeedStmtFree;  // BUG-38767 statement free가 필요한지 여부
    QCD_HSTMT  hStmt;           // statement handle
} qsxRefCursorInfo;

class qsxRefCursor 
{
 public :
    static IDE_RC initialize( qsxRefCursorInfo * aRefCurInfo,
                              UShort             aId );
    
    static IDE_RC finalize( qsxRefCursorInfo  * aRefCurInfo );
    
    static IDE_RC openFor( qsxRefCursorInfo   * aRefCurInfo,
                           qcStatement        * aStatement,
                           qsUsingParam       * aUsingParams,
                           UInt                 aUsingParamCount,
                           SChar              * aQueryString,
                           UInt                 aQueryLen,
                           UInt                 aSqlIdx /* BUG-38767 */ );
    
    static IDE_RC close( qsxRefCursorInfo     * aRefCurInfo );

    static IDE_RC fetchInto( qsxRefCursorInfo  * aRefCurInfo,
                             qcStatement       * aQcStmt,
                             qsProcStmtFetch   * aFetch );
};

#define QSX_REF_CURSOR_IS_OPEN( cur )         ( (cur)->isOpen )
    
#define QSX_REF_CURSOR_IS_FOUND( cur )                                    \
  ( ( ((cur)->rowCountIsNull == ID_TRUE) ||                          \
      ((cur)->rowCount <= 0) ||                                      \
      ((cur)->isEndOfCursor == ID_TRUE) ) ?                          \
        ( ID_FALSE )                                                  \
            :                                                         \
        ( ID_TRUE )                                                   \
  )

#define QSX_REF_CURSOR_ROWCOUNT( cur )   ( (cur)->rowCount ) 
#define QSX_REF_CURSOR_ROWCOUNT_IS_NULL( cur ) ( (cur)->rowCountIsNull )


#endif /* _O_QSX_REF_CURSOR_H_ */











