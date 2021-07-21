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

#include <idnEuckr.h>
#include <idnAscii.h>
#include <idnKsc5601.h>

SInt convertMbToWc4Euckr( void   * aSrc,
                          SInt     aSrcRemain,
                          void   * aDest,
                          SInt     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      EUCKR ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   * sCharPtr;
    UChar     sBuf[2];
    SInt      sRet;

    sCharPtr = (UChar*)aSrc;

    /* Code set 0 (ASCII or KS C 5636-1993) */
    if( sCharPtr[0] < 0x80 )
    {
        sRet = convertMbToWc4Ascii(aSrc, aSrcRemain, aDest, aDestRemain);
    }
    else
    {
        /* Code set 1 (KS C 5601-1992, now KS X 1001:2002) */
        if( (sCharPtr[0] >= 0xa1) && (sCharPtr[0] < 0xff) )
        {
            if (aSrcRemain < 2)
            {
                sRet = RET_TOOFEW;
            }
            else
            {
                if( (sCharPtr[1] >= 0xa1) && (sCharPtr[1] < 0xff) )
                {
                    sBuf[0] = sCharPtr[0]-0x80;
                    sBuf[1] = sCharPtr[1]-0x80;

                    sRet = convertMbToWc4Ksc5601(sBuf, 2, aDest, aDestRemain);
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

SInt convertWcToMb4Euckr( void   * aSrc,
                          SInt     aSrcRemain,
                          void   * aDest,
                          SInt     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      UTF16BE ==> EUCKR
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   sBuf[2];
    SInt    sRet;
    UChar * sDestCharPtr;

    sDestCharPtr = (UChar *)aDest;

    /* Code set 0 (ASCII or KS C 5636-1993) */
    sRet = convertWcToMb4Ascii( aSrc, aSrcRemain, aDest, aDestRemain );

    if (sRet != RET_ILUNI)
    {
        // Nothing to do
    }
    else
    {
        /* Code set 1 (KS C 5601-1992, now KS X 1001:2002) */
        sRet = convertWcToMb4Ksc5601( aSrc, aSrcRemain, sBuf, aDestRemain );

        if (sRet != RET_ILUNI)
        {
            if (aDestRemain < 2)
            {    
                sRet = RET_TOOSMALL;
            }
            else
            {
                sDestCharPtr[0] = sBuf[0]+0x80;
                sDestCharPtr[1] = sBuf[1]+0x80;

                sRet = 2;
            }
        }
        else
        {
            sRet = RET_ILUNI;
        }
    }

    return sRet;
}

SInt copyEuckr( void    * aSrc,
                SInt      aSrcRemain,
                void    * aDest,
                SInt      /* aDestRemain */ )
{
/***********************************************************************
 *
 * Description :
 *      EUCKR 값을 그대로 이용할 수 있는 경우에 사용하는 함수이다.
 *      ex) EUCKR ==> MS949
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   * sSrcCharPtr;
    UChar   * sDestCharPtr;
    SInt      sRet;

    sSrcCharPtr = (UChar *)aSrc;
    sDestCharPtr = (UChar *)aDest;

    /* Code set 0 (ASCII or KS C 5636-1993) */
    if ( sSrcCharPtr[0] < 0x80)
    {
        *sDestCharPtr = *sSrcCharPtr;
        sRet = 1;
    }
    else
    {
        /* Code set 1 (KS C 5601-1992, now KS X 1001:2002) */
        if( (sSrcCharPtr[0] >= 0xa1) &&
            (sSrcCharPtr[0] < 0xff) )
        {
            if (aSrcRemain < 2)
            {
                return RET_TOOFEW;
            }
            else
            {
                if ( (sSrcCharPtr[1] >= 0xa1) &&
                     (sSrcCharPtr[1] < 0xff) )
                {
                    sDestCharPtr[0] = sSrcCharPtr[0];
                    sDestCharPtr[1] = sSrcCharPtr[1];

                    return 2;
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
 
