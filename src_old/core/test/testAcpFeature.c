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
 * $Id: testAcpFeature.c 11251 2010-06-14 06:44:26Z denisb $
 ******************************************************************************/

#include <act.h>
#include <acpFeature.h>


acp_sint32_t main(void)
{
    acp_rc_t sRC;
    acp_feature_t sFeature;
    acp_bool_t sSupport;
    acp_char_t sAtomicType[128];
    
    ACT_TEST_BEGIN();

    sRC = acpFeatureStatus(ACP_FEATURE_TYPE_ATOMIC, &sSupport, sAtomicType);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

#if (defined ALTI_CFG_CPU_PARISC)

    /* Atomic opertions are not supported on PARISC */
    ACT_CHECK(sSupport == ACP_FALSE);

#elif (defined ALTI_CFG_OS_SOLARIS) && \
    (ALTI_CFG_OS_MAJOR == 2) &&  \
    (ALTI_CFG_OS_MINOR <= 9)

    ACT_CHECK(sSupport == ACP_TRUE);
    ACT_CHECK(acpCStrCmp(sAtomicType,
                         "SIMULATE",
                         acpCStrLen("SIMULATE", 10)) == 0);
#else

    ACT_CHECK(sSupport == ACP_TRUE); 
    ACT_CHECK(acpCStrCmp(sAtomicType,
                         "NATIVE",
                         acpCStrLen("NATIVE", 10)) == 0);
#endif

    sRC = acpFeatureStatus(ACP_FEATURE_TYPE_IPV6, &sSupport);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSupport == ACP_FALSE);

    sFeature = 0xFFFFFFFF;
    sRC = acpFeatureStatus(sFeature, &sSupport);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    ACT_TEST_END();

    return 0;
}
