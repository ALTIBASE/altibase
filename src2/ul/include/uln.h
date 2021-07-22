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

#ifndef _O_ULN_H_
#define _O_ULN_H_ 1


#include <acp.h>
#include <acl.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <aciVarString.h>

#include <mtcl.h>

#include <sqlcli.h>
#include <ulnDef.h>

ACP_EXTERN_C_BEGIN

typedef struct ulnObject  ulnObject;

typedef struct ulnEnv     ulnEnv;
typedef struct ulnDbc     ulnDbc;
typedef struct ulnStmt    ulnStmt;
typedef struct ulnDesc    ulnDesc;
typedef struct ulnDescRec ulnDescRec;
typedef struct ulnIndLenPtrPair ulnIndLenPtrPair;
/*
 * ---------------------------
 * cli2 에서 넘어온 상수들
 * ---------------------------
 */

/* fix BUG-17606, BUG-17724 */
/* BUG-33012 A flag when opening UNIX_ODBC_INST_LIB_NAME should be changed */
#if defined(HP_HPUX) ||  defined(IA64_HP_HPUX)
#define ALTIBASE_ODBCINST_LIBRARY_PATH "libodbcinst.sl"
#else /* HP_HPUX  */
#define ALTIBASE_ODBCINST_LIBRARY_PATH "libodbcinst.so"
#endif

/* fix BUG-17606 ,BUG-17724*/
#if defined(HP_HPUX) ||  defined(IA64_HP_HPUX)
/* fix BUG-22936 64bit SQLLEN이 32bit , 64bit 모두 제공해야 합니다. */
#if defined(COMPILE_64BIT)
#if defined(BUILD_REAL_64_BIT_MODE)
#define ALTIBASE_ODBC_NAME "libaltibase_odbc-64bit-ul64.sl"
#else
#define ALTIBASE_ODBC_NAME "libaltibase_odbc-64bit-ul32.sl"
#endif /* BUILD_REAL_64_BIT_MODE */
#else
#define ALTIBASE_ODBC_NAME "libaltibase_odbc.sl"
#endif /* COMPILE_64BIT */
#else /* HP_HPUX  */
/* fix BUG-22936 64bit SQLLEN이 32bit , 64bit 모두 제공해야 합니다. */
#if defined(COMPILE_64BIT)
#if defined(BUILD_REAL_64_BIT_MODE)
#define ALTIBASE_ODBC_NAME "libaltibase_odbc-64bit-ul64.so"
#else
#define ALTIBASE_ODBC_NAME "libaltibase_odbc-64bit-ul32.so"
#endif /* BUILD_REAL_64_BIT_MODE */
#else
#define ALTIBASE_ODBC_NAME "libaltibase_odbc.so"
#endif /* COMPILE_64BIT */
#endif /* HP_HPUX  */

#define MAX_ENVIRONMENTS_CNT 1
#define DBMS_NAME            "Altibase"
#define DBMS_VER             "04.3.0401"

#ifdef USE_ODBC2
# define ALTIBASE_ODBC_VER   "02.50"
#else
# define ALTIBASE_ODBC_VER   "03.51"
#endif

#define ALTIBASE_DLL_VER     "04.5.1341"
#define MAX_NAME_LEN         40
#define MAX_OBJECT_NAME_LEN  128         /* == QC_MAX_OBJECT_NAME_LEN */
#define MAX_DISPLAY_NAME_LEN 42          /* == MMC_MAX_DISPLAY_NAME_SIZE */
#define MAX_VERSTR_LEN       20

#define ULN_SIZE_OF_NULLTERMINATION_CHARACTER 1
#define ULN_NULL_TERMINATION_CHARACTER        0

#define ULN_MAX_CONVERSION_RATIO              (3)

// 1문자의 최대 크기 (가장 큰 문자는 UTF-EBCDIC으로 최대 5바이트)
#define ULN_MAX_CHARSIZE                      5

/*
   Define initial values of Handles
*/
#define ULN_INVALID_HANDLE        -1 

/* BUG-46019 서버에서 오는 메세지를 사용자 APP에 전달하는 Callback Struct */
typedef void (*ulnMessageCallback)(acp_uint8_t  *aMessage,
                                   acp_uint32_t  aLength,
                                   void         *aUserData);

typedef struct ulnMessageCallbackStruct
{
    ulnMessageCallback  mFunction;

    /*
     * 사용자가 임의의 Userdata를 설정할 수 있다.
     * 이 Userdata는 mFunction이 호출 될 때 aUserData로 넘어가게 된다.
     */
    void               *mUserData;
} ulnMessageCallbackStruct;

/*
 * ODBC 3.x functions
 */
SQLRETURN ulnAllocHandle(acp_sint16_t  aHandleType,
                         void         *aInputHandle,
                         void        **aOutputHandlePtr);

SQLRETURN ulnAllocHandleStd(acp_sint16_t  aHandleType,
                            void         *aInputHandle,
                            void        **aOutputHandlePtr);

SQLRETURN ulnFreeHandle(acp_sint16_t  aHandleType,
                        void         *aHandle);

SQLRETURN ulnFreeStmt(ulnStmt      *aStmt,
                      acp_uint16_t  aOption);

SQLRETURN ulnSetEnvAttr(ulnEnv       *aEnv,
                        acp_sint32_t  aAttribute,
                        void         *aValuePtr,
                        acp_sint32_t  aStrLen);

SQLRETURN ulnGetEnvAttr(ulnEnv       *aEnv,
                        acp_sint32_t  aAttribute,
                        void         *aValuePtr,
                        acp_sint32_t  aBufferLength,
                        acp_sint32_t *aStringLengthPtr);

/* PROJ-1721 Name-based Binding */
SQLRETURN ulnBindParameter(ulnStmt      *aStmt,
                           acp_uint16_t  aParamNumber,
                           acp_char_t   *aParamName,
                           acp_sint16_t  aInputOutputType,
                           acp_sint16_t  aValueType,
                           acp_sint16_t  aParamType,
                           ulvULen       aColumnSize,
                           acp_sint16_t  aDecimalDigits,
                           void         *aParamValuePtr,
                           ulvSLen       aBufferLength,
                           ulvSLen      *aStrLenOrIndPtr);

SQLRETURN ulnBindCol(ulnStmt      *aStmt,
                     acp_uint16_t  aColumnNumber,
                     acp_sint16_t  aTargetType,
                     void         *aTargetValuePtr,
                     ulvSLen       aBufferLength,
                     ulvSLen      *aStrLenOrIndPtr);

/*
 * 아래의 두 함수는 SQLError() 함수의 SQLGetDiagRec() 함수로의 매핑때문에 존재한다.
 */
acp_sint32_t ulnObjectGetSqlErrorRecordNumber(ulnObject    *aHandle);
ACI_RC       ulnObjectSetSqlErrorRecordNumber(ulnObject    *aHandle,
                                              acp_sint32_t  aRecNumber);

SQLRETURN ulnGetDiagRec(acp_sint16_t  aHandleType,
                        ulnObject    *aObject,
                        acp_sint16_t  aRecNumber,
                        acp_char_t   *aSqlstate,
                        acp_sint32_t *aNativeErrorPtr,
                        acp_char_t   *aMessageText,
                        acp_sint16_t  aBufferLength,
                        acp_sint16_t *aTextLengthPtr,
                        acp_bool_t    aRemoveAfterGet);

SQLRETURN ulnGetDiagField(acp_sint16_t  aHandleType,
                          ulnObject    *aObject,
                          acp_sint16_t  aRecNumber,
                          acp_sint16_t  aDiagIdentifier,
                          void         *aDiagInfoPtr,
                          acp_sint16_t  aBufferLength,
                          acp_sint16_t *aStringLengthPtr);

SQLRETURN ulnDriverConnect(ulnDbc       *aDbc,
                           acp_char_t   *aConnectionString,
                           acp_sint16_t  aConnectionStringLength,
                           acp_char_t   *aOutConnectionString,
                           acp_sint16_t  aOutBufferLength,
                           acp_sint16_t *aOutConnectionStringLength);

SQLRETURN ulnConnect(ulnDbc       *aDbc,
                     acp_char_t   *aServerName,
                     acp_sint16_t  aServerNameLength,
                     acp_char_t   *aUserName,
                     acp_sint16_t  aUserNameLength,
                     acp_char_t   *aAuthentication,
                     acp_sint16_t  aAuthenticationLength);

// forward decalration
struct ulnFnContext;
struct ulnPtContext;

//fix BUG-17722
ACI_RC ulnConnectCore(ulnDbc              *aDbc,
                      struct ulnFnContext *aFC);

SQLRETURN ulnDisconnect(ulnDbc *aDbc);
SQLRETURN ulnDisconnectLocal(ulnDbc *aDbc);

SQLRETURN ulnFetch(ulnStmt *aStmt);
SQLRETURN ulnFetchScroll(ulnStmt      *aStmt,
                         acp_sint16_t  aFetchOrientation,
                         ulvSLen       aFetchOffset);
SQLRETURN ulnExtendedFetch(ulnStmt      *aStmt,
                           acp_uint16_t  aOrientation,
                           ulvSLen       aOffset,
                           acp_uint32_t *aRowCountPtr,
                           acp_uint16_t *aRowStatusArray);

SQLRETURN ulnGetConnectAttr(ulnDbc       *aDbc,
                            acp_sint32_t  aAttrCode,
                            void         *aValPtr,
                            acp_sint32_t  aValLen,
                            acp_sint32_t *aIndPtr);

SQLRETURN ulnSetConnectAttr(ulnDbc       *aDbc,
                            acp_sint32_t  aAttrCode,
                            void         *aValPtr,
                            acp_sint32_t  aLen);

SQLRETURN ulnNumResultCols(ulnStmt      *aStmt,
                           acp_sint16_t *aColumnCountPtr);
SQLRETURN ulnNumParams(ulnStmt      *aStmt,
                       acp_sint16_t *aParamCountPtr);

SQLRETURN ulnRowCount(ulnStmt *aStmt,
                      ulvSLen *aRowCountPtr);
/* BUG-44572 */
SQLRETURN ulnNumRows(ulnStmt *aStmt,
                     ulvSLen *aNumRowsPtr);

SQLRETURN ulnMoreResults(ulnStmt *aStmt);

SQLRETURN ulnDescribeCol(ulnStmt      *aStmt,
                         acp_uint16_t  aColumnNumber,
                         acp_char_t   *aColumnName,
                         acp_sint16_t  aBufferLength,
                         acp_sint16_t *aNameLengthPtr,
                         acp_sint16_t *aDataTypePtr,
                         ulvULen      *aColumnSizePtr,
                         acp_sint16_t *aDecimalDigitsPtr,
                         acp_sint16_t *aNullablePtr);

SQLRETURN ulnDescribeParam(ulnStmt      *aStmt,
                           acp_uint16_t  aParamNumber,
                           acp_sint16_t *aDataTypePtr,
                           ulvULen      *aParamSizePtr,
                           acp_sint16_t *aDecimalDigitsPtr,
                           acp_sint16_t *aNullablePtr);

SQLRETURN ulnColAttribute(ulnStmt      *aStmt,
                          acp_uint16_t  aColumnNumber,
                          acp_uint16_t  aFieldIdentifier,
                          void         *aCharacterAttributePtr,
                          acp_sint16_t  aBufferLength,
                          acp_sint16_t *aStringLengthPtr,
                          void         *aNumericAttributePtr);

/* PROJ-1721 Name-based Binding */
SQLRETURN ulnPrepare(ulnStmt      *aStmt,
                     acp_char_t   *aStatementText,
                     acp_sint32_t  aTextLength,
                     acp_char_t   *aAnalyzeText);

SQLRETURN ulnExecute(ulnStmt *aStmt);

SQLRETURN ulnExecDirect(ulnStmt      *aStmt,
                        acp_char_t   *aStatementText,
                        acp_sint32_t  aTextLength);

SQLRETURN ulnNativeSql(ulnDbc       *aDbc,
                       acp_char_t   *aInputStatement,
                       acp_sint32_t  aInputLength,
                       acp_char_t   *aOutputStatement,
                       acp_sint32_t  aOutputBufferSize,
                       acp_sint32_t *aOutputLength);

SQLRETURN ulnSetStmtAttr(ulnStmt      *aStmt,
                         acp_sint32_t  aAttribute,
                         void         *aValuePtr,
                         acp_sint32_t  aStringLength);
SQLRETURN ulnGetStmtAttr(ulnStmt      *aStmt,
                         acp_sint32_t  aAttribute,
                         void         *aValuePtr,
                         acp_sint32_t  aBufferLength,
                         acp_sint32_t *aStringLengthPtr);

SQLRETURN ulnSetScrollOptions(ulnStmt      *aStmt,
                              acp_uint16_t  aConcurrency,
                              ulvSLen       aKeySetSize,
                              acp_uint16_t  aRowSetSize);

SQLRETURN ulnPutData(ulnStmt *aStmt,
                     void    *aDataPtr,
                     ulvSLen  aStrLenOrInd);

SQLRETURN ulnParamData(ulnStmt  *aStmt,
                       void    **aValuePtr);

SQLRETURN ulnGetData(ulnStmt      *aStmt,
                     acp_uint16_t  aColumnNumber,
                     acp_sint16_t  aTargetType,
                     void         *aTargetValuePtr,
                     ulvSLen       aBufferLength,
                     ulvSLen      *aIndPtr);

SQLRETURN ulnSetPos(ulnStmt      *aStmt,
                    acp_uint16_t  aRowNumber,
                    acp_uint16_t  aOperation,
                    acp_uint16_t  aLockType);

SQLRETURN ulnBulkOperations(ulnStmt *    aStmt,
                            acp_sint16_t aOperation);

SQLRETURN ulnColumns(ulnStmt      *aStmt,
                     acp_char_t   *aCatalogName,
                     acp_sint16_t  aNameLength1,
                     acp_char_t   *aSchemaName,
                     acp_sint16_t  aNameLength2,
                     acp_char_t   *aTableName,
                     acp_sint16_t  aNameLength3,
                     acp_char_t   *aColumnName,
                     acp_sint16_t  aNameLength4);

SQLRETURN ulnTables(ulnStmt      *aStmt,
                    acp_char_t   *aCatalogName,
                    acp_sint16_t  aNameLength1,
                    acp_char_t   *aSchemaName,
                    acp_sint16_t  aNameLength2,
                    acp_char_t   *aTableName,
                    acp_sint16_t  aNameLength3,
                    acp_char_t   *aTableType,
                    acp_sint16_t  aNameLength4);

SQLRETURN ulnStatistics(ulnStmt      *aStmt,
                        acp_char_t   *aCatalogName,
                        acp_sint16_t  aNameLength1,
                        acp_char_t   *aSchemaName,
                        acp_sint16_t  aNameLength2,
                        acp_char_t   *aTableName,
                        acp_sint16_t  aNameLength3,
                        acp_uint16_t  aUnique,
                        acp_uint16_t  aReserved);

SQLRETURN ulnPrimaryKeys(ulnStmt      *aStmt,
                         acp_char_t   *aCatalogName,
                         acp_sint16_t  aNameLength1,
                         acp_char_t   *aSchemaName,
                         acp_sint16_t  aNameLength2,
                         acp_char_t   *aTableName,
                         acp_sint16_t  aNameLength3);

SQLRETURN ulnProcedures(ulnStmt      *aStmt,
                        acp_char_t   *aCatalogName,   /* unused */
                        acp_sint16_t  aNameLength1,   /* unused */
                        acp_char_t   *aSchemaName,
                        acp_sint16_t  aNameLength2,
                        acp_char_t   *aProcName,
                        acp_sint16_t  aNameLength3);

SQLRETURN ulnProcedureColumns(ulnStmt      *aStmt,
                              acp_char_t   *aCatalogName,
                              acp_sint16_t  aNameLength1,
                              acp_char_t   *aSchemaName,
                              acp_sint16_t  aNameLength2,
                              acp_char_t   *aProcName,
                              acp_sint16_t  aNameLength3,
                              acp_char_t   *aColumnName,
                              acp_sint16_t  aNameLength4,
                              acp_bool_t    aOrderByPos);

SQLRETURN ulnSpecialColumns(ulnStmt      *aStmt,
                            acp_uint16_t  aIdentifierType,   /* unused */
                            acp_char_t   *aCatalogName,      /* unused */
                            acp_sint16_t  aNameLength1,      /* unused */
                            acp_char_t   *aSchemaName,
                            acp_sint16_t  aNameLength2,
                            acp_char_t   *aTableName,
                            acp_sint16_t  aNameLength3,
                            acp_uint16_t  aScope,            /* unused */
                            acp_uint16_t  aNullable);        /* unused */

SQLRETURN ulnForeignKeys(ulnStmt      *aStmt,
                         acp_char_t   *aPKCatalogName,
                         acp_sint16_t  aNameLength1,
                         acp_char_t   *aPKSchemaName,
                         acp_sint16_t  aNameLength2,
                         acp_char_t   *aPKTableName,
                         acp_sint16_t  aNameLength3,
                         acp_char_t   *aFKCatalogName,
                         acp_sint16_t  aNameLength4,
                         acp_char_t   *aFKSchemaName,
                         acp_sint16_t  aNameLength5,
                         acp_char_t   *aFKTableName,
                         acp_sint16_t  aNameLength6);

SQLRETURN ulnTablePrivileges(ulnStmt      *aStmt,
                             acp_char_t   *aCatalogName,  /* unused */
                             acp_sint16_t  aNameLength1,  /* unused */
                             acp_char_t   *aSchemaName,
                             acp_sint16_t  aNameLength2,
                             acp_char_t   *aTableName,
                             acp_sint16_t  aNameLength3);

SQLRETURN ulnEndTran(acp_sint16_t  aHandleType,
                     ulnObject    *aObj,
                     acp_sint16_t  aCompletionType);

SQLRETURN ulnGetFunctions(ulnDbc       *aDbc,
                          acp_uint16_t  aFunctionID,
                          acp_uint16_t *aSupportedPtr);

SQLRETURN ulnGetTypeInfo(ulnStmt      *aStmt,
                         acp_sint16_t  aDataType);

SQLRETURN ulnGetInfo(ulnDbc       *aDbc,
                     acp_uint16_t  aInfoType,
                     void         *aInfoValuePtr,
                     acp_sint16_t  aBufferLength,
                     acp_sint16_t *aStrLenPtr);

SQLRETURN ulnSetDescField(ulnDesc      *aDesc,
                          acp_sint16_t  aRecNumber,
                          acp_sint16_t  aFieldIdentifier,
                          void         *aValuePtr,
                          acp_sint32_t  aBufferLength);

SQLRETURN ulnGetDescRec(ulnDesc      *aDesc,
                        acp_sint16_t  aRecordNumber,
                        acp_char_t   *aName,
                        acp_sint16_t  aBufferLength,
                        acp_sint16_t *aStringLengthPtr,
                        acp_sint16_t *aTypePtr,
                        acp_sint16_t *aSubTypePtr,
                        ulvSLen      *aLengthPtr,
                        acp_sint16_t *aPrecisionPtr,
                        acp_sint16_t *aScalePtr,
                        acp_sint16_t *aNullablePtr);

SQLRETURN ulnGetDescField(ulnDesc      *aDesc,
                          acp_sint16_t  aRecNumber,
                          acp_sint16_t  aFieldIdentifier,
                          void         *aValuePtr,
                          acp_sint32_t  aBufferLength,
                          acp_sint32_t *aStringLengthPtr);

/* BUG-46785 Shard statement partial rollback */
SQLRETURN ulnSetSavepoint( ulnDbc             * aDbc,
                           const acp_char_t   * aSavepointName,
                           acp_uint32_t         aSavepointNameLength );

SQLRETURN ulnRollbackToSavepoint( ulnDbc             * aDbc,
                                  const acp_char_t   * aSavepointName,
                                  acp_uint32_t         aSavepointNameLength );

/*
 * Functions dealing with LOB
 */
SQLRETURN ulnGetLob(acp_sint16_t  aHandleType,
                    ulnObject    *aObject,
                    acp_sint16_t  aLocatorCType,
                    acp_uint64_t  aSrcLocator,
                    acp_uint32_t  aFromPosition,
                    acp_uint32_t  aForLength,
                    acp_sint16_t  aTargetCType,
                    void         *aBuffer,
                    acp_uint32_t  aBufferSize,
                    acp_uint32_t *aLengthWritten);

SQLRETURN ulnPutLob(acp_sint16_t  aHandleType,
                    ulnObject    *aObject,
                    acp_sint16_t  aLocatorCType,
                    acp_uint64_t  aLocator,
                    acp_uint32_t  aFromPosition,
                    acp_uint32_t  aForLength,
                    acp_sint16_t  aSourceCType,
                    void         *aBuffer,
                    acp_uint32_t  aBufferSize);

SQLRETURN ulnGetLobLength(acp_sint16_t  aHandleType,
                          ulnObject    *aObject,
                          acp_uint64_t  aLocator,
                          acp_sint16_t  aLocatorType,
                          acp_uint32_t *aLengthPtr,
                          acp_uint16_t *aIsNull);

SQLRETURN ulnBindFileToCol(ulnStmt       *aStmt,
                           acp_sint16_t   aColumnNumber,
                           acp_char_t   **aFileNameArray,
                           ulvSLen       *aFileNameLengthArray,
                           acp_uint32_t  *aFileOptionPtr,
                           ulvSLen        aMaxFileNameLength,
                           ulvSLen       *aValueLength);

SQLRETURN ulnBindFileToParam(ulnStmt       *aStmt,
                             acp_uint16_t   aParamNumber,
                             acp_sint16_t   aSqlType,
                             acp_char_t   **aFileNameArray,
                             ulvSLen       *aFileNameLengthArray,
                             acp_uint32_t  *aFileOptionPtr,
                             ulvSLen        aMaxFileNameLength,
                             ulvSLen       *aIndicator);

SQLRETURN ulnFreeLob(acp_sint16_t  aHandleType,
                     ulnObject    *aObject,
                     acp_uint64_t  aLocator);

/* PROJ-2047 Strengthening LOB - Added Interfaces */
SQLRETURN ulnTrimLob(acp_sint16_t   aHandleType,
                     ulnObject     *aObject,
                     acp_sint16_t   aLocatorCType,
                     acp_uint64_t   aLocator,
                     acp_uint32_t   aStartOffset);


SQLRETURN ulnCloseCursor(ulnStmt *aStmt);

/* PROJ-2177 User Interface - Cancel */
SQLRETURN ulnCancel(ulnStmt *aStmt);

/*
 * Functions out of specification
 */
SQLRETURN ulnGetPlan(ulnStmt     *aStmt,
                     acp_char_t **aPlan);

/*
 * Functions for converting values of BIGINT and Long
 */
void         ulnTypeConvertSLongToBIGINT(acp_sint64_t  aInputLongValue,
                                         SQLBIGINT    *aOutputLongValuePtr);
void         ulnTypeConvertULongToUBIGINT(acp_uint64_t  aInputLongValue,
                                          SQLUBIGINT   *aOutputLongValuePtr);
acp_sint64_t ulnTypeConvertBIGINTtoSLong(SQLBIGINT aInput);
acp_uint64_t ulnTypeConvertUBIGINTtoULong(SQLUBIGINT aInput);

typedef acp_sint32_t (*ulnGetProfileString)(acp_char_t   *aDSN,
                                            acp_char_t   *aKey,
                                            acp_char_t   *aDefValuet,
                                            acp_char_t   *aBuffer,
                                            acp_sint32_t  aBufferSize,
                                            acp_char_t   *aFileName);

extern ulnGetProfileString  gPrivateProfileFuncPtr;


/* ------------------------------------------------
 *  Debug & Trace functionality
 * ----------------------------------------------*/

extern acp_bool_t gTrace;

#define ULN_DEBUG(a)  acpFprintf(ACP_STD_ERR, "[%s] %s:%d\n", #a, __FILE__, __LINE__); acpStdFlush(ACP_STD_ERR);

#if defined(DEBUG)
# define ULN_TRACE(name) if (gTrace == ACP_TRUE) { ULN_DEBUG(name) }
#else
# define ULN_TRACE(name) ;
#endif

/* ------------------------------------------------
 *  SQLConnect* 관련 Framework
 * ----------------------------------------------*/

// =================== SQLConnect
typedef ACI_RC    (*ulnSetupSQLGetPPSCallback)(ulnDbc *aDbc);
typedef SQLRETURN (*ulnSQLConnectCallback)(ulnDbc       *aDbc,
                                           acp_char_t   *aServerName,
                                           acp_sint16_t  aServerNameLength,
                                           acp_char_t   *aUserName,
                                           acp_sint16_t  aUserNameLength,
                                           acp_char_t   *aAuthentication,
                                           acp_sint16_t  aAuthenticationLength);

typedef struct ulnSQLConnectFrameWork
{
    ulnSetupSQLGetPPSCallback mSetup;
    ulnSQLConnectCallback     mConnect;
} ulnSQLConnectFrameWork;


// ==================== SQLDriverConnect

// return ACP_TRUE : ommitted the DSN else ACP_FALSE
typedef ACI_RC (*ulnCheckConnStrConstraint)(acp_char_t   *aConnStr,
                                            SQLUSMALLINT  aDriverCompletion,
                                            acp_bool_t   *aNeedPrompt);

typedef ACI_RC (*ulnOpenDialog)(SQLHWND       aHWND,
                                acp_char_t   *aOldConnStr,
                                acp_char_t   *aNewConnStr,
                                acp_sint16_t  aNewBufLen);

// connect body
typedef SQLRETURN (*ulnSQLDriverConnectCallback)(ulnDbc       *aDbc,
                                                 acp_char_t   *aConnStr,
                                                 acp_sint16_t  aConnStrLength,
                                                 acp_char_t   *aOutConnStr,
                                                 acp_sint16_t  aOutBufferLength,
                                                 acp_sint16_t *aOutStringLength);
typedef struct ulnSQLDriverConnectFrameWork
{
    ulnSetupSQLGetPPSCallback    mSetup;
    ulnCheckConnStrConstraint    mCheckPrompt;
    ulnOpenDialog                mOpenDialog;
    ulnSQLDriverConnectCallback  mConnect;
} ulnSQLDriverConnectFrameWork;

// bug-26661: nls_use not applied to nls module for ut
extern mtlModule* gNlsModuleForUT;

/* PROJ-2733-DistTxInfo Shard DistTxInfo for NATC */
typedef struct ulnShardDistTxInfoStruct
{
    acp_uint64_t  mEnvSCN;
    acp_uint64_t  mSCN;
    acp_uint64_t  mTxFirstStmtSCN;
    acp_uint64_t  mTxFirstStmtTime;
    acp_uint8_t   mDistLevel;
} ulnShardDistTxInfoStruct;

ACP_EXTERN_C_END

#endif /* _O_ULN_H_ */

