/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idl.i 81437 2017-10-26 04:32:16Z minku.kang $
 * $Log$
 * Revision 1.42  2006/05/25 02:06:52  shsuh
 * BUG-14490
 *
 * Revision 1.41  2006/05/17 00:12:11  newdaily
 * fix compile warning
 *
 * Revision 1.40  2006/05/16 00:23:36  sjkim
 * TASK-1786
 *
 * Revision 1.39.2.2  2006/04/17 03:34:46  sjkim
 * TASK-1786
 *
 * Revision 1.39.2.1  2006/04/10 06:28:32  sjkim
 * TASK-1786
 *
 * Revision 1.39  2006/03/28 00:15:26  sjkim
 * TASK-1733
 *
 * Revision 1.38.6.1  2006/03/09 03:09:28  sjkim
 * TASK-1733
 *
 * Revision 1.38  2005/12/13 04:30:52  hjohn
 * for fix windows compile
 *
 * Revision 1.37  2005/12/09 08:20:29  jhseong
 * fix MATH function compile problem
 *
 * Revision 1.36  2005/12/09 08:15:08  jhseong
 * fix MATH function compile problem
 *
 * Revision 1.35  2005/12/09 01:37:48  jhseong
 * fix BUG-13505
 *
 * Revision 1.34  2005/12/07 01:27:11  mycomman
 * TASK-1501
 *
 * Revision 1.33  2005/09/06 01:57:36  hjohn
 * fix WinCE merge for PROJ-1347
 *
 * Revision 1.32  2005/08/31 05:44:12  jdlee
 * fix for PROJ-1347
 *
 * Revision 1.31  2005/08/26 01:13:24  shsuh
 * BUG-12841
 *
 * Revision 1.30  2005/08/25 02:27:38  shsuh
 * BUG-12629
 *
 * Revision 1.29  2005/07/21 00:54:20  shsuh
 * BUG-12098
 *
 * Revision 1.28  2005/06/10 04:12:45  shsuh
 * PROJ-1417
 *
 * Revision 1.27  2005/06/10 00:52:57  shsuh
 * PROJ-1385
 *
 * Revision 1.26.2.2  2005/04/27 00:42:58  newdaily
 * fix PRJ-1347
 *
 * Revision 1.26.2.1  2005/04/25 01:06:32  newdaily
 * fix PRJ-1347
 *
 * Revision 1.26  2005/04/20 05:21:43  pu7ha
 * winCE porting prj1347
 *
 * Revision 1.25  2005/04/18 06:32:44  newdaily
 * PRJ-1347
 *
 * Revision 1.23.18.2  2005/02/03 03:44:40  newdaily
 * fix PRJ-1347
 *
 * Revision 1.23.18.1  2005/02/03 02:41:38  newdaily
 * PRJ-1347
 *
 * Revision 1.24  2005/04/13 02:45:01  mycomman
 * AMD64 Linux porting
 *
 * Revision 1.23  2004/05/28 12:57:19  lons
 * *** empty log message ***
 *
 * Revision 1.22  2004/05/28 11:25:26  lons
 * *** empty log message ***
 *
 * Revision 1.21  2004/05/10 02:13:27  sbjang
 * fix for BUG-6242
 *
 * Revision 1.20  2004/04/20 08:34:10  hjohn
 * (WINCE) to undeclare popen/pclose/strerror for Windows CE
 *
 * Revision 1.19  2004/04/06 00:42:37  bethy
 * CASE-1722
 *
 * Revision 1.18  2004/02/25 01:18:57  sbjang
 * ITRON pd layer porting
 *
 * Revision 1.17  2004/01/29 08:12:41  newdaily
 * Fix compile error
 *
 * Revision 1.16  2004/01/29 06:51:37  assam
 * altiStatistics commit.
 *
 * Revision 1.15  2004/01/02 05:56:25  jdlee
 * fix compile error on linux
 *
 * Revision 1.14  2003/12/31 04:52:47  newdaily
 * fix PR-5769
 *
 * Revision 1.13  2003/12/29 10:43:03  hjohn
 * for VxWorks
 *
 * Revision 1.12  2003/12/10 08:20:49  newdaily
 * fix PR-5633
 *
 * Revision 1.11  2003/12/10 08:02:53  newdaily
 * fix PR-5633
 *
 * Revision 1.9  2003/11/25 09:15:09  kbjung
 * bug-5505 upper lib remove
 *
 * Revision 1.8  2003/08/14 00:34:19  sbjang
 * To fix BUG-5075
 *
 * Revision 1.7  2003/08/08 02:40:51  hjohn
 * Win32 Porting : for cygwin : PC_CYGWIN ==> CYGWIN32
 *
 * Revision 1.6  2003/06/20 04:34:42  bethy
 * BUG-4587 : splitting client/server library package
 *
 * Revision 1.5  2003/06/12 05:40:22  hjohn
 * A3 CYGWIN32 porting
 *
 * Revision 1.4  2003/06/04 10:03:32  assam
 * mms idm changed
 *
 * Revision 1.3  2003/06/04 05:02:53  assam
 * mms idm changed
 *
 * Revision 1.2  2003/03/07 03:45:44  newdaily
 * fix
 *
 * Revision 1.1  2002/12/17 05:43:24  jdlee
 * create
 *
 * Revision 1.1.1.1  2002/10/18 13:55:36  jdlee
 * create
 *
 * Revision 1.57  2002/09/13 07:22:37  sjkim
 * refix for PR-2974 : errno를 0으로 초기화 함
 *
 * Revision 1.56  2002/09/13 07:09:14  sjkim
 * fix for PR-2974
 *
 * Revision 1.55  2002/08/20 05:01:39  hjohn
 * win32 IPC support
 *
 * Revision 1.54  2002/07/30 02:18:10  sjkim
 * dbadmin status memory bug fix & print SCN in session information
 *
 * Revision 1.53  2002/06/17 00:44:00  sjkim
 * fix for problem-handler message print detaily
 *
 * Revision 1.52  2002/04/25 05:37:29  assam
 * *** empty log message ***
 *
 * Revision 1.51  2002/01/28 06:00:56  assam
 * *** empty log message ***
 *
 * Revision 1.50  2002/01/22 06:36:07  leekmo
 * *** empty log message ***
 *
 * Revision 1.49  2002/01/22 05:26:38  assam
 * *** empty log message ***
 *
 * Revision 1.48  2001/12/20 10:59:47  newdaily
 * DEC socket read problem fix
 *
 * Revision 1.47  2001/12/18 04:21:46  assam
 * *** empty log message ***
 *
 * Revision 1.46  2001/12/13 07:00:52  assam
 * *** empty log message ***
 *
 * Revision 1.45  2001/12/12 10:32:14  mylee
 * add strmatch
 *
 * Revision 1.44  2001/12/03 06:49:09  assam
 * remove warning
 *
 * Revision 1.43  2001/11/15 06:51:26  jdlee
 * nomenclature
 *
 * Revision 1.42  2001/11/09 08:04:05  jdlee
 * nto_qnx porting
 *
 * Revision 1.41  2001/11/09 07:50:17  jdlee
 * nto_qnx porting
 *
 * Revision 1.40  2001/10/17 06:25:17  bethy
 * signal 발생시 retry하도록..
 *
 * Revision 1.39  2001/08/27 10:18:36  jdlee
 * altibase 2.0 aix porting
 *
 * Revision 1.38  2001/08/21 15:05:46  sjkim
 * align vULong & vSLong added
 *
 * Revision 1.37  2001/08/21 06:26:26  assam
 * PR/1451, PR/1450
 *
 * Revision 1.36  2001/08/09 07:30:08  kmkim
 * fix
 *
 * Revision 1.35  2001/08/09 02:05:55  assam
 * swapTable 추가
 *
 * Revision 1.34  2001/08/07 02:58:17  leekmo
 * fix PR-1374 PR-1375
 *
 * Revision 1.33  2001/07/12 01:01:16  jdlee
 * porting to solaris 2.5
 *
 * Revision 1.32  2001/06/26 10:56:42  jdlee
 * fix PR-1059
 *
 * Revision 1.31  2001/06/11 10:16:45  jdlee
 * fix
 *
 * Revision 1.30  2001/06/11 08:31:58  jdlee
 * nomenclature
 *
 * Revision 1.29  2001/06/07 06:58:04  kskim
 * Fix: alpha linux
 *
 * Revision 1.28  2001/05/30 02:19:10  kskim
 * Porting: alpha linux
 *
 * Revision 1.27  2001/05/11 08:43:42  kskim
 * Add directio()
 *
 * Revision 1.26  2001/04/28 07:10:12  jdlee
 * fix
 *
 * Revision 1.25  2001/04/18 07:54:35  sjkim
 * ID_LONG fix
 *
 * Revision 1.24  2001/03/28 03:38:27  sjkim
 * fileno fix
 *
 * Revision 1.23  2001/03/28 02:06:38  sjkim
 * stackdump fix
 *
 * Revision 1.22  2001/03/06 08:45:46  jdlee
 * porting to aix
 *
 * Revision 1.21  2001/03/02 07:56:11  kmkim
 * 1st version of WIN32 porting
 *
 * Revision 1.20  2001/02/22 11:33:24  jdlee
 * fix
 *
 * Revision 1.19.2.3  2001/03/02 05:20:43  kmkim
 * THE PORT TO WIN32
 *
 * Revision 1.19.2.2  2001/02/28 02:41:32  kmkim
 * no message
 *
 * Revision 1.19.2.1  2001/02/21 09:00:05  kmkim
 * win32 porting - 컴파일만 성공
 *
 * Revision 1.19  2001/02/21 05:11:52  jdlee
 * fix linux porting
 *
 * Revision 1.18  2000/12/14 08:15:18  sjkim
 * dec fix
 *
 * Revision 1.17  2000/12/08 12:27:10  jdlee
 * fix
 *
 * Revision 1.16  2000/12/08 06:11:33  sjkim
 * msglog fix
 *
 * Revision 1.15  2000/12/05 06:30:26  sjkim
 * toupper for hp fix after kernel library patch.
 *
 * Revision 1.14  2000/11/24 05:13:24  sjkim
 * fix
 *
 * Revision 1.13  2000/11/24 04:24:22  sjkim
 * ntoh(), hton() added
 *
 * Revision 1.12  2000/10/16 08:48:05  mskim
 * include termios.h for TRU64
 *
 * Revision 1.11  2000/09/25 08:51:21  mskim
 * *** empty log message ***
 *
 * Revision 1.10  2000/09/15 01:45:19  sjkim
 * sm naming fix
 *
 * Revision 1.9	 2000/09/06 02:20:03  sjkim
 * fix
 *
 * Revision 1.8	 2000/09/04 09:27:08  sjkim
 * bound thread fix
 *
 * Revision 1.7	 2000/08/16 09:12:05  sjkim
 * fix hp
 *
 * Revision 1.6	 2000/08/02 12:27:06  sjkim
 * tsp function fix
 *
 * Revision 1.5	 2000/07/25 06:10:14  kskim
 * add pthread_cond_init
 *
 * Revision 1.4	 2000/07/25 03:42:14  jdlee
 * fix
 *
 * Revision 1.3	 2000/07/13 08:25:29  sjkim
 * error code fatal->abort
 *
 * Revision 1.2	 2000/06/21 07:36:00  sjkim
 * fix dbadmin, altibase
 *
 * Revision 1.1.1.1  2000/06/07 11:44:06  jdlee
 * init
 *
 * Revision 1.13  2000/05/02 01:01:53  sjkim
 * FIX for 64비트 컴파일
 *
 * Revision 1.12  2000/04/18 11:03:43  sjkim
 *  network optimization
 *
 * Revision 1.11  2000/04/12 06:52:41  sjkim
 * chmod() 추가, TID 처리 변경
 *
 * Revision 1.10  2000/03/22 08:45:55  assam
 * *** empty log message ***
 *
 * Revision 1.9	 2000/03/16 06:52:06  sjkim
 * write line 추가
 *
 * Revision 1.8	 2000/03/16 06:29:06  sjkim
 * timed readline 추가
 *
 * Revision 1.7	 2000/02/29 01:57:04  sjkim
 * specific 수정
 *
 * Revision 1.6	 2000/02/29 00:52:02  sjkim
 *  라이브러리 수정
 *
 * Revision 1.5	 2000/02/22 05:57:00  assam
 * *** empty log message ***
 *
 * Revision 1.4	 2000/02/11 05:19:29  sjkim
 * idlVA inline 추가
 *
 * Revision 1.3	 2000/02/11 05:10:32  sjkim
 * idlVA 추가
 *
 * Revision 1.2	 2000/02/09 05:29:27  sjkim
 * assert 고려중
 *
 * Revision 1.1	 2000/01/28 06:55:32  sjkim
 * idl : inline functions
 *
 * Revision 1.2	 2000/01/19 07:32:33  jdlee
 * remove ISP
 *
 * Revision 1.1	 2000/01/19 07:31:14  jdlee
 * ispIDL to idl
 *
 * Revision 1.1	 2000/01/18 07:43:07  sjkim
 * IDL 인라인 함수 구현 화일
 *
 *
 **********************************************************************/

/***********************************************************************
 *
 * NAME
 *   idl.i
 *
 * DESCRIPTION
 *   InDependent Layer inline functions
 *   idlOS:: 혹은 IDL:: 에서 구현될 인라인 함수들의 body가 포함됨.
 *
 * MODIFIED    (MM/DD/YYYY)
 *    sjkim	01/18/2000 - Created
 *
 **********************************************************************/
#ifndef O_IDL_I
#define O_IDL_I

#ifndef O_IDL_H
#include <idl.h>
#endif

#ifdef DEC_TRU64
#include <ctype.h>
#endif

inline SInt
idlOS::getMaxOpen()
{
#if defined(VC_WIN32) || defined(WRS_VXWORKS)
	return idlVA::max_handles();
#elif defined(ITRON)
    /* empty */
    return -1;
#else
    struct rlimit limit;
    if (idlOS::getrlimit(RLIMIT_NOFILE, &limit) < 0)
    {
	return -1;
    }
    return limit.rlim_cur;
#endif
}

inline SInt
idlOS::setMaxOpen(SInt MaxOpen)
{
#if defined(VC_WIN32) || defined(WRS_VXWORKS)
	return idlVA::set_handle_limit(MaxOpen);
#elif defined(ITRON)
    return -1;
#else
    struct rlimit limit;
    if (idlOS::getrlimit(RLIMIT_NOFILE, &limit) < 0)
    {
	return -1;
    }

    limit.rlim_cur = MaxOpen;
    return idlOS::setrlimit(RLIMIT_NOFILE, &limit);
#endif
}

#if !defined (VC_WINCE) && !defined(ARM_LINUX) && !defined(ELDK_LINUX) && !defined(MIPS64_LINUX)
inline FILE* idlOS::popen(SChar *command, SChar *mode)
{
#if defined( VC_WIN32 )
    return ::_popen(command, mode);
#elif defined(WRS_VXWORKS)
    return NULL;
#elif defined(ITRON)
    /* empty */
    return NULL;
#else
    return ::popen(command, mode);
#endif
}
#endif /* !VC_WINCE && !ARM_LINUX && !ELDK_LINUX && !MIPS64_LINUX */

inline PDL_HANDLE idlOS::idlOS_fileno(FILE *fp)
{
#if defined( VC_WIN32 )
    return PDL_INVALID_HANDLE;
#elif defined(ITRON)
   /* empty */
    return PDL_INVALID_HANDLE;
#else
    return fileno(fp);
#endif
}

#if !defined( VC_WINCE )
inline SInt idlOS::pclose(FILE *fp)
{
#if defined( VC_WIN32 )
    return ::_pclose(fp);
#elif defined(WRS_VXWORKS)
    return -1;
#elif defined(ITRON)
    /* empty */
    return -1;
#else
    return ::pclose(fp);
#endif
}
#endif /* !VC_WINCE */

inline ULong idlOS::getThreadID()
{
#ifdef DEC_TRU64
    return pthread_getsequence_np(PDL_OS::thr_self());
#elif defined(ITRON)
    /* empty */
    return 1;
#else
    return (ULong)PDL_OS::thr_self();
#endif
}

inline SInt idlOS::getParallelIndex()
{
    SInt sParallelIndex;

#if defined(INTEL_LINUX) || defined(ALPHA_LINUX) || defined(NTO_QNX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(AMD64_LINUX) || defined(X86_64_LINUX)
    // Consecutive thread id's in Linux are 1024 apart.
    sParallelIndex = (SInt) PDL_OS::thr_self() / 1024;
#elif defined( IBM_AIX )
    // Consecutive thread id's in AIX are 257 apart.
    sParallelIndex = (SInt) PDL_OS::thr_self() / 257;
#elif defined(DEC_TRU64)
    sParallelIndex = pthread_getsequence_np(PDL_OS::thr_self());
#elif defined(XEON_LINUX)
    // thread id의 하위 12바이트는 항상 0x960 이더라. (by egonspace)
    sParallelIndex = (SInt) PDL_OS::thr_self() >> 12;
#elif defined(X86_64_DARWIN)
    sParallelIndex = (SLong)(PDL_OS::thr_self()) / 1024;
#else
    sParallelIndex = (SInt)PDL_OS::thr_self();
#endif

    return (sParallelIndex > 0) ? sParallelIndex : -sParallelIndex;
}

inline ULong idlOS::getThreadID(PDL_thread_t aID)
{
#ifdef DEC_TRU64
    return pthread_getsequence_np(aID);
#elif defined(ITRON)
    /* empty */
    return 1;
#else
    return (ULong)aID;
#endif
}

inline SInt idlOS::tcgetattr ( SInt filedes, struct termios *termios_p )
{
#if defined(VC_WIN32)
    // KM_DO_NOTHING_FATAL
    return 0;
#elif defined(WRS_VXWORKS)
    return 0;
#elif defined(ITRON)
    return 0;
#elif defined(SYMBIAN)
    PDL_UNUSED_ARG(filedes);
    PDL_UNUSED_ARG(termios_p);
    return 0;
#else
    return ::tcgetattr(filedes, termios_p);
#endif
}
inline SInt idlOS::tcsetattr( SInt filedes, SInt optional_actions, struct termios *termios_p )
{
#if defined(VC_WIN32)
    // KM_DO_NOTHING_FATAL
    return 0;	
#elif defined(WRS_VXWORKS)
    return 0;
#elif defined(ITRON)
    return 0;
#elif defined(SYMBIAN)
    PDL_UNUSED_ARG(filedes);
    PDL_UNUSED_ARG(optional_actions);
    PDL_UNUSED_ARG(termios_p);
    return 0;
#else
    return ::tcsetattr(filedes, optional_actions, termios_p );
#endif
}

/*
 * 함수의 리턴값을 받는 것이 프로그래밍 하기 매우 편함.
 * => so, PDL의 라이브러리를 다시 한번 inline으로 변환
 */

inline void *idlOS::thr_getspecific2 (PDL_thread_key_t key)
{
    void *p;
    PDL_OS::thr_getspecific(key, &p);
    return p;
}
inline SInt idlOS::chmod(SChar * /*filename*/, mode_t /*t*/)
{
#if defined(VC_WIN32)
    // KM_DO_NOTHING
    return 0;
#elif defined(WRS_VXWORKS)
    return 0;
#elif defined(ITRON)
    return 0;
#else
	return -1;
#endif
}

#if !defined(VC_WINCE)
inline SChar *idlOS::strerror(SInt errnum)
{
#if !defined(ITRON)
    return ::strerror(errnum);
#else
    return NULL;
#endif
}
#endif /* !VC_WINCE */

inline SInt idlOS::pthread_cond_init(pthread_cond_t *cond,
									   pthread_condattr_t *cond_attr)
{
#if defined(VC_WIN32) || defined(WRS_VXWORKS)
    PDL_condattr_t attributes;
	if (cond_attr == NULL) // create default condition attribute
	{
		if (PDL_OS::condattr_init (attributes) != 0)
		{
			return -1;
		}
		cond_attr = &attributes;
	}
    return PDL_OS::cond_init(cond, *cond_attr);
#elif defined(ITRON)
    /* empty */
    return -1;
#else
#       if !defined(CYGWIN32)
            return ::pthread_cond_init(cond,cond_attr);
#       else
            return pthread_cond_init(cond,cond_attr);
#       endif
#endif
}


inline SInt idlOS::pthread_once ( pthread_once_t * aOnceControl, pthread_once_init_function_t aInitFunc)
{
#if defined(VC_WINCE) || defined(VC_WIN32)

  int result;

  if (aOnceControl == NULL || aInitFunc == NULL)
    {

      result = EINVAL;
      goto FAIL0;

    }
  else
    {
      result = 0;
    }

  if (!aOnceControl->done)
    {
      if (InterlockedIncrement (&(aOnceControl->started)) == 0)
        {
          /*
           * First thread to increment the started variable
           */
          (*aInitFunc) ();
          aOnceControl->done = TRUE;

        }
      else
        {
          /*
           * Block until other thread finishes executing the onceRoutine
           */
          while (!(aOnceControl->done))
            {
              /*
               * The following gives up CPU cycles without pausing
               * unnecessarily
               */
              Sleep (0);
            }
        }
    }
FAIL0:
    return (result);
#elif defined(WRS_VXWORKS) 
    if( taskLock() == OK ) 
    { 
        if( (*aOnceControl) == 0 ) 
        { 
            (*aInitFunc)(); 
            (*aOnceControl)++; 
        } 
        (void)taskUnlock(); 
    } 
#else
    return ::pthread_once(aOnceControl, aInitFunc);
#endif
}

inline SInt   idlOS::pthread_getconcurrency(void)
{
#if defined(HP_HPUX) || \
    ((defined(SPARC_SOLARIS) || defined(X86_SOLARIS)) && (OS_MAJORVER == 2) && (OS_MINORVER >= 7))
    return ::pthread_getconcurrency();
#elif ((defined(SPARC_SOLARIS) || defined(X86_SOLARIS)) && (OS_MAJORVER == 2) && (OS_MINORVER < 7))
    return ::thr_getconcurrency();
#elif defined(INTEL_LINUX) || defined(ALPHA_LINUX) || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(AMD64_LINUX) || defined(X86_64_DARWIN)
/*
  BUGBUG : g++ linux compile시 -D_GNU_SOURCE를 넣어 full pthread api를 사용할
	   수 있는지 검사 요망 : by gamestar 2000/9/4
    #if defined(__USE_UNIX98)
	return ::pthread_getconcurrency();
    #else
	return 0;
    #endif
*/
    return 0;
#elif defined(VC_WIN32)
    // KM_DO_NOTHING
    return 0;
#elif defined(ITRON)
    return -1;
#else
    return -1;
#endif
}

#if defined(HP_HPUX) || \
    ((defined(SPARC_SOLARIS) || defined(X86_SOLARIS)) && (OS_MAJORVER == 2) && (OS_MINORVER >= 7)) || \
    defined(IBM_AIX)
inline SInt idlOS::pthread_setconcurrency( SInt new_level )
{
    return ::pthread_setconcurrency(new_level);
}
#elif ((defined(SPARC_SOLARIS) || defined(X86_SOLARIS)) && (OS_MAJORVER == 2) && (OS_MINORVER < 7))
inline SInt idlOS::pthread_setconcurrency( SInt new_level )
{
    return ::thr_setconcurrency(new_level);
}
#elif defined(INTEL_LINUX) || defined(ALPHA_LINUX) || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(AMD64_LINUX) || defined(X86_64_DARWIN)
inline SInt idlOS::pthread_setconcurrency( SInt )
{
    return 0;
}

/*
  BUGBUG : g++ linux compile시 -D_GNU_SOURCE를 넣어 full pthread api를 사용할
	   수 있는지 검사 요망 : by gamestar 2000/9/4
    #if defined(__USE_UNIX98)
	return ::pthread_getconcurrency();
    #else
	return 0;
    #endif
*/
#elif defined(VC_WIN32) || defined(ITRON)
    // KM_DO_NOTHING
inline SInt idlOS::pthread_setconcurrency( SInt )
{
    return 0;
}
#else
inline SInt idlOS::pthread_setconcurrency( SInt )
{
    return -1;
}
#endif

inline int idlOS::directio ( PDL_HANDLE fd, int mode)
{
#if defined(SPARC_SOLARIS) || defined(X86_SOLARIS)
# if (OS_MAJORVER == 2) && (OS_MINORVER == 5)
	if ( fd != 0 && mode != 0 )
		return -1;
	return 0;
# else /* (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
	return ::directio (fd, mode);
# endif /* (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
#endif

	if ( fd != 0 && mode != 0 )
		return -1;
	return 0;
}

inline SInt idlOS::idlOS_feof(FILE *fp)
{
    return feof(fp);
}

inline UInt idlOS::strToUInt( UChar* aValue,
			      UInt   aLength )
{
    UInt sValue;

    for( sValue = 0; aLength > 0; aValue++, aLength-- )
    {
	sValue = sValue*10 + *aValue - '0';
    }

    return sValue;
}

inline ULong idlOS::strToULong( UChar* aValue,
                                UInt   aLength )
{
    ULong sValue;

    for( sValue = 0; aLength > 0; aValue++, aLength-- )
    {
	sValue = sValue*10 + *aValue - '0';
    }

    return sValue;
}

inline void idlOS::strUpper( void*  aValue,
                             size_t aLength )
{
    UChar* sValue;

    for( sValue = (UChar*)aValue; aLength > 0; sValue++, aLength-- )
    {
        *sValue = idlOS_toupper( *sValue );
    }
}

inline SInt idlOS::strCaselessMatch( const void* aValue1,
                                     const void* aValue2 )
{
    const UChar* sValue1;
    const UChar* sValue2;

    sValue1 = (const UChar*)aValue1;
    sValue2 = (const UChar*)aValue2;

    for( ; *sValue1 != '\0' && *sValue2 != '\0'; sValue1++, sValue2++ )
    {
        if( idlOS_toupper(*sValue1) != idlOS_toupper(*sValue2) )
        {
            return -1;
        }
    }

    if( *sValue1 != '\0' || *sValue2 != '\0' )
    {
        return -1;
    }

    return 0;
}

inline SInt idlOS::strCaselessMatch( const void* aValue1,
                                     size_t	 aLength1,
                                     const void* aValue2,
                                     size_t	 aLength2 )
{
    const UChar* sValue1;
    const UChar* sValue2;
    size_t       sCount;

    if( aLength1 != aLength2 )
    {
        return -1;
    }

    sValue1 = (const UChar*)aValue1;
    sValue2 = (const UChar*)aValue2;
    for( sCount = 0; sCount < aLength1; sCount++ )
    {
        if( idlOS_toupper(sValue1[sCount]) != idlOS_toupper(sValue2[sCount]) )
        {
            return -1;
        }
    }

    return 0;
}

inline SInt idlOS::strMatch( const void* aValue1,
			     size_t	 aLength1,
			     const void* aValue2,
			     size_t	 aLength2 )
{
    return aLength1 == aLength2 ?
	   memcmp( aValue1, aValue2, aLength1 ) : -1;
}

inline SInt idlOS::strCompare( const void* aValue1,
			       size_t	   aLength1,
			       const void* aValue2,
			       size_t	   aLength2 )
{
    if( aLength1 < aLength2 )
    {
	return ( memcmp( aValue1, aValue2, aLength1 ) >	 0 ) ? 1 : -1 ;
    }
    if( aLength1 > aLength2 )
    {
	return ( memcmp( aValue1, aValue2, aLength2 ) >= 0 ) ? 1 : -1 ;
    }
    return memcmp( aValue1, aValue2, aLength1 );
}

inline int idlOS::idlOS_tolower( int c )
{
    return ( ( c >= 'A' && c <= 'Z' ) ? (c|0x20) : c );
}

inline int idlOS::idlOS_toupper( int c )
{
    return ( ( c >= 'a' && c <= 'z' ) ? (c&0xDF) : c );
}

inline int idlOS::idlOS_isspace( int c )
{
     return ( c==' '||c=='\f'||c=='\n'||c=='\r'||c=='\t'||c=='\v') ? 1 : 0 ;
}

#ifdef COMPILE_64BIT
#define IDL_ALIGN_VALUE 7
#else
#define IDL_ALIGN_VALUE 3
#endif

inline SInt   idlOS::align(SInt Size)
{
    return ( (Size + IDL_ALIGN_VALUE) & ~IDL_ALIGN_VALUE );
}
inline UInt   idlOS::align(UInt Size)
{
    return ( (Size + IDL_ALIGN_VALUE) & ~IDL_ALIGN_VALUE );
}
inline SLong  idlOS::align(SLong Size)
{
    return ( (Size + IDL_ALIGN_VALUE) & ~IDL_ALIGN_VALUE );
}
inline ULong  idlOS::align(ULong Size)
{
    return ( (Size + IDL_ALIGN_VALUE) & ~IDL_ALIGN_VALUE );
}

inline SInt   idlOS::align4(SInt Size)
{
    return ( (Size + 3) & ~3 );
}
inline UInt   idlOS::align4(UInt Size)
{
    return ( (Size + 3) & ~3 );
}
inline SLong  idlOS::align4(SLong Size)
{
    return ( (Size + 3) & ~3 );
}
inline ULong  idlOS::align4(ULong Size)
{
    return ( (Size + 3) & ~3 );
}

inline SInt   idlOS::align8(SInt Size)
{
    return ( (Size + 7) & ~7 );
}
inline UInt   idlOS::align8(UInt Size)
{
    return ( (Size + 7) & ~7 );
}
inline SLong  idlOS::align8(SLong Size)
{
    return ( (Size + 7) & ~7 );
}
inline ULong  idlOS::align8(ULong Size)
{
    return ( (Size + 7) & ~7 );
}

inline UInt idlOS::align( UInt aOffset, UInt aAlign )
{
    return (( aOffset + aAlign - 1 ) / aAlign) * aAlign;
}

inline ULong idlOS::alignLong( ULong aOffset, ULong aAlign )
{
    return (( aOffset + aAlign - 1 ) / aAlign) * aAlign;
}

inline void* idlOS::align(void* a_pMem, UInt aAlign)
{
#ifdef COMPILE_64BIT
    return (void*)((( (ULong)a_pMem + aAlign - 1 ) / aAlign) * aAlign);
#else
    return (void*)((( (UInt)a_pMem + aAlign - 1 ) / aAlign) * aAlign);
#endif
}

inline void* idlOS::alignDown(void* aMemoryPtr, UInt aAlignSize )
{
#ifdef COMPILE_64BIT
    return (void*)(( (ULong)aMemoryPtr / aAlignSize) * aAlignSize);
#else
    return (void*)(( (UInt)aMemoryPtr / aAlignSize) * aAlignSize);
#endif
}

/*
#ifdef COMPILE_64BIT
    ULong *k = (ULong *)&a_pMem; // Workaround to convert pointer to value.
    return (void*)((( *k + aAlign - 1 ) / aAlign) * aAlign);
#else
    UInt *k = (UInt *)&a_pMem;
    return (void*)((( *k + aAlign - 1 ) / aAlign) * aAlign);
#endif
*/

#if (defined( IBM_AIX ) && !defined (__GNUG__) && defined(COMPILE_64BIT)) || (defined(VC_WIN32) && !defined(COMPILE_64BIT)) || defined(X86_64_DARWIN)

inline vSLong  idlOS::align(vSLong Size)
{
    return ( (Size + IDL_ALIGN_VALUE) & ~IDL_ALIGN_VALUE );
}
inline vULong  idlOS::align(vULong Size)
{
    return ( (Size + IDL_ALIGN_VALUE) & ~IDL_ALIGN_VALUE );
}
inline vSLong  idlOS::align4(vSLong Size)
{
    return ( (Size + 3) & ~3 );
}
inline vULong  idlOS::align4(vULong Size)
{
    return ( (Size + 3) & ~3 );
}
inline vSLong  idlOS::align8(vSLong Size)
{
    return ( (Size + 7) & ~7 );
}
inline vULong  idlOS::align8(vULong Size)
{
    return ( (Size + 7) & ~7 );
}
#endif




// Non Blocking 모드로 만듦
inline SInt
idlVA::setNonBlock(PDL_SOCKET fd)
{
#if defined(VC_WIN32)
    return idlVA::set_flags(fd, PDL_NONBLOCK);
#elif defined(WRS_VXWORKS) 
    SInt a = 1; 
    return ioctl(fd, FIONBIO, (int)&a); 
#else
    SInt flags = idlOS::fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return idlOS::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

// Blocking 모드로 만듦
inline SInt
idlVA::setBlock(PDL_SOCKET fd)
{
#if defined(VC_WIN32)
    return idlVA::clr_flags(fd, PDL_NONBLOCK);
#elif defined(WRS_VXWORKS) 
    SInt a = 0; 
    return ioctl(fd, FIONBIO, (int)&a); 
#else
    SInt flags = idlOS::fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return idlOS::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

inline SInt
idlVA::setSockReuseraddr(PDL_SOCKET fd)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    SInt flag = 1;
    return idlOS::setsockopt(fd,
			     SOL_SOCKET,
			     SO_REUSEADDR,
			     (SChar *) &flag,
			     (SInt)(sizeof(SInt)) );
#endif
}

inline ssize_t idlVA::readline(PDL_HANDLE fd, void *vptr, size_t maxlen)
{
    ssize_t	n, rc;
    SChar	c, *ptr;

    ptr = (SChar *)vptr;
    for (n = 1; n < (ssize_t)maxlen; n++) {
    again:
	rc = idlOS::read(fd, &c, 1);
	if ( rc == 1 )
	{
            *ptr++ = c;
            if (c == '\n')
                break;	/* newline is stored, like fgets() */
        }
	else
	    if (rc == 0)
            {
                if (n == 1)
                    return(0);	/* EOF, no data read */
                else
                    break;		/* EOF, some data was read */
            }
            else
            {
                if (errno == EINTR)
                    goto again;
                return(-1);		/* error, errno set by read() */
            }
    }

    *ptr = 0;	/* null terminate like fgets() */
    return(n);
}

/*BUGBUG_NT*/
inline ssize_t idlVA::recvline(PDL_SOCKET fd, const SChar *vptr, size_t maxlen)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    ssize_t	n, rc;
    SChar	c, *ptr;

    ptr = (SChar *)vptr;
    for (n = 1; n < (ssize_t)maxlen; n++)
    {
    again:
        if ( (rc = idlOS::recv(fd, &c, 1)) == 1)
	{
            *ptr++ = c;
            if (c == '\n')
                break;	/* newline is stored, like fgets() */
        }
	else
        {
	    if (rc == 0)
            {
                if (n == 1)
                    return(0);	/* EOF, no data read */
                else
                    break;		/* EOF, some data was read */
            }
            else
            {
                //fix BUG-17609
#ifndef PDL_WIN32
                if (errno == EINTR)
#else
                if (errno == WSAEINTR)
#endif
                    goto again;
                return(-1);		/* error, errno set by read() */
            }
        }
    }

    *ptr = 0;	/* null terminate like fgets() */
    return(n);
#endif
}
/*BUGBUG_NT ADD*/


inline ssize_t idlVA::timed_readline(PDL_HANDLE fp,
				  void *message,
				  ssize_t maxlen,
				  PDL_Time_Value *tm)
{
    /* BUGBUG_NT: PDL_HANDLE -> PDL_SOCKET may not be casted */
    if (idlVA::handle_read_ready((PDL_SOCKET)fp, tm) > 0)
    {
	//idlOS::fgets(message, FIFO_BUFFER, Rfp);
	return readline(fp, message, maxlen);
    }
    return -1;
}
/*BUGBUG_NT*/
inline ssize_t idlVA::timed_recvline(PDL_SOCKET fp,
				  const SChar *message,
				  ssize_t maxlen,
				  PDL_Time_Value *tm)
{
    ssize_t	n, rc;
    SChar	c, *ptr;

    ptr = (SChar *)message;
    for (n = 1; n < (ssize_t)maxlen; n++)
    {
    again:
        if ( (rc = idlVA::recv_nn_i(fp, &c, 1, tm)) == 1)
	{
            *ptr++ = c;
            if (c == '\n')
                break;	/* newline is stored, like fgets() */
        }
	else
        {
	    if (rc == 0)
            {
                if (n == 1)
                    return(0);	/* EOF, no data read */
                else
                    break;		/* EOF, some data was read */
            }
            else
            {
//fix BUG-17609
#ifndef PDL_WIN32
                if (errno == EINTR)
#else
                if (errno == WSAEINTR)
#endif                
                    goto again;
                return(-1);		/* error, errno set by read() */
            }
        }
    }

    *ptr = 0;	/* null terminate like fgets() */
    return(n);
}
/*BUGBUG_NT ADD*/


inline ssize_t idlVA::writeline(PDL_HANDLE fd, void *vptr)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    return idlOS::write(fd, vptr, idlOS::strlen((SChar *)vptr));
#endif
}

/*BUGBUG_NT*/
inline ssize_t idlVA::sendline(PDL_SOCKET fd, const SChar *vptr)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    return idlOS::send(fd, (const char *)vptr, idlOS::strlen((const char*)vptr));
#endif

}
/*BUGBUG_NT ADD*/


inline ssize_t idlVA::timed_writeline(PDL_HANDLE fp,
				   void *message,
				   PDL_Time_Value *tm)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    /* BUGBUG_NT: PDL_HANDLE -> PDL_SOCKET may not be casted */
    if (idlVA::handle_write_ready((PDL_SOCKET)fp, tm) > 0)
    {
	//idlOS::fgets(message, FIFO_BUFFER, Rfp);
	return writeline(fp, message);
    }
    return -1;
#endif

}

/*BUGBUG_NT*/
inline ssize_t idlVA::timed_sendline(PDL_SOCKET fp,
				   const SChar *message,
				   PDL_Time_Value *tm)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    if (idlVA::handle_write_ready(fp, tm) > 0)
    {
	//idlOS::fgets(message, FIFO_BUFFER, Rfp);
	return sendline(fp, message);
    }
    return -1;
#endif

}
/*BUGBUG_NT ADD*/

// RECV ======================================
ASYS_INLINE ssize_t
idlVA::recv_nn_i (PDL_SOCKET handle,
		  void *buf,
		  size_t len)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    size_t bytes_transferred;
    ssize_t n;

    for (bytes_transferred = 0;
	 bytes_transferred < len;
	 bytes_transferred += n)
    {
	n = idlVA::recv_i (handle,
			   (char *) buf + bytes_transferred,
			   len - bytes_transferred);
	if (n == -1)
	{
	    // If blocked, try again.
	    if (errno == EWOULDBLOCK)
		n = 0;

	    //
	    // No timeouts in this version.
	    //

	    // Other errors.
	    return -1;
	}
	else if (n == 0)
	    return 0;
    }

    return bytes_transferred;
#endif

}

// recv_nn_to_i는 조금이라도 받은 데이터가 있더라도 Timeout이 걸리면
// 바로 에러를 리턴한다.

// 하지만 recv_nn은 조금이라도 받은 데이터가 있으면 Timeout이 걸려도
// 데이터를 끝까지 받는다.
ASYS_INLINE ssize_t
idlVA::recv_nn_i (PDL_SOCKET handle,
		  void *buf,
		  size_t len,
		  const PDL_Time_Value *timeout)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    //int val = 0;
    // for performance
    //idlVA::record_and_set_non_blocking_mode (handle, val);

    size_t bytes_transferred;
    ssize_t n;
    int error = 0;
#if defined(IBM_AIX) || defined(SPARC_SOLARIS) || defined (X86_SOLARIS)
    int i=0;
#endif
    for (bytes_transferred = 0;
	 bytes_transferred < len;
	 bytes_transferred += n)
    {
	int result = idlVA::handle_read_ready (handle,
					       timeout);

	if (result == -1)
	{
		// Timed out; return bytes transferred.
		if (errno == ETIME)
		{
            // 조금이라도 받은 데이터가 있으면 Timeout을 무시하고
            // 데이터를 끝까지 받는다.
			if (bytes_transferred == 0)
			{
				break;
			}
			else
			{
				n = 0;
				continue;
			}
		}
//fix BUG-17609
#ifndef PDL_WIN32
        if (errno == EINTR)
#else
        if (errno == WSAEINTR) 
#endif
		{
			n = 0;
			errno = 0;
			continue;
		}

		// Other errors.
		error = 1;
		break;
    }

    n = idlVA::recv_i (handle,
                        (char *) buf + bytes_transferred,
                        len - bytes_transferred);

    // Errors (note that errno cannot be EWOULDBLOCK since select()
    // just told us that data is available to read).
    if (n == -1 || n == 0)
    {
        //fix BUG-17609        
#ifndef    PDL_WIN32
        if ((n == -1) && (errno == EINTR))
#else
        if ((n == -1) && (errno == WSAEINTR))
#endif
		{
			n = 0;
			errno = 0;
			continue;
		}
#if defined(IBM_AIX) || defined(SPARC_SOLARIS) || defined (X86_SOLARIS)
            // the above asumption is not true for IBM AIX
            // If blocked, try again. refer PR-1059
            if( errno == EAGAIN )
            {
                // To fix PR-1374 and PR-1375
                // EAGAIN can be returned when client abnormally terminated

                // to Fix PR-2974: 2002/09/13
                // 대량의 Data가 IO에 의해 처리될 경우, EAGAIN이 발생하며,
                // 10회이상 발생할 수 있다.(Data Lost)  따라서, 이 count를 5000으로 수정한다.
                // 근거 : 1. send_nn() 혹은 recv_nn()이 한번 불릴 때 64K 전송시
                //           5000번 이상의 EAGAIN이 호출될 가능성이 거의 없다.
                //        2. disconnection시 EAGAIN의 경우 5000번 수행후 반드시 종료된다.
                //           즉, busy wait이기 때문에 수초 내에 종료될 수 있다.
                i++;
                if ( i > 5000 )
                {
                    error = 2;
                    break;
                }
                errno = 0; // errno 초기화.
                n = 0;
            }
            else
            {
                error = 1;
                break;
            }
#else /* IBM_AIX || SPARC_SOLARIS || X86_SOLARIS */
            error = 1;
            break;
#endif /* IBM_AIX || SPARC_SOLARIS || X86_SOLARIS */
        }
    }
    // for performance
    //idlVA::restore_non_blocking_mode (handle, val);

#ifdef IBM_AIX
    if ( error == 2 )
    {
        return -2;
    }
#endif /* IBM_AIX */
    if (error)
        return -1;
    else
        return bytes_transferred;
#endif

}

// To Fix BUG-15181 [A3/A4] recv_nn_i가 timeout을 무시합니다.
// recv_nn 은 조금이라도 받은 데이터가 있으면 timeout을 무시한다.
// ( replication에서 이러한 스키마로 recv_nn을 사용하고 있어서
//   recv_nn을 직접 수정할 수 없음. )
//
// recv_nn_to는 조금이라도 받은 데이터가 있어도
// timeout걸리면 에러를 리턴한다.
ASYS_INLINE ssize_t
idlVA::recv_nn_to_i (PDL_SOCKET handle,
		  void *buf,
		  size_t len,
		  const PDL_Time_Value *timeout)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    //int val = 0;
    // for performance
    //idlVA::record_and_set_non_blocking_mode (handle, val);

    size_t bytes_transferred;
    ssize_t n;
    int error = 0;
#if defined(IBM_AIX) || defined(SPARC_SOLARIS) || defined (X86_SOLARIS)
    int i=0;
#endif
    for (bytes_transferred = 0;
	 bytes_transferred < len;
	 bytes_transferred += n)
    {
	int result = idlVA::handle_read_ready (handle,
					       timeout);

	if (result == -1)
	{
		// Timed out; return bytes transferred.
		if (errno == ETIME)
		{
            // To Fix BUG-15181 [A3/A4] recv_nn_i가 timeout을 무시합니다.
            error = 1;
            
            break;
		}
//fix BUG-17609
#ifndef   PDL_WIN32
        if (errno == EINTR)
#else
        if( errno == WSAEINTR)
#endif
		{
			n = 0;
			errno = 0;
			continue;
		}

		// Other errors.
		error = 1;
		break;
    }

    n = idlVA::recv_i (handle,
                        (char *) buf + bytes_transferred,
                        len - bytes_transferred);

    // Errors (note that errno cannot be EWOULDBLOCK since select()
    // just told us that data is available to read).
    if (n == -1 || n == 0)
    {
        //fix BUG-17609
#ifndef PDL_WIN32
        if ((n == -1) && (errno == EINTR))
#else
        if( (n == -1) && (errno == WSAEINTR))
#endif 
		{
			n = 0;
			errno = 0;
			continue;
		}
#if defined(IBM_AIX) || defined(SPARC_SOLARIS) || defined (X86_SOLARIS)
            // the above asumption is not true for IBM AIX
            // If blocked, try again. refer PR-1059
            if( errno == EAGAIN )
            {
                // To fix PR-1374 and PR-1375
                // EAGAIN can be returned when client abnormally terminated

                // to Fix PR-2974: 2002/09/13
                // 대량의 Data가 IO에 의해 처리될 경우, EAGAIN이 발생하며,
                // 10회이상 발생할 수 있다.(Data Lost)  따라서, 이 count를 5000으로 수정한다.
                // 근거 : 1. send_nn() 혹은 recv_nn()이 한번 불릴 때 64K 전송시
                //           5000번 이상의 EAGAIN이 호출될 가능성이 거의 없다.
                //        2. disconnection시 EAGAIN의 경우 5000번 수행후 반드시 종료된다.
                //           즉, busy wait이기 때문에 수초 내에 종료될 수 있다.
                i++;
                if ( i > 5000 )
                {
                    error = 2;
                    break;
                }
                errno = 0; // errno 초기화.
                n = 0;
            }
            else
            {
                error = 1;
                break;
            }
#else /* IBM_AIX || SPARC_SOLARIS || X86_SOLARIS */
            error = 1;
            break;
#endif /* IBM_AIX || SPARC_SOLARIS || X86_SOLARIS */
        }
    }
    // for performance
    //idlVA::restore_non_blocking_mode (handle, val);

#ifdef IBM_AIX
    if ( error == 2 )
    {
        return -2;
    }
#endif /* IBM_AIX */
    if (error)
        return -1;
    else
        return bytes_transferred;
#endif

}



ASYS_INLINE ssize_t
idlVA::recv_i (PDL_SOCKET handle, void *buf, size_t len)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
#if defined (PDL_WIN32) || defined (PDL_PSOS)
    return idlOS::recv (handle, (char *) buf, len);
#else
    return idlOS::read (handle, (char *) buf, len);
#endif /* PDL_WIN32 */
#endif

}

// SEND  ======================================


ASYS_INLINE ssize_t
idlVA::send_nn_i (PDL_SOCKET handle,
                  void *buf,
                  size_t len)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    size_t bytes_transferred;
    ssize_t n;

    for (bytes_transferred = 0;
         bytes_transferred < len;
         bytes_transferred += n)
    {
        n = idlVA::send_i (handle,
                           (char *) buf + bytes_transferred,
                           len - bytes_transferred);
        if (n == -1)
        {
            // If blocked, try again.
            if (errno == EWOULDBLOCK)
                n = 0;

            //
            // No timeouts in this version.
            //

            // Other errors.
            return -1;
        }
        else if (n == 0)
            return 0;
    }

    return bytes_transferred;
#endif

}

ASYS_INLINE ssize_t
idlVA::send_nn_i (PDL_SOCKET handle,
                  void *buf,
                  size_t len,
                  const PDL_Time_Value *timeout)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
    //int val = 0;
    //idlVA::record_and_set_non_blocking_mode (handle, val);

    size_t bytes_transferred;
    ssize_t n;
    int error = 0;
#if defined(IBM_AIX) || defined(SPARC_SOLARIS) || defined (X86_SOLARIS)
    int i=0;
#endif

    for (bytes_transferred = 0;
         bytes_transferred < len;
         bytes_transferred += n)
    {
        int result = idlVA::handle_write_ready (handle,
                                                timeout);

        if (result == -1)
        {
            // Timed out; return bytes transferred.
            if (errno == ETIME)
                break;

//fix BUG-17609
#ifndef PDL_WIN32
            if (errno == EINTR)
#else
            if (errno == WSAEINTR)
#endif
            {
				n = 0;
				errno = 0;
				continue;
			}

            // Other errors.
            error = 1;
            break;
        }

        n = idlVA::send_i (handle,
                           (char *) buf + bytes_transferred,
                           len - bytes_transferred);

        // Errors (note that errno cannot be EWOULDBLOCK since select()
        // just told us that data can be written).
        if (n == -1 || n == 0)
        {
#ifndef    PDL_WIN32
            if ((n == -1) && (errno == EINTR))
#else
            if ((n == -1) && (errno == WSAEINTR))
#endif
			{
				n = 0;
				errno = 0;
				continue;
			}
#if defined(IBM_AIX) || defined(SPARC_SOLARIS) || defined (X86_SOLARIS)
            // the above asumption is not true for IBM AIX
            // If blocked, try again. refer PR-1059
            if( errno == EAGAIN )
            {
                // To fix PR-1374 and PR-1375
                // EAGAIN can be returned when client abnormally terminated

                // to Fix PR-2974: 2002/09/13
                // 대량의 Data가 IO에 의해 처리될 경우, EAGAIN이 발생하며,
                // 10회이상 발생할 수 있다. 따라서, 이 count를 5000으로 수정한다.
                // 근거 : 1. send_nn() 혹은 recv_nn()이 한번 불릴 때 64K 전송시
                //           5000번 이상의 EAGAIN이 호출될 가능성이 거의 없다.
                //        2. disconnection시 EAGAIN의 경우 5000번 수행후 반드시 종료된다.
                //           즉, busy wait이기 때문에 수초 내에 종료될 수 있다.
                i++;
                if ( i > 5000 )
                {
                    error = 2;
                    break;
                }
                errno = 0; // errno 초기화.
                n = 0;
            }
            else
            {
                error = 1;
                break;
            }
#else /* IBM_AIX || SPARC_SOLARIS || X86_SOLARIS */
            error = 1;
            break;
#endif /* IBM_AIX || SPARC_SOLARIS || X86_SOLARIS */
        }
    }

    //idlVA::restore_non_blocking_mode (handle, val);

#ifdef IBM_AIX
    if ( error == 2 )
    {
        return -2;
    }
#endif /* IBM_AIX */
    if (error)
        return -1;
    else
        return bytes_transferred;
#endif

}


ASYS_INLINE ssize_t
idlVA::send_i (PDL_SOCKET handle,  void *buf, size_t len)
{
#if defined(ITRON)
     /* empty */
    return -1;
#else
#if defined (PDL_WIN32) || defined (PDL_PSOS)
    return idlOS::send (handle, (const char *) buf, len);
#else
    return idlOS::write (handle, (const char *) buf, len);
#endif /* PDL_WIN32 */
#endif

}



// network -> host
# ifdef ENDIAN_IS_BIG_ENDIAN
inline UShort  idlOS::ntoh ( UShort Value )
{
    return Value;
}
inline SShort  idlOS::ntoh ( SShort Value )
{
    return Value;
}
inline UInt    idlOS::ntoh ( UInt   Value )
{
    return Value;
}
inline SInt    idlOS::ntoh ( SInt   Value )
{
    return Value;
}
inline ULong   idlOS::ntoh ( ULong  Value )
{
    return Value;
}
inline SLong   idlOS::ntoh ( SLong  Value )
{
    return Value;
}

// host -> network
inline UShort  idlOS::hton ( UShort Value )
{
    return Value;
}
inline SShort  idlOS::hton ( SShort Value )
{
    return Value;
}
inline UInt    idlOS::hton ( UInt   Value )
{
    return Value;
}
inline SInt    idlOS::hton ( SInt   Value )
{
    return Value;
}
inline ULong   idlOS::hton ( ULong  Value )
{
    return Value;
}
inline SLong   idlOS::hton ( SLong  Value )
{
    return Value;
}

#else // little endian

inline UShort  idlOS::ntoh ( UShort Value )
{
    return (((Value&0xFF00)>>8)|((Value&0x00FF)<<8));
}
inline SShort  idlOS::ntoh ( SShort Value )
{
    return (((Value&0xFF00)>>8)|((Value&0x00FF)<<8));
}
inline UInt    idlOS::ntoh ( UInt   Value )
{
    return (UInt)(((Value&ID_ULONG(0xFF000000))>>24)|((Value&ID_ULONG(0x00FF0000))>>8)|
            ((Value&ID_ULONG(0x0000FF00))<<8)|((Value&ID_ULONG(0x000000FF))<<24));
}
inline SInt    idlOS::ntoh ( SInt   Value )
{
    return (SInt)(((Value&ID_ULONG(0xFF000000))>>24)|((Value&ID_ULONG(0x00FF0000))>>8)|
            ((Value&ID_ULONG(0x0000FF00))<<8)|((Value&ID_ULONG(0x000000FF))<<24));
}
inline ULong   idlOS::ntoh ( ULong  Value )
{
    return (((Value&ID_ULONG(0xFF00000000000000))>>56)|
            ((Value&ID_ULONG(0x00FF000000000000))>>40)|
            ((Value&ID_ULONG(0x0000FF0000000000))>>24)|
            ((Value&ID_ULONG(0x000000FF00000000))>>8)|
            ((Value&ID_ULONG(0x00000000FF000000))<<8)|
            ((Value&ID_ULONG(0x0000000000FF0000))<<24)|
            ((Value&ID_ULONG(0x000000000000FF00))<<40)|
            ((Value&ID_ULONG(0x00000000000000FF))<<56));
}
inline SLong   idlOS::ntoh ( SLong  Value )
{
    return (((Value&ID_LONG(0xFF00000000000000))>>56)|
            ((Value&ID_LONG(0x00FF000000000000))>>40)|
            ((Value&ID_LONG(0x0000FF0000000000))>>24)|
            ((Value&ID_LONG(0x000000FF00000000))>>8)|
            ((Value&ID_LONG(0x00000000FF000000))<<8)|
            ((Value&ID_LONG(0x0000000000FF0000))<<24)|
            ((Value&ID_LONG(0x000000000000FF00))<<40)|
            ((Value&ID_LONG(0x00000000000000FF))<<56));
}

// host -> network
inline UShort  idlOS::hton ( UShort Value )
{
    return (((Value&0xFF00)>>8)|((Value&0x00FF)<<8));
}
inline SShort  idlOS::hton ( SShort Value )
{
    return (((Value&0xFF00)>>8)|((Value&0x00FF)<<8));
}
inline UInt    idlOS::hton ( UInt   Value )
{
    return (UInt)(((Value&ID_ULONG(0xFF000000))>>24)|((Value&ID_ULONG(0x00FF0000))>>8)|
            ((Value&ID_ULONG(0x0000FF00))<<8)|((Value&ID_ULONG(0x000000FF))<<24));
}
inline SInt    idlOS::hton ( SInt   Value )
{
    return (SInt)(((Value&ID_ULONG(0xFF000000))>>24)|((Value&ID_ULONG(0x00FF0000))>>8)|
            ((Value&ID_ULONG(0x0000FF00))<<8)|((Value&ID_ULONG(0x000000FF))<<24));
}
inline ULong   idlOS::hton ( ULong  Value )
{
    return (((Value&ID_ULONG(0xFF00000000000000))>>56)|
            ((Value&ID_ULONG(0x00FF000000000000))>>40)|
            ((Value&ID_ULONG(0x0000FF0000000000))>>24)|
            ((Value&ID_ULONG(0x000000FF00000000))>>8)|
            ((Value&ID_ULONG(0x00000000FF000000))<<8)|
            ((Value&ID_ULONG(0x0000000000FF0000))<<24)|
            ((Value&ID_ULONG(0x000000000000FF00))<<40)|
            ((Value&ID_ULONG(0x00000000000000FF))<<56));
}
inline SLong   idlOS::hton ( SLong  Value )
{
    return (((Value&ID_LONG(0xFF00000000000000))>>56)|
            ((Value&ID_LONG(0x00FF000000000000))>>40)|
            ((Value&ID_LONG(0x0000FF0000000000))>>24)|
            ((Value&ID_LONG(0x000000FF00000000))>>8)|
            ((Value&ID_LONG(0x00000000FF000000))<<8)|
            ((Value&ID_LONG(0x0000000000FF0000))<<24)|
            ((Value&ID_LONG(0x000000000000FF00))<<40)|
            ((Value&ID_LONG(0x00000000000000FF))<<56));
}

#endif // endian compile for hton(), ntoh()

inline double idlOS::logb(double x)
{
#if defined(VC_WIN32)
    return ::_logb(x);
#elif defined(WRS_VXWORKS)
    return 0;
#elif defined(ITRON)
    return -1;
#else
#  if !defined(CYGWIN32)
	return ::logb(x);
#  else
	return logb(x);
#  endif
#endif /*VC_WIN32*/
}

/*
inline double idlOS::ceil(double x)
{
	return ::ceil(x);
}
*/


// math functions in <math.h> -- start
inline SDouble idlOS::sin(SDouble x)
{
    return ::sin(x);
}

inline SDouble idlOS::sinh(SDouble x)
{
    return ::sinh(x);
}

inline SDouble idlOS::asin(SDouble x)
{
    return ::asin(x);
}
/*
inline SDouble idlOS::asinh(SDouble x)
{
    return ::asinh(x);
}
*/

inline SDouble idlOS::cos(SDouble x)
{
    return ::cos(x);
}

inline SDouble idlOS::cosh(SDouble x)
{
    return ::cosh(x);
}

inline SDouble idlOS::acos(SDouble x)
{
    return ::acos(x);
}
/*
inline SDouble idlOS::acosh(SDouble x)
{
    return ::acosh(x);
}
*/
inline SDouble idlOS::tan(SDouble x)
{
    return ::tan(x);
}

inline SDouble idlOS::tanh(SDouble x)
{
    return ::tanh(x);
}

inline SDouble idlOS::atan(SDouble x)
{
    return ::atan(x);
}
/*
inline SDouble idlOS::atanh(SDouble x)
{
    return ::atanh(x);
}
*/
inline SDouble idlOS::atan2(SDouble y, SDouble x)
{
    return ::atan2(y, x);
}

inline SDouble idlOS::sqrt(SDouble x)
{
    return ::sqrt(x);
}
/*
inline SDouble idlOS::cbrt(SDouble x)
{
    return ::cbrt(x);
}
*/
inline SDouble idlOS::floor(SDouble x)
{
    return ::floor(x);
}

inline SDouble idlOS::ceil(SDouble x)
{
    return ::ceil(x);
}
/*
inline SDouble idlOS::copysign(SDouble x, SDouble y)
{
    return ::copysign(x, y);
}

inline SDouble idlOS::erf(SDouble x)
{
    return ::erf(x);
}

inline SDouble idlOS::erfc(SDouble x)
{
    return ::erfc(x);
}
*/
inline SDouble idlOS::fabs(SDouble x)
{
    return ::fabs(x);
}
/*
inline SDouble idlOS::lgamma(SDouble x)
{
    return ::lgamma(x);
}

inline SDouble idlOS::lgamma_r(SDouble x, SInt *signgamp)
{
    return ::lgamma_r(x, signgamp);
}
*/
inline SDouble idlOS::hypot(SDouble x, SDouble y)
{
#if (defined(VC_WIN32) && (_MSC_VER >= 1400)) || (defined(VC_WINCE))
   return ::_hypot(x, y);
#else
   return ::hypot(x, y);
#endif
}
/*
inline SInt idlOS::ilogb(SDouble x)
{
    return ::ilogb(x);
}
*/
/*
inline SInt idlOS::isnan(SDouble dsrc)
{
    return ::isnan(dsrc);
}
*/

inline SDouble idlOS::log(SDouble x)
{
    return ::log(x);
}

inline SDouble idlOS::log10(SDouble x)
{
    return ::log10(x);
}
/*
inline SDouble idlOS::log1p(SDouble x)
{
    return ::log1p(x);
}

inline SDouble idlOS::nextafter(SDouble x, SDouble y)
{
    return ::nextafter(x, y);
}
*/
inline SDouble idlOS::pow(SDouble x, SDouble y)
{
    return ::pow(x, y);
}
/*
inline SDouble idlOS::remainder(SDouble x, SDouble y)
{
    return ::remainder(x, y);
}
*/
inline SDouble idlOS::fmod(SDouble x, SDouble y)
{
    return ::fmod(x, y);
}
/*
inline SDouble idlOS::rint(SDouble x)
{
    return ::rint(x);
}

inline SDouble idlOS::scalbn(SDouble x, SInt n)
{
    return ::scalbn(x, n);
}
*/
/*
inline SDouble idlOS::significand(SDouble x)
{
    return ::significand(x);
}
*/
/*
inline SDouble idlOS::scalb(SDouble x, SDouble n)
{
    return ::scalb(x, n);
}
*/
inline SDouble idlOS::exp(SDouble x)
{
    return ::exp(x);
}
/*
inline SDouble idlOS::expm1(SDouble x)
{
    return ::expm1(x);
}
*/
// math functions in <math.h> -- end


#endif



inline PDL_OFF_T idlOS::ftell(FILE *aFile)
{
    #if defined(VC_WIN32) && defined(VC_WIN64)
        return ::_ftelli64(aFile);
    #else
        return ::ftell(aFile);
    #endif
}

inline size_t idlOS::strnlen(const SChar *aString, size_t aMaxLength)
{
    size_t sPosition = 0;

    while(sPosition < aMaxLength)
    {
        if(*(aString + sPosition) == 0)
        {
            break;
        }
        sPosition++;
    }

    return sPosition;
}

