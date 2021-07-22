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
 * $Id: qdpGrant.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QDP_GRANT_H_
#define  _O_QDP_GRANT_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

class qdpGrant
{
public:
    static IDE_RC validateGrantSystem(
        qcStatement     * aStatement);
    
    static IDE_RC validateGrantObject(
        qcStatement     * aStatement);
    
    static IDE_RC executeGrantSystem(
        qcStatement     * aStatement);

    static IDE_RC executeGrantObject(
        qcStatement     * aStatement);

    static IDE_RC checkObjectInfo(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        qcNamePosition    aObjectName,
        UInt            * aUserID,
        qdpObjID        * aObjID,
        SChar           * aObjectType);

    static IDE_RC checkGrantees(
        qcStatement     * aStatement,
        qdGrantees      * aGrantees);

    static IDE_RC checkDuplicatePrivileges(
        qcStatement     * aStatement,
        qdPrivileges    * aPrivilege);

    static IDE_RC haveObjectWithGrant(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        UInt              aPrivID,
        qdpObjID          aObjID,
        SChar           * aObjType);

    static IDE_RC haveObjectWithGrantOfSequence(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        qdpObjID          aObjID,
        SChar           * aObjType);

    static IDE_RC haveObjectWithGrantOfTable(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        qdpObjID          aObjID,
        SChar           * aObjType);

    static IDE_RC haveObjectWithGrantOfProcedure(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        qdpObjID          aObjID,
        SChar           * aObjType);

    // PROJ-1073 Package
    static IDE_RC haveObjectWithGrantOfPkg(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        qdpObjID          aObjID,
        SChar           * aObjType);

    // PROJ-1371 directory
    static IDE_RC haveObjectWithGrantOfDirectory(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        qdpObjID          aObjID,
        SChar           * aObjType);
    
    static IDE_RC grantDefaultPrivs4CreateUser(
        qcStatement     * aStatement,
        UInt              aGrantorID,
        UInt              aGranteeID);

    static IDE_RC checkUsefulPrivilege4Object(
        qcStatement     * aStatement,
        qdPrivileges    * aPrivilege,
        SChar           * aObjectType,
        qdpObjID          aObjID);
    
    static IDE_RC insertObjectPrivIntoMeta(
        qcStatement     * aStatement,
        UInt              aGrantorID,
        qdGrantees      * aGrantee,
        UInt              aPrivID,
        UInt              aUserID,
        qdpObjID          aObjID,
        SChar           * aObjectType,
        UInt              aWithGrantOption);

    // PROJ-1685
    static IDE_RC haveObjectWithGrantOfLibrary(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        qdpObjID          aObjID,
        SChar           * aObjType);

    /* PROJ-1812 ROLE */
    static IDE_RC grantPublicRole4CreateUser(
        qcStatement * aStatement,
        UInt          aGrantorID,
        UInt          aGranteeID );
    
};

#endif // _O_QDP_GRANT_H_
