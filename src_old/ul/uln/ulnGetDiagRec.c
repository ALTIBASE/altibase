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
#include <ulnDiagnostic.h>

/*
 * BUGBUG:
 * SQLCHAR * 는 C 의 unsigned char * 로 매핑된다.
 * MSDN ODBC Appendix D:Data Types -> C Data Types 에 보면 표가 나온다.
 */
SQLRETURN ulnGetDiagRec(acp_sint16_t   aHandleType,
                        ulnObject     *aObject,
                        acp_sint16_t   aRecNumber,
                        acp_char_t    *aSqlState,
                        acp_sint32_t  *aNativeErrorPtr,
                        acp_char_t    *aMessageText,
                        acp_sint16_t   aBufferLength,
                        acp_sint16_t  *aTextLengthPtr,
                        acp_bool_t     aRemoveAfterGet)
{
    ULN_FLAG(sNeedUnlock);

    acp_char_t   *sSqlState;
    ulnObject    *sObject = NULL;

    acp_sint32_t  sNativeErrorCode;
    acp_sint16_t  sActualLength;
    SQLRETURN     sSqlReturnCode = SQL_SUCCESS;

    ulnDiagRec   *sDiagRec;

    ACI_TEST_RAISE(aObject == NULL, InvalidHandle);

    sObject = aObject;

    /*
     * 핸들 타입과 객체의 실제 타입이 일치하는지 체크
     */
    switch (aHandleType)
    {
        case SQL_HANDLE_STMT:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_STMT, InvalidHandle);
            break;

        case SQL_HANDLE_DBC:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_DBC, InvalidHandle);
            sObject = (ulnObject*)ulnDbcSwitch((ulnDbc *) sObject);
            break;

        case SQL_HANDLE_ENV:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_ENV, InvalidHandle);
            break;

        case SQL_HANDLE_DESC:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_DESC, InvalidHandle);
            ACI_TEST_RAISE(ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_ARD &&
                           ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_APD &&
                           ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_IRD &&
                           ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_IPD,
                           InvalidHandle);
            break;

        default:
            ACI_RAISE(InvalidHandle);
            break;
    }

    /*
     * ======================================
     * Function BEGIN
     * ======================================
     */

    ACI_TEST_RAISE(aRecNumber <= 0, Error);
    ACI_TEST_RAISE(aBufferLength < 0, Error);

    /*
     * Note : 상태전이 테이블은 필요 없다. 핸들 할당 안되있으면 무조건 Invalid Handle
     */

    /*
     * Diagnostic Record 를 가리키는 포인터를 얻는다.
     */
    ACI_TEST_RAISE(ulnGetDiagRecFromObject(sObject, &sDiagRec, aRecNumber) != ACI_SUCCESS,
                   NoData);

    /*
     * SQLSTATE String 을 가리키는 포인터를 얻기
     * Note : 곧장 사용자 메모리에 복사를 하지 않은 이유는 GetDiagField 에서도 쓰기 때문
     */
    ulnDiagRecGetSqlState(sDiagRec, &sSqlState);

    /*
     * NativeErrorCode
     */
    sNativeErrorCode = ulnDiagRecGetNativeErrorCode(sDiagRec);

    /*
     * Message Text
     */
    // BUG-22887 Windows ODBC 에러
    // 함수안에서 알아서 해주고 있다.
    ACI_TEST(ulnDiagRecGetMessageText(sDiagRec,
                                      aMessageText,
                                      aBufferLength,
                                      &sActualLength) != ACI_SUCCESS);
    /*
     * Truncation 시 SQL_SUCCESS_WITH_INFO 리턴
     */
    if ( sActualLength >= aBufferLength )
    {
        sSqlReturnCode = SQL_SUCCESS_WITH_INFO;
    }

    // BUG-23965 aTextLengthPtr 이 NULL 이 들어올때 잘못 처리함
    /*
     * TextLength
     */
    if(aTextLengthPtr != NULL)
    {
        *aTextLengthPtr = sActualLength;
    }

    /*
     * SQLSTATE
     */
    if(aSqlState != NULL)
    {
        acpSnprintf(aSqlState, 6, "%s", sSqlState);
    }

    /*
     * NativeErrorCode
     */
    if(aNativeErrorPtr != NULL)
    {
        *aNativeErrorPtr = sNativeErrorCode;
    }

    /*
     * SQLError() 에 의해서 호출 되었을 경우,
     * 사용자에게 알맞은 값을 돌려준 후 해당 레코드를 삭제해야 한다.
     */
    if(aRemoveAfterGet == ACP_TRUE)
    {
        ulnDiagHeaderRemoveDiagRec(sDiagRec->mHeader, sDiagRec);
    }

    /*
     * ======================================
     * Function END
     * ======================================
     */

    return sSqlReturnCode;

    ACI_EXCEPTION(InvalidHandle)
    {
        sSqlReturnCode = SQL_INVALID_HANDLE;
    }

    ACI_EXCEPTION(Error)
    {
        sSqlReturnCode = SQL_ERROR;
    }

    ACI_EXCEPTION(NoData)
    {
        sSqlReturnCode = SQL_NO_DATA;
    }

    ACI_EXCEPTION_END;
    {
        if (aSqlState != NULL)
        {
            *aSqlState = 0;
        }

        if (aNativeErrorPtr != NULL)
        {
            *aNativeErrorPtr = 0;
        }

        if ((aMessageText != NULL) && (aBufferLength > 0))
        {
            *aMessageText = 0;
        }

        if (aTextLengthPtr != NULL)
        {
            *aTextLengthPtr = 0;
        }
    }

    ULN_IS_FLAG_UP(sNeedUnlock)
    {
        ULN_OBJECT_UNLOCK(sObject, ULN_FID_GETDIAGREC);
    }

    return sSqlReturnCode;
}

