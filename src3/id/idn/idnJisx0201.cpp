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

#include <idnJisx0201.h>

SInt convertMbToWc4Jisx0201( void   * aSrc,
                             SInt     /* aSrcRemain */,
                             void   * aDest,
                             SInt     /* aDestRemain */ )
{
/***********************************************************************
 *
 * Description :
 *      (EUCJP, SHIFT-JIS) ==> UTF16BE 에서 사용되는 변환
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   * sSrcCharPtr;
    SInt      sRet;
    UShort    wc;
    
    sSrcCharPtr = (UChar *)aSrc;

    if( sSrcCharPtr[0] < 0x80 )
    {
        if( sSrcCharPtr[0] == 0x7e )
        {
            wc = (UShort) 0x203e;
        }
        else
        {
            wc = (UShort) sSrcCharPtr[0];
        }

        WC_TO_UTF16BE( aDest, wc );
                
        sRet = 2;
    }
    else
    {
        if( (sSrcCharPtr[0] >= 0xa1) && (sSrcCharPtr[0] < 0xe0) )
        {
            wc = (UShort)sSrcCharPtr[0] + 0xfec0;

            WC_TO_UTF16BE( aDest, wc );

            sRet = 2;
        }
        else
        {
            sRet = RET_ILSEQ;
        }
    }

    return sRet;
}

SInt convertWcToMb4Jisx0201( void   * aSrc,
                             SInt     /* aSrcRemain */,
                             void   * aDest,
                             SInt     /* aDestRemain */ )
{
/***********************************************************************
 *
 * Description :
 *      (EUCJP, SHIFT-JIS) <== UTF16 에서 사용되는 변환
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort    wc;
    UChar   * sDestCharPtr;
    SInt      sRet;

    UTF16BE_TO_WC( wc, aSrc );
    
    sDestCharPtr = (UChar *)aDest;

    if( (wc < 0x0080) && 
        (wc != 0x007e) )
    {
        sDestCharPtr[0] = (UChar)wc;

        sRet = 1;
    }
    else
    {
        if( wc == 0x00a5 )
        {
            sDestCharPtr[0] = 0x5c;

            sRet = 1;
        }
        else if( wc == 0x203e )
        {
            sDestCharPtr[0] = 0x7e;

            sRet = 1;
        }
        else if( (wc >= 0xff61) && (wc < 0xffa0) )
        {
            sDestCharPtr[0] = (UChar)(wc - 0xfec0);

            sRet = 1;
        }
        else
        {
            sRet = RET_ILUNI;
        }
    }

    return sRet;
}
 
