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


void findChar()
{
    acp_str_t    sStr = ACP_STR_CONST("hello, world!");
    acp_sint32_t sIndex;
    acp_rc_t     sRC;

    sIndex = ACP_STR_INDEX_INITIALIZER;

    while (1)
    {
        sRC = acpStrFindChar(&sStr,
                             'l',
                             &sIndex,
                             sIndex,
                             ACP_STR_SEARCH_BACKWARD | ACP_STR_CASE_INSENSITIVE);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpPrintf("found 'l' at %d\n", sIndex);
        }
        else if (ACP_RC_IS_ENOENT(sRC))
        {
            (void)acpPrintf("no more 'l' exists\n");
        }
        else
        {
            (void)acpPrintf("error code = %d\n", sRC);
        }
    }
}
