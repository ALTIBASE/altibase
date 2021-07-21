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
// config-irix6.x-kcc.h,v 4.4 1999/05/07 02:53:01 othman Exp

// The following configuration file is designed to work for the SGI
// Indigo2EX running Irix 6.5 platform using the KAI C++ compiler.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#include "config-kcc-common.h"


// Platform supports the very odd IRIX 6.2 threads...
#define PDL_HAS_THREADS
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif /* !PDL_MT_SAFE */
#define PDL_HAS_IRIX62_THREADS

// IRIX 6.2 supports a variant of POSIX Pthreads, supposedly POSIX 1c
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_STD

#include "config-irix6.x-common.h"

// Needed for the threading stuff?
#include /**/ <sched.h>
#include /**/ <task.h>

#define PTHREAD_MIN_PRIORITY PX_PRIO_MIN
#define PTHREAD_MAX_PRIORITY PX_PRIO_MAX

// Compiler/platform has thread-specific storage
#define PDL_HAS_THREAD_SPECIFIC_STORAGE

// The pthread_cond_timedwait call does not reset the timer.
#define PDL_LACKS_COND_TIMEDWAIT_RESET 1

#define PDL_HAS_STRING_CLASS
#ifndef IRIX6
# define IRIX6
#endif

// Denotes that GNU has cstring.h as standard
// which redefines memchr()
#define PDL_HAS_GNU_CSTRING_H

// Compiler/platform supports SVR4 signal typedef.
#define PDL_HAS_IRIX_53_SIGNALS

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Compiler/platform supports sys_siglist array.
//#define PDL_HAS_SYS_SIGLIST


#endif /* PDL_CONFIG_H */
