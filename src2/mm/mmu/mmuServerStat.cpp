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

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idp.h>
#include <smrDef.h>
#include <mmuServerStat.h>
#include <mmErrorCode.h>
#include <mmuProperty.h>


// Used without initialize()
idBool mmuServerStat::isFileExist()
{
    if ( idlOS::access( mFileName, F_OK ) == 0 )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

// Used without initialize()
//fix BUG-18025
IDE_RC mmuServerStat::createLockFile(idBool* aRetry)
{
    PDL_HANDLE sHandle = PDL_INVALID_HANDLE;
    SChar sBuf[4] = "mMm";
    
    /* BUG-33900 Prevent multiple instances */
    sHandle = idlOS::open(mFileName,
                          O_RDWR | O_CREAT,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    *aRetry = ID_FALSE;
    IDE_TEST( sHandle == PDL_INVALID_HANDLE);
    // fix BUG-17545.
    IDE_TEST_RAISE((idlOS::write(sHandle, sBuf, ID_SIZEOF(sBuf)) != ID_SIZEOF(sBuf)),
                   WRITE_ERROR);
    idlOS::close(sHandle);

    return IDE_SUCCESS;

    IDE_EXCEPTION(WRITE_ERROR)
    {
        idlOS::close(sHandle);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmuServerStat::initialize()
{
    SChar *sHomeDir;
 
    
    sHomeDir = idp::getHomeDir();

    idlOS::memset(mFileName, 0, ID_SIZEOF(mFileName));
    
    /* BUG-33900 Prevent multiple instances */
    /* BUG-45135 */
    idlOS::snprintf(mFileName, ID_SIZEOF(mFileName), "%s%c%s",
                    sHomeDir,
                    IDL_FILE_SEPARATOR,
                    IDP_LOCKFILE);

    return IDE_SUCCESS;
}

/* BUG 18294 */
IDE_RC mmuServerStat::initFileLock()
{
    IDE_TEST_RAISE ( idlOS::flock_init( &mLockFile,
                            O_RDWR,
                            mFileName,
                            0644) != 0, init_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(init_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_FLOCK_INIT, mFileName));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmuServerStat::destFileLock()
{
   IDE_TEST_RAISE(idlOS::flock_destroy(&mLockFile) != 0, destroy_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(destroy_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_FLOCK_DESTROY, mFileName));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmuServerStat::destroy()
{
    return IDE_SUCCESS;
}

IDE_RC mmuServerStat::hold()
{
    IDE_TEST_RAISE(idlOS::flock_wrlock(&mLockFile) != 0, hold_error);
    return IDE_SUCCESS;

    IDE_EXCEPTION(hold_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_FLOCK_WRLOCK));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC mmuServerStat::tryhold()
{
    IDE_TEST_RAISE(idlOS::flock_trywrlock(&mLockFile) != 0, hold_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(hold_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_FLOCK_TRYWRLOCK));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmuServerStat::release()
{
    IDE_TEST_RAISE(idlOS::flock_unlock(&mLockFile) != 0, release_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(release_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_FLOCK_UNLOCK));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmuServerStat::checkServerRunning(idBool *aRunningFlag)
{
    struct sockaddr_in servaddr;
    SInt   sState = 0;
    PDL_SOCKET  sFd = PDL_INVALID_SOCKET;

    /* ---------------------------
     *  [1] Server Running 검사 : FileLock
     * --------------------------*/

    if (idlOS::flock_trywrlock(&mLockFile) == 0)
    {
        /* --------------------------------------------------------
         *  [2] Server Running 검사 : Port Bind
         *
         *      [1] 과정에서 Lock이 잡혔더라도
         *          LockFile을 삭제했기 때문에 얻어진 틀린
         *          값일 수 있다. 따라서, 해당 포트를 Bind해서
         *          정말로 실행중이 아닌지 다시 재검사 한다.
         * ------------------------------------------------------*/

        idlOS::memset(&servaddr, 0, sizeof(servaddr));

        sFd = idlOS::socket(AF_INET, SOCK_STREAM, 0);
        IDE_TEST_RAISE(sFd < 0, socket_error);
        sState = 1;

        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port        = htons(mmuProperty::getPortNo());

        IDE_TEST_RAISE(idlVA::setSockReuseraddr(sFd) != 0, reuse_error);
        if ( idlOS::bind(sFd,
                         (struct sockaddr *)(&servaddr),
                         sizeof(servaddr)) < 0)
        {
            *aRunningFlag = ID_TRUE;
        }
        else
        {
            *aRunningFlag = ID_FALSE;
        }
        sState = 0;
        (void)idlOS::closesocket(sFd);
    }
    else
    {
        IDE_TEST_RAISE(errno != EBUSY, flock_error);
        *aRunningFlag = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(reuse_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idc_SVC_INET_BIND_ERROR,
                                (UInt)mmuProperty::getPortNo()));
    }
    IDE_EXCEPTION(flock_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_FLOCK_TRYWRLOCK));
    }
    IDE_EXCEPTION(socket_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idc_INET_SOCKET_CREATE_FAILED));
    }
    IDE_EXCEPTION_END;
    if (sState == 1)
    {
        (void)idlOS::closesocket(sFd);
    }
    return IDE_FAILURE;
}

