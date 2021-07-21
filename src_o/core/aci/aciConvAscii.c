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
 * $Id: aciConvAscii.c 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/


#include <aciConvAscii.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Ascii( void   * aSrc, 
                                                    acp_sint32_t     aSrcRemain,
                                                    acp_sint32_t   * aSrcAdvance, 
                                                    void   * aDest, 
                                                    acp_sint32_t     aDestRemain)
{
/***********************************************************************
 *
 * Description :
 *      ASCII ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   sChar;
    acp_sint32_t    sRet;

    aSrcRemain = 0;
    aDestRemain = 0;

    sChar = *(acp_uint8_t*) aSrc;

    if( sChar < 0x80 )
    {
        ((acp_uint8_t*)aDest)[0] = 0;
        ((acp_uint8_t*)aDest)[1] = sChar;
        sRet = 2;
    }
    else
    {
        sRet = ACICONV_RET_ILSEQ;
    }

    *aSrcAdvance = 1;
    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Ascii( void   * aSrc, 
                                                    acp_sint32_t     aSrcRemain,
                                                    acp_sint32_t   * aSrcAdvance, 
                                                    void   * aDest, 
                                                    acp_sint32_t     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      UTF16BE ==> ASCII
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t  * sChar = (acp_uint8_t *)aDest;
    acp_sint32_t     sRet;

    aDestRemain = aSrcRemain = 0;
    
    if( ACICONV_IS_UTF16_ASCII_PTR(aSrc) == ACP_TRUE )
    {
        *sChar = *((acp_uint8_t*)aSrc + 1);
        sRet = 1;
    }
    else
    {
        sRet = ACICONV_RET_ILUNI;
    }

    *aSrcAdvance = 2;
    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvCopyAscii( void    * aSrc, 
                                          acp_sint32_t      aSrcRemain,
                                          acp_sint32_t    * aSrcAdvance, 
                                          void    * aDest, 
                                          acp_sint32_t      aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      ASCII 값을 그대로 이용할 수 있는 경우에 사용하는 함수이다.
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t  * sSrcCharPtr = (acp_uint8_t*)aSrc;
    acp_uint8_t  * sDestCharPtr = (acp_uint8_t*)aDest;
    acp_sint32_t     sRet;

    aDestRemain = aSrcRemain = 0;

    if ( *sSrcCharPtr < 0x80 )
    {
        *sDestCharPtr = *sSrcCharPtr;
        sRet = 1;
    }
    else
    {
        sRet = ACICONV_RET_ILSEQ;
    }

    *aSrcAdvance = 1;
    return sRet;
}
 
