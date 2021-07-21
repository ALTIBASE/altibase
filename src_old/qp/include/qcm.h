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
 * $Id: qcm.h 90861 2021-05-18 07:41:58Z jake.jang $
 **********************************************************************/

#ifndef _O_QCM_H_
#define _O_QCM_H_ 1

#include    <smiDef.h>
#include    <qc.h>
#include    <qtc.h>
#include    <qsx.h>
#include    <qcmTableInfo.h>
#include    <qcmFixedTable.h>
#include    <qcmPerformanceView.h>
#include    <qcmTableInfoMgr.h>
#include    <qdParseTree.h>

extern smiCursorProperties gMetaDefaultCursorProperty;

/**************************************************************
                       BASIC DEFINITIONS
 **************************************************************/

// check SYS_DATABASE_ record
#define QCM_META_MAJOR_VER              (9)
#define QCM_META_MINOR_VER              (3)
#define QCM_META_PATCH_VER              (1)

#define QCM_META_MAJOR_STR_VER          "9"
#define QCM_META_MINOR_STR_VER          "3"
#define QCM_META_PATCH_STR_VER          "1"

#define QCM_TABLES_SEQ_STARTVALUE       ((SLong)4)
#define QCM_TABLES_SEQ_INCREMETVALUE    ((SLong)1)
#define QCM_TABLES_SEQ_CACHEVALUE       ((SLong)100)
#define QCM_TABLES_SEQ_MAXVALUE         ((SLong)2097151)

/* 
 * PROJ-2206
 * tableID (QCM_TABLES_SEQ_MAXVALUE 2097151 ) 중복 되지 않게 더 큰 값으로 설정 한다.
 * with 절에 의해 생성된 view 테이블에 할당된다. 이후 with절에 의한 stmt가 인라인뷰로
 * 되면서 할당된 ID값이 동일한 뷰를 sameview로 간주하여 materialize 하도록 한다.
 * db에서 유일한 값이 아니라 statement에서만 유일한 값이다.
 * 0                         : inline view 의 ID
 * 4           ~ 2097151     : table ID, created view 의 ID
 * 2097151 + 1 ~ 3000000 - 1 : Fixed table, Performance View 의 ID
 * 3000000     ~ UINT MAX    : with 절에 의해 생성된 inline view의 ID
 */
#define QCM_WITH_TABLES_SEQ_MINVALUE    ((SLong)3000000)

#define QCM_TABLES_SEQ_MINVALUE         ((SLong)1)
#define QCM_TABLEID_SEQ_TBL_ID          (3)

#define QCM_META_SEQ_MAXVALUE           ((SLong)(2147383647))
#define QCM_META_SEQ_MAXVALUE_STR       "2147383647"
#define QCM_META_OBJECT_NAME_LEN        "128"
#define QCM_META_NAME_LEN               "40"
#define QCM_MAX_MINMAX_VALUE_LEN        "48"

/* PROJ-1812 ROLE
 * PUBLIC ROLE 추가
 * PUBLIC  =  0  <--- ROLE
 * SYSTEM_ =  1  <--- USER
 * SYS     =  2  <--- USER */
#define QCM_META_SEQ_MINVALUE_STR       "0"

#define QCM_MAX_META_INDICES            (10)  // maximum indices / table
#define QCM_MAX_META_COLUMNS            (255)
#define QCM_MAX_DEFAULT_VAL             (4000)
#define QCM_MAX_DEFAULT_VAL_STR         "4000"

#define QCM_MAX_SQL_STR_LEN             (9196)

#define QCM_META_CURSOR_FLAG (SMI_LOCK_READ|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE)

#define QCM_TABLES_TBL_ID                   (1)
#define QCM_COLUMNS_TBL_ID                  (2)

#define QCM_TABLES_COL_CNT                  (28)

#define QCM_COLUMNS_COL_CNT                 (18)

#define QC_SYS_OBJ_NAME_HEADER   ((SChar*) "__SYS_")

// PROJ-1502 PARTITIONED DISK TABLE
#define QC_SYS_PARTITIONED_OBJ_NAME_HEADER   ((SChar*) "__SYS_PART_")

#define QC_EMPTY_USER_ID        (0)
#define QC_PUBLIC_USER_ID       (0)
#define QC_SYSTEM_USER_ID       (1)
#define QC_SYS_USER_ID          (2)

// BUG-20767, BUG-24540, BUG-24629
#define QC_SYSTEM_USER_NAME     ((SChar*) "SYSTEM_")
#define QC_SYS_USER_NAME        ((SChar*) "SYS")


extern SChar      * gDBSequenceName[];

/* for formatting SQL statement */
#define QCM_SQL_INT32_FMT       "INTEGER'%"ID_INT32_FMT"'"
#define QCM_SQL_UINT32_FMT      "INTEGER'%"ID_UINT32_FMT"'"
#define QCM_SQL_CHAR_FMT        "CHAR'%s'"
#define QCM_SQL_BIGINT_FMT      "BIGINT'%"ID_INT64_FMT"'"
#define QCM_SQL_VARCHAR_FMT     "VARCHAR'%s'"
#define QCM_SQL_BYTE_FMT        "BYTE'%s'"
#define QCM_OID_TO_BIGINT       ULong
#define QCM_VULONG_TO_BIGINT    ULong
#define QCM_VULONG_TO_BIGINT    ULong
// for sharding 내부적으로 받은 string을 대소문자 구분하지 않고 처리하기 위해
// ex> 'a' -> A, '"a"' -> a
//     'A' -> A, '"A"' -> A
#define QCM_SQL_STRING_SKIP_FMT "\"%s\""

// BUG-20908
#define QCM_MAX_RESOLVE_SYNONYM         (64)
#define QCM_MAX_RESOLVE_SYNONYM_STR     ((SChar*) "64")

/**************************************************************
                       META TABLE NAMES
 **************************************************************/

#define QCM_DATABASE                  "SYS_DATABASE_"
#define QCM_DN_USERS                  "SYS_DN_USERS_"
#define QCM_TBS_USERS                 "SYS_TBS_USERS_"
#define QCM_USERS                     "DBA_USERS_" // BUG-41230 SYS_USERS_ => DBA_USERS_
#define QCM_TABLES                    "SYS_TABLES_"
#define QCM_COLUMNS                   "SYS_COLUMNS_"
#define QCM_INDICES                   "SYS_INDICES_"
#define QCM_CONSTRAINTS               "SYS_CONSTRAINTS_"
#define QCM_CONSTRAINT_COLUMNS        "SYS_CONSTRAINT_COLUMNS_"
#define QCM_INDEX_COLUMNS             "SYS_INDEX_COLUMNS_"

#define QCM_REPLICATIONS              "SYS_REPLICATIONS_"
#define QCM_REPL_RECEIVER             "SYS_REPL_RECEIVER_"
#define QCM_REPL_HOSTS                "SYS_REPL_HOSTS_"
#define QCM_REPL_ITEMS                "SYS_REPL_ITEMS_"
#define QCM_REPL_ITEMS_HISTORY        "SYS_REPL_ITEMS_HISTORY_"
#define QCM_REPL_RECOVERY_INFOS       "SYS_REPL_RECOVERY_INFOS_"
#define QCM_REPL_OFFLINE_DIR          "SYS_REPL_OFFLINE_DIR_" //PROJ-1915
#define QCM_REPL_ITEM_REPLACE_HISTORY         "SYS_REPL_ITEM_REPLACE_HISTORY_" 
#define QCM_PROCEDURES                "SYS_PROCEDURES_"
#define QCM_PROC_PARAS                "SYS_PROC_PARAS_"
#define QCM_PROC_PARSE                "SYS_PROC_PARSE_"
#define QCM_PROC_RELATED              "SYS_PROC_RELATED_"

// PROJ-1073 Package를 위한 Meta Table
#define QCM_PKGS                      "SYS_PACKAGES_"
#define QCM_PKG_PARAS                 "SYS_PACKAGE_PARAS_"
#define QCM_PKG_PARSE                 "SYS_PACKAGE_PARSE_"
#define QCM_PKG_RELATED               "SYS_PACKAGE_RELATED_"

// PROJ-1359 Trigger를 위한 Meta Table
#define QCM_TRIGGERS                  "SYS_TRIGGERS_"
#define QCM_TRIGGER_STRINGS           "SYS_TRIGGER_STRINGS_"
#define QCM_TRIGGER_UPDATE_COLUMNS    "SYS_TRIGGER_UPDATE_COLUMNS_"
#define QCM_TRIGGER_DML_TABLES        "SYS_TRIGGER_DML_TABLES_"

// PROJ-1362 LOB을 위한 메타테이블
#define QCM_LOBS                      "SYS_LOBS_"

// PROJ-2002 Column Security를 위한 메타테이블
#define QCM_SECURITY                  "SYS_SECURITY_"
#define QCM_ENCRYPTED_COLUMNS         "SYS_ENCRYPTED_COLUMNS_"

#define QCM_PRIVILEGES                "SYS_PRIVILEGES_"
#define QCM_GRANT_SYSTEM              "SYS_GRANT_SYSTEM_"
#define QCM_GRANT_OBJECT              "SYS_GRANT_OBJECT_"
#define QCM_XA_HEURISTIC_TRANS        "SYS_XA_HEURISTIC_TRANS_"
#define QCM_VIEWS                     "SYS_VIEWS_"
#define QCM_VIEW_PARSE                "SYS_VIEW_PARSE_"
#define QCM_VIEW_RELATED              "SYS_VIEW_RELATED_"
// PROJ-1076 Synonym
#define QCM_SYNONYM                   "SYS_SYNONYMS_"

// PROJ-1502 PARTITIONED DISK TABLE
#define QCM_PART_TABLES               "SYS_PART_TABLES_"
#define QCM_PART_INDICES              "SYS_PART_INDICES_"
#define QCM_TABLE_PARTITIONS          "SYS_TABLE_PARTITIONS_"
#define QCM_INDEX_PARTITIONS          "SYS_INDEX_PARTITIONS_"
#define QCM_PART_KEY_COLUMNS          "SYS_PART_KEY_COLUMNS_"
#define QCM_PART_LOBS                 "SYS_PART_LOBS_"

// PROJ-1442 Replication Online 중 DDL 허용
#define QCM_REPL_OLD_ITEMS            "SYS_REPL_OLD_ITEMS_"
#define QCM_REPL_OLD_COLUMNS          "SYS_REPL_OLD_COLUMNS_"
#define QCM_REPL_OLD_INDICES          "SYS_REPL_OLD_INDICES_"
#define QCM_REPL_OLD_INDEX_COLUMNS    "SYS_REPL_OLD_INDEX_COLUMNS_"

// PROJ-2642
#define QCM_REPL_OLD_CHECKS           "SYS_REPL_OLD_CHECKS_"
#define QCM_REPL_OLD_CHECK_COLUMNS    "SYS_REPL_OLD_CHECK_COLUMNS_"

// PROJ-1371 DIRECTORY를 위한 Meta Table
#define QCM_DIRECTORIES               "SYS_DIRECTORIES_"

/* BUG-21387 COMMENT */
#define QCM_COMMENTS                  "SYS_COMMENTS_"

/* PROJ-2207 Password policy support */
#define QCM_PASSWORD_HISTORY          "SYS_PASSWORD_HISTORY_"

/* PROJ-2211 Materialized View */
#define QCM_MATERIALIZED_VIEWS        "SYS_MATERIALIZED_VIEWS_"

// PROJ-1685
#define QCM_LIBRARIES                 "SYS_LIBRARIES_"

// PROJ-2223 audit
#define QCM_AUDIT                     "SYS_AUDIT_"
#define QCM_AUDIT_ALL_OPTS            "SYS_AUDIT_ALL_OPTS_"

// PROJ-2264 Dictionary table
#define QCM_COMPRESSION_TABLES        "SYS_COMPRESSION_TABLES_"

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
#define QCM_CONSTRAINT_RELATED        "SYS_CONSTRAINT_RELATED_"
#define QCM_INDEX_RELATED             "SYS_INDEX_RELATED_"

/* PROJ-1438 Job Scheduler */
#define QCM_JOBS                      "SYS_JOBS_"

#define QCM_DATABASE_LINKS_           "SYS_DATABASE_LINKS_"

/* PROJ-1812 ROLE */
#define QCM_USER_ROLES                "SYS_USER_ROLES_"

/* PROJ-2422 srid */
#define QCM_GEOMETRIES                "SYS_GEOMETRIES_"

/**************************************************************
                         INDEX ORDER
 **************************************************************/
/*
      The index order should be exactly same with
      sCrtMetaSql array of qcmCreate::runDDLforMETA
 */
// Index Order for DBA_USERS_
#define QCM_USER_USERID_IDX_ORDER                        (0)
#define QCM_USER_USERNAME_IDX_ORDER                      (1)

// Index Order for SYS_DN_USERS_
#define QCM_DN_USER_DN_AID_NAME_IDX_ORDER                (0)

// Index Order for SYS_TABLES_
#define QCM_TABLES_TABLENAME_USERID_IDX_ORDER            (0)
#define QCM_TABLES_TABLEID_IDX_ORDER                     (1)
#define QCM_TABLES_USERID_IDX_ORDER                      (2)
#define QCM_TABLES_TBSID_IDX_ORDER                       (3)

// Index Order for SYS_COLUMNS_
#define QCM_COLUMNS_COLUMID_IDX_ORDER                    (0)
#define QCM_COLUMNS_TABLEID_COLID_IDX_ORDER              (1)
#define QCM_COLUMNS_TABLEID_COLORDER_IDX_ORDER           (2)

// Index Order for SYS_INDICES_
#define QCM_INDICES_INDEXID_INDEX_TYPE_IDX_ORDER         (0)
#define QCM_INDICES_USERID_INDEXNAME_IDX_ORDER           (1)
#define QCM_INDICES_TABLEID_INDEXID_IDX_ORDER            (2)
#define QCM_INDICES_TBSID_IDX_ORDER                      (3)

// Index Order for SYS_CONSTRAINTS_
#define QCM_CONSTRAINTS_TABLEID_CONSTRAINTTYPE_IDX_ORDER (0)
#define QCM_CONSTRAINTS_TABLEID_INDEXID_IDX_ORDER        (1)
#define QCM_CONSTRAINTS_REFTABLEID_REFINDEXID_IDX_ORDER  (2)
#define QCM_CONSTRAINTS_USERID_CONSTID_IDX_ORDER         (3)
#define QCM_CONSTRAINTS_USERID_REFTABLEID_IDX_ORDER      (4)
#define QCM_CONSTRAINTS_CONSTID_IDX_ORDER                (5)
#define QCM_CONSTRAINTS_CONSTRAINTNAME_IDX_ORDER         (6)

// Index Order for SYS_CONSTRAINT_COLUMNS_
#define QCM_CONSTRAINT_COLUMNS_CONSTRAINT_ID_CONSTRAINT_COL_ORDER_IDX_ORDER (0)
#define QCM_CONSTRAINT_COLUMNS_TABLEID_IDX_ORDER         (1)

// Index Order for SYS_INDEX_COLUMNS_
#define QCM_INDEX_COLUMNS_INDEX_ID_COLUMN_ID_SORT_ORDER_IDX_ORDER (0)

// Index Order for SYS_REPLICATIONS_
#define QCM_REPL_INDEX_REPL_NAME                         (0)

// Index Order for SYS_REPL_RECEIVER_
#define QCM_REPL_RECEIVER_INDEX_REPL_NAME                 (0)

// Index Order for SYS_REPL_HOSTS_
#define QCM_REPLHOST_INDEX_HOSTNO                        (0)
#define QCM_REPLHOST_INDEX_NAME_N_IP_N_PORT              (1)
#define QCM_REPLHOST_INDEX_IP_N_PORT                     (2)

// Index Order for SYS_REPL_OFFLINE_DIR_
#define QCM_REPL_OFFLINE_DIR_INDEX_REPL_NAME             (0)

// Index Order for SYS_REPL_ITEMS_
#define QCM_REPLITEM_INDEX_NAME_N_OID                    (0)
#define QCM_REPLITEM_INDEX_OID                    (1)

// Index Order for SYS_REPL_ITEM_REPLACE_HISTORY_
#define QCM_REPL_ITEM_REPLACE_HISTORY_INDEX_NAME_N_OID   (0)

// Index Order for SYS_REPL_OLD_ITEMS_
#define QCM_REPLOLDITEM_INDEX_NAME_OID                   (0)

// Index Order for SYS_REPL_OLD_COLUMNS_
#define QCM_REPLOLDCOL_INDEX_NAME_OID_COL                (0)

// Index Order for SYS_REPL_OLD_INDICES_
#define QCM_REPLOLDIDX_INDEX_NAME_OID_IDX                (0)

// Index Order for SYS_REPL_OLD_INDEX_COLUMNS_
#define QCM_REPLOLDIDXCOL_INDEX_NAME_OID_IDX_COL         (0)

// Index Order for SYS_REPL_OLD_CHECKS_ 
#define QCM_REPLOLDCHECKS_INDEX_NAME_OID_CID             (0)

// Index Order for SYS_REPL_OLD_CHECK_COLUMNS_ 
#define QCM_REPLOLDCHECKCOLUMNS_INDEX_NAME_OID_CID       (0)

// Index Order for SYS_PROCEDURES_
#define QCM_PROCEDURES_PROCNAME_USERID_IDX_ORDER         (0)
#define QCM_PROCEDURES_PROCOID_IDX_ORDER                 (1)
#define QCM_PROCEDURES_USERID_IDX_ORDER                  (2)

// Index Order for SYS_PROC_PARAS_
#define QCM_PROC_PARAS_PROCOID_PARANAME_IDX_ORDER        (0)
#define QCM_PROC_PARAS_PROCOID_PARAORDER_IDX_ORDER       (1)

// Index Order for SYS_PROC_PARSE_
#define QCM_PROC_PARSE_USERID_IDX_ORDER        (0)
#define QCM_PROC_PARSE_PROCOID_SEQNO_IDX_ORDER (1)

// Index Order for SYS_PROC_RELATED_
#define QCM_PROC_RELATED_USERID_IDX_ORDER                 (0)
#define QCM_PROC_RELATED_PROCOID_IDX_ORDER                (1)
#define QCM_PROC_RELATED_RELUSERID_RELOBJNAME_RELOBJTYPE  (2)

// PROJ-1073 Package
// Index Order for SYS_PACKAGES_
#define QCM_PKGS_PKGNAME_PKGTYPE_USERID_IDX_ORDER                (0)
#define QCM_PKGS_PKGOID_IDX_ORDER                                (1)
#define QCM_PKGS_PKGTYPE_IDX_ORDER                               (2)
#define QCM_PKGS_USERID_IDX_ORDER                                (3)

// Index Order for SYS_PACKAGE_PARAS_
#define QCM_PKG_PARAS_OBJECTNAME_OBJECTOID_PARAORDER_IDX_ORDER   (0)
#define QCM_PKG_PARAS_OBJECTNAME_OBJECTOID_PARANAME_IDX_ORDER    (1)
#define QCM_PKG_PARAS_OBJECTOID_OBJECTNAME_PKGNAME_IDX_ORDER     (2)

// Index Order for SYS_PACKAGE_PARSE_
#define QCM_PKG_PARSE_PKGOID_PKGTYPE_SEQID_IDX_ORDER             (0)
#define QCM_PKG_PARSE_USERID_IDX_ORDER                           (1)

// Index Order for SYS_PACKAGE_RELATED_
#define QCM_PKG_RELATED_USERID_IDX_ORDER                         (0)
#define QCM_PKG_RELATED_PKGOID_IDX_ORDER                         (1)
#define QCM_PKG_RELATED_RELUSERID_RELOBJECTNAME_RELOBJECTTYPE    (2)

// PROJ-1359 Index Order for SYS_TRIGGERS_
#define QCM_TRIGGERS_USERID_TRIGGERNAME_INDEX             (0)
#define QCM_TRIGGERS_TABLEID_TRIGGEROID_INDEX             (1)

// PROJ-1359 Index Order for SYS_TRIGGER_STRINGS_
#define QCM_TRIGGER_SRTINGS_TRIGGEROID_SEQNO_INDEX        (0)
#define QCM_TRIGGER_SRTINGS_TABLEID_INDEX                 (1)

// PROJ-1359 Index Order for SYS_TRIGGER_UPDATE_COLUMNS_
#define QCM_TRIGGER_UPDATE_COLUMNS_TRIGGEROID_INDEX       (0)
#define QCM_TRIGGER_UPDATE_COLUMNS_TABLEID_INDEX          (1)

// PROJ-1359 Index Order for SYS_TRIGGER_DML_TABLES_
#define QCM_TRIGGER_DML_TABLES_TABLE_ID_INDEX             (0)
#define QCM_TRIGGER_DML_TABLES_TRIGGEROID_INDEX           (1)

// PROJ-1362 Index Order for SYS_LOBS_
#define QCM_LOBS_COLUMNID_IDX_ORDER             (0)
#define QCM_LOBS_TABLEID_COLUMNID_IDX_ORDER     (1)
#define QCM_LOBS_TBSID_IDX_ORDER                (2)

// PROJ-2002 Index Order for SYS_ENCRYPTED_COLUMNS_
#define QCM_ENCRYPTED_COLUMNS_COLUMNID_IDX_ORDER            (0)
#define QCM_ENCRYPTED_COLUMNS_TABLEID_COLUMNID_IDX_ORDER    (1)

// Index Order for SYS_PRIVILEGES_
#define QCM_PRIVILEGES_PRIV_ID_IDX_ORDER        (0)

// Index Order for SYS_GRANT_SYSTEM_
#define QCM_GRANT_SYSTEM_IDX1_ORDER             (0)

// Index Order for SYS_GRANT_OBJECT_
#define QCM_GRANT_OBJECT_IDX1_ORDER             (0)
#define QCM_GRANT_OBJECT_IDX2_ORDER             (1)
#define QCM_GRANT_OBJECT_IDX3_ORDER             (2)

// Index Order for SYS_XA_HEURISTIC_TRANS_
#define QCM_XA_HEURISTIC_TRANS_FORMATID_GLOBALTXID_BRANCHQUALIFIER_IDX_ORDER \
    (0)

// Index Order for SYS_VIEWS_
#define QCM_VIEWS_VIEWID_IDX_ORDER                          (0)
#define QCM_VIEWS_USERID_IDX_ORDER                          (1)

// Index Order for SYS_VIEW_PARSE_
#define QCM_VIEW_PARSE_USERID_IDX_ORDER                     (0)
#define QCM_VIEW_PARSE_VIEWID_SEQNO_IDX_ORDER               (1)

// Index Order for SYS_VIEW_RELATED_
#define QCM_VIEW_RELATED_USERID_IDX_ORDER                   (0)
#define QCM_VIEW_RELATED_VIEWID_IDX_ORDER                   (1)
#define QCM_VIEW_RELATED_RELUSERID_RELOBJNAME_RELOBJTYPE    (2)

// Index Order for SYS_TBS_USERS_
#define QCM_TBS_USERS_TBSID_IDX_ORDER                       (0)
// Proj-1076 Synonym
#define QCM_SYNONYMS_USERID_SYNONYMNAME_IDX_ORDER           (0)

// PROJ-1502 PARTITIONED DISK TABLE - BEGIN -

// Index Order for SYS_PART_TABLES_
#define QCM_PART_TABLES_IDX1_ORDER                          (0)

// Index Order for SYS_PART_INDICES_
#define QCM_PART_INDICES_IDX1_ORDER                         (0)

// Index Order for SYS_TABLE_PARTITIONS_
#define QCM_TABLE_PARTITIONS_IDX1_ORDER                     (0)
#define QCM_TABLE_PARTITIONS_IDX2_ORDER                     (1)
#define QCM_TABLE_PARTITIONS_IDX3_ORDER                     (2)
#define QCM_TABLE_PARTITIONS_IDX4_ORDER                     (3)
#define QCM_TABLE_PARTITIONS_IDX5_ORDER                     (4)

// Index Order for SYS_INDEX_PARTITIONS_
#define QCM_INDEX_PARTITIONS_IDX1_ORDER                     (0)
#define QCM_INDEX_PARTITIONS_IDX2_ORDER                     (1)
#define QCM_INDEX_PARTITIONS_IDX3_ORDER                     (2)
#define QCM_INDEX_PARTITIONS_IDX4_ORDER                     (3)

// Index Order for SYS_PART_KEY_COLUMNS_
#define QCM_PART_KEY_COLUMNS_IDX1_ORDER                     (0)

// Index Order for SYS_PART_LOBS_
#define QCM_PART_LOBS_IDX1_ORDER                            (0)
#define QCM_PART_LOBS_IDX2_ORDER                            (1)

// PROJ-1502 PARTITIONED DISK TABLE - END -

// PROJ-1371 Index Order for SYS_DIRECTORIES_
#define QCM_DIRECTORIES_DIRECTORYID_IDX_ORDER     (0)
#define QCM_DIRECTORIES_USERID_IDX_ORDER          (1)
#define QCM_DIRECTORIES_DIRECTORYNAME_IDX_ORDER   (2)

/* BUG-21387 COMMENT */
#define QCM_COMMENTS_IDX1_ORDER                             (0)

/* PROJ-2207 Password policy support */
#define QCM_PASSWORD_HISTORY_IDX1_ORDER               (0)
#define QCM_PASSWORD_HISTORY_IDX2_ORDER               (1)

/* PROJ-2211 Materialized View */
#define QCM_MATERIALIZED_VIEWS_IDX_NAME_USERID        (0)
#define QCM_MATERIALIZED_VIEWS_IDX_MVIEWID            (1)
#define QCM_MATERIALIZED_VIEWS_IDX_USERID             (2)

// PROJ-1685
#define QCM_LIBRARIES_IDX_NAME_USERID                 (0)
#define QCM_LIBRARIES_IDX_USERID                      (1)

// PROJ-2223 audit
#define QCM_AUDIT_ALL_OPTS_USERID_OBJNAME_IDX_ORDER   (0)

// PROJ-2264 Dictionary table
#define QCM_COMPRESSION_TABLES_IDX1_ORDER             (0)

/* BUG-35445 Index Order for SYS_CONSTRAINT_RELATED_ */
#define QCM_CONSTRAINT_RELATED_USERID_IDX_ORDER       (0)
#define QCM_CONSTRAINT_RELATED_TABLEID_IDX_ORDER      (1)
#define QCM_CONSTRAINT_RELATED_CONSTRAINTID_IDX_ORDER (2)
#define QCM_CONSTRAINT_RELATED_RELPROCNAME_RELUSERID  (3)

/* BUG-35445 Index Order for SYS_INDEX_RELATED_ */
#define QCM_INDEX_RELATED_USERID_IDX_ORDER            (0)
#define QCM_INDEX_RELATED_TABLEID_IDX_ORDER           (1)
#define QCM_INDEX_RELATED_INDEXID_IDX_ORDER           (2)
#define QCM_INDEX_RELATED_RELPROCNAME_RELUSERID       (3)

/* PROJ-1438 Job Scheduler for SYS_JOBS_ */
#define QCM_JOBS_ID_IDX_ORDER                         (0)
#define QCM_JOBS_NAME_IDX_ORDER                       (1)

/* PROJ-1812 ROLE */
#define QCM_USER_ROLES_IDX1_ORDER                     (0)
#define QCM_USER_ROLES_IDX2_ORDER                     (1)
#define QCM_USER_ROLES_IDX3_ORDER                     (2)

/**************************************************************
                  BASIC META TABLE COLUMN COUNTS
 **************************************************************/
#define QCM_SEQUENCE_COL_COUNT                   (0)

/* PROJ-2422 srid */
#define QCM_GEOMETRIES_COLUMN_ID_IDX_ORDER            (0)
#define QCM_GEOMETRIES_TABLE_ID_COLUMN_ID_IDX_ORDER   (1)

/**************************************************************
                         COLUMN ORDERS
 **************************************************************/
// sequence for ID
#define QCM_DB_SEQUENCE_TABLEID             (0)
#define QCM_DB_SEQUENCE_USERID              (1)
#define QCM_DB_SEQUENCE_INDEXID             (2)
#define QCM_DB_SEQUENCE_CONSTRID            (3)
#define QCM_DB_SEQUENCE_DIRECTORYID         (4)

#define QCM_DB_SEQUENCE_TABLE_PARTITION_ID  (5)
#define QCM_DB_SEQUENCE_INDEX_PARTITION_ID  (6)
#define QCM_DB_SEQUENCE_DATABASE_LINK_ID    (7)
#define QCM_DB_SEQUENCE_MVIEWID             (8)
#define QCM_DB_SEQUENCE_LIBRARYID           (9) // PROJ-1685
#define QCM_DB_SEQUENCE_DICTIONARY_ID       (10) // PROJ-2264 Dictionary table
#define QCM_DB_SEQUENCE_JOB_ID              (11) // PROJ-1438 Job Scheduler

// SYS_DATABASE_
#define QCM_DATABASE_DB_NAME_COL_ORDER          (0)
#define QCM_DATABASE_OWNER_DN_COL_ORDER         (1)
#define QCM_DATABASE_META_MAJOR_VER_COL_ORDER   (2)
#define QCM_DATABASE_META_MINOR_VER_COL_ORDER   (3)
#define QCM_DATABASE_META_PATCH_VER_COL_ORDER   (4)
#define QCM_DATABASE_PREV_META_MAJOR_VER_COL_ORDER   (5)
#define QCM_DATABASE_PREV_META_MINOR_VER_COL_ORDER   (6)
#define QCM_DATABASE_PREV_META_PATCH_VER_COL_ORDER   (7)

// SYS_DN_USERS_
#define QCM_DN_USERS_USER_DN_COL_ORDER          (0)
#define QCM_DN_USERS_USER_AID_COL_ORDER         (1)
#define QCM_DN_USERS_USER_NAME_COL_ORDER        (2)
#define QCM_DN_USERS_REGISTERED_COL_ORDER       (3)
#define QCM_DN_USERS_START_PERIOD_COL_ORDER     (4)
#define QCM_DN_USERS_END_PERIOD_COL_ORDER       (5)

// SYS_TABLES_
#define QCM_TABLES_USER_ID_COL_ORDER            (0)
#define QCM_TABLES_TABLE_ID_COL_ORDER           (1)
#define QCM_TABLES_TABLE_OID_COL_ORDER          (2)
#define QCM_TABLES_COLUMN_COUNT_COL_ORDER       (3)
#define QCM_TABLES_TABLE_NAME_COL_ORDER         (4)
#define QCM_TABLES_TABLE_TYPE_COL_ORDER         (5)
#define QCM_TABLES_REPLICATION_COUNT_COL_ORDER  (6)
#define QCM_TABLES_REPL_RECOERY_COUNT_COL_ORDER (7) // PROJ-1608 RECOVERY FROM REPLICATION
#define QCM_TABLES_MAXROW_COL_ORDER             (8)
#define QCM_TABLES_TBS_ID_COL_ORDER             (9)
#define QCM_TABLES_TBS_NAME_COL_ORDER           (10)
#define QCM_TABLES_PCTFREE_COL_ORDER            (11)
#define QCM_TABLES_PCTUSED_COL_ORDER            (12)
#define QCM_TABLES_INIT_TRANS_COL_ORDER         (13)
#define QCM_TABLES_MAX_TRANS_COL_ORDER          (14)
#define QCM_TABLES_INITEXTENTS_COL_ORDER        (15) // PROJ-1671 Bitmap Space Management
#define QCM_TABLES_NEXTEXTENTS_COL_ORDER        (16) // PROJ-1671 Bitmap Space Management
#define QCM_TABLES_MINEXTENTS_COL_ORDER         (17) // PROJ-1671 Bitmap Space Management
#define QCM_TABLES_MAXEXTENTS_COL_ORDER         (18) // PROJ-1671 Bitmap Space Management
#define QCM_TABLES_IS_PARTITIONED_COL_ORDER     (19) // PROJ-1502 PARTITIONED DISK TABLE
#define QCM_TABLES_TEMPORARY_COL_ORDER          (20) // PROJ-1407 Temporary Table
#define QCM_TABLES_HIDDEN_COL_ORDER             (21) // PROJ-1624 Global Non-partitioned Index
#define QCM_TABLES_ACCESS_COL_ORDER             (22) /* PROJ-2359 Table/Partition Access Option */
#define QCM_TABLES_PARALLEL_DEGREE_COL_ORDER    (23) /* PROJ-1071 Parallel Query */
#define QCM_TABLES_USABLE_COL_ORDER             (24) /* TASK-7307 DML Data Consistency in Shard */
#define QCM_TABLES_SHARD_FLAG_COL_ORDER         (25) /* TASK-7307 */
#define QCM_TABLES_CREATED_COL_ORDER            (26) // fix BUG-14394
#define QCM_TABLES_LAST_DDL_TIME_COL_ORDER      (27) // fix BUG-14394

// SYS_COLUMNS_
#define QCM_COLUMNS_TABLE_ID_COL_ORDER          (6)
#define QCM_COLUMNS_COLUMN_ORDER_COL_ORDER      (9)
#define QCM_COLUMNS_COLUMN_NAME_COL_ORDER       (10)
#define QCM_COLUMNS_IS_NULLABLE_COL_ORDER       (11)
#define QCM_COLUMNS_DEFAULT_VAL_COL_ORDER       (12)
#define QCM_COLUMNS_IN_ROW_SIZE_ORDER           (14) // PROJ-1557
#define QCM_COLUMNS_REPL_CONDITION              (15) // not used anymore
#define QCM_COLUMNS_IS_HIDDEN_COL_ORDER         (16) /* PROJ-1090 Function-based Index */
#define QCM_COLUMNS_IS_KEY_PRESERVED_COL_ORDER  (17) // PROJ-2204 Join Update, Delete

// SYS_INDICES_
#define QCM_INDICES_USER_ID_COL_ORDER           (0)
#define QCM_INDICES_TABLE_ID_COL_ORDER          (1)
#define QCM_INDICES_INDEX_ID_COL_ORDER          (2)
#define QCM_INDICES_INDEXNAME_COL_ORDER         (3)
#define QCM_INDICES_INDEXTYPE_ID_COL_ORDER      (4)
#define QCM_INDICES_ISUNIQUE_COL_ORDER          (5)
#define QCM_INDICES_COLUMN_CNT_COL_ORDER        (6)
#define QCM_INDICES_ISRANGE_COL_ORDER           (7)
#define QCM_INDICES_ISPERS_COL_ORDER            (8)
#define QCM_INDICES_ISDIRECTKEY_COL_ORDER       (9)  /* PROJ-2433 MEMORY BTREE DIRECTKEY */
#define QCM_INDICES_TBSID_COL_ORDER             (10)
#define QCM_INDICES_IS_PARTITIONED_COL_ORDER    (11) /* PROJ-1502 PARTITIONED DISK TABLE */
#define QCM_INDICES_INDEX_TABLE_ID_COL_ORDER    (12) /* PROJ-1624 Global Non-partitioned Index */
#define QCM_INDICES_CREATED_COL_ORDER           (13) /* fix BUG-14394 */
#define QCM_INDICES_LAST_DDL_TIME_COL_ORDER     (14) /* fix BUG-14394 */

// SYS_CONSTRAINTS_
#define QCM_CONSTRAINTS_USER_ID_COL_ORDER                   (0)
#define QCM_CONSTRAINTS_TABLE_ID_COL_ORDER                  (1)
#define QCM_CONSTRAINTS_CONSTRAINT_ID_COL_ORDER             (2)
#define QCM_CONSTRAINTS_CONSTRAINT_NAME_COL_ORDER           (3)
#define QCM_CONSTRAINTS_CONSTRAINT_TYPE_COL_ORDER           (4)
#define QCM_CONSTRAINTS_INDEX_ID_COL_ORDER                  (5)
#define QCM_CONSTRAINTS_COLUMN_CNT_COL_ORDER                (6)
#define QCM_CONSTRAINTS_REFERENCED_TABLE_ID_COL_ORDER       (7)
#define QCM_CONSTRAINTS_REFERENCED_CONSTRAINT_ID_COL_ORDER  (8)
#define QCM_CONSTRAINTS_DELETE_RULE_COL_ORDER               (9)
#define QCM_CONSTRAINTS_CHECK_CONDITION_COL_ORDER           (10) /* PROJ-1107 Check Constraint 지원 */
#define QCM_CONSTRAINTS_VALIDATED_ORDER                     (11) // PROJ-1874 FK Novalidate

/* SYS_CONSTRAINT_COLUMNS_ */
#define QCM_CONSTRAINT_COLUMNS_CONSTRAINT_ID_COL_ORDER  (2)
#define QCM_CONSTRAINT_COLUMNS_CONSTRAINT_COL_ORDER     (3)
#define QCM_CONSTRAINT_COLUMNS_COLUMN_ID_COL_ORDER      (4)

/* DBA_USERS_ */
#define QCM_USERS_USER_ID_COL_ORDER        (0)
#define QCM_USERS_USER_NAME_COL_ORDER      (1)
#define QCM_USERS_USER_PASSWD_COL_ORDER    (2)
#define QCM_USERS_DEFAULT_TBS_ID_COL_ORDER (3)
#define QCM_USERS_TEMP_TBS_ID_COL_ORDER    (4)
/* PROJ-2207 Password policy support */
#define QCM_USERS_ACCOUNT_LOCK             (5)
#define QCM_USERS_ACCOUNT_LOCK_DATE        (6)
#define QCM_USERS_PASSWORD_LIMIT_FLAG      (7)
#define QCM_USERS_FAILED_LOGIN_ATTEMPTS    (8)
#define QCM_USERS_FAILED_LOGIN_COUNT       (9)
#define QCM_USERS_PASSWORD_LOCK_TIME       (10)
#define QCM_USERS_PASSWORD_EXPIRY_DATE     (11)
#define QCM_USERS_PASSWORD_LIFE_TIME       (12)
#define QCM_USERS_PASSWORD_GRACE_TIME      (13)
#define QCM_USERS_PASSWORD_REUSE_DATE      (14)
#define QCM_USERS_PASSWORD_REUSE_TIME      (15)
#define QCM_USERS_PASSWORD_REUSE_MAX       (16)
#define QCM_USERS_PASSWORD_REUSE_COUNT     (17)
#define QCM_USERS_PASSWORD_VERIFY_FUNCTION (18)
/* PROJ-1812 ROLE */
#define QCM_USERS_USER_TYPE_COL_ORDER      (19)
#define QCM_USERS_DISABLE_TCP_COL_ORDER    (20) /* PROJ-2474 SSL/TLS Support */
#define QCM_USERS_CREATED                  (21)
#define QCM_USERS_LAST_DDL_TIME            (22)

// SYS_PROCEDURES_
#define QCM_PROCEDURES_USERID_COL_ORDER           (0)
#define QCM_PROCEDURES_PROCOID_COL_ORDER          (1)
#define QCM_PROCEDURES_PROCNAME_COL_ORDER         (2)
#define QCM_PROCEDURES_OBJECTTYP_COL_ORDER        (3)
#define QCM_PROCEDURES_STATUS_COL_ORDER           (4)
#define QCM_PROCEDURES_PARANUM_COL_ORDER          (5)
#define QCM_PROCEDURES_RETURNDATATYPE_COL_ORDER   (6)
#define QCM_PROCEDURES_RETURNSIZE_COL_ORDER       (7)
#define QCM_PROCEDURES_RETURNLENGTH_COL_ORDER     (8)
#define QCM_PROCEDURES_RETURNPRECISION_COL_ORDER  (9)
#define QCM_PROCEDURES_RETURNSCALE_COL_ORDER      (10)
#define QCM_PROCEDURES_PARSENO_COL_ORDER          (11)
#define QCM_PROCEDURES_PARSELEN_COL_ORDER         (12)
#define QCM_PROCEDURES_CREATED_COL_ORDER          (13) // fix BUG-14394
#define QCM_PROCEDURES_LAST_DDL_TIME_COL_ORDER    (14) // fix BUG-14394
#define QCM_PROCEDURES_AUTHID_COL_ORDER           (15) /* BUG-45306 PSM AUTHID */

// SYS_PROC_PARAS_
#define QCM_PROC_PARAS_USERID_COL_ORDER      (0)
#define QCM_PROC_PARAS_PROCOID_COL_ORDER     (1)
#define QCM_PROC_PARAS_PARANAME_COL_ORDER    (2)
#define QCM_PROC_PARAS_PARAORDER_COL_ORDER   (3)
#define QCM_PROC_PARAS_INOUTTYPE_COL_ORDER   (4)
#define QCM_PROC_PARAS_DATATYPE_COL_ORDER    (5)
#define QCM_PROC_PARAS_SIZE_COL_ORDER        (6)
#define QCM_PROC_PARAS_LENGTH_COL_ORDER      (7)
#define QCM_PROC_PARAS_PRECISION_COL_ORDER   (8)
#define QCM_PROC_PARAS_SCALE_COL_ORDER       (9)
#define QCM_PROC_PARAS_DEFAULTVA_COL_ORDER   (10)

// SYS_PROC_PARSE_
#define QCM_PROC_PARSE_USERID_COL_ORDER  (0)
#define QCM_PROC_PARSE_PROCOID_COL_ORDER (1)
#define QCM_PROC_PARSE_SEQNO_COL_ORDER   (2)
#define QCM_PROC_PARSE_PARSE_COL_ORDER   (3)

// SYS_PROC_RELATED_
#define QCM_PROC_RELATED_USERID_COL_ORDER              (0)
#define QCM_PROC_RELATED_PROCOID_COL_ORDER             (1)
#define QCM_PROC_RELATED_RELATEDUSERID_COL_ORDER       (2)
#define QCM_PROC_RELATED_RELATEDOBJECTNAME_COL_ORDER   (3)
#define QCM_PROC_RELATED_RELATEDOBJECTTYPE_COL_ORDER   (4)

// PROJ-1073 Package
// SYS_PACKAGES_
#define QCM_PKGS_USERID_COL_ORDER                   (0)
#define QCM_PKGS_PKGOID_COL_ORDER                   (1)
#define QCM_PKGS_PKGNAME_COL_ORDER                  (2)
#define QCM_PKGS_PKGTYPE_COL_ORDER                  (3)
#define QCM_PKGS_STATUS_COL_ORDER                   (4)
#define QCM_PKGS_CREATED_COL_ORDER                  (5)
#define QCM_PKGS_LASTTIMEDDL_COL_ORDER              (6)
#define QCM_PKGS_AUTHID_COL_ORDER                   (7)      /* BUG-45306 PSM AUTHID */

// SYS_PACKAGE_PARAS_
#define QCM_PKG_PARAS_USERID_COL_ORDER              (0)
#define QCM_PKG_PARAS_OBJECTNAME_COL_ORDER          (1)
#define QCM_PKG_PARAS_PKGNAME_COL_ORDER             (2)
#define QCM_PKG_PARAS_OBJECTOID_COL_ORDER           (3)
#define QCM_PKG_PARAS_SUBID_COL_ORDER               (4)
#define QCM_PKG_PARAS_SUBTYPE_COL_ORDER             (5)
#define QCM_PKG_PARAS_PARANAME_COL_ORDER            (6)
#define QCM_PKG_PARAS_PARAORDER_COL_ORDER           (7)
#define QCM_PKG_PARAS_INOUTTYPE_COL_ORDER           (8)
#define QCM_PKG_PARAS_DATATYPE_COL_ORDER            (9)
#define QCM_PKG_PARAS_LANGID_COL_ORDER              (10)
#define QCM_PKG_PARAS_SIZE_COL_ORDER                (11)
#define QCM_PKG_PARAS_PRECISON_COL_ORDER            (12)
#define QCM_PKG_PARAS_SCALE_COL_ORDER               (13)
#define QCM_PKG_PARAS_DEFAULTVAL_COL_ORDER          (14)

// SYS_PACKAGE_PARSE_
#define QCM_PKG_PARSE_USERID_COL_ORDER              (0)
#define QCM_PKG_PARSE_PKGOID_COL_ORDER              (1)
#define QCM_PKG_PARSE_PKGTYPE_COL_ORDER             (2)
#define QCM_PKG_PARSE_SEQNO_COL_ORDER               (3)
#define QCM_PKG_PARSE_PARSE_COL_ORDER               (4)

// SYS_PACKAGE_RELATED_
#define QCM_PKG_RELATED_USERID_COL_ORDER            (0)
#define QCM_PKG_RELATED_PKGOID_COL_ORDER            (1)
#define QCM_PKG_RELATED_RELATEDUSERID_COL_ORDER     (2)
#define QCM_PKG_RELATED_RELATEDOBJECTNAME_COL_ORDER (3)
#define QCM_PKG_RELATED_RELATEDOBJECTTYPE_COL_ORDER (4)

//=========================================================
// [PROJ-1359] Trigger를 위한 Meta Table 구조
// SYS_TRIGGERS_ : Trigger의 전체적인 메타 정보를 관리한다.
//
// USER_ID          : User ID
// USER_NAME        : User Name
// TRIGGER_OID      : Trigger Object ID
// TRIGGER_NAME     : Trigger Name
// TABLE_ID         : Trigger가 참조하는 Table의 ID
// IS_ENABLE        : 수행 여부의 지정
// EVENT_TIME       : BEFORE or AFTER Event
// EVENT_TYPE       : Trigger Event의 종류(INSERT, DELETE, UPDATE)
// UPDATE_COLUMN_CNT: Update Event에 포함된 Column의 개수
// GRANULARITY      : Action Granularity
// REF_ROW_CNT      : Referencing Row 의 개수
// SUBSTING_CNT     : Trigger 생성문이 분할된 개수
// STRING_LENGTH    : Trigger 생성문의 총 길이
//=========================================================

// PROJ-1359 SYS_TRIGGERS_
#define QCM_TRIGGERS_USER_ID                    (0)
#define QCM_TRIGGERS_USER_NAME                  (1)
#define QCM_TRIGGERS_TRIGGER_OID                (2)
#define QCM_TRIGGERS_TRIGGER_NAME               (3)
#define QCM_TRIGGERS_TABLE_ID                   (4)
#define QCM_TRIGGERS_IS_ENABLE                  (5)
#define QCM_TRIGGERS_EVENT_TIME                 (6)
#define QCM_TRIGGERS_EVENT_TYPE                 (7)
#define QCM_TRIGGERS_UPDATE_COLUMN_CNT          (8)
#define QCM_TRIGGERS_GRANULARITY                (9)
#define QCM_TRIGGERS_REF_ROW_CNT                (10)
#define QCM_TRIGGERS_SUBSTRING_CNT              (11)
#define QCM_TRIGGERS_STRING_LENGTH              (12)
#define QCM_TRIGGERS_CREATED                    (13) // fix BUG-14394
#define QCM_TRIGGERS_LAST_DDL_TIME              (14) // fix BUG-14394

//=========================================================
// [PROJ-1359] Trigger 생성 구문을 포함하고 있는 Meta Table
// SYS_TRIGGER_STRINGS_ : Trigger의 재컴파일시에 사용된다.
// TABLE_ID      : Table ID
// TRIGGER_OID   : Trigger의 OID
// SEQNO         : Trigger 생성 구문의 Substring 순서
// SUBSTRING     : Trigger 생성 구문의 실제 Sub-String
//=========================================================

// PROJ-1359 SYS_TRIGGER_STRINGS_
#define QCM_TRIGGER_STRINGS_TABLE_ID            (0)
#define QCM_TRIGGER_STRINGS_TRIGGER_OID         (1)
#define QCM_TRIGGER_STRINGS_SEQNO               (2)
#define QCM_TRIGGER_STRINGS_SUBSTRING           (3)

//=========================================================
// [PROJ-1359] UPDATE Event의 OF 구문에 존재하는 Column을 관리
// SYS_TRIGGER_UPDATE_COLUMNS_
// TABLE_ID      : Table ID
// TRIGGER_OID   : Trigger의 OID
// COLUMN_ID     : OF 구문의 Column ID
//=========================================================

// PROJ-1359 SYS_TRIGGER_UPDATE_COLUMNS_
#define QCM_TRIGGER_UPDATE_COLUMNS_TABLE_ID     (0)
#define QCM_TRIGGER_UPDATE_COLUMNS_TRIGGER_OID  (1)
#define QCM_TRIGGER_UPDATE_COLUMNS_COLUMN_ID    (2)

//=========================================================
// [PROJ-1359] Action Body가 DML로 접근하는 테이블을 관리
// SYS_TRIGGER_DML_TABLES_
// TABLE_ID          : Table ID
// TRIGGER_OID       : Trigger의 OID
// DML_TABLE_ID      : DML로 접근하는 Table의 ID
// STMT_TYPE         : 해당 Table에 접근하는 STMT의 종류
//=========================================================

// PROJ-1359 SYS_TRIGGER_DML_TABLES_
#define QCM_TRIGGER_DML_TABLES_TABLE_ID         (0)
#define QCM_TRIGGER_DML_TABLES_TRIGGER_OID      (1)
#define QCM_TRIGGER_DML_TABLES_DML_TABLE_ID     (2)
#define QCM_TRIGGER_DML_TABLES_DML_TYPE         (3)

//=========================================================
// [PROJ-1362] LOB컬럼의 속성을 저장하는 테이블
// SYS_LOBS_
// USER_ID           : LOB컬럼을 저장한 테이블을 소유한 사용자의 ID
// TABLE_ID          : LOB컬럼을 저장한 테이블의 ID
// TBS_ID            : LOB컬럼이 저장된 테이블스페이스의 ID
// LOGGING           : LOB컬럼의 logging 속성
// BUFFER            : LOB컬럼의 buffer 속성
// IS_DEFAULT_TBS    : LOB컬럼의 테이블스페이스 지정 여부
//=========================================================
#define QCM_LOBS_USER_ID_COL_ORDER              (0)
#define QCM_LOBS_TABLE_ID_COL_ORDER             (1)
#define QCM_LOBS_COLUMN_ID_COL_ORDER            (2)
#define QCM_LOBS_TBS_ID_COL_ORDER               (3)
#define QCM_LOBS_LOGGING_COL_ORDER              (4)
#define QCM_LOBS_BUFFER_COL_ORDER               (5)
#define QCM_LOBS_IS_DEFAULT_TBS_COL_ORDER       (6)

//=========================================================
// [PROJ-2002] Column Security 컬럼의 속성을 저장하는 테이블
// SYS_SECURITY_
// MODULE_NAME       : 보안모듈의 이름
// ECC_POLICY_NAME   : ECC Policy의 이름
// ECC_POLICY_CODE   : ECC Policy의 검증 코드
//=========================================================
#define QCM_SECURITY_MODULE_NAME_COL_ORDER             (0)
#define QCM_SECURITY_MODULE_VERSION_COL_ORDER          (1)
#define QCM_SECURITY_MODULE_ECC_POLICY_NAME_COL_ORDER  (2)
#define QCM_SECURITY_MODULE_ECC_POLICY_CODE_COL_ORDER  (3)

//=========================================================
// [PROJ-2002] Column Security 컬럼의 속성을 저장하는 테이블
// SYS_ENCRYPTED_COLUMNS_
// USER_ID           : 보안컬럼을 저장한 테이블을 소유한 사용자의 ID
// TABLE_ID          : 보안컬럼을 저장한 테이블의 ID
// COLUMN_ID         : 보안컬럼의 ID
// ENCRYPT_PRECISION : 보안컬럼의 precision
// POLICY_NAME       : 보안컬럼 암호화에 사용한 policy 이름
// POLICY_CODE       : 보안컬럼 암호화에 사용한 policy의 검증 코드
//=========================================================
#define QCM_ENCRYPTED_COLUMNS_USER_ID_COL_ORDER            (0)
#define QCM_ENCRYPTED_COLUMNS_TABLE_ID_COL_ORDER           (1)
#define QCM_ENCRYPTED_COLUMNS_COLUMN_ID_COL_ORDER          (2)
#define QCM_ENCRYPTED_COLUMNS_ENCRYPT_PRECISION_COL_ORDER  (3)
#define QCM_ENCRYPTED_COLUMNS_POLICY_NAME_COL_ORDER        (4)
#define QCM_ENCRYPTED_COLUMNS_POLICY_CODE_COL_ORDER        (5)

// SYS_REPLICATIONS_
#define QCM_REPLICATION_REPL_NAME                   (0)
#define QCM_REPLICATION_LAST_USED_HOST_NO           (1)
#define QCM_REPLICATION_HOST_COUNT                  (2)
#define QCM_REPLICATION_IS_STARTED                  (3)
#define QCM_REPLICATION_XSN                         (4)
#define QCM_REPLICATION_ITEM_COUNT                  (5)
#define QCM_REPLICATION_CONFLICT_RESOLUTION         (6)
#define QCM_REPLICATION_REPL_MODE                   (7)
#define QCM_REPLICATION_ROLE                        (8)
#define QCM_REPLICATION_OPTIONS                     (9)
#define QCM_REPLICATION_INVALID_RECOVERY            (10)
#define QCM_REPLICATION_REMOTE_FAULT_DETECT_TIME    (11)
#define QCM_REPLICATION_GIVE_UP_TIME                (12)
#define QCM_REPLICATION_GIVE_UP_XSN                 (13)
#define QCM_REPLICATION_PARALLEL_APPLIER_COUNT      (14)
#define QCM_REPLICATION_APPLIER_INIT_BUFFER_SIZE    (15)
#define QCM_REPLICATION_REMOTE_XSN                  (16)
#define QCM_REPLICATION_PEER_REPL_NAME              (17)
#define QCM_REPLICATION_REMOTE_LAST_DDL_XSN         (18)
#define QCM_REPLICATION_XLOGFILE_CURRENT_LSN_FILENO (19)
#define QCM_REPLICATION_XLOGFILE_CURRENT_LSN_OFFSET (20)

// SYS_REPL_RECEIVER
#define QCM_REPL_RECEIVER_REPL_NAME                   (0)
#define QCM_REPL_RECEIVER_REMOTE_XSN                  (1)
#define QCM_REPL_RECEIVER_XLOGFILE_CURRENT_LSN_FILENO (2)
#define QCM_REPL_RECEIVER_XLOGFILE_CURRENT_LSN_OFFSET (3)

// SYS_REPL_HOSTS_
#define QCM_REPLHOST_HOST_NO                    (0)
#define QCM_REPLHOST_REPL_NAME                  (1)
#define QCM_REPLHOST_HOST_IP                    (2)
#define QCM_REPLHOST_PORT_NO                    (3)
#define QCM_REPLHOST_CONN_TYPE                  (4)
#define QCM_REPLHOST_IB_LATENCY                 (5)


/* PROJ-1915 */
// SYS_REPL_OFFLINE_DIR_
#define QCM_REPL_OFFLINE_DIR_REPL_NAME          (0)
#define QCM_REPL_OFFLINE_DIR_LFGID              (1)
#define QCM_REPL_OFFLINE_DIR_PATH               (2)


// SYS_REPL_ITEMS_
#define QCM_REPLITEM_REPL_NAME                  (0)
#define QCM_REPLITEM_TABLE_OID                  (1)
#define QCM_REPLITEM_LOCAL_USER                 (2)
#define QCM_REPLITEM_LOCAL_TABLE                (3)
#define QCM_REPLITEM_LOCAL_PARTITION            (4)
#define QCM_REPLITEM_REMOTE_USER                (5)
#define QCM_REPLITEM_REMOTE_TABLE               (6)
#define QCM_REPLITEM_REMOTE_PARTITION           (7)
#define QCM_REPLITEM_IS_PARTITION               (8)
#define QCM_REPLITEM_INVALID_MAX_SN             (9)
#define QCM_REPLITEM_CONDITION                  (10)  // not used anymore
#define QCM_REPLITEM_REPLICATION_UNIT           (11)
#define QCM_REPLITEM_IS_CONDITION_SYNCED        (12)

// SYS_REPL_ITEMS_HISTORY_
#define QCM_REPLITEM_REPL_NAME                  (0)
#define QCM_REPLITEM_USER_NAME                  (1)
#define QCM_REPLITEM_TABLE_NAME                 (2)
#define QCM_REPLITEM_PARTITION_NAME             (3)
#define QCM_REPLITEM_OLD_OID                    (4)
#define QCM_REPLITEM_NEW_OID                    (5)

// SYS_REPL_OLD_ITEMS_
#define QCM_REPLOLDITEMS_REPLICATION_NAME       (0)
#define QCM_REPLOLDITEMS_TABLE_OID              (1)
#define QCM_REPLOLDITEMS_USER_NAME              (2)
#define QCM_REPLOLDITEMS_TABLE_NAME             (3)
#define QCM_REPLOLDITEMS_PARTITION_NAME         (4)
#define QCM_REPLOLDITEMS_PRIMARY_KEY_INDEX_ID   (5)
#define QCM_REPLOLDITEMS_REMOTE_USER_NAME       (6)
#define QCM_REPLOLDITEMS_REMOTE_TABLE_NAME      (7)
#define QCM_REPLOLDITEMS_REMOTE_PARTITION_NAME  (8)
#define QCM_REPLOLDITEMS_PARTITION_ORDER        (9)
#define QCM_REPLOLDITEMS_PARTITION_MIN_VALUE    (10)
#define QCM_REPLOLDITEMS_PARTITION_MAX_VALUE    (11)
#define QCM_REPLOLDITEMS_INVALID_MAX_SN         (12)
#define QCM_REPLOLDITEMS_TABLE_ID               (13)
#define QCM_REPLOLDITEMS_TABLE_PARTITION_TYPE   (14)
#define QCM_REPLOLDITEMS_IS_PARTITION           (15)
#define QCM_REPLOLDITEMS_REPLICATION_UNIT       (16)
#define QCM_REPLOLDITEMS_TBS_TYPE               (17)
#define QCM_REPLOLDITEMS_PARTITION_METHOD       (18)
#define QCM_REPLOLDITEMS_PARTITION_COUNT        (19)

// SYS_REPL_OLD_COLUMNS_
#define QCM_REPLOLDCOLS_REPLICATION_NAME        (0)
#define QCM_REPLOLDCOLS_TABLE_OID               (1)
#define QCM_REPLOLDCOLS_COLUMN_NAME             (2)
#define QCM_REPLOLDCOLS_MT_DATATYPE_ID          (3)
#define QCM_REPLOLDCOLS_MT_LANGUAGE_ID          (4)
#define QCM_REPLOLDCOLS_MT_FLAG                 (5)
#define QCM_REPLOLDCOLS_MT_PRECISION            (6)
#define QCM_REPLOLDCOLS_MT_SCALE                (7)
#define QCM_REPLOLDCOLS_MT_ECRYPT_PRECISION     (8)
#define QCM_REPLODLCOLS_MT_POLICY_NAME          (9)
#define QCM_REPLOLDCOLS_MT_SRID                 (10)
#define QCM_REPLOLDCOLS_SM_ID                   (11)
#define QCM_REPLOLDCOLS_SM_FLAG                 (12)
#define QCM_REPLOLDCOLS_SM_OFFSET               (13)
#define QCM_REPLOLDCOLS_SM_VARORDER             (14)
#define QCM_REPLOLDCOLS_SM_SIZE                 (15)
#define QCM_REPLOLDCOLS_SM_DIC_TABLE_OID        (16)
#define QCM_REPLOLDCOLS_SM_COL_SPACE            (17)
#define QCM_REPLOLDCOLS_QP_FLAG                 (18)
#define QCM_REPLOLDCOLS_DEFAULT_VAL_STR         (19)

// SYS_REPL_OLD_INDICES_
#define QCM_REPLOLDIDXS_REPLICATION_NAME        (0)
#define QCM_REPLOLDIDXS_TABLE_OID               (1)
#define QCM_REPLOLDIDXS_INDEX_ID                (2)
#define QCM_REPLOLDIDXS_INDEX_NAME              (3)
#define QCM_REPLOLDIDXS_TYPE_ID                 (4)
#define QCM_REPLOLDIDXS_IS_UNIQUE               (5)
#define QCM_REPLOLDIDXS_IS_LOCAL_UNIQUE         (6)
#define QCM_REPLOLDIDXS_IS_RANGE                (7)

// SYS_REPL_OLD_INDEX_COLUMNS_
#define QCM_REPLOLDIDXCOLS_REPLICATION_NAME     (0)
#define QCM_REPLOLDIDXCOLS_TABLE_OID            (1)
#define QCM_REPLOLDIDXCOLS_INDEX_ID             (2)
#define QCM_REPLOLDIDXCOLS_KEY_COLUMN_ID        (3)
#define QCM_REPLOLDIDXCOLS_KEY_COLUMN_FLAG      (4)
#define QCM_REPLOLDIDXCOLS_COMPOSITE_ORDER      (5)

// SYS_REPL_OLD_CHECKS_ 
#define QCM_REPLOLDCHECKS_REPLICATION_NAME      (0)
#define QCM_REPLOLDCHECKS_TABLE_OID             (1)
#define QCM_REPLOLDCHECKS_CONSTRAINT_ID         (2)
#define QCM_REPLOLDCHECKS_CHECK_NAME            (3)
#define QCM_REPLOLDCHECKS_CONDITION             (4)

// SYS_REPL_OLD_CHECK_COLUMNS_
#define QCM_REPLOLDCHECKCOLUMNS_REPLICATION_NAME (0)
#define QCM_REPLOLDCHECKCOLUMNS_TABLE_OID       (1)
#define QCM_REPLOLDCHECKCOLUMNS_CONSTRAINT_ID   (2)
#define QCM_REPLOLDCHECKCOLUMNS_COLUMN_ID       (3)

// SYS_REPL_RECOVERY_INFOS_
#define QCM_REPLRECOVERY_REPL_NAME              (0)
#define QCM_REPLRECOVERY_MASTER_BEGIN_SN        (1)
#define QCM_REPLRECOVERY_MASTER_COMMIT_SN       (2)
#define QCM_REPLRECOVERY_REPLICATED_BEGIN_SN    (3)
#define QCM_REPLRECOVERY_REPLICATED_COMMIT_SN   (4)

/* PROJ-2240 */
#define RP_COND_STATEMENT_MAX_LEN (16 + (QC_MAX_OBJECT_NAME_LEN * 2) + QC_CONDITION_LEN)

// SYS_REPL_ITEM_REPLACE_HISTORY_
#define QCM_REPL_ITEM_REPLACE_HISTORY_REPL_NAME                 (0)
#define QCM_REPL_ITEM_REPLACE_HISTORY_USER_NAME                 (1)
#define QCM_REPL_ITEM_REPLACE_HISTORY_TABLE_NAME                (2)
#define QCM_REPL_ITEM_REPLACE_HISTORY_PARTITION_NAME            (3)
#define QCM_REPL_ITEM_REPLACE_HISTORY_OLD_OID                   (4)
#define QCM_REPL_ITEM_REPLACE_HISTORY_NEW_OID                   (5)

// SYS_PRIVILEGES_
#define QCM_PRIVILEGES_PRIV_ID                (0)
#define QCM_PRIVILEGES_PRIV_TYPE              (1)
#define QCM_PRIVILEGES_PRIV_NAME              (2)

// SYS_GRANT_SYSTEM_
#define QCM_GRANT_SYSTEM_GRANTOR_ID           (0)
#define QCM_GRANT_SYSTEM_GRANTEE_ID           (1)
#define QCM_GRANT_SYSTEM_PRIV_ID              (2)

// SYS_GRANT_OBJECT_
#define QCM_GRANT_OBJECT_GRANTOR_ID           (0)
#define QCM_GRANT_OBJECT_GRANTEE_ID           (1)
#define QCM_GRANT_OBJECT_PRIV_ID              (2)
#define QCM_GRANT_OBJECT_USER_ID              (3)
#define QCM_GRANT_OBJECT_OBJ_ID               (4)
#define QCM_GRANT_OBJECT_OBJ_TYPE             (5)
#define QCM_GRANT_OBJECT_WITH_GRANT_OPTION    (6)

// SYS_XA_HEURISTIC_TRANS_
#define QCM_XA_HEURISTIC_TRANS_FORMAT_ID         (0)
#define QCM_XA_HEURISTIC_TRANS_GLOBAL_TX_ID      (1)
#define QCM_XA_HEURISTIC_TRANS_BRANCH_QUALIFIER  (2)
#define QCM_XA_HEURISTIC_TRANS_STATUS            (3)
#define QCM_XA_HEURISTIC_TRANS_OCCUR_TIME        (4)

// SYS_VIEWS_
#define QCM_VIEWS_USERID_COL_ORDER                      (0)
#define QCM_VIEWS_VIEWID_COL_ORDER                      (1)
#define QCM_VIEWS_STATUS_COL_ORDER                      (2)
#define QCM_VIEWS_READ_ONLY_COL_ORDER                   (3)

// SYS_VIEW_PARSE_
#define QCM_VIEW_PARSE_USERID_COL_ORDER                 (0)
#define QCM_VIEW_PARSE_VIEWID_COL_ORDER                 (1)
#define QCM_VIEW_PARSE_SEQNO_COL_ORDER                  (2)
#define QCM_VIEW_PARSE_PARSE_COL_ORDER                  (3)

// SYS_VIEW_RELATED_
#define QCM_VIEW_RELATED_USERID_COL_ORDER               (0)
#define QCM_VIEW_RELATED_VIEWID_COL_ORDER               (1)
#define QCM_VIEW_RELATED_RELATEDUSERID_COL_ORDER        (2)
#define QCM_VIEW_RELATED_RELATEDOBJECTNAME_COL_ORDER    (3)
#define QCM_VIEW_RELATED_RELATEDOBJECTTYPE_COL_ORDER    (4)

// PROJ-1371 SYS_DIRECTORIES_
#define QCM_DIRECTORIES_DIRECTORY_ID                    (0)
#define QCM_DIRECTORIES_USER_ID                         (1)
#define QCM_DIRECTORIES_DIRECTORY_NAME                  (2)
#define QCM_DIRECTORIES_DIRECTORY_PATH                  (3)
#define QCM_DIRECTORIES_CREATED                         (4) // fix BUG-14394
#define QCM_DIRECTORIES_LAST_DDL_TIME                   (5) // fix BUG-14394

// SYS_TBS_USERS_
#define QCM_TBS_USERS_TBS_ID_COL_ORDER                  (0)
#define QCM_TBS_USERS_USER_ID_COL_ORDER                 (1)
#define QCM_TBS_USERS_IS_ACCESS_COL_ORDER               (2)

// Proj-1076 SYS_SYNONYMS_
#define QCM_SYNONYMS_SYNONYM_OWNER_ID_COL_ORDER          (0)
#define QCM_SYNONYMS_SYNONYM_NAME_COL_ORDER              (1)
#define QCM_SYNONYMS_OBJECT_OWNER_NAME_COL_ORDER         (2)
#define QCM_SYNONYMS_OBJECT_NAME_COL_ORDER               (3)
#define QCM_SYNONYMS_CREATED_COL_ORDER                   (4) // fix BUG-14394
#define QCM_SYNONYMS_LAST_DDL_TIME_COL_ORDER             (5) // fix BUG-14394

// PROJ-1502 PARTITIONED DISK TABLE - BEGIN -

// SYS_PART_TABLES_
#define QCM_PART_TABLES_USER_ID_COL_ORDER                (0)
#define QCM_PART_TABLES_TABLE_ID_COL_ORDER               (1)
#define QCM_PART_TABLES_PARTITION_METHOD_COL_ORDER       (2)
#define QCM_PART_TABLES_PARTITION_KEY_COUNT_COL_ORDER    (3)
#define QCM_PART_TABLES_ROW_MOVEMENT_COL_ORDER           (4)

// SYS_PART_INDICES_
#define QCM_PART_INDICES_USER_ID_COL_ORDER               (0)
#define QCM_PART_INDICES_TABLE_ID_COL_ORDER              (1)
#define QCM_PART_INDICES_INDEX_ID_COL_ORDER              (2)
#define QCM_PART_INDICES_PARTITION_TYPE_COL_ORDER        (3)
#define QCM_PART_INDICES_IS_LOCAL_UNIQUE_COL_ORDER       (4)

// SYS_TABLE_PARTITIONS_
#define QCM_TABLE_PARTITIONS_USER_ID_COL_ORDER              (0)
#define QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER             (1)
#define QCM_TABLE_PARTITIONS_PARTITION_OID_COL_ORDER        (2)
#define QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER         (3)
#define QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER       (4)
#define QCM_TABLE_PARTITIONS_PARTITION_MIN_VALUE_COL_ORDER  (5)
#define QCM_TABLE_PARTITIONS_PARTITION_MAX_VALUE_COL_ORDER  (6)
#define QCM_TABLE_PARTITIONS_PARTITION_ORDER_COL_ORDER      (7)
#define QCM_TABLE_PARTITIONS_TBS_ID_COL_ORDER               (8)
#define QCM_TABLE_PARTITIONS_PARTITION_ACCESS_COL_ORDER     (9) /* PROJ-2359 Table/Partition Access Option */
#define QCM_TABLE_PARTITIONS_PARTITION_USABLE_COL_ORDER     (10) /* TASK-7307 DML Data Consistency in Shard */
#define QCM_TABLE_PARTITIONS_REPLICATION_COUNT_COL_ORDER    (11)
#define QCM_TABLE_PARTITIONS_REPL_RECOVERY_COUNT_COL_ORDER  (12)

// SYS_INDEX_PARTITIONS_
#define QCM_INDEX_PARTITIONS_USER_ID_COL_ORDER              (0)
#define QCM_INDEX_PARTITIONS_TABLE_ID_COL_ORDER             (1)
#define QCM_INDEX_PARTITIONS_INDEX_ID_COL_ORDER             (2)
#define QCM_INDEX_PARTITIONS_TABLE_PARTITION_ID_COL_ORDER   (3)
#define QCM_INDEX_PARTITIONS_INDEX_PARTITION_ID_COL_ORDER   (4)
#define QCM_INDEX_PARTITIONS_INDEX_PARTITION_NAME_COL_ORDER (5)
#define QCM_INDEX_PARTITIONS_PARTITION_MIN_VALUE_COL_ORDER  (6)
#define QCM_INDEX_PARTITIONS_PARTITION_MAX_VALUE_COL_ORDER  (7)
#define QCM_INDEX_PARTITIONS_TBS_ID_COL_ORDER               (8)

// SYS_PART_KEY_COLUMNS_
#define QCM_PART_KEY_COLUMNS_USER_ID_COL_ORDER              (0)
#define QCM_PART_KEY_COLUMNS_PARTITION_OBJ_ID_COL_ORDER     (1)
#define QCM_PART_KEY_COLUMNS_COLUMN_ID_COL_ORDER            (2)
#define QCM_PART_KEY_COLUMNS_OBJECT_TYPE_COL_ORDER          (3)
#define QCM_PART_KEY_COLUMNS_PART_COL_ORDER_COL_ORDER       (4)

// SYS_PART_LOBS_
#define QCM_PART_LOBS_USER_ID_COL_ORDER                     (0)
#define QCM_PART_LOBS_TABLE_ID_COL_ORDER                    (1)
#define QCM_PART_LOBS_PARTITION_ID_COL_ORDER                (2)
#define QCM_PART_LOBS_COLUMN_ID_COL_ORDER                   (3)
#define QCM_PART_LOBS_TBS_ID_COL_ORDER                      (4)
#define QCM_PART_LOBS_LOGGING_COL_ORDER                     (5)
#define QCM_PART_LOBS_BUFFER_COL_ORDER                      (6)

// PROJ-1502 PARTITIONED DISK TABLE - END -

/* BUG-21387 COMMENT */
#define QCM_COMMENTS_USER_NAME_COL_ORDER                    (0)
#define QCM_COMMENTS_TABLE_NAME_COL_ORDER                   (1)
#define QCM_COMMENTS_COLUMN_NAME_COL_ORDER                  (2)
#define QCM_COMMENTS_COMMENTS_COL_ORDER                     (3)

/* PORJ-2207 Password policy support */
#define QCM_PASSWORD_LIMITS_USER_ID                    (0)
#define QCM_PASSWORD_LIMITS_ACCOUNT_STATUS             (1)        
#define QCM_PASSWORD_LIMITS_FAILED_LOGIN_ATTEMPTS      (2)
#define QCM_PASSWORD_LIMITS_PASSWORD_LIFE_TIME         (3)
#define QCM_PASSWORD_LIMITS_PASSWORD_REUSE_TIME        (4)
#define QCM_PASSWORD_LIMITS_PASSWORD_REUSE_MAX         (5)
#define QCM_PASSWORD_LIMITS_PASSWORD_LOCK_TIME         (6)
#define QCM_PASSWORD_LIMITS_PASSWORD_GRACE_TIME        (7)
#define QCM_PASSWORD_LIMITS_PASSWORD_VERIFY_FUNCTION   (8)

/* PORJ-2207 Password policy support */
#define QCM_PASSWORD_HISTORY_USER_ID                    (0)
#define QCM_PASSWORD_HISTORY_PASSWORD                   (1)
#define QCM_PASSWORD_HISTORY_PASSWORD_DATE              (2)
/* PROJ-2211 Materialized View */
#define QCM_MATERIALIZED_VIEWS_USER_ID_COL_ORDER            (0)
#define QCM_MATERIALIZED_VIEWS_MVIEW_ID_COL_ORDER           (1)
#define QCM_MATERIALIZED_VIEWS_MVIEW_NAME_COL_ORDER         (2)
#define QCM_MATERIALIZED_VIEWS_TABLE_ID_COL_ORDER           (3)
#define QCM_MATERIALIZED_VIEWS_VIEW_ID_COL_ORDER            (4)
#define QCM_MATERIALIZED_VIEWS_REFRESH_TYPE_COL_ORDER       (5)
#define QCM_MATERIALIZED_VIEWS_REFRESH_TIME_COL_ORDER       (6)
#define QCM_MATERIALIZED_VIEWS_CREATED_COL_ORDER            (7)
#define QCM_MATERIALIZED_VIEWS_LAST_DDL_TIME_COL_ORDER      (8)
#define QCM_MATERIALIZED_VIEWS_LAST_REFRESH_TIME_COL_ORDER  (9)

// PROJ-1685
#define QCM_LIBRARIES_LIBRARY_ID_ORDER                      (0)
#define QCM_LIBRARIES_USER_ID_ORDER                         (1)
#define QCM_LIBRARIES_LIBRARY_NAME_ORDER                    (2)
#define QCM_LIBRARIES_FILE_SPEC_ORDER                       (3)
#define QCM_LIBRARIES_DYNAMIC_ORDER                         (4)
#define QCM_LIBRARIES_STATUS_ORDER                          (5)

// PROJ-2223 audit
#define QCM_AUDIT_ALL_OPTS_USER_ID_COL_ORDER           (0)
#define QCM_AUDIT_ALL_OPTS_OBJECT_NAME_COL_ORDER       (1)
#define QCM_AUDIT_ALL_OPTS_SELECT_SUCCESS_COL_ORDER    (2)
#define QCM_AUDIT_ALL_OPTS_SELECT_FAILURE_COL_ORDER    (3)
#define QCM_AUDIT_ALL_OPTS_INSERT_SUCCESS_COL_ORDER    (4)
#define QCM_AUDIT_ALL_OPTS_INSERT_FAILURE_COL_ORDER    (5)
#define QCM_AUDIT_ALL_OPTS_UPDATE_SUCCESS_COL_ORDER    (6)
#define QCM_AUDIT_ALL_OPTS_UPDATE_FAILURE_COL_ORDER    (7)
#define QCM_AUDIT_ALL_OPTS_DELETE_SUCCESS_COL_ORDER    (8)
#define QCM_AUDIT_ALL_OPTS_DELETE_FAILURE_COL_ORDER    (9)
#define QCM_AUDIT_ALL_OPTS_MOVE_SUCCESS_COL_ORDER      (10)
#define QCM_AUDIT_ALL_OPTS_MOVE_FAILURE_COL_ORDER      (11)
#define QCM_AUDIT_ALL_OPTS_MERGE_SUCCESS_COL_ORDER     (12)
#define QCM_AUDIT_ALL_OPTS_MERGE_FAILURE_COL_ORDER     (13)
#define QCM_AUDIT_ALL_OPTS_ENQUEUE_SUCCESS_COL_ORDER   (14)
#define QCM_AUDIT_ALL_OPTS_ENQUEUE_FAILURE_COL_ORDER   (15)
#define QCM_AUDIT_ALL_OPTS_DEQUEUE_SUCCESS_COL_ORDER   (16)
#define QCM_AUDIT_ALL_OPTS_DEQUEUE_FAILURE_COL_ORDER   (17)
#define QCM_AUDIT_ALL_OPTS_LOCK_SUCCESS_COL_ORDER      (18)
#define QCM_AUDIT_ALL_OPTS_LOCK_FAILURE_COL_ORDER      (19)
#define QCM_AUDIT_ALL_OPTS_EXECUTE_SUCCESS_COL_ORDER   (20)
#define QCM_AUDIT_ALL_OPTS_EXECUTE_FAILURE_COL_ORDER   (21)
#define QCM_AUDIT_ALL_OPTS_COMMIT_SUCCESS_COL_ORDER    (22)
#define QCM_AUDIT_ALL_OPTS_COMMIT_FAILURE_COL_ORDER    (23)
#define QCM_AUDIT_ALL_OPTS_ROLLBACK_SUCCESS_COL_ORDER  (24)
#define QCM_AUDIT_ALL_OPTS_ROLLBACK_FAILURE_COL_ORDER  (25)
#define QCM_AUDIT_ALL_OPTS_SAVEPOINT_SUCCESS_COL_ORDER (26)
#define QCM_AUDIT_ALL_OPTS_SAVEPOINT_FAILURE_COL_ORDER (27)
#define QCM_AUDIT_ALL_OPTS_CONNECT_SUCCESS_COL_ORDER   (28)
#define QCM_AUDIT_ALL_OPTS_CONNECT_FAILURE_COL_ORDER   (29)
#define QCM_AUDIT_ALL_OPTS_DISCONNECT_SUCCESS_COL_ORDER     (30)
#define QCM_AUDIT_ALL_OPTS_DISCONNECT_FAILURE_COL_ORDER     (31)
#define QCM_AUDIT_ALL_OPTS_ALTER_SESSION_SUCCESS_COL_ORDER  (32)
#define QCM_AUDIT_ALL_OPTS_ALTER_SESSION_FAILURE_COL_ORDER  (33)
#define QCM_AUDIT_ALL_OPTS_ALTER_SYSTEM_SUCCESS_COL_ORDER   (34)
#define QCM_AUDIT_ALL_OPTS_ALTER_SYSTEM_FAILURE_COL_ORDER   (35)
#define QCM_AUDIT_ALL_OPTS_DDL_SUCCESS_COL_ORDER            (36)
#define QCM_AUDIT_ALL_OPTS_DDL_FAILURE_COL_ORDER            (37)
#define QCM_AUDIT_ALL_OPTS_CREATED_COL_ORDER                (38)
#define QCM_AUDIT_ALL_OPTS_LAST_DDL_TIME_COL_ORDER          (39)

#define QCM_AUDIT_IS_STARTED_COL_ORDER                 (0)


// PROJ-2264 Dictionary table
#define QCM_COMPRESSION_TABLES_TABLE_ID_COL_ORDER           (0)
#define QCM_COMPRESSION_TABLES_COLUMN_ID_COL_ORDER          (1)
#define QCM_COMPRESSION_TABLES_DIC_TABLE_ID_COL_ORDER       (2)
#define QCM_COMPRESSION_TABLES_MAXROWS_COL_ORDER            (3)

/* BUG-35445 SYS_CONSTRAINT_RELATED_ */
#define QCM_CONSTRAINT_RELATED_USERID_COL_ORDER             (0)
#define QCM_CONSTRAINT_RELATED_TABLEID_COL_ORDER            (1)
#define QCM_CONSTRAINT_RELATED_CONSTRAINTID_COL_ORDER       (2)
#define QCM_CONSTRAINT_RELATED_RELATEDUSERID_COL_ORDER      (3)
#define QCM_CONSTRAINT_RELATED_RELATEDPROCNAME_COL_ORDER    (4)

/* BUG-35445 SYS_INDEX_RELATED_ */
#define QCM_INDEX_RELATED_USERID_COL_ORDER                  (0)
#define QCM_INDEX_RELATED_TABLEID_COL_ORDER                 (1)
#define QCM_INDEX_RELATED_INDEXID_COL_ORDER                 (2)
#define QCM_INDEX_RELATED_RELATEDUSERID_COL_ORDER           (3)
#define QCM_INDEX_RELATED_RELATEDPROCNAME_COL_ORDER         (4)

/* PROJ-1438 Job Scheduler */
#define QCM_JOBS_ID_COL_ORDER               (0)
#define QCM_JOBS_NAME_COL_ORDER             (1)
#define QCM_JOBS_EXECQUERY_COL_ORDER        (2)
#define QCM_JOBS_START_COL_ORDER            (3)
#define QCM_JOBS_END_COL_ORDER              (4)
#define QCM_JOBS_INTERVAL_COL_ORDER         (5)
#define QCM_JOBS_INTERVALTYPE_COL_ORDER     (6)
#define QCM_JOBS_STATE_COL_ORDER            (7)
#define QCM_JOBS_LASTEXEC_COL_ORDER         (8)
#define QCM_JOBS_EXECCOUNT_COL_ORDER        (9)
#define QCM_JOBS_ERRORCODE_COL_ORDER        (10)
#define QCM_JOBS_IS_ENABLE_COL_ORDER        (11) // BUG-41713 each job enable disable
#define QCM_JOBS_COMMENT_COL_ORDER          (12) // BUG-41713 each job enable disable

/* PROJ-1812 ROLE */
#define QCM_USER_ROLES_GRANTOR_ID_COL_ORDER            (0)
#define QCM_USER_ROLES_GRANTEE_ID_COL_ORDER            (1)
#define QCM_USER_ROLES_ROLE_ID_COL_ORDER               (2)

/* PROJ-2422 srid */
#define QCM_GEOMETRIES_USER_ID_COL_ORDER             (0)
#define QCM_GEOMETRIES_TABLE_ID_COL_ORDER            (1)
#define QCM_GEOMETRIES_COLUMN_ID_COL_ORDER           (2)
#define QCM_GEOMETRIES_COORD_DIMEMSION_COL_ORDER     (3) 
#define QCM_GEOMETRIES_SRID_COL_ORDER                (4)

#define QCM_TABLE_TYPE_IS_MEMORY( flag ) (( flag & SMI_TABLE_TYPE_MASK ) \
                                          == SMI_TABLE_MEMORY ? ID_TRUE : ID_FALSE )
#define QCM_TABLE_TYPE_IS_DISK( flag ) (( flag & SMI_TABLE_TYPE_MASK ) \
                                        == SMI_TABLE_DISK ? ID_TRUE : ID_FALSE )

/* TASK-7217 Sharded sequence */
#define QCM_MAX_SHARDED_SEQUENCE_SCALE (16)

/**************************************************************
                        Extern Module CallBack
 **************************************************************/

#define QCM_MAX_EXTERN_MODULE_CNT                        (128)

typedef IDE_RC (*qcmInitGlobalHandle) (void);
typedef IDE_RC (*qcmInitMetaHandle) ( smiStatement * aSmiStmt );

typedef struct qcmExternModule
{
    qcmInitGlobalHandle mInitGlobalHandle;
    qcmInitMetaHandle   mInitMetaHandle;
} qcmExternModule;

typedef struct qcmAllExternModule
{
    UShort     mCnt;
    qcmExternModule * mExternModule[QCM_MAX_EXTERN_MODULE_CNT];
} qcmAllExternModule;

extern qcmAllExternModule gExternModule;

/**************************************************************
                        TABLE HANDLES
 **************************************************************/

extern const void * gQcmUsers;
extern const void * gQcmDNUsers;
extern const void * gQcmTables;
extern const void * gQcmDatabase;
extern const void * gQcmColumns;
extern const void * gQcmIndices;
extern const void * gQcmConstraints;
extern const void * gQcmConstraintColumns;
extern const void * gQcmIndexColumns;
extern const void * gQcmNativeProcedureGroups;

extern const void * gQcmProcedures;
extern const void * gQcmProcParas;
extern const void * gQcmProcParse;
extern const void * gQcmProcRelated;

// PROJ-1073 Package
extern const void * gQcmPkgs;
extern const void * gQcmPkgParas;
extern const void * gQcmPkgParse;
extern const void * gQcmPkgRelated;

// PROJ-1359 Tabl Handnes for Trigger
extern const void * gQcmTriggers;
extern const void * gQcmTriggerStrings;
extern const void * gQcmTriggerUpdateColumns;
extern const void * gQcmTriggerDmlTables;

// PROJ-1362 Table Handles for LOB
extern const void * gQcmLobs;

// PROJ-2002 Table Handles for Security
extern const void * gQcmSecurity;
extern const void * gQcmEncryptedColumns;

#if !defined(SMALL_FOOTPRINT)
extern const void * gQcmReplications;
extern const void * gQcmReplReceiver;
extern const void * gQcmReplHosts;
extern const void * gQcmReplItems;

// PROJ-1442 Replication Online 중 DDL 허용
extern const void * gQcmReplOldItems;
extern const void * gQcmReplOldCols;
extern const void * gQcmReplOldIdxs;
extern const void * gQcmReplOldIdxCols;

// PROJ-2642
extern const void * gQcmReplOldChecks;
extern const void * gQcmReplOldCheckColumns;

extern const void * gQcmReplRecoveryInfos;

/* PROJ-1915 */
extern const void * gQcmReplOfflineDirs;

extern const void * gQcmReplItemReplaceHistory;
#endif

extern const void * gQcmPrivileges;
extern const void * gQcmGrantSystem;
extern const void * gQcmGrantObject;
extern const void * gQcmIsamKeyDesc;
extern const void * gQcmIsamKeyPart;

extern const void * gQcmXaHeuristicTrans;
extern const void * gQcmViews;
extern const void * gQcmViewParse;
extern const void * gQcmViewRelated;

extern const void * gQcmTBSUsers;

// Proj-1076 Synonym
extern const void * gQcmSynonyms;

// PROJ-1371 Table Handles for Directory
extern const void * gQcmDirectories;

// PROJ-1502 PARTITIONED DISK TABLE
extern const void * gQcmPartTables;
extern const void * gQcmPartIndices;
extern const void * gQcmTablePartitions;
extern const void * gQcmIndexPartitions;
extern const void * gQcmPartKeyColumns;
extern const void * gQcmPartLobs;

/* BUG-21387 COMMENT */
extern const void * gQcmComments;

/* PROJ-2207 Password policy support */
extern const void * gQcmPasswordHistory;

/* PROJ-2211 Materialized View */
extern const void * gQcmMaterializedViews;

// PROJ-1685
extern const void * gQcmLibraries;

// PROJ-2223 audit
extern const void * gQcmAudit;
extern const void * gQcmAuditAllOpts;

// PROJ-2264 Dictionary table
extern const void * gQcmCompressionTables;

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
extern const void * gQcmConstraintRelated;
extern const void * gQcmIndexRelated;

/* BUG-1438 Job Scheduler */
extern const void * gQcmJobs;

/* PROJ-2422 srid */
extern const void * gQcmGeometries;

/* PROJ-1812 ROLE */
extern const void * gQcmUserRoles;

/**************************************************************
                        INDEX HANDLES
 **************************************************************/
extern const void * gQcmTablesIndex                [QCM_MAX_META_INDICES];
extern const void * gQcmColumnsIndex               [QCM_MAX_META_INDICES];
extern const void * gQcmConstraintsIndex           [QCM_MAX_META_INDICES];
extern const void * gQcmConstraintColumnsIndex     [QCM_MAX_META_INDICES];
extern const void * gQcmIndexColumnsIndex          [QCM_MAX_META_INDICES];
extern const void * gQcmUsersIndex                 [QCM_MAX_META_INDICES];
extern const void * gQcmDNUsersIndex               [QCM_MAX_META_INDICES];
extern const void * gQcmIndicesIndex               [QCM_MAX_META_INDICES];
extern const void * gQcmNativeProcedureGroupsIndex [QCM_MAX_META_INDICES];

#if !defined(SMALL_FOOTPRINT)
extern const void * gQcmReplicationsIndex          [QCM_MAX_META_INDICES];
extern const void * gQcmReplHostsIndex             [QCM_MAX_META_INDICES];
extern const void * gQcmReplItemsIndex             [QCM_MAX_META_INDICES];

// PROJ-1442 Replication Online 중 DDL 허용
extern const void * gQcmReplOldItemsIndex          [QCM_MAX_META_INDICES];
extern const void * gQcmReplOldColsIndex           [QCM_MAX_META_INDICES];
extern const void * gQcmReplOldIdxsIndex           [QCM_MAX_META_INDICES];
extern const void * gQcmReplOldIdxColsIndex        [QCM_MAX_META_INDICES];

// PROJ-2642
extern const void * gQcmReplOldChecksIndex         [QCM_MAX_META_INDICES];
extern const void * gQcmReplOldCheckColumnsIndex   [QCM_MAX_META_INDICES];

/* PROJ-1915 */
extern const void * gQcmReplOfflineDirsIndex       [QCM_MAX_META_INDICES];

extern const void * gQcmReplItemReplaceHistoryIndex       [QCM_MAX_META_INDICES];
#endif

extern const void * gQcmProceduresIndex            [QCM_MAX_META_INDICES];
extern const void * gQcmProcParasIndex             [QCM_MAX_META_INDICES];
extern const void * gQcmProcParseIndex             [QCM_MAX_META_INDICES];
extern const void * gQcmProcRelatedIndex           [QCM_MAX_META_INDICES];

// PROJ-1073 Package
extern const void * gQcmPkgsIndex                  [QCM_MAX_META_INDICES];
extern const void * gQcmPkgParasIndex              [QCM_MAX_META_INDICES];
extern const void * gQcmPkgParseIndex              [QCM_MAX_META_INDICES];
extern const void * gQcmPkgRelatedIndex            [QCM_MAX_META_INDICES];

// PROJ-1359 Index Handles for Trigger
extern const void * gQcmTriggersIndex              [QCM_MAX_META_INDICES];
extern const void * gQcmTriggerStringsIndex        [QCM_MAX_META_INDICES];
extern const void * gQcmTriggerUpdateColumnsIndex  [QCM_MAX_META_INDICES];
extern const void * gQcmTriggerDmlTablesIndex      [QCM_MAX_META_INDICES];

// PROJ-1362 Index Handles for LOB
extern const void * gQcmLobsIndex                  [QCM_MAX_META_INDICES];

// PROJ-2002 Index Handles for Security
extern const void * gQcmEncryptedColumnsIndex      [QCM_MAX_META_INDICES];

extern const void * gQcmPrivilegesIndex            [QCM_MAX_META_INDICES];
extern const void * gQcmGrantSystemIndex           [QCM_MAX_META_INDICES];
extern const void * gQcmGrantObjectIndex           [QCM_MAX_META_INDICES];

extern const void * gQcmXaHeuristicTransIndex      [QCM_MAX_META_INDICES];
extern const void * gQcmViewsIndex                 [QCM_MAX_META_INDICES];
extern const void * gQcmViewParseIndex             [QCM_MAX_META_INDICES];
extern const void * gQcmViewRelatedIndex           [QCM_MAX_META_INDICES];

extern const void * gQcmTBSUsersIndex              [QCM_MAX_META_INDICES];
// Proj-1076 Synonym
extern const void * gQcmSynonymsIndex              [QCM_MAX_META_INDICES];

// PROJ-1371 Index Handles for Directories
extern const void * gQcmDirectoriesIndex           [QCM_MAX_META_INDICES];

// PROJ-1502 PARTITIONED DISK TABLE
extern const void * gQcmPartTablesIndex            [QCM_MAX_META_INDICES];
extern const void * gQcmPartIndicesIndex           [QCM_MAX_META_INDICES];
extern const void * gQcmTablePartitionsIndex       [QCM_MAX_META_INDICES];
extern const void * gQcmIndexPartitionsIndex       [QCM_MAX_META_INDICES];
extern const void * gQcmPartKeyColumnsIndex        [QCM_MAX_META_INDICES];
extern const void * gQcmPartLobsIndex              [QCM_MAX_META_INDICES];

/* BUG-21387 COMMENT */
extern const void * gQcmCommentsIndex              [QCM_MAX_META_INDICES];

/* PROJ-2207 Password policy support */
extern const void * gQcmPasswordHistoryIndex       [QCM_MAX_META_INDICES];

/* PROJ-2211 Materialized View */
extern const void * gQcmMaterializedViewsIndex     [QCM_MAX_META_INDICES];

// PROJ-1685
extern const void * gQcmLibrariesIndex             [QCM_MAX_META_INDICES];

// PROJ-2223 audit
extern const void * gQcmAuditAllOptsIndex          [QCM_MAX_META_INDICES];

// PROJ-2264 Dictionary table
extern const void * gQcmCompressionTablesIndex     [QCM_MAX_META_INDICES];

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
extern const void * gQcmConstraintRelatedIndex     [QCM_MAX_META_INDICES];
extern const void * gQcmIndexRelatedIndex          [QCM_MAX_META_INDICES];

/* PROJ-1438 */
extern const void * gQcmJobsIndex                  [QCM_MAX_META_INDICES];

/* PROJ-2422 srid */
extern const void * gQcmGeometriesIndex            [QCM_MAX_META_INDICES];

/* PROJ-1812 ROLE */
extern const void * gQcmUserRolesIndex             [QCM_MAX_META_INDICES];

extern const SChar * gMetaTableNames[];

/* TASK-7217 Sharded sequence */
extern       SLong gShardNodeID[QCM_MAX_SHARDED_SEQUENCE_SCALE];

/**************************************************************
                  CLASS & STRUCTURE DEFINITION
 **************************************************************/

extern qcmTableInfo * gQcmUsersTempInfo;
extern qcmTableInfo * gQcmDNUsersTempInfo;

typedef struct qcmTableInfoList
{
    qcmTableInfo        * tableInfo;
    qcmRefChildInfo     * childInfo;     // BUG-15282, BUG-28049
    qcmTableInfoList    * next;
} qcmTableInfoList;

typedef struct qcmIndexInfoList
{
    UInt                  indexID;
    qcmTableInfo        * tableInfo;
    qcmRefChildInfo     * childInfo;     // BUG-15282, BUG-28049
    qcmIndexInfoList    * next;

    // PROJ-1624 global non-partitioned index
    idBool                isPartitionedIndex;
    qdIndexTableList      indexTable;
    
} qcmIndexInfoList;

typedef struct qcmSequenceInfoList
{
    qcmSequenceInfo       sequenceInfo;
    qcmSequenceInfoList * next;
} qcmSequenceInfoList;

typedef struct qcmTriggerInfoList
{
    smOID                 triggerOID;
    qcmTableInfo        * tableInfo;
    qcmTriggerInfoList  * next;
} qcmTriggerInfoList;

typedef struct qcmProcInfoList
{
    qsOID                 procOID;
    SChar               * procName;
    qcmProcInfoList     * next;
} qcmProcInfoList;

// PROJ-1685
typedef struct qcmLibraryInfo
{
    UInt      libraryID;
    UInt      userID;
    SChar     libraryName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar     fileSpec[QCM_MAX_DEFAULT_VAL + 1];
    SChar     dynamic[1];
    SChar     status[7 + 1];
} qcmLibraryInfo;

// PROJ-1073 Package
typedef struct qcmPkgInfoList
{
    qsOID                 pkgOID;
    SChar               * pkgName;
    SInt                  pkgType;   /* BUG-39340 */
    qcmPkgInfoList      * next;
} qcmPkgInfoList;

typedef IDE_RC (*qcmSetStructMemberFunc)( idvSQL        * aStatistics,
                                          const void    * aRow,
                                          void          * aOutStruct);

class qcm
{
private:
    // for server start. (qcm::init)
    static IDE_RC getQcmColumn(smiStatement  * aSmiStmt,
                               qcmTableInfo  * aTableInfo);

    // BUG-42877
    // lob column이 있으면 lob 정보를 sTableInfo->columns의 flag에 추가한다.
    static IDE_RC getQcmLobColumn(smiStatement * aSmiStmt,
                                  qcmTableInfo * aTableInfo);

    static IDE_RC getQcmIndices(smiStatement * aSmiStmt,
                                qcmTableInfo * aTableInfo);

    static IDE_RC getQcmConstraints(smiStatement * aSmiStmt,
                                    UInt           aTableID,
                                    qcmTableInfo * aTableInfo);

    static IDE_RC getQcmConstraintColumn(smiStatement  * aSmiStmt,
                                         UInt            aConstrID,
                                         UInt          * aColumns);
    /* PROJ-2639 Altibase Disk Edition */
    static IDE_RC checkDiskEdition( smiStatement * aSmiStmt );

    static IDE_RC checkMetaVersionAndUpgrade(smiStatement * aSmiStmt);

    static IDE_RC searchTableID( smiStatement * aSmiStmt,
                                 SInt           aTableID,
                                 idBool       * aExist );

    static IDE_RC searchIndexID( smiStatement * aSmiStmt,
                                 SInt           aIndexID,
                                 idBool       * aExist );

    static IDE_RC searchConstrID( smiStatement * aSmiStmt,
                                  SInt           aConstrID,
                                  idBool       * aExist );

    static IDE_RC searchJobID( smiStatement * aSmiStmt,
                               SInt           aJobID,
                               idBool       * aExist );

public:
    // for createdb & server start.
    // checks meta table exists.
    // if exists, returns IDE_SUCCESS, else IDE_FAILURE;
    static IDE_RC check(smiStatement * aSmiStmt);

    static void initializeGlobalVariables( const void * aTable );
    
    // for server start.
    // create tempInfo.
    static IDE_RC initMetaCaches(smiStatement * aSmiStmt);
    
    // initialize meta handle.
    static IDE_RC initMetaHandles(
        smiStatement * aSmiStmt,
        UInt         * aCurrMinorVersion = NULL);
    
    static IDE_RC makeAndSetQcmTableInfo(
        smiStatement * aSmiStmt,
        UInt           aTableID,
        smOID          aTableOID,
        const void   * aTableRow = NULL);

    // remove tempInfo.
    static IDE_RC finiMetaCaches(smiStatement * aSmiStmt);

    // BUG-13725
    static void setOperatableFlag( qcmTableType   aTableType,
                                   UInt         * aOperatableFlag );
    
    // Meta Table의 Tablespace ID필드로부터 데이터를 읽어온다
    static scSpaceID getSpaceID(const void * aTableRow,
                                UInt         aFieldOffset );

    // validation support.
    static IDE_RC existObject(
        qcStatement    * aStatement,
        idBool           aIsPublicObject,
        qcNamePosition   aUserName,
        qcNamePosition   aObjectName,
        qsObjectType     aObjectType,        // BUG-37293
        UInt           * aUserID,
        qsOID          * aObjectID,
        idBool         * aExist );

    static IDE_RC getTableInfo(
        qcStatement    *aStatement,
        UInt            aUserID,
        qcNamePosition  aTableName,
        qcmTableInfo  **aTableInfo, // out
        smSCN          *aSCN,
        void          **aTableHandle);

    static IDE_RC getTableInfo(
        qcStatement     *aStatement,
        UInt             aUserID,
        UChar           *aTableName,
        SInt             aTableNameSize,
        qcmTableInfo   **aTableInfo,
        smSCN           *aSCN,
        void           **aTableHandle);

    static IDE_RC getTableInfo(qcStatement     *aStatement,
                               SChar           *aUserName,
                               SChar           *aTableName,
                               qcmTableInfo   **aOutTableInfo,
                               smSCN           *aOutSCN,
                               void           **aOutTableHandle);

    static IDE_RC getTableInfoAndLock(qcStatement     *aStatement,
                                      SChar           *aUserName,
                                      SChar           *aTableName,
                                      smiTableLockMode aLockMode,
                                      ULong            aTimeout,
                                      qcmTableInfo   **aOutTableInfo,
                                      void           **aOutTableHandle);

    static IDE_RC getTableInfoByID(
        qcStatement    *aStatement,
        UInt            aTableID,
        qcmTableInfo  **aTableInfo, // out
        smSCN          *aSCN,
        void          **aTableHandle);      // out

    static IDE_RC checkSequenceInfo( qcStatement      * aStatement,
                                     UInt               aUserID,
                                     qcNamePosition     aNamePos,
                                     qcmSequenceInfo  * aSequenceInfo,       // out
                                     void            ** aSequenceHandle );   // out

    static IDE_RC getSequenceInfo( qcStatement      * aStatement,
                                   UInt               aUserID,
                                   UChar            * aSequenceName,
                                   SInt               aSequenceNameSize,
                                   qcmSequenceInfo  * aSequenceInfo,
                                   void            ** aSequenceHandle );

    static IDE_RC checkIndexByUser(
        qcStatement    *aStatement,
        qcNamePosition  aUserName,
        qcNamePosition  aIndexName,
        UInt *          aUserID,
        UInt *          aTableID,
        UInt *          aIndexID);

    static IDE_RC checkObjectByUserID(
        qcStatement *aStatement,
        UInt         aUserID,
        idBool      *aIsTrue);      // out

    static IDE_RC getTableHandleByName(
        smiStatement * aSmiStmt,
        UInt           aUserID,
        UChar        * aTableName,
        SInt           aTableNameSize,
        void        ** aTableHandle,     // out
        smSCN        * aSCN);            // out
    
    static IDE_RC getTableIDAndTypeByName( smiStatement     * aSmiStmt,
                                           UInt               aUserID,
                                           UChar            * aTableName,
                                           SInt               aTableNameSize,
                                           UInt             * aTableID,
                                           qcmTableType     * aTableType );
    
    static IDE_RC getTableHandleByID(
        smiStatement * aSmiStmt,
        UInt           aTableID,
        void        ** aTableHandle,
        smSCN        * aSCN,
        const void  ** aTableRow = NULL,
        const idBool   aTouchTable = ID_FALSE);

    // BUG-16980
    static IDE_RC getSequenceHandleByName(
        smiStatement * aSmiStmt,
        UInt           aUserID,
        UChar        * aSequenceName,
        SInt           aSequenceNameSize,
        void        ** aSequenceHandle);   // out

    // BUG-16980
    static IDE_RC getSequenceInfoByName(
        smiStatement     * aSmiStmt,
        UInt               aUserID,
        UChar            * aSequenceName,
        SInt               aSequenceNameSize,
        qcmSequenceInfo  * aSequenceInfo,     // out
        void            ** aSequenceHandle);  // out

    // for execution of ddl.
    static IDE_RC getNextTableID(
        qcStatement *aStatement,
        UInt        *aTableID);

    static IDE_RC getNextIndexID(
        qcStatement *aStatement,
        UInt        *aIndexID);

    static IDE_RC getNextConstrID(
        qcStatement *aStatement,
        UInt        *aConstrID);

    static IDE_RC getNextDirectoryID(
        qcStatement *aStatement,
        SLong       *aDirectoryID);

    // PROJ-1685
    static IDE_RC getNextLibraryID(
        qcStatement *aStatement,
        SLong       *aLibraryID);

    // PROJ-2264 Dictionary table
    static IDE_RC getNextDictionaryID(
        qcStatement *aStatement,
        UInt        *aDictionaryID);

    /* PROJ-1438 Job Scheduler */
    static IDE_RC getNextJobID(
        qcStatement *aStatement,
        UInt        *aJobID);

    static IDE_RC getMetaTable(
        const SChar  * aMetaTableName,
        const void  ** aTableHandle, // out
        smiStatement * aSmiStmt);

    static IDE_RC getMetaIndex( const void ** v_indexHandle,
                                const void  * v_tableHandle ) ;
    
    static void makeMetaRangeSingleColumn(
        qtcMetaRangeColumn  * aRangeColumn,
        const mtcColumn     * aColumnDesc,
        const void          * aValue,
        smiRange            * aRange);

    static void makeMetaRangeDoubleColumn(
        qtcMetaRangeColumn  * aFirstRangeColumn,
        qtcMetaRangeColumn  * aSecondRangeColumn,
        const mtcColumn     * aFirstColumnDesc,
        const void          * aFirstColValue,
        const mtcColumn     * aSecondColumnDesc,
        const void          * aSecondColValue,
        smiRange            * aRange);

    static void makeMetaRangeTripleColumn(
        qtcMetaRangeColumn  * aFirstRangeColumn,
        qtcMetaRangeColumn  * aSecondRangeColumn,
        qtcMetaRangeColumn  * aThirdRangeColumn,
        const mtcColumn     * aFirstColumnDesc,
        const void          * aFirstColValue,
        const mtcColumn     * aSecondColumnDesc,
        const void          * aSecondColValue,
        const mtcColumn     * aThirdColumnDesc,
        const void          * aThirdColValue,
        smiRange            * aRange);

    static void makeMetaRangeFourColumn(
        qtcMetaRangeColumn  * aFirstRangeColumn,
        qtcMetaRangeColumn  * aSecondRangeColumn,
        qtcMetaRangeColumn  * aThirdRangeColumn,
        qtcMetaRangeColumn  * aFourthRangeColumn,
        const mtcColumn     * aFirstColumnDesc,
        const void          * aFirstColValue,
        const mtcColumn     * aSecondColumnDesc,
        const void          * aSecondColValue,
        const mtcColumn     * aThirdColumnDesc,
        const void          * aThirdColValue,
        const mtcColumn     * aFourthColumnDesc,
        const void          * aFourthColValue,
        smiRange            * aRange);

    static void makeMetaRangeFiveColumn(
        qtcMetaRangeColumn  * aFirstRangeColumn,
        qtcMetaRangeColumn  * aSecondRangeColumn,
        qtcMetaRangeColumn  * aThirdRangeColumn,
        qtcMetaRangeColumn  * aFourthRangeColumn,
        qtcMetaRangeColumn  * aFifthRangeColumn,
        const mtcColumn     * aFirstColumnDesc,
        const void          * aFirstColValue,
        const mtcColumn     * aSecondColumnDesc,
        const void          * aSecondColValue,
        const mtcColumn     * aThirdColumnDesc,
        const void          * aThirdColValue,
        const mtcColumn     * aFourthColumnDesc,
        const void          * aFourthColValue,
        const mtcColumn     * aFifthColumnDesc,
        const void          * aFifthColValue,
        smiRange            * aRange);

    static IDE_RC getQcmLocalUniqueByCols( smiStatement  * aSmiStmt,
                                           qcmTableInfo  * aTableInfo,
                                           UInt            aKeyColCount,
                                           UInt          * aKeyCols,
                                           UInt          * aKeyColsFlag,
                                           qcmUnique     * aLocalUnique );

    static void destroyQcmTableInfo(qcmTableInfo *aInfo);

    static IDE_RC touchTable( smiStatement      * aSmiStmt,
                              UInt                aTableID,
                              smiTBSLockValidType aTBSLvType );

    static IDE_RC getParentKey(qcStatement    * aStatement,
                               qcmForeignKey  * aForeignKey,
                               qcmParentInfo  * aReferenceInfo);

    static IDE_RC getChildKeys(qcStatement       * aStatement,
                               qcmIndex          * aUniqueIndex,
                               qcmTableInfo      * aTableInfo,
                               qcmRefChildInfo  ** aChildInfo); // BUG-28049

    static IDE_RC getChildKeysForDelete(qcStatement       * aStatement,
                                        UInt                aReferencingUserID,
                                        qcmIndex          * aUniqueIndex,
                                        qcmTableInfo      * aTableInfo,
                                        idBool              aDropTablespace,
                                        qcmRefChildInfo  ** aChildInfo); // BUG-28049

    /**********************************/
    /* common routine for meta select */
    /**********************************/
    static IDE_RC selectRow
    (
        smiStatement            * aStmt,
        const void              * aTable,
        smiCallBack             * aCallback,
        smiRange                * aRange,
        const void              * aIndex,
        /* function that sets members in aMetaStruct from aRow */
        qcmSetStructMemberFunc    aSetStructMemberFunc,

        /* output address space.
         * if NULL, only count tuples and
         * a_metaXXXXX arguments are ignored.
         * see selectCount.
         */
        void                    * aMetaStruct,          /* OUT */
        /* distance between structures in address space*/
        UInt                      aMetaStructDistance,
        /* maximum count of structures capable for aMetaStruct */
        /* o_mataStruct should have been allocated
         * aMetaStructDistance * aMetaStructMaxCount */
        UInt                      aMetaStructMaxCount,
        vSLong                  * aSelectedRowCount      /* OUT */
    );

    // by default do full scan
    static IDE_RC selectCount
    (
        smiStatement        * aSmiStmt,
        const void          * aTable,
        vSLong              * aSelectedRowCount,  /* OUT */
        smiCallBack         * aCallback = NULL,
        smiRange            * aRange    = NULL,
        const void          * aIndex    = NULL
    );

    static void getIntegerFieldValue(
        const void * aRow,
        mtcColumn  * aColumn,
        UInt       * aValue );

    static void getIntegerFieldValue(
        const void * aRow,
        mtcColumn  * aColumn,
        SInt       * aValue );

    static void getCharFieldValue(
        const void * aRow,
        mtcColumn  * aColumn,
        SChar      * aValue );

    static void getBigintFieldValue (
        const void * aRow,
        mtcColumn  * aColumn,
        SLong      * aValue );

    static IDE_RC checkCascadeOption(
        qcStatement * aStatement,
        UInt          aUserId,
        UInt          aReferencedTableID,
        idBool      * aExist );

    static IDE_RC copyQcmColumns(
        iduMemory  *  aMemory,
        qcmColumn  *  aSource,
        qcmColumn  ** aTarget,
        UInt          aColCount );

    //fix PROJ-159
    static IDE_RC copyQcmColumns(
        iduVarMemList  * aMemory,
        qcmColumn      * aSource,
        qcmColumn     ** aTarget,
        UInt             aColCount );

    static IDE_RC getIsDefaultTBS(
        qcStatement   * aStatement,
        UInt            aTableID,
        UInt            aColumnID,
        idBool        * aIsDefaultTBS );

    // BUG-34492
    // lock table for DML validation
    static IDE_RC lockTableForDMLValidation(
        qcStatement  * aStatement,
        void         * aTableHandle,
        smSCN          aSCN);

    // lock table for DDL validation
    static IDE_RC lockTableForDDLValidation(
        qcStatement * aStatement,
        void        * aTableHandle,
        smSCN         aSCN);

    // lock table for execution (DML,DDL)
    static IDE_RC validateAndLockTable( qcStatement      * aStatement,
                                        void             * aTableHandle,
                                        smSCN              aSCN,
                                        smiTableLockMode   aLockMode );

    static IDE_RC validateAndLockAllObjects(
        qcStatement * aStatement );

    /* PROJ-2446 ONE SOURCE */
    static IDE_RC makeAndSetQcmTblInfoCallback( smiStatement * aSmiStmt,
                                                UInt           aTableID,
                                                smOID          aTableOID );

    // PROJ-2689 downgrade meta
    static IDE_RC checkMetaVersionAndDowngrade(smiStatement * aSmiStmt);

    static idBool existGlobalNonPartitionedIndex( qcmTableInfo * aTableInfo, qcmIndex ** aIndex );

    /* TASK-7217 Sharded sequence */
    static void setShardNodeID( UInt aNodeID );

    static void unsetShardNodeID( void );

    static SLong getShardNodeID( UInt aScale );
};

#endif /* _O_QCM_H_ */
