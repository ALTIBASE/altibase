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
 *  CStr To Double
 * ----------------------------------------------*/

acp_rc_t testCStrToDoubleInit(actPerfFrmThrObj *aThrObj)
{
    ACP_UNUSED(aThrObj);
    return ACP_RC_SUCCESS;
}

acp_rc_t testCStrToDoubleBody(actPerfFrmThrObj *aThrObj)
{
    acp_char_t      sString[128];
    acp_double_t    sDouble;
    acp_uint64_t    i;

    ACP_UNUSED(aThrObj);
    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        (void)acpSnprintf(sString, 128, "0.%0100d", i);
        (void)acpCStrToDouble(sString, 127, &sDouble, NULL);
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testCStrToDoubleFinal(actPerfFrmThrObj *aThrObj)
{
    ACP_UNUSED(aThrObj);
    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  Data Descriptor
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "CStrToDouble";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* 64 add */
    {
        "CString to Double",
        1,
        ACP_UINT64_LITERAL(1000000),
        testCStrToDoubleInit,
        testCStrToDoubleBody,
        testCStrToDoubleFinal
    } ,
    {
        "CString to Double",
        2,
        ACP_UINT64_LITERAL(1000000),
        testCStrToDoubleInit,
        testCStrToDoubleBody,
        testCStrToDoubleFinal
    } ,
    {
        "CString to Double",
        4,
        ACP_UINT64_LITERAL(1000000),
        testCStrToDoubleInit,
        testCStrToDoubleBody,
        testCStrToDoubleFinal
    } ,
    {
        "CString to Double",
        8,
        ACP_UINT64_LITERAL(1000000),
        testCStrToDoubleInit,
        testCStrToDoubleBody,
        testCStrToDoubleFinal
    } ,
    ACT_PERF_FRM_SENTINEL
};

