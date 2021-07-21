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

#ifndef _O_SMR_LOG_FILE_MULTIPLEX_THREAD_H_
#define _O_SMR_LOG_FILE_MULTIPLEX_THREAD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <smDef.h>

#define SMR_LOG_MULTIPLEX_THREAD_TYPE_CNT   (4)

typedef enum smrLogMultiplexThreadState
{
    SMR_LOG_MPLEX_THREAD_STATE_WAKEUP,
    SMR_LOG_MPLEX_THREAD_STATE_WAIT,
    SMR_LOG_MPLEX_THREAD_STATE_ERROR
}smrLogMultiplexThreadState;

typedef enum smrLogMultiplexThreadType
{
    SMR_LOG_MPLEX_THREAD_TYPE_CREATE = 0,
    SMR_LOG_MPLEX_THREAD_TYPE_DELETE = 1,
    SMR_LOG_MPLEX_THREAD_TYPE_SYNC   = 2,
    SMR_LOG_MPLEX_THREAD_TYPE_APPEND = 3
}smrLogMultiplexThreadType;

typedef struct smrLogFileOpenList
{
    smrLogFile   mLogFileOpenListHdr;
    iduMutex     mLogFileOpenListMutex;
    UInt         mLogFileOpenListLen;
}smrLogFileOpenList;

class smrLogMultiplexThread : public idtBaseThread
{
//For Operation    
public:

    smrLogMultiplexThread();     
    virtual ~smrLogMultiplexThread();

    static IDE_RC initialize( smrLogMultiplexThread ** aSyncThread,
                              smrLogMultiplexThread ** aCreateThread,
                              smrLogMultiplexThread ** aDeleteThread,
                              smrLogMultiplexThread ** aAppendThread,
                              const SChar            * aOriginalLogPath );

    static IDE_RC wakeUpSyncThread( 
                        smrLogMultiplexThread  * aSyncThread,
                        smrSyncByWho             aWhoSync,
                        idBool                   aSyncLstPage,
                        UInt                     aFileNoToSync,
                        UInt                     aOffetToSync );

    static IDE_RC wakeUpCreateThread( 
                            smrLogMultiplexThread * aCreateThread,
                            UInt                  * aTargetFileNo,
                            UInt                    aLstFileNo,
                            SChar                 * aLogFileInitBuffer,
                            SInt                    aLogFileSize );

    static IDE_RC wakeUpDeleteThread( 
                            smrLogMultiplexThread * aDeleteThread,
                            UInt                    aDeleteFstFileNo,
                            UInt                    aDeleteLstFileNo,
                            idBool                  aIsCheckPoint );

    static IDE_RC destroy( smrLogMultiplexThread * aSyncThread,
                           smrLogMultiplexThread * aCreateThread,
                           smrLogMultiplexThread * aDeleteThread,
                           smrLogMultiplexThread * aAppendThread );

    static IDE_RC wait( smrLogMultiplexThread * aThread );

    static IDE_RC syncToDisk( smrLogMultiplexThread * aSyncThread,
                              UInt                    aOffset,
                              UInt                    aLogFileSize,
                              smrLogFile            * aOriginalLogFile );
    
    IDE_RC openLstLogFile( UInt          aLogFileNo,
                           UInt          aOffset,
                           smrLogFile  * aOriginalLogFile,
                           smrLogFile ** aLogFile );

    IDE_RC openLogFile( UInt          aLogFileNo,
                        idBool        aAddToList,
                        smrLogFile ** aLogFile,
                        idBool      * aIsExist );

    /* BUG-35051 서버 생성이후 바로 로그 다중화 프로퍼티를 설정하면 비정상
     * 종료합니다. */
    IDE_RC preOpenLogFile( UInt    aLogFileNo,
                           SChar * aLogFileInitBuffer,
                           UInt    aLogFileSize );

    IDE_RC recoverMultiplexLogFile( UInt aLstLogFileNo );

    inline void setCurLogFile( smrLogFile * aMultiplexLogFile,
                               smrLogFile * aOriginalLogFile )
    {
        IDE_ASSERT( mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_APPEND );
        IDE_ASSERT( aMultiplexLogFile != NULL );
        IDE_ASSERT( aOriginalLogFile  != NULL );

        mMultiplexLogFile = aMultiplexLogFile;
        mOriginalLogFile  = aOriginalLogFile;
    }

    static inline void setOriginalCurLogFileNo( UInt aOriginalCurLogFileNo )
    {
        mOriginalCurFileNo = aOriginalCurLogFileNo;
    }

    inline IDE_RC startThread();

private:

    IDE_RC checkMultiplexPath();

    IDE_RC appendLog( UInt * aSkipCnt );

    IDE_RC syncLog();

    IDE_RC createLogFile();

    IDE_RC deleteLogFile();

    IDE_RC shutdownAndDestroyThread();

    IDE_RC startAndInitializeThread( 
                        const SChar             * aMultiplexPath,
                        UInt                      aThreadIdx,
                        smrLogMultiplexThreadType aThreadType );
    
    IDE_RC add2LogFileOpenList( smrLogFile * aNewLogFile, idBool aWithLock );

    IDE_RC removeFromLogFileOpenList( smrLogFile * aRemoveLogFile );
    
    IDE_RC isCompleteLogFile( smrLogFile * aLogFile, 
                              idBool * aIsCompleteLogFile );

    IDE_RC recoverLogFile( smrLogFile * aLogFile );

    IDE_RC closeLogFile( smrLogFile * aLogFile, idBool aRemoveFromList );

    void checkLogFileOpenList( smrLogFileOpenList * aLogFileOpenList );

    void checkOriginalLogSwitch( idBool   * aIsNeedRestart );   /* BUG-38801 */

    IDE_RC wakeUp();

    smrLogFile * findLogFileInList( UInt                 aLogFileNo,
                                    smrLogFileOpenList * aLogFileOpenList);

    virtual void run();

    inline IDE_RC lock() 
    { return mMutex.lock(NULL); }

    inline IDE_RC unlock()
    { return mMutex.unlock(); }

    inline smrLogFileOpenList * getLogFileOpenList()
    {
        IDE_ASSERT( mMultiplexIdx < mMultiplexCnt ); 
        return &mLogFileOpenList[ mMultiplexIdx ]; 
    }

    inline smrLogFile * getOpenFileListHdr()
    {
        IDE_ASSERT( mMultiplexIdx < mMultiplexCnt ); 
        return &mLogFileOpenList[mMultiplexIdx].mLogFileOpenListHdr;
    }

    inline void wait4NextLogFile( smrLogFile * aCurLogFile );

    static IDE_RC initLogFileOpenList( smrLogFileOpenList * aLogFileOpenList,
                                       UInt                 aListIdx );

    static IDE_RC destroyLogFileOpenList( 
                            smrLogFileOpenList    * aLogFileOpenList,
                            smrLogMultiplexThread * aSyncThread );

//For Member
private:

    /* thread member variable */
    const SChar                       * mMultiplexPath;
    UInt                                mMultiplexIdx;
    smrLogMultiplexThreadType           mThreadType;
    volatile smrLogMultiplexThreadState mThreadState;
    iduMutex                            mMutex;
    iduCond                             mCv;
    smrLogFile                        * mMultiplexLogFile;
    smrLogFile                        * mOriginalLogFile;

    /* append log parameter */
    volatile static UInt               mOriginalCurFileNo;

    /* sync log parameter */
    static idBool                      mSyncLstPageInLstLF;
    static smrSyncByWho                mWhoSync;
    static UInt                        mFileNoToSync;
    static UInt                        mOffsetToSync;

    /* create logfile parameter */
    static SChar                     * mLogFileInitBuffer;
    static UInt                      * mTargetFileNo;
    static UInt                        mLstFileNo;
    static SInt                        mLogFileSize;

    /* delete logfile parameter */
    static idBool                      mIsCheckPoint;
    static UInt                        mDeleteFstFileNo;
    static UInt                        mDeleteLstFileNo;

    /* class member variable */
    static idBool                      mFinish;
    static smrLogFileOpenList        * mLogFileOpenList;
    static const SChar               * mOriginalLogPath;

public:
    static UInt                        mMultiplexCnt;
    static UInt                        mOriginalLstLogFileNo;
    static idBool                      mIsOriginalLogFileCreateDone;

};

inline void smrLogMultiplexThread::wait4NextLogFile( smrLogFile * aCurLogFile )
{
    PDL_Time_Value    sTV;
    UInt              sNxtLogFileNo;
    smrLogFile      * sOpenLogFileListHdr;

    IDE_ASSERT( aCurLogFile != NULL );

    sTV.set( 0, 1 ); 

    sOpenLogFileListHdr = getOpenFileListHdr();
    IDE_ASSERT( sOpenLogFileListHdr !=  aCurLogFile );

    sNxtLogFileNo = aCurLogFile->mFileNo + 1;

    while( sNxtLogFileNo != aCurLogFile->mNxtLogFile->mFileNo )
    {
        idlOS::sleep( sTV );

        if( aCurLogFile->mNxtLogFile != sOpenLogFileListHdr )
        {
            IDE_ASSERT( aCurLogFile->mNxtLogFile->mFileNo 
                        == sNxtLogFileNo );
            break;
        }
    }
}

inline IDE_RC smrLogMultiplexThread::startThread()
{
    IDE_TEST( start() != IDE_SUCCESS );

    IDE_TEST( waitToStart(0/*시작이 완료 될때까지 대기*/) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif
