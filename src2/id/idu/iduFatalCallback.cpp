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
 * $Id:
 **********************************************************************/

#include <idu.h>
#include <iduFatalCallback.h>

iduMutex            iduFatalCallback::mMutex;
iduCallbackFunction iduFatalCallback::mCallbackFunction[IDU_FATAL_INFO_CALLBACK_ARR_SIZE] = { NULL, };
idBool              iduFatalCallback::mInit = ID_FALSE;


/***********************************************************************
 * Description : Fatal Info 초기화
 *
 * Fatal 시 SM 관련 정보 출력 callback 초기화 및
 * idu callback에 doCallback 등록
 *
 **********************************************************************/
IDE_RC iduFatalCallback::initializeStatic()
{
    UInt i;

    IDE_TEST( mMutex.initialize( (SChar*) "Fatal Info Callback Mutex",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    for( i = 0 ; i < IDU_FATAL_INFO_CALLBACK_ARR_SIZE ; i++ )
    {
        mCallbackFunction[i] = NULL;
    }

    mInit = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mInit = ID_FALSE;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Fatal Info 정리 및 ID의 callback에서 제거
 *
 **********************************************************************/
IDE_RC iduFatalCallback::destroyStatic()
{
    mInit = ID_FALSE;

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Fatal Info 함수 추가
 *
 *
 **********************************************************************/
void iduFatalCallback::setCallback( iduCallbackFunction aCallbackFunction )
{
    UInt i;

    if ( mInit == ID_TRUE )
    {
        lock();

        for( i = 0 ; i < IDU_FATAL_INFO_CALLBACK_ARR_SIZE  ; i++ )
        {
            if( mCallbackFunction[i] == NULL )
            {
                mCallbackFunction[i] = aCallbackFunction;
                break;
            }
        }

        unlock();

        // 빈자리를 발견하지 못한경우
        IDE_DASSERT( i < IDU_FATAL_INFO_CALLBACK_ARR_SIZE );
    }
}

/***********************************************************************
 * Description : Fatal Info 함수 제거
 *
 *
 **********************************************************************/
void iduFatalCallback::unsetCallback( iduCallbackFunction aCallbackFunction )
{
    UInt i;

    if ( mInit == ID_TRUE )
    {
        lock();

        for ( i = 0 ; i < IDU_FATAL_INFO_CALLBACK_ARR_SIZE ; i++ )
        {
            if ( mCallbackFunction[i] == aCallbackFunction )
            {
                mCallbackFunction[i] = NULL;
                break;
            }
        }

        unlock();
    }
}

/***********************************************************************
 * Description : 등록된 Fatal Info 함수 실행 - 디버깅 정보 출력
 *
 *
 **********************************************************************/
void iduFatalCallback::doCallback()
{
    UInt   i;
    iduCallbackFunction sCallback;

    if ( mInit == ID_TRUE )
    {
        ideLog::log( IDE_DUMP_0,
                     "\n\n"
                     "*--------------------------------------------------------*\n"
                     "*           Execute module callback functions            *\n"
                     "*--------------------------------------------------------*\n" );

        for ( i = 0 ; i < IDU_FATAL_INFO_CALLBACK_ARR_SIZE ; i++ )
        {
            sCallback = mCallbackFunction[i];
            mCallbackFunction[i] = NULL;

            if ( sCallback != NULL )
            {
                sCallback();
                ideLog::log( IDE_DUMP_0,
                             "========================================================\n");
            }
        }
    }
}
