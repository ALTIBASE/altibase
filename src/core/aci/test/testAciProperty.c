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
 * $Id: testAciProperty.c 10969 2010-04-29 09:04:59Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <aciProperty.h>


void testGetMethods(void)
{

    ACT_CHECK(aciPropertyGetXLatchUseSignal() == 0);

    ACT_CHECK(aciPropertyGetMutexType() == 0);

    ACT_CHECK(aciPropertyGetCheckMutexDurationTimeEnable() == 0);

    ACT_CHECK(aciPropertyGetTimedStatistics() == 0);

    ACT_CHECK(aciPropertyGetLatchType() == 0);

    ACT_CHECK(aciPropertyGetMutexSpinCount() == 0);

    ACT_CHECK(aciPropertyGetNativeMutexSpinCount() == 0);

    ACT_CHECK(aciPropertyGetLatchSpinCount() == 0);

    ACT_CHECK(aciPropertyGetProtocolDump() == 0);

    ACT_CHECK(aciPropertyGetServerTrcFlag() == 0);

    ACT_CHECK(aciPropertyGetSmTrcFlag() == 0);

    ACT_CHECK(aciPropertyGetQpTrcFlag() == 0);

    /* BUG-49108*/
    ACT_CHECK(aciPropertyGetJobTrcFlag() == 0);

    /* BUG-46138 */
    ACT_CHECK(aciPropertyGetSdTrcFlag() == 0);

    ACT_CHECK(aciPropertyGetRpTrcFlag() == 0);

    ACT_CHECK(aciPropertyGetDlTrcFlag() == 0);

    ACT_CHECK(aciPropertyGetLkTrcFlag() == 0);

    ACT_CHECK(aciPropertyGetXaTrcFlag() == 0);

    ACT_CHECK(aciPropertyGetSourceInfo() == 0);

    ACT_CHECK(aciPropertyGetAllMsglogFlush() == 0);

    ACT_CHECK(aciPropertyGetPrepareMemoryMax() == 0);

    ACT_CHECK(aciPropertyGetExecuteMemoryMax() == 0);

    ACT_CHECK(aciPropertyGetDirectIOEnabled() == 0);

    ACT_CHECK(aciPropertyGetQueryProfFlag() == 0);

    ACT_CHECK(aciPropertyGetQueryProfBufSize() == 0);

    ACT_CHECK(aciPropertyGetQueryProfBufFullSkip() == 0);

    ACT_CHECK(aciPropertyGetQueryProfBufFlushSize() == 0);

    ACT_CHECK(aciPropertyGetQueryProfFileSize() == 0);

    ACT_CHECK(aciPropertyGetDirectIOPageSize() == 0);

    ACT_CHECK(aciPropertyGetEnableRecTest() == 0);

    ACT_CHECK(aciPropertyGetShowMutexLeakList() == 0);

    ACT_CHECK(aciPropertyGetScalability4CPU() == ACI_SCALABILITY_CLIENT_CPU);

    ACT_CHECK(aciPropertyGetMaxScalability() == ACI_SCALABILITY_CLIENT_MAX); 
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testGetMethods();

    ACT_TEST_END();

    return 0;
}
