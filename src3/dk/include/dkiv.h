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

#ifndef _O_DKIV_H_
#define _O_DKIV_H_ 1

extern IDE_RC dkivRegisterPerformaceView( void );

/*
 * Following functions are for internal usage.
 */

extern IDE_RC dkivRegisterDblinkAltiLinkerStatus( void );
extern IDE_RC dkivRegisterDblinkDatabaseLinkInfo( void );
extern IDE_RC dkivRegisterDblinkLinkerSessionInfo( void );
extern IDE_RC dkivRegisterDblinkLinkerControlSessionInfo( void );
extern IDE_RC dkivRegisterDblinkLinkerDataSessionInfo( void );
extern IDE_RC dkivRegisterDblinkGlobalTransactionInfo( void );
extern IDE_RC dkivRegisterDblinkRemoteTransactionInfo( void );
extern IDE_RC dkivRegisterDblinkNotifierTransactionInfo( void );
extern IDE_RC dkivRegisterDblinkShardNotifierTransactionInfo( void );
extern IDE_RC dkivRegisterDblinkRemoteStatementInfo( void );

#endif /* _O_DKIV_H_ */
