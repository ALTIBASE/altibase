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

#ifndef  _O_ULN_TRACELOG_H_
#define  _O_ULN_TRACELOG_H_ 1

#define ULN_TRACE_LOG_PATH_LEN (2048)
#define ULN_TRACE_LOG_MAX_LINE_COUNT_PER_FILE 40000
#define ULN_TRACE_LOG_FILE_NAME "altibase_cli"

/* bug-35142 cli trace log */
enum
{
    ULN_TRACELOG_OFF    = 0,
    ULN_TRACELOG_LOW    = 1,
    ULN_TRACELOG_MID    = 2,
    ULN_TRACELOG_HIGH   = 3
};

// PROJ-1645 UL TraceLog File.
typedef struct ulnTraceLogFile
{
    acp_char_t       mCurrentFileName[ULN_TRACE_LOG_PATH_LEN];
    acp_uint32_t     mCurrentLogLineCnt;
    acp_uint32_t     mCurrentLogFileNo;
    acp_thr_rwlock_t mFileLatch;
    acp_sint32_t     mUlnTraceLevel; /* bug-35142 cli trace log */
}ulnTraceLogFile;

extern ulnTraceLogFile   gTraceLogFile;

ACP_EXTERN_C_BEGIN

void ULN_TRACE_LOG(ulnFnContext     *aCtx,
                   acp_sint32_t      aLevel,
                   void             *aData,
                   acp_sint32_t      aDataLen,
                   const acp_char_t *aFormat,
                   ...);

void ULN_TRACE_LOG_V(ulnFnContext     *aCtx,
                     acp_sint32_t      aLevel,
                     void             *aData,
                     acp_sint32_t      aDataLen,
                     const acp_char_t *aFormat,
                     va_list           aArgs);

void ulnInitTraceLog();
void ulnFinalTraceLog();

ACP_EXTERN_C_END

#endif /* _O_ULN_TRACELOG_H_ */
