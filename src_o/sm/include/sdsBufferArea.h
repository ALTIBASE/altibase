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
 * $Id:$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer
 **********************************************************************/

#ifndef  _O_SDS_BUFFERAREA_H_
#define  _O_SDS_BUFFERAREA_H_  1

#include <idl.h>
#include <idu.h>

#include <sdsBCB.h>

/* applyFuncToEachBCBs 함수 내에서 각 BCB에 적용하는 함수의 형식 */
typedef IDE_RC (*sdsBufferAreaActFunc)( sdsBCB *aSBCB, void *aFiltAgr );

typedef enum
{
    SDS_EXTENT_STATE_FREE,
    SDS_EXTENT_STATE_MOVEDOWN_ING,
    SDS_EXTENT_STATE_MOVEDOWN_DONE,
    SDS_EXTENT_STATE_FLUSH_ING,
    SDS_EXTENT_STATE_FLUSH_DONE
}sdsExtentStateType;

class sdsBufferArea
{
public:
    IDE_RC initializeStatic( UInt aExtentCnt, UInt aBCBCnt );

    IDE_RC destroyStatic( void );

    idBool getTargetFlushExtentIndex( UInt *aExtentIndex );

    IDE_RC getTargetMoveDownIndex( idvSQL  * aStatistics, 
                                   UInt *aExtentIndex );

    IDE_RC changeStateFlushDone( UInt aExtentIndex );
  
    IDE_RC changeStateMovedownDone( UInt aExtentIndex );

    IDE_RC waitForFlushJobDone ( idvSQL  * aStatistics, idBool *aSuccess );

    inline UInt getBCBCount( void );

    inline sdsBCB* getBCB( UInt aIndex );

    inline void getExtentCnt( UInt * aMovedoweIngPages,
                              UInt * aMovedownDonePages,
                              UInt * aFlushIngPages,
                              UInt * aFlushDonePages ); 

    inline idBool hasExtentMovedownDone( void );

    inline UInt getLastFlushExtent();

    /* 통계 정보 관련 함수 */
    IDE_RC applyFuncToEachBCBs( sdsBufferAreaActFunc    aFunc,
                                void                  * aObj);
private:

public :

private:
    /* BCB를 접근할때 필요한 정보*/
    sdsBCB             ** mBCBArray;
    /* BCB 개수 */
    UInt                  mBCBCount;
    /*  BCB Extent 상태 */
    sdsExtentStateType  * mBCBExtState;
    /* BCB Extent 개수 */
    UInt                  mBCBExtentCount;
    /* BCB 할당을 위한 메모리 풀 */
    iduMemPool            mBCBMemPool;
    /* Extent를 검색 위한 mutex */
    iduMutex              mExtentMutex;     
    /* Secondaty Buffer에 여유공간이 없을때 대기하기 위한 mutex */
    iduMutex              mMutexForWait;
    /* Secondary Buffer에 mmovedown 할때 사용 */
    UInt                  mMovedownPos;
    /* Secondary Buffer를 flush 할때 사용 */
    UInt                  mFlushPos;
    /* Secondaty Byffer에 여유공간이 없을때  대기하기 위한 cond variable */
    iduCond               mCondVar;
    /* 대기중인 쓰레드의 갯수 */
    UInt                  mWaitingClientCount;
};

/******************************************************************************
 * Description :
 ******************************************************************************/
UInt sdsBufferArea::getBCBCount ( void )
{
    return mBCBCount;
}

sdsBCB* sdsBufferArea::getBCB( UInt aIndex )
{
    IDE_ASSERT( aIndex < mBCBCount );

    return mBCBArray[aIndex];  
}

/******************************************************************************
 * Description :
 * 1.aMovedoweIngPages [OUT] - movedown 중인 Ext 
 * 2.aMovedownDonePages[OUT] - movedown 완료된 Ext
 * 3.aFlushIngPages    [OUT] - flush 중인 Ext
 * 4.aFlushDonePages   [OUT] - flush 완료된 Ext + free Ext
   1+2 가 Flush 대상의 Ext
 ******************************************************************************/
void sdsBufferArea::getExtentCnt( UInt * aMovedoweIngPages,
                                  UInt * aMovedownDonePages,
                                  UInt * aFlushIngPages,
                                  UInt * aFlushDonePages ) 
{
    UInt sMovedoweIngPages  = 0;
    UInt sMovedownDonePages = 0;
    UInt sFlushIngPages     = 0; 
    UInt sFlushDonePages    = 0; 
    UInt i;
    UInt sFrameCntInExt;

    sFrameCntInExt = mBCBCount/mBCBExtentCount;

    for( i = 0 ; i< mBCBExtentCount ; i++ )
    {
        switch( mBCBExtState[i] )
        {
            case SDS_EXTENT_STATE_MOVEDOWN_ING:
                sMovedoweIngPages++;
                break;

            case SDS_EXTENT_STATE_MOVEDOWN_DONE:
                sMovedownDonePages++;
                break;

            case SDS_EXTENT_STATE_FLUSH_ING:
                sFlushIngPages++;
                break;

            case SDS_EXTENT_STATE_FLUSH_DONE:
                sFlushDonePages++;                
                break;

            default:
                break;
        }
    }

    if( aMovedoweIngPages != NULL )
    {
        *aMovedoweIngPages = sMovedoweIngPages * sFrameCntInExt;
    }
    if( aMovedownDonePages != NULL )
    {
        *aMovedownDonePages = sMovedownDonePages * sFrameCntInExt;
    }
    if( aFlushIngPages != NULL )
    {
        *aFlushIngPages = sFlushIngPages * sFrameCntInExt; 
    }
    if( aFlushDonePages != NULL )
    {
        *aFlushDonePages = sFlushDonePages * sFrameCntInExt;
    }
}

/******************************************************************************
 * Description :
 ******************************************************************************/
idBool sdsBufferArea::hasExtentMovedownDone()
{
    idBool sRet = ID_FALSE;

    if( mBCBExtState[mFlushPos] == SDS_EXTENT_STATE_MOVEDOWN_DONE )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

UInt sdsBufferArea::getLastFlushExtent()
{
    UInt sRet;

    IDE_ASSERT( mExtentMutex.lock( NULL /*aStatistics*/ ) == IDE_SUCCESS );

    if( mFlushPos != 0 )
    {
        sRet = mFlushPos-1;
    }
    else
    {
        sRet = mBCBExtentCount-1; 
    } 

    IDE_ASSERT( mExtentMutex.unlock() == IDE_SUCCESS );

    return sRet;
}
#endif
