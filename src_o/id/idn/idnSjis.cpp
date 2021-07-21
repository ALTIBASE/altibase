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
 
/***********************************************************************
 * $Id: qdbCommon.cpp 25810 2008-04-30 00:20:56Z peh $
 **********************************************************************/

#include <idnSjis.h>
#include <idnJisx0201.h>
#include <idnJisx0208.h>

SInt convertMbToWc4Sjis( void    * aSrc,
                         SInt      aSrcRemain,
                         void    * aDest,
                         SInt      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      SHIFTJIS ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   * sSrcCharPtr;
    SInt      sRet;
    UChar     sBuf[2];
    UChar     t1;
    UChar     t2;
    UShort    wc;
    
    sSrcCharPtr = (UChar *)aSrc;

    if( (sSrcCharPtr[0] < 0x80) || 
        ((sSrcCharPtr[0] >= 0xa1) && (sSrcCharPtr[0] <= 0xdf)) )
    {
        sRet = convertMbToWc4Jisx0201(aSrc, aSrcRemain, aDest, aDestRemain);
    }
    else
    {
        if( (sSrcCharPtr[0] >= 0x81 && sSrcCharPtr[0] <= 0x9f) || 
            (sSrcCharPtr[0] >= 0xe0 && sSrcCharPtr[0] <= 0xea) )
        {
            if (aSrcRemain < 2)
            {
                sRet = RET_TOOFEW;
            }
            else
            {
                if( (sSrcCharPtr[1] >= 0x40 && sSrcCharPtr[1] <= 0x7e) || 
                    (sSrcCharPtr[1] >= 0x80 && sSrcCharPtr[1] <= 0xfc) )
                {
                    t1 = (sSrcCharPtr[0] < 0xe0 ? 
                                    sSrcCharPtr[0]-0x81 : sSrcCharPtr[0]-0xc1);
                    t2 = (sSrcCharPtr[1] < 0x80 ? 
                                    sSrcCharPtr[1]-0x40 : sSrcCharPtr[1]-0x41);

                    sBuf[0] = 2*t1 + (t2 < 0x5e ? 0 : 1) + 0x21;
                    sBuf[1] = (t2 < 0x5e ? t2 : t2-0x5e) + 0x21;

                    sRet = convertMbToWc4Jisx0208(sBuf, 2, aDest, aDestRemain);
                }
                else
                {
                    sRet = RET_ILSEQ;
                }
            }
        }
        else if (sSrcCharPtr[0] >= 0xf0 && sSrcCharPtr[0] <= 0xf9) 
        {
            /* User-defined range. See
            * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
            if (aSrcRemain < 2)
            {
                sRet = RET_TOOFEW;
            }
            else
            {
                if( (sSrcCharPtr[1] >= 0x40 && sSrcCharPtr[1] <= 0x7e) || 
                    (sSrcCharPtr[1] >= 0x80 && sSrcCharPtr[1] <= 0xfc) )
                {
                    wc = 0xe000 + 
                        188*(sSrcCharPtr[0] - 0xf0) + 
                        (sSrcCharPtr[1] < 0x80 ? 
                         sSrcCharPtr[1]-0x40 : sSrcCharPtr[1]-0x41);

                    WC_TO_UTF16BE( aDest, wc );

                    sRet = 2;
                }
                else
                {
                    sRet = RET_ILSEQ;
                }
            }
        }
        else
        {
            sRet = RET_ILSEQ;
        }
    }

    return sRet;
}


SInt convertWcToMb4Sjis( void    * aSrc,
                         SInt      aSrcRemain,
                         void    * aDest,
                         SInt      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      SHIFTJIS <== UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   sBuf[2];
    SInt    sRet = 0;
    UChar * sDestCharPtr;
    UChar   t1;
    UChar   t2;
    UShort  wc;
    
    sDestCharPtr = (UChar *)aDest;

    UTF16BE_TO_WC( wc, aSrc );
    
    /* Try JIS X 0201-1976. */
    sRet = convertWcToMb4Jisx0201( aSrc, aSrcRemain, sBuf, 1 );

    if (sRet != RET_ILUNI)
    {
        if( sBuf[0] < 0x80 || 
            (sBuf[0] >= 0xa1 && sBuf[0] <= 0xdf) )
        {
            sDestCharPtr[0] = sBuf[0];
            return 1;
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    /* Try JIS X 0208-1990. */
    sRet = convertWcToMb4Jisx0208( aSrc, aSrcRemain, sBuf, 2 );

    if (sRet != RET_ILUNI) 
    {
        if (aDestRemain < 2)
        {
            return RET_TOOSMALL;
        }
        else
        {
            if( (sBuf[0] >= 0x21 && sBuf[0] <= 0x74) && 
                (sBuf[1] >= 0x21 && sBuf[1] <= 0x7e) )
            {
                t1 = (sBuf[0] - 0x21) >> 1;
                t2 = (((sBuf[0] - 0x21) & 1) ? 0x5e : 0) + (sBuf[1] - 0x21);

                sDestCharPtr[0] = (t1 < 0x1f ? t1+0x81 : t1+0xc1);
                sDestCharPtr[1] = (t2 < 0x3f ? t2+0x40 : t2+0x41);

                return 2;
            }
            else
            {
                // Nothing to do
            }
        }
    }
    else
    {
        // Nothing to do
    }

    /* User-defined range. See
    * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
    if ( (wc >= 0xe000) && (wc < 0xe758) )
    {
        if (aDestRemain < 2)
        {
            return RET_TOOSMALL;
        }
        else
        {
            sBuf[0] = (UInt) (wc - 0xe000) / 188;
            sBuf[1] = (UInt) (wc - 0xe000) % 188;

            sDestCharPtr[0] = sBuf[0]+0xf0;
            sDestCharPtr[1] = (sBuf[1] < 0x3f ? 
                                            sBuf[1]+0x40 : sBuf[1]+0x41);

            return 2;
        }
    }
    else
    {
        // Nothing to do
    }

    return RET_ILUNI;
}
 
