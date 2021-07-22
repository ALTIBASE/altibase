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

#include <uln.h>
#include <sqlcli.h>

/* ------------------------------------------------
 *  Client Type
 * ----------------------------------------------*/

const acp_char_t *ulcGetClientType()
{
    return "UNIX_ODBC";
}

/* ------------------------------------------------
 *  SQLConnect Connection Callback
 * ----------------------------------------------*/

#define GETPROFILESTRING_FUNCTION "SQLGetPrivateProfileString"

/**
 *  Global Pointer to the SQLGetPrivateProfileString function
 */

ulnGetProfileString  gPrivateProfileFuncPtr = NULL;

ACI_RC setupSQLGetPrivateProfileString(ulnDbc *aDbc)
{
    ulnGetProfileString   sFunctionPtr;
    acp_dl_t              sLibHandle;
    acp_char_t           *sLibFileName = NULL; // bug-26338

	acp_bool_t            sOpen = ACP_FALSE;

    ACP_UNUSED(aDbc);

    if( gPrivateProfileFuncPtr == NULL)
    {
        ACI_TEST_RAISE(acpDlOpen(&sLibHandle,
                                 NULL,
                                 NULL,
                                 ACP_FALSE) != ACP_RC_SUCCESS,
                       ERR_LD_OPEN);

		sOpen = ACP_TRUE;

        sFunctionPtr = (ulnGetProfileString)acpDlSym(&sLibHandle,
                                                     GETPROFILESTRING_FUNCTION);

        if(sFunctionPtr == NULL)
        {
            // fix BUG-20582
            acpDlClose(&sLibHandle);

            //fix BUG-22447 IODBC UNIX ODBC mananger co-work
            // bug-26338: print dlerr msg when dlopen failed for iodbc
            if (acpEnvGet("UNIX_ODBC_INST_LIB_NAME", &sLibFileName) != ACP_RC_SUCCESS)
            {
                sLibFileName = ALTIBASE_ODBCINST_LIBRARY_PATH;
            }

            /* fix BUG-17606 */
            /* BUG-33012 A flag when opening UNIX_ODBC_INST_LIB_NAME should be changed */
            ACI_TEST_RAISE(acpDlOpen(&sLibHandle,
                                     NULL,
                                     sLibFileName,
                                     ACP_FALSE) != ACP_RC_SUCCESS,
                           ERR_LD_OPEN);

            sFunctionPtr  = (ulnGetProfileString)acpDlSym(&sLibHandle,
                                                          GETPROFILESTRING_FUNCTION);

            ACI_TEST_RAISE(sFunctionPtr == NULL, ERR_LD_FN);
        }

        gPrivateProfileFuncPtr = sFunctionPtr;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_LD_OPEN);
    {
        // bug-26338: print dlerr msg when dlopen failed for iodbc
        acpPrintf("Altibase: setupSQLGetPrivateProfile: dlopen(%s) error:\n %s\n",
                  (sLibFileName != NULL) ? sLibFileName: "NULL",
                  acpDlError(&sLibHandle));
    }
    ACI_EXCEPTION(ERR_LD_FN);
    {
        /* BUGBUG-TODO - Message ??*/
    }
    ACI_EXCEPTION_END;

    // BUG-40316
    if( sOpen ==  ACP_TRUE )
    {
        acpDlClose(&sLibHandle);
    }

    return ACI_FAILURE;
}

ulnSQLConnectFrameWork gSQLConnectModule =
{
    setupSQLGetPrivateProfileString,
    ulnConnect
};

/* ------------------------------------------------
 *  SQLDriverConnect Connection Callback
 * ----------------------------------------------*/

static ACI_RC ulcsDummyCheckConnStrConstraint(acp_char_t   *aConnStr,
                                              SQLUSMALLINT  aDriverCompletion,
                                              acp_bool_t   *aIsPrompt )
{
    ACP_UNUSED(aConnStr);
    ACP_UNUSED(aDriverCompletion);

    // Prompt를 띄울 이유가 없음.
    *aIsPrompt = ACP_FALSE;

    return ACI_SUCCESS;
}

ulnSQLDriverConnectFrameWork gSQLDriverConnectModule =
{
    setupSQLGetPrivateProfileString,
    ulcsDummyCheckConnStrConstraint,
    NULL,
    ulnDriverConnect
};
