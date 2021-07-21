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

PDL_HANDLE idfCore::open(const SChar *aPathName, SInt aFlag, ...)
{
    SInt     sFileID;
    SChar   *sPath = NULL;
    SInt     sFd;
    SInt     sIndex;
    idfMeta *sMeta = NULL;
    idBool   sLock = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;

    IDE_TEST_RAISE((sFd = idf::getUnusedFd()) == IDF_INVALID_HANDLE, fd_error);

    sPath = idf::getpath(aPathName);

    IDE_TEST(sPath == NULL);

    // 파일이 존재하는 경우
    if((sFileID = idf::getFileIDByName(sPath)) != IDF_INVALID_FILE_ID) 
    {
        // O_TRUNC 속성이 있으면 파일의 크기를 0으로 만든다.
        if(aFlag & O_TRUNC)
        {
            sMeta = idf::mMetaList[sFileID].mMeta;

            sMeta->mSize = 0;

            idf::mFdList[sFd].mFileID = sFileID;
            idf::mFdList[sFd].mCursor = 0;

            idf::mMetaList[sFileID].mFileOpenCount++;
        }
        else
        {
            idf::mFdList[sFd].mFileID = sFileID;
            idf::mFdList[sFd].mCursor = 0;

            // 파일을 열은 회수(mFileOpenCount)를 증가하고,
            // idfCore::close() 호출시 하나씩 감소한다.
            idf::mMetaList[sFileID].mFileOpenCount++;
        }
    }
    // 파일이 존재하지 않으면 aFlag를 조사하여 O_CREAT가 있으면 파일을
    // 생성한다. 처음 파일을 생성할 때는 페이지를 할당하지 않는다.
    else
    {
        if(aFlag & O_CREAT) // aFlag에 O_CREAT 속성이 있는 경우
        {
            if((sFileID = idf::getUnusedFileID()) != IDF_INVALID_FILE_ID)
            {
                // 빈 file ID가 존재하면 Meta를 할당하고 파일을 생성한다.
                (void)idf::alloc((void **)&sMeta, ID_SIZEOF(idfMeta));

                idlOS::memset(sMeta, 
                              '\0', 
                              ID_SIZEOF(idfMeta));

                idf::mMetaList[sFileID].mMeta = sMeta;
                idf::mMetaList[sFileID].mFileOpenCount= 1;

                idf::mFdList[sFd].mFileID = sFileID;
                idf::mFdList[sFd].mCursor = 0;

                idlOS::strncpy(sMeta->mFileName, 
                               sPath, 
                               IDF_MAX_FILE_NAME_LEN);

                sMeta->mFileID      = sFileID;
                sMeta->mNumOfPagesU = 0;
                sMeta->mNumOfPagesA = 0;
                sMeta->mSignature   = htonl(0xABCDABCD);

                for(sIndex = 0; sIndex < IDF_DIRECT_MAP_SIZE; sIndex++)
                {
                    sMeta->mDirectPages[sIndex] = 0;
                }

                for(sIndex = 0; sIndex < 16; sIndex++)
                {
                    sMeta->mIndPages[sIndex] = 0;
                }

                sMeta->mSize = 0;

                idf::mMaster.mNumOfFiles++;
            }
            else
            {
                // 빈 file ID가 없으면 파일에 대한 Meta를 생성할 수 없기
                // 때문에 더 이상 파일을 생성할 수 없다.
                (void)idf::freeFd(sFd);

                sFd = IDF_INVALID_HANDLE;

                errno = ENFILE; // File table overflow
            }
        }
        else // 파일이 존재하지 않고, aFlag에 O_CREAT 속성이 없는 경우
        {
            (void)idf::freeFd(sFd);

            sFd = IDF_INVALID_HANDLE;

            errno = ENOENT; // No such file or directory
        }
    }

    if((sFd != IDF_INVALID_HANDLE) && (sMeta != NULL))
    {
        if((aFlag & O_CREAT) || (aFlag & O_TRUNC))
        {
            idf::mIsSync = ID_FALSE;

            // 변경된 로그를 체크한다.
            if(idf::mLogList[sFileID] == 0)
            {
                idf::mLogList[sFileID] = 1;

                idf::mLogListCount++;
            }

            IDE_TEST(idf::masterLog(0) != IDE_SUCCESS);
        }
    }

    if(sPath != NULL)
    {
        (void)idf::free(sPath);

        sPath = NULL;
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    return (PDL_HANDLE)sFd;

    IDE_EXCEPTION(fd_error);
    {
        errno = EMFILE; // Too many open files
    }
    
    IDE_EXCEPTION_END;

    if(sPath != NULL)
    {
        (void)idf::free(sPath);

        sPath = NULL;
    }

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return (PDL_HANDLE)IDF_INVALID_HANDLE;
}

SInt idfCore::close(PDL_HANDLE aFd)
{
    SInt   sFd = (SInt)aFd;
    SInt   sFileID;
    idBool sLock = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;
    
    // Fd가 사용중인지 확인한다. 사용중인 Fd가 아니면 에러를 발생한다.
    IDE_TEST_RAISE((idf::mFdList[sFd].mIsUsed) == IDF_FD_UNUSED, fd_error);

    sFileID = idf::mFdList[sFd].mFileID;

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    // Meta List에 대해서는 단순히 파일을 열은 횟수를 감소한다.
    // File Descriptor List에 mIsUsed가 IDF_FD_USED(1)가 아니면 이미 파일에
    // 대한 Meta를 지운 경우이므로 메타에 대해서는 처리를 하지 않는다.
    // 만약 Meta에 대해 데이터를 변경, 디스크에 저장하면 다른 파일의 Meta를
    // 잘못 변경할 수 있으므로 Meta를 변경하지 않도록 주의한다.
    if(idf::mFdList[sFd].mIsUsed == IDF_FD_USED)
    {
        idf::mMetaList[sFileID].mFileOpenCount--;
    }

    // Fd를 반환하여 다음에 파일을 열 때 사용하도록 한다.
    (void)idf::freeFd(sFd);

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Retrun Value
    //      0 : Success
    //     -1 : Failure
    return 0;

    IDE_EXCEPTION(fd_error);
    {
        errno = EBADF; // Bad file number
    } 
    
    IDE_EXCEPTION_END;

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1;
}

#if defined(VC_WINCE)
PDL_HANDLE idfCore::creat(SChar *aPathName, mode_t /*aMode*/)
#else
PDL_HANDLE idfCore::creat(const SChar *aPathName, mode_t /*aMode*/)
#endif
{
    // 모드는 지원하지 않는다.
    // open()을 호출하면 errno도 open()에서 설정한다.
    // 따라서 단순히 open()호출한 결과만 반환하면 된다.
    PDL_HANDLE sFd = idfCore::open(aPathName, O_CREAT | O_RDWR | O_TRUNC);

    return sFd;
}

ssize_t idfCore::read(PDL_HANDLE aFd, void *aBuf, size_t aCount)
{
    SInt     sFd        = (SInt)aFd;
    SInt     sReadCount = 0;
    SInt     sFileID;
    UInt     sPageIndex;
    UInt     sPageOffset;
    UInt     sReadSize;
    UInt     sPageRemain;
    UInt     sLogicalPage = 0;
    SChar   *sBuf    = (SChar*)aBuf;
    SLong    sCount  = aCount;
    ULong    sCursor = 0;
    idfMeta *sMeta;
    idBool   sLock = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;

    // Fd가 사용중인지 확인한다. 사용중인 Fd가 아니면 에러를 발생한다.
    IDE_TEST_RAISE(idf::mFdList[sFd].mIsUsed != IDF_FD_USED, fd_error);

    sFileID = idf::mFdList[sFd].mFileID;

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    sMeta = idf::mMetaList[sFileID].mMeta;

    // sCursor를 토대로 실제 데이터가 위치한 페이지 주소를 찾는다.
    sCursor = idf::mFdList[sFd].mCursor;

    sPageIndex = sCursor / idf::mPageSize;

    IDE_TEST_RAISE(sCount < 0, invalid_arg_error);

    // 메타에서 파일 크기를 얻어와서 읽을 크기를 재설정한다.
    if((SLong)(sMeta->mSize - sCursor) < sCount)
    {
        sCount = sMeta->mSize - sCursor;
    }

    // 페이지에서 데이터를 읽어오고 읽은 길이를 누적한다.
    if(sCount > 0)
    {
        while(sCount > 0)
        {
            sPageOffset = sCursor % idf::mPageSize;

            sPageRemain = (idf::mPageSize - sPageOffset);

            // 페이지의 남은 공간과 남은 데이터 중 작은 크기만큼 읽는다.
            //     sPageRemain : 페이지 내의 남은 공간
            //     sCount      : 파일 내의 남은 데이터
            sReadSize = (sPageRemain < sCount) ? sPageRemain : sCount;

            if((sLogicalPage = idf::getPageAddrR(sMeta, sFileID, sPageIndex))
               <= 0)
            {
                idlOS::memset(sBuf, 
                              '\0', 
                              sReadSize);
            }
            else
            {
                IDE_TEST(idf::lseekFs(idf::mFd,
                                      sLogicalPage * idf::mPageSize + sPageOffset,
                                      SEEK_SET) == -1);

                IDE_TEST(idf::readFs(idf::mFd, 
                                     sBuf, 
                                     sReadSize) != sReadSize);
            }

            sBuf       += sReadSize;
            sCursor    += sReadSize;
            sReadCount += sReadSize;
            sCount     -= sReadSize;

            // 페이지를 다 읽은 경우 다음 페이지를 읽도록 한다.
            if(((sCursor % idf::mPageSize) == 0) && (sReadSize > 0))
            {
                sPageIndex++;
            }
        }
    }

    idf::mFdList[sFd].mCursor = sCursor;

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //     sReadCount ==  0 : End Of File
    //     sReadCount  >  0 : The number of bytes read
    //     sReadCount == -1 : Failure
    return (ssize_t)sReadCount;

    IDE_EXCEPTION(invalid_arg_error);
    {
        errno = EINVAL;
    }

    IDE_EXCEPTION(fd_error);
    {
        errno = EBADF; // Bad file number
    } 
    
    IDE_EXCEPTION_END;

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1;
}

ssize_t idfCore::write(PDL_HANDLE aFd, const void* aBuf, size_t aCount)
{
    SInt     sFd = (SInt)aFd;
    SInt     sFileID;
    SInt     sPageIndex;
    SInt     sPageOffset;
    SInt     sWriteSize;
    SInt     sPageRemain;
    SInt     sLogicalPage = 0;
    SLong    sCount       = aCount;
    ULong    sCursor      = 0;
    SChar   *sBuf         = (SChar*)aBuf;
    size_t   sWriteCount  = 0;
    idfMeta *sMeta        = NULL;
    idBool   sLock        = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;

    // Fd가 사용중인지 확인한다. 사용중인 Fd가 아니면 에러를 발생한다.
    IDE_TEST_RAISE(idf::mFdList[sFd].mIsUsed != IDF_FD_USED, fd_error);

    sFileID = idf::mFdList[sFd].mFileID;

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    sMeta = idf::mMetaList[sFileID].mMeta;

    // sCursor를 토대로 페이지 인덱스를 얻어온 뒤 
    // 실제 데이터가 위치할 페이지 주소를 찾는다.
    sCursor = idf::mFdList[sFd].mCursor;

    sPageIndex = sCursor / idf::mPageSize;

    IDE_TEST_RAISE(sCount < 0, invalid_arg_error);

    if(sCount > 0)
    {
        idf::mIsSync = ID_FALSE;

        idf::mIsDirty = ID_FALSE;

        // 페이지 인덱스를 이용하여 페이지 주소를 얻어온다.
        sLogicalPage = idf::getPageAddrW(sMeta, sFileID, sPageIndex);

        IDE_TEST_RAISE(sLogicalPage == -1, free_page_error);

        while(sCount > 0)
        {
            sPageOffset = sCursor % idf::mPageSize;

            sPageRemain = (idf::mPageSize - sPageOffset);

            // 페이지의 남은 공간과 남은 데이터 크기 중 작은 크기만큼 쓴다.
            sWriteSize = (sPageRemain < sCount) ? sPageRemain : sCount;

            IDE_TEST(idf::lseekFs(idf::mFd,
                                  sLogicalPage * idf::mPageSize + sPageOffset,
                                  SEEK_SET) == -1);

            IDE_TEST(idf::writeFs(idf::mFd, 
                                  sBuf, 
                                  sWriteSize) != sWriteSize);

            sBuf        += sWriteSize;
            sCursor     += sWriteSize;
            sWriteCount += sWriteSize;
            sCount      -= sWriteSize;

            // 페이지를 다 쓴경우 다음 페이지에 쓰도록 한다.
            if(((sCursor % idf::mPageSize) == 0) && (sWriteSize> 0) && (sCount > 0))
            {
                sPageIndex++;

                IDE_TEST_RAISE((sLogicalPage = idf::getPageAddrW(sMeta,
                                                                 sFileID,
                                                                 sPageIndex))
                               == -1, free_page_error);

            }
        }

        // Meta의 파일 크기 정보를 수정한다.
        if(sMeta->mSize < sCursor)
        {
            sMeta->mSize = sCursor;

            idf::mIsDirty = ID_TRUE;
        }

        // 파일에 데이터를 기록한 경우에만 Meta를 기록한다.
        // Meta가 변경되지 않았으면 Meta를 저장하지 않는다.
        if(idf::mIsDirty == ID_TRUE)
        {
            if(idf::mLogList[sFileID] == 0)
            {
                idf::mLogList[sFileID] = 1;

                idf::mLogListCount++;
            }

            IDE_TEST(idf::masterLog(0) != IDE_SUCCESS);
        }

        // 커서의 위치를 이동한다.
        idf::mFdList[sFd].mCursor = sCursor;
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //     sWriteCount ==  0 : Nothing was written
    //     sWriteCount  >  0 : The number of bytes written
    //     sWriteCount == -1 : Failure
    return (ssize_t)sWriteCount;

    IDE_EXCEPTION(free_page_error);
    {
    }

    IDE_EXCEPTION(invalid_arg_error);
    {
        errno = EINVAL;
    }

    IDE_EXCEPTION(fd_error);
    {
        errno = EBADF; // Bad file number
    } 
    
    IDE_EXCEPTION_END;

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1;
}

PDL_OFF_T idfCore::lseek(PDL_HANDLE aFd, PDL_OFF_T aOffset, SInt aWhence)
{
    SInt      sFd = (SInt)aFd;
    SInt      sFileID;
    PDL_OFF_T sCursor;
    idBool    sLock = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;

    // Fd가 사용중인지 확인한다. 사용중인 Fd가 아니면 에러를 발생한다.
    IDE_TEST_RAISE(idf::mFdList[sFd].mIsUsed != IDF_FD_USED, fd_error);

    sFileID = idf::mFdList[sFd].mFileID;

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    // aWhence에 따라서 위치를 지정한다.
    // SEEK_SET, SEEK_CUR, SEEK_END 이외의 값이 오면 에러를 발생한다.
    switch(aWhence)
    {
    case SEEK_SET:
        sCursor = 0;

        break;
    case SEEK_CUR:
        sCursor = idf::mFdList[sFd].mCursor;

        break;
    case SEEK_END:
        sCursor = idf::mMetaList[sFileID].mMeta->mSize;

        break;
    default:
        errno = EINVAL; // Invalid argument

        sCursor = -1;

        break;
    }

    if(sCursor >= 0)
    {
        sCursor += aOffset;

        if(sCursor < 0)
        {
            errno = EINVAL; // Invalid argument

            sCursor = -1;
        }
        else
        {
            idf::mFdList[sFd].mCursor = sCursor;
        }
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //     sCursor  >  0 : Success
    //     sCursor == -1 : Failure
    return sCursor;

    IDE_EXCEPTION(fd_error);
    {
        errno = EBADF; // Bad file number
    } 

    IDE_EXCEPTION_END;

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1; 
}

SInt idfCore::fstat(PDL_HANDLE aFd, PDL_stat *aBuf)
{
    SInt   sFd = (SInt)aFd;
    SInt   sFileID;
    idBool sLock = ID_FALSE;

    IDE_TEST_RAISE(aBuf == NULL, bad_address_error);

    (void)idf::lock();
    sLock = ID_TRUE;
    
    // Fd가 사용중인지 확인한다. 사용중인 Fd가 아니면 에러를 발생한다.
    IDE_TEST_RAISE(idf::mFdList[sFd].mIsUsed != IDF_FD_USED, fd_error);

    sFileID = idf::mFdList[sFd].mFileID;

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    aBuf->st_size = (idf::mMetaList[sFileID].mMeta)->mSize;

     sLock = ID_FALSE;
     (void)idf::unlock();

    // Return Value
    //      0 : Success
    //     -1 : Failure
    return 0;

    IDE_EXCEPTION(bad_address_error);
    {
        errno = EFAULT; // Bad address
    } 

    IDE_EXCEPTION(fd_error);
    {
        errno = EBADF; // Bad file number
    } 
    
    IDE_EXCEPTION_END;

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1;
}

SInt idfCore::unlink(const SChar* aPathName)
{
    SInt   sFileID;
    SInt   sRc   = -1;
    SChar *sPath = NULL;
    idBool sLock = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;

    sPath = idf::getpath(aPathName);

    IDE_TEST(sPath == NULL);

    sFileID = idf::getFileIDByName(sPath);

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    if(idf::freeMeta(sFileID) == IDE_SUCCESS)
    {
        sRc = 0;
    }
    else
    {
        sRc = -1;
    }

    if(idf::mLogList[sFileID] == 0)
    {
        idf::mLogList[sFileID] = 1;

        idf::mLogListCount++;
    }

    IDE_TEST(idf::masterLog(1) != IDE_SUCCESS);

    IDE_TEST(idf::fsyncFs(idf::mFd) == -1);

    IDE_TEST(idf::initLog() != IDE_SUCCESS);

    idf::mIsSync = ID_TRUE;

    if(sPath != NULL)
    {
        (void)idf::free(sPath);

        sPath = NULL;
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //     sRc ==  0 : Success
    //     sRc == -1 : Failure
    return sRc;

    IDE_EXCEPTION(fd_error);
    {
        errno = ENOENT; // No such file or directory
    }
    
    IDE_EXCEPTION_END;

    if(sPath != NULL)
    {
        (void)idf::free(sPath);

        sPath = NULL;
    }

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1; 
}

SInt idfCore::rename(const SChar *aOldpath, const SChar *aNewpath)
{
    SInt     sFileID;
    SInt     sNewFileID;
    SChar   *sOldPath = NULL;
    SChar   *sNewPath = NULL;
    idfMeta *sMeta    = NULL;
    idBool   sLock    = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;

    sOldPath = idf::getpath(aOldpath);
    sNewPath = idf::getpath(aNewpath);

    IDE_TEST(sOldPath == NULL);
    IDE_TEST(sNewPath == NULL);

    sFileID    = idf::getFileIDByName(sOldPath);
    sNewFileID = idf::getFileIDByName(sNewPath);

    // aOldPath에 해당하는 파일이 없다면 errno에 ENOENT를 설정학고
    // -1을 반환한다.
    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    idf::mIsSync = ID_FALSE;

    if(sNewFileID != IDF_INVALID_FILE_ID)
    {
        // aNewPath에 해당하는 파일이 존재하면 파일을 삭제한다.
        IDE_TEST(idf::freeMeta(sNewFileID) != IDE_SUCCESS);

        if(idf::mLogList[sNewFileID] == 0)
        {
            idf::mLogList[sNewFileID] = 1;

            idf::mLogListCount++;
        }
    }

    sMeta = idf::mMetaList[sFileID].mMeta;

    idlOS::strncpy(sMeta->mFileName, 
                   sNewPath, 
                   IDF_MAX_FILE_NAME_LEN);

    if(idf::mLogList[sFileID] == 0)
    {
        idf::mLogList[sFileID] = 1;

        idf::mLogListCount++;
    }

    if(sNewFileID != IDF_INVALID_FILE_ID)
    {
        IDE_TEST(idf::masterLog(1) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(idf::masterLog(0) != IDE_SUCCESS);
    }

    if(sOldPath != NULL)
    {
        (void)idf::free(sOldPath);

        sOldPath = NULL;
    }

    if(sNewPath != NULL)
    {
        (void)idf::free(sNewPath);

        sNewPath = NULL;
    }

     sLock = ID_FALSE;
     (void)idf::unlock();

    // Return Value
    //       0 : Success
    //      -1 : Failure
    return 0;

    IDE_EXCEPTION(fd_error);
    {
        errno = ENOENT; // No such file or directory
    }
    
    IDE_EXCEPTION_END;

    if(sOldPath != NULL)
    {
        (void)idf::free(sOldPath);

        sOldPath = NULL;
    }

    if(sNewPath != NULL)
    {
        (void)idf::free(sNewPath);

        sNewPath = NULL;
    }

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1; 
}

SInt idfCore::access(const SChar* aPathName, SInt /*aMode*/)
{
    SInt   sFileID;
    SChar *sPathName = NULL;
    idBool sLock     = ID_FALSE;

    // access 함수에서 aMode는 사용하지 않는다.

    (void)idf::lock();
    sLock = ID_TRUE;

    // aPathName이 NULL이면 errno에 EINVAL(Invalid argument)를 설정하고
    // NULL을 반환한다.
    IDE_TEST_RAISE(aPathName == NULL, invalid_arg_error);

    sPathName = idf::getpath(aPathName);

    IDE_TEST(sPathName == NULL);

    IDE_TEST_RAISE(idlOS::strcmp(sPathName, "") == 0, not_exist_error);

    // sFileID의 값이 IDF_INVALID_FILE_ID가 아니면 0을 반환한다.
    // 만약 IDF_INVALID_FILE_ID이면 해당 sPath가 디렉토리인지 확인한다.
    // sPath에 해당하는 파일 또는 디렉토리가 존재하지 않으면 errno에 ENOENT
    // 를 설정하고 -1을 반환한다.
    if((sFileID = idf::getFileIDByName(sPathName)) == IDF_INVALID_FILE_ID)
    {
        // idf::isDir()이 IDE_SUCCESS를 반환하면 디렉토리이다.
        IDE_TEST_RAISE(idf::isDir(sPathName) != IDE_SUCCESS, not_exist_error);
    }

    if(sPathName != NULL)
    {
        (void)idf::free(sPathName);

        sPathName = NULL;
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //      0 : Success
    //     -1 : Failure
    return 0;

    IDE_EXCEPTION(invalid_arg_error);
    {
        errno = EINVAL; // Invalid argument
    }

    IDE_EXCEPTION(not_exist_error);
    {
        errno = ENOENT; // No such file or directory
    }
    
    IDE_EXCEPTION_END;

    if(sPathName != NULL)
    {
        (void)idf::free(sPathName);

        sPathName = NULL;
    }

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1; 
}

SInt idfCore::fsync(PDL_HANDLE aFd)
{
    SInt   sFileID;
    SInt   sFd   = (SInt)aFd;
    idBool sLock = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;
    
    // Fd가 사용중인지 확인한다. 사용중인 Fd가 아니면 에러를 발생한다.
    IDE_TEST_RAISE(idf::mFdList[sFd].mIsUsed != IDF_FD_USED, fd_error);

    sFileID = idf::mFdList[sFd].mFileID;

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    if(idf::mIsSync == ID_FALSE)
    {
        if(idf::mTransCount > 0)
        {
            IDE_TEST(idf::masterLog(1) != IDE_SUCCESS);
        }

        IDE_TEST(idf::fsyncFs(idf::mFd) == -1);

        IDE_TEST(idf::initLog() != IDE_SUCCESS);

        idf::mIsSync = ID_TRUE;
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //      0 : Success
    //     -1 : Failure
    return 0;

    IDE_EXCEPTION(fd_error);
    {
        errno = EBADF; // Bad file number
    } 
    
    IDE_EXCEPTION_END;

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1;
}

SInt idfCore::ftruncate(PDL_HANDLE aFd, PDL_OFF_T aLength)
{
    SInt     sFileID;
    SInt     sFd = (SInt)aFd;
    idfMeta *sMeta;
    idBool   sLock = ID_FALSE;

    (void)idf::lock();
    sLock = ID_TRUE;
    
    // Fd가 사용중인지 확인한다. 사용중인 Fd가 아니면 에러를 발생한다.
    IDE_TEST_RAISE(idf::mFdList[sFd].mIsUsed != IDF_FD_USED, fd_error);

    // aLength는 반드시 0보다 크거나 같은 값이어야 한다.
    IDE_TEST_RAISE(aLength < 0, invalid_arg_error);

    sFileID = idf::mFdList[sFd].mFileID;

    IDE_TEST_RAISE(sFileID == IDF_INVALID_FILE_ID, fd_error);

    // aLength가 파일의 크기보다 큰 경우 파일의 크기가 늘어나고,
    // aLength가 파일의 크기보다 작은 경우 파일의 크기가 줄어든다.
    // 파일이 늘어난 부분에 대해 read()를 하면 버퍼에 '\0'이 채워진다.

    // aLength가 0이면 길이의 변화가 없으므로 아무 작업도 하지 않는다.
    if(aLength > 0)
    {
        idf::mIsSync = ID_FALSE;

        sMeta = idf::mMetaList[sFileID].mMeta;

        sMeta->mSize = aLength;

        if(idf::mLogList[sFileID] == 0)
        {
            idf::mLogList[sFileID] = 1;

            idf::mLogListCount++;
        }

        IDE_TEST(idf::masterLog(0) != IDE_SUCCESS);
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //      0 : Success
    //     -1 : Failure
    return 0;

    IDE_EXCEPTION(invalid_arg_error);
    {
        errno = EINVAL; // Invalid argument
    }

    IDE_EXCEPTION(fd_error);
    {
        errno = EBADF; // Bad file number
    } 
    
    IDE_EXCEPTION_END;

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return -1;
}

DIR* idfCore::opendir(const SChar* aName)
{
    SChar  *sName = NULL;
    idfDir *sDir;
    idBool  sLock = ID_FALSE;

    // aName이 NULL이면 errno에 EINVAL(Invalid argument)를 설정하고
    // NULL을 반환한다.
    IDE_TEST_RAISE(aName == NULL, invalid_arg_error);

    (void)idf::lock();
    sLock = ID_TRUE;

    sName = idf::getpath(aName);

    IDE_TEST(sName == NULL);

    IDE_TEST_RAISE(idlOS::strcmp(sName, "") == 0, not_exist_error);

    IDE_TEST_RAISE(idf::isDir(sName) == IDE_FAILURE, not_exist_error);

    sDir = (idfDir*)idf::getDir(sName);

    if(sName != NULL)
    {
        (void)idf::free(sName);

        sName = NULL;
    }

    sLock = ID_FALSE;
    (void)idf::unlock();

    // Return Value
    //     sDir != NULL : Success
    //     sDir == NULL : Failure
    return (DIR*)sDir;

    IDE_EXCEPTION(invalid_arg_error);
    {
        errno = EINVAL; // Invalid argument
    }

    IDE_EXCEPTION(not_exist_error);
    {
        errno = ENOENT; // No such file or directory
    }
    
    IDE_EXCEPTION_END;

    if(sName != NULL)
    {
        (void)idf::free(sName);

        sName = NULL;
    }

    if(sLock == ID_TRUE)
    {
        sLock = ID_FALSE;
        (void)idf::unlock();
    }

    return NULL;
}

SInt idfCore::readdir_r(DIR            *aDirp,
                        struct dirent  *aEntry,
                        struct dirent **aResult)
{
    SInt       sRc = -1;
    SInt       sIndex;
    SInt       sDirIndex;
    SInt       sDirCount;
    idfDir    *sDir = (idfDir*)aDirp;
    dirent    *sDirent;
    idfDirent *sDirentP;

    sDirIndex = sDir->mDirIndex;
    sDirCount = sDir->mDirCount;

    if(sDirIndex == sDirCount)
    {
        // DIR 객체에서 dirent객체를 모두 읽었다.
        *aResult = NULL;
    }
    else
    {
        sDirentP = (idfDirent*)sDir->mFirst;

        for(sIndex = 0; sIndex < sDirIndex; sIndex++)
        {
            sDirentP = sDirentP->mNext;
        }

        sDirent = (dirent*)sDirentP->mDirent;

        idlOS::strncpy(aEntry->d_name,
                       sDirent->d_name,
                       idlOS::strlen(sDirent->d_name));

        sDir->mDirIndex++;

        *aResult = sDirent;

        sRc = 0;
    }

    // Return Value
    //     sRc ==  0 : Success
    //     sRc == -1 : Failure
    return sRc;
}

struct dirent * idfCore::readdir(DIR *aDirp)
{
    SInt       sIndex;
    SInt       sDirIndex;
    SInt       sDirCount;
    idfDir    *sDir = (idfDir*)aDirp;
    dirent    *sDirent = NULL;
    idfDirent *sDirentP;

    sDirIndex = sDir->mDirIndex;
    sDirCount = sDir->mDirCount;

    if(sDirIndex == sDirCount)
    {
        // DIR 객체에서 dirent객체를 모두 읽었다.
        sDirent = NULL;
    }
    else
    {
        sDirentP = (idfDirent*)sDir->mFirst;

        for(sIndex = 0; sIndex < sDirIndex; sIndex++)
        {
            sDirentP = sDirentP->mNext;
        }

        sDirent = (dirent*)sDirentP->mDirent;

        sDir->mDirIndex++;
    }

    // Return Value
    //     sDirent != NULL : Success
    //     sDirent == NULL : Failure
    return sDirent;
}

void idfCore::closedir(DIR *aDirp)
{
    idfDir    *sDir = (idfDir*)aDirp;
    idfDirent *sDirentP;
    idfDirent *sTemp;

    sDirentP = sDir->mFirst;

    while(sDirentP != NULL)
    {
        sTemp = sDirentP->mNext;

        (void)idf::free(sDirentP->mDirent);
        (void)idf::free(sDirentP);

        sDirentP = sTemp;
    }

    // 1. Dirent 객체를 반환한다.
    // 2. Dir 객체를 반환한다.

    (void)idf::free(sDir);
}

SLong idfCore::getDiskFreeSpace(const SChar* /*aPathName*/)
{
    SLong  sRc = -1;
    SLong  sTotal;
    SLong  sUsed;

    // 파티션이 한개이므로 aPathName은 사용하지 않는다.
    // aPathName에 관계없이 남은 디스크 용량을 반환한다.
    (void)idf::lock();

    // 전체 크기
    sTotal = ((SLong)idf::mPageSize * idf::mPageNum);

    // 사용한 크기
    sUsed = ((SLong)idf::mPageSize * idf::mMaster.mAllocedPages);

    // 사용 가능한 크기
    sRc = sTotal - sUsed;

    (void)idf::unlock();

    // Return Value
    //    sRc ==  0 : Disk Has No Free Space
    //    sRc  >  0 : Disk's Free Space
    //    sRc == -1 : Failure
    return sRc;
}

PDL_OFF_T idfCore::filesize(PDL_HANDLE handle)
{
    PDL_stat sb;

    return (idf::fstat(handle, &sb) == -1) ? -1 : (PDL_OFF_T) sb.st_size;
}
