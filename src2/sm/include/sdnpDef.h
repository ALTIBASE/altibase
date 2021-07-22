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
 

/*******************************************************************************
 * $Id: sdnpDef.h 88191 2020-07-27 03:08:54Z mason.lee $
 ******************************************************************************/

#ifndef _O_SDNP_DEF_H_
# define _O_SDNP_DEF_H_ 1

//disk table에 대한 GRID iterator
#include <smDef.h>

typedef struct sdnpIterator
{
    smSCN       mSCN;
    smSCN       mInfinite;
    void*       mTrans;
    void*       mTable;
    SChar*      mCurRecPtr;  // MRDB scan module에서만 정의해서 쓰도록 수정?
    SChar*      mLstFetchRecPtr;
    scGRID      mRowGRID;
    smTID       mTid;
    UInt        mFlag;

    smiCursorProperties  * mProperties;
    smiStatement         * mStatement;
    /* smiIterator 공통 변수 끝 */

    const smiRange     * mRange;
    const smiRange     * mNxtRange;
    const smiCallBack  * mFilter;

    sddTableSpaceNode  * mTBSNode;
} sdnpIterator;

#endif /* _O_SDNP_DEF_H_ */
