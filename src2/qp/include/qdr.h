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
 * $Id: qdr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QDR_H_
#define _O_QDR_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

// parameter may vary during implementation.

// qd userR -- create user, alter user
class qdr
{
public:
    // validate.
    static IDE_RC validateCreate(
        qcStatement * aStatement);
    static IDE_RC validateAlter(
        qcStatement * aStatement);
    /* PROJ-2207 Password policy support */
    static IDE_RC validatePasswOpts(
        qcStatement * aStatement);

    // execute
    static IDE_RC executeCreate(
        qcStatement * aStatement);
    static IDE_RC executeAlter(
        qcStatement * aStatement);
    
    static void passwPolicyDefaultOpts(
        qcStatement * aStatement );
};

#endif // _O_QDR_H_
