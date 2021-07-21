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
 * $Id: aclReadLine.c 8570 2009-11-09 05:35:11Z gurugio $
 *
 ******************************************************************************/

#include <acpAtomic.h>
#include <acpSys.h>
#include <acpCStr.h>
#include <acpThr.h>
#include <aceAssert.h>
#include <aclReadLine.h>
#include <acpStd.h>

/* ------------------------------------------------
 *  BUGBUG :  
 *  Only Unix can support readline library
 *  But, We can find a way to support same functionality on windows,
 *  it will be possible. now, we will not support in on windows.
 * ----------------------------------------------*/

#if defined(ACL_READLINE_SUPPORT)
#include <readline.h>
#endif

/**
 * initialize aclReadLine Library
 * @param aAppName name of application 
 * @return result code
 *
 */

ACP_EXPORT acp_rc_t aclReadLineInit(acp_str_t *aAppName)
{
#if defined(ACL_READLINE_SUPPORT)
    static acp_char_t gAppName[32];
    
    ACP_UNUSED(aAppName);

    (void)acpCStrCpy(gAppName, 32,
                      acpStrGetBuffer(aAppName),
                      sizeof(gAppName) - 1);
    gAppName[sizeof(gAppName) - 1] = 0;
    rl_readline_name = gAppName;
    
#else
    ACP_UNUSED(aAppName);
#endif
    
    return ACP_RC_SUCCESS;
}


/**
 * finalize aclReadLine Library
 * @return result code
 *
 */
ACP_EXPORT acp_rc_t aclReadLineFinal()
{
    return ACP_RC_SUCCESS;
}


/**
 * load history file to history buffer
 * @param aFileName filename to be loaded
 * @return result code
 *
 */

ACP_EXPORT acp_rc_t aclReadLineLoadHistory(acp_str_t *aFileName)
{
#if defined(ACL_READLINE_SUPPORT)
    return (read_history(acpStrGetBuffer(aFileName)) == 0 ?
            ACP_RC_SUCCESS : ACP_RC_ESRCH);
#else
    ACP_UNUSED(aFileName);
    return ACP_RC_SUCCESS;
#endif
}


/**
 * save current history contents to file
 * @param aFileName filename to be saved
 * @return result code
 *
 */

ACP_EXPORT acp_rc_t aclReadLineSaveHistory(acp_str_t *aFileName)
{
#if defined(ACL_READLINE_SUPPORT)
    return (write_history(acpStrGetBuffer(aFileName)) == 0 ?
            ACP_RC_SUCCESS : ACP_RC_ESRCH);
#else
    ACP_UNUSED(aFileName);
    return ACP_RC_SUCCESS;
#endif
}


/**
 * add aStr to history buffer
 * @param aStr string to be added
 * @return result code
 *
 */

ACP_EXPORT acp_rc_t aclReadLineAddHistory(acp_str_t *aStr)
{
#if defined(ACL_READLINE_SUPPORT)
    return (add_history(acpStrGetBuffer(aStr)) == 0 ?
            ACP_RC_SUCCESS : ACP_RC_EINVAL);
#else
    ACP_UNUSED(aStr);
    return ACP_RC_SUCCESS;
#endif
}


/**
 * check aStr whether the aStr is same with final history buffer.
 * @param aStr String to be compared.
 * @return result code
 *
 */

ACP_EXPORT acp_bool_t aclReadLineIsInHistory(acp_str_t *aStr)
{
#if defined(ACL_READLINE_SUPPORT)
    HIST_ENTRY *sOldHist = history_get(history_length);

    if (sOldHist == NULL)
    {
        return ACP_FALSE;
    }
    else
    {
        if (acpStrCmpCString(aStr, sOldHist->line, ACP_STR_CASE_SENSITIVE) == 0)
        {
            return ACP_TRUE;
        }
        else
        {
            return ACP_FALSE;
        }
    }
#else
    ACP_UNUSED(aStr);
    return ACP_TRUE;
#endif
}

/**
 * set another completion function
 * @param aFunc function pointer (aclReadLineCallbackFunc type)
 * to be set. A completion function is called when user strokes tab-tab.
 * New completion function must return C-string pointer that it want to
 * show for the user or NULL which terminates calling completion function.
 * @return no return
 *
 */
ACP_EXPORT void aclReadLineSetComplete(aclReadLineCallbackFunc_t *aFunc)
{
#if defined(ACL_READLINE_SUPPORT)
    /* rl_completion_entry_function is defined in 
     * acl/libedit/src/readline.c. Its type is Function
     * declared in acl/libedit/include/readline.h. */
    rl_completion_entry_function = (Function *)aFunc;
#endif
}

/**
 * get string from console.
 * @param aPrompt string to be used as a prompt
 * @param aGetString string input by user on console.
 * @return result code
 *
 */
ACP_EXPORT acp_rc_t aclReadLineGetString(acp_char_t *aPrompt,
                                         acp_str_t  *aGetString)
{
    acp_rc_t sRC;

#if defined(ACL_READLINE_SUPPORT)
    acp_char_t *sBuf = readline(aPrompt);

    if(NULL == sBuf)
    {
        sRC = ACP_RC_EOF;
    }
    else
    {
        sRC = acpStrCpyCString(aGetString, sBuf);
    }
#else
    
    (void)acpPrintf("%s", aPrompt);
    sRC = acpStdGetString(ACP_STD_IN, aGetString);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        (void)acpStrReplaceChar(aGetString, '\n', 0, 0, -1);
    }
    else
    {
        /* Do nothing */
    }
#endif
    return sRC;
}


