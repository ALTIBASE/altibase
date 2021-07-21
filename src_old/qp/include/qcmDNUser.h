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
 * $Id: qcmDNUser.h 21727 2007-05-11 02:04:52Z orc $
 **********************************************************************/

#ifndef _O_QCM_DNUSER_H_
#define _O_QCM_DNUSER_H_ 1

#include    <qc.h>
#include    <qtc.h>

class qcmDNUser
{
public:
    static IDE_RC isPermittedUsername(
        qcStatement   * aStatement,
        smiStatement  * aSmiStmt,
        SChar         * aUsername,
        idBool        * aIsPermitted);

    static IDE_RC getUsername(
        qcStatement   * aStatement,
        smiStatement  * aSmiStmt,
        SChar         * aUsrDN,
        SChar         * aUsername,
        UInt            aUsernameLen);
};

#endif /* _O_QCM_DNUSER_H_ */
