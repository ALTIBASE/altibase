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
typedef struct testSubStruct
{
    acp_sint16_t m16;
    acp_sint32_t m32;
    acp_sint64_t m64;
} testSubStruct;


/* ------------------------------------------------
 *  SUB64
 * ----------------------------------------------*/

acp_rc_t testSUB64Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testSubStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testSubStruct));
    ACP_TEST(sRC != ACP_RC_SUCCESS);

    (void)acpAtomicSet64(&sPtr->m64,
                         aThrObj->mObj.mFrm->mParallel *
                         aThrObj->mObj.mFrm->mLoopCount);

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = sPtr;
    }

    return ACP_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return sRC;

}

acp_rc_t testSUB64Body(actPerfFrmThrObj *aThrObj)
{
    testSubStruct *sSUB64 = (testSubStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicSub64(&(sSUB64->m64), 1);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testSUB64Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testSubStruct *sSUB64 = (testSubStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sSUB64->m64,
                   ("ERROR !! in "
                    "SUB64 ToBe = %lld Curr = %d\n",
                    sSum, sSUB64->m64));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
/*     ACP_EXCEPTION_END; */
/*     return sRC; */

}

/* ------------------------------------------------
 *  SUB32
 * ----------------------------------------------*/

acp_rc_t testSUB32Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testSubStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testSubStruct));
    ACP_TEST(sRC != ACP_RC_SUCCESS);

    (void)acpAtomicSet32(&sPtr->m32,
                         aThrObj->mObj.mFrm->mParallel *
                         aThrObj->mObj.mFrm->mLoopCount);

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = sPtr;
    }

    return ACP_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return sRC;

}

acp_rc_t testSUB32Body(actPerfFrmThrObj *aThrObj)
{
    testSubStruct *sSUB32 = (testSubStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicSub32(&(sSUB32->m32), 1);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testSUB32Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testSubStruct *sSUB32 = (testSubStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sSUB32->m32,
                   ("ERROR !! in "
                    "SUB32 ToBe = %lld Curr = %d\n",
                    sSum, sSUB32->m32));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  SUB16
 * ----------------------------------------------*/

acp_rc_t testSUB16Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testSubStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testSubStruct));
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

acp_rc_t testSUB16Body(actPerfFrmThrObj *aThrObj)
{
    testSubStruct *sSUB16 = (testSubStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicSub16(&(sSUB16->m16), 1);

        (void)acpAtomicSub16(&(sSUB16->m16), -1);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testSUB16Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testSubStruct *sSUB16 = (testSubStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sSUB16->m16,
                   ("ERROR !! in "
                    "[%d] SUB16 ToBe = %lld Curr = %d\n",
                    aThrObj->mObj.mFrm->mParallel,
                    sSum,
                    sSUB16->m16));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}


/* ------------------------------------------------
 *  Data Descriptor
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "AtomicPerfSUB";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* 64 sub */
    {
        "Atomic SUB64",
        1,
        ACP_UINT64_LITERAL(100000000),
        testSUB64Init,
        testSUB64Body,
        testSUB64Final
    } ,
    {
        "Atomic SUB64",
        2,
        ACP_UINT64_LITERAL(100000000),
        testSUB64Init,
        testSUB64Body,
        testSUB64Final
    } ,
    {
        "Atomic SUB64",
        4,
        ACP_UINT64_LITERAL(10000000),
        testSUB64Init,
        testSUB64Body,
        testSUB64Final
    } ,
    {
        "Atomic SUB64",
        8,
        ACP_UINT64_LITERAL(10000000),
        testSUB64Init,
        testSUB64Body,
        testSUB64Final
    } ,

    /* 32 sub */
    {
        "Atomic SUB32",
        1,
        ACP_UINT64_LITERAL(100000000),
        testSUB32Init,
        testSUB32Body,
        testSUB32Final
    } ,
    {
        "Atomic SUB32",
        2,
        ACP_UINT64_LITERAL(100000000),
        testSUB32Init,
        testSUB32Body,
        testSUB32Final
    } ,
    {
        "Atomic SUB32",
        4,
        ACP_UINT64_LITERAL(10000000),
        testSUB32Init,
        testSUB32Body,
        testSUB32Final
    } ,
    {
        "Atomic SUB32",
        8,
        ACP_UINT64_LITERAL(10000000),
        testSUB32Init,
        testSUB32Body,
        testSUB32Final
    } ,

        /* 16 sub */
    {
        "Atomic SUB16",
        1,
        ACP_UINT64_LITERAL(100000000),
        testSUB16Init,
        testSUB16Body,
        testSUB16Final
    } ,
    {
        "Atomic SUB16",
        2,
        ACP_UINT64_LITERAL(100000000),
        testSUB16Init,
        testSUB16Body,
        testSUB16Final
    } ,
    {
        "Atomic SUB16",
        4,
        ACP_UINT64_LITERAL(10000000),
        testSUB16Init,
        testSUB16Body,
        testSUB16Final
    } ,
    {
        "Atomic SUB16",
        8,
        ACP_UINT64_LITERAL(10000000),
        testSUB16Init,
        testSUB16Body,
        testSUB16Final
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

