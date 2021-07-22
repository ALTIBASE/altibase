/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFileAIO.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduFileAIO.h>

/* ------------------------------------------------
 *  AIX 4.3, 5.1의 경우 Lagacy AIO를 지원한다.
 *  이를 위해 별도의 함수 set으로 분리한다.
 *  aio 관련 함수를 id 혹은 pd 라이브러리에
 *  넣지 않은 이유는 구현과정에서의 상이함으로 인해
 *  구현 및 테스트를 완료하고,
 *  이후에 넣기 위함이다.  by gamestar : 2005/5/6
 * ----------------------------------------------*/

#if defined(IBM_AIX) && ( ((OS_MAJORVER == 5) && (OS_MINORVER == 1)) || \
                          ((OS_MAJORVER == 4) && (OS_MINORVER == 3)))

#define ALTIBASE_USE_AIX_SPECIFIC_AIO 1

#endif

#if !defined(VC_WIN32) && !defined(ANDROID) && !defined(SYMBIAN)
IDE_RC iduFileAIO::initialize(PDL_HANDLE aHandle)
{
    mHandle = aHandle;

    idlOS::memset(&mCB, 0, sizeof(mCB));

    return IDE_SUCCESS;
}

IDE_RC iduFileAIO::destroy()
{
    IDE_ASSERT(mHandle != IDL_INVALID_HANDLE);

    return IDE_SUCCESS;
}

IDE_RC iduFileAIO::read (PDL_HANDLE aHandle,
                         PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    mHandle = aHandle;
    return read(a_where, a_buffer, a_size);
}

IDE_RC iduFileAIO::read (PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    IDE_ASSERT ( mHandle != IDL_INVALID_HANDLE );

    mErrno = 0;

#if defined(ALTIBASE_USE_AIX_SPECIFIC_AIO)
    /* Nothing to do in FileDescriptor*/
#else
    mCB.aio_fildes = mHandle;
#endif

    mCB.aio_offset = a_where;
    mCB.aio_buf    = (SChar *) a_buffer;
    mCB.aio_nbytes = a_size;

#if defined(ALTIBASE_USE_AIX_SPECIFIC_AIO)
    mCB.aio_flag = 0;

    IDE_TEST_RAISE(aio_read( mHandle, &mCB ) != 0, read_error);

#else
    mCB.aio_sigevent.sigev_notify = SIGEV_NONE;
    IDE_TEST_RAISE(aio_read( &mCB ) != 0, read_error);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(read_error);
    {
        mErrno = errno;
        IDE_SET(ideSetErrorCode(idERR_ABORT_ASYNC_IO_READ_FAILED, errno));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFileAIO::write(PDL_HANDLE aHandle,
                         PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    mHandle = aHandle;
    return write(a_where, a_buffer, a_size);
}

IDE_RC iduFileAIO::write (PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    IDE_ASSERT ( mHandle != IDL_INVALID_HANDLE );

    mErrno = 0;

#if defined(ALTIBASE_USE_AIX_SPECIFIC_AIO)
    /* Nothing to do in FileDescriptor*/
#else
    mCB.aio_fildes = mHandle;
#endif

    mCB.aio_offset = a_where;
    mCB.aio_buf    = (SChar *) a_buffer;
    mCB.aio_nbytes = a_size;

#if defined(ALTIBASE_USE_AIX_SPECIFIC_AIO)
    mCB.aio_flag = 0;
    IDE_TEST_RAISE(aio_write( mHandle, &mCB ) != 0, write_error);
#else
    mCB.aio_sigevent.sigev_notify = SIGEV_NONE;
    IDE_TEST_RAISE(aio_write( &mCB ) != 0, write_error);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(write_error);
    {
        mErrno = errno;

        IDE_SET(ideSetErrorCode(idERR_ABORT_ASYNC_IO_WRITE_FAILED, errno));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFileAIO::sync()
{
    IDE_ASSERT ( mHandle != IDL_INVALID_HANDLE );
    IDE_TEST_RAISE( idlOS::fsync(mHandle) != 0, sync_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(sync_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_SyncError, "AIO Sync"));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

idBool     iduFileAIO::isFinish(SInt *aErrorCode)
{
    SInt sErrno;

    IDE_ASSERT ( mHandle != IDL_INVALID_HANDLE );

    sErrno = aio_error(&mCB);

    if (aErrorCode != NULL)
    {
        *aErrorCode = sErrno;
    }

    return ( (sErrno == EINPROGRESS) ? ID_FALSE : ID_TRUE);
}


IDE_RC iduFileAIO::join()
{
    IDE_ASSERT(aio_error(&mCB) != EINPROGRESS);

    IDE_TEST_RAISE(aio_return(&mCB) < 0, return_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(return_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_ASYNC_IO_RETURN_FAILED,
                                (SInt)(aio_return(&mCB))));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}


IDE_RC iduFileAIO::waitForFinish(SInt *aErrorCode)
{
    PDL_Time_Value sSleep;
    SInt           sState = 0;

    sSleep.initialize(0, IDU_AIO_FILE_AGAIN_SLEEP_TIME);
    while(isFinish(aErrorCode) == ID_FALSE)
    {
        idlOS::sleep(sSleep);
    }
    sState = 1;
    IDE_TEST(join() != IDE_SUCCESS); // resource collect

    IDE_TEST_RAISE(*aErrorCode != 0, op_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(op_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_ASYNC_IO_FAILED, (SInt)(*aErrorCode)));
    }
    IDE_EXCEPTION_END;
    if (sState == 1)
    {
        IDE_ASSERT(join() == IDE_SUCCESS);
    }
    return IDE_FAILURE;
}
#else
IDE_RC iduFileAIO::initialize(PDL_HANDLE aHandle)
{
    return IDE_FAILURE;
}

IDE_RC iduFileAIO::destroy()
{
    return IDE_FAILURE;
}

IDE_RC iduFileAIO::read (PDL_HANDLE aHandle,
                         PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    return IDE_FAILURE;
}

IDE_RC iduFileAIO::read (PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    return IDE_FAILURE;
}

IDE_RC iduFileAIO::write(PDL_HANDLE aHandle,
                         PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    return IDE_FAILURE;
}

IDE_RC iduFileAIO::write (PDL_OFF_T  a_where,
                         void*  a_buffer,
                         size_t a_size)
{
    return IDE_FAILURE;
}

IDE_RC iduFileAIO::sync()
{
    return IDE_FAILURE;
}

idBool iduFileAIO::isFinish(SInt *aErrorCode)
{
    return ID_FALSE;
}


IDE_RC iduFileAIO::join()
{
    return IDE_FAILURE;
}


IDE_RC iduFileAIO::waitForFinish(SInt *aErrorCode)
{
    return IDE_FAILURE;
}
#endif
