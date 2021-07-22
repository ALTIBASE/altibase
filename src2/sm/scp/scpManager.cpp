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
 * $Id: scpManager.cpp
 **********************************************************************/
/**************************************************************
 * FILE DESCRIPTION : scpManager.cpp                            *
 * -----------------------------------------------------------*
 *
 * Proj-2059 DB Upgrade 기능
 * Server 중심적으로 데이터를 가져오고 넣는 기능
  **************************************************************/

#include <idl.h>
#include <ide.h>
#include <smiDef.h>
#include <smiMisc.h>
#include <scpModule.h>
#include <scpManager.h>
#include <smcTable.h>

iduMutex scpManager::mMutex;
smuList  scpManager::mHandleList;



IDE_RC scpManager::initializeStatic()
{
    IDE_TEST( mMutex.initialize( (SChar*)"SM_DATA_PORT_LIST_MUTEX",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);


    SMU_LIST_INIT_BASE( &mHandleList );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC scpManager::destroyStatic()
{
    UInt    sState = 2;

    sState = 1;
    IDE_TEST( destroyList() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 2:
        (void) destroyList();
    case 1:
        (void) mMutex.destroy();
        break;
    default:
        break;
    }

    return IDE_FAILURE;
}

IDE_RC scpManager::findHandle( SChar      * aName,
                               void      ** aHandle )
{
    smuList      * sOpNode;
    smuList      * sBaseNode;
    idBool         sMutexLocked = ID_FALSE;
    scpHandle    * sHandle;


    IDE_DASSERT(  aName   != NULL );
    IDE_DASSERT(  aHandle != NULL );

    (*aHandle) = NULL;

    IDE_TEST( lock() != IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    sBaseNode = getListHead();

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
          sOpNode != sBaseNode;
          sOpNode = SMU_LIST_GET_NEXT(sOpNode) )
    {
        sHandle = (scpHandle*) sOpNode->mData;
       
        if( idlOS::strncmp( (const SChar*)sHandle->mJobName, 
                            (const SChar*)aName, 
                            SMI_DATAPORT_JOBNAME_SIZE ) 
                            == 0 )
        {
            (*aHandle) = (void*)sHandle;
            IDE_CONT( FOUND_HANDLE );
        }
    }

    IDE_EXCEPTION_CONT( FOUND_HANDLE );

    sMutexLocked = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMutexLocked == ID_TRUE )
    {
        (void) unlock();
    }

    return IDE_FAILURE;
}

IDE_RC scpManager::addNode( void * aHandle )
{
    scpHandle        * sHandle;
    smuList          * sNewNode;
    UInt               sState = 0;
    idBool             sMutexLocked = ID_FALSE;

    IDE_DASSERT( aHandle != NULL );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 1, 
                                 ID_SIZEOF(smuList),
                                 (void**)&sNewNode )
                != IDE_SUCCESS );
    sState = 1;

    sHandle = (scpHandle*)aHandle;

    SMU_LIST_INIT_NODE( sNewNode );
    sNewNode->mData = (void*)sHandle;
    sHandle->mSelf  = (void*)sNewNode;

    IDE_TEST( lock() != IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    SMU_LIST_ADD_LAST( &mHandleList, sNewNode );
    sMutexLocked = ID_FALSE;

    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;



    IDE_EXCEPTION_END;

    IDE_PUSH();

    if( sMutexLocked == ID_TRUE )
    {
        (void) unlock();
    }

    switch ( sState )
    {
    case 1 :
        (void) iduMemMgr::free( sNewNode ) ;
    default:
        break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC scpManager::delNode( void      * aHandle )
{
    scpHandle        * sHandle;
    smuList          * sDelNode;
    idBool             sMutexLocked = ID_FALSE;
    UInt               sState = 1;

    IDE_DASSERT( aHandle != NULL );

    sHandle = (scpHandle*)aHandle;

    sDelNode = (smuList*)sHandle->mSelf;

    IDE_TEST( lock() != IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    SMU_LIST_DELETE( sDelNode );

    sMutexLocked = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );


    sState = 0;
    IDE_TEST( iduMemMgr::free( sDelNode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if( sMutexLocked == ID_TRUE )
    {
        (void) unlock() ;
    }

    switch ( sState )
    {
        case 1 :
            (void) iduMemMgr::free( sDelNode );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC scpManager::destroyList()
{
    smuList          * sOpNode;
    smuList          * sBaseNode;
    smuList          * sNextNode;
    idBool             sMutexLocked = ID_FALSE;
    UInt               sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    sBaseNode = getListHead();

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
          sOpNode != sBaseNode;
          sOpNode = sNextNode )
    {
        sNextNode = SMU_LIST_GET_NEXT(sOpNode);

        SMU_LIST_DELETE(sOpNode);

        sState = 1;
        IDE_TEST( iduMemMgr::free( sOpNode ) != IDE_SUCCESS);
        sState = 0;
    }

    sMutexLocked = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // list 진행 도중 깨진 경우는 메모리가 긁힌 경우이기 때문에
    // 딱히 도리가 없다. 진행할 수도 없다.
    // 긁혔다고 추정되는 부분을 출력하고 종료한다.
    if( sState == 1 )
    {
        ideLog::logMem( IDE_SERVER_0, 
                        ((UChar*)sOpNode) - 512,
                        1024 );
        IDE_ASSERT( 0 );
    }

    if( sMutexLocked == ID_TRUE )
    {
        (void)unlock();
    }

    return IDE_FAILURE;
}


