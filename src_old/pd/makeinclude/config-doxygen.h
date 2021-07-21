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
 
/**
 * This is a configuration file to define all the macros that Doxygen
 * needs
 *
 * @file config-doxygen.h
 *
 * config-doxygen.h,v 1.7 2002/04/23 05:37:59 jwillemsen Exp
 *
 * @author Carlos O'Ryan <coryan@uci.edu>
 * @author Darrell Brunsch <brunsch@uci.edu>
 *
 */
#ifndef PDL_CONFIG_DOXYGEN_H
#define PDL_CONFIG_DOXYGEN_H

/// Make the wchar_t interfaces available.
#define PDL_HAS_WCHAR

/// Make all the emulation versions of string operations visible
// #define PDL_LACKS_WCSTOK
#define PDL_LACKS_ITOW
#define PDL_LACKS_STRCASECMP
#define PDL_LACKS_STRCSPN
#define PDL_LACKS_STRCHR
#define PDL_LACKS_STRRCHR
#define PDL_LACKS_WCSCAT
#define PDL_LACKS_WCSCHR
#define PDL_LACKS_WCSCMP
#define PDL_LACKS_WCSCPY
#define PDL_LACKS_WCSICMP
#define PDL_LACKS_WCSLEN
#define PDL_LACKS_WCSNCAT
#define PDL_LACKS_WCSNCMP
#define PDL_LACKS_WCSNCPY
#define PDL_LACKS_WCSNICMP
#define PDL_LACKS_WCSPBRK
#define PDL_LACKS_WCSRCHR
#define PDL_LACKS_WCSCSPN
#define PDL_LACKS_WCSSPN
#define PDL_LACKS_WCSSTR

/// Support for threads enables several important classes
#define PDL_HAS_THREADS

/// Support for Win32 enables the WFMO_Reactor and several Async I/O
/// classes
#define PDL_WIN32

/// Enable support for POSIX Asynchronous I/O calls
#define PDL_HAS_AIO_CALLS

/// Enable support for TLI interfaces
#define PDL_HAS_TLI

/// Enable support for the SSL wrappers
#define PDL_HAS_SSL 1

/// Several GUI Reactors that are only enabled in some platforms.
#define PDL_HAS_XT
#define PDL_HAS_FL
#define PDL_HAS_QT
#define PDL_HAS_TK
#define PDL_HAS_GTK

/// Enable exceptions
#define PDL_HAS_EXCEPTIONS

/// Enable timeprobes
#define PDL_COMPILE_TIMEPROBES

/// Enable unicode to generate PDL_Registry_Name_Space
#define UNICODE

/// These defines make sure that Svc_Conf_y.cpp and Svc_Conf_l.cpp are correctly
/// parsed
#define __cplusplus
#define PDL_YY_USE_PROTOS

/// TAO features that should be documented too
#define TAO_HAS_RT_CORBA 1
#define TAO_HAS_MINIMUM_CORBA 0
#define TAO_HAS_AMI 1
#define TAO_HAS_INTERCEPTORS 1

#endif /* PDL_CONFIG_DOXYGEN_H */
