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
 
#include <idl.h>
#include <iduProperty.h>
#include <ideFaultMgr.h>
#include <ideErrorMgr.h>
#include <ideLogEntry.h>
#include <ideLog.h>
#include <idtContainer.h>

const static struct 
{
    SInt mSigNum;
    SInt mSigCode;
} gFaultToleratableSignals[] = {
    { SIGBUS,  BUS_ADRALN   },
    { SIGBUS,  BUS_ADRERR   },
    { SIGSEGV, SEGV_MAPERR  },
    { SIGSEGV, SEGV_ACCERR  },
#if defined(ALTI_CFG_OS_LINUX)
    { SIGSEGV, SI_KERNEL    }, /* when accessing virtual memory above limit on Linux */
#endif
    { SIGFPE,  FPE_INTDIV   },
    { SIGFPE,  FPE_INTOVF   },
    { SIGFPE,  FPE_FLTDIV   },
    { SIGFPE,  FPE_FLTOVF   },
    { SIGFPE,  FPE_FLTUND   },
    { SIGFPE,  FPE_FLTRES   },
    { SIGFPE,  FPE_FLTINV   },
    { SIGFPE,  FPE_FLTSUB   },
#if defined(ALTI_CFG_OS_AIX) && defined(ALTI_CFG_CPU_POWERPC)
    { SIGTRAP, SI_UNDEFINED }, /* with -qcheck compile option on AIX/PowerPC */
#endif
    { 0,       0            }
};

SInt gIdeFTTrace = 0;

IDE_RC ideEnableFaultMgr(idBool aIsThreadEnable)
{
    int          sRet;
    ideFaultMgr *sFaultMgr = ideGetFaultMgr();

    IDE_TEST_CONT(sFaultMgr->mIsThreadEnable == aIsThreadEnable, NORMAL_EXIT);

    sRet = pthread_sigmask(0, NULL, &sFaultMgr->mSavedSigMask);
    IDE_TEST(sRet != 0);

    sFaultMgr->mIsThreadEnable = aIsThreadEnable;

    ideClearFTCallStack(0);

    IDE_FT_TRACE("%s", (aIsThreadEnable == ID_TRUE) ?
                       "ENABLED" : "DISABLED");

    IDE_EXCEPTION_CONT(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool ideIsEnabledFaultMgr()
{
    ideFaultMgr *sFaultMgr = ideGetFaultMgr();

    return sFaultMgr->mIsThreadEnable;
}

void ideClearFTCallStack( SInt aRootCnt )
{
    ideFaultMgr *sFaultMgr = ideGetFaultMgr();

    if ( aRootCnt == 0 )
    {
        sFaultMgr->mIsCallStackException  = ID_FALSE;
        sFaultMgr->mIsTransientDisable    = ID_FALSE;
        sFaultMgr->mIsExceptionDisable    = ID_FALSE;
        sFaultMgr->mDisabledEntryFuncName = NULL;
        sFaultMgr->mDisabledCallCount     = 0;
        sFaultMgr->mDummyFlag             = 0;
        sFaultMgr->mCallStackNext     = 0;
        sFaultMgr->mRootBeginDepth    = 0;
    }
    else
    {
        sFaultMgr->mRootBeginDepth   += aRootCnt;

        if ( sFaultMgr->mRootBeginDepth == 1 )
        {
            sFaultMgr->mIsCallStackException  = ID_FALSE;
            sFaultMgr->mIsTransientDisable    = ID_FALSE;
            sFaultMgr->mIsExceptionDisable    = ID_FALSE;
            sFaultMgr->mDisabledEntryFuncName = NULL;
            sFaultMgr->mDisabledCallCount     = 0;
            sFaultMgr->mDummyFlag             = 0;
            sFaultMgr->mCallStackNext         = 0;
        }
        else
        {
            /* mRootBeginDepth가 1보다 큰 경우, 
             * 이미 상위에서 ROOT_BEGIN 을 한 것이므로 초기화 하지 않는다.  */
        }
    }
}

idBool ideCanFaultTolerate(SInt        aSigNum,
                           siginfo_t  *aSigInfo,
                           ucontext_t *aContext)
{
    idBool sIsFaultTolerate = ID_FALSE;
    SInt   i;

    PDL_UNUSED_ARG(aContext);

    if (ideCanFaultTolerate() == ID_TRUE)
    {
        for (i = 0; gFaultToleratableSignals[i].mSigNum != 0; ++i)
        {
            if ((gFaultToleratableSignals[i].mSigNum  == aSigNum) &&
                (gFaultToleratableSignals[i].mSigCode == aSigInfo->si_code))
            {
                sIsFaultTolerate = ID_TRUE;
                break;
            }
        }
    }
    else
    {
        sIsFaultTolerate = ID_FALSE;
    }

    return sIsFaultTolerate;
}

idBool ideCanFaultTolerate()
{
    ideFaultMgr *sFaultMgr = ideGetFaultMgr();
    idBool       sIsFaultTolerate = ID_FALSE;
    
    if ((iduProperty::getFaultToleranceEnable() == ID_TRUE) &&
        (sFaultMgr->mIsThreadEnable == ID_TRUE) &&
        (sFaultMgr->mCallStackNext > 0) &&
        (sFaultMgr->mIsTransientDisable == ID_FALSE) &&
        (sFaultMgr->mIsExceptionDisable == ID_FALSE))
    {
        sIsFaultTolerate = ID_TRUE;
    }
    else
    {
        sIsFaultTolerate = ID_FALSE;
    }

    return sIsFaultTolerate;
}

void ideNonLocalJumpForFaultTolerance()
{
    ideFaultMgr *sFaultMgr     = ideGetFaultMgr();
    UInt         sCallStackTop = sFaultMgr->mCallStackNext - 1;

    IDE_FT_TRACE("longjmp to %s()",
                 sFaultMgr->mCallStack[sCallStackTop].mEntryFuncName);

    /* non-local jump */
    longjmp(sFaultMgr->mCallStack[sCallStackTop].mJmpBuf, 1);
}

void ideLogFT(const SChar *aFunction,
              const SChar *aFile,
              SInt         aLine,
              const SChar *aFormat,
              ...)
{
    ideFaultMgr *sFaultMgr = ideGetFaultMgr();

    if ((gIdeFTTrace != 0) && (sFaultMgr->mIsThreadEnable == ID_TRUE))
    {
        acp_char_t sMsg[IDE_MESSAGE_SIZE + 1] = { '\0', };
        acp_size_t sLen;
        time_t     sTime;
        va_list    sVaList;
        PDL_HANDLE sFD = ideLog::getErrFD();

        sTime = idlOS::time(NULL);

        (void)acpSnprintf(sMsg, IDE_MESSAGE_SIZE,
                          "[0x%016"ID_XINT64_FMT" %"ID_XINT64_FMT"][PID:%"ID_UINT64_FMT"][Thread-%"ID_UINT32_FMT"] FT: ",
                          sTime,
                          ideLogEntry::getLogSerial(),
                          ideLog::getProcID(),
                          idtContainer::getThreadNumber());

        sLen = acpCStrLen(sMsg, IDE_MESSAGE_SIZE);

        va_start(sVaList, aFormat);
        (void)acpVsnprintf(sMsg + sLen, IDE_MESSAGE_SIZE - sLen, aFormat, sVaList);
        va_end(sVaList);

        sLen += acpCStrLen(sMsg + sLen, IDE_MESSAGE_SIZE - sLen);

        (void)acpSnprintf(sMsg + sLen, IDE_MESSAGE_SIZE - sLen,
                          " in %s() (%s:%"ID_INT32_FMT")",
                          aFunction, idlVA::basename(aFile), aLine);

        sLen += acpCStrLen(sMsg + sLen, IDE_MESSAGE_SIZE - sLen);

        sMsg[sLen++] = '\n';
        
        idlOS::write(sFD, sMsg, sLen);
    }
    else
    {
        /* nothing to do */
    }
}

void ideLogFTMacro(const SChar *aFunction,
                   const SChar *aFile,
                   SInt         aLine,
                   SInt         aCallLevel,
                   const SChar *aFTMacro)
{
    ideLogFT(aFunction,
             aFile, aLine,
             "Lv-%-4"ID_INT32_FMT" %-20s", aCallLevel, aFTMacro);
}

