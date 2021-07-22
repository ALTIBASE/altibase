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
 * $Id: actDump.c 1571 2008-01-03 05:52:39Z shsuh $
 ******************************************************************************/

#include <acpChar.h>
#include <actDump.h>


#define ACT_DUMP_BYTES_IN_ROW 16


ACP_EXPORT void actDump(void *aBuffer, acp_size_t aLen)
{
    acp_uint8_t *sBuffer = aBuffer;
    acp_uint8_t  sAscii[ACT_DUMP_BYTES_IN_ROW];
    acp_size_t   i;
    acp_size_t   j;

    for (i = 0; i < aLen; i += ACT_DUMP_BYTES_IN_ROW)
    {
        (void)acpPrintf("%08zx:", i);

        for (j = i; j < (i + ACT_DUMP_BYTES_IN_ROW); j++)
        {
            if (j < aLen)
            {
                (void)acpPrintf(" %02x", sBuffer[j]);

                if (acpCharIsPrint(sBuffer[j]) == ACP_TRUE)
                {
                    sAscii[j % ACT_DUMP_BYTES_IN_ROW] = sBuffer[j];
                }
                else
                {
                    sAscii[j % ACT_DUMP_BYTES_IN_ROW] = '.';
                }
            }
            else
            {
                (void)acpPrintf("   ");

                sAscii[j % ACT_DUMP_BYTES_IN_ROW] = ' ';
            }

            if ((j % 4) == 3)
            {
                (void)acpPrintf(" ");
            }
            else
            {
                /* do nothing */
            }
        }

        (void)acpPrintf(" |%.*s|\n", ACT_DUMP_BYTES_IN_ROW, sAscii);
    }
}
