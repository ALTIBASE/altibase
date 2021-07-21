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
 

/***********************************************************************
 * $Id: rpDef.h 90177 2021-03-11 04:29:45Z lswhh $
 **********************************************************************/

#ifndef _O_RP_DEF_H_
#define _O_RP_DEF_H_ 1

#include <idl.h>

#define RP_STATUS_NONE       (0)
#define RP_STATUS_TX_BEGIN   (1 << 0)
#define RP_STATUS_STMT_BEGIN (1 << 1)
#define RP_STATUS_CUR_OPEN   (1 << 2)

#define RP_SET_STATUS_INIT(st)       {  (st) = RP_STATUS_NONE; }
#define RP_SET_STATUS_TX_BEGIN(st)   {  (st) |= RP_STATUS_TX_BEGIN; }
#define RP_SET_STATUS_STMT_BEGIN(st) {  (st) |= RP_STATUS_STMT_BEGIN; }
#define RP_SET_STATUS_CUR_OPEN(st)   {  (st) |= RP_STATUS_CUR_OPEN; }

#define RP_SET_STATUS_TX_END(st)     {  (st) &= ~RP_STATUS_TX_BEGIN; }
#define RP_SET_STATUS_STMT_END(st)   {  (st) &= ~RP_STATUS_STMT_BEGIN; }
#define RP_SET_STATUS_CUR_CLOSE(st)  {  (st) &= ~RP_STATUS_CUR_OPEN; }

#define RP_IS_STATUS_TX_BEGIN(st)    ( (st) & RP_STATUS_TX_BEGIN )
#define RP_IS_STATUS_STMT_BEGIN(st)  ( (st) & RP_STATUS_STMT_BEGIN )
#define RP_IS_STATUS_CUR_OPEN(st)    ( (st) & RP_STATUS_CUR_OPEN )

#define RP_STATUS_VALID                                 0
#define RP_STATUS_INVALID_CURSOR_OPEN_WITHOUT_STMT_OPEN 1
#define RP_STATUS_INVALID_CURSOR_OPEN_WITHOUT_TX_BEGIN  2
#define RP_STATUS_INVALUD_STMT_BEGIN_WITHOUT_TX_BEGIN   3

#define RP_GET_INVALID_STATUS_CODE(st)                            \
(                                                                  \
    ( (st) & RP_STATUS_CUR_OPEN) ?                                \
         ( (st) & RP_STATUS_STMT_OPEN ) ?                         \
             ( (st) & RP_STATUS_TX_BEGIN ) ?                      \
                 (RP_STATUS_VALID)                                \
             :                                                     \
                 (RP_STATUS_INVALID_CURSOR_OPEN_WITHOUT_TX_BEGIN) \
         :                                                         \
             (RP_STATUS_INVALID_CURSOR_OPEN_WITHOUT_STMT_OPEN)    \
    :                                                              \
         ( (st) & RP_STATUS_STMT_BEGIN ) ?                        \
             ( (st) & RP_STATUS_TX_BEGIN ) ?                      \
                 (RP_STATUS_VALID)                                \
             :                                                     \
             (RP_STATUS_INVALUD_STMT_BEGIN_WITHOUT_TX_BEGIN)      \
         :                                                         \
         (RP_STATUS_VALID)                                        \
)        

#define RP_TEST_STATUS_VALID(st, err_label)                       \
{                                                                  \
    if ( RP_GET_INVALID_STATUS_CODE(st) != RP_STATUS_VALID )     \
    {                                                              \
        IDE_RAISE(err_label);                                      \
    }                                                              \
}

#define RP_GET_STATUS_MESSAGE(st)                                      \
(                                                                       \
    ( RP_GET_INVALID_STATUS_CODE((st)) ==                              \
        RP_STATUS_INVALID_CURSOR_OPEN_WITHOUT_STMT_OPEN ) ?            \
        ("Cursor is open with statement closed")                        \
    :                                                                   \
        ( RP_GET_INVALID_STATUS_CODE((st)) ==                          \
            RP_STATUS_INVALID_CURSOR_OPEN_WITHOUT_TX_BEGIN ) ?         \
            ("Cursor is open with transaction inactive")                \
        :                                                               \
            ( RP_GET_INVALID_STATUS_CODE((st)) ==                      \
                RP_STATUS_INVALUD_STMT_BEGIN_WITHOUT_TX_BEGIN ) ?      \
                ("Statement is open with transaction inactive")         \
            :                                                           \
                ( RP_GET_INVALID_STATUS_CODE((st)) ==                  \
                     RP_STATUS_VALID ) ?                               \
                     ("Everything is valid (Internal server error).")   \
                :                                                       \
                     ("Internal server error - Invalid status." )       \
)

typedef enum
{
    RP_START_RECV_OK = 0,
    RP_START_RECV_NETWORK_ERROR,
    RP_START_RECV_INVALID_VERSION,
    RP_START_RECV_HBT_OCCURRED,
    
    RP_START_RECV_MAX_MAX
} rpRecvStartStatus;

#define RP_SENDER_APPLY_CHECK_INTERVAL 10 //10 sec
#define RP_UNUSED_RECEIVER_INDEX (SMX_LOCK_WAIT_REPL_TX_ID)

/* for xlogfileMgr */
#define XLOGFILE_PATH_MAX_LENGTH    (1024)
#define XLOGFILE_HEADER_SIZE_INIT   (UInt)(ID_SIZEOF(rpdXLogfileHeader))

typedef ULong rpXLogLSN;

#define RP_XLOGLSN_OFFSET_BIT_SIZE    (ID_ULONG(32))
#define RP_XLOGLSN_OFFSET_MASK        (ID_UINT_MAX)
#define RP_XLOGLSN_INIT               (0)

/* FileNo와 Offset으로 XLogLSN을 만든다. */
#define RP_SET_XLOGLSN( FILENO, OFFSET )               \
        ( ( ( rpXLogLSN )FILENO << RP_XLOGLSN_OFFSET_BIT_SIZE ) | OFFSET )

/* XLogLSN에서 FileNo와 Offset을 추출한다. */
#define RP_GET_XLOGLSN( FILENO, OFFSET, XLOGLSN )   \
        { FILENO = (rpXLogLSN)XLOGLSN >> RP_XLOGLSN_OFFSET_BIT_SIZE; \
          OFFSET = (rpXLogLSN)XLOGLSN & RP_XLOGLSN_OFFSET_MASK; }

/* XLogLSN에서 FileNo를 추출한다. */
#define RP_GET_FILENO_FROM_XLOGLSN( FILENO, XLOGLSN )   \
        FILENO = (rpXLogLSN)XLOGLSN >> RP_XLOGLSN_OFFSET_BIT_SIZE;

/* XLogLSN에서 Offset를 추출한다. */
#define RP_GET_OFFSET_FROM_XLOGLSN( OFFSET, XLOGLSN )   \
        OFFSET = (rpXLogLSN)XLOGLSN & RP_XLOGLSN_OFFSET_MASK;

#define RP_IS_INIT_XLOGLSN( XLOGLSN )   ((XLOGLSN) == RP_XLOGLSN_INIT )

#define RP_ENDIAN_ASSIGN2(dst, src)                    \
    do                                                 \
{                                                  \
    *((UChar *)(dst) + 1) = *((UChar *)(src) + 0); \
    *((UChar *)(dst) + 0) = *((UChar *)(src) + 1); \
} while (0)

#define RP_ENDIAN_ASSIGN4(dst, src)                    \
    do                                                 \
{                                                  \
    *((UChar *)(dst) + 3) = *((UChar *)(src) + 0); \
    *((UChar *)(dst) + 2) = *((UChar *)(src) + 1); \
    *((UChar *)(dst) + 1) = *((UChar *)(src) + 2); \
    *((UChar *)(dst) + 0) = *((UChar *)(src) + 3); \
} while (0)

#define RP_ENDIAN_ASSIGN8(dst, src)                    \
    do                                                 \
{                                                  \
    *((UChar *)(dst) + 7) = *((UChar *)(src) + 0); \
    *((UChar *)(dst) + 6) = *((UChar *)(src) + 1); \
    *((UChar *)(dst) + 5) = *((UChar *)(src) + 2); \
    *((UChar *)(dst) + 4) = *((UChar *)(src) + 3); \
    *((UChar *)(dst) + 3) = *((UChar *)(src) + 4); \
    *((UChar *)(dst) + 2) = *((UChar *)(src) + 5); \
    *((UChar *)(dst) + 1) = *((UChar *)(src) + 6); \
    *((UChar *)(dst) + 0) = *((UChar *)(src) + 7); \
} while (0)

#endif /* _O_RP_DEF_H_ */
