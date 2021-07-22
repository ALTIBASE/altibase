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
 * $Id: qsxPkg.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QSX_PKG_H_
#define _Q_QSX_PKG_H_ 1

#include <iduMemory.h>
#include <iduLatch.h>
#include <qc.h>
#include <qsParseTree.h>

#define RETRY_SLEEP_USEC          (200) // less than 100 usec has no means
                                        // because wait time can be always
                                        // graterthan 100 usec  10->200
#define MAX_RETRY_COUNT           (1000)

typedef struct qsxPkgInfo
{
    qsOID              pkgOID;
    UInt               modifyCount;

    idBool             isValid;
    UInt               sessionID; // BUG-36291 invalid로 변경한 SessionID

    iduVarMemList    * qmsMem;
    qsPkgParseTree   * planTree;
    qsObjectType       objType;
    qsRelatedObjects * relatedObjects;
    qcTemplate       * tmplate;
    UInt               privilegeCount;
    UInt             * granteeID; // arrary: granteeID[privilegeCount]
} qsxPkgInfo;

typedef struct qsxPkgObjectInfo
{
    iduLatch           latch;
    iduLatch           latchForStatus;
    idBool             isAvailable;
    qsxPkgInfo       * pkgInfo;
} qsxPkgObjectInfo;

class qsxPkg
{
public :
    static IDE_RC createPkgObjectInfo( qsOID               aPkgOID,
                                       qsxPkgObjectInfo ** aPkgObjectInfo );
    
    static IDE_RC destroyPkgObjectInfo( qsOID               aPkgOID,
                                        qsxPkgObjectInfo ** aPkgObjectInfo );
    
    static IDE_RC disablePkgObjectInfo( qsOID          aPkgOID );
    
    static IDE_RC getPkgObjectInfo( qsOID               aPkgOID,
                                    qsxPkgObjectInfo ** aPkgObjectInfo );
    
    static IDE_RC setPkgObjectInfo( qsOID              aPkgOID,
                                    qsxPkgObjectInfo * aPkgObjectInfo );
    
    static IDE_RC createPkgInfo (qsOID         aPkgOID,
                                 qsxPkgInfo ** aPkgInfo);

    static IDE_RC destroyPkgInfo(qsxPkgInfo ** aPkgInfo);

    static IDE_RC getPkgInfo( qsOID          aPkgOID,
                              qsxPkgInfo  ** aPkgInfo );
    
    static IDE_RC setPkgInfo( qsOID          aPkgOID,
                              qsxPkgInfo   * aPkgInfo );

    static IDE_RC setPkgText(smiStatement * aSmiStmt,
                              qsOID         aPkgOID,
                              SChar       * aPkgText,
                              SInt          aPkgTextLen);
    
    static IDE_RC getPkgText(qcStatement * aStatement,
                              qsOID        aPkgOID,
                              SChar     ** aPkgText,
                              SInt       * aPkgTextLen);

    static IDE_RC latchS( qsOID          aPkgOID );

    static IDE_RC latchX( qsOID          aPkgOID,
                          idBool         aTryLock );
 
    // fix BUG-18854
    static IDE_RC latchXForRecompile( qsOID          aPkgOID );

    static IDE_RC unlatch( qsOID          aPkgOID );

    static IDE_RC makeStatusValid( qcStatement * aStatement,
                                   qsOID         aPkgOID );

    static IDE_RC makeStatusValidTx( qcStatement * aStatement,
                                     qsOID         aPkgOID );

    static IDE_RC makeStatusInvalid( qcStatement * aStatement,
                                     qsOID         aPkgOID );

    static IDE_RC makeStatusInvalidTx( qcStatement * aStatement,
                                       qsOID         aPkgOID );

    static IDE_RC makeStatusInvalidForRecompile( qcStatement * aStatement,
                                                 qsOID         aPkgOID );

    static IDE_RC latchXForStatus( qsOID          aPkgOID );

    static IDE_RC unlatchForStatus( qsOID          aPkgOID );

    /* BUG-38844 package subproram의 parse tree 검색. */
    static IDE_RC findSubprogramPlanTree(
        qsxPkgInfo       * aPkgInfo,
        UInt               aSubprogramID,
        qsProcParseTree ** aSubprogramPlanTree );

    /* PROJ-2446 ONE SOURCE FOR SUPPOTING PACKAGE */
    static IDE_RC createPkgObjAndInfoCallback( smiStatement * aSmiStmt,
                                               qsOID          aPkgOID );
};

#endif /* _Q_QSX_PKG_H_ */

