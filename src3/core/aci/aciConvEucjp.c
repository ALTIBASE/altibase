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
 * $Id: aciConvEucjp.c 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/

#include <aciConvEucjp.h>
#include <aciConvAscii.h>
#include <aciConvJisx0201.h>
#include <aciConvJisx0208.h>
#include <aciConvJisx0212.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Eucjp( void   * aSrc,
                                                    acp_sint32_t     aSrcRemain,
                                                    acp_sint32_t   * aSrcAdvance,
                                                    void   * aDest,
                                                    acp_sint32_t     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      EUCJP ==> UTF16
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_sint32_t      sRet;
    acp_uint8_t     sBuf[2];
    acp_uint16_t    wc;
    
    /* Code set 0 (ASCII or JIS X 0201-1976 Roman) */
    if( sSrcCharPtr[0] < 0x80 )
    {
        sRet = aciConvConvertMbToWc4Ascii(aSrc, aSrcRemain, aSrcAdvance, aDest,
                                          aDestRemain);
    }
    /* Code set 1 (JIS X 0208) */
    else if( (sSrcCharPtr[0] >= 0xa1) && (sSrcCharPtr[0] < 0xff) )
    {
        if( aSrcRemain < 2 )
        {
            sRet = ACICONV_RET_TOOFEW;
            *aSrcAdvance = aSrcRemain;
        }
        else
        {
            if( sSrcCharPtr[0] < 0xf5 )
            {
                if( (sSrcCharPtr[1] >= 0xa1) && (sSrcCharPtr[1] < 0xff) )
                {
                    sBuf[0] = sSrcCharPtr[0]-0x80;
                    sBuf[1] = sSrcCharPtr[1]-0x80;

                    sRet = aciConvConvertMbToWc4Jisx0208(sBuf, 2, aSrcAdvance,
                                                         aDest, aDestRemain);
                }
                else
                {
                    sRet = ACICONV_RET_ILSEQ;
                    *aSrcAdvance = 2;
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

                    ACICONV_WC_TO_UTF16BE( aDest, wc );
                    
                    sRet = 2;
                    *aSrcAdvance = 2;
                }
                else
                {
                    *aSrcAdvance = 2;
                    return ACICONV_RET_ILSEQ;
                }
            }
        }
    }
    /* Code set 2 (half-width katakana) */
    else if( sSrcCharPtr[0] == 0x8e )
    {
        if( aSrcRemain < 2 )
        {
            sRet = ACICONV_RET_TOOFEW;
            *aSrcAdvance = aSrcRemain;
        }
        else
        {
            if( (sSrcCharPtr[1] >= 0xa1) && (sSrcCharPtr[1] < 0xe0) )
            {
                sRet = aciConvConvertMbToWc4Jisx0201((acp_uint8_t *)aSrc+1,
                                              (acp_uint32_t)aSrcRemain-1, 
                                              aSrcAdvance,
                                              aDest, 
                                              aDestRemain);

                if( sRet == ACICONV_RET_ILSEQ )
                {
                    sRet = ACICONV_RET_ILSEQ;
                    *aSrcAdvance = 2;
                }
                else
                {
                    sRet = 2;
                    *aSrcAdvance = 2;
                }
            }
            else
            {
                sRet = ACICONV_RET_ILSEQ;
                *aSrcAdvance = 2;
            }
        }
    }
    /* Code set 3 (JIS X 0212-1990) */
    else if( sSrcCharPtr[0] == 0x8f )
    {
        if( aSrcRemain < 2 )
        {
            sRet = ACICONV_RET_TOOFEW;
            *aSrcAdvance = aSrcRemain;
        }
        else
        {
            if( sSrcCharPtr[1] >= 0xa1 && sSrcCharPtr[1] < 0xff)
            {
                if( aSrcRemain < 3 )
                {
                    sRet = ACICONV_RET_TOOFEW;
                    *aSrcAdvance = aSrcRemain;
                }
                else
                {
                    if( sSrcCharPtr[1] < 0xf5 )
                    {
                        if((sSrcCharPtr[2] >= 0xa1) && (sSrcCharPtr[2] < 0xff))
                        {
                            sBuf[0] = sSrcCharPtr[1]-0x80; 
                            sBuf[1] = sSrcCharPtr[2]-0x80;

                            sRet = aciConvConvertMbToWc4Jisx0212(sBuf, 2,
                                             aSrcAdvance, aDest, aDestRemain);

                            if( sRet == ACICONV_RET_ILSEQ )
                            {
                                *aSrcAdvance = 3;
                                return ACICONV_RET_ILSEQ;
                            }
                            else
                            {
                                sRet = 3;
                                *aSrcAdvance = 3;
                            }
                        }
                        else
                        {
                            sRet = ACICONV_RET_ILSEQ;
                            *aSrcAdvance = 3;
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

                            ACICONV_WC_TO_UTF16BE( aDest, wc );
                            
                            sRet = 3;
                            *aSrcAdvance = 3;
                        }
                        else
                        {
                            sRet = ACICONV_RET_ILSEQ;
                            *aSrcAdvance = 3;
                        }
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

    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Eucjp( void   * aSrc,
                                                    acp_sint32_t     aSrcRemain,
                                                    acp_sint32_t   * aSrcAdvance,
                                                    void   * aDest,
                                                    acp_sint32_t     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      UTF16 ==> EUCJP
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t     sBuf[2];
    acp_sint32_t      sRet;
    acp_uint16_t    wc;
    acp_uint8_t   * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_uint8_t     c1;
    acp_uint8_t     c2;

    /* Code set 0 (ASCII or JIS X 0201-1976 Roman) */
    sRet = aciConvConvertWcToMb4Ascii( aSrc, aSrcRemain, aSrcAdvance, aDest,
                                       aDestRemain );

    if( sRet != ACICONV_RET_ILUNI )
    {
        /* Nothing to do */
    }
    else
    {
        /* Code set 1 (JIS X 0208) */
        sRet = aciConvConvertWcToMb4Jisx0208( aSrc, aSrcRemain, aSrcAdvance,
                                              sBuf, 2 );

        if( sRet != ACICONV_RET_ILUNI )
        {
            if( aDestRemain < 2 )
            {
                sRet = ACICONV_RET_TOOSMALL;
                *aSrcAdvance = 2;
            }
            else
            {
                sDestCharPtr[0] = sBuf[0]+0x80;
                sDestCharPtr[1] = sBuf[1]+0x80;

                sRet = 2;
                *aSrcAdvance = 2;
            }
        }
        else
        {
            /* Code set 2 (half-width katakana) */
            sRet = aciConvConvertWcToMb4Jisx0201( aSrc, aSrcRemain, aSrcAdvance,
                                                  sBuf, 2 );

            if( (sRet != ACICONV_RET_ILUNI) && (sBuf[0] >= 0x80) )
            {
                if( aDestRemain < 2 )
                {
                    sRet = ACICONV_RET_TOOSMALL;
                    *aSrcAdvance = 2;
                }
                else
                {
                    sDestCharPtr[0] = 0x8e;
                    sDestCharPtr[1] = sBuf[0];

                    sRet = 2;
                    *aSrcAdvance = 2;
                }
            }
            else
            {
                /* Code set 3 (JIS X 0212-1990) */
                sRet = aciConvConvertWcToMb4Jisx0212( aSrc, aSrcRemain,
                                                      aSrcAdvance, sBuf, 2 );

                if (sRet != ACICONV_RET_ILUNI)
                {
                    if( aDestRemain < 3 )
                    {
                        sRet = ACICONV_RET_TOOSMALL;
                        *aSrcAdvance = 2;
                    }
                    else
                    {
                        sDestCharPtr[0] = 0x8f;
                        sDestCharPtr[1] = sBuf[0]+0x80;
                        sDestCharPtr[2] = sBuf[1]+0x80;

                        sRet = 3;
                        *aSrcAdvance = 2;
                    }
                }
                else
                {
                    ACICONV_UTF16BE_TO_WC( wc, aSrc );
                    
                    /* Extra compatibility with Shift_JIS.  */
                    if( wc == 0x00a5 )
                    {
                        sDestCharPtr[0] = 0x5c;
                        sRet = 1;
                        *aSrcAdvance = 2;
                    }
                    /* Extra compatibility with Shift_JIS.  */
                    else if( wc == 0x203e )
                    {
                        sDestCharPtr[0] = 0x7e;
                        sRet = 1;
                        *aSrcAdvance = 2;
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
                                    sRet = ACICONV_RET_TOOSMALL;
                                    *aSrcAdvance = 2;
                                }
                                else
                                {
                                    c1 = (acp_uint32_t) (wc - 0xe000) / 94;
                                    c2 = (acp_uint32_t) (wc - 0xe000) % 94;

                                    sDestCharPtr[0] = c1+0xf5;
                                    sDestCharPtr[1] = c2+0xa1;

                                    sRet = 2;
                                    *aSrcAdvance = 2;
                                }
                            }
                            else
                            {
                                if( aDestRemain < 3 )
                                {
                                    sRet = ACICONV_RET_TOOSMALL;
                                    *aSrcAdvance = 2;
                                }
                                else
                                {
                                    c1 = (acp_uint32_t) (wc - 0xe3ac) / 94;
                                    c2 = (acp_uint32_t) (wc - 0xe3ac) % 94;

                                    sDestCharPtr[0] = 0x8f;
                                    sDestCharPtr[1] = c1+0xf5;
                                    sDestCharPtr[2] = c2+0xa1;

                                    sRet = 3;
                                    *aSrcAdvance = 2;
                                }
                            }
                        }
                        else
                        {
                            sRet = ACICONV_RET_ILUNI;
                            *aSrcAdvance = 2;
                        }
                    }
                }
            }
        }
    }

    return sRet;
}
 
