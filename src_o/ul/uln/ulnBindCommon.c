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
#include <ulnBindCommon.h>
#include <ulnBindConvSize.h>
#include <ulnConv.h>    // for ulnConvGetEndianFunc

#define fNONE ulnBindConvSize_NONE
#define fCHAR ulnBindConvSize_DB_CHARACTER
#define fNCHR ulnBindConvSize_NATIONAL_CHARACTER

static ulnBindConvSizeFunc * const ulnBindConvSizeMap [ULN_CTYPE_MAX][ULN_MTYPE_MAX] =
{
/*                                                                                                                                                       T                                                                                        */
/*                                                               S                                                                                       I                       I                                       G               N        */
/*                               V               N               M       I                                                                       V       M                       N                       B       C       E               V        */
/*                               A       N       U               A       N       B                       D       B       V       N               A       E                       T                       L       L       O               A        */
/*                               R       U       M               L       T       I               F       O       I       A       I               R       S                       E                       O       O       M       N       R        */
/*               N       C       C       M       E               L       E       G       R       L       U       N       R       B       B       B       T       D       T       R       B       C       B       B       E       C       C        */
/*               U       H       H       B       R       B       I       G       I       E       O       B       A       B       B       Y       Y       A       A       I       V       L       L       L       L       T       H       H        */
/*               L       A       A       E       I       I       N       E       N       A       A       L       R       I       L       T       T       M       T       M       A       O       O       O       O       R       A       A        */
/*               L       R       R       R       C       T       T       R       T       L       T       E       Y       T       E       E       E       P       E       E       L       B       B       C       C       Y       R       R        */
/* NULL */     { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* DEFAULT */  { fNONE,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNCHR,  fNCHR  },
/* CHAR */     { fNONE,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fNONE,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fCHAR,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNCHR,  fNCHR  },
/* NUMERIC */  { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* BIT */      { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* STINYINT */ { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* UTINYINT */ { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* SSHORT */   { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* USHORT */   { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* SLONG */    { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* ULONG */    { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* SBIGINT */  { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* UBIGINT */  { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* FLOAT */    { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* DOUBLE */   { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* BINARY */   { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* DATE */     { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* TIME */     { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* TIMESTAMP */{ fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* INTERVAL */ { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* BLOB_LOC */ { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* CLOB_LOC */ { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* FILE */     { fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE  },
/* WCHAR */    { fNONE,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNONE,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNCHR,  fNONE,  fNONE,  fNONE,  fNONE,  fNONE,  fNCHR,  fNCHR  }
/*               N       C       V       N       N       B       S       I       B       R       F       D       B       V       N       B       V       T       D       T       I       B       C       B       C       G       N       N        */
/*               U       H       A       U       U       I       M       N       I       E       L       O       I       A       I       Y       A       I       A       I       N       L       L       L       L       E       C       V        */
/*               L       A       R       M       M       T       A       T       G       A       O       U       N       R       B       T       R       M       T       M       T       O       O       O       O       O       H       A        */
/*               L       R       C       B       E               L       E       I       L       A       B       A       B       B       E       B       E       E       E       E       B       B       B       B       M       A       R        */
/*                               H       E       R               L       G       N               T       L       R       I       L               Y       S                       R                       L       L       E       R       C        */
/*                               A       R       I               I       E       T                       E       Y       T       E               T       T                       V                       O       O       T               H        */
/*                               R               C               N       R                                                                       E       A                       A                       C       C       R               A        */
/*                                                               T                                                                                       M                       L                                       Y               R        */
/*                                                                                                                                                       P                                                                                        */
};

#undef fNONE
#undef fCHAR
#undef fNCHR

acp_bool_t ulnBindIsFixedLengthColumn(ulnDescRec *aAppDescRec, ulnDescRec *aImpDescRec)
{
    ulnCTypeID sCTYPE;

    /*
     * Memory bound LOB 이 아니면서 BINARY 나 CHAR 가 아닌 녀석들이 고정폭 컬럼이다.
     */
    if (ulnTypeIsMemBoundLob(aImpDescRec->mMeta.mMTYPE,
                             aAppDescRec->mMeta.mCTYPE) != ACP_TRUE)
    {
        sCTYPE = aAppDescRec->mMeta.mCTYPE;

        /*
         * BUGBUG : 과연 BIT 가 variable length 인가? ODBC 스펙에서는 아니다.
         *          그러나, SQL 92 표준에서는 기다.
         */
        if (sCTYPE == ULN_CTYPE_BINARY ||
            sCTYPE == ULN_CTYPE_CHAR ||
            sCTYPE == ULN_CTYPE_BIT)
        {
            return ACP_FALSE;
        }
    }

    return ACP_TRUE;
}

/*
 * Descriptor 에서 주어진 index 를 갖는 desc record 를 찾아서
 *
 *  1. 존재할 경우
 *      Descriptor 에서 삭제하고, 초기화한 후 리턴한다.
 *
 *  2. 존재하지 않을 경우
 *      하나를 생성한 후 리턴한다.
 */

ACI_RC ulnBindArrangeNewDescRec(ulnDesc *aDesc, acp_uint16_t aIndex, ulnDescRec **aOutputDescRec)
{
    ulnDescRec *sDescRec;

    if (aIndex <= ulnDescGetHighestBoundIndex(aDesc))
    {
        sDescRec = ulnDescGetDescRec(aDesc, aIndex);

        if (sDescRec == NULL)
        {
            ACI_TEST(ulnDescRecCreate(aDesc, &sDescRec, aIndex) != ACI_SUCCESS);

            ulnDescRecInitialize(aDesc, sDescRec, aIndex);
            ulnDescRecInitLobArray(sDescRec);
            ulnDescRecInitOutParamBuffer(sDescRec);
            ulnBindInfoInitialize(&sDescRec->mBindInfo);
        }
        else
        {
            /*
             * re-bind 할 경우 기존에 존재하던 것에 덮어쓴다.
             */
            ACI_TEST(ulnDescRemoveDescRec(aDesc, sDescRec, ACP_FALSE) != ACI_SUCCESS);

            /*
             * Note : re-bind 일 경우에는 BindInfo 를 초기화 안한다.
             *        만약 BindInfo 가 같을 경우 서버로 보내는 BindInfoSet 을 하나 줄이기 위함이다.
             *
             *        LobArray 도 초기화 해서는 안된다.
             *        LobArray 를 BindInfoSet 에서 할당하므로 초기화하면 NULL 이 되어버리기 때문.
             */

            ulnDescRecInitialize(aDesc, sDescRec, aIndex);
        }
    }
    else
    {
        ACI_TEST(ulnDescRecCreate(aDesc, &sDescRec, aIndex) != ACI_SUCCESS);

        ulnDescRecInitialize(aDesc, sDescRec, aIndex);
        ulnDescRecInitLobArray(sDescRec);
        ulnDescRecInitOutParamBuffer(sDescRec);
        ulnBindInfoInitialize(&sDescRec->mBindInfo);
    }

    *aOutputDescRec = sDescRec;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ulvSLen ulnBindGetUserIndLenValue(ulnIndLenPtrPair *aIndLenPair)
{
    /*
     * SQL_DESC_INDICATOR_POINTER 와 SQL_DESC_OCTET_LENGTH_PTR 이
     * 가리키는 곳의 값을 읽고 비교해서 적절한 값을 만들어서 리턴하는 함수
     *
     * 주의사항 : aIndLenPair 은 실제 포인터 값이 offset 등을 더해서 계산된
     *            실 주소라야만 한다.
     *
     * Note : SQL_DESC_INDICATOR_POINTER 과 SQL_DESC_OCTET_LENGTH_PTR 과의 관계
     *        odbc 의 문장을 읽어 보면, SQL_DESC_INDICATOR_POINTER 는 단지 NULL_DATA 를
     *        표시하는 용도로만 사용되는 듯 하다.
     */

    if (aIndLenPair->mIndicatorPtr == aIndLenPair->mLengthPtr)
    {
        if (aIndLenPair->mLengthPtr == NULL)
        {
            return SQL_NTS;
        }
        else
        {
            return *aIndLenPair->mLengthPtr;
        }
    }
    else
    {
        if (aIndLenPair->mIndicatorPtr == NULL)
        {
            if (aIndLenPair->mLengthPtr == NULL)
            {
                return SQL_NTS;
            }
            else
            {
                return *aIndLenPair->mLengthPtr;
            }
        }
        else
        {
            if (*aIndLenPair->mIndicatorPtr == SQL_NULL_DATA)
            {
                return SQL_NULL_DATA;
            }
            else
            {
                return *aIndLenPair->mLengthPtr;
            }
        }
    }
}

acp_uint16_t ulnBindCalcNumberOfParamsToSend(ulnStmt *aStmt)
{
    /*
     * BUG-16106 : array execute (stmt), FreeStmt(SQL_RESET_PARAMS) 를 한 후
     *             "select count(*) from t1"
     *             등을 수행하면 여기서 some of parameters are not bound 에러가 발생한다.
     *             발생하면 안된다.
     *
     *             파라미터 갯수를 잡은 후에 체크한 후 진행한다.
     *
     * 사용자가 바인드를 하나도 안한 상태에서 Execute 하면 그냥 서버로 Execute 를 바로
     * 보내버리게 된다. 그래서, 서버에서 보내 준 파라미터 갯수와 사용자가 바인드한 파라미터
     * 갯수를 비교해서 큰 녀석만큼 돌아야 한다.
     *
     *          ulnExecExecuteCore,
     *          ulnExecLobPhase,
     *          ulnBindDataWriteRow, 함수에서 호출한다.
     *
     * 사용자가 바인드를 하나도 안한 상태에서 Execute 하면 그냥 서버로 Execute 를 바로
     * 보내버리게 된다. 그래서, 서버에서 보내 준 파라미터 갯수와 사용자가 바인드한 파라미터
     * 갯수를 비교해서 큰 녀석만큼 돌아야 한다.
     */

    acp_uint16_t sNumberOfBoundParams; /* 사용자가 바인드한 파라미터의 갯수 */
    acp_uint16_t sNumberOfParams;      /* ParamSet 에 있는 param의 갯수 - X 축의 원소갯수 */

    sNumberOfBoundParams = ulnDescGetHighestBoundIndex(aStmt->mAttrApd);
    sNumberOfParams      = ulnStmtGetParamCount(aStmt);

    if (sNumberOfParams < sNumberOfBoundParams)
    {
        sNumberOfParams = sNumberOfBoundParams;
    }

    return sNumberOfParams;
}

ACI_RC ulnBindAdjustStmtInfo( ulnStmt *aStmt )
{
    acp_uint32_t  sParamNumber;
    acp_uint16_t  sNumberOfParams;      /* 파라미터의 갯수 */
    ulnCTypeID    sMetaCTYPE;
    ulnMTypeID    sMetaMTYPE;
    ulnMTypeID    sMTYPE                = ULN_MTYPE_MAX;
    ulnDescRec   *sDescRecIpd;
    ulnDescRec   *sDescRecApd;
    acp_uint32_t  sMaxBindDataSize      = 0;
    acp_uint32_t  sInOutType            = ULN_PARAM_INOUT_TYPE_INIT;
    acp_uint32_t  sInOutTypeWithoutBLOB = ULN_PARAM_INOUT_TYPE_INIT;

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(aStmt);

    for (sParamNumber = 1;
         sParamNumber <= sNumberOfParams;
         sParamNumber++)
    {
        sDescRecApd = ulnStmtGetApdRec(aStmt, sParamNumber);
        if( sDescRecApd == NULL )
        {
            continue;
        }

        sDescRecIpd = ulnStmtGetIpdRec(aStmt, sParamNumber);
        ACE_ASSERT( sDescRecIpd != NULL);

        sMetaCTYPE = ulnMetaGetCTYPE(&sDescRecApd->mMeta);
        sMetaMTYPE = ulnMetaGetMTYPE(&sDescRecIpd->mMeta);

        sMTYPE = ulnBindInfoGetMTYPEtoSet(sMetaCTYPE, sMetaMTYPE);
        ACE_ASSERT( sMTYPE < ULN_MTYPE_MAX );

        sMaxBindDataSize += (ulnBindConvSizeMap[sMetaCTYPE][sMetaMTYPE])(aStmt->mParentDbc,
                                                                         ulnMetaGetOctetLength(&sDescRecIpd->mMeta));

        /* 
         *  BUG-30537 : The size of additional information for abstract data types,
         *              cmtVariable and cmtInVariable,
         *              should be added to sMaxBindDataSize.
         *              Maximum size of additional information is 8.
         *              additional information of cmtVariable :
         *              Uchar(TypeID) + UInt(Offset) + UShort(Size) + UChar(EndFlag))
         */
        sMaxBindDataSize = sMaxBindDataSize + 8;

        if( ulnTypeIsMemBoundLob(ulnMetaGetMTYPE(&sDescRecApd->mMeta),
                                 ulnMetaGetCTYPE(&sDescRecIpd->mMeta)) == ACP_TRUE )
        {
            sInOutType |= ULN_PARAM_INOUT_TYPE_OUTPUT;
        }
        else
        {
            sInOutType |= ulnDescRecGetParamInOut(sDescRecIpd);
            sInOutTypeWithoutBLOB |= ulnDescRecGetParamInOut(sDescRecIpd);
        }
    }

    // 1. 바이드할 최대 데이터 사이즈를 설정한다.
    aStmt->mMaxBindDataSize = sMaxBindDataSize;

    // 2. 바인드 컬럼들의 전체 IN/OUT 타입을 설정한다.
    aStmt->mInOutType = sInOutType;

    // 3. BLOB타입을 제외한 바인드 컬럼들의 전체 IN/OUT 타입을 설정한다.
    aStmt->mInOutTypeWithoutBLOB = sInOutTypeWithoutBLOB;

    // 4. Bind Param Info의 재빌드를 강요한다.
    ulnStmtSetBuildBindInfo(aStmt, ACP_TRUE);

    return ACI_SUCCESS;
}
