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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideMsgLog.h>
#include <idErrorCode.h>
#include <iduShmDef.h>
#include <iduShmKeyMgr.h>

iduShmSSegment  * iduShmKeyMgr::mSSegment;

iduShmKeyMgr::iduShmKeyMgr()
{
}

iduShmKeyMgr::~iduShmKeyMgr()
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
IDE_RC iduShmKeyMgr::getShmKeyCandidate( key_t * aShmKeyCandidate )
{
    IDE_ASSERT( mSSegment != NULL );
    IDE_ASSERT( aShmKeyCandidate != NULL );

    // 사용할 수 있는 Key후보가 있는가?
    if( mSSegment->mNxtShmKey > IDU_MIN_SHM_KEY_CANDIDATE )
    {
        *aShmKeyCandidate = --mSSegment->mNxtShmKey;
    }
    else /* Rotate shm key */
    {
        mSSegment->mNxtShmKey = IDU_MAX_SHM_KEY_CANDIDATE;
        *aShmKeyCandidate = mSSegment->mNxtShmKey;
    }

    return IDE_SUCCESS;

#if 0
    IDE_EXCEPTION( error_no_more_key );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_No_More_Shm_Key ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/*
 * shmShmKeyMgr의 static 초기화 수행
 */
IDE_RC iduShmKeyMgr::initializeStatic( iduShmSSegment *aSSegment )
{
    mSSegment = aSSegment;

    return IDE_SUCCESS;
}

/*
 * shmShmKeyMgr의 static 파괴 수행
 */
IDE_RC iduShmKeyMgr::destroyStatic()
{
    mSSegment = NULL;

    return IDE_SUCCESS;
}


