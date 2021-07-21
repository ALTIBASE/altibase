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
 
// config-aix6.1.h,v 1.2 2002/01/25 23:32:39 shuston Exp
//
// Config file for AIX 6.1

#ifndef PDL_AIX_VERS
# define PDL_AIX_VERS 601
#endif

#include "config-aix-4.x.h"

// Platform supports IPv6
#define PDL_HAS_IP6

// I think this is correct, but needs to be verified...   -Steve Huston
#define PDL_HAS_SIGTIMEDWAIT
