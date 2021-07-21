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
 * $Id: smuSynchroList.cpp 15368 2006-03-23 01:14:43Z kmkim $
 **********************************************************************/

#include <idl.h>
#include <smuProperty.h>
#include <smuSynchroList.h>

/* 객체 초기화
   
   [IN] aListName - List의 이름
   [IN] aMemoryIndex - List Node를 할당할 Memory Pool에 기록할 모듈명
 */
IDE_RC smuSynchroList::initialize( SChar * aListName,
                                   iduMemoryClientIndex aMemoryIndex)
{
    SChar sName[128];

    idlOS::snprintf(sName, 128, "%s_MUTEX",
                    aListName );
    
    IDE_TEST( mListMutex.initialize( sName,
                                     IDU_MUTEX_KIND_NATIVE,
                                     IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
        

    idlOS::snprintf(sName, 128, "%s_NODE_POOL",
                    aListName );
    
    IDE_TEST( mListNodePool.initialize(
                  aMemoryIndex,
                  sName,
                  ID_SCALABILITY_SYS, 
                  (vULong)ID_SIZEOF(smuList),  // elem_size
                  128,                         // elem_count
                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                  ID_TRUE,							/* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                  ID_FALSE,							/* ForcePooling */
                  ID_TRUE,							/* GarbageCollection */
                  ID_TRUE,                          /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS );			
        
    SMU_LIST_INIT_BASE( & mList );

    mElemCount = 0;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 객체 파괴 */
IDE_RC smuSynchroList::destroy()
{
    // List에 List Node가 남아있어서는 안된다.
    IDE_DASSERT( SMU_LIST_IS_EMPTY( &mList ) );
    IDE_DASSERT( mElemCount == 0 );
    
    SMU_LIST_INIT_BASE( & mList );

    IDE_TEST( mListNodePool.destroy() != IDE_SUCCESS );
    
    IDE_TEST( mListMutex.destroy() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Linked List의 Head로부터 Data를 제거 

   [IN] aData - Linked List에 매달아 놓은 데이터의 포인터
                만약 List가 비어있다면 NULL을 리턴한다.
 */
IDE_RC smuSynchroList::removeFromHead( void ** aData )
{
    IDE_DASSERT( aData != NULL );
    
    UInt         sStage = 0;
    smuList    * sListNode = NULL;

    IDE_TEST( mListMutex.lock( NULL ) != IDE_SUCCESS );
    sStage = 1;

    if ( ! SMU_LIST_IS_EMPTY( & mList ) )
    {
        sListNode = SMU_LIST_GET_FIRST( & mList );

        SMU_LIST_DELETE( sListNode );

        mElemCount --;
    }
    
    sStage = 0;
    IDE_TEST( mListMutex.unlock() != IDE_SUCCESS );

    if ( sListNode == NULL )
    {
        // List가 비어있는 경우
        *aData = NULL;
    }
    else
    {
        *aData = sListNode->mData ;

        // List에 Add할때부터 
        // 데이터의 포인터로 NULL을 허용하지 않았으므로,
        // Data의 포인터가 NULL일 수 없다.
        IDE_ASSERT( *aData != NULL );
        
        IDE_TEST( mListNodePool.memfree(sListNode) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1 :
                IDE_ASSERT( mListMutex.unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();
    
    return IDE_FAILURE;
}


/* Linked List의 Tail로부터 Data를 제거 

   [IN] aData - Linked List에 매달아 놓은 데이터의 포인터
                만약 List가 비어있다면 NULL을 리턴한다.
 */
IDE_RC smuSynchroList::removeFromTail( void ** aData )
{
    IDE_DASSERT( aData != NULL );
    
    UInt         sStage = 0;
    smuList    * sListNode = NULL;

    IDE_TEST( mListMutex.lock(NULL) != IDE_SUCCESS );
    sStage = 1;

    if ( ! SMU_LIST_IS_EMPTY( & mList ) )
    {
        sListNode = SMU_LIST_GET_LAST( & mList );

        SMU_LIST_DELETE( sListNode );

        mElemCount --;
    }
    
    sStage = 0;
    IDE_TEST( mListMutex.unlock() != IDE_SUCCESS );

    if ( sListNode == NULL )
    {
        // List가 비어있는 경우
        *aData = NULL;
    }
    else
    {
        *aData = sListNode->mData ;

        // List에 Add할때부터 
        // 데이터의 포인터로 NULL을 허용하지 않았으므로,
        // Data의 포인터가 NULL일 수 없다.
        IDE_ASSERT( *aData != NULL );
        
        IDE_TEST( mListNodePool.memfree(sListNode) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1 :
                IDE_ASSERT( mListMutex.unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();
    
    return IDE_FAILURE;
}


/* Linked List의 Tail을 리턴, 리스트에서 제거하지는 않음 )

   [IN] aData - Linked List에 매달아 놓은 데이터의 포인터
                만약 List가 비어있다면 NULL을 리턴한다.
 */
IDE_RC smuSynchroList::peekTail( void ** aData )
{
    IDE_DASSERT( aData != NULL );
    
    UInt         sStage = 0;
    smuList    * sListNode = NULL;

    IDE_TEST( mListMutex.lock(NULL) != IDE_SUCCESS );
    sStage = 1;

    if ( ! SMU_LIST_IS_EMPTY( & mList ) )
    {
        sListNode = SMU_LIST_GET_LAST( & mList );

    }
    
    sStage = 0;
    IDE_TEST( mListMutex.unlock() != IDE_SUCCESS );

    if ( sListNode == NULL )
    {
        // List가 비어있는 경우
        *aData = NULL;
    }
    else
    {
        *aData = sListNode->mData ;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1 :
                IDE_ASSERT( mListMutex.unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();
    
    return IDE_FAILURE;
}


/* Linked List의 Head에 Data를 Add 

   [IN] aData - Linked List에 매달아 놓을 데이터의 포인터
 */
IDE_RC smuSynchroList::addToHead( void * aData )
{
    IDE_DASSERT( aData != NULL );
    
    UInt         sStage = 0;
    smuList    * sListNode ;

    // Pool에서 List Node할당
    // List Pool이 별도의 동시성 제어를 하기 때문에
    // mListMutex를 잡지 않은 채로 수행가능.
    IDE_TEST( mListNodePool.alloc( (void**) & sListNode ) != IDE_SUCCESS );

    SMU_LIST_INIT_NODE( sListNode );
    sListNode->mData = aData;
    
    IDE_TEST( mListMutex.lock( NULL ) != IDE_SUCCESS );
    sStage = 1;
    
    SMU_LIST_ADD_FIRST( &mList, sListNode );

    mElemCount ++;
    
    sStage = 0;
    IDE_TEST( mListMutex.unlock() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1 :
                IDE_ASSERT( mListMutex.unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();
    
    return IDE_FAILURE;
}


/* Linked List의 Tail에 Data를 Add 

   [IN] aData - Linked List에 매달아 놓을 데이터의 포인터
 */
IDE_RC smuSynchroList::addToTail( void * aData )
{
    IDE_DASSERT( aData != NULL );
    
    UInt         sStage = 0;
    smuList    * sListNode ;

    // Pool에서 List Node할당
    // List Pool이 별도의 동시성 제어를 하기 때문에
    // mListMutex를 잡지 않은 채로 수행가능.
    IDE_TEST( mListNodePool.alloc( (void**) & sListNode ) != IDE_SUCCESS );

    SMU_LIST_INIT_NODE( sListNode );
    sListNode->mData = aData;
    
    IDE_TEST( mListMutex.lock(NULL) != IDE_SUCCESS );
    sStage = 1;
    
    SMU_LIST_ADD_LAST( &mList, sListNode );

    mElemCount ++;
    
    sStage = 0;
    IDE_TEST( mListMutex.unlock() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1 :
                IDE_ASSERT( mListMutex.unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();
    
    return IDE_FAILURE;
}


/* Linked List안의 Element갯수를 리턴한다

   [OUT] aElemCount - 리스트 안의 Element갯수
 */
IDE_RC smuSynchroList::getElementCount( UInt * aElemCount )
{
    IDE_DASSERT( aElemCount != NULL );


    *aElemCount = mElemCount;

    return IDE_SUCCESS;
}
