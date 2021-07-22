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

/*******************************************************************************
 * $Id: rpdXLogfileMgr.h
 ******************************************************************************/

#ifndef _O_RPD_XLOGFILEMGR_H_
#define _O_RPD_XLOGFILEMGR_H_ 1

#include <idtBaseThread.h>
#include <rpdXLogfileFlusher.h>
#include <rpdXLogfileCreater.h>
#include <sdi.h>
#include <idp.h>
#include <idu.h>
#include <smi.h>
#include <qci.h>
#include <rpDef.h>

typedef struct rpdXLogfile
{
    iduFile         mFile;
    UInt            mFileNo;
    void *          mBase;
    rpdXLogfile *   mNextXLogfile;
    rpdXLogfile *   mPrevXLogfile;
    SInt            mFileRefCnt;
    iduMutex        mRefCntMutex; //해당 mutex는 refCnt를 변경할때 잡는다.
    idBool          mIsWriteEnd;
} rpdXLogfile;

typedef struct rpdXLogfileHeader
{
    UInt    mFileWriteOffset;
    UInt    mFileFirstXLogOffset;
    smSN    mFileFirstXSN;
} rpdXLogfileHeader;

class rpdXLogfileMgr
{
public:
    /* 외부에서 호출가능한 인터페이스 */
    IDE_RC  initialize( SChar *     aReplName, 
                        rpXLogLSN   aXLogLSN, 
                        UInt        aExceedTimeout,
                        idBool *    aExitFlag,
                        void *      aReceiverPtr,
                        rpXLogLSN    aLastXLogLSN );
    IDE_RC  finalize();

    /* read */
    IDE_RC  checkRemainXLog();
    IDE_RC  getRemainSize( UInt *   aRemainSize );
    IDE_RC  getNextFileRemainSize( UInt *   aRemainSize );
    idBool  isLastXLog();
    IDE_RC  readXLog( void * aDest, UInt aLength, idBool aIsConvert = ID_TRUE );
    IDE_RC  readXLogDirect( void ** aMMapPtr, UInt aLength );

    /* write */
    IDE_RC  writeXLog( void * aSrc,
                       UInt   aLength,
                       smSN	  aXSN );

    /* xlogfile open */
    IDE_RC  openXLogfileForWrite();
    IDE_RC  openXLogfileForRead( UInt aXLogfileNo );
    IDE_RC  openFirstXLogfileForRead();
    IDE_RC  openAndMappingFile( UInt aXLogfileNo, rpdXLogfile ** aNewFile );
    static IDE_RC  openAndReadHeader( SChar * aRepName, SChar * aPath,
                                      UInt    aXLogfileNo, rpdXLogfileHeader * aOutHeader );
    void    setFileMeta( rpdXLogfile *  aFile, UInt aFileNo );

    /* for recovery */
    IDE_RC  findLastWrittenFileAndSetWriteInfo();

    void    getReadInfo( rpXLogLSN * aXLogLSN );

    SChar   mRepName[QCI_MAX_NAME_LEN + 1];
    SChar * mXLogDirPath;

public:
    /* 현재 사용중인 xlogfile의 정보 */
    rpXLogLSN   getReadXLogLSNWithLock();
    UInt        getReadFileNoWithLock();
    void        setReadInfoWithLock( rpXLogLSN aLSN, UInt  aFileNo );
    rpXLogLSN   mWriteXLogLSN;
    UInt        mWriteFileNo;
    iduMutex    mWriteInfoMutex;
    void *      mWriteBase;

    rpXLogLSN   getWriteXLogLSNWithLock();
    UInt        getWriteFileNoWithLock();
    void        setWriteInfoWithLock( rpXLogLSN aLSN, UInt  aFileNo );
    rpXLogLSN   mReadXLogLSN;
    UInt        mReadFileNo;
    iduMutex    mReadInfoMutex;
    void *      mReadBase;
    void        setWaitWrittenXLog( idBool aIsWait );
    idBool      getWaitWrittenXLog();

    /* 현재 열려있는 xlogfile */
    rpdXLogfile *   mCurWriteXLogfile;
    rpdXLogfile *   mCurReadXLogfile;

    void    addNewFileToFileList( rpdXLogfile * aBeforeFile, rpdXLogfile * aNewFile );
    void    addToFileListinLast( rpdXLogfile * aNewFile );

    rpdXLogfile *   mXLogfileListHeader;
    rpdXLogfile *   mXLogfileListTail;
    iduMutex        mXLogfileListMutex;

    ULong           mFileMaxSize;
    UInt            mExceedTimeout;

    /* file의 reference count 관련 */
    UInt    getFileRefCnt( rpdXLogfile * aFile );
    void    incFileRefCnt( rpdXLogfile * aFile );
    void    decFileRefCnt( rpdXLogfile * aFile );

    /* xlogfile 정리 관련 */
    void    removeFile( rpdXLogfile * aFile );

    /* sub thread 관련 */
    IDE_RC  stopSubThread();
    void    destroySubThread();
    rpdXLogfileFlusher *  mXLFFlusher;
    rpdXLogfileCreater *  mXLFCreater;

    /* 기타 */
    idBool      mEndFlag;
    idBool *    mExitFlag;
    idBool      mWaitWrittenXLog;

    /* read의 sleep/wake 관련 */
    void        sleepReader();
    void        wakeupReader();
    iduMutex    mReadWaitMutex;
    iduCond     mReadWaitCV;
    idBool      mIsReadSleep;
    // read가 write를 따라잡았을때 read의 작업 제어를 위해 사용

    /* write의 sleep/wake 관련 */
    void        sleepWriter();
    void        wakeupWriter();
    iduMutex    mWriteWaitMutex;
    iduCond     mWriteWaitCV;
    idBool      mIsWriteSleep;
    // write가 새 파일을 요구하나 새 파일이 아직 생성되지 않았을때 write의 작업 제어를 위해 사용

    /* static 함수 */
    static IDE_RC   removeAllXLogfiles( SChar * aReplName, UInt aFileNo );
    static void     rpdXLogfileEmergency( idBool aFlag );

    IDE_RC          getXLogLSNFromXSN( smSN aXSN, rpXLogLSN * aXLogLSN );
    static IDE_RC   getXLogLSNFromXSNAndReplName( smSN aXSN, SChar * aReplName, rpXLogLSN * aXLogLSN );
    static IDE_RC   getXLogfileNumberInternal( smSN aXSN,
                                               SInt aStartFileNo,
                                               SChar * aReplName,
                                               rpXLogLSN * aXLogLSN );

    static UInt checkXLogFileAndGetFileNoFromRepName( SChar * aReplName,
                                                      SChar * aFileName,
                                                      idBool * aIsXLogfile );
    static IDE_RC findFirstAndLastFileNo( SChar * aReplName,
                                          UInt * aFirstXLogFileNo,
                                          UInt * aLastXLogFileNo );

    static IDE_RC findRemainFirstXLogLSN( SChar     * aReplName,
                                          rpXLogLSN * aFirstXLogLSN );


    /* debug용 함수 */
    IDE_RC  byteToString( UChar   * aByte,
                          UShort    aByteSize,
                          UChar   * aString,
                          UShort  * aStringSize );
};

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getReadInfo                *
 * ------------------------------------------------------------------*
 * 현재 read info(XLogLSN)을 상위 모듈로 넘겨 replication meta에 저장한다.
 *
 * aXLogLSN    - [OUT] read 중인 xlogfile의 XLogLSN
 *********************************************************************/
inline void rpdXLogfileMgr::getReadInfo( rpXLogLSN * aXLogLSN )
{
    IDE_ASSERT( aXLogLSN != NULL );

    IDE_ASSERT( mReadInfoMutex.lock( NULL ) == IDE_SUCCESS );
    *aXLogLSN    = mReadXLogLSN;
    IDE_ASSERT( mReadInfoMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::isLastXLog                 *
 * ------------------------------------------------------------------*
 * 해당 함수는 recovery시 모든 gap이 처리되었는지 체크하기 위해 사용된다.
 *
 * aIsLastXLog - [OUT] xlogfile의 모든 xlog gap이 처리되었는지 여부
*********************************************************************/
inline idBool rpdXLogfileMgr::isLastXLog()
{
    idBool  sIsEnd = ID_FALSE;

    if( mReadXLogLSN == mWriteXLogLSN )
    {
        sIsEnd = ID_TRUE;
    }
    else
    {
        sIsEnd = ID_FALSE;
    }

    return sIsEnd;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::setWaitWrittenXLog         *
 * ------------------------------------------------------------------*
 * 기본적으로 read시 read가 write를 따라잡을 경우 일정 시간 대기한다.
 * 그런데 예외상황등으로 write가 일부만 쓰여진 상태에서 더이상 write가
 * 수행되지 않는 경우 read가 들어오지 않는 write를 대기하는 경우가 발생할 수 있다.
 * 이를 막기위해 mWaitWrittenXLog라는 변수를 두고
 * 해당 변수가 true일 경우에만 read가 write를 따라잡았을때 대기하도록 한다.
*********************************************************************/
inline void rpdXLogfileMgr::setWaitWrittenXLog( idBool  aIsWait )
{
    mWaitWrittenXLog = aIsWait;

    return;
}

inline idBool rpdXLogfileMgr::getWaitWrittenXLog( )
{
    return mWaitWrittenXLog;
}
#endif // _O_RPD_XLOGFILEMGR_H_
