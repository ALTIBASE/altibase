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

/* Specification: RFC 3629 */

#include <idnUtf8.h>

SInt convertMbToWc4Utf8( void    * aSrc,
                         SInt      aSrcRemain,
                         void    * aDest,
                         SInt      /* aDestRemain */ )
{
/***********************************************************************
 *
 * Description :
 *      UTF8 ==> UTF16
 *
 * Implementation :
 *      surrogate 문자는 지원하지 않음.
 *      보충문자는 UTF16으로 변환하지 않는다.
 *      (3 byte UTF8까지만 변환하면 됨)
 *
 ***********************************************************************/

    UChar   * sSrcCharPtr;
    SInt      sRet;
    UShort    wc;
    
    sSrcCharPtr = (UChar *)aSrc;

    if (sSrcCharPtr[0] < 0x80)
    {
        wc = sSrcCharPtr[0];

        WC_TO_UTF16BE( aDest, wc );
        
        sRet = 2;
    }
    else if (sSrcCharPtr[0] < 0xc2)
    {
        sRet = RET_ILSEQ;
    }
    else if (sSrcCharPtr[0] < 0xe0)
    {
        if (aSrcRemain < 2)
        {    
            sRet = RET_TOOFEW;
        }
        else
        {
            if( ((sSrcCharPtr[1] ^ 0x80) >= 0x40) )
            {
                sRet = RET_ILSEQ;
            }
            else
            {
                wc = ((UShort) (sSrcCharPtr[0] & 0x1f) << 6)
                    | (UShort) (sSrcCharPtr[1] ^ 0x80);

                WC_TO_UTF16BE( aDest, wc );
                
                sRet = 2;
            }
        }
    }
    else if (sSrcCharPtr[0] < 0xf0) 
    {
        if (aSrcRemain < 3)
        {
            sRet = RET_TOOFEW;
        }
        else
        {
            if( !( (sSrcCharPtr[1] ^ 0x80) < 0x40 && 
                   (sSrcCharPtr[2] ^ 0x80) < 0x40 && 
                   (sSrcCharPtr[0] >= 0xe1 || sSrcCharPtr[1] >= 0xa0) ) )
            {
                sRet = RET_ILSEQ;
            }
            else
            {
                wc = ((UShort) (sSrcCharPtr[0] & 0x0f) << 12)
                    | ((UShort) (sSrcCharPtr[1] ^ 0x80) << 6)
                    | (UShort) (sSrcCharPtr[2] ^ 0x80);

                WC_TO_UTF16BE( aDest, wc );

                sRet = 2;
            }
        }
    }
    else
    {
        sRet = RET_ILSEQ;
    }

    return sRet;
}

SInt convertWcToMb4Utf8( void    * aSrc,
                         SInt      /* aSrcRemain */,
                         void    * aDest,
                         SInt      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      UTF8 <== UTF16
 *
 * Implementation :
 *      surrogate 문자는 지원하지 않음.
 *      보충문자는 UTF8으로 변환하지 않는다.
 *      (3 byte UTF8까지만 변환하면 됨)
 *
 ***********************************************************************/

    SInt                sCount;
    UChar             * sDestCharPtr;
    UShort              wc;

    sDestCharPtr = (UChar *)aDest;

    UTF16BE_TO_WC( wc, aSrc );
    
    if (wc < 0x80)
    {
        sCount = 1;
    }
    else if (wc < 0x800)
    {
        sCount = 2;
    }
    else
    {
        sCount = 3;
    }

    if (aDestRemain < sCount)
    {
        return RET_TOOSMALL;
    }
    else
    {
        /* note: code falls through cases! */
        switch (sCount) 
        {
            case 3: 
                sDestCharPtr[2] = 0x80 | (wc & 0x3f); 
                wc = wc >> 6; 
                wc |= 0x800;
            case 2: 
                sDestCharPtr[1] = 0x80 | (wc & 0x3f); 
                wc = wc >> 6; 
                wc |= 0xc0;
            case 1: 
                sDestCharPtr[0] = wc;
        }

        return sCount;
    }
}
 
