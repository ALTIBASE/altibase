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
 * $Id: aciConvSjis.c 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/

#include <aciConvSjis.h>
#include <aciConvJisx0201.h>
#include <aciConvJisx0208.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Sjis( void    * aSrc,
                                                   acp_sint32_t      aSrcRemain,
                                                   acp_sint32_t    * aSrcAdvance,
                                                   void    * aDest,
                                                   acp_sint32_t      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      SHIFTJIS ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_sint32_t      sRet;
    acp_uint8_t     sBuf[2];
    acp_uint8_t     t1;
    acp_uint8_t     t2;
    acp_uint16_t    wc;
    
    if( (sSrcCharPtr[0] < 0x80) || 
        ((sSrcCharPtr[0] >= 0xa1) && (sSrcCharPtr[0] <= 0xdf)) )
    {
        sRet = aciConvConvertMbToWc4Jisx0201(aSrc, aSrcRemain, aSrcAdvance,
                                             aDest, aDestRemain);
    }
    else
    {
        if( (sSrcCharPtr[0] >= 0x81 && sSrcCharPtr[0] <= 0x9f) || 
            (sSrcCharPtr[0] >= 0xe0 && sSrcCharPtr[0] <= 0xea) )
        {
            if (aSrcRemain < 2)
            {
                sRet = ACICONV_RET_TOOFEW;
                *aSrcAdvance = aSrcRemain;
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

                    sRet = aciConvConvertMbToWc4Jisx0208(sBuf, 2, aSrcAdvance, aDest, aDestRemain);
                }
                else
                {
                    sRet = ACICONV_RET_ILSEQ;
                    *aSrcAdvance = 2;
                }
            }
        }
        else if (sSrcCharPtr[0] >= 0xf0 && sSrcCharPtr[0] <= 0xf9) 
        {
            /* User-defined range. See
            * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
            if (aSrcRemain < 2)
            {
                sRet = ACICONV_RET_TOOFEW;
                *aSrcAdvance = aSrcRemain;
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

                    ACICONV_WC_TO_UTF16BE( aDest, wc );

                    sRet = 2;
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
            *aSrcAdvance = 1;
        }
    }

    return sRet;
}


ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Sjis( void    * aSrc,
                                                   acp_sint32_t      aSrcRemain,
                                                   acp_sint32_t    * aSrcAdvance,
                                                   void    * aDest,
                                                   acp_sint32_t      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      SHIFTJIS <== UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   sBuf[2];
    acp_sint32_t    sRet = 0;
    acp_uint8_t * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_uint8_t   t1;
    acp_uint8_t   t2;
    acp_uint16_t  wc;
    
    ACICONV_UTF16BE_TO_WC( wc, aSrc );
    
    /* Try JIS X 0201-1976. */
    sRet = aciConvConvertWcToMb4Jisx0201( aSrc, aSrcRemain, aSrcAdvance, sBuf, 1 );

    if (sRet != ACICONV_RET_ILUNI)
    {
        if( sBuf[0] < 0x80 || 
            (sBuf[0] >= 0xa1 && sBuf[0] <= 0xdf) )
        {
            sDestCharPtr[0] = sBuf[0];
            *aSrcAdvance = 2;
            return 1;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* Try JIS X 0208-1990. */
    sRet = aciConvConvertWcToMb4Jisx0208( aSrc, aSrcRemain, aSrcAdvance, sBuf, 2 );

    if (sRet != ACICONV_RET_ILUNI) 
    {
        if (aDestRemain < 2)
        {
            *aSrcAdvance = 2;
            return ACICONV_RET_TOOSMALL;
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

                *aSrcAdvance = 2;
                return 2;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* User-defined range. See
    * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
    if ( (wc >= 0xe000) && (wc < 0xe758) )
    {
        if (aDestRemain < 2)
        {
            *aSrcAdvance = 2;
            return ACICONV_RET_TOOSMALL;
        }
        else
        {
            sBuf[0] = (acp_uint32_t) (wc - 0xe000) / 188;
            sBuf[1] = (acp_uint32_t) (wc - 0xe000) % 188;

            sDestCharPtr[0] = sBuf[0]+0xf0;
            sDestCharPtr[1] = (sBuf[1] < 0x3f ? 
                                            sBuf[1]+0x40 : sBuf[1]+0x41);

            *aSrcAdvance = 2;
            return 2;
        }
    }
    else
    {
        /* Nothing to do */
    }

    *aSrcAdvance = 2;
    return ACICONV_RET_ILUNI;
}
 
