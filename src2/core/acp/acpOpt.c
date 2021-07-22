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
 * $Id: acpOpt.c 9999 2010-02-12 01:40:38Z gurugio $
 ******************************************************************************/

/**
 * @example sampleAcpOptGet.c
 * following is the execution result of sampleAcpOpt.c
 *
 * <pre>
 * $ prog
 * $ prog --help
 *   start                         start the service
 *   stop                          stop the service
 *
 * Mandatory arguments to long options are mandatory for short options too.
 *   -u, --user=USER               specify name
 *   -h, --help                    display this help and exit
 * $ prog -u nobody start
 * command = 3
 * user = nobody
 * name = (undefined)
 * $ prog --user=nobody nothing
 * command = 0
 * user = nobody
 * name = nothing
 * $ prog --user nobody stop anything
 * command = 4
 * user = nobody
 * name = anything
 * </pre>
 */

#include <acpCStr.h>
#include <acpOpt.h>
#include <acpList.h>

#define ACP_OPT_HELP_ALIGN   32
#define ACP_OPT_MAX_OPT_LEN  512
#define ACP_OPT_MAX_SHORT_COUNT 256
#define ACP_OPT_ERR_INVALID "invalid option: "
#define ACP_OPT_ERR_L_UNK   "unknown option: --%S"
#define ACP_OPT_ERR_L_INV   "invalid use of option: --%S"
#define ACP_OPT_ERR_S_UNK   "unknown option: -%c"
#define ACP_OPT_ERR_S_INV   "invalid use of option: -%c"


ACP_INLINE const acp_opt_def_t *acpOptFindOptShrt(acp_char_t           aArg,
                                                  const acp_opt_def_t *aOptDefs)
{
    const acp_opt_def_t *sDef;

    if (aOptDefs == NULL)
    {
        return NULL;
    }
    else
    {
        for (sDef = aOptDefs; sDef->mValue != 0; sDef++)
        {
            if (sDef->mShortOpt == aArg)
            {
                return sDef;
            }
            else
            {
                /* continue */
            }
        }
    }

    return NULL;
}

ACP_INLINE const acp_opt_def_t *acpOptFindOptLong(acp_str_t           *aArg,
                                                  const acp_opt_def_t *aOptDefs)
{
    const acp_opt_def_t *sDef;

    if (aOptDefs == NULL)
    {
        return NULL;
    }
    else
    {
        for (sDef = aOptDefs; sDef->mValue != 0; sDef++)
        {
            if (acpStrCmpCString(aArg, sDef->mLongOpt, 0) == 0)
            {
                return sDef;
            }
            else
            {
                /* continue */
            }
        }
    }

    return NULL;
}

ACP_INLINE const acp_opt_cmd_t *acpOptFindCmd(acp_char_t         **aArg,
                                              const acp_opt_cmd_t *aOptCmds)
{
    const acp_opt_cmd_t *sCmd;

    if (aOptCmds == NULL)
    {
        return NULL;
    }
    else
    {
        for (sCmd = aOptCmds; sCmd->mValue != 0; sCmd++)
        {
            if (acpCStrCmp(*aArg, sCmd->mName, ACP_OPT_MAX_OPT_LEN) == 0)
            {
                return sCmd;
            }
            else
            {
                /* continue */
            }
        }
    }

    return NULL;
}

ACP_INLINE acp_rc_t acpOptParseOptShrt(acp_opt_t           *aOpt,
                                       const acp_opt_def_t *aOptDefs,
                                       acp_sint32_t        *aValue,
                                       acp_char_t         **aArg,
                                       acp_str_t           *aError)
{
    const acp_opt_def_t *sDef;
    const acp_char_t    *sCursor;
    acp_rc_t             sRC;

    sCursor = aOpt->mArgCursor;

    /*
     * advance cursor
     */
    if (sCursor[1] == '\0')
    {
        aOpt->mArgCursor = NULL;
    }
    else
    {
        aOpt->mArgCursor++;
    }

    /*
     * find option
     */
    sDef = acpOptFindOptShrt(*sCursor, aOptDefs);

    if (sDef == NULL)
    {
        (void)acpStrCpyFormat(aError, ACP_OPT_ERR_S_UNK, *sCursor);
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    /*
     * get argument of option
     */
    sRC = ACP_RC_SUCCESS;

    switch (sDef->mHasArg)
    {
        case ACP_OPT_ARG_NOTEXIST:
            *aValue = sDef->mValue;
            *aArg =  "";
            break;

        case ACP_OPT_ARG_OPTIONAL:
            (void)acpStrCpyFormat(aError, ACP_OPT_ERR_S_INV, *sCursor);
            sRC = ACP_RC_EINVAL;
            break;

        case ACP_OPT_ARG_REQUIRED:
            if ((sCursor[-1] != '-') || (sCursor[1] != '\0'))
            {
                (void)acpStrCpyFormat(aError, ACP_OPT_ERR_S_INV, *sCursor);
                sRC = ACP_RC_EINVAL;
            }
            else
            {
                aOpt->mArgCount--;
                aOpt->mArgs++;

                if (aOpt->mArgCount == 0)
                {
                    (void)acpStrCpyFormat(aError, ACP_OPT_ERR_S_INV, *sCursor);
                    sRC = ACP_RC_EINVAL;
                }
                else
                {
                    *aValue = sDef->mValue;
                    *aArg = *aOpt->mArgs;
                }
            }
            break;
    }

    return sRC;
}

ACP_INLINE acp_rc_t acpOptParseOptLong(acp_opt_t           *aOpt,
                                       const acp_opt_def_t *aOptDefs,
                                       acp_sint32_t        *aValue,
                                       acp_char_t         **aArg,
                                       acp_str_t           *aError)
{
    const acp_opt_def_t *sDef;
    const acp_char_t    *sArg     = *aOpt->mArgs;
    acp_str_t            sOptName = ACP_STR_CONST((acp_char_t *)&sArg[2]);
    acp_sint32_t         sIndex   = ACP_STR_INDEX_INITIALIZER;
    acp_rc_t             sRC;

    /*
     * separate argument of option
     */
    sRC = acpStrFindChar(&sOptName, '=', &sIndex, sIndex, 0);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acpStrSetConstCStringWithLen(&sOptName, &sArg[2], sIndex);
    }
    else
    {
        sIndex = -1;
    }

    /*
     * find option
     */
    sDef = acpOptFindOptLong(&sOptName, aOptDefs);

    if (sDef == NULL)
    {
        (void)acpStrCpyFormat(aError, ACP_OPT_ERR_L_UNK, &sOptName);
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    /*
     * get argument of option
     */
    sRC = ACP_RC_SUCCESS;

    switch (sDef->mHasArg)
    {
        case ACP_OPT_ARG_NOTEXIST:
            /*
             * long option without argument
             */
            if (sIndex == -1)
            {
                *aValue = sDef->mValue;
                *aArg = "";
            }
            else
            {
                (void)acpStrCpyFormat(aError, ACP_OPT_ERR_L_INV, &sOptName);
                sRC = ACP_RC_EINVAL;
            }
            break;

        case ACP_OPT_ARG_OPTIONAL:
            *aValue = sDef->mValue;

            if (sIndex == -1)
            {
                /*
                 * long option without argument
                 */
                if (sDef->mDefaultArg != NULL)
                {
                    *aArg = (acp_char_t *)sDef->mDefaultArg;
                }
                else
                {
                    *aArg = "";
                    
                }
            }
            else
            {
                /*
                 * long option with argument
                 */
                *aArg =  (acp_char_t *)(&sArg[sIndex + 3]);
            }
            break;

        case ACP_OPT_ARG_REQUIRED:
            /*
             * long option with argument
             */
            if (sIndex == -1)
            {
                aOpt->mArgCount--;
                aOpt->mArgs++;

                if (aOpt->mArgCount == 0)
                {
                    (void)acpStrCpyFormat(aError, ACP_OPT_ERR_L_INV, &sOptName);
                    sRC = ACP_RC_EINVAL;
                }
                else
                {
                    *aValue = sDef->mValue;
                    *aArg = *aOpt->mArgs;
                }
            }
            else
            {
                *aValue = sDef->mValue;
                *aArg = (acp_char_t *)(&sArg[sIndex + 3]);
            }
            break;
    }

    return sRC;
}

ACP_INLINE void acpOptHelpCmd(const acp_opt_cmd_t *aOptCmds, acp_str_t *aHelp)
{
    const acp_opt_cmd_t *sCmd;

    for (sCmd = aOptCmds; (sCmd != NULL) && (sCmd->mValue != 0); sCmd++)
    {
        if (sCmd->mHelp == NULL)
        {
            (void)acpStrCatFormat(aHelp, "  %s\n", sCmd->mName);
        }
        else
        {
            (void)acpStrCatFormat(aHelp,
                                  "  %-*s %s\n",
                                  ACP_OPT_HELP_ALIGN - 3,
                                  sCmd->mName,
                                  sCmd->mHelp);
        }
    }
}

ACP_INLINE void acpOptHelpOptDesc(const acp_opt_def_t *aOptDef,
                                  acp_str_t           *aUsage,
                                  acp_str_t           *aHelp)
{
    if (aOptDef->mHelp == NULL)
    {
        if (aOptDef->mDefaultArg != NULL)
        {
            (void)acpStrCatFormat(aHelp,
                                  "%-*S default: %s\n",
                                  ACP_OPT_HELP_ALIGN - 1,
                                  aUsage,
                                  aOptDef->mDefaultArg);
        }
        else
        {
            (void)acpStrCatFormat(aHelp, "%S\n", aUsage);
        }
    }
    else
    {
        if (aOptDef->mDefaultArg != NULL)
        {
            (void)acpStrCatFormat(aHelp,
                                  "%-*S %s"
                                  ACP_OPT_HELP_NEWLINE
                                  "(default: %s)\n",
                                  ACP_OPT_HELP_ALIGN - 1,
                                  aUsage,
                                  aOptDef->mHelp,
                                  aOptDef->mDefaultArg);
        }
        else
        {
            (void)acpStrCatFormat(aHelp,
                                  "%-*S %s\n",
                                  ACP_OPT_HELP_ALIGN - 1,
                                  aUsage,
                                  aOptDef->mHelp);
        }
    }
}

ACP_INLINE acp_rc_t acpOptHelpOpt(const acp_opt_def_t *aOptDefs,
                                  acp_str_t           *aHelp)
{
    const acp_opt_def_t *sDef = aOptDefs;

    ACP_STR_DECLARE_STATIC(sUsage, 100);

    ACP_STR_INIT_STATIC(sUsage);

    for (sDef = aOptDefs; sDef->mValue != 0; sDef++)
    {
        acpStrClear(&sUsage);

        if(sDef->mHelp != NULL)
        {
            switch (sDef->mHasArg)
            {
                case ACP_OPT_ARG_NOTEXIST:
                    if (sDef->mLongOpt != NULL)
                    {
                        if (sDef->mShortOpt != 0)
                        {
                            (void)acpStrCatFormat(&sUsage,
                                                  "  -%c, --%s",
                                                  sDef->mShortOpt,
                                                  sDef->mLongOpt);
                        }
                        else
                        {
                            (void)acpStrCatFormat(&sUsage,
                                                  "      --%s",
                                                  sDef->mLongOpt);
                        }
                    }
                    else
                    {
                        if (sDef->mShortOpt != 0)
                        {
                            (void)acpStrCatFormat(&sUsage,
                                                  "  -%c",
                                                  sDef->mShortOpt);
                        }
                        else
                        {
                            return ACP_RC_EINVAL;
                        }
                    }
                    break;

                case ACP_OPT_ARG_REQUIRED:
                    if (sDef->mLongOpt != NULL)
                    {
                        if (sDef->mShortOpt != 0)
                        {
                            (void)acpStrCatFormat(&sUsage,
                                                  "  -%c, --%s=%s",
                                                  sDef->mShortOpt,
                                                  sDef->mLongOpt,
                                                  sDef->mArgName);
                        }
                        else
                        {
                            (void)acpStrCatFormat(&sUsage,
                                                  "      --%s=%s",
                                                  sDef->mLongOpt,
                                                  sDef->mArgName);
                        }
                    }
                    else
                    {
                        return ACP_RC_EINVAL;
                    }
                    break;

                case ACP_OPT_ARG_OPTIONAL:
                    if ((sDef->mLongOpt != NULL) && (sDef->mShortOpt == 0))
                    {
                        (void)acpStrCatFormat(&sUsage,
                                              "      --%s[=%s]",
                                              sDef->mLongOpt,
                                              sDef->mArgName);
                    }
                    else
                    {
                        return ACP_RC_EINVAL;
                    }
                    break;
            }

            acpOptHelpOptDesc(sDef, &sUsage, aHelp);
        }
        else
        {
            /* do nothing */
        }
    }

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t acpOptCheckInsertIntoList(acp_list_t *aList,
                                              const acp_char_t *aLongOpt)
{
    acp_rc_t sRC;
    acp_list_t *sListHead = (acp_list_t *)aList;
    acp_list_node_t *sIterator = NULL;
    acp_list_node_t *sNewElem = NULL;

    /* Check if this option already in list and add it if not */
    ACP_LIST_ITERATE(sListHead, sIterator)
    {
        ACP_TEST_RAISE(acpCStrCmp(
                           (acp_char_t *)sIterator->mObj,
                           aLongOpt,
                           acpCStrLen(aLongOpt, ACP_OPT_MAX_OPT_LEN)) == 0,
                       DUP_OPTION);
    }

    /* Init new elem */
    sRC = acpMemAlloc((void **)&sNewElem, sizeof(acp_list_node_t));
    
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FAIL_MEM_ALLOC);

    acpListInitObj(sNewElem, (void *)aLongOpt);
    acpListAppendNode(sListHead, sNewElem);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(DUP_OPTION)
    {
        /* Duplicated */
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(FAIL_MEM_ALLOC)
    {
        /* Memory allocation issue */
        sRC = ACP_RC_ENOMEM;
    }
    ACP_EXCEPTION_END;

    return sRC;
}


/**
 * initializes an option object with defined arguments and commands
 *
 * @param aOpt pointer to the option object
 * @param aArgc argument count
 * @param aArgv array of arguments
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if @a aArgc is 0 or @a aArgv is NULL
 */
ACP_EXPORT acp_rc_t acpOptInit(acp_opt_t           *aOpt,
                               acp_sint32_t         aArgc,
                               acp_char_t * const   aArgv[])
{
    if ((aArgc < 1) || (aArgv == NULL))
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        aOpt->mArgCount  = aArgc;
        aOpt->mArgs      = aArgv;
        aOpt->mArgCursor = NULL;

        return ACP_RC_SUCCESS;
    }
}

/**
 * gets an option
 * @param aOpt pointer to the intialized option object
 * @param aOptDefs array of defined arguments; could be NULL
 * @param aOptCmds array of defined commands; could be NULL
 * @param aValue pointer to a variable
 * which will be stored the return value of found argument
 * @param aArg pointer to the constant string object
 * to get argument of option or command
 * @param aError pointer to a string to get error description
 * @param aErrBufLen the aError buffer size
 * @return result code
 *
 * if it finds a valid option or command, it sets @a *aValue to
 * <tt>mValue</tt> of matched @a aOptDefs or @a aOptCmds element.
 * and @a aArg will contain the argument of option or command itself.
 *
 * the <tt>mValue</tt> of the last element of @a aOptDefs and @a aOptCmds
 * should be 0.
 *
 * when you define the <tt>mValue</tt> of @a aOptDefs or @a aOptCmds,
 * you should use 0, and @a aOptDefs and @a aOptCmds should not use
 * same <tt>mValue</tt>.
 *
 * if it encounters an option that was not defined, it sets @a *aValue to 0
 * and @a aArg will contain the string.
 *
 * it returns #ACP_RC_EOF if all options have been parsed.
 *
 * it returns #ACP_RC_EINVAL if it encounters an error while parsing options,
 * for example, the required argument is not specified.
 * and @a aArg will contain the error description.
 */
ACP_EXPORT acp_rc_t acpOptGet(acp_opt_t           *aOpt,
                              const acp_opt_def_t *aOptDefs,
                              const acp_opt_cmd_t *aOptCmds,
                              acp_sint32_t        *aValue,
                              acp_char_t         **aArg,
                              acp_char_t          *aError,
                              acp_size_t           aErrBufLen)
{
    acp_rc_t             sRC = ACP_RC_SUCCESS;
    
    const acp_opt_cmd_t *sCmd;
    
    ACP_STR_DECLARE_DYNAMIC(sError); 
    ACP_STR_INIT_DYNAMIC(sError, 1024, 1024);

    /*
     * parse argument
     */
    if (aOpt->mArgCursor != NULL)
    {
        sRC = acpOptParseOptShrt(aOpt, aOptDefs, aValue, aArg, &sError);
    }
    else
    {
        /*
         * advance argument
         */
        if (aOpt->mArgCount <= 1)
        {
            sRC =  ACP_RC_EOF;
            return sRC;
        }
        else
        {
            aOpt->mArgCount--;
            aOpt->mArgs++;
        }

        if ((*aOpt->mArgs)[0] == '-')
        {
            if ((*aOpt->mArgs)[1] == '-')
            {
                /*
                 * long option
                 */
                if ((*aOpt->mArgs)[2] == '\0')
                {
                    (void)acpStrCpyCString(&sError, ACP_OPT_ERR_INVALID "--");
                    sRC = ACP_RC_EINVAL;
                }
                else
                {
                    sRC =  acpOptParseOptLong(aOpt,
                                              aOptDefs,
                                              aValue,
                                              aArg,
                                              &sError);
                }
            }
            else
            {
                /*
                 * short option
                 */
                if ((*aOpt->mArgs)[1] == '\0')
                {
                    (void)acpStrCpyCString(&sError, ACP_OPT_ERR_INVALID "-");
                    sRC = ACP_RC_EINVAL;
                }
                else
                {
                    aOpt->mArgCursor = &(*aOpt->mArgs)[1];

                    sRC =  acpOptParseOptShrt(aOpt,
                                              aOptDefs,
                                              aValue,
                                              aArg,
                                              &sError);
                }
            }
        }
        else
        {
            /*
             * command
             */
            *aArg = *aOpt->mArgs;

            sCmd    = acpOptFindCmd(aArg, aOptCmds);
            *aValue = (sCmd == NULL) ? 0 : sCmd->mValue;

            sRC =  ACP_RC_SUCCESS;
        }
    }

    /* User want to get error message */
    if (aError != NULL)
    {
        acp_char_t* sMsg = acpStrGetBuffer(&sError);
        acp_size_t  sLen;

        /* If sError has a buffer,
         * it means an error occured and the buffer has a message. */
        if (sMsg != NULL)
        {
            sLen = acpStrGetLength(&sError);
            (void)acpCStrCpy(aError, aErrBufLen, sMsg, sLen);
        }
        else
        {
            /* No error occured */
        }
    }
    else
    {
        /* NULL means no message return */
    }
        
    ACP_STR_FINAL(sError);
    return sRC;
}

/**
 * check options structure for duplicates
 *
 * @param aOptDef array of defined arguments
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if it called with NULL parameter
 * or duplicate options present in input table.
 * it returns #ACP_RC_ENOMEM in case of memory allocation problems.
 */
ACP_EXPORT acp_rc_t acpOptCheckOpts(const acp_opt_def_t *aOptDef)
{
    acp_rc_t            sRC;
    const acp_opt_def_t *sOpt;
    acp_char_t          sShortOptCount[ACP_OPT_MAX_SHORT_COUNT];
    acp_list_t          sLongOptByLengthLists[ACP_OPT_MAX_OPT_LEN];
    acp_size_t          sOptLength;
    acp_size_t          i;
    acp_list_node_t     *sIterator = NULL;
    acp_list_node_t     *sNext = NULL;

    ACP_TEST_RAISE(aOptDef == NULL, WRONG_INPUT_PARAM);

    acpMemSet(sShortOptCount, 0, sizeof(sShortOptCount));
    for(i = 0; i < ACP_OPT_MAX_OPT_LEN; i++)
    {
        acpListInit(&sLongOptByLengthLists[i]);
    }

    for(sOpt = aOptDef; sOpt->mValue != 0; sOpt++)
    {
        if((acp_uint32_t)(sOpt->mShortOpt) > 0)
        {
            if(sShortOptCount[(acp_uint32_t)sOpt->mShortOpt] == 0)
            {
                sShortOptCount[(acp_uint32_t)sOpt->mShortOpt]++;
            }
            else
            {
                ACP_RAISE(DUP_SHORT_OPTION);
            }
        }
        else
        {
            /* Do nothing */
        }

        if(sOpt->mLongOpt != NULL)
        {
            sOptLength = acpCStrLen(sOpt->mLongOpt, ACP_OPT_MAX_OPT_LEN);

            sRC = acpOptCheckInsertIntoList(
                      &sLongOptByLengthLists[sOptLength],
                      sOpt->mLongOpt);

            ACP_TEST_RAISE(ACP_RC_IS_EINVAL(sRC), DUP_LONG_OPTION);
            ACP_TEST_RAISE(ACP_RC_IS_ENOMEM(sRC), FAIL_MEM_ALLOC);
        }
        else
        {
            /* Do nothing */
        }
    }

    /* Free memory allocated for the lists */
    for(i = 0; i < ACP_OPT_MAX_OPT_LEN; i++)
    {
        ACP_LIST_ITERATE_SAFE(&sLongOptByLengthLists[i], sIterator, sNext)
        {
            acpListDeleteNode(sIterator);
            acpMemFree(sIterator);
        }
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(DUP_SHORT_OPTION)
    {
        /* Short option duplicated */
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(DUP_LONG_OPTION)
    {
        /* Long option duplicated */
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(FAIL_MEM_ALLOC)
    {
        /* Memory allocation problem */
        sRC = ACP_RC_ENOMEM;
    }
    ACP_EXCEPTION(WRONG_INPUT_PARAM)
    {
        /* Input param couldn't be NULL */
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION_END;

    /* Free memory allocated for the lists */
    for(i = 0; i < ACP_OPT_MAX_OPT_LEN; i++)
    {
        ACP_LIST_ITERATE_SAFE(&sLongOptByLengthLists[i], sIterator, sNext)
        {
            acpListDeleteNode(sIterator);
            acpMemFree(sIterator);
        }
    }

    return sRC;
}

/**
 * gets the help string for options
 *
 * @param aOptDefs array of defined arguments; could be NULL
 * @param aOptCmds array of defined commands; could be NULL
 * @param aHelp return the pointer of  help message
 * @param aHelpBufLen the aHelp Buffer Size
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if it encounters an error
 * while parsing the defined option and command definitions.
 * In order to prevent memory leak,
 * you *must* free the aHelp pointer after using which allocated string buffer
 * by this API.
 */
ACP_EXPORT acp_rc_t acpOptHelp(const acp_opt_def_t *aOptDefs,
                               const acp_opt_cmd_t *aOptCmds,
                               acp_char_t          *aHelp,
                               acp_size_t           aHelpBufLen)
{
    /*
     * clear help string
     */
    acp_rc_t sRC = ACP_RC_SUCCESS;
    ACP_STR_DECLARE_DYNAMIC(sHelp); 
    ACP_STR_INIT_DYNAMIC(sHelp, 1024, 1024);

    
    acpStrClear(&sHelp);

    /*
     * make help for commands
     */
    acpOptHelpCmd(aOptCmds, &sHelp);

    /*
     * make help for options
     */
    if ((aOptDefs != NULL) && (aOptDefs->mValue != 0))
    {
        
        if (acpStrGetLength(&sHelp) != 0)
        {
            (void)acpStrCatCString(&sHelp, "\n");
        }
        else
        {
            /* do nothing */
        }

        (void)acpStrCatCString(&sHelp,
                               "Mandatory arguments to long options "
                               "are mandatory for short options too.\n");

        sRC = acpOptHelpOpt(aOptDefs, &sHelp);
        
    }
    else
    {
        /* just returning the string */
    }

    if (aHelp != NULL)
    {
        acp_char_t *sMsg = acpStrGetBuffer(&sHelp);
        acp_size_t  sLen;

        if (sMsg != NULL)
        {
            sLen = acpStrGetLength(&sHelp);
            (void)acpCStrCpy(aHelp, aHelpBufLen, sMsg, sLen);
        }
        else
        {
            /* No error occured */
        }
    }
    else
    {
        /* NULL means no message return */
    }
        
    ACP_STR_FINAL(sHelp);
    return sRC;
}
