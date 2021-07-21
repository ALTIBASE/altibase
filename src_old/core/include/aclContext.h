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
 * $Id: aclContext.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACL_CONTEXT_H_)
#define _O_ACL_CONTEXT_H_

/**
 * @file
 * @ingroup CoreError
 */

#include <acpMem.h>
#include <aceException.h>


ACP_EXTERN_C_BEGIN


/**
 * predefined objects to be inserted into a context
 */
#define ACL_CONTEXT_PREDEFINED_OBJECTS          \
    ace_exception_t *mException;                \
    ace_exception_t  mExceptionObj


typedef struct aclContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;
} aclContext;


/**
 * declares a context
 * @param aContextName a context name
 * @param aVarName a variable name
 */
#define ACL_CONTEXT_DECLARE(aContextName, aVarName)     \
    aContextName  aVarName;                             \
    aContextName *aContext = &(aVarName)


/**
 * initializes the context
 */
#define ACL_CONTEXT_INIT()                                              \
    do                                                                  \
    {                                                                   \
        aclContext *sTheContext = (aclContext *)aContext;               \
                                                                        \
        sTheContext->mException = &sTheContext->mExceptionObj;          \
                                                                        \
        acpMemSet(sTheContext->mException, 0, sizeof(ace_exception_t)); \
    } while (0)

/**
 * finalizes the context
 */
#define ACL_CONTEXT_FINAL()                     \
    do                                          \
    {                                           \
    } while (0)


ACP_EXTERN_C_END


#endif
