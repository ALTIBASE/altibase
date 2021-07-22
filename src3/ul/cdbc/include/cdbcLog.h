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

#ifndef CDBC_LOG_H
#define CDBC_LOG_H 1

ACP_EXTERN_C_BEGIN



#define CDBCLOG_FILE stdout

#ifdef USE_CDBCLOG
    #define CDBCLOG_ENABLED() \
        ACP_RC_IS_SUCCESS(acpEnvGet("CDBCLOG_ENABLE", &gLogOption))

    #define CDBCLOG_PRINT_INDENT() do\
    {\
        ACE_DASSERT(gLogDeps >= 0);\
        if (CDBCLOG_ENABLED())\
        {\
            fprintf(CDBCLOG_FILE, "%.*s", gLogDeps * 3, gLogDepsStr);\
            fflush(CDBCLOG_FILE);\
        }\
    } while (ACP_FALSE)

    #define CDBCLOG_PRINT_CORE(line, args) do\
    {\
        if (CDBCLOG_ENABLED())\
        {\
            CDBCLOG_PRINT_INDENT();\
            if ((line) > 0)\
            {\
                fprintf(CDBCLOG_FILE, "line %d : ", line);\
            }\
            fprintf args ;\
            fprintf(CDBCLOG_FILE, "\n");\
            fflush(CDBCLOG_FILE);\
        }\
    } while (ACP_FALSE)

    #define CDBCLOG_PRINT(aStr)                      CDBCLOG_PRINT_CORE(__LINE__, (CDBCLOG_FILE, aStr))
    #define CDBCLOG_PRINTF_ARG1(aForm, aArg1)        CDBCLOG_PRINT_CORE(__LINE__, (CDBCLOG_FILE, aForm, aArg1))
    #define CDBCLOG_PRINTF_ARG2(aForm, aArg1, aArg2) CDBCLOG_PRINT_CORE(__LINE__, (CDBCLOG_FILE, aForm, aArg1, aArg2))
    #define CDBCLOG_PRINT_VAL(aForm, aVal)           CDBCLOG_PRINT_CORE(__LINE__, (CDBCLOG_FILE, # aVal " = " aForm, aVal))

    #define CDBCLOG_IN_CORE(line, aFuncName) do\
    {\
        if (CDBCLOG_ENABLED() && (gLogDeps == 0))\
        {\
            fprintf(CDBCLOG_FILE, "--------\n");\
        }\
        CDBCLOG_PRINT_CORE(line, (CDBCLOG_FILE, ">> %s()", aFuncName));\
        gLogDeps++;\
    } while (ACP_FALSE)

    #define CDBCLOG_OUT_CORE(line, aFuncName) do\
    {\
        gLogDeps--;\
        CDBCLOG_PRINT_CORE(line, (CDBCLOG_FILE, "<< %s()", aFuncName));\
    } while (ACP_FALSE)

    #define CDBCLOG_OUT_VAL_CORE(line, aFuncName, aForm, aVal) do\
    {\
        gLogDeps--;\
        CDBCLOG_PRINT_CORE(line, (CDBCLOG_FILE, "<< %s() : " aForm, aFuncName, aVal));\
    } while (ACP_FALSE)

    #define CDBCLOG_CALL(aFuncName)                     CDBCLOG_IN_CORE(__LINE__, aFuncName)
    #define CDBCLOG_BACK(aFuncName)                     CDBCLOG_OUT_CORE(__LINE__, aFuncName)
    #define CDBCLOG_BACK_VAL(aFuncName, aForm, aVal)    CDBCLOG_OUT_VAL_CORE(__LINE__, aFuncName, aForm, aVal)
    #define CDBCLOG_IN()                                CDBCLOG_IN_CORE(0, CDBC_FUNC_NAME)
    #define CDBCLOG_OUT()                               CDBCLOG_OUT_CORE(0, CDBC_FUNC_NAME)
    #define CDBCLOG_OUT_VAL(aForm, aVal)                CDBCLOG_OUT_VAL_CORE(0, CDBC_FUNC_NAME, aForm, aVal)
#else
    #define CDBCLOG_PRINT(aStr)
    #define CDBCLOG_PRINTF_ARG1(aForm, aArg1)
    #define CDBCLOG_PRINTF_ARG2(aForm, aArg1, aArg2)
    #define CDBCLOG_PRINT_VAL(aForm, aVal)
    #define CDBCLOG_CALL(aFuncName)
    #define CDBCLOG_BACK(aFuncName)
    #define CDBCLOG_BACK_VAL(aFuncName, aForm, aVal)
    #define CDBCLOG_IN()
    #define CDBCLOG_OUT()
    #define CDBCLOG_OUT_VAL(aForm, aVal)
#endif



extern acp_char_t   *gLogDepsStr;
extern acp_sint32_t  gLogDeps;
extern acp_char_t   *gLogOption;



ACP_EXTERN_C_END

#endif /* CDBC_LOG_H */

