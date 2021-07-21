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
typedef struct testDecStruct
{
    acp_sint16_t m16;
    acp_sint32_t m32;
    acp_sint64_t m64;
} testDecStruct;


/* ------------------------------------------------
 *  DEC64
 * ----------------------------------------------*/
acp_rc_t testDEC64Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testDecStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testDecStruct));
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

acp_rc_t testDEC64Body(actPerfFrmThrObj *aThrObj)
{
    testDecStruct *sDEC64 = (testDecStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicDec64(&(sDEC64->m64));
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testDEC64Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testDecStruct *sDEC64 = (testDecStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sDEC64->m64,
                   ("ERROR !! in "
                    "DEC64 ToBe = %lld Curr = %d\n",
                    sSum, sDEC64->m64));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  DEC32
 * ----------------------------------------------*/
acp_rc_t testDEC32Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testDecStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testDecStruct));
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

acp_rc_t testDEC32Body(actPerfFrmThrObj *aThrObj)
{
    testDecStruct *sDEC32 = (testDecStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

/*     (void)acpFprintf(ACP_STD_OUT, "Number = %d\n", aThrObj->mNumber); */

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicDec32(&(sDEC32->m32));
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testDEC32Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testDecStruct *sDEC32 = (testDecStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sDEC32->m32,
                   ("ERROR !! in "
                    "DEC32 ToBe = %lld Curr = %d\n",
                    sSum, sDEC32->m32));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  DEC16
 * ----------------------------------------------*/
acp_rc_t testDEC16Init(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t sRC;
    testDecStruct *sPtr = NULL;
    acp_uint32_t i = 0;

    sRC = acpMemAlloc((void **)&sPtr, sizeof(testDecStruct));
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

acp_rc_t testDEC16Body(actPerfFrmThrObj *aThrObj)
{
    testDecStruct *sDEC16 = (testDecStruct *)aThrObj->mObj.mData;
    acp_uint64_t   i;

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpAtomicDec16(&(sDEC16->m16));

        (void)acpAtomicInc16(&(sDEC16->m16));
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testDEC16Final(actPerfFrmThrObj *aThrObj)
{
    /* result verify */
    testDecStruct *sDEC16 = (testDecStruct *)aThrObj->mObj.mData;
    acp_uint64_t   sSum;

    sSum = 0;

    ACT_CHECK_DESC(sSum == (acp_uint64_t)sDEC16->m16,
                   ("ERROR !! in "
                    "[%d] DEC16 ToBe = %lld Curr = %d\n",
                    aThrObj->mObj.mFrm->mParallel,
                    sSum,
                    sDEC16->m16));

    acpMemFree(aThrObj->mObj.mData);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  Data Descriptor
 * ----------------------------------------------*/
acp_char_t /*@unused@*/ *gPerName = "AtomicPerfDEC";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* 64 dec */
    {
        "Atomic DEC64",
        1,
        ACP_UINT64_LITERAL(100000000),
        testDEC64Init,
        testDEC64Body,
        testDEC64Final
    } ,
    {
        "Atomic DEC64",
        2,
        ACP_UINT64_LITERAL(100000000),
        testDEC64Init,
        testDEC64Body,
        testDEC64Final
    } ,
    {
        "Atomic DEC64",
        4,
        ACP_UINT64_LITERAL(10000000),
        testDEC64Init,
        testDEC64Body,
        testDEC64Final
    } ,
    {
        "Atomic DEC64",
        8,
        ACP_UINT64_LITERAL(10000000),
        testDEC64Init,
        testDEC64Body,
        testDEC64Final
    } ,

    /* 32 dec */
    {
        "Atomic DEC32",
        1,
        ACP_UINT64_LITERAL(100000000),
        testDEC32Init,
        testDEC32Body,
        testDEC32Final
    } ,
    {
        "Atomic DEC32",
        2,
        ACP_UINT64_LITERAL(100000000),
        testDEC32Init,
        testDEC32Body,
        testDEC32Final
    } ,
    {
        "Atomic DEC32",
        4,
        ACP_UINT64_LITERAL(10000000),
        testDEC32Init,
        testDEC32Body,
        testDEC32Final
    } ,
    {
        "Atomic DEC32",
        8,
        ACP_UINT64_LITERAL(10000000),
        testDEC32Init,
        testDEC32Body,
        testDEC32Final
    } ,

        /* 16 dec */
    {
        "Atomic DEC16",
        1,
        ACP_UINT64_LITERAL(100000000),
        testDEC16Init,
        testDEC16Body,
        testDEC16Final
    } ,
    {
        "Atomic DEC16",
        2,
        ACP_UINT64_LITERAL(100000000),
        testDEC16Init,
        testDEC16Body,
        testDEC16Final
    } ,
    {
        "Atomic DEC16",
        4,
        ACP_UINT64_LITERAL(10000000),
        testDEC16Init,
        testDEC16Body,
        testDEC16Final
    } ,
    {
        "Atomic DEC16",
        8,
        ACP_UINT64_LITERAL(10000000),
        testDEC16Init,
        testDEC16Body,
        testDEC16Final
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

