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
 * $Id: smcRecord.cpp 91512 2021-08-21 07:50:50Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idv.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smp.h>
#include <smc.h>
#include <smcReq.h>
#include <smcLob.h>
#include <sgmManager.h>
#include <smx.h>

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
 * aAddOIDFlag     - [IN] 1. SM_INSERT_ADD_OID_OK : OID List에 Insert한 작업기록.
 *                        2. SM_INSERT_ADD_OID_NO : OID List에 Insert한 작업을 기록하지 않는다.

 *  TASK-4690, BUG-32319 [sm-mem-collection] The number of MMDB update log can
 *                       be reduced to 1.
 *  SMR_SMC_PERS_INSERT_ROW 로그와 SMR_SMC_PERS_UPDATE_VERSION_ROW 로그는
 *  서로 동일한 redo 함수를 사용한다.
 *  BUG-32319에서 SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION 로그와
 *  SMR_SMC_PERS_UPDATE_VERSION 로그가 하나로 합쳐졌기 때문에
 *  SMR_SMC_PERS_INSERT_ROW의 로그 포맷을 SMR_SMC_PERS_UPDATE_VERSION_ROW 로그의
 *  포맷과 동일하게 맞춰주기 위해 OldVersion RowOID와 NextOID를 추가하였다.
 *
 *  log structure:
 *   <update_log, NewOid, FixedRow_size, Fixed_Row, var_col1, var_col2...,
 *    OldVersion RowOID(NULL_OID), OldVersion NextOID(NULL_OID)>
 *  Logging시 Variable Column과 Fixed Row의 로그를 Replication을 위해 하나로
 *  구성한다.
 ***********************************************************************/
IDE_RC smcRecord::insertVersion( idvSQL*          /*aStatistics*/,
                                 void*            aTrans,
                                 void*            aTableInfoPtr,
                                 smcTableHeader*  aHeader,
                                 smSCN            aInfinite,
                                 SChar**          aRow,
                                 scGRID*          aRetInsertSlotGRID,
                                 const smiValue*  aValueList,
                                 UInt             aAddOIDFlag )
{
    const smiColumn   * sCurColumn    = NULL;
    SChar             * sNewFixRowPtr = NULL;
    // BUG-43117 : const smiValue* -> smiValue* (코드 프리즈 후 새로 버그 등록)
    smiValue          * sCurValue     = NULL;
    UInt                i;
    smOID               sFixOid;
    smOID               sFstVCPieceOID;
    scPageID            sPageID                 = SM_NULL_PID;
    UInt                sAfterImageSize         = 0;
    UInt                sColumnCount            = 0;
    SInt                sUnitedVarColumnCount   = 0;
    UInt                sLargeVarCount          = 0;
    smpPersPageHeader * sPagePtr                = NULL;
    const smiColumn   * sUnitedVarColumns[SMI_COLUMN_ID_MAXIMUM];
    const smiColumn   * sLargeVarColumns[SMI_COLUMN_ID_MAXIMUM];
    smiValue            sUnitedVarValues[SMI_COLUMN_ID_MAXIMUM];
    smcLobDesc        * sCurLobDesc     = NULL;
    idBool              sIsAddOID = ID_FALSE;

    // BUG-47366 UnitedVar의 Alloc Log 제거
    smOID               sVCPieceOID;
    SChar             * sVCPiecePtr    = NULL;
    smVCPieceHeader   * sVCPieceHeader = NULL;

    IDE_ERROR( aTrans     != NULL );
    IDE_ERROR( aHeader    != NULL );
    IDE_ERROR( aValueList != NULL );

    IDU_FIT_POINT( "1.PROJ-2118@smcRecord::insertVersion" );

    /* Log를 기록하기 위한 Buffer를 초기화한다. 이 Buffer는 Transaction마다
       하나씩 존재한다.*/
    smLayerCallback::initLogBuffer( aTrans );

    /* insert하기 위하여 새로운 Slot 할당 받음.*/
    IDE_TEST( smpFixedPageList::allocSlot( aTrans,
                                           aHeader->mSpaceID,
                                           aTableInfoPtr,
                                           aHeader->mSelfOID,
                                           &(aHeader->mFixed.mMRDB),
                                           &sNewFixRowPtr,
                                           aInfinite,
                                           aHeader->mMaxRow,
                                           SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT)
              != IDE_SUCCESS );

    sPageID = SMP_SLOT_GET_PID(sNewFixRowPtr);
    sFixOid = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sNewFixRowPtr ) );

    /* BUG-32091 [sm_collection] add TableOID in PageHeader
     * Page에 기록된 TableOID와 위로부터 내려온 TableOID를 비교하여 검증함 */
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

    /* TASK-4690, BUG-32319 [sm-mem-collection] The number of MMDB update log
     *                      can be reduced to 1.
     *
     * SMR_SMC_PERS_UPDATE_VERSION_ROW 로그와 포맷을 맞추어야 한다.
     * sAfterImageSize = ID_SIZEOF(UShort)
     *                   + fixed_slot_size
     *                   + variable_slot_size
     *                   + ID_SIZEOF(smOID) <--- OldVersion RowOID(NULL_OID) */
    sAfterImageSize = aHeader->mFixed.mMRDB.mSlotSize - SMP_SLOT_HEADER_SIZE;
    sAfterImageSize += ID_SIZEOF(UShort);
    sAfterImageSize += ID_SIZEOF(ULong);

    /* --------------------------------------------------------
     * [4] Row의 fixed 영역에 대한 로깅 및
     *     variable column에 대한 실제 데이타 삽입
     *     (로그레코드를 구성하는 작업이며
     *      실제 데이타레코드의 작업은 마지막 부분에서 수행함)
     * -------------------------------------------------------- */
    sColumnCount    = aHeader->mColumnCount;

    // BUG-43117 : 코드 프리즈 후 새로 버그 등록
    sCurValue       = (smiValue *)aValueList;

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

        IDU_FIT_POINT( "3.PROJ-2118@smcRecord::insertVersion" );

        if( ( sCurColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sCurColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
                    sCurLobDesc = (smcLobDesc*)(sNewFixRowPtr + sCurColumn->offset);

                    // prepare에서 기존 LobDesc를 old version으로 인식하기 때문에 초기화
                    SMC_LOB_DESC_INIT( sCurLobDesc );

                    if(sCurValue->length > 0)
                    {
                        // SQL로 직접 LOB 데이타를 넣은 경우
                        IDE_TEST( smcLob::reserveSpaceInternal(
                                              aTrans,
                                              aHeader,
                                              (smiColumn*)sCurColumn,
                                              sCurLobDesc->mLobVersion,
                                              sCurLobDesc,
                                              0,                /* aOffset */
                                              sCurValue->length,
                                              aAddOIDFlag,
                                              ID_TRUE )         /* aIsFullWrite */
                                  != IDE_SUCCESS );

                        IDE_TEST( smcLob::writeInternal(
                                      aTrans,
                                      aHeader,
                                      (UChar*)sNewFixRowPtr,
                                      (smiColumn*)sCurColumn,
                                      0,                     /* aOffset */
                                      sCurValue->length,
                                      (UChar*)sCurValue->value,
                                      ID_FALSE,              /* aIsWriteLog */
                                      ID_FALSE,              /* aIsReplSenderSend */
                                      (smLobLocator)NULL )   /* aLobLocator */
                                  != IDE_SUCCESS );

                        if(sCurLobDesc->flag == SM_VCDESC_MODE_OUT)
                        {
                            sAfterImageSize += getVCAMLogSize(sCurValue->length);
                            sLargeVarColumns[sLargeVarCount] = sCurColumn;
                            sLargeVarCount++;
                        }
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

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    /* Geometry Type 또는 VARIABLE_LARGE 타입을 지정한 경우
                     * UnitedVar가 아닌 PROJ-2419 적용 전 Variable 구조를 사용한다. */
                    sFstVCPieceOID = SM_NULL_OID;
                    
                    IDE_TEST( smcRecord::insertLargeVarColumn ( aTrans,
                                                                aHeader,
                                                                sNewFixRowPtr,
                                                                sCurColumn,
                                                                sIsAddOID,
                                                                sCurValue->value,
                                                                sCurValue->length,
                                                                &sFstVCPieceOID )
                              != IDE_SUCCESS );
                    
                    if ( sFstVCPieceOID != SM_NULL_OID )
                    {
                        /* Variable Column이 Out Mode로 저장됨. */
                        sAfterImageSize += getVCAMLogSize(sCurValue->length);
                        sLargeVarColumns[sLargeVarCount] = sCurColumn;
                        sLargeVarCount++;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;

                case SMI_COLUMN_TYPE_FIXED:
                    IDU_FIT_POINT( "2.PROJ-2118@smcRecord::insertVersion" );

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
        }
        else
        {
            // PROJ-2264
            idlOS::memcpy(sNewFixRowPtr + sCurColumn->offset,
                          sCurValue->value,
                          sCurValue->length);
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
                                           sUnitedVarColumnCount,
                                           &sAfterImageSize )
                != IDE_SUCCESS );
    }
    else
    {
        /* United Var 가 없어도 NULL OID 는 로깅한다 */
        sAfterImageSize += ID_SIZEOF(smOID);
    }

    //PROJ-2419 insert UnitedVarColumn FitTest: before log write
    IDU_FIT_POINT( "1.PROJ-2419@smcRecord::insertVersion::before_log_write" );   
 
    /* Insert에 대한 Log를 기록한다. */
    IDE_TEST( smcRecordUpdate::writeInsertLog( aTrans,
                                               aHeader,
                                               sNewFixRowPtr,
                                               sAfterImageSize,
                                               sUnitedVarColumnCount,
                                               sUnitedVarColumns,
                                               sLargeVarCount,
                                               sLargeVarColumns )
              != IDE_SUCCESS );

    /* BUG-14513: Fixed Slot을 할당시 Alloc Slot Log를 기록하지
       않도록 수정. insert log가 Alloc Slot에 대한 Redo, Undo수행.
       이 때문에 insert log기록 후 Slot header에 대한 Update수행
    */
    smpFixedPageList::setAllocatedSlot( aInfinite, sNewFixRowPtr );

    // BUG-47366 UnitedVar의 Alloc Log 제거
    // insert UnitedVar 에서 발생한 alloc Slot들은 Slot Header가 초기화 안되있음
    // 초기화 해주어야 한다.
    if ( sUnitedVarColumnCount > 0 )
    {
        sVCPieceOID = ((smpSlotHeader*)sNewFixRowPtr)->mVarOID;
        while ( sVCPieceOID != SM_NULL_OID )
        {
            IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                               sVCPieceOID,
                                               (void**)&sVCPiecePtr )
                        == IDE_SUCCESS );
            sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
            
            sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
            sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
            sVCPieceHeader->flag &= ~SM_VCPIECE_TYPE_MASK;
            sVCPieceHeader->flag |= SM_VCPIECE_TYPE_OTHER;

            IDE_TEST( smmDirtyPageMgr::insDirtyPage( aHeader->mSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                      != IDE_SUCCESS);

            sVCPieceOID = sVCPieceHeader->nxtPieceOID;
        }
    }

    if (aTableInfoPtr != NULL)
    {
        /* Record가 새로 추가되었으므로 Record Count를 증가시켜준다.*/
        smLayerCallback::incRecCntOfTableInfo( aTableInfoPtr );
    }

    
    //PROJ-2419 insert UnitedVarColumn FitTest: after log write
    IDU_FIT_POINT( "2.PROJ-2419@smcRecord::insertVersion::after_log_write" );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID, sPageID)
             != IDE_SUCCESS);

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

    IDE_PUSH();

    IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                             sPageID) == IDE_SUCCESS);

    IDE_POP();

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
IDE_RC smcRecord::isUpdatedVersionBySameStmt( void            * aTrans,
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
    IDE_TEST( smmManager::holdPageXLatch( aHeader->mSpaceID, sPageID )
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
    IDE_TEST( smmManager::releasePageLatch(aHeader->mSpaceID, sPageID)
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smmManager::releasePageLatch(aHeader->mSpaceID, sPageID)
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
IDE_RC smcRecord::checkUpdatedVersionBySameStmt( void             * aTrans,
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
            // BUGBUG : lock row 와 동시에 다른 stmt가 들어오나요??
            IDE_DASSERT( 0 );

            /* aRow에 대한 Lock을 aTrans가 이미 잡았다.*/
            /* BUG-39399, BUG-39507
             * 1. aInfiniteSCN과 sNxtRowSCN의 InfiniteSCN 이 같다면,
             *    같은 statement에서 update 한 경우이다.
             * 2. sNxtRowSCN이 aInfiniteSCN + SM_SCN_INF_DELETE_BIT 이면
             *    같은 statement에서 delete 한 경우이다. */
            if( SM_SCN_IS_EQ( &sNxtRowSCN, &aInfiniteSCN ) ||
                SM_SCN_IS_EQ( &sNxtRowSCN, &sInfiniteSCNWithDeleteBit ) )
            {
                // BUGBUG : limitSCN 은 (FREE) -> (LOCK | TID) -> (INFINITE | TID) 로 변경되는데
                //          어떻게하면 LOCK 인데 INFINITE 일까요... 
                IDE_DASSERT( 0 ); 

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
 * aOldRID       - [IN] None
 * aRow          - [OUT] Update의해 만들어진 New Version의 Row Pointer를 넘겨준다.
 * aRetSlotRID   - [IN] None
 * aColumnList   - [IN] Update될 Column List
 * aValueList    - [IN] Update될 Column들의 새로운 Vaule List
 * aRetryInfo    - [IN] Retry를 위해 점검할 column의 list
 * aInfinite     - [IN] New Version의 Row SCN
 * aModifyIdxBit - [IN] None
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC smcRecord::updateVersion( idvSQL               * /* aStatistics */,
                                 void                 * aTrans,
                                 smSCN                  aViewSCN,
                                 void                 * /*aTableInfoPtr*/,
                                 smcTableHeader       * aHeader,
                                 SChar                * aOldRow,
                                 scGRID                 /* aOldGRID */,
                                 SChar               ** aRow,
                                 scGRID               * aRetSlotGRID,
                                 const smiColumnList  * aColumnList,
                                 const smiValue       * aValueList,
                                 const smiDMLRetryInfo* aRetryInfo,
                                 smSCN                  aInfinite,
                                 void                 * /*aLobInfo4Update*/,
                                 ULong                  /*aModifyIdxBit */,
                                 idBool                 aForbiddenToRetry )  
{
    return updateVersionInternal( aTrans,
                                  aViewSCN,
                                  aHeader,
                                  aOldRow,
                                  aRow,
                                  aRetSlotGRID,
                                  aColumnList,
                                  aValueList,
                                  aRetryInfo,
                                  aInfinite,
                                  SMC_UPDATE_BY_TABLECURSOR,
                                  aForbiddenToRetry );
}

/***********************************************************************
 * Description : aOldRow를 Update한다. Update시 MMDB의 MVCC에 의해서
 *               새로운 Version을 만들어 낸다. Update가 끝나게 되면
 *               aOldRow는 새로운 Version을 가리키게 된다.
 *
 * aTrans        - [IN] Transaction Pointer
 * aViewSCN      - [IN] 이 연산을 수행하는 Cursor의 ViewSCN
 * aHeader       - [IN] aOldRow가 속해 있는 Table Header Pointer
 * aOldRow       - [IN] Update할 Row.
 * aRow          - [OUT] Update의해 만들어진 New Version의 Row Pointer를 넘겨준다.
 * aRetSlotGRID  - [IN] None
 * aColumnList   - [IN] Update될 Column List
 * aValueList    - [IN] Update될 Column들의 새로운 Vaule List
 * aRetryInfo    - [IN] Retry를 위해 점검할 column의 list
 * aInfinite     - [IN] New Version의 Row SCN
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC smcRecord::updateVersionInternal( void                 * aTrans,
                                         smSCN                  aViewSCN,
                                         smcTableHeader       * aHeader,
                                         SChar                * aOldRow,
                                         SChar               ** aRow,
                                         scGRID               * aRetSlotGRID,
                                         const smiColumnList  * aColumnList,
                                         const smiValue       * aValueList,
                                         const smiDMLRetryInfo* aRetryInfo,
                                         smSCN                  aInfinite,
                                         smcUpdateOpt           aOpt,
                                         idBool                 aForbiddenToRetry )
{
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;
    const smiValue        * sCurValue;
    SChar                 * sOldFixRowPtr = aOldRow;
    SChar                 * sNewFixRowPtr = NULL;
    smOID                   sOldFixRowOID;
    smOID                   sNewFixRowOID;
    scPageID                sOldPageID = SC_NULL_PID;
    scPageID                sNewPageID = SC_NULL_PID;
    UInt                    sAftImageSize;
    UInt                    sBfrImageSize;
    UInt                    sState = 0;
    UInt                    sColumnSeq;
    smpPersPageHeader     * sPagePtr;

    vULong                  sFixedRowSize;
    smpSlotHeader         * sOldSlotHeader = NULL;

    smOID                   sFstVCPieceOID;
    idBool                  sIsReplSenderSend;
    idBool                  sIsLockRow      = ID_FALSE;
    smcLobDesc            * sCurLobDesc;
    smTID                   sTransID;

    UInt                    sUnitedVarColumnCount   = 0;
    UInt                    sLogVarCount            = 0;
    const smiColumn       * sUnitedVarColumns[SMI_COLUMN_ID_MAXIMUM];
    smiValue                sUnitedVarValues[SMI_COLUMN_ID_MAXIMUM];

    // BUG-47366 UnitedVar의 Alloc Log 제거
    smOID                   sVCPieceOID;
    SChar                 * sVCPiecePtr    = NULL;
    smVCPieceHeader       * sVCPieceHeader = NULL;

    idBool                  sIsLockedNext = ID_FALSE;

    IDE_ERROR( aTrans  != NULL );
    IDE_ERROR( aOldRow != NULL );
    IDE_ERROR( aHeader != NULL );

    IDU_FIT_POINT( "1.PROJ-2118@smcRecord::updateVersionInternal" );

    /* BUGBUG: By Newdaily
       aColumnList와 aValueList가 NULL인 경우가 존재함.
       이에 대한 검증이 필요합니다.

       추가: By Clee
       PRJ-1362 LOB에서 LOB update시에 fixed-row에 대한 versioning으로
       NULL로 들어와서 전체 컬럼을 모두 복사함.

       IDE_DASSERT( aColumnList != NULL );
       IDE_DASSERT( aValueList != NULL );
    */

    /* !!Fixed Row는 Update시 무조건 New Version이 만들어진다 */

    sOldFixRowPtr = aOldRow;

    /* Transaction Log Buffer를 초기화한다. */
    smLayerCallback::initLogBuffer( aTrans );
    sOldSlotHeader = (smpSlotHeader*)sOldFixRowPtr;
    sOldPageID     = SMP_SLOT_GET_PID(aOldRow);
    sOldFixRowOID  = SM_MAKE_OID( sOldPageID,
                                  SMP_SLOT_GET_OFFSET( sOldSlotHeader ) );

    sIsReplSenderSend =  smcTable::needReplicate( aHeader, aTrans );

    if( aColumnList == NULL )
    {
        /* BUG-15718: Replication Sender가 log분석중 서버 사망:
           Sender가 읽어서 보내야할 로그증 Update log이고  Before Image Size가 0이면
           서버가 죽도록하였다. 때문에 이렇게 Column에 대한 Update없이 단지
           New Version을 만드는 Update로그는 Sender가 Skip하도록 한다. */
        sIsReplSenderSend = ID_FALSE;
    }

    IDU_FIT_POINT( "1.BUG-42154@smcRecord::updateVersionInternal::beforelock" );

    /* update하기 위하여 old version이 속한 page에 대하여
       holdPageXLatch  */
    IDE_TEST( smmManager::holdPageXLatch( aHeader->mSpaceID, sOldPageID )
              != IDE_SUCCESS );
    sState = 1;

    /* sOldFixRowPtr를 다른 Transaction이 이미 Update했는지 조사.*/
    IDE_TEST( recordLockValidation( aTrans,
                                    aViewSCN,
                                    aHeader,
                                    &sOldFixRowPtr,
                                    ID_ULONG_MAX,
                                    &sState,
                                    aRetryInfo,
                                    aForbiddenToRetry )
              != IDE_SUCCESS );

    // PROJ-1784 DML without retry
    // sOldFixRowPtr이 변경 될 수 있으므로 값을 다시 설정해야 함
    // ( 이경우 page latch 도 다시 잡았음 )
    sOldSlotHeader = (smpSlotHeader*)sOldFixRowPtr;
    sOldPageID     = SMP_SLOT_GET_PID( sOldFixRowPtr );
    sOldFixRowOID  = SM_MAKE_OID( sOldPageID,
                                  SMP_SLOT_GET_OFFSET( sOldSlotHeader ) );

    /* BUG-33738 [SM] undo of SMC_PERS_UPDATE_VERSION_ROW log is wrong
     * 이미 lock row 된 경우 update에 대한 rollback시 lock bit를
     * 설정해주어야 한다.
     * 따라서 이미 lock bit가 설정된 경우인지 확인한다. */
    if ( SMP_SLOT_IS_LOCK_TRUE(sOldSlotHeader) )
    {
        sIsLockRow = ID_TRUE;
    }
    else
    {
        sIsLockRow = ID_FALSE;

        /* sSlotHeader == aOldRow == sOldFixRowPtr
         * TASK-4690, BUG-32319 [sm-mem-collection] The number of MMDB update log
         *                      can be reduced to 1.
         * RecordLock을 위해 mNext에 Lock을 임시로 설정한다.
         * update 로깅 후 mNext를 NewVersion RowOID로 설정한다. */
        sTransID = smLayerCallback::getTransID( aTrans );
        SMP_SLOT_SET_LOCK( sOldSlotHeader, sTransID );
        sIsLockedNext = ID_TRUE;
    }

    sState = 0;
    IDE_TEST( smmManager::releasePageLatch(aHeader->mSpaceID, sOldPageID)
              != IDE_SUCCESS );

    IDU_FIT_POINT( "2.BUG-42154@smcRecord::updateVersionInternal::afterlock" );

    /* 위 연산이 끝나게 되면 Row에 Lock잡히게 된다. 왜냐면 Next Version에
       Lock을 잡았기 때문이다. 따라서 다른 Transaction들은 해당 레코드에
       접근하게 되면 Wait된다. 그래서 위에서 Page Latch 또한 풀었다.*/

    /* New Version을 할당한다. */
    IDE_TEST( smpFixedPageList::allocSlot(aTrans,
                                          aHeader->mSpaceID,
                                          NULL,
                                          aHeader->mSelfOID,
                                          &(aHeader->mFixed.mMRDB),
                                          &sNewFixRowPtr,
                                          aInfinite,
                                          aHeader->mMaxRow,
                                          SMP_ALLOC_FIXEDSLOT_NONE)
              != IDE_SUCCESS );

    sNewPageID = SMP_SLOT_GET_PID(sNewFixRowPtr);
    sNewFixRowOID = SM_MAKE_OID( sNewPageID,
                         SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sNewFixRowPtr) );

    /* Tx OID List에 Old Version 추가. */
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       aHeader->mSelfOID,
                                       sOldFixRowOID,
                                       aHeader->mSpaceID,
                                       SM_OID_OLD_UPDATE_FIXED_SLOT )
              != IDE_SUCCESS );

    if(aOpt == SMC_UPDATE_BY_TABLECURSOR)
    {
        /* Tx OID List에 New Version 추가. */
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           sNewFixRowOID,
                                           aHeader->mSpaceID,
                                           SM_OID_NEW_UPDATE_FIXED_SLOT )
                  != IDE_SUCCESS );
    }
    else
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
                                           sNewFixRowOID,
                                           aHeader->mSpaceID,
                                           (SM_OID_NEW_UPDATE_FIXED_SLOT & ~SM_OID_ACT_CURSOR_INDEX) |
                                           SM_OID_ACT_AGING_INDEX )
                  != IDE_SUCCESS );
    }

    /* BUG-32091 [sm_collection] add TableOID in PageHeader
     * Page에 기록된 TableOID와 위로부터 내려온 TableOID를 비교하여 검증함 */
    IDE_ASSERT( smmManager::getPersPagePtr( aHeader->mSpaceID,
                                            sOldPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    IDE_ASSERT( sPagePtr->mTableOID == aHeader->mSelfOID );

    /* 위 연산이 끝나게 되면 Row에 Lock잡히게 된다. 왜냐면 Next Version에
       끝나지 않은 Transaction이 만든 New Version이 있게 되면 다른 Transaction들은
       Wait하게 되기때문이다. 그래서 위에서 Page Latch 또한 풀었다.*/
    sState = 2;

    /* 새로운 버젼에 기존 버젼의 값 중 변경되지 않는 값을 copy.*/
    sFixedRowSize = aHeader->mFixed.mMRDB.mSlotSize - SMP_SLOT_HEADER_SIZE;

    idlOS::memcpy( sNewFixRowPtr + SMP_SLOT_HEADER_SIZE,
                   sOldFixRowPtr + SMP_SLOT_HEADER_SIZE,
                   sFixedRowSize);

    /* TASK-4690, BUG-32319 [sm-mem-collection] The number of MMDB update log
     *                      can be reduced to 1.
     * Before Image에 OldVersion RowOID(ULong)와 lock row(idBool) 여부가 기록된다
     * After Image에 OldVersion RowOID가 기록된다. */
    sBfrImageSize = ID_SIZEOF(ULong) + ID_SIZEOF(idBool);
    sAftImageSize = sFixedRowSize + ID_SIZEOF(UShort)/*Fixed Row Log Size*/ +
                    ID_SIZEOF(ULong);

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

        IDU_FIT_POINT( "3.PROJ-2118@smcRecord::updateVersionInternal" );

        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:

                    sCurLobDesc = (smcLobDesc*)(sNewFixRowPtr + sColumn->offset);

                    // SQL로 직접 LOB 데이타를 넣은 경우
                    IDE_TEST( smcLob::reserveSpaceInternal(
                                  aTrans,
                                  aHeader,
                                  (smiColumn*)sColumn,
                                  0, /* aLobVersion */
                                  sCurLobDesc,
                                  0, /* aOffset */
                                  sCurValue->length,
                                  SM_FLAG_INSERT_LOB,
                                  ID_TRUE /* aIsFullWrite */)
                              != IDE_SUCCESS );

                    if( sCurValue->length > 0 )
                    {
                        IDE_TEST( smcLob::writeInternal(
                                      aTrans,
                                      aHeader,
                                      (UChar*)sNewFixRowPtr,
                                      (smiColumn*)sColumn,
                                      0, /* aOffset */
                                      sCurValue->length,
                                      (UChar*)sCurValue->value,
                                      ID_FALSE,  // aIsWriteLog
                                      ID_FALSE,  // aIsReplSenderSend
                                      (smLobLocator)NULL )     // aLobLocator
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Here length is always 0. */

                        if( sCurValue->value  == NULL )
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

                    if( sIsReplSenderSend == ID_TRUE )
                    {
                        /* Dumm Lob VC Log: Column ID(UInt) | Length(UInt) */
                        sBfrImageSize += (ID_SIZEOF(UInt) * 2);
                    }

                    if(sCurLobDesc->flag == SM_VCDESC_MODE_OUT)
                    {
                        sAftImageSize += getVCAMLogSize( sCurValue->length );
                    }

                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                    sUnitedVarColumns[sUnitedVarColumnCount]       = sColumn;
                    sUnitedVarValues[sUnitedVarColumnCount].length = sCurValue->length;
                    sUnitedVarValues[sUnitedVarColumnCount].value  = sCurValue->value;
                    sUnitedVarColumnCount++;

                    if ( sIsReplSenderSend == ID_TRUE )
                    {
                        /* Before VC Image Header: Column ID(UInt) | Length(UInt)
                           Body: Value */
                        sBfrImageSize += getVCBMLogSize( getColumnLen( sColumn,
                                                                       sOldFixRowPtr ) );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    /* Geometry Type 또는 VARIABLE_LARGE 타입을 지정한 경우
                     * UnitedVar가 아닌 PROJ-2419 적용 전 Variable 구조를 사용한다. */
                    IDE_TEST( deleteVC( aTrans, 
                                        aHeader, 
                                        sColumn, 
                                        sOldFixRowPtr )
                              != IDE_SUCCESS );

                    sFstVCPieceOID = SM_NULL_OID;

                    IDE_TEST( smcRecord::insertLargeVarColumn ( aTrans,
                                                                aHeader,
                                                                sNewFixRowPtr,
                                                                sColumn,
                                                                ID_TRUE /*Add OID*/,
                                                                sCurValue->value,
                                                                sCurValue->length,
                                                                &sFstVCPieceOID )
                              != IDE_SUCCESS );

                    if ( sFstVCPieceOID != SM_NULL_OID )
                    {
                        sAftImageSize += getVCAMLogSize( sCurValue->length );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    if ( sIsReplSenderSend == ID_TRUE )
                    {
                        /* Before VC Image Header: Column ID(UInt) | Length(UInt)
                           Body: Value */
                        sBfrImageSize += getVCBMLogSize( getColumnLen( sColumn,
                                                                       sOldFixRowPtr ) );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;

                case SMI_COLUMN_TYPE_FIXED:

                    IDU_FIT_POINT( "2.PROJ-2118@smcRecord::updateVersionInternal" );

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

                    if ( sIsReplSenderSend == ID_TRUE )
                    {
                        sBfrImageSize += getFCMVLogSize( sColumn->size );
                    }

                    break;

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
        else // SMI_COLUMN_COMPRESSION_TRUE
        {
            // PROJ-2264
            idlOS::memcpy( sNewFixRowPtr + sColumn->offset,
                           sCurValue->value,
                           sCurValue->length );

            if ( sIsReplSenderSend == ID_TRUE )
            {
                sBfrImageSize += getFCMVLogSize( ID_SIZEOF(smOID) );
            }
            else
            {
                /* Nothing to do */
            }
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
                                               sUnitedVarColumnCount,
                                               &sAftImageSize )
                    != IDE_SUCCESS );

            sLogVarCount = sUnitedVarColumnCount;

        }
        else
        {
            IDE_TEST( smcRecord::updateUnitedVarColumns( aTrans,
                                                         aHeader,
                                                         sOldFixRowPtr,
                                                         sNewFixRowPtr,
                                                         sUnitedVarColumns,
                                                         sUnitedVarValues,
                                                         sUnitedVarColumnCount,
                                                         &sLogVarCount,
                                                         &sAftImageSize)
                != IDE_SUCCESS );
        }
        /* BUG-47366 Before Image Log에 Old UnitedVar OID가 들어가야 한다.
         * BUG-47723 OID Cnt 위치 변경 */
        sBfrImageSize += ID_SIZEOF(smOID) ;

        sVCPieceOID = ((smpSlotHeader*)sOldFixRowPtr)->mVarOID;

        if ( sVCPieceOID != SM_NULL_OID )
        {
            /* BUG-47366 총 몇개의 OID가 들어가는지 계산한다.
             * BUG-47723 OID Cnt 위치 변경 */
            sBfrImageSize += ID_SIZEOF(UShort);
 
            do
            {
                sBfrImageSize += ID_SIZEOF(smOID);

                IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );
                sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                sVCPieceOID = sVCPieceHeader->nxtPieceOID;
            }
            while ( sVCPieceOID != SM_NULL_OID );
        }
    }
    else /* united var col update가 발생하지 않은 경우, 두가지로 다시 갈라진다.  */
    {
        sFstVCPieceOID = ((smpSlotHeader*)sOldFixRowPtr)->mVarOID;
        ((smpSlotHeader*)sNewFixRowPtr)->mVarOID = sFstVCPieceOID;

        if ( sFstVCPieceOID == SM_NULL_OID )    /* 아예 united var col이 없다*/
        {
            sAftImageSize += ID_SIZEOF(smOID); /* NULL OID logging */

            /* BUG-47366 Before Image Log에 UnitedVar OID가 들어가야 한다. */
            sBfrImageSize += ID_SIZEOF(smOID);
        }
        else                                    /* united var col 이 있으나 이 update에 해당되지 않았다. */
        {                                       /* redo 시에 구분하여 처리하기위해 OID를 주고 count 0 을 로깅한다 */
            sAftImageSize += ID_SIZEOF(smOID) + ID_SIZEOF(UShort); /* OID + zero Count */
            /* BUG-47366 Before Image Log에 UnitedVar OID가 들어가야 한다. */
            sBfrImageSize += ID_SIZEOF(smOID) + ID_SIZEOF(UShort);
        }
    }

    //PROJ-2419 update UnitedVarColumn FitTest: before log write
    IDU_FIT_POINT( "1.PROJ-2419@smcRecord::updateVersion::before_log_write" );

    IDE_TEST( smcRecordUpdate::writeUpdateVersionLog(
                  aTrans,
                  aHeader,
                  aColumnList,
                  sIsReplSenderSend,
                  sOldFixRowOID,
                  sOldFixRowPtr,
                  sNewFixRowPtr,
                  sIsLockRow,
                  sBfrImageSize,
                  sAftImageSize,
                  sLogVarCount ) //aHeader->mUnitedVarColumnCount 로 대체하고 변수제거
              != IDE_SUCCESS );

    //PROJ-2419 update UnitedVarColumn FitTest: after log write
    IDU_FIT_POINT( "2.PROJ-2419@smcRecord::updateVersion::after_log_write" );

    IDL_MEM_BARRIER;

    /* sOldFixRowPtr의 Next Version을 New Version으로 Setting.*/
    SMP_SLOT_SET_NEXT_OID( sOldSlotHeader, sNewFixRowOID );
    SM_SET_SCN( &(sOldSlotHeader->mLimitSCN), &aInfinite );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID, sOldPageID)
              != IDE_SUCCESS );

    /* BUG-14513: Fixed Slot을 할당시 Alloc Slot Log를 기록하지
       않도록 수정. Update log가 Alloc Slot에 대한 Redo, Undo수행.
       이 때문에 Update log기록 후 Slot header에 대한 Update수행
    */
    smpFixedPageList::setAllocatedSlot( aInfinite, sNewFixRowPtr );

    // BUG-47366 UnitedVar의 Alloc Log 제거
    // insert UnitedVar 에서 발생한 alloc Slot들은 Slot Header가 초기화 안되있음
    // 초기화 해주어야 한다.
    if ( sUnitedVarColumnCount > 0 )
    {
        sVCPieceOID = ((smpSlotHeader*)sNewFixRowPtr)->mVarOID;
        while ( sVCPieceOID != SM_NULL_OID )
        {
            IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                               sVCPieceOID,
                                               (void**)&sVCPiecePtr )
                        == IDE_SUCCESS );
            sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
            
            sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
            sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
            sVCPieceHeader->flag &= ~SM_VCPIECE_TYPE_MASK;
            sVCPieceHeader->flag |= SM_VCPIECE_TYPE_OTHER;

            IDE_TEST( smmDirtyPageMgr::insDirtyPage( aHeader->mSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                      != IDE_SUCCESS);

            sVCPieceOID = sVCPieceHeader->nxtPieceOID;
        }

        /* BUG-47366 deleteVC의 Log도 제거하기 위해
         * UpdateVersion Log가 기록된 이후에 Old Version의 UnitedVar의 deleteVC를 수행한다. */
        IDE_TEST( deleteVC( aTrans, 
                            aHeader, 
                            ((smpSlotHeader*)sOldFixRowPtr)->mVarOID ,
                            ID_FALSE ) );
    }

    *aRow = (SChar*)sNewFixRowPtr;
    if(aRetSlotGRID != NULL)
    {
        SC_MAKE_GRID( *aRetSlotGRID,
                      aHeader->mSpaceID,
                      sNewPageID,
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sNewFixRowPtr ) );
    }

    sState = 0;
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                            sNewPageID) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_column_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INVALID_COLUMN_SIZE) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부화 및
    //Aging이 밀리는 현상이 더 심화됨.
    //update시 inconsistency한 view에 의해 retry한 횟수
    switch( ideGetErrorCode() )
    {
        case smERR_RETRY_Already_Modified:
        {
            SMX_INC_SESSION_STATISTIC( aTrans,
                                       IDV_STAT_INDEX_UPDATE_RETRY_COUNT,
                                       1 /* Increase Value */ );

            smcTable::incStatAtABort( aHeader, SMC_INC_RETRY_CNT_OF_UPDATE );
        }
        break;
        case smERR_RETRY_Row_Retry :

            IDE_ASSERT( lockRowInternal( aTrans,
                                         aHeader,
                                         sOldFixRowPtr ) == IDE_SUCCESS );

            *aRow = sOldFixRowPtr;

            break;
        default:
            break;
    }

    if ( sIsLockedNext == ID_TRUE )
    {
        SMP_SLOT_SET_UNLOCK( sOldSlotHeader );
    }

    switch(sState)
    {
        case 2:
            IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                                     sNewPageID)
                       == IDE_SUCCESS);
            break;

        case 1:
            // PROJ-1784 DML without retry
            // sOldFixRowPtr가 변경 될 수 있으므로 PID를 다시 가져와야 함
            sOldPageID = SMP_SLOT_GET_PID( sOldFixRowPtr );

            IDE_ASSERT(smmManager::releasePageLatch(aHeader->mSpaceID,
                                                    sOldPageID)
                       == IDE_SUCCESS);
            IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                                     sOldPageID)
                       == IDE_SUCCESS);
            break;
        default:
            break;
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
IDE_RC smcRecord::updateUnitedVarColumns( void                * aTrans,
                                          smcTableHeader      * aHeader,
                                          SChar               * aOldRow,
                                          SChar               * aNewRow,
                                          const smiColumn    ** aColumns,
                                          smiValue            * aValues,
                                          UInt                  aVarColumnCount,
                                          UInt                * aLogVarCount,
                                          UInt                * aImageSize )
{
    const smiColumn   * sCurColumn  = NULL;
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

    IDE_ASSERT( aTrans          != NULL );
    IDE_ASSERT( aHeader         != NULL );
    IDE_ASSERT( aOldRow         != NULL );
    IDE_ASSERT( aNewRow         != NULL );
    IDE_ASSERT( aColumns        != NULL );
    IDE_ASSERT( aValues         != NULL );
    IDE_ASSERT( aLogVarCount    != NULL );
    IDE_ASSERT( aImageSize      != NULL );

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
                                      sUnitedVarColCnt,
                                      aImageSize )
            != IDE_SUCCESS );

    *aLogVarCount = sUnitedVarColCnt;

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
IDE_RC smcRecord::updateInplace(idvSQL                  * /*aStatistics*/,
                                void                    * aTrans,
                                smSCN                     /*aViewSCN*/,
                                void                    * /*aTableInfoPtr*/,
                                smcTableHeader          * aHeader,
                                SChar                   * aRowPtr,
                                scGRID                    /*aOldGRID*/,
                                SChar                  ** aRow,
                                scGRID                  * aRetSlotGRID,
                                const smiColumnList     * aColumnList,
                                const smiValue          * aValueList,
                                const smiDMLRetryInfo * /*aRetryInfo */,
                                smSCN                   /*aInfinite*/,
                                void                  * /*aLobInfo4Update*/,
                                ULong                   aModifyIdxBit,
                                idBool                  /*aForbiddenToRetry*/ )
{
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;
    const smiValue        * sCurValue;
    smOID                   sFstVCPieceOID = SM_NULL_OID;
    smVCDesc              * sCurVCDesc;
    smcLobDesc            * sCurLobDesc;
    smOID                   sFixOID;
    UInt                    sState  = 0;
    scPageID                sPageID = SM_NULL_PID ;
    ULong                   sRowBuffer[SM_PAGE_SIZE / ID_SIZEOF(ULong)];
    SChar                 * sRowPtrBuffer;
    SInt                    sStoreMode;
    UInt                    sColumnSeq;
    UInt                    sUnitedVarColumnCount = 0;
    UInt                    sDummyCount     = 0;
    UInt                    sDummySize      = 0;
    const smiColumn       * sUnitedVarColumns[SMI_COLUMN_ID_MAXIMUM];
    smiValue                sUnitedVarValues[SMI_COLUMN_ID_MAXIMUM];

    // BUG-47366 UnitedVar의 Alloc Log 제거
    smOID                   sVCPieceOID;
    SChar                 * sVCPiecePtr    = NULL;
    smVCPieceHeader       * sVCPieceHeader = NULL;

    IDE_ERROR( aTrans      != NULL );
    IDE_ERROR( aHeader     != NULL );
    IDE_ERROR( aRowPtr     != NULL );
    IDE_ERROR( aRow        != NULL );
    IDE_ERROR( aColumnList != NULL );
    IDE_ERROR( aValueList  != NULL );

    sRowPtrBuffer = (SChar*)sRowBuffer;

    smLayerCallback::initLogBuffer( aTrans );

    sPageID = SMP_SLOT_GET_PID(aRowPtr);

    /* Fixed Row의 Row ID를 가져온다. */
    sFixOID = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRowPtr ) );

    sCurColumnList = aColumnList;
    sCurValue      = aValueList;

    /* BUG-29424
       [valgrind] smcLob::writeInternal() 에서 valgrind 오류가 발생합니다.
       sRowPtrBuffer에 smpSlotHeader를 복사해야 합니다. */
    idlOS::memcpy(sRowPtrBuffer,
                  aRowPtr,
                  ID_SIZEOF(smpSlotHeader));

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

        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:

                    sCurLobDesc = (smcLobDesc*)(sRowPtrBuffer + sColumn->offset);

                    idlOS::memcpy(sCurLobDesc,
                                  aRowPtr + sColumn->offset,
                                  ID_SIZEOF(smcLobDesc));

                    if ( SM_VCDESC_IS_MODE_IN(sCurLobDesc) )
                    {
                        /* 데이타가 In-Mode로 저장되었을 경우 데이타는 Fixed영역에
                           저장되어있다. */
                        idlOS::memcpy( (SChar*)sCurLobDesc + ID_SIZEOF(smVCDescInMode),
                                       aRowPtr + sColumn->offset + ID_SIZEOF(smVCDescInMode),
                                       sCurLobDesc->length );
                    }

                    /* smcLob::prepare4WriteInternal에서 sCurLobDesc을 바꿀수 있기때문에
                       데이타를 복사해서 넘겨야 한다. 왜냐하면 Update Inplace로 하기때문에
                       데이타영역에 대한 갱신을 하기전에 로그를 먼저 기록해야하나 변경시 로그를
                       기록하지 않고 이후에 기록되기때문이다.
                    */
                    // SQL로 직접 LOB 데이타를 넣은 경우
                    IDE_TEST( smcLob::reserveSpaceInternal( aTrans,
                                                            aHeader,
                                                            (smiColumn*)sColumn,
                                                            0, /* aLobVersion */
                                                            sCurLobDesc,
                                                            0, /* aOffset */
                                                            sCurValue->length,
                                                            SM_FLAG_INSERT_LOB,
                                                            ID_TRUE /* aIsFullWrite */)
                              != IDE_SUCCESS );

                    if( sCurValue->length > 0 )
                    {
                        IDE_TEST( smcLob::writeInternal(
                                      aTrans,
                                      aHeader,
                                      (UChar*)sRowPtrBuffer,
                                      (smiColumn*)sColumn,
                                      0, /* aOffset */
                                      sCurValue->length,
                                      (UChar*)sCurValue->value,
                                      ID_FALSE,  // aIsWriteLog
                                      ID_FALSE,  // aIsReplSenderSend
                                      smLobLocator(NULL) )     // aLobLocator
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
                    sUnitedVarColumns[sUnitedVarColumnCount]       = sColumn;
                    sUnitedVarValues[sUnitedVarColumnCount].length = sCurValue->length;
                    sUnitedVarValues[sUnitedVarColumnCount].value  = sCurValue->value;
                    sUnitedVarColumnCount++;

                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    /* Geometry Type 또는 VARIABLE_LARGE 타입을 지정한 경우
                     * UnitedVar가 아닌 PROJ-2419 적용 전 Variable 구조를 사용한다. */
                    IDE_TEST( deleteVC( aTrans, 
                                        aHeader, 
                                        sColumn, 
                                        aRowPtr )
                              != IDE_SUCCESS );

                    IDE_TEST( smcRecord::insertLargeVarColumn ( aTrans,
                                                                aHeader,
                                                                sRowPtrBuffer,
                                                                sColumn,
                                                                ID_TRUE /*Add OID*/,
                                                                sCurValue->value,
                                                                sCurValue->length,
                                                                &sFstVCPieceOID )
                              != IDE_SUCCESS );

                    break;

                case SMI_COLUMN_TYPE_FIXED:
                default:
                    break;
            }
        }
        else
        {
            // PROJ-2264
            // nothing to do
        }
    }

    if ( sUnitedVarColumnCount > 0 )
    {
        IDE_TEST( smcRecord::updateUnitedVarColumns( aTrans,
                                                     aHeader,
                                                     aRowPtr,
                                                     sRowPtrBuffer,
                                                     sUnitedVarColumns,
                                                     sUnitedVarValues,
                                                     sUnitedVarColumnCount,
                                                     &sDummyCount,
                                                     &sDummySize )
                != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( smcRecordUpdate::writeUpdateInplaceLog( aTrans,
                                                      aHeader,
                                                      aRowPtr,
                                                      aColumnList,
                                                      aValueList,
                                                      sRowPtrBuffer,
                                                      aModifyIdxBit )
              != IDE_SUCCESS );
    sState = 1;

    if ( aModifyIdxBit != 0)
    {
        /* Table의 Index에서 aRowPtr를 지운다. */
        IDE_TEST( smLayerCallback::deleteRowFromIndex( aRowPtr,
                                                       aHeader,
                                                       aModifyIdxBit )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
    
    sCurColumnList = aColumnList;
    sCurValue      = aValueList;

    for( sColumnSeq = 0 ;
         sCurColumnList != NULL;
         sCurColumnList  = sCurColumnList->next, sCurValue++, sColumnSeq++ )
    {
        sColumn = sCurColumnList->column;

        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:

                    /* Fixed Row가 새로운 LobDesc를 가리키도록 한다.*/
                    sCurLobDesc = (smcLobDesc*)(sRowPtrBuffer + sColumn->offset);

                    sStoreMode = sCurLobDesc->flag & SM_VCDESC_MODE_MASK;

                    if( sStoreMode == SM_VCDESC_MODE_OUT )
                    {
                        idlOS::memcpy(aRowPtr + sColumn->offset,
                                      sCurLobDesc,
                                      ID_SIZEOF(smcLobDesc));
                    }
                    else
                    {
                        IDE_ASSERT_MSG( sStoreMode == SM_VCDESC_MODE_IN,
                                        "Table OID  : %"ID_vULONG_FMT"\n"
                                        "Space ID   : %"ID_UINT32_FMT"\n"
                                        "Page  ID   : %"ID_UINT32_FMT"\n"
                                        "Column seq : %"ID_UINT32_FMT"\n"
                                        "Store Mode : %"ID_UINT32_FMT"\n",
                                        aHeader->mSelfOID,
                                        aHeader->mSpaceID,
                                        sPageID,
                                        sColumnSeq,
                                        sStoreMode );

                        /* In - Mode일 경우 Value가 sRowPtrBuffer에 저장되어 있다.*/
                        idlOS::memcpy(aRowPtr + sColumn->offset,
                                      sCurLobDesc,
                                      ID_SIZEOF(smVCDescInMode) + sCurLobDesc->length);
                    }

                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    /* Geometry Type 또는 VARIABLE_LARGE 타입을 지정한 경우
                     * UnitedVar가 아닌 PROJ-2419 적용 전 Variable 구조를 사용한다. */
                    /* Fixed Row가 새로운 VC를 가리키도록 한다.*/
                    sCurVCDesc = (smVCDesc*)(sRowPtrBuffer + sColumn->offset);

                    sStoreMode = sCurVCDesc->flag & SM_VCDESC_MODE_MASK;

                    if ( sStoreMode == SM_VCDESC_MODE_OUT )
                    {
                        idlOS::memcpy( aRowPtr + sColumn->offset,
                                       sCurVCDesc,
                                       ID_SIZEOF(smVCDesc) );
                    }
                    else
                    {
                        IDE_ASSERT_MSG( sStoreMode == SM_VCDESC_MODE_IN,
                                        "Table OID  : %"ID_vULONG_FMT"\n"
                                        "Space ID   : %"ID_UINT32_FMT"\n"
                                        "Page  ID   : %"ID_UINT32_FMT"\n"
                                        "Column seq : %"ID_UINT32_FMT"\n"
                                        "Store Mode : %"ID_UINT32_FMT"\n",
                                        aHeader->mSelfOID,
                                        aHeader->mSpaceID,
                                        sPageID,
                                        sColumnSeq,
                                        sStoreMode );

                        /* In - Mode일 경우 Value가 sRowPtrBuffer에 저장되어 있다.*/
                        idlOS::memcpy(aRowPtr + sColumn->offset,
                                      sCurVCDesc,
                                      ID_SIZEOF(smVCDescInMode) + sCurVCDesc->length);
                    }
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
        else
        {
            // PROJ-2264
            idlOS::memcpy(aRowPtr + sColumn->offset,
                          sCurValue->value,
                          sCurValue->length);
        }
    }

    // BUG-47366 UnitedVar의 Alloc Log 제거
    // insert UnitedVar 에서 발생한 alloc Slot들은 Slot Header가 초기화 안되있음
    // 초기화 해주어야 한다.
    if ( sUnitedVarColumnCount > 0 )
    {
        sVCPieceOID = ((smpSlotHeader*)sRowPtrBuffer)->mVarOID;
        while ( sVCPieceOID != SM_NULL_OID )
        {
            IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                               sVCPieceOID,
                                               (void**)&sVCPiecePtr )
                        == IDE_SUCCESS );
            sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
            
            sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
            sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
            sVCPieceHeader->flag &= ~SM_VCPIECE_TYPE_MASK;
            sVCPieceHeader->flag |= SM_VCPIECE_TYPE_OTHER;

            IDE_TEST( smmDirtyPageMgr::insDirtyPage( aHeader->mSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                      != IDE_SUCCESS);

            sVCPieceOID = sVCPieceHeader->nxtPieceOID;
        }

        /* 기존에는 삭제부터 하고 새로 insert 하지만 united var 는 old value 를 읽어와야하므로
         * 새로 입력하고서 삭제한다 */
        IDE_TEST( deleteVC( aTrans,
                            aHeader,
                            ((smpSlotHeader*)aRowPtr)->mVarOID,
                            ID_FALSE ) 
                != IDE_SUCCESS );
    }

    /* United Var 카피 */
    ((smpSlotHeader*)aRowPtr)->mVarOID = ((smpSlotHeader*)sRowPtrBuffer)->mVarOID;

    *aRow = (SChar*)aRowPtr;

    if(aRetSlotGRID != NULL)
    {
        SC_MAKE_GRID( *aRetSlotGRID,
                      aHeader->mSpaceID,
                      sPageID,
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRowPtr ) );
    }

    sState = 0;
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                            sPageID) != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       aHeader->mSelfOID,
                                       sFixOID,
                                       aHeader->mSpaceID,
                                       SM_OID_UPDATE_FIXED_SLOT )
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_column_size );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INVALID_COLUMN_SIZE) );
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                                 sPageID) == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * task-2399 Lock Row와 Delete Row를 없애자.
 *
 *
 * *************************
 * 기존의 동작 방식 및 문제점
 * *************************
 * 기존에는 delete row연산을 수행하면, delete 연산을 수행한 row에 새로운 버전을
 * 생성하여 delete 표시를 한 후, mNext로 연결하였다.
 * 현재 row가 delete 되었는지 여부는 mNext에 연결된 row가 delete row인지를 확인
 * 하면 알수 있었고, 다른 트랜잭션이 현재 row를 봐야하는지 여부를 결정할때도,
 * 'update row연산을 통해 만들어진 버전에서 자신이 봐야하는 view를 결정하는 것'과 같은
 * 방법을 적용할 수 있었다. rollback이 수행될때도 같은 방법을 적용하면 되었다.( 새로
 * 생성된 버전을 aging)
 *
 * 하지만 이 때문에 항상 1개의 row를 여분으로 남겨두어야 했고 (database가 가득 찼을때
 * 삭제가 가능하도록), delete row를 수행할때 새로운 row를 생성해야 했었다.
 * 이것은 소스코드를 어렵게 만드는 원인이 되었고, 성능에도 영향을 미쳤다.  그래서 delete
 * row연산이 수행될때 새로운 버전(row)를 생성하지 않고, 현재 연산이 수행되는 row에
 * delete연산의 정보를 저장하는 방식을 적용하였다.
 *
 * *****************************
 * 새로운 방식을 적용
 * *****************************
 * 'delete row연산이 수행되는 것'은 'update row연산이 수행되는 것'에 대해 결정적으로 다른
 * 점이 있다.  그것은 update row로 생성된 새로운 버전에 대해 다시 새로운 버전이 생성
 * 될 수 있으나, delete row연산이 수행된 row에 대해서는 더이상 새로운 버전이 생성되지
 * 않는다는 것이다.
 * 그렇기 때문에 delete row연산이 수행된 row의 mNext를 다른 용도로 사용할 수 있게 된다.
 * 왜냐하면 더이상 새로운 버전이 생성되지 않기 때문이다. 이곳에 'delete row연산이 수행된 row'
 * 에 대해 MVCC를 적용할 수 있도록 필요한 정보를 저장하고, 이 정보가 세팅된 row를 delete
 * row 연산이 수행된 row로 판단하면 된다.
 *
 * 즉, 정리하면...
 * 기존의 방식 : row.mNext = 새로운 버전(delete row)의 OID
 * 새로운 방식 : row.mNext = MVCC를 적용하기 위해 필요한 정보.
 *
 * 새로운 방식에서 MVCC를 적용하기 위해 필요한 정보는 delete row연산이 수행된 트랜잭션의
 * SCN과, 수행한 트랜잭션의 TID이다. 이것은 delete row연산을 수행한 트랜잭션이
 * 아직 COMMIT하지 않았을때, 같은 트랜잭션이 현재 row가 delete되었는지 여부를 판단할때,
 * 또는 COMMIT    하였다면, 다른 트랜잭션이 현재 row가 delete되었는지 여부를 판단할때,
 * 필요하다.
 *
 * 새로운 방식에서는
 * mNext의 공간은 SCN을 저장하기 위한 공간으로 사용하고, TID를 저장하기 위해서는 현재
 * row의 mTID공간을 이용한다.  즉 delete 연산이 수행되는 row의 mTID에 저장된 내용을
 * 현재 트랜잭션으로 엎어치는 것이다.
 *
 * ***************************
 * 새로운 방식의 적용이 가능한 이유
 * ***************************
 * mNext에 OID대신에 SCN을 쓰는 것이 가능한 이유는
 * 1. oid는 항상 8byte align되어있기 때문에 마지막 3bit가 항상 0이다.
 * 그렇기 때문에 SCN을 저장할때, 마지막 3byte가 000이 아닌 경우에 이것을 SCN으로 인식
 * 할 수 있고, 이경우에 현재 row는 delete row연산이 수행된 것으로 볼 수 있다.
 *
 * 현재의 mTID공간에 새로운 트랜잭션의 id를 기록하는것이 가능한 이유는
 * 1. delete 연산을 수행하는 트랜잭션은 현재 row에 저장되어있는 트랜잭션과 같은 트랜잭션
 * 이거나
 * 2. row의 mTID에 저장된 트랜잭션은 이미 수행이 완료된 트랜잭션이다.(이 경우에는 기존의
 * mTID 정보는 의미가 없다.)
 * 그렇기 때문에 현재 row의 mTID정보를 갱신할 수 있는 것이다.
 *
 * *********************************
 * 새로운 방식에서 야기된 동시성 관련 문제
 * *********************************
 * 이전에 비해 달라진 가장 중요한 사항은, 동시성에 관련된 문제이다.
 * write연산( update, insert, delete)은 페이지 래치를 잡고선 변경을 수행하기 때문에
 * write연산 끼리는 동시성 문제가 나타나지 않지만,
 * read 연산( checkSCN)은 페이지 래치를 잡지 않고선 row를 읽기 때문에, 문제가 생길수
 * 있다.
 *
 * 예전에는 변경되는 것이 단지 mNext였고, 이것은 새로운 버전이 생성되고 난 이후에 바뀌는
 * 것이 었기 때문에 문제가 되지 않았다.
 * 하지만 지금은 mTID와 mNext가 같이 변하고, 예전과는 다르게 mNext가 32bit 머신 에서도
 * 64bit로 바뀌었기 때문에 (즉, mNext도 두번에 걸쳐서 변한다. 예전에는 32bit에서는
 * 32bit였었다. ) 동시성에 각별히 신경을 써주어야 한다.
 *
 * 이것을 해결하기 위해 row에 write연산을 수행할때 mTID와 mNext를 변경하는 순서를
 * 결정하였고, read트랜잭션에서는 mTID와 mNext를 읽는 순서를 변경하는 순서의 역순으로
 * 하였다. mNext의 변수를 읽어오는것과 쓰는 것도 순서를 맞추었다. 동시성에 관련된 자세한
 * 것은 설계문서를 참조하면 된다.
 *
 ***********************************************************************/
/***********************************************************************
 * Description : aRow가 가리키는 row를 삭제한다.
 *
 * aStatistics   - [IN] None
 * aTrans        - [IN] Transaction Pointer
 * aViewSCN      - [IN] 이 연산을 수행하는 Cursor의 ViewSCN
 * aTableInfoPtr - [IN] Table Info Pointer(Table에 Transaction이 Insert,
 *                        Delete한 Record Count에 대한 정보를 가지고 있다.
 * aHeader       - [IN] Table Header Pointer
 * aRow          - [IN] 삭제할 Row
 * aRetSlotGRID  - [IN] None
 * aInfinite     - [IN] Cursor의 Infinite SCN
 * aRetryInfo    - [IN] Retry를 위해 점검할 column의 list
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC smcRecord::removeVersion( idvSQL               * /*aStatistics*/,
                                 void                 * aTrans,
                                 smSCN                  aViewSCN,
                                 void                 * aTableInfoPtr,
                                 smcTableHeader       * aHeader,
                                 SChar                * aRow,
                                 scGRID                /*aSlotGRID*/,
                                 smSCN                  aInfinite,
                                 const smiDMLRetryInfo* aRetryInfo,
                                 idBool                 aIsDequeue,   
                                 idBool                 aForbiddenToRetry )
{
    const smiColumn   * sColumn;
    UInt                sLockState = 0;
    scPageID            sPageID;
    UInt                i;
    smOID               sRemoveOID;
    smpPersPageHeader * sPagePtr;
    idBool              sImplFlagChange = ID_FALSE;
    UInt                sColumnCnt;
    smcMakeLogFlagOpt   sMakeLogOpt = SMC_MKLOGFLAG_SET_ALLOC_FIXED_NO;
    ULong               sDeleteSCN;
    ULong               sNxtSCN;
    smTID               sNxtTID;
    smpSlotHeader     * sSlotHeader = (smpSlotHeader*)aRow;

    IDE_ERROR( aRow    != NULL );
    IDE_ERROR( aTrans  != NULL );
    IDE_ERROR( aHeader != NULL );
    
    IDU_FIT_POINT( "1.PROJ-2118@smcRecord::removeVersion" );

    sPageID = SMP_SLOT_GET_PID( aRow );
    sRemoveOID = SM_MAKE_OID( sPageID,
                              SMP_SLOT_GET_OFFSET( sSlotHeader ) );
    SMX_GET_SCN_AND_TID( sSlotHeader->mLimitSCN, sNxtSCN, sNxtTID );

    /* BUG-48230: 자신이 이미 lock row를 잡았으면 record lock validation
     *            할 필요가 없다. */
    if (( SM_SCN_IS_NOT_LOCK_ROW( sNxtSCN ) ) ||
        ( smxTrans::getTransID( aTrans ) != sNxtTID ))
    {
        /*  remove하기 위하여 이 row가 속한 page에 대하여 holdPageXLatch */
        IDE_TEST( smmManager::holdPageXLatch( aHeader->mSpaceID,
                                              sPageID ) != IDE_SUCCESS );
        sLockState = 1;

        IDE_TEST( recordLockValidation( aTrans,
                                        aViewSCN,
                                        aHeader,
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
        sPageID = SMP_SLOT_GET_PID( aRow );
        sRemoveOID = SM_MAKE_OID( sPageID,
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

        smpFixedPageList::dumpSlotHeader( sSlotHeader );

        IDE_ASSERT( 0 );
    }

    //slotHeader의 mNext에 들어갈 값을 세팅한다. delete연산이 수행되면
    //이 연산을 수행하는 트랜잭션의 aInfinite정보가 세팅된다. (with deletebit)
    SM_GET_SCN( &sDeleteSCN, &aInfinite );
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

    /* remove의 target이 되는 row는 항상 mNext만 쓰이고 있다. */
    IDE_TEST( smcRecordUpdate::writeRemoveVersionLog(
                                                      aTrans,
                                                      aHeader,
                                                      aRow,
                                                      sNxtSCN,
                                                      sDeleteSCN,
                                                      sMakeLogOpt,
                                                      &sImplFlagChange)
              != IDE_SUCCESS );

    IDU_FIT_POINT( "BUG-49063@smcRecord::removeVersion::setLimitSCN" );
    SM_SET_SCN( &(sSlotHeader->mLimitSCN), &sDeleteSCN );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                            sPageID) != IDE_SUCCESS );

    if ( sLockState == 1 )
    {
        IDE_TEST( smmManager::releasePageLatch(aHeader->mSpaceID, sPageID)
                  != IDE_SUCCESS );
        sLockState = 0;
    }
 
    /* BUG-32091 [sm_collection] add TableOID in PageHeader
     * Page에 기록된 TableOID와 위로부터 내려온 TableOID를 비교하여 검증함 */
    IDE_ASSERT( smmManager::getPersPagePtr( aHeader->mSpaceID,
                                            sPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    IDE_ASSERT( sPagePtr->mTableOID == aHeader->mSelfOID );

    sColumnCnt = aHeader->mColumnCount;

    for(i=0 ; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn(aHeader, i);

        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
                    /* LOB에 속한 Piece들의 Delete Flag만 Delete되었다고
                       설정한다.*/
                    IDE_TEST( deleteLob( aTrans, 
                                         aHeader,
                                         sColumn,
                                         aRow,
                                         ID_FALSE )
                              != IDE_SUCCESS );

                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    /* Geometry Type 또는 VARIABLE_LARGE 타입을 지정한 경우
                     * UnitedVar가 아닌 PROJ-2419 적용 전 Variable 구조를 사용한다. */
                    IDE_TEST( deleteVC( aTrans, 
                                        aHeader, 
                                        sColumn,
                                        aRow,
                                        ID_FALSE )
                              != IDE_SUCCESS );
                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                case SMI_COLUMN_TYPE_FIXED:
                default:
                    break;
            }
        }
        else
        {
            // PROJ-2264
            // Nothing to do
        }
    }

    IDE_TEST( deleteVC( aTrans,
                        aHeader,
                        ((smpSlotHeader*)aRow)->mVarOID, 
                        ID_FALSE) 
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
    IDE_PUSH();

    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부하 및
    //Aging이 밀리는 현상이 더 심화됨
    //remove시 inconsistency한 view에 의해 retry한 횟수.
    if( ideGetErrorCode() == smERR_RETRY_Already_Modified )
    {
        SMX_INC_SESSION_STATISTIC( aTrans,
                                   IDV_STAT_INDEX_DELETE_RETRY_COUNT,
                                   1 /* Increase Value */ );

        smcTable::incStatAtABort( aHeader, SMC_INC_RETRY_CNT_OF_DELETE );
    }

    if( sLockState != 0 )
    {
        // PROJ-1784 DML without retry
        // aRow가 변경 될 수 있으므로 PID를 다시 가져와야 함
        sPageID = SMP_SLOT_GET_PID( aRow );

        IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID,
                                                 sPageID) == IDE_SUCCESS);
        IDE_ASSERT(smmManager::releasePageLatch(aHeader->mSpaceID,
                                                sPageID) == IDE_SUCCESS);
    }
    IDE_POP();


    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC smcRecord::nextOIDall( smcTableHeader * aHeader,
                              SChar          * aCurRow,
                              SChar         ** aNxtRow )
{

    IDE_ASSERT( aHeader != NULL );
    IDE_ASSERT( aNxtRow != NULL );

    return smpAllocPageList::nextOIDall( aHeader->mSpaceID,
                                         aHeader->mFixedAllocList.mMRDB,
                                         aCurRow,
                                         aHeader->mFixed.mMRDB.mSlotSize,
                                         aNxtRow );
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC smcRecord::freeVarRowPending( void            * aTrans,
                                     smcTableHeader  * aHeader,
                                     smOID             aPieceOID,
                                     SChar           * aRow )
{
    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        IDE_TEST(smpVarPageList::addFreeSlotPending(
                                             aTrans,
                                             aHeader->mSpaceID,
                                             aHeader->mVar.mMRDB,
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
IDE_RC smcRecord::freeFixRowPending( void            * aTrans,
                                     smcTableHeader  * aHeader,
                                     SChar           * aRow )
{
    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        IDE_TEST(smpFixedPageList::addFreeSlotPending(
                                         aTrans,
                                         aHeader->mSpaceID,
                                         &(aHeader->mFixed.mMRDB),
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
IDE_RC smcRecord::setFreeVarRowPending( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        smOID             aPieceOID,
                                        SChar           * aRow,
                                        smSCN             aSCN )
{
    smLSN sNTA;

    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

        IDE_TEST(smpVarPageList::freeSlot(aTrans,
                                          aHeader->mSpaceID,
                                          aHeader->mVar.mMRDB,
                                          aPieceOID,
                                          aRow,
                                          &sNTA,
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
IDE_RC smcRecord::setFreeFixRowPending( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        SChar           * aRow,
                                        smSCN             aSCN )
{
    if(smcTable::isDropedTable(aHeader) == ID_FALSE)
    {
        IDU_FIT_POINT( "1.BUG-15969@smcRecord::setFreeFixRowPending" );

        IDE_TEST(smpFixedPageList::freeSlot(aTrans,
                                            aHeader->mSpaceID,
                                            &(aHeader->mFixed.mMRDB),
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
IDE_RC smcRecord::setSCN( scSpaceID  aSpaceID,
                          SChar   *  aRow,
                          smSCN      aSCN )
{
    smpSlotHeader *sSlotHeader;
    scPageID       sPageID;

    sSlotHeader = (smpSlotHeader*)aRow;

    sPageID = SMP_SLOT_GET_PID( aRow );
    SM_SET_SCN( &(sSlotHeader->mCreateSCN), &aSCN );

    return smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID);
}

/***********************************************************************
 * Description :
 * PROJ-2429 Dictionary based data compress for on-disk DB
 * setSCN 연산을 로깅하여 record가 refine시 정리되는것을 방지
 ***********************************************************************/
IDE_RC smcRecord::setSCNLogging( void           * aTrans,
                                 smcTableHeader * aHeader,
                                 SChar          * aRow,
                                 smSCN            aSCN )
{
    smpSlotHeader *sSlotHeader;
    scPageID       sPageID;

    IDE_ASSERT( smcRecordUpdate::writeSetSCNLog( aTrans, 
                                                 aHeader,
                                                 aRow )
                == IDE_SUCCESS );

    sSlotHeader = (smpSlotHeader*)aRow;

    sPageID = SMP_SLOT_GET_PID( aRow );
    SM_SET_SCN( &(sSlotHeader->mCreateSCN), &aSCN );

    return smmDirtyPageMgr::insDirtyPage(aHeader->mSpaceID, sPageID);
}

/***********************************************************************
 * Description : IN-DOUBT Transaction이 수정한 레코드에 무한대 SCN을
 *               설정한다.
 *
 * [BUG-26415] XA 트랜잭션중 Partial Rollback(Unique Volation)된 Prepare
 *             트랜잭션이 존재하는 경우 서버 재구동이 실패합니다.
 * : Insert 도중 Unique Volation으로 실패한 경우는 롤백된 레코드의
 *   Delete Bit를 유지해야 한다.( refine단계에서 제거되어야 한다 )
 * : 재구동시에만 호출되는 함수임.
 ***********************************************************************/
IDE_RC smcRecord::setSCN4InDoubtTrans( scSpaceID  aSpaceID,
                                       smTID      aTID,
                                       SChar   *  aRow )
{
    smpSlotHeader *sSlotHeader;
    scPageID       sPageID;
    smSCN          sSCN;
    ULong          sTID;

    sTID = (ULong)aTID;

    sSlotHeader = (smpSlotHeader*)aRow;

    SM_SET_SCN_INFINITE_AND_TID( &sSCN, sTID );

    sPageID = SMP_SLOT_GET_PID( aRow );

    if( SM_SCN_IS_DELETED( sSlotHeader->mCreateSCN ) )
    {
        SM_SET_SCN_DELETE_BIT( &sSCN );
    }

    SM_SET_SCN( &(sSlotHeader->mCreateSCN), &sSCN );

    return smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID);
}

/***********************************************************************
 * Description : row의 mNext를 SCN으로 변경하는 함수
 ***********************************************************************/
IDE_RC smcRecord::setRowNextToSCN( scSpaceID            aSpaceID,
                                   SChar              * aRow,
                                   smSCN                aSCN )
{
    smpSlotHeader *sSlotHeader;
    scPageID       sPageID;

    IDE_ASSERT( aRow != NULL );

    sSlotHeader = (smpSlotHeader*)aRow;

    sPageID = SMP_SLOT_GET_PID(aRow);

    SM_SET_SCN( &(sSlotHeader->mLimitSCN), &aSCN );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID)
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aRowHeader에 Delete Bit를 설정한다.
 *
 * aSpaceID   - [IN] aRowHeader가 속한 Table이 있는 Table Space ID
 * aRowHeader - [IN] Row의 Pointer
 ***********************************************************************/
IDE_RC smcRecord::setDeleteBitOnHeader( scSpaceID       aSpaceID,
                                        smpSlotHeader * aRowHeader )
{
    return setDeleteBitOnHeaderInternal( aSpaceID,
                                         aRowHeader,
                                         ID_TRUE /* Set Delete Bit */ );
}

/***********************************************************************
 * Description : aRowHeader에 Delete Bit를 set/unset한다.
 *
 * aSpaceID        - [IN] aRowHeader가 속한 Table이 있는 Table Space ID
 * aRowHeader      - [IN] Row의 Pointer
 * aIsSetDeleteBit - [IN] if aIsSetDeleteBit = ID_TRUE, set delete bit,
 *                        else unset delete bit of row.
 ***********************************************************************/
IDE_RC smcRecord::setDeleteBitOnHeaderInternal( scSpaceID       aSpaceID,
                                                void          * aRowHeader,
                                                idBool          aIsSetDeleteBit)
{
    scPageID       sPageID;
    smpSlotHeader *sSlotHeader = (smpSlotHeader*)aRowHeader;

    if( aIsSetDeleteBit == ID_TRUE )
    {
        SM_SET_SCN_DELETE_BIT( &(sSlotHeader->mCreateSCN) );
    }
    else
    {
        SM_CLEAR_SCN_DELETE_BIT( &(sSlotHeader->mCreateSCN) );
    }

    sPageID = SMP_SLOT_GET_PID(sSlotHeader);
    return smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID);
}

/***********************************************************************
 * Description : aRowHeader의 SCN에 Delete Bit을 Setting한다.
 *
 * aTrans     - [IN]: Transaction Pointer
 * aRowHeader - [IN]: Row Header Pointer
 * aFlag      - [IN]: 1. SMC_WRITE_LOG_OK : log기록.
 *                    2. SMC_WRITE_LOG_NO : log기록안함.
 ***********************************************************************/
IDE_RC smcRecord::setDeleteBit( void             * aTrans,
                                scSpaceID          aSpaceID,
                                void             * aRowHeader,
                                SInt               aFlag)
{
    scPageID    sPageID;
    smOID       sRecOID;
    UInt        sState = 0;

    sPageID = SMP_SLOT_GET_PID(aRowHeader);
    sRecOID = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRowHeader ) );

    IDE_TEST(smmManager::holdPageXLatch(aSpaceID, sPageID) != IDE_SUCCESS);
    sState = 1;

    if(aFlag == SMC_WRITE_LOG_OK)
    {
        IDE_TEST(smrUpdate::setDeleteBitAtFixRow(NULL, /* idvSQL* */
                                                 aTrans,
                                                 aSpaceID,
                                                 sRecOID)
                 != IDE_SUCCESS);

    }

    /* BUG-14953 : PK가 두개들어감. Rollback시 SCN값이
       항상 무한대 Bit가 Setting되어 있어야 한다. 그렇지
       않으면 Index의 Unique Check가 오동작하여 Duplicate
       Key가 들어간다.*/
    SM_SET_SCN_DELETE_BIT( &(((smpSlotHeader*)aRowHeader)->mCreateSCN) );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID) != IDE_SUCCESS);

    IDE_TEST(smmManager::releasePageLatch(aSpaceID, sPageID) != IDE_SUCCESS);
    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                 sPageID) == IDE_SUCCESS);
        IDE_ASSERT(smmManager::releasePageLatch(aSpaceID,
                                                sPageID) == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * DESCRIPTION : 테이블 생성시에 NTA OP 타입인 SMR_OP_CREATE_TABLE 에
 * 대한 udno 처리시에 수행하는 함수로써 table header가 저장된 fixed slot
 * 에 대한 처리를 한다.
 ***********************************************************************/
IDE_RC smcRecord::setDropTable( void   *  aTrans,
                                SChar  *  aRow )
{
    scPageID       sPageID;
    UInt           sState = 0;
    smpSlotHeader *sSlotHeader;
    smOID          sRecOID;

    sSlotHeader= (smpSlotHeader*)aRow;

    sPageID = SMP_SLOT_GET_PID(aRow);
    sRecOID = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( sSlotHeader ) );

    IDE_TEST(smmManager::holdPageXLatch(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                        sPageID) != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(smrUpdate::setDeleteBitAtFixRow(
                 NULL, /* idvSQL* */
                 aTrans,
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 sRecOID) != IDE_SUCCESS);


    IDE_TEST(smrUpdate::setDropFlagAtFixedRow(
                 NULL, /* idvSQL* */
                 aTrans,
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 sRecOID,
                 ID_TRUE) != IDE_SUCCESS);


    // BUG-27329 CodeSonar::Uninitialized Variable (2)
    SM_SET_SCN_DELETE_BIT( &(sSlotHeader->mCreateSCN) );

    SMP_SLOT_SET_DROP( sSlotHeader );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           sPageID) != IDE_SUCCESS);

    IDE_TEST(smmManager::releasePageLatch(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                          sPageID) != IDE_SUCCESS);
    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(
                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID)
                   == IDE_SUCCESS);
        IDE_ASSERT(smmManager::releasePageLatch(
                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID)
                   == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aRID가 가리키는 Variable Column Piece의 Flag값을 aFlag로 바꾼다.
 *
 * aTrans       - [IN] Transaction Pointer
 * aTableOID    - [IN] Table OID
 * aSpaceID     - [IN] Variable Column Piece 가 속한 Tablespace의 ID
 * aVCPieceOID  - [IN] Variable Column Piece OID
 * aVCPiecePtr  - [IN] Variabel Column Piece Ptr
 * aVCPieceFlag - [IN] Variabel Column Piece Flag
 ***********************************************************************/
IDE_RC smcRecord::setFreeFlagAtVCPieceHdr( void    * aTrans,
                                           smOID     aTableOID,
                                           scSpaceID aSpaceID,
                                           smOID     aVCPieceOID,
                                           void    * aVCPiecePtr,
                                           UShort    aVCPieceFreeFlag,
                                           idBool    aLogging /* BUG-46854: default 인수 추가 */)
{
    smVCPieceHeader *sVCPieceHeader;
    scPageID         sPageID;
    UShort           sVCPieceFlag;

    // fix BUG-27221 : [codeSonar] Null Pointer Dereference
    IDE_ASSERT( aVCPiecePtr != NULL );

    sPageID        = SM_MAKE_PID(aVCPieceOID);
    sVCPieceHeader = (smVCPieceHeader *)aVCPiecePtr;

    sVCPieceFlag = sVCPieceHeader->flag;
    sVCPieceFlag &= ~SM_VCPIECE_FREE_MASK;
    sVCPieceFlag |= aVCPieceFreeFlag;

    // BUG-46854 : delete일 때 flag 세팅 로그 쓰기 삭제 ( 미리 씀 )
    if ( aLogging == ID_TRUE )
    { 
        IDE_TEST(smrUpdate::setFlagAtVarRow(NULL, /* idvSQL* */
                                            aTrans,
                                            aSpaceID,
                                            aTableOID,
                                            aVCPieceOID,
                                            sVCPieceHeader->flag,
                                            sVCPieceFlag)
                 != IDE_SUCCESS);
    }

    sVCPieceHeader->flag = sVCPieceFlag;

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC smcRecord::setIndexDropFlag( void    * aTrans,
                                    smOID     aTableOID,
                                    smOID     aIndexOID,
                                    void    * aIndexHeader,
                                    UShort    aDropFlag )
{
    scPageID          sPageID;
    smnIndexHeader  * sIndexHeader;

    sPageID      = SM_MAKE_PID(aIndexOID);
    sIndexHeader = (smnIndexHeader *)aIndexHeader;

    IDE_TEST(smrUpdate::setIndexDropFlag(NULL, /* idvSQL* */
                                         aTrans,
                                         aTableOID,
                                         aIndexOID,
                                         sIndexHeader->mDropFlag,
                                         aDropFlag)
             != IDE_SUCCESS);


    sIndexHeader->mDropFlag = aDropFlag;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            sPageID)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * task-2399 Lock Row와 Delete Row를 없애자.
 *
 *
 * 기존에는 Lock Row연산을 수행하면, 새로운 버전(lock row)를 만들어 lock 연산이
 * 수행된 row의 mNext로 연결하였다.
 * 하지만 lock의 경우엔 lock이 걸린 당시의 scn이 필요없기 때문에(MVCC가 적용되지 않음)
 * 구지 이렇게 할 필요가 없다. 즉, 단지 lock이 걸려있는지 여부만 표시하면되고, 이것을
 * 수행한 트랜잭션을 적어주면 된다.
 *
 * 이렇게 하기 위해서 현재 row의 mTID와 mNext를 이용한다.
 * mTID에는 Lock row연산을 수행한 트랜잭션의 tid를 적어주고, mNext는 Lock이 걸렸다는
 * 것을 표시해 준다.
 *
 * 이러한 방식이 가능한 이유는 위에 removeVersion함수 위에 적혀진 주석의 '새로운 방식의
 * 적용이 가능한 이유'와 '새로운 방식에서 야기된 동시성 관련 문제'를 참고하면 된다.
 *
 ***********************************************************************/
/***********************************************************************
 * Description : select for update를 수행할때 불리는 함수. 즉, select for update
 *          를 수행한 row에 대해서 Lock연산이 수행된다. 이때, svpSlotHeader의
 *          mNext에 LockRow임을 표시한다.  이정보는 현재 트랜잭션이 수행중인 동안만
 *          유효하다.
 ***********************************************************************/
IDE_RC smcRecord::lockRow(void           * aTrans,
                          smSCN            aViewSCN,
                          smcTableHeader * aHeader,
                          SChar          * aRow,
                          ULong            aLockWaitTime,
                          idBool           aForbiddenToRetry )
{
    smpSlotHeader * sSlotHeader;
    scPageID        sPageID;
    UInt            sState          = 0;
    smSCN           sRowSCN;
    smTID           sRowTID;
    
    sSlotHeader = (smpSlotHeader*)aRow;

    sPageID = SMP_SLOT_GET_PID(sSlotHeader);

    IDE_TEST(smmManager::holdPageXLatch(aHeader->mSpaceID,
                                        sPageID) != IDE_SUCCESS);
    sState = 1;

    SMX_GET_SCN_AND_TID( sSlotHeader->mLimitSCN, sRowSCN, sRowTID );

    //이미 자신이 LOCK을 걸었다면 다시 LOCK을 걸려고 하지 않는다.
    if ( ( SM_SCN_IS_LOCK_ROW( sRowSCN ) ) &&
         ( sRowTID == smLayerCallback::getTransID( aTrans ) ) )
    {
        /* do nothing */
    }
    else
    {
        IDE_TEST(recordLockValidation( aTrans,
                                       aViewSCN,
                                       aHeader,
                                       &aRow,
                                       aLockWaitTime,
                                       &sState,
                                       NULL, /* without retry info*/
                                       aForbiddenToRetry ) 
                 != IDE_SUCCESS );

        // PROJ-1784 retry info가 없으므로 변경되지 않는다.
        IDE_ASSERT( aRow == (SChar*)sSlotHeader );

        IDE_TEST( lockRowInternal( aTrans,
                                   aHeader,
                                   aRow ) != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST(smmManager::releasePageLatch(aHeader->mSpaceID,
                                          sPageID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        sPageID = SMP_SLOT_GET_PID(aRow);
        IDE_ASSERT(smmManager::releasePageLatch(aHeader->mSpaceID,
                                                sPageID) == IDE_SUCCESS);
        IDE_POP();
    }

    /* BUG-24151: [SC] Update Retry, Delete Retry, Statement Rebuild Count를
     *            AWI로 추가해야 합니다.*/
    if( ideGetErrorCode() == smERR_RETRY_Already_Modified )
    {
        SMX_INC_SESSION_STATISTIC( aTrans,
                                   IDV_STAT_INDEX_LOCKROW_RETRY_COUNT,
                                   1 /* Increase Value */ );

        smcTable::incStatAtABort( aHeader, SMC_INC_RETRY_CNT_OF_LOCKROW );
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
IDE_RC smcRecord::lockRowInternal(void           * aTrans,
                                  smcTableHeader * aHeader,
                                  SChar          * aRow)
{
    smpSlotHeader *sSlotHeader = (smpSlotHeader*)aRow;
    smOID          sRecOID;
    smSCN          sRowSCN;
    smTID          sRowTID;
    ULong          sTransID;
    ULong          sNxtSCN;

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
        
        sRecOID = SM_MAKE_OID( SMP_SLOT_GET_PID(aRow),
                               SMP_SLOT_GET_OFFSET( sSlotHeader ) );

        SM_GET_SCN( &sNxtSCN, &( sSlotHeader->mLimitSCN ) );

        /* BUG-17117 select ... For update후에 server kill, start하면
         * 레코드가 사라집니다.
         *
         * Redo시에는 Lock을 풀어주어야 하기때문에 redo시 AfterImage는
         * SM_NULL_OID가 되어야 합니다. */
        IDE_TEST( smrUpdate::updateNextVersionAtFixedRow(
                      NULL, /* idvSQL* */
                      aTrans,
                      aHeader->mSpaceID,
                      aHeader->mSelfOID,
                      sRecOID,
                      sNxtSCN,
                      SM_NULL_OID )
              != IDE_SUCCESS );


        sTransID = (ULong)smLayerCallback::getTransID( aTrans );
        SMP_SLOT_SET_LOCK( sSlotHeader, sTransID );

        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           sRecOID,
                                           aHeader->mSpaceID,
                                           SM_OID_LOCK_FIXED_SLOT )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : lockRow가 수행된 row에 대해 lock을 해제할때 호출되는 함수
 ***********************************************************************/
IDE_RC smcRecord::unlockRow( void           * aTrans,
                             scSpaceID        aSpaceID,
                             SChar          * aRow )
{
    smpSlotHeader *sSlotHeader;
    scPageID       sPageID;
    smTID          sRowTID;

    sSlotHeader = (smpSlotHeader*)aRow;

    sRowTID = SMP_GET_TID( sSlotHeader->mLimitSCN );

    if ( ( sRowTID == smLayerCallback::getTransID( aTrans ) ) &&
         ( SMP_SLOT_IS_LOCK_TRUE( sSlotHeader ) ) )
    {
        SMP_SLOT_SET_UNLOCK(sSlotHeader);

        sPageID = SMP_SLOT_GET_PID(aRow);

        IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                               sPageID) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
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
 *     : SMR_LT_UPDATE의 SMR_SMC_PERS_UPDATE_FIXED_ROW
 *   + table header에 nullrow에 대한 설정 및 로깅
 *     : SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_SET_NULLROW
 ***********************************************************************/
IDE_RC smcRecord::makeNullRow( void*           aTrans,
                               smcTableHeader* aHeader,
                               smSCN           aInfiniteSCN,
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
    IDE_TEST( insertVersion( NULL, // aStatistics*
                             aTrans,
                             NULL, // aTableInfoPtr
                             aHeader,
                             aInfiniteSCN,
                             (SChar**)&sNullSlotHeaderPtr,
                             NULL, // aRetInsertSlotGRID
                             aNullRow,
                             aLoggingFlag )
              != IDE_SUCCESS );
    sNullSlotHeaderPtr->mVarOID = SM_NULL_OID;

    idlOS::memcpy(&sAfterSlotHeader, sNullSlotHeaderPtr, SMP_SLOT_HEADER_SIZE);
    sAfterSlotHeader.mCreateSCN = sNullRowSCN;

    sNullRowPID = SMP_SLOT_GET_PID((SChar *)sNullSlotHeaderPtr);
    sNullRowOID = SM_MAKE_OID( sNullRowPID,
                               SMP_SLOT_GET_OFFSET( sNullSlotHeaderPtr ) );

    /* insert된 nullrow header의 변경을 로깅한다. */
    IDE_TEST(smrUpdate::updateFixedRowHead( NULL, /* idvSQL* */
                                            aTrans,
                                            aHeader->mSpaceID,
                                            sNullRowOID,
                                            sNullSlotHeaderPtr,
                                            &sAfterSlotHeader,
                                            ID_SIZEOF(smpSlotHeader) ) != IDE_SUCCESS);


    /* insert 된 nullrow에 scn을 설정한다. */
    IDE_TEST(setSCN(aHeader->mSpaceID,
                    (SChar*)sNullSlotHeaderPtr,
                    sNullRowSCN) != IDE_SUCCESS);

    *aNullRowOID = sNullRowOID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 32k over Variable Column을 Table의 Variable Page List에 Insert한다.
 *               따라서 항상 out-mode로 저장한다.
 *
 *               주의: VC Piece의 각 Header에 대해서는 Alloc시 logging을
 *                    하지만 VC Piece의 Value가 저장된 부분에 대해서는 Logging
 *                    하지 않는다. 왜냐하면 Insert, Update시 그 부분에 대한
 *                    Logging을 하기 때문이다. 여기서 Logging을 Update후에
 *                    하더라도 문제가 없는 이 이유는 Update영역의 Before Image가
 *                    의미가 없기 때문이다.
 *
 * aTrans       - [IN] Transaction Pointer
 * aHeader      - [IN] Table Header
 * aFixedRow    - [IN] VC의 VCDesc가 저장될 Fixed Row Pointer
 * aColumn      - [IN] VC Column Description
 * aAddOIDFlag  - [IN] Variable Column을 구성하는 VC를 Transaction OID List에
 *                     Add를 원하면 ID_TRUE, 아니면 ID_FALSE.
 * aValue       - [IN] Variable Column의 Value
 * aLength      - [IN] Variable Column의 Value의 Length
 * aFstPieceOID - [OUT] Variable Column을 구성하는 첫번째 VC Piece OID
 ***********************************************************************/
IDE_RC smcRecord::insertLargeVarColumn( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        SChar           * aFixedRow,
                                        const smiColumn * aColumn,
                                        idBool            aAddOIDFlag,
                                        const void      * aValue,
                                        UInt              aLength,
                                        smOID           * aFstPieceOID)
{
    UInt      sVCPieceLength;
    UInt      sValuePartLength = aLength;
    smOID     sCurVCPieceOID = SM_NULL_OID;
    smOID     sNxtVCPieceOID = SM_NULL_OID;
    SChar*    sPiecePtr;
    UInt      sOffset = 0;
    SChar*    sPartValue;
    smVCDesc* sVCDesc;

    smVCDescInMode *sVCDescInMode;

    IDE_ASSERT( aTrans    != NULL );
    IDE_ASSERT( aHeader   != NULL );
    IDE_ASSERT( aFixedRow != NULL );
    IDE_ASSERT( aColumn   != NULL );
    IDE_ASSERT_MSG(( aAddOIDFlag == ID_TRUE )||
                   ( aAddOIDFlag == ID_FALSE ),
                   "Add OID Flag : %"ID_UINT32_FMT"\n",
                   aAddOIDFlag );

    sVCDesc = getVCDesc(aColumn, aFixedRow);

    if(aLength <= aColumn->vcInOutBaseSize)
    {
        /* Store Value In In-Mode */
        sVCDescInMode = (smVCDescInMode*)sVCDesc;

        sVCDescInMode->length = aLength;
        sVCDescInMode->flag   = SM_VCDESC_MODE_IN;

        if(aLength != 0)
        {
            idlOS::memcpy(sVCDescInMode + 1, (SChar*)aValue, aLength);
        }
    }
    else
    {
        /* Store Value In Out-Mode */

        IDE_ASSERT( aFstPieceOID != NULL );

        /* =================================================================
         * Value가 SMP_VC_PIECE_MAX_SIZE를 넘는 경우 여러 페이지에 걸쳐 저장하고 넘지
         * 않는다면 하나의 Variable Piece에 저장한다. 이때 VC를 구성할 Piece의 저장 순서
         * 는 맨 마지막 Piece부터 저장한다. 이렇게 하는 이유는 Value의 앞부터 저장할 경우
         * 다음 Piece의 OID를 알수 없기 때문에 Piece에 대한 Logging과 별도로 현재 Piece
         * 의 앞단에 Piece가 존재할 경우 앞단의 Piece의 nextPieceOID에 대한 logging을
         * 별도로 수행해야한다. 하지만 맨 마지막 Piece부터 역순으로 저장할 경우 현재 Piece
         * 의 Next Piece OID을 알기 때문에 Variable Piece의 AllocSlot시 next piece에
         * 대한 oid logging을 같이 하여 logging양을 줄일 수 있다.
         * ================================================================ */
        sVCPieceLength  = sValuePartLength % SMP_VC_PIECE_MAX_SIZE;
        sOffset         = (sValuePartLength / SMP_VC_PIECE_MAX_SIZE) * SMP_VC_PIECE_MAX_SIZE;

        if( sVCPieceLength == 0 )
        {
            sVCPieceLength  = SMP_VC_PIECE_MAX_SIZE;
            sOffset        -= SMP_VC_PIECE_MAX_SIZE;
        }

        while ( 1 )
        {
            /* [1-1] info 정보를 위해 variable column 정보 구성 */
            sPartValue  = (SChar*)aValue + sOffset;

            /* [1-2] Value를 저장하기 위해 variable piece 할당 */
            IDE_TEST( smpVarPageList::allocSlot( aTrans,
                                                 aHeader->mSpaceID,
                                                 aHeader->mSelfOID,
                                                 aHeader->mVar.mMRDB,
                                                 sVCPieceLength,
                                                 sNxtVCPieceOID,
                                                 &sCurVCPieceOID,
                                                 &sPiecePtr )
                      != IDE_SUCCESS );

            IDE_TEST( smpVarPageList::setValue( aHeader->mSpaceID,
                                                sCurVCPieceOID,
                                                sPartValue,
                                                sVCPieceLength)
                      != IDE_SUCCESS );

            if( aAddOIDFlag == ID_TRUE )
            {
                /* [1-3] 새로 할당받은 variable piece를 versioning list에 추가 */
                IDE_TEST( smLayerCallback::addOID( aTrans,
                                                   aHeader->mSelfOID,
                                                   sCurVCPieceOID,
                                                   aHeader->mSpaceID,
                                                   SM_OID_NEW_VARIABLE_SLOT )
                          != IDE_SUCCESS );
            }

            /* [1-4] info 정보가 multiple page에 걸쳐서 저장되는 경우
             *       그 페이지들끼리의 리스트를 유지                  */
            sNxtVCPieceOID  = sCurVCPieceOID;

            sValuePartLength -= sVCPieceLength;
            if( sValuePartLength <= 0 )
            {
                IDE_ASSERT_MSG( sValuePartLength == 0,
                                "sValuePartLength : %"ID_UINT32_FMT"\n",
                                sValuePartLength );
                break;
            }

            sVCPieceLength  = SMP_VC_PIECE_MAX_SIZE;
            sOffset        -= SMP_VC_PIECE_MAX_SIZE;
        }

        sVCDesc->length = aLength;
        sVCDesc->flag   = SM_VCDESC_MODE_OUT;
        sVCDesc->fstPieceOID = sCurVCPieceOID;

        *aFstPieceOID = sCurVCPieceOID;
    }

    IDE_ASSERT_MSG( ( SM_VCDESC_IS_MODE_IN (sVCDesc) ) ||
                    ( sCurVCPieceOID != SM_NULL_OID ),
                    "Flag : %"ID_UINT32_FMT", "
                    "VCPieceOID : %"ID_UINT32_FMT"\n",
                    sVCDesc->flag ,
                    sCurVCPieceOID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 여러개의 Variable Column을 하나의 variable slot 에 Insert한다.
 *               항상 out-mode로 저장하며 OID는 fixed row slot header에 있다.
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
 * aFixedRow        - [IN] VC의 OID가 저장될 Fixed Row Pointer
 * aColumns         - [IN] VC Column Description
 * aAddOIDFlag      - [IN] Variable Column을 구성하는 VC를 Transaction OID List에
 *                         Add를 원하면 ID_TRUE, 아니면 ID_FALSE.
 * aValues          - [IN] Variable Column의 Values
 * aVarcolumnCount  - [IN] Variable Column과 Value의 숫자
 * aImageSize       - [OUT] insert한 united var 의 로그 크기
 ***********************************************************************/
IDE_RC smcRecord::insertUnitedVarColumns( void            *  aTrans,
                                          smcTableHeader  *  aHeader,
                                          SChar           *  aFixedRow,
                                          const smiColumn ** aColumns,
                                          idBool             aAddOIDFlag,
                                          smiValue        *  aValues,
                                          SInt               aVarColumnCount, 
                                          UInt            *  aImageSize )
{
    smOID         sCurVCPieceOID    = SM_NULL_OID;
    smOID         sNxtVCPieceOID    = SM_NULL_OID;

    // Piece 크기 초기화 : LastOffsetArray size
    UInt          sVarPieceLen      = ID_SIZEOF(UShort);

    UInt          sLogImageLen      = 0;
    UInt          sPrevVarPieceLen  = 0;
    UInt          sVarPieceCnt      = 0;
    UShort        sVarPieceColCnt   = 0;
    idBool        sIsTrailingNull   = ID_FALSE;
    SInt          i                 = ( aVarColumnCount - 1 ); /* 가장 마지막 Column 부터 삽입한다. */

    // BUG-43117
    // 현재의 앞 순번(ex. 5번 col이면 4번 col) 컬럼의 패딩 크기
    UInt          sPrvAlign         = 0;
    // 현재 컬럼의 추정 패딩 크기
    UInt          sCurAlign         = 0;
    // varPiece 헤더를 제외한 실제 패딩 크기와 offset array의 총크기가 합쳐진 실제 logging 할 vaPiece의 길이
    UInt          sVarPieceLen4Log  = 0;

    IDE_ASSERT( aTrans          != NULL );
    IDE_ASSERT( aHeader         != NULL );
    IDE_ASSERT( aFixedRow       != NULL );
    IDE_ASSERT( aColumns        != NULL );
    IDE_ASSERT( aVarColumnCount  > 0 );
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
            /* nothing to do */
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

            /* 2 = Previous Column offset +  End offset */  

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
                /* Previous Column을 더하면 슬롯 크기를 넘기므로, 지금까지 컬럼들 insert */
                IDE_TEST( insertUnitedVarPiece( aTrans,
                                                aHeader,
                                                aAddOIDFlag,
                                                &aValues[i],
                                                aColumns, 
                                                (UInt)(sVarPieceLen),
                                                sVarPieceColCnt,
                                                sNxtVCPieceOID,
                                                &sCurVCPieceOID,
                                                &sVarPieceLen4Log )
                        != IDE_SUCCESS );

                sVarPieceCnt++;
               
                // 실제 varPiece길이, except header
                sLogImageLen += sVarPieceLen4Log;

                sNxtVCPieceOID  = sCurVCPieceOID;
                sVarPieceColCnt = 0;
                
                /* insert 했으므로 다시 초기화 */
                sVarPieceLen    = ID_SIZEOF(UShort);
            }
        }
        else /* 첫컬럼에 도달한 경우, 모은 컬럼 insert */
        {
            sVarPieceColCnt++; 
            
            // value의 길이 + offset array 1칸 크기
            sVarPieceLen += aValues[i].length + ID_SIZEOF(UShort);
            
            //현재 컬럼의 패딩 크기 추정  /* BUG-43117 */
            sCurAlign = SMC_GET_COLUMN_PAD_LENGTH( aValues[i], aColumns[i] );

            sVarPieceLen += sCurAlign;  

            IDE_TEST( insertUnitedVarPiece( aTrans,
                                            aHeader,
                                            aAddOIDFlag,
                                            &aValues[i],
                                            aColumns, /* BUG-43117 */
                                            (UInt)(sVarPieceLen),
                                            sVarPieceColCnt,
                                            sNxtVCPieceOID,
                                            &sCurVCPieceOID,
                                            &sVarPieceLen4Log  /* BUG-43117 */)
                      != IDE_SUCCESS );
            sVarPieceCnt++;

            // 실제 varPiece길이, header 제외
            sLogImageLen += sVarPieceLen4Log;
        }
    }

    /* fixed row 에서 첫 var piece 로 이어지도록 oid 설정 */
    ((smpSlotHeader*)aFixedRow)->mVarOID = sCurVCPieceOID;

    //zzzz coverage 를 위해 if 삭제할 수 있음
    if ( sCurVCPieceOID != SM_NULL_OID )
    {
        *aImageSize += SMC_UNITED_VC_LOG_SIZE( sLogImageLen, aVarColumnCount, sVarPieceCnt );
    }
    else
    {
        *aImageSize += ID_SIZEOF(smOID);
    }

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
 * aVarPieceLen4Log - [OUT] 실제 varPiece길이, header 제외
 ***********************************************************************/
IDE_RC smcRecord::insertUnitedVarPiece( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        idBool            aAddOIDFlag,
                                        smiValue        * aValues,
                                        const smiColumn** aColumns, /* BUG-43117 */
                                        UInt              aTotalVarLen,
                                        UShort            aVarColCount,
                                        smOID             aNxtVCPieceOID,
                                        smOID           * aCurVCPieceOID,
                                        UInt            * aVarPieceLen4Log /* BUG-43117 */)
{
    SChar       * sPiecePtr         = NULL;
    UShort      * sColOffsetPtr     = NULL;     /* offsetArray 포인터 */
    smiValue    * sCurValue         = NULL;
    UShort        sValueOffset      = 0;
    UInt          i                 = 0;

    //PROJ-2419 insert UnitedVarPiece FitTest: before alloc slot
    IDU_FIT_POINT( "1.PROJ-2419@smcRecord::insertUnitedVarPiece::allocSlot" );

    /* PROJ-2419 allocSlot 내부에서 VarPiece Header의
     * NxtOID는 NULLOID로, Flag는 Used로 초기화 되어 넘어온다. */
    /* BUG-43379
     * smVCPieceHeader의 크기는 slot size로 page를 초기화 시킬 때 포함시킨다.
     * 따라서 allocSlot 할 때는 고려할 필요가 없다. 자세한 내용은 
     * smpVarPageList::initializePageListEntry함수와
     * smpVarPageList::initializePage 함수를 참조. */
    IDE_TEST( smpVarPageList::allocSlot( aTrans,
                                         aHeader->mSpaceID,
                                         aHeader->mSelfOID,
                                         aHeader->mVar.mMRDB,
                                         aTotalVarLen,
                                         aNxtVCPieceOID,
                                         aCurVCPieceOID,
                                         &sPiecePtr,
                                         SM_VCPIECE_TYPE_OTHER,
                                         ID_FALSE ) // BUG-47366 UnitedVar의 Alloc Log 제거
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
            sValueOffset = (UShort) idlOS::align( (UInt) sValueOffset, aColumns[i]->align );    

            sColOffsetPtr[i] = sValueOffset;

            //PROJ-2419 insert UnitedVarPiece FitTest: before mem copy 
            IDU_FIT_POINT( "2.PROJ-2419@smcRecord::insertUnitedVarPiece::memcpy" );

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

    //BUG-43117 : 실제 varPiece길이, header 제외
    *aVarPieceLen4Log = sValueOffset - ID_SIZEOF(smVCPieceHeader);

    //PROJ-2419 insert UnitedVarPiece FitTest: before dirty page 
    IDU_FIT_POINT( "3.PROJ-2419@smcRecord::insertUnitedVarPiece::insDirtyPage" );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( aHeader->mSpaceID,
                                             SM_MAKE_PID(*aCurVCPieceOID))
            != IDE_SUCCESS );

    if ( aAddOIDFlag == ID_TRUE )
    {
        //PROJ-2419 insert UnitedVarPiece FitTest: before add OID 
        IDU_FIT_POINT( "4.PROJ-2419@smcRecord::insertUnitedVarPiece::addOID" );

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
        /* do nothing */
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
 * aRetryInfo    - [IN] Retry Info
 * aForbiddenToRetry - [IN] retry error 올리는거 금지, abort 발생 
 ***********************************************************************/
IDE_RC smcRecord::recordLockValidation(void                  * aTrans,
                                       smSCN                   aViewSCN,
                                       smcTableHeader        * aHeader,
                                       SChar                ** aRow,
                                       ULong                   aLockWaitTime,
                                       UInt                  * aState,
                                       const smiDMLRetryInfo * aRetryInfo,
                                       idBool                  aForbiddenToRetry )
{
    smpSlotHeader * sCurSlotHeaderPtr;
    smpSlotHeader * sPrvSlotHeaderPtr;
    smpSlotHeader * sSlotHeaderPtr;
    scSpaceID       sSpaceID;
    scPageID        sPageID         = SM_NULL_PID;
    scPageID        sNxtPageID;
    smTID           sWaitTransID    = SM_NULL_TID;
    smSCN           sNxtRowSCN;
    smTID           sNxtRowTID      = SM_NULL_TID;
    smTID           sTransID        = SM_NULL_TID;
    smOID           sNextOID        = SM_NULL_OID;
    UInt            sSessionChkCnt  = 0;

    IDE_ASSERT( aHeader != NULL );
    IDE_ASSERT( aTrans  != NULL );
    IDE_ASSERT( aRow    != NULL );
    IDE_ASSERT( aState  != NULL );

    sSpaceID = aHeader->mSpaceID;

    sPrvSlotHeaderPtr = (smpSlotHeader *)*aRow;
    sCurSlotHeaderPtr = sPrvSlotHeaderPtr;

    /* BUG-15642 Record가 두번 Free되는 경우가 발생함.:
     * 이런 경우가 발생하기전에 validateUpdateTargetRow에서
     * 체크하여 문제가 있다면 Rollback시키고 다시 retry를 수행시킴*/
    // 이 함수 내에서는 row의 변하는 부분이 참조되지 않는다.  그렇기 때문에 row의 변하는
    // 부분을 따로 할당하여 함수의인자로 줄 필요가 없다.
    IDE_TEST_RAISE( validateUpdateTargetRow( aTrans,
                                             sSpaceID,
                                             aViewSCN,
                                             sCurSlotHeaderPtr,
                                             aRetryInfo )
                    != IDE_SUCCESS, already_modified );

    sPageID = SMP_SLOT_GET_PID(*aRow);

    sTransID = smLayerCallback::getTransID( aTrans );

    while( ! SM_SCN_IS_FREE_ROW( sCurSlotHeaderPtr->mLimitSCN ) /* aRow가 최신버전인가?*/)
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
                /* 다른 트랜잭션이 lock을 잡았으므로 그가 끝나기를 기다린다. */
            }
        }
        else
        {
            /* check whether the record is already modified. */
            // PROJ-1784 DML without retry
            if( SM_SCN_IS_NOT_INFINITE( sNxtRowSCN ) )
            {
                IDE_TEST_RAISE( aRetryInfo == NULL, already_modified );

                // 바로 처리한다.
                IDE_TEST_RAISE( SM_SCN_IS_DELETED( sNxtRowSCN ), already_modified );

                sNextOID = SMP_SLOT_GET_NEXT_OID( sCurSlotHeaderPtr );

                IDE_ASSERT( SM_IS_VALID_OID( sNextOID ) );

                // Next Row 가 있는 경우 따라간다
                // Next Row의 Page ID가 다르면 Page Latch를 다시 잡는다.
                IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                                   sNextOID,
                                                   (void**)&sCurSlotHeaderPtr )
                            == IDE_SUCCESS );

                sNxtPageID = SMP_SLOT_GET_PID( sCurSlotHeaderPtr );

                IDE_ASSERT( sNxtPageID != SM_NULL_PID );

                if( sPageID != sNxtPageID )
                {
                    *aState = 0;
                    IDE_TEST( smmManager::releasePageLatch( sSpaceID, sPageID )
                              != IDE_SUCCESS );
                    // to Next

                    IDE_TEST(smmManager::holdPageXLatch( sSpaceID, sNxtPageID ) != IDE_SUCCESS);
                    *aState = 1;
                    sPageID = sNxtPageID;
                }
            }

            /* BUG-39233
             * lock wait를 한번 했는데 계속 같은 Tx 가 mLimitSCN을 정리안하고 있는경우 */
            if( (sWaitTransID != SM_NULL_TID) &&
                (sWaitTransID == sNxtRowTID) )
            {
                sNextOID = SMP_SLOT_GET_NEXT_OID( sCurSlotHeaderPtr );

                // Next Row 가 있는 경우 따라간다.
                if( sNextOID != SM_NULL_OID )
                {
                    IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                                       sNextOID,
                                                       (void**)&sSlotHeaderPtr )
                                == IDE_SUCCESS );

                    // next의 mCreateSCN이 commit된 scn이면 mLimitSCN을 강제로 설정한다.
                    if( SM_SCN_IS_NOT_INFINITE( sSlotHeaderPtr->mCreateSCN ) )
                    {
                        // trc log
                        ideLog::log( IDE_SERVER_0,
                                     "recordLockValidation() invalid mLimitSCN\n"
                                     "TID %"ID_UINT32_FMT", "
                                     "Next Row TID %"ID_UINT32_FMT", "
                                     "SCN %"ID_XINT64_FMT"\n"
                                     "RetryInfo : %"ID_XPOINTER_FMT"\n",
                                     sTransID,
                                     sNxtRowTID,
                                     SM_SCN_TO_LONG( sNxtRowSCN ),
                                     aRetryInfo );

                        smpFixedPageList::dumpSlotHeader( sCurSlotHeaderPtr );

                        ideLog::logMem( IDE_SERVER_0,
                                        (UChar*)sCurSlotHeaderPtr,
                                        aHeader->mFixed.mMRDB.mSlotSize );

                        smpFixedPageList::dumpFixedPage( sSpaceID,
                                                         sPageID,
                                                         aHeader->mFixed.mMRDB.mSlotSize );

                        /* debug 모드에서는 assert 시키고,
                         * release 모드에서는 mLimitSCN 설정 후, already_modified로 fail 시킴 */
                        IDE_DASSERT( 0 );

                        // mLimitSCN을 설정
                        SM_SET_SCN( &(sCurSlotHeaderPtr->mLimitSCN), &(sSlotHeaderPtr->mCreateSCN) );

                        // set dirty page
                        IDE_TEST( smmDirtyPageMgr::insDirtyPage( sSpaceID, sPageID )
                                  != IDE_SUCCESS );

                        IDE_RAISE( already_modified );
                    }
                }
            }
            else
            {
                /* do nothing */
            }
        }//else

        sWaitTransID = sNxtRowTID;

        *aState = 0;
        IDE_TEST( smmManager::releasePageLatch( sSpaceID, sPageID )
                  != IDE_SUCCESS );

        IDU_FIT_POINT_RAISE( "1.BUG-39168@smcRecord::recordLockValidation",
                              abort_timeout );
        /* BUG-39168 본 함수에서 무한루프에 빠진 현상으로 인하여
         * timeout 을 확인하는 code 추가합니다. */
        sSessionChkCnt++;
        IDE_TEST_RAISE( iduCheckSessionEvent( smxTrans::getStatistics( aTrans ) )
                        != IDE_SUCCESS, abort_timeout );

        /* Next Version의 Transaction이 끝날때까지 기다린다. */
        IDE_TEST( smLayerCallback::waitLockForTrans( aTrans,
                                                     sWaitTransID,
                                                     sSpaceID,
                                                     aLockWaitTime )
                  != IDE_SUCCESS );

        IDE_TEST(smmManager::holdPageXLatch(sSpaceID, sPageID) != IDE_SUCCESS);
        *aState = 1;
    } // end of while

    IDU_FIT_POINT_RAISE( "smcRecord::recordLockValidation::isSameColumnValue", already_modified );
    if( sCurSlotHeaderPtr != sPrvSlotHeaderPtr )
    {
        IDE_ASSERT( aRetryInfo != NULL );
        IDE_ASSERT( aRetryInfo->mIsRowRetry == ID_FALSE );

        // Cur과 prv를 비교한다.
        if( aRetryInfo->mStmtRetryColLst != NULL )
        {
            IDE_TEST_RAISE( isSameColumnValue( sSpaceID,
                                               aRetryInfo->mStmtRetryColLst,
                                               sPrvSlotHeaderPtr,
                                               sCurSlotHeaderPtr )
                            == ID_FALSE , already_modified );
        }

        if( aRetryInfo->mRowRetryColLst != NULL )
        {
            IDE_TEST_RAISE( isSameColumnValue( sSpaceID,
                                               aRetryInfo->mRowRetryColLst,
                                               sPrvSlotHeaderPtr,
                                               sCurSlotHeaderPtr )
                            == ID_FALSE , need_row_retry );
        }
    }

    *aRow = (SChar*)sCurSlotHeaderPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( abort_timeout );
    {
        /* BUG-47457
         * 첫번째loop에서는, 이 함수까지는 오는중 session out이 될 수 있다.
         * 두번째loop에서는, 첫번째 loop 마지막의 page x latch 잡느라고 session out 될 수 있다.
         * 세번째loop부터는 일반적이지 않은 경우이다. TRACE LOG를 남긴다. */
        if ( sSessionChkCnt >= 3 )
        {
            /* BUG-39168 본 함수에서 무한루프에 빠진 현상으로 인하여
             * timeout 을 확인하는 code 추가합니다. */
            ideLog::log( IDE_SERVER_0,
                         "recordLockValidation() timeout\n"
                         "TID %"ID_UINT32_FMT", "
                         "Next Row TID %"ID_UINT32_FMT", "
                         "SCN %"ID_XINT64_FMT"\n"
                         "RetryInfo : %"ID_XPOINTER_FMT"\n",
                         sTransID,
                         sNxtRowTID,
                         SM_SCN_TO_LONG( sNxtRowSCN ),
                         aRetryInfo );

            smpFixedPageList::dumpSlotHeader( sCurSlotHeaderPtr );

            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)sCurSlotHeaderPtr,
                            aHeader->mFixed.mMRDB.mSlotSize );

            smpFixedPageList::dumpFixedPage( sSpaceID,
                                             sPageID,
                                             aHeader->mFixed.mMRDB.mSlotSize );
        }
    }
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
                             "TableOID:%"ID_vULONG_FMT", "
                             "ViewSCN:%"ID_UINT64_FMT", "
                             "CreateSCN:%"ID_UINT64_FMT", "
                             "LimitSCN:%"ID_UINT64_FMT,
                             sSpaceID,
                             aHeader->mSelfOID,
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
idBool smcRecord::isSameColumnValue( scSpaceID               aSpaceID,
                                     const smiColumnList   * aChkColumnList,
                                     smpSlotHeader         * aPrvSlotHeaderPtr,
                                     smpSlotHeader         * aCurSlotHeaderPtr )
{
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;

    SChar           * sPrvRowPtr = (SChar*)aPrvSlotHeaderPtr;
    SChar           * sCurRowPtr = (SChar*)aCurSlotHeaderPtr;
    SChar           * sPrvValue  = NULL;
    SChar           * sCurValue  = NULL;
    smVCDesc        * sPrvVCDesc;
    smVCDesc        * sCurVCDesc;
    smOID             sPrvVCPieceOID;
    smOID             sCurVCPieceOID;
    smVCPieceHeader * sPrvVCPieceHeader = NULL;
    smVCPieceHeader * sCurVCPieceHeader = NULL;
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

                if ( idlOS::memcmp( sCurRowPtr + sColumn->offset,
                                    sPrvRowPtr + sColumn->offset,
                                    sColumn->size ) != 0 )
                {
                    sIsSameColumnValue = ID_FALSE;
                }
                else
                {
                    /* do nothing */
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

            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                /* PROJ-2419 */
                sPrvVCDesc = getVCDesc( sColumn, sPrvRowPtr );
                sCurVCDesc = getVCDesc( sColumn, sCurRowPtr );

                if ( sPrvVCDesc->length != sCurVCDesc->length )
                {
                    sIsSameColumnValue = ID_FALSE;
                    break;
                }
                else
                {
                    /* do nothing */
                }

                // 같아야 하는지 확인
                IDE_ASSERT( ( sPrvVCDesc->flag & SM_VCDESC_MODE_MASK ) ==
                            ( sCurVCDesc->flag & SM_VCDESC_MODE_MASK ) );

                if ( sCurVCDesc->length != 0 )
                {
                    if ( SM_VCDESC_IS_MODE_IN( sCurVCDesc) )
                    {
                        if ( idlOS::memcmp( (smVCDescInMode*)sPrvVCDesc + 1,
                                            (smVCDescInMode*)sCurVCDesc + 1,
                                            sCurVCDesc->length ) != 0)
                        {
                            sIsSameColumnValue = ID_FALSE;
                            break;
                        }
                        else
                        {
                            /* do nothing */
                        }
                    }
                    else
                    {
                        IDE_DASSERT( ( sCurVCDesc->flag & SM_VCDESC_MODE_MASK ) == SM_VCDESC_MODE_OUT );

                        if ( sPrvVCDesc->fstPieceOID == sCurVCDesc->fstPieceOID )
                        {
                            // update가 없었다
                            break;
                        }
                        else
                        {
                            /* do nothing */
                        }

                        sPrvVCPieceOID = sPrvVCDesc->fstPieceOID;
                        sCurVCPieceOID = sCurVCDesc->fstPieceOID;

                        while ( ( sPrvVCPieceOID != SM_NULL_OID ) &&
                                ( sCurVCPieceOID != SM_NULL_OID ) )
                        {
                            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                                               sPrvVCPieceOID,
                                                               (void**)&sPrvVCPieceHeader )
                                        == IDE_SUCCESS );

                            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                                               sCurVCPieceOID,
                                                               (void**)&sCurVCPieceHeader )
                                        == IDE_SUCCESS );

                            if ( sPrvVCPieceHeader->length != sCurVCPieceHeader->length )
                            {
                                sIsSameColumnValue = ID_FALSE;
                                break;
                            }
                            else
                            {
                                /* do nothing */
                            }

                            if ( idlOS::memcmp( sPrvVCPieceHeader + 1,
                                                sCurVCPieceHeader + 1,
                                                sCurVCPieceHeader->length ) != 0 )
                            {
                                sIsSameColumnValue = ID_FALSE;
                                break;
                            }
                            else
                            {
                                /* do nothing */
                            }

                            sPrvVCPieceOID = sPrvVCPieceHeader->nxtPieceOID;
                            sCurVCPieceOID = sCurVCPieceHeader->nxtPieceOID;
                        }

                        if ( sPrvVCPieceOID != sCurVCPieceOID )
                        {
                            // 둘 중 하나가 null이 아닌 경우
                            sIsSameColumnValue = ID_FALSE;
                            break;
                        }
                        else
                        {
                            /* do nothing */
                        }
                    }
                }
                else
                {
                    /* do nothing */
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

                if ( SM_VCDESC_IS_MODE_IN( sCurLobDesc ) )
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
 *  3. 만약 Row의 SCN이 무한대라면, Row의 TID는 이 연산을 수행하는
 *     Transaction과 동일해야 한다.
 *  4. 무한대가 아니라면 Row의 SCN은 반드시 aViewSCN보다 작아야 한다.
 *
 *  aRowPtr   - [IN] Row Pointer
 *  aSpaceID  - [IN] Row의 Table이 속한 Tablespace의 ID
 *  aViewSCN  - [IN] View SCN
 *  aRowPtr   - [IN] 검증을 수행할 Row Pointer
 *  aRetryInfo- [IN] Retry Info
 *
 ***********************************************************************/
 IDE_RC smcRecord::validateUpdateTargetRow( void                  * aTrans,
                                            scSpaceID               aSpaceID,
                                            smSCN                   aViewSCN,
                                            void                  * aRowPtr,
                                            const smiDMLRetryInfo * aRetryInfo )
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
                /* PROJ-2694 Fetch Across Rollback */
                if ( ( sTrans->mCursorOpenInfSCN != SM_SCN_INIT ) &&
                     ( SM_SCN_IS_LT( &sRowSCN, &(sTrans->mCursorOpenInfSCN) ) == ID_TRUE ) )
                {
                    /* holdable cursor open 이전 자신이 생성한 row에 접근했을 경우
                     * 해당 Tx를 rollback시 cursor를 재활용할 수 없다. */
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
                     "TID:[%u] View SCN:[%llx]\n",
                     smLayerCallback::getTransID( aTrans ),
                     SM_SCN_TO_LONG( aViewSCN ) );

        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     "[##WARNING ##WARNING!!] Target row's info\n");
        logSlotInfo(aRowPtr);

        sRowOID = SMP_SLOT_GET_NEXT_OID( sSlotHeader );

        if( SM_IS_VALID_OID( sRowOID ) )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                         "[##WARNING ##WARNING!!] Next row of target row info\n");
            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                               sRowOID,
                                               (void**)&sLogSlotPtr )
                        == IDE_SUCCESS );
            logSlotInfo( sLogSlotPtr );
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
IDE_RC smcRecord::deleteVC( void              *aTrans,
                            smcTableHeader    *aHeader,
                            smOID              aFstOID,
                            idBool             aLogging )  
{
    smOID             sVCPieceOID       = aFstOID;
    smOID             sNxtVCPieceOID    = SM_NULL_OID;
    SChar            *sVCPiecePtr       = NULL;

    /* VC가 여러개의 VC Piece구성될 경우 모든
       Piece에 대해서 삭제 작업을 수행한다. */
    while ( sVCPieceOID != SM_NULL_OID )
    {
        IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID, 
                                           sVCPieceOID,
                                           (void**)&sVCPiecePtr )
                == IDE_SUCCESS );

        sNxtVCPieceOID = ((smVCPieceHeader*)sVCPiecePtr)->nxtPieceOID;

        //PROJ-2419 delete VC
        IDU_FIT_POINT( "1.PROJ-2419@smcRecord::deleteVC::setFreeFlagAtVCPieceHdr" );

        IDE_TEST( setFreeFlagAtVCPieceHdr(aTrans,
                                          aHeader->mSelfOID,
                                          aHeader->mSpaceID,
                                          sVCPieceOID,
                                          sVCPiecePtr,
                                          SM_VCPIECE_FREE_OK,
                                          aLogging)
                != IDE_SUCCESS);

        //PROJ-2419 delete VC
        IDU_FIT_POINT( "2.PROJ-2419@smcRecord::deleteVC::addOID" );

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
 * Description : aRow의 aColumn이 가리키는 Lob Column을 지운다.
 *
 * aTrans    - [IN] Transaction Pointer
 * aHeader   - [IN] Table Header
 * aColumn   - [IN] Table Column List
 * aRow      - [IN] Row Pointer
 ***********************************************************************/
IDE_RC smcRecord::deleteLob(void              *aTrans,
                            smcTableHeader    *aHeader,
                            const smiColumn   *aColumn,
                            SChar             *aRow,
                            idBool             aLogging)  //BUG-46854
{
    smVCDesc     *sVCDesc;
    smcLobDesc   *sCurLobDesc;

    IDE_ASSERT( aHeader != NULL );
    IDE_ASSERT( aColumn != NULL );
    IDE_ASSERT( aRow    != NULL );
    IDE_ASSERT_MSG( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                    == SMI_COLUMN_TYPE_LOB,
                    "flag : %"ID_UINT32_FMT"\n",
                    aColumn->flag );

    sCurLobDesc = (smcLobDesc*)(aRow + aColumn->offset);

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
        IDE_TEST(deleteVC(aTrans, aHeader, sVCDesc->fstPieceOID, aLogging)
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
 * aRowPtr의 SlotHeader정보를 boot log에 남긴다.
 * aRow의 mTID와 mNext는 언제든지 변할 수 있으므로, 변하지 않는 값을 인자로
 * 따로 받는다.
 * aRow - [IN]: Row Pointer
 ***********************************************************************/
void smcRecord::logSlotInfo(const void *aRow)
{
    smpSlotHeader *sSlotHeader;
    scPageID       sPageID;
    smOID          sRecOID;
    smSCN          sSCN;
    smTID          sTID;
    smOID          sNextOID;

    sSlotHeader = (smpSlotHeader *)aRow;
    sPageID = SMP_SLOT_GET_PID(aRow);

    sRecOID = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( sSlotHeader ) );

    SMX_GET_SCN_AND_TID( sSlotHeader->mCreateSCN, sSCN, sTID );
    sNextOID = SMP_SLOT_GET_NEXT_OID( sSlotHeader );

    ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                "TID:[%u] "
                "Row Info: OID [%lu] "
                "SCN:[%llx],"
                "Next:[%lu],"
                "Flag:[%llx]\n",
                sTID,
                sRecOID,
                SM_SCN_TO_LONG( sSCN ),
                sNextOID,
                SMP_SLOT_GET_FLAGS( sSlotHeader ) );
}

/***********************************************************************
 * description : aRow에서 aColumn이 가리키는 vc의 Header를 구한다.
 *
 * aRow             -   [in] fixed row pointer
 * aColumn          -   [in] column desc
 * aVCPieceHeader   -  [out] return VCPieceHeader pointer
 * aOffsetIdx       -  [out] return OffsetIdx in vc piece
 ***********************************************************************/
IDE_RC smcRecord::getVCPieceHeader( const void      *  aRow,
                                    const smiColumn *  aColumn,
                                    smVCPieceHeader ** aVCPieceHeader,
                                    UShort          *  aOffsetIdx )
{
    UShort            sOffsetIdx        = ID_USHORT_MAX;
    smVCPieceHeader * sVCPieceHeader    = NULL;
    smOID             sOID              = SM_NULL_OID;

    IDE_ASSERT( aRow    != NULL );
    IDE_ASSERT( aColumn != NULL );

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

/***********************************************************************
 * Description: Variable Length Data를 읽는다. 만약 길이가 최대 Piece길이 보다
 *              작고 읽어야 할 Piece가 하나의 페이지에 존재한다면 읽어야 할 위치의
 *              첫번째 바이트가 위치한 메모리 포인터를 리턴한다. 그렇지 않을 경우 Row의
 *              값을 복사해서 넘겨준다.
 *
 * aRow         - [IN] 읽어들일 컴럼을 가지고 있는 Row Pointer
 * aColumn      - [IN] 읽어들일 컬럼의 정보
 * aFstPos      - [IN] 읽어들인 Row의 첫번째 위치
 * aLength      - [IN-OUT] IN : IsReturnLength == ID_FALSE
 *                         OUT: isReturnLength == ID_TRUE
 * aBuffer      - [IN] value가 복사됨.
 *
 ***********************************************************************/
SChar* smcRecord::getVarRow( const void      * aRow,
                             const smiColumn * aColumn,
                             UInt              aFstPos,
                             UInt            * aLength,
                             SChar           * aBuffer,
                             idBool            aIsReturnLength )
{
    smVCPieceHeader * sVCPieceHeader    = NULL;
    SChar           * sRet              = NULL;
    UShort          * sCurrOffsetPtr    = NULL;
    UShort            sOffsetIdx        = 0;

    IDE_ASSERT( aRow    != NULL );
    IDE_ASSERT( aColumn != NULL );
    IDE_ASSERT_MSG( ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_VARIABLE ) ||
                    ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_VARIABLE_LARGE ),
                    "flag : %"ID_UINT32_FMT"\n",
                    aColumn->flag );
    IDE_ASSERT_MSG( ( aColumn->flag & SMI_COLUMN_STORAGE_MASK )
                    == SMI_COLUMN_STORAGE_MEMORY,
                    "flag : %"ID_UINT32_FMT"\n",
                    aColumn->flag );

    if ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK ) != SMI_COLUMN_TYPE_VARIABLE_LARGE )
    {    /* United Var 는 항상 aIsReturnLength에 관계 없이 length 를 리턴해도된다.  */
        if (( (smpSlotHeader*)aRow)->mVarOID == SM_NULL_OID )
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
    }
    else
    {
        sRet = getVarLarge( aRow,
                            aColumn,
                            aFstPos,
                            aLength,
                            aBuffer,
                            aIsReturnLength );
    }

    return sRet;
}

SChar* smcRecord::getVarLarge( const void      * aRow,
                               const smiColumn * aColumn,
                               UInt              aFstPos,
                               UInt            * aLength,
                               SChar           * aBuffer,
                               idBool            aIsReturnLength )
{
    smVCDesc * sVCDesc;
    SChar    * sRow;
    SChar    * sRet;
    
    sRow    = (SChar*)aRow + aColumn->offset;
    sVCDesc = (smVCDesc*)sRow;

    /* 32k 넘는 Large var value 를 잘라서 읽는 경우에는
     * isReturnLength 가 FALSE 로 들어온다 */
    if ( aIsReturnLength == ID_TRUE )
    {
        *aLength = sVCDesc->length;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sVCDesc->length != 0 )
    {
        if ( SM_VCDESC_IS_MODE_IN( sVCDesc) )
        {
            /* Varchar InMode저장시*/
            sRet = (SChar*)sRow + ID_SIZEOF( smVCDescInMode );
        }
        else
        {
            /* Varchar OutMode저장시*/
            sRet = smpVarPageList::getValue( aColumn->colSpace,
                                             aFstPos,
                                             *aLength,
                                             sVCDesc->fstPieceOID,
                                             aBuffer );
        }
    }
    else
    {
        /* Nothing to do */
        sRet = NULL;
    }
    return sRet;
}

/***********************************************************************
 * description : arow에서 acolumn이 가리키는 value의 length를 구한다.
 *
 * aRowPtr  - [in] fixed row pointer
 * aColumn  - [in] column desc
 ***********************************************************************/
UInt smcRecord::getVarColumnLen( const smiColumn    * aColumn,
                                 const SChar        * aRowPtr )
{
    smVCPieceHeader * sVCPieceHeader = NULL;
    smVCDesc        * sVCDesc        = NULL;
    UShort          * sCurrOffsetPtr = NULL;
    UShort            sLength        = 0;
    UShort            sOffsetIdx     = ID_USHORT_MAX;

    IDE_DASSERT( ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE) ||
                 ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE_LARGE) );

    if ( (aColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE_LARGE )
    {
        sVCDesc = (smVCDesc*)(aRowPtr + aColumn->offset);
        sLength = sVCDesc->length;
    }
    else
    {
        if (( (smpSlotHeader*)aRowPtr)->mVarOID == SM_NULL_OID )
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
UInt smcRecord::getColumnLen( const smiColumn * aColumn,
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
        case SMI_COLUMN_TYPE_VARIABLE_LARGE:
            sLength = getVarColumnLen( aColumn, (const SChar*)aRowPtr );
            break;

        case SMI_COLUMN_TYPE_FIXED:
            sLength = aColumn->size;
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return sLength;
}

UInt smcRecord::getUnitedVCLogSize( const smcTableHeader* aHeader, smOID aOID )
{
    UInt                sUnitedVCLogSize    = 0;
    UInt                sVCSize             = 0;
    smVCPieceHeader   * sVCPieceHeader      = NULL;
    smOID               sOID                = aOID;

    sUnitedVCLogSize += ID_SIZEOF(smOID);                         /* First Piece OID size */

    sUnitedVCLogSize += ID_SIZEOF(UShort);                        /* Column Count size */

    sUnitedVCLogSize += ID_SIZEOF(UInt) * aHeader->mUnitedVarColumnCount; /* Column id list size */

    IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                       sOID,
                                       (void**)&sVCPieceHeader)
            == IDE_SUCCESS );

    while ( sVCPieceHeader != NULL )
    {
        // end offset 변수로
        sVCSize = *((UShort*)(sVCPieceHeader + 1) + sVCPieceHeader->colCount) - ID_SIZEOF(smVCPieceHeader);

        sUnitedVCLogSize    += sVCSize;         /* value size */

        /* Next OID, Column count in this piece */
        sUnitedVCLogSize    += ID_SIZEOF(smOID) + ID_SIZEOF(UShort);

        if ( sVCPieceHeader->nxtPieceOID != SM_NULL_OID )
        {
            sOID    = sVCPieceHeader->nxtPieceOID;

            IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                               sOID,
                                               (void**)&sVCPieceHeader)
                    == IDE_SUCCESS );
        }
        else
        {
            sVCPieceHeader = NULL;
        }
    }

    return sUnitedVCLogSize;
}

UInt smcRecord::getUnitedVCColCount( scSpaceID aSpaceID, smOID aOID )
{
    UInt                sRet            = 0;
    smVCPieceHeader   * sVCPieceHeader  = NULL;
    smOID               sOID            = aOID;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       sOID,
                                       (void**)&sVCPieceHeader)
            == IDE_SUCCESS );

    while ( sVCPieceHeader != NULL )
    {
        sRet += sVCPieceHeader->colCount;

        if ( sVCPieceHeader->nxtPieceOID != SM_NULL_OID )
        {
            sOID    = sVCPieceHeader->nxtPieceOID;

            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                               sOID,
                                               (void**)&sVCPieceHeader)
                    == IDE_SUCCESS );
        }
        else
        {
            sVCPieceHeader = NULL;
        }
    }

    return sRet;

}
