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

#ifndef _ULN_ENVIRONMENT_H_
#define _ULN_ENVIRONMENT_H_ 1

struct ulnEnv
{
    ulnObject mObj;

    acp_uint32_t mDbcCount;          /* ENV가 가지고 있는 DBC의 갯수  */
    acp_list_t   mDbcList;           /* DBC의 리스트 헤드 */

    //acp_list_t    mDataSourceList;    /* DataSource 의 리스트 헤드 */
    //acp_uint32_t  mDataSourceCount;   /* ENV가 가지고 있는 DataSource 의 갯수 */

    acp_sint32_t mOdbcVersion;       /* SQL_ATTR_ODBC_VERSION */
    acp_uint32_t mConnPooling;       /* SQL_ATTR_CONNECTION_POOLING */
    acp_uint32_t mConnPoolMatch;     /* SQL_ATTR_CP_MATCH */
    acp_sint32_t mOutputNts;         /* SQL_ATTR_OUTPUT_NTS */

    acp_uint64_t mUlnVersion;

    /* BUG-35332 The socket files can be moved */
    ulnProperties mProperties;       /* Server Properties */

    /* PROJ-2733-DistTxInfo */
    acp_uint64_t mSCN;

    /* Sharding Context 관련은 구조체 마지막에 유지. */
    ulsdModule  *mShardModule;
};

/*
 * Function Declarations
 */

ACI_RC ulnEnvCreate(ulnEnv **aOutputEnv);
ACI_RC ulnEnvDestroy(ulnEnv *aEnv);
ACI_RC ulnEnvInitialize(ulnEnv *aEnv);

ACI_RC ulnEnvAddDbc(ulnEnv *aEnv, ulnDbc *aDbc);
ACI_RC ulnEnvRemoveDbc(ulnEnv *aEnv, ulnDbc *aDbc);

acp_uint32_t ulnEnvGetDbcCount(ulnEnv *aEnv);

acp_uint32_t ulnEnvGetOdbcVersion(ulnEnv *aEnv);
ACI_RC       ulnEnvSetOdbcVersion(ulnEnv *aEnv, acp_uint32_t aVersion);

ACI_RC ulnEnvSetOutputNts(ulnEnv *aEnv, acp_sint32_t aNts);
ACI_RC ulnEnvGetOutputNts(ulnEnv *aEnv, acp_sint32_t *aNts);

ACI_RC       ulnEnvSetUlnVersion(ulnEnv *aEnv, acp_uint64_t aVersion);
acp_uint64_t ulnEnvGetUlnVersion(ulnEnv *aEnv);

#endif /* _ULN_ENVIRONMENT_H_ */
