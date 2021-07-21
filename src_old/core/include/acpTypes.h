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
 
/*******************************************************************************
 * $Id: acpTypes.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_TYPES_H_)
#define _O_ACP_TYPES_H_

/**
 * @file
 * @ingroup CoreType
 */

#include <acpConfig.h>


#if defined(ACP_CFG_DOXYGEN)

/**
 * extern as @c "C" symbols
 */
#define ACP_EXTERN_C
/**
 * begins extern as @c "C" symbols
 */
#define ACP_EXTERN_C_BEGIN
/**
 * ends extern as @c "C" symbols
 */
#define ACP_EXTERN_C_END

#elif defined(__cplusplus)

#define ACP_EXTERN_C       extern "C"
#define ACP_EXTERN_C_BEGIN extern "C" {
#define ACP_EXTERN_C_END   }

#else

#define ACP_EXTERN_C       extern
#define ACP_EXTERN_C_BEGIN
#define ACP_EXTERN_C_END

#endif


ACP_EXTERN_C_BEGIN


/*
 * Character Type
 */
/**
 * 1-byte character type
 */
typedef char               acp_char_t;

/*
 * Integer Type
 */
/**
 * 1-byte signed integer type
 */
typedef signed char        acp_sint8_t;
/**
 * 1-byte unsigned integer type
 */
typedef unsigned char      acp_uint8_t;

/**
 * 2-byte signed integer type
 */
typedef signed short       acp_sint16_t;
/**
 * 2-byte unsigned integer type
 */
typedef unsigned short     acp_uint16_t;

/**
 * 4-byte signed integer type
 */
typedef signed int         acp_sint32_t;
/**
 * 4-byte unsigned integer type
 */
typedef unsigned int       acp_uint32_t;

#if defined(ACP_CFG_DOXYGEN)

/**
 * 8-byte signed integer type
 */
typedef signed long        acp_sint64_t;
/**
 * 8-byte unsigned integer type
 */
typedef unsigned long      acp_uint64_t;

#elif defined(ALTI_CFG_OS_WINDOWS)

typedef signed __int64     acp_sint64_t;
typedef unsigned __int64   acp_uint64_t;

#elif defined(ACP_CFG_COMPILE_64BIT)

# if defined(__STATIC_ANALYSIS_DOING__)

  typedef signed long long   acp_sint64_t;
  typedef unsigned long long acp_uint64_t;
# else
  typedef signed long        acp_sint64_t;
  typedef unsigned long      acp_uint64_t;

# endif

#else

typedef signed long long   acp_sint64_t;
typedef unsigned long long acp_uint64_t;

#endif

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

#if defined(ACP_CFG_COMPILE_64BIT)
typedef signed __int64     acp_slong_t;
typedef unsigned __int64   acp_ulong_t;
#else
typedef signed int         acp_slong_t;
typedef unsigned int       acp_ulong_t;
#endif

#else

# if defined(__STATIC_ANALYSIS_DOING__)
#  if defined(ACP_CFG_COMPILE_64BIT)
    typedef signed long long   acp_slong_t;
    typedef unsigned long long acp_ulong_t;
#  else
    typedef signed long        acp_slong_t;
    typedef unsigned long      acp_ulong_t;
#  endif

# else /* normal compile */
/**
 * 4-byte signed integer type in 32-bit system,
 * 8-byte signed integer type in 64-bit system
 */
 typedef signed long        acp_slong_t;
/**
 * 4-byte unsigned integer type in 32-bit system,
 * 8-byte unsigned integer type in 64-bit system
 */
 typedef unsigned long      acp_ulong_t;
# endif

#endif

/*
 * Floating Point Type
 */
/**
 * single precision floating pointer number type
 */
typedef float              acp_float_t;
/**
 * double precision floating pointer number type
 */
typedef double             acp_double_t;

/*
 * System Type
 */
#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)
typedef acp_sint32_t       acp_key_t;
typedef acp_ulong_t        acp_size_t;
typedef acp_slong_t        acp_ssize_t;
typedef acp_sint64_t       acp_offset_t;
#else
/**
 * ipc key for shared memory or semaphore objects
 */
typedef key_t              acp_key_t;
/**
 * unsigned integer type for memory access;
 * 4-byte unsigned integer in 32-bit system,
 * 8-byte unsigned integer in 64-bit system
 */
typedef size_t             acp_size_t;
/**
 * signed integer type for memory access;
 * 4-byte signed integer in 32-bit system,
 * 8-byte signed integer in 64-bit system
 */
typedef ssize_t            acp_ssize_t;
/**
 * signed integer type for file access (file size, file offset);
 * 4-byte signed integer in 32-bit file system,
 * 8-byte signed integer in 64-bit file system
 */
typedef off_t              acp_offset_t;
#endif

/**
 * type for difference between pointer
 */
typedef ptrdiff_t          acp_ptrdiff_t;

/*
 * Boolean Type
 */
/**
 * boolean type whose value can be one of #ACP_TRUE, #ACP_FALSE
 */
typedef acp_uint8_t        acp_bool_t;

/**
 * boolean value for @c false
 */
#define ACP_FALSE          ((acp_bool_t)0)
/**
 * boolean value for @c true
 */
#define ACP_TRUE           ((acp_bool_t)1)

/*
 * Literal
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * makes 8-byte unsigned integer literal
 * @param aVal a literal integer
 */
#define ACP_UINT64_LITERAL(aVal)
/**
 * makes 8-byte signed integer literal
 * @param aVal a literal integer
 */
#define ACP_SINT64_LITERAL(aVal)
/**
 * makes double precision floating pointer number
 * from 8-byte signed integer literal
 * @param aVal a literal number
 */
#define ACP_SINT64_TO_DOUBLE(aVal)

#elif defined(ALTI_CFG_OS_WINDOWS)

#define ACP_UINT64_LITERAL(aVal)   ((acp_uint64_t)(aVal ## ui64))
#define ACP_SINT64_LITERAL(aVal)   ((acp_sint64_t)(aVal ## i64))
#define ACP_SINT64_TO_DOUBLE(aVal) ((acp_double_t)(aVal))

#elif defined(ACP_CFG_COMPILE_64BIT)

#define ACP_UINT64_LITERAL(aVal)   ((acp_uint64_t)(aVal ## UL))
#define ACP_SINT64_LITERAL(aVal)   ((acp_sint64_t)(aVal ## L))
#define ACP_SINT64_TO_DOUBLE(aVal) ((acp_double_t)(aVal))

#else

#define ACP_UINT64_LITERAL(aVal)   ((acp_uint64_t)(aVal ## ULL))
#define ACP_SINT64_LITERAL(aVal)   ((acp_sint64_t)(aVal ## LL))
#define ACP_SINT64_TO_DOUBLE(aVal) ((acp_double_t)(aVal))

#endif

/*
 * Min / Max
 */
/**
 * takes greater value
 * @param aVal1 a value
 * @param aVal2 a value
 * this macro will evaluate arguments more than once,
 * so arguments should not be expressions with potential side effects.
 */
#define ACP_MAX(aVal1, aVal2) (((aVal1) > (aVal2)) ? (aVal1) : (aVal2))
/**
 * takes lesser value
 * @param aVal1 a value
 * @param aVal2 a value
 * this macro will evaluate arguments more than once,
 * so arguments should not be expressions with potential side effects.
 */
#define ACP_MIN(aVal1, aVal2) (((aVal1) < (aVal2)) ? (aVal1) : (aVal2))

/*
 * Suppress warning for unused variables
 */
/**
 * suppresses unused variable warning
 * @param aVar a variable name
 * this macro may evaluate arguments more than once,
 * so arguments should not be expressions with potential side effects.
 */
#define ACP_UNUSED(aVar) (void)(aVar)

/*
 * Integer Value Range
 */
/**
 * the maximum value which can be stored in #acp_sint8_t
 */
#define ACP_SINT8_MAX  ((acp_sint8_t)0x7F)
/**
 * the minimum value which can be stored in #acp_sint8_t
 */
#define ACP_SINT8_MIN  ((acp_sint8_t)(-(ACP_SINT8_MAX) - 1))
/**
 * the maximum value which can be stored in #acp_uint8_t
 */
#define ACP_UINT8_MAX  ((acp_uint8_t)0xFF)
/**
 * the minimum value which can be stored in #acp_uint8_t
 */
#define ACP_UINT8_MIN  ((acp_uint8_t)0)
/**
 * the maximum value which can be stored in #acp_sint16_t
 */
#define ACP_SINT16_MAX ((acp_sint16_t)0x7FFF)
/**
 * the minimum value which can be stored in #acp_sint16_t
 */
#define ACP_SINT16_MIN ((acp_sint16_t)(-(ACP_SINT16_MAX) - 1))
/**
 * the maximum value which can be stored in #acp_uint16_t
 */
#define ACP_UINT16_MAX ((acp_uint16_t)0xFFFF)
/**
 * the minimum value which can be stored in #acp_uint16_t
 */
#define ACP_UINT16_MIN ((acp_uint16_t)0)
/**
 * the maximum value which can be stored in #acp_sint32_t
 */
#define ACP_SINT32_MAX ((acp_sint32_t)0x7FFFFFFF)
/**
 * the minimum value which can be stored in #acp_sint32_t
 */
#define ACP_SINT32_MIN ((acp_sint32_t)(-(ACP_SINT32_MAX) - 1))
/**
 * the maximum value which can be stored in #acp_uint32_t
 */
#define ACP_UINT32_MAX ((acp_uint32_t)0xFFFFFFFF)
/**
 * the minimum value which can be stored in #acp_uint32_t
 */
#define ACP_UINT32_MIN ((acp_uint32_t)0)
/**
 * the maximum value which can be stored in #acp_sint64_t
 */
#define ACP_SINT64_MAX ACP_SINT64_LITERAL(0x7FFFFFFFFFFFFFFF)
/**
 * the minimum value which can be stored in #acp_sint64_t
 */
#define ACP_SINT64_MIN (-(ACP_SINT64_MAX)-ACP_SINT64_LITERAL(1))
/**
 * the maximum value which can be stored in #acp_uint64_t
 */
#define ACP_UINT64_MAX ACP_UINT64_LITERAL(0xFFFFFFFFFFFFFFFF)
/**
 * the minimum value which can be stored in #acp_uint64_t
 */
#define ACP_UINT64_MIN ACP_UINT64_LITERAL(0)

#if defined(ACP_CFG_DOXYGEN) || defined(ACP_CFG_COMPILE_64BIT)

/**
 * the maximum value which can be stored in #acp_slong_t
 */
#define ACP_SLONG_MAX  ACP_SINT64_MAX
/**
 * the minimum value which can be stored in #acp_slong_t
 */
#define ACP_SLONG_MIN  ACP_SINT64_MIN
/**
 * the maximum value which can be stored in #acp_ulong_t
 */
#define ACP_ULONG_MAX  ACP_UINT64_MAX
/**
 * the minimum value which can be stored in #acp_ulong_t
 */
#define ACP_ULONG_MIN  ACP_UINT64_MIN

#else

#define ACP_SLONG_MAX  ACP_SINT32_MAX
#define ACP_SLONG_MIN  ACP_SINT32_MIN
#define ACP_ULONG_MAX  ACP_UINT32_MAX
#define ACP_ULONG_MIN  ACP_UINT32_MIN
#endif

/**
 * the maximum value which can be stored in #acp_size_t
 */
#define ACP_SIZE_MAX   ACP_ULONG_MAX

/**
 * the maximum value which can be stored in #acp_float_t
 */
#define ACP_FLOAT_MAX  FLT_MAX
/**
 * the minimum value which can be stored in #acp_float_t
 */
#define ACP_FLOAT_MIN  FLT_MIN
/**
 * the maximum value which can be stored in #acp_double_t
 */
#define ACP_DOUBLE_MAX DBL_MAX
/**
 * the minimum value which can be stored in #acp_double_t
 */
#define ACP_DOUBLE_MIN DBL_MIN

/*
 * Export
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * exported symbol
 */
#define ACP_EXPORT

#else

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DL_STATIC)
#if defined(ACP_CFG_DL_EXPORT)
#define ACP_EXPORT __declspec(dllexport)
#else
#define ACP_EXPORT __declspec(dllimport)
#endif
#else
#define ACP_EXPORT
#endif

#endif

/*
 * Inline
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * inline function
 */
#define ACP_INLINE

#else

#if defined(__GNUC__)
#if defined(ACP_CFG_DEBUG)
#define ACP_INLINE static __inline__
#elif ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC__MINOR__ > 0)))
#define ACP_INLINE static __inline__ __attribute__((always_inline))
#else
#define ACP_INLINE static __inline__
#endif
#elif defined(_MSC_VER)
#define ACP_INLINE static __forceinline
#else
#define ACP_INLINE static
#endif

#endif

/**
 * extended character types
 */
typedef acp_uint8_t        acp_byte_t;
typedef acp_uint16_t       acp_dchar_t;
typedef acp_uint32_t       acp_qchar_t;

/**
 * file and shared memory permission type
 */

#if defined(ALTI_CFG_OS_WINDOWS)
typedef acp_sint32_t       acp_mode_t;
#else
typedef mode_t             acp_mode_t;
#endif

#define ACP_FILE_NAME __FILE__
#define ACP_LINE_NUM __LINE__

/**
 * thread-specific data generate by compiler
 */

#if defined(ALTI_CFG_OS_WINDOWS)
#define ACP_TLS(aType, aVarName) static __declspec(thread) aType aVarName
#else
#if defined(ALTI_CFG_CPU_POWERPC)
#define ACP_TLS(aType, aVarName) static __thread __attribute__ ((tls_model("local-dynamic"))) aType aVarName
#else
#define ACP_TLS(aType, aVarName) static __thread aType aVarName
#endif
#endif

/**
 * branch prediction
 * BUG-35758 [id] add branch prediction to AIX for the performance.
 */
#if (defined(ALTI_CFG_OS_LINUX)) || (defined(ALTI_CFG_OS_AIX) && (__xlC__ >= 0x0a01))
#define ACP_LIKELY_TRUE(x)       __builtin_expect((long int)(x), 1)
#define ACP_LIKELY_FALSE(x)      __builtin_expect((long int)(x), 0)
#else
#define ACP_LIKELY_TRUE(x)       x
#define ACP_LIKELY_FALSE(x)      x
#endif

ACP_EXTERN_C_END


#endif
