/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: ideMsgLog.cpp 91286 2021-07-21 02:21:41Z kclee $
 **********************************************************************/

/***********************************************************************
 * NAME
 *  ideMsgLog.cpp
 *
 * DESCRIPTION
 *  This file defines Error Log.
 *
 *   !!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!
 *   Don't Use IDE_ASSERT() in this file.
 **********************************************************************/

#include <acpAtomic.h>
#include <acpPrintf.h>
#include <acpThr.h>
#include <acpTypes.h>
#include <idl.h>
#include <ideCallback.h>
#include <ideErrorMgr.h>
#include <iduVersion.h>
#include <iduMemDefs.h>
#include <iduMemMgr.h>
#include <ideMsgLog.h>
#include <idtContainer.h>

#if defined(IDE_MSGLOG_SIMULATE_STARVATION)
#include <acpRand.h>
#include <acpSleep.h>
#endif

#if defined(ITRON)
#include <t_services.h>
#endif

/*
 *  TRACE Logging Format
 *  {
 *      [2006/02/25 03:33:11] [Thread-19223, SessID-01] [Level-32]
 *      Message
 *  }

 *  ^^^^ <= padding
 *  {}   <= begin/end block
 *  []   <= sub begin/end block
 *
 */

#define IDE_MSGLOG_HEADER_NUMBER "* ALTIBASE LOGFILE-"

#define IDE_MSGLOG_CONTENT_PAD     ""
#define IDE_MSGLOG_CONTENT_PAD_LEN (sizeof(IDE_MSGLOG_CONTENT_PAD) - 1)

#define IDE_MSGLOG_DEFAULT_LOG_SIZE ( 2*1024*1024 )
#define IDE_MSGLOG_MAX_MESSAGE_SIZE ( mSize - 128 ) /* size minus the
                                                     * approximate
                                                     * header size */

/* Helpers for ideMsgLog::logFindEndOffset() method */
#define IDE_MSG_LOG_MID( beg, end ) ( beg + ( ( ( end ) - ( beg ) ) >> 1 ) )
#define IDE_MSG_LOG_IS_RESERVE_CHAR( ch ) ( ( ( ch ) == IDE_RESERVE_FILL_CHAR ) || \
                                            ( ( ch ) == '\0' ) )

/*
 * XDB DA applications require write access to trace log and they may
 * run from a different account than the server process.
 */
#if defined( ALTIBASE_PRODUCT_HDB )
# define IDE_MSGLOG_FILE_PERM S_IRUSR | S_IWUSR | S_IRGRP
#else
# define IDE_MSGLOG_FILE_PERM 0666 /* rwrwrw */
#endif

UInt ideMsgLog::mPermission = IDE_MSGLOG_FILE_PERM;

IDE_RC ideMsgLog::setPermission(const UInt aPerm)
{
    UInt sUsr;
    UInt sGrp;
    UInt sOth;

    sOth = aPerm % 10;
    sGrp = (aPerm / 10) % 10;
    sUsr = (aPerm / 100) % 10;

    IDE_TEST(sOth >= 8);
    IDE_TEST(sGrp >= 8);
    IDE_TEST(sUsr >= 8);

    mPermission = (sUsr << 6) | (sGrp << 3) | sOth;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC ideMsgLog::initialize(const ideLogModule aSelf,
                             const SChar*       aPath,
                             const size_t       aFileSize,
                             const UInt         aMaxNumber,
                             const idBool       aEnabled)
{
    IDE_ASSERT(aSelf < IDE_LOG_MAX_MODULE);

    mFD         = PDL_INVALID_HANDLE;
    mSelf       = aSelf;
    mSize       = aFileSize==0? IDE_MSGLOG_DEFAULT_LOG_SIZE:aFileSize;
    mMaxNumber  = aMaxNumber;
    mCurNumber  = 0;
    mEnabled    = aEnabled;

    mRotating   = 0;

    (void)idlOS::strncpy(mPath, aPath, ID_MAX_FILE_NAME);

    return IDE_SUCCESS;
}

IDE_RC ideMsgLog::destroy(void)
{
    if(mEnabled == ID_TRUE)
    {
        if(mFD != PDL_INVALID_HANDLE)
        {
            (void)close();
        }
        else
        {
            /* fall through */
        }
    }
    else
    {
        /* pass */
    }

    /*
     * Additional destruction will be added here, if needed
     */

    return IDE_SUCCESS;
}

IDE_RC ideMsgLog::open(idBool aDebug)
{
    mDebug = aDebug;

    if(mEnabled == ID_TRUE)
    {
        mFD = idlOS::open(mPath, O_APPEND | O_RDWR);
        if(mFD == PDL_INVALID_HANDLE)
        {
            /*
             * File does not exists
             * Try create
             */

            IDE_TEST(createFileAndHeader() != IDE_SUCCESS);
        }
        else
        {
            /*
             * Check log file header
             */

            if(checkHeader() != ID_TRUE)
            {
                (void)close();
                mCurNumber = 0;
                IDE_TEST(createFileAndHeader() != IDE_SUCCESS);
            }
            else
            {
                /* Success */
                (void)idlOS::fchmod((int)mFD, mPermission);
            }
        }

#if defined(ALTI_CFG_OS_WINDOWS)
#else
        if(mSelf == IDE_ERR && aDebug == ID_FALSE)
        {
            IDE_TEST(idlOS::dup2(mFD, 1) == -1);
            IDE_TEST(idlOS::dup2(mFD, 2) == -1);
        }
        else
        {
            /*
             * In debug running state : 'altibase -n',
             * do not duplicate altibase_error.log with stdout, stderr
             */
        }
#endif
        
        (void)idlOS::lseek(mFD, 0, SEEK_END);
    }
    else
    {
        /* Pass logging */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC ideMsgLog::close(void)
{
    if(mEnabled == ID_TRUE)
    {
        (void)idlOS::close( mFD );
        mFD = PDL_INVALID_HANDLE;
    }
    else
    {
        /* pass */
    }

    return IDE_SUCCESS;
}

IDE_RC ideMsgLog::flush(void)
{
    if(mEnabled == ID_TRUE)
    {
        IDE_TEST(idlOS::fsync(mFD) != 0);
    }
    else
    {
        /* pass */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC ideMsgLog::logBody(const SChar* aMsg, const size_t sLen)
{
    if(mEnabled == ID_TRUE)
    {
        IDE_TEST(rotate() != IDE_SUCCESS);
        IDE_TEST(idlOS::write(mFD, aMsg, sLen) != (ssize_t)sLen);
        unrotate();
    }
    else
    {
        /* pass */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    unrotate();
    return IDE_FAILURE;
}

IDE_RC ideMsgLog::logBody(const ideLogEntry& aEntry)
{
    if(mEnabled == ID_TRUE)
    {
        IDE_TEST(logBody(aEntry.mBuffer, aEntry.mLength) != IDE_SUCCESS);
    }
    else
    {
        /* pass */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

idBool ideMsgLog::checkExist(void)
{
    IDE_TEST(mFD == PDL_INVALID_HANDLE);
    IDE_TEST(idlOS::access(mPath, R_OK | W_OK | F_OK) != 0);

    return ID_TRUE;

    IDE_EXCEPTION_END;
    return ID_FALSE;
}

idBool ideMsgLog::isSameFile( PDL_HANDLE    aFD,
                              SChar       * aPath )
{
    PDL_stat        sInfoFromFD;
    PDL_stat        sInfoFromPath;

    IDE_TEST( idlOS::fstat( aFD, &sInfoFromFD ) != 0 );

    IDE_TEST( idlOS::stat( aPath, &sInfoFromPath ) != 0 );

    /* check disk drive number */
    IDE_TEST( sInfoFromFD.st_dev != sInfoFromPath.st_dev );

    /* check inode */
    IDE_TEST( sInfoFromFD.st_ino != sInfoFromPath.st_ino );

    return ID_TRUE;

    IDE_EXCEPTION_END;

    return ID_FALSE;
}

IDE_RC ideMsgLog::rotate(void)
{
    idBool  sNeedWriteMessage = ID_FALSE;

    if(mEnabled == ID_TRUE)
    {
        size_t sOffset;
        SInt sRotating;

        /* For IDE_ERR, do not perform rotation */
        IDE_TEST_RAISE(mSelf == IDE_ERR, IAMERR);

        sRotating = acpAtomicCas32(&mRotating, 1, 0);

        if(sRotating == 1)
        {
            /* spin */
            do
            {
                idlOS::thr_yield();
                sRotating = acpAtomicCas32(&mRotating, 1, 0);
            } while(sRotating == 1);
        }
        else
        {
            /* fall through */
        }

        IDL_MEM_BARRIER; //BUG-49155

        if(checkExist() == ID_FALSE)
        {
            IDE_TEST(createFileAndHeader() != IDE_SUCCESS);
        }
        else
        {
            if ( isSameFile( mFD, mPath ) == ID_TRUE )
            {
                /* do nothing */
            }
            else
            {
                /* file is modified so close and open */
                (void)close();
                IDE_TEST( open( mDebug ) != IDE_SUCCESS );

                sNeedWriteMessage = ID_TRUE;
            }

            if(mMaxNumber > 0)
            {
                sOffset = (size_t)idlOS::lseek(mFD, 0, SEEK_END);

                if(sOffset >= mSize)
                {
                    IDE_TEST(closeAndRename() != IDE_SUCCESS);
                    mCurNumber = (mCurNumber + 1) % mMaxNumber;
                    IDE_TEST(createFileAndHeader() != IDE_SUCCESS);
                }
                else
                {
                    /* do not create file */
                }
            }
            else
            {
                /* fall through */
            }

            if ( sNeedWriteMessage == ID_TRUE )
            {
                IDE_TEST( writeWarningMessage() != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        /* pass */
    }

    /* For IDE_ERR, do not perform rotation */
    IDE_EXCEPTION_CONT(IAMERR);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    unrotate();
    return IDE_FAILURE;
}

IDE_RC ideMsgLog::writeWarningMessage( void )
{
    SChar   sMessage[128] = { 0, };
    UInt    sLength = 0;

    sLength = idlOS::snprintf( sMessage, 
                               ID_SIZEOF( sMessage ),
                               "\n\n!! WARNING : trc file was modified !!\n\n" );

    IDE_TEST( idlOS::write( mFD, sMessage, sLength ) == -1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC ideMsgLog::createFileAndHeader(void)
{
    if(mEnabled == ID_TRUE)
    {
        SInt    sLen;
        SChar   sHeader[1024];

        mFD = idlOS::open(mPath, O_CREAT | O_RDWR, mPermission);
        IDE_TEST_RAISE( mFD == PDL_INVALID_HANDLE, ERR_OPEN );

        sLen = idlOS::snprintf(sHeader, sizeof(sHeader),
                               IDE_MSGLOG_HEADER_NUMBER"%"ID_UINT32_FMT
                               " (%s) : %s %s\n",
                               mCurNumber,
                               iduVersionString,
                               iduGetSystemInfoString(),
                               iduGetProductionTimeString());

        IDE_TEST_RAISE( idlOS::write(mFD, sHeader, sLen) == -1, ERR_WRITE );
    }
    else
    {
        /* pass */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OPEN )
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_SysOpen, mPath ) );
    }
    IDE_EXCEPTION( ERR_WRITE )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_SysWrite, mPath ) );
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC ideMsgLog::closeAndRename(void)
{
    if(mEnabled == ID_TRUE)
    {
        SChar   sNewPath[ID_MAX_FILE_NAME];

        (void)close();

        idlOS::snprintf(sNewPath, ID_MAX_FILE_NAME, "%s-%"ID_UINT32_FMT,
                        mPath, mCurNumber);

        (void)idlOS::unlink(sNewPath);
        (void)idlOS::rename(mPath, sNewPath);
    }
    else
    {
        /* pass */
    }

    return IDE_SUCCESS;
}

idBool ideMsgLog::checkHeader(void)
{
    if(mEnabled == ID_TRUE)
    {
        SInt    sLen;
        SChar   sHeader[1024];

        IDE_TEST(mFD == PDL_INVALID_HANDLE);

        IDE_TEST(idlOS::read(mFD, sHeader, sizeof(sHeader)) == -1);

        sLen = idlOS::strlen(IDE_MSGLOG_HEADER_NUMBER);
        IDE_TEST(idlOS::strncmp(sHeader, IDE_MSGLOG_HEADER_NUMBER, sLen) != 0);

        mCurNumber = idlOS::strtol(sHeader + sLen, NULL, 10);

        IDE_TEST(mCurNumber >= mMaxNumber);
    }
    else
    {
        /* pass */
    }

    return ID_TRUE;

    IDE_EXCEPTION_END;
    return ID_FALSE;
}

PDL_HANDLE ideMsgLog::getFD()
{
    if ( mEnabled == ID_TRUE )
    {
        if ( checkExist() == ID_FALSE )
        {
            IDE_TEST( createFileAndHeader() != IDE_SUCCESS );
        }
        else
        {
            if ( isSameFile( mFD, mPath ) == ID_TRUE )
            {
                /* do nothing */
            }
            else
            {
                (void)close();
                IDE_TEST( open( mDebug ) != IDE_SUCCESS );

                IDE_TEST( writeWarningMessage() != IDE_SUCCESS );
            }
        }
    }
    else
    {
        /* do nothing */
    }

    return mFD;

    IDE_EXCEPTION_END;

    return PDL_INVALID_HANDLE;
}
