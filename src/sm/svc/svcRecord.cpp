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
 
#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smErrorCode.h>
#include <svm.h>
#include <smp.h>
#include <svp.h>
#include <svcDef.h>
#include <svcReq.h>
#include <svcRecord.h>
#include <svcRecordUndo.h>
#include <svcLob.h>
#include <sgmManager.h>

/***********************************************************************
 * Description : aHeader가 가리키는 Table에 대해서 Insert를 수행한다.
 *
 * aStatistics     - [IN] None
 * aTrans          - [IN] Transaction Pointer
 * aTableInfoPtr   - [IN] Table Info Pointer(Table에 Transaction이 Insert,
 *                        Delete한 Record Count에 대한 정보를 가지고 있다.
 * aHeader         - [IN] Table Header Pointer
 * aInfinite       - [IN] Insert시 새로운 Record를 할당해야 되는데 그 때
 *                        Record Header의 SCN에 Setting될 값
 * aRow            - [IN] Insert하기 위해 새롭게 할당된 Record의 Pointer
 *
 * aRetInsertSlotGRID - [IN] None
 * aValueList      - [IN] Value List
 * aAddOIDFlag     - [IN] 1.SM_INSERT_ADD_OID_OK : OID List에 Insert한 작업기록.
 *                        2.SM_INSERT_ADD_OID_NO : OID List에 Insert한 작업을 기록하지 않는다.
                          1.SM_UNDO_LOGGING_OK : refineDB, createTable 
                          2.SM_UNDO_LOGGING_NO : undo시 Compensation Log를 기록하지 않는다.
 *  log structure:
 *   <update_log, NewOid, FixedRow_size, Fixed_Row, var_col1, var_col2...>
 ***********************************************************************/
IDE_RC svcRecord::insertVersion( idvSQL         * /*aStatistics*/,
                                 void           * aTrans,
                                 void           * aTableInfoPtr,
                                 smcTableHeader * aHeader,
                                 smSCN            aInfinite,
                                 SChar         ** aRow,
                                 scGRID         * aRetInsertSlotGRID,
                                 const smiValue * aValueList,
                                 UInt             aAddOIDFlag )
{
    const smiColumn   * sCurColumn;
    SChar             * sNewFixRowPtr = NULL;
    const smiValue    * sCurValue;
    UInt                i;
    smOID               sFixOid;
    scPageID            sPageID             = SM_NULL_PID;
    UInt                sColumnCount        = 0;
    UInt                sUnitedVarColumnCount     = 0;
    idBool              sIsAddOID = ID_FALSE;
    smcLobDesc        * sCurLobDesc;
    smpPersPageHeader * sPagePtr;
    smiValue            sUnitedVarValues[SMI_COLUMN_ID_MAXIMUM];
    const smiColumn   * sUnitedVarColumns[SMI_COLUMN_ID_MAXIMUM];
    
    IDE_ERROR( aTrans     != NULL );
    IDE_ERROR( aHeader    != NULL );
    IDE_ERROR( aValueList != NULL );

    IDU_FIT_POINT( "1.PROJ-2118@svcRecord::insertVersion" );

    /* insert하기 위하여 새로운 Slot 할당 받음.*/
    IDE_TEST( svpFixedPageList::allocSlot( aTrans,
                                           aHeader->mSpaceID,
                                           aTableInfoPtr,
                                           aHeader->mSelfOID,
                                           &(aHeader->mFixed.mVRDB),
                                           &sNewFixRowPtr,
                                           aInfinite,
                                           aHeader->mMaxRow,
                                           SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT)
              != IDE_SUCCESS );

    sPageID = SMP_SLOT_GET_PID(sNewFixRowPtr);
    sFixOid = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sNewFixRowPtr ) );

    /* BUG-32091 [sm_collection] add TableOID in PageHeader
     * TableOID를 Get한다.*/
    IDE_ASSERT( smmManager::getPersPagePtr( aHeader->mSpaceID,
                                            sPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    IDE_ASSERT( sPagePtr->mTableOID == aHeader->mSelfOID );

    /* Insert한 Row에 대하여 version list 추가 */
    if ( SM_INSERT_ADD_OID_IS_OK(aAddOIDFlag) )
    {
        sIsAddOID = ID_TRUE;
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           sFixOid,
                                           aHeader->mSpaceID,
                                           SM_OID_NEW_INSERT_FIXED_SLOT )
                  != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    /* --------------------------------------------------------
     * [4] Row의 fixed 영역에 대한 로깅 및
     *     variable column에 대한 실제 데이타 삽입
     *     (로그레코드를 구성하는 작업이며
     *      실제 데이타레코드의 작업은 마지막 부분에서 수행함)
     * -------------------------------------------------------- */
    sColumnCount = aHeader->mColumnCount;
    sCurValue    = aValueList;

    for( i = 0; i < sColumnCount; i++ )
    {
        /* Variable column의 경우
            (1) Variabel page에서 slot 할당하여 실제 데이타 레코드  설정.
            (2) 레코드의 fixed 영역에 smVCDesc 정보 구성 */
        sCurColumn = smcTable::getColumn(aHeader,i);

        /* --------------------------------------------------------------------
         *  [4-1] Variable column의 경우
         *        (1) Variabel page에서 slot 할당하여 실제 데이타 레코드  설정.
         *        (2) 레코드의 fixed 영역에 smVarColumn 정보 구성
         * -------------------------------------------------------------------- */
        if( (sCurColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
        {
            IDE_ERROR_RAISE_MSG(
                ( (sCurValue->length <= sCurColumn->size) &&
                  ( ((sCurValue->length  > 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value == NULL)) ) ),
                err_invalid_column_size,
                "Table OID     : %"ID_vULONG_FMT"\n"
                "Column Seq    : %"ID_UINT32_FMT"\n"
                "Column Size   : %"ID_UINT32_FMT"\n"
                "Column Offset : %"ID_UINT32_FMT"\n"
                "Value Length  : %"ID_UINT32_FMT"\n",
                aHeader->mSelfOID,
                i,
                sCurColumn->size,
                sCurColumn->offset,
                sCurValue->length );
        }
        else
        {
            IDE_ERROR_RAISE_MSG(
                ( (sCurValue->length <= sCurColumn->size) &&
                  ( ((sCurValue->length  > 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value == NULL)) ) ),
                err_invalid_column_size,
                "Table OID     : %"ID_vULONG_FMT"\n"
                "Column Seq    : %"ID_UINT32_FMT"\n"
                "Column Size   : %"ID_UINT32_FMT"\n"
                "Column Offset : %"ID_UINT32_FMT"\n"
                "Value Length  : %"ID_UINT32_FMT"\n",
                aHeader->mSelfOID,
                i,
                sCurColumn->size,
                sCurColumn->offset,
                sCurValue->length );
        }

        IDU_FIT_POINT( "2.PROJ-2118@svcRecord::insertVersion" );

        switch( sCurColumn->flag & SMI_COLUMN_TYPE_MASK )
        {
            /* PROJ-2174 Supporting LOB in the volatile tablespace */
            case SMI_COLUMN_TYPE_LOB:

                sCurLobDesc = (smcLobDesc *)(sNewFixRowPtr + sCurColumn->offset);

                /* prepare에서 기존 LobDesc를 old version으로 인식하기 때문에 초기화 */
                SMC_LOB_DESC_INIT( sCurLobDesc );

                if( sCurValue->length > 0 )
                {
                    /* SQL로 직접 Lob 데이터를 넣을 경우 */
                    IDE_TEST( svcLob::reserveSpaceInternal(
                                                      aTrans,
                                                      (smcTableHeader *)aHeader,
                                                      (smiColumn *)sCurColumn,
                                                      sCurLobDesc->mLobVersion,
                                                      sCurLobDesc,
                                                      0,
                                                      sCurValue->length,
                                                      aAddOIDFlag,
                                                      ID_TRUE /* aIsFullWrite */)
                              != IDE_SUCCESS );

                    IDE_TEST( svcLob::writeInternal( (UChar *)sNewFixRowPtr,
                                                     (smiColumn *)sCurColumn,
                                                     0,
                                                     sCurValue->length,
                                                     (UChar *)sCurValue->value )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Here length is always 0. */
                    
                    if( sCurValue->value == NULL )
                    {
                        /* null */
                        sCurLobDesc->flag =
                            SM_VCDESC_MODE_IN | SM_VCDESC_NULL_LOB_TRUE;
                    }
                    else
                    {
                        /* empty (nothing to do) */
                    }
                }

                sCurLobDesc->length = sCurValue->length;
                
                break;

            case SMI_COLUMN_TYPE_VARIABLE:
                sUnitedVarColumns[sUnitedVarColumnCount]       = sCurColumn;
                sUnitedVarValues[sUnitedVarColumnCount].length = sCurValue->length;
                sUnitedVarValues[sUnitedVarColumnCount].value  = sCurValue->value;
                sUnitedVarColumnCount++;

                break;
            case SMI_COLUMN_TYPE_FIXED:

                IDU_FIT_POINT( "3.PROJ-2118@svcRecord::insertVersion" );

                // BUG-30104 Fixed Column의 length는 0이 될 수 없습니다.
                IDE_ERROR_MSG( sCurValue->length > 0,
                               "Table OID  : %"ID_vULONG_FMT"\n"
                               "Space ID   : %"ID_UINT32_FMT"\n"
                               "Column Seq : %"ID_UINT32_FMT"\n",
                               aHeader->mSelfOID,
                               aHeader->mSpaceID,
                               i );

                /* Fixed column의 경우
                   (1) 레코드의 fixed 영역에 실제 값 설정 */
                idlOS::memcpy(sNewFixRowPtr + sCurColumn->offset,
                              sCurValue->value,
                              sCurValue->length);

                break;

            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
            default:
                ideLog::log( IDE_ERR_0,
                             "Invalid column[%u] "
                             "flag : %u "
                             "( TableOID : %lu )\n",
                             i,
                             sCurColumn->flag,
                             aHeader->mSelfOID );

                IDE_ASSERT( 0 );
                break;
        }

        sCurValue++ ;
    }

    if ( sUnitedVarColumnCount > 0)
    {
        IDE_TEST( insertUnitedVarColumns ( aTrans,
                                           aHeader,
                                           sNewFixRowPtr,
                                           sUnitedVarColumns,
                                           sIsAddOID,
                                           sUnitedVarValues,
                                           sUnitedVarColumnCount )
                != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    /*
     * [BUG-24353] volatile table의 modify column 실패후 레코드가 사라집니다.
     * - Volatile Table은 Compensation Log를 기록하지 않는다.
     */
    if( SM_UNDO_LOGGING_IS_OK(aAddOIDFlag) )
    {
        /* Insert에 대한 Log를 기록한다. */
        IDE_TEST( svcRecordUndo::logInsert( smLayerCallback::getLogEnv( aTrans ),
                                            aTrans,
                                            aHeader->mSelfOID,
                                            aHeader->mSpaceID,
                                            sNewFixRowPtr )
                  != IDE_SUCCESS);
    }

    /* BUG-14513: Fixed Slot을 할당시 Alloc Slot Log를 기록하지
       않도록 수정. insert log가 Alloc Slot에 대한 Redo, Undo수행.
       이 때문에 insert log기록 후 Slot header에 대한 Update수행
    */
    svpFixedPageList::setAllocatedSlot( aInfinite, sNewFixRowPtr );

    if (aTableInfoPtr != NULL)
    {
        /* Record가 새로 추가되었으므로 Record Count를 증가시켜준다.*/
        smLayerCallback::incRecCntOfTableInfo( aTableInfoPtr );
    }

    if(aRow != NULL)
    {
        *aRow = sNewFixRowPtr;
    }
    if(aRetInsertSlotGRID != NULL)
    {
        SC_MAKE_GRID( *aRetInsertSlotGRID,
                      aHeader->mSpaceID,
                      sPageID,
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sNewFixRowPtr ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_column_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INVALID_COLUMN_SIZE) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BUG-39399, BUG-39507
 *
 * aTrans        - [IN] Transaction Pointer
 * aViewSCN      - [IN] 이 연산을 수행하는 Cursor의 ViewSCN
 * aHeader       - [IN] aRow가 속해 있는 Table Header Pointer
 * aRow          - [IN] Update할 Row.
 * aInfinite     - [IN] New Version의 Row SCN
 * aIsUpdatedBySameStmt - [OUT] 같은 statement에서 update 했는지 여부
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC svcRecord::isUpdatedVersionBySameStmt( void            * aTrans,
                                              smSCN             aViewSCN,
                                              smcTableHeader  * aHeader,
                                              SChar           * aRow,
                                              smSCN             aInfinite,
                                              idBool          * aIsUpdatedBySameStmt,
                                              idBool            aForbiddenToRetry )
{
    UInt        sState      = 0;
    scPageID    sPageID     = SMP_SLOT_GET_PID( aRow );

    /* 1. hold x latch */
    IDE_TEST( svmManager::holdPageXLatch( aHeader->mSpaceID, sPageID )
              != IDE_SUCCESS );
    sState = 1;

    /* 2. isUpdatedVersionBySameStmt - checkUpdatedVersionBySameStmt */
    IDE_TEST( checkUpdatedVersionBySameStmt( aTrans,
                                             aViewSCN,
                                             aHeader,
                                             aRow,
                                             aInfinite,
                                             aIsUpdatedBySameStmt,
                                             aForbiddenToRetry )
              != IDE_SUCCESS );

    /* 3. relase latch */
    sState = 0;
    IDE_TEST( svmManager::releasePageLatch(aHeader->mSpaceID, sPageID)
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( svmManager::releasePageLatch(aHeader->mSpaceID, sPageID)
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BUG-39399, BUG-39507
 *
 * aTrans        - [IN] Transaction Pointer
 * aViewSCN      - [IN] ViewSCN
 * aHeader       - [IN] aRow가 속해 있는 Table Header Pointer
 * aRow          - [IN] Delete나 Update대상 Row
 * aInfiniteSCN  - [IN] New Version의 Row SCN
 * aIsUpdatedBySameStmt - [OUT] 같은 statement에서 update 했는지 여부
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC svcRecord::checkUpdatedVersionBySameStmt( void             * aTrans,
                                                 smSCN              aViewSCN,
                                                 smcTableHeader   * aHeader,
                                                 SChar            * aRow,
                                                 smSCN              aInfiniteSCN,
                                                 idBool           * aIsUpdatedBySameStmt,
                                                 idBool             aForbiddenToRetry )
{
    smSCN   sNxtRowSCN;
    smSCN   sInfiniteSCNWithDeleteBit;
    smTID   sNxtRowTID      = SM_NULL_TID;

    IDE_ASSERT( aHeader != NULL );
    IDE_ASSERT( aTrans  != NULL );
    IDE_ASSERT( aRow    != NULL );

    SM_INIT_SCN( &sNxtRowSCN );
    SM_INIT_SCN( &sInfiniteSCNWithDeleteBit );

    *aIsUpdatedBySameStmt   = ID_FALSE; /* BUG-39507 */
    SM_SET_SCN( &sInfiniteSCNWithDeleteBit, &aInfiniteSCN );
    SM_SET_SCN_DELETE_BIT( &sInfiniteSCNWithDeleteBit );

    /* BUG-15642 Record가 두번 Free되는 경우가 발생함.:
     * 이런 경우가 발생하기전에 validateUpdateTargetRow에서
     * 체크하여 문제가 있다면 Rollback시키고 다시 retry를 수행시킴*/
    /* 이 함수 내에서는 row의 변하는 부분이 참조되지 않는다.
     * 그렇기 때문에 row의 변하는 부분을 따로 할당하여
     * 함수의인자로 줄 필요가 없다. */

   IDE_TEST_RAISE( validateUpdateTargetRow( aTrans,
                                            aHeader->mSpaceID,
                                            aViewSCN,
                                            (smpSlotHeader *)aRow,
                                            NULL /*aRetryInfo*/ )
                    != IDE_SUCCESS, already_modified );

    SMX_GET_SCN_AND_TID( ((smpSlotHeader *)aRow)->mLimitSCN,
                         sNxtRowSCN,
                         sNxtRowTID );

    /* Next Version이 Lock Row인 경우 */
    if( SM_SCN_IS_LOCK_ROW( sNxtRowSCN ) )
    {
        if ( sNxtRowTID == smLayerCallback::getTransID( aTrans ) )
        {
            /* aRow에 대한 Lock을 aTrans가 이미 잡았다.*/
            /* BUG-39399, BUG-39507
             * 1. aInfiniteSCN과 sNxtRowSCN의 InfiniteSCN 이 같다면,
             *    같은 statement에서 update 한 경우이다.
             * 2. sNxtRowSCN이 aInfiniteSCN + SM_SCN_INF_DELETE_BIT 이면
             *    같은 statement에서 delete 한 경우이다. */
            if( SM_SCN_IS_EQ( &sNxtRowSCN, &aInfiniteSCN ) ||
                SM_SCN_IS_EQ( &sNxtRowSCN, &sInfiniteSCNWithDeleteBit ) )
            {
                *aIsUpdatedBySameStmt = ID_TRUE;
            }
            else
            {
                *aIsUpdatedBySameStmt = ID_FALSE;
            }
        }
        else
        {
            /* 다른 트랜잭션이 lock을 잡았으므로 내 stmt에서 update 했을 수 없다. */
            *aIsUpdatedBySameStmt = ID_FALSE;
        }
    }
    else
    {
        /* BUG-39399, BUG-39507
         * 1. aInfiniteSCN과 sNxtRowSCN의 InfiniteSCN 이 같다면,
         *    같은 statement에서 update 한 경우이다.
         * 2. sNxtRowSCN이 aInfiniteSCN + SM_SCN_INF_DELETE_BIT 이면
         *    같은 statement에서 delete 한 경우이다. */
        if( SM_SCN_IS_EQ( &sNxtRowSCN, &aInfiniteSCN ) ||
            SM_SCN_IS_EQ( &sNxtRowSCN, &sInfiniteSCNWithDeleteBit ) )
        {
            *aIsUpdatedBySameStmt = ID_TRUE;
        }
        else
        {
            *aIsUpdatedBySameStmt = ID_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( already_modified );
    {
        if( aForbiddenToRetry == ID_TRUE )
        {
            IDE_DASSERT( ((smxTrans*)aTrans)->mIsGCTx == ID_TRUE ); 

            SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];
            idlOS::snprintf( sMsgBuf,
                             SMI_MAX_ERR_MSG_LEN,
                             "[UPDATE VALIDATION] "
                             "TableOID:%"ID_vULONG_FMT", "
                             "ViewSCN:%"ID_UINT64_FMT", "
                             "CreateSCN:%"ID_UINT64_FMT,
                             aHeader->mSelfOID,
                             SM_SCN_TO_LONG(aViewSCN),
                             SM_SCN_TO_LONG(((smpSlotHeader *)aRow)->mCreateSCN) );

            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) ); 

            IDE_ERRLOG( IDE_SD_19 );
        }
        else
        {
            IDE_SET( ideSetErrorCode(smERR_RETRY_Already_Modified) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aOldRow를 Update한다. Update시 MMDB의 MVCC에 의해서
 *               새로운 Version을 만들어 낸다. Update가 끝나게 되면
 *               aOldRow는 새로운 Version을 가리키게 된다.
 *
 * aStatistics   - [IN] None
 * aTrans        - [IN] Transaction Pointer
 * aViewSCN      - [IN] 이 연산을 수행하는 Cursor의 ViewSCN
 * aHeader       - [IN] aOldRow가 속해 있는 Table Header Pointer
 * aOldRow       - [IN] Update할 Row.
 * aOldGRID      - [IN] None
 * aRow          - [OUT] Update의해 만들어진 New Version의 Row Pointer를 넘겨준다.
 * aRetSlotGRID  - [IN] None
 * aColumnList   - [IN] Update될 Column List
 * aValueList    - [IN] Update될 Column들의 새로운 Vaule List
 * aRetryInfo    - [IN] Retry를 위해 점검할 column의 list
 * aInfinite     - [IN] New Version의 Row SCN
 * aModifyIdxBit - [IN] None
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC svcRecord::updateVersion( idvSQL               * /*aStatistics*/,
                                 void                 * aTrans,
                                 smSCN                  aViewSCN,
                                 void                 * /*aTableInfoPtr*/,
                                 smcTableHeader       * aHeader,
                                 SChar                * aOldRow,
                                 scGRID                 /*aOldGRID*/,
                                 SChar                **aRow,
                                 scGRID               * aRetSlotGRID,
                                 const smiColumnList  * aColumnList,
                                 const smiValue       * aValueList,
                                 const smiDMLRetryInfo* aRetryInfo,
                                 smSCN                  aInfinite,
                                 void                 * /*aLobInfo4Update*/,
                                 ULong                  /*aModifyIdxBit*/,
                                 idBool                 aForbiddenToRetry )
{
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;
    const smiValue        * sCurValue;
    SChar                 * sNewFixRowPtr = NULL;
    smOID                   sFixOID;
    smOID                   sTargetOID;
    UInt                    sState = 0;
    UInt                    sColumnSeq;
    scPageID                sPageID = SM_NULL_PID;
    vULong                  sFixedRowSize;
    smpSlotHeader         * sOldSlotHeader = (smpSlotHeader*)aOldRow;
    smpPersPageHeader     * sPagePtr;
    smOID                   sFstVCPieceOID;
    scPageID                sNewFixRowPID;
    smcLobDesc            * sCurLobDesc;
    idBool                  sIsHeadNextChanged = ID_FALSE;
    ULong                   sBeforeLimit;
    UInt                    sUnitedVarColumnCount = 0;
    const smiColumn       * sUnitedVarColumns[SMI_COLUMN_ID_MAXIMUM];
    smiValue                sUnitedVarValues[SMI_COLUMN_ID_MAXIMUM];

    SM_MAX_SCN( &sBeforeLimit );

    IDE_ERROR( aTrans  != NULL );
    IDE_ERROR( aOldRow != NULL );
    IDE_ERROR( aHeader != NULL );

    IDU_FIT_POINT( "1.PROJ-2118@svcRecord::updateVersion" );

    /* BUGBUG: By Newdaily
       aColumnList와 aValueList가 NULL인 경우가 존재함.
       이에 대한 검증이 필요합니다.
    */

    /* !!Fixd Row는 Update시 무조건 New Version이 만들어진다 */

    sOldSlotHeader = (smpSlotHeader*)aOldRow;
    sPageID     = SMP_SLOT_GET_PID(aOldRow);
    sTargetOID  = SM_MAKE_OID( sPageID,
                               SMP_SLOT_GET_OFFSET( sOldSlotHeader ) );

    /* update하기 위하여 old version이 속한 page에 대하여
       holdPageXLatch  */
    IDE_TEST( svmManager::holdPageXLatch( aHeader->mSpaceID, sPageID )
              != IDE_SUCCESS );
    sState = 1;

    /* aOldRow를 다른 Transaction이 이미 Update했는지 조사.*/
    IDE_TEST( recordLockValidation( aTrans,
                                    aViewSCN,
                                    aHeader->mSpaceID,
                                    &aOldRow,
                                    ID_ULONG_MAX,
                                    &sState,
                                    aRetryInfo,
                                    aForbiddenToRetry )
              != IDE_SUCCESS );

    // PROJ-1784 DML without retry
    // sOldFixRowPtr이 변경 될 수 있으므로 값을 다시 설정해야 함
    // ( 이경우 page latch 도 다시 잡았음 )
    sOldSlotHeader = (smpSlotHeader*)aOldRow;
    sPageID     = SMP_SLOT_GET_PID(aOldRow);
    sTargetOID  = SM_MAKE_OID( sPageID,
                               SMP_SLOT_GET_OFFSET( sOldSlotHeader ) );

    /* New Version을 할당한다. */
    IDE_TEST( svpFixedPageList::allocSlot(aTrans,
                                          aHeader->mSpaceID,
                                          NULL,
                                          aHeader->mSelfOID,
                                          &(aHeader->mFixed.mVRDB),
                                          &sNewFixRowPtr,
                                          aInfinite,
                                          aHeader->mMaxRow,
                                          SMP_ALLOC_FIXEDSLOT_NONE)
              != IDE_SUCCESS );

    sNewFixRowPID = SMP_SLOT_GET_PID(sNewFixRowPtr);
    sFixOID       = SM_MAKE_OID( sNewFixRowPID,
                                 SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sNewFixRowPtr ) );

    /* BUG-32091 [sm_collection] add TableOID in PageHeader
     * TableOID를 Get한다.*/
    IDE_ASSERT( smmManager::getPersPagePtr( aHeader->mSpaceID,
                                            sPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    IDE_ASSERT( sPagePtr->mTableOID == aHeader->mSelfOID );

    /* Tx OID List에 Old Version 추가. */
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       aHeader->mSelfOID,
                                       sTargetOID,
                                       aHeader->mSpaceID,
                                       SM_OID_OLD_UPDATE_FIXED_SLOT )
              != IDE_SUCCESS );

    /* PROJ-2174 Supporting LOB in the volatile tablespace
     * LOB CURSOR에 의해 호출된 경우 aColumnList와 aValueList가
     * 둘 다 NULL이다.
     * 따라서 두 값이 NULL이면 LOB CURSOR에 의한 호출로 판단하고,
     * 아닐경우 TABLE CURSOR에 의한 호출로 판단한다. */
    if ( (aColumnList == NULL) && (aValueList == NULL) )
    {
        /* LOB Cursor의 Update는 Cursor Close시에 Row가 Insert되지 않고
         * 업데이트후에 바로 업데이트한다. 때문에 Cursor Close시에 또
         * Insert가 되지않도록 SM_OID_ACT_CURSOR_INDEX을 Clear해서
         * OID List에 추가한다.
         *
         * BUG-33127 [SM] when lob update operation is rolled back, index keys
         *           does not be deleted.
         *
         * lob update 중 rollback 되는 경우 SM_OID_ACT_CURSOR_INDEX 플래그를
         * 제거하기 때문에 Cursor close시 인덱스에 키를 삽입하지 않습니다.
         * 그리고 rollback시 aging또한 SM_OID_ACT_CURSOR_INDEX플래그로 인해
         * 하지 않게 됩니다.
         * 하지만 lob인 경우 인덱스 키를 바로 삽입하기 때문에 rollback이 발생하면
         * 인덱스 키를 삭제해 주어야 합니다.
         * 따라서 SM_OID_ACT_AGING_INDEX 플래그를 추가하여 rollback시 인덱스에서
         * 키를 삭제하도록 수정합니다. */
        
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           sFixOID,
                                           aHeader->mSpaceID,
                                           ( SM_OID_NEW_UPDATE_FIXED_SLOT & ~SM_OID_ACT_CURSOR_INDEX )
                                             | SM_OID_ACT_AGING_INDEX )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           sFixOID,
                                           aHeader->mSpaceID,
                                           SM_OID_NEW_UPDATE_FIXED_SLOT )
                  != IDE_SUCCESS );
    }

    /* aOldRow의 Next Version을 New Version으로 Setting.*/
    /* sPageID에 대한 latch를 풀기 전에 반드시 바꿔야 한다. */
    SMP_SLOT_SET_NEXT_OID( sOldSlotHeader, sFixOID );
    /* aOldRow->mNext를 바꾸기 전에 값을 저장해놓는다. 이는 나중에
       로깅할 때 기록된다. */
    SM_SET_SCN( &sBeforeLimit, &(sOldSlotHeader->mLimitSCN ) );
    SM_SET_SCN( &(sOldSlotHeader->mLimitSCN), &aInfinite );
    sIsHeadNextChanged = ID_TRUE;

    sState = 0;
    IDE_TEST( svmManager::releasePageLatch(aHeader->mSpaceID, sPageID)
              != IDE_SUCCESS );

    /* 위 연산이 끝나게 되면 Row에 Lock잡히게 된다. 왜냐면 Next Version에
       끝나지 않은 Transaction이 만든 New Version이 있게 되면 다른 Transaction들은
       Wait하게 되기때문이다. 그래서 위에서 Page Latch 또한 풀었다.*/

    sFixedRowSize = aHeader->mFixed.mVRDB.mSlotSize - SMP_SLOT_HEADER_SIZE;

    idlOS::memcpy( sNewFixRowPtr + SMP_SLOT_HEADER_SIZE,
                   aOldRow + SMP_SLOT_HEADER_SIZE,
                   sFixedRowSize);

    /* update되는 값을 설정 */
    sCurColumnList = aColumnList;
    sCurValue      = aValueList;

    /* Fixed Row와 별도의 Variable Column에 저장되는 Var Column은
       Update되는 Column만 New Version을 만든다.*/
    for( sColumnSeq = 0 ;
         sCurColumnList != NULL;
         sCurColumnList = sCurColumnList->next, sCurValue++, sColumnSeq++ )
    {
        sColumn = sCurColumnList->column;

        if( (sColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
        {
            IDE_ERROR_RAISE_MSG(
                ( (sCurValue->length <= sColumn->size) &&
                  ( ((sCurValue->length  > 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value == NULL)) ) ),
                err_invalid_column_size,
                "Table OID     : %"ID_vULONG_FMT"\n"
                "Column Seq    : %"ID_UINT32_FMT"\n"
                "Column Size   : %"ID_UINT32_FMT"\n"
                "Column Offset : %"ID_UINT32_FMT"\n"
                "Value Length  : %"ID_UINT32_FMT"\n",
                aHeader->mSelfOID,
                sColumnSeq,
                sColumn->size,
                sColumn->offset,
                sCurValue->length );
        }
        else
        {
            IDE_ERROR_RAISE_MSG(
                ( (sCurValue->length <= sColumn->size) &&
                  ( ((sCurValue->length  > 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value == NULL)) ) ),
                err_invalid_column_size,
                "Table OID     : %"ID_vULONG_FMT"\n"
                "Column Seq    : %"ID_UINT32_FMT"\n"
                "Column Size   : %"ID_UINT32_FMT"\n"
                "Column Offset : %"ID_UINT32_FMT"\n"
                "Value Length  : %"ID_UINT32_FMT"\n",
                aHeader->mSelfOID,
                sColumnSeq,
                sColumn->size,
                sColumn->offset,
                sCurValue->length );
        }

        IDU_FIT_POINT( "2.PROJ-2118@svcRecord::updateVersion" );

        switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
        {
            /* PROJ-2174 Supporting LOB in the volatile tablespace */
            case SMI_COLUMN_TYPE_LOB:

                sCurLobDesc = (smcLobDesc*)(sNewFixRowPtr + sColumn->offset);

                /* SQL로 직접 Lob 데이터를 넣은 경우 */
                IDE_TEST( svcLob::reserveSpaceInternal(
                                      aTrans,
                                      (smcTableHeader *)aHeader,
                                      (smiColumn*)sColumn,
                                      0, /* aLobVersion */
                                      sCurLobDesc,
                                      0,
                                      sCurValue->length,
                                      SM_FLAG_INSERT_LOB,
                                      ID_TRUE /* aIsFullWrite */)
                          != IDE_SUCCESS );

                if( sCurValue->length > 0 )
                {
                    IDE_TEST( svcLob::writeInternal( (UChar*)sNewFixRowPtr,
                                                     (smiColumn*)sColumn,
                                                     0,
                                                     sCurValue->length,
                                                     (UChar*)sCurValue->value )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Here lenght is always 0. */
                    
                    if( sCurValue->value == NULL )
                    {
                        /* null */
                        sCurLobDesc->flag =
                            SM_VCDESC_MODE_IN | SM_VCDESC_NULL_LOB_TRUE;
                    }
                    else
                    {
                        /* empty (nothing to do) */
                    }
                }

                sCurLobDesc->length = sCurValue->length;

                break;

            case SMI_COLUMN_TYPE_VARIABLE:
                sUnitedVarColumns[sUnitedVarColumnCount]        =   sColumn;
                sUnitedVarValues[sUnitedVarColumnCount].length  =   sCurValue->length;
                sUnitedVarValues[sUnitedVarColumnCount].value   =   sCurValue->value;
                sUnitedVarColumnCount++;

                break;

            case SMI_COLUMN_TYPE_FIXED:

                IDU_FIT_POINT( "3.PROJ-2118@svcRecord::updateVersion" );

                // BUG-30104 Fixed Column의 length는 0이 될 수 없습니다.
                IDE_ERROR_MSG( sCurValue->length > 0,
                               "Table OID  : %"ID_vULONG_FMT"\n"
           "Space ID   : %"ID_UINT32_FMT"\n"
                               "Column Seq : %"ID_UINT32_FMT"\n",
                               aHeader->mSelfOID,
                               aHeader->mSpaceID,
                               sColumnSeq );

                idlOS::memcpy( sNewFixRowPtr + sColumn->offset,
                               sCurValue->value,
                               sCurValue->length );

                break;

            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
            default:
                ideLog::log( IDE_ERR_0,
                             "Invalid column[%u] "
                             "flag : %u "
                             "( TableOID : %lu )\n",
                             sColumnSeq,
                             sColumn->flag,
                             aHeader->mSelfOID );

                IDE_ASSERT( 0 );
                break;
        }
    }

    if ( sUnitedVarColumnCount > 0)
    {
        /* 모든 united var column 을 업데이트한다면, list 재구성을 거칠 필요가 없다 */
        if ( aHeader->mUnitedVarColumnCount == sUnitedVarColumnCount )
        {
            IDE_TEST( insertUnitedVarColumns ( aTrans,
                                               aHeader,
                                               sNewFixRowPtr,
                                               sUnitedVarColumns,
                                               ID_TRUE,
                                               sUnitedVarValues,
                                               sUnitedVarColumnCount )
                    != IDE_SUCCESS );

            IDE_TEST( deleteVC( aTrans,
                                aHeader,
                                ((smpSlotHeader*)aOldRow)->mVarOID ) );

        }
        else
        {
            IDE_TEST( svcRecord::updateUnitedVarColumns( aTrans,
                                                         aHeader,
                                                         aOldRow,
                                                         sNewFixRowPtr,
                                                         sUnitedVarColumns,
                                                         sUnitedVarValues,
                                                         sUnitedVarColumnCount )
                    != IDE_SUCCESS );
        }
    }
    else /* united var col update가 발생하지 않은 경우 */
    {
        sFstVCPieceOID = ((smpSlotHeader*)aOldRow)->mVarOID;
        ((smpSlotHeader*)sNewFixRowPtr)->mVarOID = sFstVCPieceOID;
    }
    /* undo log를 기록한다. */
    /* aOldRow->mNext를 바꾸기 전에 로깅해야 이전 값을 알 수 있다. */
    IDE_TEST( svcRecordUndo::logUpdate( smLayerCallback::getLogEnv( aTrans ),
                                        aTrans,
                                        aHeader->mSelfOID,
                                        aHeader->mSpaceID,
                                        aOldRow,
                                        sNewFixRowPtr,
                                        sBeforeLimit )
              != IDE_SUCCESS );

    /* BUG-14513: Fixed Slot을 할당시 Alloc Slot Log를 기록하지
       않도록 수정. Update log가 Alloc Slot에 대한 Redo, Undo수행.
       이 때문에 Update log기록 후 Slot header에 대한 Update수행
    */
    svpFixedPageList::setAllocatedSlot( aInfinite, sNewFixRowPtr );

    *aRow = (SChar*)sNewFixRowPtr;
    if(aRetSlotGRID != NULL)
    {
        SC_MAKE_GRID( *aRetSlotGRID,
                      aHeader->mSpaceID,
                      sPageID,
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sNewFixRowPtr ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_column_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INVALID_COLUMN_SIZE) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if( ideGetErrorCode() == smERR_RETRY_Row_Retry )
    {
        IDE_ASSERT( lockRowInternal( aTrans,
                                     aHeader,
                                     aOldRow ) == IDE_SUCCESS );
        *aRow = aOldRow;
    }

    if( sIsHeadNextChanged == ID_TRUE )
    {
        if( sState == 0 )
        {
            IDE_ASSERT( svmManager::holdPageXLatch( aHeader->mSpaceID,
                                                    sPageID )
                        == IDE_SUCCESS);
            sState = 1;
        }

        SM_SET_SCN( &(sOldSlotHeader->mLimitSCN), &sBeforeLimit );
        SMP_SLOT_INIT_POSITION( sOldSlotHeader );
        SMP_SLOT_SET_USED( sOldSlotHeader );
    }

    if( sState != 0 )
    {
        // PROJ-1784 DML without retry
        // aOldRow가 변경 될 수 있으므로 PID를 다시 가져와야 함
        sPageID = SMP_SLOT_GET_PID( aOldRow );
        IDE_ASSERT(svmManager::releasePageLatch(aHeader->mSpaceID,
                                                sPageID)
                   == IDE_SUCCESS);
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aOldRow를 Update한다. MVCC아니라 Inplace로 Update한다.
 *
 * aTrans           - [IN] Transaction Pointer
 * aHeader          - [IN] Table Header
 * aOldRow          - [IN] 기존 value 를 가진 old row
 * aNewRow          - [IN] 새 value 를 저장할 new row
 * aColumns         - [IN] VC Column Description
 * aAddOIDFlag      - [IN] Variable Column을 구성하는 VC를 Transaction OID List에
 *                         Add를 원하면 ID_TRUE, 아니면 ID_FALSE.
 * aValues          - [IN] Variable Column의 Values
 * aVarcolumnCount  - [IN] Variable Column과 Value의 숫자
 ***********************************************************************/
IDE_RC svcRecord::updateUnitedVarColumns( void                * aTrans,
                                          smcTableHeader      * aHeader,
                                          SChar               * aOldRow,
                                          SChar               * aNewRow,
                                          const smiColumn    ** aColumns,
                                          smiValue            * aValues,
                                          UInt                  aVarColumnCount )
{
    const smiColumn   * sCurColumn = NULL;
    const smiColumn   * sColumns[SMI_COLUMN_ID_MAXIMUM];

    smiValue            sValues[SMI_COLUMN_ID_MAXIMUM];

    UInt                sUnitedVarColCnt    = 0;
    UInt                sUpdateColIdx       = 0;
    UInt                sOldValueCnt        = 0;
    UInt                i                   = 0;
    UInt                sSumOfColCnt        = 0;
    UInt                sOffsetIdxInPiece   = 0;
    UShort            * sCurrOffsetPtr      = NULL;
    smOID               sOID                = SM_NULL_OID;

    smVCPieceHeader   * sVCPieceHeader      = NULL;

    IDE_ASSERT( aTrans      != NULL );
    IDE_ASSERT( aHeader     != NULL );
    IDE_ASSERT( aOldRow     != NULL );
    IDE_ASSERT( aNewRow     != NULL );
    IDE_ASSERT( aColumns    != NULL );


    sOID = ((smpSlotHeader*)aOldRow)->mVarOID;
    
    if ( sOID != SM_NULL_OID )
    {
        IDE_ASSERT( smmManager::getOIDPtr( aColumns[0]->colSpace,
                                           sOID,
                                           (void**)&sVCPieceHeader)
                    == IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    for ( i = 0 ; i < aHeader->mColumnCount ; i++) /* 루프를 돌며 새 value list 생성 */
    {
        sCurColumn = smcTable::getColumn( aHeader, i); // Column Idx

        if ( (sCurColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE )
        {
            sColumns[sUnitedVarColCnt]  = sCurColumn;

            /* update 할 컬럼은 새 value 카피 */
            if ( ( sUpdateColIdx < aVarColumnCount )
                    && ( sCurColumn->id == aColumns[sUpdateColIdx]->id ))
            {
                sValues[sUnitedVarColCnt].length = aValues[sUpdateColIdx].length;
                sValues[sUnitedVarColCnt].value = aValues[sUpdateColIdx].value;
                sUpdateColIdx++;
            }
            else /* 바뀌지 않은 컬럼은 old value 카피 */
            {
                sOldValueCnt++;

                sOffsetIdxInPiece = sCurColumn->varOrder - sSumOfColCnt;

                while ( ( sVCPieceHeader != NULL ) 
                        && ( sOffsetIdxInPiece >= sVCPieceHeader->colCount ))
                {
                    if ( sVCPieceHeader->nxtPieceOID != SM_NULL_OID )
                    {
                        sOffsetIdxInPiece   -= sVCPieceHeader->colCount;
                        sSumOfColCnt        += sVCPieceHeader->colCount;
                        sOID                = sVCPieceHeader->nxtPieceOID;

                        IDE_ASSERT( smmManager::getOIDPtr( sCurColumn->colSpace,
                                                           sOID,
                                                           (void**)&sVCPieceHeader)
                                == IDE_SUCCESS );
                    }
                    else
                    {
                        sVCPieceHeader = NULL;
                        break;
                    }
                }

                if ( ( sVCPieceHeader != NULL )
                        && ( sOffsetIdxInPiece < sVCPieceHeader->colCount ) )
                {
                    sCurrOffsetPtr = (UShort*)(sVCPieceHeader + 1) + sOffsetIdxInPiece;

                    sValues[sUnitedVarColCnt].length    = ( *( sCurrOffsetPtr + 1 )) - ( *sCurrOffsetPtr );
                    sValues[sUnitedVarColCnt].value     = (SChar*)sVCPieceHeader + (*sCurrOffsetPtr);
                }
                else
                {
                    sValues[sUnitedVarColCnt].length    = 0;
                    sValues[sUnitedVarColCnt].value     = NULL;
                }
            }

            sUnitedVarColCnt++;
        }
        else
        {
            /* do nothing */
        }
    }

    IDE_ERROR( sUpdateColIdx + sOldValueCnt == sUnitedVarColCnt );
    IDE_TEST( insertUnitedVarColumns( aTrans,
                                      aHeader,
                                      aNewRow,
                                      sColumns,
                                      ID_TRUE,
                                      sValues,
                                      sUnitedVarColCnt )
            != IDE_SUCCESS );

    IDE_TEST( deleteVC( aTrans,
                        aHeader,
                        ((smpSlotHeader*)aOldRow)->mVarOID ) 
            != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : aOldRow를 Update한다. MVCC아니라 Inplace로 Update한다.
 *
 * aStatistics   - [IN] None
 * aTrans        - [IN] Transaction Pointer
 * aViewSCN      - [IN] 이 연산을 수행하는 Cursor의 ViewSCN
 * aHeader       - [IN] aOldRow가 속해 있는 Table Header Pointer
 * aOldRow       - [IN] Update할 Row
 * aOldGRID      - [IN] None
 * aRow          - [OUT] aOldRow와 같은 값을 넘겨준다.
 * aRetSlotGRID  - [IN] None
 * aColumnList   - [IN] Update될 Column List
 * aValueList    - [IN] Update될 Column들의 새로운 Vaule List
 * aRetryInfo    - [IN] None
 * aInfinite     - [IN] None
 * aLogInfo4Update-[IN] None
 * aOldRecImage  - [IN] None
 * aModifyIdxBit - [IN] Update할 Index List
 * aForbiddenToRetry - [IN] None
 ***********************************************************************/
IDE_RC svcRecord::updateInplace(idvSQL                * /*aStatistics*/,
                                void                  * aTrans,
                                smSCN                   /*aViewSCN*/,
                                void                  * /*aTableInfoPtr*/,
                                smcTableHeader        * aHeader,
                                SChar                 * aRowPtr,
                                scGRID                  /*aOldGRID*/,
                                SChar                ** aRow,
                                scGRID                * aRetSlotGRID,
                                const smiColumnList   * aColumnList,
                                const smiValue        * aValueList,
                                const smiDMLRetryInfo * /*aRetryInfo*/,
                                smSCN                   /*aInfinite*/,
                                void                  * /*aLobInfo4Update*/,
                                ULong                   aModifyIdxBit,
                                idBool                  /*aForbiddenToRetry*/ )
{
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;
    const smiValue        * sCurValue;
    smOID                   sFixedRowID;
    scPageID                sPageID = SM_NULL_PID;
    UInt                    sColumnSeq;
    smcLobDesc            * sCurLobDesc;
    UInt                    sUnitedVarColumnCount = 0;
    const smiColumn       * sUnitedVarColumns[SMI_COLUMN_ID_MAXIMUM];
    smiValue                sUnitedVarValues[SMI_COLUMN_ID_MAXIMUM];

    IDE_ERROR( aTrans      != NULL );
    IDE_ERROR( aHeader     != NULL );
    IDE_ERROR( aRowPtr     != NULL );
    IDE_ERROR( aRow        != NULL );
    IDE_ERROR( aColumnList != NULL );
    IDE_ERROR( aValueList  != NULL );

    /* inplace update에 대한 undo 로그를 먼저 기록한다.
       왜냐하면 update가 기존 row에 대해 직접 일어나기 때문에
       데이터가 변경되기 전에 로깅이 먼저 이루어져야 한다. */
    IDE_TEST( svcRecordUndo::logUpdateInplace( smLayerCallback::getLogEnv( aTrans ),
                                               aTrans,
                                               aHeader->mSpaceID,
                                               aHeader->mSelfOID,
                                               aRowPtr,
                                               aColumnList,
                                               aModifyIdxBit )
              != IDE_SUCCESS );

    sPageID = SMP_SLOT_GET_PID(aRowPtr);

    if ( aModifyIdxBit != 0 )
    {
        /* Table의 Index에서 aRowPtr를 지운다. */
        IDE_TEST( smLayerCallback::deleteRowFromIndex( aRowPtr,
                                                       aHeader,
                                                       aModifyIdxBit )
                  != IDE_SUCCESS );
    }

    /* Fixed Row의 Row ID를 가져온다. */
    sFixedRowID = SM_MAKE_OID( sPageID,
                               SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRowPtr ) );

    sCurColumnList = aColumnList;
    sCurValue      = aValueList;

    /* Update된 VC( Variable Column ) 에 새로운 VC를 만든다.
       VC의 Update는 새로운 Row를 만드는 것으로 수행된다. */
    for( sColumnSeq = 0 ;
         sCurColumnList != NULL;
         sCurColumnList  = sCurColumnList->next, sCurValue++, sColumnSeq++ )
    {
        sColumn = sCurColumnList->column;

        if( (sColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
        {
            IDE_ERROR_RAISE_MSG(
                ( (sCurValue->length <= sColumn->size) &&
                  ( ((sCurValue->length  > 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value == NULL)) ) ),
                err_invalid_column_size,
                "Table OID     : %"ID_vULONG_FMT"\n"
                "Column Seq    : %"ID_UINT32_FMT"\n"
                "Column Size   : %"ID_UINT32_FMT"\n"
                "Column Offset : %"ID_UINT32_FMT"\n"
                "Value Length  : %"ID_UINT32_FMT"\n",
                aHeader->mSelfOID,
                sColumnSeq,
                sColumn->size,
                sColumn->offset,
                sCurValue->length );
        }
        else
        {
            IDE_ERROR_RAISE_MSG(
                ( (sCurValue->length <= sColumn->size) &&
                  ( ((sCurValue->length  > 0) && (sCurValue->value != NULL)) ||
                    ((sCurValue->length == 0) && (sCurValue->value == NULL)) ) ),
                err_invalid_column_size,
                "Table OID     : %"ID_vULONG_FMT"\n"
                "Column Seq    : %"ID_UINT32_FMT"\n"
                "Column Size   : %"ID_UINT32_FMT"\n"
                "Column Offset : %"ID_UINT32_FMT"\n"
                "Value Length  : %"ID_UINT32_FMT"\n",
                aHeader->mSelfOID,
                sColumnSeq,
                sColumn->size,
                sColumn->offset,
                sCurValue->length );
        }

        switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
        {
            /* PROJ-2174 Supporting LOB in the volatile tablespace */
            case SMI_COLUMN_TYPE_LOB:

                sCurLobDesc = (smcLobDesc*)(aRowPtr + sColumn->offset);

                /* smcLob::prepare4WriteInternal에서 sCurLobDesc을 바꿀수
                 * 있기 때문에 데이터를 복사해서 넘겨야 한다.
                 * 왜냐하면 UpdateInplace로 하기때문에 데이터영역에 대한
                 * 갱신을 하기전에 로그를 먼저 기록해야 하나 변경시 로그를
                 * 기록하지 않고 이후에 기록되기 때문이다.
                 */

                /* SQL로 직접 Lob 데이터를 넣은 경우 */
                IDE_TEST( svcLob::reserveSpaceInternal(
                                      aTrans,
                                      (smcTableHeader *)aHeader,
                                      (smiColumn*)sColumn,
                                      0, /* aLobVersion */
                                      sCurLobDesc,
                                      0,
                                      sCurValue->length,
                                      SM_FLAG_INSERT_LOB,
                                      ID_TRUE /* aIsFullWrite */)
                          != IDE_SUCCESS );

                if( sCurValue->length > 0 )
                {
                    IDE_TEST( svcLob::writeInternal( (UChar*)aRowPtr,
                                                     (smiColumn*)sColumn,
                                                     0,
                                                     sCurValue->length,
                                                     (UChar*)sCurValue->value )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Here length is always 0. */
                    
                    if( sCurValue->value == NULL )
                    {
                        /* null */
                        sCurLobDesc->flag
                            = SM_VCDESC_MODE_IN | SM_VCDESC_NULL_LOB_TRUE;
                    }
                    else
                    {
                        /* empty (nothing to do) */
                    }
                }

                sCurLobDesc->length = sCurValue->length;

                break;

            case SMI_COLUMN_TYPE_VARIABLE:
                sUnitedVarColumns[sUnitedVarColumnCount]        =   sColumn;
                sUnitedVarValues[sUnitedVarColumnCount].length  =   sCurValue->length;
                sUnitedVarValues[sUnitedVarColumnCount].value   =   sCurValue->value;
                sUnitedVarColumnCount++;

                break;

            case SMI_COLUMN_TYPE_FIXED:

                // BUG-30104 Fixed Column의 length는 0이 될 수 없습니다.
                IDE_ERROR_MSG( sCurValue->length > 0,
                               "Table OID  : %"ID_vULONG_FMT"\n"
                               "Space ID   : %"ID_UINT32_FMT"\n"
                               "Column Seq : %"ID_UINT32_FMT"\n",
                               aHeader->mSelfOID,
                               aHeader->mSpaceID,
                               sColumnSeq );

                /* BUG-42407
                 * Fixed column의 경우 항상 컬럼 길이가 고정.
                 * source 와 destination 이 같으면 memcpy를 skip 한다. */
                if ( (aRowPtr + sColumn->offset) != (sCurValue->value) )
                {    
                    idlOS::memcpy(aRowPtr + sColumn->offset,
                                  sCurValue->value,
                                  sCurValue->length);
                }    
                else 
                {    
                    /* do nothing */
                }    

                break;

            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
            default:
                ideLog::log( IDE_ERR_0,
                             "Invalid column[%u] "
                             "flag : %u "
                             "( TableOID : %lu )\n",
                             sColumnSeq,
                             sColumn->flag,
                             aHeader->mSelfOID );
                IDE_ASSERT(0);
                break;
        }
    }

    if ( sUnitedVarColumnCount > 0 )
    {
        IDE_TEST( svcRecord::updateUnitedVarColumns( aTrans,
                                                     aHeader,
                                                     aRowPtr,
                                                     aRowPtr,
                                                     sUnitedVarColumns,
                                                     sUnitedVarValues,
                                                     sUnitedVarColumnCount )
                != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    *aRow = (SChar*)aRowPtr;
    if(aRetSlotGRID != NULL)
    {
        SC_MAKE_GRID( *aRetSlotGRID,
                      aHeader->mSpaceID,
                      sPageID,
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRowPtr ) );
    }

    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       aHeader->mSelfOID,
                                       sFixedRowID,
                                       aHeader->mSpaceID,
                                       SM_OID_UPDATE_FIXED_SLOT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_column_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INVALID_COLUMN_SIZE) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * task-2399 Lock Row와 Delete Row를 없애자.
 * 으로 인해 변경된 사항을 알고 싶으면 smcRecord::removeVersion에 적힌 주석을 참조
 ***********************************************************************/
/***********************************************************************
 * Description : aRow가 가리키는 row를 삭제한다.
 *
 * aStatistics    - [IN] None
 * aTrans         - [IN] Transaction Pointer
 * aViewSCN       - [IN] 이 연산을 수행하는 Cursor의 ViewSCN
 * aTableInfoPtr  - [IN] Table Info Pointer(Table에 Transaction이 Insert,
 *                        Delete한 Record Count에 대한 정보를 가지고 있다.

 * aHeader      - [IN] Table Header Pointer
 * aRow         - [IN] 삭제할 Row
 * aRetSlotRID  - [IN] None
 * aInfinite    - [IN] Cursor의 Infinite SCN
 * aIterator    - [IN] None
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC svcRecord::removeVersion( idvSQL               * /*aStatistics*/,
                                 void                 * aTrans,
                                 smSCN                  aViewSCN,
                                 void                 * aTableInfoPtr,
                                 smcTableHeader       * aHeader,
                                 SChar                * aRow,
                                 scGRID                 /*aSlotRID*/,
                                 smSCN                  aInfinite,
                                 const smiDMLRetryInfo* aRetryInfo,
                                 idBool                 aIsDequeue, 
                                 idBool                 aForbiddenToRetry )
{
    const smiColumn * sColumn;
    UInt              sLockState = 0;
    scPageID          sPageID;
    UInt              i;
    smOID             sRemoveOID;
    UInt              sColumnCnt;
    smpSlotHeader   * sSlotHeader = (smpSlotHeader*)aRow;
    smSCN             sDeleteSCN;
    ULong             sNxtSCN;
    smTID             sNxtTID;

    IDE_ERROR( aRow    != NULL );
    IDE_ERROR( aTrans  != NULL );
    IDE_ERROR( aHeader != NULL );

    IDU_FIT_POINT( "1.PROJ-2118@svcRecord::removeVersion" );

    sPageID     = SMP_SLOT_GET_PID(aRow);
    sRemoveOID  = SM_MAKE_OID( sPageID,
                               SMP_SLOT_GET_OFFSET( sSlotHeader ) );
    SMX_GET_SCN_AND_TID( sSlotHeader->mLimitSCN, sNxtSCN, sNxtTID );

    /* BUG-48230: 자신이 이미 lock row를 잡았으면 record lock validation 
     *            할 필요가 없다. */  
    if (( SM_SCN_IS_NOT_LOCK_ROW( sNxtSCN ) ) ||
        ( smxTrans::getTransID( aTrans ) != sNxtTID ))
    {
        /*  remove하기 위하여 이 row가 속한 page에 대하여 holdPageXLatch */
        IDE_TEST( svmManager::holdPageXLatch( aHeader->mSpaceID,
                                              sPageID ) != IDE_SUCCESS );
        sLockState = 1;

        IDE_TEST( recordLockValidation(aTrans,
                                       aViewSCN,
                                       aHeader->mSpaceID,
                                       &aRow,
                                       ID_ULONG_MAX,
                                       &sLockState,
                                       aRetryInfo,
                                       aForbiddenToRetry )
                  != IDE_SUCCESS );

        // PROJ-1784 DML without retry
        // sOldFixRowPtr이 변경 될 수 있으므로 값을 다시 설정해야 함
        // ( 이경우 page latch 도 다시 잡았음 )
        sSlotHeader = (smpSlotHeader*)aRow;
        sPageID     = SMP_SLOT_GET_PID(aRow);
        sRemoveOID  = SM_MAKE_OID( sPageID,
                                   SMP_SLOT_GET_OFFSET( sSlotHeader ) );
    }

    //remove 연산이 수행될 수 있는 row의 조건은 next버전이 없거나,
    //next가 존재한다면, 그것이 lock일 때만 가능하다.
    if( !( SM_SCN_IS_FREE_ROW( sSlotHeader->mLimitSCN ) ||
           SM_SCN_IS_LOCK_ROW( sSlotHeader->mLimitSCN ) ) )
    {
        ideLog::log( IDE_ERR_0,
                     "Table OID : %lu\n"
                     "Space ID  : %u\n"
                     "Page ID   : %u\n",
                     aHeader->mSelfOID,
                     aHeader->mSpaceID,
                     sPageID );

        ideLog::logMem( IDE_ERR_0,
                        (UChar*)sSlotHeader,
                        ID_SIZEOF( smpSlotHeader ),
                        "Slot Header" );

        svpFixedPageList::dumpSlotHeader( sSlotHeader );

        IDE_ASSERT( 0 );
    }

    //slotHeader의 mNext에 들어갈 값을 세팅한다. delete연산이 수행되면
    //이 연산을 수행하는 트랜잭션의 aInfinite정보가 세팅된다. (with deletebit)
    SM_SET_SCN( &sDeleteSCN, &aInfinite );
    SM_SET_SCN_DELETE_BIT( &sDeleteSCN );

    /* BUG-48230: deQueue일 경우 row에 lock이 되어 있다. undo시에 이 lock row를 
     *            제거해주어야 한다. */
    if ( aIsDequeue == ID_FALSE )
    {
        SM_SET_SCN( &sNxtSCN,  &( sSlotHeader->mLimitSCN ) );
    }
    else
    {
        SM_SET_SCN_FREE_ROW( &sNxtSCN );
    }

    /* delete에 대한 로그를 기록한다. */
    IDE_TEST( svcRecordUndo::logDelete( smLayerCallback::getLogEnv( aTrans ),
                                        aTrans,
                                        aHeader->mSelfOID,
                                        aRow,
                                        sNxtSCN )
              != IDE_SUCCESS );

    IDL_MEM_BARRIER;

    IDU_FIT_POINT( "BUG-49063@svcRecord::removeVersion::setLimitSCN" );
    SM_SET_SCN( &(sSlotHeader->mLimitSCN), &sDeleteSCN );

    if( sLockState == 1 )
    {
        IDE_TEST( svmManager::releasePageLatch(aHeader->mSpaceID, sPageID)
                  != IDE_SUCCESS );
        sLockState = 0;
    }
    sColumnCnt = aHeader->mColumnCount;

    for(i=0 ; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn(aHeader, i);

        switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
        {
            /* PROJ-2174 Supporting LOB in the volatile tablespace */
            case SMI_COLUMN_TYPE_LOB:
                /* LOB에 속한 Piece들의 Delete Flag만 Delete되었다고
                 * 설정한다.*/
                IDE_TEST( deleteLob( aTrans, (smcTableHeader *)aHeader, sColumn, aRow)
                          != IDE_SUCCESS );

                break;

            case SMI_COLUMN_TYPE_VARIABLE:
                /* United VC 는 밑에서 한번에 다 지운다 */
                break;

            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                /* volitile 은 geometry 를 지원하지 않으므로 large var가 들어올수 없다 */
                IDE_ASSERT( 0 );
                break;

            case SMI_COLUMN_TYPE_FIXED:
                break;

            default:
                break;
        }
    }

    IDE_TEST( deleteVC( aTrans, 
                        aHeader, 
                        ((smpSlotHeader*)aRow)->mVarOID ) 
            != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       aHeader->mSelfOID,
                                       sRemoveOID,
                                       aHeader->mSpaceID,
                                       SM_OID_DELETE_FIXED_SLOT)
              != IDE_SUCCESS );

    if (aTableInfoPtr != NULL)
    {
        smLayerCallback::decRecCntOfTableInfo( aTableInfoPtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLockState != 0 )
    {
        IDE_PUSH();
        // PROJ-1784 DML without retry
        // aRow가 변경 될 수 있으므로 PID를 다시 가져와야 함
        sPageID = SMP_SLOT_GET_PID(aRow);
        IDE_ASSERT(svmManager::releasePageLatch(aHeader->mSpaceID,
                                                sPageID) == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;
}

/*******************************************************************o****
 * Description :
 ***********************************************************************/
IDE_RC svcRecord::nextOIDall( smcTableHeader * aHeader,
                              SChar          * aCurRow,
                              SChar         ** aNxtRow )
{

    IDE_ASSERT( aHeader != NULL );
    IDE_ASSERT( aNxtRow != NULL );

    return svpAllocPageList::nextOIDall( aHeader->mSpaceID,
                                         aHeader->mFixedAllocList.mVRDB,
                                         aCurRow,
                                         aHeader->mFixed.mVRDB.mSlotSize,
                                         aNxtRow );
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC svcRecord::freeVarRowPending( void            * aTrans,
                                     smcTableHeader  * aHeader,
                                     smOID             aPieceOID,
                                     SChar           * aRow )
{
    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        IDE_TEST(svpVarPageList::addFreeSlotPending(
                     aTrans,
                     aHeader->mSpaceID,
                     aHeader->mVar.mVRDB,
                     aPieceOID,
                     aRow)
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC svcRecord::freeFixRowPending( void            * aTrans,
                                     smcTableHeader  * aHeader,
                                     SChar           * aRow )
{
    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        IDE_TEST(svpFixedPageList::addFreeSlotPending(
                     aTrans,
                     aHeader->mSpaceID,
                     &(aHeader->mFixed.mVRDB),
                     aRow)
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC svcRecord::setFreeVarRowPending( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        smOID             aPieceOID,
                                        SChar           * aRow,
                                        smSCN             aSCN )
{
    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        IDE_TEST(svpVarPageList::freeSlot(aTrans,
                                          aHeader->mSpaceID,
                                          aHeader->mVar.mVRDB,
                                          aPieceOID,
                                          aRow,
                                          SMP_TABLE_NORMAL,
                                          aSCN )
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC svcRecord::setFreeFixRowPending( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        SChar           * aRow,
                                        smSCN             aSCN )
{
    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        IDE_TEST(svpFixedPageList::freeSlot(aTrans,
                                            aHeader->mSpaceID,
                                            &(aHeader->mFixed.mVRDB),
                                            aRow,
                                            SMP_TABLE_NORMAL,
                                            aSCN )
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC svcRecord::setSCN( SChar   *  aRow,
                          smSCN      aSCN )
{
    smpSlotHeader *sSlotHeader;

    sSlotHeader = (smpSlotHeader*)aRow;

    SM_SET_SCN( &(sSlotHeader->mCreateSCN), &aSCN );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC svcRecord::setDeleteBitOnHeader( smpSlotHeader * aRowHeader )
{
    SM_SET_SCN_DELETE_BIT( &(aRowHeader->mCreateSCN) );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : aRowHeader의 SCN에 Delete Bit을 Setting한다.
 *
 * aTrans     - [IN]: Transaction Pointer
 * aRowHeader - [IN]: Row Header Pointer
 ***********************************************************************/
IDE_RC svcRecord::setDeleteBit( scSpaceID          aSpaceID,
                                void             * aRowHeader )
{
    scPageID    sPageID;
    UInt        sState = 0;

    sPageID = SMP_SLOT_GET_PID(aRowHeader);

    IDE_TEST(svmManager::holdPageXLatch(aSpaceID, sPageID) != IDE_SUCCESS);
    sState = 1;

    /* BUG-14953 : PK가 두개들어감. Rollback시 SCN값이
       항상 무한대 Bit가 Setting되어 있어야 한다. 그렇지
       않으면 Index의 Unique Check가 오동작하여 Duplicate
       Key가 들어간다.*/
    SM_SET_SCN_DELETE_BIT( &(((smpSlotHeader*)aRowHeader)->mCreateSCN) );

    IDE_TEST(svmManager::releasePageLatch(aSpaceID, sPageID) != IDE_SUCCESS);
    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(svmManager::releasePageLatch(aSpaceID,
                                                sPageID) == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;
}


/* PROJ-2174 Supporting LOB in the volatile tablespace */
 /***********************************************************************
 * Description : aRID가 가리키는 Variable Column Piece의 Flag값을
 *               aFlag로 바꾼다.
 *
 * aTrans       - [IN] Transaction Pointer
 * aTableOID    - [IN] Table OID
 * aSpaceID     - [IN] Variable Column Piece 가 속한 Tablespace의 ID
 * aVCPieceOID  - [IN] Variable Column Piece OID
 * aVCPiecePtr  - [IN] Variabel Column Piece Ptr
 * aVCPieceFlag - [IN] Variabel Column Piece Flag
 ***********************************************************************/
IDE_RC svcRecord::setFreeFlagAtVCPieceHdr( void       * aVCPiecePtr,
                                           UShort       aVCPieceFreeFlag )
{
    smVCPieceHeader   * sVCPieceHeader;
    UShort              sVCPieceFlag;

    /* fix BUG-27221 : [codeSonar] Null Pointer Dereference */
    IDE_ASSERT( aVCPiecePtr != NULL );

    sVCPieceHeader = (smVCPieceHeader *)aVCPiecePtr;

    sVCPieceFlag = sVCPieceHeader->flag;
    sVCPieceFlag &= ~SM_VCPIECE_FREE_MASK;
    sVCPieceFlag |= aVCPieceFreeFlag;

    sVCPieceHeader->flag = sVCPieceFlag;

    return IDE_SUCCESS;
}

/***********************************************************************
 * task-2399 Lock Row와 Delete Row를 없애자.
 * 으로 인해 변경된 사항을 알고 싶으면 smcRecord::lockRow에 적힌 주석을 참조
 ***********************************************************************/
/***********************************************************************
 * Description : select for update를 수행할때 불리는 함수. 즉, select for update
 *          를 수행한 row에 대해서 Lock연산이 수행된다. 이때, smpSlotHeader의
 *          mNext에 LockRow임을 표시한다.  이정보는 현재 트랜잭션이 수행중인 동안만
 *          유효하다.
 ***********************************************************************/
IDE_RC svcRecord::lockRow(void           * aTrans,
                          smSCN            aViewSCN,
                          smcTableHeader * aHeader,
                          SChar          * aRow,
                          ULong            aLockWaitTime,
                          idBool           aForbiddenToRetry )
                            
{
    smpSlotHeader *sSlotHeader;
    scPageID       sPageID;
    UInt           sState = 0;
    smSCN          sRowSCN;
    smTID          sRowTID;

    sSlotHeader = (smpSlotHeader*)aRow;

    sPageID = SMP_SLOT_GET_PID(sSlotHeader);

    IDE_TEST(svmManager::holdPageXLatch(aHeader->mSpaceID,
                                        sPageID) != IDE_SUCCESS);
    sState = 1;

    SMX_GET_SCN_AND_TID( sSlotHeader->mLimitSCN, sRowSCN, sRowTID );

    //이미 Lock이 걸려 있을 경우엔 다시 lock을 걸지 않는다.
    if ( ( SM_SCN_IS_NOT_LOCK_ROW( sRowSCN ) ) || 
         ( sRowTID != smLayerCallback::getTransID( aTrans ) ) )
    {
        IDE_TEST(recordLockValidation( aTrans,
                                       aViewSCN,
                                       aHeader->mSpaceID,
                                       &aRow,
                                       aLockWaitTime,
                                       &sState,
                                       NULL, // Without Retry Info
                                       aForbiddenToRetry )
                 != IDE_SUCCESS );

        // PROJ-1784 retry info가 없으므로 변경되지 않는다.
        IDE_ASSERT( aRow == (SChar*)sSlotHeader );

        IDE_TEST( lockRowInternal( aTrans,
                                   aHeader,
                                   aRow ) != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST(svmManager::releasePageLatch(aHeader->mSpaceID,
                                          sPageID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(svmManager::releasePageLatch(aHeader->mSpaceID,
                                                sPageID) == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : record loc을 잡는다.
 *               이미 record lock validation이 된 상태에서
 *               Lock row를 하는 경우 사용.
 *
 *     aTrans   - [IN] Transaction Pointer
 *     aHeader  - [IN] Table header
 *     aRow     - [IN] lock을 잡을 row
 ***********************************************************************/
IDE_RC svcRecord::lockRowInternal(void           * aTrans,
                                  smcTableHeader * aHeader,
                                  SChar          * aRow)
{
    smpSlotHeader *sSlotHeader = (smpSlotHeader*)aRow;
    smOID          sRecOID;
    smSCN          sRowSCN;
    smTID          sRowTID;
    ULong          sTransID;

    SMX_GET_SCN_AND_TID( sSlotHeader->mLimitSCN, sRowSCN, sRowTID );

    //이미 자신이 LOCK을 걸었다면 다시 LOCK을 걸려고 하지 않는다.
    if ( ( SM_SCN_IS_LOCK_ROW( sRowSCN ) ) &&
         ( sRowTID == smLayerCallback::getTransID( aTrans ) ) )
    {
        /* do nothing */
    }
    else
    {
        IDE_DASSERT( SM_SCN_IS_FREE_ROW(sSlotHeader->mLimitSCN) );
    
        /* mNext 변경에 따른 physical log 기록 */
        IDE_TEST( svcRecordUndo::logPhysical8( smLayerCallback::getLogEnv( aTrans ),
                                               (SChar*)&sSlotHeader->mLimitSCN,
                                               ID_SIZEOF(ULong) )
                  != IDE_SUCCESS );

        sTransID = (ULong)smLayerCallback::getTransID( aTrans );
        SMP_SLOT_SET_LOCK( sSlotHeader, sTransID );

        sRecOID = SM_MAKE_OID( SMP_SLOT_GET_PID(aRow),
                               SMP_SLOT_GET_OFFSET( sSlotHeader ) );

        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           sRecOID,
                                           aHeader->mSpaceID,
                                           SM_OID_LOCK_FIXED_SLOT )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : lockRow가 수행된 row에 대해 lock을 해제할때 호출되는 함수
 ***********************************************************************/
IDE_RC svcRecord::unlockRow( void           * aTrans,
                             SChar          * aRow )
{
    smpSlotHeader *sSlotHeader;
    smTID          sRowTID;

    sSlotHeader = (smpSlotHeader*)aRow;

    sRowTID = SMP_GET_TID( sSlotHeader->mLimitSCN );

    if ( ( sRowTID == smLayerCallback::getTransID( aTrans ) ) &&
         ( SMP_SLOT_IS_LOCK_TRUE( sSlotHeader ) ) )
    {
        SMP_SLOT_SET_UNLOCK( sSlotHeader );
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : table에 nullrow를 생성
 *
 * memory table의 null row를 삽입하고, table header에
 * nullrow의 OID를 assign 한다.
 *
 * - 2nd code design
 *   + table 타입에 따라 NullRow를 data page에 insert 및 로깅
 *   + insert된 nullrow에 대하여 NULL_ROW_SCN 설정 및 로깅
 *     : SMR_LT_UPDATE의 SMR_SVC_PERS_UPDATE_FIXED_ROW
 *   + table header에 nullrow에 대한 설정 및 로깅
 *     : SMR_LT_UPDATE의 SMR_SVC_TABLEHEADER_SET_NULLROW
 ***********************************************************************/
IDE_RC svcRecord::makeNullRow( void*           aTrans,
                               smcTableHeader* aHeader,
                               smSCN           aInfinite,
                               const smiValue* aNullRow,
                               UInt            aLoggingFlag,
                               smOID*          aNullRowOID )
{
    scPageID        sNullRowPID;
    smSCN           sNullRowSCN;
    smOID           sNullRowOID;
    smpSlotHeader*  sNullSlotHeaderPtr;
    smpSlotHeader   sAfterSlotHeader;

    SM_SET_SCN_NULL_ROW( &sNullRowSCN );

    /* memory table에 nullrow를 삽입한다.*/
    IDE_TEST( insertVersion( NULL,
                             aTrans,
                             NULL,
                             aHeader,
                             aInfinite,
                             (SChar**)&sNullSlotHeaderPtr,
                             NULL,
                             aNullRow,
                             aLoggingFlag )
              != IDE_SUCCESS );
    sNullSlotHeaderPtr->mVarOID = SM_NULL_OID;

    idlOS::memcpy(&sAfterSlotHeader, sNullSlotHeaderPtr, SMP_SLOT_HEADER_SIZE);
    sAfterSlotHeader.mCreateSCN = sNullRowSCN;

    sNullRowPID = SMP_SLOT_GET_PID(sNullSlotHeaderPtr);
    sNullRowOID = SM_MAKE_OID( sNullRowPID,
                               SMP_SLOT_GET_OFFSET( sNullSlotHeaderPtr ) );

    /* insert 된 nullrow에 scn을 설정한다. */
    IDE_TEST(setSCN((SChar*)sNullSlotHeaderPtr,
                    sNullRowSCN) != IDE_SUCCESS);

    *aNullRowOID = sNullRowOID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 여러개의 Variable Column을 하나의 variable slot 에 Insert한다.
 *               항상 out-mode로 저장하며 vcDesc는 fixed row slot header에 있다.
 *
 *               주의: VC Piece의 각 Header에 대해서는 Alloc시 logging을
 *                    하지만 VC Piece의 Value가 저장된 부분에 대해서는 Logging
 *                    하지 않는다. 왜냐하면 Insert, Update시 그 부분에 대한
 *                    Logging을 하기 때문이다. 여기서 Logging을 Update후에
 *                    하더라도 문제가 없는 이 이유는 Update영역의 Before Image가
 *                    의미가 없기 때문이다.
 *
 * aTrans           - [IN] Transaction Pointer
 * aHeader          - [IN] Table Header
 * aFixedRow        - [IN] VC의 VCDesc가 저장될 Fixed Row Pointer
 * aColumns         - [IN] VC Column Description
 * aAddOIDFlag      - [IN] Variable Column을 구성하는 VC를 Transaction OID List에
 *                         Add를 원하면 ID_TRUE, 아니면 ID_FALSE.
 * aValues          - [IN] Variable Column의 Values
 * aVarcolumnCount  - [IN] Variable Column과 Value의 숫자
 ***********************************************************************/
IDE_RC svcRecord::insertUnitedVarColumns( void            *  aTrans,
                                          smcTableHeader  *  aHeader,
                                          SChar           *  aFixedRow,
                                          const smiColumn ** aColumns,
                                          idBool             aAddOIDFlag,
                                          smiValue        *  aValues,
                                          SInt               aVarColumnCount )
{
    smOID         sCurVCPieceOID    = SM_NULL_OID;
    smOID         sNxtVCPieceOID    = SM_NULL_OID;
    
    // Piece 크기 초기화 : LastOffsetArray size
    UInt          sVarPieceLen      = ID_SIZEOF(UShort);
    
    UInt          sPrevVarPieceLen  = 0;
    UShort        sVarPieceColCnt   = 0;
    idBool        sIsTrailingNull   = ID_FALSE;
    SInt          i                 = ( aVarColumnCount - 1 ); /* 가장 마지막 Column 부터 삽입한다. */

    // BUG-43117
    // 현재의 앞 순번(ex. 5번 col이면 4번 col) 컬럼의 패딩 크기
    UInt          sPrvAlign         = 0;
    // 현재 컬럼의 추정 패딩 크기
    UInt          sCurAlign         = 0;

    IDE_ASSERT( aTrans    != NULL );
    IDE_ASSERT( aHeader   != NULL );
    IDE_ASSERT( aFixedRow != NULL );
    IDE_ASSERT( aColumns  != NULL );
    IDE_ASSERT_MSG(( aAddOIDFlag == ID_TRUE ) ||
                   ( aAddOIDFlag == ID_FALSE ),
                   "Add OID Flag : %"ID_UINT32_FMT"\n",
                   aAddOIDFlag );

    /* =================================================================
     * Value덩어리가 SMP_VC_PIECE_MAX_SIZE를 넘는 경우 여러 페이지에 걸쳐 저장하고 넘지
     * 않는다면 하나의 Variable Piece에 저장한다. 이때 VC를 구성할 Piece의 저장 순서
     * 는 맨 마지막 Piece부터 저장한다. 이렇게 하는 이유는 Value의 앞부터 저장할 경우
     * 다음 Piece의 OID를 알수 없기 때문에 Piece에 대한 Logging과 별도로 현재 Piece
     * 의 앞단에 Piece가 존재할 경우 앞단의 Piece의 nextPieceOID에 대한 logging을
     * 별도로 수행해야한다. 하지만 맨 마지막 Piece부터 역순으로 저장할 경우 현재 Piece
     * 의 Next Piece OID을 알기 때문에 Variable Piece의 AllocSlot시 next piece에
     * 대한 oid logging을 같이 하여 logging양을 줄일 수 있다.
     * 하나의 column이 서로 다른 var piece에 나뉘어 저장되지 않는다.
     * ================================================================ */

    /* BUG-44463 SM codesonar warning
     *           aVarColumnCount가 1보다 클때만 해당 함수에 들어오므로 
     *           따로 i가 0보다 큰지는 체크하지 않아도 된다.
     *           해당 함수는 자주 호출되므로 성능향상을 위해 i 값의 체크를 생략한다. */

    /* BUG-43320 마지막 Column이 NULL 이라면
     * Trailing null 이다. */
    if ( aValues[i].length == 0 ) /* 끝컬럼이고 길이가 0인경우, start of trailing null */
    {
        sIsTrailingNull = ID_TRUE;
        i--;
    }
    else
    {
        /* Nothing to do */
    }

    for ( ; i >= 0 ; i-- )
    {
        if ( sIsTrailingNull == ID_TRUE )
        {
            if ( aValues[i].length == 0 )
            {
                continue; /* continuation of trailing null */
            }
            else
            {
                sIsTrailingNull = ID_FALSE;
            }
        }
        else
        {
            /* nothing todo */
        }


        if ( i > 0 ) /* 첫컬럼 제외한 나머지 컬럼들 */
        {
            sVarPieceColCnt++;

            // BUG-43117 : varPiece의 길이를 추정

            // value의 길이 + offset array 1칸 크기
            sVarPieceLen += aValues[i].length + ID_SIZEOF(UShort);

            // 현재의 앞 순번 컬럼의 패딩 크기 추정   
            sPrvAlign = SMC_GET_COLUMN_PAD_LENGTH( aValues[i-1], aColumns[i-1] );            

            // 현재 컬럼의 패딩 크기 추정              
            sCurAlign = SMC_GET_COLUMN_PAD_LENGTH( aValues[i], aColumns[i] );

            // 추정한 현재 컬럼의 패딩크기 varPiece 길이에 추가
            sVarPieceLen += sCurAlign;  
            
            // 현재 앞 순번 컬럼의 길이
            sPrevVarPieceLen = aValues[i - 1].length;

            /* 현재 까지 추정되는 varPiece의 총 길이 =
             * 누적된 varPiece 길이 + 앞 순번 컬럼의 길이(패딩+value길이)
             *                      + 앞 순번 offset array 크기
             */
            if ( ( sVarPieceLen + sPrvAlign + sPrevVarPieceLen + ID_SIZEOF(UShort) )  
                 <= SMP_VC_PIECE_MAX_SIZE )
            {
                /* Previous Column 까지 지금의 slot 에 저장 가능 하다면 그냥 넘어간다 */
            }
            else
            {
                /* Previous Column을 더하면 최대크기를 넘기므로, 지금까지 컬럼들 insert */
                IDE_TEST( insertUnitedVarPiece( aTrans,
                                                aHeader,
                                                aAddOIDFlag,
                                                &aValues[i],
                                                aColumns,   /* BUG-43117 */
                                                sVarPieceLen,
                                                sVarPieceColCnt,
                                                sNxtVCPieceOID,
                                                &sCurVCPieceOID )
                        != IDE_SUCCESS );

                sNxtVCPieceOID  = sCurVCPieceOID;
                sVarPieceColCnt = 0;
                
                /* insert 했으므로 다시 초기화 */
                sVarPieceLen    =  ID_SIZEOF(UShort);
            }
        }
        else /* 첫컬럼에 도달한 경우, insert */
        {
            sVarPieceColCnt++; 

            // value의 길이 + offset array 1칸 크기
            sVarPieceLen += aValues[i].length + ID_SIZEOF(UShort);

            //현재 컬럼의 패딩 크기 추정
            sCurAlign = SMC_GET_COLUMN_PAD_LENGTH( aValues[i], aColumns[i] );
 
            sVarPieceLen += sCurAlign;  /* BUG-43117 */

            IDE_TEST( insertUnitedVarPiece( aTrans,
                                            aHeader,
                                            aAddOIDFlag,
                                            &aValues[i],
                                            aColumns,
                                            sVarPieceLen,
                                            sVarPieceColCnt,
                                            sNxtVCPieceOID,
                                            &sCurVCPieceOID )
                      != IDE_SUCCESS );
        }
    }

    /* fixed row 에서 첫 var piece 로 이어지도록 oid 설정 */
    ((smpSlotHeader*)aFixedRow)->mVarOID = sCurVCPieceOID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 여러개의 Variable Column을 하나의 variable slot 에 Insert한다.
 *               항상 out-mode로 저장하며 vcDesc는 fixed row slot header에 있다.
 *
 *               주의: VC Piece의 각 Header에 대해서는 Alloc시 logging을
 *                    하지만 VC Piece의 Value가 저장된 부분에 대해서는 Logging
 *                    하지 않는다. 왜냐하면 Insert, Update시 그 부분에 대한
 *                    Logging을 하기 때문이다. 여기서 Logging을 Update후에
 *                    하더라도 문제가 없는 이 이유는 Update영역의 Before Image가
 *                    의미가 없기 때문이다.
 *
 * aTrans           - [IN] Transaction Pointer
 * aHeader          - [IN] Table Header
 * aFixedRow        - [IN] VC의 VCDesc가 저장될 Fixed Row Pointer
 * aColumns         - [IN] VC Column Description
 * aAddOIDFlag      - [IN] Variable Column을 구성하는 VC를 Transaction OID List에
 *                         Add를 원하면 ID_TRUE, 아니면 ID_FALSE.
 * aValues          - [IN] Variable Column의 Values
 * aVarcolnCount    - [IN] Variable Column과 Value의 숫자
 ***********************************************************************/
IDE_RC svcRecord::insertUnitedVarPiece( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        idBool            aAddOIDFlag,
                                        smiValue        * aValues,
                                        const smiColumn** aColumns, /* BUG-43117 */
                                        UInt              aTotalVarLen,
                                        UShort            aVarColCount,
                                        smOID             aNxtVCPieceOID,
                                        smOID           * aCurVCPieceOID )
{
    SChar       * sPiecePtr         = NULL;
    UShort      * sColOffsetPtr     = NULL;
    smiValue    * sCurValue         = NULL;
    UShort        sValueOffset      = 0;
    SInt          i                 = 0;

    /* PROJ-2419 allocSlot 내부에서 Variable Column의
     * NxtOID는 NULLOID로, Flag는 Used로 초기화 되어 넘어온다. */
    IDE_TEST( svpVarPageList::allocSlot( aTrans,
                                         aHeader->mSpaceID,
                                         aHeader->mSelfOID,
                                         aHeader->mVar.mVRDB,
                                         aTotalVarLen,
                                         aNxtVCPieceOID,
                                         aCurVCPieceOID,
                                         &sPiecePtr )
            != IDE_SUCCESS );

    /* VarPiece Header의 Count값을 갱신 */
    ((smVCPieceHeader*)sPiecePtr)->colCount = aVarColCount;

    /* VarPiece의 Offset Array의 시작 주소를 가져옴 */
    sColOffsetPtr = (UShort*)(sPiecePtr + ID_SIZEOF(smVCPieceHeader));

    sValueOffset = ID_SIZEOF(smVCPieceHeader) + ( ID_SIZEOF(UShort) * (aVarColCount+1) );

    for (i = 0; i < aVarColCount; i++)
    {
        sCurValue   = &aValues[i];

        /* BUG-43287 length가 0인 column, 즉 NULL Column의 경우
         * Column 자신의 align 값을 사용하여 align을 맞추면 안된다.
         * 다음 Column의 align 값이 더 큰 값일 경우
         * 시작 Offset이 달라져서 NULL Column임에도 Size가 0이 아니게되어
         * NULL Column이라고 판단을 하지 못하게 되기 때문
         * NULL의 경우 Column List 내부의 가장 큰 align 값으로 align을 맞춰주어야 한다. */
        if ( sCurValue->length != 0 )
        {
            //BUG-43117 align problem
            sValueOffset = (UShort) idlOS::align( (UInt) sValueOffset, aColumns[i]->align);

            sColOffsetPtr[i] = sValueOffset;

            idlOS::memcpy( sPiecePtr + sValueOffset, sCurValue->value, sCurValue->length );

            sValueOffset += sCurValue->length;
        }
        else
        {
            sValueOffset = (UShort) idlOS::align( (UInt) sValueOffset, aColumns[i]->maxAlign );
            sColOffsetPtr[i] = sValueOffset;
        }
    }
    //var piece 의 끝 offset 저장. 사이즈 구하는데 필요.
    sColOffsetPtr[aVarColCount] = sValueOffset;

    if ( aAddOIDFlag == ID_TRUE )
    {
        /* 새로 할당받은 variable piece를 versioning list에 추가 */
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           *aCurVCPieceOID,
                                           aHeader->mSpaceID,
                                           SM_OID_NEW_VARIABLE_SLOT )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : aRow에 대해서 Delete나 Update할수있는지 조사한다.
 *
 *               :check
 *               if aRow doesn't have next version, OK
 *               else
 *                  if tid of next version is equal to aRow's, OK
 *                  else
 *                      if scn of next version is infinite,
 *                          wait for transaction of next version to end,
 *                          when wake up, goto check
 *                      end
 *                  end
 *                end
 *
 *
 * aTrans        - [IN] Transaction Pointer
 * aViewSCN      - [IN] ViewSCN
 * aSpaceID      - [IN] SpaceID
 * aRow          - [IN] Delete나 Update대상 Row
 * aLockWaitTime - [IN] Lock Wait시 Time Out시간.
 * aState        - [IN] function return시 현재 Row가 속해있는 Page가 Latch를
 *                      잡고 있다면 1, 아니면 0
 *
 * aRetryInfo    - [IN] Retry Info
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC svcRecord::recordLockValidation(void                   *aTrans,
                                       smSCN                   aViewSCN,
                                       scSpaceID               aSpaceID,
                                       SChar                 **aRow,
                                       ULong                   aLockWaitTime,
                                       UInt                   *aState,
                                       const smiDMLRetryInfo * aRetryInfo,
                                       idBool                  aForbiddenToRetry )
{
    smpSlotHeader * sCurSlotHeaderPtr;
    smpSlotHeader * sPrvSlotHeaderPtr;
    scPageID        sPageID;
    scPageID        sNxtPageID;
    smTID           sWaitTransID = SM_NULL_TID;
    smSCN           sNxtRowSCN;
    smTID           sNxtRowTID;
    smTID           sTransID;
    smOID           sNextOID;

    IDE_ASSERT( aTrans != NULL );
    IDE_ASSERT( aRow   != NULL );
    IDE_ASSERT( *aRow  != NULL );
    IDE_ASSERT( aState != NULL );

    sPrvSlotHeaderPtr = (smpSlotHeader *)*aRow;
    sCurSlotHeaderPtr = sPrvSlotHeaderPtr;

    /* BUG-15642 Record가 두번 Free되는 경우가 발생함.:
     * 이런 경우가 발생하기전에 validateUpdateTargetRow에서
     * 체크하여 문제가 있다면 Rollback시키고 다시 retry를 수행시킴*/
    IDE_TEST_RAISE( validateUpdateTargetRow( aTrans,
                                             aSpaceID,
                                             aViewSCN,
                                             sCurSlotHeaderPtr,
                                             aRetryInfo )
                    != IDE_SUCCESS, already_modified );

    sPageID = SMP_SLOT_GET_PID(*aRow);

    sTransID = smLayerCallback::getTransID( aTrans );

    while(! SM_SCN_IS_FREE_ROW( sCurSlotHeaderPtr->mLimitSCN ) /* aRow가 최신버전인가?*/)
    {
        SMX_GET_SCN_AND_TID( sCurSlotHeaderPtr->mLimitSCN, sNxtRowSCN, sNxtRowTID );

        /* Next Version이 Lock Row인 경우 */
        if( SM_SCN_IS_LOCK_ROW( sNxtRowSCN ) )
        {
            if( sNxtRowTID == sTransID )
            {
                /* aRow에 대한 Lock을 aTrans가 이미 잡았다.*/
                break;
            }
            else
            {
                /*다른 트랜잭션이 lock을 잡았다면 그가 끝나기를 기다린다.*/
            }
        }
        else
        {
            if( SM_SCN_IS_NOT_INFINITE( sNxtRowSCN ) )
            {
                IDE_TEST_RAISE( aRetryInfo == NULL, already_modified );

                // 바로 처리한다.
                IDE_TEST_RAISE( SM_SCN_IS_DELETED( sNxtRowSCN ), already_modified );

                sNextOID = SMP_SLOT_GET_NEXT_OID( sCurSlotHeaderPtr );

                IDE_ASSERT( SM_IS_VALID_OID( sNextOID ) );

                // Next Row 가 있는 경우 따라간다
                // Next Row의 Page ID가 다르면 Page Latch를 다시 잡는다.
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                                   sNextOID,
                                                   (void**)&sCurSlotHeaderPtr )
                            == IDE_SUCCESS );

                sNxtPageID = SMP_SLOT_GET_PID( sCurSlotHeaderPtr );

                IDE_ASSERT( sNxtPageID != SM_NULL_PID );

                if( sPageID != sNxtPageID )
                {
                    *aState = 0;
                    IDE_TEST( svmManager::releasePageLatch( aSpaceID, sPageID )
                              != IDE_SUCCESS );
                    // to Next

                    IDE_TEST(svmManager::holdPageXLatch( aSpaceID, sNxtPageID ) != IDE_SUCCESS);
                    *aState = 1;
                    sPageID = sNxtPageID;
                }
            } //if
        } // else

        sWaitTransID = sNxtRowTID;

        *aState = 0;
        IDE_TEST( svmManager::releasePageLatch( aSpaceID, sPageID )
                  != IDE_SUCCESS );

        /* Next Version의 Transaction이 끝날때까지 기다린다. */
        IDE_TEST( smLayerCallback::waitLockForTrans( aTrans,
                                                     sWaitTransID,
                                                     aSpaceID,
                                                     aLockWaitTime )
                  != IDE_SUCCESS );

        IDE_TEST(svmManager::holdPageXLatch(aSpaceID, sPageID) != IDE_SUCCESS);
        *aState = 1;
    } // while

    if( sCurSlotHeaderPtr != sPrvSlotHeaderPtr )
    {
        IDE_ASSERT( aRetryInfo != NULL );
        IDE_ASSERT( aRetryInfo->mIsRowRetry == ID_FALSE );

        // Cur과 prv를 비교한다.
        if( aRetryInfo->mStmtRetryColLst != NULL )
        {
            IDE_TEST_RAISE( isSameColumnValue( aSpaceID,
                                               aRetryInfo->mStmtRetryColLst,
                                               sPrvSlotHeaderPtr,
                                               sCurSlotHeaderPtr )
                            == ID_FALSE , already_modified );
        }

        if( aRetryInfo->mRowRetryColLst != NULL )
        {
            IDE_TEST_RAISE( isSameColumnValue( aSpaceID,
                                               aRetryInfo->mRowRetryColLst,
                                               sPrvSlotHeaderPtr,
                                               sCurSlotHeaderPtr )
                            == ID_FALSE , need_row_retry );
        }
    }

    *aRow = (SChar*)sCurSlotHeaderPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( already_modified );
    {
        if( aForbiddenToRetry == ID_TRUE )
        {
            IDE_DASSERT( ((smxTrans*)aTrans)->mIsGCTx == ID_TRUE );

            SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];
            idlOS::snprintf( sMsgBuf,
                             SMI_MAX_ERR_MSG_LEN,
                             "[RECORD VALIDATION] "
                             "SpaceID:%"ID_UINT32_FMT", "
                             "ViewSCN:%"ID_UINT64_FMT", "
                             "CreateSCN:%"ID_UINT64_FMT", "
                             "LimitSCN:%"ID_UINT64_FMT,
                             aSpaceID,
                             SM_SCN_TO_LONG(aViewSCN),
                             SM_SCN_TO_LONG(sCurSlotHeaderPtr->mCreateSCN),
                             SM_SCN_TO_LONG(sCurSlotHeaderPtr->mLimitSCN) );

            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) );

            IDE_ERRLOG( IDE_SD_19 );
        }
        else
        {
            IDE_SET( ideSetErrorCode(smERR_RETRY_Already_Modified) );
        }
    }
    IDE_EXCEPTION( need_row_retry );
    {
        IDE_SET(ideSetErrorCode (smERR_RETRY_Row_Retry));
    }
    IDE_EXCEPTION_END;

    *aRow = (SChar*)sCurSlotHeaderPtr;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Column list를 기준으로 두개의 row의 value가 같은지
 *               비교한다.
 *
 * aSpaceID          - [IN] Space ID
 * aChkColumnList    - [IN] 비교 할 column 의 list
 * aPrvSlotHeaderPtr - [IN] 비교 할 Record 1
 * aCurSlotHeaderPtr - [IN] 비교 할 Record 2
 *
 ***********************************************************************/
idBool svcRecord::isSameColumnValue( scSpaceID               /* aSpaceID */,
                                     const smiColumnList   * aChkColumnList,
                                     smpSlotHeader         * aPrvSlotHeaderPtr,
                                     smpSlotHeader         * aCurSlotHeaderPtr )
{
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;

    SChar           * sPrvRowPtr = (SChar*)aPrvSlotHeaderPtr;
    SChar           * sCurRowPtr = (SChar*)aCurSlotHeaderPtr;
    SChar           * sPrvValue         = NULL;
    SChar           * sCurValue         = NULL;
    idBool            sIsSameColumnValue = ID_TRUE;
    smcLobDesc      * sPrvLobDesc;
    smcLobDesc      * sCurLobDesc;
    UInt              sPrvVCLength  = 0;
    UInt              sCurVCLength  = 0;

    IDE_ASSERT( sPrvRowPtr != NULL );
    IDE_ASSERT( sCurRowPtr != NULL );

    /* Fixed Row와 별도의 Variable Column에 저장되는 Var Column은
       Update되는 Column만 New Version을 만든다.*/
    for( sCurColumnList = aChkColumnList;
         (( sCurColumnList != NULL ) &&
          ( sIsSameColumnValue != ID_FALSE )) ;
         sCurColumnList = sCurColumnList->next )
    {
        sColumn = sCurColumnList->column;

        switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
        {
            case SMI_COLUMN_TYPE_FIXED:

                if( idlOS::memcmp( sCurRowPtr + sColumn->offset,
                                   sPrvRowPtr + sColumn->offset,
                                   sColumn->size ) != 0 )
                {
                    sIsSameColumnValue = ID_FALSE;
                }

                break;

            case SMI_COLUMN_TYPE_VARIABLE:
                sPrvValue = sgmManager::getVarColumn( sPrvRowPtr, sColumn, &sPrvVCLength );
                sCurValue = sgmManager::getVarColumn( sCurRowPtr, sColumn, &sCurVCLength );

                if ( sPrvVCLength != sCurVCLength )
                {
                    sIsSameColumnValue = ID_FALSE;
                }
                else
                {
                    if ( idlOS::memcmp( sPrvValue,
                                        sCurValue, 
                                        sCurVCLength ) != 0 )
                    {
                        sIsSameColumnValue = ID_FALSE;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                break;

            case SMI_COLUMN_TYPE_LOB:

                sPrvLobDesc = (smcLobDesc*)(sPrvRowPtr + sColumn->offset);
                sCurLobDesc = (smcLobDesc*)(sCurRowPtr + sColumn->offset);

                /* BUG-41952
                   1. SM_VCDESC_MODE_IN 인 경우(즉, in-mode LOB인 경우),
                      smcLobDesc 만큼 비교하면 안됩니다.
                      in-mode의 경우 smVCDescInMode만 저장되고, 뒤에 데이터가 저장됩니다.
                   2. smVCDescInMode 와 smcLobDesc 둘 다 구조체입니다.
                      in-mode, out-mode 모두 memcmp를 사용하면 안되고,
                      구조체 안의 멤버를 직접 비교 해야 합니다. */

                // In-mode와 out-mode에 공동적으로 사용되는 값 비교
                if ( ( sPrvLobDesc->flag   != sCurLobDesc->flag   ) ||
                     ( sPrvLobDesc->length != sCurLobDesc->length ) )
                {
                    sIsSameColumnValue = ID_FALSE;
                    break;
                }

                if ( SM_VCDESC_IS_MODE_IN( sCurLobDesc) )
                {
                    // In-mode일 경우에는 내부 값을 비교
                    if ( idlOS::memcmp( ((UChar*)sPrvLobDesc) + ID_SIZEOF(smVCDescInMode),
                                        ((UChar*)sCurLobDesc) + ID_SIZEOF(smVCDescInMode),
                                        sCurLobDesc->length ) != 0 )
                    {
                        sIsSameColumnValue = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    IDE_DASSERT( ( sCurLobDesc->flag & SM_VCDESC_MODE_MASK ) == SM_VCDESC_MODE_OUT );

                    /* Out-mode일 경우에는 각 구조체의 값들을 비교한다.
                       1. LOB 전체가 갱신되었을 경우,
                          mLobVersion은 0이 되고, mFirstLPCH가 새롭게 할당되며,
                          할당된 mFirstLPCH[0]의 mOID가 fstPieceOID로 할당이 된다.
                          그러므로 mFirstLPCH의 값과 fstPieceOID 값이 다른지 비교하면 된다.
                       2. LOB 부분만 갱신되었을 경우,
                          mLobVersion이 증가하게 되고, mLobVersion만 비교하면 된다.
                       3. LOB에서 LPCH가 추가된 경우,
                          mLPCHCount 값이 증가하므로, mLPCHCount만 비교하면 된다.
                       즉, 각 구조체의 멤버 변수를 비교하는 것으로 컬럼 값의 동일 유무 판단 가능 */
                    if ( ( sPrvLobDesc->fstPieceOID != sCurLobDesc->fstPieceOID ) ||
                         ( sPrvLobDesc->mLPCHCount  != sCurLobDesc->mLPCHCount  ) ||
                         ( sPrvLobDesc->mLobVersion != sCurLobDesc->mLobVersion ) ||
                         ( sPrvLobDesc->mFirstLPCH  != sCurLobDesc->mFirstLPCH  ) )
                    {
                        sIsSameColumnValue = ID_FALSE;
                        break;
                    }
                }

                break;

            /* volatile에는 Geometry가 존재하지 않는다. */
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
            default:
                IDE_ASSERT(0);
                break;
        }
    }

    return sIsSameColumnValue;

}

/***********************************************************************
 *  Update, delete, lock row하려는 row가 정상인지 Test한다.
 *  다음과 같은 체크를 수행한다.
 *
 *  1. 선택한 Row에 Delete Bit가 Setting될 수가 없다.
 *  2. 선택한 Row는 Row의 SCN이 SM_SCN_ROLLBACK_NEW가 될수가 없다.
 *  3. Row의 SCN이 무한대가 아니라면 Row의 SCN은 반드시 aViewSCN보다 작아야 한다.
 *
 *  aRowPtr   - [IN] Row Pointer
 *  aSpaceID  - [IN] Row의 Table이 속한 Tablespace의 ID
 *  aViewSCN  - [IN] View SCN
 *  aRowPtr   - [IN] 검증을 수행할 Row Pointer
 *  aRetryInfo- [IN] Retry Info
 *
 ***********************************************************************/
IDE_RC svcRecord::validateUpdateTargetRow(void                  * aTrans,
                                          scSpaceID               aSpaceID,
                                          smSCN                   aViewSCN,
                                          void                  * aRowPtr,
                                          const smiDMLRetryInfo * aRetryInfo)
{
    smSCN           sRowSCN;
    smTID           sRowTID;
    smOID           sRowOID;
    smpSlotHeader * sSlotHeader;
    idBool          sIsError = ID_FALSE;
    void          * sLogSlotPtr;
    smxTrans      * sTrans = (smxTrans*)aTrans;

    sSlotHeader = (smpSlotHeader *)aRowPtr;

    SMX_GET_SCN_AND_TID( sSlotHeader->mCreateSCN, sRowSCN, sRowTID );

    /* 1. 선택한 Row에 Delete Bit가 Setting될 수가 없다. */
    if( SM_SCN_IS_DELETED( sRowSCN ) )
    {
        // Delete될 Row를 Update하려고 한다.
        sIsError = ID_TRUE;

        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    "[##WARNING ##WARNING!!] The Row was already set deleted\n");
    }

    if( sIsError == ID_FALSE )
    {
        /* 2. 선택한 Row는 Row의 SCN이 SM_SCN_ROLLBACK_NEW가 될수가 없다. */
        if( SM_SCN_IS_FREE_ROW( sRowSCN ) )
        {
            // 이미 Ager가 Free한 Row를 Free하려고 한다.
            sIsError = ID_TRUE;
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        "[##WARNING ##WARNING!!] The Row was already freed\n");
        }
    }

    if( sIsError == ID_FALSE )
    {
        /* 3. 만약 Row의 SCN이 무한대라면, Row의 TID는 이 연산을 수행하는
           Transaction과 동일해야 한다. */
        if( SM_SCN_IS_INFINITE( sRowSCN ) )
        {
            if ( smLayerCallback::getTransID( aTrans ) != sRowTID )
            {
                sIsError = ID_TRUE;
                ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                            "[##WARNING ##WARNING!!] The Row is invalid target row\n");
            }
            else
            {
                /* PROJ-2694 Fetch Across Rollback
                 * holdable cursor open 이전 자신이 생성한 row에 접근했을 경우
                 * 해당 Tx를 rollback시 cursor를 재활용할 수 없다. */
                if ( ( sTrans->mCursorOpenInfSCN != SM_SCN_INIT ) &&
                     ( SM_SCN_IS_LT( &sRowSCN, &( sTrans->mCursorOpenInfSCN ) ) == ID_TRUE ) )
                {
                    sTrans->mIsReusableRollback = ID_FALSE;
                }
            }
        }
        else
        {
            /* 4. 무한대가 아니라면 Row의 SCN은 반드시 aViewSCN보다 작아야 한다. */
            if( SM_SCN_IS_GT( &sRowSCN, &aViewSCN ) )
            {
                /* PROJ-1784 Row retry 에서는 View SCN보다 클수도 있다 */
                if( aRetryInfo == NULL )
                {
                    sIsError = ID_TRUE;
                    ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                                "[##WARNING ##WARNING!!] This statement can't read this row\n");
                }
                else
                {
                    if( aRetryInfo->mIsRowRetry == ID_FALSE )
                    {
                        sIsError = ID_TRUE;
                        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                                    "[##WARNING ##WARNING!!] This statement can't read this row\n");
                    }
                }
            }
        }
    }

    if( sIsError == ID_TRUE )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     "TID:[%d] View SCN:[%llx]\n",
                     smLayerCallback::getTransID( aTrans ),
                     SM_SCN_TO_LONG( aViewSCN ) );

        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     "[##WARNING ##WARNING!!] Target row's info\n");
        smcRecord::logSlotInfo(aRowPtr);

        sRowOID = SMP_SLOT_GET_NEXT_OID( sSlotHeader );

        if( SM_IS_VALID_OID( sRowOID ) )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                         "[##WARNING ##WARNING!!] Next row of target row info\n");
            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                               sRowOID,
                                               &sLogSlotPtr )
                        == IDE_SUCCESS );
            smcRecord::logSlotInfo( sLogSlotPtr );
        }
        else
        {
            if( SMP_SLOT_IS_LOCK_TRUE( sSlotHeader ))
            {
                ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                             "[##WARNING ##WARNING!!] target row is locked \n");
            }
            else
            {
                if( SM_SCN_IS_FREE_ROW( sSlotHeader->mLimitSCN ) )
                {
                    ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                                 "[##WARNING ##WARNING!!] Next pointer of target row has null \n");
                }
                else
                {
                    ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                                 "[##WARNING ##WARNING!!] target row is deleted row \n");
                }
            }
        }
    }

    IDE_TEST_RAISE( sIsError == ID_TRUE, err_invalide_version);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalide_version );
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aFixedRow에서 aColumn이 가리키는 Variable Column을 삭제한다.
 *
 * aTrans     - [IN] Transaction Pointer
 * aHeader    - [IN] Table Header
 * aFstOID    - [IN] VC Piece 의 첫 OID
 ***********************************************************************/
IDE_RC svcRecord::deleteVC(void              *aTrans,
                           smcTableHeader    *aHeader,
                           smOID              aFstOID )
{
    smOID             sVCPieceOID       = SM_NULL_OID;
    smOID             sNxtVCPieceOID    = SM_NULL_OID;
    SChar            *sVCPiecePtr       = NULL;

    sVCPieceOID = aFstOID;

    /* VC가 여러개의 VC Piece구성될 경우 모든
       Piece에 대해서 삭제 작업을 수행한다. */
    while ( sVCPieceOID != SM_NULL_OID )
    {
        IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                           sVCPieceOID,
                                           (void**)&sVCPiecePtr )
                == IDE_SUCCESS );

        sNxtVCPieceOID = ((smVCPieceHeader*)sVCPiecePtr)->nxtPieceOID;

        IDE_TEST( svcRecordUndo::logPhysical8( smLayerCallback::getLogEnv( aTrans ),
                                               (SChar*)&((smVCPieceHeader*)sVCPiecePtr)->flag,
                                               ID_SIZEOF(((smVCPieceHeader*)sVCPiecePtr)->flag) )
                  != IDE_SUCCESS );

        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           sVCPieceOID,
                                           aHeader->mSpaceID,
                                           SM_OID_OLD_VARIABLE_SLOT )
                != IDE_SUCCESS );

        sVCPieceOID = sNxtVCPieceOID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * description : aRow에서 aColumn이 가리키는 vc의 Header를 구한다.
 *
 * aRow     - [in] fixed row pointer
 * aColumn  - [in] column desc
 ***********************************************************************/
IDE_RC svcRecord::getVCPieceHeader( const void      *  aRow,
                                    const smiColumn *  aColumn,
                                    smVCPieceHeader ** aVCPieceHeader,
                                    UShort          *  aOffsetIdx )
{
    UShort            sOffsetIdx        = ID_USHORT_MAX;
    smVCPieceHeader * sVCPieceHeader    = NULL;
    smOID             sOID              = SM_NULL_OID;

    IDE_ASSERT( aRow            != NULL );
    IDE_ASSERT( aColumn         != NULL );

    sOID        = ((smpSlotHeader*)aRow)->mVarOID;

    IDE_DASSERT( sOID != SM_NULL_OID );

    IDE_ASSERT( smmManager::getOIDPtr( aColumn->colSpace,
                                       sOID,
                                       (void**)&sVCPieceHeader)
            == IDE_SUCCESS );

    sOffsetIdx = aColumn->varOrder;

    while ( sOffsetIdx >= sVCPieceHeader->colCount )
    {
        if ( sVCPieceHeader->nxtPieceOID != SM_NULL_OID )
        {
            sOffsetIdx -= sVCPieceHeader->colCount;
            sOID    = sVCPieceHeader->nxtPieceOID;

            IDE_ASSERT( smmManager::getOIDPtr( aColumn->colSpace,
                                               sOID,
                                               (void**)&sVCPieceHeader)
                    == IDE_SUCCESS );
        }
        else
        {
            sVCPieceHeader  = NULL;
            sOffsetIdx      = ID_USHORT_MAX;
            break;
        }
    }

    *aOffsetIdx     = sOffsetIdx;
    *aVCPieceHeader = sVCPieceHeader;

    return IDE_SUCCESS;
}

/* PROJ-2174 Supporting LOB in the volatile tablespace */
 /***********************************************************************
 * Description : aRow의 aColumn이 가리키는 Lob Column을 지운다.
 *
 * aTrans    - [IN] Transaction Pointer
 * aHeader   - [IN] Table Header
 * aColumn   - [IN] Table Column List
 * aRow      - [IN] Row Pointer
 ***********************************************************************/
IDE_RC svcRecord::deleteLob(void              * aTrans,
                            smcTableHeader    * aHeader,
                            const smiColumn   * aColumn,
                            SChar             * aRow)
{
    smVCDesc    * sVCDesc;
    smcLobDesc  * sCurLobDesc;

    IDE_DASSERT( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                 == SMI_COLUMN_TYPE_LOB);

    sCurLobDesc = (smcLobDesc *)(aRow + aColumn->offset);

    if((sCurLobDesc->flag & SM_VCDESC_MODE_MASK) ==
       SM_VCDESC_MODE_OUT)
    {
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           (smOID)(sCurLobDesc->mFirstLPCH),
                                           aHeader->mSpaceID,
                                           SM_OID_OLD_LPCH )
                  != IDE_SUCCESS );
    }

    sVCDesc = getVCDesc(aColumn, aRow);

    if ( ( sVCDesc->flag & SM_VCDESC_MODE_MASK ) == SM_VCDESC_MODE_OUT )
    {
        IDE_TEST(deleteVC(aTrans, aHeader, sVCDesc->fstPieceOID)
                != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do. inmode 는 fixed record에서 함께 처리된다. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: Variable Length Data를 읽는다. 만약 길이가 최대 Piece길이 보다
 *              작고 읽어야 할 Piece가 하나의 페이지에 존재한다면 읽어야 할 위치의
 *              첫번째 바이트가 위치한 메모리 포인터를 리턴한다. 그렇지 않을 경우 Row의
 *              값을 복사해서 넘겨준다.
 *
 * aRow         - [IN] 읽어들일 컴럼을 가지고 있는 Row Pointer
 * aColumn      - [IN] 읽어들일 컬럼의 정보
 * aLength      - [IN-OUT] IN : IsReturnLength == ID_FALSE
 *                         OUT: isReturnLength == ID_TRUE
 * aBuffer      - [IN] value가 복사됨.
 *
 ***********************************************************************/
SChar* svcRecord::getVarRow( const void      * aRow,
                             const smiColumn * aColumn,
                             UInt              /* aFstPos */,
                             UInt            * aLength,
                             SChar           * /* aBuffer */,
                             idBool            /* aIsReturnLength*/ )
{
    SChar           * sRet              = NULL;
    UShort          * sCurrOffsetPtr    = NULL;
    smVCPieceHeader * sVCPieceHeader    = NULL;
    UShort            sOffsetIdx        = 0;

    IDE_ASSERT( aRow    != NULL );
    IDE_ASSERT( aColumn != NULL );
    IDE_ASSERT_MSG( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE,
                    "flag : %"ID_UINT32_FMT"\n",
                    aColumn->flag );
    IDE_ASSERT_MSG( ( aColumn->flag & SMI_COLUMN_STORAGE_MASK )
                    == SMI_COLUMN_STORAGE_MEMORY,
                    "flag : %"ID_UINT32_FMT"\n",
                    aColumn->flag );


    if ( ((smpSlotHeader*)aRow)->mVarOID == SM_NULL_OID )
    {
        *aLength    = 0; 
        sRet        = NULL;
    }
    else
    {
        IDE_ASSERT( getVCPieceHeader( aRow, aColumn, &sVCPieceHeader, &sOffsetIdx ) == IDE_SUCCESS );

        if ( sVCPieceHeader == NULL ) 
        {
            /* training null */
            *aLength    = 0; 
            sRet        = NULL;
        }
        else
        {
            IDE_DASSERT( sOffsetIdx != ID_USHORT_MAX );
            IDE_DASSERT( sOffsetIdx < sVCPieceHeader->colCount );

            /* +1 은 헤더 사이즈를 건너뛰고 offset array 를 탐색하기 위함이다. */
            sCurrOffsetPtr = ((UShort*)(sVCPieceHeader + 1) + sOffsetIdx);

            IDE_DASSERT( *(sCurrOffsetPtr + 1) >= *sCurrOffsetPtr );

            /* next offset 과의 차가 길이가 된다. */
            *aLength    = ( *( sCurrOffsetPtr + 1 )) - ( *sCurrOffsetPtr );

            if ( *aLength == 0 )
            {
                /* 중간에 낀 NULL value는 offset 차이가 0 이다 */
                sRet = NULL;
            }
            else
            {
                sRet = (SChar*)sVCPieceHeader + (*sCurrOffsetPtr);
            }
        }
    }

    return sRet;
}

/***********************************************************************
 * description : aRowPtr에서 aColumn이 가리키는 value의 length를 구한다.
 *
 * aRowPtr  - [in] fixed row pointer
 * aColumn  - [in] column desc
 ***********************************************************************/
UInt svcRecord::getVarColumnLen( const smiColumn    * aColumn,
                                 const SChar        * aRowPtr )
                                 
{
    smVCPieceHeader * sVCPieceHeader = NULL;
    UShort          * sCurrOffsetPtr = NULL;
    UShort            sLength        = 0;
    UShort            sOffsetIdx     = ID_USHORT_MAX;

    IDE_DASSERT( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                 == SMI_COLUMN_TYPE_VARIABLE);

    if ( ((smpSlotHeader*)aRowPtr)->mVarOID == SM_NULL_OID )
    {
        sLength = 0;
    }
    else
    {
        IDE_ASSERT( getVCPieceHeader( aRowPtr, aColumn, &sVCPieceHeader, &sOffsetIdx ) == IDE_SUCCESS );

        if ( sVCPieceHeader == NULL )
        {
            sLength = 0;
        }
        else
        {
            IDE_DASSERT( sOffsetIdx != ID_USHORT_MAX );
            IDE_DASSERT( sOffsetIdx < sVCPieceHeader->colCount );

            /* +1 은 헤더 사이즈를 건너뛰고 offset array 를 탐색하기 위함이다. */
            sCurrOffsetPtr = ((UShort*)(sVCPieceHeader + 1) + sOffsetIdx);

            IDE_DASSERT( *(sCurrOffsetPtr + 1) >= *sCurrOffsetPtr );

            sCurrOffsetPtr = (UShort*)(sVCPieceHeader + 1) + sOffsetIdx; /* +1 headersize 건너뛰기*/

            sLength = ( *( sCurrOffsetPtr + 1 )) - ( *sCurrOffsetPtr ); /* +1 은 next offset */
        }
    }

    return sLength;
}

/***********************************************************************
 * Description : aRowPtr이 가리키는 Row에서 aColumn에 해당하는
 *               Column의 길이는 리턴한다.
 *      
 * aRowPtr - [IN] Row Pointer
 * aColumn - [IN] Column정보.
 ***********************************************************************/
UInt svcRecord::getColumnLen( const smiColumn * aColumn,
                              SChar           * aRowPtr )
{
    smVCDesc            * sVCDesc   = NULL;
    UInt                  sLength   = 0;
    
    switch ( (aColumn->flag & SMI_COLUMN_TYPE_MASK) )
    {
        case SMI_COLUMN_TYPE_LOB:
                sVCDesc = (smVCDesc*)getColumnPtr(aColumn, aRowPtr);
                sLength = sVCDesc->length;
                break;

        case SMI_COLUMN_TYPE_VARIABLE:
            sLength = getVarColumnLen( aColumn, (const SChar*)aRowPtr );
            break;

        case SMI_COLUMN_TYPE_FIXED:
            sLength = aColumn->size;
            break;

        case SMI_COLUMN_TYPE_VARIABLE_LARGE:
        default:
            IDE_ASSERT(0);
            break;
    }

    return sLength;
}
