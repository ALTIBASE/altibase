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
    return "CLI";
}

/* ------------------------------------------------
 *  SQLConnect Connection Callback
 * ----------------------------------------------*/

static ACI_RC ulcsSetupDummy(ulnDbc * aDbc)
{
    ACP_UNUSED(aDbc);

    return ACI_SUCCESS;
}

ulnSQLConnectFrameWork gSQLConnectModule =
{
    ulcsSetupDummy,
    ulnConnect
};

/* ------------------------------------------------
 *  SQLDriverConnect Connection Callback
 * ----------------------------------------------*/

static ACI_RC ulcsDummyCheckConnStrConstraint(acp_char_t *aConnStr, SQLUSMALLINT aDriverCompletion, acp_bool_t *aIsPrompt )
{
    ACP_UNUSED(aConnStr);
    ACP_UNUSED(aDriverCompletion);

    // Prompt를 띄울 이유가 없음.
    *aIsPrompt = ACP_FALSE;

    return ACI_SUCCESS;
}

ulnSQLDriverConnectFrameWork gSQLDriverConnectModule =
{
    ulcsSetupDummy,
    ulcsDummyCheckConnStrConstraint,
    NULL,
    ulnDriverConnect
};
