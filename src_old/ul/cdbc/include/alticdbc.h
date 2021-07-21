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

#ifndef _ALTIBASE_CDBC_H_
#define _ALTIBASE_CDBC_H_ 1

#if defined (__cplusplus)
extern "C" {
#endif



#define ALTIBASE_NTS                        -3

#define ALTIBASE_MAX_ATTR_LEN               512
#define ALTIBASE_MAX_HOST_LEN               ALTIBASE_MAX_ATTR_LEN
#define ALTIBASE_MAX_APP_INFO_LEN           128
#define ALTIBASE_MAX_NLS_USE_LEN            20
#define ALTIBASE_MAX_DATE_FORMAT_LEN        63
#define ALTIBASE_MAX_IPC_FILEPATH_LEN       ALTIBASE_MAX_ATTR_LEN

#define ALTIBASE_ATOMIC_ARRAY_ON            1
#define ALTIBASE_ATOMIC_ARRAY_OFF           0

#define ALTIBASE_AUTOCOMMIT_ON              1
#define ALTIBASE_AUTOCOMMIT_OFF             0
#define ALTIBASE_AUTOCOMMIT_DEFAULT         ALTIBASE_AUTOCOMMIT_ON

#define ALTIBASE_ALL_USERS                  "%"
#define ALTIBASE_ALL_TABLES                 "%"
#define ALTIBASE_ALL_COLUMNS                "%"
#define ALTIBASE_ALL_TABLE_TYPES            "%"

#ifndef ALTIBASE_FAILOVER_SUCCESS
#define ALTIBASE_FAILOVER_SUCCESS           0x51190
#endif
#define ALTIBASE_SUCCESS                    0
#define ALTIBASE_SUCCESS_WITH_INFO          1
#define ALTIBASE_NEED_DATA                  99
#define ALTIBASE_NO_DATA                    100
#define ALTIBASE_ERROR                      (-1)
#define ALTIBASE_INVALID_HANDLE             (-2)
#define ALTIBASE_NULL_DATA                  (-1)
#define ALTIBASE_DATA_AT_EXEC               (-2)

#define ALTIBASE_SUCCEEDED(rc)              ( ((rc) & (~1)) == 0 )
#define ALTIBASE_NOT_SUCCEEDED(rc)          ( ! ALTIBASE_SUCCEEDED(rc) )

#define ALTIBASE_NO_DATA_FOUND              ALTIBASE_NO_DATA

/* array fetch status */
#define ALTIBASE_ROW_SUCCESS                0
#define ALTIBASE_ROW_SUCCESS_WITH_INFO      6
#define ALTIBASE_ROW_DELETED                1
#define ALTIBASE_ROW_UPDATED                2
#define ALTIBASE_ROW_NOROW                  3
#define ALTIBASE_ROW_ADDED                  4
#define ALTIBASE_ROW_ERROR                  5
#define ALTIBASE_ROW_PROCEED                0
#define ALTIBASE_ROW_IGNORE                 1

#define ALTIBASE_ROW_SUCCEEDED(s)           ((s) == ALTIBASE_ROW_SUCCESS || (s) == (ALTIBASE_ROW_SUCCESS_WITH_INFO)

/* array binding status */
#define ALTIBASE_PARAM_SUCCESS              0
#define ALTIBASE_PARAM_SUCCESS_WITH_INFO    6
#define ALTIBASE_PARAM_ERROR                5
#define ALTIBASE_PARAM_UNUSED               7
#define ALTIBASE_PARAM_DIAG_UNAVAILABLE     1
#define ALTIBASE_PARAM_PROCEED              0
#define ALTIBASE_PARAM_IGNORE               1

#define ALTIBASE_PARAM_SUCCEEDED(s)         ((s) == ALTIBASE_PARAM_SUCCESS || (s) == (ALTIBASE_PARAM_SUCCESS_WITH_INFO)

#define ALTIBASE_FETCH_CONT                 -1

#define ALTIBASE_INVALID_FIELDCOUNT         -1
#define ALTIBASE_INVALID_PARAMCOUNT         -1
#define ALTIBASE_INVALID_AFFECTEDROW        -1
#define ALTIBASE_INVALID_ROWCOUNT           -1
#define ALTIBASE_INVALID_PROCESSED          -1
#define ALTIBASE_INVALID_FETCHED            -1

#define ALTIBASE_INVALID_VERSION            -1
#define ALTIBASE_MAJOR_VER_UNIT             100000000
#define ALTIBASE_MINOR_VER_UNIT             1000000
#define ALTIBASE_TERM_VER_UNIT              10000
#define ALTIBASE_PATCHSET_LEVEL_UNIT        100
#define ALTIBASE_PATCH_LEVEL_UNIT           1
#define GET_MAJOR_VERSION(v)                (  (v) / ALTIBASE_MAJOR_VER_UNIT )
#define GET_MINOR_VERSION(v)                ( ((v) % ALTIBASE_MAJOR_VER_UNIT) / ALTIBASE_MINOR_VER_UNIT )
#define GET_TERM_VERSION(v)                 ( ((v) % ALTIBASE_MINOR_VER_UNIT) / ALTIBASE_TERM_VER_UNIT )
#define GET_DEV_VERSION(v)                  GET_TERM_VERSION(v)
#define GET_PATCHSET_VERSION(v)             ( ((v) % ALTIBASE_TERM_VER_UNIT) / ALTIBASE_PATCHSET_LEVEL_UNIT )
#define GET_PATCH_VERSION(v)                ( ((v) % ALTIBASE_PATCHSET_LEVEL_UNIT) / ALTIBASE_PATCH_LEVEL_UNIT)

#define ALTIBASE_SEEK_SET                   0
#define ALTIBASE_SEEK_CUR                   1
#define ALTIBASE_SEEK_END                   2



typedef int ALTIBASE_RC;

#if USE_ENUM_BOOL
typedef enum enum_altibase_bool
{
    ALTIBASE_FALSE  = 0,
    ALTIBASE_TRUE   = 1
} ALTIBASE_BOOL;
#else
typedef int ALTIBASE_BOOL;
#define ALTIBASE_FALSE 0
#define ALTIBASE_TRUE  1
#endif

typedef enum enum_altibase_field_types
{
    ALTIBASE_TYPE_UNKNOWN   = 0,
    ALTIBASE_TYPE_CHAR      = 100,
    ALTIBASE_TYPE_VARCHAR   = 101,
    ALTIBASE_TYPE_NCHAR     = 110,
    ALTIBASE_TYPE_NVARCHAR  = 111,
    ALTIBASE_TYPE_SMALLINT  = 201,
    ALTIBASE_TYPE_INTEGER   = 202,
    ALTIBASE_TYPE_BIGINT    = 203,
    ALTIBASE_TYPE_REAL      = 204,
    ALTIBASE_TYPE_DOUBLE    = 205,
    ALTIBASE_TYPE_NUMERIC   = 211,
    ALTIBASE_TYPE_FLOAT     = 212,
    ALTIBASE_TYPE_DATE      = 300,
    ALTIBASE_TYPE_BLOB      = 401,
    ALTIBASE_TYPE_CLOB      = 402,
    ALTIBASE_TYPE_BYTE      = 500,
    ALTIBASE_TYPE_NIBBLE    = 510,
    ALTIBASE_TYPE_BIT       = 520,
    ALTIBASE_TYPE_VARBIT    = 521,
    ALTIBASE_TYPE_GEOMETRY  = 600
} ALTIBASE_FIELD_TYPE;

#define IS_IN_RANGE(v,min,max)  ( ((min) <= (v)) && ((v) <= (max)) )
#define NOT_IN_RANGE(v,min,max) ( ! IS_IN_RANGE(v,min,max) )
#define IS_STR_TYPE(t)          ( IS_IN_RANGE(t, ALTIBASE_TYPE_CHAR, ALTIBASE_TYPE_NVARCHAR) || ((t) == ALTIBASE_TYPE_CLOB) )
#define IS_PHYNUM_TYPE(t)       IS_IN_RANGE(t, ALTIBASE_TYPE_SMALLINT, ALTIBASE_TYPE_DOUBLE)
#define IS_NUMERIC_TYPE(t)      IS_IN_RANGE(t, ALTIBASE_TYPE_FLOAT, ALTIBASE_TYPE_NUMERIC)
#define IS_NUM_TYPE(t)          ( IS_PNUM_TYPE(t) || IS_NUMERIC_TYPE(t) )
#define IS_BIN_TYPE(t)          ( ((t) == ALTIBASE_TYPE_BLOB) || IS_IN_RANGE(t, ALTIBASE_TYPE_BYTE, ALTIBASE_TYPE_GEOMETRY) )
#define IS_LOB_TYPE(t)          ( ((t) == ALTIBASE_TYPE_BLOB) || ((t) == ALTIBASE_TYPE_CLOB) )
#define IS_LONG_TYPE(t)         ( IS_LOB_TYPE(t) || ((t) == ALTIBASE_TYPE_GEOMETRY) )
#define IS_BIT_TYPE(t)          IS_IN_RANGE(t, ALTIBASE_TYPE_BIT, ALTIBASE_TYPE_VARBIT)

typedef enum enum_altibase_bind_type
{
    ALTIBASE_BIND_NULL          =  0,
    ALTIBASE_BIND_BINARY        = -2,
    ALTIBASE_BIND_STRING        =  1,
    ALTIBASE_BIND_WSTRING       = -8,
    ALTIBASE_BIND_SMALLINT      =  5,
    ALTIBASE_BIND_INTEGER       =  4,
    ALTIBASE_BIND_BIGINT        = (-5) + (-20),
    ALTIBASE_BIND_REAL          =  7,
    ALTIBASE_BIND_DOUBLE        =  8,
    ALTIBASE_BIND_NUMERIC       =  2,
    ALTIBASE_BIND_DATE          =  93
} ALTIBASE_BIND_TYPE;

#define IS_STR_BIND(type) ( ((type) == ALTIBASE_BIND_STRING) || ((type) == ALTIBASE_BIND_WSTRING) )
#define IS_VAR_BIND(type) ( ((type) == ALTIBASE_BIND_BINARY) || IS_STR_BIND(type) )

typedef enum enum_altibase_option
{
    ALTIBASE_AUTOCOMMIT                 =  102,
    ALTIBASE_TXN_ISOLATION              =  108,
    ALTIBASE_CONNECTION_TIMEOUT         =  113,
    ALTIBASE_DATE_FORMAT                = 2003,
    ALTIBASE_NLS_USE                    = 2005,
    ALTIBASE_PORT                       = 2007,
    ALTIBASE_APP_INFO                   = 2008,
    ALTIBASE_NLS_NCHAR_LITERAL_REPLACE  = 2028,
    ALTIBASE_IPC_FILEPATH               = 2032,
    ALTIBASE_CLIENT_IP                  = 2064
} ALTIBASE_OPTION;

typedef enum enum_altibase_stmt_attr_type
{
    ALTIBASE_STMT_ATTR_ATOMIC_ARRAY     = 2027
} ALTIBASE_STMT_ATTR_TYPE;



#ifndef SIZEOF_LONG
    # if defined(__alpha) || defined(__sparcv9) || defined(__amd64) || defined(__LP64__) || defined(_LP64) || defined(__64BIT__)
        # define SIZEOF_LONG        8
    #else
        # define SIZEOF_LONG        4
    #endif
#endif

#if (SIZEOF_LONG == 8) && !defined(BUILD_REAL_64_BIT_MODE)
    typedef int  ALTIBASE_LONG;
#else
    typedef long ALTIBASE_LONG;
#endif



typedef void *          ALTIBASE_HANDLE;
typedef ALTIBASE_HANDLE ALTIBASE;
typedef ALTIBASE_HANDLE ALTIBASE_STMT;
typedef ALTIBASE_HANDLE ALTIBASE_RES;

typedef char *          ALTIBASE_COL;
typedef ALTIBASE_COL*   ALTIBASE_ROW;

#define ALTIBASE_MAX_NAME_LEN           40
#define ALTIBASE_MAX_FIELD_NAME_LEN     ALTIBASE_MAX_NAME_LEN
#define ALTIBASE_MAX_TABLE_NAME_LEN     ALTIBASE_MAX_NAME_LEN

typedef struct st_altibase_field
{
    ALTIBASE_FIELD_TYPE type;
    char                name[ALTIBASE_MAX_FIELD_NAME_LEN + 1];
    int                 name_length;
    char                org_name[ALTIBASE_MAX_FIELD_NAME_LEN + 1];
    int                 org_name_length;
    char                table[ALTIBASE_MAX_TABLE_NAME_LEN + 1];
    int                 table_length;
    char                org_table[ALTIBASE_MAX_TABLE_NAME_LEN + 1];
    int                 org_table_length;
    int                 size;
    int                 scale;
} ALTIBASE_FIELD;

typedef struct st_altibase_bind
{
    ALTIBASE_BIND_TYPE  buffer_type;
    void               *buffer;
    ALTIBASE_LONG       buffer_length;
    ALTIBASE_LONG      *length;
    ALTIBASE_BOOL      *is_null;
    int                 error;
} ALTIBASE_BIND;

typedef struct st_altibase_charset_info
{
    unsigned int        id;
    void               *name;
    int                 name_length;
    int                 mbmaxlen;
} ALTIBASE_CHARSET_INFO;

/* same with SQL_TIMESTAMP_STRUCT */
typedef struct st_altibase_timestamp
{
    short               year;
    unsigned short      month;
    unsigned short      day;
    unsigned short      hour;
    unsigned short      minute;
    unsigned short      second;
    int                 fraction;
} ALTIBASE_TIMESTAMP;

#define ALTIBASE_MAX_NUMERIC_LEN 16 /* SQL_MAX_NUMERIC_LEN */

/* same with SQL_NUMERIC_STRUCT */
typedef struct st_altibase_numeric
{
    unsigned char       precision;
    unsigned char       scale;
    char                sign;
    unsigned char       val[ALTIBASE_MAX_NUMERIC_LEN];
} ALTIBASE_NUMERIC;

typedef enum enum_altibase_failover_event
{
    ALTIBASE_FO_BEGIN = 0,
    ALTIBASE_FO_END   = 1,
    ALTIBASE_FO_ABORT = 2,
    ALTIBASE_FO_GO    = 3,
    ALTIBASE_FO_QUIT  = 4
} ALTIBASE_FAILOVER_EVENT;

typedef ALTIBASE_FAILOVER_EVENT (*altibase_failover_callback_func)
(
    ALTIBASE                 altibase,
    void                    *app_context,
    ALTIBASE_FAILOVER_EVENT  event
);



#define GET_BYTE(vp, idx) \
	( ((unsigned char *)(vp))[idx] )

#define GET_NIBBLE_LENGTH(vp) \
	GET_BYTE(vp, 0)

#define GET_NIBBLE_VALUE(vp, idx) ( \
    ((idx) < 0 || GET_NIBBLE_LENGTH(vp) <= (idx)) \
     ? (-1) \
     : (0xF & ( GET_BYTE(vp, 1 + (idx)/2) >> (((idx) % 2) == 0 ? 4 : 0) ) ) \
)

#define GET_BIT_LENGTH(vp) \
    ( *((int*)(vp)) )

#define GET_BIT_VALUE(vp, idx) ( \
    ((idx) < 0 || GET_BIT_LENGTH(vp) <= (idx)) \
    ? (-1) \
    : ( 0x01 & ( GET_BYTE(vp, 4 + (idx)/8) >> (7 - ((idx) % 8)) ) ) \
)



ALTIBASE_LONG altibase_affected_rows ( ALTIBASE altibase );
const char * altibase_get_charset ( ALTIBASE altibase );
ALTIBASE_RC altibase_close ( ALTIBASE altibase );
ALTIBASE_RC altibase_commit ( ALTIBASE altibase );
ALTIBASE_RC altibase_connect ( ALTIBASE    altibase,
                               const char *connstr );
ALTIBASE_RC altibase_data_seek ( ALTIBASE_RES  result,
                                 ALTIBASE_LONG offset );
unsigned int altibase_errno ( ALTIBASE altibase );
const char * altibase_error ( ALTIBASE altibase );
ALTIBASE_FIELD * altibase_field ( ALTIBASE_RES result,
                                  int          fieldnr );
ALTIBASE_FIELD * altibase_fields ( ALTIBASE_RES result );
ALTIBASE_LONG * altibase_fetch_lengths ( ALTIBASE_RES result );
ALTIBASE_ROW altibase_fetch_row ( ALTIBASE_RES result );
int altibase_field_count ( ALTIBASE altibase );
ALTIBASE_RC altibase_free_result ( ALTIBASE_RES result );
ALTIBASE_RC altibase_get_charset_info ( ALTIBASE               altibase,
                                        ALTIBASE_CHARSET_INFO *cs );
const char * altibase_client_verstr (void);
int altibase_client_version (void);
const char * altibase_host_info ( ALTIBASE altibase );
const char * altibase_proto_verstr (ALTIBASE aABConn);
int altibase_proto_version (ALTIBASE aABConn);
const char * altibase_server_verstr ( ALTIBASE altibase );
int altibase_server_version ( ALTIBASE altibase );
ALTIBASE altibase_init (void);
ALTIBASE_RES altibase_list_fields ( ALTIBASE    altibase,
                                    const char *restrictions[] );
ALTIBASE_RES altibase_list_tables ( ALTIBASE    altibase,
                                    const char *restrictions[] );
ALTIBASE_RC altibase_next_result ( ALTIBASE result );
ALTIBASE_RC altibase_stmt_next_result ( ALTIBASE_STMT result );
int altibase_num_fields ( ALTIBASE_RES result );
ALTIBASE_LONG altibase_num_rows ( ALTIBASE_RES result );
ALTIBASE_RC altibase_set_option ( ALTIBASE         altibase,
                                  ALTIBASE_OPTION  option,
                                  const void      *arg );
ALTIBASE_RC altibase_query ( ALTIBASE    altibase,
                             const char *qstr );
ALTIBASE_RC altibase_rollback ( ALTIBASE altibase );
ALTIBASE_RC altibase_set_autocommit ( ALTIBASE altibase,
                                      int      mode );
ALTIBASE_RC altibase_set_charset ( ALTIBASE    altibase,
                                   const char *csname );
ALTIBASE_RC altibase_set_failover_callback ( ALTIBASE                         altibase,
                                             altibase_failover_callback_func  callback,
                                             void                            *app_context );
const char * altibase_sqlstate ( ALTIBASE altibase );
ALTIBASE_RES altibase_store_result ( ALTIBASE altibase );
ALTIBASE_RES altibase_use_result ( ALTIBASE altibase );
ALTIBASE_LONG altibase_stmt_affected_rows ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_get_attr ( ALTIBASE_STMT            stmt,
                                     ALTIBASE_STMT_ATTR_TYPE  option,
                                     void                    *arg );
ALTIBASE_RC altibase_stmt_set_attr ( ALTIBASE_STMT            stmt,
                                     ALTIBASE_STMT_ATTR_TYPE  option,
                                     const void              *arg );
ALTIBASE_RC altibase_stmt_bind_param ( ALTIBASE_STMT  stmt,
                                       ALTIBASE_BIND *bind );
ALTIBASE_RC altibase_stmt_bind_result ( ALTIBASE_STMT  stmt,
                                        ALTIBASE_BIND *bind );
ALTIBASE_RC altibase_stmt_close ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_data_seek ( ALTIBASE_STMT stmt,
                                      ALTIBASE_LONG offset );
unsigned int altibase_stmt_errno ( ALTIBASE_STMT stmt );
const char * altibase_stmt_error ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_execute ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_fetch ( ALTIBASE_STMT stmt );
ALTIBASE_LONG altibase_stmt_fetched ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_fetch_column ( ALTIBASE_STMT  stmt,
                                         ALTIBASE_BIND *bind,
                                         int            column,
                                         ALTIBASE_LONG  offset );
int altibase_stmt_field_count ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_free_result ( ALTIBASE_STMT stmt );
ALTIBASE_STMT altibase_stmt_init ( ALTIBASE altibase );
ALTIBASE_LONG altibase_stmt_num_rows ( ALTIBASE_STMT stmt );
int altibase_stmt_param_count ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_prepare ( ALTIBASE_STMT  stmt,
                                    const char    *qstr );
ALTIBASE_LONG altibase_stmt_processed ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_reset ( ALTIBASE_STMT stmt );
ALTIBASE_RES altibase_stmt_result_metadata ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_send_long_data ( ALTIBASE_STMT  stmt,
                                           int            param_num,
                                           void          *data,
                                           ALTIBASE_LONG  length );
ALTIBASE_RC altibase_stmt_set_array_bind ( ALTIBASE_STMT stmt,
                                           int           array_size );
ALTIBASE_RC altibase_stmt_set_array_fetch ( ALTIBASE_STMT stmt,
                                            int           array_size );
const char * altibase_stmt_sqlstate ( ALTIBASE_STMT stmt );
unsigned short * altibase_stmt_status ( ALTIBASE_STMT stmt );
ALTIBASE_RC altibase_stmt_store_result ( ALTIBASE_STMT stmt );



#if defined (__cplusplus)
};
#endif

#endif /* _ALTIBASE_CDBC_H_ */

