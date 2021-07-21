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
 * $Id: testAcpPrintfPerf.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acpOpt.h>
#include <acpPrintf.h>
#include <acpSys.h>
#include <act.h>


#if defined(ALTI_CFG_OS_WINDOWS)
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif


#if 0
#define FORMAT_INT    "%#0512.10x %#-0512.10x"
#define FORMAT_FLOAT  "%#0512.256e %#-0512.256e"
#define FORMAT_STRING "%#0512.100s %#-0512.100s"
#else
#define FORMAT_INT    "%#512.10x %#-512.10x"
#define FORMAT_FLOAT  "%#0512.15e %#-512.15e"
#define FORMAT_STRING "%512.100s %-512.100s"
#endif

#define VALUE_INT     1234567890
#define VALUE_FLOAT   (2.0 / 3.0 * 1000000.0)
#define VALUE_STRING  "Altibase Core Porting Layer"


#define SURE(aStmt)                                             \
    do                                                          \
    {                                                                       \
        acp_char_t sCeMsg_MACRO_LOCAL_VAR[1024];                            \
        acp_rc_t   sCeRC_MACRO_LOCAL_VAR;                                   \
                                                                            \
        sCeRC_MACRO_LOCAL_VAR = aStmt;                                      \
                                                                            \
        if (ACP_RC_NOT_SUCCESS(sCeRC_MACRO_LOCAL_VAR))                      \
        {                                                                   \
            acpErrorString(sCeRC_MACRO_LOCAL_VAR,                           \
                           sCeMsg_MACRO_LOCAL_VAR,                          \
                           sizeof(sCeMsg_MACRO_LOCAL_VAR));                 \
            (void)acpPrintf("%s:%d %s\n[%d] %s\n",                          \
                            __FILE__,                                       \
                            __LINE__,                                       \
                            #aStmt,                                         \
                            sCeRC_MACRO_LOCAL_VAR,                          \
                            sCeMsg_MACRO_LOCAL_VAR);                        \
            acpProcExit(1);                                                 \
        }                                                                   \
        else                                                                \
        {                                                                   \
        }                                                                   \
    } while (0)


enum
{
    OPT_UNKNOWN = 0,
    OPT_FUNC,
    OPT_LOOP,
    OPT_TYPE,
    OPT_THREAD_COUNT,
    OPT_HELP
};

enum
{
    FUNC_ALL = 0,
    FUNC_ACP,
    FUNC_SYSTEM
};

enum
{
    TYPE_ALL = 0,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
};

typedef acp_sint32_t funcPrintf(acp_char_t *,
                                acp_size_t,
                                const acp_char_t *, ...);

struct acpTestThreadParam
{
    acp_sint32_t mType;
    acp_uint32_t mLoop;
    funcPrintf  *mFuncPtr;
};

static acp_opt_def_t gOptDef[] =
{
    {
        OPT_FUNC,
        ACP_OPT_ARG_REQUIRED,
        'f',
        "func",
        "0",
        "FUNCTION",
        "function to test (0=all,1=acp,2=system)"
    },
    {
        OPT_LOOP,
        ACP_OPT_ARG_REQUIRED,
        'l',
        "loop",
        "500000",
        "LOOP",
        "number of test loop"
    },
    {
        OPT_TYPE,
        ACP_OPT_ARG_REQUIRED,
        't',
        "type",
        "0",
        "TYPE",
        "data type to test (0=all,1=int,2=float,3=string)"
    },
    {
        OPT_THREAD_COUNT,
        ACP_OPT_ARG_REQUIRED,
        'c',
        "thread-count",
        "1",
        "THREAD-COUNT",
        "number of threads (for multi-threaded performance test), 0=use system core count."
    },
    {
        OPT_HELP,
        ACP_OPT_ARG_NOTEXIST,
        'h',
        "help",
        NULL,
        NULL,
        "display this help and exit"
    },
    {
        0,
        ACP_OPT_ARG_NOTEXIST,
        0,
        NULL,
        NULL,
        NULL,
        NULL
    }
};


static void checkResult(void)
{
#if !defined(ALINT)
/* ------------------------------------------------
 *  this function is for comparing performance of printf
 *  between native and acpPrintf*().
 *  so, we can skip ALINT checking here.
 * ----------------------------------------------*/
    
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4995)
#endif
    acp_char_t   sBuffer1[1024];
    acp_char_t   sBuffer2[1024];
    acp_sint32_t sMaxLen = sizeof(sBuffer1) - 1;
    acp_sint32_t sLen1;
    acp_sint32_t sLen2;

    (void)acpSnprintf(sBuffer1,
                      sizeof(sBuffer1),
                      FORMAT_INT"%n",
                      VALUE_INT,
                      VALUE_INT,
                      &sLen1);
    sLen2 = SNPRINTF(sBuffer2,
                     sizeof(sBuffer2),
                     FORMAT_INT,
                     VALUE_INT,
                     VALUE_INT);

    ACT_CHECK_DESC(memcmp(sBuffer1, sBuffer2, ACP_MIN(sMaxLen, sLen1)) == 0,
                   ("different result for int\n"
                    "=== acpSnprintf ===\n"
                    "%s\n-------------------\n"
                    "=== snprintf ===\n"
                    "%s\n-------------------",
                    sBuffer1,
                    sBuffer2));

#if defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == -1);
#elif defined(ALTI_CFG_OS_HPUX)
#  if ((ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR == 31))
    ACT_CHECK_DESC(sLen1 == sLen2,
                   ("different returned length for int\n"
                    "acpSnprintf -> %d\n"
                    "snprintf    -> %d",
                    sLen1,
                    sLen2));
#  else
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == -1);
#  endif
#elif defined(ALTI_CFG_OS_TRU64)
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == 1023);
#else
    ACT_CHECK_DESC(sLen1 == sLen2,
                   ("different returned length for int\n"
                    "acpSnprintf -> %d\n"
                    "snprintf    -> %d",
                    sLen1,
                    sLen2));
#endif

    (void)acpSnprintf(sBuffer1,
                      sizeof(sBuffer1),
                      FORMAT_FLOAT"%n",
                      VALUE_FLOAT,
                      VALUE_FLOAT,
                      &sLen1);
    sLen2 = SNPRINTF(sBuffer2,
                     sizeof(sBuffer2),
                     FORMAT_FLOAT,
                     VALUE_FLOAT,
                     VALUE_FLOAT);

#if !defined(ALTI_CFG_OS_AIX) &&                 \
    !defined(ALTI_CFG_OS_HPUX) &&                \
    !defined(ALTI_CFG_OS_TRU64) &&               \
    !defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK_DESC(memcmp(sBuffer1, sBuffer2, ACP_MIN(sMaxLen, sLen1)) == 0,
                   ("different result for float\n"
                    "=== acpSnprintf ===\n"
                    "%s\n-------------------\n"
                    "=== snprintf ===\n"
                    "%s\n-------------------",
                    sBuffer1,
                    sBuffer2));
#endif

#if defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == -1);
#elif defined(ALTI_CFG_OS_HPUX)
#  if ((ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR == 31))
    ACT_CHECK_DESC(sLen1 == sLen2,
                   ("different returned length for int\n"
                    "acpSnprintf -> %d\n"
                    "snprintf    -> %d",
                    sLen1,
                    sLen2));
#  else
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == -1);
#  endif
#elif defined(ALTI_CFG_OS_TRU64)
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == 1023);
#else
    ACT_CHECK_DESC(sLen1 == sLen2,
                   ("different returned length for float\n"
                    "acpSnprintf -> %d\n"
                    "snprintf    -> %d",
                    sLen1,
                    sLen2));
#endif

    (void)acpSnprintf(sBuffer1,
                      sizeof(sBuffer1),
                      FORMAT_STRING"%n",
                      VALUE_STRING,
                      VALUE_STRING,
                      &sLen1);
    sLen2 = SNPRINTF(sBuffer2,
                     sizeof(sBuffer2),
                     FORMAT_STRING,
                     VALUE_STRING,
                     VALUE_STRING);

    ACT_CHECK_DESC(memcmp(sBuffer1, sBuffer2, ACP_MIN(sMaxLen, sLen1)) == 0,
                   ("different result for string\n"
                    "=== acpSnprintf ===\n"
                    "%s\n-------------------\n"
                    "=== snprintf ===\n"
                    "%s\n-------------------",
                    sBuffer1,
                    sBuffer2));

#if defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == -1);
#elif defined(ALTI_CFG_OS_HPUX)
#  if ((ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR == 31))
    ACT_CHECK_DESC(sLen1 == sLen2,
                   ("different returned length for int\n"
                    "acpSnprintf -> %d\n"
                    "snprintf    -> %d",
                    sLen1,
                    sLen2));
#  else
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == -1);
#  endif
#elif defined(ALTI_CFG_OS_TRU64)
    ACT_CHECK(sLen1 == 1025);
    ACT_CHECK(sLen2 == 1023);
#else
    ACT_CHECK_DESC(sLen1 == sLen2,
                   ("different returned length for string\n"
                    "acpSnprintf -> %d\n"
                    "snprintf    -> %d",
                    sLen1,
                    sLen2));
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif /* ALINT */
}

static acp_sint32_t acpTestThrFunc( void *aParam )
{
    struct acpTestThreadParam *sParam = ( struct acpTestThreadParam * )aParam;

    acp_char_t    sBuffer[1024];
    acp_uint32_t  i;

    switch ( sParam->mType )
    {
        case TYPE_INT:
            (void)acpPrintf("(int):    ");
            for (i = 0; i < sParam->mLoop; i++)
            {
                (void)(*sParam->mFuncPtr)( sBuffer,
                                           sizeof(sBuffer),
                                           FORMAT_INT,
                                           VALUE_INT,
                                           VALUE_INT );
            }
            break;
    case TYPE_FLOAT:
        (void)acpPrintf("(float):  ");
        for (i = 0; i < sParam->mLoop; i++)
        {
            (void)(*sParam->mFuncPtr)( sBuffer,
                                       sizeof(sBuffer),
                                       FORMAT_FLOAT,
                                       VALUE_FLOAT,
                                       VALUE_FLOAT );
        }
        break;
    case TYPE_STRING:
        (void)acpPrintf("(string): ");
        for (i = 0; i < sParam->mLoop; i++)
        {
            (void)(*sParam->mFuncPtr)( sBuffer,
                                       sizeof(sBuffer),
                                       FORMAT_STRING,
                                       VALUE_STRING,
                                       VALUE_STRING );
            }
            break;
        default:
            (void)acpPrintf("invalid type\n");
            return 1;
    }

    return 0;
}

static void runTest( acp_sint32_t aFunc,
                     acp_sint32_t aLoop,
                     acp_sint32_t aType,
                     acp_uint32_t aThreadCount )
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4995)
#endif
    struct acpTestThreadParam sParam;

    acp_time_t    sTime;
    acp_sint32_t  sExitCode;
    acp_uint32_t  i;
    acp_thr_t    *sThreads = NULL;

    (void)acpPrintf("loop %d of ", aLoop);

    if ( aThreadCount > 1 )
    {
        SURE( acpMemCalloc( (void**) &sThreads,
                            aThreadCount,
                            sizeof( acp_thr_t ) ) );
    }
    else
    {
        /* do nothing */
    }

    switch (aFunc)
    {
        case FUNC_ACP:
            (void)acpPrintf("acpSnprintf");
            sParam.mFuncPtr = acpSnprintf;
            break;
        case FUNC_SYSTEM:
            (void)acpPrintf("   snprintf");
            sParam.mFuncPtr = SNPRINTF;
            break;
        default:
            (void)acpPrintf("invalid function\n");
            acpProcExit(1);
            break;
    }

    sParam.mLoop = aLoop;
    sParam.mType = aType;

    sTime = acpTimeNow();

    ACT_ASSERT( aThreadCount > 0 );
    if ( aThreadCount > 1 )
    {
        for ( i = 0 ; i < aThreadCount ; i++ )
        {
            SURE( acpThrCreate( &sThreads[ i ],
                                NULL,
                                acpTestThrFunc,
                                &sParam ) );
        }

        for ( i = 0 ; i < aThreadCount ; i++ )
        {
            SURE( acpThrJoin( &sThreads[ i ],
                              &sExitCode ) );
            ACT_CHECK( sExitCode == 0 );
        }
    }
    else
    {
        acpTestThrFunc( &sParam );
    }

    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("%5lld msec\n", acpTimeToMsec(sTime));

    if ( sThreads != NULL )
    {
        acpMemFree( sThreads );
    }
    else
    {
        /* do nothing */
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_opt_t    sOpt;
    acp_sint32_t sOptValue;
    acp_sint32_t sFunc = FUNC_ALL;
    acp_sint32_t sType = TYPE_ALL;
    acp_sint32_t sLoop = 500000;
    acp_sint32_t sFuncMin;
    acp_sint32_t sFuncMax;
    acp_sint32_t sTypeMin;
    acp_sint32_t sTypeMax;
    acp_sint32_t sIntSign;
    acp_uint32_t sThreadCount = 1;
    acp_uint64_t sIntValue;
    acp_rc_t     sRC;
    acp_char_t  *sArg;
    acp_char_t   sError[1024];
    acp_char_t   sHelp[1024];

    ACT_TEST_BEGIN();

    SURE(acpOptInit(&sOpt, aArgc, aArgv));

    while (1)
    {
        sRC = acpOptGet(&sOpt, gOptDef, NULL, &sOptValue,
                        &sArg, sError, sizeof(sError));

        if (ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else if (ACP_RC_IS_SUCCESS(sRC))
        {
            /* continue */
        }
        else
        {
            (void)acpPrintf("%s\n", sError);
            acpProcExit(1);
            break;
        }

        switch (sOptValue)
        {
            case OPT_FUNC:
                SURE(acpCStrToInt64(sArg, 128, &sIntSign,
                                    &sIntValue, 10, NULL));
                sFunc = (acp_sint32_t)sIntValue * sIntSign;
                break;
            case OPT_LOOP:
                SURE(acpCStrToInt64(sArg, 128, &sIntSign,
                                    &sIntValue, 10, NULL));
                sLoop = (acp_sint32_t)sIntValue * sIntSign;
                break;
            case OPT_TYPE:
                SURE(acpCStrToInt64(sArg, 128, &sIntSign,
                                    &sIntValue, 10, NULL));
                sType = (acp_sint32_t)sIntValue * sIntSign;
                break;
            case OPT_THREAD_COUNT:
                SURE(acpCStrToInt32(sArg, 128, &sIntSign,
                                    &sThreadCount, 10, NULL));
                if ( sThreadCount == 0 )
                {
                    SURE( acpSysGetCPUCount( &sThreadCount ) );
                }
                else
                {
                    /* do nothing */
                }
                break;
            case OPT_HELP:
                (void)acpOptHelp(gOptDef, NULL, sHelp, sizeof(sHelp));
                (void)acpPrintf("%s", sHelp);
                acpProcExit(0);
                break;
            default:
                (void)acpPrintf("unknown argument: %s\n", sArg);
                acpProcExit(1);
                break;
        }
    }

    checkResult();

    if (sFunc == FUNC_ALL)
    {
        sFuncMin = FUNC_ACP;
        sFuncMax = FUNC_SYSTEM;
    }
    else
    {
        sFuncMin = sFunc;
        sFuncMax = sFunc;
    }

    if (sType == TYPE_ALL)
    {
        sTypeMin = TYPE_INT;
        sTypeMax = TYPE_STRING;
    }
    else
    {
        sTypeMin = sType;
        sTypeMax = sType;
    }

    for (sType = sTypeMin; sType <= sTypeMax; sType++)
    {
        for (sFunc = sFuncMin; sFunc <= sFuncMax; sFunc++)
        {
            runTest( sFunc, sLoop, sType, sThreadCount );
        }
    }

    ACT_TEST_END();

    return 0;
}
