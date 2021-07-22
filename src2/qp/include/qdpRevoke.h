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
 * $Id: qdpRevoke.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QDP_REVOKE_H_
#define  _O_QDP_REVOKE_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

class qdpRevoke
{
public:
    static IDE_RC validateRevokeSystem(
        qcStatement * aStatement);
    static IDE_RC validateRevokeObject(
        qcStatement * aStatement);
    
    static IDE_RC executeRevokeSystem(
        qcStatement * aStatement);
    static IDE_RC executeRevokeObject(
        qcStatement * aStatement);

private:
    static IDE_RC deleteChainObjectByGrantOption(
        qcStatement    * aStatement,
        qcmTableInfo   * aTableInfo,
        UInt             aPrivID,
        UInt             aNextGrantorID,
        UInt             aFirstGrantor);

    static IDE_RC deleteSystemAllPrivileges(
        qcStatement    * aStatement,
        UInt             aGrantorID,
        UInt             aGranteeID);

    static IDE_RC deleteObjectPrivFromMeta(
        qcStatement     * aStatement,
        UInt              aGrantorID,
        UInt              aGranteeID,
        UInt              aPrivID,
        UInt              aUserID,
        qdpObjID          aObjID,
        SChar           * aObjectType,
        UInt              aWithGrantOption);
};

#endif // _O_QDP_REVOKE_H_
