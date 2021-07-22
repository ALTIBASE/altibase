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
 * $Id: aciConvCp936.c 563 2014-06-30 00:49:01Z myungsub.shin $
 **********************************************************************/

#include <aciErrorMgr.h>
#include <aciConvAscii.h>
#include <aciConvCp936.h>
#include <aciConvGbk.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Cp936( void         * aSrc,
                                                    acp_sint32_t   aSrcRemain,
                                                    acp_sint32_t * aSrcAdvance,
                                                    void         * aDest,
                                                    acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-2414 [기능성] GBK, CP936 character set 추가
 *     CP936 ==> UTF16BE
 *
 * Implementation :
 *     1) 1바이트 인코딩은 ASCII 에 전달.
 *     2) 2바이트 인코딩은 두 단계로 처리.
 *        2-1) 서브셋인 GBK 에 전달.
 *        2-2) GBK 의 범위을 넘는 문자라면, MS936 의 변환방안을 적용.
 *
 ***********************************************************************/

    acp_uint8_t  * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_sint32_t   sRet;
    acp_uint16_t   sWc;

    aDestRemain = 0;

    /* 1) ASCII 에 전달 */
    if ( sSrcCharPtr[0] < 0x80 )
    {
        sRet = aciConvConvertMbToWc4Ascii( aSrc,
                                           aSrcRemain,
                                           aSrcAdvance,
                                           aDest,
                                           aDestRemain );
    }
    else
    {
        /* 2-1) GBK 에 전달 */
        if ( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] < 0xff ) )
        {
            if ( aSrcRemain < 2 )
            {
                *aSrcAdvance = aSrcRemain;
                sRet = ACICONV_RET_TOOFEW;
            }
            else
            {
                sRet = aciConvConvertMbToWc4Gbk( aSrc,
                                                 aSrcRemain,
                                                 aSrcAdvance,
                                                 aDest,
                                                 aDestRemain );
            }
        }
        else
        {
            /* 2-2) MS936 의 변환방안을 적용 */
            if ( sSrcCharPtr[0] == 0x80 )
            {
                /* MS936 부터 0x80 에 EURO SIGN 을 추가하였다.
                 *
                 * 0x20ac 는 UTF16BE 의 EURO SIGN 값이다. 이후에 UTF16LE 이 추가
                 * 되면 값을 수정해야 한다.
                 */
                sWc = 0x20ac;
                ACICONV_WC_TO_UTF16BE( aDest, sWc );

                *aSrcAdvance = 1;
                sRet = 2;
            }
            else
            {
                *aSrcAdvance = 2;
                sRet = ACICONV_RET_ILSEQ;
            }
        }

        if ( sRet != ACICONV_RET_ILSEQ )
        {
            ACI_RAISE( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }

        /* 2-2) MS936 의 변환방안을 적용 */
        if ( ( sSrcCharPtr[0] >= 0xa1 ) && ( sSrcCharPtr[0] <= 0xa2 ) )
        {
            if ( aSrcRemain < 2 )
            {
                *aSrcAdvance = aSrcRemain;
                sRet = ACICONV_RET_TOOFEW;
            }
            else
            {
                if ( ( ( sSrcCharPtr[1] >= 0x40 ) &&
                       ( sSrcCharPtr[1] < 0x7f ) ) ||
                     ( ( sSrcCharPtr[1] >= 0x80 ) &&
                       ( sSrcCharPtr[1] < 0xa1 ) ) )
                {
                    sWc = 0xe4c6 + ( 96 * ( sSrcCharPtr[0] - 0xa1 ) );
                    sWc += sSrcCharPtr[1] -
                        ( sSrcCharPtr[1] >= 0x80 ? 0x41 : 0x40 );
                    ACICONV_WC_TO_UTF16BE( aDest, sWc );

                    *aSrcAdvance = 2;
                    sRet = 2;
                }
                else
                {
                    *aSrcAdvance = 2;
                    sRet = ACICONV_RET_ILSEQ;
                }
            }
        }
        else
        {
            if ( ( ( sSrcCharPtr[0] >= 0xaa ) && ( sSrcCharPtr[0] < 0xb0 ) ) ||
                 ( ( sSrcCharPtr[0] >= 0xf8 ) && ( sSrcCharPtr[0] < 0xff ) ) )
            {
                if ( aSrcRemain < 2 )
                {
                    *aSrcAdvance = aSrcRemain;
                    sRet = ACICONV_RET_TOOFEW;
                }
                else
                {
                    if ( ( sSrcCharPtr[1] >= 0xa1 ) &&
                         ( sSrcCharPtr[1] < 0xff ) )
                    {
                        sWc = 0xe000;
                        sWc += 94 * ( sSrcCharPtr[0] -
                                      ( sSrcCharPtr[0] >= 0xf8 ?
                                        0xf2 : 0xaa ) );
                        sWc += sSrcCharPtr[1] - 0xa1;
                        ACICONV_WC_TO_UTF16BE( aDest, sWc );

                        *aSrcAdvance = 2;
                        sRet = 2;
                    }
                    else
                    {
                        *aSrcAdvance = 2;
                        sRet = ACICONV_RET_ILSEQ;
                    }
                }
            }
            else
            {
                *aSrcAdvance = 2;
                sRet = ACICONV_RET_ILSEQ;
            }
        }
    }

    ACI_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Cp936( void         * aSrc,
                                                    acp_sint32_t   aSrcRemain,
                                                    acp_sint32_t * aSrcAdvance,
                                                    void         * aDest,
                                                    acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-2414 [기능성] GBK, CP936 character set 추가
 *     UTF16BE ==> CP936
 *
 * Implementation :
 *     1) ASCII 에 전달.
 *     2) 변환 실패 시, GBK 에 전달.
 *     3) 변환 실패 시, MS936 의 변환방안을 적용.
 *
 ***********************************************************************/

    acp_uint8_t  * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_uint32_t   sFir;
    acp_uint32_t   sSec;
    acp_sint32_t   sRet;
    acp_uint16_t   sWc;

    aSrcRemain = 0;

    /* 1) ASCII 에 전달 */
    sRet = aciConvConvertWcToMb4Ascii( aSrc,
                                       aSrcRemain,
                                       aSrcAdvance,
                                       aDest,
                                       aDestRemain );

    if ( sRet != ACICONV_RET_ILUNI )
    {
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* 2) GBK 에 전달 */
    sRet = aciConvConvertWcToMb4Gbk( aSrc,
                                     aSrcRemain,
                                     aSrcAdvance,
                                     aDest,
                                     aDestRemain );

    if ( sRet != ACICONV_RET_ILUNI )
    {
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* 3) MS936 의 변환방안을 적용 */
    ACICONV_UTF16BE_TO_WC( sWc, aSrc );

    if ( ( sWc >= 0xe000 ) && ( sWc < 0xe586 ) )
    {
        if ( aDestRemain < 2 )
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_TOOSMALL;
        }
        else
        {
            if ( sWc < 0xe4c6 )
            {
                sSec = sWc - 0xe000;
                sFir = sSec / 94;
                sSec = sSec % 94;
                sDestCharPtr[0] = sFir + ( sFir < 6 ? 0xaa : 0xf2 );
                sDestCharPtr[1] = sSec + 0xa1;
            }
            else
            {
                sSec = sWc - 0xe4c6;
                sFir = sSec / 96;
                sSec = sSec % 96;
                sDestCharPtr[0] = sFir + 0xa1;
                sDestCharPtr[1] = sSec + ( sSec < 0x3f ? 0x40 : 0x41 );
            }

            *aSrcAdvance = 2;
            sRet = 2;
        }
    }
    else
    {
        /* 0x20ac 는 UTF16BE 의 EURO SIGN 값이다. 이후에 UTF16LE이 추가되면 값을
         * 수정해야 한다.
         */
        if ( sWc == 0x20ac )
        {
            sDestCharPtr[0] = 0x80;

            *aSrcAdvance = 2;
            sRet = 1;
        }
        else
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_ILUNI;
        }
    }

    ACI_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}
