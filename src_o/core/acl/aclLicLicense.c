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
 * $Id: aclLic.c 4528 2009-02-13 01:19:19Z shawn $
 ******************************************************************************/

#include <aclLic.h>
#include <acpFile.h>
#include <acpStr.h>
#include <acpCStr.h>
#include <acpError.h>
#include <acpTime.h>
#include <acv.h>

static void aclLicCheckExpire(acl_lic_t* aLicInfo)
{
    const acp_size_t sDateLength = sizeof(aLicInfo->mLicenseInfo.mExpireDate);
    acp_time_t sTime = acpTimeNow();
    acp_time_exp_t sToday; 
    acp_uint8_t sTimeString[sizeof(aLicInfo->mLicenseInfo.mExpireDate) + 1];

    /* Format as YYYYMMDD */
    acpTimeGetLocalTime(sTime, &sToday);
    (void)acpSnprintf((acp_char_t*)sTimeString, sDateLength + 1,
                      "%04d%02d%02d",
                      sToday.mYear, sToday.mMonth, sToday.mDay);
    
    if( /* Date validation */
        acpCStrCmp(
            (acp_char_t*)(aLicInfo->mLicenseInfo.mLastRanDate),
            (acp_char_t*)sTimeString, sDateLength
            ) <= 0)
    {
        /* Copy Current Date to License Info */
        (void)acpMemCpy(aLicInfo->mLicenseInfo.mLastRanDate, sTimeString, 8);
        
        /* Check Expiration */
        if(
            (acpCStrLen(
                    (acp_char_t*)(aLicInfo->mLicenseInfo.mExpireDate),
                    sDateLength)
                != 0) &&
            (acpCStrCmp(
                    (acp_char_t*)(aLicInfo->mLicenseInfo.mLastRanDate),
                    (acp_char_t*)(aLicInfo->mLicenseInfo.mExpireDate),
                    sDateLength)
                > 0))
        {
            aLicInfo->mLicenseInfo.mExpired = (acp_uint8_t)(1);
        }
        else
        {
            aLicInfo->mLicenseInfo.mExpired = (acp_uint8_t)(0);
        }
    }
    else
    {
        /* System date is modified to past. */
        aLicInfo->mLicenseInfo.mExpired = (acp_uint8_t)(1);
    }
}

ACP_EXPORT acp_rc_t aclLicLoadLicense(
    acl_lic_t* aLicInfo,
    acp_char_t* aPath
    )
{
    acp_rc_t sRC;

    sRC = (NULL == aLicInfo || NULL == aPath)?
        ACP_RC_EFAULT :
        aclLicInit(aLicInfo, aPath);

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicCheckValidity(
    acl_lic_t* aLicInfo,
    acp_sint32_t* aValidity)
{
    acp_rc_t sRC;
    acp_rc_t sReturn = ACP_RC_SUCCESS;

    if((NULL == aLicInfo) || (NULL == aValidity))
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        sRC = aclLicOpenFile(aLicInfo);
    }

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acp_uint8_t sKey[ACL_LIC_USER_KEYLENGTH];
        acp_uint8_t sData[ACL_LIC_USER_DATALENGTH];
        acl_lic_data_t sPlainData;

        if(ACP_RC_IS_SUCCESS(
                sRC = aclLicReadFile(aLicInfo, 0, sKey, sData)
                ))
        {
            acp_bool_t sEqual = ACP_FALSE;
            
            /* Decipher data with key */
            (void)aclCryptTEADecipher(
                sData, sKey, &sPlainData,
                ACL_LIC_DATALENGTH, ACL_LIC_KEYLENGTH);
            
            if(0 != acpMemCmp(sKey, sPlainData.mKey, ACL_LIC_KEYLENGTH))
            {
                *aValidity = ACL_LIC_INVALIDKEY;
            }
            else
            {
                /* Deciphered. Check key and Hardware ID */
                sRC = aclLicCheckHardwareID(&sPlainData, &sEqual);
                
                if(ACP_RC_NOT_SUCCESS(sRC) || (ACP_FALSE == sEqual))
                {
                    *aValidity = ACL_LIC_IDNOTMATCH;
                }
                else
                {
                    /* Success! Check Expiration Date */
                    acpMemCpy(&(aLicInfo->mLicenseInfo),
                              &sPlainData, sizeof(sPlainData));
                    aclLicCheckExpire(aLicInfo);

                    *aValidity = ACL_LIC_VALID;
                }
            }
        }
        else
        {
            *aValidity = ACL_LIC_UNKNOWN;
        }

        (void)aclLicCloseFile(aLicInfo);

    }
    else
    {
        if(ACP_RC_IS_EOF(sRC))
        {
            *aValidity = ACL_LIC_INVALIDFILE;
        }
        else
        {
            if(ACP_RC_IS_ENOENT(sRC))
            {
                *aValidity = ACL_LIC_FILENOTFOUND;
            }
            else
            {
                /* Do nothing */
                *aValidity = ACL_LIC_UNKNOWN;
                sReturn = sRC;
            }
        }
    }
    
    return sReturn;
}

ACP_EXPORT acp_rc_t aclLicSaveLicense(
    acl_lic_t* aLicInfo
    )
{
    acp_rc_t sRC;

    sRC =
        (NULL == aLicInfo)?
            ACP_RC_EFAULT : 
            ((0 == aLicInfo->mLicenseInfo.mIsInstalled)?
                ACP_RC_EINVAL : aclLicOpenFile(aLicInfo)
            ); 

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Get today */
        const acp_size_t sDateLength =
            sizeof(aLicInfo->mLicenseInfo.mExpireDate);
        acp_time_t sTime = acpTimeNow();
        acp_time_exp_t sToday;
        acp_uint8_t sKey[ACL_LIC_DATALENGTH];
        acp_uint8_t sEncData[ACL_LIC_DATALENGTH];
        acp_uint8_t sTimeString[
            sizeof(aLicInfo->mLicenseInfo.mExpireDate) + 1];

        /* Format as YYYYMMDD */
        acpTimeGetLocalTime(sTime, &sToday);
        (void)acpSnprintf((acp_char_t*)sTimeString, sDateLength + 1,
                          "%04d%02d%02d",
                          sToday.mYear, sToday.mMonth, sToday.mDay);
        (void)acpMemCpy(aLicInfo->mLicenseInfo.mLastRanDate,
                        sTimeString, sDateLength);

        /* Check Expiration */
        if(acpCStrLen(
                (acp_char_t*)(aLicInfo->mLicenseInfo.mExpireDate),
                8) != 0)
        {
            if(acpCStrCmp(
                (acp_char_t*)(aLicInfo->mLicenseInfo.mLastRanDate),
                (acp_char_t*)(aLicInfo->mLicenseInfo.mExpireDate),
                8) > 0)
            {
                aLicInfo->mLicenseInfo.mExpired = (acp_uint8_t)(1);
            }
            else
            {
                /* Do nothing */
            }
        }
        else
        {
            /* Do nothing */
        }

        /* Write to file */
        (void)acpMemSet(sKey, 0, ACL_LIC_DATALENGTH);
        (void)acpMemCpy(sKey, aLicInfo->mLicenseInfo.mKey, ACL_LIC_KEYLENGTH);
        (void)aclCryptTEAEncipher(
            &(aLicInfo->mLicenseInfo), sKey, sEncData,
            ACL_LIC_DATALENGTH, ACL_LIC_KEYLENGTH);

        sRC = aclLicWriteFile(aLicInfo, 0, sKey, sEncData);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = aclLicCloseFile(aLicInfo);
        }
        else
        {
            /* Already an error with Write */
            (void)aclLicCloseFile(aLicInfo);
        }
    }
    else
    {
        /* Just return error code !!!!!!!!!!!! */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicGetData(
    acl_lic_t* aLicInfo,
    void* aKey, acp_size_t aKeySize,
    void* aData, acp_size_t aDataSize
    )
{
    acp_rc_t sRC;

    if((NULL == aLicInfo) || (NULL == aKey) || (NULL == aData))
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        if(0 == aLicInfo->mLicenseInfo.mIsInstalled ||
           1 == aLicInfo->mLicenseInfo.mExpired)
        {
            sRC = ACP_RC_ENOTSUP;
        }
        else
        {
            sRC = aclLicOpenFile(aLicInfo);
        }
        
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            acp_uint8_t sKey[ACL_LIC_USER_KEYLENGTH];
            acp_uint8_t sEncKey[ACL_LIC_USER_KEYLENGTH];
            acp_uint8_t sData[ACL_LIC_USER_DATALENGTH];
            acp_uint8_t sPlainData[ACL_LIC_USER_DATALENGTH];
            acp_sint32_t sIndex;
            
            /* Encipher key to find data */
            acpMemSet(sKey, 0, ACL_LIC_USER_KEYLENGTH);
            (void)acpMemCpy(sKey, aKey, aKeySize);
            (void)aclCryptTEAEncipher(
                sKey, sKey, sEncKey,
                ACL_LIC_USER_KEYLENGTH,
                ACL_LIC_USER_KEYLENGTH);
            
            /* Find key in table */
            sRC = aclLicFindKey(aLicInfo, &sIndex, sEncKey, ACP_FALSE); 
        
            if(ACP_RC_IS_EEXIST(sRC))
            {
                /* Read appropriate key */
                sRC = aclLicReadFile(aLicInfo, sIndex, sEncKey, sData);
                if(ACP_RC_IS_SUCCESS(sRC))
                {
                    (void)aclCryptTEADecipher(sData, sKey, sPlainData,
                                              ACL_LIC_USER_DATALENGTH,
                                              ACL_LIC_USER_KEYLENGTH);
                    (void)acpMemCpy(aData, sPlainData, aDataSize);
                    
                    sRC = aclLicCloseFile(aLicInfo);
                }
                else
                {
                    (void)aclLicCloseFile(aLicInfo);
                }
            }
            else
            {
                if(ACP_RC_IS_SUCCESS(sRC))
                {
                    /* No entry */
                    sRC = ACP_RC_ENOENT;
                }
                else
                {
                    /* Do nothing */
                }
                /* Already an error with FindKey */
                (void)aclLicCloseFile(aLicInfo);
            }
        }
        else
        {
            /* Do nothing
             * Just return Failure */
        }
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicSetData(
    acl_lic_t* aLicInfo,
    void* aKey, acp_size_t aKeySize,
    void* aData, acp_size_t aDataSize,
    acp_bool_t aUpdateIfExists
    )
{
    acp_rc_t sRC;
        
    acp_uint8_t sKey[ACL_LIC_USER_KEYLENGTH];
    acp_uint8_t sEncKey[ACL_LIC_USER_KEYLENGTH];

    acp_sint32_t sIndex = -1;

    if((NULL == aLicInfo) || (NULL == aKey) || (NULL == aData))
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* fall through */
    }

    if((0 == aLicInfo->mLicenseInfo.mIsInstalled) ||
       (1 == aLicInfo->mLicenseInfo.mExpired))
    {
        return ACP_RC_ENOTSUP;
    }
    else
    {
        /* Fall through */
    }
    
    sRC = aclLicOpenFile(aLicInfo); 
    
    if(ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* Fall through */
    }

    (void)acpMemSet(sKey, 0, ACL_LIC_USER_KEYLENGTH);
    (void)acpMemCpy(sKey, aKey, aKeySize);
    
    /* Encipher Key to find value */
    (void)aclCryptTEAEncipher(sKey, sKey, sEncKey,
                              ACL_LIC_USER_KEYLENGTH,
                              ACL_LIC_USER_KEYLENGTH);
    
    sRC = aclLicFindKey(aLicInfo,
                        &sIndex,
                        sEncKey,
                        aUpdateIfExists);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acp_uint8_t sData[ACL_LIC_USER_DATALENGTH];
        acp_uint8_t sEncData[ACL_LIC_USER_DATALENGTH];

        (void)acpMemSet(sData, 0, ACL_LIC_USER_DATALENGTH);
        (void)acpMemCpy(sData, aData, aDataSize);
        (void)aclCryptTEAEncipher(sData, aKey, sEncData,
                              ACL_LIC_USER_DATALENGTH,
                              ACL_LIC_USER_KEYLENGTH);
    
        sRC = aclLicWriteFile(aLicInfo, sIndex, sEncKey, sEncData);
    }
    else
    {
        /* Fall through */
    }
    
    /* Make sure the file is closed */
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = aclLicCloseFile(aLicInfo);
    }
    else
    {
        /* Already an error with WriteFile
         * Error shall be handled at caller */
        (void)aclLicCloseFile(aLicInfo);
    }
    
    return sRC;
}

ACP_EXPORT acp_rc_t aclLicInstall(
    acl_lic_t* aLicInfo,
    void* aKey, acp_size_t aKeySize,
    void* aData, acp_size_t aDataSize
    )
{
    acp_rc_t sRC;

    if((NULL == aLicInfo) || (NULL == aKey) || (NULL == aData))
    {
        sRC = ACP_RC_EFAULT;
    }
    else if(
        (aKeySize > 0) &&
        (aKeySize <= ACL_LIC_KEYLENGTH) &&
        (ACL_LIC_DATALENGTH == aDataSize))
    {
        acl_lic_data_t* sLic = (acl_lic_data_t*)aData;
        acp_uint8_t sKey[ACL_LIC_DATALENGTH];

        (void)acpMemSet(sKey, 0, ACL_LIC_DATALENGTH);
        (void)acpMemCpy(sKey, aKey, aKeySize);

        if(0 == acpMemCmp(sKey, sLic->mKey, ACL_LIC_KEYLENGTH))
        {
            (void)acpMemCpy(&(aLicInfo->mLicenseInfo),
                            sLic, ACL_LIC_DATALENGTH);
            aLicInfo->mLicenseInfo.mIsInstalled = (acp_uint8_t)(1);
            aLicInfo->mLicenseInfo.mExpired = (acp_uint8_t)(0);

            sRC = aclLicCreateLicense(aLicInfo);
            if(ACP_RC_IS_SUCCESS(sRC))
            {
                /* Fall through */
            }
            else
            {
                (void)acpMemSet(&(aLicInfo->mLicenseInfo),
                                0, ACL_LIC_DATALENGTH);
            }
        }
        else
        {
            sRC = ACP_RC_ENOTSUP;
        }
    }
    else
    {
        sRC = ACP_RC_ENOTSUP;
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicIsInstalled(
    acl_lic_t* aLicInfo,
    acp_bool_t* aIsInstalled
    )
{
    acp_rc_t sRC;

    if((NULL == aLicInfo) || (NULL == aIsInstalled))
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
        
        if(0 == aLicInfo->mLicenseInfo.mIsInstalled)
        {
            *aIsInstalled = ACP_FALSE;
        }
        else
        {
            *aIsInstalled = ACP_TRUE;
        }
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicSetExpired(
    acl_lic_t* aLicInfo
    )
{
    acp_uint8_t sKey[ACL_LIC_DATALENGTH];
    acp_uint8_t sEncData[ACL_LIC_DATALENGTH];
    acp_rc_t sRC;

    if(NULL == aLicInfo)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        /* Set Expired Flag */
        aLicInfo->mLicenseInfo.mExpired = (acp_uint8_t)(1);
        sRC = aclLicOpenFile(aLicInfo);
    }

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Write to file */
        (void)acpMemSet(sKey, 0, ACL_LIC_DATALENGTH);
        (void)acpMemCpy(sKey, aLicInfo->mLicenseInfo.mKey, ACL_LIC_KEYLENGTH);
        (void)aclCryptTEAEncipher(
            &(aLicInfo->mLicenseInfo), sKey, sEncData,
            ACL_LIC_KEYLENGTH, ACL_LIC_DATALENGTH);

        sRC = aclLicWriteFile(aLicInfo, 0, sKey, sEncData);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = aclLicCloseFile(aLicInfo);
        }
        else
        {
            /* Already an error with WriteFile
             * Error shall be handled at caller */
            (void)aclLicCloseFile(aLicInfo);
        }
    }
    else
    {
        /* Do nothing.
         * Return failure */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicIsExpired(
    acl_lic_t* aLicInfo,
    acp_bool_t* aExpired
    )
{
    if((NULL == aLicInfo) || (NULL == aExpired))
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* fall through */
    }

    /* Check Expiration again */
    aclLicCheckExpire(aLicInfo);

    if(0 != aLicInfo->mLicenseInfo.mExpired)
    {
        *aExpired = ACP_TRUE;
    }
    else
    {
        *aExpired = ACP_FALSE;
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t aclLicSetNetworkTraffic(
    acl_lic_t* aLicInfo,
    acp_uint64_t aTraffic
    )
{
    acp_uint8_t sNetwork[16];
    acp_uint8_t sKey[ACL_LIC_DATALENGTH];
    acp_uint8_t sEncData[ACL_LIC_DATALENGTH];
    acp_rc_t sRC;
    acp_sint32_t i;

    acp_char_t sN1;
    acp_char_t sN2;

    if(NULL == aLicInfo)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* Fall through */
    }

    for(i = 0; i < 8; i++)
    {
        sNetwork[i] = (acp_uint8_t)((aTraffic >> (8 * (acp_uint32_t)i)) & 0xFF);
    }
    for(; i<16; i++)
    {
        sNetwork[i] = 0;
    }
    (void)acpMemCpy(aLicInfo->mLicenseInfo.mLastRanTraffic, sNetwork, 16);

    /* If network traffic exceeds, set expired flag */
    for(i = 15; i >= 0; i--)
    {
        sN1 = aLicInfo->mLicenseInfo.mExpireTraffic[i];
        sN2 = aLicInfo->mLicenseInfo.mLastRanTraffic[i];

        if(sN1 != 0)
        {
            if(sN2 > sN1)
            {
                aLicInfo->mLicenseInfo.mExpired = (acp_uint8_t)(1);
                break;
            }
            else
            {
                if(sN2 < sN1)
                {
                    break;
                }
                else
                {
                    /* Loop */
                }
            }
        }
        else
        {
            /* No need to compare
             * With this, there will be no comparison when mExpireTraffic is
             * filled with 0 */
        }
    }

    sRC = aclLicOpenFile(aLicInfo);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Write to file */
        (void)acpMemSet(sKey, 0, ACL_LIC_DATALENGTH);
        (void)acpMemCpy(sKey, aLicInfo->mLicenseInfo.mKey, ACL_LIC_KEYLENGTH);
        (void)aclCryptTEAEncipher(
            &(aLicInfo->mLicenseInfo), sKey, sEncData,
            ACL_LIC_KEYLENGTH,
            ACL_LIC_DATALENGTH);

        sRC = aclLicWriteFile(aLicInfo, 0, sKey, sEncData);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = aclLicCloseFile(aLicInfo);
        }
        else
        {
            /* Already an error with WriteFile
             * Error shall be handled at caller */
            (void)aclLicCloseFile(aLicInfo);
        }
    }
    else
    {
        /* Error open file
         * Return failure */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicGetExpiryDate(
    acl_lic_t* aLicInfo,
    acp_sint32_t* aYear,
    acp_sint32_t* aMonth,
    acp_sint32_t* aDay,
    acp_bool_t* aPermanent)
{
    acp_rc_t sRC;

    if(NULL == aLicInfo)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;

        if(acpCStrLen((acp_char_t*)
                        (aLicInfo->mLicenseInfo.mExpireDate),
                        8) == 0)
        {
            *aPermanent = ACP_TRUE;
        }
        else
        {
            acp_char_t* sExpireDate = (acp_char_t*)
                aLicInfo->mLicenseInfo.mExpireDate;
            *aPermanent = ACP_FALSE;

            *aYear = 
                (sExpireDate[0] - '0') * 1000 +
                (sExpireDate[1] - '0') * 100 +
                (sExpireDate[2] - '0') * 10 +
                (sExpireDate[3] - '0');

            *aMonth = 
                (sExpireDate[4] - '0') * 10 +
                (sExpireDate[5] - '0');

            *aDay = 
                (sExpireDate[6] - '0') * 10 +
                (sExpireDate[7] - '0');
        }
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicGetProducts(
    acl_lic_t* aLicInfo,
    acp_char_t* aProducts,
    acp_size_t aSize)
{
    acp_rc_t sRC;

    if(NULL == aLicInfo)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        (void)acpCStrCpy(
            aProducts, aSize,
            (acp_char_t*)aLicInfo->mLicenseInfo.mProducts,
            sizeof(aLicInfo->mLicenseInfo.mProducts));

        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}
