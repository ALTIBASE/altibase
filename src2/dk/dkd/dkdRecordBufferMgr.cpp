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

#include <dkdRecordBufferMgr.h>


/***********************************************************************
 * Description: Record buffer manager 를 초기화한다.
 *              이 과정에서 DK data buffer 로부터 record buffer 할당이 
 *              가능한지 체크해보고 할당 가능한 경우는 record buffer 
 *              manager 를 생성하고, 그렇지 않다면 disk temp table 
 *              manager 를 생성한다.
 *
 *  BUG-37215: 원래는 record buffer manager 객체가 생성되는 시점에서 
 *             TLSF memory allocator 를 생성하도록 되어 있었으나 효율적인 
 *             메모리 할당 및 해제를 위해 global allocator 를 data buffer 
 *             manager ( dkdDataBufferMgr )에 두고, 입력인자로 넘겨받아
 *             사용하도록 변경됨에 따라 TLSF memory allocator 를 생성과 
 *             관련된 부분들이 삭제되고 입력인자가 하나 추가됨.
 *
 *  aBufBlockCnt    - [IN] 이 record buffer manager 가 할당받을 buffer
 *                         를 구성하는 buffer block 의 개수
 *  aAllocator      - [IN] record buffer 를 할당받을 때 사용하기 위한 
 *                         TLSF memory allocator 를 가리키는 포인터
 *
 **********************************************************************/
IDE_RC  dkdRecordBufferMgr::initialize( UInt             aBufBlockCnt, 
                                        iduMemAllocator *aAllocator )
{
    IDE_ASSERT( aBufBlockCnt > 0 );

    mIsEndOfFetch = ID_FALSE;
    mRecordCnt    = 0;
    mBufSize      = aBufBlockCnt * (ULong)dkdDataBufferMgr::getBufferBlockSize();
    mBufBlockCnt  = aBufBlockCnt;
    mCurRecord    = NULL;
    mAllocator    = aAllocator;

    IDU_LIST_INIT( &mRecordList );

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : Record buffer manager 를 정리한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dkdRecordBufferMgr::finalize()
{
    iduListNode     *sCurRecordNode;
    iduListNode     *sNextRecordNode;

    IDU_LIST_ITERATE_SAFE( &mRecordList, sCurRecordNode, sNextRecordNode )
    {
        /* BUG-37487 : void */
        (void)iduMemMgr::free( sCurRecordNode->mObj, mAllocator ); 

        if ( --mRecordCnt < 0 )
        {
            mRecordCnt = 0;
        }
        else
        {
            /* record count became zero */
        }
    }

    IDE_ASSERT( mRecordCnt == 0 );

    /* BUG-37215 */
    mAllocator = NULL;
}

/************************************************************************
 * Description : Record buffer 로부터 record 하나를 fetch 해온다. 
 *
 *  aRow        - [OUT] fetch 해올 row 를 가리키는 포인터
 *
 ************************************************************************/
IDE_RC  dkdRecordBufferMgr::fetchRow( void  **aRow )
{
    /* fetch row */
    if ( mCurRecord != NULL )
    {
        if ( mCurRecord->mData != NULL )
        {
            *aRow = mCurRecord->mData;
            IDE_TEST( moveNext() != IDE_SUCCESS );
        }
        else
        {
            /* error!! */
        }
    }
    else
    {
        /* no more rows exists, success */
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer 에 record 하나를 insert 한다. 
 *
 *  aRecord     - [IN] insert 할 record
 *
 ************************************************************************/
IDE_RC  dkdRecordBufferMgr::insertRow( dkdRecord  *aRecord )
{
    IDU_LIST_INIT_OBJ( &(aRecord->mNode), aRecord );
    IDU_LIST_ADD_LAST( &mRecordList, &(aRecord->mNode) );

    mRecordCnt++;

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : Record buffer 의 iteration 을 restart 시킨다.
 *
 ************************************************************************/
IDE_RC  dkdRecordBufferMgr::restart()
{
    IDE_TEST( moveFirst() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer 의 cursor 가 record buffer 의 다음 record
 *               를 가리키도록 한다.
 *
 ************************************************************************/
IDE_RC  dkdRecordBufferMgr::moveNext()
{
    dkdRecord       *sRecord = NULL;
    iduListNode     *sNextNode;

    sNextNode = IDU_LIST_GET_NEXT( &mCurRecord->mNode );
    sRecord   = (dkdRecord *)(sNextNode->mObj);

    mCurRecord = sRecord;

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : Record buffer 의 cursor 가 record buffer 의 처음 record
 *               를 가리키도록 한다.
 *
 ************************************************************************/
IDE_RC  dkdRecordBufferMgr::moveFirst()
{
    dkdRecord       *sRecord = NULL;
    iduListNode     *sFirstNode;

    sFirstNode = IDU_LIST_GET_FIRST( &mRecordList );
    sRecord    = (dkdRecord *)(sFirstNode->mObj);

    mCurRecord = sRecord;

    return IDE_SUCCESS;
}

IDE_RC  fetchRowFromRecordBuffer( void *aMgrHandle, void **aRow )
{
    IDE_TEST( ((dkdRecordBufferMgr *)aMgrHandle)->fetchRow( aRow )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC  insertRowIntoRecordBuffer( void *aMgrHandle, dkdRecord *aRecord )
{
    IDE_TEST( ((dkdRecordBufferMgr *)aMgrHandle)->insertRow( aRecord ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC  restartRecordBuffer( void  *aMgrHandle )
{
    IDE_TEST( ((dkdRecordBufferMgr *)aMgrHandle)->restart() 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


