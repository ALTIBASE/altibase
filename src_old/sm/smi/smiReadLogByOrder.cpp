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
 * $Id: smiReadLogByOrder.cpp 85722 2019-06-25 06:18:05Z minku.kang $
 **********************************************************************/

#include <smDef.h>
#include <smr.h>
#include <smi.h>

/*
  이 Class는 Parallel Logging환경에서 Replication Sender가 로그를 읽어서
  보낼 경우 Log Header의 Sequence Number순으로 로그를 보내기 위해서 작성된
  Class이다. smiReadInfo를 유지하여 보내어야 할 로그의
  물리적, 논리적 위치정보를 구축하고 Read가 호출될때 마다
  smiReadInfo를 분석하여 보내어야할 로그를 선택한다. 선택하는 기준은
  다음과 같다.

      - 무조건 로그를 읽으면 된다. 왜냐하면 물리적인 위치와 Log Header의 Sequence
        Number의 순서가 동일하기 때문이다.
*/

/***********************************************************************
 * Description : aInitLSN보다 크거나 같은 LSN값을 가진 로그중
 *               가장 작은 LSN값을 가진 log의 위치를 찾는다. 만약없으면
 *               endLSN으로 reading position을 setting한다.
 *               
 *
 * aInitSN         - [IN] 첫번째 읽어야할 로그의 SN
 * aPreOpenFileCnt - [IN] PreRead할 로그파일의 갯수
 * aNotDecomp      - [IN] 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부 (압축된 형태로 반환)
 */
IDE_RC smiReadLogByOrder::initialize( smSN     aInitSN,
                                      UInt     aPreOpenFileCnt,
                                      idBool   aIsRemoteLog,
                                      ULong    aLogFileSize,
                                      SChar ** aLogDirPath,
                                      idBool   aNotDecomp )
{
    UInt   j;
    UInt   sFileNo;
    idBool sIsRemoteLogMgrInit = ID_FALSE;
    smLSN  sInitLSN;

    mNotDecomp = aNotDecomp;

    IDE_DASSERT( (aInitSN != SM_SN_NULL) && (aInitSN < SM_SN_MAX) );

    SM_MAKE_LSN( sInitLSN, aInitSN );

    /* PROJ-1915 : ID_FALSE :local 로그, ID_TRUE : Remote Log */
    mIsRemoteLog    = aIsRemoteLog;

    if ( mIsRemoteLog == ID_TRUE )
    {
        /* PROJ-1915 : off-line 로그 처리를 위한 smrRemoteLogMgr초기화 */
        IDE_TEST( mRemoteLogMgr.initialize( aLogFileSize, 
                                            aLogDirPath,
                                            mNotDecomp )
                  != IDE_SUCCESS );
        sIsRemoteLogMgrInit = ID_TRUE;
    }

    mCurReadInfoPtr = NULL;
    mPreReadFileCnt = aPreOpenFileCnt;
    SM_LSN_MAX( mLstReadLogLSN );    

    IDE_TEST( initializeReadInfo( & mReadInfo ) != IDE_SUCCESS );
    
    IDE_TEST( mPQueueRedoInfo.initialize( IDU_MEM_SM_SMR,
                                          1,                       /* aDataMaxCnt*/
                                          ID_SIZEOF(smiReadInfo*), /* aDataSize */
                                          compare )
             != IDE_SUCCESS );

    /* 첫번째 읽어야 될 로그파일의 위치를 찾는다.*/
    IDE_TEST( setFirstReadLogFile( sInitLSN ) != IDE_SUCCESS );

    /* 첫번째 읽어야 될 로그레코드의 위치를 찾는다.*/
    IDE_TEST( setFirstReadLogPos( sInitLSN ) != IDE_SUCCESS );

    /* mPreReadFileCnt > 0일때만 PreRead Thread를 뛰운다.*/
    if ( mPreReadFileCnt != 0 )
    {
        IDE_TEST( mPreReadLFThread.initialize() != IDE_SUCCESS );

        sFileNo = mReadInfo.mReadLSN.mFileNo + 1;
        /* mReadInfo[i].mReadLSN.mFileNo가 가리키는 파일은
           이미 Open되어있다.*/
        for ( j = 0 ; j < mPreReadFileCnt ; j++ )
        {
            IDE_TEST( mPreReadLFThread.addOpenLFRequest( sFileNo )
                      != IDE_SUCCESS );
            sFileNo++;
        }

        /* PreRead LogFile Thread를 Start시킨다. */
        IDE_TEST( mPreReadLFThread.start() != IDE_SUCCESS );
        IDE_TEST( mPreReadLFThread.waitToStart(0) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsRemoteLogMgrInit == ID_TRUE )
    {
        IDE_PUSH();
        (void)mRemoteLogMgr.destroy();
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smiReadLogByOrder를 해제한다.
 *
 * 할당된 Resource를 해제한다.
 */
IDE_RC smiReadLogByOrder::destroy()
{
    
    IDE_TEST( mPQueueRedoInfo.destroy()  != IDE_SUCCESS );

    IDE_TEST( destroyReadInfo( & mReadInfo ) != IDE_SUCCESS );

    if ( mPreReadFileCnt != 0 )
    {
        IDE_TEST( mPreReadLFThread.shutdown() != IDE_SUCCESS );
        IDE_TEST( mPreReadLFThread.destroy() != IDE_SUCCESS );
    }

    /* PROJ-1915 */
    if ( mIsRemoteLog == ID_TRUE )
    {
        IDE_TEST( mRemoteLogMgr.destroy() != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Read Info를 초기화 한다.

    [IN] aReadInfo - 초기화할 Read Info
 */
IDE_RC smiReadLogByOrder::initializeReadInfo( smiReadInfo * aReadInfo )
{
    IDE_DASSERT( aReadInfo != NULL );
    
    idlOS::memset( aReadInfo, 0, ID_SIZEOF( *aReadInfo ) );
    
    aReadInfo->mLogFilePtr = NULL;

    // 로그 압축해제 버퍼 핸들의 초기화
    IDE_TEST( aReadInfo->mDecompBufferHandle.initialize( IDU_MEM_SM_SMR )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Read Info를 파괴한다.
    
    [IN] aReadInfo - 파괴할 Read Info
 */
IDE_RC smiReadLogByOrder::destroyReadInfo( smiReadInfo * aReadInfo )
{
    IDE_DASSERT( aReadInfo != NULL );

    // 로그 압축해제 버퍼 핸들의 파괴
    IDE_TEST( aReadInfo->mDecompBufferHandle.destroy()
              != IDE_SUCCESS );
    
    if ( aReadInfo->mLogFilePtr != NULL )
    {
        /* PROJ-1915 */
        if ( mIsRemoteLog == ID_FALSE )
        {
            IDE_TEST( smrLogMgr::closeLogFile(aReadInfo->mLogFilePtr)
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( mRemoteLogMgr.closeLogFile(aReadInfo->mLogFilePtr)
                      != IDE_SUCCESS );
        }
        aReadInfo->mLogFilePtr = NULL;        
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : iduPriorityQueue에서 Item들을 Compare할 때 사용하는
 *               Callback Function임. iduPriorityQueue의 initialize할때 넘겨짐
 *
 * arg1  - [IN] compare할 smiReadInfo 1
 * arg2  - [IN] compare할 smiReadInfo 2
*/
SInt smiReadLogByOrder::compare(const void *arg1,const void *arg2)
{
    smLSN sLSN1;
    smLSN sLSN2;

    IDE_DASSERT( arg1 != NULL );
    IDE_DASSERT( arg2 != NULL );
        
    SM_GET_LSN( sLSN1, ((*(smiReadInfo**)arg1))->mReadLSN );
    SM_GET_LSN( sLSN2, ((*(smiReadInfo**)arg2))->mReadLSN );

    if ( smrCompareLSN::isGT( &sLSN1, &sLSN2 ) )
    {
        return 1;
    }
    else
    {
        if ( smrCompareLSN::isLT( &sLSN1, &sLSN2 ) )
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}

/***********************************************************************
 * Description : 마지막으로 읽은 로그 다음에 기록된 Log를 읽어서 준다.

  mSortRedoInfo에 들어있는 smiReadInfo중에서 가장 작은 mLSN값을
  가진 Log를 읽어들인다.
  
  aSN           - [OUT] Log의 SN
  aLSN          - [OUT] Log의 LSN
  aLogHeadPtr   - [OUT] aLSN이 가리키는 log의 Log Header Ptr
  aLogPtr       - [OUT] aLSN이 가리키는 log의 Log Buffr Ptr
  aIsValid  - [OUT] aLSN이 가리키는 log가 Valid하면 ID_TRUE아니면 ID_FALSE
 **********************************************************************/
IDE_RC smiReadLogByOrder::readLog( smSN    * aSN,
                                   smLSN   * aLSN,
                                   void   ** aLogHeadPtr,
                                   void   ** aLogPtr,
                                   idBool  * aIsValid )
{
    idBool    sIsExistLogTail = ID_FALSE;
    idBool    sUnderflow      = ID_FALSE;
#ifdef DEBUG 
    smLSN     sReadLSN;
#endif

    IDE_DASSERT( aLSN             != NULL );
    IDE_DASSERT( aLogPtr          != NULL );
    IDE_DASSERT( aIsValid         != NULL );
    
    *aIsValid = ID_FALSE;

    if ( mCurReadInfoPtr != NULL )
    {
        if ( mCurReadInfoPtr->mIsValid == ID_TRUE )
        {

            if ( ( ( mCurReadInfoPtr->mLogHead.mFlag ) & SMR_LOG_COMPRESSED_MASK )
                 == SMR_LOG_COMPRESSED_OK )
            {
                // Comp 로그면 FILE END가 없다.
                mCurReadInfoPtr->mReadLSN.mOffset += mCurReadInfoPtr->mLogSizeAtDisk;
            } 
            else
            {
                /* 마지막으로 Redo연산을 한 smiReadInfo의 mReadLSN은
                   smrRecoveryMgr의 Redo에서 update가 된다. 따라서 다시
                   mReadLSN이 가리키는 로그를 읽어서 smiReadInfo를 갱신하고
                   다시 mSortArrRedoInfo에 넣어야 한다. */
                if ( smrLogHeadI::getType( &mCurReadInfoPtr->mLogHead )
                     == SMR_LT_FILE_END )
                {
                    mCurReadInfoPtr->mReadLSN.mFileNo++;
                    mCurReadInfoPtr->mReadLSN.mOffset = 0;
                    mCurReadInfoPtr->mIsLogSwitch = ID_TRUE;
                }
                else
                {
                    mCurReadInfoPtr->mReadLSN.mOffset += mCurReadInfoPtr->mLogSizeAtDisk;
                }
            }
            mCurReadInfoPtr->mIsValid = ID_FALSE;
        }
    }

    /* 로그를 읽어서 Valid한 로그를 찾는다 */
    IDE_TEST( searchValidLog( &sIsExistLogTail ) != IDE_SUCCESS );

    mPQueueRedoInfo.dequeue( (void*)&mCurReadInfoPtr, &sUnderflow );

    if ( sUnderflow == ID_FALSE )
    {
        /* BUG-26717 CodeSonar Null Pointer Dereference
         * sUnderflow가 ID_FALSE인 경우 mCurReadInfoPtr는 Null이 아님 */
        IDE_ASSERT( mCurReadInfoPtr != NULL );

        *aIsValid = mCurReadInfoPtr->mIsValid;

        /* 읽어야할 로그가 없는경우 */
        if ( sIsExistLogTail == ID_TRUE )
        {
            IDE_DASSERT( mCurReadInfoPtr->mIsValid == ID_TRUE );
            
            /* Read전에 첫번째 read시 sIsExistLogTail이 True경우는
               없다. 따라서 mLstReadLogLSN은 SM_SN_NULL이 될수 없다. */
            IDE_ASSERT( !SM_IS_LSN_MAX( mLstReadLogLSN ) );           

            /* 
               LSN번호순으로 보내는 것을 보장되기 되기 때문에 마지막으로 보낸
               로그의 LSN값과 현재 로그의 LSN값을 비교해서 보내지
               않은 로그가 있는지 조사한다.이를 위해 마지막으로 보내 로그의
               LSN값과 현재 로그의 LSN값이 작은지 조사하여
               이미 보낸 LSN보다 작은 LSN을 가지는 로그가 발견되면 중간에 빠진 LSN값을 가지고
               로그를 기록하는 Transaction이 있는 것으로 판단한다.
               발견된 중간에 빠진 LSN은 queue에 다시 넣지 않았으므로 replication 에서 제외될듯. 
               에러를 올려서 Sender가 다시 retry 할지 판단하게 한다.  
            */
#ifdef DEBUG
            sReadLSN = smrLogHeadI::getLSN( &mCurReadInfoPtr->mLogHead ); 
            IDE_DASSERT( smrCompareLSN::isEQ( &mCurReadInfoPtr->mReadLSN, &sReadLSN ) );
#endif
            //[TASK-6757]LFG,SN 제거
            if ( smrCompareLSN::isLT( &mCurReadInfoPtr->mReadLSN, &mLstReadLogLSN ) )
            {
                ideLog::log( IDE_ERR_0,
                             "Read Invalid Log during Read Log By Order. \n"
                             "CurRedo LSN = %"ID_UINT32_FMT",%"ID_UINT32_FMT"\n"
                             "Last Apply  LSN = %"ID_UINT32_FMT",%"ID_UINT32_FMT"\n",
                             mCurReadInfoPtr->mReadLSN.mFileNo,
                             mCurReadInfoPtr->mReadLSN.mOffset,
                             mLstReadLogLSN.mFileNo,
                             mLstReadLogLSN.mOffset );

                *aIsValid = ID_FALSE;
                mCurReadInfoPtr = NULL;

                IDE_ERROR_RAISE(0, error_read_invalid_log );
            }
            else
            {
                /* nothing to do */
            }
        }
        
        if ( *aIsValid == ID_TRUE )
        {
            /* 마지막으로 읽은 Log의 LSN값을 저장한다.*/
#ifdef DEBUG
            sReadLSN = smrLogHeadI::getLSN( &mCurReadInfoPtr->mLogHead ); 
            IDE_DASSERT( smrCompareLSN::isEQ( &mCurReadInfoPtr->mReadLSN, &sReadLSN ) );
#endif
            mLstReadLogLSN = mCurReadInfoPtr->mReadLSN;
            *aSN           = SM_MAKE_SN( mCurReadInfoPtr->mReadLSN );

            *aLogPtr       = mCurReadInfoPtr->mLogPtr;
            *aLogHeadPtr   = &(mCurReadInfoPtr->mLogHead);

            *aLSN = mCurReadInfoPtr->mReadLSN;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_read_invalid_log )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL_ARG,
                                  "[Repl] read log by order" ));
    }
    IDE_EXCEPTION_END;

    *aLogPtr          = NULL;
    *aLogHeadPtr      = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aInitLSN보다 큰 SN중 가장 작은 SN값을
 *               가진 로그파일을 찾는다.
 * [TASK-6757]LFG,SN 제거  BUGBUG : LSN을 알고 있다.-> 간단히 찾을수 있다. 
 *
 * aInitLSN    - [IN] 첫번째 읽어야 할 로그의 LSN값
 ***********************************************************************/
IDE_RC smiReadLogByOrder::setFirstReadLogFile( smLSN aInitLSN )
{
    smLSN       sEndLSN;
    UInt        sFstChkFileNo  = 0;
    UInt        sEndChkFileNo  = 0;
    UInt        sFstReadFileNo = 0;
    UInt        sState      = 0;
    idBool      sIsFound    = ID_FALSE;
    idBool      sIsValid    = ID_FALSE;
    SChar       sErrMsg[512] = { 0, };
#ifdef DEBUG
    smLSN       sReadLSN;
#endif
    /* LSN을 지정하여 Replication을 시작 할 경우 
     * ex) ALTER REPLICATION ALA1 START AT SN(0)
     * LSN 이 INIT 을  가질수 있어서 ASSERT 로 검사하면 안된다. */
    IDE_DASSERT( !SM_IS_LSN_MAX( aInitLSN ) );
 
    // BUG-27725
    SM_LSN_INIT( sEndLSN );
 
    /* File을 Check하는 도중 Check포인트가 발생하여 Log File이 지워질
       수 있다. 이를 방지하기 위해서 Lock을 수행한다.*/
    if ( mIsRemoteLog == ID_FALSE )
    {
        IDE_TEST( smrRecoveryMgr::lockDeleteLogFileMtx()
                  != IDE_SUCCESS );
        sState = 1;

        smrRecoveryMgr::getLstDeleteLogFileNo( &sFstChkFileNo );
    }
    else
    {
        mRemoteLogMgr.getFirstFileNo( &sFstChkFileNo );
    }
    
    if ( mIsRemoteLog == ID_FALSE )
    {
        smrLogMgr::getLstLSN( &sEndLSN );
    }
    else
    {
        mRemoteLogMgr.getLstLSN( &sEndLSN );
    }
    
    sEndChkFileNo = sEndLSN.mFileNo;

    if ( mIsRemoteLog == ID_FALSE )
    {
        (void)smrLogMgr::getFirstNeedLFN( aInitLSN,
                                          sFstChkFileNo,
                                          sEndChkFileNo,
                                          &sFstReadFileNo );
    }
    else
    {
        (void)mRemoteLogMgr.getFirstNeedLFN( aInitLSN,
                                             sFstChkFileNo,
                                             sEndChkFileNo,
                                             &sFstReadFileNo );
    }

    if ( mIsRemoteLog == ID_FALSE )
    {
        sState = 0;
        IDE_TEST( smrRecoveryMgr::unlockDeleteLogFileMtx()
                  != IDE_SUCCESS );
    }

    /*
       sENDLSN의 mFileNo보다 작은 로그 파일들에
       대해서 aInitLSN보다 큰 LSN중 가장 작은 LSN을 가진 로그파일을 찾는다.
    */
    SM_SET_LSN( mReadInfo.mReadLSN,
                sFstReadFileNo,
                0 );
   
    mReadInfo.mLogFilePtr  = NULL;
    mReadInfo.mIsValid     = ID_FALSE;
    SM_LSN_INIT( mReadInfo.mLstLogLSN );
    mReadInfo.mIsLogSwitch = ID_FALSE;

    /* mReadLSN이 가리키는 Log를 읽어서 mReadInfo에 삽입한다. */
    if ( mIsRemoteLog == ID_FALSE )
    {
        IDE_TEST( smrLogMgr::readFirstLogHead( &(mReadInfo.mReadLSN),
                                               &(mReadInfo.mLogHead) )
                  != IDE_SUCCESS );
        /* MagicNumber로 최소의 valid  검사를 한다. */
        sIsValid = smrLogFile::isValidMagicNumber( &(mReadInfo.mReadLSN), 
                                                   &(mReadInfo.mLogHead) );
    }
    else
    {
        /*
         *  Offline replicator 에서 사용되면 Log 파일의 유효성을 확인한다.
         */
        IDE_TEST( mRemoteLogMgr.readFirstLogHead( &(mReadInfo.mReadLSN),
                                                  &(mReadInfo.mLogHead),
                                                  &sIsValid )
                  !=IDE_SUCCESS );
    }

    if ( sIsValid == ID_TRUE )
    {
#ifdef DEBUG
        sReadLSN = smrLogHeadI::getLSN( &mReadInfo.mLogHead ); 
        IDE_DASSERT( smrCompareLSN::isEQ( &mReadInfo.mReadLSN, &sReadLSN ) );
#endif

        /* MagicNumber로 최소의 valid 검사만 해서 넘긴다. 
         * offset 읽을때 또 valid 검사를 한다.  */
    
        mReadInfo.mIsValid = ID_TRUE;
        SM_GET_LSN( mReadInfo.mLstLogLSN, mReadInfo.mReadLSN );

        // BUG-29115
        // 적어도 하나는 aInitLSN을 포함해야 한다.
        if ( smrCompareLSN::isLTE( &mReadInfo.mLstLogLSN, &aInitLSN ) )
        {
            sIsFound = ID_TRUE;
        }
        else
        {
            /* nothing to do ... */
        }
    }
    else
    {
        /* do nothing */
    }

    IDU_FIT_POINT_RAISE( "smiReadLogByOrder::setFirstReadLogFile:sIsFound",
                         ERR_NOT_FOUND_LOG );
    IDE_TEST_RAISE( sIsFound == ID_FALSE, ERR_NOT_FOUND_LOG );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_LOG);
    {
        idlOS::snprintf( sErrMsg, 512,
                         "sFstChkFileNo : %"ID_UINT32_FMT
                         ", sEndChkFileNo : %"ID_UINT32_FMT
                         ", sFstReadFileNo : %"ID_UINT32_FMT,
                         sFstChkFileNo,
                         sEndChkFileNo,
                         sFstReadFileNo );
        IDE_SET(ideSetErrorCode( smERR_ABORT_NotFoundLog, 
                                 aInitLSN.mFileNo,
                                 aInitLSN.mOffset,
                                 sErrMsg ));
    }
    IDE_EXCEPTION_END;
        
    if ( sState != 0 )
    {
        IDE_PUSH();
        (void)smrRecoveryMgr::unlockDeleteLogFileMtx();
        IDE_POP();
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aInitLSN보다 큰 SN중 가장 작은 SN값을
 *               가진 로그파일에서 실제 로그의 위치를 찾는다.
 * [TASK-6757]LFG,SN 제거  BUGBUG : LSN을 알고 있다.-> 간단히 찾을수 있다. 
 *
 * [BUG-44571] aInitLSN이 파일의 마지막 로그(SMR_LT_FILE_END)이거나
 *             그보다 큰 OFFSET을 갖는 경우 로그를 찾지못한다.
 *             이경우, 다음파일의 첫로그(offset:0)를 리턴하도록 한다. 
 *
 * aInitLSN    - [IN] 첫번째 읽어야 할 로그의 LSN값
 **********************************************************************/
IDE_RC smiReadLogByOrder::setFirstReadLogPos( smLSN aInitLSN )
{
    void      * sQueueData;
    idBool      sOverflow   = ID_FALSE;
    smLSN       sLstWriteLSN;
    SChar       sErrMsg[512] = { 0, };
    smLSN       sEndLSN;
#ifdef DEBUG
    smLSN       sReadLSN;
#endif

    if ( mIsRemoteLog == ID_FALSE )
    {
        smrLogMgr::getLstLSN( &sEndLSN );
    }
    else
    {
        mRemoteLogMgr.getLstLSN( &sEndLSN );
    }

    /* LSN을 지정하여 Replication을 시작 할 경우 
     * ex) ALTER REPLICATION ALA1 START AT SN(0)
     * LSN 이 INIT을 가질수 있어서 ASSERT 로 검사하면 안된다. */
    IDE_DASSERT( !SM_IS_LSN_MAX( aInitLSN ) );
 
    /* mReadInfo의 정해진 로그파일내에서
     * aInitLSN보다 크거나 같은 LSN을 가진 로그중에서 가장 작은
     * LSN을 가진 로그의 위치를 찾는다. */
    while ( mReadInfo.mIsValid == ID_TRUE )
    {
        /* 현재 읽는 로그의 LSN값이 aInitLSN값보다 첫번째로 크거나 같다면
         * 이로그가 aInitLSN과 가장 가까운 값이다.왜냐하면 로그는 순서대로
         * 기록되기때문이다. */

#ifdef DEBUG
        sReadLSN = smrLogHeadI::getLSN( &mReadInfo.mLogHead ); 
        IDE_DASSERT( smrCompareLSN::isEQ( &mReadInfo.mReadLSN, &sReadLSN ) );
#endif
        if ( smrCompareLSN::isGTE( &mReadInfo.mReadLSN, &aInitLSN ) ) 
        {
            /* BUG-21726 */
            if ( mReadInfo.mLogPtr == NULL )
            {
                /* mReadLSN이 가리키는 Log를 읽어서 mReadInfo에 삽입한다.*/
                if ( mIsRemoteLog == ID_FALSE )
                {
                    if ( mNotDecomp == ID_FALSE )
                    {
                        /* 압축된 로그파일의 경우 압축을 풀어서 레코드를 읽어온다. */
                        IDE_TEST( smrLogMgr::readLog(
                                                &(mReadInfo.mDecompBufferHandle ),
                                                &(mReadInfo.mReadLSN),
                                                ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                                &(mReadInfo.mLogFilePtr),
                                                &(mReadInfo.mLogHead),
                                                &(mReadInfo.mLogPtr),
                                                &(mReadInfo.mIsValid),
                                                &(mReadInfo.mLogSizeAtDisk) )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* 압축된 로그파일의 경우 압축을 풀지않고 반환한다. */
                        IDE_TEST( smrLogMgr::readLog4RP(
                                                &(mReadInfo.mReadLSN),
                                                ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                                &(mReadInfo.mLogFilePtr),
                                                &(mReadInfo.mLogHead),
                                                &(mReadInfo.mLogPtr),
                                                &(mReadInfo.mIsValid),
                                                &(mReadInfo.mLogSizeAtDisk) )
                                  != IDE_SUCCESS );
                    }
                }
                else
                {
                    IDE_TEST( mRemoteLogMgr.readLogAndValid(
                                            &(mReadInfo.mDecompBufferHandle ),
                                            &(mReadInfo.mReadLSN),
                                            ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                            &(mReadInfo.mLogFilePtr),
                                            &(mReadInfo.mLogHead),
                                            &(mReadInfo.mLogPtr),
                                            &(mReadInfo.mIsValid),
                                            &(mReadInfo.mLogSizeAtDisk) )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* nothing to do ... */
            }

            sQueueData = (void *)&(mReadInfo);
            mPQueueRedoInfo.enqueue((void*)&sQueueData, &sOverflow);
            IDE_ASSERT( sOverflow == ID_FALSE );

            break;
        }
        else
        {
            /* nothing to do ... */
        }

        IDU_FIT_POINT_RAISE( "smiReadLogByOrder::setFirstReadLogPos:smrLogHeadI::getType::mReadLSN.mFileNo",
                             ERR_NOT_FOUND_LOG );

        if ( ( smrLogHeadI::getFlag( &mReadInfo.mLogHead ) & SMR_LOG_COMPRESSED_MASK )
               == SMR_LOG_COMPRESSED_OK )
        {
            /* 다음 로그를 읽기 위해 */
            mReadInfo.mReadLSN.mOffset += mReadInfo.mLogSizeAtDisk;
        }
        else
        {
            if ( smrLogHeadI::getType( &mReadInfo.mLogHead ) == SMR_LT_FILE_END )
            {
                if ( mReadInfo.mReadLSN.mFileNo < sEndLSN.mFileNo )
                {
                    /* BUG-44571
                     * 발견된 로그가 파일의 마지막 로그(SMR_LT_FILE_END)인경우 
                     * 다음파일의 첫로그(offset:0)를 읽는다. */
                    SM_SET_LSN( mReadInfo.mReadLSN,
                                mReadInfo.mReadLSN.mFileNo + 1, /* next file no. */
                                0 );
                }
                else
                {
                    /* 다음 로그파일이 없다. */
                    IDE_RAISE( ERR_NOT_FOUND_LOG );
                }
            }
            else
            {
                /* 다음 로그를 읽기 위해 */
                mReadInfo.mReadLSN.mOffset += mReadInfo.mLogSizeAtDisk;
            }
        }
        
        /* mReadLSN이 가리키는 Log를 읽어서 mReadInfo에 삽입한다.*/
        if ( mIsRemoteLog == ID_FALSE )
        {
            if ( mNotDecomp == ID_FALSE )
            {
                /* 압축된 로그파일의 경우 압축을 풀어서 레코드를 읽어온다. */
                IDE_TEST( smrLogMgr::readLog( &(mReadInfo.mDecompBufferHandle ),
                                              &(mReadInfo.mReadLSN),
                                              ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                              &(mReadInfo.mLogFilePtr),
                                              &(mReadInfo.mLogHead),
                                              &(mReadInfo.mLogPtr),
                                              &(mReadInfo.mIsValid),
                                              &(mReadInfo.mLogSizeAtDisk) )
                          != IDE_SUCCESS );
            }
            else
            {
                /* 압축된 로그파일의 경우 압축을 풀지않고 반환한다. */
                IDE_TEST( smrLogMgr::readLog4RP( &(mReadInfo.mReadLSN),
                                                 ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                                 &(mReadInfo.mLogFilePtr),
                                                 &(mReadInfo.mLogHead),
                                                 &(mReadInfo.mLogPtr),
                                                 &(mReadInfo.mIsValid),
                                                 &(mReadInfo.mLogSizeAtDisk) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( mRemoteLogMgr.readLogAndValid(
                                        &(mReadInfo.mDecompBufferHandle ),
                                        &(mReadInfo.mReadLSN),
                                        ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                        &(mReadInfo.mLogFilePtr),
                                        &(mReadInfo.mLogHead),
                                        &(mReadInfo.mLogPtr),
                                        &(mReadInfo.mIsValid),
                                        &(mReadInfo.mLogSizeAtDisk) )
                    != IDE_SUCCESS );

            /* BUG-26768 */
            /* initLSN보다 큰 SN으로 기록된 로그가 없는 경우
             * mIsValid를 ID_TRUE로 하고 나온다. */
            mRemoteLogMgr.getLstLSN( &sLstWriteLSN );
            if ( ( mReadInfo.mIsValid == ID_FALSE ) &&
                 ( smrCompareLSN::isLT( &sLstWriteLSN, &aInitLSN ) ) )           
            {
                mReadInfo.mIsValid = ID_TRUE;
                break;
            }
            else
            {
                /* noghint to do ... */
            }
        }
    } // end of while
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_LOG);
    {
        idlOS::snprintf( sErrMsg, 512,
                         "Last read LogLSN : %"ID_UINT32_FMT",%"ID_UINT32_FMT,
                         mReadInfo.mLstLogLSN.mFileNo, mReadInfo.mLstLogLSN.mOffset );
        IDE_SET(ideSetErrorCode( smERR_ABORT_NotFoundLog, 
                                 aInitLSN.mFileNo,
                                 aInitLSN.mOffset,
                                 sErrMsg ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 새로운 읽을 로그가
 *               기록되었는지 체크하고 있다면 읽어들인다.
 *
 * aIsExistLogTail - [OUT] mReadInfo가 가리키는
 *                        로그가 Invalid한 로그라면
 *                        ID_TRUE, 아니면 ID_FALSE.
 *
 */
IDE_RC smiReadLogByOrder::searchValidLog( idBool *aIsExistLogTail )
{
    UInt    sPreReadFileCnt = mPreReadFileCnt;
    void   *sQueueData;
    idBool  sOverflow  = ID_FALSE;
    smLSN   sLastLogLSN;
    smLSN   sReadLSN;

    IDE_DASSERT( aIsExistLogTail != NULL );

    *aIsExistLogTail = ID_TRUE;

    //읽을 로그가 없었다면 다시 새로운 로그가 기록되었는지
    //체크해야한다.
    if ( mReadInfo.mIsValid == ID_FALSE )
    {
        if ( mIsRemoteLog == ID_FALSE )
        {
            /* BUG-42739
             * getlstWriteSN 이 아닌 smiGetValidLSN을 이용해서
             * Dummy를 포함하지 않는 last Used LSN 을 받아와야 한다. */
            IDE_TEST( smiGetLastValidLSN( &sLastLogLSN ) != IDE_SUCCESS );

            if ( !(smrCompareLSN::isEQ( &sLastLogLSN, &mReadInfo.mLstLogLSN ) ) )
            {
                if ( mNotDecomp == ID_FALSE )
                {
                    /*mReadLSN이 가리키는 Log를 읽어서 mReadInfo에 삽입한다.*/
                    IDE_TEST( smrLogMgr::readLog(
                                             &(mReadInfo.mDecompBufferHandle ),
                                             &(mReadInfo.mReadLSN),
                                             ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                             &(mReadInfo.mLogFilePtr),
                                             &(mReadInfo.mLogHead),
                                             &(mReadInfo.mLogPtr),
                                             &(mReadInfo.mIsValid),
                                             &(mReadInfo.mLogSizeAtDisk) )
                             != IDE_SUCCESS );
                }
                else
                {
                    /* mReadLSN이 가리키는 Log를 읽어서 mReadInfo에 삽입한다
                       압축된 로그파일의 경우 압축을 풀지않고 반환한다. */
                    IDE_TEST( smrLogMgr::readLog4RP(
                                             &(mReadInfo.mReadLSN),
                                             ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                             &(mReadInfo.mLogFilePtr),
                                             &(mReadInfo.mLogHead),
                                             &(mReadInfo.mLogPtr),
                                             &(mReadInfo.mIsValid),
                                             &(mReadInfo.mLogSizeAtDisk) )
                             != IDE_SUCCESS );
                }

            /* BUG-37931 
             ------------------------------------------------     /  --------------------
             LSN    | 100 | 101 | 102   | 103 | 104   | 105 | .. /_ .. | 110 | 111  | 112
             Status | ok  | ok  | dummy | ok  | dummy | ok  |     /    | ok  | dummy| ok
             ------------ A ------------------------- B ----     /   - C ---------- D ---
                                                                server restart
             case 1) service 상태
                     dummy 로그를 포함한 최대 LSN (B)
                     마지막으로 valid 한 로그를 저장한 시점 (A)
                     101( A ) 까지만 읽어야 하는데 102를 읽었다면 retry

             case 2) server restart
                     restart 후 dummy 로그를 포함한 최대 LSN (D)
                     restart 후 valid 한 로그를 저장한 시점 (C)

                     dummy만 남기고 죽은 녀석이 있다면..
                     getLstWriteLSN이 (C) 까지 증가했을것임
                     rp에서 처리한다. */

#ifdef DEBUG 
                sReadLSN = smrLogHeadI::getLSN( &mReadInfo.mLogHead ); 
                IDE_DASSERT( smrCompareLSN::isEQ( &mReadInfo.mReadLSN, &sReadLSN ) );
#endif
                if ( smrCompareLSN::isLT( &sLastLogLSN, &mReadInfo.mReadLSN ) == ID_TRUE )
                {
                    mReadInfo.mIsValid     = ID_FALSE;
                    mReadInfo.mIsLogSwitch = ID_FALSE;
                }
                else
                {
                    /* nothing to do ... */
                }

                if ( ( mReadInfo.mIsLogSwitch == ID_TRUE ) &&
                     ( sPreReadFileCnt != 0 ) )
                {
                    IDE_TEST( mPreReadLFThread.closeLogFile( mReadInfo.mReadLSN.mFileNo )
                              != IDE_SUCCESS );

                    /* Prefetch Thread를 깨워서 미리 로그파일을 읽어들인다.*/
                    IDE_TEST( mPreReadLFThread.addOpenLFRequest(
                                                         mReadInfo.mReadLSN.mFileNo +
                                                         sPreReadFileCnt )
                              != IDE_SUCCESS );
                    mReadInfo.mIsLogSwitch = ID_FALSE;
                }
                else
                {
                    /* nothing to do ... */
                }

                if ( mReadInfo.mIsValid == ID_TRUE )
                {
                    IDE_ASSERT( sOverflow == ID_FALSE );

                    sReadLSN = smrLogHeadI::getLSN( &mReadInfo.mLogHead ); 
#ifdef DEBUG 
                    IDE_ASSERT( smrCompareLSN::isEQ( &mReadInfo.mReadLSN, &sReadLSN ) );
#endif
                    SM_GET_LSN( mReadInfo.mLstLogLSN, sReadLSN );
                    sQueueData = (void*)(&mReadInfo); //BUG-21726

                    mPQueueRedoInfo.enqueue( (void*)&sQueueData, &sOverflow );
                    *aIsExistLogTail = ID_FALSE;
                }
                else
                {
                    /* nothing to do ... */
                }
            }
            else
            {

                /* 새로운 로그가 없다 */ 
                /* nothing to do ... */
                 

            }
        }
        else //remote log일 경우
        {
            mRemoteLogMgr.getLstLSN( &sLastLogLSN ); 
            if ( !(smrCompareLSN::isEQ( &sLastLogLSN, &mReadInfo.mLstLogLSN ) ) )
            {
                /*mReadLSN이 가리키는 Log를 읽어서 mArrReadInfo에 삽입한다.*/
                IDE_TEST( mRemoteLogMgr.readLogAndValid(
                                         &(mReadInfo.mDecompBufferHandle ),
                                         &(mReadInfo.mReadLSN),
                                         ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                         &(mReadInfo.mLogFilePtr),
                                         &(mReadInfo.mLogHead),
                                         &(mReadInfo.mLogPtr),
                                         &(mReadInfo.mIsValid),
                                         &(mReadInfo.mLogSizeAtDisk) )
                          != IDE_SUCCESS );
            
                if ( ( mReadInfo.mIsLogSwitch == ID_TRUE ) &&
                     ( sPreReadFileCnt != 0 ) )
                {
                    IDE_TEST( mPreReadLFThread.closeLogFile( mReadInfo.mReadLSN.mFileNo )
                              != IDE_SUCCESS );

                    /* Prefetch Thread를 깨워서 미리 로그파일을 읽어들인다.*/
                    IDE_TEST( mPreReadLFThread.addOpenLFRequest(
                                                     mReadInfo.mReadLSN.mFileNo +
                                                     sPreReadFileCnt)
                              != IDE_SUCCESS );
                    mReadInfo.mIsLogSwitch = ID_FALSE;
                }                    
                else
                {
                    /* nothing to do ... */
                }
                
                if ( mReadInfo.mIsValid == ID_TRUE )
                {
                    IDE_ASSERT( sOverflow == ID_FALSE );

#ifdef DEBUG
                    sReadLSN = smrLogHeadI::getLSN( &mReadInfo.mLogHead ); 
                    IDE_DASSERT( smrCompareLSN::isEQ( &mReadInfo.mReadLSN, &sReadLSN ) );
#endif
                    SM_GET_LSN( mReadInfo.mLstLogLSN, mReadInfo.mReadLSN );
                    sQueueData = (void*)(&mReadInfo); //BUG-21726
            
                    mPQueueRedoInfo.enqueue( (void*)&sQueueData, &sOverflow );
                    *aIsExistLogTail = ID_FALSE;
                }
                else
                {
                    /* nothing to do ... */
                }
            }
            else
            {
                /* nothing to do ... */
            }
        }
    }
    else
    {
        *aIsExistLogTail = ID_FALSE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 마지막으로 읽은시점까지 읽었던 로그가 모두 sync되었는지
 *               Check 한다.
 *
 * aIsSynced - [OUT] 모두 Sync가 되면 ID_TRUE, 아니면 ID_FALSE
 ***********************************************************************/
IDE_RC smiReadLogByOrder::isAllReadLogSynced( idBool *aIsSynced )
{
    smLSN sSyncedLSN;
    smLSN sSyncLSN;
    
    IDE_DASSERT( aIsSynced != NULL );

    *aIsSynced = ID_TRUE;

    IDE_TEST( smrLogMgr::getLFThread().getSyncedLSN( &sSyncedLSN )
              != IDE_SUCCESS );

    sSyncLSN = mReadInfo.mReadLSN;

    if ( mCurReadInfoPtr == &mReadInfo )
    {
        if ( mCurReadInfoPtr->mIsValid == ID_TRUE )
        {
            /* BUGBUG: Sender가 보낸 로그이기때문에 */
            sSyncLSN.mOffset +=  mCurReadInfoPtr->mLogSizeAtDisk;
        }
        else
        {
            /* nothing to do ... */
        }
    }
    else
    {
        /* nothing to do ... */
    }

    if ( smrCompareLSN::isLT( &(sSyncedLSN),
                              &(sSyncLSN) )
         == ID_TRUE )
    {
        *aIsSynced = ID_FALSE;
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : 마지막으로 읽은시점LSN을 받아서 다음 로그부터
 *               읽을 수 있도록 초기화 한다.
 *               PROJ-1670 replication log buffer에서 로그를 읽을 때
 *               이 클래스가 아닌 rpdLogBufferMgr 클래스를 통해
 *               sender가 로그를 읽을 수 있게되었다.
 *              그러나, rpdLogBufferMgr에는 모든 로그가 존재하지 않기 때문에
 *               bufferMgr에 없는 로그인 경우 이 클래스를 통해 읽어야 한다.
 *               그래서 마지막으로 읽은 LSN을 통해 읽을 수 있는
 *               함수를 추가하게 되었다.
 * aLstReadLSN - [IN] 마지막으로 읽은 LSN
 ***********************************************************************/
IDE_RC smiReadLogByOrder::startByLSN( smLSN aLstReadLSN )
{
    UInt  sFileNo    = 0;
    UInt  i          = 0;
#ifdef DEBUG
    smLSN sReadLSN;
#endif

    SM_GET_LSN( mReadInfo.mReadLSN, aLstReadLSN );
    mReadInfo.mLogFilePtr  = NULL;
    mReadInfo.mIsValid     = ID_TRUE;
    SM_LSN_MAX( mReadInfo.mLstLogLSN );
    mReadInfo.mIsLogSwitch = ID_FALSE;

    if ( mNotDecomp == ID_FALSE )
    {
        /* 가장 마지막으로 읽은 로그의 LSN(mReadLSN)에
         * 해당하는 파일을 열어 로그를 읽는다.
         * 읽어야 하는 로그는 다음 로그이다.*/
        IDE_TEST( smrLogMgr::readLog( &(mReadInfo.mDecompBufferHandle ),
                                      &(mReadInfo.mReadLSN),
                                      ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                      &(mReadInfo.mLogFilePtr),
                                      &(mReadInfo.mLogHead),
                                      &(mReadInfo.mLogPtr),
                                      &(mReadInfo.mIsValid),
                                      &(mReadInfo.mLogSizeAtDisk) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* 가장 마지막으로 읽은 로그의 LSN(mReadLSN)에
         * 해당하는 파일을 열어 로그를 읽는다.
         * 읽어야 하는 로그는 다음 로그이다.
         * 압축된 로그파일의 경우 압축을 풀지않고 반환한다. */

        IDE_TEST( smrLogMgr::readLog4RP( &(mReadInfo.mReadLSN),
                                         ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                         &(mReadInfo.mLogFilePtr),
                                         &(mReadInfo.mLogHead),
                                         &(mReadInfo.mLogPtr),
                                         &(mReadInfo.mIsValid),
                                         &(mReadInfo.mLogSizeAtDisk) )
                  != IDE_SUCCESS );
    }

#ifdef DEBUG
    sReadLSN = smrLogHeadI::getLSN( &mReadInfo.mLogHead ); 
    IDE_DASSERT( smrCompareLSN::isEQ( &mReadInfo.mReadLSN, &sReadLSN ) );
#endif
    SM_GET_LSN( mReadInfo.mLstLogLSN, mReadInfo.mReadLSN );    
    SM_GET_LSN( mLstReadLogLSN, mReadInfo.mReadLSN );
    mCurReadInfoPtr = &mReadInfo;

    sFileNo = mReadInfo.mReadLSN.mFileNo + 1;

    if ( mPreReadFileCnt != 0 )
    {
        for ( i = 0 ; i < mPreReadFileCnt ; i++ )
        {
            IDE_TEST( mPreReadLFThread.addOpenLFRequest( sFileNo )
                      != IDE_SUCCESS );
            sFileNo++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : 모든 열린 로그파일을 close 한다.
 *               sender가 로그파일 읽는 것을 중단할 때 사용한다.
 *               PROJ-1670 replication log buffer-- startByLSN참고
 ***********************************************************************/
IDE_RC smiReadLogByOrder::stop()
{
    UInt    i           = 0;
    UInt    sFileNo;
    void  * sQueueData;
    idBool  sUnderflow  = ID_FALSE;

    /* PROJ-1670 replication log buffer를 사용할 때만 호출되어야 한다. */
    mPQueueRedoInfo.dequeue( (void*)&sQueueData, &sUnderflow );

    if ( mReadInfo.mLogFilePtr != NULL )
    {
        IDE_TEST( smrLogMgr::closeLogFile( mReadInfo.mLogFilePtr )
                  != IDE_SUCCESS );
        mReadInfo.mLogFilePtr = NULL;
    }
    else
    {
        /* nothing to do ... */
    }

    if ( mPreReadFileCnt != 0 )
    {
        sFileNo = mReadInfo.mReadLSN.mFileNo + 1;

        for ( i = 0 ; i < mPreReadFileCnt ; i++ )
        {
            IDE_TEST( mPreReadLFThread.closeLogFile( sFileNo )
                      != IDE_SUCCESS );
            sFileNo++;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Read 정보를 제공한다.
 *
 * aReadLSN    - [OUT] Read LSN을 저장할 메모리
 */
void smiReadLogByOrder::getReadLSN( smLSN     * aReadLSN )
{
    IDE_DASSERT( aReadLSN != NULL );
    SM_GET_LSN( *aReadLSN, mReadInfo.mReadLSN );
    return;
}

/**********************************************************************
 * PROJ-1915 
 * Description : 오프라인 로그에서 마지막으로 Log를 기록하기 위해 사용된 SN값을 리턴한다.
 *
 * aSN  - [OUT] output parameter
 **********************************************************************/
IDE_RC smiReadLogByOrder::getRemoteLastUsedGSN( smSN * aSN )
{
    smSN  sRetSN = 0;
    smLSN sTmpLSN;

    IDE_ASSERT( mIsRemoteLog == ID_TRUE ); //remote log여야 한다.

    mRemoteLogMgr.getLstLSN( &sTmpLSN );
    if ( !SM_IS_LSN_INIT( sTmpLSN ) )
    {
        sRetSN = SM_MAKE_SN( sTmpLSN );
    }
    else
    {
        /* nothing to do ... */
    }
    *aSN = sRetSN;

    return IDE_SUCCESS;
}

idBool smiReadLogByOrder::isCompressedLog( SChar * aRawLog )
{
    return smrLogComp::isCompressedLog( aRawLog );
}

smOID smiReadLogByOrder::getTableOID( SChar * aRawLog )
{
    return smrLogComp::getTableOID( aRawLog );
}

IDE_RC smiReadLogByOrder::decompressLog( SChar      * aCompLog,
                                         smLSN      * aReadLSN,
                                         smiLogHdr  * aLogHead,
                                         SChar     ** aLogPtr )
{
    UInt sDummySize = 0;

    IDE_TEST( smrLogComp::decompressCompLog( &( mReadInfo.mDecompBufferHandle ),
                                             aReadLSN->mOffset,
                                             aCompLog,
                                             smrLogFile::makeMagicNumber( aReadLSN->mFileNo,
                                                                          aReadLSN->mOffset ),
                                             aLogHead,
                                             aLogPtr,
                                             &sDummySize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
