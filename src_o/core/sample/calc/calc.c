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
 * $Id: calc.c 1414 2007-12-07 08:14:12Z shsuh $
 ******************************************************************************/

#include <calc.h>


ACP_INLINE void calcInitVar(calcContext *aContext)
{
    acp_sint32_t i;

    for (i = 0; i < CALC_VAR_MAX; i++)
    {
        ACP_STR_INIT_DYNAMIC(aContext->mVar[i].mName, 0, 16);
    }
}

ACP_INLINE void calcFinalVar(calcContext *aContext)
{
    acp_sint32_t i;

    for (i = 0; i < CALC_VAR_MAX; i++)
    {
        ACP_STR_FINAL(aContext->mVar[i].mName);
    }
}

acp_sint32_t calcGetVar(calcContext *aContext, calcToken *aToken)
{
    acp_sint32_t i;

    ACP_STR_DECLARE_CONST(sName);
    ACP_STR_INIT_CONST(sName);

    acpStrSetConstCStringWithLen(&sName, aToken->mPtr, aToken->mLen);

    for (i = 0; i < CALC_VAR_MAX; i++)
    {
        if (acpStrEqual(&aContext->mVar[i].mName, &sName, 0) == ACP_TRUE)
        {
            return aContext->mVar[i].mValue;
        }
        else
        {
            /* continue */
        }
    }

    return 0;
}

void calcSetVar(calcContext  *aContext,
                calcToken    *aToken,
                acp_sint32_t  aValue)
{
    acp_sint32_t sIndex = -1;
    acp_sint32_t i;

    ACP_STR_DECLARE_CONST(sName);
    ACP_STR_INIT_CONST(sName);

    acpStrSetConstCStringWithLen(&sName, aToken->mPtr, aToken->mLen);

    for (i = 0; i < CALC_VAR_MAX; i++)
    {
        if (acpStrEqual(&aContext->mVar[i].mName, &sName, 0) == ACP_TRUE)
        {
            aContext->mVar[i].mValue = aValue;
            return;
        }
        else
        {
            if ((sIndex == -1) &&
                (acpStrGetLength(&aContext->mVar[i].mName) == 0))
            {
                sIndex = i;
            }
            else
            {
                /* do nothing */
            }
        }
    }

    if (sIndex != -1)
    {
        (void)acpStrCpy(&aContext->mVar[sIndex].mName, &sName);
        aContext->mVar[sIndex].mValue = aValue;
    }
    else
    {
        /* continue */
    }
}

acp_sint32_t main(void)
{
    void         *sParser;
    calcToken     sToken;
    acp_sint32_t  sTokenID;
    acp_rc_t      sRC;

    ACP_STR_DECLARE_STATIC(sInput, 128);

    ACL_CONTEXT_DECLARE(calcContext, sContext);

    ACP_STR_INIT_STATIC(sInput);

    ACL_CONTEXT_INIT();

    calcInitVar(&sContext);

    while (1)
    {
        sRC = acpStdGetString(ACP_STD_IN, &sInput);

        ACE_TEST(ACP_RC_NOT_SUCCESS(sRC));

        sContext.mCursor = acpStrGetBuffer(&sInput);
        sContext.mLimit  = sContext.mCursor + acpStrGetLength(&sInput) - 1;
        sContext.mError  = ACP_FALSE;

        sParser = calcParseAlloc();

        do
        {
            sTokenID = calcGetToken(&sContext);

            sToken.mPtr = sContext.mToken;
            sToken.mLen = (acp_sint32_t)(sContext.mCursor - sContext.mToken);

            if (sTokenID == CALC_TOKEN_INVALID)
            {
                (void)acpPrintf("invalid token '%.*s'\n",
                                sToken.mLen,
                                sToken.mPtr);
                break;
            }
            else
            {
                calcParse(sParser, sTokenID, sToken, &sContext);

                if (sContext.mError == ACP_TRUE)
                {
                    break;
                }
                else
                {
                    /* do nothing */
                }
            }
        } while (sTokenID != CALC_TOKEN_EOF);

        calcParseFree(sParser);
    }

    calcFinalVar(&sContext);

    ACL_CONTEXT_FINAL();

    return 0;

    ACE_EXCEPTION_END;

    return 1;
}
