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
#include <idl.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dkc.h>

#include <dkcUtil.h>

#include <dkcParser.h>

/*
 *
 */ 
IDE_RC dkcLoadDblinkConf( dkcDblinkConf ** aDblinkConf )
{
    dkcDblinkConf * sDblinkConf = NULL;
    dkcParser * sParser = NULL;
    SInt sStage = 0;
    
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( *sDblinkConf ),
                                       (void **)&sDblinkConf,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERROR_MEMORY_ALLOC );
    sStage = 1;
    
    dkcUtilInitializeDblinkConf( sDblinkConf );

    IDE_TEST( dkcParserCreate( &sParser ) != IDE_SUCCESS );
    sStage = 2;
    
    IDE_TEST( dkcParserRun( sParser, sDblinkConf ) != IDE_SUCCESS );
    
    IDE_TEST( dkcParserDestroy( sParser ) != IDE_SUCCESS );

    *aDblinkConf = sDblinkConf;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sStage )
    {
        case 2:
            (void)dkcParserDestroy( sParser );
        case 1:
            (void)iduMemMgr::free( sDblinkConf );
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkcFreeDblinkConf( dkcDblinkConf * aDblinkConf )
{
    IDU_FIT_POINT( "dkc::dkcFreeDblinkConf::dkcUtilFinalizeDblinkConf::sleep" );
    
    dkcUtilFinalizeDblinkConf( aDblinkConf );

    (void)iduMemMgr::free( aDblinkConf );
    
    return IDE_SUCCESS;

#ifdef ALTIBASE_FIT_CHECK    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}
