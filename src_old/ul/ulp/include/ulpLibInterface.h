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

#ifndef _ULP_LIB_INTERFACE_
#define _ULP_LIB_INTERFACE_ 1

#include <sqlcli.h>
#include <stdbool.h>
#include <sqltypes.h>

#ifdef __cplusplus
# define EXTERN_C        extern "C"
#else
# define EXTERN_C
#endif

#define sqlca    (*ulpGetSqlca())
#define SQLCODE  (*ulpGetSqlcode())
#define SQLSTATE (*ulpGetSqlstate())
#define ssqlca   sqlca

#define SES_DUPKEY_ERR      (-69720)
#define SES_FILE_CREATE     SQL_FILE_CREATE
#define SES_FILE_OVERWRITE  SQL_FILE_OVERWRITE
#define SES_FILE_APPEND     SQL_FILE_APPEND
#define SES_FILE_READ       SQL_FILE_READ
#define APRE_DUPKEY_ERR     (-69720)
#define APRE_FILE_CREATE    SQL_FILE_CREATE
#define APRE_FILE_OVERWRITE SQL_FILE_OVERWRITE
#define APRE_FILE_APPEND    SQL_FILE_APPEND
#define APRE_FILE_READ      SQL_FILE_READ
#define MAX_CHAR_PTR        (65001)

#define APRE_SHORT_IS_NULL(a)      (ulpShortIsNull(&(a)))
#define APRE_INT_IS_NULL(a)        (ulpIntIsNull(&(a)))
#define APRE_LONG_IS_NULL(a)       (ulpLongIsNull(&(a)))
#define APRE_FLOAT_IS_NULL(a)      (ulpFloatIsNull(&(a)))
#define APRE_DOUBLE_IS_NULL(a)     (ulpDoubleIsNull(&(a)))
#define APRE_NUMERIC_TO_LONG(a)    (ulpNumericToLong(&(a)))
#define APRE_NUMERIC_TO_DOUBLE(a)  (ulpNumericToDouble(&(a)))

typedef char APRE_BINARY;
typedef char APRE_BINARY2;
typedef char APRE_BYTES;
typedef char APRE_VARBYTES;
typedef char APRE_NIBBLE;
typedef int  APRE_INTEGER;
typedef char APRE_NUMERIC;
typedef char APRE_BLOB;
typedef SQLUBIGINT APRE_BLOB_LOCATOR;
typedef char APRE_CLOB;
typedef SQLUBIGINT APRE_CLOB_LOCATOR;
typedef char APRE_BIT;

typedef char SES_BINARY;
typedef char SES_BYTES;
typedef char SES_VARBYTES;
typedef char SES_NIBBLE;
typedef int  SES_INTEGER;
typedef char SES_NUMERIC;
typedef char SES_BLOB;
typedef SQLUBIGINT SES_BLOB_LOCATOR;
typedef char SES_CLOB;
typedef SQLUBIGINT SES_CLOB_LOCATOR;
typedef char SES_BIT;

typedef struct { SQLLEN len; char arr[1]; } VARCHAR;
typedef struct { SQLLEN len; char arr[1]; } varchar;

typedef struct ulpSqlca
{
    char        sqlcaid[8];
    int         sqlcabc;
    int         sqlcode;
    struct
    {
        short   sqlerrml;
        char    sqlerrmc[2048];
    } sqlerrm;
    char        sqlerrp[8];
    int         sqlerrd[6];
    char        sqlwarn[8];
    char        sqlext[8];
} ulpSqlca;
typedef int  ulpSqlcode;
typedef char ulpSqlstate[6];
#define ses_sqlca ulpSqlca

typedef struct ulpHostVar
{
    void*  mHostVar;
    unsigned int mSizeof;
    unsigned int mLen;
    unsigned int mMaxlen;
    short  mUnsign;
    short  mInOut;
    short  mType;
    short  mDiagType;
    short  mIsDynAlloc;
    SQLLEN *mHostInd;
    SQLLEN *mVcInd;
    int    mMoreInfo;

    int isstruct;
    unsigned int structsize;
    int isarr;
    unsigned int arrsize;
    SQLUINTEGER *mFileopt;

    /* Padding for future growth */
    unsigned char filler[64];
} ulpHostVar;

typedef struct ulpSqlstmt
{
    /* Precompiler version */
    int ver;

    /* Embedded SQL info. */
    unsigned int  stmttype;
    char *stmt;
    char *stmtname;
    char *curname;
    char *extrastr;

    /* Error info. */
    ulpSqlca    *sqlcaerr;
    ulpSqlcode  *sqlcodeerr;
    ulpSqlstate *sqlstateerr;

    /* Info. of host variables */
    ulpHostVar *hostvalue;

    /* Binding info. of host variables */
    int isarr;
    unsigned int arrsize;
    int isstruct;
    unsigned int structsize;
    unsigned int iters;
    unsigned short numofhostvar;
    int sqlinfo;
    short scrollcur;

    unsigned int cursorscrollable;
    unsigned int cursorsensitivity;
    unsigned int cursorwithhold;

    void*  statusptr;
    void*  errcodeptr;
    int isatomic;
    int ismt;
    short *esqlopts;
    /* Padding for future growth */
    unsigned char filler[64];
} ulpSqlstmt;

struct SQLDA
{
    int        N; /* Descriptor size in number of entries        */
    char     **V; /* Ptr to Arr of addresses of main variables   */
    int       *L; /* Ptr to Arr of lengths of buffers            */
    short     *T; /* Ptr to Arr of types of buffers              */
    SQLLEN   **I; /* Ptr to Arr of addresses of indicator vars   */
    int        F; /* Number of variables found by DESCRIBE       */
};
typedef struct SQLDA SQLDA;

typedef enum
{
    SQLDA_TYPE_CHAR = 1,
    SQLDA_TYPE_VARCHAR,
    SQLDA_TYPE_SSHORT,
    SQLDA_TYPE_SINT,
    SQLDA_TYPE_SLONG,
    SQLDA_TYPE_SLONGLONG,
    SQLDA_TYPE_DOUBLE,
    SQLDA_TYPE_FLOAT,
    SQLDA_TYPE_USHORT,
    SQLDA_TYPE_UINT,
    SQLDA_TYPE_ULONG,
    SQLDA_TYPE_ULONGLONG,
    SQLDA_TYPE_DATE,
    SQLDA_TYPE_TIME,
    SQLDA_TYPE_TIMESTAMP
} sqldaType;

EXTERN_C ulpSqlca    *ulpGetSqlca( void );
EXTERN_C ulpSqlcode  *ulpGetSqlcode( void );
EXTERN_C ulpSqlstate *ulpGetSqlstate( void );
EXTERN_C void         ulpDoEmsql( char *, ulpSqlstmt *, void * );
EXTERN_C void        *ulpAlign( void* aMemPtr, unsigned int aAlign );

EXTERN_C SQLDA* SQLSQLDAAlloc( int allocSize );
EXTERN_C void SQLSQLDAFree( SQLDA *sqlda );

EXTERN_C bool ulpShortIsNull( const short *aValue );
EXTERN_C bool ulpIntIsNull( const int *aValue );
EXTERN_C bool ulpLongIsNull( const long *aValue );
EXTERN_C bool ulpFloatIsNull( const void *aValue );
EXTERN_C bool ulpDoubleIsNull( const void *aValue );
EXTERN_C long ulpNumericToLong( SQL_NUMERIC_STRUCT *aNumeric );
EXTERN_C double ulpNumericToDouble( SQL_NUMERIC_STRUCT *aNumeric );
#endif
