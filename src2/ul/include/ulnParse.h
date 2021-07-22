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

#ifndef _ULN_PARSE_H_
#define _ULN_PARSE_H_ 1

ACP_EXTERN_C_BEGIN

ACP_INLINE acp_sint32_t ulnParseIndexOf( const acp_char_t *aSrcStr,
                                         acp_sint32_t      aSrcStrSize,
                                         acp_sint32_t      aStartPos,
                                         acp_char_t        aFindChar )
{
    acp_sint32_t  i;  

    for (i = aStartPos; i < aSrcStrSize; i++)
    {   
        if (aSrcStr[i] == aFindChar)
        {
            break;
        }
    }   

    return i;
}

ACP_INLINE acp_sint32_t ulnParseIndexOfNonWhitespace( const acp_char_t *aSrcStr,
                                                      acp_sint32_t      aSrcStrSize,
                                                      acp_sint32_t      aStartPos )
{
    acp_sint32_t  i;  

    for (i = aStartPos; i < aSrcStrSize; i++)
    {   
        if (acpCharIsSpace(aSrcStr[i]) == ACP_FALSE)
        {
            break;
        }
    }   

    return i;
}

ACP_INLINE acp_sint32_t ulnParseIndexOfIdEnd( const acp_char_t *aSrcStr,
                                              acp_sint32_t      aSrcStrSize,
                                              acp_sint32_t      aStartPos )
{
    acp_sint32_t i;

    if (aSrcStr[aStartPos] == '_' || acpCharIsAlpha(aSrcStr[aStartPos]))
    {   
        for (i = aStartPos + 1; i < aSrcStrSize; i++)
        {
            if (aSrcStr[i] != '_' && acpCharIsAlnum(aSrcStr[i]) == ACP_FALSE)
            {
                break;
            }
        }
    }   
    else
    {   
        i = aStartPos;
    }   

    return i;
}

typedef enum ulnConnStrParseCallbackEvent
{
    ULN_CONNSTR_PARSE_EVENT_GO,
    ULN_CONNSTR_PARSE_EVENT_IGNORE,
    ULN_CONNSTR_PARSE_EVENT_ERROR
} ulnConnStrParseCallbackEvent;

#define ULN_CONNSTR_PARSE_RC_SUCCESS 0x01
#define ULN_CONNSTR_PARSE_RC_STOP    0x02
#define ULN_CONNSTR_PARSE_RC_ERROR   0x04

/**
 * ulnConnStrParse()를 위한 콜백
 *
 * @param[in]  aFnContext  function context
 * @param[in]  aKey        키
 * @param[in]  aKeyLen     키 길이
 * @param[in]  aVal        값
 * @param[in]  aValLen     값 길이
 * @param[in]  aFilter     필터
 *
 * @return 다음과 같은 값으로 구성된 플래그
 *         - ULN_CONNSTR_PARSE_SUCCESS : 처리를 잘 끝냄
 *         - ULN_CONNSTR_PARSE_STOP    : 더 파싱할 필요 없음
 *         - ULN_CONNSTR_PARSE_ERROR   : 에러가 발생했고, aFnContext에 에러 정보를 설정함
 */
typedef acp_sint32_t (*ulnConnStrParseCallback)( void                         *aContext,
                                                 ulnConnStrParseCallbackEvent  aEvent,
                                                 acp_sint32_t                  aPos,
                                                 const acp_char_t             *aKey,
                                                 acp_sint32_t                  aKeyLen,
                                                 const acp_char_t             *aVal,
                                                 acp_sint32_t                  aValLen,
                                                 void                         *aFilter );

acp_sint32_t ulnConnStrParse( void                    *aContext,
                              const acp_char_t        *aConnStr,
                              acp_sint16_t             aConnStrLen,
                              ulnConnStrParseCallback  aCallback,
                              void                    *aFilter );

/* PROJ-1538 IPv6 */
ACP_INLINE acp_bool_t ulnParseIsBracketedAddress( acp_char_t   *aAddress,
                                                  acp_sint32_t  aAddressLen )
{
    if ( aAddress != NULL &&
         aAddressLen > 2 &&
         aAddress[0] == '[' &&
         aAddress[aAddressLen - 1] == ']')
    {
        return ACP_TRUE;
    }   
    else
    {   
        return ACP_FALSE;
    }
}

typedef enum ulnConnStrAttrType
{
    ULN_CONNSTR_ATTR_TYPE_INVALID = -1,
    ULN_CONNSTR_ATTR_TYPE_NONE,
    ULN_CONNSTR_ATTR_TYPE_NORMAL,
    ULN_CONNSTR_ATTR_TYPE_ENCLOSED,
    ULN_CONNSTR_ATTR_TYPE_NEED_TO_ENCLOSE
} ulnConnStrAttrType;

ulnConnStrAttrType ulnConnStrGetAttrType( const acp_char_t *aAttrVal );

acp_rc_t ulnConnStrAppendStrAttr( acp_char_t       *aConnStr,
                                  acp_size_t        aConnStrSize,
                                  const acp_char_t *aAttrKey,
                                  const acp_char_t *aAttrVal );

ACP_EXTERN_C_END

#endif /* _ULN_PARSE_H_ */

