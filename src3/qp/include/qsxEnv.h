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
 * $Id: qsxEnv.h 90088 2021-02-26 05:02:04Z khkwak $
 **********************************************************************/

#ifndef _O_QSX_ENV_H_
#define _O_QSX_ENV_H_ 1

#include <ideErrorMgr.h>
#include <iduMemory.h>

#include <smi.h>

#include <qc.h>
#include <qcg.h>
#include <qtcDef.h>
#include <qsParseTree.h>
#include <qsxDef.h>
#include <qsxRelatedProc.h>

#define QSX_DEFAULT_SQLCODE 0
#define QSX_DEFAULT_SQLERRM "Successfully completed"

// BUG-41279
// Prevent parallel execution while executing 'select for update' clause.
// qsxEnvInfo.mFlag
#define QSX_ENV_FLAG_INIT                          0x00000000

#define QSX_ENV_STMT_TYPE_MASK                     0x0000001F

#define QSX_ENV_DURING_SELECT                      0x00000001
#define QSX_ENV_DURING_SELECT_FOR_UPDATE           0x00000003  // 0x00000001 & 0x00000002
/* BUG-43197 autonomous transaction */
#define QSX_ENV_DURING_AT                          0x00000004
// BUG-44294 PSM내에서 실행한 DML이 변경한 row 수를 반환하도록 합니다.
#define QSX_ENV_DURING_DML                         0x00000008
#define QSX_ENV_DURING_DDL                         0x00000010

// TASK-7244 PSM partial rollback in Sharding
#define QSX_ENV_BEGIN_PROC                         0x00000100

// TASK-7244 Set shard split method to PSM info
#define QSX_ENV_SHARD_FLAG                         ( QSX_ENV_SHARD_HASH_PROC |  \
                                                     QSX_ENV_SHARD_RANGE_PROC | \
                                                     QSX_ENV_SHARD_LIST_PROC |  \
                                                     QSX_ENV_SHARD_CLONE_PROC | \
                                                     QSX_ENV_SHARD_SOLO_PROC )
#define QSX_ENV_SHARD_HASH_PROC                    0x00000200
#define QSX_ENV_SHARD_RANGE_PROC                   0x00000400
#define QSX_ENV_SHARD_LIST_PROC                    0x00000800
#define QSX_ENV_SHARD_CLONE_PROC                   0x00001000
#define QSX_ENV_SHARD_SOLO_PROC                    0x00002000

struct qsxCursorInfo;
struct qsxArrayInfo;

// PROJ-1075 array variables
typedef struct qsxReturnVarList
{
    qsxArrayInfo            * mArrayInfo;
    struct qsxReturnVarList * mNext;
} qsxReturnVarList;

// BUG-42322
typedef struct qsxStackFrame
{
    qsOID             mOID;
    SInt              mLineno;
    SChar           * mObjectType;
    SChar           * mUserAndObjectName;
} qsxStackFrame;

/* BUG-43160 */
typedef struct qsxRaisedExcpInfo
{
    qsOID                  mRaisedExcpOID;
    SInt                   mRaisedExcpId;
    UInt                   mRaisedExcpErrorMsgLen;
    /* user_maxsize_name.package_maxsize_name.exception_maxsize_name = (128 * 3) + (1 * 2) = 386
       MAX_ERROR_MSG_LEN + 1 = 2048 + 1 = 2049
       객체 정보 + 에러 메시지를 모두 포함하게 위해서는 MAX_ERROR_MSG_LEN + 1을 사용한다.
       에러 메시지가 MAX_ERROR_MSG_LEN을 넘으면, MAX SIZE만큼만 에러메시지를 찍어준다. */
    SChar                  mRaisedExcpErrorMsg[MAX_ERROR_MSG_LEN + 1]; 
} qsxRaisedExcpInfo;

typedef struct qsxEnvInfo
{
    /***********************************/
    /* procedure execution environment */
    /***********************************/

    idBool                 mIsInitialized;
    
    // PROJ-1381 Fetch Across Commit
    // BUG-41729
    // Prevent parallel execution while executing 'select for update' clause.
    UInt                   mFlag;
   
    SInt                   mCallDepth ;

    // these variables are valid only when in the "others" exception block.
    // (mOthersClauseDepth > 0)
    // SEE PL/SQL User's reference 10-110,111.
    SInt                   mOthersClauseDepth;
    
    // plan tree used for recursive call
    qsProcParseTree      * mProcPlan;
 
    // PROJ-1073 Package
    // plan tree used for package initialize section
    qsPkgParseTree       * mPkgPlan;
   
    //--------------------------------
    // session attributes
    // commit, rollback등 session정보를 얻기 위해서는
    // qciSessionCallback 함수를 호출해야하는데,
    // 이때, mmSession 정보가 필요함.
    //--------------------------------
    qcSession            * mSession;

    qsxCursorInfo        * mCursorsInUseFence;
    qsxCursorInfo        * mCursorsInUse;

    // PROJ-1075 array variables
    qsxReturnVarList     * mReturnVar;
   
    // TASK-7244 DBMS_SHARD_GET_DIAGNOSTICS package
    iduList                mErrorList;
    SInt                   mErrorListCount;

    // SQLCODE, SQLERRM attributes.
    UInt                   mSqlCode ;
    SChar                  mSqlErrorMessage [ MAX_ERROR_MSG_LEN + 1 ];

    // BUG-42322
    qsxStackFrame          mStackBuffer[QSX_MAX_CALL_DEPTH + 1];
    SInt                   mStackCursor;
    qsxStackFrame          mPrevStackInfo;

    /* BUG-43154
       password_verify_function이 autonomous_transaction(AT) pragma가 선언된 function일 경우 비정상 종료 */
    idBool                 mExecPWVerifyFunc;

    /* BUG-43160 */
    qsxRaisedExcpInfo      mRaisedExcpInfo;

    // BUG-44856
    qcNamePosition         mSqlInfo;

    // BUG-46074 Multiple trigger event
    qcmColumn            * mTriggerUptColList;
    UInt                   mTriggerEventType;

    // TASK-7218
    UInt                   mSqlCode4Restore;
} qsxEnvInfo;

/* PROJ-2197 PSM Renewal
 * PSM의 SQL구문에서 return bulk into를 사용하면 array를 binding 할 수 없어서
 * return node(array)와 PSM의 tmplate를 DML을 실행하는 qcStatement에
 * 넘겨주기 위한 구조체 */
typedef struct qsxEnvParentInfo
{
    qcTemplate              * parentTmplate;
    qmmReturnInto           * parentReturnInto;
} qsxEnvParentInfo;

#define QSX_ENV_ADD_CURSORS_IN_USE(env, cur)      \
    (cur)->mNext          = (env)->mCursorsInUse; \
    (env)->mCursorsInUse  = (cur);

#define QSX_ENV_CURSORS_IN_USE(env)          \
    ( (env)->mCursorsInUse )

#define QSX_ENV_PLAN_TREE(env) \
    ( (env)-> mProcPlan )

#define QSX_ENV_TEMPLATE_POOL(env) \
    ( (env)-> mProcTemplatePool )

#define QSX_ENV_ERROR_CODE( env )  \
    ( (env)-> mSqlCode )

#define QSX_ENV_ERROR_MESSAGE( env ) \
    ( (env)-> mSqlErrorMessage )

#define QSX_ENV_SESSION_INTF( env ) \
    ( (env)-> mSession )

//PROJ-1073 Package
#define QSX_ENV_PKG_PLAN_TREE(env) \
    ( (env)-> mPkgPlan )

// fix BUG-32565
#define QSX_FIX_ERROR_MESSAGE( env ) \
    ( qsxEnv::fixErrorMessage( env ) )

// BUG-45990
#define QSX_ENV_SET_STMT_FLAG( stmt )                                                              \
   {                                                                                               \
       if ( (stmt)->myPlan->parseTree->stmtKind == QCI_STMT_SELECT )                               \
       {                                                                                           \
           (stmt)->spxEnv->mFlag |= QSX_ENV_DURING_SELECT;                                         \
       }                                                                                           \
       else if ((stmt)->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE)                 \
       {                                                                                           \
           (stmt)->spxEnv->mFlag |= QSX_ENV_DURING_SELECT_FOR_UPDATE;                              \
       }                                                                                           \
       else if ( ((stmt)->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DML ) \
       {                                                                                           \
           (stmt)->spxEnv->mFlag |= QSX_ENV_DURING_DML;                                            \
       }                                                                                           \
       else if ( ((stmt)->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DDL ) \
       {                                                                                           \
           (stmt)->spxEnv->mFlag |= QSX_ENV_DURING_DDL;                                            \
       }                                                                                           \
   }

class qsxEnv
{
  public :
    static IDE_RC invoke (
        qsxEnvInfo         * aEnv,
        qcStatement        * aQcStmt,
        qsOID                aProcOID,     /* procedure, function, package spec의 OID */
        qsOID                aPkgBodyOID,  /* package body의 OID */
        UInt                 aSubprogramID,
        qtcNode            * aCallSpecNode );

    static IDE_RC invokeWithStack (
        qsxEnvInfo        * aEnv,
        qcStatement       * aQcStmt,
        qsOID               aProcOID,     /* procedure, function, package spec의 OID */
        qsOID               aPkgBodyOID,  /* package body의 OID */
        UInt                aSubprogramID,
        qtcNode           * aCallSpecNode,
        mtcStack          * aStack,
        SInt                aStackRemain );
    
    /*  functions for performance. */
    static void initialize( qsxEnvInfo  * aEnv,
                            qcSession   * aSession );
    
    static void resetForInvocation   ( qsxEnvInfo         * aEnv );
    static void reset                ( qsxEnvInfo         * aEnv );

    static IDE_RC increaseCallDepth  ( qsxEnvInfo         * aEnv );
    static IDE_RC decreaseCallDepth  ( qsxEnvInfo         * aEnv );

    // SQLCODE, SQLERRM ATTRIBUTES.
    static void clearErrorVariables  ( qsxEnvInfo         * aEnv );

    // SQLCODE & SQLERRM
    static void setErrorVariables    ( qsxEnvInfo         * aEnv );  
    // SQLCODE
    static void setErrorCode         ( qsxEnvInfo         * aEnv );
    // SQLERRM
    static void setErrorMessage      ( qsxEnvInfo         * aEnv );
    static IDE_RC beginOthersClause  ( qsxEnvInfo         * aEnv );
    
    static IDE_RC endOthersClause    ( qsxEnvInfo         * aEnv );

    // BUG-41311
    static void backupReturnValue( qsxEnvInfo        * aEnv,
                                   qsxReturnVarList ** aValue );

    static void restoreReturnValue( qsxEnvInfo       * aEnv,
                                    qsxReturnVarList * aValue );
    
    // transaction processing statements.
    static IDE_RC savepoint(
        qsxEnvInfo         * aEnv,
        const SChar        * aSavePoint );
    
    static IDE_RC commit  (
        qsxEnvInfo         * aEnv );
    
    static IDE_RC rollback(
        qsxEnvInfo         * aEnv,
        qcStatement        * aQcStmt,
        const SChar        * aSavePoint );
    
    static IDE_RC closeCursorsInUse(
        qsxEnvInfo         * aEnv,
        qcStatement        * aQcStmt );

    // BUG-41311
    static IDE_RC addReturnArray(
        qsxEnvInfo    * aEnv,
        qsxArrayInfo  * aArrayInfo );
    
    // PROJ-1075 function에서 return된 array변수의 할당 해제
    static void freeReturnArray( qsxEnvInfo * aEnv );

    /* REAL static functions that does not require qsxEnvInfo */
    // critical errors are not catched by others clause
    static idBool isCriticalError(UInt aErrorCode);

    // fix BUG-32565
    static void fixErrorMessage( qsxEnvInfo * aEnv );

    // BUG-42322
    static void pushStack( qsxEnvInfo * aEnv );

    static void popStack( qsxEnvInfo * aEnv );

    static void copyStack( qsxEnvInfo * aTargetEnv,
                           qsxEnvInfo * aSourceEnv );

    static void setStackInfo( qsxEnvInfo * aEnv,
                              qsOID        aOID,
                              SInt         aLineno,
                              SChar      * aObjectType,
                              SChar      * aUserAndObjectName );

    /* BUG-43160 */
    static void initializeRaisedExcpInfo( qsxRaisedExcpInfo * aRaisedExcpInfo );

    // TASK-7244 PSM partial rollback in Sharding
    static void setBeginSP( qsxEnvInfo * aEnv );
    static void unsetBeginSP( qsxEnvInfo * aEnv );
    static idBool isBeginSP( qsxEnvInfo * aEnv );
};

#endif /* _O_QSX_ENV_H_ */

