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
 * $Id$
 **********************************************************************/

#ifndef _O_QCM_AUDIT_H_
#define _O_QCM_AUDIT_H_ 1

#include <qci.h>
#include <qcm.h>
#include <qdcAudit.h>

class qcmAudit
{
public:
    static IDE_RC getOption( smiStatement      * aSmiStmt,
                             UInt                aUserID,
                             SChar             * aObjectName,
                             UInt                aObjectNameSize,
                             qciAuditOperation * aOperation,
                             idBool            * aExist );

    static IDE_RC getAllOptions( smiStatement        * aSmiStmt,
                                 qciAuditOperation  ** aObjectOptions,
                                 UInt                * aObjectOptionCount,
                                 qciAuditOperation  ** aUserOptions,
                                 UInt                * aUserOptionCount );
    
    static IDE_RC getStatus( smiStatement  * aSmiStmt,
                             idBool        * aIsStarted );

    static IDE_RC updateObjectName( qcStatement  * aStatement,
                                    UInt           aUserID,
                                    SChar        * aOldObjectName,
                                    SChar        * aNewObjectName );

    static IDE_RC deleteObject( qcStatement  * aStatement,
                                UInt           aUserID,
                                SChar        * aObjectName );
};

#endif /* _O_QCM_AUDIT_H_ */
