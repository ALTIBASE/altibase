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

#ifndef _O_CMT_VARIABLE_H_
#define _O_CMT_VARIABLE_H_ 1

#define CMT_VARIABLE_STRING 0
#define CMT_VARIABLE_BINARY 1

typedef struct cmtVariablePiece
{
    iduListNode  mPieceListNode;

    UInt         mOffset;
    UInt         mSize;
    UChar       *mData;
} cmtVariablePiece;

typedef struct cmtVariable
{
    UInt             mTotalSize;
    UInt             mCurrentSize;

    UShort           mPieceCount;
    iduList          mPieceList;

    cmtVariablePiece mPiece;
} cmtVariable;

typedef struct cmtInVariable
{
    UInt      mSize;
    UChar    *mData;
} cmtInVariable;

typedef IDE_RC (*cmtVariableGetDataCallback)(cmtVariable *aVariable,
                                             UInt         aOffset,
                                             UInt         aSize,
                                             UChar       *aData,
                                             void        *aContext);


IDE_RC cmtVariableInitialize(cmtVariable *aVariable);
IDE_RC cmtVariableFinalize(cmtVariable *aVariable);

UInt   cmtVariableGetSize(cmtVariable *aVariable);
IDE_RC cmtVariableGetData(cmtVariable *aVariable, UChar *aBuffer, UInt aBufferSize);

IDE_RC cmtVariableGetDataWithCallback(cmtVariable                *aVariable,
                                      cmtVariableGetDataCallback  aCallback,
                                      void                       *aContext);

IDE_RC cmtVariableSetData(cmtVariable *aVariable, UChar *aBuffer, UInt aBufferSize);
IDE_RC cmtVariableSetVarString(cmtVariable *aVariable, iduVarString *aString);


#endif
