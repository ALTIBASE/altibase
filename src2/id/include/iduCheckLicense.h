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
 

/*****************************************************************************
 * $Id: iduCheckLicense.h 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <acp.h>
#include <idTypes.h>

#ifndef _O_IDU_CHECK_LICENSE_H_
#define _O_IDU_CHECK_LICENSE_H_ 1

/* For open source */

#define COMMUNITYEDITION    ((UChar)(0))
#define STANDARDEDITION     ((UChar)(1))
#define ENTERPRISEEDITION   ((UChar)(2))
#define TRIALEDITION        ((UChar)(3))

typedef struct iduLicenseHeader
{
    UChar mType;
} iduLicenseHeader;

struct iduLicense
{
    iduLicense();

    static IDE_RC  initializeStatic(void);

    IDE_RC  load(const SChar* = NULL);
    IDE_RC  applyMax(void);

    iduLicenseHeader    mLicense;           /* License */
    SChar               mPath[ID_MAX_FILE_NAME];    /* Pathname of license file */
    ULong               mMemMax;            /* MEM_MAX_DB_SIZE */
    ULong               mDiskMax;           /* DISK_MAX_DB_SIZE */
    ULong               mExpire;
    ULong               mIssued;
    UInt                mMaxCores;
    UInt                mMaxNUMAs;

    static acp_mac_addr_t      mMacAddr[ID_MAX_HOST_ID_NUM];
    static UInt                mNoMac;
};

class iduCheckLicense
{
public:
    static IDE_RC initializeStatic(void);
    static IDE_RC check(void);
    static IDE_RC update(void);
    static IDE_RC getHostUniqueString(SChar*, UInt);
    static inline UInt getMaxCores(void)
    {
        return mLicense.mMaxCores;
    }
    static inline UChar getMaxNUMAs(void)
    {
        return mLicense.mMaxNUMAs;
    }

    static iduLicense mLicense;
};

#endif

