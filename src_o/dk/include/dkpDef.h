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
 * $id$
 **********************************************************************/

#ifndef _O_DKP_DEF_H_
#define _O_DKP_DEF_H_ 1


#include <idTypes.h>

#include <dkDef.h>
#include <dkaDef.h>
#include <dksDef.h>

/* Define ADLP protocol common header */
/* ---------------------------------------------------------------------
 *  dkpProtocolHeader (ADLP Common header) :
 *  ________________________________________________________________________________________
 * | OpID(1) | Param(1) | Flags(1) | Reserved(1) |   Session ID (4)   |   Body length (4)   |
 * |_________|__________|__________|_____________|____________________|_____________________|
 * --------------------------------------------------------------------*/

#define DKP_CM_HDR_LEN                          (16)

#define DKP_ADLP_HDR_OPID_LEN                   (1)
#define DKP_ADLP_HDR_PARAM_LEN                  (1)
#define DKP_ADLP_HDR_FLAGS_LEN                  (1)
#define DKP_ADLP_HDR_RESERVED_LEN               (1)
#define DKP_ADLP_HDR_SESSIONID_LEN              (4)
#define DKP_ADLP_HDR_DATA_LEN                   (4)
#define DKP_ADLP_HDR_LEN                    (DKP_ADLP_HDR_OPID_LEN      + \
                                             DKP_ADLP_HDR_PARAM_LEN     + \
                                             DKP_ADLP_HDR_FLAGS_LEN     + \
                                             DKP_ADLP_HDR_RESERVED_LEN  + \
                                             DKP_ADLP_HDR_SESSIONID_LEN + \
                                             DKP_ADLP_HDR_DATA_LEN)

#define DKP_ADLP_PACKET_DATA_MAX_LEN            (((1024)*(32)) -        \
                                                 DKP_CM_HDR_LEN -       \
                                                 DKP_ADLP_HDR_LEN)

/* Define ADLP common header 2nd element's flags for sending protocols */
#define DKP_ADLP_PACKET_PARAM_NOT_USED          (0xFF)
#define DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED    (0x00)
#define DKP_ADLP_PACKET_PARAM_FRAGMENTED        (0x01)
#define DKP_ADLP_PACKET_PARAM_FORCE             (0x02)
#define DKP_ADLP_PACKET_PARAM_NORMAL            (0x03)

/* Values of dkpProtocolHeader::mFlags (1 byte)
 * _____________________________________________________________________________
 * | 7:N/A | 6:N/A | 5:N/A | 4:N/A | 3:N/A | 2:N/A | 1: RemoteTx Begin  | 0: XA |
 * |_______|_______|_______|_______|_______|_______|____________________|_______|
 *
 *  RemoteTx Begin: 처음으로 원격 트랜잭션을 Begin 하는지 여부..
 */
#define DKP_ADLP_PACKET_FLAGS_NOT_USED            (0x00)
#define DKP_ADLP_PACKET_FLAGS_SET_XA( aFlags )    ( (aFlags) |= 0x01 )
#define DKP_ADLP_PACKET_FLAGS_UNSET_XA( aFlags )  ( (aFlags) &= (0xFE) )
#define DKP_ADLP_PACKET_FLAGS_IS_XA( aFlags )     ( ( ((aFlags) & 0x01) == 0x01 )? ID_TRUE : ID_FALSE )

#define DKP_ADLP_PACKET_FLAGS_SET_TX_BEGIN( aFlags )    ( (aFlags) |= 0x02 )
#define DKP_ADLP_PACKET_FLAGS_UNSET_TX_BEGIN( aFlags )  ( (aFlags) &= (0xFD) )
#define DKP_ADLP_PACKET_FLAGS_IS_TX_BEGIN( aFlags )     ( ( ((aFlags) & 0x02) == 0x02 )? ID_TRUE : ID_FALSE )

/* Values of dkpProtocolHeader::mReserved 확장시,
 * dkpProtocolMgr에서 인자값들을 aReserved를 검색해서 변경.
 */
#define DKP_ADLP_PACKET_RESERVED_NOT_USED          (0x00)


#define DKP_ALTILINKER_PROPERTY_CNT             DKA_CONF_MAX

#define DKP_LINKER_CONTROL_SESSION_ID       DKS_LINKER_CONTROL_SESSION_ID
#define DKP_LINKER_NOTIFY_SESSION_ID        DKS_LINKER_NOTIFY_SESSION_ID

#define DKP_PREPARE_LINKER_SHUTDOWN_TIMEOUT     (30)

#define DKP_TRANSACTION_ROLLBACK_ALL            ((UChar)(0x00))
#define DKP_TRANSACTION_ROLLBACK_TO_SAVEPOINT   ((UChar)(0x01))

/* BUG-37588 */
#define DKP_NO_ERROR_DESCRIPTION_STR            "No remote error description."

/* BUG-37850 */
#define DKP_FRAGMENTED_ROW                      (1)

/* -----------------------------------------------------------------
 * ADLP Protocol operation for New DBLink phase 1 (PROJ-1832)
 *
 *  Operation ID(8bit):
 *            _______________________________
 *           |C/D|   |   |   |   |   |   |   |
 *           |___|___|___|___|___|___|___|___|
 *
 *      1 bit   : C (Control Operation) / D (Data Operation)
 *      2~7 bit : Operation identifier
 * ----------------------------------------------------------------*/
typedef enum
{
    /* Control operations */
    DKP_CREATE_LINKER_CTRL_SESSION = 0x01,                  /* 0x01 */
    DKP_CREATE_LINKER_CTRL_SESSION_RESULT,                  /* 0x02 */
    DKP_CREATE_LINKER_DATA_SESSION,                         /* 0x03 */
    DKP_CREATE_LINKER_DATA_SESSION_RESULT,                  /* 0x04 */
    DKP_DESTROY_LINKER_DATA_SESSION,                        /* 0x05 */
    DKP_DESTROY_LINKER_DATA_SESSION_RESULT,                 /* 0x06 */
    DKP_REQUEST_LINKER_STATUS,                              /* 0x07 */
    DKP_REQUEST_LINKER_STATUS_RESULT,                       /* 0x08 */
    DKP_REQUEST_PV_INFO_DBLINK_ALTILINKER_STATUS,           /* 0x09 */
    DKP_REQUEST_PV_INFO_DBLINK_ALTILINKER_STATUS_RESULT,    /* 0x0a */
    DKP_PREPARE_LINKER_SHUTDOWN,                            /* 0x0b */
    DKP_PREPARE_LINKER_SHUTDOWN_RESULT,                     /* 0x0c */
    DKP_DO_LINKER_SHUTDOWN,                                 /* 0x0d */
    DKP_REQUEST_LINKER_DUMP,                                /* 0x0e */
    DKP_REQUEST_LINKER_DUMP_RESULT,                         /* 0x0f */

    /* Operation separator */
    DKP_UNKNOWN_OPERATION = 0x7f,                           /* 0x7f */

    /* Data operations */
    DKP_PREPARE_REMOTE_STMT = 0x81,                         /* 0x81 */
    DKP_PREPARE_REMOTE_STMT_RESULT,                         /* 0x82 */
    DKP_REQUEST_REMOTE_ERROR_INFO,                          /* 0x83 */
    DKP_REQUEST_REMOTE_ERROR_INFO_RESULT,                   /* 0x84 */
    DKP_EXECUTE_REMOTE_STMT,                                /* 0x85 */
    DKP_EXECUTE_REMOTE_STMT_RESULT,                         /* 0x86 */
    DKP_REQUEST_FREE_REMOTE_STMT,                           /* 0x87 */
    DKP_REQUEST_FREE_REMOTE_STMT_RESULT,                    /* 0x88 */
    DKP_FETCH_RESULT_SET,                                   /* 0x89 */
    DKP_FETCH_RESULT_ROW,                                   /* 0x8a */
    DKP_REQUEST_RESULT_SET_META,                            /* 0x8b */
    DKP_REQUEST_RESULT_SET_META_RESULT,                     /* 0x8c */
    DKP_BIND_REMOTE_VARIABLE,                               /* 0x8d */
    DKP_BIND_REMOTE_VARIABLE_RESULT,                        /* 0x8e */
    DKP_REQUEST_CLOSE_REMOTE_NODE_SESSION,                  /* 0x8f */
    DKP_REQUEST_CLOSE_REMOTE_NODE_SESSION_RESULT,           /* 0x90 */
    DKP_SET_AUTO_COMMIT_MODE,                               /* 0x91 */
    DKP_SET_AUTO_COMMIT_MODE_RESULT,                        /* 0x92 */
    DKP_CHECK_REMOTE_NODE_SESSION,                          /* 0x93 */
    DKP_CHECK_REMOTE_NODE_SESSION_RESULT,                   /* 0x94 */
    DKP_COMMIT,                                             /* 0x95 */
    DKP_COMMIT_RESULT,                                      /* 0x96 */
    DKP_ROLLBACK,                                           /* 0x97 */
    DKP_ROLLBACK_RESULT,                                    /* 0x98 */
    DKP_SAVEPOINT,                                          /* 0x99 */
    DKP_SAVEPOINT_RESULT,                                   /* 0x9a */
    DKP_ABORT_REMOTE_STMT,                                  /* 0x9b */
    DKP_ABORT_REMOTE_STMT_RESULT,                           /* 0x9c */
    DKP_PREPARE_REMOTE_STMT_BATCH,                          /* 0x9d */
    DKP_PREPARE_REMOTE_STMT_BATCH_RESULT,                   /* 0x9e */
    DKP_REQUEST_FREE_REMOTE_STMT_BATCH,                     /* 0x9f */
    DKP_REQUEST_FREE_REMOTE_STMT_BATCH_RESULT,              /* 0xa0 */
    DKP_EXECUTE_REMOTE_STMT_BATCH,                          /* 0xa1 */
    DKP_EXECUTE_REMOTE_STMT_BATCH_RESULT,                   /* 0xa2 */
    DKP_PREPARE_ADD_BATCH,                                  /* 0xa3 */
    DKP_PREPARE_ADD_BATCH_RESULT,                           /* 0xa4 */
    DKP_ADD_BATCH,                                          /* 0xa5 */
    DKP_ADD_BATCH_RESULT,                                   /* 0xa6 */        
    /* PROJ-2569 */
    DKP_XA_PREPARE,                                         /* 0xa7 */
    DKP_XA_PREPARE_RESULT,                                  /* 0xa8 */
    DKP_XA_COMMIT,                                          /* 0xa9 */
    DKP_XA_COMMIT_RESULT,                                   /* 0xaa */
    DKP_XA_ROLLBACK,                                        /* 0xab */
    DKP_XA_ROLLBACK_RESULT,                                 /* 0xac */
    DKP_XA_FORGET,                                          /* 0xad */
    DKP_XA_FORGET_RESULT                                    /* 0xae */
} dkpOperationID;

typedef enum
{
    DKP_RC_SUCCESS = 0x00,                                  
    DKP_RC_SUCCESS_FRAGMENTED,                              
    DKP_RC_FAILED,                                          
    DKP_RC_FAILED_REMOTE_SERVER_ERROR,                      /* 3  */
    DKP_RC_FAILED_STMT_TYPE_ERROR,
    DKP_RC_FAILED_NOT_PREPARED,
    DKP_RC_FAILED_INVALID_DATA_TYPE,
    DKP_RC_FAILED_INVALID_DB_CHARACTER_SET,
    DKP_RC_FAILED_SET_AUTO_COMMIT_MODE,
    DKP_RC_FAILED_REMOTE_SERVER_DISCONNECT,                 /* 9  */
    DKP_RC_FAILED_UNKNOWN_OPERATION,                        
    DKP_RC_FAILED_UNKNOWN_SESSION,                          
    DKP_RC_FAILED_WRONG_PACKET,                             
    DKP_RC_FAILED_OUT_OF_RANGE,                             
    DKP_RC_FAILED_UNKNOWN_REMOTE_STMT,                      
    DKP_RC_FAILED_EXIST_SESSION,                            /* 15 */
    DKP_RC_FAILED_REMOTE_NODE_SESSION_COUNT_EXCESS,         
    DKP_RC_FAILED_SET_SAVEPOINT,
    DKP_RC_TIMED_OUT,                                       
    DKP_RC_BUSY,                                            
    DKP_RC_END_OF_FETCH,                                    /* 20 */
    DKP_RC_XAEXCEPTION,

    DKP_RC_MAX                          
} dkpResultCode;

/* ---------------------------------------------------------------------
 * DK AltiLinker properties type enumeration
 *
 *  Linker control session 을 통해 AltiLinker 프로세스로 전달할 property
 *  들만 enumeration 하므로 dkaDef.h 에 enumeration 과는 차이가 존재한다. 
 * --------------------------------------------------------------------*/
typedef enum
{
    /* AltiLinker property types for ADLP protocol */
    DKP_PROPERTY_REMOTE_NODE_RECEIVE_TIMEOUT = 0,
    DKP_PROPERTY_QUERY_TIMEOUT,
    DKP_PROPERTY_NON_QUERY_TIMEOUT,
    DKP_PROPERTY_THREAD_COUNT,
    DKP_PROPERTY_THREAD_SLEEP_TIME,
    DKP_PROPERTY_REMOTE_NODE_SESSION_COUNT,
    DKP_PROPERTY_TRACE_LOGGING_LEVEL,
    DKP_PROPERTY_TRACE_LOG_FILE_SIZE,
    DKP_PROPERTY_TRACE_LOG_FILE_COUNT,
    DKP_PROPERTY_TARGETS,
    DKP_PROPERTY_MAX_COUNT,                    

    /* Target's sub-items */
    DKP_PROPERTY_TARGET_NAME = 100,
    DKP_PROPERTY_TARGET_JDBC_DRIVER,
    DKP_PROPERTY_TARGET_JDBC_DRIVER_CLASS_NAME,
    DKP_PROPERTY_TARGET_URL,
    DKP_PROPERTY_TARGET_USER,
    DKP_PROPERTY_TARGET_PASSWD,
    DKP_PROPERTY_TARGET_XADATASOURCE_CLASS_NAME,
    DKP_PROPERTY_TARGET_XADATASOURCE_URL_SETTER_NAME
} dkpLinkerProperties;

/* ---------------------------------------------------------------------
 * DK protocol column meta type enumeration
 * --------------------------------------------------------------------*/
typedef enum
{
    DKP_COL_TYPE_CHAR           = 1,
    DKP_COL_TYPE_VARCHAR        = 12,
    DKP_COL_TYPE_SMALLINT       = 5,
    DKP_COL_TYPE_TINYINT        = -6,
    DKP_COL_TYPE_DECIMAL        = 3,
    DKP_COL_TYPE_NUMERIC        = 2,
    DKP_COL_TYPE_FLOAT          = 6,
    DKP_COL_TYPE_INTEGER        = 4,
    DKP_COL_TYPE_REAL           = 7,
    DKP_COL_TYPE_DOUBLE         = 8,
    DKP_COL_TYPE_BIGINT         = -5,
    DKP_COL_TYPE_DATE           = 9,
    DKP_COL_TYPE_TIME           = 10,
    DKP_COL_TYPE_TIMESTAMP      = 11,
    DKP_COL_TYPE_BINARY         = -2,
    DKP_COL_TYPE_BLOB           = 30,
    DKP_COL_TYPE_CLOB           = 40
} dkpColumnType;

/* Define a column meta info */
typedef struct dkpColumn
{
    SInt            mType;
    SChar           mName[DK_NAME_LEN + 1];
    SInt            mScale;         /* Altibase: -44 ~ 128 */
    UInt            mPrecision;     /* Altibase: 0 ~ 38 */
    UInt            mNullable;      /* JDBC: 0, 1(nullable), 2(unknown) */
} dkpColumn;


#endif /* _O_DKP_DEF_H_ */
