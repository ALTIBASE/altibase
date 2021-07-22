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
 * $Id: acpOpt.h 7488 2009-09-08 08:15:50Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_OPT_H_)
#define _O_ACP_OPT_H_

/**
 * @file
 * @ingroup CoreEnv
 *
 * acpOpt is a replacement for standard getopt() libc function.
 * acpOpt provides short and long option parsing and command parsing.
 *
 * you can define options via #acp_opt_def_t and commands via #acp_opt_cmd_t.
 *
 * see sample: @link sampleAcpOptGet.c @endlink
 */

#include <acpStr.h>


ACP_EXTERN_C_BEGIN


/**
 * new line for help string
 */
#define ACP_OPT_HELP_NEWLINE "\n                                  "

/**
 * specifies whether an argument is taken by the option or not
 * @see acp_opt_def_t
 */
typedef enum acp_opt_arg_t
{
    ACP_OPT_ARG_NOTEXIST = 0, /**< the option does not task an argument  */
    ACP_OPT_ARG_REQUIRED,     /**< the option requires an argument       */
    ACP_OPT_ARG_OPTIONAL      /**< the option takes an optional argument */
} acp_opt_arg_t;

/**
 * structure for define options
 *
 * set mShortOpt to 0 if the option have no defined short name,
 * set mLongOpt to NULL if the option have no defined long name.
 * but, at least one of them should be defined.
 *
 * the short option is case sensitive and must not be '-'.
 * the long option should begin with lower case letter[a-z]
 * and should be consist of lower case letters[a-z] and '-'.
 *
 * if the option requires an argument, it should define long option.
 * if the option takes an optional argument, it must not define short option.
 *
 * <table border="1" cellspacing="0" cellpadding="4">
 * <tr>
 * <td><b><tt>short option</tt></b></td><td><b><tt>long option</tt></b></td>
 * <td><b>no argument</b></td>
 * <td><b>required argument</b></td>
 * <td><b>optional argument</b></td>
 * </tr>
 * <tr>
 * <td><tt>'o'</tt></td><td><tt>"option"</tt></td>
 * <td>-o<br>--option</td>
 * <td>-o VALUE<br>--option VALUE<br>--option=VALUE</td>
 * <td><i>INVALID; short option must not be defined</i></td>
 * </tr>
 * <tr>
 * <td><tt>'o'</tt></td><td>&nbsp;</td>
 * <td>-o</td>
 * <td><i>INVALID; long option should be defined</i></td>
 * <td><i>INVALID; long option should be defined</i></td>
 * </tr>
 * <tr>
 * <td>&nbsp;</td><td><tt>"option"</tt></td>
 * <td>--option</td>
 * <td>--option VALUE<br>--option=VALUE</td>
 * <td>--option<br>--option=VALUE</td>
 * </tr>
 * </table>
 *
 * @see acpOptGet() acpOptHelp()
 */
typedef struct acp_opt_def_t
{
    acp_sint32_t      mValue;      /**< return value of the option        */
    acp_opt_arg_t     mHasArg;     /**< specifies argument of the option  */
    acp_char_t        mShortOpt;   /**< short option (0 if no short opt)  */
    const acp_char_t *mLongOpt;    /**< long option (NULL if no long opt) */
    const acp_char_t *mDefaultArg; /**< default argument for optional     */
    const acp_char_t *mArgName;    /**< argument name for help            */
    const acp_char_t *mHelp;       /**< help string of the option         */
} acp_opt_def_t;

/**
 * indicates the end of option definition.
 * ex)
 * acp_opt_def_t sOpts[] = 
 * {
 *      {VALUE1, ACP_OPT_ARG_OPTIONAL, ...},
 *      {VALUE2, ...},
 *      ...,
 *      ACP_OPT_SENTINEL // Option definitions after this line are ignored.
 * };
 */
#define ACP_OPT_SENTINEL {0, ACP_OPT_ARG_NOTEXIST, 0, NULL, NULL, NULL, NULL}

/**
 * structure for define command
 *
 * the name of command is case sensitive
 *
 * @see acpOptGet() acpOptHelp()
 */
typedef struct acp_opt_cmd_t
{
    acp_sint32_t      mValue; /**< return value of the command */
    const acp_char_t *mName;  /**< name of the command         */
    const acp_char_t *mHelp;  /**< help string of the command  */
} acp_opt_cmd_t;

/**
 * option object
 *
 * @see acpOptInit() acpOptGet()
 */
typedef struct acp_opt_t
{
    acp_sint32_t        mArgCount;
    acp_char_t * const *mArgs;
    const acp_char_t   *mArgCursor;
} acp_opt_t;


ACP_EXPORT acp_rc_t acpOptInit(acp_opt_t           *aOpt,
                               acp_sint32_t         aArgc,
                               acp_char_t * const   aArgv[]);

ACP_EXPORT acp_rc_t acpOptGet(acp_opt_t           *aOpt,
                              const acp_opt_def_t *aOptDefs,
                              const acp_opt_cmd_t *aOptCmds,
                              acp_sint32_t        *aValue,
                              acp_char_t         **aArg,
                              acp_char_t          *aError,
                              acp_size_t           aErrBufLen);

ACP_EXPORT acp_rc_t acpOptHelp(const acp_opt_def_t *aOptDefs,
                               const acp_opt_cmd_t *aOptCmds,
                               acp_char_t          *aHelp,
                               acp_size_t           aHelpBufLen);

ACP_EXPORT acp_rc_t acpOptCheckOpts(const acp_opt_def_t *aOptDef);

ACP_EXTERN_C_END


#endif
