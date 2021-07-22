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

#include <cmnIB.h>
#include <cmErrorCodeClient.h>

cmnIB gIB;

#define CMN_IB_FUNC_SYMBOL(aFuncName) \
    { offsetof(cmnIBFuncs, aFuncName), #aFuncName }

ACI_RC cmnIBInitialize(void)
{
    acp_rc_t       sRC = ACP_RC_SUCCESS;
    acp_sint32_t   i;
    acp_sint32_t   sCount;
    void         **sSymbol;

    ACI_TEST_RAISE(gIB.mRdmaCmHandle.mHandle != NULL, AlreadyLoaded);

    gIB.mLibErrorCode = cmERR_IGNORE_NoError;
    gIB.mLibErrorMsg[0] = '\0';

    /* open dynamic library */
    sRC = acpDlOpen(&gIB.mRdmaCmHandle, NULL, CMN_IB_RDMACM_LIB_NAME, ACP_TRUE);
    ACI_TEST_RAISE(sRC != ACP_RC_SUCCESS, ERR_DLOPEN);

    /* load function symbols */
    struct
    {
        acp_sint64_t      mOffset;
        const acp_char_t *mFuncName;
    } sSymbols[] = {
        CMN_IB_FUNC_SYMBOL(rsocket),
        CMN_IB_FUNC_SYMBOL(rbind),
        CMN_IB_FUNC_SYMBOL(rlisten),
        CMN_IB_FUNC_SYMBOL(raccept),
        CMN_IB_FUNC_SYMBOL(rconnect),
        CMN_IB_FUNC_SYMBOL(rshutdown),
        CMN_IB_FUNC_SYMBOL(rclose),
        CMN_IB_FUNC_SYMBOL(rrecv),
        CMN_IB_FUNC_SYMBOL(rsend),
        CMN_IB_FUNC_SYMBOL(rread),
        CMN_IB_FUNC_SYMBOL(rwrite),
        CMN_IB_FUNC_SYMBOL(rpoll),
        CMN_IB_FUNC_SYMBOL(rselect),
        CMN_IB_FUNC_SYMBOL(rgetpeername),
        CMN_IB_FUNC_SYMBOL(rgetsockname),
        CMN_IB_FUNC_SYMBOL(rsetsockopt),
        CMN_IB_FUNC_SYMBOL(rgetsockopt),
        CMN_IB_FUNC_SYMBOL(rfcntl)
    };

    sCount = ACI_SIZEOF(sSymbols) / ACI_SIZEOF(sSymbols[0]);
    for (i = 0; i < sCount; ++i)
    {
        sSymbol = (void **)((acp_char_t *)&gIB.mFuncs + sSymbols[i].mOffset);
        *sSymbol = acpDlSym(&gIB.mRdmaCmHandle, sSymbols[i].mFuncName);

        ACI_TEST_RAISE(*sSymbol == NULL, ERR_DLSYM);
    }

    ACI_EXCEPTION_CONT(AlreadyLoaded);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_DLOPEN)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_DLOPEN,
                                CMN_IB_RDMACM_LIB_NAME,
                                acpDlError(&gIB.mRdmaCmHandle)));
    }
    ACI_EXCEPTION(ERR_DLSYM)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_DLSYM,
                                CMN_IB_RDMACM_LIB_NAME,
                                acpDlError(&gIB.mRdmaCmHandle)));
    }
    ACI_EXCEPTION_END;

    gIB.mLibErrorCode = aciGetErrorCode();
    acpSnprintf(gIB.mLibErrorMsg, CMN_IB_LIB_ERROR_MSG_LEN, "%s", aciGetErrorMsg(gIB.mLibErrorCode));

    (void)cmnIBFinalize();

    return ACI_FAILURE;     
}

ACI_RC cmnIBFinalize(void)
{
    if (gIB.mRdmaCmHandle.mHandle != NULL)
    {
        (void)acpDlClose(&gIB.mRdmaCmHandle);
        gIB.mRdmaCmHandle.mHandle = NULL;
    }
    else
    {
        /* already closed */
    }

    return ACI_SUCCESS;
}

