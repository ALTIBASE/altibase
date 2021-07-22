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
// config-minimal.h,v 4.11 1999/05/19 11:36:39 levine Exp

// This configuration file is designed to build only the minimal
// PDL_OS adaptation layer.

#ifndef PDL_CONFIG_MINIMAL_H
#define PDL_CONFIG_MINIMAL_H

#define PDL_HAS_MINIMAL_PDL_OS

// Only instantiate the PDL_OS_Object_Manager.
#define PDL_MAIN_OBJECT_MANAGER \
  PDL_OS_Object_Manager pdl_os_object_manager;

#if !defined(PDL_USE_THREAD_MANAGER_ADAPTER)
  // To prevent use of PDL_Thread_Exit functions in
  // PDL_Thread_Adapter::invoke ().
# define PDL_USE_THREAD_MANAGER_ADAPTER
#endif /* ! PDL_USE_THREAD_MANAGER_ADAPTER */

#if defined (PDL_ASSERT)
# undef PDL_ASSERT
#endif /* PDL_ASSERT */
#define PDL_ASSERT(x)

#if defined (PDL_DEBUG)
# undef PDL_DEBUG
#endif /* PDL_DEBUG */
#define PDL_DEBUG(x)

#if defined (PDL_ERROR)
# undef PDL_ERROR
#endif /* PDL_ERROR */
#define PDL_ERROR(x)

// Added for removing interdependency between COMPONENTS of PDL
// by sjkim 2000.2.23
#define PDL_THREADS_DONT_INHERIT_LOG_MSG
#define PDL_NLOGGING

#endif /* PDL_CONFIG_MINIMAL_H */
