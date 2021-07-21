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
 * $Id: acpCallstack.c 5767 2009-05-25 04:58:31Z djin $
 ******************************************************************************/

#include <acpCallstack.h>
#include <acpCallstackPrivate.h>
#include <acpPrintf.h>

typedef acp_rc_t acpCallstackPrintFormatFunc(void             *aTarget,
                                             const acp_char_t *aFormat,
                                             ...);

typedef struct acpCallstackDump
{
    acpCallstackPrintFormatFunc *mFunc;     /* Function to print */
    void                        *mTarget;   /* Target to print*/
} acpCallstackDump;


/*
 * Dump aFrame to aDump
 */
static acp_sint32_t acpCallstackDumpCallback(acp_callstack_frame_t *aFrame,
                                             void                  *aDump)
{
    acpCallstackDump *sDump = aDump;

    /* Printing function already defined */
    (void)(*sDump->mFunc)(sDump->mTarget,
                          "%3d: %0*p",
                          aFrame->mIndex,
                          ACP_CFG_COMPILE_BIT / 4 + 2,
                          aFrame->mAddress);

    /* Display file name if exists */
    if ((acpStrGetLength(&aFrame->mSymInfo.mFileName) > 0) ||
        (acpStrGetLength(&aFrame->mSymInfo.mFuncName) > 0))
    {
        if (acpStrGetLength(&aFrame->mSymInfo.mFileName) > 0)
        {
            (void)(*sDump->mFunc)(sDump->mTarget,
                                  " %S:",
                                  &aFrame->mSymInfo.mFileName);
        }
        else
        {
            (void)(*sDump->mFunc)(sDump->mTarget, " ");
        }

        /* Display function name */
        if (acpStrGetLength(&aFrame->mSymInfo.mFuncName) > 0)
        {
            (void)(*sDump->mFunc)(sDump->mTarget,
                                  "%S",
                                  &aFrame->mSymInfo.mFuncName);

            /* print pointer */
            if (aFrame->mSymInfo.mFuncAddr != NULL)
            {
                (void)(*sDump->mFunc)(sDump->mTarget,
                                      "+%#lx",
                                      (acp_ulong_t)aFrame->mAddress -
                                      (acp_ulong_t)aFrame->mSymInfo.mFuncAddr);
            }
            else
            {
                /* do nothing */
            }
        }
        /* Display ?? if function name unknown */
        else
        {
            (void)(*sDump->mFunc)(sDump->mTarget, "??");
        }
    }
    else
    {
        /* do nothing */
    }

    (void)(*sDump->mFunc)(sDump->mTarget, "\n");

    return 0;
}

/*
 * Dump aCallstack to aStr 
 */
ACP_EXPORT acp_rc_t acpCallstackDumpToString(acp_callstack_t *aCallstack,
                                             acp_str_t       *aStr)
{
    acpCallstackDump sDump;

    sDump.mFunc   = (acpCallstackPrintFormatFunc *)acpStrCatFormat;
    sDump.mTarget = aStr;

    return acpCallstackTrace(aCallstack, acpCallstackDumpCallback, &sDump);
}

ACP_EXPORT acp_rc_t acpCallstackDumpToStdFile(acp_callstack_t *aCallstack,
                                              acp_std_file_t  *aFile)
{
    acpCallstackDump sDump;

    sDump.mFunc   = (acpCallstackPrintFormatFunc *)acpFprintf;
    sDump.mTarget = aFile;

    return acpCallstackTrace(aCallstack, acpCallstackDumpCallback, &sDump);
}

#if defined(ALTI_CFG_OS_WINDOWS)

/*
 * Windows Callstack Trace
 * Not implemented fully
 * Using dbghelp.dll and symbol files may be needed
 */
ACP_EXPORT acp_rc_t acpCallstackTrace(acp_callstack_t          *aCallstack,
                                      acp_callstack_callback_t *aCallback,
                                      void                     *aCallbackData)
{
    acp_rc_t sRC;

    __try
    {
        sRC = acpCallstackTraceInternal(aCallstack, aCallback, aCallbackData);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        sRC = ACP_RC_EFAULT;
    }

    return sRC;
}

#else

static sigjmp_buf gSigJmpBuf;

static void acpCallstackExceptionHandler(acp_sint32_t aSignal)
{
    ACP_UNUSED(aSignal);

    siglongjmp(gSigJmpBuf, 1);
}

/*
 * Unix Callstack Trace
 * Fully Implemented
 */
ACP_EXPORT acp_rc_t acpCallstackTrace(acp_callstack_t          *aCallstack,
                                      acp_callstack_callback_t *aCallback,
                                      void                     *aCallbackData)
{
    struct sigaction sSigAct;
    struct sigaction sOldSigSegv;
    struct sigaction sOldSigBus;
    sigset_t         sMaskSet;
    sigset_t         sOldSet;
    acp_sint32_t     sRet;
    acp_rc_t         sRC;

    acpMemSet(&sSigAct,     0, sizeof(struct sigaction));
    acpMemSet(&sOldSigSegv, 0, sizeof(struct sigaction));
    acpMemSet(&sOldSigBus,  0, sizeof(struct sigaction));
    
    acpMemSet(&sMaskSet,    0, sizeof(sigset_t));
    acpMemSet(&sOldSet,     0, sizeof(sigset_t));

    
    sRet = sigsetjmp(gSigJmpBuf, 1);

    if (sRet == 0)
    {
        sSigAct.sa_handler = acpCallstackExceptionHandler;
        sSigAct.sa_flags   = 0;
        (void)sigemptyset(&sSigAct.sa_mask);

        (void)sigaction(SIGSEGV, &sSigAct, &sOldSigSegv);
        (void)sigaction(SIGBUS, &sSigAct, &sOldSigBus);

        (void)sigfillset(&sMaskSet);
        (void)sigdelset(&sMaskSet, SIGSEGV);
        (void)sigdelset(&sMaskSet, SIGBUS);
        (void)pthread_sigmask(SIG_SETMASK, &sMaskSet, &sOldSet);

        sRC = acpCallstackTraceInternal(aCallstack, aCallback, aCallbackData);
    }
    else
    {
        sRC = ACP_RC_EFAULT;
    }

    (void)sigaction(SIGSEGV, &sOldSigSegv, NULL);
    (void)sigaction(SIGBUS, &sOldSigBus, NULL);
    (void)pthread_sigmask(SIG_SETMASK, &sOldSet, NULL);

    return sRC;
}

#endif
