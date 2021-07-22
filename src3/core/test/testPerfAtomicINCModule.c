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
typedef struct testIncStruct
{
    acp_sint16_t m16;
    acp_sint32_t m32;
    acp_sint64_t m64;
} testIncStruct;


/* ------------------------------------------------
 *  INC64
 * ----------------------------------------------*/

acp_rc_t testINC64Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testIncStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testIncStruct));
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

acp_rc_t testINC64Body(actPerfFrmThrObj *aThrObj)
{
    testIncStruct *sINC64 = (testIncStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicInc64(&(sINC64->m64));
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testINC64Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testIncStruct *sINC64 = (testIncStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = aThrObj->mObj.mFrm->mParallel * aThrObj->mObj.mFrm->mLoopCount;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sINC64->m64,
                   ("ERROR !! in "
                    "INC64 ToBe = %lld Curr = %d\n",
                    sSum, sINC64->m64));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
/*     ACP_EXCEPTION_END; */
/*     return sRC; */

}

/* ------------------------------------------------
 *  INC32
 * ----------------------------------------------*/

acp_rc_t testINC32Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testIncStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testIncStruct));
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

acp_rc_t testINC32Body(actPerfFrmThrObj *aThrObj)
{
    testIncStruct *sINC32 = (testIncStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicInc32(&(sINC32->m32));
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testINC32Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testIncStruct *sINC32 = (testIncStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = aThrObj->mObj.mFrm->mParallel * aThrObj->mObj.mFrm->mLoopCount;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sINC32->m32,
                   ("ERROR !! in "
                    "INC32 ToBe = %lld Curr = %d\n",
                    sSum, sINC32->m32));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  INC16
 * ----------------------------------------------*/

acp_rc_t testINC16Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testIncStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testIncStruct));
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

acp_rc_t testINC16Body(actPerfFrmThrObj *aThrObj)
{
    testIncStruct *sINC16 = (testIncStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicInc16(&(sINC16->m16));

        (void)acpAtomicDec16(&(sINC16->m16));
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testINC16Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testIncStruct *sINC16 = (testIncStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sINC16->m16,
                   ("ERROR !! in "
                    "[%d] INC16 ToBe = %lld Curr = %d\n",
                    aThrObj->mObj.mFrm->mParallel,
                    sSum,
                    sINC16->m16));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}


/* ------------------------------------------------
 *  Data Descriptor
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "AtomicPerfINC";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* 64 inc */
    {
        "Atomic INC64",
        1,
        ACP_UINT64_LITERAL(100000000),
        testINC64Init,
        testINC64Body,
        testINC64Final
    } ,
    {
        "Atomic INC64",
        2,
        ACP_UINT64_LITERAL(100000000),
        testINC64Init,
        testINC64Body,
        testINC64Final
    } ,
    {
        "Atomic INC64",
        4,
        ACP_UINT64_LITERAL(10000000),
        testINC64Init,
        testINC64Body,
        testINC64Final
    } ,
    {
        "Atomic INC64",
        8,
        ACP_UINT64_LITERAL(10000000),
        testINC64Init,
        testINC64Body,
        testINC64Final
    } ,

    /* 32 inc */
    {
        "Atomic INC32",
        1,
        ACP_UINT64_LITERAL(100000000),
        testINC32Init,
        testINC32Body,
        testINC32Final
    } ,
    {
        "Atomic INC32",
        2,
        ACP_UINT64_LITERAL(100000000),
        testINC32Init,
        testINC32Body,
        testINC32Final
    } ,
    {
        "Atomic INC32",
        4,
        ACP_UINT64_LITERAL(10000000),
        testINC32Init,
        testINC32Body,
        testINC32Final
    } ,
    {
        "Atomic INC32",
        8,
        ACP_UINT64_LITERAL(10000000),
        testINC32Init,
        testINC32Body,
        testINC32Final
    } ,

        /* 16 inc */
    {
        "Atomic INC16",
        1,
        ACP_UINT64_LITERAL(100000000),
        testINC16Init,
        testINC16Body,
        testINC16Final
    } ,
    {
        "Atomic INC16",
        2,
        ACP_UINT64_LITERAL(100000000),
        testINC16Init,
        testINC16Body,
        testINC16Final
    } ,
    {
        "Atomic INC16",
        4,
        ACP_UINT64_LITERAL(10000000),
        testINC16Init,
        testINC16Body,
        testINC16Final
    } ,
    {
        "Atomic INC16",
        8,
        ACP_UINT64_LITERAL(10000000),
        testINC16Init,
        testINC16Body,
        testINC16Final
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

