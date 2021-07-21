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

#ifndef _ULP_TYPES_H_
#define _ULP_TYPES_H_ 1

typedef enum
{
    S_UNKNOWN       =-2,
    S_IGNORE        =-1,
    S_SetOptThread  = 0,
    S_Connect       = 1,
    S_Disconnect    = 2,
    S_DirectDML     = 3,/*DML*/
    S_DirectSEL     = 4,/*SELECT*/
    S_DirectRB      = 5,/*ROLLBACK*/
    S_DirectPSM     = 6,/*PSM*/
    S_DirectOTH     = 7,/*others*/
    S_ExecIm        = 8,
    S_Prepare       = 9,
    S_ExecDML       = 10,/*DML*/
    S_ExecOTH       = 11,/*others*/
    S_DeclareCur    = 12,
    S_Open          = 13,
    S_Fetch         = 14,
    S_Close         = 15,
    S_CloseRel      = 16,
    S_DeclareStmt   = 17,
    S_AutoCommit    = 18,
    S_Commit        = 19,
    S_Batch         = 20,
    S_Free          = 21,
    S_AlterSession  = 22,
    S_FreeLob       = 23,
    S_FailOver      = 24,
    S_BindVariables = 25, /* BUG-41010 */
    S_SetArraySize  = 26, /* BUG-41010 */
    S_SelectList    = 27, /* BUG-41010 */
    S_DirectANONPSM = 28, /* BUG-46824 anonymous block*/
    S_GetStmtDiag   = 29, /* TASK-7218 */
    S_GetConditionDiag = 30  /* TASK-7218 */
} ulpStmtType;

typedef enum
{
    H_UNKNOWN = -1,
    H_NUMERIC = 0,
    H_INTEGER,
    H_INT,
    H_LONG,
    H_LONGLONG,
    H_SHORT,
    H_CHAR,
    H_VARCHAR,
    H_BINARY,
    H_BIT,
    H_BYTES,
    H_VARBYTE,
    H_NIBBLE,
    H_FLOAT,
    H_DOUBLE,
    H_USER_DEFINED,   // struct or typedef -> only used in the compiler.
    H_DATE,
    H_TIME,
    H_TIMESTAMP,
    H_BLOB,
    H_CLOB,
    H_BLOBLOCATOR,
    H_CLOBLOCATOR,
    H_BLOB_FILE,
    H_CLOB_FILE,
    H_FAILOVERCB,
    H_NCHAR,
    H_NVARCHAR,
    H_SQLDA,
    H_BINARY2,
    H_NUMERIC_STRUCT /* BUG-45933 */
} ulpHostType;

typedef enum
{
    H_IN = 0,
    H_OUT,
    H_INOUT,
    H_OUT_4PSM
} ulpHostIOType;

/* TASK-7218 Handling Multiple Errors */
typedef enum
{
    H_DIAG_UNKNOWN = -1,
    H_STMT_DIAG_NUMBER = 0,
    H_STMT_DIAG_ROW_COUNT,
    H_COND_DIAG_RETURNED_SQLCODE,
    H_COND_DIAG_RETURNED_SQLSTATE,
    H_COND_DIAG_MESSAGE_TEXT,
    H_COND_DIAG_ROW_NUMBER,
    H_COND_DIAG_COLUMN_NUMBER
} ulpHostDiagType;

#endif
