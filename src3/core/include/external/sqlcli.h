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
 
/***********************************************************************
 * $Id: sqlcli.h 3626 2008-11-18 10:31:35Z shawn $
 ***********************************************************************/

#ifndef _O_ALTIBASE_SQL_CLI_H_
#define _O_ALTIBASE_SQL_CLI_H_ 1

#define HAVE_LONG_LONG

#if defined(_WIN64)
#define BUILD_REAL_64_BIT_MODE
#endif

#if defined(_MSC_VER)
#   include <windows.h>
#endif


#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>

/*
 *  For WINDOWS old compilers such as VS60 and so forth, 
 *  which do not have any information about SQLLEN series types.
 */
#if defined(_MSC_VER)
#if (_MSC_VER <= 1200)
#   if !defined(SQLLEN)
#       define  SQLLEN           SQLINTEGER
#       define  SQLULEN          SQLUINTEGER
#       define  SQLSETPOSIROW    SQLUSMALLINT
        typedef SQLULEN          SQLROWCOUNT;
        typedef SQLULEN          SQLROWSETSIZE;
        typedef SQLULEN          SQLTRANSID;
        typedef SQLLEN           SQLROWOFFSET;
#   endif /* if !defined(SQLLEN) */
#endif /* if (_MSC_VER <= 1200) */
#endif /* if defined(_MSC_VER) */

/*
 * -------------------------
 * Altibase specific types
 * -------------------------
 */

#define SQL_BLOB_LOCATOR        31
#define SQL_CLOB_LOCATOR        41
#define SQL_C_BLOB_LOCATOR      SQL_BLOB_LOCATOR
#define SQL_C_CLOB_LOCATOR      SQL_CLOB_LOCATOR

/*
 * Note : There is no type such as SQL_C_BLOB or SQL_C_CLOB.
 *        One shall make use of SQL_C_BINARY and SQL_C_CHAR instead.
 */

#define SQL_BLOB                30      /* SQL_LONGVARBINARY */
#define SQL_CLOB                40      /* SQL_LONGVARCHAR   */
#define SQL_BYTE                20001
#define SQL_VARBYTE             20003
#define SQL_NIBBLE              20002
#define SQL_GEOMETRY            10003
#define SQL_VARBIT              (-8)
#define SQL_NATIVE_TIMESTAMP    3010

#define SQL_BYTES               SQL_BYTE /* Deprecated !!! use SQL_BYTE instead */

/*
 * -----------------------------
 * Altibase specific attributes
 * -----------------------------
 */

#define ALTIBASE_DATE_FORMAT            2003
#define SQL_ATTR_FAILOVER               2004
#define ALTIBASE_NLS_USE                2005
#define ALTIBASE_EXPLAIN_PLAN           2006
#define SQL_ATTR_IDN_LANG               ALTIBASE_NLS_USE
#define SQL_ATTR_PORT		            2007

#define ALTIBASE_APP_INFO               2008
#define ALTIBASE_STACK_SIZE             2009

#define ALTIBASE_OPTIMIZER_MODE         2010

#define ALTIBASE_OPTIMIZER_MODE_UNKNOWN (0)
#define ALTIBASE_OPTIMIZER_MODE_COST    (1)
#define ALTIBASE_OPTIMIZER_MODE_RULE    (2)

#define ALTIBASE_UTRANS_TIMEOUT         2011
#define ALTIBASE_FETCH_TIMEOUT          2012
#define ALTIBASE_IDLE_TIMEOUT           2013
#define ALTIBASE_HEADER_DISPLAY_MODE    2014

#define SQL_ATTR_LONGDATA_COMPAT        2015

#define ALTIBASE_CONN_ATTR_GPKIWORKDIR  2016
#define ALTIBASE_CONN_ATTR_GPKICONFDIR  2017
#define ALTIBASE_CONN_ATTR_USERCERT     2018
#define ALTIBASE_CONN_ATTR_USERKEY      2019
#define ALTIBASE_CONN_ATTR_USERAID      2020
#define ALTIBASE_CONN_ATTR_USERPASSWD   2021

/* PROJ-2616 */
#define ALTIBASE_CONN_ATTR_IPC_FILEPATH          2032
#define ALTIBASE_CONN_ATTR_IPCDA_FILEPATH        2060
#define ALTIBASE_CONN_ATTR_IPCDA_CLIENT_SLEEP_TIME   2061
#define ALTIBASE_CONN_ATTR_IPCDA_CLIENT_EXPIRE_COUNT 2062

/* PROJ-1573 */
#define ALTIBASE_XA_RMID                2022
/* bug-18246 */
#define ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT   2023
#define ALTIBASE_CONN_ATTR_UNIXDOMAIN_FILEPATH 2024

/* PROJ-1719 */
#define ALTIBASE_XA_NAME                2025
#define ALTIBASE_XA_LOG_DIR             2026

/* PROJ-1518 */
#define ALTIBASE_STMT_ATTR_ATOMIC_ARRAY 2027

/*
 * STMT and DBC attributes that decide how many rows to prefetch
 * or how much memory to reserve as prefetch cache memory
 */
#define SQL_ATTR_PREFETCH_ROWS          12001
#define SQL_ATTR_PREFETCH_BLOCKS        12002
#define SQL_ATTR_PREFETCH_MEMORY        12003

/*
 * File option constants.
 * Used in a call to SQLBindFileToCol() or SQLBindFileToParam().
 */
#define SQL_FILE_CREATE     1
#define SQL_FILE_OVERWRITE  2
#define SQL_FILE_APPEND     3
#define SQL_FILE_READ       4


/*
 * ----------------------------
 * Functions for handling LOB
 * ----------------------------
 */

#ifndef SQL_API
#   define SQL_API
#endif

SQLRETURN SQL_API SQLBindFileToCol(SQLHSTMT     aHandle,
                                   SQLSMALLINT  aColumnNumber,
                                   SQLCHAR     *aFileNameArray,
                                   SQLLEN      *aFileNameLengthArray,
                                   SQLUINTEGER *aFileOptionsArray,
                                   SQLLEN       aMaxFileNameLength,
                                   SQLLEN      *aLenOrIndPtr);

SQLRETURN SQL_API SQLBindFileToParam(SQLHSTMT     aHandle,
                                     SQLSMALLINT  aParameterNumber,
                                     SQLSMALLINT  aSqlType,
                                     SQLCHAR     *aFileNameArray,
                                     SQLLEN      *aFileNameLengthArray,
                                     SQLUINTEGER *aFileOptionsArray,
                                     SQLLEN       aMaxFileNameLength,
                                     SQLLEN      *aLenOrIndPtr);

SQLRETURN SQL_API SQLGetLobLength(SQLHSTMT     aHandle,
                                  SQLUBIGINT   aLocator,
                                  SQLSMALLINT  aLocatorCType,
                                  SQLUINTEGER *aLobLengthPtr);

SQLRETURN SQL_API SQLPutLob(SQLHSTMT     aHandle,
                            SQLSMALLINT  aLocatorCType,
                            SQLUBIGINT   aLocator,
                            SQLUINTEGER  aStartOffset,
                            SQLUINTEGER  aSizeToBeUpdated,
                            SQLSMALLINT  aSourceCType,
                            SQLPOINTER   aDataToPut,
                            SQLUINTEGER  aSizeDataToPut);

SQLRETURN SQL_API SQLGetLob(SQLHSTMT     aHandle,
                            SQLSMALLINT  aLocatorCType,
                            SQLUBIGINT   aLocator,
                            SQLUINTEGER  aStartOffset,
                            SQLUINTEGER  aSizeToGet,
                            SQLSMALLINT  aTargetCType,
                            SQLPOINTER   aBufferToStoreData,
                            SQLUINTEGER  aSizeBuffer,
                            SQLUINTEGER *aSizeReadPtr);

SQLRETURN SQL_API SQLFreeLob(SQLHSTMT aHandle, SQLUBIGINT aLocator);


/*
 * ----------------------------------
 * Altibase Specific CLI Functions
 * ----------------------------------
 */

SQLRETURN SQL_API SQLGetPlan(SQLHSTMT aHandle, SQLCHAR **aPlan);

#endif

