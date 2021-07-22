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

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <ulnInit.h>
#include <ulpLibInterface.h>
#include <ulpLibInterFuncA.h>
#include <ulpLibInterFuncB.h>
#include <ulpLibMacro.h>
#include <mtcdTypes.h>

/* 초기화 코드가 처리돼야하는지에 대한 flag */
acp_bool_t gUlpLibDoInitProc = ACP_TRUE;

/* 초기화 코드 한번호츨울 보장하기 위한 latch */
acp_thr_rwlock_t  gUlpLibLatch4Init;

typedef ACI_RC ulpLibInterFunc ( acp_char_t *aConnName,
                                 ulpSqlstmt *aSqlstmt,
                                 void *aReserved );

static ulpLibInterFunc * const gUlpLibInterFuncArray[ULP_LIB_INTER_FUNC_MAX] =
{
    ulpSetOptionThread,    /* S_SetOptThread  = 0 */
    ulpConnect,            /* S_Connect       = 1 */
    ulpDisconnect,         /* S_Disconnect    = 2 */
    ulpRunDirectQuery,     /* S_DirectDML     = 3, DML */ 
    ulpRunDirectQuery,     /* S_DirectSEL     = 4, SELECT */    
    ulpRunDirectQuery,     /* S_DirectRB      = 5, ROLLBACK */
    ulpRunDirectQuery,     /* S_DirectPSM     = 6, PSM */  
    ulpRunDirectQuery,     /* S_DirectOTH     = 7, others */
    ulpExecuteImmediate,   /* S_ExecIm        = 8 */
    ulpPrepare,            /* S_Prepare       = 9 */
    ulpExecute,            /* S_ExecDML       = 10, DML */   
    ulpExecute,            /* S_ExecOTH       = 11, others */ 
    ulpDeclareCursor,      /* S_DeclareCur    = 12 */    
    ulpOpen,               /* S_Open          = 13 */ 
    ulpFetch,              /* S_Fetch         = 14 */
    ulpClose,              /* S_Close         = 15 */
    ulpCloseRelease,       /* S_CloseRel      = 16 */
    ulpDeclareStmt,        /* S_DeclareStmt   = 17 */
    ulpAutoCommit,         /* S_AutoCommit    = 18 */
    ulpCommit,             /* S_Commit        = 19 */
    ulpBatch,              /* S_Batch         = 20 */
    ulpFree,               /* S_Free          = 21 */
    ulpAlterSession,       /* S_AlterSession  = 22 */
    ulpFreeLob,            /* S_FreeLob       = 23 */
    ulpFailOver,           /* S_FailOver      = 24 */
    ulpBindVariable,       /* S_BindVariables = 25 */
    ulpSetArraySize,       /* S_SetArraySize  = 26 */
    ulpSelectList,         /* S_SelectList    = 27 */
    ulpRunDirectQuery,     /* S_DirectANONPSM = 28, anonymous block */
    ulpGetStmtDiag,        /* S_GetStmtDiag   = 29 */
    ulpGetConditionDiag    /* S_GetCondDiag   = 30 */
};

static
void ulpLibInit( void )
{
    /* Initialize for latch */
    /* Fix BUG-27644 Apre 의 ulpConnMgr::ulpInitialzie, finalize가 잘못됨. */
    ACE_ASSERT( ulnInitialize() == ACI_SUCCESS );

    /* init grobal latch for the first call of ulpDoEmsql function */
    if ( acpThrRwlockCreate( &gUlpLibLatch4Init, ACP_THR_RWLOCK_DEFAULT ) != ACP_RC_SUCCESS )
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Latch_Init_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
}

EXTERN_C
ulpSqlca *ulpGetSqlca( void )
{
    ACP_TLS(ulpSqlca, sSqlcaData);

    return &sSqlcaData;
}

EXTERN_C
ulpSqlcode *ulpGetSqlcode( void )
{
    ACP_TLS(ulpSqlcode, sSqlcodeData);

    return &sSqlcodeData;
}

EXTERN_C
ulpSqlstate *ulpGetSqlstate( void )
{
    ACP_TLS(ulpSqlstate, sSqlstateData);

    return &sSqlstateData;
}

EXTERN_C
void ulpDoEmsql( acp_char_t * aConnName, ulpSqlstmt *aSqlstmt, void *aReserved )
{
    /* 초기화 함수를 호출하기위함.*/
    static acp_thr_once_t sOnceControl = ACP_THR_ONCE_INITIALIZER;
    /* library 초기화 함수 ulpLibInit 호출함.*/
    acpThrOnce( &sOnceControl, ulpLibInit );

    /*
     *    Initilaize
     */

    /* ulpDoEmsql이 처음 호출될 경우에만 처리되게함. */
    if ( gUlpLibDoInitProc == ACP_TRUE )
    {
        /* get write lock */
        ACI_TEST_RAISE ( acpThrRwlockLockWrite( &gUlpLibLatch4Init ) != ACP_RC_SUCCESS,
                         ERR_LATCH_WRITE );

        /* do some initial job */
        if ( gUlpLibDoInitProc == ACP_TRUE )
        {
            /* BUG-28209 : AIX 에서 c compiler로 컴파일하면 생성자 호출안됨. */
            /* initialize ConnMgr class*/
            ACE_ASSERT( ulpInitializeConnMgr() == ACI_SUCCESS );
        }

        ACI_TEST_RAISE ( acpThrRwlockUnlock( &gUlpLibLatch4Init ) != ACP_RC_SUCCESS,
                         ERR_LATCH_RELEASE );
    }

    /*
     *    Multithread option set for EXEC SQL OPTION (THREADS = ON);
     */
    if ( (acp_bool_t) aSqlstmt->ismt == ACP_TRUE )
    {
        /* BUG-43429 한번켜지면 꺼지지 않는다. */
        gUlpLibOption.mIsMultiThread = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }

    ACI_TEST_RAISE( (0 > (acp_sint32_t) aSqlstmt->stmttype) ||
                    ((acp_sint32_t) aSqlstmt->stmttype > ULP_LIB_INTER_FUNC_MAX),
                    ERR_INVALID_STMTTYPE
                  );

    /*
     *    Call library internal function
     */

    gUlpLibInterFuncArray[aSqlstmt->stmttype]( aConnName, aSqlstmt, aReserved );

    return;

    ACI_EXCEPTION (ERR_LATCH_WRITE);
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Latch_Write_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION (ERR_LATCH_RELEASE);
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Latch_Release_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION (ERR_INVALID_STMTTYPE);
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Unknown_Stmttype_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION_END;

    return;
}

/* BUG-35518 Shared pointer should be supported in APRE */
EXTERN_C
void *ulpAlign( void* aMemPtr, unsigned int aAlign )
{
    return (void*)((( (unsigned long)aMemPtr + aAlign - 1 ) / aAlign) * aAlign);
}

/* BUG-41010 */
EXTERN_C
SQLDA* SQLSQLDAAlloc( int allocSize )
{
    SQLDA *sSqlda = NULL;

    ACE_ASSERT( acpMemAlloc( (void**)&sSqlda, ACI_SIZEOF(SQLDA) ) == ACP_RC_SUCCESS );
    acpMemSet( sSqlda, 0, ACI_SIZEOF(SQLDA) );

    ACE_ASSERT( acpMemAlloc( (void**)&(sSqlda->V),
                             ACI_SIZEOF(char*) * allocSize ) == ACP_RC_SUCCESS );
    acpMemSet( sSqlda->V, 0, ACI_SIZEOF(char*) * allocSize );

    ACE_ASSERT( acpMemAlloc( (void**)&(sSqlda->L),
                             ACI_SIZEOF(int) * allocSize ) == ACP_RC_SUCCESS );
    acpMemSet( sSqlda->L, 0, ACI_SIZEOF(int) * allocSize );

    ACE_ASSERT( acpMemAlloc( (void**)&(sSqlda->T),
                             ACI_SIZEOF(short) * allocSize ) == ACP_RC_SUCCESS );
    acpMemSet( sSqlda->T, 0, ACI_SIZEOF(short) * allocSize );

    ACE_ASSERT( acpMemAlloc( (void**)&(sSqlda->I),
                             ACI_SIZEOF(short*) * allocSize ) == ACP_RC_SUCCESS );
    acpMemSet( sSqlda->I, 0, ACI_SIZEOF(short*) * allocSize );
    
    return sSqlda;
}

EXTERN_C
void SQLSQLDAFree( SQLDA *sqlda )
{
    (void)acpMemFree( sqlda->V );
    (void)acpMemFree( sqlda->L );
    (void)acpMemFree( sqlda->T );
    (void)acpMemFree( sqlda->I );

    (void)acpMemFree( sqlda );
}

/* BUG-45779 */
EXTERN_C
bool ulpShortIsNull( const short *aValue )
{
    return (*aValue == MTD_SMALLINT_NULL) ? true : false;
}

EXTERN_C
bool ulpIntIsNull( const int *aValue )
{
    return (*aValue == MTD_INTEGER_NULL) ? true : false ;
}

EXTERN_C
bool ulpLongIsNull( const long *aValue )
{
    return (*aValue == MTD_BIGINT_NULL) ? true : false ;
}

EXTERN_C
bool ulpFloatIsNull( const void* aValue )
{
    return ((*(acp_uint32_t*)aValue & MTD_REAL_EXPONENT_MASK) == MTD_REAL_EXPONENT_MASK) ? true : false ;
}

EXTERN_C
bool ulpDoubleIsNull( const void* aValue )
{
    return ((*(acp_uint64_t*)aValue & MTD_DOUBLE_EXPONENT_MASK) == MTD_DOUBLE_EXPONENT_MASK) ? true : false ;
}

/* BUG-45933 */
EXTERN_C 
long ulpNumericToLong(SQL_NUMERIC_STRUCT *aNumeric)
{
    acp_slong_t sValue = 0;
    acp_slong_t sLast  = 1;
    int  i;
    
    for (i = 0; i < SQL_MAX_NUMERIC_LEN; i++)
    {   
        sValue += sLast * aNumeric->val[i];
        sLast  *= 256;
    }
    
    return (long)sValue;
}

EXTERN_C 
double ulpNumericToDouble(SQL_NUMERIC_STRUCT *aNumeric)
{
    acp_double_t sDouble    = ulpNumericToLong(aNumeric);
    acp_double_t sTempValue = 1.0;

    int    i;

    if (aNumeric->scale > 0)
    {   
        for (i = 0; i < aNumeric->scale; i++)
        {   
            sTempValue *= 10.0;
        }
        sDouble /= sTempValue;
    }
    else if (aNumeric->scale < 0)
    {   
        for (i = 0; i > aNumeric->scale; i--)
        {
            sTempValue *= 10.0;
        }
        sDouble *= sTempValue;
    }

    if (aNumeric->sign == 0)
    {   
        sDouble *= -1;
    }

    return (double)sDouble;
}
