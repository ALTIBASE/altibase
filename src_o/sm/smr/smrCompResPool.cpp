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
 * $Id: smrCompResPool.cpp 15368 2006-03-23 01:14:43Z kmkim $
 **********************************************************************/

#include <idl.h>
#include <smrCompResPool.h>

/* 객체 초기화
   [IN] aPoolName - 리소스 풀의 이름 
   [IN] aInitialResourceCount - 초기 리소스 갯수
   [IN] aMinimumResourceCount - 최소 리소스 갯수
   [IN] aGarbageCollectionSecond - 리소스가 몇 초 이상 사용되지 않을 경우 Garbage Collection할지?
 */
IDE_RC smrCompResPool::initialize( SChar * aPoolName,
                                   UInt    aInitialResourceCount,    
                                   UInt    aMinimumResourceCount,
                                   UInt    aGarbageCollectionSecond )
{
    IDE_TEST( mCompResList.initialize( aPoolName,
                                        IDU_MEM_SM_SMR )
              != IDE_SUCCESS );

    IDE_TEST( mCompResMemPool.initialize(
                                  IDU_MEM_SM_SMR,
                                  aPoolName,
                                  ID_SCALABILITY_SYS, 
                                  (vULong)ID_SIZEOF(smrCompRes),    /* elem_size */
                                  aInitialResourceCount,            /* elem_count */
                                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                  ID_TRUE,							/* UseMutex   */
                                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                  ID_FALSE,							/* ForcePooling */
                                  ID_TRUE,							/* GarbageCollection */
                                  ID_TRUE,                          /* HWCacheLine */
                                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
              != IDE_SUCCESS);			

    mMinimumResourceCount = aMinimumResourceCount;
    
    // 시간 측정이 Micro단위로 이루어지므로,
    // Garbage Collection할 기준시간을 Micro초로 미리 변환해둔다.
    mGarbageCollectionMicro = aGarbageCollectionSecond * 1000000;

    // Overflow발생 여부 검사 
    IDE_ASSERT( mGarbageCollectionMicro > aGarbageCollectionSecond );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 객체 파괴 */
IDE_RC smrCompResPool::destroy()
{
    IDE_TEST( destroyAllCompRes() != IDE_SUCCESS );
    
    IDE_TEST( mCompResMemPool.destroy() != IDE_SUCCESS );

    IDE_TEST( mCompResList.destroy() != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 로그 압축을 위한 Resource 를 얻는다

   
   [IN] aCompRes - 할당한 로그 압축 Resource
 */
IDE_RC smrCompResPool::allocCompRes( smrCompRes ** aCompRes )
{
    IDE_DASSERT( aCompRes != NULL );
    
    smrCompRes * sCompRes = NULL;
    

    // 1. 재활용 List에서 찾는다
    //      CPU Cache Hit를 높이기 위해,
    //      List의 같은위치(Head)에서 Remove, Add한다.
    IDE_TEST( mCompResList.removeFromHead( (void**) & sCompRes )
              != IDE_SUCCESS );
    
    
    // 2. 재활용 List에 없는 경우 직접 할당
    if ( sCompRes == NULL )
    {
        IDE_TEST( createCompRes( & sCompRes ) != IDE_SUCCESS );
    }

    // 3. 사용된 시각 기록
    if ( IDV_TIME_AVAILABLE() == ID_TRUE )
    {
        IDV_TIME_GET( & sCompRes->mLastUsedTime );
    }
    else
    {
        /* Timer가 초기화 되기 전에 alloc 받는 경우 */
        sCompRes->mLastUsedTime.iTime.mClock = 0;
    }
    
    *aCompRes = sCompRes ;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 로그 압축을 위한 Resource 를 반납
   
   [IN] aCompRes - 반납할 로그 압축 Resource
*/
IDE_RC smrCompResPool::freeCompRes( smrCompRes * aCompRes )
{
    IDE_DASSERT( aCompRes != NULL );

    // CPU Cache Hit를 높이기 위해,
    // List의 같은위치(Head)에서 Remove, Add한다.
    IDE_TEST( mCompResList.addToHead( aCompRes ) != IDE_SUCCESS );

    // 가장 오래 전에 사용된 리소스의 사용시각과 현재시각의 차이가
    // 특정 시간을 벗어나면 Pool에서 제거.
    //
    // Free할 때마다 하나씩만 가비지 콜렉트 한다.
    // 시스템에 갑자기 부하를 주지 않고
    // 가비지 콜렉트 하는 부하를 분산시키기 위함

    IDE_TEST( garbageCollectOldestRes() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    리소스 풀 내에서의 가장 오래된 리소스 하나에 대해
    Garbage Collection을 실시한다.
    
    - 가장 오래 전에 사용된 리소스 (List의 Tail에 매달린 Element)가
      특정 시간동안 사용되지 않은 경우 Garbage Collection실시
 */
IDE_RC smrCompResPool::garbageCollectOldestRes()
{
    smrCompRes * sGarbageRes;
    
    IDE_TEST( mCompResList.removeGarbageFromTail( mMinimumResourceCount,
                                                  mGarbageCollectionMicro,
                                                  & sGarbageRes )
              != IDE_SUCCESS );
    if ( sGarbageRes != NULL )
    {
        // Garbage collection 수행!
        IDE_TEST( destroyCompRes( sGarbageRes )
                  != IDE_SUCCESS );
    }
    else
    {
        // Garbage Collection할 리소스가 없다.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

    
        

/* 로그 압축을 위한 Resource 를 생성한다

   [IN] aCompRes - 로그 압축 Resource
 */
IDE_RC smrCompResPool::createCompRes( smrCompRes ** aCompRes )
{
    IDE_DASSERT( aCompRes != NULL );

    smrCompRes * sCompRes;

    // 리소스 구조체 할당
    /* smrCompResPool_createCompRes_alloc_CompRes.tc */
    IDU_FIT_POINT("smrCompResPool::createCompRes::alloc::CompRes");
    IDE_TEST( mCompResMemPool.alloc( (void**) & sCompRes ) != IDE_SUCCESS );

    // 로그 압축해제 버퍼 핸들의 초기화
    IDE_TEST( sCompRes->mDecompBufferHandle.initialize( IDU_MEM_SM_SMR )
              != IDE_SUCCESS );
    
    // 로그 압축버퍼 핸들의 초기화
    IDE_TEST( sCompRes->mCompBufferHandle.initialize( IDU_MEM_SM_SMR )
              != IDE_SUCCESS );
    
    *aCompRes = sCompRes ;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 로그 압축을 위한 Resource 를 파괴한다

   [IN] aCompRes - 로그 압축 Resource
 */
IDE_RC smrCompResPool::destroyCompRes( smrCompRes * aCompRes )
{
    IDE_DASSERT( aCompRes != NULL );

    // 로그 압축해제 핸들의 파괴
    IDE_TEST( aCompRes->mDecompBufferHandle.destroy() != IDE_SUCCESS );
    
    // 로그 압축버퍼 핸들의 파괴
    IDE_TEST( aCompRes->mCompBufferHandle.destroy() != IDE_SUCCESS );

    IDE_TEST( mCompResMemPool.memfree( aCompRes ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 이 Pool이 생성한 모든 압축 Resource 를 파괴한다

   [IN] aCompRes - 로그 압축 Resource

 */
IDE_RC smrCompResPool::destroyAllCompRes()
{

    smrCompRes * sCompRes;

    
    for(;;)
    {
        IDE_TEST( mCompResList.removeFromHead( (void**) & sCompRes )
                  != IDE_SUCCESS );

        if ( sCompRes == NULL ) // 더 이상 List Node가 없는 경우 
        {
            break;
        }

        IDE_TEST( destroyCompRes( sCompRes ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 로그 압축 리소스가 가진 버퍼의 크기를 조절한다.

   [IN] aCompRes - 로그 압축 Resource
   [IN] aSize    - 조절할 크기

 */
IDE_RC smrCompResPool::tuneCompRes( smrCompRes * aCompRes,
                                    UInt         aSize )
{
    IDE_DASSERT( aCompRes != NULL );

    IDE_TEST( aCompRes->mDecompBufferHandle.tuneSize( aSize ) != IDE_SUCCESS );
    
    IDE_TEST( aCompRes->mCompBufferHandle.tuneSize( aSize ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


