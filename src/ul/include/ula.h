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
 
#ifndef _O_ULA_COMMON_H_
#define _O_ULA_COMMON_H_ 1

#include <ulaErrorMgr.h>

/* SM Definition */
#define ULA_SN_NULL (ACP_UINT64_MAX)
#define ULA_SN_MAX  (ACP_UINT64_MAX - 1)

typedef acp_uint32_t  ulaTID;          // smTID
typedef acp_uint64_t  ulaSN;           // smSN
typedef acp_uint64_t  ulaLobLocator;   // smLobLocator
typedef struct ulaValue                // smiValue
{
    acp_uint32_t  length;
    void         *value;
} ulaValue;


/* QP definition */

/* RP definition */
#define RP_MAKE_VERSION( major, minor, fix, endian )   \
    ( (((acp_uint64_t)((acp_uint16_t)(major)  & 0xFFFF)) << 48) | \
      (((acp_uint64_t)((acp_uint16_t)(minor)  & 0xFFFF)) << 32) | \
      (((acp_uint64_t)((acp_uint16_t)(fix)    & 0xFFFF)) << 16) | \
      (((acp_uint64_t)((acp_uint16_t)(endian) & 0xFFFF)))  )

#define REPLICATION_MAJOR_VERSION   (7)
#define REPLICATION_MINOR_VERSION   (4)
#define REPLICATION_FIX_VERSION     (8)

#define RP_CURRENT_VERSION ( RP_MAKE_VERSION( REPLICATION_MAJOR_VERSION, \
                                              REPLICATION_MINOR_VERSION, \
                                              REPLICATION_FIX_VERSION, \
                                              REPLICATION_ENDIAN_64BIT ) )

#define ULA_NAME_LEN                (40 + 1)
#define ULA_LOG_FILENAME_LEN        (1024)

#define ULA_SOCKET_TYPE_LEN         (4 + 1)
#define ULA_IP_LEN                  (39 + 1)
#define ULA_PORT_LEN                (5 + 1)
#define ULA_SOCK_NAME_LEN           (250 + 1) /* UNIX Socket filename len */
#define ULA_IP_STACK_LEN            (5)

#define ULA_UNUSED_SN               (0)


typedef enum
{
    ULA_X_NONE  = 0,
    ULA_X_BEGIN = 1,            // (Not Used) Transaction Begin
    ULA_X_COMMIT,               // Transaction Commit
    ULA_X_ABORT,                // Transaction rollback
    ULA_X_INSERT,               // DML: Insert
    ULA_X_UPDATE,               // DML: Update
    ULA_X_DELETE,               // DML: Delete
    ULA_X_IMPL_SP_SET,          // (Not Used) Implicit Savepoint set
    ULA_X_SP_SET,               // Savepoint Set
    ULA_X_SP_ABORT,             // Abort to savepoint
    ULA_X_STMT_BEGIN,           // (Not Used) Statement Begin
    ULA_X_STMT_END,             // (Not Used) Statement End
    ULA_X_CURSOR_OPEN,          // (Not Used) Cursor Open
    ULA_X_CURSOR_CLOSE,         // (Not Used) Cursor Close
    ULA_X_LOB_CURSOR_OPEN,      // LOB Cursor open
    ULA_X_LOB_CURSOR_CLOSE,     // LOB Cursor close
    ULA_X_LOB_PREPARE4WRITE,    // LOB Prepare for write
    ULA_X_LOB_PARTIAL_WRITE,    // LOB Partial write
    ULA_X_LOB_FINISH2WRITE,     // LOB Finish to write
    ULA_X_KEEP_ALIVE,           // Keep Alive
    ULA_X_ACK,                  // (Response Only) ACK
    ULA_X_REPL_STOP,            // Replication Stop
    ULA_X_FLUSH,                // (Not Used) Replication Flush
    ULA_X_FLUSH_ACK,            // (Not Used) Replication Flush ack
    ULA_X_STOP_ACK,             // (Response Only) Stop Ack
    ULA_X_HANDSHAKE,            // (Not Used in ALA) Handshake Request
    ULA_X_HANDSHAKE_READY,      // (Not Used in ALA) Handshake Ready
    ULA_X_MAX
} ulaXLogType;

typedef struct ulaXLogHeader        /* XLog Header */
{
    ulaXLogType   mType;          // XLog Type
    ulaTID        mTID;           // Transaction ID
    ulaSN         mSN;            // SN
    ulaSN         mSyncSN;        // Reserved
    ulaSN         mRestartSN;     // Restart SN
    acp_uint64_t  mTableOID;      // Table OID
} ulaXLogHeader;

typedef struct ulaXLogPrimaryKey    /* Primary Key */
{
    acp_uint32_t  mPKColCnt;      // Primary Key Column Count
    ulaValue     *mPKColArray;    // Priamry Key Column Value Array
} ulaXLogPrimaryKey;

typedef struct ulaXLogColumn        /* Column */
{
    acp_uint32_t  mColCnt;        // Column Count
    acp_uint32_t *mCIDArray;      // Column ID Array
    ulaValue     *mBColArray;     // Before Image Column Value Array (Only for Update)
    ulaValue     *mAColArray;     // After Image Column Value Array
} ulaXLogColumn;

typedef struct ulaXLogSavepoint     /* Savepoint */
{
    acp_uint32_t  mSPNameLen;     // Savepoint Name Length
    acp_char_t   *mSPName;        // Savepoint Name
} ulaXLogSavepoint;

typedef struct ulaXLogLOB           /* LOB (LOB Column does not have Before Image) */
{
    acp_uint64_t  mLobLocator;   // LOB Locator of Altibase
    acp_uint32_t  mLobColumnID;
    acp_uint32_t  mLobOffset;
    acp_uint32_t  mLobOldSize;
    acp_uint32_t  mLobNewSize;
    acp_uint32_t  mLobPieceLen;
    acp_uint8_t  *mLobPiece;
} ulaXLogLOB;

typedef struct ulaXLog              /* XLog */
{
    ulaXLogHeader       mHeader;
    ulaXLogPrimaryKey   mPrimaryKey;
    ulaXLogColumn       mColumn;
    ulaXLogSavepoint    mSavepoint;
    ulaXLogLOB          mLOB;

    // Used in XLog Linked List
    struct ulaXLog     *mPrev;
    struct ulaXLog     *mNext;
} ulaXLog;


/* Mutex Information
 *- Module -          - Mutex -   - Info -
 * ulaComm              No        호출하는 곳에서 동기화해야 함
 * ulaErrorMgr          No        Thread에 설정하므로 불필요
 * ulaLog               Yes       내부적으로 이미 Mutex 지원
 * ulaMeta              No        Handshake 중에만 변경
 * ulaTransTbl          Yes       Transaction Table, Collection List
 * ulaXLogCollection    Yes       Network Send/Receive,
 *                                XLog Pool, Authentication Information, ACK Info
 * ulaXLogLinkedLit     Support   XLog Queue에서 필요
 */

#endif /* _O_ULA_COMMON_H_ */
