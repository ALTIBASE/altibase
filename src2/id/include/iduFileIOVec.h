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
 
/***********************************************************************
 * $Id:
 **********************************************************************/

/***********************************************************************
 *
 * NAME
 *   iduFileIOVec.h
 *
 * DESCRIPTION
 *
 * PUBLIC FUNCTION(S)
 *
 * PRIVATE FUNCTION(S)
 *
 * NOTES
 *
 * MODIFIED    (MM/DD/YYYY)
 *    djin      01/11/2017 - Created
 *
 **********************************************************************/

#ifndef _O_IDU_FILE_IOVEC_H_
#define _O_IDU_FILE_IOVEC_H_ 1

#include <idErrorCode.h>
#include <idl.h>
#include <idv.h>
#include <iduFXStack.h>
#include <iduMemDefs.h>
#include <iduProperty.h>

class iduFileIOVec
{
public:
    iduFileIOVec();
    ~iduFileIOVec();

    IDE_RC  initialize(SInt = 0, ...);
    IDE_RC  initialize(SInt, void**, size_t*);
    IDE_RC  destroy(void);
    IDE_RC  add(void*, size_t);
    IDE_RC  add(SInt, void**, size_t*);
    IDE_RC  clear(void);

    /* Cast operators */
    inline struct iovec*    getIOVec(void)  { return mIOVec; }
    inline SInt             getCount(void)  { return mCount; }
    inline operator struct  iovec*(void)    { return getIOVec(); }
    inline operator SInt          (void)    { return getCount(); }

private:
    /* 데이터를 저장할 포인터와 길이 배열 */
    struct iovec*   mIOVec;
    /* mIOVec에 설정된 데이터/길이 개수 */
    SInt            mCount;
    /* mIOVec의 크기. */
    SInt            mSize;
};

#endif /* of _O_IDU_FILE_IOVEC_H_ */
