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
 * $Id: smnfDef.h 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#ifndef _O_SMNF_DEF_H_
# define _O_SMNF_DEF_H_ 1

# include <idl.h>
# include <idTypes.h>

# include <smc.h>
# include <iduMemory.h>

typedef struct smnfIterator
{
    smSCN               SCN;
    smSCN               infinite;
    void              * trans;
    smcTableHeader    * table;
    SChar             * curRecPtr;
    SChar             * lstFetchRecPtr;
    scGRID              mRowGRID;
    smTID               tid;
    UInt                flag;

    smiCursorProperties  * mProperties;
    smiStatement         * mStatement;
    /* smiIterator 공통 변수 끝 */

    UChar*             mRecBuffer;
    UChar*             mRecEndBuffer;
    const smiCallBack* filter;
    SChar**            slot;
    UInt               lstReadOffset;
    UChar*             mTraversePtr;
    ULong              mReadRecordPos;
    ULong              mFixRecCount;
    iduFixedTableMemory mMemory;
    idBool              mIsMemoryInit;
    
} smnfIterator;
#endif /* _O_SMNF_DEF_H_ */
