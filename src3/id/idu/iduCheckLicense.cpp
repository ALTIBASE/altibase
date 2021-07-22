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
 * $Id: iduCheckLicense.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <acp.h>
#include <idl.h>
#include <idErrorCode.h>
#include <ideCallback.h>
#include <ideErrorMgr.h>
#include <idsDES.h>
#include <iduCheckLicense.h>
#include <ideLog.h>
#include <idp.h>
#include <iduVersion.h>
#include <idt.h>

static const SChar * const DEFAULT_LICENSE = IDL_FILE_SEPARATORS"conf"IDL_FILE_SEPARATORS"license";

acp_mac_addr_t  iduLicense::mMacAddr[ID_MAX_HOST_ID_NUM];
UInt            iduLicense::mNoMac;
iduLicense      iduCheckLicense::mLicense;

iduLicense::iduLicense()
{
    mMemMax  = ID_ULONG(1) * 1024 * 1024 * 1024 * 1024;
    mDiskMax = ID_ULONG(5) * 1024 * 1024 * 1024 * 1024;

    idlOS::memset(&mLicense, 0, sizeof(mLicense));
    idlOS::memset(mPath,     0, ID_MAX_FILE_NAME);
}

IDE_RC iduLicense::initializeStatic(void)
{
    acp_rc_t    sRC;

    UInt        i;
    SChar       sTmpBuffer[256];
    SChar       sMsgBuffer[16384];

    sRC = acpSysGetMacAddress(mMacAddr, ID_MAX_HOST_ID_NUM, &mNoMac);
    IDE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), EGETMAC);
    IDE_TEST_RAISE(mNoMac == 0, EGETMAC);

    idlOS::strcpy(sMsgBuffer, "Enumerating authorization keys\n");
    for(i = 0; i < mNoMac; i++)
    {
        idlOS::snprintf(sTmpBuffer, sizeof(sTmpBuffer),
                        "\tMAC Address [%3d] : [%02X:%02X:%02X:%02X:%02X:%02X]\n",
                        i,
                        (UChar)mMacAddr[i].mAddr[0],
                        (UChar)mMacAddr[i].mAddr[1],
                        (UChar)mMacAddr[i].mAddr[2],
                        (UChar)mMacAddr[i].mAddr[3],
                        (UChar)mMacAddr[i].mAddr[4],
                        (UChar)mMacAddr[i].mAddr[5]);
        idlOS::strcat(sMsgBuffer, sTmpBuffer);
    }

    ideLog::log(IDE_SERVER_0, sMsgBuffer);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EGETMAC)
    {
        IDE_CALLBACK_SEND_MSG("[ERROR] Unable to detect MAC addresses!");
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduLicense::load(const SChar* aPath)
{
    /*For open source*/
    PDL_UNUSED_ARG( aPath );

    mMemMax   = ID_ULONG_MAX;
    mDiskMax  = ID_ULONG_MAX;
    mExpire   = ID_ULONG_MAX;   
    mIssued   = 0;             
    mMaxCores = ACP_PSET_MAX_CPU;
    mMaxNUMAs = IDT_MAX_NUMA_NODES;
    IDE_CALLBACK_SEND_SYM( "Server is Open Edition.\n" );
    ideLog::log( IDE_SERVER_0, "MEM_MAX_DB_SIZE is UNLIMITED." );
    ideLog::log( IDE_SERVER_0, "DISK_MAX_DB_SIZE is UNLIMITED." );
    return IDE_SUCCESS;
}

IDE_RC iduLicense::applyMax(void)
{
    SChar sCharVal[32];
    SChar sMsg[128];
    ULong sMemMax;
    ULong sDiskMax;

    IDE_TEST(idp::read("MEM_MAX_DB_SIZE", &sMemMax) != IDE_SUCCESS);
    IDE_TEST(idp::read("DISK_MAX_DB_SIZE", &sDiskMax) != IDE_SUCCESS);

    IDE_TEST_RAISE(sMemMax > mMemMax, EMEMMAXEXCEED);

    idlOS::snprintf(sCharVal, sizeof(sCharVal), "%llu", mDiskMax);
    IDE_TEST(idp::updateForce("DISK_MAX_DB_SIZE", sCharVal) != IDE_SUCCESS);
    IDE_TEST(idtCPUSet::relocateCPUs(mMaxCores, mMaxNUMAs) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EMEMMAXEXCEED)
    {
        idlOS::snprintf(sMsg, sizeof(sMsg),
                        "MEM_MAX_DB_SIZE(%llu) exceed license limitation(%llu).\n",
                        sMemMax, mMemMax);
        IDE_CALLBACK_SEND_SYM(sMsg);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduCheckLicense::initializeStatic(void)
{
    return iduLicense::initializeStatic();
}

IDE_RC iduCheckLicense::check()
{
    IDE_TEST(mLicense.load() != IDE_SUCCESS);
    IDE_TEST(mLicense.applyMax() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduCheckLicense::update()
{
    iduLicense  sLicense;
    SChar       sMsg[128];

    IDE_TEST(sLicense.load() != IDE_SUCCESS);

    IDE_TEST_RAISE(sLicense.mLicense.mType == TRIALEDITION,
                   ECANNOTUPDATETRIAL);
    IDE_TEST_RAISE(
        (sLicense.mLicense.mType == STANDARDEDITION) &&
        (mLicense.mLicense.mType == ENTERPRISEEDITION),
        ECANNOTUPDATESTANDARD);

    IDE_TEST_RAISE(sLicense.mMemMax   < mLicense.mMemMax  , EMEMSMALL);
    IDE_TEST_RAISE(sLicense.mDiskMax  < mLicense.mDiskMax , EDISKSMALL);
    IDE_TEST_RAISE(sLicense.mExpire   < mLicense.mExpire  , EEXPIREPAST);
    IDE_TEST_RAISE(sLicense.mMaxCores < mLicense.mMaxCores, ELESSCORES);
    IDE_TEST_RAISE(sLicense.applyMax() != IDE_SUCCESS     , EAPPLYFAIL);

    if(sLicense.mMaxCores != mLicense.mMaxCores)
    {
        idlOS::snprintf(sMsg, sizeof(sMsg),
                        "Max CPU core count changed from %d to %d.",
                        mLicense.mMaxCores, sLicense.mMaxCores);
        IDE_CALLBACK_SEND_MSG(sMsg);
        IDE_CALLBACK_SEND_MSG("It is recommended to restart the server.");
    }

    idlOS::memcpy(&mLicense, &sLicense, sizeof(iduLicense));

    if(mLicense.mLicense.mType == ENTERPRISEEDITION)
    {
        IDE_CALLBACK_SEND_MSG("MEM_MAX_DB_SIZE can be enlarged to UNLIMITED.");
        IDE_CALLBACK_SEND_MSG("New DISK_MAX_DB_SIZE is UNLIMITED.");
    }
    else
    {
        idlOS::snprintf(sMsg, sizeof(sMsg),
                        "MEM_MAX_DB_SIZE can be enlarged to %lluGB.",
                        mLicense.mMemMax / 1024 / 1024 / 1024);
        IDE_CALLBACK_SEND_MSG(sMsg);
        idlOS::snprintf(sMsg, sizeof(sMsg),
                        "New DISK_MAX_DB_SIZE is %lluGB.",
                        mLicense.mDiskMax / 1024 / 1024 / 1024);
        IDE_CALLBACK_SEND_MSG(sMsg);
    }

    idlOS::snprintf(sMsg, sizeof(sMsg),
                    "New expiry date is %llu.",
                    mLicense.mExpire);
    IDE_CALLBACK_SEND_MSG(sMsg);

    IDE_CALLBACK_SEND_MSG("License update successfully!");

    return IDE_SUCCESS;

    IDE_EXCEPTION(ECANNOTUPDATETRIAL)
    {
        IDE_CALLBACK_SEND_MSG("[ERROR] Cannot update to trial edition.");
    }

    IDE_EXCEPTION(ECANNOTUPDATESTANDARD)
    {
        IDE_CALLBACK_SEND_MSG("[ERROR] Cannot update from enterprise edition "
                              "to standard edition.");
    }

    IDE_EXCEPTION(EMEMSMALL)
    {
        if(mLicense.mMemMax == ID_ULONG_MAX)
        {
            idlOS::snprintf(sMsg, sizeof(sMsg),
                            "[ERROR] New MEM_MAX_DB_SIZE is smaller!\n"
                            "\tCurrent value : UNLIMITED\n"
                            "\tNew value     : %lluGB\n",
                            sLicense.mMemMax / 1024 / 1024 / 1024);
        }
        else
        {
            idlOS::snprintf(sMsg, sizeof(sMsg),
                            "[ERROR] New MEM_MAX_DB_SIZE is smaller!\n"
                            "\tCurrent value : %lluGB\n"
                            "\tNew value     : %lluGB\n",
                            mLicense.mMemMax / 1024 / 1024 / 1024,
                            sLicense.mMemMax / 1024 / 1024 / 1024);
        }
        IDE_CALLBACK_SEND_MSG(sMsg);
    }

    IDE_EXCEPTION(EDISKSMALL)
    {
        if(mLicense.mDiskMax == ID_ULONG_MAX)
        {
            idlOS::snprintf(sMsg, sizeof(sMsg),
                            "[ERROR] New DISK_MAX_DB_SIZE is smaller!\n"
                            "\tCurrent value : UNLIMITED\n"
                            "\tNew value     : %lluGB\n",
                            sLicense.mDiskMax / 1024 / 1024 / 1024);
        }
        else
        {
            idlOS::snprintf(sMsg, sizeof(sMsg),
                            "[ERROR] New DISK_MAX_DB_SIZE is smaller!\n"
                            "\tCurrent value : %lluGB\n"
                            "\tNew value     : %lluGB\n",
                            mLicense.mDiskMax / 1024 / 1024 / 1024,
                            sLicense.mDiskMax / 1024 / 1024 / 1024);
        }
        IDE_CALLBACK_SEND_MSG(sMsg);
    }

    IDE_EXCEPTION(EEXPIREPAST)
    {
        idlOS::snprintf(sMsg, sizeof(sMsg),
                        "[ERROR] New expiry date is improper!\n"
                        "\tCurrent value : %d\n"
                        "\tNew value     : %d\n",
                        mLicense.mExpire,
                        sLicense.mExpire);
        IDE_CALLBACK_SEND_MSG(sMsg);
    }

    IDE_EXCEPTION(ELESSCORES)
    {
        idlOS::snprintf(sMsg, sizeof(sMsg),
                        "[ERROR] New core count is smaller!\n"
                        "\tCurrent value : %llu\n"
                        "\tNew value     : %llu\n",
                        mLicense.mMaxCores,
                        sLicense.mMaxCores);
        IDE_CALLBACK_SEND_MSG(sMsg);
    }

    IDE_EXCEPTION(EAPPLYFAIL)
    {
        IDE_CALLBACK_SEND_MSG("[ERROR] Cannot apply new license!\n");
    }

    IDE_EXCEPTION_END;
    IDE_CALLBACK_SEND_MSG("License update failed!\n");
    return IDE_FAILURE;
}

IDE_RC iduCheckLicense::getHostUniqueString(SChar* aBuf, UInt aBufSize)
{
    idlOS::snprintf(aBuf, aBufSize,
                    "%02X%02X%02X%02X%02X%02X",
                    (UInt)(mLicense.mMacAddr[0].mAddr)[0],
                    (UInt)(mLicense.mMacAddr[0].mAddr)[1],
                    (UInt)(mLicense.mMacAddr[0].mAddr)[2],
                    (UInt)(mLicense.mMacAddr[0].mAddr)[3],
                    (UInt)(mLicense.mMacAddr[0].mAddr)[4],
                    (UInt)(mLicense.mMacAddr[0].mAddr)[5]);

    return IDE_SUCCESS;
}

