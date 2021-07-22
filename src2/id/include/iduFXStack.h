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

#ifndef _O_IDU_FXSTACK_H_
#define _O_IDU_FXSTACK_H_ 1

#include <idl.h>
#include <ide.h>
#include <iduCond.h>
#include <iduMutex.h>

typedef struct iduFXStackInfo
{
    UInt       mMaxItemCnt;  // 최대 스택이 가질수 있는 Item의 갯수
    UInt       mCurItemCnt;  // 현재 스택의 가질수 있는 Item의 갯수
    UInt       mItemSize;    // Item의 크기(Byte)

    UInt       mAlign;       // Align을 위해서 추가.

    iduMutex        mMutex;  // 동시성 제어를 위해.
    iduCond         mCV;     // pop이 대기할때 사용.
    UInt            mWaiterCnt;

    SChar      mArray[1];       // Item의 가지고 있는 Stack Array
} iduFXStackInfo;

/* Stack에 대해서 Pop연산 수행시 Stack이 비어있을때 새로운 Item이
   Push될때까지 무한 대기를 할지 바로 비었다고 리턴할지를 결정  */
typedef enum iduFXStackWaitMode
{
    IDU_FXSTACK_POP_WAIT = 0, /* 무한대기 */
    IDU_FXSTACK_POP_NOWAIT    /* 바로리턴 */
} iduFXStackWaitMode;

class iduFXStack
{
public:
    static IDE_RC initialize( SChar          *aMtxName,
                              iduFXStackInfo *aStackInfo,
                              UInt            aMaxItemCnt,
                              UInt            aItemSize );

    static IDE_RC destroy( iduFXStackInfo *aStackInfo );

    static IDE_RC push( idvSQL         *aStatSQL,
                        iduFXStackInfo *aStackInfo,
                        void           *aItem );

    static IDE_RC pop( idvSQL             *aStatSQL,
                       iduFXStackInfo     *aStackInfo,
                       iduFXStackWaitMode  aWaitMode,
                       void               *aPopItem,
                       idBool             *aIsEmpty );

    static inline idBool isEmpty( iduFXStackInfo *aStackInfo );

    static inline IDE_RC waitForPush( iduFXStackInfo *aStackInfo );
    static inline IDE_RC getupWaiters( iduFXStackInfo *aStackInfo );

    static inline SInt getItemCnt( iduFXStackInfo *aStackInfo ) { return aStackInfo->mCurItemCnt;};
};

/* 스택이 비었는지를 리턴 */
inline idBool iduFXStack::isEmpty( iduFXStackInfo *aStackInfo )
{
    return ( aStackInfo->mCurItemCnt == 0 ? ID_TRUE : ID_FALSE ) ;
}

/* 스택에 새로운 Item이 Push될때까지 대기 */
inline IDE_RC iduFXStack::waitForPush( iduFXStackInfo *aStackInfo )
{
    IDE_RC rc;

    aStackInfo->mWaiterCnt++;

    rc = aStackInfo->mCV.wait(&(aStackInfo->mMutex));

    aStackInfo->mWaiterCnt--;

    IDE_TEST_RAISE( rc != IDE_SUCCESS, err_cond_wait );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_wait );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondWait ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Stack에 Item이 Push되기를 대기하는 Waiter를 깨운다. */
inline IDE_RC iduFXStack::getupWaiters( iduFXStackInfo *aStackInfo )
{
    if( aStackInfo->mWaiterCnt > 0 )
    {
        IDE_TEST_RAISE( aStackInfo->mCV.signal() != IDE_SUCCESS,
                        err_cond_signal );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_signal );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondSignal ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif /* _O_IDU_FXSTACK_H_ */
