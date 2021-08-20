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

/*
 * These interface are used for only DK module.
 *
 * some dki component like dkif may acceess some data of a dkiSession.
 * These interfaces are accessor of dkiSession.
 *
 * These interfaces are implemented at dki.cpp.
 */

#ifndef _O_DKI_SESSION_H_
#define _O_DKI_SESSION_H_ 1

typedef struct dkiSession dkiSession;

extern dkmSession * dkiSessionGetDkmSession( dkiSession * aSession );
extern dkmSession * dkiSessionGetDkmSessionWithoutCreate( dkiSession * aSession );
extern void dkiSessionSetDkmSessionNull( dkiSession * aSession );
extern IDE_RC dkiSessionCheckAndInitialize( dkiSession * aSession );

#endif /* _O_DKI_SESSION_H_ */
