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
 * $Id: aciConvCp949.c 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/

#include <aciErrorMgr.h>
#include <aciConvCp949.h>
#include <aciConvKsc5601.h>
#include <aciConvAscii.h>
#include <aciConvUhc1.h>
#include <aciConvUhc2.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Cp949( void    * aSrc,
                                                    acp_sint32_t      aSrcRemain,
                                                    acp_sint32_t    * aSrcAdvance,
                                                    void    * aDest,
                                                    acp_sint32_t      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      CP949 ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_sint32_t      sRet;
    acp_uint8_t     sBuf[2];
    acp_uint16_t    wc;
    
    /* Code set 0 (ASCII) */
    if (sSrcCharPtr[0] < 0x80)
    {
        sRet = aciConvConvertMbToWc4Ascii(aSrc, aSrcRemain, aSrcAdvance, aDest,
                                          aDestRemain);
    }
    else
    {
        /* UHC part 1 */
        if (sSrcCharPtr[0] >= 0x81 && sSrcCharPtr[0] <= 0xa0)
        {
            sRet = aciConvConvertMbToWc4Uhc1(aSrc, aSrcRemain, aSrcAdvance,
                                             aDest, aDestRemain);
        }
        else
        {
            if (sSrcCharPtr[0] >= 0xa1 && sSrcCharPtr[0] < 0xff)
            {
                if (aSrcRemain < 2)
                {
                    sRet = ACICONV_RET_TOOFEW;
                    *aSrcAdvance = aSrcRemain;
                }
                else
                {
                    if (sSrcCharPtr[1] < 0xa1)
                    {
                        /* UHC part 2 */
                        sRet = aciConvConvertMbToWc4Uhc2(aSrc, aSrcRemain, 
                                              aSrcAdvance, aDest, aDestRemain);
                    }
                    else if((sSrcCharPtr[1] < 0xff) && 
                            (sSrcCharPtr[0] != 0xa2 || sSrcCharPtr[1] != 0xe8))
                    {
                        /* Code set 1 (KS C 5601-1992, now KS X 1001:1998) */
                        sBuf[0] = sSrcCharPtr[0]-0x80; 
                        sBuf[1] = sSrcCharPtr[1]-0x80;

                        sRet = aciConvConvertMbToWc4Ksc5601(sBuf, 2, aSrcAdvance,
                                                     aDest, aDestRemain);
                        if (sRet != ACICONV_RET_ILSEQ)
                        {
                            /* Nothing to do */
                        }
                        else
                        {
                            /* User-defined characters */
                            if (sSrcCharPtr[0] == 0xc9)
                            {
                                wc = 0xe000 + 
                                    (sSrcCharPtr[1] - 0xa1);

                                ACICONV_WC_TO_UTF16BE(aDest, wc);
                                
                                sRet = 1;
                                *aSrcAdvance = 2;
                            }
                            else
                            {
                                sRet = ACICONV_RET_ILSEQ;
                                *aSrcAdvance = 2;
                            }

                            if (sSrcCharPtr[0] == 0xfe)
                            {
                                wc = 0xe05e + 
                                    (sSrcCharPtr[1] - 0xa1);

                                ACICONV_WC_TO_UTF16BE(aDest, wc);
                                
                                sRet = 1;
                                *aSrcAdvance = 2;
                            }
                            else
                            {
                                sRet = ACICONV_RET_ILSEQ;
                                *aSrcAdvance = 2;
                            }
                        }
                    }
                    else
                    {
                        sRet = ACICONV_RET_ILSEQ;
                        *aSrcAdvance = 2;
                    }
                }
            }
            else
            {
                sRet = ACICONV_RET_ILSEQ;
                *aSrcAdvance = 1;
            }
        }
    }

    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Cp949( void    * aSrc,
                                                    acp_sint32_t      aSrcRemain,
                                                    acp_sint32_t    * aSrcAdvance,
                                                    void    * aDest,
                                                    acp_sint32_t      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      CP949 <== UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   sBuf[2];
    acp_sint32_t    sRet;
    acp_uint8_t * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_uint16_t  wc;

    /* Code set 0 (ASCII) */
    sRet = aciConvConvertWcToMb4Ascii( aSrc, aSrcRemain, aSrcAdvance, aDest,
                                       aDestRemain );

    if (sRet != ACICONV_RET_ILUNI)
    {
        /* Nothing to do */
    }
    else
    {
        ACICONV_UTF16BE_TO_WC( wc, aSrc );
        
        /* Code set 1 (KS C 5601-1992, now KS X 1001:1998) */
        if (wc != 0x327e)
        {
            sRet = aciConvConvertWcToMb4Ksc5601( aSrc, aSrcRemain, aSrcAdvance,
                                                 sBuf, 2 );

            if (sRet != ACICONV_RET_ILUNI)
            {
                if (aDestRemain < 2)
                {
                    sRet = ACICONV_RET_TOOSMALL;
                    *aSrcAdvance = aSrcRemain;
                }
                else
                {
                    sDestCharPtr[0] = sBuf[0]+0x80;
                    sDestCharPtr[1] = sBuf[1]+0x80;

                    sRet = 2;
                    *aSrcAdvance = 2;
                }

                /* fix BUG-25959 UTF8에 MS949로 변환이 제대로 되지 않습니다. */
                ACI_RAISE(RET_CONV);
            }
            else
            {
                sRet = ACICONV_RET_ILUNI;
                *aSrcAdvance = 2;
            }
        }
        /* UHC */
        if (wc >= 0xac00 && wc < 0xd7a4)
        {
            if (wc < 0xc8a5)
            {
                sRet = aciConvConvertWcToMb4Uhc1(aSrc, aSrcRemain, aSrcAdvance,
                                                 aDest, aDestRemain);
            }
            else
            {
                sRet = aciConvConvertWcToMb4Uhc2(aSrc, aSrcRemain, aSrcAdvance,
                                                 aDest, aDestRemain);
            }

            /* fix BUG-25959 UTF8에 MS949로 변환이 제대로 되지 않습니다. */
            ACI_RAISE(RET_CONV);
        }
        /* User-defined characters */
        if (wc >= 0xe000 && wc < 0xe0bc)
        {
            if (aDestRemain < 2)
            {
                sRet = ACICONV_RET_TOOSMALL;
                *aSrcAdvance = 2;
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
                *aSrcAdvance = 2;
            }
        }
        else
        {
            sRet = ACICONV_RET_ILUNI;
            *aSrcAdvance = 2;
        }
    }

    /* fix BUG-25959 UTF8에 MS949로 변환이 제대로 되지 않습니다. */
    ACI_EXCEPTION_CONT( RET_CONV );

    return sRet;
}

 
