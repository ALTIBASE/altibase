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
// config-sunos5.x-sunc++-4.x-orbix.h,v 4.3 1999/04/13 13:53:38 shuston Exp

// The following configuration file is designed to work for SunOS
// 5.[56] platforms using the SunC++ 4.x compiler. This works with the
// Orbix 2.x CORBA IDL compiler.

#ifndef PDL_CONFIG_ORBIX_H
#define PDL_CONFIG_ORBIX_H

// Pick up correct platform definition file based on compiler's predefined
// macros.
#if defined (__SunOS_5_5) || defined (__SunOS_5_5_1)
#  include "config-sunos5.5.h"
#elif defined (__SunOS_5_6)
#  include "config-sunos5.6.h"
#else
#  error "Need a new platform extension in config-sunos5.x-sunc++-4.x-orbix.h"
#endif /* __SunOS_* */

// Platform contains the Orbix CORBA implementation.
#define PDL_HAS_ORBIX 1

// Platform contains the multi-threaded Orbix CORBA implementation, unless
// overridden by site config.
#if !defined (PDL_HAS_MT_ORBIX)
#  define PDL_HAS_MT_ORBIX 1
#endif /* PDL_HAS_MT_ORBIX */

#endif /* PDL_CONFIG_ORBIX_H */
