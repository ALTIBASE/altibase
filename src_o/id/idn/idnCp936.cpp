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
 * $Id: idnCp936.cpp 65528 2014-06-16 07:10:52Z myungsub.shin $
 **********************************************************************/

#include <ide.h>
#include <idnCp936.h>
#include <idnAscii.h>
#include <idnGbk.h>

SInt convertMbToWc4Cp936( void    * aSrc,
                          SInt      aSrcRemain,
                          void    * aDest,
                          SInt      aDestRemain )
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

    UChar  * sSrcCharPtr;
    SInt     sRet;
    UShort   sWc;

    sSrcCharPtr = (UChar *)aSrc;

    /* 1) ASCII 에 전달 */
    if ( sSrcCharPtr[0] < 0x80 )
    {
        sRet = convertMbToWc4Ascii( aSrc, aSrcRemain, aDest, aDestRemain );
    }
    else
    {
        /* 2-1) GBK 에 전달 */
        if ( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] < 0xff ) )
        {
            if ( aSrcRemain < 2 )
            {
                sRet = RET_TOOFEW;
            }
            else
            {
                sRet = convertMbToWc4Gbk( aSrc,
                                          aSrcRemain,
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
                WC_TO_UTF16BE( aDest, sWc );

                sRet = 2;
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

        /* 2-2) MS936 의 변환방안을 적용 */
        if ( ( sSrcCharPtr[0] >= 0xa1 ) && ( sSrcCharPtr[0] <= 0xa2 ) )
        {
            if ( aSrcRemain < 2 )
            {
                sRet = RET_TOOFEW;
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
                    WC_TO_UTF16BE( aDest, sWc );

                    sRet = 2;
                }
                else
                {
                    /* Nothing to do */
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
                    sRet = RET_TOOFEW;
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
                        WC_TO_UTF16BE( aDest, sWc );

                        sRet = 2;
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
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}

SInt convertWcToMb4Cp936( void    * aSrc,
                          SInt      aSrcRemain,
                          void    * aDest,
                          SInt      aDestRemain )
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

    UChar  * sDestCharPtr;
    UInt     sFir;
    UInt     sSec;
    SInt     sRet;
    UShort   sWc;

    sDestCharPtr = (UChar *)aDest;

    /* 1) ASCII 에 전달 */
    sRet = convertWcToMb4Ascii( aSrc, aSrcRemain, aDest, aDestRemain );

    if ( sRet != RET_ILUNI )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do. */
    }

    /* 2) GBK 에 전달 */
    sRet = convertWcToMb4Gbk( aSrc, aSrcRemain, aDest, aDestRemain );

    if ( sRet != RET_ILUNI )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do. */
    }

    /* 3) MS936 의 변환방안을 적용. */
    UTF16BE_TO_WC( sWc, aSrc );

    if ( ( sWc >= 0xe000 ) && ( sWc < 0xe586 ) )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
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

                sRet = 2;
            }
            else
            {
                sSec = sWc - 0xe4c6;
                sFir = sSec / 96;
                sSec = sSec % 96;
                sDestCharPtr[0] = sFir + 0xa1;
                sDestCharPtr[1] = sSec + ( sSec < 0x3f ? 0x40 : 0x41 );

                sRet = 2;
            }
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

            sRet = 1;
        }
        else
        {
            sRet = RET_ILUNI;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}
