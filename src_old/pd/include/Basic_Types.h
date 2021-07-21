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
// Basic_Types.h,v 4.61 1999/09/22 16:23:16 levine Exp

// ============================================================================
//
// = LIBRARY
//    pdl
//
// = FILENAME
//    Basic_Types.h
//
// = AUTHORS
//    David L. Levine
//
// = DESCRIPTION
//    #defines the list of preprocessor macros below.  The config.h file can
//    pre-define any of these to short-cut the definitions.  This is usually
//    only necessary if the preprocessor does all of its math using integers.
//
//    Sizes of built-in types:
//      PDL_SIZEOF_CHAR
//      PDL_SIZEOF_WCHAR
//      PDL_SIZEOF_SHORT
//      PDL_SIZEOF_INT
//      PDL_SIZEOF_LONG
//      PDL_SIZEOF_LONG_LONG
//      PDL_SIZEOF_VOID_P
//      PDL_SIZEOF_FLOAT
//      PDL_SIZEOF_DOUBLE
//      PDL_SIZEOF_LONG_DOUBLE
//
//    Wrappers for built-in types of specific sizes:
//      PDL_USHORT16 /* For backward compatibility.  Use PDL_UINT16 instead. */
//      PDL_INT16
//      PDL_UINT16
//      PDL_INT32
//      PDL_UINT32
//      PDL_UINT64
//    (Note: PDL_INT64 is not defined, because there is no PDL_LongLong for
//     platforms that don't have a native 8-byte integer type.)
//
//    Byte-order (endian-ness) determination:
//      PDL_BYTE_ORDER, to either PDL_BIG_ENDIAN or PDL_LITTLE_ENDIAN
//
// ============================================================================

#ifndef PDL_BASIC_TYPES_H
# define PDL_BASIC_TYPES_H


/* before modifying, check acpConfig.h ,Basic_Types.h, and OS.h
   (BUG-45623,BUG-46609)*/

#ifndef FD_SETSIZE_NOT_REDEFINED
#define FD_SETSIZE_NOT_REDEFINED

#if !defined(ANDROID)
#  if defined(ALTI_CFG_OS_HPUX)
#    undef  FD_SETSIZE
#    define FD_SETSIZE 65536    
#  elif defined(ALTI_CFG_OS_AIX)
#    ifdef _H_SELECT
#      error  select_h_is_already_included
#    endif
#    undef  FD_SETSIZE
#    define FD_SETSIZE 65536   
#  else
#    ifdef _WIN32
#      define FD_SETSIZE 1024
#    else
#      if !defined(INTEL_LINUX)
#        undef  FD_SETSIZE
#        define FD_SETSIZE 65536
#      endif
#    endif
#  endif
#endif

#endif  /* #ifndef FD_SETSIZE_NOT_REDEFINED */


# if !defined (PDL_LACKS_PRAGMA_ONCE)
#   pragma once
# endif /* PDL_LACKS_PRAGMA_ONCE */

// A char always has 1 byte, by definition.
# define PDL_SIZEOF_CHAR 1

// Unfortunately, there isn't a portable way to determine the size of a wchar.
# if defined (PDL_HAS_WCHAR_TYPEDEFS_CHAR)
#   define PDL_SIZEOF_WCHAR 1
# else
#   define PDL_SIZEOF_WCHAR sizeof (wchar_t)
# endif /* PDL_HAS_WCHAR_TYPEDEFS_CHAR */

// The number of bytes in a short.
# if !defined (PDL_SIZEOF_SHORT)
#   if (USHRT_MAX) == 255U
#     define PDL_SIZEOF_SHORT 1
#   elif (USHRT_MAX) == 65535U
#     define PDL_SIZEOF_SHORT 2
#   elif (USHRT_MAX) == 4294967295U
#     define PDL_SIZEOF_SHORT 4
#   elif (USHRT_MAX) == 18446744073709551615U
#     define PDL_SIZEOF_SHORT 8
#   else
#     error: unsupported short size, must be updated for this platform!
#   endif /* USHRT_MAX */
# endif /* !defined (PDL_SIZEOF_SHORT) */

// The number of bytes in an int.
# if !defined (PDL_SIZEOF_INT)
#   if (UINT_MAX) == 65535U
#     define PDL_SIZEOF_INT 2
#   elif (UINT_MAX) == 4294967295U
#     define PDL_SIZEOF_INT 4
#   elif (UINT_MAX) == 18446744073709551615U
#     define PDL_SIZEOF_INT 8
#   else
#     error: unsupported int size, must be updated for this platform!
#   endif /* UINT_MAX */
# endif /* !defined (PDL_SIZEOF_INT) */

// The number of bytes in a long.
// NOTE - since preprocessors only need to do integer math, this is a likely
// place for a preprocessor to not properly support being able to figure out
// the proper size.  HP aC++ and GNU gcc have this difficulty so watch out.
# if !defined (PDL_SIZEOF_LONG)
#   if (ULONG_MAX) == 65535UL
#     define PDL_SIZEOF_LONG 2
#   elif ((ULONG_MAX) == 4294967295UL)
#     define PDL_SIZEOF_LONG 4
#   elif ((ULONG_MAX) == 18446744073709551615UL)
#     define PDL_SIZEOF_LONG 8
#   else
#     error: unsupported long size, must be updated for this platform!
#   endif /* ULONG_MAX */
# endif /* !defined (PDL_SIZEOF_LONG) */

// The number of bytes in a long long.
# if !defined (PDL_SIZEOF_LONG_LONG)
#   if defined (PDL_LACKS_LONGLONG_T)
#     define PDL_SIZEOF_LONG_LONG 8
#   else  /* ! PDL_WIN32 && ! PDL_LACKS_LONGLONG_T */
#     if PDL_SIZEOF_LONG == 8
#       define PDL_SIZEOF_LONG_LONG 8
       typedef unsigned long PDL_UINT64;
#     elif defined (ULLONG_MAX) && !defined (__GNUG__)
       // Some compilers use ULLONG_MAX and others, e.g. Irix, use
       // ULONGLONG_MAX.
#       if (ULLONG_MAX) == 18446744073709551615ULL
#         define PDL_SIZEOF_LONG_LONG 8
#       elif (ULLONG_MAX) == 4294967295ULL
#         define PDL_SIZEOF_LONG_LONG 4
#       else
#         error Unsupported long long size needs to be updated for this platform
#       endif
       typedef unsigned long long PDL_UINT64;
#     elif defined (ULONGLONG_MAX) && !defined (__GNUG__)
       // Irix 6.x, for example.
#       if (ULONGLONG_MAX) == 18446744073709551615ULL
#         define PDL_SIZEOF_LONG_LONG 8
#       elif (ULONGLONG_MAX) == 4294967295ULL
#         define PDL_SIZEOF_LONG_LONG 4
#       else
#         error Unsupported long long size needs to be updated for this platform
#       endif
       typedef unsigned long long PDL_UINT64;
#     else
       // PDL_SIZEOF_LONG_LONG is not yet known, but the platform doesn't
       // claim PDL_LACKS_LONGLONG_T, so assume it has 8-byte long longs.
#       define PDL_SIZEOF_LONG_LONG 8
#       if defined (sun) && !defined (PDL_LACKS_U_LONGLONG_T)
         // sun #defines u_longlong_t, maybe other platforms do also.
         // Use it, at least with g++, so that its -pedantic doesn't
         // complain about no ANSI C++ long long.
         typedef u_longlong_t PDL_UINT64;
#       else
         // LynxOS 2.5.0 and Linux don't have u_longlong_t.
         typedef unsigned long long PDL_UINT64;
#       endif /* sun */
#     endif /* ULLONG_MAX && !__GNUG__ */
#   endif /* ! PDL_WIN32 && ! PDL_LACKS_LONGLONG_T */
# endif /* !defined (PDL_SIZEOF_LONG_LONG) */


// The sizes of the commonly implemented types are now known.  Set up
// typedefs for whatever we can.  Some of these are needed for certain cases
// of PDL_UINT64, so do them before the 64-bit stuff.

# if PDL_SIZEOF_SHORT == 2
  typedef short PDL_INT16;
  typedef unsigned short PDL_UINT16;
# elif PDL_SIZEOF_INT == 2
  typedef int PDL_INT16;
  typedef unsigned int PDL_UINT16;
# elif (PDL_SIZEOF_SHORT) == 4 && defined(_CRAYMPP)
  // mpp cray - uses Alpha processors
  //   Use the real 32-bit quantity for PDL_INT32's, and use a "long"
  //   for PDL_INT16's.  This gets around conflicts with size_t in some PDL
  //   method signatures, among other things.
  typedef long PDL_INT16;
  typedef unsigned long PDL_UINT16;
  typedef short PDL_INT32;
  typedef unsigned short PDL_UINT32;
# elif (PDL_SIZEOF_SHORT) == 8 && defined(_UNICOS)
  // vector cray - hard 64-bit, all 64 bit types
  typedef short PDL_INT16;
  typedef unsigned short PDL_UINT16;
# else
#   error Have to add to the PDL_UINT16 type setting
# endif

typedef PDL_UINT16 PDL_USHORT16;

# if PDL_SIZEOF_INT == 4
  typedef int PDL_INT32;
  typedef unsigned int PDL_UINT32;
#   if defined (__KCC) && !defined (PDL_LACKS_LONGLONG_T)
  typedef unsigned long long PDL_UINT64;
#   endif /* __KCC */
# elif PDL_SIZEOF_LONG == 4
  typedef long PDL_INT32;
  typedef unsigned long PDL_UINT32;
# elif (PDL_SIZEOF_INT) == 8 && defined(_UNICOS)
  // vector cray - hard 64-bit, all 64 bit types
#   if !defined(_CRAYMPP)
      typedef int PDL_INT32;
      typedef unsigned int PDL_UINT32;
#   endif
  typedef unsigned long long PDL_UINT64;
# else
#   error Have to add to the PDL_UINT32 type setting
# endif

// The number of bytes in a void *.
# ifndef PDL_SIZEOF_VOID_P
#   define PDL_SIZEOF_VOID_P PDL_SIZEOF_LONG
# endif /* PDL_SIZEOF_VOID_P */

// Type for doing arithmetic on pointers ... as elsewhere, we assume
// that unsigned versions of a type are the same size as the signed
// version of the same type.
#if PDL_SIZEOF_VOID_P == PDL_SIZEOF_INT
# if defined (__SUNPRO_CC)
    // For unknown reasons, Sun CC 5.0 won't allow a reintepret cast
    // of a 64-bit pointer to a 64-bit int.
    typedef u_long ptr_arith_t;
# else  /* ! __SUNPRO_CC */
    typedef u_int ptr_arith_t;
# endif /* ! __SUNPRO_CC */
#elif PDL_SIZEOF_VOID_P == PDL_SIZEOF_LONG
  typedef u_long ptr_arith_t;
#elif PDL_SIZEOF_VOID_P == PDL_SIZEOF_LONG_LONG
# if !defined(PDL_WIN64)
  typedef u_long long ptr_arith_t;
# else
  typedef ULONGLONG ptr_arith_t;
# endif
#else
# error "Can't find a suitable type for doing pointer arithmetic."
#endif /* PDL_SIZEOF_VOID_P */

#if defined (PDL_LACKS_LONGLONG_T)
  // This throws away the high 32 bits.  It's very unlikely that a
  // pointer will be more than 32 bits wide if the platform does not
  // support 64-bit integers.
# define PDL_LONGLONG_TO_PTR(PTR_TYPE, L) \
  PDL_reinterpret_cast (PTR_TYPE, L.lo ())
#else  /* ! PDL_LACKS_LONGLONG_T */
# define PDL_LONGLONG_TO_PTR(PTR_TYPE, L) \
  PDL_reinterpret_cast (PTR_TYPE, PDL_static_cast (ptr_arith_t, L))
#endif /* ! PDL_LACKS_LONGLONG_T */

// If the platform lacks a long long, define one.
# if defined (PDL_LACKS_LONGLONG_T)
  class PDL_Export PDL_U_LongLong
  // = TITLE
  //     Unsigned long long for platforms that don't have one.
  //
  // = DESCRIPTION
  //     Provide our own unsigned long long.  This is intended to be
  //     use with PDL_High_Res_Timer, so the division operator assumes
  //     that the quotient fits into a u_long.
  //     Please note that the constructor takes (optionally) two values.
  //     The high one contributes 0x100000000 times its value.  So,
  //     for example, (0, 2) is _not_ 20000000000, but instead
  //     0x200000000.  To emphasize this, the default values are expressed
  //     in hex, and output () dumps the value in hex.
  {
  public:
    // = Initialization and termination methods.
    PDL_U_LongLong (const PDL_UINT32 lo = 0x0, const PDL_UINT32 hi = 0x0);
    PDL_U_LongLong (const PDL_U_LongLong &);
    PDL_U_LongLong &operator= (const PDL_U_LongLong &);
    ~PDL_U_LongLong (void);

    // = Overloaded relation operators.
    int operator== (const PDL_U_LongLong &) const;
    int operator== (const PDL_UINT32) const;
    int operator!= (const PDL_U_LongLong &) const;
    int operator!= (const PDL_UINT32) const;
    int operator< (const PDL_U_LongLong &) const;
    int operator< (const PDL_UINT32) const;
    int operator<= (const PDL_U_LongLong &) const;
    int operator<= (const PDL_UINT32) const;
    int operator> (const PDL_U_LongLong &) const;
    int operator> (const PDL_UINT32) const;
    int operator>= (const PDL_U_LongLong &) const;
    int operator>= (const PDL_UINT32) const;

    PDL_U_LongLong operator+ (const PDL_U_LongLong &) const;
    PDL_U_LongLong operator+ (const PDL_UINT32) const;
    PDL_U_LongLong operator- (const PDL_U_LongLong &) const;
    PDL_U_LongLong operator- (const PDL_UINT32) const;
    PDL_U_LongLong operator* (const PDL_UINT32);
    PDL_U_LongLong &operator*= (const PDL_UINT32);

    PDL_U_LongLong operator<< (const u_int) const;
    PDL_U_LongLong &operator<<= (const u_int);
    PDL_U_LongLong operator>> (const u_int) const;
    PDL_U_LongLong &operator>>= (const u_int);

    double operator/ (const double) const;

    PDL_U_LongLong &operator+= (const PDL_U_LongLong &);
    PDL_U_LongLong &operator+= (const PDL_UINT32);
    PDL_U_LongLong &operator-= (const PDL_U_LongLong &);
    PDL_U_LongLong &operator-= (const PDL_UINT32);
    PDL_U_LongLong &operator++ ();
    PDL_U_LongLong &operator-- ();
    PDL_U_LongLong &operator|= (const PDL_U_LongLong);
    PDL_U_LongLong &operator|= (const PDL_UINT32);
    PDL_U_LongLong &operator&= (const PDL_U_LongLong);
    PDL_U_LongLong &operator&= (const PDL_UINT32);

    // Note that the following take PDL_UINT32 arguments.  These are
    // typical use cases, and easy to implement.  But, they limit the
    // return values to 32 bits as well.  There are no checks for
    // overflow.
    PDL_UINT32 operator/ (const PDL_UINT32) const;
    PDL_UINT32 operator% (const PDL_UINT32) const;

    // The following only operate on the lower 32 bits (they take only
    // 32 bit arguments).
    PDL_UINT32 operator| (const PDL_INT32) const;
    PDL_UINT32 operator& (const PDL_INT32) const;

    // The following operators convert their arguments to
    // PDL_UINT32.  So, there may be information loss if they are
    // used.
    PDL_U_LongLong operator* (const PDL_INT32);
    PDL_U_LongLong &operator*= (const PDL_INT32);
    PDL_UINT32 operator/ (const PDL_INT32) const;
#   if PDL_SIZEOF_INT == 4
    PDL_UINT32 operator/ (const u_long) const;
    PDL_UINT32 operator/ (const long) const;
#   else  /* PDL_SIZEOF_INT != 4 */
    PDL_UINT32 operator/ (const u_int) const;
    PDL_UINT32 operator/ (const int) const;
#   endif /* PDL_SIZEOF_INT != 4 */

    // = Helper methods.
    void output (FILE * = stdout) const;
    // Outputs the value to the FILE, in hex.

    PDL_UINT32 hi (void) const;
    PDL_UINT32 lo (void) const;

    void hi (const PDL_UINT32 hi);
    void lo (const PDL_UINT32 lo);

    PDL_ALLOC_HOOK_DECLARE;

  private:
    union
      {
        struct
          {
            PDL_UINT32 hi_;
            // High 32 bits.

            PDL_UINT32 lo_;
            // Low 32 bits.
          } data_;
        // To ensure alignment on 8-byte boundary.

        // double isn't usually usable with PDL_LACKS_FLOATING_POINT,
        // but this seems OK.
        double for_alignment_;
        // To ensure alignment on 8-byte boundary.
      };

    // NOTE:  the following four accessors are inlined here in
    // order to minimize the extent of the data_ struct.  It's
    // only used here; the .i and .cpp files use the accessors.

    const PDL_UINT32 &h_ () const { return data_.hi_; }
    // Internal utility function to hide access through struct.

    PDL_UINT32 &h_ () { return data_.hi_; }
    // Internal utility function to hide access through struct.

    const PDL_UINT32 &l_ () const { return data_.lo_; }
    // Internal utility function to hide access through struct.

    PDL_UINT32 &l_ () { return data_.lo_; }
    // Internal utility function to hide access through struct.

    // NOTE:  the above four accessors are inlined here in
    // order to minimize the extent of the data_ struct.  It's
    // only used here; the .i and .cpp files use the accessors.

    PDL_UINT32 ul_shift (PDL_UINT32 a, PDL_UINT32 c_in, PDL_UINT32 *c_out);
    PDL_U_LongLong ull_shift (PDL_U_LongLong a, PDL_UINT32 c_in,
                              PDL_UINT32 *c_out);
    PDL_U_LongLong ull_add (PDL_U_LongLong a, PDL_U_LongLong b,
                            PDL_UINT32 *carry);
    PDL_U_LongLong ull_mult (PDL_U_LongLong a, PDL_UINT32 b,
                             PDL_UINT32 *carry);
    // These functions are used to implement multiplication.
  };

  typedef PDL_U_LongLong PDL_UINT64;

# endif /* PDL_LACKS_LONGLONG_T */

// Conversions from PDL_UINT64 to PDL_UINT32.  PDL_CU64_TO_CU32 should
// be used on const PDL_UINT64's.
# if defined (PDL_LACKS_LONGLONG_T)
#   define PDL_U64_TO_U32(n) ((n).lo ())
#   define PDL_CU64_TO_CU32(n) ((n).lo ())
# else  /* ! PDL_LACKS_LONGLONG_T */
#   define PDL_U64_TO_U32(n) (PDL_static_cast (PDL_UINT32, (n)))
#   define PDL_CU64_TO_CU32(n) \
     (PDL_static_cast (PDL_CAST_CONST PDL_UINT32, (n)))
# endif /* ! PDL_LACKS_LONGLONG_T */

// 64-bit literals require special marking on some platforms.
# if defined (PDL_WIN32)
#   if defined (__IBMCPP__) && (__IBMCPP__ >= 400)
#     define PDL_UINT64_LITERAL(n) n ## LL
#     define PDL_INT64_LITERAL(n) n ## LL
#else
#   define PDL_UINT64_LITERAL(n) n ## ui64
#   define PDL_INT64_LITERAL(n) n ## i64
#endif /* defined (__IBMCPP__) && (__IBMCPP__ >= 400) */
# elif defined (PDL_LACKS_LONGLONG_T)
    // Can only specify 32-bit arguments.
#   define PDL_UINT64_LITERAL(n) (PDL_U_LongLong (n))
      // This one won't really work, but it'll keep
      // some compilers happy until we have better support
#   define PDL_INT64_LITERAL(n) (PDL_U_LongLong (n))
# else  /* ! PDL_WIN32  &&  ! PDL_LACKS_LONGLONG_T */
#   define PDL_UINT64_LITERAL(n) n ## ull
#   define PDL_INT64_LITERAL(n) n ## ll
# endif /* ! PDL_WIN32  &&  ! PDL_LACKS_LONGLONG_T */

#if !defined (PDL_UINT64_FORMAT_SPECIFIER)
# define PDL_UINT64_FORMAT_SPECIFIER "%llu"
#endif /* PDL_UINT64_FORMAT_SPECIFIER */

#if !defined (PDL_INT64_FORMAT_SPECIFIER)
# define PDL_INT64_FORMAT_SPECIFIER "%lld"
#endif /* PDL_INT64_FORMAT_SPECIFIER */

// Cast from UINT64 to a double requires an intermediate cast to INT64
// on some platforms.
# if defined (PDL_WIN32)
#   define PDL_UINT64_DBLCAST_ADAPTER(n) PDL_static_cast (__int64, n)
# elif defined (PDL_LACKS_LONGLONG_T)
   // Only use the low 32 bits.
#   define PDL_UINT64_DBLCAST_ADAPTER(n) PDL_U64_TO_U32 (n)
# else  /* ! PDL_WIN32 && ! PDL_LACKS_LONGLONG_T */
#   define PDL_UINT64_DBLCAST_ADAPTER(n) (n)
# endif /* ! PDL_WIN32 && ! PDL_LACKS_LONGLONG_T */


// The number of bytes in a float.
# ifndef PDL_SIZEOF_FLOAT
#   if FLT_MAX_EXP == 128
#     define PDL_SIZEOF_FLOAT 4
#   elif FLT_MAX_EXP == 1024
#     define PDL_SIZEOF_FLOAT 8
#   else
#     error: unsupported float size, must be updated for this platform!
#   endif /* FLT_MAX_EXP */
# endif /* PDL_SIZEOF_FLOAT */

// The number of bytes in a double.
# ifndef PDL_SIZEOF_DOUBLE
#   if DBL_MAX_EXP == 128
#     define PDL_SIZEOF_DOUBLE 4
#   elif DBL_MAX_EXP == 1024
#     define PDL_SIZEOF_DOUBLE 8
#   else
#     error: unsupported double size, must be updated for this platform!
#   endif /* DBL_MAX_EXP */
# endif /* PDL_SIZEOF_DOUBLE */

// The number of bytes in a long double.
# ifndef PDL_SIZEOF_LONG_DOUBLE
#   if LDBL_MAX_EXP == 128
#     define PDL_SIZEOF_LONG_DOUBLE 4
#   elif LDBL_MAX_EXP == 1024
#     define PDL_SIZEOF_LONG_DOUBLE 8
#   elif LDBL_MAX_EXP == 16384
#     if defined (LDBL_DIG)  &&  LDBL_DIG == 18
#       define PDL_SIZEOF_LONG_DOUBLE 12
#     else  /* ! LDBL_DIG  ||  LDBL_DIG != 18 */
#       define PDL_SIZEOF_LONG_DOUBLE 16
#     endif /* ! LDBL_DIG  ||  LDBL_DIG != 18 */
#   else
#     error: unsupported double size, must be updated for this platform!
#   endif /* LDBL_MAX_EXP */
# endif /* PDL_SIZEOF_LONG_DOUBLE */

// Max and min sizes for the PDL integer types.
#define PDL_CHAR_MAX 0x7F
#define PDL_CHAR_MIN -(PDL_CHAR_MAX)-1
#define PDL_OCTET_MAX 0xFF
#define PDL_INT16_MAX 0x7FFF
#define PDL_INT16_MIN -(PDL_INT16_MAX)-1
#define PDL_UINT16_MAX 0xFFFF
#define PDL_WCHAR_MAX PDL_UINT16_MAX
#define PDL_INT32_MAX 0x7FFFFFFF
#define PDL_INT32_MIN -(PDL_INT32_MAX)-1
#define PDL_UINT32_MAX 0xFFFFFFFF
#define PDL_INT64_MAX PDL_INT64_LITERAL(0x7FFFFFFFFFFFFFFF)
#define PDL_INT64_MIN -(PDL_INT64_MAX)-1
#define PDL_UINT64_MAX PDL_UINT64_LITERAL(0xFFFFFFFFFFFFFFFF)
// These use ANSI/IEEE format.
#define PDL_FLT_MAX 3.402823466e+38F
#define PDL_DBL_MAX 1.7976931348623158e+308

// Byte-order (endian-ness) determination.
# if defined (BYTE_ORDER)
#   if (BYTE_ORDER == LITTLE_ENDIAN)
#     define PDL_LITTLE_ENDIAN 0123X
#     define PDL_BYTE_ORDER PDL_LITTLE_ENDIAN
#   elif (BYTE_ORDER == BIG_ENDIAN)
#     define PDL_BIG_ENDIAN 3210X
#     define PDL_BYTE_ORDER PDL_BIG_ENDIAN
#   else
#     error: unknown BYTE_ORDER!
#   endif /* BYTE_ORDER */
# elif defined (__BYTE_ORDER)
#   if (__BYTE_ORDER == __LITTLE_ENDIAN)
#     define PDL_LITTLE_ENDIAN 0123X
#     define PDL_BYTE_ORDER PDL_LITTLE_ENDIAN
#   elif (__BYTE_ORDER == __BIG_ENDIAN)
#     define PDL_BIG_ENDIAN 3210X
#     define PDL_BYTE_ORDER PDL_BIG_ENDIAN
#   else
#     error: unknown __BYTE_ORDER!
#   endif /* __BYTE_ORDER */
# else /* ! BYTE_ORDER && ! __BYTE_ORDER */
  // We weren't explicitly told, so we have to figure it out . . .
#   if defined (i386) || defined (__i386__) || defined (_M_IX86) || \
     defined (vax) || defined (__alpha)
    // We know these are little endian.
#     define PDL_LITTLE_ENDIAN 0123X
#     define PDL_BYTE_ORDER PDL_LITTLE_ENDIAN
#   else
    // Otherwise, we assume big endian.
#     define PDL_BIG_ENDIAN 3210X
#     define PDL_BYTE_ORDER PDL_BIG_ENDIAN
#   endif
# endif /* ! BYTE_ORDER && ! __BYTE_ORDER */

# if defined (__PDL_INLINE__)
#   include "Basic_Types.i"
# endif /* __PDL_INLINE__ */

#endif /* PDL_BASIC_TYPES_H */
