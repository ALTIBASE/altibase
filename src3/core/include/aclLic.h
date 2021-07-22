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
 * $Id: aclLic.h 5585 2009-05-12 06:06:41Z djin $
 ******************************************************************************/

#if !defined(_O_ACL_LIC_H_)
#define _O_ACL_LIC_H_

/**
 * @file
 * @ingroup CoreLicense
 */

#include <aclCrypt.h>
#include <acpFile.h>

ACP_EXTERN_C_BEGIN

#define ACL_LIC_HEADER_LENGTH 128
#define ACL_LIC_HEADER                          \
    "Altibase License String........." /* 0 */  \
    "0123456789ABCDEF0123456789ABCDEF" /* 1 */  \
    "let f = Lambda x y. x * y;......" /* 2 */  \
    "f 32 4 = 128..............SoFar"  /* 3 */

#define ACL_LIC_NAME "AltiLicense.dat"

#define ACL_LIC_KEYLENGTH (16)
#define ACL_LIC_DATALENGTH (1024)

#define ACL_LIC_USER_KEYLENGTH (1024)
#define ACL_LIC_USER_DATALENGTH (1024)

#define ACL_LIC_NO_USER_DATA (512)

/* Product to license */
typedef struct acl_lic_product_t
{
    /* Exactly 32bytes */
    acp_uint8_t mName[28];
    acp_uint8_t mVersionMajor; /* Major Version */
    acp_uint8_t mVersionMinor; /* Minor Version */
    acp_uint8_t mVersionPatch; /* Patch Version */
    acp_uint8_t mVersionMisc;  /* Misc Version */
} acl_lic_product_t;

typedef struct acl_lic_data_t
{
    /* Version Information of aclLicense */
    acp_uint8_t mVersionMajor; /* Major Version */
    acp_uint8_t mVersionMinor; /* Minor Version */
    acp_uint8_t mVersionPatch; /* Patch Version */
    acp_uint8_t mVersionMisc;  /* Misc Version */
    acp_uint8_t mFill1[2];     /* Filler */
    /* So far 6bytes */

    /* Evaluation Information of aclLicense */
    acp_uint8_t mIsInstalled;  /* IsInstalled if not zero */
    acp_uint8_t mExpired;    /* Expired if not zero */
    /* So far 8bytes */

    /* License key again */
    acp_uint8_t mKey[32]; /* License Key */
    /* So far 40bytes */

    /* Command and Binaries */
    acp_uint8_t mArgv0[128];     /* Command */
    acp_uint8_t mProducts[256];  /* Binary List */
    /* So far 424bytes */

    /* Expiration Information */
    /* Expiration Date - YYYYMMDD */
    acp_uint8_t mExpireDate[8];
    acp_uint8_t mLastRanDate[8];
    /* Expiration Network Traffic.
     * Real number stored is m[0] + m[1] * 100 + m[2] * 10000 + ...*/
    acp_uint8_t mExpireTraffic[16];
    acp_uint8_t mLastRanTraffic[16];
    /* So far 448bytes */

    /* User Area */
    acp_uint8_t mUser[256];
    /* So far 704bytes */

    /* Hardware ID Type and ID */
    acp_uint8_t mID[128];

    /* Reserved Area */
    acp_uint8_t mReserved[40];
    /* So far 896bytes */

    /* Checksum */
    acp_uint8_t mChecksum[128]; /* Checksum */
    /* So far 1024bytes */
} acl_lic_data_t;

typedef acp_rc_t aclLoadLicenseFunc_t(void*, acp_size_t);
typedef acp_rc_t aclLoadFunc_t(void*, acp_size_t, void*, acp_size_t);

/* License Information of a shared object file */
typedef struct acl_lic_t
{
    acp_char_t mName[ACP_PATH_MAX_LENGTH]; /* Name of Shared Object */
    aclLoadLicenseFunc_t* mLoadLicenseFunc; /* Not Used */
    aclLoadFunc_t* mLoadFunc; /* Not Used */

    acl_lic_data_t mLicenseInfo;
    acp_file_t mFD;
    acp_offset_t mOffset;
} acl_lic_t;

/**
 * aclLicLoadLicense : Load License Information
 * @param aLicInfo License Information
 * @param aPath Path of Shared Object
 * @return ACP_RC_SUCCESS when successfully load data,
 * ACP_RC_ENOENT No shared object on aPath
 */
ACP_EXPORT acp_rc_t aclLicLoadLicense(
    acl_lic_t* aLicInfo,
    acp_char_t* aPath
    );

#define ACL_LIC_VALID        (0)
#define ACL_LIC_FILENOTFOUND (1)
#define ACL_LIC_INVALIDFILE  (2)
#define ACL_LIC_INVALIDKEY   (3)
#define ACL_LIC_IDNOTMATCH   (4)
#define ACL_LIC_UNKNOWN      (-1)

/**
 * aclLicLoadLicense : Load License Information
 * @param aLicInfo License Information
 * @param aValidity Whether License is valid
 * ACL_LIC_VALID when License is valid
 * ACL_LIC_KEYNOTMATCH when key in license file is invalid
 * ACL_LIC_IDNOTMATCH when hardware ID and ID in license does not match 
 * @return ACP_RC_SUCCESS when successfully load data,
 * ACP_RC_EOF Cannot find the License file header
 */
ACP_EXPORT acp_rc_t aclLicCheckValidity(
    acl_lic_t* aLicInfo,
    acp_sint32_t* aValidity
    );

/**
 * aclLicSaveLicense : Save License Information and set expired when expired
 * @param aLicInfo License Information
 * @return ACP_RC_SUCCESS when successfully load data,
 * ACP_RC_EINVAL Shared object is not licensed
 */
ACP_EXPORT acp_rc_t aclLicSaveLicense(
    acl_lic_t* aLicInfo
    );

/**
 * aclLicLoad : Load User Data saved with Key.
 * @param aLicInfo License Information
 * @param aKey Key Data to Retrive
 * @param aKeySize size of aKey in bytes max 1024
 * @param aData Buffer to store data according to aKey
 * @param aDataSize size of aData in bytes max 1024
 * @return ACP_RC_SUCCESS when successfully load data,
 * ACP_RC_ENOENT when aKey not exists
 * ACP_RC_EBUSY when license file is being used
 */
ACP_EXPORT acp_rc_t aclLicGetData(
    acl_lic_t* aLicInfo,
    void* aKey, acp_size_t aKeySize,
    void* aData, acp_size_t aDataSize
    );

/**
 * aclLicSetData : Save User Data with Key.
 * @param aLicInfo License Information
 * @param aKey Key Data to Retrive
 * @param aKeySize size of aKey in bytes max 1024
 * @param aData Buffer to store data according to aKey
 * @param aDataSize size of aData in bytes max 1024
 * @param aUpdateIfExists set true to update existing data
 * @return ACP_RC_SUCCESS when successfully saved data,
 * ACP_RC_EEXIST when aKey exists and aUpdateIfExists is ACP_FALSE
 * ACP_RC_EBUSY when license file is being used
 * ACP_RC_ENOSPC when no more space left
 */
ACP_EXPORT acp_rc_t aclLicSetData(
    acl_lic_t* aLicInfo,
    void* aKey, acp_size_t aKeySize,
    void* aData, acp_size_t aDataSize,
    acp_bool_t aUpdateIfExists
    );

/**
 * aclLicInstall : Install License to Shared Object
 * @param aLicInfo License Information
 * @param aKey Key Data to Retrive
 * @param aKeySize size of aKey in bytes max 16
 * @param aData Buffer to store data according to aKey
 * @param aDataSize size of aData in bytes max 1024
 * @return ACP_RC_SUCCESS when successfully installed license
 * ACP_RC_ENOENT when aKey not exists
 * ACP_RC_EBUSY when license file is being used
 */
ACP_EXPORT acp_rc_t aclLicInstall(
    acl_lic_t* aLicInfo,
    void* aKey, acp_size_t aKeySize,
    void* aData, acp_size_t aDataSize
    );

/**
 * aclLicIsInstalled : Check if the license is installed
 * @param aLicInfo License Information
 * @param aIsInstalled True if License is installed
 * @return ACP_RC_SUCCESS when aLicInfo is valid
 * ACP_RC_EINVAL when aLicInfo is not valid
 * ACP_RC_EBUSY when license file is being used
 */
ACP_EXPORT acp_rc_t aclLicIsInstalled(
    acl_lic_t* aLicInfo,
    acp_bool_t* aIsInstalled
    );

/**
 * aclLicSetExpired : Expire current license. Cannot be undone.
 * @param aLicInfo License Information
 * @return ACP_RC_SUCCESS when successfully expired
 * ACP_RC_EINVAL when aLicInfo is not valid
 * ACP_RC_EBUSY when license file is being used
 */
ACP_EXPORT acp_rc_t aclLicSetExpired(
    acl_lic_t* aLicInfo
    );

/**
 * aclLicIsExpired : Check License-Related Shared Object is Expired.
 * @param aLicInfo License Information
 * @param aExpired true when aLicInfo is expired, false otherwise
 * @return ACP_RC_SUCCESS when aLicInfo is valid
 * ACP_RC_EINVAL when aLicInfo is not valid
 * ACP_RC_EBUSY when license file is being used
 */
ACP_EXPORT acp_rc_t aclLicIsExpired(
    acl_lic_t* aLicInfo,
    acp_bool_t* aExpired
    );

/**
 * aclLicSetNetworkTraffic : Set Network traffic to aLicInfo
 * @param aLicInfo License Information
 * @param aTraffic Current network traffic
 * @return ACP_RC_SUCCESS when aLicInfo is valid and aTraffic successfully set.
 * ACP_RC_EINVAL when aLicInfo is not valid
 * ACP_RC_EBUSY when license file is being used
 */
ACP_EXPORT acp_rc_t aclLicSetNetworkTraffic(
    acl_lic_t* aLicInfo,
    acp_uint64_t aTraffic
    );

/**
 * aclLicSetNetworkTraffic : Set Network traffic to aLicInfo
 * @param aLicInfo License Information
 * @param aYear To store Year
 * @param aMonth To store Month (1..12)
 * @param aDay To store Day (1..31)
 * @param aPermanent To store Whether this license is permenant.
 * When this value is ACP_TRUE, aYear, aMonth, aDay is undecided.
 * @return ACP_RC_SUCCESS when aLicInfo is valid
 * ACP_RC_EINVAL when aLicInfo is not valid
 */
ACP_EXPORT acp_rc_t aclLicGetExpiryDate(
    acl_lic_t* aLicInfo,
    acp_sint32_t* aYear,
    acp_sint32_t* aMonth,
    acp_sint32_t* aDay,
    acp_bool_t* aPermanent);

/**
 * aclLicSetNetworkTraffic : Set Network traffic to aLicInfo
 * @param aLicInfo License Information
 * @param aProducts Pointer to store the product name
 * @param aSize Size of aProductName
 * @return ACP_RC_SUCCESS unless aLicInfo is NULL
 * ACP_RC_EINVAL when aLicInfo is NULL
 */
ACP_EXPORT acp_rc_t aclLicGetProducts(
    acl_lic_t* aLicInfo,
    acp_char_t* aProducts,
    acp_size_t aSize);

/** 
 * aclLicCheckHardwareID : Check Hardware ID matches
 * @param aLicInfo : License Information Structure
 * @param aEqual : point to store boolean value.
 * ACP_TRUE when aLicInfo->mID matches, ACP_FALSE otherwise
 */
ACP_EXPORT acp_rc_t aclLicCheckHardwareID(
    acl_lic_data_t* aLicInfo,
    acp_bool_t* aEqual);

/**
 * aclLicOpenFile : Open License File
 * @param aLicInfo : License Information Structure
 */
ACP_EXPORT acp_rc_t aclLicOpenFile(acl_lic_t* aLicInfo);

/**
 * aclLicReadFile : Read Key and Data with Offset
 * @param aLicInfo : License Information Structure
 * @param aOffset : Offset position
 * @param aKey : pointer to store Key
 * @param aData : pointer to store Data
 */
ACP_EXPORT acp_rc_t aclLicReadFile(
    acl_lic_t* aLicInfo, acp_offset_t aOffset,
    void* aKey, void* aData);

/**
 * aclLicWriteFile : Write Key and Data File with Offset
 * @param aLicInfo : License Information Structure
 * @param aOffset : Offset position
 * @param aKey : pointer that stores Key
 * @param aData : pointer that stores Data
 */
ACP_EXPORT acp_rc_t aclLicWriteFile(
    acl_lic_t* aLicInfo, acp_offset_t aOffset,
    void* aKey, void* aData);

/**
 * aclLicCloseFile : Close License File
 * @param aLicInfo : License Information Structure
 */
ACP_EXPORT acp_rc_t aclLicCloseFile(acl_lic_t* aLicInfo);

/**
 * aclLicFindKey : Find Key Position in License File
 * @param aLicInfo : License Information Structure
 * @param aIndex : Pointer to store offset
 * @param aEncKey : Encrypted Key to find
 * @param aUpdateIfExists : Update when exitsts
 */
ACP_EXPORT acp_rc_t aclLicFindKey(
    acl_lic_t* aLicInfo,
    acp_sint32_t* aIndex,
    void* aEncKey,
    acp_bool_t aUpdateIfExists);

/**
 * aclLicInit : Init License
 * @param aLicInfo : License Information Structure
 * @param aPath : Path of License File
 */
ACP_EXPORT acp_rc_t aclLicInit(acl_lic_t* aLicInfo, acp_char_t* aPath);

/**
 * aclLicCreateLicense : Create a New License File
 * @param aLicInfo : License Information Structure
 */
ACP_EXPORT acp_rc_t aclLicCreateLicense(acl_lic_t* aLicInfo);

ACP_EXTERN_C_END

#endif
