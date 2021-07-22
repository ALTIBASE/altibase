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

#ifndef _Q_QMV_WITH_H_
#define _Q_QMV_WITH_H_ 1

#include <qc.h>
#include <qmsParseTree.h>

class qmvWith
{
public:
    static IDE_RC validate( qcStatement * aStatement );

    static IDE_RC parseViewInTableRef( qcStatement * aStatement,
                                       qmsTableRef * aTableRef );

private:
    static IDE_RC makeWithStmtFromWithClause( qcStatement        * aStatement,
                                              qcStmtListMgr      * aStmtListMgr,
                                              qmsWithClause      * aWithClause,
                                              qcWithStmt        ** aNewNode );

    static IDE_RC makeParseTree( qcStatement * aStatement,
                                 qmsTableRef * aTableRef,
                                 qcWithStmt  * aWithStmt );

    // PROJ-2582 recursive with
    static IDE_RC validateColumnAlias( qmsWithClause  * aWithClause );
};

#endif /* QMVWITH_H_ */
