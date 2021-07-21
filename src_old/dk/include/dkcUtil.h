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
 * $id$
 **********************************************************************/

#ifndef _O_DKC_UTIL_H_
#define _O_DKC_UTIL_H_ 1

extern void dkcUtilInitializeDblinkConf( dkcDblinkConf * aDblinkConf );
extern void dkcUtilFinalizeDblinkConf( dkcDblinkConf * aDblinkConf );

extern void dkcUtilInitializeDblinkConfTarget(
    dkcDblinkConfTarget * aDblinkConfTarget );
extern void dkcUtilFinalizeDblinkConfTarget( dkcDblinkConfTarget * aTarget );

extern void dkcUtilPrintDblinkConf( dkcDblinkConf * aDblinkConf );

extern void dkcUtilSetString( SChar * aSource,
                              UInt aSourceLength,
                              SChar ** aTarget );

extern idBool dkcUtilAppendDblinkConfTarget( dkcDblinkConf * aDblinkConf,
                                             dkcDblinkConfTarget * aTarget );

extern void dkcUtilMoveDblinkConf( dkcDblinkConf * aSrc,
                                   dkcDblinkConf * aDest );

#endif /* _O_DKC_UTIL_H_ */
