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
 * $id$
 **********************************************************************/

#include <ide.h>
#include <idu.h>

#include <mtd.h>

#include <smi.h>

#include <dkErrorCode.h>

#include <dkm.h>
#include <dkisIterator.h>

static iduMemPool gIteratorPool;

/*
 *
 */
#ifdef ALTIBASE_PRODUCT_XDB
static IDE_RC dkisPrepareIteratorMem( idvSQL           * /* aStatistics */,
                                      smnStIndexHeader * /* aIndex */,
                                      void             * /* aModule */ )
#else
static IDE_RC dkisPrepareIteratorMem( const smnIndexModule * )
#endif
{
    IDE_TEST( gIteratorPool.initialize( IDU_MEM_DK,
                                        (SChar *)"DK_ITERATOR_POOL",
                                        1,
                                        ID_SIZEOF( dkisIterator ),
                                        8,
                                        IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                        ID_TRUE,							/* UseMutex */
                                        IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                        ID_FALSE,							/* ForcePooling */
                                        ID_TRUE,							/* GarbageCollection */
                                        ID_TRUE,                          /* HWCacheLine */
                                        IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
#ifdef ALTIBASE_PRODUCT_XDB
static IDE_RC dkisReleaseIteratorMem( idvSQL           * /* aStatistics */,
                                      smnStIndexHeader * /* aIndex */,
                                      void             * /* aModule */ )
#else
static IDE_RC dkisReleaseIteratorMem( const smnIndexModule * )
#endif
{
    IDE_TEST( gIteratorPool.destroy() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;    
}

/*
 *
 */ 
static IDE_RC dkisAA( void * /*aIterator*/, const void  ** /*aRow*/)
{
    return IDE_SUCCESS;
}

/*
 *
 */ 
static IDE_RC dkisNA( void )
{
    IDE_SET( ideSetErrorCode( dkERR_ABORT_TRAVERSE_NOT_APPLICABLE ) );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
static IDE_RC dkisBeforeFirst( void * aIterator,
                               const smSeekFunc ** aSeekFunc )
{
    dkisIterator * sIterator = (dkisIterator *)aIterator;
    
    IDE_TEST( dkmIsFirst( sIterator->mSession,
                          sIterator->mStatementId )
              != IDE_SUCCESS );

    *aSeekFunc += 6;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
static IDE_RC dkisFetchNext( void * aIterator,
                             const void       ** aRow)
{
    dkisIterator * sIterator = (dkisIterator *)aIterator;
    idBool sLoopFlag = ID_TRUE;
    idBool sResult = ID_TRUE;
    
    while ( sLoopFlag )
    {
        IDE_TEST( dkmFetchNextRow( sIterator->mSession,
                                   sIterator->mStatementId,
                                   (void **)aRow )
                  != IDE_SUCCESS );
        
        if ( *aRow != NULL &&
             sIterator->mFilter != NULL )
        {
            IDE_TEST( sIterator->mFilter->callback( &sResult,
                                                    *aRow,
                                                    NULL,
                                                    0,
                                                    SC_NULL_GRID,
                                                    sIterator->mFilter->data )
                      != IDE_SUCCESS );
            if ( sResult == ID_TRUE )
            {
                break;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            sLoopFlag = ID_FALSE;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

static const smSeekFunc dkisSeekFunctions[12] =
{ /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
    (smSeekFunc)dkisNA,
    (smSeekFunc)dkisNA,
    (smSeekFunc)dkisBeforeFirst,
    (smSeekFunc)dkisNA,
    (smSeekFunc)dkisAA,
    (smSeekFunc)dkisAA,
    (smSeekFunc)dkisFetchNext,
    (smSeekFunc)dkisNA,
    (smSeekFunc)dkisNA,
    (smSeekFunc)dkisNA,
    (smSeekFunc)dkisAA,
    (smSeekFunc)dkisAA
};

/*
 *
 */ 
static IDE_RC dkisInit( dkisIterator          * aIterator,
                        void                  * aTrans,
                        void                  * aTable,
                        smnIndexHeader        * /*aIndex*/,
                        void                  * /*aDumpObject*/,
                        const smiRange        * /*aRange*/,
                        const smiRange        * /* aKeyFilter */,
                        const smiCallBack     * /*aFilter*/,
                        UInt                    /*aFlag*/,
                        smSCN                   aSCN,
                        smSCN                   aInfinite,
                        idBool                  /*aUntouchable*/,
                        smiCursorProperties   * aProperties,
                        const smSeekFunc     ** aSeekFunc,
                        smiStatement          * aStatement )
{
    aIterator->mIterator.SCN = aSCN;
    aIterator->mIterator.infinite = aInfinite;
    aIterator->mIterator.trans = aTrans;
    aIterator->mIterator.table = aTable;
    aIterator->mIterator.curRecPtr = NULL;
    aIterator->mIterator.lstFetchRecPtr = NULL;
    aIterator->mIterator.tid = SM_NULL_TID;
    aIterator->mIterator.properties = aProperties;
    aIterator->mIterator.mStatement = aStatement; 
    
    SC_MAKE_GRID( aIterator->mIterator.mRowGRID,
                  SC_NULL_SPACEID,
                  SC_NULL_PID,
                  SC_NULL_OFFSET );
    
    aIterator->mIterator.flag = 0;

    IDE_TEST( dkmRestartRow( aIterator->mSession,
                             aIterator->mStatementId )
              != IDE_SUCCESS );
    
    *aSeekFunc = dkisSeekFunctions;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
        
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkisDest( dkisIterator * /*aIterator*/ )
{
    return IDE_SUCCESS;
}


/*
 *
 */ 
static IDE_RC dkisFreeIterator( void * aIterator )
{
    dkisIterator * sIterator = (dkisIterator *)aIterator;

    IDE_TEST( gIteratorPool.memfree( sIterator ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
static IDE_RC dkisAllocIterator( void ** aIterator )
{
    dkisIterator * sIterator = NULL;

    IDE_TEST_RAISE( gIteratorPool.alloc( (void **)&sIterator )
                    != IDE_SUCCESS, ERROR_ALLOC_MEMORY );

    *((dkisIterator **)aIterator) = sIterator;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_ALLOC_MEMORY )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;    
}

#ifdef ALTIBASE_PRODUCT_XDB

static smnIndexModule gRemoteTableIndexModule = {
    
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_REMOTE,
                              SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( dkisIterator ),
    (smnMemoryFunc) dkisPrepareIteratorMem,
    (smnMemoryFunc) dkisReleaseIteratorMem,
    (smnPrepareNodeMemoryFunc) NULL,
    (smnDestroyNodeMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc)   NULL,
    (smnAttachIndexFunc) NULL,
    (smnDetachIndexFunc) NULL,
    (smnDetachAttachIndexFunc) NULL,

    (smTableCursorLockRowFunc) NULL,
    (smnInsertFunc) NULL,
    (smnDeleteFunc) NULL,
    (smnFreeFunc)   NULL,
    (smnExistKeyFunc) NULL,          // slot이 존재하는지 확인.    
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             NULL,

    (smInitFunc)   dkisInit,
    (smDestFunc)   dkisDest,
    (smnFreeNodeFunc) NULL,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc) NULL,
    (smnSetPositionFunc) NULL,

    (smnAllocIteratorFunc) dkisAllocIterator,
    (smnFreeIteratorFunc)  dkisFreeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,

    (smnMakeDiskImageFunc)  NULL,
    (smnBuildIndexFunc)	    NULL,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         NULL
};

#else

static smnIndexModule gRemoteTableIndexModule = {
    
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_REMOTE,
                              SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( dkisIterator ),
    (smnMemoryFunc) dkisPrepareIteratorMem,
    (smnMemoryFunc) dkisReleaseIteratorMem,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc)   NULL,

    (smTableCursorLockRowFunc) NULL,
    (smnDeleteFunc) NULL,
    (smnFreeFunc)   NULL,
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             NULL,

    (smInitFunc)   dkisInit,
    (smDestFunc)   dkisDest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc) NULL,
    (smnSetPositionFunc) NULL,

    (smnAllocIteratorFunc) dkisAllocIterator,
    (smnFreeIteratorFunc)  dkisFreeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,

    (smnMakeDiskImageFunc)  NULL,
    (smnBuildIndexFunc)	    NULL,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         NULL
};

#endif /* ALTIBASE_PRODUCT_XDB */

/*
 *
 */
IDE_RC dkisRegisterIndexModule( void )
{
#ifdef ALTIBASE_PRODUCT_XDB
    return smiUpdateIndexModule( NULL, &gRemoteTableIndexModule );
#else
    return smiUpdateIndexModule( &gRemoteTableIndexModule );
#endif
}
