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
typedef struct testAddStruct
{
    acp_sint16_t m16;
    acp_sint32_t m32;
    acp_sint64_t m64;
} testAddStruct;


/* ------------------------------------------------
 *  ADD64
 * ----------------------------------------------*/

acp_rc_t testADD64Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testAddStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testAddStruct));
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

acp_rc_t testADD64Body(actPerfFrmThrObj *aThrObj)
{
    testAddStruct *sADD64 = (testAddStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicAdd64(&(sADD64->m64), 1);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testADD64Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testAddStruct *sADD64 = (testAddStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = aThrObj->mObj.mFrm->mParallel * aThrObj->mObj.mFrm->mLoopCount;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sADD64->m64,
                   ("ERROR !! in "
                    "ADD64 ToBe = %lld Curr = %d\n",
                    sSum, sADD64->m64));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
/*     ACP_EXCEPTION_END; */
/*     return sRC; */

}

/* ------------------------------------------------
 *  ADD32
 * ----------------------------------------------*/

acp_rc_t testADD32Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testAddStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testAddStruct));
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

acp_rc_t testADD32Body(actPerfFrmThrObj *aThrObj)
{
    testAddStruct *sADD32 = (testAddStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicAdd32(&(sADD32->m32), 1);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testADD32Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testAddStruct *sADD32 = (testAddStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = aThrObj->mObj.mFrm->mParallel * aThrObj->mObj.mFrm->mLoopCount;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sADD32->m32,
                   ("ERROR !! in "
                    "ADD32 ToBe = %lld Curr = %d\n",
                    sSum, sADD32->m32));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  ADD16
 * ----------------------------------------------*/

acp_rc_t testADD16Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testAddStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testAddStruct));
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

acp_rc_t testADD16Body(actPerfFrmThrObj *aThrObj)
{
    testAddStruct *sADD16 = (testAddStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicAdd16(&(sADD16->m16), 1);

        (void)acpAtomicAdd16(&(sADD16->m16), -1);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testADD16Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testAddStruct *sADD16 = (testAddStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sADD16->m16,
                   ("ERROR !! in "
                    "[%d] ADD16 ToBe = %lld Curr = %d\n",
                    aThrObj->mObj.mFrm->mParallel,
                    sSum,
                    sADD16->m16));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}


/* ------------------------------------------------
 *  Data Descriptor
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "AtomicPerfADD";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* 64 add */
    {
        "Atomic ADD64",
        1,
        ACP_UINT64_LITERAL(100000000),
        testADD64Init,
        testADD64Body,
        testADD64Final
    } ,
    {
        "Atomic ADD64",
        2,
        ACP_UINT64_LITERAL(100000000),
        testADD64Init,
        testADD64Body,
        testADD64Final
    } ,
    {
        "Atomic ADD64",
        4,
        ACP_UINT64_LITERAL(10000000),
        testADD64Init,
        testADD64Body,
        testADD64Final
    } ,
    {
        "Atomic ADD64",
        8,
        ACP_UINT64_LITERAL(10000000),
        testADD64Init,
        testADD64Body,
        testADD64Final
    } ,

    /* 32 add */
    {
        "Atomic ADD32",
        1,
        ACP_UINT64_LITERAL(100000000),
        testADD32Init,
        testADD32Body,
        testADD32Final
    } ,
    {
        "Atomic ADD32",
        2,
        ACP_UINT64_LITERAL(100000000),
        testADD32Init,
        testADD32Body,
        testADD32Final
    } ,
    {
        "Atomic ADD32",
        4,
        ACP_UINT64_LITERAL(10000000),
        testADD32Init,
        testADD32Body,
        testADD32Final
    } ,
    {
        "Atomic ADD32",
        8,
        ACP_UINT64_LITERAL(10000000),
        testADD32Init,
        testADD32Body,
        testADD32Final
    } ,

        /* 16 add */
    {
        "Atomic ADD16",
        1,
        ACP_UINT64_LITERAL(100000000),
        testADD16Init,
        testADD16Body,
        testADD16Final
    } ,
    {
        "Atomic ADD16",
        2,
        ACP_UINT64_LITERAL(100000000),
        testADD16Init,
        testADD16Body,
        testADD16Final
    } ,
    {
        "Atomic ADD16",
        4,
        ACP_UINT64_LITERAL(10000000),
        testADD16Init,
        testADD16Body,
        testADD16Final
    } ,
    {
        "Atomic ADD16",
        8,
        ACP_UINT64_LITERAL(10000000),
        testADD16Init,
        testADD16Body,
        testADD16Final
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

