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
#include <acpCStr.h>
#include <acpError.h>
#include <acpTime.h>
#include <acv.h>

#define ACL_LIC_EXT "dat"
#define ACL_LIC_EXT_LEN 4

#define ACL_LIC_PATH_INITSIZE 256
#define ACL_LIC_PATH_MAXSIZE 1024

ACP_EXPORT acp_rc_t aclLicCheckHardwareID(
    acl_lic_data_t* aLicInfo,
    acp_bool_t* aEqual)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if(NULL == aLicInfo)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        acp_char_t sID[ACP_SYS_ID_MAXSIZE + 1] = {0, };

        sRC = acpSysGetHardwareID(sID, ACP_SYS_ID_MAXSIZE);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            *aEqual = (0 == acpCStrCmp(
                    sID,
                    (acp_char_t*)aLicInfo->mID,
                    ACP_SYS_ID_MAXSIZE)) ? ACP_TRUE : ACP_FALSE;
        }
        else
        {
            /* Do nothing */
        }
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicOpenFile(acl_lic_t* aLicInfo)
{
    acp_rc_t sRC;

    sRC = (aLicInfo == NULL)?
        ACP_RC_EFAULT :
        acpFileOpen(&(aLicInfo->mFD),
                    aLicInfo->mName,
                    ACP_O_RDWR,
                    ACP_S_IRUSR | ACP_S_IWUSR);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acp_offset_t sOffset;
        acp_offset_t sPosition;
        acp_uint8_t sData[ACL_LIC_HEADER_LENGTH];
        acp_size_t sRead;
        aLicInfo->mOffset = -1;

        /* Find header position */
        for(sOffset = 0; ; sOffset++)
        {
            sRC = acpFileSeek(&(aLicInfo->mFD),
                              sOffset, ACP_SEEK_SET,
                              &sPosition);

            if(ACP_RC_NOT_SUCCESS(sRC))
            {
                (void)acpFileClose(&(aLicInfo->mFD));
                break;
            }
            else
            {
                /* Fall through */
            }

            sRC = acpFileRead(&(aLicInfo->mFD), sData,
                              ACL_LIC_HEADER_LENGTH, &sRead);
            if(ACP_RC_NOT_SUCCESS(sRC))
            {
                (void)acpFileClose(&(aLicInfo->mFD));
                break;
            }
            else
            {
                /* Fall through */
            }

            if(0 == acpMemCmp(sData, ACL_LIC_HEADER, ACL_LIC_HEADER_LENGTH - 1))
            {
                aLicInfo->mOffset = sOffset + ACL_LIC_HEADER_LENGTH;
                break;
            }
            else
            {
                /* loop */
            }
        }
    }
    else
    {
        /* Just return error code */
    }

    if(ACP_RC_IS_SUCCESS(sRC) && -1 == aLicInfo->mOffset)
    {
        /* Header not found */
        sRC = ACP_RC_ENOENT;
    }
    else
    {
        /* Error */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicReadFile(
    acl_lic_t* aLicInfo, acp_offset_t aOffset,
    void* aKey, void* aData)
{
    acp_offset_t sPos;
    acp_size_t sRead1 = 0;
    acp_size_t sRead2 = 0;
    acp_rc_t sRC;

    if((NULL == aLicInfo) || (NULL == aKey) || (NULL == aData))
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* Fall through */
    }

    sRC = acpFileSeek(&(aLicInfo->mFD),
                      aLicInfo->mOffset +
                      aOffset * (ACL_LIC_USER_KEYLENGTH +
                                 ACL_LIC_USER_DATALENGTH),
                      ACP_SEEK_SET,
                      &sPos);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = acpFileRead(&(aLicInfo->mFD), aKey,
                          ACL_LIC_USER_KEYLENGTH, &sRead1);
        if(ACP_RC_IS_SUCCESS(sRC) && (ACL_LIC_USER_KEYLENGTH == sRead1))
        {
            sRC = acpFileRead(&(aLicInfo->mFD), aData,
                              ACL_LIC_USER_DATALENGTH, &sRead2);
            if(ACP_RC_IS_SUCCESS(sRC) && ACL_LIC_USER_DATALENGTH == sRead2)
            {
                /* All done right */
            }
            else
            {
                sRC = ACP_RC_EACCES;
            }
        }
        else
        {
            /* Error */
        }
    }
    else
    {
        /* Error anyway */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicWriteFile(
    acl_lic_t* aLicInfo, acp_offset_t aOffset,
    void* aKey, void* aData)
{
    acp_offset_t sPos;
    acp_size_t sWritten1 = 0;
    acp_size_t sWritten2 = 0;
    acp_rc_t sRC;

    if((NULL == aLicInfo) || (NULL == aKey) || (NULL == aData))
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* Fall through */
    }

    sRC = acpFileSeek(&(aLicInfo->mFD),
                      aLicInfo->mOffset + aOffset *
                      (ACL_LIC_USER_KEYLENGTH + ACL_LIC_USER_DATALENGTH),
                      ACP_SEEK_SET,
                      &sPos);


    if(ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = acpFileWrite(&(aLicInfo->mFD), aKey,
                           ACL_LIC_USER_KEYLENGTH, &sWritten1);
        if(ACP_RC_IS_SUCCESS(sRC) && (ACL_LIC_USER_KEYLENGTH == sWritten1))
        {
            sRC = acpFileWrite(&(aLicInfo->mFD), aData,
                               ACL_LIC_USER_DATALENGTH, &sWritten2);
            if(ACP_RC_IS_SUCCESS(sRC) && ACL_LIC_USER_DATALENGTH == sWritten2)
            {
                /* All done right */
            }
            else
            {
                /* Cannot write to file */
                sRC = ACP_RC_EACCES;
            }
        }
        else
        {
            /* Error */
            sRC = ACP_RC_EACCES;
        }
    }
    else
    {
        /* Return Error Code */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicCloseFile(acl_lic_t* aLicInfo)
{
    acp_rc_t sRC;

    if(NULL == aLicInfo)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* Fall through */
    }

    sRC = acpFileSync(&(aLicInfo->mFD));
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = acpFileClose(&(aLicInfo->mFD));
    }
    else
    {
        (void)acpFileClose(&(aLicInfo->mFD));
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicFindKey(
    acl_lic_t* aLicInfo,
    acp_sint32_t* aIndex,
    void* aEncKey,
    acp_bool_t aUpdateIfExists)
{
    acp_rc_t sRC;
    acp_sint32_t i;
    acp_uint8_t sFileKey[ACL_LIC_USER_KEYLENGTH];
    acp_uint8_t sFileData[ACL_LIC_USER_DATALENGTH];
    acp_uint8_t sZero[ACL_LIC_USER_KEYLENGTH];

    (void)acpMemSet(sZero, 0, ACL_LIC_USER_KEYLENGTH);

    for(i = 1; i < ACL_LIC_NO_USER_DATA; i++)
    {
        sRC = aclLicReadFile(aLicInfo, i, sFileKey, sFileData);

        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* Fall through */
        }

        if(0 == acpMemCmp(sFileKey, aEncKey, ACL_LIC_USER_KEYLENGTH))
        {
            *aIndex = i;

            if(ACP_FALSE == aUpdateIfExists)
            {
                sRC = ACP_RC_EEXIST;
                break;
            }
            else
            {
                break;
            }
        }
        else if(0 == acpMemCmp(sFileKey, sZero, ACL_LIC_USER_KEYLENGTH))
        {
            *aIndex = i;
            break;
        }
        else
        {
            /* Loop */
        }
    }

    if(ACL_LIC_NO_USER_DATA == i)
    {
        /* Reached end of file
         * * No more space to store data */
        sRC = ACP_RC_ENOSPC;
    }
    else
    {
        /* Do nothing */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicInit(acl_lic_t* aLicInfo, acp_char_t* aPath)
{
    acp_rc_t sRC;
    acp_path_pool_t sPool;
    acp_stat_t sStat;
    acp_char_t* sExt = NULL;

    /* The license file must act
     * as a repository file and also as a license file */
    sRC = ((NULL == aLicInfo) || (NULL == aPath))?
        ACP_RC_EFAULT : ACP_RC_SUCCESS;

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        /* Set license information to all zeros */
        acpMemSet(&(aLicInfo->mLicenseInfo), 0, sizeof(aLicInfo->mLicenseInfo));

        acpPathPoolInit(&sPool);
        sExt = acpPathExt(aPath, &sPool);

        if (NULL == sExt)
        {
            /* Get system error code */
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /* Check if aPath ends with .dat */
            if (0 == acpCStrCmp(sExt, ACL_LIC_EXT, ACL_LIC_EXT_LEN))
            {
                /* Ends with extension */
                (void)acpCStrCpy(aLicInfo->mName, ACP_PATH_MAX_LENGTH,
                                 aPath, ACP_PATH_MAX_LENGTH);
            }
            else
            {
                /* Only path
                 * Defaulting to license file name as AltiLicense.dat */
                (void)acpSnprintf(aLicInfo->mName,
                                  ACP_PATH_MAX_LENGTH,
                                  "%s/%s", aPath, ACL_LIC_NAME);
            }

            /* Checks whether file exists */
            sRC = acpFileStatAtPath(aLicInfo->mName,
                                    &sStat,
                                    ACP_FALSE);
        }
        acpPathPoolFinal(&sPool);
    }
    else
    {
        /* Do nothing
         * just return error code */
    }

    return sRC;
}

static acp_rc_t aclLicMakeChunk(acl_lic_t* aLicInfo)
{
    acp_sint32_t i;
    acp_rc_t sRC;

    acp_uint8_t sKey[ACL_LIC_USER_KEYLENGTH];
    acp_uint8_t sData[ACL_LIC_USER_DATALENGTH];
    acp_size_t sWritten;

    acpMemSet(sKey, 0, ACL_LIC_USER_KEYLENGTH);
    acpMemSet(sData, 0, ACL_LIC_USER_DATALENGTH);

    for(i = 1; i < ACL_LIC_NO_USER_DATA; i++)
    {
        sRC = acpFileWrite(&(aLicInfo->mFD), sKey,
                           ACL_LIC_USER_KEYLENGTH, &sWritten);

        if(ACP_RC_IS_SUCCESS(sRC) && ACL_LIC_USER_KEYLENGTH == sWritten)
        {
            sRC = acpFileWrite(
                &(aLicInfo->mFD), sData, ACL_LIC_USER_DATALENGTH, &sWritten);

            if(ACP_RC_IS_SUCCESS(sRC) && ACL_LIC_USER_DATALENGTH == sWritten)
            {
                /* Loop */
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclLicCreateLicense(acl_lic_t* aLicInfo)
{
    acp_rc_t   sRC;
    acp_size_t sWritten = 0;

    sRC = (aLicInfo == NULL)?
        ACP_RC_EFAULT :
        acpFileOpen(&(aLicInfo->mFD),
                    aLicInfo->mName,
                    ACP_O_CREAT | ACP_O_RDWR,
                    ACP_S_IRUSR | ACP_S_IWUSR);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acp_uint8_t sKey[ACL_LIC_USER_KEYLENGTH];
        acp_uint8_t sData[ACL_LIC_USER_DATALENGTH];

        sRC = acpFileWrite(
            &(aLicInfo->mFD), ACL_LIC_HEADER,
            ACL_LIC_HEADER_LENGTH, &sWritten);

        if(ACP_RC_IS_SUCCESS(sRC) && ACL_LIC_HEADER_LENGTH == sWritten)
        {
            const acp_size_t sDateLength =
                sizeof(aLicInfo->mLicenseInfo.mLastRanDate);
            acp_time_t sTime = acpTimeNow();
            acp_time_exp_t sToday;
            acp_uint8_t sTimeString[
                sizeof(aLicInfo->mLicenseInfo.mLastRanDate) + 1];

            /* Format as YYYYMMDD */
            acpTimeGetLocalTime(sTime, &sToday);
            (void)acpSnprintf((acp_char_t*)sTimeString, sDateLength + 1,
                              "%04d%02d%02d",
                              sToday.mYear, sToday.mMonth, sToday.mDay);
            (void)acpMemCpy(aLicInfo->mLicenseInfo.mLastRanDate,
                            sTimeString, sDateLength);

            acpMemSet(sKey, 0, ACL_LIC_USER_KEYLENGTH);
            acpMemSet(sData, 0, ACL_LIC_USER_DATALENGTH);
            acpMemCpy(sKey, aLicInfo->mLicenseInfo.mKey,
                      ACL_LIC_KEYLENGTH);

            (void)aclCryptTEAEncipher(
                &(aLicInfo->mLicenseInfo), sKey, sData,
                ACL_LIC_DATALENGTH, ACL_LIC_KEYLENGTH);

            sRC = acpFileWrite(
                &(aLicInfo->mFD), sKey,
                ACL_LIC_USER_KEYLENGTH, &sWritten);

            sRC = acpFileWrite(
                &(aLicInfo->mFD), sData,
                ACL_LIC_USER_DATALENGTH, &sWritten);

            if(ACP_RC_IS_SUCCESS(sRC))
            {
                sRC = aclLicMakeChunk(aLicInfo);
            }
            else
            {
                /* Error */
            }
        }
        else
        {
            /* Return Error Code */
        }

        if(ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = acpFileClose(&(aLicInfo->mFD));
        }
        else
        {
            (void)acpFileClose(&(aLicInfo->mFD));
        }
    }
    else
    {
        /* Return Error Code */
    }

    return sRC;
}
