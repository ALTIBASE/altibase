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
 * $Id: qdpDrop.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QDP_DROP_H_
#define _O_QDP_DROP_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

class qdpDrop
{
public:
    static IDE_RC removePriv4DropUser(
        qcStatement * aStatement,
        UInt          aUserID);
    
    static IDE_RC removePriv4DropTable(
        qcStatement * aStatement,
        qdpObjID      aTableID);

    static IDE_RC removePriv4DropProc(
        qcStatement * aStatement,
        qdpObjID      aProcOID);

    // PROJ-1073 Package
    static IDE_RC removePriv4DropPkg(
        qcStatement * aStatement,
        qdpObjID      aPkgOID);

    static IDE_RC removePriv4DropDirectory(
        qcStatement * aStatement,
        qdpObjID      aDirectoryOID);
};

#endif // _O_QDP_DROP_H_
