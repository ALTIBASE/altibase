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
 * $Id: qmcMemory.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QMC_MEMORY_H_
#define _O_QMC_MEMORY_H_ 1

#include <iduMemory.h>

typedef struct qmcMemoryHeader
{
    qmcMemoryHeader * next;
    UInt              cursor;
    UInt              totalSize;
} qmcMemoryHeader;

class qmcMemory
{
private:
    qmcMemoryHeader * mHeader;
    qmcMemoryHeader * mCurrent;
    UInt              mBufferSize;
    UInt              mHeaderSize;

public:
    /* BUG-38290 */
    void              init( UInt aRowSize );
    IDE_RC            alloc( iduMemory * aMemory, size_t aSize, void** );
    IDE_RC            cralloc( iduMemory * aMemory, size_t aSize, void** );
    void              clear( UInt aRowSize );
    void              clearForReuse();

private:
    IDE_RC            header( iduMemory * aMemory, void ** );
    IDE_RC            extend( iduMemory * aMemory, size_t aSize, void** );
    void              setBufferSize( size_t aSize );
    IDE_RC            extendMemory( iduMemory * aMemory, size_t aSize );
};

#endif /* _O_QMC_MEMORY_H_ */
