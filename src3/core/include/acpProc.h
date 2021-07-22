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
 * $Id: acpProc.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_PROC_H_)
#define _O_ACP_PROC_H_

/**
 * @file
 * @ingroup CoreProc
 */

#include <acpStr.h>
#include <acpSignal.h>


ACP_EXTERN_C_BEGIN

/**
 * process object
 */
#if defined(ALTI_CFG_OS_WINDOWS)
typedef DWORD acp_proc_t;
#else
typedef pid_t acp_proc_t;
#endif

#define ACP_PROC_T_INITIALIZER ((acp_proc_t)(0))

/**
 * process state
 */
typedef acp_uint32_t acp_proc_state_t;
#define ACP_PROC_RUNNING 0 /**< process is running    */
#define ACP_PROC_EXITED  1 /**< process is terminated */

/**
 * acpProcForkExecEnvPipeOpt options
 */
#define ACP_PROC_FORKEXEC_DEFAULT          0
#define ACP_PROC_FORKEXEC_PRESERVE_PATHSEP 1 /** In Windows,
                                specifying this option prevents the
                                UNIX-style path separators in the
                                arguments from being replaced by
                                Windows-style path separators (See
                                BUG-35105) */

/**
 * struct for process wait result
 *
 * @see acpProcWait()
 */
typedef struct acp_proc_wait_t
{
    /**
     * process id of wait result
     */
    acp_proc_t     mPID;
    /**
     * process state
     */
    acp_proc_state_t mState;
    /**
     * return code of the process
     * (state is #ACP_PROC_EXITED)
     */
    acp_sint32_t     mExitCode;
} acp_proc_wait_t;

/**
 * struct for standart handles redirection
 *
 * @see acpProckForkExecEnvPipe()
 */
typedef struct acp_proc_std_handles_t
{
    /**
     * StdIn handler for child process
     *
     * @see acpFilePipe()
     */
    acp_file_t *mStdIn;
    /**
     * StdOut handler for child process
     *
     * @see acpFilePipe()
     */
    acp_file_t *mStdOut;
    /**
     * StdErr handler for child process
     *
     * @see acpFilePipe()
     */
    acp_file_t *mStdErr;
} acp_proc_std_handles_t;

/**
 * Make exit code
 */
#define ACP_PROC_EXITCODE(wr)       ((acp_uint32_t)((wr) & 0377) << 8)
/**
 * Make signal code
 */
#define ACP_PROC_SIGNALCODE(wr)     ((wr) & 0x7F)
/**
 * Get process ID of wait result
 */
#define ACP_PROC_WR_PID(wr)         ((wr).mPID)
/**
 * Get status of wait result.
 * Values can be ACP_PROC_RUNNING or ACP_PROC_EXITED
 */
#define ACP_PROC_WR_STATE(wr)       ((wr).mState)
/**
 * Get return code of process wait result wr
 * The return code is set with exitcode or signal the process caught
 */
#define ACP_PROC_WR_RETURNCODE(wr)  ((wr).mExitCode & 0xFF7F)
/**
 * Get exit code of process wait result wr
 * The exit code is set with #acpProcExit(), or return value of main
 */
#define ACP_PROC_WR_EXITCODE(wr) (((acp_uint32_t)((wr).mExitCode) >> 8) & 0377)

ACP_EXPORT acp_rc_t acpProcForkExecEnvPipeOpt(
    acp_proc_t*                   aProc,
    acp_char_t*                   aFile,
    acp_char_t* const             aArgs[],
    acp_char_t* const             aEnvs[],
    const acp_bool_t              aDuplicateEnv,
    const acp_bool_t              aSearchInPath,
    const acp_proc_std_handles_t* aStdHandles,
    acp_uint32_t                  aOption);

ACP_INLINE acp_rc_t acpProcForkExecEnvPipe(
    acp_proc_t*                   aProc,
    acp_char_t*                   aFile,
    acp_char_t* const             aArgs[],
    acp_char_t* const             aEnvs[],
    const acp_bool_t              aDuplicateEnv,
    const acp_bool_t              aSearchInPath,
    const acp_proc_std_handles_t* aStdHandles)
{
    return acpProcForkExecEnvPipeOpt(aProc, aFile, aArgs, aEnvs,
                                     aDuplicateEnv, aSearchInPath, aStdHandles,
                                     ACP_PROC_FORKEXEC_DEFAULT);
}

ACP_EXPORT acp_rc_t acpProcExecEnv(
    acp_char_t*                   aFile,
    acp_char_t* const             aArgs[],
    acp_char_t* const             aEnvs[],
    const acp_bool_t              aDuplicateEnv,
    const acp_bool_t              aSearchInPath);

/**
 * replaces current process image with a new process image
 *
 * @param aFile file name or path name to executable program
 * @param aArgs array of arguments
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
ACP_INLINE acp_rc_t acpProcExec(acp_char_t         *aFile,
                                acp_char_t* const   aArgs[],
                                acp_bool_t          aSearchInPath)
{
    return acpProcExecEnv(aFile, aArgs, NULL, ACP_FALSE, aSearchInPath);
}

/**
 * creates a child process
 *
 * @param aProc pointer to the process object
 * @param aFile file name or path name to executable program
 * @param aArgs argument array to execute program
 * @param aEnvs environment variables to be passed to newly exec'ed process.
 *  must be in the form of "VARNAME=VARVALUE", with the last element of NULL.
 *  When this value is NULL, the environment variables should not be changed.
 * @param aDuplicateEnv a boolean value whether to duplicate current process'
 *  environment variables. If this value is #ACP_FALSE, newly executed process
 *  will have only @a aEnvs as its environment variables.
 * @param aSearchInPath whether the @a aFile could be searched
 * in the PATH environment
 * @return ACP_RC_SUCCESS when successful.
 * ACP_RC_ENOENT when aFile not exists
 * ACP_RC_ESRCH when execution failed
 * @see acpProcExec()
 */
ACP_INLINE acp_rc_t acpProcForkExecEnv(
    acp_proc_t*                   aProc,
    acp_char_t*                   aFile,
    acp_char_t* const             aArgs[],
    acp_char_t* const             aEnvs[],
    const acp_bool_t              aDuplicateEnv,
    const acp_bool_t              aSearchInPath)
{
    return acpProcForkExecEnvPipe(aProc, aFile, aArgs, aEnvs,
                                  aDuplicateEnv, aSearchInPath, NULL);
}

/**
 * creates a child process
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
ACP_INLINE acp_rc_t acpProcForkExec(
    acp_proc_t             *aProc,
    acp_char_t             *aFile,
    acp_char_t* const       aArgs[],
    acp_bool_t              aSearchInPath)
{
    return acpProcForkExecEnv(aProc, aFile, aArgs, NULL, ACP_FALSE,
                              aSearchInPath);
}

ACP_EXPORT acp_rc_t acpProcLaunchDaemon(
    acp_proc_t             *aProc,
    acp_char_t             *aFile,
    acp_char_t* const       aArgs[],
    acp_bool_t              aSearchInPath);

ACP_EXPORT acp_rc_t acpProcWait(acp_proc_t      *aProc,
                                acp_proc_wait_t *aWaitResult,
                                acp_bool_t       aUntilTerminate);

ACP_EXPORT acp_rc_t acpProcKill(acp_proc_t *aProc);

ACP_EXPORT void     acpProcExit(acp_sint32_t aExitCode);
ACP_EXPORT void     acpProcAbort(void);

ACP_EXPORT acp_rc_t acpProcFromPID(acp_proc_t *aProc, acp_uint32_t aPID);
ACP_EXPORT acp_rc_t acpProcIsAlive(acp_proc_t *aProc, acp_bool_t *aIsAlive);
ACP_EXPORT acp_rc_t acpProcIsAliveByPID(acp_uint32_t aPID,
                                        acp_bool_t *aIsAlive);

ACP_EXPORT acp_rc_t acpProcDaemonize(acp_char_t *aWorkingDirPath,
                                     acp_bool_t  aCloseAllHandle);

ACP_EXPORT acp_rc_t acpProcNoCldWait(void);

ACP_EXPORT acp_rc_t acpProcSystem(
    acp_char_t* aCommand,
    acp_proc_wait_t* aWR);

ACP_EXPORT acp_rc_t acpProcDetachConsole(acp_char_t *aWorkingDirPath,
                                     acp_bool_t  aCloseAllHandle);

/**
 * gets process id of the specified process
 *
 * @param aProc pointer to the process object
 * @return process id of the process
 */
ACP_INLINE acp_uint32_t acpProcGetID(acp_proc_t *aProc)
{
    return (acp_uint32_t)((*aProc) & ACP_UINT32_MAX);
}

/**
 * gets process id of current process
 *
 * @return process id
 */
ACP_INLINE acp_uint32_t acpProcGetSelfID(void)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    return (acp_uint32_t)GetCurrentProcessId();
#else
    return (acp_uint32_t)getpid();
#endif
}

ACP_EXTERN_C_END

#endif
