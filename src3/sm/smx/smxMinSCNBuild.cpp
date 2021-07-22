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

# include <smErrorCode.h>
# include <smuProperty.h>
# include <smxTransMgr.h>
# include <smxMinSCNBuild.h>

smxMinSCNBuild::smxMinSCNBuild() : idtBaseThread()
{

}

smxMinSCNBuild::~smxMinSCNBuild()
{

}

ULong smxMinSCNBuild::mVersioningMinTime;

/**********************************************************************
 *
 * Description : Thread를 초기화한다.
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::initialize()
{
    /* PROJ-2733
       TIME-SCN 리스트를 최대 사이즈로 정한다. 

       - REBUILD_MIN_VIEWSCN_INTERVAL_이 1ms 로 잡는다면, 초당 1000개의 TIME-SCN을 기록하게 된다. 
       - VERSIONING_MIN_TIME의 최대값은 100sec 이다. 
       => 최대크기는 1,000 * 100 = 100,000 (+2 여유) 으로 정한다.
     */
    mTimeSCNListMaxCnt = ( 1000 /* 프로퍼티 REBUILD_MIN_VIEWSCN_INTERVAL_ 이 초당 1000개 */ 
                           * 100 /* 프로퍼티 VERSIONING_MIN_TIME 최대값 100 sec */
                           + 2 /* 여유로 2개를 더 준다. */ );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                 mTimeSCNListMaxCnt * ID_SIZEOF( smxTimeSCNNode ),
                                 (void**)&mTimeSCNList ) != IDE_SUCCESS );

    clearTimeSCNList();

    SM_SET_SCN_INFINITE( &mTimeSCN );
    SM_SET_SCN_INFINITE( &mAccessSCN );
    SM_SET_SCN_INFINITE( &mAgingMemViewSCN );
    SM_SET_SCN_INFINITE( &mAgingDskViewSCN );

    mVersioningMinTime  = smuProperty::getVersioningMinTime(); /* milli sec */

    mFinish = ID_FALSE;
    mResume = ID_FALSE;

    SM_INIT_SCN( &mSysMinDskViewSCN );

    // BUG-24885 wrong delayed stamping
    SM_INIT_SCN( &mSysMinDskFstViewSCN );

    // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
    SM_INIT_SCN( &mSysMinOldestFstViewSCN );

    IDE_TEST( mMutex.initialize((SChar*)"TRANSACTION_MINVIEWSCN_BUILDER",
                                IDU_MUTEX_KIND_POSIX,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST_RAISE(mCV.initialize((SChar*)"TRANSACTION_MINVIEWSCN_BUILDER")
                   != IDE_SUCCESS, err_cond_var_init);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_var_init );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondInit) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/**********************************************************************
 *
 * Description : Thread를 해제한다.
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::destroy()
{
    IDE_TEST_RAISE( mCV.destroy() != IDE_SUCCESS, err_cond_destroy );

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    IDE_ASSERT( iduMemMgr::free( mTimeSCNList )
                == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_destroy );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondDestroy) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 *
 * Description : Thread를 구동한다.
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::startThread()
{
    /* PROJ-2733
     * db의 startSCN 보다 큰 SCN 으로 접근을 제한한다. */
    if ( isActiveVersioningMinTime() == ID_TRUE )
    {
        setGlobalConsistentSCN();
    }

    IDE_TEST(start() != IDE_SUCCESS);

    IDE_TEST(waitToStart(0) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 *
 * Description : Thread를 종료한다.
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::shutdown()
{
    SInt          sState = 0;

    lock( NULL /* idvSQL* */);
    sState = 1;

    mFinish = ID_TRUE;

    IDE_TEST_RAISE(mCV.signal() != IDE_SUCCESS, err_cond_signal);

    sState = 0;
    unlock();

    IDE_TEST_RAISE(join() != IDE_SUCCESS, err_thr_join);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();

        unlock();

        IDE_POP();
    }

    return IDE_FAILURE;

}


/**********************************************************************
 *
 * Description : Thread의 Main Job을 수행한다.
 *
 * UPDATE_MIN_VIEWSCN_INTERVAL_ 프로퍼티에 지정한 시간마다 쓰레드는
 * Minimum Disk ViewSCN을 구하여 갱신한다.
 *
 **********************************************************************/
void smxMinSCNBuild::run()
{
    IDE_RC         sRC;
    SLong          sInterval;
    UInt           sState = 0;
    smSCN          sSysMinDskViewSCN;
    smSCN          sSysMinDskFstViewSCN;
    smSCN          sSysMinOldestFstViewSCN;
    PDL_Time_Value sCurrTimeValue;
    PDL_Time_Value sPropTimeValue;

restart :
    sState = 0;

    lock( NULL /* idvSQL* */);
    sState = 1;

    mResume = ID_TRUE;

    while( 1 )
    {
        // BUG-28819 REBUILD_MIN_VIEWSCN_INTERVAL_을 0으로 유지하고 restart하면
        // SysMinOldestFstViewSCN이 갱신되지 않고 초기값 대로 0이 유지되어서
        // 볼수 없는 page를 보게 되어 System이 비정상 종료합니다.
        // Inserval이 0이더라도 처음 한번은 각 값들을 구하도록 수정합니다.
        if( mResume == ID_TRUE )
        {
            if ( isActiveVersioningMinTime() == ID_TRUE )
            {
                /* PROJ-2733
                   SHARDING ENABLE시에만 TimeSCN, AccessSCN, AgingViewSCN 을 갱신한다. */
                setGlobalConsistentSCN();
            }

            // Time이 다 된 경우에는 다음 값들을 구하고
            // Inserval이 갱신 된 경우에는 구하지 않습니다.

            // BUG-24885 wrong delayed stamping
            // 모든 active 트랜잭션들의 minimum disk FstSCN을 구하도록 추가
            smxTransMgr::getDskSCNsofAll( &sSysMinDskViewSCN,
                                          &sSysMinDskFstViewSCN,    
                                          &sSysMinOldestFstViewSCN );  // BUG-26881

            // BUG-24885 wrong delayed stamping
            // set the minimum disk FstSCN
            SM_SET_SCN( &mSysMinDskViewSCN, &sSysMinDskViewSCN );
            SM_SET_SCN( &mSysMinDskFstViewSCN, &sSysMinDskFstViewSCN );

            // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
            SM_SET_SCN( &mSysMinOldestFstViewSCN, &sSysMinOldestFstViewSCN );
        }

        mResume   = ID_FALSE;
        sInterval = smuProperty::getRebuildMinViewSCNInterval();

        if ( sInterval != 0 )
        {
            sPropTimeValue.set( 0, sInterval * 1000 );

            sCurrTimeValue  = idlOS::gettimeofday();
            sCurrTimeValue += sPropTimeValue;

            sState = 0;
            sRC = mCV.timedwait(&mMutex, &sCurrTimeValue, IDU_IGNORE_TIMEDOUT);
            sState = 1;
        }
        else
        {
            ideLog::log( SM_TRC_LOG_LEVEL_TRANS,
                         SM_TRC_TRANS_DISABLE_MIN_VIEWSCN_BUILDER );

            sRC = mCV.wait(&mMutex);
            IDE_TEST_RAISE( sRC != IDE_SUCCESS, err_cond_wait );
        }

        if ( mFinish == ID_TRUE )
        {
            break;
        }

        IDE_TEST_RAISE( sRC != IDE_SUCCESS , err_cond_wait );

        if(mCV.isTimedOut() == ID_TRUE)
        {
            mResume = ID_TRUE;
        }
        else
        {
            /* do nothing */
        }
    }

    mResume = ID_FALSE;

    sState = 0;
    unlock();

    return;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();

        unlock();

        IDE_POP();
    }

    goto restart;
}

/**********************************************************************
 *
 * Description : ALTER SYSTEM 구문으로 사용자가 Disk Minimum View SCN을 갱신한다.
 *
 *               ALTER SYSTEM REBUILD MIN_VIEWSCN;
 *
 *               Rebuild 명령이므로 함수 완료 시점에는 Disk Min View SCN이
 *               구해져 있어야 한다. Thread 작업에 맏기지 않고 직접 구한다.
 *
 * aStatistics  - [IN] statistics
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::resumeAndWait( idvSQL  * aStatistics )
{
    SInt    sState = 0;
    smSCN   sSysMinDskViewSCN;
    smSCN   sSysMinDskFstViewSCN;
    smSCN   sSysMinOldestFstViewSCN;

    lock( aStatistics );
    sState = 1;

    if ( isActiveVersioningMinTime() == ID_TRUE )
    {
        /* PROJ-2733
           SHARDING ENABLE시에만 TimeSCN, AccessSCN, AgingViewSCN 을 갱신한다. */
        setGlobalConsistentSCN();
    }

    // BUG-24885 wrong delayed stamping
    // 모든 active 트랜잭션들의 minimum disk FstSCN을 구하도록 추가
    smxTransMgr::getDskSCNsofAll( &sSysMinDskViewSCN,
                                  &sSysMinDskFstViewSCN,        
                                  &sSysMinOldestFstViewSCN );  // BUG-26881

    // BUG-24885 wrong delayed stamping
    // set the minimum disk FstSCN
    SM_SET_SCN( &mSysMinDskViewSCN, &sSysMinDskViewSCN );
    SM_SET_SCN( &mSysMinDskFstViewSCN, &sSysMinDskFstViewSCN );

    IDU_FIT_POINT( "1.BUG-32650@smxMinSCNBuild::resumeAndWait" );

    // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
    SM_SET_SCN( &mSysMinOldestFstViewSCN, &sSysMinOldestFstViewSCN );

    IDE_TEST( clearInterval() != IDE_SUCCESS );

    sState = 0;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();

        unlock();

        IDE_POP();
    }

    return IDE_FAILURE;

}


/**********************************************************************
 *
 * Description : MinDskViewSCN 갱신주기를 변경한다.
 *
 * ALTER SYSTEM SET UPDATE_MIN_VIEWSCN_INTERVAL_ .. 구문으로
 * 사용자가 Minimum Disk View SCN을 갱신주기를 변경한다.
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::resetInterval()
{
    SInt   sState = 0;

    lock( NULL /* idvSQL* */ );
    sState = 1;

    IDE_TEST( clearInterval() != IDE_SUCCESS );

    sState = 0;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        unlock();
        IDE_POP();
    }

    return IDE_FAILURE;
}

/**********************************************************************
 *
 * PROJ-2733 분산 트랜잭션 정합성
 *
 * Description : versioning min time 을 변경한다.
 *
 * ALTER SYSTEM SET VERSIONING_MIN_TIME  구문으로
 * 사용자가 versioning min time을 변경한다.
 *
 * smuProperty::mVersioningMinTime 값이 변경된후, 이함수가 호출된다.
 * VERSIONING_MIN_TIME = 0 은 accessSCN, agingViewSCN을 더이상 구하지않고,
 * sytem (view) scn 을 사용한다는 의미이다.
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::resetVersioningMinTime()
{
    SInt   sState = 0;
    ULong sOldVersioningMinTime;
    ULong sNewVersioningMinTime;

    lock( NULL /* idvSQL* */ );
    sState = 1;

    if ( smuProperty::getShardEnable() == ID_FALSE )
    {
        IDE_CONT( SKIP_VERSIONING_MIN_TIME_RESET );
    }

    sOldVersioningMinTime = mVersioningMinTime;

    sNewVersioningMinTime = smuProperty::getVersioningMinTime(); /* milli sec */

    if ( ( sOldVersioningMinTime == 0 ) &&
         ( sNewVersioningMinTime > 0 ) )
    {
        /* 1. 작동 시작 */

        /* 이전 작동시 남아있을수있는 TIME-SCN LIST를 정리한다. */
        ID_SERIAL_BEGIN( clearTimeSCNList() );
        ID_SERIAL_END(   setGlobalConsistentSCN() );
    }
    else if ( ( sOldVersioningMinTime > 0 ) &&
              ( sNewVersioningMinTime == 0 ) )
    {
        /* 2.  작동 정지 */

        /* AccessSCN, AgingViewSCN 세팅 순서는 지켜져야 한다. */
        ID_SERIAL_BEGIN( SM_SET_SCN_INFINITE( &mTimeSCN ) );  
        ID_SERIAL_EXEC(  SM_SET_SCN_INFINITE( &mAccessSCN ), 1 ); 
        ID_SERIAL_EXEC(  SM_SET_SCN_INFINITE( &mAgingMemViewSCN ), 2 ); 
        ID_SERIAL_END(   SM_SET_SCN_INFINITE( &mAgingDskViewSCN ) ); 
    }

    mVersioningMinTime = sNewVersioningMinTime;

    IDE_EXCEPTION_CONT( SKIP_VERSIONING_MIN_TIME_RESET );

    IDE_TEST( clearInterval() != IDE_SUCCESS );

    sState = 0;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        unlock();
    }

    return IDE_FAILURE;
}

/* TIME-SCN LIST를 clear한다. */
void smxMinSCNBuild::clearTimeSCNList()
{
    SInt i;

    mTimeSCNListLastIdx = -1;
    mTimeSCNListBaseIdx = -1;

    for ( i = 0;
          i < mTimeSCNListMaxCnt;
          i++ )
    {
        mTimeSCNList[i].mTime.initialize();
        SM_INIT_SCN( &(mTimeSCNList[i].mSCN) );
    }
}

/**********************************************************************
 *
 * Description : 쓰레드의 현재 Interval을 초기화한다.
 *
 **********************************************************************/
IDE_RC smxMinSCNBuild::clearInterval()
{
    mResume = ID_FALSE;
    IDE_TEST_RAISE( mCV.signal() != IDE_SUCCESS, err_cond_signal );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_signal );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondSignal) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 PROJ-2733 분산 트랜잭션 정합성 :  TIME-SCN 리스트에 최신정보를 추가한다.

 TIME-SCN 리스트는 고정된 크기의 ARRAY로 index 0,1,2, ... 순서로 기록되며,
 모두 사용할 경우 index 0 부터 덮여쓰여진다.
 마지막에 기록한 index 위치는 mTimeSCNListLastIdx에 기록된다.
 **********************************************************************/
void smxMinSCNBuild::addTimeSCNList()
{
    SInt sLastIdx;

    sLastIdx = mTimeSCNListLastIdx;

    sLastIdx = ( (sLastIdx + 1) % mTimeSCNListMaxCnt );

    mTimeSCNList[sLastIdx].mTime = idlOS::gettimeofday();
    mTimeSCNList[sLastIdx].mSCN  = smmDatabase::getLstSystemSCN();

    mTimeSCNListLastIdx = sLastIdx;
}

/**********************************************************************
 PROJ-2733 분산 트랜잭션 정합성 : 현재시간을 바탕으로 TIME SCN을 구한다.

 현재시간에서 프로퍼티VERSIONING_MIN_TIME(디폴트 6초)를 뺀 시간을
 TIME-SCN 리스트에서 찾아 그것에 매칭되는 SCN을 TIME SCN으로 구한다.
 매칭되는 시간이 없다면 가장 가까운 옛시간으로 구한다.
 
 ex) TIME-SCN 리스트에 기록된 시간이
     1, 2, 3, 4, 5, 6, 7, 8 이고 현재시간이 8.5라면,

     8.5 - 6(프로퍼티 디폴트) = 2.5 
     2.5에 가장 가까운 옛시간 2가 선택된다.
 **********************************************************************/
void smxMinSCNBuild::setTimeSCN()
{
    SInt           sIdx;
    smSCN          sSCN;
#ifdef DEBUG
    smSCN          sOldTimeSCN;
#endif
    smSCN          sNewTimeSCN;
    PDL_Time_Value sOldTime;
    PDL_Time_Value sNewTime;
    PDL_Time_Value sTime;
    PDL_Time_Value sAddTime;
    PDL_Time_Value sCurTime;

    /* addTimeSCNList()이 호출되어,
       TIME-SCN LIST에 적어도 하나의 item이 있음을 보장한다. */
    addTimeSCNList();

    IDE_DASSERT( mTimeSCNListLastIdx >= 0 );

    sIdx     = mTimeSCNListLastIdx;
    sCurTime = idlOS::gettimeofday();

    sAddTime.msec( mVersioningMinTime ); /* milli sec */

    /* 가장 최근시간부터 역으로 검색한다. */
    while ( 1 )
    {
        sTime = mTimeSCNList[sIdx].mTime;
        sSCN  = mTimeSCNList[sIdx].mSCN;

        if ( SM_SCN_IS_INIT( sSCN ) )
        {
            /* 초기화된 값이면 직전것을 선택한다. */
            sIdx = ( (sIdx + 1) % mTimeSCNListMaxCnt );
            break;      
        }

        if ( ( sTime + sAddTime ) <= sCurTime )
        {
            /* found */
            break;
        }

        sIdx = ( ( sIdx == 0 )
                 ? ( mTimeSCNListMaxCnt - 1 ) : ( sIdx - 1 ) );

        if ( sIdx == mTimeSCNListLastIdx )
        {
            /* 검색이 끝났는데, 적절한 TIME SCN을 찾지못했다.
               LIST에서 가장 오래된 값으로 TIME SCN을 설정한다. */
            sIdx = ( (sIdx + 1) % mTimeSCNListMaxCnt );
            break;      
        }
    }

#ifdef DEBUG
    getTimeSCN( &sOldTimeSCN );
#endif
    sOldTime.initialize(); /* 0, 0 */
    if ( mTimeSCNListBaseIdx > 0 )
    {
        sOldTime = mTimeSCNList[mTimeSCNListBaseIdx].mTime;
    }

    sNewTimeSCN = mTimeSCNList[sIdx].mSCN;
    sNewTime = mTimeSCNList[sIdx].mTime;

    /* set */
    if ( mTimeSCNListBaseIdx != sIdx )
    {
        if ( sOldTime < sNewTime )
        {
#ifdef DEBUG
            if ( SM_SCN_IS_NOT_INFINITE( sOldTimeSCN ) )
            {
                IDE_ASSERT( SM_SCN_IS_LE( &sOldTimeSCN, &sNewTimeSCN ) );
            }
#endif
            SM_SET_SCN( &mTimeSCN, &sNewTimeSCN );
            mTimeSCNListBaseIdx = sIdx;
        }
    }
}

/* min view와 TIME SCN으로 AgingViewSCN을 구한다. */
void smxMinSCNBuild::setAgingMemViewSCN()
{
    smSCN sNewAgingMemViewSCN;
    smTID sDummyTID;

    /* 내부에서 TIME SCN과 비교한다. */
    smxTransMgr::getMinMemViewSCNofAll( &sNewAgingMemViewSCN,
                                        &sDummyTID,
                                        ID_TRUE /* TIME_SCN을 초기값으로 min view를 구한다. */ );

    /* AgingViewSCN 은 계속 증가한다 */
    if ( SM_SCN_IS_INFINITE( mAgingMemViewSCN ) ||
         SM_SCN_IS_LT( &mAgingMemViewSCN, &sNewAgingMemViewSCN ) )
    {
        SM_SET_SCN( &mAgingMemViewSCN, &sNewAgingMemViewSCN );
    }
}

/* dsk min view, TIME SCN 중 작은 값으로 AgingViewSCN을 구한다. 
   smxMinSCNBuild::run() 에서 호출된다. 
   AccessSCN 설정보다 늦게 설정되어야 한다. 
 */
void smxMinSCNBuild::setAgingDskViewSCN()
{
    smSCN sTimeSCN; 
    smSCN sSysMinDskViewSCN;
    smSCN sNewAgingDskViewSCN;


    getTimeSCN( &sTimeSCN );
    IDE_DASSERT( SM_SCN_IS_EQ( &sTimeSCN,&mAccessSCN ) );
    SM_SET_SCN_VIEW_BIT( &sTimeSCN );

    /* 내부에서 TIME SCN과 비교 안 했다 */
    /* BUGBUG - 현시점은 한 주기전 mSysMinDskFstViewSCN 이긴함. 
                aging 밀리면 여기도 봐야함 */
    getMinDskViewSCN( &sSysMinDskViewSCN );

    sNewAgingDskViewSCN = SM_SCN_IS_LT(&sTimeSCN,&sSysMinDskViewSCN) ? sTimeSCN : sSysMinDskViewSCN;

    IDE_DASSERT( SM_SCN_IS_NOT_INFINITE( sNewAgingDskViewSCN ) );

    /* AgingViewSCN 은 계속 증가한다 */
    if ( SM_SCN_IS_INFINITE( mAgingDskViewSCN ) ||
         SM_SCN_IS_LT( &mAgingDskViewSCN, &sNewAgingDskViewSCN ) )
    {
        SM_SET_SCN( &mAgingDskViewSCN, &sNewAgingDskViewSCN );
    }
}

/* TimeSCN으로 AccessSCN을 설정한다. */
void smxMinSCNBuild::setAccessSCN()
{
    smSCN sOldAccessSCN;
    smSCN sNewAccessSCN;

    getAccessSCN( &sOldAccessSCN );

    getTimeSCN( &sNewAccessSCN );

    /* AccessSCN 은 계속 증가한다 */
    if ( SM_SCN_IS_INFINITE( sOldAccessSCN ) ||
         SM_SCN_IS_LT( &sOldAccessSCN, &sNewAccessSCN ) )
    {
        SM_SET_SCN( &mAccessSCN, &sNewAccessSCN );
    }
}

/* TimeSCN/AccessSCN/AgingViewSCN 을 현재 system scn 으로 재설정한다.
   Replica SCN을 설정할때 사용된다.
   외부 인터페이스에서 호출된다. */
void smxMinSCNBuild::setGlobalConsistentSCNAsSystemSCN()
{
    lock( NULL );

    if ( isActiveVersioningMinTime() == ID_FALSE )
    {
        IDE_CONT( SKIP_ACCESS_SCN_SET );
    }

    ID_SERIAL_BEGIN( clearTimeSCNList() );
    ID_SERIAL_END(   setGlobalConsistentSCN() );

    IDE_EXCEPTION_CONT( SKIP_ACCESS_SCN_SET );

    unlock();
}

void smxMinSCNBuild::setGlobalConsistentSCN()
{
    /* AccessSCN과 AgingViewSCN이 역전되는 것을 막기위해 순서가 지켜져야 한다.

       AccessSCN이 먼저 증가하고, AgingViewSCN 이 증가해야한다.
       MUST BE ( AccessSCN >= AgingViewSCN )

       반례
     
       0. AgingViewSCN이 갱신됨.
       1. TX에 AgingViewSCN보다 작은 fstMinView가 세팅된다.
          ( 이후부터 AgingViewSCN에 영향 )
       2. AccessSCN과fstMinView 체크하지만, 통과
         => 문제발생: Aging이 되어버렸는데 작은fstMinView로 봐버림.
       3. AccessSCN갱신됨.

       위같이 발생하지않으려면 AccessSCN이 AgingViewSCN보다 먼저 갱신되어야 함.
     */
    ID_SERIAL_BEGIN( setTimeSCN() );  
    ID_SERIAL_EXEC(  setAccessSCN(), 1 ); 
    ID_SERIAL_EXEC(  setAgingMemViewSCN(), 2 ); 
    ID_SERIAL_END(   setAgingDskViewSCN() ); 
}

