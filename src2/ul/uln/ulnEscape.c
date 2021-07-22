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

/**
 * 이 파일은 기존의 cli2 에 있던 escape 의 소스를 그대로 가져온 것이다.
 * 차후에 시간이 허락하는대로 효과적이고 제대로 동작하도록 수정할 필요가 있다.
 * 우선은 급한 김에 돌아가기만 하면 된다는 목표로 삽입한 것이다.
 *
 * BUGBUG: 새로 구현해야 한다.
 */

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnEscape.h>

#define ALLOC_MIN 1024

#define FIND_NEXT_TOKEN(tPtr, tStart, tEnd)         \
    for (tPtr = tStart;                             \
         acpCharIsSpace(*tPtr) && (tPtr < tEnd);    \
         tPtr++);

ulnEscapeModule gEscapeModule[ULN_ESCAPE_MAX] =
{
    {
        "d",
        ulnEscapeUnescapeDate
    },
    {
        "t",
        ulnEscapeUnescapeTime
    },
    {
        "ts",
        ulnEscapeUnescapeTimestamp
    },
    {
        "call",
        ulnEscapeUnescapeProcedureCall
    }
};

void ulnEscapeInitialize(ulnEscape *aEscape)
{
    aEscape->mSql = NULL;
    aEscape->mLen = 0;
    aEscape->mPtr = NULL;

    aEscape->mUnescapedSql = NULL;
    aEscape->mUnescapedLen = 0;
    aEscape->mUnescapedPtr = NULL;
}

void ulnEscapeFinalize  (ulnEscape *aEscape)
{
    if (aEscape->mUnescapedSql != NULL)
    {
        acpMemFree(aEscape->mUnescapedSql);
    }

    aEscape->mSql = NULL;
    aEscape->mLen = 0;
    aEscape->mPtr = NULL;

    aEscape->mUnescapedSql = NULL;
    aEscape->mUnescapedLen = 0;
    aEscape->mUnescapedPtr = NULL;
}

acp_char_t* ulnEscapeUnescapedSql(ulnEscape  *aEscape)
{
    acp_char_t* sRet;

    if (aEscape->mUnescapedSql != NULL)
    {
        sRet = aEscape->mUnescapedSql;
    }
    else
    {
        sRet = aEscape->mSql;
    }

    return sRet;
}

acp_sint32_t ulnEscapeUnescapedLen(ulnEscape  *aEscape)
{
    acp_sint32_t sRet;

    if (aEscape->mUnescapedSql != NULL)
    {
        sRet = aEscape->mUnescapedPtr - aEscape->mUnescapedSql;
    }
    else
    {
        sRet = aEscape->mLen;
    }

    return sRet;
}

ACI_RC ulnEscapeUnescapeByLen(ulnEscape    *aEscape,
                              acp_char_t   *aSql,
                              acp_sint32_t  aLen)
{
    acp_bool_t  sInQuote   = ACP_FALSE;
    acp_char_t *sPtrEscape = NULL;
    acp_char_t *sPtr;
    acp_char_t *sPtrEnd;

    aEscape->mSql = aSql;
    if (aLen == SQL_NTS)
    {
        aEscape->mLen = acpCStrLen(aEscape->mSql, ACP_SINT32_MAX);
    }
    else
    {
        aEscape->mLen = aLen;
    }
    aEscape->mPtr = aSql;

    aEscape->mUnescapedSql = NULL;
    aEscape->mUnescapedLen = 0;
    aEscape->mUnescapedPtr = NULL;

    for (sPtr = aEscape->mSql, sPtrEnd = (aEscape->mSql + aEscape->mLen);
         sPtr < sPtrEnd; sPtr++)
    {
        if (sInQuote == ACP_FALSE)
        {
            switch (*sPtr)
            {
                case '\'':
                    sInQuote = ACP_TRUE;
                    break;
                case '{':
                    sPtrEscape = sPtr;
                    break;
                case '}':
                    if (sPtrEscape != NULL)
                    {
                        ACI_TEST(ulnEscapeUnescapeByPtr(aEscape,
                                                        sPtrEscape,
                                                        sPtr)
                                 != ACI_SUCCESS);
                        sPtrEscape = NULL;
                    }
                    break;
                default:
                    break;
            }
        }
        else
        {
            if (*sPtr == '\'')
            {
                sInQuote = ACP_FALSE;
            }
        }
    }

    if (aEscape->mUnescapedSql != NULL)
    {
        ulnEscapeWriteToPtr(aEscape, sPtrEnd);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC ulnEscapeUnescapeByPtr(ulnEscape  *aEscape,
                              acp_char_t *aEscapePtr,
                              acp_char_t *aEnd)
{
    acp_char_t      *sPtrContent;
    acp_char_t      *sPtrRecover = NULL;
    ulnEscapeModule *sModule = ulnEscapeFindModule(aEscapePtr,
                                                   aEnd,
                                                   &sPtrContent);

    if (sModule != NULL)
    {
        if (aEscape->mUnescapedSql != NULL)
        {
            sPtrRecover = aEscape->mUnescapedPtr;
        }

        ACI_TEST(ulnEscapeWriteToPtr(aEscape, aEscapePtr) != ACI_SUCCESS);
        aEscape->mPtr = sPtrContent;

        if (sModule->mUnescape(aEscape, aEscapePtr, aEnd, sPtrContent) != ACI_SUCCESS)
        {
            aEscape->mPtr = aEscapePtr;

            if (aEscape->mUnescapedSql != NULL)
            {
                aEscape->mUnescapedPtr = sPtrRecover ? sPtrRecover : aEscape->mUnescapedSql;
            }
        }
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ulnEscapeModule* ulnEscapeFindModule(acp_char_t  *aEscapePtr,
                                     acp_char_t  *aEnd,
                                     acp_char_t **aPtr)
{
    acp_char_t    sIdName[10];
    acp_uint32_t  sIdLen;
    acp_char_t   *sPtr;
    acp_uint32_t  i;

    for (sPtr = aEscapePtr + 1;
         !acpCharIsAlpha(*sPtr) && (sPtr < aEnd);
         sPtr++);

    for (sIdLen = 0;
         acpCharIsAlpha(sPtr[sIdLen]) && (sPtr + sIdLen < aEnd);
         sIdLen++);

    if (sIdLen < ACI_SIZEOF(sIdName))
    {
        acpMemCpy(sIdName, sPtr, sIdLen);
        sIdName[sIdLen] = 0;

        for (i = 0; i < ULN_ESCAPE_MAX; i++)
        {
            ACE_DASSERT(acpCStrLen(gEscapeModule[i].mIdentifier, ACP_SINT32_MAX) < ACI_SIZEOF(sIdName));

            if (acpCStrCmp(sIdName, gEscapeModule[i].mIdentifier, ACI_SIZEOF(sIdName)) == 0)
            {
                FIND_NEXT_TOKEN(sPtr, sPtr + sIdLen + 1, aEnd);
                *aPtr = sPtr;

                return &gEscapeModule[i];
            }
        }
    }

    return NULL;
}

ACI_RC ulnEscapeExtendBuffer(ulnEscape    *aEscape,
                             acp_sint32_t  aSizeDelta)
{
    if (aEscape->mUnescapedSql == NULL)
    {
        aEscape->mUnescapedLen = (((ACP_MAX(aEscape->mLen, aSizeDelta) - 1) / ALLOC_MIN) + 1) * ALLOC_MIN;
        ACI_TEST(acpMemAlloc((void**)&aEscape->mUnescapedSql, aEscape->mUnescapedLen)
                 != ACP_RC_SUCCESS);
        aEscape->mUnescapedPtr = aEscape->mUnescapedSql;
    }
    else
    {
        if (aSizeDelta + (aEscape->mUnescapedPtr - aEscape->mUnescapedSql) > aEscape->mUnescapedLen)
        {
            acp_sint32_t sOffset   = aEscape->mUnescapedPtr - aEscape->mUnescapedSql;
            aEscape->mUnescapedLen += ALLOC_MIN;
            ACI_TEST(acpMemRealloc((void**)&aEscape->mUnescapedSql, aEscape->mUnescapedLen)
                     != ACP_RC_SUCCESS);
            aEscape->mUnescapedPtr  = aEscape->mUnescapedSql + sOffset;
        }
    }

    ACI_TEST(aEscape->mUnescapedSql == NULL);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    {
    }

    return ACI_FAILURE;
}

ACI_RC ulnEscapeWriteString(ulnEscape        *aEscape,
                            const acp_char_t *aStr)
{
    acp_sint32_t sLen = acpCStrLen(aStr, ACP_SINT32_MAX);

    if (sLen > 0)
    {
        ACI_TEST(ulnEscapeExtendBuffer(aEscape, sLen) != ACI_SUCCESS);

        acpMemCpy(aEscape->mUnescapedPtr, aStr, sLen);

        aEscape->mUnescapedPtr += sLen;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC ulnEscapeWriteToPtr(ulnEscape    *aEscape,
                           acp_char_t        *aPtr)
{
    acp_sint32_t sSize = aPtr - aEscape->mPtr;

    if (sSize > 0)
    {
        ACI_TEST(ulnEscapeExtendBuffer(aEscape, sSize) != ACI_SUCCESS);

        acpMemCpy(aEscape->mUnescapedPtr, aEscape->mPtr, sSize);

        aEscape->mPtr           = aPtr;
        aEscape->mUnescapedPtr += sSize;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC ulnEscapeUnescapeDate(ulnEscape  *aSelf,
                             acp_char_t *aEscapePtr,
                             acp_char_t *aEnd,
                             acp_char_t *aPtr)
{
    ACP_UNUSED(aEscapePtr);
    ACP_UNUSED(aPtr);

    ACI_TEST(ulnEscapeWriteString(aSelf, "TO_DATE(") != ACI_SUCCESS);

    ACI_TEST(ulnEscapeWriteToPtr(aSelf, aEnd) != ACI_SUCCESS);
    aSelf->mPtr = aEnd + 1;

    ACI_TEST(ulnEscapeWriteString(aSelf, ",'YYYY-MM-DD')") != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC ulnEscapeUnescapeTime(ulnEscape  *aSelf,
                             acp_char_t *aEscapePtr,
                             acp_char_t *aEnd,
                             acp_char_t *aPtr)
{
    ACP_UNUSED(aEscapePtr);
    ACP_UNUSED(aPtr);

    ACI_TEST(ulnEscapeWriteString(aSelf, "TO_DATE(") != ACI_SUCCESS);

    ACI_TEST(ulnEscapeWriteToPtr(aSelf, aEnd) != ACI_SUCCESS);
    aSelf->mPtr = aEnd + 1;

    ACI_TEST(ulnEscapeWriteString(aSelf, ",'HH:MI:SS')") != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC ulnEscapeUnescapeTimestamp(ulnEscape  *aSelf,
                                  acp_char_t *aEscapePtr,
                                  acp_char_t *aEnd,
                                  acp_char_t *aPtr)
{
    acp_char_t   *sPtr;
    acp_sint32_t  sPrecision = 0;

    ACP_UNUSED(aEscapePtr);

    for (sPtr = aPtr + 1; sPtr < aEnd; sPtr++)
    {
        if ((*sPtr == '\'') || (sPrecision == 7))
        {
            break;
        }

        if (sPrecision > 0)
        {
            if (acpCharIsDigit(*sPtr))
            {
                sPrecision++;
            }
        }
        else
        {
            if (*sPtr == '.')
            {
                sPrecision = 1;
            }
        }
    }

    ACI_TEST(ulnEscapeWriteString(aSelf, "TO_DATE(") != ACI_SUCCESS);

    ACI_TEST(ulnEscapeWriteToPtr(aSelf, sPtr) != ACI_SUCCESS);
    aSelf->mPtr = aEnd + 1;

    if (sPrecision == 0)
    {
        ACI_TEST(ulnEscapeWriteString(aSelf, ".") != ACI_SUCCESS);
        sPrecision++;
    }
    while (sPrecision++ < 7)
    {
        ACI_TEST(ulnEscapeWriteString(aSelf, "0") != ACI_SUCCESS);
    }

    ACI_TEST(ulnEscapeWriteString(aSelf, "','YYYY-MM-DD HH:MI:SS.SSSSSS')") != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC ulnEscapeUnescapeProcedureCall(ulnEscape  *aSelf,
                                      acp_char_t *aEscapePtr,
                                      acp_char_t *aEnd,
                                      acp_char_t *aPtr)
{
    acp_char_t *sPtr;

    FIND_NEXT_TOKEN(sPtr, aEscapePtr + 1, aEnd);

    if (*sPtr == '?')
    {
        FIND_NEXT_TOKEN(sPtr, sPtr + 1, aEnd);

        ACI_TEST(*sPtr != '=');

        FIND_NEXT_TOKEN(sPtr, sPtr + 1, aEnd);
        FIND_NEXT_TOKEN(sPtr, sPtr + 4, aEnd);

        ACI_TEST(sPtr != aPtr);

        ACI_TEST(ulnEscapeWriteString(aSelf, "EXECUTE ? := ") != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnEscapeWriteString(aSelf, "EXECUTE ") != ACI_SUCCESS);
    }

    ACI_TEST(ulnEscapeWriteToPtr(aSelf, aEnd) != ACI_SUCCESS);
    aSelf->mPtr = aEnd + 1;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}
