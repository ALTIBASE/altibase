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

#ifndef _O_CMB_BLOCK_H_
#define _O_CMB_BLOCK_H_ 1

//fix BUG-17864.
#if defined(SMALL_FOOTPRINT)
#define CMB_BLOCK_DEFAULT_SIZE   (4 * 1024)
#else
#define CMB_BLOCK_DEFAULT_SIZE   (32* 1024)
#endif

/*BUG-44275 "IPCDA select test 에서 fetch 이상" */
typedef enum
{
    CMB_IPCDA_SHM_ACTIVATED   = 1,
    CMB_IPCDA_SHM_DEACTIVATED = 2,
} cmbIPCDAChannelState;

typedef struct cmbBlock
{
    UShort       mBlockSize;
    UShort       mDataSize;
    UShort       mCursor;
    UChar        mIsEncrypted;
    UChar        mAlign;

    iduListNode  mListNode;

    UChar       *mData;
} cmbBlock;

typedef struct cmbBlockIPCDA
{
    cmbBlock                       mBlock;
    volatile UInt                  mOperationCount;              /* 데이터 영역에 있는 Protocol_Block의 수 */
    volatile cmbIPCDAChannelState  mWFlag;                       /* Write lock flag */
    volatile cmbIPCDAChannelState  mRFlag;                       /* Read lock flag */
    UChar                          mData;                        /* 실제 데이터 영역 */
}cmbBlockIPCDA;

typedef struct cmbBlockSimpleQueryFetchIPCDA
{
    acp_uint8_t  *mData;
}cmbBlockSimpleQueryFetchIPCDA;

/*
 * Move Data between Blocks
 */

IDE_RC cmbBlockMove(cmbBlock *aTargetBlock, cmbBlock *aSourceBlock, UInt aOffset);

/*
 * Read Data from Block
 */

IDE_RC cmbBlockReadSChar(cmbBlock *aBlock, SChar *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadUChar(cmbBlock *aBlock, UChar *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadSShort(cmbBlock *aBlock, SShort *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadUShort(cmbBlock *aBlock, UShort *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadSInt(cmbBlock *aBlock, SInt *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadUInt(cmbBlock *aBlock, UInt *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadSLong(cmbBlock *aBlock, SLong *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadULong(cmbBlock *aBlock, ULong *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadSFloat(cmbBlock *aBlock, SFloat *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadSDouble(cmbBlock *aBlock, SDouble *aValue, idBool *aIsEnd);
IDE_RC cmbBlockReadDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, idBool *aIsEnd);
IDE_RC cmbBlockReadInterval(cmbBlock *ablock, cmtInterval *aInterval, idBool *aIsEnd);
IDE_RC cmbBlockReadNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, idBool *aIsEnd);
IDE_RC cmbBlockReadVariable(cmbBlock *aBlock, cmtVariable *aVariable, idBool *aIsEnd);
IDE_RC cmbBlockReadInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, idBool *aIsEnd);
IDE_RC cmbBlockReadBit(cmbBlock *aBlock, cmtBit *aBit, idBool *aIsEnd);
IDE_RC cmbBlockReadInBit(cmbBlock *aBlock, cmtInBit *aInBit, idBool *aIsEnd);
IDE_RC cmbBlockReadNibble(cmbBlock *aBlock, cmtNibble *aNibble, idBool *aIsEnd);
IDE_RC cmbBlockReadInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, idBool *aIsEnd);
IDE_RC cmbBlockReadLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, idBool *aIsEnd);

IDE_RC cmbBlockReadAny(cmbBlock *aBlock, cmtAny *aAny, idBool *aIsEnd);
IDE_RC cmbBlockReadCollection(cmbBlock *aBlock, cmtCollection *aCollection, idBool *aIsEnd);

/*
 * Write Data to Block
 */

IDE_RC cmbBlockWriteSChar(cmbBlock *aBlock, SChar aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteUChar(cmbBlock *aBlock, UChar aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteSShort(cmbBlock *aBlock, SShort aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteUShort(cmbBlock *aBlock, UShort aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteSInt(cmbBlock *aBlock, SInt aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteUInt(cmbBlock *aBlock, UInt aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteSLong(cmbBlock *aBlock, SLong aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteULong(cmbBlock *aBlock, ULong aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteSFloat(cmbBlock *aBlock, SFloat aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteSDouble(cmbBlock *aBlock, SDouble aValue, idBool *aIsEnd);
IDE_RC cmbBlockWriteDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, idBool *aIsEnd);
IDE_RC cmbBlockWriteInterval(cmbBlock *aBlock, cmtInterval *aInterval, idBool *aIsEnd);
IDE_RC cmbBlockWriteNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, idBool *aIsEnd);
IDE_RC cmbBlockWriteVariable(cmbBlock *aBlock, cmtVariable *aVariable, UInt *aSizeLeft, idBool *aIsEnd);
IDE_RC cmbBlockWriteInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, UInt *aSizeLeft, idBool *aIsEnd);
IDE_RC cmbBlockWriteBit(cmbBlock *aBlock, cmtBit *aBit, UInt *aSizeLeft, idBool *aIsEnd);
IDE_RC cmbBlockWriteInBit(cmbBlock *aBlock, cmtInBit *aInBit, UInt *aSizeLeft, idBool *aIsEnd);
IDE_RC cmbBlockWriteNibble(cmbBlock *aBlock, cmtNibble *aNibble, UInt *aSizeLeft, idBool *aIsEnd);
IDE_RC cmbBlockWriteInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, UInt *aSizeLeft, idBool *aIsEnd);
IDE_RC cmbBlockWriteLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, idBool *aIsEnd);

IDE_RC cmbBlockWriteAny(cmbBlock *aBlock, cmtAny *aAny, UInt *aSizeLeft, idBool *aIsEnd);
IDE_RC cmbBlockWriteCollection(cmbBlock *aBlock, cmtCollection *aCollection, UInt *aSizeLeft, idBool *aIsEnd);

/*Get size of SimpleQueryFetchDataBlock*/
UInt cmbBlockGetIPCDASimpleQueryDataBlockSize();

#endif
