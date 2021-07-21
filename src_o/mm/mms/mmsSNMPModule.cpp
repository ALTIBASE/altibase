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

/*
 * PROJ-2473 SNMP 지원
 */

idmModule *mmsSNMPModule::mModules[] =
{
    &mmsSNMPModule::mIso,
    &mmsSNMPModule::mOrg,
    &mmsSNMPModule::mDod,
    &mmsSNMPModule::mInternet,
    &mmsSNMPModule::mPrivate,
    &mmsSNMPModule::mEnterprises,

    /* 1.3.6.1.4.1.17180 */
    &mmsSNMPModule::mAltibase,

    /* 1.3.6.1.4.1.17180.1 Trap */

    /* 1.3.6.1.4.1.17180.2 */
    &mmsSNMPModule::mAltiPropertyTable,
    /* 1.3.6.1.4.1.17180.2.1 */
    &mmsSNMPModule::mAltiPropertyEntry,
    /* 1.3.6.1.4.1.17180.2.1.1 Index */
    /* 1.3.6.1.4.1.17180.2.1.2 모듈파일에 정의 */
    &mmsSNMPModule::mAltiPropertyAlarmQueryTimeout,
    /* 1.3.6.1.4.1.17180.2.1.3 모듈파일에 정의 */
    &mmsSNMPModule::mAltiPropertyAlarmUtransTimeout,
    /* 1.3.6.1.4.1.17180.2.1.4 모듈파일에 정의 */
    &mmsSNMPModule::mAltiPropertyAlarmFetchTimeout,
    /* 1.3.6.1.4.1.17180.2.1.5 모듈파일에 정의 */
    &mmsSNMPModule::mAltiPropertyAlarmSessionFailureCount,

    /* 1.3.6.1.4.1.17180.3 */
    &mmsSNMPModule::mAltiStatus,
    /* 1.3.6.1.4.1.17180.3.1 */
    &mmsSNMPModule::mAltiStatusTable,
    /* 1.3.6.1.4.1.17180.3.1.1 */
    &mmsSNMPModule::mAltiStatusEntry,
    /* 1.3.6.1.4.1.17180.3.1.1.1, Index */
    /* 1.3.6.1.4.1.17180.3.1.1.2, 모듈파일에 정의 */
    &mmsSNMPModule::mAltiStatusDBName,
    /* 1.3.6.1.4.1.17180.3.1.1.3, 모듈파일에 정의 */
    &mmsSNMPModule::mAltiStatusDBVersion,
    /* 1.3.6.1.4.1.17180.3.1.1.4, 모듈파일에 정의 */
    &mmsSNMPModule::mAltiStatusRunningTime,
    /* 1.3.6.1.4.1.17180.3.1.1.5, 모듈파일에 정의 */
    &mmsSNMPModule::mAltiStatusProcessID,
    /* 1.3.6.1.4.1.17180.3.1.1.6, 모듈파일에 정의 */
    &mmsSNMPModule::mAltiStatusSessionCount,
    NULL
};

/* iduModule 구조 참고
struct idmModule {
    UChar*           name;
    idmModule*       parent;
    idmModule*       child;
    idmModule*       brother;
    idmModule*       next;
    void*            data;
    oid              id;
    UInt             type;
    UInt             flag;
    UInt             depth;
    idmInitFunc      init;
    idmFinalFunc     final;
    idmGetNextIdFunc getNextId;
    idmGetFunc       get;
    idmSetFunc       set;
};
*/

/* 1. iso */
idmModule mmsSNMPModule::mIso =
{
    (UChar *)"iso", NULL,
    NULL, NULL, NULL, NULL,
    1, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3 org */
idmModule mmsSNMPModule::mOrg =
{
    (UChar *)"org", &mmsSNMPModule::mIso,
    NULL, NULL, NULL, NULL,
    3, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6 dod */
idmModule mmsSNMPModule::mDod =
{
    (UChar *)"dod", &mmsSNMPModule::mOrg,
    NULL, NULL, NULL, NULL,
    6, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1 internet */
idmModule mmsSNMPModule::mInternet =
{
    (UChar *)"internet", &mmsSNMPModule::mDod,
    NULL, NULL, NULL, NULL,
    1, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1.4 private */
idmModule mmsSNMPModule::mPrivate =
{
    (UChar *)"private", &mmsSNMPModule::mInternet,
    NULL, NULL, NULL, NULL,
    4, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1.4.1 enterprises */
idmModule mmsSNMPModule::mEnterprises =
{
    (UChar *)"enterprises", &mmsSNMPModule::mPrivate,
    NULL, NULL, NULL, NULL,
    1, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1.4.1.17180 altibase - 왜 17180일까??? */
idmModule mmsSNMPModule::mAltibase =
{
    (UChar *)"altibase", &mmsSNMPModule::mEnterprises,
    NULL, NULL, NULL, NULL,
    17180, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1.4.1.17180.2 altiPropertyTable */
idmModule mmsSNMPModule::mAltiPropertyTable =
{
    (UChar *)"altiPropertyTable", &mmsSNMPModule::mAltibase,
    NULL, NULL, NULL, NULL,
    2, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1.4.1.17180.2.1 altiPropertyEntry */
idmModule mmsSNMPModule::mAltiPropertyEntry =
{
    (UChar *)"altiPropertyEntry", &mmsSNMPModule::mAltiPropertyTable,
    NULL, NULL, NULL, NULL,
    1, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};


/* 1.3.6.1.4.1.17180.3 altiStatus */
idmModule mmsSNMPModule::mAltiStatus =
{
    (UChar *)"altiStatus", &mmsSNMPModule::mAltibase,
    NULL, NULL, NULL, NULL,
    3, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1.4.1.17180.3.1 altiStatusTable */
idmModule mmsSNMPModule::mAltiStatusTable =
{
    (UChar *)"altiStatusTable", &mmsSNMPModule::mAltiStatus,
    NULL, NULL, NULL, NULL,
    1, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};

/* 1.3.6.1.4.1.17180.3.1.1 altiStatusEntry */
idmModule mmsSNMPModule::mAltiStatusEntry =
{
    (UChar *)"altiStatusEntry", &mmsSNMPModule::mAltiStatusTable,
    NULL, NULL, NULL, NULL,
    1, IDM_TYPE_NONE, IDM_FLAG_HAVE_CHILD_FALSE, 0,
    idm::initDefault,
    idm::finalDefault,
    idm::getNextIdDefault,
    idm::unsupportedGet,
    idm::unsupportedSet
};
