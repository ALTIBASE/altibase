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

#ifndef _O_DKD_RESULT_SET_META_CACHE_H_
#define _O_DKD_RESULT_SET_META_CACHE_H_ 1

extern IDE_RC dkdResultSetMetaCacheInitialize( void );
extern IDE_RC dkdResultSetMetaCacheFinalize( void );

extern IDE_RC dkdResultSetMetaCacheInsert( SChar * aLinkName,
                                           SChar * aRemoteQuery,
                                           UInt aColumnCount,
                                           dkpColumn * aSourceColumnArray,
                                           mtcColumn * aDestColumnArray );

extern IDE_RC dkdResultSetMetaCacheGetAndHold( SChar * aLinkName,
                                               SChar * aRemoteQuery,
                                               UInt * aColumnCount,
                                               dkpColumn ** aSourceColumnArray,
                                               mtcColumn ** aDestColumnArray );

extern IDE_RC dkdResultSetMetaCacheRelese( void );

extern IDE_RC dkdResultSetMetaCacheDelete( SChar * aLinkName );

extern IDE_RC dkdResultSetMetaCacheInvalid( SChar * aLinkName,
                                            SChar * aRemoteQuery );

#endif /* _O_DKD_RESULT_SET_META_CACHE_H_ */
