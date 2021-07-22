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

#ifndef _O_CM_CLIENT_H_
#define _O_CM_CLIENT_H_ 1

#include <acp.h>
#include <acl.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <aciVarString.h>

#include <idConfig.h>
#include <idTypesClient.h>

/*
 * CM모듈의 모든 HEADER 파일들
 *
 * CM모듈내에서는 이 HEADER 파일만 include 하면 됨
 */

#include <cmErrorCodeClient.h>
#include <cmuIpcPathClient.h>
#include <cmuIPCDAPathClient.h>
#include <cmuVersionClient.h>
#include <cmtDef.h>
#include <cmtDateTimeClient.h>
#include <cmtIntervalClient.h>
#include <cmtNumericClient.h>
#include <cmtVariableClient.h>
#include <cmtVariablePrivateClient.h>
#include <cmtBitClient.h>
#include <cmtNibbleClient.h>
#include <cmtLobLocatorClient.h>
#include <cmtAnyClient.h>
#include <cmtCollectionClient.h>
#include <cmtAnyPrivateClient.h>
#include <cmbBlockClient.h>
#include <cmbBlockMarshalClient.h>
#include <cmbPoolClient.h>
#include <cmbPoolPrivateClient.h>
#include <cmbShmClient.h>
#include <cmbShmIPCDAClient.h>
#include <cmmSessionClient.h>
#include <cmmSessionPrivateClient.h>
#include <cmnDefClient.h>
#include <cmpHeaderClient.h>
#include <cmnLinkClient.h>
#include <cmnLinkPrivateClient.h>
#include <cmnLinkPeerClient.h>
#include <cmnLinkListenClient.h>
#include <cmnSockClient.h>
#include <cmnDispatcherClient.h>
#include <cmnDispatcherPrivateClient.h>
#include <cmpDefBASEClient.h>
#include <cmpArgBASEClient.h>
#include <cmpDefDBClient.h>
#include <cmpArgDBClient.h>
#include <cmpDefRP.h>
#include <cmpArgRPClient.h>
#include <cmpMarshalClient.h>
#include <cmpDefClient.h>
#include <cmpModuleClient.h>
#include <cmiClient.h>
#include <cmxXidClient.h>
#include <cmnIB.h>
#include <cmnSockIBClient.h>

#endif
