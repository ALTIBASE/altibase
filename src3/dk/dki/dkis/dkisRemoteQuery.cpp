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

#include <smi.h>

#include <dkErrorCode.h>

#include <dkm.h>

#include <dkiSession.h>
#include <dkisIterator.h>

/*
 * if same, then true. if different, then false.
 */
static IDE_RC dkisCompareMeta( dkmSession * aSession,
                               SLong aStatementId,
                               smiRemoteTable * aSmiRemoteTable,
                               idBool * aIsSame )
{
    SInt sColumnCount = 0;
    SInt i = 0;
    idBool sIsSame = ID_FALSE;
    dkmColumnInfo * sColumnInfoArray = NULL;
    
    IDE_TEST( dkmGetColumnInfo( aSession,
                                aStatementId,
                                &sColumnCount,
                                &sColumnInfoArray )
              != IDE_SUCCESS );

    if ( sColumnCount != aSmiRemoteTable->mColumnCount )
    {
        sIsSame = ID_FALSE;
        goto return_function;
    }
    else
    {
        /* nothing to do */
    }
    
    for ( i = 0; i < sColumnCount; i++ )
    {
        if ( idlOS::strcmp( sColumnInfoArray[i].columnName,
                            aSmiRemoteTable->mColumns[i].mName )
             != 0 )
        {
            sIsSame = ID_FALSE;
            goto return_function;
        }
        else
        {
            /* nothing to do */
        }

        if ( aSmiRemoteTable->mColumns[i].mTypeId !=
             sColumnInfoArray[i].column.module->id )
        {
            sIsSame = ID_FALSE;
            goto return_function;
        }
        else
        {
            /* nothing to do */
        }

        if ( aSmiRemoteTable->mColumns[i].mPrecision !=
             sColumnInfoArray[i].column.precision )
        {
            sIsSame = ID_FALSE;
            goto return_function;
        }
        else
        {
            /* nothing  to do */
        }

        if ( aSmiRemoteTable->mColumns[i].mScale !=
             sColumnInfoArray[i].column.scale )
        {
            sIsSame = ID_FALSE;
            goto return_function;
        }
        else
        {
            /* nothing  to do */
        }
    }

    sIsSame = ID_TRUE;

return_function:
    
    *aIsSame = sIsSame;

    (void)dkmFreeColumnInfo( sColumnInfoArray );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
#ifdef ALTIBASE_PRODUCT_XDB
static IDE_RC dkisOpenRemoteQuery( smiTableCursor * aCursor,
                                   const void * /* aTable */,
                                   smSCN /* aSCN */,
                                   const smiRange * /* aKeyRange */,
                                   const smiRange * /* aKeyFilter */,
                                   const smiCallBack * aFilter,
                                   smlStLockNode * /* aCurLockNodePtr */,
                                   smlStLockSlot * /* aCurLockSlotPtr */ )
#else
static IDE_RC dkisOpenRemoteQuery( smiTableCursor * aCursor,
                                   const void * /* aTable */,
                                   smSCN /* aSCN */,
                                   const smiRange * /* aKeyRange */,
                                   const smiRange * /* aKeyFilter */,
                                   const smiCallBack * aFilter,
                                   smlLockNode * /* aCurLockNodePtr */,
                                   smlLockSlot * /* aCurLockSlotPtr */ )
#endif
{
    dkmSession * sSession = NULL;
    SLong sStatementId;
    dkisIterator * sIterator = NULL;
    idBool sIsSame = ID_FALSE;
    SInt sStage = 0;

    IDE_DASSERT( aCursor != NULL );
    /* BUG-43787 */
    /* dblink는 parallel 힌트를 지원하지 않음. */
    IDE_TEST_RAISE( aCursor->mCursorProp.mParallelReadProperties.mThreadCnt > 1,
                    ERR_NOT_APPLY_PARALLEL );

    sSession = dkiSessionGetDkmSession(
        (dkiSession *)aCursor->mCursorProp.mRemoteTableParam.mDkiSession );
    IDE_TEST( dkmCheckSessionAndStatus( sSession ) != IDE_SUCCESS );

    IDE_TEST( dkmAllocAndExecuteQueryStatement(
                  aCursor->mCursorProp.mRemoteTableParam.mQcStatement,
                  sSession,
                  aCursor->mCursorProp.mRemoteTable->mIsStore,
                  aCursor->mCursorProp.mRemoteTable->mLinkName,
                  aCursor->mCursorProp.mRemoteTable->mRemoteQuery,
                  &sStatementId )
              != IDE_SUCCESS );
    sStage = 1;
    
    IDE_TEST( dkisCompareMeta( sSession,
                               sStatementId,
                               aCursor->mCursorProp.mRemoteTable,
                               &sIsSame )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsSame != ID_TRUE, REBUILD_ERROR );

    sIterator = (dkisIterator *)aCursor->mIterator;

    sIterator->mFilter = aFilter;
    sIterator->mSession = sSession;
    sIterator->mStatementId = sStatementId;

    IDE_TEST( ((smnIndexModule *)aCursor->mIndexModule)->mInitIterator( (void *)aCursor->mIterator,
                                                                        aCursor->mStatement->getTrans(),
                                                                        (void *)aCursor->mTable,
                                                                        NULL,
                                                                        NULL,
                                                                        NULL,
                                                                        NULL,
                                                                        NULL,
                                                                        0,
                                                                        aCursor->mSCN,
                                                                        aCursor->mInfinite,
                                                                        ID_TRUE,
                                                                        &aCursor->mCursorProp,
                                                                        &aCursor->mSeekFunc,
                                                                        aCursor->mStatement ) 
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( REBUILD_ERROR )
    {
#if 0        
        IDE_SET( ideSetErrorCode( dkERR_REBUILD_META_IS_DIFFERENT ) );
#else
        IDE_SET( ideSetErrorCode( dkERR_ABORT_META_IS_DIFFERENT ) );
#endif
    }
    IDE_EXCEPTION( ERR_NOT_APPLY_PARALLEL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dkisOpenRemoteQuery] Remote table does not support the PARALLEL hint" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sStage )
    {
        case 1:
            (void)dkmFreeQueryStatement( sSession, sStatementId );
        default:
            break;
    }

    IDE_POP();

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkisCloseRemoteQuery( smiTableCursor * aCursor )
{
    dkisIterator * sIterator = NULL;
    
    sIterator = (dkisIterator *)aCursor->mIterator;

    IDE_TEST( dkmFreeQueryStatement( sIterator->mSession,
                                     sIterator->mStatementId )
              != IDE_SUCCESS );
    
    sIterator->mStatementId = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkisInsertRow( smiTableCursor  * /* aCursor */,
                             const smiValue  * /* aValueList */,
                             void           ** /* aRow */,
                             scGRID          * /* aGRID */ )
{
    IDE_SET( ideSetErrorCode( dkERR_ABORT_FUNCTION_NOT_AVAILABLE ) );

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
#ifdef ALTIBASE_PRODUCT_XDB
static IDE_RC dkisUpdateRow( smiTableCursor       * /* aCursor */,
                             const smiValue       * /* aValueList */ )
#else
static IDE_RC dkisUpdateRow( smiTableCursor       * /* aCursor */,
                             const smiValue       * /* aValueList */,
                             const smiDMLRetryInfo* /* aRetryInfo*/ )
#endif
{
    IDE_SET( ideSetErrorCode( dkERR_ABORT_FUNCTION_NOT_AVAILABLE ) );

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
#ifdef ALTIBASE_PRODUCT_XDB
static IDE_RC dkisDeleteRow( smiTableCursor       * /* aCursor */ )
#else
static IDE_RC dkisDeleteRow( smiTableCursor       * /* aCursor */,
                             const smiDMLRetryInfo* /* aRetryInfo*/ )
#endif
{
    IDE_SET( ideSetErrorCode( dkERR_ABORT_FUNCTION_NOT_AVAILABLE ) );

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */  
static IDE_RC dkisBeforeFirstModified( smiTableCursor * /* aCursor */,
                                       UInt             /* aFlag */ )
{
    IDE_SET( ideSetErrorCode( dkERR_ABORT_FUNCTION_NOT_AVAILABLE ) );

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */  
static IDE_RC dkisReadOldRow( smiTableCursor * /* aCursor */,
                              const void    ** /* aRow */,
                              scGRID         * /* aRowGRID */ )
{
    IDE_SET( ideSetErrorCode( dkERR_ABORT_FUNCTION_NOT_AVAILABLE ) );

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */  
static IDE_RC dkisReadNewRow( smiTableCursor * /* aCursor */,
                              const void    ** /* aRow */,
                              scGRID         * /* aRowGRID */ )
{
    IDE_SET( ideSetErrorCode( dkERR_ABORT_FUNCTION_NOT_AVAILABLE ) );

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

static smiTableSpecificFuncs gTableSpecificFunctions =
{
    dkisOpenRemoteQuery,
    dkisCloseRemoteQuery,

    dkisInsertRow,
    dkisUpdateRow,
    dkisDeleteRow,

    dkisBeforeFirstModified,
    
    dkisReadOldRow,
    dkisReadNewRow
};

/*
 *
 */
static IDE_RC dkisGetNullRowFunc( smiTableCursor  * aCursor,
                                  void           ** aRow,
                                  scGRID          * aRid )
{
    dkisIterator * sIterator = NULL;

    sIterator = (dkisIterator *)aCursor->mIterator;

    IDE_TEST( dkmGetNullRow( sIterator->mSession,
                             sIterator->mStatementId,
                             aRow,
                             aRid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkisRegisterRemoteQueryCallback( void )
{
    return smiTableCursor::setRemoteQueryCallback( &gTableSpecificFunctions,
                                                   dkisGetNullRowFunc );
}
