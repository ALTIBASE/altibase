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
 * $Id: qcmDatabase.h 82166 2018-02-01 07:26:29Z ahra.cho $
 **********************************************************************/

#ifndef _O_QCM_DATABASE_H_
#define _O_QCM_DATABASE_H_ 1

#include <qc.h>
#include <qcm.h>
#include <qtc.h>

class qcmDatabase
{
public:
    static IDE_RC getMetaVersion(
        smiStatement * aSmiStmt,
        UInt         * aMajorVersion, 
        UInt         * aMinorVersion,
        UInt         * aPatchVersion );

    static IDE_RC checkDatabase( 
        smiStatement      * aSmiStmt,
        SChar             * aOwnerDN,
        UInt                aOwnerDNLen);

    // PROJ-2689 downgrade meta
    static IDE_RC getMetaPrevVersion(
        smiStatement * aSmiStmt,
        UInt         * aPrevMajorVersion, 
        UInt         * aPrevMinorVersion,
        UInt         * aPrevPatchVersion );

    // PROJ-2689 downgrade meta
    static IDE_RC setMetaPrevVersion( 
        qcStatement     * aQcStmt,
        UInt              aPrevMajorVersion,
        UInt              aPrevMinorVersion,
        UInt              aPrevPatchVersion );

private :
    // PROJ-2689 downgrade meta
    static IDE_RC setMetaPrevVersionInternal( 
        smiStatement    * aSmiStmt,
        UInt              aPrevMajorVersion,
        UInt              aPrevMinorVersion,
        UInt              aPrevPatchVersion );
};

#endif /* _O_QCM_DATABASE_H_ */












