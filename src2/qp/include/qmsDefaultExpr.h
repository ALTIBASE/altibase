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
 * $Id: qmsDefaultExpr.h 56549 2012-11-23 04:38:50Z sungminee $
 **********************************************************************/

#ifndef _O_QMS_DEFAULT_EXPR_H_
#define _O_QMS_DEFAULT_EXPR_H_ 1

#include <qc.h>
#include <qdParseTree.h>
#include <qmsParseTree.h>
#include <qcmTableInfo.h>
#include <qtcDef.h>

typedef struct qmsExprNode
{
    // transform용 자료
    qmsTableRef  * tableRef;
    qcmColumn    * column;

    // compare용 node tree
    qtcNode      * node;
    
    qmsExprNode  * next;
} qmsExprNode;

class qmsDefaultExpr
{
public:
    
    /* PROJ-1090 Function-based Index */
    static IDE_RC isFunctionBasedIndex( qcmTableInfo * aTableInfo,
                                        qcmIndex     * aIndex,
                                        idBool       * aResult );

    /* PROJ-1090 Function-based Index */
    static IDE_RC isBaseColumn( qcStatement  * aStatement,
                                qcmTableInfo * aTableInfo,
                                SChar        * aDefaultExprStr,
                                UInt           aColumnID,
                                idBool       * aResult );

    /* PROJ-1090 Function-based Index */
    static IDE_RC isRelatedToFunctionBasedIndex( qcStatement  * aStatement,
                                                 qcmTableInfo * aTableInfo,
                                                 qcmIndex     * aIndex,
                                                 UInt           aColumnID,
                                                 idBool       * aResult );
    /* PROJ-1090 Function-based Index */
    static IDE_RC makeColumnListFromExpression( qcStatement   * aStatement,
                                                qcmColumn    ** aColumnList,
                                                qtcNode       * aNode );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC makeFunctionNameListFromExpression( qcStatement         * aStatement,
                                                      qdFunctionNameList ** aFunctionNameList,
                                                      qtcNode             * aTargetNode,
                                                      qtcNode             * aTopNode );

    /* PROJ-1090 Function-based Index */
    static IDE_RC makeBaseColumn( qcStatement    * aStatement,
                                  qcmTableInfo   * aTableInfo,
                                  qcmColumn      * aDefaultExpr,
                                  qcmColumn     ** aBaseColumns );

    /* PROJ-1090 Function-based Index */
    static void setUsedColumnToTableRef( mtcTemplate  * aTemplate,
                                         qmsTableRef  * aTableRef,
                                         qcmColumn    * aColumns,
                                         idBool         aIsList );

    /* PROJ-1090 Function-based Index */
    static void setRowBufferFromBaseColumn( mtcTemplate  * aTemplate,
                                            UShort         aSrcTupleID,
                                            UShort         aDstTupleID,
                                            qcmColumn    * aBaseColumns,
                                            void         * aRowBuffer );
    
    /* PROJ-1090 Function-based Index */
    static IDE_RC setRowBufferFromSmiValueArray( mtcTemplate  * aTemplate,
                                                 qmsTableRef  * aTableRef,
                                                 qcmColumn    * aColumns,
                                                 void         * aRowBuffer,
                                                 smiValue     * aValueArr,
                                                 idBool         aIsValueArrDisk );
    
    /* PROJ-1090 Function-based Index */
    static IDE_RC calculateDefaultExpression( qcTemplate   * aTemplate,
                                              qmsTableRef  * aTableRef,
                                              qcmColumn    * aUpdateColumns,
                                              qcmColumn    * aDefaultExprColumns,
                                              const void   * aRowBuffer,
                                              smiValue     * aValueArr,
                                              qcmColumn    * aTableColumnsForValueArr );
    
    /* PROJ-1090 Function-based Index */
    static IDE_RC makeNodeListForFunctionBasedIndex( qcStatement   * aStatement,
                                                     qmsTableRef   * aTableRef,
                                                     qmsExprNode  ** aDefaultExprList );

    /* PROJ-1090 Function-based Index */
    static IDE_RC findAndReplaceNodeForFunctionBasedIndex( qcStatement   * aStatement,
                                                           qtcNode       * aTargetNode,
                                                           qmsExprNode   * aExprNodeList,
                                                           qtcNode      ** aResultNode,
                                                           idBool        * aNeedToEstimate );

    /* PROJ-1090 Function-based Index */
    static IDE_RC applyFunctionBasedIndex( qcStatement   * aStatement,
                                           qtcNode       * aTargetNode,
                                           qmsFrom       * aStartFrom,
                                           qtcNode      ** aResultNode );
    
    /* PROJ-1090 Function-based Index */
    static IDE_RC addDefaultExpressionColumnsRelatedToColumn(
        qcStatement       * aStatement,
        qcmColumn        ** aDefaultExprColumns,
        qcmTableInfo      * aTableInfo,
        UInt                aColumnID );

    /* PROJ-1090 Function-based Index */
    static IDE_RC makeDefaultExpressionColumnsRelatedToTable(
        qcStatement       * aStatement,
        qcmColumn        ** aDefaultExprColumns,
        qcmTableInfo      * aTableInfo );
};

#endif /* _O_QMS_DEFAULT_EXPR_H_ */
