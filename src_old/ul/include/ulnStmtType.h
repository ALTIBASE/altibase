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

#ifndef _O_ULN_STMT_TYPE_H_
#define _O_ULN_STMT_TYPE_H_ 1

/* qciStmtType.h */

/*
 *  Statement Bit Mask
 *   ID => 0000 0000
 *         ^^^^ ^^^^
 *         Type Index
 *
 *   Type : DDL, DML, DCL, SP, DB
 */
#define ULN_STMT_TYPE_MAX   16
#define ULN_STMT_MASK_MASK  (0x000F0000)
#define ULN_STMT_MASK_INDEX (0x0000FFFF)

enum ulnStmtKind
{
    ULN_STMT_MASK_DDL     = 0x00000000,
    ULN_STMT_MASK_DML     = 0x00010000,
    ULN_STMT_MASK_DCL     = 0x00020000,
    ULN_STMT_MASK_SP      = 0x00040000,
    ULN_STMT_MASK_DB      = 0x00050000,

    //----------------------------------------------------
    //  DDL
    //----------------------------------------------------

    ULN_STMT_SCHEMA_DDL = ULN_STMT_MASK_DDL,
    ULN_STMT_NON_SCHEMA_DDL,
    ULN_STMT_CRT_SP,
    ULN_STMT_COMMENT,
    ULN_STMT_SHARD_DDL,

    //----------------------------------------------------
    //  DML
    //----------------------------------------------------

    ULN_STMT_SELECT = ULN_STMT_MASK_DML,
    ULN_STMT_LOCK_TABLE,
    ULN_STMT_INSERT,
    ULN_STMT_UPDATE,
    ULN_STMT_DELETE,
    ULN_STMT_CHECK_SEQUENCE,
    ULN_STMT_MOVE,
    // Proj-1360 Queue
    ULN_STMT_ENQUEUE,
    ULN_STMT_DEQUEUE,
    // PROJ-1362
    ULN_STMT_SELECT_FOR_UPDATE,
    // PROJ-1988 Implement MERGE statement */
    ULN_STMT_MERGE,
    // BUG-42397 Ref Cursor Static SQL
    ULN_STMT_SELECT_FOR_CURSOR,

    //----------------------------------------------------
    //  DCL + SESSION + SYSTEM
    //----------------------------------------------------

    ULN_STMT_COMMIT = ULN_STMT_MASK_DCL,
    ULN_STMT_ROLLBACK,
    ULN_STMT_SAVEPOINT,
    ULN_STMT_SET_AUTOCOMMIT_TRUE,
    ULN_STMT_SET_AUTOCOMMIT_FALSE,
    ULN_STMT_SET_REPLICATION_MODE,
    ULN_STMT_SET_PLAN_DISPLAY_ONLY,
    ULN_STMT_SET_PLAN_DISPLAY_ON,
    ULN_STMT_SET_PLAN_DISPLAY_OFF,
    ULN_STMT_SET_TX,
    ULN_STMT_SET_STACK,
    ULN_STMT_SET,
    ULN_STMT_ALT_SYS_CHKPT,
    ULN_STMT_ALT_SYS_SHRINK_MEMPOOL,
    ULN_STMT_ALT_SYS_MEMORY_COMPACT,
    ULN_STMT_SET_SYSTEM_PROPERTY,
    ULN_STMT_SET_SESSION_PROPERTY,
    ULN_STMT_CHECK,
    ULN_STMT_ALT_SYS_REORGANIZE,
    ULN_STMT_ALT_SYS_VERIFY,
    ULN_STMT_ALT_SYS_ARCHIVELOG,
    ULN_STMT_ALT_TABLESPACE_CHKPT_PATH,
    ULN_STMT_ALT_TABLESPACE_DISCARD,
    ULN_STMT_ALT_DATAFILE_ONOFF,
    ULN_STMT_ALT_RENAME_DATAFILE,
    ULN_STMT_ALT_TABLESPACE_BACKUP,
    ULN_STMT_ALT_SYS_SWITCH_LOGFILE,
    ULN_STMT_ALT_SYS_FLUSH_BUFFER_POOL,
    ULN_STMT_CONNECT,
    ULN_STMT_DISCONNECT,
    ULN_STMT_ENABLE_PARALLEL_DML,
    ULN_STMT_DISABLE_PARALLEL_DML,
    ULN_STMT_COMMIT_FORCE,
    ULN_STMT_ROLLBACK_FORCE,

    // PROJ-1568 Buffer Manager Renewal
    ULN_STMT_FLUSHER_ONOFF,

    // PROJ-1436 SQL Plan Cache
    ULN_STMT_ALT_SYS_COMPACT_PLAN_CACHE,
    ULN_STMT_ALT_SYS_RESET_PLAN_CACHE,

    ULN_STMT_ALT_SYS_REBUILD_MIN_VIEWSCN,

    // PROJ-2002 Column Security
    ULN_STMT_ALT_SYS_SECURITY,

    /* PROJ-1832 New database link */
    ULN_STMT_CONTROL_DATABASE_LINKER,
    ULN_STMT_CLOSE_DATABASE_LINK,

    ULN_STMT_COMMIT_FORCE_DATABASE_LINK,
    ULN_STMT_ROLLBACK_FORCE_DATABASE_LINK,

    // PROJ-2223 Audit
    ULN_STMT_ALT_SYS_AUDIT,
    ULN_STMT_AUDIT_OPTION,
    ULN_STMT_NOAUDIT_OPTION,
    /* BUG-39074 */
    ULN_STMT_DELAUDIT_OPTION, 

    /* BUG-42852 STOP과 FLUSH를 DCL로 변환합니다. */
    ULN_STMT_ALT_REPLICATION_STOP,
    ULN_STMT_ALT_REPLICATION_FLUSH,

    /* BUG-42639 Monitoring query */
    ULN_STMT_SELECT_FOR_FIXED_TABLE,

    /* PROJ-2624 ACCESS LIST */
    ULN_STMT_RELOAD_ACCESS_LIST,

    //BUG-45915
    ULN_STMT_ALT_SYS_DUMP_CALLSTACKS,

    /* PROJ-2701 Sharding online data rebuild */
    ULN_STMT_RELOAD_SHARD_META_NUMBER,
    ULN_STMT_RELOAD_SHARD_META_NUMBER_LOCAL,

    /* A sql for replication thread is DCL using one other internal transaction */
    ULN_STMT_ALT_REPLICATION_START,
    ULN_STMT_ALT_REPLICATION_QUICKSTART,
    ULN_STMT_ALT_REPLICATION_SYNC,
    ULN_STMT_ALT_REPLICATION_SYNC_CONDITION,
    ULN_STMT_ALT_REPLICATION_TEMP_SYNC,

    /* BUG-48216 */
    ULN_STMT_ROLLBACK_TO_SAVEPOINT,

    ULN_STMT_ALT_REPLICATION_FAILBACK,
    ULN_STMT_ALT_REPLICATION_DELETE_ITEM_REPLACE_HISTORY,
    
    ULN_STMT_ALT_REPLICATION_FAILOVER,

    //----------------------------------------------------
    //  SP
    //----------------------------------------------------

    ULN_STMT_EXEC_FUNC = ULN_STMT_MASK_SP,
    ULN_STMT_EXEC_PROC,
    ULN_STMT_EXEC_TEST_REC,  // PROJ-1552
    ULN_STMT_EXEC_AB,

    //----------------------------------------------------
    //  DB
    //----------------------------------------------------

    ULN_STMT_CREATE_DB = ULN_STMT_MASK_DB,
    ULN_STMT_ALTER_DB,
    ULN_STMT_DROP_DB,

    //----------------------------------------------------
    //  STMT MAX
    //----------------------------------------------------

    ULN_STMT_MASK_MAX = ACP_SINT32_MAX
};

#define ulnStmtTypeIsSP(aStmtType) \
    ( (((aStmtType) & ULN_STMT_MASK_MASK) == ULN_STMT_MASK_SP) ? ACP_TRUE : ACP_FALSE )

#define ulnStmtTypeIsDDL(aStmtType) \
    ( (((aStmtType) & ULN_STMT_MASK_MASK) == ULN_STMT_MASK_DDL) ? ACP_TRUE : ACP_FALSE )
 
#endif /* _O_ULN_STMT_TYPE_H_ */
