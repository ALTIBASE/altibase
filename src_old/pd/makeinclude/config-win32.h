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
// config-win32.h,v 4.52 2000/01/15 07:52:26 schmidt Exp

// The following configuration file is designed to work for Windows
// 9x, Windows NT 3.51, and Windows NT 4.0 platforms and supports a
// variety of compilers.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if defined (PDL_HAS_WINCE) || defined (UNDER_CE)
# include "config-WinCE.h"
#endif /* PDL_HAS_WINCE */

#if defined (_MSC_VER)
# include "config-win32-msvc.h"
#elif defined (__BORLANDC__)
# include "config-win32-borland.h"
#elif defined (__IBMCPP__)
# include "config-win32-visualage.h"
#else
# error "Compiler does not seem to be supported"
#endif /* _MSC_VER */

#endif /* PDL_CONFIG_H */
