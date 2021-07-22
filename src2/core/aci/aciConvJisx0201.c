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
 * $Id: aciConvJisx0201.c 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/

#include <aciConvJisx0201.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Jisx0201( void   * aSrc,
                                                       acp_sint32_t     aSrcRemain,
                                                       acp_sint32_t   * aSrcAdvance,
                                                       void   * aDest,
                                                       acp_sint32_t     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      (EUCJP, SHIFT-JIS) ==> UTF16BE   
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_sint32_t      sRet;
    acp_uint16_t    wc;

    aSrcRemain = aDestRemain = 0;
    
    if( sSrcCharPtr[0] < 0x80 )
    {
        if( sSrcCharPtr[0] == 0x7e )
        {
            wc = (acp_uint16_t) 0x203e;
        }
        else
        {
            wc = (acp_uint16_t) sSrcCharPtr[0];
        }

        ACICONV_WC_TO_UTF16BE( aDest, wc );
                
        sRet = 2;
        *aSrcAdvance = 1;
    }
    else
    {
        if( (sSrcCharPtr[0] >= 0xa1) && (sSrcCharPtr[0] < 0xe0) )
        {
            wc = (acp_uint16_t)sSrcCharPtr[0] + 0xfec0;

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

    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Jisx0201( void   * aSrc,
                                                       acp_sint32_t     aSrcRemain,
                                                       acp_sint32_t   * aSrcAdvance,
                                                       void   * aDest,
                                                       acp_sint32_t     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      (EUCJP, SHIFT-JIS) <== UTF16   
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint16_t    wc;
    acp_uint8_t   * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_sint32_t      sRet;

    aSrcRemain = aDestRemain = 0;
    
    ACICONV_UTF16BE_TO_WC( wc, aSrc );
    
    if( (wc < 0x0080) && 
        (wc != 0x007e) )
    {
        sDestCharPtr[0] = (acp_uint8_t)wc;

        sRet = 1;
        *aSrcAdvance = 2;
    }
    else
    {
        if( wc == 0x00a5 )
        {
            sDestCharPtr[0] = 0x5c;

            sRet = 1;
            *aSrcAdvance = 2;
        }
        else if( wc == 0x203e )
        {
            sDestCharPtr[0] = 0x7e;

            sRet = 1;
            *aSrcAdvance = 2;
        }
        else if( (wc >= 0xff61) && (wc < 0xffa0) )
        {
            sDestCharPtr[0] = (acp_uint8_t)(wc - 0xfec0);

            sRet = 1;
            *aSrcAdvance = 2;
        }
        else
        {
            sRet = ACICONV_RET_ILUNI;
            *aSrcAdvance = 2;
        }
    }

    return sRet;
}
 
