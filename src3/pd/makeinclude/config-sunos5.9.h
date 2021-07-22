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
// config-sunos5.9.h,v 1.1 2002/10/09 01:19:16 shuston Exp

// The following configuration file is designed to work for SunOS 5.9
// (Solaris 9) platforms using the SunC++ 5.x (Forte 6 and 7), or g++
// compilers.

#ifndef PDL_CONFIG_H

// PDL_CONFIG_H is defined by one of the following #included headers.

// #include the SunOS 5.8 config, then add any SunOS 5.9 updates below.
#include "config-sunos5.8.h"

#if !defined (PDL_HAS_IP6)
#define PDL_HAS_IP6 // Platform supports IPv6
#endif

#endif /* PDL_CONFIG_H */
