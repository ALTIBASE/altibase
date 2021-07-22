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
 * $Id: aciConvCp936ext.c 571 2014-07-17 04:54:19Z myungsub.shin $
 **********************************************************************/

#include <aciConvCp936ext.h>

static const acp_uint16_t gCp936Ext2UniA6[ /*181-159*/ ] = {
    /* 0xa6 */
                                                            0xa284,
    0xa287, 0xa288, 0xa28b, 0xa28e, 0xa2f1, 0xa28c, 0xa28f, 0xa2f0,
    0xa2f3, 0xa2f2, 0xa2f5, 0xa34c, 0xa34c, 0xa28a, 0xa28d, 0xa286,
    0xa289, 0xa280 ,0xa34c, 0xa282, 0xa285
};

static const acp_uint16_t gCp936Ext2UniA8[ /*128-122*/ ] = {
    /* 0xa8 */
                    0x5ee0, 0xa34c, 0x5df5, 0x5df9, 0xa34c, 0x5ed0
};

static const acp_uint16_t gCp936ExtPage01[ /*16*/ ] = {
    0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0xf40c, 0x5cb1, 0x5cb1, 0x5cb1,/*0x40-0x47*/
    0xf40f, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1 /*0x48-0x4f*/
};

static const acp_uint16_t gCp936ExtPage02[ /*24*/ ] = {
    0x5cb1, 0xf40a, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1,/*0x50-0x57*/
    0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1,/*0x58-0x5f*/
    0x5cb1, 0xf471, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1, 0x5cb1 /*0x60-0x67*/
};

static const acp_uint16_t gCp936ExtPageFE[ /*24*/ ] = {
    0x5cb1, 0xfa43, 0x5cb1, 0xfa45, 0xfa44, 0xfa51, 0xfa50, 0xfa41,/*0x30-0x37*/
    0xfa40, 0xfa53, 0xfa52, 0xfa5f, 0xfa5e, 0xfa57, 0xfa56, 0xfa55,/*0x38-0x3f*/
    0xfa54, 0xfa59, 0xfa58, 0xfa5b, 0xfa5a, 0x5cb1, 0x5cb1, 0x5cb1 /*0x40-0x47*/
};

ACP_EXPORT
acp_sint32_t aciConvConvertMbToWc4Cp936ext( void         * aSrc,
                                            acp_sint32_t   aSrcRemain,
                                            acp_sint32_t * aSrcAdvance,
                                            void         * aDest,
                                            acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-2414 [기능성] GBK, CP936ext character set 추가
 *     CP936EXT ==> UTF16BE
 *
 * Implementation :
 *     1) CP936EXT 에 전달
 *
 ***********************************************************************/

    acp_uint8_t  * sSrcCharPtr = (acp_uint8_t *)aSrc;
    acp_uint32_t   sNum;
    acp_sint32_t   sRet;
    acp_uint16_t   sWc;

    aDestRemain = 0;

    /* 1) CP936EXT 에 전달 */
    if ( ( sSrcCharPtr[0] == 0xa6 ) || ( sSrcCharPtr[0] == 0xa8 ) )
    {
        if ( aSrcRemain < 2 )
        {
            *aSrcAdvance = aSrcRemain;
            sRet = ACICONV_RET_TOOFEW;
        }
        else
        {
            if ( ( ( sSrcCharPtr[1] >= 0x40 ) && ( sSrcCharPtr[1] < 0x7f ) ) ||
                 ( ( sSrcCharPtr[1] >= 0x80 ) && ( sSrcCharPtr[1] < 0xff ) ) )
            {
                sNum = ( 190 * ( sSrcCharPtr[0] - 0x81 ) ) +
                         ( sSrcCharPtr[1] -
                           ( sSrcCharPtr[1] >= 0x80 ? 0x41 : 0x40 ) );
                sWc = 0xfffd;

                if ( sNum < 7410 )
                {
                    if ( ( sNum >= 7189 ) && ( sNum < 7211 ) )
                    {
                        sWc = gCp936Ext2UniA6[sNum-7189]
                            ^ ACI_CP936EXT_XOR_VALUE;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    if ( ( sNum >= 7532 ) && ( sNum < 7538 ) )
                    {
                        sWc = gCp936Ext2UniA8[sNum-7532]
                            ^ ACI_CP936EXT_XOR_VALUE;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                if ( sWc != 0xfffd )
                {
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
    else
    {
        /* GBK 에서 이미 체크하기 때문에, 사실상 접근할 수 없는 분기이다. */
        *aSrcAdvance = 1;
        sRet = ACICONV_RET_ILSEQ;
    }

    return sRet;
}

ACP_EXPORT
acp_sint32_t aciConvConvertWcToMb4Cp936ext( void         * aSrc,
                                            acp_sint32_t   aSrcRemain,
                                            acp_sint32_t * aSrcAdvance,
                                            void         * aDest,
                                            acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-2414 [기능성] GBK, CP936ext character set 추가
 *     UTF16BE ==> CP936ext
 *
 * Implementation :
 *     1) CP936EXT 의 변환방안을 적용
 *
 ***********************************************************************/

    acp_uint8_t  * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_sint32_t   sRet;
    acp_uint16_t   sVal;
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
        /* 1) CP936EXT 의 변환방안을 적용 */
        if ( ( sWc >= 0x0140 ) && ( sWc < 0x0150 ) )
        {
            sVal = gCp936ExtPage01[sWc-0x0140] ^ ACI_CP936EXT_XOR_VALUE;
        }
        else
        {
            if ( ( sWc >= 0x0250 ) && ( sWc < 0x0268 ) )
            {
                sVal = gCp936ExtPage02[sWc-0x0250] ^ ACI_CP936EXT_XOR_VALUE;
            }
            else
            {
                if ( ( sWc >= 0xfe30 ) && ( sWc < 0xfe48 ) )
                {
                    sVal = gCp936ExtPageFE[sWc-0xfe30] ^ ACI_CP936EXT_XOR_VALUE;
                }
                else
                {
                    sVal = 0xfffd;
                }
            }
        }

        if ( sVal != 0xfffd )
        {
            sDestCharPtr[0] = ( sVal >> 8 );
            sDestCharPtr[1] = ( sVal & 0xff );

            *aSrcAdvance = 2;
            sRet = 2;
        }
        else
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_ILUNI;
        }
    }

    return sRet;
}
