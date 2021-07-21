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

#include <mmm.h>
#include <mmErrorCode.h>
#include <mmi.h>



#define RLIMIT_LOG_BUF_SIZE 1024



#define RLIMIT_LOG_APPEND(aResource) \
do {\
    if (idlOS::getrlimit(aResource, &sRLimit) == 0) {\
        sRLimitLogBufOffset += idlOS::snprintf(sRLimitLogBuf + sRLimitLogBufOffset,\
                                               RLIMIT_LOG_BUF_SIZE - sRLimitLogBufOffset,\
                                               "%-17s : %"ID_INT64_FMT", %"ID_INT64_FMT"\n",\
                                               #aResource,\
                                               sRLimit.rlim_cur,\
                                               sRLimit.rlim_max);\
    }\
} while (0)



static IDE_RC mmmActLogRLimit(mmmPhase        /*aPhase*/,
                              UInt            /*aOptionflag*/,
                              mmmPhaseAction */*aAction*/)
{
    SChar         sRLimitLogBuf[RLIMIT_LOG_BUF_SIZE] = { '\0', };
    UInt          sRLimitLogBufOffset = 0;
    struct rlimit sRLimit;

    sRLimitLogBufOffset += idlOS::snprintf(sRLimitLogBuf, RLIMIT_LOG_BUF_SIZE, MM_MSG_RLIMIT_HEADER);

#if defined(RLIMIT_CPU)
    RLIMIT_LOG_APPEND(RLIMIT_CPU);
#endif
#if defined(RLIMIT_FSIZE)
    RLIMIT_LOG_APPEND(RLIMIT_FSIZE);
#endif
#if defined(RLIMIT_DATA)
    RLIMIT_LOG_APPEND(RLIMIT_DATA);
#endif
#if defined(RLIMIT_STACK)
    RLIMIT_LOG_APPEND(RLIMIT_STACK);
#endif
#if defined(RLIMIT_CORE)
    RLIMIT_LOG_APPEND(RLIMIT_CORE);
#endif
#if defined(RLIMIT_RSS)
    RLIMIT_LOG_APPEND(RLIMIT_RSS);
#endif
#if defined(RLIMIT_NOFILE) /* == RLIMIT_OPEN_MAX */
    RLIMIT_LOG_APPEND(RLIMIT_NOFILE);
#endif
#if defined(RLIMIT_AS)
    RLIMIT_LOG_APPEND(RLIMIT_AS);
#endif
#if defined(RLIMIT_TCACHE)
    RLIMIT_LOG_APPEND(RLIMIT_TCACHE);
#endif
#if defined(RLIMIT_NPROC)
    RLIMIT_LOG_APPEND(RLIMIT_NPROC);
#endif
#if defined(RLIMIT_MEMLOCK)
    RLIMIT_LOG_APPEND(RLIMIT_MEMLOCK);
#endif
#if defined(RLIMIT_LOCKS)
    RLIMIT_LOG_APPEND(RLIMIT_LOCKS);
#endif
#if defined(RLIMIT_SIGPENDING)
    RLIMIT_LOG_APPEND(RLIMIT_SIGPENDING);
#endif
#if defined(RLIMIT_MSGQUEUE)
    RLIMIT_LOG_APPEND(RLIMIT_MSGQUEUE);
#endif
#if defined(RLIMIT_NICE)
    RLIMIT_LOG_APPEND(RLIMIT_NICE);
#endif
#if defined(RLIMIT_RTPRIO)
    RLIMIT_LOG_APPEND(RLIMIT_RTPRIO);
#endif
#if defined(RLIMIT_RTTIME)
    RLIMIT_LOG_APPEND(RLIMIT_RTTIME);
#endif
#if defined(RLIMIT_NLIMITS)
    RLIMIT_LOG_APPEND(RLIMIT_NLIMITS);
#endif
#if defined(RLIMIT_AIO_OPS)
    RLIMIT_LOG_APPEND(RLIMIT_AIO_OPS);
#endif
#if defined(RLIMIT_AIO_MEM)
    RLIMIT_LOG_APPEND(RLIMIT_AIO_MEM);
#endif
#if defined(RLIMIT_RSESTACK)
    RLIMIT_LOG_APPEND(RLIMIT_RSESTACK);
#endif

    ideLog::log(IDE_SERVER_0, sRLimitLogBuf);

    return IDE_SUCCESS;
}

mmmPhaseAction gMmmActLogRLimit =
{
    (SChar *)"Logging Resource Limits",
    0,
    mmmActLogRLimit
};

