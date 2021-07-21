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
 * $Id: acpError.c 1161 2007-11-14 08:35:55Z jykim $
 ******************************************************************************/

/**
 * @example sampleAcpError.c
 */

#include <acpError.h>
#include <acpPrintf.h>


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


#if defined(ALTI_CFG_OS_WINDOWS)

static const struct
{
    acp_rc_t          mRC;
    const acp_char_t *mMsg;
} gAcpErrorWin[] =
{
    WSAEINTR,           "Interrupted system call",
    WSAEBADF,           "Bad file number",
    WSAEACCES,          "Permission denied",
    WSAEFAULT,          "Bad address",
    WSAEINVAL,          "Invalid argument",
    WSAEMFILE,          "Too many open sockets",
    WSAEWOULDBLOCK,     "Operation would block",
    WSAEINPROGRESS,     "Operation now in progress",
    WSAEALREADY,        "Operation already in progress",
    WSAENOTSOCK,        "Socket operation on non-socket",
    WSAEDESTADDRREQ,    "Destination address required",
    WSAEMSGSIZE,        "Message too long",
    WSAEPROTOTYPE,      "Protocol wrong type for socket",
    WSAENOPROTOOPT,     "Bad protocol option",
    WSAEPROTONOSUPPORT, "Protocol not supported",
    WSAESOCKTNOSUPPORT, "Socket type not supported",
    WSAEOPNOTSUPP,      "Operation not supported on socket",
    WSAEPFNOSUPPORT,    "Protocol family not supported",
    WSAEAFNOSUPPORT,    "Address family not supported",
    WSAEADDRINUSE,      "Address already in use",
    WSAEADDRNOTAVAIL,   "Can't assign requested address",
    WSAENETDOWN,        "Network is down",
    WSAENETUNREACH,     "Network is unreachable",
    WSAENETRESET,       "Net connection reset",
    WSAECONNABORTED,    "Software caused connection abort",
    WSAECONNRESET,      "Connection reset by peer",
    WSAENOBUFS,         "No buffer space available",
    WSAEISCONN,         "Socket is already connected",
    WSAENOTCONN,        "Socket is not connected",
    WSAESHUTDOWN,       "Can't send after socket shutdown",
    WSAETOOMANYREFS,    "Too many references, can't splice",
    WSAETIMEDOUT,       "Connection timed out",
    WSAECONNREFUSED,    "Connection refused",
    WSAELOOP,           "Too many levels of symbolic links",
    WSAENAMETOOLONG,    "File name too long",
    WSAEHOSTDOWN,       "Host is down",
    WSAEHOSTUNREACH,    "No route to host",
    WSAENOTEMPTY,       "Directory not empty",
    WSAEPROCLIM,        "Too many processes",
    WSAEUSERS,          "Too many users",
    WSAEDQUOT,          "Disc quota exceeded",
    WSAESTALE,          "Stale NFS file handle",
    WSAEREMOTE,         "Too many levels of remote in path",
    WSASYSNOTREADY,     "Network system is unavailable",
    WSAVERNOTSUPPORTED, "Winsock version out of range",
    WSANOTINITIALISED,  "WSAStartup not yet called",
    WSAEDISCON,         "Graceful shutdown in progress",
    WSAHOST_NOT_FOUND,  "Host not found",
    WSANO_DATA,         "No host data of that type was found",
    0,                  NULL
};

static void acpErrorStringWin(acp_rc_t aRC, char *aBuffer, acp_size_t aSize)
{
    acp_size_t sLen;
    acp_size_t i;

    sLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
                         | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL,
                         aRC,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPTSTR)aBuffer,
                         (DWORD)aSize,
                         NULL);

    if (sLen == 0)
    {
        for (i = 0; gAcpErrorWin[i].mMsg != NULL; i++)
        {
            if (gAcpErrorWin[i].mRC == aRC)
            {
                acpSnprintf(aBuffer, aSize, "%s", gAcpErrorWin[i].mMsg);
                break;
            }
        }

        if (gAcpErrorWin[i].mMsg == NULL)
        {
            acpSnprintf(aBuffer, aSize, "Unknown error %d", aRC + ACP_RR_WIN);
        }
    }
    else
    {
        /*
         * FormatMessage put the message in the buffer, but it may
         * have embedded a newline (\r\n), and possible more than one.
         * Remove the newlines replacing them with a space. This is not
         * as visually perfect as moving all the remaining message over,
         * but more efficient.
         */
        for (i = 0; i < sLen; i++)
        {
            if ((aBuffer[i] == '\r') || (aBuffer[i] == '\n'))
            {
                aBuffer[i] = ' ';
            }
        }
    }
}

#endif


/**
 * get the human readable string describing the result code
 *
 * @param aRC the result code
 * @param aBuffer the buffer to store error description string
 * @param aSize size of the buffer
 */
ACP_EXPORT void acpErrorString(acp_rc_t    aRC,
                               acp_char_t *aBuffer,
                               acp_size_t  aSize)
{
    /*
     * SUCCESS
     */
    if (aRC == ACP_RC_SUCCESS)
    {
        (void)acpSnprintf(aBuffer, aSize, "Success");
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * error range ACP
     */
    if ((aRC > ACP_RR_ACP) && (aRC < ACP_RC_MAX))
    {
        (void)acpSnprintf(aBuffer,
                          aSize,
                          "%s",
                          gAcpErrorMsg[aRC - ACP_RR_ACP - 1]);
        return;
    }
    else
    {
        /* do nothing */
    }

#if defined(ALTI_CFG_OS_WINDOWS)
    /*
     * error range WIN
     */
    if ((aRC >= ACP_RR_WIN) && (aRC < ACP_RR_ACP))
    {
        acpErrorStringWin(aRC - ACP_RR_WIN, aBuffer, aSize);
        return;
    }
    else
    {
        /* do nothing */
    }
#endif

    /*
     * error range STD
     */
    if (aRC < ACP_RR_WIN)
    {
#if defined(ALTI_CFG_OS_HPUX)    ||              \
    defined(ALTI_CFG_OS_SOLARIS) ||              \
    defined(ALTI_CFG_OS_WINDOWS)
        char *sMsg = strerror(aRC);

        if (sMsg != NULL)
        {
            (void)acpSnprintf(aBuffer, aSize, "%s", sMsg);
        }
        else
        {
            (void)acpSnprintf(aBuffer, aSize, "Unknown error %d", aRC);
        }
#elif defined(ALTI_CFG_OS_LINUX)
        char *sMsg = (void *)strerror_r(aRC, aBuffer, aSize);

        if ((sMsg != NULL) && sMsg != (void *)-1)
        {
            (void)acpSnprintf(aBuffer, aSize, "%s", sMsg);
        }
        else
        {
            /* do nothing */
        }
#else
        acp_sint32_t sRet;

        sRet = strerror_r(aRC, aBuffer, aSize);

        if (sRet == EINVAL)
        {
            (void)acpSnprintf(aBuffer, aSize, "Unknown error %d", aRC);
        }
        else
        {
            /* do nothing */
        }
#endif
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * unknown error
     */
    (void)acpSnprintf(aBuffer, aSize, "Unknown error %d", aRC);
}
