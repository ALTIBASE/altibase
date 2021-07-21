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
 * $Id$
 **********************************************************************/

#ifndef _O_QDN_CHECK_H_
#define _O_QDN_CHECK_H_ 1

#include <qc.h>
#include <qtc.h>
#include <qdParseTree.h>

class qdnCheck
{
public:
    static IDE_RC addCheckConstrSpecRelatedToColumn(
                qcStatement         * aStatement,
                qdConstraintSpec   ** aConstrSpec,
                qcmCheck            * aChecks,
                UInt                  aCheckCount,
                UInt                  aColumnID );

    static IDE_RC makeCheckConstrSpecRelatedToTable(
                qcStatement         * aStatement,
                qdConstraintSpec   ** aConstrSpec,
                qcmCheck            * aChecks,
                UInt                  aCheckCount );

    static IDE_RC renameColumnInCheckConstraint(
                qcStatement      * aStatement,
                qdConstraintSpec * aConstrList,
                qcmTableInfo     * aTableInfo,
                qcmColumn        * aOldColumn,
                qcmColumn        * aNewColumn );

    static IDE_RC setMtcColumnToCheckConstrList(
                qcStatement      * aStatement,
                qcmTableInfo     * aTableInfo,
                qdConstraintSpec * aConstrList );

    static IDE_RC verifyCheckConstraintList(
                qcTemplate       * aTemplate,
                qdConstraintSpec * aConstrList );

    static IDE_RC verifyCheckConstraintListWithFullTableScan(
                qcStatement      * aStatement,
                qmsTableRef      * aTableRef,
                qdConstraintSpec * aCheckConstrList );

    static IDE_RC renameColumnInExpression(
                qcStatement         * aStatement,
                SChar              ** aNewExpressionStr,
                qtcNode             * aNode,
                qcNamePosition        aOldColumnName,
                qcNamePosition        aNewColumnName );
    
private:
    
    static IDE_RC makeColumnNameListFromExpression(
                qcStatement         * aStatement,
                qcNamePosList      ** aColumnNameList,
                qtcNode             * aNode,
                qcNamePosition        aColumnName );
};

#endif /* _O_QDN_CHECK_H_ */
