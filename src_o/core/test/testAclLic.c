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
 * $Id: testAclLic.c 5585 2009-05-12 06:06:41Z djin $
 ******************************************************************************/

#include <act.h>
#include <aclLic.h>
#include <acpPath.h>
#include <acpMem.h>
#include <acpTime.h>

#define ACLLICENSESTRING "ALTIBASELICENSE0"
#define ACLLICENSELENGTH 16

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_rc_t sRC;
    acp_path_pool_t sPool;
    acl_lic_t sLicInfo;
    acl_lic_data_t sLicData;
    acp_sint32_t sValidity;
    acp_uint8_t sKey[][ACL_LIC_USER_KEYLENGTH] = 
    {
        "We are the champions, my friend....",
        "Perhaps love is like a resting place",
        "I was seek and tired of everything."
    };
    acp_uint8_t sData[][ACL_LIC_USER_DATALENGTH] = 
    {
        "And I'll think to myself......",
        "In those times of trouble When you are most alone!!!",
        "Tonight the super trouper beams are gonna find me"
    };

    acp_uint8_t sData2[ACL_LIC_USER_DATALENGTH];
    acp_bool_t sIsEvaluated;
    acp_char_t sID[1024];
    acp_char_t sLibDir[ACP_PATH_MAX_LENGTH] = ".";

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN(); 

    acpPathPoolInit(&sPool); 

    sRC = acpSysGetHardwareID(sID, sizeof(sID));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Open License File */
    sRC = aclLicLoadLicense(&sLicInfo, sLibDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ENOENT(sRC));

    sRC = aclLicCheckValidity(&sLicInfo, &sValidity);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ENOENT(sRC));

    sRC = aclLicIsInstalled(&sLicInfo, &sIsEvaluated);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_FALSE == sIsEvaluated);

    sRC = aclLicSetData(&sLicInfo, sKey[0], 1024, sData[0], 1024, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));

    sRC = aclLicGetData(&sLicInfo, sKey[0], 1024, sData2, 1024);
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));

    sRC = aclLicGetData(&sLicInfo,
                     ACLLICENSESTRING, ACLLICENSELENGTH,
                     sData2, 1024);
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));

    /* Prepare License Information */
    (void)acpMemSet(&sLicData, 0, sizeof(sLicData));
    (void)acpMemCpy(sLicData.mKey, ACLLICENSESTRING, ACLLICENSELENGTH);
    (void)acpMemCpy(sLicData.mArgv0,
                    "testAclLic",
                    10);
    sRC = aclLicInstall(&sLicInfo, 
                         ACLLICENSESTRING, ACLLICENSELENGTH,
                         &sLicData, sizeof(sLicData));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclLicIsInstalled(&sLicInfo, &sIsEvaluated);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_TRUE == sIsEvaluated);

    sRC = aclLicSetData(&sLicInfo, sKey[0], 1024, sData[0], 1024, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("aclLicSetData must success, but returned %d",
                    sRC));
    sRC = aclLicSetData(&sLicInfo, sKey[1], 1024, sData[1], 1024, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("aclLicSetData must success, but returned %d",
                    sRC));
    sRC = aclLicSetData(&sLicInfo, sKey[2], 1024, sData[2], 1024, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("aclLicSetData must success, but returned %d",
                    sRC));

    sRC = aclLicGetData(&sLicInfo, sKey[2], 1024, sData2, 1024);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("aclLicGetData must success, but returned %d",
                    sRC));
    ACT_CHECK(0 == acpMemCmp(sData[2], sData2, 1024));

    sRC = aclLicGetData(&sLicInfo, sKey[1], 1024, sData2, 1024);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("aclLicGetData must success, but returned %d",
                    sRC));
    ACT_CHECK(0 == acpMemCmp(sData[1], sData2, 1024));

    sRC = aclLicGetData(&sLicInfo, sKey[0], 1024, sData2, 1024);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("aclLicGetData must success, but returned %d",
                    sRC));
    ACT_CHECK(0 == acpMemCmp(sData[0], sData2, 1024));

    sRC = aclLicGetData(&sLicInfo,
                     ACLLICENSESTRING, ACLLICENSELENGTH,
                     sData2, 1024);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    ACT_TEST_END();

    return 0;
}
