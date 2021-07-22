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
 * $Id: smrCompResList.cpp 15368 2006-03-23 01:14:43Z kmkim $
 **********************************************************************/

#include <idl.h>
#include <smrCompResList.h>

/*  특정 시간동안 사용되지 않은 압축 리소스를
    Linked List의 Tail로부터 제거

    [IN] aMinimumResourceCount - 최소 리소스 갯수 
    [IN] aGarbageCollectionMicro -
            몇 Micro 초동안 사용되지 않은 경우 Garbage로 분류할지?
    [OUT] aCompRes - 압축 리소스 
 */
IDE_RC smrCompResList::removeGarbageFromTail(
           UInt          aMinimumResourceCount,
           ULong         aGarbageCollectionMicro,
           smrCompRes ** aCompRes )
{
    IDE_ASSERT( IDV_TIME_AVAILABLE() == ID_TRUE );

    IDE_DASSERT( aCompRes != NULL );
    
    UInt         sStage = 0;
    smuList    * sListNode = NULL;
    smrCompRes * sTailRes;
    ULong        sElapsedUS;
    
    idvTime sCurrTime;

    IDE_TEST( mListMutex.lock(NULL) != IDE_SUCCESS );
    sStage = 1;

    // 최소 유지 갯수보다 클 경우
    if ( mElemCount > aMinimumResourceCount )
    {

        IDE_ASSERT( ! SMU_LIST_IS_EMPTY( & mList ) );
        
        sListNode = SMU_LIST_GET_LAST( & mList );
        
        sTailRes = (smrCompRes*) sListNode->mData;
        
        // 현재 시각과 마지막 사용된 시각의 시간 차이 계산
        IDV_TIME_GET( & sCurrTime );
        sElapsedUS = IDV_TIME_DIFF_MICRO( & sTailRes->mLastUsedTime,
                                          & sCurrTime );
        
        // Garbage Collection할 시간이 된 경우 
        if ( sElapsedUS >= aGarbageCollectionMicro )
        {
            SMU_LIST_DELETE( sListNode );
            
            mElemCount --;
        }
        else
        {
            // Garbage Collection할 시간이 되지 않은 경우
            sListNode = NULL;
        }
    }
    
    sStage = 0;
    IDE_TEST( mListMutex.unlock() != IDE_SUCCESS );

    if ( sListNode == NULL )
    {
        // List가 비어있는 경우
        *aCompRes = NULL;
    }
    else
    {
        *aCompRes = (smrCompRes*)sListNode->mData ;

        // List에 Add할때부터 
        // 데이터의 포인터로 NULL을 허용하지 않았으므로,
        // Data의 포인터가 NULL일 수 없다.
        IDE_ASSERT( *aCompRes != NULL );
        
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
