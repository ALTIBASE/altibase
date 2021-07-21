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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnGetConnectAttr.h>

/*
 * ULN_SFID_77
 * SQLGetConnectAttr(), DBC, C2
 *
 *	    -- [1]
 *	    (08003) [2]
 *
 *	where
 *      [1] The Attribute argument was
 *              SQL_ATTR_ACCESS_MODE,
 *              SQL_ATTR_AUTOCOMMIT,
 *              SQL_ATTR_LOGIN_TIMEOUT,
 *              SQL_ATTR_ODBC_CURSORS,
 *              SQL_ATTR_TRACE,
 *              or SQL_ATTR_TRACEFILE,
 *              or a value had been set for the connection attribute.
 *
 *      [2] The Attribute argument was not
 *              SQL_ATTR_ACCESS_MODE,
 *              SQL_ATTR_AUTOCOMMIT,
 *              SQL_ATTR_LOGIN_TIMEOUT,
 *              SQL_ATTR_ODBC_CURSORS,
 *              SQL_ATTR_TRACE,
 *              or SQL_ATTR_TRACEFILE,
 *              and a value had not been set for the connection attribute.
 *
 *      BUGBUG : 해석하기 상당히 난감하다-_-;
 *               일단, 속성이 위에서 열거한 속성이 아닐 때에는 무조건 08003 을 리턴해 보자.
 *               아무래도 속성을 사용자가 세팅했는지 안했는지를 구분하는 무엇인가를
 *               가지고 있어야 할 것 같지만,
 *               일단, 드라이버 매니져가 내어줘야 할 에러코드이므로 ㅋㅋ 그냥 모른척 하자 -_-;
 */
ACI_RC ulnSFID_77(ulnFnContext *aFnContext)
{
    acp_sint32_t aAttrCode;

    if(aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        aAttrCode = *(acp_sint32_t *)(aFnContext->mArgs);

        switch(aAttrCode)
        {
            case SQL_ATTR_ACCESS_MODE:
            case SQL_ATTR_AUTOCOMMIT:
            case SQL_ATTR_LOGIN_TIMEOUT:
            case SQL_ATTR_ODBC_CURSORS:
            case SQL_ATTR_TRACE:
            case SQL_ATTR_TRACEFILE:
            case SQL_ATTR_CONNECTION_DEAD: /* BUG-15741 */

            case ALTIBASE_DATE_FORMAT:     /* BUG-15907 Before Gets */
            case ALTIBASE_APP_INFO:
            case ALTIBASE_NLS_USE:
            case ALTIBASE_NLS_NCHAR_LITERAL_REPLACE:  /* PROJ-1579 NCHAR */
            case ALTIBASE_NLS_CHARACTERSET:           /* PROJ-1579 NCHAR */
            case ALTIBASE_NLS_NCHAR_CHARACTERSET:     /* PROJ-1579 NCHAR */
            case ALTIBASE_EXPLAIN_PLAN:
            case ALTIBASE_STACK_SIZE:
            case ALTIBASE_PREFER_IPV6:
            /* BUG-31144 */
            case ALTIBASE_MAX_STATEMENTS_PER_SESSION:
            case ALTIBASE_SESSION_ID:      /* BUG-34447 isql SET NUMF[ORMAT] fmt */

            /* ?? - does it let get before ?? */
            case ALTIBASE_OPTIMIZER_MODE      :
            /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
            case ALTIBASE_DDL_TIMEOUT         :
            case ALTIBASE_UTRANS_TIMEOUT      :
            case ALTIBASE_FETCH_TIMEOUT       :
            case ALTIBASE_IDLE_TIMEOUT        :
            case ALTIBASE_HEADER_DISPLAY_MODE :
            /* PROJ-2047 Strengthening LOB - LOBCACHE */
            case ALTIBASE_LOB_CACHE_THRESHOLD :

            case SQL_ATTR_PORT:
            case SQL_ATTR_DISCONNECT_BEHAVIOR:
            case SQL_ATTR_CONNECTION_TIMEOUT:
            case SQL_ATTR_CURRENT_CATALOG:
            case SQL_ATTR_METADATA_ID:
            case SQL_ATTR_FAILOVER:
            case SQL_BATCH_SUPPORT:
            case SQL_ATTR_CONNECTION_POOLING:
            case SQL_ATTR_ASYNC_ENABLE:
            case SQL_ATTR_QUIET_MODE:
            case SQL_ATTR_QUERY_TIMEOUT:
            case SQL_ATTR_TXN_ISOLATION:
            case SQL_ATTR_PACKET_SIZE:

            case ALTIBASE_ALTERNATE_SERVERS:
            case ALTIBASE_LOAD_BALANCE:
            case ALTIBASE_CONNECTION_RETRY_COUNT:
            case ALTIBASE_CONNECTION_RETRY_DELAY:
            case ALTIBASE_SESSION_FAILOVER:
                break;

            default:
                ACI_RAISE(LABEL_ABORT_NO_CONNECTION);
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_NO_CONNECTION)
    {
        ulnError(aFnContext, ulERR_ABORT_NO_CONNECTION, "");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static void getString(acp_char_t *aAttr,void *aValPtr, acp_sint32_t aValLen, acp_sint32_t   *aIndPtr)
{
    if(aAttr == NULL)
    {
        acpSnprintf((acp_char_t*)aValPtr, aValLen, "");
    }
    else
    {
        acpSnprintf((acp_char_t*)aValPtr, aValLen, "%s", aAttr);
    }

    if(aIndPtr != NULL)
    {
        *aIndPtr = acpCStrLen((acp_char_t*)aValPtr, aValLen);
    }
}

SQLRETURN ulnGetConnectAttr(ulnDbc       *aDbc,
                            acp_sint32_t  aAttrCode,
                            void         *aValPtr,
                            acp_sint32_t  aValLen,
                            acp_sint32_t *aIndPtr)
{
    ULN_FLAG(sNeedExit);
    ulnFnContext  sFnContext;
    ulnDbc       *sDbc;
    acp_bool_t    sIsClosed;

    /* proj-1573 XA */
    sDbc = ulnDbcSwitch(aDbc);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETCONNECTATTR, sDbc, ULN_OBJ_TYPE_DBC);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(&aAttrCode)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST_RAISE( aValPtr == NULL, ERR_IH000);

    /*
     * BUGBUG : DBC 구조체에 바로 access 하지 말고, 함수를 사용하도록 수정
     */
    switch(aAttrCode)
    {
        case ALTIBASE_DATE_FORMAT:
            (void)getString(sDbc->mDateFormat, aValPtr, aValLen, aIndPtr);
            break;

        case ALTIBASE_APP_INFO:
            if(sDbc->mAppInfo == NULL)
            {
                acpSnprintf((acp_char_t*)aValPtr, aValLen, "");
            }
            else
            {
                acpSnprintf((acp_char_t*)aValPtr, aValLen, "%s", sDbc->mAppInfo);
            }

            if(aIndPtr != NULL)
            {
                *((acp_uint32_t*)aIndPtr) = acpCStrLen((acp_char_t*)aValPtr, aValLen);
            }
            break;


        case ALTIBASE_NLS_USE:
            (void)getString( ulnDbcGetNlsLangString(sDbc), aValPtr, aValLen, aIndPtr);
            break;

        //PROJ-1645 UL Failover.
        case ALTIBASE_ALTERNATE_SERVERS:
            (void)getString( ulnDbcGetAlternateServer(sDbc), aValPtr, aValLen, aIndPtr);
            break;
        case ALTIBASE_LOAD_BALANCE:
            *(acp_uint32_t*)aValPtr = (acp_uint32_t)ulnDbcGetLoadBalance(sDbc);
            break;
        case ALTIBASE_CONNECTION_RETRY_COUNT:
            *(acp_uint32_t*)aValPtr = ulnDbcGetConnectionRetryCount(sDbc);
            break;
        case ALTIBASE_CONNECTION_RETRY_DELAY:
            *(acp_uint32_t*)aValPtr = ulnDbcGetConnectionRetryDelay(sDbc);
            break;
        case ALTIBASE_SESSION_FAILOVER:
            *(acp_uint32_t*)aValPtr = (acp_uint32_t)ulnDbcGetSessionFailover(sDbc);
            break;

        /* bug-31468: adding conn-attr for trace logging */
        case ALTIBASE_TRACELOG:
            *(acp_uint32_t*)aValPtr = ulnDbcGetTraceLog(sDbc);
            break;

        // PROJ-1579 NCHAR
        case ALTIBASE_NLS_NCHAR_LITERAL_REPLACE:
            *(acp_uint32_t*)aValPtr = sDbc->mNlsNcharLiteralReplace;
            break;

        // PROJ-1579 NCHAR
        case ALTIBASE_NLS_CHARACTERSET_VALIDATION:
            *(acp_uint32_t*)aValPtr = sDbc->mNlsCharactersetValidation;
            break;

        // PROJ-1579 NCHAR
        // 데이터베이스 캐릭터 셋을 의미한다.
        case ALTIBASE_NLS_CHARACTERSET:
            (void)getString( ulnDbcGetNlsCharsetString(sDbc), aValPtr, aValLen, aIndPtr);
            break;

        case ALTIBASE_NLS_NCHAR_CHARACTERSET:
            // 내셔널 캐릭터 셋을 의미한다.
            (void)getString( ulnDbcGetNlsNcharCharsetString(sDbc), aValPtr, aValLen, aIndPtr);
            break;

        case SQL_ATTR_ACCESS_MODE:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrAccessMode;
            break;

        case SQL_ATTR_PORT:
            *(acp_uint32_t*)aValPtr = sDbc->mPortNumber;
            break;

        case SQL_ATTR_AUTOCOMMIT:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrAutoCommit;
            break;

        case SQL_ATTR_CONNECTION_TIMEOUT:
            *(acp_uint32_t*)aValPtr = ulnDbcGetConnectionTimeout(sDbc);
            break;

        case SQL_ATTR_QUERY_TIMEOUT:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrQueryTimeout;
            break;

        case SQL_ATTR_CURRENT_CATALOG:
            (void)getString( ulnDbcGetCurrentCatalog(sDbc), aValPtr, aValLen, aIndPtr );
            break;

        case SQL_ATTR_DISCONNECT_BEHAVIOR:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrDisconnect;
            break;

        case SQL_ATTR_ENLIST_IN_DTC:
            /*
             * O1S02
             */
            ulnError(&sFnContext,
                     ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_ENLIST_IN_XA");
            break;

        case SQL_ATTR_ENLIST_IN_XA:
            /*
             * O1S02
             */
            ulnError(&sFnContext,
                     ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_ENLIST_IN_XA");
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrLoginTimeout;
            break;

        case SQL_ATTR_ODBC_CURSORS:
            *(acp_uint32_t*)aValPtr  =  sDbc->mAttrOdbcCursors;
            break;

        case SQL_ATTR_PACKET_SIZE:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrPacketSize;
            break;

        case ALTIBASE_STACK_SIZE: //BUGBUG request to server should or no ???
            *(acp_uint32_t*)aValPtr = sDbc->mAttrStackSize;
            break;

        case SQL_ATTR_QUIET_MODE:
            /*
             * O1S02
             */
            ulnError(&sFnContext,
                     ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_QUIET_MODE");

            *(acp_uint32_t*)aValPtr = (acp_uint32_t)sDbc->mAttrQuietMode;
            break;

        case SQL_ATTR_TRACE:
            /*
             * O1S02
             */
            ulnError(&sFnContext,
                     ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_TRACE");

            *(acp_uint32_t*)aValPtr = sDbc->mAttrTrace;
            break;

        case SQL_ATTR_TRACEFILE:
            /*
             * O1S02
             */
            ulnError(&sFnContext,
                     ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_TRACEFILE");

            (void)getString( sDbc->mAttrTracefile, aValPtr, aValLen, aIndPtr);
            break;

        case SQL_ATTR_TRANSLATE_LIB:
            /*
             * O1S02
             */
            ulnError(&sFnContext,
                     ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_TRANSLATE_LIB");

            (void)getString( sDbc->mAttrTranslateLib, aValPtr, aValLen, aIndPtr);
            break;

        case SQL_ATTR_TRANSLATE_OPTION:
            /*
             * O1S02
             */
            ulnError(&sFnContext,
                     ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_TRANSLATE_OPTION");

            *(acp_uint32_t*)aValPtr = sDbc->mAttrTranslateOption;
            break;

        case SQL_ATTR_TXN_ISOLATION:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrTxnIsolation;
            break;

        case SQL_ATTR_ODBC_VERSION:
            *(acp_uint32_t*)aValPtr = sDbc->mOdbcVersion;
            break;

        case SQL_ATTR_CONNECTION_POOLING: /**< Environment KeekUp BUGBUG */
            *(acp_uint32_t*)aValPtr = sDbc->mAttrConnPooling;
            break;

        case SQL_ATTR_CONNECTION_DEAD:

            /* bug-29379
             * sql_attr_connection_dead promptly detects dead conn
             * before: 현재 저장된 연결상태정보 반환
             * after : 현재 연결상태를 확인하여 결과 반환
             */
            if (ulnDbcIsConnected(sDbc) == ACP_FALSE)
            {
                *(acp_uint32_t*)aValPtr = SQL_TRUE;
            }
            /* check current connection state */
            else
            {
                sIsClosed = ACP_FALSE;
                cmiCheckLink((cmiLink *)sDbc->mLink, &sIsClosed);
                if (sIsClosed == ACP_TRUE)
                {
                    *(acp_uint32_t*)aValPtr = SQL_TRUE;
                }
                else
                {
                    *(acp_uint32_t*)aValPtr = SQL_FALSE;
                }
            }
            break;

        case SQL_ATTR_ASYNC_ENABLE:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrAsyncEnable;
            break;

        case SQL_ATTR_METADATA_ID:
            ACI_RAISE(LABEL_NOT_IMPLEMENTED);
            break;

        case ALTIBASE_EXPLAIN_PLAN:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrExplainPlan;
            break;

        case ALTIBASE_OPTIMIZER_MODE:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrOptimizerMode;
            break;

        /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case ALTIBASE_DDL_TIMEOUT:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrDdlTimeout;
            break;

        case ALTIBASE_UTRANS_TIMEOUT:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrUtransTimeout;
            break;

        case ALTIBASE_FETCH_TIMEOUT:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrFetchTimeout;
            break;

        case ALTIBASE_IDLE_TIMEOUT:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrIdleTimeout;
            break;

        case ALTIBASE_HEADER_DISPLAY_MODE:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrHeaderDisplayMode;
            break;

        case SQL_ATTR_LONGDATA_COMPAT:
            *(acp_uint32_t*)aValPtr = (ulnDbcGetLongDataCompat(sDbc) == ACP_TRUE) ? SQL_TRUE : SQL_FALSE;
            break;

        case ALTIBASE_XA_NAME:
            (void)getString(sDbc->mXaName, aValPtr, aValLen, aIndPtr);
            break;

        /* proj-1538 ipv6 */
        case ALTIBASE_PREFER_IPV6:
            *(acp_uint32_t*)aValPtr =
                (sDbc->mAttrPreferIPv6 == ACP_TRUE) ? SQL_TRUE : SQL_FALSE;
            break;

        /* BUG-31144 */
        case ALTIBASE_MAX_STATEMENTS_PER_SESSION:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrMaxStatementsPerSession;
            break;

        /* PROJ-2209 DBTIMEZONE */
        case ALTIBASE_TIME_ZONE:
            (void)getString( sDbc->mTimezoneString, aValPtr, aValLen, aIndPtr );
            break;

        /* PROJ-2047 Strengthening LOB - LOBCACHE */
        case ALTIBASE_LOB_CACHE_THRESHOLD:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrLobCacheThreshold;
            break;

        /* BUG-36548 Return code of client functions should be differed by ODBC version */
        case ALTIBASE_ODBC_COMPATIBILITY:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrOdbcCompatibility;
            break;

        /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
        case ALTIBASE_FORCE_UNLOCK:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrForceUnlock;
            break;

        /* BUG-34447 isql SET NUMF[ORMAT] fmt */
        case ALTIBASE_SESSION_ID:
            *(acp_uint32_t*)aValPtr = sDbc->mSessionID;
            break;

        /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
        case ALTIBASE_SOCK_RCVBUF_BLOCK_RATIO:
            *(acp_uint32_t *)aValPtr = ulnDbcGetAttrSockRcvBufBlockRatio(sDbc);
            break;

        /* BUG-44271 */
        case ALTIBASE_SOCK_BIND_ADDR:
            (void)getString( sDbc->mSockBindAddr, aValPtr, aValLen, aIndPtr );
            break;

        /* BUG-45286 */
        case ALTIBASE_PDO_DEFER_PROTOCOLS:
            *(acp_uint32_t*)aValPtr = sDbc->mAttrPDODeferProtocols;
            break;

        /* PROJ-2681 */
        case ALTIBASE_IB_LATENCY:
            *(acp_sint32_t*)aValPtr = ulnDbcGetIBLatency(sDbc);
            break;

        case ALTIBASE_IB_CONCHKSPIN:
            *(acp_sint32_t*)aValPtr = ulnDbcGetIBConChkSpin(sDbc);
            break;

        /* BUG-47257 */
        case ALTIBASE_GLOBAL_TRANSACTION_LEVEL:
            *(acp_sint32_t*)aValPtr = ulnDbcGetGlobalTransactionLevel( sDbc );
            break;
            
            // PROJ-2727 add connect attr
        case ALTIBASE_COMMIT_WRITE_WAIT_MODE:
            *(acp_sint32_t*)aValPtr = sDbc->mCommitWriteWaitMode;
            break;

        case ALTIBASE_ST_OBJECT_BUFFER_SIZE:
            *(acp_sint32_t*)aValPtr = sDbc->mSTObjBufSize;
            break;
            
        case ALTIBASE_TRX_UPDATE_MAX_LOGSIZE:
            *(acp_uint64_t*)aValPtr = sDbc->mUpdateMaxLogSize;
            break;            

        case ALTIBASE_PARALLEL_DML_MODE:
            *(acp_sint32_t*)aValPtr = sDbc->mParallelDmlMode;
            break;
            
        case ALTIBASE_NLS_NCHAR_CONV_EXCP:
            *(acp_sint32_t*)aValPtr = sDbc->mNlsNcharConvExcp;
            break;
            
        case ALTIBASE_AUTO_REMOTE_EXEC:
            *(acp_sint32_t*)aValPtr = sDbc->mAutoRemoteExec;
            break;
            
        case ALTIBASE_TRCLOG_DETAIL_PREDICATE:
            *(acp_sint32_t*)aValPtr = sDbc->mTrclogDetailPredicate;
            break;
            
        case ALTIBASE_OPTIMIZER_DISK_INDEX_COST_ADJ:
            *(acp_sint32_t*)aValPtr = sDbc->mOptimizerDiskIndexCostAdj;
            break;
            
        case ALTIBASE_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
            *(acp_sint32_t*)aValPtr = sDbc->mOptimizerMemoryIndexCostAdj;
            break;
            
        case ALTIBASE_NLS_TERRITORY:
            (void)getString( sDbc->mNlsTerritory, aValPtr, aValLen, aIndPtr );
            break;
            
        case ALTIBASE_NLS_ISO_CURRENCY:
            (void)getString( sDbc->mNlsISOCurrency, aValPtr, aValLen, aIndPtr );
            break;

        case ALTIBASE_NLS_CURRENCY:
            (void)getString( sDbc->mNlsCurrency, aValPtr, aValLen, aIndPtr );
            break;
            
        case ALTIBASE_NLS_NUMERIC_CHARACTERS:
            (void)getString( sDbc->mNlsNumChar, aValPtr, aValLen, aIndPtr );
            break;
            
        case ALTIBASE_QUERY_REWRITE_ENABLE:
            *(acp_sint32_t*)aValPtr = sDbc->mQueryRewriteEnable;
            break;
            
        case ALTIBASE_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
            *(acp_sint32_t*)aValPtr = sDbc->mDblinkRemoteStatementAutoCommit;
            break;
            
        case ALTIBASE_RECYCLEBIN_ENABLE:
            *(acp_sint32_t*)aValPtr = sDbc->mRecyclebinEnable;
            break;
            
        case ALTIBASE___USE_OLD_SORT:
            *(acp_sint32_t*)aValPtr = sDbc->mUseOldSort;
            break;
            
        case ALTIBASE_ARITHMETIC_OPERATION_MODE:
            *(acp_sint32_t*)aValPtr = sDbc->mArithmeticOpMode;
            break;
            
        case ALTIBASE_RESULT_CACHE_ENABLE:
            *(acp_sint32_t*)aValPtr = sDbc->mResultCacheEnable;
            break;
            
        case ALTIBASE_TOP_RESULT_CACHE_MODE:
            *(acp_sint32_t*)aValPtr = sDbc->mTopResultCacheMode;
            break;

        case ALTIBASE_OPTIMIZER_AUTO_STATS:
            *(acp_sint32_t*)aValPtr = sDbc->mOptimizerAutoStats;
            break;
            
        case ALTIBASE___OPTIMIZER_TRANSITIVITY_OLD_RULE:
            *(acp_sint32_t*)aValPtr = sDbc->mOptimizerTransitivityOldRule;
            break;
            
        case ALTIBASE_OPTIMIZER_PERFORMANCE_VIEW:
            *(acp_sint32_t*)aValPtr = sDbc->mOptimizerPerformanceView;
            break;
            
        case ALTIBASE_REPLICATION_DDL_SYNC:
            *(acp_sint32_t*)aValPtr = sDbc->mReplicationDDLSync;
            break;
            
        case ALTIBASE_REPLICATION_DDL_SYNC_TIMEOUT:
            *(acp_sint32_t*)aValPtr = sDbc->mReplicationDDLSyncTimeout;
            break;
            
        case ALTIBASE___PRINT_OUT_ENABLE:
            *(acp_sint32_t*)aValPtr = sDbc->mPrintOutEnable;
            break;
            
        case ALTIBASE_TRCLOG_DETAIL_SHARD:
            *(acp_sint32_t*)aValPtr = sDbc->mTrclogDetailShard;
            break;
            
        case ALTIBASE_SERIAL_EXECUTE_MODE:
            *(acp_sint32_t*)aValPtr = sDbc->mSerialExecuteMode;
            break;
            
        case ALTIBASE_TRCLOG_DETAIL_INFORMATION:
            *(acp_sint32_t*)aValPtr = sDbc->mTrcLogDetailInformation;
            break;

        case ALTIBASE___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
            *(acp_sint32_t*)aValPtr = sDbc->mOptimizerDefaultTempTbsType;
            break;

        case ALTIBASE_NORMALFORM_MAXIMUM:
            *(acp_sint32_t*)aValPtr = sDbc->mNormalFormMaximum;
            break;

        case ALTIBASE___REDUCE_PARTITION_PREPARE_MEMORY:
            *(acp_sint32_t*)aValPtr = sDbc->mReducePartPrepareMemory;
            break;
 
        case ALTIBASE_TRANSACTIONAL_DDL:
            *(acp_uint32_t*)aValPtr = sDbc->mTransactionalDDL;
            break;
 
        case ALTIBASE_GLOBAL_DDL:
            *(acp_uint32_t*)aValPtr = sDbc->mGlobalDDL;
            break;
          
        case ALTIBASE_SHARD_STATEMENT_RETRY:
            *(acp_sint32_t*)aValPtr = ulnDbcGetShardStatementRetry( sDbc );
            break;

         case ALTIBASE_INDOUBT_FETCH_TIMEOUT:
            *(acp_sint32_t*)aValPtr = ulnDbcGetIndoubtFetchTimeout( sDbc );
            break;
            
        case ALTIBASE_INDOUBT_FETCH_METHOD:
            *(acp_sint32_t*)aValPtr = ulnDbcGetIndoubtFetchMethod( sDbc );
            break;
           
        default:
            ACI_RAISE(ERR_HY092);
    }

    /*
     * Exit
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(ERR_HY092);
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aAttrCode);
    }

    ACI_EXCEPTION(ERR_IH000);
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }

    ACI_EXCEPTION(LABEL_NOT_IMPLEMENTED)
    {
        ulnError(&sFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}



ACI_RC ulnCallbackDBPropertyGetResult(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aServiceSession,
                                      void               *aUserContext)
{
    cmiProtocolContext        *sCtx       = aProtocolContext;
    ulnFnContext              *sFnContext = (ulnFnContext *)aUserContext;
    ulnDbc                    *sDbc       = sFnContext->mHandle.mDbc;
    acp_uint32_t               sLen       = ACP_UINT32_MAX;
    acp_uint16_t               sPropertyID;
    acp_uint16_t               sOrgCursor;
    acp_uint32_t               sPropertyValue4; /* BUG-39817 */

    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD2(sCtx, &sPropertyID);

    sOrgCursor = sCtx->mReadBlock->mCursor;

    /*
     * Note : uln 의 function context 안에서 언제든지 property get request 를 날릴 수 있을려면,
     *        function context 의 object 가 stmt 일 경우도 고려해야 한다.
     *        특히, date format 을 가져와서 동기화시키는 부분에서 그렇다.
     *
     *        안그러면, 핸들 타입이 dbc 가 아니므로 에러가 나는데,
     *        이 에러가 -_-;;;; 08s02, communication failure 로 나온다.
     *
     * BUGBUG : 콜백함수는 ACI_FAILURE 를 리턴하면 안된다.
     *          에러를 세팅하고, ACI_SUCCESS 를 리턴해야 한다.
     *
     *          그러나 이 함수를 보아하니, 에러코드 세팅하는 부분이 없다.
     *          적절한 에러코드를 세팅하도록 소스를 보강해야 한다.
     *          일단, ACI_FAILURE 를 리턴하도록 둔 상태로 무조건 communication fail 을 리턴하도록
     *          해 두었다가 소스 보강 후에 ACI_SUCCESS 를 리턴하도록 고친다.
     */
    if(ULN_OBJ_GET_TYPE(sDbc) == ULN_OBJ_TYPE_STMT)
    {
        sDbc = ((ulnStmt *)sDbc)->mParentDbc;
    }

    ACI_TEST(ULN_OBJ_GET_TYPE(sDbc) != ULN_OBJ_TYPE_DBC);

    switch(sPropertyID)
    {
        case ULN_PROPERTY_AUTOCOMMIT              :
            CMI_RD1(sCtx, sDbc->mAttrAutoCommit);
            break;

        case ULN_PROPERTY_EXPLAIN_PLAN            :
            CMI_RD1(sCtx, sDbc->mAttrExplainPlan);
            break;

        case ULN_PROPERTY_ISOLATION_LEVEL         :
            CMI_RD4(sCtx, &sPropertyValue4);

            switch (sPropertyValue4) /* BUG-39817 */
            {
                case ULN_SERVER_ISOLATION_LEVEL_READ_COMMITTED:
                    sDbc->mAttrTxnIsolation = SQL_TXN_READ_COMMITTED;
                    break;

                case ULN_SERVER_ISOLATION_LEVEL_REPEATABLE_READ:
                    sDbc->mAttrTxnIsolation = SQL_TXN_REPEATABLE_READ;
                    break;

                case ULN_SERVER_ISOLATION_LEVEL_SERIALIZABLE:
                    sDbc->mAttrTxnIsolation = SQL_TXN_SERIALIZABLE;
                    break;

                default:
                    ACI_TEST(1);
            }
            break;

        case ULN_PROPERTY_OPTIMIZER_MODE          :
            CMI_RD4(sCtx, &sDbc->mAttrOptimizerMode);
            break;

        case ULN_PROPERTY_HEADER_DISPLAY_MODE     :
            CMI_RD4(sCtx, &sDbc->mAttrHeaderDisplayMode);
            break;

        case ULN_PROPERTY_STACK_SIZE              :
            CMI_RD4(sCtx, &sDbc->mAttrStackSize);
            break;

        case ULN_PROPERTY_IDLE_TIMEOUT            :
            CMI_RD4(sCtx, &sDbc->mAttrIdleTimeout);
            break;


        /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case ULN_PROPERTY_DDL_TIMEOUT           :
            CMI_RD4(sCtx, &sDbc->mAttrDdlTimeout);
            break;

        case ULN_PROPERTY_QUERY_TIMEOUT           :
            CMI_RD4(sCtx, &sDbc->mAttrQueryTimeout);
            break;

        case ULN_PROPERTY_FETCH_TIMEOUT           :
            CMI_RD4(sCtx, &sDbc->mAttrFetchTimeout);
            break;

        case ULN_PROPERTY_UTRANS_TIMEOUT          :
            CMI_RD4(sCtx, &sDbc->mAttrUtransTimeout);
            break;

        case ULN_PROPERTY_DATE_FORMAT             :
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mDateFormat, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mDateFormat, sLen);
            break;
        // fix BUG-18971
        case ULN_PROPERTY_SERVER_PACKAGE_VERSION  :
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mServerPackageVer, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mServerPackageVer, sLen);
            break;

        case ULN_PROPERTY_CLIENT_PACKAGE_VERSION  :
        case ULN_PROPERTY_CLIENT_PROTOCOL_VERSION :
        case ULN_PROPERTY_CLIENT_PID              :
        case ULN_PROPERTY_CLIENT_TYPE             :
        case ULN_PROPERTY_NLS                     :
        case ULN_PROPERTY_NLS_NCHAR_LITERAL_REPLACE :   /* PROJ-1579 NCHAR */
        case ULN_PROPERTY_TIME_ZONE: /* PROJ-2209 DBTIMEZONE */
            ACI_TEST(1);
            break;

        case ULN_PROPERTY_NLS_CHARACTERSET        :     /* PROJ-1579 NCHAR */
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mNlsCharsetString, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mNlsCharsetString, sLen);

            if (sDbc->mNlsCharsetString != NULL)
            {
                ACI_TEST(mtlModuleByName( (const mtlModule **)&(sDbc->mCharsetLangModule),
                                          sDbc->mNlsCharsetString,
                                          acpCStrLen(sDbc->mNlsCharsetString, ACP_SINT32_MAX) )
                         != ACI_SUCCESS);
            }

            break;

        case ULN_PROPERTY_NLS_NCHAR_CHARACTERSET  :     /* PROJ-1579 NCHAR */
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mNlsNcharCharsetString, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mNlsNcharCharsetString, sLen);

            if(sDbc->mNlsNcharCharsetString != NULL)
            {
                ACI_TEST(mtlModuleByName( (const mtlModule **)&(sDbc->mNcharCharsetLangModule),
                                          sDbc->mNlsNcharCharsetString,
                                          acpCStrLen(sDbc->mNlsNcharCharsetString, ACP_SINT32_MAX) )
                         != ACI_SUCCESS);
            }
            break;

        case ULN_PROPERTY_ENDIAN              :
            CMI_RD1(sCtx, sDbc->mIsSameEndian);
#if defined(ENDIAN_IS_BIG_ENDIAN)

#else
            sDbc->mIsSameEndian = !(sDbc->mIsSameEndian);
#endif
            break;

        /* BUG-31144 */
        case ULN_PROPERTY_MAX_STATEMENTS_PER_SESSION:
            CMI_RD4(sCtx, &sDbc->mAttrMaxStatementsPerSession);
            break;

        /* PROJ-2047 Strengthening LOB - LOBCACHE */
        case ULN_PROPERTY_LOB_CACHE_THRESHOLD:
            CMI_RD4(sCtx, &sDbc->mAttrLobCacheThreshold);
            break;

        case ULN_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
            CMI_RD1( sCtx, sDbc->mAttrGlobalTransactionLevel );
            break;

            // PROJ-2727 add connect attr
        case ULN_PROPERTY_COMMIT_WRITE_WAIT_MODE:
            CMI_RD1( sCtx, sDbc->mCommitWriteWaitMode );
            break;

        case ULN_PROPERTY_ST_OBJECT_BUFFER_SIZE:
            CMI_RD4(sCtx, &sDbc->mSTObjBufSize);
            break;

        case ULN_PROPERTY_TRX_UPDATE_MAX_LOGSIZE:
            CMI_RD8(sCtx, &sDbc->mUpdateMaxLogSize);
            break;

        case ULN_PROPERTY_PARALLEL_DML_MODE:
            CMI_RD4(sCtx, &sDbc->mParallelDmlMode);
            break;
            
        case ULN_PROPERTY_NLS_NCHAR_CONV_EXCP:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY_AUTO_REMOTE_EXEC:
            CMI_RD4( sCtx, &sDbc->mAutoRemoteExec );
            break;

        case ULN_PROPERTY_TRCLOG_DETAIL_PREDICATE:
            CMI_RD4( sCtx, &sDbc->mTrclogDetailPredicate );
            break;

        case ULN_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ:
            CMI_RD4(sCtx, &sDbc->mOptimizerDiskIndexCostAdj);
            break;
            
        case ULN_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
            CMI_RD4(sCtx, &sDbc->mOptimizerMemoryIndexCostAdj);
            break;

        case ULN_PROPERTY_NLS_TERRITORY:
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mNlsTerritory, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mNlsTerritory, sLen);
            break;

        case ULN_PROPERTY_NLS_ISO_CURRENCY:
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mNlsISOCurrency, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mNlsISOCurrency, sLen);
            break;

        case ULN_PROPERTY_NLS_CURRENCY:
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mNlsISOCurrency, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mNlsISOCurrency, sLen);
            break;

        case ULN_PROPERTY_NLS_NUMERIC_CHARACTERS:
            CMI_RD4(sCtx, &sLen);
            ACI_TEST(ulnDbcAttrMem( &sDbc->mNlsISOCurrency, sLen) != ACI_SUCCESS);
            CMI_RCP(sCtx, sDbc->mNlsISOCurrency, sLen);
            break;

        case ULN_PROPERTY_QUERY_REWRITE_ENABLE:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY_RECYCLEBIN_ENABLE:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY___USE_OLD_SORT:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY_ARITHMETIC_OPERATION_MODE:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY_RESULT_CACHE_ENABLE:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY_TOP_RESULT_CACHE_MODE:
            CMI_RD4( sCtx, &sDbc->mNlsNcharConvExcp );
            break;

        case ULN_PROPERTY_OPTIMIZER_AUTO_STATS:
            CMI_RD4( sCtx, &sDbc->mOptimizerAutoStats );
            break;

        case ULN_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE:
            CMI_RD4( sCtx, &sDbc->mOptimizerTransitivityOldRule );
            break;

        case ULN_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW:
            CMI_RD4( sCtx, &sDbc->mOptimizerPerformanceView );
            break;

        case ULN_PROPERTY_REPLICATION_DDL_SYNC:
            CMI_RD4( sCtx, &sDbc->mReplicationDDLSync );
            break;

        case ULN_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT:
            CMI_RD4( sCtx, &sDbc->mReplicationDDLSyncTimeout );
            break;

        case ULN_PROPERTY___PRINT_OUT_ENABLE:
            CMI_RD4( sCtx, &sDbc->mPrintOutEnable );
            break;

        case ULN_PROPERTY_TRCLOG_DETAIL_SHARD:
            CMI_RD4( sCtx, &sDbc->mTrclogDetailShard );
            break;

        case ULN_PROPERTY_SERIAL_EXECUTE_MODE:
            CMI_RD4( sCtx, &sDbc->mSerialExecuteMode );
            break;

        case ULN_PROPERTY_TRCLOG_DETAIL_INFORMATION:
            CMI_RD4( sCtx, &sDbc->mTrcLogDetailInformation );
            break;

        case ULN_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
            CMI_RD4( sCtx, &sDbc->mOptimizerDefaultTempTbsType );
            break;

        case ULN_PROPERTY_NORMALFORM_MAXIMUM:
            CMI_RD4( sCtx, &sDbc->mNormalFormMaximum );
            break;

        case ULN_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY:
            CMI_RD4( sCtx, &sDbc->mReducePartPrepareMemory );
            break;

        case ULN_PROPERTY_TRANSACTIONAL_DDL:
            CMI_RD4(sCtx, &sDbc->mTransactionalDDL);
            break;

        case ULN_PROPERTY_GLOBAL_DDL:
            CMI_RD4(sCtx, &sDbc->mGlobalDDL);
            break;

        case ULN_PROPERTY_SHARD_STATEMENT_RETRY:
            CMI_RD1( sCtx, sDbc->mShardStatementRetry );
            break;

        case ULN_PROPERTY_INDOUBT_FETCH_TIMEOUT:
            CMI_RD4( sCtx, &sDbc->mIndoubtFetchTimeout );
            break;

        case ULN_PROPERTY_INDOUBT_FETCH_METHOD:
            CMI_RD1( sCtx, sDbc->mIndoubtFetchMethod );
            break;
        case ULN_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD: /* BUG-48132 */
            CMI_RD4( sCtx, &sDbc->mPlanHashOrSortMethod );
            break;
        case ULN_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX        : /* BUG-48161 */
            CMI_RD4( sCtx, &sDbc->mBucketCountMax );
            break;
        case ULN_PROPERTY_DDL_LOCK_TIMEOUT:
            CMI_RD4( sCtx, &sDbc->mDDLLockTimeout );
            break;
        /*
         * Warning : 앞으로 추가되는 서버-클라이언트 프로퍼티는 반드시
         *           ulnSetConnectAttrOff()에 추가되어야 한다.
         */

        default:
            ACI_TEST(1);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;

    sCtx->mReadBlock->mCursor = sOrgCursor;

    switch(sPropertyID)
    {
        case ULN_PROPERTY_AUTOCOMMIT              :
        case ULN_PROPERTY_EXPLAIN_PLAN            :
        case ULN_PROPERTY_ENDIAN                  :
        case ULN_PROPERTY_SHARD_STATEMENT_RETRY   :
        case ULN_PROPERTY_INDOUBT_FETCH_METHOD    :
            sCtx->mReadBlock->mCursor += 1;
            break;

        case ULN_PROPERTY_ISOLATION_LEVEL         :
        case ULN_PROPERTY_OPTIMIZER_MODE          :
        case ULN_PROPERTY_HEADER_DISPLAY_MODE     :
        case ULN_PROPERTY_STACK_SIZE              :
        case ULN_PROPERTY_IDLE_TIMEOUT            :
        case ULN_PROPERTY_QUERY_TIMEOUT           :
        case ULN_PROPERTY_FETCH_TIMEOUT           :
        case ULN_PROPERTY_UTRANS_TIMEOUT          :
        case ULN_PROPERTY_DDL_TIMEOUT             :
        case ULN_PROPERTY_MAX_STATEMENTS_PER_SESSION:
        case ULN_PROPERTY_LOB_CACHE_THRESHOLD     :  /* BUG-36966 */
        case ULN_PROPERTY_TRANSACTIONAL_DDL       :
        case ULN_PROPERTY_GLOBAL_DDL              :
        case ULN_PROPERTY_INDOUBT_FETCH_TIMEOUT   :
        case ULN_PROPERTY_DDL_LOCK_TIMEOUT        :
            sCtx->mReadBlock->mCursor += 4;
            break;

        case ULN_PROPERTY_DATE_FORMAT             :
        case ULN_PROPERTY_SERVER_PACKAGE_VERSION  :
        case ULN_PROPERTY_NLS_CHARACTERSET        :
        case ULN_PROPERTY_NLS_NCHAR_CHARACTERSET  :
            CMI_RD4(sCtx, &sLen);
            sCtx->mReadBlock->mCursor += sLen;
            break;

        default:
            break;
    }

    return ACI_FAILURE;
}
