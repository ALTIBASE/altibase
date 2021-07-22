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
 
#include <idu.h>
#include <idf.h>
#include <idfMemory.h>

//========================
// 함수 포인터 (I/O APIs)
//========================
idfopen      idf::open      = idf::idlopen;
idfclose     idf::close     = idlOS::close;
idfread      idf::read      = &idlOS::read;
idfwrite     idf::write     = &idlOS::write;
idfpread     idf::pread     = idlOS::pread;
idfpwrite    idf::pwrite    = idlOS::pwrite;
idfreadv     idf::readv     = idlOS::readv;
idfwritev    idf::writev    = idlOS::writev;
#if !defined(PDL_LACKS_PREADV)
idfpreadv    idf::preadv    = idlOS::preadv;
idfpwritev   idf::pwritev   = idlOS::pwritev;
#endif
idfcreat     idf::creat     = idlOS::creat;
idflseek     idf::lseek     = idlOS::lseek;
idffstat     idf::fstat     = idlOS::fstat;
idfunlink    idf::unlink    = idlOS::unlink;
idfrename    idf::rename    = idlOS::rename;
idfaccess    idf::access    = idlOS::access;
idffsync     idf::fsync     = idlOS::fsync;
idfftruncate idf::ftruncate = idlOS::ftruncate;
idfopendir   idf::opendir   = idlOS::opendir;
idfreaddir_r idf::readdir_r = idlOS::readdir_r;
idfreaddir   idf::readdir   = idlOS::readdir;
idfclosedir  idf::closedir  = idlOS::closedir;
idffilesize  idf::filesize  = idlOS::filesize;
idfgetDiskFreeSpace idf::getDiskFreeSpace = idlVA::getDiskFreeSpace;

//===============================================
// 멤버 변수
//     1. File Handle
//     2. File Name
//     3. System
//     4. Setting
//     5. Mutex
//===============================================

//==============================
// 1-1. File Handle
// 1-2. File Handle for Logging
//==============================
PDL_HANDLE idf::mFd    = PDL_INVALID_HANDLE;
PDL_HANDLE idf::mLogFd = PDL_INVALID_HANDLE;

//============================
// 2-1. File Name
// 2-2. File Name for Logging
//============================
SChar *idf::mFileName    = NULL;
SChar *idf::mLogFileName = NULL;

// 3. System
//===============================================
// Block Map은 1byte가 1개의 Block을 가리킨다.
// 페이지를 할당할 때 Block의 크기만큼 할당한다.
//===============================================
UChar        *idf::mBlockMap = NULL;
SInt         *idf::mBlockList = NULL;
UChar        *idf::mEmptyBuf = NULL;
UInt          idf::mFileOpenCount = 0;
idfMaster     idf::mMaster;
idfFdEntry   *idf::mFdList   = NULL;
idfMetaEntry *idf::mMetaList = NULL;
idfPageMap   *idf::mPageMapPool = NULL;
UInt          idf::mPageMapPoolCount;
UInt         *idf::mCRCTable = NULL; // 256 bytes CRC Table
idfLogHeader *idf::mLogHeader = NULL;

UChar *idf::mLogBuffer = NULL;
UInt   idf::mLogBufferCursor;
UInt   idf::mTransCount;
UChar *idf::mLogList  = NULL;
UInt   idf::mLogListCount;

idfMapHeader *idf::mMapHeader = NULL;
UInt         *idf::mMapLog = NULL;
UInt          idf::mMapLogCount;

UInt idf::mPageNum;
UInt idf::mPageSize;
UInt idf::mPagesPerBlock;
UInt idf::mMaxFileOpenCount;
UInt idf::mMaxFileCount;
UInt idf::mSystemSize;

UInt idf::mDirectPageNum;
UInt idf::mIndPageNum;
UInt idf::mIndMapSize;
UInt idf::mIndEntrySize;
UInt idf::mMapPoolSize;
SInt idf::mMetaSize;
SInt idf::mMasterSize;

idBool idf::mIsSync;
idBool idf::mIsDirty;

UChar idf::mLogMode;

PDL_thread_mutex_t  idf::mMutex;
PDL_OS::pdl_flock_t idf::mLockFile;

idfMemory gAlloca;

UChar idf::mRecCheck = IDF_END_RECOVERY;

void idf::initWithIdlOS()
{
    idf::open      = idf::idlopen;
    idf::close     = idlOS::close;
#if defined(WRS_VXWORKS)
//fix assuming & on overloaded member function error
    idf::read      = &idlOS::read;
    idf::write     = &idlOS::write;
#else
    idf::read      = idlOS::read;
    idf::write     = idlOS::write;
#endif
    idf::readv     = idlOS::readv;
    idf::writev    = idlOS::writev;
#if !defined(PDL_LACKS_PREADV)
    idf::preadv    = idlOS::preadv;
    idf::pwritev   = idlOS::pwritev;
#endif
    idf::creat     = idlOS::creat;
    idf::lseek     = idlOS::lseek;
    idf::fstat     = idlOS::fstat;
    idf::unlink    = idlOS::unlink;
    idf::rename    = idlOS::rename;
    idf::access    = idlOS::access;
    idf::fsync     = idlOS::fsync;
    idf::ftruncate = idlOS::ftruncate;
    idf::opendir   = idlOS::opendir;
    idf::readdir_r = idlOS::readdir_r;
    idf::readdir   = idlOS::readdir;
    idf::closedir  = idlOS::closedir;
#if defined(WRS_VXWORKS)
//fix assuming & on overloaded member function error
    idf::filesize  = &idlOS::filesize;
#else
    idf::filesize  = idlOS::filesize;
#endif
    idf::getDiskFreeSpace = idlVA::getDiskFreeSpace;
}

void idf::initWithIdfCore()
{
    idf::open      = idfCore::open;
    idf::close     = idfCore::close;
    idf::read      = idfCore::read;
    idf::write     = idfCore::write;
    idf::readv     = idlOS::readv;
    idf::writev    = idlOS::writev;
#if !defined(PDL_LACKS_PREADV)
    idf::preadv    = idlOS::preadv;
    idf::pwritev   = idlOS::pwritev;
#endif
    idf::creat     = idfCore::creat;
    idf::lseek     = idfCore::lseek;
    idf::fstat     = idfCore::fstat;
    idf::unlink    = idfCore::unlink;
    idf::rename    = idfCore::rename;
    idf::access    = idfCore::access;
    idf::fsync     = idfCore::fsync;
    idf::ftruncate = idfCore::ftruncate;
    idf::opendir   = idfCore::opendir;
    idf::readdir_r = idfCore::readdir_r;
    idf::readdir   = idfCore::readdir;
    idf::closedir  = idfCore::closedir;
    idf::filesize  = idfCore::filesize;
    idf::getDiskFreeSpace = idfCore::getDiskFreeSpace;

    //===================================================================
    // 파일 설정값을 default로 설정한다.
    // 파일이 존재하고, Master Page가 유효하면 Master의 값으로 변경한다.
    //===================================================================
    idf::mPageNum          = IDF_PAGE_NUM;
    idf::mPageSize         = IDF_PAGE_SIZE;
    idf::mPagesPerBlock    = IDF_PAGES_PER_BLOCK;
    idf::mMaxFileOpenCount = IDF_MAX_FILE_OPEN_COUNT;
    idf::mMaxFileCount     = IDF_MAX_FILE_COUNT;;

    idf::mMapPoolSize      = IDF_MAP_POOL_SIZE;
    idf::mDirectPageNum    = IDF_DIRECT_PAGE_NUM;
    idf::mIndPageNum       = IDF_INDIRECT_PAGE_NUM;
    idf::mIndMapSize       = IDF_INDIRECT_MAP_SIZE;
    idf::mIndEntrySize     = IDF_INDIRECT_ENTRY_SIZE;

    idf::mMetaSize   = ID_SIZEOF(idfMeta);
    idf::mMasterSize = ID_SIZEOF(idfMaster);

    idf::mIsSync = ID_FALSE;
    idf::mIsDirty = ID_FALSE;

    idf::mLogMode = IDF_DEFAULT_LOG_MODE;

    idf::mTransCount   = 0;

    idf::mRecCheck = IDF_END_RECOVERY;
}

void idf::initMasterPage(idfMaster *aMaster)
{
    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        // aMaster가 NULL이면 기본 값으로 idf::mMaster를 설정한다.
        if(aMaster == NULL)
        {
            idf::mMaster.mMajorVersion = IDF_MAJOR_VERSION;
            idf::mMaster.mMinorVersion = IDF_MINOR_VERSION;

            idf::mMaster.mNumOfPages = IDF_PAGE_NUM;
            idf::mMaster.mSizeOfPage = IDF_PAGE_SIZE;
            idf::mMaster.mPagesPerBlock = IDF_PAGES_PER_BLOCK;
            idf::mMaster.mFSMode = 0;

            idf::mMaster.mLogMode = IDF_DEFAULT_LOG_MODE;
            idf::mMaster.mNumOfFiles = 0;
            idf::mMaster.mMaxFileCount = IDF_MAX_FILE_COUNT;
            idf::mMaster.mMaxFileOpenCount = IDF_MAX_FILE_OPEN_COUNT;
            idf::mMaster.mSignature = idlOS::hton(IDF_SYSTEM_SIGN);
        }
        // aMaster가 NULL이 아니면 aMaster값으로 idf::mMaster와
        // idf의 변수를 초기화 한다.
        else
        {
            // 사용자로 부터 받은 Master를 초기화 한다.
            if(&(idf::mMaster) != aMaster)
            {
                idf::mMaster.mMajorVersion = IDF_MAJOR_VERSION;
                idf::mMaster.mMinorVersion = IDF_MINOR_VERSION;

                if(idf::mMaster.mNumOfPages == 0)
                {
                    idf::mMaster.mNumOfPages = IDF_PAGE_NUM;
                }
                if(idf::mMaster.mSizeOfPage == 0)
                {
                    idf::mMaster.mSizeOfPage = idf::mPageSize;
                }

                if((idf::mMaster.mFSMode != 0) && (idf::mMaster.mFSMode != 1))
                {
                    idf::mMaster.mFSMode = 0;
                }

                if((idf::mMaster.mLogMode == IDF_NO_LOG_MODE) ||
                   (idf::mMaster.mLogMode == IDF_LOG_MODE))
                {
                    idf::mMaster.mLogMode = IDF_DEFAULT_LOG_MODE;
                }

                idf::mMaster.mNumOfFiles = 0;

                idf::mMaster.mTimestamp = 0;
                // idf::mMaster.mPagesPerBlock은 4의 배수여야 한다.
                if((idf::mMaster.mPagesPerBlock == 0) ||
                   (idf::mMaster.mPagesPerBlock % 4 != 0))
                {
                    idf::mMaster.mPagesPerBlock = idf::mPagesPerBlock;
                }

                if(idf::mMaster.mMaxFileCount == 0)
                {
                    idf::mMaster.mMaxFileCount = IDF_MAX_FILE_COUNT;
                }
                if(idf::mMaster.mMaxFileOpenCount == 0)
                {
                    idf::mMaster.mMaxFileOpenCount = IDF_MAX_FILE_OPEN_COUNT;
                }

                idf::mMaster.mSignature = idlOS::hton(IDF_SYSTEM_SIGN);
            }

        }

        if((aMaster != NULL) & (&(idf::mMaster) == aMaster))
        {   
            idf::mMaster.mNumOfFiles       = idlOS::ntoh(idf::mMaster.mNumOfFiles);
            idf::mMaster.mMajorVersion     = idlOS::ntoh(idf::mMaster.mMajorVersion);
            idf::mMaster.mMinorVersion     = idlOS::ntoh(idf::mMaster.mMinorVersion);
            idf::mMaster.mAllocedPages     = idlOS::ntoh(idf::mMaster.mAllocedPages);
            idf::mMaster.mNumOfPages       = idlOS::ntoh(idf::mMaster.mNumOfPages);
            idf::mMaster.mSizeOfPage       = idlOS::ntoh(idf::mMaster.mSizeOfPage);
            idf::mMaster.mPagesPerBlock    = idlOS::ntoh(idf::mMaster.mPagesPerBlock);
            idf::mMaster.mMaxFileOpenCount = idlOS::ntoh(idf::mMaster.mMaxFileOpenCount);
            idf::mMaster.mMaxFileCount     = idlOS::ntoh(idf::mMaster.mMaxFileCount);
        }
        
        // 단일 데이타 파일 생성시 Master 시스템 정보 초기화.
        idf::mPageNum          = idf::mMaster.mNumOfPages;
        idf::mPageNum          = idf::mMaster.mNumOfPages;
        idf::mPageSize         = idf::mMaster.mSizeOfPage;
        idf::mPagesPerBlock    = idf::mMaster.mPagesPerBlock;
        idf::mMaxFileOpenCount = idf::mMaster.mMaxFileOpenCount;
        idf::mMaxFileCount     = idf::mMaster.mMaxFileCount;
        idf::mIndMapSize       = idf::mMaster.mSizeOfPage / ID_SIZEOF(UInt);
        idf::mIndEntrySize     = ID_SIZEOF(UInt) * idf::mMaster.mPagesPerBlock;
        idf::mLogMode          = idf::mMaster.mLogMode;

        idf::mSystemSize = IDF_FIRST_META_POSITION;

        idf::mSystemSize += idf::mMaxFileCount * IDF_META_SIZE;

        if((idf::mSystemSize % idf::mPageSize) == 0)
        {
            idf::mSystemSize = (idf::mSystemSize / idf::mPageSize);
        }
        else
        {
            idf::mSystemSize = (idf::mSystemSize / idf::mPageSize) + 1;
        }
        if((idf::mSystemSize % idf::mPagesPerBlock) == 0)
        {
            idf::mSystemSize = (idf::mSystemSize / idf::mPagesPerBlock);
        }
        else
        {
            idf::mSystemSize = (idf::mSystemSize / idf::mPagesPerBlock) + 1;
        }

        if((aMaster == NULL) || (&(idf::mMaster) != aMaster))
        {
            idf::mMaster.mAllocedPages = idf::mSystemSize * idf::mPagesPerBlock;
        }
    }
}

//===================================================================
// File Descriptor 리스트를 초기화 한다.
// 파일 시스템이 초기화 되면 FD는 모두 사용하지 않은 상태이기 때문에
// 모두 0으로 초기화 한다.
//===================================================================
void idf::initFdList()
{
    UInt sIndex;
    UInt sFdListSize = (UInt)(ID_SIZEOF(idfFdEntry) * idf::mMaxFileOpenCount);

    (void)idf::alloc((void **)&idf::mFdList, sFdListSize);

    for(sIndex = 0; sIndex < idf::mMaxFileOpenCount; sIndex++)
    {
        idf::mFdList[sIndex].mIsUsed = IDF_FD_UNUSED;
        idf::mFdList[sIndex].mFileID = IDF_INVALID_FILE_ID;
        idf::mFdList[sIndex].mCursor = 0;
    }
}

//============================
// Meta를 읽어서 초기화 한다.
//============================
IDE_RC idf::initMetaList()
{
    UInt     sIndex;
    UInt     sOffsetIndex;
    UInt     sMapIndex;
    UInt     sPageIndex;
    UInt     sBlockIndex;
    UInt     sBlockOffset;
    UInt     sCount  = 0;
    UInt    *sMap    = NULL;
    UInt    *sIndMap = NULL;
    idfMeta *sMeta   = NULL;
    UInt     sNum;

    (void)idf::alloc((void **)&idf::mMetaList, 
                     ID_SIZEOF(idfMetaEntry) * idf::mMaxFileCount);

    (void)idf::alloc((void **)&sMeta, idf::mMetaSize);

    (void)idf::alloc((void **)&sIndMap, idf::mPageSize);

    for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
    {
        IDE_TEST(idf::lseekFs(idf::mFd,
                              IDF_FIRST_META_POSITION + sIndex * IDF_META_SIZE,
                              SEEK_SET) == -1);

        IDE_TEST(idf::readFs(idf::mFd, 
                             sMeta, 
                             idf::mMetaSize) < idf::mMetaSize);

        sMeta->mFileID      = idlOS::ntoh(sMeta->mFileID);
        sMeta->mSize        = idlOS::ntoh(sMeta->mSize);
        sMeta->mCTime       = idlOS::ntoh(sMeta->mCTime);
        sMeta->mNumOfPagesU = idlOS::ntoh(sMeta->mNumOfPagesU);
        sMeta->mNumOfPagesA = idlOS::ntoh(sMeta->mNumOfPagesA);
        sMeta->mCRC         = idlOS::ntoh(sMeta->mCRC);        

        for(sNum = 0; sNum < IDF_PAGES_PER_BLOCK; sNum++)
        {
            sMeta->mDirectPages[sNum] = idlOS::ntoh(sMeta->mDirectPages[sNum]);
        }

        for(sNum = 0; sNum < IDF_INDIRECT_PAGE_NUM; sNum++)
        {
            sMeta->mIndPages[sNum] = idlOS::ntoh(sMeta->mIndPages[sNum]);
        }

        if(sMeta->mSignature != idlOS::ntoh(IDF_SYSTEM_SIGN))
        {
            idf::mMetaList[sIndex].mMeta = NULL;

            continue;
        }

        idf::mMetaList[sIndex].mMeta = sMeta;
        idf::mMetaList[sIndex].mFileOpenCount = 0;

        sMap = &(sMeta->mDirectPages[0]);

        //======================
        // Direct Page Map 검사
        //======================
        for(sMapIndex = 0;
            sMapIndex < idf::mDirectPageNum;
            sMapIndex += idf::mPagesPerBlock)
        {
            sBlockIndex = sMap[sMapIndex] & IDF_FILE_HOLE_INVERTER;

            sBlockIndex /= idf::mPagesPerBlock;

            if(sBlockIndex != 0)
            {
                IDE_ASSERT(idf::mBlockMap[sBlockIndex] == 0x00);

                idf::mBlockMap[sBlockIndex] = 0xFF;

                idf::mBlockList[sBlockIndex] = sIndex;
            }
        }

        //========================
        // Indirect Page Map 검사
        //========================

        sMap = &(sMeta->mIndPages[0]);

        for(sMapIndex = 0; sMapIndex < idf::mIndPageNum; sMapIndex++)
        {
            if(sMap[sMapIndex] == 0)
            {
                continue;
            }

            // Indirect Map이 할당되어 있다.
            sPageIndex = sMap[sMapIndex] & IDF_FILE_HOLE_INVERTER;

            sBlockOffset = sPageIndex % idf::mPagesPerBlock;

            sBlockIndex = sPageIndex / idf::mPagesPerBlock;

            if(sBlockIndex != 0 && sBlockOffset == 0)
            {
                IDE_ASSERT(idf::mBlockMap[sBlockIndex] == 0x00);

                idf::mBlockMap[sBlockIndex] = 0xFF;

                idf::mBlockList[sBlockIndex] = sIndex;
            }

            // Indirect Map을 사용하지 않은 경우 다음 Map을 조사한다.
            if((sMap[sMapIndex] & IDF_FILE_HOLE_MASK) != 0)
            {
                continue;
            }

            // Indirect Map을 읽어서 Block Map을 초기화 한다.
            IDE_TEST(idf::lseekFs(idf::mFd,
                                  sPageIndex * idf::mPageSize,
                                  SEEK_SET) == -1);

            IDE_TEST(idf::readFs(idf::mFd,
                                 sIndMap,
                                 idf::mPageSize) != idf::mPageSize);

            for(sOffsetIndex = 0;
                sOffsetIndex < idf::mIndMapSize;
                sOffsetIndex += idf::mPagesPerBlock)
            {
                sIndMap[sOffsetIndex] = idlOS::ntoh(sIndMap[sOffsetIndex]);
            }

            for(sOffsetIndex = 0;
                sOffsetIndex < idf::mIndMapSize;
                sOffsetIndex += idf::mPagesPerBlock)
            {
                if(sIndMap[sOffsetIndex] == 0)
                {
                    continue;
                }

                sBlockIndex = sIndMap[sOffsetIndex] & IDF_FILE_HOLE_INVERTER;

                sBlockIndex /= idf::mPagesPerBlock;

                if(sBlockIndex != 0)
                {
                    IDE_ASSERT(idf::mBlockMap[sBlockIndex] == 0x00);

                    idf::mBlockMap[sBlockIndex] = 0xFF;

                    idf::mBlockList[sBlockIndex] = sIndex;
                }
            }
        }

        (void)idf::alloc((void **)&sMeta, idf::mMetaSize);

        sCount++;
    }

    // 읽은 Meta의 개수와 Master에 기록된 Meta의 개수가 다르면 에러
    IDE_TEST(sCount != idf::mMaster.mNumOfFiles);

    (void)idf::free(sIndMap);

    (void)idf::free(sMeta);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sIndMap != NULL)
    {
        (void)idf::free(sIndMap);
    }

    if(sMeta != NULL)
    {
        (void)idf::free(sMeta);
    }

    return IDE_FAILURE;
}

//===========================================================
// Meta를 초기화 한다.
// initMetaList는 Meta를 읽어서 초기화 하지만,
// initMetaList2는 맨 처음 파일시스템을 생성할 때 호출하므로
// 모두 0으로 초기화 한다.
//===========================================================
IDE_RC idf::initMetaList2()
{
    UInt   sIndex;
    UInt   sSize;
    UInt   sSystemSize;

    sSize = ID_SIZEOF(idfMetaEntry) * idf::mMaxFileCount;
    
    (void)idf::alloc((void **)&idf::mMetaList, sSize);

    for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
    {
        idf::mMetaList[sIndex].mMeta = NULL;
        idf::mMetaList[sIndex].mFileOpenCount = 0;
    }

    sSystemSize = idf::mSystemSize * idf::mPagesPerBlock;

    IDE_TEST(idf::lseekFs(mFd,
                          0,
                          SEEK_SET) == -1);

    for(sIndex = 0; sIndex < sSystemSize; sIndex++)
    {
        IDE_TEST(idf::writeFs(mFd,
                              idf::mEmptyBuf,
                              idf::mPageSize) != idf::mPageSize);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//=======================================================================
// initFileName()은 가상 파일시스템의 이름과 로그파일의 이름을 저장한다.
// 이 함수는 initializeStatic() 함수에서만 호출된다.
// 따라서 메모리 할당 중에 죽는 경우 메모리 해제를 initializeStatic()
// 함수에서 처리한다.
//=======================================================================
void idf::initFileName(const SChar * aFileName)
{
    SInt sFileNameLen = idlOS::strlen(aFileName) + 1;

    idf::alloc((void **)&idf::mFileName, sFileNameLen);
    
    idlOS::strncpy(idf::mFileName, 
                   aFileName, 
                   sFileNameLen);

    // aFileName에 '.log'를 붙여 log file 이름을 만든다.
    sFileNameLen += 4;

    idf::alloc((void **)&idf::mLogFileName, sFileNameLen);

    idlOS::snprintf(idf::mLogFileName, sFileNameLen, "%s.log", aFileName);
}

void idf::initMutex()
{
    IDE_ASSERT(idlOS::thread_mutex_init(&idf::mMutex) == 0);
}

void idf::initPageMapPool()
{
    UInt sIndex;
    UInt sSize;
    idfPageMap *sPageMap;

    sSize = ID_SIZEOF(idfPageMap);

    (void)idf::alloc((void **)&idf::mPageMapPool, sSize);

    idf::mPageMapPool->mMap = NULL;
    idf::mPageMapPool->mFileID = IDF_INVALID_FILE_ID;

    (void)idf::alloc((void **)&sPageMap, sSize);

    sPageMap->mMap = NULL;
    sPageMap->mFileID = IDF_INVALID_FILE_ID;

    sPageMap->mPrev = idf::mPageMapPool;
    sPageMap->mNext = idf::mPageMapPool;

    idf::mPageMapPool->mNext = sPageMap;
    idf::mPageMapPool->mPrev = sPageMap;

    for(sIndex = 2; sIndex < idf::mMapPoolSize; sIndex++)
    {
        (void)idf::alloc((void **)&sPageMap, sSize);

        sPageMap->mMap = NULL;
        sPageMap->mFileID = IDF_INVALID_FILE_ID;

        sPageMap->mNext = idf::mPageMapPool->mNext;
        sPageMap->mPrev = idf::mPageMapPool;

        idf::mPageMapPool->mNext->mPrev = sPageMap;
        idf::mPageMapPool->mNext = sPageMap;
    }

    idf::mPageMapPoolCount = 0;
}

//======================================================================
// initBlockMap은 블록 개수만큼 리스트를 만들고 0으로 초기화 한다.
// Block Map은 이 함수에서는 생성만 하고, initMeta에서 Meta를 읽으면서
// 사용한 블록에 대해서 체크를 한다.
// 시스템이 사용한 영역(Master + Meta)만 initBlockMap에서 사용한 것으로
// 표시한다.
//======================================================================
void idf::initBlockMap()
{
    SInt sMapSize = idf::mPageNum / idf::mPagesPerBlock;

    (void)idf::alloc((void **)&idf::mBlockMap, sMapSize);

    idlOS::memset(idf::mBlockMap, 
                  0x00, 
                  sMapSize);

    (void)idf::alloc((void**)&idf::mBlockList, sMapSize * ID_SIZEOF(SInt));

    idlOS::memset(idf::mBlockList,
                  0xFF,
                  sMapSize * ID_SIZEOF(SInt));

    //================================================================
    // System Page 크기 만큼은 사용한 것으로 설정하여 데이터 페이지에
    // 할당하지 못하도록 한다.
    //================================================================
    idlOS::memset(idf::mBlockMap, 
                  0xFF, 
                  idf::mSystemSize);
}

void idf::initLogList()
{
    (void)idf::alloc((void **)&idf::mLogBuffer, IDF_LOG_BUFFER_SIZE);

    (void)idf::alloc((void **)&idf::mLogList, idf::mMaxFileCount);

    (void)idf::alloc((void **)&idf::mMapHeader, 
                     ID_SIZEOF(idfMapHeader) * IDF_PAGE_MAP_LOG_SIZE);

    (void)idf::alloc((void **)&idf::mMapLog, 
                     idf::mIndEntrySize * IDF_PAGE_MAP_LOG_SIZE);

    idlOS::memset(idf::mLogList, 
                  0x00, 
                  idf::mMaxFileCount);

    idf::mLogBufferCursor = 0;

    idf::mLogListCount = 0;

    idf::mMapLogCount = 0;
}

//========================================================================
// open()에서 파일을 열 때 FD 리스트를 순회하여 빈 FD를 찾아 반환해 준다.
//========================================================================
SInt idf::getUnusedFd()
{
    UInt sIndex;
    SInt sFd = IDF_INVALID_HANDLE;

    if(idf::mFileOpenCount < idf::mMaxFileOpenCount)
    {   
        for(sIndex = 0; sIndex < idf::mMaxFileOpenCount; sIndex++)
        {
            if(idf::mFdList[sIndex].mIsUsed == IDF_FD_UNUSED)
            {
                sFd = sIndex;

                idf::mFdList[sIndex].mIsUsed = IDF_FD_USED;
                idf::mFileOpenCount++;

                break;
            }
        }
    }   

    return sFd;
}

//=====================================
// 파일의 이름으로 파일의 ID를 얻는다.
// 파일 ID와 Meta는 1:1로 매칭된다.
//=====================================
SInt idf::getFileIDByName(const SChar *aPathName)
{
    UInt   sIndex;
    UInt   sPathNameLen;
    UInt   sCount = 0;
    SInt   sFileID = IDF_INVALID_FILE_ID;
    SChar *sPathName = NULL;

    sPathName = getpath(aPathName);

    IDE_TEST(sPathName == NULL);

    sPathNameLen = idlOS::strlen(sPathName);

    for(sIndex = 0;
        (sIndex < idf::mMaxFileCount) && (sCount < idf::mMaster.mNumOfFiles);
        sIndex++)
    {
        if(idf::mMetaList[sIndex].mMeta == NULL)
        {
            continue;
        }

        sCount++;

        if((idlOS::strncmp(sPathName,
                           idf::mMetaList[sIndex].mMeta->mFileName,
                           sPathNameLen))
           == 0)
        {
            if(sPathNameLen == 
                    idlOS::strlen(idf::mMetaList[sIndex].mMeta->mFileName))
            {
                sFileID = sIndex;
                break;
            }
        }
    }

    if(sPathName != NULL)
    {
        (void)idf::free(sPathName);
    }

    return sFileID;

    IDE_EXCEPTION_END;

    if(sPathName != NULL)
    {
        (void)idf::free(sPathName);
    }

    return IDF_INVALID_FILE_ID;
}

SInt idf::getUnusedFileID()
{
    UInt sIndex;
    SInt sFileID = IDF_INVALID_FILE_ID;

    for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
    {
        if(idf::mMetaList[sIndex].mMeta == NULL)
        {
            sFileID = sIndex;

            break;
        }
    }

    return sFileID;
}

IDE_RC idf::getFreePage(UInt *aMap, UInt aFileID)
{
    UInt sIndex;
    UInt sMax;
    UInt sPageIndex;
    UInt sTotalBlocks = idf::mPageNum / idf::mPagesPerBlock;

    IDE_TEST(aMap == NULL);

    // free 페이지를 얻어올 때 시스템 페이지는 검사하지 않는다.
    for(sIndex = idf::mSystemSize; sIndex < sTotalBlocks; sIndex++)
    {
        if(idf::mBlockMap[sIndex] == 0x00)
        {
            // 빈 페이지가 있으면 할당한다.
            idf::mBlockMap[sIndex] = 0xFF;

            idf::mBlockList[sIndex] = aFileID;

            idf::mMaster.mAllocedPages += idf::mPagesPerBlock;

            sIndex *= idf::mPagesPerBlock;

            for(sPageIndex = 0;
                sPageIndex < idf::mPagesPerBlock;
                sPageIndex++)
            {
                aMap[sPageIndex] = ((sIndex + sPageIndex) | IDF_FILE_HOLE_MASK);
            }

            sIndex /= idf::mPagesPerBlock;

            break;
        }
    }

    // 빈 페이지가 없는 경우 No space error를 발생한다.
    IDE_TEST_RAISE(sIndex == sTotalBlocks, nospace_error);

    sIndex = sIndex * idf::mPagesPerBlock * idf::mPageSize;

    IDE_TEST(idf::lseekFs(idf::mFd, 
                          sIndex, 
                          SEEK_SET) == -1);

    sMax = idf::mPagesPerBlock * idf::mPageSize / EMPTY_BUFFER_SIZE;

    for(sIndex = 0; sIndex < sMax; sIndex++)
    {
        IDE_TEST(idf::writeFs(idf::mFd, 
                              idf::mEmptyBuf, 
                              EMPTY_BUFFER_SIZE) != EMPTY_BUFFER_SIZE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(nospace_error)
    {
        // 빈 페이지가 없는 경우 No space error를 발생한다.
        errno = ENOSPC; // No space left on device
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SChar *idf::getpath(const SChar *aPathName)
{
    UInt   sIndex = 0;
    SChar *sPathName;
    SChar *sNewName;
    SChar *sDiv;

    sPathName = (SChar*)idlOS::strstr(aPathName, "altibase_home/");

#if defined(VC_WIN32)
    if(sPathName == NULL)
    {
        sPathName = (SChar*)idlOS::strstr(aPathName, "altibase_home\\");
    }
#endif // VC_WIN32

    if(sPathName == NULL)
    {
        sPathName = (SChar*)aPathName;
    }
    else
    {
        sPathName += 14; // idlOS::strlen("altibase_home\") == 14
    }

    //=======================================================================
    // sNewName은 getpath()함수에서 메모리를 해제하지 않는다.
    // 메모리 해제는 이 함수를 호출한 함수에서 다 사용한 후에 해제해야 한다.
    //=======================================================================
    (void)idf::alloc((void **)&sNewName, IDF_MAX_FILE_NAME_LEN);

    // 제일 앞에 오는 '.', '/', '\\'를 제거할때 사용한다.
#if defined(VC_WIN32)
    sDiv = (SChar*)"./\\";
#else
    sDiv = (SChar*)"./";
#endif // VC_WIN32

    // 제일 앞에 오는 '.', '/', '\\'를 제거한다.
    while(idlOS::strchr(sDiv, *sPathName) != NULL)
    {
        sPathName++;
    }

    // path의 중간에 반복하여 나타나는 '/', '\\'를 제거할때 사용한다.
#if defined(VC_WIN32)
    sDiv = (SChar*)"/\\";
#else
    sDiv = (SChar*)"/";
#endif // VC_WIN32

    do
    {
        // path의 중간에 반복하여 나타나는 '/', '\\'를 제거한다.
        if(idlOS::strchr(sDiv, *sPathName) != NULL)
        {
            while((*(sPathName + 1) != '\0') &&
                  (idlOS::strchr(sDiv, *(sPathName + 1)) != NULL))
            {
                sPathName++;
            }
        }

#if defined(VC_WIN32)
        // Windows계열의 OS에서 '/'문자를 '\\'로 통일한다.
        if(*sPathName == '/')
        {
            sNewName[sIndex] = '\\';
        }
        else
        {
            sNewName[sIndex] = *sPathName;
        }
#else
        sNewName[sIndex] = *sPathName;
#endif // VC_WIN32

        sIndex++;
        sPathName++;

        if(sIndex == IDF_MAX_FILE_NAME_LEN - 1)
        {
            break;
        }
    } while(*sPathName != '\0');

    // path의 제일 마지막에 위치한 '/', '\\'문자를 삭제한다.
    if(idlOS::strchr(sDiv, sNewName[sIndex - 1]) != NULL)
    {
        sNewName[sIndex -1] = '\0';
    }
    else
    {
        sNewName[sIndex] = '\0';
    }

    return sNewName;
}

//=======================================================================
// 파일의 Indirect Page Map을 가져온다.
// 우선 Indirect Page Map Pool을 검색하여 Indirect page를 찾고,
// Indirect Page Map이 Pool에 없으면 디스크에서 읽는다.
//
//     aFileID       : File ID
//     aIndex        : 몇 번째 Indrect Page Map인지를 나타낸다.
//     aMapIndex     : Indirect Page Map의 몇 번째 페이지인지 나타낸다.
//     aLogicalPage  : Indirect Page Map을 저장하고 있는 페이지 번호.
//
//
// * Indirect Page Map Pool의 구성
//     각 Page Map은 Double Linked List로 연결된다.
//
//  ------------------- 
// | idf::mPageMapPool |
//  -------------------
//         |
//         v
//     ----------       ----------       ----------             ----------
//    | Page Map | <-> | Page Map | <-> | Page Map | <- ... -> | Page Map |
//     ----------       ----------       ----------             ----------
//         ^                                                        ^
//         |________________________________________________________|
//
//=======================================================================
UInt *idf::getPageMap(SInt aFileID,   UInt aIndex,
                      UInt aMapIndex, UInt aLogicalPage)
{
    // 1. idf::mPageMapPool에서 검색하여 페이지가 존재하는지 확인한다. 
    // 2. idf::mPageMapPool에 없으면 LRU 알고리즘으로 Page Map을 가져온다.

    UInt        sIndex;
    UInt       *sMap;
    UInt        sFind = 0;
    idfPageMap *sPageMap;
    UInt        sCount;

    sPageMap = idf::mPageMapPool;

    for(sIndex = 0; sIndex < idf::mMapPoolSize; sIndex++)
    {
        if((sPageMap->mFileID == aFileID) &&
           (sPageMap->mPageIndex == aIndex) &&
           (sPageMap->mMapIndex == aMapIndex))
        {
            // PageMap을 찾은 경우 찾은 노드를 제일 앞으로 이동한다. (LRU)
            if(sIndex != 0)
            {
                if(sPageMap != idf::mPageMapPool)
                {
                    sPageMap->mPrev->mNext = sPageMap->mNext;
                    sPageMap->mNext->mPrev = sPageMap->mPrev;

                    // 아래의 순서가 변경되면 안된다.
                    idf::mPageMapPool->mPrev->mNext = sPageMap;

                    sPageMap->mPrev = idf::mPageMapPool->mPrev;

                    idf::mPageMapPool->mPrev = sPageMap;

                    sPageMap->mNext = idf::mPageMapPool;

                    idf::mPageMapPool = sPageMap;
                }
            }

            sMap = idf::mPageMapPool->mMap;

            sFind = 1;

            break;
        }

        sPageMap = sPageMap->mNext;
    }

    if(sFind == 0)
    {
        // PageMap을 찾지 못한 경우
        // PageMapPool이 다 차지 않은 경우 PageMapPoolCount를 증가한다.
        // PageMap을 shift하여 가장 오래동안 사용하지 않은 PageMap을
        // 초기화하고 사용하도록 한다.
        if(idf::mPageMapPoolCount != idf::mMapPoolSize)
        {
            idf::mPageMapPoolCount++;
        }

        idf::mPageMapPool = idf::mPageMapPool->mPrev;

        // PageMap이 존재하지 않으면 PageMap을 새로 할당한다.
        if(mPageMapPool->mMap == NULL)
        {
            (void)idf::alloc((void **)&sMap, idf::mIndEntrySize);
        }
        else
        {
            sMap = idf::mPageMapPool->mMap;
        }

        idlOS::memset(sMap, 
                      0x00, 
                      idf::mIndEntrySize);

        idf::mPageMapPool->mFileID = aFileID;
        idf::mPageMapPool->mPageIndex = aIndex;
        idf::mPageMapPool->mMapIndex = aMapIndex;
        idf::mPageMapPool->mMap = sMap;
    }

    // Logical Page가 0이면 할당한 페이지가 없으므로 0을 반환한다.
    // Logical Page가 0이 아니고 Map Pool에 없으면 우선 Page Map Log Pool에서
    // 검색하여 해당 Page Map이 있는지 검사한다.
    // Page Map Log Pool에도 해당 Page Map이 없으면 디스크에서 Page Map을
    // 읽어서 반환한다.
    if((!sFind) && (aLogicalPage != 0))
    {
        idfMapHeader *sMapHeader = NULL;

        for(sIndex = (idf::mMapLogCount - 1); (SInt)sIndex >= 0; sIndex--)
        {
            sMapHeader = &(idf::mMapHeader[sIndex]);

            if((sMapHeader->mFileID == aFileID) &&
               (sMapHeader->mIndex == aLogicalPage) &&
               (sMapHeader->mMapIndex == aMapIndex))
            {
                sFind = 1;

                idlOS::memcpy((void*)sMap, 
                              (void*)(&idf::mMapLog[(idf::mIndEntrySize * sIndex)]), 
                              idf::mIndEntrySize);
                break;
            }
        }

        if(!sFind)
        {
            IDE_TEST(idf::lseekFs(idf::mFd,
                                  ((aLogicalPage * idf::mPageSize) + (aMapIndex * ID_SIZEOF(UInt))),
                                  SEEK_SET) == -1);

            IDE_TEST(idf::readFs(idf::mFd, 
                                 sMap, 
                                 idf::mIndEntrySize) != idf::mIndEntrySize);

            for(sCount = 0; sCount < idf::mPagesPerBlock; sCount++)
            {
                sMap[sCount] = idlOS::ntoh(sMap[sCount]);
            }
        }
    }

    return sMap;

    IDE_EXCEPTION_END;

    return NULL;
}

UInt idf::getPageAddrW(idfMeta *aMeta, UInt aFileID, SInt aPageIndex)
{
    UInt  sLogicalPage = 0;
    SInt  sPageOffset = aPageIndex % idf::mPagesPerBlock;
    UInt *sPage;
    UInt *sMap;
    UInt  sIndPage;
    UInt  sIndIndex = 0;
    UInt  sIndOffset;
    UInt  sMapIndex;
    UInt  sMapOffset = 0;

    if(aPageIndex < IDF_DIRECT_MAP_SIZE)
    {
        // cursor가 Direct Page Map 내에 있는 경우
        // Direct Page Map의 aPageIndex에 위치한 Logical Page가 Page Addr이다.
        sPage = &(aMeta->mDirectPages[aPageIndex]);

        // sMap은 페이지를 할당하기 위해 getFreePage를 호출할 때 사용한다.
        sMap = &(aMeta->mDirectPages[aPageIndex - sPageOffset]);

        sMapIndex = 0;
    }
    else
    {
        //==========================================================
        // cursor가 Indirect Page Map 위치에 있는 경우
        //
        // sIndIndex  : Indirect Map의 Index
        // sIndOffset : Indirect Map의 Offset
        //              (Indirect Map을 할당할 때 사용)
        // sMapIndex  : Inidirect Map 내의 Index
        // sMapOffset : Indirect Map 내의 Offset
        //              (Indirect Map 내에 페이지를 할당할 때 사용)
        //
        // ex>
        // Indirect Map 0 [ ... ] <- sIndOffset
        // Indirect Map 1 [ ... ] <- sIndIndex
        //                   |_______________
        //   ...                            |
        //                                  |
        // Indirect Map n [ ... ]           |
        //         _________________________|
        //        |
        //        v
        // Indirect Map #1
        // [index 0] [index 1] [index 2] [[index 3]
        //  ^^ sMapOffset       ^^ sMapIndex
        //   ...
        //==========================================================
        sIndIndex  = (aPageIndex - IDF_DIRECT_MAP_SIZE) / idf::mIndMapSize;
        sIndOffset = sIndIndex - (sIndIndex % idf::mPagesPerBlock);
        sMapIndex  = (aPageIndex - IDF_DIRECT_MAP_SIZE) % idf::mIndMapSize;
        sMapOffset = sMapIndex - (sMapIndex % idf::mPagesPerBlock);

        // Indirect Page Map의 범위를 벗어나면 에러를 발생한다.
        IDE_TEST_RAISE(sIndIndex >= idf::mIndPageNum, toolarge_error);

        // Meta의 IndirectPage를 조사하여 존재하는지 확인한다.
        // IndirectPage가 존재하면 Indirect Page Map을 읽을 수 있도록
        // getPageMap의 인자로 넘겨준다.
        if(aMeta->mIndPages[sIndIndex] & IDF_FILE_HOLE_MASK)
        {
            sIndPage = 0;

            aMeta->mIndPages[sIndIndex] &= IDF_FILE_HOLE_INVERTER;

            idf::mIsDirty = ID_TRUE;
        }
        else
        {
            sIndPage = aMeta->mIndPages[sIndIndex];
        }

        // 페이지 맵 Pool에서 페이지 맵을 얻어온다.
        // aMeta->mIndPages[sIndIndex]가 0이면
        // 0으로 채운 배열을 반환한다.
        sMap = getPageMap(aFileID, sIndIndex, sMapOffset, sIndPage); 

        IDE_TEST(sMap == NULL);

        // aMeta->mIndPages[sIndIndex]이 0이면
        // Indirect Page Map이 존재하지 않는 것이므로,
        // Indirect Page Map을 저장할 페이지를 얻어온다.
        if(aMeta->mIndPages[sIndIndex] == 0)
        {
            IDE_TEST(idf::getFreePage(&(aMeta->mIndPages[sIndOffset]), aFileID)
                     != IDE_SUCCESS);

            aMeta->mIndPages[sIndIndex] &= IDF_FILE_HOLE_INVERTER;

            idf::mIsDirty = ID_TRUE;
        }

        sPage = &(sMap[sMapIndex - sMapOffset]);
    }

    if(*sPage == 0)
    {
        // Data를 저장할 Logical Address를 할당하지 않았으므로
        // Page를 할당해 준다.
        IDE_TEST(idf::getFreePage(sMap, aFileID) != IDE_SUCCESS);

        aMeta->mNumOfPagesA += idf::mPagesPerBlock;

        idf::mIsDirty = ID_TRUE;
    }

    if(*sPage & IDF_FILE_HOLE_MASK)
    {
        *sPage &= IDF_FILE_HOLE_INVERTER;

        if(aPageIndex >= IDF_DIRECT_MAP_SIZE)
        {
            // aPageIndex가 Indirect Page Map에 속해 있는 경우
            IDE_TEST(idf::appandPageMapLog(aFileID, sIndIndex, sMapOffset)
                     != IDE_SUCCESS);
        }

        aMeta->mNumOfPagesU++;

        idf::mIsDirty = ID_TRUE;
    }

    sLogicalPage = *sPage;

    return sLogicalPage;

    IDE_EXCEPTION(toolarge_error)
    {
        // 파일의 크기가 너무 큰 경우 File Too Large error를 발생한다.
        errno = EFBIG; // File too large
    }

    IDE_EXCEPTION_END;

    return ((UInt)(-1));
}

UInt idf::getPageAddrR(idfMeta *aMeta, UInt aFileID, SInt aPageIndex)
{
    UInt  sLogicalPage = 0;
    UInt  sIndIndex = 0;
    UInt  sMapIndex = 0;
    UInt  sMapOffset;
    UInt  sIndPage;
    UInt *sMap;

    if(aPageIndex < IDF_DIRECT_MAP_SIZE)
    {
        // cursor가 Direct Page Map 내에 있는 경우
        sLogicalPage = aMeta->mDirectPages[aPageIndex];

        // sLogicalPage가 0이거나 IDF_FILE_HOLE_MASK와 & 연산을 했을때
        // 0이 아니면 File Hole이다.
        if(sLogicalPage & IDF_FILE_HOLE_MASK)
        {
            // File Hole
            sLogicalPage = 0;
        }
    }
    else
    {
        // cursor가 Indirect Page Map 위치에 있는 경우
        sIndIndex  = (aPageIndex - IDF_DIRECT_MAP_SIZE) / idf::mIndMapSize;
        sMapIndex  = (aPageIndex - IDF_DIRECT_MAP_SIZE) % idf::mIndMapSize;
        sMapOffset = sMapIndex - (sMapIndex % idf::mPagesPerBlock);

        IDE_TEST(idf::mIndPageNum < sIndIndex);

        // Meta의 IndirectPage를 조사하여 존재하는지 확인한다.
        sIndPage = aMeta->mIndPages[sIndIndex];

        // 존재하지 않거나 File Hole인 경우 0을 반환한다.
        if((sIndPage == 0) || (sIndPage & IDF_FILE_HOLE_MASK))
        {
            sLogicalPage = 0;
        }
        else
        {
            // Indirect Page가 0이 아닌 경우 page map을 얻어온다.
            sMap = getPageMap(aFileID,
                              sIndIndex,
                              sMapOffset,
                              aMeta->mIndPages[sIndIndex]);

            IDE_TEST(sMap == NULL);

            sLogicalPage = sMap[sMapIndex - sMapOffset];

            if(sLogicalPage & IDF_FILE_HOLE_MASK)
            {
                sLogicalPage = 0;
            }
        }
    }

    return sLogicalPage;

    IDE_EXCEPTION_END;

    return ((UInt)(-1));
}

UInt idf::getCRC(const UChar *aBuf, SInt aSize)
{         
    UInt sCRC = 0;
        
    while(aSize--) 
    {        
        sCRC = idf::mCRCTable[(sCRC ^ *(aBuf++)) & 0xFF] ^ (sCRC >> 8);
    } 
    
    return sCRC; 
}   

void idf::initCRCTable()
{   
    UInt i, j, k; 
    UInt id = 0xEDB88320; 
   
    (void)idf::alloc((void **)&idf::mCRCTable, ID_SIZEOF(UInt) * 256);

    for(i = 0; i < 256; ++i) {
        k = i;
        for(j = 0; j < 8; ++j) {
            if (k & 1) k = (k >> 1) ^ id;
            else k >>= 1;
        }
        idf::mCRCTable[i] = k;
    }
}

void idf::dumpMaster(idfMaster * aMaster)
{
    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        SLong  sTotal = ((SLong)aMaster->mSizeOfPage * aMaster->mNumOfPages);
        SLong  sUsed  = ((SLong)aMaster->mSizeOfPage * aMaster->mAllocedPages);
        SFloat sUsedP = ((SFloat)sUsed / sTotal) * 100;
        SFloat sFreeP = (((SFloat)sTotal - sUsed) / sTotal) * 100;

        idlOS::printf(" VERSION          : v%"ID_INT32_FMT".%"ID_INT32_FMT"\n",
                aMaster->mMajorVersion, aMaster->mMinorVersion);
        idlOS::printf(" Total Size       : %"ID_UINT64_FMT"\n", sTotal);
        idlOS::printf(" Used Size        : %"ID_UINT64_FMT" (%3.2f%%)\n", sUsed, sUsedP);
        idlOS::printf(" Free Size        : %"ID_UINT64_FMT" (%3.2f%%)\n",
                sTotal - sUsed, sFreeP);
        idlOS::printf(" Pages Per Block  : %"ID_UINT32_FMT"\n", idf::mPagesPerBlock);
        idlOS::printf(" Num Of Pages     : %"ID_UINT32_FMT"\n", aMaster->mNumOfPages);
        idlOS::printf(" Size Of Page     : %"ID_UINT32_FMT"\n", aMaster->mSizeOfPage);
        idlOS::printf(" Alloced Pages    : %"ID_UINT32_FMT"\n", aMaster->mAllocedPages);
        idlOS::printf(" File System Mode : %"ID_INT32_FMT"\n", (UInt)aMaster->mFSMode);
        idlOS::printf(" Log Mode         : %"ID_INT32_FMT"\n", aMaster->mLogMode);
        idlOS::printf(" Num Of Files     : %"ID_UINT32_FMT"\n", aMaster->mNumOfFiles);
    }
}

void idf::dumpMeta(PDL_HANDLE aFd)
{
    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        SInt     sFileID;
        SInt     sFd = (SInt)aFd;
        idfMeta *sMeta;

        if(idf::mFdList[sFd].mIsUsed == IDF_FD_UNUSED)
        {
            return;
        }

        sFileID = idf::mFdList[sFd].mFileID;

        sMeta = idf::mMetaList[sFileID].mMeta;

        if(sMeta != NULL)
        {
            idlOS::printf("%10s %4"ID_INT32_FMT" %10"ID_UINT32_FMT" %5"ID_INT32_FMT" %5"ID_INT32_FMT" %s\n", 
                          "-rw-------", 
                          sMeta->mFileID,
                          sMeta->mSize,
                          sMeta->mNumOfPagesU,
                          sMeta->mNumOfPagesA,
                          sMeta->mFileName);             
        }
    }
}

void idf::dumpMetaP(const SChar *aPath)
{
    SChar   *sPath;
    SInt     sFileID;
    UInt     sIndex;
    UInt     sCount;
    UInt     sPageMax;
    UInt     sMaxIndex;
    UInt     sMaxOffset;
    idfMeta *sMeta;
    UInt    *sMap = NULL;

    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        sPath = idf::getpath(aPath);

        sFileID = idf::getFileIDByName(aPath);

        sMeta = idf::mMetaList[sFileID].mMeta;

        if((sMeta != NULL) && (sMeta->mSize > 0))
        {
            idlOS::printf("\n");
            idlOS::printf("=======================================================\n");
            idlOS::printf("                   META PAGE MAP DUMP\n");
            idlOS::printf("=======================================================\n");
            idlOS::printf(" [File : %s][Size : %"ID_UINT32_FMT"]\n", sPath, sMeta->mSize);
            idlOS::printf("---------------------- Direct Page --------------------\n");

            for(sIndex = 0; sIndex < IDF_DIRECT_MAP_SIZE; sIndex++)
            {
                if((sIndex != 0) && (sIndex % 4 == 0))
                {
                    idlOS::printf("\n");
                }

                idlOS::printf("[%5"ID_INT32_FMT"] %5"ID_INT32_FMT" ",
                              sIndex,
                              sMeta->mDirectPages[sIndex] & IDF_FILE_HOLE_INVERTER);
            }

            idlOS::printf("\n");

            sPageMax = sMeta->mSize / idf::mPageSize;

            if(sPageMax >= idf::mDirectPageNum)
            {
                (void)idf::alloc((void **)&sMap, idf::mPageSize);

                idlOS::printf("------------------ Indirect Page Map ------------------\n");

                for(sIndex = 0; sIndex < idf::mIndPageNum; sIndex++)
                {
                    if((sIndex != 0) && (sIndex % 4 == 0))
                    {
                        idlOS::printf("\n");
                    }

                    idlOS::printf("[%5"ID_INT32_FMT"] %5"ID_INT32_FMT" ", sIndex, sMeta->mIndPages[sIndex] & IDF_FILE_HOLE_INVERTER);
                }

                idlOS::printf("\n");

                sMaxIndex  = (sPageMax - idf::mDirectPageNum) / idf::mIndMapSize + 1;
                sMaxOffset = (sPageMax - idf::mDirectPageNum) % idf::mIndMapSize;

                idlOS::printf("-------------------- Indirect Page --------------------\n");

                for(sIndex = 0; sIndex < sMaxIndex; sIndex++)
                {
                    if((sMeta->mIndPages[sIndex] != 0) &&
                       ((sMeta->mIndPages[sIndex] & IDF_FILE_HOLE_MASK)
                       == 0))
                    {
                        (void)idf::lseekFs(idf::mFd,
                                           sMeta->mIndPages[sIndex] * idf::mPageSize,
                                           SEEK_SET);

                        (void)idf::readFs(idf::mFd, 
                                          sMap, 
                                          idf::mPageSize);

                        for(sCount = 0; sCount < idf::mIndMapSize; sCount++)
                        {
                            if((sIndex == sMaxIndex -1) && (sMaxOffset == sCount))
                            {
                                break;
                            }

                            if((sCount != 0) && (sCount % 4 == 0))
                            {
                                idlOS::printf("\n");
                            }

                            sMap[sCount] = idlOS::ntoh(sMap[sCount]);
                            
                            idlOS::printf("[%5"ID_INT32_FMT"] %5"ID_INT32_FMT" ", sCount, sMap[sCount]);
                        }
                    }
                }

                idlOS::printf("\n");

                (void)idf::free(sMap);
            }

            idlOS::printf("\n");
        }

        if(sPath != NULL)
        {
            (void)idf::free(sPath);
        }
    }
}

void idf::dumpAllMeta()
{
    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        UInt sFileCount;
        UInt sIndex;
        UInt sCount = 0;
        idfMeta *sMeta;

        sFileCount = idf::mMaster.mNumOfFiles;

        idlOS::printf("total %"ID_UINT32_FMT"\n", idf::mMaster.mAllocedPages);
#if 0
        idlOS::printf("%10s %4s %10s %5s %5s %s\n", 
                      "MODE", 
                      "ID",
                      "SIZE",
                      "U",
                      "A",
                      "NAME");             
#endif

        for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
        {
            if(sCount == sFileCount)
            {
                break;
            }

            sMeta = idf::mMetaList[sIndex].mMeta;
            if(sMeta != NULL)
            {
                sCount++;

                idlOS::printf("%10s %4"ID_INT32_FMT" %10"ID_UINT32_FMT" %5"ID_INT32_FMT" %5"ID_INT32_FMT" %s\n", 
                              "-rw-------", 
                              sMeta->mFileID,
                              sMeta->mSize,
                              sMeta->mNumOfPagesU,
                              sMeta->mNumOfPagesA,
                              sMeta->mFileName);             
            }
        }
    }
}

void idf::dumpBlockMap()
{
    UInt sIndex;
    UInt sCount;
    UInt sTotalBlock = idf::mPageNum / idf::mPagesPerBlock;

    idlOS::printf("===============================================================\n");
    idlOS::printf("                        DUMP BLOCK MAP\n");
    idlOS::printf("===============================================================\n");

    for(sCount = 0; sCount < sTotalBlock; sCount += (idf::mPagesPerBlock*2))
    {
        idlOS::printf("%8"ID_INT32_FMT" : ", sCount);

        for(sIndex = 0; sIndex < idf::mPagesPerBlock; sIndex++)
        {
            idlOS::printf("%2x ", idf::mBlockMap[sCount + sIndex]);
        }

        idlOS::printf("    ");

        for(sIndex = 0; sIndex < idf::mPagesPerBlock; sIndex++)
        {
            idlOS::printf("%2x ", idf::mBlockMap[sCount + sIndex]);
        }

        idlOS::printf("\n");
    }
    idlOS::printf("===========================================\n");
}

/*
IDE_RC idf::validateMaster()
{
    UInt   sIndex;
    UInt   sCount = 0;
    UInt   sMax = idf::mPageNum / idf::mPagesPerBlock;
    IDE_RC sRc;

    for(sIndex = 0; sIndex < sMax; sIndex++)
    {
        if(idf::mBlockMap[sIndex] == 0xFF)
        {
            sCount++;
        }
    }

    sCount *= idf::mPagesPerBlock;

    if((sCount)== idf::mMaster.mAllocedPages)
    {
        sRc = IDE_SUCCESS;
    }
    else
    {
        sRc = IDE_FAILURE;
    }

    return sRc;
}
*/

IDE_RC idf::initializeCore(const SChar * aFileName)
{
    //=============================================================
    // aFileName이 NULL이 아니면 idfCore의 함수를 사용하도록 한다.
    //=============================================================
    (void)idf::initWithIdfCore();

    (void)gAlloca.init(IDF_PAGE_SIZE, IDF_ALIGN_SIZE, ID_FALSE);

    (void)idf::initCRCTable();

    (void)idf::initPageMapPool();

    (void)idf::alloc((void **)&idf::mEmptyBuf, EMPTY_BUFFER_SIZE);

    idlOS::memset(idf::mEmptyBuf, 
                  '\0', 
                  EMPTY_BUFFER_SIZE);

    (void)idf::alloc((void **)&idf::mLogHeader, ID_SIZEOF(idfLogHeader));

    //======================
    // Mutex를 초기화 한다.
    //======================
    (void)idf::initMutex();

    (void)idf::initFileName(aFileName);

#if !defined(USE_RAW_DEVICE)
    (void)idf::initLockFile();
#endif

    return IDE_SUCCESS;
}

//====================================================================
// idf 클래스를 초기화 한다. initializeStatic()과 다른 점은 이 함수는
// Utility에서 최초로 파일을 생성하고자 할 때 사용한다.
// 그 이외에는 이 함수를 사용하지 않는다.
//====================================================================
IDE_RC idf::initializeStatic2(const SChar * aFileName, idfMaster *aMaster)
{
    UInt   sIndex;
    SInt   sRc;
    SChar *sBuf = NULL;

    //fix for VxWorks raw device
    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        IDE_RAISE(pass);
    }

    if(aFileName == NULL)
    {
        //======================================================
        // aFileName이 NULL이면 idlOS의 함수를 사용하도록 한다.
        //======================================================
        (void)idf::initWithIdlOS();
    }
    else
    {
        IDE_TEST(idf::initializeCore(aFileName) != IDE_SUCCESS);

        // aFileName이 NULL이 아니면 idf::initializeCore()를 호출한 후
        // mFileName이 NULL일 수 없다.
        // Klocwork error를 방지하기 위해 넣음
        IDE_TEST(idf::mFileName == NULL);

        IDE_TEST((idf::mFd = idlOS::open(idf::mFileName,
                        O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                        S_IRUSR | S_IWUSR))
                 == PDL_INVALID_HANDLE);

        // 파일을 초기화하지 않으면 다른 작업이 불가능 하다.
        // 사용자가 입력한 Master를 가지고 Master 페이지를 초기화 한다.
        if(aMaster != NULL)
        {
            idlOS::memcpy(&idf::mMaster, 
                          aMaster, 
                          idf::mMasterSize);
        }

        // Master 초기화
        (void)idf::initMasterPage(aMaster);

        //==================================================================
        // File System의 Mode가 1이면 파일을 disk의 크기만큼 미리 할당한다.
        // File System의 환경 설정은 initMasterPage()에서 하므로 이 함수
        // 다음에 disk 공간을 모두 할당한다.
        //==================================================================
        if(idf::mMaster.mFSMode == 1)
        {
            (void)idf::alloc((void **)&sBuf, idf::mPageSize);

            idlOS::memset(sBuf, 
                          '\0', 
                          idf::mPageSize);

            for(sIndex = 0; sIndex < idf::mPageNum; sIndex++)
            {
                IDE_TEST((sRc = idf::writeFs(idf::mFd, 
                                             sBuf, 
                                             idf::mPageSize)) != (SInt)idf::mPageSize);
            }

            (void)idf::free(sBuf);

            sBuf = NULL;
        }

        (void)idf::initBlockMap();

        // Meta 초기화
        IDE_TEST(idf::initMetaList2() != IDE_SUCCESS);

        // Fd List 초기화
        (void)idf::initFdList();

        if(idf::mMaster.mLogMode == IDF_LOG_MODE)
        {
            // 로그파일을 생성한다.
            idf::mLogFd = idlOS::open(idf::mLogFileName,
                                      O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                                      S_IRUSR | S_IWUSR);

            IDE_TEST(idf::mLogFd == PDL_INVALID_HANDLE);
        }

        (void)initLogList();

        IDE_TEST(idf::masterLog(1) != IDE_SUCCESS);

        sIndex = idlOS::hton(IDF_SYSTEM_SIGN);

        IDE_TEST(idf::lseekFs(idf::mFd, 
                              0, 
                              SEEK_SET) == -1);

        IDE_TEST(idf::writeFs(idf::mFd, 
                              &sIndex, 
                              4) != 4);

        IDE_TEST(idf::writeMaster() != IDE_SUCCESS);
    }

    IDE_EXCEPTION_CONT(pass);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(idf::mFileName!= NULL)
    {
        (void)idf::free(idf::mFileName);

        idf::mFileName = NULL;
    }

    if(idf::mLogFileName!= NULL)
    {
        (void)idf::free(idf::mLogFileName);

        idf::mLogFileName = NULL;
    }

    if(sBuf != NULL)
    {
        (void)idf::free(sBuf);

        sBuf = NULL;
    }

    return IDE_FAILURE;
}

//================
// initialize idf 
//================
IDE_RC idf::initializeStatic(const SChar * aFileName)
{
    SChar *sCurrentVFS;
    UInt   sCheck;

    //fix for VxWorks raw device
    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        IDE_RAISE(pass);
    }

    if(aFileName == NULL)
    {
        if((sCurrentVFS = idlOS::getenv(ALTIBASE_ENV_PREFIX"VFS")) !=  NULL)
        {
            aFileName = sCurrentVFS;
        }
    }

    if(aFileName == NULL)
    {
        //======================================================
        // aFileName이 NULL이면 idlOS의 함수를 사용하도록 한다.
        //======================================================
        (void)idf::initWithIdlOS();
    }
    else
    {
        IDE_TEST(idf::initializeCore(aFileName) != IDE_SUCCESS);

        // aFileName이 NULL이 아니면 idf::initializeCore()를 호출한 후
        // mFileName이 NULL일 수 없다.
        // Klocwork error를 방지하기 위해 넣음
        IDE_TEST(idf::mFileName == NULL);

        // serverStart() 함수에서 initializeStatic() 함수를 호출한 경우
        // 파일이 없으면 기본 설정값을 가지고 파일을 생성한다.
        if((idf::mFd = idlOS::open(idf::mFileName, O_RDWR | O_BINARY))
           == PDL_INVALID_HANDLE)
        {
            IDE_TEST((idf::mFd = idlOS::open(idf::mFileName,
                            O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                            S_IRUSR | S_IWUSR))
                     == PDL_INVALID_HANDLE);

            sCheck = idlOS::hton(IDF_SYSTEM_SIGN);

            IDE_TEST(idf::lseekFs(idf::mFd, 
                                  0, 
                                  SEEK_SET) == -1);

            IDE_TEST(idf::writeFs(idf::mFd, 
                                  &sCheck, 
                                  4) != 4);

            //===================================================
            // 파일을 초기화하지 않으면 다른 작업이 불가능 하다.
            //===================================================

            // Master 초기화
            (void)idf::initMasterPage(NULL);

            // Block Map 초기화
            (void)idf::initBlockMap();

            // Meta 초기화
            IDE_TEST(idf::initMetaList2() != IDE_SUCCESS);

            // Fd List 초기화
            (void)idf::initFdList();

            if(idf::mMaster.mLogMode == IDF_LOG_MODE)
            {
                // 로그파일을 생성한다.
                idf::mLogFd = idlOS::open(idf::mLogFileName,
                                          O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                                          S_IRUSR | S_IWUSR);
            }

            IDE_TEST(idf::mLogFd == PDL_INVALID_HANDLE);
        }
        else
        {
            //================================================================
            // 파일의 Master, Meta 페이지를 읽어서 파일 시스템을 초기화 한다.
            //================================================================

            // Master 페이지를 읽는다.
            IDE_TEST(idf::lseekFs(idf::mFd, 
                                  0, 
                                  SEEK_SET) == -1);

            IDE_TEST(idf::readFs(idf::mFd, 
                                 &sCheck, 
                                 4) != 4);

            if(sCheck != idlOS::hton(IDF_SYSTEM_SIGN))
            {
                idlOS::exit(-1);
            }

            IDE_TEST(idf::readFs(idf::mFd, 
                                 &idf::mMaster, 
                                 idf::mMasterSize) != idf::mMasterSize);

            if(idf::mMaster.mSignature == idlOS::hton(IDF_SYSTEM_SIGN))
            {
                idf::mLogMode = idf::mMaster.mLogMode;

                if((idf::mLogMode != IDF_LOG_MODE) &&
                   (idf::mLogMode != IDF_NO_LOG_MODE))
                {
                    idf::mLogMode = IDF_DEFAULT_LOG_MODE;
                }
            }
            else
            {
                idf::mLogMode = IDF_DEFAULT_LOG_MODE;
            }

            if(idf::mLogMode == IDF_LOG_MODE)
            {
                // mFilename.log가 존재하는지 확인한다. 
                if(idlOS::access(idf::mLogFileName, O_RDWR) == 0)
                {
                    idf::mLogFd = idlOS::open(idf::mLogFileName, O_RDWR);

                    IDE_TEST(idf::mLogFd == PDL_INVALID_HANDLE);

                    // 로그 파일이 존재하면 recovery를 수행한다.
                    IDE_TEST(idf::doRecovery() != IDE_SUCCESS);

                    IDE_TEST(idf::lseekFs(idf::mLogFd, 
                                          0, 
                                          SEEK_SET) == -1);
                }
                else
                {
                    // 로그 파일이 존재하지 않으면 새로 생성한다.
                    idf::mLogFd = idlOS::open(idf::mLogFileName,
                            O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                            S_IRUSR | S_IWUSR);

                    IDE_TEST(idf::mLogFd == PDL_INVALID_HANDLE);
                }
            }

            IDE_TEST(idf::lseekFs(idf::mFd, 
                                  4, 
                                  SEEK_SET) == -1);

            IDE_TEST(idf::readFs(idf::mFd, 
                                 &(idf::mMaster), 
                                 idf::mMasterSize) != idf::mMasterSize);

            (void)idf::initMasterPage(&(idf::mMaster));

            (void)idf::initFdList();

            (void)idf::initBlockMap();

            IDE_TEST(idf::initMetaList() != IDE_SUCCESS);

            //===========================================================
            // Master의 Signature가 올바른지 확인한다.
            // Recovery 과정에서 Master를 수정할 수 있기 때문에
            // Recovery를 수행한 후 호출한다.
            // Recovery 후에도 Master의 Signature가 올바르지 않으면
            // 파일 시스템이 깨진 경우이므로 더이상의 동작은 무의미하다.
            //===========================================================
            if(idf::mMaster.mSignature != idlOS::hton(IDF_SYSTEM_SIGN))
            {
                idlOS::exit(-1);
            }
        }

        (void)idf::initLogList();

        IDE_TEST(idf::masterLog(1) != IDE_SUCCESS);

        IDE_TEST(idf::writeMaster() != IDE_SUCCESS);
    }

    IDE_EXCEPTION_CONT(pass);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(idf::mFileName != NULL)
    {
        (void)idf::free(idf::mFileName);

        idf::mFileName = NULL;
    }

    if(idf::mLogFileName != NULL)
    {
        (void)idf::free(idf::mLogFileName);

        idf::mLogFileName = NULL;
    }

    return IDE_FAILURE;
}

void idf::finalFiles()
{
    if(idf::mFd != PDL_INVALID_HANDLE)
    {
        (void)idlOS::close(idf::mFd);

        idf::mFd = PDL_INVALID_HANDLE;
    }
    if(idf::mLogFd != PDL_INVALID_HANDLE)
    {
        (void)idlOS::close(idf::mLogFd);

        if(idf::mLogFileName != NULL)
        {
            (void)idlOS::unlink(idf::mLogFileName);
        }

        idf::mLogFd = PDL_INVALID_HANDLE;
    }

    if(idf::mFileName != NULL)
    {
        (void)idf::free(idf::mFileName);

        idf::mFileName = NULL;
    }
    if(idf::mLogFileName != NULL)
    {
        (void)idf::free(idf::mLogFileName);

        idf::mLogFileName = NULL;
    }
}

//==============
// finalize idf
//==============
IDE_RC idf::finalizeStatic()
{
    UInt        sIndex;
    idfPageMap *sTemp;
    idfPageMap *sMap;

    if(idf::mFd == PDL_INVALID_HANDLE)
    {
        IDE_RAISE(pass);
    }

    //===================================================================
    // finalize 도중 다른 쓰레드가 작업을 수행하려 할 수 있으므로 Meta에
    // 락을 잡는다.
    // 물론, finalize 도중에 다른 쓰레드가 동작해서는 안된다.
    //===================================================================
    (void)idf::lock();

    // 로그를 내린 후 로그의 내용을 모두 디스크에 반영한다.
    if(idf::mTransCount != 0)
    {
        idf::masterLog(1);
    }

    IDE_TEST(idf::fsyncFs(idf::mFd) == -1);

    (void)idf::finalFiles();

    //==============================
    // 할당한 자원을 모두 해제한다.
    //==============================

    if(idf::mEmptyBuf != NULL)
    {
        (void)idf::free(idf::mEmptyBuf);

        idf::mEmptyBuf = NULL;
    }

    if(idf::mLogHeader != NULL)
    {
        (void)idf::free(idf::mLogHeader);

        idf::mLogHeader = NULL;
    }

    // Block Map 해제
    if(idf::mBlockMap != NULL)
    {
        (void)idf::free(idf::mBlockMap);

        idf::mBlockMap = NULL;
    }

    if(idf::mBlockList != NULL)
    {
        (void)idf::free(idf::mBlockList);

        idf::mBlockList = NULL;
    }

    if(idf::mCRCTable != NULL)
    {
        (void)idf::free(idf::mCRCTable);

        idf::mCRCTable = NULL;
    }

    // Page Map 해제
    sMap = idf::mPageMapPool;
    sTemp = sMap->mNext;

    while(sMap != NULL)
    {
        if(sMap->mMap != NULL)
        {
            (void)idf::free(sMap->mMap);
            sMap->mMap = NULL;
            sMap->mNext = NULL;
        }

        (void)idf::free(sMap);

        sMap = sTemp;

        if(sMap == idf::mPageMapPool)
        {
            break;
        }

        sTemp = sMap->mNext;
    }

    // Fd List 해제
    if(idf::mFdList != NULL)
    {
        (void)idf::free(idf::mFdList);

        idf::mFdList = NULL;
    }

    // Meta List 해제
    if(idf::mMetaList != NULL)
    {
        for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
        {
            if(idf::mMetaList[sIndex].mMeta != NULL)
            {
                (void)idf::free(idf::mMetaList[sIndex].mMeta);
            }
        }

        (void)idf::free(idf::mMetaList);
    }

    if(idf::mLogBuffer != NULL)
    {
        (void)idf::free(idf::mLogBuffer);
    }

    if(idf::mLogList != NULL)
    {
        (void)idf::free(idf::mLogList);
    }

    if(idf::mMapHeader != NULL)
    {
        (void)idf::free(idf::mMapHeader);
    }

    if(idf::mMapLog != NULL)
    {
        (void)idf::free(idf::mMapLog);
    }

    (void)gAlloca.destroy();

    (void)idf::unlock();

    (void)idf::finalMutex();

#if !defined(USE_RAW_DEVICE)
    (void)idf::finalLockFile();
#endif

    IDE_EXCEPTION_CONT(pass);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idf::finalMutex()
{
    IDE_ASSERT(idlOS::thread_mutex_destroy(&idf::mMutex) == 0);
}

//======================================================================
// idlOS::open(conat SChar*, SInt, SInt, SInt) 함수를 호출하기 위한 API
//======================================================================
PDL_HANDLE idf::idlopen(const SChar * pathname, SInt flag, ...)
{
    SInt mode = 0;

    if(flag & O_CREAT)
    {
        va_list arg;
        va_start(arg, flag);
        mode = va_arg(arg, SInt);
        va_end(arg);
    }

    return idlOS::open(pathname, flag, mode, 0);
}

void idf::lock()
{
    IDE_ASSERT(idlOS::thread_mutex_lock(&idf::mMutex) == 0);
}

void idf::unlock()
{
    IDE_ASSERT(idlOS::thread_mutex_unlock(&idf::mMutex) == 0);
}

//===========================================
// 사용을 완료한 File Descriptor를 반환한다.
// 반환한 File Descriptor는 재사용한다.
//===========================================
IDE_RC idf::freeFd(SInt aFd)
{
    idfFdEntry *sFd = &(idf::mFdList[aFd]);

    sFd->mFileID = IDF_INVALID_FILE_ID;
    sFd->mCursor = 0;
    sFd->mIsUsed = IDF_FD_UNUSED;

    idf::mFileOpenCount--;

    return IDE_SUCCESS;
}

//============================================================================
// 사용을 완료한 Meta를 반환한다.
// 반환한 Meta의 FileID를 재사용 하므로 FileID와 관련된 자원을 모두 해제한다.
//============================================================================
IDE_RC idf::freeMeta(SInt aFileID)
{
    idfMetaEntry *sMetaEntry = &(idf::mMetaList[aFileID]);
    idfMeta      *sMeta      = sMetaEntry->mMeta;
    idfPageMap   *sPageMap;
    idfPageMap   *sNextPageMap;
    UInt          sIndex;
    UInt          sCount = 0;
    UInt          sTotalBlocks = idf::mPageNum / idf::mPagesPerBlock;

    IDE_ASSERT(sMeta != NULL);

    //============================================================
    // 아직 해당 파일을 열고 있는 File Descriptor가 있다면
    // 사용 불가능 하도록 mIsUsed를 IDF_FD_INVALID(2)로 설정한다.
    // * mIsUsed 값의 의미
    //   IDF_FD_UNUSED  (0) : 사용하고 있지 않음,
    //   IDF_FD_USED    (1) : 사용하고 있음,
    //   IDF_FD_INVALID (2) : 사용하고 있지만 유효하지 않음
    //============================================================
    if(sMetaEntry->mFileOpenCount != 0)
    {
        for(sIndex = 0; sIndex < idf::mMaxFileOpenCount; sIndex++)
        {
            if(idf::mFdList[sIndex].mFileID == aFileID)
            {
                idf::mFdList[sIndex].mIsUsed = IDF_FD_INVALID;

                if((++sCount) == idf::mFileOpenCount)
                {
                    break;
                }
            }
        }
    }

    idlOS::memset(sMetaEntry, 
                  0x00, 
                  ID_SIZEOF(idfMetaEntry));

    idf::mMaster.mNumOfFiles--;

    //==========================================================================
    // Page Map Pool에서 aFileID에 해당하는 Page Map을 삭제한다.
    // Page Map Pool에서 Page Map을 삭제한 뒤 리스트의 가장 마지막으로 이동한다.
    // Page Map에 달려있는 mMap의 메모리는 이곳에서 해제하지 않는다.
    // Page Map에 달려있는 mMap은 finalizeStatic을 호출하면 그곳에서
    // Page Map Pool의 메모리를 해제하면서 mMap의 메모리도 함께 해제한다.
    //==========================================================================
    if(idf::mPageMapPoolCount > 0)
    {
        sPageMap = idf::mPageMapPool;

        do
        {
            sNextPageMap = sPageMap->mNext;

            if(sPageMap->mFileID == aFileID)
            {
                if(sPageMap == idf::mPageMapPool)
                {
                    // Page Map Pool이 모두 사용중이고 가장 앞의 Page Map에
                    // 해당하는 Meta를 삭제한 경우 단순히 리스트를 shift한다.
                    sPageMap->mFileID = IDF_INVALID_FILE_ID;

                    idf::mPageMapPool = idf::mPageMapPool->mNext;
                }
                else
                {
                    // 삭제한 Meta의 PageMap을 제일 마지막 위치로 이동한다.
                    sPageMap->mNext->mPrev = sPageMap->mPrev;
                    sPageMap->mPrev->mNext = sPageMap->mNext;

                    idf::mPageMapPool->mPrev->mNext = sPageMap;

                    sPageMap->mPrev = idf::mPageMapPool->mPrev;
                    sPageMap->mNext = idf::mPageMapPool;

                    idf::mPageMapPool->mPrev = sPageMap;

                    sPageMap->mFileID = IDF_INVALID_FILE_ID;
                }

                idf::mPageMapPoolCount--;

                // 사용중인 PageMap을 모두 검사했으면 종료한다.
                if(idf::mPageMapPoolCount == 0)
                {
                    break;
                }
            }

            sPageMap = sNextPageMap;
        } while(sPageMap != idf::mPageMapPool);
    }

    for(sIndex = idf::mSystemSize; sIndex < sTotalBlocks; sIndex++)
    {
        if(idf::mBlockList[sIndex] == aFileID)
        {
            idf::mBlockList[sIndex] = -1;

            idf::mBlockMap[sIndex] = 0x00;

            idf::mMaster.mAllocedPages -= idf::mPagesPerBlock;
        }
    }

    (void)idf::free(sMeta);

    sMeta = NULL;

    return IDE_SUCCESS;
}

IDE_RC idf::isDir(const SChar *aName)
{
    UInt     sNumOfFiles;
    UInt     sIndex;
    UInt     sCount = 0;
    UInt     sNameLen;
    idfMeta *sMeta;
    IDE_RC   sRc = IDE_FAILURE;;

    sNumOfFiles = idf::mMaster.mNumOfFiles;

    // 이 함수를 호출하기 전에 aName이 NULL인지 확인하므로 이 함수에서 다시
    // aName이 NULL인지 확인하지 않는다.
    assert(aName != NULL);

    sNameLen = idlOS::strlen(aName);

    // 전체 Meta개수 만큼 loop를 수행한다.
    for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
    {
        // Meta가 NULL이 아니면 이름을 검사한다.
        if(idf::mMetaList[sIndex].mMeta == NULL)
        {
            continue;
        }

        sMeta = idf::mMetaList[sIndex].mMeta;

        // 파일 시스템내에 디렉토리가 별도로 존재하지 않으므로
        // aName이 Meta에 저장한 파일 이름의 substring이면 
        // aName을 디렉토리로 간주한다.
        if((idlOS::strncmp(sMeta->mFileName, aName, sNameLen) == 0) &&
           (idlOS::strlen(sMeta->mFileName) > sNameLen))
        {
            // aName이 디렉토리인 것을 확인하였다면 바로 loop를 종료한다.
            sRc = IDE_SUCCESS;
            break;
        }

        // Meta에 존재하는 파일을 모두 읽었으면 종료한다.
        ++sCount;

        if(sCount >= sNumOfFiles)
        {
            break;
        }
    }

    return sRc;
}

idfDir* idf::getDir(const SChar *aName)
{
    UInt       sNumOfFiles;
    UInt       sIndex;
    UInt       sNameLen;
    UInt       sDirSize;
    idfMeta   *sMeta;
    idfDir    *sDir = NULL;
    idfDirent *sDirentP = NULL;
    dirent    *sDirent = NULL;
    UInt       sFence = 0;

    sNumOfFiles = idf::mMaster.mNumOfFiles;

    sNameLen = idlOS::strlen(aName);

    (void)idf::alloc((void **)&sDir, ID_SIZEOF(idfDir));

    sDir->mDirCount = 0;
    sDir->mDirIndex = 0;
    sDir->mFirst = NULL;

    sDirSize = ID_SIZEOF(dirent);

    // 전체 Meta개수 만큼 loop를 수행한다.
    for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
    {
        // Meta가 NULL이 아니면 이름을 검사한다.
        if(idf::mMetaList[sIndex].mMeta == NULL)
        {
            continue;
        }

        sMeta = idf::mMetaList[sIndex].mMeta;

        // 파일 시스템내에 디렉토리가 별도로 존재하지 않으므로
        // aName이 Meta에 저장한 파일 이름의 substring이면 
        // aName을 디렉토리로 간주한다.
        if((idlOS::strncmp(sMeta->mFileName, aName, sNameLen) == 0) &&
           (idlOS::strlen(sMeta->mFileName) > sNameLen) &&
           ((*(sMeta->mFileName + sNameLen) == '\\') || (*(sMeta->mFileName + sNameLen) == '/')))
        {
            (void)idf::alloc((void **)&sDirentP, ID_SIZEOF(idfDirent));

            sDirentP->mNext = sDir->mFirst;
            sDir->mFirst = sDirentP;

            //==================================================================
            // struct dirent는 platform마다 형태가 약간씩 다르다.
            // 하지만 d_name은 공통으로 갖고 있다.
            // hpux, linux, aix, dec, windows는 d_name이 char 배열로 되어있다.
            // 그 밖의 platform은 d_name이 char[1], char*이다.
            // (대표적으로 Solaris는 char[1]이고 windows는 struct dirent가 없다.
            //  PD에 windows를 위한 dirent가 있다.)
            // 따라서 d_name이 char 배열로 되어있지 않은 platform에서는
            // struct dirent의 크기 + 파일 이름 길이 + 1 만큼을
            // struct dirent의 크기로 하여 malloc한다.
            //==================================================================
#if (!defined(HPUX) && !defined(LINUX) && !defined(AIX) && !defined(DEC)) && !defined(VC_WIN32)
            (void)idf::alloc((void **)&sDirent, 
                             sDirSize + idlOS::strlen(sMeta->mFileName) + 1);
#else
            (void)idf::alloc((void **)&sDirent, sDirSize);
#endif

            //fix for SPARC_SOLARIS 5.10
            //signal SEGV (no mapping at the fault address) in _malloc_unlocked 
            sFence = idlOS::strlen(sMeta->mFileName + sNameLen + 1);

            idlOS::memcpy(sDirent->d_name,
                          sMeta->mFileName + sNameLen + 1,
                          sFence);
            sDirent->d_name[sFence] = '\0';

            sDirentP->mDirent = sDirent;
            sDir->mDirCount++;
        }

        // Meta에 존재하는 파일을 모두 읽었으면 종료한다.
        if(--sNumOfFiles == 0)
        {
            break;
        }
    }

    return sDir;;
}

void idf::getMetaCRC(idfMeta *aMeta)
{
    aMeta->mCRC = idf::getCRC((const UChar*)aMeta,
                              (idf::mMetaSize - ID_SIZEOF(UInt) * 2));
}

void idf::getMasterCRC()
{
    idf::mMaster.mCRC = idf::getCRC((const UChar*)&(idf::mMaster),
                                  (idf::mMasterSize - ID_SIZEOF(UInt) * 2));
}

IDE_RC idf::writeMaster()
{
    idfMaster sBufMaster;

    // Master의 CRC 값을 구한다.
    idf::getMasterCRC();

    // Master 페이지를 디스크에 저장한다.
    IDE_TEST(idf::lseekFs(idf::mFd, 
                          4, 
                          SEEK_SET) == -1);

#if defined (ENDIAN_IS_BIG_ENDIAN)
    IDE_TEST(idf::writeFs(idf::mFd, 
                          &(idf::mMaster), 
                          idf::mMasterSize) != idf::mMasterSize);
#else
    if(mRecCheck == IDF_START_RECOVERY)
    {
        IDE_TEST(idf::writeFs(idf::mFd, 
                 &(idf::mMaster), 
                 idf::mMasterSize) != idf::mMasterSize);
    }
    else
    {
        // idf::mMaster is not pointer and &(idf::mMaster) cannot be NULL.
        // Therefore code here checking &(idf::mMaster) is NULL is removed

        idlOS::memcpy(&sBufMaster, 
                      &(idf::mMaster), 
                      idf::mMasterSize);

        sBufMaster.mNumOfFiles       = idlOS::hton(sBufMaster.mNumOfFiles);
        sBufMaster.mMajorVersion     = idlOS::hton(sBufMaster.mMajorVersion);
        sBufMaster.mMinorVersion     = idlOS::hton(sBufMaster.mMinorVersion);
        sBufMaster.mAllocedPages     = idlOS::hton(sBufMaster.mAllocedPages);
        sBufMaster.mNumOfPages       = idlOS::hton(sBufMaster.mNumOfPages);
        sBufMaster.mSizeOfPage       = idlOS::hton(sBufMaster.mSizeOfPage);
        sBufMaster.mPagesPerBlock    = idlOS::hton(sBufMaster.mPagesPerBlock);
        sBufMaster.mMaxFileOpenCount = idlOS::hton(sBufMaster.mMaxFileOpenCount);
        sBufMaster.mMaxFileCount     = idlOS::hton(sBufMaster.mMaxFileCount);

        IDE_TEST(idf::writeFs(idf::mFd, 
                              &(sBufMaster), 
                              idf::mMasterSize) != idf::mMasterSize);
    }
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SInt idf::fdeof(PDL_HANDLE aHandle)
{
    PDL_OFF_T sEndPos;
    PDL_OFF_T sCurPos;
    SInt      sResult;

    sCurPos = idf::lseek(aHandle, 
                         0, 
                         SEEK_CUR);

    sEndPos = idf::filesize(aHandle);

    if(sEndPos == sCurPos)
    {
        sResult = 1;
    }
    else
    {
        sResult = 0;
    }

    return sResult;
}

SChar *idf::fdgets(SChar *aBuf, SInt aSize, PDL_HANDLE aHandle)
{
    SInt i;
    SInt sSize = aSize - 1;

    for(i = 0; i < sSize; i++)
    {
        if(idf::read(aHandle, aBuf + i, 1) <= 0)
        {
            aBuf[i] = '\0';

            break;
        }

        if(aBuf[i] == '\n')
        {
            break;
        }
    }

    if(i == sSize)
    {
        aBuf[i] = '\0';
    }
    else
    {
        aBuf[i + 1] = '\0';
    }

    if(aBuf[0] == '\0')
    {
        return NULL;
    }

    return aBuf;
}

idBool idf::isVFS()
{
    return (idf::open == idfCore::open) ? ID_TRUE : ID_FALSE;
}

void idf::initLockFile()
{
    PDL_HANDLE sFD;
    SChar      sBuffer[4] = { 0, 0, 0, 0 };
    SChar      sFileName[ID_MAX_FILE_NAME];

    idlOS::snprintf(sFileName,
                    ID_MAX_FILE_NAME,
                    "%s.lock",
                    idf::mFileName);

    if(idlOS::access(sFileName, F_OK) != 0)
    {
        sFD = idlOS::open(sFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

        IDE_TEST(sFD == PDL_INVALID_HANDLE);

        IDE_TEST(idf::writeFs(sFD,
                              sBuffer,
                              ID_SIZEOF(sBuffer)) != ID_SIZEOF(sBuffer));

        IDE_TEST(idlOS::close(sFD) != 0);
    }

    IDE_TEST(idlOS::flock_init(&idf::mLockFile,
                               O_RDWR,
                               sFileName,
                               0644) != 0);

    IDE_TEST(idlOS::flock_trywrlock(&mLockFile) != 0);

    return;

    IDE_EXCEPTION_END;

    idlOS::printf("Failed to initialize flock: errno=%"ID_INT32_FMT"\n", errno);

    idlOS::exit(-1);
}

void idf::finalLockFile()
{
    IDE_TEST(idlOS::flock_unlock(&idf::mLockFile) != 0);

    IDE_TEST(idlOS::flock_destroy(&idf::mLockFile) != 0);

    return;

    IDE_EXCEPTION_END;

    idlOS::printf("Failed to finalize flock: errno=%"ID_INT32_FMT"\n", errno);

    idlOS::exit(-1);
}

IDE_RC idf::masterLog(UInt aMode)
{
    UInt  sSize;
    UInt  sIndex;
    UInt  sHeaderSize;
    UInt  sMapHeaderSize;
    UInt  sCRC;
    UInt  sCount;
    UInt  sRc;

    idfMeta      *sMeta         = NULL;
    idfMapHeader *sMapHeader    = NULL;
    UInt         *sBufMapLog    = NULL;
    idfMeta      *sBufMetaLog   = NULL;
    idfMaster    *sMasterLog    = NULL;
    idfMapHeader *sMapHeaderLog = NULL;   
    UInt          sNum;
    UInt         *sBufMap;
    idfMeta       sBufMeta;

    idf::mTransCount++;

    //=============================================================
    // 트랜젝션이 설정한 값을 넘으면 시스템 메타데이터를 저장한다.
    // 메타 데이터는 우선 로그에 저장을 하고, 저장한 로그의 크기가
    // 일정 크기를 넘으면 디스크에 반영한다.
    // 로그에 저장할 때는 idf::mLogBuffer를 사용하여 write횟수를
    // 조절한다.
    //=============================================================

    if((idf::mTransCount > IDF_LOG_TRANS_SIZE) || (aMode != 0))
    {
        //=================================================
        // 로그에 변경한 메타데이터를 저장한다.
        // 로그 모드를 사용하지 않으면 이 부분은 생략한다.
        //=================================================
        if(idf::mLogMode == IDF_LOG_MODE)
        {
            sHeaderSize = ID_SIZEOF(idfLogHeader);
            sIndex = 0;
            sCount = idf::mLogListCount;

            //=====================
            // 1. Meta를 저장한다.
            //=====================
            if(sCount > 0)
            {
                sSize = sHeaderSize + idf::mMetaSize;

                while(sCount != 0 && sIndex < idf::mMaxFileCount)
                {
                    if(idf::mLogList[sIndex] == 0)
                    {
                        sIndex++;

                        continue;
                    }

                    sMeta = idf::mMetaList[sIndex].mMeta;

                    if(sMeta == NULL)
                    {
                        (void)idf::alloc((void **)&sMeta, idf::mMetaSize);

                        idlOS::memset(sMeta, 
                                      0x00, 
                                      idf::mMetaSize);
                    }

                    idf::mLogHeader->mFileID = idlOS::hton(sIndex);
                    idf::mLogHeader->mType   = idlOS::hton(IDF_META_LOG);

                    idf::getMetaCRC(sMeta);

                    if(idf::mLogBufferCursor + sSize > IDF_LOG_BUFFER_SIZE)
                    {
                        sRc = idf::writeFs(idf::mLogFd, 
                                           idf::mLogBuffer, 
                                           idf::mLogBufferCursor);

                        IDE_TEST(sRc != idf::mLogBufferCursor);

                        idf::mLogBufferCursor = 0;
                    }

                    idlOS::memcpy((idf::mLogBuffer + idf::mLogBufferCursor), 
                                  idf::mLogHeader, 
                                  sHeaderSize);

                    idf::mLogBufferCursor += sHeaderSize;

                    idlOS::memcpy((idf::mLogBuffer + idf::mLogBufferCursor), 
                                  sMeta, 
                                  idf::mMetaSize);

#if !defined (ENDIAN_IS_BIG_ENDIAN)
                    sBufMetaLog = (idfMeta *)(idf::mLogBuffer + idf::mLogBufferCursor); 

                    sBufMetaLog->mFileID      = idlOS::hton(sBufMetaLog->mFileID);
                    sBufMetaLog->mSize        = idlOS::hton(sBufMetaLog->mSize);
                    sBufMetaLog->mCTime       = idlOS::hton(sBufMetaLog->mCTime);
                    sBufMetaLog->mNumOfPagesU = idlOS::hton(sBufMetaLog->mNumOfPagesU);
                    sBufMetaLog->mNumOfPagesA = idlOS::hton(sBufMetaLog->mNumOfPagesA);

                    for(sNum = 0; sNum < IDF_PAGES_PER_BLOCK; sNum++)
                    {
                        sBufMetaLog->mDirectPages[sNum] = idlOS::hton(sBufMetaLog->mDirectPages[sNum]);
                    }

                    for(sNum = 0; sNum < IDF_INDIRECT_PAGE_NUM; sNum++)
                    {
                        sBufMetaLog->mIndPages[sNum] = idlOS::hton(sBufMetaLog->mIndPages[sNum]);
                    }

                    sCRC = getCRC((UChar*)sBufMetaLog, idf::mMetaSize - (ID_SIZEOF(UInt) * 2));

                    sBufMetaLog->mCRC = idlOS::hton(sCRC);
#endif

                    idf::mLogBufferCursor += idf::mMetaSize;

                    sCount--;

                    if(idf::mMetaList[sIndex].mMeta == NULL)
                    {
                        idf::free(sMeta);
                    }

                    sIndex++;
                }
            }

            //========================
            // 2. PageMap을 저장한다.
            //========================
            if(idf::mMapLogCount > 0)
            {
                sMapHeaderSize = ID_SIZEOF(idfMapHeader);

                sSize = sHeaderSize + sMapHeaderSize + idf::mIndEntrySize + ID_SIZEOF(UInt);

                idf::mLogHeader->mType   = idlOS::hton(IDF_PAGEMAP_LOG);

                for(sIndex = 0; sIndex < idf::mMapLogCount; sIndex++)
                {
                    if(idf::mLogBufferCursor + sSize > IDF_LOG_BUFFER_SIZE)
                    {
                        sRc = idf::writeFs(idf::mLogFd, 
                                           idf::mLogBuffer, 
                                           idf::mLogBufferCursor);

                        IDE_TEST(sRc != idf::mLogBufferCursor);

                        idf::mLogBufferCursor = 0;
                    }

                    sMapHeader = &(idf::mMapHeader[sIndex]);

                    idf::mLogHeader->mFileID = idlOS::hton(sMapHeader->mFileID);
 
                    idlOS::memcpy((idf::mLogBuffer + idf::mLogBufferCursor), 
                                  idf::mLogHeader, 
                                  sHeaderSize);

                    idf::mLogBufferCursor += sHeaderSize;

                    idlOS::memcpy((void*)(idf::mLogBuffer + idf::mLogBufferCursor), 
                                  sMapHeader, 
                                  sMapHeaderSize);

#if !defined (ENDIAN_IS_BIG_ENDIAN)
                    sMapHeaderLog = (idfMapHeader *)(idf::mLogBuffer + idf::mLogBufferCursor);
                    
                    sMapHeaderLog->mFileID   = idlOS::hton(sMapHeaderLog->mFileID);
                    sMapHeaderLog->mIndex    = idlOS::hton(sMapHeaderLog->mIndex);
                    sMapHeaderLog->mMapIndex = idlOS::hton(sMapHeaderLog->mMapIndex);
#endif
 
                    idf::mLogBufferCursor += sMapHeaderSize;

                    idlOS::memcpy((void*)(idf::mLogBuffer + idf::mLogBufferCursor), 
                                  (void*)&(idf::mMapLog[idf::mPagesPerBlock * sIndex]), 
                                  idf::mIndEntrySize);

#if !defined (ENDIAN_IS_BIG_ENDIAN)
                    sBufMapLog = (UInt *) (idf::mLogBuffer + idf::mLogBufferCursor);
                    
                    for(sNum= 0; sNum < idf::mPagesPerBlock; sNum++)
                    {
                        sBufMapLog[sNum] = idlOS::hton(sBufMapLog[sNum]);
                    }
#endif

                    idf::mLogBufferCursor += idf::mIndEntrySize;

#if defined (ENDIAN_IS_BIG_ENDIAN)
                    sCRC = idf::getCRC((const UChar*)&(idf::mMapLog[idf::mPagesPerBlock * sIndex]),
                            idf::mIndEntrySize);
#else
                    sCRC = idf::getCRC((const UChar*)&(sBufMapLog[0]),
                            idf::mIndEntrySize);

                    sCRC = idlOS::hton(sCRC);
#endif

                    idlOS::memcpy((void*)(idf::mLogBuffer + idf::mLogBufferCursor), 
                                  (void*)(&sCRC), 
                                  ID_SIZEOF(UInt));

                    idf::mLogBufferCursor += ID_SIZEOF(UInt);
                }
            }

            //=======================
            // 3. Master를 저장한다.
            //=======================
            if(idf::mLogBufferCursor + idf::mMasterSize > IDF_LOG_BUFFER_SIZE)
            {
                sRc = idf::writeFs(idf::mLogFd,
                                   idf::mLogBuffer,
                                   idf::mLogBufferCursor);

                IDE_TEST(sRc != idf::mLogBufferCursor);

                idf::mLogBufferCursor = 0;
            }

            idf::mLogHeader->mFileID = 0;
            idf::mLogHeader->mType   = idlOS::hton(IDF_MASTER_LOG);

            getMasterCRC();

            idlOS::memcpy((idf::mLogBuffer + idf::mLogBufferCursor),
                          idf::mLogHeader,
                          sHeaderSize);

            idf::mLogBufferCursor += sHeaderSize;

            idlOS::memcpy((idf::mLogBuffer + idf::mLogBufferCursor),
                          &(idf::mMaster),
                          idf::mMasterSize);

#if !defined (ENDIAN_IS_BIG_ENDIAN)
            sMasterLog = (idfMaster *) (idf::mLogBuffer + idf::mLogBufferCursor);
            sMasterLog->mNumOfFiles       = idlOS::hton(sMasterLog->mNumOfFiles);
            sMasterLog->mMajorVersion     = idlOS::hton(sMasterLog->mMajorVersion);
            sMasterLog->mMinorVersion     = idlOS::hton(sMasterLog->mMinorVersion);
            sMasterLog->mAllocedPages     = idlOS::hton(sMasterLog->mAllocedPages);
            sMasterLog->mNumOfPages       = idlOS::hton(sMasterLog->mNumOfPages);
            sMasterLog->mSizeOfPage       = idlOS::hton(sMasterLog->mSizeOfPage);
            sMasterLog->mPagesPerBlock    = idlOS::hton(sMasterLog->mPagesPerBlock);
            sMasterLog->mMaxFileOpenCount = idlOS::hton(sMasterLog->mMaxFileOpenCount);
            sMasterLog->mMaxFileCount     = idlOS::hton(sMasterLog->mMaxFileCount);

            sCRC = getCRC((UChar*)sMasterLog, idf::mMasterSize - (ID_SIZEOF(UInt) * 2));

            sMasterLog->mCRC = idlOS::hton (sCRC);
#endif

            idf::mLogBufferCursor += idf::mMasterSize;

            sRc = idf::writeFs(idf::mLogFd,
                               idf::mLogBuffer,
                               idf::mLogBufferCursor);

            IDE_TEST(sRc != idf::mLogBufferCursor);

            idf::mLogBufferCursor = 0;

            IDE_TEST(idf::fsyncFs(idf::mLogFd) == -1);
        }

        //========================================
        // 디스크에 변경한 메타데이터를 기록한다.
        //========================================

        //=====================
        // 1. Meta를 저장한다.
        //=====================
        if(idf::mLogListCount > 0)
        {
            sIndex = 0;

            while(idf::mLogListCount != 0 && sIndex < idf::mMaxFileCount)
            {
                if(idf::mLogList[sIndex] == 0)
                {
                    sIndex++;

                    continue;
                }

                idf::mLogList[sIndex] = 0;

                sMeta = idf::mMetaList[sIndex].mMeta;

                if(sMeta == NULL)
                {
                    (void)idf::alloc((void **)&sMeta, idf::mMetaSize);

                    idlOS::memset(sMeta, 
                                  0x00, 
                                  idf::mMetaSize);
                }

                idf::mLogHeader->mFileID = idlOS::hton(sIndex);
                idf::mLogHeader->mType   = idlOS::hton(IDF_META_LOG);

                // aFileID에 해당하는 Meta를 디스크에 저장한다.
                IDE_TEST(idf::lseekFs(idf::mFd,
                                      IDF_FIRST_META_POSITION + sIndex * IDF_META_SIZE,
                                      SEEK_SET) == -1); 

#if defined (ENDIAN_IS_BIG_ENDIAN)
                IDE_TEST(idf::writeFs(idf::mFd, 
                                      sMeta, 
                                      idf::mMetaSize) != idf::mMetaSize);
#else
                idlOS::memcpy(&sBufMeta , sMeta, idf::mMetaSize);
                
                sBufMeta.mFileID      = idlOS::hton(sBufMeta.mFileID);
                sBufMeta.mSize        = idlOS::hton(sBufMeta.mSize);
                sBufMeta.mCTime       = idlOS::hton(sBufMeta.mCTime);
                sBufMeta.mNumOfPagesU = idlOS::hton(sBufMeta.mNumOfPagesU);
                sBufMeta.mNumOfPagesA = idlOS::hton(sBufMeta.mNumOfPagesA);
                sBufMeta.mCRC         = idlOS::hton(sBufMeta.mCRC);

                for(sNum = 0; sNum < IDF_PAGES_PER_BLOCK; sNum++)
                {
                    sBufMeta.mDirectPages[sNum] = idlOS::hton(sBufMeta.mDirectPages[sNum]);
                }

                for(sNum = 0; sNum < IDF_INDIRECT_PAGE_NUM; sNum++)
                {
                    sBufMeta.mIndPages[sNum] = idlOS::hton(sBufMeta.mIndPages[sNum]);
                }

                IDE_TEST(idf::writeFs(idf::mFd, 
                            &sBufMeta, 
                            idf::mMetaSize) != idf::mMetaSize);
#endif

                if(idf::mMetaList[sIndex].mMeta == NULL)
                {
                    idf::free(sMeta);
                }

                sIndex++;

                idf::mLogListCount--;
            }
        }

        //========================
        // 2. PageMap을 저장한다.
        //========================
        if(idf::mMapLogCount > 0)
        {
            idf::mLogHeader->mType = idlOS::hton(IDF_PAGEMAP_LOG);

            for(sIndex = 0; sIndex < idf::mMapLogCount; sIndex++)
            {
                sMapHeader = &(idf::mMapHeader[sIndex]);

                IDE_TEST(idf::lseekFs(idf::mFd,
                                      ((sMapHeader->mIndex * idf::mPageSize) + (sMapHeader->mMapIndex * ID_SIZEOF(UInt))),
                                      SEEK_SET) == -1);

#if defined (ENDIAN_IS_BIG_ENDIAN)
                sRc = idf::writeFs(idf::mFd,
                                   (void*)&(idf::mMapLog[idf::mPagesPerBlock * sIndex]),
                                   idf::mIndEntrySize);
#else
                idlOS::memcpy((void*)&sBufMap,
                        (void*)&idf::mMapLog,
                        idf::mIndEntrySize);

                for(sNum = sIndex; sNum < idf::mPagesPerBlock * (sIndex + 1); sNum++)
                {
                    sBufMap[sNum] = idlOS::hton(sBufMap[sNum]);
                }

                sRc = idf::writeFs(idf::mFd,
                        (void*)&(sBufMap[idf::mPagesPerBlock * sIndex]),
                        idf::mIndEntrySize);
#endif
 
                IDE_TEST(sRc != idf::mIndEntrySize);
            }

            // 페이지 맵 로그의 개수를 0으로 초기화 한다.
            idf::mMapLogCount = 0;
        }

        //=======================
        // 3. Master를 저장한다.
        //=======================
        IDE_TEST(idf::writeMaster() != IDE_SUCCESS);

        if(idlOS::filesize(idf::mLogFd) > 10*1024*1024)
        {
            IDE_TEST(idf::fsyncFs(idf::mFd) == -1);

            IDE_TEST(idf::initLog() != IDE_SUCCESS);

            idf::mIsSync = ID_TRUE;
        }

        idf::mTransCount = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idf::appandPageMapLog(SInt aFileID, SInt aIndex, SInt aMapIndex)
{
    UInt sIndex;
    UInt sPageIndex;

    idfMapHeader *sMapHeader = NULL;
    idfMeta      *sMeta = NULL;

    sMeta = idf::mMetaList[aFileID].mMeta;

    sPageIndex = sMeta->mIndPages[aIndex];

    for(sIndex = 0; sIndex < idf::mMapLogCount; sIndex++)
    {
        sMapHeader = &(idf::mMapHeader[sIndex]);

        if((sMapHeader->mFileID == aFileID) &&
           (sMapHeader->mIndex == sPageIndex) &&
           (sMapHeader->mMapIndex == (UInt)aMapIndex))
        {
            break;
        }
    }

    if(sIndex < IDF_PAGE_MAP_LOG_SIZE)
    {
        if(sIndex == idf::mMapLogCount)
        {
            sMapHeader = &(idf::mMapHeader[sIndex]);

            sMapHeader->mFileID = aFileID;
            sMapHeader->mIndex = sMeta->mIndPages[aIndex];
            sMapHeader->mMapIndex = aMapIndex;

            idf::mMapLogCount++;
        }

        idlOS::memcpy((void*)(&idf::mMapLog[(idf::mPagesPerBlock * sIndex)]),
                      (void*)idf::mPageMapPool->mMap,
                      idf::mIndEntrySize);
    }
    else
    {
        IDE_ASSERT(0);
    }

    return IDE_SUCCESS;
}

IDE_RC idf::initLog()
{
    if(idf::mLogMode == IDF_LOG_MODE)
    {
        IDE_TEST(idf::mLogFd == PDL_INVALID_HANDLE);

        IDE_TEST(idlOS::ftruncate(idf::mLogFd, 0) == -1);

        IDE_TEST(idf::lseekFs(idf::mLogFd, 
                              0, 
                              SEEK_SET) == -1);

        idf::mLogHeader->mFileID = 0;
        idf::mLogHeader->mType   = IDF_MASTER_LOG;

        getMasterCRC();

        IDE_TEST(idf::writeFs(idf::mLogFd,
                              idf::mLogHeader,
                              ID_SIZEOF(idfLogHeader)) != ID_SIZEOF(idfLogHeader));

        IDE_TEST(idf::writeFs(idf::mLogFd, 
                              &(idf::mMaster), 
                              idf::mMasterSize) != idf::mMasterSize);

        IDE_TEST(idf::fsyncFs(idf::mLogFd) == -1);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idf::doRecovery()
{
    idfMaster     sMaster;
    idfMeta       sMeta1;
    idfLogHeader  sHeader;
    idfMapHeader  sMapHeader;

    UInt         *sPageMap   = NULL;
    idfMeta      *sMeta      = NULL;
    SInt         *sCheckList = NULL;

    SInt          sHeaderSize;
    SInt          sMapHeaderSize;
    UInt          sCRC;
    SInt          sTxCount = 0;
    SInt          sRc;
    SInt          sState = 0;
    UInt          sIndex;

    // 로그를 사용하지 않으면 Recovery를 수행하지 않는다.
    if(idf::mLogMode == IDF_NO_LOG_MODE)
    {
        IDE_RAISE(pass);
    }

    if(idlOS::filesize(idf::mLogFd) == 0)
    {
        IDE_RAISE(pass);
    }

    sHeaderSize    = ID_SIZEOF(idfLogHeader);
    sMapHeaderSize = ID_SIZEOF(idfMapHeader);

    IDE_TEST(idf::lseekFs(idf::mLogFd, 
                          0, 
                          SEEK_SET) == -1);

    sRc = idf::readFs(idf::mLogFd, 
                      &sHeader, 
                      sHeaderSize);

    if(sRc != sHeaderSize)
    {
        IDE_RAISE(pass2);
    }

    sRc = idf::readFs(idf::mLogFd, 
                      &sMaster, 
                      idf::mMasterSize);

    if(sRc != idf::mMasterSize)
    {
        IDE_RAISE(pass2);
    }

    if(sMaster.mSignature != idlOS::hton(IDF_SYSTEM_SIGN))
    {
        IDE_RAISE(pass2);
    }

    sCRC = getCRC((UChar*)&sMaster, idf::mMasterSize - (ID_SIZEOF(UInt) * 2));

    sCRC = idlOS::hton(sCRC);

    if(sCRC != sMaster.mCRC)
    {
        IDE_RAISE(pass2);
    }

    sMaster.mPagesPerBlock    = idlOS::ntoh(sMaster.mPagesPerBlock);
    sMaster.mMaxFileOpenCount = idlOS::ntoh(sMaster.mMaxFileOpenCount);

    // 임시로 초기화 한다.
    idf::mIndEntrySize = ID_SIZEOF(UInt) * sMaster.mPagesPerBlock;
    idf::mMaxFileOpenCount = sMaster.mMaxFileOpenCount;

    sTxCount++;

    (void)idf::alloc((void **)&sPageMap, idf::mIndEntrySize);

    (void)idf::alloc((void **)&sCheckList, 
                     idf::mMaxFileOpenCount * ID_SIZEOF(UInt));

    (void)idf::alloc((void **)&sMeta,
                     idf::mMetaSize * idf::mMaxFileCount);

    for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
    {
        sCheckList[sIndex] = 0;
    }

    //===============
    // 1. Test Phase
    //===============

    while((sRc = idf::readFs(idf::mLogFd, 
                             &sHeader, 
                             sHeaderSize)) == sHeaderSize)
    {

        sHeader.mType = idlOS::hton(sHeader.mType);
        
        switch(sHeader.mType)
        {
        case IDF_MASTER_LOG:
            sRc = idf::readFs(idf::mLogFd, 
                              &sMaster, 
                              idf::mMasterSize);

            if(sRc != idf::mMasterSize)
            {
                sState = 1;
                break;
            }

            if(sMaster.mSignature != idlOS::hton(IDF_SYSTEM_SIGN))
            {
                sState = 1;
                break;
            }

            sCRC = getCRC((UChar*)&sMaster,
                          idf::mMasterSize - (ID_SIZEOF(UInt) * 2));

            sCRC = idlOS::hton(sCRC);

            if(sCRC != sMaster.mCRC)
            {
                sState = 1;
                break;
            }

            sTxCount++;

            sState = 0;

            break;
        case IDF_META_LOG:
            sRc = idf::readFs(idf::mLogFd, 
                              &sMeta1, 
                              idf::mMetaSize);

            if(sRc != idf::mMetaSize)
            {
                sState = 1;
                break;
            }

            sCRC = getCRC((UChar*)&sMeta1,
                          idf::mMetaSize - (ID_SIZEOF(UInt) * 2));

            sCRC = idlOS::hton(sCRC);

            sMeta1.mFileID = idlOS::ntoh(sMeta1.mFileID);

            if(sCRC != sMeta1.mCRC)
            {
                sState = 1;
                break;
            }

            sCheckList[sMeta1.mFileID]++;

            break;
        case IDF_PAGEMAP_LOG:
            sRc = idf::readFs(idf::mLogFd, 
                              &sMapHeader, 
                              sMapHeaderSize);

            if(sRc != sMapHeaderSize)
            {
                sState = 1;
                break;
            }

            sRc = idf::readFs(idf::mLogFd, 
                              sPageMap, 
                              idf::mIndEntrySize);

            if(sRc != (SInt)idf::mIndEntrySize)
            {
                sState = 1;
                break;
            }

            sRc = idf::readFs(idf::mLogFd, 
                              &sCRC, 
                              4);

            if(sRc != 4)
            {
                sState = 1;
                break;
            }

            sRc = idf::getCRC((UChar*)sPageMap, idf::mIndEntrySize);
 
            sRc = idlOS::hton(sRc);
            
            if((UInt)sRc != sCRC)
            {
                sState = 1;
                break;
            }

            break;
        default:
            //로그파일이 깨질수 있다.
            sState = 1;
            break;
        }

        if(sState == 1)
        {
            break;
        }
    }

    //===================
    // 2. Recovery Phase
    //===================

    IDE_TEST(idf::lseekFs(idf::mLogFd, 
                          0, 
                          SEEK_SET) == -1);

    while((sRc = idf::readFs(idf::mLogFd, 
                             &sHeader, 
                             sHeaderSize)) == sHeaderSize)
    {
        if(sTxCount <= 0)
        {
            break;
        }

        sHeader.mType = idlOS::hton(sHeader.mType);
 
        switch(sHeader.mType)
        {
        case IDF_MASTER_LOG:
            sRc = idf::readFs(idf::mLogFd, 
                              &sMaster, 
                              idf::mMasterSize);

            IDE_ASSERT(sRc == idf::mMasterSize);

            // 마지막의 Master만 기록하도록 한다.
            if(sTxCount == 1)
            {
                IDE_ASSERT(sMaster.mSignature == idlOS::hton(IDF_SYSTEM_SIGN));

                sCRC = getCRC((UChar*)&sMaster,
                              idf::mMasterSize - (ID_SIZEOF(UInt) * 2));

                sCRC = idlOS::hton(sCRC);

                IDE_ASSERT(sCRC == sMaster.mCRC);

                if(idlOS::memcmp((void*)&idf::mMaster,
                                 (void*)&sMaster,
                                 idf::mMasterSize) != 0)
                {
                    idlOS::memcpy((void*)&idf::mMaster,
                                  (void*)&sMaster,
                                  idf::mMasterSize);

                    sTxCount = -1;
                }
                else
                {
                    sTxCount = 0;
                }
            }
            else
            {
                sTxCount--;
            }

            break;
        case IDF_META_LOG:
            sRc = idf::readFs(idf::mLogFd, 
                              &sMeta1, 
                              idf::mMetaSize);

            sHeader.mFileID = idlOS::ntoh(sHeader.mFileID);

            sCheckList[sHeader.mFileID]--;

            IDE_ASSERT(sRc == idf::mMetaSize);

            // 마지막 Meta만 기록하도록 한다.
            if(sCheckList[sHeader.mFileID] == 0 || sTxCount == 1)
            {
                sCRC = getCRC((UChar*)&sMeta1,
                              idf::mMetaSize - (ID_SIZEOF(UInt) * 2));

                sCRC = idlOS::hton(sCRC);

                IDE_ASSERT(sCRC == sMeta1.mCRC);

                idlOS::memcpy(&(sMeta[sHeader.mFileID]), 
                              &sMeta1, 
                              idf::mMetaSize);

                sCheckList[sHeader.mFileID] = -1;
            }

            break;
        case IDF_PAGEMAP_LOG:
            // Page Map Log는 CRC값이 올바르면 기록하도록 한다.
            sRc = idf::readFs(idf::mLogFd, 
                              &sMapHeader, 
                              sMapHeaderSize);

            sMapHeader.mFileID   = idlOS::ntoh(sMapHeader.mFileID);
            sMapHeader.mIndex    = idlOS::ntoh(sMapHeader.mIndex);
            sMapHeader.mMapIndex = idlOS::ntoh(sMapHeader.mMapIndex);

            IDE_ASSERT(sRc == sMapHeaderSize);

            sRc = idf::readFs(idf::mLogFd, 
                              sPageMap, 
                              idf::mIndEntrySize);
            
            IDE_ASSERT(sRc == (SInt)idf::mIndEntrySize);
            
            sRc = idf::readFs(idf::mLogFd, 
                              &sCRC, 
                              4);

            IDE_ASSERT(sRc == 4);

            sRc = idf::getCRC((UChar*)sPageMap, idf::mIndEntrySize);
 
            sRc = idlOS::hton(sRc);
           
            IDE_ASSERT((UInt)sRc == sCRC);

            IDE_TEST(idf::lseekFs(idf::mFd,
                                  sMapHeader.mIndex * idf::mPageSize + sMapHeader.mMapIndex * ID_SIZEOF(UInt),
                                  SEEK_SET) == -1);

#ifdef __CSURF__
            IDE_ASSERT( *sPageMap > idf::mIndEntrySize );
#endif
            idf::writeFs(idf::mFd, 
                         sPageMap, 
                         idf::mIndEntrySize);

            break;
        default:
            IDE_ASSERT(0);
            break;
        }
    }

    for(sIndex = 0; sIndex < idf::mMaxFileCount; sIndex++)
    {
        if(sCheckList[sIndex] == -1) // 기록한다.
        {
            // Meta의 CRC 값을 구한다.
            (void)idf::getMetaCRC(&(sMeta[sIndex]));

            // aFileID에 해당하는 Meta를 디스크에 저장한다.
            IDE_TEST(idf::lseekFs(idf::mFd,
                                  IDF_FIRST_META_POSITION + sIndex * IDF_META_SIZE,
                                  SEEK_SET) == -1); 

            IDE_TEST(idf::writeFs(idf::mFd, 
                                  &(sMeta[sIndex]), 
                                  idf::mMetaSize) != idf::mMetaSize);
        }
    }

    mRecCheck = IDF_START_RECOVERY;

    IDE_TEST(idf::writeMaster() != IDE_SUCCESS);

    mRecCheck = IDF_END_RECOVERY;

    // Recovery가 완료되었으면 디스크를 동기화 하고 로그를 초기화 한다.
    IDE_TEST(idf::fsyncFs(idf::mFd) == -1);

    IDE_EXCEPTION_CONT(pass2);

    IDE_TEST(idf::initLog() != IDE_SUCCESS);

    if(sPageMap != NULL)
    {
        (void)idf::free(sPageMap);

        sPageMap = NULL;
    }

    if(sCheckList != NULL)
    {
        (void)idf::free(sCheckList);

        sCheckList = NULL;
    }

    if(sMeta != NULL)
    {
        (void)idf::free(sMeta);

        sMeta = NULL;
    }

    IDE_EXCEPTION_CONT(pass);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sPageMap != NULL)
    {
        (void)idf::free(sPageMap);
    }

    if(sCheckList != NULL)
    {
        (void)idf::free(sCheckList);
    }

    if(sMeta != NULL)
    {
        (void)idf::free(sMeta);
    }

    return IDE_FAILURE;
}

void idf::alloc(void **aBuffer, size_t aSize)
{
    (void)gAlloca.alloc(aBuffer, aSize);
}

void idf::free(void *aBuffer)
{
    (void)gAlloca.free(aBuffer);
}

ssize_t idf::writeFs(PDL_HANDLE aFd, const void* aBuf, size_t aCount)
{
#if !defined(USE_RAW_DEVICE) || defined(WRS_VXWORKS)
    return idlOS::write(aFd, 
                        aBuf, 
                        aCount);
#else
    SChar *sBufW    = NULL;
    SChar *sBufR    = NULL;
    UInt   sSize    = aCount;
    UInt   sWsize   = 0;
    UInt   sRemnant = 0;
    SInt   sCursor  = 0;
    SInt   sRewind  = 0;

    IDE_ASSERT(aCount <= IDF_PAGE_SIZE);

    //32K write buffer
    sBufW = gAlloca.getBufferW();

    //IDF_ALIGNED_BUFFER_SIZE read buffer
    sBufR = gAlloca.getBufferR();

    idlOS::memcpy(sBufW,
                  aBuf,
                  aCount);

    sCursor = idf::lseekFs(aFd,
                           0,
                           SEEK_CUR);

    IDE_TEST(sCursor == -1);

    sRewind = sCursor;

    while(sSize > 0)
    {
        sRemnant = sCursor % IDF_ALIGNED_BUFFER_SIZE;

        if((sRemnant == 0) && (sSize >= IDF_ALIGNED_BUFFER_SIZE))
        {
            sWsize = IDF_ALIGNED_BUFFER_SIZE;

            IDE_TEST(idf::lseekFs(aFd,
                                  sCursor,
                                  SEEK_SET) == -1);

            IDE_TEST(idlOS::write(aFd,
                                  sBufW + aCount - sSize,
                                  sWsize) != sWsize);
        }
        else
        {
            idlOS::memset(sBufR,
                          0x00,
                          IDF_ALIGNED_BUFFER_SIZE);

            if(sRemnant == 0)
            {
                sWsize = sSize;
            }
            else
            {
                sWsize = (IDF_ALIGNED_BUFFER_SIZE - sRemnant) < sSize ? 
                         (IDF_ALIGNED_BUFFER_SIZE - sRemnant) : sSize;
            }

            IDE_TEST(idf::lseekFs(aFd,
                                  sCursor - sRemnant,
                                  SEEK_SET) == -1);

            //읽은 크기가 IDF_ALIGNED_BUFFER_SIZE 보다 적은 경우도 있다. 
            IDE_TEST(idlOS::read(aFd,
                                 sBufR,
                                 IDF_ALIGNED_BUFFER_SIZE) == -1);

            idlOS::memcpy(sBufR + sRemnant,
                          sBufW + aCount - sSize,
                          sWsize);
 
            IDE_TEST(idf::lseekFs(aFd,
                                  sCursor - sRemnant,
                                  SEEK_SET) == -1);
  
            IDE_TEST(idlOS::write(aFd,
                                  sBufR,
                                  IDF_ALIGNED_BUFFER_SIZE) != IDF_ALIGNED_BUFFER_SIZE);
        }

        sSize -= sWsize;
        sCursor += sWsize;
    }

    sRewind += (aCount - sSize);

    IDE_TEST(idf::lseekFs(aFd,
                          sRewind,
                          SEEK_SET) == -1);

    return aCount - sSize;

    IDE_EXCEPTION_END;

    return -1;
#endif
}

ssize_t idf::readFs(PDL_HANDLE aFd, void *aBuf, size_t aCount)
{
#if !defined(USE_RAW_DEVICE) || defined(WRS_VXWORKS)
    return idlOS::read(aFd, 
                       aBuf, 
                       aCount);
#else
    SChar *sBufW    = NULL;
    SChar *sBufR    = NULL;
    UInt   sSize    = aCount;
    UInt   sRsize   = 0;
    UInt   sRemnant = 0;
    SInt   sCursor  = 0;
    SInt   sRewind  = 0;

    IDE_ASSERT(aCount <= IDF_PAGE_SIZE);

    //32K write buffer
    sBufW = gAlloca.getBufferW();

    //IDF_ALIGNED_BUFFER_SIZE read buffer
    sBufR = gAlloca.getBufferR();

    sCursor = idf::lseekFs(aFd,
                           0,
                           SEEK_CUR);

    IDE_TEST(sCursor == -1);

    sRewind = sCursor;

    while(sSize > 0)
    {
        sRemnant = sCursor % IDF_ALIGNED_BUFFER_SIZE;

        if((sRemnant == 0) && (sSize >= IDF_ALIGNED_BUFFER_SIZE))
        {
            sRsize = IDF_ALIGNED_BUFFER_SIZE;

            IDE_TEST(idf::lseekFs(aFd,
                                  sCursor,
                                  SEEK_SET) == -1);

            IDE_TEST(idlOS::read(aFd,
                                 sBufR,
                                 sRsize) != sRsize);

            idlOS::memcpy(sBufW + aCount - sSize,
                          sBufR,
                          sRsize);
        }
        else
        {
            if(sRemnant == 0)
            {
                sRsize = sSize;
            }
            else
            {
                sRsize = (IDF_ALIGNED_BUFFER_SIZE - sRemnant) < sSize ? 
                         (IDF_ALIGNED_BUFFER_SIZE - sRemnant) : sSize;
            }

            IDE_TEST(idf::lseekFs(aFd,
                                  sCursor - sRemnant,
                                  SEEK_SET) == -1);
 
            //읽은 크기가 IDF_ALIGNED_BUFFER_SIZE 보다 적은 경우도 있다. 
            IDE_TEST(idlOS::read(aFd,
                                 sBufR,
                                 IDF_ALIGNED_BUFFER_SIZE) == -1);

            idlOS::memcpy(sBufW + aCount - sSize,
                          sBufR + sRemnant,
                          sRsize);
        }

        sSize -= sRsize;
        sCursor += sRsize;
    }

    idlOS::memcpy(aBuf,
                  sBufW,
                  aCount - sSize);

    sRewind += (aCount - sSize);

    IDE_TEST(idf::lseekFs(aFd,
                          sRewind,
                          SEEK_SET) == -1);

    return aCount - sSize;

    IDE_EXCEPTION_END;

    return -1;
#endif
}

PDL_OFF_T idf::lseekFs(PDL_HANDLE aFd, PDL_OFF_T aOffset, SInt aWhence)
{
#if !defined(WRS_VXWORKS)
    return idlOS::lseek(aFd, 
                        aOffset, 
                        aWhence);
#else
    SInt sCursor = 0;
    SInt sRc     = 0;

    if(aWhence == SEEK_SET)
    {
        sCursor = aOffset;;

        sRc = ::ioctl(aFd,
                      FIOSEEK,
                      sCursor);
    }
    else
    {
        if(aWhence == SEEK_CUR)
        {
            sCursor = ::ioctl(aFd, 
                              FIOWHERE, 
                              0);

            IDE_TEST(sCursor == -1);

            sCursor += aOffset;

            sRc = ::ioctl(aFd,
                          FIOSEEK,
                          sCursor);
        }
        else
        {
            IDE_ASSERT(0);
        }
    }

    return sRc;

    IDE_EXCEPTION_END;

    return -1;
#endif
}

SInt idf::fsyncFs(PDL_HANDLE aFd)
{
#if !defined(WRS_VXWORKS)
    return idlOS::fsync(aFd);
#else
    return ::ioctl(aFd, 
                   FIOFLUSH, 
                   0);
#endif
}

#if defined(PDL_LACKS_PREADV)
ssize_t idf::preadv(PDL_HANDLE aFD, const iovec* aVec, int aCount, PDL_OFF_T aWhere)
{
    ssize_t sRet;
    IDE_TEST( ::lseek(aFD, aWhere, SEEK_SET) == -1 );
    sRet = ::readv(aFD, aVec, aCount);

    return sRet;

    IDE_EXCEPTION_END;
    return -1;
}

ssize_t idf::pwritev(PDL_HANDLE aFD, const iovec* aVec, int aCount, PDL_OFF_T aWhere)
{
    ssize_t sRet;
    IDE_TEST( ::lseek(aFD, aWhere, SEEK_SET) == -1 );
    sRet = ::writev(aFD, aVec, aCount);

    return sRet;

    IDE_EXCEPTION_END;
    return -1;
}
#endif

