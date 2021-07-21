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

#include <ide.h>
#include <sdd.h>
#include <smErrorCode.h>
#include <smu.h>
#include <smrDef.h>
#include <sdbDPathBufferMgr.h>
#include <sdbDPathBFThread.h>

sdbDPathBFThread::sdbDPathBFThread() : idtBaseThread()
{
}

sdbDPathBFThread::~sdbDPathBFThread()
{
}

/*******************************************************************************
 * Description : DPathBCBInfo 초기화
 *
 * aDPathBCBInfo - [IN] 초기화 할 DPathBuffInfo
 ******************************************************************************/
IDE_RC sdbDPathBFThread::initialize( sdbDPathBuffInfo  * aDPathBCBInfo )
{
    IDE_TEST( sdbDPathBufferMgr::initBulkIOInfo(&mDPathBulkIOInfo)
              != IDE_SUCCESS );

    mFinish       = ID_FALSE;
    mDPathBCBInfo = aDPathBCBInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : 할당된 Resource를 Free 시킨다.
 ******************************************************************************/
IDE_RC sdbDPathBFThread::destroy()
{
    IDE_TEST( sdbDPathBufferMgr::destBulkIOInfo(&mDPathBulkIOInfo)
              != IDE_SUCCESS );

    mDPathBCBInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Flush Thread를 시작시킨다.
 **********************************************************************/
IDE_RC sdbDPathBFThread::startThread()
{
    IDE_TEST(start() != IDE_SUCCESS);
    IDE_TEST(waitToStart(0) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : mDPathBCBInfo가 가리키는 Flush Request List에 대해서
 *               Flush작업을 수행한다. 깨어나는 주기는
 *               __DIRECT_BUFFER_FLUSH_THREAD_SYNC_INTERVAL이다.
 **********************************************************************/
void sdbDPathBFThread::run()
{
    scSpaceID      sNeedSyncSpaceID = 0;
    PDL_Time_Value sTV;

    sTV.set(0, smuProperty::getDPathBuffFThreadSyncInterval() );

    while(mFinish == ID_FALSE)
    {
        idlOS::sleep( sTV );

        if( mFinish == ID_TRUE )
        {
            break;
        }

        /* Flush Request List에 대해서 Flush작업을 요청한다. */
        IDE_TEST( sdbDPathBufferMgr::flushBCBInList(
                      NULL, /* idvSQL* */
                      mDPathBCBInfo,
                      &mDPathBulkIOInfo,
                      &(mDPathBCBInfo->mFReqPgList),
                      &sNeedSyncSpaceID )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdbDPathBufferMgr::writePagesByBulkIO(
                  NULL, /* idvSQL* */
                  mDPathBCBInfo,
                  &mDPathBulkIOInfo,
                  &sNeedSyncSpaceID )
              != IDE_SUCCESS );

    if( sNeedSyncSpaceID != 0 )
    {
        /* 마지막 writePage호출시 write가 수행된 TableSpace에 대해
         * Sync를 수행한다. */
        IDE_TEST( sddDiskMgr::syncTBSInNormal(
                      NULL, /* idvSQL* */
                      sNeedSyncSpaceID ) != IDE_SUCCESS );
    }

    return;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL( "Error In Direct Path Flush Thread" );
}

/***********************************************************************
 * Description : Flush Thread가 종료시키고 종료될때까지 기다린다.
 **********************************************************************/
IDE_RC sdbDPathBFThread::shutdown()
{
    mFinish = ID_TRUE;

    IDE_TEST_RAISE(join() != IDE_SUCCESS,
                   err_thr_join);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
