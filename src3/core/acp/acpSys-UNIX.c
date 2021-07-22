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
 * $Id: acpSys-UNIX.c 9528 2010-01-14 02:52:05Z denisb $
 ******************************************************************************/

#include <acpMem.h>
#include <acpPrintf.h>
#include <acpSock.h>
#include <acpSys.h>
#include <acpTimePrivate.h>
#include <acpTest.h>
#include <acpCStr.h>

/**
 * gets the limit of file open
 *
 * @param aHandleLimit pointer to a variable to get the limit
 * @return result code
 *
 * it returns #ACP_RC_ENOTSUP if the system does not have such limitation
 */
ACP_EXPORT acp_rc_t acpSysGetHandleLimit(acp_uint32_t *aHandleLimit)
{
    acp_ulong_t sValue = 0;
    acp_rc_t    sRC;

    sRC = acpSysGetResourceLimit(RLIMIT_NOFILE, &sValue);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        *aHandleLimit = (acp_uint32_t)sValue;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * sets the limit of file open
 *
 * @param aHandleLimit limit to set
 * @return result code
 *
 * it returns #ACP_RC_ENOTSUP if the system does not have such limitation
 */
ACP_EXPORT acp_rc_t acpSysSetHandleLimit(acp_uint32_t aHandleLimit)
{
    return acpSysSetResourceLimit(RLIMIT_NOFILE, aHandleLimit);
}

/**
 * gets the number of cpu online
 *
 * @param aCount pointer to a variable to get cpu count
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSysGetCPUCount(acp_uint32_t *aCount)
{
    acp_slong_t sRet;

#if defined(_SC_NPROCESSORS_ONLN)
    sRet = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(ALTI_CFG_OS_HPUX)
    sRet = mpctl(MPC_GETNUMSPUS, 0, 0);
#endif

    if (sRet == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        *aCount = (sRet > 1) ? (acp_uint32_t)sRet : 1;

        return ACP_RC_SUCCESS;
    }
}

/**
 * gets the system page size
 *
 * @param aPageSize pointer to a variable to get system page size
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSysGetPageSize(acp_size_t *aPageSize)
{
    acp_slong_t sRet;

#if defined(_SC_PAGESIZE)
    sRet = sysconf(_SC_PAGESIZE);
#elif defined(_SC_PAGE_SIZE)
    sRet = sysconf(_SC_PAGE_SIZE);
#else
    sRet = getpagesize();
#endif

    if (sRet == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        *aPageSize = (acp_size_t)sRet;

        return ACP_RC_SUCCESS;
    }
}

/**
 * gets the os block size
 *
 * @param aBlockSize pointer to a variable of os block size
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSysGetBlockSize(acp_size_t *aBlockSize)
{
    acp_rc_t sRC;

#if defined(DEV_BSIZE)
    *aBlockSize = (acp_size_t)DEV_BSIZE;
    sRC         = ACP_RC_SUCCESS;
#elif defined(S_BLKSIZE)
    *aBlockSize = (acp_size_t)S_BLKSIZE;
    sRC         = ACP_RC_SUCCESS;
#else
    *aBlockSize = 0;
    sRC         = ACP_RC_ENOENT;
#endif

    return sRC;
}

/**
 * gets the host name
 *
 * @param aBuffer pointer to a buffer to store host name
 * @param aSize size of buffer
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSysGetHostName(acp_char_t *aBuffer, acp_size_t aSize)
{
    acp_sint32_t sRet;

    sRet = gethostname(aBuffer, aSize);

    return (sRet != 0) ? ACP_RC_GET_NET_ERROR() : ACP_RC_SUCCESS;
}

/**
 * gets user name of the current process
 * @param aBuffer pointer to a buffer to store user name
 * @param aSize size of buffer
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSysGetUserName(acp_char_t *aBuffer, acp_size_t aSize)
{
    /*
     * BUGBUG:
     *
     * getpwuid() is not thread safe. the thread safe version is getpwuid_r()
     */
    struct passwd *sPwInfo = getpwuid(getuid());

    if (sPwInfo == NULL)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return acpSnprintf(aBuffer, aSize, "%s", sPwInfo->pw_name);
    }
}

ACP_INLINE acp_time_t acpTimeFromClock(clock_t aClock)
{
    acp_float_t sSec = (acp_float_t)aClock / (acp_float_t)sysconf(_SC_CLK_TCK);

    return (acp_time_t)(sSec * 1000000);
}

/**
 * gets the cpu times of the current process
 *
 * @param aCpuTimes pointer to a structure to get the cpu times
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSysGetCPUTimes(acp_cpu_times_t *aCpuTimes)
{
    struct tms sTmsBuf;
    clock_t    sRet;

    sRet = times(&sTmsBuf);

    if (sRet == (clock_t)-1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    aCpuTimes->mUserTime   = acpTimeFromClock(sTmsBuf.tms_utime);
    aCpuTimes->mSystemTime = acpTimeFromClock(sTmsBuf.tms_stime);

    return ACP_RC_SUCCESS;
}
