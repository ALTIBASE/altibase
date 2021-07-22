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
 * $Id: testAciErrorMgr.c 10969 2010-04-29 09:04:59Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <aciTypes.h>
#include <acpCStr.h>
#include <aciErrorMgr.h>

#define ACI_TEST_ERROR_CODE0 0
#define ACI_TEST_ERROR_MSG0 "Test error msg0"

#define ACI_TEST_ERROR_CODE1 1
#define ACI_TEST_ERROR_MSG1 "Test error msg1"

#define ACI_TEST_ERROR_CODE2 2
#define ACI_TEST_ERROR_MSG2 "Test error msg2"

#define ACI_TEST_STATE0 "St0"
#define ACI_TEST_STATE1 "St1"
#define ACI_TEST_STATE2 "St2"

#define ACI_TEST_ERROR_ACTION_FATAL   (1 | ACI_E_ACTION_FATAL)
#define ACI_TEST_ERROR_ACTION_ABORT   (1 | ACI_E_ACTION_ABORT)
#define ACI_TEST_ERROR_ACTION_IGNORE  (1 | ACI_E_ACTION_IGNORE)
#define ACI_TEST_ERROR_ACTION_RETRY   (1 | ACI_E_ACTION_RETRY)
#define ACI_TEST_ERROR_ACTION_REBUILD (1 | ACI_E_ACTION_REBUILD)

#define ACI_TEST_ERROR_MSB "testAciErrorMsb.msb"
#define ACI_TEST_MSB_MSG0 "No ID module error"
#define ACI_TEST_MSB_MSG1 "No character set is found."

#define ACI_TEST_ERROR_CODE45 45
#define ACI_TEST_ERROR_FILENAME "filename.ext"
#define ACI_TEST_ERROR_MSG_CMP  "Unable to open a file [filename.ext]"

aci_client_error_factory_t  gTestErrorFactory[] =
{
    {ACI_TEST_STATE0, ACI_TEST_ERROR_MSG0},
    {ACI_TEST_STATE1, ACI_TEST_ERROR_MSG1},
    {ACI_TEST_STATE2, ACI_TEST_ERROR_MSG2}
};

void testSetGetErrorCode(void)
{
    acp_uint32_t  sErrorCode = 0;
    acp_uint32_t  sSystemErrorCode = 0;
    acp_char_t   *sErrorMsg = NULL;

    /* Case1 */
    (void)aciSetErrorCode(ACI_TEST_ERROR_CODE1);
    sErrorCode = aciGetErrorCode();

    ACT_CHECK(sErrorCode == ACI_TEST_ERROR_CODE1);

    /* Case2 */
    (void)aciSetErrorCodeAndMsg(ACI_TEST_ERROR_CODE1, ACI_TEST_ERROR_MSG1);

    sErrorMsg  = aciGetErrorMsg(aciGetErrorCode());
    sErrorCode = aciGetErrorCode();

    ACT_CHECK(sErrorCode == ACI_TEST_ERROR_CODE1);
    ACT_CHECK(acpCStrCmp(sErrorMsg, ACI_TEST_ERROR_MSG1, 256) == 0);

    /* Case3 */
    ACP_RC_SET_OS_ERROR(ACP_RC_EINVAL);
    (void)aciSetErrorCode(ACI_TEST_ERROR_CODE2);

    sErrorCode = aciGetErrorCode();
    ACT_CHECK(sErrorCode == ACI_TEST_ERROR_CODE2);

    sSystemErrorCode = aciGetSystemErrno();
    ACT_CHECK(sSystemErrorCode == ACP_RC_EINVAL);

}

void testTestRaiseFired(void)
{
    ACI_TEST_RAISE(ACP_TRUE, err_test);

    ACT_CHECK(ACP_FALSE);

    return;

    ACI_EXCEPTION(err_test);
    {
        ACT_CHECK(ACP_TRUE);
    }

    ACI_EXCEPTION_END;

    return;
}

void testTestRaiseMissed(void)
{
    ACI_TEST_RAISE(ACP_FALSE, err_test);

    ACT_CHECK(ACP_TRUE);

    return;

    ACI_EXCEPTION(err_test);
    {
        ACT_CHECK(ACP_FALSE);
    }

    ACI_EXCEPTION_END;

    return;
}

void testRaise(void)
{
    ACI_RAISE(err_test);

    ACT_CHECK(ACP_FALSE);

    return;

    ACI_EXCEPTION(err_test);
    {
        ACT_CHECK(ACP_TRUE);
    }
 
    ACI_EXCEPTION_END;

    return;
}

void testAciSet(void)
{
    acp_char_t  *sErrorFile = NULL;
    acp_uint32_t sErrorCode = 0;
    acp_uint32_t sErrorLine = 0;

    ACI_SET(aciSetErrorCode(ACI_TEST_ERROR_CODE1));

    sErrorFile = aciGetErrorFile();
    ACT_CHECK(acpCStrCmp(sErrorFile, __FILE__, 256) == 0);

    sErrorLine = aciGetErrorLine();
    ACT_CHECK(sErrorLine == 142);/* 142 is number of line with ACI_SET */

    sErrorCode = aciGetErrorCode();
    ACT_CHECK(sErrorCode == ACI_TEST_ERROR_CODE1);

}

void testActions(void)
{
    (void)aciSetErrorCode(ACI_TEST_ERROR_ACTION_ABORT);
    ACT_CHECK(aciIsAbort() == ACI_SUCCESS);

    (void)aciSetErrorCode(ACI_TEST_ERROR_ACTION_FATAL);
    ACT_CHECK(aciIsFatal() == ACI_SUCCESS);

    (void)aciSetErrorCode(ACI_TEST_ERROR_ACTION_IGNORE);
    ACT_CHECK(aciIsIgnore() == ACI_SUCCESS);

    (void)aciSetErrorCode(ACI_TEST_ERROR_ACTION_RETRY);
    ACT_CHECK(aciIsRetry() == ACI_SUCCESS);

    (void)aciSetErrorCode(ACI_TEST_ERROR_ACTION_REBUILD);
    ACT_CHECK(aciIsRebuild() == ACI_SUCCESS);
}

void testClearError(void)
{
    acp_uint32_t sErrorCode = 0;
    acp_uint32_t  sSystemErrorCode = 0;
    acp_char_t   *sErrorMsg = NULL;

    ACP_RC_SET_OS_ERROR(ACP_RC_EINVAL);
    ACI_SET(aciSetErrorCodeAndMsg(ACI_TEST_ERROR_CODE1,
                                  ACI_TEST_ERROR_MSG1));

    sErrorCode = aciGetErrorCode();
    ACT_CHECK(sErrorCode == ACI_TEST_ERROR_CODE1);

    sErrorMsg  = aciGetErrorMsg(aciGetErrorCode());
    ACT_CHECK(acpCStrCmp(sErrorMsg, ACI_TEST_ERROR_MSG1, 256) == 0);

    sSystemErrorCode = aciGetSystemErrno();
    ACT_CHECK(sSystemErrorCode == ACP_RC_EINVAL);

    aciClearError();

    sErrorCode = aciGetErrorCode();
    ACT_CHECK(sErrorCode != ACI_TEST_ERROR_CODE1);

    sSystemErrorCode = aciGetSystemErrno();
    ACT_CHECK(sSystemErrorCode == 0);

    sErrorMsg  = aciGetErrorMsg(aciGetErrorCode());
    ACT_CHECK(acpCStrCmp(sErrorMsg,"", 256) == 0);

}


void testFind(void)
{
    (void)aciSetErrorCode(ACI_TEST_ERROR_CODE1);

    ACT_CHECK(aciFindErrorCode(ACI_TEST_ERROR_CODE1) == ACI_SUCCESS);
    ACT_CHECK(aciFindErrorCode(ACI_TEST_ERROR_CODE2) == ACI_FAILURE);

    (void)aciSetErrorCode(ACI_TEST_ERROR_ACTION_ABORT);

    ACT_CHECK(aciFindErrorAction(ACI_E_ACTION_ABORT) == ACI_SUCCESS);
    ACT_CHECK(aciFindErrorAction(ACI_E_ACTION_RETRY) == ACI_FAILURE);
}

void testGetErrorMgr(void)
{
    struct aci_error_mgr_t *sErrorMgr = NULL;
    struct aci_error_mgr_t *sErrorMgrTest = NULL;

    acp_uint32_t sErrorCode = 0;

    sErrorMgr = aciGetErrorMgr();

    ACT_CHECK(sErrorMgr != NULL);

    sErrorMgrTest = aciSetErrorCode(ACI_TEST_ERROR_CODE2);

    ACT_CHECK(sErrorMgr == sErrorMgrTest);

    sErrorCode = aciGetErrorCode();

    ACT_CHECK(sErrorCode == ACI_TEST_ERROR_CODE2);
}

void testSetClientErrorCode(void)
{
    aci_client_error_mgr_t sClientErrorMgr;

    aciSetClientErrorCode(&sClientErrorMgr,
                          gTestErrorFactory,
                          ACI_TEST_ERROR_CODE1,
                          NULL);

    ACT_CHECK(sClientErrorMgr.mErrorCode == ACI_TEST_ERROR_CODE1);
    ACT_CHECK(acpCStrCmp(sClientErrorMgr.mErrorState,
                         ACI_TEST_STATE1, 256) == 0);
    ACT_CHECK(acpCStrCmp(sClientErrorMgr.mErrorMessage,
                         ACI_TEST_ERROR_MSG1, 256) == 0);

}

void testRegisterErrorMsb(void)
{
    acp_char_t   *sErrorMsg = NULL;

    ACT_CHECK(aciRegistErrorMsb(ACI_TEST_ERROR_MSB) == ACI_SUCCESS);

    (void)aciSetErrorCode(ACI_TEST_ERROR_CODE0);

    sErrorMsg  = aciGetErrorMsg(aciGetErrorCode());
    ACT_CHECK(acpCStrCmp(sErrorMsg, ACI_TEST_MSB_MSG0, 256) == 0);


    (void)aciSetErrorCode(ACI_TEST_ERROR_CODE1);

    sErrorMsg  = aciGetErrorMsg(aciGetErrorCode());
    ACT_CHECK(acpCStrCmp(sErrorMsg, ACI_TEST_MSB_MSG1, 256) == 0);

    ACT_CHECK(aciGetErrorArgCount(ACI_TEST_ERROR_CODE45) == 1);

    (void)aciSetErrorCode(ACI_TEST_ERROR_CODE45, ACI_TEST_ERROR_FILENAME);

    sErrorMsg  = aciGetErrorMsg(aciGetErrorCode());

    ACT_CHECK(acpCStrCmp(sErrorMsg, ACI_TEST_ERROR_MSG_CMP, 256) == 0);

}

void testCreateErrorMgr(void)
{
    acp_uint32_t  sErrorCode = 0;

    ACT_CHECK(aciCreateErrorMgr() == ACI_SUCCESS);

    (void)aciSetErrorCode(ACI_TEST_ERROR_CODE1);
    sErrorCode = aciGetErrorCode();

    ACT_CHECK(sErrorCode == ACI_TEST_ERROR_CODE1);
}


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testSetGetErrorCode();

    testTestRaiseFired();
    testTestRaiseMissed();

    testRaise();

    testAciSet();

    testClearError();

    testActions();

    testFind();

    testGetErrorMgr();

    testSetClientErrorCode();

    testRegisterErrorMsb();

    testCreateErrorMgr();

    ACT_TEST_END();

    return 0;
}
