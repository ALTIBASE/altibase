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
 *
 * Description :
 *
 *      PROJ-2452 Result caching of Statement level
 *
 * 용어 설명 :
 *
 **********************************************************************/

#include <idl.h>
#include <iduMemory.h>
#include <qtc.h>
#include <qtcHash.h>

IDE_RC qtcHash::initialize( iduMemory     * aMemory,
                            UInt            aBucketCnt,
                            UInt            aKeyCnt,
                            UInt          * aRemainSize,
                            qtcHashTable ** aHashTable )
/***********************************************************************
 *
 * Description : STATE_MAKE_HASH_TABLE 일 때 수행되며
 *               cacheObj->mHashTable 을 생성한다.
 *
 * Implementation :
 *
 *             - alloc cacheObj->mHashTable
 *             - alloc cacheObj->mHashTable->mBucketArr
 *             - set cacheObj->mRemainSize
 *
 **********************************************************************/
{
    qtcHashTable * sHashTable = NULL;
    UInt sNeedSize = 0;
    UInt i = 0;

    IDE_ERROR_RAISE( aMemory != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aBucketCnt  > 0, ERR_UNEXPECTED_CACHE_ERROR ); 
    IDE_ERROR_RAISE( aKeyCnt     > 0, ERR_UNEXPECTED_CACHE_ERROR ); 
    IDE_ERROR_RAISE( aRemainSize != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( *aHashTable == NULL, ERR_UNEXPECTED_CACHE_ERROR );

    // Calculate need size
    sNeedSize +=
        ( ID_SIZEOF(qtcHashTable) + (ID_SIZEOF(qtcHashBucket) * aBucketCnt) );

    IDE_TEST( *aRemainSize < sNeedSize );

    IDU_FIT_POINT( "qtcHash::initialize::alloc1", idERR_ABORT_InsufficientMemory);

    // Alloc hash table
    IDE_TEST( aMemory->alloc( ID_SIZEOF(qtcHashTable), (void**)&sHashTable )
              != IDE_SUCCESS);

    // Alloc bucket array
    IDE_TEST( aMemory->alloc( ID_SIZEOF(qtcHashBucket) * aBucketCnt,
                              (void**)&sHashTable->mBucketArr )
              != IDE_SUCCESS);

    // Init bucket array
    for( i = 0; i < aBucketCnt; i++ )
    {
        sHashTable->mBucketArr[i].mRecordCnt = 0;
        IDU_LIST_INIT( &(sHashTable->mBucketArr[i].mRecordList) );
    }

    // Init hash table variables
    sHashTable->mKeyCnt    = aKeyCnt;
    sHashTable->mBucketCnt = aBucketCnt;
    sHashTable->mTotalRecordCnt = 0;

    *aHashTable = sHashTable;
    *aRemainSize -= sNeedSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcHash::initialize",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtcHash::insert( qtcHashTable  * aHashTable,
                        qtcHashRecord * aNewRecord )
/***********************************************************************
 *
 * Description : currRecord 를 hashTable 에 추가
 *
 * Implementation : 
 *
 *          - search bucket index
 *          - insert currRecord into bucket
 *
 **********************************************************************/
{
    iduList * sHeader = NULL;
    iduList * sNew    = NULL;
    iduList * sCurr   = NULL;
    UInt      sIndex;

    IDE_ERROR_RAISE( aNewRecord != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aHashTable != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aHashTable->mBucketArr != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aHashTable->mBucketCnt != 0, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aNewRecord->mKey > 0, ERR_UNEXPECTED_CACHE_ERROR );

    // Search bucket index
    sIndex  = hash( aNewRecord->mKey, aHashTable->mBucketCnt );

    sNew    = &(aNewRecord->mRecordListNode);
    sHeader = &(aHashTable->mBucketArr[sIndex].mRecordList);
    sCurr   = sHeader->mNext;

    while( ( sCurr != sHeader ) && 
           ( ((qtcHashRecord *)sCurr->mObj)->mKey < ((qtcHashRecord *)sNew->mObj)->mKey ) )
    {
        sCurr = sCurr->mNext;
    }

    // Insert into bucket
    IDU_LIST_ADD_BEFORE( sCurr, sNew );

    aHashTable->mBucketArr[sIndex].mRecordCnt++;
    aHashTable->mTotalRecordCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcHash::insert",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtcHash::compareKeyData( mtcStack      * aStack,
                                qtcHashRecord * aRecord,
                                UInt            aKeyCnt,
                                idBool        * aResult )
/***********************************************************************
 *
 * Description : PROJ-2452 DETERMINISTIC function caching
 *               stack 과 qtcHashRecord 의 비교 결과를 반환한다.
 *
 * Implementation : 
 *
 **********************************************************************/
{
    idBool sResult = ID_TRUE;
    UInt   sLength = 0;
    UInt   sOffset = 0;
    UInt   i = 0;

    IDE_ERROR_RAISE( aStack  != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aRecord != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aKeyCnt       > 0, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aRecord->mKey > 0, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aRecord->mKeyLengthArr != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aRecord->mKeyData      != NULL, ERR_UNEXPECTED_CACHE_ERROR );

    for ( i = 0; i < aKeyCnt; i++ )
    {
        sLength = aStack[i].column->module->actualSize( aStack[i].column,
                                                        aStack[i].value );

        IDE_ERROR_RAISE( sLength > 0, ERR_UNEXPECTED_CACHE_ERROR );

        // Compare key length
        if ( sLength == aRecord->mKeyLengthArr[i] )
        {
            // Compare key data
            if ( idlOS::memcmp( aStack[i].value,
                                (SChar *)aRecord->mKeyData + sOffset,
                                sLength ) == 0 )
            {
                sOffset += sLength;
            }
            else
            {
                sResult = ID_FALSE;
                break;
            }
        }
        else
        {
            sResult = ID_FALSE;
            break;
        }
    }

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtcHash::compareKeyData",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
