/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFileAIO.h 35244 2009-09-02 23:52:47Z orc $
 **********************************************************************/

/***********************************************************************
 * This file is for Asynchronous I/O Class
 **********************************************************************/

#ifndef _O_IDU_FILE_AIO_H_
#define _O_IDU_FILE_AIO_H_ 1

#include <idu.h>
#include <idl.h>

#define IDU_AIO_FILE_AGAIN_SLEEP_TIME 1000

class iduFileAIO
{
    PDL_HANDLE    mHandle;
#if !defined(VC_WIN32) && !defined(ANDROID) && !defined(SYMBIAN)
    struct aiocb  mCB; /* AIO Control Block */
#endif
    SInt          mErrno;
public:
    iduFileAIO()  {}
    ~iduFileAIO() {}
    IDE_RC     initialize(PDL_HANDLE aHandle = PDL_INVALID_HANDLE);
    IDE_RC     destroy();

    PDL_HANDLE getFileHandle();

    IDE_RC     write(PDL_OFF_T  a_where,
                     void*  a_buffer,
                     size_t a_size);

    IDE_RC     read (PDL_OFF_T  a_where,
                     void*  a_buffer,
                     size_t a_size);

    IDE_RC     write(PDL_HANDLE,
                     PDL_OFF_T  a_where,
                     void*  a_buffer,
                     size_t a_size);

    IDE_RC     read (PDL_HANDLE,
                     PDL_OFF_T  a_where,
                     void*  a_buffer,
                     size_t a_size);

    IDE_RC     sync();

    IDE_RC     join(); // resource collection : resemble thread join
    idBool     isFinish(SInt *aErrorCode);
    IDE_RC     waitForFinish(SInt *aErrorCode);

    SInt       getErrno() { return mErrno; }


};

inline PDL_HANDLE iduFileAIO::getFileHandle()
{
    return mHandle;
}

#endif  // _O_FILE_H_

