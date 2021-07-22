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
 * $Id: acpProc-WINDOWS.c 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#include <acpOS.h>
#include <acpProc.h>
#include <acpPrintf.h>
#include <acpSpinLock.h>
#include <acpSleep.h>
#include <acpEnv.h>
#include <acpPath.h>

#define ACP_PROC_WIN_SYSTEMROOT     "SystemRoot"
#define ACP_PROC_WIN_CMD_VAR        "ComSpec"
#define ACP_PROC_WIN_DEFAULT_CMD    "cmd.exe"
#define ACP_PROC_WIN_SYSTEM         "%s /C %s"
#define ACP_PROC_WIN_CMDNOTFOUND    9
#define ACP_PROC_NOTFOUNDCODE       127

/* Internal Data structure */
typedef HANDLE acpProcHandle;

#define ACPPROC_CHUNKSIZE MAXIMUM_WAIT_OBJECTS
typedef acp_proc_t acpChunkElementT;
typedef struct acpChunks acpChunks;

struct acpChunks
{
    acpChunkElementT      mElements[ACPPROC_CHUNKSIZE];
    volatile acp_sint32_t   mCount;

    acpChunks* mPrev;
    acpChunks* mNext;
};

static acpChunks gAcpChunkListHead =
{
    {0, },
    0, /* Initially no element */
    &gAcpChunkListHead, /* Circular doubly linked list */
    &gAcpChunkListHead
};
static acpChunks* gAcpSentinel = &gAcpChunkListHead;

/* Locking */
static acp_spin_lock_t gAcpProcLock = ACP_SPIN_LOCK_INITIALIZER(-1);
static acp_bool_t gIsCldNoWait = ACP_FALSE;

/*
 * Internal function to insert a process ID into list
 */
static acp_rc_t acpChunkInsertElement(const acpChunkElementT aElement)
{
    acpChunks* sIter = gAcpSentinel;

    while(ACPPROC_CHUNKSIZE == sIter->mCount)
    {
        sIter = sIter->mNext;
        if(gAcpSentinel == sIter)
        {
            /* All nodes are full
             * Allocate new */
            acpChunks* sNew = NULL;
            acp_rc_t sRC = acpMemAlloc(&sNew, sizeof(acpChunks));

            if(ACP_RC_IS_SUCCESS(sRC))
            {
                sNew->mCount = 0;

                /* Link nodes */
                sNew->mPrev = sIter->mPrev;
                sNew->mNext = sIter;
                sIter->mPrev = sNew;
                sNew->mPrev->mNext = sNew;

                sIter = sNew;
            }
            else
            {
                return sRC;
            }
        }
    }

    sIter->mElements[sIter->mCount++] = aElement;
    return ACP_RC_SUCCESS;
}

/* Delete a Node */
static void acpChunkDeleteNode(acpChunks* aNode)
{
    if(gAcpSentinel != aNode)
    {
        /* Re-link */
        aNode->mPrev->mNext = aNode->mNext;
        aNode->mNext->mPrev = aNode->mPrev;

        /* Delete node */
        acpMemFree(aNode);
    }
    else
    {
        /* Cannot remove sentinel */
    }
}

/*
 * Internal function to delete a process ID from list
 */
static acp_rc_t acpChunkDeleteElement(const acpChunkElementT aElement)
{
    acpChunks* sIter = gAcpSentinel;
    acp_bool_t sContinue = ACP_TRUE;
    acp_sint32_t i;


    do {
        /* Delete if identical element found */
        for(i=0; i<sIter->mCount; i++)
        {
            if(aElement == sIter->mElements[i])
            {
                sContinue = ACP_FALSE;
                sIter->mCount--;
                for(; i<sIter->mCount; i++)
                {
                    sIter->mElements[i] = sIter->mElements[i + 1];
                }

                break;
            }
        }

        /* Delete if node empty */
        if(0 == sIter->mCount && gAcpSentinel != sIter)
        {
            acpChunks* sToDel = sIter;
            sIter = sIter->mNext;
            acpChunkDeleteNode(sToDel);
        }
        else
        {
            sIter = sIter->mNext;
        }
    } while(sIter != gAcpSentinel && sContinue);

    return ACP_RC_SUCCESS;
}

/*
 * buildin function to get process termination status infomation
 */
ACP_INLINE acp_proc_wait_t acpProcGetWaitResult(HANDLE aHandle)
{
    acp_proc_wait_t sWaitResult;

    DWORD sStatus;
    (void)GetExitCodeProcess(aHandle, &sStatus);
    sWaitResult.mPID = GetProcessId(aHandle);

    /***************************************************
      =================================================
      |    UNIX     |          WINDOWS                |
      =================================================
          SIGSEGV      STATUS_ACCESS_VIOLATION
          SIGSEGV      STATUS_STACK_OVERFLOW
          SIGSEGV      STATUS_STACK_BUFFER_OVERRUN
          SIGBUS       STATUS_IN_PAGE_ERROR
          SIGILL       STATUS_ILLEGAL_INSTRUCTION
          SIGILL       STATUS_PRIVILEGED_INSTRUCTION
          SIGFPE       STATUS_FLOAT_DENORMAL_OPERAND
          SIGFPE       STATUS_FLOAT_DIVIDE_BY_ZERO
          SIGFPE       STATUS_FLOAT_INEXACT_RESULT
          SIGFPE       STATUS_FLOAT_INVALID_OPERATION
          SIGFPE       STATUS_FLOAT_OVERFLOW
          SIGFPE       STATUS_FLOAT_UNDERFLOW
          SIGFPE       STATUS_INTEGER_DIVIDE_BY_ZERO
          SIGFPE       STATUS_INTEGER_OVERFLOW
          SIGSTKFL     STATUS_FLOAT_STACK_CHECK
          SIGSYS       STATUS_INVALID_PARAMETER
      ================================================
    ***************************************************/

    switch (sStatus)
    {
        /* SIGFPE */
        case STATUS_FLOAT_DENORMAL_OPERAND:
        case STATUS_FLOAT_DIVIDE_BY_ZERO:
        case STATUS_FLOAT_INEXACT_RESULT:
        case STATUS_FLOAT_INVALID_OPERATION:
        case STATUS_FLOAT_OVERFLOW:
        case STATUS_FLOAT_UNDERFLOW:
        case STATUS_INTEGER_DIVIDE_BY_ZERO:
        case STATUS_INTEGER_OVERFLOW:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGFPE;
            break;

        /* SIGTKFL */
        case STATUS_FLOAT_STACK_CHECK:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGSTKFLT;
            break;

        /* SIGBUS */
        case STATUS_IN_PAGE_ERROR:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGBUS;
            break;

        /* SIGSYS */
        case STATUS_INVALID_PARAMETER:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGSYS;
            break;

        /* SIGILL */
        case STATUS_ILLEGAL_INSTRUCTION:
        case STATUS_PRIVILEGED_INSTRUCTION:
        case STATUS_NONCONTINUABLE_EXCEPTION:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGILL;
            break;

        /* SIGALRM */
        case STATUS_TIMEOUT:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGALRM;
            break;

        /* SIGINT */
        case STATUS_CONTROL_C_EXIT:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGINT;
            break;

        /* SIGSEGV */
        case STATUS_ACCESS_VIOLATION:
        case STATUS_DATATYPE_MISALIGNMENT:
        case STATUS_ARRAY_BOUNDS_EXCEEDED:
        case STATUS_NO_MEMORY:
        case STATUS_INVALID_DISPOSITION:
        case STATUS_STACK_OVERFLOW:
        case STATUS_STACK_BUFFER_OVERRUN:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGSEGV;
            break;

        /* SIGABRT */
        case 3: /* Aborted with raise(SIGABRT); */
        case STATUS_ABORT_EXIT:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGABRT;
            break;

        /* SIGKILL */
        case 1: /* Killed by Task Manager */
        case STATUS_PROCESS_KILLED:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = SIGKILL;
            break;

        /* for exit code */
        default:
            sWaitResult.mState    = ACP_PROC_EXITED;
            sWaitResult.mExitCode = sStatus;
            break;
    }

    return sWaitResult;
}

/*
 * Internal function to get Handle from Process ID
 */
static acp_rc_t acpProcGetHandle(acp_proc_t aProc, acpProcHandle* aHandle)
{
    *aHandle = OpenProcess(PROCESS_ALL_ACCESS | SYNCHRONIZE,
                           ACP_FALSE, aProc);

    return (NULL == (*aHandle))? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

/*
 * Internal function to wait for a series of handles
 */
static acp_rc_t acpProcWaitForHandles(acpProcHandle* aHandles,
                                      acp_sint32_t aSize,
                                      acp_sint32_t* aIndex)
{
    DWORD sWaitResult = WaitForMultipleObjects(aSize, aHandles, ACP_FALSE, 0);
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if(WAIT_FAILED == sWaitResult)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        if(WAIT_TIMEOUT == sWaitResult)
        {
            sRC = ACP_RC_EAGAIN;
        }
        else
        {
            if(WAIT_ABANDONED <= sWaitResult)
            {
                *aIndex = sWaitResult - WAIT_ABANDONED;
            }
            else
            {
                if(WAIT_OBJECT_0 <= sWaitResult)
                {
                    *aIndex = sWaitResult - WAIT_OBJECT_0;
                }
            }
        }
    }

    return sRC;
}

/*
 * Internal function to get Handle from Process ID
 */
static acp_rc_t acpProcWaitForListNode(acpChunks* aNode,
                                       acp_proc_wait_t* aWR)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    acp_sint32_t j = aNode->mCount;
    acp_sint32_t i;
    acp_sint32_t k;

    if(aNode == gAcpSentinel && 0 == aNode->mCount)
    {
        aNode = aNode->mNext;
    }
    else
    {
        /* Do nothing. Wait for first list node element */
    }

    if(0 == j)
    {
        /* Do nothing */
    }
    else
    {
        acp_sint32_t sProcIndex = -1;

        acpProcHandle sHandles[ACPPROC_CHUNKSIZE];
        for(i = 0; i < j; i++)
        {
            if(ACP_RC_IS_SUCCESS(
                    sRC = acpProcGetHandle(aNode->mElements[i], &sHandles[i]))
              )
            {
                continue; /* ^^; */
            }
            else
            {
                k = i;
                break;
            }
        }

        if(ACP_RC_IS_SUCCESS(sRC))
        {
            /* Successfully got all handles */
            sRC = acpProcWaitForHandles(sHandles,
                                        j,
                                        &sProcIndex);
            k = j;
        }
        else
        {
            /* Have nothing to do
             * Handles are mess! */
        }

        if(-1 != sProcIndex)
        {
            /* Gotcha! */
            if(NULL != aWR)
            {
                *aWR = acpProcGetWaitResult(sHandles[sProcIndex]);
            }
            else
            {
                /* Wait result is ignored */
            }

            aNode->mCount--;

            /* Remove exited Process ID */
            for(i = sProcIndex; i < j - 1; i++)
            {
                aNode->mElements[i] = aNode->mElements[i + 1];
            }

        }
        else
        {
            /* No child exited */
        }

        /* Free Handles */
        for(i=0; i < k; i++)
        {
            (void)CloseHandle(sHandles[i]);
        }
    }

    return sRC;
}

/*
 * builtin function to convert args to acp_str_t
 *
 * aReplacePathSep determines whether to replace UNIX-style path
 * separators(/) in aArgs into Windows-style path separators (\).
 * This has a side effect where option strings such as /R will have
 * their slashes converted to backslashes as well (e.g. /R would
 * become \R), which may not be desirable. (See BUG-35105)
 *
 * UNIX-style path separators in aFile are always translated into
 * Windows-style.
 */
static acp_rc_t acpConvertArgsToStr(acp_str_t*        aStr,
                                    acp_char_t*       aFile,
                                    acp_char_t* const aArgs[],
                                    acp_bool_t        aReplacePathSep)
{
    acp_rc_t     sRC;
    acp_sint32_t i;

    sRC = acpStrCpyFormat(aStr, "\"%s\"", aFile);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    if (aReplacePathSep)
    {
        /* replace path seperators in aFile later together with the
         * arguments */
    }
    else
    {
        (void)acpStrReplaceChar(aStr, '/', '\\', 0, -1);
    }

    for (i = 0; aArgs[i] != 0; i++)
    {
        sRC = acpStrCatFormat(aStr, " \"%s\"", aArgs[i]);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* do nothing */
        }
    }

    if (aReplacePathSep)
    {
        (void)acpStrReplaceChar(aStr, '/', '\\', 0, -1);
    }
    else
    {
        /* nothing to do as we are not replacing path separators in
         * the arguments */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpProcLaunchDaemon(
    acp_proc_t             *aProc,
    acp_char_t             *aFile,
    acp_char_t* const       aArgs[],
    acp_bool_t              aSearchInPath)
{
    acp_bool_t              sRet;
    acp_rc_t                sRC;
    acp_path_pool_t         sPathPool;
    acp_char_t             *sFilePath = NULL;

    STARTUPINFO             sStartupInfo;
    PROCESS_INFORMATION     sProcessInfo;

    ACP_STR_DECLARE_DYNAMIC(sPath);

    acpPathPoolInit(&sPathPool);

    /* acpFileStateAtPath needs full path.
       If the path includes no extension, set "exe". */
    if (acpCStrCmp(acpPathExt(aFile, &sPathPool), "", 1) == 0)
    {
        sFilePath = acpPathMakeExe(aFile, &sPathPool);
    }
    else
    {
        sFilePath = acpPathFull(aFile, &sPathPool);
    }

    if(ACP_FALSE == aSearchInPath)
    {
        acp_stat_t sStat;

        /*
         * Whan not aSearchInPath, sFilePath must exist!
         */
        sRC = acpFileStatAtPath(sFilePath, &sStat, ACP_FALSE);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FILE_NOT_FOUND);
    }
    else
    {
        sFilePath = acpPathMakeExe(aFile, &sPathPool);
    }

    /*
     * BUG - 30120 acpProcLaunchDaemon fails to execute daemon with
     * a large parameters set.
     * Check sFilePath.
     * Windows couldn't work with path longer then MAX_PATH.
     */
    ACP_TEST_RAISE(acpCStrLen(sFilePath,
                              ACP_PATH_MAX_LENGTH) > MAX_PATH,
                   PATH_TOO_LONG);

    /*
     * BUG - 30120 acpProcLaunchDaemon fails to execute daemon with
     * a large parameters set.
     * sPath is a path with options, so it could be longer then MAX_PATH.
     */
    ACP_STR_INIT_DYNAMIC(sPath, ACP_PATH_MAX_LENGTH, 0);

    /* fills memory block with zero */
    ZeroMemory(&sStartupInfo, sizeof(STARTUPINFO));

    /* fills the default information  */
    sStartupInfo.cb         = sizeof(STARTUPINFO);
    sStartupInfo.dwFlags    = STARTF_USESTDHANDLES;
    sStartupInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    sStartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    sStartupInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    /* convert args array to path string */
    sRC = acpConvertArgsToStr(&sPath, sFilePath, aArgs, ACP_TRUE);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PATH_CONVERT_FAILED);

    /* One or more arguments */
    sRet = CreateProcess(NULL,
                         acpStrGetBuffer(&sPath),
                         NULL,       /* No process attributes */
                         NULL,       /* No thread attributes */
                         TRUE,       /* Allow handle inheritance */
                         0,          /* no need to use console */
                         NULL,       /* Environment variables */
                         NULL,       /* No current directory */
                         &sStartupInfo,
                         &sProcessInfo);

    ACP_STR_FINAL(sPath);

    ACP_TEST_RAISE(0 == sRet, EXEC_FAILED);

    *aProc = sProcessInfo.dwProcessId;

    /* we should close hThread handle opened in kernel */
    (void)CloseHandle(sProcessInfo.hThread);

    /* parent do not wait the new created child,
     * so hProcess handle is not necessary */
    (void)CloseHandle(sProcessInfo.hProcess);

    acpPathPoolFinal(&sPathPool);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(FILE_NOT_FOUND)
    {
        /* We have sRC as error code */
    }

    ACP_EXCEPTION(PATH_TOO_LONG)
    {
        /* Path to executable is to long */
        sRC = ACP_RC_ENAMETOOLONG;
    }

    ACP_EXCEPTION(PATH_CONVERT_FAILED)
    {
        ACP_STR_FINAL(sPath);
        /* We have sRC as error code */
    }

    ACP_EXCEPTION(EXEC_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    acpPathPoolFinal(&sPathPool);

    ACP_EXCEPTION_END;
    return sRC;
}

ACP_EXPORT acp_rc_t acpProcForkExecEnvPipeOpt(
    acp_proc_t*                   aProc,
    acp_char_t*                   aFile,
    acp_char_t* const             aArgs[],
    acp_char_t* const             aEnvs[],
    const acp_bool_t              aDuplicateEnv,
    const acp_bool_t              aSearchInPath,
    const acp_proc_std_handles_t* aStdHandles,
    acp_uint32_t                  aOption)
{
    acp_bool_t              sRet;
    acp_rc_t                sRC;
    acp_char_t*             sEnv = NULL;
    acp_char_t**            sEnvs = NULL;
    acp_size_t              sEnvSize = 0;
    acp_size_t              sCurEnvLength;
    acp_size_t              sEnvLength = 0;
    acp_uint32_t            i;
    acp_path_pool_t         sPathPool;
    acp_char_t*             sFilePath = NULL;

    STARTUPINFO             sStartupInfo;
    PROCESS_INFORMATION     sProcessInfo;

    ACP_STR_DECLARE_DYNAMIC(sPath);

    acpPathPoolInit(&sPathPool);

    /* acpFileStateAtPath needs full path.
       If the path incldes no extension, set "exe". */
    if (acpCStrCmp(acpPathExt(aFile, &sPathPool), "", 1) == 0)
    {
        sFilePath = acpPathMakeExe(aFile, &sPathPool);
    }
    else
    {
        sFilePath = acpPathFull(aFile, &sPathPool);
    }

    if(ACP_FALSE == aSearchInPath)
    {
        acp_stat_t sStat;

        /*
         * Whan not aSearchInPath, file name must exist!
         */
        sRC = acpFileStatAtPath(sFilePath, &sStat, ACP_FALSE);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FILE_NOT_FOUND);
    }
    else
    {
        /* Fall through */
    }

    ACP_STR_INIT_DYNAMIC(sPath, MAX_PATH, 0);

    /* fills memory block with zero */
    ZeroMemory(&sStartupInfo, sizeof(STARTUPINFO));

    /* fills the default information  */
    sStartupInfo.cb         = sizeof(STARTUPINFO);
    sStartupInfo.dwFlags    = STARTF_USESTDHANDLES;
    /* check if we need to redefine standard handles */
    if (aStdHandles != NULL)
    {
        if (aStdHandles->mStdIn != NULL )
        {
            sStartupInfo.hStdInput  = aStdHandles->mStdIn->mHandle;
        }
        else
        {
            sStartupInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
        }

        if (aStdHandles->mStdOut != NULL )
        {
            sStartupInfo.hStdOutput  = aStdHandles->mStdOut->mHandle;
        }
        else
        {
            sStartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        }

        if (aStdHandles->mStdErr != NULL )
        {
            sStartupInfo.hStdError  = aStdHandles->mStdErr->mHandle;
        }
        else
        {
            sStartupInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
        }
    }
    else
    {
        sStartupInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
        sStartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        sStartupInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    }

    /* convert args array to path string */
    sRC = acpConvertArgsToStr(&sPath, sFilePath, aArgs,
                              !(aOption & ACP_PROC_FORKEXEC_PRESERVE_PATHSEP));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PATH_CONVERT_FAILED);

    /* Prepare for environment variable */
    sEnv = NULL;
    sEnvs = NULL;

    if(NULL == aEnvs)
    {
        /* 
         * No need to pass environment variables
         * The child process shall inherit current env. var.s
         */
    }
    else
    {
        /* Current environment variables */
        if(ACP_TRUE == aDuplicateEnv)
        {
            acp_size_t          sEnvBefore;
            acp_size_t          sEnvAfter = (acp_size_t)0;
            acp_size_t          sEnvAdditional = (acp_size_t)0;

            while(NULL != aEnvs[sEnvAdditional])
            {
                sEnvAdditional++;
            }

            (void)acpEnvList(NULL, 0, &sEnvAfter);

            /*
             * To be sure that another thread did not touch environment variable
             */
            do
            {
                sEnvBefore = sEnvAfter;
                sRC = acpMemRealloc(
                    (void**)(&sEnvs),
                    sizeof(acp_char_t*) * (sEnvAdditional + sEnvBefore + 1)
                    );
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENV_ALLOC_FAILED);
                (void)acpEnvList(sEnvs + sEnvAdditional,
                                 sEnvBefore,
                                 &sEnvAfter);
            } while(sEnvBefore != sEnvAfter);

            for(i = 0; NULL != aEnvs[i]; i++)
            {
                sEnvs[i] = aEnvs[i];
            }
            sEnvs[sEnvAfter + sEnvAdditional] = NULL;
        }
        else
        {
            /*
             * A Workaround :
             * On some system, %SystemRoot% must be passed to child process
             */
            acp_size_t  sEnvAdditional = (acp_size_t)0;
            acp_size_t  sLenRoot = (acp_size_t)0;
            acp_char_t* sSystemRoot = NULL;

            while(NULL != aEnvs[sEnvAdditional])
            {
                sEnvAdditional++;
            }

            sRC = acpMemAlloc(
                (void**)(&sEnvs),
                sizeof(acp_char_t*) * (sEnvAdditional + 2)
                );
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENV_ALLOC_FAILED);

            sRC = acpEnvGet(ACP_PROC_WIN_SYSTEMROOT, &sSystemRoot);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENV_ALLOC_FAILED);

            sLenRoot  = acpCStrLen(sSystemRoot, ACP_PATH_MAX_LENGTH);
            sLenRoot += acpCStrLen(ACP_PROC_WIN_SYSTEMROOT "=",
                                   ACP_PATH_MAX_LENGTH) + 1;

            sRC = acpMemAlloc((void**)(&sEnvs[0]),
                              sizeof(acp_char_t) * sLenRoot
                              );
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ROOT_ALLOC_FAILED);
            (void)acpSnprintf(sEnvs[0], sLenRoot,
                              ACP_PROC_WIN_SYSTEMROOT "=%s",
                              sSystemRoot);
            for(i = 0; NULL != aEnvs[i]; i++)
            {
                sEnvs[i + 1] = aEnvs[i];
            }
            sEnvs[sEnvAdditional + 1] = NULL;
        }

        /*
         * Merge sEnvs into one long string
         * Environment variables to pass to CreateProcess must be in the form of
         * VAR1=VALUE1\0VAR2=VALUE2\0...VARn=VALUEn\0\0
         *   <- ends with Two terminators
         */
        for(i = 0; NULL != sEnvs[i]; i++)
        {
            for(sCurEnvLength = 1; ACP_CSTR_TERM != sEnvs[i][sCurEnvLength];)
            {
                /* Length-counting loop */
                sCurEnvLength *= 2;
                sCurEnvLength = acpCStrLen(sEnvs[i], sCurEnvLength);
            }
            /* Including NUL terminator */
            sCurEnvLength++;

            /* Enlarge sEnv so that it can contain sEnvs[i] */
            if(sCurEnvLength + sEnvLength > sEnvSize)
            {
                while(sCurEnvLength + sEnvLength > sEnvSize)
                {
                    sEnvSize = (0 == sEnvSize)? 1024 : (sEnvSize * 2);
                }

                sRC = acpMemRealloc((void**)(&sEnv),
                                    sizeof(acp_char_t) * sEnvSize);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENV_CONVERT_FAILED);
            }
            else
            {
                /* No need of enlarge */
            }

            /* Merge sEnvs[i] into sEnv */
            acpMemCpy((void*)(sEnv + sEnvLength), sEnvs[i], sCurEnvLength);
            sEnvLength += sCurEnvLength;
        }

        /* Add terminator */
        sRC = acpMemRealloc((void**)(&sEnv),
                            sizeof(acp_char_t) * (sEnvLength + 1));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENV_CONVERT_FAILED);
        sEnv[sEnvLength] = ACP_CSTR_TERM;
    }

    /* One or more arguments */
    sRet = CreateProcess(NULL,
                         acpStrGetBuffer(&sPath),
                         NULL,       /* No process attributes */
                         NULL,       /* No thread attributes */
                         TRUE,       /* Allow handle inheritance */
                         0,          /* no need to use console */
                         sEnv,       /* Environment variables */
                         NULL,       /* No current directory */
                         &sStartupInfo,
                         &sProcessInfo);

    ACP_STR_FINAL(sPath);

    if(ACP_FALSE == aDuplicateEnv)
    {
        if(NULL != sEnvs)
        {
            acpMemFree(sEnvs[0]);
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        /* No need of free */
    }
        
    acpMemFree(sEnv);

    ACP_TEST_RAISE(0 == sRet, EXEC_FAILED);

    *aProc = sProcessInfo.dwProcessId;

    /* we should close hThread handle opened in kernel */
    (void)CloseHandle(sProcessInfo.hThread);

    if (gIsCldNoWait == ACP_FALSE)
    {
        (void)acpSpinLockLock(&gAcpProcLock);
        /* Insert Process ID into list */
        (void)acpChunkInsertElement(*aProc);
        (void)acpSpinLockUnlock(&gAcpProcLock);
    }
    else
    {
        /* do nothing */
    }
    
    acpPathPoolFinal(&sPathPool);
    
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(FILE_NOT_FOUND)
    {
        /* We have sRC as error code */
    }

    ACP_EXCEPTION(PATH_CONVERT_FAILED)
    {
        ACP_STR_FINAL(sPath);
        /* We have sRC as error code */
    }

    ACP_EXCEPTION(ENV_ALLOC_FAILED)
    {
        ACP_STR_FINAL(sPath);
        if(ACP_FALSE == aDuplicateEnv)
        {
            if(NULL != sEnvs)
            {
                acpMemFree(sEnvs[0]);
            }
            else
            {
                /* Do nothing */
            }
                
            acpMemFree(sEnvs);
        }
        else
        {
            /* No need of free */
        }
        /* We have sRC as error code */
    }

    ACP_EXCEPTION(ROOT_ALLOC_FAILED)
    {
        ACP_STR_FINAL(sPath);
        acpMemFree(sEnvs);
        /* We have sRC as error code */
    }

    ACP_EXCEPTION(ENV_CONVERT_FAILED)
    {
        ACP_STR_FINAL(sPath);
        if(ACP_FALSE == aDuplicateEnv)
        {
            if(NULL != sEnvs)
            {
                acpMemFree(sEnvs[0]);
            }
            else
            {
                /* Do nothing */
            }
                
            acpMemFree(sEnvs);
        }
        else
        {
            /* No need of free */
        }

        acpMemFree(sEnv);
        /* We have sRC as error code */
    }

    ACP_EXCEPTION(EXEC_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    
    acpPathPoolFinal(&sPathPool);

    ACP_EXCEPTION_END;
    return sRC;
}

ACP_EXPORT acp_rc_t acpProcExecEnv(
    acp_char_t*                 aFile,
    acp_char_t* const           aArgs[],
    acp_char_t* const           aEnvs[],
    const acp_bool_t            aDuplicateEnv,
    const acp_bool_t            aSearchInPath)
{
    acp_proc_t sProc;
    acp_rc_t   sRC;

    sRC = acpProcForkExecEnv(
            &sProc, aFile, aArgs, aEnvs,
            aDuplicateEnv, aSearchInPath);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* exits the parent process normally */
        acpProcExit(0);
        return ACP_RC_GET_OS_ERROR();
    }
}

ACP_EXPORT acp_rc_t acpProcWait(acp_proc_t      *aProc,
                                acp_proc_wait_t *aWaitResult,
                                acp_bool_t       aUntilTerminate)
{
    acp_rc_t sRC;

    if(gIsCldNoWait)
    {
        return ACP_RC_ECHILD;
    }
    else
    {
        /* fall through */
    }

    if(NULL == aProc)
    {
        /* Check whether child node list is empty */
        acpChunks* sIter = gAcpSentinel;

        if(sIter->mNext == gAcpSentinel && 0 == sIter->mCount)
        {
            /* No child to wait */
            sRC = ACP_RC_ECHILD;
        }
        else
        {
            do {
                /* Lock and wait for children */
                (void)acpSpinLockLock(&gAcpProcLock);

                /* From first to last */
                sIter = (0 == gAcpSentinel->mCount)?
                    gAcpSentinel->mNext : gAcpSentinel;

                do {
                    sRC = acpProcWaitForListNode(sIter, aWaitResult);
                    if(ACP_RC_IS_EAGAIN(sRC))
                    {
                        /* Do nothing */
                        sIter = sIter->mNext;
                    }
                    else
                    {
                        aUntilTerminate = FALSE;
                        if(0 == sIter->mCount)
                        {
                            acpChunkDeleteNode(sIter);
                        }
                        else
                        {
                            /* sIter is still filled */
                        }
                        break;
                    }
                } while(sIter != gAcpSentinel);

                (void)acpSpinLockUnlock(&gAcpProcLock);
                /* Sleep for a while */
                acpSleepMsec(100);
            } while(aUntilTerminate);
        }
    }
    else
    {
        acpProcHandle sHandle;
        DWORD sTimeout;

        if(aUntilTerminate)
        {
            sTimeout = INFINITE;
        }
        else
        {
            sTimeout = 0;
        }

        if(ACP_RC_IS_SUCCESS(
                sRC = acpProcGetHandle(*aProc, &sHandle)))
        {
            sRC = WaitForSingleObject(sHandle, sTimeout);
            if(ACP_RC_IS_SUCCESS(sRC))
            {
                /* Find and delete exited child process ID */
                (void)acpSpinLockLock(&gAcpProcLock);
                (void)acpChunkDeleteElement(*aProc);
                if(NULL == aWaitResult)
                {
                    /* Do nothing */
                }
                else
                {
                    *aWaitResult = acpProcGetWaitResult(sHandle);
                }
                (void)acpSpinLockUnlock(&gAcpProcLock);
            }
            (void)CloseHandle(sHandle);
        }
        else
        {
            /* Cannot get handle
             * Process ID is invalid */

            sRC = ACP_RC_ECHILD;
        }
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpProcKill(acp_proc_t *aProc)
{
    acp_rc_t sRC;
    acpProcHandle sHandle;

    if(ACP_RC_IS_SUCCESS(sRC = acpProcGetHandle(*aProc, &sHandle)))
    {
        acpSleepMsec(100);
        if(0 == TerminateProcess(sHandle, STATUS_PROCESS_KILLED))
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /* If the process killed is one of my children,
             * We don't have to remove it
             * acpProcWait will handle it handsomely. */
        }
        (void)CloseHandle(sHandle);
    }
    else
    {
        /* Cannot get handle
         * Process ID is invalid */
    }

    return sRC;
}

ACP_EXPORT void acpProcExit(acp_sint32_t aExitCode)
{
    /* Exit with aExitCode */
    (void)acpStdFlush(NULL);
    (void)TerminateProcess(GetCurrentProcess(), ACP_PROC_EXITCODE(aExitCode));
#if defined(__STATIC_ANALYSIS_DOING__)
    /* codesonar doesn't stop on TerminateProcess(),
       so, exit force
    */
    exit(aExitCode);
    
#endif
}

ACP_EXPORT void acpProcAbort(void)
{
    /* Terminate process with SIGABRT */
    /* or raise SIGABRT
    (void)acpStdFlush(NULL);
    (void)TerminateProcess(GetCurrentProcess(), SIGABRT);
    */
    raise(SIGABRT);
}

ACP_EXPORT acp_rc_t acpProcFromPID(acp_proc_t *aProc, acp_uint32_t aPID)
{
    acp_rc_t sRC;

    if(NULL == aProc)
    {
        sRC = ACP_RC_EINVAL;
    }
    else
    {
        *aProc = aPID;
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpProcIsAlive(acp_proc_t *aProc, acp_bool_t *aIsAlive)
{
    acp_rc_t sRC;

    if(NULL == aProc)
    {
        sRC = ACP_RC_EINVAL;
    }
    else
    {
        sRC = acpProcIsAliveByPID(acpProcGetID(aProc), aIsAlive);
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpProcIsAliveByPID(acp_uint32_t aPID, acp_bool_t *aIsAlive)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    acpProcHandle   sRetHandle;
    DWORD    sExitCode;
    BOOL     sRetBool;

    /* Windows Server 2003 & Windows XP/2000 should have
     * the PROCESS_QUERY_INFORMATION access right of the handle. */
    sRetHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)aPID);

    if (sRetHandle == NULL)
    {
        sRC = ACP_RC_GET_OS_ERROR();

        *aIsAlive = ACP_FALSE;

        /* ERROR_INVALID_PARAMETER means the passwd PID is not valid */
        if (sRC == ACP_RC_FROM_SYS_ERROR(ERROR_INVALID_PARAMETER))
        {
            return ACP_RC_SUCCESS;
        }
        else
        {
            return sRC;
        }
    }
    else
    {
        /* do nothing */
    }

    /* Check it again by using the process handle we got */
    sRetBool = GetExitCodeProcess(sRetHandle, &sExitCode);

    if (sRetBool == 0)
    {
        *aIsAlive = ACP_FALSE;

        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        if (sExitCode == STILL_ACTIVE)
        {
            *aIsAlive = ACP_TRUE;
        }
        else
        {
            *aIsAlive = ACP_FALSE;
        }
    }

    (void)CloseHandle(sRetHandle);

    return sRC;
}

ACP_EXPORT acp_rc_t acpProcNoCldWait(void)
{
    acpChunks*  sIter = NULL;

    acpSpinLockLock(&gAcpProcLock);

    sIter = gAcpSentinel->mNext;
    do {
        acpChunkDeleteNode(sIter);
        sIter = sIter->mNext;
    } while(sIter != gAcpSentinel);

    gAcpSentinel->mNext = gAcpSentinel;
    gAcpSentinel->mPrev = gAcpSentinel;
    gAcpSentinel->mCount = 0;
    gIsCldNoWait = ACP_TRUE;

    acpSpinLockUnlock(&gAcpProcLock);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpProcDaemonize(acp_char_t *aWorkingDirPath,
                                     acp_bool_t  aCloseAllHandle)
{
    acp_path_pool_t sPool;
    acp_rc_t        sRC;
    BOOL            sRet;

    sRC = acpOSGetVersionInfo();

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * changes working directory
     */
    if (aWorkingDirPath != NULL)
    {
        acp_char_t* sPath = NULL;

        acpPathPoolInit(&sPool);
        sPath = acpPathFull(aWorkingDirPath, &sPool);


        if (NULL == sPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if(-1 == _chdir(sPath))
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
                /* Needless? */
                sRC = ACP_RC_SUCCESS;
            }
        }

        acpPathPoolFinal(&sPool);
    }
    else
    {
        /* If aWorkingDirPath is NULL,
         * WE DO NOT CHANGE A PROCESS WORKING PATH */
    }

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        if (acpOSGetVersion() >= ACP_OS_WIN_2000)
        {
            /* detaches the calling process from its console */
            sRet = FreeConsole();

            if (sRet == 0)
            {
                return ACP_RC_GET_OS_ERROR();
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* close standard handles */
            (void)CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
            (void)CloseHandle(GetStdHandle(STD_OUTPUT_HANDLE));
            (void)CloseHandle(GetStdHandle(STD_ERROR_HANDLE));
        }
    }
    else
    {
        /* Do nothing
         * return error code */
    }

    return sRC;
}

/**
 * executes aCommand
 * @param aCommand command line to execute
 * @param aWR wait result of system when ACP_RC_SUCCESS,
 * undefined when not successful. In both cases, aWR->mPID is useless.
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcSystem(
    acp_char_t* aCommand,
    acp_proc_wait_t* aWR)
{
    PROCESS_INFORMATION  sProcessInfo;
    STARTUPINFO          sStartupInfo;
    DWORD                sRet;
    acp_rc_t             sRC;
    acp_char_t*          sCmd = NULL;

    ACP_STR_DECLARE_DYNAMIC(sPath);
    ACP_STR_INIT_DYNAMIC(sPath, MAX_PATH, MAX_PATH * 2);
    /* acp_str_t have to support infinite extension later */

    sRC = acpEnvGet(ACP_PROC_WIN_CMD_VAR, &sCmd);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = acpStrCpyFormat(&sPath, ACP_PROC_WIN_SYSTEM, sCmd, aCommand);
    }
    else
    {
        if(ACP_RC_IS_ENOENT(sRC))
        {
            sRC = acpStrCpyFormat(
                    &sPath, ACP_PROC_WIN_SYSTEM,
                    ACP_PROC_WIN_DEFAULT_CMD, aCommand
                    );
        }
        else
        {
            /* Do nothing */
        }
    }


    if(ACP_RC_NOT_SUCCESS(sRC))
    {
        /* Do nothing. return error code */
    }
    else
    {
        /* fills memory block with zero */
        ZeroMemory(&sStartupInfo, sizeof(STARTUPINFO));

        /* fills the default information  */
        sStartupInfo.cb         = sizeof(STARTUPINFO);
        sStartupInfo.dwFlags    = STARTF_USESTDHANDLES;
        sStartupInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
        sStartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        sStartupInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

        sRet = CreateProcess(
                NULL,
                acpStrGetBuffer(&sPath),
                NULL,       /* No process attributes */
                NULL,       /* No thread attributes */
                TRUE,       /* Allow handle inheritance */
                0,          /* no need to use console */
                NULL,       /* No environment */
                NULL,       /* No current directory */
                &sStartupInfo,
                &sProcessInfo);

        if(0 == sRet)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sRet = WaitForSingleObject(sProcessInfo.hProcess, INFINITE);

            /* No need of error handling once the process is created */
            if(NULL == aWR)
            {
                /* Do nothing */
            }
            else
            {
                *aWR = acpProcGetWaitResult(sProcessInfo.hProcess);
                if(ACP_PROC_WIN_CMDNOTFOUND == aWR->mExitCode)
                {
                    /* Map exit code when command not found */
                    aWR->mExitCode = ACP_PROC_EXITCODE(ACP_PROC_NOTFOUNDCODE);
                }
                else
                {
                    /* Do nothing */
                }
            }
        }
    }

    ACP_STR_FINAL(sPath);

    return sRC;
}

ACP_EXPORT acp_rc_t acpProcDetachConsole(acp_char_t *aWorkingDirPath,
                                     acp_bool_t  aCloseAllHandle)
{
    acp_path_pool_t sPool;
    acp_rc_t        sRC;

    sRC = acpOSGetVersionInfo();

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * changes working directory
     */
    if (aWorkingDirPath != NULL)
    {
        acp_char_t* sPath = NULL;

        acpPathPoolInit(&sPool);
        sPath = acpPathFull(aWorkingDirPath, &sPool);


        if (NULL == sPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if(-1 == _chdir(sPath))
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
                /* Needless? */
                sRC = ACP_RC_SUCCESS;
            }
        }

        acpPathPoolFinal(&sPool);
    }
    else
    {
        /* If aWorkingDirPath is NULL,
         * WE DO NOT CHANGE A PROCESS WORKING PATH */
    }

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* At all versions, it must close standard handles */
        (void)CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
        (void)CloseHandle(GetStdHandle(STD_OUTPUT_HANDLE));
        (void)CloseHandle(GetStdHandle(STD_ERROR_HANDLE));
    }
    else
    {
        /* Do nothing
         * return error code */
    }

    return sRC;
}
