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
 
// config-mit-pthread.h,v 4.12 1998/10/17 01:55:51 schmidt Exp

#ifndef PDL_CONFIG_MIT_PTHREAD_H
#define PDL_CONFIG_MIT_PTHREAD_H

// Platform uses int for select() rather than fd_set.
#if !defined(PDL_HAS_SELECT_H)
#define PDL_HAS_SELECT_H
#endif

// Threads
#define PDL_HAS_THREADS
#if !defined (PDL_MT_SAFE)
        #define PDL_MT_SAFE 1
#endif
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_STD
#define PDL_LACKS_PTHREAD_CANCEL
#define PDL_HAS_PTHREAD_SIGMASK
#define PDL_HAS_SIGWAIT
//#define       PDL_HAS_PTHREAD_YIELD_VOID_PTR
//#define       PDL_HAS_PTHREAD_ATTR_INIT
//#define       PDL_HAS_PTHREAD_ATTR_DESTROY
#define PDL_LACKS_THREAD_PROCESS_SCOPING
//#define PDL_LACKS_THREAD_STACK_ADDR
// If PDL doesn't compile due to the lack of these methods, please
// send email to schmidt@cs.wustl.edu reporting this.
// #define PDL_LACKS_CONDATTR_PSHARED
// #define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SETSCHED

#include <pthread.h>
#if defined(_M_UNIX)
#include <sys/regset.h>
#endif

#define PDL_LACKS_TIMEDWAIT_PROTOTYPES
#define PDL_HAS_RECV_TIMEDWAIT
#define PDL_HAS_RECVFROM_TIMEDWAIT
#define PDL_HAS_RECVMSG_TIMEDWAIT
#define PDL_HAS_SEND_TIMEDWAIT
#define PDL_HAS_SENDTO_TIMEDWAIT
#define PDL_HAS_SENDMSG_TIMEDWAIT
#define PDL_HAS_READ_TIMEDWAIT
#define PDL_HAS_READV_TIMEDWAIT
#define PDL_HAS_WRITE_TIMEDWAIT
#define PDL_HAS_WRITEV_TIMEDWAIT

#endif /* PDL_CONFIG_MIT_PTHREAD_H */
