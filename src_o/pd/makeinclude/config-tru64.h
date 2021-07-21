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
// config-tru64.h,v 1.5 1999/07/15 15:30:41 levine Exp

// The following configuration file is designed to work for the
// Digital UNIX V4.0a and later platforms.  It relies on
// config-osf1-4.0.h, and adds deltas for newer platforms.

#ifndef PDL_CONFIG_TRU64_H
#define PDL_CONFIG_TRU64_H

#if defined (DIGITAL_UNIX)
#  include "config-osf1-4.0.h"
#  if DIGITAL_UNIX >= 0x40F
#    define PDL_LACKS_SYSTIME_H
#  endif /* DIGITAL_UNIX >= 0x40F */
#  if DIGITAL_UNIX >= 0x500
#    define _LIBC_POLLUTION_H_
#  endif /* DIGITAL_UNIX >= 0x500 */
#else  /* ! DIGITAL_UNIX */
#  include "config-osf1-3.2.h"
#endif /* ! DIGITAL_UNIX */

#if defined PDL_LACKS_THREAD_PROCESS_SCOPING
#undef PDL_LACKS_THREAD_PROCESS_SCOPING
#endif

#endif /* PDL_CONFIG_TRU64_H */
