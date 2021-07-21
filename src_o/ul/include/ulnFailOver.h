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

#ifndef  _O_ULN_FAILOVER_H_
#define  _O_ULN_FAILOVER_H_ 1

#include <uln.h>



/* BUG-31390 Failover info for v$session */
typedef enum
{
    ULN_FAILOVER_TYPE_CTF = 0,
    ULN_FAILOVER_TYPE_STF = 1
} ulnFailoverType;

typedef enum
{
    ULN_FAILOVER_CALLBACK_OUT_STATE = 0,
    ULN_FAILOVER_CALLBACK_IN_STATE
} ulnFailoverCallbackState;

typedef enum
{
    ULN_FAILOVER_SUSPEND_OFF_STATE = 0,
    ULN_FAILOVER_SUSPEND_ON_STATE
} ulnFailoverSuspendState;                  /* BUG-47131 샤드 All meta 환경에서 Client failover 시 hang 발생 */

typedef struct ulnFailoverServerInfo
{
    acp_char_t       *mHost;
    acp_uint16_t      mPort;
    acp_char_t       *mDBName;
    acp_sint32_t      mDBNameLen;
} ulnFailoverServerInfo;

struct  ulnFnContext;



acp_bool_t ulnFailoverIsOn(ulnDbc *aDbc);

ACI_RC ulnFailoverConnect( ulnFnContext          *aFnContext,
                           ulnFailoverType        aFailoverType,
                           ulnFailoverServerInfo *aNewServerInfo );

/* BUG-46092 */
acp_bool_t ulnDiagRecIsNeedFailover(ulnObject *aObject);

ACI_RC ulnFailoverXaReOpen(ulnDbc *  aDbc);

ACI_RC ulnFailoverCreateServerInfo( ulnFailoverServerInfo **aServerInfo,
                                    acp_char_t             *aHost,
                                    acp_uint16_t            aPort,
                                    acp_char_t             *aDBName );

void ulnFailoverDestroyServerInfo(ulnFailoverServerInfo *aServerInfo);

ACI_RC ulnFailoverCreatePrimaryServerInfo( ulnFnContext           *aFnContext,
                                           ulnFailoverServerInfo **aServerInfo );

void ulnFailoverClearServerList(ulnDbc *aDbc);

void ulnFailoverInitialize(ulnDbc *aDbc);

void ulnFailoverFinalize(ulnDbc *aDbc);

ACI_RC ulnFailoverBuildServerList(ulnFnContext *aFnContext);

ACI_RC ulnFailoverDoCTF(ulnFnContext *aContext);

ACI_RC ulnFailoverDoSTF(ulnFnContext *aContext);

void ulnFailoverAddServer( ulnDbc                *aDbc,
                           ulnFailoverServerInfo *aServerInfo );

ACI_RC ulnFailoverAddServerList( ulnFnContext *aFC,
                                 acp_char_t   *aAlternateServerList );

SQLUINTEGER ulnDummyFailoverCallbackFunction(SQLHDBC      aDBC,
                                             void        *aAppContext,
                                             SQLUINTEGER  aFailoverEvent);



#endif /* _O_ULN_FAILOVER_H_ */

