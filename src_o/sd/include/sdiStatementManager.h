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
 * $Id$ sdiStatement.h
 **********************************************************************/

#ifndef _O_SDI_STATEMENT_MANAGER_H_
#define _O_SDI_STATEMENT_MANAGER_H_ 1

#include <sdiStatement.h>

class sdiStatementManager
{
public:

    /*************************************************************************
     * sdlStatementManager API wrapping layer
     *************************************************************************/
    static void   initializeStatement( sdiStatement * aSdStmt );

    static void   freeRemoteStatement( sdiStatement  * aSdStmt,
                                       sdiClientInfo * aClientInfo,
                                       UInt            aNodeId,
                                       UChar           aMode );

    static void   freeAllRemoteStatement( sdiStatement  * aSdStmt,
                                          sdiClientInfo * aClientInfo,
                                          UChar           aMode );

    static void   finalizeStatement( sdiStatement * aSdStmt );

    /* PROJ-2728 Sharding LOB */
    static void   findRemoteStatement( sdiStatement   * aSdStmt,
                                       UInt             aNodeId,
                                       UInt             aRemoteStmtId,
                                       sdlRemoteStmt ** aRemoteStmt );
               
};

#endif  // _O_SDI_STATEMENT_MANAGER_H_

