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
 * $Id: aceException.c 8661 2009-11-16 04:50:23Z djin $
 ******************************************************************************/

/**
 * @example sampleAceException.c
 */

#include <acpPrintf.h>
#include <aceException.h>
#include <aceMsgTable.h>
#include <aceError.h>


static ace_error_callback_t ***gAceErrorCallback = NULL;

ACP_INLINE ace_error_callback_t *aceGetErrorCallback(acp_sint32_t aProductID,
                                                     acp_sint32_t aErrorLevel)
{
    if ((aProductID < ACE_PRODUCT_MIN) || (aProductID > ACE_PRODUCT_MAX))
    {
        return NULL;
    }
    else
    {
        /* do nothing */
    }

    if ((aErrorLevel < ACE_LEVEL_MIN) || (aErrorLevel > ACE_LEVEL_MAX))
    {
        return NULL;
    }
    else
    {
        /* do nothing */
    }

    if (gAceErrorCallback[aProductID] != NULL)
    {
        return gAceErrorCallback[aProductID][aErrorLevel];
    }
    else
    {
        return NULL;
    }
}

/**
 * sets error callback function
 * @param aProductID product id
 * @param aCallbacks array of 16 callback functions for each error level
 */
ACP_EXPORT void aceSetErrorCallback(acp_sint32_t           aProductID,
                                    ace_error_callback_t **aCallbacks)
{
    if ((aProductID >= ACE_PRODUCT_MIN) && (aProductID <= ACE_PRODUCT_MAX))
    {
        gAceErrorCallback[aProductID] = aCallbacks;
    }
    else
    {
        /* do nothing */
    }
}

ACP_EXPORT void aceDump(ace_exception_t *aException, acp_std_file_t *aFile)
{
#if defined(ACP_CFG_DEBUG)
    acp_sint32_t i;

    (void)acpFprintf(aFile,
                     "Level-%d Error %s\n",
                     ACE_ERROR_LEVEL(aException->mErrorCode),
                     aException->mErrorMsg);

    (void)acpFprintf(aFile, "  ==== Exception Stack Dump ====\n");

    for (i = aException->mExprCount - 1; i >= 0; i--)
    {
        (void)acpFprintf(aFile,
                         "    %3d: %s\n",
                         aException->mExprCount - i,
                         aException->mExpr[i]);
    }

    (void)acpFprintf(aFile, "  ==== End of Dump ====\n");
#else
    if (aException->mExpr[0] != '\0')
    {
        (void)acpFprintf(aFile,
                         "Level-%d Error in %s %s\n",
                         ACE_ERROR_LEVEL(aException->mErrorCode),
                         aException->mExpr,
                         aException->mErrorMsg);
    }
    else
    {
        (void)acpFprintf(aFile,
                         "Level-%d Error %s\n",
                         ACE_ERROR_LEVEL(aException->mErrorCode),
                         aException->mErrorMsg);
    }
#endif
}

ACP_EXPORT void aceSetError(void            *aContext,
                            ace_exception_t *aException,
                            acp_uint32_t     aErrorLevel,
                            ace_msgid_t      aErrorCode, ...)
{
    ace_error_callback_t *sCallback = NULL;
    const acp_char_t     *sProduct;
    acp_sint32_t          sLen = 0;
    va_list               sArgs;

    /*
     * 에러코드 세팅
     */
    aException->mErrorCode = (aErrorLevel << ACE_LEVEL_OFFSET) | aErrorCode;

    /*
     * 에러코드 출력
     */
    sProduct = aceMsgTableGetProduct(aErrorCode);

    if (sProduct != NULL)
    {
        (void)acpSnprintf(aException->mErrorMsg,
                          sizeof(aException->mErrorMsg),
                          "[%s-%05x] %n",
                          sProduct,
                          ACE_ERROR_SUBCODE(aErrorCode),
                          &sLen);
    }
    else
    {
        (void)acpSnprintf(aException->mErrorMsg,
                          sizeof(aException->mErrorMsg),
                          "[E%02X-%05x] %n",
                          ACE_ERROR_PRODUCT(aErrorCode),
                          ACE_ERROR_SUBCODE(aErrorCode),
                          &sLen);
    }

    /*
     * 에러메세지 출력
     */
    va_start(sArgs, aErrorCode);

    (void)acpVsnprintf(aException->mErrorMsg + sLen,
                       sizeof(aException->mErrorMsg) - sLen,
                       aceMsgTableGetErrMsgFormat(aErrorCode),
                       sArgs);

    va_end(sArgs);

    /*
     * 콜백 함수 호출
     */
    sCallback = aceGetErrorCallback(ACE_ERROR_PRODUCT(aErrorCode), aErrorLevel);

    if (sCallback != NULL)
    {
        (*sCallback)(aContext);
    }
    else
    {
        /* do nothing */
    }
}

ACP_EXPORT void aceErrorSetCallbackArea(ace_error_callback_t*** aError)
{
    gAceErrorCallback = aError;
}

ACP_EXPORT ace_error_callback_t*** aceErrorGetCallbackArea(void)
{
    return gAceErrorCallback;
}

