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

#include <ide.h>
#include <idu.h>

extern IDE_RC dkisRegisterRemoteQueryCallback( void );
extern IDE_RC dkisRegisterIndexModule( void );
extern IDE_RC dkis2PCCallback( void );
extern IDE_RC dkis2PCNullCallback( void );

/*
 *
 */ 
IDE_RC dkisRegister( void )
{
    IDE_TEST( dkisRegisterIndexModule() != IDE_SUCCESS );
    
    IDE_TEST( dkisRegisterRemoteQueryCallback() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkisRegister2PCCallback( void )
{
    IDE_TEST( dkis2PCCallback() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkisUnregister2PCCallback( void )
{
    IDE_TEST( dkis2PCNullCallback() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

