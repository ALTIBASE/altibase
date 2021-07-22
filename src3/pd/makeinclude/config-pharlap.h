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
// config-pharlap.h,v 1.6 1999/12/17 19:31:41 shuston Exp

// This configuration file is for use with the PharLap Realtime ETS Kernel.
// It has been tested with PharLap TNT Embedded ToolSuite version 9.1.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#define PDL_HAS_PHARLAP
// Some features are only available with the Realtime edition of ETS.
// Assume that if using PDL, the realtime version is also being used, but
// allow it to be turned off as well.
#ifndef PDL_HAS_PHARLAP_RT
#  define PDL_HAS_PHARLAP_RT
#else
#  if (PDL_HAS_PHARLAP_RT == 0)
#    undef PDL_HAS_PHARLAP_RT
#  endif
#endif

// Fortunately, PharLap ETS offers much of the Win32 API. But it's still on
// WinNT 3.5, Winsock 1.1
#define PDL_HAS_WINNT4 0
#define PDL_HAS_WINSOCK2 0

// The TSS implementation doesn't pass muster on the TSS_Test, but it works
// well with PDL's TSS emulation.
#define PDL_HAS_TSS_EMULATION

#define PDL_LACKS_MMAP
#define PDL_LACKS_MPROTECT
#define PDL_LACKS_MSYNC
#define PDL_LACKS_TCP_NODELAY

// There's no host table, by default. So using "localhost" won't work.
// If your system does have the ability to use "localhost" and you want to,
// define it before including this file.
#if !defined (PDL_LOCALHOST)
# define PDL_LOCALHOST "127.0.0.1"
#endif /* PDL_LOCALHOST */

// Don't know how to get the page size at execution time. This is most likely
// the correct value.
#define PDL_PAGE_SIZE 4096

// Maximum compensation (10 ms) for early return from timed ::select ().
#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

// Let the config-win32.h file do its thing
#undef PDL_CONFIG_H
#include /**/ "config-win32.h"
#include /**/ <embkern.h>
#if defined (PDL_HAS_PHARLAP_RT)
# include /**/ <embtcpip.h>
#endif /* PDL_HAS_PHARLAP_RT */

#endif /* PDL_CONFIG_H */
