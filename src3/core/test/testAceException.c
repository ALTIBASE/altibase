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
 * $Id: testAceException.c 5910 2009-06-08 08:20:20Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <aclContext.h>

typedef struct testContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;
} testContext;


static acp_sint32_t testFunc(testContext *aContext, acp_sint32_t aValue)
{
    ACE_CLEAR();

    ACE_TEST_RAISE(aValue == 1, LABEL_EXCEPTION1);
    ACE_TEST_RAISE(aValue == 2, LABEL_EXCEPTION2);
    ACE_TEST_RAISE(aValue == 3, LABEL_EXCEPTION3);
    ACE_TEST_RAISE(aValue == 4, LABEL_EXCEPTION4);
    ACE_TEST_RAISE(aValue == 5, LABEL_EXCEPTION5);
    ACE_TEST_RAISE(aValue == 6, LABEL_EXCEPTION6);
    ACE_TEST_RAISE(aValue == 7, LABEL_EXCEPTION7);
    ACE_TEST_RAISE(aValue == 8, LABEL_EXCEPTION8);
    ACE_TEST(aValue == 9);

    return 0;

    ACE_EXCEPTION(LABEL_EXCEPTION1)
    {
        ACE_SET(aValue = 1);
#if !defined(ALINT)
        ACE_PUSH();
#endif
        ACE_PUSH();
        (void)testFunc(aContext, 9);
        ACE_POP(0);
#if !defined(ALINT)
        ACE_POP(1);
#endif
        return aValue;
    }
    ACE_EXCEPTION(LABEL_EXCEPTION2)
    {
        ACE_SET(return 2);
    }
    ACE_EXCEPTION(LABEL_EXCEPTION3)
    {
        ACE_SET(return 3);
    }
    ACE_EXCEPTION(LABEL_EXCEPTION4)
    {
        ACE_SET(return 4);
    }
    ACE_EXCEPTION(LABEL_EXCEPTION5)
    {
        ACE_SET(return 5);
    }
    ACE_EXCEPTION(LABEL_EXCEPTION6)
    {
        ACE_SET(return 6);
    }
    ACE_EXCEPTION(LABEL_EXCEPTION7)
    {
        ACE_SET(return 7);
    }
    ACE_EXCEPTION(LABEL_EXCEPTION8)
    {
        ACE_SET(return 8);
    }
    ACE_EXCEPTION_END;

    ACE_SET(aValue = 9);

    return 9;
}


#if defined(ACP_CFG_DEBUG)

static const acp_char_t *gTestExpr[][3] =
{
    {
        NULL
    },
    {
        "testAceException.c:24: ACE_TEST_RAISE(aValue == 1, LABEL_EXCEPTION1)",
        "testAceException.c:38: ACE_SET(aValue = 1)",
        NULL
    },
    {
        "testAceException.c:25: ACE_TEST_RAISE(aValue == 2, LABEL_EXCEPTION2)",
        "testAceException.c:52: ACE_SET(return 2)",
        NULL
    },
    {
        "testAceException.c:26: ACE_TEST_RAISE(aValue == 3, LABEL_EXCEPTION3)",
        "testAceException.c:56: ACE_SET(return 3)",
        NULL
    },
    {
        "testAceException.c:27: ACE_TEST_RAISE(aValue == 4, LABEL_EXCEPTION4)",
        "testAceException.c:60: ACE_SET(return 4)",
        NULL
    },
    {
        "testAceException.c:28: ACE_TEST_RAISE(aValue == 5, LABEL_EXCEPTION5)",
        "testAceException.c:64: ACE_SET(return 5)",
        NULL
    },
    {
        "testAceException.c:29: ACE_TEST_RAISE(aValue == 6, LABEL_EXCEPTION6)",
        "testAceException.c:68: ACE_SET(return 6)",
        NULL
    },
    {
        "testAceException.c:30: ACE_TEST_RAISE(aValue == 7, LABEL_EXCEPTION7)",
        "testAceException.c:72: ACE_SET(return 7)",
        NULL
    },
    {
        "testAceException.c:31: ACE_TEST_RAISE(aValue == 8, LABEL_EXCEPTION8)",
        "testAceException.c:76: ACE_SET(return 8)",
        NULL
    },
    {
        "testAceException.c:32: ACE_TEST(aValue == 9)",
        "testAceException.c:80: ACE_SET(aValue = 9)",
        NULL
    },
    {
        NULL
    }
};

#else

static const acp_char_t *gTestExpr[] =
{
    NULL,
    "testAceException.c:38",
    "testAceException.c:52",
    "testAceException.c:56",
    "testAceException.c:60",
    "testAceException.c:64",
    "testAceException.c:68",
    "testAceException.c:72",
    "testAceException.c:76",
    "testAceException.c:80",
    NULL
};

#endif


static void testException(testContext *aContext)
{
    ace_exception_t *sException;
    acp_sint32_t     sRet;
    acp_sint32_t     i;
#if defined(ACP_CFG_DEBUG)
    acp_sint32_t     j;
#endif

    sException = aContext->mException;

    for (i = 0; i < 10; i++)
    {
        sRet = testFunc(aContext, i);
        ACT_CHECK_DESC(sRet == i,
                       ("testFunc should return %d but %d", i, sRet));

#if defined(ACP_CFG_DEBUG)
        for (j = 0; gTestExpr[i][j] != NULL; j++)
        {
            ACT_CHECK_DESC(acpCStrCmp(sException->mExpr[j],
                                       gTestExpr[i][j],
                                       sizeof(sException->mExpr[0]) ) == 0,
                           ("exception expression should be \"%s\" but \"%s\""
                            "at case %d",
                            gTestExpr[i][j],
                            sException->mExpr[j],
                            i));
        }
        ACT_CHECK_DESC(j == sException->mExprCount,
                       ("number of exception expression should be %d but %d "
                        "at case %d",
                        j,
                        sException->mExprCount,
                        i));
#else
        if (gTestExpr[i] != NULL)
        {
            ACT_CHECK_DESC(acpCStrCmp(sException->mExpr,
                                       gTestExpr[i],
                                       sizeof(sException->mExpr) ) == 0,
                           ("exception expression should be \"%s\" but \"%s\""
                            "at case %d",
                            gTestExpr[i],
                            sException->mExpr,
                            i));
        }
        else
        {
            /* do nothing */
        }
#endif
    }
}


acp_sint32_t main(void)
{
    ACL_CONTEXT_DECLARE(testContext, sContext);

    ACT_TEST_BEGIN();

    ACL_CONTEXT_INIT();

    testException(&sContext);

    ACL_CONTEXT_FINAL();

    ACT_TEST_END();

    return 0;
}
