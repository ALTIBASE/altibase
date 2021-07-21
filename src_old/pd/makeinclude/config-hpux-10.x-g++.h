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
// config-hpux-10.x-g++.h,v 4.22 1999/08/30 02:52:49 levine Exp

// The following configuration file is designed to work for HP
// platforms running HP/UX 10.x using G++.

//   #ifndef PDL_CONFIG_H
//   #define PDL_CONFIG_H

// config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
// this must appear before its #include.
#define PDL_HAS_STRING_CLASS

#include "config-g++-common.h"

// These are apparantly some things which are special to g++ on HP?  They are
// compiler-related settings, but not in config-g++-common.h

#define PDL_HAS_BROKEN_CONVERSIONS
// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T
#define _CLOCKID_T

//  #include "config-hpux-10.x.h"

//  #endif /* PDL_CONFIG_H */
