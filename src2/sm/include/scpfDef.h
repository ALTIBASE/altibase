/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 *
 * $Id: scpfDef.h $
 *
 **********************************************************************/

#ifndef _O_SCPF_DEF_H_
#define _O_SCPF_DEF_H_ 1

#include <smDef.h>
#include <scpDef.h>
#include <iduFile.h>

#define SCPF_VARINT_1BYTE_THRESHOLD        (64)
#define SCPF_VARINT_2BYTE_THRESHOLD        (16384)
#define SCPF_VARINT_3BYTE_THRESHOLD        (4194304)
#define SCPF_VARINT_4BYTE_THRESHOLD        (1073741824)

#define SCPF_WRITE_UINT              SMI_WRITE_UINT
#define SCPF_WRITE_USHORT            SMI_WRITE_USHORT
#define SCPF_WRITE_ULONG             SMI_WRITE_ULONG
#define SCPF_READ_UINT               SMI_READ_UINT
#define SCPF_READ_USHORT             SMI_READ_USHORT
#define SCPF_READ_ULONG              SMI_READ_ULONG

#define SCPF_GET_VARINT_MAXSIZE      (4)
#define SCPF_GET_VARINT_SIZE( src )                                \
     ( ( (*(src)) < SCPF_VARINT_1BYTE_THRESHOLD ) ? 1 :     \
       ( (*(src)) < SCPF_VARINT_2BYTE_THRESHOLD ) ? 2 :     \
       ( (*(src)) < SCPF_VARINT_3BYTE_THRESHOLD ) ? 3 : 4 )

/* UInt을 BigEndian으로 Write한다. */
#define SCPF_WRITE_VARINT(src, dst){                                 \
     if( (*(UInt*)(src)) < SCPF_VARINT_1BYTE_THRESHOLD )             \
     {                                                               \
        ((UChar*)(dst))[0] = *(UInt*)(src);                          \
     } else                                                          \
     if( (*(UInt*)(src)) < SCPF_VARINT_2BYTE_THRESHOLD )             \
     {                                                               \
        ((UChar*)(dst))[0] = (((*(UInt*)(src))>>8) & 63) | ( 1<<6 ); \
        ((UChar*)(dst))[1] = (*(UInt*)(src)) & 255;                  \
     } else                                                          \
     if( (*(UInt*)(src)) < SCPF_VARINT_3BYTE_THRESHOLD )             \
     {                                                               \
        ((UChar*)(dst))[0] = (((*(UInt*)(src))>>16) & 63) | ( 2<<6 );\
        ((UChar*)(dst))[1] = (((*(UInt*)(src))>> 8) & 255);          \
        ((UChar*)(dst))[2] = (*(UInt*)(src)) & 255;                  \
     } else                                                          \
     if( (*(UInt*)(src)) < SCPF_VARINT_4BYTE_THRESHOLD )             \
     {                                                               \
        ((UChar*)(dst))[0] = (((*(UInt*)(src))>>24) & 63) | ( 3<<6 );\
        ((UChar*)(dst))[1] = (((*(UInt*)(src))>>16) & 255);          \
        ((UChar*)(dst))[2] = (((*(UInt*)(src))>> 8) & 255);          \
        ((UChar*)(dst))[3] = (*(UInt*)(src)) & 255;                  \
     } else                                                          \
     {                                                               \
         IDE_ASSERT( 0 );                                            \
     }                                                               \
}

/* UInt을 BigEndian으로 Write한다. */
#define SCPF_READ_VARINT(src, dst){                    \
    switch( (((UChar*)(src))[0]) >> 6 )                \
    {                                                  \
    case 0:                                            \
        *(dst) = (((UChar*)(src))[0] & 63);            \
        break;                                         \
    case 1:                                            \
        *(dst) = (((((UChar*)(src))[0]) & 63)<<8)      \
               | (((UChar*)(src))[1]);                 \
        break;                                         \
    case 2:                                            \
        *(dst) = (((((UChar*)(src))[0]) & 63)<<16)     \
               | ((((UChar*)(src))[1])<<8)             \
               | ((((UChar*)(src))[2]));               \
        break;                                         \
    case 3:                                            \
        *(dst) = (((((UChar*)(src))[0]) & 63)<<24)     \
               | ((((UChar*)(src))[1])<<16)            \
               | ((((UChar*)(src))[2])<<8)             \
               | ((((UChar*)(src))[3]));               \
        break;                                         \
    default:                                           \
        break;                                         \
    }                                                  \
}

#define SCPF_GET_SLOT_VAL_LEN( src )                       \
    ( *(src) - SCPF_GET_VARINT_SIZE(src) )

#define SCPF_LOB_LEN_SIZE     ID_SIZEOF( UInt )


//File의 Version
#define SCPF_VERSION_1        (1)
#define SCPF_VERSION_LATEST   SCPF_VERSION_1

#define SCPF_DPFILE_EXT       ((SChar*)".dpf")
#define SCPF_DPFILE_SEPARATOR '_'

// BlockInfo는 File로부터 읽은 Block을 올려놓음 메모리상의
// 객체로, Export시 1개, Import시 최소 2개를 사용한다.
#define SCPF_MIN_BLOCKINFO_COUNT   (2)

#define SCPF_OFFSET_NULL           (0)

// 현재 진행중인 일의 BlockBuffer내 Position
typedef struct scpfBlockBufferPosition
{
    UInt  mSeq;         // 읽고있는 Block Info내 Block Sequence.
    UInt  mID;          // 현재 읽고있는 Block의 File내 번호.
    UInt  mOffset;      // Block내 Offset 
    UInt  mSlotSeq;     // Block내 Slot번호
} scpfBlockBufferPosition;

// 현재 진행중인 일의 File내 위치
typedef struct scpfFilePosition
{
    UInt       mIdx;      // 현재 다루고 있는 파일의 번호
    ULong      mOffset;   // 현재까지 읽거나 쓴 FileOffset
} scpfFilePosition;

// 하나의 Block에 관한 종합적인 정보
typedef struct scpfBlockInfo
{
    //Store
    UInt       mCheckSum;             // Checksum
    UInt       mBlockID;              // 이 Block의 File 내 Seq
    SLong      mRowSeqOfFirstSlot;    // Block내 첫번째 Slot의 RowSeq
    SLong      mRowSeqOfLastSlot;     // Block내 마지막 Slot RowSeq

    UInt       mFirstRowSlotSeq;      // Block내 첫번째 Row의 SlotSeq 
    UInt       mFirstRowOffset;       // Block내 첫번째 Row의 Offset
    idBool     mHasLastChainedSlot;   // Block내 마지막 Slot이 다음 Block에
                                      // Chained돼어 있는지 여부
    UInt       mSlotCount ;           // Block내 총 Slot의 개수
 
    //Runtime
    UChar    * mBlockPtr;             // 이 Block에 대한 실제 데이터
} scpfBlockInfo;

extern smiDataPortHeaderDesc  gSCPfBlockHeaderEncDesc[];

//File의 Header에 기록된다.
typedef struct scpfHeader
{
    UInt  mFileHeaderSize;     // File의 헤더 부분의 크기
    UInt  mBlockHeaderSize;    // Block의 Header의 크기
    UInt  mBlockSize;          // Block 하나의 크기
    UInt  mBlockInfoCount;     // Import하는데 필요한 BlockInfo의 개수
    UInt  mMaxRowCountInBlock; // 하나의 Block이 가진 Row의 최대 개수. 
                               // smiRow4DP의 크기 결정에 사용됨.
                               // Block내 Row는 UInt개를 넘을 수 없다
                               //     (Block의 크기가 UInt가 한계이기 )
    UInt  mBlockCount;         // 이 File의 총 Block 개수.
                               // Export시에는 현재까지 생성된 Block의 개수
                               // Import시에는 File이 가지고 있는 Block의 개수
    SLong mRowCount;           // 이 File이 가진 Row의 개수
    SLong mFirstRowSeqInFile;  // File내 첫번째 Row
    SLong mLastRowSeqInFile;   // File내 미자믹 Row
} scpfHeader;

extern smiDataPortHeaderDesc  gSCPfFileHeaderEncDesc[];

// Data를 저장할 File의 Handle
typedef struct scpfHandle
{
    scpHandle               mCommonHandle;
    scpfHeader              mFileHeader;       // FileHeader

    scpfBlockBufferPosition mBlockBufferPosition;    // 진행중인 Block 위치
    scpfFilePosition        mFilePosition;           // 진행중인 파일 위치


    /* 예)    초기상태
     *        BlockMap 0,1,2,3     
     *
     *     => 0,1번 Block의 Row는 모두 처리하였음
     *        (BlockBufferPosition.mSeq가 2를 가리킴)
     *
     *     => 2개 만큼 Shift시킴
     *        BlockMap  2,3,0,1    
     *
     *     => 이후 0,1번에 4,5번 Block을 올림
     *        BlockMap  2,3,4,5    
     */    
    scpfBlockInfo     ** mBlockMap;         // 읽고쓰는 메모리상의 Buffer.
                                            // Circular하게 순환된다.
                  
    smiRow4DP          * mRowList;          // Import시 보낼 Row들(Imp)

    /*  Block을 읽고 쓰는데 필요한 동적 메모리 공간을 Block마다 하나하나
     * 할당하지 않고, 한번에 N개씩 할당한 후 BlockMap에 분배한다.
     *  따라서 할당한 메모리를 해제할때를 위해 할당받을 당시의 Pointer
     * 를 가지고 있어야 한다. 다음 세 변수는 그 Pointer이다. */
    scpfBlockInfo      * mBlockBuffer;      // BlockInfo의 Buffer
    smiValue           * mValueBuffer;      // RowList내 ValueList(Imp)
    UChar              * mAllocatedBlock;   // 할당한 Block
    UInt               * mColumnMap;        // columnHeader의 ColumnMap목록
                                            // 을 임시로 저장해둔다

    UInt       mColumnChainingThreshold;// Column이 Chainning되는 크기

    SLong      mRowSeq;                 // Import/Export진행중인 Row
    SLong      mLimitRowSeq;            // Import하려는 마지막 Row(Imp)
    SLong      mSplit;                  // 파일이 쪼개지는 분기점

    UChar    * mChainedSlotBuffer;      // Chained된 Row를 기록하는 공간(Imp)
    UInt       mUsedChainedSlotBufSize; // 사용된 ChainedRowBuf 크기 (Imp)

    idBool     mIsFirstSlotInRow;       // Row의 첫 Slot인가 (Exp)

    UInt       mAllocatedLobSlotSize;   // 기록 예약해둔 Lob영역의 크기
    UInt       mRemainLobLength;        // 현재 기록하는 Lob의 남은 길이
    UInt       mRemainLobColumnCount;   // 기록해야 하는 남은 LobColumn개수

    idBool     mOpenFile;
    iduFile    mFile;      
    SChar      mDirectory[SM_MAX_FILE_NAME];  // Directory이름
    SChar      mObjName[SM_MAX_FILE_NAME];    // Object이름
    SChar      mFileName[SM_MAX_FILE_NAME];   // 파일의 이름

} scpfHandle;

#endif // _O_SCPF_DEF_H_
