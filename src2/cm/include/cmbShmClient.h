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

#ifndef _O_CMB_SHM_CLIENT_H_
#define _O_CMB_SHM_CLIENT_H_ 1

#if !defined(CM_DISABLE_IPC)

/**** 공유 메모리 구조

  shm buffer ---->0 + -------------+ <----+  <---- start of a channel info
                    | channel info |      |
                    + -------------+
                    | channel info |      |
                    + -------------+
                    | channel info |      |
            ----->1 + -------------+ <----+  <---- start of a default channel buffer
                    | cm read buf  |      |
                    + -------------+
                    | cm write buf |      |
            ----->2 + -------------+ <----+  <---- start of a default channel buffer
                    | cm read buf  |      |
                    + -------------+
                    | cm write buf |      |
                    + -------------+
                    .......
            ----->n + -------------+ <----+  <---- n: the number of channels, start of a default channel buffer
                    | cm read buf  |      |
                    + -------------+
                    | cm write buf |      |
                    + -------------+

cf)
bug-27250 free Buf list can be crushed when client killed.
modified date: 20090128
as-is: extra spare buffer blocks added to the end of the buffer list
to-be: extra spare buffer blocks do not be prepared any more and
       the server do not manage the list as a linked list but just a array.

bug-29201
After bug-27250 is fixed, ShmHeader and ShmBufHeader structures are no needed.
that structures include information for extra-buffer.
 ***** */

/*
 * fix BUG-18834
 */
#define CMB_SHM_ALIGN                  (8)

/*
 * fix BUG-18830
 */
#define CMB_SHM_DEFAULT_BUFFER_COUNT   (2)

/*
 * bug-27250 free Buf list can be crushed when client killed
 * semaphore undo용 초기값 (10은 임의의 넉넉한 값임)
 */
#define CMB_SHM_SEMA_UNDO_VALUE        (10)

typedef struct cmbShmInfo
{
    acp_uint32_t mMaxChannelCount;         /* maximum number of ipc channel */
    acp_uint32_t mMaxBufferCount;          /* maximum number of ipc channel buffer */
    acp_uint32_t mUsedChannelCount;        /* channel count in used */
    acp_uint32_t mReserved;
} cmbShmInfo;

/*
 * Shared Memory에 존재
 */


typedef struct cmbShmChannelInfo    /* must be align by 8 byte */
{
    acp_uint32_t mTicketNum;        /* BUG-32398 타임스탬프에서 티켓번호로 변경 */
                                    /* New Connection시 설정. prevent IPC ghost-connection */
    acp_uint64_t mPID;              /* Client의 PID */
} cmbShmChannelInfo;


acp_uint64_t cmbShmGetFullSize(acp_uint32_t aChannelCount);

cmbShmChannelInfo* cmbShmGetChannelInfo(acp_sint8_t *aBaseBuf,
                                        acp_uint32_t aChannelID);

acp_sint8_t* cmnShmGetBuffer(acp_sint8_t *aBaseBuf,
                             acp_uint32_t aChannelCount,
                             acp_uint32_t aBufIdx);

acp_sint8_t* cmbShmGetDefaultServerBuffer(acp_sint8_t *aBaseBuf,
                                          acp_uint32_t aChannelCount,
                                          acp_uint32_t aChannelID);

acp_sint8_t* cmbShmGetDefaultClientBuffer(acp_sint8_t *aBaseBuf,
                                          acp_uint32_t aChannelCount,
                                          acp_uint32_t aChannelID);

ACI_RC cmbShmInitializeStatic();
ACI_RC cmbShmFinalizeStatic();

ACI_RC cmbShmAttach();
ACI_RC cmbShmDetach();

#endif

#endif
