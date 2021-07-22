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
#include <ulnConnect.h>
#include <ulnConnectCore.h>

#define ULN_CONNECT_ATTR_MAX_STRING_LENGTH    512

ACI_RC ulnSFID_85(ulnFnContext *aFnContext)
{
    if(aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        /*
         * 빠져나갈 때 SQL_SUCCESS 를 리턴할 경우, 즉, 연결이 성공한 경우
         * Connected 상태 (4번 상태) 로 상태 전이를 한다.
         */
        if(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext)) != 0)
        {
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mDbc, ULN_S_C4);
        }
    }

    return ACI_SUCCESS;
}

/* Check Input Parameters  */
ACI_RC ulnStringCheck(ulnFnContext     *aContext,
                      const acp_char_t *aStr,
                      acp_sint16_t      aStrLen,
                      acp_uint32_t     *aBufLen)
{
    *aBufLen = 0;

    /* 1. ERR_HY009 : Invalide Use of Null Pointer  */
    ACI_TEST_RAISE( aStr == NULL, LABEL_INVALID_USE_OF_NULL );

    /* 2. ERR_HY090 : Invalid string of buffer length  */
    if(aStrLen <= 0)
    {
        ACI_TEST_RAISE( aStrLen != SQL_NTS, LABEL_INVALID_BUFFER_LEN );

        /*
         * BUG-28980 [CodeSonar]Null Pointer Dereference
         */
        if(aStr != NULL)
        {
            *aBufLen = acpCStrLen(aStr, ACP_SINT32_MAX);
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        *aBufLen = aStrLen;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        ulnError(aContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN)
    {
        ulnError(aContext, ulERR_ABORT_INVALID_BUFFER_LEN, (acp_uint32_t)aStrLen);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnConnect(ulnDbc       *aDbc,
                     acp_char_t   *aServerName,
                     acp_sint16_t  aServerNameLength,
                     acp_char_t   *aUserName,
                     acp_sint16_t  aUserNameLength,
                     acp_char_t   *aPassword,
                     acp_sint16_t  aPasswordLength)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext sFnContext;
    acp_uint32_t sRealLength = 0;

    // fix BUG-19054
    acp_char_t   sServerName[ULN_CONNECT_ATTR_MAX_STRING_LENGTH + 1];
    acp_char_t   sUserName[ULN_CONNECT_ATTR_MAX_STRING_LENGTH + 1];
    acp_char_t   sPassword[ULN_CONNECT_ATTR_MAX_STRING_LENGTH + 1];

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_CONNECT, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedExit);

    /* PROJ-1573 XA */
    ACI_TEST_RAISE(aDbc->mXaEnlist == ACP_TRUE, LABEL_SUCCESS_RETURN);

    /*
     * ===========================
     * Function BEGIN
     * ===========================
     */

    /* BUG-36256 Improve property's communication. */
    ulnConnAttrArrReInit(&aDbc->mUnsupportedProperties);

    /* ODBC spec define to request DEFAULT DataSource */
    if(aServerName == NULL)
    {
        aServerName       = (acp_char_t *)"DEFAULT";
        aServerNameLength = SQL_NTS;
    }

    /*
     * Servername
     */

    ACI_TEST(ulnStringCheck(&sFnContext,
                            aServerName,
                            aServerNameLength,
                            &sRealLength) != ACI_SUCCESS);

    // fix BUG-19054
    sRealLength = ACP_MIN(sRealLength, ULN_CONNECT_ATTR_MAX_STRING_LENGTH);

    if (aServerNameLength > 0)
    {
        acpCStrCpy(sServerName, ULN_CONNECT_ATTR_MAX_STRING_LENGTH + 1,
                   aServerName, sRealLength);
        sServerName[sRealLength] = '\0';

        ACI_TEST(ulnSetConnAttrById(&sFnContext,
                                    ULN_CONN_ATTR_DSN,
                                    sServerName,
                                    sRealLength) != ACI_SUCCESS);

    }
    else
    {
        ACI_TEST(ulnSetConnAttrById(&sFnContext,
                                    ULN_CONN_ATTR_DSN,
                                    aServerName,
                                    sRealLength) != ACI_SUCCESS);
    }

    /*
     * User ID
     */

    // fix BUG-19631
    if (aUserName != NULL)
    {
        ACI_TEST(ulnStringCheck(&sFnContext,
                                aUserName,
                                aUserNameLength,
                                &sRealLength) != ACI_SUCCESS);

        // fix BUG-19054
        sRealLength = ACP_MIN(sRealLength, ULN_CONNECT_ATTR_MAX_STRING_LENGTH);

        if (aUserNameLength > 0)
        {
            acpCStrCpy(sUserName, ULN_CONNECT_ATTR_MAX_STRING_LENGTH + 1,
                       aUserName, sRealLength);
            sUserName[sRealLength] = '\0';

            ACI_TEST(ulnSetConnAttrById(&sFnContext,
                                        ULN_CONN_ATTR_UID,
                                        sUserName,
                                        sRealLength) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST(ulnSetConnAttrById(&sFnContext,
                                        ULN_CONN_ATTR_UID,
                                        aUserName,
                                        sRealLength) != ACI_SUCCESS);
        }
    }

    /*
     * Passwd
     */

    // fix BUG-19631
    if (aPassword != NULL)
    {
        ACI_TEST(ulnStringCheck(&sFnContext,
                                aPassword,
                                aPasswordLength,
                                &sRealLength) != ACI_SUCCESS);

        // fix BUG-19054
        sRealLength = ACP_MIN(sRealLength, ULN_CONNECT_ATTR_MAX_STRING_LENGTH);

        if (aPasswordLength > 0)
        {
            acpCStrCpy(sPassword, ULN_CONNECT_ATTR_MAX_STRING_LENGTH + 1,
                       aPassword, sRealLength);
            sPassword[sRealLength] = '\0';

            ACI_TEST(ulnSetConnAttrById(&sFnContext,
                                        ULN_CONN_ATTR_PWD,
                                        sPassword,
                                        sRealLength) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST(ulnSetConnAttrById(&sFnContext,
                                        ULN_CONN_ATTR_PWD,
                                        aPassword,
                                        sRealLength) != ACI_SUCCESS);
        }
    }

    /*
     * User Profile (odbc.ini, registry 등) 을 이용한 DBC Attribute 세팅
     *
     * cli 를 직접 쓸 경우 아무것도 안하는 더미 함수 호출
     */

    ACI_TEST(ulnSetConnAttrByProfileFunc(&sFnContext,
                                         ulnDbcGetDsnString(aDbc),
                                         (acp_char_t *)"odbc.ini") != ACI_SUCCESS);

    // fix BUG-19631
    ACI_TEST_RAISE( (aDbc->mUserName == NULL || aDbc->mPassword == NULL), LABEL_INVALID_USE_OF_NULL );

    /* BUG-47327 */
#ifdef COMPILE_SHARDCLI
    /* BUG-45707 */
    ulsdDbcSetShardCli( aDbc, ULSD_SHARD_CLIENT_TRUE );

    aDbc->mShardDbcCxt.mTargetShardMetaNumber = 0UL;

    ulnDbcSetShardMetaNumber( aDbc, 0 );

    /* BUG-47272 */
    if ( aDbc->mShardDbcCxt.mShardSessionType == ULSD_SESSION_TYPE_LIB )
    {
        ACI_TEST( ulsdSetConnAttrForLibConn( &sFnContext ) != ACI_SUCCESS );
    }
#endif /* COMPILE_SHARDCLI */

    ulsdInitDistTxInfo(aDbc);  /* PROJ-2733-DistTxInfo */

    /* TASK-7219 Non-shard DML */
    ulnDbcInitStmtExecSeqForShardTx(aDbc);

    ACI_TEST( ulnFailoverBuildServerList(&sFnContext) != ACI_SUCCESS );

    ACI_TEST( ulnConnectCore(aDbc, &sFnContext) != ACI_SUCCESS );

#ifdef COMPILE_SHARDCLI
    // shard connection 실패시
    // sFnContext 에 SQL_SUCCESS_WITH_INFO 를 세팅하고 SQL_ERROR 를 반환
    ACI_TEST( !SQL_SUCCEEDED( ulsdModuleNodeConnect( aDbc,
                                                     &sFnContext,
                                                     aServerName,
                                                     aServerNameLength,
                                                     aUserName,
                                                     aUserNameLength,
                                                     aPassword,
                                                     aPasswordLength ) ) );
#endif /* COMPILE_SHARDCLI */

    /*
     * ===========================
     * Function END
     * ===========================
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [server: %s user: %s]",
            "ulnConnect", aServerName, aUserName);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_SUCCESS_RETURN);
    {
        ulnExit(&sFnContext);
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION_END;

    /*
     * Note : to sjkim :
     *        DBC 의 메모리는 SQLFreeHandle(DBC) 를 호출하면서 해제됩니다 - shawn
     */

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [server: %s user: %s] fail",
            "ulnConnect", aServerName, aUserName);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
