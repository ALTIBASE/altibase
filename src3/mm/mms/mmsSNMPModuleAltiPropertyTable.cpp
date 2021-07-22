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
#include <iduProperty.h>

/*
 * PROJ-2473 SNMP Áö¿ø
 */

/* 1.3.6.1.4.1.17180.2.1.2 AltiPropertyAlarmQueryTimeout */
idmModule mmsSNMPModule::mAltiPropertyAlarmQueryTimeout =
{
    (UChar *)"altiPropertyAlarmQueryTimeout", &mmsSNMPModule::mAltiPropertyEntry,
    NULL, NULL, NULL, NULL,
    2, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getAlarmQueryTimeout,
    mmsSNMPModule::setAlarmQueryTimeout
};

IDE_RC mmsSNMPModule::getAlarmQueryTimeout(const idmModule * /*aModule*/,
                                           const idmId     * /*aOID*/,
                                           UInt            *aType,
                                           void            *aValue,
                                           UInt            *aLength,
                                           UInt             aMaximum)
{
    SInt  sValue;
    SChar sString[32];
    UInt  sLength = 0;

    *aType  = IDM_TYPE_STRING;
    sValue  = iduProperty::getSNMPAlarmQueryTimeout();
    sLength = idlOS::snprintf(sString, sizeof(sString), "%"ID_INT32_FMT, sValue);

    IDE_TEST_RAISE(aMaximum < sLength, ERR_UNABLE_TO_GET_ATTRIBUTE);

    idlOS::memcpy(aValue, sString, sLength);
    *aLength = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmsSNMPModule::setAlarmQueryTimeout(idmModule   * /*aModule*/,
                                           const idmId * /*aOID*/,
                                           UInt         aType,
                                           const void  *aValue,
                                           UInt         aLength)
{
    SInt  sValue;
    SChar sString[32];

    IDE_TEST_RAISE(aType != IDM_TYPE_STRING || aLength >= sizeof(sString),
                   ERR_UNABLE_TO_SET_ATTRIBUTE);

    idlOS::memcpy(sString, aValue, aLength);
    sString[aLength] = '\0';

    sValue = idlOS::atoi(sString);
    if (sValue > 0)
    {
        sValue = 1;
    }
    else
    {
        /* Nothing */
    }

    iduProperty::setSNMPAlarmQueryTimeout(sValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_SET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Set_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 1.3.6.1.4.1.17180.2.1.3 AltiPropertyAlarmUtransTimeout */
idmModule mmsSNMPModule::mAltiPropertyAlarmUtransTimeout =
{
    (UChar *)"altiPropertyAlarmUtransTimeout", &mmsSNMPModule::mAltiPropertyEntry,
    NULL, NULL, NULL, NULL,
    3, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getAlarmUtransTimeout,
    mmsSNMPModule::setAlarmUtransTimeout
};

IDE_RC mmsSNMPModule::getAlarmUtransTimeout(const idmModule * /*aModule*/,
                                            const idmId     * /*aOID*/,
                                            UInt            *aType,
                                            void            *aValue,
                                            UInt            *aLength,
                                            UInt             aMaximum)
{
    SInt  sValue;
    SChar sString[32];
    UInt  sLength = 0;

    *aType  = IDM_TYPE_STRING;
    sValue  = iduProperty::getSNMPAlarmUtransTimeout();
    sLength = idlOS::snprintf(sString, sizeof(sString), "%"ID_INT32_FMT, sValue);

    IDE_TEST_RAISE(aMaximum < sLength, ERR_UNABLE_TO_GET_ATTRIBUTE);

    idlOS::memcpy(aValue, sString, sLength);
    *aLength = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmsSNMPModule::setAlarmUtransTimeout(idmModule   * /*aModule*/,
                                            const idmId * /*aOID*/,
                                            UInt         aType,
                                            const void  *aValue,
                                            UInt         aLength)
{
    SInt  sValue;
    SChar sString[32];

    IDE_TEST_RAISE(aType != IDM_TYPE_STRING || aLength >= sizeof(sString),
                   ERR_UNABLE_TO_SET_ATTRIBUTE);

    idlOS::memcpy(sString, aValue, aLength);
    sString[aLength] = '\0';

    sValue = idlOS::atoi(sString);
    if (sValue > 0)
    {
        sValue = 1;
    }
    else
    {
        /* Nothing */
    }

    iduProperty::setSNMPAlarmUtransTimeout(sValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_SET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Set_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 1.3.6.1.4.1.17180.2.1.4 AltiPropertyAlarmFetchTimeout */
idmModule mmsSNMPModule::mAltiPropertyAlarmFetchTimeout =
{
    (UChar *)"altiPropertyAlarmFetchTimeout", &mmsSNMPModule::mAltiPropertyEntry,
    NULL, NULL, NULL, NULL,
    4, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getAlarmFetchTimeout,
    mmsSNMPModule::setAlarmFetchTimeout
};

IDE_RC mmsSNMPModule::getAlarmFetchTimeout(const idmModule * /*aModule*/,
                                           const idmId     * /*aOID*/,
                                           UInt            *aType,
                                           void            *aValue,
                                           UInt            *aLength,
                                           UInt             aMaximum)
{
    SInt  sValue;
    SChar sString[32];
    UInt  sLength = 0;

    *aType  = IDM_TYPE_STRING;
    sValue  = iduProperty::getSNMPAlarmFetchTimeout();
    sLength = idlOS::snprintf(sString, sizeof(sString), "%"ID_INT32_FMT, sValue);

    IDE_TEST_RAISE(aMaximum < sLength, ERR_UNABLE_TO_GET_ATTRIBUTE);

    idlOS::memcpy(aValue, sString, sLength);
    *aLength = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmsSNMPModule::setAlarmFetchTimeout(idmModule   * /*aModule*/,
                                           const idmId * /*aOID*/,
                                           UInt         aType,
                                           const void  *aValue,
                                           UInt         aLength)
{
    SInt  sValue;
    SChar sString[32];

    IDE_TEST_RAISE(aType != IDM_TYPE_STRING || aLength >= sizeof(sString),
                   ERR_UNABLE_TO_SET_ATTRIBUTE);

    idlOS::memcpy(sString, aValue, aLength);
    sString[aLength] = '\0';

    sValue = idlOS::atoi(sString);
    if (sValue > 0)
    {
        sValue = 1;
    }
    else
    {
        /* Nothing */
    }

    iduProperty::setSNMPAlarmFetchTimeout(sValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_SET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Set_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 1.3.6.1.4.1.17180.2.1.5 AltiPropertyAlarmSessionFailureCount */
idmModule mmsSNMPModule::mAltiPropertyAlarmSessionFailureCount =
{
    (UChar *)"altiPropertyAlarmSessionFailureCount", &mmsSNMPModule::mAltiPropertyEntry,
    NULL, NULL, NULL, NULL,
    5, IDM_TYPE_STRING, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    mmsSNMPModule::getAlarmSessionFailureCount,
    mmsSNMPModule::setAlarmSessionFailureCount
};

IDE_RC mmsSNMPModule::getAlarmSessionFailureCount(const idmModule * /*aModule*/,
                                                  const idmId     * /*aOID*/,
                                                  UInt            *aType,
                                                  void            *aValue,
                                                  UInt            *aLength,
                                                  UInt             aMaximum)
{
    SInt  sValue;
    SChar sString[32];
    UInt  sLength = 0;

    *aType  = IDM_TYPE_STRING;
    sValue  = iduProperty::getSNMPAlarmSessionFailureCount();
    sLength = idlOS::snprintf(sString, sizeof(sString), "%"ID_INT32_FMT, sValue);

    IDE_TEST_RAISE(aMaximum < sLength, ERR_UNABLE_TO_GET_ATTRIBUTE);

    idlOS::memcpy(aValue, sString, sLength);
    *aLength = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_GET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmsSNMPModule::setAlarmSessionFailureCount(idmModule   * /*aModule*/,
                                                  const idmId * /*aOID*/,
                                                  UInt         aType,
                                                  const void  *aValue,
                                                  UInt         aLength)
{
    SInt  sValue;
    SChar sString[32];

    IDE_TEST_RAISE(aType != IDM_TYPE_STRING || aLength >= sizeof(sString),
                   ERR_UNABLE_TO_SET_ATTRIBUTE);

    idlOS::memcpy(sString, aValue, aLength);
    sString[aLength] = '\0';

    sValue = idlOS::atoi(sString);
    iduProperty::setSNMPAlarmSessionFailureCount(sValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_UNABLE_TO_SET_ATTRIBUTE)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Set_Attribute));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
