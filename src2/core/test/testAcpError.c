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
 * $Id: testAcpError.c 5910 2009-06-08 08:20:20Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <acpError.h>


/*
 * this message array should be same as
 * error message array defined in acpError.c
 */
static const acp_char_t *gAcpErrorMsg[ACP_RC_MAX - ACP_RR_ACP - 1] =
{
    "invalid argument",                                      /*  1 */
    "operation interrupted",                                 /*  2 */
    "no such process",                                       /*  3 */
    "name is too long",                                      /*  4 */
    "already exists",                                        /*  5 */
    "not exist",                                             /*  6 */
    "not empty",                                             /*  7 */
    "range overflow",                                        /*  8 */
    "resource busy",                                         /*  9 */
    "deadlock condition",                                    /* 10 */
    "permission denied",                                     /* 11 */
    "access denied",                                         /* 12 */
    "resource temporarily unavailable",                      /* 13 */
    "operation now in progress",                             /* 14 */
    "operation timed out",                                   /* 15 */
    "bad file descriptor",                                   /* 16 */
    "not a directory",                                       /* 17 */
    "not a socket",                                          /* 18 */
    "not enough space",                                      /* 19 */
    "not enough memory",                                     /* 20 */
    "too many open files",                                   /* 21 */
    "file table overflow",                                   /* 22 */
    "illegal seek",                                          /* 23 */
    "broken pipe",                                           /* 24 */
    "connection refused",                                    /* 25 */
    "connection aborted",                                    /* 26 */
    "connection reset by peer",                              /* 27 */
    "host is unreachable",                                   /* 28 */
    "network is unreachable",                                /* 29 */
    "cross-device link",                                     /* 30 */
    "operation not supported",                               /* 31 */
    "operation not implemented",                             /* 32 */
    "end of file",                                           /* 33 */
    "dynamic linker error",                                  /* 34 */
    "result truncated",                                      /* 35 */
    "bad address",                                           /* 36 */
    "bad message file",                                      /* 37 */
    "operation canceled",                                    /* 38 */
    "no child process",                                      /* 39 */
};

/*
 * this error codes should be ordered same as defined in acpError.h
 */
static const acp_rc_t gAcpErrorCode[ACP_RC_MAX - ACP_RR_ACP - 1] =
{
    ACP_RC_EINVAL,
    ACP_RC_EINTR,
    ACP_RC_ESRCH,
    ACP_RC_ENAMETOOLONG,
    ACP_RC_EEXIST,
    ACP_RC_ENOENT,
    ACP_RC_ENOTEMPTY,
    ACP_RC_ERANGE,
    ACP_RC_EBUSY,
    ACP_RC_EDEADLK,
    ACP_RC_EPERM,
    ACP_RC_EACCES,
    ACP_RC_EAGAIN,
    ACP_RC_EINPROGRESS,
    ACP_RC_ETIMEDOUT,
    ACP_RC_EBADF,
    ACP_RC_ENOTDIR,
    ACP_RC_ENOTSOCK,
    ACP_RC_ENOSPC,
    ACP_RC_ENOMEM,
    ACP_RC_EMFILE,
    ACP_RC_ENFILE,
    ACP_RC_ESPIPE,
    ACP_RC_EPIPE,
    ACP_RC_ECONNREFUSED,
    ACP_RC_ECONNABORTED,
    ACP_RC_ECONNRESET,
    ACP_RC_EHOSTUNREACH,
    ACP_RC_ENETUNREACH,
    ACP_RC_EXDEV,
    ACP_RC_ENOTSUP,
    ACP_RC_ENOIMPL,
    ACP_RC_EOF,
    ACP_RC_EDLERR,
    ACP_RC_ETRUNC,
    ACP_RC_EFAULT,
    ACP_RC_EBADMDL,
    ACP_RC_ECANCELED,
    ACP_RC_ECHILD
};


acp_sint32_t main(void)
{
    acp_char_t   sMsg[100];
    acp_rc_t     sRC;
    acp_sint32_t i;

    ACT_TEST_BEGIN();

    for (i = 0; i < ACP_RC_MAX - ACP_RR_ACP - 1; i++)
    {
        sRC = gAcpErrorCode[i];

        if (sRC >= ACP_RR_ACP)
        {
            acpErrorString(sRC, sMsg, sizeof(sMsg));

            ACT_CHECK_DESC(acpCStrCmp(gAcpErrorMsg[i],
                                       sMsg,
                                       sizeof(sMsg)) == 0,
                           ("Mismatch default error message for error code %d",
                            sRC));
        }
        else
        {
            /*
             * ignore because the sRC is remapped to system error code
             */
        }
    }

    ACT_TEST_END();

    return 0;
}
