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

#ifndef _O_CMB_BLOCK_CLIENT_H_
#define _O_CMB_BLOCK_CLIENT_H_ 1

/*
 * fix BUG-17864.
 */
#if defined(SMALL_FOOTPRINT)
#define CMB_BLOCK_DEFAULT_SIZE   (4 * 1024)
#else
#define CMB_BLOCK_DEFAULT_SIZE   (32 * 1024)
#endif

/*BUG-44275 "IPCDA select test 에서 fetch 이상" */
typedef enum
{
    CMB_IPCDA_SHM_ACTIVATED   = 1,
    CMB_IPCDA_SHM_DEACTIVATED = 2,
} cmbIPCDAChannelState;

typedef struct cmbBlock
{
    acp_uint16_t       mBlockSize;
    acp_uint16_t       mDataSize;
    acp_uint16_t       mCursor;
    acp_uint8_t        mIsEncrypted;
    acp_uint8_t        mAlign;

    acp_list_node_t    mListNode;

    acp_uint8_t       *mData;
} cmbBlock;

typedef struct cmbBlockIPCDA
{
    cmbBlock                        mBlock;
    volatile acp_uint32_t           mOperationCount;              /* 데이터영역에 있는 Protocol block의 수 */
    volatile cmbIPCDAChannelState   mWFlag;                       /* Write lock flag */
    volatile cmbIPCDAChannelState   mRFlag;                       /* Read lock flag */
    acp_uint8_t                     mData;                        /* 실제 데이터 영역 */
}cmbBlockIPCDA;

typedef struct cmbBlockSimpleQueryFetchIPCDA
{
    acp_uint8_t  *mData;
}cmbBlockSimpleQueryFetchIPCDA;

/*
 * Move Data between Blocks
 */

ACI_RC cmbBlockMove(cmbBlock *aTargetBlock, cmbBlock *aSourceBlock, acp_uint32_t aOffset);

/*
 * Read Data from Block
 */

ACI_RC cmbBlockReadSChar(cmbBlock *aBlock, acp_sint8_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadUChar(cmbBlock *aBlock, acp_uint8_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadSShort(cmbBlock *aBlock, acp_sint16_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadUShort(cmbBlock *aBlock, acp_uint16_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadSInt(cmbBlock *aBlock, acp_sint32_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadUInt(cmbBlock *aBlock, acp_uint32_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadSLong(cmbBlock *aBlock, acp_sint64_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadULong(cmbBlock *aBlock, acp_uint64_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadSFloat(cmbBlock *aBlock, acp_float_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadSDouble(cmbBlock *aBlock, acp_double_t *aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadInterval(cmbBlock *ablock, cmtInterval *aInterval, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadVariable(cmbBlock *aBlock, cmtVariable *aVariable, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadBit(cmbBlock *aBlock, cmtBit *aBit, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadInBit(cmbBlock *aBlock, cmtInBit *aInBit, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadNibble(cmbBlock *aBlock, cmtNibble *aNibble, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, acp_bool_t *aIsEnd);

ACI_RC cmbBlockReadAny(cmbBlock *aBlock, cmtAny *aAny, acp_bool_t *aIsEnd);
ACI_RC cmbBlockReadCollection(cmbBlock *aBlock, cmtCollection *aCollection, acp_bool_t *aIsEnd);

/*
 * Write Data to Block
 */

ACI_RC cmbBlockWriteSChar(cmbBlock *aBlock, acp_sint8_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteUChar(cmbBlock *aBlock, acp_uint8_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteSShort(cmbBlock *aBlock, acp_sint16_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteUShort(cmbBlock *aBlock, acp_uint16_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteSInt(cmbBlock *aBlock, acp_sint32_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteUInt(cmbBlock *aBlock, acp_uint32_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteSLong(cmbBlock *aBlock, acp_sint64_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteULong(cmbBlock *aBlock, acp_uint64_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteSFloat(cmbBlock *aBlock, acp_float_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteSDouble(cmbBlock *aBlock, acp_double_t aValue, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteInterval(cmbBlock *aBlock, cmtInterval *aInterval, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteVariable(cmbBlock *aBlock, cmtVariable *aVariable, acp_uint32_t *aSizeLeft, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteBit(cmbBlock *aBlock, cmtBit *aBit, acp_uint32_t *aSizeLeft, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteInBit(cmbBlock *aBlock, cmtInBit *aInBit, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteNibble(cmbBlock *aBlock, cmtNibble *aNibble, acp_uint32_t *aSizeLeft, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, acp_bool_t *aIsEnd);

ACI_RC cmbBlockWriteAny(cmbBlock *aBlock, cmtAny *aAny, acp_uint32_t *aSizeLeft, acp_bool_t *aIsEnd);
ACI_RC cmbBlockWriteCollection(cmbBlock *aBlock, cmtCollection *aCollection, acp_uint32_t *aSizeLeft, acp_bool_t *aIsEnd);

/*Get size of SimpleQueryFetchDataBlock*/
acp_uint32_t cmbBlockGetIPCDASimpleQueryDataBlockSize();

#endif
