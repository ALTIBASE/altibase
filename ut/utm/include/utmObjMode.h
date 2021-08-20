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
 * $Id: utmObjMode.h $
 **********************************************************************/

#ifndef _O_UTM_OBJMODE_H_
#define _O_UTM_OBJMODE_H_ 1

SQLRETURN objectModeInfoQuery();


SQLRETURN getObjModeTableQuery( FILE   *aTblFp,
                                FILE   *aDbStatsFp,
                                SChar  *aUserName,
                                SChar  *aObjName );

/* BUG-49242 Queue object mode */
SQLRETURN getObjModeQueueQuery( FILE   *aQueueFp,
                                SChar  *aUserName,
                                SChar  *aObjName );

SQLRETURN getObjModeMViewQuery( FILE  * aMViewFp,
                                SChar * aUserName,
                                SChar * aObjectName );

SQLRETURN getObjModeViewQuery( FILE  *aViewFp,
                               SChar *aUserName,
                               SChar *aObjectName );

SQLRETURN getObjModeProcQuery( FILE  *aProcFp,
                               SChar *aUserName,
                               SChar *aObjectName );

SQLRETURN getObjModePkgQuery( FILE  *aPkgFp,
                              SChar *aUserName,
                              SChar *aObjectName );
#endif /* _O_UTM_OBJMODE_H_ */
