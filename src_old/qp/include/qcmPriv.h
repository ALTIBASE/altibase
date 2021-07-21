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
 * $Id: qcmPriv.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCM_PRIV_H_
#define _O_QCM_PRIV_H_ 1

#include    <qc.h>
#include    <qtc.h>

#define QCM_MAX_GRANTEE_CNT     (100000)

//-------------------------------------------------------------------------
// The following definition needs for making INSERT statement in createdb
//-------------------------------------------------------------------------

#define QCM_PRIV_TYPE_OBJECT_STR     "1"
#define QCM_PRIV_TYPE_SYSTEM_STR     "2"

// SYSTEM ALL PRIVILEGES
#define QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO            (1)
#define QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_STR           "1"
#define QCM_PRIV_NAME_SYSTEM_ALL_PRIVILEGES_STR         "ALL"

// OBJECT PRIVILEGES
#define QCM_PRIV_ID_OBJECT_ALTER_NO                     (101)
#define QCM_PRIV_ID_OBJECT_ALTER_STR                    "101"
#define QCM_PRIV_NAME_OBJECT_ALTER_STR                  "ALTER"

#define QCM_PRIV_ID_OBJECT_DELETE_NO                    (102)
#define QCM_PRIV_ID_OBJECT_DELETE_STR                   "102"
#define QCM_PRIV_NAME_OBJECT_DELETE_STR                 "DELETE"

#define QCM_PRIV_ID_OBJECT_EXECUTE_NO                   (103)
#define QCM_PRIV_ID_OBJECT_EXECUTE_STR                  "103"
#define QCM_PRIV_NAME_OBJECT_EXECUTE_STR                "EXECUTE"

#define QCM_PRIV_ID_OBJECT_INDEX_NO                     (104)
#define QCM_PRIV_ID_OBJECT_INDEX_STR                    "104"
#define QCM_PRIV_NAME_OBJECT_INDEX_STR                  "INDEX"

#define QCM_PRIV_ID_OBJECT_INSERT_NO                    (105)
#define QCM_PRIV_ID_OBJECT_INSERT_STR                   "105"
#define QCM_PRIV_NAME_OBJECT_INSERT_STR                 "INSERT"

#define QCM_PRIV_ID_OBJECT_REFERENCES_NO                (106)
#define QCM_PRIV_ID_OBJECT_REFERENCES_STR               "106"
#define QCM_PRIV_NAME_OBJECT_REFERENCES_STR             "REFERENCES"

#define QCM_PRIV_ID_OBJECT_SELECT_NO                    (107)
#define QCM_PRIV_ID_OBJECT_SELECT_STR                   "107"
#define QCM_PRIV_NAME_OBJECT_SELECT_STR                 "SELECT"

#define QCM_PRIV_ID_OBJECT_UPDATE_NO                    (108)
#define QCM_PRIV_ID_OBJECT_UPDATE_STR                   "108"
#define QCM_PRIV_NAME_OBJECT_UPDATE_STR                 "UPDATE"

// PROJ-1371 DIRECTORY object privilege
#define QCM_PRIV_ID_DIRECTORY_READ_NO                   (109)
#define QCM_PRIV_ID_DIRECTORY_READ_STR                  "109"
#define QCM_PRIV_NAME_DIRECTORY_READ_STR                "READ"

#define QCM_PRIV_ID_DIRECTORY_WRITE_NO                  (110)
#define QCM_PRIV_ID_DIRECTORY_WRITE_STR                 "110"
#define QCM_PRIV_NAME_DIRECTORY_WRITE_STR               "WRITE"


// SYSTEM PRIVILEGES
#define QCM_PRIV_ID_SYSTEM_ALTER_SYSTEM_NO              (201)
#define QCM_PRIV_ID_SYSTEM_ALTER_SYSTEM_STR             "201"
#define QCM_PRIV_NAME_SYSTEM_ALTER_SYSTEM_STR           "ALTER_SYSTEM"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_INDEX_NO          (202)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_INDEX_STR         "202"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_INDEX_STR       "CREATE_ANY_INDEX"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_INDEX_NO           (203)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_INDEX_STR          "203"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_INDEX_STR        "ALTER_ANY_INDEX"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_INDEX_NO            (204)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_INDEX_STR           "204"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_INDEX_STR         "DROP_ANY_INDEX"

#define QCM_PRIV_ID_SYSTEM_CREATE_PROCEDURE_NO          (205)
#define QCM_PRIV_ID_SYSTEM_CREATE_PROCEDURE_STR         "205"
#define QCM_PRIV_NAME_SYSTEM_CREATE_PROCEDURE_STR       "CREATE_PROCEDURE"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_PROCEDURE_NO      (206)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_PROCEDURE_STR     "206"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_PROCEDURE_STR   "CREATE_ANY_PROCEDURE"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_PROCEDURE_NO       (207)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_PROCEDURE_STR      "207"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_PROCEDURE_STR    "ALTER_ANY_PROCEDURE"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_PROCEDURE_NO        (208)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_PROCEDURE_STR       "208"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_PROCEDURE_STR     "DROP_ANY_PROCEDURE"

#define QCM_PRIV_ID_SYSTEM_EXECUTE_ANY_PROCEDURE_NO     (209)
#define QCM_PRIV_ID_SYSTEM_EXECUTE_ANY_PROCEDURE_STR    "209"
#define QCM_PRIV_NAME_SYSTEM_EXECUTE_ANY_PROCEDURE_STR  "EXECUTE_ANY_PROCEDURE"

#define QCM_PRIV_ID_SYSTEM_CREATE_SEQUENCE_NO           (210)
#define QCM_PRIV_ID_SYSTEM_CREATE_SEQUENCE_STR          "210"
#define QCM_PRIV_NAME_SYSTEM_CREATE_SEQUENCE_STR        "CREATE_SEQUENCE"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_SEQUENCE_NO       (211)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_SEQUENCE_STR      "211"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_SEQUENCE_STR    "CREATE_ANY_SEQUENCE"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_SEQUENCE_NO        (212)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_SEQUENCE_STR       "212"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_SEQUENCE_STR     "ALTER_ANY_SEQUENCE"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_SEQUENCE_NO         (213)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_SEQUENCE_STR        "213"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_SEQUENCE_STR      "DROP_ANY_SEQUENCE"

#define QCM_PRIV_ID_SYSTEM_SELECT_ANY_SEQUENCE_NO       (214)
#define QCM_PRIV_ID_SYSTEM_SELECT_ANY_SEQUENCE_STR      "214"
#define QCM_PRIV_NAME_SYSTEM_SELECT_ANY_SEQUENCE_STR    "SELECT_ANY_SEQUENCE"

#define QCM_PRIV_ID_SYSTEM_CREATE_SESSION_NO            (215)
#define QCM_PRIV_ID_SYSTEM_CREATE_SESSION_STR           "215"
#define QCM_PRIV_NAME_SYSTEM_CREATE_SESSION_STR         "CREATE_SESSION"

#define QCM_PRIV_ID_SYSTEM_ALTER_SESSION_NO             (216)
#define QCM_PRIV_ID_SYSTEM_ALTER_SESSION_STR            "216"
#define QCM_PRIV_NAME_SYSTEM_ALTER_SESSION_STR          "ALTER_SESSION"

#define QCM_PRIV_ID_SYSTEM_CREATE_TABLE_NO              (217)
#define QCM_PRIV_ID_SYSTEM_CREATE_TABLE_STR             "217"
#define QCM_PRIV_NAME_SYSTEM_CREATE_TABLE_STR           "CREATE_TABLE"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_TABLE_NO          (218)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_TABLE_STR         "218"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_TABLE_STR       "CREATE_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_TABLE_NO           (219)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_TABLE_STR          "219"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_TABLE_STR        "ALTER_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_DELETE_ANY_TABLE_NO          (220)
#define QCM_PRIV_ID_SYSTEM_DELETE_ANY_TABLE_STR         "220"
#define QCM_PRIV_NAME_SYSTEM_DELETE_ANY_TABLE_STR       "DELETE_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_TABLE_NO            (221)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_TABLE_STR           "221"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_TABLE_STR         "DROP_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_INSERT_ANY_TABLE_NO          (222)
#define QCM_PRIV_ID_SYSTEM_INSERT_ANY_TABLE_STR         "222"
#define QCM_PRIV_NAME_SYSTEM_INSERT_ANY_TABLE_STR       "INSERT_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_LOCK_ANY_TABLE_NO            (223)
#define QCM_PRIV_ID_SYSTEM_LOCK_ANY_TABLE_STR           "223"
#define QCM_PRIV_NAME_SYSTEM_LOCK_ANY_TABLE_STR         "LOCK_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_SELECT_ANY_TABLE_NO          (224)
#define QCM_PRIV_ID_SYSTEM_SELECT_ANY_TABLE_STR         "224"
#define QCM_PRIV_NAME_SYSTEM_SELECT_ANY_TABLE_STR       "SELECT_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_UPDATE_ANY_TABLE_NO          (225)
#define QCM_PRIV_ID_SYSTEM_UPDATE_ANY_TABLE_STR         "225"
#define QCM_PRIV_NAME_SYSTEM_UPDATE_ANY_TABLE_STR       "UPDATE_ANY_TABLE"

#define QCM_PRIV_ID_SYSTEM_CREATE_USER_NO               (226)
#define QCM_PRIV_ID_SYSTEM_CREATE_USER_STR              "226"
#define QCM_PRIV_NAME_SYSTEM_CREATE_USER_STR            "CREATE_USER"

#define QCM_PRIV_ID_SYSTEM_ALTER_USER_NO                (227)
#define QCM_PRIV_ID_SYSTEM_ALTER_USER_STR               "227"
#define QCM_PRIV_NAME_SYSTEM_ALTER_USER_STR             "ALTER_USER"

#define QCM_PRIV_ID_SYSTEM_DROP_USER_NO                 (228)
#define QCM_PRIV_ID_SYSTEM_DROP_USER_STR                "228"
#define QCM_PRIV_NAME_SYSTEM_DROP_USER_STR              "DROP_USER"

#define QCM_PRIV_ID_SYSTEM_CREATE_VIEW_NO               (229)
#define QCM_PRIV_ID_SYSTEM_CREATE_VIEW_STR              "229"
#define QCM_PRIV_NAME_SYSTEM_CREATE_VIEW_STR            "CREATE_VIEW"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_VIEW_NO           (230)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_VIEW_STR          "230"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_VIEW_STR        "CREATE_ANY_VIEW"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_VIEW_NO             (231)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_VIEW_STR            "231"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_VIEW_STR          "DROP_ANY_VIEW"

#define QCM_PRIV_ID_SYSTEM_GRANT_ANY_PRIVILEGES_NO      (232)
#define QCM_PRIV_ID_SYSTEM_GRANT_ANY_PRIVILEGES_STR     "232"
#define QCM_PRIV_NAME_SYSTEM_GRANT_ANY_PRIVILEGES_STR   "GRANT_ANY_PRIVILEGES"

#define QCM_PRIV_ID_SYSTEM_ALTER_DATABASE_NO            (233)
#define QCM_PRIV_ID_SYSTEM_ALTER_DATABASE_STR           "233"
#define QCM_PRIV_NAME_SYSTEM_ALTER_DATABASE_STR         "ALTER_DATABASE"

#define QCM_PRIV_ID_SYSTEM_DROP_DATABASE_NO             (234)
#define QCM_PRIV_ID_SYSTEM_DROP_DATABASE_STR            "234"
#define QCM_PRIV_NAME_SYSTEM_DROP_DATABASE_STR          "DROP_DATABASE"

#define QCM_PRIV_ID_SYSTEM_CREATE_TABLESPACE_NO         (235)
#define QCM_PRIV_ID_SYSTEM_CREATE_TABLESPACE_STR        "235"
#define QCM_PRIV_NAME_SYSTEM_CREATE_TABLESPACE_STR      "CREATE_TABLESPACE"

#define QCM_PRIV_ID_SYSTEM_ALTER_TABLESPACE_NO          (236)
#define QCM_PRIV_ID_SYSTEM_ALTER_TABLESPACE_STR         "236"
#define QCM_PRIV_NAME_SYSTEM_ALTER_TABLESPACE_STR       "ALTER_TABLESPACE"

#define QCM_PRIV_ID_SYSTEM_DROP_TABLESPACE_NO           (237)
#define QCM_PRIV_ID_SYSTEM_DROP_TABLESPACE_STR          "237"
#define QCM_PRIV_NAME_SYSTEM_DROP_TABLESPACE_STR        "DROP_TABLESPACE"

#define QCM_PRIV_ID_SYSTEM_MANAGE_TABLESPACE_NO         (238)
#define QCM_PRIV_ID_SYSTEM_MANAGE_TABLESPACE_STR        "238"
#define QCM_PRIV_NAME_SYSTEM_MANAGE_TABLESPACE_STR      "MANAGE_TABLESPACE"

#define QCM_PRIV_ID_SYSTEM_UNLIMITED_TABLESPACE_NO      (239)
#define QCM_PRIV_ID_SYSTEM_UNLIMITED_TABLESPACE_STR     "239"
#define QCM_PRIV_NAME_SYSTEM_UNLIMITED_TABLESPACE_STR   "UNLIMITED_TABLESPACE"

#define QCM_PRIV_ID_SYSTEM_SYSDBA_NO                    (240)
#define QCM_PRIV_ID_SYSTEM_SYSDBA_STR                   "240"
#define QCM_PRIV_NAME_SYSTEM_SYSDBA_STR                 "SYSDBA"

// PROJ-1359 Trigger
#define QCM_PRIV_ID_SYSTEM_CREATE_TRIGGER_NO            (241)
#define QCM_PRIV_ID_SYSTEM_CREATE_TRIGGER_STR           "241"
#define QCM_PRIV_NAME_SYSTEM_CREATE_TRIGGER_STR         "CREATE_TRIGGER"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO        (242)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_STR       "242"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_TRIGGER_STR     "CREATE_ANY_TRIGGER"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_TRIGGER_NO         (243)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_TRIGGER_STR        "243"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_TRIGGER_STR      "ALTER_ANY_TRIGGER"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_TRIGGER_NO          (244)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_TRIGGER_STR         "244"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_TRIGGER_STR       "DROP_ANY_TRIGGER"

// Project - 1076 Synonym
#define QCM_PRIV_ID_SYSTEM_CREATE_SYNONYM_NO            (245)
#define QCM_PRIV_ID_SYSTEM_CREATE_SYNONYM_STR           "245"
#define QCM_PRIV_NAME_SYSTEM_CREATE_SYNONYM_STR         "CREATE_SYNONYM"

#define QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_SYNONYM_NO     (246)
#define QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_SYNONYM_STR    "246"
#define QCM_PRIV_NAME_SYSTEM_CREATE_PUBLIC_SYNONYM_STR "CREATE_PUBLIC_SYNONYM"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_SYNONYM_NO        (247)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_SYNONYM_STR       "247"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_SYNONYM_STR    "CREATE_ANY_SYNONYM"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_SYNONYM_NO          (248)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_SYNONYM_STR         "248"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_SYNONYM_STR       "DROP_ANY_SYNONYM"

#define QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_SYNONYM_NO       (249)
#define QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_SYNONYM_STR      "249"
#define QCM_PRIV_NAME_SYSTEM_DROP_PUBLIC_SYNONYM_STR    "DROP_PUBLIC_SYNONYM"

// PROJ-1371 DIRECTORY system privilege

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_DIRECTORY_NO      (250)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_DIRECTORY_STR     "250"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_DIRECTORY_STR   "CREATE_ANY_DIRECTORY"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_DIRECTORY_NO        (251)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_DIRECTORY_STR       "251"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_DIRECTORY_STR     "DROP_ANY_DIRECTORY"

/* PROJ-2211 Materialized View */
#define QCM_PRIV_ID_SYSTEM_CREATE_MATERIALIZED_VIEW_NO        (252)
#define QCM_PRIV_ID_SYSTEM_CREATE_MATERIALIZED_VIEW_STR       "252"
#define QCM_PRIV_NAME_SYSTEM_CREATE_MATERIALIZED_VIEW_STR     "CREATE_MATERIALIZED_VIEW"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_NO    (253)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_STR   "253"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_STR "CREATE_ANY_MATERIALIZED_VIEW"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_NO     (254)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_STR    "254"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_STR  "ALTER_ANY_MATERIALIZED_VIEW"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_NO      (255)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_STR     "255"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_STR   "DROP_ANY_MATERIALIZED_VIEW"

// PROJ-1685
#define QCM_PRIV_ID_SYSTEM_CREATE_LIBRARY_NO            (256)
#define QCM_PRIV_ID_SYSTEM_CREATE_LIBRARY_STR           "256"
#define QCM_PRIV_NAME_SYSTEM_CREATE_LIBRARY_STR         "CREATE_LIBRARY"

#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_LIBRARY_NO        (257)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_LIBRARY_STR       "257"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_LIBRARY_STR     "CREATE_ANY_LIBRARY"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_LIBRARY_NO          (258)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_LIBRARY_STR         "258"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_LIBRARY_STR       "DROP_ANY_LIBRARY"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_LIBRARY_NO         (259)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_LIBRARY_STR        "259"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_LIBRARY_STR      "ALTER_ANY_LIBRARY"

/*
 * PROJ-1832 New database links
 */

#define QCM_PRIV_ID_SYSTEM_CREATE_DATABASE_LINK_NO      (260)
#define QCM_PRIV_ID_SYSTEM_CREATE_DATABASE_LINK_STR     "260"
#define QCM_PRIV_NAME_SYSTEM_CREATE_DATABASE_LINK_STR   "CREATE_DATABASE_LINK"

#define QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_DATABASE_LINK_NO      (261)
#define QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_DATABASE_LINK_STR     "261"
#define QCM_PRIV_NAME_SYSTEM_CREATE_PUBLIC_DATABASE_LINK_STR   "CREATE_PUBLIC_DATABASE_LINK"

#define QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_DATABASE_LINK_NO      (262)
#define QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_DATABASE_LINK_STR     "262"
#define QCM_PRIV_NAME_SYSTEM_DROP_PUBLIC_DATABASE_LINK_STR   "DROP_PUBLIC_DATABASE_LINK"

/* PROJ-1812 ROLE */
#define QCM_PRIV_ID_SYSTEM_CREATE_ROLE_NO      (263)
#define QCM_PRIV_ID_SYSTEM_CREATE_ROLE_STR     "263"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ROLE_STR   "CREATE_ROLE"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_ROLE_NO      (264)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_ROLE_STR     "264"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_ROLE_STR   "DROP_ANY_ROLE"

#define QCM_PRIV_ID_SYSTEM_GRANT_ANY_ROLE_NO      (265)
#define QCM_PRIV_ID_SYSTEM_GRANT_ANY_ROLE_STR     "265"
#define QCM_PRIV_NAME_SYSTEM_GRANT_ANY_ROLE_STR   "GRANT_ANY_ROLE"

/* BUG-41408 normal user create, alter, drop job */
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_JOB_NO      (266)
#define QCM_PRIV_ID_SYSTEM_CREATE_ANY_JOB_STR     "266"
#define QCM_PRIV_NAME_SYSTEM_CREATE_ANY_JOB_STR   "CREATE_ANY_JOB"

#define QCM_PRIV_ID_SYSTEM_DROP_ANY_JOB_NO      (267)
#define QCM_PRIV_ID_SYSTEM_DROP_ANY_JOB_STR     "267"
#define QCM_PRIV_NAME_SYSTEM_DROP_ANY_JOB_STR   "DROP_ANY_JOB"

#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_JOB_NO      (268)
#define QCM_PRIV_ID_SYSTEM_ALTER_ANY_JOB_STR     "268"
#define QCM_PRIV_NAME_SYSTEM_ALTER_ANY_JOB_STR   "ALTER_ANY_JOB"
//-------------------------------------------------------------------------

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_ALTER_MASK             (0x00010000)
# define QCM_OBJECT_PRIV_ALTER_ON               (0x00010000)
# define QCM_OBJECT_PRIV_ALTER_OFF              (0x00000000)

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_DELETE_MASK            (0x00020000)
# define QCM_OBJECT_PRIV_DELETE_ON              (0x00020000)
# define QCM_OBJECT_PRIV_DELETE_OFF             (0x00000000)

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_EXECUTE_MASK           (0x00040000)
# define QCM_OBJECT_PRIV_EXECUTE_ON             (0x00040000)
# define QCM_OBJECT_PRIV_EXECUTE_OFF            (0x00000000)

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_INDEX_MASK             (0x00080000)
# define QCM_OBJECT_PRIV_INDEX_ON               (0x00080000)
# define QCM_OBJECT_PRIV_INDEX_OFF              (0x00000000)

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_INSERT_MASK            (0x00100000)
# define QCM_OBJECT_PRIV_INSERT_ON              (0x00100000)
# define QCM_OBJECT_PRIV_INSERT_OFF             (0x00000000)

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_REFERENCES_MASK        (0x00200000)
# define QCM_OBJECT_PRIV_REFERENCES_ON          (0x00200000)
# define QCM_OBJECT_PRIV_REFERENCES_OFF         (0x00000000)

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_SELECT_MASK            (0x00400000)
# define QCM_OBJECT_PRIV_SELECT_ON              (0x00400000)
# define QCM_OBJECT_PRIV_SELECT_OFF             (0x00000000)

/* qcmPrivilege->privilegeBitMap                          */
# define QCM_OBJECT_PRIV_UPDATE_MASK            (0x00800000)
# define QCM_OBJECT_PRIV_UPDATE_ON              (0x00800000)
# define QCM_OBJECT_PRIV_UPDATE_OFF             (0x00000000)

//-------------------------------------------------------------------------

# define QCM_WITH_GRANT_OPTION_FALSE           (0)
# define QCM_WITH_GRANT_OPTION_TRUE            (1)

typedef smOID qdpObjID;     // object ID ( tableID or procOID )

typedef struct qcmGrantSystem
{
    UInt              grantorID;
    UInt              granteeID;
    UInt              privID;

    qcmGrantSystem  * next;
} qcmGrantSystem;

typedef struct qcmGrantObject
{
    UInt              grantorID;
    UInt              granteeID;
    UInt              privID;
    UInt              userID;
    qdpObjID          objID;    // if object is table, then objID is INTEGER
                                // if object is PSM, then objID is BIGINT
    SChar             objType;
    UInt              withGrantOption;
    // QCM_WITH_GRANT_OPTION_FALSE or QCM_WITH_GRANT_OPTION_TRUE

    qcmGrantObject  * next;
} qcmGrantObject;

class qcmPriv
{
public:
    static IDE_RC getQcmPrivileges(
        smiStatement  * aSmiStmt,
        qdpObjID        aTableID,
        qcmTableInfo  * aTableInfo);

    static IDE_RC getGranteeOfPSM(
        smiStatement  * aSmiStmt,
        qdpObjID        aPSMID,
        UInt          * aPrivilegeCount,
        UInt         ** aGranteeIDs);

    // PROJ-1073 Package
    static IDE_RC getGranteeOfPkg(
        smiStatement  * aSmiStmt,
        qdpObjID        aPkgID,
        UInt          * aPrivilegeCount,
        UInt         ** aGranteeIDs);

    static IDE_RC checkSystemPrivWithGrantor(
        qcStatement     * aStatement,
        UInt              aGrantorID,
        UInt              aGranteeID,
        UInt              aPrivID,
        idBool          * aExist);

    static IDE_RC checkSystemPrivWithoutGrantor(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        UInt              aPrivID,
        idBool          * aExist);

    static IDE_RC checkObjectPrivWithGrantOption(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        UInt              aPrivID,
        qdpObjID          aObjID,
        SChar           * aObjType,
        UInt              aWithGrantOption,
        idBool          * aExist);

    // PROJ-1371 Directories
    static IDE_RC checkObjectPriv(
        qcStatement     * aStatement,
        UInt              aGranteeID,
        UInt              aPrivID,
        qdpObjID          aObjID,
        SChar           * aObjType,
        idBool          * aExist);

    static IDE_RC getGrantObjectWithGrantee(
        qcStatement     * aStatement,
        iduVarMemList   * aMemory, //fix PROJ-1596
        UInt              aGrantorID,
        UInt              aGranteeID,
        UInt              aPrivID,
        qdpObjID          aObjID,
        SChar           * aObjType,
        qcmGrantObject ** aGrantObject);

    static IDE_RC getGrantObjectWithoutGrantee(
        qcStatement     * aStatement,
        iduMemory       * aMemory,
        UInt              aGrantorID,
        UInt              aPrivID,
        qdpObjID          aObjID,
        SChar           * aObjType,
        qcmGrantObject ** aGrantObject);

    static IDE_RC getGrantObjectWithoutPrivilege(
        qcStatement     * aStatement,
        iduVarMemList   * aMemory, //fix PROJ-1596
        UInt              aGrantorID,
        UInt              aGranteeID,
        qdpObjID          aObjID,
        SChar           * aObjType,
        qcmGrantObject ** aGrantObject);

    static IDE_RC setQcmGrantObject(
        void            * aRow,
        qcmGrantObject  * aGrantObject);

    static IDE_RC searchPrivilegeInfo(
        qcmTableInfo  * aTableInfo,
        UInt            aGranteeID,
        qcmPrivilege ** aQcmPriv);

    static IDE_RC checkPrivilegeInfo(
        UInt                   aPrivilegeCount,
        struct qcmPrivilege  * aPrivilegeInfo,
        UInt                   aGranteeID,
        UInt                   aPrivilegeID,
        qcmPrivilege        ** aQcmPriv);

    static IDE_RC setPrivilegeBitMap(
        UInt            aPrivID,
        qcmPrivilege  * aQcmPriv);

    static IDE_RC updateLastDDLTime(
        qcStatement   * aStatement,
        SChar         * aObjectType,
        qdpObjID        aObjID );    

    /* PROJ-1812 ROLE */
    static IDE_RC checkRoleWithGrantor(
        qcStatement     * aStatement,
        UInt              aGrantorID,
        UInt              aGranteeID,
        UInt              aRoleID,
        idBool          * aExist );
    
private:
    static IDE_RC makeQcmGrantObject(
        iduMemory       * aMemory,
        void            * aRow,
        qcmGrantObject ** aGrantObject);

    //fix PROJ-1596
    static IDE_RC makeQcmGrantObject(
        iduVarMemList   * aMemory,
        void            * aRow,
        qcmGrantObject ** aGrantObject);
};

#endif /* _O_QCM_PRIV_H_ */
