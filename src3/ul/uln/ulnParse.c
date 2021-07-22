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

#include <ulnPrivate.h>

// BUGBUG (2014-10-15) 정규식이든 yacc이든 정확한 포맷을 확인하고 파싱하는 방법으로 바꿔야한다.
/**
 * 연결 문자열을 통해 연결 속성을 설정한다.
 *
 * 연결 문자열의 형태는 다음을 참고한다:
 * http://nok.altibase.com/x/MBr2AQ
 *
 * @param[in]  aContext     context
 * @param[in]  aConnStr     연결 문자열
 * @param[in]  aConnStrLen  연결 문자열의 길이(octet length)
 * @param[in]  aCallback    파싱한 속성 처리에 사용할 콜백
 * @param[in]  aFilter      callback에서 사용할 필터
 *
 * @return 처리한 속성 수. 에러가 발생했다면 -1
 *
 * @warning ulERR_IGNORE_CONNECTION_STR_IGNORED     연결 문자열에서 지정한 속성을 무시했을 경우
 * @error   ulERR_ABORT_INVALID_CONNECTION_STR_FORM 연결 문자열 포맷이 잘못된 경우
 */
acp_sint32_t ulnConnStrParse( void                    *aContext,
                              const acp_char_t        *aConnStr,
                              acp_sint16_t             aConnStrLen,
                              ulnConnStrParseCallback  aCallback,
                              void                    *aFilter )
{
    acp_sint32_t  sPos = 0;
    acp_sint32_t  sKeyPosL;
    acp_sint32_t  sKeyPosR;
    acp_sint32_t  sKeyLen;
    acp_sint32_t  sValPosL;
    acp_sint32_t  sValPosR;
    acp_sint32_t  sValLen;
    acp_sint32_t  sCallbackRC;
    acp_sint32_t  sRC = 0;

    do
    {
        sKeyPosL = sPos = ulnParseIndexOfNonWhitespace(aConnStr, aConnStrLen, sPos);
        ACI_TEST_RAISE(sPos >= aConnStrLen, STOP_CONNSTR_PARSE);
        if (aConnStr[sPos] == ';')
        {
            sPos++;
            continue;
        }
        sKeyPosR = sPos = ulnParseIndexOfIdEnd(aConnStr, aConnStrLen, sPos);
        ACI_TEST_RAISE(sPos >= aConnStrLen, INVALID_CONNSTR_FORMAT);
        sKeyLen = sKeyPosR - sKeyPosL;

        sPos = ulnParseIndexOfNonWhitespace(aConnStr, aConnStrLen, sPos);
        ACI_TEST_RAISE(sPos >= aConnStrLen, INVALID_CONNSTR_FORMAT);
        ACI_TEST_RAISE(aConnStr[sPos] != '=', INVALID_CONNSTR_FORMAT);
        sPos++;

        sPos = ulnParseIndexOfNonWhitespace(aConnStr, aConnStrLen, sPos);
        if (sPos >= aConnStrLen)
        {
            /* no value */
            ACI_TEST( aCallback( aContext, ULN_CONNSTR_PARSE_EVENT_IGNORE, sPos,
                                 aConnStr + sKeyPosL, sKeyLen, NULL, 0, aFilter )
                      == ULN_CONNSTR_PARSE_RC_ERROR );
            break;
        }
        switch (aConnStr[sPos])
        {
            case ';':
                /* no value */
                ACI_TEST( aCallback( aContext, ULN_CONNSTR_PARSE_EVENT_IGNORE, sPos,
                                     aConnStr + sKeyPosL, sKeyLen, NULL, 0, aFilter )
                          == ULN_CONNSTR_PARSE_RC_ERROR );
                continue;

            case '"':
                sValPosL = sPos;
                sPos = ulnParseIndexOf(aConnStr, aConnStrLen, sPos + 1, '"');
                ACI_TEST_RAISE(aConnStr[sPos] != '"', INVALID_CONNSTR_FORMAT);
                sValPosR = ++sPos;
                break;

            case '(':
                sValPosL = sPos;
                for (sPos++; sPos < aConnStrLen && aConnStr[sPos] != ')'; sPos++)
                {
                    switch (aConnStr[sPos])
                    {
                        case ',':
                        case '.':
                        case ':':
                        case '/':
                        case '%':
                        case '[':
                        case ']':
                            break;
                        default:
                            ACI_TEST_RAISE( acpCharIsSpace(aConnStr[sPos]) == ACP_FALSE &&
                                            acpCharIsAlnum(aConnStr[sPos]) == ACP_FALSE,
                                            INVALID_CONNSTR_FORMAT );
                            break;
                    }
                }
                ACI_TEST_RAISE(sPos >= aConnStrLen || aConnStr[sPos] != ')', INVALID_CONNSTR_FORMAT);
                sValPosR = ++sPos;
                break;

            case '[':
                sValPosL = sPos;
                for (sPos++; sPos < aConnStrLen && aConnStr[sPos] != ']'; sPos++)
                {
                    switch (aConnStr[sPos])
                    {
                        case ':':
                        case '/':
                        case '%':
                        case '.':
                            break;
                        default:
                            ACI_TEST_RAISE( acpCharIsAlnum(aConnStr[sPos]) == ACP_FALSE,
                                            INVALID_CONNSTR_FORMAT );
                            break;
                    }
                }
                ACI_TEST_RAISE(sPos >= aConnStrLen || aConnStr[sPos] != ']', INVALID_CONNSTR_FORMAT);
                sValPosR = ++sPos;
                break;

            default:
                sValPosL = sPos;
                sValPosR = sPos = ulnParseIndexOf(aConnStr, aConnStrLen, sPos + 1, ';');
                ACI_TEST_RAISE(sPos < aConnStrLen && aConnStr[sPos] != ';', INVALID_CONNSTR_FORMAT);
                for (; sValPosL < sValPosR && acpCharIsSpace(aConnStr[sValPosR-1]) == ACP_TRUE; sValPosR--)
                    ;
                break;
        }
        sValLen = sValPosR - sValPosL;

        sCallbackRC = aCallback( aContext, ULN_CONNSTR_PARSE_EVENT_GO, sPos,
                                 aConnStr + sKeyPosL, sKeyLen,
                                 aConnStr + sValPosL, sValLen,
                                 aFilter );
        ACI_TEST(sCallbackRC & ULN_CONNSTR_PARSE_RC_ERROR);
        if (sCallbackRC & ULN_CONNSTR_PARSE_RC_SUCCESS)
        {
            sRC++;
        }
        if (sCallbackRC & ULN_CONNSTR_PARSE_RC_STOP)
        {
            break;
        }
    } while (sPos < aConnStrLen);

    ACI_EXCEPTION_CONT(STOP_CONNSTR_PARSE);

    return sRC;

    ACI_EXCEPTION(INVALID_CONNSTR_FORMAT)
    {
        if (sPos < aConnStrLen)
        {
            aCallback( aContext, ULN_CONNSTR_PARSE_EVENT_ERROR, sPos, aConnStr + sPos, 1, NULL, 0, aFilter );
        }
        else
        {
            aCallback( aContext, ULN_CONNSTR_PARSE_EVENT_ERROR, sPos, NULL, 0, NULL, 0, aFilter );
        }
    }
    ACI_EXCEPTION_END;

    return -1;
}

ulnConnStrAttrType ulnConnStrGetAttrType( const acp_char_t *aAttrVal )
{
    acp_size_t         sAttrValLen;
    acp_sint32_t       sFoundIdx;
    ulnConnStrAttrType sRet;

    if (aAttrVal == NULL || aAttrVal[0] == '\0')
    {
        sRet = ULN_CONNSTR_ATTR_TYPE_NONE;
    }
    else
    {
        sAttrValLen = acpCStrLen(aAttrVal, ACP_SINT32_MAX);
        if ( (aAttrVal[0] == '"' && aAttrVal[sAttrValLen - 1] == '"') )
        {
            sRet = ULN_CONNSTR_ATTR_TYPE_ENCLOSED;
        }
        else if ( (acpCStrFindChar((acp_char_t *)aAttrVal, ';', &sFoundIdx, 0, 0) == ACP_RC_ENOENT) &&
                  (acpCharIsSpace(aAttrVal[0]) == ACP_FALSE) &&
                  (acpCharIsSpace(aAttrVal[sAttrValLen - 1]) == ACP_FALSE) )
        {
            sRet = ULN_CONNSTR_ATTR_TYPE_NORMAL;
        }
        else if ( acpCStrFindChar((acp_char_t *)aAttrVal, '"', &sFoundIdx, 0, 0) == ACP_RC_ENOENT )
        {
            sRet = ULN_CONNSTR_ATTR_TYPE_NEED_TO_ENCLOSE;
        }
        else
        {
            sRet = ULN_CONNSTR_ATTR_TYPE_INVALID;
        }
    }

    return sRet;
}

acp_rc_t ulnConnStrAppendStrAttr( acp_char_t       *aConnStr,
                                  acp_size_t        aConnStrSize,
                                  const acp_char_t *aAttrKey,
                                  const acp_char_t *aAttrVal )
{
    ulnConnStrAttrType sConnStrAttrType = ulnConnStrGetAttrType(aAttrVal);
    acp_size_t         sConnStrLen;
    acp_rc_t           sRC = ACP_RC_SUCCESS;

    ACI_TEST_RAISE( sConnStrAttrType == ULN_CONNSTR_ATTR_TYPE_NONE, NO_NEED_WORK );

    sConnStrLen = acpCStrLen(aConnStr, aConnStrSize);

    switch ( sConnStrAttrType )
    {
        case ULN_CONNSTR_ATTR_TYPE_NORMAL:
        case ULN_CONNSTR_ATTR_TYPE_ENCLOSED:
            sRC = acpSnprintf( aConnStr + sConnStrLen,
                               aConnStrSize - sConnStrLen,
                               "%s=%s;", aAttrKey, aAttrVal );
            break;

        case ULN_CONNSTR_ATTR_TYPE_NEED_TO_ENCLOSE:
            sRC = acpSnprintf( aConnStr + sConnStrLen,
                               aConnStrSize - sConnStrLen,
                               "%s=\"%s\";", aAttrKey, aAttrVal );
            break;

        default:
        case ULN_CONNSTR_ATTR_TYPE_INVALID:
            sRC = ACP_RC_EINVAL;
            break;
    }

    ACI_EXCEPTION_CONT( NO_NEED_WORK );

    return sRC;
}
