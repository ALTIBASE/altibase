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
 * $Id: aciConvUtf8.c 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/

/* Specification: RFC 3629 */

#include <aciConvUtf8.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Utf8( void    * aSrc,
                                                   acp_sint32_t      aSrcRemain,
                                                   acp_sint32_t    * aSrcAdvance,
                                                   void    * aDest,
                                                   acp_sint32_t      aDestRemain )
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

    acp_uint8_t   * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_sint32_t      sRet;
    acp_uint16_t    wc;

    aDestRemain = 0;
    
    if (sSrcCharPtr[0] < 0x80)
    {
        wc = sSrcCharPtr[0];

        ACICONV_WC_TO_UTF16BE( aDest, wc );
        
        sRet = 2;
        *aSrcAdvance = 1;
    }
    else if (sSrcCharPtr[0] < 0xc2)
    {
        sRet = ACICONV_RET_ILSEQ;
        *aSrcAdvance = 1;
    }
    else if (sSrcCharPtr[0] < 0xe0)
    {
        if (aSrcRemain < 2)
        {    
            sRet = ACICONV_RET_TOOFEW;
            *aSrcAdvance = aSrcRemain;
        }
        else
        {
            if( ((sSrcCharPtr[1] ^ 0x80) >= 0x40) )
            {
                sRet = ACICONV_RET_ILSEQ;
                *aSrcAdvance = 2;
            }
            else
            {
                wc = ((acp_uint16_t) (sSrcCharPtr[0] & 0x1f) << 6)
                    | (acp_uint16_t) (sSrcCharPtr[1] ^ 0x80);

                ACICONV_WC_TO_UTF16BE( aDest, wc );
                
                sRet = 2;
                *aSrcAdvance = 2;
            }
        }
    }
    else if (sSrcCharPtr[0] < 0xf0) 
    {
        if (aSrcRemain < 3)
        {
            sRet = ACICONV_RET_TOOFEW;
            *aSrcAdvance = aSrcRemain;
        }
        else
        {
            if( !( (sSrcCharPtr[1] ^ 0x80) < 0x40 && 
                   (sSrcCharPtr[2] ^ 0x80) < 0x40 && 
                   (sSrcCharPtr[0] >= 0xe1 || sSrcCharPtr[1] >= 0xa0) ) )
            {
                sRet = ACICONV_RET_ILSEQ;
                *aSrcAdvance = 3;
            }
            else
            {
                wc = ((acp_uint16_t) (sSrcCharPtr[0] & 0x0f) << 12)
                    | ((acp_uint16_t) (sSrcCharPtr[1] ^ 0x80) << 6)
                    | (acp_uint16_t) (sSrcCharPtr[2] ^ 0x80);

                ACICONV_WC_TO_UTF16BE( aDest, wc );

                sRet = 2;
                *aSrcAdvance = 3;
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

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Utf8( void    * aSrc,
                                                   acp_sint32_t      aSrcRemain,
                                                   acp_sint32_t    * aSrcAdvance,
                                                   void    * aDest,
                                                   acp_sint32_t      aDestRemain )
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

    acp_sint32_t                sCount;
    acp_uint8_t             * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_uint16_t              wc;

    aSrcRemain = 0;
    
    ACICONV_UTF16BE_TO_WC( wc, aSrc );
    
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
        *aSrcAdvance = 2;
        return ACICONV_RET_TOOSMALL;
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

        *aSrcAdvance = 2;
        return sCount;
    }
}
 
