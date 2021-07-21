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
 * $Id: acpProc-UNIX.c 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#include <acpEnv.h>
#include <acpPath.h>
#include <acpProc.h>
#include <acpSys.h>

#define ACP_PROC_TERMINATE_PROCESS_IF_FORK_SUCCESS(aPID)   \
    if ((aPID) == -1)                                      \
    {                                                      \
        return ACP_RC_GET_OS_ERROR();                      \
    }                                                      \
    else                                                   \
    {                                                      \
        if ((aPID) != 0)                                   \
        {                                                  \
            exit(0);                                       \
        }                                                  \
        else                                               \
        {                                                  \
            /* do nothing */                               \
        }                                                  \
    }

static void acpProcZombieHandler(acp_sint32_t aSignal)
{
    acp_sint32_t sStatus;
    pid_t        sPID;

    ACP_UNUSED(aSignal);

    do
    {
        sPID = waitpid(-1, &sStatus, WNOHANG);
    }
    while (sPID > 0);
}

/**
 * make parent not to call acpProcWait() directly.
 * After it is called, the parent process should not
 * call acpProcWait() directly.
 *
 * @return result code
 *
 * @see acpProcWait()
 */
ACP_EXPORT acp_rc_t acpProcNoCldWait(void)
{
    struct sigaction sSigAct;
    acp_sint32_t     sRet;
    acp_rc_t         sRC;

    sRet = sigemptyset(&sSigAct.sa_mask);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sSigAct.sa_handler = acpProcZombieHandler;
    sSigAct.sa_flags   = SA_RESTART;

    sRet = sigaction(SIGCHLD, &sSigAct, NULL);

    if (sRet != 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

static acp_rc_t acpProcFindPath(acp_char_t*         aFile,
                                acp_char_t**        aBuffer,
                                acp_path_pool_t*    aPool,
                                acp_bool_t          aSearchInPath)
{
    acp_rc_t     sRC;

    if((NULL == strstr(aFile, "/")) && (ACP_TRUE == aSearchInPath))
    {
        /*
         * aFile is not absolute and aSearchInPath is TRUE
         * find aPath in PATH
         */
        acp_uint32_t sPathIndex = 0;
        acp_uint32_t sFindIndex = 0;
        acp_rc_t     sFileRC;
        acp_char_t*  sPath = NULL;

        acp_stat_t   sStat;
        acp_char_t   sPathFinder[ACP_PATH_MAX_LENGTH + 1];

        sRC = acpEnvGet("PATH", &sPath);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), NO_PATH_VALUE);

        do /* Loop until the end of $PATH */
        {
            for(sPathIndex = 0;
                (sPath[sFindIndex] != ':') &&
                (sPath[sFindIndex] != ACP_CSTR_TERM);
                sFindIndex++, sPathIndex++)
            {
                sPathFinder[sPathIndex] = sPath[sFindIndex];
            }

            /* Skip ':' */
            sPathFinder[sPathIndex] = '/';
            sPathFinder[sPathIndex + 1] = ACP_CSTR_TERM;

            /* concat filename after path */
            sRC = acpCStrCat(sPathFinder, ACP_PATH_MAX_LENGTH - sPathIndex,
                             aFile, ACP_PATH_MAX_LENGTH);
            sFileRC = acpFileStatAtPath(sPathFinder, &sStat, ACP_TRUE);

            if(ACP_RC_IS_SUCCESS(sFileRC))
            {
                /* Found! */
                *aBuffer = (acp_char_t*)acpPathFull(sPathFinder, aPool);
                ACP_TEST_RAISE(NULL == *aBuffer, PATH_FULL_FAILED);
                break;
            }
            else
            {
                ACP_TEST_RAISE(ACP_CSTR_TERM == sPath[sFindIndex],
                               FILE_NOT_FOUND);
                /* Loop */
                sFindIndex++;
            }
        } while(ACP_TRUE);
    }
    else
    {
        /* aFile is absolute or not find aPath */
        (*aBuffer) = (acp_char_t*)acpPathFull(aFile, aPool);
        ACP_TEST_RAISE(NULL == (*aBuffer), PATH_FULL_FAILED);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(NO_PATH_VALUE)
    {
        /* There is no PATH but aSearchInPath is ACP_TRUE */
        sRC = ACP_RC_ENOTSUP;
    }

    ACP_EXCEPTION(FILE_NOT_FOUND)
    {
        /* File not found */
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION(PATH_FULL_FAILED)
    {
        /* File not found */
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;
    return sRC;
}

static acp_rc_t acpProcCopyEnv(acp_char_t***        aDest,
                               acp_char_t* const    aEnvs[],
                               acp_bool_t           aDuplicateEnv)
{
    acp_char_t**        sEnvs = NULL;
    acp_rc_t            sRC;

    if(ACP_TRUE == aDuplicateEnv)
    {
        acp_size_t          sEnvBefore;
        acp_size_t          sEnvAfter = (acp_size_t)0;
        acp_size_t          sEnvAdditional = (acp_size_t)0;
        acp_sint32_t        i;

        while(NULL != aEnvs[sEnvAdditional])
        {
            sEnvAdditional++;
        }

        (void)acpEnvList(NULL, 0, &sEnvAfter);
        sEnvs = NULL;

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
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENVALLOC_FAILED);
            (void)acpEnvList(sEnvs + sEnvAdditional, sEnvBefore, &sEnvAfter);
        } while(sEnvBefore != sEnvAfter);

        for(i = 0; NULL != aEnvs[i]; i++)
        {
            sEnvs[i] = aEnvs[i];
        }
        sEnvs[sEnvAfter + sEnvAdditional] = NULL;
    }
    else
    {
        sEnvs = (acp_char_t**)aEnvs;
    }

    *aDest = sEnvs;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENVALLOC_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
        if(NULL != sEnvs)
        {
            acpMemFree(sEnvs);
        }
        else
        {
            /* Do nothing */
        }
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * replaces current process image with a new process image
 *
 * @param aFile file name or path name to executable program
 * @param aArgs array of arguments
 * @param aEnvs environment variables to pass to newly exec'ed process.
 *  must be in the form of "VARNAME=VARVALUE", last element with NULL.
 *  when this value is NULL, the environment variables shall not be changed.
 * @param aDuplicateEnv a boolean value whether to duplicate current process'
 *  environment variables. If this value is #ACP_FALSE, newly executed process
 *  shall have only @a aEnvs as its environment variables.
 * @param aSearchInPath whether the @a aFile could be searched
 * in the PATH envrionment
 * @return result code
 *
 * @a aArgs is a pointer to a null-terminated array of character pointers to
 * null-terminated character strings. These strings construct the argument list
 * to be made available to the new process. At least one argument must be
 * present in the array; by custom, the first element should be the name of the
 * executed program
 * (for example, the last component of executable program path).
 */
ACP_EXPORT acp_rc_t acpProcExecEnv(
    acp_char_t*                 aFile,
    acp_char_t* const           aArgs[],
    acp_char_t* const           aEnvs[],
    const acp_bool_t            aDuplicateEnv,
    const acp_bool_t            aSearchInPath)
{
    acp_rc_t            sRC;
    acp_sint32_t        sArgCount;
    acp_sint32_t        i;

    acp_char_t**        sArgs = NULL;
    acp_char_t**        sEnvs = NULL;

    acp_path_pool_t     sPool;

    for(sArgCount = 0; aArgs[sArgCount] != NULL; sArgCount++)
    {
        /* Do-nothing loop */
    }

    sRC = acpMemAlloc((void**)(&sArgs), sizeof(acp_char_t*) * (sArgCount + 2));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ALLOC_FAILED);

    acpPathPoolInit(&sPool);

    /* Copy trailing arguments */
    for(i = 0; i <= sArgCount; i++)
    {
        sArgs[i + 1] = aArgs[i];
    }

    /* When there's no new environment variable */
    if(NULL == aEnvs)
    {
        /* Set argument 0 as binary command file name */
        sArgs[0] = (acp_char_t*)acpPathFull(aFile, &sPool);

        if (NULL == sArgs[0])
        {
            sRC = ACP_RC_GET_OS_ERROR();
            ACP_RAISE(EXEC_FAILED);
        }
        else
        {
            /* do nothing */
        }

        if(ACP_TRUE == aSearchInPath)
        {
            (void)execvp(sArgs[0], sArgs);
        }
        else
        {
            (void)execv(sArgs[0], sArgs);
        }
    }
    /* There are new environment variables */
    else
    {
        /* Set argument 0 as binary command file name */
        sRC = acpProcFindPath(aFile, &(sArgs[0]), &sPool, aSearchInPath);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), EXEC_FAILED);

        sRC = acpProcCopyEnv(&sEnvs, aEnvs, aDuplicateEnv);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), EXEC_FAILED);

        (void)execve(sArgs[0], sArgs, sEnvs);
    }

    /* Must not reach here when successful.
     * When reached, there is an error */
    ACP_RAISE(EXEC_RETURNED);

    ACP_EXCEPTION(ALLOC_FAILED)
    {
        /* Handle nothing */
    }

    ACP_EXCEPTION(EXEC_FAILED)
    {
        acpPathPoolFinal(&sPool);
        acpMemFree(sArgs);
    }
    
    ACP_EXCEPTION(EXEC_RETURNED)
    {
        /* clean all */
        acpPathPoolFinal(&sPool);
        acpMemFree(sArgs);
        if((NULL != aEnvs) &&
           (ACP_TRUE == aDuplicateEnv) &&
           (NULL != sEnvs))
        {
            acpMemFree(sEnvs);
        }
        else
        {
            /* Do nothing */
        }

        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;

    return sRC;
}

/**
 * Launch daemon process, it creates a child process
 * but the parent does not need to wait the created process.
 *
 * @param aProc pointer to the process object
 * @param aFile file name or path name to executable program
 * @param aArgs argument array to execute program
 * @param aSearchInPath whether the @a aFile could be searched
 * in the PATH environment
 * @return ACP_RC_SUCCESS when successful.
 * ACP_RC_ENOENT when aFile not exists
 * ACP_RC_ESRCH when execution failed
 * @see acpProcExec()
 */
ACP_EXPORT acp_rc_t acpProcLaunchDaemon(
    acp_proc_t             *aProc,
    acp_char_t             *aFile,
    acp_char_t* const       aArgs[],
    acp_bool_t              aSearchInPath)
{
    pid_t        sForkPID;
    acp_rc_t     sRC = ACP_RC_SUCCESS;
    acp_sint32_t sPipe[2];
    acp_sint32_t sRet;
    acp_ssize_t  sSize = 0;

    acp_proc_t sSecondChild;


    /*
     * open pipe to get error of exec()
     */
    sRet = pipe(sPipe);
    ACP_TEST_RAISE(0 != sRet, PIPE_CREATE_FAILED);

    /*
     * set FD_CLOEXEC flag to be automatically closed after successful exec()
     */
    sRet = fcntl(sPipe[1], F_GETFD);
    ACP_TEST_RAISE(-1 == sRet, PIPE_SET_FAILED);

    sRet = fcntl(sPipe[1], F_SETFD, sRet | FD_CLOEXEC);
    ACP_TEST_RAISE(-1 == sRet, PIPE_SET_FAILED);

    /*
     * fork a child process
     */
    sForkPID = fork();

    switch(sForkPID)
    {
    case -1:
        ACP_RAISE(FORK_FAILED);
        break;
    case 0:
        /*
         * first child process forks the second child
         * and terminates immediatly
         * so that the second child has init-process as parent.
         */
        (void)close(sPipe[0]);

        if (setsid() == -1)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /* setsid() successed */
            sRC = ACP_RC_SUCCESS;
        }

        /* EPERM means the process is already session leader,
           so it is not failure */
        if (ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_EPERM(sRC))
        {
            sRC = acpProcForkExecEnv(&sSecondChild,
                                     aFile,
                                     aArgs,
                                     NULL,
                                     ACP_FALSE,
                                     aSearchInPath);
        }
        else
        {
            /* do not fork child and exit */
        }

        /* alarm parent that second child is created */
        (void)write(sPipe[1], &sRC, sizeof(sRC));
        acpProcExit(0);
        break;

    default:
        /*
         * parent process
         */

        /* Read child status */
        sSize = read(sPipe[0], &sRC, sizeof(sRC));

        if (sSize != sizeof(sRC))
        {
            sRC = ACP_RC_ESRCH;
            ACP_RAISE(CHILD_FAILED);
        }
        else
        {
            /* Continue with sRC from child */
        }

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CHILD_FAILED);

        (void)close(sPipe[0]);
        (void)close(sPipe[1]);

        *aProc = sForkPID;
    }

    /* wait the first child process termination,
     * so that the parent does not need to wait again
     * the outsize of this function */
    (void)waitpid(sForkPID, NULL, 0);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(PIPE_CREATE_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(PIPE_SET_FAILED)
    {
        (void)close(sPipe[0]);
        (void)close(sPipe[1]);
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(FORK_FAILED)
    {
        (void)close(sPipe[0]);
        (void)close(sPipe[1]);
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(CHILD_FAILED)
    {
        (void)waitpid(sForkPID, NULL, 0);

        (void)close(sPipe[0]);
        (void)close(sPipe[1]);
    }


    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * creates a child process
 *
 * @param aProc pointer to the process object
 * @param aFile file name or path name to executable program
 * @param aArgs argument array to execute program
 * @param aEnvs environment variables to pass to newly exec'ed process.
 *  must be in the form of "VARNAME=VARVALUE", last element with NULL.
 *  when this value is NULL, the environment variables shall not be changed.
 * @param aDuplicateEnv a boolean value whether to duplicate current process'
 *  environment variables. If this value is #ACP_FALSE, newly executed process
 *  shall have only @a aEnvs as its environment variables.
 * @param aSearchInPath whether the @a aFile could be searched
 * in the PATH environment
 * @param aStdHandles a pointer to the structure with handles which shoud be
 * used as a standard handles
 * @return ACP_RC_SUCCESS when successful.
 * ACP_RC_ENOENT when aFile not exists
 * ACP_RC_ESRCH when execution failed
 * @see acpProcExec()
 */
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
    pid_t        sForkPID;
    acp_rc_t     sRC;
    acp_sint32_t sPipe[2];
    acp_sint32_t sRet;
    acp_ssize_t  sSize;
    acp_file_t   sDupNewHandle;

    ACP_UNUSED(aOption);

    /*
     * open pipe to get error of exec()
     */
    sRet = pipe(sPipe);
    ACP_TEST_RAISE(0 != sRet, PIPE_CREATE_FAILED);

    /*
     * set FD_CLOEXEC flag to be automatically closed after successful exec()
     */
    sRet = fcntl(sPipe[1], F_GETFD);
    ACP_TEST_RAISE(-1 == sRet, PIPE_SET_FAILED);

    sRet = fcntl(sPipe[1], F_SETFD, sRet | FD_CLOEXEC);
    ACP_TEST_RAISE(-1 == sRet, PIPE_SET_FAILED);

    /*
     * fork a child process
     */
    sForkPID = fork();

    switch(sForkPID)
    {
    case -1:
        ACP_RAISE(FORK_FAILED);
        break;
    case 0:
        /*
         * child process
         */
        (void)close(sPipe[0]);

        /*
         * Associate aStdHandlers with std streams of the child process
         */
        if (aStdHandles != NULL)
        {
            if (aStdHandles->mStdIn != NULL)
            {
                sRet = close(STDIN_FILENO);
                ACP_TEST_RAISE(-1 == sRet, STDHANDLE_DUP_FAILED);
                sRC = acpFileDup(aStdHandles->mStdIn, &sDupNewHandle);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), STDHANDLE_DUP_FAILED);
            }
            else
            {
                /*
                 * Do not redefine stdin handle
                 */
            }

            if (aStdHandles->mStdOut != NULL)
            {
                sRet = close(STDOUT_FILENO);
                ACP_TEST_RAISE(-1 == sRet, STDHANDLE_DUP_FAILED);
                sRC = acpFileDup(aStdHandles->mStdOut, &sDupNewHandle);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), STDHANDLE_DUP_FAILED);
            }
            else
            {
                /*
                 * Do not redefine stdout handle
                 */
            }

            if (aStdHandles->mStdErr != NULL)
            {
                sRet = close(STDERR_FILENO);
                ACP_TEST_RAISE(-1 == sRet, STDHANDLE_DUP_FAILED);
                sRC = acpFileDup(aStdHandles->mStdErr, &sDupNewHandle);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), STDHANDLE_DUP_FAILED);
            }
            else
            {
                /*
                 * Do not redefine stderr handle
                 */
            }
        }
        else
        {
            /* 
             * Do not redefine std handles
             */
        }

        sRC = acpProcExecEnv(aFile, aArgs, aEnvs, aDuplicateEnv, aSearchInPath);
        (void)write(sPipe[1], &sRC, sizeof(sRC));
        acpProcExit(0);
        break;

    default:
        /*
         * parent process
         */
        *aProc = sForkPID;
        
        (void)close(sPipe[1]);
        sSize = read(sPipe[0], &sRC, sizeof(sRC));
        ACP_TEST_RAISE(0 != sSize, CHILD_FAILED);

        (void)close(sPipe[0]);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(PIPE_CREATE_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(PIPE_SET_FAILED)
    {
        (void)close(sPipe[0]);
        (void)close(sPipe[1]);
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(FORK_FAILED)
    {
        (void)close(sPipe[0]);
        (void)close(sPipe[1]);
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(CHILD_FAILED)
    {
        if(sizeof(sRC) == sSize)
        {
            /* Child wrote error code to pipe
             * We already received sRC as error code */
        }
        else
        {
            /* No such process */
            sRC = ACP_RC_ESRCH;
        }

        (void)waitpid(sForkPID, NULL, 0);

        (void)close(sPipe[0]);
    }

    ACP_EXCEPTION(STDHANDLE_DUP_FAILED)
    {
        (void)close(sPipe[1]);
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * waits for a child process
 *
 * @param aProc pointer to the process object
 * @param aWaitResult pointer to the #acp_proc_wait_t
 * to store the state of the child process
 * @param aUntilTerminate whether waits until the child process terminates
 * or returns immediately
 * @return result code
 *
 * if @a aProc is null, it waits any child process.
 *
 * it returns #ACP_RC_EAGAIN
 * if @a aUntilTerminate is #ACP_FALSE and there is no state change.
 */
ACP_EXPORT acp_rc_t acpProcWait(acp_proc_t      *aProc,
                                acp_proc_wait_t *aWaitResult,
                                acp_bool_t       aUntilTerminate)
{
    pid_t        sPID;
    acp_sint32_t sState;
    acp_sint32_t sOption;

    /*
     * set up options to call waitpid()
     */
    if (aUntilTerminate == ACP_TRUE)
    {
        sOption = 0;
    }
    else
    {
        sOption = WNOHANG;
    }

    /*
     * call waitpid()
     */
    if (aProc == NULL)
    {
        sPID = waitpid(-1, &sState, sOption);
    }
    else
    {
        sPID = waitpid(*aProc, &sState, sOption);
    }

    /*
     * check return code of waitpid()
     */
    if (sPID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else if (sPID == 0)
    {
        return ACP_RC_EAGAIN;
    }
    else
    {
        /* do nothing */
    }

    /*
     * fill wait result
     */
    if (aWaitResult != NULL)
    {
        aWaitResult->mPID = sPID;

        if((WIFEXITED(sState) != 0) || (WIFSIGNALED(sState) != 0))
        {
            aWaitResult->mState    = ACP_PROC_EXITED;
            aWaitResult->mExitCode = sState;
        }
        else
        {
            aWaitResult->mState    = ACP_PROC_RUNNING;
            aWaitResult->mExitCode = 0;
        }
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

/**
 * kills a process
 *
 * @param aProc pointer to the process object
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcKill(acp_proc_t *aProc)
{
    acp_sint32_t sRet;

    sRet = kill(*aProc, SIGKILL);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

/**
 * exits current process
 *
 * @param aExitCode exit code
 */
ACP_EXPORT void acpProcExit(acp_sint32_t aExitCode)
{
    exit(aExitCode);
}

/**
 * aborts current process
 */
ACP_EXPORT void acpProcAbort(void)
{
    abort();
}

/**
 * get process handle from process id
 *
 * @param aProc pointer to the process object
 * @param aPID process id indicating the process object
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcFromPID(acp_proc_t *aProc, acp_uint32_t aPID)
{
    if (aProc == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        *aProc = aPID;
    }

    return ACP_RC_SUCCESS;
}

/**
 * check the process live from process handle
 * @param aProc pointer to the process object
 * @param aIsAlive whether the process is alive or not
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcIsAlive(acp_proc_t *aProc, acp_bool_t *aIsAlive)
{
    return acpProcIsAliveByPID(acpProcGetID(aProc), aIsAlive);
}

/**
 * check the process aliveness from process id
 * @param aPID process id indicating the process object
 * @param aIsAlive whether the process is alive or not
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcIsAliveByPID(acp_uint32_t aPID,
                                        acp_bool_t *aIsAlive)
{
    acp_sint32_t sRet;
    acp_rc_t     sRC;

    sRet = kill((pid_t)aPID, 0);

    if (sRet != 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();

        if (ACP_RC_IS_ESRCH(sRC))
        {
            *aIsAlive = ACP_FALSE;

            sRC = ACP_RC_SUCCESS;
        }
        else
        {
            if(ACP_RC_IS_EPERM(sRC))
            {
                *aIsAlive = ACP_TRUE;
                sRC = ACP_RC_SUCCESS;
            }
            else
            {
                /* really an error */
            }
        }
    }
    else
    {
        *aIsAlive = ACP_TRUE;

        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

/**
 * This function will be deleted in Nov, 2009.
 *
 * @param aWorkingDirPath working directory path
 * @param aCloseAllHandle whether the daemonized process
 * should be close all file handle
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcDaemonize(acp_char_t *aWorkingDirPath,
                                     acp_bool_t  aCloseAllHandle)
{
    pid_t           sPID;
    acp_path_pool_t sPool;
    acp_rc_t        sRC = ACP_RC_SUCCESS;
    acp_uint32_t    sHandleLimit;
    acp_sint32_t    i;

    /*
     * parent terminates
     */
    sPID = fork();

    ACP_PROC_TERMINATE_PROCESS_IF_FORK_SUCCESS(sPID);

    /*
     * become session leader
     */
    sPID = setsid();

    if (sPID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    /*
     * 1st child terminates
     */
    (void)signal(SIGHUP, SIG_IGN);

    sPID = fork();

    ACP_PROC_TERMINATE_PROCESS_IF_FORK_SUCCESS(sPID);

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
            if (-1 == chdir(sPath))
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
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

    /*
     * clears out file mode create mask
     */
    (void)umask(0);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /*
         * closes all file handle
         */
        if (aCloseAllHandle == ACP_TRUE)
        {
#if defined(ALTI_CFG_OS_AIX)
            acp_slong_t sResource;

            ACP_UNUSED(sHandleLimit);

            /* BUGBUG:
             *   sys_conf() returns a value that means a maximum value of fd,
             *   it returns how many fd can be opened. */

            /* _SC_OPEN_MAX :
             *    the maximum number of files that
             *    one process can have open at any one time */
            sResource = sysconf(_SC_OPEN_MAX);

            if (sResource != -1)
            {
                for (i = 0; i < sResource; i++)
                {
                    (void)close(i);
                }
                sRC = ACP_RC_SUCCESS;
            }
            else
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
#else
            sRC = acpSysGetHandleLimit(&sHandleLimit);

            if (ACP_RC_IS_SUCCESS(sRC))
            {
                for (i = 0; (acp_uint32_t)i < sHandleLimit; i++)
                {
                    (void)close(i);
                }
            }
            else
            {
                /* error situation. sRC have the error code */
            }
#endif

        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* Do nothing */
    }

    return sRC;
}

/**
 * executes aCommand
 * @param aCommand command line to execute
 * @param aWR wait result of system when ACP_RC_SUCCESS,
 * undefined when not successful. In both cases, @a aWR->mPID is useless.
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcSystem(
    acp_char_t* aCommand,
    acp_proc_wait_t* aWR)
{
    acp_rc_t sRC;
    acp_sint32_t sResult = system(aCommand);

    if(-1 == sResult)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_SUCCESS;

        if(NULL == aWR)
        {
            /* Do Nothing */
        }
        else
        {
            aWR->mPID = 0;
            aWR->mState = ACP_PROC_EXITED;
            aWR->mExitCode = sResult;
        }
    }

    return sRC;
}

/**
 * Every daemon process must call this function at the beginning.
 *
 * Detach console of current process so that the process cannot
 * print and/or input data on console. And change working path
 * into root-directory and close extra file handle.
 *
 * This function substitues for daemonizing function. So the name
 * might be confused.
 *
 * @param aWorkingDirPath working directory path
 * @param aCloseAllHandle whether the daemonized process
 * should be close all file handle
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcDetachConsole(acp_char_t *aWorkingDirPath,
                                     acp_bool_t  aCloseAllHandle)
{
    acp_path_pool_t sPool;
    acp_rc_t        sRC = ACP_RC_SUCCESS;
    acp_uint32_t    sHandleLimit;
    acp_sint32_t    i;


    /*
     * changes working directory
     */
    if (aWorkingDirPath != NULL)
    {
        acp_char_t* sPath = NULL;

        acpPathPoolInit(&sPool);
        sPath = acpPathFull(aWorkingDirPath, &sPool);

        ACP_TEST_RAISE(NULL == sPath, DETACH_FAIL);

        if (-1 == chdir(sPath))
        {
            /* fail to change, exit */
            acpPathPoolFinal(&sPool);
            ACP_RAISE(DETACH_FAIL);
        }
        else
        {
            /* path access is over,
             * no need of path pool
             */
            acpPathPoolFinal(&sPool);
        }

    }
    else
    {
        /* If aWorkingDirPath is NULL,
         * WE DO NOT CHANGE A PROCESS WORKING PATH */
    }

    /*
     * clears out file mode create mask
     */
    (void)umask(0);

    /*
     * closes all file handle (and STD_OUT, STD_IN, STD_ERR)
     */
    if (aCloseAllHandle == ACP_TRUE)
    {
#if defined(ALTI_CFG_OS_AIX)
        acp_slong_t sResource;

        ACP_UNUSED(sHandleLimit);

        /* BUGBUG:
         *   sys_conf() returns a value that means a maximum value of fd,
         *   it returns how many fd can be opened. */

        /* _SC_OPEN_MAX :
         *    the maximum number of files that
         *    one process can have open at any one time */
        sResource = sysconf(_SC_OPEN_MAX);

        ACP_TEST_RAISE(sResource == -1, DETACH_FAIL);

        for (i = 0; i < sResource; i++)
        {
            (void)close(i);
        }
#else
        sRC = acpSysGetHandleLimit(&sHandleLimit);

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), DETACH_FAIL);

        for (i = 0; (acp_uint32_t)i < sHandleLimit; i++)
        {
            (void)close(i);
        }
#endif

    }
    else
    {
        /* do nothing */
    }

    ACP_EXCEPTION(DETACH_FAIL)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;
    return sRC;
}
