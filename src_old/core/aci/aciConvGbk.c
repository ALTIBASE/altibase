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
 * $Id: aciConvGbk.c 571 2014-07-17 04:54:19Z myungsub.shin $
 **********************************************************************/

#include <aciErrorMgr.h>
#include <aciConvGbk.h>
#include <aciConvGb2312.h>
#include <aciConvCp936ext.h>
#include <aciConvGbkext1.h>
#include <aciConvGbkext2.h>
#include <aciConvGbkextinv.h>

ACP_EXPORT acp_sint32_t aciConvConvertMbToWc4Gbk( void         * aSrc,
                                                  acp_sint32_t   aSrcRemain,
                                                  acp_sint32_t * aSrcAdvance,
                                                  void         * aDest,
                                                  acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-2414 [기능성] GBK, CP936 character set 추가
 *     GBK ==> UTF16BE
 *
 * Implementation :
 *     1) 서브셋인 GB2312 와 상이한 문자를 먼저 변환.
 *     2) 그외의 문자는 서브셋인 GB2312 에 전달.
 *     3) User_Defined_Area 인 경우 CP936EXT 에 전달.
 *     4) 변환에 실패하면, GBK 의 변환방안을 적용.
 *
 ***********************************************************************/

    acp_uint8_t  * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_sint32_t   sRet;
    acp_uint16_t   sWc;

    aDestRemain = 0;

    if ( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] < 0xff ) )
    {
        if ( aSrcRemain < 2 )
        {
            *aSrcAdvance = aSrcRemain;
            sRet = ACICONV_RET_TOOFEW;
        }
        else
        {
            if ( ( sSrcCharPtr[0] >= 0xa1 ) && ( sSrcCharPtr[0] <= 0xf7 ) )
            {
                /* 1) 서브셋인 GB2312 와 상이한 문자를 먼저 변환
                 *
                 * GB2312 와 GBK 간에 동일한 문자에 상이한 유니코드 값을 지니는
                 * 문자가 있으며, 그문자를 처리하기 위한 분기문이다. 아래에 해당
                 * 하는 문자 값과 유니코드 값을 비교하였다.
                 *
                 * Code      GB2312    GBK
                 * 0xA1A4    U+30FB    U+00B7    MIDDLE DOT
                 * 0xA1AA    U+2015    U+2014    EM DASH
                 *
                 * 따라서, 아래의 예외처리는 위의 문자를 GB2312 에게 전달하기 않
                 * 고, GBK 의 유니코드 값으로 변환하는 작업이다.
                 */
                if ( sSrcCharPtr[0] == 0xa1 )
                {
                    if ( sSrcCharPtr[1] == 0xa4 )
                    {
                        /* 이 유니코드는 UTF16BE 의 값으로, 이후에 UTF16LE 이 추
                         * 가되면 값을 수정해야 한다.
                         */
                        sWc = 0x00b7;
                        ACICONV_WC_TO_UTF16BE( aDest, sWc );

                        *aSrcAdvance = 2;
                        sRet = 2;
                    }
                    else if ( sSrcCharPtr[1] == 0xaa )
                    {
                        /* 이후에 UTF16LE 이 추가되면 값을 수정해야 한다. */
                        sWc = 0x2014;
                        ACICONV_WC_TO_UTF16BE( aDest, sWc );

                        *aSrcAdvance = 2;
                        sRet = 2;
                    }
                    else
                    {
                        /* 예외 처리 범위에 포함되지 않는 문자이다. GB2312 로 처
                         * 리해야 한다. 아직, 변환되지 않은 것을 알리기 위해 sRe
                         * t 값을 설정한다.
                         */
                        *aSrcAdvance = 2;
                        sRet = ACICONV_RET_ILSEQ;
                    }
                }
                else
                {
                    /* 변환되지 않은 것을 알리기 위해 sRet 값을 설정한다. */
                    *aSrcAdvance = 2;
                    sRet = ACICONV_RET_ILSEQ;
                }

                if ( sRet != ACICONV_RET_ILSEQ )
                {
                    ACI_RAISE( NORMAL_EXIT );
                }
                else
                {
                    /* Nothing to do */
                }

                /* 2) 그외의 문자는 서브셋인 GB2312 에 전달 */
                if ( ( sSrcCharPtr[1] >= 0xa1 ) &&
                     ( sSrcCharPtr[1] < 0xff ) )
                {
                    sRet = aciConvConvertMbToWc4Gb2312( aSrc,
                                                        aSrcRemain,
                                                        aSrcAdvance,
                                                        aDest,
                                                        aDestRemain );

                    if ( sRet != ACICONV_RET_ILSEQ )
                    {
                        ACI_RAISE( NORMAL_EXIT );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    /* 3) CP936EXT 에 전달
                     *
                     * GB2312 에서 변환하지 않는 User_Defined_Area의 문자가 존재
                     * 한다. 이러한 문자는 GB2312 범위에 있지만, 변환 코드페이지
                     * ( 코드값 배열 )가 없다. 따라서, 코드페이지가 정의된 CP936
                     * EXT 에 전달한다.
                     */
                    sRet = aciConvConvertMbToWc4Cp936ext( aSrc,
                                                          aSrcRemain,
                                                          aSrcAdvance,
                                                          aDest,
                                                          aDestRemain );
                }
                else
                {
                    *aSrcAdvance = 2;
                    sRet = ACICONV_RET_ILSEQ;
                }
            }
            else
            {
                /* 4) GBK 의 변환방안을 적용 */
                if ( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] <= 0xa0 ) )
                {
                    sRet = aciConvConvertMbToWc4Gbkext1( aSrc,
                                                         aSrcRemain,
                                                         aSrcAdvance,
                                                         aDest,
                                                         aDestRemain );
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

            /* 4) GBK 의 변환방안을 적용 */
            if ( ( sSrcCharPtr[0] >= 0xa8 ) &&
                 ( sSrcCharPtr[0] <= 0xfe ) )
            {
                sRet = aciConvConvertMbToWc4Gbkext2( aSrc,
                                                     aSrcRemain,
                                                     aSrcAdvance,
                                                     aDest,
                                                     aDestRemain );
            }
            else
            {
                if ( sSrcCharPtr[0] == 0xa2 )
                {
                    if ( ( sSrcCharPtr[1] >= 0xa1 ) &&
                         ( sSrcCharPtr[1] <= 0xaa ) )
                    {
                        sWc = 0x2170 + ( sSrcCharPtr[1] - 0xa1 );
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
                else
                {
                    *aSrcAdvance = 2;
                    sRet = ACICONV_RET_ILSEQ;
                }
            }
        }
    }
    else
    {
        *aSrcAdvance = 1;
        sRet = ACICONV_RET_ILSEQ;
    }

    ACI_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}

ACP_EXPORT acp_sint32_t aciConvConvertWcToMb4Gbk( void         * aSrc,
                                                  acp_sint32_t   aSrcRemain,
                                                  acp_sint32_t * aSrcAdvance,
                                                  void         * aDest,
                                                  acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-2414 [기능성] GBK, CP936 character set 추가
 *     UTF16BE ==> GBK
 *
 * Implementation :
 *     1) GB2312 에 전달.
 *     2) GBK 의 변환방안을 적용.
 *
 ***********************************************************************/

    acp_uint8_t  * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_sint32_t   sRet;
    acp_uint16_t   sWc;

    aSrcRemain = 0;

    ACICONV_UTF16BE_TO_WC( sWc, aSrc );

    if ( aDestRemain < 2 )
    {
        *aSrcAdvance = 2;
        sRet = ACICONV_RET_TOOSMALL;
    }
    else
    {
        /* 1) GB2312 에 전달 */
        if ( ( sWc != 0x30fb ) && ( sWc != 0x2015 ) )
        {
            sRet = aciConvConvertWcToMb4Gb2312( aSrc,
                                                aSrcRemain,
                                                aSrcAdvance,
                                                aDest,
                                                aDestRemain );
        }
        else
        {
            /* 변환되지 않은 것을 알리기 위해 sRet 값을 설정한다. */
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_ILUNI;
        }

        if ( sRet != ACICONV_RET_ILUNI )
        {
            ACI_RAISE( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }

        /* 2) GBK 의 변환방안을 적용 */
        sRet = aciConvConvertWcToMb4Gbkextinv(aSrc,
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

        if ( ( sWc >= 0x2170 ) && ( sWc <= 0x2179 ) )
        {
            sDestCharPtr[0] = 0xa2;
            sDestCharPtr[1] = 0xa1 + ( sWc - 0x2170 );

            *aSrcAdvance = 2;
            sRet = 2;
        }
        else
        {
            sRet = aciConvConvertWcToMb4Cp936ext( aSrc,
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

            /* 서브셋인 GB2312 와 상이한 문자의 예외처리이다. */
            if ( sWc == 0x00b7 )
            {
                sDestCharPtr[0] = 0xa1;
                sDestCharPtr[1] = 0xa4;

                *aSrcAdvance = 2;
                sRet = 2;
            }
            else
            {
                if ( sWc == 0x2014 )
                {
                    sDestCharPtr[0] = 0xa1;
                    sDestCharPtr[1] = 0xaa;

                    *aSrcAdvance = 2;
                    sRet = 2;
                }
                else
                {
                    *aSrcAdvance = 2;
                    sRet = ACICONV_RET_ILUNI;
                }
            }
        }
    }

    ACI_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}
/* PROJ-2414 [기능성] GBK, CP936 character set 추가
 *
 *  - GB18030 이 추가되었을 때에 사용할 수 있는 복사 함수이다.
 *  - 현재에 사용하지 않으므로 주석처리를 한다. ( 2014-07-15 )
 *
 * ACP_EXPORT acp_sint32_t aciConvCopyGbk( void         * aSrc,
 *                                         acp_sint32_t   aSrcRemain,
 *                                         acp_sint32_t * aSrcAdvance,
 *                                         void         * aDest,
 *                                         acp_sint32_t   aDestRemain )
 * {
 */
/***********************************************************************
 *
 * Description :
 *     PROJ-2414 [기능성] GBK, CP936 character set 추가
 *     ex) GBK ==> CP936
 *             ==> GB18030
 *
 * Implementation :
 *     기존의 캐릭터 셋 복사함수와 설계 및 구현이 동일하다.
 *
 ***********************************************************************/
/*
 *     acp_uint8_t  * sSrcCharPtr;
 *     acp_uint8_t  * sDestCharPtr;
 *     acp_sint32_t   sRet;
 * 
 *     sSrcCharPtr = (acp_uint8_t *)aSrc;
 *     sDestCharPtr = (acp_uint8_t *)aDest;
 * 
 *     if ( sSrcCharPtr[0] < 0x80 )
 *     {
 *         *sDestCharPtr = *sSrcCharPtr;
 * 
 *         *aSrcAdvance = 1;
 *         sRet = 1;
 *     }
 *     else
 *     {
 *         if( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] < 0xff ) )
 *         {
 *             if ( aSrcRemain < 2 )
 *             {
 *                 *aSrcAdvance = aSrcRemain;
 *                 sRet = ACICONV_RET_TOOFEW;
 *             }
 *             else
 *             {
 *                 if ( aDestRemain < 2 )
 *                 {
 *                     *aSrcAdvance = 2;
 *                     sRet = ACICONV_RET_TOOSMALL;
 *                 }
 *                 else
 *                 {
 *                     sDestCharPtr[0] = sSrcCharPtr[0];
 *                     sDestCharPtr[1] = sSrcCharPtr[1];
 * 
 *                     *aSrcAdvance = 2;
 *                     sRet = 2;
 *                 }
 *             }
 *         }
 *         else
 *         {
 *             *aSrcAdvance = 1;
 *             sRet = ACICONV_RET_ILSEQ;
 *         }
 *     }
 * 
 *     return sRet;
 * }
 */
