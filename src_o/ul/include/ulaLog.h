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
 
#ifndef _O_ULA_LOG_MGR_H_
#define _O_ULA_LOG_MGR_H_ 1

#include <ulaErrorMgr.h>

#define ULA_LOG_FILENAME_LEN    (1024)

typedef struct ulaLogMgr ulaLogMgr;

ACI_RC ulaEnableLogging(const acp_char_t *aLogDirectory,
                        const acp_char_t *aLogFileName,
                        acp_uint32_t      aFileSize,
                        acp_uint32_t      aMaxFileNumber,
                        ulaErrorMgr      *aOutErrorMgr);
ACI_RC ulaDisableLogging(ulaErrorMgr *aOutErrorMgr);

ACI_RC ulaLogTrace(ulaErrorMgr       *aOutErrorMgr,
                   const acp_char_t  *aFormat,
                   ...);

ACI_RC ulaLogError(ulaErrorMgr *aOutErrorMgr);

ACI_RC ulaLogMgrCreateFileAndHeader(ulaLogMgr *aLogMgr);

#endif /* _O_ULA_LOG_MGR_H_ */
