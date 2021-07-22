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

#include <ide.h>
#include <idnCp949.h>
#include <idnKsc5601.h>
#include <idnAscii.h>
#include <idnUhc1.h>
#include <idnUhc2.h>

SInt convertMbToWc4Cp949( void    * aSrc,
                          SInt      aSrcRemain,
                          void    * aDest,
                          SInt      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      CP949 ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   * sSrcCharPtr;
    SInt      sRet;
    UChar     sBuf[2];
    UShort    wc;
    
    sSrcCharPtr = (UChar *)aSrc;

    /* Code set 0 (ASCII) */
    if (sSrcCharPtr[0] < 0x80)
    {
        sRet = convertMbToWc4Ascii(aSrc, aSrcRemain, aDest, aDestRemain);
    }
    else
    {
        /* UHC part 1 */
        if (sSrcCharPtr[0] >= 0x81 && sSrcCharPtr[0] <= 0xa0)
        {
            sRet = convertMbToWc4Uhc1(aSrc, aSrcRemain, aDest, aDestRemain);
        }
        else
        {
            if (sSrcCharPtr[0] >= 0xa1 && sSrcCharPtr[0] < 0xff)
            {
                if (aSrcRemain < 2)
                {
                    sRet = RET_TOOFEW;
                }
                else
                {
                    if (sSrcCharPtr[1] < 0xa1)
                    {
                        /* UHC part 2 */
                        sRet = convertMbToWc4Uhc2(aSrc, aSrcRemain, 
                                                  aDest, aDestRemain);
                    }
                    else if((sSrcCharPtr[1] < 0xff) && 
                            (sSrcCharPtr[0] != 0xa2 || sSrcCharPtr[1] != 0xe8))
                    {
                        /* Code set 1 (KS C 5601-1992, now KS X 1001:1998) */
                        sBuf[0] = sSrcCharPtr[0]-0x80; 
                        sBuf[1] = sSrcCharPtr[1]-0x80;

                        sRet = convertMbToWc4Ksc5601(sBuf, 2, 
                                                     aDest, aDestRemain);
                        if (sRet != RET_ILSEQ)
                        {
                            // Nothing to do
                        }
                        else
                        {
                            /* User-defined characters */
                            if (sSrcCharPtr[0] == 0xc9)
                            {
                                wc = 0xe000 + 
                                    (sSrcCharPtr[1] - 0xa1);

                                WC_TO_UTF16BE(aDest, wc);
                                
                                sRet = 1;
                            }
                            else
                            {
                                sRet = RET_ILSEQ;
                            }

                            if (sSrcCharPtr[0] == 0xfe)
                            {
                                wc = 0xe05e + 
                                    (sSrcCharPtr[1] - 0xa1);

                                WC_TO_UTF16BE(aDest, wc);
                                
                                sRet = 1;
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
            }
            else
            {
                sRet = RET_ILSEQ;
            }
        }
    }

    return sRet;
}

SInt convertWcToMb4Cp949( void    * aSrc,
                          SInt      aSrcRemain,
                          void    * aDest,
                          SInt      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      CP949 <== UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   sBuf[2];
    SInt    sRet;
    UChar * sDestCharPtr;
    UShort  wc;

    sDestCharPtr = (UChar *)aDest;

    /* Code set 0 (ASCII) */
    sRet = convertWcToMb4Ascii( aSrc, aSrcRemain, aDest, aDestRemain );

    if (sRet != RET_ILUNI)
    {
        // Nothing to do
    }
    else
    {
        UTF16BE_TO_WC( wc, aSrc );
        
        /* Code set 1 (KS C 5601-1992, now KS X 1001:1998) */
        if (wc != 0x327e)
        {
            sRet = convertWcToMb4Ksc5601( aSrc, aSrcRemain, sBuf, 2 );

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

                // fix BUG-25959 UTF8에 MS949로 변환이 제대로 되지 않습니다.
                IDE_CONT(RET_CONV);
            }
            else
            {
                sRet = RET_ILUNI;
            }
        }
        /* UHC */
        if (wc >= 0xac00 && wc < 0xd7a4)
        {
            if (wc < 0xc8a5)
            {
                sRet = convertWcToMb4Uhc1(aSrc, aSrcRemain, aDest, aDestRemain);
            }
            else
            {
                sRet = convertWcToMb4Uhc2(aSrc, aSrcRemain, aDest, aDestRemain);
            }

            // fix BUG-25959 UTF8에 MS949로 변환이 제대로 되지 않습니다.
            IDE_CONT(RET_CONV);
        }
        /* User-defined characters */
        if (wc >= 0xe000 && wc < 0xe0bc)
        {
            if (aDestRemain < 2)
            {
                sRet = RET_TOOSMALL;
            }
            else
            {
                if (wc < 0xe05e)
                {
                    sDestCharPtr[0] = 0xc9;
                    sDestCharPtr[1] = wc - 0xe000 + 0xa1;
                }
                else
                {
                    sDestCharPtr[0] = 0xfe;
                    sDestCharPtr[1] = wc - 0xe05e + 0xa1;
                }

                sRet = 2;
            }
        }
        else
        {
            sRet = RET_ILUNI;
        }
    }

    // fix BUG-25959 UTF8에 MS949로 변환이 제대로 되지 않습니다.
    IDE_EXCEPTION_CONT( RET_CONV );

    return sRet;
}

 
