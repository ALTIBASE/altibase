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
 
// config-fsu-pthread.h,v 4.13 1998/10/17 01:55:49 schmidt Exp

#ifndef PDL_CONFIG_FSU_PTHREAD_H
#define PDL_CONFIG_FSU_PTHREAD_H

#define PDL_LACKS_CONST_TIMESPEC_PTR

// Threads
#define PDL_HAS_THREADS
#if !defined(PDL_MT_SAFE)
#define PDL_MT_SAFE 1
#endif

/*
** FSU implements 1003.4a draft 6 threads - the PDL_HAS_FSU_PTHREADS def
** is needed for some peculiarities with this particular implementation.
*/
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_DRAFT6
#define PDL_HAS_FSU_PTHREADS
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_SIGWAIT
#define PDL_HAS_PTHREAD_YIELD_VOID_PTR
#define PDL_HAS_PTHREAD_ATTR_INIT
#define PDL_HAS_PTHREAD_ATTR_DESTROY
#define PDL_LACKS_THREAD_STACK_ADDR
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SETSCHED
#if defined(M_UNIX)
#define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS
#endif

#if !defined(PDL_HAS_POSIX_TIME)
#define PDL_HAS_POSIX_TIME
#define PDL_LACKS_TIMESPEC_T
#endif

#include <pthread.h>

#if !defined(PTHREAD_STACK_MIN)
#define PTHREAD_STACK_MIN (1024*10)
#endif

#define PDL_LACKS_THREAD_PROCESS_SCOPING

#undef PTHREAD_INHERIT_SCHED

struct sched_param
{
  int sched_priority;
  int prio;
};

#endif /* PDL_CONFIG_FSU_PTHREAD_H */
