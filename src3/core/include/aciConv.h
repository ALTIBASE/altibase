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
 * $Id: aciConv.h 11353 2010-06-25 05:28:05Z djin $
 **********************************************************************/
/*
 * Description :
 *     캐릭터 셋 변환 모듈
 *
 **********************************************************************/

#ifndef _O_ACICONVCONV_H_
#define _O_ACICONVCONV_H_ 1

#include <aciTypes.h>
#include <aciConvCharSet.h>

ACP_EXTERN_C_BEGIN

/* Return code if invalid. (xxx_mbtowc) */
#define ACICONV_RET_ILSEQ      (-1)

/* Return code if only a shift sequence of n bytes was read. (xxx_mbtowc) */
#define ACICONV_RET_TOOFEW     (-2)



/* Return code if invalid. (xxx_wctomb) */
#define ACICONV_RET_ILUNI      (-3)

/* Return code if output buffer is too small. (xxx_wctomb, xxx_reset) */
#define ACICONV_RET_TOOSMALL   (-4)

/* CJK character sets [CCS = coded character set] [CJKV.INF chapter 3] */


/* 캐릭터 셋 변환을 위한 함수 포인터 */
typedef acp_sint32_t (*aciCharSetConvFunc)( void* aSrc, 
                                 acp_sint32_t aSrcRemain,
                                 acp_sint32_t* aSrcAdvance, 
                                 void* aDest, 
                                 acp_sint32_t aDestRemain );

typedef struct aciConvCharSetConvModule
{
    acp_uint32_t             convPass; /* 필요한 변환 횟수 */
    aciCharSetConvFunc  conv1;
    aciCharSetConvFunc  conv2;
} aciConvCharSetConvModule;

ACP_EXPORT ACI_RC
aciConvConvertCharSet2( aciConvCharSetList   aSrcCharSet,
                aciConvCharSetList   aDestCharSet,
                void           * aSrc,
                acp_sint32_t           * aSrcRemain,
                void           * aDest,
                acp_sint32_t           * aDestRemain,
                acp_sint32_t             aNlsNcharConvExcp );

ACP_INLINE ACI_RC
aciConvConvertCharSet( aciConvCharSetList   aSrcCharSet,
                aciConvCharSetList   aDestCharSet,
                void           * aSrc,
                acp_sint32_t             aSrcRemain,
                void           * aDest,
                acp_sint32_t           * aDestRemain,
                acp_sint32_t             aNlsNcharConvExcp )
{
    return aciConvConvertCharSet2( aSrcCharSet, aDestCharSet, aSrc, &aSrcRemain,
                                   aDest, aDestRemain, aNlsNcharConvExcp );
}

/* To fix BUG-22699 UTF16은 BIG ENDIAN으로 통일. */
/* 다음 매크로는 컨버전 연산이나 비교검사 시에 */
/* Wide-Char(컴퓨터가 알아볼 수 있는 unicode)가 필요하므로 그때 사용. */

#if defined(ACP_CFG_BIG_ENDIAN)

#define ACICONV_UTF16BE_TO_WC(destWC,srcCharPtr)                 \
    do {                                                 \
        ((acp_uint8_t*)&destWC)[0] = ((acp_uint8_t*)srcCharPtr)[0];  \
        ((acp_uint8_t*)&destWC)[1] = ((acp_uint8_t*)srcCharPtr)[1];  \
    } while(0);

#define ACICONV_WC_TO_UTF16BE(destCharPtr,srcWC)                 \
    do {                                                 \
        ((acp_uint8_t*)destCharPtr)[0] = ((acp_uint8_t*)&srcWC)[0];  \
        ((acp_uint8_t*)destCharPtr)[1] = ((acp_uint8_t*)&srcWC)[1];  \
    } while(0);

#else

#define ACICONV_UTF16BE_TO_WC(destWC,srcCharPtr)                 \
    do {                                                 \
        ((acp_uint8_t*)&destWC)[0] = ((acp_uint8_t*)srcCharPtr)[1];  \
        ((acp_uint8_t*)&destWC)[1] = ((acp_uint8_t*)srcCharPtr)[0];  \
    } while(0);

#define ACICONV_WC_TO_UTF16BE(destCharPtr,srcWC)                 \
    do {                                                 \
        ((acp_uint8_t*)destCharPtr)[0] = ((acp_uint8_t*)&srcWC)[1];  \
        ((acp_uint8_t*)destCharPtr)[1] = ((acp_uint8_t*)&srcWC)[0];  \
    } while(0);

#endif

ACP_EXTERN_C_END

#endif /* _O_ACICONVCONV_H_ */
 
