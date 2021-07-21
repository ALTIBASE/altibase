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
#include <ulnPrivate.h>
#include <ulnString.h>

acp_char_t* ulnStrRchr(acp_char_t *aString, acp_sint32_t aChar)
{
    acp_char_t *sP = aString + acpCStrLen(aString, ACP_SINT32_MAX);

    while (*sP != aChar)
    {
        if (sP == aString)
        {
            return 0;
        }
        else
        {
            sP--;
        }
    }

    return sP;
}

/* proj-1538 ipv6
 * this is added for using in ulnParseIPv6Addr to find ':'
 */
acp_char_t* ulnStrChr(acp_char_t *aString, acp_sint32_t aChar)
{
    acp_char_t *sP = aString;
    while (*sP != '\0')
    {
        if (*sP == aChar)
        {
            return sP;
        }
        sP++;
    }
    return NULL;
}

acp_char_t* ulnStrSep(acp_char_t **aStringp, const acp_char_t *aDelim)
{
    acp_char_t *s;
    const acp_char_t *sPanp;
    acp_sint32_t c, sc;
    acp_char_t *sTok;
    if ((s = *aStringp) != NULL)
    {

        for(sTok = s;;)
        {
            c = *s++;
            sPanp = aDelim;
            do
            {
                if ((sc = *sPanp++) == c)
                {
                    if (c == 0)
                    {
                        s = NULL;
                    }
                    else
                    {
                        *(--s)=0;
                        ++s;
                    }
                    *aStringp = s;
                    return  sTok;
                }
            } while (sc != 0);
        }
    }
    return NULL;
}

