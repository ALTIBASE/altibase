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
 * $Id: testAceMsgTable.c 8694 2009-11-18 01:58:22Z djin $
 ******************************************************************************/

#include <acpDl.h>
#include <act.h>
#include <aclContext.h>
#include <testMsg.h>
#include <aceMsgTable.h>

/*
 * When additional Shared Object test is needed, uncomment the line below
#define TEST_ACE_DL 1
*/

ACE_MSG_DECLARE_GLOBAL_AREA;
ACE_ERROR_CALLBACK_DECLARE_GLOBAL_AREA;

typedef struct testContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;
    acp_uint32_t mCount;
} testContext;

static acp_str_t gTestErrMsg1 = ACP_STR_CONST("[E80-00004] (null)");
static acp_str_t gTestErrMsg2 = ACP_STR_CONST(
    "[TST-00004] msg test error 4 100; message goes long");
static acp_str_t gTestErrMsg3 = ACP_STR_CONST(
    "[TST-00004] msg test error 4 100 [KOR]");


static testContext *gContextPtr = NULL;
static acp_sint32_t gStage = 0;

static void testCallback(testContext *aContext)
{
    ACT_CHECK(aContext == gContextPtr);

    ACT_CHECK(ACE_GET_ERROR_COMPARE(ACE_GET_ERROR(),
                                    TEST_ERR_ERROR4) == ACP_TRUE);
    switch (gStage)
    {
#if defined(TEST_ACE_DL)
    case 0: case 1:
#else
    case 0:
#endif
        ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg2,
                                        ACE_GET_ERROR_MSG(),
                                        0) == 0,
                       ("error message should be \"%S\" but \"%s\"",
                        &gTestErrMsg2,
                        ACE_GET_ERROR_MSG()));
        break;
#if defined(TEST_ACE_DL)
    case 2: case 3:
#else
    case 1:
#endif
        ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg3,
                                        ACE_GET_ERROR_MSG(),
                                        0) == 0,
                       ("error message should be \"%S\" but \"%s\"",
                        &gTestErrMsg3,
                        ACE_GET_ERROR_MSG()));
        break;
#if defined(TEST_ACE_DL)
    case 4: case 5:
#else
    case 2:
#endif
        ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg1,
                                        ACE_GET_ERROR_MSG(),
                                        0) == 0,
                       ("error message should be \"%S\" but \"%s\"",
                        &gTestErrMsg1,
                        ACE_GET_ERROR_MSG()));
        break;
    default:
        break;
    }

    gStage++;
}

static void testFunc(testContext *aContext)
{
    ACE_SET(ACE_SET_ERROR(ACE_LEVEL_1, TEST_ERR_ERROR4, 100));
}


static ace_error_callback_t *gTestErrorCallback[] =
{
    NULL,
    (ace_error_callback_t *)testCallback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

#define TEST_MSG_DL     "testAceMsg"
#define TEST_MSG_DLERR  "testAceMsgError"
#define TEST_MSG_FUNC   "libtestAceMsg"
#define TEST_MSG_INIT   "libtestAceMsgInit"
#define TEST_MSG_SET    "libtestAceMsgSet"
#define TEST_ERR_SET    "libtestAceErrSet"
#define TEST_MSG_FINI   "libtestAceMsgFini"
#define TEST_MAXLENGTH  (1024)

typedef void     testAceMsgInitType(void);
typedef acp_rc_t testAceMsgFuncType(void*, acp_char_t*, acp_size_t);
typedef void     testAceMsgSetType(void*);

static void testDlAceMsg(testContext* aContext,
                         acp_str_t*   aDlPath,
                         acp_str_t*   aMsgShouldBe)
{
#if !defined(TEST_ACE_DL)
    ACP_UNUSED(aContext);
    ACP_UNUSED(aDlPath);
    ACP_UNUSED(aMsgShouldBe);
#else
    acp_rc_t        sRC;
    acp_char_t*     sDlName;
    acp_dl_t        sDl;

    testAceMsgInitType* sDlTestInit;
    testAceMsgFuncType* sDlTestFunc;
    testAceMsgSetType*  sDlTestSetMsg;
    testAceMsgSetType*  sDlTestSetErr;
    testAceMsgInitType* sDlTestFini;
    acp_char_t      sDlError[TEST_MAXLENGTH];

    sDlName = TEST_MSG_DL;
    sRC = acpDlOpen(&sDl, acpStrGetBuffer(aDlPath), sDlName, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sDlTestFunc = (testAceMsgFuncType*)acpDlSym(&sDl, TEST_MSG_FUNC);
    ACT_CHECK(NULL != sDlTestFunc);
    sDlTestSetErr = (testAceMsgSetType*)acpDlSym(&sDl, TEST_ERR_SET);
    ACT_CHECK(NULL != sDlTestSetErr);
    sDlTestSetMsg = (testAceMsgSetType*)acpDlSym(&sDl, TEST_MSG_SET);
    ACT_CHECK(NULL != sDlTestSetMsg);

    if(NULL != sDlTestFunc)
    {
        (*sDlTestSetErr)(ACE_ERROR_CALLBACK_GLOBAL_AREA);
        (*sDlTestSetMsg)(ACE_MSG_GLOBAL_AREA);
        sRC = (*sDlTestFunc)(aContext, sDlError, TEST_MAXLENGTH);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC(
            acpStrCmpCString(aMsgShouldBe, sDlError, 0) == 0,
            ("error message should be \"%S\" but \"%s\"",
             aMsgShouldBe,
             sDlError));
    }
    else
    {
        /* Do nothing */
    }

    (void)acpDlClose(&sDl);

    sDlName = TEST_MSG_DLERR;
    sRC = acpDlOpen(&sDl, acpStrGetBuffer(aDlPath), sDlName, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sDlTestInit = (testAceMsgInitType*)acpDlSym(&sDl, TEST_MSG_INIT);
    ACT_CHECK(NULL != sDlTestInit);
    sDlTestFunc = (testAceMsgFuncType*)acpDlSym(&sDl, TEST_MSG_FUNC);
    ACT_CHECK(NULL != sDlTestFunc);
    sDlTestSetMsg = (testAceMsgSetType*)acpDlSym(&sDl, TEST_MSG_SET);
    ACT_CHECK(NULL != sDlTestSetMsg);
    sDlTestFini = (testAceMsgInitType*)acpDlSym(&sDl, TEST_MSG_FINI);
    ACT_CHECK(NULL != sDlTestFini);

    if(NULL != sDlTestFunc)
    {
        (*sDlTestInit)();
        (*sDlTestSetMsg)(ACE_MSG_GLOBAL_AREA);
        sRC = (*sDlTestFunc)(aContext, sDlError, TEST_MAXLENGTH);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC(
            acpStrCmpCString(aMsgShouldBe, sDlError, 0) == 0,
            ("error message should be \"%S\" but \"%s\"",
             aMsgShouldBe,
             sDlError));
        (*sDlTestFini)();
    }
    else
    {
        /* Do nothing */
    }

    (void)acpDlClose(&sDl);
#endif
}



acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_path_pool_t sPool;
    acp_rc_t        sRC;
    acp_str_t       sAltLang = ACP_STR_CONST("KO");

    ACL_CONTEXT_DECLARE(testContext, sContext);

    ACP_STR_DECLARE_DYNAMIC(sDir);


    ACP_STR_INIT_DYNAMIC(sDir, 0, 32);

    ACP_UNUSED(aArgc);

    ACE_MSG_SET_GLOBAL_AREA;
    ACE_ERROR_CALLBACK_SET_GLOBAL_AREA;

    ACT_TEST_BEGIN();

    ACL_CONTEXT_INIT();

    sContext.mCount = 0;
    acpPathPoolInit(&sPool);

    sRC = acpStrCpyFormat(&sDir, "%s/msg", acpPathDir(aArgv[0], &sPool));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    acpPathPoolFinal(&sPool);

    testFunc(&sContext);
    ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg1, ACE_GET_ERROR_MSG(), 0) == 0,
                   ("error message should be \"%S\" but \"%s\"",
                    &gTestErrMsg1,
                    ACE_GET_ERROR_MSG()));
    testDlAceMsg(&sContext, &sDir, &gTestErrMsg1);

    sRC = testMsgLoad(&sDir, NULL, NULL, NULL);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("testMsgLoad should return SUCCESS but %d", sRC));

    testFunc(&sContext);
    ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg2, ACE_GET_ERROR_MSG(), 0) == 0,
                   ("error message should be \"%S\" but \"%s\"",
                    &gTestErrMsg2,
                    ACE_GET_ERROR_MSG()));
    testDlAceMsg(&sContext, &sDir, &gTestErrMsg2);

    aceSetErrorCallback(ACE_PRODUCT_TST, gTestErrorCallback);

    gContextPtr = &sContext;

    testFunc(&sContext);
    ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg2, ACE_GET_ERROR_MSG(), 0) == 0,
                   ("error message should be \"%S\" but \"%s\"",
                    &gTestErrMsg2,
                    ACE_GET_ERROR_MSG()));
    testDlAceMsg(&sContext, &sDir, &gTestErrMsg2);


    testMsgUnload();
    sRC = testMsgLoad(&sDir, NULL, &sAltLang, NULL);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("testMsgLoad should return SUCCESS but %d", sRC));

    testFunc(&sContext);
    ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg3, ACE_GET_ERROR_MSG(), 0) == 0,
                   ("error message should be \"%S\" but \"%s\"",
                    &gTestErrMsg3,
                    ACE_GET_ERROR_MSG()));
    testDlAceMsg(&sContext, &sDir, &gTestErrMsg3);

    testMsgUnload();

    testFunc(&sContext);
    ACT_CHECK_DESC(acpStrCmpCString(&gTestErrMsg1, ACE_GET_ERROR_MSG(), 0) == 0,
                   ("error message should be \"%S\" but \"%s\"",
                    &gTestErrMsg1,
                    ACE_GET_ERROR_MSG()));
    testDlAceMsg(&sContext, &sDir, &gTestErrMsg1);

#if !defined(TEST_ACE_DL)
    ACT_CHECK_DESC((3 == gStage),
                   ("testCallback must be called 6 times but %d times called",
                    gStage));
#else
    ACT_CHECK_DESC((6 == gStage),
                   ("testCallback must be called 6 times but %d times called",
                    gStage));
    ACT_CHECK_DESC((10 == sContext.mCount),
                   ("External testCallback must be called 10 times "
                    "but %d times called",
                    sContext.mCount));
#endif

    ACL_CONTEXT_FINAL();

    ACP_STR_FINAL(sDir);

    ACT_TEST_END();

    return 0;
}
