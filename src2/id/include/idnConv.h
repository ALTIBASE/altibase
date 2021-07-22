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
 * $Id: idnConv.h 24174 2007-11-19 01:23:49Z copyrei $
 *
 * Description :
 *     캐릭터 셋 변환 모듈
 *
 **********************************************************************/

#ifndef _O_IDNCONV_H_
#define _O_IDNCONV_H_ 1

#include <idl.h>
#include <idnCharSet.h>

/* Return code if invalid. (xxx_mbtowc) */
#define RET_ILSEQ      (-1)

/* Return code if only a shift sequence of n bytes was read. (xxx_mbtowc) */
#define RET_TOOFEW     (-2)



/* Return code if invalid. (xxx_wctomb) */
#define RET_ILUNI      (-3)

/* Return code if output buffer is too small. (xxx_wctomb, xxx_reset) */
#define RET_TOOSMALL   (-4)

/* CJK character sets [CCS = coded character set] [CJKV.INF chapter 3] */

/*typedef struct Summary16 {
  UShort indx; // index into big table
  UShort used; // bitmask of used entries
} Summary16; */

// 캐릭터 셋 변환을 위한 함수 포인터
typedef SInt (*charSetConvFunc)( void* aSrc, 
                                 SInt aSrcRemain, 
                                 void* aDest, 
                                 SInt aDestRemain );

typedef struct idnCharSetConvModule
{
    UInt             convPass; // 필요한 변환 횟수
    charSetConvFunc  conv1;
    charSetConvFunc  conv2;
} idnCharSetConvModule;

IDE_RC
convertCharSet( idnCharSetList   aSrcCharSet,
                idnCharSetList   aDestCharSet,
                void           * aSrc,
                SInt             aSrcRemain,
                void           * aDest,
                SInt           * aDestRemain,
                SInt             aNlsNcharConvExcp );

// To fix BUG-22699 UTF16은 BIG ENDIAN으로 통일.
// 다음 매크로는 컨버전 연산이나 비교검사 시에
// Wide-Char(컴퓨터가 알아볼 수 있는 unicode)가 필요하므로 그때 사용.

#if defined(ENDIAN_IS_BIG_ENDIAN)

#define UTF16BE_TO_WC(destWC,srcCharPtr)                 \
    do {                                                 \
        ((UChar*)&destWC)[0] = ((UChar*)srcCharPtr)[0];  \
        ((UChar*)&destWC)[1] = ((UChar*)srcCharPtr)[1];  \
    } while(0);

#define WC_TO_UTF16BE(destCharPtr,srcWC)                 \
    do {                                                 \
        ((UChar*)destCharPtr)[0] = ((UChar*)&srcWC)[0];  \
        ((UChar*)destCharPtr)[1] = ((UChar*)&srcWC)[1];  \
    } while(0);

#else

#define UTF16BE_TO_WC(destWC,srcCharPtr)                 \
    do {                                                 \
        ((UChar*)&destWC)[0] = ((UChar*)srcCharPtr)[1];  \
        ((UChar*)&destWC)[1] = ((UChar*)srcCharPtr)[0];  \
    } while(0);

#define WC_TO_UTF16BE(destCharPtr,srcWC)                 \
    do {                                                 \
        ((UChar*)destCharPtr)[0] = ((UChar*)&srcWC)[1];  \
        ((UChar*)destCharPtr)[1] = ((UChar*)&srcWC)[0];  \
    } while(0);

#endif

#endif /* _O_IDNCONV_H_ */
 
