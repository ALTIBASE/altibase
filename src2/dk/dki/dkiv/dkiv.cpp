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

#include <idl.h>
#include <ide.h>

#include <dkiv.h>

IDE_RC dkivRegisterPerformaceView( void )
{
    IDE_TEST( dkivRegisterDblinkAltiLinkerStatus() != IDE_SUCCESS );
    
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkivRegisterDblinkDatabaseLinkInfo() != IDE_SUCCESS );
#endif
    
    IDE_TEST( dkivRegisterDblinkLinkerSessionInfo() != IDE_SUCCESS );

    IDE_TEST( dkivRegisterDblinkLinkerControlSessionInfo() != IDE_SUCCESS );

    IDE_TEST( dkivRegisterDblinkLinkerDataSessionInfo() != IDE_SUCCESS );

    IDE_TEST( dkivRegisterDblinkGlobalTransactionInfo() != IDE_SUCCESS );

    IDE_TEST( dkivRegisterDblinkRemoteTransactionInfo() != IDE_SUCCESS );

    IDE_TEST( dkivRegisterDblinkNotifierTransactionInfo() != IDE_SUCCESS );
    
    IDE_TEST( dkivRegisterDblinkShardNotifierTransactionInfo() != IDE_SUCCESS );

    IDE_TEST( dkivRegisterDblinkRemoteStatementInfo() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
