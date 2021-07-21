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
 * $Id: iduStringHash.cpp 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#include <idl.h>
#include <iduStringHash.h>
#include <iduHashUtil.h>



IDE_RC iduStringHash::initialize( iduMemoryClientIndex aIndex,
                                  UInt                 aInitFreeBucketCount,
                                  UInt                 aHashTableSize )
{
#define IDE_FN "iduStringHash::initialize"
    IDE_MSGLOG_FUNC( IDE_MSGLOG_BODY( "" ) );

    UInt                 sIndex;
    iduStringHashBucket *sNewBucket;

    mIndex           = aIndex;
    mHashTableSize   = aHashTableSize;
    mFreeBucketCount = aInitFreeBucketCount;
    mCashBucketCount = aInitFreeBucketCount;
    
    IDE_TEST( iduMemMgr::malloc( mIndex,
                                 ID_SIZEOF( iduStringHashBucket ) * aHashTableSize,
                                 ( void** )&mHashTable )
              != IDE_SUCCESS );

    for( sIndex = 0; sIndex < mHashTableSize; sIndex++ )
    {
        ( mHashTable + sIndex )->pPrev = mHashTable + sIndex;
        ( mHashTable + sIndex )->pNext = mHashTable + sIndex;
        ( mHashTable + sIndex )->pData = NULL;
    }

    mFreeBucketList.pPrev = &mFreeBucketList;
    mFreeBucketList.pNext = &mFreeBucketList;
    
    for( sIndex = 0; sIndex < mFreeBucketCount; sIndex++ )
    {
        IDE_TEST( iduMemMgr::malloc( mIndex,
                                     ID_SIZEOF( iduStringHashBucket ),
                                     ( void** )&sNewBucket )
                  != IDE_SUCCESS );

        initHashBucket( sNewBucket );
        addHashBucketToFreeList( sNewBucket );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduStringHash::destroy( )
{
#define IDE_FN "iduStringHash::destroy"
    IDE_MSGLOG_FUNC( IDE_MSGLOG_BODY( "" ) );

    iduStringHashBucket *sCurHashBucket;
    iduStringHashBucket *sNxtHashBucket;
    
    sCurHashBucket = mFreeBucketList.pNext;

    // Free List에 있는 Hash Bucket을 반환한다.
    while( sCurHashBucket != &mFreeBucketList )
    {
        sNxtHashBucket = sCurHashBucket->pNext;
        
        IDE_TEST( iduMemMgr::free( sCurHashBucket ) != IDE_SUCCESS );

        sCurHashBucket = sNxtHashBucket;
    }

    // Hash Table을 삭제
    IDE_TEST( iduMemMgr::free( mHashTable ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduStringHash::insert( SChar * aString, void* aData )
{
#define IDE_FN "iduStringHash::insert"
    IDE_MSGLOG_FUNC( IDE_MSGLOG_BODY( "" ) );

    iduStringHashBucket *sNewHashBucket;
    vULong               sHashValue;
    iduStringHashBucket *sBucketList;
    vULong               sKey;
    UInt                 sStringLen;
    SChar               *sTempString;
    //SChar               *tmp = NULL;
    
    IDE_TEST( allocHashBucket( &sNewHashBucket ) != IDE_SUCCESS );

    IDE_TEST ( ( sStringLen = idlOS::strlen ( aString ) ) == 0 );
    sKey = iduHashUtil::hashString( IDU_HASHUTIL_INITVALUE, (const UChar *)aString, sStringLen );

    sNewHashBucket->nKey  = sKey;
    sNewHashBucket->pData = aData;

    IDE_TEST( iduMemMgr::malloc( mIndex,
                                 sStringLen,
                                 ( void** )& sTempString )
              != IDE_SUCCESS );
    sNewHashBucket->nString = sTempString;

    idlOS::strcpy ( sNewHashBucket->nString , aString );

    sHashValue = hash( sKey );
    
    sBucketList = mHashTable + sHashValue;
    
    addHashBucketToList( sBucketList, sNewHashBucket );
    //idlOS::strcpy( tmp, NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

void* iduStringHash::search( SChar * aString, iduStringHashBucket **aHashBucket )
{
#define IDE_FN "iduStringHash::search"
    IDE_MSGLOG_FUNC( IDE_MSGLOG_BODY( "" ) );

    vULong                sHashValue;
    iduStringHashBucket  *sBucketList;
    iduStringHashBucket  *sCurHashBucket;
    void                 *sData = NULL;
    vULong                sKey;
    UInt                  sStringLen;

    sStringLen = idlOS::strlen ( aString );

    IDE_TEST ( sStringLen == 0 );
    sKey = iduHashUtil::hashString( IDU_HASHUTIL_INITVALUE, (const UChar *) aString, sStringLen );
    
    sHashValue = hash( sKey );

    sBucketList = mHashTable + sHashValue;

    sCurHashBucket = sBucketList->pNext;
    
    while( sCurHashBucket != sBucketList )
    {
        if( sCurHashBucket->nKey > sKey )
        {
            break;
        }

        if( sCurHashBucket->nKey == sKey )
        {
            if ( idlOS::strcmp( sCurHashBucket->nString , aString ) == 0 )
            {
                sData = sCurHashBucket->pData;
            
                break;
            }
        }

        sCurHashBucket = sCurHashBucket->pNext;
    }

    if( aHashBucket != NULL )
    {
        *aHashBucket = ( sData == NULL ) ? NULL : sCurHashBucket;
    }
    
    return sData;

    IDE_EXCEPTION_END;

    return NULL;
    
#undef IDE_FN
}

/*
    모든 ( Hash Key, Data )를 순회한다
    [IN] aVisitor   - Hash Key의 Visitor Function
    [IN] VisitorArg - Visitor Function에 넘길 인자

    <참고사항>
     traverse도중에 hash에서 해당 bucket을 제거할 수도 있도록 설계되었다
 */
IDE_RC iduStringHash::traverse( iduStringHashVisitor    aVisitor,
                                void                  * aVisitorArg )
{
    UInt                  sIndex;
    iduStringHashBucket  *sBucketList;
    iduStringHashBucket  *sCurHashBucket;
    iduStringHashBucket  *sNextHashBucket;
    
    for( sIndex = 0; sIndex < mHashTableSize; sIndex++ )
    {
        sBucketList = ( mHashTable + sIndex );

        sCurHashBucket = sBucketList->pNext;
    
        while( sCurHashBucket != sBucketList )
        {
            // Visitor가 현재 Bucket을 제거할 경우에 대비하여
            // 다음 Bucket을 미리 얻어놓는다.
            sNextHashBucket = sCurHashBucket->pNext;
            
            IDE_TEST( ( *aVisitor )( sCurHashBucket->nString,
                                     sCurHashBucket->pData,
                                     aVisitorArg )
                      != IDE_SUCCESS );

            sCurHashBucket = sNextHashBucket ;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC iduStringHash::remove( SChar * aString )
{
#define IDE_FN "iduStringHash::remove"
    IDE_MSGLOG_FUNC( IDE_MSGLOG_BODY( "" ) );

    iduStringHashBucket* sHashBucket = NULL;

    search( aString, &sHashBucket );

    IDE_TEST( sHashBucket == NULL );

    if( sHashBucket->nString != NULL )
    {
        IDE_TEST ( iduMemMgr::free( sHashBucket->nString ) 
                   != IDE_SUCCESS );
    }

    removeHashBuckeFromList( sHashBucket );

    IDE_TEST( freeHashBucket( sHashBucket ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduStringHash::allocHashBucket( iduStringHashBucket **aHashBucket )
{
#define IDE_FN "iduStringHash::allocHashBucket"
    IDE_MSGLOG_FUNC( IDE_MSGLOG_BODY( "" ) );

    iduStringHashBucket *sFreeHashBucket;
    
    if( mFreeBucketCount != 0 )
    {
        assert( mFreeBucketList.pNext != &mFreeBucketList );
        
        sFreeHashBucket = mFreeBucketList.pNext;
        removeHashBucketFromFreeList( sFreeHashBucket );
        mFreeBucketCount--;
    }
    else
    {
        IDE_TEST( iduMemMgr::malloc( mIndex,
                                     ID_SIZEOF( iduStringHashBucket ),
                                     ( void** )&sFreeHashBucket )
                  != IDE_SUCCESS );
    }

    initHashBucket( sFreeHashBucket );
    
    *aHashBucket = sFreeHashBucket;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduStringHash::freeHashBucket( iduStringHashBucket *aHashBucket )
{
#define IDE_FN "iduStringHash::freeHashBucket"
    IDE_MSGLOG_FUNC( IDE_MSGLOG_BODY( "" ) );

    if( mFreeBucketCount >= mCashBucketCount )
    {
        IDE_TEST( iduMemMgr::free( aHashBucket ) != IDE_SUCCESS );
    }
    else
    {
        mFreeBucketCount++;
        addHashBucketToFreeList( aHashBucket );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

