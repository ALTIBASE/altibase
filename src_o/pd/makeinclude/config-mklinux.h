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
// config-mklinux.h,v 4.3 1999/01/28 14:55:01 levine Exp

// The following configuration file is designed to work for MkLinux
// platforms using GNU C++.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#include "config-linux-common.h"

#define PDL_HAS_SVR4_DYNAMIC_LINKING
#define PDL_HAS_AUTOMATIC_INIT_FINI

#undef PDL_HAS_SOCKLEN_T
#define PDL_HAS_SIZET_SOCKET_LEN

#endif /* PDL_CONFIG_H */
