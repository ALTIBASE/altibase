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
 * $Id: qcuSessionPkg.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QCU_SESSIONPKG_H_
#define _O_QCU_SESSIONPKG_H_  1

#include <qc.h>
#include <qsxPkg.h>

class qcuSessionPkg
{
public:
    static IDE_RC finalizeSessionPkgInfoList( qcSessionPkgInfo ** aSessionPkgInfo );

    static IDE_RC makeSessionPkgInfo( qcSessionPkgInfo ** aNewSessionPkgInfo );

    static IDE_RC delSessionPkgInfo( qcStatement * aStatement, 
                                     qsOID         aPkgOID );

    static IDE_RC searchPkgInfoFromSession( 
                    qcStatement          * aStatement,
                    qsxPkgInfo           * aPkgInfo,
                    mtcStack             * aStack,
                    SInt                   aStackRemain,
                    qcTemplate          ** aTemplate    /* output */ );

    static IDE_RC getTmplate( qcStatement  * aStatement,
                              smOID          aPkgOID,
                              mtcStack     * aStack,
                              SInt           aStacRemain,
                              mtcTemplate ** aTmplate );

    static IDE_RC initPkgVariable( qsxExecutorInfo * aExecInfo,
                                   qcStatement     * aStatement,
                                   qsxPkgInfo      * aPkgInfo,
                                   qcTemplate      * aTemplate );

    static IDE_RC finiPkgVariable( qcStatement * aStatement,
                                   qsxPkgInfo  * aPkgInfo,
                                   qcTemplate  * aTemplate );

    static IDE_RC initPkgCursors( qcStatement * aQcStmt, qsOID aObjectID );

    static IDE_RC finiPkgCursors( qcStatement * aQcStmt, qsOID aObjectID );

    /* BUG-38844
       execute 시점에 meta에서 package가 있는지 찾아서 qsxPkgInfo를 넘겨준다. */
    static IDE_RC getPkgInfo( qcStatement     * aStatement,
                              UInt              aUserID,
                              qcNamePosition    aPkgName,
                              qsObjectType      aObjectType,
                              qsxPkgInfo     ** aPkgInfo );
};

#endif // _O_QCU_SESSIONPKG_H_
