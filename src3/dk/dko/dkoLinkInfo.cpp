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
 

#include <idl.h>
#include <ide.h>
#include <idu.h>

#include <dkErrorCode.h>

#include <dkoLinkInfo.h>

#ifdef ALTIBASE_PRODUCT_XDB

#else

typedef struct dkoLinkInfoNode
{
    dkoLinkInfo mLinkInfo;

    iduListNode mNode;

} dkoLinkInfoNode;

static iduList  gLinkInfoList;

static iduMutex gLinkInfoListMutex;

static UInt     gLinkInfoListCount;


/*
 *
 */
IDE_RC dkoLinkInfoInitialize( void )
{
    gLinkInfoListCount = 0;

    IDU_LIST_INIT( &gLinkInfoList );

    IDE_TEST_RAISE( gLinkInfoListMutex.initialize( (SChar *)"DKO_LINK_INFO_LIST_MUTEX",
                                                   IDU_MUTEX_KIND_POSIX,
                                                   IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_SET( ideSetErrorCode( dkERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkoLinkInfoFinalize( void )
{
    iduListNode     * sIterator = NULL;
    iduListNode     * sNextNode = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &gLinkInfoList, sIterator, sNextNode )
        {
            sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

            IDU_LIST_REMOVE( sIterator );

            (void)iduMemMgr::free( sLinkInfoNode );
        }
    }
    else
    {
        /* link object list is empty */
    }

    gLinkInfoListCount = 0;

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    IDE_TEST( gLinkInfoListMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkoLinkInfoCreate( UInt aLinkId )
{
    SInt sStage = 0;
    dkoLinkInfo * sLinkInfo = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                       1,
                                       ID_SIZEOF( dkoLinkInfoNode ),
                                       (void **)&sLinkInfoNode,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sStage = 1;

    sLinkInfo = &(sLinkInfoNode->mLinkInfo);

    sLinkInfo->mId      = aLinkId;
    sLinkInfo->mStatus  = DKO_LINK_OBJ_READY;
    sLinkInfo->mRefCnt  = 0;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );

    IDU_LIST_INIT_OBJ( &(sLinkInfoNode->mNode), sLinkInfoNode );

    IDU_LIST_ADD_LAST( &gLinkInfoList, &(sLinkInfoNode->mNode) );

    gLinkInfoListCount++;

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)iduMemMgr::free( sLinkInfoNode );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkoLinkInfoDestroy( UInt aLinkId )
{
    iduListNode     * sIterator = NULL;
    iduListNode     * sNextNode = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;
    dkoLinkInfo     * sLinkInfo = NULL;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &gLinkInfoList, sIterator, sNextNode )
        {
            sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

            IDE_DASSERT( sLinkInfoNode != NULL );

            sLinkInfo = &(sLinkInfoNode->mLinkInfo);

            if ( sLinkInfo->mId == aLinkId )
            {
                IDU_LIST_REMOVE( sIterator );

                (void)iduMemMgr::free( sLinkInfoNode );

                IDE_DASSERT( gLinkInfoListCount > 0 );

                gLinkInfoListCount--;

                break;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* link object list is empty */
    }

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/*
 *
 */
UInt dkoLinkInfoGetLinkInfoCount( void )
{
    return gLinkInfoListCount;
}

/*
 *
 */
IDE_RC dkoLinkInfoGetForPerformanceView( dkoLinkInfo ** aInfoArray, UInt * aInfoCount )
{
    iduListNode     * sIterator     = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;
    dkoLinkInfo     * sLinkInfo     = NULL;
    dkoLinkInfo     * sInfoArray    = NULL;
	idBool			  sIsLock		= ID_FALSE;
    UInt              i             = 0;
    UInt              sLinkInfoCnt  = 0;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );
	sIsLock = ID_TRUE;

    sLinkInfoCnt = dkoLinkInfoGetLinkInfoCount();

    if ( sLinkInfoCnt > 0 )
    {
        IDU_FIT_POINT_RAISE( "dkmGetDatabaseLinkInfo::malloc::Info",
                             ERR_MEMORY_ALLOC );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           ID_SIZEOF( dkoLinkInfo ) * sLinkInfoCnt,
                                           (void **)&sInfoArray,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
        {
            IDU_LIST_ITERATE( &gLinkInfoList, sIterator )
            {
                sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

                IDE_TEST_CONT( i == sLinkInfoCnt, FOUND_COMPLETE );

                if( sLinkInfoNode != NULL )
                {
                    sLinkInfo = &(sLinkInfoNode->mLinkInfo);

                    sInfoArray[i].mId = sLinkInfo->mId;
                    sInfoArray[i].mStatus = sLinkInfo->mStatus;
                    sInfoArray[i].mRefCnt = sLinkInfo->mRefCnt;

                    i++;
                }
                else
                {
                    /* nothing to do */
                }
            }
        }
        else
        {
            /* link object list is empty */
        }
    }
    else
    {
        /* nothing to do */
    }

    IDE_EXCEPTION_CONT( FOUND_COMPLETE );

    *aInfoArray = sInfoArray;
    *aInfoCount = sLinkInfoCnt;

	sIsLock = ID_FALSE;
    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sInfoArray != NULL )
    {
        (void)iduMemMgr::free( sInfoArray );
        sInfoArray = NULL;
    }
    else
    {
        /* do nothing */
    }

	if ( sIsLock == ID_TRUE )
	{
		sIsLock = ID_FALSE;
    	IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );
	}
	else
	{
		/* nothing to do */
	}
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkoLinkInfoSetStatus( UInt aLinkId, dkoLinkInfoStatus aStatus )
{
    iduListNode     * sIterator = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;
    dkoLinkInfo     * sLinkInfo = NULL;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE( &gLinkInfoList, sIterator )
        {
            sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

            IDE_DASSERT( sLinkInfoNode != NULL );

            sLinkInfo = &(sLinkInfoNode->mLinkInfo);

            if ( sLinkInfo->mId == aLinkId )
            {
                if ( aStatus == DKO_LINK_OBJ_TOUCH )
                {
                    if ( sLinkInfo->mStatus == DKO_LINK_OBJ_READY )
                    {
                        sLinkInfo->mStatus = aStatus;
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }
                else
                {
                    sLinkInfo->mStatus = aStatus;
                }

                break;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* link object list is empty */
    }

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkoLinkInfoGetStatus( UInt aLinkId, dkoLinkInfoStatus *aStatus )
{
    iduListNode     * sIterator = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;
    dkoLinkInfo     * sLinkInfo = NULL;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE( &gLinkInfoList, sIterator )
        {
            sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

            IDE_DASSERT( sLinkInfoNode != NULL );

            sLinkInfo = &(sLinkInfoNode->mLinkInfo);

            if ( sLinkInfo->mId == aLinkId )
            {
                *aStatus = (dkoLinkInfoStatus)sLinkInfo->mStatus;
                break;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* link object list is empty */
    }

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkoLinkInfoIncresaseReferenceCount( UInt aLinkId )
{
    iduListNode     * sIterator = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;
    dkoLinkInfo     * sLinkInfo = NULL;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE( &gLinkInfoList, sIterator )
        {
            sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

            IDE_DASSERT( sLinkInfoNode != NULL );

            sLinkInfo = &(sLinkInfoNode->mLinkInfo);

            if ( sLinkInfo->mId == aLinkId )
            {
                sLinkInfo->mRefCnt++;
                break;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* link object list is empty */
    }

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkoLinkInfoDecresaseReferenceCount( UInt aLinkId )
{
    iduListNode     * sIterator = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;
    dkoLinkInfo     * sLinkInfo = NULL;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE( &gLinkInfoList, sIterator )
        {
            sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

            IDE_DASSERT( sLinkInfoNode != NULL );

            sLinkInfo = &(sLinkInfoNode->mLinkInfo);

            if ( sLinkInfo->mId == aLinkId )
            {
                IDE_DASSERT( sLinkInfo->mRefCnt > 0 );
                
                sLinkInfo->mRefCnt--;
                break;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* link object list is empty */
    }

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkoLinkInfoGetReferenceCount( UInt aLinkId, UInt * aReferenceCount )
{
    iduListNode     * sIterator = NULL;
    dkoLinkInfoNode * sLinkInfoNode = NULL;
    dkoLinkInfo     * sLinkInfo = NULL;

    IDE_ASSERT( gLinkInfoListMutex.lock( NULL ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &gLinkInfoList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE( &gLinkInfoList, sIterator )
        {
            sLinkInfoNode = (dkoLinkInfoNode *)sIterator->mObj;

            IDE_DASSERT( sLinkInfoNode != NULL );

            sLinkInfo = &(sLinkInfoNode->mLinkInfo);

            if ( sLinkInfo->mId == aLinkId )
            {
                *aReferenceCount = sLinkInfo->mRefCnt;
                break;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* link object list is empty */
    }

    IDE_ASSERT( gLinkInfoListMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

#endif /* ALTIBASE_PRODUCT_XDB */
