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
 
// -*- C++ -*-
// config-irix6.5.x-sgic++.h,v 1.1 2000/08/28 18:23:33 othman Exp

// Use this file for IRIX 6.5.x

#ifndef PDL_CONFIG_IRIX65X_H
#define PDL_CONFIG_IRIX65X_H

// Include IRIX 6.[234] configuration
#include "config-irix6.x-sgic++.h"

// Irix 6.5 man pages show that they exist
#undef PDL_LACKS_CONDATTR_PSHARED
#undef PDL_LACKS_MUTEXATTR_PSHARED

#endif /* PDL_CONFIG_IRIX65X_H */
