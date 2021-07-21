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
 * $Id:$
 **********************************************************************/

#include <idl.h>
#include <iduFile.h>
#include <iduFXStack.h>

/***********************************************************************
 * Description : Stack을 초기화 한다. 스택에 필요한 Memory는 이미 할당되
 *               되어 있다고 가정하고 출발한다. 여기서는 Mutex와 여타
 *               다른 Member만 초기화 한다. 이렇게 하는 이유는 가급적
 *               malloc을 호출하지 않기 위해서 이다.
 *
 * aStackName   - [IN] Stack 이름, 보통 스택을 사용목적을 기술한다.
 * aStackInfo   - [IN] iduFXStackInfo로서 스택 자료구조
 * aMaxItemCnt  - [IN] 스택이 가질수 있는 최대 Item갯수
 * aItemSize    - [IN] 스택의 Item 크기.
 **********************************************************************/
IDE_RC iduFXStack::initialize( SChar          *aMtxName,
                               iduFXStackInfo *aStackInfo,
                               UInt            aMaxItemCnt,
                               UInt            aItemSize )
{
    IDE_TEST( aStackInfo->mMutex.initialize(
                  aMtxName,
                  IDU_MUTEX_KIND_POSIX,
                  IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST_RAISE(aStackInfo->mCV.initialize() != IDE_SUCCESS,
                   err_cond_init);

    aStackInfo->mMaxItemCnt = aMaxItemCnt;
    aStackInfo->mCurItemCnt = 0;
    aStackInfo->mItemSize   = aItemSize;
    aStackInfo->mWaiterCnt  = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_init );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondInit ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Stack의 Mutex와 Condition Variable을 정리한다.
 *
 * aStackInfo - [IN] Stack 자료구조.
 **********************************************************************/
IDE_RC iduFXStack::destroy( iduFXStackInfo *aStackInfo )
{
    IDE_ASSERT( iduFXStack::isEmpty( aStackInfo ) == ID_TRUE );

    IDE_TEST_RAISE( aStackInfo->mCV.destroy() != IDE_SUCCESS,
                    err_cond_dest);

    IDE_TEST( aStackInfo->mMutex.destroy()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_dest );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondDestroy ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Stack에 새로운 Item을 Push한다. Max개 이상 넣으면
 *               ASSERT로 죽게 한다. 새로운 Item을 Push하고 혹시
 *               새로운 Item이 Push되기를 기다리는 Transa
 *
 * aStackInfo - [IN] Stack 자료 구조
 * aItem      - [IN] Push할 Stack의 Item
 **********************************************************************/
IDE_RC iduFXStack::push( idvSQL         *aStatSQL,
                         iduFXStackInfo *aStackInfo,
                         void           *aItem )
{
    UInt   sCurItemCnt;
    UInt   sItemSize;
    SChar *sPushPos;
    SInt   sState = 0;

    IDE_ASSERT( aStackInfo->mMutex.lock( aStatSQL )
                == IDE_SUCCESS );
    sState = 1;

    sCurItemCnt = aStackInfo->mCurItemCnt;
    sItemSize   = aStackInfo->mItemSize;

    /* Max개 이상 넣으면 죽는다. */
    IDE_ASSERT( sCurItemCnt < aStackInfo->mMaxItemCnt );

    sPushPos = aStackInfo->mArray + sCurItemCnt * sItemSize;

    /* 아이템의 크기가 vULong크기면 memcpy를 호출하지 않고 Assign을 한다. 유의
       할 점은 aItem의 시작주소가 8Byte Align되어 있어야 한다. */
    if( sItemSize == ID_SIZEOF(UInt) )
    {
        *((UInt*)sPushPos) = *((UInt*)aItem);
    }
    else
    {
        if( sItemSize == ID_SIZEOF(ULong) )
        {
            *((ULong*)sPushPos) = *((ULong*)aItem);
        }
        else
        {
            idlOS::memcpy( aStackInfo->mArray + sCurItemCnt * sItemSize,
                           aItem,
                           sItemSize );
        }
    }

    /* 새로운 Item이 Push되었으므로 현재 Item의 갯수를 1증가 시킨다. */
    aStackInfo->mCurItemCnt++;

    /* 새로운 Item이 Insert되기를 대기하는 Thread들을 깨워준다. */
    IDE_TEST( getupWaiters( aStackInfo ) != IDE_SUCCESS );

    sState = 0;
    IDE_ASSERT( aStackInfo->mMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState !=  0 )
    {
        IDE_ASSERT( aStackInfo->mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 새로운 Item을 Stack에 Push한다.
 *
 * aStackInfo - [IN] Stack 자료 구조.
 * aWaitMode  - [IN] When stack is empty,
 *                   if aWaitMode = IDU_FXSTACK_POP_WAIT 이면,
 *                     새로운 Item이 Push될때까지 대기,
 *                   else
 *                     Stack이 비었다고 return한다.
 *
 * aPopItem   - [IN] Pop된 Item이 복사될 메모리 영역
 * aIsEmpty   - [IN] Stack이 비었으면 ID_TRUE, 아니면 ID_FALSE
 **********************************************************************/
IDE_RC iduFXStack::pop( idvSQL             *aStatSQL,
                        iduFXStackInfo     *aStackInfo,
                        iduFXStackWaitMode  aWaitMode,
                        void               *aPopItem,
                        idBool             *aIsEmpty )
{
    UInt   sCurItemCnt;
    UInt   sItemSize;
    SChar *sPopPos;
    SInt   sState = 0;

    *aIsEmpty   = ID_TRUE;

    IDE_ASSERT( aStackInfo->mMutex.lock( aStatSQL )
                == IDE_SUCCESS );
    sState = 1;

    while(1)
    {
        sCurItemCnt = aStackInfo->mCurItemCnt;

        if( ( sCurItemCnt == 0 ) &&
            ( aWaitMode   == IDU_FXSTACK_POP_WAIT ) )
        {
            /* 새로운 Push가 발생할때까지 기다린다. */
            IDE_TEST( waitForPush( aStackInfo ) != IDE_SUCCESS );
        }
        else
        {
            break;
        }
    }

    /* 현재 Stack이 비었다고 Return한다. 이때 aWaitMode가
       IDU_FXSTACK_POP_NOWAIT 이어야 한다. */
    IDE_TEST_CONT( sCurItemCnt == 0, STACK_EMPTY );

    sItemSize = aStackInfo->mItemSize;

    sCurItemCnt--;
    sPopPos = aStackInfo->mArray + ( sCurItemCnt ) * sItemSize;

    /* Item크기가 vULong 크기이면 memcpy를 부르지 않고 assign시킨다. */
    if( sItemSize == ID_SIZEOF(UInt) )
    {
        *((UInt*)aPopItem) = *((UInt*)sPopPos);
    }
    else
    {
        if( sItemSize == ID_SIZEOF(ULong) )
        {
            *((ULong*)aPopItem) = *((ULong*)sPopPos);
        }
        else
        {
            idlOS::memcpy( aPopItem,
                           sPopPos,
                           sItemSize );
        }
    }

    /* Stack에서 Item을 제거했으므로 1 감소 시킨다. */
    aStackInfo->mCurItemCnt = sCurItemCnt;

    *aIsEmpty = ID_FALSE;

    IDE_EXCEPTION_CONT( STACK_EMPTY );

    sState = 0;
    IDE_ASSERT( aStackInfo->mMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( aStackInfo->mMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}
