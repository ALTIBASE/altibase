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

//==========================================================================
/**
 *  @file   config-all.h
 *
 *  config-all.h,v 4.49 2002/05/17 15:05:06 schmidt Exp
 *
 *  @author (Originally in OS.h)Doug Schmidt <schmidt@cs.wustl.edu>
 *  @author Jesper S. M|ller<stophph@diku.dk>
 *  @author and a cast of thousands...
 */
//==========================================================================

#ifndef PDL_CONFIG_ALL_H
#define PDL_CONFIG_ALL_H

#include "config.h"

#if !defined (PDL_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* PDL_LACKS_PRAGMA_ONCE */

// =========================================================================
// Enable/Disable Features By Default
// =========================================================================

# if !defined (PDL_HAS_POSITION_INDEPENDENT_POINTERS)
#   define PDL_HAS_POSITION_INDEPENDENT_POINTERS 1
# endif /* PDL_HAS_POSITION_INDEPENDENT_POINTERS */

// =========================================================================
// RCSID Macros
// =========================================================================

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
      static inline const char* get_rcsid_ ## path ## _ ## file (const char*) \
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

// =========================================================================
// INLINE macros
//
// These macros handle all the inlining of code via the .i or .inl files
// =========================================================================

#if defined (PDL_LACKS_INLINE_FUNCTIONS) && !defined (PDL_NO_INLINE)
#  define PDL_NO_INLINE
#endif /* defined (PDL_LACKS_INLINE_FUNCTIONS) && !defined (PDL_NO_INLINE) */

// PDL inlining has been explicitly disabled.  Implement
// internally within PDL by undefining __PDL_INLINE__.
#if defined (PDL_NO_INLINE)
#  undef __PDL_INLINE__
#endif /* ! PDL_NO_INLINE */

#if defined (__PDL_INLINE__)
#  define PDL_INLINE inline
#  if !defined (PDL_HAS_INLINED_OSCALLS)
#    define PDL_HAS_INLINED_OSCALLS
#  endif /* !PDL_HAS_INLINED_OSCALLS */
#else
#  define PDL_INLINE
#endif /* __PDL_INLINE__ */

# if !defined (PDL_HAS_GNUC_BROKEN_TEMPLATE_INLINE_FUNCTIONS)
#   define PDL_INLINE_FOR_GNUC PDL_INLINE
# else
#   define PDL_INLINE_FOR_GNUC
# endif /* PDL_HAS_GNUC_BROKEN_TEMPLATE_INLINE_FUNCTIONS */

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

// =========================================================================
// EXPLICIT macro
// =========================================================================

# if defined (PDL_HAS_EXPLICIT_KEYWORD)
#   define PDL_EXPLICIT explicit
# else  /* ! PDL_HAS_EXPLICIT_KEYWORD */
#   define PDL_EXPLICIT
# endif /* ! PDL_HAS_EXPLICIT_KEYWORD */

// =========================================================================
// MUTABLE macro
// =========================================================================

# if defined (PDL_HAS_MUTABLE_KEYWORD)
#   define PDL_MUTABLE mutable
#   define PDL_CONST_WHEN_MUTABLE const // Addition #1
# else  /* ! PDL_HAS_MUTABLE_KEYWORD */
#   define PDL_MUTABLE
#   define PDL_CONST_WHEN_MUTABLE       // Addition #2
# endif /* ! PDL_HAS_MUTABLE_KEYWORD */

// ============================================================================
// EXPORT macros
//
// Since Win32 DLL's do not export all symbols by default, they must be
// explicitly exported (which is done by *_Export macros).
// ============================================================================

// Win32 should have already defined the macros in config-win32-common.h
#if !defined (PDL_HAS_CUSTOM_EXPORT_MACROS)
#  define PDL_Proper_Export_Flag
#  define PDL_Proper_Import_Flag
#  define PDL_EXPORT_SINGLETON_DECLARATION(T)
#  define PDL_IMPORT_SINGLETON_DECLARATION(T)
#  define PDL_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  define PDL_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* PDL_HAS_CUSTOM_EXPORT_MACROS */

// This is a whim of mine -- that instead of annotating a class with
// PDL_Export in its declaration, we make the declaration near the TOP
// of the file with PDL_DECLARE_EXPORT.
// TS = type specifier (e.g., class, struct, int, etc.)
// ID = identifier
// So, how do you use it?  Most of the time, just use ...
// PDL_DECLARE_EXPORT(class, someobject);
// If there are global functions to be exported, then use ...
// PDL_DECLARE_EXPORT(void, globalfunction) (int, ...);
// Someday, when template libraries are supported, we made need ...
// PDL_DECLARE_EXPORT(template class, sometemplate) <class TYPE, class LOCK>;
# define PDL_DECLARE_EXPORT(TS,ID) TS PDL_Export ID

// ============================================================================
// Cast macros
//
// These macros are used to choose between the old cast style and the new
// *_cast<> operators
// ============================================================================

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

// ============================================================================
// UNICODE macros (to be added later)
// ============================================================================

// ============================================================================
// Compiler Silencing macros
//
// Some compilers complain about parameters that are not used.  This macro
// should keep them quiet.
// ============================================================================

#if defined (ghs) || defined (__GNUC__) || defined (__hpux) || defined (__sgi) || defined (__DECCXX) || defined (__KCC) || defined (__rational__) || defined (__USLC__) || defined (PDL_RM544)
// Some compilers complain about "statement with no effect" with (a).
// This eliminates the warnings, and no code is generated for the null
// conditional statement.  NOTE: that may only be true if -O is enabled,
// such as with GreenHills (ghs) 1.8.8.
# define PDL_UNUSED_ARG(a) do {/* null */} while (&a == 0)
#else /* ghs || __GNUC__ || ..... */
# define PDL_UNUSED_ARG(a) (a)
#endif /* ghs || __GNUC__ || ..... */

#if defined (__sgi) || defined (ghs) || defined (__DECCXX) || defined(__BORLANDC__) || defined (__KCC) || defined (PDL_RM544) || defined (__USLC__)
# define PDL_NOTREACHED(a)
#else  /* __sgi || ghs || ..... */
# define PDL_NOTREACHED(a) a
#endif /* __sgi || ghs || ..... */

// ============================================================================
// PDL_NEW macros
//
// A useful abstraction for expressions involving operator new since
// we can change memory allocation error handling policies (e.g.,
// depending on whether ANSI/ISO exception handling semantics are
// being used).
// ============================================================================

#if defined (PDL_NEW_THROWS_EXCEPTIONS)

// Since new() throws exceptions, we need a way to avoid passing
// exceptions past the call to new because PDL counts on having a 0
// return value for a failed allocation. Some compilers offer the
// new (nothrow) version, which does exactly what we want. Others
// do not. For those that do not, this sets up what exception is thrown,
// and then below we'll do a try/catch around the new to catch it and
// return a 0 pointer instead.

#  if defined (__HP_aCC)
      // I know this works for HP aC++... if <stdexcept> is used, it
      // introduces other stuff that breaks things, like <memory>, which
      // screws up auto_ptr.
#    include /**/ <new>
    // _HP_aCC was first defined at aC++ 03.13 on HP-UX 11. Prior to that
    // (03.10 and before) a failed new threw bad_alloc. After that (03.13
    // and above) the exception thrown is dependent on the below settings.
#    if (HPUX_VERS >= 1100)
#      if (((__HP_aCC <  32500 && !defined (RWSTD_NO_NAMESPACE)) || \
            (__HP_aCC >= 32500 && defined (_HP_NAMESPACE_STD))) \
           || defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB))
#        define PDL_bad_alloc std::bad_alloc
#        define PDL_nothrow   std::nothrow
#        define PDL_nothrow_t std::nothrow_t
#      else
#        define PDL_bad_alloc bad_alloc
#        define PDL_nothrow   nothrow
#        define PDL_nothrow_t nothrow_t
#      endif /* __HP_aCC */
#    elif ((__HP_aCC <  12500 && !defined (RWSTD_NO_NAMESPACE)) || \
           (__HP_aCC >= 12500 && defined (_HP_NAMESPACE_STD)))
#      define PDL_bad_alloc std::bad_alloc
#      define PDL_nothrow   std::nothrow
#      define PDL_nothrow_t std::nothrow_t
#    else
#      define PDL_bad_alloc bad_alloc
#      define PDL_nothrow   nothrow
#      define PDL_nothrow_t nothrow_t
#    endif /* HPUX_VERS < 1100 */
#    define PDL_throw_bad_alloc throw PDL_bad_alloc ()
#  elif defined (__SUNPRO_CC)
#      if (__SUNPRO_CC < 0x500) || (__SUNPRO_CC_COMPAT == 4)
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
#  elif defined (__BORLANDC__) || defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
#    include /**/ <new>
#    define PDL_bad_alloc std::bad_alloc
#    define PDL_throw_bad_alloc throw PDL_bad_alloc ()
#  else
#    include /**/ <new>
#    define PDL_bad_alloc bad_alloc
#    define PDL_throw_bad_alloc throw PDL_bad_alloc ()
#  endif /* __HP_aCC */

#  if defined (PDL_HAS_NEW_NOTHROW)
#    define PDL_NEW_RETURN(POINTER,CONSTRUCTOR,RET_VAL) \
   do { POINTER = new (PDL_nothrow) CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; return RET_VAL; } \
   } while (0)
#    define PDL_NEW(POINTER,CONSTRUCTOR) \
   do { POINTER = new(PDL_nothrow) CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; return; } \
   } while (0)
#    define PDL_NEW_NORETURN(POINTER,CONSTRUCTOR) \
   do { POINTER = new(PDL_nothrow) CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; } \
   } while (0)

#  else

#    define PDL_NEW_RETURN(POINTER,CONSTRUCTOR,RET_VAL) \
   do { try { POINTER = new CONSTRUCTOR; } \
        catch (PDL_bad_alloc) { errno = ENOMEM; POINTER = 0; return RET_VAL; } \
   } while (0)

#    define PDL_NEW(POINTER,CONSTRUCTOR) \
   do { try { POINTER = new CONSTRUCTOR; } \
        catch (PDL_bad_alloc) { errno = ENOMEM; POINTER = 0; return; } \
   } while (0)

#    define PDL_NEW_NORETURN(POINTER,CONSTRUCTOR) \
   do { try { POINTER = new CONSTRUCTOR; } \
        catch (PDL_bad_alloc) { errno = ENOMEM; POINTER = 0; } \
   } while (0)
#  endif /* PDL_HAS_NEW_NOTHROW */

#else /* PDL_NEW_THROWS_EXCEPTIONS */

# define PDL_NEW_RETURN(POINTER,CONSTRUCTOR,RET_VAL) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; return RET_VAL; } \
   } while (0)
# define PDL_NEW(POINTER,CONSTRUCTOR) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; return; } \
   } while (0)
# define PDL_NEW_NORETURN(POINTER,CONSTRUCTOR) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { errno = ENOMEM; } \
   } while (0)

# define PDL_throw_bad_alloc \
  void* gcc_will_complain_if_literal_0_is_returned = 0; \
  return gcc_will_complain_if_literal_0_is_returned

#endif /* PDL_NEW_THROWS_EXCEPTIONS */

// ============================================================================
// PDL_ALLOC_HOOK* macros
//
// Macros to declare and define class-specific allocation operators.
// ============================================================================

# if defined (PDL_HAS_ALLOC_HOOKS)
#   define PDL_ALLOC_HOOK_DECLARE \
  void *operator new (size_t bytes); \
  void operator delete (void *ptr);

  // Note that these are just place holders for now.  Some day they
  // may be be replaced by <PDL_Malloc>.
#   define PDL_ALLOC_HOOK_DEFINE(CLASS) \
  void *CLASS::operator new (size_t bytes) { return ::new char[bytes]; } \
  void CLASS::operator delete (void *ptr) { delete [] ((char *) ptr); }
# else
#   define PDL_ALLOC_HOOK_DECLARE struct __Ace {} /* Just need a dummy... */
#   define PDL_ALLOC_HOOK_DEFINE(CLASS)
# endif /* PDL_HAS_ALLOC_HOOKS */

// ============================================================================
// PDL_OSCALL_* macros
//
// The following two macros ensure that system calls are properly
// restarted (if necessary) when interrupts occur.
// ============================================================================

#if defined (PDL_HAS_SIGNAL_SAFE_OS_CALLS)
#   define PDL_OSCALL(OP,TYPE,FAILVALUE,RESULT) \
  do \
    RESULT = (TYPE) OP; \
  while (RESULT == FAILVALUE && errno == EINTR)
#   define PDL_OSCALL_RETURN(OP,TYPE,FAILVALUE) \
  do { \
    TYPE pdl_result_; \
    do \
      pdl_result_ = (TYPE) OP; \
    while (pdl_result_ == FAILVALUE && errno == EINTR); \
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
#else /* PDL_HAS_SIGNAL_SAFE_OS_CALLS */
#   define PDL_OSCALL_RETURN(OP,TYPE,FAILVALUE) do { TYPE pdl_result_ = FAILVALUE; pdl_result_ = pdl_result_; return OP; } while (0)
#   define PDL_OSCALL(OP,TYPE,FAILVALUE,RESULT) do { RESULT = (TYPE) OP; } while (0)
#endif /* PDL_HAS_SIGNAL_SAFE_OS_CALLS */

// ============================================================================
// at_exit declarations
// ============================================================================

// Marker for cleanup, used by PDL_Exit_Info.
extern int pdl_exit_hook_marker;

// For use by <PDL_OS::exit>.
extern "C"
{
  typedef void (*PDL_EXIT_HOOK) (void);
}

// ============================================================================
// log_msg declarations
// ============================================================================

# if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
typedef int (*PDL_SEH_EXCEPT_HANDLER)(void *);
// Prototype of win32 structured exception handler functions.
// They are used to get the exception handling expression or
// as exception handlers.
# endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */

class PDL_OS_Thread_Descriptor;
class PDL_OS_Log_Msg_Attributes;
typedef void (*PDL_INIT_LOG_MSG_HOOK) (PDL_OS_Log_Msg_Attributes &attr
# if defined (PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
                                       , PDL_SEH_EXCEPT_HANDLER selector
                                       , PDL_SEH_EXCEPT_HANDLER handler
# endif /* PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS */
                                       );
typedef void (*PDL_INHERIT_LOG_MSG_HOOK) (PDL_OS_Thread_Descriptor*,
                                          PDL_OS_Log_Msg_Attributes &);

typedef void (*PDL_CLOSE_LOG_MSG_HOOK) (void);

typedef void (*PDL_SYNC_LOG_MSG_HOOK) (const PDL_TCHAR *prog_name);

typedef PDL_OS_Thread_Descriptor *(*PDL_THR_DESC_LOG_MSG_HOOK) (void);

// ============================================================================
// Fundamental types
// ============================================================================

#if defined (PDL_WIN32)

typedef HANDLE PDL_HANDLE;
typedef SOCKET PDL_SOCKET;
# define PDL_INVALID_HANDLE INVALID_HANDLE_VALUE

#else /* ! PDL_WIN32 */

typedef int PDL_HANDLE;
typedef PDL_HANDLE PDL_SOCKET;
# define PDL_INVALID_HANDLE -1

#endif /* PDL_WIN32 */

typedef void *(*PDL_THR_FUNC)(void *);

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

// ============================================================================
// PACE macros
// ============================================================================
#if defined (PDL_HAS_PACE) && !defined (PDL_WIN32)
# define PDL_HAS_POSIX_SEM
#endif /* PDL_HAS_PACE */

// ============================================================================
// PDL_USES_CLASSIC_SVC_CONF macro
// ============================================================================

// For now, default is to use the classic svc.conf format.
#if !defined (PDL_USES_CLASSIC_SVC_CONF)
# if defined (PDL_HAS_CLASSIC_SVC_CONF) && defined (PDL_HAS_XML_SVC_CONF)
#   error You can only use either CLASSIC or XML svc.conf, not both.
# endif
// Change the PDL_HAS_XML_SVC_CONF to PDL_HAS_CLASSIC_SVC_CONF when
// we switch PDL to use XML svc.conf as default format.
# if defined (PDL_HAS_XML_SVC_CONF)
#   define PDL_USES_CLASSIC_SVC_CONF 0
# else
#   define PDL_USES_CLASSIC_SVC_CONF 1
# endif /* PDL_HAS_XML_SVC_CONF */
#endif /* PDL_USES_CLASSIC_SVC_CONF */

// ============================================================================
// Default svc.conf file extension.
// ============================================================================
#if defined (PDL_USES_CLASSIC_SVC_CONF) && (PDL_USES_CLASSIC_SVC_CONF == 1)
# define PDL_DEFAULT_SVC_CONF_EXT   ".conf"
#else
# define PDL_DEFAULT_SVC_CONF_EXT   ".conf.xml"
#endif /* PDL_USES_CLASSIC_SVC_CONF && PDL_USES_CLASSIC_SVC_CONF == 1 */

// ============================================================================
// Miscellaneous macros
// ============================================================================

// This is used to indicate that a platform doesn't support a
// particular feature.
#if defined PDL_HAS_VERBOSE_NOTSUP
  // Print a console message with the file and line number of the
  // unsupported function.
# if defined (PDL_HAS_STANDARD_CPP_LIBRARY) && (PDL_HAS_STANDARD_CPP_LIBRARY != 0)
#   include /**/ <cstdio>
# else
#   include /**/ <stdarg.h> // LynxOS requires this before stdio.h
#   include /**/ <stdio.h>
# endif
# define PDL_NOTSUP_RETURN(FAILVALUE) do { errno = ENOTSUP; fprintf (stderr, PDL_LIB_TEXT ("PDL_NOTSUP: %s, line %d\n"), __FILE__, __LINE__); return FAILVALUE; } while (0)
# define PDL_NOTSUP do { errno = ENOTSUP; fprintf (stderr, PDL_LIB_TEXT ("PDL_NOTSUP: %s, line %d\n"), __FILE__, __LINE__); return; } while (0)
#else /* ! PDL_HAS_VERBOSE_NOTSUP */
# define PDL_NOTSUP_RETURN(FAILVALUE) do { errno = ENOTSUP ; return FAILVALUE; } while (0)
# define PDL_NOTSUP do { errno = ENOTSUP; return; } while (0)
#endif /* ! PDL_HAS_VERBOSE_NOTSUP */

#endif /* PDL_CONFIG_ALL_H */
