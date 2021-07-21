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
 * $Id: aciConvEuckr.c 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/

#include <aciConvEuckr.h>
#include <aciConvAscii.h>
#include <aciConvKsc5601.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Euckr( void   * aSrc,
                                                    acp_sint32_t     aSrcRemain,
                                                    acp_sint32_t   * aSrcAdvance,
                                                    void   * aDest,
                                                    acp_sint32_t     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      EUCKR ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   * sCharPtr = (acp_uint8_t*)aSrc;
    acp_uint8_t     sBuf[2];
    acp_sint32_t      sRet;

    /* Code set 0 (ASCII or KS C 5636-1993) */
    if( sCharPtr[0] < 0x80 )
    {
        sRet = aciConvConvertMbToWc4Ascii(aSrc, aSrcRemain, aSrcAdvance, aDest,
                                          aDestRemain);
    }
    else
    {
        /* Code set 1 (KS C 5601-1992, now KS X 1001:2002) */
        if( (sCharPtr[0] >= 0xa1) && (sCharPtr[0] < 0xff) )
        {
            if (aSrcRemain < 2)
            {
                sRet = ACICONV_RET_TOOFEW;
                *aSrcAdvance = aSrcRemain;
            }
            else
            {
                if( (sCharPtr[1] >= 0xa1) && (sCharPtr[1] < 0xff) )
                {
                    sBuf[0] = sCharPtr[0]-0x80;
                    sBuf[1] = sCharPtr[1]-0x80;

                    sRet = aciConvConvertMbToWc4Ksc5601(sBuf, 2, aSrcAdvance,
                                                        aDest, aDestRemain);
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

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Euckr( void   * aSrc,
                                                    acp_sint32_t     aSrcRemain,
                                                    acp_sint32_t   * aSrcAdvance,
                                                    void   * aDest,
                                                    acp_sint32_t     aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      UTF16BE ==> EUCKR
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   sBuf[2];
    acp_sint32_t    sRet;
    acp_uint8_t * sDestCharPtr = (acp_uint8_t *)aDest;

    /* Code set 0 (ASCII or KS C 5636-1993) */
    sRet = aciConvConvertWcToMb4Ascii( aSrc, aSrcRemain, aSrcAdvance, aDest,
                                       aDestRemain );

    if (sRet != ACICONV_RET_ILUNI)
    {
        /* Nothing to do */
    }
    else
    {
        /* Code set 1 (KS C 5601-1992, now KS X 1001:2002) */
        sRet = aciConvConvertWcToMb4Ksc5601( aSrc, aSrcRemain, aSrcAdvance,
                                             sBuf, aDestRemain );

        if (sRet != ACICONV_RET_ILUNI)
        {
            if (aDestRemain < 2)
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
            sRet = ACICONV_RET_ILUNI;
            *aSrcAdvance = 2;
        }
    }

    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvCopyEuckr( void    * aSrc,
                                          acp_sint32_t      aSrcRemain,
                                          acp_sint32_t    * aSrcAdvance,
                                          void    * aDest,
                                          acp_sint32_t      aDestRemain )
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

    acp_uint8_t   * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_uint8_t   * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_sint32_t      sRet;

    aDestRemain = 0;

    /* Code set 0 (ASCII or KS C 5636-1993) */
    if ( sSrcCharPtr[0] < 0x80)
    {
        *sDestCharPtr = *sSrcCharPtr;
        sRet = 1;
        *aSrcAdvance = 1;
    }
    else
    {
        /* Code set 1 (KS C 5601-1992, now KS X 1001:2002) */
        if( (sSrcCharPtr[0] >= 0xa1) &&
            (sSrcCharPtr[0] < 0xff) )
        {
            if (aSrcRemain < 2)
            {
                *aSrcAdvance = aSrcRemain;
                return ACICONV_RET_TOOFEW;
            }
            else
            {
                if ( (sSrcCharPtr[1] >= 0xa1) &&
                     (sSrcCharPtr[1] < 0xff) )
                {
                    sDestCharPtr[0] = sSrcCharPtr[0];
                    sDestCharPtr[1] = sSrcCharPtr[1];

                    *aSrcAdvance = 2;
                    return 2;
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
 
