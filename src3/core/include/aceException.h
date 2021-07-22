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
 * $Id: aceException.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACE_EXCEPTION_H_)
#define _O_ACE_EXCEPTION_H_

/**
 * @file
 * @ingroup CoreError
 */

#include <acpCallstack.h>
#include <acpStd.h>
#include <aceError.h>
#include <acpDl.h>

ACP_EXTERN_C_BEGIN


#define ACE_EXCEPTION_MSG_LEN    256

#if defined(ACP_CFG_DEBUG)
#define ACE_EXCEPTION_EXPR_LEN   1024
#define ACE_EXCEPTION_STACK_SIZE 10
#else
#define ACE_EXCEPTION_EXPR_LEN   40
#define ACE_EXCEPTION_LINE_LEN   4
#endif


typedef struct ace_exception_t
{
    ace_msgid_t  mErrorCode;
    acp_char_t   mErrorMsg[ACE_EXCEPTION_MSG_LEN];
#if defined(ACP_CFG_DEBUG)
    acp_sint32_t mExprCount;
    acp_char_t   mExpr[ACE_EXCEPTION_STACK_SIZE][ACE_EXCEPTION_EXPR_LEN];
#else
    acp_char_t   mExpr[ACE_EXCEPTION_EXPR_LEN];
#endif
} ace_exception_t;


/**
 * callback function of #ACE_SET_ERROR
 * @param aContext current context
 */
typedef void ace_error_callback_t(void *aContext);

#define ACE_ERROR_CALLBACK_DECLARE_GLOBAL_AREA                              \
static ace_error_callback_t* gErrorCallbackGlobals[ACE_PRODUCT_MAX + 1]
#define ACE_ERROR_CALLBACK_GLOBAL_AREA \
    ((ace_error_callback_t***)(&gErrorCallbackGlobals))
#define ACE_ERROR_CALLBACK_SET_GLOBAL_AREA                                  \
    do                                                                      \
    {                                                                       \
        aceErrorSetCallbackArea(ACE_ERROR_CALLBACK_GLOBAL_AREA);            \
    } while(0)

#if defined(ACP_CFG_DEBUG) || defined(ACP_CFG_DOXYGEN)

/**
 * clears the exception
 */
#define ACE_CLEAR()                                             \
    do                                                          \
    {                                                           \
        ((aclContext *)aContext)->mException->mErrorCode = 0;   \
        ((aclContext *)aContext)->mException->mExprCount = 0;   \
    } while (0)

/**
 * marks a statement as an exception point
 * @param aStmt a statement
 */
#define ACE_SET(aStmt)                                                      \
    do                                                                      \
    {                                                                       \
        ace_exception_t *sLocalObj_MACRO_LOCAL_VAR =                        \
            ((aclContext *)aContext)->mException;                           \
                                                                            \
        if (sLocalObj_MACRO_LOCAL_VAR->mExprCount < ACE_EXCEPTION_STACK_SIZE) \
        {                                                                   \
            (void)acpSnprintf(sLocalObj_MACRO_LOCAL_VAR->                   \
                              mExpr[sLocalObj_MACRO_LOCAL_VAR->mExprCount], \
                              ACE_EXCEPTION_EXPR_LEN,                       \
                              "%s:%d: ACE_SET(%s)",                         \
                              __FILE__,                                     \
                              __LINE__,                                     \
                              #aStmt);                                      \
            sLocalObj_MACRO_LOCAL_VAR->mExprCount++;                        \
        }                                                                   \
        else                                                                \
        {                                                                   \
        }                                                                   \
                                                                            \
        aStmt;                                                              \
    } while (0)

/**
 * tests an expression and if it is true then jump to the #ACE_EXCEPTION_END
 * @param aExpr an expression to test
 */
#define ACE_TEST(aExpr)                                                        \
    do                                                                         \
    {                                                                          \
        if (aExpr)                                                             \
        {                                                                      \
            ace_exception_t *sLocalObj_MACRO_LOCAL_VAR =                    \
                ((aclContext *)aContext)->mException;                       \
                                                                               \
            if (sLocalObj_MACRO_LOCAL_VAR->mExprCount <                     \
                ACE_EXCEPTION_STACK_SIZE)                                   \
            {                                                                  \
                (void)acpSnprintf(sLocalObj_MACRO_LOCAL_VAR->               \
                                  mExpr[sLocalObj_MACRO_LOCAL_VAR->mExprCount],\
                                  ACE_EXCEPTION_EXPR_LEN,                      \
                                  "%s:%d: ACE_TEST(%s)",                       \
                                  __FILE__,                                    \
                                  __LINE__,                                    \
                                  #aExpr);                                     \
                sLocalObj_MACRO_LOCAL_VAR->mExprCount++;                    \
            }                                                                  \
            else                                                               \
            {                                                                  \
            }                                                                  \
                                                                               \
            goto ACE_EXCEPTION_END_LABEL;                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
        }                                                                      \
    } while (0)

/**
 * tests an expression and if it is true then jump to the exception @a aLabel
 * @param aExpr an expression to test
 * @param aLabel label to jump if @a aExpr is true
 */
#define ACE_TEST_RAISE(aExpr, aLabel)                                          \
    do                                                                         \
    {                                                                          \
        if (aExpr)                                                             \
        {                                                                      \
            ace_exception_t *sLocalObj_MACRO_LOCAL_VAR =                    \
                ((aclContext *)aContext)->mException;                       \
                                                                               \
            if (sLocalObj_MACRO_LOCAL_VAR->mExprCount <                     \
                ACE_EXCEPTION_STACK_SIZE)                                   \
            {                                                                  \
                (void)acpSnprintf(sLocalObj_MACRO_LOCAL_VAR->               \
                                  mExpr[sLocalObj_MACRO_LOCAL_VAR->mExprCount],\
                                  ACE_EXCEPTION_EXPR_LEN,                      \
                                  "%s:%d: ACE_TEST_RAISE(%s, %s)",             \
                                  __FILE__,                                    \
                                  __LINE__,                                    \
                                  #aExpr,                                      \
                                  #aLabel);                                    \
                sLocalObj_MACRO_LOCAL_VAR->mExprCount++;                    \
            }                                                                  \
            else                                                               \
            {                                                                  \
            }                                                                  \
                                                                               \
            goto aLabel;                                                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
        }                                                                      \
    } while (0)

/**
 * jump to the exception @a aLabel
 * @param aLabel label to jump
 */
#define ACE_RAISE(aLabel)                                                      \
    do                                                                         \
    {                                                                          \
        ace_exception_t *sLocalObj_MACRO_LOCAL_VAR =                        \
            ((aclContext *)aContext)->mException;                           \
                                                                               \
        if (sLocalObj_MACRO_LOCAL_VAR->mExprCount < ACE_EXCEPTION_STACK_SIZE) \
        {                                                                      \
            (void)acpSnprintf(sLocalObj_MACRO_LOCAL_VAR->                   \
                              mExpr[sLocalObj_MACRO_LOCAL_VAR->mExprCount], \
                              ACE_EXCEPTION_EXPR_LEN,                          \
                              "%s:%d: ACE_RAISE(%s)",                          \
                              __FILE__,                                        \
                              __LINE__,                                        \
                              #aLabel);                                        \
            sLocalObj_MACRO_LOCAL_VAR->mExprCount++;                        \
        }                                                                      \
        else                                                                   \
        {                                                                      \
        }                                                                      \
                                                                               \
        goto aLabel;                                                           \
    } while (0)

#else

#define ACE_CLEAR()

#define ACE_SET(aStmt)                                                         \
    do                                                                         \
    {                                                                          \
        ace_exception_t *sLocalObj_MACRO_LOCAL_VAR =                        \
            ((aclContext *)aContext)->mException;                           \
                                                                               \
        (void)acpSnprintf(sLocalObj_MACRO_LOCAL_VAR->mExpr,                 \
                          ACE_EXCEPTION_EXPR_LEN,                              \
                          "%.*s:%d",                                           \
                          ACE_EXCEPTION_EXPR_LEN - ACE_EXCEPTION_LINE_LEN - 2, \
                          __FILE__,                                            \
                          __LINE__);                                           \
                                                                               \
        aStmt;                                                                 \
    } while (0)

#define ACE_TEST(aExpr)                         \
    do                                          \
    {                                           \
        if (aExpr)                              \
        {                                       \
            ACP_UNUSED(aContext);               \
                                                \
            goto ACE_EXCEPTION_END_LABEL;       \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)

#define ACE_TEST_RAISE(aExpr, aLabel)           \
    do                                          \
    {                                           \
        if (aExpr)                              \
        {                                       \
            ACP_UNUSED(aContext);               \
                                                \
            goto aLabel;                        \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)

#define ACE_RAISE(aLabel)                       \
    do                                          \
    {                                           \
        ACP_UNUSED(aContext);                   \
                                                \
        goto aLabel;                            \
    } while (0)

#endif


#if defined(ALINT)
#define ACE_EXCEPTION(aLabel) aLabel:
#else
/**
 * ends previous exception handling and
 * begins handling of an exception @a aLabel
 * @param aLabel exception label to handle
 */
#define ACE_EXCEPTION(aLabel) goto ACE_EXCEPTION_END_LABEL; aLabel:
#endif

/**
 * begins handling of an exception @a aLabel
 * @param aLabel exception label to handle
 */
#define ACE_EXCEPTION_CONT(aLabel) aLabel:

/**
 * ends all exception handling
 */
#define ACE_EXCEPTION_END                       \
    ACE_EXCEPTION_END_LABEL:                    \
    do                                          \
    {                                           \
    } while (0)

/**
 * pushes current exception to a stack
 */
#define ACE_PUSH()                                                             \
    do                                                                         \
    {                                                                          \
        ace_exception_t sBackupObj = *(((aclContext *)aContext)->mException)

/**
 * pops an exception from the stack and
 * restores popped exception to current exception if @a aExpr is true
 * @param aExpr an expression
 */
#define ACE_POP(aExpr)                                                  \
    if (aExpr)                                                          \
    {                                                                   \
        *(((aclContext *)aContext)->mException) = sBackupObj;           \
    }                                                                   \
    else                                                                \
    {                                                                   \
    }                                                                   \
    } while (0)


/**
 * gets the current error value
 * @result error value
 */
#define ACE_GET_ERROR() (((aclContext *)aContext)->mException->mErrorCode)

/**
 * gets the current error message
 * @result error message as C style null termiated string
 */
#define ACE_GET_ERROR_MSG()                                                    \
    ((const acp_char_t *)((aclContext *)aContext)->mException->mErrorMsg)

/**
 * compare is same error coode 
 * @result compare result : ACP_TRUE or ACP_FALSE
 */
#define ACE_GET_ERROR_COMPARE(CodeA, CodeB) (                                  \
                        (ACE_ERROR_CODE(CodeA) == ACE_ERROR_CODE(CodeB))       \
                        ? ACP_TRUE : ACP_FALSE)

#if defined(ACP_CFG_DOXYGEN)

/**
 * sets an error
 * @param aErrorLevel error level
 * @param aErrorCode error code
 * @param ... variable number of arguments for the error message
 */
#define ACE_SET_ERROR(aErrorLevel, aErrorCode, ...)

#else

#define ACE_SET_ERROR aceSetError

#endif


/**
 * dumps the current error and exception stack to the file stream
 * @param aFile file stream as #acp_std_file_t *
 */
#define ACE_DUMP(aFile) aceDump(((aclContext *)aContext)->mException, (aFile))


ACP_EXPORT void aceSetErrorCallback(acp_sint32_t           aProductID,
                                    ace_error_callback_t **aCallbacks);


ACP_EXPORT void aceDump(ace_exception_t *aException, acp_std_file_t *aFile);

ACP_EXPORT void aceSetError(void            *aContext,
                            ace_exception_t *aException,
                            acp_uint32_t     aErrorLevel,
                            ace_msgid_t      aErrorCode, ...);


ACP_EXPORT void aceErrorSetCallbackArea(ace_error_callback_t*** aError);
ACP_EXPORT ace_error_callback_t*** aceErrorGetCallbackArea(void);

ACP_EXTERN_C_END


#endif
