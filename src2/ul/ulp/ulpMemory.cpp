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

#include <ulpMemory.h>

ulpMemory::ulpMemory()
{
    mHead = NULL;
}

void* ulpMemory::ulpAlloc( UInt aBufSize )
{
    ulpMemoryHeader *sHead;

    sHead = (ulpMemoryHeader *)idlOS::malloc( ID_SIZEOF(ulpMemoryHeader) );
    if( sHead == NULL )
    {
        return NULL;
    }

    // initialize
    sHead->mNext = NULL;
    sHead->mLen  = 0;
    sHead->mBuf  = NULL;

    if( mHead == NULL )
    {
        mHead = sHead;
    }
    else
    {
        sHead->mNext = mHead;
        mHead        = sHead;
    }

    if( aBufSize > 0 )
    {
        /* BUG-32413 APRE memory allocation failure should be fixed */
        sHead->mBuf = (void *)idlOS::malloc( aBufSize );
        if( sHead->mBuf != NULL )
        {
            idlOS::memset(sHead->mBuf, 0, ID_SIZEOF(aBufSize) );
            sHead->mLen = aBufSize;
        }
        else
        {
            return NULL;
        }
    }

    return sHead->mBuf;
}

void  ulpMemory::ulpFree ()
{
    ulpMemoryHeader *sHead;
    ulpMemoryHeader *sNHead;

    sHead = mHead;

    while( sHead != NULL )
    {
        sNHead = sHead -> mNext;
        if ( sHead->mBuf != NULL )
        {
            idlOS::free( sHead->mBuf );
        }
        idlOS::free( sHead );
        sHead = sNHead;
    }

    mHead = NULL;
}
