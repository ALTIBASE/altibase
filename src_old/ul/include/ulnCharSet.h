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

#ifndef _ULN_CHARSET_H_
#define _ULN_CHARSET_H_ 1

#include <uln.h>

// ulConvert() option flags
#define CONV_DATA_IN        0x01
#define CONV_DATA_OUT       0x02
#define CONV_CALC_TOTSIZE   0x10 // 문자열 전체 길이 계산

typedef enum ulnCharactersetValidation
{
    ULN_CHARACTERSET_VALIDATION_OFF,
    ULN_CHARACTERSET_VALIDATION_ON,
    ULN_CHARACTERSET_VALIDATION_MAX
} ulnCharactersetValidation;

// PROJ-1579 NCHAR
typedef ACI_RC ulnCharSetValidFunc(const mtlModule* aSrcCharSet,
                                   acp_uint8_t *    aSourceIndex,
                                   acp_uint8_t *    aSourceFence);

ACI_RC ulnCharSetValidOff(const mtlModule* aSrcCharSet,
                          acp_uint8_t *    aSourceIndex,
                          acp_uint8_t *    aSourceFence);

ACI_RC ulnCharSetValidOn(const mtlModule* aSrcCharSet,
                         acp_uint8_t *    aSourceIndex,
                         acp_uint8_t *    aSourceFence);

typedef struct ulnCharSet
{
    acp_uint8_t  *mSrc;
    acp_sint32_t  mSrcLen;

    acp_uint8_t  *mDest;
    acp_sint32_t  mDestLen;

    acp_uint8_t  *mWcharEndianBuf;
    acp_sint32_t  mWcharEndianBufMaxLen;

    // BUG-27515: 변환된 문자열 정보
    // 마지막 문자가 잘린 경우에도 mConvedSrcLen은 해당 문자까지의 길이를 나타낸다.
    // 사용자 버퍼를 받는 ulConvert()에서만 유효.
    acp_sint32_t  mConvedSrcLen;                  // 변환된 문자열의 원본 길이 (byte 단위)
    acp_sint32_t  mCopiedDesLen;                  // 버퍼에 쓴 길이 (byte 단위)
    acp_uint8_t   mRemainText[ULN_MAX_CHARSIZE];  // 마지막 문자가 잘린 경우, 남은 값
    acp_sint32_t  mRemainTextLen;                 // 마지막 문자가 잘린 경우, 남은 길이

    /* 
     * PROJ-2047 Strengthening LOB - Partial Converting
     *
     * Partial Converting을 위한 남은 Src.
     * 다음에 전송 받은 데이터 앞부분에 붙여야 한다.
     */
    acp_uint8_t   mRemainSrc[ULN_MAX_CHARSIZE];
    acp_sint32_t  mRemainSrcLen;
} ulnCharSet;

ACP_INLINE void ulnCharSetInitialize(ulnCharSet* aCharSet)
{
    aCharSet->mSrc     = NULL;
    aCharSet->mDest    = NULL;
    aCharSet->mSrcLen  = 0;
    aCharSet->mDestLen = 0;

    aCharSet->mConvedSrcLen  = 0;
    aCharSet->mCopiedDesLen  = 0;
    aCharSet->mRemainTextLen = 0;

    aCharSet->mWcharEndianBuf = NULL;

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    aCharSet->mRemainSrcLen = 0;
}

ACP_INLINE void ulnCharSetFinalize(ulnCharSet* aCharSet)
{
    if (aCharSet->mDest != NULL)
    {
        acpMemFree(aCharSet->mDest);
    }

    aCharSet->mSrc     = NULL;
    aCharSet->mDest    = NULL;
    aCharSet->mSrcLen  = 0;
    aCharSet->mDestLen = 0;

    if (aCharSet->mWcharEndianBuf != NULL)
    {
        acpMemFree(aCharSet->mWcharEndianBuf);
        aCharSet->mWcharEndianBuf = NULL;
    }

    /* PROJ-2047 Strengthening LOB - Partial Converting */
    aCharSet->mRemainSrcLen = 0;
}

ACP_INLINE acp_uint8_t* ulnCharSetGetConvertedText(ulnCharSet *aCharSet)
{
    return (aCharSet->mDest != NULL) ? aCharSet->mDest : aCharSet->mSrc;
}

ACP_INLINE acp_sint32_t ulnCharSetGetConvertedTextLen(ulnCharSet *aCharSet)
{
    return aCharSet->mDestLen;
}

ACP_INLINE acp_sint32_t ulnCharSetGetConvedSrcLen(ulnCharSet *aCharSet)
{
    return aCharSet->mConvedSrcLen;
}

ACP_INLINE acp_sint32_t ulnCharSetGetCopiedDestLen(ulnCharSet *aCharSet)
{
    return aCharSet->mCopiedDesLen;
}

ACP_INLINE acp_uint8_t* ulnCharSetGetRemainText(ulnCharSet *aCharSet)
{
    return aCharSet->mRemainText;
}

ACP_INLINE acp_sint32_t ulnCharSetGetRemainTextLen(ulnCharSet *aCharSet)
{
    return aCharSet->mRemainTextLen;
}

/* PROJ-2047 Strengthening LOB - Partial Converting */
ACP_INLINE acp_uint8_t* ulnCharSetGetRemainSrc(ulnCharSet *aCharSet)
{
    return aCharSet->mRemainSrc;
}

ACP_INLINE acp_sint32_t ulnCharSetGetRemainSrcLen(ulnCharSet *aCharSet)
{
    return aCharSet->mRemainSrcLen;
}

ACI_RC ulnCharSetConvWcharEndian(ulnCharSet   *aCharSet,
                                 acp_uint8_t  *aSrcPtr,
                                 acp_sint32_t  aSrcLen);

ACI_RC ulnCharSetConvertNLiteral(ulnCharSet      *aCharSet,
                                 ulnFnContext    *aFnContext,
                                 const mtlModule *aSrcCharSet,
                                 const mtlModule *aDestCharSet,
                                 void            *aSrc,
                                 acp_sint32_t     aSrcLen);

ACI_RC ulnCharSetConvert(ulnCharSet      *aCharSet,
                         ulnFnContext    *aFnContext,
                         void            *aObj,
                         const mtlModule *aSrcCharSet,
                         const mtlModule *aDestCharSet,
                         void            *aSrc,
                         acp_sint32_t     aSrcLen,
                         acp_sint32_t     aOption);

ACI_RC ulnCharSetConvertUseBuffer(ulnCharSet      *aCharSet,
                                  ulnFnContext    *aFnContext,
                                  void            *aObj,
                                  const mtlModule *aSrcCharSet,
                                  const mtlModule *aDestCharSet,
                                  void            *aSrc,
                                  acp_sint32_t     aSrcLen,
                                  void            *aDest,
                                  acp_sint32_t     aDestLen,
                                  acp_sint32_t     aOption);

#endif /* _ULN_CHARSET_H_ */
