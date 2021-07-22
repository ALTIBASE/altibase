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
// config-linux-pthread.h,v 4.29 1999/08/29 21:43:54 schmidt Exp

// The following configuration file is designed to work for Linux
// platforms using GNU C++ and the MIT threads package.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#include "config-linux-common.h"

#define PDL_HAS_THREADS
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif
// And they're even POSIX pthreads (MIT implementation)
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_STD
#define PDL_LACKS_RWLOCK_T
// If PDL doesn't compile due to the lack of these methods, please
// send email to schmidt@cs.wustl.edu reporting this.
// #define PDL_LACKS_CONDATTR_PSHARED
// #define PDL_LACKS_MUTEXATTR_PSHARED

// To use pthreads on Linux you'll need to use the MIT version, for
// now...
#define _MIT_POSIX_THREADS 1
#include /**/ <pthread/mit/pthread.h>

#endif /* PDL_CONFIG_H */
