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
 * $Id $
 **********************************************************************/

#include <sdpDef.h>
#include <smiMisc.h>
#include <smuProperty.h>
#include <sdtHashModule.h>
#include <sdtSortSegment.h>
#include <sdpTableSpace.h>
#include <sdtWAExtentMgr.h>
#include <sdptbExtent.h>

UInt           sdtWAExtentMgr::mWAExtentCount;
UInt           sdtWAExtentMgr::mMaxWAExtentCount;
iduMutex       sdtWAExtentMgr::mMutex;
iduStackMgr    sdtWAExtentMgr::mFreeExtentPool;
iduMemPool     sdtWAExtentMgr::mNExtentArrPool;

/******************************************************************
 * WorkArea
 ******************************************************************/
/**************************************************************************
 * Description :
 * 처음 서버 구동할때 호출된다.
 * 변수값을 초기화하고 Mutex를 만들고 WA Extent 를 생성한다.
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::initializeStatic()
{
    UChar        *sExtent = NULL;
    UInt          sState = 0;
    UInt          i;
    idBool        sIsEmpty;

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    IDE_TEST_RAISE( smuProperty::getMaxTotalWASize() >
                    ( idlOS::sysconf(_SC_PHYS_PAGES) * idlOS::sysconf(_SC_PAGESIZE) ),
                    ERR_ABORT_INTERNAL_MAXSIZE );
#endif

    IDE_TEST( mMutex.initialize( (SChar*)"SDT_WA_EXTENT_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    mWAExtentCount    = calcWAExtentCount( getInitTotalWASize() );
    mMaxWAExtentCount = 0;

    /************************* WorkArea ***************************/
    IDE_TEST( mFreeExtentPool.initialize( IDU_MEM_SM_TEMP,
                                          ID_SIZEOF( UChar* ) )
              != IDE_SUCCESS );
    sState = 1;

    for( i = 0 ; i < getWAExtentCount() ; i++ )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                     ID_SIZEOF(sdtWAExtent),
                                     (void**)&sExtent )
                  != IDE_SUCCESS );

        IDE_TEST( mFreeExtentPool.push( ID_FALSE, // lock
                                        (void*) &sExtent )
                  != IDE_SUCCESS );
        sExtent = NULL;

        /* 1개 malign하고 2개째에서 실패하도록 설정 */
        // TC/FIT/Limit/sm/sdt/sdtWAExtentMgr_initializeStatic_malign.tc
        IDU_FIT_POINT_RAISE( "sdtWAExtentMgr::initializeStatic::malign", memory_allocate_failed );
    }

    IDE_TEST( mNExtentArrPool.initialize(
                  IDU_MEM_SM_TEMP,
                  (SChar*)"SDT_NEXTENT_ARRAY_POOL",
                  ID_SCALABILITY_SYS,
                  ID_SIZEOF( sdtNExtentArr ),
                  64,
                  IDU_AUTOFREE_CHUNK_LIMIT,             /* ChunkLimit */
                  ID_TRUE,                              /* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,      /* AlignByte */
                  ID_FALSE,                             /* ForcePooling */
                  ID_TRUE,                              /* GarbageCollection */
                  ID_TRUE,                              /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY )
              != IDE_SUCCESS);
    sState = 2;

    IDE_TEST( sdtHashModule::initializeStatic() != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( sdtSortSegment::initializeStatic() != IDE_SUCCESS );
    sState = 4;

    return IDE_SUCCESS;

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    IDE_EXCEPTION( ERR_ABORT_INTERNAL_MAXSIZE )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InternalServerErrorWithString, 
                                  " The supplied value was beyond the bounds of MaxTotalWASize" ) );
    }
#endif

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( memory_allocate_failed );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
#endif
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 4:
            sdtSortSegment::destroyStatic() ;
        case 3:
            sdtHashModule::destroyStatic() ;
        case 2:
            mNExtentArrPool.destroy();
        case 1:
            if ( sExtent != NULL )
            {
                // Push 하다가 실패한 것
                (void) iduMemMgr::free( (void*)sExtent );
            }

            while( 1 )
            {
                IDE_ASSERT( mFreeExtentPool.pop( ID_FALSE, // lock
                                                 (void*) &sExtent,
                                                 &sIsEmpty )
                            == IDE_SUCCESS );

                if ( sIsEmpty == ID_FALSE )
                {
                    (void) iduMemMgr::free( (void*)sExtent );
                    mWAExtentCount--;
                }
                else
                {
                    break;
                }
            }
            IDE_DASSERT( getWAExtentCount() == 0 );
            mFreeExtentPool.destroy();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 서버 내릴때 호출된다. WA Extent와 Mutex를 free 한다.
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::destroyStatic()
{
    idBool     sIsEmpty;
    UChar    * sExtent;

    sdtHashModule::destroyStatic();
    sdtSortSegment::destroyStatic();

    while( 1 )
    {
        IDE_ASSERT( mFreeExtentPool.pop( ID_FALSE, // lock
                                         (void*) &sExtent,
                                         &sIsEmpty )
                    == IDE_SUCCESS );

        if ( sIsEmpty == ID_FALSE )
        {
            (void)iduMemMgr::free( (void*)sExtent );
            mWAExtentCount-- ;
        }
        else
        {
            break;
        }
    }

    IDE_DASSERT( getWAExtentCount() == 0 );
    mWAExtentCount = 0;

    IDE_ASSERT( mFreeExtentPool.destroy() == IDE_SUCCESS );
    IDE_ASSERT( mNExtentArrPool.destroy() == IDE_SUCCESS );
    IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

/**************************************************************************
 * Description : Start start시점에 Temp tablespace의 모든 Extent를
 *               모두 미리 확보한다.
 *
 * aStatistics - [IN] 통계정보
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::prepareCachedFreeNExts( idvSQL  * aStatistics )
{
    sctTableSpaceNode*  sCurrSpaceNode;

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while ( sCurrSpaceNode != NULL )
    {
        switch( sCurrSpaceNode->mType )
        {
            case SMI_DISK_SYSTEM_TEMP:
            case SMI_DISK_USER_TEMP:

                IDE_TEST( sdptbExtent::prepareCachedFreeExts( aStatistics,
                                                              sCurrSpaceNode )
                          != IDE_SUCCESS );
                break;
            default:
                break;
        }

        sCurrSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Total WA Size property 에 맞춰서 Total WA를 재구축 한다.
 *
 * aNewSize  - [IN] 새로운 Total WA 크기
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::resizeWAExtentPool( ULong aNewSize )
{
    SLong   sNewExtentCount = calcWAExtentCount( aNewSize );
    SLong   sOldExtentCount ;
    SLong   sPushCount;
    SLong   sPopCount;
    SLong   i;
    UChar * sExtent = NULL;
    idBool  sIsEmpty;
    SLong   sExtentCount;
    UInt    sState = 0;

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    IDE_TEST_RAISE( aNewSize >
                    ( idlOS::sysconf(_SC_PHYS_PAGES) * idlOS::sysconf(_SC_PAGESIZE) ),
                    ERR_ABORT_INTERNAL_MAXSIZE );
#endif

    sOldExtentCount = calcWAExtentCount( getInitTotalWASize() );

    if ( sNewExtentCount > sOldExtentCount )
    {
        sState = 1;
        sExtentCount = sNewExtentCount - sOldExtentCount;

        for( i = 0 ; i < sExtentCount ; i++ )
        {
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                         ID_SIZEOF(sdtWAExtent),
                                         (void**)&sExtent )
                      != IDE_SUCCESS );

            IDE_TEST( mFreeExtentPool.push( ID_FALSE, // lock
                                            (void*)&sExtent )
                      != IDE_SUCCESS );

            mWAExtentCount++;
            sExtent = NULL;

            /* 1개 malign하고 2개째에서 실패하도록 설정 */
            // TC/FIT/Limit/sm/sdt/sdtWAExtentMgr_resizeWAExtentPool_malign.tc
            IDU_FIT_POINT_RAISE( "sdtWAExtentMgr::resizeWAExtentPool::malign", memory_allocate_failed );
        }
        sState = 0;
    }
    else
    {
        if ( sNewExtentCount < sOldExtentCount )
        {
            sState = 2;
            sExtentCount = sOldExtentCount - sNewExtentCount;

            for( i = 0; i < sExtentCount ; i++ )
            {
                IDE_TEST( mFreeExtentPool.pop( ID_FALSE, // lock
                                               (void*)&sExtent,
                                               &sIsEmpty )
                          != IDE_SUCCESS );

                if ( sIsEmpty == ID_FALSE )
                {
                    mWAExtentCount--;
                    (void)iduMemMgr::free( (void*)sExtent );
                }
                else
                {
                    // 만약 용 중이라면 해제 하지 못할수도 있다.
                    // 하지만 temp table drop시점에 mWAExtentCount를 보고
                    // free할 수 있으므로, 상관없다.
                    break;
                }
            }
            sState = 0;
        }
    }

    IDE_DASSERT( mFreeExtentPool.getTotItemCnt() <= mWAExtentCount );

    return IDE_SUCCESS;

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    IDE_EXCEPTION( ERR_ABORT_INTERNAL_MAXSIZE )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InternalServerErrorWithString, 
                                  " The supplied value was beyond the bounds of MaxTotalWASize" ) );
    }
#endif
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( memory_allocate_failed );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
#endif
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2 :
            {
                sPopCount = sOldExtentCount - getWAExtentCount();

                for( i = 0 ; i < sPopCount ; i++ )
                {
                    IDE_ASSERT( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                                   ID_SIZEOF(sdtWAExtent),
                                                   (void**)&sExtent )
                                == IDE_SUCCESS );

                    IDE_ASSERT( mFreeExtentPool.push( ID_FALSE, // lock
                                                      (void*) &sExtent )
                                == IDE_SUCCESS );
                    mWAExtentCount++;
                }
            }
            break;
        case 1 :
            {
                if ( sExtent != NULL )
                {
                    // Push 하다가 실패한 것
                    (void)iduMemMgr::free( (void*)sExtent );
                }

                sPushCount = getWAExtentCount() - sOldExtentCount ;

                for( i = 0; i < sPushCount ; i++ )
                {
                    IDE_ASSERT( mFreeExtentPool.pop( ID_FALSE, // lock
                                                     (void*)&sExtent,
                                                     &sIsEmpty )
                                == IDE_SUCCESS );

                    if ( sIsEmpty == ID_FALSE )
                    {
                        mWAExtentCount--;
                        (void)iduMemMgr::free( (void*)sExtent );
                    }
                    else
                    {
                        // 만약 용 중이라면 해제 하지 못할수도 있다.
                        // 하지만 temp table drop시점에 mWAExtentCount를 보고
                        // free할 수 있으므로, 상관없다.
                        break;
                    }
                }
            }
            break;
        default:
            break;
    }

    IDE_DASSERT( mFreeExtentPool.getTotItemCnt() <= mWAExtentCount );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : WAExtent들을 할당한다.
 *
 * aStatistics        - 
 * aStatsPtr          -
 * aWAExtentInfo      -
 * aInitWAExtentCount - 
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::initWAExtents( idvSQL            * aStatistics,
                                      smiTempTableStats * aStatsPtr,
                                      sdtWAExtentInfo   * aWAExtentInfo,
                                      UInt                aInitWAExtentCount )
{
    PDL_Time_Value   sTV;
    sdtWAExtent    * sWAExtent;
    idBool           sIsEmpty;
    idBool           sIsLock     = ID_FALSE;
    UInt             sAllocState = 0;
    UInt             sTryCount4AllocWExtent = 0;
    SInt             sOverInitWAExtentCount;

    IDE_ERROR( aInitWAExtentCount  > 0 );
    IDE_ERROR( aWAExtentInfo->mTail == NULL );

    // 초과 할당 가능한지 확인
    sOverInitWAExtentCount = smuProperty::getTmpOverInitWAExtCnt();
    sTV.set(0, smuProperty::getTempSleepInterval() );

    lock();
    sIsLock = ID_TRUE;
    IDE_DASSERT( mFreeExtentPool.getTotItemCnt() <= mWAExtentCount );
    /***************************WAExtent 할당****************************/

    while( 1 )
    {
        IDE_TEST( mFreeExtentPool.pop( ID_FALSE, // lock
                                       (void*) &sWAExtent,
                                       &sIsEmpty )
                  != IDE_SUCCESS );

        if ( sIsEmpty == ID_FALSE )
        {
            sAllocState = 1;
            break;
        }

        if (( calcWAExtentCount( smuProperty::getMaxTotalWASize() ) > getWAExtentCount() ) ||
            ( sOverInitWAExtentCount > 0 ))
        {
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                         ID_SIZEOF(sdtWAExtent),
                                         (void**)&sWAExtent )
                      != IDE_SUCCESS );
            mWAExtentCount++;
            sAllocState = 2;
            break;
        }

        sIsLock = ID_FALSE;
        unlock();

        idlOS::sleep( sTV );

        aStatsPtr->mAllocWaitCount++;
        sTryCount4AllocWExtent++;

        IDE_TEST_RAISE( sTryCount4AllocWExtent >
                        smuProperty::getTempAllocTryCount(),
                        ERROR_NOT_ENOUGH_WORKAREA );

        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS);

        lock();
        sIsLock = ID_TRUE;
    }

    sIsLock = ID_FALSE;
    unlock();

    sWAExtent->mNextExtent = NULL;
    aWAExtentInfo->mHead   = sWAExtent;
    aWAExtentInfo->mTail   = sWAExtent;
    aWAExtentInfo->mCount  = 1;

    sAllocState = 3;

    if ( allocWAExtents( aWAExtentInfo,
                         aInitWAExtentCount - 1 ) != IDE_SUCCESS )
    {
        IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_WORKAREA );

        if ( sOverInitWAExtentCount > 0 )
        {
            // sOverInitWAExtentCount 가 설정되어 있으면
            // sOverInitWAExtentCount 보다 한개 적게 할당한다.
            // 0 이었으면 - 가 되어서 통과
            if ( sOverInitWAExtentCount > (SInt)aInitWAExtentCount )
            {
                sOverInitWAExtentCount = (SInt)aInitWAExtentCount;
            }

            aStatsPtr->mOverAllocCount++;
            while( --sOverInitWAExtentCount > 0 )
            {
                IDE_TEST( memAllocWAExtent( aWAExtentInfo ) != IDE_SUCCESS );
                aStatsPtr->mOverAllocCount++;
            }
        }
        else
        {
            // sOverInitWAExtentCount 가 설정되어 있지 않으면.
            // TempAllocTryCount 동안 재시도한다.
            while ( allocWAExtents( aWAExtentInfo,
                                    aInitWAExtentCount - 1 ) != IDE_SUCCESS )
            {
                IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_WORKAREA );

                idlOS::sleep( sTV );

                aStatsPtr->mAllocWaitCount++;
                sTryCount4AllocWExtent++;

                IDE_TEST( sTryCount4AllocWExtent > smuProperty::getTempAllocTryCount() );
                IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NOT_ENOUGH_WORKAREA );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_ENOUGH_WORKAREA ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        unlock();
    }

    switch( sAllocState )
    {
        case 3:
            (void)freeWAExtents( aWAExtentInfo );
            break;
        case 2:
            (void)iduMemMgr::free( sWAExtent );
            mWAExtentCount--;
            break;
        case 1:
            lock();
            (void)mFreeExtentPool.push( ID_FALSE, // lock
                                        (void*) &sWAExtent );
            unlock();
            break;
        default:
            break;
    }
    return IDE_FAILURE;
}


/**************************************************************************
 * Description :  WAExtent들을 할당한다.
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::allocWAExtents( sdtWAExtentInfo * aWAExtentInfo,
                                       UInt              aGetWAExtentCount )
{
    sdtWAExtent    * sWAExtent;
    idBool           sIsEmpty;
    idBool           sIsLock = ID_FALSE;
    UInt             sExceedableExtentCount;
    UInt             sAllocWAExtentCount;
    UInt             sPopWAExtentCount;

    IDE_ERROR( aGetWAExtentCount   > 0 );
    IDE_ERROR( aWAExtentInfo->mTail != NULL );
    /* 아예 할당하기에 WA가 부족한 상황 */

    // 초과 할당(malloc) 가능한 WAExtent 갯수 확인
    sExceedableExtentCount = calcWAExtentCount( smuProperty::getMaxTotalWASize() ) ;

    if ( sExceedableExtentCount > getWAExtentCount() )
    {
        sExceedableExtentCount -= getWAExtentCount();
    }
    else
    {
        sExceedableExtentCount = 0;
    }

    lock();
    sIsLock = ID_TRUE;

    /***************************WAExtent 할당****************************/

    /* 할당 가능한 갯수 가 요청한 수 보다 부족한 경우. */
    IDE_TEST_RAISE( ( aGetWAExtentCount > ( getFreeWAExtentCount() + sExceedableExtentCount ) ),
                    ERROR_NOT_ENOUGH_WORKAREA );

    if ( aGetWAExtentCount > getFreeWAExtentCount() )
    {
        // free wa extent를 넘어서 초과 할당하는 경우,
        // init total wa extent 만으로는 부족한 경우
        sAllocWAExtentCount = aGetWAExtentCount - getFreeWAExtentCount();
        sPopWAExtentCount = getFreeWAExtentCount();

        // TC/FIT/Server/sm/Bugs/BUG-45857/BUG-45857.tc
        IDU_FIT_POINT_RAISE( "BUG-45857@sdtWAExtentMgr::allocWAExtents::ERROR_NOT_ENOUGH_WORKAREA",
                             ERROR_NOT_ENOUGH_WORKAREA );
    }
    else
    {
        sAllocWAExtentCount = 0;
        sPopWAExtentCount = aGetWAExtentCount;
    }

    while( sPopWAExtentCount-- > 0 )
    {
        IDU_FIT_POINT( "1.BUG-47334@sdtWAExtentMtr::allocWAExtents::pop" );
        IDE_TEST( mFreeExtentPool.pop( ID_FALSE, // lock
                                       (void*) &sWAExtent,
                                       &sIsEmpty )
                  != IDE_SUCCESS );

        IDE_ERROR( sIsEmpty == ID_FALSE ); /* 반드시 성공해야함 */

        aWAExtentInfo->mTail->mNextExtent = sWAExtent;
        aWAExtentInfo->mTail = sWAExtent;
        aWAExtentInfo->mCount++;
    }

    while( sAllocWAExtentCount-- > 0 )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                     ID_SIZEOF(sdtWAExtent),
                                     (void**)&sWAExtent )
                  != IDE_SUCCESS );

        aWAExtentInfo->mTail->mNextExtent = sWAExtent;
        aWAExtentInfo->mTail = sWAExtent;
        aWAExtentInfo->mCount++;
        mWAExtentCount++;
    }

    IDE_DASSERT( mFreeExtentPool.getTotItemCnt() <= mWAExtentCount );

    updateMaxWAExtentCount();

    sWAExtent->mNextExtent = NULL; // 마지막 Extent는 Next가 Null

    sIsLock = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NOT_ENOUGH_WORKAREA );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_ENOUGH_WORKAREA ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        unlock();
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WAExtent 한개를 할당한다. Total WA Extent가 full인 상황애서
 * 하나만 추가 할당한다.
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::memAllocWAExtent( sdtWAExtentInfo * aWAExtentInfo )
{
    sdtWAExtent    * sWAExtent;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                 ID_SIZEOF(sdtWAExtent),
                                 (void**)&sWAExtent )
              != IDE_SUCCESS );

    aWAExtentInfo->mTail->mNextExtent = sWAExtent;
    aWAExtentInfo->mTail = sWAExtent;
    aWAExtentInfo->mCount++;
    sWAExtent->mNextExtent = NULL; // 마지막 Extent는 Next가 Null

    lock();

    mWAExtentCount++;
    updateMaxWAExtentCount();

    IDE_DASSERT( mFreeExtentPool.getTotItemCnt() <= mWAExtentCount );

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/**************************************************************************
 * Description :
 * WAExtent들을 반환한다.
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::freeWAExtents( sdtWAExtentInfo * aWAExtentInfo )
{
    sdtWAExtent    * sWAExtent;
    sdtWAExtent    * sNxtWAExtent;
    UInt             sLockState = 0;
    ULong            sTargetExtentCount;
    UInt             sExtentCount = 0;

    lock();
    sLockState = 1;

    sTargetExtentCount = calcWAExtentCount( getInitTotalWASize() );

    // 0번 Exntet에 WA Segment가 있으므로 마지막에 해제하여야 한다.
    sWAExtent = aWAExtentInfo->mHead ;

    while( sWAExtent != NULL )
    {
        if ( getWAExtentCount() > sTargetExtentCount )
        {
            sNxtWAExtent = sWAExtent->mNextExtent;

            mWAExtentCount--;
            IDE_TEST( iduMemMgr::free( sWAExtent ) != IDE_SUCCESS );

            sWAExtent = sNxtWAExtent;
        }
        else
        {
            IDE_TEST( mFreeExtentPool.push( ID_FALSE, // lock
                                            (void*) &sWAExtent )
                      != IDE_SUCCESS );

            sWAExtent = sWAExtent->mNextExtent;
        }
        sExtentCount++;
    }

    IDE_ASSERT( aWAExtentInfo->mCount == sExtentCount );

    IDE_DASSERT( mFreeExtentPool.getTotItemCnt() <= mWAExtentCount );

    sLockState = 0;
    unlock();

    aWAExtentInfo->mHead  = NULL;
    aWAExtentInfo->mTail  = NULL;
    aWAExtentInfo->mCount = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLockState == 1 )
    {
        unlock();
    }

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * WAExtent들을 반환한다.
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::freeAllNExtents( scSpaceID           aSpaceID,
                                        sdtNExtFstPIDList * aNExtFstPIDList )
{
    sdtNExtentArr * sNxtNExtArr;
    sdtNExtentArr * sCurNExtArr;
    UInt            i;

    if ( aNExtFstPIDList->mHead != NULL )
    {
        IDE_ASSERT( aNExtFstPIDList->mTail != NULL );

        sCurNExtArr = aNExtFstPIDList->mHead;

        while( sCurNExtArr != aNExtFstPIDList->mTail )
        {
            for( i = 0 ; i < SDT_NEXTARR_EXTCOUNT ; i++ )
            {
                (void)sdptbExtent::freeTmpExt( aSpaceID,
                                               sCurNExtArr->mMap[ i ] );
            }

            sNxtNExtArr = sCurNExtArr->mNextArr;
            IDE_TEST( mNExtentArrPool.memfree( (void*)sCurNExtArr ) != IDE_SUCCESS );
            sCurNExtArr = sNxtNExtArr;
        }

        for( i = 0 ; i < ( aNExtFstPIDList->mCount % SDT_NEXTARR_EXTCOUNT ) ; i++ )
        {
            (void)sdptbExtent::freeTmpExt( aSpaceID,
                                           sCurNExtArr->mMap[ i ] );
        }

        IDE_TEST( mNExtentArrPool.memfree( (void*)sCurNExtArr ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 해당 SpaceID로부터 빈 NormalExtent를 가져온다. (sdptbExtent::allocExts
 * 사용 )
 *
 * 예외가 발생한다고 해도 WASegment정리 할 때 정리한다.
 * 즉 여기에서는 NExtent할당한 것에 대한 예외 처리를 하지 않는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::allocFreeNExtent( idvSQL            * aStatistics,
                                         smiTempTableStats * aStatsPtr,
                                         scSpaceID           aSpaceID,
                                         sdtNExtFstPIDList * aNExtFstPIDList )
{
    sdpExtDesc        sExtDesc;
    sdtNExtentArr   * sNExtentArr;

    if (( aNExtFstPIDList->mCount % SDT_NEXTARR_EXTCOUNT ) == 0 )
    {
        IDE_TEST( mNExtentArrPool.alloc( (void**)&sNExtentArr ) != IDE_SUCCESS );
        aStatsPtr->mRuntimeMemSize += ID_SIZEOF( sdtNExtentArr );    

        if ( aNExtFstPIDList->mTail != NULL )
        {
            IDE_ASSERT( aNExtFstPIDList->mHead != NULL );

            aNExtFstPIDList->mTail->mNextArr = sNExtentArr;
        }
        else
        {
            IDE_ASSERT( aNExtFstPIDList->mHead == NULL );

            aNExtFstPIDList->mHead = sNExtentArr;
        }
        aNExtFstPIDList->mTail = sNExtentArr;
        sNExtentArr->mNextArr = NULL;
    }
    // TC/FIT/Server/sm/Bugs/BUG-45857/BUG-45857.tc
    IDU_FIT_POINT( "BUG-45857@sdtWAExtentMgr::allocFreeNExtent::ERROR_NOT_ENOUGH_NEXTENTSIZE" );

    IDE_TEST( sdptbExtent::allocTmpExt( aStatistics,
                                        aSpaceID,
                                        (sdpExtDesc*)&sExtDesc ) != IDE_SUCCESS );

    IDE_ASSERT( sExtDesc.mLength == SDT_WAEXTENT_PAGECOUNT );
    IDE_ASSERT( sExtDesc.mExtFstPID != SM_NULL_PID );

    aNExtFstPIDList->mTail->mMap[ aNExtFstPIDList->mCount % SDT_NEXTARR_EXTCOUNT ] = sExtDesc.mExtFstPID;
    aNExtFstPIDList->mLastFreeExtFstPID = sExtDesc.mExtFstPID;
    aNExtFstPIDList->mCount++;
    aNExtFstPIDList->mPageSeqInLFE = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * X$Tempinfo를 위해 레코드 구축
 ***************************************************************************/
IDE_RC sdtWAExtentMgr::buildTempInfoRecord( void                * aHeader,
                                            iduFixedTableMemory * aMemory )
{
    smiTempInfo4Perf sInfo;
    UInt             sLockState = 0;

    lock();
    sLockState = 1;

    SMI_TT_SET_TEMPINFO_UINT( "TOTAL WA EXTENT COUNT", getWAExtentCount(), "EXTENT" );
    SMI_TT_SET_TEMPINFO_UINT( "FREE WA EXTENT COUNT", getFreeWAExtentCount(), "EXTENT" );
    SMI_TT_SET_TEMPINFO_ULONG( "MAX USED TOTAL WA SIZE", (ULong)getMaxWAExtentCount() * SDT_WAEXTENT_SIZE, "BYTES" );
    SMI_TT_SET_TEMPINFO_UINT( "EXTENT SIZE", SDT_WAEXTENT_SIZE, "BYTES" );

    sLockState = 0;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLockState )
    {
        case 1:
            unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}



void sdtWAExtentMgr::dumpNExtentList( sdtNExtFstPIDList & aNExtentList,
                                      SChar             * aOutBuf,
                                      UInt                aOutSize )
{
    sdtNExtentArr    * sCurNExtArr;
    UInt               i;
    UInt               sTotal;

    if ( aNExtentList.mCount > 0 )
    {
        IDE_ASSERT( aNExtentList.mHead != NULL );
        IDE_ASSERT( aNExtentList.mTail != NULL );

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "DUMP NORMAL EXTENT FIRST PAGEID MAP:\n"
                                   "       Count   : %"ID_UINT64_FMT"\n"
                                   "       FirstSeq: %"ID_UINT32_FMT"\n"
                                   "       LstFreeExtFirstPID: %"ID_UINT32_FMT"\n"
                                   "       FirstPID: [ 0 ] ",
                                   aNExtentList.mCount,
                                   aNExtentList.mPageSeqInLFE,
                                   aNExtentList.mLastFreeExtFstPID );
        sTotal = 0;
        for(  sCurNExtArr = aNExtentList.mHead;
              sCurNExtArr != aNExtentList.mTail;
              sCurNExtArr = sCurNExtArr->mNextArr, sTotal+= SDT_NEXTARR_EXTCOUNT )
        {
            for( i = 0 ; i < SDT_NEXTARR_EXTCOUNT ; i++ )
            {
                if ( ( i % 10 ) == 9 )
                {
                    (void)idlVA::appendFormat( aOutBuf,
                                               aOutSize,
                                               "%"ID_UINT32_FMT"\n"
                                               "[%3"ID_UINT32_FMT"] ",
                                               sCurNExtArr->mMap[i],
                                               i + sTotal );
                }
                else
                {
                    (void)idlVA::appendFormat( aOutBuf,
                                               aOutSize,
                                               "%"ID_UINT32_FMT", ",
                                               sCurNExtArr->mMap[i] );
                }
            }
        }

        for( i = 0 ; i < ( aNExtentList.mCount % SDT_NEXTARR_EXTCOUNT ) ; i++ )
        {
            if ( ( i % 10 ) == 9 )
            {
                (void)idlVA::appendFormat( aOutBuf,
                                           aOutSize,
                                           "%"ID_UINT32_FMT"\n"
                                           "[%3"ID_UINT32_FMT"] ",
                                           sCurNExtArr->mMap[i],
                                           i + sTotal );
            }
            else
            {
                (void)idlVA::appendFormat( aOutBuf,
                                           aOutSize,
                                           "%"ID_UINT32_FMT", ",
                                           sCurNExtArr->mMap[i] );
            }
        }

        (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n" );
    }

    return;
}

