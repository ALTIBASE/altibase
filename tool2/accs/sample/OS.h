/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
// -*- C++ -*-
// OS.h,v 4.934 2000/02/18 17:36:19 coryan Exp

// ============================================================================
//
// = LIBRARY
//   pdl
//
// = FILENAME
//   OS.h
//
// = AUTHOR
//   Doug Schmidt <schmidt@cs.wustl.edu>, Jesper S. M|ller
//   <stophph@diku.dk>, and a cast of thousands...
//
// ============================================================================

#ifndef PDL_OS_H
# define PDL_OS_H

// This file should be a link to the platform/compiler-specific
// configuration file (e.g., config-sunos5-sunc++-4.x.h).
# include "pdl/inc_user_config.h"

# if !defined (PDL_LACKS_PRAGMA_ONCE)
#   pragma once
# endif /* PDL_LACKS_PRAGMA_ONCE */

# if !defined (PDL_MALLOC_ALIGN)
#   define PDL_MALLOC_ALIGN ((int) sizeof (long))
# endif /* PDL_MALLOC_ALIGN */

# if !defined (PDL_HAS_POSITION_INDEPENDENT_POINTERS)
#   define PDL_HAS_POSITION_INDEPENDENT_POINTERS 1
# endif /* PDL_HAS_POSITION_INDEPENDENT_POINTERS */

// States of a recyclable object.
enum PDL_Recyclable_State
{
  PDL_RECYCLABLE_IDLE_AND_PURGABLE,
  // Idle and can be purged.

  PDL_RECYCLABLE_IDLE_BUT_NOT_PURGABLE,
  // Idle but cannot be purged.

  PDL_RECYCLABLE_PURGABLE_BUT_NOT_IDLE,
  // Can be purged, but is not idle (mostly for debugging).

  PDL_RECYCLABLE_BUSY,
  // Busy (i.e., cannot be recycled or purged).

  PDL_RECYCLABLE_CLOSED,
  // Closed.

  PDL_RECYCLABLE_UNKNOWN
  // Unknown state.
};

#if !defined (PDL_DEFAULT_PAGEFILE_POOL_BASE)
#define PDL_DEFAULT_PAGEFILE_POOL_BASE (void *) 0
#endif /* PDL_DEFAULT_PAGEFILE_POOL_BASE */

#if !defined (PDL_DEFAULT_PAGEFILE_POOL_SIZE)
#define PDL_DEFAULT_PAGEFILE_POOL_SIZE (size_t) 0x01000000
#endif /* PDL_DEFAULT_PAGEFILE_POOL_SIZE */

#if !defined (PDL_DEFAULT_PAGEFILE_POOL_CHUNK)
#define PDL_DEFAULT_PAGEFILE_POOL_CHUNK (size_t) 0x00010000
#endif /* PDL_DEFAULT_PAGEFILE_POOL_CHUNK */

#if !defined (PDL_DEFAULT_PAGEFILE_POOL_NAME)
#define PDL_DEFAULT_PAGEFILE_POOL_NAME PDL_TEXT ("Default_PDL_Pagefile_Memory_Pool")
#endif /* PDL_DEFAULT_PAGEFILE_POOL_NAME */

#if !defined (PDL_DEFAULT_MESSAGE_BLOCK_PRIORITY)
#define PDL_DEFAULT_MESSAGE_BLOCK_PRIORITY 0
#endif /* PDL_DEFAULT_MESSAGE_BLOCK_PRIORITY */

#if !defined (PDL_DEFAULT_SERVICE_REPOSITORY_SIZE)
#define PDL_DEFAULT_SERVICE_REPOSITORY_SIZE 1024
#endif /* PDL_DEFAULT_SERVICE_REPOSITORY_SIZE */

#if !defined (PDL_REACTOR_NOTIFICATION_ARRAY_SIZE)
#define PDL_REACTOR_NOTIFICATION_ARRAY_SIZE 1024
#endif /* PDL_REACTOR_NOTIFICATION_ARRAY_SIZE */

// Do not change these values wantonly since GPERF depends on them..
#define PDL_ASCII_SIZE 128
#define PDL_EBCDIC_SIZE 256

#if 'a' < 'A'
#define PDL_HAS_EBCDIC
#define PDL_STANDARD_CHARACTER_SET_SIZE 256
#else
#define PDL_HAS_ASCII
#define PDL_STANDARD_CHARACTER_SET_SIZE 128
#endif /* 'a' < 'A' */

// Get OS.h to compile on some of the platforms without DIR info yet.
# if !defined (PDL_HAS_DIRENT)
typedef int DIR;
struct dirent {};
# endif /* PDL_HAS_DIRENT */

# if defined (PDL_PSOS_TM)
typedef long long longlong_t;
typedef long      id_t;
# endif /* PDL_PSOS_TM */

# if defined (PDL_HAS_MOSTLY_UNICODE_APIS) && !defined (UNICODE)
#   error UNICODE must be defined when using PDL_HAS_MOSTLY_UNICODE_APIS, check your compiler document on how to enable UNICODE.
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS && !UNICODE */

# if defined (PDL_LACKS_INLINE_FUNCTIONS) && !defined (PDL_NO_INLINE)
#   define PDL_NO_INLINE
# endif /* defined (PDL_LACKS_INLINE_FUNCTIONS) && !defined (PDL_NO_INLINE) */

# if defined (PDL_HAS_ANSI_CASTS)

#   define PDL_sap_any_cast(TYPE)                                      reinterpret_cast<TYPE> (const_cast<PDL_Addr &> (PDL_Addr::sap_any))

#   define PDL_static_cast(TYPE, EXPR)                                 static_cast<TYPE> (EXPR)
#   define PDL_static_cast_1_ptr(TYPE, T1, EXPR)                       static_cast<TYPE<T1> *> (EXPR)
#   define PDL_static_cast_2_ptr(TYPE, T1, T2, EXPR)                   static_cast<TYPE<T1, T2> *> (EXPR)
#   define PDL_static_cast_3_ptr(TYPE, T1, T2, T3, EXPR)               static_cast<TYPE<T1, T2, T3> *> (EXPR)
#   define PDL_static_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)           static_cast<TYPE<T1, T2, T3, T4> *> (EXPR)
#   define PDL_static_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)       static_cast<TYPE<T1, T2, T3, T4, T5> *> (EXPR)
#   define PDL_static_cast_1_ref(TYPE, T1, EXPR)                       static_cast<TYPE<T1> &> (EXPR)
#   define PDL_static_cast_2_ref(TYPE, T1, T2, EXPR)                   static_cast<TYPE<T1, T2> &> (EXPR)
#   define PDL_static_cast_3_ref(TYPE, T1, T2, T3, EXPR)               static_cast<TYPE<T1, T2, T3> &> (EXPR)
#   define PDL_static_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)           static_cast<TYPE<T1, T2, T3, T4> &> (EXPR)
#   define PDL_static_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)       static_cast<TYPE<T1, T2, T3, T4, T5> &> (EXPR)

#   define PDL_const_cast(TYPE, EXPR)                                  const_cast<TYPE> (EXPR)
#   define PDL_const_cast_1_ptr(TYPE, T1, EXPR)                        const_cast<TYPE<T1> *> (EXPR)
#   define PDL_const_cast_2_ptr(TYPE, T1, T2, EXPR)                    const_cast<TYPE<T1, T2> *> (EXPR)
#   define PDL_const_cast_3_ptr(TYPE, T1, T2, T3, EXPR)                const_cast<TYPE<T1, T2, T3> *> (EXPR)
#   define PDL_const_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)            const_cast<TYPE<T1, T2, T3, T4> *> (EXPR)
#   define PDL_const_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)        const_cast<TYPE<T1, T2, T3, T4, T5> *> (EXPR)
#   define PDL_const_cast_1_ref(TYPE, T1, EXPR)                        const_cast<TYPE<T1> &> (EXPR)
#   define PDL_const_cast_2_ref(TYPE, T1, T2, EXPR)                    const_cast<TYPE<T1, T2> &> (EXPR)
#   define PDL_const_cast_3_ref(TYPE, T1, T2, T3, EXPR)                const_cast<TYPE<T1, T2, T3> &> (EXPR)
#   define PDL_const_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)            const_cast<TYPE<T1, T2, T3, T4> &> (EXPR)
#   define PDL_const_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)        const_cast<TYPE<T1, T2, T3, T4, T5> &> (EXPR)

#   define PDL_reinterpret_cast(TYPE, EXPR)                            reinterpret_cast<TYPE> (EXPR)
#   define PDL_reinterpret_cast_1_ptr(TYPE, T1, EXPR)                  reinterpret_cast<TYPE<T1> *> (EXPR)
#   define PDL_reinterpret_cast_2_ptr(TYPE, T1, T2, EXPR)              reinterpret_cast<TYPE<T1, T2> *> (EXPR)
#   define PDL_reinterpret_cast_3_ptr(TYPE, T1, T2, T3, EXPR)          reinterpret_cast<TYPE<T1, T2, T3> *> (EXPR)
#   define PDL_reinterpret_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)      reinterpret_cast<TYPE<T1, T2, T3, T4> *> (EXPR)
#   define PDL_reinterpret_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)  reinterpret_cast<TYPE<T1, T2, T3, T4, T5> *> (EXPR)
#   define PDL_reinterpret_cast_1_ref(TYPE, T1, EXPR)                  reinterpret_cast<TYPE<T1> &> (EXPR)
#   define PDL_reinterpret_cast_2_ref(TYPE, T1, T2, EXPR)              reinterpret_cast<TYPE<T1, T2> &> (EXPR)
#   define PDL_reinterpret_cast_3_ref(TYPE, T1, T2, T3, EXPR)          reinterpret_cast<TYPE<T1, T2, T3> &> (EXPR)
#   define PDL_reinterpret_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)      reinterpret_cast<TYPE<T1, T2, T3, T4> &> (EXPR)
#   define PDL_reinterpret_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)  reinterpret_cast<TYPE<T1, T2, T3, T4, T5> &> (EXPR)

#   if defined (PDL_LACKS_RTTI)
#     define PDL_dynamic_cast(TYPE, EXPR)                              static_cast<TYPE> (EXPR)
#     define PDL_dynamic_cast_1_ptr(TYPE, T1, EXPR)                    static_cast<TYPE<T1> *> (EXPR)
#     define PDL_dynamic_cast_2_ptr(TYPE, T1, T2, EXPR)                static_cast<TYPE<T1, T2> *> (EXPR)
#     define PDL_dynamic_cast_3_ptr(TYPE, T1, T2, T3, EXPR)            static_cast<TYPE<T1, T2, T3> *> (EXPR)
#     define PDL_dynamic_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)        static_cast<TYPE<T1, T2, T3, T4> *> (EXPR)
#     define PDL_dynamic_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)    static_cast<TYPE<T1, T2, T3, T4, T5> *> (EXPR)
#     define PDL_dynamic_cast_1_ref(TYPE, T1, EXPR)                    static_cast<TYPE<T1> &> (EXPR)
#     define PDL_dynamic_cast_2_ref(TYPE, T1, T2, EXPR)                static_cast<TYPE<T1, T2> &> (EXPR)
#     define PDL_dynamic_cast_3_ref(TYPE, T1, T2, T3, EXPR)            static_cast<TYPE<T1, T2, T3> &> (EXPR)
#     define PDL_dynamic_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)        static_cast<TYPE<T1, T2, T3, T4> &> (EXPR)
#     define PDL_dynamic_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)    static_cast<TYPE<T1, T2, T3, T4, T5> &> (EXPR)
#   else  /* ! PDL_LACKS_RTTI */
#     define PDL_dynamic_cast(TYPE, EXPR)                              dynamic_cast<TYPE> (EXPR)
#     define PDL_dynamic_cast_1_ptr(TYPE, T1, EXPR)                    dynamic_cast<TYPE<T1> *> (EXPR)
#     define PDL_dynamic_cast_2_ptr(TYPE, T1, T2, EXPR)                dynamic_cast<TYPE<T1, T2> *> (EXPR)
#     define PDL_dynamic_cast_3_ptr(TYPE, T1, T2, T3, EXPR)            dynamic_cast<TYPE<T1, T2, T3> *> (EXPR)
#     define PDL_dynamic_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)        dynamic_cast<TYPE<T1, T2, T3, T4> *> (EXPR)
#     define PDL_dynamic_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)    dynamic_cast<TYPE<T1, T2, T3, T4, T5> *> (EXPR)
#     define PDL_dynamic_cast_1_ref(TYPE, T1, EXPR)                    dynamic_cast<TYPE<T1> &> (EXPR)
#     define PDL_dynamic_cast_2_ref(TYPE, T1, T2, EXPR)                dynamic_cast<TYPE<T1, T2> &> (EXPR)
#     define PDL_dynamic_cast_3_ref(TYPE, T1, T2, T3, EXPR)            dynamic_cast<TYPE<T1, T2, T3> &> (EXPR)
#     define PDL_dynamic_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)        dynamic_cast<TYPE<T1, T2, T3, T4> &> (EXPR)
#     define PDL_dynamic_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)    dynamic_cast<TYPE<T1, T2, T3, T4, T5> &> (EXPR)
#   endif /* ! PDL_LACKS_RTTI */

# else

#   define PDL_sap_any_cast(TYPE)                                      ((TYPE) (PDL_Addr::sap_any))

#   define PDL_static_cast(TYPE, EXPR)                                 ((TYPE) (EXPR))
#   define PDL_static_cast_1_ptr(TYPE, T1, EXPR)                       ((TYPE<T1> *) (EXPR))
#   define PDL_static_cast_2_ptr(TYPE, T1, T2, EXPR)                   ((TYPE<T1, T2> *) (EXPR))
#   define PDL_static_cast_3_ptr(TYPE, T1, T2, T3, EXPR)               ((TYPE<T1, T2, T3> *) (EXPR))
#   define PDL_static_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)           ((TYPE<T1, T2, T3, T4> *) (EXPR))
#   define PDL_static_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)       ((TYPE<T1, T2, T3, T4, T5> *) (EXPR))
#   define PDL_static_cast_1_ref(TYPE, T1, EXPR)                       ((TYPE<T1> &) (EXPR))
#   define PDL_static_cast_2_ref(TYPE, T1, T2, EXPR)                   ((TYPE<T1, T2> &) (EXPR))
#   define PDL_static_cast_3_ref(TYPE, T1, T2, T3, EXPR)               ((TYPE<T1, T2, T3> &) (EXPR))
#   define PDL_static_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)           ((TYPE<T1, T2, T3, T4> &) (EXPR))
#   define PDL_static_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)       ((TYPE<T1, T2, T3, T4, T5> &) (EXPR))

#   define PDL_const_cast(TYPE, EXPR)                                  ((TYPE) (EXPR))
#   define PDL_const_cast_1_ptr(TYPE, T1, EXPR)                        ((TYPE<T1> *) (EXPR))
#   define PDL_const_cast_2_ptr(TYPE, T1, T2, EXPR)                    ((TYPE<T1, T2> *) (EXPR))
#   define PDL_const_cast_3_ptr(TYPE, T1, T2, T3, EXPR)                ((TYPE<T1, T2, T3> *) (EXPR))
#   define PDL_const_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)            ((TYPE<T1, T2, T3, T4> *) (EXPR))
#   define PDL_const_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)        ((TYPE<T1, T2, T3, T4, T5> *) (EXPR))
#   define PDL_const_cast_1_ref(TYPE, T1, EXPR)                        ((TYPE<T1> &) (EXPR))
#   define PDL_const_cast_2_ref(TYPE, T1, T2, EXPR)                    ((TYPE<T1, T2> &) (EXPR))
#   define PDL_const_cast_3_ref(TYPE, T1, T2, T3, EXPR)                ((TYPE<T1, T2, T3> &) (EXPR))
#   define PDL_const_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)            ((TYPE<T1, T2, T3, T4> &) (EXPR))
#   define PDL_const_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)        ((TYPE<T1, T2, T3, T4, T5> &) (EXPR))

#   define PDL_reinterpret_cast(TYPE, EXPR)                            ((TYPE) (EXPR))
#   define PDL_reinterpret_cast_1_ptr(TYPE, T1, EXPR)                  ((TYPE<T1> *) (EXPR))
#   define PDL_reinterpret_cast_2_ptr(TYPE, T1, T2, EXPR)              ((TYPE<T1, T2> *) (EXPR))
#   define PDL_reinterpret_cast_3_ptr(TYPE, T1, T2, T3, EXPR)          ((TYPE<T1, T2, T3> *) (EXPR))
#   define PDL_reinterpret_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)      ((TYPE<T1, T2, T3, T4> *) (EXPR))
#   define PDL_reinterpret_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)  ((TYPE<T1, T2, T3, T4, T5> *) (EXPR))
#   define PDL_reinterpret_cast_1_ref(TYPE, T1, EXPR)                  ((TYPE<T1> &) (EXPR))
#   define PDL_reinterpret_cast_2_ref(TYPE, T1, T2, EXPR)              ((TYPE<T1, T2> &) (EXPR))
#   define PDL_reinterpret_cast_3_ref(TYPE, T1, T2, T3, EXPR)          ((TYPE<T1, T2, T3> &) (EXPR))
#   define PDL_reinterpret_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)      ((TYPE<T1, T2, T3, T4> &) (EXPR))
#   define PDL_reinterpret_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)  ((TYPE<T1, T2, T3, T4, T5> &) (EXPR))

#   define PDL_dynamic_cast(TYPE, EXPR)                                ((TYPE) (EXPR))
#   define PDL_dynamic_cast_1_ptr(TYPE, T1, EXPR)                      ((TYPE<T1> *) (EXPR))
#   define PDL_dynamic_cast_2_ptr(TYPE, T1, T2, EXPR)                  ((TYPE<T1, T2> *) (EXPR))
#   define PDL_dynamic_cast_3_ptr(TYPE, T1, T2, T3, EXPR)              ((TYPE<T1, T2, T3> *) (EXPR))
#   define PDL_dynamic_cast_4_ptr(TYPE, T1, T2, T3, T4, EXPR)          ((TYPE<T1, T2, T3, T4> *) (EXPR))
#   define PDL_dynamic_cast_5_ptr(TYPE, T1, T2, T3, T4, T5, EXPR)      ((TYPE<T1, T2, T3, T4, T5> *) (EXPR))
#   define PDL_dynamic_cast_1_ref(TYPE, T1, EXPR)                      ((TYPE<T1> &) (EXPR))
#   define PDL_dynamic_cast_2_ref(TYPE, T1, T2, EXPR)                  ((TYPE<T1, T2> &) (EXPR))
#   define PDL_dynamic_cast_3_ref(TYPE, T1, T2, T3, EXPR)              ((TYPE<T1, T2, T3> &) (EXPR))
#   define PDL_dynamic_cast_4_ref(TYPE, T1, T2, T3, T4, EXPR)          ((TYPE<T1, T2, T3, T4> &) (EXPR))
#   define PDL_dynamic_cast_5_ref(TYPE, T1, T2, T3, T4, T5, EXPR)      ((TYPE<T1, T2, T3, T4, T5> &) (EXPR))
# endif /* PDL_HAS_ANSI_CASTS */

# if !defined (PDL_CAST_CONST)
    // Sun CC 4.2, for example, requires const in reinterpret casts of
    // data members in const member functions.  But, other compilers
    // complain about the useless const.  This keeps everyone happy.
#   if defined (__SUNPRO_CC)
#     define PDL_CAST_CONST const
#   else  /* ! __SUNPRO_CC */
#     define PDL_CAST_CONST
#   endif /* ! __SUNPRO_CC */
# endif /* ! PDL_CAST_CONST */

// Deal with MSVC++ insanity for CORBA...
# if defined (PDL_HAS_BROKEN_NAMESPACES)
#   define PDL_CORBA_1(NAME) CORBA_##NAME
#   define PDL_CORBA_2(TYPE, NAME) CORBA_##TYPE##_##NAME
#   define PDL_CORBA_3(TYPE, NAME) CORBA_##TYPE::NAME
#   define PDL_NESTED_CLASS(TYPE, NAME) NAME
# else  /* ! PDL_HAS_BROKEN_NAMESPACES */
#   define PDL_CORBA_1(NAME) CORBA::NAME
#   define PDL_CORBA_2(TYPE, NAME) CORBA::TYPE::NAME
#   define PDL_CORBA_3(TYPE, NAME) CORBA::TYPE::NAME
#   define PDL_NESTED_CLASS(TYPE, NAME) TYPE::NAME
# endif /* ! PDL_HAS_BROKEN_NAMESPACES */

# if defined (PDL_NO_INLINE)
  // PDL inlining has been explicitly disabled.  Implement
  // internally within PDL by undefining __PDL_INLINE__.
#   undef __PDL_INLINE__
# endif /* ! PDL_NO_INLINE */

# if !defined (PDL_DEFAULT_CLOSE_ALL_HANDLES)
#   define PDL_DEFAULT_CLOSE_ALL_HANDLES 1
# endif /* PDL_DEFAULT_CLOSE_ALL_HANDLES */

// The maximum length for a fully qualified Internet name.
# if !defined(PDL_MAX_FULLY_QUALIFIED_NAME_LEN)
#   define PDL_MAX_FULLY_QUALIFIED_NAME_LEN 256
# endif /* PDL_MAX_FULLY_QUALIFIED_NAME_LEN */

# if defined (PDL_HAS_4_4BSD_SENDMSG_RECVMSG)
    // Control message size to pass a file descriptor.
#   define PDL_BSD_CONTROL_MSG_LEN sizeof (struct cmsghdr) + sizeof (PDL_HANDLE)
#   if defined (PDL_LACKS_CMSG_DATA_MACRO)
#     if defined (PDL_LACKS_CMSG_DATA_MEMBER)
#       define CMSG_DATA(cmsg) ((unsigned char *) ((struct cmsghdr *) (cmsg) + 1))
#     else
#       define CMSG_DATA(cmsg) ((cmsg)->cmsg_data)
#     endif /* PDL_LACKS_CMSG_DATA_MEMBER */
#   endif /* PDL_LACKS_CMSG_DATA_MACRO */
# endif /* PDL_HAS_4_4BSD_SENDMSG_RECVMSG */

// Define the default constants for PDL.  Many of these are used for
// the PDL tests and applications.  You can change these values by
// defining the macros in your config.h file.

# if !defined (PDL_DEFAULT_TIMEOUT)
#   define PDL_DEFAULT_TIMEOUT 5
# endif /* PDL_DEFAULT_TIMEOUT */

# if !defined (PDL_DEFAULT_BACKLOG)
#   define PDL_DEFAULT_BACKLOG 5
# endif /* PDL_DEFAULT_BACKLOG */

# if !defined (PDL_DEFAULT_THREADS)
#   define PDL_DEFAULT_THREADS 1
# endif /* PDL_DEFAULT_THREADS */

// The following 3 defines are used in the IP multicast and broadcast tests.
# if !defined (PDL_DEFAULT_BROADCAST_PORT)
#   define PDL_DEFAULT_BROADCAST_PORT 10000
# endif /* PDL_DEFAULT_BROADCAST_PORT */

# if !defined (PDL_DEFAULT_MULTICAST_PORT)
#   define PDL_DEFAULT_MULTICAST_PORT 10001
# endif /* PDL_DEFAULT_MULTICAST_PORT */

# if !defined (PDL_DEFAULT_MULTICAST_ADDR)
// This address MUST be within the range for host group addresses:
// 224.0.0.0 to 239.255.255.255.
#   define PDL_DEFAULT_MULTICAST_ADDR "224.9.9.2"
# endif /* PDL_DEFAULT_MULTICAST_ADDR */

// Default port number for HTTP.
# if !defined (PDL_DEFAULT_HTTP_SERVER_PORT)
#   define PDL_DEFAULT_HTTP_SERVER_PORT 80
# endif /* PDL_DEFAULT_HTTP_SERVER_PORT */

// Used in many IPC_SAP tests
# if !defined (PDL_DEFAULT_SERVER_PORT)
#   define PDL_DEFAULT_SERVER_PORT 10002
# endif /* PDL_DEFAULT_SERVER_PORT */

# if !defined (PDL_DEFAULT_HTTP_PORT)
#   define PDL_DEFAULT_HTTP_PORT 80
# endif /* PDL_DEFAULT_HTTP_PORT */

# if !defined (PDL_DEFAULT_MAX_SOCKET_BUFSIZ)
#   define PDL_DEFAULT_MAX_SOCKET_BUFSIZ 65536
# endif /* PDL_DEFAULT_MAX_SOCKET_BUFSIZ */

# if !defined (PDL_DEFAULT_SERVER_PORT_STR)
#   define PDL_DEFAULT_SERVER_PORT_STR "10002"
# endif /* PDL_DEFAULT_SERVER_PORT_STR */

// Used for the Service_Directory test
# if !defined (PDL_DEFAULT_SERVICE_PORT)
#   define PDL_DEFAULT_SERVICE_PORT 10003
# endif /* PDL_DEFAULT_SERVICE_PORT */

// Used for the PDL_Thread_Spawn test
# if !defined (PDL_DEFAULT_THR_PORT    )
#   define PDL_DEFAULT_THR_PORT 10004
# endif /* PDL_DEFAULT_THR_PORT */

// Used for <SOCK_Connect::connect> tests
# if !defined (PDL_DEFAULT_LOCAL_PORT)
#   define PDL_DEFAULT_LOCAL_PORT 10005
# endif /* PDL_DEFAULT_LOCAL_PORT */

// Used for Connector tests
# if !defined (PDL_DEFAULT_LOCAL_PORT_STR)
#   define PDL_DEFAULT_LOCAL_PORT_STR "10005"
# endif /* PDL_DEFAULT_LOCAL_PORT_STR */

// Used for the name server.
# if !defined (PDL_DEFAULT_NAME_SERVER_PORT)
#   define PDL_DEFAULT_NAME_SERVER_PORT 10006
# endif /* PDL_DEFAULT_NAME_SERVER_PORT */

# if !defined (PDL_DEFAULT_NAME_SERVER_PORT_STR)
#   define PDL_DEFAULT_NAME_SERVER_PORT_STR "10006"
# endif /* PDL_DEFAULT_NAME_SERVER_PORT_STR */

// Used for the token server.
# if !defined (PDL_DEFAULT_TOKEN_SERVER_PORT)
#   define PDL_DEFAULT_TOKEN_SERVER_PORT 10007
# endif /* PDL_DEFAULT_TOKEN_SERVER_PORT */

# if !defined (PDL_DEFAULT_TOKEN_SERVER_PORT_STR)
#   define PDL_DEFAULT_TOKEN_SERVER_PORT_STR "10007"
# endif /* PDL_DEFAULT_TOKEN_SERVER_PORT_STR */

// Used for the logging server.
# if !defined (PDL_DEFAULT_LOGGING_SERVER_PORT)
#   define PDL_DEFAULT_LOGGING_SERVER_PORT 10008
# endif /* PDL_DEFAULT_LOGGING_SERVER_PORT */

# if !defined (PDL_DEFAULT_LOGGING_SERVER_PORT_STR)
#   define PDL_DEFAULT_LOGGING_SERVER_PORT_STR "10008"
# endif /* PDL_DEFAULT_LOGGING_SERVER_PORT_STR */

// Used for the logging server.
# if !defined (PDL_DEFAULT_THR_LOGGING_SERVER_PORT)
#   define PDL_DEFAULT_THR_LOGGING_SERVER_PORT 10008
# endif /* PDL_DEFAULT_THR_LOGGING_SERVER_PORT */

# if !defined (PDL_DEFAULT_THR_LOGGING_SERVER_PORT_STR)
#   define PDL_DEFAULT_THR_LOGGING_SERVER_PORT_STR "10008"
# endif /* PDL_DEFAULT_THR_LOGGING_SERVER_PORT_STR */

// Used for the time server.
# if !defined (PDL_DEFAULT_TIME_SERVER_PORT)
#   define PDL_DEFAULT_TIME_SERVER_PORT 10009
# endif /* PDL_DEFAULT_TIME_SERVER_PORT */

# if !defined (PDL_DEFAULT_TIME_SERVER_PORT_STR)
#   define PDL_DEFAULT_TIME_SERVER_PORT_STR "10009"
# endif /* PDL_DEFAULT_TIME_SERVER_PORT_STR */

# if !defined (PDL_DEFAULT_TIME_SERVER_STR)
#   define PDL_DEFAULT_TIME_SERVER_STR "PDL_TS_TIME"
# endif /* PDL_DEFAULT_TIME_SERVER_STR */

// Used by the FIFO tests and the Client_Logging_Handler netsvc.
# if !defined (PDL_DEFAULT_RENDEZVOUS)
#   if defined (PDL_HAS_STREAM_PIPES)
#     define PDL_DEFAULT_RENDEZVOUS "/tmp/fifo.pdl"
#   else
#     define PDL_DEFAULT_RENDEZVOUS "localhost:10010"
#   endif /* PDL_HAS_STREAM_PIPES */
# endif /* PDL_DEFAULT_RENDEZVOUS */

# if !defined (PDL_DEFAULT_LOGGER_KEY)
#   if defined (PDL_HAS_UNICODE) && defined (UNICODE)
#     if defined (PDL_HAS_STREAM_PIPES)
#       define PDL_DEFAULT_LOGGER_KEY L"/tmp/server_daemon"
#     else
#       define PDL_DEFAULT_LOGGER_KEY L"localhost:10012"
#     endif /* PDL_HAS_STREAM_PIPES */
#   else /* PDL_HAS_UNICODE */
#     if defined (PDL_HAS_STREAM_PIPES)
#       define PDL_DEFAULT_LOGGER_KEY "/tmp/server_daemon"
#     else
#       define PDL_DEFAULT_LOGGER_KEY "localhost:10012"
#     endif /* PDL_HAS_STREAM_PIPES */
#   endif /* PDL_HAS_UNICODE && UNICODE */
# endif /* PDL_DEFAULT_LOGGER_KEY */

// The way to specify the local host for loopback IP. This is usually
// "localhost" but it may need changing on some platforms.
# if !defined (PDL_LOCALHOST)
#   define PDL_LOCALHOST ASYS_TEXT("localhost")
# endif

# if !defined (PDL_DEFAULT_SERVER_HOST)
#   define PDL_DEFAULT_SERVER_HOST PDL_LOCALHOST
# endif /* PDL_DEFAULT_SERVER_HOST */

// Default shared memory key
# if !defined (PDL_DEFAULT_SHM_KEY)
#   define PDL_DEFAULT_SHM_KEY 1234
# endif /* PDL_DEFAULT_SHM_KEY */

// Default segment size used by SYSV shared memory (128 K)
# if !defined (PDL_DEFAULT_SEGMENT_SIZE)
#   define PDL_DEFAULT_SEGMENT_SIZE 1024 * 128
# endif /* PDL_DEFAULT_SEGMENT_SIZE */

// Maximum number of SYSV shared memory segments
// (does anyone know how to figure out the right values?!)
# if !defined (PDL_DEFAULT_MAX_SEGMENTS)
#   define PDL_DEFAULT_MAX_SEGMENTS 6
# endif /* PDL_DEFAULT_MAX_SEGMENTS */

// Name of the map that's stored in shared memory.
# if !defined (PDL_NAME_SERVER_MAP)
#   define PDL_NAME_SERVER_MAP "Name Server Map"
# endif /* PDL_NAME_SERVER_MAP */

// Default file permissions.
# if !defined (PDL_DEFAULT_FILE_PERMS)
#   define PDL_DEFAULT_FILE_PERMS 0666
# endif /* PDL_DEFAULT_FILE_PERMS */

// Default directory permissions.
# if !defined (PDL_DEFAULT_DIR_PERMS)
#   define PDL_DEFAULT_DIR_PERMS 0777
# endif /* PDL_DEFAULT_DIR_PERMS */

// Default size of the PDL Reactor.
# if !defined (PDL_DEFAULT_SELECT_REACTOR_SIZE)
#   define PDL_DEFAULT_SELECT_REACTOR_SIZE FD_SETSIZE
# endif /* PDL_DEFAULT_SELECT_REACTOR_SIZE */

# if !defined (PDL_DEFAULT_TIMEPROBE_TABLE_SIZE)
#   define PDL_DEFAULT_TIMEPROBE_TABLE_SIZE 8 * 1024
# endif /* PDL_DEFAULT_TIMEPROBE_TABLE_SIZE */

// Default size of the PDL Map_Manager.
# if !defined (PDL_DEFAULT_MAP_SIZE)
#   define PDL_DEFAULT_MAP_SIZE 1024
# endif /* PDL_DEFAULT_MAP_SIZE */

// Defaults for PDL Timer Wheel
# if !defined (PDL_DEFAULT_TIMER_WHEEL_SIZE)
#   define PDL_DEFAULT_TIMER_WHEEL_SIZE 1024
# endif /* PDL_DEFAULT_TIMER_WHEEL_SIZE */

# if !defined (PDL_DEFAULT_TIMER_WHEEL_RESOLUTION)
#   define PDL_DEFAULT_TIMER_WHEEL_RESOLUTION 100
# endif /* PDL_DEFAULT_TIMER_WHEEL_RESOLUTION */

// Default size for PDL Timer Hash table
# if !defined (PDL_DEFAULT_TIMER_HASH_TABLE_SIZE)
#   define PDL_DEFAULT_TIMER_HASH_TABLE_SIZE 1024
# endif /* PDL_DEFAULT_TIMER_HASH_TABLE_SIZE */

// Defaults for the PDL Free List
# if !defined (PDL_DEFAULT_FREE_LIST_PREALLOC)
#   define PDL_DEFAULT_FREE_LIST_PREALLOC 0
# endif /* PDL_DEFAULT_FREE_LIST_PREALLOC */

# if !defined (PDL_DEFAULT_FREE_LIST_LWM)
#   define PDL_DEFAULT_FREE_LIST_LWM 0
# endif /* PDL_DEFAULT_FREE_LIST_LWM */

# if !defined (PDL_DEFAULT_FREE_LIST_HWM)
#   define PDL_DEFAULT_FREE_LIST_HWM 25000
# endif /* PDL_DEFAULT_FREE_LIST_HWM */

# if !defined (PDL_DEFAULT_FREE_LIST_INC)
#   define PDL_DEFAULT_FREE_LIST_INC 100
# endif /* PDL_DEFAULT_FREE_LIST_INC */

# if !defined (PDL_UNIQUE_NAME_LEN)
#   define PDL_UNIQUE_NAME_LEN 100
# endif /* PDL_UNIQUE_NAME_LEN */

# if !defined (PDL_MAX_DGRAM_SIZE)
   // This is just a guess.  8k is the normal limit on
   // most machines because that's what NFS expects.
#   define PDL_MAX_DGRAM_SIZE 8192
# endif /* PDL_MAX_DGRAM_SIZE */

# if !defined (PDL_DEFAULT_ARGV_BUFSIZ)
#   define PDL_DEFAULT_ARGV_BUFSIZ 1024 * 4
# endif /* PDL_DEFAULT_ARGV_BUFSIZ */

// Because most WinCE APIs use wchar strings, many of PDL functions
// must adapt to this change also.  For backward compatibility (most
// platforms still use ASCII char APIs even if they support UNICODE,)
// these new macros are introduced to avoid code bloating.
//
// ASYS stands for PDL SYSTEM something.

# if defined (PDL_HAS_MOSTLY_UNICODE_APIS)
#   define ASYS_TCHAR wchar_t
#   define ASYS_TEXT(STRING)   __TEXT (STRING)
# else
#   define ASYS_TCHAR char
#   define ASYS_TEXT(STRING)   STRING
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

// Macro to replace __TEXT
# if !defined (PDL_TEXT)
#   if (defined (PDL_HAS_UNICODE) && (defined (UNICODE)))
#     define PDL_TEXT(STRING) L##STRING
#   else
#     define PDL_TEXT(STRING) STRING
#   endif /* UNICODE && PDL_HAS_UNICODE */
# endif /* !defined PDL_TEXT */

# if !defined (PDL_HAS_TEXT_MACRO_CONFLICT)
#   define __TEXT(STRING) PDL_TEXT(STRING)
# endif /* PDL_HAS_TEXT_MACRO_CONFLICT */

// Here are all PDL-specific global declarations needed throughout
// PDL.

// Helpful dump macros.
# define PDL_BEGIN_DUMP ASYS_TEXT ("\n====\n(%P|%t|%x)")
# define PDL_END_DUMP ASYS_TEXT ("====\n")

// A free list which create more elements when there aren't enough
// elements.
# define PDL_FREE_LIST_WITH_POOL 1

// A simple free list which doen't allocate/deallocate elements.
# define PDL_PURE_FREE_LIST 2

# if defined (PDL_NDEBUG)
#   define PDL_DB(X)
# else
#   define PDL_DB(X) X
# endif /* PDL_NDEBUG */

// Nasty macro stuff to account for Microsoft Win32 DLL nonsense.  We
// use these macros so that we don't end up with PDL software
// hard-coded to Microsoft proprietary extensions to C++.

// First, we define how to properly export/import objects.
# if defined (PDL_WIN32) /* Only Win32 needs special treatment. */
#   if defined(_MSC_VER) || defined(__BORLANDC__) || defined (__IBMCPP__)
/*  Microsoft, Borland: */
#     define PDL_Proper_Export_Flag __declspec (dllexport)
#     define PDL_Proper_Import_Flag __declspec (dllimport)
#     define PDL_EXPORT_SINGLETON_DECLARATION(T)  template class __declspec (dllexport) T
#     define PDL_IMPORT_SINGLETON_DECLARATION(T)  extern template class T
#   else /* defined (_MSC_VER) || defined (__BORLANDC__) || defined (__IBMCPP__) */
/* Non-Microsoft, non-Borland: */
#     define PDL_Proper_Export_Flag _export
#     define PDL_Proper_Import_Flag _import
#     define PDL_EXPORT_SINGLETON_DECLARATION(T)
#     define PDL_IMPORT_SINGLETON_DECLARATION(T)
#   endif /* defined(_MSC_VER) || defined(__BORLANDC__) */
# else  /* ! PDL_WIN32 */
#   define PDL_Proper_Export_Flag
#   define PDL_Proper_Import_Flag
#   define PDL_EXPORT_SINGLETON_DECLARATION(T)
#   define PDL_IMPORT_SINGLETON_DECLARATION(T)
# endif /* PDL_WIN32 */

// Here are definition for PDL library.
# if defined (PDL_HAS_DLL) && (PDL_HAS_DLL == 1)
#   if defined (PDL_BUILD_DLL)
#     define PDL_Export PDL_Proper_Export_Flag
#     define PDL_SINGLETON_DECLARATION(T) PDL_EXPORT_SINGLETON_DECLARATION (T)
#   else
#     define PDL_Export PDL_Proper_Import_Flag
#     define PDL_SINGLETON_DECLARATION(T) PDL_IMPORT_SINGLETON_DECLARATION (T)
#   endif /* PDL_BUILD_DLL */
# else /* ! PDL_HAS_DLL */
#   define PDL_Export
#   define PDL_SINGLETON_DECLARATION(T)
# endif /* PDL_HAS_DLL */

// Here are definition for PDL_Svc library.
# if defined (PDL_HAS_SVC_DLL) && (PDL_HAS_SVC_DLL == 1)
#   if defined (PDL_BUILD_SVC_DLL)
#     define PDL_Svc_Export PDL_Proper_Export_Flag
#     define PDL_SVC_SINGLETON_DECLARATION(T) PDL_EXPORT_SINGLETON_DECLARATION (T)
#   else
#     define PDL_Svc_Export PDL_Proper_Import_Flag
#     define PDL_SVC_SINGLETON_DECLARATION(T) PDL_IMPORT_SINGLETON_DECLARATION (T)
#   endif /* PDL_BUILD_SVC_DLL */
# else /* PDL_HAS_SVC_DLL */
#   define PDL_Svc_Export
#   define PDL_SVC_SINGLETON_DECLARATION(T)
# endif /* PDL_HAS_SVC_DLL */

// This is a whim of mine -- that instead of annotating a class with
// PDL_Export in its declaration, we make the declaration near the TOP
// of the file with PDL_DECLARE_EXPORT.
# define PDL_DECLARE_EXPORT(TS,ID) TS PDL_Export ID
// TS = type specifier (e.g., class, struct, int, etc.)
// ID = identifier
// So, how do you use it?  Most of the time, just use ...
// PDL_DECLARE_EXPORT(class, someobject);
// If there are global functions to be exported, then use ...
// PDL_DECLARE_EXPORT(void, globalfunction) (int, ...);
// Someday, when template libraries are supported, we made need ...
// PDL_DECLARE_EXPORT(template class, sometemplate) <class TYPE, class LOCK>;

// PDL_NO_HEAP_CHECK macro can be used to suppress false report of
// memory leaks. It turns off the built-in heap checking until the
// block is left. The old state will then be restored Only used for
// Win32 (in the moment).
# if defined (PDL_WIN32)

// This is necessary to work around bugs with Win32 non-blocking
// connects...
#   if !defined (PDL_NON_BLOCKING_BUG_DELAY)
#     define PDL_NON_BLOCKING_BUG_DELAY 35000
#   endif /* PDL_NON_BLOCKING_BUG_DELAY */

#   if defined (_DEBUG) && !defined (PDL_HAS_WINCE) && !defined (__BORLANDC__)
class PDL_Export PDL_No_Heap_Check
{
public:
  PDL_No_Heap_Check (void)
    : old_state (_CrtSetDbgFlag (_CRTDBG_REPORT_FLAG))
  { _CrtSetDbgFlag (old_state & ~_CRTDBG_ALLOC_MEM_DF);}
  ~PDL_No_Heap_Check (void) { _CrtSetDbgFlag (old_state);}
private:
  int old_state;
};
#     define PDL_NO_HEAP_CHECK PDL_No_Heap_Check ____no_heap;
#   else /* !_DEBUG */
#     define PDL_NO_HEAP_CHECK
#   endif /* _DEBUG */
# else /* !PDL_WIN32 */
#   define PDL_NO_HEAP_CHECK
# endif /* PDL_WIN32 */

// Turn a number into a string.
# define PDL_ITOA(X) #X

// Create a string of a server address with a "host:port" format.
# define PDL_SERVER_ADDRESS(H,P) H":"P

// A couple useful inline functions for checking whether bits are
// enabled or disabled.

// Efficiently returns the least power of two >= X...
# define PDL_POW(X) (((X) == 0)?1:(X-=1,X|=X>>1,X|=X>>2,X|=X>>4,X|=X>>8,X|=X>>16,(++X)))
# define PDL_EVEN(NUM) (((NUM) & 1) == 0)
# define PDL_ODD(NUM) (((NUM) & 1) == 1)
# define PDL_BIT_ENABLED(WORD, BIT) (((WORD) & (BIT)) != 0)
# define PDL_BIT_DISABLED(WORD, BIT) (((WORD) & (BIT)) == 0)
# define PDL_BIT_CMP_MASK(WORD, BIT, MASK) (((WORD) & (BIT)) == MASK)
# define PDL_SET_BITS(WORD, BITS) (WORD |= (BITS))
# define PDL_CLR_BITS(WORD, BITS) (WORD &= ~(BITS))

// include the PDL min()/max() functions.
# include "pdl/Min_Max.h"

// Keep the compiler from complaining about parameters which are not used.
# if defined (ghs) || defined (__GNUC__) || defined (__hpux) || defined (__sgi) || defined (__DECCXX) || defined (__KCC) || defined (__rational__) || (__USLC__)
// Some compilers complain about "statement with no effect" with (a).
// This eliminates the warnings, and no code is generated for the null
// conditional statement.  NOTE: that may only be true if -O is enabled,
// such as with GreenHills (ghs) 1.8.8.
#   define PDL_UNUSED_ARG(a) {if (&a) /* null */ ;}
# else
#   define PDL_UNUSED_ARG(a) (a)
# endif /* ghs */

# if defined (__sgi) || defined (ghs) || defined (__DECCXX) || defined(__BORLANDC__) || defined (__KCC)
#   define PDL_NOTREACHED(a)
# else  /* ! defined . . . */
#   define PDL_NOTREACHED(a) a
# endif /* ! defined . . . */

# if !defined (PDL_ENDLESS_LOOP)
#  define PDL_ENDLESS_LOOP
# endif /* ! PDL_ENDLESS_LOOP */

# if defined (PDL_NEEDS_FUNC_DEFINITIONS)
    // It just evaporated ;-)  Not pleasant.
#   define PDL_UNIMPLEMENTED_FUNC(f)
# else
#   define PDL_UNIMPLEMENTED_FUNC(f) f;
# endif /* PDL_NEEDS_FUNC_DEFINITIONS */

// Easy way to designate that a class is used as a pseudo-namespace.
// Insures that g++ "friendship" anamolies are properly handled.
# define PDL_CLASS_IS_NAMESPACE(CLASSNAME) \
private: \
CLASSNAME (void); \
CLASSNAME (const CLASSNAME&); \
friend class pdl_dewarn_gplusplus

// These hooks enable PDL to have all dynamic memory management
// automatically handled on a per-object basis.

# if defined (PDL_HAS_ALLOC_HOOKS)
#   define PDL_ALLOC_HOOK_DECLARE \
  void *operator new (size_t bytes); \
  void operator delete (void *ptr);

  // Note that these are just place holders for now.  They'll
  // be replaced by the PDL_Malloc stuff shortly...
#   define PDL_ALLOC_HOOK_DEFINE(CLASS) \
  void *CLASS::operator new (size_t bytes) { return ::new char[bytes]; } \
  void CLASS::operator delete (void *ptr) { delete [] ((char *) ptr); }
# else
#   define PDL_ALLOC_HOOK_DECLARE struct __Ace {} /* Just need a dummy... */
#   define PDL_ALLOC_HOOK_DEFINE(CLASS)
# endif /* PDL_HAS_ALLOC_HOOKS */

# if defined (PDL_LACKS_KEY_T)
#   if defined (PDL_WIN32)
   // Win32 doesn't use numeric values to name its semaphores, it uses
   // strings!
typedef char *key_t;
#   else
typedef int key_t;
#   endif /* PDL_WIN32 */
# endif /* PDL_LACKS_KEY_T */

# if defined (VXWORKS)
#   if defined (ghs)
    // GreenHills 1.8.8 needs the stdarg.h #include before the #include of
    // vxWorks.h.
    // Also, be sure that these #includes come _after_ the key_t typedef, and
    // before the #include of time.h.
#     include /**/ <stdarg.h>
#   endif /* ghs */

#   include /**/ <vxWorks.h>
# endif /* VXWORKS */


///////////////////////////////////////////
//                                       //
// NOTE: Please do not add any #includes //
//       before this point.  On VxWorks, //
//       vxWorks.h must be #included     //
//       first!                          //
//                                       //
///////////////////////////////////////////

# if defined (PDL_PSOS)

    // remap missing error numbers for system functions
#   define EPERM        1        /* Not super-user                        */
#   define ENOENT       2        /* No such file or directory             */
#   define ESRCH        3        /* No such process                       */
#   if ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM)
#     define EINTR        4        /* interrupted system call               */
#   endif /* ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM) */
#   define EBADF        9        /* Bad file number                       */
#   define EAGAIN       11       /* Resource temporarily unavailable      */
#   if ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM)
#     define EWOULDBLOCK  EAGAIN   /* Blocking resource request would block */
#     define ENOMEM       12       /* Not enough core                       */
#   endif /* ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM) */
#   define EACCES       13       /* Permission denied                     */
#   define EFAULT       14       /* Bad access                            */
#   if ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM)
#     define EEXIST       17       /* File exists                           */
#   endif /* ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM) */
#   define ENOSPC       28       /* No space left on device               */
#   if ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM)
#     define EPIPE        32       /* Broken pipe                           */
#   endif /* ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM) */
#   define ETIME        62       /* timer expired                         */
#   define ENAMETOOLONG 78       /* path name is too long                 */
#   define ENOSYS       89       /* Unsupported file system operation     */
#   if ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM)
#     define EADDRINUSE   125      /* Address already in use                */
#     define ENETUNREACH  128      /* Network is unreachable                */
#     define EISCONN      133      /* Socket is already connected           */
#     define ESHUTDOWN    143      /* Can't send after socket shutdown      */
#     define ECONNREFUSED 146      /* Connection refused                    */
#     define EINPROGRESS  150      /* operation now in progress             */
#   endif /* ! defined (PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM) */
#   define ERRMAX       151      /* Last error number                     */

#   if ! defined (NSIG)
#     define NSIG 32
#   endif /* NSIG */

#   if ! defined (TCP_NODELAY)
#     define TCP_NODELAY  1
#   endif /* TCP_NODELAY */

#if defined (PDL_LACKS_ASSERT_MACRO)
 #define assert(expr)
#endif

#   if defined (PDL_PSOSIM)

#     include /**/ "pdl/sys_conf.h" /* system configuration file */
#     include /**/ <psos.h>         /* pSOS+ system calls                */
#     include /**/ <pna.h>          /* pNA+ TCP/IP Network Manager calls */

    /* In the *simulator* environment, use unsigned int for size_t */
#     define size_t  unsigned int


    /*   include <rpc.h>       pRPC+ Remote Procedure Call Library calls   */
    /*                         are not supported by pSOSim                 */
    /*                                                                     */
    /*   include <phile.h>     pHILE+ file system calls are not supported  */
    /*                         by pSOSim *so*, for the time being, we make */
    /*                         use of UNIX file system headers and then    */
    /*                         when we have time, we wrap UNIX file system */
    /*                         calls w/ pHILE+ wrappers, and modify PDL to */
    /*                         use the wrappers under pSOSim               */

    /* put includes for necessary UNIX file system calls here */
#     include /**/ <sys/stat.h>
#     include /**/ <sys/ioctl.h>
#     include /**/ <sys/sockio.h>
#     include /**/ <netinet/tcp.h>

#     define TCP_
#     if ! defined (BUFSIZ)
#       define BUFSIZ 1024
#     endif  /* ! defined (BUFSIZ) */


#   else

#     if defined (PDL_PSOS_CANT_USE_SYS_TYPES)
      // these are missing from the pSOS types.h file, and the compiler
      // supplied types.h file collides with the pSOS version
      typedef unsigned char u_char;
      typedef unsigned short        u_short;
      typedef unsigned int  u_int;
      typedef unsigned long u_long;
      typedef unsigned char uchar_t;
      typedef unsigned short        ushort_t;
      typedef unsigned int  uint_t;
      typedef unsigned long ulong_t;
      typedef char *caddr_t;

#       if defined (PDL_PSOS_DIAB_PPC)
      typedef unsigned long pid_t;
#     define PDL_INVALID_PID ((pid_t) ~0)
#       else /* !defined (PDL_PSOS_DIAB_PPC) */
      typedef long pid_t;
#     define PDL_INVALID_PID ((pid_t) -1)
#       endif /* defined (PDL_PSOS_DIAB_PPC) */

//      typedef unsigned char wchar_t;
#     endif

#     include /**/ "pdl/sys_conf.h" /* system configuration file */
#     include /**/ <configs.h>   /* includes all pSOS headers */
//    #include /**/ <psos.h>    /* pSOS system calls */
#     include /**/ <pna.h>      /* pNA+ TCP/IP Network Manager calls */
#     include /**/ <phile.h>     /* pHILE+ file system calls */
//    #include /**/ <prepccfg.h>     /* pREPC+ file system calls */
#     if defined (PDL_PSOS_DIAB_MIPS)
#       if defined (PDL_PSOS_USES_DIAB_SYS_CALLS)
#         include /**/ <unistd.h>    /* Diab Data supplied file system calls */
#       else
#         include /**/ <prepc.h>
#       endif /* PDL_PSOS_USES_DIAB_SYS_CALLS */
#       include /**/ <sys/wait.h>    /* Diab Data supplied header file */
#     endif /* PDL_PSOS_DIAB_MIPS */

// This collides with phile.h
//    #include /**/ <sys/stat.h>    /* Diab Data supplied header file */

  // missing preprocessor definitions
#     define AF_UNIX 0x1
#     define PF_UNIX AF_UNIX
#     define PF_INET AF_INET
#     define AF_MAX AF_INET
#     define IFF_LOOPBACK IFF_EXTLOOPBACK

  typedef long fd_mask;
#     define IPPORT_RESERVED       1024
#     define IPPORT_USERRESERVED   5000

#     define howmany(x, y) (((x)+((y)-1))/(y))

  extern "C"
  {
    typedef void (* PDL_SignalHandler) (void);
    typedef void (* PDL_SignalHandlerV) (void);
  }

#     if !defined(SIG_DFL)
#       define SIG_DFL (PDL_SignalHandler) 0
#     endif  // philabs

#   endif /* defined (PDL_PSOSIM) */

// For general purpose portability

#   define PDL_BITS_PER_ULONG (8 * sizeof (u_long))

typedef u_long PDL_idtype_t;
typedef u_long PDL_id_t;
#   define PDL_SELF (0)
typedef u_long PDL_pri_t;

// pHILE+ calls the DIR struct XDIR instead
#    if !defined (PDL_PSOS_DIAB_PPC)
typedef XDIR DIR;
#    endif /* !defined (PDL_PSOS_DIAB_PPC) */


// Use pSOS semaphores, wrapped . . .
typedef struct
{
  u_long sema_;
  // Semaphore handle.  This is allocated by pSOS.

  char name_[4];
  // Name of the semaphore: really a 32 bit number to pSOS
} PDL_sema_t;

// Used for PDL_MMAP_Memory_Pool
#   if !defined (PDL_DEFAULT_BACKING_STORE)
#     define PDL_DEFAULT_BACKING_STORE "/tmp/pdl-malloc-XXXXXX"
#   endif /* PDL_DEFAULT_BACKING_STORE */

// Used for PDL_FILE_Connector
#   if !defined (PDL_DEFAULT_TEMP_FILE)
#     define PDL_DEFAULT_TEMP_FILE "/tmp/pdl-file-XXXXXX"
#   endif /* PDL_DEFAULT_TEMP_FILE */

// Used for logging
#   if !defined (PDL_DEFAULT_LOGFILE)
#     define PDL_DEFAULT_LOGFILE "/tmp/logfile"
#   endif /* PDL_DEFAULT_LOGFILE */

// Used for dynamic linking.
#   if !defined (PDL_DEFAULT_SVC_CONF)
#     define PDL_DEFAULT_SVC_CONF "./svc.conf"
#   endif /* PDL_DEFAULT_SVC_CONF */

#   if !defined (PDL_DEFAULT_SEM_KEY)
#     define PDL_DEFAULT_SEM_KEY 1234
#   endif /* PDL_DEFAULT_SEM_KEY */

#   define PDL_STDIN 0
#   define PDL_STDOUT 1
#   define PDL_STDERR 2

#   define PDL_DIRECTORY_SEPARATOR_STR_A "/"
#   define PDL_DIRECTORY_SEPARATOR_CHAR_A '/'

// Define the name of the environment variable that defines the temp
// directory.
#   define PDL_DEFAULT_TEMP_DIR_ENV_A "TMP"
#   define PDL_DEFAULT_TEMP_DIR_ENV_W L"TMP"

#   define PDL_DLL_SUFFIX ".so"
#   define PDL_DLL_PREFIX "lib"
#   define PDL_LD_SEARCH_PATH "LD_LIBRARY_PATH"
#   define PDL_LD_SEARCH_PATH_SEPARATOR_STR ":"
#   define PDL_LOGGER_KEY "/tmp/server_daemon"

#   define PDL_PLATFORM_A "pSOS"
#   define PDL_PLATFORM_EXE_SUFFIX_A ""

# if defined (PDL_HAS_MOSTLY_UNICODE_APIS)
#   define ASYS_WIDE_STRING(ASCII_STRING) PDL_WString (ASCII_STRING).fast_rep ()
# else
#   define ASYS_WIDE_STRING(ASCII_STRING) ASCII_STRING
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

#   if defined (PDL_HAS_UNICODE)
#     define PDL_DIRECTORY_SEPARATOR_STR_W L"/"
#     define PDL_DIRECTORY_SEPARATOR_CHAR_W L'/'
#     define PDL_PLATFORM_W L"pSOS"
#     define PDL_PLATFORM_EXE_SUFFIX_W L""
#   else
#     define PDL_DIRECTORY_SEPARATOR_STR_W "/"
#     define PDL_DIRECTORY_SEPARATOR_CHAR_W '/'
#     define PDL_PLATFORM_W "pSOS"
#     define PDL_PLATFORM_EXE_SUFFIX_W ""
#   endif /* PDL_HAS_UNICODE */

#   define PDL_MAX_DEFAULT_PORT 65535

#   if ! defined(MAXPATHLEN)
#     define MAXPATHLEN  1024
#   endif /* MAXPATHLEN */

#   if ! defined(MAXNAMLEN)
#     define MAXNAMLEN   255
#   endif /* MAXNAMLEN */

#   if defined (PDL_LACKS_MMAP)
#     define PROT_READ 0
#     define PROT_WRITE 0
#     define PROT_EXEC 0
#     define PROT_NONE 0
#     define PROT_RDWR 0
#     define MAP_PRIVATE 0
#     define MAP_SHARED 0
#     define MAP_FIXED 0
#   endif /* PDL_LACKS_MMAP */

// The following 3 defines are used by the PDL Name Server...
#   if !defined (PDL_DEFAULT_NAMESPACE_DIR_A)
#     define PDL_DEFAULT_NAMESPACE_DIR_A "/tmp"
#   endif /* PDL_DEFAULT_NAMESPACE_DIR_A */

#   if !defined (PDL_DEFAULT_LOCALNAME_A)
#     define PDL_DEFAULT_LOCALNAME_A "localnames"
#   endif /* PDL_DEFAULT_LOCALNAME_A */

#   if !defined (PDL_DEFAULT_GLOBALNAME_A)
#     define PDL_DEFAULT_GLOBALNAME_A "globalnames"
#   endif /* PDL_DEFAULT_GLOBALNAME_A */

#   if defined (PDL_HAS_UNICODE)
#     if !defined (PDL_DEFAULT_NAMESPACE_DIR_W)
#       define PDL_DEFAULT_NAMESPACE_DIR_W L"/tmp"
#     endif /* PDL_DEFAULT_NAMESPACE_DIR_W */
#     if !defined (PDL_DEFAULT_LOCALNAME_W)
#       define PDL_DEFAULT_LOCALNAME_W L"localnames"
#     endif /* PDL_DEFAULT_LOCALNAME_W */
#     if !defined (PDL_DEFAULT_GLOBALNAME_W)
#       define PDL_DEFAULT_GLOBALNAME_W L"globalnames"
#     endif /* PDL_DEFAULT_GLOBALNAME_W */
#   else
#     if !defined (PDL_DEFAULT_NAMESPACE_DIR_W)
#       define PDL_DEFAULT_NAMESPACE_DIR_W "/tmp"
#     endif /* PDL_DEFAULT_NAMESPACE_DIR_W */
#     if !defined (PDL_DEFAULT_LOCALNAME_W)
#       define PDL_DEFAULT_LOCALNAME_W "localnames"
#     endif /* PDL_DEFAULT_LOCALNAME_W */
#     if !defined (PDL_DEFAULT_GLOBALNAME_W)
#       define PDL_DEFAULT_GLOBALNAME_W "globalnames"
#     endif /* PDL_DEFAULT_GLOBALNAME_W */
#   endif /* PDL_HAS_UNICODE */

typedef int PDL_HANDLE;
typedef PDL_HANDLE PDL_SOCKET;
#   define PDL_INVALID_HANDLE -1
typedef int PDL_exitcode;

typedef PDL_HANDLE PDL_SHLIB_HANDLE;
#   define PDL_SHLIB_INVALID_HANDLE PDL_INVALID_HANDLE
#   define PDL_DEFAULT_SHLIB_MODE 0

#   define PDL_INVALID_SEM_KEY -1

struct  hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* address length */
  char    **h_addr_list;  /* (first, only) address from name server */
#   define h_addr h_addr_list[0]   /* the first address */
};

struct  servent {
  char     *s_name;    /* official service name */
  char    **s_aliases; /* alias list */
  int       s_port;    /* port # */
  char     *s_proto;   /* protocol to use */
};

// For Win32 compatibility.

typedef const char *LPCTSTR;
typedef char *LPTSTR;
typedef char TCHAR;

#   define PDL_SEH_TRY if (1)
#   define PDL_SEH_EXCEPT(X) while (0)
#   define PDL_SEH_FINALLY if (1)

#   if !defined (LPSECURITY_ATTRIBUTES)
#     define LPSECURITY_ATTRIBUTES int
#   endif /* !defined LPSECURITY_ATTRIBUTES */
#   if !defined (GENERIC_READ)
#     define GENERIC_READ 0
#   endif /* !defined GENERIC_READ */
#   if !defined (FILE_SHARE_READ)
#     define FILE_SHARE_READ 0
#   endif /* !defined FILE_SHARE_READ */
#   if !defined (OPEN_EXISTING)
#     define OPEN_EXISTING 0
#   endif /* !defined OPEN_EXISTING */
#   if !defined (FILE_ATTRIBUTE_NORMAL)
#     define FILE_ATTRIBUTE_NORMAL 0
#   endif /* !defined FILE_ATTRIBUTE_NORMAL */
#   if !defined (MAXIMUM_WAIT_OBJECTS)
#     define MAXIMUM_WAIT_OBJECTS 0
#   endif /* !defined MAXIMUM_WAIT_OBJECTS */
#   if !defined (FILE_FLAG_OVERLAPPED)
#     define FILE_FLAG_OVERLAPPED 0
#   endif /* !defined FILE_FLAG_OVERLAPPED */
#   if !defined (FILE_FLAG_SEQUENTIAL_SCAN)
#     define FILE_FLAG_SEQUENTIAL_SCAN 0
#   endif /* !defined FILE_FLAG_SEQUENTIAL_SCAN */

struct PDL_OVERLAPPED
{
  u_long Internal;
  u_long InternalHigh;
  u_long Offset;
  u_long OffsetHigh;
  PDL_HANDLE hEvent;
};

#   if !defined (USER_INCLUDE_SYS_TIME_TM)
#     if defined (PDL_PSOS_DIAB_PPC)
typedef struct timespec timespec_t;
#     else /* ! defined (PDL_PSOS_DIAB_PPC) */
typedef struct timespec
{
  time_t tv_sec; // Seconds
  long tv_nsec; // Nanoseconds
} timespec_t;
#     endif /* defined (PDL_PSOS_DIAB_PPC) */
#   endif /*  !defined (USER_INCLUDE_SYS_TIME_TM) */

#if defined (PDL_PSOS_HAS_TIME)

// Use pSOS time, wrapped . . .
class PDL_Export PDL_PSOS_Time_t
{
public:
  PDL_PSOS_Time_t (void);
    // default ctor: date, time, and ticks all zeroed.

  PDL_PSOS_Time_t (const timespec_t& t);
    // ctor from a timespec_t

  operator timespec_t ();
    // type cast operator (to a timespec_t)

  static u_long get_system_time (PDL_PSOS_Time_t& t);
    // static member function to get current system time

  static u_long set_system_time (const PDL_PSOS_Time_t& t);
    // static member function to set current system time

#   if defined (PDL_PSOSIM)
  static u_long init_simulator_time (void);
    // static member function to initialize system time, using UNIX calls
#   endif /* PDL_PSOSIM */

  static const u_long max_ticks;
    // max number of ticks supported in a single system call
private:
  // = Constants for prying info out of the pSOS time encoding.
  static const u_long year_mask;
  static const u_long month_mask;
  static const u_long day_mask;
  static const u_long hour_mask;
  static const u_long minute_mask;
  static const u_long second_mask;
  static const int year_shift;
  static const int month_shift;
  static const int hour_shift;
  static const int minute_shift;
  static const int year_origin;
  static const int month_origin;

  // error codes
  static const u_long err_notime;   // system time not set
  static const u_long err_illdate;  // date out of range
  static const u_long err_illtime;  // time out of range
  static const u_long err_illticks; // ticks out of range

   u_long date_;
  // date : year in bits 31-16, month in bits 15-8, day in bits 7-0

  u_long time_;
  // time : hour in bits 31-16, minutes in bits 15-8, seconds in bits 7-0

  u_long ticks_;
  // ticks: number of system clock ticks (KC_TICKS2SEC-1 max)
} ;
#endif /* PDL_PSOS_HAS_TIME */

# endif /* defined (PDL_PSOS) */

# if defined (PDL_HAS_CHARPTR_SPRINTF)
#   define PDL_SPRINTF_ADAPTER(X) ::strlen (X)
# else
#   define PDL_SPRINTF_ADAPTER(X) X
# endif /* PDL_HAS_CHARPTR_SPRINTF */

# if defined (__PDL_INLINE__)
#   define PDL_INLINE inline
#   if !defined (PDL_HAS_INLINED_OSCALLS)
#     define PDL_HAS_INLINED_OSCALLS
#   endif /* !PDL_HAS_INLINED_OSCALLS */
# else
#   define PDL_INLINE
# endif /* __PDL_INLINE__ */

// Default address for shared memory mapped files and SYSV shared memory
// (defaults to 64 M).
# if !defined (PDL_DEFAULT_BASE_ADDR)
#   define PDL_DEFAULT_BASE_ADDR ((char *) (64 * 1024 * 1024))
# endif /* PDL_DEFAULT_BASE_ADDR */

// This fudge factor can be overriden for timers that need it, such as on
// Solaris, by defining the PDL_TIMER_SKEW symbol in the appropriate config
// header.
# if !defined (PDL_TIMER_SKEW)
#   define PDL_TIMER_SKEW 0
# endif /* PDL_TIMER_SKEW */

// This needs to go here *first* to avoid problems with AIX.
# if defined (PDL_HAS_PTHREADS)
extern "C" {
#   define PDL_DONT_INCLUDE_PDL_SIGNAL_H
#     include /**/ <signal.h>
#   undef PDL_DONT_INCLUDE_PDL_SIGNAL_H
#   include /**/ <pthread.h>
#   if defined (DIGITAL_UNIX)
#     define pthread_self __pthread_self
extern "C" pthread_t pthread_self (void);
#   endif /* DIGITAL_UNIX */
}
#   if defined (HPUX_10)
//    HP-UX 10 needs to see cma_sigwait, and since _CMA_NOWRAPPERS_ is defined,
//    this header does not get included from pthreads.h.
#     include /**/ <dce/cma_sigwait.h>
#   endif /* HPUX_10 */
# endif /* PDL_HAS_PTHREADS */

// There are a lot of threads-related macro definitions in the config files.
// They came in at different times and from different places and platform
// requirements as threads evolved.  They are probably not all needed - some
// overlap or are otherwise confused.  This is an attempt to start
// straightening them out.
# if defined (PDL_HAS_PTHREADS_STD)    /* POSIX.1c threads (pthreads) */
  // ... and 2-parameter asctime_r and ctime_r
#   if !defined (PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R) && \
       !defined (PDL_HAS_STHREADS)
#     define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
#   endif
# endif /* PDL_HAS_PTHREADS_STD */

# if (PDL_NTRACE == 1)
#   define PDL_TRACE(X)
# else
#   define PDL_TRACE(X) PDL_Trace ____ (ASYS_TEXT (X), __LINE__, ASYS_TEXT (__FILE__))
# endif /* PDL_NTRACE */

# if !defined (PDL_HAS_WINCE) && !defined (PDL_PSOS_DIAB_MIPS)
#     include /**/ <time.h>
#   if defined (__Lynx__)
#     include /**/ <st.h>
#     include /**/ <sem.h>
#   endif /* __Lynx__ */
# endif /* PDL_HAS_WINCE PDL_PSOS_DIAB_MIPS */

# if defined (PDL_LACKS_SYSTIME_H)
// Some platforms may need to include this, but I suspect that most
// will get it from <time.h>
#   if defined (VXWORKS)
#     include /**/ <sys/times.h>
#   else
#     include /**/ <sys/time.h>
#   endif /* VXWORKS */
# endif /* PDL_LACKS_SYSTIME_H */

# if !defined (PDL_HAS_POSIX_TIME) && !defined (PDL_PSOS)
// Definition per POSIX.
typedef struct timespec
{
  time_t tv_sec; // Seconds
  long tv_nsec; // Nanoseconds
} timespec_t;
# elif defined (PDL_HAS_BROKEN_POSIX_TIME)
// OSF/1 defines struct timespec in <sys/timers.h> - Tom Marrs
#   include /**/ <sys/timers.h>
# endif /* !PDL_HAS_POSIX_TIME */

# if defined(PDL_LACKS_TIMESPEC_T)
typedef struct timespec timespec_t;
# endif /* PDL_LACKS_TIMESPEC_T */

# if !defined (PDL_HAS_CLOCK_GETTIME) && !defined (_CLOCKID_T)
typedef int clockid_t;
#   if !defined (CLOCK_REALTIME)
#     define CLOCK_REALTIME 0
#   endif /* CLOCK_REALTIME */
# endif /* ! PDL_HAS_CLOCK_GETTIME && ! _CLOCKID_T */

// -------------------------------------------------------------------
// These forward declarations are only used to circumvent a bug in
// MSVC 6.0 compiler.  They shouldn't cause any problem for other
// compilers and they can be removed once MS release a SP that contains
// the fix.
class PDL_Time_Value;
PDL_Export PDL_Time_Value operator + (const PDL_Time_Value &tv1,
                                      const PDL_Time_Value &tv2);

PDL_Export PDL_Time_Value operator - (const PDL_Time_Value &tv1,
                                      const PDL_Time_Value &tv2);
// -------------------------------------------------------------------

class PDL_Export PDL_Time_Value
{
  // = TITLE
  //     Operations on "timeval" structures.
  //
  // = DESCRIPTION
  //     This class centralizes all the time related processing in
  //     PDL.  These timers are typically used in conjunction with OS
  //     mechanisms like <select>, <poll>, or <cond_timedwait>.
  //     <PDL_Time_Value> makes the use of these mechanisms portable
  //     across OS platforms,
public:
  // = Useful constants.

  static const PDL_Time_Value zero;
  // Constant "0".

  static const PDL_Time_Value max_time;
  // Constant for maximum time representable.  Note that this time is
  // not intended for use with <select> or other calls that may have
  // *their own* implementation-specific maximum time representations.
  // Its primary use is in time computations such as those used by the
  // dynamic subpriority strategies in the <PDL_Dynamic_Message_Queue>
  // class.

  // = Initialization methods.

  void initialize (void);
  // Default Constructor.

  void initialize (long sec, long usec = 0);
  // Constructor.

  // = Methods for converting to/from various time formats.
  void initialize (const struct timeval &t);
  // Construct the <PDL_Time_Value> from a <timeval>.

  void initialize (const timespec_t &t);
  //  Initializes the <PDL_Time_Value> object from a <timespec_t>.

# if defined (PDL_WIN32)
  void initialize (const FILETIME &ft);
  //  Initializes the PDL_Time_Value object from a Win32 FILETIME
# endif /* PDL_WIN32 */

  void set (long sec, long usec);
  // Construct a <Time_Value> from two <long>s.

  void set (double d);
  // Construct a <Time_Value> from a <double>, which is assumed to be
  // in second format, with any remainder treated as microseconds.

  void set (const timeval &t);
  // Construct a <Time_Value> from a <timeval>.

  void set (const timespec_t &t);
  // Initializes the <Time_Value> object from a <timespec_t>.

# if defined (PDL_WIN32)
  void set (const FILETIME &ft);
  //  Initializes the <Time_Value> object from a <timespec_t>.
# endif /* PDL_WIN32 */

  long msec (void) const;
  // Converts from <Time_Value> format into milli-seconds format.

  void msec (long);
  // Converts from milli-seconds format into <Time_Value> format.

  operator timespec_t () const;
  // Returns the value of the object as a <timespec_t>.

  operator timeval () const;
  // Returns the value of the object as a <timeval>.

  operator const timeval *() const;
  // Returns a pointer to the object as a <timeval>.

# if defined (PDL_WIN32)
  operator FILETIME () const;
  // Returns the value of the object as a Win32 FILETIME.
# endif /* PDL_WIN32 */

  // = The following are accessor/mutator methods.

  long sec (void) const;
  // Get seconds.

  void sec (long sec);
  // Set seconds.

  long usec (void) const;
  // Get microseconds.

  void usec (long usec);
  // Set microseconds.

  // = The following are arithmetic methods for operating on
  // Time_Values.

  void operator += (const PDL_Time_Value &tv);
  // Add <tv> to this.

  void operator -= (const PDL_Time_Value &tv);
  // Subtract <tv> to this.

  friend PDL_Export PDL_Time_Value operator + (const PDL_Time_Value &tv1,
                                               const PDL_Time_Value &tv2);
  // Adds two PDL_Time_Value objects together, returns the sum.

  friend PDL_Export PDL_Time_Value operator - (const PDL_Time_Value &tv1,
                                               const PDL_Time_Value &tv2);
  // Subtracts two PDL_Time_Value objects, returns the difference.

  friend PDL_Export int operator < (const PDL_Time_Value &tv1,
                                    const PDL_Time_Value &tv2);
  // True if tv1 < tv2.

  friend PDL_Export int operator > (const PDL_Time_Value &tv1,
                                    const PDL_Time_Value &tv2);
  // True if tv1 > tv2.

  friend PDL_Export int operator <= (const PDL_Time_Value &tv1,
                                     const PDL_Time_Value &tv2);
  // True if tv1 <= tv2.

  friend PDL_Export int operator >= (const PDL_Time_Value &tv1,
                                     const PDL_Time_Value &tv2);
  // True if tv1 >= tv2.

  friend PDL_Export int operator == (const PDL_Time_Value &tv1,
                                     const PDL_Time_Value &tv2);
  // True if tv1 == tv2.

  friend PDL_Export int operator != (const PDL_Time_Value &tv1,
                                     const PDL_Time_Value &tv2);
  // True if tv1 != tv2.

  void dump (void) const;
  // Dump the state of an object.

# if defined (PDL_WIN32)
  static const DWORDLONG FILETIME_to_timval_skew;
  // Const time difference between FILETIME and POSIX time.
# endif /* PDL_WIN32 */

private:
  void normalize (void);
  // Put the timevalue into a canonical form.

  timeval tv_;
  // Store the values as a <timeval>.
};

class PDL_Export PDL_Countdown_Time
{
  // = TITLE
  //     Keeps track of the amount of elapsed time.
  //
  // = DESCRIPTION
  //     This class has a side-effect on the <max_wait_time> -- every
  //     time the <stop> method is called the <max_wait_time> is
  //     updated.
public:
  // = Initialization and termination methods.
  PDL_Countdown_Time (PDL_Time_Value *max_wait_time);
  // Cache the <max_wait_time> and call <start>.

  ~PDL_Countdown_Time (void);
  // Call <stop>.

  int start (void);
  // Cache the current time and enter a start state.

  int stop (void);
  // Subtract the elapsed time from max_wait_time_ and enter a stopped
  // state.

  int update (void);
  // Calls stop and then start.  max_wait_time_ is modified by the
  // call to stop.

private:  PDL_Time_Value *max_wait_time_;
  // Maximum time we were willing to wait.

  PDL_Time_Value start_time_;
  // Beginning of the start time.

  int stopped_;
  // Keeps track of whether we've already been stopped.
};

# if defined (PDL_HAS_USING_KEYWORD)
#   define PDL_USING using
# else
#   define PDL_USING
# endif /* PDL_HAS_USING_KEYWORD */

# if defined (PDL_HAS_TYPENAME_KEYWORD)
#   define PDL_TYPENAME typename
# else
#   define PDL_TYPENAME
# endif /* PDL_HAS_TYPENAME_KEYWORD */

# if defined (PDL_HAS_STD_TEMPLATE_SPECIALIZATION)
#   define PDL_TEMPLATE_SPECIALIZATION template<>
# else
#   define PDL_TEMPLATE_SPECIALIZATION
# endif /* PDL_HAS_STD_TEMPLATE_SPECIALIZATION */

# if defined (PDL_HAS_STD_TEMPLATE_METHOD_SPECIALIZATION)
#   define PDL_TEMPLATE_METHOD_SPECIALIZATION template<>
# else
#   define PDL_TEMPLATE_METHOD_SPECIALIZATION
# endif /* PDL_HAS_STD_TEMPLATE_SPECIALIZATION */

# if defined (PDL_HAS_EXPLICIT_KEYWORD)
#   define PDL_EXPLICIT explicit
# else  /* ! PDL_HAS_EXPLICIT_KEYWORD */
#   define PDL_EXPLICIT
# endif /* ! PDL_HAS_EXPLICIT_KEYWORD */

# if defined (PDL_HAS_MUTABLE_KEYWORD)
#   define PDL_MUTABLE mutable
# else  /* ! PDL_HAS_MUTABLE_KEYWORD */
#   define PDL_MUTABLE
# endif /* ! PDL_HAS_MUTABLE_KEYWORD */

// The following is necessary since many C++ compilers don't support
// typedef'd types inside of classes used as formal template
// arguments... ;-(.  Luckily, using the C++ preprocessor I can hide
// most of this nastiness!

# if defined (PDL_HAS_TEMPLATE_TYPEDEFS)

// Handle PDL_Message_Queue.
#   define PDL_SYNCH_DECL class _PDL_SYNCH
#   define PDL_SYNCH_USE _PDL_SYNCH
#   define PDL_SYNCH_MUTEX_T PDL_TYPENAME _PDL_SYNCH::MUTEX
#   define PDL_SYNCH_CONDITION_T PDL_TYPENAME _PDL_SYNCH::CONDITION
#   define PDL_SYNCH_SEMAPHORE_T PDL_TYPENAME _PDL_SYNCH::SEMAPHORE

// Handle PDL_Malloc*
#   define PDL_MEM_POOL_1 class _PDL_MEM_POOL
#   define PDL_MEM_POOL_2 _PDL_MEM_POOL
#   define PDL_MEM_POOL _PDL_MEM_POOL
#   define PDL_MEM_POOL_OPTIONS PDL_TYPENAME _PDL_MEM_POOL::OPTIONS

// Handle PDL_Svc_Handler
#   define PDL_PEER_STREAM_1 class _PDL_PEER_STREAM
#   define PDL_PEER_STREAM_2 _PDL_PEER_STREAM
#   define PDL_PEER_STREAM _PDL_PEER_STREAM
#   define PDL_PEER_STREAM_ADDR PDL_TYPENAME _PDL_PEER_STREAM::PEER_ADDR

// Handle PDL_Acceptor
#   define PDL_PEER_ACCEPTOR_1 class _PDL_PEER_ACCEPTOR
#   define PDL_PEER_ACCEPTOR_2 _PDL_PEER_ACCEPTOR
#   define PDL_PEER_ACCEPTOR _PDL_PEER_ACCEPTOR
#   define PDL_PEER_ACCEPTOR_ADDR PDL_TYPENAME _PDL_PEER_ACCEPTOR::PEER_ADDR

// Handle PDL_Connector
#   define PDL_PEER_CONNECTOR_1 class _PDL_PEER_CONNECTOR
#   define PDL_PEER_CONNECTOR_2 _PDL_PEER_CONNECTOR
#   define PDL_PEER_CONNECTOR _PDL_PEER_CONNECTOR
#   define PDL_PEER_CONNECTOR_ADDR PDL_TYPENAME _PDL_PEER_CONNECTOR::PEER_ADDR
#   if !defined(PDL_HAS_TYPENAME_KEYWORD)
#     define PDL_PEER_CONNECTOR_ADDR_ANY PDL_PEER_CONNECTOR_ADDR::sap_any
#   else
    //
    // If the compiler supports 'typename' we cannot use
    //
    // PEER_CONNECTOR::PEER_ADDR::sap_any
    //
    // because PEER_CONNECTOR::PEER_ADDR is not considered a type. But:
    //
    // typename PEER_CONNECTOR::PEER_ADDR::sap_any
    //
    // will not work either, because now we are declaring sap_any a
    // type, further:
    //
    // (typename PEER_CONNECTOR::PEER_ADDR)::sap_any
    //
    // is considered a casting expression. All I can think of is using a
    // typedef, I tried PEER_ADDR but that was a source of trouble on
    // some platforms. I will try:
    //
#     define PDL_PEER_CONNECTOR_ADDR_ANY PDL_PEER_ADDR_TYPEDEF::sap_any
#   endif /* PDL_HAS_TYPENAME_KEYWORD */

// Handle PDL_SOCK_*
#   define PDL_SOCK_ACCEPTOR PDL_SOCK_Acceptor
#   define PDL_SOCK_CONNECTOR PDL_SOCK_Connector
#   define PDL_SOCK_STREAM PDL_SOCK_Stream

// Handle PDL_MEM_*
#   define PDL_MEM_ACCEPTOR PDL_MEM_Acceptor
#   define PDL_MEM_CONNECTOR PDL_MEM_Connector
#   define PDL_MEM_STREAM PDL_MEM_Stream

// Handle PDL_LSOCK_*
#   define PDL_LSOCK_ACCEPTOR PDL_LSOCK_Acceptor
#   define PDL_LSOCK_CONNECTOR PDL_LSOCK_Connector
#   define PDL_LSOCK_STREAM PDL_LSOCK_Stream

// Handle PDL_TLI_*
#   define PDL_TLI_ACCEPTOR PDL_TLI_Acceptor
#   define PDL_TLI_CONNECTOR PDL_TLI_Connector
#   define PDL_TLI_STREAM PDL_TLI_Stream

// Handle PDL_SPIPE_*
#   define PDL_SPIPE_ACCEPTOR PDL_SPIPE_Acceptor
#   define PDL_SPIPE_CONNECTOR PDL_SPIPE_Connector
#   define PDL_SPIPE_STREAM PDL_SPIPE_Stream

// Handle PDL_UPIPE_*
#   define PDL_UPIPE_ACCEPTOR PDL_UPIPE_Acceptor
#   define PDL_UPIPE_CONNECTOR PDL_UPIPE_Connector
#   define PDL_UPIPE_STREAM PDL_UPIPE_Stream

// Handle PDL_FILE_*
#   define PDL_FILE_CONNECTOR PDL_FILE_Connector
#   define PDL_FILE_STREAM PDL_FILE_IO

// Handle PDL_*_Memory_Pool.
#   define PDL_MMAP_MEMORY_POOL PDL_MMAP_Memory_Pool
#   define PDL_LITE_MMAP_MEMORY_POOL PDL_Lite_MMAP_Memory_Pool
#   define PDL_SBRK_MEMORY_POOL PDL_Sbrk_Memory_Pool
#   define PDL_SHARED_MEMORY_POOL PDL_Shared_Memory_Pool
#   define PDL_LOCAL_MEMORY_POOL PDL_Local_Memory_Pool
#   define PDL_PAGEFILE_MEMORY_POOL PDL_Pagefile_Memory_Pool

# else /* TEMPLATES are broken in some form or another (i.e., most C++ compilers) */

// Handle PDL_Message_Queue.
#   if defined (PDL_HAS_OPTIMIZED_MESSAGE_QUEUE)
#     define PDL_SYNCH_DECL class _PDL_SYNCH_MUTEX_T, class _PDL_SYNCH_CONDITION_T, class _PDL_SYNCH_SEMAPHORE_T
#     define PDL_SYNCH_USE _PDL_SYNCH_MUTEX_T, _PDL_SYNCH_CONDITION_T, _PDL_SYNCH_SEMAPHORE_T
#   else
#     define PDL_SYNCH_DECL class _PDL_SYNCH_MUTEX_T, class _PDL_SYNCH_CONDITION_T
#     define PDL_SYNCH_USE _PDL_SYNCH_MUTEX_T, _PDL_SYNCH_CONDITION_T
#   endif /* PDL_HAS_OPTIMIZED_MESSAGE_QUEUE */
#   define PDL_SYNCH_MUTEX_T _PDL_SYNCH_MUTEX_T
#   define PDL_SYNCH_CONDITION_T _PDL_SYNCH_CONDITION_T
#   define PDL_SYNCH_SEMAPHORE_T _PDL_SYNCH_SEMAPHORE_T

// Handle PDL_Malloc*
#   define PDL_MEM_POOL_1 class _PDL_MEM_POOL, class _PDL_MEM_POOL_OPTIONS
#   define PDL_MEM_POOL_2 _PDL_MEM_POOL, _PDL_MEM_POOL_OPTIONS
#   define PDL_MEM_POOL _PDL_MEM_POOL
#   define PDL_MEM_POOL_OPTIONS _PDL_MEM_POOL_OPTIONS

// Handle PDL_Svc_Handler
#   define PDL_PEER_STREAM_1 class _PDL_PEER_STREAM, class _PDL_PEER_ADDR
#   define PDL_PEER_STREAM_2 _PDL_PEER_STREAM, _PDL_PEER_ADDR
#   define PDL_PEER_STREAM _PDL_PEER_STREAM
#   define PDL_PEER_STREAM_ADDR _PDL_PEER_ADDR

// Handle PDL_Acceptor
#   define PDL_PEER_ACCEPTOR_1 class _PDL_PEER_ACCEPTOR, class _PDL_PEER_ADDR
#   define PDL_PEER_ACCEPTOR_2 _PDL_PEER_ACCEPTOR, _PDL_PEER_ADDR
#   define PDL_PEER_ACCEPTOR _PDL_PEER_ACCEPTOR
#   define PDL_PEER_ACCEPTOR_ADDR _PDL_PEER_ADDR

// Handle PDL_Connector
#   define PDL_PEER_CONNECTOR_1 class _PDL_PEER_CONNECTOR, class _PDL_PEER_ADDR
#   define PDL_PEER_CONNECTOR_2 _PDL_PEER_CONNECTOR, _PDL_PEER_ADDR
#   define PDL_PEER_CONNECTOR _PDL_PEER_CONNECTOR
#   define PDL_PEER_CONNECTOR_ADDR _PDL_PEER_ADDR
#   define PDL_PEER_CONNECTOR_ADDR_ANY PDL_PEER_CONNECTOR_ADDR::sap_any

// Handle PDL_SOCK_*
#   define PDL_SOCK_ACCEPTOR PDL_SOCK_Acceptor, PDL_INET_Addr
#   define PDL_SOCK_CONNECTOR PDL_SOCK_Connector, PDL_INET_Addr
#   define PDL_SOCK_STREAM PDL_SOCK_Stream, PDL_INET_Addr

// Handle PDL_MEM_*
#   define PDL_MEM_ACCEPTOR PDL_MEM_Acceptor, PDL_MEM_Addr
#   define PDL_MEM_CONNECTOR PDL_MEM_Connector, PDL_INET_Addr
#   define PDL_MEM_STREAM PDL_MEM_Stream, PDL_INET_Addr

// Handle PDL_LSOCK_*
#   define PDL_LSOCK_ACCEPTOR PDL_LSOCK_Acceptor, PDL_UNIX_Addr
#   define PDL_LSOCK_CONNECTOR PDL_LSOCK_Connector, PDL_UNIX_Addr
#   define PDL_LSOCK_STREAM PDL_LSOCK_Stream, PDL_UNIX_Addr

// Handle PDL_TLI_*
#   define PDL_TLI_ACCEPTOR PDL_TLI_Acceptor, PDL_INET_Addr
#   define PDL_TLI_CONNECTOR PDL_TLI_Connector, PDL_INET_Addr
#   define PDL_TLI_STREAM PDL_TLI_Stream, PDL_INET_Addr

// Handle PDL_SPIPE_*
#   define PDL_SPIPE_ACCEPTOR PDL_SPIPE_Acceptor, PDL_SPIPE_Addr
#   define PDL_SPIPE_CONNECTOR PDL_SPIPE_Connector, PDL_SPIPE_Addr
#   define PDL_SPIPE_STREAM PDL_SPIPE_Stream, PDL_SPIPE_Addr

// Handle PDL_UPIPE_*
#   define PDL_UPIPE_ACCEPTOR PDL_UPIPE_Acceptor, PDL_SPIPE_Addr
#   define PDL_UPIPE_CONNECTOR PDL_UPIPE_Connector, PDL_SPIPE_Addr
#   define PDL_UPIPE_STREAM PDL_UPIPE_Stream, PDL_SPIPE_Addr

// Handle PDL_FILE_*
#   define PDL_FILE_CONNECTOR PDL_FILE_Connector, PDL_FILE_Addr
#   define PDL_FILE_STREAM PDL_FILE_IO, PDL_FILE_Addr

// Handle PDL_*_Memory_Pool.
#   define PDL_MMAP_MEMORY_POOL PDL_MMAP_Memory_Pool, PDL_MMAP_Memory_Pool_Options
#   define PDL_LITE_MMAP_MEMORY_POOL PDL_Lite_MMAP_Memory_Pool, PDL_MMAP_Memory_Pool_Options
#   define PDL_SBRK_MEMORY_POOL PDL_Sbrk_Memory_Pool, PDL_Sbrk_Memory_Pool_Options
#   define PDL_SHARED_MEMORY_POOL PDL_Shared_Memory_Pool, PDL_Shared_Memory_Pool_Options
#   define PDL_LOCAL_MEMORY_POOL PDL_Local_Memory_Pool, PDL_Local_Memory_Pool_Options
#   define PDL_PAGEFILE_MEMORY_POOL PDL_Pagefile_Memory_Pool, PDL_Pagefile_Memory_Pool_Options
# endif /* PDL_HAS_TEMPLATE_TYPEDEFS */

// These two are only for backward compatibility. You should avoid
// using them if not necessary.
# define PDL_SYNCH_1 PDL_SYNCH_DECL
# define PDL_SYNCH_2 PDL_SYNCH_USE

// For Win32 compatibility...
# if !defined (PDL_WSOCK_VERSION)
#   define PDL_WSOCK_VERSION 0, 0
# endif /* PDL_WSOCK_VERSION */

# if defined (PDL_HAS_BROKEN_CTIME)
#   undef ctime
# endif /* PDL_HAS_BROKEN_CTIME */

extern "C" {
typedef void (*PDL_Service_Object_Exterminator)(void *);
}

// Static service macros
# define PDL_STATIC_SVC_DECLARE(X) extern PDL_Static_Svc_Descriptor pdl_svc_desc_##X ;
# define PDL_STATIC_SVC_DEFINE(X, NAME, TYPE, FN, FLAGS, ACTIVE) \
PDL_Static_Svc_Descriptor pdl_svc_desc_##X = { NAME, TYPE, FN, FLAGS, ACTIVE };
# define PDL_STATIC_SVC_REQUIRE(X)\
class PDL_Static_Svc_##X {\
public:\
        PDL_Static_Svc_##X() { PDL_Service_Config::static_svcs ()->insert (&pdl_svc_desc_##X); }\
};\
static PDL_Static_Svc_##X pdl_static_svc_##X;


// More generic dynamic/static service macros.
# define PDL_FACTORY_DECLARE(CLS,X) extern "C" CLS##_Export PDL_Service_Object *_make_##X (PDL_Service_Object_Exterminator *);
# define PDL_FACTORY_DEFINE(CLS,X) \
extern "C" void _gobble_##X (void *p) { \
  PDL_Service_Object *_p = PDL_reinterpret_cast (PDL_Service_Object *, p); \
  PDL_ASSERT (_p != 0); \
  delete _p; } \
extern "C" PDL_Service_Object *_make_##X (PDL_Service_Object_Exterminator *gobbler) \
{ PDL_TRACE (#X); \
if (gobbler != 0) *gobbler = (PDL_Service_Object_Exterminator) _gobble_##X; return new X; }

// Dynamic/static service macros.
# define PDL_SVC_FACTORY_DECLARE(X) PDL_FACTORY_DECLARE (PDL_Svc, X)
# define PDL_SVC_INVOKE(X) _make_##X (0)
# define PDL_SVC_NAME(X) _make_##X
# define PDL_SVC_FACTORY_DEFINE(X) PDL_FACTORY_DEFINE (PDL_Svc, X)

# if defined (PDL_HAS_THREADS) && (defined (PDL_HAS_THREAD_SPECIFIC_STORAGE) || defined (PDL_HAS_TSS_EMULATION))
#   define PDL_TSS_TYPE(T) PDL_TSS< T >
#   if defined (PDL_HAS_BROKEN_CONVERSIONS)
#     define PDL_TSS_GET(I, T) (*(I))
#   else
#     define PDL_TSS_GET(I, T) ((I)->operator T * ())
#   endif /* PDL_HAS_BROKEN_CONVERSIONS */
# else
#   define PDL_TSS_TYPE(T) T
#   define PDL_TSS_GET(I, T) (I)
# endif /* PDL_HAS_THREADS && (PDL_HAS_THREAD_SPECIFIC_STORAGE || PDL_HAS_TSS_EMULATIOND) */

# if defined (PDL_LACKS_MODE_MASKS)
// MODE MASKS

// the following macros are for POSIX conformance.

#   if !defined (PDL_HAS_USER_MODE_MASKS)
#     define S_IRWXU 00700         /* read, write, execute: owner. */
#     define S_IRUSR 00400         /* read permission: owner. */
#     define S_IWUSR 00200         /* write permission: owner. */
#     define S_IXUSR 00100         /* execute permission: owner. */
#   endif /* PDL_HAS_USER_MODE_MASKS */
#   define S_IRWXG 00070           /* read, write, execute: group. */
#   define S_IRGRP 00040           /* read permission: group. */
#   define S_IWGRP 00020           /* write permission: group. */
#   define S_IXGRP 00010           /* execute permission: group. */
#   define S_IRWXO 00007           /* read, write, execute: other. */
#   define S_IROTH 00004           /* read permission: other. */
#   define S_IWOTH 00002           /* write permission: other. */
#   define S_IXOTH 00001           /* execute permission: other. */

# endif /* PDL_LACKS_MODE_MASKS */

# if defined (PDL_LACKS_SEMBUF_T)
struct sembuf
{
  unsigned short sem_num; // semaphore #
  short sem_op; // semaphore operation
  short sem_flg; // operation flags
};
# endif /* PDL_LACKS_SEMBUF_T */

# if defined (PDL_HAS_H_ERRNO)
void herror (const char *str);
# endif /* PDL_HAS_H_ERRNO */

# if defined (PDL_LACKS_MSGBUF_T)
struct msgbuf {};
# endif /* PDL_LACKS_MSGBUF_T */

# if defined (PDL_LACKS_STRRECVFD)
struct strrecvfd {};
# endif /* PDL_LACKS_STRRECVFD */

# if defined (PDL_HAS_PROC_FS)
#   include /**/ <sys/procfs.h>
# endif /* PDL_HAS_PROC_FS */

# if defined (PDL_HAS_UNICODE)
#   if defined (PDL_HAS_STANDARD_CPP_LIBRARY) && (PDL_HAS_STANDARD_CPP_LIBRARY != 0)
#     include /**/ <cwchar>
#   elif !defined (__BORLANDC__) && !defined (PDL_HAS_WINCE) /* PDL_HAS_STANDARD_CPP_LIBRARY */
#     include /**/ <wchar.h>
#   endif /* PDL_HAS_STANDARD_CPP_LIBRARY */
# elif defined (PDL_HAS_XPG4_MULTIBYTE_CHAR)
#   include /**/ <wchar.h>
# elif defined (PDL_LACKS_WCHAR_T)
typedef PDL_UINT32 wchar_t;
# endif /* PDL_HAS_UNICODE */

# if defined (PDL_HAS_BROKEN_WRITEV)
typedef struct iovec PDL_WRITEV_TYPE;
# else
typedef const struct iovec PDL_WRITEV_TYPE;
# endif /* PDL_HAS_BROKEN_WRITEV */

# if defined (PDL_HAS_BROKEN_READV)
typedef const struct iovec PDL_READV_TYPE;
# else
typedef struct iovec PDL_READV_TYPE;
# endif /* PDL_HAS_BROKEN_READV */

# if defined (PDL_HAS_BROKEN_SETRLIMIT)
typedef struct rlimit PDL_SETRLIMIT_TYPE;
# else
typedef const struct rlimit PDL_SETRLIMIT_TYPE;
# endif /* PDL_HAS_BROKEN_SETRLIMIT */

# if defined (PDL_MT_SAFE) && (PDL_MT_SAFE != 0)
#   define PDL_MT(X) X
#   if !defined (_REENTRANT)
#     define _REENTRANT
#   endif /* _REENTRANT */
# else
#   define PDL_MT(X)
# endif /* PDL_MT_SAFE */

# if !defined (PDL_DEFAULT_THREAD_PRIORITY)
#   define PDL_DEFAULT_THREAD_PRIORITY (-0x7fffffffL - 1L)
# endif /* PDL_DEFAULT_THREAD_PRIORITY */

// Convenient macro for testing for deadlock, as well as for detecting
// when mutexes fail.
# define PDL_GUARD(MUTEX,OBJ,LOCK) \
  PDL_Guard< MUTEX > OBJ (LOCK); \
    if (OBJ.locked () == 0) return;
# define PDL_GUARD_RETURN(MUTEX,OBJ,LOCK,RETURN) \
  PDL_Guard< MUTEX > OBJ (LOCK); \
    if (OBJ.locked () == 0) return RETURN;
# define PDL_WRITE_GUARD(MUTEX,OBJ,LOCK) \
  PDL_Write_Guard< MUTEX > OBJ (LOCK); \
    if (OBJ.locked () == 0) return;
# define PDL_WRITE_GUARD_RETURN(MUTEX,OBJ,LOCK,RETURN) \
  PDL_Write_Guard< MUTEX > OBJ (LOCK); \
    if (OBJ.locked () == 0) return RETURN;
# define PDL_READ_GUARD(MUTEX,OBJ,LOCK) \
  PDL_Read_Guard< MUTEX > OBJ (LOCK); \
    if (OBJ.locked () == 0) return;
# define PDL_READ_GUARD_RETURN(MUTEX,OBJ,LOCK,RETURN) \
  PDL_Read_Guard< MUTEX > OBJ (LOCK); \
    if (OBJ.locked () == 0) return RETURN;

# if defined (PDL_HAS_POSIX_SEM)
#   include /**/ <semaphore.h>

#   if !defined (SEM_FAILED) && !defined (PDL_LACKS_NAMED_POSIX_SEM)
#     define SEM_FAILED ((sem_t *) -1)
#   endif  /* !SEM_FAILED */

typedef struct
{
  sem_t *sema_;
  // Pointer to semaphore handle.  This is allocated by PDL if we are
  // working with an unnamed POSIX semaphore or by the OS if we are
  // working with a named POSIX semaphore.

  char *name_;
  // Name of the semaphore (if this is non-NULL then this is a named
  // POSIX semaphore, else its an unnamed POSIX semaphore).
} PDL_sema_t;
# endif /* PDL_HAS_POSIX_SEM */

struct cancel_state
{
  int cancelstate;
  // e.g., PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE,
  // PTHREAD_CANCELED.

  int canceltype;
  // e.g., PTHREAD_CANCEL_DEFERRED and PTHREAD_CANCEL_ASYNCHRONOUS.
};

# if defined (PDL_HAS_WINCE)
#   include /**/ <types.h>

typedef DWORD  nlink_t;

// CE's add-on for c-style fstat/stat functionalities.  This struct is
// by no mean complete compared to what you usually find in UNIX
// platforms.  Only members that have direct conversion using Win32's
// BY_HANDLE_FILE_INFORMATION are defined so that users can discover
// non-supported members at compile time.  Time values are of type
// PDL_Time_Value for easy comparison.

struct stat
{
  //  mode_t   st_mode;    // UNIX styled file attribute
  //  nlink_t  st_nlink;   // number of hard links
  PDL_Time_Value st_atime; // time of last access
  PDL_Time_Value st_mtime; // time of last data modification
  off_t st_size;           // file size, in bytes
  //  u_long   st_blksize; // optimal blocksize for I/O
  //  u_long   st_flags;   // user defined flags for file
};

# else /* ! PDL_HAS_WINCE */
#   if defined (PDL_LACKS_SYS_TYPES_H)
#     if ! defined (PDL_PSOS)
  typedef unsigned char u_char;
  typedef unsigned short u_short;
  typedef unsigned int u_int;
  typedef unsigned long u_long;

  typedef unsigned char uchar_t;
  typedef unsigned short ushort_t;
  typedef unsigned int  uint_t;
  typedef unsigned long ulong_t;
#     endif /* ! defined (PDL_PSOS) */
#   else
#     include /**/ <sys/types.h>
#   endif  /* PDL_LACKS_SYS_TYPES_H */

#   if ! defined (PDL_PSOS)
#     include /**/ <sys/stat.h>
#   endif
# endif /* PDL_HAS_WINCE */


#if defined (PDL_HAS_NO_THROW_SPEC)
#   define PDL_THROW_SPEC(X)
#else
# if defined (PDL_HAS_EXCEPTIONS)
#   define PDL_THROW_SPEC(X) throw X
#   if defined (PDL_WIN32)
// @@ MSVC "supports" the keyword but doesn't implement it (Huh?).
//    Therefore, we simply supress the warning for now.
#     pragma warning( disable : 4290 )
#   endif /* PDL_WIN32 */
# else  /* ! PDL_HAS_EXCEPTIONS */
#   define PDL_THROW_SPEC(X)
# endif /* ! PDL_HAS_EXCEPTIONS */
#endif /*PDL_HAS_NO_THROW_SPEC*/


#if defined (PDL_HAS_PRIOCNTL)
  // Need to #include thread.h before #defining THR_BOUND, etc.,
  // when building without threads on SunOS 5.x.
#  if defined (sun)
#    include /**/ <thread.h>
#  endif /* sun */

  // Need to #include these before #defining USYNC_PROCESS on SunOS 5.x.
# include /**/ <sys/rtpriocntl.h>
# include /**/ <sys/tspriocntl.h>
#endif /* PDL_HAS_PRIOCNTL */

# if defined (PDL_HAS_THREADS)

#   if defined (PDL_HAS_STHREADS)
#     include /**/ <synch.h>
#     include /**/ <thread.h>
#     define PDL_SCOPE_PROCESS P_PID
#     define PDL_SCOPE_LWP P_LWPID
#     define PDL_SCOPE_THREAD (PDL_SCOPE_LWP + 1)
#   else
#     define PDL_SCOPE_PROCESS 0
#     define PDL_SCOPE_LWP 1
#     define PDL_SCOPE_THREAD 2
#   endif /* PDL_HAS_STHREADS */

#   if !defined (PDL_HAS_PTHREADS)
#     define PDL_SCHED_OTHER 0
#     define PDL_SCHED_FIFO 1
#     define PDL_SCHED_RR 2
#   endif /* ! PDL_HAS_PTHREADS */

#   if defined (PDL_HAS_PTHREADS)
#     define PDL_SCHED_OTHER SCHED_OTHER
#     define PDL_SCHED_FIFO SCHED_FIFO
#     define PDL_SCHED_RR SCHED_RR

// Definitions for mapping POSIX pthreads draft 6 into 1003.1c names

#     if defined (PDL_HAS_PTHREADS_DRAFT6)
#       define PTHREAD_SCOPE_PROCESS           PTHREAD_SCOPE_LOCAL
#       define PTHREAD_SCOPE_SYSTEM            PTHREAD_SCOPE_GLOBAL
#       define PTHREAD_CREATE_UNDETACHED       0
#       define PTHREAD_CREATE_DETACHED         1
#       define PTHREAD_CREATE_JOINABLE         0
#       define PTHREAD_EXPLICIT_SCHED          0
#       define PTHREAD_MIN_PRIORITY            0
#       define PTHREAD_MAX_PRIORITY            126
#     endif /* PDL_HAS_PTHREADS_DRAFT6 */

// Definitions for THREAD- and PROCESS-LEVEL priorities...some
// implementations define these while others don't.  In order to
// further complicate matters, we don't redefine the default (*_DEF)
// values if they've already been defined, which allows individual
// programs to have their own PDL-wide "default".

// PROCESS-level values
#     if !defined(_UNICOS)
#       define PDL_PROC_PRI_FIFO_MIN  (sched_get_priority_min(SCHED_FIFO))
#       define PDL_PROC_PRI_RR_MIN    (sched_get_priority_min(SCHED_RR))
#       define PDL_PROC_PRI_OTHER_MIN (sched_get_priority_min(SCHED_OTHER))
#     else // UNICOS is missing a sched_get_priority_min() implementation
#       define PDL_PROC_PRI_FIFO_MIN  0
#       define PDL_PROC_PRI_RR_MIN    0
#       define PDL_PROC_PRI_OTHER_MIN 0
#     endif
#     define PDL_PROC_PRI_FIFO_MAX  (sched_get_priority_max(SCHED_FIFO))
#     define PDL_PROC_PRI_RR_MAX    (sched_get_priority_max(SCHED_RR))
#     define PDL_PROC_PRI_OTHER_MAX (sched_get_priority_max(SCHED_OTHER))
#     if !defined(PDL_PROC_PRI_FIFO_DEF)
#       define PDL_PROC_PRI_FIFO_DEF (PDL_PROC_PRI_FIFO_MIN + (PDL_PROC_PRI_FIFO_MAX - PDL_PROC_PRI_FIFO_MIN)/2)
#     endif
#     if !defined(PDL_PROC_PRI_RR_DEF)
#       define PDL_PROC_PRI_RR_DEF (PDL_PROC_PRI_RR_MIN + (PDL_PROC_PRI_RR_MAX - PDL_PROC_PRI_RR_MIN)/2)
#     endif
#     if !defined(PDL_PROC_PRI_OTHER_DEF)
#       define PDL_PROC_PRI_OTHER_DEF (PDL_PROC_PRI_OTHER_MIN + (PDL_PROC_PRI_OTHER_MAX - PDL_PROC_PRI_OTHER_MIN)/2)
#     endif

// THREAD-level values
#     if defined(PRI_FIFO_MIN) && defined(PRI_FIFO_MAX) && defined(PRI_RR_MIN) && defined(PRI_RR_MAX) && defined(PRI_OTHER_MIN) && defined(PRI_OTHER_MAX)
#       define PDL_THR_PRI_FIFO_MIN  (long) PRI_FIFO_MIN
#       define PDL_THR_PRI_FIFO_MAX  (long) PRI_FIFO_MAX
#       define PDL_THR_PRI_RR_MIN    (long) PRI_RR_MIN
#       define PDL_THR_PRI_RR_MAX    (long) PRI_RR_MAX
#       define PDL_THR_PRI_OTHER_MIN (long) PRI_OTHER_MIN
#       define PDL_THR_PRI_OTHER_MAX (long) PRI_OTHER_MAX
#     elif defined (AIX)
#       define PDL_THR_PRI_FIFO_MIN  (long) PRIORITY_MIN
#       define PDL_THR_PRI_FIFO_MAX  (long) PRIORITY_MAX
#       define PDL_THR_PRI_RR_MIN    (long) PRIORITY_MIN
#       define PDL_THR_PRI_RR_MAX    (long) PRIORITY_MAX
#       define PDL_THR_PRI_OTHER_MIN (long) PRIORITY_MIN
#       define PDL_THR_PRI_OTHER_MAX (long) PRIORITY_MAX
#     elif defined (sun)
        // SunOS 5.6 could use sched_get_priority_min/max () for FIFO
        // and RR.  But for OTHER, it returns negative values, which
        // can't be used.  sched_get_priority_min/max () aren't
        // supported in SunOS 5.5.1.
#       define PDL_THR_PRI_FIFO_MIN  (long) 0
#       define PDL_THR_PRI_FIFO_MAX  (long) 59
#       define PDL_THR_PRI_RR_MIN    (long) 0
#       define PDL_THR_PRI_RR_MAX    (long) 59
#       define PDL_THR_PRI_OTHER_MIN (long) 0
#       define PDL_THR_PRI_OTHER_MAX (long) 59
#     else
#       define PDL_THR_PRI_FIFO_MIN  (long) PDL_PROC_PRI_FIFO_MIN
#       define PDL_THR_PRI_FIFO_MAX  (long) PDL_PROC_PRI_FIFO_MAX
#       define PDL_THR_PRI_RR_MIN    (long) PDL_PROC_PRI_RR_MIN
#       define PDL_THR_PRI_RR_MAX    (long) PDL_PROC_PRI_RR_MAX
#       define PDL_THR_PRI_OTHER_MIN (long) PDL_PROC_PRI_OTHER_MIN
#       define PDL_THR_PRI_OTHER_MAX (long) PDL_PROC_PRI_OTHER_MAX
#     endif
#     if !defined(PDL_THR_PRI_FIFO_DEF)
#       define PDL_THR_PRI_FIFO_DEF  ((PDL_THR_PRI_FIFO_MIN + PDL_THR_PRI_FIFO_MAX)/2)
#     endif
#     if !defined(PDL_THR_PRI_RR_DEF)
#       define PDL_THR_PRI_RR_DEF ((PDL_THR_PRI_RR_MIN + PDL_THR_PRI_RR_MAX)/2)
#     endif
#     if !defined(PDL_THR_PRI_OTHER_DEF)
#       define PDL_THR_PRI_OTHER_DEF ((PDL_THR_PRI_OTHER_MIN + PDL_THR_PRI_OTHER_MAX)/2)
#     endif

// Typedefs to help compatibility with Windows NT and Pthreads.
typedef pthread_t PDL_hthread_t;
typedef pthread_t PDL_thread_t;

#     if defined (PDL_HAS_TSS_EMULATION)
        typedef pthread_key_t PDL_OS_thread_key_t;
        typedef u_long PDL_thread_key_t;
#     else  /* ! PDL_HAS_TSS_EMULATION */
        typedef pthread_key_t PDL_thread_key_t;
#     endif /* ! PDL_HAS_TSS_EMULATION */

#     if !defined (PDL_LACKS_COND_T)
typedef pthread_mutex_t PDL_mutex_t;
typedef pthread_cond_t PDL_cond_t;
typedef pthread_condattr_t PDL_condattr_t;
#     endif /* ! PDL_LACKS_COND_T */
typedef pthread_mutex_t PDL_thread_mutex_t;

#     if !defined (PTHREAD_CANCEL_DISABLE)
#       define PTHREAD_CANCEL_DISABLE      0
#     endif /* PTHREAD_CANCEL_DISABLE */

#     if !defined (PTHREAD_CANCEL_ENABLE)
#       define PTHREAD_CANCEL_ENABLE       0
#     endif /* PTHREAD_CANCEL_ENABLE */

#     if !defined (PTHREAD_CANCEL_DEFERRED)
#       define PTHREAD_CANCEL_DEFERRED     0
#     endif /* PTHREAD_CANCEL_DEFERRED */

#     if !defined (PTHREAD_CANCEL_ASYNCHRONOUS)
#       define PTHREAD_CANCEL_ASYNCHRONOUS 0
#     endif /* PTHREAD_CANCEL_ASYNCHRONOUS */

#     define THR_CANCEL_DISABLE      PTHREAD_CANCEL_DISABLE
#     define THR_CANCEL_ENABLE       PTHREAD_CANCEL_ENABLE
#     define THR_CANCEL_DEFERRED     PTHREAD_CANCEL_DEFERRED
#     define THR_CANCEL_ASYNCHRONOUS PTHREAD_CANCEL_ASYNCHRONOUS

#     if !defined (PTHREAD_CREATE_JOINABLE)
#       if defined (PTHREAD_CREATE_UNDETACHED)
#         define PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED
#       else
#         define PTHREAD_CREATE_JOINABLE 0
#       endif /* PTHREAD_CREATE_UNDETACHED */
#     endif /* PTHREAD_CREATE_JOINABLE */

#     if !defined (PTHREAD_CREATE_DETACHED)
#       define PTHREAD_CREATE_DETACHED 1
#     endif /* PTHREAD_CREATE_DETACHED */

#     if !defined (PTHREAD_PROCESS_PRIVATE) && !defined (PDL_HAS_PTHREAD_PROCESS_ENUM)
#       if defined (PTHREAD_MUTEXTYPE_FAST)
#         define PTHREAD_PROCESS_PRIVATE PTHREAD_MUTEXTYPE_FAST
#       else
#         define PTHREAD_PROCESS_PRIVATE 0
#       endif /* PTHREAD_MUTEXTYPE_FAST */
#     endif /* PTHREAD_PROCESS_PRIVATE */

#     if !defined (PTHREAD_PROCESS_SHARED) && !defined (PDL_HAS_PTHREAD_PROCESS_ENUM)
#       if defined (PTHREAD_MUTEXTYPE_FAST)
#         define PTHREAD_PROCESS_SHARED PTHREAD_MUTEXTYPE_FAST
#       else
#         define PTHREAD_PROCESS_SHARED 1
#       endif /* PTHREAD_MUTEXTYPE_FAST */
#     endif /* PTHREAD_PROCESS_SHARED */

#     if defined (PDL_HAS_PTHREADS_DRAFT4)
#       if defined (PTHREAD_PROCESS_PRIVATE)
#         if !defined (USYNC_THREAD)
#         define USYNC_THREAD    PTHREAD_PROCESS_PRIVATE
#         endif /* ! USYNC_THREAD */
#       else
#         if !defined (USYNC_THREAD)
#         define USYNC_THREAD    MUTEX_NONRECURSIVE_NP
#         endif /* ! USYNC_THREAD */
#       endif /* PTHREAD_PROCESS_PRIVATE */

#       if defined (PTHREAD_PROCESS_SHARED)
#         if !defined (USYNC_PROCESS)
#         define USYNC_PROCESS   PTHREAD_PROCESS_SHARED
#         endif /* ! USYNC_PROCESS */
#       else
#         if !defined (USYNC_PROCESS)
#         define USYNC_PROCESS   MUTEX_NONRECURSIVE_NP
#         endif /* ! USYNC_PROCESS */
#       endif /* PTHREAD_PROCESS_SHARED */
#     elif !defined (PDL_HAS_STHREADS)
#       if !defined (USYNC_THREAD)
#       define USYNC_THREAD PTHREAD_PROCESS_PRIVATE
#       endif /* ! USYNC_THREAD */
#       if !defined (USYNC_PROCESS)
#       define USYNC_PROCESS PTHREAD_PROCESS_SHARED
#       endif /* ! USYNC_PROCESS */
#     endif /* PDL_HAS_PTHREADS_DRAFT4 */

#     define THR_BOUND               0x00000001
#     if defined (CHORUS)
#       define THR_NEW_LWP             0x00000000
#     else
#       define THR_NEW_LWP             0x00000002
#     endif /* CHORUS */
#     define THR_DETACHED            0x00000040
#     define THR_SUSPENDED           0x00000080
#     define THR_DAEMON              0x00000100
#     define THR_JOINABLE            0x00010000
#     define THR_SCHED_FIFO          0x00020000
#     define THR_SCHED_RR            0x00040000
#     define THR_SCHED_DEFAULT       0x00080000
#     if defined (PDL_HAS_IRIX62_THREADS)
#       define THR_SCOPE_SYSTEM        0x00100000
#     else
#       define THR_SCOPE_SYSTEM        THR_BOUND
#     endif /* PDL_HAS_IRIX62_THREADS */
#     define THR_SCOPE_PROCESS       0x00200000
#     define THR_INHERIT_SCHED       0x00400000
#     define THR_EXPLICIT_SCHED      0x00800000
#     define THR_SCHED_IO            0x01000000

#     if !defined (PDL_HAS_STHREADS)
#       if !defined (PDL_HAS_POSIX_SEM)
class PDL_Export PDL_sema_t
{
  // = TITLE
  //   This is used to implement semaphores for platforms that support
  //   POSIX pthreads, but do *not* support POSIX semaphores, i.e.,
  //   it's a different type than the POSIX <sem_t>.
friend class PDL_OS;
protected:
  PDL_mutex_t lock_;
  // Serialize access to internal state.

  PDL_cond_t count_nonzero_;
  // Block until there are no waiters.

  u_long count_;
  // Count of the semaphore.

  u_long waiters_;
  // Number of threads that have called <PDL_OS::sema_wait>.
};
#       endif /* !PDL_HAS_POSIX_SEM */

#       if defined (PDL_LACKS_PTHREAD_YIELD) && defined (PDL_HAS_THR_YIELD)
        // If we are on Solaris we can just reuse the existing
        // implementations of these synchronization types.
#         if !defined (PDL_LACKS_RWLOCK_T)
#           include /**/ <synch.h>
typedef rwlock_t PDL_rwlock_t;
#         endif /* !PDL_LACKS_RWLOCK_T */
#         include /**/ <thread.h>
#       endif /* (PDL_LACKS_PTHREAD_YIELD) && defined (PDL_HAS_THR_YIELD) */

#     else
#       if !defined (PDL_HAS_POSIX_SEM)
typedef sema_t PDL_sema_t;
#       endif /* !PDL_HAS_POSIX_SEM */
#     endif /* !PDL_HAS_STHREADS */
#   elif defined (PDL_HAS_STHREADS)
// Solaris threads, without PTHREADS.
// Typedefs to help compatibility with Windows NT and Pthreads.
typedef thread_t PDL_thread_t;
typedef thread_key_t PDL_thread_key_t;
typedef mutex_t PDL_mutex_t;
#     if !defined (PDL_LACKS_RWLOCK_T)
typedef rwlock_t PDL_rwlock_t;
#     endif /* !PDL_LACKS_RWLOCK_T */
#     if !defined (PDL_HAS_POSIX_SEM)
typedef sema_t PDL_sema_t;
#     endif /* !PDL_HAS_POSIX_SEM */

typedef cond_t PDL_cond_t;
struct PDL_Export PDL_condattr_t
{
  int type;
};
typedef PDL_thread_t PDL_hthread_t;
typedef PDL_mutex_t PDL_thread_mutex_t;

#     define THR_CANCEL_DISABLE      0
#     define THR_CANCEL_ENABLE       0
#     define THR_CANCEL_DEFERRED     0
#     define THR_CANCEL_ASYNCHRONOUS 0
#     define THR_JOINABLE            0
#     define THR_SCHED_FIFO          0
#     define THR_SCHED_RR            0
#     define THR_SCHED_DEFAULT       0

#   elif defined (PDL_PSOS)

// Some versions of pSOS provide native mutex support.  For others,
// implement PDL_thread_mutex_t and PDL_mutex_t using pSOS semaphores.
// Either way, the types are all u_longs.
typedef u_long PDL_mutex_t;
typedef u_long PDL_thread_mutex_t;
typedef u_long PDL_thread_t;
typedef u_long PDL_hthread_t;

#if defined (PDL_PSOS_HAS_COND_T)
typedef u_long PDL_cond_t;
struct PDL_Export PDL_condattr_t
{
  int type;
};
#endif


// TCB registers 0-7 are for application use
#     define PSOS_TASK_REG_TSS 0
#     define PSOS_TASK_REG_MAX 7

#     define PSOS_TASK_MIN_PRIORITY   1
#     define PSOS_TASK_MAX_PRIORITY 239

// Key type: the PDL TSS emulation requires the key type be unsigned,
// for efficiency.  Current POSIX and Solaris TSS implementations also
// use unsigned int, so the PDL TSS emulation is compatible with them.
// Native pSOS TSD, where available, uses unsigned long as the key type.
#     if defined (PDL_PSOS_HAS_TSS)
typedef u_long PDL_thread_key_t;
#     else
typedef u_int PDL_thread_key_t;
#     endif /* PDL_PSOS_HAS_TSS */

#     define THR_CANCEL_DISABLE      0  /* thread can never be cancelled */
#     define THR_CANCEL_ENABLE       0      /* thread can be cancelled */
#     define THR_CANCEL_DEFERRED     0      /* cancellation deferred to cancellation point */
#     define THR_CANCEL_ASYNCHRONOUS 0      /* cancellation occurs immediately */

#     define THR_BOUND               0
#     define THR_NEW_LWP             0
#     define THR_DETACHED            0
#     define THR_SUSPENDED           0
#     define THR_DAEMON              0
#     define THR_JOINABLE            0

#     define THR_SCHED_FIFO          0
#     define THR_SCHED_RR            0
#     define THR_SCHED_DEFAULT       0
#     define USYNC_THREAD            T_LOCAL
#     define USYNC_PROCESS           T_GLOBAL

/* from psos.h */
/* #define T_NOPREEMPT     0x00000001   Not preemptible bit */
/* #define T_PREEMPT       0x00000000   Preemptible */
/* #define T_TSLICE        0x00000002   Time-slicing enabled bit */
/* #define T_NOTSLICE      0x00000000   No Time-slicing */
/* #define T_NOASR         0x00000004   ASRs disabled bit */
/* #define T_ASR           0x00000000   ASRs enabled */

/* #define SM_GLOBAL       0x00000001  1 = Global */
/* #define SM_LOCAL        0x00000000  0 = Local */
/* #define SM_PRIOR        0x00000002  Queue by priority */
/* #define SM_FIFO         0x00000000  Queue by FIFO order */

/* #define T_NOFPU         0x00000000   Not using FPU */
/* #define T_FPU           0x00000002   Using FPU bit */

#   elif defined (VXWORKS)
// For mutex implementation using mutual-exclusion semaphores (which
// can be taken recursively).
#     include /**/ <semLib.h>

#     include /**/ <envLib.h>
#     include /**/ <hostLib.h>
#     include /**/ <ioLib.h>
#     include /**/ <remLib.h>
#     include /**/ <selectLib.h>
#     include /**/ <sigLib.h>
#     include /**/ <sockLib.h>
#     include /**/ <sysLib.h>
#     include /**/ <taskLib.h>
#     include /**/ <taskHookLib.h>

extern "C"
struct sockaddr_un {
  short sun_family;    // AF_UNIX.
  char  sun_path[108]; // path name.
};

#     define MAXPATHLEN  1024
#     define MAXNAMLEN   255
#     define NSIG (_NSIGS + 1)

// task options:  the other options are either obsolete, internal, or for
// Fortran or Ada support
#     define VX_UNBREAKABLE        0x0002  /* breakpoints ignored */
#     define VX_FP_TASK            0x0008  /* floating point coprocessor */
#     define VX_PRIVATE_ENV        0x0080  /* private environment support */
#     define VX_NO_STACK_FILL      0x0100  /* do not stack fill for
                                              checkstack () */

#     define THR_CANCEL_DISABLE      0
#     define THR_CANCEL_ENABLE       0
#     define THR_CANCEL_DEFERRED     0
#     define THR_CANCEL_ASYNCHRONOUS 0
#     define THR_BOUND               0
#     define THR_NEW_LWP             0
#     define THR_DETACHED            0
#     define THR_SUSPENDED           0
#     define THR_DAEMON              0
#     define THR_JOINABLE            0
#     define THR_SCHED_FIFO          0
#     define THR_SCHED_RR            0
#     define THR_SCHED_DEFAULT       0
#     define USYNC_THREAD            0
#     define USYNC_PROCESS           1 /* It's all global on VxWorks
                                          (without MMU option). */

#     if !defined (PDL_DEFAULT_SYNCH_TYPE)
 // Types include these options: SEM_Q_PRIORITY, SEM_Q_FIFO,
 // SEM_DELETE_SAFE, and SEM_INVERSION_SAFE.  SEM_Q_FIFO is
 // used as the default because that is VxWorks' default.
#       define PDL_DEFAULT_SYNCH_TYPE SEM_Q_FIFO
#     endif /* ! PDL_DEFAULT_SYNCH_TYPE */

typedef SEM_ID PDL_mutex_t;
// Implement PDL_thread_mutex_t with PDL_mutex_t because there's just
// one process . . .
typedef PDL_mutex_t PDL_thread_mutex_t;
#     if !defined (PDL_HAS_POSIX_SEM)
// Use VxWorks semaphores, wrapped ...
typedef struct
{
  SEM_ID sema_;
  // Semaphore handle.  This is allocated by VxWorks.

  char *name_;
  // Name of the semaphore:  always NULL with VxWorks.
} PDL_sema_t;
#     endif /* !PDL_HAS_POSIX_SEM */
typedef char * PDL_thread_t;
typedef int PDL_hthread_t;
// Key type: the PDL TSS emulation requires the key type be unsigned,
// for efficiency.  (Current POSIX and Solaris TSS implementations also
// use u_int, so the PDL TSS emulation is compatible with them.)
typedef u_int PDL_thread_key_t;

      // Marker for PDL_Thread_Manager to indicate that it allocated
      // an PDL_thread_t.  It is placed at the beginning of the ID.
#     define PDL_THR_ID_ALLOCATED '\022'

#   elif defined (PDL_HAS_WTHREADS)

typedef CRITICAL_SECTION PDL_thread_mutex_t;
typedef struct
{
  int type_; // Either USYNC_THREAD or USYNC_PROCESS
  union
  {
    HANDLE proc_mutex_;
    CRITICAL_SECTION thr_mutex_;
  };
} PDL_mutex_t;

// Wrapper for NT Events.
typedef HANDLE PDL_event_t;

//@@ PDL_USES_WINCE_SEMA_SIMULATION is used to debug
//   semaphore simulation on WinNT.  It should be
//   changed to PDL_USES_HAS_WINCE at some later point.
#     if !defined (PDL_USES_WINCE_SEMA_SIMULATION)
typedef HANDLE PDL_sema_t;
#     else

class PDL_Export PDL_sema_t
{
  // = TITLE
  // Semaphore simulation for Windows CE.
public:
  PDL_thread_mutex_t lock_;
  // Serializes access to <count_>.

  PDL_event_t count_nonzero_;
  // This event is signaled whenever the count becomes non-zero.

  u_int count_;
  // Current count of the semaphore.
};

#     endif /* PDL_USES_WINCE_SEMA_SIMULATION */

// These need to be different values, neither of which can be 0...
#     define USYNC_THREAD 1
#     define USYNC_PROCESS 2

#     define THR_CANCEL_DISABLE      0
#     define THR_CANCEL_ENABLE       0
#     define THR_CANCEL_DEFERRED     0
#     define THR_CANCEL_ASYNCHRONOUS 0
#     define THR_DETACHED            0x02000000 /* ignore in most places */
#     define THR_BOUND               0          /* ignore in most places */
#     define THR_NEW_LWP             0          /* ignore in most places */
#     define THR_DAEMON              0          /* ignore in most places */
#     define THR_JOINABLE            0          /* ignore in most places */
#     define THR_SUSPENDED   CREATE_SUSPENDED
#     define THR_USE_AFX             0x01000000
#     define THR_SCHED_FIFO          0
#     define THR_SCHED_RR            0
#     define THR_SCHED_DEFAULT       0
#   endif /* PDL_HAS_PTHREADS / STHREADS / PSOS / VXWORKS / WTHREADS */

#   if defined (PDL_LACKS_COND_T)
class PDL_Export PDL_cond_t
{
  // = TITLE
  //     This structure is used to implement condition variables on
  //     platforms that lack it natively, such as VxWorks, pSoS, and
  //     Win32.
  //
  // = DESCRIPTION
  //     At the current time, this stuff only works for threads
  //     within the same process.
public:
  friend class PDL_OS;

  long waiters (void) const;
  // Returns the number of waiters.

protected:
  long waiters_;
  // Number of waiting threads.

  PDL_thread_mutex_t waiters_lock_;
  // Serialize access to the waiters count.

  PDL_sema_t sema_;
  // Queue up threads waiting for the condition to become signaled.

#     if defined (VXWORKS) || defined (PDL_PSOS)
  PDL_sema_t waiters_done_;
  // A semaphore used by the broadcast/signal thread to wait for all
  // the waiting thread(s) to wake up and be released from the
  // semaphore.
#     elif defined (PDL_WIN32)
  HANDLE waiters_done_;
  // An auto reset event used by the broadcast/signal thread to wait
  // for the waiting thread(s) to wake up and get a chance at the
  // semaphore.
#     else
#       error "Please implement this feature or check your config.h file!"
#     endif /* VXWORKS || PDL_PSOS */

  size_t was_broadcast_;
  // Keeps track of whether we were broadcasting or just signaling.
};

struct PDL_Export PDL_condattr_t
{
  int type;
};
#   endif /* PDL_LACKS_COND_T */

#   if defined (PDL_LACKS_RWLOCK_T) && !defined (PDL_HAS_PTHREADS_UNIX98_EXT)
struct PDL_Export PDL_rwlock_t
{
  // = TITLE
  //     This is used to implement readers/writer locks on NT,
  //     VxWorks, and POSIX pthreads.
  //
  // = DESCRIPTION
  //     At the current time, this stuff only works for threads
  //     within the same process.
protected:
  friend class PDL_OS;

  PDL_mutex_t lock_;
  // Serialize access to internal state.

  PDL_cond_t waiting_readers_;
  // Reader threads waiting to acquire the lock.

  int num_waiting_readers_;
  // Number of waiting readers.

  PDL_cond_t waiting_writers_;
  // Writer threads waiting to acquire the lock.

  int num_waiting_writers_;
  // Number of waiting writers.

  int ref_count_;
  // Value is -1 if writer has the lock, else this keeps track of the
  // number of readers holding the lock.

  int important_writer_;
  // indicate that a reader is trying to upgrade

  PDL_cond_t waiting_important_writer_;
  // condition for the upgrading reader
};
#   elif defined (PDL_HAS_PTHREADS_UNIX98_EXT)
typedef pthread_rwlock_t PDL_rwlock_t;
#   elif defined (PDL_HAS_STHREADS)
#     include /**/ <synch.h>
typedef rwlock_t PDL_rwlock_t;
#   endif /* PDL_LACKS_RWLOCK_T */

// Define some default thread priorities on all threaded platforms, if
// not defined above or in the individual platform config file.
// PDL_THR_PRI_FIFO_DEF should be used by applications for default
// real-time thread priority.  PDL_THR_PRI_OTHER_DEF should be used
// for non-real-time priority.
#   if !defined(PDL_THR_PRI_FIFO_DEF)
#     if defined (PDL_WTHREADS)
      // It would be more in spirit to use THREAD_PRIORITY_NORMAL.  But,
      // using THREAD_PRIORITY_ABOVE_NORMAL should give preference to the
      // threads in this process, even if the process is not in the
      // REALTIME_PRIORITY_CLASS.
#       define PDL_THR_PRI_FIFO_DEF THREAD_PRIORITY_ABOVE_NORMAL
#     else  /* ! PDL_WTHREADS */
#       define PDL_THR_PRI_FIFO_DEF 0
#     endif /* ! PDL_WTHREADS */
#   endif /* ! PDL_THR_PRI_FIFO_DEF */

#   if !defined(PDL_THR_PRI_OTHER_DEF)
#     if defined (PDL_WTHREADS)
      // It would be more in spirit to use THREAD_PRIORITY_NORMAL.  But,
      // using THREAD_PRIORITY_ABOVE_NORMAL should give preference to the
      // threads in this process, even if the process is not in the
      // REALTIME_PRIORITY_CLASS.
#       define PDL_THR_PRI_OTHER_DEF THREAD_PRIORITY_NORMAL
#     else  /* ! PDL_WTHREADS */
#       define PDL_THR_PRI_OTHER_DEF 0
#     endif /* ! PDL_WTHREADS */
#   endif /* ! PDL_THR_PRI_OTHER_DEF */

#if defined (PDL_HAS_RECURSIVE_MUTEXES)
typedef PDL_thread_mutex_t PDL_recursive_thread_mutex_t;
#else
class PDL_recursive_thread_mutex_t
{
  // = TITLE
  //     Implement a thin C++ wrapper that allows nested acquisition
  //     and release of a mutex that occurs in the same thread.
  //
  // = DESCRIPTION
  //     This implementation is based on an algorithm sketched by Dave
  //     Butenhof <butenhof@zko.dec.com>.  Naturally, I take the
  //     credit for any mistakes ;-)
public:
  PDL_thread_mutex_t nesting_mutex_;
  // Guards the state of the nesting level and thread id.

  PDL_cond_t lock_available_;
  // This condition variable suspends other waiting threads until the
  // mutex is available.

  int nesting_level_;
  // Current nesting level of the recursion.

  PDL_thread_t owner_id_;
  // Current owner of the lock.
};
#endif /* PDL_WIN32 */

# else /* !PDL_HAS_THREADS, i.e., the OS/platform doesn't support threading. */

// Give these things some reasonable value...
#   define PDL_SCOPE_PROCESS 0
#   define PDL_SCOPE_LWP 1
#   define PDL_SCOPE_THREAD 2
#   define PDL_SCHED_OTHER 0
#   define PDL_SCHED_FIFO 1
#   define PDL_SCHED_RR 2
#   if !defined (THR_CANCEL_DISABLE)
#     define THR_CANCEL_DISABLE      0
#   endif /* ! THR_CANCEL_DISABLE */
#   if !defined (THR_CANCEL_ENABLE)
#     define THR_CANCEL_ENABLE       0
#   endif /* ! THR_CANCEL_ENABLE */
#   if !defined (THR_CANCEL_DEFERRED)
#     define THR_CANCEL_DEFERRED     0
#   endif /* ! THR_CANCEL_DEFERRED */
#   if !defined (THR_CANCEL_ASYNCHRONOUS)
#     define THR_CANCEL_ASYNCHRONOUS 0
#   endif /* ! THR_CANCEL_ASYNCHRONOUS */
#   if !defined (THR_JOINABLE)
#     define THR_JOINABLE    0     /* ignore in most places */
#   endif /* ! THR_JOINABLE */
#   if !defined (THR_DETACHED)
#     define THR_DETACHED    0     /* ignore in most places */
#   endif /* ! THR_DETACHED */
#   if !defined (THR_DAEMON)
#     define THR_DAEMON      0     /* ignore in most places */
#   endif /* ! THR_DAEMON */
#   if !defined (THR_BOUND)
#     define THR_BOUND       0     /* ignore in most places */
#   endif /* ! THR_BOUND */
#   if !defined (THR_NEW_LWP)
#     define THR_NEW_LWP     0     /* ignore in most places */
#   endif /* ! THR_NEW_LWP */
#   if !defined (THR_SUSPENDED)
#     define THR_SUSPENDED   0     /* ignore in most places */
#   endif /* ! THR_SUSPENDED */
#   if !defined (THR_SCHED_FIFO)
#     define THR_SCHED_FIFO 0
#   endif /* ! THR_SCHED_FIFO */
#   if !defined (THR_SCHED_RR)
#     define THR_SCHED_RR 0
#   endif /* ! THR_SCHED_RR */
#   if !defined (THR_SCHED_DEFAULT)
#     define THR_SCHED_DEFAULT 0
#   endif /* ! THR_SCHED_DEFAULT */
#   if !defined (USYNC_THREAD)
#     define USYNC_THREAD 0
#   endif /* ! USYNC_THREAD */
#   if !defined (USYNC_PROCESS)
#     define USYNC_PROCESS 0
#   endif /* ! USYNC_PROCESS */
// These are dummies needed for class OS.h
typedef int PDL_cond_t;
struct PDL_Export PDL_condattr_t
{
  int type;
};
typedef int PDL_mutex_t;
typedef int PDL_thread_mutex_t;
typedef int PDL_recursive_thread_mutex_t;
#   if !defined (PDL_HAS_POSIX_SEM) && !defined (PDL_PSOS)
typedef int PDL_sema_t;
#   endif /* !PDL_HAS_POSIX_SEM && !PDL_PSOS */
typedef int PDL_rwlock_t;
typedef int PDL_thread_t;
typedef int PDL_hthread_t;
typedef u_int PDL_thread_key_t;

// Ensure that PDL_THR_PRI_FIFO_DEF and PDL_THR_PRI_OTHER_DEF are
// defined on non-threaded platforms, to support application source
// code compatibility.  PDL_THR_PRI_FIFO_DEF should be used by
// applications for default real-time thread priority.
// PDL_THR_PRI_OTHER_DEF should be used for non-real-time priority.
#   if !defined(PDL_THR_PRI_FIFO_DEF)
#     define PDL_THR_PRI_FIFO_DEF 0
#   endif /* ! PDL_THR_PRI_FIFO_DEF */
#   if !defined(PDL_THR_PRI_OTHER_DEF)
#     define PDL_THR_PRI_OTHER_DEF 0
#   endif /* ! PDL_THR_PRI_OTHER_DEF */

# endif /* PDL_HAS_THREADS */

# if defined (PDL_PSOS)

// Wrapper for NT events on pSOS.
class PDL_Export PDL_event_t
{
  friend class PDL_OS;

protected:

  PDL_mutex_t lock_;
  // Protect critical section.

  PDL_cond_t condition_;
  // Keeps track of waiters.

  int manual_reset_;
  // Specifies if this is an auto- or manual-reset event.

  int is_signaled_;
  // "True" if signaled.

  u_long waiting_threads_;
  // Number of waiting threads.
};

# endif /* PDL_PSOS */

// Standard C Library includes
// NOTE: stdarg.h must be #included before stdio.h on LynxOS.
# include /**/ <stdarg.h>
# if !defined (PDL_HAS_WINCE)
#   include /**/ <assert.h>
#   include /**/ <stdio.h>
// this is a nasty hack to get around problems with the
// pSOS definition of BUFSIZ as the config table entry
// (which is valued using the LC_BUFSIZ value anyway)
#   if defined (PDL_PSOS)
#     if defined (BUFSIZ)
#       undef BUFSIZ
#     endif /* defined (BUFSIZ) */
#     define BUFSIZ LC_BUFSIZ
#   endif /* defined (PDL_PSOS) */

#if defined (PDL_PSOS_DIAB_MIPS)
#undef size_t
typedef unsigned int size_t;
#endif

#   if !defined (PDL_LACKS_NEW_H)
#     include /**/ <new.h>
#   endif /* ! PDL_LACKS_NEW_H */

#   if !defined (PDL_PSOS_DIAB_MIPS)  &&  !defined (VXWORKS)
#   define PDL_DONT_INCLUDE_PDL_SIGNAL_H
#     include /**/ <signal.h>
#   undef PDL_DONT_INCLUDE_PDL_SIGNAL_H
#   endif /* ! PDL_PSOS_DIAB_MIPS && ! VXWORKS */

#   include /**/ <errno.h>

#   if ! defined (PDL_PSOS_DIAB_MIPS)
#   include /**/ <fcntl.h>
#   endif /* ! PDL_PSOS_DIAB_MIPS */
# endif /* PDL_HAS_WINCE */

# include /**/ <limits.h>
# include /**/ <ctype.h>
# if ! defined (PDL_PSOS_DIAB_MIPS)
# include /**/ <string.h>
# include /**/ <stdlib.h>
# endif /* ! PDL_PSOS_DIAB_MIPS */
# include /**/ <float.h>

// This is defined by XOPEN to be a minimum of 16.  POSIX.1g
// also defines this value.  platform-specific config.h can
// override this if need be.
# if !defined (IOV_MAX)
#  define IOV_MAX 16
# endif /* IOV_MAX */

# if defined (PDL_PSOS_SNARFS_HEADER_INFO)
  // Header information snarfed from compiler provided header files
  // that are not included because there is already an identically
  // named file provided with pSOS, which does not have this info
  // from compiler supplied stdio.h
  extern FILE *fdopen(int, const char *);
  extern int getopt(int, char *const *, const char *);
  extern char *tempnam(const char *, const char *);
  extern "C" int fileno(FILE *);

//  #define fileno(stream) ((stream)->_file)

  // from compiler supplied string.h
  extern char *strdup (const char *);

  // from compiler supplied stat.h
  extern mode_t umask (mode_t);
  extern int mkfifo (const char *, mode_t);
  extern int mkdir (const char *, mode_t);

  // from compiler supplied stdlib.h
  extern int putenv (char *);

  int isatty (int h);

# endif /* PDL_PSOS_SNARFS_HEADER_INFO */

# if defined (PDL_NEEDS_SCHED_H)
#   include /**/ <sched.h>
# endif /* PDL_NEEDS_SCHED_H */

# if defined (PDL_HAS_WINCE)
#   define islower iswlower
#   define isdigit iswdigit
# endif /* PDL_HAS_WINCE */

#if !defined (PDL_OSTREAM_TYPE)
# if defined (PDL_LACKS_IOSTREAM_TOTALLY)
#   define PDL_OSTREAM_TYPE FILE
# else  /* ! PDL_LACKS_IOSTREAM_TOTALLY */
#   define PDL_OSTREAM_TYPE ostream
# endif /* ! PDL_LACKS_IOSTREAM_TOTALLY */
#endif /* ! PDL_OSTREAM_TYPE */

#if !defined (PDL_DEFAULT_LOG_STREAM)
# if defined (PDL_LACKS_IOSTREAM_TOTALLY)
#   define PDL_DEFAULT_LOG_STREAM 0
# else  /* ! PDL_LACKS_IOSTREAM_TOTALLY */
#   define PDL_DEFAULT_LOG_STREAM (&cerr)
# endif /* ! PDL_LACKS_IOSTREAM_TOTALLY */
#endif /* ! PDL_DEFAULT_LOG_STREAM */

// If the user wants minimum IOStream inclusion, we will just include
// the forward declarations
# if defined (PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION)
// Forward declaration for streams
#   include "pdl/iosfwd.h"
# else /* PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION */
// Else they will get all the stream header files
#   include "pdl/streams.h"
# endif /* PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION */

# if !defined (PDL_HAS_WINCE)
#   if ! defined (PDL_PSOS_DIAB_MIPS)
#   include /**/ <fcntl.h>
#   endif /* ! PDL_PSOS_DIAB_MIPS */
# endif /* PDL_HAS_WINCE */

// This must come after signal.h is #included.
# if defined (SCO)
#   define SIGIO SIGPOLL
#   include /**/ <sys/regset.h>
# endif /* SCO */

# if defined PDL_HAS_BYTESEX_H
#   include /**/ <bytesex.h>
# endif /* PDL_HAS_BYTESEX_H */
# include "pdl/Basic_Types.h"

/* This should work for linux, solaris 5.6 and above, IRIX, OSF */
# if defined (PDL_HAS_LLSEEK) || defined (PDL_HAS_LSEEK64)
#   if PDL_SIZEOF_LONG == 8
      typedef off_t PDL_LOFF_T;
#   elif defined (__sgi) || defined (AIX) || defined (HPUX)
      typedef off64_t PDL_LOFF_T;
#   elif defined (__sun)
      typedef offset_t PDL_LOFF_T;
#   else
      typedef loff_t PDL_LOFF_T;
#   endif
# endif /* PDL_HAS_LLSEEK || PDL_HAS_LSEEK64 */

// Define some helpful constants.
// Not type-safe, and signed.  For backward compatibility.
#define PDL_ONE_SECOND_IN_MSECS 1000L
#define PDL_ONE_SECOND_IN_USECS 1000000L
#define PDL_ONE_SECOND_IN_NSECS 1000000000L

// Type-safe, and unsigned.
static const PDL_UINT32 PDL_U_ONE_SECOND_IN_MSECS = 1000U;
static const PDL_UINT32 PDL_U_ONE_SECOND_IN_USECS = 1000000U;
static const PDL_UINT32 PDL_U_ONE_SECOND_IN_NSECS = 1000000000U;

# if defined (PDL_HAS_SIG_MACROS)
#   undef sigemptyset
#   undef sigfillset
#   undef sigaddset
#   undef sigdelset
#   undef sigismember
# endif /* PDL_HAS_SIG_MACROS */

// This must come after signal.h is #included.  It's to counteract
// the sigemptyset and sigfillset #defines, which only happen
// when __OPTIMIZE__ is #defined (really!) on Linux.
# if defined (linux) && defined (__OPTIMIZE__)
#   undef sigemptyset
#   undef sigfillset
# endif /* linux && __OPTIMIZE__ */

// Prototypes should come after pdl/Basic_Types.h since some types may
// be used in the prototypes.

#if defined (PDL_LACKS_GETPGID_PROTOTYPE) && \
    !defined (_XOPEN_SOURCE) && !defined (_XOPEN_SOURCE_EXTENDED)
extern "C" pid_t getpgid (pid_t pid);
#endif  /* PDL_LACKS_GETPGID_PROTOTYPE &&
           !_XOPEN_SOURCE && !_XOPEN_SOURCE_EXTENDED */

#if defined (PDL_LACKS_STRPTIME_PROTOTYPE) && !defined (_XOPEN_SOURCE)
extern "C" char *strptime (const char *s, const char *fmt, struct tm *tp);
#endif  /* PDL_LACKS_STRPTIME_PROTOTYPE */

#if defined (PDL_LACKS_STRTOK_R_PROTOTYPE) && !defined (_POSIX_SOURCE)
extern "C" char *strtok_r (char *s, const char *delim, char **save_ptr);
#endif  /* PDL_LACKS_STRTOK_R_PROTOTYPE */

#if !defined (_LARGEFILE64_SOURCE)
# if defined (PDL_LACKS_LSEEK64_PROTOTYPE) && \
     defined (PDL_LACKS_LLSEEK_PROTOTYPE)
#   error Define either PDL_LACKS_LSEEK64_PROTOTYPE or PDL_LACKS_LLSEEK_PROTOTYPE, not both!
# elif defined (PDL_LACKS_LSEEK64_PROTOTYPE)
extern "C" PDL_LOFF_T lseek64 (int fd, PDL_LOFF_T offset, int whence);
# elif defined (PDL_LACKS_LLSEEK_PROTOTYPE)
extern "C" PDL_LOFF_T llseek (int fd, PDL_LOFF_T offset, int whence);
# endif
#endif  /* _LARGEFILE64_SOURCE */

#if defined (PDL_LACKS_PREAD_PROTOTYPE) && (_XOPEN_SOURCE - 0) != 500
// _XOPEN_SOURCE == 500    Single Unix conformance
extern "C" ssize_t pread (int fd,
                          void *buf,
                          size_t nbytes,
                          off_t offset);

extern "C" ssize_t pwrite (int fd,
                           const void *buf,
                           size_t n,
                           off_t offset);
#endif  /* PDL_LACKS_PREAD_PROTOTYPE && (_XOPEN_SOURCE - 0) != 500 */

# if defined (PDL_LACKS_UALARM_PROTOTYPE)
extern "C" u_int ualarm (u_int usecs, u_int interval);
# endif /* PDL_LACKS_UALARM_PROTOTYPE */


# if defined (PDL_HAS_BROKEN_SENDMSG)
typedef struct msghdr PDL_SENDMSG_TYPE;
# else
typedef const struct msghdr PDL_SENDMSG_TYPE;
# endif /* PDL_HAS_BROKEN_SENDMSG */

# if defined (PDL_HAS_BROKEN_RANDR)
// The SunOS 5.4.X version of rand_r is inconsistent with the header
// files...
typedef u_int PDL_RANDR_TYPE;
extern "C" int rand_r (PDL_RANDR_TYPE seed);
# else
#   if defined (HPUX_10)
// HP-UX 10.x's stdlib.h (long *) doesn't match that man page (u_int *)
typedef long PDL_RANDR_TYPE;
#   else
typedef u_int PDL_RANDR_TYPE;
#   endif /* HPUX_10 */
# endif /* PDL_HAS_BROKEN_RANDR */

# if defined (PDL_HAS_UTIME)
#   include /**/ <utime.h>
# endif /* PDL_HAS_UTIME */

# if !defined (PDL_HAS_MSG) && !defined (SCO)
struct msghdr {};
# endif /* PDL_HAS_MSG */

# if defined (PDL_HAS_MSG) && defined (PDL_LACKS_MSG_ACCRIGHTS)
#   if !defined (msg_accrights)
#     undef msg_control
#     define msg_accrights msg_control
#   endif /* ! msg_accrights */

#   if !defined (msg_accrightslen)
#     undef msg_controllen
#     define msg_accrightslen msg_controllen
#   endif /* ! msg_accrightslen */
# endif /* PDL_HAS_MSG && PDL_LACKS_MSG_ACCRIGHTS */

# if !defined (PDL_HAS_SIG_ATOMIC_T)
typedef int sig_atomic_t;
# endif /* !PDL_HAS_SIG_ATOMIC_T */

# if !defined (PDL_HAS_SSIZE_T)
typedef int ssize_t;
# endif /* PDL_HAS_SSIZE_T */

# if defined (PDL_HAS_OLD_MALLOC)
typedef char *PDL_MALLOC_T;
# else
typedef void *PDL_MALLOC_T;
# endif /* PDL_HAS_OLD_MALLOC */

# if defined (PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES)
// Prototypes for both signal() and struct sigaction are consistent..
#   if defined (PDL_HAS_SIG_C_FUNC)
extern "C" {
#   endif /* PDL_HAS_SIG_C_FUNC */
#   if !defined (PDL_PSOS)
typedef void (*PDL_SignalHandler)(int);
typedef void (*PDL_SignalHandlerV)(int);
#   endif /* !defined (PDL_PSOS) */
#   if defined (PDL_HAS_SIG_C_FUNC)
}
#   endif /* PDL_HAS_SIG_C_FUNC */
# elif defined (PDL_HAS_LYNXOS_SIGNALS)
typedef void (*PDL_SignalHandler)(...);
typedef void (*PDL_SignalHandlerV)(...);
# elif defined (PDL_HAS_TANDEM_SIGNALS)
typedef void (*PDL_SignalHandler)(...);
typedef void (*PDL_SignalHandlerV)(...);
# elif defined (PDL_HAS_IRIX_53_SIGNALS)
typedef void (*PDL_SignalHandler)(...);
typedef void (*PDL_SignalHandlerV)(...);
# elif defined (PDL_HAS_SPARCWORKS_401_SIGNALS)
typedef void (*PDL_SignalHandler)(int, ...);
typedef void (*PDL_SignalHandlerV)(int,...);
# elif defined (PDL_HAS_SUNOS4_SIGNAL_T)
typedef void (*PDL_SignalHandler)(...);
typedef void (*PDL_SignalHandlerV)(...);
# elif defined (PDL_HAS_SVR4_SIGNAL_T)
// SVR4 Signals are inconsistent (e.g., see struct sigaction)..
typedef void (*PDL_SignalHandler)(int);
#   if !defined (m88k)     /*  with SVR4_SIGNAL_T */
typedef void (*PDL_SignalHandlerV)(void);
#   else
typedef void (*PDL_SignalHandlerV)(int);
#   endif  //  m88k        /*  with SVR4_SIGNAL_T */
# elif defined (PDL_WIN32)
typedef void (__cdecl *PDL_SignalHandler)(int);
typedef void (__cdecl *PDL_SignalHandlerV)(int);
# elif defined (PDL_HAS_UNIXWARE_SVR4_SIGNAL_T)
typedef void (*PDL_SignalHandler)(int);
typedef void (*PDL_SignalHandlerV)(...);
# else /* This is necessary for some older broken version of cfront */
#   if defined (SIG_PF)
#     define PDL_SignalHandler SIG_PF
#   else
typedef void (*PDL_SignalHandler)(int);
#   endif /* SIG_PF */
typedef void (*PDL_SignalHandlerV)(...);
# endif /* PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES */

# if defined (BUFSIZ)
#   define PDL_STREAMBUF_SIZE BUFSIZ
# else
#   define PDL_STREAMBUF_SIZE 1024
# endif /* BUFSIZ */

# if defined (PDL_WIN32)
// Turn off warnings for /W4
// To resume any of these warning: #pragma warning(default: 4xxx)
// which should be placed after these defines
#   ifndef ALL_WARNINGS
// #pragma warning(disable: 4101)  // unreferenced local variable
#     pragma warning(disable: 4127)  /* constant expression for TRACE/ASSERT */
#     pragma warning(disable: 4134)  /* message map member fxn casts */
#     pragma warning(disable: 4511)  /* private copy constructors are good to have */
#     pragma warning(disable: 4512)  /* private operator= are good to have */
#     pragma warning(disable: 4514)  /* unreferenced inlines are common */
#     pragma warning(disable: 4710)  /* private constructors are disallowed */
#     pragma warning(disable: 4705)  /* statement has no effect in optimized code */
// #pragma warning(disable: 4701)  // local variable *may* be used without init
// #pragma warning(disable: 4702)  // unreachable code caused by optimizations
#     pragma warning(disable: 4791)  /* loss of debugging info in retail version */
// #pragma warning(disable: 4204)  // non-constant aggregate initializer
#     pragma warning(disable: 4275)  /* deriving exported class from non-exported */
#     pragma warning(disable: 4251)  /* using non-exported as public in exported */
#     pragma warning(disable: 4786)  /* identifier was truncated to '255' characters in the browser information */
#     pragma warning(disable: 4097)  /* typedef-name used as synonym for class-name */
#   endif /*!ALL_WARNINGS */

// STRICT type checking in WINDOWS.H enhances type safety for Windows
// programs by using distinct types to represent all the different
// HANDLES in Windows. So for example, STRICT prevents you from
// mistakenly passing an HPEN to a routine expecting an HBITMAP.
// Note that we only use this if we
#   if defined (PDL_HAS_STRICT) && (PDL_HAS_STRICT != 0)
#     if !defined (STRICT)   /* may already be defined */
#       define STRICT
#     endif /* !STRICT */
#   endif /* PDL_HAS_STRICT */

#   if !defined (PDL_HAS_WINCE)
#     include /**/ <sys/timeb.h>
#   endif /* PDL_HAS_WINCE */

// The following defines are used by the PDL Name Server...
#   if !defined (PDL_DEFAULT_NAMESPACE_DIR_W)
#     define PDL_DEFAULT_NAMESPACE_DIR_W L"C:"IDL_FILE_SEPARATORS"temp"
#   endif /* PDL_DEFAULT_NAMESPACE_DIR_W */

#   if !defined (PDL_DEFAULT_NAMESPACE_DIR_A)
#     define PDL_DEFAULT_NAMESPACE_DIR_A "C:"IDL_FILE_SEPARATORS"temp"
#   endif /* PDL_DEFAULT_NAMESPACE_DIR_A */

#   if !defined (PDL_DEFAULT_LOCALNAME_A)
#     define PDL_DEFAULT_LOCALNAME_A "localnames"
#   endif /* PDL_DEFAULT_LOCALNAME_A */

#   if !defined (PDL_DEFAULT_LOCALNAME_W)
#     define PDL_DEFAULT_LOCALNAME_W L"localnames"
#   endif /* PDL_DEFAULT_LOCALNAME_W */

#   if !defined (PDL_DEFAULT_GLOBALNAME_A)
#     define PDL_DEFAULT_GLOBALNAME_A "globalnames"
#   endif /* PDL_DEFAULT_GLOBALNAME_A */

#   if !defined (PDL_DEFAULT_GLOBALNAME_W)
#     define PDL_DEFAULT_GLOBALNAME_W L"globalnames"
#   endif /* PDL_DEFAULT_GLOBALNAME_W */

// Need to work around odd glitches with NT.
#   if !defined (PDL_MAX_DEFAULT_PORT)
#     define PDL_MAX_DEFAULT_PORT 0
#   endif /* PDL_MAX_DEFAULT_PORT */

// We're on WinNT or Win95
#   define PDL_PLATFORM_A "Win32"
#   define PDL_PLATFORM_EXE_SUFFIX_A ".exe"
#   define PDL_PLATFORM_W L"Win32"
#   define PDL_PLATFORM_EXE_SUFFIX_W L".exe"

// Used for PDL_MMAP_Memory_Pool
#   if !defined (PDL_DEFAULT_BACKING_STORE)
#     define PDL_DEFAULT_BACKING_STORE PDL_TEXT ("C:"IDL_FILE_SEPARATORS"temp"IDL_FILE_SEPARATORS"pdl-malloc-XXXXXX")
#   endif /* PDL_DEFAULT_BACKING_STORE */

// Used for PDL_FILE_Connector
#   if !defined (PDL_DEFAULT_TEMP_FILE)
#     define PDL_DEFAULT_TEMP_FILE PDL_TEXT ("C:"IDL_FILE_SEPARATORS"temp"IDL_FILE_SEPARATORS"pdl-file-XXXXXX")
#   endif /* PDL_DEFAULT_TEMP_FILE */

// Used for logging
#   if !defined (PDL_DEFAULT_LOGFILE)
#     define PDL_DEFAULT_LOGFILE "C:"IDL_FILE_SEPARATORS"temp"IDL_FILE_SEPARATORS"logfile"
#   endif /* PDL_DEFAULT_LOGFILE */

// Used for dynamic linking
#   if !defined (PDL_DEFAULT_SVC_CONF)
#     define PDL_DEFAULT_SVC_CONF "."IDL_FILE_SEPARATORS"svc.conf"
#   endif /* PDL_DEFAULT_SVC_CONF */

// The following are #defines and #includes that are specific to
// WIN32.
#   define PDL_STDIN GetStdHandle (STD_INPUT_HANDLE)
#   define PDL_STDOUT GetStdHandle (STD_OUTPUT_HANDLE)
#   define PDL_STDERR GetStdHandle (STD_ERROR_HANDLE)

// Default semaphore key and mutex name
#   if !defined (PDL_DEFAULT_SEM_KEY)
#     define PDL_DEFAULT_SEM_KEY "PDL_SEM_KEY"
#   endif /* PDL_DEFAULT_SEM_KEY */

#   define PDL_INVALID_SEM_KEY 0

#   if defined (PDL_HAS_WINCE)
// @@ WinCE probably doesn't have structural exception support
//    But I need to double check to find this out
#     define PDL_SEH_TRY if (1)
#     define PDL_SEH_EXCEPT(X) while (0)
#     define PDL_SEH_FINALLY if (1)
#   else
#     if defined(__BORLANDC__)
#       if (__BORLANDC__ >= 0x0530) /* Borland C++ Builder 3.0 */
#         define PDL_SEH_TRY try
#         define PDL_SEH_EXCEPT(X) __except(X)
#         define PDL_SEH_FINALLY __finally
#       else
#         define PDL_SEH_TRY if (1)
#         define PDL_SEH_EXCEPT(X) while (0)
#         define PDL_SEH_FINALLY if (1)
#       endif
#     elif defined (__IBMCPP__) && (__IBMCPP__ >= 400)
#         define PDL_SEH_TRY if (1)
#         define PDL_SEH_EXCEPT(X) while (0)
#         define PDL_SEH_FINALLY if (1)
#     else
#       define PDL_SEH_TRY __try
#       define PDL_SEH_EXCEPT(X) __except(X)
#       define PDL_SEH_FINALLY __finally
#     endif /* __BORLANDC__ */
#   endif /* PDL_HAS_WINCE */

#   if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
typedef int (*PDL_SEH_EXCEPT_HANDLER)(void *);
// Prototype of win32 structured exception handler functions.
// They are used to get the exception handling expression or
// as exception handlers.
#   endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */

// The "null" device on Win32.
#   define PDL_DEV_NULL "nul"

// Define the pathname separator characters for Win32 (ugh).
#   define PDL_DIRECTORY_SEPARATOR_STR_A "\\"
#   define PDL_DIRECTORY_SEPARATOR_STR_W L"\\"
#   define PDL_DIRECTORY_SEPARATOR_CHAR_A '\\'
#   define PDL_DIRECTORY_SEPARATOR_CHAR_W L'\\'
#   define PDL_LD_SEARCH_PATH "PATH"
#   define PDL_LD_SEARCH_PATH_SEPARATOR_STR ";"
#   define PDL_DLL_SUFFIX ".dll"
#   define PDL_DLL_PREFIX ""

// Define the name of the environment variable that defines the temp
// directory.
#   define PDL_DEFAULT_TEMP_DIR_ENV_A "TEMP"
#   define PDL_DEFAULT_TEMP_DIR_ENV_W L"TEMP"

// This will help until we figure out everything:
#   define NFDBITS 32 /* only used in unused functions... */
// These two may be used for internal flags soon:
#   define MAP_PRIVATE 1
#   define MAP_SHARED  2
#   define MAP_FIXED   4

#   define RUSAGE_SELF 1

struct shmaddr { };
struct msqid_ds {};

// Fake the UNIX rusage structure.  Perhaps we can add more to this
// later on?
struct rusage
{
  FILETIME ru_utime;
  FILETIME ru_stime;
};

// MMAP flags
#   define PROT_READ PAGE_READONLY
#   define PROT_WRITE PAGE_READWRITE
#   define PROT_RDWR PAGE_READWRITE
/* If we can find suitable use for these flags, here they are:
PAGE_WRITECOPY
PAGE_EXECUTE
PAGE_EXECUTE_READ
PAGE_EXECUTE_READWRITE
PAGE_EXECUTE_WRITECOPY
PAGE_GUARD
PAGE_NOACCESS
PAGE_NOCACHE  */

#   if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
#     include "pdl/ws2tcpip.h"
#   endif /* PDL_HAS_WINSOCK2 */

// error code mapping
#   define ETIME                   ERROR_SEM_TIMEOUT
#   define EWOULDBLOCK             WSAEWOULDBLOCK
#   define EINPROGRESS             WSAEINPROGRESS
#   define EALREADY                WSAEALREADY
#   define ENOTSOCK                WSAENOTSOCK
#   define EDESTADDRREQ            WSAEDESTADDRREQ
#   define EMSGSIZE                WSAEMSGSIZE
#   define EPROTOTYPE              WSAEPROTOTYPE
#   define ENOPROTOOPT             WSAENOPROTOOPT
#   define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#   define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#   define EOPNOTSUPP              WSAEOPNOTSUPP
#   define EPFNOSUPPORT            WSAEPFNOSUPPORT
#   define EAFNOSUPPORT            WSAEAFNOSUPPORT
#   define EADDRINUSE              WSAEADDRINUSE
#   define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#   define ENETDOWN                WSAENETDOWN
#   define ENETUNREACH             WSAENETUNREACH
#   define ENETRESET               WSAENETRESET
#   define ECONNABORTED            WSAECONNABORTED
#   define ECONNRESET              WSAECONNRESET
#   define ENOBUFS                 WSAENOBUFS
#   define EISCONN                 WSAEISCONN
#   define ENOTCONN                WSAENOTCONN
#   define ESHUTDOWN               WSAESHUTDOWN
#   define ETOOMANYREFS            WSAETOOMANYREFS
#   define ETIMEDOUT               WSAETIMEDOUT
#   define ECONNREFUSED            WSAECONNREFUSED
#   define ELOOP                   WSAELOOP
#   define EHOSTDOWN               WSAEHOSTDOWN
#   define EHOSTUNREACH            WSAEHOSTUNREACH
#   define EPROCLIM                WSAEPROCLIM
#   define EUSERS                  WSAEUSERS
#   define EDQUOT                  WSAEDQUOT
#   define ESTALE                  WSAESTALE
#   define EREMOTE                 WSAEREMOTE
// Grrr! These two are already defined by the horrible 'standard'
// library.
// #define ENAMETOOLONG            WSAENAMETOOLONG
// #define ENOTEMPTY               WSAENOTEMPTY

#   if !defined (PDL_HAS_WINCE)
#     include /**/ <time.h>
#     include /**/ <direct.h>
#     include /**/ <process.h>
#     include /**/ <io.h>
#   endif /* PDL_HAS_WINCE */

#   if defined (__BORLANDC__)
#     include /**/ <fcntl.h>
#     define _chdir chdir
#     define _ftime ftime
#     undef _access
#     define _access access
#     if (__BORLANDC__ <= 0x540)
#       define _getcwd getcwd
#       define _stat stat
#     endif
#     define _isatty isatty
#     define _umask umask
#     define _fstat fstat
#     define _stricmp stricmp
#     define _strnicmp strnicmp

#     define _timeb timeb

#     define _O_CREAT O_CREAT
#     define _O_EXCL  O_EXCL
#     define _O_TRUNC O_TRUNC
      // 0x0800 is used for O_APPEND.  0x08 looks free.
#     define _O_TEMPORARY 0x08 // see fcntl.h
#     define _O_RDWR   O_RDWR
#     define _O_WRONLY O_WRONLY
#     define _O_RDONLY O_RDONLY
#     define _O_APPEND O_APPEND
#     define _O_BINARY O_BINARY
#     define _O_TEXT   O_TEXT
#   endif /* __BORLANDC__ */

typedef OVERLAPPED PDL_OVERLAPPED;
typedef DWORD PDL_thread_t;
typedef HANDLE PDL_hthread_t;
typedef long pid_t;
#define PDL_INVALID_PID ((pid_t) -1)
#   if defined (PDL_HAS_TSS_EMULATION)
      typedef DWORD PDL_OS_thread_key_t;
      typedef u_int PDL_thread_key_t;
#   else  /* ! PDL_HAS_TSS_EMULATION */
      typedef DWORD PDL_thread_key_t;
#   endif /* ! PDL_HAS_TSS_EMULATION */

// 64-bit quad-word definitions.
typedef unsigned __int64 PDL_QWORD;
typedef unsigned __int64 PDL_hrtime_t;
inline PDL_QWORD PDL_MAKE_QWORD (DWORD lo, DWORD hi) { return PDL_QWORD (lo) | (PDL_QWORD (hi) << 32); }
inline DWORD PDL_LOW_DWORD  (PDL_QWORD q) { return (DWORD) q; }
inline DWORD PDL_HIGH_DWORD (PDL_QWORD q) { return (DWORD) (q >> 32); }

// Win32 dummies to help compilation.

#   if !defined (__BORLANDC__)
typedef DWORD nlink_t;
typedef int mode_t;
typedef int uid_t;
typedef int gid_t;
#   endif /* __BORLANDC__ */
typedef char *caddr_t;
struct rlimit { };
struct t_call { };
struct t_bind { };
struct t_info { };
struct t_optmgmt { };
struct t_discon { };
struct t_unitdata { };
struct t_uderr { };
struct netbuf { };

// This is for file descriptors.
typedef HANDLE PDL_HANDLE;

// For Win32 compatibility.
typedef SOCKET PDL_SOCKET;

typedef DWORD PDL_exitcode;
#   define PDL_INVALID_HANDLE INVALID_HANDLE_VALUE
#   define PDL_SYSCALL_FAILED 0xFFFFFFFF

// Needed to map calls to NT transparently.
#   define MS_ASYNC 0
#   define MS_INVALIDATE 0

// Reliance on CRT - I don't really like this.

#   define O_NDELAY    1
#   if !defined (MAXPATHLEN)
#     define MAXPATHLEN  _MAX_PATH
#   endif /* !MAXPATHLEN */
#   define MAXNAMLEN   _MAX_FNAME
#   define EADDRINUSE WSAEADDRINUSE

// The ordering of the fields in this struct is important.  It has to
// match those in WSABUF.
struct iovec
{
  size_t iov_len; // byte count to read/write
  char *iov_base; // data to be read/written

  // WSABUF is a Winsock2-only type.
#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
  operator WSABUF &(void) { return *((WSABUF *) this); }
#endif /* defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0) */
};

struct msghdr
{
  sockaddr * msg_name;
  // optional address

  int msg_namelen;
  // size of address

  iovec *msg_iov;
  /* scatter/gather array */

  int msg_iovlen;
  // # elements in msg_iov

  caddr_t msg_accrights;
  // access rights sent/received

  int msg_accrightslen;
};

typedef int PDL_idtype_t;
typedef DWORD PDL_id_t;
#   define PDL_SELF (0)
typedef int PDL_pri_t;

// Dynamic loading-related types - used for dlopen and family.
#   if !defined(RTLD_LAZY)
#     define RTLD_LAZY 1
#   endif /* !RTLD_LAZY */
typedef HINSTANCE PDL_SHLIB_HANDLE;
#   define PDL_SHLIB_INVALID_HANDLE 0
#   define PDL_DEFAULT_SHLIB_MODE 0

# elif defined (PDL_PSOS)

typedef PDL_UINT64 PDL_hrtime_t;

#   if defined (PDL_SIGINFO_IS_SIGINFO_T)
  typedef struct siginfo siginfo_t;
#   endif /* PDL_LACKS_SIGINFO_H */

# else /* !defined (PDL_WIN32) && !defined (PDL_PSOS) */

#   if (defined (PDL_HAS_UNICODE) && (defined (UNICODE)))
typedef const wchar_t *LPCTSTR;
typedef wchar_t *LPTSTR;
typedef wchar_t TCHAR;
#   else
typedef const char *LPCTSTR;
typedef char *LPTSTR;
typedef char TCHAR;
#   endif /* PDL_HAS_UNICODE && UNICODE */

#   if defined (m88k)
#     define RUSAGE_SELF 1
#   endif  /*  m88k */

// Default port is MAX_SHORT.
#   define PDL_MAX_DEFAULT_PORT 65535

// Default semaphore key
#   if !defined (PDL_DEFAULT_SEM_KEY)
#     define PDL_DEFAULT_SEM_KEY 1234
#   endif /* PDL_DEFAULT_SEM_KEY */

#   define PDL_INVALID_SEM_KEY -1

// Define the pathname separator characters for UNIX.
#   define PDL_DIRECTORY_SEPARATOR_STR_A "/"
#   define PDL_DIRECTORY_SEPARATOR_CHAR_A '/'

// Define the name of the environment variable that defines the temp
// directory.
#   define PDL_DEFAULT_TEMP_DIR_ENV_A "TMP"
#   define PDL_DEFAULT_TEMP_DIR_ENV_W L"TMP"

// We're some kind of UNIX...
#   define PDL_PLATFORM_A "UNIX"
#   define PDL_PLATFORM_EXE_SUFFIX_A ""

#   if defined (PDL_HAS_UNICODE)
#     define PDL_DIRECTORY_SEPARATOR_STR_W L"/"
#     define PDL_DIRECTORY_SEPARATOR_CHAR_W L'/'
#     define PDL_PLATFORM_W L"UNIX"
#     define PDL_PLATFORM_EXE_SUFFIX_W L""
#   else
#     define PDL_DIRECTORY_SEPARATOR_STR_W "/"
#     define PDL_DIRECTORY_SEPARATOR_CHAR_W '/'
#     define PDL_PLATFORM_W "UNIX"
#     define PDL_PLATFORM_EXE_SUFFIX_W ""
#   endif /* PDL_HAS_UNICODE */

#   if !defined (PDL_LD_SEARCH_PATH)
#     define PDL_LD_SEARCH_PATH "LD_LIBRARY_PATH"
#   endif /* PDL_LD_SEARCH_PATH */
#   if !defined (PDL_LD_SEARCH_PATH_SEPARATOR_STR)
#     define PDL_LD_SEARCH_PATH_SEPARATOR_STR ":"
#   endif /* PDL_LD_SEARCH_PATH_SEPARATOR_STR */

#   if !defined (PDL_DLL_SUFFIX)
#     define PDL_DLL_SUFFIX ".so"
#   endif /* PDL_DLL_SUFFIX */
#   if !defined (PDL_DLL_PREFIX)
#     define PDL_DLL_PREFIX "lib"
#   endif /* PDL_DLL_PREFIX */

// The following 3 defines are used by the PDL Name Server...
#   if !defined (PDL_DEFAULT_NAMESPACE_DIR_A)
#     define PDL_DEFAULT_NAMESPACE_DIR_A "/tmp"
#   endif /* PDL_DEFAULT_NAMESPACE_DIR_A */

#   if !defined (PDL_DEFAULT_LOCALNAME_A)
#     define PDL_DEFAULT_LOCALNAME_A "localnames"
#   endif /* PDL_DEFAULT_LOCALNAME_A */

#   if !defined (PDL_DEFAULT_GLOBALNAME_A)
#     define PDL_DEFAULT_GLOBALNAME_A "globalnames"
#   endif /* PDL_DEFAULT_GLOBALNAME_A */

#   if defined (PDL_HAS_UNICODE)
#     if !defined (PDL_DEFAULT_NAMESPACE_DIR_W)
#       define PDL_DEFAULT_NAMESPACE_DIR_W L"/tmp"
#     endif /* PDL_DEFAULT_NAMESPACE_DIR_W */
#     if !defined (PDL_DEFAULT_LOCALNAME_W)
#       define PDL_DEFAULT_LOCALNAME_W L"localnames"
#     endif /* PDL_DEFAULT_LOCALNAME_W */
#     if !defined (PDL_DEFAULT_GLOBALNAME_W)
#       define PDL_DEFAULT_GLOBALNAME_W L"globalnames"
#     endif /* PDL_DEFAULT_GLOBALNAME_W */
#   else
#     if !defined (PDL_DEFAULT_NAMESPACE_DIR_W)
#       define PDL_DEFAULT_NAMESPACE_DIR_W "/tmp"
#     endif /* PDL_DEFAULT_NAMESPACE_DIR_W */
#     if !defined (PDL_DEFAULT_LOCALNAME_W)
#       define PDL_DEFAULT_LOCALNAME_W "localnames"
#     endif /* PDL_DEFAULT_LOCALNAME_W */
#     if !defined (PDL_DEFAULT_GLOBALNAME_W)
#       define PDL_DEFAULT_GLOBALNAME_W "globalnames"
#     endif /* PDL_DEFAULT_GLOBALNAME_W */
#   endif /* PDL_HAS_UNICODE */

// Used for PDL_MMAP_Memory_Pool
#   if !defined (PDL_DEFAULT_BACKING_STORE)
#     define PDL_DEFAULT_BACKING_STORE "/tmp/pdl-malloc-XXXXXX"
#   endif /* PDL_DEFAULT_BACKING_STORE */

// Used for PDL_FILE_Connector
#   if !defined (PDL_DEFAULT_TEMP_FILE)
#     define PDL_DEFAULT_TEMP_FILE "/tmp/pdl-file-XXXXXX"
#   endif /* PDL_DEFAULT_TEMP_FILE */

// Used for logging
#   if !defined (PDL_DEFAULT_LOGFILE)
#     define PDL_DEFAULT_LOGFILE "/tmp/logfile"
#   endif /* PDL_DEFAULT_LOGFILE */

// Used for dynamic linking.
#   if !defined (PDL_DEFAULT_SVC_CONF)
#     define PDL_DEFAULT_SVC_CONF "./svc.conf"
#   endif /* PDL_DEFAULT_SVC_CONF */

// The following are #defines and #includes that are specific to UNIX.

#   define PDL_STDIN 0
#   define PDL_STDOUT 1
#   define PDL_STDERR 2

// Be consistent with Winsock naming
typedef int PDL_exitcode;
#   define PDL_INVALID_HANDLE -1
#   define PDL_SYSCALL_FAILED -1

#   define PDL_SEH_TRY if (1)
#   define PDL_SEH_EXCEPT(X) while (0)
#   define PDL_SEH_FINALLY if (1)

# if !defined (PDL_INVALID_PID)
# define PDL_INVALID_PID ((pid_t) -1)
# endif /* PDL_INVALID_PID */

// The "null" device on UNIX.
#   define PDL_DEV_NULL "/dev/null"

// Wrapper for NT events on UNIX.
class PDL_Export PDL_event_t
{
  friend class PDL_OS;
protected:
  PDL_mutex_t lock_;
  // Protect critical section.

  PDL_cond_t condition_;
  // Keeps track of waiters.

  int manual_reset_;
  // Specifies if this is an auto- or manual-reset event.

  int is_signaled_;
  // "True" if signaled.

  u_long waiting_threads_;
  // Number of waiting threads.
};

// Provide compatibility with Windows NT.
typedef int PDL_HANDLE;
// For Win32 compatibility.
typedef PDL_HANDLE PDL_SOCKET;

struct PDL_OVERLAPPED
{
  u_long Internal;
  u_long InternalHigh;
  u_long Offset;
  u_long OffsetHigh;
  PDL_HANDLE hEvent;
};

// Add some typedefs and macros to enhance Win32 conformance...
#   if !defined (LPSECURITY_ATTRIBUTES)
#     define LPSECURITY_ATTRIBUTES int
#   endif /* !defined LPSECURITY_ATTRIBUTES */
#   if !defined (GENERIC_READ)
#     define GENERIC_READ 0
#   endif /* !defined GENERIC_READ */
#   if !defined (FILE_SHARE_READ)
#     define FILE_SHARE_READ 0
#   endif /* !defined FILE_SHARE_READ */
#   if !defined (OPEN_EXISTING)
#     define OPEN_EXISTING 0
#   endif /* !defined OPEN_EXISTING */
#   if !defined (FILE_ATTRIBUTE_NORMAL)
#     define FILE_ATTRIBUTE_NORMAL 0
#   endif /* !defined FILE_ATTRIBUTE_NORMAL */
#   if !defined (MAXIMUM_WAIT_OBJECTS)
#     define MAXIMUM_WAIT_OBJECTS 0
#   endif /* !defined MAXIMUM_WAIT_OBJECTS */
#   if !defined (FILE_FLAG_OVERLAPPED)
#     define FILE_FLAG_OVERLAPPED 0
#   endif /* !defined FILE_FLAG_OVERLAPPED */
#   if !defined (FILE_FLAG_SEQUENTIAL_SCAN)
#     define FILE_FLAG_SEQUENTIAL_SCAN 0
#   endif   /* FILE_FLAG_SEQUENTIAL_SCAN */

#   if defined (PDL_HAS_BROKEN_IF_HEADER)
struct ifafilt;
#   endif /* PDL_HAS_BROKEN_IF_HEADER */

#   if defined (PDL_HAS_AIX_BROKEN_SOCKET_HEADER)
#     undef __cplusplus
#     include /**/ <sys/socket.h>
#     define __cplusplus
#   else
#     include /**/ <sys/socket.h>
#   endif /* PDL_HAS_AIX_BROKEN_SOCKET_HEADER */

extern "C"
{
#   if defined (VXWORKS)
  struct  hostent {
    char    *h_name;        /* official name of host */
    char    **h_aliases;    /* aliases:  not used on VxWorks */
    int     h_addrtype;     /* host address type */
    int     h_length;       /* address length */
    char    **h_addr_list;  /* (first, only) address from name server */
#     define h_addr h_addr_list[0]   /* the first address */
  };
#   elif defined (PDL_HAS_CYGWIN32_SOCKET_H)
#     include /**/ <cygwin32/socket.h>
#   else
#     if defined (PDL_HAS_STL_QUEUE_CONFLICT)
#       define queue _Queue_
#     endif /* PDL_HAS_STL_QUEUE_CONFLICT */
#     include /**/ <netdb.h>
#     if defined (PDL_HAS_STL_QUEUE_CONFLICT)
#       undef queue
#     endif /* PDL_HAS_STL_QUEUE_CONFLICT */
#   endif /* VXWORKS */

// This part if to avoid STL name conflict with the map structure
// in net/if.h.
#   if defined (PDL_HAS_STL_MAP_CONFLICT)
#     define map _Resource_Allocation_Map_
#   endif /* PDL_HAS_STL_MAP_CONFLICT */
#   include /**/ <net/if.h>
#   if defined (PDL_HAS_STL_MAP_CONFLICT)
#     undef map
#   endif /* PDL_HAS_STL_MAP_CONFLICT */

#   if defined (PDL_HAS_STL_QUEUE_CONFLICT)
#     define queue _Queue_
#   endif /* PDL_HAS_STL_QUEUE_CONFLICT */
#   include /**/ <netinet/in.h>
#   if defined (PDL_HAS_STL_QUEUE_CONFLICT)
#     undef queue
#   endif /* PDL_HAS_STL_QUEUE_CONFLICT */

#   if defined (VXWORKS)
      // Work around a lack of ANSI prototypes for these functions on VxWorks.
      unsigned long  inet_addr (const char *);
      char           *inet_ntoa (const struct in_addr);
      struct in_addr inet_makeaddr (const int, const int);
      unsigned long  inet_network (const char *);
#   else  /* ! VXWORKS */
#     include /**/ <arpa/inet.h>
#   endif /* ! VXWORKS */
}
#   if !defined (PDL_LACKS_TCP_H)
#     include /**/ <netinet/tcp.h>
#   endif /* PDL_LACKS_TCP_H */

#   if defined (__Lynx__)
#     ifndef howmany
#       define howmany(x, y)   (((x)+((y)-1))/(y))
#     endif /* howmany */
#   endif /* __Lynx__ */

#   if defined (CHORUS)
#     include /**/ <chorus.h>
#     include /**/ <cx/select.h>
#     include /**/ <sys/uio.h>
#     include /**/ <time.h>
#     include /**/ <stdfileio.h>
#     include /**/ <am/afexec.h>
#     include /**/ <sys/types.h>
#     include /**/ <sys/signal.h>
#     include /**/ <sys/wait.h>
#     include /**/ <pwd.h>
#     include /**/ <timer/chBench.h>
extern_C int      getgid          __((void));
extern_C int      getuid          __((void));
extern_C char*    getcwd          __((char* buf, size_t size));
extern_C int      pipe            __((int* fildes));
extern_C int      gethostname     __((char*, size_t));

// This must come after limits.h is included
#     define MAXPATHLEN _POSIX_PATH_MAX

typedef cx_fd_mask fd_mask;
#     ifndef howmany
#       define howmany(x, y)   (((x)+((y)-1))/(y))
#     endif /* howmany */
typedef void (*__sighandler_t)(int); // keep Signal compilation happy
#   elif defined (CYGWIN32)
#     include /**/ <sys/uio.h>
#     include /**/ <sys/file.h>
#     include /**/ <sys/time.h>
#     include /**/ <sys/resource.h>
#     include /**/ <sys/wait.h>
#     include /**/ <pwd.h>
#   elif defined (__QNX__)
#     include /**/ <sys/uio.h>
#     include /**/ <sys/ipc.h>
#     include /**/ <sys/sem.h>
#     include /**/ <sys/time.h>
#     include /**/ <sys/wait.h>
#     include /**/ <pwd.h>
      // sets O_NDELAY
#     include /**/ <unix.h>
#     include /**/ <sys/param.h> /* for NBBY */
      typedef long fd_mask;
#     if !defined (NFDBITS)
#       define NFDBITS (sizeof(fd_mask) * NBBY)        /* bits per mask */
#     endif /* ! NFDBITS */
#     if !defined (howmany)
#       define howmany(x, y)   (((x)+((y)-1))/(y))
#     endif /* ! howmany */
#   elif ! defined (VXWORKS)
#     include /**/ <sys/uio.h>
#     include /**/ <sys/ipc.h>
#     include /**/ <sys/shm.h>
#     include /**/ <sys/sem.h>
#     include /**/ <sys/file.h>
#     include /**/ <sys/time.h>
#     include /**/ <sys/resource.h>
#     include /**/ <sys/wait.h>
#     include /**/ <pwd.h>
#   endif /* ! VXWORKS */
#   include /**/ <sys/ioctl.h>
#   include /**/ <dirent.h>

// IRIX5 defines bzero() in this odd file...
#   if defined (PDL_HAS_BSTRING)
#     include /**/ <bstring.h>
#   endif /* PDL_HAS_BSTRING */

// AIX defines bzero() in this odd file...
#   if defined (PDL_HAS_STRINGS)
#     include /**/ <strings.h>
#   endif /* PDL_HAS_STRINGS */

#   if defined (PDL_HAS_TERM_IOCTLS)
#     if defined (__QNX__)
#       include /**/ <termios.h>
#     else  /* ! __QNX__ */
#       include /**/ <sys/termios.h>
#     endif /* ! __QNX__ */
#   endif /* PDL_HAS_TERM_IOCTLS */

#   if !defined (PDL_LACKS_UNISTD_H)
#     include /**/ <unistd.h>
#   endif /* PDL_LACKS_UNISTD_H */

#   if defined (PDL_HAS_AIO_CALLS)
#     include /**/ <aio.h>
#   endif /* PDL_HAS_AIO_CALLS */

#   if !defined (PDL_LACKS_PARAM_H)
#     include /**/ <sys/param.h>
#   endif /* PDL_LACKS_PARAM_H */

#   if !defined (PDL_LACKS_UNIX_DOMAIN_SOCKETS) && !defined (VXWORKS)
#     include /**/ <sys/un.h>
#   endif /* PDL_LACKS_UNIX_DOMAIN_SOCKETS */

#   if defined (PDL_HAS_SIGINFO_T)
#     if !defined (PDL_LACKS_SIGINFO_H)
#       if defined (__QNX__)
#         include /**/ <sys/siginfo.h>
#       else  /* ! __QNX__ */
#         include /**/ <siginfo.h>
#       endif /* ! __QNX__ */
#     endif /* PDL_LACKS_SIGINFO_H */
#     if !defined (PDL_LACKS_UCONTEXT_H)
#       include /**/ <ucontext.h>
#     endif /* PDL_LACKS_UCONTEXT_H */
#   endif /* PDL_HAS_SIGINFO_T */

#   if defined (PDL_HAS_POLL)
#     include /**/ <poll.h>
#   endif /* PDL_HAS_POLL */

#   if defined (PDL_HAS_STREAMS)
#     if defined (AIX)
#       if !defined (_XOPEN_EXTENDED_SOURCE)
#         define _XOPEN_EXTENDED_SOURCE
#       endif /* !_XOPEN_EXTENDED_SOURCE */
#       include /**/ <stropts.h>
#       undef _XOPEN_EXTENDED_SOURCE
#     else
#       include /**/ <stropts.h>
#     endif /* AIX */
#   endif /* PDL_HAS_STREAMS */

#   if defined (PDL_LACKS_T_ERRNO)
extern int t_errno;
#   endif /* PDL_LACKS_T_ERRNO */

#   if defined (DIGITAL_UNIX)
  // sigwait is yet another macro on Digital UNIX 4.0, just causing
  // trouble when introducing member functions with the same name.
  // Thanks to Thilo Kielmann" <kielmann@informatik.uni-siegen.de> for
  // this fix.
#     if defined  (__DECCXX_VER)
#       undef sigwait
        // cxx on Digital Unix 4.0 needs this declaration.  With it,
        // <::_Psigwait> works with cxx -pthread.  g++ does _not_ need
        // it.
        extern "C" int _Psigwait __((const sigset_t *set, int *sig));
#     endif /* __DECCXX_VER */
#   elif !defined (PDL_HAS_SIGWAIT)
  extern "C" int sigwait (sigset_t *set);
#   endif /* ! DIGITAL_UNIX && ! PDL_HAS_SIGWAIT */

#   if defined (PDL_HAS_SELECT_H)
#     include /**/ <sys/select.h>
#   endif /* PDL_HAS_SELECT_H */

#   if defined (PDL_HAS_ALLOCA_H)
#     include /**/ <alloca.h>
#   endif /* PDL_HAS_ALLOCA_H */

#   if defined (PDL_HAS_TIUSER_H) || defined (PDL_HAS_XTI) || defined (PDL_HAS_FORE_ATM_XTI)
#     if defined (PDL_HAS_BROKEN_XTI_MACROS)
#       undef TCP_NODELAY
#       undef TCP_MAXSEG
#     endif /* PDL_HAS_BROKEN_XTI_MACROS */
#     if defined (PDL_HAS_TIUSER_H_BROKEN_EXTERN_C)
extern "C" {
#     endif /* PDL_HAS_TIUSER_H_BROKEN_EXTERN_C */
#     if defined (PDL_HAS_FORE_ATM_XTI)
#       include /**/ <fore_xti/xti_user_types.h>
#       include /**/ <fore_xti/xti.h>
#       include /**/ <fore_xti/xti_atm.h>
#       include /**/ <fore_xti/netatm/atm.h>
#       include /**/ <fore_xti/ans.h>
#     elif defined (PDL_HAS_TIUSER_H)
#       include /**/ <tiuser.h>
#     elif defined (PDL_HAS_SYS_XTI_H)
#         define class pdl_xti_class
#         include /**/ <sys/xti.h>
#         undef class
#     else
#         include /**/ <xti.h>
#     endif /* PDL_HAS_FORE_ATM_XTI */
#     if defined (PDL_HAS_TIUSER_H_BROKEN_EXTERN_C)
}
#     endif /* PDL_HAS_TIUSER_H_BROKEN_EXTERN_C */
#   endif /* PDL_HAS_TIUSER_H || PDL_HAS_XTI */

/* Set the proper handle type for dynamically-loaded libraries. */
/* Also define a default 'mode' for loading a library - the names and values */
/* differ between OSes, so if you write code that uses the mode, be careful */
/* of the platform differences. */
#   if defined (PDL_HAS_SVR4_DYNAMIC_LINKING)
#    if defined (PDL_HAS_DLFCN_H_BROKEN_EXTERN_C)
extern "C" {
#     endif /* PDL_HAS_DLFCN_H_BROKEN_EXTERN_C */
#     include /**/ <dlfcn.h>
#     if defined (PDL_HAS_DLFCN_H_BROKEN_EXTERN_C)
}
#     endif /* PDL_HAS_DLFCN_H_BROKEN_EXTERN_C */
  typedef void *PDL_SHLIB_HANDLE;
#   define PDL_SHLIB_INVALID_HANDLE 0
#     if !defined (RTLD_LAZY)
#       define RTLD_LAZY 1
#     endif /* !RTLD_LAZY */
#   if defined (__KCC)
#   define PDL_DEFAULT_SHLIB_MODE RTLD_LAZY | RTLD_GROUP | RTLD_NODELETE
#   else
#   define PDL_DEFAULT_SHLIB_MODE RTLD_LAZY
#   endif /* KCC */
#   elif defined (__hpux)
#     if defined(__GNUC__) || __cplusplus >= 199707L
#       include /**/ <dl.h>
#     else
#       include /**/ <cxxdl.h>
#     endif /* (g++ || HP aC++) vs. HP C++ */
  typedef shl_t PDL_SHLIB_HANDLE;
#   define PDL_SHLIB_INVALID_HANDLE 0
#   define PDL_DEFAULT_SHLIB_MODE BIND_DEFERRED
#   else
#     if !defined(RTLD_LAZY)
#       define RTLD_LAZY 1
#     endif /* !RTLD_LAZY */
  typedef void *PDL_SHLIB_HANDLE;
#   define PDL_SHLIB_INVALID_HANDLE 0
#   define PDL_DEFAULT_SHLIB_MODE RTLD_LAZY

#   endif /* PDL_HAS_SVR4_DYNAMIC_LINKING */

#   if defined (PDL_HAS_SOCKIO_H)
#     include /**/ <sys/sockio.h>
#   endif /* PDL_HAS_SOCKIO_ */

// There must be a better way to do this...
#   if !defined (RLIMIT_NOFILE)
#     if defined (linux) || defined (AIX) || defined (SCO)
#       if defined (RLIMIT_OFILE)
#         define RLIMIT_NOFILE RLIMIT_OFILE
#       else
#         define RLIMIT_NOFILE 200
#       endif /* RLIMIT_OFILE */
#     endif /* defined (linux) || defined (AIX) || defined (SCO) */
#   endif /* RLIMIT_NOFILE */

#   if !defined (PDL_HAS_TLI_PROTOTYPES)
// Define PDL_TLI headers for systems that don't prototype them....
extern "C"
{
  int t_accept(int fildes, int resfd, struct t_call *call);
  char *t_alloc(int fildes, int struct_type, int fields);
  int t_bind(int fildes, struct t_bind *req, struct t_bind *ret);
  int t_close(int fildes);
  int t_connect(int fildes, struct t_call *sndcall,
                struct t_call *rcvcall);
  void t_error(const char *errmsg);
  int t_free(char *ptr, int struct_type);
  int t_getinfo(int fildes, struct t_info *info);
  int t_getname (int fildes, struct netbuf *namep, int type);
  int t_getstate(int fildes);
  int t_listen(int fildes, struct t_call *call);
  int t_look(int fildes);
  int t_open(char *path, int oflag, struct t_info *info);
  int t_optmgmt(int fildes, struct t_optmgmt *req,
                struct t_optmgmt *ret);
  int t_rcv(int fildes, char *buf, u_int nbytes, int *flags);
  int t_rcvconnect(int fildes, struct t_call *call);
  int t_rcvdis(int fildes, struct t_discon *discon);
  int t_rcvrel(int fildes);
  int t_rcvudata(int fildes, struct t_unitdata *unitdata, int *flags);
  int t_rcvuderr(int fildes, struct t_uderr *uderr);
  int t_snd(int fildes, char *buf, u_int nbytes, int flags);
  int t_snddis(int fildes, struct t_call *call);
  int t_sndrel(int fildes);
  int t_sndudata(int fildes, struct t_unitdata *unitdata);
  int t_sync(int fildes);
  int t_unbind(int fildes);
}
#   endif /* !PDL_HAS_TLI_PROTOTYPES */

#   if defined (PDL_LACKS_MMAP)
#     define PROT_READ 0
#     define PROT_WRITE 0
#     define PROT_EXEC 0
#     define PROT_NONE 0
#     define PROT_RDWR 0
#     define MAP_PRIVATE 0
#     define MAP_SHARED 0
#     define MAP_FIXED 0
#   endif /* PDL_LACKS_MMAP */

// Fixes a problem with HP/UX.
#   if defined (PDL_HAS_BROKEN_MMAP_H)
extern "C"
{
#     include /**/ <sys/mman.h>
}
#   elif !defined (PDL_LACKS_MMAP)
#     include /**/ <sys/mman.h>
#   endif /* PDL_HAS_BROKEN_MMAP_H */

// OSF1 has problems with sys/msg.h and C++...
#   if defined (PDL_HAS_BROKEN_MSG_H)
#     define _KERNEL
#   endif /* PDL_HAS_BROKEN_MSG_H */
#   if !defined (PDL_LACKS_SYSV_MSG_H)
#     include /**/ <sys/msg.h>
#   endif /* PDL_LACKS_SYSV_MSG_H */
#   if defined (PDL_HAS_BROKEN_MSG_H)
#     undef _KERNEL
#   endif /* PDL_HAS_BROKEN_MSG_H */

#   if defined (PDL_LACKS_SYSV_MSQ_PROTOS)
extern "C"
{
  int msgget (key_t, int);
  int msgrcv (int, void *, size_t, long, int);
  int msgsnd (int, const void *, size_t, int);
  int msgctl (int, int, struct msqid_ds *);
}
#   endif /* PDL_LACKS_SYSV_MSQ_PROTOS */

#   if defined (PDL_HAS_PRIOCNTL)
#     include /**/ <sys/priocntl.h>
#   endif /* PDL_HAS_PRIOCNTL */

#   if defined (PDL_HAS_IDTYPE_T)
  typedef idtype_t PDL_idtype_t;
#   else
  typedef int PDL_idtype_t;
#   endif /* PDL_HAS_IDTYPE_T */

#   if defined (PDL_HAS_STHREADS) || defined (DIGITAL_UNIX)
#     if defined (PDL_LACKS_PRI_T)
    typedef int pri_t;
#     endif /* PDL_LACKS_PRI_T */
  typedef id_t PDL_id_t;
#     define PDL_SELF P_MYID
  typedef pri_t PDL_pri_t;
#   else  /* ! PDL_HAS_STHREADS && ! DIGITAL_UNIX */
  typedef long PDL_id_t;
#     define PDL_SELF (-1)
  typedef short PDL_pri_t;
#   endif /* ! PDL_HAS_STHREADS && ! DIGITAL_UNIX */

#   if defined (PDL_HAS_HI_RES_TIMER) &&  !defined (PDL_LACKS_LONGLONG_T)
  /* hrtime_t is defined on systems (Suns) with PDL_HAS_HI_RES_TIMER */
  typedef hrtime_t PDL_hrtime_t;
#   else  /* ! PDL_HAS_HI_RES_TIMER  ||  PDL_LACKS_LONGLONG_T */
  typedef PDL_UINT64 PDL_hrtime_t;
#   endif /* ! PDL_HAS_HI_RES_TIMER  ||  PDL_LACKS_LONGLONG_T */

# endif /* !defined (PDL_WIN32) && !defined (PDL_PSOS) */

// defined Win32 specific macros for UNIX platforms
# if !defined (O_BINARY)
#   define O_BINARY 0
# endif /* O_BINARY */
# if !defined (_O_BINARY)
#   define _O_BINARY O_BINARY
# endif /* _O_BINARY */
# if !defined (O_TEXT)
#   define O_TEXT 0
# endif /* O_TEXT */
# if !defined (_O_TEXT)
#   define _O_TEXT O_TEXT
# endif /* _O_TEXT */
# if !defined (O_RAW)
#   define O_RAW 0
# endif /* O_RAW */
# if !defined (_O_RAW)
#   define _O_RAW O_RAW
# endif /* _O_RAW */

# if !defined (PDL_DEFAULT_SYNCH_TYPE)
#   define PDL_DEFAULT_SYNCH_TYPE USYNC_THREAD
# endif /* ! PDL_DEFAULT_SYNCH_TYPE */

# if !defined (PDL_MAP_PRIVATE)
#   define PDL_MAP_PRIVATE MAP_PRIVATE
# endif /* ! PDL_MAP_PRIVATE */

# if !defined (PDL_MAP_SHARED)
#   define PDL_MAP_SHARED MAP_SHARED
# endif /* ! PDL_MAP_SHARED */

# if !defined (PDL_MAP_FIXED)
#   define PDL_MAP_FIXED MAP_FIXED
# endif /* ! PDL_MAP_FIXED */

# if defined (PDL_LACKS_UTSNAME_T)
#   define _SYS_NMLN 257
struct utsname
{
  TCHAR sysname[_SYS_NMLN];
  TCHAR nodename[_SYS_NMLN];
  TCHAR release[_SYS_NMLN];
  TCHAR version[_SYS_NMLN];
  TCHAR machine[_SYS_NMLN];
};
# else
#   include /**/ <sys/utsname.h>
# endif /* PDL_LACKS_UTSNAME_T */

// Increase the range of "address families".  Please note that this
// must appear _after_ the include of sys/socket.h, for the AF_FILE
// definition on Linux/glibc2.
# define AF_ANY (-1)
# define AF_SPIPE (AF_MAX + 1)
# if !defined (AF_FILE)
#   define AF_FILE (AF_MAX + 2)
# endif /* ! AF_FILE */
# define AF_DEV (AF_MAX + 3)
# define AF_UPIPE (AF_SPIPE)

# if defined (PDL_SELECT_USES_INT)
typedef int PDL_FD_SET_TYPE;
# else
typedef fd_set PDL_FD_SET_TYPE;
# endif /* PDL_SELECT_USES_INT */

# if !defined (MAXNAMELEN)
#   if defined (FILENAME_MAX)
#     define MAXNAMELEN FILENAME_MAX
#   else
#     define MAXNAMELEN 256
#   endif /* FILENAME_MAX */
# endif /* MAXNAMELEN */

# if !defined(MAXHOSTNAMELEN)
#   define MAXHOSTNAMELEN  256
# endif /* MAXHOSTNAMELEN */

// Define INET string length constants if they haven't been defined
//
// for IPv4 dotted-decimal
# if !defined (INET_ADDRSTRLEN)
#   define INET_ADDRSTRLEN 16
# endif /* INET_ADDRSTRLEN */
//
// for IPv6 hex string
# if !defined (INET6_ADDRSTRLEN)
#   define INET6_ADDRSTRLEN 46
# endif /* INET6_ADDRSTRLEN */

# if defined (PDL_LACKS_SIGSET)
typedef u_int sigset_t;
# endif /* PDL_LACKS_SIGSET */

# if defined (PDL_LACKS_SIGACTION)
struct sigaction
{
  int sa_flags;
  PDL_SignalHandlerV sa_handler;
  sigset_t sa_mask;
};
# endif /* PDL_LACKS_SIGACTION */

# if !defined (SIGHUP)
#   define SIGHUP 0
# endif /* SIGHUP */

# if !defined (SIGINT)
#   define SIGINT 0
# endif /* SIGINT */

# if !defined (SIGSEGV)
#   define SIGSEGV 0
# endif /* SIGSEGV */

# if !defined (SIGIO)
#   define SIGIO 0
# endif /* SIGSEGV */

# if !defined (SIGUSR1)
#   define SIGUSR1 0
# endif /* SIGUSR1 */

# if !defined (SIGUSR2)
#   define SIGUSR2 0
# endif /* SIGUSR2 */

# if !defined (SIGCHLD)
#   define SIGCHLD 0
# endif /* SIGCHLD */

# if !defined (SIGCLD)
#   define SIGCLD SIGCHLD
# endif /* SIGCLD */

# if !defined (SIGQUIT)
#   define SIGQUIT 0
# endif /* SIGQUIT */

# if !defined (SIGPIPE)
#   define SIGPIPE 0
# endif /* SIGPIPE */

# if !defined (SIGALRM)
#   define SIGALRM 0
# endif /* SIGALRM */

# if !defined (SIG_DFL)
#   if defined (PDL_PSOS_DIAB_MIPS) || defined (PDL_PSOS_DIAB_PPC)
#     define SIG_DFL ((void *) 0)
#   else
#     define SIG_DFL ((__sighandler_t) 0)
#   endif
# endif /* SIG_DFL */

# if !defined (SIG_IGN)
#   if defined (PDL_PSOS_DIAB_MIPS) || defined (PDL_PSOS_DIAB_PPC)
#     define SIG_IGN ((void *) 1)     /* ignore signal */
#   else
#     define SIG_IGN ((__sighandler_t) 1)     /* ignore signal */
#   endif
# endif /* SIG_IGN */

# if !defined (SIG_ERR)
#   if defined (PDL_PSOS_DIAB_MIPS) || defined (PDL_PSOS_DIAB_PPC)
#     define SIG_ERR ((void *) -1)    /* error return from signal */
#   else
#     define SIG_ERR ((__sighandler_t) -1)    /* error return from signal */
#   endif
# endif /* SIG_ERR */

# if !defined (O_NONBLOCK)
#   define O_NONBLOCK  1
# endif /* O_NONBLOCK  */

# if !defined (SIG_BLOCK)
#   define SIG_BLOCK   1
# endif /* SIG_BLOCK   */

# if !defined (SIG_UNBLOCK)
#   define SIG_UNBLOCK 2
# endif /* SIG_UNBLOCK */

# if !defined (SIG_SETMASK)
#   define SIG_SETMASK 3
# endif /* SIG_SETMASK */

# if !defined (IPC_CREAT)
#   define IPC_CREAT 0
# endif /* IPC_CREAT */

# if !defined (IPC_NOWAIT)
#   define IPC_NOWAIT 0
# endif /* IPC_NOWAIT */

# if !defined (IPC_RMID)
#   define IPC_RMID 0
# endif /* IPC_RMID */

# if !defined (IPC_EXCL)
#   define IPC_EXCL 0
# endif /* IPC_EXCL */

# if !defined (IP_DROP_MEMBERSHIP)
#   define IP_DROP_MEMBERSHIP 0
# endif /* IP_DROP_MEMBERSHIP */

# if !defined (IP_ADD_MEMBERSHIP)
#   define IP_ADD_MEMBERSHIP 0
#   define PDL_LACKS_IP_ADD_MEMBERSHIP
# endif /* IP_ADD_MEMBERSHIP */

# if !defined (IP_DEFAULT_MULTICAST_TTL)
#   define IP_DEFAULT_MULTICAST_TTL 0
# endif /* IP_DEFAULT_MULTICAST_TTL */

# if !defined (IP_DEFAULT_MULTICAST_LOOP)
#   define IP_DEFAULT_MULTICAST_LOOP 0
# endif /* IP_DEFAULT_MULTICAST_LOOP */

# if !defined (IP_MAX_MEMBERSHIPS)
#   define IP_MAX_MEMBERSHIPS 0
# endif /* IP_MAX_MEMBERSHIP */

# if !defined (SIOCGIFBRDADDR)
#   define SIOCGIFBRDADDR 0
# endif /* SIOCGIFBRDADDR */

# if !defined (SIOCGIFADDR)
#   define SIOCGIFADDR 0
# endif /* SIOCGIFADDR */

# if !defined (IPC_PRIVATE)
#   define IPC_PRIVATE PDL_INVALID_SEM_KEY
# endif /* IPC_PRIVATE */

# if !defined (IPC_STAT)
#   define IPC_STAT 0
# endif /* IPC_STAT */

# if !defined (GETVAL)
#   define GETVAL 0
# endif /* GETVAL */

# if !defined (F_GETFL)
#   define F_GETFL 0
# endif /* F_GETFL */

# if !defined (SETVAL)
#   define SETVAL 0
# endif /* SETVAL */

# if !defined (GETALL)
#   define GETALL 0
# endif /* GETALL */

# if !defined (SETALL)
#   define SETALL 0
# endif /* SETALL */

# if !defined (SEM_UNDO)
#   define SEM_UNDO 0
# endif /* SEM_UNDO */

# if defined (__Lynx__)
    // LynxOS Neutrino sets NSIG to the highest-numbered signal.
#   define PDL_NSIG (NSIG + 1)
# else
  // All other platforms set NSIG to one greater than the
  // highest-numbered signal.
#   define PDL_NSIG NSIG
# endif /* __Lynx__ */

# if !defined (R_OK)
#   define R_OK    04      /* Test for Read permission. */
# endif /* R_OK */

# if !defined (W_OK)
#   define W_OK    02      /* Test for Write permission. */
# endif /* W_OK */

# if !defined (X_OK)
#   define X_OK    01      /* Test for eXecute permission. */
# endif /* X_OK */

# if !defined (F_OK)
#   define F_OK    0       /* Test for existence of File. */
# endif /* F_OK */

# if !defined (ESUCCESS)
#   define ESUCCESS 0
# endif /* !ESUCCESS */

# if !defined (EIDRM)
#   define EIDRM 0
# endif /* !EIDRM */

# if !defined (ENOSYS)
#   define ENOSYS EFAULT /* Operation not supported or unknown error. */
# endif /* !ENOSYS */

# if !defined (ENFILE)
#   define ENFILE EMFILE /* No more socket descriptors are available. */
# endif /* !ENOSYS */

# if !defined (ENOTSUP)
#   define ENOTSUP ENOSYS  /* Operation not supported. */
# endif /* !ENOTSUP */

# if !defined (ECOMM)
    // Not the same, but ECONNABORTED is provided on NT.
#   define ECOMM ECONNABORTED
# endif /* ECOMM */

# if !defined (WNOHANG)
#   define WNOHANG 0100
# endif /* !WNOHANG */

# if !defined (EDEADLK)
#   define EDEADLK 1000 /* Some large number.... */
# endif /* !EDEADLK */

# if !defined (MS_SYNC)
#   define MS_SYNC 0x0
# endif /* !MS_SYNC */

# if !defined (PIPE_BUF)
#   define PIPE_BUF 5120
# endif /* PIPE_BUF */

# if !defined (PROT_RDWR)
#   define PROT_RDWR (PROT_READ|PROT_WRITE)
# endif /* PROT_RDWR */

# if !defined (WNOHANG)
#   define WNOHANG 0
# endif /* WNOHANG */

# if defined (PDL_HAS_POSIX_NONBLOCK)
#   define PDL_NONBLOCK O_NONBLOCK
# else
#   define PDL_NONBLOCK O_NDELAY
# endif /* PDL_HAS_POSIX_NONBLOCK */

// These are used by the <PDL_IPC_SAP::enable> and
// <PDL_IPC_SAP::disable> methods.  They must be unique and cannot
// conflict with the value of <PDL_NONBLOCK>.  We make the numbers
// negative here so they won't conflict with other values like SIGIO,
// etc.
# define PDL_SIGIO -1
# define PDL_SIGURG -2
# define PDL_CLOEXEC -3

# define LOCALNAME 0
# define REMOTENAME 1

# if !defined (ETIMEDOUT) && defined (ETIME)
#   define ETIMEDOUT ETIME
# endif /* ETIMEDOUT */

# if !defined (ETIME) && defined (ETIMEDOUT)
#   define ETIME ETIMEDOUT
# endif /* ETIMED */

# if !defined (EBUSY)
#   define EBUSY ETIME
# endif /* EBUSY */

# if !defined (_SC_TIMER_MAX)
#   define _SC_TIMER_MAX 44
# endif /* _SC_TIMER_MAX */

// Default number of <PDL_Event_Handler>s supported by
// <PDL_Timer_Heap>.
# if !defined (PDL_DEFAULT_TIMERS)
#   define PDL_DEFAULT_TIMERS _SC_TIMER_MAX
# endif /* PDL_DEFAULT_TIMERS */

# if defined (PDL_HAS_STRUCT_NETDB_DATA)
typedef char PDL_HOSTENT_DATA[sizeof(struct hostent_data)];
typedef char PDL_SERVENT_DATA[sizeof(struct servent_data)];
typedef char PDL_PROTOENT_DATA[sizeof(struct protoent_data)];
# else
#   if !defined PDL_HOSTENT_DATA_SIZE
#     define PDL_HOSTENT_DATA_SIZE (4*1024)
#   endif /*PDL_HOSTENT_DATA_SIZE */
#   if !defined PDL_SERVENT_DATA_SIZE
#     define PDL_SERVENT_DATA_SIZE (4*1024)
#   endif /*PDL_SERVENT_DATA_SIZE */
#   if !defined PDL_PROTOENT_DATA_SIZE
#     define PDL_PROTOENT_DATA_SIZE (2*1024)
#   endif /*PDL_PROTOENT_DATA_SIZE */
typedef char PDL_HOSTENT_DATA[PDL_HOSTENT_DATA_SIZE];
typedef char PDL_SERVENT_DATA[PDL_SERVENT_DATA_SIZE];
typedef char PDL_PROTOENT_DATA[PDL_PROTOENT_DATA_SIZE];
# endif /* PDL_HAS_STRUCT_NETDB_DATA */

# if !defined (PDL_HAS_SEMUN) || (defined (__GLIBC__) && defined (_SEM_SEMUN_UNDEFINED))
union semun
{
  int val; // value for SETVAL
  struct semid_ds *buf; // buffer for IPC_STAT & IPC_SET
  u_short *array; // array for GETALL & SETALL
};
# endif /* !PDL_HAS_SEMUN || (defined (__GLIBC__) && defined (_SEM_SEMUN_UNDEFINED)) */

// Max size of an PDL Log Record data buffer.  This can be reset in
// the config.h file if you'd like to increase or decrease the size.
# if !defined (PDL_MAXLOGMSGLEN)
#   define PDL_MAXLOGMSGLEN 4 * 1024
# endif /* PDL_MAXLOGMSGLEN */

// Max size of an PDL Token.
# define PDL_MAXTOKENNAMELEN 40

// Max size of an PDL Token client ID.
# define PDL_MAXCLIENTIDLEN MAXHOSTNAMELEN + 20

// Create some useful typedefs.

typedef const char **SYS_SIGLIST;
typedef void *(*PDL_THR_FUNC)(void *);
// This is for C++ static methods.
# if defined (VXWORKS)
typedef int PDL_THR_FUNC_INTERNAL_RETURN_TYPE;
typedef FUNCPTR PDL_THR_FUNC_INTERNAL;  // where typedef int (*FUNCPTR) (...)
# elif defined (PDL_PSOS)
typedef void (*PDL_THR_FUNC_INTERNAL)(void *);
# else
typedef PDL_THR_FUNC PDL_THR_FUNC_INTERNAL;
# endif /* VXWORKS */

extern "C" {
typedef void (*PDL_THR_C_DEST)(void *);
}
typedef void (*PDL_THR_DEST)(void *);

extern "C"
{
# if defined (VXWORKS)
typedef FUNCPTR PDL_THR_C_FUNC;  // where typedef int (*FUNCPTR) (...)
# elif defined (PDL_PSOS)
// needed to handle task entry point type inconsistencies in pSOS+
typedef void (*PSOS_TASK_ENTRY_POINT)();
typedef void (*PDL_THR_C_FUNC)(void *);
# else
typedef void *(*PDL_THR_C_FUNC)(void *);
# endif /* VXWORKS */
}

# if !defined (MAP_FAILED) || defined (PDL_HAS_BROKEN_MAP_FAILED)
#   undef MAP_FAILED
#   define MAP_FAILED ((void *) -1)
# elif defined (PDL_HAS_LONG_MAP_FAILED)
#   undef MAP_FAILED
#   define MAP_FAILED ((void *) -1L)
# endif /* !MAP_FAILED || PDL_HAS_BROKEN_MAP_FAILED */

# if defined (PDL_HAS_MOSTLY_UNICODE_APIS)
#   if defined (PDL_HAS_CHARPTR_DL)
typedef ASYS_TCHAR * PDL_DL_TYPE;
#   else
typedef const ASYS_TCHAR * PDL_DL_TYPE;
#   endif /* PDL_HAS_CHARPTR_DL */
# else
#   if defined (PDL_HAS_CHARPTR_DL)
typedef char * PDL_DL_TYPE;
#   else
typedef const char * PDL_DL_TYPE;
#   endif /* PDL_HAS_CHARPTR_DL */
#endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

# if !defined (PDL_HAS_SIGINFO_T)
struct PDL_Export siginfo_t
{
  siginfo_t (PDL_HANDLE handle);
  siginfo_t (PDL_HANDLE *handles);      // JCEJ 12/23/96

  PDL_HANDLE si_handle_;
  // Win32 HANDLE that has become signaled.

  PDL_HANDLE *si_handles_;
  // Array of Win32 HANDLEs all of which have become signaled.
};
# endif /* PDL_HAS_SIGINFO_T */

// Typedef for the null handler func.
extern "C"
{
  typedef void (*PDL_SIGNAL_C_FUNC)(int,siginfo_t*,void*);
}

# if !defined (PDL_HAS_UCONTEXT_T)
typedef int ucontext_t;
# endif /* PDL_HAS_UCONTEXT_T */

# if !defined (SA_SIGINFO)
#   define SA_SIGINFO 0
# endif /* SA_SIGINFO */

# if !defined (SA_RESTART)
#   define SA_RESTART 0
# endif /* SA_RESTART */

# if defined (PDL_HAS_TIMOD_H)
#   if defined (PDL_HAS_STL_QUEUE_CONFLICT)
#     define queue _Queue_
#   endif /* PDL_HAS_STL_QUEUE_CONFLICT */
#   include /**/ <sys/timod.h>
#   if defined (PDL_HAS_STL_QUEUE_CONFLICT)
#     undef queue
#   endif /* PDL_HAS_STL_QUEUE_CONFLICT */
# elif defined (PDL_HAS_OSF_TIMOD_H)
#   include /**/ <tli/timod.h>
# endif /* PDL_HAS_TIMOD_H */

# if defined rewinddir
#   undef rewinddir
# endif /* rewinddir */

class PDL_Export PDL_Thread_ID
{
  // = TITLE
  //     Defines a platform-independent thread ID.
public:
  // = Initialization method.
  PDL_Thread_ID (PDL_thread_t, PDL_hthread_t);
  PDL_Thread_ID (const PDL_Thread_ID &id);

  // = Set/Get the Thread ID.
  PDL_thread_t id (void);
  void id (PDL_thread_t);

  // = Set/Get the Thread handle.
  PDL_hthread_t handle (void);
  void handle (PDL_hthread_t);

  // != Comparison operator.
  int operator== (const PDL_Thread_ID &) const;
  int operator!= (const PDL_Thread_ID &) const;

private:
  PDL_thread_t thread_id_;
  // Identify the thread.

  PDL_hthread_t thread_handle_;
  // Handle to the thread (typically used to "wait" on Win32).
};

// Type of the extended signal handler.
typedef void (*PDL_Sig_Handler_Ex) (int, siginfo_t *siginfo, ucontext_t *ucontext);

// If the xti.h file redefines the function names, do it now, else
// when the functigon definitions are encountered, they won't match the
// declaration here.

# if defined (PDL_REDEFINES_XTI_FUNCTIONS)
#   include /**/ <xti.h>
#   if defined (UNIXWARE_2_0)         /* They apparantly forgot one... */
extern "C" int _xti_error(char *);
#   endif /* UNIXWARE */
# endif /* PDL_REDEFINES_XTI_FUNCTIONS */

// = The PDL_Sched_Priority type should be used for platform-
//   independent thread and process priorities, by convention.
//   int should be used for OS-specific priorities.
typedef int PDL_Sched_Priority;

// forward declaration
class PDL_Sched_Params;

# if defined (PDL_LACKS_FILELOCKS)
#   if ! defined (VXWORKS) && ! defined (PDL_PSOS)
// VxWorks defines struct flock in sys/fcntlcom.h.  But it doesn't
// appear to support flock ().
struct flock
{
  short l_type;
  short l_whence;
  off_t l_start;
  off_t l_len;          /* len == 0 means until end of file */
  long  l_sysid;
  pid_t l_pid;
  long  l_pad[4];               /* reserve area */
};
#   endif /* ! VXWORKS */
# endif /* PDL_LACKS_FILELOCKS */

# if !defined (PDL_HAS_IP_MULTICAST)  &&  defined (PDL_LACKS_IP_ADD_MEMBERSHIP)
  // Even if PDL_HAS_IP_MULTICAST is not defined, if IP_ADD_MEMBERSHIP
  // is defined, assume that the ip_mreq struct is also defined
  // (presumably in netinet/in.h).
  struct ip_mreq
  {
    struct in_addr imr_multiaddr;
    // IP multicast address of group
    struct in_addr imr_interface;
    // local IP address of interface
  };
# endif /* ! PDL_HAS_IP_MULTICAST  &&  PDL_LACKS_IP_ADD_MEMBERSHIP */

# if !defined (PDL_HAS_STRBUF_T)
struct strbuf
{
  int maxlen; // no. of bytes in buffer.
  int len;    // no. of bytes returned.
  void *buf;  // pointer to data.
};
# endif /* PDL_HAS_STRBUF_T */

class PDL_Export PDL_Str_Buf : public strbuf
{
  // = TITLE
  //     Simple wrapper for STREAM pipes strbuf.
public:
  // = Initialization method
  PDL_Str_Buf (void *b = 0, int l = 0, int max = 0);
  // Constructor.

  PDL_Str_Buf (strbuf &);
  // Constructor.
};

# if defined (PDL_HAS_BROKEN_BITSHIFT)
  // This might not be necessary any more:  it was added prior to the
  // (fd_mask) cast being added to the version below.  Maybe that cast
  // will fix the problem on tandems.    Fri Dec 12 1997 David L. Levine
#   define PDL_MSB_MASK (~(PDL_UINT32 (1) << PDL_UINT32 (NFDBITS - 1)))
# else
  // This needs to go here to avoid overflow problems on some compilers.
#   if defined (PDL_WIN32)
    //  Does PDL_WIN32 have an fd_mask?
#     define PDL_MSB_MASK (~(1 << (NFDBITS - 1)))
#   else  /* ! PDL_WIN32 */
#     define PDL_MSB_MASK (~((fd_mask) 1 << (NFDBITS - 1)))
#   endif /* ! PDL_WIN32 */
# endif /* PDL_HAS_BROKEN_BITSHIFT */

// Signature for registering a cleanup function that is used by the
// <PDL_Object_Manager> and the <PDL_Thread_Manager>.
# if defined (PDL_HAS_SIG_C_FUNC)
extern "C" {
# endif /* PDL_HAS_SIG_C_FUNC */
typedef void (*PDL_CLEANUP_FUNC)(void *object, void *param) /* throw () */;
# if defined (PDL_HAS_SIG_C_FUNC)
}
# endif /* PDL_HAS_SIG_C_FUNC */

// Marker for cleanup, used by PDL_Exit_Info.
extern int pdl_exit_hook_marker;

// For use by <PDL_OS::exit>.
extern "C"
{
  typedef void (*PDL_EXIT_HOOK) (void);
}

# if defined (PDL_WIN32)
// Default WIN32 structured exception handler.
int PDL_SEH_Default_Exception_Selector (void *);
int PDL_SEH_Default_Exception_Handler (void *);
# endif /* PDL_WIN32 */

class PDL_Export PDL_Cleanup
{
  // = TITLE
  //    Base class for objects that are cleaned by PDL_Object_Manager.
public:
  PDL_Cleanup (void);
  // No-op constructor.

  virtual ~PDL_Cleanup (void);
  // Destructor.

  virtual void cleanup (void *param = 0);
  // Cleanup method that, by default, simply deletes itself.
};

// Adapter for cleanup, used by PDL_Object_Manager.
extern "C" PDL_Export
void pdl_cleanup_destroyer (PDL_Cleanup *, void *param = 0);

class PDL_Export PDL_Cleanup_Info
{
  // = TITLE
  //     Hold cleanup information for thread/process
public:
  PDL_Cleanup_Info (void);
  // Default constructor.

  int operator== (const PDL_Cleanup_Info &o) const;
  // Equality operator.

  int operator!= (const PDL_Cleanup_Info &o) const;
  // Inequality operator.

  void *object_;
  // Point to object that gets passed into the <cleanup_hook_>.

  PDL_CLEANUP_FUNC cleanup_hook_;
  // Cleanup hook that gets called back.

  void *param_;
  // Parameter passed to the <cleanup_hook_>.
};

class PDL_Cleanup_Info_Node;

class PDL_Export PDL_OS_Exit_Info
{
  // = TITLE
  //     Hold Object Manager cleanup (exit) information.
  //
  // = DESCRIPTION
  //     For internal use by the PDL library, only.
public:
  PDL_OS_Exit_Info (void);
  // Default constructor.

  ~PDL_OS_Exit_Info (void);
  // Destructor.

  int at_exit_i (void *object, PDL_CLEANUP_FUNC cleanup_hook, void *param);
  // Use to register a cleanup hook.

  int find (void *object);
  // Look for a registered cleanup hook object.  Returns 1 if already
  // registered, 0 if not.

  void call_hooks ();
  // Call all registered cleanup hooks, in reverse order of
  // registration.

private:
  PDL_Cleanup_Info_Node *registered_objects_;
  // Keeps track of all registered objects.  The last node is only
  // used to terminate the list (it doesn't contain a valid
  // PDL_Cleanup_Info).
};

// Run the thread entry point for the <PDL_Thread_Adapter>.  This must
// be an extern "C" to make certain compilers happy...
#if defined (PDL_PSOS)
extern "C" void pdl_thread_adapter (unsigned long args);
#else /* ! defined (PDL_PSOS) */
extern "C" PDL_Export void *pdl_thread_adapter (void *args);
#endif /* PDL_PSOS */

class PDL_OS_Thread_Descriptor
{
  // = TITLE
  //     Parent class of all PDL_Thread_Descriptor classes.
  //
  // =
  //     Container for PDL_Thread_Descriptor members that are
  //     used in PDL_OS.
public:
  long flags (void) const;
  // Get the thread creation flags.

protected:
  PDL_OS_Thread_Descriptor (long flags = 0);
  // For use by PDL_Thread_Descriptor.

  long flags_;
  // Keeps track of whether this thread was created "detached" or not.
  // If a thread is *not* created detached then if someone calls
  // <PDL_Thread_Manager::wait>, we need to join with that thread (and
  // close down the handle).
};

// Forward decl.
class PDL_Thread_Manager;
class PDL_Thread_Descriptor;

class PDL_Export PDL_Thread_Adapter
{
  // = TITLE
  //     Converts a C++ function into a function <pdl_thread_adapter>
  //     function that can be called from a thread creation routine
  //     (e.g., <pthread_create> or <_beginthreadex>) that expects an
  //     extern "C" entry point.  This class also makes it possible to
  //     transparently provide hooks to register a thread with an
  //     <PDL_Thread_Manager>.
  //
  // = DESCRIPTION
  //     This class is used in <PDL_OS::thr_create>.  In general, the
  //     thread that creates an object of this class is different from
  //     the thread that calls <invoke> on this object.  Therefore,
  //     the <invoke> method is responsible for deleting itself.
public:
  PDL_Thread_Adapter (PDL_THR_FUNC user_func,
                      void *arg,
                      PDL_THR_C_FUNC entry_point = (PDL_THR_C_FUNC) pdl_thread_adapter,
                      PDL_Thread_Manager *thr_mgr = 0,
                      PDL_Thread_Descriptor *td = 0
# if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
                      , PDL_SEH_EXCEPT_HANDLER selector = 0,
                      PDL_SEH_EXCEPT_HANDLER handler = 0
# endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */
                      );
  // Constructor.

  void *invoke (void);
  // Execute the <user_func_> with the <arg>.  This function deletes
  // <this>, thereby rendering the object useless after the call
  // returns.

  PDL_Thread_Manager *thr_mgr (void);
  // Accessor for the optional <Thread_Manager>.

  PDL_THR_C_FUNC entry_point (void);
  // Accessor for the C entry point function to the OS thread creation
  // routine.

private:
  ~PDL_Thread_Adapter (void);
  // Ensure that this object must be allocated on the heap.

  void inherit_log_msg (void);
  // Inherit the logging features if the parent thread has an
  // <PDL_Log_Msg>.

  PDL_THR_FUNC user_func_;
  // Thread startup function passed in by the user (C++ linkage).

  void *arg_;
  // Argument to thread startup function.

  PDL_THR_C_FUNC entry_point_;
  // Entry point to the underlying OS thread creation call (C
  // linkage).

  PDL_Thread_Manager *thr_mgr_;
  // Optional thread manager.

  PDL_OS_Thread_Descriptor *thr_desc_;
  // Optional thread descriptor.  Passing this pointer in will force
  // the spawned thread to cache this location in <Log_Msg> and wait
  // until <Thread_Manager> fills in all information in thread
  // descriptor.

# if !defined (PDL_THREADS_DONT_INHERIT_LOG_MSG)
  PDL_OSTREAM_TYPE *ostream_;
  // Ostream where the new TSS Log_Msg will use.

  u_long priority_mask_;
  // Priority_mask to be used in new TSS Log_Msg.

  int tracing_enabled_;
  // Are we allowing tracing in this thread?

  int restart_;
  // Indicates whether we should restart system calls that are
  // interrupted.

  int trace_depth_;
  // Depth of the nesting for printing traces.

#   if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
  PDL_SEH_EXCEPT_HANDLER seh_except_selector_;
  PDL_SEH_EXCEPT_HANDLER seh_except_handler_;
#   endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */
# endif /* PDL_THREADS_DONT_INHERIT_LOG_MSG */

  friend class PDL_Thread_Adapter_Has_Private_Destructor;
  // Friend declaration to avoid compiler warning:  only defines a private
  // destructor and has no friends.
};

class PDL_Export PDL_Thread_Hook
{
  // = TITLE
  //     This class makes it possible to provide user-defined "start"
  //     hooks that are called before the thread entry point function
  //     is invoked.

public:
  virtual void *start (PDL_THR_FUNC func,
                       void *arg);
  // This method can be overridden in a subclass to customize this
  // pre-function call "hook" invocation that can perform
  // initialization processing before the thread entry point <func>
  // method is called back.  The <func> and <arg> passed into the
  // start hook are the same as those passed by the application that
  // spawned the thread.

  static PDL_Thread_Hook *thread_hook (PDL_Thread_Hook *hook);
  // sets the system wide thread hook, returns the previous thread
  // hook or 0 if none is set.

  static PDL_Thread_Hook *thread_hook (void);
  // Returns the current system thread hook.
};

#ifdef NOTDEF /* jdlee */
class PDL_Export PDL_Thread_Control
{
  // = TITLE
  //     Used to keep track of a thread's activities within its entry
  //     point function.
  //
  // = DESCRIPTION
  //     A <PDL_Thread_Manager> uses this class to ensure that threads
  //     it spawns automatically register and unregister themselves
  //     with it.
  //
  //     This class can be stored in thread-specific storage using the
  //     <PDL_TSS> wrapper.  When a thread exits the
  //     <PDL_TSS::cleanup> function deletes this object, thereby
  //     ensuring that it gets removed from its associated
  //     <PDL_Thread_Manager>.
public:
  PDL_Thread_Control (PDL_Thread_Manager *tm = 0,
                      int insert = 0);
  // Initialize the thread control object.  If <insert> != 0, then
  // register the thread with the Thread_Manager.

  ~PDL_Thread_Control (void);
  // Remove the thread from its associated <Thread_Manager> and exit
  // the thread if <do_thr_exit> is enabled.

  void *exit (void *status,
              int do_thr_exit);
  // Remove this thread from its associated <Thread_Manager> and exit
  // the thread if <do_thr_exit> is enabled.

  int insert (PDL_Thread_Manager *tm, int insert = 0);
  // Store the <Thread_Manager> and use it to register ourselves for
  // correct shutdown.

  PDL_Thread_Manager *thr_mgr (void);
  // Returns the current <Thread_Manager>.

  PDL_Thread_Manager *thr_mgr (PDL_Thread_Manager *);
  // Atomically set a new <Thread_Manager> and return the old
  // <Thread_Manager>.

  void *status (void *status);
  // Set the exit status (and return existing status).

  void *status (void);
  // Get the current exit status.

  void dump (void) const;
  // Dump the state of an object.

  PDL_ALLOC_HOOK_DECLARE;
  // Declare the dynamic allocation hooks.

private:
  PDL_Thread_Manager *tm_;
  // Pointer to the thread manager for this block of code.

  void *status_;
  // Keeps track of the exit status for the thread.
};

class PDL_Export PDL_Thread_Exit
{
  // = TITLE
  //    Keep exit information for a Thread in thread specific storage.
  //    so that the thread-specific exit hooks will get called no
  //    matter how the thread exits (e.g., via <PDL_Thread::exit>, C++
  //    or Win32 exception, "falling off the end" of the thread entry
  //    point function, etc.).
  //
  // = DESCRIPTION
  //    This clever little helper class is stored in thread-specific
  //    storage using the <PDL_TSS> wrapper.  When a thread exits the
  //    <PDL_TSS::cleanup> function deletes this object, thereby
  //    closing it down grpdlfully.
public:
  PDL_Thread_Exit (void);
  // Capture the Thread that will be cleaned up automatically.

  void thr_mgr (PDL_Thread_Manager *tm);
  // Set the <PDL_Thread_Manager>.

  ~PDL_Thread_Exit (void);
  // Destructor calls the thread-specific exit hooks when a thread
  // exits.

  static PDL_Thread_Exit *instance (void);
  // Singleton access point.

  static void cleanup (void *instance, void *);
  // Cleanup method, used by the <PDL_Object_Manager> to destroy the
  // singleton.

private:
  PDL_Thread_Control thread_control_;
  // Automatically add/remove the thread from the
  // <PDL_Thread_Manager>.
};
#endif /* NOTDEF */

# if defined (PDL_HAS_PHARLAP_RT)
#define PDL_IPPROTO_TCP SOL_SOCKET
# else
#define PDL_IPPROTO_TCP IPPROTO_TCP
# endif /* PDL_HAS_PHARLAP_RT */

# if defined (PDL_LACKS_FLOATING_POINT)
typedef PDL_UINT32 PDL_timer_t;
# else
typedef double PDL_timer_t;
# endif /* PDL_LACKS_FLOATING_POINT */

# if defined (PDL_HAS_PRUSAGE_T)
    typedef prusage_t PDL_Rusage;
# elif defined (PDL_HAS_GETRUSAGE)
    typedef rusage PDL_Rusage;
# else
    typedef int PDL_Rusage;
# endif /* PDL_HAS_PRUSAGE_T */

#if defined (PDL_HAS_SYS_FILIO_H)
# include /**/ <sys/filio.h>
#endif /* PDL_HAS_SYS_FILIO_H */

# if defined (PDL_WIN32)
    // = typedef for the _stat data structure
    typedef struct _stat PDL_stat;
# else
    typedef struct stat PDL_stat;
# endif /* PDL_WIN32 */

// We need this for MVS...
extern "C" {
  typedef int (*PDL_COMPARE_FUNC)(const void *, const void *);
}

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
#if defined (PDL_HAS_WINSOCK2_GQOS)
typedef SERVICETYPE PDL_SERVICE_TYPE;
#else
typedef u_long PDL_SERVICE_TYPE;
#endif /* PDL_HAS_WINSOCK2_GQOS */
typedef GROUP PDL_SOCK_GROUP;
typedef WSAPROTOCOL_INFO PDL_Protocol_Info;
#define PDL_OVERLAPPED_SOCKET_FLAG WSA_FLAG_OVERLAPPED

#define PDL_XP1_QOS_SUPPORTED XP1_QOS_SUPPORTED
#define PDL_XP1_SUPPORT_MULTIPOINT XP1_SUPPORT_MULTIPOINT

#define PDL_BASEERR WSABASEERR
#define PDL_ENOBUFS WSAENOBUFS
#define PDL_FROM_PROTOCOL_INFO FROM_PROTOCOL_INFO
#define PDL_FLAG_MULTIPOINT_C_ROOT WSA_FLAG_MULTIPOINT_C_ROOT
#define PDL_FLAG_MULTIPOINT_C_LEAF WSA_FLAG_MULTIPOINT_C_LEAF
#define PDL_FLAG_MULTIPOINT_D_ROOT WSA_FLAG_MULTIPOINT_D_ROOT
#define PDL_FLAG_MULTIPOINT_D_LEAF WSA_FLAG_MULTIPOINT_D_LEAF

#define PDL_QOS_NOT_SPECIFIED QOS_NOT_SPECIFIED
#define PDL_SERVICETYPE_NOTRAFFIC SERVICETYPE_NOTRAFFIC
#define PDL_SERVICETYPE_CONTROLLEDLOAD SERVICETYPE_CONTROLLEDLOAD
#define PDL_SERVICETYPE_GUARANTEED SERVICETYPE_GUARANTEED

#define PDL_JL_SENDER_ONLY JL_SENDER_ONLY
#define PDL_JL_BOTH JL_BOTH

#define PDL_SIO_GET_QOS SIO_GET_QOS
#define PDL_SIO_MULTIPOINT_LOOPBACK SIO_MULTIPOINT_LOOPBACK
#define PDL_SIO_MULTICAST_SCOPE SIO_MULTICAST_SCOPE
#define PDL_SIO_SET_QOS SIO_SET_QOS

#else
typedef u_long PDL_SERVICE_TYPE;
typedef u_long PDL_SOCK_GROUP;
struct PDL_Protocol_Info
{
  u_long dwServiceFlags1;
  int iAddressFamily;
  int iProtocol;
  char szProtocol[255+1];
};
#define PDL_OVERLAPPED_SOCKET_FLAG 0

#define PDL_XP1_QOS_SUPPORTED        0x00002000
#define PDL_XP1_SUPPORT_MULTIPOINT   0x00000400

#define PDL_BASEERR   10000
#define PDL_ENOBUFS   (PDL_BASEERR+55)

#define PDL_FROM_PROTOCOL_INFO (-1)

#define PDL_FLAG_MULTIPOINT_C_ROOT    0x02
#define PDL_FLAG_MULTIPOINT_C_LEAF    0x04
#define PDL_FLAG_MULTIPOINT_D_ROOT    0x08
#define PDL_FLAG_MULTIPOINT_D_LEAF    0x10

#define PDL_QOS_NOT_SPECIFIED            0xFFFFFFFF
#define PDL_SERVICETYPE_NOTRAFFIC        0x00000000  // No data in this direction.
#define PDL_SERVICETYPE_CONTROLLEDLOAD   0x00000002  // Controlled Load.
#define PDL_SERVICETYPE_GUARANTEED       0x00000003  // Guaranteed.

#define PDL_JL_SENDER_ONLY    0x01
#define PDL_JL_BOTH           0x04

#define PDL_SIO_GET_QOS              (0x40000000 | 0x08000000 | 7)
#define PDL_SIO_MULTIPOINT_LOOPBACK  (0x08000000 | 9)
#define PDL_SIO_MULTICAST_SCOPE      (0x08000000 | 10)
#define PDL_SIO_SET_QOS              (0x08000000 | 11)

#endif /* PDL_HAS_WINSOCK2 && PDL_HAS_WINSOCK2 != 0 */

class PDL_Export PDL_Flow_Spec
#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
  : public FLOWSPEC
#endif /* PDL_HAS_WINSOCK2 */
{
  // = TITLE
  //   Wrapper class that defines the flow spec QoS information, which
  //   is used by IntServ (RSVP) and DiffServ.
public:
  // = Initialization methods.
  PDL_Flow_Spec (void);
  // Default constructor.

  PDL_Flow_Spec (u_long token_rate,
                 u_long token_bucket_size,
                 u_long peak_bandwidth,
                 u_long latency,
                 u_long delay_variation,
                 PDL_SERVICE_TYPE service_type,
                 u_long max_sdu_size,
                 u_long minimum_policed_size,
                 int ttl,
                 int priority);
  // Constructor that initializes all the fields.

  // = Get/set the token rate in bytes/sec.
  u_long token_rate (void) const;
  void token_rate (u_long tr);

  // = Get/set the token bucket size in bytes.
  u_long token_bucket_size (void) const;
  void token_bucket_size (u_long tbs);

  // = Get/set the PeakBandwidth in bytes/sec.
  u_long peak_bandwidth (void) const;
  void peak_bandwidth (u_long pb);

  // = Get/set the latency in microseconds.
  u_long latency (void) const;
  void latency (u_long l);

  // = Get/set the delay variation in microseconds.
  u_long delay_variation (void) const;
  void delay_variation (u_long dv);

  // = Get/set the service type.
  PDL_SERVICE_TYPE service_type (void) const;
  void service_type (PDL_SERVICE_TYPE st);

  // = Get/set the maximum SDU size in bytes.
  u_long max_sdu_size (void) const;
  void max_sdu_size (u_long mss);

  // = Get/set the minimum policed size in bytes.
  u_long minimum_policed_size (void) const;
  void minimum_policed_size (u_long mps);

  // = Get/set the time-to-live.
  int ttl (void) const;
  void ttl (int t);

  // = Get/set the priority.
  int priority (void) const;
  void priority (int p);

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0) && \
    defined (PDL_HAS_WINSOCK2_GQOS)
#else
private:
  u_long token_rate_;
  u_long token_bucket_size_;
  u_long peak_bandwidth_;
  u_long latency_;
  u_long delay_variation_;
  PDL_SERVICE_TYPE service_type_;
  u_long max_sdu_size_;
  u_long minimum_policed_size_;
  int ttl_;
  int priority_;
#endif /* defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0) && \
          defined (PDL_HAS_WINSOCK2_GQOS) */
};

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
class PDL_Export PDL_QoS : public QOS
#else
class PDL_Export PDL_QoS
#endif /* PDL_HAS_WINSOCK2 */
{
  // = TITLE
  //   Wrapper class that holds the sender and receiver flow spec
  //   information, which is used by IntServ (RSVP) and DiffServ.
public:
  // = Get/set the flow spec for data sending.
  PDL_Flow_Spec sending_flowspec (void) const;
  void sending_flowspec (const PDL_Flow_Spec &fs);

  // = Get/set the flow spec for data receiving.
  PDL_Flow_Spec receiving_flowspec (void) const;
  void receiving_flowspec (const PDL_Flow_Spec &fs);

  // = Get/set the provider specific information.
  iovec provider_specific (void) const;
  void provider_specific (const iovec &ps);

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
#else
private:

  PDL_Flow_Spec sending_flowspec_;
  PDL_Flow_Spec receiving_flowspec_;
#endif

};

class PDL_Export PDL_QoS_Params
{
  // = TITLE
  //   Wrapper class that simplifies the information passed to the QoS
  //   enabled <PDL_OS::connect> and <PDL_OS::join_leaf> methods.
public:
  PDL_QoS_Params (iovec *caller_data = 0,
                  iovec *callee_data = 0,
                  PDL_QoS *socket_qos = 0,
                  PDL_QoS *group_socket_qos = 0,
                  u_long flags = 0);
  // Initialize the data members.  The <caller_data> is a pointer to
  // the user data that is to be transferred to the peer during
  // connection establishment.  The <callee_data> is a pointer to the
  // user data that is to be transferred back from the peer during
  // connection establishment.  The_<socket_qos> is a pointer to the
  // flow specifications for the socket, one for each direction.  The
  // <group_socket_qos> is a pointer to the flow speicfications for
  // the socket group, if applicable.  The_<flags> indicate if we're a
  // sender, receiver, or both.

  // = Get/set caller data.
  iovec *caller_data (void) const;
  void caller_data (iovec *);

  // = Get/set callee data.
  iovec *callee_data (void) const;
  void callee_data (iovec *);

  // = Get/set socket qos.
  PDL_QoS *socket_qos (void) const;
  void socket_qos (PDL_QoS *);

  // = Get/set group socket qos.
  PDL_QoS *group_socket_qos (void) const;
  void group_socket_qos (PDL_QoS *);

  // = Get/set flags.
  u_long flags (void) const;
  void flags (u_long);

private:
  iovec *caller_data_;
  // A pointer to the user data that is to be transferred to the peer
  // during connection establishment.

  iovec *callee_data_;
  // A pointer to the user data that is to be transferred back from
  // the peer during connection establishment.

  PDL_QoS *socket_qos_;
  // A pointer to the flow speicfications for the socket, one for each
  // direction.

  PDL_QoS *group_socket_qos_;
  // A pointer to the flow speicfications for the socket group, if
  // applicable.

  u_long flags_;
  // Flags that indicate if we're a sender, receiver, or both.
};

// Callback function that's used by the QoS-enabled <PDL_OS::accept>
// method.
typedef int (*PDL_QOS_CONDITION_FUNC) (iovec *caller_id,
                                       iovec *caller_data,
                                       PDL_QoS *socket_qos,
                                       PDL_QoS *group_socket_qos,
                                       iovec *callee_id,
                                       iovec *callee_data,
                                       PDL_SOCK_GROUP *g,
                                       u_long callbackdata);

// Callback function that's used by the QoS-enabled <PDL_OS::ioctl>
// method.
#if defined(PDL_HAS_WINSOCK2) && PDL_HAS_WINSOCK2 != 0
typedef LPWSAOVERLAPPED_COMPLETION_ROUTINE PDL_OVERLAPPED_COMPLETION_FUNC;
#else
typedef void (*PDL_OVERLAPPED_COMPLETION_FUNC) (u_long error,
                                                u_long bytes_transferred,
                                                PDL_OVERLAPPED *overlapped,
                                                u_long flags);
#endif /* PDL_HAS_WINSOCK2 != 0 */

class PDL_Export PDL_Accept_QoS_Params
{
  // = TITLE
  //   Wrapper class that simplifies the information passed to the QoS
  //   enabled <PDL_OS::accept> method.
public:
  PDL_Accept_QoS_Params (PDL_QOS_CONDITION_FUNC qos_condition_callback = 0,
                         u_long callback_data = 0);
  // Initialize the data members.  The <qos_condition_callback> is the
  // address of an optional, application-supplied condition function
  // that will make an accept/reject decision based on the caller
  // information pass in as parameters, and optionally create or join
  // a socket group by assinging an appropriate value to the result
  // parameter <g> of this function.  The <callback_data> data is
  // passed back to the application as a condition function parameter,
  // i.e., it is an Asynchronous Completion Token (ACT).

  // = Get/set QoS condition callback.
  PDL_QOS_CONDITION_FUNC qos_condition_callback (void) const;
  void qos_condition_callback (PDL_QOS_CONDITION_FUNC qcc);

  // = Get/Set callback data.
  u_long callback_data (void) const;
  void callback_data (u_long cd);

private:
  PDL_QOS_CONDITION_FUNC qos_condition_callback_;
  // This is the address of an optional, application-supplied
  // condition function that will make an accept/reject decision based
  // on the caller information pass in as parameters, and optionally
  // create or join a socket group by assinging an appropriate value
  // to the result parameter <g> of this function.

  u_long callback_data_;
  // This data is passed back to the application as a condition
  // function parameter, i.e., it is an Asynchronous Completion Token
  // (ACT).
};

class PDL_Export PDL_OS
{
  // = TITLE
  //     This class defines an OS independent programming API that
  //     shields developers from nonportable aspects of writing
  //     efficient system programs on Win32, POSIX and other versions
  //     of UNIX, and various real-time operating systems.
  //
  // = DESCRIPTION
  //     This class encapsulates the differences between various OS
  //     platforms.  When porting PDL to a new platform, this class is
  //     the place to focus on.  Once this file is ported to a new
  //     platform, pretty much everything else comes for "free."  See
  //     <www.cs.wustl.edu/~schmidt/PDL_wrappers/etc/PDL-porting.html>
  //     for instructions on porting PDL.  Please see the README file
  //     in this directory for complete information on the meaning of
  //     the various macros.

  PDL_CLASS_IS_NAMESPACE (PDL_OS);
public:

# if defined (CHORUS)
  // We must format this code as follows to avoid confusing OSE.
  enum PDL_HRTimer_Op
    {
      PDL_HRTIMER_START = K_BSTART,
      PDL_HRTIMER_INCR = K_BPOINT,
      PDL_HRTIMER_STOP = K_BSTOP,
      PDL_HRTIMER_GETTIME = 0xFFFF
    };
# else  /* ! CHORUS */
  enum PDL_HRTimer_Op
    {
      PDL_HRTIMER_START = 0x0,  // Only use these if you can stand
      PDL_HRTIMER_INCR = 0x1,   // for interrupts to be disabled during
      PDL_HRTIMER_STOP = 0x2,   // the timed interval!!!!
      PDL_HRTIMER_GETTIME = 0xFFFF
    };
# endif /* ! CHORUS */

  class pdl_flock_t
  {
    // = TITLE
    //     OS file locking structure.
  public:
    void dump (void) const;
  // Dump state of the object.

# if defined (PDL_WIN32)
    PDL_OVERLAPPED overlapped_;
# else
    struct flock lock_;
# endif /* PDL_WIN32 */

    LPCTSTR lockname_;
    // Name of this filelock.

    PDL_HANDLE handle_;
    // Handle to the underlying file.

# if defined (CHORUS)
    PDL_mutex_t *process_lock_;
    // This is the mutex that's stored in shared memory.  It can only
    // be destroyed by the actor that initialized it.
# endif /* CHORUS */
  };

# if defined (PDL_WIN32)
  // = Default Win32 Security Attributes definition.
  static LPSECURITY_ATTRIBUTES default_win32_security_attributes (LPSECURITY_ATTRIBUTES);

  // = Win32 OS version determination function.
  static const OSVERSIONINFO &get_win32_versioninfo (void);
  // Return the win32 OSVERSIONINFO structure.

# endif /* PDL_WIN32 */

  // = A set of wrappers for miscellaneous operations.
  static int atoi (const char *s);
  static char *getenv (const char *symbol);
  static int putenv (const char *string);
  static int getopt (int argc,
                     char *const *argv,
                     const char *optstring);
  static int argv_to_string (ASYS_TCHAR **argv,
                             ASYS_TCHAR *&buf,
                             int substitute_env_args = 1);
  static int string_to_argv (ASYS_TCHAR *buf,
                             size_t &argc,
                             ASYS_TCHAR **&argv,
                             int substitute_env_args = 1);
  static long sysconf (int);

  // = A set of wrappers for condition variables.
  static int condattr_init (PDL_condattr_t &attributes,
                             int type = PDL_DEFAULT_SYNCH_TYPE);
  static int condattr_destroy (PDL_condattr_t &attributes);
  static int cond_broadcast (PDL_cond_t *cv);
  static int cond_destroy (PDL_cond_t *cv);
  static int cond_init (PDL_cond_t *cv,
                        short type = PDL_DEFAULT_SYNCH_TYPE,
                        LPCTSTR name = 0,
                        void *arg = 0);
  static int cond_init (PDL_cond_t *cv,
                        PDL_condattr_t &attributes,
                        LPCTSTR name = 0,
                        void *arg = 0);
  static int cond_signal (PDL_cond_t *cv);
  static int cond_timedwait (PDL_cond_t *cv,
                             PDL_mutex_t *m,
                             PDL_Time_Value *);
  static int cond_wait (PDL_cond_t *cv,
                        PDL_mutex_t *m);
# if defined (PDL_WIN32) && defined (PDL_HAS_WTHREADS)
  static int cond_timedwait (PDL_cond_t *cv,
                             PDL_thread_mutex_t *m,
                             PDL_Time_Value *);
  static int cond_wait (PDL_cond_t *cv,
                        PDL_thread_mutex_t *m);
# endif /* PDL_WIN32 && PDL_HAS_WTHREADS */

  // = A set of wrappers for determining config info.
  static int uname (struct utsname *name);
  static long sysinfo (int cmd,
                       char *buf,
                       long count);
  static int hostname (char *name,
                       size_t maxnamelen);

  // = A set of wrappers for explicit dynamic linking.
  static int dlclose (PDL_SHLIB_HANDLE handle);

  static ASYS_TCHAR *dlerror (void);
  static PDL_SHLIB_HANDLE dlopen (const ASYS_TCHAR *filename,
                                  int mode = PDL_DEFAULT_SHLIB_MODE);
  static void *dlsym (PDL_SHLIB_HANDLE handle,
                      const char *symbol);

  // = A set of wrappers for the directory iterator.
  static DIR *opendir (const char *filename);
  static void closedir (DIR *);
  static struct dirent *readdir (DIR *);
  static int readdir_r (DIR *dirp,
                        struct dirent *entry,
                        struct dirent **result);
  static long telldir (DIR *);
  static void seekdir (DIR *,
                       long loc);
  static void rewinddir (DIR *);

  // = A set of wrappers for stdio file operations.
  static int last_error (void);
  static void last_error (int);
  static int set_errno_to_last_error (void);
  static int set_errno_to_wsa_last_error (void);
  static int fclose (FILE *fp);
  static int fcntl (PDL_HANDLE handle,
                    int cmd,
                    long arg = 0);
  static int fdetach (const char *file);

  static int fsync(PDL_HANDLE handle);

# if !defined (PDL_HAS_WINCE)
  // CE doesn't support these char version functions.
  // However, we should provide UNICODE version of them.
  static FILE *fopen (const char *filename,
                      const char *mode);
#   if defined (fdopen)
#     undef fdopen
#   endif /* fdopen */
  static FILE *fdopen (PDL_HANDLE handle,
                       const char *mode);
  static char *fgets (char *buf,
                      int size,
                      FILE *fp);
  static int stat (const char *file,
                   struct stat *);
  static int truncate (const char *filename,
                       off_t length);
  static int fprintf (FILE *fp,
                      const char *format,
                      ...);
  static int sprintf (char *buf,
                      const char *format,
                      ...);
  static int vsprintf (char *buffer,
                       const char *format,
                       va_list argptr);
  static void perror (const char *s);

  static int printf (const char *format, ...);

  // The old gets () which directly maps to the evil, unprotected
  // gets () has been deprecated.  If you really need gets (),
  // consider the following one.

  // A better gets ().
  //   If n == 0, input is swallowed, but NULL is returned.
  //   Otherwise, reads up to n-1 bytes (not including the newline),
  //              then swallows rest up to newline
  //              then swallows newline
  static char *gets (char *str, int n = 0);
  static int puts (const char *s);
  static int fputs (const char *s,
                    FILE *stream);
# endif /* ! PDL_HAS_WINCE */

  static int fflush (FILE *fp);
  static size_t fread (void *ptr,
                       size_t size,
                       size_t nelems,
                       FILE *fp);
  static int fseek (FILE *fp,
                    long offset,
                    int ptrname);
  static int fstat (PDL_HANDLE,
                    struct stat *);
  static int lstat (const char *,
                    struct stat *);
  static int ftruncate (PDL_HANDLE,
                        off_t);
  static size_t fwrite (const void *ptr,
                        size_t size,
                        size_t nitems,
                        FILE *fp);
  static void rewind (FILE *fp);

  // = Wrappers for searching and sorting.
  static void *bsearch (const void *key,
                        const void *base,
                        size_t nel,
                        size_t size,
                        PDL_COMPARE_FUNC);
  static void qsort (void *base,
                     size_t nel,
                     size_t width,
                     PDL_COMPARE_FUNC);

  // = A set of wrappers for file locks.
  static int flock_init (PDL_OS::pdl_flock_t *lock,
                         int flags = 0,
                         LPCTSTR name = 0,
                         mode_t perms = 0);
  static int flock_destroy (PDL_OS::pdl_flock_t *lock);
# if defined (PDL_WIN32)
  static void adjust_flock_params (PDL_OS::pdl_flock_t *lock,
                                   short whence,
                                   off_t &start,
                                   off_t &len);
# endif /* PDL_WIN32 */
  static int flock_rdlock (PDL_OS::pdl_flock_t *lock,
                           short whence = 0,
                           off_t start = 0,
                           off_t len = 0);
  static int flock_tryrdlock (PDL_OS::pdl_flock_t *lock,
                              short whence = 0,
                              off_t start = 0,
                              off_t len = 0);
  static int flock_trywrlock (PDL_OS::pdl_flock_t *lock,
                              short whence = 0,
                              off_t start = 0,
                              off_t len = 0);
  static int flock_unlock (PDL_OS::pdl_flock_t *lock,
                           short whence = 0,
                           off_t start = 0,
                           off_t len = 0);
  static int flock_wrlock (PDL_OS::pdl_flock_t *lock,
                           short whence = 0,
                           off_t start = 0,
                           off_t len = 0);

  // = A set of wrappers for low-level process operations.
  static int atexit (PDL_EXIT_HOOK func);
  static int execl (const char *path,
                    const char *arg0, ...);
  static int execle (const char *path,
                     const char *arg0, ...);
  static int execlp (const char *file,
                     const char *arg0, ...);
  static int execv (const char *path,
                    char *const argv[]);
  static int execvp (const char *file,
                     char *const argv[]);
  static int execve (const char *path,
                     char *const argv[],
                     char *const envp[]);
  static void _exit (int status = 0);
  static void exit (int status = 0);
  static void abort (void);
  static pid_t fork (void);
  static pid_t fork (const char *program_name);
  static pid_t fork_exec (ASYS_TCHAR *argv[]);
  // Forks and exec's a process in a manner that works on Solaris and
  // NT.  argv[0] must be the full path name to the executable.

  static int getpagesize (void);
  static int allocation_granularity (void);

  static gid_t getgid (void);
  static int setgid (gid_t);
  static pid_t getpid (void);
  static pid_t getpgid (pid_t pid);
  static pid_t getppid (void);
  static uid_t getuid (void);
  static int setuid (uid_t);
  static pid_t setsid (void);
  static int setpgid (pid_t pid, pid_t pgid);
  static int setreuid (uid_t ruid, uid_t euid);
  static int setregid (gid_t rgid, gid_t egid);
  static int system (const char *s);
  static pid_t waitpid (pid_t pid,
                        PDL_exitcode *status = 0,
                        int wait_options = 0,
                        PDL_HANDLE handle = 0);
  // Calls <::waitpid> on UNIX/POSIX platforms and <::await> on
  // Chorus.  Does not work on Vxworks, or pSoS.
  // On Win32, <pid> is ignored if the <handle> is not equal to 0.
  // Passing the process <handle> is prefer on Win32 because using
  // <pid> to wait on the project doesn't always work correctly
  // if the waited process has already terminated.
  static pid_t wait (pid_t pid,
                     PDL_exitcode *status,
                     int wait_options = 0,
                     PDL_HANDLE handle = 0);
  // Calls <::WaitForSingleObject> on Win32 and <PDL::waitpid>
  // otherwise.  Returns the passed in <pid_t> on success and -1 on
  // failure.
  // On Win32, <pid> is ignored if the <handle> is not equal to 0.
  // Passing the process <handle> is prefer on Win32 because using
  // <pid> to wait on the project doesn't always work correctly
  // if the waited process has already terminated.
  static pid_t wait (int * = 0);
  // Calls OS <::wait> function, so it's only portable to UNIX/POSIX
  // platforms.

  // = A set of wrappers for timers and resource stats.
  static u_int alarm (u_int secs);
  static u_int ualarm (u_int usecs,
                       u_int interval = 0);
  static u_int ualarm (const PDL_Time_Value &tv,
                       const PDL_Time_Value &tv_interval);
  static PDL_hrtime_t gethrtime (const PDL_HRTimer_Op = PDL_HRTIMER_GETTIME);
# if defined (PDL_HAS_POWERPC_TIMER) && (defined (ghs) || defined (__GNUG__))
  static void readPPCTimeBase (u_long &most,
                               u_long &least);
# endif /* PDL_HAS_POWERPC_TIMER  &&  (ghs or __GNUG__) */
  static int clock_gettime (clockid_t,
                            struct timespec *);
  static PDL_Time_Value gettimeofday (void);
  static int getrusage (int who,
                        struct rusage *rusage);
  static int getrlimit (int resource,
                        struct rlimit *rl);
  static int setrlimit (int resource,
                        PDL_SETRLIMIT_TYPE *rl);
  static int sleep (u_int seconds);
  static int sleep (const PDL_Time_Value &tv);
  static int nanosleep (const struct timespec *requested,
                        struct timespec *remaining = 0);

# if defined (PDL_HAS_BROKEN_R_ROUTINES)
#   undef ctime_r
#   undef asctime_r
#   undef rand_r
#   undef getpwnam_r
# endif /* PDL_HAS_BROKEN_R_ROUTINES */

# if defined (difftime)
#   define PDL_DIFFTIME(t1, t0) difftime(t1,t0)
#   undef difftime
# endif /* difftime */

  // = A set of wrappers for operations on time.
# if !defined (PDL_HAS_WINCE)
  static time_t mktime (struct tm *timeptr);
# endif /* !PDL_HAS_WINCE */

  // wrapper for time zone information.
  static void tzset (void);
  static long timezone (void);

  static double difftime (time_t t1,
                          time_t t0);
  static time_t time (time_t *tloc = 0);
  static struct tm *localtime (const time_t *clock);
  static struct tm *localtime_r (const time_t *clock,
                                 struct tm *res);
  static struct tm *gmtime (const time_t *clock);
  static struct tm *gmtime_r (const time_t *clock,
                              struct tm *res);
  static char *asctime (const struct tm *tm);
  static char *asctime_r (const struct tm *tm,
                          char *buf, int buflen);
  static ASYS_TCHAR *ctime (const time_t *t);
# if !defined (PDL_HAS_WINCE)
  static char *ctime_r (const time_t *clock,
                        char *buf,
                        int buflen);
# endif /* !PDL_HAS_WINCE */
# if defined (PDL_HAS_MOSTLY_UNICODE_APIS)
  static wchar_t *ctime_r (const time_t *clock,
                           wchar_t *buf,
                           int buflen);
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */
  static size_t strftime (char *s,
                          size_t maxsize,
                          const char *format,
                          const struct tm *timeptr);

  // = A set of wrappers for memory managment.
  static void *sbrk (int brk);
  static void *calloc (size_t elements,
                       size_t sizeof_elements);
  static void *malloc (size_t);
  static void *realloc (void *,
                        size_t);
  static void free (void *);

  // = A set of wrappers for memory copying operations.
  static int memcmp (const void *t,
                     const void *s,
                     size_t len);
  static const void *memchr (const void *s,
                             int c,
                             size_t len);
  static void *memcpy (void *t,
                       const void *s,
                       size_t len);
  static void *memmove (void *t,
                        const void *s,
                        size_t len);
  static void *memset (void *s,
                       int c,
                       size_t len);

  // = A set of wrappers for System V message queues.
  static int msgctl (int msqid,
                     int cmd,
                     struct msqid_ds *);
  static int msgget (key_t key,
                     int msgflg);
  static int msgrcv (int int_id,
                     void *buf,
                     size_t len,
                     long type,
                     int flags);
  static int msgsnd (int int_id,
                     const void *buf,
                     size_t len,
                     int flags);

  // = A set of wrappers for memory mapped files.
  static int madvise (caddr_t addr,
                      size_t len,
                      int advice);
  static void *mmap (void *addr,
                     size_t len,
                     int prot,
                     int flags,
                     PDL_HANDLE handle,
                     off_t off = 0,
                     PDL_HANDLE *file_mapping = 0,
                     LPSECURITY_ATTRIBUTES sa = 0);
  static int mprotect (void *addr,
                       size_t len,
                       int prot);
  static int msync (void *addr,
                    size_t len,
                    int sync);
  static int munmap (void *addr,
                     size_t len);

  // = A set of wrappers for recursive mutex locks.
  static int recursive_mutex_init (PDL_recursive_thread_mutex_t *m,
                                   LPCTSTR name = 0,
                                   void *arg = 0,
                                   LPSECURITY_ATTRIBUTES sa = 0);
  static int recursive_mutex_destroy (PDL_recursive_thread_mutex_t *m);

  static int recursive_mutex_lock (PDL_recursive_thread_mutex_t *m);

  static int recursive_mutex_trylock (PDL_recursive_thread_mutex_t *m);

  static int recursive_mutex_unlock (PDL_recursive_thread_mutex_t *m);

  // = A set of wrappers for mutex locks.
  static int mutex_init (PDL_mutex_t *m,
                         int type = PDL_DEFAULT_SYNCH_TYPE,
                         LPCTSTR name = 0,
                         void *arg = 0,
                         LPSECURITY_ATTRIBUTES sa = 0);
  static int mutex_destroy (PDL_mutex_t *m);

  static int mutex_lock (PDL_mutex_t *m);
  // Win32 note: Abandoned mutexes are not treated differently. 0 is
  // returned since the calling thread does get the ownership.

  static int mutex_lock (PDL_mutex_t *m,
                         int &abandoned);
  // This method is only implemented for Win32.  For abandoned
  // mutexes, <abandoned> is set to 1 and 0 is returned.

  static int mutex_trylock (PDL_mutex_t *m);
  // Win32 note: Abandoned mutexes are not treated differently. 0 is
  // returned since the calling thread does get the ownership.

  static int mutex_trylock (PDL_mutex_t *m,
                            int &abandoned);
  // This method is only implemented for Win32.  For abandoned
  // mutexes, <abandoned> is set to 1 and 0 is returned.

  static int mutex_unlock (PDL_mutex_t *m);

  // = A set of wrappers for mutex locks that only work within a
  // single process.
  static int thread_mutex_init (PDL_thread_mutex_t *m,
                                int type = PDL_DEFAULT_SYNCH_TYPE,
                                LPCTSTR name = 0,
                                void *arg = 0);
  static int thread_mutex_destroy (PDL_thread_mutex_t *m);
  static int thread_mutex_lock (PDL_thread_mutex_t *m);
  static int thread_mutex_trylock (PDL_thread_mutex_t *m);
  static int thread_mutex_unlock (PDL_thread_mutex_t *m);

  // = A set of wrappers for low-level file operations.
  static int access (const char *path,
                     int amode);
  static int close (PDL_HANDLE handle);
  static PDL_HANDLE creat (LPCTSTR filename,
                           mode_t mode);
  static PDL_HANDLE dup (PDL_HANDLE handle);
  static int dup2 (PDL_HANDLE oldfd,
                   PDL_HANDLE newfd);
  static int fattach (int handle,
                      const char *path);
  static long filesize (PDL_HANDLE handle);
  static long filesize (LPCTSTR handle);
  static int getmsg (PDL_HANDLE handle,
                     struct strbuf *ctl,
                     struct strbuf
                     *data, int *flags);
  static int getpmsg (PDL_HANDLE handle,
                      struct strbuf *ctl,
                      struct strbuf
                      *data,
                      int *band,
                      int *flags);
  static int ioctl (PDL_HANDLE handle,
                    int cmd,
                    void * = 0);
  // UNIX-style <ioctl>.
  static int ioctl (PDL_HANDLE socket,
                    u_long io_control_code,
                    void *in_buffer_p,
                    u_long in_buffer,
                    void *out_buffer_p,
                    u_long out_buffer,
                    u_long *bytes_returned,
                    PDL_OVERLAPPED *overlapped,
                    PDL_OVERLAPPED_COMPLETION_FUNC func);
  // QoS-enabled <ioctl>.
  static int ioctl (PDL_HANDLE socket,
                    u_long io_control_code,
                    PDL_QoS &pdl_qos,
                    u_long *bytes_returned,
                    void *buffer_p = 0,
                    u_long buffer = 0,
                    PDL_OVERLAPPED *overlapped = 0,
                    PDL_OVERLAPPED_COMPLETION_FUNC func = 0);
  // QoS-enabled <ioctl> when the I/O control code is either SIO_SET_QOS
  // or SIO_GET_QOS.
  static int isastream (PDL_HANDLE handle);
  static int isatty (int handle);
#if defined (PDL_WIN32) && !defined (PDL_HAS_WINCE)
  static int isatty (PDL_HANDLE handle);
#endif /* PDL_WIN32 && !PDL_HAS_WINCE */
  static off_t lseek (PDL_HANDLE handle,
                      off_t offset,
                      int whence);
#if defined (PDL_HAS_LLSEEK) || defined (PDL_HAS_LSEEK64)
  PDL_LOFF_T llseek (PDL_HANDLE handle, PDL_LOFF_T offset, int whence);
#endif /* PDL_HAS_LLSEEK */
  static PDL_HANDLE open (const char *filename,
                          int mode,
                          int perms = 0,
                          LPSECURITY_ATTRIBUTES sa = 0);
  static int putmsg (PDL_HANDLE handle,
                     const struct strbuf *ctl,
                     const struct strbuf *data,
                     int flags);
  static int putpmsg (PDL_HANDLE handle,
                      const struct strbuf *ctl,
                      const struct strbuf *data,
                      int band,
                      int flags);
  static ssize_t read (PDL_HANDLE handle,
                       void *buf,
                       size_t len);
  static ssize_t read (PDL_HANDLE handle,
                       void *buf,
                       size_t len,
                       PDL_OVERLAPPED *);
  static ssize_t read_n (PDL_HANDLE handle,
                         void *buf,
                         size_t len);
  // Receive <len> bytes into <buf> from <handle> (uses the
  // <PDL_OS::read> call, which uses the <read> system call on UNIX
  // and the <ReadFile> call on Win32).  If <handle> is set to
  // non-blocking mode this call will poll until all <len> bytes are
  // received.
  static int readlink (const char *path,
                       char *buf,
                       size_t bufsiz);
  static ssize_t pread (PDL_HANDLE handle,
                        void *buf,
                        size_t nbyte,
                        off_t offset);
  static int recvmsg (PDL_HANDLE handle,
                      struct msghdr *msg,
                      int flags);
  static int sendmsg (PDL_HANDLE handle,
                      const struct msghdr *msg,
                      int flags);
  static ssize_t write (PDL_HANDLE handle,
                        const void *buf,
                        size_t nbyte);
  static ssize_t write (PDL_HANDLE handle,
                        const void *buf,
                        size_t nbyte,
                        PDL_OVERLAPPED *);
  static ssize_t write_n (PDL_HANDLE handle,
                          const void *buf,
                          size_t len);
  // Send <len> bytes from <buf> to <handle> (uses the <PDL_OS::write>
  // calls, which is uses the <write> system call on UNIX and the
  // <WriteFile> call on Win32).  If <handle> is set to non-blocking
  // mode this call will poll until all <len> bytes are sent.
  static ssize_t pwrite (PDL_HANDLE handle,
                         const void *buf,
                         size_t nbyte,
                         off_t offset);
  static ssize_t readv (PDL_HANDLE handle,
                        iovec *iov,
                        int iovlen);
  static ssize_t writev (PDL_HANDLE handle,
                         const iovec *iov,
                         int iovcnt);
  static ssize_t recvv (PDL_HANDLE handle,
                        iovec *iov,
                        int iovlen);
  static ssize_t sendv (PDL_HANDLE handle,
                        const iovec *iov,
                        int iovcnt);

  // = A set of wrappers for event demultiplexing and IPC.
  static int select (int width,
                     fd_set *rfds,
                     fd_set *wfds,
                     fd_set *efds,
                     const PDL_Time_Value *tv = 0);
  static int select (int width,
                     fd_set *rfds,
                     fd_set *wfds,
                     fd_set *efds,
                     const PDL_Time_Value &tv);
  static int poll (struct pollfd *pollfds,
                   u_long len,
                   const PDL_Time_Value *tv = 0);
  static int poll (struct pollfd *pollfds,
                   u_long len,
                   const PDL_Time_Value &tv);
  static int pipe (PDL_HANDLE handles[]);

  static PDL_HANDLE shm_open (const char *filename,
                              int mode,
                              int perms = 0,
                              LPSECURITY_ATTRIBUTES sa = 0);
  static int shm_unlink (const char *path);

  // = A set of wrappers for directory operations.
  static mode_t umask (mode_t cmask);
# if !defined (PDL_HAS_MOSTLY_UNICODE_APIS)
  static int chdir (const char *path);
  static int mkdir (const char *path,
                    mode_t mode = PDL_DEFAULT_DIR_PERMS);
  static int mkfifo (const char *file,
                     mode_t mode = PDL_DEFAULT_FILE_PERMS);
  static char *mktemp (char *t);
  static char *getcwd (char *,
                       size_t);
  static int rename (const char *old_name,
                     const char *new_name);
  static int unlink (const char *path);
  static char *tempnam (const char *dir = 0,
                        const char *pfx = 0);
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

  // = A set of wrappers for random number operations.
  static int rand (void);
  static int rand_r (PDL_RANDR_TYPE &seed);
  static void srand (u_int seed);

  // = A set of wrappers for readers/writer locks.
  static int rwlock_init (PDL_rwlock_t *rw,
                          int type = PDL_DEFAULT_SYNCH_TYPE,
                          LPCTSTR name = 0,
                          void *arg = 0);
  static int rwlock_destroy (PDL_rwlock_t *rw);
  static int rw_rdlock (PDL_rwlock_t *rw);
  static int rw_wrlock (PDL_rwlock_t *rw);
  static int rw_tryrdlock (PDL_rwlock_t *rw);
  static int rw_trywrlock (PDL_rwlock_t *rw);
  static int rw_trywrlock_upgrade (PDL_rwlock_t *rw);
  static int rw_unlock (PDL_rwlock_t *rw);

  // = A set of wrappers for auto-reset and manuaevents.
  static int event_init (PDL_event_t *event,
                         int manual_reset = 0,
                         int initial_state = 0,
                         int type = PDL_DEFAULT_SYNCH_TYPE,
                         LPCTSTR name = 0,
                         void *arg = 0,
                         LPSECURITY_ATTRIBUTES sa = 0);
  static int event_destroy (PDL_event_t *event);
  static int event_wait (PDL_event_t *event);
  static int event_timedwait (PDL_event_t *event,
                              PDL_Time_Value *timeout);
  static int event_signal (PDL_event_t *event);
  static int event_pulse (PDL_event_t *event);
  static int event_reset (PDL_event_t *event);

  // = A set of wrappers for semaphores.
  static int sema_destroy (PDL_sema_t *s);
  static int sema_init (PDL_sema_t *s,
                        u_int count,
                        int type = PDL_DEFAULT_SYNCH_TYPE,
                        LPCTSTR name = 0,
                        void *arg = 0,
                        int max = 0x7fffffff,
                        LPSECURITY_ATTRIBUTES sa = 0);
  static int sema_post (PDL_sema_t *s);
  static int sema_post (PDL_sema_t *s,
                        size_t release_count);
  static int sema_trywait (PDL_sema_t *s);
  static int sema_wait (PDL_sema_t *s);
  static int sema_wait (PDL_sema_t *s,
                        PDL_Time_Value &tv);

  // = A set of wrappers for System V semaphores.
  static int semctl (int int_id,
                     int semnum,
                     int cmd,
                     semun);
  static int semget (key_t key,
                     int nsems,
                     int flags);
  static int semop (int int_id,
                    struct sembuf *sops,
                    size_t nsops);

  // = Thread scheduler interface.
  static int sched_params (const PDL_Sched_Params &, PDL_id_t id = PDL_SELF);
  // Set scheduling parameters.  An id of PDL_SELF indicates, e.g.,
  // set the parameters on the calling thread.

  // = A set of wrappers for System V shared memory.
  static void *shmat (int int_id,
                      void *shmaddr,
                      int shmflg);
  static int shmctl (int int_id,
                     int cmd,
                     struct shmid_ds *buf);
  static int shmdt (void *shmaddr);
  static int shmget (key_t key,
                     size_t size,
                     int flags);

  // = A set of wrappers for Signals.
  static int kill (pid_t pid,
                   int signum);
  static int sigaction (int signum,
                        const struct sigaction *nsa,
                        struct sigaction *osa);
  static int sigaddset (sigset_t *s,
                        int signum);
  static int sigdelset (sigset_t *s,
                        int signum);
  static int sigemptyset (sigset_t *s);
  static int sigfillset (sigset_t *s);
  static int sigismember (sigset_t *s,
                          int signum);
  static PDL_SignalHandler signal (int signum,
                                   PDL_SignalHandler);
  static int sigsuspend (const sigset_t *set);
  static int sigprocmask (int how,
                          const sigset_t *nsp,
                          sigset_t *osp);

  static int pthread_sigmask (int how,
                              const sigset_t *nsp,
                              sigset_t *osp);

  // = A set of wrappers for sockets.
  static PDL_HANDLE accept (PDL_HANDLE handle,
                            struct sockaddr *addr,
                            int *addrlen);
  // BSD-style <accept> (no QoS).
  static PDL_HANDLE accept (PDL_HANDLE handle,
                            struct sockaddr *addr,
                            int *addrlen,
                            const PDL_Accept_QoS_Params &qos_params);
  // QoS-enabled <accept>, which passes <qos_params> to <accept>.  If
  // the OS platform doesn't support QoS-enabled <accept> then the
  // <qos_params> are ignored and the BSD-style <accept> is called.
  static int bind (PDL_HANDLE s,
                   struct sockaddr *name,
                   int namelen);
  static int connect (PDL_HANDLE handle,
                      struct sockaddr *addr,
                      int addrlen);
  // BSD-style <connect> (no QoS).
  static int connect (PDL_HANDLE handle,
                      const sockaddr *addr,
                      int addrlen,
                      const PDL_QoS_Params &qos_params);
  // QoS-enabled <connect>, which passes <qos_params> to <connect>.
  // If the OS platform doesn't support QoS-enabled <connect> then the
  // <qos_params> are ignored and the BSD-style <connect> is called.

  static int closesocket (PDL_HANDLE s);
  static struct hostent *gethostbyaddr (const char *addr,
                                        int length,
                                        int type);
  static struct hostent *gethostbyname (const char *name);
  static struct hostent *gethostbyname2 (const char *name, int type);
  static struct hostent *gethostbyaddr_r (const char *addr,
                                          int length,
                                          int type,
                                          struct hostent *result,
                                          PDL_HOSTENT_DATA buffer,
                                          int *h_errnop);
  static struct hostent *gethostbyname_r (const char *name,
                                          struct hostent *result,
                                          PDL_HOSTENT_DATA buffer,
                                          int *h_errnop);
  static int getpeername (PDL_HANDLE handle,
                          struct sockaddr *addr,
                          int *addrlen);
  static struct protoent *getprotobyname (const char *name);
  static struct protoent *getprotobyname_r (const char *name,
                                            struct protoent *result,
                                            PDL_PROTOENT_DATA buffer);
  static struct protoent *getprotobynumber (int proto);
  static struct protoent *getprotobynumber_r (int proto,
                                              struct protoent *result,
                                              PDL_PROTOENT_DATA buffer);
  static struct servent *getservbyname (const char *svc,
                                        const char *proto);
  static struct servent *getservbyname_r (const char *svc,
                                          const char *proto,
                                          struct servent *result,
                                          PDL_SERVENT_DATA buf);
  static int getsockname (PDL_HANDLE handle,
                          struct sockaddr *addr,
                          int *addrlen);
  static int getsockopt (PDL_HANDLE handle,
                         int level,
                         int optname,
                         char *optval,
                         int *optlen);
  static long inet_addr (const char *name);
  static char *inet_ntoa (const struct in_addr addr);
  static int inet_aton (const char *strptr,
                        struct in_addr *addr);
  static const char *inet_ntop (int family,
                                const void *addrptr,
                                char *strptr,
                                size_t len);
  static int inet_pton (int family,
                        const char *strptr,
                        void *addrptr);
  static int enum_protocols (int *protocols,
                             PDL_Protocol_Info *protocol_buffer,
                             u_long *buffer_length);
  // Retrieve information about available transport protocols
  // installed on the local machine.
  static PDL_HANDLE join_leaf (PDL_HANDLE socket,
                               const sockaddr *name,
                               int namelen,
                               const PDL_QoS_Params &qos_params);
  // Joins a leaf node into a QoS-enabled multi-point session.
  static int listen (PDL_HANDLE handle,
                     int backlog);
  static int recv (PDL_HANDLE handle,
                   char *buf,
                   int len,
                   int flags = 0);
  static int recvfrom (PDL_HANDLE handle,
                       char *buf,
                       int len,
                       int flags,
                       struct sockaddr *addr,
                       int *addrlen);
  static int recvfrom (PDL_HANDLE handle,
                       iovec *buffers,
                       int buffer_count,
                       size_t &number_of_bytes_recvd,
                       int &flags,
                       struct sockaddr *addr,
                       int *addrlen,
                       PDL_OVERLAPPED *overlapped,
                       PDL_OVERLAPPED_COMPLETION_FUNC func);
  static int send (PDL_HANDLE handle,
                   const char *buf,
                   int len,
                   int flags = 0);
  static int sendto (PDL_HANDLE handle,
                     const char *buf,
                     int len,
                     int flags,
                     const struct sockaddr *addr,
                     int addrlen);
  static int sendto (PDL_HANDLE handle,
                     const iovec *buffers,
                     int buffer_count,
                     size_t &number_of_bytes_sent,
                     int flags,
                     const struct sockaddr *addr,
                     int addrlen,
                     PDL_OVERLAPPED *overlapped,
                     PDL_OVERLAPPED_COMPLETION_FUNC func);
  static int setsockopt (PDL_HANDLE handle,
                         int level,
                         int optname,
                         const char *optval,
                         int optlen);
  // QoS-enabled <ioctl> wrapper.
  static int shutdown (PDL_HANDLE handle,
                       int how);
  static PDL_HANDLE socket (int protocol_family,
                            int type,
                            int proto);

  // Create a BSD-style socket (no QoS).
  static PDL_HANDLE socket (int protocol_family,
                            int type,
                            int proto,
                            PDL_Protocol_Info *protocolinfo,
                            PDL_SOCK_GROUP g,
                            u_long flags);
  // Create a QoS-enabled socket.  If the OS platform doesn't support
  // QoS-enabled <socket> then the BSD-style <socket> is called.

  static int socketpair (int domain,
                         int type,
                         int protocol,
                         PDL_HANDLE sv[2]);
  static int socket_init (int version_high = 1,
                          int version_low = 1);
  // Initialize WinSock before first use (e.g., when a DLL is first
  // loaded or the first use of a socket() call.

  static int socket_fini (void);
  // Finalize WinSock after last use (e.g., when a DLL is unloaded).

  // = A set of wrappers for password routines.
  static void setpwent (void);
  static void endpwent (void);
  static struct passwd *getpwent (void);
  static struct passwd *getpwnam (const char *user);
  static struct passwd *getpwnam_r (const char *name,
                                    struct passwd *pwent,
                                    char *buffer,
                                    int buflen);

  // = A set of wrappers for regular expressions.
  static char *compile (const char *instring,
                        char *expbuf,
                        char *endbuf);
  static int step (const char *str,
                   char *expbuf);

  // = A set of wrappers for non-UNICODE string operations.
  static int to_lower (int c);
  static int strcasecmp (const char *s,
                         const char *t);
  static int strncasecmp (const char *s,
                          const char *t,
                          size_t len);
  static char *strcat (char *s,
                       const char *t);
  static char *strchr (char *s,
                       int c);
  static const char *strchr (const char *s,
                             int c);
  static char *strrchr (char *s,
                        int c);
  static const char *strrchr (const char *s,
                              int c);
  static char *strnchr (char *s,
                        int c,
                        size_t len);
  static const char *strnchr (const char *s,
                              int c,
                              size_t len);
  static int strcmp (const char *s,
                     const char *t);
  static int strncmp (const char *s,
                      const char *t,
                      size_t len);
  static char *strcpy (char *s,
                       const char *t);
  static char *strecpy (char *des, const char *src);
  // Copies <src> to <des>, returning a pointer to the end of the
  // copied region, rather than the beginning, as <strcpy> does.
  static char *strpbrk (char *s1,
                        const char *s2);
  static const char *strpbrk (const char *s1,
                              const char *s2);
  static size_t strcspn (const char *s,
                         const char *reject);
  static size_t strspn(const char *s1,
                       const char *s2);
#if defined (PDL_HAS_STRPTIME)
  static char *strptime (char *buf,
                         const char *format,
                         struct tm *tm);
#endif /* PDL_HAS_STRPTIME */
  static char *strstr (char *s,
                       const char *t);
  static const char *strstr (const char *s,
                             const char *t);
  static char *strnstr (char *s,
                        const char *t,
                        size_t len);
  static const char *strnstr (const char *s,
                              const char *t,
                              size_t len);
  static char *strdup (const char *s); // Uses malloc
  static char *strenvdup (const char *str);
  static size_t strlen (const char *s);
  static char *strncpy (char *s,
                        const char *t,
                        size_t len);
  static char *strncat (char *s,
                        const char *t,
                        size_t len);
  static char *strtok (char *s,
                       const char *tokens);
  static char *strtok_r (char *s,
                         const char *tokens,
                         char **lasts);
  static long strtol (const char *s,
                      char **ptr,
                      int base);
  static u_long strtoul (const char *s,
                         char **ptr,
                         int base);
  static double strtod (const char *s,
                        char **endptr);
  static int pdl_isspace (const char s);

# if !defined (PDL_HAS_WCHAR_TYPEDEFS_CHAR)
  static size_t strlen (const wchar_t *s);
  static wchar_t *strcpy (wchar_t *s,
                          const wchar_t *t);
  static int strcmp (const wchar_t *s,
                     const wchar_t *t);
  static size_t strspn (const wchar_t *string,
                        const wchar_t *charset);
  static wchar_t *strenvdup (const wchar_t *str);
# endif /* ! PDL_HAS_WCHAR_TYPEDEFS_CHAR */

#if !defined (PDL_HAS_WCHAR_TYPEDEFS_USHORT)
  static size_t strlen (const PDL_UINT16 *s);
  static PDL_UINT16 *strcpy (PDL_UINT16 *s,
                             const PDL_UINT16 *t);
  static int strcmp (const PDL_USHORT16 *s,
                     const PDL_USHORT16 *t);
#endif /* ! PDL_HAS_WCHAR_TYPEDEFS_USHORT */

  // The following WChar typedef and functions are used by TAO.  TAO
  // does not use wchar_t because the size of wchar_t is
  // platform-dependent. These are to be used for all
  // manipulate\ions of CORBA::WString.
  typedef PDL_UINT16 WChar;
  static u_int wslen (const WChar *);
  static WChar *wscpy (WChar *,
                       const WChar *);
  static int wscmp (const WChar *,
                    const WChar *);
  static int wsncmp (const WChar *,
                     const WChar *,
                     size_t len);

# if defined (PDL_HAS_UNICODE)
  // = A set of wrappers for UNICODE string operations.
  static int atoi (const wchar_t *s);
  static wint_t to_lower (wint_t c);
  static wchar_t *strcat (wchar_t *s,
                          const wchar_t *t);
  static wchar_t *strchr (wchar_t *s,
                          wint_t c);
  static const wchar_t *strchr (const wchar_t *s,
                                wint_t c);
  static wchar_t *strecpy (wchar_t *s, const wchar_t *t);
  static wchar_t *strrchr (wchar_t *s,
                           wint_t c);
  static const wchar_t *strrchr (const wchar_t *s,
                                 wint_t c);
  static wchar_t *strnchr (wchar_t *s,
                           wint_t c,
                           size_t len);
  static const wchar_t *strnchr (const wchar_t *s,
                                 wint_t c,
                                 size_t len);
  static int strncmp (const wchar_t *s,
                      const wchar_t *t,
                      size_t len);
  static int strcasecmp (const wchar_t *s,
                         const wchar_t *t);
  static int strncasecmp (const wchar_t *s,
                          const wchar_t *t,
                          size_t len);
  static wchar_t *strpbrk (wchar_t *s1,
                           const wchar_t *s2);
  static const wchar_t *strpbrk (const wchar_t *s1,
                                 const wchar_t *s2);
  static wchar_t *strncpy (wchar_t *s,
                           const wchar_t *t,
                           size_t len);
  static wchar_t *strncat (wchar_t *s,
                           const wchar_t *t,
                           size_t len);
  static wchar_t *strtok (wchar_t *s,
                          const wchar_t *tokens);
  static long strtol (const wchar_t *s,
                      wchar_t **ptr,
                      int base);
  static u_long strtoul (const wchar_t *s,
                         wchar_t **ptr,
                         int base);
  static double strtod (const wchar_t *s,
                        wchar_t **endptr);
  static int pdl_isspace (wchar_t c);

#   if defined (PDL_WIN32)
  static wchar_t *strstr (wchar_t *s,
                          const wchar_t *t);
  static const wchar_t *strstr (const wchar_t *s,
                                const wchar_t *t);
  static wchar_t *strnstr (wchar_t *s,
                           const wchar_t *t,
                           size_t len);
  static const wchar_t *strnstr (const wchar_t *s,
                                 const wchar_t *t,
                                 size_t len);
  static wchar_t *strdup (const wchar_t *s); // Uses malloc
  static int sprintf (wchar_t *buf,
                      const wchar_t *format,
                      ...);
#     if 0
  static int sprintf (wchar_t *buf,
                      const char *format,
                      ...);
#     endif /* 0 */
  // the following three are newly added for CE.
  // but they can also be use on Win32.
  static wchar_t *fgets (wchar_t *buf,
                         int size,
                         FILE *fp);
  static int fprintf (FILE *fp,
                      const wchar_t *format,
                      ...);
  static void perror (const wchar_t *s);

  static int vsprintf (wchar_t *buffer,
                       const wchar_t *format,
                       va_list argptr);

  static int access (const wchar_t *path,
                     int amode);
  static FILE *fopen (const wchar_t *filename,
                      const wchar_t *mode);
  static FILE *fdopen (PDL_HANDLE handle,
                       const wchar_t *mode);
  static int stat (const wchar_t *file,
                   struct stat *);
  static int truncate (const wchar_t *filename,
                       off_t length);
  static int putenv (const wchar_t *str);
  static wchar_t *getenv (const wchar_t *symbol);
  static int system (const wchar_t *s);
  static int hostname (wchar_t *name,
                       size_t maxnamelen);
  static PDL_HANDLE open (const wchar_t *filename,
                          int mode,
                          int perms = 0,
                          LPSECURITY_ATTRIBUTES sa = 0);
  static int rename (const wchar_t *oldname,
                     const wchar_t *newname);
  static int unlink (const wchar_t *path);
  static wchar_t *mktemp (wchar_t *t);
  static int mkdir (const wchar_t *path,
                    mode_t mode = PDL_DEFAULT_DIR_PERMS);
  static int chdir (const wchar_t *path);
  static wchar_t *getcwd (wchar_t *,
                          size_t);
  static int mkfifo (const wchar_t *file,
                          mode_t mode = PDL_DEFAULT_FILE_PERMS);
#   endif /* PDL_WIN32 */
# endif /* PDL_HAS_UNICODE */

  // = A set of wrappers for TLI.
  static int t_accept (PDL_HANDLE fildes,
                       int resfd,
                       struct t_call
                       *call);
  static char *t_alloc (PDL_HANDLE fildes,
                        int struct_type,
                        int
                        fields);
  static int t_bind (PDL_HANDLE fildes,
                     struct t_bind *req,
                     struct
                     t_bind *ret);
  static int t_close (PDL_HANDLE fildes);
  static int t_connect(int fildes,
                       struct t_call *sndcall,
                       struct t_call *rcvcall);
  static void t_error (const char *errmsg);
  static int t_free (char *ptr,
                     int struct_type);
  static int t_getinfo (PDL_HANDLE fildes,
                        struct t_info *info);
  static int t_getname (PDL_HANDLE fildes,
                        struct netbuf *namep,
                        int type);
  static int t_getstate (PDL_HANDLE fildes);
  static int t_listen (PDL_HANDLE fildes,
                       struct t_call *call);
  static int t_look (PDL_HANDLE fildes);
  static int t_open (char *path,
                     int oflag,
                     struct t_info *info);
  static int t_optmgmt (PDL_HANDLE fildes,
                        struct t_optmgmt *req,
                        struct t_optmgmt *ret);
  static int t_rcv (PDL_HANDLE fildes,
                    char *buf,
                    u_int nbytes,
                    int *flags);
  static int t_rcvdis (PDL_HANDLE fildes,
                       struct t_discon *discon);
  static int t_rcvrel (PDL_HANDLE fildes);
  static int t_rcvudata (PDL_HANDLE fildes,
                         struct t_unitdata *unitdata,
                         int *flags);
  static int t_rcvuderr (PDL_HANDLE fildes,
                         struct t_uderr *uderr);
  static int t_snd (PDL_HANDLE fildes,
                    char *buf,
                    u_int nbytes,
                    int flags);
  static int t_snddis (PDL_HANDLE fildes,
                       struct t_call *call);
  static int t_sndrel (PDL_HANDLE fildes);
  static int t_sync (PDL_HANDLE fildes);
  static int t_unbind (PDL_HANDLE fildes);

# if 0
  // = A set of wrappers for threads (these are portable since they use the PDL_Thread_ID).
  static int thr_continue (const PDL_Thread_ID &thread);
  static int thr_create (PDL_THR_FUNC,
                         void *args,
                         long flags,
                         PDL_Thread_ID *,
                         long priority = PDL_DEFAULT_THREAD_PRIORITY,
                         void *stack = 0,
                         size_t stacksize = 0);
  static int thr_getprio (PDL_Thread_ID thr_id,
                          int &prio,
                          int *policy = 0);
  static int thr_join (PDL_Thread_ID waiter_id,
                       void **status);
  static int thr_kill (PDL_Thread_ID thr_id,
                       int signum);
  static PDL_Thread_ID thr_self (void);
  static int thr_setprio (PDL_Thread_ID thr_id,
                          int prio);
  static int thr_setprio (const PDL_Sched_Priority prio);
  static int thr_suspend (PDL_Thread_ID target_thread);
  static int thr_cancel (PDL_Thread_ID t_id);
# endif /* 0 */

  // = A set of wrappers for threads

  // These are non-portable since they use PDL_thread_t and
  // PDL_hthread_t and will go away in a future release.
  static int thr_continue (PDL_hthread_t target_thread);
  static int thr_create (PDL_THR_FUNC func,
                         void *args,
                         long flags,
                         PDL_thread_t *thr_id,
                         PDL_hthread_t *t_handle = 0,
                         long priority = PDL_DEFAULT_THREAD_PRIORITY,
                         void *stack = 0,
                         size_t stacksize = 0,
                         PDL_Thread_Adapter *thread_adapter = 0);
  // Creates a new thread having <flags> attributes and running <func>
  // with <args> (if <thread_adapter> is non-0 then <func> and <args>
  // are ignored and are obtained from <thread_adapter>).  <thr_id>
  // and <t_handle> are set to the thread's ID and handle (?),
  // respectively.  The thread runs at <priority> priority (see
  // below).
  //
  // The <flags> are a bitwise-OR of the following:
  // = BEGIN<INDENT>
  // THR_CANCEL_DISABLE, THR_CANCEL_ENABLE, THR_CANCEL_DEFERRED,
  // THR_CANCEL_ASYNCHRONOUS, THR_BOUND, THR_NEW_LWP, THR_DETACHED,
  // THR_SUSPENDED, THR_DAEMON, THR_JOINABLE, THR_SCHED_FIFO,
  // THR_SCHED_RR, THR_SCHED_DEFAULT
  // = END<INDENT>
  //
  // By default, or if <priority> is set to
  // PDL_DEFAULT_THREAD_PRIORITY, an "appropriate" priority value for
  // the given scheduling policy (specified in <flags}>, e.g.,
  // <THR_SCHED_DEFAULT>) is used.  This value is calculated
  // dynamically, and is the median value between the minimum and
  // maximum priority values for the given policy.  If an explicit
  // value is given, it is used.  Note that actual priority values are
  // EXTREMEMLY implementation-dependent, and are probably best
  // avoided.
  //
  // Note that <thread_adapter> is always deleted by <thr_create>,
  // therefore it must be allocated with global operator new.

  static int thr_getprio (PDL_hthread_t thr_id,
                          int &prio);
  static int thr_join (PDL_hthread_t waiter_id,
                       void **status);
  static int thr_join (PDL_thread_t waiter_id,
                       PDL_thread_t *thr_id,
                       void **status);
  static int thr_kill (PDL_thread_t thr_id,
                       int signum);
  static PDL_thread_t thr_self (void);
  static void thr_self (PDL_hthread_t &);
  static int thr_setprio (PDL_hthread_t thr_id,
                          int prio);
  static int thr_setprio (const PDL_Sched_Priority prio);
  static int thr_suspend (PDL_hthread_t target_thread);
  static int thr_cancel (PDL_thread_t t_id);

  static int thr_cmp (PDL_hthread_t t1,
                      PDL_hthread_t t2);
  static int thr_equal (PDL_thread_t t1,
                        PDL_thread_t t2);
  static void thr_exit (void *status = 0);
  static int thr_getconcurrency (void);
  static int lwp_getparams (PDL_Sched_Params &);
# if defined (PDL_HAS_TSS_EMULATION) && defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
  static int thr_getspecific (PDL_OS_thread_key_t key,
                              void **data);
# endif /* PDL_HAS_TSS_EMULATION && PDL_HAS_THREAD_SPECIFIC_STORAGE */
  static int thr_getspecific (PDL_thread_key_t key,
                              void **data);
  static int thr_keyfree (PDL_thread_key_t key);
  static int thr_key_detach (void *inst);
# if defined (PDL_HAS_THR_C_DEST)
#   if defined (PDL_HAS_TSS_EMULATION) && defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
  static int thr_keycreate (PDL_OS_thread_key_t *key,
                            PDL_THR_C_DEST,
                            void *inst = 0);
#   endif /* PDL_HAS_TSS_EMULATION && PDL_HAS_THREAD_SPECIFIC_STORAGE */
  static int thr_keycreate (PDL_thread_key_t *key,
                            PDL_THR_C_DEST,
                            void *inst = 0);
# else
#   if defined (PDL_HAS_TSS_EMULATION) && defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
  static int thr_keycreate (PDL_OS_thread_key_t *key,
                            PDL_THR_DEST,
                            void *inst = 0);
#   endif /* PDL_HAS_TSS_EMULATION && PDL_HAS_THREAD_SPECIFIC_STORAGE */
  static int thr_keycreate (PDL_thread_key_t *key,
                            PDL_THR_DEST,
                            void *inst = 0);
# endif /* PDL_HAS_THR_C_DEST */
  static int thr_key_used (PDL_thread_key_t key);
  static size_t thr_min_stack (void);
  static int thr_setconcurrency (int hint);
  static int lwp_setparams (const PDL_Sched_Params &);
# if defined (PDL_HAS_TSS_EMULATION) && defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
  static int thr_setspecific (PDL_OS_thread_key_t key,
                              void *data);
# endif /* PDL_HAS_TSS_EMULATION && PDL_HAS_THREAD_SPECIFIC_STORAGE */
  static int thr_setspecific (PDL_thread_key_t key,
                              void *data);
  static int thr_sigsetmask (int how,
                             const sigset_t *nsm,
                             sigset_t *osm);
  static int thr_setcancelstate (int new_state,
                                 int *old_state);
  static int thr_setcanceltype (int new_type,
                                int *old_type);
  static int sigwait (sigset_t *set,
                      int *sig = 0);
  static int sigtimedwait (const sigset_t *set,
                           siginfo_t *info,
                           const PDL_Time_Value *timeout);
  static void thr_testcancel (void);
  static void thr_yield (void);

  static void unique_name (const void *object,
                           LPTSTR name,
                           size_t length);
  // This method uses process id and object pointer to come up with a
  // machine wide unique name.  The process ID will provide uniqueness
  // between processes on the same machine. The "this" pointer of the
  // <object> will provide uniqueness between other "live" objects in
  // the same process. The uniqueness of this name is therefore only
  // valid for the life of <object>.

  static PDL_thread_t NULL_thread;
  // This is necessary to deal with POSIX pthreads and their use of
  // structures for thread ids.

  static PDL_hthread_t NULL_hthread;
  // This is necessary to deal with POSIX pthreads and their use of
  // structures for thread handles.

  static PDL_thread_key_t NULL_key;
  // This is necessary to deal with POSIX pthreads and their use of
  // structures for TSS keys.

# if defined (CHORUS)
  static KnCap actorcaps_[PDL_CHORUS_MAX_ACTORS];
  // This is used to map an actor's id into a KnCap for killing and
  // waiting actors.
# endif /* CHORUS */

# if defined (PDL_WIN32)
  static int socket_initialized_;
  // Keeps track of whether we've already initialized WinSock...
# endif /* PDL_WIN32 */

  static void mutex_lock_cleanup (void *mutex);
  // Handle asynchronous thread cancellation cleanup.

  static void cleanup_tss (const u_int main_thread);
  // Call TSS destructors for the current thread.  If the current
  // thread is the main thread, then the argument must be 1.
  // For private use of PDL_Object_Manager and PDL_Thread_Adapter only.

# if defined (PDL_MT_SAFE) && (PDL_MT_SAFE != 0) && defined (PDL_LACKS_NETDB_REENTRANT_FUNCTIONS)
  static int netdb_acquire (void);
  static int netdb_release (void);
# endif /* defined (PDL_MT_SAFE) && PDL_LACKS_NETDB_REENTRANT_FUNCTIONS */

  static int scheduling_class (const char *class_name, PDL_id_t &);
  // Find the schedling class ID that corresponds to the class name.

  static int set_scheduling_params (const PDL_Sched_Params &,
                                    PDL_id_t id = PDL_SELF);
  // Friendly interface to <priocntl>(2).

  // Can't call the following priocntl, because that's a macro on Solaris.
  static int priority_control (PDL_idtype_t, PDL_id_t, int, void *);
  // Low-level interface to <priocntl>(2).

private:
  static PDL_EXIT_HOOK exit_hook_;
  // Function that is called by <PDL_OS::exit>, if non-null.

  static PDL_EXIT_HOOK set_exit_hook (PDL_EXIT_HOOK hook);
  // For use by PDL_Object_Manager only, to register its exit hook..

  friend class PDL_OS_Object_Manager;
  // Allow the PDL_OS_Object_Manager to call set_exit_hook.

#if defined (PDL_HAS_STRPTIME)  &&  defined (PDL_LACKS_NATIVE_STRPTIME)
  static int strptime_getnum (char *buf,
                              int *num,
                              int *bi,
                              int *fi,
                              int min,
                              int max);
#endif /* PDL_HAS_STRPTIME  &&  PDL_LACKS_NATIVE_STRPTIME */

# if defined (PDL_WIN32)
#   if defined (PDL_HAS_WINCE)
  static const wchar_t *day_of_week_name[7];
  static const wchar_t *month_name[12];
  // Supporting data for ctime and ctime_r functions on WinCE.
#   endif /* PDL_HAS_WINCE */

  static void fopen_mode_to_open_mode_converter (char x,
                                                 int &hmode);
  // Translate fopen's mode char to open's mode.  This helper function
  // is here to avoid maintaining several pieces of identical code.

  static OSVERSIONINFO win32_versioninfo_;

# endif /* PDL_WIN32 */
};

class PDL_Export PDL_Object_Manager_Base
{
  // = TITLE
  //     Base class for PDL_Object_Manager(s).
  //
  // = DESCRIPTION
  //     Encapsulates the most useful PDL_Object_Manager data structures.
# if (defined (PDL_PSOS) && defined (__DIAB))  || \
     (defined (__DECCXX_VER) && __DECCXX_VER < 60000000)
  // The Diab compiler got confused and complained about access rights
  // if this section was protected (changing this to public makes it happy).
  // Similarly, DEC CXX 5.6 needs the methods to be public.
public:
# else  /* ! (PDL_PSOS && __DIAB)  ||  ! __DECCXX_VER < 60000000 */
protected:
# endif /* ! (PDL_PSOS && __DIAB)  ||  ! __DECCXX_VER < 60000000 */
  PDL_Object_Manager_Base (void);
  // Default constructor.

  virtual ~PDL_Object_Manager_Base (void);
  // Destructor.

public:
  virtual int init (void) = 0;
  // Explicitly initialize.  Returns 0 on success, -1 on failure due
  // to dynamic allocation failure (in which case errno is set to
  // ENOMEM), or 1 if it had already been called.

  virtual int fini (void) = 0;
  // Explicitly destroy.  Returns 0 on success, -1 on failure because
  // the number of fini () calls hasn't reached the number of init ()
  // calls, or 1 if it had already been called.

  enum Object_Manager_State
    {
      OBJ_MAN_UNINITIALIZED = 0,
      OBJ_MAN_INITIALIZING,
      OBJ_MAN_INITIALIZED,
      OBJ_MAN_SHUTTING_DOWN,
      OBJ_MAN_SHUT_DOWN
    };

protected:
  int starting_up_i (void);
  // Returns 1 before PDL_Object_Manager_Base has been constructed.
  // This flag can be used to determine if the program is constructing
  // static objects.  If no static object spawns any threads, the
  // program will be single-threaded when this flag returns 1.  (Note
  // that the program still might construct some static objects when
  // this flag returns 0, if PDL_HAS_NONSTATIC_OBJECT_MANAGER is not
  // defined.)

  int shutting_down_i (void);
  // Returns 1 after PDL_Object_Manager_Base has been destroyed.  This
  // flag can be used to determine if the program is in the midst of
  // destroying static objects.  (Note that the program might destroy
  // some static objects before this flag can return 1, if
  // PDL_HAS_NONSTATIC_OBJECT_MANAGER is not defined.)

  Object_Manager_State object_manager_state_;
  // State of the Object_Manager;

  u_int dynamically_allocated_;
  // Flag indicating whether the PDL_Object_Manager was dynamically
  // allocated by PDL.  (If is was dynamically allocated by the
  // application, then the application is responsible for destroying
  // it.)

  PDL_Object_Manager_Base *next_;
  // Link to next Object_Manager, for chaining.
private:
  // Disallow copying by not implementing the following . . .
  PDL_Object_Manager_Base (const PDL_Object_Manager_Base &);
  PDL_Object_Manager_Base &operator= (const PDL_Object_Manager_Base &);
};

extern "C"
void
PDL_OS_Object_Manager_Internal_Exit_Hook (void);

class PDL_Export PDL_OS_Object_Manager : public PDL_Object_Manager_Base
{
public:
  virtual int init (void);
  // Explicitly initialize.

  virtual int fini (void);
  // Explicitly destroy.

  static int starting_up (void);
  // Returns 1 before the <PDL_OS_Object_Manager> has been
  // constructed.  See <PDL_Object_Manager::starting_up> for more
  // information.

  static int shutting_down (void);
  // Returns 1 after the <PDL_OS_Object_Manager> has been destroyed.
  // See <PDL_Object_Manager::shutting_down> for more information.

  enum Preallocated_Object
    {
# if defined (PDL_MT_SAFE) && (PDL_MT_SAFE != 0)
      PDL_OS_MONITOR_LOCK,
      PDL_TSS_CLEANUP_LOCK,
      PDL_LOG_MSG_INSTANCE_LOCK,
#   if defined (PDL_HAS_TSS_EMULATION)
      PDL_TSS_KEY_LOCK,
#     if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
      PDL_TSS_BASE_LOCK,
#     endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */
#   endif /* PDL_HAS_TSS_EMULATION */
# else
      // Without PDL_MT_SAFE, There are no preallocated objects.  Make
      // sure that the preallocated_array size is at least one by
      // declaring this dummy . . .
      PDL_OS_EMPTY_PREALLOCATED_OBJECT,
# endif /* PDL_MT_SAFE */

      PDL_OS_PREALLOCATED_OBJECTS  // This enum value must be last!
    };
  // Unique identifiers for preallocated objects.

  static sigset_t *default_mask (void);
  // Accesses a default signal set used, for example, in
  // <PDL_Sig_Guard> methods.

  static PDL_Thread_Hook *thread_hook (void);
  // Returns the current thread hook for the process.

  static PDL_Thread_Hook *thread_hook (PDL_Thread_Hook *new_thread_hook);
  // Returns the existing thread hook and assign a <new_thread_hook>.

public:
  // = Applications shouldn't use these so they're hidden here.

  // They're public so that the <PDL_Object_Manager> can be
  // constructed/destructed in <main> with
  // <PDL_HAS_NONSTATIC_OBJECT_MANAGER>.
  PDL_OS_Object_Manager (void);
  // Constructor.

  ~PDL_OS_Object_Manager (void);
  // Destructor.

private:
  static PDL_OS_Object_Manager *instance (void);
  // Accessor to singleton instance.

  static PDL_OS_Object_Manager *instance_;
  // Singleton instance pointer.

  static void *preallocated_object[PDL_OS_PREALLOCATED_OBJECTS];
  // Table of preallocated objects.

  sigset_t *default_mask_;
  // Default signal set used, for example, in PDL_Sig_Guard.

  PDL_Thread_Hook *thread_hook_;
  // Thread hook that's used by this process.

  PDL_OS_Exit_Info exit_info_;
  // For at_exit support.

  int at_exit (PDL_EXIT_HOOK func);
  // For <PDL_OS::atexit> support.

  static void print_error_message (u_int line_number, LPCTSTR message);
  // For use by init () and fini (), to consolidate error reporting.

  friend class PDL_OS;
  friend class PDL_Object_Manager;
  friend class PDL_OS_Object_Manager_Manager;
  friend class PDL_TSS_Cleanup;
  friend class PDL_TSS_Emulation;
  friend class PDL_Log_Msg;
  friend void PDL_OS_Object_Manager_Internal_Exit_Hook ();
  // This class is for internal use by PDL_OS, etc., only.
};

# if defined (PDL_HAS_WINCE)

//          **** Warning ****
// You should not use the following functions under CE at all.
//
//          **** Warning ****

size_t fwrite (void *buf, size_t sz, size_t count, FILE *fp);
size_t fread (void *buf, size_t sz, size_t count, FILE *fp);
int getc (FILE *fp);
int ferror (FILE *fp);
int isatty (int h);
int fileno (FILE *fp);
int fflush (FILE *fp);
#   if defined (UNDER_CE) && (UNDER_CE < 211)
void exit (int status);
#   endif /* UNDER_CE && UNDER_CE < 211 */
int fprintf (FILE *fp, char *format, const char *msg); // not a general purpose
                                                 // fprintf at all.
int printf (const char *format, ...);
int putchar (int c);

//          **** End CE Warning ****

# endif /* PDL_HAS_WINCE */

# if defined (PDL_LACKS_TIMEDWAIT_PROTOTYPES)
extern "C" ssize_t recv_timedwait (PDL_HANDLE handle,
                                   char *buf,
                                   int len,
                                   int flags,
                                   struct timespec *timeout);
extern "C" ssize_t read_timedwait (PDL_HANDLE handle,
                                   char *buf,
                                   size_t n,
                                   struct timespec *timeout);
extern "C" ssize_t recvmsg_timedwait (PDL_HANDLE handle,
                                      struct msghdr *msg,
                                      int flags,
                                      struct timespec *timeout);
extern "C" ssize_t recvfrom_timedwait (PDL_HANDLE handle,
                                       char *buf,
                                       int len,
                                       int flags,
                                       struct sockaddr *addr,
                                       int
                                       *addrlen,
                                       struct timespec *timeout);
extern "C" ssize_t readv_timedwait (PDL_HANDLE handle,
                                    iovec *iov,
                                    int iovcnt,
                                    struct timespec* timeout);
extern "C" ssize_t send_timedwait (PDL_HANDLE handle,
                                   const char *buf,
                                   int len,
                                   int flags,
                                   struct timespec *timeout);
extern "C" ssize_t write_timedwait (PDL_HANDLE handle,
                                    const void *buf,
                                    size_t n,
                                    struct timespec *timeout);
extern "C" ssize_t sendmsg_timedwait (PDL_HANDLE handle,
                                      PDL_SENDMSG_TYPE *msg,
                                      int flags,
                                      struct timespec *timeout);
extern "C" ssize_t sendto_timedwait (PDL_HANDLE handle,
                                     const char *buf,
                                     int len,
                                     int flags,
                                     const struct sockaddr *addr,
                                     int addrlen,
                                     struct timespec *timeout);
extern "C" ssize_t writev_timedwait (PDL_HANDLE handle,
                                     PDL_WRITEV_TYPE *iov,
                                     int iovcnt,
                                     struct timespec *timeout);
# endif /* PDL_LACKS_TIMEDWAIT_PROTOTYPES */

# if defined (PDL_HAS_TSS_EMULATION)
    // Allow config.h to set the default number of thread keys.
#   if !defined (PDL_DEFAULT_THREAD_KEYS)
#     define PDL_DEFAULT_THREAD_KEYS 64
#   endif /* ! PDL_DEFAULT_THREAD_KEYS */

class PDL_Export PDL_TSS_Emulation
{
  // = TITLE
  //     Thread-specific storage emulation.
  //
  // = DESCRIPTION
  //     This provides a thread-specific storage implementation.
  //     It is intended for use on platforms that don't have a
  //     native TSS, or have a TSS with limitations such as the
  //     number of keys or lack of support for removing keys.
public:
  typedef void (*PDL_TSS_DESTRUCTOR)(void *value) /* throw () */;

  enum { PDL_TSS_THREAD_KEYS_MAX = PDL_DEFAULT_THREAD_KEYS };
  // Maximum number of TSS keys allowed over the life of the program.

  static u_int total_keys ();
  // Returns the total number of keys allocated so far.

  static int next_key (PDL_thread_key_t &key);
  // Sets the argument to the next available key.  Returns 0 on success,
  // -1 if no keys are available.

  static PDL_TSS_DESTRUCTOR tss_destructor (const PDL_thread_key_t key);
  // Returns the exit hook associated with the key.  Does _not_ check
  // for a valid key.

  static void tss_destructor (const PDL_thread_key_t key,
                              PDL_TSS_DESTRUCTOR destructor);
  // Associates the TSS destructor with the key.  Does _not_ check
  // for a valid key.

  static void *&ts_object (const PDL_thread_key_t key);
  // Accesses the object referenced by key in the current thread's TSS array.
  // Does _not_ check for a valid key.

  static void *tss_open (void *ts_storage[PDL_TSS_THREAD_KEYS_MAX]);
  // Setup an array to be used for local TSS.  Returns the array
  // address on success.  Returns 0 if local TSS had already been
  // setup for this thread.  There is no corresponding tss_close ()
  // because it is not needed.
  // NOTE: tss_open () is called by PDL for threads that it spawns.
  // If your application spawns threads without using PDL, and it uses
  // PDL's TSS emulation, each of those threads should call tss_open
  // ().  See the pdl_thread_adapter () implementaiton for an example.

  static void tss_close ();
  // Shutdown TSS emulation.  For use only by PDL_OS::cleanup_tss ().

private:
  // Global TSS structures.
  static u_int total_keys_;
  // Always contains the value of the next key to be allocated.

  static PDL_TSS_DESTRUCTOR tss_destructor_ [PDL_TSS_THREAD_KEYS_MAX];
  // Array of thread exit hooks (TSS destructors) that are called for each
  // key (that has one) when the thread exits.

#   if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
  static void **tss_base (void* ts_storage[] = 0, u_int *ts_created = 0);
  // Location of current thread's TSS array.
#   else  /* ! PDL_HAS_THREAD_SPECIFIC_STORAGE */
  static void **&tss_base ();
  // Location of current thread's TSS array.
#   endif /* ! PDL_HAS_THREAD_SPECIFIC_STORAGE */

#   if defined (PDL_HAS_THREAD_SPECIFIC_STORAGE)
  // Rely on native thread specific storage for the implementation,
  // but just use one key.
  static PDL_OS_thread_key_t native_tss_key_;

  // Used to indicate if native tss key has been allocated
  static int key_created_;
#   endif /* PDL_HAS_THREAD_SPECIFIC_STORAGE */
};

# else   /* ! PDL_HAS_TSS_EMULATION */
#   if defined (TLS_MINIMUM_AVAILABLE)
    // WIN32 platforms define TLS_MINIMUM_AVAILABLE natively.
#     define PDL_DEFAULT_THREAD_KEYS TLS_MINIMUM_AVAILABLE
#   endif /* TSL_MINIMUM_AVAILABLE */

# endif /* PDL_HAS_TSS_EMULATION */

// moved PDL_TSS_Ref, PDL_TSS_Info, and PDL_TSS_Keys class
// declarations from OS.cpp so they are visible to the single
// file of template instantiations.
# if defined (PDL_WIN32) || defined (PDL_HAS_TSS_EMULATION) || (defined (PDL_PSOS) && defined (PDL_PSOS_HAS_TSS))
class PDL_TSS_Ref
{
  // = TITLE
  //     "Reference count" for thread-specific storage keys.
  //
  // = DESCRIPTION
  //     Since the <PDL_Unbounded_Stack> doesn't allow duplicates, the
  //     "reference count" is the identify of the thread_id.
public:
  PDL_TSS_Ref (PDL_thread_t id);
  // Constructor

  PDL_TSS_Ref (void);
  // Default constructor

  int operator== (const PDL_TSS_Ref &) const;
  // Check for equality.

  int operator!= (const PDL_TSS_Ref &) const;
  // Check for inequality.

// private:

  PDL_thread_t tid_;
  // ID of thread using a specific key.
};

class PDL_TSS_Info
{
  // = TITLE
  //     Thread Specific Key management.
  //
  // = DESCRIPTION
  //     This class maps a key to a "destructor."
public:
  PDL_TSS_Info (PDL_thread_key_t key,
                void (*dest)(void *) = 0,
                void *tss_inst = 0);
  // Constructor

  PDL_TSS_Info (void);
  // Default constructor

  int key_in_use (void) const { return thread_count_ != -1; }
  // Returns 1 if the key is in use, 0 if not.

  void key_in_use (int flag) { thread_count_ = flag == 0  ?  -1  :  1; }
  // Mark the key as being in use if the flag is non-zero, or
  // not in use if the flag is 0.

  int operator== (const PDL_TSS_Info &) const;
  // Check for equality.

  int operator!= (const PDL_TSS_Info &) const;
  // Check for inequality.

  void dump (void);
  // Dump the state.

private:
  PDL_thread_key_t key_;
  // Key to the thread-specific storage item.

  void (*destructor_)(void *);
  // "Destructor" that gets called when the item is finally released.

  void *tss_obj_;
  // Pointer to PDL_TSS<xxx> instance that has/will allocate the key.

  int thread_count_;
  // Count of threads that are using this key.  Contains -1 when the
  // key is not in use.

  friend class PDL_TSS_Cleanup;
};

class PDL_TSS_Keys
{
  // = TITLE
  //     Collection of in-use flags for a thread's TSS keys.
  //     For internal use only by PDL_TSS_Cleanup; it is public because
  //     some compilers can't use nested classes for template instantiation
  //     parameters.
  //
  // = DESCRIPTION
  //     Wrapper around array of whether each key is in use.  A simple
  //     typedef doesn't work with Sun C++ 4.2.
public:
  PDL_TSS_Keys (void);
  // Default constructor, to initialize all bits to zero (unused).

  int test_and_set (const PDL_thread_key_t key);
  // Mark the specified key as being in use, if it was not already so marked.
  // Returns 1 if the had already been marked, 0 if not.

  int test_and_clear (const PDL_thread_key_t key);
  // Mark the specified key as not being in use, if it was not already so
  // cleared.  Returns 1 if the had already been cleared, 0 if not.

private:
  static void find (const u_int key, u_int &word, u_int &bit);
  // For a given key, find the word and bit number that represent it.

  enum
    {
#   if PDL_SIZEOF_LONG == 8
      PDL_BITS_PER_WORD = 64,
#   elif PDL_SIZEOF_LONG == 4
      PDL_BITS_PER_WORD = 32,
#   else
#     error PDL_TSS_Keys only supports 32 or 64 bit longs.
#   endif /* PDL_SIZEOF_LONG == 8 */
      PDL_WORDS = (PDL_DEFAULT_THREAD_KEYS - 1) / PDL_BITS_PER_WORD + 1
    };

  u_long key_bit_words_[PDL_WORDS];
  // Bit flag collection.  A bit value of 1 indicates that the key is in
  // use by this thread.
};

# endif /* defined (PDL_WIN32) || defined (PDL_HAS_TSS_EMULATION) */

class PDL_Export PDL_OS_WString
{
  // = TITLE
  //     A lightweight wchar* to char* string conversion class.
  //
  // = DESCRIPTION
  //     The purpose of this class is to perform conversion between
  //     wchar and char strings.  It is not intended for general
  //     purpose use.
public:
  PDL_OS_WString (const PDL_USHORT16 *s);
  // Ctor must take a wchar stirng.

  ~PDL_OS_WString (void);
  // Dtor will free up the memory.

  char *char_rep (void);
  // Return the internal char* representation.

private:
  char *rep_;
  // Internal pointer to the converted string.

  PDL_OS_WString (void);
  PDL_OS_WString (PDL_OS_WString &);
  PDL_OS_WString& operator= (PDL_OS_WString &);
  // Disallow these operation.
};

class PDL_Export PDL_OS_CString
{
  // = TITLE
  //     A lightweight char* to wchar* string conversion class.
  //
  // = DESCRIPTION
  //     The purpose of this class is to perform conversion from
  //     char* to wchar* strings.  It is not intended for general
  //     purpose use.
public:
  PDL_OS_CString (const char *s);
  // Ctor must take a wchar stirng.

  ~PDL_OS_CString (void);
  // Dtor will free up the memory.

  PDL_USHORT16 *wchar_rep (void);
  // Return the internal char* representation.

private:
  PDL_USHORT16 *rep_;
  // Internal pointer to the converted string.

  PDL_OS_CString (void);
  PDL_OS_CString (PDL_OS_CString &);
  PDL_OS_CString operator= (PDL_OS_CString &);
  // Disallow these operation.
};

// Support non-scalar thread keys, such as with some POSIX
// implementations, e.g., MVS.
# if defined (PDL_HAS_NONSCALAR_THREAD_KEY_T)
#   define PDL_KEY_INDEX(OBJ,KEY) \
  u_int OBJ; \
  PDL_OS::memcpy (&OBJ, &KEY, sizeof (u_int))
# else
#   define PDL_KEY_INDEX(OBJ,KEY) u_int OBJ = KEY
# endif /* PDL_HAS_NONSCALAR_THREAD_KEY_T */

// A useful abstraction for expressions involving operator new since
// we can change memory allocation error handling policies (e.g.,
// depending on whether ANSI/ISO exception handling semantics are
// being used).

# if defined (PDL_NEW_THROWS_EXCEPTIONS)
# if (__SUNPRO_CC)
#      if (__SUNPRO_CC < 0x500) || ((__SUNPRO_CC == 0x500 && __SUNPRO_CC_COMPAT == 4))
#        include /**/ <exception.h>
         // Note: we catch ::xalloc rather than just xalloc because of
         // a name clash with unsafe_ios::xalloc()
#        define PDL_bad_alloc ::xalloc
#        define PDL_throw_bad_alloc throw PDL_bad_alloc ("no more memory")
#      else
#        include /**/ <new>
#        define PDL_bad_alloc std::bad_alloc
#        define PDL_throw_bad_alloc throw PDL_bad_alloc ()
#      endif /* __SUNPRO_CC < 0x500 */
#   else
    // I know this works for HP aC++... if <stdexcept> is used, it
    // introduces other stuff that breaks things, like <memory>, which
    // screws up auto_ptr.
#     include /**/ <new>
#     if (defined (__HP_aCC) && !defined (RWSTD_NO_NAMESPACE)) \
         || defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
#       define PDL_bad_alloc std::bad_alloc
#     else
#       define PDL_bad_alloc bad_alloc
#     endif /* RWSTD_NO_NAMESPACE */
#     define PDL_throw_bad_alloc throw PDL_bad_alloc ()
#   endif /* __SUNPRO_CC */

#   define PDL_NEW_RETURN(POINTER,CONSTRUCTOR,RET_VAL) \
   do { try { POINTER = new CONSTRUCTOR; } \
        catch (PDL_bad_alloc) { errno = ENOMEM; return RET_VAL; } \
   } while (0)

#   define PDL_NEW(POINTER,CONSTRUCTOR) \
   do { try { POINTER = new CONSTRUCTOR; } \
        catch (PDL_bad_alloc) { errno = ENOMEM; return; } \
   } while (0)

# else /* PDL_NEW_THROWS_EXCEPTIONS */

#   define PDL_NEW_RETURN(POINTER,CONSTRUCTOR,RET_VAL) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; return RET_VAL; } \
   } while (0)
#   define PDL_NEW(POINTER,CONSTRUCTOR) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; return; } \
   } while (0)

# define PDL_throw_bad_alloc \
      void* gcc_will_complain_if_literal_0_is_returned = 0; \
      return gcc_will_complain_if_literal_0_is_returned

# endif /* PDL_NEW_THROWS_EXCEPTIONS */

// Some useful abstrations for expressions involving
// PDL_Allocator.malloc ().  The difference between PDL_NEW_MALLOC*
// with PDL_ALLOCATOR* is that they call constructors also.

# define PDL_ALLOCATOR_RETURN(POINTER,ALLOCATOR,RET_VAL) \
   do { POINTER = ALLOCATOR; \
     if (POINTER == 0) { errno = ENOMEM; return RET_VAL; } \
   } while (0)
# define PDL_ALLOCATOR(POINTER,ALLOCATOR) \
   do { POINTER = ALLOCATOR; \
     if (POINTER == 0) { errno = ENOMEM; return; } \
   } while (0)

# define PDL_NEW_MALLOC_RETURN(POINTER,ALLOCATOR,CONSTRUCTOR,RET_VAL) \
   do { POINTER = ALLOCATOR; \
     if (POINTER == 0) { errno = ENOMEM; return RET_VAL;} \
     else { new (POINTER) CONSTRUCTOR; } \
   } while (0)
# define PDL_NEW_MALLOC(POINTER,ALLOCATOR,CONSTRUCTOR) \
   do { POINTER = ALLOCATOR; \
     if (POINTER == 0) { errno = ENOMEM; return;} \
     else { new (POINTER) CONSTRUCTOR; } \
   } while (0)

# define PDL_NOOP(x)

# define PDL_DES_NOFREE(POINTER,CLASS) \
   do { \
        if (POINTER) \
          { \
            (POINTER)->~CLASS (); \
          } \
      } \
   while (0)

# define PDL_DES_ARRAY_NOFREE(POINTER,SIZE,CLASS) \
   do { \
        if (POINTER) \
          { \
            for (size_t i = 0; \
                 i < SIZE; \
                 ++i) \
            { \
              (&(POINTER)[i])->~CLASS (); \
            } \
          } \
      } \
   while (0)

# define PDL_DES_FREE(POINTER,DEALLOCATOR,CLASS) \
   do { \
        if (POINTER) \
          { \
            (POINTER)->~CLASS (); \
            DEALLOCATOR (POINTER); \
          } \
      } \
   while (0)

# define PDL_DES_ARRAY_FREE(POINTER,SIZE,DEALLOCATOR,CLASS) \
   do { \
        if (POINTER) \
          { \
            for (size_t i = 0; \
                 i < SIZE; \
                 ++i) \
            { \
              (&(POINTER)[i])->~CLASS (); \
            } \
            DEALLOCATOR (POINTER); \
          } \
      } \
   while (0)

# if defined (PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR)
#   define PDL_DES_NOFREE_TEMPLATE(POINTER,T_CLASS,T_PARAMETER) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->~T_CLASS (); \
            } \
        } \
     while (0)
#   define PDL_DES_ARRAY_NOFREE_TEMPLATE(POINTER,SIZE,T_CLASS,T_PARAMETER) \
     do { \
          if (POINTER) \
            { \
              for (size_t i = 0; \
                   i < SIZE; \
                   ++i) \
              { \
                (&(POINTER)[i])->~T_CLASS (); \
              } \
            } \
        } \
     while (0)
#if defined(__IBMCPP__) && (__IBMCPP__ >= 400)
#   define PDL_DES_FREE_TEMPLATE(POINTER,DEALLOCATOR,T_CLASS,T_PARAMETER) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->~T_CLASS T_PARAMETER (); \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
#else
#   define PDL_DES_FREE_TEMPLATE(POINTER,DEALLOCATOR,T_CLASS,T_PARAMETER) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->~T_CLASS (); \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
#endif /* defined(__IBMCPP__) && (__IBMCPP__ >= 400) */
#   define PDL_DES_ARRAY_FREE_TEMPLATE(POINTER,SIZE,DEALLOCATOR,T_CLASS,T_PARAMETER) \
     do { \
          if (POINTER) \
            { \
              for (size_t i = 0; \
                   i < SIZE; \
                   ++i) \
              { \
                (&(POINTER)[i])->~T_CLASS (); \
              } \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
#if defined(__IBMCPP__) && (__IBMCPP__ >= 400)
#   define PDL_DES_FREE_TEMPLATE2(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->~T_CLASS <T_PARAM1, T_PARAM2> (); \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
#else
#   define PDL_DES_FREE_TEMPLATE2(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->~T_CLASS (); \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
#endif /* defined(__IBMCPP__) && (__IBMCPP__ >= 400) */
#   define PDL_DES_FREE_TEMPLATE3(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2,T_PARAM3) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->~T_CLASS (); \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
#   define PDL_DES_FREE_TEMPLATE4(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2,T_PARAM3, T_PARAM4) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->~T_CLASS (); \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
#   define PDL_DES_ARRAY_FREE_TEMPLATE2(POINTER,SIZE,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2) \
     do { \
          if (POINTER) \
            { \
              for (size_t i = 0; \
                   i < SIZE; \
                   ++i) \
              { \
                (&(POINTER)[i])->~T_CLASS (); \
              } \
              DEALLOCATOR (POINTER); \
            } \
        } \
     while (0)
# else /* ! PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR */
#   define PDL_DES_NOFREE_TEMPLATE(POINTER,T_CLASS,T_PARAMETER) \
     do { \
          if (POINTER) \
            { \
              (POINTER)->T_CLASS T_PARAMETER::~T_CLASS (); \
            } \
        } \
     while (0)
#   define PDL_DES_ARRAY_NOFREE_TEMPLATE(POINTER,SIZE,T_CLASS,T_PARAMETER) \
     do { \
          if (POINTER) \
            { \
              for (size_t i = 0; \
                   i < SIZE; \
                   ++i) \
              { \
                (POINTER)[i].T_CLASS T_PARAMETER::~T_CLASS (); \
              } \
            } \
        } \
     while (0)
#   if defined (__Lynx__) && __LYNXOS_SDK_VERSION == 199701L
  // LynxOS 3.0.0's g++ has trouble with the real versions of these.
#     define PDL_DES_FREE_TEMPLATE(POINTER,DEALLOCATOR,T_CLASS,T_PARAMETER)
#     define PDL_DES_ARRAY_FREE_TEMPLATE(POINTER,DEALLOCATOR,T_CLASS,T_PARAMETER)
#     define PDL_DES_FREE_TEMPLATE2(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2)
#     define PDL_DES_FREE_TEMPLATE3(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2)
#     define PDL_DES_FREE_TEMPLATE4(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2)
#     define PDL_DES_ARRAY_FREE_TEMPLATE2(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2)
#   else
#     define PDL_DES_FREE_TEMPLATE(POINTER,DEALLOCATOR,T_CLASS,T_PARAMETER) \
       do { \
            if (POINTER) \
              { \
                POINTER->T_CLASS T_PARAMETER::~T_CLASS (); \
                DEALLOCATOR (POINTER); \
              } \
          } \
       while (0)
#     define PDL_DES_ARRAY_FREE_TEMPLATE(POINTER,SIZE,DEALLOCATOR,T_CLASS,T_PARAMETER) \
       do { \
            if (POINTER) \
              { \
                for (size_t i = 0; \
                     i < SIZE; \
                     ++i) \
                { \
                  POINTER[i].T_CLASS T_PARAMETER::~T_CLASS (); \
                } \
                DEALLOCATOR (POINTER); \
              } \
          } \
       while (0)
#     define PDL_DES_FREE_TEMPLATE2(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2) \
       do { \
            if (POINTER) \
              { \
                POINTER->T_CLASS <T_PARAM1, T_PARAM2>::~T_CLASS (); \
                DEALLOCATOR (POINTER); \
              } \
          } \
       while (0)
#     define PDL_DES_FREE_TEMPLATE3(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2,T_PARAM3) \
       do { \
            if (POINTER) \
              { \
                POINTER->T_CLASS <T_PARAM1, T_PARAM2, T_PARAM3>::~T_CLASS (); \
                DEALLOCATOR (POINTER); \
              } \
          } \
       while (0)
#     define PDL_DES_FREE_TEMPLATE4(POINTER,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2,T_PARAM3,T_PARAM4) \
       do { \
            if (POINTER) \
              { \
                POINTER->T_CLASS <T_PARAM1, T_PARAM2, T_PARAM3, T_PARAM4>::~T_CLASS (); \
                DEALLOCATOR (POINTER); \
              } \
          } \
       while (0)
#     define PDL_DES_ARRAY_FREE_TEMPLATE2(POINTER,SIZE,DEALLOCATOR,T_CLASS,T_PARAM1,T_PARAM2) \
       do { \
            if (POINTER) \
              { \
                for (size_t i = 0; \
                     i < SIZE; \
                     ++i) \
                { \
                  POINTER[i].T_CLASS <T_PARAM1, T_PARAM2>::~T_CLASS (); \
                } \
                DEALLOCATOR (POINTER); \
              } \
          } \
       while (0)
#   endif /* defined (__Lynx__) && __LYNXOS_SDK_VERSION == 199701L */
# endif /* defined ! PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR */

# if defined (PDL_HAS_SIGNAL_SAFE_OS_CALLS)
// The following two macros ensure that system calls are properly
// restarted (if necessary) when interrupts occur.
#   define PDL_OSCALL(OP,TYPE,FAILVALUE,RESULT) \
  do \
    RESULT = (TYPE) OP; \
  while (RESULT == FAILVALUE && errno == EINTR && PDL_LOG_MSG->restart ())
#   define PDL_OSCALL_RETURN(OP,TYPE,FAILVALUE) \
  do { \
    TYPE pdl_result_; \
    do \
      pdl_result_ = (TYPE) OP; \
    while (pdl_result_ == FAILVALUE && errno == EINTR && PDL_LOG_MSG->restart ()); \
    return pdl_result_; \
  } while (0)
# elif defined (PDL_WIN32)
#   define PDL_OSCALL_RETURN(X,TYPE,FAILVALUE) \
  do \
    return (TYPE) X; \
  while (0)
#   define PDL_OSCALL(X,TYPE,FAILVALUE,RESULT) \
  do \
    RESULT = (TYPE) X; \
  while (0)
#   if defined (__BORLANDC__) && (__BORLANDC__ <= 0x550)
#   define PDL_WIN32CALL_RETURN(X,TYPE,FAILVALUE) \
  do { \
    TYPE pdl_result_; \
    TYPE pdl_local_result_ = (TYPE) X; \
    pdl_result_ = pdl_local_result_; \
    if (pdl_result_ == FAILVALUE) \
      PDL_OS::set_errno_to_last_error (); \
    return pdl_result_; \
  } while (0)
#   else
#     define PDL_WIN32CALL_RETURN(X,TYPE,FAILVALUE) \
  do { \
    TYPE pdl_result_; \
    pdl_result_ = (TYPE) X; \
    if (pdl_result_ == FAILVALUE) \
      PDL_OS::set_errno_to_last_error (); \
    return pdl_result_; \
  } while (0)
#   endif /* defined (__BORLANDC__) && (__BORLANDC__ <= 0x550) */
#   define PDL_WIN32CALL(X,TYPE,FAILVALUE,RESULT) \
  do { \
    RESULT = (TYPE) X; \
    if (RESULT == FAILVALUE) \
      PDL_OS::set_errno_to_last_error (); \
  } while (0)
# else
#   define PDL_OSCALL_RETURN(OP,TYPE,FAILVALUE) do { TYPE pdl_result_ = FAILVALUE; pdl_result_ = pdl_result_; return OP; } while (0)
#   define PDL_OSCALL(OP,TYPE,FAILVALUE,RESULT) do { RESULT = (TYPE) OP; } while (0)
# endif /* PDL_HAS_SIGNAL_SAFE_OS_CALLS */

# if defined (PDL_HAS_THR_C_FUNC)
// This is necessary to work around nasty problems with MVS C++.
extern "C" PDL_Export void pdl_mutex_lock_cleanup_adapter (void *args);
#   define PDL_PTHREAD_CLEANUP_PUSH(A) pthread_cleanup_push (pdl_mutex_lock_cleanup_adapter, (void *) A);
#   define PDL_PTHREAD_CLEANUP_POP(A) pthread_cleanup_pop(A)
# elif defined (PDL_HAS_PTHREADS) && !defined (PDL_LACKS_PTHREAD_CLEANUP)
#   define PDL_PTHREAD_CLEANUP_PUSH(A) pthread_cleanup_push (PDL_OS::mutex_lock_cleanup, (void *) A);
#   define PDL_PTHREAD_CLEANUP_POP(A) pthread_cleanup_pop(A)
# else
#   define PDL_PTHREAD_CLEANUP_PUSH(A)
#   define PDL_PTHREAD_CLEANUP_POP(A)
# endif /* PDL_HAS_THR_C_FUNC */

# if !defined (PDL_DEFAULT_MUTEX_A)
#   define PDL_DEFAULT_MUTEX_A "PDL_MUTEX"
# endif /* PDL_DEFAULT_MUTEX_A */

# if !defined (PDL_MAIN)
#   define PDL_MAIN main
# endif /* ! PDL_MAIN */

# if defined (PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER)
#   if !defined (PDL_HAS_NONSTATIC_OBJECT_MANAGER)
#     define PDL_HAS_NONSTATIC_OBJECT_MANAGER
#   endif /* PDL_HAS_NONSTATIC_OBJECT_MANAGER */
# endif /* PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER */

# if defined (PDL_HAS_NONSTATIC_OBJECT_MANAGER) && !defined (PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER)

#   if !defined (PDL_HAS_MINIMAL_PDL_OS)
#     include "pdl/Object_Manager.h"
#   endif /* ! PDL_HAS_MINIMAL_PDL_OS */

// Rename "main ()" on platforms that don't allow it to be called "main ()".

// Also, create PDL_Object_Manager static instance(s) in "main ()".
// PDL_MAIN_OBJECT_MANAGER defines the PDL_Object_Manager(s) that will
// be instantiated on the stack of main ().  Note that it is only used
// when compiling main ():  its value does not affect the contents of
// pdl/OS.o.
#   if !defined (PDL_MAIN_OBJECT_MANAGER)
#     define PDL_MAIN_OBJECT_MANAGER \
        PDL_OS_Object_Manager pdl_os_object_manager; \
        PDL_Object_Manager pdl_object_manager;
#   endif /* ! PDL_MAIN_OBJECT_MANAGER */

#   if defined (PDL_PSOSIM)
// PSOSIM root lacks the standard argc, argv command line parameters,
// create dummy argc and argv in the "real" main  and pass to "user" main.
// NOTE: PDL_MAIN must be defined to give the return type as well as the
// name of the entry point.
#     define main \
pdl_main_i (int, char *[]);                /* forward declaration */ \
PDL_MAIN ()   /* user's entry point, e.g., "main" w/out argc, argv */ \
{ \
  int argc = 1;                            /* dummy arg count */ \
  char *argv[] = {"psosim"};               /* dummy arg list */ \
  PDL_MAIN_OBJECT_MANAGER \
  int ret_val = -1; /* assume the worst */ \
  if (PDL_PSOS_Time_t::init_simulator_time ()) /* init simulator time */ \
  { \
    PDL_ERROR((LM_ERROR, "init_simulator_time failed\n"));  /* report */ \
  } \
  else \
  { \
    ret_val = pdl_main_i (argc, argv);   /* call user main, save result */ \
  } \
  PDL_OS::exit (ret_val);                /* pass code to simulator exit */ \
} \
int \
pdl_main_i
#   elif defined (PDL_PSOS) && defined (PDL_PSOS_LACKS_ARGC_ARGV)
// PSOS root lacks the standard argc, argv command line parameters,
// create dummy argc and argv in the "real" main  and pass to "user" main.
// Ignore return value from user main as well.  NOTE: PDL_MAIN must be
// defined to give the return type as well as the name of the entry point
#     define main \
pdl_main_i (int, char *[]);               /* forward declaration */ \
PDL_MAIN ()   /* user's entry point, e.g., "main" w/out argc, argv */ \
{ \
  int argc = 1;                           /* dummy arg count */ \
  char *argv[] = {"root"};                /* dummy arg list */ \
  PDL_MAIN_OBJECT_MANAGER \
  pdl_main_i (argc, argv);                /* call user main, ignore result */ \
} \
int \
pdl_main_i
#   else
#     define main \
pdl_main_i (int, ASYS_TCHAR *[]);                  /* forward declaration */ \
int \
PDL_MAIN (int argc, ASYS_TCHAR *argv[]) /* user's entry point, e.g., main */ \
{ \
  PDL_MAIN_OBJECT_MANAGER \
  return pdl_main_i (argc, argv);           /* what the user calls "main" */ \
} \
int \
pdl_main_i
#     if defined (PDL_WIN32) && defined (UNICODE)
#     define wmain \
pdl_main_i (int, ASYS_TCHAR *[]);                  /* forward declaration */ \
int \
wmain (int argc, ASYS_TCHAR *argv[]) /* user's entry point, e.g., main */ \
{ \
  PDL_MAIN_OBJECT_MANAGER \
  return pdl_main_i (argc, argv);           /* what the user calls "main" */ \
} \
int \
pdl_main_i
#     endif /* PDL_WIN32 && UNICODE */
#   endif   /* PDL_PSOSIM */
# endif /* PDL_HAS_NONSTATIC_OBJECT_MANAGER && !PDL_HAS_WINCE && !PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER */

# if defined (PDL_HAS_WINCE)
#   if defined (PDL_HAS_WINCE_BROKEN_ERRNO)
class PDL_Export PDL_CE_Errno
{
  // Some versions of CE don't support <errno> and some versions'
  // implementations are busted.  So we implement our own.
  // Our implementation takes up one Tls key, however, it does not
  // allocate memory fromt the heap so there's no problem with cleanin
  // up the errno when a thread exit.
public:
  PDL_CE_Errno () {}
  static void init ();
  static void fini ();
  static PDL_CE_Errno *instance ();

  operator int (void) const;
  int operator= (int);

private:
  static PDL_CE_Errno *instance_;
  static DWORD errno_key_;
};

#     define errno (* (PDL_CE_Errno::instance ()))
#   endif /* PDL_HAS_WINCE_BROKEN_ERRNO */

class PDL_Export PDL_CE_Bridge
{
  // = TITLE
  //     This class bridges between PDL's default text output windows
  //     and the original PDL program.
  //
  // = DESCRIPTION
  //     As there is no such thing as text-based programs on Windows
  //     CE.  We need to create a windows to read the command prompt
  //     and bridge the output windows with the original PDL program
  //     entry point.  You'll need to link your program with
  //     "pdl-windows.lib" for this to work.  You can refer to
  //     $PDL_ROOT/WindowsCE/Main for how I use a dialog box to run
  //     the original PDL programs.  This is certainly not the only
  //     way to use PDL in Windows programs.
public:
  PDL_CE_Bridge (void);
  // Default ctor.

  PDL_CE_Bridge (HWND, int notification, int idc);
  // Construct and set the default windows.

  ~PDL_CE_Bridge (void);
  // Default dtor.

  void set_window (HWND, int notification, int idc);
  // Specify which window to use.

  void set_self_default (void);
  // Set the default window.

  int notification (void);
  int idc (void);
  HWND window (void);
  // Access functions.

  static PDL_CE_Bridge *get_default_winbridge (void);
  // Get the reference of default PDL_CE_BRIDGE.

  int write_msg (LPCTSTR str);
  // Write a string to windows.

#if 0
  int write_msg (CString *cs);
  // Write a CString to windows.  Notice that the CString object will
  // be freed by windows.
#endif /* 0 */
private:
  // @@ We should use a allocator here.

  HWND text_output_;
  // A pointer to the window that knows how to
  // handle PDL related messages.

  int notification_;
  // Notification of the window that receives WM_COMMAND when
  // outputing strings.

  int idc_;
  // IDC code of the window that receives WM_COMMAND when
  // outputing strings.

  ASYS_TCHAR *cmdline_;

  static PDL_CE_Bridge *default_text_bridge_;
  // A pointer to the default PDL_CE_BRIDGE obj.
};

# endif /* PDL_HAS_WINCE */

#if defined (PDL_HAS_WINCE_BROKEN_ERRNO)
#  define PDL_ERRNO_TYPE PDL_CE_Errno
#else
#  define PDL_ERRNO_TYPE int
#endif /* PDL_HAS_WINCE */

class PDL_Export PDL_Errno_Guard
{
  // = TITLE
  //   Provides a wrapper to improve performance when thread-specific
  //   errno must be saved and restored in a block of code.
  //
  // = DESCRIPTION
  //   The typical use-case for this is the following:
  //
  //   int error = errno;
  //   call_some_function_that_might_change_errno ();
  //   errno = error;
  //
  //   This can be replaced with
  //
  //   {
  //     PDL_Errno_Guard guard (errno);
  //     call_some_function_that_might_change_errno ();
  //   }
  //
  //   This implementation is more elegant and more efficient since it
  //   avoids an unnecessary second access to thread-specific storage
  //   by caching a pointer to the value of errno in TSS.
public:
  // = Initialization and termination methods.
  PDL_Errno_Guard (PDL_ERRNO_TYPE &errno_ref,
                   int error);
  //  Stash the value of <error> into <error_> and initialize the
  //  <errno_ptr_> to the address of <errno_ref>.

  PDL_Errno_Guard (PDL_ERRNO_TYPE &errno_ref);
  //  Stash the value of <errno> into <error_> and initialize the
  //  <errno_ptr_> to the address of <errno_ref>.

  ~PDL_Errno_Guard (void);
  // Reset the value of <errno> to <error>.

#if defined (PDL_HAS_WINCE_BROKEN_ERRNO)
  int operator= (const PDL_ERRNO_TYPE &errno_ref);
  // Assign <errno_ref> to <error_>.
#endif /* PDL_HAS_WINCE_BROKEN_ERRNO */

  int operator= (int error);
  // Assign <error> to <error_>.

  int operator== (int error);
  // Compare <error> with <error_> for equality.

  int operator!= (int error);
  // Compare <error> with <error_> for inequality.

private:
#if defined (PDL_MT_SAFE)
  PDL_ERRNO_TYPE *errno_ptr_;
#endif /* PDL_MT_SAFE */
  int error_;
};

// This is used to indicate that a platform doesn't support a
// particular feature.
# if defined PDL_HAS_VERBOSE_NOTSUP
  // Print a console message with the file and line number of the
  // unsupported function.
#   define PDL_NOTSUP_RETURN(FAILVALUE) do { errno = ENOTSUP; PDL_OS::fprintf (stderr, PDL_TEXT ("PDL_NOTSUP: %s, line %d\n"), __FILE__, __LINE__); return FAILVALUE; } while (0)
#   define PDL_NOTSUP do { errno = ENOTSUP; PDL_OS::fprintf (stderr, PDL_TEXT ("PDL_NOTSUP: %s, line %d\n"), __FILE__, __LINE__); return; } while (0)
# else /* ! PDL_HAS_VERBOSE_NOTSUP */
#   define PDL_NOTSUP_RETURN(FAILVALUE) do { errno = ENOTSUP ; return FAILVALUE; } while (0)
#   define PDL_NOTSUP do { errno = ENOTSUP; return; } while (0)
# endif /* ! PDL_HAS_VERBOSE_NOTSUP */

# if defined (PDL_HAS_GNUC_BROKEN_TEMPLATE_INLINE_FUNCTIONS)
#   define PDL_INLINE_FOR_GNUC PDL_INLINE
# else
#   define PDL_INLINE_FOR_GNUC
# endif /* PDL_HAS_GNUC_BROKEN_TEMPLATE_INLINE_FUNCTIONS */

# if defined (PDL_WIN32) && ! defined (PDL_HAS_WINCE) \
                         && ! defined (PDL_HAS_PHARLAP)
typedef TRANSMIT_FILE_BUFFERS PDL_TRANSMIT_FILE_BUFFERS;
typedef LPTRANSMIT_FILE_BUFFERS PDL_LPTRANSMIT_FILE_BUFFERS;
typedef PTRANSMIT_FILE_BUFFERS PDL_PTRANSMIT_FILE_BUFFERS;

#   define PDL_INFINITE INFINITE
#   define PDL_STATUS_TIMEOUT STATUS_TIMEOUT
#   define PDL_WAIT_FAILED WAIT_FAILED
#   define PDL_WAIT_TIMEOUT WAIT_TIMEOUT
# else /* PDL_WIN32 */
struct PDL_TRANSMIT_FILE_BUFFERS
{
  void *Head;
  size_t HeadLength;
  void *Tail;
  size_t TailLength;
};
typedef PDL_TRANSMIT_FILE_BUFFERS* PDL_PTRANSMIT_FILE_BUFFERS;
typedef PDL_TRANSMIT_FILE_BUFFERS* PDL_LPTRANSMIT_FILE_BUFFERS;

#   if !defined (PDL_INFINITE)
#     define PDL_INFINITE LONG_MAX
#   endif /* PDL_INFINITE */
#   define PDL_STATUS_TIMEOUT LONG_MAX
#   define PDL_WAIT_FAILED LONG_MAX
#   define PDL_WAIT_TIMEOUT LONG_MAX
# endif /* PDL_WIN32 */

# if defined (UNICODE)

#   if !defined (PDL_DEFAULT_NAMESPACE_DIR)
#     define PDL_DEFAULT_NAMESPACE_DIR PDL_DEFAULT_NAMESPACE_DIR_W
#   endif /* PDL_DEFAULT_NAMESPACE_DIR */
#   if !defined (PDL_DEFAULT_LOCALNAME)
#     define PDL_DEFAULT_LOCALNAME PDL_DEFAULT_LOCALNAME_W
#   endif /* PDL_DEFAULT_LOCALNAME */
#   if !defined (PDL_DEFAULT_GLOBALNAME)
#     define PDL_DEFAULT_GLOBALNAME PDL_DEFAULT_GLOBALNAME_W
#   endif /* PDL_DEFAULT_GLOBALNAME */
#   if !defined (PDL_DIRECTORY_SEPARATOR_STR)
#     define PDL_DIRECTORY_SEPARATOR_STR PDL_DIRECTORY_SEPARATOR_STR_W
#   endif /* PDL_DIRECTORY_SEPARATOR_STR */
#   if !defined (PDL_DIRECTORY_SEPARATOR_CHAR)
#     define PDL_DIRECTORY_SEPARATOR_CHAR PDL_DIRECTORY_SEPARATOR_CHAR_W
#   endif /* PDL_DIRECTORY_SEPARATOR_CHAR */
#   if !defined (PDL_PLATFORM)
#     define PDL_PLATFORM PDL_PLATFORM_W
#   endif /* PDL_PLATFORM */
#   if !defined (PDL_PLATFORM_EXE_SUFFIX)
#     define PDL_PLATFORM_EXE_SUFFIX PDL_PLATFORM_EXE_SUFFIX_W
#   endif /* PDL_PLATFORM_EXE_SUFFIX */
#   if !defined (PDL_DEFAULT_MUTEX_W)
#     define PDL_DEFAULT_MUTEX_W L"PDL_MUTEX"
#   endif /* PDL_DEFAULT_MUTEX_W */
#   if !defined (PDL_DEFAULT_MUTEX)
#     define PDL_DEFAULT_MUTEX PDL_DEFAULT_MUTEX_W
#   endif /* PDL_DEFAULT_MUTEX */
#   if !defined (PDL_DEFAULT_TEMP_DIR_ENV)
#     define PDL_DEFAULT_TEMP_DIR_ENV PDL_DEFAULT_TEMP_DIR_ENV_W
#   endif /* PDL_DEFAULT_TEMP_DIR_ENV */

# else

#   if !defined (PDL_DEFAULT_NAMESPACE_DIR)
#     define PDL_DEFAULT_NAMESPACE_DIR PDL_DEFAULT_NAMESPACE_DIR_A
#   endif /* PDL_DEFAULT_NAMESPACE_DIR */
#   if !defined (PDL_DEFAULT_LOCALNAME)
#     define PDL_DEFAULT_LOCALNAME PDL_DEFAULT_LOCALNAME_A
#   endif /* PDL_DEFAULT_LOCALNAME */
#   if !defined (PDL_DEFAULT_GLOBALNAME)
#     define PDL_DEFAULT_GLOBALNAME PDL_DEFAULT_GLOBALNAME_A
#   endif /* PDL_DEFAULT_GLOBALNAME */
#   if !defined (PDL_DIRECTORY_SEPARATOR_STR)
#     define PDL_DIRECTORY_SEPARATOR_STR PDL_DIRECTORY_SEPARATOR_STR_A
#   endif /* PDL_DIRECTORY_SEPARATOR_STR */
#   if !defined (PDL_DIRECTORY_SEPARATOR_CHAR)
#     define PDL_DIRECTORY_SEPARATOR_CHAR PDL_DIRECTORY_SEPARATOR_CHAR_A
#   endif /* PDL_DIRECTORY_SEPARATOR_CHAR */
#   if !defined (PDL_PLATFORM)
#     define PDL_PLATFORM PDL_PLATFORM_A
#   endif /* PDL_PLATFORM */
#   if !defined (PDL_PLATFORM_EXE_SUFFIX)
#     define PDL_PLATFORM_EXE_SUFFIX PDL_PLATFORM_EXE_SUFFIX_A
#   endif /* PDL_PLATFORM_EXE_SUFFIX */
#   if !defined (PDL_DEFAULT_MUTEX_W)
#     define PDL_DEFAULT_MUTEX_W "PDL_MUTEX"
#   endif /* PDL_DEFAULT_MUTEX_W */
#   if !defined (PDL_DEFAULT_MUTEX)
#     define PDL_DEFAULT_MUTEX PDL_DEFAULT_MUTEX_A
#   endif /* PDL_DEFAULT_MUTEX */
#   if !defined (PDL_DEFAULT_TEMP_DIR_ENV)
#     define PDL_DEFAULT_TEMP_DIR_ENV PDL_DEFAULT_TEMP_DIR_ENV_A
#   endif /* PDL_DEFAULT_TEMP_DIR_ENV */
# endif /* UNICODE */

// Some PDL classes always use inline functions to maintain high
// performance, but some platforms have buggy inline function support.
// In this case, we don't use inline with them.
# if defined (PDL_LACKS_INLINE_FUNCTIONS)
#   if defined (ASYS_INLINE)
#     undef ASYS_INLINE
#   endif /* ASYS_INLINE */
#   define ASYS_INLINE
#   if defined (PDL_HAS_INLINED_OSCALLS)
#     undef PDL_HAS_INLINED_OSCALLS
#   endif /* PDL_HAS_INLINED_OSCALLS */
# else
#   define ASYS_INLINE inline
# endif /* PDL_LACKS_INLINE_FUNCTIONS */

# if !defined (PDL_HAS_MINIMAL_PDL_OS)
#   include "pdl/Trace.h"
# endif /* ! PDL_HAS_MINIMAL_PDL_OS */

// The following are some insane macros that are useful in cases when
// one has to have a string in a certain format.  Both of these macros
// allow the user to create a temporary copy. If the user needs to
// keep this temporary string around, they must do some sort of strdup
// on the temporary string.
//
// The PDL_WIDE_STRING macro allows the user to create a temporary
// representation of an ASCII string as a WIDE string.
//
// The PDL_MULTIBYTE_STRING macro allows the user to create a
// temporary representation of a WIDE string as an ASCII string. Note
// that some data may be lost in this conversion.

# if defined (UNICODE)
#   define PDL_WIDE_STRING(ASCII_STRING) \
PDL_OS_CString (ASCII_STRING).wchar_rep ()
#   define PDL_MULTIBYTE_STRING(WIDE_STRING) \
PDL_OS_WString (WIDE_STRING).char_rep ()
#   define PDL_TEXT_STRING PDL_WString
#   if defined (PDL_HAS_MOSTLY_UNICODE_APIS)
#     define ASYS_MULTIBYTE_STRING(WIDE_STRING) WIDE_STRING
#     define ASYS_ONLY_WIDE_STRING(WIDE_STRING) WIDE_STRING
#     define ASYS_ONLY_MULTIBYTE_STRING(WIDE_STRING) \
PDL_OS_WString (WIDE_STRING).char_rep ()
#   else
#     define ASYS_MULTIBYTE_STRING(WIDE_STRING) \
PDL_OS_WString (WIDE_STRING).char_rep ()
#     define ASYS_ONLY_WIDE_STRING(ASCII_STRING) \
PDL_OS_CString (ASCII_STRING).wchar_rep ()
#     define ASYS_ONLY_MULTIBYTE_STRING(WIDE_STRING) WIDE_STRING
#   endif /* PDL_HAS_MOSTLY_UNICODE_APIS */
# else
#   define PDL_WIDE_STRING(ASCII_STRING) ASCII_STRING
#   define PDL_MULTIBYTE_STRING(WIDE_STRING) WIDE_STRING
#   define PDL_TEXT_STRING PDL_CString
#   define ASYS_MULTIBYTE_STRING(WIDE_STRING) WIDE_STRING
#   define ASYS_ONLY_WIDE_STRING(WIDE_STRING) WIDE_STRING
#   define ASYS_ONLY_MULTIBYTE_STRING(WIDE_STRING) WIDE_STRING
# endif /* UNICODE */

# if defined (PDL_HAS_MOSTLY_UNICODE_APIS)
#   define ASYS_WIDE_STRING(ASCII_STRING) PDL_OS_CString (ASCII_STRING).wchar_rep ()
# else
#   define ASYS_WIDE_STRING(ASCII_STRING) ASCII_STRING
# endif /* PDL_HAS_MOSTLY_UNICODE_APIS */

# if defined (PDL_HAS_INLINED_OSCALLS)
#   if defined (PDL_INLINE)
#     undef PDL_INLINE
#   endif /* PDL_INLINE */
#   define PDL_INLINE inline
#   include "pdl/OS.i"
# endif /* PDL_HAS_INLINED_OSCALLS */

# if !defined (PDL_HAS_MINIMAL_PDL_OS)
    // This needs to come here to avoid problems with circular dependencies.
#   include "pdl/Log_Msg.h"
# endif /* ! PDL_HAS_MINIMAL_PDL_OS */

// Byte swapping macros to deal with differences between little endian
// and big endian machines.  Note that "long" here refers to 32 bit
// quantities.
# define PDL_SWAP_LONG(L) ((PDL_SWAP_WORD ((L) & 0xFFFF) << 16) \
            | PDL_SWAP_WORD(((L) >> 16) & 0xFFFF))
# define PDL_SWAP_WORD(L) ((((L) & 0x00FF) << 8) | (((L) & 0xFF00) >> 8))

# if defined (PDL_LITTLE_ENDIAN)
#   define PDL_HTONL(X) PDL_SWAP_LONG (X)
#   define PDL_NTOHL(X) PDL_SWAP_LONG (X)
#   define PDL_IDL_NCTOHL(X) (X)
#   define PDL_IDL_NSTOHL(X) (X)
# else
#   define PDL_HTONL(X) X
#   define PDL_NTOHL(X) X
#   define PDL_IDL_NCTOHL(X) (X << 24)
#   define PDL_IDL_NSTOHL(X) ((X) << 16)
# endif /* PDL_LITTLE_ENDIAN */

# if defined (PDL_LITTLE_ENDIAN)
#   define PDL_HTONS(x) PDL_SWAP_WORD(x)
#   define PDL_NTOHS(x) PDL_SWAP_WORD(x)
# else
#   define PDL_HTONS(x) x
#   define PDL_NTOHS(x) x
# endif /* PDL_LITTLE_ENDIAN */

# if defined (PDL_HAS_AIO_CALLS)
  // = Giving unique PDL scoped names for some important
  // RTSignal-Related constants. Becuase sometimes, different
  // platforms use different names for these constants.

  // Number of realtime signals provided in the system.
  // _POSIX_RTSIG_MAX is the upper limit on the number of real time
  // signals supported in a posix-4 compliant system.
#   if defined (_POSIX_RTSIG_MAX)
#     define PDL_RTSIG_MAX _POSIX_RTSIG_MAX
#   else /* not _POSIX_RTSIG_MAX */
  // POSIX-4 compilant system has to provide atleast 8 RT signals.
  // @@ Make sure the platform does *not* define this constant with
  // some other name. If yes, use that instead of 8.
#     define PDL_RTSIG_MAX 8
#   endif /* _POSIX_RTSIG_MAX */
# endif /* PDL_HAS_AIO_CALLS */

  // Wrapping around wait status <wstat> macros for platforms that
  // lack them.

  // Evaluates to a non-zero value if status was returned for a child
  // process that terminated normally.  0 means status wasn't
  // returned.
#if !defined (WIFEXITED)
#   define WIFEXITED(stat) 1
#endif /* WIFEXITED */

  // If the value of WIFEXITED(stat) is non-zero, this macro evaluates
  // to the exit code that the child process exit(3C), or the value
  // that the child process returned from main.  Pepdlful exit code is
  // 0.
#if !defined (WEXITSTATUS)
#   define WEXITSTATUS(stat) stat
#endif /* WEXITSTATUS */

  // Evaluates to a non-zero value if status was returned for a child
  // process that terminated due to the receipt of a signal.  0 means
  // status wasnt returned.
#if !defined (WIFSIGNALED)
#   define WIFSIGNALED(stat) 0
#endif /* WIFSIGNALED */

  // If the value of  WIFSIGNALED(stat)  is non-zero,  this macro
  // evaluates to the number of the signal that  caused  the
  // termination of the child process.
#if !defined (WTERMSIG)
#   define WTERMSIG(stat) 0
#endif /* WTERMSIG */

#if !defined (WIFSTOPPED)
#   define WIFSTOPPED(stat) 0
#endif /* WIFSTOPPED */

#if !defined (WSTOPSIG)
#   define WSTOPSIG(stat) 0
#endif /* WSTOPSIG */

#if !defined (WIFCONTINUED)
#   define WIFCONTINUED(stat) 0
#endif /* WIFCONTINUED */

#if !defined (WCOREDUMP)
#   define WCOREDUMP(stat) 0
#endif /* WCOREDUMP */

// Stuff used by the PDL CDR classes.
#if defined PDL_LITTLE_ENDIAN
#  define PDL_CDR_BYTE_ORDER 1
// little endian encapsulation byte order has value = 1
#else  /* ! PDL_LITTLE_ENDIAN */
#  define PDL_CDR_BYTE_ORDER 0
// big endian encapsulation byte order has value = 0
#endif /* ! PDL_LITTLE_ENDIAN */

// Default constants for PDL CDR performance optimizations.
#define PDL_DEFAULT_CDR_BUFSIZE 512
#define PDL_DEFAULT_CDR_EXP_GROWTH_MAX 4096
#define PDL_DEFAULT_CDR_LINEAR_GROWTH_CHUNK 4096
// Even though the strategy above minimizes copies in some cases it is
// more efficient just to copy the octet sequence, for instance, while
// enconding a "idall" octet sequence in a buffer that has enough
// space. This parameter controls the default value for "idall enough",
// but an ORB may use a different value, set from a command line option.
#define PDL_DEFAULT_CDR_MEMCPY_TRADEOFF 256

// In some environments it is useful to swap the bytes on write, for
// instance: a fast server can be feeding a lot of slow clients that
// happen to have the wrong byte order.
// This macro enables the functionality to support that, but we still
// need a way to activate it on a per-connection basis.
//
// #define PDL_ENABLE_SWAP_ON_WRITE

// In some environements we never need to swap bytes when reading, for
// instance embebbed systems (such as avionics) or homogenous
// networks.
// Setting this macro disables the capabilities to demarshall streams
// in the wrong byte order.
//
// #define PDL_DISABLE_SWAP_ON_READ

// For some applications it is important to optimize octet sequences
// and minimize the number of copies made of the sequence buffer.
// TAO supports this optimizations by sharing the CDR stream buffer
// and the octet sequences buffer via PDL_Message_Block's.
// This feature can be disabled for: debugging, performance
// comparisons, complaince checking (the octet sequences add an API to
// access the underlying message block).


// Typedefs and macros for efficient PDL CDR memory address
// boundary alignment

// Efficiently align "value" up to "alignment", knowing that all such
// boundaries are binary powers and that we're using two's complement
// arithmetic.

// Since the alignment is a power of two its binary representation is:
//    alignment      = 0...010...0
//
// hence
//
//    alignment - 1  = 0...001...1 = T1
//
// so the complement is:
//
//  ~(alignment - 1) = 1...110...0 = T2
//
// Notice that there is a multiple of <alignment> in the range
// [<value>,<value> + T1], also notice that if
//
// X = ( <value> + T1 ) & T2
//
// then
//
// <value> <= X <= <value> + T1
//
// because the & operator only changes the last bits, and since X is a
// multiple of <alignment> (its last bits are zero) we have found the
// multiple we wanted.
//

#define align_binary(ptr, alignment) \
    ((ptr + ((ptr_arith_t)((alignment)-1))) & (~((ptr_arith_t)((alignment)-1))))

// Efficiently round "ptr" up to an "alignment" boundary, knowing that
// all such boundaries are binary powers and that we're using two's
// complement arithmetic.
//
#define ptr_align_binary(ptr, alignment) \
        ((char *) align_binary (((ptr_arith_t) (ptr)), (alignment)))

// Defining POSIX4 real-time signal range.
#if defined PDL_HAS_AIO_CALLS
#define PDL_SIGRTMIN SIGRTMIN
#define PDL_SIGRTMAX SIGRTMAX
#else /* !PDL_HAS_AIO_CALLS */
#define PDL_SIGRTMIN 0
#define PDL_SIGRTMAX 0
#endif /* PDL_HAS_AIO_CALLS */

#endif  /* PDL_OS_H */
