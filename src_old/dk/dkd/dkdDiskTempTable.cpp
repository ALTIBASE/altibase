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
#include <idl.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <mtc.h>
#ifndef ALTIBASE_PRODUCT_XDB
#include <smiSortTempTable.h>
#endif

#include <dkdDiskTempTable.h>

struct dkdDiskTempTable
{
    void * mTable;
    
    UInt mColumnCount;

    mtcColumn * mColumnArray;
    smiColumnList * mSmiColumnList;
    
    smiValue * mSmiValueRow;
#ifndef ALTIBASE_PRODUCT_XDB    
    smiSortTempCursor * mCursor;
#endif
};

#ifdef ALTIBASE_PRODUCT_XDB

/* nothing to do */

#else
/*
 *
 */
static IDE_RC dkdTempTableAllocHandle( UInt aColumnCount,
                                       dkdDiskTempTableHandle ** aHandle )
{
    dkdDiskTempTableHandle * sHandle = NULL;
    SInt sStage = 0;

    IDU_FIT_POINT_RAISE( "dkdTempTableAllocHandle::malloc::Handle", 
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( *sHandle ),
                                       (void **)&sHandle,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sStage = 1;

    IDU_FIT_POINT_RAISE( "dkdTempTableAllocHandle::malloc::SmiValueRow",
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( smiValue ) * aColumnCount,
                                       (void **) &sHandle->mSmiValueRow )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sStage = 2;

    IDU_FIT_POINT_RAISE( "dkdTempTableAllocHandle::malloc::SmiColumnList",
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc(
                        IDU_MEM_DK,
                        ID_SIZEOF( *(sHandle->mSmiColumnList) ) * aColumnCount,
                        (void **)&(sHandle->mSmiColumnList),
                        IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sStage = 3;

    IDU_FIT_POINT_RAISE( "dkdTempTableAllocHandle::malloc::ColumnArray",
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc(
                        IDU_MEM_DK,
                        ID_SIZEOF( *(sHandle->mColumnArray) ) * aColumnCount,
                        (void **)&(sHandle->mColumnArray),
                        IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    *aHandle = sHandle;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            (void)iduMemMgr::free( sHandle->mSmiColumnList );
        case 2:
            (void)iduMemMgr::free( sHandle->mSmiValueRow );
        case 1:
            (void)iduMemMgr::free( sHandle );
        default:
            break;
    }
    IDE_POP();
    
    return IDE_FAILURE;
}
#endif /* ALTIBASE_PRODUCT_XDB */

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
/*
 *
 */
static void dkdTempTableFreeHandle( dkdDiskTempTableHandle * aHandle )
{
    (void)iduMemMgr::free( aHandle->mColumnArray );
    (void)iduMemMgr::free( aHandle->mSmiValueRow );
    (void)iduMemMgr::free( aHandle->mSmiColumnList );
    (void)iduMemMgr::free( aHandle );
}

/*
 * smiColumnList is used to create smiTempTable.
 */ 
static void initializeSmiColumnList( mtcColumn * aColumnArray,
                                     UInt aColumnCount,
                                     smiColumnList * aSmiColumnList )
{
    UInt i;
    
    for ( i = 0; i < aColumnCount; i++ )
    {
        /* Link next node */
        if ( i < aColumnCount - 1 )
        {
            aSmiColumnList[i].next = &aSmiColumnList[i + 1];
        }
        else
        {
            aSmiColumnList[i].next = NULL;
        }

        aSmiColumnList[i].column = &aColumnArray[i].column;
    }
}

/*
 *
 */
static void copyColumnArray( mtcColumn * aSource,
                             UInt aColumnCount,
                             mtcColumn * aDestination )
{
    UInt i;

    for ( i = 0; i < aColumnCount; i++ )
    {
        aDestination[i] = aSource[i];
    }
}
#endif /* ALTIBASE_PRODUCT_XDB */

/*
 *
 */
IDE_RC dkdDiskTempTableCreate( void * aQcStatement,
                               mtcColumn * aColumnArray,
                               UInt aColumnCount, 
                               dkdDiskTempTableHandle ** aHandle )
{
#ifdef ALTIBASE_PRODUCT_XDB
    DK_UNUSED( aQcStatement );
    DK_UNUSED( aColumnArray );
    DK_UNUSED( aColumnCount );
    DK_UNUSED( aHandle );
    
    return IDE_FAILURE;
#else
    dkdDiskTempTableHandle * sHandle = NULL;
    SInt sStage = 0;
    scSpaceID sSpaceId;
    smiStatement * sStatement = NULL;

    IDE_TEST( dkdTempTableAllocHandle( aColumnCount, &sHandle )
              != IDE_SUCCESS );
    sStage = 1;

    initializeSmiColumnList( aColumnArray,
                             aColumnCount,
                             sHandle->mSmiColumnList );

    copyColumnArray( aColumnArray, aColumnCount, sHandle->mColumnArray );
    
    sHandle->mCursor = NULL;
    sHandle->mColumnCount = aColumnCount;

    IDE_TEST( qciMisc::getTempSpaceId( aQcStatement, &sSpaceId )
              != IDE_SUCCESS );
    
    IDE_TEST( qciMisc::getSmiStatement( aQcStatement, &sStatement )
              != IDE_SUCCESS );
    
    IDE_TEST( smiSortTempTable::create( 
                  NULL,
                  sSpaceId,
                  0, /* aWorkAreaSize */
                  sStatement,
                  SMI_TTFLAG_RANGESCAN,            /* SORT Temp Table  */
                  sHandle->mSmiColumnList,         // Table의 Column 구성
                  sHandle->mSmiColumnList,         // key column list
                  0,                               // WorkGroupRatio
                (const void **) &sHandle->mTable ) // Table 핸들
        != IDE_SUCCESS );
    
    *aHandle = sHandle;
            
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sStage )
    {
        case 1:
            dkdTempTableFreeHandle( sHandle );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
#endif
}

/*
 *
 */
IDE_RC dkdDiskTempTableDrop( dkdDiskTempTableHandle * aHandle )
{
#ifdef ALTIBASE_PRODUCT_XDB
    DK_UNUSED( aHandle );
    
    return IDE_FAILURE;
#else
    IDE_TEST( smiSortTempTable::drop( aHandle->mTable )
              != IDE_SUCCESS );

    dkdTempTableFreeHandle( aHandle );
            
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/*
 *
 */
IDE_RC dkdDiskTempTableInsertRow( dkdDiskTempTableHandle * aHandle,
                                  void * aRow )
{
#ifdef ALTIBASE_PRODUCT_XDB
    DK_UNUSED( aHandle );
    DK_UNUSED( aRow );
    
    return IDE_FAILURE;
#else
    UInt i;
    
    for ( i = 0; i < aHandle->mColumnCount; i++ )
    {
        aHandle->mSmiValueRow[i].value = 
            (SChar *)aRow + aHandle->mColumnArray[i].column.offset;

        aHandle->mSmiValueRow[i].length = 
            aHandle->mColumnArray[i].module->actualSize(
                NULL,
                aHandle->mSmiValueRow[i].value );
    }
    
    IDE_TEST( smiSortTempTable::insert( aHandle->mTable,
                                        aHandle->mSmiValueRow )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/*
 *
 */ 
IDE_RC dkdDiskTempTableOpenCursor( dkdDiskTempTableHandle * aHandle )
{
#ifdef ALTIBASE_PRODUCT_XDB
    DK_UNUSED( aHandle );
    
    return IDE_FAILURE;
#else
    UInt sFlag;

    sFlag = SMI_TCFLAG_FORWARD | SMI_TCFLAG_ORDEREDSCAN | SMI_TCFLAG_IGNOREHIT;

    IDE_TEST( smiSortTempTable::openCursor( 
                  aHandle->mTable,
                  sFlag,
                  NULL,  // Update Column정보
                  smiGetDefaultKeyRange(),
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  &(aHandle->mCursor) )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/*
 *
 */ 
IDE_RC dkdDiskTempTableRestartCursor( dkdDiskTempTableHandle * aHandle )
{
#ifdef ALTIBASE_PRODUCT_XDB
    DK_UNUSED( aHandle );
    
    return IDE_FAILURE;
#else
    UInt sFlag;

    sFlag = SMI_TCFLAG_FORWARD | SMI_TCFLAG_ORDEREDSCAN | SMI_TCFLAG_IGNOREHIT;
    
    IDE_TEST( smiSortTempTable::restartCursor( 
                  aHandle->mCursor,
                  sFlag,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter() )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/*
 *
 */ 
IDE_RC dkdDiskTempTableFetchRow( dkdDiskTempTableHandle * aHandle,
                                 void ** aRow )
{
#ifdef ALTIBASE_PRODUCT_XDB
    DK_UNUSED( aHandle );
    DK_UNUSED( aRow );
    
    return IDE_FAILURE;
#else
    scGRID sDummyGRID;
    
    IDE_TEST( smiSortTempTable::fetch( aHandle->mCursor,
                                       (UChar **)aRow,
                                       &sDummyGRID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}
