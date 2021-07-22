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
 * $Id: smnfModule.cpp 89495 2020-12-14 05:19:22Z emlee $
 **********************************************************************/

#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <smc.h>
#include <sm2x.h>
#include <smm.h>
#include <smnReq.h>

#include <smErrorCode.h>
#include <smnModules.h>
#include <smnfDef.h>
#include <smnfModule.h>
#include <smnManager.h>
#include <smiFixedTable.h>


static IDE_RC smnfInit( smnfIterator        * aIterator,
                        smxTrans            * aTrans,
                        smcTableHeader      * aTable,
                        smnIndexHeader      * /* aIndex */,
                        void                * aDumpObject,
                        const smiRange      * /* aRange */,
                        const smiRange      * /* aKeyFilter */,
                        const smiCallBack   * aFilter,
                        UInt                  /* aFlag */,
                        smSCN                 aSCN,
                        smSCN                 aInfinite,
                        idBool                /* aUntouchable */,
                        smiCursorProperties * aProperties,
                        const smSeekFunc   ** aSeekFunc, 
                        smiStatement        * aStatement );

static IDE_RC smnfDest( smnfIterator* aIterator );

static IDE_RC        smnfNA           ( void );
static IDE_RC        smnfAA           ( smnfIterator* aIterator,
                                        const void**  aRow );
static IDE_RC smnfBeforeFirst  ( smnfIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );
static IDE_RC smnfFetchNext    ( smnfIterator*       aIterator,
                                 const void**        aRow      );
static IDE_RC smnfPrepareIteratorMem( const smnIndexModule* );
static IDE_RC smnfReleaseIteratorMem(const smnIndexModule* );
static IDE_RC smnfFreeIterator( void * aIteratorMem);
static IDE_RC smnfAllocIterator( void ** aIteratorMem );
IDE_RC smnfCheckFilter( void   * aIterator,
                        void   * aRecord,
                        idBool * aResult );
void smnfCheckLimit( void   * aIterator,
                     idBool * aFirstLimitResult,
                     idBool * aLastLimitResult );
void smnfCheckLimitAndMovePos( void   * aIterator,
                               idBool * aLimitResult,
                               idBool   aIsMovePos );
IDE_RC smnfCheckLastLimit( void   * aIterator,
                           idBool * aIsLastLimitResult );

smnIndexModule smnfModule = {
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_FIXED,
                              SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( smnfIterator ),
    (smnMemoryFunc) smnfPrepareIteratorMem,
    (smnMemoryFunc) smnfReleaseIteratorMem,
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

    (smInitFunc)   smnfInit,
    (smDestFunc)   smnfDest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc)  NULL,
    (smnSetPositionFunc)  NULL,

    (smnAllocIteratorFunc) smnfAllocIterator,
    (smnFreeIteratorFunc)  smnfFreeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,

    (smnMakeDiskImageFunc)  NULL,

    (smnBuildIndexFunc)     NULL,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         NULL
};

static const smSeekFunc smnfSeekFunctions[12] =
{ /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)smnfFetchNext,
        (smSeekFunc)smnfNA,
        (smSeekFunc)smnfBeforeFirst,
        (smSeekFunc)smnfNA,
        (smSeekFunc)smnfAA,
        (smSeekFunc)smnfAA,
        (smSeekFunc)smnfNA,
        (smSeekFunc)smnfNA,
        (smSeekFunc)smnfNA,
        (smSeekFunc)smnfNA,
        (smSeekFunc)smnfAA,
        (smSeekFunc)smnfAA
};

static IDE_RC smnfPrepareIteratorMem( const smnIndexModule* )
{
    return IDE_SUCCESS;
}

static IDE_RC smnfReleaseIteratorMem(const smnIndexModule* )
{
    return IDE_SUCCESS;
}

void * smnfGetRecordPtr(void *aCurRec)
{
    IDE_ASSERT(aCurRec != NULL);

    return ( ((UChar *)aCurRec) + idlOS::align8(ID_SIZEOF(void *)));
}


IDE_RC smnfCheckFilter( void   * aIterator,
                        void   * aRecord,
                        idBool * aResult )
{
    smnfIterator * sIterator;

    sIterator = (smnfIterator*)aIterator;

    IDE_TEST( sIterator->filter->callback( aResult,
                                           smnfGetRecordPtr(aRecord),
                                           NULL,
                                           0,
                                           SC_NULL_GRID,
                                           sIterator->filter->data )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void smnfCheckLimit( void   * aIterator,
                     idBool * aFirstLimitResult,
                     idBool * aLastLimitResult )
{
    smnfIterator * sIterator;

    sIterator = (smnfIterator*)aIterator;

    *aFirstLimitResult = ID_FALSE;
    *aLastLimitResult = ID_FALSE;

    if( sIterator->mProperties->mFirstReadRecordPos <=
        sIterator->mReadRecordPos )
    {
        *aFirstLimitResult = ID_TRUE;
    }

    if( (sIterator->mProperties->mFirstReadRecordPos +
         sIterator->mProperties->mReadRecordCount) >
         sIterator->mReadRecordPos )
    {
        *aLastLimitResult = ID_TRUE;
    }
}

void smnfCheckLimitAndMovePos( void   * aIterator,
                               idBool * aLimitResult,
                               idBool   aIsMovePos )
{
    smnfIterator * sIterator;
    idBool         sFirstLimitResult;
    idBool         sLastLimitResult;

    sIterator = (smnfIterator*)aIterator;
    *aLimitResult = ID_FALSE;

    smnfCheckLimit( aIterator,
                    &sFirstLimitResult,
                    &sLastLimitResult );

    if( sFirstLimitResult == ID_FALSE ) // 읽어야 될 위치보다 앞에 있는 경우.
    {
        /*
         *         first           last
         * ----------*---------------*---------->
         *      ^
         */
    }
    else
    {
        if( sLastLimitResult == ID_TRUE ) // 읽어야 될 범위 안에 있는 경우.
        {
            /*
             *         first           last
             * ----------*---------------*---------->
             *                 ^           
             */
            *aLimitResult = ID_TRUE;
        }
        else // 읽어야 될 위치보다 뒤에 있는 경우.
        {
            /*
             *         first           last
             * ----------*---------------*---------->
             *                                 ^
             */
        }
    }

    if( aIsMovePos == ID_TRUE )
    {
        sIterator->mReadRecordPos++;
    }
}

// BUG-26201
IDE_RC smnfCheckLastLimit( void   * aIterator,
                           idBool * aIsLastLimitResult )
{
    smnfIterator * sIterator;

    sIterator = (smnfIterator*)aIterator;

    // Session Event를 검사한다.
    IDE_TEST(iduCheckSessionEvent(sIterator->mProperties->mStatistics) != IDE_SUCCESS);

    if( (sIterator->mProperties->mFirstReadRecordPos +
         sIterator->mProperties->mReadRecordCount) >
         sIterator->mReadRecordPos )
    {
        *aIsLastLimitResult = ID_FALSE;
    }
    else
    {
        // 읽어야 될 위치보다 뒤에 있는 경우.
        /*
         *         first           last
         * ----------*---------------*---------->
         *                                 ^
         */
        *aIsLastLimitResult = ID_TRUE;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smnfInit( smnfIterator        * aIterator,
                        smxTrans            * aTrans,
                        smcTableHeader      * aTable,
                        smnIndexHeader      * ,
                        void                * aDumpObject,
                        const smiRange      * ,
                        const smiRange      * ,
                        const smiCallBack   * aFilter,
                        UInt                  ,
                        smSCN                 aSCN,
                        smSCN                 aInfinite,
                        idBool                ,
                        smiCursorProperties * aProperties,
                        const smSeekFunc   ** aSeekFunc,
                        smiStatement        * aStatement )

{

    iduFixedTableBuildFunc sBuildFunc;
    smiFixedTableRecord   *sEndRecord;

    idvSQL                *sSQLStat;
    SInt                   sState = 0;

    // BUG-39503
    // Fixed Table이 참조되는 쿼리는, 결과 정합성을 지키지 않아도 된다.
    // Open된 상태에서 결과가 바뀌었더라도 Cursor를 Restart 할 수 있어야 한다.

    // PROJ-2083 DUAL Table
    if( (aTable->mFlag & SMI_TABLE_DUAL_MASK) == SMI_TABLE_DUAL_TRUE &&
        (aIterator->mRecBuffer != NULL) )
    {
        // Restart
        // DUAL table은 Restart시 record를 새로 구성하지 않고
        // 기존에 읽은 레코드를 다시 탐색하도록 한다.
        aIterator->mIsMemoryInit   = ID_FALSE;    
        aIterator->mTraversePtr    = aIterator->mRecBuffer;
    }
    else
    {        
        sBuildFunc = smiFixedTable::getBuildFunc(aTable);

        aIterator->SCN             = aSCN;
        aIterator->infinite        = aInfinite;
        aIterator->trans           = aTrans;
        aIterator->table           = aTable;
        aIterator->lstFetchRecPtr  = NULL;
        SC_MAKE_NULL_GRID( aIterator->mRowGRID );
        aIterator->tid             = smLayerCallback::getTransID( aTrans );
        aIterator->filter          = aFilter;
        aIterator->mProperties     = aProperties;
        aIterator->mTraversePtr    = NULL;
        aIterator->mReadRecordPos  = 0;
        aIterator->mStatement      = aStatement;

        // To Test BUG-13919
        // Memory Alloc 과정 중 실패를 가정함.
        IDU_FIT_POINT( "1.BUG-13919@smnfModule::smnfInit" );

        // BUG-41560
        if ( aIterator->mIsMemoryInit == ID_FALSE )
        {
            IDE_TEST( aIterator->mMemory.initialize( NULL ) != IDE_SUCCESS );

            /* BUG-41305 메모리 중복 초기화 방지 */
            aIterator->mIsMemoryInit = ID_TRUE;
        }
        else
        {
            IDE_TEST( aIterator->mMemory.restartInit() != IDE_SUCCESS );
        }

        sState = 1;

        aIterator->mMemory.setContext( (void*)aIterator );
        
        *aSeekFunc = smnfSeekFunctions;

        // PROJ-1618
        IDE_TEST( sBuildFunc( NULL, /* aStatistics */
                              aTable,
                              aDumpObject,
                              &(aIterator->mMemory) ) != IDE_SUCCESS );

        aIterator->mRecBuffer = aIterator->mMemory.getBeginRecord();

        // Check End of Record
        sEndRecord = (smiFixedTableRecord *)aIterator->mMemory.getCurrRecord();

        if (sEndRecord != NULL)
        {
            sEndRecord->mNext = NULL;
        }


        aIterator->mTraversePtr  = aIterator->mRecBuffer;
    }

    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD(sSQLStat, mMemCursorSeqScan, 1);

    if (sSQLStat != NULL)
    {
        IDV_SESS_ADD(sSQLStat->mSess, IDV_STAT_INDEX_MEM_CURSOR_SEQ_SCAN, 1);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        aIterator->mIsMemoryInit = ID_FALSE;
        IDE_ASSERT( aIterator->mMemory.destroy() == IDE_SUCCESS );  
    }

    return IDE_FAILURE;

}

static IDE_RC smnfDest( smnfIterator* aIterator )
{
    /* BUG-41305, BUG-41377 dual을 사용하지 않는 fixed table의 경우 재사용을 대비하여
     * 할당된 메모리를 반환해야 한다. */
    if( ( ( aIterator->table->mFlag & SMI_TABLE_DUAL_MASK ) == SMI_TABLE_DUAL_FALSE )
        && ( aIterator->mIsMemoryInit == ID_TRUE ) ) 
    {
        aIterator->mIsMemoryInit = ID_FALSE;
        IDE_TEST(aIterator->mMemory.destroy() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static IDE_RC smnfAA( smnfIterator*,
               const void** )
{

    return IDE_SUCCESS;

}

static IDE_RC smnfNA( void )
{

    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;

}

static IDE_RC smnfBeforeFirst( smnfIterator* aIterator,
                               const smSeekFunc** )
{

    aIterator->mTraversePtr    = aIterator->mRecBuffer;

    return IDE_SUCCESS;

}

static UChar * smnfGetNextSlot(UChar *aSlot)
{
    smiFixedTableRecord *aCurRec = (smiFixedTableRecord *)aSlot;

    if (aCurRec != NULL)
    {
        return (UChar *)aCurRec->mNext;
    }
    else
    {
        return NULL;
    }
}

static IDE_RC smnfFetchNext( smnfIterator* aIterator,
                             const void**  aRow )
{

    IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

    if ( aIterator->mTraversePtr == NULL )
    {
        aIterator->slot = NULL;
        *aRow           = NULL;
    }
    else
    {
        *aRow = smnfGetRecordPtr(aIterator->mTraversePtr);
        aIterator->mTraversePtr = smnfGetNextSlot(aIterator->mTraversePtr);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

static IDE_RC smnfAllocIterator( void ** aIteratorMem )
{

    smnfIterator*       sIterator;

    /* ------------------------------------------------
     *  BUG-13379 :
     * use mRecBuffer as a initialization-Hint
     * ----------------------------------------------*/

    sIterator             = (smnfIterator*)*(aIteratorMem);
    sIterator->mRecBuffer = NULL;
    sIterator->mIsMemoryInit = ID_FALSE;

    return IDE_SUCCESS;
}


static IDE_RC smnfFreeIterator( void * aIteratorMem)
{

    smnfIterator*       sIterator = (smnfIterator *)aIteratorMem;

    // PROJ-2083 DUAL Table
    if( ( ( sIterator->table->mFlag & SMI_TABLE_DUAL_MASK ) == SMI_TABLE_DUAL_FALSE )
        && ( sIterator->mIsMemoryInit == ID_TRUE ) ) 
    {
        IDE_TEST(sIterator->mMemory.destroy() != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
