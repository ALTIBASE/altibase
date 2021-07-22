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
 * $Id: testAcpDl.c 4816 2009-03-11 13:43:04Z sjkim $
 ******************************************************************************/

#include <act.h>
#include <acpDl.h>
#include <acpEnv.h>
#include <acpStr.h>


#define LIB_NAME "testAcpDl"

#define SIG_VAL 0xa5a5c3c3

acp_uint32_t testAcpDlSymFunc(void)
{
    return SIG_VAL;
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_char_t      *sName = LIB_NAME;
    acp_dl_t         sDl;
    acp_rc_t         sRC;
    void            *sSymbol;
    acp_char_t      *sHomeDir;
    acp_char_t       sDir[1024];

#if !defined(ALTI_CFG_OS_WINDOWS)
    acp_uint32_t (*sFuncPtr)(void);
#endif
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    /*
     * BUG-30240
     * To check dlopening NULL
     * BUG-30270
     * In windows, opening NULL is not supported. The bug is registered as a
     * known issue
     */
    sRC = acpDlOpen(&sDl, NULL, NULL, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

#if !defined(ALTI_CFG_OS_WINDOWS)
# if defined(ALTI_CFG_OS_HPUX)
    ACT_CHECK(sDl.mIsSelf == ACP_TRUE);
# endif
    sFuncPtr = (acp_uint32_t (*)(void))acpDlSym(&sDl, "testAcpDlSymFunc");
    ACT_CHECK_DESC(sFuncPtr != NULL,
                   ("testAcpDlSymFunc symbol must be found"));

    if(sFuncPtr != NULL)
    {
        ACT_CHECK_DESC(sFuncPtr == testAcpDlSymFunc,
                       ("Return value of acpDlSym is wrong"));
        ACT_CHECK(sFuncPtr() == (acp_uint32_t)SIG_VAL);
    }
    else
    {
        /* Do nothing */
    }

    sFuncPtr = (acp_uint32_t (*)(void))
        acpDlSym(&sDl, "testAcpDlSymFuncMUSTFAIL");
    ACT_CHECK_DESC(sFuncPtr == NULL,
                   ("acpDlSym is finding non-exist symbol"));
    ACT_CHECK_DESC(acpDlError(&sDl) != NULL,
                   ("Error message must be set up"));

#endif
    
    sRC = acpDlClose(&sDl);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK_DESC(acpDlError(&sDl) == NULL,
                   ("Error message must be cleared"));

    sRC = acpEnvGet( ALTIBASE_ENV_PREFIX"HOME", &sHomeDir );
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSnprintf(sDir, sizeof(sDir), "%s/lib", sHomeDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpDlOpen(&sDl, sDir, sName, ACP_TRUE);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        ACT_CHECK(acpDlError(&sDl) == NULL);
    }
    else
    {
        ACT_ERROR(("acpDlOpen failed for %s: %s", sName, acpDlError(&sDl)));
    }
    
#if defined(ALTI_CFG_OS_HPUX)
    ACT_CHECK(sDl.mIsSelf == ACP_FALSE);
#endif

    sSymbol = acpDlSym(&sDl, "libtestAcpDlFunc1");
    ACT_CHECK_DESC(sSymbol != NULL,
                   ("libtestAcpDlFunc1 symbol not found"));
    ACT_CHECK(acpDlError(&sDl) == NULL);

    sSymbol = acpDlSym(&sDl, "libtestAcpDlFunc2");
    ACT_CHECK_DESC(sSymbol != NULL,
                   ("libtestAcpDlFunc2 symbol not found"));
    ACT_CHECK(acpDlError(&sDl) == NULL);

    sSymbol = acpDlSym(&sDl, "libtestAcpDlFunc3");
    ACT_CHECK_DESC(sSymbol == NULL,
                   ("libtestAcpDlFunc3 symbol should not be exist"));
    ACT_CHECK(acpDlError(&sDl) != NULL);

    sRC = acpDlClose(&sDl);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpDlError(&sDl) == NULL);

    ACT_TEST_END();

    return 0;
}
