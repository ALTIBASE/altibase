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
// config-linux-lxpthreads.h,v 4.49 1999/08/29 21:43:54 schmidt Exp

/* The following configuration file is designed to work for Linux
   platforms using GNU C++ and Xavier Leroy's pthreads package.  For
   more information you should check out his Web site:

   http://pauillac.inria.fr/~xleroy/linuxthreads/

   The version I have installed and working is an RPM*
   based on Xavier's 0.5 release. I don't know where
   the tarball of 0.5 can be found, but I suspect that
   Xavier's site has it...

        * RPM == Redhat Package Management

        My system is a Caldera-based distribution with many upgraded
        packages.  If you don't use RPM, there is a program (rpm2cpio)
        which will extract the files for "normal consumption".

        You may also want to check out the "PDL On Linux" pages at:

                http://users.deltanet.com/users/slg/PDL/

        (They were a little out of date when I last was there
        however.) */

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// added by gamestar 
#include "config-linux-common.h"

#undef PDL_LACKS_PREAD_PROTOTYPE //pwrite & pread compile-error

#define PDL_HAS_SVR4_DYNAMIC_LINKING
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Yes, we do have threads.
#define PDL_HAS_THREADS
// And they're even POSIX pthreads (MIT implementation)
#define PDL_HAS_PTHREADS
// ... and the final standard even!
#define PDL_HAS_PTHREADS_STD

#if !defined (PDL_MT_SAFE)
        #define PDL_MT_SAFE 1                           // JCEJ 12/22/96        #1
#endif
#define PDL_HAS_THREAD_SPECIFIC_STORAGE         // jcej 12/22/96        #2
#define PTHREAD_MIN_PRIORITY            0       // JCEJ 12/22/96        #3
#if !defined(PDL_LACKS_PTHREAD_SIGMASK)
#  define PTHREAD_MAX_PRIORITY          99      // CJC  02/11/97
#else
#  define PTHREAD_MAX_PRIORITY          32      // JCEJ 12/22/96        #3
#endif

#define PDL_LACKS_THREAD_STACK_ADDR             // JCEJ 12/17/96
// CASE 1832 khshim
//#define PDL_LACKS_THREAD_STACK_SIZE             // JCEJ 12/17/96

#define PDL_LACKS_RWLOCK_T                      // JCEJ 12/23/96        #1
#define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS    // JCEJ 1/7-8/96

#if defined(__GLIBC__)
// Platform supports reentrant functions (i.e., all the POSIX *_r
// functions).
#define PDL_HAS_REENTRANT_FUNCTIONS
// getprotobyname_r have a different signature!
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
// uses ctime_r & asctime_r with only two parameters vs. three
#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
#endif

#include /**/ <pthread.h>

// BUGBUG - ANDROID 
struct semid_ds
{
  /* empty */
};

union semun
{
  int val;              // value for SETVAL
  struct semid_ds *buf; // buffer for IPC_STAT & IPC_SET
  u_short *array;       // array for GETALL & SETALL
};

typedef long fd_mask;

#define howmany(x, y) (((x)+((y)-1))/(y))
#define _BYTE_ORDER LITTLE_ENDIAN

inline int pthread_condattr_init(pthread_condattr_t *) { return 0; }
inline int pthread_condattr_destroy(pthread_condattr_t *) { return 0; }
inline int pthread_setcancelstate(int, int *) { return 0; }
inline int pthread_setcanceltype(int, int *) { return 0; }
inline void pthread_yield() {}
inline void tzset() {}

#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_PTHREAD_CANCEL
#define PDL_LACKS_PTHREAD_SIGMASK
#define PDL_LACKS_TELLDIR
#define PDL_LACKS_SEEKDIR 
#define PDL_LACKS_TRUNCATE

#define PDL_LACKS_UCONTEXT_H
#define PDL_LACKS_SYSV_MSG_H
#define PDL_HAS_SIGINFO_T
#define PDL_LACKS_SIGINFO_H
#define PDL_LACKS_SETSCHED
#define PDL_LACKS_PWD_FUNCTIONS

#undef PDL_HAS_BYTESEX_H
#undef PDL_HAS_SYSV_IPC

#define SMALL_FOOTPRINT

#endif /* PDL_CONFIG_H */
