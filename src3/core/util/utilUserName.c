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
 * $Id:
 ******************************************************************************/

#include <acp.h>
#include <aceError.h>

#define UTIL_USERNAME_MAX_LENGTH 128

acp_sint32_t main(void)
{
    acp_char_t sUsername[UTIL_USERNAME_MAX_LENGTH];
    acp_rc_t   sRC;

    sRC = acpSysGetUserName(sUsername, UTIL_USERNAME_MAX_LENGTH);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    (void)acpFprintf(ACP_STD_OUT, "%s\n", sUsername);
    (void)acpStdFlush(ACP_STD_OUT);
    return 0;

    ACP_EXCEPTION_END;
    return 1;
}
