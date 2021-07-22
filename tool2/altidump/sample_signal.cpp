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
 
#include <idl.h>
#include <ideErrorMgr.h>
#include <ideMsgLog.h>
#include <iduStack.h>
#include <sample.h>

extern ideMsgLog mMsgLogForce;

sigset_t         mSigSet;


// 비정상적인 종료시 시그널 핸들러 수행
#if defined(IBM_AIX)
void problem_signal_handler(SInt signum, SInt dummy, struct sigcontext *SCP)
#elif defined(SPARC_SOLARIS)
void problem_signal_handler(SInt signum, siginfo_t *sig, struct ucontext *SCP)
#elif defined(INTEL_LINUX) || defined(POWERPC_LINUX) || defined(AMD64_LINUX)
void problem_signal_handler(SInt signum, struct siginfo *aInfo, struct ucontext *SCP)
#else
void problem_signal_handler(SInt signum)
#endif    
{
#if defined(IBM_AIX)

    vULong  sCoreAddr;
    vULong *sFrame;
    sCoreAddr = (vULong)SCP->sc_jmpbuf.jmp_context.iar;/* core position */
    sFrame = (vULong *)SCP->sc_jmpbuf.jmp_context.gpr[1];    /* stack */
    iduStack::dumpStack( mMsgLogForce, sFrame, ID_TRUE, sCoreAddr );
#elif defined(SPARC_SOLARIS)
    vULong  sCoreAddr;
    vULong *sFrame;
    sCoreAddr = (vULong )SCP->uc_mcontext.gregs[REG_nPC]; /* CORE position pc */
    sFrame = (vULong *)SCP->uc_mcontext.gregs[REG_SP]; /* stack */
    iduStack::dumpStack( mMsgLogForce, sFrame, ID_TRUE, sCoreAddr);
#elif defined(INTEL_LINUX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(ALPHA_LINUX) || defined(POWERPC_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX)
    vULong  sCoreAddr;
    vULong *sFrame;
    sCoreAddr = (vULong )SCP->uc_mcontext.gregs[14]; // caller address
    sFrame = (vULong *)SCP->uc_mcontext.gregs[6];  // STACK EBP PTR
    iduStack::dumpStack( mMsgLogForce, sFrame, ID_TRUE, sCoreAddr );
#else
    iduStack::dumpStack( mMsgLogForce );    
#endif
    idlOS::abort();
#undef IDE_FN
}

IDE_RC setupDefaultAltibaseSignal()
{

#define IDE_FN "IDE_RC mmiMgr::setupAltibaseSignal()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    static struct sigaction problem_act, altibase_exit_act, null_thread_act, oact;
    
    idlOS::sigemptyset(&problem_act.sa_mask);
    problem_act.sa_handler = (PDL_SignalHandler)problem_signal_handler;
    problem_act.sa_flags   = SA_RESTART | SA_SIGINFO;

    IDE_TEST_RAISE(idlOS::sigaction(SIGBUS, &problem_act, &oact) < 0,
                   sigaction_error);
    IDE_TEST_RAISE(idlOS::sigaction(SIGFPE, &problem_act, &oact) < 0,
                   sigaction_error);
    IDE_TEST_RAISE(idlOS::sigaction(SIGILL, &problem_act, &oact) < 0,
                   sigaction_error);
    IDE_TEST_RAISE(idlOS::sigaction(SIGSEGV, &problem_act, &oact) < 0,
                   sigaction_error);
    IDE_TEST_RAISE(idlOS::sigaction(SIGTRAP, &problem_act, &oact) < 0,
                   sigaction_error);
    

    /* ----------------------
     * [2] idlOS::sigwait()을 위한 시그널 셋팅
     * ----------------------*/
    IDE_TEST_RAISE(idlOS::sigemptyset(&mSigSet) != 0, sigemptyset_error);
#ifdef DEC_TRU64
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGHUP) != 0, sigaddset_error);
    //IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGINT) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGQUIT) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGABRT) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGEMT) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGSYS) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGPIPE) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGALRM) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGTERM) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGURG) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGTSTP) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGCONT) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGCHLD) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGTTIN) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGTTOU) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGPOLL) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGXCPU) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGXFSZ) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGVTALRM) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGPROF) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGWINCH) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGINFO) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGUSR1) != 0, sigaddset_error);
    IDE_TEST_RAISE(idlOS::sigaddset(&mSigSet, SIGRESV) != 0, sigaddset_error);
#else    
    IDE_TEST_RAISE(idlOS::sigfillset(&mSigSet)  != 0, sigfillset_error);
#endif

/* BUGBUG - PR-998 workaround for LINUX-THREAD */
#if defined(INTEL_LINUX) || defined(ALPHA_LINUX) || defined(POWERPC_LINUX) || defined(AMD64_LINUX)
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGTERM) != 0, sigdelset_error);
#endif
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGBUS) != 0, sigdelset_error);
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGFPE) != 0, sigdelset_error);
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGILL) != 0, sigdelset_error);
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGSEGV) != 0, sigdelset_error);
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGTRAP) != 0, sigdelset_error);

    // [1] 인위적인 인터럽트 생성을 위함.
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGUSR2) != 0, sigdelset_error);

    // [2] 인위적 코어 생성 : debug => ^\ : release => kill -SIGQUIT pid
    IDE_TEST_RAISE(idlOS::sigdelset(&mSigSet, SIGQUIT) != 0, sigdelset_error);
    
    /* 
     * 문맥상으로는 SIG_SETMASK가 맞으나, 현재까지 SIG_BLOCK으로
     * 테스트가 되었기 때문에 이것으로 유지.
     * 단, AIX의 경우 SIG_BLOCK으로 동작하지 않기 때문에 SIG_SETMASK로
     * 놓고 사용한다. 
     * 최종적으로, 모든 플랫폼에서 SIG_SETMASK가 문제가 없다고 테스트
     * 되었을 때, SIG_BLOCK을 제거하여야 할 것임. 2000/3/30 by gamestar
     */
#if defined(IBM_AIX)
    IDE_TEST_RAISE(idlOS::sigprocmask(SIG_SETMASK, &mSigSet, NULL) != 0,
                   sigprocmask_error);
#else
    IDE_TEST_RAISE(idlOS::sigprocmask(SIG_BLOCK, &mSigSet, NULL) != 0,
                   sigprocmask_error);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(sigemptyset_error);
    IDE_EXCEPTION(sigfillset_error);
    IDE_EXCEPTION(sigdelset_error);
#ifdef DEC_TRU64
    IDE_EXCEPTION(sigaddset_error);
#endif /* DEC_TRU64 */
    IDE_EXCEPTION(sigprocmask_error);
    IDE_EXCEPTION(sigaction_error);
    IDE_EXCEPTION_END;
    {
    }
    return IDE_FAILURE;


#undef IDE_FN
}




IDE_RC setupDefaultThreadSignal()
{
    sigset_t newset, oldset;
    SInt rc;

    idlOS::sigemptyset(&newset);
    idlOS::sigaddset(&newset, SIGINT);

    rc = idlOS::pthread_sigmask(SIG_BLOCK, &newset, &oldset);
    IDE_TEST_RAISE(rc != 0, sigmask_error);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(sigmask_error);
    {
        printf("ERR \n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
