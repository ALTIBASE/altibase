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
 
#include <acpStr.h>


acp_sint64_t convertToInteger(const acp_char_t *aString)
{
    acp_rc_t     sRC;
    acp_sint32_t sSign;
    acp_uint64_t sVal    = 0;
    acp_str_t    sString = ACP_STR_CONST(aString);

    sRC = acpStrToInteger(&sString, &sSign, &sVal, NULL, 0, 0);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        (void)acpPrintf("value = %lld\n", (acp_sint64_t)sVal * sSign);
    }
    else
    {
        (void)acpPrintf("error code = %d\n", sRC);
    }

    return (acp_sint64_t)sValue * sSign;
}
