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

SQLRETURN ulnSetDescRec(ulnDesc      *aDesc,
                        acp_sint16_t  aRecNumber,
                        acp_sint16_t  aType,
                        acp_sint16_t  aSubType,
                        ulvSLen       aLength,
                        acp_sint16_t  aPrecision,
                        acp_sint16_t  aScale,
                        void         *aDataPtr,
                        ulvSLen      *aStringLengthPtr,
                        ulvSLen      *aIndicatorPtr)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext  sFnContext;
    ulnDescType   sDescType;

    ulnDescRec   *sDescRec    = NULL;

    ACP_UNUSED(aType);
    ACP_UNUSED(aSubType);
    ACP_UNUSED(aLength);
    ACP_UNUSED(aPrecision);
    ACP_UNUSED(aScale);
    ACP_UNUSED(aDataPtr);
    ACP_UNUSED(aStringLengthPtr);
    ACP_UNUSED(aIndicatorPtr);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETDESCFIELD, aDesc, ULN_OBJ_TYPE_DESC);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * ====================================
     * Function Body BEGIN
     * ====================================
     */

    sDescType = ULN_OBJ_GET_DESC_TYPE(aDesc);

    ACI_TEST_RAISE(sDescType != ULN_DESC_TYPE_ARD &&
                   sDescType != ULN_DESC_TYPE_APD &&
                   sDescType != ULN_DESC_TYPE_IRD &&
                   sDescType != ULN_DESC_TYPE_IPD,
                   LABEL_INVALID_HANDLE);

    ACI_TEST_RAISE(sDescType == ULN_DESC_TYPE_IRD, LABEL_CANNOT_MODIFY_IRD);

    ACI_TEST_RAISE(aRecNumber <= 0, LABEL_INVALID_DESC_INDEX);

    if ((acp_uint16_t)aRecNumber > ulnDescGetHighestBoundIndex(aDesc))
    {
        ACI_TEST_RAISE(ulnBindArrangeNewDescRec(aDesc, (acp_uint16_t)aRecNumber, &sDescRec)
                       != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);

        ACI_TEST_RAISE(ulnDescAddDescRec(aDesc, sDescRec) != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);
    }
    else
    {
        /*
         * =======================================================================
         * =======================================================================
         * =======================================================================
         * =======================================================================
         *
         *
         *
         *
         * BUGBUG : 여기까지만 구현했으며 이 다음부터 구현해야 한다.
         *
         *
         *
         *
         * =======================================================================
         * =======================================================================
         * =======================================================================
         * =======================================================================
         */

        sDescRec = ulnDescGetDescRec(aDesc, aRecNumber);
    }

    /*
     * BUGBUG : Consistency check 를 해야 하는데... 그거 하는 루틴이 ul 자체에 아직 없음.
     */

    /*
     * ====================================
     * Function Body END
     * ====================================
     */

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_CANNOT_MODIFY_IRD)
    {
        ulnError(&sFnContext, ulERR_ABORT_CANNOT_MODIFY_IRD);
    }

    ACI_EXCEPTION(LABEL_INVALID_DESC_INDEX)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, aRecNumber);
    }

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnSetDescRec");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
