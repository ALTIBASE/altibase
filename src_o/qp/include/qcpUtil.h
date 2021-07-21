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
 * $Id: qcpUtil.h 82186 2018-02-05 05:17:56Z lswhh $
 **********************************************************************/

#ifndef _Q_QCP_UTIL_H_
#define _Q_QCP_UTIL_H_ 1

#include <qc.h>
#include <qtc.h>
#include <qdParseTree.h>

// BUG-44700 reserved words error msg
typedef struct qcpUtilReservedWordTables
{
    const SChar * mWord;
    UInt          mLen;
    UInt          mReservedType;   // qcply.y파일에서 column_name에 추가되었으면 1 , 추가되지 않았으면 0
} qcpUtilReservedWordTables;

class qcpUtil
{
  public:

    static IDE_RC makeColumns4Queue(
        qcStatement         * aStatement,
        qcNamePosition      * aQueueSize,
        UInt                  aMessageColFlag,
        UInt                  aMessageColInRowLen,
        qdTableParseTree    * aParseTree);

    static IDE_RC makeColumns4StructQueue(
        qcStatement         * aStatement,
        qcmColumn           * aColumns,
        qdTableParseTree    * aParseTree);

    static IDE_RC makeDefaultExpression(
        qcStatement         * aStatement,
        qtcNode            ** aNode,
        SChar               * aString,
        SInt                  aStrlen);

    /* PROJ-1107 Check Constraint 지원 */
    static IDE_RC makeConstraintColumnsFromExpression(
        qcStatement         * aStatement,
        qcmColumn          ** aConstraintColumns,
        qtcNode             * aNode );

    static IDE_RC resetOffset(
        qtcNode             * aNode,
        SChar               * aStmtText,
        SInt                  aOffset);

    /* PROJ-1090 Function-based Index */
    static IDE_RC makeHiddenColumnNameAndPosition(
        qcStatement           * aStatement,
        qcNamePosition          aIndexName,
        qcNamePosition          aStartPos,
        qcNamePosition          aEndPos,
        qdColumnWithPosition  * aColumn );

    static void setLastTokenPosition(
        void            * aLexer,
        qcNamePosition  * aPosition );

    static void setEndTokenPosition(
        void            * aLexer,
        qcNamePosition  * aPosition );

    static void checkReservedWord( const SChar   * aWord,
                                   UInt      aLen,
                                   idBool  * aIsReservedWord );
    
    static IDE_RC buildRecordForReservedWord( idvSQL              * /* aStatistics */,
                                              void                * aHeader,
                                              void                * /* aDumpObj */,
                                              iduFixedTableMemory * aMemory );
};

#endif  // _Q_QCP_UTIL_H_
