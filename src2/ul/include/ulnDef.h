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

#ifndef _O_ULNDEF_H_
#define _O_ULNDEF_H_ 1

#include <idConfig.h>

/* SQLLEN LENGTH
 * ----------------------------------------------------------------
 * | FLAG                         |   64Bit ENV   |   32Bit ENV   |
 * ----------------------------------------------------------------
 * | BUILD_REAL_64_BIT_MODE [ON]  |     64Bit     |     32Bit     |
 * ----------------------------------------------------------------
 * | BUILD_REAL_64_BIT_MODE [OFF] |     32Bit     |     32Bit     |
 * ----------------------------------------------------------------
 */

#if defined(BUILD_REAL_64_BIT_MODE)

#define ulvSLen     acp_sint64_t
#define ulvULen     acp_uint64_t

# if defined( __GNUC__ ) /* GNU 컴파일러 (ULL이 있어야 함) */

#define ULN_LEN(a)  a ## LL
#define ULN_ULEN(a) a ## ULL

# else /* 기타 컴파일러 : 추가영역 */

#define ULN_LEN(a)  a ## LL
#define ULN_ULEN(a) a ## ULL

# endif

#else

#define ulvSLen     acp_sint32_t
#define ulvULen     acp_uint32_t

#define ULN_LEN(a)  a
#define ULN_ULEN(a) a

#endif


#define ULN_vLEN(a)  ((ulvSLen)(ULN_LEN(a)))
#define ULN_vULEN(a) ((ulvULen)(ULN_ULEN(a)))

#ifdef SQL_WCHART_CONVERT
#define ulWChar  wchar_t
#else
#define ulWChar  acp_uint16_t
#endif


/* PROJ-2177 User Interface - Cancel */

#define ULN_SESS_ID_NONE    0
#define ULN_STMT_ID_NONE    0
#define ULN_STMT_CID_NONE   0

/* BUG-37101 */

#define ULN_NUMERIC_MAX_PRECISION   38
#define ULN_NUMERIC_UNDEF_SCALE     ((acp_sint16_t)0x8000)
#define ULN_NUMERIC_UNDEF_PRECISION ((acp_sint16_t)0x8000)

/* bug-35142 cli trace log
   hex dump 시 길이 제한 */
#define ULN_TRACE_LOG_MAX_DATA_LEN  30

/* BUG-39817 */
/* SMI_ISOLATION_CONSISTENT */
#define ULN_SERVER_ISOLATION_LEVEL_READ_COMMITTED  (0x00000000)
/* SMI_ISOLATION_REPEATABLE */
#define ULN_SERVER_ISOLATION_LEVEL_REPEATABLE_READ (0x00000001)
/* SMI_ISOLATION_NO_PHANTOM */
#define ULN_SERVER_ISOLATION_LEVEL_SERIALIZABLE    (0x00000002)

/**
 * PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
 *
 *   ULN_CNN_SOCK_RCVBUF_BLOCK_RATIO_MAX(2^16) =
 *       maximum socket receive buffer size(2^31) / CM block size(32K)
 */
#define ULN_CONN_SOCK_RCVBUF_BLOCK_RATIO_MIN                  (0)
#define ULN_CONN_SOCK_RCVBUF_BLOCK_RATIO_MAX                  (ACP_UINT16_MAX)
#define ULN_CONN_SOCK_RCVBUF_BLOCK_RATIO_DEFAULT              (0)
#define ULN_SOCK_RCVBUF_BLOCK_RATIO_DEFAULT                   (2)

#define ULN_STMT_PREFETCH_ASYNC_DEFAULT                       ALTIBASE_PREFETCH_ASYNC_OFF
#if defined(ALTI_CFG_OS_LINUX)
#define ULN_STMT_PREFETCH_AUTO_TUNING_DEFAULT                 ACP_TRUE
#else /* ALTI_CFG_OS_LINUX */
#define ULN_STMT_PREFETCH_AUTO_TUNING_DEFAULT                 ACP_FALSE
#endif /* ALTI_CFG_OS_LINUX */

#define ULN_STMT_PREFETCH_AUTO_TUNING_MIN_DEFAULT             (1)
#define ULN_STMT_PREFETCH_AUTO_TUNING_MAX_DEFAULT             (ACP_UINT32_MAX)

#endif /* _O_ULNDEF_H_ */
