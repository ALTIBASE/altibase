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
 * $Id: qmcMemory.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <qmcMemory.h>

#define QMC_DEFAULT_BUFFER_SIZE (65536 - (UInt) idlOS::align(ID_SIZEOF(qmcMemoryHeader)))

/* BUG-38290 qmcMemory 에서 iduMemory 멤버 제거 */
void qmcMemory::init( UInt aRowSize )
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mHeader = mCurrent = NULL;
    mHeaderSize = (UInt) idlOS::align8((UInt)ID_SIZEOF(qmcMemoryHeader));

    setBufferSize( aRowSize );

#undef IDE_FN
}

/* BUG-38290 qmcMemory 가 iduMemory 를 저장하지 않고,
 * 메모리 할당 시 할당할 iduMemory 를 받아 오도록 함 */
IDE_RC qmcMemory::alloc( iduMemory * aMemory, size_t aSize, void **aMemPtr )
{
#define IDE_FN "qmcMemory::alloc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sResult;
    size_t sSize;
    void * sMemPtr;
    
    IDE_TEST(header( aMemory, &sMemPtr ) != IDE_SUCCESS);

    sSize = idlOS::align8( (UInt)aSize );

    if(sSize == 0)
    {
        // Abort error ...
        IDE_TEST(aMemory->alloc(sSize, aMemPtr) != IDE_SUCCESS);
    }

    if ( sSize > (mCurrent->totalSize - mCurrent->cursor) )
    {
        IDE_TEST(extend( aMemory, sSize, &sMemPtr) != IDE_SUCCESS);
    }
    
    sResult = (SChar*) mCurrent + mCurrent->cursor;
    mCurrent->cursor += sSize;

    *aMemPtr = sResult;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

/* BUG-38290 qmcMemory 가 iduMemory 를 저장하지 않고,
 * 메모리 할당 시 할당할 iduMemory 를 받아 오도록 함 */
IDE_RC qmcMemory::cralloc( iduMemory * aMemory, size_t aSize, void** aMemPtr )
{
#define IDE_FN "qmcMemory::cralloc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST(alloc(aMemory, aSize, aMemPtr) != IDE_SUCCESS);
    
    idlOS::memset(*aMemPtr, 0, aSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

void qmcMemory::clear( UInt aRowSize )
{
/***********************************************************************
 *
 *  Description : To fix BUG-17591
 *                qmcMemory에서 사용한 메모리 블럭 내용을 전부 초기화 한다.
 *                이전에 alloc했던 메모리를 재사용하지 않는다.
 *
 *  Implementation :
 *
 ***********************************************************************/
    
    if ( mHeader != NULL )
    {
        mHeader = mCurrent = NULL;
        mHeaderSize = (UInt) idlOS::align8((UInt)ID_SIZEOF(qmcMemoryHeader));
        setBufferSize( aRowSize );
    }
    else
    {
        // Nothing to do.
    }
}

void 
qmcMemory::clearForReuse()
{
/***********************************************************************
 *
 *  Description : To fix BUG-17591
 *                qmcMemory에서 사용할 메모리의 위치를 첫 위치로 돌린다.
 *                이전에 alloc했던 메모리를 재사용한다.
 *
 *  Implementation :
 *
 ***********************************************************************/
    
    if ( mHeader != NULL )
    {
        mCurrent = mHeader;
        mCurrent->cursor = mHeaderSize;
    }
    else
    {
        // Nothing to do.
    }
}


IDE_RC qmcMemory::header( iduMemory * aMemory, void **aMemPtr )
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( mHeader == NULL )
    {
        IDE_TEST(aMemory->alloc(mBufferSize, (void**)&mHeader) != IDE_SUCCESS);
        
        mCurrent = mHeader;
        mCurrent->cursor = mHeaderSize;
        mCurrent->totalSize = mBufferSize;
        mCurrent->next = NULL;
    }

    *aMemPtr = mCurrent;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC qmcMemory::extend( iduMemory * aMemory, size_t aSize, void ** aMemPtr  )
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( mCurrent->next != NULL )
    {
        while ( mCurrent->next != NULL )
        {
            mCurrent = mCurrent->next;
                
            if ( (mCurrent->totalSize - mHeaderSize) < aSize )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if ( (mCurrent->totalSize - mHeaderSize) < aSize )
        {
            IDE_TEST(extendMemory( aMemory, aSize ) != IDE_SUCCESS);
        }
        else
        {
            // To fix PR-4531
            mCurrent->cursor = mHeaderSize;
        }
    }
    else
    {
        IDE_TEST(extendMemory( aMemory, aSize ) != IDE_SUCCESS);
    }

    *aMemPtr = mCurrent;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aMemPtr = NULL;
    
    return IDE_FAILURE;

#undef IDE_FN
}

void
qmcMemory::setBufferSize( size_t aSize )
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( aSize < 1024 )
    {
        mBufferSize = 4096;
    }
    else
    {
        mBufferSize = ( UInt ) ( ( idlOS::align8( ( UInt )aSize ) + mHeaderSize ) * 4 );

        if ( mBufferSize > (UInt)QMC_DEFAULT_BUFFER_SIZE )
        {
            mBufferSize = (UInt) idlOS::align8((UInt)aSize) + mHeaderSize;
        }
        else
        {
            /* Nothing to do */
        }
    }

#undef IDE_FN
}

IDE_RC qmcMemory::extendMemory( iduMemory * aMemory, size_t aSize )
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( aSize > mBufferSize - mHeaderSize)
    {
        setBufferSize( aSize );
    }

    IDE_TEST(aMemory->alloc(mBufferSize, (void**)&(mCurrent->next))
             != IDE_SUCCESS);

    if ( mCurrent->next != NULL )
    {
        mCurrent = mCurrent->next;
        mCurrent->next = NULL;
        mCurrent->cursor = mHeaderSize;
        mCurrent->totalSize = mBufferSize;
    }
    else
    {
        mCurrent = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}
