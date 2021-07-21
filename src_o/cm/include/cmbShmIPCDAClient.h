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

#ifndef _O_CMB_SHM_DA_CLIENT_H_
#define _O_CMB_SHM_DA_CLIENT_H_ 1

#if !defined(CM_DISABLE_IPCDA)

/**** 공유 메모리 구조

  shm buffer ---->0 + -----------------------+ <----+  <---- start of a channel info
                    | channel info |      |
                    + ---------------------------+
                    | channel info |      |
                    + ---------------------------+
                    | channel info |      |
            ----->1 + -----------------------+ <----+  <---- start of a default channel buffer
                    | cm read buf  |      |
                    + ---------------------------+
                    | cm write buf |      |
                    + ---------------------------+
                    | cm simpleQuery buf |       |
            ----->2 + -----------------------+ <----+  <---- start of a default channel buffer
                    | cm read buf  |      |
                    + ---------------------------+
                    | cm write buf |      |
                    + ---------------------------+
                    | cm simpleQuery buf |       |
                  .......
            ----->n + ------------------------+ <----+  <---- n: the number of channels, start of a default channel buffer
                    | cm read buf  |      |
                    + ---------------------------+
                    | cm write buf |      |
                    + ---------------------------+
                    | cm simpleQuery buf |       |
                    + ---------------------------+
*/

/*
 * fix BUG-18834
 */
#define CMB_SHM_ALIGN                  (8)

/*
 * fix BUG-18830
 */
#define CMB_SHM_DEFAULT_BUFFER_COUNT    (2)
#define CMB_SHM_DEFAULT_DATABLOCK_COUNT (1)

/*
 * bug-27250 free Buf list can be crushed when client killed
 * semaphore undo용 초기값 (10은 임의의 넉넉한 값임)
 */
#define CMB_SHM_SEMA_UNDO_VALUE        (10)

#define CMB_INVALID_IPCDA_SHM_KEY      -1

typedef struct cmbShmIPCDAInfo
{
    acp_uint32_t        mMaxChannelCount;           /* maximum number of ipcda channel */
    acp_uint32_t        mMaxBufferCount;            /* maximum number of ipcda channel buffer */
    acp_uint32_t        mUsedChannelCount;          /* channel count used */

    acp_sint32_t        mClientCount;

    acp_sint32_t       *mSemChannelID;

    acp_key_t           mShmKey;
    acp_sint32_t        mShmID;
    acp_shm_t           mShm;
    acp_uint8_t        *mShmBuffer;

    acp_key_t           mShmKeyForSimpleQuery;
    acp_sint32_t        mShmIDForSimpleQuery;
    acp_shm_t           mShmForSimpleQuery;
    acp_uint8_t        *mShmBufferForSimpleQuery;

} cmbShmIPCDAInfo;

/* Shared Memory에 존재 */
typedef struct cmbShmIPCDAChannelInfo    /* must be align by 8 byte */
{
    acp_uint32_t mTicketNum;        /* BUG-32398 타임스탬프에서 티켓번호로 변경 */
                                    /* New Connection시 설정. prevent IPC ghost-connection */
    acp_uint64_t mPID;              /* Client의 PID */
} cmbShmIPCDAChannelInfo;

acp_uint64_t cmbShmIPCDAGetSize(acp_uint32_t aChannelCount);

acp_uint64_t cmbShmIPCDAGetDataBlockSize(acp_uint32_t aChannelCount);

cmbShmIPCDAChannelInfo* cmbShmIPCDAGetChannelInfo(acp_uint8_t *aBaseBuf, acp_uint32_t aChannelID);

acp_uint8_t* cmbShmIPCDAGetDefaultServerBuffer(acp_uint8_t *aBaseBuf,
                                               acp_uint32_t aChannelCount,
                                               acp_uint32_t aChannelID);

acp_uint8_t* cmbShmIPCDAGetDefaultClientBuffer(acp_uint8_t *aBaseBuf,
                                               acp_uint32_t aChannelCount,
                                               acp_uint32_t aChannelID);

ACI_RC cmbShmIPCDAInitializeStatic();
ACI_RC cmbShmIPCDAFinalizeStatic();

ACI_RC cmbShmIPCDAAttach();
void   cmbShmIPCDADetach();

ACP_INLINE acp_uint8_t* cmnShmIPCDAGetDataBlock(acp_uint8_t *aBaseBuf, acp_uint32_t aBufIdx)
{
    return (aBaseBuf + (cmbBlockGetIPCDASimpleQueryDataBlockSize() * aBufIdx));
}

ACP_INLINE acp_uint8_t* cmbShmIPCDAGetClientReadDataBlock(acp_uint8_t *aBaseBuf,
                                                          acp_uint32_t aChannelCount,
                                                          acp_uint32_t aChannelID)
{
    ACP_UNUSED(aChannelCount);
    return (cmnShmIPCDAGetDataBlock( aBaseBuf, (aChannelID * CMB_SHM_DEFAULT_DATABLOCK_COUNT)));
}

ACP_INLINE acp_uint8_t* cmbShmIPCDAGetClientWriteDataBlock(acp_uint8_t *aBaseBuf,
                                                           acp_uint32_t aChannelCount,
                                                           acp_uint32_t aChannelID)
{
    ACP_UNUSED(aBaseBuf);
    ACP_UNUSED(aChannelCount);
    ACP_UNUSED(aChannelID);
    return NULL;
}

#endif

#endif
