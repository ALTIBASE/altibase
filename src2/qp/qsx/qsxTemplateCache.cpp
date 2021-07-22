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

#include <qsxTemplateCache.h>
#include <qcg.h>

IDE_RC
qsxTemplateCache::build( qcStatement * aStatement,
                         qcTemplate  * aTemplate )
{
    UInt i;
    UInt sCount;
    qcTemplate * sTemplate;
    UInt sState = 0;

    IDU_LIST_INIT( &mCache );

    mCount = 0;
    sCount = QCU_PSM_TEMPLATE_CACHE_COUNT;

    sTemplate = QC_PRIVATE_TMPLATE(aStatement);

    IDE_TEST( mLock.initialize( (SChar*)"PSM template cache latch" )
              != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSX,
                                 ID_SIZEOF(iduMemory),
                                 (void**)&mCacheMemory )
                  != IDE_SUCCESS );

    mCacheMemory = new (mCacheMemory) iduMemory;

    (void)mCacheMemory->init( IDU_MEM_QSX );

    sState = 1;

    for( i  = 0; i < sCount; i++ )
    {
        IDE_TEST( qcg::allocPrivateTemplate( aStatement,
                                             mCacheMemory )
                  != IDE_SUCCESS );

        IDE_TEST( qcg::cloneAndInitTemplate( mCacheMemory,
                                             aTemplate,
                                             QC_PRIVATE_TMPLATE(aStatement) )
                  != IDE_SUCCESS );

        IDE_TEST( qsx::addObject( &mCache,
                                  QC_PRIVATE_TMPLATE(aStatement),
                                  mCacheMemory ) != IDE_SUCCESS );

        mCount++;
    }

    QC_PRIVATE_TMPLATE(aStatement) = sTemplate;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        mCacheMemory->freeAll( 0 );
        mCacheMemory->destroy();
        (void)iduMemMgr::free( mCacheMemory );
        mCacheMemory = NULL;
    }

    QC_PRIVATE_TMPLATE(aStatement) = sTemplate;

    return IDE_FAILURE;
}

IDE_RC
qsxTemplateCache::destroy()
{
    iduMemory   * sCacheMemory = mCacheMemory;

    if( sCacheMemory != NULL )
    {
#if defined(DEBUG)
        UInt          sCount = 0;
        iduListNode * sNode = NULL;

        IDU_LIST_ITERATE( &mCache, sNode )
        {
            sCount++;
        }

        IDE_DASSERT( sCount == mCount );
#endif

        sCacheMemory->freeAll( 0 );

        sCacheMemory->destroy();

        IDE_TEST( iduMemMgr::free( sCacheMemory ) != IDE_SUCCESS );

        IDU_LIST_INIT( &mCache );

        IDE_TEST( mLock.destroy() != IDE_SUCCESS );

        mCacheMemory = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxTemplateCache::get( qcTemplate  ** aTemplate,
                       iduListNode ** aCacheNode )
{
    iduList * sNode;

    if( mCacheMemory != NULL )
    {
        IDE_TEST( mLock.lockWrite( NULL,
                                   NULL ) != IDE_SUCCESS );

        if( IDU_LIST_IS_EMPTY( &mCache ) != ID_TRUE )
        {
            sNode = IDU_LIST_GET_FIRST( &mCache );

            *aTemplate  = (qcTemplate *)(sNode->mObj);

            *aCacheNode = sNode;

            IDU_LIST_REMOVE( sNode );
        }
        else
        {
            *aCacheNode = NULL;
        }

        IDE_TEST( mLock.unlock() != IDE_SUCCESS );
    }
    else
    {
        *aCacheNode = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC
qsxTemplateCache::put( iduListNode * aCacheNode )
{
    IDE_TEST( mLock.lockWrite( NULL,
                               NULL ) != IDE_SUCCESS );

    IDU_LIST_ADD_FIRST( &mCache, aCacheNode );

    IDE_TEST( mLock.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}
