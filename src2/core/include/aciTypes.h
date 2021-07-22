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
 
/***********************************************************************
 * $Id:
 **********************************************************************/


#ifndef  _O_ACITYPES_H_
# define _O_ACITYPES_H_ 1

#include <acpTypes.h>

ACP_EXTERN_C_BEGIN


typedef enum
{
    ACI_FAILURE = -1,
    ACI_SUCCESS =  0,
    ACI_CM_STOP =  1
} ACI_RC;


/* printf formats for pointer-sized integers */
#ifdef ALTI_CFG_OS_WINDOWS
#define ACI_POINTER_FMT	"ld"
#define ACI_UPOINTER_FMT	"lu"
#define ACI_xPOINTER_FMT	"lx"
#define ACI_XPOINTER_FMT	"lX"
#else			    /* 32 and 64-bit UNIX */
#define ACI_POINTER_FMT	"ld"
#define ACI_UPOINTER_FMT	"lu"
#define ACI_xPOINTER_FMT	"lx"
#define ACI_XPOINTER_FMT	"lX"
#endif


/* printf formats for 4-byte integers */
#define ACI_INT32_FMT    "d"
#define ACI_UINT32_FMT   "u"
#define ACI_xINT32_FMT  "x"
#define ACI_XINT32_FMT  "X"


/* printf formats for 8-byte integers */
#ifdef ALTI_CFG_OS_WINDOWS
#define ACI_INT64_FMT	"lld"
#define ACI_UINT64_FMT	"llu"
#define ACI_xINT64_FMT	"llx"
#define ACI_XINT64_FMT	"llX"
#else				/* 32 and 64-bit UNIX */
#define ACI_INT64_FMT	"lld"
#define ACI_UINT64_FMT	"llu"
#define ACI_xINT64_FMT	"llx"
#define ACI_XINT64_FMT	"llX"
#endif


#if defined(ACP_CFG_COMPILE_64BIT)
#define ACI_vULONG_FMT  ACI_UINT64_FMT 
#define ACI_vxULONG_FMT ACI_xINT64_FMT 
#define ACI_vXULONG_FMT ACI_XINT64_FMT 
#define ACI_vSLONG_FMT  ACI_INT64_FMT 
#define ACI_vxSLONG_FMT ACI_xINT64_FMT 
#define ACI_vXSLONG_FMT ACI_XINT64_FMT 
#else
#define ACI_vULONG_FMT ACI_UINT32_FMT
#define ACI_vxULONG_FMT "x"
#define ACI_vXULONG_FMT "X"
#define ACI_vSLONG_FMT  "d"
#define ACI_vxSLONG_FMT "x"
#define ACI_vXSLONG_FMT "X"
#endif

#define ACI_xINT32_FMT  "x"
#define ACI_XINT32_FMT  "X"

/* printf formats for floating-point data types */
/* these formats are platform-independent */
#define ACI_DOUBLE_G_FMT     ".15G"
#define ACI_PRINT_G_FMT      ".12G"
#define ACI_FLOAT_G_FMT       ".7G"


#define ACI_SIZEOF(a) (sizeof(a))

ACP_EXTERN_C_END

#endif /* _O_ACITYPES_H_ */
