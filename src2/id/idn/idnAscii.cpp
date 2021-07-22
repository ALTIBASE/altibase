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

#include <idnAscii.h>

SInt convertMbToWc4Ascii( void   * aSrc, 
                          SInt     /* aSrcRemain */, 
                          void   * aDest, 
                          SInt     /* aDestRemain */ )
{
/***********************************************************************
 *
 * Description :
 *      ASCII ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   sChar;
    SInt    sRet;

    sChar = *(UChar*) aSrc;

    if( sChar < 0x80 )
    {
        ((UChar*)aDest)[0] = 0;
        ((UChar*)aDest)[1] = sChar;
        sRet = 2;
    }
    else
    {
        sRet = RET_ILSEQ;
    }

    return sRet;
}

SInt convertWcToMb4Ascii( void   * aSrc, 
                          SInt     /* aSrcRemain */, 
                          void   * aDest, 
                          SInt     /* aDestRemain */ )
{
/***********************************************************************
 *
 * Description :
 *      UTF16BE ==> ASCII
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar  * sChar;
    SInt     sRet;

    sChar = (UChar *)aDest;

    if( IDN_IS_UTF16_ASCII_PTR(aSrc) == ID_TRUE )
    {
        *sChar = *((UChar*)aSrc + 1);
        sRet = 1;
    }
    else
    {
        sRet = RET_ILUNI;
    }

    return sRet;
}

SInt copyAscii( void    * aSrc, 
                SInt      /* aSrcRemain */, 
                void    * aDest, 
                SInt      /* aDestRemain */ )
{
/***********************************************************************
 *
 * Description :
 *      ASCII 값을 그대로 이용할 수 있는 경우에 사용하는 함수이다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar  * sSrcCharPtr;
    UChar  * sDestCharPtr;
    SInt     sRet;

    sSrcCharPtr = (UChar*)aSrc;
    sDestCharPtr = (UChar*)aDest;

    if ( *sSrcCharPtr < 0x80 )
    {
        *sDestCharPtr = *sSrcCharPtr;
        sRet = 1;
    }
    else
    {
        sRet = RET_ILSEQ;
    }

    return sRet;
}
 
