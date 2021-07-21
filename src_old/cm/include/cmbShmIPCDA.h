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

#ifndef _O_CMB_SHM_DA_H_
#define _O_CMB_SHM_DA_H_ 1

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

//fix BUG-18834
#define CMB_SHM_ALIGN                  (8)

//fix BUG-18830
#define CMB_SHM_DEFAULT_BUFFER_COUNT    (2)
#define CMB_SHM_DEFAULT_DATABLOCK_COUNT (1)

// bug-27250 free Buf list can be crushed when client killed
// semaphore undo용 초기값 (10은 임의의 넉넉한 값임)
#define CMB_SHM_SEMA_UNDO_VALUE        10

#define CMB_INVALID_IPCDA_SHM_KEY      -1

typedef struct cmbShmIPCDAInfo
{
    UInt                mMaxChannelCount;         // maximum number of ipcda channel
    UInt                mMaxBufferCount;          // maximum number of ipcda channel buffer
    UInt                mUsedChannelCount;        // channel count used

    SInt                mClientCount;
    idBool             *mChannelList;

    key_t               mShmKey;
    PDL_HANDLE          mShmID;
    UChar              *mShmBuffer;

    key_t               mShmKeyForSimpleQuery;
    PDL_HANDLE          mShmIDForSimpleQuery;
    UChar              *mShmBufferForSimpleQuery;

    key_t              *mSemChannelKey;
    SInt               *mSemChannelID;
} cmbShmIPCDAInfo;

/* Shared Memory에 존재 */
typedef struct cmbShmIPCDAChannelInfo    // must be align by 8 byte
{
    UInt   mTicketNum;              // BUG-32398 타임스탬프에서 티켓번호로 변경
                                    // New Connection시 설정. prevent IPC ghost-connection
    ULong  mPID;                    // Client의 PID
} cmbShmIPCDAChannelInfo;

inline ULong cmbShmIPCDAGetSize(UInt aChannelCount)
{
    return ((ULong)ID_SIZEOF(cmbShmIPCDAChannelInfo) * (ULong)aChannelCount) +
            (ULong)CMB_BLOCK_DEFAULT_SIZE *
            (ULong)(aChannelCount * (CMB_SHM_DEFAULT_BUFFER_COUNT));

}

inline ULong cmbShmIPCDAGetDataBlockSize(UInt aChannelCount)
{
    return (ULong)cmbBlockGetIPCDASimpleQueryDataBlockSize() *
           (ULong)((aChannelCount)*(CMB_SHM_DEFAULT_DATABLOCK_COUNT));
}

inline cmbShmIPCDAChannelInfo* cmbShmIPCDAGetChannelInfo(UChar *aBaseBuf, UInt   aChannelID)
{
    return (cmbShmIPCDAChannelInfo*)(aBaseBuf + ID_SIZEOF(cmbShmIPCDAChannelInfo) * aChannelID);
}

inline UChar* cmnShmIPCDAGetBuffer(UChar *aBaseBuf,
                                   UInt   aChannelCount,
                                   UInt   aBufIdx)
{
    return (aBaseBuf +                                      // base buffer
            ID_SIZEOF(cmbShmIPCDAChannelInfo) * aChannelCount +  // channel info
            (CMB_BLOCK_DEFAULT_SIZE * aBufIdx));
}

inline UChar* cmnShmIPCDAGetDataBlock(UChar *aBaseBuf, UInt   aBufIdx)
{
    return (aBaseBuf + (cmbBlockGetIPCDASimpleQueryDataBlockSize() * aBufIdx));
}

inline UChar* cmbShmIPCDAGetDefaultServerBuffer(UChar *aBaseBuf,
                                                UInt   aChannelCount,
                                                UInt   aChannelID)
{
    return (cmnShmIPCDAGetBuffer(aBaseBuf, 
                                 aChannelCount,
                                 aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT));
}

inline UChar* cmbShmIPCDAGetDefaultClientBuffer(UChar *aBaseBuf,
                                                UInt   aChannelCount,
                                                UInt   aChannelID)
{
    return (cmnShmIPCDAGetBuffer(aBaseBuf, 
                                 aChannelCount,
                                 (aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT) + 1));
}

inline UChar* cmbShmIPCDAGetServerWriteDataBlock(UChar *aBaseBuf,
                                                 UInt /*aChannelCount*/,
                                                 UInt   aChannelID)
{
    return (cmnShmIPCDAGetDataBlock(aBaseBuf, (aChannelID * CMB_SHM_DEFAULT_DATABLOCK_COUNT)));
}

inline UChar* cmbShmIPCDAGetServerReadDataBlock(UChar */*aBaseBuf*/,
                                                UInt   /*aChannelCount*/,
                                                UInt   /*aChannelID*/)
{
    return NULL;
}

IDE_RC cmbShmIPCDAInitializeStatic();
IDE_RC cmbShmIPCDAFinalizeStatic();

IDE_RC cmbShmIPCDACreate(SInt aMaxChannelCount);
IDE_RC cmbShmIPCDADestroy();

IDE_RC cmbShmIPCDAAttach();
void   cmbShmIPCDADetach();

#endif

#endif
