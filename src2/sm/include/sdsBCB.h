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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer
 **********************************************************************/

#ifndef _O_SDS_BCB_H_
#define _O_SDS_BCB_H_ 1

#include <iduLatch.h>
#include <iduMutex.h>
#include <idv.h>
#include <idl.h>

#include <smDef.h>
#include <sdbDef.h>
#include <sdbBCB.h> 

/* --------------------------------------------------------------------
 * 특정 BCB가 어떠한 조건을 만족하는지 여부를
 * 검사할때 사용
 * ----------------------------------------------------------------- */
typedef idBool (*sdsFiltFunc)( void *aBCB, void *aFiltAgr );

/* --------------------------------------------------------------------
 * BCB의 상태 정의
 * ----------------------------------------------------------------- */
typedef enum
{
    SDS_SBCB_FREE = 0,  /*현재 사용되지 않는 상태. hash에서 제거되어 있다.*/
    SDS_SBCB_CLEAN,     /* hash에서 접근가능하고 변경된 내용 없음 
                         * 디스크 IO없이 그냥 replace가 가능하다. 
                         * 이후 상태가 CLEAN으로 다시 내려오면 쓰지 않는다. */
    SDS_SBCB_DIRTY,     /* hash에서 접근가능하고 변경된 내용 있음 . 
                         * replace하려면 HDD에 flush 해야 한다. */
    SDS_SBCB_INIOB,     /* flusher가 flush를 위해 자신의 내부 버퍼(IOB)에 
                         * 현 BCB 내용을 저장한 상태 */
    SDS_SBCB_OLD        /* IOB에 있는데 다음 페이지가 들어와서 Flusher가 
                           해당 작업을 마무리후 지워야 하는 상태 */
} sdsSBCBState;

class sdsBCB
{

public:
    IDE_RC initialize( UInt aBCBID );

    IDE_RC destroy( void );

    IDE_RC setFree( void );

    IDE_RC dump( sdsBCB* aSBCB );

    /* BCB mutex */
    inline IDE_RC lockBCBMutex( idvSQL *aStatistics )
    {
        return mBCBMutex.lock( aStatistics);
    }

    inline IDE_RC unlockBCBMutex( void )
    {
        return mBCBMutex.unlock() ;
    }

    /* ReadIOMutex  mutex */
    inline IDE_RC lockReadIOMutex( idvSQL *aStatistics )
    {
        return mReadIOMutex.lock( aStatistics );
    }

    inline IDE_RC unlockReadIOMutex( void )
    {
        return mReadIOMutex.unlock();
    }

    inline IDE_RC tryLockReadIOMutex( idBool * aLocked )
    {
        return mReadIOMutex.trylock( * aLocked );
    }

private:

public:
    /* 공통 부분 */
    SD_BCB_PARAMETERS
    /* SBCB 고유 ID */
    ULong          mSBCBID;            
    /* 해당 page의 상태. 즉 dirty / clean 상태 */
    sdsSBCBState   mState;         
    /* state를 참조,갱신할때 획득한다 */
    iduMutex       mBCBMutex;     
    /* 읽기와 제거연산을 위한 mutex */
    iduMutex       mReadIOMutex;     
    /* 대응하는 BCB */
    sdbBCB       * mBCB;           
    /* Page가 Secondary에 쓰여 질때 page의 LSN */
    smLSN          mPageLSN;

private:

};

#endif //_O_SDS_BCB_H_

