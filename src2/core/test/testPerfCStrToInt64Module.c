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

/* ------------------------------------------------
 *  CStr To Integer
 * ----------------------------------------------*/

acp_rc_t testCStrToInt64Init(actPerfFrmThrObj *aThrObj)
{
    ACP_UNUSED(aThrObj);
    return ACP_RC_SUCCESS;
}

acp_rc_t testCStrToInt64Body(actPerfFrmThrObj *aThrObj)
{
    acp_sint32_t    sSign;
    acp_uint64_t    sValue;
    acp_uint64_t    i;
    acp_char_t*     sEnd;

    acp_char_t      sString[128];

    ACP_UNUSED(aThrObj);
    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpSnprintf(sString, 128, "%llu", i);
        (void)acpCStrToInt64(sString, 127, &sSign, &sValue, 10, &sEnd);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testCStrToInt64Final(actPerfFrmThrObj *aThrObj)
{
    ACP_UNUSED(aThrObj);
    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  Data Descriptor
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "CStrToInt64";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* 64 add */
    {
        "CString to Integer",
        1,
        ACP_UINT64_LITERAL(10000000),
        testCStrToInt64Init,
        testCStrToInt64Body,
        testCStrToInt64Final
    } ,
    {
        "CString to Integer",
        2,
        ACP_UINT64_LITERAL(10000000),
        testCStrToInt64Init,
        testCStrToInt64Body,
        testCStrToInt64Final
    } ,
    {
        "CString to Integer",
        4,
        ACP_UINT64_LITERAL(10000000),
        testCStrToInt64Init,
        testCStrToInt64Body,
        testCStrToInt64Final
    } ,
    {
        "CString to Integer",
        8,
        ACP_UINT64_LITERAL(10000000),
        testCStrToInt64Init,
        testCStrToInt64Body,
        testCStrToInt64Final
    } ,
    ACT_PERF_FRM_SENTINEL
};

