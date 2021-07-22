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
 
/* -*- C++ -*- */
// config-psosim-g++.h,v 4.13 1999/07/08 17:00:12 schmidt Exp

// The following configuration file is designed to work for pSOSim on SunOS5
// using the GNU/Cygnus g++ 2.7.2 compiler, without repo patch.

///////////////////////////////////////////////////////////////////////////////
// * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * //
//                                                                           //
//   Because pSOSim includes UNIX system header files in order to do         //
//   its emulation of pSOSystSem on Solaris, there are a number of           //
//   things that are "available" to PDL on pSOSim that are *not*             //
//   really available on pSOSystem.  Every effort has been made to           //
//   avoid dependencies on these "features" in the PDL pSOSim port,          //
//   which has in turn necessarily required pSOSim specific definitions.     //
//                                                                           //
//   To ease portability between pSOSim and pSOSystem, the definitions       //
//   in this file have been separated into three groups: those that          //
//   are known to be appropriate for both pSOSim and  pSOSystem, those       //
//   known to be appropriate for pSOSim but (probably) not for pSOSystem,    //
//   and those that are (probably) appropriate for pSOSystem, but are        //
//   not appropriate for pSOSim.                                             //
//                                                                           //
//   When porting from pSOSim to pSOSystem, it is (probably) a good          //
//   idea to leave the definitions in the first category alone,              //
//   comment out the definitions in the second category, and uncomment       //
//   the definitions in the third category.  Additional definitions          //
//   may need to be added to the third category, as only those that          //
//   were encountered during the pSOSim port were added (that is, one        //
//   of the system files included by pSOSim could be compensating for        //
//   a definition pSOSystem really needs.                                    //
//                                                                           //
// * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * //
///////////////////////////////////////////////////////////////////////////////


#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

////////////////////////////////////////////////////////////////
//                                                            //
//   Definitions appropriate for both pSOSim and pSOSystem    //
//                                                            //
////////////////////////////////////////////////////////////////

#if ! defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#if defined (__GNUG__)
# include "config-g++-common.h"
#endif /* __GNUG__ */

#define PDL_HAS_IP_MULTICAST

#define PDL_HAS_CPLUSPLUS_HEADERS

/* #define PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION */

// #define PDL_LACKS_U_LONG_LONG

#define PDL_LACKS_HRTIME_T

// #define PDL_LACKS_EVENT_T

#define PDL_HAS_VERBOSE_NOTSUP

#define PDL_LACKS_MEMORY_H

#define PDL_LACKS_MALLOC_H

#define PDL_LACKS_MMAP

#define PDL_LACKS_UNIX_DOMAIN_SOCKETS

#define PDL_HAS_NONSTATIC_OBJECT_MANAGER

#define PDL_LACKS_SEMBUF_T

#define PDL_LACKS_EXEC

#define PDL_LACKS_FORK


// rename the main entry point
#define PDL_MAIN extern "C" void root

// All this is commented out for the single threaded port
/*

#define PDL_HAS_THREADS

#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

#define PDL_DEFAULT_THREAD_KEYS 256

#define PDL_LACKS_COND_T


*/

#define PDL_HAS_TSS_EMULATION


////////////////////////////////////////////////////////////////
//                                                            //
//   Definitions appropriate for pSOSim but not pSOSystem     //
//                                                            //
////////////////////////////////////////////////////////////////

#define PDL_HAS_POSIX_TIME

////////////////////////////////////////////////////////////////
//                                                            //
//   Definitions appropriate for pSOSystem but not pSOSim     //
//                                                            //
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
//                                                            //
//   Definitions that have not been categorized               //
//                                                            //
////////////////////////////////////////////////////////////////


#define PDL_LACKS_PTHREAD_THR_SIGSETMASK

/* #define PDL_HAS_BROKEN_SENDMSG */

/* #define PDL_HAS_BROKEN_WRITEV  */

#define PDL_HAS_CHARPTR_SOCKOPT

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_HAS_MSG

#define PDL_HAS_POSIX_NONBLOCK

#define PDL_HAS_SIGINFO_T

#define PDL_HAS_SIGWAIT

#define PDL_HAS_SIG_ATOMIC_T

#define PDL_HAS_STRERROR

#define PDL_LACKS_ACCESS

#define PDL_LACKS_GETHOSTENT

#define PDL_LACKS_GETSERVBYNAME

#define PDL_LACKS_IOSTREAM_FX

#define PDL_LACKS_KEY_T

#define PDL_LACKS_LINEBUFFERED_STREAMBUF

#define PDL_LACKS_LONGLONG_T

#define PDL_LACKS_LSTAT

#define PDL_LACKS_MADVISE

#define PDL_LACKS_MKTEMP

#define PDL_LACKS_MPROTECT

#define PDL_LACKS_MSYNC

#define PDL_LACKS_PARAM_H

#define PDL_LACKS_PWD_FUNCTIONS

#define PDL_LACKS_READLINK

#define PDL_LACKS_RLIMIT

#define PDL_LACKS_RWLOCK_T

#define PDL_LACKS_SBRK

#define PDL_LACKS_SIGINFO_H

#define PDL_LACKS_SIGNED_CHAR

#define PDL_LACKS_SI_ADDR

#define PDL_LACKS_SOCKETPAIR

#define PDL_LACKS_STRCASECMP

#define PDL_LACKS_STRRECVFD

#define PDL_LACKS_SYSCALL

#define PDL_LACKS_SYSV_MSG_H

#define PDL_LACKS_SYSV_SHMEM

#define PDL_LACKS_SYS_NERR

#define PDL_LACKS_TIMESPEC_T

#define PDL_LACKS_UCONTEXT_H

#define PDL_LACKS_UNIX_SIGNALS

#define PDL_LACKS_UTSNAME_T

// #define PDL_LACKS_SYSTIME_H

#define PDL_PAGE_SIZE 4096

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#if !defined (PDL_PSOS)
#define PDL_PSOS
#endif /* PDL_PSOS */

#if !defined (PDL_PSOSIM)
#define PDL_PSOSIM
#endif /* PDL_PSOSIM */

#if !defined (PDL_PSOS_TBD)
#define PDL_PSOS_TBD
#endif /* PDL_PSOS_TBD */

// By default, don't include RCS Id strings in object code.
#if !defined (PDL_USE_RCSID)
#define PDL_USE_RCSID 0
#endif /* #if !defined (PDL_USE_RCSID) */
#define PDL_LACKS_MKFIFO

#define PDL_MALLOC_ALIGN 8
#endif /* PDL_CONFIG_H */
