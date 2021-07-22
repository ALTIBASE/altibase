/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mmsSNMPModule.h>

#include <iduVersion.h>
#include <mmuProperty.h>
#include <mmtSessionManager.h>

/*
 * PROJ-2473 SNMP 지원
 */

time_t mmsSNMPModule::mStartTime;

/* 1.3.6.1.4.1.17180.3.1.1.2 altiStatusDBName */
idmModule mmsSNMPModule::mAltiStatusDBName =
{
    (UChar *)"altiStatusDBName", &mmsSNMPModule::mAltiStatusEntry,
    NULL, NULL, NULL, NULL,
    2, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getDBName,
    idm::unsupportedSet
};

IDE_RC mmsSNMPModule::getDBName(const idmModule * /*aModule*/,
                                const idmId     * /*aOID*/,
                                UInt            *aType,
                                void            *aValue,
                                UInt            *aLength,
                                UInt             aMaximum)
{
    SChar sValue[32];
    UInt  sLength;
    
    *aType = IDM_TYPE_STRING;

    /* BUGBUG 테이블에서 조회해야 정확함, 하지만 이상하게 쓰는 사용자는 없을듯... */
    sLength = idlOS::snprintf(sValue, ID_SIZEOF(sValue), mmuProperty::getDbName());

    IDE_TEST_RAISE(sLength > aMaximum, ERR_UNABLE_TO_GET_ATTRIBUTE);
    
    *aLength = sLength;
    idlOS::memcpy(aValue, sValue, sLength);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 1.3.6.1.4.1.17180.3.1.1.3 altiStatusDBVersion */
idmModule mmsSNMPModule::mAltiStatusDBVersion =
{
    (UChar *)"altiStatusDBVersion", &mmsSNMPModule::mAltiStatusEntry,
    NULL, NULL, NULL, NULL,
    3, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getDBVersion,
    idm::unsupportedSet
};

IDE_RC mmsSNMPModule::getDBVersion(const idmModule * /*aModule*/,
                                   const idmId     * /*aOID*/,
                                   UInt            *aType,
                                   void            *aValue,
                                   UInt            *aLength,
                                   UInt             aMaximum)
{
    SChar sValue[32];
    UInt  sLength;
    
    *aType = IDM_TYPE_STRING;
    
    sLength = idlOS::snprintf(sValue,
                              ID_SIZEOF(sValue),
                              "%s", iduVersionString);

    IDE_TEST_RAISE(sLength > aMaximum, ERR_UNABLE_TO_GET_ATTRIBUTE);
    
    *aLength = sLength;
    idlOS::memcpy(aValue, sValue, sLength);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 1.3.6.1.4.1.17180.3.1.1.4 altiStatusRunningTime */
idmModule mmsSNMPModule::mAltiStatusRunningTime =
{
    (UChar *)"altiStatusRunningTime", &mmsSNMPModule::mAltiStatusEntry,
    NULL, NULL, NULL, NULL,
    4, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    mmsSNMPModule::initRunningTime,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getRunningTime,
    idm::unsupportedSet
};

IDE_RC mmsSNMPModule::initRunningTime(idmModule * /*aModule*/)
{
    mStartTime = idlOS::time(NULL);

    return IDE_SUCCESS;
}

IDE_RC mmsSNMPModule::getRunningTime(const idmModule * /*aModule*/,
                                     const idmId     * /*aOID*/,
                                     UInt            *aType,
                                     void            *aValue,
                                     UInt            *aLength,
                                     UInt             aMaximum)
{
    SChar  sValue[32] = {'\0', };
    UInt   sLength  = 0;

    time_t sRunningTime;
    UInt   sDays    = 0;
    UInt   sHours   = 0;
    UInt   sMinutes = 0;
    UInt   sSeconds = 0;

    *aType = IDM_TYPE_STRING;

    sRunningTime = idlOS::time(NULL) - mStartTime;

    sDays  = sRunningTime / 86400;
    sRunningTime %= 86400;

    sHours = sRunningTime / 3600;
    sRunningTime %= 3600;

    sMinutes = sRunningTime / 60;
    sSeconds = sRunningTime % 60;

    if (sDays > 0)
    {
        sLength += idlOS::snprintf(&sValue[sLength],
                                   ID_SIZEOF(sValue) - sLength,
                                   "%"ID_INT32_FMT" days, ", sDays);
    }
    else
    {
        /* Nothing */
    }
    
    sLength += idlOS::snprintf(&sValue[sLength],
                               ID_SIZEOF(sValue) - sLength,
                               "%02"ID_INT32_FMT":%02"ID_UINT32_FMT":%02"ID_UINT32_FMT,
                               sHours, sMinutes, sSeconds);

    IDE_TEST_RAISE(sLength > aMaximum, ERR_UNABLE_TO_GET_ATTRIBUTE);
    
    *aLength = sLength;
    idlOS::memcpy(aValue, sValue, sLength);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 1.3.6.1.4.1.17180.3.1.1.5 altiStatusProcessID */
idmModule mmsSNMPModule::mAltiStatusProcessID =
{
    (UChar *)"altiStatusProcessID", &mmsSNMPModule::mAltiStatusEntry,
    NULL, NULL, NULL, NULL,
    5, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getProcessID,
    idm::unsupportedSet
};

IDE_RC mmsSNMPModule::getProcessID(const idmModule * /*aModule*/,
                                   const idmId     * /*aOID*/,
                                   UInt            *aType,
                                   void            *aValue,
                                   UInt            *aLength,
                                   UInt             aMaximum)
{
    SChar sValue[32];
    UInt  sLength;
    ULong sProcessID;
    
    *aType     = IDM_TYPE_STRING;
    sProcessID = idlOS::getpid();
    
    sLength = idlOS::snprintf(sValue,
                              ID_SIZEOF(sValue),
                              "%"ID_UINT64_FMT,
                              sProcessID);

    IDE_TEST_RAISE(sLength > aMaximum, ERR_UNABLE_TO_GET_ATTRIBUTE);
    
    *aLength = sLength;
    idlOS::memcpy(aValue, sValue, sLength);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 1.3.6.1.4.1.17180.3.1.1.6 altiStatusSessionCount */
idmModule mmsSNMPModule::mAltiStatusSessionCount =
{
    (UChar *)"altiStatusSessionCount", &mmsSNMPModule::mAltiStatusEntry,
    NULL, NULL, NULL, NULL,
    6, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getSessionCount,
    idm::unsupportedSet
};

IDE_RC mmsSNMPModule::getSessionCount(const idmModule * /*aModule*/,
                                      const idmId     * /*aOID*/,
                                      UInt            *aType,
                                      void            *aValue,
                                      UInt            *aLength,
                                      UInt             aMaximum)
{
    SChar sValue[32];
    UInt  sLength;
    UInt  sSessionCount;
    
    *aType        = IDM_TYPE_STRING;
    sSessionCount = mmtSessionManager::getSessionCount();

    sLength = idlOS::snprintf(sValue,
                              ID_SIZEOF(sValue),
                              "%"ID_UINT32_FMT,
                              sSessionCount);

    IDE_TEST_RAISE(sLength > aMaximum, ERR_UNABLE_TO_GET_ATTRIBUTE);
    
    *aLength = sLength;
    idlOS::memcpy(aValue, sValue, sLength);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
