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
 * $Id: smmShmKeyMgr.cpp 18299 2006-09-26 07:51:56Z xcom73 $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smmShmKeyList.h>
#include <smmShmKeyMgr.h>


smmShmKeyList smmShmKeyMgr::mFreeKeyList; // 재활용 공유메모리 Key List
iduMutex      smmShmKeyMgr::mMutex; 
key_t         smmShmKeyMgr::mSeekKey;

// 사용가능한 공유메모리 Key중 가장 작은 값 
#define SMM_MIN_SHM_KEY_CANDIDATE (1024)

smmShmKeyMgr::smmShmKeyMgr()
{
}
smmShmKeyMgr::~smmShmKeyMgr()
{
}



/*
   다음 사용할 공유메모리 Key 후보를 찾아 리턴한다.

   후보인 이유 :
     해당 Key를 이용하여 새 공유메모리 영역을 생성할 수 있을 수도 있고
     이미 해당 Key로 공유메모리 영역이 생성되어 있을 수도 있기 때문.

   aShmKeyCandidate [OUT] 0 : 공유메모리 Key 후보가 없음
                          Otherwise : 사용할 수 있는 공유메모리 Key 후보

   PROJ-1548 Memory Tablespace
   - mSeekKey에 대한 동시성 제어를 하는 이유
     - mSeekKey는 여러 Tx가 동시에 접근할 수 있다.
     - mSeekKey는 여러 Tablespace에서 동시에 접근할 수 있다.

  - 가능하면 이전에 다른 Tablespace에서 사용되었던 Key를
     재활용하여 사용한다.
*/
IDE_RC smmShmKeyMgr::getShmKeyCandidate(key_t * aShmKeyCandidate) 
{
    UInt sState = 0;
    
    IDE_DASSERT( aShmKeyCandidate != NULL )
    
    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;
    
    
    // 재활용 Key List에 재활용 Key가 있다면 이를 먼저 사용한다.
    if ( mFreeKeyList.isEmpty() == ID_FALSE )
    {
        IDE_TEST( mFreeKeyList.removeKey( aShmKeyCandidate ) != IDE_SUCCESS );
    }
    else // 재활용 Key List 에 Key가 없다.
    {
        // 사용할 수 있는 Key후보가 있는가?
        if ( mSeekKey > SMM_MIN_SHM_KEY_CANDIDATE ) 
        {
            *aShmKeyCandidate = --mSeekKey;
        }
        else // 사용할 수 있는 Key후보가 없다.
        {
            IDE_RAISE(error_no_more_key);
        }
    }

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(error_no_more_key);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_No_More_Shm_Key));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1 :
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 * 현재 사용중인 Key를 알려준다.
 *
 * 이 함수는 attach시에 호출되며,
 * 이미 사용된 키보다 큰 값을 공유메모리 후보키로 사용하지 않도록 한다.
 *
 * aUsedKey [IN] 현재 사용중인 Key
 */ 
IDE_RC smmShmKeyMgr::notifyUsedKey(key_t aUsedKey)
{
    UInt sState = 0;
    
    IDE_DASSERT( aUsedKey != 0 );
    
    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;
    
    // BUGBUG-1548 FreeKey List를 Incremental하게 구축 해주어야 한다.
    if ( mSeekKey >= aUsedKey ) 
    {
        // 이미 사용된 Key보다 하나 작은 Key부터 사용한다.
        mSeekKey = aUsedKey -1;
    }

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1 :
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();
    
    return IDE_FAILURE;
}


/*
 * 더이상 사용되지 않는 Key를 알려준다.
 *
 * 이 함수는 Tablespace drop/offline시 공유메모리 영역을 제거할때
 * 호출되며, 더이상 사용되지 않는 공유메모리 Key를 다른
 * Tablespace에서 재활용될 수 있도록 돕는다.
 *
 * aUnusedKey [IN] 더이상 사용되지 않는 Key
 */ 
IDE_RC smmShmKeyMgr::notifyUnusedKey(key_t aUnusedKey)
{
    UInt sState = 0;
    
    IDE_DASSERT( aUnusedKey != 0 );
    
    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;
    
    // 재활용 Key List에 추가한다.
    IDE_TEST( mFreeKeyList.addKey( aUnusedKey ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1 :
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 * shmShmKeyMgr의 static 초기화 수행 
 */
IDE_RC smmShmKeyMgr::initializeStatic()
{
    // 재활용 Key관리자 초기화
    IDE_TEST( mFreeKeyList.initialize() != IDE_SUCCESS );
    
    // mSeekKey값을 Read/Write할 때 잡는 Mutex
    IDE_TEST( mMutex.initialize((SChar*)"SHARED_MEMORY_KEY_MUTEX",
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );
    
    mSeekKey = (key_t) smuProperty::getShmDBKey();
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * shmShmKeyMgr의 static 파괴 수행 
 */
IDE_RC smmShmKeyMgr::destroyStatic()
{
    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    IDE_TEST( mFreeKeyList.destroy() != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

