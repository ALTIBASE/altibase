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
 * $Id: sdrMtxStack.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 *
 **********************************************************************/

#include <ide.h>
#include <sdrMtxStack.h>
#include <sdb.h>
#include <smErrorCode.h>

void sdrMtxStack::logMtxStack( sdrMtxStackInfo *aStack )
{
    UInt i;
    
    ideLog::log(SM_TRC_LOG_LEVEL_DEBUG, SM_TRC_MTXSTACK_SIZE_OVERFLOW1);

    for( i = 0; i < aStack->mCurrSize; i++ )
    {
        switch( aStack->mArray[i].mLatchMode )
        {
            case SDR_MTX_PAGE_X:
              ideLog::log(IDE_SM_0,
                          SM_TRC_MTXSTACK_SIZE_OVERFLOW2,
                          i,
                          ((sdbBCB*)aStack->mArray[i].mObject)->mSpaceID,
                          ((sdbBCB*)aStack->mArray[i].mObject)->mPageID);
              break;
            case SDR_MTX_PAGE_S:
              ideLog::log(IDE_SM_0,
                          SM_TRC_MTXSTACK_SIZE_OVERFLOW3,
                          i,
                          ((sdbBCB*)aStack->mArray[i].mObject)->mSpaceID,
                          ((sdbBCB*)aStack->mArray[i].mObject)->mPageID);
              break;
            case SDR_MTX_PAGE_NO:
              ideLog::log(IDE_SM_0,
                          SM_TRC_MTXSTACK_SIZE_OVERFLOW4,
                          i,
                          ((sdbBCB*)aStack->mArray[i].mObject)->mSpaceID,
                          ((sdbBCB*)aStack->mArray[i].mObject)->mPageID);
              break;
            case SDR_MTX_LATCH_X:
              ideLog::log(IDE_SM_0,
                          SM_TRC_MTXSTACK_SIZE_OVERFLOW5,
                          i);
              break;
            case SDR_MTX_LATCH_S:
              ideLog::log(IDE_SM_0,
                          SM_TRC_MTXSTACK_SIZE_OVERFLOW6,
                          i);
              break;
            case SDR_MTX_MUTEX_X:
              ideLog::log(IDE_SM_0,
                          SM_TRC_MTXSTACK_SIZE_OVERFLOW7,
                          i);
              break;
            default:
              IDE_DASSERT( 0 );
              break;
        }
    }
}

/***********************************************************************
 * Description : 마지막 item을 stack에서 얻는다
 *
 * 스택에서 제거하지 않음. 값만얻는다.
 *
 * - 1st code design
 *
 *   aStack->mArray[현재 크기]를 리턴
 **********************************************************************/
sdrMtxLatchItem* sdrMtxStack::getTop(sdrMtxStackInfo*  aStack )
{

    IDE_DASSERT( aStack != NULL );
    return &(aStack->mArray[aStack->mCurrSize-1]);
    
}

/***********************************************************************
 * Description : 현재 stack의 size를 얻는다.
 **********************************************************************/
UInt sdrMtxStack::getTotalSize(sdrMtxStackInfo*  aStack)
{

    IDE_DASSERT( aStack != NULL );
    return aStack->mTotalSize;
}

/***********************************************************************
 * Description : 스택에 해당 아이템을 찾는다.
 *
 * 비교 함수가 제공된다.
 *
 * - 1st. code design
 *
 *   + while( 모든 스택의 아이템에 대해서 )
 *     if( 아이템이 같으면  ) 아이템을 리턴
 *   + null을 리턴
 **********************************************************************/
sdrMtxLatchItem* sdrMtxStack::find( 
    sdrMtxStackInfo*          aStack,
    sdrMtxLatchItem*          aItem,
    sdrMtxItemSourceType      aType,
    sdrMtxLatchItemApplyInfo *aVector )
{

    UInt sLoop;
    UInt sLatchMode;
    sdrMtxLatchItem *sCurrItem;
    UInt sStackSize;
    

    IDE_DASSERT( aStack != NULL );
    IDE_DASSERT( aItem != NULL );
    IDE_DASSERT( validate(aStack) == ID_TRUE );

    sStackSize= getCurrSize(aStack);
    
    for( sLoop = 0; sLoop != sStackSize; ++sLoop )
    {
        // 현재 stack의 item
        sCurrItem = &(aStack->mArray[sLoop]);
        
        // stack에 있는 item의 latch mode
        sLatchMode = sCurrItem->mLatchMode;

        // 스택 아이템의 타입과 인자 타입이 같다. 즉
        // 같은 종류의 오브젝트이며 검사하길 원한다.
        if( aVector[sLatchMode].mSourceType == aType )
        {
            if( aVector[sLatchMode].mCompareFunc(
                                         aItem->mObject,
                                         aStack->mArray[sLoop].mObject )
                == ID_TRUE )
            {
                return sCurrItem;
            }   
        }
    }

    return NULL;
    
}

/***********************************************************************
 * Description : 
 * stack을 dump
 *
 **********************************************************************/
void sdrMtxStack::dump(
    sdrMtxStackInfo          *aStack,
    sdrMtxLatchItemApplyInfo *aVector,
    sdrMtxDumpMode            aDumpMode )
{

    UInt sLoop;
    UInt sLatchMode;
    UInt sStackSize;

    IDE_DASSERT(aStack != NULL);
    IDE_DASSERT(aVector != NULL);
    
    sStackSize = getCurrSize(aStack);

    ideLog::log( IDE_SERVER_0,
                 "----------- Mtx Stack Info Begin ----------\n"
                 "Stack Total Capacity\t: %u\n"
                 "Stack Curr Size\t: %u",
                 getTotalSize(aStack),
                 sStackSize );

    if( aDumpMode == SDR_MTX_DUMP_DETAILED )
    {    
        for( sLoop = 0; sLoop != sStackSize; ++sLoop )
        {
            sLatchMode = aStack->mArray[sLoop].mLatchMode;
            
            ideLog::log( IDE_SERVER_0,
                         "Latch Mode(Stack Num)\t: %s(%u)",
                         (aVector[sLatchMode]).mLatchModeName, sLoop);
            
            (aVector[sLatchMode]).mDumpFunc( aStack->mArray[sLoop].mObject );
        }
    }

    ideLog::log( IDE_SERVER_0, "----------- Mtx Stack Info End ----------" );
}

/***********************************************************************
 *
 * Description : 
 *
 * validate
 *
 * Implementation :
 * 
 **********************************************************************/

#ifdef DEBUG
idBool sdrMtxStack::validate(  sdrMtxStackInfo   *aStack )
{
    UInt sLoop;
    sdrMtxLatchItem *aItem;
    UInt sStackSize;
    
    IDE_DASSERT( aStack != NULL );
    
    sStackSize = getCurrSize(aStack);
    for( sLoop = 0; sLoop != sStackSize; ++sLoop )
    {
        aItem = &(aStack->mArray[sLoop]);
        IDE_DASSERT( aItem != NULL );
        IDE_DASSERT( aItem->mObject != NULL );


        IDE_DASSERT( (aItem->mLatchMode == SDR_MTX_PAGE_X) ||
                     (aItem->mLatchMode == SDR_MTX_PAGE_S) ||
                     (aItem->mLatchMode == SDR_MTX_PAGE_NO) ||
                     (aItem->mLatchMode == SDR_MTX_LATCH_X) ||
                     (aItem->mLatchMode == SDR_MTX_LATCH_S) ||
                     (aItem->mLatchMode == SDR_MTX_MUTEX_X) );
    }

    return ID_TRUE;    
    
}
#endif 
