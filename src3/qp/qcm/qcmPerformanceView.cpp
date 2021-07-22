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
 * $Id: qcmPerformanceView.cpp 91159 2021-07-07 06:47:36Z jinku.ko $
 *
 * Description :
 *
 *     FT 테이블들에 대하여
 *
 *        [BUG-16437]
 *
 *        CREATE VIEW V$VIEW
 *           (C1, C2, C3)
 *        AS SELECT
 *           A1, A2, A3
 *        FROM X$TABLE;
 *
 *     과 같이 view를 정의하여 일반 사용자에게는  이 view에 대한 연산만을
 *     개방하도록 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smDef.h>
#include <smi.h>
#include <smcDef.h>
#include <mtcDef.h>
#include <mtdTypes.h>
#include <mtuProperty.h>
#include <qcg.h>
#include <qcmPerformanceView.h>
#include <qcuProperty.h>
#include <qcuSqlSourceInfo.h>
#include <qcpManager.h>
#include <qmv.h>


// PROJ-1726
// 동적 모듈화가 진행된 모듈의 경우 퍼포먼스 뷰의 정의가
// <모듈>i.h 에 define 으로 정의되어있다.
#include <rpi.h>
#include <sti.h>
#include <sdi.h>

// PROJ-1726 - qcmPerformanceViewManager 에서 사용하는
// static 변수 선언.
SChar ** qcmPerformanceViewManager::mPreViews;
SInt     qcmPerformanceViewManager::mNumOfPreViews;
SInt     qcmPerformanceViewManager::mNumOfAddedViews;
iduList  qcmPerformanceViewManager::mAddedViewList;
SChar ** qcmPerformanceViewManager::mShardViews;
SInt     qcmPerformanceViewManager::mShardViewCount;

// global performance view strings.
SChar * gQcmPerformanceViews[] =
{
    /* BUG-46217 V$TABLE은 performance view만 보여주도록 함 */
    (SChar*)"CREATE VIEW V$TABLE "
               "(NAME, SLOTSIZE, COLUMNCOUNT) "
            "AS SELECT "
               "NAME, SLOTSIZE, COLUMNCOUNT "
            "FROM X$TABLE "
            "WHERE SUBSTR(NAME, 1, 2) IN ('S$', 'V$') "
            "ORDER BY NAME",

    //[TASK-6757]LFG,SN 제거 : LFG_ID 는 0 으로 출력함
    (SChar*)"CREATE VIEW V$DISK_RTREE_HEADER "
           "(INDEX_NAME, INDEX_ID, INDEX_TBS_ID, TABLE_TBS_ID,"
           "IS_CONSISTENT, "
           "IS_CREATED_WITH_LOGGING, IS_CREATED_WITH_FORCE,"
           "COMPLETION_LSN_LFG_ID, COMPLETION_LSN_FILE_NO, COMPLETION_LSN_FILE_OFFSET, "
           "INIT_TRANS, MAX_TRANS, FREE_NODE_HEAD, FREE_NODE_CNT, FREE_NODE_SCN, "
           "INITEXTENTS, NEXTEXTENTS, MINEXTENTS, MAXEXTENTS ) "
        "AS SELECT "
           "INDEX_NAME, INDEX_ID, INDEX_TBS_ID, TABLE_TBS_ID,"
           "IS_CONSISTENT,"
           "IS_CREATED_WITH_LOGGING, IS_CREATED_WITH_FORCE,"
           "CAST(0 AS INTEGER), COMPLETION_LSN_FILE_NO, COMPLETION_LSN_FILE_OFFSET, "
           "INIT_TRANS, MAX_TRANS, FREE_NODE_HEAD, FREE_NODE_CNT, FREE_NODE_SCN, "
           "INITEXTENTS, NEXTEXTENTS, MINEXTENTS, MAXEXTENTS "
        "FROM X$DISK_RTREE_HEADER",

    //[TASK-6757]LFG,SN 제거 : LFG_ID 는 0 으로 출력함
    (SChar*)"CREATE VIEW V$DISK_BTREE_HEADER "
               "(INDEX_NAME, INDEX_ID, INDEX_TBS_ID, TABLE_TBS_ID,"
               "IS_UNIQUE, COLLENINFO_LIST, IS_CONSISTENT, "
               "IS_CREATED_WITH_LOGGING, IS_CREATED_WITH_FORCE,"
               "COMPLETION_LSN_LFG_ID, COMPLETION_LSN_FILE_NO, COMPLETION_LSN_FILE_OFFSET, "
               "INIT_TRANS, MAX_TRANS, FREE_NODE_HEAD, FREE_NODE_CNT, "
               "INITEXTENTS, NEXTEXTENTS, MINEXTENTS, MAXEXTENTS ) "
            "AS SELECT "
               "INDEX_NAME, INDEX_ID, INDEX_TBS_ID, TABLE_TBS_ID,"
               "IS_UNIQUE, COLLENINFO_LIST, IS_CONSISTENT,"
               "IS_CREATED_WITH_LOGGING, IS_CREATED_WITH_FORCE,"
               "CAST(0 AS INTEGER), COMPLETION_LSN_FILE_NO, COMPLETION_LSN_FILE_OFFSET, "
               "INIT_TRANS, MAX_TRANS, FREE_NODE_HEAD, FREE_NODE_CNT,"
               "INITEXTENTS, NEXTEXTENTS, MINEXTENTS, MAXEXTENTS "
            "FROM X$DISK_BTREE_HEADER",

    (SChar*)"CREATE VIEW V$MEM_BTREE_HEADER "
               "(INDEX_NAME, INDEX_ID, INDEX_TBS_ID, TABLE_TBS_ID,"
               "IS_UNIQUE, IS_NOT_NULL, USED_NODE_COUNT, PREPARE_NODE_COUNT, "
               "BUILT_TYPE ) "
            "AS SELECT "
               "INDEX_NAME, INDEX_ID, INDEX_TBS_ID, TABLE_TBS_ID,"
               "IS_UNIQUE, IS_NOT_NULL, USED_NODE_COUNT, PREPARE_NODE_COUNT, "
               "BUILT_TYPE "
            "FROM X$MEM_BTREE_HEADER",

    // BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
    (SChar*)"CREATE VIEW V$MEM_BTREE_NODEPOOL "
               "(TOTAL_PAGE_COUNT, TOTAL_NODE_COUNT, FREE_NODE_COUNT, USED_NODE_COUNT, "
               "NODE_SIZE, TOTAL_ALLOC_REQ, TOTAL_FREE_REQ, FREE_REQ_COUNT ) "
            "AS SELECT "
               "TOTAL_PAGE_COUNT, TOTAL_NODE_COUNT, FREE_NODE_COUNT, USED_NODE_COUNT, "
               "NODE_SIZE, TOTAL_ALLOC_REQ, TOTAL_FREE_REQ, FREE_REQ_COUNT "
            "FROM X$MEM_BTREE_NODEPOOL",

    (SChar*)"CREATE VIEW V$ALLCOLUMN "
               "( TABLENAME, COLNAME ) "
            "AS SELECT /*+ USE_HASH( T, C ) */ "
               "C.TABLENAME, C.COLNAME "
            "FROM X$TABLE T, X$COLUMN C "
            "WHERE T.NAME = C.TABLENAME",

    (SChar*)"CREATE VIEW V$TABLESPACES "
               "(ID, NAME, NEXT_FILE_ID, TYPE, STATE, EXTENT_MANAGEMENT, SEGMENT_MANAGEMENT, "
               "DATAFILE_COUNT, TOTAL_PAGE_COUNT, EXTENT_PAGE_COUNT,"
               "ALLOCATED_PAGE_COUNT, PAGE_SIZE, ATTR_LOG_COMPRESS) "
            "AS SELECT /*+ USE_HASH( A, B ) */ "
               "B.ID, B.NAME, A.NEXT_FILE_ID, B.TYPE, B.STATE_BITSET, A.EXTENT_MANAGEMENT,"
               "A.SEGMENT_MANAGEMENT,"
               "A.DATAFILE_COUNT,"
               "A.TOTAL_PAGE_COUNT,A.EXTENT_PAGE_COUNT,"
               "A.ALLOCATED_PAGE_COUNT,"
               "A.PAGE_SIZE,"
               "B.ATTR_LOG_COMPRESS "
            "FROM X$TABLESPACES A, X$TABLESPACES_HEADER B "
            "WHERE A.ID=B.ID",

    // archive 정보등 백업에서 사용되는 performance view.
    //[TASK-6757]LFG,SN 제거 : LFG_ID 는 0으로 출력함  
    (SChar*)"CREATE VIEW V$ARCHIVE "
                "(LFG_ID, ARCHIVE_MODE, ARCHIVE_THR_RUNNING, ARCHIVE_DEST, "
                "NEXTLOGFILE_TO_ARCH, OLDEST_ACTIVE_LOGFILE,CURRENT_LOGFILE) "
            "AS SELECT "
                "CAST(0 AS INTEGER), ARCHIVE_MODE,ARCHIVE_THR_RUNNING, ARCHIVE_DEST, "
                "NEXTLOGFILE_TO_ARCH, OLDEST_ACTIVE_LOGFILE,CURRENT_LOGFILE "
            "FROM X$ARCHIVE",

    // buffer pool 통계를 보여주는 performance view.
    (SChar*)"CREATE VIEW V$BUFFPOOL_STAT"
                "(ID, POOL_SIZE, PAGE_SIZE, HASH_BUCKET_COUNT, HASH_CHAIN_LATCH_COUNT, "
                "LRU_LIST_COUNT, PREPARE_LIST_COUNT, FLUSH_LIST_COUNT, CHECKPOINT_LIST_COUNT, "
                "VICTIM_SEARCH_COUNT, HASH_PAGES, HOT_LIST_PAGES, COLD_LIST_PAGES, "
                "PREPARE_LIST_PAGES, FLUSH_LIST_PAGES, CHECKPOINT_LIST_PAGES, "
                "FIX_PAGES, GET_PAGES, READ_PAGES, CREATE_PAGES, HIT_RATIO, "
                "HOT_HITS, COLD_HITS, PREPARE_HITS, FLUSH_HITS, OTHER_HITS, "
                "PREPARE_VICTIMS, LRU_VICTIMS, VICTIM_FAILS, PREPARE_AGAIN_VICTIMS, "
                "VICTIM_SEARCH_WARP, LRU_SEARCHS, LRU_SEARCHS_AVG, LRU_TO_HOTS, "
                "LRU_TO_COLDS, LRU_TO_FLUSHS, HOT_INSERTIONS, COLD_INSERTIONS,"
                "DB_SINGLE_READ_PERF,DB_MULTI_READ_PERF ) "
            "AS SELECT "
                "ID, POOL_SIZE, PAGE_SIZE, HASH_BUCKET_COUNT, HASH_CHAIN_LATCH_COUNT, "
                "LRU_LIST_COUNT, PREPARE_LIST_COUNT, FLUSH_LIST_COUNT, CHECKPOINT_LIST_COUNT, "
                "VICTIM_SEARCH_COUNT, HASH_PAGES, HOT_LIST_PAGES, COLD_LIST_PAGES, "
                "PREPARE_LIST_PAGES, FLUSH_LIST_PAGES, CHECKPOINT_LIST_PAGES, "
                "FIX_PAGES, GET_PAGES, READ_PAGES, CREATE_PAGES, HIT_RATIO, "
                "HOT_HITS, COLD_HITS, PREPARE_HITS, FLUSH_HITS, OTHER_HITS, "
                "PREPARE_VICTIMS, LRU_VICTIMS, VICTIM_FAILS, PREPARE_AGAIN_VICTIMS, "
                "VICTIM_SEARCH_WARP, LRU_SEARCHS, LRU_SEARCHS_AVG, LRU_TO_HOTS, "
                "LRU_TO_COLDS, LRU_TO_FLUSHS, HOT_INSERTIONS, COLD_INSERTIONS, "
                "CAST( CASE WHEN NORMAL_READ_USEC = 0 THEN 0 " 
                "ELSE ( NORMAL_READ_PAGE_COUNT * 8192 / 1024  ) "
                "/ ( NORMAL_READ_USEC/1000/1000 ) END AS DOUBLE),"
                "CAST( CASE WHEN MPR_READ_USEC = 0 THEN 0 " 
                "ELSE ( MPR_READ_PAGE_COUNT * 8192 / 1024  ) "
                "/ ( MPR_READ_USEC/1000/1000 ) END AS DOUBLE) "
            "FROM X$BUFFER_POOL_STAT",

    (SChar*)"CREATE VIEW V$UNDO_BUFF_STAT"
                "(READ_PAGE_COUNT,GET_PAGE_COUNT,FIX_PAGE_COUNT,"
                "CREATE_PAGE_COUNT,HIT_RATIO) "
            "AS SELECT "
                "READ_PAGES,GET_PAGES,"
                "FIX_PAGES,CREATE_PAGES,HIT_RATIO "
            "FROM X$BUFFER_PAGE_INFO WHERE PAGE_TYPE = 10",

    // memory GC상태를 보여주는 fixed table.
    (SChar*)"CREATE VIEW V$MEMGC "
                "(GC_NAME, CURRSYSTEMVIEWSCN, MINMEMSCNINTXS, OLDESTTX, SCNOFTAIL, "
                "IS_EMPTY_OIDLIST, ADD_OID_CNT, GC_OID_CNT, "
                "AGING_REQUEST_OID_CNT, AGING_PROCESSED_OID_CNT, THREAD_COUNT) "
            "AS SELECT "
                "GC_NAME, CURRSYSTEMVIEWSCN, MINMEMSCNINTXS, OLDESTTX, SCNOFTAIL, "
                "IS_EMPTY_OIDLIST, ADD_OID_CNT, GC_OID_CNT, "
                "AGING_REQUEST_OID_CNT, AGING_PROCESSED_OID_CNT, THREAD_COUNT "
            "FROM  X$MEMGC_L_AGER "
            "UNION "
            "SELECT "
                "GC_NAME, CURRSYSTEMVIEWSCN, MINMEMSCNINTXS, OLDESTTX, SCNOFTAIL, "
                "IS_EMPTY_OIDLIST, ADD_OID_CNT, GC_OID_CNT, "
                "AGING_REQUEST_OID_CNT, AGING_PROCESSED_OID_CNT, THREAD_COUNT "
            "FROM X$MEMGC_DELTHR",

    // disk GC상태를 보여주는 fixed table.
    /* xxxxxxxxxxxxxxxxxxxxxxxxxx
    (SChar*)"CREATE VIEW V$DISKGC "
                "(GC_NAME, THREAD_COUNT, CURR_SYSTEMVIEWSCN, MINDISKSCN_INTXS, SCNOFTAIL, "
                "ADD_TSS_CNT, GC_TSS_CNT, TX_GC_TSS_CNT, GET_PAGE_CNT, READ_PAGE_CNT, "
                "LAST_IO_WAIT_TIME_US, MAX_IO_WAIT_TIME_US, TOTAL_IO_WAIT_TIME_US) "
            "AS SELECT "
                "GC_NAME, THREAD_COUNT, CURR_SYSTEMVIEWSCN, MINDISKSCN_INTXS, "
                "SCNOFTAIL, ADD_TSS_CNT, GC_TSS_CNT, TX_GC_TSS_CNT, GET_PAGE_CNT, READ_PAGE_CNT, "
                "LAST_IO_WAIT_TIME_US, MAX_IO_WAIT_TIME_US, TOTAL_IO_WAIT_TIME_US "
            "FROM X$DISK_GC "
            "UNION "
            "SELECT "
                "GC_NAME, THREAD_COUNT, CURR_SYSTEMVIEWSCN, MINDISKSCN_INTXS, "
                "SCNOFTAIL, ADD_TSS_CNT, GC_TSS_CNT, TX_GC_TSS_CNT, GET_PAGE_CNT, READ_PAGE_CNT, "
                "LAST_IO_WAIT_TIME_US, MAX_IO_WAIT_TIME_US, TOTAL_IO_WAIT_TIME_US "
            "FROM  X$DISK_DEL_THR",
            */

    // alias되어 performace view로 보여지는 fixed table.
    //[TASK-6757]LFG,SN 제거 : LFG_ID 는 0으로 출력함  
    (SChar*)"CREATE VIEW V$DATAFILES "
                "( ID, NAME, SPACEID, "
                "OLDEST_LSN_LFGID, OLDEST_LSN_FILENO, OLDEST_LSN_OFFSET, "
                "CREATE_LSN_LFGID, CREATE_LSN_FILENO, CREATE_LSN_OFFSET, "
                "SM_VERSION, NEXTSIZE, MAXSIZE, INITSIZE, CURRSIZE, "
                "AUTOEXTEND, IOCOUNT, OPENED, MODIFIED, STATE, "
                "MAX_OPEN_FD_COUNT, CUR_OPEN_FD_COUNT ) "
            "AS SELECT "
                "ID, NAME, SPACEID, "
                "CAST(0 AS INTEGER), DISK_REDO_LSN_FILENO, DISK_REDO_LSN_OFFSET, "
                "CAST(0 AS INTEGER), CREATE_LSN_FILENO, CREATE_LSN_OFFSET, "
                "SM_VERSION, NEXTSIZE, MAXSIZE, INITSIZE, CURRSIZE, "
                "AUTOEXTEND, IOCOUNT, OPENED, MODIFIED, STATE, "
                "MAX_OPEN_FD_COUNT, CUR_OPEN_FD_COUNT "
            "FROM X$DATAFILES",

    // PR-13911 [PROJ-1490] DB Free Page List Performance View구현
    // MEM_DBFILE_SIZE와 MEM_FREE_PAGE_COUNT을 계산하여 select하도록 수정
    (SChar*)"CREATE VIEW V$DATABASE "
                "(DB_NAME, PRODUCT_SIGNATURE, DB_SIGNATURE, VERSION_ID, "
                "COMPILE_BIT, ENDIAN, LOGFILE_SIZE, TX_TBL_SIZE, "
                "LAST_SYSTEM_SCN, INIT_SYSTEM_SCN, DURABLE_SYSTEM_SCN, "
                "MEM_MAX_DB_SIZE, MEM_ALLOC_PAGE_COUNT, "
                "MEM_FREE_PAGE_COUNT, MAX_ACCESS_FILE_SIZ )"
            "AS SELECT /*+ USE_HASH( A, B ) */ "
                "A.DB_NAME,A.PRODUCT_SIGNATURE,A.DB_SIGNATURE, "
                "A.VERSION_ID, A.COMPILE_BIT,A.ENDIAN,A.LOGFILE_SIZE, "
                "A.TX_TBL_SIZE, B.LAST_SYSTEM_SCN, CAST(0 AS VARCHAR(29)), "
                "A.DURABLE_SYSTEM_SCN, "
                "( SELECT memory_value1 from x$property "
                "  WHERE name='MEM_MAX_DB_SIZE' LIMIT 1) MEM_MAX_DB_SIZE, "
                "( SELECT SUM(MEM_ALLOC_PAGE_COUNT) FROM X$MEMBASE ) "
                "AS MEM_ALLOC_PAGE_COUNT, "
                "( SELECT SUM(FREE_PAGE_COUNT) "
                "  FROM X$MEM_TABLESPACE_FREE_PAGE_LIST ) "
                "AS MEM_FREE_PAGE_COUNT, "
                "DECODE(B.MAX_ACCESS_FILE_SIZE,4,'2^31 -1', 8,'2^63-1',"
                "       'INVALID_SIZE') MAX_ACCESS_FILE_SIZ "
            "FROM X$MEMBASE A,  X$MEMBASEMGR B "
            "WHERE A.SPACE_ID = B.SPACE_ID AND A.SPACE_ID = 0",

    (SChar*)"CREATE VIEW V$MEM_TABLESPACES "
                "(SPACE_ID, "
                "SPACE_NAME, "
                "SPACE_STATUS, "
                "SPACE_SHM_KEY, "
                "AUTOEXTEND_MODE, "
                "AUTOEXTEND_NEXTSIZE, "
                "MAXSIZE, "
                "CURRENT_SIZE, "
                "DBFILE_SIZE, DBFILE_COUNT_0, "
                "DBFILE_COUNT_1, TIMESTAMP, ALLOC_PAGE_COUNT, "
                "FREE_PAGE_COUNT, RESTORE_TYPE, CURRENT_DB, "
                "HIGH_LIMIT_PAGE, PAGE_COUNT_PER_FILE, "
                "PAGE_COUNT_IN_DISK )"
            "AS SELECT /*+ ORDERED USE_HASH( A, B ) USE_HASH( B, C )  */ "
                "A.SPACE_ID, "
                "C.SPACE_NAME, "
                "C.SPACE_STATUS, "
                "C.SPACE_SHM_KEY, "
                "C.AUTOEXTEND_MODE, "
                "C.AUTOEXTEND_NEXTSIZE, "
                "C.MAXSIZE, "
                "C.CURRENT_SIZE, "
                "CAST( A.MEM_DBFILE_PAGE_COUNT * B.PAGE_SIZE AS DOUBLE ) AS DBFILE_SIZE,"
                "A.MEM_DBFILE_COUNT_0, A.MEM_DBFILE_COUNT_1, A.MEM_TIMESTAMP,"
                "A.MEM_ALLOC_PAGE_COUNT, "
                "D.FREE_PAGE_COUNT "
                "AS FREE_PAGE_COUNT, "
                "B.RESTORE_TYPE, "
                "B.CURRENT_DB, "
                "B.HIGH_LIMIT_PAGE, "
                "B.PAGE_COUNT_PER_FILE, "
                "B.PAGE_COUNT_IN_DISK "
                "FROM X$MEMBASE A, X$MEMBASEMGR B, X$MEM_TABLESPACE_DESC C, ("
                "SELECT SPACE_ID, SUM(FREE_PAGE_COUNT) FREE_PAGE_COUNT FROM "
                "X$MEM_TABLESPACE_FREE_PAGE_LIST "
                "GROUP BY SPACE_ID ) D "
            "WHERE A.SPACE_ID = B.SPACE_ID AND B.SPACE_ID = C.SPACE_ID AND C.SPACE_ID = D.SPACE_ID",

    /* BUG-31080 - [SM] need the checking method about the amount of
     *             volatile tablespace.
     * V$VOL_TABLESPACES에 ALLOC_PAGE_COUNT, FREE_PAGE_COUNT 칼럼을 추가함. */
    (SChar*)"CREATE VIEW V$VOL_TABLESPACES "
                "(SPACE_ID, "
                "SPACE_NAME, "
                "SPACE_STATUS, "
                "INIT_SIZE, "
                "AUTOEXTEND_MODE, "
                "NEXT_SIZE, "
                "MAX_SIZE, "
                "CURRENT_SIZE, "
                "ALLOC_PAGE_COUNT, "
                "FREE_PAGE_COUNT ) "
            "AS SELECT /*+ ORDERED USE_HASH( A, B ) USE_HASH( B, C ) */ "
                "B.SPACE_ID, "
                "B.SPACE_NAME, "
                "B.SPACE_STATUS, "
                "B.INIT_SIZE, "
                "B.AUTOEXTEND_MODE, "
                "B.NEXT_SIZE, "
                "B.MAX_SIZE, "
                "B.CURRENT_SIZE, "
                "A.ALLOCATED_PAGE_COUNT, "
                "C.FREE_PAGE_COUNT "
                "FROM X$TABLESPACES A, X$VOL_TABLESPACE_DESC B, ("
                "SELECT SPACE_ID, SUM(FREE_PAGE_COUNT) FREE_PAGE_COUNT FROM "
                "X$VOL_TABLESPACE_FREE_PAGE_LIST "
                "GROUP BY SPACE_ID ) C "
           "WHERE A.ID = B.SPACE_ID AND B.SPACE_ID = C.SPACE_ID",

    (SChar*)"CREATE VIEW V$MEM_TABLESPACE_CHECKPOINT_PATHS AS "
            "SELECT * FROM X$MEM_TABLESPACE_CHECKPOINT_PATHS",

    (SChar*)"CREATE VIEW V$MEM_TABLESPACE_STATUS_DESC AS "
            "SELECT * FROM X$MEM_TABLESPACE_STATUS_DESC",

    (SChar*)"CREATE VIEW V$MEMSTAT "
                "( NAME, ALLOC_SIZE, ALLOC_COUNT, MAX_TOTAL_SIZE ) "
            "AS SELECT "
                "NAME, SUM(ALLOC_SIZE), SUM(ALLOC_COUNT), SUM(MAX_TOTAL_SIZE) "
            "FROM X$MEMSTAT GROUP BY NAME",

    (SChar*)"CREATE VIEW V$MEMALLOC "
                "( ALLOC_NAME, ALLOC_TYPE, POOL_SIZE, USED_COUNT, FREE_SIZE ) "
            "AS SELECT "
                " ALLOC_NAME, ALLOC_TYPE, POOL_SIZE, USED_SIZE, "
                " CAST(POOL_SIZE-USED_SIZE AS BIGINT) AS FREE_SIZE "
            "FROM X$MEMALLOC",

    /* Local SID를 얻어오는 질의, 다른 노드의 SID는 설정될 수 없으므로,
     * X$Property에는 자신의 SID만 존재한다. */
    (SChar*)"CREATE VIEW V$PROPERTY "
                "( NAME, STOREDCOUNT, ATTR, MIN, MAX, "
                "VALUE1, VALUE2, VALUE3, VALUE4, VALUE5, VALUE6, "
                "VALUE7, VALUE8 ) "
            "AS SELECT /*+USE_HASH(A,B)*/ "
                "A.NAME, A.MEMORY_VALUE_COUNT, A.ATTR, A.MIN, A.MAX, "
                "A.MEMORY_VALUE1, A.MEMORY_VALUE2, A.MEMORY_VALUE3, A.MEMORY_VALUE4, "
                "A.MEMORY_VALUE5, A.MEMORY_VALUE6, A.MEMORY_VALUE7, A.MEMORY_VALUE8 "
            "FROM X$PROPERTY A, X$PROPERTY B "
            "WHERE MOD(A.ATTR,2)=0 AND A.SID = B.SID AND B.NAME = 'SID'",

    (SChar*)"CREATE VIEW V$SPROPERTY "
        "( SID, NAME, STOREDCOUNT, ATTR, MIN, MAX, ISSPECIFIED, "
        "VALUE1, VALUE2, VALUE3, VALUE4, VALUE5, VALUE6, VALUE7, VALUE8 ) "
        /* asterisk로 설정된 값이 있는 경우 프로퍼티 마다 하나씩(LOCAL SID를 갖는 프로퍼티) 보여준다. */
        "AS SELECT '*', A.NAME, A.SPFILE_BY_ASTERISK_VALUE_COUNT, "
                "A.ATTR, A.MIN, A.MAX, 'TRUE', "
                "A.SPFILE_BY_ASTERISK_VALUE1, A.SPFILE_BY_ASTERISK_VALUE2, "
                "A.SPFILE_BY_ASTERISK_VALUE3, A.SPFILE_BY_ASTERISK_VALUE4, "
                "A.SPFILE_BY_ASTERISK_VALUE5, A.SPFILE_BY_ASTERISK_VALUE6, "
                "A.SPFILE_BY_ASTERISK_VALUE7, A.SPFILE_BY_ASTERISK_VALUE8 "
            "FROM X$PROPERTY A, X$PROPERTY B "
            "WHERE A.SID = B.SID AND B.NAME = 'SID' "
                "AND A.SPFILE_BY_ASTERISK_VALUE_COUNT > 0 AND MOD(A.ATTR,2)=0 "
        "UNION ALL "
        /* "SID"로 설정된 값은 모든 SID, NAME에 대해 보여준다. */
            "SELECT SID, NAME, SPFILE_BY_SID_VALUE_COUNT, "
                "ATTR, MIN, MAX, 'TRUE', "
                "SPFILE_BY_SID_VALUE1, SPFILE_BY_SID_VALUE2, "
                "SPFILE_BY_SID_VALUE3, SPFILE_BY_SID_VALUE4, "
                "SPFILE_BY_SID_VALUE5, SPFILE_BY_SID_VALUE6, "
                "SPFILE_BY_SID_VALUE7, SPFILE_BY_SID_VALUE8 "
            "FROM X$PROPERTY "
            "WHERE MOD(ATTR,2)=0 AND SPFILE_BY_SID_VALUE_COUNT > 0 "
        "UNION ALL "
        /* 하나의 프로퍼티 리스트에서 asterisk와 "SID" 두 가지 중 하나도
         * 설정된 것이 없으면 asterisk와 null을 갖는 값을 반환한다.
         * v$property를 v로 사용하여 프로퍼티 목록에서
         * '*'나'sid'로 설정되지 않은 프로퍼티를 찾음. */
            "SELECT '*', V.NAME, 0, ATTR, MIN, MAX, 'FALSE', "
                "VARCHAR'', VARCHAR'', VARCHAR'', VARCHAR'', VARCHAR'', VARCHAR'', VARCHAR'', VARCHAR'' "
            "FROM "
                "(SELECT /*+USE_HASH(A,B)*/ A.NAME, A.MEMORY_VALUE_COUNT, A.ATTR, A.MIN, A.MAX "
                 "FROM X$PROPERTY A, X$PROPERTY B "
                 "WHERE MOD(A.ATTR,2)=0 AND A.SID = B.SID AND B.NAME = 'SID') V LEFT OUTER JOIN "
                "(SELECT NAME "
                 "FROM X$PROPERTY X "
                 "WHERE (SPFILE_BY_ASTERISK_VALUE_COUNT > 0 OR SPFILE_BY_SID_VALUE_COUNT > 0) AND MOD(ATTR,2)=0 "
                 "GROUP BY NAME) W ON V.NAME = W.NAME "
            "WHERE W.NAME IS NULL",

    (SChar*)"CREATE VIEW V$SESSION "
                "( ID, TRANS_ID,TASK_STATE, COMM_NAME, XA_SESSION_FLAG, "
                "XA_ASSOCIATE_FLAG, QUERY_TIME_LIMIT, DDL_TIME_LIMIT, FETCH_TIME_LIMIT, "
                "UTRANS_TIME_LIMIT, IDLE_TIME_LIMIT, IDLE_START_TIME, "
                "ACTIVE_FLAG, OPENED_STMT_COUNT, CLIENT_PACKAGE_VERSION, "
                "CLIENT_PROTOCOL_VERSION, CLIENT_PID, CLIENT_TYPE, "
                "CLIENT_APP_INFO, CLIENT_NLS, DB_USERNAME, DB_USERID, "
                "DEFAULT_TBSID, DEFAULT_TEMP_TBSID, SYSDBA_FLAG, "
                "AUTOCOMMIT_FLAG, SESSION_STATE, ISOLATION_LEVEL, "
                "REPLICATION_MODE, TRANSACTION_MODE, COMMIT_WRITE_WAIT_MODE, "
                "OPTIMIZER_MODE, HEADER_DISPLAY_MODE, "
                "CURRENT_STMT_ID, STACK_SIZE, DEFAULT_DATE_FORMAT, TRX_UPDATE_MAX_LOGSIZE, "
                "PARALLE_DML_MODE, LOGIN_TIME, FAILOVER_SOURCE, "
                "NLS_TERRITORY, NLS_ISO_CURRENCY, NLS_CURRENCY, NLS_NUMERIC_CHARACTERS, TIME_ZONE, "
                "LOB_CACHE_THRESHOLD, QUERY_REWRITE_ENABLE, "
                "DBLINK_GLOBAL_TRANSACTION_LEVEL, "
                "DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "
                "MAX_STATEMENTS_PER_SESSION, "
                "SSL_CIPHER, " 
                "SSL_CERTIFICATE_SUBJECT, "
                "SSL_CERTIFICATE_ISSUER, "
                "CLIENT_INFO, "
                "MODULE, "
                "ACTION, "
                "REPLICATION_DDL_SYNC, "
                "REPLICATION_DDL_SYNC_TIMELIMIT, "
                "MESSAGE_CALLBACK, "
                "SHARD_DDL_LOCK_TIMEOUT, "
                "SHARD_DDL_LOCK_TRY_COUNT, "
                "DDL_LOCK_TIMEOUT ) "
            "AS SELECT "
                "A.ID, "
                "A.TRANS_ID , "
                "DECODE(A.TASK_STATE, 0, 'WAITING', "
                "                     1, 'READY', "
                "                     2, 'EXECUTING', "
                "                     3, 'QUEUE WAIT', "
                "                     4, 'QUEUE READY', "
                "                     'UNKNOWN') TASK_STATE, "
                "A.COMM_NAME, A.XA_SESSION_FLAG, A.XA_ASSOCIATE_FLAG, "
                "A.QUERY_TIME_LIMIT, A.DDL_TIME_LIMIT, A.FETCH_TIME_LIMIT, "
                "A.UTRANS_TIME_LIMIT, A.IDLE_TIME_LIMIT, A.IDLE_START_TIME, "
                "A.ACTIVE_FLAG, A.OPENED_STMT_COUNT, "
                "A.CLIENT_PACKAGE_VERSION, A.CLIENT_PROTOCOL_VERSION, "
                "A.CLIENT_PID, A.CLIENT_TYPE, A.CLIENT_APP_INFO, "
                "A.CLIENT_NLS, A.DB_USERNAME, A.DB_USERID, "
                "A.DEFAULT_TBSID, A.DEFAULT_TEMP_TBSID, A.SYSDBA_FLAG, "
                "A.AUTOCOMMIT_FLAG, "
                "DECODE(A.SESSION_STATE, 0, 'INIT', "
                "                        1, 'AUTH', "
                "                        2, 'SERVICE READY', "
                "                        3, 'SERVICE', "
                "                        4, 'END', "
                "                        5, 'ROLLBACK', "
                "                        'UNKNOWN') SESSION_STATE, "
                "A.ISOLATION_LEVEL, A.REPLICATION_MODE, A.TRANSACTION_MODE, "
                "A.COMMIT_WRITE_WAIT_MODE, A.OPTIMIZER_MODE, "
                "A.HEADER_DISPLAY_MODE, A.CURRENT_STMT_ID, A.STACK_SIZE, "
                "A.DEFAULT_DATE_FORMAT, A.TRX_UPDATE_MAX_LOGSIZE, "
                "A.PARALLEL_DML_MODE, A.LOGIN_TIME, A.FAILOVER_SOURCE, "
                "A.NLS_TERRITORY, A.NLS_ISO_CURRENCY, A.NLS_CURRENCY, A.NLS_NUMERIC_CHARACTERS, "
                "A.TIME_ZONE, A.LOB_CACHE_THRESHOLD, "
                "DECODE(A.QUERY_REWRITE_ENABLE, 0, 'FALSE', "
                "                               1, 'TRUE', "
                "                               'UNKNOWN') QUERY_REWRITE_ENABLE, "
                "A.DBLINK_GLOBAL_TRANSACTION_LEVEL, "
                "A.DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "
                "A.MAX_STATEMENTS_PER_SESSION, "
                "A.SSL_CIPHER, "
                "A.SSL_CERTIFICATE_SUBJECT, "
                "A.SSL_CERTIFICATE_ISSUER, "
                "A.CLIENT_APP_INFO, "
                "A.MODULE, "
                "A.ACTION, "
                "A.REPLICATION_DDL_SYNC, "
                "A.REPLICATION_DDL_SYNC_TIMELIMIT, "
                "DECODE(A.MESSAGE_CALLBACK, 0, 'UNREG', "
                "                           1, 'REG', "
                "                              'UNKNOWN') MESSAGE_CALLBACK, "
                "A.SHARD_DDL_LOCK_TIMEOUT, "
                "A.SHARD_DDL_LOCK_TRY_COUNT, "
                "A.DDL_LOCK_TIMEOUT "
            "FROM X$SESSION A",

    (SChar*)"CREATE VIEW V$SESSIONMGR "
                "( TASK_COUNT, BASE_TIME, LOGIN_TIMEOUT_COUNT, "
                "IDLE_TIMEOUT_COUNT, QUERY_TIMEOUT_COUNT, DDL_TIMEOUT_COUNT, "
                "FETCH_TIMEOUT_COUNT, UTRANS_TIMEOUT_COUNT, "
                "SESSION_TERMINATE_COUNT ) "
            "AS SELECT "
                "TASK_COUNT, BASE_TIME, LOGIN_TIMEOUT_COUNT, "
                "IDLE_TIMEOUT_COUNT, QUERY_TIMEOUT_COUNT, DDL_TIMEOUT_COUNT, "
                "FETCH_TIMEOUT_COUNT, UTRANS_TIMEOUT_COUNT, "
                "SESSION_TERMINATE_COUNT "
            "FROM X$SESSIONMGR",

    (SChar*)"CREATE VIEW V$STATEMENT "
                "( ID, PARENT_ID, CURSOR_TYPE, SESSION_ID, TX_ID, QUERY, "
                "LAST_QUERY_START_TIME, QUERY_START_TIME, FETCH_START_TIME, "
                "EXECUTE_STATE, FETCH_STATE, ARRAY_FLAG, ROW_NUMBER, EXECUTE_FLAG, BEGIN_FLAG, "
                "TOTAL_TIME, PARSE_TIME, VALIDATE_TIME, OPTIMIZE_TIME, "
                "EXECUTE_TIME, FETCH_TIME, SOFT_PREPARE_TIME, "
                "SQL_CACHE_TEXT_ID, SQL_CACHE_PCO_ID, "
                "OPTIMIZER, COST, "
                "USED_MEMORY, READ_PAGE, WRITE_PAGE, GET_PAGE, CREATE_PAGE, "
                "UNDO_READ_PAGE, UNDO_WRITE_PAGE, UNDO_GET_PAGE, "
                "UNDO_CREATE_PAGE, MEM_CURSOR_FULL_SCAN, "
                "MEM_CURSOR_INDEX_SCAN, DISK_CURSOR_FULL_SCAN, "
                "DISK_CURSOR_INDEX_SCAN, EXECUTE_SUCCESS, "
                "EXECUTE_FAILURE, FETCH_SUCCESS, FETCH_FAILURE, "
                "PROCESS_ROW, MEMORY_TABLE_ACCESS_COUNT, "
                "SEQNUM, EVENT, P1, P2, P3, "
                "WAIT_TIME, SECOND_IN_TIME, SIMPLE_QUERY, "
                "MATHEMATICS_TEMP_MEMORY )"
            "AS SELECT /*+ USE_HASH( A, B ) */ "
                "A.ID, A.PARENT_ID, A.CURSOR_TYPE, A.SESSION_ID, A.TX_ID, A.QUERY, "
                "A.LAST_QUERY_START_TIME, A.QUERY_START_TIME, A.FETCH_START_TIME, "
                "DECODE(A.STATE, 0, 'ALLOC', "
                "                1, 'PREPARED', "
                "                2, 'EXECUTED', "
                "                'UNKNOWN') EXECUTE_STATE, "
                "DECODE(A.FETCH_STATE, 0, 'PROCEED', "
                "                      1, 'CLOSE', "
                "                      2, 'NO RESULTSET', "
                "                      3, 'INVALIDATED', "
                "                      'UNKNOWN') FETCH_STATE, "
                "A.ARRAY_FLAG, A.ROW_NUMBER, A.EXECUTE_FLAG, A.BEGIN_FLAG, "
                "A.TOTAL_TIME, A.PARSE_TIME, A.VALIDATE_TIME, A.OPTIMIZE_TIME, "
                "A.EXECUTE_TIME, A.FETCH_TIME, A.SOFT_PREPARE_TIME, "
                "A.SQL_CACHE_TEXT_ID, A.SQL_CACHE_PCO_ID, "
                "A.OPTIMIZER, A.COST, "
                "A.USED_MEMORY, A.READ_PAGE, A.WRITE_PAGE, A.GET_PAGE, "
                "A.CREATE_PAGE, "
                "A.UNDO_READ_PAGE, A.UNDO_WRITE_PAGE, A.UNDO_GET_PAGE, "
                "A.UNDO_CREATE_PAGE, A.MEM_CURSOR_FULL_SCAN, "
                "A.MEM_CURSOR_INDEX_SCAN, A.DISK_CURSOR_FULL_SCAN, "
                "A.DISK_CURSOR_INDEX_SCAN, A.EXECUTE_SUCCESS, "
                "A.EXECUTE_FAILURE, A.FETCH_SUCCESS, A.FETCH_FAILURE, "
                "A.PROCESS_ROW, A.MEMORY_TABLE_ACCESS_COUNT,"
                "A.SEQNUM, B.EVENT, A.P1, A.P2, "
                "A.P3, A.WAIT_TIME, A.SECOND_IN_TIME, A.SIMPLE_QUERY, "
                "A.MATHEMATICS_TEMP_MEMORY "
            "FROM X$STATEMENT A, X$WAIT_EVENT_NAME B "
            "WHERE A.SEQNUM = B.EVENT_ID ",

    //[TASK-6757]LFG,SN 제거 : LFG_ID 는 0으로 출력함 
    //BUG-47727: undo 진행 상태 관련 정보 추가 
    (SChar*)"CREATE VIEW V$TRANSACTION "
                "( ID, SESSION_ID, MEMORY_VIEW_SCN, MIN_MEMORY_LOB_VIEW_SCN, "
                "DISK_VIEW_SCN, MIN_DISK_LOB_VIEW_SCN, COMMIT_SCN, "
                "STATUS, UPDATE_STATUS, LOG_TYPE, "
                "XA_COMMIT_STATUS, XA_PREPARED_TIME, "
                "FIRST_UNDO_NEXT_LSN_LFGID, FIRST_UNDO_NEXT_LSN_FILENO, "
                "FIRST_UNDO_NEXT_LSN_OFFSET, CURRENT_UNDO_NEXT_SN, "
                "CURRENT_UNDO_NEXT_LSN_LFGID, CURRENT_UNDO_NEXT_LSN_FILENO, "
                "CURRENT_UNDO_NEXT_LSN_OFFSET, LAST_UNDO_NEXT_LSN_LFGID, "
                "LAST_UNDO_NEXT_LSN_FILENO, LAST_UNDO_NEXT_LSN_OFFSET, "
                "LAST_UNDO_NEXT_SN, SLOT_NO, "
                "UPDATE_SIZE, ENABLE_ROLLBACK, FIRST_UPDATE_TIME, "
                "LOG_BUF_SIZE, LOG_OFFSET, SKIP_CHECK_FLAG, "
                "SKIP_CHECK_SCN_FLAG, DDL_FLAG, TSS_RID, "
                "RESOURCE_GROUP_ID, LEGACY_TRANS_COUNT, ISOLATION_LEVEL, "
                "PROCESSED_UNDO_TIME, ESTIMATED_TOTAL_UNDO_TIME,"
                "TOTAL_LOG_COUNT, "
                "TOTAL_UNDO_LOG_COUNT, PROCESSED_UNDO_LOG_COUNT ) "
            "AS SELECT "
                "ID, SESSION_ID, MIN_MEM_VIEW_SCN, MIN_MEM_VIEW_SCN_FOR_LOB, "
                "MIN_DISK_VIEW_SCN, MIN_DISK_VIEW_SCN_FOR_LOB, COMMIT_SCN, "
                "STATUS, UPDATE_STATUS, LOG_TYPE, "
                "XA_COMMIT_STATUS, XA_PREPARED_TIME, "
                "CAST(0 AS INTEGER), FIRST_UNDO_NEXT_LSN_FILENO, "
                "FIRST_UNDO_NEXT_LSN_OFFSET, CURRENT_UNDO_NEXT_SN, "
                "CAST(0 AS INTEGER), CURRENT_UNDO_NEXT_LSN_FILENO, "
                "CURRENT_UNDO_NEXT_LSN_OFFSET, CAST(0 AS INTEGER), "
                "LAST_UNDO_NEXT_LSN_FILENO, LAST_UNDO_NEXT_LSN_OFFSET, "
                "LAST_UNDO_NEXT_SN, SLOT_NO, "
                "UPDATE_SIZE, ENABLE_ROLLBACK, FIRST_UPDATE_TIME, "
                "LOG_BUF_SIZE, LOG_OFFSET, SKIP_CHECK_FLAG, "
                "SKIP_CHECK_SCN_FLAG, DDL_FLAG, TSS_RID, "
                "RESOURCE_GROUP_ID, LEGACY_TRANS_COUNT, ISOLATION_LEVEL, "
                "PROCESSED_UNDO_TIME, ESTIMATED_TOTAL_UNDO_TIME,"
                "TOTAL_LOG_COUNT, " 
                "CAST( TOTAL_LOG_COUNT - PROCESSED_UNDO_LOG_COUNT AS BIGINT ),"
                "PROCESSED_UNDO_LOG_COUNT "
            "FROM X$TRANSACTIONS",

    /* PROJ-1704 Disk MVCC Renewal */
    (SChar*)"CREATE VIEW V$TXSEGS "
            "AS SELECT * "  
            "FROM X$ACTIVE_TXSEGS",

    (SChar*)"CREATE VIEW V$UDSEGS "
            "( SPACE_ID, SEG_PID, TXSEG_ENTRY_ID, "
            "  CUR_ALLOC_EXTENT_RID, CUR_ALLOC_PAGE_ID, "
            "  TOTAL_EXTENT_COUNT, TOTAL_EXTDIR_COUNT, "
            "  PAGE_COUNT_IN_EXTENT ) "
            "AS SELECT "
            "  SPACE_ID, SEG_PID, TXSEG_ENTRY_ID, "
            "  CUR_ALLOC_EXTENT_RID, CUR_ALLOC_PAGE_ID, "
            "  TOTAL_EXTENT_COUNT, TOTAL_EXTDIR_COUNT, "
            "  PAGE_COUNT_IN_EXTENT "
            " FROM X$UDSEGS",

    (SChar*)"CREATE VIEW V$TSSEGS "
            "( SPACE_ID, SEG_PID, TXSEG_ENTRY_ID, "
            "  CUR_ALLOC_EXTENT_RID, CUR_ALLOC_PAGE_ID, "
            "  TOTAL_EXTENT_COUNT, TOTAL_EXTDIR_COUNT, "
            "  PAGE_COUNT_IN_EXTENT ) "
            "AS SELECT "
            "  SPACE_ID, SEG_PID, TXSEG_ENTRY_ID, "
            "  CUR_ALLOC_EXTENT_RID,  CUR_ALLOC_PAGE_ID, "
            "  TOTAL_EXTENT_COUNT, TOTAL_EXTDIR_COUNT, "
            "  PAGE_COUNT_IN_EXTENT "
            " FROM X$TSSEGS",

    (SChar*)"CREATE VIEW V$SESSION_WAIT "
            "( SID, SEQNUM, EVENT, P1, P2, P3, "
            "WAIT_CLASS_ID, WAIT_CLASS, WAIT_TIME, SECOND_IN_TIME ) "
            "AS SELECT /*+ USE_HASH( A, B, C, D ) */ "
            "A.ID SID, B.SEQNUM, C.EVENT, B.P1, B.P2, "
            "B.P3, C.WAIT_CLASS_ID, D.WAIT_CLASS, "
            "B.WAIT_TIME, B.SECOND_IN_TIME "
            "FROM X$SESSION A, X$STATEMENT B, X$WAIT_EVENT_NAME C, "
            "X$WAIT_CLASS_NAME D  "
            "WHERE "
            "A.CURRENT_STMT_ID=B.ID AND B.SEQNUM = C.EVENT_ID AND "
            "C.WAIT_CLASS_ID = D.WAIT_CLASS_ID",

    (SChar*)"CREATE VIEW V$TRANSACTION_MGR "
                "( TOTAL_COUNT, FREE_LIST_COUNT, BEGIN_ENABLE, ACTIVE_COUNT, SYS_MIN_DISK_VIEWSCN ) "
            "AS SELECT "
                "TOTAL_COUNT, FREE_LIST_COUNT, BEGIN_ENABLE, ACTIVE_COUNT, SYS_MIN_DISK_VIEWSCN "
            "FROM X$TRANSACTION_MANAGER",

    //PROJ-1436 SQL-Plan Cache
    (SChar*)"CREATE VIEW V$SQL_PLAN_CACHE "
             "( MAX_CACHE_SIZE, CURRENT_HOT_LRU_SIZE ,CURRENT_COLD_LRU_SIZE, "
               "CURRENT_CACHE_SIZE, CURRENT_CACHE_OBJ_COUNT,CACHE_HIT_COUNT, "
               "CACHE_MISS_COUNT, CACHE_IN_FAIL_COUNT, CACHE_OUT_COUNT, "
               "CACHE_INSERTED_COUNT, NONE_CACHE_SQL_TRY_COUNT ) "
            "AS SELECT "
               "MAX_CACHE_SIZE, CURRENT_HOT_LRU_SIZE ,CURRENT_COLD_LRU_SIZE, "
               "CURRENT_CACHE_SIZE, CURRENT_CACHE_OBJ_CNT, CACHE_HIT_COUNT, "
               "CACHE_MISS_COUNT, CACHE_IN_FAIL_COUNT, CACHE_OUT_COUNT, "
               "CACHE_INSERTED_COUNT, NONE_CACHE_SQL_TRY_COUNT "
            "FROM X$SQL_PLAN_CACHE",

    (SChar*)"CREATE VIEW V$SQL_PLAN_CACHE_SQLTEXT "
             "( SQL_TEXT_ID, SQL_TEXT, CHILD_PCO_COUNT, CHILD_PCO_CREATE_COUNT, PLAN_CACHE_KEEP ) "
            "AS SELECT "
               "SQL_TEXT_ID, SQL_TEXT, CHILD_PCO_COUNT, CHILD_PCO_CREATE_COUNT, "
               "DECODE( PLAN_CACHE_KEEP, 0, 'UNKEEP', "
               "                         1, 'KEEP', "
               "                         'UNKEEP' ) PLAN_CACHE_KEEP "
            "FROM X$SQL_PLAN_CACHE_SQLTEXT",

    (SChar*)"CREATE VIEW V$SQL_PLAN_CACHE_PCO "
             "( SQL_TEXT_ID, PCO_ID, CREATE_REASON, HIT_COUNT, REBUILD_COUNT, PLAN_STATE, LRU_REGION, PLAN_SIZE, FIX_COUNT, PLAN_CACHE_KEEP ) "
            "AS SELECT "
            "A.SQL_TEXT_ID, A.PCO_ID ,"
            "DECODE( A.CREATE_REASON, 0, 'CREATED_BY_CACHE_MISS', "
            "                         1, 'CREATED_BY_PLAN_INVALIDATION', "
            "                         2, 'CREATED_BY_PLAN_TOO_OLD ') CREATE_REASON ,"
            "A.HIT_COUNT, "
            "A.REBUILD_COUNT, "
            "DECODE( A.PLAN_STATE, 0, 'NOT_READY', "
            "                      1, 'READY', "
            "                      2, 'HARD-PREPARE-NEED', " 
            "                      3, 'OLD_PLAN' ) PLAN_STATE , "
            "DECODE( A.LRU_REGION, 0, 'NONE', "
            "                      1, 'COLD_REGION', "
            "                      2, 'HOT_REGION' ) LRU_REGION, "
            "A.PLAN_SIZE, "
            "A.FIX_COUNT, "
            "DECODE( PLAN_CACHE_KEEP, 0, 'UNKEEP', "
            "                         1, 'KEEP', "
            "                         'UNKEEP' ) PLAN_CACHE_KEEP "
           "FROM X$SQL_PLAN_CACHE_PCO A",
    
    (SChar*)"CREATE VIEW V$XID   "
             "(XID_VALUE, ASSOC_SESSION_ID,TRANS_ID, STATE, STATE_START_TIME, STATE_DURATION, TX_BEGIN_FLAG, REF_COUNT ) "
            "AS SELECT "
            "A.XID_VALUE  ,"
            "A.ASSOC_SESSION_ID  ,"
            "A.TRANS_ID , "
            "DECODE(A.STATE  , 0, 'IDLE' ,    "
            "                  1, 'ACTIVE',   "
            "                  2, 'PREPARED',  "
            "                  3, 'HEURISTICALLY_COMMITED',  "                    
            "                  4, 'HEURISTICALLY_ROLLBACKED',  "
            "                  5, 'NO_TX',  "
            "                  6, 'ROLLBACK_ONLY',  "
            "                  'UNKNOWN') STATE , "
            "A.STATE_START_TIME, "
            "A.STATE_DURATION, "
            "DECODE(A.TX_BEGIN_FLAG, 0, 'NOT-BEGIN',   "
            "                               1, 'BEGIN') TX_BEGIN_FLAG,   "
            "A.REF_COUNT   "
            "FROM X$XID A",
    
    (SChar*)"CREATE VIEW V$DB_FREEPAGELISTS "
                "(SPACE_ID, RESOURCE_GROUP_ID, FIRST_FREE_PAGE_ID, FREE_PAGE_COUNT ) "
            "AS SELECT "
                "SPACE_ID, RESOURCE_GROUP_ID, FIRST_FREE_PAGE_ID, FREE_PAGE_COUNT "
            "FROM X$MEM_TABLESPACE_FREE_PAGE_LIST ",

    (SChar*)"CREATE VIEW V$FLUSHINFO "
                "( LOW_FLUSH_LENGTH, HIGH_FLUSH_LENGTH, LOW_PREPARE_LENGTH, "
                "   CHECKPOINT_FLUSH_COUNT, FAST_START_IO_TARGET,"
                "   FAST_START_LOGFILE_TARGET, REQ_JOB_COUNT ) "
            "AS SELECT "
                "LOW_FLUSH_LENGTH, HIGH_FLUSH_LENGTH, LOW_PREPARE_LENGTH, "
                "CHECKPOINT_FLUSH_COUNT, FAST_START_IO_TARGET,"
                "FAST_START_LOGFILE_TARGET, REQ_JOB_COUNT FROM   X$FLUSH_MGR",

    (SChar*)"CREATE VIEW V$FLUSHER "
                "( ID, ALIVE, CURRENT_JOB, DOING_IO, INIOB_COUNT,"
                " REPLACE_FLUSH_JOBS, REPLACE_FLUSH_PAGES, REPLACE_SKIP_PAGES,"
                " CHECKPOINT_FLUSH_JOBS, CHECKPOINT_FLUSH_PAGES, CHECKPOINT_SKIP_PAGES,"
                " OBJECT_FLUSH_JOBS, OBJECT_FLUSH_PAGES, OBJECT_SKIP_PAGES,"
                " LAST_SLEEP_SEC, TIMEOUT, SIGNALED, TOTAL_SLEEP_SEC,"
                "TOTAL_FLUSH_PAGES, TOTAL_LOG_SYNC_USEC, TOTAL_DW_USEC, "
                "TOTAL_WRITE_USEC, TOTAL_SYNC_USEC, TOTAL_FLUSH_TEMP_PAGES, "
                "TOTAL_TEMP_WRITE_USEC,TOTAL_CALC_CHECKSUM_USEC,"
                "DB_WRITE_PERF,TEMP_WRITE_PERF) "
            "AS SELECT "
                "ID, ALIVE, CURRENT_JOB, DOING_IO, INIOB_COUNT,"
                "REPLACE_FLUSH_JOBS, REPLACE_FLUSH_PAGES, REPLACE_SKIP_PAGES,"
                "CHECKPOINT_FLUSH_JOBS, CHECKPOINT_FLUSH_PAGES, CHECKPOINT_SKIP_PAGES,"
                "OBJECT_FLUSH_JOBS, OBJECT_FLUSH_PAGES, OBJECT_SKIP_PAGES,"
                "LAST_SLEEP_SEC, TIMEOUT, SIGNALED, TOTAL_SLEEP_SEC,"
                "TOTAL_FLUSH_PAGES, TOTAL_LOG_SYNC_USEC, TOTAL_DW_USEC, "
                "TOTAL_WRITE_USEC, TOTAL_SYNC_USEC, TOTAL_FLUSH_TEMP_PAGES, "
                "TOTAL_TEMP_WRITE_USEC,TOTAL_CALC_CHECKSUM_USEC,"
                "CAST( CASE WHEN TOTAL_WRITE_USEC + TOTAL_SYNC_USEC = 0 THEN 0 " 
                "ELSE "
                "( TOTAL_FLUSH_PAGES * 8192 / 1024 ) "
                "/ ( (TOTAL_WRITE_USec + TOTAL_SYNC_USec)/1000/1000 ) END AS DOUBLE),"
                "CAST( CASE WHEN TOTAL_TEMP_WRITE_USec = 0 THEN 0 ELSE "
                "( TOTAL_FLUSH_TEMP_PAGES * 8192 / 1024 ) "
                "/ ( (TOTAL_TEMP_WRITE_USec )/1000/1000 ) END AS DOUBLE) "
                "FROM X$FLUSHER",

    (SChar*)"CREATE VIEW V$LATCH "
                "(SPACE_ID,PAGE_ID, TRY_READ_LATCH, READ_SUCCESS_IMME, "
                "READ_MISS, TRY_WRITE_LATCH,WRITE_SUCCESS_IMME, WRITE_MISS, "
                "SLEEPS_CNT) "
            "AS SELECT "
                "SPACE_ID, PAGE_ID, LATCH_READ_GETS, "
                "CAST(LATCH_READ_GETS - LATCH_READ_MISSES AS BIGINT), "
                "LATCH_READ_MISSES, LATCH_WRITE_GETS, "
                "CAST(LATCH_WRITE_GETS - LATCH_WRITE_MISSES AS BIGINT), "
                "LATCH_WRITE_MISSES, LATCH_SLEEPS "
            "FROM X$BCB "
            "WHERE LATCH_READ_GETS > 0 "
                "OR LATCH_WRITE_GETS > 0 "
            "ORDER BY SPACE_ID, PAGE_ID",

    // PRJ-1497 SM page size 32K -> 8K
    (SChar*)"CREATE VIEW V$MEMTBL_INFO "
                "(TABLESPACE_ID, TABLE_OID, MEM_PAGE_CNT, "
                "MEM_VAR_PAGE_CNT, MEM_SLOT_PERPAGE, MEM_SLOT_SIZE, "
                "FIXED_ALLOC_MEM, FIXED_USED_MEM, VAR_ALLOC_MEM, "
                "VAR_USED_MEM, MEM_FIRST_PAGEID, "
                "STATEMENT_REBUILD_COUNT,"
                "UNIQUE_VIOLATION_COUNT, UPDATE_RETRY_COUNT, "
                "DELETE_RETRY_COUNT, COMPRESSED_LOGGING, IS_CONSISTENT ) "
            "AS SELECT "
                "TABLESPACE_ID, TABLE_OID, MEM_PAGE_CNT, "
                "MEM_VAR_PAGE_CNT, MEM_SLOT_CNT, MEM_SLOT_SIZE, "
                "CAST(MEM_PAGE_CNT * 32768 AS DOUBLE), FIXED_USED_MEM, "
                "CAST(MEM_VAR_PAGE_CNT * 32768 AS DOUBLE), VAR_USED_MEM, "
                "MEM_FIRST_PAGEID, "
                "STATEMENT_REBUILD_COUNT,"
                "UNIQUE_VIOLATION_COUNT, UPDATE_RETRY_COUNT, "
                "DELETE_RETRY_COUNT, COMPRESSED_LOGGING, IS_CONSISTENT "

            "FROM X$TABLE_INFO "
            "WHERE TABLE_TYPE <> 12288",

    (SChar*)"CREATE VIEW V$DISKTBL_INFO "
                "(TABLESPACE_ID, TABLE_OID, DISK_TOTAL_PAGE_CNT, "
                "DISK_PAGE_CNT, SEG_PID, META_PAGE, FST_EXTRID, LST_EXTRID, "
                "PCTFREE, PCTUSED, INIT_TRANS, MAX_TRANS, "
                "INITEXTENTS, NEXTEXTENTS, MINEXTENTS, MAXEXTENTS, "
                "COMPRESSED_LOGGING, IS_CONSISTENT ) "
            "AS SELECT "
                "TABLESPACE_ID, TABLE_OID, DISK_TOTAL_PAGE_CNT, "
                "DISK_PAGE_CNT, SEG_PID, META_PAGE, FST_EXTRID, LST_EXTRID, "
                "PCTFREE, PCTUSED, INIT_TRANS, MAX_TRANS, "
                "INITEXTENTS, NEXTEXTENTS, MINEXTENTS, MAXEXTENTS, "
                "COMPRESSED_LOGGING, IS_CONSISTENT "
            "FROM X$TABLE_INFO "
            "WHERE TABLE_TYPE = 12288 ",

    // lock performance view.
    (SChar*)"CREATE VIEW V$LOCK "
               "( LOCK_ITEM_TYPE, TBS_ID, TABLE_OID, DBF_ID, TRANS_ID, "
               "LOCK_DESC, LOCK_CNT, IS_GRANT ) "
            "AS SELECT /*+ USE_HASH( A, B ) */ "
               "DECODE(A.LOCK_ITEM_TYPE, 0, 'NONE', "
               "       1,'TBS', 2, 'TBL', 3, 'DBF', 'UNKNOWN') LOCK_ITEM_TYPE, "
               "A.TBS_ID, A.TABLE_OID, BIGINT'-1' AS DBF_ID, "
               "A.TRANS_ID, B.LOCK_DESC, A.LOCK_CNT, A.IS_GRANT "
               "FROM X$LOCK A, X$LOCK_MODE B "
               "WHERE A.LOCK_MODE = B.LOCK_MODE "
            "UNION "
               "SELECT /*+ USE_HASH( A, B ) */ "
               "DECODE(A.LOCK_ITEM_TYPE, 0, 'NONE', "
               "       1,'TBS', 2, 'TBL', 3, 'DBF', 'UNKNOWN') LOCK_ITEM_TYPE, "
               "A.TBS_ID, BIGINT'-1' AS TABLE_OID, A.DBF_ID, "
               "A.TRANS_ID, B.LOCK_DESC, A.LOCK_CNT, A.IS_GRANT FROM "
               "X$LOCK_TABLESPACE A, X$LOCK_MODE B "
               "WHERE A.LOCK_MODE = B.LOCK_MODE ",

    (SChar*)"CREATE VIEW V$LOCK_STATEMENT "
                "( SESSION_ID, ID, TX_ID, QUERY,STATE,BEGIN_FLAG, "
                "LOCK_ITEM_TYPE, TBS_ID, TABLE_OID, DBF_ID, LOCK_DESC, LOCK_CNT, IS_GRANT ) "
            "AS SELECT /*+ ORDERED USE_HASH( A, B ) USE_HASH( B, C )  */ "
                "B.SESSION_ID, B.ID, B.TX_ID, B.QUERY, B.STATE, "
                "B.BEGIN_FLAG, "
               "DECODE(A.LOCK_ITEM_TYPE, 0, 'NONE', "
               "       1,'TBS', 2, 'TBL', 3, 'DBF', 'UNKNOWN') LOCK_ITEM_TYPE, "
                "A.TBS_ID, A.TABLE_OID , BIGINT'-1' AS DBF_ID, "
                "C.LOCK_DESC, A.LOCK_CNT, A.IS_GRANT "
            "FROM X$STATEMENT B ,X$LOCK A, X$LOCK_MODE C "
            "WHERE A.TRANS_ID = B.TX_ID AND A.LOCK_MODE = C.LOCK_MODE "
            "UNION "
            "SELECT /*+ ORDERED USE_HASH( A, B ) USE_HASH( B, C )  */ "
               "B.SESSION_ID, B.ID, B.TX_ID, B.QUERY, B.STATE, "
               "B.BEGIN_FLAG, "
               "DECODE(A.LOCK_ITEM_TYPE, 0, 'NONE', "
               "       1,'TBS', 2, 'TBL', 3, 'DBF', 'UNKNOWN') LOCK_ITEM_TYPE, "
                "A.TBS_ID, BIGINT'-1' AS TABLE_OID , "
                "A.DBF_ID, C.LOCK_DESC, A.LOCK_CNT, A.IS_GRANT "
            "FROM X$STATEMENT B ,X$LOCK_TABLESPACE A, X$LOCK_MODE C "
            "WHERE A.TRANS_ID = B.TX_ID AND A.LOCK_MODE = C.LOCK_MODE",

    (SChar*)"CREATE VIEW V$MUTEX "
                "( NAME, TRY_COUNT, LOCK_COUNT, MISS_COUNT, SPIN_VALUE, "
        "  TOTAL_LOCK_TIME_US, MAX_LOCK_TIME_US, THREAD_ID ) "
            "AS SELECT "
             "CASE WHEN IDLE = 1 THEN 'IDLE' "
            "ELSE NAME END AS NAME, "
                "TRY_COUNT, LOCK_COUNT, MISS_COUNT, SPIN_VALUE,  "
        "TOTAL_LOCK_TIME_US, MAX_LOCK_TIME_US, THREAD_ID "
            "FROM X$MUTEX",

    (SChar*)"CREATE VIEW V$VERSION "
                "( PRODUCT_VERSION, PKG_BUILD_PLATFORM_INFO, "
                "PRODUCT_TIME, SM_VERSION, META_VERSION, PROTOCOL_VERSION, "
                "REPL_PROTOCOL_VERSION ) "
            "AS SELECT "
                "PRODUCT_VERSION, PKG_BUILD_PLATFORM_INFO, "
                "PRODUCT_TIME, SM_VERSION, META_VERSION, PROTOCOL_VERSION, "
                "REPL_PROTOCOL_VERSION "
            "FROM X$VERSION",

    (SChar*)"CREATE VIEW V$SEQ "
                "( SEQ_OID, CURRENT_SEQ, START_SEQ, INCREMENT_SEQ, "
                "CACHE_SIZE, MAX_SEQ, MIN_SEQ, IS_CYCLE ) "
            "AS SELECT "
                "SEQ_OID, CURRENT_SEQ, START_SEQ, INCREMENT_SEQ, "
                "SYNC_INTERVAL CACHE_SIZE, MAX_SEQ, MIN_SEQ, "
                "DECODE(NUMAND(FLAG,16),0,'NO',16,'YES','UNKNOWN') IS_CYCLE "
            "FROM X$SEQ",

    (SChar*)"CREATE VIEW V$INSTANCE "
                "( STARTUP_PHASE, STARTUP_TIME_SEC, WORKING_TIME_SEC ) "
            "AS SELECT "
                "DECODE(A.STARTUP_PHASE,0, 'INIT',1, 'PRE-PROCESS', "
                "       2,'PROCESS', 3, 'CONTROL', 4, 'META', "
                "       5, 'SERVICE', 7, 'DOWNGRADE', 'UNKNOWN STATE') STARTUP_PHASE, "
                "A.STARTUP_TIME_SEC, A.WORKING_TIME_SEC "
            "FROM X$INSTANCE A",

    (SChar*)"CREATE VIEW V$SERVICE_THREAD "
                "( ID, TYPE, STATE,RUN_MODE, SESSION_ID, STATEMENT_ID, START_TIME, "
                "EXECUTE_TIME, TASK_COUNT, READY_TASK_COUNT, THREAD_ID ) "
            "AS SELECT "
                "A.ID, "
                "DECODE(A.TYPE,  0, 'SOCKET(MULTIPLEXING)', "
                "                1, 'SOCKET(DEDICATED)', "
                "                2, 'IPC', "
                "                4, 'IPCDA', "
                "                'UNKNOWN') AS TYPE, "
                "DECODE(A.STATE, 0, 'NONE', "
                "                1, 'POLL', "
                "                2, 'EXECUTE', "
                "                3, 'QUEUE-WAIT', "
                "                'UNKNOWN') STATE, "
                "DECODE(A.RUN_MODE,0, 'SHARED', "
                "                  1, 'DEDICATED', "
                "                  'UNKNOWN') AS RUN_MODE, "
                "A.SESSION_ID, A.STATEMENT_ID, A.START_TIME, A.EXECUTE_TIME, "
                "A.TASK_COUNT, A.READY_TASK_COUNT, A.THREAD_ID "
            "FROM X$SERVICE_THREAD A",

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       BUG-29335A performance-view about service thread should be strengthened for problem tracking
    */
        (SChar*)"CREATE VIEW V$SERVICE_THREAD_MGR "
                "(ADD_THR_COUNT , REMOVE_THR_COUNT) "
            "AS SELECT "
                "A.ADD_THR_COUNT, "
                "A. REMOVE_THR_COUNT "
            "FROM  X$SERVICE_THREAD_MGR  A",

    //[TASK-6757]LFG,SN 제거 : LFG_ID 는 0으로 출력함  
    (SChar*)"CREATE VIEW V$LOG"
                "( BEGIN_CHKPT_LFGID, BEGIN_CHKPT_FILE_NO, "
                "BEGIN_CHKPT_FILE_OFFSET, END_CHKPT_LFGID, END_CHKPT_FILE_NO, "
                "END_CHKPT_FILE_OFFSET, SERVER_STATUS, "
                "ARCHIVELOG_MODE, TRANSACTION_SEGMENT_COUNT, " 
                " OLDEST_LFGID, OLDEST_LOGFILE_NO, OLDEST_LOGFILE_OFFSET ) "
            "AS SELECT "
                "CAST(0 AS INTEGER), BEGIN_CHKPT_FILE_NO, "
                "BEGIN_CHKPT_FILE_OFFSET, CAST(0 AS INTEGER), "
                "END_CHKPT_FILE_NO, END_CHKPT_FILE_OFFSET, "
                "DECODE(SERVER_STATUS, 0,'SERVER SHUTDOWN', "
                "       1, 'SERVER STARTED', 'UNKNOWN  STATE') SERVER_STATUS, "
                "DECODE(ARCHIVE_MODE, 0, 'NOARCHIVE', 1, 'ARCHIVE', "
                "       'UNKNOWN MODE') ARCHIVELOG_MODE, "
                "TRANSACTION_SEGMENT_COUNT, "
                "CAST(0 AS INTEGER), OLDEST_LOGFILE_NO, "
                "OLDEST_LOGFILE_OFFSET "
             "FROM X$LOG",

    (SChar*)"CREATE VIEW V$STABLE_MEM_DATAFILES "
                "( MEM_DATA_FILE )"
            "AS SELECT "
                "MEM_DATA_FILE "
            "FROM X$STABLE_MEM_DATAFILES",

    (SChar*)"CREATE VIEW V$STATNAME "
                "( SEQNUM, NAME ) "
            "AS SELECT "
                "SEQNUM, NAME "
            "FROM X$STATNAME",

    (SChar*)"CREATE VIEW V$SYSSTAT "
                "( SEQNUM, NAME, VALUE ) "
            "AS SELECT /*+ USE_HASH( A, B ) */ "
                "A.SEQNUM, A.NAME, B.VALUE "
            "FROM X$STATNAME A, X$SYSSTAT B "
            "WHERE A.SEQNUM = B.SEQNUM ",

    (SChar*)"CREATE VIEW V$SESSTAT "
                "( SID, SEQNUM, NAME, VALUE ) "
            "AS SELECT /*+ USE_HASH( A, B ) */ "
                "B.SID, A.SEQNUM, A.NAME, B.VALUE "
            "FROM X$STATNAME A, X$SESSTAT B "
            "WHERE A.SEQNUM = B.SEQNUM ",

    (SChar*)"CREATE VIEW V$WAIT_CLASS_NAME"
                "( WAIT_CLASS_ID, WAIT_CLASS ) "
            "AS SELECT "
                " WAIT_CLASS_ID, WAIT_CLASS "
            "FROM X$WAIT_CLASS_NAME ",

    (SChar*)"CREATE VIEW V$SYSTEM_WAIT_CLASS "
                 "( WAIT_CLASS_ID, WAIT_CLASS, TOTAL_WAITS, TIME_WAITED ) "
                 " AS SELECT /*+ USE_HASH( A, B, C ) */ "
                      " C.WAIT_CLASS_ID, C.WAIT_CLASS, "
                      " SUM(A.TOTAL_WAITS) TOTAL_WAITS, "
                      " CAST(SUM(A.TIME_WAITED_MICRO)/1000 AS DOUBLE) TIME_WAITED "
                 " FROM  "
                 " X$SYSTEM_EVENT A, "
                 " X$WAIT_EVENT_NAME B, "
                 " X$WAIT_CLASS_NAME C "
                 " WHERE "
                 " A.EVENT_ID = B.EVENT_ID AND "
                 " B.WAIT_CLASS_ID = C.WAIT_CLASS_ID "
                 " GROUP BY C.WAIT_CLASS_ID, C.WAIT_CLASS ",

    (SChar*)"CREATE VIEW V$SESSION_WAIT_CLASS "
                 "( SID, SERIAL, WAIT_CLASS_ID, "
                 "  WAIT_CLASS, TOTAL_WAITS, TIME_WAITED ) "
                 " AS SELECT /*+ USE_HASH( A, B, C ) */ "
                      " A.SID, A.EVENT_ID SERIAL, "
                      " C.WAIT_CLASS_ID, C.WAIT_CLASS, "
                      " SUM(A.TOTAL_WAITS) TOTAL_WAITS, "
                      " CAST(SUM(A.TIME_WAITED_MICRO)/1000 AS DOUBLE) TIME_WAITED "
                 " FROM  "
                 " X$SESSION_EVENT A, "
                 " X$WAIT_EVENT_NAME B, "
                 " X$WAIT_CLASS_NAME C "
                 " WHERE "
                 " A.EVENT_ID = B.EVENT_ID AND "
                 " B.WAIT_CLASS_ID = C.WAIT_CLASS_ID "
                 " GROUP BY A.SID, A.EVENT_ID, "
                 " C.WAIT_CLASS_ID, C.WAIT_CLASS ",

     (SChar*)"CREATE VIEW V$EVENT_NAME "
                 " ( EVENT_ID, NAME, WAIT_CLASS_ID, WAIT_CLASS ) "
             " AS SELECT /*+ USE_HASH( A, B ) */ "
                 " A.EVENT_ID, A.EVENT NAME, A.WAIT_CLASS_ID, B.WAIT_CLASS "
             " FROM X$WAIT_EVENT_NAME A, X$WAIT_CLASS_NAME B "
             " WHERE A.WAIT_CLASS_ID = B.WAIT_CLASS_ID ",

    (SChar*)"CREATE VIEW V$SYSTEM_EVENT "
              " ( EVENT, TOTAL_WAITS, TOTAL_TIMEOUTS, TIME_WAITED, "
              " AVERAGE_WAIT, TIME_WAITED_MICRO, EVENT_ID, "
               " WAIT_CLASS_ID, WAIT_CLASS ) "
            " AS SELECT /*+ USE_HASH( A, B, C ) */ "
                " B.EVENT, "
                " A.TOTAL_WAITS, "
                " A.TOTAL_TIMEOUTS, "
                " A.TIME_WAITED, "
                " A.AVERAGE_WAIT, "
                " A.TIME_WAITED_MICRO, "
                " A.EVENT_ID, "
                " B.WAIT_CLASS_ID, "
                " C.WAIT_CLASS "
            " FROM "
                " X$SYSTEM_EVENT A, "
                " X$WAIT_EVENT_NAME B,"
                " X$WAIT_CLASS_NAME C "
            " WHERE "
                " A.EVENT_ID = B.EVENT_ID AND "
                " B.WAIT_CLASS_ID = C.WAIT_CLASS_ID ",

    (SChar*)"CREATE VIEW V$SESSION_EVENT"
                "( SID, EVENT, TOTAL_WAITS, TOTAL_TIMEOUTS, TIME_WAITED, "
                " AVERAGE_WAIT, MAX_WAIT, TIME_WAITED_MICRO, EVENT_ID, "
                " WAIT_CLASS_ID, WAIT_CLASS ) "
            "AS SELECT /*+ USE_HASH( A, B, C ) */ "
               " A.SID, "
               " B.EVENT, "
               " A.TOTAL_WAITS, "
               " A.TOTAL_TIMEOUTS, "
               " A.TIME_WAITED, "
               " A.AVERAGE_WAIT, "
               " A.MAX_WAIT, "
               " A.TIME_WAITED_MICRO, "
               " A.EVENT_ID, "
               " B.WAIT_CLASS_ID, "
               " C.WAIT_CLASS "
            "FROM "
               " X$SESSION_EVENT   A, "
               " X$WAIT_EVENT_NAME B, "
               " X$WAIT_CLASS_NAME C "
            " WHERE "
            " A.EVENT_ID = B.EVENT_ID AND "
            " B.WAIT_CLASS_ID = C.WAIT_CLASS_ID",

    (SChar*)"CREATE VIEW V$FILESTAT "
            "( SPACEID, FILEID, PHYRDS, PHYWRTS, "
            "PHYBLKRD, PHYBLKWRT, SINGLEBLKRDS, "
            "READTIM, WRITETIM, SINGLEBLKRDTIM, "
            "AVGIOTIM, LSTIOTIM, MINIOTIM, MAXIORTM, MAXIOWTM ) "
            "AS SELECT "
            "SPACEID, FILEID, PHYRDS, PHYWRTS, "
            "PHYBLKRD, PHYBLKWRT, SINGLEBLKRDS, "
            "CAST(READTIM/1000 AS DOUBLE) READTIM, CAST(WRITETIM/1000 AS DOUBLE) WRITETIM, CAST(SINGLEBLKRDTIM/1000 AS DOUBLE) SINGLEBLKRDTIM, "
            "CAST(AVGIOTIM/1000 AS DOUBLE) AVGIOTIM, CAST(LSTIOTIM/1000 AS DOUBLE) LSTIOTIM, CAST(MINIOTIM/1000 AS DOUBLE) MINIOTIM, "
            "CAST(MAXIORTM/1000 AS DOUBLE) MAXIORTM, CAST(MAXIOWTM/1000 AS DOUBLE) MAXIOWTM "
            "FROM X$FILESTAT",

    (SChar*)"CREATE VIEW V$SQLTEXT "
                "( SID, STMT_ID, PIECE, TEXT ) "
            "AS SELECT "
                "SID, STMT_ID, PIECE, TEXT "
            "FROM X$SQLTEXT",

    (SChar*)"CREATE VIEW V$PLANTEXT  "
                "( SID, STMT_ID, PIECE, TEXT ) "
            "AS SELECT "
                "SID, STMT_ID, PIECE, TEXT "
            "FROM X$PLANTEXT",

    (SChar*)"CREATE VIEW V$PROCTEXT "
                "( PROC_OID, PIECE, TEXT ) "
            "AS SELECT "
                "PROC_OID, PIECE, TEXT "
            "FROM X$PROCTEXT",

    // PROJ-1073 Package
    (SChar*)"CREATE VIEW V$PKGTEXT "
                "( PACKAGE_OID, PIECE, TEXT ) "
            "AS SELECT "
                "PACKAGE_OID, PIECE, TEXT "
            "FROM X$PKGTEXT",

    // BUG-17202
    // ODBC SPEC에 맞는 결과를 보여주기 위해
    // ODBC_DATA_TYPE과 ODBC_SQL_DATA_TYPE를 DATA_TYPE, SQL_DATA_TYPE으로 변환
    // BUG-17684 V$DATATYPE 에 DATA_TYPE(server type), ODBC_DATA_TYPE 분리

    (SChar*)"CREATE VIEW V$DATATYPE "
                "( TYPE_NAME, DATA_TYPE, ODBC_DATA_TYPE, COLUMN_SIZE, LITERAL_PREFIX, "
                "LITERAL_SUFFIX, CREATE_PARAM, NULLABLE, CASE_SENSITIVE, "
                "SEARCHABLE, UNSIGNED_ATTRIBUTE, FIXED_PREC_SCALE, "
                "AUTO_UNIQUE_VALUE, LOCAL_TYPE_NAME, MINIMUM_SCALE, "
                "MAXIMUM_SCALE, SQL_DATA_TYPE, SQL_DATETIME_SUB, "
                "NUM_PREC_RADIX, INTERVAL_PRECISION ) "
            "AS SELECT "
                "TYPE_NAME, DATA_TYPE, ODBC_DATA_TYPE, COLUMN_SIZE, LITERAL_PREFIX, "
                "LITERAL_SUFFIX, CREATE_PARAM, NULLABLE, CASE_SENSITIVE, "
                "SEARCHABLE, UNSIGNED_ATTRIBUTE, FIXED_PREC_SCALE, "
                "AUTO_UNIQUE_VALUE, LOCAL_TYPE_NAME, MINIMUM_SCALE, "
                "MAXIMUM_SCALE, ODBC_SQL_DATA_TYPE, SQL_DATETIME_SUB, "
                "NUM_PREC_RADIX, INTERVAL_PRECISION "
            "FROM X$DATATYPE "
            "ORDER BY TYPE_NAME",

    (SChar*)"CREATE VIEW V$INDEX "
                "( TABLE_OID, INDEX_SEG_PID, INDEX_ID, INDEXTYPE ) "
            "AS SELECT "
                "TABLE_OID, INDEX_SEG_PID, INDEX_ID, "
                "DECODE(INDEX_TYPE,2, 'PRIMARY','NORMAL') INDEXTYPE "
            "FROM X$INDEX",

    /* BUG-24577 V$SEGMENT에 UDSEG 와 TSSEG 정보를 출력한다 */
    /* BUG-44171 V$SEGMENT.SEGMENT_TYPE 에 LOB을 추가 */
    (SChar*)"CREATE VIEW V$SEGMENT "
                "( SPACE_ID, TABLE_OID, SEGMENT_PID, SEGMENT_TYPE, "
                "SEGMENT_STATE, EXTENT_TOTAL_COUNT ) "
            "AS SELECT "
                "SPACE_ID, TABLE_OID, SEGMENT_PID, "
                "DECODE( SEGMENT_TYPE, 5,'INDEX', 6,'TABLE', 7,'LOB', 'UNKNOWN'), "
                "DECODE( SEGMENT_STATE, 0,'FREE', 1,'USED', 'UNKNOWN'), "
                "TOTAL_EXTENT_COUNT "
                "FROM X$SEGMENT "
            "UNION  SELECT "
                "SPACE_ID, BIGINT'0' TABLE_OID, SEG_PID, "
                "DECODE( TYPE, 4,'UDSEG', 'UNKNOWN'), "
                "DECODE( STATE, 0,'FREE', 1,'USED', 'UNKNOWN'), "
                "TOTAL_EXTENT_COUNT "
                "FROM X$UDSEGS "
            "UNION  SELECT "
                "SPACE_ID, BIGINT'0' TABLE_OID, SEG_PID, "
                "DECODE( TYPE, 3,'TSSEG', 'UNKNOWN'), "
                "DECODE( STATE, 0,'FREE', 1,'USED', 'UNKNOWN'), "
                "TOTAL_EXTENT_COUNT "
                "FROM X$TSSEGS ",

    //[TASK-6757]LFG,SN 제거 : LFG_ID 는 0으로 출력함  
    (SChar*)"CREATE VIEW V$LFG "
                "( LFG_ID, CUR_WRITE_LF_NO, CUR_WRITE_LF_OFFSET, "
                "LF_OPEN_COUNT, LF_PREPARE_COUNT, LF_PREPARE_WAIT_COUNT, "
                "LST_PREPARE_LF_NO, END_LSN_LFGID, END_LSN_FILE_NO, "
                "END_LSN_OFFSET, FIRST_DELETED_LOGFILE, LAST_DELETED_LOGFILE, "
                "RESET_LSN_LFGID, RESET_LSN_FILE_NO, RESET_LSN_OFFSET, "
                "UPDATE_TX_COUNT, GC_WAIT_COUNT, GC_ALREADY_SYNC_COUNT, "
                "GC_REAL_SYNC_COUNT ) "
            "AS SELECT "
                "CAST(0 AS INTEGER), CUR_WRITE_LF_NO, CUR_WRITE_LF_OFFSET, "
                "LF_OPEN_COUNT, LF_PREPARE_COUNT, LF_PREPARE_WAIT_COUNT, "
                "LST_PREPARE_LF_NO, CAST(0 AS INTEGER), END_LSN_FILE_NO, "
                "END_LSN_OFFSET, FIRST_DELETED_LOGFILE, LAST_DELETED_LOGFILE, "
                "CAST(0 AS INTEGER), RESET_LSN_FILE_NO, RESET_LSN_OFFSET, "
                "UPDATE_TX_COUNT, GC_WAIT_COUNT, GC_ALREADY_SYNC_COUNT, "
                "GC_REAL_SYNC_COUNT "
            "FROM X$LFG",

    (SChar*)"CREATE VIEW V$LOCK_WAIT "
                "( TRANS_ID, WAIT_FOR_TRANS_ID ) "
            "AS SELECT "
                "TRANS_ID, WAIT_FOR_TRANS_ID "
            "FROM X$LOCK_WAIT",

    (SChar*)"CREATE VIEW V$TRACELOG "
                "( MODULE_NAME, TRCLEVEL, FLAG, POWLEVEL, DESCRIPTION ) "
            "AS SELECT "
                "MODULE_NAME, TRCLEVEL, FLAG, POWLEVEL, DESCRIPTION "
            "FROM X$TRACELOG",

    (SChar*)"CREATE VIEW  V$CATALOG "
                "( TABLE_OID, COLUMN_CNT, COLUMN_VAR_SLOT_CNT, "
                "INDEX_CNT, INDEX_VAR_SLOT_CNT ) "
            "AS SELECT "
                "TABLE_OID, COLUMN_CNT, COLUMN_VAR_SLOT_CNT, "
                "INDEX_CNT, INDEX_VAR_SLOT_CNT "
            "FROM X$CATALOG",

    /* TASK-4007 [SM] PBT를 위한 기능 추가
     * V$BUFFPAGEINFO의 HITRATIO계산이 잘못되는 버그를 수정합니다.*/
    /*BUG-30429     v$buffpageinfo 의 내용의 HIT-RATIO 계산시
                    FIX_PAGE_COUNT를 누락하고 있습니다*/
    (SChar*)"CREATE VIEW V$BUFFPAGEINFO "
                "( PAGE_TYPE, READ_PAGE_COUNT, "
                "GET_PAGE_COUNT, FIX_PAGE_COUNT, CREATE_PAGE_COUNT, HIT_RATIO ) "
            "AS SELECT "
                "DECODE( PAGE_TYPE, 0,'PAGE UNFORMAT',"
                "                   1,'PAGE FORMAT',"
                "                   2,'PAGE INDEX META BTREE',"
                "                   3,'PAGE INDEX META RTREE',"
                "                   4,'PAGE INDEX BTREE',"
                "                   5,'PAGE INDEX RTREE',"
                "                   6,'PAGE TABLE',"
                "                   7,'PAGE TEMP TABLE META',"
                "                   8,'PAGE TEMP TABLE DATA',"
                "                   9,'PAGE TSS',"
                "                  10,'PAGE UNDO',"
                "                  11,'PAGE LOB DATA',"
                "                  12,'PAGE LOB INODE',"
                "                  13,'PAGE FMS SEGHDR',"
                "                  14,'PAGE FMS EXTDIR',"
                "                  15,'PAGE TMS SEGHDR',"
                "                  16,'PAGE TMS LFBMP',"
                "                  17,'PAGE TMS ITBMP',"
                "                  18,'PAGE TMS RTBMP',"
                "                  19,'PAGE TMS EXTDIR',"
                "                  20,'PAGE CMS SEGHDR',"
                "                  21,'PAGE CMS EXTDIR',"
                "                  22,'PAGE FEBT FSB',"
                "                  23,'PAGE FEBT EGH',"
                "                  24,'PAGE LOB META',"
                "                  25,'PAGE HV TEMP NODE',"
                "                  26,'PAGE HV TEMP DATA',"
                "                  'UNKNOWN'),"
                "SUM(READ_PAGES), SUM(GET_PAGES), "
                "SUM(FIX_PAGES), SUM(CREATE_PAGES), "
                "CAST(CASE WHEN SUM(GET_PAGES) = 0 "
                "    THEN 0.0 "
                "    ELSE (SUM(GET_PAGES)+SUM(FIX_PAGES)-SUM(READ_PAGES))*100 "
                "        /(SUM(GET_PAGES)+SUM(FIX_PAGES)) "
                "END AS DOUBLE) "
            "FROM X$BUFFER_PAGE_INFO GROUP BY PAGE_TYPE",

    (SChar*)"CREATE VIEW V$SYSTEM_CONFLICT_PAGE"
                "( PAGE_TYPE, LATCH_MISS_CNT, LATCH_MISS_TIME ) "
            "AS SELECT "
                "DECODE( PAGE_TYPE, 0,'PAGE UNFORMAT',"
                "                   1,'PAGE FORMAT',"
                "                   2,'PAGE INDEX META BTREE',"
                "                   3,'PAGE INDEX META RTREE',"
                "                   4,'PAGE INDEX BTREE',"
                "                   5,'PAGE INDEX RTREE',"
                "                   6,'PAGE TABLE',"
                "                   7,'PAGE TEMP TABLE META',"
                "                   8,'PAGE TEMP TABLE DATA',"
                "                   9,'PAGE TSS',"
                "                  10,'PAGE UNDO',"
                "                  11,'PAGE LOB DATA',"
                "                  12,'PAGE LOB INODE',"
                "                  13,'PAGE FMS SEGHDR',"
                "                  14,'PAGE FMS EXTDIR',"
                "                  15,'PAGE TMS SEGHDR',"
                "                  16,'PAGE TMS LFBMP',"
                "                  17,'PAGE TMS ITBMP',"
                "                  18,'PAGE TMS RTBMP',"
                "                  19,'PAGE TMS EXTDIR',"
                "                  20,'PAGE CMS SEGHDR',"
                "                  21,'PAGE CMS EXTDIR',"
                "                  22,'PAGE FEBT FSB',"
                "                  23,'PAGE FEBT EGH',"
                "                  24,'PAGE LOB META',"
                "                  25,'PAGE HV TEMP NODE',"
                "                  26,'PAGE HV TEMP DATA',"
                "                  'UNKNOWN'),"
                "LATCH_MISS_CNT, LATCH_MISS_TIME "
            "FROM X$SYSTEM_CONFLICT_PAGE",

    // PROJ-1697
    (SChar*)"CREATE VIEW V$DB_PROTOCOL "
                "(OP_NAME, OP_ID, COUNT) "
            "AS SELECT "
                "OP_NAME, OP_ID, COUNT "
            "FROM X$DB_PROTOCOL",

    /* BUG-20856 XA */
    (SChar*)"CREATE VIEW V$DBA_2PC_PENDING "
                "( LOCAL_TRAN_ID, GLOBAL_TX_ID ) "
            "AS SELECT "
                "LOCAL_TRAN_ID, GLOBAL_TX_ID "
            "FROM X$TXPENDING",

    // PROJ-1579 NCHAR
    (SChar*)"CREATE VIEW V$NLS_PARAMETERS "
                "(SESSION_ID, NLS_USE, NLS_CHARACTERSET, NLS_NCHAR_CHARACTERSET, NLS_COMP, "
                 "NLS_NCHAR_CONV_EXCP, NLS_NCHAR_LITERAL_REPLACE )"
            "AS SELECT "
                "A.ID, "
                "A.CLIENT_NLS, "
                "B.NLS_CHARACTERSET, B.NLS_NCHAR_CHARACTERSET, "
                "DECODE(C.MEMORY_VALUE1, 0, 'BINARY', "
                        "         1, 'ANSI', 'UNKNOWN' "
                "      ) NLS_COMP, "
                "DECODE(A.NLS_NCHAR_CONV_EXCP, 0, 'FALSE', "
                        "                      1, 'TRUE', 'UNKNOWN' "
                "      ) NLS_NCHAR_CONV_EXCP, "
                "DECODE(A.NLS_NCHAR_LITERAL_REPLACE, 0, 'FALSE', "
                                "                    1, 'TRUE', 'UNKNOWN' "
                "      ) NLS_NCHAR_LITERAL_REPLACE "
            "FROM X$SESSION A, X$MEMBASE B, X$PROPERTY C "
            "WHERE B.SPACE_ID = 0 AND C.NAME='NLS_COMP' AND "
            "      MOD(C.ATTR,2)=0 AND A.ID = SESSION_ID() ",

    // PROJ-2068
    (SChar*)"CREATE VIEW V$DIRECT_PATH_INSERT "
                "(COMMIT_TX_COUNT, ABORT_TX_COUNT, INSERT_ROW_COUNT, "
                " ALLOC_BUFFER_PAGE_TRY_COUNT, ALLOC_BUFFER_PAGE_FAIL_COUNT) "
            "AS SELECT "
                "COMMIT_TX_COUNT, ABORT_TX_COUNT, INSERT_ROW_COUNT, "
                " ALLOC_BUFFER_PAGE_TRY_COUNT, ALLOC_BUFFER_PAGE_FAIL_COUNT "
            "FROM X$DIRECT_PATH_INSERT",

    // BUG-31100 need V$DISK_UNDO_USAGE for the users
    (SChar*)"CREATE VIEW V$DISK_UNDO_USAGE AS "
            "SELECT SUM(TX_EXT_CNT) AS TX_EXT_CNT,  "
                   "SUM(UNEXPIRED_EXT_CNT) AS USED_EXT_CNT,  "
                   "CAST (SUM(CASE WHEN IS_ONLINE='Y' THEN EXPIRED_EXT_CNT ELSE 0 END ) + SUM( UNSTEAL_EXT_CNT ) AS BIGINT)  AS  UNSTEALABLE_EXT_CNT , "
                   "CAST (SUM(CASE WHEN IS_ONLINE='Y' THEN 0 ELSE EXPIRED_EXT_CNT END ) AS BIGINT) AS REUSABLE_EXT_CNT ,  "
                   "SUM(TOT_EXT_CNT) AS TOTAL_EXT_CNT  "
            "FROM X$DISK_UNDO_SEGHDR ",

    /* TASK-4990 */
    (SChar*)"CREATE VIEW V$DBMS_STATS AS "
            "SELECT * FROM X$DBMS_STATS",

    // BUG-33711 [sm_index] add X$USAGE view
    (SChar*)"CREATE VIEW V$USAGE AS "
                "(SELECT TYPE, TARGET_ID, META_SPACE, USED_SPACE, "
                "AGEABLE_SPACE, FREE_SPACE FROM X$DBMS_STATS WHERE TYPE IN ( 'T','I'))",
    // BUG-40454 disk temp tablespace usage view
    (SChar*)"CREATE VIEW V$DISK_TEMP_STAT "
                "(TBS_ID, TRANSACTION_ID, CONSUME_TIME, "
                "READ_COUNT, WRITE_COUNT, WRITE_PAGE_COUNT, "
                "OVER_ALLOC_COUNT,ALLOC_WAIT_COUNT,WRITE_WAIT_COUNT,QUEUE_WAIT_COUNT, "
                "WORK_AREA_SIZE, MAX_WORK_AREA_SIZE, DISK_USAGE, RUNTIME_MAP_SIZE ) "
           "AS SELECT "
               "TBS_ID, TRANSACTION_ID, CONSUME_TIME, "
               "READ_COUNT, WRITE_COUNT, WRITE_PAGE_COUNT, "
               "OVER_ALLOC_COUNT,ALLOC_WAIT_COUNT,0,0,"
               "WORK_AREA_SIZE, MAX_WORK_AREA_SIZE, NORMAL_AREA_SIZE, RUNTIME_MAP_SIZE "
               "FROM X$TEMPTABLE_STATS "
               "WHERE CREATE_TIME > DROP_TIME; ",
    (SChar*)"CREATE VIEW v$DISK_TEMP_INFO  "
            "AS SELECT "
                " NAME,  VALUE,  UNIT "
                "FROM X$TEMPINFO WHERE NAME LIKE '%MAX ESTIMATED%';",
    // PROJ-2133 incremental backup
    (SChar*)"CREATE VIEW V$BACKUP_INFO "
                "(BEGIN_BACKUP_TIME, END_BACKUP_TIME, INCREMENTAL_BACKUP_CHUNK_COUNT, "
                " BACKUP_TARGET, BACKUP_LEVEL, BACKUP_TYPE, TABLESPACE_ID, FILE_ID, BACKUP_TAG, BACKUP_FILE ) "
            "AS SELECT "
                "BEGIN_BACKUP_TIME, END_BACKUP_TIME, INCREMENTAL_BACKUP_CHUNK_COUNT, "
                " BACKUP_TARGET, BACKUP_LEVEL, BACKUP_TYPE, TABLESPACE_ID, FILE_ID, BACKUP_TAG, BACKUP_FILE  "
            "FROM X$BACKUP_INFO",
    // PROJ-2133 incremental backup
    (SChar*)"CREATE VIEW V$OBSOLETE_BACKUP_INFO "
                "(BEGIN_BACKUP_TIME, END_BACKUP_TIME, INCREMENTAL_BACKUP_CHUNK_COUNT, "
                " BACKUP_TARGET, BACKUP_LEVEL, BACKUP_TYPE, TABLESPACE_ID, FILE_ID, BACKUP_TAG, BACKUP_FILE ) "
            "AS SELECT "
                "BEGIN_BACKUP_TIME, END_BACKUP_TIME, INCREMENTAL_BACKUP_CHUNK_COUNT, "
                " BACKUP_TARGET, BACKUP_LEVEL, BACKUP_TYPE, TABLESPACE_ID, FILE_ID, BACKUP_TAG, BACKUP_FILE  "
            "FROM X$OBSOLETE_BACKUP_INFO",
    /* PROJ-2208 Multi Currency */
    (SChar *)"CREATE VIEW V$NLS_TERRITORY AS "
             "SELECT NAME FROM X$NLS_TERRITORY",
    /* PROJ-2209 DBTIMEZONE */
    (SChar*)"CREATE VIEW V$TIME_ZONE_NAMES "
                "(NAME, UTC_OFFSET ) "
            "AS SELECT "
                "NAME, UTC_OFFSET FROM X$TIME_ZONE_NAMES ",

    // PROJ-1685
    (SChar*)"CREATE VIEW V$EXTPROC_AGENT"
                "( SID, PID, SOCK_FILE, CREATED, LAST_RECEIVED, LAST_SENT, STATE ) "
            "AS SELECT "
                "SID, PID, SOCK_FILE, CREATED, LAST_RECEIVED, LAST_SENT,  "
                "DECODE(STATE, 0, 'INITIALIZED', "
                "              1, 'RUNNING', "
                "              2, 'STOPPED', "
                "              3, 'FAILED', "
                "              'UNKNOWN') "
            "FROM X$EXTPROC_AGENT",

    /* PROJ-2102 Secondary Buffer */

    /* Secondary Buffer Pool  통계를 보여주는 performance view. */
    (SChar*)"CREATE VIEW V$SBUFFER_STAT "
                "(PAGE_COUNT, HASH_BUCKET_COUNT,HASH_CHAIN_LATCH_COUNT, "
                "CHECKPOINT_LIST_COUNT, HASH_PAGES, FLUSH_PAGES, CHECKPOINT_LIST_PAGES,"
                "GET_PAGES, READ_PAGES, WRITE_PAGES, HIT_RATIO, "
                "SINGLE_PAGE_READ_USEC, SINGLE_PAGE_WRITE_USEC, "
                "MPR_READ_USEC, MPR_READ_PAGE ,"
                "SINGLE_READ_PERF, MULTI_READ_PERF ) "
            "AS SELECT "
                "BUFFER_PAGES, HASH_BUCKET_COUNT,HASH_CHAIN_LATCH_COUNT, "
                "CHECKPOINT_LIST_COUNT, HASH_PAGES, FLUSH_DONE_PAGES, CHECKPOINT_LIST_PAGES,"
                "GET_PAGES, READ_PAGES, WRITE_PAGES, HIT_RATIO, "
                "SINGLE_PAGE_READ_USEC, SINGLE_PAGE_WRITE_USEC, "
                "MPR_READ_USEC, MPR_READ_PAGES, "
                "CAST( CASE WHEN SINGLE_PAGE_READ_USEC = 0 THEN 0 " 
                "ELSE ( READ_PAGES * 8192 / 1024  ) "
                "/ ( SINGLE_PAGE_READ_USEC/1000/1000 ) END AS DOUBLE),"
                "CAST( CASE WHEN MPR_READ_USEC = 0 THEN 0 " 
                "ELSE ( MPR_READ_PAGES * 8192 / 1024  ) "
                "/ ( MPR_READ_USEC/1000/1000 ) END AS DOUBLE) "
            "FROM X$SBUFFER_STAT",

    /* Secondary Buffer Flusher 각각의 통계를 보여주는 performance view. */
    (SChar*)"CREATE VIEW V$SFLUSHER "
                "( ID, ALIVE, CURRENT_JOB, DOING_IO, INIOB_COUNT,"
                "REPLACE_FLUSH_JOBS, REPLACE_FLUSH_PAGES, REPLACE_SKIP_PAGES,"
                "CHECKPOINT_FLUSH_JOBS, CHECKPOINT_FLUSH_PAGES, CHECKPOINT_SKIP_PAGES,"
                "OBJECT_FLUSH_JOBS, OBJECT_FLUSH_PAGES, OBJECT_SKIP_PAGES,"
                "LAST_SLEEP_SEC, TIMEOUT, SIGNALED, TOTAL_SLEEP_SEC,"
                "TOTAL_FLUSH_PAGES, TOTAL_DW_USEC, "
                "TOTAL_WRITE_USEC, TOTAL_SYNC_USEC,"
                "TOTAL_FLUSH_TEMP_PAGES, TOTAL_TEMP_WRITE_USEC,"
                "DB_WRITE_PERF,TEMP_WRITE_PERF ) "
            "AS SELECT "
                "ID, ALIVE, CURRENT_JOB, DOING_IO, INIOB_COUNT,"
                "REPLACE_FLUSH_JOBS, REPLACE_FLUSH_PAGES, REPLACE_SKIP_PAGES,"
                "CHECKPOINT_FLUSH_JOBS, CHECKPOINT_FLUSH_PAGES, CHECKPOINT_SKIP_PAGES,"
                "OBJECT_FLUSH_JOBS, OBJECT_FLUSH_PAGES, OBJECT_SKIP_PAGES,"
                "LAST_SLEEP_SEC, TIMEOUT, SIGNALED, TOTAL_SLEEP_SEC,"
                "TOTAL_FLUSH_PAGES, TOTAL_DW_USEC, "
                "TOTAL_WRITE_USEC, TOTAL_SYNC_USEC,"
                "TOTAL_FLUSH_TEMP_PAGES, TOTAL_TEMP_WRITE_USEC,"
                "CAST( CASE WHEN TOTAL_WRITE_USEC + TOTAL_SYNC_USEC = 0 THEN 0 " 
                "ELSE "
                "( TOTAL_FLUSH_PAGES * 8192 / 1024 ) "
                "/ ( (TOTAL_WRITE_USec + TOTAL_SYNC_USec)/1000/1000 ) END AS DOUBLE),"
                "CAST( CASE WHEN TOTAL_TEMP_WRITE_USec = 0 THEN 0 ELSE "
                "( TOTAL_FLUSH_TEMP_PAGES * 8192 / 1024 ) "
                "/ ( (TOTAL_TEMP_WRITE_USec )/1000/1000 ) END AS DOUBLE) "
                "FROM X$SBUFFER_FLUSHER",
 
    /* Secondary Buffer FlushMgr의 통계를 보여주는 performance view. */
    (SChar*)"CREATE VIEW V$SFLUSHINFO "
                "( FLUSHER_COUNT, CHECKPOINT_LIST_COUNT, REQ_JOB_COUNT, "
                "REPLACE_PAGES,  CHECKPOINT_PAGES, "
                "MIN_BCB_ID, MIN_SPACEID, MIN_PAGEID )"
            "AS SELECT "
                "FLUSHER_COUNT, CHECKPOINT_LIST_COUNT, REQ_JOB_COUNT, "
                "REPLACE_PAGES, CHECKPOINT_PAGES, "
                "MIN_BCB_ID, MIN_SPACEID, MIN_PAGEID "
            "FROM X$SBUFFER_FLUSH_MGR",

    /* PROJ-2451 Concurrent Execute Package */
    (SChar*)"CREATE VIEW V$INTERNAL_SESSION "
                "( ID, TRANS_ID, "
                "QUERY_TIME_LIMIT, DDL_TIME_LIMIT, FETCH_TIME_LIMIT, "
                "UTRANS_TIME_LIMIT, IDLE_TIME_LIMIT, IDLE_START_TIME, "
                "ACTIVE_FLAG, OPENED_STMT_COUNT, "
                "DB_USERNAME, DB_USERID, "
                "DEFAULT_TBSID, DEFAULT_TEMP_TBSID, SYSDBA_FLAG, "
                "AUTOCOMMIT_FLAG, SESSION_STATE, ISOLATION_LEVEL, "
                "REPLICATION_MODE, TRANSACTION_MODE, COMMIT_WRITE_WAIT_MODE, "
                "OPTIMIZER_MODE, HEADER_DISPLAY_MODE, "
                "CURRENT_STMT_ID, STACK_SIZE, DEFAULT_DATE_FORMAT, TRX_UPDATE_MAX_LOGSIZE, "
                "PARALLE_DML_MODE, LOGIN_TIME, FAILOVER_SOURCE, "
                "NLS_TERRITORY, NLS_ISO_CURRENCY, NLS_CURRENCY, NLS_NUMERIC_CHARACTERS, TIME_ZONE, "
                "LOB_CACHE_THRESHOLD, QUERY_REWRITE_ENABLE ) "
            "AS SELECT "
                "A.ID, "
                "A.TRANS_ID , "
                "A.QUERY_TIME_LIMIT, A.DDL_TIME_LIMIT, A.FETCH_TIME_LIMIT, "
                "A.UTRANS_TIME_LIMIT, A.IDLE_TIME_LIMIT, A.IDLE_START_TIME, "
                "A.ACTIVE_FLAG, A.OPENED_STMT_COUNT, "
                "A.DB_USERNAME, A.DB_USERID, "
                "A.DEFAULT_TBSID, A.DEFAULT_TEMP_TBSID, A.SYSDBA_FLAG, "
                "A.AUTOCOMMIT_FLAG, "
                "DECODE(A.SESSION_STATE, 0, 'INIT', "
                "                        1, 'AUTH', "
                "                        2, 'SERVICE READY', "
                "                        3, 'SERVICE', "
                "                        4, 'END', "
                "                        5, 'ROLLBACK', "
                "                        'UNKNOWN') SESSION_STATE, "
                "A.ISOLATION_LEVEL, A.REPLICATION_MODE, A.TRANSACTION_MODE, "
                "A.COMMIT_WRITE_WAIT_MODE, A.OPTIMIZER_MODE, "
                "A.HEADER_DISPLAY_MODE, A.CURRENT_STMT_ID, A.STACK_SIZE, "
                "A.DEFAULT_DATE_FORMAT, A.TRX_UPDATE_MAX_LOGSIZE, "
                "A.PARALLEL_DML_MODE, A.LOGIN_TIME, A.FAILOVER_SOURCE, "
                "A.NLS_TERRITORY, A.NLS_ISO_CURRENCY, A.NLS_CURRENCY, A.NLS_NUMERIC_CHARACTERS, "
                "A.TIME_ZONE, A.LOB_CACHE_THRESHOLD, "
                "DECODE(A.QUERY_REWRITE_ENABLE, 0, 'FALSE', "
                "                               1, 'TRUE', "
                "                               'UNKNOWN') QUERY_REWRITE_ENABLE "
            "FROM X$INTERNAL_SESSION A",
    /* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
    /* BUG-48515 LIMIT(최대 접속 혀용개수), CONNECTED(현재 접속 개수) 추가*/
    (SChar*)"CREATE VIEW V$ACCESS_LIST "
                "( ID, OPERATION, ADDRESS, MASK, LIMIT, CONNECTED )"
            "AS SELECT "
                "A.ID ,"
                "DECODE(A.OPERATION, 0, 'DENY', 1, 'PERMIT', NULL) ,"
                "A.ADDRESS ,"
                "A.MASK ,"
                "A.LIMIT ,"
                "A.CONNECTED "
            "FROM X$ACCESS_LIST A",
    /* PROJ-2626 Snapshot Export */
    (SChar *)"CREATE VIEW V$SNAPSHOT "
                "( SCN, BEGIN_TIME, BEGIN_MEM_USAGE, BEGIN_DISK_UNDO_USAGE, "
                "CURRENT_TIME, CURRENT_MEM_USAGE, CURRENT_DISK_UNDO_USAGE ) "
             "AS SELECT "
                "A.SCN, A.BEGIN_TIME, A.BEGIN_MEM_USAGE, A.BEGIN_DISK_UNDO_USAGE, "
                "A.CURRENT_TIME, A.CURRENT_MEM_USAGE, A.CURRENT_DISK_UNDO_USAGE "
                "FROM X$SNAPSHOT A",
    // BUG-44528 V$RESERVED_WORDS
    (SChar *)"CREATE VIEW V$RESERVED_WORDS "
                "( KEYWORD, LENGTH, RESERVED_TYPE ) "
             "AS SELECT "
                "A.KEYWORD, A.LENGTH, A.RESERVED_TYPE "
                "FROM X$RESERVED_WORDS A",    
    /* PROJ-2717 Internal Procedures */
    (SChar*)"CREATE VIEW V$LIBRARY "
                "( FILE_SPEC, REFERENCE_COUNT, FILE_SIZE, CREATE_TIME, OPEN_TIME ) "
            "AS SELECT "
                "A.FILE_SPEC, A.REFERENCE_COUNT, A.FILE_SIZE, A.CREATE_TIME, A.OPEN_TIME "
            "FROM X$LIBRARY A "
            "WHERE A.REFERENCE_COUNT >= 0",
    (SChar*)"CREATE VIEW V$PROCINFO"
                "( PROC_OID, MODIFY_COUNT, STATUS, SESSION_ID, PROC_TYPE, SHARD_SPLIT_METHOD ) "
            "AS SELECT "
                "A.PROC_OID, A.MODIFY_COUNT, "
                "DECODE(A.STATUS, 0, 'VALID', "
                "                 1, 'INVALID' ) STATUS, "
                "A.SESSION_ID, "
                "DECODE(A.PROC_TYPE, 0, 'NORMAL', "
                "                    1, 'EXTERNAL C', "
                "                    2, 'INTERNAL C', "
                "                    'UNKNOWN') PROC_TYPE, "
                "DECODE(A.SHARD_SPLIT_METHOD, 0, 'NONE', "
                "                             1, 'HASH', "
                "                             2, 'RANGE', "
                "                             3, 'LIST', "
                "                             4, 'CLONE', "
                "                             5, 'SOLO', "
                "                             'UNKNOWN') SHARD_SPLIT_METHOD "
            "FROM X$PROCINFO A",
    NULL
};


/************************************/
/* BUG-45646 shard performance view */
/************************************/

SChar * gQcmShardPerformanceViews[] =
{
    (SChar*)"CREATE VIEW S$CONNECTION_INFO "
            "AS ( "
            "SELECT NODE_ID, NODE_NAME, COMM_NAME, TOUCH_COUNT, LINK_FAILURE "
            "FROM X$SHARD_CONNECTION_INFO ) "
            ,
    (SChar*)"CREATE VIEW S$PROPERTY "
            "AS ( "
            "SELECT "
               "NODE_NAME, NAME, STOREDCOUNT, ATTR, MIN, MAX, "
               "VALUE1, VALUE2, VALUE3, VALUE4, VALUE5, VALUE6, VALUE7, VALUE8 "
            "FROM NODE[DATA] ( SELECT SHARD_NODE_NAME() NODE_NAME, * FROM V$PROPERTY ) ) "
            ,
    (SChar*)"CREATE VIEW S$SESSION "
            "AS ( "
            "SELECT "
               "ID, SHARD_META_NUMBER, NODE_NAME, "
               "SHARD_CLIENT, "
               "SHARD_SESSION_TYPE, "
               "SESSION_ID, TRANS_ID, "
               "TASK_STATE, "
               "COMM_NAME, XA_SESSION_FLAG, XA_ASSOCIATE_FLAG, QUERY_TIME_LIMIT, "
               "DDL_TIME_LIMIT, FETCH_TIME_LIMIT, UTRANS_TIME_LIMIT, IDLE_TIME_LIMIT, IDLE_START_TIME, "
               "ACTIVE_FLAG, OPENED_STMT_COUNT, CLIENT_PACKAGE_VERSION, CLIENT_PROTOCOL_VERSION, "
               "CLIENT_PID, CLIENT_TYPE, CLIENT_APP_INFO, CLIENT_NLS, DB_USERNAME, DB_USERID, "
               "DEFAULT_TBSID, DEFAULT_TEMP_TBSID, SYSDBA_FLAG, AUTOCOMMIT_FLAG, "
               "SESSION_STATE, "
               "ISOLATION_LEVEL, REPLICATION_MODE, TRANSACTION_MODE, COMMIT_WRITE_WAIT_MODE, "
               "OPTIMIZER_MODE, HEADER_DISPLAY_MODE, CURRENT_STMT_ID, STACK_SIZE, DEFAULT_DATE_FORMAT, "
               "TRX_UPDATE_MAX_LOGSIZE, PARALLEL_DML_MODE, LOGIN_TIME, FAILOVER_SOURCE, NLS_TERRITORY, "
               "NLS_ISO_CURRENCY, NLS_CURRENCY, NLS_NUMERIC_CHARACTERS, TIME_ZONE, LOB_CACHE_THRESHOLD, "
               "QUERY_REWRITE_ENABLE, "
               "DBLINK_GLOBAL_TRANSACTION_LEVEL, DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "
               "MAX_STATEMENTS_PER_SESSION, SSL_CIPHER, SSL_CERTIFICATE_SUBJECT, SSL_CERTIFICATE_ISSUER, "
               "MODULE, ACTION, REPLICATION_DDL_SYNC, REPLICATION_DDL_SYNC_TIMELIMIT, "
               "MESSAGE_CALLBACK," 
               "GLOBAL_TRANSACTION_LEVEL, ARITHMETIC_OPERATION_MODE, NORMALFORM_MAXIMUM, "
               "ST_OBJECT_BUFFER_SIZE, NLS_NCHAR_CONV_EXCP, AUTO_REMOTE_EXEC, "
               "TRCLOG_DETAIL_PREDICATE, OPTIMIZER_DISK_INDEX_COST_ADJ, OPTIMIZER_MEMORY_INDEX_COST_ADJ, "
               "RECYCLEBIN_ENABLE, RESULT_CACHE_ENABLE, TOP_RESULT_CACHE_MODE, "
               "OPTIMIZER_AUTO_STATS, OPTIMIZER_PERFORMANCE_VIEW, "
               "TRCLOG_DETAIL_SHARD, SERIAL_EXECUTE_MODE, TRCLOG_DETAIL_INFORMATION, TRANSACTIONAL_DDL, "
               "GCTX_COORD_SCN, GCTX_PREPARE_SCN, GCTX_GLOBAL_COMMIT_SCN, GCTX_LAST_SYSTEM_SCN, "
               "SHARD_STATEMENT_RETRY, INDOUBT_FETCH_TIMEOUT, INDOUBT_FETCH_METHOD, "
               "LAST_SHARD_META_NUMBER, RECEIVED_SHARD_META_NUMBER, "
               "SHARD_STMT_EXEC_SEQ "
            "FROM NODE[DATA] ( SELECT SHARD_NODE_NAME() NODE_NAME, "
                                   "SHARD_PIN ID, SHARD_META_NUMBER, "
                                   "DECODE(SHARD_CLIENT, 1, 'Y', 'N') SHARD_CLIENT, "
                                   "DECODE(SHARD_SESSION_TYPE, 0, 'U', 1, 'C', 2, 'L','-') SHARD_SESSION_TYPE, "
                                   "ID SESSION_ID, TRANS_ID, "
                                   "DECODE(TASK_STATE, 0, 'WAITING', "
                                                      "1, 'READY', "
                                                      "2, 'EXECUTING', "
                                                      "3, 'QUEUE WAIT', "
                                                      "4, 'QUEUE READY', "
                                                      "'UNKNOWN') TASK_STATE, "
                                   "COMM_NAME, XA_SESSION_FLAG, XA_ASSOCIATE_FLAG, QUERY_TIME_LIMIT, "
                                   "DDL_TIME_LIMIT, FETCH_TIME_LIMIT, UTRANS_TIME_LIMIT, IDLE_TIME_LIMIT, IDLE_START_TIME, "
                                   "ACTIVE_FLAG, OPENED_STMT_COUNT, CLIENT_PACKAGE_VERSION, CLIENT_PROTOCOL_VERSION, "
                                   "CLIENT_PID, CLIENT_TYPE, CLIENT_APP_INFO, CLIENT_NLS, DB_USERNAME, DB_USERID, "
                                   "DEFAULT_TBSID, DEFAULT_TEMP_TBSID, SYSDBA_FLAG, AUTOCOMMIT_FLAG, "
                                   "DECODE(SESSION_STATE, 0, 'INIT', "
                                                         "1, 'AUTH', "
                                                         "2, 'SERVICE READY', "
                                                         "3, 'SERVICE', "
                                                         "4, 'END', "
                                                         "5, 'ROLLBACK', "
                                                         "'UNKNOWN') SESSION_STATE, "
                                   "ISOLATION_LEVEL, REPLICATION_MODE, TRANSACTION_MODE, COMMIT_WRITE_WAIT_MODE, "
                                   "OPTIMIZER_MODE, HEADER_DISPLAY_MODE, CURRENT_STMT_ID, STACK_SIZE, DEFAULT_DATE_FORMAT, "
                                   "TRX_UPDATE_MAX_LOGSIZE, PARALLEL_DML_MODE, LOGIN_TIME, FAILOVER_SOURCE, NLS_TERRITORY, "
                                   "NLS_ISO_CURRENCY, NLS_CURRENCY, NLS_NUMERIC_CHARACTERS, TIME_ZONE, LOB_CACHE_THRESHOLD, "
                                   "DECODE(QUERY_REWRITE_ENABLE, 0, 'FALSE', "
                                                                "1, 'TRUE', "
                                                                "'UNKNOWN') QUERY_REWRITE_ENABLE, "
                                   "DBLINK_GLOBAL_TRANSACTION_LEVEL, DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "
                                   "MAX_STATEMENTS_PER_SESSION, SSL_CIPHER, SSL_CERTIFICATE_SUBJECT, SSL_CERTIFICATE_ISSUER, "
                                   "MODULE, ACTION, REPLICATION_DDL_SYNC, REPLICATION_DDL_SYNC_TIMELIMIT, "
                                   "DECODE(MESSAGE_CALLBACK, 0, 'UNREG', "
                                                            "1, 'REG', "
                                                            "'UNKNOWN') MESSAGE_CALLBACK," 


                                   "GLOBAL_TRANSACTION_LEVEL, ARITHMETIC_OPERATION_MODE, NORMALFORM_MAXIMUM, "
                                   "ST_OBJECT_BUFFER_SIZE, NLS_NCHAR_CONV_EXCP, AUTO_REMOTE_EXEC, "
                                   "TRCLOG_DETAIL_PREDICATE, OPTIMIZER_DISK_INDEX_COST_ADJ, OPTIMIZER_MEMORY_INDEX_COST_ADJ, "
                                   "RECYCLEBIN_ENABLE, RESULT_CACHE_ENABLE, TOP_RESULT_CACHE_MODE, "
                                   "OPTIMIZER_AUTO_STATS, OPTIMIZER_PERFORMANCE_VIEW, "
                                   "TRCLOG_DETAIL_SHARD, SERIAL_EXECUTE_MODE, TRCLOG_DETAIL_INFORMATION, TRANSACTIONAL_DDL, "
                                   "GCTX_COORD_SCN, GCTX_PREPARE_SCN, GCTX_GLOBAL_COMMIT_SCN, GCTX_LAST_SYSTEM_SCN, "
                                   "SHARD_STATEMENT_RETRY, INDOUBT_FETCH_TIMEOUT, INDOUBT_FETCH_METHOD, "
                                   "LAST_SHARD_META_NUMBER, RECEIVED_SHARD_META_NUMBER, "
                                   "SHARD_STMT_EXEC_SEQ "
                              "FROM X$SESSION WHERE SHARD_PIN != '0-0-0' ) ) "
            ,
    (SChar*)"CREATE VIEW S$STATEMENT "
            "AS ( "
            "SELECT "
               "SHARD_SESSION_ID, NODE_NAME, SHARD_SESSION_TYPE, QUERY_TYPE, "
               "ID, PARENT_ID, CURSOR_TYPE, "
               "SESSION_ID, TX_ID, QUERY, LAST_QUERY_START_TIME, QUERY_START_TIME, "
               "FETCH_START_TIME, EXECUTE_STATE, FETCH_STATE, ARRAY_FLAG, ROW_NUMBER, EXECUTE_FLAG, "
               "BEGIN_FLAG, TOTAL_TIME, PARSE_TIME, VALIDATE_TIME, OPTIMIZE_TIME, EXECUTE_TIME, "
               "FETCH_TIME, SOFT_PREPARE_TIME, SQL_CACHE_TEXT_ID, SQL_CACHE_PCO_ID, OPTIMIZER, COST, "
               "USED_MEMORY, READ_PAGE, WRITE_PAGE, GET_PAGE, CREATE_PAGE, UNDO_READ_PAGE, "
               "UNDO_WRITE_PAGE, UNDO_GET_PAGE, UNDO_CREATE_PAGE, MEM_CURSOR_FULL_SCAN, "
               "MEM_CURSOR_INDEX_SCAN, DISK_CURSOR_FULL_SCAN, DISK_CURSOR_INDEX_SCAN, EXECUTE_SUCCESS, "
               "EXECUTE_FAILURE, FETCH_SUCCESS, FETCH_FAILURE, PROCESS_ROW, MEMORY_TABLE_ACCESS_COUNT, "
               "SEQNUM, EVENT, P1, P2, P3, WAIT_TIME, SECOND_IN_TIME, SIMPLE_QUERY, MATHEMATICS_TEMP_MEMORY, "
               "GCTX_REQUEST_SCN, DISTRIBUTION_FIRST_STMT_SCN, DISTRIBUTION_FIRST_STMT_TIME, DISTRIBUTION_LEVEL, "
               "SHARD_PARTIAL_EXEC_TYPE "
            "FROM NODE[DATA]( "
               "SELECT /*+ USE_HASH( A, B ) */ "
                   "A.SHARD_PIN SHARD_SESSION_ID, SHARD_NODE_NAME() NODE_NAME, "
                   "DECODE(A.SHARD_SESSION_TYPE, 0, 'U', 1, 'C', 2, 'L','-') SHARD_SESSION_TYPE, "
                   "DECODE(A.SHARD_SESSION_TYPE, 0, DECODE(SHARD_QUERY_TYPE, 1, 'S', 2, 'N', '-'), 2, 'S', '-') QUERY_TYPE, "
                   "A.ID ID, A.PARENT_ID PARENT_ID, A.CURSOR_TYPE CURSOR_TYPE, "
                   "A.SESSION_ID SESSION_ID, A.TX_ID TX_ID, A.QUERY QUERY, "
                   "A.LAST_QUERY_START_TIME LAST_QUERY_START_TIME, A.QUERY_START_TIME QUERY_START_TIME, "
                   "A.FETCH_START_TIME FETCH_START_TIME, "
                   "DECODE(A.STATE, 0, 'ALLOC', "
                                   "1, 'PREPARED', "
                                   "2, 'EXECUTED', "
                                   "'UNKNOWN') EXECUTE_STATE, "
                   "DECODE(A.FETCH_STATE, 0, 'PROCEED', "
                                         "1, 'CLOSE', "
                                         "2, 'NO RESULTSET', "
                                         "3, 'INVALIDATED', "
                                         "'UNKNOWN') FETCH_STATE, "
                   "A.ARRAY_FLAG ARRAY_FLAG, A.ROW_NUMBER ROW_NUMBER, A.EXECUTE_FLAG EXECUTE_FLAG, "
                   "A.BEGIN_FLAG BEGIN_FLAG, A.TOTAL_TIME TOTAL_TIME, A.PARSE_TIME PARSE_TIME, "
                   "A.VALIDATE_TIME VALIDATE_TIME, A.OPTIMIZE_TIME OPTIMIZE_TIME, "
                   "A.EXECUTE_TIME EXECUTE_TIME, A.FETCH_TIME FETCH_TIME, A.SOFT_PREPARE_TIME SOFT_PREPARE_TIME, "
                   "A.SQL_CACHE_TEXT_ID SQL_CACHE_TEXT_ID, A.SQL_CACHE_PCO_ID SQL_CACHE_PCO_ID, "
                   "A.OPTIMIZER OPTIMIZER, A.COST COST, A.USED_MEMORY USED_MEMORY, A.READ_PAGE READ_PAGE, "
                   "A.WRITE_PAGE WRITE_PAGE, A.GET_PAGE GET_PAGE, A.CREATE_PAGE CREATE_PAGE, "
                   "A.UNDO_READ_PAGE UNDO_READ_PAGE, A.UNDO_WRITE_PAGE UNDO_WRITE_PAGE, "
                   "A.UNDO_GET_PAGE UNDO_GET_PAGE, A.UNDO_CREATE_PAGE UNDO_CREATE_PAGE, "
                   "A.MEM_CURSOR_FULL_SCAN MEM_CURSOR_FULL_SCAN, A.MEM_CURSOR_INDEX_SCAN MEM_CURSOR_INDEX_SCAN, "
                   "A.DISK_CURSOR_FULL_SCAN DISK_CURSOR_FULL_SCAN, A.DISK_CURSOR_INDEX_SCAN DISK_CURSOR_INDEX_SCAN, "
                   "A.EXECUTE_SUCCESS EXECUTE_SUCCESS, A.EXECUTE_FAILURE EXECUTE_FAILURE, "
                   "A.FETCH_SUCCESS FETCH_SUCCESS, A.FETCH_FAILURE FETCH_FAILURE, A.PROCESS_ROW PROCESS_ROW, "
                   "A.MEMORY_TABLE_ACCESS_COUNT MEMORY_TABLE_ACCESS_COUNT, A.SEQNUM SEQNUM, B.EVENT EVENT, "
                   "A.P1 P1, A.P2 P2, A.P3 P3, A.WAIT_TIME WAIT_TIME, A.SECOND_IN_TIME SECOND_IN_TIME, "
                   "A.SIMPLE_QUERY SIMPLE_QUERY, A.MATHEMATICS_TEMP_MEMORY, "
                   "A.GCTX_REQUEST_SCN, A.DISTRIBUTION_FIRST_STMT_SCN, A.DISTRIBUTION_FIRST_STMT_TIME, A.DISTRIBUTION_LEVEL, "
                   "DECODE( A.SHARD_PARTIAL_EXEC_TYPE, 0, 'NORMAL', "
                                                      "1, 'PARTIAL COORD', "
                                                      "2, 'PARTIAL QUERY' ) SHARD_PARTIAL_EXEC_TYPE "
               "FROM X$STATEMENT A, X$WAIT_EVENT_NAME B "
               "WHERE A.SEQNUM = B.EVENT_ID AND A.SHARD_PIN != '0-0-0' ) D ) "
            ,
    (SChar*)"CREATE VIEW S$TIME_SCN "
            "AS ( "
            "SELECT "
                "NODE_NAME, TIME, SYSTEM_SCN, BASE "
            "FROM NODE[DATA] ( SELECT SHARD_NODE_NAME() NODE_NAME, TIME, SYSTEM_SCN, BASE FROM X$TIME_SCN ) ) "
            ,
    (SChar*)"CREATE VIEW S$PENDING_WAIT "
            "AS (  "
            "SELECT   "
                "NODE_NAME, TRANS_ID, SHARD_PIN, WAIT_FOR_TRANS_ID, WAIT_FOR_XID, WAIT_FOR_SHARD_PIN  "
            "FROM NODE[DATA] ( "
                 "SELECT SHARD_NODE_NAME() NODE_NAME, "
                    "TRANS_ID, SHARD_PIN, WAIT_FOR_TRANS_ID, WAIT_FOR_XID, WAIT_FOR_SHARD_PIN " 
                 "FROM X$PENDING_WAIT ) );  "
            , 
    (SChar*)"CREATE VIEW S$LOCK_WAIT "
            "AS ( "
            "SELECT "
                "NODE_NAME, "
                "TRANS_ID, SHARD_PIN, "
                "DISTRIBUTION_FIRST_STMT_SCN, DISTRIBUTION_FIRST_STMT_TIME, DISTRIBUTION_LEVEL, " 
                "WAIT_FOR_TRANS_ID, WAIT_FOR_SHARD_PIN, "
                "WAIT_FOR_DISTRIBUTION_FIRST_STMT_SCN, WAIT_FOR_DISTRIBUTION_FIRST_STMT_TIME, WAIT_FOR_DISTRIBUTION_LEVEL "
            "FROM NODE[DATA] ( "
                "SELECT SHARD_NODE_NAME() NODE_NAME, "
                "L.TRANS_ID, T.SHARD_PIN, "
                "CASE WHEN ( T.DISTRIBUTION_FIRST_STMT_VIEW_SCN LIKE 'INFINITE%' ) "
                "    THEN T.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "    ELSE "
                "    CASE WHEN TO_NUMBER(T.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 0 OR TO_NUMBER(T.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 1 "
                "        THEN T.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "        ELSE LPAD(TO_CHAR( TO_NUMBER(T.DISTRIBUTION_FIRST_STMT_VIEW_SCN) - 7 ), 19, ' ') "
                "    END " 
                "END AS DISTRIBUTION_FIRST_STMT_SCN, "
                "T.DISTRIBUTION_FIRST_STMT_TIME, T.DISTRIBUTION_LEVEL, "
                "L.WAIT_FOR_TRANS_ID, "
                "T2.SHARD_PIN AS WAIT_FOR_SHARD_PIN, "
                "CASE WHEN ( T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN LIKE 'INFINITE%' ) "
                "    THEN T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "    ELSE "
                "    CASE WHEN TO_NUMBER(T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 0 OR TO_NUMBER(T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 1 "
                "        THEN T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "        ELSE LPAD(TO_CHAR( TO_NUMBER(T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN) - 7 ), 19, ' ') "
                "    END " 
                "END AS WAIT_FOR_DISTRIBUTION_FIRST_STMT_SCN, "
                "T2.DISTRIBUTION_FIRST_STMT_TIME AS WAIT_FOR_DISTRIBUTION_FIRST_STMT_TIME, T2.DISTRIBUTION_LEVEL AS WAIT_FOR_DISTRIBUTION_LEVEL "
                    "FROM X$LOCK_WAIT L, X$TRANSACTIONS T, (SELECT * FROM X$TRANSACTIONS) T2 "
                    "WHERE L.TRANS_ID = T.ID AND L.WAIT_FOR_TRANS_ID = T2.ID ) ) "
            ,
    (SChar*)"CREATE VIEW S$DIST_LOCK_WAIT "
            "AS ( "
            "SELECT "
                "NODE_NAME, TRANS_ID, SHARD_PIN, "
                "DISTRIBUTION_FIRST_STMT_SCN, DISTRIBUTION_FIRST_STMT_TIME, DISTRIBUTION_LEVEL, "
                "WAIT_FOR_TRANS_ID, WAIT_FOR_SHARD_PIN, "
                "WAIT_FOR_DISTRIBUTION_FIRST_STMT_SCN, WAIT_FOR_DISTRIBUTION_FIRST_STMT_TIME, WAIT_FOR_DISTRIBUTION_LEVEL, "
                "DISTRIBUTION_DEADLOCK_DETECTION, DISTRIBUTION_DEADLOCK_WAIT_TIME, DISTRIBUTION_DEADLOCK_ELAPSED_TIME "
            "FROM NODE[DATA] ( "
                "SELECT SHARD_NODE_NAME() NODE_NAME, "
                "D.TRANS_ID, T.SHARD_PIN, "
                "CASE WHEN ( T.DISTRIBUTION_FIRST_STMT_VIEW_SCN LIKE 'INFINITE%' ) "
                "    THEN T.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "    ELSE "
                "    CASE WHEN TO_NUMBER(T.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 0 OR TO_NUMBER(T.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 1 "
                "        THEN T.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "        ELSE LPAD(TO_CHAR( TO_NUMBER(T.DISTRIBUTION_FIRST_STMT_VIEW_SCN) - 7 ), 19, ' ') "
                "    END " 
                "END AS DISTRIBUTION_FIRST_STMT_SCN, "
                "T.DISTRIBUTION_FIRST_STMT_TIME, T.DISTRIBUTION_LEVEL, "
                "D.WAIT_FOR_TRANS_ID, "
                "T2.SHARD_PIN AS WAIT_FOR_SHARD_PIN, "
                "CASE WHEN ( T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN LIKE 'INFINITE%' ) "
                "    THEN T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "    ELSE "
                "    CASE WHEN TO_NUMBER(T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 0 OR TO_NUMBER(T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 1 "
                "        THEN T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "        ELSE LPAD(TO_CHAR( TO_NUMBER(T2.DISTRIBUTION_FIRST_STMT_VIEW_SCN) - 7 ), 19, ' ') "
                "    END " 
                "END AS WAIT_FOR_DISTRIBUTION_FIRST_STMT_SCN, "
                "T2.DISTRIBUTION_FIRST_STMT_TIME AS WAIT_FOR_DISTRIBUTION_FIRST_STMT_TIME, T2.DISTRIBUTION_LEVEL AS WAIT_FOR_DISTRIBUTION_LEVEL, "
                "T.DISTRIBUTION_DEADLOCK_DETECTION, T.DISTRIBUTION_DEADLOCK_WAIT_TIME, T.DISTRIBUTION_DEADLOCK_ELAPSED_TIME "
                "FROM X$DIST_LOCK_WAIT D, X$TRANSACTIONS T, (SELECT * FROM X$TRANSACTIONS) T2 "
                "WHERE D.TRANS_ID = T.ID AND D.WAIT_FOR_TRANS_ID = T2.ID ) )"
            ,
    (SChar*)"CREATE VIEW S$TRANSACTION ( "
                "NODE_NAME, ID, SESSION_ID, MEMORY_VIEW_SCN, MIN_MEMORY_LOB_VIEW_SCN, "
                "DISK_VIEW_SCN, MIN_DISK_LOB_VIEW_SCN, "
                "LAST_REQUEST_VIEW_SCN, PREPARE_SCN, COMMIT_SCN, "
                "STATUS, UPDATE_STATUS, LOG_TYPE, "
                "XA_COMMIT_STATUS, XA_PREPARED_TIME, "
                "FIRST_UNDO_NEXT_LSN_FILENO, "
                "FIRST_UNDO_NEXT_LSN_OFFSET, CURRENT_UNDO_NEXT_SN, "
                "CURRENT_UNDO_NEXT_LSN_FILENO, "
                "CURRENT_UNDO_NEXT_LSN_OFFSET, "
                "LAST_UNDO_NEXT_LSN_FILENO, LAST_UNDO_NEXT_LSN_OFFSET, "
                "LAST_UNDO_NEXT_SN, SLOT_NO, "
                "UPDATE_SIZE, ENABLE_ROLLBACK, FIRST_UPDATE_TIME, "
                "LOG_BUF_SIZE, LOG_OFFSET, SKIP_CHECK_FLAG, "
                "SKIP_CHECK_SCN_FLAG, DDL_FLAG, TSS_RID, "
                "RESOURCE_GROUP_ID, LEGACY_TRANS_COUNT, ISOLATION_LEVEL, "
                "PROCESSED_UNDO_TIME, ESTIMATED_TOTAL_UNDO_TIME, "
                "TOTAL_LOG_COUNT, "
                "TOTAL_UNDO_LOG_COUNT, "
                "PROCESSED_UNDO_LOG_COUNT, GLOBAL_CONSISTENCY, "
                "SHARD_PIN, DISTRIBUTION_FIRST_STMT_TIME, "
                "DISTRIBUTION_FIRST_STMT_SCN, "
                "DISTRIBUTION_LEVEL, "
                "DISTRIBUTION_DEADLOCK_DETECTION, DISTRIBUTION_DEADLOCK_WAIT_TIME, DISTRIBUTION_DEADLOCK_ELAPSED_TIME ) "
            "AS ( "
            "SELECT "
                "NODE_NAME, ID, SESSION_ID, MIN_MEM_VIEW_SCN, MIN_MEM_VIEW_SCN_FOR_LOB, "
                "MIN_DISK_VIEW_SCN, MIN_DISK_VIEW_SCN_FOR_LOB, "
                "LAST_REQUEST_VIEW_SCN, PREPARE_SCN, COMMIT_SCN, "
                "STATUS, UPDATE_STATUS, LOG_TYPE, "
                "XA_COMMIT_STATUS, XA_PREPARED_TIME, "
                "FIRST_UNDO_NEXT_LSN_FILENO, "
                "FIRST_UNDO_NEXT_LSN_OFFSET, CURRENT_UNDO_NEXT_SN, "
                "CURRENT_UNDO_NEXT_LSN_FILENO, "
                "CURRENT_UNDO_NEXT_LSN_OFFSET, "
                "LAST_UNDO_NEXT_LSN_FILENO, LAST_UNDO_NEXT_LSN_OFFSET, "
                "LAST_UNDO_NEXT_SN, SLOT_NO, "
                "UPDATE_SIZE, ENABLE_ROLLBACK, FIRST_UPDATE_TIME, "
                "LOG_BUF_SIZE, LOG_OFFSET, SKIP_CHECK_FLAG, "
                "SKIP_CHECK_SCN_FLAG, DDL_FLAG, TSS_RID, "
                "RESOURCE_GROUP_ID, LEGACY_TRANS_COUNT, ISOLATION_LEVEL, "
                "PROCESSED_UNDO_TIME, ESTIMATED_TOTAL_UNDO_TIME, "
                "TOTAL_LOG_COUNT, "
                "CAST( TOTAL_LOG_COUNT - PROCESSED_UNDO_LOG_COUNT AS BIGINT ), "
                "PROCESSED_UNDO_LOG_COUNT, GLOBAL_CONSISTENCY, "
                "SHARD_PIN, DISTRIBUTION_FIRST_STMT_TIME, " 
                "CASE WHEN ( DISTRIBUTION_FIRST_STMT_VIEW_SCN LIKE 'INFINITE%' ) "
                "    THEN DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "    ELSE "
                "    CASE WHEN TO_NUMBER(DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 0 OR TO_NUMBER(DISTRIBUTION_FIRST_STMT_VIEW_SCN) = 1 "
                "        THEN DISTRIBUTION_FIRST_STMT_VIEW_SCN "
                "        ELSE LPAD(TO_CHAR( TO_NUMBER(DISTRIBUTION_FIRST_STMT_VIEW_SCN) - 7 ), 19, ' ') "
                "    END " 
                "END, "
                "DISTRIBUTION_LEVEL, "
                "DISTRIBUTION_DEADLOCK_DETECTION, DISTRIBUTION_DEADLOCK_WAIT_TIME, DISTRIBUTION_DEADLOCK_ELAPSED_TIME "
            "FROM NODE[DATA] ( SELECT SHARD_NODE_NAME() NODE_NAME, * FROM X$TRANSACTIONS WHERE SHARD_PIN != '0-0-0' ) )"
            ,
    NULL
};

extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdBigint;
extern mtdModule mtdSmallint;
extern mtdModule mtdInteger;
extern mtdModule mtdDouble;

IDE_RC qcmPerformanceView::makeParseTreeForViewInSelect(
    qcStatement     * aStatement,
    qmsTableRef     * aTableRef )
{
/***********************************************************************
 *
 * Description :
 *  select * from v$table 과 같은 질의에 대해서
 *  v$table의 parseTree를 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement         * sStatement;
    SChar               * sStmtText;
    UInt                  sStmtTextLen;
    qdTableParseTree    * sCreateViewParseTree;

    // alloc qcStatement for view
    IDU_FIT_POINT( "qcmPerformanceView::makeParseTreeForViewInSelect::alloc::sStatement",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          qcStatement,
                          &sStatement)
             != IDE_SUCCESS);

    // set meber of qcStatement
    idlOS::memcpy( sStatement, aStatement, ID_SIZEOF(qcStatement) );

    // myPlan을 재설정한다.
    sStatement->myPlan = & sStatement->privatePlan;
    sStatement->myPlan->planEnv = NULL;

    // PROJ-1726 - performance view 스트링을 얻기 위해서
    // gQcmPerformanceViews 를 직접접근하는 대신
    // qcmPerformanceViewManager::get 을 이용, 간접접근을 한다.
    // 이는 gQcmPerformanceViews 외에 동적모듈이 로드될 때
    // 추가로 등록되는 performance view 를 위한 것이다.
    sStmtText = qcmPerformanceViewManager::get( aTableRef->tableInfo->viewArrayNo,
                                                aTableRef->tableInfo->mPVType );

    IDE_TEST_RAISE( sStmtText == NULL, ERR_PERFORMANCE_VIEW );
    sStmtTextLen = idlOS::strlen( sStmtText );

    IDU_FIT_POINT( "qcmPerformanceView::makeParseTreeForViewInSelect::alloc::stmtText",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
            sStmtTextLen+1,
            (void **)(&(sStatement->myPlan->stmtText)) )
        != IDE_SUCCESS);

    // PROJ-1726 - performance view 스트링을 얻기 위해서
    // gQcmPerformanceViews 를 직접접근하는 대신
    // qcmPerformanceViewManager::get 을 이용, 간접접근을 한다.
    idlOS::memcpy(sStatement->myPlan->stmtText,
                  sStmtText,
                  sStmtTextLen);
    sStatement->myPlan->stmtText[sStmtTextLen] = '\0';
    sStatement->myPlan->stmtTextLen = sStmtTextLen;

    sStatement->myPlan->parseTree   = NULL;
    sStatement->myPlan->plan        = NULL;

    // parsing view
    IDE_TEST(qcpManager::parseIt( sStatement ) != IDE_SUCCESS);

    sCreateViewParseTree = (qdTableParseTree *)(sStatement->myPlan->parseTree);

    // set parse tree
    aTableRef->view = sCreateViewParseTree->select;

    // planEnv를 재설정한다.
    aTableRef->view->myPlan->planEnv = aStatement->myPlan->planEnv;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PERFORMANCE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmPerformanceView::makeParseTreeForViewInSelect",
                                  "invalid tableRef" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPerformanceView::registerPerformanceView( idvSQL    * aStatistics,
                                                    qcmPVType   aType )
{
/***********************************************************************
 *
 * Description :
 *  fixedTable을 기반으로 gQcmPerformanceViews[]에 정의된
 *  performanceView를 정의한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_TEST( qcmPerformanceView::runDDLforPV( aStatistics, aType ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmPerformanceView::executeDDL(
    qcStatement * aStatement,
    SChar       * aText )
{
/***********************************************************************
 *
 * Description :
 *  create view V$...  statement의 P/V/O/E 수행
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar *         sText = NULL;

    aStatement->myPlan->stmtTextLen = idlOS::strlen(aText);

    IDU_FIT_POINT( "qcmPerformanceView::executeDDL::malloc::sText",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               idlOS::align4(aStatement->myPlan->stmtTextLen + 1),
                               (void**)&sText)
             != IDE_SUCCESS);

    idlOS::strncpy(sText, aText, aStatement->myPlan->stmtTextLen );
    sText[aStatement->myPlan->stmtTextLen] = '\0';
    aStatement->myPlan->stmtText = sText;

    aStatement->myPlan->parseTree   = NULL;
    aStatement->myPlan->plan        = NULL;

    // parsing
    IDE_TEST(qcpManager::parseIt(aStatement) != IDE_SUCCESS);

    IDE_TEST(qcmPerformanceView::parseCreate(aStatement)  != IDE_SUCCESS);
    IDE_TEST(qtc::fixAfterParsing( QC_SHARED_TMPLATE(aStatement))
             != IDE_SUCCESS);

    // validation
    IDE_TEST(aStatement->myPlan->parseTree->validate(aStatement)  != IDE_SUCCESS);
    IDE_TEST(qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                      QC_SHARED_TMPLATE(aStatement))
             != IDE_SUCCESS);

    // optimization
    IDE_TEST(aStatement->myPlan->parseTree->optimize( aStatement ) != IDE_SUCCESS );
    IDE_TEST(qtc::fixAfterOptimization( aStatement )
             != IDE_SUCCESS);

    IDE_TEST(qcg::setPrivateArea(aStatement) != IDE_SUCCESS);

    IDE_TEST(qcg::stepAfterPVO(aStatement) != IDE_SUCCESS);

    // execution
    IDE_TEST(aStatement->myPlan->parseTree->execute(aStatement) != IDE_SUCCESS);

    // set success
    QC_PRIVATE_TMPLATE(aStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(aStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;

    qcg::clearStatement(aStatement,
                        ID_FALSE ); /* aRebuild = ID_FALSE */

    IDE_TEST(iduMemMgr::free(sText)
             != IDE_SUCCESS);

    sText = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sText != NULL)
    {
        (void)iduMemMgr::free(sText);
    }

    return IDE_FAILURE;
}

IDE_RC qcmPerformanceView::registerOnIduFixedTableDesc(
    qdTableParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *  새로 생성된 create view V$... 의 내용을 fixedTable에 등록
 *  이 작업은 create view vXXX 를 SYS_TABLES_ 에 등록하는 과정과 유사
 *
 * Implementation :
 *
 ***********************************************************************/

    iduFixedTableDesc    * sFixedTableDesc;
    iduFixedTableColDesc * sFixedTableColDesc;
    SInt                   sColumnCount;
    SInt                   i = 0;
    SInt                   sNameLen;
    qcmColumn            * sQcmColumn;
    const mtdModule      * sModule;
    SInt                   sState = 0;
    SInt                   sAllocNameCount = 0; // for cleanup code

    IDU_FIT_POINT( "qcmPerformanceView::registerOnIduFixedTableDesc::malloc::sFixedTableDesc",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               ID_SIZEOF( iduFixedTableDesc ),
                               (void**)&sFixedTableDesc )
             != IDE_SUCCESS);
    sState = 1;

    for( sQcmColumn = aParseTree->columns, sColumnCount =0;
         sQcmColumn != NULL;
         sQcmColumn = sQcmColumn->next, sColumnCount++ ) ;

    // alloc sColumnCount + 1, 1 means null columnDescription for detecting terminal.
    IDU_FIT_POINT( "qcmPerformanceView::registerOnIduFixedTableDesc::malloc::sFixedTableColDesc",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               ID_SIZEOF( iduFixedTableColDesc ) * (sColumnCount+1),
                               (void**)&sFixedTableColDesc )
             != IDE_SUCCESS);
    sState = 2;


    for( sQcmColumn = aParseTree->columns, i = 0;
         i < sColumnCount;
         sQcmColumn = sQcmColumn->next, i++ )
    {
        if( sQcmColumn->namePos.size == 0 )
        {
            sNameLen = idlOS::strlen( sQcmColumn->name );
        }
        else
        {
            sNameLen = sQcmColumn->namePos.size;
        }

        IDU_FIT_POINT( "qcmPerformanceView::registerOnIduFixedTableDesc::malloc::mName",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                                   (sNameLen+1),
                                   (void**)&(sFixedTableColDesc[i].mName) )
                 != IDE_SUCCESS);
        sAllocNameCount++; // for cleanup code

        if( sQcmColumn->namePos.size == 0 )
        {
            idlOS::strncpy( sFixedTableColDesc[i].mName,
                            sQcmColumn->name,
                            sNameLen );
        }
        else
        {
            idlOS::strncpy( sFixedTableColDesc[i].mName,
                            sQcmColumn->namePos.stmtText + sQcmColumn->namePos.offset,
                            sNameLen );
        }
        sFixedTableColDesc[i].mName[sNameLen] = '\0';
        sFixedTableColDesc[i].mOffset       = sQcmColumn->basicInfo->column.offset;
        sFixedTableColDesc[i].mLength       = sQcmColumn->basicInfo->precision;

        sModule = sQcmColumn->basicInfo->module;

        if( sModule == &mtdChar )
        {
            sFixedTableColDesc[i].mDataType = IDU_FT_TYPE_CHAR;
        }
        else if(sModule == &mtdVarchar )
        {
            sFixedTableColDesc[i].mDataType = IDU_FT_TYPE_VARCHAR;
        }
        else if( sModule == &mtdBigint )
        {
            sFixedTableColDesc[i].mDataType = IDU_FT_TYPE_BIGINT;
        }
        else if( sModule == &mtdSmallint )
        {
            sFixedTableColDesc[i].mDataType = IDU_FT_TYPE_SMALLINT;
        }
        else if( sModule == &mtdInteger )
        {
            sFixedTableColDesc[i].mDataType = IDU_FT_TYPE_INTEGER;
        }
        else if( sModule == &mtdDouble )
        {
            sFixedTableColDesc[i].mDataType = IDU_FT_TYPE_DOUBLE;
        }
        else
        {
            IDE_ASSERT( 0 );
        }

        sFixedTableColDesc[i].mConvCallback = NULL;
        sFixedTableColDesc[i].mColOffset    = 0;
        sFixedTableColDesc[i].mColSize      = 0;
        sFixedTableColDesc[i].mTableName    = NULL;
    }
    sFixedTableColDesc[i].mName         = NULL;
    sFixedTableColDesc[i].mOffset       = 0;
    sFixedTableColDesc[i].mLength       = 0;
    sFixedTableColDesc[i].mDataType     = IDU_FT_TYPE_MASK;
    sFixedTableColDesc[i].mConvCallback = NULL;
    sFixedTableColDesc[i].mColOffset    = 0;
    sFixedTableColDesc[i].mColSize      = 0;
    sFixedTableColDesc[i].mTableName    = NULL;

    IDU_FIT_POINT( "qcmPerformanceView::registerOnIduFixedTableDesc::malloc::mTableName",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               (aParseTree->tableName.size+1),
                               (void**)&(sFixedTableDesc->mTableName) )
             != IDE_SUCCESS);
    sState = 3;

    idlOS::strncpy( sFixedTableDesc->mTableName,
                    aParseTree->tableName.stmtText + aParseTree->tableName.offset,
                    aParseTree->tableName.size  );
    sFixedTableDesc->mTableName[aParseTree->tableName.size] = '\0';
    sFixedTableDesc->mBuildFunc   = nullBuildRecord;
    sFixedTableDesc->mColDesc     = sFixedTableColDesc;
    sFixedTableDesc->mEnablePhase = IDU_STARTUP_INIT;
    sFixedTableDesc->mSlotSize    = 0;
    sFixedTableDesc->mColCount    = 0;
    sFixedTableDesc->mUseTrans    = IDU_FT_DESC_TRANS_NOT_USE;
    sFixedTableDesc->mNext        = NULL;

    iduFixedTable::registFixedTable( sFixedTableDesc );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            (void)iduMemMgr::free(sFixedTableDesc->mTableName);
            /* fall through */
        case 2:
            // first, free mName of each fixed table column's desc
            for( sQcmColumn = aParseTree->columns, i = 0;
                 i < sAllocNameCount;
                 sQcmColumn = sQcmColumn->next, i++ )
            {
                (void)iduMemMgr::free(sFixedTableColDesc[i].mName);
            }

            // then, free the desc
            (void)iduMemMgr::free(sFixedTableColDesc);
            /* fall through */
        case 1:
            (void)iduMemMgr::free(sFixedTableDesc);
            /* fall through */
        default:
            break;
    }

    return IDE_FAILURE;
}

// PROJ-1726
IDE_RC qcmPerformanceViewManager::initialize()
{
    SInt i = 0;

    mPreViews        = (SChar**)gQcmPerformanceViews;
    mNumOfPreViews   = 0;
    mNumOfAddedViews = 0;
    mShardViews      = NULL;
    mShardViewCount  = 0;

    // count total size of gQcmPerformanceViews
    for( i = 0; mPreViews[i] != NULL; i++ )
    {
        mNumOfPreViews++;
    }

    /* BUG-45646 */
    if ( sdi::isShardEnable() == ID_TRUE )
    {
        mShardViews = (SChar**)gQcmShardPerformanceViews;

        for( i = 0; mShardViews[i] != NULL; i++ )
        {
            mShardViewCount++;
        }
    }

    IDU_LIST_INIT( &mAddedViewList );

    return IDE_SUCCESS;
}

IDE_RC qcmPerformanceViewManager::finalize()
{
    SChar       * sViewStr = NULL;
    iduListNode * sIterator;
    iduListNode * sDummy   = NULL;

    IDU_LIST_ITERATE_SAFE( &mAddedViewList, sIterator, sDummy )
    {
        IDU_LIST_REMOVE( sIterator );
        sViewStr = (SChar*)(sIterator->mObj);

        IDE_TEST(iduMemMgr::free( sViewStr ) != IDE_SUCCESS);
        IDE_TEST(iduMemMgr::free( sIterator ) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPerformanceViewManager::add(SChar* aViewStr)
{
    iduListNode * sNode;
    SChar       * sViewStr;
    SInt          sSize;
    SInt          sState = 0;

    IDU_FIT_POINT( "qcmPerformanceViewManager::add::malloc::sNode",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               ID_SIZEOF(iduListNode),
                               (void **)&sNode ) != IDE_SUCCESS);
    sState = 1;

    sSize = idlOS::strlen(aViewStr);

    IDU_FIT_POINT( "qcmPerformanceViewManager::add::malloc::sViewStr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               sSize + 1,
                               (void **)&sViewStr ) != IDE_SUCCESS);
    sState = 2;

    idlOS::strncpy( sViewStr, aViewStr, sSize + 1 );
    sViewStr[sSize] = '\0';

    IDU_LIST_INIT_OBJ( sNode, sViewStr );

    IDU_LIST_ADD_LAST( &mAddedViewList, sNode );

    mNumOfAddedViews++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free(sViewStr);
            /* fall through */
        case 1:
            (void)iduMemMgr::free(sNode);
            /* fall through */
        default:
            break;
    }

    return IDE_FAILURE;
}

SChar* qcmPerformanceViewManager::getFromAddedViews(int aIdx)
{
    int           sIdx = 0;
    SChar       * sRetViewStr = NULL;
    iduListNode * sIterator;

    IDU_LIST_ITERATE( &mAddedViewList, sIterator )
    {
        if( sIdx != aIdx )
        {
            sIdx++;
        }
        else
        {
            sRetViewStr = (SChar*)(sIterator->mObj);

            return sRetViewStr;
        }
    }

    return NULL;
}

SChar* qcmPerformanceViewManager::get( int aIdx, qcmPVType aType )
{
    SChar* sRetStr = NULL;

    switch( aType )
    {
        case QCM_PV_TYPE_NORMAL:
            // 만일 gQcmPerformanceViews 에 등록된 performance view 인 경우
            if(mNumOfPreViews > aIdx)
            {
                sRetStr = mPreViews[aIdx];
            }
            // 동적모듈에서 등록된 performance view 인 경우
            else
            {
                if ( aIdx < getTotalViewCount( QCM_PV_TYPE_NORMAL ) )
                {
                    sRetStr = getFromAddedViews(aIdx - mNumOfPreViews);
                }
            }
            break;

        case QCM_PV_TYPE_SHARD:
            sRetStr = mShardViews[aIdx];
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    return sRetStr;
}

IDE_RC qcmPerformanceView::runDDLforPV( idvSQL    * aStatistics,
                                        qcmPVType   aType )
{
/***********************************************************************
 *
 * Description :
 *  gQcmPerformanceViews[i] 을 fixedTable을 기반으로 DDL 수행
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt            i;
    smiTrans        sTrans;
    smiStatement    sSmiStmt;
    smiStatement  * sDummySmiStmt;
    qcStatement     sStatement;
    SChar         * sStmtText;
    UInt            sStage = 0;

    iduMemory       sIduMem;

    UInt            sSmiStmtFlag  = 0;

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_UNTOUCHABLE;


    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    sIduMem.init(IDU_MEM_QCM);
    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );

    sStage = 1;

    qcg::setSmiStmt( &sStatement, &sSmiStmt);

    sStatement.mStatistics = aStatistics;
    
    sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_ALTER_META_MASK;
    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_ALTER_META_ENABLE;

    // transaction begin
    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            aStatistics,
                            (SMI_ISOLATION_NO_PHANTOM |
                             SMI_TRANSACTION_NORMAL |
                             SMI_TRANSACTION_REPL_DEFAULT |
                             SMI_COMMIT_WRITE_NOWAIT) )
              != IDE_SUCCESS );
    sStage = 2;

    // PROJ-1726 - performance view 스트링을 얻기 위해서
    // gQcmPerformanceViews 를 직접접근하는 대신
    // qcmPerformanceViewManager::get(idx) 을 이용, 간접접근을 한다.
    for ( i = 0; i < qcmPerformanceViewManager::getTotalViewCount( aType ); i++ )
    {
        // PROJ-1726 - performance view 스트링을 얻기 위해서
        // gQcmPerformanceViews 를 직접접근하는 대신
        // qcmPerformanceViewManager::get 을 이용, 간접접근을 한다.
        sStmtText = qcmPerformanceViewManager::get( i, aType );
        IDE_TEST_RAISE( sStmtText == NULL, ERR_PERFORMANCE_VIEW );

        IDE_TEST( sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag )
                  != IDE_SUCCESS );
        sStage = 3;

        IDE_TEST( executeDDL( &sStatement, sStmtText )
                  != IDE_SUCCESS );
        sStage = 2;

        IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS)
                  != IDE_SUCCESS );
        sStage = 1;
    }

    // transaction commit
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 0;

    // free the members of qcStatement
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( sTrans.destroy( aStatistics ) != IDE_SUCCESS );

    sIduMem.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PERFORMANCE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmPerformanceView::runDDLforPV",
                                  "invalid tableRef" ));
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            (void) sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
            /* fallthrough */
        case 2:
            (void) sTrans.rollback();
            /* fallthrough */
        case 1:
            (void) qcg::freeStatement( &sStatement );
            (void) sTrans.destroy( aStatistics );
            /* fallthrough */
        case 0:
            sIduMem.destroy();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}


IDE_RC qcmPerformanceView::parseCreate(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *  parseTree를 전체를 탐색하면서 V$를 정의할 때 X$가 아닌 일반 table로
 *  정의되지 않도록 validation 수행.
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* detect X$..., V$...
    *  = initial : 0
    *   -> in case of "X$" or "V$" : 1
    *    --> in case of repeated "X$" or "V$" : 1
    *    --> in case of normal table : 3
    *   -> in case of normal table : 2
    *    --> in case of "X$" or "V$" : 3
    *    --> in case of normal table : 2
    *
    *  BUGBUG      , jhseong, define하자.
    *  if   1      : special tables for FixedTable, Performance View
    *  else 2      : normal tables
    *  else 0 or 3 : ERROR
    *
    */

    QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus = 0;

    IDE_TEST( qcmFixedTable::validateTableName( aStatement,
                                                sParseTree->tableName )
              != IDE_SUCCESS );

    IDE_TEST( qmv::detectDollarTables( sParseTree->select )
              != IDE_SUCCESS );

    if( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 1 )
    {
        // PV이면 function을 override해서 일반 테이블과 구분짓는다.
        aStatement->myPlan->parseTree->validate =
            qcmPerformanceView::validateCreate;
        aStatement->myPlan->parseTree->execute =
            qcmPerformanceView::executeCreate;
    }
    else
    {
        IDE_ASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmPerformanceView::validateCreate(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *  create view v$XXX ... 구문을 validation함.
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree;
    IDE_RC                sSelectValidation;
    IDE_RC                sColumnValidation;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( qcmFixedTable::checkDuplicatedTable( aStatement,
                                                   sParseTree->tableName )
              != IDE_SUCCESS );

    sParseTree->flag &= ~QDV_OPT_STATUS_MASK;
    sParseTree->flag |= QDV_OPT_STATUS_VALID;


    //------------------------------------------------------------------
    // validation of SELECT statement
    //------------------------------------------------------------------
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        |= (QMV_PERFORMANCE_VIEW_CREATION_TRUE);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        &= ~(QMV_VIEW_CREATION_MASK);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        |= (QMV_VIEW_CREATION_TRUE);

    sSelectValidation = qmv::validateSelect(sParseTree->select);
    if (sSelectValidation != IDE_SUCCESS)
    {
        IDE_ASSERT( 0 );
    }

    //------------------------------------------------------------------
    // check SEQUENCE
    //------------------------------------------------------------------
    if ( (sParseTree->flag & QDV_OPT_STATUS_MASK) == QDV_OPT_STATUS_VALID )
    {
        // check CURRVAL, NEXTVAL
        if (sParseTree->select->myPlan->parseTree->currValSeqs != NULL ||
            sParseTree->select->myPlan->parseTree->nextValSeqs != NULL )
        {
            IDE_ASSERT( 0 );
        }
    }

    //------------------------------------------------------------------
    // validation of column name and count
    //------------------------------------------------------------------
    if ( (sParseTree->flag & QDV_OPT_STATUS_MASK) == QDV_OPT_STATUS_VALID )
    {
        sColumnValidation = qdbCreate::validateTargetAndMakeColumnList(aStatement);
        if (sColumnValidation != IDE_SUCCESS)
        {
            IDE_ASSERT( 0 );
        }

        sColumnValidation = qdbCommon::validateColumnListForCreate(
            aStatement,
            sParseTree->columns,
            ID_FALSE );
        
        if (sColumnValidation != IDE_SUCCESS)
        {
            IDE_ASSERT( 0 );
        }
    }
    else if ( (sParseTree->flag & QDV_OPT_STATUS_MASK) != QDV_OPT_STATUS_VALID )
    {
        IDE_ASSERT( 0 );
    }

    sParseTree->TBSAttr.mID = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPerformanceView::executeCreate(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *  create view v$XXX ... 구문을 실행함.
 *  실행 결과는 iduFixedTableDesc 자료구조로 fixedTable에 view가 등록됨.
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( registerOnIduFixedTableDesc( sParseTree ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

class iduFixedTableMemory;
IDE_RC
qcmPerformanceView::nullBuildRecord( idvSQL              * /* aStatistics */,
                                     void                * /* aHeader */,
                                     void                * /* aDumpObj */,
                                     iduFixedTableMemory * /* aMemory */ )

{
/***********************************************************************
 *
 * Description :
 *  iduFixedTableDesc 에서 사용되는 null function pointer
 *
 * Implementation :
 *
 ***********************************************************************/

    return IDE_SUCCESS;
}
