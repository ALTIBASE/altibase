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

#ifndef _O_ULXMSGLOG_H_
#define _O_ULXMSGLOG_H_ 1

#include <acp.h>
#include <acl.h>

#define ULX_LOG_DIR_LEN     (1024)
#define ULX_XA_NAME_LEN     (256)

typedef struct ulxMsgLog
{
    acp_char_t mXaName[ULX_XA_NAME_LEN];
    acp_char_t mLogDir[ULX_LOG_DIR_LEN];
} ulxMsgLog;

void ulxMsgLogInitialize(ulxMsgLog     *aMsgLog);

acp_char_t *ulxMsgLogGetXaName(ulxMsgLog *aMsgLog);
void        ulxMsgLogSetXaName(ulxMsgLog  *aMsgLog,
                               acp_char_t *aXaName);

void ulxMsgLogGetLogFileName(ulxMsgLog    *aMsgLog,
                             acp_char_t   *aBuf,
                             acp_uint32_t aBufSize);
void ulxMsgLogParseOpenStr(ulxMsgLog  *aMsgLog,
                           acp_char_t *aOpenStr);



/* ----------------------------------------------------------------------
 *  XaLog Function (altibase_xa.log)
 * ----------------------------------------------------------------------*/
ACI_RC ulxLogTrace(ulxMsgLog *aLogObj, ... );

#endif /* _O_ULXMSGLOG_H_ */
