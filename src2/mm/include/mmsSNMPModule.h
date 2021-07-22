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

#ifndef _O_MMS_SNMP_MODULES_H_
#define _O_MMS_SNMP_MODULES_H_ 1

#include <ide.h>
#include <idm.h>

#define IDM_GET_ARGUMENTS     \
    const idmModule *,        \
    const idmId     *,        \
    UInt            *aType,   \
    void            *aValue,  \
    UInt            *aLength, \
    UInt             aMaximum

#define IDM_SET_ARGUMENTS \
    idmModule   *,        \
    const idmId *,        \
    UInt         aType,   \
    const void  *aValue,  \
    UInt         aLength       

/*
 * PROJ-2473 SNMP Áö¿ø
 */

class mmsSNMPModule
{
private:
    static idmModule *mModules[];

    static idmModule mIso;
    static idmModule mOrg;
    static idmModule mDod;
    static idmModule mInternet;
    static idmModule mPrivate;
    static idmModule mEnterprises;

    static idmModule mAltibase;

    static idmModule mAltiPropertyTable;
    static idmModule mAltiPropertyEntry;
    static idmModule mAltiPropertyAlarmQueryTimeout;
    static idmModule mAltiPropertyAlarmUtransTimeout;
    static idmModule mAltiPropertyAlarmFetchTimeout;
    static idmModule mAltiPropertyAlarmSessionFailureCount;

    static idmModule mAltiStatus;
    static idmModule mAltiStatusTable;
    static idmModule mAltiStatusEntry;
    static idmModule mAltiStatusDBName;
    static idmModule mAltiStatusDBVersion;
    static idmModule mAltiStatusRunningTime;
    static idmModule mAltiStatusProcessID;
    static idmModule mAltiStatusSessionCount;

    /* AltiStatusTable */
    static time_t mStartTime;

    static IDE_RC getDBName      (IDM_GET_ARGUMENTS);
    static IDE_RC getDBVersion   (IDM_GET_ARGUMENTS);
    static IDE_RC initRunningTime(idmModule *);
    static IDE_RC getRunningTime (IDM_GET_ARGUMENTS);
    static IDE_RC getProcessID   (IDM_GET_ARGUMENTS);
    static IDE_RC getSessionCount(IDM_GET_ARGUMENTS);

    /* AltiPropertyTable */
    static IDE_RC getAlarmQueryTimeout (IDM_GET_ARGUMENTS);
    static IDE_RC getAlarmUtransTimeout(IDM_GET_ARGUMENTS);
    static IDE_RC getAlarmFetchTimeout (IDM_GET_ARGUMENTS);
    static IDE_RC getAlarmSessionFailureCount(IDM_GET_ARGUMENTS);

    static IDE_RC setAlarmQueryTimeout (IDM_SET_ARGUMENTS);
    static IDE_RC setAlarmUtransTimeout(IDM_SET_ARGUMENTS);
    static IDE_RC setAlarmFetchTimeout (IDM_SET_ARGUMENTS);
    static IDE_RC setAlarmSessionFailureCount(IDM_SET_ARGUMENTS);

public:
    static idmModule **getModules()
    {
        return mModules;
    }
};

#endif /* _O_MMS_SNMP_MODULES_H_ */
