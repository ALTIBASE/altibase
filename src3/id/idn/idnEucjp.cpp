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

#include <idnEucjp.h>
#include <idnAscii.h>
#include <idnJisx0201.h>
#include <idnJisx0208.h>
#include <idnJisx0212.h>

SInt convertMbToWc4Eucjp( void   * aSrc,
                          SInt     aSrcRemain,
                          void   * aDest,
                          SInt     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      EUCJP ==> UTF16
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   * sSrcCharPtr;
    SInt      sRet;
    UChar     sBuf[2];
    UShort    wc;
    
    sSrcCharPtr = (UChar *)aSrc;

    /* Code set 0 (ASCII or JIS X 0201-1976 Roman) */
    if( sSrcCharPtr[0] < 0x80 )
    {
        sRet = convertMbToWc4Ascii(aSrc, aSrcRemain, aDest, aDestRemain);
    }
    /* Code set 1 (JIS X 0208) */
    else if( (sSrcCharPtr[0] >= 0xa1) && (sSrcCharPtr[0] < 0xff) )
    {
        if( aSrcRemain < 2 )
        {
            sRet = RET_TOOFEW;
        }
        else
        {
            if( sSrcCharPtr[0] < 0xf5 )
            {
                if( (sSrcCharPtr[1] >= 0xa1) && (sSrcCharPtr[1] < 0xff) )
                {
                    sBuf[0] = sSrcCharPtr[0]-0x80;
                    sBuf[1] = sSrcCharPtr[1]-0x80;

                    sRet = convertMbToWc4Jisx0208(sBuf, 2, aDest, aDestRemain);
                }
                else
                {
                    sRet = RET_ILSEQ;
                }
            }
            else
            {
                /* User-defined range. See
                * Ken Lunde's "CJKV Information Processing", 
                table 4-66, p. 206. */
                if( (sSrcCharPtr[1] >= 0xa1) && (sSrcCharPtr[1] < 0xff) ) 
                {
                    wc = 0xe000 + 
                        94 * (sSrcCharPtr[0]-0xf5) + 
                        (sSrcCharPtr[1]-0xa1);

                    WC_TO_UTF16BE( aDest, wc );
                    
                    sRet = 2;
                }
                else
                {
                    return RET_ILSEQ;
                }
            }
        }
    }
    /* Code set 2 (half-width katakana) */
    else if( sSrcCharPtr[0] == 0x8e )
    {
        if( aSrcRemain < 2 )
        {
            sRet = RET_TOOFEW;
        }
        else
        {
            if( (sSrcCharPtr[1] >= 0xa1) && (sSrcCharPtr[1] < 0xe0) )
            {
                sRet = convertMbToWc4Jisx0201((UChar *)aSrc+1,
                                              (UInt)aSrcRemain-1, 
                                              aDest, 
                                              aDestRemain);

                if( sRet == RET_ILSEQ )
                {
                    sRet = RET_ILSEQ;
                }
                else
                {
                    sRet = 2;
                }
            }
            else
            {
                sRet = RET_ILSEQ;
            }
        }
    }
    /* Code set 3 (JIS X 0212-1990) */
    else if( sSrcCharPtr[0] == 0x8f )
    {
        if( aSrcRemain < 2 )
        {
            sRet = RET_TOOFEW;
        }
        else
        {
            if( sSrcCharPtr[1] >= 0xa1 && sSrcCharPtr[1] < 0xff)
            {
                if( aSrcRemain < 3 )
                {
                    sRet = RET_TOOFEW;
                }
                else
                {
                    if( sSrcCharPtr[1] < 0xf5 )
                    {
                        if((sSrcCharPtr[2] >= 0xa1) && (sSrcCharPtr[2] < 0xff))
                        {
                            sBuf[0] = sSrcCharPtr[1]-0x80; 
                            sBuf[1] = sSrcCharPtr[2]-0x80;

                            sRet = convertMbToWc4Jisx0212(sBuf, 2, 
                                                          aDest, aDestRemain);

                            if( sRet == RET_ILSEQ )
                            {
                                return RET_ILSEQ;
                            }
                            else
                            {
                                sRet = 3;
                            }
                        }
                        else
                        {
                            sRet = RET_ILSEQ;
                        }
                    }
                    else 
                    {
                        /* User-defined range. See
                        * Ken Lunde's "CJKV Information Processing", 
                        table 4-66, p. 206. */
                        if(sSrcCharPtr[2] >= 0xa1 && sSrcCharPtr[2] < 0xff)
                        {
                            wc = 0xe3ac + 
                                94 * (sSrcCharPtr[1]-0xf5) + 
                                (sSrcCharPtr[2]-0xa1);

                            WC_TO_UTF16BE( aDest, wc );
                            
                            sRet = 3;
                        }
                        else
                        {
                            sRet = RET_ILSEQ;
                        }
                    }
                }
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

    return sRet;
}

SInt convertWcToMb4Eucjp( void   * aSrc,
                          SInt     aSrcRemain,
                          void   * aDest,
                          SInt     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      UTF16 ==> EUCJP
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar     sBuf[2];
    SInt      sRet;
    UShort    wc;
    UChar   * sDestCharPtr;
    UChar     c1;
    UChar     c2;

    sDestCharPtr = (UChar *)aDest;

    /* Code set 0 (ASCII or JIS X 0201-1976 Roman) */
    sRet = convertWcToMb4Ascii( aSrc, aSrcRemain, aDest, aDestRemain );

    if( sRet != RET_ILUNI )
    {
        // Nothing to do
    }
    else
    {
        /* Code set 1 (JIS X 0208) */
        sRet = convertWcToMb4Jisx0208( aSrc, aSrcRemain, sBuf, 2 );

        if( sRet != RET_ILUNI )
        {
            if( aDestRemain < 2 )
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
            /* Code set 2 (half-width katakana) */
            sRet = convertWcToMb4Jisx0201( aSrc, aSrcRemain, sBuf, 2 );

            if( (sRet != RET_ILUNI) && (sBuf[0] >= 0x80) )
            {
                if( aDestRemain < 2 )
                {
                    sRet = RET_TOOSMALL;
                }
                else
                {
                    sDestCharPtr[0] = 0x8e;
                    sDestCharPtr[1] = sBuf[0];

                    sRet = 2;
                }
            }
            else
            {
                /* Code set 3 (JIS X 0212-1990) */
                sRet = convertWcToMb4Jisx0212( aSrc, aSrcRemain, sBuf, 2 );

                if (sRet != RET_ILUNI)
                {
                    if( aDestRemain < 3 )
                    {
                        sRet = RET_TOOSMALL;
                    }
                    else
                    {
                        sDestCharPtr[0] = 0x8f;
                        sDestCharPtr[1] = sBuf[0]+0x80;
                        sDestCharPtr[2] = sBuf[1]+0x80;

                        sRet = 3;
                    }
                }
                else
                {
                    UTF16BE_TO_WC( wc, aSrc );
                    
                    /* Extra compatibility with Shift_JIS.  */
                    if( wc == 0x00a5 )
                    {
                        sDestCharPtr[0] = 0x5c;
                        sRet = 1;
                    }
                    /* Extra compatibility with Shift_JIS.  */
                    else if( wc == 0x203e )
                    {
                        sDestCharPtr[0] = 0x7e;
                        sRet = 1;
                    }
                    /* User-defined range. See
                    * Ken Lunde's "CJKV Information Processing", 
                    table 4-66, p. 206. */
                    else
                    {
                        if( (wc >= 0xe000) && (wc < 0xe758) )
                        {
                            if(wc < 0xe3ac)
                            {
                                if( aDestRemain < 2 )
                                {
                                    sRet = RET_TOOSMALL;
                                }
                                else
                                {
                                    c1 = (UInt) (wc - 0xe000) / 94;
                                    c2 = (UInt) (wc - 0xe000) % 94;

                                    sDestCharPtr[0] = c1+0xf5;
                                    sDestCharPtr[1] = c2+0xa1;

                                    sRet = 2;
                                }
                            }
                            else
                            {
                                if( aDestRemain < 3 )
                                {
                                    sRet = RET_TOOSMALL;
                                }
                                else
                                {
                                    c1 = (UInt) (wc - 0xe3ac) / 94;
                                    c2 = (UInt) (wc - 0xe3ac) % 94;

                                    sDestCharPtr[0] = 0x8f;
                                    sDestCharPtr[1] = c1+0xf5;
                                    sDestCharPtr[2] = c2+0xa1;

                                    sRet = 3;
                                }
                            }
                        }
                        else
                        {
                            sRet = RET_ILUNI;
                        }
                    }
                }
            }
        }
    }

    return sRet;
}
 
