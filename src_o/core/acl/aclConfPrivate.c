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
 
/*******************************************************************************
 * $Id: aclConfPrivate.c 5621 2009-05-13 07:34:16Z sjkim $
 ******************************************************************************/

#include <aclConf.h>
#include <aclConfPrivate.h>


ACP_INLINE acl_conf_def_t *aclConfFindDef(acl_conf_def_t *aDef, acp_str_t *aKey)
{
    acl_conf_def_t *sDefaultDef = NULL;
    acp_sint32_t    i;

    for (i = 0; aDef[i].mType != ACL_CONF_TYPE_NONE; i++)
    {
        /*
         * UNNAMED_CONTAINER 타입의 Def를 Default Def로 세팅
         */
        if ((sDefaultDef == NULL) &&
            (aDef[i].mType == ACL_CONF_TYPE_UNNAMED_CONTAINER))
        {
            sDefaultDef = &aDef[i];
        }
        else
        {
            /* do nothing */
        }

        /*
         * Key를 비교하여 Def를 찾음
         */
        if (acpStrCmpCString(aKey, aDef[i].mKey, ACP_STR_CASE_INSENSITIVE) == 0)
        {
            return &aDef[i];
        }
        else
        {
            /* continue */
        }
    }

    return sDefaultDef;
}

ACP_INLINE acp_bool_t aclConfConvertInt(aclConfContext *aContext,
                                        aclConfToken   *aToken,
                                        acp_sint32_t   *aSign,
                                        acp_uint64_t   *aValue)
{
    acp_uint64_t sValue;
    acp_sint32_t sIndex;
    acp_rc_t     sRC;

    ACP_STR_DECLARE_CONST(sStr);
    ACP_STR_INIT_CONST(sStr);

    ACE_ASSERT(aToken != NULL);
    ACE_ASSERT(aToken->mPtr != NULL);
    
    acpStrSetConstCStringWithLen(&sStr, aToken->mPtr, aToken->mLen);

    sRC = acpStrToInteger(&sStr, aSign, &sValue, &sIndex, 0, 0);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_TYPE_MISMATCH);
        return ACP_FALSE;
    }
    else
    {
        /* do nothing */
    }

    if (sIndex == sStr.mLength)
    {
        *aValue = sValue;
    }
    else if (sIndex == (sStr.mLength - 1))
    {
        switch (acpStrGetChar(&sStr, sIndex))
        {
            case 'k':
            case 'K':
                *aValue = sValue * 1024;
                break;
            case 'm':
            case 'M':
                *aValue = sValue * 1024 * 1024;
                break;
            case 'g':
            case 'G':
                *aValue = sValue * 1024 * 1024 * 1024;
                break;
            default:
                aclConfError(aContext, aToken, ACL_CONF_ERR_TYPE_MISMATCH);
                return ACP_FALSE;
        }

        if (*aValue > sValue)
        {
            /* do nothing */
        }
        else
        {
            aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
            return ACP_FALSE;
        }
    }
    else
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_TYPE_MISMATCH);
        return ACP_FALSE;
    }

    return ACP_TRUE;
}

ACP_INLINE void aclConfCallbackBoolean(aclConfContext          *aContext,
                                       acl_conf_def_t          *aDef,
                                       acl_conf_set_callback_t *aCallback,
                                       aclConfToken            *aToken)
{
    acp_bool_t sValue;

    ACP_UNUSED(aDef);

    switch (*aToken->mPtr)
    {
        case 't':
        case 'T':
        case 'y':
        case 'Y':
        case '1':
            sValue = ACP_TRUE;
            break;
        case 'f':
        case 'F':
        case 'n':
        case 'N':
        case '0':
            sValue = ACP_FALSE;
            break;
        default:
            aclConfError(aContext, aToken, ACL_CONF_ERR_TYPE_MISMATCH);
            return;
    }

    aContext->mCallbackResult = (*aCallback)(aContext->mDepth,
                                             aContext->mKey,
                                             aToken->mLine,
                                             &sValue,
                                             aContext->mUserContext);

    if (aContext->mCallbackResult != 0)
    {
        aContext->mRC = ACP_RC_ECANCELED;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclConfCallbackSInt32(aclConfContext          *aContext,
                                      acl_conf_def_t          *aDef,
                                      acl_conf_set_callback_t *aCallback,
                                      aclConfToken            *aToken)
{
    acp_uint64_t sTmpMax = (acp_uint64_t)ACP_SINT32_MAX;
    acp_uint64_t sTmpVal = ACP_UINT64_LITERAL(0);
    acp_sint32_t sSign;
    acp_sint32_t sValue;
    acp_bool_t   sConvIntRet;

    ACP_STR_DECLARE_CONST(sStr);
    ACP_STR_INIT_CONST(sStr);

    acpStrSetConstCStringWithLen(&sStr, aToken->mPtr, aToken->mLen);

    sConvIntRet = aclConfConvertInt(aContext, aToken, &sSign, &sTmpVal);

    if (sConvIntRet != ACP_TRUE)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if ((sSign == 1) && (sTmpVal > sTmpMax))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else if ((sSign == -1) && (sTmpVal > sTmpMax + 1))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        sValue = sSign * (acp_sint32_t)sTmpVal;
    }

    if (((aDef->mMin.mSInt32 != 0) || (aDef->mMax.mSInt32 != 0)) &&
        ((sValue < aDef->mMin.mSInt32) || (sValue > aDef->mMax.mSInt32)))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        /* do nothing */
    }

    aContext->mCallbackResult = (*aCallback)(aContext->mDepth,
                                             aContext->mKey,
                                             aToken->mLine,
                                             &sValue,
                                             aContext->mUserContext);

    if (aContext->mCallbackResult != 0)
    {
        aContext->mRC = ACP_RC_ECANCELED;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclConfCallbackUInt32(aclConfContext          *aContext,
                                      acl_conf_def_t          *aDef,
                                      acl_conf_set_callback_t *aCallback,
                                      aclConfToken            *aToken)
{
    acp_uint64_t sTmpVal = ACP_UINT64_LITERAL(0);
    acp_sint32_t sSign;
    acp_uint32_t sValue;
    acp_bool_t   sConvIntRet;

    ACP_STR_DECLARE_CONST(sStr);
    ACP_STR_INIT_CONST(sStr);

    acpStrSetConstCStringWithLen(&sStr, aToken->mPtr, aToken->mLen);

    sConvIntRet = aclConfConvertInt(aContext, aToken, &sSign, &sTmpVal);

    if (sConvIntRet != ACP_TRUE)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if ((sSign == -1) || (sTmpVal > ACP_UINT32_MAX))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        sValue = (acp_uint32_t)sTmpVal;
    }

    if (((aDef->mMin.mUInt32 != 0) || (aDef->mMax.mUInt32 != 0)) &&
        ((sValue < aDef->mMin.mUInt32) || (sValue > aDef->mMax.mUInt32)))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        /* do nothing */
    }

    aContext->mCallbackResult = (*aCallback)(aContext->mDepth,
                                             aContext->mKey,
                                             aToken->mLine,
                                             &sValue,
                                             aContext->mUserContext);

    if (aContext->mCallbackResult != 0)
    {
        aContext->mRC = ACP_RC_ECANCELED;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclConfCallbackSInt64(aclConfContext          *aContext,
                                      acl_conf_def_t          *aDef,
                                      acl_conf_set_callback_t *aCallback,
                                      aclConfToken            *aToken)
{
    acp_sint64_t sValue;
    acp_uint64_t sTmpMax = (acp_uint64_t)ACP_SINT64_MAX;
    acp_uint64_t sTmpVal = ACP_UINT64_LITERAL(0);
    acp_sint32_t sSign;
    acp_bool_t   sConvIntRet;

    ACP_STR_DECLARE_CONST(sStr);
    ACP_STR_INIT_CONST(sStr);

    acpStrSetConstCStringWithLen(&sStr, aToken->mPtr, aToken->mLen);

    sConvIntRet = aclConfConvertInt(aContext, aToken, &sSign, &sTmpVal);

    if (sConvIntRet != ACP_TRUE)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if ((sSign == 1) && (sTmpVal > sTmpMax))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else if ((sSign == -1) && (sTmpVal > sTmpMax + 1))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        sValue = (acp_sint64_t)sSign * sTmpVal;
    }

    if (((aDef->mMin.mSInt64 != 0) || (aDef->mMax.mSInt64 != 0)) &&
        ((sValue < aDef->mMin.mSInt64) || (sValue > aDef->mMax.mSInt64)))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        /* do nothing */
    }

    aContext->mCallbackResult = (*aCallback)(aContext->mDepth,
                                             aContext->mKey,
                                             aToken->mLine,
                                             &sValue,
                                             aContext->mUserContext);

    if (aContext->mCallbackResult != 0)
    {
        aContext->mRC = ACP_RC_ECANCELED;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclConfCallbackUInt64(aclConfContext          *aContext,
                                      acl_conf_def_t          *aDef,
                                      acl_conf_set_callback_t *aCallback,
                                      aclConfToken            *aToken)
{
    acp_uint64_t sValue;
    acp_sint32_t sSign;
    acp_bool_t   sConvIntRet;

    ACP_STR_DECLARE_CONST(sStr);
    ACP_STR_INIT_CONST(sStr);

    acpStrSetConstCStringWithLen(&sStr, aToken->mPtr, aToken->mLen);

    sConvIntRet = aclConfConvertInt(aContext, aToken, &sSign, &sValue);

    if (sConvIntRet != ACP_TRUE)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if (sSign == -1)
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        /* do nothing */
    }

    if (((aDef->mMin.mUInt64 != 0) || (aDef->mMax.mUInt64 != 0)) &&
        ((sValue < aDef->mMin.mUInt64) || (sValue > aDef->mMax.mUInt64)))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_VALUE_OVERFLOW);
        return;
    }
    else
    {
        /* do nothing */
    }

    aContext->mCallbackResult = (*aCallback)(aContext->mDepth,
                                             aContext->mKey,
                                             aToken->mLine,
                                             &sValue,
                                             aContext->mUserContext);

    if (aContext->mCallbackResult != 0)
    {
        aContext->mRC = ACP_RC_ECANCELED;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclConfCallbackString(aclConfContext          *aContext,
                                      acl_conf_def_t          *aDef,
                                      acl_conf_set_callback_t *aCallback,
                                      aclConfToken            *aToken)
{
    ACP_STR_DECLARE_CONST(sValue);
    ACP_STR_INIT_CONST(sValue);

    ACP_UNUSED(aDef);

    acpStrSetConstCStringWithLen(&sValue, aToken->mPtr, aToken->mLen);

    aContext->mCallbackResult = (*aCallback)(aContext->mDepth,
                                             aContext->mKey,
                                             aToken->mLine,
                                             &sValue,
                                             aContext->mUserContext);

    if (aContext->mCallbackResult != 0)
    {
        aContext->mRC = ACP_RC_ECANCELED;
    }
    else
    {
        /* do nothing */
    }
}

void aclConfAssignKey(aclConfContext *aContext, aclConfToken *aToken)
{
    acl_conf_def_t *sNewDef = NULL;
    acp_str_t      *sKey = NULL;

    /*
     * Token으로부터 Key를 얻어옴
     */
    sKey = &aContext->mKey[aContext->mDepth].mKey;
    acpStrSetConstCStringWithLen(sKey, aToken->mPtr, aToken->mLen);

    /*
     * Error: 상위 Key가 CONTAINER 타입이 아님
     */
    if (aContext->mBaseDef[aContext->mDepth] == NULL)
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_TYPE_MISMATCH);
        aContext->mIgnoreDepth++;
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Key로 Def를 얻어옴
     */
    sNewDef = aclConfFindDef(aContext->mBaseDef[aContext->mDepth], sKey);
    aContext->mCurrentDef[aContext->mDepth] = sNewDef;

    /*
     * Error: Key를 찾을 수 없음
     */
    if (sNewDef == NULL)
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_UNKNOWN_KEY);
        aContext->mIgnoreDepth++;
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Error: Key가 이미 정의되었음 (UNNAMED_CONTAINER 타입이면 생략)
     */
    if ((sNewDef->mLineFound != 0) &&
        (sNewDef->mType != ACL_CONF_TYPE_UNNAMED_CONTAINER))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_DUPLICATED_KEY);
        aContext->mIgnoreDepth++;
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Error: Multiplicity 초과함
     */
    if (aContext->mDepth > 0)
    {
        acl_conf_def_t *sCurDef = aContext->mCurrentDef[aContext->mDepth - 1];
        acp_sint32_t    sIndex  = aContext->mKey[aContext->mDepth - 1].mIndex;

        if ((sCurDef->mMultiplicity > 0) && (sCurDef->mMultiplicity <= sIndex))
        {
            /*
             * Error는 Parse Tree에 의해 발생됨
             */
            aContext->mIgnoreDepth++;
            return;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    /*
     * Key가 입력됨
     */
    sNewDef->mLineFound = aContext->mLine;

    aContext->mKey[aContext->mDepth].mID    = sNewDef->mID;
    aContext->mKey[aContext->mDepth].mIndex = 0;
    aContext->mDepth++;

    /*
     * SubDef 세팅
     */
    if (aContext->mDepth < ACL_CONF_DEPTH_MAX)
    {
        aclConfInitDef(sNewDef->mSubDef);

        aContext->mBaseDef[aContext->mDepth] = sNewDef->mSubDef;
    }
    else
    {
        /* do nothing */
    }
}

void aclConfAssignNull(aclConfContext *aContext, aclConfToken *aToken)
{
    void                    *sValue = NULL;
    acl_conf_def_t          *sDef = NULL;
    acl_conf_set_callback_t *sCallback = NULL;

    ACP_STR_DECLARE_CONST(sStr);

    /*
     * 현재 Key의 Def를 얻어옴
     */
    if (aContext->mDepth > 0)
    {
        sDef = aContext->mCurrentDef[aContext->mDepth - 1];

        ACE_DASSERT(sDef != NULL);
    }
    else
    {
        ACE_DASSERT(aContext->mDepth > 0);

        return;
    }

    /*
     * Error: Multiplicity 초과함
     */
    if ((sDef->mMultiplicity > 0) &&
        (sDef->mMultiplicity <= aContext->mKey[aContext->mDepth - 1].mIndex))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_TOO_MANY_VALUE);
        aContext->mIgnoreDepth++;
        aContext->mDepth--;
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Callback 함수를 얻어옴
     */
    if (sDef->mCallback != NULL)
    {
        sCallback = sDef->mCallback;
    }
    else
    {
        sCallback = aContext->mDefaultCallback;
    }

    /*
     * Value 세팅
     */
    switch (sDef->mType)
    {
        case ACL_CONF_TYPE_BOOLEAN:
            sValue = &sDef->mDefault.mBoolean;
            break;
        case ACL_CONF_TYPE_SINT32:
            sValue = &sDef->mDefault.mSInt32;
            break;
        case ACL_CONF_TYPE_UINT32:
            sValue = &sDef->mDefault.mUInt32;
            break;
        case ACL_CONF_TYPE_SINT64:
            sValue = &sDef->mDefault.mSInt64;
            break;
        case ACL_CONF_TYPE_UINT64:
            sValue = &sDef->mDefault.mUInt64;
            break;
        case ACL_CONF_TYPE_STRING:
            if (sDef->mDefault.mString != NULL)
            {
                ACP_STR_INIT_CONST(sStr);
                acpStrSetConstCString(&sStr, sDef->mDefault.mString);
                sValue = &sStr;
            }
            else
            {
                /* do nothing */
            }
            break;
        default:
            sCallback = NULL;
            break;
    }

    /*
     * Callback 함수 호출
     */
    if (sCallback != NULL)
    {
        aContext->mCallbackResult = (*sCallback)(aContext->mDepth,
                                                 aContext->mKey,
                                                 aToken->mLine,
                                                 sValue,
                                                 aContext->mUserContext);
        if (aContext->mCallbackResult != 0)
        {
            aContext->mRC = ACP_RC_ECANCELED;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    aContext->mKey[aContext->mDepth - 1].mIndex++;
}

void aclConfAssignValue(aclConfContext *aContext, aclConfToken *aToken)
{
    acl_conf_def_t          *sDef = NULL;
    acl_conf_set_callback_t *sCallback = NULL;

    /*
     * 현재 Key의 Def를 얻어옴
     */
    if (aContext->mDepth > 0)
    {
        sDef = aContext->mCurrentDef[aContext->mDepth - 1];

        ACE_DASSERT(sDef != NULL);
    }
    else
    {
        ACE_DASSERT(aContext->mDepth > 0);

        return;
    }

    /*
     * Error: Multiplicity 초과함
     */
    if ((sDef->mMultiplicity > 0) &&
        (sDef->mMultiplicity <= aContext->mKey[aContext->mDepth - 1].mIndex))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_TOO_MANY_VALUE);
        aContext->mIgnoreDepth++;
        aContext->mDepth--;
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Error: Value가 Quoted String일 경우 STRING 타입이어야 함
     */
    if ((aToken->mIsQuotedString == ACP_TRUE) &&
        (sDef->mType != ACL_CONF_TYPE_STRING))
    {
        aclConfError(aContext, aToken, ACL_CONF_ERR_TYPE_MISMATCH);
        return;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Callback 함수를 얻어옴
     */
    if (sDef->mCallback != NULL)
    {
        sCallback = sDef->mCallback;
    }
    else
    {
        sCallback = aContext->mDefaultCallback;
    }

    /*
     * Callback 함수 호출
     */
    if (sCallback != NULL)
    {
        switch (sDef->mType)
        {
            case ACL_CONF_TYPE_BOOLEAN:
                aclConfCallbackBoolean(aContext, sDef, sCallback, aToken);
                break;
            case ACL_CONF_TYPE_SINT32:
                aclConfCallbackSInt32(aContext, sDef, sCallback, aToken);
                break;
            case ACL_CONF_TYPE_UINT32:
                aclConfCallbackUInt32(aContext, sDef, sCallback, aToken);
                break;
            case ACL_CONF_TYPE_SINT64:
                aclConfCallbackSInt64(aContext, sDef, sCallback, aToken);
                break;
            case ACL_CONF_TYPE_UINT64:
                aclConfCallbackUInt64(aContext, sDef, sCallback, aToken);
                break;
            case ACL_CONF_TYPE_STRING:
                aclConfCallbackString(aContext, sDef, sCallback, aToken);
                break;
            case ACL_CONF_TYPE_CONTAINER:
            case ACL_CONF_TYPE_UNNAMED_CONTAINER:
                aclConfError(aContext, aToken, ACL_CONF_ERR_TYPE_MISMATCH);
                break;
            default:
                aclConfError(aContext, aToken, ACL_CONF_ERR_UNKNOWN_TYPE);
                break;
        }
    }
    else
    {
        /* do nothing */
    }

    aContext->mKey[aContext->mDepth - 1].mIndex++;
}

void aclConfAssignDefault(aclConfContext *aContext, acl_conf_def_t *aDef)
{
    void                    *sValue = NULL;
    acl_conf_set_callback_t *sCallback = NULL;

    ACP_STR_DECLARE_CONST(sStr);

    /*
     * Callback 함수를 얻어옴
     */
    if (aDef->mCallback != NULL)
    {
        sCallback = aDef->mCallback;
    }
    else
    {
        sCallback = aContext->mDefaultCallback;
    }

    /*
     * Key 세팅
     */
    acpStrSetConstCString(&aContext->mKey[aContext->mDepth].mKey, aDef->mKey);
    aContext->mKey[aContext->mDepth].mIndex = 0;
    aContext->mKey[aContext->mDepth].mID    = aDef->mID;
    aContext->mDepth++;

    /*
     * Value 세팅
     */
    switch (aDef->mType)
    {
        case ACL_CONF_TYPE_BOOLEAN:
            sValue = &aDef->mDefault.mBoolean;
            break;
        case ACL_CONF_TYPE_SINT32:
            sValue = &aDef->mDefault.mSInt32;
            break;
        case ACL_CONF_TYPE_UINT32:
            sValue = &aDef->mDefault.mUInt32;
            break;
        case ACL_CONF_TYPE_SINT64:
            sValue = &aDef->mDefault.mSInt64;
            break;
        case ACL_CONF_TYPE_UINT64:
            sValue = &aDef->mDefault.mUInt64;
            break;
        case ACL_CONF_TYPE_STRING:
            if (aDef->mDefault.mString != NULL)
            {
                ACP_STR_INIT_CONST(sStr);
                acpStrSetConstCString(&sStr, aDef->mDefault.mString);
                sValue = &sStr;
            }
            else
            {
                /* do nothing */
            }
            break;
        default:
            sCallback = NULL;
            break;
    }

    /*
     * Callback 함수 호출
     */
    if (sCallback != NULL)
    {
        /* BUGBUG : default value를 assign 할 때 configure파일에 정의되어
         * 있지 않은 key값에 대한 line number는 ACL_CONF_TYPE_STRING(-1)으로
         * 세팅하여 리턴한다.
         */
        aContext->mCallbackResult = (*sCallback)(aContext->mDepth,
                                                 aContext->mKey,
                                                 ACL_CONF_UNDEFINED_KEY,
                                                 sValue,
                                                 aContext->mUserContext);
        if (aContext->mCallbackResult != 0)
        {
            aContext->mRC = ACP_RC_ECANCELED;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    /*
     * Depth 원복
     */
    aContext->mDepth--;
}
