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

#ifndef _O_CMT_VARIABLE_CLIENT_H_
#define _O_CMT_VARIABLE_CLIENT_H_ 1

#define CMT_VARIABLE_STRING  (0)
#define CMT_VARIABLE_BINARY  (1)

typedef struct cmtVariablePiece
{
    acp_list_node_t  mPieceListNode;

    acp_uint32_t     mOffset;
    acp_uint32_t     mSize;
    acp_uint8_t     *mData;
} cmtVariablePiece;

typedef struct cmtVariable
{
    acp_uint32_t     mTotalSize;
    acp_uint32_t     mCurrentSize;

    acp_uint16_t     mPieceCount;
    acp_list_t       mPieceList;

    cmtVariablePiece mPiece;
} cmtVariable;

typedef struct cmtInVariable
{
    acp_uint32_t     mSize;
    acp_uint8_t     *mData;
} cmtInVariable;

typedef ACI_RC (*cmtVariableGetDataCallback)(cmtVariable  *aVariable,
                                             acp_uint32_t  aOffset,
                                             acp_uint32_t  aSize,
                                             acp_uint8_t  *aData,
                                             void         *aContext);


ACI_RC cmtVariableInitialize(cmtVariable *aVariable);
ACI_RC cmtVariableFinalize(cmtVariable *aVariable);

acp_uint32_t cmtVariableGetSize(cmtVariable *aVariable);
ACI_RC cmtVariableGetData(cmtVariable *aVariable, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize);

ACI_RC cmtVariableGetDataWithCallback(cmtVariable                *aVariable,
                                      cmtVariableGetDataCallback  aCallback,
                                      void                       *aContext);

ACI_RC cmtVariableSetData(cmtVariable *aVariable, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize);
ACI_RC cmtVariableSetVarString(cmtVariable *aVariable, aci_var_string_t *aString);


#endif
