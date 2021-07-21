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

void ulnDebugDumpBuffer(acp_uint8_t *aBuffer, acp_uint32_t aSizeOfBuffer)
{
    acp_sint32_t i;
    acp_sint32_t j;

    for (j = i = 0; (acp_uint32_t)i < aSizeOfBuffer; i++, j++)
    {
        if (j == 0)
        {
            acpFprintf(ACP_STD_OUT, "%p : ", aBuffer + i);
        }

        acpFprintf(ACP_STD_OUT, "%02X ", *(aBuffer + i));

        if (j == 0x13)
        {
            acpFprintf(ACP_STD_OUT, "\n");
            j = -1;
        }
        else if ((j & 0x03) == 0x03)
        {
            acpFprintf(ACP_STD_OUT, " ");
        }
    }

    if (j != 0x13)
    {
        acpFprintf(ACP_STD_OUT, "\n");
    }

    acpFprintf(ACP_STD_OUT, "\n");

    acpStdFlush(ACP_STD_OUT);
}

