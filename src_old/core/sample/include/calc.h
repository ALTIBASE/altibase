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
 * $Id: calc.h 1414 2007-12-07 08:14:12Z shsuh $
 ******************************************************************************/

#include <acp.h>
#include <ace.h>
#include <acl.h>


#define CALC_VAR_MAX 100


/*
 * special token id
 */
#define CALC_TOKEN_INVALID -1
#define CALC_TOKEN_EOF      0


/*
 * variable dictionary
 */
typedef struct calcVar
{
    ACP_STR_DECLARE_DYNAMIC(mName);
    acp_sint32_t mValue;
} calcVar;

/*
 * context structure
 */
typedef struct calcContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;

    const acp_char_t *mCursor;
    const acp_char_t *mLimit;
    const acp_char_t *mToken;

    acp_bool_t        mError;
    calcVar           mVar[CALC_VAR_MAX];
} calcContext;


/*
 * token structure
 */
typedef struct calcToken
{
    const acp_char_t *mPtr;
    acp_sint32_t      mLen;
} calcToken;


/*
 * lexer interface
 */
acp_sint32_t calcGetToken(calcContext *aContext);

/*
 * parser interface
 */
void *calcParseAlloc(void);
void  calcParseFree(void *aParser);
void  calcParse(void         *aParser,
                acp_sint32_t  aTokenID,
                calcToken     aToken,
                calcContext  *aContext);
#if defined(ACP_CFG_DEBUG)
void calcParseTrace(acp_std_file_t *aFile, acp_char_t *aPrefix);
#endif

/*
 * variable access functions
 */
acp_sint32_t calcGetVar(calcContext *aContext, calcToken *aToken);
void         calcSetVar(calcContext  *aContext,
                        calcToken    *aToken,
                        acp_sint32_t  aValue);
