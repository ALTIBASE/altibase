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
 * $Id: smrLogFile.h 89697 2021-01-05 10:29:13Z et16 $
 **********************************************************************/

#ifndef _O_SMR_LOG_FILE_H_
#define _O_SMR_LOG_FILE_H_ 1

#include <idu.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smrDef.h>
#include <smrLogHeadI.h>

/*
 * 하나의 로그파일에 대한 operation들을 구현한 객체
 *
 */
class smrLogFile
{
    friend class smrBackupMgr;
    friend class smrLogMgr;
    friend class smrLogFileMgr;
    friend class smrRemoteLogMgr; //PROJ-1915

//For Operation
public:
    static IDE_RC initializeStatic( UInt aLogFileSize );
    static IDE_RC destroyStatic();

    IDE_RC initialize();
    IDE_RC destroy();

    IDE_RC create( SChar  * aStrFilename,
                   SChar  * aInitBuffer,
                   UInt     aSize );

    IDE_RC prepare( SChar   * aStrFilename,
                    SChar   * aTempFileName,
                    SChar   * aInitBuffer,
                    SInt      aSize );

    IDE_RC backup(SChar   * aBackupFullFilePath);

    /* 로그파일을 DISK에서 제거한다.
     * checkpoint도중 로그파일 삭제가 실패하면 에러상황으로 처리하고,
     * restart recovery시에 로그파일 삭제가 실패하면 정상상황으로 처리한다.
     */
    static IDE_RC remove( SChar   * aStrFileName,
                          idBool    aIsCheckpoint );


    inline void append( SChar   * aStrData,
                        UInt      aSize );

    /* BUG-45711 */
    inline void scribble( SChar  * aStrData,
                           UInt     aSize,
                           UInt     aOffset );

    void   read( UInt     aOffset,
                 SChar  * aStrBuffer,
                 UInt     aSize );

    void   read( UInt       aOffset,
                 SChar   ** aLogPtr );

    IDE_RC readFromDisk( UInt     aOffset,
                         SChar  * aStrBuffer,
                         UInt     aSize );

    void   clear( UInt aBegin );
    inline IDE_RC sync() { return mFile.sync(); }

    // 로그 기록시 Align할 Page의 크기 리턴 
    UInt getLogPageSize();

    // 로그를 특정 Offset까지 Sync한다.
    IDE_RC syncLog( idBool  aSyncLastPage,
                    UInt    aOffsetToSync );

    inline void setPos( UInt    aFileNo,
                        UInt    aOffset )
    {
        mFileNo         = aFileNo;
        mOffset         = aOffset;
        mFreeSize       = mSize - aOffset;
        mPreSyncOffset  = aOffset;
        mSyncOffset     = aOffset;
    }

    inline IDE_RC lock();
    inline IDE_RC unlock();

    IDE_RC write( SInt     aWhere,
                  void   * aBuffer,
                  SInt     aSize );

    static void   getMemoryStatus() { mLogBufferPool.status(); }

    inline UInt getFileNo() { return mFileNo; }

#if defined(IA64_HP_HPUX)
   SInt touchMMapArea();
#else
   void touchMMapArea();
#endif

    // Log Buffer의 aStartOffset에서 aEndOffset까지 Disk에 내린다.
    IDE_RC syncToDisk( UInt aStartOffset, UInt aEndOffset );

    // 하나의 로그 레코드가 Valid한지 여부를 판별한다.
    static idBool isValidLog( smLSN       * aLSN,
                              smrLogHead  * aLogHeadPtr,
                              SChar       * aLogPtr,
                              UInt          aLogSizeAtDisk);

    // 로그파일번호와 로그레코드 오프셋으로 매직넘버를 생성한다.
    static inline smMagic makeMagicNumber( UInt     aFileNo,
                                           UInt     aOffset );

    // BUG-37018 log의 magic nunber가 유효한지 검사한다.
    static inline idBool isValidMagicNumber( smLSN       * aLSN,
                                             smrLogHead  * aLogHeadPtr );


    //* 로그파일의 File Begin Log가 정상인지 체크한다.
    IDE_RC checkFileBeginLog( UInt aFileNo );
    idBool isOpened() { return mIsOpened; };

    SChar* getFileName() { return mFile.getFileName(); }

    // Direct I/O를 사용할 경우 Direct I/O Page크기로
    // Align된 주소를 mBase에 세팅
    IDE_RC allocAndAlignLogBuffer();

    // allocAndAlignLogBuffer 로 할당한 로그버퍼를 Free한다.
    IDE_RC freeLogBuffer();

    inline void   setEndLogFlush( idBool aFinished );
    inline idBool isAllLogSynced(); /* BUG-35392 */

    inline idBool getEndLogFlush();

    smrLogFile();
    virtual ~smrLogFile();

    // 로그파일을 open한다.
    IDE_RC open( UInt     aFileNo,
                 SChar  * aStrFilename,
                 idBool   aIsMultiplexLogFile,
                 UInt     aSize,
                 idBool   aWrite      = ID_FALSE );

    IDE_RC close();

private:
    IDE_RC mmap( UInt   aSize,
                 idBool aWrite );
    IDE_RC unmap( void );
    UInt   getLastValidOffset(); /* BUG-35392 */
    
//For Member
public:
    //로그파일의 내용을 더이상 디스크에 기록할 필요가 없다면
    //ID_TRUE, else ID_FALSE
    idBool       mEndLogFlush;

    //Open되었으면 ID_TRUE, else ID_FALSE
    idBool       mIsOpened;
    
    // 디스크상의 로그파일
    iduFile      mFile;

    /* Logfile의 FileNo */
    UInt         mFileNo;
    /* Logfile의 로그가 기록될 위치 */
    UInt         mOffset;
    
    // 현재까지 sync가 완료된 크기
    // 즉, 다음번에 sync를 시작해야할 offset
    UInt         mSyncOffset;

    // Sync요청이 들어왔으나 마지막 페이지라서 sync하지 못 한 경우
    // mSyncOffset을 mPreSyncOffset을 기록해둔다.
    // syncLog함수에서는 mPreSyncOffset을 보고,
    // 이전에 마지막 페이지라서 sync못한 경우에는
    // 마지막 페이지라도 sync를 하도록 한다.
    UInt         mPreSyncOffset;
    UInt         mFreeSize;
    UInt         mSize;

    // 하나의 로그파일의 내용을 지니는 버퍼
    //
    // Durability Level 5에서 
    // Direct I/O시 한다면 Log Buffer의 시작 주소 또한
    // Direct I/O Page크기에 맞게 Align을 해주어야 한다.
    //  - mBaseAlloced : mLogBufferPool에서 할당받은 Align되지 않은 로그버퍼
    //  - mBase        : mBaseAlloced를 Direct I/O Page크기 로 Align한 로그버퍼
    void        *mBaseAlloced;
    void        *mBase;

    // 이 로그파일의 reference count
    UInt         mRef;

    // 이 로그파일에 대한 동시성 제어를 위한 Mutex
    iduMutex     mMutex;
    // 이 로그파일을 다 써서 다른 로그파일로 switch되었는지 여부
    idBool       mSwitch;
    // 이 로그파일에 쓰기가 가능한지 여부
    idBool       mWrite;
    // 이 로그파일을 mmap으로 열었는지 여부
    idBool       mMemmap;
    // 디스크의 로그파일을 mFile로 열었는지 여부 
    idBool       mOnDisk;

    // smrLogFleMgr에서 open된 로그파일 리스트를 관리할 때 사용
    smrLogFile  *mPrvLogFile;
    smrLogFile  *mNxtLogFile;

    // smrLFThread에서 flush할 로그파일 리스트를 관리할 때 사용
    smrLogFile  *mSyncPrvLogFile;
    smrLogFile  *mSyncNxtLogFile;

    //PROJ-2232 log multiplex
    //다중화 로그버퍼에 SMR_LT_FILE_END까지 복사되었고 다중화 로그파일이
    //다음 로그파일로 switch되었는지 확인
    volatile idBool   mMultiplexSwitch[ SM_LOG_MULTIPLEX_PATH_MAX_CNT ]; 

    // 로그파일 버퍼를 할당받을 메모리 풀
    static iduMemPool mLogBufferPool;

//For Test
    UInt         mLstSyncPos;

    idBool       mIsMultiplexLogFile;
};

inline IDE_RC smrLogFile::lock()
{
    return mMutex.lock( NULL );
}

inline IDE_RC smrLogFile::unlock()
{

    return mMutex.unlock();

}

/*
 * 로그파일 번호와 로그레코드 오프셋을 이용하여
 * 로그파일에 기록할 매직넘버를 생성한다.
 *
 * [ 매직넘버의 역할 ]
 * 매직넘버는 32비트 값으로 모든 로그의 헤더에 기록된다.
 * +--------------------+-------------------------+
 * | 상위비트           |  하위비트               |
 * | 9 비트 ( 0~ 511 )  |  23 비트 ( 0 ~ 8M - 1 ) |
 * | 로그파일번호       |  로그레코드 offset      |
 * +--------------------+-------------------------+
 *
 * SYNC_CREATE_ = 0 으로 주어 로그파일을 0으로 memset되지 않도록 생성할 경우
 * Invalid한 Log를 Valid한 로그로 판별할 확률을 낮추어준다.
 */


/*
 * 로그파일번호와 로그레코드 오프셋으로 매직넘버를 생성한다.
 *
 * aFileNo [IN] 로그파일번호
 * aOffset [IN] 로그파일 오프셋
 *
 * return       로그파일 번호와 오프셋을 이용하여 생성한 매직넘버를 리턴한다.
 */
inline smMagic smrLogFile::makeMagicNumber( UInt    aFileNo,
                                            UInt    aOffset )
{
#define LOG_FILE_NO_BITS (4)
#define LOG_FILE_NO_MASK ( (1<<LOG_FILE_NO_BITS)-1 )

#define LOG_RECORD_OFFSET_BITS (12)
#define LOG_RECORD_OFFSET_MASK ( (1<<LOG_RECORD_OFFSET_BITS)-1 )
    UInt sMagic;

    IDE_DASSERT( (LOG_FILE_NO_BITS + LOG_RECORD_OFFSET_BITS) == 16 );

    sMagic = ( ( aFileNo & ( LOG_FILE_NO_MASK ) ) << LOG_RECORD_OFFSET_BITS ) |
               ( aOffset & LOG_RECORD_OFFSET_MASK );

    /* BUG-35392
     * 매직넘버는 2바이트 smMagic(UShort)으로만 사용하므로
     * UInt가 아닌 2바이트(smMagic)를 리턴 하도록 한다. */
    sMagic &= 0x0000FFFF;

    return (smMagic)sMagic;
}

void smrLogFile::setEndLogFlush( idBool aFinished )
{
    if( aFinished == ID_TRUE )
    {
        IDE_ASSERT( mSyncOffset == mOffset );
    }

    mEndLogFlush = aFinished;
}

/***********************************************************************
 * BUG-35392 
 * Description : LogFile이 모두 Sync되었는지의 여부를 반환한다.
 *               switch대상 로그에 대해서만 호출 되므로
 *               mSyncOffset, mOffset 의 동시성은 걱정하지 않아도 된다.
 ***********************************************************************/
idBool smrLogFile::isAllLogSynced()
{
    IDE_ASSERT( mSyncOffset <= mOffset );

    if( mSyncOffset == mOffset )
    {
        return  ID_TRUE;
    }

    return  ID_FALSE;
}

idBool smrLogFile::getEndLogFlush()
{
    return mEndLogFlush;
}

/***********************************************************************
 * BUG-37018
 * Description : code 중복을 방지하기 위해 추가된 inline 함수
 *               dummy log의 valid검사는 magic number로만 검증가능
 *               하기때문에 본 함수로 검증한다.
 *               (일반 로그의 valid검사할때도 사용된다) 
 **********************************************************************/
idBool smrLogFile::isValidMagicNumber( smLSN       * aLSN,
                                       smrLogHead  * aLogHeadPtr )
{
    idBool sIsValid;

    if ( makeMagicNumber( aLSN->mFileNo,
                          aLSN->mOffset )
         != smrLogHeadI::getMagic(aLogHeadPtr) )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_LOGFILE_INVALID_MAGIC,
                    aLSN->mFileNo,
                    aLSN->mOffset,
                    smrLogHeadI::getMagic(aLogHeadPtr),
                    makeMagicNumber(aLSN->mFileNo, 
                                    aLSN->mOffset));

        sIsValid = ID_FALSE;
    }
    else
    {
        sIsValid = ID_TRUE;
    }
    
    return sIsValid;
}

inline void smrLogFile::append( SChar  * aStrData,
                                UInt     aSize )
{
    IDE_ASSERT( ( aSize != 0 ) && ( mFreeSize >= (UInt)aSize ) );

    idlOS::memcpy( (SChar*)mBase + mOffset,
                    aStrData,
                    aSize );

    /* BUG-45916 MEM_BARRIER 제거시 AIX 장비에서
     * Log Multiplex Thread가 memcpy 완료 이전에
     * Log를 Copy 하는 경우가 발생함. */
    IDL_MEM_BARRIER;

    mFreeSize -= aSize;
    mOffset   += aSize;
}

/* BUG-45711 */
inline void smrLogFile::scribble( SChar  * aStrData,
                                  UInt     aSize,
                                  UInt     aOffset )
{
    /* 로그 쓰기 */
    idlOS::memcpy( (SChar *)mBase + aOffset,
                    aStrData,
                    aSize );
}

#endif /* _O_SMR_LOG_FILE_H_ */
