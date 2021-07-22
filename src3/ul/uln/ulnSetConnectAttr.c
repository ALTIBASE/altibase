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

#include <ulnSetConnectAttr.h>
#include <ulnURL.h>
#include <ulnString.h>
#include <ulnConnString.h>
#include <ulnFailOver.h>
#include <ulnDbc.h>
/* PROJ-1573 XA */
#include <ulxXaConnection.h>
#include <ulnDataSource.h>
#include <ulnConv.h>
#include <ulsdDistTxInfo.h>

/*
 * ULN_SFID_82
 * SQLSetConnectAttr(), DBC, C2
 *      -- [1]
 *      (08003) [2]
 *  where
 *      [1] The Attribute argument was not SQL_ATTR_TRANSLATE_LIB or SQL_ATTR_TRANSLATE_OPTION.
 *      [2] The Attribute argument was SQL_ATTR_TRANSLATE_LIB or SQL_ATTR_TRANSLATE_OPTION.
 */

ACI_RC ulnSFID_82(ulnFnContext *aFnContext)
{
    acp_sint32_t sAttribute;

    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        sAttribute = *(acp_sint32_t *)(aFnContext->mArgs);
        switch (sAttribute)
        {
            case SQL_ATTR_TRANSLATE_LIB:
            case SQL_ATTR_TRANSLATE_OPTION:  /* [2] : 08003 */
                ACI_RAISE(LABEL_ABORT_NO_CONN);
                break;

            default:;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_NO_CONN)
    {
        ulnError(aFnContext, ulERR_ABORT_NO_CONNECTION, "");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_83
 * SQLSetConnectAttr(), DBC, C4, C5
 *      --[3]
 *      (08002)[4]
 *      HY011[5]
 *  where
 *      [3] The Attribute argument was not SQL_ATTR_ODBC_CURSORS or SQL_ATTR_PACKET_SIZE.
 *      [4] The Attribute argument was SQL_ATTR_ODBC_CURSORS.
 *      [5] The Attribute argument was SQL_ATTR_PACKET_SIZE.
 */
ACI_RC ulnSFID_83(ulnFnContext *aFnContext)
{
    acp_sint32_t sAttribute;

    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        sAttribute = *(acp_sint32_t *)(aFnContext->mArgs);
        switch ( sAttribute )
        {
            case SQL_ATTR_PORT:
            case ALTIBASE_NLS_USE:
            case ALTIBASE_NLS_NCHAR_LITERAL_REPLACE: /* PROJ-1579 NCHAR */
            case ALTIBASE_NLS_CHARACTERSET_VALIDATION: /* PROJ-1579 NCHAR */
            case ALTIBASE_NLS_CHARACTERSET: /* PROJ-1579 NCHAR */
            case ALTIBASE_NLS_NCHAR_CHARACTERSET: /* PROJ-1579 NCHAR */
            case SQL_ATTR_ODBC_CURSORS:  /* [4] : 08002 */
            case ALTIBASE_PREFER_IPV6:   /* proj-1538 ipv6 */
                ACI_RAISE(LABEL_ABORT_CONNECTION);
                break;

            case ALTIBASE_XA_NAME:
            case ALTIBASE_XA_RMID:
            case SQL_ATTR_PACKET_SIZE:  /* [5] : HY011 */
            case SQL_ATTR_CURRENT_CATALOG:
            case ALTIBASE_LOAD_BALANCE:
                ACI_RAISE(LABEL_ABORT_SET);
                break;

            default:
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_CONNECTION)
    {
        ulnError(aFnContext, ulERR_ABORT_CONNECTION_NAME_IN_USE);
    }
    ACI_EXCEPTION(LABEL_ABORT_SET)
    {
        ulnError(aFnContext, ulERR_ABORT_ATTRIBUTE_CANNOT_BE_SET_NOW);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_84
 * SQLSetConnectAttr(), DBC 상태 전이함수 : C6 상태
 *      --[3] and [6]
 *      C5[8]
 *      (08002)[4]
 *      HY011[5] or [7]
 *  where
 *      [3] The Attribute argument was not SQL_ATTR_ODBC_CURSORS or SQL_ATTR_PACKET_SIZE.
 *      [4] The Attribute argument was SQL_ATTR_ODBC_CURSORS.
 *      [5] The Attribute argument was SQL_ATTR_PACKET_SIZE.
 *      [6] The Attribute argument was not SQL_ATTR_AUTOCOMMIT,
 *          or the Attribute argument was SQL_ATTR_AUTOCOMMIT and setting this attribute
 *          did not commit the transaction.
 *      [7] The Attribute argument was SQL_ATTR_TXN_ISOLATION.
 *      [8] The Attribute argument was SQL_ATTR_AUTOCOMMIT,
 *          and setting this attribute committed the transaction.
 */
ACI_RC ulnSFID_84(ulnFnContext *aFnContext)
{
    acp_sint32_t sAttribute;

    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        sAttribute = *(acp_sint32_t *)(aFnContext->mArgs);

        /* ONLY BEFORE CONNECT */
        switch (sAttribute)
        {
            case SQL_ATTR_PORT:
            case SQL_ATTR_ODBC_CURSORS: /* [4] : 08002 */
                ACI_RAISE(LABEL_CONN_NAME_IN_USE);
                break;

            case ALTIBASE_XA_NAME:
            case ALTIBASE_XA_RMID:
            case ALTIBASE_NLS_USE:
            case ALTIBASE_NLS_NCHAR_LITERAL_REPLACE:
            case ALTIBASE_NLS_CHARACTERSET_VALIDATION: /* PROJ-1579 NCHAR */
            case ALTIBASE_NLS_CHARACTERSET: /* PROJ-1579 NCHAR */
            case ALTIBASE_NLS_NCHAR_CHARACTERSET: /* PROJ-1579 NCHAR */
            case SQL_ATTR_FAILOVER:
            case SQL_BATCH_SUPPORT:
            case SQL_ATTR_CONNECTION_POOLING:
            case SQL_ATTR_PACKET_SIZE: /* [5] or [7] : HY011 */
            case ALTIBASE_PREFER_IPV6: /* proj-1538 ipv6     */
            case SQL_ATTR_CURRENT_CATALOG:
            case ALTIBASE_LOAD_BALANCE:
                ACI_RAISE(LABEL_INVALID_TIME);
                break;

            default:
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_TIME)
    {
        ulnError(aFnContext, ulERR_ABORT_ATTRIBUTE_CANNOT_BE_SET_NOW);
    }

    ACI_EXCEPTION(LABEL_CONN_NAME_IN_USE)
    {
        ulnError(aFnContext, ulERR_ABORT_CONNECTION_NAME_IN_USE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* BUG-39817 */
static ACI_RC ulnSetTxnIsolation(ulnFnContext *aFnContext, void *aValuePtr)
{
    acp_slong_t   sValue = (acp_slong_t)aValuePtr;
    ulnDbc       *sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    switch (sValue)
    {
        case ULN_ISOLATION_LEVEL_DEFAULT: /* 0 */
        case SQL_TXN_READ_UNCOMMITTED:    /* 1 */
        case SQL_TXN_READ_COMMITTED:      /* 2 */
        case SQL_TXN_REPEATABLE_READ:     /* 4 */
        case SQL_TXN_SERIALIZABLE:        /* 8 */
            /* acceptable */
            break;

        default:
            ACI_RAISE(LABEL_INVALID_ATTR_VALUE);
            break;
    }

    if (sValue == SQL_TXN_READ_UNCOMMITTED)
    {
        ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                 "SQL_ATTR_TXN_ISOLATION changed to SQL_TXN_READ_COMMITTED");
        sValue = SQL_TXN_READ_COMMITTED;
    }
    else
    {
        /* nothing to do */
    }

    if (ulnDbcIsConnected(sDbc) == ACP_TRUE)
    {
        if (sValue == ULN_ISOLATION_LEVEL_DEFAULT)
        {
            ACI_RAISE(LABEL_INVALID_ATTR_VALUE);
        }
        else
        {
            /* nothing to do */
        }

        /* send to server */
        ACI_TEST(ulnSendConnectAttr(aFnContext,
                                    ULN_PROPERTY_ISOLATION_LEVEL,
                                    (void *)sValue) != ACI_SUCCESS);
    }
    else
    {
        /* will be set after connect, if disconnected */
    }

    aFnContext->mHandle.mDbc->mAttrTxnIsolation = (acp_uint32_t)sValue;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_ATTR_VALUE);
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_ATTRIBUTE_VALUE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static acp_sint32_t ulnAttrLength(void *aValuePtr, acp_sint32_t aLength)
{
    switch (aLength)
    {
        case SQL_NULL_DATA:               //(-1)
            aLength = 0;
            break;

        case SQL_DATA_AT_EXEC:            //(-2)
            break;

        case SQL_NTS:                     //(-3)
            aLength = (acp_sint32_t)acpCStrLen((acp_char_t *)aValuePtr, ACP_SINT32_MAX);
            break;

        case SQL_IS_POINTER:              //(-4)
            aLength = ACI_SIZEOF(void *);
            break;

        case SQL_IS_UINTEGER:             //(-5)
        case SQL_IS_INTEGER :             //(-6)
            aLength = ACI_SIZEOF(acp_uint32_t);
            break;

        case SQL_IS_USMALLINT:            //(-7)
        case SQL_IS_SMALLINT:             //(-8)
            aLength = ACI_SIZEOF(acp_uint16_t);
            break;

        default:
            if (aLength < SQL_LEN_BINARY_ATTR_OFFSET)
            {
                aLength = (-aLength) + SQL_LEN_BINARY_ATTR_OFFSET;
            }
            break;
    }

    return aLength;
}

ACI_RC ulnSetConnAttrConnType(ulnFnContext *aFnContext, ulnConnType aConnType)
{
    ulnDbc *sDbc = aFnContext->mHandle.mDbc;

    if (ulnDbcGetConnType(sDbc) == ULN_CONNTYPE_INVALID) // 설정되었으면 무시
    {
        // 설정되어 있지 않을 경우에만 수행

        switch (aConnType)
        {
            case ULN_CONNTYPE_TCP:
                ACI_TEST_RAISE(ulnDbcSetCmiLinkImpl(sDbc, CMI_LINK_IMPL_TCP) != ACI_SUCCESS,
                               LABEL_NOT_SUPPORTED_LINK);
                break;

            case ULN_CONNTYPE_UNIX:
                ACI_TEST_RAISE(ulnDbcSetCmiLinkImpl(sDbc, CMI_LINK_IMPL_UNIX) != ACI_SUCCESS,
                               LABEL_NOT_SUPPORTED_LINK);
                break;

            case ULN_CONNTYPE_IPC:
                ACI_TEST_RAISE(ulnDbcSetCmiLinkImpl(sDbc, CMI_LINK_IMPL_IPC) != ACI_SUCCESS,
                               LABEL_NOT_SUPPORTED_LINK);
                break;

            case ULN_CONNTYPE_IPCDA:
                ACI_TEST_RAISE(ulnDbcSetCmiLinkImpl(sDbc, CMI_LINK_IMPL_IPCDA) != ACI_SUCCESS,
                               LABEL_NOT_SUPPORTED_LINK);
                break;

            /* PROJ-2474 SSL/TLS */
            case ULN_CONNTYPE_SSL:
                ACI_TEST_RAISE(ulnDbcSetCmiLinkImpl(sDbc, CMI_LINK_IMPL_SSL) != ACI_SUCCESS, 
                               LABEL_NOT_SUPPORTED_LINK);
                break;

            /* PROJ-2681 */
            case ULN_CONNTYPE_IB:
                ACI_TEST_RAISE(ulnDbcSetCmiLinkImpl(sDbc, CMI_LINK_IMPL_IB) != ACI_SUCCESS, 
                               LABEL_NOT_SUPPORTED_LINK);
                break;

            case ULN_CONNTYPE_INVALID:
                ACI_RAISE(LABEL_INVALID_CONN_TYPE_ERR);
                break;
        }

        ulnDbcSetConnType(sDbc, aConnType);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_SUPPORTED_LINK)
    {
        ulnError(aFnContext, ulERR_ABORT_CONNTYPE_NOT_SUPPORTED, aConnType);
    }
    ACI_EXCEPTION(LABEL_INVALID_CONN_TYPE_ERR)
    {
        /*
         * HY010
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_CONN_TYPE_ERROR, aConnType);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnSetConnAttrPortNumber(ulnFnContext *aFnContext, acp_sint32_t aPortNumber)
{
    ulnDbc *sDbc = aFnContext->mHandle.mDbc;

    // BUG-38108 Cannot reuse SQLHDBC after connection failure
    ulnDbcSetPortNumber(sDbc, aPortNumber);

    return ACI_SUCCESS;
}

static ACI_RC ulnSetConnAttrCheckValue(ulnFnContext  *aFnContext,
                                       ulnConnAttrID  aConnAttrID,
                                       void          *aValuePtr,
                                       acp_sint32_t   aLength)
{
    acp_sint32_t        sMaxLength;
    ulnDiscreteInt     *sAllowedDIntList;
    ulnReservedString  *sAllowedStrList;
    ulnMetricPrefixInt *sAllowedMIntList;

    /*
     * BUGBUG : 이 함수에 들어오는 모든 String 타입 속성 세팅의 aValuePtr 은
     *          null terminated 가 보장되어야 한다.
     */
 
    // PROJ-2727
    // TRX_UPDATE_MAX_LOGSIZE ULONG value로 인해
    // acp_slong_t => acp_ulong_t 변경
    acp_ulong_t sValue = (acp_ulong_t)aValuePtr;

    /*
     * 범위 검사 및 길이 체크, 그리고 optional feature not implemented 체크
     */
    switch (gUlnConnAttrTable[aConnAttrID].mAttrType)
    {
        case ULN_CONN_ATTR_TYPE_INT:

            sAllowedMIntList = (ulnMetricPrefixInt *)gUlnConnAttrTable[aConnAttrID].mCheck;
            ACI_TEST_RAISE(sAllowedMIntList == NULL, LABEL_NOT_IMPLEMENTED);

            ACI_TEST_RAISE((sValue < (acp_uint64_t)gUlnConnAttrTable[aConnAttrID].mMinValue) ||
                           (sValue > (acp_uint64_t)gUlnConnAttrTable[aConnAttrID].mMaxValue),
                           LABEL_INVALID_ATTR_VALUE);
            break;


        case ULN_CONN_ATTR_TYPE_DISCRETE_INT:

            sAllowedDIntList = (ulnDiscreteInt *)gUlnConnAttrTable[aConnAttrID].mCheck;
            ACI_TEST_RAISE(sAllowedDIntList == NULL, LABEL_NOT_IMPLEMENTED);

            while (sAllowedDIntList->mKey != NULL)
            {
                if (sAllowedDIntList->mValue == (acp_sint32_t)sValue)
                {
                    break;
                }

                sAllowedDIntList++;
            }

            ACI_TEST_RAISE(sAllowedDIntList->mKey == NULL, LABEL_INVALID_ATTR_VALUE);
            break;


        case ULN_CONN_ATTR_TYPE_STRING:
        case ULN_CONN_ATTR_TYPE_UPPERCASE_STRING:

            sAllowedStrList = (ulnReservedString *)gUlnConnAttrTable[aConnAttrID].mCheck;
            ACI_TEST_RAISE(sAllowedStrList == NULL, LABEL_NOT_IMPLEMENTED);

            if (gUlnConnAttrTable[aConnAttrID].mMaxValue == 0)
            {
                sMaxLength = ACP_SINT32_MAX;
            }
            else
            {
                sMaxLength = gUlnConnAttrTable[aConnAttrID].mMaxValue;
            }

            ACI_TEST_RAISE((aLength < gUlnConnAttrTable[aConnAttrID].mMinValue) ||
                           (aLength > sMaxLength),
                           LABEL_INVALID_ATTR_LENGTH);
            break;

        case ULN_CONN_ATTR_TYPE_CALLBACK:
        case ULN_CONN_ATTR_TYPE_POINTER:
            break;

        default:
            ACE_ASSERT(0);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_ATTR_VALUE);
    {
        /* HY024 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_ATTRIBUTE_VALUE);
    }

    ACI_EXCEPTION(LABEL_NOT_IMPLEMENTED)
    {
        /* HYC00 : Optional feature not implemented */
        ulnError(aFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION(LABEL_INVALID_ATTR_LENGTH);
    {
        /* HY024 */
        ulnError(aFnContext, ulERR_ABORT_STRING_LENGTH_TOO_LONG);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnSetConnAttr(ulnFnContext  *aFnContext,
                             ulnConnAttrID  aConnAttrID,
                             void          *aValuePtr,
                             acp_sint32_t   aLength)
{
    /*
     * BUGBUG : 이 함수에 들어오는 모든 String 타입 속성 세팅의 aValuePtr 은
     *          null terminated 가 보장되어야 한다.
     */

    acp_slong_t  sValue = (acp_slong_t)aValuePtr;
    ulnDbc      *sDbc   = aFnContext->mHandle.mDbc;
    acp_bool_t   sIsXaConnection = ulnIsXaConnection(sDbc);
    acp_char_t   sNlsUse[20];
    acp_sint32_t sNlsUseLen;
    acp_slong_t  sOldValue;

#ifdef COMPILE_SHARDCLI
    /* BUG-47272 */
    ACI_TEST_CONT( ulsdIsSetConnectAttr( aFnContext, aConnAttrID ) 
                   == ACP_FALSE, NO_NEED_WORK );
#endif

    /*
     * 범위 검사 및 길이 체크, 그리고 optional feature not implemented 체크
     */

    ACI_TEST(ulnSetConnAttrCheckValue(aFnContext, aConnAttrID, aValuePtr, aLength)
             != ACI_SUCCESS);

    /* 
     * BUG-36256 Improve property's communication
     *
     * 서버에서 지원되지 않는 프로퍼티는 서버에 요청을 하지 않는다.
     * 즉 On을 해도 항상 Off 된다.
     */
    ACI_TEST_RAISE(ulnConnAttrArrHasId(&sDbc->mUnsupportedProperties,
                                       aConnAttrID) == ACI_SUCCESS,
                   NO_NEED_WORK);

    switch (aConnAttrID)
    {
        /*
         * =================================================================
         * Connection String 이나 ODBC.INI 를 통해서만 설정할 수 있는 속성들
         * =================================================================
         */

        /*
         * --------------------------------------
         * String 속성들 : null 체크는 이미 이루어졌음
         * --------------------------------------
         */

        case ULN_CONN_ATTR_DSN:
            ACI_TEST_RAISE(ulnDbcSetDsnString(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_UID:
            ACI_TEST_RAISE(ulnDbcSetUserName(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_UNIXDOMAIN_FILEPATH:
            ACI_TEST_RAISE(ulnDbcSetUnixdomainFilepath(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_IPC_FILEPATH:
            ACI_TEST_RAISE(ulnDbcSetIpcFilepath(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_PWD:
            ACI_TEST_RAISE(ulnDbcSetPassword(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_HOSTNAME:
            ACI_TEST_RAISE(ulnDbcSetHostNameString(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        /*
         * ---------------------------------------
         * acp_uint32_t, Bool 속성들
         * ---------------------------------------
         */

        case ULN_CONN_ATTR_CONNTYPE:
            ACI_TEST(ulnSetConnAttrConnType(aFnContext, (ulnConnType)sValue) != ACI_SUCCESS);
            break;

        // bug-19279 remote sysdba enable
        // ex) PRIVILEGE=SYSDBA
        case ULN_CONN_ATTR_PRIVILEGE:
            ulnDbcSetPrivilege(sDbc, (ulnPrivilege)sValue);
            break;

        //PROJ-1645 UL Failover.
        case ULN_CONN_ATTR_ALTERNATE_SERVERS:
            ACI_TEST_RAISE(ulnDbcSetAlternateServers(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;
        case ULN_CONN_ATTR_LOAD_BALANCE:
            ulnDbcSetLoadBalance(sDbc,(acp_bool_t)sValue);
            break;
        case ULN_CONN_ATTR_CONNECTION_RETRY_COUNT:
            ulnDbcSetConnectionRetryCount(sDbc,(acp_uint32_t)sValue);
            break;
        case ULN_CONN_ATTR_CONNECTION_RETRY_DELAY:
            ulnDbcSetConnectionRetryDelay(sDbc,(acp_uint32_t)sValue);
            break;
        case ULN_CONN_ATTR_SESSION_FAILOVER:
            ulnDbcSetSessionFailover(sDbc,(acp_bool_t)sValue);
            break;

        /* bug-31468: adding conn-attr for trace logging */
        case ULN_CONN_ATTR_TRACELOG:
            ulnDbcSetTraceLog(sDbc,(acp_uint32_t)sValue);
            break;

        /*
         * =================================================================================
         * Connection String, ODBC.INI, SQLSetConnectAttr 어느것으로든 설정할 수 있는 속성들
         * =================================================================================
         */

        /*
         * --------------------------------------
         * String 속성들
         * --------------------------------------
         */

        case ULN_CONN_ATTR_NLS_USE:
            /* ------------------------------------------------
             *  1. 이미 연결된 상태이거나,
             *  2. 초기 연결String 혹은 DSN 으로 부터 읽어올 때 초기상태일 경우
             *     에만 설정가능하다.
             *
             *     ===> ulnSendConnectAttr() 을 호출하지 않는다.
             *          (PROJ-1579 NCHAR에서 호출하는 것으로 변경됨)
             * ----------------------------------------------*/
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);

            acpCStrCpy(sNlsUse, ACI_SIZEOF(sNlsUse), (acp_char_t*)aValuePtr, aLength);
            sNlsUseLen = (acp_sint32_t)acpCStrLen(sNlsUse, ACI_SIZEOF(sNlsUse));

            acpCStrToUpper(sNlsUse, sNlsUseLen);

            //BUG-22684
            // Client Characterset을  mClientCharsetLangModule에 저장한다.
            ACI_TEST_RAISE(mtlModuleByName((const mtlModule **)&(sDbc->mClientCharsetLangModule),
                                           sNlsUse,
                                           sNlsUseLen)
                           != ACI_SUCCESS, LABEL_CHARACTERSET_NOT_FOUND);

            // bug-26661: nls_use not applied to nls module for ut
            // nls_use 값을 UT에서 사용할 gNlsModuleForUT에도 적용
            ACI_TEST_RAISE(mtlModuleByName((const mtlModule **)&gNlsModuleForUT,
                                           sNlsUse,
                                           sNlsUseLen)
                           != ACI_SUCCESS, LABEL_CHARACTERSET_NOT_FOUND);

            ACI_TEST_RAISE(ulnDbcSetNlsLangString(sDbc, (acp_char_t *)sNlsUse, sNlsUseLen) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);

            // PROJ-1579 NCHAR
            // 서버에 ALTIBASE_NLS_USE(클라이언트 캐릭터 셋) 정보를 알려준다.
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_NLS, sDbc->mNlsLangString)
                     != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_DATE_FORMAT:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            ACI_TEST_RAISE(ulnDbcSetDateFormat(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);

            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_DATE_FORMAT, sDbc->mDateFormat)
                     != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_TIME_ZONE:
            ACI_TEST_RAISE( aValuePtr == NULL, ERR_HY090 );
            ACI_TEST_RAISE( ulnDbcSetTimezoneSring( sDbc, (acp_char_t *)aValuePtr, aLength )
                            != ACI_SUCCESS,
                            LABEL_NOT_ENOUGH_MEMORY );

            ACI_TEST( ulnSendConnectAttr( aFnContext, ULN_PROPERTY_TIME_ZONE, sDbc->mTimezoneString )
                      != ACI_SUCCESS );
            break;

        case ULN_CONN_ATTR_APP_INFO:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            ACI_TEST_RAISE(ulnDbcSetAppInfo(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);

            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_APP_INFO, sDbc->mAppInfo)
                     != ACI_SUCCESS);
            break;

        // PROJ-1579 NCHAR
        case ULN_CONN_ATTR_NLS_CHARACTERSET:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            break;

        // PROJ-1579 NCHAR
        case ULN_CONN_ATTR_NLS_NCHAR_CHARACTERSET:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            break;

        /*
         * --------------------------------
         * Integer, discrete integer 속성들
         * --------------------------------
         */

        case ULN_CONN_ATTR_QUERY_TIMEOUT:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_QUERY_TIMEOUT, (void *)sValue)  != ACI_SUCCESS);
            sDbc->mAttrQueryTimeout = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_LOGIN_TIMEOUT:
            //BUG-24574 [ODBC] SQLSetConnectAttr(dbc, SQL_ATTR_LOGIN_TIMEOUT, 0, 0); 했을때 접속 실패
            if (sValue != 0)
            {
                sDbc->mAttrLoginTimeout = (acp_uint32_t)sValue;
            }
            else
            {
                sDbc->mAttrLoginTimeout = ULN_TMOUT_MAX;
            }
            break;

        case ULN_CONN_ATTR_AUTOCOMMIT:
            /* if xa enlist error */
            ACI_TEST_RAISE(ulnIsXaActive(sDbc) == ACP_TRUE, LABEL_CANNOT_BE_SET_NOW);
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_AUTOCOMMIT, (void *)sValue) != ACI_SUCCESS);
            sDbc->mAttrAutoCommit = (acp_uint8_t)sValue;

            /* PROJ-2733-DistTxInfo autocommit off이면 DistTxInfo를 초기화 */
            if (sDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF)
            {
                ulsdInitDistTxInfo(sDbc);

                /* TASK-7219 Non-shard DML */
                ulnDbcInitStmtExecSeqForShardTx(sDbc);
            }
            break;

        case ULN_CONN_ATTR_CONNECTION_TIMEOUT:
            ulnDbcSetConnectionTimeout(sDbc, (acp_uint32_t)sValue);
            break;

        case ULN_CONN_ATTR_PACKET_SIZE:
            ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_PACKET_SIZE");
            sDbc->mAttrPacketSize = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_TXN_ISOLATION:
            ACI_TEST(ulnSetTxnIsolation(aFnContext, aValuePtr) != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_ODBC_VERSION:
            sDbc->mOdbcVersion = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_DISCONNECT_BEHAVIOR:
            sDbc->mAttrDisconnect = (acp_uint8_t)sValue;
            break;

        case ULN_CONN_ATTR_CONNECTION_POOLING:
            /*
             * BUGBUG : ENV 의 설정에 맞게 고려해야 한다.
             */
            sDbc->mAttrConnPooling = (acp_uint8_t)sValue;
            break;

        case ULN_CONN_ATTR_PORT_NO:
            ACI_TEST(ulnSetConnAttrPortNumber(aFnContext, (acp_sint32_t)sValue) != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_ASYNC_ENABLE:
            sDbc->mAttrAsyncEnable = (acp_uint8_t)sValue;
            break;

        case ULN_CONN_ATTR_METADATA_ID:
            break;

        case ULN_CONN_ATTR_EXPLAIN_PLAN:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_EXPLAIN_PLAN, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrExplainPlan = (acp_uint8_t)sValue;
            break;

        case ULN_CONN_ATTR_STACK_SIZE:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_STACK_SIZE, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrStackSize = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_ACCESS_MODE:
            sDbc->mAttrAccessMode = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_OPTIMIZER_MODE:
            ACI_TEST(ulnSendConnectAttr(aFnContext,
                                        ULN_PROPERTY_OPTIMIZER_MODE,
                                        (void *)sValue) != ACI_SUCCESS);
            sDbc->mAttrOptimizerMode = (acp_uint32_t)sValue;
            break;

        /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case ULN_CONN_ATTR_DDL_TIMEOUT:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_DDL_TIMEOUT, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrDdlTimeout = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_UTRANS_TIMEOUT:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_UTRANS_TIMEOUT, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrUtransTimeout = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_FETCH_TIMEOUT:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_FETCH_TIMEOUT, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrFetchTimeout = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_IDLE_TIMEOUT:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_IDLE_TIMEOUT, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrIdleTimeout = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_XA_RMID:
            /* PROJ-1573 XA */
            /* if dbc is already enlisted in xa, error */
            ACI_TEST_RAISE(sIsXaConnection == ACP_TRUE, LABEL_CANNOT_BE_SET_NOW);
            /* if no such rmid connection exist, error */
            ACI_TEST_RAISE(ulxFindConnection((acp_sint32_t)sValue, &(sDbc->mXaConnection))
                           != ACI_SUCCESS, LABEL_CANNOT_BE_SET_NOW);
            /* set enlist flag */
            sDbc->mXaEnlist = ACP_TRUE;
            break;

        case ULN_CONN_ATTR_XA_NAME:
            ACI_TEST_RAISE(ulnDbcSetXaName(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        /* BUG-20898 */
        case  ULN_CONN_ATTR_XA_LOG_DIR:
            // do Nothing
            break;

        case ULN_CONN_ATTR_ENLIST_IN_DTC:
        case ULN_CONN_ATTR_ENLIST_IN_XA:
            if (sValue == SQL_TRUE)
            {
                /* if this connection already enlisted in xa, error */
                ACI_TEST_RAISE(sIsXaConnection == ACP_TRUE,
                               LABEL_CANNOT_BE_SET_NOW);
                /* get first connection in xa connection list */
                ACI_TEST_RAISE(ulxGetFirstConnection(&sDbc->mXaConnection),
                               LABEL_CANNOT_BE_SET_NOW);
                /* set enlist flag */
                sDbc->mXaEnlist = ACP_TRUE;
            }
            else
            {
                /* if this connection already enlisted in xa, error */
                ACI_TEST_RAISE(sIsXaConnection == ACP_TRUE,
                               LABEL_CANNOT_BE_SET_NOW);
            }
            break;

        case ULN_CONN_ATTR_HEADER_DISPLAY_MODE:
            ACI_TEST(ulnSendConnectAttr(aFnContext,
                                        ULN_PROPERTY_HEADER_DISPLAY_MODE,
                                        (void *)sValue) != ACI_SUCCESS);
            sDbc->mAttrHeaderDisplayMode = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_LONGDATA_COMPAT:
            if (sValue == SQL_TRUE)
            {
                ulnDbcSetLongDataCompat(sDbc, ACP_TRUE);
            }
            else
            {
                ulnDbcSetLongDataCompat(sDbc, ACP_FALSE);
            }
            break;

        case ULN_CONN_ATTR_ANSI_APP:
            if (sValue == SQL_TRUE)
            {
                ulnDbcSetAnsiApp(sDbc, ACP_TRUE);
            }
            else
            {
                ulnDbcSetAnsiApp(sDbc, ACP_FALSE);
            }
            break;

        // PROJ-1579 NCHAR
        case ULN_CONN_ATTR_NLS_NCHAR_LITERAL_REPLACE:
            ACI_TEST(ulnSendConnectAttr(aFnContext,
                                        ULN_PROPERTY_NLS_NCHAR_LITERAL_REPLACE,
                                        (void *)sValue) != ACI_SUCCESS);
            sDbc->mNlsNcharLiteralReplace = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_NLS_CHARACTERSET_VALIDATION:
            sDbc->mNlsCharactersetValidation = (acp_uint32_t)sValue;
            break;

        /*
         * ====================================================
         * SQLSetConnectAttr() 을 통해서만 설정가능한 속성들
         * ====================================================
         */

        case ULN_CONN_ATTR_MESSAGE_CALLBACK:
            sOldValue = (sDbc->mMessageCallback != NULL) ? (acp_slong_t)ACP_TRUE : (acp_slong_t)ACP_FALSE;

            /* BUG-46019 MESSAGE_CALLBACK은 ulnSendConnectAttr() 이전에 값을 설정해야 한다. */
            ulnRegisterMessageCallback(sDbc, (ulnMessageCallbackStruct *)aValuePtr);

            sValue = (sDbc->mMessageCallback != NULL) ? (acp_slong_t)ACP_TRUE : (acp_slong_t)ACP_FALSE;

            /* BUG-46019 필요에 따라 서버에 콜백 등록 여부를 전달한다. */
            if (sOldValue != sValue)
            {
                ACI_TEST(ulnSendConnectAttr(aFnContext,
                                            ULN_PROPERTY_MESSAGE_CALLBACK,
                                            (void *)sValue) != ACI_SUCCESS);
            }
            break;

        //PROJ-1645 UL Failover.
        case ULN_CONN_ATTR_FAILOVER_CALLBACK:
            ulnDbcSetFailoverCallbackContext(sDbc,(SQLFailOverCallbackContext*) aValuePtr);
            break;

        /*
         * =====================================================================
         * 아직 구현하지 않았거나 지원하지 않는 속성들 : 윗쪽에서 에러 내어버림.
         * =====================================================================
         */

        case ULN_CONN_ATTR_URL:
            break;
        case ULN_CONN_ATTR_ODBC_CURSORS: /* odbc 3.0 에서는 쓸 수 없다. stmt 속성 */
            sDbc->mAttrOdbcCursors = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_CURRENT_CATALOG:
            ACI_TEST_RAISE(ulnDbcSetCurrentCatalog(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS, 
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_TRACEFILE:
            ACI_TEST_RAISE(ulnDbcSetTracefile(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS, 
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_TRANSLATE_LIB:
            ACI_TEST_RAISE(ulnDbcSetTranslateLib(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS, 
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_TRANSLATE_OPTION:
            sDbc->mAttrTranslateOption = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_TRACE:
            sDbc->mAttrTrace = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_QUIET_MODE:
            sDbc->mAttrQuietMode = sValue;
            break;

        /* proj-1538 ipv6 */
        case ULN_CONN_ATTR_PREFER_IPV6:
            sDbc->mAttrPreferIPv6 = (sValue == SQL_TRUE)? ACP_TRUE: ACP_FALSE;
            break;

        /* BUG-31144 */
        case ULN_CONN_ATTR_MAX_STATEMENTS_PER_SESSION:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_MAX_STATEMENTS_PER_SESSION, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrMaxStatementsPerSession= (acp_uint32_t)sValue;
            break;

        /* PROJ-1891 Deferred Prepare */
        case ULN_CONN_ATTR_DEFERRED_PREPARE:
            ulnDbcSetAttrDeferredPrepare(sDbc, (acp_uint8_t)sValue);
            break;

        /* PROJ-2047 Strengthening LOB - LOBCACHE */
        case ULN_CONN_ATTR_LOB_CACHE_THRESHOLD:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_LOB_CACHE_THRESHOLD, (void *)sValue)
                     != ACI_SUCCESS);
            sDbc->mAttrLobCacheThreshold = (acp_uint32_t)sValue;
            break;

        /* BUG-36548 Return code of client functions should be differed by ODBC version */
        case ULN_CONN_ATTR_ODBC_COMPATIBILITY:
            sDbc->mAttrOdbcCompatibility = (acp_uint32_t)sValue;
            break;
                
        /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
        case ULN_CONN_ATTR_FORCE_UNLOCK:
            sDbc->mAttrForceUnlock = (acp_uint32_t)sValue;
            break;

        /* PROJ-2474 SSL/TLS */
        case ULN_CONN_ATTR_SSL_CA:
            ACI_TEST_RAISE(ulnDbcSetSslCa(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
            		       LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_SSL_CAPATH:
            ACI_TEST_RAISE(ulnDbcSetSslCaPath(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
            		       LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_SSL_CERT:
            ACI_TEST_RAISE(ulnDbcSetSslCert(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
            		       LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_SSL_KEY:
            ACI_TEST_RAISE(ulnDbcSetSslKey(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
            		       LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_SSL_VERIFY:
            sDbc->mAttrSslVerify = (acp_bool_t)sValue;
            break;

        case ULN_CONN_ATTR_SSL_CIPHER:
            ACI_TEST_RAISE(ulnDbcSetSslCipher(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
            		       LABEL_NOT_ENOUGH_MEMORY);
            break;

        case ULN_CONN_ATTR_IPCDA_FILEPATH:
            ACI_TEST_RAISE(ulnDbcSetIPCDAFilepath(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
        case ULN_CONN_ATTR_SOCK_RCVBUF_BLOCK_RATIO:
            ACI_TEST(ulnDbcSetAttrSockRcvBufBlockRatio(aFnContext,
                                                       (acp_uint32_t)sValue)
                    != ACI_SUCCESS);
            break;

        /* BUG-44271 */
        case ULN_CONN_ATTR_SOCK_BIND_ADDR:
            ACI_TEST_RAISE(ulnDbcSetSockBindAddr(sDbc, (acp_char_t *)aValuePtr, aLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEMORY);
            break;

        /* BUG-45286 */
        case ULN_CONN_ATTR_PDO_DEFER_PROTOCOLS:
            sDbc->mAttrPDODeferProtocols = (acp_uint32_t)sValue;
            break;

        /* PROJ-2638 shard native linker */
        case ULN_CONN_ATTR_SHARD_LINKER_TYPE:
            sDbc->mShardDbcCxt.mShardLinkerType = (acp_uint8_t)sValue;
            break;

        /* PROJ-2660 hybrid sharding */
        case ULN_CONN_ATTR_SHARD_PIN:
            /* For Sharding. */
            /* Do Nothing. */
            break;

        /* BUG-46090 Meta Node SMN 전파 */
        case ULN_CONN_ATTR_SHARD_META_NUMBER:
            ulnDbcSetShardMetaNumber( sDbc, (acp_uint64_t)sValue );

            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_SHARD_META_NUMBER,
                                          (void*)sValue )
                      != ACI_SUCCESS );

            ulnDbcSetSentShardMetaNumber( sDbc, (acp_uint64_t)sValue );
            break;
 
        /* PROJ-2638 shard native linker */
        case ULN_CONN_ATTR_SHARD_NODE_NAME:
            ACI_TEST_RAISE( aLength > ULSD_MAX_NODE_NAME_LEN, LABEL_NOT_ENOUGH_MEMORY );
            acpMemCpy( sDbc->mShardDbcCxt.mShardTargetDataNodeName,
                       (acp_char_t *) aValuePtr,
                       aLength );
            sDbc->mShardDbcCxt.mShardTargetDataNodeName[aLength] = '\0';
            break;

        case ULN_CONN_ATTR_SHARD_CONNTYPE:
            if ( ulnDbcGetShardConnType( sDbc ) == ULN_CONNTYPE_INVALID )
            {
                ACI_TEST( ulnDbcSetShardConnType( sDbc, (ulnConnType)sValue )
                          != ACI_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            break;

        /* PROJ-2681 */
        case ULN_CONN_ATTR_IB_LATENCY:
            ulnDbcSetIBLatency(sDbc, (acp_sint32_t)sValue);
            break;

        case ULN_CONN_ATTR_IB_CONCHKSPIN:
            ulnDbcSetIBConChkSpin(sDbc, (acp_sint32_t)sValue);
            break;

        /* BUG-45707 */
        case ULN_CONN_ATTR_SHARD_CLIENT:
            /* For Sharding. */
            /* Do Nothing. */
            break;

        case ULN_CONN_ATTR_SHARD_SESSION_TYPE:
            sDbc->mShardDbcCxt.mShardSessionType = (acp_uint8_t)sValue;
            break;

        /* BUG-47257 */
        case ULN_CONN_ATTR_GLOBAL_TRANSACTION_LEVEL:
            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_GLOBAL_TRANSACTION_LEVEL,
                                          (void*)sValue )
                      != ACI_SUCCESS );
            sDbc->mAttrGlobalTransactionLevel = (acp_uint8_t)sValue;
            break;
            
            // PROJ-2727 add connect attr
        case ULN_CONN_ATTR_COMMIT_WRITE_WAIT_MODE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_COMMIT_WRITE_WAIT_MODE,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mCommitWriteWaitMode = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_ST_OBJECT_BUFFER_SIZE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_ST_OBJECT_BUFFER_SIZE,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mSTObjBufSize = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_TRX_UPDATE_MAX_LOGSIZE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_TRX_UPDATE_MAX_LOGSIZE,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mUpdateMaxLogSize = (acp_uint64_t)sValue;
            break;

        case ULN_CONN_ATTR_PARALLEL_DML_MODE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_PARALLEL_DML_MODE,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mParallelDmlMode = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_NLS_NCHAR_CONV_EXCP:
                        ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_NLS_NCHAR_CONV_EXCP,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mNlsNcharConvExcp = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_AUTO_REMOTE_EXEC:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_AUTO_REMOTE_EXEC,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mAutoRemoteExec = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_TRCLOG_DETAIL_PREDICATE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_TRCLOG_DETAIL_PREDICATE,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mTrclogDetailPredicate = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mOptimizerDiskIndexCostAdj = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ,
                                         (void *)sValue )
                     != ACI_SUCCESS);
            sDbc->mOptimizerMemoryIndexCostAdj = (acp_uint32_t)sValue;
            break;

            //
        case ULN_CONN_ATTR_QUERY_REWRITE_ENABLE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_QUERY_REWRITE_ENABLE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mQueryRewriteEnable = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mDblinkRemoteStatementAutoCommit = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_RECYCLEBIN_ENABLE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_RECYCLEBIN_ENABLE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mRecyclebinEnable = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR___USE_OLD_SORT:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___USE_OLD_SORT,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mUseOldSort = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_ARITHMETIC_OPERATION_MODE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_ARITHMETIC_OPERATION_MODE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mArithmeticOpMode = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_RESULT_CACHE_ENABLE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_RESULT_CACHE_ENABLE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mResultCacheEnable = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_TOP_RESULT_CACHE_MODE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_TOP_RESULT_CACHE_MODE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mTopResultCacheMode = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_OPTIMIZER_AUTO_STATS:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_OPTIMIZER_AUTO_STATS,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mOptimizerAutoStats = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR___OPTIMIZER_TRANSITIVITY_OLD_RULE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mOptimizerTransitivityOldRule = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_OPTIMIZER_PERFORMANCE_VIEW:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mOptimizerPerformanceView = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_REPLICATION_DDL_SYNC:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_REPLICATION_DDL_SYNC,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mReplicationDDLSync = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_REPLICATION_DDL_SYNC_TIMEOUT:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mReplicationDDLSyncTimeout = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR___PRINT_OUT_ENABLE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___PRINT_OUT_ENABLE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mPrintOutEnable = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_TRCLOG_DETAIL_SHARD:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_TRCLOG_DETAIL_SHARD,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mTrclogDetailShard = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_SERIAL_EXECUTE_MODE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_SERIAL_EXECUTE_MODE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mSerialExecuteMode = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR_TRCLOG_DETAIL_INFORMATION:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_TRCLOG_DETAIL_INFORMATION,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mTrcLogDetailInformation = (acp_uint32_t)sValue;
            break;
            
        case ULN_CONN_ATTR___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mOptimizerDefaultTempTbsType = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_NORMALFORM_MAXIMUM:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_NORMALFORM_MAXIMUM,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mNormalFormMaximum = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR___REDUCE_PARTITION_PREPARE_MEMORY:
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mReducePartPrepareMemory = (acp_uint32_t)sValue;
            break;
            
            // STRING
        case ULN_CONN_ATTR_NLS_TERRITORY:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            ACI_TEST_RAISE(ulnDbcSetNlsTerriroty( sDbc,
                                                  (acp_char_t *)aValuePtr,
                                                  aLength )
                           != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_NLS_TERRITORY,
                                         sDbc->mNlsTerritory )
                     != ACI_SUCCESS );
            break;
            
        case ULN_CONN_ATTR_NLS_ISO_CURRENCY:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            ACI_TEST_RAISE(ulnNlsISOCurrency( sDbc,
                                              (acp_char_t *)aValuePtr,
                                              aLength)
                           != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_NLS_ISO_CURRENCY,
                                         sDbc->mNlsISOCurrency )
                     != ACI_SUCCESS );
            break;
            
        case ULN_CONN_ATTR_NLS_CURRENCY:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            ACI_TEST_RAISE(ulnNlsCurrency( sDbc,
                                           (acp_char_t *)aValuePtr,
                                           aLength)
                           != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_NLS_CURRENCY,
                                         sDbc->mNlsCurrency )
                     != ACI_SUCCESS );
            break;
            
        case ULN_CONN_ATTR_NLS_NUMERIC_CHARACTERS:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            ACI_TEST_RAISE(ulnNlsNumChar( sDbc,
                                          (acp_char_t *)aValuePtr,
                                          aLength)
                           != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY_NLS_NUMERIC_CHARACTERS,
                                         sDbc->mNlsNumChar )
                     != ACI_SUCCESS );
            break;
           
        case ULN_CONN_ATTR_TRANSACTIONAL_DDL:
            ACI_TEST( ulnSendConnectAttr( aFnContext, 
                                          ULN_PROPERTY_TRANSACTIONAL_DDL, 
                                          (void *)sValue ) 
                      != ACI_SUCCESS );
            sDbc->mTransactionalDDL = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_SHARD_INTERNAL_LOCAL_OPERATION:
            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_SHARD_INTERNAL_LOCAL_OPERATION,
                                          (void*)sValue )
                != ACI_SUCCESS );
            break;

        case ULN_CONN_ATTR_INVOKE_USER:
            ACI_TEST_RAISE(aValuePtr == NULL, ERR_HY090);
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_INVOKE_USER, (void *)sValue) != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_GLOBAL_DDL:
            ACI_TEST(ulnSendConnectAttr(aFnContext, ULN_PROPERTY_GLOBAL_DDL, (void *)sValue) != ACI_SUCCESS);
            sDbc->mGlobalDDL = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_SHARD_STATEMENT_RETRY:
            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_SHARD_STATEMENT_RETRY,
                                          (void*)sValue )
                      != ACI_SUCCESS );
            ulnDbcSetShardStatementRetry( sDbc, (acp_uint8_t)sValue );
            break;

        case ULN_CONN_ATTR_INDOUBT_FETCH_TIMEOUT:
            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_INDOUBT_FETCH_TIMEOUT,
                                          (void*)sValue )
                      != ACI_SUCCESS );
            ulnDbcSetIndoubtFetchTimeout( sDbc, (acp_uint32_t)sValue );
            break;
 
        case ULN_CONN_ATTR_INDOUBT_FETCH_METHOD:
            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_INDOUBT_FETCH_METHOD,
                                          (void*)sValue )
                      != ACI_SUCCESS );
            ulnDbcSetIndoubtFetchMethod( sDbc, (acp_uint8_t)sValue );
            break;

        case ULN_CONN_ATTR_SHARD_COORD_FIX_CTRL_CALLBACK:
            ACI_TEST_RAISE( sDbc->mShardDbcCxt.mShardSessionType != ULSD_SESSION_TYPE_COORD,
                            LABEL_CANNOT_BE_SET_NOW );
            ulnDbcSetShardCoordFixCtrlContext( sDbc, (ulnShardCoordFixCtrlContext*) aValuePtr);
            break;

        case ULN_CONN_ATTR___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD: /* BUG-48132 */
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mPlanHashOrSortMethod = (acp_uint32_t)sValue;
            break;
        case ULN_CONN_ATTR___OPTIMIZER_BUCKET_COUNT_MAX: /* BUG-48161 */
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mBucketCountMax = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_DDL_LOCK_TIMEOUT:
            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_DDL_LOCK_TIMEOUT,
                                          (void*)sValue )
                      != ACI_SUCCESS );
            ulnDbcSetDDLLockTimeout( sDbc, (acp_uint32_t)sValue );
            break;
        case ULN_CONN_ATTR___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION: /* BUG-48348 */ 
            ACI_TEST(ulnSendConnectAttr( aFnContext,
                                         ULN_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION,
                                         (void *)sValue )
                     != ACI_SUCCESS );
            sDbc->mEliminateCommonSubexpression = (acp_uint32_t)sValue;
            break;

        case ULN_CONN_ATTR_REBUILD_SHARD_META_NUMBER:
            ACI_TEST( ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_REBUILD_SHARD_META_NUMBER,
                                          (void*)sValue )
                      != ACI_SUCCESS );

            ulnDbcSetSentRebuildShardMetaNumber( sDbc, (acp_uint64_t)sValue );
            break;

            
        /*
         * Warning : 제품 릴리즈 이후 추가되는 서버-클라이언트 프로퍼티는 반드시
         *           ulnSetConnectAttrOff()에 추가되어야 한다.
         */

        default:
            if (aConnAttrID >= ULN_CONN_ATTR_MAX)
            {
                ACE_ASSERT(0);
            }
            else
            {
                /* nothing to do */
            }
            break;

        /*
         * Note : compiler warning 이 나지 않아야 한다. 만약 warning 나면 뭔가 하나 빠진것임.
         */
    }

    ACI_EXCEPTION_CONT(NO_NEED_WORK)

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CANNOT_BE_SET_NOW)
    {
        ulnError(aFnContext, ulERR_ABORT_ATTRIBUTE_CANNOT_BE_SET_NOW);
    }
    ACI_EXCEPTION(ERR_HY090);
    {
        ulnError(aFnContext, ulERR_ABORT_Invalid_StrLen_IndPtr_Error);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEMORY)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnSetConnAttr");
    }
    ACI_EXCEPTION(LABEL_CHARACTERSET_NOT_FOUND)
    {
        ulnError(aFnContext, ulERR_ABORT_idnSetDefaultFactoryError, (acp_char_t *)aValuePtr);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/*
 * Note :
 *      ulnSetConnAttrByConnString() 에서 호출되는 ulnSetConnAttrById
 *          NTS guarantee    : OK. 항상 NTS
 *          aAttrValueLength : 항상 string 의 길이. 양수 혹은 0
 *
 *      ulnSetConnAttrByProfileFunc() 에서 호출되는 ulnSetConnAttrById
 *          NTS guarantee    : ?. NTS 인지 아닌지 odbc 문서에서 이야기 안함.
 *          aAttrValueLength : 항상 string 의 길이. 양수. ??
 */
/*
 * BUGBUG : ulnConnAttrProcess*** 함수들은
 *              1. aAttrValue 에 NULL 포인터가 안들어온다는 것.
 *              2. aAttrValue 에 NTS 가 들어온다는 것
 *              3. aAttrValueLength 에 양수의 유효한 값이 들어온다는 것
 *          을 반드시 보장할 것.
 */
static ACI_RC ulnConnAttrProcessIntAttr(ulnFnContext  *aFnContext,
                                        ulnConnAttrID  aConnAttr,
                                        acp_char_t    *aAttrValue,
                                        acp_sint32_t   aAttrValueLength)
{
    ulnMetricPrefixInt *sAllowedMultiplicator;
    acp_sint32_t        sPosLen;
    acp_char_t         *sCurr = aAttrValue;
    acp_sint32_t        sSign             = 1;
    acp_uint64_t        sSel              = 0;
    acp_sint64_t        sValue;

    /*
     * BUG-28980 [CodeSonar]Null Pointer Dereference
     */
    ACI_TEST_RAISE((aAttrValue == NULL) || (aAttrValueLength < 0), LABEL_INVALID_ATTRIBUTE_VALUE);

    ACI_TEST_RAISE(acpCStrToInt64(aAttrValue,
                                  aAttrValueLength,
                                  &sSign,
                                  &sSel,
                                  10,
                                  &sCurr) != ACP_RC_SUCCESS,
                   LABEL_INVALID_ATTRIBUTE_VALUE);

    sValue = (acp_sint64_t)sSel * sSign; /* BUG-44151 */

    /* K, M, G, sec, hours와 같은 단위 변환 문자 처리 */
    sPosLen = ulnParseIndexOfNonWhitespace(aAttrValue, aAttrValueLength, (acp_sint32_t)(sCurr - aAttrValue));
    if (sPosLen < aAttrValueLength)
    {
        sCurr = aAttrValue + sPosLen;
        sPosLen = aAttrValueLength - sPosLen;

        sAllowedMultiplicator = (ulnMetricPrefixInt *)gUlnConnAttrTable[aConnAttr].mCheck;
        ACI_TEST_RAISE(sAllowedMultiplicator == NULL, LABEL_NOT_IMPLEMENTED);

        for (; sAllowedMultiplicator->mKey != NULL; sAllowedMultiplicator++)
        {
            if (acpCStrCmp(sAllowedMultiplicator->mKey, sCurr, sPosLen) == 0 &&
                sAllowedMultiplicator->mKey[sPosLen] == '\0')
            {
                break;
            }
        }
        ACI_TEST_RAISE(sAllowedMultiplicator->mKey == NULL, LABEL_INVALID_ATTRIBUTE_VALUE);

        sValue *= sAllowedMultiplicator->mValue;
    }

    /*
     * Note : 범위 체크는 ulnSetConnAttr 에서 한다.
     */

    /*
     * BUG-28980 [CodeSonar]strncpy Length Unreasonable
     * aAttrValueLength는 사용될 일이 없으나 경고 제거를 위해
     */
    ACI_TEST(ulnSetConnAttr(aFnContext,
                            aConnAttr,
                            (void *)(acp_slong_t)sValue,
                            ACI_SIZEOF(acp_slong_t)) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_IMPLEMENTED)
    {
        /* HYC00 : Optional feature not implemented */
        ulnError(aFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION(LABEL_INVALID_ATTRIBUTE_VALUE)
    {
        /*
         * HY024 : Invalid attribute value
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_ATTRIBUTE_VALUE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnConnAttrProcessDiscreteIntAttr(ulnFnContext  *aFnContext,
                                                ulnConnAttrID  aConnAttr,
                                                acp_char_t    *aAttrValue,
                                                acp_sint32_t   aAttrValueLength)
{
    ulnDiscreteInt *sAllowedPair;
    ulvSLen         sValue;

    sAllowedPair = (ulnDiscreteInt *)gUlnConnAttrTable[aConnAttr].mCheck;

    ACI_TEST_RAISE(sAllowedPair == NULL, LABEL_NOT_IMPLEMENTED);

    while (sAllowedPair->mKey != NULL)
    {
        if ( (0 == acpCStrCaseCmp(sAllowedPair->mKey, aAttrValue, aAttrValueLength)) &&
             (sAllowedPair->mKey[aAttrValueLength] == '\0') )
        {
            break;
        }

        sAllowedPair++;
    }

    /*
     * 설정 가능한 키워드 (ulnConnAttributre.cpp 파일의 테이블들에서 정의해 둔 키워드) 가
     * 아니면, ULN_CONN_ATTR_INVALID_VALUE 가 sValue 에 세팅됨.
     *
     * 에러는 ulnSetConnAttr() 함수에서 범위 체크하면서 발생.
     */

    sValue = sAllowedPair->mValue;
    /*
     * BUG-28980 [CodeSonar]strncpy Length Unreasonable
     * aAttrValueLength는 사용될 일이 없으나 경고 제거를 위해
     */
    ACI_TEST(ulnSetConnAttr(aFnContext,
                            aConnAttr,
                            (void *)(acp_slong_t)sValue,
                            ACI_SIZEOF(acp_slong_t)) != ACI_SUCCESS);

    /*
     * Note : 범위 체크는 ulnSetConnAttr() 에서 한다.
     */

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_IMPLEMENTED)
    {
        /* HYC00 : Optional feature not implemented */
        ulnError(aFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}






static ACI_RC ulnConnAttrProcessStringAttr(ulnFnContext  *aFnContext,
                                           ulnConnAttrID  aConnAttr,
                                           acp_char_t    *aAttrValue,
                                           acp_sint32_t   aLength)
{
    ulnReservedString *sReservedWordList;

    sReservedWordList = (ulnReservedString *)gUlnConnAttrTable[aConnAttr].mCheck;

    ACI_TEST_RAISE(sReservedWordList == NULL, LABEL_NOT_IMPLEMENTED);

    while (sReservedWordList->mKey != NULL)
    {
        if (aAttrValue != NULL)
        {
            if (0 == acpCStrCmp(aAttrValue, sReservedWordList->mKey, aLength) &&
                sReservedWordList->mKey[aLength] == '\0')
            {
                break;
            }
        }

        sReservedWordList++;
    }

    /* set up value   */
    if (sReservedWordList->mKey == NULL)
    {
        /*
         * 매치되는 문자열이 없어서 입력받은 문자열을 그대로 세팅할려고 하는데,
         * 입력받은 문자열이 NULL 이나 "" 이면, NULL 을 세팅한다.
         */
        if (aAttrValue != NULL)
        {
            if (*aAttrValue == 0)
            {
                aAttrValue = NULL;
            }
        }
    }
    else
    {
        /*
         * 매치되는 문자열이 있어서 확장을 한다.
         */
        aAttrValue = (acp_char_t *)(sReservedWordList->mValue);
    }

    ACI_TEST(ulnSetConnAttr(aFnContext,
                            aConnAttr,
                            aAttrValue,
                            aLength) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_IMPLEMENTED)
    {
        /* HYC00 : Optional feature not implemented */
        ulnError(aFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnConnAttrProcessUpperCaseStringAttr(ulnFnContext  *aFnContext,
                                                    ulnConnAttrID  aConnAttr,
                                                    acp_char_t    *aAttrValue,
                                                    acp_sint32_t   aLength)
{
    ulnDbc          *sDbc;       //BUG-22684
    acp_char_t      *sTempAttrValue = NULL;
    acp_char_t      *sIndex;
    acp_char_t      *sFence;
    const mtlModule *sDefaultModule;// = mtlDefaultModule();

    //BUG-22684
    // Connection이 이루어지기 전에 함수가 호출될 경우에는 sDbs가 NULL이기 때문에 이때는 default Module을 사용하도록 한다.
    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    ACI_TEST( sDbc == NULL );           //BUG-28623 [CodeSonar]Null Pointer Dereference

    if ( sDbc->mClientCharsetLangModule == NULL )
    {
        sDefaultModule = mtlDefaultModule();
    }
    else
    {
        sDefaultModule = sDbc->mClientCharsetLangModule;
    }

    if (aLength > 0)
    {
        ACI_TEST_RAISE(acpMemAlloc((void**)&sTempAttrValue, aLength + 1) != ACP_RC_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);

        // PRJ-1678 : For multi-byte character set strings
        acpCStrCpy(sTempAttrValue, aLength + 1,
                   aAttrValue, aLength);

        sIndex = sTempAttrValue;
        sFence = sIndex + aLength;
        for(;sIndex < sFence;)
        {
            *sIndex = acpCharToUpper(*sIndex);
            ACI_TEST_RAISE(sDefaultModule->nextCharPtr((acp_uint8_t**)&sIndex,
                                                       (acp_uint8_t*)sFence)
                           != NC_VALID,
                           ERR_INVALID_CHARACTER );
        }
        *sFence = 0;
    }

    ACI_TEST(ulnConnAttrProcessStringAttr(aFnContext,
                                          aConnAttr,
                                          sTempAttrValue,
                                          aLength) != ACI_SUCCESS);

    if (sTempAttrValue != NULL)
    {
        acpMemFree(sTempAttrValue);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnConnAttrProcessUpperCaseStringAttr");
    }

    ACI_EXCEPTION(ERR_INVALID_CHARACTER)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_INVALID_CHARACTER);
    }

    ACI_EXCEPTION_END;

    if (sTempAttrValue != NULL)
    {
        acpMemFree(sTempAttrValue);
    }

    return ACI_FAILURE;
}


ACI_RC ulnSetConnAttrById(ulnFnContext  *aFnContext,
                          ulnConnAttrID  aConnAttr,
                          acp_char_t    *aAttrValueString,
                          acp_sint32_t   aAttrValueStringLength)
{
    switch (gUlnConnAttrTable[aConnAttr].mAttrType)
    {
        case ULN_CONN_ATTR_TYPE_INT:
            ACI_TEST(ulnConnAttrProcessIntAttr(aFnContext,
                                               aConnAttr,
                                               aAttrValueString,
                                               aAttrValueStringLength) != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_TYPE_DISCRETE_INT:
            ACI_TEST(ulnConnAttrProcessDiscreteIntAttr(aFnContext,
                                                       aConnAttr,
                                                       aAttrValueString,
                                                       aAttrValueStringLength) != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_TYPE_STRING:
            ACI_TEST(ulnConnAttrProcessStringAttr(aFnContext,
                                                  aConnAttr,
                                                  aAttrValueString,
                                                  aAttrValueStringLength) != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_TYPE_UPPERCASE_STRING:
            ACI_TEST(ulnConnAttrProcessUpperCaseStringAttr(aFnContext,
                                                           aConnAttr,
                                                           aAttrValueString,
                                                           aAttrValueStringLength) != ACI_SUCCESS);
            break;

        case ULN_CONN_ATTR_TYPE_CALLBACK:
        case ULN_CONN_ATTR_TYPE_POINTER:
            /*
             * Note : 포인터형 attribute (callback 등) 는 connection string 이나
             *        profile string 으로 세팅할 수 없다.
             *        오직 SQLSetConnectAttr() 로만 세팅할 수 있으므로 이 함수를 타지 않는다.
             */
            ACI_RAISE(LABEL_MEM_MAN_ERR);
            break;

        default:
            ACE_ASSERT(0);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnSetConnAttrById");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t ulnErrorConnStrPairIgnored( ulnFnContext     *aFnContext,
                                         const acp_char_t *aKey,
                                         acp_sint32_t      aKeyLen,
                                         const acp_char_t *aVal,
                                         acp_sint32_t      aValLen )
{
    #define ULN_CONNATTR_BUF_LEN 40

    acp_char_t sKey[ULN_CONNATTR_BUF_LEN + 1] = {'\0', };
    acp_char_t sVal[ULN_CONNATTR_BUF_LEN + 1] = {'\0', };

    acpCStrCpy(sKey, ACI_SIZEOF(sKey), aKey, aKeyLen);
    if (aKeyLen > ULN_CONNATTR_BUF_LEN)
    {
        /*
         * this code appends "..." in the end of string not the beginning.(BUG-39656)
         */
        acpCStrCpy( (sKey + ULN_CONNATTR_BUF_LEN + 1) - 5, 4, " ...", 4);
    }
    if (aVal != NULL)
    {
        acpCStrCpy(sVal, ACI_SIZEOF(sVal), aVal, aValLen);
        if (aValLen > ULN_CONNATTR_BUF_LEN)
        {
            acpCStrCpy( (sVal + ULN_CONNATTR_BUF_LEN + 1)- 5, 4, " ...", 4);
        }
    }
    else
    {
        acpCStrCpy(sVal, ACI_SIZEOF(sVal), "null", 4);
    }

    return ulnError(aFnContext, ulERR_IGNORE_CONNECTION_STR_IGNORED, sKey, sVal);

    #undef ULN_CONNATTR_BUF_LEN
}

acp_sint32_t ulnCallbackSetConnAttr( void                         *aContext,
                                     ulnConnStrParseCallbackEvent  aEvent,
                                     acp_sint32_t                  aPos,
                                     const acp_char_t             *aKey,
                                     acp_sint32_t                  aKeyLen,
                                     const acp_char_t             *aVal,
                                     acp_sint32_t                  aValLen,
                                     void                         *aFilter )
{
    ulnFnContext  *sFnContext = (ulnFnContext *) aContext;
    ulnConnAttrID  sFilterID;
    ulnConnAttrID  sConnAttrID;
    acp_sint32_t   sRC = 0;

    switch (aEvent)
    {
        case ULN_CONNSTR_PARSE_EVENT_GO:
            sFilterID = (ulnConnAttrID) aFilter;
            sConnAttrID = ulnGetConnAttrIDfromKEYWORD(aKey, aKeyLen);
            if ( (sFilterID == ULN_CONN_ATTR_MAX) || (sFilterID == sConnAttrID) )
            {
                if (sConnAttrID <  ULN_CONN_ATTR_MAX)
                {
                    ACI_TEST( ulnSetConnAttrById(sFnContext, sConnAttrID, (acp_char_t *)aVal, aValLen)
                              != ACI_SUCCESS );
                    sRC |= ULN_CONNSTR_PARSE_RC_SUCCESS;
                }
                else
                {
                    ACI_TEST( ulnErrorConnStrPairIgnored(sFnContext, aKey, aKeyLen, aVal, aValLen)
                              != ACI_SUCCESS );
                }

                if (sFilterID != ULN_CONN_ATTR_MAX)
                {
                    sRC |= ULN_CONNSTR_PARSE_RC_STOP;
                }
            }
            break;

        case ULN_CONNSTR_PARSE_EVENT_IGNORE:
            ACI_TEST( ulnErrorConnStrPairIgnored(sFnContext, aKey, aKeyLen, aVal, aValLen)
                      != ACI_SUCCESS );
            break;

        case ULN_CONNSTR_PARSE_EVENT_ERROR:
            /* 참조 문자가 없으면 '?'로 */
            ACI_TEST( ulnError(sFnContext, ulERR_ABORT_INVALID_CONNECTION_STR_FORM,
                               aPos, (aKey == NULL) ? '?' : aKey[0])
                      != ACI_SUCCESS );
            break;

        default:
            ACE_ASSERT(0);
            break;
    }

    return sRC;

    ACI_EXCEPTION_END;

    return ULN_CONNSTR_PARSE_RC_ERROR;
}

#if defined(ALTIBASE_ODBC)

/* ------------------------------------------------
 *  ODBC
 * ----------------------------------------------*/

ACI_RC ulnSetConnAttrByProfileFunc(ulnFnContext  *aFnContext,
                                   acp_char_t    *aDSNString,
                                   acp_char_t    *aResourceName)
{
    ulnConnAttrID sConnAttr;

    acp_char_t    sBuffer[1024];
    acp_sint32_t  i;
    acp_sint32_t  sSize;

    /* Check The Resource for all aliases */
    for (i = 0; gUlnConnAttrMap_PROFILE[i].mKey != NULL; i++)
    {
        /*
         * unix-odbc
         *      setupSQLGetPrivateProfileString() 에서 dlsym() 을 통해 얻어온 함수를 호출.
         *
         * sqlcli
         *      이 함수 컴파일 안됨.
         *      이 함수 대신 아래쪽의 아무것도 안하는 버젼이 컴파일됨.
         *
         * 자세한 것은 gSQLConnectModule 을 tag 로 찍어보도록.
         */
        sSize = gPrivateProfileFuncPtr(aDSNString,
                                       (acp_char_t *)(gUlnConnAttrMap_PROFILE[i].mKey),
                                       (acp_char_t *)"",
                                       sBuffer,
                                       ACI_SIZEOF(sBuffer),
                                       aResourceName);

        if (sSize > 0)
        {
            sConnAttr = gUlnConnAttrMap_PROFILE[i].mConnAttrID;

            /*
             * BUGBUG : null-terminating 하는 부분이 주석처리되어 있었음.
             *          SQLGetPrivateProfileString() 이 null terminated string 을 준다는 보장이...
             *          문서에 읽어봐도 언급이 없던데...
             */

            ACI_TEST(ulnSetConnAttrById(aFnContext,
                                        sConnAttr,
                                        sBuffer,
                                        sSize) != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
#else /* if defined(ALTIBASE_ODBC) */

/* ------------------------------------------------
 *  ODBCCLI
 * ----------------------------------------------*/

ACI_RC ulnSetConnAttrByProfileFunc(ulnFnContext *aFnContext,
                                   acp_char_t   *aDSNString,
                                   acp_char_t   *aResourceName)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDSNString);
    ACP_UNUSED(aResourceName);

    // nothing to do in cli

    return ACI_SUCCESS;
}
#endif /* if defined(ALTIBASE_ODBC) */

ACI_RC ulnSetConnAttrBySQLConstant(ulnFnContext *aFnContext,
                                   acp_sint32_t  aSQLConstant,
                                   void         *aValuePtr,
                                   acp_sint32_t  aLength)
{
    ulnConnAttrID sConnAttr;
    acp_sint32_t  sRealLength;

    /*
     * ulnConnAttrID 얻어오기
     */

    sConnAttr = ulnGetConnAttrIDfromSQL_ATTR_ID(aSQLConstant);

    ACI_TEST_RAISE(sConnAttr == ULN_CONN_ATTR_MAX, LABEL_INVALID_ATTRIBUTE);

    if (gUlnConnAttrTable[sConnAttr].mAttrType == ULN_CONN_ATTR_TYPE_STRING ||
        gUlnConnAttrTable[sConnAttr].mAttrType == ULN_CONN_ATTR_TYPE_UPPERCASE_STRING)
    {
        /*
         * Note : SQLSetConnectAttr() 에서도 string 으로 세팅하는 속성들은
         *        공통되는 길이, reserved word 체크 등을 수행하기 위해서
         *        ulnSetConnAttrById() 함수를 통해서 ulnSetConnAttr() 을 호출해야 한다.
         *
         * sRealLength 는 음수여서는 안됨.
         */
        ACI_TEST_RAISE(aValuePtr == NULL, LABEL_INVALID_USE_OF_NULL);

        sRealLength = ulnAttrLength(aValuePtr, aLength);
        ACI_TEST_RAISE(sRealLength < 0, LABEL_INVALID_STRLEN);

        ACI_TEST(ulnSetConnAttrById(aFnContext,
                                    sConnAttr,
                                    (acp_char_t *)aValuePtr,
                                    sRealLength) != ACI_SUCCESS);
    }
    else
    {
        /*
         * string 이 아닌 일반적인 connection attribute 세팅하기
         */
        ACI_TEST(ulnSetConnAttr(aFnContext,
                                sConnAttr,
                                aValuePtr,
                                aLength) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL);
    {
        /* HY009 : Invalid use of null pointer */
        ulnError(aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION(LABEL_INVALID_STRLEN)
    {
        /* HY090 */
        ulnError(aFnContext, ulERR_ABORT_Invalid_StrLen_IndPtr_Error);
    }

    ACI_EXCEPTION(LABEL_INVALID_ATTRIBUTE)
    {
        /*
         * HY092 : Invalid attribute/option identifier
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aSQLConstant);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/*
 * Note : 64 비트 odbc 고려
 *
 * When the Attribute parameter has one of the following values,
 * a 64-bit value is passed in Value:
 *      SQL_ATTR_QUIET_MODE  --> 사용하지 않으므로 무시
 *
 * When the Option parameter has one of the following values,
 * a 64-bit value is passed in *Value:
 *      SQL_MAX_LENGTH
 *      SQL_MAX_ROWS
 *      SQL_ROWSET_SIZE
 *      SQL_KEYSET_SIZE
 *
 *      상기 모든 연결 속성들을 현재는 쓰지 않음.
 */

SQLRETURN ulnSetConnectAttr(ulnDbc *aDbc, acp_sint32_t aSQL_ATTR_ID, void *aValuePtr, acp_sint32_t aLength)
{
    ULN_FLAG(sNeedExit);
    ulnFnContext sFnContext;

    ulnDbc *sDbc;

    sDbc = ulnDbcSwitch(aDbc);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETCONNECTATTR, sDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST(ulnEnter(&sFnContext, (void *)(&aSQL_ATTR_ID)) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedExit);

    /*
     * =====================
     * Function main
     * =====================
     */

    ACI_TEST(ulnSetConnAttrBySQLConstant(&sFnContext,
                                         aSQL_ATTR_ID,
                                         aValuePtr,
                                         aLength) != ACI_SUCCESS);

    /*
     * =====================
     * Function end
     * =====================
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| [%2"ACI_INT32_FMT" ptr: %p]",
            "ulnSetConnectAttr", aSQL_ATTR_ID, aValuePtr);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [%2"ACI_INT32_FMT" ptr: %p] fail",
            "ulnSetConnectAttr", aSQL_ATTR_ID, aValuePtr);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

ACI_RC ulnCallbackDBPropertySetResult(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aServiceSession,
                                      void               *aUserContext)
{
    cmiProtocolContext        *sCtx       = aProtocolContext;
    ulnFnContext              *sFnContext = (ulnFnContext *)aUserContext;
    ulnDbc                    *sDbc       = NULL;
    acp_uint16_t               sPropertyID;
    acp_uint64_t               sSCN       = 0;
    acp_uint8_t                sGTxLevel  = ACP_UINT8_MAX;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    /* PROJ-2733-Protocol */
    CMI_RD2(sCtx, &sPropertyID);

    ULN_FNCONTEXT_GET_DBC( sFnContext, sDbc );

    switch (sPropertyID)
    {
        case CMP_DB_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
            CMI_RD1(sCtx, sGTxLevel);
            CMI_RD8(sCtx, &sSCN);

            /* PROJ-2733-DistTxInfo */
            if (ulsdIsGCTx(sGTxLevel) == ACP_TRUE)
            {
                if (sSCN > 0)
                {
                    ulsdUpdateSCN(sDbc, &sSCN);
                }
            }
            break;

        default:
            /* non-reachable */
            ACI_TEST_RAISE(sPropertyID >= CMP_DB_PROPERTY_MAX, LABEL_INVALID_ATTR_VALUE);  
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_ATTR_VALUE);
    {
        ulnError(sFnContext, ulERR_ABORT_INVALID_ATTRIBUTE_VALUE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * BUG-36256 Improve property's communication
 *
 * ulnSetConnectAttrOff
 *
 * @aDbc        : Object
 * @aPropertyID : Property ID
 *
 * aPropertyID를 Off 한다.
 * 릴리즈 이후 추가되는 서버-클라이언트 프로퍼티는 모두 추가되어야 한다.
 * (ULN_PROPERTY_LOB_CACHE_THRESHOLD는 샘플)
 *
 * tag간 하위 버전의 서버에 상위버전의 클라이언트가 접속하는 경우 서버에서
 * 지원되지 않는 프로퍼티는 강제로 꺼야 한다. 만약 반드시 지원되어야 한다면
 * INCOMPATIBLE_PROPERTY를 발생시켜야 한다. tag간에는 호환이 되도록 새로운
 * 프로퍼티는 반드시 Off 속성을 가져야 한다.
 */
ACI_RC ulnSetConnectAttrOff(ulnFnContext  *aFnContext,
                            ulnDbc        *aDbc,
                            ulnPropertyId  aPropertyID)
{
    ulnConnAttrID sConnAttrID = ULN_CONN_ATTR_MAX;

    switch (aPropertyID)
    {
        case ULN_PROPERTY_LOB_CACHE_THRESHOLD:
            sConnAttrID = ULN_CONN_ATTR_LOB_CACHE_THRESHOLD;
            aDbc->mAttrLobCacheThreshold = ULN_LOB_CACHE_THRESHOLD_MIN;
            break;

            /* BUG-39817 */
        case ULN_PROPERTY_ISOLATION_LEVEL:
            sConnAttrID = ULN_CONN_ATTR_TXN_ISOLATION;
            aDbc->mAttrTxnIsolation = ULN_ISOLATION_LEVEL_DEFAULT;
            break;

        /* PROJ-2638 shard native linker */
        case ULN_PROPERTY_SHARD_LINKER_TYPE:
            sConnAttrID = ULN_CONN_ATTR_SHARD_LINKER_TYPE;
            aDbc->mShardDbcCxt.mShardLinkerType = 0;
            break;

        /* PROJ-2638 shard native linker */
        case ULN_PROPERTY_SHARD_PIN:
            sConnAttrID = ULN_CONN_ATTR_SHARD_PIN;
            aDbc->mShardDbcCxt.mShardPin = ULSD_SHARD_PIN_INVALID;
            break;

        /* BUG-46090 Meta Node SMN 전파 */
        case ULN_PROPERTY_SHARD_META_NUMBER:
            sConnAttrID = ULN_CONN_ATTR_SHARD_META_NUMBER;
            aDbc->mShardDbcCxt.mShardMetaNumber = 0;
            break;

        /* PROJ-2638 shard native linker */
        case ULN_PROPERTY_SHARD_NODE_NAME:
            sConnAttrID = ULN_CONN_ATTR_SHARD_NODE_NAME;
            aDbc->mShardDbcCxt.mShardTargetDataNodeName[0] = '\0';
            break;

        /* BUG-45707 */
        case ULN_PROPERTY_SHARD_CLIENT:
            sConnAttrID = ULN_CONN_ATTR_SHARD_CLIENT;
            aDbc->mShardDbcCxt.mShardClient = ULSD_SHARD_CLIENT_FALSE;
            break;

        case ULN_PROPERTY_SHARD_SESSION_TYPE:
            sConnAttrID = ULN_CONN_ATTR_SHARD_SESSION_TYPE;
            aDbc->mShardDbcCxt.mShardSessionType = ULSD_SESSION_TYPE_USER;
            break;

        case ULN_PROPERTY_MESSAGE_CALLBACK:  /* BUG-46019 */
            sConnAttrID = ULN_CONN_ATTR_MESSAGE_CALLBACK;
            /* MessageCallback은 이미 설정되어 있다. */
            break;

        /* BUG-46092 */
        case ULN_PROPERTY_SHARD_CLIENT_CONNECTION_REPORT:
            sConnAttrID = ULN_CONN_ATTR_SHARD_CLIENT_CONNECTION_REPORT;
            ACI_RAISE( LABEL_ABORT_INCOMPATIBLE_PROPERTY );
            break;

        case ULN_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
            sConnAttrID = ULN_CONN_ATTR_GLOBAL_TRANSACTION_LEVEL;
            ACI_RAISE( LABEL_ABORT_INCOMPATIBLE_PROPERTY );
            break;
            
        case ULN_PROPERTY_REBUILD_SHARD_META_NUMBER:
            sConnAttrID = ULN_CONN_ATTR_REBUILD_SHARD_META_NUMBER;
            ACI_RAISE( LABEL_ABORT_INCOMPATIBLE_PROPERTY );
            break;
            
        /* Warning : 프로퍼티는 본 코멘트 위에 추가할 것.
                     제품 릴리즈 이후 추가된 System, Session 프로퍼티만 추가할 것. */

        /*
         * 새로운 프로퍼티 추가시 아래와 같은 코드는 가능한
         * 만들어지지 않아야 한다.
        case ULN_PRPOERTY_???_PROPERTY:
            sConnAttrID = ULN_CONN_ATTR_???;
            ACI_RAISE(LABEL_ABORT_INCOMPATIBLE_PROPERTY);
            break;
         */

        default:
            ACI_RAISE(LABEL_ABORT_INCOMPATIBLE_PROPERTY);
            break;
    }

    if (sConnAttrID != ULN_CONN_ATTR_MAX)
    {
        ACI_TEST_RAISE(ulnConnAttrArrAddId(&aDbc->mUnsupportedProperties,
                                           sConnAttrID),
                       LABEL_FATAL_NOT_ENOUGH_MEMORY);
    }
    else
    {
        /* nothing to do */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_INCOMPATIBLE_PROPERTY)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_INCOMPATIBLE_PROPERTY,
                 ulnGetConnAttrKEYWORDfromConnAttrId(sConnAttrID));
    }
    ACI_EXCEPTION(LABEL_FATAL_NOT_ENOUGH_MEMORY)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnSetConnectAttrOff");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// PROJ-2727
// QUERY를 이용한 connect attribute변경 시 dbc에 변경 된 attribute를 설정
// ex> SQLExecDirect(stmt, "ALTER SESSION SET OPTIMIZER_MODE = 1");
ACI_RC ulnSetConnAttributeToDbc( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt )
{
    ulnDbc      *sDbc;
    acp_uint64_t sAttributeCVal = 0;

    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );
        
    if ( ulnStmtGetStatementType( aStmt ) == ULN_STMT_SET_SESSION_PROPERTY )
    {
        if (( (ULN_FNCONTEXT_GET_RC(aFnContext)) == SQL_SUCCESS ) ||
            ( (ULN_FNCONTEXT_GET_RC(aFnContext)) == SQL_SUCCESS_WITH_INFO ))
        {
            if (( (aStmt->mAttributeID) == ULN_PROPERTY_DATE_FORMAT ) ||
                ( (aStmt->mAttributeID) == ULN_PROPERTY_TIME_ZONE ) ||
                ( (aStmt->mAttributeID) == ULN_PROPERTY_NLS_TERRITORY ) ||
                ( (aStmt->mAttributeID) == ULN_PROPERTY_NLS_ISO_CURRENCY ) ||
                ( (aStmt->mAttributeID) == ULN_PROPERTY_NLS_CURRENCY ) ||
                ( (aStmt->mAttributeID) == ULN_PROPERTY_NLS_NUMERIC_CHARACTERS ))
            {
                sDbc->mAttributeCVal = 0;
                ulnDbcSetStringAttr( &sDbc->mAttributeCStr,
                                     (acp_char_t *)aStmt->mAttributeStr,
                                     aStmt->mAttributeLen );
                sDbc->mAttributeCLen = aStmt->mAttributeLen;                
            }
            else
            {
                sAttributeCVal =  (acp_uint64_t)ulncStrToSLong(  (const acp_char_t *)aStmt->mAttributeStr,
                                                                 (acp_char_t **)NULL,
                                                                 10 );
                sDbc->mAttributeCVal = sAttributeCVal;
                sDbc->mAttributeCStr = NULL;
                sDbc->mAttributeCLen = 0;                
            }
            
            switch ( aStmt->mAttributeID )
            {
                case ULN_PROPERTY_DATE_FORMAT:
                    sDbc->mAttributeCID = ALTIBASE_DATE_FORMAT;
                    ACI_TEST_RAISE(ulnDbcSetDateFormat( sDbc,
                                                        (acp_char_t *)aStmt->mAttributeStr,
                                                        aStmt->mAttributeLen )
                                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY);
                    break;
                    
                case ULN_PROPERTY_QUERY_TIMEOUT:
                    sDbc->mAttributeCID     = SQL_ATTR_QUERY_TIMEOUT;
                    sDbc->mAttrQueryTimeout = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_DDL_TIMEOUT:
                    sDbc->mAttributeCID   = ALTIBASE_DDL_TIMEOUT;
                    sDbc->mAttrDdlTimeout = sAttributeCVal;                   
                    break;

                case ULN_PROPERTY_FETCH_TIMEOUT:
                    sDbc->mAttributeCID     = ALTIBASE_FETCH_TIMEOUT;
                    sDbc->mAttrFetchTimeout = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_UTRANS_TIMEOUT:
                    sDbc->mAttributeCID       = ALTIBASE_UTRANS_TIMEOUT;
                    sDbc->mAttrUtransTimeout = sAttributeCVal;
                    break;

                case ULN_PROPERTY_IDLE_TIMEOUT:
                    sDbc->mAttributeCID    = ALTIBASE_IDLE_TIMEOUT;
                    sDbc->mAttrIdleTimeout = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_OPTIMIZER_MODE:
                    sDbc->mAttributeCID      = ALTIBASE_OPTIMIZER_MODE;
                    sDbc->mAttrOptimizerMode = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_HEADER_DISPLAY_MODE:
                    sDbc->mAttributeCID          = ALTIBASE_HEADER_DISPLAY_MODE;
                    sDbc->mAttrHeaderDisplayMode = sAttributeCVal;
                    break;
                    
                case ULN_PROPERTY_NORMALFORM_MAXIMUM:
                    sDbc->mAttributeCID      = ALTIBASE_NORMALFORM_MAXIMUM;
                    sDbc->mNormalFormMaximum = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
                    sDbc->mAttributeCID                = ALTIBASE___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE;
                    sDbc->mOptimizerDefaultTempTbsType = sAttributeCVal;
                    break;

                case ULN_PROPERTY_COMMIT_WRITE_WAIT_MODE:
                    sDbc->mAttributeCID        = ALTIBASE_COMMIT_WRITE_WAIT_MODE;
                    sDbc->mCommitWriteWaitMode = sAttributeCVal;                    
                    break;

                case ULN_PROPERTY_ST_OBJECT_BUFFER_SIZE:
                    sDbc->mAttributeCID = ALTIBASE_ST_OBJECT_BUFFER_SIZE;
                    sDbc->mSTObjBufSize = sAttributeCVal;                    
                    break;
                
                case ULN_PROPERTY_TRX_UPDATE_MAX_LOGSIZE:
                    sDbc->mAttributeCID     = ALTIBASE_TRX_UPDATE_MAX_LOGSIZE;
                    sDbc->mUpdateMaxLogSize = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_PARALLEL_DML_MODE:
                    sDbc->mAttributeCID    = ALTIBASE_PARALLEL_DML_MODE;
                    sDbc->mParallelDmlMode = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_NLS_NCHAR_CONV_EXCP:
                    sDbc->mAttributeCID     = ALTIBASE_NLS_NCHAR_CONV_EXCP;
                    sDbc->mNlsNcharConvExcp = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_AUTO_REMOTE_EXEC:
                    sDbc->mAttributeCID   = ALTIBASE_AUTO_REMOTE_EXEC;
                    sDbc->mAutoRemoteExec = sAttributeCVal;                    
                    break;
     
                case ULN_PROPERTY_MAX_STATEMENTS_PER_SESSION:
                    sDbc->mAttributeCID                = ALTIBASE_MAX_STATEMENTS_PER_SESSION;
                    sDbc->mAttrMaxStatementsPerSession = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_TRCLOG_DETAIL_PREDICATE:
                    sDbc->mAttributeCID          = ALTIBASE_TRCLOG_DETAIL_PREDICATE;
                    sDbc->mTrclogDetailPredicate = sAttributeCVal;                    
                    break;
                                        
                case ULN_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ:
                    sDbc->mAttributeCID              = ALTIBASE_OPTIMIZER_DISK_INDEX_COST_ADJ;
                    sDbc->mOptimizerDiskIndexCostAdj = sAttributeCVal;
                    break;
                    
                case ULN_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
                    sDbc->mAttributeCID                = ALTIBASE_OPTIMIZER_MEMORY_INDEX_COST_ADJ;
                    sDbc->mOptimizerMemoryIndexCostAdj = sAttributeCVal;
                    break;
                    
                case ULN_PROPERTY_NLS_TERRITORY:
                    sDbc->mAttributeCID = ALTIBASE_NLS_TERRITORY;
                    ACI_TEST_RAISE(ulnDbcSetNlsTerriroty( sDbc,
                                                          (acp_char_t *)aStmt->mAttributeStr,
                                                          aStmt->mAttributeLen )
                                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY);
                    break;
                    
                case ULN_PROPERTY_NLS_ISO_CURRENCY:
                    sDbc->mAttributeCID = ALTIBASE_NLS_ISO_CURRENCY;
                    ACI_TEST_RAISE(ulnNlsISOCurrency( sDbc,
                                                      (acp_char_t *)aStmt->mAttributeStr,
                                                      aStmt->mAttributeLen )
                                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );
                    break;
                    
                case ULN_PROPERTY_NLS_CURRENCY:
                    sDbc->mAttributeCID = ALTIBASE_NLS_CURRENCY;
                    ACI_TEST_RAISE(ulnNlsCurrency( sDbc,
                                                   (acp_char_t *)aStmt->mAttributeStr,
                                                   aStmt->mAttributeLen )
                                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );
                    break;
                    
                case ULN_PROPERTY_NLS_NUMERIC_CHARACTERS:
                    sDbc->mAttributeCID = ALTIBASE_NLS_NUMERIC_CHARACTERS;
                    ACI_TEST_RAISE(ulnNlsNumChar( sDbc,
                                                  (acp_char_t *)aStmt->mAttributeStr,
                                                  aStmt->mAttributeLen )
                                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );
                    break;
                    
                case ULN_PROPERTY_TIME_ZONE:
                    sDbc->mAttributeCID = ALTIBASE_TIME_ZONE;
                    ACI_TEST_RAISE( ulnDbcSetTimezoneSring( sDbc,
                                                            (acp_char_t *)aStmt->mAttributeStr,
                                                            aStmt->mAttributeLen )
                                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );                    
                    break;
                    
                case ULN_PROPERTY_LOB_CACHE_THRESHOLD:
                    sDbc->mAttributeCID          = ALTIBASE_LOB_CACHE_THRESHOLD;
                    sDbc->mAttrLobCacheThreshold = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_QUERY_REWRITE_ENABLE:
                    sDbc->mAttributeCID       = ALTIBASE_QUERY_REWRITE_ENABLE;
                    sDbc->mQueryRewriteEnable = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
                    sDbc->mAttributeCID               = ALTIBASE_GLOBAL_TRANSACTION_LEVEL;
                    sDbc->mAttrGlobalTransactionLevel = sAttributeCVal;

                    /* BUG-48109 ALTER SESSION으로 GTx를 변경한 경우 ALTER SESSION 실행시 분산정보가
                                 설정되었기 때문에 Level에 상관없이 초기화 해야 한다. */
                    ulsdInitDistTxInfo( sDbc );
                    break;
                    
                case ULN_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
                    sDbc->mAttributeCID                    = ALTIBASE_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT;
                    sDbc->mDblinkRemoteStatementAutoCommit = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_RECYCLEBIN_ENABLE:
                    sDbc->mAttributeCID     = ALTIBASE_RECYCLEBIN_ENABLE;
                    sDbc->mRecyclebinEnable = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY___USE_OLD_SORT:
                    sDbc->mAttributeCID = ALTIBASE___USE_OLD_SORT;
                    sDbc->mUseOldSort   = sAttributeCVal;
                    break;

                case ULN_PROPERTY_ARITHMETIC_OPERATION_MODE:
                    sDbc->mAttributeCID     = ALTIBASE_ARITHMETIC_OPERATION_MODE;
                    sDbc->mArithmeticOpMode = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_RESULT_CACHE_ENABLE:
                    sDbc->mAttributeCID      = ALTIBASE_RESULT_CACHE_ENABLE;
                    sDbc->mResultCacheEnable = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_TOP_RESULT_CACHE_MODE:
                    sDbc->mAttributeCID       = ALTIBASE_TOP_RESULT_CACHE_MODE;
                    sDbc->mTopResultCacheMode = sAttributeCVal;                    
                    break;

                case ULN_PROPERTY_OPTIMIZER_AUTO_STATS:
                    sDbc->mAttributeCID       = ALTIBASE_OPTIMIZER_AUTO_STATS;
                    sDbc->mOptimizerAutoStats = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE:
                    sDbc->mAttributeCID                 = ALTIBASE___OPTIMIZER_TRANSITIVITY_OLD_RULE;
                    sDbc->mOptimizerTransitivityOldRule = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW:
                    sDbc->mAttributeCID             = ALTIBASE_OPTIMIZER_PERFORMANCE_VIEW;
                    sDbc->mOptimizerPerformanceView = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_REPLICATION_DDL_SYNC:
                    sDbc->mAttributeCID       = ALTIBASE_REPLICATION_DDL_SYNC;
                    sDbc->mReplicationDDLSync = sAttributeCVal;
                    break;
                    
                case ULN_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT:
                    sDbc->mAttributeCID              = ALTIBASE_REPLICATION_DDL_SYNC_TIMEOUT;
                    sDbc->mReplicationDDLSyncTimeout = sAttributeCVal;
                    break;
                    
                case ULN_PROPERTY___PRINT_OUT_ENABLE:
                    sDbc->mAttributeCID   = ALTIBASE___PRINT_OUT_ENABLE;
                    sDbc->mPrintOutEnable = sAttributeCVal;
                    break;
                    
                case ULN_PROPERTY_TRCLOG_DETAIL_SHARD:
                    sDbc->mAttributeCID      = ALTIBASE_TRCLOG_DETAIL_SHARD;
                    sDbc->mTrclogDetailShard = sAttributeCVal;
                    break;
                    
                case ULN_PROPERTY_SERIAL_EXECUTE_MODE:
                    sDbc->mAttributeCID      = ALTIBASE_SERIAL_EXECUTE_MODE;
                    sDbc->mSerialExecuteMode = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY_TRCLOG_DETAIL_INFORMATION:
                    sDbc->mAttributeCID            = ALTIBASE_TRCLOG_DETAIL_INFORMATION;
                    sDbc->mTrcLogDetailInformation = sAttributeCVal;                    
                    break;
                    
                case ULN_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY:
                    sDbc->mAttributeCID            = ALTIBASE___REDUCE_PARTITION_PREPARE_MEMORY;
                    sDbc->mReducePartPrepareMemory = sAttributeCVal;                    
                    break;
 
                case ULN_PROPERTY_TRANSACTIONAL_DDL:
                    sDbc->mAttributeCID     = ALTIBASE_TRANSACTIONAL_DDL;
                    sDbc->mTransactionalDDL = sAttributeCVal;                    
                    break;

                case ULN_PROPERTY_GLOBAL_DDL:
                    sDbc->mAttributeCID     = ALTIBASE_GLOBAL_DDL;
                    sDbc->mGlobalDDL        = sAttributeCVal;                    
                    break;

                case ULN_PROPERTY_INVOKE_USER:
                    // BUG-47862 Internal use only.
                    // Do nothing.
                    break;

                case ULN_PROPERTY_SHARD_STATEMENT_RETRY:
                    sDbc->mAttributeCID            = ALTIBASE_SHARD_STATEMENT_RETRY;
                    ulnDbcSetShardStatementRetry( sDbc, (acp_uint8_t)sAttributeCVal );
                    break;
                    
                case ULN_PROPERTY_INDOUBT_FETCH_TIMEOUT:
                    sDbc->mAttributeCID            = ALTIBASE_INDOUBT_FETCH_TIMEOUT;
                    ulnDbcSetIndoubtFetchTimeout( sDbc, (acp_uint32_t)sAttributeCVal );
                    break;

                case ULN_PROPERTY_INDOUBT_FETCH_METHOD:
                    sDbc->mAttributeCID            = ALTIBASE_INDOUBT_FETCH_METHOD;
                    ulnDbcSetIndoubtFetchMethod( sDbc, (acp_uint8_t)sAttributeCVal );
                    break;
                case ULN_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD: /* BUG-48132 */
                    sDbc->mAttributeCID         = ALTIBASE___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD;
                    sDbc->mPlanHashOrSortMethod = sAttributeCVal;
                    break;
                case ULN_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX: /* BUG-48161 */
                    sDbc->mAttributeCID   = ALTIBASE___OPTIMIZER_BUCKET_COUNT_MAX;
                    sDbc->mBucketCountMax = sAttributeCVal;
                    break;
                case ULN_PROPERTY_DDL_LOCK_TIMEOUT:
                    sDbc->mAttributeCID            = ALTIBASE_DDL_LOCK_TIMEOUT;
                    ulnDbcSetDDLLockTimeout( sDbc, (acp_sint32_t)sAttributeCVal );
                    break;
                case ULN_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION: /* BUG-48348 */ 
                    sDbc->mAttributeCID                 = ALTIBASE___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION;
                    sDbc->mEliminateCommonSubexpression = sAttributeCVal;
                    break;
                default:
                    ACI_RAISE( ATTRIBUTE_INVALID );
                    break;
                    
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }
    
    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEMORY)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnSetConnAttributeForDbc");
    }
    ACI_EXCEPTION( ATTRIBUTE_INVALID )
    {
        ulnError( aFnContext, ulERR_ABORT_SHARD_ERROR,
                  "ulnSetConnAttributeForDbc",
                  "Attribute is invalid." );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

