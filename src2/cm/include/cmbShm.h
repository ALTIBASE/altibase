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

#ifndef _O_CMB_SHM_H_
#define _O_CMB_SHM_H_ 1

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
       the server do not manage the list as a linked list but just a array

bug-29201
After bug-27250 is fixed, ShmHeader and ShmBufHeader structures are no needed.
that structures include information for extra-buffer.
 ***** */

//fix BUG-18834
#define CMB_SHM_ALIGN                  (8)

//fix BUG-18830
#define CMB_SHM_DEFAULT_BUFFER_COUNT   (2)

// bug-27250 free Buf list can be crushed when client killed
// semaphore undo용 초기값 (10은 임의의 넉넉한 값임)
#define CMB_SHM_SEMA_UNDO_VALUE        10

typedef struct cmbShmInfo
{
    UInt  mMaxChannelCount;         // maximum number of ipc channel
    UInt  mMaxBufferCount;          // maximum number of ipc channel buffer
    UInt  mUsedChannelCount;        // channel count in used
    UInt  mReserved;
} cmbShmInfo;

/*
 * Shared Memory에 존재
 */

typedef struct cmbShmChannelInfo    // must be align by 8 byte
{
    UInt   mTicketNum;              // BUG-32398 타임스탬프에서 티켓번호로 변경
                                    // New Connection시 설정. prevent IPC ghost-connection
    ULong  mPID;                    // Client의 PID
} cmbShmChannelInfo;

inline ULong cmbShmGetFullSize(UInt aChannelCount)
{
    return ((ULong)ID_SIZEOF(cmbShmChannelInfo) * (ULong)aChannelCount) +
            (ULong)CMB_BLOCK_DEFAULT_SIZE *
            (ULong)(aChannelCount * (CMB_SHM_DEFAULT_BUFFER_COUNT));

}

inline cmbShmChannelInfo* cmbShmGetChannelInfo(SChar *aBaseBuf,
                                               UInt   aChannelID)
{
    return (cmbShmChannelInfo*)(aBaseBuf +                // base buffer
                                ID_SIZEOF(cmbShmChannelInfo) * aChannelID);
}

inline SChar* cmnShmGetBuffer(SChar *aBaseBuf,
                              UInt   aChannelCount,
                              UInt   aBufIdx)
{
    return (aBaseBuf +                                      // base buffer
            ID_SIZEOF(cmbShmChannelInfo) * aChannelCount +  // channel info
            (CMB_BLOCK_DEFAULT_SIZE * aBufIdx));
}

inline SChar* cmbShmGetDefaultServerBuffer(SChar *aBaseBuf,
                                           UInt   aChannelCount,
                                           UInt   aChannelID)
{
    return (cmnShmGetBuffer(aBaseBuf, aChannelCount, aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT));
}

inline SChar* cmbShmGetDefaultClientBuffer(SChar *aBaseBuf,
                                           UInt   aChannelCount,
                                           UInt   aChannelID)
{
    return (cmnShmGetBuffer(aBaseBuf, aChannelCount, (aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT) + 1));
}


IDE_RC cmbShmInitializeStatic();
IDE_RC cmbShmFinalizeStatic();

IDE_RC cmbShmCreate(SInt aMaxChannelCount);
IDE_RC cmbShmDestroy();

IDE_RC cmbShmAttach();
IDE_RC cmbShmDetach();

#endif

#endif
