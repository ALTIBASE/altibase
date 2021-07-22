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
 * $Id: actTest.h 6321 2009-07-10 06:41:11Z shawn $
 ******************************************************************************/

#if !defined(_O_ACT_TEST_H_)
#define _O_ACT_TEST_H_

/**
 * @file
 * @ingroup CoreUnit
 */

#include <acpPrintf.h>
#include <acpProc.h>


ACP_EXTERN_C_BEGIN


ACP_EXPORT void actTestBegin(void);
ACP_EXPORT void actTestEnd(void);

ACP_EXPORT void actTestError(const acp_char_t *aFileName, acp_sint32_t aLine);

ACP_INLINE void actTestPrintHint(void)
{
    acp_char_t sSysMsg[256];
    acp_char_t sNetMsg[256];

    acp_rc_t sSysRC = ACP_RC_GET_OS_ERROR();
    acp_rc_t sNetRC = ACP_RC_GET_NET_ERROR();

    acpErrorString(sSysRC, sSysMsg, sizeof(sSysMsg));
    acpErrorString(sNetRC, sNetMsg, sizeof(sNetMsg));

    (void)acpPrintf(" (Hint Sys Error = [%d \"%s\"] "
                    "Net Error = [%d \"%s\"])\n",
                    (acp_sint32_t)sSysRC, sSysMsg,
                    (acp_sint32_t)sNetRC, sNetMsg);
}


/**
 * begins an unit test
 * this will call acpInitialize() automatically, so you do not need to call it.
 */
#define ACT_TEST_BEGIN() actTestBegin()

/**
 * ends an unit test
 * this will call acpFinalize() and exit the process.
 */
#define ACT_TEST_END()   actTestEnd()

/**
 * checks an expression and print error if the expression fails
 * @param aExpr an expression
 */
#define ACT_CHECK(aExpr)                                \
    do                                                  \
    {                                                   \
        if (aExpr)                                      \
        {                                               \
        }                                               \
        else                                            \
        {                                               \
            actTestError(__FILE__, __LINE__);           \
            (void)acpPrintf("%s", #aExpr);              \
            actTestPrintHint();                         \
        }                                               \
    } while (0)

/**
 * checks an expression and print the specified message if the expression fails
 * @param aExpr an expression
 * @param aMsg message to print;
 * this should be parenthesized because it actually calls 'acpPrintf @a aMsg'
 */
#define ACT_CHECK_DESC(aExpr, aMsg)                     \
    do                                                  \
    {                                                   \
        if (aExpr)                                      \
        {                                               \
        }                                               \
        else                                            \
        {                                               \
            actTestError(__FILE__, __LINE__);           \
            (void)acpPrintf aMsg ;                      \
            actTestPrintHint();                         \
        }                                               \
    } while (0)

/**
 * print an error message
 * @param aMsg message to print;
 * this should be parenthesized because it actually calls 'acpPrintf @a aMsg'
 */
#define ACT_ERROR(aMsg)                                 \
    do                                                  \
    {                                                   \
        actTestError(__FILE__, __LINE__);               \
        (void)acpPrintf aMsg ;                          \
        actTestPrintHint();                             \
    } while (0)

/**
 * checks an expression and exit program if the expression fails
 * @param aExpr an expression
 */
#define ACT_ASSERT(aExpr)                               \
    do                                                  \
    {                                                   \
        if (aExpr)                                      \
        {                                               \
        }                                               \
        else                                            \
        {                                               \
            actTestError(__FILE__, __LINE__);           \
            (void)acpPrintf("%s", #aExpr);              \
            actTestPrintHint();                         \
            acpProcExit(1);                             \
        }                                               \
    } while (0)

/**
 * checks an expression and exit the program if the expression fails
 * @param aExpr an expression
 * @param aMsg message to print;
 * this should be parenthesized because it actually calls 'acpPrintf @a aMsg'
 */
#define ACT_ASSERT_DESC(aExpr, aMsg)                    \
    do                                                  \
    {                                                   \
        if (aExpr)                                      \
        {                                               \
        }                                               \
        else                                            \
        {                                               \
            actTestError(__FILE__, __LINE__);           \
            (void)acpPrintf aMsg ;                      \
            actTestPrintHint();                         \
            acpProcExit(1);                             \
        }                                               \
    } while (0)


ACP_EXTERN_C_END


#endif
