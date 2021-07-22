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
// inc_user_config.h,v 4.5 1998/10/20 02:34:14 levine Exp

// Use this file to include config.h instead of including
// config.h directly.

#ifndef PDL_INC_USER_CONFIG_H
#define PDL_INC_USER_CONFIG_H

#include "config.h"

#if !defined (PDL_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* PDL_LACKS_PRAGMA_ONCE */

// By default, DO include RCS Id strings in object code.
#if ! defined (PDL_USE_RCSID)
#  define PDL_USE_RCSID 1
#endif /* #if ! defined (PDL_USE_RCSID) */

#if (defined (PDL_USE_RCSID) && (PDL_USE_RCSID != 0))
#  if ! defined (PDL_RCSID)

   // This hack has the following purposes:
   // 1. To define the RCS id string variable as a static char*, so
   //    that there won't be any duplicate extern symbols at link
   //    time.
   // 2. To have a RCS id string variable with a unique name for each
   //    file.
   // 3. To avoid warnings of the type "variable declared and never
   //    used".

#    define PDL_RCSID(path, file, id) \
      inline const char* get_rcsid_ ## path ## _ ## file (const char*) \
      { \
        return id ; \
      } \
      static const char* rcsid_ ## path ## _ ## file = \
        get_rcsid_ ## path ## _ ## file ( rcsid_ ## path ## _ ## file ) ;

#  endif /* #if ! defined (PDL_RCSID) */
#else

   // RCS id strings are not wanted.
#  if defined (PDL_RCSID)
#    undef PDL_RCSID
#  endif /* #if defined (PDL_RCSID) */
#  define PDL_RCSID(path, file, id) /* noop */
#endif /* #if (defined (PDL_USE_RCSID) && (PDL_USE_RCSID != 0)) */

#endif /* PDL_INC_USER_CONFIG_H */
