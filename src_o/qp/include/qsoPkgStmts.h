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
 * $Id: qsoPkgStmts.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QSO_PKG_STMTS_H_
#define _Q_QSO_PKG_STMTS_H_ 1

#include <qc.h>
#include <qsParseTree.h>

class qsoPkgStmts
{
public:
    static IDE_RC optimizeBodyBlock(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC optimizeCreateProc(
        qcStatement * aStatement,
        qsPkgStmts * aPkgStmts );
};

#endif  // _Q_QSO_PKG_STMTS_H_

