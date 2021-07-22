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
 
#ifndef _O_IDF_H_
#define _O_IDF_H_ 1

#include <OS.h>

/*
1. 리눅스 raw device는 fsync를 지원하지 않는다.
   따라서 리눅스에서는 raw device를 사용하면 안된다.

2. VxWorks raw device는 alignment를 고려하지 않아도 된다.
   하지만 lseek, fsync는 ioctl 함수를 통해서만 지원된다.
   따라서 logging, lock, lseek, fsync에 대한 고려가 필요하다.
*/

//==============================================================
//
//                       System Setting
//
// * Disk Size (IDF_PAGE_SIZE * IDF_PAGE_NUM)
//
//         4G : 4294967296    2G : 2147483648
//         1G : 1073741824  512M :  536870912
//       256M :  268435456  128M :  134217728
//
// * Page Size (IDF_PAGE_SIZE)
//
//     512, 1024(1K), 4096(4K), 32768(32K)
//
// * Page Num (IDF_PAGE_NUM)
//
//     (Disk Size) / (Page Size)
//
// * Block Size (IDF_PAGES_PER_BLOCK)
//
//     (Page Size) * (IDF_PAGES_PER_BLOCK)
//
// * Max File Size in VFS
//
//     ( (Page Size)^2 / sizeof(UInt) * IDF_INDIRECT_PAGE_NUM )
//     + ( IDF_DIRECT_MAP_SIZE * (Page Size) )
//
// * Meta Size (IDF_META_SIZE)
//     > (32 + IDF_INDIRECT_PAGE_NUM) * sizeof(UInt)
//
//==============================================================

#if defined(SMALL_FOOTPRINT)
// PreDefined4 (Disk Size  : 32M, Page Size     :  1K,
//              Block Size :  4K, Max File Size : 48M)
#define IDF_PAGE_SIZE         (1024)
#define IDF_PAGE_NUM          (32768)
#define IDF_PAGE_MAP_LOG_SIZE (32768)
#define IDF_PAGES_PER_BLOCK   (4)
#define IDF_META_SIZE         (1024)
#define IDF_INDIRECT_PAGE_NUM (192)
#define IDF_MAX_FILE_COUNT    (128)
#else
// default (Disk Size  :   4G, Page Size     : 32K,
//          Block Size : 256K, Max File Size :  4G)
#define IDF_PAGE_SIZE         (32768)
#define IDF_PAGE_NUM          (131072)
#define IDF_PAGE_MAP_LOG_SIZE (4096)
#define IDF_PAGES_PER_BLOCK   (8)
#define IDF_META_SIZE         (192)
#define IDF_INDIRECT_PAGE_NUM (16)
#define IDF_MAX_FILE_COUNT    (1024)
#endif

/*
// PreDefined1 (Disk Size  : 256M, Page Size     :   4K,
//              Block Size :  16K, Max File Size : 256M)
#define IDF_PAGE_SIZE         (4096)
#define IDF_PAGE_NUM          (65536)
#define IDF_PAGE_MAP_LOG_SIZE (32768)
#define IDF_PAGES_PER_BLOCK   (4)
#define IDF_META_SIZE         (400)
#define IDF_INDIRECT_PAGE_NUM (64)
#define IDF_MAX_FILE_COUNT    (1024)
*/

/*
// PreDefined2 (Disk Size  : 64M, Page Size     :  1K,
//              Block Size :  4K, Max File Size : 48M)
#define IDF_PAGE_SIZE         (1024)
#define IDF_PAGE_NUM          (65536)
#define IDF_PAGE_MAP_LOG_SIZE (32768)
#define IDF_PAGES_PER_BLOCK   (4)
#define IDF_META_SIZE         (1024)
#define IDF_INDIRECT_PAGE_NUM (192)
#define IDF_MAX_FILE_COUNT    (1024)
*/

/*
// PreDefined3 (Disk Size  : 64M, Page Size     : 512B,
//              Block Size :  2K, Max File Size :  12M)
#define IDF_PAGE_SIZE         (512)
#define IDF_PAGE_NUM          (131072)
#define IDF_PAGE_MAP_LOG_SIZE (32768)
#define IDF_PAGES_PER_BLOCK   (4)
#define IDF_META_SIZE         (1024)
#define IDF_INDIRECT_PAGE_NUM (192)
#define IDF_MAX_FILE_COUNT    (1024)
*/

//================
// Default Values
//================

// version
#define IDF_MAJOR_VERSION (0)
#define IDF_MINOR_VERSION (3)

// Page
//#define IDF_PAGE_NUM        (131072)
//#define IDF_PAGE_SIZE        (32768) //(32*1024)
//#define IDF_PAGES_PER_BLOCK      (8)

// File Count
#define IDF_MAX_FILE_OPEN_COUNT (1024)
//#define IDF_MAX_FILE_COUNT      (1024)
#define IDF_MAX_FILE_NAME_LEN     (64)

// File ID and File Descriptor
#define IDF_INVALID_MODE    (99)
#define IDF_INVALID_FILE_ID (-1)
#define IDF_INVALID_HANDLE  (-1)

#define IDF_FD_UNUSED  (0)
#define IDF_FD_USED    (1)
#define IDF_FD_INVALID (2)

// System Meta Data Position and Size
#define IDF_FIRST_META_POSITION (128)
//#define IDF_META_SIZE           (248)

// Logical Page Address Inverter
#define IDF_FILE_HOLE_MASK     (0x80000000)
#define IDF_FILE_HOLE_INVERTER (0x7FFFFFFF)

// 페이지 할당을 8개씩 하기 때문에 Direct Page Map의 크기는 8의 배수여야 한다.
#define IDF_DIRECT_MAP_SIZE      (8)
#define IDF_INDIRECT_MAP_SIZE (8192) //(IDF_PAGE_SIZE / 4)
#define IDF_INDIRECT_ENTRY_SIZE (32) // sizeof(int) * IDF_PAGES_PER_BLOCK 
#define IDF_MAP_POOL_SIZE        (8)
#define IDF_DIRECT_PAGE_NUM      (8)
//#define IDF_INDIRECT_PAGE_NUM   (16)

// Log Type
#define IDF_INVALID_LOG  (0)
#define IDF_MASTER_LOG   (1)
#define IDF_META_LOG     (2)
#define IDF_PAGEMAP_LOG  (3)

// Log Mode of File System
#define IDF_NO_LOG_MODE (0)
#define IDF_LOG_MODE    (1)
#if defined(USE_RAW_DEVICE)
#define IDF_DEFAULT_LOG_MODE IDF_NO_LOG_MODE
#else
#define IDF_DEFAULT_LOG_MODE IDF_LOG_MODE
#endif

// Log Setting
#define IDF_LOG_BUFFER_SIZE (32768)
//#define IDF_PAGE_MAP_LOG_SIZE (4096)
#define IDF_LOG_TRANS_SIZE (32)

// Buffer for Dirty Page
#define EMPTY_BUFFER_SIZE (IDF_PAGE_SIZE)

// System Signature
#define IDF_SYSTEM_SIGN (0xABCDABCD)

#if defined(USE_RAW_DEVICE) && !defined(WRS_VXWORKS)
#define IDF_ALIGN_SIZE (512)
#else
#define IDF_ALIGN_SIZE (8)
#endif

#define IDF_ALIGNED_BUFFER_SIZE (IDF_ALIGN_SIZE * 8)

#define IDF_END_RECOVERY    (0)
#define IDF_START_RECOVERY  (1)

//============
// Structures
//============
typedef struct idfMaster
{
    UInt  mMajorVersion;     // file system major version
    UInt  mMinorVersion;     // file system minor version
    UInt  mNumOfPages;       // 전체 Page의 개수
    UInt  mSizeOfPage;       // 1개 Page의 크기
    UInt  mAllocedPages;     // 할당한 페이지 개수, mNumOfPages >= mAllocedPages
    UChar mFSMode;           // 파일의 공간을 한 번에 모두 할당할지 설정
    UChar mLogMode;          // Log의 모드를 설정 [0 : no logging | 1 : logging]
    UInt  mNumOfFiles;       // file 개수
    UInt  mPagesPerBlock;    // Block 당 Page 개수
    SInt  mTimestamp;        // Master Page를 저장한 시간
    SInt  mMaxFileCount;     // 최대 파일 개수
    SInt  mMaxFileOpenCount; // 최대 파일 descriptor 개수
    UInt  mCRC;              // CRC of Master
    UInt  mSignature;        // Mater Page의 마지막을 명시
} idfMaster;

typedef struct idfMeta
{
    SChar mFileName[IDF_MAX_FILE_NAME_LEN];  // 파일 이름
    SInt  mFileID;                           // 파일 고유 번호, 0번부터 할당
    UInt  mSize;                             // 파일 크기
    SInt  mCTime;                            // 파일 생성 시간
    UInt  mNumOfPagesU;                      // 사용중인 Page 개수
    UInt  mNumOfPagesA;                      // 할당한 Page 개수
    UInt  mDirectPages[IDF_DIRECT_MAP_SIZE]; // Direct Page Address(MAX 256KB)
    UInt  mIndPages[IDF_INDIRECT_PAGE_NUM];  // FAT의 주소(MAX 4GB)
    UInt  mCRC;                              // CRC of Meta
    UInt  mSignature;
} idfMeta;

typedef struct idfMetaEntry
{
    idfMeta *mMeta;
    SInt     mFileOpenCount;
} idfMetaEntry;

typedef struct idfFdEntry
{
    UInt  mIsUsed;
    SInt  mFileID;
    ULong mCursor;
} idfFdEntry;

typedef struct idfDirent
{
    // 디렉토리 밑의 Dirent객체는 double linked list로 연결되어 있다.
    // readdir_r() 함수 호출시 list를 탐색하면서 하나씩 반환한다.
    // dirent 객체는 mDirent에 달려있다.
    void      *mDirent;
    idfDirent *mNext; // next idfDirent객체를 가리킴
} idfDirent;

typedef struct idfDir
{
    SInt       mDirCount; // idfDirent객체의 개수
    SInt       mDirIndex; // readdir_r() 호출시 읽은 idfDirent객체의 index
    idfDirent *mFirst;    // 첫번째 idfDirent객체를 가리킴
} idfDir;
typedef struct idfPageMap // idf Indirect Page Map
{
    SInt        mFileID;
    UInt        mPageIndex;
    UInt        mMapIndex;
    UInt       *mMap;
    idfPageMap *mNext;
    idfPageMap *mPrev;
} idfPageMap;

//===============================================
// Log type
//     0 : N/A
//     1 : Master    (mFildID => N/A)
//     2 : Meta
//     3 : Page Map
//===============================================
typedef struct idfLogHeader
{
    SInt mFileID;
    UInt mType;
} idfLogHeader;

typedef struct idfMapHeader
{
    SInt mFileID;
    UInt mIndex;
    UInt mMapIndex;
} idfMapHeader;

typedef struct idfMemList
{
    idfMemList *mNext;
    void       *mPtr;
    UInt        mSize;
} idfMemList;

//===============
// API Functions
//===============
typedef PDL_HANDLE (*idfopen)     (const SChar*, SInt, ...); 
typedef SInt       (*idfclose)    (PDL_HANDLE);
#if defined(VC_WINCE)
typedef PDL_HANDLE (*idfcreat)    (SChar*, mode_t);
#else
typedef PDL_HANDLE (*idfcreat)    (const SChar*, mode_t);
#endif
typedef ssize_t    (*idfread)     (PDL_HANDLE, void*, size_t);
typedef ssize_t    (*idfwrite)    (PDL_HANDLE, const void*, size_t);
typedef ssize_t    (*idfreadv)    (PDL_HANDLE, const iovec*, int);
typedef ssize_t    (*idfwritev)   (PDL_HANDLE, const iovec*, int);
typedef ssize_t    (*idfpread)    (PDL_HANDLE, void*, size_t, off_t);
typedef ssize_t    (*idfpwrite)   (PDL_HANDLE, const void*, size_t, off_t);
typedef ssize_t    (*idfpreadv)   (PDL_HANDLE, const iovec*, int, PDL_OFF_T);
typedef ssize_t    (*idfpwritev)  (PDL_HANDLE, const iovec*, int, PDL_OFF_T);
typedef PDL_OFF_T  (*idflseek)    (PDL_HANDLE, PDL_OFF_T, SInt);
typedef SInt       (*idffstat)    (PDL_HANDLE, PDL_stat*);
typedef SInt       (*idfunlink)   (const SChar*);
typedef SInt       (*idfrename)   (const SChar*, const SChar*);
typedef SInt       (*idfaccess)   (const SChar*, SInt);
typedef SInt       (*idffsync)    (PDL_HANDLE);
typedef SInt       (*idfftruncate)(PDL_HANDLE, PDL_OFF_T);
typedef DIR*       (*idfopendir)  (const SChar*);
typedef SInt       (*idfreaddir_r)(DIR*, struct dirent*, struct dirent**);
typedef void       (*idfclosedir) (DIR*);
typedef PDL_OFF_T  (*idffilesize) (PDL_HANDLE handle);
typedef SLong      (*idfgetDiskFreeSpace)(const SChar*);

typedef struct dirent* (*idfreaddir)(DIR*);

//==============
// class of idf
//==============
class idf
{
private:
public :
    //================================================
    // 멤버 변수
    //     idf::mMaster : 
    //     idf::mFd     : 단일 데이터 파일 descriptor
    //     idf::mLogFd  : 로그 파일 descriptor 
    //================================================
    static idfMaster     mMaster;
    static idfMetaEntry *mMetaList;
    static idfFdEntry   *mFdList;
    static UChar        *mBlockMap; // 전체 블록의 상태 정보를 표시
    static SInt         *mBlockList;
    static UChar        *mEmptyBuf;
    static UInt         *mCRCTable; // 256bytes CRC Table
    static idfLogHeader *mLogHeader;

    static UInt mFileOpenCount;

    static UInt   mTransCount;
    static UChar *mLogList;
    static UInt   mLogListCount;
    static UChar *mLogBuffer;
    static UInt   mLogBufferCursor;

    static idfMapHeader *mMapHeader;
    static UInt         *mMapLog;
    static UInt          mMapLogCount;

    // Meta의 Indirect Page에 접근하기 위해 존재하며, idfPageMap을 관리한다.
    static idfPageMap *mPageMapPool;
    static UInt        mPageMapPoolCount;

    static SChar *mFileName;
    static SChar *mLogFileName;

    static PDL_HANDLE mFd;
    static PDL_HANDLE mLogFd;

    // 설정을 위한 변수
    static UInt mPageNum;
    static UInt mPageSize;
    static UInt mPagesPerBlock;
    static UInt mMaxFileOpenCount;
    static UInt mMaxFileCount;
    static UInt mSystemSize;

    static UInt mDirectPageNum;
    static UInt mIndPageNum;
    static UInt mIndMapSize;
    static UInt mIndEntrySize;
    static UInt mMapPoolSize;

    static SInt mMetaSize;
    static SInt mMasterSize;

    static idBool mIsSync;
    static idBool mIsDirty;

    static UChar mLogMode;

    static UChar mRecCheck;
    
    static PDL_thread_mutex_t  mMutex;
    static PDL_OS::pdl_flock_t mLockFile;

    // 초기화 함수
    static void   initWithIdlOS();
    static void   initWithIdfCore();
    static void   initMasterPage(idfMaster*);
    static IDE_RC initMetaList();
    static IDE_RC initMetaList2();
    static void   initFdList();
    static void   initBlockMap();
    static void   initPageMapPool();
    static void   initMutex();
    static void   initFileName(const SChar *);
    static void   initLockFile();
    static void   initCRCTable();
    static void   initLogList();

    // 복구 함수
    static IDE_RC doRecovery();

    // 종료 함수
    static void   finalMutex();
    static void   finalFiles();
    static void   finalLockFile();

    //=====================================================
    // Desc : call idlOS::open()
    // Arg  : const SChar *, SInt, ...
    //     pathname : file path
    //     flag     : file open mode
    //     ...      : file creation option (optional)
    //=====================================================
    static PDL_HANDLE idlopen(const SChar *pathname, SInt flag, ...);

    //========================
    // 함수 포인터 (I/O APIs)
    //========================
    static idfopen      open;
    static idfclose     close;
    static idfread      read;
    static idfwrite     write;
    static idfreadv     readv;
    static idfwritev    writev;
    static idfpread     pread;
    static idfpwrite    pwrite;
    static idfcreat     creat;
    static idflseek     lseek;
    static idffstat     fstat;
    static idfunlink    unlink;
    static idfrename    rename;
    static idfaccess    access;
    static idffsync     fsync;
    static idfftruncate ftruncate;
    static idfopendir   opendir;
    static idfreaddir_r readdir_r;
    static idfreaddir   readdir;
    static idfclosedir  closedir;
    static idffilesize  filesize;
    static idfgetDiskFreeSpace getDiskFreeSpace;

#if !defined(PDL_LACKS_PREADV)
    static idfpreadv    preadv;
    static idfpwritev   pwritev;
#else
    static ssize_t preadv(PDL_HANDLE, const iovec*, int, PDL_OFF_T);
    static ssize_t pwritev(PDL_HANDLE, const iovec*, int, PDL_OFF_T);
#endif

    //=====================================================
    // Desc : initialize idf (recovery or create log file)
    // Arg  : const SChar*
    //     aFileName : Single Data File Name
    //=====================================================
    static IDE_RC initializeStatic(const SChar *aFileName);

    //=====================================================
    // Desc : return ID_TRUE if idf use VFS
    // Arg  : none
    //=====================================================
    static idBool isVFS();

    //=====================================================
    // Desc : initialize idf (for VFS Utility)
    // Arg  : const SChar*, idfMaster*
    //     aFileName : Single Data File Name
    //     idfMaster : For VFS Options
    //=====================================================
    static IDE_RC initializeStatic2(const SChar *aFileName, idfMaster*);

    static IDE_RC initializeCore(const SChar * aFileName);

    //=====================================================
    // Desc : finalize idf and remove log file
    // Arg  : none
    //=====================================================
    static IDE_RC finalizeStatic();

    //=====================================================
    // Desc : Dump MASTER PAGE
    // Arg  : aMaster 
    //=====================================================
    static void dumpMaster( idfMaster * aMaster );

    //=====================================================
    // Desc : Dump META PAGE
    // Arg  : SInt
    //     aFileDescriptor : File Descriptor for Dump
    //=====================================================
    static void dumpMeta(PDL_HANDLE);
    static void dumpAllMeta();
    static void dumpBlockMap();
    static void dumpMetaP(const SChar*);

    /*
    static IDE_RC validateMaster();
    */

    // 공통 사용 
    static SChar *getpath(const SChar*);
    static IDE_RC getFreePage(UInt*, UInt);
    static SInt   getUnusedFd();
    static SInt   getFileIDByName(const SChar*);
    static SInt   getUnusedFileID();
    static UInt   getPageAddrW(idfMeta*, UInt, SInt);
    static UInt   getPageAddrR(idfMeta*, UInt, SInt);
    static UInt  *getPageMap(SInt, UInt, UInt, UInt);
    static UInt   getCRC(const UChar*, SInt);
    static void   getMetaCRC(idfMeta*);
    static void   getMasterCRC();

    // 디렉토리 관련 함수
    static idfDir *getDir(const SChar*);
    static IDE_RC  isDir(const SChar*);

    // 자원 반환 함수
    static IDE_RC freeFd(SInt);
    static IDE_RC freeMeta(SInt);

    // 시스템 페이지 관련 함수
    static IDE_RC writeMaster();

    // 로그 기록 함수
    static IDE_RC masterLog(UInt);
    static IDE_RC appandPageMapLog(SInt, SInt, SInt);
    static IDE_RC initLog();

    // lock 관련 함수
    static void lock();
    static void unlock();

    static SChar *fdgets(SChar*, SInt, PDL_HANDLE);
    static SInt   fdeof(PDL_HANDLE);

    static void      alloc(void **aBuffer, size_t aSize);
    static void      free(void *aBuffer);
    static ssize_t   writeFs(PDL_HANDLE aFd, const void* aBuf, size_t aCount);
    static ssize_t   readFs(PDL_HANDLE aFd, void *aBuf, size_t aCount);
    static PDL_OFF_T lseekFs(PDL_HANDLE aFd, PDL_OFF_T aOffset, SInt aWhence);
    static SInt       fsyncFs(PDL_HANDLE aFd);
};

//==================
// class of idfCore
//==================
class idfCore
{
private:
public:
    static PDL_HANDLE open     (const SChar*, SInt, ...);
    static SInt       close    (PDL_HANDLE);
#if defined(VC_WINCE)
    static PDL_HANDLE creat    (SChar*, mode_t);
#else
    static PDL_HANDLE creat    (const SChar*, mode_t);
#endif
    static ssize_t    read     (PDL_HANDLE, void*, size_t);
    static ssize_t    write    (PDL_HANDLE, const void*, size_t);
    static PDL_OFF_T  lseek    (PDL_HANDLE, PDL_OFF_T, SInt);
    static SInt       fstat    (PDL_HANDLE, PDL_stat*);
    static SInt       unlink   (const SChar*);
    static SInt       rename   (const SChar*, const SChar*);
    static SInt       access   (const SChar*, SInt);
    static SInt       fsync    (PDL_HANDLE);
    static SInt       ftruncate(PDL_HANDLE, PDL_OFF_T);
    static DIR*       opendir  (const SChar*);
    static SInt       readdir_r(DIR*, struct dirent*, struct dirent**);
    static void       closedir (DIR*);
    static SLong      getDiskFreeSpace(const SChar*);
    static PDL_OFF_T  filesize ( PDL_HANDLE handle );

    static struct dirent* readdir(DIR*);
};

#endif /* _O_IDF_H_ */
