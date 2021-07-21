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
 
#include <idsRC4.h>


void idsRC4::setKey(RC4Context *aCtx, const UChar *aKey, UInt aKeyLen)
{
    UChar      *sState;
    UInt        sKeyIndex;
    UInt        sStateIndex;
    UInt        sCounter;
    UInt        t, u;

    sState = aCtx->state;
    aCtx->x = 0;
    aCtx->y = 0;

    /* initialize */
    for(sCounter = 0; sCounter < 256; sCounter++)
    {
        sState[sCounter] = sCounter;
    }
    sKeyIndex = 0;
    sStateIndex = 0;

    for(sCounter = 0; sCounter < 256; sCounter++)
    {
        t = sState[sCounter];
        sStateIndex = (sStateIndex + aKey[sKeyIndex] + t) & 0xff;
        u = sState[sStateIndex];

        /* swap */
        sState[sStateIndex] = t;
        sState[sCounter] = u;

        /* check boundary */
        if(++sKeyIndex >= aKeyLen)
        {
            sKeyIndex = 0;
        }
    }
}


UInt idsRC4::rc4Byte(RC4Context *aCtx)
{
    UInt        x, y;
    UInt        sx, sy;
    UChar      *sState;

    sState = aCtx->state;

    x = (aCtx->x + 1) & 0xff;
    sx = sState[x];

    y = (sx + aCtx->y) & 0xff;
    sy = sState[y];

    aCtx->x = x;
    aCtx->y = y;

    sState[y] = sx;
    sState[x] = sy;

    return sState[(sx + sy) & 0xff];
}


void idsRC4::convert(RC4Context *aCtx, const UChar *aSrc,
                     UInt aSrcLen, UChar *aDest)
{
    UInt        i;

    /*
     * For either encryption or decryption, the input text is processed one
     * byte at a time.  A pseudorandom byte is generated:
     */
    for(i = 0; i < aSrcLen; i++)
    {
        // XOR Operation
        aDest[i] = aSrc[i] ^ rc4Byte(aCtx);

    }
}
