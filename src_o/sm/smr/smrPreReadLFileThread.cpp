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
 * $Id: smrPreReadLFileThread.cpp 82426 2018-03-09 05:12:27Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smr.h>

#define SMR_PREREADLF_INFO_POOL_ELEMENT_COUNT (32)

smrPreReadLFileThread::smrPreReadLFileThread() : idtBaseThread()
{
}

smrPreReadLFileThread::~smrPreReadLFileThread()
{
}

/***********************************************************************
 * Description : 초기화를 수행한다.
 *
 */
IDE_RC smrPreReadLFileThread::initialize()
{
    mFinish = ID_FALSE;
    mResume = ID_FALSE;
    
    IDE_TEST_RAISE( mCV.initialize((SChar *)"PreReadLFileThread Cond")
                    != IDE_SUCCESS, err_cond_var_init);
    
    IDE_TEST( mMutex.initialize( (SChar*) "PreReadLFileThread Mutex",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST( mCondMutex.initialize( (SChar*) "PreReadLFileThread Cond Mutex",
                                     IDU_MUTEX_KIND_POSIX,
                                     IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    
    /* Open LogFile Request List를 초기화한다.*/
    mOpenLFRequestList.mPrev = &mOpenLFRequestList;
    mOpenLFRequestList.mNext = &mOpenLFRequestList;
    /* Open LogFile List를 초기화한다.*/
    mOpenLFList.mPrev = &mOpenLFList;
    mOpenLFList.mNext = &mOpenLFList;

    IDE_TEST( mPreReadLFInfoPool.initialize( IDU_MEM_SM_SMR,
                                             0,
                                             (SChar*) "SMR_PREREADLF_INFO_POOL",
                                             ID_SIZEOF(smrPreReadLFInfo),
                                             SMR_PREREADLF_INFO_POOL_ELEMENT_COUNT,
                                             10, /* 10개이상면 Free */
                                             ID_TRUE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : open된 Logfile을 close하고 할당된 Resource를 반환한다.
 *
 */
IDE_RC smrPreReadLFileThread::destroy()
{
    smrPreReadLFInfo *sCurPreReadInfo;
    smrPreReadLFInfo *sNxtPreReadInfo;

    /* open logfile request list에 모든 request를 취소하고
       삭제한다.*/
    sCurPreReadInfo = mOpenLFRequestList.mNext;
    
    while(sCurPreReadInfo != &mOpenLFRequestList)
    {
        sNxtPreReadInfo = sCurPreReadInfo->mNext;
        removeFromLFRequestList(sCurPreReadInfo);
        
        IDE_TEST( mPreReadLFInfoPool.memfree( (void*)sCurPreReadInfo )
                  != IDE_SUCCESS );
        
        sCurPreReadInfo = sNxtPreReadInfo;
        
    }

    /* open Log file list에서 file이 open되어 있으면 close하고
       할당된 메모리를 반납한다.*/
    sCurPreReadInfo = mOpenLFList.mNext;

    // 열려 있는 모든 LogFile을 Close한다.
    while ( sCurPreReadInfo != &mOpenLFList )
    {
        sNxtPreReadInfo = sCurPreReadInfo->mNext;
        removeFromLFList(sCurPreReadInfo);
        if ( ( sCurPreReadInfo->mFlag & SMR_PRE_READ_FILE_MASK ) 
             == SMR_PRE_READ_FILE_OPEN )
        {
            /* mFlag값에 SMR_PRE_READ_FILE_OPEN이 되어 있으면 logfile이
               Open되어 있는것이다.*/
            IDE_TEST( smrLogMgr::closeLogFile(sCurPreReadInfo->mLogFilePtr)
                      != IDE_SUCCESS );
        }

        IDE_TEST( mPreReadLFInfoPool.memfree((void*)sCurPreReadInfo)
                  != IDE_SUCCESS );
        
        sCurPreReadInfo = sNxtPreReadInfo;
    }
    
    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );
    IDE_TEST( mCondMutex.destroy() != IDE_SUCCESS );

    IDE_TEST( mPreReadLFInfoPool.destroy() != IDE_SUCCESS );

    IDE_TEST_RAISE(mCV.destroy() != IDE_SUCCESS, err_cond_destroy);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_destroy ); 
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : open logfile request를 계속 보고 있다고 request가 들어오면
 *               일단 request를 자신 open logfile list로 옮기고 open을 수행한다.
 *               이때 logfile open은 메모리에 모든 logfile을 읽어들이는 것이다.
 *               그리고 logfile의 open이 완료되면 request info에 mFlag에
 *               file의 open이 완료되어다는 것을 표시하기 위해
 *               SMR_PRE_READ_FILE_OPEN을 Setting한다. 그런데 mFlag값에
 *               SMR_PRE_READ_FILE_CLOSE이 되어 있다면 이는 open을 request한쪽에서
 *               PreReadThread가 Open하는 사이에 close를 요청한 것이다. 이땐 다시
 *               logfile을 close하고 할당된 resoruce를 반납한다.
 *
 *****************************************************************************/
void smrPreReadLFileThread::run()
{
    SInt                sState      = 0;
    smrPreReadLFInfo  * sCurPreReadInfo;
    time_t              sUntilTime;
    SInt                rc;

    while(mFinish == ID_FALSE)
    {
        if(sState == 0)
        {
            IDE_TEST( lockCond() != IDE_SUCCESS );
            sState = 1;
        }
        
        sUntilTime = idlOS::time(NULL) + 1;
        mTV.set(sUntilTime);

        sState = 0;
        rc = mCV.timedwait(&mCondMutex, &mTV);
        sState = 1;
        
        if ( rc != IDE_SUCCESS )
        {
            // 일반 UNIX SYSTEM CALL RETURN 규약: err == ETIME
            // PDL CALL RETURN 규약: err == -1 && errno == ETIME 
            IDE_TEST_RAISE(mCV.isTimedOut() != ID_TRUE, err_cond_wait);
            mResume = ID_TRUE;
        }
        else
        {
            if ( mResume == ID_FALSE ) 
            {
                // clear checkpoint interval의 경우이므로 time을 reset 한다.
                continue;
            }
        }

        if ( smuProperty::isRunLogPreReadThread() == SMU_THREAD_OFF )
        {
            // To Fix PR-14783
            // System Thread의 작업을 수행하지 않도록 한다.
            continue;
        }
        else
        {
            // Go Go 
        }
        
        sState = 0;
        IDE_TEST( unlockCond() != IDE_SUCCESS );
        
        sCurPreReadInfo = NULL;
            
        /* logfile open request list에서 reqest를 가져와서
           open logfile list에 추가한다. */
        IDE_TEST( getJobOfPreReadInfo(&sCurPreReadInfo)
                  != IDE_SUCCESS );
        
        if(sCurPreReadInfo != NULL)
        {
            rc = smrLogMgr::openLogFile( sCurPreReadInfo->mFileNo,
                                         ID_FALSE,
                                         &(sCurPreReadInfo->mLogFilePtr) );

            IDE_TEST( lock() != IDE_SUCCESS );
            sState = 2;

            if ( rc == IDE_FAILURE )
            {
                if ( ( errno == ENOENT ) ||
                     ( ideGetErrorCode() == smERR_ABORT_WaitLogFileOpen ) )
                {
                    sCurPreReadInfo->mLogFilePtr = NULL;
                    removeFromLFList(sCurPreReadInfo);
                    
                    IDE_TEST( mPreReadLFInfoPool.memfree( (void*)sCurPreReadInfo ) 
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_RAISE(error_file_open);
                }
            }
            else
            {
                
                if ( ( ( sCurPreReadInfo->mFlag & SMR_PRE_READ_FILE_MASK )
                       == SMR_PRE_READ_FILE_CLOSE ) || 
                     ( rc != IDE_SUCCESS ) )
                {
                    /* PreReadThread가 logfile을 open하고 있는사이에 다른쪽에서
                       close를 요청했다.*/
                    IDE_TEST( smrLogMgr::closeLogFile(sCurPreReadInfo->mLogFilePtr )
                              != IDE_SUCCESS);
                    
                    sCurPreReadInfo->mLogFilePtr = NULL;
                    removeFromLFList(sCurPreReadInfo);
                    
                    IDE_TEST( mPreReadLFInfoPool.memfree((void*)sCurPreReadInfo)
                             != IDE_SUCCESS );
                }
                else
                {
                    /* open logfile list에 있는 PreReadInfo의 mFlag값에
                       SMR_PRE_READ_FILE_OPEN을 setting한다.*/
                    sCurPreReadInfo->mFlag &= ~SMR_PRE_READ_FILE_MASK;
                    sCurPreReadInfo->mFlag |= SMR_PRE_READ_FILE_OPEN;
                }
            }
            
            sState = 0;
            IDE_TEST( unlock() != IDE_SUCCESS );
            
        }
    } /* While */

    if( sState == 1 )
    {
        sState = 0;
        IDE_TEST( unlockCond() != IDE_SUCCESS );
    }
    
    return;

    IDE_EXCEPTION(error_file_open);
    {
    }
    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch(sState)
    {
        case 2:
            IDE_ASSERT( unlock() == IDE_SUCCESS );
            break;
            
        case 1:
            IDE_ASSERT( unlockCond() == IDE_SUCCESS );
            break;

        default:
            break;
    }
    IDE_POP();

    IDE_CALLBACK_FATAL( "Pre Read Logfile Thread Fatal" );
    return;
}

/***********************************************************************
 * Description : PreReadThread에게 aFileNo에 해당하는 파일에
 *               대해서 open을 요청한다.
 *
 * aFileNo  - [IN] LogFile No
 */
IDE_RC smrPreReadLFileThread::addOpenLFRequest( UInt aFileNo )
{
    SInt              sState = 0;
    smrPreReadLFInfo *sCurPreReadInfo;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( mPreReadLFInfoPool.alloc((void**)&sCurPreReadInfo)
             != IDE_SUCCESS );

    initPreReadInfo(sCurPreReadInfo);
    
    sCurPreReadInfo->mFileNo = aFileNo;

    /* LogFile Open Request에 PreReadInfo를 추가한다.*/
    addToLFRequestList(sCurPreReadInfo);

    IDE_DASSERT(findInOpenLFRequestList( aFileNo ) ==
                sCurPreReadInfo);
    
    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    IDE_TEST( resume() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        (void)unlock();
        IDE_POP();
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : <aFileNo>에 해당하는 Logfile에 대해서 Close를 요청한다.
 *
 * aFileNo - [IN] LogFile No
 */
IDE_RC smrPreReadLFileThread::closeLogFile( UInt aFileNo )
{
    SInt              sState = 0;
    smrPreReadLFInfo *sCurPreReadInfo;
    
    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* Open LogFile List에 있는지 Check한다.*/
    sCurPreReadInfo = findInOpenLFList( aFileNo );

    if(sCurPreReadInfo == NULL)
    {
        sCurPreReadInfo = findInOpenLFRequestList( aFileNo );

        if(sCurPreReadInfo != NULL)
        {
            // 현재 Request List에만 있고 파일은 Open되지 않았기 때문에
            // List에서 제거만 한다.
            removeFromLFRequestList(sCurPreReadInfo);
            IDE_TEST( mPreReadLFInfoPool.memfree((void*)sCurPreReadInfo)
                      != IDE_SUCCESS );
        }
    }
    else
    {
        if ( ( sCurPreReadInfo->mFlag & SMR_PRE_READ_FILE_MASK )
               == SMR_PRE_READ_FILE_OPEN )
        {
            /* File이 Open되어 있다.*/
            IDE_TEST( smrLogMgr::closeLogFile( sCurPreReadInfo->mLogFilePtr )
                      != IDE_SUCCESS );

            sCurPreReadInfo->mLogFilePtr = NULL;

            removeFromLFList(sCurPreReadInfo);
            
            IDE_TEST( mPreReadLFInfoPool.memfree( (void*)sCurPreReadInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // 현재 PreRead Thread가 이 파일을 open하고 있다. 나중에 open이
            // 끝나고 나서 sCurPreReadInfo->mFlag의 값을 Check하여
            // SMR_PRE_READ_FILE_CLOSE이면 File을 Close하고 정리한다.
            IDE_ASSERT( (sCurPreReadInfo->mFlag & SMR_PRE_READ_FILE_MASK)
                         == SMR_PRE_READ_FILE_NON );
            sCurPreReadInfo->mFlag &= ~SMR_PRE_READ_FILE_MASK;
            sCurPreReadInfo->mFlag |= SMR_PRE_READ_FILE_CLOSE;
        }
    }

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            IDE_ASSERT( unlock() == IDE_SUCCESS);
        default:
            break;
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : open logfile Request list에서 aFileNo을 찾는다.
 *
 * aFileNo - [IN] LogFile No
 */
smrPreReadLFInfo* smrPreReadLFileThread::findInOpenLFRequestList( UInt aFileNo )
{
    smrPreReadLFInfo *sCurPreReadInfo;
        
    sCurPreReadInfo = mOpenLFRequestList.mNext;

    while(sCurPreReadInfo != &mOpenLFRequestList)
    {
        if ( sCurPreReadInfo->mFileNo == aFileNo )
        {
            break;
        }
        else
        {
            /* nothing to do ... */
        }

        sCurPreReadInfo = sCurPreReadInfo->mNext;
    }

    return sCurPreReadInfo == &mOpenLFRequestList ? NULL : sCurPreReadInfo;
}

/***********************************************************************
 * Description : open logfile list에서 aFileNo을 찾는다.
 *
 * aFileNo - [IN] LogFile No
 */
smrPreReadLFInfo* smrPreReadLFileThread::findInOpenLFList( UInt aFileNo )
{
    smrPreReadLFInfo *sCurPreReadInfo;

    sCurPreReadInfo = mOpenLFList.mNext;
    
    while(sCurPreReadInfo != &mOpenLFList)
    {
        if ( sCurPreReadInfo->mFileNo == aFileNo )
        {
            break;
        }
        else
        {
            /* nothing to do ... */
        }

        sCurPreReadInfo = sCurPreReadInfo->mNext;
    }

    return sCurPreReadInfo == &mOpenLFList ? NULL : sCurPreReadInfo;
}

/***********************************************************************
 * Description : open logfile Request list에 request가 있다면 이것이 lst log
 *               file no보다 작거나 같으면 이를 request list에서 open list로
 *               이동하고 이에대한 prereadinfo를 aPreReadInfo에 넣어준다.
 *               
 * aPreReadInfo - [OUT] Open해야될 PreReadInfo
 *
 */
IDE_RC smrPreReadLFileThread::getJobOfPreReadInfo(smrPreReadLFInfo **aPreReadInfo)
{
    SInt      sState = 0;
    smLSN     sLstLSN;
    smrPreReadLFInfo *sCurPreReadInfo;

    IDE_DASSERT( aPreReadInfo != NULL );

    *aPreReadInfo = NULL;
    SM_LSN_MAX( sLstLSN );

    if ( isEmptyOpenLFRequestList() == ID_FALSE )
    {
        smrLogMgr::getLstLSN( &sLstLSN ) ;
            
        IDE_TEST( lock() != IDE_SUCCESS );
        sState = 1;

        if ( isEmptyOpenLFRequestList() == ID_FALSE )
        {
            sCurPreReadInfo = mOpenLFRequestList.mNext;

            while ( sCurPreReadInfo != &mOpenLFRequestList )
            {
                // 실제로 로그가 기록된 로그파일인지 Check한다.
                if ( sCurPreReadInfo->mFileNo <= sLstLSN.mFileNo )
                {
                    removeFromLFRequestList( sCurPreReadInfo );
                    addToLFList( sCurPreReadInfo );

                    IDE_ASSERT( findInOpenLFList( sCurPreReadInfo->mFileNo )
                                == sCurPreReadInfo );

                    *aPreReadInfo = sCurPreReadInfo;
                    break;
                }
                    
                sCurPreReadInfo = sCurPreReadInfo->mNext;
            }
        }
        else
        {
            /* nothing do to */
        }

        sState = 0;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }
    else
    {
        /* nothing do to */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        (void)unlock();
        IDE_POP();
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : PreReadThread를 종료한다.
 *
 */
IDE_RC smrPreReadLFileThread::shutdown()
{
    mFinish = ID_TRUE;
    
    IDL_MEM_BARRIER;

    // 쓰레드가 완전히 종료될 때까지 기다린다.
    IDE_TEST_RAISE( join() != IDE_SUCCESS, err_thr_join );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));   
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : PreReadThread를 깨운다. 이미 작업중이면 무시된다.
 *
 */
IDE_RC smrPreReadLFileThread::resume()
{
    SInt sState = 0;
    
    IDE_TEST( lockCond() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST_RAISE(mCV.signal() != IDE_SUCCESS, err_cond_signal);

    sState = 0;
    IDE_TEST( unlockCond() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(unlockCond() == IDE_SUCCESS);
        IDE_POP();
    }
    
    return IDE_FAILURE;
    
}
