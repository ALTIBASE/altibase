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
 * $Id: qciStmtType.h 90585 2021-04-13 07:35:51Z minku.kang $
 **********************************************************************/

#ifndef _O_QCI_STMT_TYPE_H_
#define _O_QCI_STMT_TYPE_H_ 1

/*
 *  Statement Bit Mask
 *   ID => 0000 0000
 *         ^^^^ ^^^^
 *         Type Index
 *
 *
 *   Type : DDL, DML, DCL, SP, DB
 */
#define QCI_STMT_TYPE_MAX   16
#define QCI_STMT_MASK_MASK  (0x000F0000)
#define QCI_STMT_MASK_INDEX (0x0000FFFF)

// ulnStmtType.h에서 아래 내용을 똑같이 복사해서 사용하고 있음.
// qciStmtType변경 시 ul쪽도 변경해야 함.
enum qciStmtType
{
    QCI_STMT_MASK_DDL     = 0x00000000,
    QCI_STMT_MASK_DML     = 0x00010000,
    QCI_STMT_MASK_DCL     = 0x00020000,
    QCI_STMT_MASK_SP      = 0x00040000,
    QCI_STMT_MASK_DB      = 0x00050000,

    //----------------------------------------------------
    //  DDL
    //----------------------------------------------------

    QCI_STMT_SCHEMA_DDL = QCI_STMT_MASK_DDL,
    QCI_STMT_NON_SCHEMA_DDL,
    QCI_STMT_CRT_SP,
    QCI_STMT_COMMENT,
    QCI_STMT_SHARD_DDL,

    //----------------------------------------------------
    //  DML
    //----------------------------------------------------

    QCI_STMT_SELECT = QCI_STMT_MASK_DML,
    QCI_STMT_LOCK_TABLE,
    QCI_STMT_INSERT,
    QCI_STMT_UPDATE,
    QCI_STMT_DELETE,
    QCI_STMT_CHECK_SEQUENCE,
    QCI_STMT_MOVE,
    // Proj-1360 Queue
    QCI_STMT_ENQUEUE,
    QCI_STMT_DEQUEUE,
    // PROJ-1362
    QCI_STMT_SELECT_FOR_UPDATE,
    /* PROJ-1988 Implement MERGE statement */
    QCI_STMT_MERGE,
    // BUG-42397 Ref Cursor Static SQL
    QCI_STMT_SELECT_FOR_CURSOR,

    //----------------------------------------------------
    //  DCL + SESSION + SYSTEM
    //----------------------------------------------------

    QCI_STMT_COMMIT = QCI_STMT_MASK_DCL,
    QCI_STMT_ROLLBACK,
    QCI_STMT_SAVEPOINT,
    QCI_STMT_SET_AUTOCOMMIT_TRUE,
    QCI_STMT_SET_AUTOCOMMIT_FALSE,
    QCI_STMT_SET_REPLICATION_MODE,
    QCI_STMT_SET_PLAN_DISPLAY_ONLY,
    QCI_STMT_SET_PLAN_DISPLAY_ON,
    QCI_STMT_SET_PLAN_DISPLAY_OFF,
    QCI_STMT_SET_TX,
    QCI_STMT_SET_STACK,
    QCI_STMT_SET,
    QCI_STMT_ALT_SYS_CHKPT,
    QCI_STMT_ALT_SYS_SHRINK_MEMPOOL,
    QCI_STMT_ALT_SYS_MEMORY_COMPACT,
    QCI_STMT_SET_SYSTEM_PROPERTY,
    QCI_STMT_SET_SESSION_PROPERTY,
    QCI_STMT_CHECK,
    QCI_STMT_ALT_SYS_REORGANIZE,
    QCI_STMT_ALT_SYS_VERIFY,
    QCI_STMT_ALT_SYS_ARCHIVELOG,
    QCI_STMT_ALT_TABLESPACE_CHKPT_PATH,
    QCI_STMT_ALT_TABLESPACE_DISCARD,
    QCI_STMT_ALT_DATAFILE_ONOFF,
    QCI_STMT_ALT_RENAME_DATAFILE,
    QCI_STMT_ALT_TABLESPACE_BACKUP,
    QCI_STMT_ALT_SYS_SWITCH_LOGFILE,
    QCI_STMT_ALT_SYS_FLUSH_BUFFER_POOL,
    QCI_STMT_CONNECT,
    QCI_STMT_DISCONNECT,
    QCI_STMT_ENABLE_PARALLEL_DML,
    QCI_STMT_DISABLE_PARALLEL_DML,
    QCI_STMT_COMMIT_FORCE,
    QCI_STMT_ROLLBACK_FORCE,

    // PROJ-1568 Buffer Manager Renewal
    QCI_STMT_FLUSHER_ONOFF,

    // PROJ-1436 SQL Plan Cache
    QCI_STMT_ALT_SYS_COMPACT_PLAN_CACHE,
    QCI_STMT_ALT_SYS_RESET_PLAN_CACHE,

    QCI_STMT_ALT_SYS_REBUILD_MIN_VIEWSCN,

    // PROJ-2002 Column Security
    QCI_STMT_ALT_SYS_SECURITY,

    /* PROJ-1832 New database link */
    QCI_STMT_CONTROL_DATABASE_LINKER,
    QCI_STMT_CLOSE_DATABASE_LINK,

    QCI_STMT_COMMIT_FORCE_DATABASE_LINK,
    QCI_STMT_ROLLBACK_FORCE_DATABASE_LINK,

    // PROJ-2223 Audit
    QCI_STMT_ALT_SYS_AUDIT,
    QCI_STMT_AUDIT_OPTION,
    QCI_STMT_NOAUDIT_OPTION,
    /* BUG-39074 */
    QCI_STMT_DELAUDIT_OPTION, 

    /* BUG-42852 STOP과 FLUSH를 DCL로 변환합니다. */
    QCI_STMT_ALT_REPLICATION_STOP,
    QCI_STMT_ALT_REPLICATION_FLUSH,

    /* BUG-42639 Monitoring query */
    QCI_STMT_SELECT_FOR_FIXED_TABLE,

    /* PROJ-2624 ACCESS LIST */
    QCI_STMT_RELOAD_ACCESS_LIST,

    //BUG-45915
    QCI_STMT_ALT_SYS_DUMP_CALLSTACKS,

    /* PROJ-2701 Sharding online data rebuild */
    QCI_STMT_RELOAD_SHARD_META_NUMBER,
    QCI_STMT_RELOAD_SHARD_META_NUMBER_LOCAL,

    /* A sql for replication thread is DCL using one other internal transaction */
    QCI_STMT_ALT_REPLICATION_START,
    QCI_STMT_ALT_REPLICATION_QUICKSTART,
    QCI_STMT_ALT_REPLICATION_SYNC,
    QCI_STMT_ALT_REPLICATION_SYNC_CONDITION,
    QCI_STMT_ALT_REPLICATION_TEMP_SYNC,

    /* BUG-48216 */
    QCI_STMT_ROLLBACK_TO_SAVEPOINT,

    QCI_STMT_ALT_REPLICATION_FAILBACK,
    QCI_STMT_ALT_REPLICATION_DELETE_ITEM_REPLACE_HISTORY,
    
    QCI_STMT_ALT_REPLICATION_FAILOVER,

    //----------------------------------------------------
    //  SP
    //----------------------------------------------------

    QCI_STMT_EXEC_FUNC = QCI_STMT_MASK_SP,
    QCI_STMT_EXEC_PROC,
    QCI_STMT_EXEC_TEST_REC,  // PROJ-1552
    QCI_STMT_EXEC_AB,

    //----------------------------------------------------
    //  DB
    //----------------------------------------------------

    QCI_STMT_CREATE_DB = QCI_STMT_MASK_DB,
    QCI_STMT_ALTER_DB,
    QCI_STMT_DROP_DB,

    //----------------------------------------------------
    //  STMT MAX
    //----------------------------------------------------

    QCI_STMT_MASK_MAX = 0x80000000
};

#endif /* _O_QCI_STMT_TYPE_H_ */
