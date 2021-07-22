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
 * $Id: qso.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qso.h>
#include <qmo.h>
#include <qmoSubquery.h>
#include <qcg.h>


IDE_RC qso::optimizeCreate(qcStatement * aStatement)
{
    qsProcParseTree     * sParseTree;
    qsProcStmts         * sProcStmt;
    UInt                  sUserID;

    sParseTree = (qsProcParseTree*) aStatement->myPlan->parseTree;
    sUserID    = QCG_GET_SESSION_USER_ID( aStatement );

    // PR-24408
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );
    
    sProcStmt = (qsProcStmts *)(sParseTree->block);

    // PROJ-1685
    if( sProcStmt != NULL )
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_FAILURE;
}

IDE_RC qso::optimizeNone(qcStatement * /* aStatement */)
{
    /* nothing to do */

    return IDE_SUCCESS;
}

// PROJ-1073 Package
IDE_RC qso::optimizeCreatePkg(qcStatement * aStatement)
{
    qsPkgParseTree     * sParseTree;
    qsProcStmts        * sPkgBlock;
    UInt                 sUserID;

    sParseTree = ( qsPkgParseTree* )( aStatement->myPlan->parseTree );
    sUserID    = QCG_GET_SESSION_USER_ID( aStatement );

    // PR-24408
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );
    
    sPkgBlock = (qsProcStmts *)(sParseTree->block);
    
    IDE_TEST( sPkgBlock->optimize( aStatement, sPkgBlock ) != IDE_SUCCESS );
    
    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_FAILURE;
}
