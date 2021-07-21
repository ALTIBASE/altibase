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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnCharSet.h>
#include <ulnConv.h>    // for ulnConvGetEndianFunc

static ulnCharSetValidFunc * const ulnCharSetValidMap[ULN_CHARACTERSET_VALIDATION_MAX] =
{
    ulnCharSetValidOff,
    ulnCharSetValidOn,
};

ACI_RC ulnCharSetValidOff(const mtlModule *aSrcCharSet,
                          acp_uint8_t     *aSourceIndex,
                          acp_uint8_t     *aSourceFence)
{
    ACP_UNUSED(aSrcCharSet);
    ACP_UNUSED(aSourceIndex);
    ACP_UNUSED(aSourceFence);

    return ACI_SUCCESS;
}


ACI_RC ulnCharSetValidOn(const mtlModule *aSrcCharSet,
                         acp_uint8_t     *aSourceIndex,
                         acp_uint8_t     *aSourceFence)
{
    while(aSourceIndex < aSourceFence)
    {
        ACI_TEST_RAISE( aSrcCharSet->nextCharPtr(&aSourceIndex, aSourceFence)
                        != NC_VALID,
                        ERR_INVALID_CHARACTER );
    }

    ACI_TEST(aSourceIndex > aSourceFence);

    return ACI_SUCCESS;

    // error를 이 함수를 호출하는 곳에서 덮어씌운다.
    ACI_EXCEPTION(ERR_INVALID_CHARACTER);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCharSetConvertNLiteral(ulnCharSet      *aCharSet,
                                 ulnFnContext    *aFnContext,
                                 const mtlModule *aSrcCharSet,
                                 const mtlModule *aDestCharSet,
                                 void            *aSrc,
                                 acp_sint32_t     aSrcLen)
{
    acp_uint8_t  *sTempIndex;
    acp_uint8_t  *sSourceIndex;
    acp_uint8_t  *sSourceFence;
    acp_uint8_t  *sResultValue;
    acp_uint8_t  *sResultFence;
    acp_sint32_t  sSrcRemain  = 0;
    acp_sint32_t  sDestRemain = 0;
    acp_sint32_t  sTempRemain = 0;

    aciConvCharSetList sSrcChatSet;
    aciConvCharSetList sDestChatSet;

    acp_bool_t    sIsSame_n;
    acp_bool_t    sIsSame_N;
    acp_bool_t    sIsSame_Quote;

    acp_bool_t    sNCharFlag = ACP_FALSE;
    acp_bool_t    sQuoteFlag = ACP_FALSE;
    acp_uint8_t   sSrcCharSize;

    aCharSet->mSrc     = (acp_uint8_t*)aSrc;
    aCharSet->mSrcLen  = aSrcLen;
    aCharSet->mDestLen = aSrcLen;

    if ((aDestCharSet != NULL) &&
        (aSrcCharSet  != NULL))
    {
        sSourceIndex = (acp_uint8_t*)aSrc;
        sSrcRemain   = aSrcLen;
        sSourceFence = sSourceIndex + sSrcRemain;

        if (aSrcCharSet != aDestCharSet)
        {
            if (aCharSet->mDest != NULL)
            {
                acpMemFree(aCharSet->mDest);
                aCharSet->mDest = NULL;
            }

            sDestRemain = aCharSet->mSrcLen * 2;
            // BUG-24878 iloader 에서 NULL 데이타 처리시 오류.
            // sDestRemain 이 0일때 malloc 을 하면 안됩니다.
            if(sDestRemain > 0)
            {
                /* BUG-30336 */
                ACI_TEST_RAISE(acpMemCalloc((void**)&aCharSet->mDest,
                                            2,
                                            aCharSet->mSrcLen) != ACP_RC_SUCCESS,
                               LABEL_NOT_ENOUGH_MEM);
            }
            else
            {
                aCharSet->mDest = NULL;
            }

            sResultValue = aCharSet->mDest;
            sResultFence = aCharSet->mDest + sDestRemain;

            sSrcChatSet  = mtlGetIdnCharSet(aSrcCharSet);
            sDestChatSet = mtlGetIdnCharSet(aDestCharSet);

            while(sSourceIndex < sSourceFence)
            {
                ACI_TEST_RAISE(sResultValue >= sResultFence,
                               LABEL_INVALID_DATA_LENGTH);

                sSrcCharSize =  mtlGetOneCharSize( sSourceIndex,
                                                   sSourceFence,
                                                   aSrcCharSet );

                // Nliteral Prefix (N'..') 시작지점인지를 검사
                if (sNCharFlag == ACP_FALSE)
                {
                    sIsSame_N = mtcCompareOneChar( sSourceIndex,
                                                   sSrcCharSize,
                                                   (acp_uint8_t*)"N",
                                                   1 );

                    sIsSame_n = mtcCompareOneChar( sSourceIndex,
                                                   sSrcCharSize,
                                                   (acp_uint8_t*)"n",
                                                   1 );

                    if (sIsSame_N == ACP_TRUE || sIsSame_n == ACP_TRUE)
                    {
                        sNCharFlag = ACP_TRUE;
                    }
                }
                // N prefix (N'..')를 이미 찾은 상태에서 quote(') 문자 검사
                else
                {
                    // N-literal의 open quote 문자를 아직 못찾은 경우
                    // open quote 문자인지 검사(N'...')
                    if (sQuoteFlag == ACP_FALSE)
                    {
                        sIsSame_Quote = mtcCompareOneChar( sSourceIndex,
                                                           sSrcCharSize,
                                                           (acp_uint8_t*)"\'",
                                                           1 );

                        // N-literal의 open quote를 찾은 경우 (N')
                        if (sIsSame_Quote == ACP_TRUE)
                        {
                            sTempRemain = sDestRemain;

                            ACI_TEST_RAISE(aciConvConvertCharSet(sSrcChatSet,
                                                                 sDestChatSet,
                                                                 sSourceIndex,
                                                                 sSrcRemain,
                                                                 sResultValue,
                                                                 &sDestRemain,
                                                                 -1 /* mNlsNcharConvExcp */ )
                                           != ACI_SUCCESS, LABEL_INVALID_DATA_LENGTH);

                            // aciConvConvertCharSet 에서 컨버젼이 안일어날수 있기 때문에 sDestRemain 을 보고 움직인다.
                            sResultValue += (sTempRemain - sDestRemain);

                            sTempIndex = sSourceIndex;

                            (void)aSrcCharSet->nextCharPtr(&sSourceIndex, sSourceFence);

                            sSrcRemain -= (sSourceIndex - sTempIndex);
                            sQuoteFlag = ACP_TRUE;
                        }
                        // N 문자 뒤에 quote 문자가 오지 않으면 N 문자는
                        // N-literal prefix가 아니므로 무시
                        else
                        {
                            sNCharFlag = ACP_FALSE;
                        }
                    }

                    // N-literal의 close quote 문자인지 검사(N'...')
                    // 위의 if와 sQuoteFlag값에 따라 if-else 관계에 있으므로
                    // else로 묶어도 되지만, N'' 의 경우 open quote 뒤에
                    // close quote가 연속으로 오므로 이를 처리하기 위해
                    // 별도의 if 로 검사한다
                    if (sQuoteFlag == ACP_TRUE)
                    {
                        sSrcCharSize =  mtlGetOneCharSize( sSourceIndex,
                                                           sSourceFence,
                                                           aSrcCharSet );

                        sIsSame_Quote = mtcCompareOneChar( sSourceIndex,
                                                           sSrcCharSize,
                                                           (acp_uint8_t*)"\'",
                                                           1 );

                        if (sIsSame_Quote == ACP_TRUE)
                        {
                            sQuoteFlag = ACP_FALSE;
                            sNCharFlag = ACP_FALSE;
                        }
                    }
                }

                sTempIndex = sSourceIndex;

                // 일반 문자열인 경우 DB charset으로 변환
                if (sQuoteFlag != ACP_TRUE)
                {
                    sTempRemain = sDestRemain;

                    ACI_TEST_RAISE(aciConvConvertCharSet(sSrcChatSet,
                                                         sDestChatSet,
                                                         sSourceIndex,
                                                         sSrcRemain,
                                                         sResultValue,
                                                         &sDestRemain,
                                                         -1 /* mNlsNcharConvExcp */ )
                                   != ACI_SUCCESS, LABEL_INVALID_DATA_LENGTH);

                    // aciConvConvertCharSet 에서 컨버젼이 안일어날수 있기 때문에 sDestRemain 을 보고 움직인다.
                    sResultValue += (sTempRemain - sDestRemain);

                    (void)aSrcCharSet->nextCharPtr(&sSourceIndex, sSourceFence);

                    sSrcRemain -= (sSourceIndex - sTempIndex);
                }
                // N-literal 문자열인 경우 (N'..') client charset 그대로 유지
                else
                {
                    (void)aSrcCharSet->nextCharPtr(&sSourceIndex, sSourceFence);

                    acpMemCpy(sResultValue, sTempIndex, sSourceIndex - sTempIndex);
                    sDestRemain -= sSourceIndex - sTempIndex;
                    sSrcRemain -= sSourceIndex - sTempIndex;
                    sResultValue += sSourceIndex - sTempIndex;
                }
            }

            aCharSet->mDestLen = sResultValue - aCharSet->mDest;
        }
        else
        {
            // TASK-3420 문자 처리 정책 개선
            // 동일한 캐릭터셋일경우 검사하지 않는다.
            // 환경변수 ALTIBASE_NLS_CHARACTERSET_VALIDATION 적용되지 않는다.
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        if (aFnContext != NULL)
        {
            ulnError(aFnContext,
                     ulERR_FATAL_MEMORY_ALLOC_ERROR,
                     "ulnCharSet::convertNLiteral");
        }
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_LENGTH)
    {
        if (aFnContext != NULL)
        {
            ulnError(aFnContext,
                     ulERR_ABORT_VALIDATE_INVALID_LENGTH);
        }
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// bug-21918: fnContext arg is NULL
// object type에 따라 Dbc 구하기
// dbc object : self return
// stmt object: parent dbc return
// desc object: parent dbc or parent stmt's parent dbc return
ulnDbc* getDbcFromObj(ulnObject* aObj)
{
    ulnDbc    *sDbc = NULL;
    ulnObject *sParentObj;

    //BUG-28184 [CodeSonar] Null Pointer Dereference
    ACI_TEST(aObj == NULL);

    switch (ULN_OBJ_GET_TYPE(aObj))
    {
        case ULN_OBJ_TYPE_DBC:
            sDbc = (ulnDbc *)aObj;
            break;
        case ULN_OBJ_TYPE_STMT:
            sDbc = (ulnDbc *)(((ulnStmt *)aObj)->mParentDbc);
            break;
        // desc 의 parent obj는 stmt이거나 dbc일 수 있다
        case ULN_OBJ_TYPE_DESC:
            sParentObj = ((ulnDesc *)aObj)->mParentObject;
            // desc's parent: Dbc
            if (ULN_OBJ_GET_TYPE(sParentObj) == ULN_OBJ_TYPE_DBC)
            {
                sDbc = (ulnDbc *)(sParentObj);
            }
            // desc's parent: stmt
            else
            {
                sDbc = (ulnDbc *)(((ulnStmt *)sParentObj)->mParentDbc);
            }
            break;
        default:
            sDbc = NULL;
            break;
    }
    return sDbc;

    ACI_EXCEPTION_END;

    return sDbc;
}

ACI_RC ulnCharSetConvert(ulnCharSet      *aCharSet,
                         ulnFnContext    *aFnContext,
                         void            *aObj,
                         const mtlModule *aSrcCharSet,
                         const mtlModule *aDestCharSet,
                         void            *aSrc,
                         acp_sint32_t    aSrcLen,
                         acp_sint32_t    aOption)
{
    ulnDbc       *sDbc = NULL;
    acp_uint8_t  *sTempIndex;
    acp_uint8_t  *sSourceIndex;
    acp_uint8_t  *sSourceFence;
    acp_uint8_t  *sResultValue;
    acp_uint8_t  *sResultFence;
    acp_sint32_t  sSrcRemain  = 0;
    acp_sint32_t  sDestRemain = 0;
    acp_sint32_t  sTempRemain = 0;

    aciConvCharSetList sSrcCharSet;
    aciConvCharSetList sDestCharSet;

    ulnObject* sObj = (ulnObject *)aObj;

// =================================================================
// bug-21918(fnContext null) 관련 수정사항
// 1번째 인자와 2번째 인자는 둘중에 하나면 받으면 된다(하나는 NULL)
// 1번째 인자를(fnContext) 받으면 내부에서 DBC 값을 구한다
// 1번째 인자가(fnContext) NULL 인 경우(SQLCWrapper에서 직접 호출한 경우)
// 2번째 인자(object)를 받아서 object 타입에 따라(dbc, stmt, desc)
// dbc를 직접 구한다
// 참고로 FnContext는 error msg를 출력하기 위해 사용하는 것이다
// dbc를 구하는 이유는 mNlsCharactersetValidation를 사용하기 위해서임
// =================================================================
    if (aFnContext == NULL)
    {
        sDbc = getDbcFromObj(sObj);
    }
    else
    {
        ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    }
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    aCharSet->mSrc     = (acp_uint8_t*)aSrc;
    aCharSet->mSrcLen  = aSrcLen;
    aCharSet->mDestLen = aSrcLen;

    if (aCharSet->mDest != NULL)
    {
        acpMemFree(aCharSet->mDest);
        aCharSet->mDest = NULL;
    }

    // BUG-24831 유니코드 드라이버에서 mtl::defaultModule() 을 호출하면 안됩니다.
    // 인자로 NULL 이 넘어올경우 클라이언트 캐릭터 셋으로 설정한다.
    if(aSrcCharSet == NULL)
    {
        aSrcCharSet = sDbc->mClientCharsetLangModule;
    }
    if(aDestCharSet == NULL)
    {
        aDestCharSet = sDbc->mClientCharsetLangModule;
    }

    if ((aDestCharSet != NULL) &&
        (aSrcCharSet  != NULL))
    {
        sSourceIndex = (acp_uint8_t*)aSrc;
        /*
         * BUG-29148 [CodeSonar]Null Pointer Dereference
         */
        ACI_TEST(sSourceIndex == NULL);
        sSrcRemain   = aSrcLen;
        sSourceFence = sSourceIndex + sSrcRemain;

        // bug-23311: utf-16 little-endian error in ODBC unicode driver
        // utf16 little-endian인 경우 big-endian으로 변환
        // ex) windows ODBC unicode driver(SQL..Set..W()계열 함수)로 들어온 문자열들
#ifndef ENDIAN_IS_BIG_ENDIAN
        if ((aSrcCharSet->id == MTL_UTF16_ID) &&
            ((aOption & CONV_DATA_IN) == CONV_DATA_IN) &&
            (aSrc != NULL) && (aSrcLen != 0))
        {
            ACI_TEST_RAISE(ulnCharSetConvWcharEndian(aCharSet,
                                                     (acp_uint8_t*)aSrc,
                                                     aSrcLen)
                           != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);
            sSourceIndex = aCharSet->mWcharEndianBuf;
            sSourceFence = sSourceIndex + sSrcRemain;
        }
#endif

        if (aSrcCharSet != aDestCharSet)
        {
            // bug-34592: converting a shift-jis to utf8 needs 3 bytes
            // ex) 0xb1 (shift-jis) => 0x ef bd b1 (utf8)
            sDestRemain = aCharSet->mSrcLen * (aDestCharSet->maxPrecision(1));

            // BUG-24878 iloader 에서 NULL 데이타 처리시 오류.
            // sDestRemain 이 0일때 malloc 을 하면 안됩니다.
            if(sDestRemain > 0)
            {
                /* BUG-30336 */
                ACI_TEST_RAISE(acpMemCalloc((void**)&aCharSet->mDest,
                                            aDestCharSet->maxPrecision(1),
                                            aCharSet->mSrcLen) != ACP_RC_SUCCESS,
                               LABEL_NOT_ENOUGH_MEM);
            }
            else
            {
                aCharSet->mDest = NULL;
            }

            sResultValue = aCharSet->mDest;
            sResultFence = aCharSet->mDest + sDestRemain;

            sSrcCharSet  = mtlGetIdnCharSet(aSrcCharSet);
            sDestCharSet = mtlGetIdnCharSet(aDestCharSet);

            while(sSourceIndex < sSourceFence)
            {
                ACI_TEST_RAISE(sResultValue >= sResultFence,
                               LABEL_INVALID_DATA_LENGTH);

                sTempRemain = sDestRemain;

                ACI_TEST_RAISE(aciConvConvertCharSet(sSrcCharSet,
                                                     sDestCharSet,
                                                     sSourceIndex,
                                                     sSrcRemain,
                                                     sResultValue,
                                                     &sDestRemain,
                                                     -1 /* mNlsNcharConvExcp */ )
                               != ACI_SUCCESS, LABEL_INVALID_DATA_LENGTH);
                // aciConvConvertCharSet 에서 컨버젼이 안일어날수 있기 때문에 sDestRemain 을 보고 움직인다.
                sResultValue += (sTempRemain - sDestRemain);

                sTempIndex = sSourceIndex;
                // TASK-3420 문자 처리 정책 개선
                // 다른 캐릭터 셋일 경우 에러를 발생하지 않는다.
                (void)aSrcCharSet->nextCharPtr(&sSourceIndex, sSourceFence);
                sSrcRemain -= (sSourceIndex - sTempIndex);
            }

            aCharSet->mDestLen = sResultValue - aCharSet->mDest;
        }
        else
        {
            // TASK-3420 문자 처리 정책 개선
            // 동일한 캐릭터셋일경우 검사하지 않는다.
            // 환경변수 ALTIBASE_NLS_CHARACTERSET_VALIDATION 적용되지 않는다.
        }

        // bug-23311: utf-16 little-endian error in ODBC unicode driver
        // 서버로부터 반환된 utf16 big-endian인 경우 little-endian으로 변환
        // ex) windows ODBC unicode driver(SQL..Get..W()계열 함수)로 반환하는 문자열들
#ifndef ENDIAN_IS_BIG_ENDIAN
        if ((aDestCharSet->id == MTL_UTF16_ID) &&
            ((aOption & CONV_DATA_OUT) == CONV_DATA_OUT))
        {
            // utf16 to utf16인 경우 src endian 직접 변경
            if (aSrcCharSet == aDestCharSet)
            {
                ACI_TEST( aSrc == NULL );     //BUG-28561 [CodeSonar] Null Pointer Dereference
                ulnConvEndian_ADJUST((acp_uint8_t*)aSrc, aSrcLen);
            }
            // 위에서 malloc을 하므로 그냥 뒤집기만 하면 됨
            else
            {
                ulnConvEndian_ADJUST(aCharSet->mDest, aCharSet->mDestLen);
            }
        }
#endif

    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        if (aFnContext != NULL)
        {
            ulnError(aFnContext,
                    ulERR_FATAL_MEMORY_ALLOC_ERROR,
                    "ulnCharSet::ulConvert");
        }
    }
    ACI_EXCEPTION(LABEL_INVALID_DATA_LENGTH)
    {
        if (aFnContext != NULL)
        {
            ulnError(aFnContext,
                     ulERR_ABORT_VALIDATE_INVALID_LENGTH);
        }
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCharSetConvertUseBuffer(ulnCharSet      *aCharSet,
                                  ulnFnContext    *aFnContext,
                                  void            *aObj,
                                  const mtlModule *aSrcCharSet,
                                  const mtlModule *aDestCharSet,
                                  void            *aSrc,
                                  acp_sint32_t     aSrcLen,
                                  void            *aDest,
                                  acp_sint32_t     aDestLen,
                                  acp_sint32_t     aOption)
{
    ulnDbc       *sDbc = NULL;
    acp_uint8_t  *sSrcPrePtr;
    acp_uint8_t  *sSrcCurPtr;
    acp_uint8_t  *sSrcFence;
    acp_uint8_t  *sResultCurPtr;
    acp_uint8_t  *sResultFence;
    acp_sint32_t  sSrcRemain  = 0;
    acp_sint32_t  sDestRemain = 0;
    acp_sint32_t  sTempRemain = 0;
    acp_uint8_t  *sConvBufPtr;
    acp_uint8_t   sConvBuffer[ULN_MAX_CHARSIZE] = {0, };  // 사용자 버퍼가 부족할때 변환하는데 사용
    acp_sint32_t  sConvSrcSize = 0;                       // 변환된 원본 길이
    acp_sint32_t  sConvDesSize = 0;                       // 변환된 길이

    aciConvCharSetList sSrcCharSet;
    aciConvCharSetList sDestCharSet;

    ulnObject* sObj = (ulnObject *)aObj;

    // bug-21918: FnContext가 NULL인 경우 error
    // FnContext 인자로 NULL을 받으면 2번째 인자로 넘겨받은
    // Dbc를 구할 필요 없이 그냥 사용하도록 한다
    if (aFnContext == NULL)
    {
        sDbc = getDbcFromObj(sObj);
    }
    else
    {
        ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    }
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * BUG-28980 [CodeSonar]Null Pointer Dereference]
     */
    ACI_TEST(aSrc == NULL);
    aCharSet->mSrc     = (acp_uint8_t*)aSrc;
    aCharSet->mSrcLen  = aSrcLen;
    aCharSet->mDestLen = aSrcLen;

    if (aCharSet->mDest != NULL)
    {
        acpMemFree(aCharSet->mDest);
        aCharSet->mDest = NULL;
    }

    // BUG-24831 유니코드 드라이버에서 mtl::defaultModule() 을 호출하면 안됩니다.
    // 인자로 NULL 이 넘어올경우 클라이언트 캐릭터 셋으로 설정한다.
    if(aSrcCharSet == NULL)
    {
        aSrcCharSet = sDbc->mClientCharsetLangModule;
    }
    if(aDestCharSet == NULL)
    {
        aDestCharSet = sDbc->mClientCharsetLangModule;
    }

    sDestRemain = aDestLen;

    if ((aDestCharSet != NULL) &&
        (aSrcCharSet  != NULL))
    {
        sSrcCurPtr = (acp_uint8_t*)aSrc;
        sSrcRemain = aSrcLen;
        sSrcFence  = sSrcCurPtr + sSrcRemain;

        // bug-23311: utf-16 little-endian error in ODBC unicode driver
        // utf16 little-endian인 경우 big-endian으로 변환
        // ex) windows ODBC unicode driver(SQL..Set..W()계열 함수)로 들어온 문자열들
#ifndef ENDIAN_IS_BIG_ENDIAN
        if ((aSrcCharSet->id == MTL_UTF16_ID) &&
            ((aOption & CONV_DATA_IN) == CONV_DATA_IN) &&
            (aSrc != NULL) && (aSrcLen != 0))
        {
            ACI_TEST_RAISE(ulnCharSetConvWcharEndian(aCharSet,
                                                     (acp_uint8_t*)aSrc,
                                                     aSrcLen)
                           != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);
            sSrcCurPtr   = aCharSet->mWcharEndianBuf;
            sSrcFence = sSrcCurPtr + sSrcRemain;
        }
#endif

        if (aSrcCharSet  != aDestCharSet)
        {
            sResultCurPtr = (acp_uint8_t*)aDest;
            sResultFence  = (acp_uint8_t*)aDest + sDestRemain;

            sSrcCharSet  = mtlGetIdnCharSet(aSrcCharSet);
            sDestCharSet = mtlGetIdnCharSet(aDestCharSet);

            aCharSet->mDestLen      = 0;
            aCharSet->mConvedSrcLen = 0;
            aCharSet->mCopiedDesLen = 0;

            while(sSrcCurPtr < sSrcFence)
            {
                ACI_TEST_RAISE(sResultCurPtr >= sResultFence,
                               LABEL_STRING_RIGHT_TRUNCATED);

                // 사용자 버퍼가 별로 안남았으면 로컬 버퍼로 변환
                sDestRemain = (sResultFence - sResultCurPtr);
                if (sDestRemain < ULN_MAX_CHARSIZE)
                {
                    sConvBufPtr = sConvBuffer;
                }
                else
                {
                    sConvBufPtr = sResultCurPtr;
                }
                sTempRemain = ULN_MAX_CHARSIZE;

                ACI_TEST_RAISE(aciConvConvertCharSet(sSrcCharSet,
                                                     sDestCharSet,
                                                     sSrcCurPtr,
                                                     sSrcRemain,
                                                     sConvBufPtr,
                                                     &sTempRemain,
                                                     -1 /* mNlsNcharConvExcp */ )
                               != ACI_SUCCESS, LABEL_CONVERT_CHARSET_FAILED);

                sConvDesSize = ULN_MAX_CHARSIZE - sTempRemain;

                // 변환된 aDest buffef 의 endian 변경
                // BUG-27515: string right truncated되거나
                // 문자가 잘려서 복사될 경우에도 제대로 변환하기 위해서
                // charset 변환 후 바로 endian을 변환한다.
#ifndef ENDIAN_IS_BIG_ENDIAN
                if ((aDestCharSet->id == MTL_UTF16_ID) &&
                    ((aOption & CONV_DATA_OUT) == CONV_DATA_OUT))
                {
                    ulnConvEndian_ADJUST(sConvBufPtr, sConvDesSize);
                }
#endif

                sSrcPrePtr = sSrcCurPtr;
                // TASK-3420 문자 처리 정책 개선
                // 다른 캐릭터 셋일 경우 에러를 발생하지 않는다.
                (void)aSrcCharSet->nextCharPtr(&sSrcCurPtr, sSrcFence);
                sConvSrcSize = sSrcCurPtr - sSrcPrePtr;

                sSrcRemain              -= sConvSrcSize;
                aCharSet->mConvedSrcLen += sConvSrcSize;
                aCharSet->mDestLen      += sConvDesSize;

                // 버퍼가 모자라면 mRemainText에 넣어둔다.
                if ((sResultCurPtr + sConvDesSize) > sResultFence)
                {
                    sTempRemain = sResultFence - sResultCurPtr;
                    acpMemCpy(sResultCurPtr, sConvBuffer, sTempRemain);
                    aCharSet->mCopiedDesLen += sTempRemain;

                    aCharSet->mRemainTextLen = sConvDesSize - sTempRemain;
                    acpMemCpy(aCharSet->mRemainText, sConvBuffer + sTempRemain, aCharSet->mRemainTextLen);

                    ACI_RAISE(LABEL_STRING_RIGHT_TRUNCATED);
                }
                else if (sConvDesSize > 0)
                {
                    // 로컬 버퍼를 이용해서 변환한 경우, 사용자 버퍼에 복사
                    if (sDestRemain < ULN_MAX_CHARSIZE)
                    {
                        acpMemCpy(sResultCurPtr, sConvBuffer, sConvDesSize);
                    }

                    sResultCurPtr           += sConvDesSize;
                    aCharSet->mCopiedDesLen += sConvDesSize;
                }
                else
                {
                    // do nothing: 컨버전이 일어나지 않은 경우
                }
            }
        }
        else
        {
            // TASK-3420 문자 처리 정책 개선
            // 동일한 캐릭터셋일경우 검사하지 않는다.
            // 환경변수 ALTIBASE_NLS_CHARACTERSET_VALIDATION 적용되지 않는다.

            // bug-23311: utf-16 little-endian error in ODBC unicode driver
            // 서버로부터 반환된 utf16 big-endian인 경우 little-endian으로 변환
            // ex) windows ODBC unicode driver(SQL..Get..W()계열 함수)로 반환하는 문자열들
#ifndef ENDIAN_IS_BIG_ENDIAN
            ACI_TEST( aCharSet->mSrc == NULL );     //BUG-28561 [CodeSonar] Null Pointer Dereference

            if ((aDestCharSet->id == MTL_UTF16_ID) &&
                ((aOption & CONV_DATA_OUT) == CONV_DATA_OUT))
            {
                // utf16 to utf16인 경우 src endian 직접 변경
                ulnConvEndian_ADJUST((acp_uint8_t*)aCharSet->mSrc, aCharSet->mSrcLen);
            }
#endif

            // BUG-27515: 동일한 케릭터셋일 경우에도 사용자 버퍼에 복사해야한다.
            sConvDesSize = ACP_MIN(aCharSet->mSrcLen, aDestLen);
            acpMemCpy(aDest, aCharSet->mSrc, sConvDesSize);
            aCharSet->mConvedSrcLen = aCharSet->mCopiedDesLen = sConvDesSize;
        }
    }

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_HIGH, aDest, aCharSet->mCopiedDesLen,
            "%-18s| [%5"ACI_INT32_FMT" to %5"ACI_INT32_FMT"]",
            "ulnCharSetConvertU", aSrcCharSet->id, aDestCharSet->id);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_STRING_RIGHT_TRUNCATED)
    {
        // 남은 문자열 길이 계산
        if ((aOption & CONV_CALC_TOTSIZE) == CONV_CALC_TOTSIZE)
        {
            sConvBufPtr = sConvBuffer;
            while (sSrcCurPtr < sSrcFence)
            {
                sTempRemain = ULN_MAX_CHARSIZE;

                ACI_TEST_RAISE(aciConvConvertCharSet(sSrcCharSet,
                                                     sDestCharSet,
                                                     sSrcCurPtr,
                                                     sSrcRemain,
                                                     sConvBufPtr,
                                                     &sTempRemain,
                                                     -1)
                               != ACI_SUCCESS, LABEL_CONVERT_CHARSET_FAILED);
                sSrcPrePtr = sSrcCurPtr;
                (void)aSrcCharSet->nextCharPtr(&sSrcCurPtr, sSrcFence);

                sSrcRemain -= (sSrcCurPtr - sSrcPrePtr);
                aCharSet->mDestLen += (ULN_MAX_CHARSIZE - sTempRemain);
            }
        }
    }
    ACI_EXCEPTION(LABEL_CONVERT_CHARSET_FAILED)
    {
        if (aFnContext != NULL)
        {
            ulnError(aFnContext,
                     ulERR_ABORT_CONVERT_CHARSET_FAILED,
                     sSrcCharSet,
                     sDestCharSet,
                     sSrcFence - sSrcCurPtr,
                     (sConvBufPtr == sConvBuffer) ? -1 : (sConvBufPtr - sResultCurPtr) );
        }
    }

#ifndef ENDIAN_IS_BIG_ENDIAN
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        if (aFnContext != NULL)
        {
            ulnError(aFnContext,
                    ulERR_FATAL_MEMORY_ALLOC_ERROR,
                    "ulnCharSet::ulConvert");
        }
    }
#endif

    ACI_EXCEPTION_END;

    if ((aSrcCharSet != NULL) && (aDestCharSet != NULL))
    {
        ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, aSrc, aSrcLen,
                "%-18s| [%5"ACI_INT32_FMT" to %5"ACI_INT32_FMT"] fail",
                "ulnCharSetConvertU", aSrcCharSet->id, aDestCharSet->id);
    }

    return ACI_FAILURE;
}

//==============================================================
// bug-23311: utf-16 little-endian error in ODBC unicode driver
// utf-16 little-endian 을 big-endian으로 변환시킴 (서버로 보낼때만 사용)
// malloc한 후 mSrc가 가리키도록 함.
// ulnCharSet이 전부 지역변수로 사용되므로 대부분은 소멸자 호출시 free됨
// parameter in이 여러번 되는 경우 호출시마다 바로바로 fee후 다시 malloc 됨
// direction이 CONV_DATA_IN인 경우만 호출됨
//==============================================================
ACI_RC ulnCharSetConvWcharEndian(ulnCharSet   *aCharSet,
                                 acp_uint8_t  *aSrcPtr,
                                 acp_sint32_t  aSrcLen)
{
    // 이전에 할당했던 메모리 해제
    // nchar parameter를 여러번 binding하는 경우 이전 memory가 남아있을 것이다
    if (aCharSet->mWcharEndianBuf != NULL)
    {
        acpMemFree(aCharSet->mWcharEndianBuf);
        aCharSet->mWcharEndianBuf = NULL;
    }

    ACI_TEST_RAISE(acpMemAlloc((void**)&aCharSet->mWcharEndianBuf, aSrcLen) != ACP_RC_SUCCESS,
                   LABEL_CONV_WCHAR_ENDIAN_ERR);

    acpMemCpy(aCharSet->mWcharEndianBuf, aSrcPtr, aSrcLen);

    // switch endian
    ulnConvEndian_ADJUST(aCharSet->mWcharEndianBuf, aSrcLen);

    // mSrc가 endian을 바꾼 영역을 가리키도록 한다.
    // 나중에 getConvertedText()를 호출하면 실제 mWcharEndianBuf 위치를 반환할 것임
    aCharSet->mSrc = aCharSet->mWcharEndianBuf;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CONV_WCHAR_ENDIAN_ERR)
    {
        // nothing to do;
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}
