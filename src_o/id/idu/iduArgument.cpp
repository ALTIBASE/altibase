/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <idl.h>
#include <ide.h>
#include <iduArgument.h>

IDE_RC iduArgument::initHandlers( iduArgumentHandler** aHandler )
{
    iduArgumentHandler** sHandler;
    
    for( sHandler = aHandler; *sHandler != NULL; sHandler++ )
    {
        if( (*sHandler)->init != NULL )
        {
            IDE_TEST( (*sHandler)->init( *sHandler) != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC iduArgument::arrangeHandlers( iduArgumentHandler** aHandler )
{
    iduArgumentHandler** sHandler;
    
    for( sHandler = aHandler; *sHandler != NULL; sHandler++ )
    {
        if( (*sHandler)->arrange != NULL )
        {
            IDE_TEST( (*sHandler)->arrange( *sHandler) != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC iduArgument::finiHandlers( iduArgumentHandler** aHandler )
{
    iduArgumentHandler** sHandler;
    
    for( sHandler = aHandler; *sHandler != NULL; sHandler++ )
    {
        if( (*sHandler)->fini != NULL )
        {
            IDE_TEST( (*sHandler)->fini( *sHandler) != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC iduArgument::parseArguments( iduArgumentHandler** aHandler,
                                    const UChar**        aArgument )
{
    iduArgumentHandler** sHandler;
    const UChar**        sArgument;
    
    while( *aArgument != NULL )
    {
        for( sHandler = aHandler; *sHandler != NULL; sHandler++ )
        {
            if( (*sHandler)->handler != NULL )
            {
                sArgument = aArgument;
                IDE_TEST( (*sHandler)->handler( *sHandler, &aArgument )
                          != IDE_SUCCESS );
                if( sArgument != aArgument )
                {
                    break;
                }
            }
        }
        IDE_TEST_RAISE( *sHandler == NULL, ERR_HANDLER_NOT_FOUND );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_HANDLER_NOT_FOUND);
    idlOS::fprintf( stderr, "\"%s\" is not usable.\n", *aArgument );
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
