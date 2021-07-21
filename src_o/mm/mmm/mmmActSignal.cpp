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
#include <iduLimitManager.h>
#include <idtContainer.h>
#include <iduStack.h>
#include <ideFaultMgr.h> /* PROJ-2617 */
#include <mmm.h>
#include <mmi.h>
#include <mmErrorCode.h>
#include <mmtThreadManager.h>

IDL_EXTERN_C void mmmSignalHandler(int, siginfo_t*, void*);
IDL_EXTERN_C void mmmExitHandler(int, siginfo_t*, void*);
IDL_EXTERN_C void mmmChildHandler(int, siginfo_t*, void*);
IDL_EXTERN_C void mmmDumpCallstack(int,  siginfo_t*,void* );

#if defined(SA_ONSTACK)
# define SIGPIPEFLAG  0
# define SIGCHILDFLAG SA_RESTART | SA_SIGINFO | SA_ONSTACK
/* PROJ-2617 */
# define SIGFLAG_FT   SA_RESTART | SA_SIGINFO | SA_ONSTACK
# define SIGOTHERFLAG SA_RESTART | SA_SIGINFO | SA_RESETHAND | SA_ONSTACK

/*it's just for dumpping callstacks(BUG-45182)*/
# define SIGFLAG4RT   SA_RESTART | SA_SIGINFO | SA_ONSTACK
#else
# define SIGPIPEFLAG  0
# define SIGCHILDFLAG SA_RESTART | SA_SIGINFO
/* PROJ-2617 */
# define SIGFLAG_FT   SA_RESTART | SA_SIGINFO
# define SIGOTHERFLAG SA_RESTART | SA_SIGINFO | SA_RESETHAND

/*it's just for dumpping callstacks(BUG-45182)*/
# define SIGFLAG4RT   SA_RESTART | SA_SIGINFO 
#endif

const static iduSignalDef gSignals[] =
{
    {
        SIGHUP,   "SIGHUP",   "Hangup",
        SIGOTHERFLAG,
        (iduHandler*)mmmSignalHandler
    },
    {
        SIGINT,   "SIGINT",   "Interrupt(^C)",
        SIGOTHERFLAG,
        (iduHandler*)mmmExitHandler
    },
    {
        SIGQUIT,  "SIGQUIT",  "Quit",
        SIGOTHERFLAG,
        (iduHandler*)mmmSignalHandler
    },
    {
        SIGILL,   "SIGILL",   "Illegal instruction",
        SIGFLAG_FT,
        (iduHandler*)mmmSignalHandler
    },
    {
        SIGTRAP,  "SIGTRAP",  "Trace trap",
        SIGOTHERFLAG,
        (iduHandler*)mmmSignalHandler
    },
    {
        SIGABRT,  "SIGABRT",  "Abort",
        SIGOTHERFLAG,
        (iduHandler*)mmmSignalHandler
    },
    {
        SIGIOT,   "SIGIOT",   "IOT trap",
        SIGOTHERFLAG,
        (iduHandler*)mmmSignalHandler
    },
    {
        SIGBUS,   "SIGBUS",   "BUS error",
        SIGFLAG_FT,
        (iduHandler*)mmmSignalHandler
    },
    {
        SIGFPE,   "SIGFPE",   "Floating-point exception",
        SIGFLAG_FT,
        (iduHandler*)mmmSignalHandler
    },
    /* Not catchable in signal handler
    { SIGKILL,  "SIGKILL",  "Kill, unblockable",            (iduHandler*)mmmSignalHandler    },
    */
    /* Not used 
    { SIGUSR1,  "SIGUSR1",  "User-defined signal 1",        (iduHandler*)mmmSignalHandler    },
    */
    {
        SIGSEGV,  "SIGSEGV",  "Segmentation violation",
        SIGFLAG_FT,
        (iduHandler*)mmmSignalHandler
    },
    /* Not used 
    { SIGUSR2,  "SIGUSR2",  "User-defined signal 2",        (iduHandler*)mmmSignalHandler    },
    */
    /* Pipe handle */
    {
        SIGPIPE,  "SIGPIPE",  "Broken pipe",
        SIGPIPEFLAG,
        (iduHandler*)SIG_IGN
    },
    /* Ignore
    { SIGALRM,  "SIGALRM",  "Alarm clock",                  (iduHandler*)mmmSignalHandler    },
    */
    {
        SIGTERM,  "SIGTERM",  "Termination",
        SIGOTHERFLAG,
        (iduHandler*)mmmSignalHandler
    },
    /*
     * BUG-42824
     * child signal handler should not be reset
     */
    {
        SIGCHLD,  "SIGCHLD",  "Child status has changed",
        SIGCHILDFLAG,
        (iduHandler*)mmmChildHandler
    },
    {
        SIGCLD,   "SIGCLD",   "Same as SIGCHLD",
        SIGCHILDFLAG,
        (iduHandler*)mmmChildHandler
    },
    {   SIGRTMIN,  "SIGRTMIN",  "Real Time Signal",  /*BUG-45182*/ 
        SIGFLAG4RT,
        (iduHandler*)mmmDumpCallstack    
    },
    /* Do not handle 
    { SIGCONT,  "SIGCONT",  "Continue",                     (iduHandler*)mmmSignalHandler    },
    */
    /* Not catchable in signal handler
    { SIGSTOP,  "SIGSTOP",  "Stop, unblockable",            (iduHandler*)mmmSignalHandler    },
    */
    /* Do not handle following signals
    { SIGTSTP,  "SIGTSTP",  "Keyboard stop",                (iduHandler*)mmmSignalHandler    },
    { SIGTTIN,  "SIGTTIN",  "Background read from tty",     (iduHandler*)mmmSignalHandler    },
    { SIGTTOU,  "SIGTTOU",  "Background write to tty",      (iduHandler*)mmmSignalHandler    },
    { SIGURG,   "SIGURG",   "Urgent condition on socket",   (iduHandler*)mmmSignalHandler    },
    { SIGXCPU,  "SIGXCPU",  "CPU limit exceeded",           (iduHandler*)mmmSignalHandler    },
    { SIGXFSZ,  "SIGXFSZ",  "File size limit exceeded",     (iduHandler*)mmmSignalHandler    },
    { SIGVTALRM,"SIGVTALRM","Virtual alarm clock",          (iduHandler*)mmmSignalHandler    },
    { SIGPROF,  "SIGPROF",  "Profiling alarm clock",        (iduHandler*)mmmSignalHandler    },
    { SIGWINCH, "SIGWINCH", "Window size change",           (iduHandler*)mmmSignalHandler    },
    { SIGIO,    "SIGIO",    "I/O now possible",             (iduHandler*)mmmSignalHandler    },
    { SIGPOLL,  "SIGPOLL",  "Pollable event occurred",      (iduHandler*)mmmSignalHandler    },
    { SIGPWR,   "SIGPWR",   "Power failure restart",        (iduHandler*)mmmSignalHandler    },
    { SIGSYS,   "SIGSYS",   "Bad system call",              (iduHandler*)mmmSignalHandler    },
    */
    {
        -1,       "UNKNOWN",  "Unknown Signal",
        0,
        NULL
    }
};

/* BUG-48201 */
#ifdef ALTI_CFG_OS_AIX
static idBool mmmIsUnset_SA_RESETHAND(UInt i)              
{
    idBool sVersion;
    idBool sSigNum;
    SInt   sRet;
    struct utsname  sUts={0,};

    sSigNum  = (idBool)((gSignals[i].mNo ==  SIGABRT) || (gSignals[i].mNo ==  SIGIOT));
    sRet     = idlOS::uname(&sUts);

    if( sRet >= 0 ) //success
    {
	//only AIX 7.2 just for now. 
    	sVersion = (idBool)((idlOS::strcmp(sUts.version,"7") == 0) && 
                            (idlOS::strcmp(sUts.release,"2") == 0));
    }	
    else
    {	
        sVersion = ID_FALSE;
    }

    return (idBool)((sSigNum == ID_TRUE) && (sVersion == ID_TRUE));	
}
#endif

#if defined(DEBUG)
static idBool gCoreDumpOnSignal = ID_FALSE;
#endif

/* PROJ-2617 */
static idBool mmmCanFaultTolerate(SInt        aSigNum,
                                  siginfo_t  *aSigInfo,
                                  ucontext_t *aContext);

/* BUG-45182 */
IDL_EXTERN_C void mmmDumpCallstack(int          /*aSigNum*/,
                                   siginfo_t*   /*aSigInfo*/,
                                   void*        /*aContext*/)
{
    iduStack::dumpStack( NULL,     /*default*/
                         NULL,     /*default*/
                         NULL,     /*default*/
                         ID_FALSE, /*default*/
                         NULL,     /*default*/
                         ID_TRUE );/*for dumping callstacks*/


}

/* ------------------------------------------------------------
 *   altibase Signal handler
 * ---------------------------------------------------------- */
IDL_EXTERN_C void mmmSignalHandler(int          aSigNum,
                                   siginfo_t*   aSigInfo,
                                   void*        aContext)
{
    SInt                i;
    const iduSignalDef* sSignal = NULL;
    ucontext_t*         sContext = (ucontext_t*)aContext;
    idBool              sIsFaultTolerate; /* PROJ-2617 */
    idtBaseThread      *sMyThread      = NULL;
    mmtServiceThread   *sServiceThread = NULL;
    mmcStatement       *sStatement     = NULL;
    SChar              *sSqlString     = NULL;


    for (i = 0; gSignals[i].mNo != -1; i++)
    {
        if (gSignals[i].mNo == aSigNum)
        {
            sSignal = &(gSignals[i]);
            break;
        }
        else
        {
            /* continue */
        }
    }

    /* PROJ-2617 */
    sIsFaultTolerate = mmmCanFaultTolerate(aSigNum, aSigInfo, sContext);

    sMyThread = (idtBaseThread*)idtContainer::getBaseThread();

    if( sMyThread != NULL ) 
    {

        if( sMyThread->isServiceThread() == ID_TRUE ) 
        {
            /* get SQL String*/
            sServiceThread = (mmtServiceThread*)sMyThread ; 
            sStatement = sServiceThread->getStatement();

            if ( sStatement != NULL )
            {
                sSqlString = sStatement->getQueryString();
            }
            else
            {
                /* Do nothing */
            }
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        /* Do nothing */
    }

    iduStack::dumpStack( sSignal, aSigInfo, sContext, sIsFaultTolerate,
                         sSqlString );


    if (sIsFaultTolerate == ID_TRUE)
    {
        ideNonLocalJumpForFaultTolerance();
    }
    else
    {
#if defined(DEBUG)
        if (gCoreDumpOnSignal == ID_TRUE)
        {
            /*[BUG-45987]
              to avoid leaving double callstacks on trc log.*/
            signal(SIGABRT,SIG_DFL); 
            idlOS::abort();
        }
        else
        {
            idlOS::exit(-1);
        }
#else
        idlOS::exit(-1);
#endif
    }
}

/* PROJ-2617 */
static idBool mmmCanFaultTolerate(SInt        aSigNum,
                                  siginfo_t  *aSigInfo,
                                  ucontext_t *aContext)
{
    idBool sIsFaultTolerate = ID_FALSE;

    if (mmm::getCurrentPhase() == MMM_STARTUP_SERVICE)
    {
        sIsFaultTolerate = ideCanFaultTolerate(aSigNum, aSigInfo, aContext);
    }
    else
    {
        /* nothing to do */
    }

    return sIsFaultTolerate;
}

/* ------------------------------------------------------------
 *   altibase Signal handler ผ๖วเ
 * ---------------------------------------------------------- */
// called by SIGINT In Debug Mode
IDL_EXTERN_C void mmmExitHandler(SInt aSignal, siginfo_t*, void*)
{
    mmm::setServerStatus(ALTIBASE_STATUS_SHUTDOWN_NORMAL);

#ifdef DEBUG
    idlOS::fprintf(stdout, "TID=[%"ID_UINT32_FMT"] signal %"ID_UINT32_FMT" occurred \n",
                   (UInt)idtContainer::getThreadNumber(), aSignal);
    idlOS::fflush(stdout);
#else
    PDL_UNUSED_ARG(aSignal);
#endif
    mmm::prepareShutdown(ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE);
}

/* SIGCHLD signal handler   */
IDL_EXTERN_C void mmmChildHandler(SInt, siginfo_t*, void*)
{
    PDL_exitcode status;

    idlOS::memset( &status, 0x00, ID_SIZEOF( PDL_exitcode ) );
    PDL_OS::waitpid(-1, &status, WNOHANG);
    
    /* End of handler */
}

static IDE_RC mmmPhaseActionSignal(mmmPhase         /*aPhase*/,
                                   UInt             /*aOptionflag*/,
                                   mmmPhaseAction * /*aAction*/)
{
    SInt                i;
    struct sigaction    sAction;
    typedef void sHandler(int, siginfo_t*, void*);

#if defined(DEBUG)
    SInt    sCoreDump;
#endif

    IDE_TEST(iduStack::initializeStatic() != IDE_SUCCESS);

    // if server status is DA : ignore for init signal handling
    //fix PROJ-1749
    if ((mmi::getServerOption() & MMI_SIGNAL_MASK) == MMI_SIGNAL_TRUE)
    {
        /* ----------------------
         * Abnormal termination
         * ----------------------*/
        for (i = 0; gSignals[i].mNo != -1; i++)
        {
            (void) idlOS::sigemptyset(&sAction.sa_mask);
            sAction.sa_sigaction = (sHandler*)gSignals[i].mFunc;
            sAction.sa_sigaction = (sHandler*)gSignals[i].mFunc;
            sAction.sa_flags     =            gSignals[i].mFlags;
#ifdef ALTI_CFG_OS_AIX
   	    /* 
             * BUG-48201 
             *   if (SIGABRT or SIGIOT ) && AIX 7.2 , unset.
             *   Otherwise,let it be.
             */
	    if( mmmIsUnset_SA_RESETHAND(i) == ID_TRUE )              
	    {	
            	sAction.sa_flags &= ~SA_RESETHAND; 
		// ideLog::log(IDE_SERVER_0,"[DEBUG] unset SA_RESETHAND"); //just for test
            }
#endif
            IDE_TEST_RAISE(idlOS::sigaction(gSignals[i].mNo, &sAction, NULL) < 0,
                           sigaction_error);
        }

    }

#if defined(DEBUG)
    IDE_TEST(idp::read("__CORE_DUMP_ON_SIGNAL", &sCoreDump) != IDE_SUCCESS);
    gCoreDumpOnSignal = (sCoreDump == 0)? ID_FALSE:ID_TRUE;
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(sigaction_error);
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#if defined(WRS_VXWORKS) 
IDE_RC  applySignalHandler() 
{ 
    return mmmPhaseActionSignal( MMM_STARTUP_SERVICE, 0, NULL ); 
} 
#endif 

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActSignal =
{
    (SChar *)"Initialize Process Signal System",
    0,
    mmmPhaseActionSignal
};

