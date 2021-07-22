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
 * $Id: testAclConf.c 4823 2009-03-12 03:09:01Z sjkim $
 ******************************************************************************/

#include <act.h>
#include <acpFile.h>
#include <acpStr.h>
#include <aclConf.h>


/* #define TEST_LOG_ALL */

#define TEST_ACL_CONF_ID_IP    10
#define TEST_ACL_CONF_ID_PORT  11
#define TEST_ACL_CONF_ID_PORT2 12

static acp_sint32_t confCallbackBoolean(acp_sint32_t    aDepth,
                                        acl_conf_key_t *aKey,
                                        acp_sint32_t    aLineNumber,
                                        void           *aValue,
                                        void           *aContext);
static acp_sint32_t confCallbackSInt32(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext);
static acp_sint32_t confCallbackUInt32(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext);
static acp_sint32_t confCallbackSInt64(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext);
static acp_sint32_t confCallbackUInt64(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext);
static acp_sint32_t confCallbackString(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext);


static acl_conf_def_t gTestConfDef11[] =
{
    ACL_CONF_DEF_STRING("NAME", 0, "", 1, NULL),
    ACL_CONF_DEF_STRING("IP", 0, "", 1, NULL),
    ACL_CONF_DEF_SINT32("PORT", 0, 1024, 1025, 65535, 1, confCallbackSInt32),
    ACL_CONF_DEF_STRING("OBJECTS", 0, "", 2, NULL),
    ACL_CONF_DEF_END()
};

static acl_conf_def_t gTestConfDef1[] =
{
    ACL_CONF_DEF_BOOLEAN("MULTIPLEXING", 0, ACP_TRUE, confCallbackBoolean),
    ACL_CONF_DEF_SINT32("THREAD_COUNT", 0, 1, 1, 64, 1, confCallbackSInt32),
    ACL_CONF_DEF_UINT32("CACHE_SIZE", 0, 65536, 0, 0, 1, confCallbackUInt32),
    ACL_CONF_DEF_SINT64("KEY_TIME", 0, 0, 0, 0, 0, confCallbackSInt64),
    ACL_CONF_DEF_UINT64("MAX_SIZE", 0, 0, 0, 0, 1, confCallbackUInt64),
    ACL_CONF_DEF_CONTAINER("SERVERS", 0, gTestConfDef11, 0),
    ACL_CONF_DEF_END()
};

static acl_conf_def_t gTestConfDef21[] =
{
    ACL_CONF_DEF_STRING("IP", 0, "", 1, NULL),
    ACL_CONF_DEF_SINT32("PORT", 0, 1024, 1025, 65535, 1, confCallbackSInt32),
    ACL_CONF_DEF_STRING("OBJECTS", 0, "", 2, NULL),
    ACL_CONF_DEF_END()
};

static acl_conf_def_t gTestConfDef2[] =
{
    ACL_CONF_DEF_BOOLEAN("MULTIPLEXING", 0, ACP_TRUE, confCallbackBoolean),
    ACL_CONF_DEF_SINT32("THREAD_COUNT", 0, 1, 1, 64, 1, confCallbackSInt32),
    ACL_CONF_DEF_UINT32("CACHE_SIZE", 0, 65536, 0, 0, 1, confCallbackUInt32),
    ACL_CONF_DEF_SINT64("KEY_TIME", 0, 0, 0, 0, 0, confCallbackSInt64),
    ACL_CONF_DEF_UINT64("MAX_SIZE", 0, 0, 0, 0, 1, confCallbackUInt64),
    ACL_CONF_DEF_CONTAINER(NULL, 0, gTestConfDef21, 0),
    ACL_CONF_DEF_END()
};

static acl_conf_def_t gTestConfDef3[] =
{
    ACL_CONF_DEF_STRING("IP", TEST_ACL_CONF_ID_IP, "127.0.0.1", 1, NULL),
    ACL_CONF_DEF_UINT32("PORT", TEST_ACL_CONF_ID_PORT, 9876,
                        1024, 65535, 1, confCallbackUInt32),
    ACL_CONF_DEF_UINT32("PORT2", TEST_ACL_CONF_ID_PORT2, 5432,
                        1024, 65535, 1, confCallbackUInt32),
    ACL_CONF_DEF_END()
};


typedef struct aclConfResult
{
    const acp_char_t  *mError;
    acp_sint32_t       mDepth;
    acp_sint32_t       mID;
    acl_conf_key_t     mKey[10];
    acl_conf_defval_t  mValue;
} aclConfResult;

typedef struct aclConfTest
{
    acl_conf_def_t *mTestDef;
    aclConfResult  *mTestResult;
} aclConfTest;


static aclConfResult gTestConfResult0[] =
{
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("MULTIPLEXING"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_BOOLEAN(ACP_TRUE)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("THREAD_COUNT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(1)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("CACHE_SIZE"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT32(65536)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("KEY_TIME"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT64(0)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("MAX_SIZE"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT64(0)
    },
    {
        NULL,
        0,
        0,
        {
            {
                ACP_STR_CONST(""),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(0)
    }
};

static aclConfResult gTestConfResult1[] =
{
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("THREAD_COUNT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(4)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("CACHE_SIZE"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT32(10485760)
    },
    {
        "testAclConf.1.conf:10: SERVER1 unknown",
        1,
        0,
        {
            {
                ACP_STR_CONST(""),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(0)
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                0
            },
            {
                ACP_STR_CONST("NAME"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("server1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                0
            },
            {
                ACP_STR_CONST("IP"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("192.168.1.1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                0
            },
            {
                ACP_STR_CONST("PORT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(1234)
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                0
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER1_OBJ1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                0
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                1
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER1_OBJ2")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                1
            },
            {
                ACP_STR_CONST("NAME"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("server2")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                1
            },
            {
                ACP_STR_CONST("IP"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("192.168.1.2")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                1
            },
            {
                ACP_STR_CONST("PORT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(2345)
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                1
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER2_OBJ1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                2
            },
            {
                ACP_STR_CONST("NAME"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("server3")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                2
            },
            {
                ACP_STR_CONST("IP"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("192.168.1.3")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                2
            },
            {
                ACP_STR_CONST("PORT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(3456)
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                2
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER3_OBJ1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                2
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                1
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER3_OBJ2")
    },
    {
        "testAclConf.1.conf:44: SERVERS.OBJECTS too many value",
        1,
        0,
        {
            {
                ACP_STR_CONST(""),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(0)
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                3
            },
            {
                ACP_STR_CONST("NAME"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("server4")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                3
            },
            {
                ACP_STR_CONST("IP"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("192.168.1.4")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                3
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER4_OBJ1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                3
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                1
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER4_OBJ2")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVERS"),
                0,
                3
            },
            {
                ACP_STR_CONST("PORT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(1024)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("MULTIPLEXING"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_BOOLEAN(ACP_TRUE)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("KEY_TIME"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT64(0)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("MAX_SIZE"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT64(0)
    },
    {
        NULL,
        0,
        0,
        {
            {
                ACP_STR_CONST(""),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(0)
    }
};

static aclConfResult gTestConfResult2[] =
{
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("MULTIPLEXING"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_BOOLEAN(ACP_FALSE)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("MAX_SIZE"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT64(ACP_UINT64_LITERAL(132070244352))
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVER1"),
                0,
                0
            },
            {
                ACP_STR_CONST("IP"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("192.168.1.1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVER1"),
                0,
                0
            },
            {
                ACP_STR_CONST("PORT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(1234)
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVER1"),
                0,
                0
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER1_OBJ1")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVER1"),
                0,
                0
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                1
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER1_OBJ2")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVER2"),
                0,
                0
            },
            {
                ACP_STR_CONST("IP"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("192.168.1.2")
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVER2"),
                0,
                0
            },
            {
                ACP_STR_CONST("PORT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(2345)
    },
    {
        NULL,
        2,
        0,
        {
            {
                ACP_STR_CONST("SERVER2"),
                0,
                0
            },
            {
                ACP_STR_CONST("OBJECTS"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("SERVER2_OBJ1")
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("THREAD_COUNT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(1)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("CACHE_SIZE"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT32(65536)
    },
    {
        NULL,
        1,
        0,
        {
            {
                ACP_STR_CONST("KEY_TIME"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT64(0)
    },
    {
        NULL,
        0,
        0,
        {
            {
                ACP_STR_CONST(""),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(0)
    }
};

static aclConfResult gTestConfResult3[] =
{
    {
        NULL,
        1,
        TEST_ACL_CONF_ID_IP,
        {
            {
                ACP_STR_CONST("IP"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_STRING("192.168.1.1")
    },
    {
        NULL,
        1,
        TEST_ACL_CONF_ID_PORT,
        {
            {
                ACP_STR_CONST("PORT"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT32(9876)
    },
    {
        NULL,
        1,
        TEST_ACL_CONF_ID_PORT2,
        {
            {
                ACP_STR_CONST("PORT2"),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_UINT32(5432)
    },
    {
        NULL,
        0,
        0,
        {
            {
                ACP_STR_CONST(""),
                0,
                0
            }
        },
        ACL_CONF_DEFVAL_SINT32(0)
    }
};

static aclConfTest gTestConf[] =
{
    {
        gTestConfDef1,
        gTestConfResult0
    },
    {
        gTestConfDef1,
        gTestConfResult1
    },
    {
        gTestConfDef2,
        gTestConfResult2
    },
    {
        gTestConfDef3,
        gTestConfResult3
    },
    {
        NULL,
        NULL
    }
};

static acp_sint32_t gTestIndex;



static void generateKeyString(acp_sint32_t    aDepth,
                              acl_conf_key_t *aKey,
                              acp_str_t      *aStr)
{
    acp_sint32_t i;

    for (i = 0; i < aDepth; i++)
    {
        (void)acpStrCatFormat(aStr,
                              "%s%S[%d]",
                              ((i == 0) ? "" : "."),
                              &aKey[i].mKey,
                              aKey[i].mIndex);
    }
}

static acp_sint32_t confCallbackBoolean(acp_sint32_t    aDepth,
                                        acl_conf_key_t *aKey,
                                        acp_sint32_t    aLineNumber,
                                        void           *aValue,
                                        void           *aContext)
{
    aclConfResult *sTestResult = aContext;

    ACP_STR_DECLARE_STATIC(sKey1, 128);
    ACP_STR_DECLARE_STATIC(sKey2, 128);

    ACP_STR_INIT_STATIC(sKey1);
    ACP_STR_INIT_STATIC(sKey2);

    generateKeyString(aDepth, aKey, &sKey1);

#if defined(TEST_LOG_ALL)
    (void)acpPrintf("[%2d] %S <- %d\n",
                    aLineNumber,
                    &sKey1,
                    *(acp_bool_t *)aValue);
#else
    ACP_UNUSED(aLineNumber);
#endif

    if (sTestResult[gTestIndex].mDepth == 0)
    {
        ACT_ERROR(("no more result specified, but %S <- %d",
                   &sKey1,
                   *(acp_bool_t *)aValue));
        return 0;
    }
    else
    {
        /* do nothing */
    }

    if (sTestResult[gTestIndex].mDepth == aDepth)
    {
        generateKeyString(aDepth, sTestResult[gTestIndex].mKey, &sKey2);

        ACT_CHECK_DESC(acpStrEqual(&sKey1, &sKey2, 0) == ACP_TRUE,
                       ("key should be %S but %S", &sKey2, &sKey1));

        ACT_CHECK_DESC(sTestResult[gTestIndex].mValue.mBoolean ==
                       *(acp_bool_t *)aValue,
                       ("value should be %d but %d for %S",
                        sTestResult[gTestIndex].mValue.mBoolean,
                        *(acp_bool_t *)aValue,
                        &sKey2));
    }
    else
    {
        ACT_ERROR(("depth should be %d but %d",
                   sTestResult[gTestIndex].mDepth,
                   aDepth));
    }

    gTestIndex++;

    return 0;
}

static acp_sint32_t confCallbackSInt32(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext)
{
    aclConfResult *sTestResult = aContext;

    ACP_STR_DECLARE_STATIC(sKey1, 128);
    ACP_STR_DECLARE_STATIC(sKey2, 128);

    ACP_STR_INIT_STATIC(sKey1);
    ACP_STR_INIT_STATIC(sKey2);

    generateKeyString(aDepth, aKey, &sKey1);

#if defined(TEST_LOG_ALL)
    (void)acpPrintf("[%2d] %S <- %d\n",
                    aLineNumber,
                    &sKey1,
                    *(acp_sint32_t *)aValue);
#else
    ACP_UNUSED(aLineNumber);
#endif

    if (sTestResult[gTestIndex].mDepth == 0)
    {
        ACT_ERROR(("no more result specified, but %S <- %d",
                   &sKey1,
                   *(acp_sint32_t *)aValue));
        return 0;
    }
    else
    {
        /* do nothing */
    }

    if (sTestResult[gTestIndex].mDepth == aDepth)
    {
        generateKeyString(aDepth, sTestResult[gTestIndex].mKey, &sKey2);

        ACT_CHECK_DESC(acpStrEqual(&sKey1, &sKey2, 0) == ACP_TRUE,
                       ("key should be %S but %S", &sKey2, &sKey1));

        ACT_CHECK_DESC(sTestResult[gTestIndex].mValue.mSInt32 ==
                       *(acp_sint32_t *)aValue,
                       ("value should be %d but %d for %S",
                        sTestResult[gTestIndex].mValue.mSInt32,
                        *(acp_sint32_t *)aValue,
                        &sKey2));
    }
    else
    {
        ACT_ERROR(("depth should be %d but %d",
                   sTestResult[gTestIndex].mDepth,
                   aDepth));
    }

    gTestIndex++;

    return 0;
}

static acp_sint32_t confCallbackUInt32(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext)
{
    aclConfResult *sTestResult = aContext;

    ACP_STR_DECLARE_STATIC(sKey1, 128);
    ACP_STR_DECLARE_STATIC(sKey2, 128);

    ACP_STR_INIT_STATIC(sKey1);
    ACP_STR_INIT_STATIC(sKey2);

    generateKeyString(aDepth, aKey, &sKey1);

#if defined(TEST_LOG_ALL)
    (void)acpPrintf("[%2d] %S <- %u\n",
                    aLineNumber,
                    &sKey1,
                    *(acp_uint32_t *)aValue);
#else
    ACP_UNUSED(aLineNumber);
#endif

    if (sTestResult[gTestIndex].mDepth == 0)
    {
        ACT_ERROR(("no more result specified, but %S <- %u",
                   &sKey1,
                   *(acp_uint32_t *)aValue));
        return 0;
    }
    else
    {
        /* do nothing */
    }

    if (sTestResult[gTestIndex].mDepth == aDepth)
    {
        generateKeyString(aDepth, sTestResult[gTestIndex].mKey, &sKey2);

        ACT_CHECK_DESC(acpStrEqual(&sKey1, &sKey2, 0) == ACP_TRUE,
                       ("key should be %S but %S", &sKey2, &sKey1));

        ACT_CHECK_DESC(sTestResult[gTestIndex].mValue.mUInt32 ==
                       *(acp_uint32_t *)aValue,
                       ("value should be %u but %u for %S",
                        sTestResult[gTestIndex].mValue.mUInt32,
                        *(acp_uint32_t *)aValue,
                        &sKey2));
    }
    else
    {
        ACT_ERROR(("depth should be %d but %d",
                   sTestResult[gTestIndex].mDepth,
                   aDepth));
    }

    /* bug-21758 */
    switch (aKey[0].mID)
    {
        case TEST_ACL_CONF_ID_PORT:
            ACT_CHECK_DESC(sTestResult[gTestIndex].mValue.mUInt32 ==
                            *(acp_uint32_t *)aValue,
                           ("value should be %u but %u for %S",
                            TEST_ACL_CONF_ID_PORT,
                            *(acp_uint32_t *)aValue,
                            &sKey2));
            break;
        case TEST_ACL_CONF_ID_PORT2:
            ACT_CHECK_DESC(sTestResult[gTestIndex].mValue.mUInt32 ==
                            *(acp_uint32_t *)aValue,
                           ("value should be %u but %u for %S",
                            TEST_ACL_CONF_ID_PORT2,
                            *(acp_uint32_t *)aValue,
                            &sKey2));
            break;
        default:
            break;
    }

    gTestIndex++;

    return 0;
}

static acp_sint32_t confCallbackSInt64(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext)
{
    aclConfResult *sTestResult = aContext;

    ACP_STR_DECLARE_STATIC(sKey1, 128);
    ACP_STR_DECLARE_STATIC(sKey2, 128);

    ACP_STR_INIT_STATIC(sKey1);
    ACP_STR_INIT_STATIC(sKey2);

    generateKeyString(aDepth, aKey, &sKey1);

#if defined(TEST_LOG_ALL)
    (void)acpPrintf("[%2d] %S <- %lld\n",
                    aLineNumber,
                    &sKey1,
                    *(acp_sint64_t *)aValue);
#else
    ACP_UNUSED(aLineNumber);
#endif

    if (sTestResult[gTestIndex].mDepth == 0)
    {
        ACT_ERROR(("no more result specified, but %S <- %lld",
                   &sKey1,
                   *(acp_sint64_t *)aValue));
        return 0;
    }
    else
    {
        /* do nothing */
    }

    if (sTestResult[gTestIndex].mDepth == aDepth)
    {
        generateKeyString(aDepth, sTestResult[gTestIndex].mKey, &sKey2);

        ACT_CHECK_DESC(acpStrEqual(&sKey1, &sKey2, 0) == ACP_TRUE,
                       ("key should be %S but %S", &sKey2, &sKey1));

        ACT_CHECK_DESC(sTestResult[gTestIndex].mValue.mSInt64 ==
                       *(acp_sint64_t *)aValue,
                       ("value should be %lld but %lld for %S",
                        sTestResult[gTestIndex].mValue.mSInt64,
                        *(acp_sint64_t *)aValue,
                        &sKey2));
    }
    else
    {
        ACT_ERROR(("depth should be %d but %d",
                   sTestResult[gTestIndex].mDepth,
                   aDepth));
    }

    gTestIndex++;

    return 0;
}

static acp_sint32_t confCallbackUInt64(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext)
{
    aclConfResult *sTestResult = aContext;

    ACP_STR_DECLARE_STATIC(sKey1, 128);
    ACP_STR_DECLARE_STATIC(sKey2, 128);

    ACP_STR_INIT_STATIC(sKey1);
    ACP_STR_INIT_STATIC(sKey2);

    generateKeyString(aDepth, aKey, &sKey1);

#if defined(TEST_LOG_ALL)
    (void)acpPrintf("[%2d] %S <- %llu\n",
                    aLineNumber,
                    &sKey1,
                    *(acp_uint64_t *)aValue);
#else
    ACP_UNUSED(aLineNumber);
#endif

    if (sTestResult[gTestIndex].mDepth == 0)
    {
        ACT_ERROR(("no more result specified, but %S <- %llu",
                   &sKey1,
                   *(acp_uint64_t *)aValue));
        return 0;
    }
    else
    {
        /* do nothing */
    }

    if (sTestResult[gTestIndex].mDepth == aDepth)
    {
        generateKeyString(aDepth, sTestResult[gTestIndex].mKey, &sKey2);

        ACT_CHECK_DESC(acpStrEqual(&sKey1, &sKey2, 0) == ACP_TRUE,
                       ("key should be %S but %S", &sKey2, &sKey1));

        ACT_CHECK_DESC(sTestResult[gTestIndex].mValue.mUInt64 ==
                       *(acp_uint64_t *)aValue,
                       ("value should be %llu but %llu for %S",
                        sTestResult[gTestIndex].mValue.mUInt64,
                        *(acp_uint64_t *)aValue,
                        &sKey2));
    }
    else
    {
        ACT_ERROR(("depth should be %d but %d",
                   sTestResult[gTestIndex].mDepth,
                   aDepth));
    }

    gTestIndex++;

    return 0;
}

static acp_sint32_t confCallbackString(acp_sint32_t    aDepth,
                                       acl_conf_key_t *aKey,
                                       acp_sint32_t    aLineNumber,
                                       void           *aValue,
                                       void           *aContext)
{
    aclConfResult *sTestResult = aContext;

    ACP_STR_DECLARE_STATIC(sKey1, 128);
    ACP_STR_DECLARE_STATIC(sKey2, 128);

    ACP_STR_INIT_STATIC(sKey1);
    ACP_STR_INIT_STATIC(sKey2);

    generateKeyString(aDepth, aKey, &sKey1);

#if defined(TEST_LOG_ALL)
    (void)acpPrintf("[%2d] %S <- %S\n",
                    aLineNumber,
                    &sKey1,
                    (acp_str_t *)aValue);
#else
    ACP_UNUSED(aLineNumber);
#endif

    if (sTestResult[gTestIndex].mDepth == 0)
    {
        ACT_ERROR(("no more result specified, but %S <- %S",
                   &sKey1,
                   (acp_str_t *)aValue));
        return 0;
    }
    else
    {
        /* do nothing */
    }

    if (sTestResult[gTestIndex].mDepth == aDepth)
    {
        generateKeyString(aDepth, sTestResult[gTestIndex].mKey, &sKey2);

        ACT_CHECK_DESC(acpStrEqual(&sKey1, &sKey2, 0) == ACP_TRUE,
                       ("key should be %S but %S", &sKey2, &sKey1));

        ACT_CHECK_DESC(acpStrCmpCString((acp_str_t *)aValue,
                                        sTestResult[gTestIndex].mValue.mString,
                                        0) == 0,
                       ("value should be \"%s\" but \"%S\" for %S",
                        sTestResult[gTestIndex].mValue.mString,
                        (acp_str_t *)aValue,
                        &sKey2));
    }
    else
    {
        ACT_ERROR(("depth should be %d but %d",
                   sTestResult[gTestIndex].mDepth,
                   aDepth));
    }

    gTestIndex++;

    return 0;
}

static acp_sint32_t confErrorCallback(acp_str_t    *aFilePath,
                                      acp_sint32_t  aLine,
                                      acp_str_t    *aError,
                                      void         *aContext)
{
    aclConfResult *sTestResult = aContext;

    ACP_UNUSED(aFilePath);
    ACP_UNUSED(aLine);

    if (sTestResult[gTestIndex].mDepth == 0)
    {
        ACT_ERROR(("no more result specified, but error reported:"));
        (void)acpPrintf("%S\n", aError);
        return 0;
    }
    else
    {
        /* do nothing */
    }

    ACT_CHECK_DESC(acpStrCmpCString(aError,
                                    sTestResult[gTestIndex].mError,
                                    0) == 0,
                   ("error should be \"%s\" but \"%S\"",
                    sTestResult[gTestIndex].mError,
                    aError));

    gTestIndex++;

    return 0;
}


acp_sint32_t main(void)
{
    acp_rc_t     sRC;
    acp_sint32_t i;

    ACP_STR_DECLARE_STATIC(sConfPath, 128);
    ACP_STR_INIT_STATIC(sConfPath);

    ACT_TEST_BEGIN();

    for (i = 0; gTestConf[i].mTestDef != NULL; i++)
    {
        sRC = acpStrCpyFormat(&sConfPath, "testAclConf.%d.conf", i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

#if defined(TEST_LOG_ALL)
        ACT_ERROR(("loading %S", &sConfPath));
#endif

        gTestIndex = 0;

        sRC = aclConfLoad(&sConfPath,
                          gTestConf[i].mTestDef,
                          confCallbackString,
                          confErrorCallback,
                          NULL,
                          gTestConf[i].mTestResult);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        ACT_CHECK_DESC(gTestConf[i].mTestResult[gTestIndex].mDepth == 0,
                       ("some callback missing in test %d", i));
    }

    ACT_TEST_END();

    return 0;
}
