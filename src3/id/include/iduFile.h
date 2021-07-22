/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFile.h 85333 2019-04-26 02:34:41Z et16 $
 **********************************************************************/

/***********************************************************************
 *
 * NAME
 *   iduFile.h
 *
 * DESCRIPTION
 *
 * PUBLIC FUNCTION(S)
 *
 * PRIVATE FUNCTION(S)
 *
 * NOTES
 *
 * MODIFIED    (MM/DD/YYYY)
 *    hyyou     02/11/2000 - Modified
 *
 **********************************************************************/

#ifndef _O_IDU_FILE_H_
#define _O_IDU_FILE_H_ 1

#include <idErrorCode.h>
#include <idl.h>
#include <idv.h>
#include <iduFXStack.h>
#include <iduMemDefs.h>
#include <iduProperty.h>
#include <iduFileIOVec.h>

#define ID_MAX_FILE_DESCRIPTOR_COUNT  1024

#if defined(IBM_AIX)
#define IDU_FILE_DIRECTIO_ALIGN_SIZE 512
#else
#define IDU_FILE_DIRECTIO_ALIGN_SIZE 4096
#endif

/* BUG-30834: Direct IO limitation of Windows */
/* Some Windows versions has the limitation of buffer size for direct IO. */
#if defined(VC_WIN32)
# if (OS_MAJORVER == 5)
#  if defined(ALTI_CFG_CPU_X86) && defined(COMPILE_64BIT)
#   define IDU_FILE_DIRECTIO_MAX_SIZE 33525760 /* 64bit XP/2K3 */
#  else
#   define IDU_FILE_DIRECTIO_MAX_SIZE 67076096 /* 32bit XP/2K3 or IA-64 XP/2K3 */
#  endif
# elif (OS_MAJORVER == 6) && (OS_MINORVER == 0)
#  define IDU_FILE_DIRECTIO_MAX_SIZE 2147479552 /* for Vista/2K8 */
# else
#  define IDU_FILE_DIRECTIO_MAX_SIZE 4294963200 /* for Win7/2K8R2 or upper */
# endif
#else
#define IDU_FILE_DIRECTIO_MAX_SIZE ID_SLONG_MAX /* no limitation */
#endif


/* ------------------------------------------------
 * 파일별 IO 통계정보
 * ----------------------------------------------*/ 
typedef struct iduFIOStat
{
    /* Physical Read I/O 발생 횟수 */
    ULong    mPhyReadCount;
    /* Physical Wrute I/O 발생 횟수 */
    ULong    mPhyWriteCount;
    /* Physical Read로 판독한 Block 개수 */
    ULong    mPhyBlockReadCount;
    /* Physical Write로 기록한 Block 개수 */
    ULong    mPhyBlockWriteCount;
    /* Read I/O time  */
    ULong    mReadTime;
    /* Write I/O time  */
    ULong    mWriteTime;    
    /* 단일 Block Read I/O time  */
    ULong    mSingleBlockReadTime;
    /* 마지막 I/O time  */
    ULong    mLstIOTime;
    /*  I/O 최소 time  */
    ULong    mMinIOTime;
    /*  Read I/O 최대 time  */
    ULong    mMaxIOReadTime;
    /*  Write I/O 최대 time  */
    ULong    mMaxIOWriteTime;
} iduFIOStat;

/* ------------------------------------------------
 * 파일별 IO 통계정보 수집 기능 On/Off 플래그
 * ----------------------------------------------*/ 
typedef enum iduFIOStatOnOff
{
    IDU_FIO_STAT_OFF = 0,
    IDU_FIO_STAT_ON,
    IDU_FIO_STAT_ONOFF_MAX

} iduFIOStatOnOff; 

/* ------------------------------------------------
 * 파일별 IO 통계정보 수집 기능 On/Off 플래그
 * ----------------------------------------------*/ 
typedef enum iduFIOType
{
    IDU_FIO_TYPE_READ = 0,
    IDU_FIO_TYPE_WRITE
    
} iduFIOType;

// DIRECT : pointer should not be NULL!!!
#define IDU_FIO_STAT_ADD(__iduBasePtr__, __Name__, __Value__) \
        {(__iduBasePtr__)->__Name__ += __Value__;}

#define IDU_FIO_STAT_SET(__iduBasePtr__, __Name__, __Value__) \
        {(__iduBasePtr__)->__Name__ = __Value__;}

typedef void (*iduEmergencyFuncType)(idBool aFlag);

typedef void (*iduFIOStatBegin)( idvTime * aTime );

typedef void (*iduFIOStatEnd)( iduFIOStat   * aStatPtr,
                               iduFIOType     aFioType,
                               idvTime      * aBeginTime );

typedef struct iduFIOStatFunc
{
   iduFIOStatBegin  mBeginStat;
   iduFIOStatEnd    mEndStat;

} iduFIOStatFunc;

#define IDU_FD_STACK_INFO_SIZE ( ID_SIZEOF( iduFXStackInfo ) + \
                                 ID_SIZEOF( PDL_HANDLE ) * ID_MAX_FILE_DESCRIPTOR_COUNT )

/* BUG-17954 pread, pwrite가 없는 OS에서는 모든 IO가 이 함수에서
   Global Mutex에 lock을 잡고 IO를 수행하여 모든 Disk IO가 경합하는
   문제가 생김. 하나의 FD에 대해서 하나의 IO 요청이 되도록 동시성
   제어를 함. */
class iduFile
{
public:
    IDE_RC     initialize( iduMemoryClientIndex aIndex,
                           UInt                 aMaxMaxFDCnt,
                           iduFIOStatOnOff      aIOStatOnOff,
                           idvWaitIndex         aIOMutexWaitEventID );
    IDE_RC     destroy();

    IDE_RC     create();
    IDE_RC     createUntilSuccess( iduEmergencyFuncType setEmergencyFunc,
                                   idBool aKillServer = ID_FALSE );

    IDE_RC     open(idBool a_bDirect   = ID_FALSE,
                    SInt   aPermission = O_RDWR); /* BUG-31702 add permission parameter */

    IDE_RC     openUntilSuccess(idBool a_bDirect   = ID_FALSE,
                                SInt   aPermission = O_RDWR); /* BUG-31702 add permission parameter */

    IDE_RC     close();
    IDE_RC     sync();
    IDE_RC     syncUntilSuccess(iduEmergencyFuncType setEmergencyFunc,
                                idBool aKillServer = ID_FALSE);

    inline void dump();

    inline IDE_RC    getFileSize( ULong *aSize );
    inline IDE_RC    setFileName( SChar *aFileName );
    inline SChar*    getFileName() { return mFilename; };

    inline idBool exist();

    IDE_RC     write( idvSQL     * aStatSQL,
                      PDL_OFF_T    aWhere,
                      void*        aBuffer,
                      size_t       aSize );

    IDE_RC     writeUntilSuccess( idvSQL             * aStatSQL,
                                  PDL_OFF_T            aWhere,
                                  void*                aBuffer,
                                  size_t               aSize,
                                  iduEmergencyFuncType setEmergencyFunc,
                                  idBool aKillServer = ID_FALSE );

    IDE_RC     writev ( idvSQL*              aStatSQL,
                        PDL_OFF_T            aWhere,
                        iduFileIOVec&        aVec );

    IDE_RC    writevUntilSuccess( idvSQL  *     aStatSQL,
                                  PDL_OFF_T     aWhere,
                                  iduFileIOVec& aVec,
                                  iduEmergencyFuncType aSetEmergencyFunc,
                                  idBool        aKillServer = ID_FALSE );

    IDE_RC     read ( idvSQL *   aStatSQL,
                      PDL_OFF_T  aWhere,
                      void*      aBuffer,
                      size_t     aSize );
    
    IDE_RC     read ( idvSQL *    aStatSQL,
                      PDL_OFF_T   aWhere,
                      void*       aBuffer,
                      size_t      aSize,
                      size_t*     aRealReadSize );

    IDE_RC     readv ( idvSQL*              aStatSQL,
                       PDL_OFF_T            aWhere,
                       iduFileIOVec&        aVec );

    IDE_RC     copy ( idvSQL * aStatSQL, 
                      SChar  * a_pFileName, 
                      idBool   a_bDirect = ID_TRUE );

    static IDE_RC allocBuff4DirectIO( iduMemoryClientIndex   aIndex,
                                      UInt    aSize,
                                      void**  aAllocBuff,
                                      void**  aAllocAlignedBuff );

    iduFIOStatOnOff getFIOStatOnOff() { return mIOStatOnOff; }

    static idBool canDirectIO( PDL_OFF_T  aWhere,
                               void*      aBuffer,
                               size_t     aSize );

    IDE_RC setMaxFDCnt( SInt aMaxFDCnt );

    inline UInt getMaxFDCnt();
    inline UInt getCurFDCnt();

    IDE_RC allocFD( idvSQL     *aStatSQL,
                    PDL_HANDLE *aFD );

    IDE_RC freeFD( PDL_HANDLE aFD );

    /*
     * BUG-27779 iduFile::truncate함수 인자가 UInt라서 ID_UINT_MAX를 
     *           넘는 크기로 truncate시 오동작합니다. 
     */
    IDE_RC truncate( ULong aFileSize );

    IDE_RC fallocate( SLong  aSize );

    IDE_RC mmap( idvSQL     *aStatSQL,
                 UInt        aSize,
                 idBool      aWrite,
                 void**      aMMapPtr );
    IDE_RC munmap(void*, UInt);

private:
    IDE_RC open( PDL_HANDLE *aFD );

    inline IDE_RC close( PDL_HANDLE  aFD );

    IDE_RC closeAll();

    IDE_RC checkDirectIOArgument( PDL_OFF_T  aWhere,
                                  void*      aBuffer,
                                  size_t     aSize );
    IDE_RC checkDirectIOArgument( PDL_OFF_T      aWhere,
                                  iduFileIOVec&  aVec );
    // File의 Open Flag를 결정한다.
    SInt getOpenFlag( idBool aIsDirectIO );

    // File IO Stat을 계산을 시작한다.
    static inline void beginFIOStat( idvTime * aTime );
    static void beginFIOStatNA( idvTime * ) { return; }

    // File IO Stat을 계산을 완료한다.
    static void endFIOStat( iduFIOStat   * aStatPtr,
                            iduFIOType     aFIOType,
                            idvTime      * aBeginTime );
    static void endFIOStatNA( iduFIOStat * , iduFIOType, idvTime * ){ return; }

public: /* POD class type should make non-static data members as public */
    iduMemoryClientIndex       mIndex;
    SChar                      mFilename[ID_MAX_FILE_NAME];
    /* 현재 File이 Direct IO로 열려 있으면 ID_TRUE, else ID_FALSE*/
    idBool                     mIsDirectIO;
    /* BUG-31702 file open시에 mode를 선택한다. */
    SInt                       mPermission;
    /* 파일별 통계정보 */
    iduFIOStat              mStat;
    iduFIOStatOnOff         mIOStatOnOff;
    static iduFIOStatFunc   mStatFunc[ IDU_FIO_STAT_ONOFF_MAX ];

    /* 파일 IO Mutex 에 대한 Wait Event ID */
    idvWaitIndex            mWaitEventID;

    /* 이 파일에 Open할 수 있는 최대 FD 갯수 */
    UInt                    mMaxFDCount;

    /* 이 파일에 현재 Open되어 있는 FD 갯수 */
    UInt                    mCurFDCount;

private:
    /* iduFXStackInfo가 저장될 Buffer영역으로 Stack Item으로 FD(File
       Descriptor)가 저장된다. 최대저장될 수 있는 FD 갯수는
       ID_MAX_FILE_DESCRIPTOR_COUNT 이다. */
    ULong                   mFDStackBuff[ ( IDU_FD_STACK_INFO_SIZE + ID_SIZEOF( ULong )  - 1 ) \
                                          / ID_SIZEOF( ULong ) ];


    /* mFDStackBuff을 가리킨다. 매번 Casting이 귀찮아서 넣었습니다. */
    iduFXStackInfo*         mFDStackInfo;

    /* IO 통계정보가 동시에 갱신되는 것을 방지 */
    iduMutex                mIOStatMutex;

    /* FD갯수 정보에 대한 동시성 제어를 위해서 추가됨 */
    iduMutex                mIOOpenMutex;
};

inline IDE_RC iduFile::open( idBool aIsDirectIO, SInt aPermission )
{
    PDL_HANDLE sFD;

    /* 이 함수는 처음에 한번만 호출되어야 한다 .*/
    IDE_ASSERT( mCurFDCount == 0 );

    mIsDirectIO = aIsDirectIO;
    mPermission = aPermission;

    /* BUG-15961: DirectIO를 쓰지 않는 System Property가 필요함 */
    if ( iduProperty::getDirectIOEnabled() == 0 )
    {
        mIsDirectIO = ID_FALSE;
    }

    IDE_TEST( open( &sFD ) != IDE_SUCCESS );

    // to remove false alarms from codesonar test
#ifdef __CSURF__
    IDE_ASSERT( mFDStackInfo->mItemSize == ID_SIZEOF(PDL_HANDLE) );
#endif

    /* 새로운 FD를 FDStackInfo에 등록한다. */
    IDE_TEST( iduFXStack::push( NULL /* idvSQL */, mFDStackInfo, &sFD )
              != IDE_SUCCESS );

    /* 첫번째 Open이므로 mIOOpenMutex을 잡을 필요가 없다. */
    mCurFDCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 현재 이 파일에 Open된 모든 FD를 Close한다. */
inline IDE_RC iduFile::close()
{
    return closeAll();
}

/* aFilename으로 파일이름을 설정한다. */
inline IDE_RC iduFile::setFileName(SChar * aFilename)
{
    IDE_TEST_RAISE( idlOS::strlen( aFilename ) >= ID_MAX_FILE_NAME,
                   too_long_error );

    idlOS::strcpy( mFilename, aFilename );

    return IDE_SUCCESS;

    IDE_EXCEPTION(too_long_error);
    {
        /* BUG-34355 */
        IDE_SET(ideSetErrorCode(idERR_ABORT_idnTooLarge));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 파일 크기를 얻어온다. */
inline IDE_RC iduFile::getFileSize( ULong *aSize )
{
    PDL_stat   sStat;
    SInt       sState = 0;
    PDL_HANDLE sFD;

    IDE_TEST( allocFD( NULL, &sFD ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST_RAISE( idf::fstat( sFD, &sStat ) != 0,
                    error_file_fstat );

    sState = 0;
    IDE_TEST( freeFD( sFD ) != IDE_SUCCESS );

    *aSize = (ULong)sStat.st_size;

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_file_fstat);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_Sysfstat,
                                        mFilename));
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( freeFD( sFD ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* mFilename을 가진 파일이 존재하는지를 알려준다. */
inline idBool iduFile::exist()
{
    if( idf::access( mFilename, F_OK ) == 0 )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/* aFD을 OS에게 반환한다. */
inline IDE_RC iduFile::close( PDL_HANDLE aFD )
{
    IDE_ASSERT( aFD != IDL_INVALID_HANDLE );

    IDE_TEST_RAISE( idf::close( aFD ) != 0, close_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION( close_error );
    IDE_EXCEPTION_END;
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_SysClose,
                                  mFilename ) );
    }
    return IDE_FAILURE;
}

inline void iduFile::dump()
{
    fprintf(stderr, "<FileName: %s Open FD Count: %d>\n",
            mFilename,
            iduFXStack::getItemCnt( mFDStackInfo ) );
}

/***********************************************************************
 * Description : I/O 통계정보에 대한 계산을 수행한다.
 *
 * Related Issue: TASK-2356 DRDB DML 문제 파악
 ***********************************************************************/
inline void iduFile::beginFIOStat( idvTime  * aBeginTime )
{
    IDV_TIME_GET( aBeginTime );
}

inline UInt iduFile::getMaxFDCnt()
{
    return mMaxFDCount;
}

inline UInt iduFile::getCurFDCnt()
{
    return mCurFDCount;
}

#endif  // _O_FILE_H_
