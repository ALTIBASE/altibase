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
// config-irix6.x-sgic++.h,v 4.20 1999/06/05 22:04:17 coryan Exp

// Use this file for IRIX 6.[234] if you have the pthreads patches
// installed.

#ifndef PDL_CONFIG_IRIX6X_H
#define PDL_CONFIG_IRIX6X_H

// Include basic (non-threaded) configuration
#include "config-irix6.x-sgic++-nothreads.h"

// Add threading support

#define PDL_HAS_IRIX62_THREADS
#define PDL_HAS_UALARM

// Needed for the threading stuff?
#include /**/ <task.h>
#define PTHREAD_MIN_PRIORITY PX_PRIO_MIN
#define PTHREAD_MAX_PRIORITY PX_PRIO_MAX

// PDL supports threads.
#define PDL_HAS_THREADS

// Platform has no implementation of pthread_condattr_setpshared(),
// even though it supports pthreads! (like Irix 6.2)
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_MUTEXATTR_PSHARED

// IRIX 6.2 supports a variant of POSIX Pthreads, supposedly POSIX 1c
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_STD

// Compiler/platform has thread-specific storage
#define PDL_HAS_THREAD_SPECIFIC_STORAGE

// The pthread_cond_timedwait call does not reset the timer.
#define PDL_LACKS_COND_TIMEDWAIT_RESET 1

// Scheduling functions are declared in <sched.h>
#define PDL_NEEDS_SCHED_H

// When threads are enabled READDIR_R is supported on IRIX.
#undef PDL_LACKS_READDIR_R

// Compile using multi-thread libraries
#if !defined (PDL_MT_SAFE)
  #define PDL_MT_SAFE 1
#endif /* PDL_MT_SAFE */

#endif /* PDL_CONFIG_IRIX6X_H */
