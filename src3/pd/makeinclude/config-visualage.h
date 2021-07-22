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
// config-visualage.h,v 4.1 1999/06/28 03:58:49 schmidt Exp

// This configuration file automatically includes the proper
// configurations for IBM's VisualAge C++ compiler on Win32 and AIX.

#ifdef __TOS_WIN__
   #include "config-win32.h"
#elif __TOS_AIX__
   #include "config-aix-4.x.h"
#else
   #include "PLATFORM NOT SPECIFIED"
#endif /* __TOS_WIN__ */
