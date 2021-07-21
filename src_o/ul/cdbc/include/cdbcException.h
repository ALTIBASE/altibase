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

#ifndef CDBC_EXCEPTION_H
#define CDBC_EXCEPTION_H 1

ACP_EXTERN_C_BEGIN



#define CDBC_EXCEPTION_CONT(aLabel)     ACI_EXCEPTION_CONT(aLabel)
#define CDBC_EXCEPTION(aExpr)           ACI_EXCEPTION(aExpr)
#define CDBC_EXCEPTION_END              ACI_EXCEPTION_END
#ifdef USE_CDBCLOG
    #define CDBC_ASSERT(aExpr) do\
    {\
        if (!(aExpr))\
        {\
            CDBCLOG_PRINT( "Assert error : " # aExpr );\
            aceAssert( # aExpr, ACP_FILE_NAME, ACP_LINE_NUM ); \
        }\
    } while (ACP_FALSE)

    #if defined(DEBUG)
        #define CDBC_DASSERT(aExpr) CDBC_ASSERT(aExpr)
    #else
        #define CDBC_DASSERT(aExpr)
    #endif

    #define CDBC_RAISE(aLabel) do\
    {\
        CDBCLOG_PRINT( # aLabel " Exception occurred." );\
        goto aLabel;\
    } while (ACP_FALSE)

    #define CDBC_TEST(aExpr) do\
    {\
        if (aExpr)\
        {\
            CDBCLOG_PRINT( "Exception occurred. : " # aExpr );\
            goto ACI_EXCEPTION_END_LABEL;\
        }\
    } while (ACP_FALSE)

    #define CDBC_TEST_RAISE(aExpr, aLabel) do\
    {\
        if (aExpr)\
        {\
            CDBCLOG_PRINT( # aLabel " Exception occurred. : " # aExpr );\
            goto aLabel;\
        }\
    } while (ACP_FALSE)
#else
    #define CDBC_ASSERT(aExpr)              ACE_ASSERT(aExpr)
    #define CDBC_DASSERT(aExpr)             ACE_DASSERT(aExpr)
    #define CDBC_RAISE(aLabel)              ACI_RAISE(aLabel)
    #define CDBC_TEST(aExpr)                ACI_TEST(aExpr)
    #define CDBC_TEST_RAISE(aExpr, aLabel)  ACI_TEST_RAISE(aExpr, aLabel)
#endif



ACP_EXTERN_C_END

#endif /* CDBC_EXCEPTION_H */

