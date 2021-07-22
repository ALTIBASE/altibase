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
 * $Id:
 **********************************************************************/

#include <idl.h>
#include <smuProperty.h>
#include <smuMemStack.h>

/**************************************************************************
 * Description : mem stack을 초기화 한다.
 *
 * aMemoryIndex    - [IN] Node를 할당할 위치
 * aDataSize       - [IN] 필요한 Data 한개의 크기
 * aStackInitCount - [IN] 초기화 할 때 미리 만들어 놓을 node수
 ***************************************************************************/
IDE_RC smuMemStack::initialize( iduMemoryClientIndex   aMemoryIndex,
                                UInt                   aDataSize,
                                UInt                   aStackInitCount )
{
    UInt         i;
    smuMemNode * sNode  = NULL;
    UInt         sState = 0;
    idBool       sIsEmpty;

    mDataSize    = aDataSize;
    mInitCount   = aStackInitCount;
    mMemoryIndex = aMemoryIndex;
    mNodeCount   = 0;

    IDE_TEST( mNodeStack.initialize( getMemoryIndex(),
                                     ID_SIZEOF( UChar* ) )
              != IDE_SUCCESS );
    sState = 1;


    for( i = 0 ; i < getInitCount() ; i++ )
    {
        IDE_TEST( iduMemMgr::malloc( getMemoryIndex(),
                                     getNodeSize(),
                                     (void**)&sNode )
                  != IDE_SUCCESS );

        idlOS::memset( (UChar*)sNode, 0, getNodeSize() );

        sNode->mDataSize = getDataSize();

        IDE_TEST( mNodeStack.push( ID_FALSE, // lock
                                   (void*) &sNode )
                  != IDE_SUCCESS );

        mNodeCount++;
        sNode = NULL;
    }

    IDE_TEST( mMutex.initialize( (SChar*)"SM_MEM_STACK_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);
    sState = 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            mMutex.destroy();
        case 1:
            if( sNode != NULL )
            {
                // Push 하다가 실패한 것
                (void) iduMemMgr::free( (void*)sNode );
            }

            while( 1 )
            {
                IDE_ASSERT( mNodeStack.pop( ID_FALSE, // lock
                                            (void*) &sNode,
                                            &sIsEmpty )
                            == IDE_SUCCESS );

                if( sIsEmpty == ID_FALSE )
                {
                    (void) iduMemMgr::free( (void*)sNode );
                }
                else
                {
                    break;
                }
            }
            mNodeStack.destroy();
        default:
            break;
    }
    mNodeCount = 0;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : mem stack을 정리한다.
 ***************************************************************************/
IDE_RC smuMemStack::destroy()
{
    UChar * sNode;
    idBool  sIsEmpty;
    idBool  sIsLock = ID_FALSE;

    lock();
    sIsLock = ID_TRUE;
    IDE_TEST( mNodeStack.getTotItemCnt() != getNodeCount() );

    while( 1 )
    {
        IDE_TEST( mNodeStack.pop( ID_FALSE, // lock
                                  (void*) &sNode,
                                  &sIsEmpty )
                  != IDE_SUCCESS );

        if( sIsEmpty == ID_FALSE )
        {
            (void) iduMemMgr::free( (void*)sNode );
        }
        else
        {
            break;
        }
    }
    mNodeCount   = 0;
    IDE_TEST( mNodeStack.destroy() != IDE_SUCCESS );

    sIsLock = ID_FALSE;
    unlock();

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsLock == ID_TRUE )
    {
        unlock();
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Mem Stack의 Pool에 유지할 Node의 갯수를 변경한다.
 *
 * aNewStackInitCount - [IN] Pool에 보관해둘 Node의 갯수
 ***************************************************************************/
IDE_RC smuMemStack::resetStackInitCount( UInt aNewStackInitCount )
{
    UInt         sOldStackCount ;
    smuMemNode * sNode = NULL;
    idBool       sIsEmpty;
    SInt         i;
    SInt         sPushCount = 0;
    idBool       sIsLock = ID_FALSE;
    SInt         sAllocCount;

    lock();
    sIsLock = ID_TRUE;

    sOldStackCount = getCount();

    if( aNewStackInitCount > sOldStackCount )
    {
        sAllocCount = aNewStackInitCount - sOldStackCount;

        for( sPushCount = 0 ; sPushCount < sAllocCount ; sPushCount++ )
        {
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                         getNodeSize(),
                                         (void**)&sNode )
                      != IDE_SUCCESS );

            idlOS::memset( (UChar*)sNode, 0, getNodeSize() );
            sNode->mDataSize = getDataSize();

            IDE_TEST( mNodeStack.push( ID_FALSE, // lock
                                       (void*)&sNode )
                      != IDE_SUCCESS );
            mNodeCount++;
            sNode = NULL;
        }
        mInitCount = aNewStackInitCount;
    }
    else
    {
        if( aNewStackInitCount < getInitCount() )
        {
            mInitCount = aNewStackInitCount;
        }
        else
        {
            // nothing
        }
    }

    sIsLock = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sNode != NULL )
    {
        // Push 하다가 실패한 것
        (void) iduMemMgr::free( (void*)sNode );
    }

    for( i = 0 ; i < sPushCount ; i++ )
    {
        (void) mNodeStack.pop( ID_FALSE, // lock
                               (void*)&sNode,
                               &sIsEmpty );

        if(sIsEmpty == ID_FALSE )
        {
            (void) iduMemMgr::free( (void*)sNode );
            mNodeCount--;
        }
        else
        {
            break;
        }
    }

    if(sIsLock == ID_TRUE )
    {
        unlock();
        sIsLock = ID_FALSE;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Mem Stack의 Data 의 크기를 변경한다.
 *
 *aNewDataSize   - [IN] 변경 할 Data의 크기
 ***************************************************************************/
IDE_RC smuMemStack::resetDataSize( UInt aNewDataSize )
{
    UInt         i;
    smuMemNode * sNode = NULL;
    idBool       sIsEmpty;
    UInt         sNodeCount;
    UInt         sState = 0;
    UInt         sOldDataSize ;
    UInt         sNodeSize;
    UInt         sPopCount = 0;
    idBool       sIsLock = ID_FALSE;

    lock();
    sIsLock = ID_TRUE;

    sOldDataSize = getDataSize();
    sNodeCount   = getCount() > getInitCount() ? getCount() : getInitCount();

    if( aNewDataSize != sOldDataSize )
    {
        sState = 1;

        IDE_TEST( mNodeStack.pop( ID_FALSE, // lock
                                  (void*)&sNode,
                                  &sIsEmpty )
                  != IDE_SUCCESS );

        while( sIsEmpty == ID_FALSE )
        {
            sPopCount++;
            mNodeCount--;
            IDE_TEST( iduMemMgr::free( (void*)sNode ) != IDE_SUCCESS );
            sNode = NULL;

            IDE_TEST( mNodeStack.pop( ID_FALSE, // lock
                                      (void*)&sNode,
                                      &sIsEmpty )
                  != IDE_SUCCESS );
        }

        sState = 2;
        mDataSize = aNewDataSize;

        sState = 3;
        sNodeSize = aNewDataSize + SMU_MEM_NODE_SIZE;

        for( i = 0 ; i < sNodeCount ; i++ )
        {
            IDE_TEST( iduMemMgr::malloc( getMemoryIndex(),
                                         sNodeSize,
                                         (void**)&sNode )
                      != IDE_SUCCESS );

            idlOS::memset( (UChar*)sNode, 0, sNodeSize );

            sNode->mDataSize = getDataSize();

            IDE_TEST( mNodeStack.push( ID_FALSE, // lock
                                      (void*)&sNode )
                      != IDE_SUCCESS );
            mNodeCount++;
            sNode = NULL;
        }
    }

    sIsLock = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 3 :
            {
                if( sNode != NULL )
                {
                    (void)iduMemMgr::free( (void*)sNode );
                    mNodeCount--;
                }
                IDE_ASSERT( mNodeStack.pop( ID_FALSE, // lock
                                           (void*)&sNode,
                                           &sIsEmpty )
                          == IDE_SUCCESS );

                while( sIsEmpty == ID_FALSE )
                {
                    (void)iduMemMgr::free( (void*)sNode );
                    mNodeCount--;

                    IDE_ASSERT( mNodeStack.pop( ID_FALSE, // lock
                                               (void*)&sNode,
                                               &sIsEmpty )
                                == IDE_SUCCESS );
                }
            }
        case 2:
            mDataSize = sOldDataSize;

        case 1 :
            {
                for( i = 0 ; i < sPopCount ; i++ )
                {
                    IDE_ASSERT( iduMemMgr::malloc( getMemoryIndex(),
                                                   getNodeSize(),
                                                   (void**)&sNode )
                                == IDE_SUCCESS );

                    idlOS::memset( (UChar*)sNode, 0, getNodeSize() );
                    sNode->mDataSize = getDataSize();

                    IDE_ASSERT( mNodeStack.push( ID_FALSE, // lock
                                                 (void*)&sNode )
                                == IDE_SUCCESS );
                    mNodeCount++;
                    sNode = NULL;
                }
            }
            break;
        default:
            break;
    }
    if( sIsLock == ID_TRUE )
    {
        unlock();
        sIsLock = ID_FALSE;
    }
    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 생성해둔 data 를 한개 가져간다.
 *               없으면 생성해서 가져간다.
 *               Lock으로 보호되지 않는다,
 *
 * aData  - [OUT] 할당한 data를 반환한다.
 ***************************************************************************/
IDE_RC smuMemStack::pop( UChar   **aData )
{
    smuMemNode * sNode = NULL;
    idBool       sIsEmpty = ID_TRUE;

    IDE_TEST( mNodeStack.pop( ID_FALSE, // lock
                              (void*)&sNode,
                              &sIsEmpty )
              != IDE_SUCCESS );

    if( sIsEmpty == ID_TRUE )
    {
        mNodeCount++;

        IDE_TEST( iduMemMgr::malloc( getMemoryIndex(),
                                     getNodeSize(),
                                     (void**)&sNode )
                  != IDE_SUCCESS );

        idlOS::memset( (UChar*)sNode, 0, getNodeSize() );

        sNode->mDataSize = getDataSize();
    }

    (*aData) = (UChar*)&(sNode->mData[0]);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsEmpty == ID_FALSE )
    {
        IDE_ASSERT( sNode != NULL );
        IDE_ASSERT( mNodeStack.push( ID_FALSE, // lock
                                     (void*)&sNode )
              == IDE_SUCCESS );
    }
    else // sIsEmpty == ID_TRUE
    {
        if( sNode != NULL )
        {
            // malloc한 경우
            (void) iduMemMgr::free( (UChar*)sNode );

            mNodeCount--;
        }
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 생성해둔 data 를 한개 가져간다.
 *               없으면 생성해서 가져간다.
 *               Lock으로 보호된다.
 *
 * aData  - [OUT] 할당한 data를 반환한다.
 ***************************************************************************/
IDE_RC smuMemStack::popWithLock( UChar   **aData )
{
    smuMemNode * sNode = NULL;
    idBool       sIsEmpty = ID_TRUE;
    idBool       sIsLock  = ID_FALSE;

    lock();
    sIsLock = ID_TRUE;

    IDE_TEST( mNodeStack.pop( ID_FALSE, // lock
                              (void*)&sNode,
                              &sIsEmpty )
              != IDE_SUCCESS );

    if( sIsEmpty == ID_TRUE )
    {
        mNodeCount++;

        sIsLock = ID_FALSE;
        unlock();

        IDE_TEST( iduMemMgr::malloc( getMemoryIndex(),
                                     getNodeSize(),
                                     (void**)&sNode )
                  != IDE_SUCCESS );

        idlOS::memset( (UChar*)sNode, 0, getNodeSize() );

        sNode->mDataSize = getDataSize();
    }
    else
    {
        sIsLock = ID_FALSE;
        unlock();
    }

    (*aData) = (UChar*)&(sNode->mData[0]);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsEmpty == ID_FALSE )
    {
        IDE_ASSERT( sNode != NULL );
        IDE_ASSERT( mNodeStack.push( ID_FALSE, // lock
                                     (void*)&sNode )
              == IDE_SUCCESS );
    }
    else // sIsEmpty == ID_TRUE
    {
        if( sNode != NULL )
        {
            // malloc한 경우
            (void) iduMemMgr::free( (UChar*)sNode );

            if( sIsLock == ID_FALSE )
            {
                lock();
                sIsLock = ID_TRUE;
            }

            mNodeCount--;
        }
    }

    if( sIsLock == ID_TRUE )
    {
        unlock();
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 한개의 data를 반환한다.
 *               만약 mem stack pool이 resize되어
 *               이전에 할당 받아갔던 지금 반환 하려는 Data와
 *               크기가 달라졌다면 mem free 한다.
 *               Lock으로 보호되지 않는다.
 *
 * aData - [IN] 반환 할 data
 ***************************************************************************/
IDE_RC smuMemStack::push( UChar  *aData )
{
    smuMemNode * sNode = (smuMemNode*)(aData - SMU_MEM_NODE_SIZE);

    if( sNode->mDataSize == getDataSize() )
    {
        IDE_TEST( mNodeStack.push( ID_FALSE, // lock
                                   (void*)&sNode )
                  != IDE_SUCCESS );

    }
    else
    {
        mNodeCount--;

        IDE_TEST( iduMemMgr::free( (UChar*)sNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 한개의 data를 반환한다.
 *               만약 mem stack pool이 resize되어
 *               이전에 할당 받아갔던 지금 반환 하려는 Data와
 *               크기가 달라졌다면 mem free 한다.
 *               Lock으로 보호한다.
 *
 * aData - [IN] 반환 할 data
 ***************************************************************************/
IDE_RC smuMemStack::pushWithLock( UChar  *aData )
{
    smuMemNode * sNode = (smuMemNode*)(aData - SMU_MEM_NODE_SIZE);
    idBool       sIsLock = ID_FALSE;

    lock();
    sIsLock = ID_TRUE;

    if( sNode->mDataSize == getDataSize() )
    {
        IDE_TEST( mNodeStack.push( ID_FALSE, // lock
                                   (void*)&sNode )
                  != IDE_SUCCESS );

        sIsLock = ID_FALSE;
        unlock();
    }
    else
    {
        mNodeCount--;
        sIsLock = ID_FALSE;
        unlock();

        IDE_TEST( iduMemMgr::free( (UChar*)sNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsLock == ID_TRUE )
    {
        unlock();
    }

    return IDE_FAILURE;
}
