/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idTypes.h 90605 2021-04-15 05:43:22Z justin.kwon $
 **********************************************************************/

/***********************************************************************
 *
 * NAME
 *   ispTypes.h
 * 
 * DESCRIPTION
 *   This file defines data types.
 * 
 * PUBLIC FUNCTION(S)
 * 
 * 
 * PRIVATE FUNCTION(S)
 * 
 * 
 * NOTES
 * 
 * MODIFIED    (MM/DD/YYYY)
 *    assam     12/27/1999 - Created
 * 
 **********************************************************************/

#ifndef  _O_IDTYPES_H_
# define _O_IDTYPES_H_ 1

#include <idConfig.h>

#if defined(VC_WIN32) || defined(VC_WIN64)
#include <windows.h>
typedef CHAR      SChar;  /* Signed    8-bits */
typedef UCHAR     UChar;  /* Unsigned  8-bits */
typedef SHORT     SShort; /* Signed   16-bits */
typedef USHORT    UShort; /* Unsigned 16-bits */
typedef INT       SInt;   /* Signed   32-bits */
typedef UINT      UInt;   /* Unsigned 32-bits */
typedef LONGLONG  SLong;  /* Signed   64-bits */
typedef ULONGLONG ULong;  /* Unsigned 64-bits */
# if !defined(COMPILE_64BIT) /* && !(_MSC_VER >= 1300) */
typedef LONG      vSLong; /* variable Signed   32,[64]-bits */
typedef ULONG     vULong; /* variable Unsigned 32,[64]-bits */
# else
typedef LONGLONG  vSLong; /* Signed   64-bits */
typedef ULONGLONG vULong; /* Unsigned 64-bits */
# endif
#elif defined(WRS_VXWORKS) || defined(SYMBIAN)
typedef char SChar;             /* Signed    8-bits */
typedef unsigned char UChar;    /* Unsigned  8-bits */
typedef short SShort;           /* Signed   16-bits */
typedef unsigned short UShort;  /* Unsigned 16-bits */
typedef int SInt;               /* Signed   32-bits */
typedef unsigned int UInt;      /* Unsigned 32-bits */
typedef long long SLong;           /* Signed   64-bits */
typedef unsigned long long ULong;  /* Unsigned 64-bits */
# if defined(COMPILE_64BIT)
typedef long long vSLong;          /* variable Signed   32,[64]-bits */
typedef unsigned long long vULong; /* variable Unsigned 32,[64]-bits */
# else
typedef long vSLong;               /* variable Signed   32,[64]-bits */
typedef unsigned long vULong;      /* variable Unsigned 32,[64]-bits */
# endif

#elif defined(ITRON)
typedef char SChar;             /* Signed    8-bits */
typedef unsigned char UChar;    /* Unsigned  8-bits */
typedef short SShort;           /* Signed   16-bits */
typedef unsigned short UShort;  /* Unsigned 16-bits */
typedef int SInt;               /* Signed   32-bits */
typedef unsigned int UInt;      /* Unsigned 32-bits */

/* # ifndef COMPILE_32BIT */
/* typedef long SLong; */               /* Signed   64-bits */
/* typedef unsigned long ULong; */      /* Unsigned 64-bits */
/* typedef long vSLong; */              /* variable Signed   32,[64]-bits */
/* typedef unsigned long vULong; */     /* variable Unsigned 32,[64]-bits */
/* # else */
/* typedef long long SLong; */          /* Signed   64-bits */
/* typedef unsigned long long ULong; */ /* Unsigned 64-bits */
/* typedef long long vSLong; */         /* variable Signed   32,[64]-bits */
/* typedef unsigned long long vULong;*/ /* variable Unsigned 32,[64]-bits */
/* # endif */

typedef long long SLong;           /* Signed   64-bits */
typedef unsigned long long ULong;  /* Unsigned 64-bits */

/* typedef long long vSLong; */          /* variable Signed   32,[64]-bits */
/* typedef unsigned long long vULong; */ /* variable Unsigned 32,[64]-bits */

typedef long vSLong;          /* variable Signed   32,[64]-bits */
typedef unsigned long vULong; /* variable Unsigned 32,[64]-bits */

#else /* !(ITRON + WRS_VXWORKS + VC_WIN32 + VC_WIN64) */

# if defined( DEC_TRU64 ) || defined( IBM_AIX )
#  include <sys/types.h>
# else
# if defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5)
#  include <sys/types.h>
# else
#  include <limits.h>
#  if !defined(CYGWIN32) && !defined(WRS_VXWORKS)
#   include <inttypes.h>
#  endif
# endif /* defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
# endif   /* DEC_TRU64 */

# if defined( INTEL_LINUX ) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(ALPHA_LINUX) || defined(NTO_QNX) || defined( CYGWIN32 ) || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) || defined(ARM_LINUX) || defined(ELDK_LINUX) || defined(MIPS64_LINUX) || defined(X86_64_DARWIN)

# if defined( __GNUC__ )
typedef char SChar;             /* Signed    8-bits */
typedef unsigned char UChar;    /* Unsigned  8-bits */
typedef short SShort;           /* Signed   16-bits */
typedef unsigned short UShort;  /* Unsigned 16-bits */
typedef int SInt;               /* Signed   32-bits */
typedef unsigned int UInt;      /* Unsigned 32-bits */

# if defined (INTEL_LINUX) || defined(NTO_QNX) || defined(CYGWIN32) || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(ARM_LINUX) || defined(ELDK_LINUX) || defined(MIPS64_LINUX)
typedef long long SLong;           /* Signed   64-bits */
# if defined (POWERPC64_LINUX)
typedef SLong     vSLong;           /* Signed   64-bits */
#endif
typedef unsigned long long ULong;  /* Unsigned 64-bits */
# else /* IA64_LINUX || ALPHA_LINUX || AMD64_LINUX */
  #if __CSURF__
    typedef long long SLong;                /* Signed   64-bits */
    typedef unsigned long long ULong;       /* Unsigned 64-bits */
  #else
    typedef long SLong;                /* Signed   64-bits */
    typedef unsigned long ULong;       /* Unsigned 64-bits */
  #endif
# endif

# else  /* __GNUC__ */
#  if defined (POWERPC64_LINUX) && defined(__xlC__)

typedef char SChar;             /* Signed    8-bits */
typedef unsigned char UChar;    /* Unsigned  8-bits */
typedef short SShort;           /* Signed   16-bits */
typedef unsigned short UShort;  /* Unsigned 16-bits */
typedef int SInt;               /* Signed   32-bits */
typedef unsigned int UInt;      /* Unsigned 32-bits */
typedef long long vSLong;           /* Signed   64-bits */
typedef long long SLong;                /* Signed   64-bits */
typedef unsigned long long ULong;       /* Unsigned 64-bits */

#  else
ERROR!!!
#  endif
# endif   /* __GNUC__ */

# else /* INTEL_LINUX || IA64_LINUX || ALPHA_LINUX || NTO_QNX || AMD64_LINUX */

/*# if defined(HP_HPUX)
typedef char          SChar;
typedef unsigned char UChar;
typedef PDL_INT16     SShort;
typedef PDL_UINT16    UShort;
typedef PDL_INT32     SInt;
typedef PDL_UINT32    UInt;
typedef long long     SLong;
typedef PDL_UINT64    ULong;
#   else */ /* HP_HPUX */
# if defined( DEC_TRU64 )
typedef char SChar;
typedef unsigned char UChar;
typedef short SShort;
typedef unsigned short UShort;
typedef int SInt;
typedef unsigned int UInt;
typedef long SLong;
typedef unsigned long ULong;
# else /* DEC_TRU64 */
# if defined( IBM_AIX )
typedef char SChar;
typedef unsigned char UChar;
typedef short SShort;
typedef unsigned short UShort;
typedef int SInt;
typedef unsigned int UInt;
typedef long long SLong;
typedef unsigned long long ULong;
# else /* defined( IBM_AIX ) */
# if defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5)
typedef char SChar;
typedef unsigned char UChar;
typedef short SShort;
typedef unsigned short UShort;
typedef int SInt;
typedef unsigned int UInt;
typedef long long SLong;
typedef unsigned long long ULong;
# else /* defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
#   if defined(__GNUG__)
typedef char     SChar;   /* Signed    8-bits */
#  else /* defined(__GNUG__) */
typedef int8_t   SChar;   /* Signed    8-bits */
#  endif /* defined(HP_GCC_332) */
typedef uint8_t  UChar;   /* Unsigned  8-bits */
typedef int16_t  SShort;  /* Signed   16-bits */
typedef uint16_t UShort;  /* Unsigned 16-bits */
typedef int32_t  SInt;    /* Signed   32-bits */
typedef uint32_t UInt;    /* Unsigned 32-bits */
typedef int64_t  SLong;   /* Signed   64-bits */
typedef uint64_t ULong;   /* Unsigned 64-bits */
# endif /* defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
# endif /* defined( IBM_AIX ) && defined( __GNUC__ ) */
# endif   /* DEC_TRU64 */
/*# endif */ /* HP_HPUX */
# endif   /* INTEL_LINUX */

/*
 * vSLong, vULong은 컴파일 환경 32, 64 비트의 크기에 맞게
 * 조절됨.
 */

# ifdef COMPILE_64BIT
# ifdef DEC_TRU64
typedef long          vSLong;  /* variable Signed   32,[64]-bits */
typedef unsigned long vULong;  /* variable Unsigned 32,[64]-bits */
# else /* DEC_TRU64 */
#   if __CSURF__
typedef long long          vSLong;  /* variable Signed   32,[64]-bits */
typedef unsigned long long vULong;  /* variable Unsigned 32,[64]-bits */
#   else
#     if !defined(POWERPC64_LINUX)
typedef int64_t       vSLong;  /* variable Signed   32,[64]-bits */
typedef uint64_t      vULong;  /* variable Unsigned 32,[64]-bits */
#     else
typedef ULong         vULong;
#     endif /* POWERPC64_LINUX */
#   endif /* _CSURF__  */
# endif   /* DEC_TRU64 */
# else /* COMPILE_64BIT */
# ifdef DEC_TRU64
typedef long          vSLong;  /* variable Signed   32,[64]-bits */
typedef unsigned long vULong;  /* variable Unsigned 32,[64]-bits */
# else /* DEC_TRU64 */
# if defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5)
typedef long          vSLong;  /* variable Signed   32,[64]-bits */
typedef unsigned long vULong;  /* variable Unsigned 32,[64]-bits */
# elif defined(POWERPC64_LINUX)
typedef LONGLONG vSLong;  /* variable Signed   32,[64]-bits */
# elif defined(CYGWIN32)
#  include <sys/types.h>
typedef __int32_t     vSLong;  /* variable Signed   [32],64-bits */
typedef __uint32_t    vULong;  /* variable Unsigned [32],64-bits */
# else /* defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
typedef int32_t       vSLong;  /* variable Signed   [32],64-bits */
typedef uint32_t      vULong;  /* variable Unsigned [32],64-bits */
# endif /* defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
# endif   /* DEC_TRU64 */
# endif   /* COMPILE_64BIT */

# endif   /* VC_WIN32 | VC_WIN64 */

/* ------------------------------------------------------------
 *            64비트 Literal 정의시 사용됨
 * (컴파일러 환경에 따라 조건적으로 정의)
 * ex) #define MYLONG   0x1234567812345678 (틀림)
 *     #define MYLONG   ID_ULONG(0x1234567812345678) (맞음)
 * ------------------------------------------------------------*/

/* 1. Windows 환경 (ULL 이 없어야 함) */
# if defined(VC_WIN32) || defined(VC_WIN64)
 
#define ID_LONG(a)  a ## i64
#define ID_ULONG(a) a ## ui64

# elif defined( __GNUC__ ) /* 2. GNU 컴파일러 (ULL이 있어야 함) */

#define ID_LONG(a)  a ## LL
#define ID_ULONG(a) a ## ULL

# else                     /* 3. 기타 컴파일러 : 추가영역 */

#define ID_LONG(a)  a ## LL
#define ID_ULONG(a) a ## ULL

# endif

#define ID_vLONG(a)  ((vSLong)(ID_LONG(a)))
#define ID_vULONG(a) ((vULong)(ID_ULONG(a)))

/* printf formats for pointer-sized integers */
#ifdef _IA64_		/* 64-bit NT */
#define ID_POINTER_FMT	"I64d"
#define ID_UPOINTER_FMT	"I64u"
#define ID_xPOINTER_FMT	"I64x"
#define ID_XPOINTER_FMT	"I64X"
#else /* _IA64_ */
#ifdef VC_WIN32		/* 32-bit NT */
#define ID_POINTER_FMT	"ld"
#define ID_UPOINTER_FMT	"lu"
#define ID_xPOINTER_FMT	"lx"
#define ID_XPOINTER_FMT	"lX"
#else			    /* 32 and 64-bit UNIX */
#define ID_POINTER_FMT	"ld"
#define ID_UPOINTER_FMT	"lu"
#define ID_xPOINTER_FMT	"lx"
#define ID_XPOINTER_FMT	"lX"
#endif
#endif /* _IA64_ */

/* printf formats for 4-byte integers */
#if defined(DEC_TRU64) || ((defined(HP_HPUX) ||defined(IA64_HP_HPUX)) && defined(COMPILE_64BIT)) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) || defined(X86_64_DARWIN) || (defined(X86_SOLARIS) && defined(COMPILE_64BIT)) 
#define ID_INT32_FMT    "d"
#define ID_UINT32_FMT   "u"
#elif defined(INTEL_LINUX) || defined(POWERPC_LINUX) || defined(ITRON) || defined(WRS_VXWORKS) || defined(SYMBIAN)
#define ID_INT32_FMT    "d"
#define ID_UINT32_FMT   "u"
#else
#define ID_INT32_FMT    "ld"
#define ID_UINT32_FMT   "lu"
#endif
   
/* printf formats for 8-byte integers */
#ifdef VC_WIN32		/* 32 and 64-bit NT */
#define ID_INT64_FMT	"I64d"
#define ID_UINT64_FMT	"I64u"
#define ID_xINT64_FMT	"I64x"
#define ID_XINT64_FMT	"I64X"
#elif defined(DEC_TRU64) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(ALPHA_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) || defined(X86_64_DARWIN) || (defined(X86_SOLARIS) && defined(COMPILE_64BIT))
#define ID_INT64_FMT	"ld"
#define ID_UINT64_FMT	"lu"
#define ID_xINT64_FMT	"lx"
#define ID_XINT64_FMT	"lX"
#else				/* 32 and 64-bit UNIX */
#define ID_INT64_FMT	"lld"
#define ID_UINT64_FMT	"llu"
#define ID_xINT64_FMT	"llx"
#define ID_XINT64_FMT	"llX"
#endif

#ifdef COMPILE_64BIT
#define ID_vULONG_FMT  ID_UINT64_FMT 
#define ID_vxULONG_FMT ID_xINT64_FMT 
#define ID_vXULONG_FMT ID_XINT64_FMT 
#define ID_vSLONG_FMT  ID_INT64_FMT 
#define ID_vxSLONG_FMT ID_xINT64_FMT 
#define ID_vXSLONG_FMT ID_XINT64_FMT 
#else
#define ID_vULONG_FMT ID_UINT32_FMT
#define ID_vxULONG_FMT "x"
#define ID_vXULONG_FMT "X"
#define ID_vSLONG_FMT  "d"
#define ID_vxSLONG_FMT "x"
#define ID_vXSLONG_FMT "X"
#endif

#define ID_xINT32_FMT  "x"
#define ID_XINT32_FMT  "X"

/* printf formats for floating-point data types */
/* these formats are platform-independent */
#define ID_DOUBLE_G_FMT     ".15G"
#define ID_PRINT_G_FMT      ".12G"
#define ID_FLOAT_G_FMT       ".7G"

#define ID_UCHAR_MAX    ((UChar)0xFF)
#define ID_USHORT_MAX   ((UShort)0xFFFF)
#define ID_UINT_MAX     ((UInt)0xFFFFFFFF)
#define ID_ULONG_MAX    ((ULong)ID_ULONG(0xFFFFFFFFFFFFFFFF))

#define ID_SCHAR_MAX    ((SChar)0x7F)
#define ID_SSHORT_MAX   ((SShort)0x7FFF)
#define ID_SINT_MAX     ((SInt)0x7FFFFFFF)
#define ID_SLONG_MAX    ((SLong)ID_LONG(0x7FFFFFFFFFFFFFFF))

#define ID_SCHAR_MIN    (-(ID_SCHAR_MAX) - 1)
#define ID_SSHORT_MIN   (-(ID_SSHORT_MAX) - 1)
#define ID_SINT_MIN     (-(ID_SINT_MAX) - 1)
#define ID_SLONG_MIN    (-(ID_SLONG_MAX) - ID_LONG(1))

#ifdef COMPILE_64BIT
#define ID_vULONG_MAX (ID_ULONG(0xFFFFFFFFFFFFFFFF))
#define ID_vSLONG_MAX (ID_LONG(0x7FFFFFFFFFFFFFFF))
#else
#define ID_vULONG_MAX (0xFFFFFFFF)
#define ID_vSLONG_MAX (0x7FFFFFFF)
#endif

typedef enum idBool
{
    ID_FALSE = 0,
    ID_TRUE  = 1
} idBool;

typedef enum IDE_RC
{
    IDE_FAILURE = -1,
    IDE_SUCCESS =  0,
    IDE_CM_STOP =  1
} IDE_RC;

// Shared Memory의 상대 주소를 알아내기 위한 Type
typedef ULong  idShmAddr;

typedef UInt   idLPID;
typedef UInt   idLclThrID;
typedef UInt   idGblThrID;


/* ------------------------------------------------
 *  BUGBUG : Float, Double을 위한 임시 설정
 *  나중에는 각 플랫폼 영역에 적절하게 정의해야 함.
 * ----------------------------------------------*/

typedef float  SFloat;
typedef double SDouble;

typedef struct idMBR
{
    SDouble minX;
    SDouble minY;
    SDouble maxX;
    SDouble maxY;
} idMBR;


/*
 * XA XID
 */
/* BUG-18981 */    
#define ID_MAXXIDDATASIZE  128      /* size in bytes */
#define ID_MAXGTRIDSIZE    64      /* maximum size in bytes of gtrid */
#define ID_MAXBQUALSIZE    64      /* maximum size in bytes of bqual */


#define ID_XID_DATA_MAX_LEN 256
#define ID_GTRIDSIZE    18      
#define ID_BQUALSIZE    4      
#define ID_XIDDATASIZE  (ID_GTRIDSIZE + ID_BQUALSIZE)

/*
 * fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
 * 그들간의 관계를 정확히 유지해야 함.
 */
#define ID_NULL_SESSION_ID  ID_UINT_MAX
#define ID_NULL_TRANS_ID    ID_UINT_MAX
struct id_xid_t
{
    vSLong formatID;            /* format identifier */
    vSLong gtrid_length;        /* value from 1 through 64 */
    vSLong bqual_length;        /* value from 1 through 64 */
    SChar  data[ID_MAXXIDDATASIZE];
};

typedef struct id_xid_t ID_XID;

/* fix PROJ-1749 */
typedef enum
{
    IDU_SINGLE_TYPE = 0,
    IDU_SERVER_TYPE,
    IDU_CLIENT_TYPE,
    IDU_MAX_TYPE
} iduPeerType;

#if defined(POWERPC64_LINUX)
#define ID_SIZEOF(a) ((ULong)sizeof(a))
#else
#define ID_SIZEOF(a) (sizeof(a))
#endif

#define ID_ARR_ELEM_CNT(x) (ID_SIZEOF((x)) / ID_SIZEOF((x)[0]))  /* BUG-45553 */

typedef struct id_host_id_t
{
    UChar   *mHostID;
    UInt     mSize;
} ID_HOST_ID;

#define ID_MAX_HOST_ID_NUM 256
#define ID_MAX_HOST_NAME_LEN 29

/* BUG-21307: VS6.0에서 Compile Error발생.
 *
 * ULong이 double로 casting시 win32에서 에러 발생 */
#ifdef _MSC_VER
/* Conversion from unsigned __int64 to double is not implemented in windows
 * and results in a compile error, thus the value must first be cast to
 * signed __int64, and then to double.
 *
 * If the 64 bit int value is greater than 2^63-1, which is the signed int64 max,
 * the macro below provides a workaround for casting a uint64 value to a double
 * in windows.
 */
#  define UINT64_TO_DOUBLE(u) ( ((u) > _I64_MAX) ? \
            (SDouble)(SLong)((u) - _I64_MAX - 1) + (SDouble)_I64_MAX + 1: \
            (SDouble)(SLong)(u) )

/* The largest double value that can be cast to uint64 in windows is the
 * signed int64 max, which is 2^63-1. The macro below provides
 * a workaround for casting large double values to uint64 in windows.
 */
#  define DOUBLE_TO_UINT64(d) ( ((d) > 0xffffffffffffffffu) ? \
            (ULong) 0xffffffffffffffffu : \
            ((d) < 0) ? (ULong) 0 : \
            ((d) > _I64_MAX) ? \
            (ULong) ((d) - _I64_MAX) - 1 + (ULong)_I64_MAX + 1: \
            (ULong)(d) )
#else
#  define UINT64_TO_DOUBLE(u) ((SDouble)(u))
#  if defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__TICCSC__)
/* double_to_uint64 defined only for MSVC and UNIX */
#  else
#  define DOUBLE_TO_UINT64(d) ( ((d) > 0xffffffffffffffffLLU) ? \
            (ULong) 0xffffffffffffffffLLU : \
            ((d) < 0) ? (ULong) 0 : (ULong)(d) )
#  endif
#endif

#define ID_1_BYTE_ASSIGN(dst, src){           \
     *((UChar*) (dst))    = *((UChar*) (src));     \
}

#define ID_2_BYTE_ASSIGN(dst, src){           \
     *((UChar*) (dst))    = *((UChar*) (src));     \
     *(((UChar*)(dst))+1) = *(((UChar*)(src))+1); \
}

#define ID_4_BYTE_ASSIGN(dst, src){           \
     *((UChar*) (dst))    = *((UChar*) (src));     \
     *(((UChar*)(dst))+1) = *(((UChar*)(src))+1); \
     *(((UChar*)(dst))+2) = *(((UChar*)(src))+2); \
     *(((UChar*)(dst))+3) = *(((UChar*)(src))+3); \
}

#define ID_8_BYTE_ASSIGN(dst, src){           \
     *((UChar*) (dst))    = *((UChar*) (src));     \
     *(((UChar*)(dst))+1) = *(((UChar*)(src))+1); \
     *(((UChar*)(dst))+2) = *(((UChar*)(src))+2); \
     *(((UChar*)(dst))+3) = *(((UChar*)(src))+3); \
     *(((UChar*)(dst))+4) = *(((UChar*)(src))+4); \
     *(((UChar*)(dst))+5) = *(((UChar*)(src))+5); \
     *(((UChar*)(dst))+6) = *(((UChar*)(src))+6); \
     *(((UChar*)(dst))+7) = *(((UChar*)(src))+7); \
}

#define ID_CHAR_BYTE_ASSIGN   ID_1_BYTE_ASSIGN
#define ID_SHORT_BYTE_ASSIGN  ID_2_BYTE_ASSIGN
#define ID_INT_BYTE_ASSIGN    ID_4_BYTE_ASSIGN
#define ID_FLOAT_BYTE_ASSIGN  ID_4_BYTE_ASSIGN
#define ID_DOUBLE_BYTE_ASSIGN ID_8_BYTE_ASSIGN
#define ID_LONG_BYTE_ASSIGN   ID_8_BYTE_ASSIGN


#define ID_WRITE_VALUE(aDest, aSrc, aSize)                      \
    IDE_DASSERT((aSize)  > 0);                                  \
    idlOS::memcpy( (void*)(aDest), (void*)(aSrc), (aSize) )

#define ID_WRITE_AND_MOVE_DEST(aDest, aSrc, aSize)              \
    IDE_DASSERT( ID_SIZEOF(*(aDest)) == 1 );                    \
    ID_WRITE_VALUE(aDest, aSrc, aSize);                         \
    (aDest) += (aSize)

#define ID_WRITE_AND_MOVE_SRC(aDest, aSrc, aSize)               \
    IDE_DASSERT( ID_SIZEOF(*(aSrc)) == 1 );                     \
    ID_WRITE_VALUE(aDest, aSrc, aSize);                         \
    (aSrc)  += (aSize)

#define ID_WRITE_AND_MOVE_BOTH(aDest, aSrc, aSize)              \
    IDE_DASSERT( ID_SIZEOF(*(aDest)) == 1 );                    \
    IDE_DASSERT( ID_SIZEOF(*(aSrc))  == 1 );                    \
    ID_WRITE_VALUE(aDest, aSrc, aSize);                         \
    (aDest) += (aSize);                                         \
    (aSrc)  += (aSize)



#define ID_WRITE_1B_VALUE(aPtr, aVal)                           \
    IDE_DASSERT((aPtr) != NULL);                                \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    IDE_DASSERT( ID_SIZEOF(*(aVal)) == 1 );                     \
    ID_1_BYTE_ASSIGN( aPtr, aVal );

#define ID_WRITE_1B_AND_MOVE_DEST(aPtr, aVal)                   \
    ID_WRITE_1B_VALUE(aPtr, aVal);                              \
    (aPtr) += (1)



#define ID_WRITE_2B_VALUE(aPtr, aVal)                           \
    IDE_DASSERT((aPtr) != NULL);                                \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    IDE_DASSERT( ID_SIZEOF(*(aVal)) == 2 );                     \
    ID_2_BYTE_ASSIGN( aPtr, aVal );

#define ID_WRITE_2B_AND_MOVE_DEST(aPtr, aVal)                   \
    ID_WRITE_2B_VALUE(aPtr, aVal);                              \
    (aPtr) += (2)



#define ID_READ_VALUE(aPtr, aRet, aSize)                        \
    IDE_DASSERT((aSize) > 0);                                   \
    idlOS::memcpy( (void*)(aRet), (void*)(aPtr), (aSize) )

#define ID_READ_AND_MOVE_PTR(aPtr, aRet, aSize)                 \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    ID_READ_VALUE(aPtr, aRet, aSize);                           \
    (aPtr) += (aSize)



#define ID_READ_1B_VALUE(aPtr, aRet)                            \
    IDE_DASSERT((aPtr) != NULL);                                \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    IDE_DASSERT( ID_SIZEOF(*(aRet)) == 1 );                     \
    ID_1_BYTE_ASSIGN( aRet, aPtr );

#define ID_READ_1B_AND_MOVE_PTR(aPtr, aRet)                     \
    ID_READ_1B_VALUE(aPtr, aRet);                               \
    (aPtr) += (1)


#define ID_READ_2B_VALUE(aPtr, aRet)                            \
    IDE_DASSERT((aPtr) != NULL);                                \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    IDE_DASSERT( ID_SIZEOF(*(aRet)) == 2 );                     \
    ID_2_BYTE_ASSIGN( aRet, aPtr );

#define ID_READ_2B_AND_MOVE_PTR(aPtr, aRet)                     \
    ID_READ_2B_VALUE(aPtr, aRet);                               \
    (aPtr) += (2)

#define ID_READ_4B_VALUE(aPtr, aRet)                            \
    IDE_DASSERT((aPtr) != NULL);                                \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    IDE_DASSERT( ID_SIZEOF(*(aRet)) == 4 );                     \
    ID_4_BYTE_ASSIGN( aRet, aPtr );

#define ID_READ_4B_AND_MOVE_PTR(aPtr, aRet)                     \
    ID_READ_4B_VALUE(aPtr, aRet);                               \
    (aPtr) += (4)


#define ID_READ_8B_VALUE(aPtr, aRet)                            \
    IDE_DASSERT((aPtr) != NULL);                                \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    IDE_DASSERT( ID_SIZEOF(*(aRet)) == 8 );                     \
    ID_8_BYTE_ASSIGN( aRet, aPtr );

#define ID_READ_8B_AND_MOVE_PTR(aPtr, aRet)                     \
    ID_READ_8B_VALUE(aPtr, aRet);                               \
    (aPtr) += (8)

#if !defined(PATH_MAX)
# define ID_MAX_FILE_NAME              1024
#else
# define ID_MAX_FILE_NAME              PATH_MAX
#endif

#if defined(VC_WIN32)
# define IDTHREAD __declspec(thread)
#else
# define IDTHREAD __thread
#endif

#endif /* _O_IDTYPES_H_ */
