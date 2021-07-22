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
 
/*******************************************************************************
 * $Id: testAcpAtomicPerf.c 5558 2009-05-11 04:29:01Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpAtomic.h>
#include <acpTime.h>

/* ------------------------------------------------
 * Main Body
 * ----------------------------------------------*/
typedef struct testCasStruct
{
    acp_sint16_t m16;
    acp_sint32_t m32;
    acp_sint64_t m64;
} testCasStruct;


/* ------------------------------------------------
 *  CAS64
 * ----------------------------------------------*/

acp_rc_t testCAS64Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testCasStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testCasStruct));
    ACP_TEST(sRC != ACP_RC_SUCCESS);

    (void)acpAtomicSet64(&sPtr->m64, 0);

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = sPtr;
    }

    return ACP_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return sRC;

}

acp_rc_t testCAS64Body(actPerfFrmThrObj *aThrObj)
{
    testCasStruct *sCAS64 = (testCasStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;
    acp_sint64_t   sOldValue;
    acp_sint64_t   sValue;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        sOldValue = sCAS64->m64;

        do
        {
            sValue    = sOldValue;
            sOldValue = acpAtomicCas64(&(sCAS64->m64), sValue + 1, sValue);
        } while (sValue != sOldValue);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testCAS64Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testCasStruct *sCAS64 = (testCasStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = aThrObj->mObj.mFrm->mParallel * aThrObj->mObj.mFrm->mLoopCount;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sCAS64->m64,
                   ("ERROR !! in "
                    "CAS64 ToBe = %lld Curr = %d\n",
                    sSum, sCAS64->m64));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
/*     ACP_EXCEPTION_END; */
/*     return sRC; */

}

/* ------------------------------------------------
 *  CAS32
 * ----------------------------------------------*/

acp_rc_t testCAS32Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testCasStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testCasStruct));
    ACP_TEST(sRC != ACP_RC_SUCCESS);

    (void)acpAtomicSet32(&sPtr->m32, 0);

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = sPtr;
    }

    return ACP_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return sRC;
}

acp_rc_t testCAS32Body(actPerfFrmThrObj *aThrObj)
{
    testCasStruct *sCAS32 = (testCasStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;
    acp_sint32_t   sOldValue;
    acp_sint32_t   sValue;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        sOldValue = sCAS32->m32;

        do
        {
            sValue    = sOldValue;
            sOldValue = acpAtomicCas32(&(sCAS32->m32), sValue + 1, sValue);
        } while (sValue != sOldValue);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testCAS32Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testCasStruct *sCAS32 = (testCasStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = aThrObj->mObj.mFrm->mParallel * aThrObj->mObj.mFrm->mLoopCount;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sCAS32->m32,
                   ("ERROR !! in "
                    "CAS32 ToBe = %lld Curr = %d\n",
                    sSum, sCAS32->m32));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  CAS16
 * ----------------------------------------------*/

acp_rc_t testCAS16Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testCasStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testCasStruct));
    ACP_TEST(sRC != ACP_RC_SUCCESS);

    (void)acpAtomicSet16(&sPtr->m16, 0);

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = sPtr;
    }

    return ACP_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return sRC;
}

acp_rc_t testCAS16Body(actPerfFrmThrObj *aThrObj)
{
    testCasStruct *sCAS16 = (testCasStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;
    acp_sint16_t   sOldValue;
    acp_sint16_t   sValue;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        sOldValue = sCAS16->m16;

        do
        {
            sValue    = sOldValue;
            sOldValue = acpAtomicCas16(&(sCAS16->m16), sValue + 1, sValue);
        } while (sValue != sOldValue);

        sOldValue = sCAS16->m16;

        do
        {
            sValue    = sOldValue;
            sOldValue = acpAtomicCas16(&(sCAS16->m16), sValue + 1, sValue);
        } while (sValue != sOldValue);

        sOldValue = sCAS16->m16;

        do
        {
            sValue    = sOldValue;
            sOldValue = acpAtomicCas16(&(sCAS16->m16), sValue - 1, sValue);
        } while (sValue != sOldValue);

        sOldValue = sCAS16->m16;

        do
        {
            sValue    = sOldValue;
            sOldValue = acpAtomicCas16(&(sCAS16->m16), sValue - 1, sValue);
        } while (sValue != sOldValue);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testCAS16Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testCasStruct *sCAS16 = (testCasStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sCAS16->m16,
                   ("ERROR !! in "
                    "[%d] CAS16 ToBe = %lld Curr = %d\n",
                    aThrObj->mObj.mFrm->mParallel,
                    sSum,
                    sCAS16->m16));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}


/* ------------------------------------------------
 *  Data Descriptor
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "AtomicPerfCAS";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* 64 cas */
    {
        "Atomic CAS64",
        1,
        ACP_UINT64_LITERAL(100000000),
        testCAS64Init,
        testCAS64Body,
        testCAS64Final
    } ,
    {
        "Atomic CAS64",
        2,
        ACP_UINT64_LITERAL(100000000),
        testCAS64Init,
        testCAS64Body,
        testCAS64Final
    } ,
    {
        "Atomic CAS64",
        4,
        ACP_UINT64_LITERAL(10000000),
        testCAS64Init,
        testCAS64Body,
        testCAS64Final
    } ,
    {
        "Atomic CAS64",
        8,
        ACP_UINT64_LITERAL(10000000),
        testCAS64Init,
        testCAS64Body,
        testCAS64Final
    } ,

    /* 32 cas */
    {
        "Atomic CAS32",
        1,
        ACP_UINT64_LITERAL(100000000),
        testCAS32Init,
        testCAS32Body,
        testCAS32Final
    } ,
    {
        "Atomic CAS32",
        2,
        ACP_UINT64_LITERAL(100000000),
        testCAS32Init,
        testCAS32Body,
        testCAS32Final
    } ,
    {
        "Atomic CAS32",
        4,
        ACP_UINT64_LITERAL(10000000),
        testCAS32Init,
        testCAS32Body,
        testCAS32Final
    } ,
    {
        "Atomic CAS32",
        8,
        ACP_UINT64_LITERAL(10000000),
        testCAS32Init,
        testCAS32Body,
        testCAS32Final
    } ,

        /* 16 cas */
    {
        "Atomic CAS16",
        1,
        ACP_UINT64_LITERAL(100000000),
        testCAS16Init,
        testCAS16Body,
        testCAS16Final
    } ,
    {
        "Atomic CAS16",
        2,
        ACP_UINT64_LITERAL(100000000),
        testCAS16Init,
        testCAS16Body,
        testCAS16Final
    } ,
    {
        "Atomic CAS16",
        4,
        ACP_UINT64_LITERAL(10000000),
        testCAS16Init,
        testCAS16Body,
        testCAS16Final
    } ,
    {
        "Atomic CAS16",
        8,
        ACP_UINT64_LITERAL(10000000),
        testCAS16Init,
        testCAS16Body,
        testCAS16Final
    } ,


    {
        NULL,
        0,
        0,
        NULL,
        NULL,
        NULL
    }

};

