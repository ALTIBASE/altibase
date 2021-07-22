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
 * $Id$
 **********************************************************************/

#ifndef _O_SMX_LCL_H_
#define _O_SMX_LCL_H_ 1

#include <idl.h>
#include <ide.h>
#include <smDef.h>
#include <smxDef.h>

// Lob Cursor List
class smxLCL
{
public:
    //fix BUG-21311
    inline void  initialize();
    inline void  destroy();
    void    getOldestSCN(smSCN* aSCN);
    void    insert(smLobCursor* aStartSCN);
    void    remove(smLobCursor* aStartSCN);
    inline smLobCursor* getBaseNode();
    inline smLobCursor* getFirstNode();
    inline smLobCursor* getNextNode(smLobCursor* aNode);
    inline  UInt getLobCursorCnt(UInt aColumnID, void *aRow);

private:
    smLobCursor   mBaseNode;
    UInt          mSyncVa;
    UInt          mSyncVb;

    /* BUG-31315 [sm_resource] Change allocation disk in mode LOB buffer, 
     * from Open disk LOB cursor to prepare for write 
     * Transaction에 열린 Lob Cursor 개수 검증을 위한 변수*/
    UInt          mCursorCnt;
};

//fix BUG-21311 transform inline  functions.
inline void smxLCL::initialize()
{
    SM_SET_SCN_INFINITE(&(mBaseNode.mLobViewEnv.mSCN));
    mBaseNode.mPrev = &mBaseNode;
    mBaseNode.mNext = &mBaseNode;
    mSyncVa = 0;
    mSyncVb = 0;
    mCursorCnt = 0;
}

inline void smxLCL::destroy()
{
//nothing to do
}

inline smLobCursor* smxLCL::getBaseNode()
{
    return &mBaseNode;
}

inline smLobCursor* smxLCL::getFirstNode()
{
    return mBaseNode.mNext;
}

inline smLobCursor* smxLCL::getNextNode(smLobCursor* aNode)
{
    return aNode->mNext;
}

/***********************************************************************
 * Description : aRow에 aColoumnID가 가리키는 LOB을 보는 Cursor의 갯수를
 *               Return한다.
 *
 * aColumnID    - [IN] Column ID
 * aRow         - [IN] Row Pointer
 ***********************************************************************/
inline  UInt smxLCL::getLobCursorCnt(UInt aColumnID, void *aRow)
{
    smLobCursor* sCurNode;
    UInt         sCount = 0;

    if( aRow == NULL )
    {
        /* aRow가 Null일 경우, 즉 특정 Row에서 찾는게 아니면
         * LCL의 전체 LobCursor 개수를 반환한다. */
        sCount = mCursorCnt;
    }
    else
    {
        sCurNode = getBaseNode()->mNext;

        while(sCurNode != &mBaseNode)
        {
            if((sCurNode->mLobViewEnv.mRow == aRow) &&
               (sCurNode->mLobViewEnv.mLobCol.id == aColumnID))
            {
                sCount++;
            }
            sCurNode = sCurNode->mNext;
        }
    }

    return sCount;
}
#endif
