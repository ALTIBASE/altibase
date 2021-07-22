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
 * $Id: acpSignal.c 7109 2009-08-21 02:51:19Z djin $
 ******************************************************************************/

#include <acpCallstack.h>
#include <acpCallstackPrivate.h>
#include <acpOS.h>
#include <acpSignal.h>
#include <acpSpinLock.h>
#include <acpThr.h>


#if defined (ALTI_CFG_OS_WINDOWS)
typedef acp_sint32_t sigset_t;
#endif

static sigset_t              gDefaultBlockSet;
static acp_signal_handler_t *gExceptionHandler = NULL;


#if defined (ALTI_CFG_OS_WINDOWS)

/*
 * buildin function to get process termination status infomation
 */
ACP_INLINE acp_sint32_t acpSignalGetSignalType(DWORD aWinException)
{
    acp_sint32_t sSignal;

    switch (aWinException)
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
            sSignal = SIGFPE;
            break;

        /* SIGTKFL */
        case STATUS_FLOAT_STACK_CHECK:
            sSignal = SIGSTKFLT;
            break;

        /* SIGBUS */
        case STATUS_IN_PAGE_ERROR:
            sSignal = SIGBUS;
            break;

        /* SIGSYS */
        case STATUS_INVALID_PARAMETER:
            sSignal = SIGSYS;
            break;

        /* SIGILL */
        case STATUS_ILLEGAL_INSTRUCTION:
        case STATUS_PRIVILEGED_INSTRUCTION:
        case STATUS_NONCONTINUABLE_EXCEPTION:
            sSignal = SIGILL;
            break;

        /* SIGALRM */
        case STATUS_TIMEOUT:
            sSignal = SIGALRM;
            break;

        /* SIGINT */
        case STATUS_CONTROL_C_EXIT:
            sSignal = SIGINT;
            break;

        /* SIGSEGV */
        case STATUS_ACCESS_VIOLATION:
        case STATUS_DATATYPE_MISALIGNMENT:
        case STATUS_ARRAY_BOUNDS_EXCEEDED:
        case STATUS_NO_MEMORY:
        case STATUS_INVALID_DISPOSITION:
        case STATUS_STACK_OVERFLOW:
        case STATUS_STACK_BUFFER_OVERRUN:
            sSignal = SIGSEGV;
            break;

        /* SIGABRT */
        case STATUS_ABORT_EXIT:
        case SIGABRT:
            sSignal = SIGABRT;
            break;

        /* SIGKILL */
        case STATUS_PROCESS_KILLED:
            sSignal = SIGKILL;
            break;

        default:
            sSignal = (acp_sint32_t)aWinException;
            break;
    }

    return sSignal;
}

static LONG __stdcall acpSignalExceptionHandler(
    LPEXCEPTION_POINTERS aExceptionInfo)
{
    PEXCEPTION_RECORD sRecord;
    PCONTEXT          sContext;
    acp_callstack_t   sCallstack;
    acp_sint32_t      sSignal;

    if (gExceptionHandler != NULL)
    {
        sRecord  = aExceptionInfo->ExceptionRecord;
        sContext = aExceptionInfo->ContextRecord;
        sSignal  = acpSignalGetSignalType(sRecord->ExceptionCode);

        acpCallstackInitFromSC(&sCallstack, sContext);

        (*gExceptionHandler)(sSignal, &sCallstack);

        acpCallstackFinal(&sCallstack);
    }
    else
    {
        /* do nothing */
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

static BOOL __stdcall acpSignalCtrlEventHandler(DWORD aCtrlType)
{
    switch(aCtrlType)
    {
        case CTRL_BREAK_EVENT:  /* SIGBREAK */
        case CTRL_C_EVENT:      /* SIGINT */
            /* for Interrupt signals, shut down the server.
             * Tell the system we have dealt with the signal
             * without waiting for Application to terminate.
             */
            if(NULL != gExceptionHandler)
            {
                /* Get current context */
                CONTEXT sContext;
                acp_callstack_t sCallstack;

                RtlCaptureContext(&sContext);

                /* Call exception handler as if we have received SIGINT */
                acpCallstackInitFromSC(&sCallstack, &sContext);
                (*gExceptionHandler)(SIGINT, &sCallstack);
                acpCallstackFinal(&sCallstack);
            }
            else
            {
                /* Do nothing */
            }
            return TRUE;

        case CTRL_CLOSE_EVENT:     /* SIGTERM */
        case CTRL_LOGOFF_EVENT:    /* SIGTERM */
        case CTRL_SHUTDOWN_EVENT:  /* SIGTERM */
            /* for Terminate signals, shut down the application.
             * THESE EVENTS WILL NOT OCCUR UNDER WIN9x!
             */
            return TRUE;
    }

    /* we should never return this value, but this is harmless */
    return ACP_FALSE;
}

void acpSignalGetDefaultBlockSet(void)
{
    /* No need to implement */
}

ACP_EXPORT acp_rc_t acpSignalBlockDefault(void)
{
    if (acpOSGetVersion() >= ACP_OS_WIN_2000)
    {
        /* 등록한 컨트롤 핸들러들을 해제시킨다. */
        SetConsoleCtrlHandler(acpSignalCtrlEventHandler, FALSE);
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSignalBlockAll(void)
{
    /*
     * [0] 윈도우에서의 시그널
     * 함수를 통해 처리해야 하는 윈도우 시그널에는 fatal error를 제외한
     * 다음과 같은 것들이 있다.
     *    (a) SIGINT
     *    (b) SIGTERM
     *    (c) SIGBREAK
     *
     * [1] 윈도우가 NT버젼 이상일 경우
     * CTRL_C_EVENT, CTRL_BREAK_EVENT 등을 처리하기 위한
     * SetConsoleCtrlHandler()을 사용하여 시그널을 블럭시킬 수가 있다.
     *
     * [2] 윈도우가 9x버젼 일 경우
     * SetConsoleCtrlHandler()가 WIN-2000 버젼 이상에서만 사용가능하므로
     * Handler를 Hooking 하기 위한 에뮬레이션 함수를 통해 구현한다.
     */
    if (acpOSGetVersion() >= ACP_OS_WIN_2000)
    {
        SetConsoleCtrlHandler(acpSignalCtrlEventHandler, TRUE);
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSignalBlock(acp_signal_set_t aSignalSet)
{
    switch (aSignalSet)
    {
        case ACP_SIGNAL_SET_INT:
            if (acpOSGetVersion() >= ACP_OS_WIN_2000)
            {
                SetConsoleCtrlHandler(acpSignalCtrlEventHandler, TRUE);
            }
            else
            {
                /* do nothing */
            }
            break;

        case ACP_SIGNAL_SET_PIPE:
            break;

        default:
            return ACP_RC_EINVAL;
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSignalUnblock(acp_signal_set_t aSignalSet)
{
    switch (aSignalSet)
    {
        case ACP_SIGNAL_SET_INT:
            if (acpOSGetVersion() >= ACP_OS_WIN_2000)
            {
                /* 등록한 컨트롤 핸들러들을 해제시킨다. */
                SetConsoleCtrlHandler(acpSignalCtrlEventHandler, FALSE);
            }
            else
            {
                /* do nothing */
            }
            break;

        case ACP_SIGNAL_SET_PIPE:
            break;

        default:
            return ACP_RC_EINVAL;
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSignalSetExceptionHandler(acp_signal_handler_t *aHandler)
{
    SetUnhandledExceptionFilter(acpSignalExceptionHandler);
    SetConsoleCtrlHandler(acpSignalCtrlEventHandler, TRUE);

    gExceptionHandler = aHandler;

    return ACP_RC_SUCCESS;
}

#else

static void acpSignalExceptionHandler(acp_sint32_t  aSignal,
                                      siginfo_t    *aSigInfo,
                                      void         *aContext)
{
    acp_callstack_t sCallstack;

    ACP_UNUSED(aSigInfo);

    if (gExceptionHandler != NULL)
    {
        acpCallstackInitFromSC(&sCallstack, aContext);

        (*gExceptionHandler)(aSignal, &sCallstack);

        acpCallstackFinal(&sCallstack);
    }
    else
    {
        /* do nothing */
    }
}

void acpSignalGetDefaultBlockSet(void)
{
    (void)pthread_sigmask(SIG_BLOCK, NULL, &gDefaultBlockSet);
}

/**
 * makes signal block mask of current thread to default
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSignalBlockDefault(void)
{
    acp_sint32_t sRet;

    sRet = pthread_sigmask(SIG_SETMASK, &gDefaultBlockSet, NULL);

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
 * blocks all signals for current thread
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSignalBlockAll(void)
{
    sigset_t     sSigSet;
    acp_sint32_t sRet;

    acpMemSet(&sSigSet, 0, sizeof(sigset_t));

    /*
     * set all signals
     */
    sRet = sigfillset(&sSigSet);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    /*
     * delete fatal exception signals
     */
    (void)sigdelset(&sSigSet, SIGILL);
    (void)sigdelset(&sSigSet, SIGABRT);
    (void)sigdelset(&sSigSet, SIGFPE);
    (void)sigdelset(&sSigSet, SIGSEGV);
    (void)sigdelset(&sSigSet, SIGTRAP);
    (void)sigdelset(&sSigSet, SIGBUS);

    sRet = pthread_sigmask(SIG_BLOCK, &sSigSet, NULL);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

/**
 * blocks a signal set for current thread
 * @param aSignalSet signal set to block
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSignalBlock(acp_signal_set_t aSignalSet)
{
    sigset_t     sSigSet;
    acp_sint32_t sRet;

    sRet = sigemptyset(&sSigSet);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    switch (aSignalSet)
    {
        case ACP_SIGNAL_SET_INT:
            sRet = sigaddset(&sSigSet, SIGINT);
            break;

        case ACP_SIGNAL_SET_PIPE:
            sRet = sigaddset(&sSigSet, SIGPIPE);
            break;

        default:
            return ACP_RC_EINVAL;
    }

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRet = pthread_sigmask(SIG_BLOCK, &sSigSet, NULL);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

/**
 * unblocks a signal set for current thread
 * @param aSignalSet signal set to unblock
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSignalUnblock(acp_signal_set_t aSignalSet)
{
    sigset_t     sSigSet;
    acp_sint32_t sRet;

    sRet = sigemptyset(&sSigSet);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    switch (aSignalSet)
    {
        case ACP_SIGNAL_SET_INT:
            sRet = sigaddset(&sSigSet, SIGINT);
            break;

        case ACP_SIGNAL_SET_PIPE:
            sRet = sigaddset(&sSigSet, SIGPIPE);
            break;

        default:
            return ACP_RC_EINVAL;
    }

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRet = pthread_sigmask(SIG_UNBLOCK, &sSigSet, NULL);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

/**
 * sets an handler for exceptional signals
 * @param aHandler handling function for exceptional signals
 * @return result code
 *
 * if @a aHandler is null, it will set default handler
 */
ACP_EXPORT acp_rc_t acpSignalSetExceptionHandler(acp_signal_handler_t *aHandler)
{
    /* 
     * The signals that will be trapped and handled with aHandler
     */
    static acp_sint32_t sSignal[] =
        {
            SIGINT,
            SIGILL,
            SIGABRT,
            SIGFPE,
            SIGSEGV,
            SIGTRAP,
            SIGPIPE,
            SIGBUS,
            0
        };

    struct sigaction sSigAct;
    acp_sint32_t     sRet;
    acp_sint32_t     i;

    sRet = sigemptyset(&sSigAct.sa_mask);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    if (aHandler != NULL)
    {
        sSigAct.sa_sigaction = acpSignalExceptionHandler;
        sSigAct.sa_flags     = SA_RESTART | SA_SIGINFO;
    }
    else
    {
        sSigAct.sa_handler = SIG_DFL;
        sSigAct.sa_flags   = SA_RESTART;
    }

    for (i = 0; sSignal[i] != 0; i++)
    {
        sRet = sigaction(sSignal[i], &sSigAct, NULL);

        if (sRet != 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /* continue */
        }
    }

    gExceptionHandler = aHandler;

    return ACP_RC_SUCCESS;
}

#endif
