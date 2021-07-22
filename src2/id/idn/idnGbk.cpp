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
 * $Id: idnGbk.cpp 66009 2014-07-16 06:55:19Z myungsub.shin $
 **********************************************************************/

#include <ide.h>
#include <idnGbk.h>
#include <idnGb2312.h>
#include <idnGbkext1.h>
#include <idnGbkext2.h>
#include <idnGbkextinv.h>
#include <idnCp936ext.h>

SInt convertMbToWc4Gbk( void    * aSrc,
                        SInt      aSrcRemain,
                        void    * aDest,
                        SInt      aDestRemain )
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

    UChar   * sSrcCharPtr;
    UShort    sWc;
    SInt      sRet;

    sSrcCharPtr = (UChar *)aSrc;

    if ( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] < 0xff ) )
    {
        if ( aSrcRemain < 2 )
        {
            sRet = RET_TOOFEW;
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
                        WC_TO_UTF16BE( aDest, sWc );

                        sRet = 2;
                    }
                    else if ( sSrcCharPtr[1] == 0xaa )
                    {
                        /* 이후에 UTF16LE 이 추가되면 값을 수정해야 한다. */
                        sWc = 0x2014;
                        WC_TO_UTF16BE( aDest, sWc );

                        sRet = 2;
                    }
                    else
                    {
                        /* 예외 처리 범위에 포함되지 않는 문자이다. GB2312 로 처
                         * 리해야 한다. 아직, 변환되지 않은 것을 알리기 위해 sRe
                         * t 값을 설정한다.
                         */
                        sRet = RET_ILSEQ;
                    }
                }
                else
                {
                    /* 변환되지 않은 것을 알리기 위해 sRet 값을 설정한다. */
                    sRet = RET_ILSEQ;
                }

                if ( sRet != RET_ILSEQ )
                {
                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    /* Nothing to do. */
                }

                /* 2) 그외의 문자는 서브셋인 GB2312 에 전달 */
                if ( ( sSrcCharPtr[1] >= 0xa1 ) &&
                     ( sSrcCharPtr[1] < 0xff ) )
                {
                    sRet = convertMbToWc4Gb2312( aSrc,
                                                 aSrcRemain,
                                                 aDest,
                                                 aDestRemain );

                    if ( sRet != RET_ILSEQ )
                    {
                        IDE_CONT( NORMAL_EXIT );
                    }
                    else
                    {
                        /* Nothing to do. */
                    }

                    /* 3) CP936EXT 에 전달
                     *
                     * GB2312 에서 변환하지 않는 User_Defined_Area의 문자가 존재
                     * 한다. 이러한 문자는 GB2312 범위에 있지만, 변환 코드페이지
                     * ( 코드값 배열 )가 없다. 따라서, 코드페이지가 정의된 CP936
                     * EXT 에 전달한다.
                     */
                    sRet = convertMbToWc4Cp936ext( aSrc,
                                                   aSrcRemain,
                                                   aDest,
                                                   aDestRemain );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* 4) GBK 의 변환방안을 적용 */
                if ( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] <= 0xa0 ) )
                {
                    sRet = convertMbToWc4Gbkext1( aSrc,
                                                  aSrcRemain,
                                                  aDest,
                                                  aDestRemain );
                }
                else
                {
                    sRet = RET_ILSEQ;
                }
            }

            if ( sRet != RET_ILSEQ )
            {
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                /* Nothing to do. */
            }

            /* 4) GBK 의 변환방안을 적용 */
            if ( ( sSrcCharPtr[0] >= 0xa8 ) &&
                 ( sSrcCharPtr[0] <= 0xfe ) )
            {
                sRet = convertMbToWc4Gbkext2( aSrc,
                                              aSrcRemain,
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
                        WC_TO_UTF16BE( aDest, sWc );

                        sRet = 2;
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
            }
        }
    }
    else
    {
        sRet = RET_ILSEQ;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}

SInt convertWcToMb4Gbk( void    * aSrc,
                        SInt      aSrcRemain,
                        void    * aDest,
                        SInt      aDestRemain )
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

    UChar             * sDestCharPtr;
    SInt                sRet;
    UShort              sWc;

    sDestCharPtr = (UChar *)aDest;

    UTF16BE_TO_WC( sWc, aSrc );

    if ( aDestRemain < 2 )
    {
        sRet = RET_TOOSMALL;
    }
    else
    {
        /* 1) GB2312 에 전달 */
        if ( ( sWc != 0x30fb ) && ( sWc != 0x2015 ) )
        {
            sRet = convertWcToMb4Gb2312( aSrc, aSrcRemain, aDest, aDestRemain );
        }
        else
        {
            /* 변환되지 않은 것을 알리기 위해 sRet 값을 설정한다. */
            sRet = RET_ILUNI;
        }

        if ( sRet != RET_ILUNI )
        {
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do. */
        }

        /* 2) GBK 의 변환방안을 적용 */
        sRet = convertWcToMb4Gbkextinv( aSrc,
                                        aSrcRemain,
                                        aDest,
                                        aDestRemain );

        if ( sRet != RET_ILUNI )
        {
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do. */
        }

        if ( ( sWc >= 0x2170 ) && ( sWc <= 0x2179 ) )
        {
            sDestCharPtr[0] = 0xa2;
            sDestCharPtr[1] = 0xa1 + ( sWc - 0x2170 );

            sRet = 2;
        }
        else
        {
            sRet = convertWcToMb4Cp936ext( aSrc,
                                           aSrcRemain,
                                           aDest,
                                           aDestRemain );

            if ( sRet != RET_ILUNI )
            {
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                /* Nothing to do. */
            }

            /* 서브셋인 GB2312 와 상이한 문자의 예외처리이다. */
            if ( sWc == 0x00b7 )
            {
                sDestCharPtr[0] = 0xa1;
                sDestCharPtr[1] = 0xa4;

                sRet = 2;
            }
            else
            {
                if ( sWc == 0x2014 )
                {
                    sDestCharPtr[0] = 0xa1;
                    sDestCharPtr[1] = 0xaa;

                    sRet = 2;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;

}
/* PROJ-2414 [기능성] GBK, CP936 character set 추가
 *
 *  - GB18030 이 추가되었을 때에 사용할 수 있는 복사 함수이다.
 *  - 현재에 사용하지 않으므로 주석처리를 한다. ( 2014-07-15 )
 *
 * SInt copyGbk( void    * aSrc,
 *               SInt      aSrcRemain,
 *              void    * aDest,
 *              SInt      aDestRemain )
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
 **********************************************************************/
/*
 *     UChar * sSrcCharPtr;
 *     UChar * sDestCharPtr;
 *     SInt    sRet;
 *
 *     sSrcCharPtr = (UChar *)aSrc;
 *     sDestCharPtr = (UChar *)aDest;
 *
 *     if ( sSrcCharPtr[0] < 0x80)
 *     {
 *         *sDestCharPtr = *sSrcCharPtr;
 *         sRet = 1;
 *     }
 *     else
 *     {
 *        if( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] < 0xff ) )
 *         {
 *             if ( aSrcRemain < 2 )
 *             {
 *                 sRet = RET_TOOFEW;
 *             }
 *             else
 *             {
 *                 if ( aDestRemain < 2 )
 *                 {
 *                     sRet = RET_TOOSMALL;
 *                 }
 *                 else
 *                 {
 *                     sDestCharPtr[0] = sSrcCharPtr[0];
 *                     sDestCharPtr[1] = sSrcCharPtr[1];
 *
 *                     sRet = 2;
 *                 }
 *             }
 *         }
 *         else
 *         {
 *             sRet = RET_ILSEQ;
 *         }
 *     }
 *
 *     return sRet;
 * }
 */
