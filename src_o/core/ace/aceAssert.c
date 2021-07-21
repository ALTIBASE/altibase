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
 * $Id: aceAssert.c 8998 2009-12-03 07:02:04Z gurugio $
 ******************************************************************************/

#include <acpPrintf.h>
#include <acpProc.h>
#include <acpStd.h>
#include <aceAssert.h>

#define ACE_ASSERT_RECURSION (1)

static aceAssertCallback *gAceAssertCallback = NULL;
static acp_sint32_t gAceAssertDepth = 0;
static acp_bool_t gAceAssertCallbackDone = ACP_FALSE;

/**
 * sets an assert callback function
 * @param aCallback function for assert callback
 */
ACP_EXPORT void aceSetAssertCallback(aceAssertCallback *aCallback)
{
    gAceAssertCallback = aCallback;
}

ACP_EXPORT void aceAssert(const acp_char_t *aExpr,
                          const acp_char_t *aFile,
                          acp_sint32_t      aLine)
{
#if defined(__STATIC_ANALYSIS_DOING__)
    acpProcExit(0);
#else

    if(ACP_FALSE == gAceAssertCallbackDone)
    {
        if (gAceAssertCallback != NULL)
        {
            if(ACE_ASSERT_RECURSION > gAceAssertDepth)
            {
                /* Increate call depth count */
                gAceAssertDepth++;
                (*gAceAssertCallback)(aExpr, aFile, aLine);

                /* Decrease call depth count */
                gAceAssertDepth--;
            }
            else
            {
                (void)acpFprintf(ACP_STD_ERR,
                             "%s:%d: Assertion failed: %s\n",
                             aFile, aLine, aExpr);

#if defined(ACP_CFG_DEBUG)
                /* ASSERT in Assert Handler */
                (void)acpFprintf(ACP_STD_ERR,
                             "Warning! ACE_ASSERT::"
                             "Possibility of Infinite Assertion!\n"
                            );
#endif
            }
        }
        else
        {
            (void)acpFprintf(ACP_STD_ERR,
                         "%s:%d: Assertion failed: %s\n",
                         aFile, aLine, aExpr);
        }

        gAceAssertCallbackDone = ACP_TRUE;
    }
    else
    {
        (void)acpSignalSetExceptionHandler(NULL);
        (void)acpFprintf(ACP_STD_ERR,
                         "%s:%d: Assertion failed: %s\n",
                         aFile, aLine, aExpr);

#if defined(ACP_CFG_DEBUG)
        /* ASSERT in Signal Exception Handler */
        (void)acpFprintf(ACP_STD_ERR,
                         "Warning! ACE_ASSERT::"
                         "Possibility of Assertion in Signal Handler!\n"
                         );
#endif
    }

    acpProcAbort();
#endif    
}
