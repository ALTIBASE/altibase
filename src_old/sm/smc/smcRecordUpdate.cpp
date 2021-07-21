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
 * $Id: smcRecordUpdate.cpp 90089 2021-02-26 06:29:49Z jiwon.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smcReq.h>
#include <smcRecordUpdate.h>
#include <sctTableSpaceMgr.h>
#include <sgmManager.h>

/***********************************************************************
 * Description : DML중 Insert시 발생하는 Log를 기록한다. 이 Insert로그는
 *               만약 이 Table에 Replication이 걸려 있을 경우 Replication
 *               Sender가 읽어 들인다. 그리고 이런 Replication을 위해서
 *               Fixed Row와 Variable Log가 각기 다른 Page영역에 저장되지만
 *               하나의 DML 로그로 기록한다.
 *
 * Type       :  SMR_SMC_PERS_INSERT_ROW
 *
 * LOG HEADER :  smrUpdateLog
 * BODY       :  After Image : Fixed Row와 Variable Column에 대한 Log를 기록.
 *                   Fixed Row Size(UShort) + Fixed Row Data
 *                              + VCLOG(1) + VCLOG(2) ... + VCLOG (n)
 *                              NULL_OID + NULL_OID
 *
 *               VCLOG       : Variable/LOB Column당 하나씩 생긴다.
 *
 *                    1. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_OUT
 *                      - Column ID(UInt) | Length(UInt) | Value | OID List
 *
 *                    2. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_IN
 *                      - None (After Image인 경우는 Fixed Row에 In Mode로 저장되고
 *                        또한 Fixed Row에 대한 Logging을 별도로 수행하기 때문에
 *                        VC에 대한 Logging이 불필요.
 *
 *               NULL_OID    : SMR_SMC_PERS_UPDATE_VERSION_ROW와 AfterImage 포맷
 *                             을 동일하게 맞추기 위해 추가
 *
 * aTrans          - [IN] Transaction Pointer
 * aHeader         - [IN] Table Header Pointer
 * aFixedRow       - [IN] Fixed Row Pointer
 * aAfterImageSize - [IN] After Image Size
 * aVarColumnCnt   - [IN] Variable Column Count
 * aArrVarColumn   - [IN] Variable Column Desc.
 *
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeInsertLog( void*             aTrans,
                                        smcTableHeader*   aHeader,
                                        SChar*            aFixedRow,
                                        UInt              aAfterImageSize,
                                        UShort            aVarColumnCnt,
                                        const smiColumn** aVarColumns,
                                        UInt              aLargeVarCnt,
                                        const smiColumn** aLargeVarColumn )
{
    UInt         i              = 0;
    UInt         sLogSize       = 0;
    UInt         sOffset        = 0;
    UShort       sVCSize        = 0;
    smOID        sVCPieceOID    = SM_NULL_OID;
    smOID        sNullOID       = SM_NULL_OID;

    smrUpdateLog sUpdateLog;
    scPageID     sPageID;
    UShort       sAfterFixRowSize; /* Fixed Row Size: 반드시 UShort이어야 한다.*/

    const smiColumn   * sCurColumn;
    smVCDesc          * sCurVarDesc;
    UInt                sLogFlag;
    smrLogType          sLogType;
    smVCPieceHeader   * sVCPieceHeader  = NULL;

    sPageID = SMP_SLOT_GET_PID(aFixedRow);

    /* Insert Row에 대하여 추가의 로그 헤더 정보 구성 */
    sLogSize = SMR_LOGREC_SIZE(smrUpdateLog) + ID_SIZEOF(smrLogTail) + aAfterImageSize;

    /* Insert한 Row에 대하여 로그 헤더 정보 구성 */
    smrLogHeadI::setType(&sUpdateLog.mHead, SMR_LT_UPDATE);
    smrLogHeadI::setTransID( &sUpdateLog.mHead, smLayerCallback::getTransID( aTrans ) );

    // To Fix PR-14581
    // sLogFlag = smrLogHeadI::getFlag(&sUpdateLog.mHead);
    IDE_TEST( makeLogFlag(aTrans,
                          aHeader,
                          SMR_SMC_PERS_INSERT_ROW,
                          SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK,
                          &sLogFlag)
              != IDE_SUCCESS );

    /* makeLogFlag가 설정되이후에 호출되어야 한다. 왜냐면 안에서
     * smLayerCallback::getLstReplStmtDepth가 return하는 값을 변경한다.*/
    smrLogHeadI::setReplStmtDepth( &sUpdateLog.mHead,
                                   smLayerCallback::getLstReplStmtDepth( aTrans ) );

    smrLogHeadI::setFlag(&sUpdateLog.mHead, sLogFlag);
    SC_MAKE_GRID( sUpdateLog.mGRID,
                  aHeader->mSpaceID,
                  sPageID,
                  SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aFixedRow ) );

    /* set table oid */
    sUpdateLog.mData        = aHeader->mSelfOID;
    sUpdateLog.mType        = SMR_SMC_PERS_INSERT_ROW;
    sUpdateLog.mBImgSize    = 0;

    smrLogHeadI::setSize(&sUpdateLog.mHead, sLogSize);
    sUpdateLog.mAImgSize   = (UInt)aAfterImageSize;

    smrLogHeadI::setPrevLSN( &sUpdateLog.mHead, smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    /* insertVersion에 대한 로깅 수행
          Log Structure <UpdateLogHead, FixedImage, VarImages> */

    /* fixed 영역에 대한 로깅 */
    sAfterFixRowSize = (UShort)(aHeader->mFixed.mMRDB.mSlotSize - SMP_SLOT_HEADER_SIZE);

    sOffset = 0;

    /* Log Header를 Transaction Log Buffer에 기록 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sUpdateLog, /* Update Log Header */
                                                 sOffset,
                                                 SMR_LOGREC_SIZE(smrUpdateLog) )
              != IDE_SUCCESS );
    sOffset += SMR_LOGREC_SIZE(smrUpdateLog);

    /* smpSlotHeader를 제외한 Fixed Row Data영역 크기 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sAfterFixRowSize, /* Fixed Row Size */
                                                 sOffset,
                                                 ID_SIZEOF(UShort) )
              != IDE_SUCCESS );
    sOffset += ID_SIZEOF(UShort);

    /* smpSlotHeader를 제외한 Fixed Row Data영역 After Image Logging */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 aFixedRow + SMP_SLOT_HEADER_SIZE,
                                                 sOffset,
                                                 sAfterFixRowSize )
              != IDE_SUCCESS );
    sOffset += sAfterFixRowSize;

    /* proj-2419 united variable column logging */
    sVCPieceOID = ((smpSlotHeader*)aFixedRow)->mVarOID;

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sVCPieceOID, /* first OID of united var piece */
                                                 sOffset,
                                                 ID_SIZEOF(smOID) )
              != IDE_SUCCESS );
    sOffset += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &aVarColumnCnt, /* Column count in United var piece */
                                                     sOffset,
                                                     ID_SIZEOF(UShort) ) 
                  != IDE_SUCCESS );
        sOffset += ID_SIZEOF(UShort);

        for ( i = 0 ; i < aVarColumnCnt ; i++ )
        {
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         &(aVarColumns[i]->id), /* Column IDs in united var piece */
                                                         sOffset,
                                                         ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );
            sOffset += ID_SIZEOF(UInt);
        }

        if ( aVarColumnCnt > 0)
        {

            /* Variable Column Value 기록. */
            while ( sVCPieceOID != SM_NULL_OID) //avarcolumncnt != 0 조건 추가
            {
                IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                                   sVCPieceOID,
                                                   (void**)&sVCPieceHeader )
                            == IDE_SUCCESS );

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->nxtPieceOID),
                                                             sOffset,
                                                             ID_SIZEOF(smOID) )
                          != IDE_SUCCESS );
                sOffset += ID_SIZEOF(smOID);


                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->colCount),
                                                             sOffset,
                                                             ID_SIZEOF(UShort) )
                          != IDE_SUCCESS );
                sOffset += ID_SIZEOF(UShort);

                sVCSize = getUnitedVCSize( sVCPieceHeader );

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             (SChar*)(sVCPieceHeader + 1),
                                                             sOffset,
                                                             (UInt)sVCSize )
                          != IDE_SUCCESS );

                sOffset += (UInt)sVCSize;

                sVCPieceOID = sVCPieceHeader->nxtPieceOID;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 각각의 Lob, Large Var Column에 대해서 차례차례 Transaction Log Buffer에 기록한다. */
    for ( i = 0; i < aLargeVarCnt; i++ )
    {
        sCurColumn  = aLargeVarColumn[i];

        sCurVarDesc = (smVCDesc*)( aFixedRow + sCurColumn->offset );

        /*
          1. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_OUT
          - Column ID(UInt) | Length(UInt) | OID List | Value

          2. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_IN
          - None (After Image인 경우는 Fixed Row에 In Mode로 저장되고 또한 Fixed
          Row에 대한 Logging을 별도로 수행하기 때문에 VC에 대한 Logging이 불필요.

        */
        IDE_TEST( smcRecordUpdate::writeVCLog4MVCC(
                      aTrans,
                      sCurColumn,
                      sCurVarDesc,
                      &sOffset,
                      SMC_VC_LOG_WRITE_TYPE_AFTERIMG)
                  != IDE_SUCCESS );
    }

    /* TASK-4690, BUG-32319 [sm-mem-collection] The number of MMDB update log
     *                      can be reduced to 1.
     * Next OID를 기록한다. InsertVersion시에는 NULL_OID
     * insertVersion과 updateVersion의 redo함수는 동일하다.
     * update 쪽과 맞춰주기 위해서 oid도 함께 기록 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sNullOID,
                                                 sOffset,
                                                 ID_SIZEOF(smOID) )
              != IDE_SUCCESS );
    sOffset += ID_SIZEOF(smOID);

    /* Log Tail을 Transaciton Log Buffer에 기록 */
    sLogType = smrLogHeadI::getType(&sUpdateLog.mHead);
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sLogType,
                                                 sOffset,
                                                 ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );
    smrLogHeadI::setType(&sUpdateLog.mHead, sLogType);

    IDE_TEST( smLayerCallback::writeTransLog( aTrans, sUpdateLog.mData ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SMR_SMC_PERS_INSERT_ROW 의 Update 로그의 Redo Function.
 *
 * After Image : Fixed Row와 Variable Column에 대한 Log를 기록.
 *               Fixed Row Size(UShort) + Fixed Row Data
 *                             + VCLOG(1) + VCLOG(2) ... + VCLOG (n)
 *
 *               VCLOG       : Variable/LOB Column당 하나씩 생긴다.
 *                   Column ID(UInt) | LENGTH(UInt) | Value | OID Cnt
 *                  | OID(1) OID(2) ... OID(n)
 * Befor Image : None
 *
 * aTID      - [IN] Transaction ID
 * aPID      - [IN] Page ID
 * aOffset   - [IN] Offset
 * aData     - [IN] Table ID
 * aImage    - [IN] After Image
 * aSize     - [IN] After Image Size
 * aFlag     - [IN] None
 ***********************************************************************/
IDE_RC smcRecordUpdate::redo_SMC_PERS_INSERT_ROW(smTID      aTID,
                                                 scSpaceID  aSpaceID,
                                                 scPageID   aPID,
                                                 scOffset   aOffset,
                                                 vULong     aData,
                                                 SChar     *aImage,
                                                 SInt       aSize,
                                                 UInt      /*aFlag*/)
{
    UInt              i;
    smpSlotHeader    *sNewSlotHeader;
    smpSlotHeader    *sOldSlotHeader;
    smOID             sTableOID = (smOID) aData;

    SChar            *sNewFixRow        = NULL;
    SChar            *sOldFixRow        = NULL;
    smOID             sNewFixOID        = SM_NULL_OID;
    ULong             sOldFixOID        = SM_NULL_OID;
    smVCPieceHeader  *sVCPieceHeader    = NULL;
    UShort            sFixRowSize       = 0;
    SChar            *sAftImage         = NULL;
    SChar            *sFence            = NULL;
    SChar            *sVCPieceValuePtr  = NULL;
    SChar            *sVCPiecePtr       = NULL;
    smOID             sVCPieceOID       = SM_NULL_OID;
    smOID             sNxtPieceOID      = SM_NULL_OID;
    UInt              sVCSize           = 0;
    UInt              sVCPartSize       = 0;
    UShort            sVCPieceSize      = 0;
    UInt              sVCPieceCnt       = 0;
    UShort            sVarColumnCnt     = 0;
    void             *sTransPtr         = NULL;

    SChar            *sBfrImage         = NULL;

    sTransPtr = smLayerCallback::getTransByTID( aTID );

    sAftImage = aImage;

    /* BUG-14513: Insert Log가 Alloc Slot에 대한 Redo수행 */
    sNewFixOID = SM_MAKE_OID( aPID, aOffset );
    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       sNewFixOID,
                                       (void**)&sNewFixRow )
                == IDE_SUCCESS );

    sNewSlotHeader = (smpSlotHeader*)sNewFixRow;
    SM_INIT_SCN( &(sNewSlotHeader->mCreateSCN) );
    SM_SET_SCN_FREE_ROW( &(sNewSlotHeader->mLimitSCN) );
    SMP_SLOT_SET_OFFSET( sNewSlotHeader, aOffset );
    SMP_SLOT_SET_USED( sNewSlotHeader );

    if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                sTableOID,
                                                SM_MAKE_OID(aPID, aOffset) /* Record ID */,
                                                aSpaceID,
                                                SM_OID_NEW_INSERT_FIXED_SLOT )
                  != IDE_SUCCESS );
    }

    /* Get Fixed Row Size : USHORT */
    idlOS::memcpy(&sFixRowSize, sAftImage, ID_SIZEOF(UShort));
    sAftImage += ID_SIZEOF(UShort);

    /* Fix Row Data영역 Redo */
    idlOS::memcpy( sNewFixRow + SMP_SLOT_HEADER_SIZE,
                   sAftImage,
                   sFixRowSize);
    sAftImage += sFixRowSize;

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    /* PROJ-2419 united variable column redo */
    idlOS::memcpy(&sVCPieceOID, sAftImage, ID_SIZEOF(smOID));

    sAftImage += ID_SIZEOF(smOID);

    sNewSlotHeader->mVarOID = sVCPieceOID;

    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* get Column count in united var piece */
        idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));

        sAftImage += ID_SIZEOF(UShort);

        /* update 에서 united var가 전혀 변경되지 않는 경우 */
        if ( sVarColumnCnt > 0 )
        {
            /* skip column IDs */
            sAftImage += ID_SIZEOF(UInt) * sVarColumnCnt;

            /* redo united var pieces*/
            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );
                /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
                   수행되어야 한다.*/
                if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
                {
                    /* for global transaction, add OID into OID_List */
                    IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                            sTableOID,
                                                            sVCPieceOID,
                                                            aSpaceID,
                                                            SM_OID_NEW_VARIABLE_SLOT )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }

                /* get next piece oid */
                idlOS::memcpy(&sNxtPieceOID, sAftImage, ID_SIZEOF(smOID));

                sAftImage += ID_SIZEOF(smOID);

                /* get column count in this piece */
                idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));

                sAftImage += ID_SIZEOF(UShort);

                /* get VC Piece Size
                 * 이 값은 value에 달린 array 의 끝에 있으므로 해당 값만 가져온후
                 * sAftImage를 손대지 않고 value 복사할때 전부 한번에 다시 복사한다 */
                idlOS::memcpy( &sVCPieceSize,
                               sAftImage + ( ID_SIZEOF(UShort) * sVarColumnCnt ),
                               ID_SIZEOF(UShort));

                /* offset 이므로 헤더를 제외해야 사이즈가 된다 */
                sVCPieceSize -= ID_SIZEOF(smVCPieceHeader);

                /* set VC piece header */
                sVCPieceHeader              = (smVCPieceHeader*)sVCPiecePtr;
                sVCPieceHeader->nxtPieceOID = sNxtPieceOID;
                sVCPieceHeader->colCount    = sVarColumnCnt;

                /* get VC value */
                idlOS::memcpy( sVCPiecePtr + ID_SIZEOF(smVCPieceHeader),
                               sAftImage,
                               sVCPieceSize );

                sAftImage += sVCPieceSize;

                // BUG-47366 UnitedVar의 AllocSlot에 대한 Redo 수행
                sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
                sVCPieceHeader->flag &= ~SM_VCPIECE_TYPE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_TYPE_OTHER;

                IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                       SM_MAKE_PID(sVCPieceOID))
                        != IDE_SUCCESS);

                sVCPieceOID = sNxtPieceOID;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /*
      - Column ID(UInt) | Length(UInt) | Value | OID List
    */

    /* 로그 마지막 OldVersion RowOID 는 제외한다. */
    sFence = aImage + aSize - ID_SIZEOF(ULong);

    /* Large Var Variable/LOB Column 영역에 대한 Redo */
    while( sAftImage < sFence )
    {
        /* 각 Column당 하나씩 존재: Column ID(UInt) | LENGTH(UInt) | Value
           OID Cnt | OID(1) OID(2) ... OID(n) */

        /* Skip Column ID */
        sAftImage += ID_SIZEOF(UInt);

        /* Get Variable Column Length */
        idlOS::memcpy( &sVCSize, sAftImage, ID_SIZEOF(UInt) );
        sAftImage += ID_SIZEOF(UInt);

        IDE_ERROR(sVCSize != 0);

        /* Get VC Piece Count */
        sVCPieceCnt = smcRecord::getVCPieceCount( sVCSize );

        /* VC Piece영역에 대해서 Redo연산을 수행한다. */
        sVCPieceValuePtr = sAftImage;
        sVCPartSize = sVCSize;

        sAftImage  += (sVCSize + ID_SIZEOF(UInt));

        for( i = 0; i < sVCPieceCnt; i++ )
        {
            idlOS::memcpy( &sVCPieceOID, sAftImage, ID_SIZEOF( smOID ) );
            sAftImage += ID_SIZEOF( smOID );

            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                               sVCPieceOID,
                                               (void**)&sVCPiecePtr )
                        == IDE_SUCCESS );

            /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
               수행되어야 한다.*/
            if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
            {
                /* for global transaction, add OID into OID_List */
                IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                        sTableOID,
                                                        sVCPieceOID,
                                                        aSpaceID,
                                                        SM_OID_NEW_VARIABLE_SLOT )
                          != IDE_SUCCESS );
            }

            if( i == (sVCPieceCnt - 1) )
            {
                sVCPieceSize = sVCPartSize;
            }
            else
            {
                sVCPieceSize = SMP_VC_PIECE_MAX_SIZE;
            }

            /* BUG-15354: [A4] SM VARCHAR 32K: Varchar저장시 PieceHeader에 대한 logging이
             * 누락되어 PieceHeader에 대한 Redo, Undo가 되지않음. */
            sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
            sVCPieceHeader->length = sVCPieceSize;

            /* Redo VC Piece영역의 Value영역을 Redo한다 */
            idlOS::memcpy( sVCPiecePtr + ID_SIZEOF(smVCPieceHeader),
                           sVCPieceValuePtr,
                           sVCPieceSize );

            IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                   SM_MAKE_PID(sVCPieceOID))
                     != IDE_SUCCESS);

            sVCPieceValuePtr += sVCPieceSize;
            sVCPartSize -= sVCPieceSize;
        }
    }

    /* After Image 마지막에 OldVersion RowOID가 있다. */
    idlOS::memcpy(&sOldFixOID, sAftImage, ID_SIZEOF(ULong));
    sAftImage += ID_SIZEOF(ULong);

    /* BUG-35149 - [SM] redo for SMR_SMC_PERS_INSERT_ROW should not use a old
     *             row's oid in log record.
     * OldVersion의 RowOID가 NULL_OID란 의미는 insert 인 경우이다.
     * update에 대한 redo와 insert에 대한 redo 함수가 본 함수로 동일하기
     * 때문에 insert와 update의 log image가 동일하고 insert인 경우
     * old version이 없기 때문에 NULL OID로 기록된다.
     * 따라서 redo시 NULL OID인 경우 old version에 next version을 설정하는
     * 작업을 하면 안된다. */
    if ( ! SM_IS_NULL_OID(sOldFixOID) )
    {
        IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                           sOldFixOID,
                                           (void**)&sOldFixRow )
                    == IDE_SUCCESS );

        sOldSlotHeader = (smpSlotHeader*)sOldFixRow;

        /* OldRow의 Next에 기록할 New Version의 OID는 본 로그의 Self OID이다.  */
        SM_INIT_SCN( &(sOldSlotHeader->mLimitSCN) );
        SM_SET_SCN_DELETE_BIT( &(sOldSlotHeader->mLimitSCN) );
        SMP_SLOT_INIT_POSITION( sOldSlotHeader );
        SMP_SLOT_SET_USED( sOldSlotHeader );
        SMP_SLOT_SET_NEXT_OID( sOldSlotHeader, sNewFixOID );

        /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
           수행되어야 한다.*/
        /* for global transaction, add OID into OID_List */
        if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
        {
            IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                    sTableOID,
                                                    sOldFixOID,
                                                    aSpaceID,
                                                    SM_OID_OLD_UPDATE_FIXED_SLOT )
                      != IDE_SUCCESS );

            IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, SM_MAKE_PID(sOldFixOID) )
                      != IDE_SUCCESS );
        }

        /* BUG-47366 deleteVC 로그 제거에 따라 OldVersion의 UnitedVard Piece들을 Free 해주어야 한다. */
        sBfrImage  = aImage; /* After Image의 시작은 Before Image의 끝이다. */
        sBfrImage -= ( ID_SIZEOF(idBool) + ID_SIZEOF(ULong) );

        sBfrImage -= ID_SIZEOF(smOID);
        idlOS::memcpy(&sVCPieceOID, sBfrImage, ID_SIZEOF(smOID));

        /* OID가 NULL이면 볼 필요 없다. */
        if ( sVCPieceOID != SM_NULL_OID )
        {
            sBfrImage -= ID_SIZEOF(UShort);
            idlOS::memcpy(&sVarColumnCnt, sBfrImage, ID_SIZEOF(UShort));

            /* 수정된 VarColumn Cnt가 0이면 Old Version을 그냥 사용하는 것이라 Free 하지 않아도 된다. */
            if ( sVarColumnCnt > 0 )
            {
                while( sVCPieceOID != SM_NULL_OID )
                {
                    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                                       sVCPieceOID,
                                                       (void**)&sVCPiecePtr )
                                == IDE_SUCCESS );
                    if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
                    {
                        /* for global transaction, add OID into OID_List */
                        IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                                sTableOID,
                                                                sVCPieceOID,
                                                                aSpaceID,
                                                                SM_OID_OLD_VARIABLE_SLOT )
                                  != IDE_SUCCESS );
                    }

                    sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                    sVCPieceHeader->flag = SM_VCPIECE_FREE_OK | SM_VCPIECE_TYPE_OTHER;

                    IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                              != IDE_SUCCESS);

                    /* get next piece oid */
                    sBfrImage -= ID_SIZEOF(smOID);
                    idlOS::memcpy( &sVCPieceOID, sBfrImage, ID_SIZEOF(smOID) );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SMR_SMC_PERS_INSERT_ROW 의 Update 로그의 Undo Function.
 *               Insert대한 Record의 데이타 영역에 대해서는 Undo가 필요없으나.
 *               서비스 중의 Undo시에 Table의 Index에 Insert된 Record에 대해
 *               서 Delete작업을 수행해야 한다.
 *
 * aTID      - [IN] Transaction ID
 * aPID      - [IN] Page ID
 * aOffset   - [IN] Offset
 * aData     - [IN] Table ID
 * aImage    - [IN] Before Image
 * aSize     - [IN] Before Image Size
 * aFlag     - [IN] None
 ***********************************************************************/
IDE_RC smcRecordUpdate::undo_SMC_PERS_INSERT_ROW(
                                           smTID       aTID,
                                           scSpaceID   aSpaceID,
                                           scPageID    aPID,
                                           scOffset    aOffset,
                                           vULong      aData,
                                           SChar     * aImage,
                                           SInt        aSize,
                                           UInt        /*aFlag*/)
{
    void           * sTransPtr;
    smOID            sFixRowID;
    smOID            sTableOID;
    void           * sFixRowPtr;

    // BUG-47366
    SChar          * sAftImage      = NULL;
    UShort           sFixRowSize    = 0;
    smOID            sVCPieceOID    = SM_NULL_OID;
    smOID            sNxtPieceOID   = SM_NULL_OID;
    SChar          * sVCPiecePtr    = NULL;
    smVCPieceHeader* sVCPieceHeader = NULL;
    UShort           sVarColumnCnt  = 0;
    UShort           sVCPieceSize   = 0;

    sTransPtr = smLayerCallback::getTransByTID( aTID );
    sTableOID = (smOID)aData;
    sFixRowID = SM_MAKE_OID(aPID, aOffset);

    if(smrRecoveryMgr::isRestart() == ID_FALSE)
    {
        IDE_TEST( smLayerCallback::undoInsertOfTableInfo( sTransPtr, aData )
                  != IDE_SUCCESS );
    }

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                       sFixRowID,
                                       (void**)&sFixRowPtr )
                == IDE_SUCCESS );

    /* BUG-14513: Insert Log가 Alloc Slot에 대한 Undo수행 */
    IDE_TEST(smcRecord::setDeleteBit( sTransPtr,
                                      aSpaceID,
                                      sFixRowPtr,
                                      SMC_WRITE_LOG_NO)
             != IDE_SUCCESS);

    /* BUG-47366 UnitedVar의 AllocSlot에 대한 Undo 수행 */
    sAftImage = aImage + aSize;
    /* Get Fixed Row Size : USHORT */
    idlOS::memcpy(&sFixRowSize, sAftImage, ID_SIZEOF(UShort));
    sAftImage += ID_SIZEOF(UShort);
    sAftImage += sFixRowSize;

    idlOS::memcpy(&sVCPieceOID, sAftImage, ID_SIZEOF(smOID));
    sAftImage += ID_SIZEOF(smOID);
    
    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* get Column count in united var piece */
        idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));
        sAftImage += ID_SIZEOF(UShort);

        /* update 에서 united var가 전혀 변경되지 않는 경우 */
        if ( sVarColumnCnt > 0 )
        {
            /* skip column IDs */
            sAftImage += ID_SIZEOF(UInt) * sVarColumnCnt;

            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );

                sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                
                sVCPieceHeader->flag = SM_VCPIECE_FREE_OK | SM_VCPIECE_TYPE_OTHER;

                IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                          != IDE_SUCCESS);

                /* get next piece oid */
                idlOS::memcpy(&sNxtPieceOID, sAftImage, ID_SIZEOF(smOID));
                sAftImage += ID_SIZEOF(smOID);

                /* get column count in this piece */
                idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));

                sAftImage += ID_SIZEOF(UShort);

                /* get VC Piece Size */
                idlOS::memcpy( &sVCPieceSize,
                               sAftImage + ( ID_SIZEOF(UShort) * sVarColumnCnt ),
                               ID_SIZEOF(UShort));

                /* offset 이므로 헤더를 제외해야 사이즈가 된다 */
                sVCPieceSize -= ID_SIZEOF(smVCPieceHeader);

                sAftImage += sVCPieceSize;

                sVCPieceOID = sNxtPieceOID;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                               SMR_RTOI_TYPE_MEMPAGE,
                                               sTableOID,
                                               0, /* IndexID */
                                               aSpaceID,
                                               aPID );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : DML중 Update시 발생하는 Log를 기록한다. 이 Update로그는
 *               만약 이 Table에 Replication이 걸려 있을 경우 Replication
 *               Sender가 읽어 들인다. 그리고 이런 Replication을 위해서
 *               Fixed Row와 Variable Log가 각기 다른 Page영역에 저장되지만
 *               하나의 DML 로그로 기록한다.
 *
 * LOG HEADER :  smrUpdateLog (SMR_SMC_PERS_UPDATE_VERSION_ROW)
 * BODY       :
 *     Befor  Image: Sender가 읽어서 보내는 로그와 OldRowOID, NextOID를 기록
 *                   각각의 Update되는 Column에 대해서
 *
 *        Fixed Column : Column ID | SIZE | DATA
 *        Var   Column :
 *            1. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SMP_VCDESC_MODE_OUT
 *               - Column ID(UInt) | Length(UInt) | Value
 *
 *            2. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SMP_VCDESC_MODE_IN
 *               - Column ID(UInt) | Length(UInt) | Value
 *        LOB   Column : replication에서 before image는 처리하지 않는다.
 *
 *     After  Image: Header를 제외한 Fixed Row 전체와 Variable Column에
 *                   대한 Log, OldRowOID와 NextOID를 기록
 *        Fixed   Column :
 *                   Fixed Row Size(UShort) + Fixed Row Data
 *
 *        Var/LOB Column :
 *            1. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_OUT
 *               - Column ID(UInt) | Length(UInt) | Value | OID Cnt | OID ... 들
 *
 *            2. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_IN
 *               - Fixed Row 로그에 데이타가 저장되어 있기때문에 문제가
 *                 로깅할 필요가 없다.
 *
 *     Primary Key : Repl Sender가 이 로그를 읽어야 할때만 기록된다.
 *         Key마다 다음과 같은 데이타가 각각 존재
 *               - Length(UInt) | Column ID(UInt) | Value
 *
 * aTrans            - [IN] Transaction Pointer
 * aHeader           - [IN] Table Header Pointer
 * aColumnList       - [IN] Update Column List
 * aIsReplSenderSend - [IN] 이 로그를 Sender가 읽는다면 ID_TRUE,else ID_FALSE
 * aOldRowOID        - [IN] Old version의 OID
 * aBFixedRow        - [IN] MVCC의 Update시 Old Version이 되는 Row Pointer
 * aAFixedRow        - [IN] MVCC의 Update시 New Version이 되는 Row Pointer
 * aIsLockRow        - [IN] old version이 lock row였는지 여부
 * aBImageSize       - [IN] Before Image Size
 * aAImageSize       - [IN] After  Image Size
 * aUnitedVarColCnt  - [IN] United Var col count 
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeUpdateVersionLog( void                 * aTrans,
                                               smcTableHeader       * aHeader,
                                               const smiColumnList  * aColumnList,
                                               idBool                 aIsReplSenderSend,
                                               smOID                  aOldRowOID,
                                               SChar                * aBFixedRow,
                                               SChar                * aAFixedRow,
                                               idBool                 aIsLockRow,
                                               UInt                   aBImageSize,
                                               UInt                   aAImageSize,
                                               UShort                 aUnitedVarColCnt )
{
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;

    scPageID          sPageID;
    SInt              sColType;
    UInt              sLogSize;
    UShort            sFixedRowSize;/* Fixed Row Size: 반드시 UShort */
    UInt              sBeforeOffset;
    UInt              sAfterOffset;
    UInt              sPrimaryKeySize   = 0;
    smVCDesc        * sVCDescPtr        = NULL;
    smrUpdateLog      sUpdateLog;
    UInt              sLogFlag;
    smrLogType        sLogType;
    smcMakeLogFlagOpt sMakeLogFlag;
    smOID             sVCPieceOID       = SM_NULL_OID;
    smOID             sCurVCPieceOID    = SM_NULL_OID; /* BUG-43320 */
    UInt              sVCLen            = 0;
    SChar           * sVCValue          = NULL;
    SChar           * sBuff             = NULL;
    UInt              sVCSize           = 0;
    smVCPieceHeader * sVCPieceHeader    = NULL;
    UInt              sCIDOffset        = 0;
    UInt              sCIDFence         = 0;
    UInt              sMax              = ID_UINT_MAX;

    sPageID = SMP_SLOT_GET_PID(aAFixedRow);

    sFixedRowSize = aHeader->mFixed.mMRDB.mSlotSize - SMP_SLOT_HEADER_SIZE;

    sLogSize = SMR_LOGREC_SIZE(smrUpdateLog)
        + aBImageSize + aAImageSize + ID_SIZEOF(smrLogTail);

    if( aIsReplSenderSend == ID_TRUE )
    {
        sPrimaryKeySize = getPrimaryKeySize(aHeader, aBFixedRow);
        sLogSize += sPrimaryKeySize;
    }

    /* Log Header 초기화 */
    smrLogHeadI::setType(&sUpdateLog.mHead, SMR_LT_UPDATE);
    smrLogHeadI::setTransID( &sUpdateLog.mHead, smLayerCallback::getTransID( aTrans ) );

    // BUG-13046 Itanium HP 11.23 BUG
#if defined(IA64_HP_HPUX)
    smrLogHeadI::setFlag(&sUpdateLog.mHead, 0);
#endif

    // To Fix PR-14581
    // sLogFlag = smrLogHeadI::getFlag(&sUpdateLog.mHead);
    sMakeLogFlag = SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK;

    if( aIsReplSenderSend == ID_FALSE )
    {
        sMakeLogFlag = (smcMakeLogFlagOpt)(SMC_MKLOGFLAG_REPL_SKIP_LOG | sMakeLogFlag);
    }

    IDE_TEST( makeLogFlag(aTrans,
                          aHeader,
                          SMR_SMC_PERS_UPDATE_VERSION_ROW,
                          sMakeLogFlag,
                          &sLogFlag)
              != IDE_SUCCESS );

    /* makeLogFlag가 설정되이후에 호출되어야 한다. 왜냐면 안에서
     * smLayerCallback::getLstReplStmtDepth가 return하는 값을 변경한다.*/
    smrLogHeadI::setReplStmtDepth( &sUpdateLog.mHead,
                                   smLayerCallback::getLstReplStmtDepth( aTrans ) );

    smrLogHeadI::setFlag(&sUpdateLog.mHead, sLogFlag);

    SC_MAKE_GRID( sUpdateLog.mGRID,
                  aHeader->mSpaceID,
                  sPageID,
                  SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aAFixedRow ) );

    sUpdateLog.mData        = aHeader->mSelfOID;
    sUpdateLog.mType        = SMR_SMC_PERS_UPDATE_VERSION_ROW;
    smrLogHeadI::setSize(&sUpdateLog.mHead, sLogSize);
    sUpdateLog.mAImgSize    = aAImageSize;
    sUpdateLog.mBImgSize    = aBImageSize;

    smrLogHeadI::setPrevLSN( &sUpdateLog.mHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    /* Begin Write Log */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &(sUpdateLog),/* Update Log Header */
                                                 0,
                                                 SMR_LOGREC_SIZE(smrUpdateLog) )
              != IDE_SUCCESS );

    sBeforeOffset = SMR_LOGREC_SIZE(smrUpdateLog);
    sAfterOffset  = sBeforeOffset + aBImageSize;

    /* Write After Fixed Record [Fixed Row Size:Fixed Row] */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sFixedRowSize,/* Fixed Row Size */
                                                 sAfterOffset,
                                                 ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    sAfterOffset += ID_SIZEOF(UShort);

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 aAFixedRow + SMP_SLOT_HEADER_SIZE, /* SlotHeader를 제외한 데이타 */
                                                 sAfterOffset,
                                                 sFixedRowSize )
              != IDE_SUCCESS );

    sAfterOffset += sFixedRowSize;

    /* proj-2419 united var logging */
    sVCPieceOID = ((smpSlotHeader*)aAFixedRow)->mVarOID;

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sVCPieceOID, /* first OID of united var piece */
                                                 sAfterOffset,
                                                 ID_SIZEOF(smOID) )
              != IDE_SUCCESS );
    sAfterOffset += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &aUnitedVarColCnt, /* Column count in United var piece */
                                                     sAfterOffset,
                                                     ID_SIZEOF(UShort) )
                  != IDE_SUCCESS );
        sAfterOffset += ID_SIZEOF(UShort);

        if ( aUnitedVarColCnt > 0 )
        {
            /* CID offset 저장했다가 column 순회할때 id 를 로깅한다 */
            sCIDOffset = sAfterOffset;

            sAfterOffset += ID_SIZEOF(UInt) * aUnitedVarColCnt;

            sCIDFence   = sAfterOffset;

            while ( sCIDOffset < sCIDFence )
            {
                /* United Var 일부만 업데이트 된 경우 
                 * rp 에게는 update 된 cid 만 전달하기 위해 uint max 로 표시한다. 
                 * BUG-43565 : 미리 컬럼 개수 만큼 UINT MAX로 채워 둔다. */

                IDE_TEST(smLayerCallback::writeLogToBuffer( aTrans,
                                                            &sMax,
                                                            sCIDOffset,
                                                            ID_SIZEOF(UInt))
                         != IDE_SUCCESS);

                sCIDOffset += ID_SIZEOF(UInt);
            }

            sCIDOffset -= (ID_SIZEOF(UInt) * aUnitedVarColCnt);

            /* BUG-43320 sVCPieceOID는 VarColumn이 기록되었는지를 판단하는데 사용되기 때문에
             * 변경되어서는 안된다. */
            sCurVCPieceOID = sVCPieceOID;

            /* Variable Column Value 기록. */
            while ( sCurVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                                   sCurVCPieceOID,
                                                   (void**)&sVCPieceHeader )
                            == IDE_SUCCESS );

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->nxtPieceOID),
                                                             sAfterOffset,
                                                             ID_SIZEOF(smOID) )
                          != IDE_SUCCESS );
                sAfterOffset += ID_SIZEOF(smOID);


                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->colCount),
                                                             sAfterOffset,
                                                             ID_SIZEOF(UShort) )
                          != IDE_SUCCESS );
                sAfterOffset += ID_SIZEOF(UShort);

                sVCSize = getUnitedVCSize(sVCPieceHeader);

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             (SChar*)(sVCPieceHeader + 1),
                                                             sAfterOffset,
                                                             (UInt)sVCSize )
                          != IDE_SUCCESS );

                sAfterOffset += (UInt)sVCSize;

                sCurVCPieceOID = sVCPieceHeader->nxtPieceOID;
            }
        }
        else
        {
            /* Nothing to do */
        }
        
    }
    else
    {
        /* Nothing to do */
    }

    /* 먼저 Update를 수행하고 Log기록을 수행한다. 이렇게 해도 되는 이유는
       Update대상들의 Before Image가 없기 때문에 WAL을 지킬필요가 없다. 즉
       이 영역이 디스크에 반영될지라도 Undo가 필요없기 때문이다. */
    sCurColumnList = aColumnList;

    for( ;
         sCurColumnList != NULL;
         sCurColumnList  = sCurColumnList->next )
    {
        sColumn  = sCurColumnList->column;
        sColType = sColumn->flag & SMI_COLUMN_TYPE_MASK;

        if ( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
                    != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch ( sColType )
            {
                case SMI_COLUMN_TYPE_LOB:
                    if ( aIsReplSenderSend == ID_TRUE )
                    {
                        IDE_TEST( writeDummyBVCLog4Lob( aTrans,
                                                        sColumn->id,
                                                        &sBeforeOffset )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    sVCDescPtr = smcRecord::getVCDesc( sColumn, aAFixedRow );

                    /* Var Column Log Header : ID | SIZE | DATA */
                    IDE_TEST( writeVCLog4MVCC( aTrans,
                                               sColumn,
                                               sVCDescPtr,
                                               &sAfterOffset,
                                               SMC_VC_LOG_WRITE_TYPE_AFTERIMG )
                              != IDE_SUCCESS );
                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                    if ( aIsReplSenderSend == ID_TRUE )
                    {
                        sVCValue = smcRecord::getVarRow( aBFixedRow,
                                                         sColumn,
                                                         0,     /* read position */
                                                         &sVCLen,
                                                         sBuff  /* dummy buf */,
                                                         ID_TRUE );
                        /* Variable Column ID기록 */
                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     &(sColumn->id),
                                                                     sBeforeOffset,
                                                                     ID_SIZEOF(UInt) )
                                  != IDE_SUCCESS );

                        sBeforeOffset += ID_SIZEOF(UInt);

                        /* Variable Column Length 기록 */
                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     &sVCLen,
                                                                     sBeforeOffset,
                                                                     ID_SIZEOF(UInt) )
                                  != IDE_SUCCESS );

                        sBeforeOffset += ID_SIZEOF(UInt);

                        /* Variable Column Value 기록 */
                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     sVCValue,
                                                                     sBeforeOffset,
                                                                     sVCLen )
                                  != IDE_SUCCESS );

                        sBeforeOffset += sVCLen;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    
                    /* BUG-43320 VarSlot이 할당되지 않은 경우
                     * CID를 기록하면 안된다. */
                    if ( sVCPieceOID != SM_NULL_OID )  
                    {
                        /* var image 는 united var log 에 포함되어있다
                         * 위에서 공간 남겨둔 united var column id 를 로깅한다 
                         * BUG-43565 : var column 순서대로 volumn ID를 로깅한다.
                         */  
                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     &(sColumn->id),
                                                                     sCIDOffset + (sColumn->varOrder*ID_SIZEOF(UInt)),
                                                                     ID_SIZEOF(UInt) )
                                  != IDE_SUCCESS );
                    }
                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                    if ( aIsReplSenderSend == ID_TRUE )
                    {
                        /* 이 Table에 Replication이 걸려있는 경우 VarColumn의
                           Old Version까지 Logging 해야 된다. Write Before Image */
                        sVCDescPtr = smcRecord::getVCDesc(sColumn, aBFixedRow);

                        /* Var Column Log Header : ID | SIZE | DATA */
                        IDE_TEST( writeVCLog4MVCC( aTrans,
                                                   sColumn,
                                                   sVCDescPtr,
                                                   &sBeforeOffset,
                                                   SMC_VC_LOG_WRITE_TYPE_BEFORIMG )
                                != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    sVCDescPtr = smcRecord::getVCDesc( sColumn, aAFixedRow );

                    /* Var Column Log Header : ID | SIZE | DATA */
                    IDE_TEST( writeVCLog4MVCC( aTrans,
                                               sColumn,
                                               sVCDescPtr,
                                               &sAfterOffset,
                                               SMC_VC_LOG_WRITE_TYPE_AFTERIMG )
                            != IDE_SUCCESS );
                    break;

                case SMI_COLUMN_TYPE_FIXED:

                    if ( aIsReplSenderSend == ID_TRUE )
                    {
                        /* 이 Table에 Replication이 걸려있는 경우 Fixed Column의
                           Update되기 전 값까지 Logging 해야 된다.*/

                        /* Fixed Column Log Header : ID | SIZE | Data */
                        IDE_TEST( writeFCLog4MVCC( aTrans,
                                                   sColumn,
                                                   &sBeforeOffset,
                                                   aBFixedRow + sColumn->offset,
                                                   sColumn->size)
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;

                default:
                    /* Column은 Variable 이거나 Fixed 이어야 한다. */
                    IDE_ERROR_MSG( 0,
                                   "sColumn->id    :%"ID_UINT32_FMT"\n"
                                   "sColumn->flag  :%"ID_UINT32_FMT"\n"
                                   "sColumn->offset:%"ID_UINT32_FMT"\n"
                                   "sColumn->vcInOu:%"ID_UINT32_FMT"\n"
                                   "sColumn->size  :%"ID_UINT32_FMT"\n"
                                   "sColType       :%"ID_UINT32_FMT"\n",
                                   sColumn->id,
                                   sColumn->flag,
                                   sColumn->offset,
                                   sColumn->vcInOutBaseSize,
                                   sColumn->size,
                                   sColType );
                    break;
            }
        }
        else //Compression
        {
            if ( aIsReplSenderSend == ID_TRUE )
            {

                /* Fixed Column Log Header : ID | SIZE | Data */
                IDE_TEST( writeFCLog4MVCC( aTrans,
                                           sColumn,
                                           &sBeforeOffset,
                                           aBFixedRow + sColumn->offset,
                                           ID_SIZEOF(smOID))
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    /* before */
    /* BUG-47366 Old Version의 UnitedVar OID를 기억해야한다. */
    sVCPieceOID = ((smpSlotHeader*)aBFixedRow)->mVarOID;

    if ( sVCPieceOID != SM_NULL_OID )
    {
        if ( aUnitedVarColCnt > 0 )
        {
            sCurVCPieceOID = SM_NULL_OID;
            /* UnitedVar OID의 끝을 알기 위해 NULL_OID를 기록해 둔다. */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         &sCurVCPieceOID,
                                                         sBeforeOffset,
                                                         ID_SIZEOF(smOID) )
                          != IDE_SUCCESS );
            sBeforeOffset += ID_SIZEOF(smOID);

            /* 첫번째 UnitedVar Piece의 OID는 가장 마지막에 Logging 할것이라 넘긴다. */
            IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                               sVCPieceOID,
                                               (void**)&sVCPieceHeader )
                        == IDE_SUCCESS );

            sCurVCPieceOID = sVCPieceHeader->nxtPieceOID;

            /* UnitedVar OID 기록. */
            while ( sCurVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                                   sCurVCPieceOID,
                                                   (void**)&sVCPieceHeader )
                            == IDE_SUCCESS );

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &sCurVCPieceOID,
                                                             sBeforeOffset,
                                                             ID_SIZEOF(smOID) )
                          != IDE_SUCCESS );
                sBeforeOffset += ID_SIZEOF(smOID);

                sCurVCPieceOID = sVCPieceHeader->nxtPieceOID;
            }
        }

        /* UnitedVar Column 의 수를 기록한다. 0인지 아닌지만 필요 */
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &aUnitedVarColCnt, /* Column count in United var piece */
                                                     sBeforeOffset,
                                                     ID_SIZEOF(UShort) )
                      != IDE_SUCCESS );
        sBeforeOffset += ID_SIZEOF(UShort);
    }

    /* 뒤에서 부터 읽을거기 때문에 Old Version의 UnitedVar OID를 가장 뒤에 기록한다.
     * NULL이여도 기록 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sVCPieceOID, /* first OID of united var piece */
                                                 sBeforeOffset,
                                                 ID_SIZEOF(smOID) )
              != IDE_SUCCESS );
    sBeforeOffset += ID_SIZEOF(smOID);

    /* Old Row OID */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aOldRowOID,
                                                 sBeforeOffset,
                                                 ID_SIZEOF(ULong) )
              != IDE_SUCCESS );

    sBeforeOffset += ID_SIZEOF(ULong);

    /* Before Next OID's lock row */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aIsLockRow,
                                                 sBeforeOffset,
                                                 ID_SIZEOF(idBool) )
              != IDE_SUCCESS );

    sBeforeOffset += ID_SIZEOF(idBool);

    /* after */
    /* Old Row OID */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aOldRowOID,
                                                 sAfterOffset,
                                                 ID_SIZEOF(ULong) )
              != IDE_SUCCESS );

    sAfterOffset += ID_SIZEOF(ULong);

    /* BUG-47723 update log의 before image size가 맞지 않습니다.
     * Offset 검증, After Size 에 repl 정보는 포함되지 않음*/
    IDE_ERROR_MSG( sBeforeOffset == ( SMR_LOGREC_SIZE(smrUpdateLog) + aBImageSize ),
                   "Before %d = %d + %d\n",
                   sBeforeOffset, SMR_LOGREC_SIZE(smrUpdateLog),  aBImageSize );

    IDE_ERROR_MSG( sAfterOffset  == ( SMR_LOGREC_SIZE(smrUpdateLog) + aBImageSize + aAImageSize ),
                   "After %d = %d + %d + %d\n",
                   sAfterOffset , SMR_LOGREC_SIZE(smrUpdateLog) , aBImageSize , aAImageSize );

    if( aIsReplSenderSend == ID_TRUE )
    {
        /* 이 Table에 Replication이 걸려있는 경우 Primary Key를
           Logging한다.*/
        IDE_TEST( writePrimaryKeyLog(aTrans,
                                     aHeader,
                                     aBFixedRow,
                                     sPrimaryKeySize,
                                     sAfterOffset)
                 != IDE_SUCCESS);

        sAfterOffset += sPrimaryKeySize;
    }

    sLogType = smrLogHeadI::getType(&sUpdateLog.mHead);
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sLogType,
                                                 sAfterOffset,
                                                 ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );
    smrLogHeadI::setType(&sUpdateLog.mHead, sLogType);

    IDE_TEST( smLayerCallback::writeTransLog( aTrans, sUpdateLog.mData ) != IDE_SUCCESS );


    /*
      Update시 새로운 Version이 생긴다. 계속 Transaction이 Update하게 되면
      계속 Version이 쌓이게 된다. 이런 문제를 없애기 위해 Update Log양이 일정 수준이
      상이 된다면 Update Inplace로 수행한다.
    */
    smLayerCallback::addToUpdateSizeOfTrans( aTrans,
                                             smrLogHeadI::getSize( &sUpdateLog.mHead ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SMR_SMC_PERS_UPDATE_VERSION_ROW 의 Update 로그의
 *               Redo Function 이다.
 *
 * After Image : Fixed Row와 Variable Column에 대한 Log를 기록.
 *               Fixed Row Size(UShort) + Fixed Row Data
 *                             + VCLOG(1) + VCLOG(2) ... + VCLOG (n)
 *
 *               VCLOG       : Variable Column당 하나씩 생긴다.
 *                   Column ID(UInt) | LENGTH(UInt) | Value | OID Cnt
 *                   |  OID(1) OID(2) ... OID(n)
 * Befor Image : None
 *
 * aTID      - [IN] Transaction ID
 * aPID      - [IN] Page ID
 * aOffset   - [IN] Offset
 * aData     - [IN] Table ID
 * aImage    - [IN] After Image
 * aSize     - [IN] After Image Size
 * aFlag     - [IN] None
 ***********************************************************************/
IDE_RC smcRecordUpdate::redo_SMC_PERS_UPDATE_VERSION_ROW(
                                                   smTID      aTID,
                                                   scSpaceID  aSpaceID,
                                                   scPageID   aPID,
                                                   scOffset   aOffset,
                                                   vULong     aData,
                                                   SChar     *aImage,
                                                   SInt       aSize,
                                                   UInt       aFlag)
{
    return redo_SMC_PERS_INSERT_ROW(aTID,
                                    aSpaceID,
                                    aPID,
                                    aOffset,
                                    aData,
                                    aImage,
                                    aSize,
                                    aFlag);
}

/***********************************************************************
 * Description : SMR_SMC_PERS_UPDATE_VERSION_ROW 의 Update 로그의 Undo
 *                Function.
 *               Insert대한 Record의 데이타 영역에 대해서는 Undo가 필요없으나.
 *               서비스 중의 Undo시에 Table의 Index에 Insert된 Record에 대해
 *               서 Delete작업을 수행해야 한다.
 *
 * aTID      - [IN] Transaction ID
 * aPID      - [IN] Page ID
 * aOffset   - [IN] Offset
 * aData     - [IN] Table ID
 * aImage    - [IN] Before Image
 * aSize     - [IN] Before Image Size
 * aFlag     - [IN] aFlag
 ***********************************************************************/
IDE_RC smcRecordUpdate::undo_SMC_PERS_UPDATE_VERSION_ROW(
                                                   smTID      aTID,
                                                   scSpaceID  aSpaceID,
                                                   scPageID   aPID,
                                                   scOffset   aOffset,
                                                   vULong     aData,
                                                   SChar     *aImage,
                                                   SInt       aSize,
                                                   UInt       /*aFlag*/)
{
    void           * sTransPtr          = NULL;
    SChar          * sBeforeImage       = NULL;
    SChar          * sBeforeOIDImagePtr = NULL;
    SChar          * sIsLockRowPtr      = NULL;
    smOID            sTableOID          = SM_NULL_OID;
    void           * sNewFixRowPtr      = NULL;
    void           * sOldFixRowPtr      = NULL;
    smOID            sOldFixOID         = SM_NULL_OID;
    smOID            sNewFixOID         = SM_NULL_OID;
    smpSlotHeader  * sOldSlotHeader     = NULL;
    idBool           sIsLockRow         = ID_FALSE;

    // BUG-47366
    SChar          * sAftImage      = NULL;
    UShort           sFixRowSize    = 0;
    smOID            sVCPieceOID    = SM_NULL_OID;
    smOID            sNxtPieceOID   = SM_NULL_OID;
    SChar          * sVCPiecePtr    = NULL;
    smVCPieceHeader* sVCPieceHeader = NULL;
    UShort           sVarColumnCnt  = 0;
    UShort           sVCPieceSize   = 0;

    SChar          * sBfrImage      = NULL;

    sTransPtr    = smLayerCallback::getTransByTID( aTID );
    sTableOID    = (smOID)aData;
    sNewFixOID   = SM_MAKE_OID( aPID, aOffset );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                       sNewFixOID,
                                       (void**)&sNewFixRowPtr )
                == IDE_SUCCESS );

    /* BUG-14513: Insert Log가 Alloc Slot에 대한 Undo수행 */
    IDE_TEST(smcRecord::setDeleteBit(sTransPtr,
                                     aSpaceID,
                                     sNewFixRowPtr,
                                     SMC_WRITE_LOG_NO)
             != IDE_SUCCESS);

    /* before image는 OldRowOID 가 담겨있다. */
    sBeforeImage = aImage;

    sBeforeOIDImagePtr = (sBeforeImage + aSize)
                         - ID_SIZEOF(ULong)
                         - ID_SIZEOF(idBool);

    sIsLockRowPtr      = (sBeforeImage + aSize) - ID_SIZEOF(idBool);

    idlOS::memcpy( &sOldFixOID, sBeforeOIDImagePtr, ID_SIZEOF(smOID) );

    idlOS::memcpy( &sIsLockRow, sIsLockRowPtr, ID_SIZEOF(idBool) );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       sOldFixOID,
                                       (void**)&sOldFixRowPtr )
                == IDE_SUCCESS );

    sOldSlotHeader = (smpSlotHeader*)sOldFixRowPtr;

    SMP_SLOT_INIT_POSITION( sOldSlotHeader );
    SMP_SLOT_SET_USED( sOldSlotHeader );
    
    /* BUG-33738 - [SM] undo of SMC_PERS_UPDATE_VERSION_ROW log is wrong
     * lockRow된 row를 update했을때 undo시 다시 lockRow해준다. */
    if ( sIsLockRow == ID_TRUE )
    {
        SMP_SLOT_SET_LOCK( sOldSlotHeader, aTID );
    }
    else
    {
        SM_SET_SCN_FREE_ROW( &(sOldSlotHeader->mLimitSCN) );
    }

    /* BUG-43182 rollback memory update 도중에 checkpoint가 발생하면
     * old version row가 삭제 될 수 있습니다. */
    IDE_TEST(smmDirtyPageMgr::insDirtyPage( aSpaceID,
                                            SM_MAKE_PID( sOldFixOID )) != IDE_SUCCESS);

    /* BUG-47366 Old Version의 UnitedVar에 대한 Undo 수행 */
    sBfrImage  = aImage + aSize; /* Before Image의 끝에서부터 역으로 읽어야 한다. */
    sBfrImage -= ( ID_SIZEOF(idBool) + ID_SIZEOF(ULong) );

    sBfrImage -= ID_SIZEOF(smOID);
    idlOS::memcpy(&sVCPieceOID, sBfrImage, ID_SIZEOF(smOID));

    /* OID가 NULL이면 볼 필요 없다. */
    if ( sVCPieceOID != SM_NULL_OID )
    {
        sBfrImage -= ID_SIZEOF(UShort);
        idlOS::memcpy(&sVarColumnCnt, sBfrImage, ID_SIZEOF(UShort));

        /* 수정된 VarColumn Cnt가 0이면 Old Version을 그냥 사용하는 것이라 변경된 것이 없다. */
        if ( sVarColumnCnt > 0 )
        {
            while( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );

                /* undo에서는 addOID를 해줄 필요가 없다. redo 또는 운영중 이미 addOID되어 있는 상태 */
                sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
                sVCPieceHeader->flag &= ~SM_VCPIECE_TYPE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_TYPE_OTHER;

                IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                          != IDE_SUCCESS);

                /* get next piece oid */
                sBfrImage -= ID_SIZEOF(smOID);
                idlOS::memcpy( &sVCPieceOID, sBfrImage, ID_SIZEOF(smOID) );
            }
        }
    }

    /* BUG-47366 New Version의 UnitedVar의 AllocSlot에 대한 Undo 수행 */
    sAftImage = aImage + aSize;
    /* Get Fixed Row Size : USHORT */
    idlOS::memcpy(&sFixRowSize, sAftImage, ID_SIZEOF(UShort));
    sAftImage += ID_SIZEOF(UShort);
    sAftImage += sFixRowSize;

    idlOS::memcpy(&sVCPieceOID, sAftImage, ID_SIZEOF(smOID));
    sAftImage += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* get Column count in united var piece */
        idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));
        sAftImage += ID_SIZEOF(UShort);

        /* update 에서 united var가 전혀 변경되지 않는 경우 */
        if ( sVarColumnCnt > 0 )
        {
            /* skip column IDs */
            sAftImage += ID_SIZEOF(UInt) * sVarColumnCnt;

            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );

                sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                
                sVCPieceHeader->flag = SM_VCPIECE_FREE_OK | SM_VCPIECE_TYPE_OTHER;

                IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                          != IDE_SUCCESS);

                /* get next piece oid */
                idlOS::memcpy(&sNxtPieceOID, sAftImage, ID_SIZEOF(smOID));
                sAftImage += ID_SIZEOF(smOID);

                /* get column count in this piece */
                idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));

                sAftImage += ID_SIZEOF(UShort);

                /* get VC Piece Size */
                idlOS::memcpy( &sVCPieceSize,
                               sAftImage + ( ID_SIZEOF(UShort) * sVarColumnCnt ),
                               ID_SIZEOF(UShort));

                /* offset 이므로 헤더를 제외해야 사이즈가 된다 */
                sVCPieceSize -= ID_SIZEOF(smVCPieceHeader);

                sAftImage += sVCPieceSize;

                sVCPieceOID = sNxtPieceOID;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                               SMR_RTOI_TYPE_MEMPAGE,
                                               sTableOID,
                                               0, /* IndexID */
                                               aSpaceID,
                                               aPID );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : DML중 Update시 발생하는 Log를 기록한다. 이 Update로그는
 *               만약 이 Table에 Replication이 걸려 있을 경우 Replication
 *               Sender가 읽어 들인다. 그리고 이런 Replication을 위해서
 *               Fixed Row와 Variable Log가 각기 다른 Page영역에 저장되지만
 *               하나의 DML 로그로 기록한다.
 *
 * LOG HEADER :  smrUpdateLog (SMR_SMC_PERS_UPDATE_INPLACE_ROW)
 * BODY       :
 *      Befor  Image: 각각의 Update되는 Column에 대해서
 *         Fixed Column : Flag(SChar) | Offset(UInt)  |
 *                        ColumnID(UInt) | SIZE(UInt) | Value
 *
 *         Var   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SMP_VCDESC_MODE_OUT:
 *                        | Value | OID
 *               SMP_VCDESC_MODE_IN:
 *                        | Value
 *         LOB    Column : replication에서 before image는 처리하지 않는다.
 *
 *      After  Image: 각각의 Update되는 Column에 대해서
 *         Fixed   Column : Flag(SChar) | Offset | ColumnID(UInt) |
 *                        SIZE(UInt) |  Value
 *
 *         Var/LOB Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SMP_VCDESC_MODE_OUT:
 *                        | Value | OID Cnt | OID 들...
 *               SMP_VCDESC_MODE_IN:
 *                        | Value
 *
 *     Primary Key : Repl Sender가 이 로그를 읽어야 할때만 기록된다.
 *         Key마다 다음과 같은 데이타가 각각 존재
 *               - Length(UInt) | Column ID(UInt) | Value
 *
 * LOG TAIL   : Log Tail
 *
 * 부가설명 :  Flag   (SChar) : SMP_VCDESC_MODE(2nd bit) | SMI_COLUMN_TYPE (1st bit)
 *
 * aTrans            - [IN] Transaction Pointer
 * aHeader           - [IN] Table Header Pointer
 * aRowPtr           - [IN] Update될 Row Ptr
 * aColumnList       - [IN] Update될 Column List
 * aValueList        - [IN] Update Value List
 * aRowPtrBuffer     - [IN] Update후에 fixed row의 VCDesc를 가지고 있는 Buffer
 * aModifyIdxBit     - [IN] Index Key가 갱신되는 Index의 list
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeUpdateInplaceLog(void*                 aTrans,
                                              smcTableHeader*       aHeader,
                                              const SChar*          aRowPtr,
                                              const smiColumnList * aColumnList,
                                              const smiValue      * aValueList,
                                              SChar*                aRowPtrBuffer,
                                              ULong                 aModifyIdxBit )
{
    UInt   sLogOffset;
    UInt   sPrimaryKeySize;
    smcLogReplOpt sIsReplSenderSend;

    static smrLogType sType = SMR_LT_UPDATE;

    if ( smcTable::needReplicate( aHeader, aTrans ) == ID_TRUE )
    {
        sIsReplSenderSend = SMC_LOG_REPL_SENDER_SEND_OK;
    }
    else
    {
        sIsReplSenderSend = SMC_LOG_REPL_SENDER_SEND_NO;
    }

    /* Log Header기록 */
    IDE_TEST( writeUIPLHdr2TxLBf( aTrans,
                                  aHeader,
                                  sIsReplSenderSend,
                                  aRowPtr,
                                  aRowPtrBuffer,
                                  aColumnList,
                                  aValueList,
                                  &sPrimaryKeySize)
              != IDE_SUCCESS );

    sLogOffset = SMR_LOGREC_SIZE(smrUpdateLog);

    /* Log BeforeImage기록 */
    IDE_TEST( writeUIPBfrLg2TxLBf( aTrans,
                                   sIsReplSenderSend,
                                   aRowPtr,
                                   ((smpSlotHeader*)aRowPtrBuffer)->mVarOID,
                                   aColumnList,
                                   &sLogOffset,
                                   aHeader->mUnitedVarColumnCount,
                                   aModifyIdxBit )
              != IDE_SUCCESS );

    /* Log After Image기록 */
    IDE_TEST( writeUIPAftLg2TxLBf( aTrans,
                                   sIsReplSenderSend,
                                   aRowPtrBuffer,
                                   ((smpSlotHeader*)aRowPtr)->mVarOID,
                                   aColumnList,
                                   aValueList,
                                   &sLogOffset,
                                   aHeader->mUnitedVarColumnCount)
              != IDE_SUCCESS );

    if( sIsReplSenderSend == SMC_LOG_REPL_SENDER_SEND_OK )
    {
        //Write Primary Key
        IDE_TEST( writePrimaryKeyLog( aTrans,
                                      aHeader,
                                      aRowPtr,
                                      sPrimaryKeySize,
                                      sLogOffset)
                 != IDE_SUCCESS);

        sLogOffset += sPrimaryKeySize;
    }

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sType,
                                                 sLogOffset,
                                                 ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   aTrans,
                                   smLayerCallback::getLogBufferOfTrans( aTrans ),
                                   NULL,  // Previous LSN Ptr
                                   NULL,  // Log LSN Ptr
                                   NULL,  // End LSN Ptr
                                   aHeader->mSelfOID ) //TableOID
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Update Inplace로그시 기록하는 Log의 UpdateLog Header기록.
 *
 *
 * aTrans            - [IN] Transaction Pointer
 * aHeader           - [IN] Table Header Pointer
 * aIsReplSenderSend - [IN] Replication Sender가 읽어서 보내는 로그인가?
 * aFixedRow         - [IN] Update되는 Row Pointer
 * aColumnList       - [IN] Update될 Column List
 * aValueList        - [IN] Update될 Value List
 * aPrimaryKeySize   - [OUT] Primary Key Size
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeUIPLHdr2TxLBf(void                 * aTrans,
                                           const smcTableHeader * aHeader,
                                           smcLogReplOpt          aIsReplSenderSend,
                                           const SChar          * aFixedRow,
                                           SChar                * aRowBuffer,
                                           const smiColumnList  * aColumnList,
                                           const smiValue       * aValueList,
                                           UInt                 * aPrimaryKeySize)
{
    smrUpdateLog            sUpdateLog;
    const smiColumn       * sColumn;
    const smiColumnList   * sCurColumnList;
    const smiValue        * sCurValue;
    scPageID                sPageID;
    smVCDesc              * sVCDesc;

    UInt                    sBImgSize           = ID_SIZEOF(ULong); // sModifyIdxBit
    UInt                    sAImgSize           = 0;
    UInt                    sLogSize            = 0;
    UInt                    sPrimaryKeySize     = 0;
    UInt                    sLogFlag            = 0;
    smOID                   sBeforeOID          = SM_NULL_OID;
    smOID                   sAfterOID           = SM_NULL_OID;

    smcLogReplOpt           sIsReplSend4Column;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aHeader != NULL );

    sPageID = SMP_SLOT_GET_PID(aFixedRow);

    /* 로그의 Before Image와 After Image 크기를 구한다.*/
    sCurColumnList = aColumnList;
    sCurValue      = aValueList;

    for( ;
         sCurColumnList != NULL;
         sCurColumnList = sCurColumnList->next, sCurValue++ )
    {
        sColumn            = sCurColumnList->column;
        sIsReplSend4Column = aIsReplSenderSend;

        IDE_TEST_RAISE( sColumn->size < sCurValue->length,
                        err_invalid_column_size );

        // BUG-37460 update log 에서 compress column 이 고려되지 않았습니다.
        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
                    sVCDesc = smcRecord::getVCDesc(sColumn, aFixedRow);
                    if( sVCDesc->length != 0)
                    {
                        //smcLobDesc.mFirstLPCH + Piece Count
                        sBImgSize += (ID_SIZEOF(smcLPCH*) + ID_SIZEOF(UInt));
                    }

                    /* BUG-37433 Lob은 REPL_SENDER_NO로 처리한다 */
                    sIsReplSend4Column = SMC_LOG_REPL_SENDER_SEND_NO;

                    sBImgSize += smcRecord::getVCUILogBMSize(
                        sIsReplSend4Column,
                        sVCDesc->flag & SM_VCDESC_MODE_MASK,
                        sVCDesc->length);
    
                    sAImgSize += smcRecord::getVCUILogAMSize(
                        smcRecord::getVCStoreMode(sColumn, sCurValue->length),
                        sCurValue->length );
    
                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                    /* united var log size 에 포함된다 */
                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                    /* Update Inplace인 경우에는 Before Image로 Recovery를
                       수행해야 하므로 VC Piece들의 OID가 있는 After Image형
                       태로 Logging을 수행해야 한다. */
                    sVCDesc = smcRecord::getVCDesc(sColumn, aFixedRow);

                    sBImgSize += smcRecord::getVCUILogBMSize( sIsReplSend4Column,
                                                              sVCDesc->flag & SM_VCDESC_MODE_MASK,
                                                              sVCDesc->length);

                    sAImgSize += smcRecord::getVCUILogAMSize(
                                        smcRecord::getVCStoreMode(sColumn, sCurValue->length),
                                        sCurValue->length );

                    break;
    
                case SMI_COLUMN_TYPE_FIXED:
                    sBImgSize += smcRecord::getFCUILogSize( sColumn->size );
                    sAImgSize += smcRecord::getFCUILogSize( sCurValue->length );
    
                    break;
    
                default:
                    IDE_ERROR_MSG( 0,
                                "sColumn->id    :%"ID_UINT32_FMT"\n"
                                "sColumn->flag  :%"ID_UINT32_FMT"\n"
                                "sColumn->offset:%"ID_UINT32_FMT"\n"
                                "sColumn->vcInOu:%"ID_UINT32_FMT"\n"
                                "sColumn->size  :%"ID_UINT32_FMT"\n",
                                sColumn->id,
                                sColumn->flag,
                                sColumn->offset,
                                sColumn->vcInOutBaseSize,
                                sColumn->size );
                    break;
            }
        }
        else
        {
            sBImgSize += smcRecord::getFCUILogSize( ID_SIZEOF(smOID) );
            sAImgSize += smcRecord::getFCUILogSize( sCurValue->length );
        }
    }

    sBeforeOID = ((smpSlotHeader*)aFixedRow)->mVarOID;
    sAfterOID  = ((smpSlotHeader*)aRowBuffer)->mVarOID;

    if ( ( sBeforeOID == sAfterOID )            /* united var 가 존재하나 update 되지 않은 경우 */
            && ( sBeforeOID != SM_NULL_OID ))
    {                                           /* First piece OID + Column count 0 */
        sBImgSize += ID_SIZEOF(smOID) + ID_SIZEOF(UShort);
        sAImgSize += ID_SIZEOF(smOID) + ID_SIZEOF(UShort);
    }
    else                                        /* 그 외에는 각각 사이즈를 구한다 */
    {
        if ( sBeforeOID == SM_NULL_OID )
        {
            sBImgSize += ID_SIZEOF(smOID);
        }
        else
        {
            sBImgSize += smcRecord::getUnitedVCLogSize( aHeader, sBeforeOID );
        }

        if ( sAfterOID == SM_NULL_OID )
        {
            sAImgSize += ID_SIZEOF(smOID);
        }
        else
        {
            sAImgSize += smcRecord::getUnitedVCLogSize( aHeader, sAfterOID );
        }
    }

    /* BUG-47366 BeforeImg의 Size를 BeforeImg의 마지막에 저장해둔다. */
    sBImgSize += ID_SIZEOF(UInt);

    smrLogHeadI::setType(&sUpdateLog.mHead, SMR_LT_UPDATE);
    smrLogHeadI::setTransID( &sUpdateLog.mHead, smLayerCallback::getTransID( aTrans ) );

    // To Fix PR-14581
    // sLogFlag = smrLogHeadI::getFlag(&sUpdateLog.mHead);
    IDE_TEST( makeLogFlag( aTrans,
                           aHeader,
                           SMR_SMC_PERS_UPDATE_INPLACE_ROW,
                           SMC_MKLOGFLAG_SET_ALLOC_FIXED_NO,
                           &sLogFlag)
              != IDE_SUCCESS );

    /* makeLogFlag가 설정되이후에 호출되어야 한다. 왜냐면 안에서
     * smLayerCallback::getLstReplStmtDepth가 return하는 값을 변경한다.*/
    smrLogHeadI::setReplStmtDepth( &sUpdateLog.mHead,
                                   smLayerCallback::getLstReplStmtDepth( aTrans ) );

    smrLogHeadI::setFlag(&sUpdateLog.mHead, sLogFlag);

    SC_MAKE_GRID( sUpdateLog.mGRID,
                  aHeader->mSpaceID,
                  sPageID,
                  SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aFixedRow ) );

    sUpdateLog.mData        = aHeader->mSelfOID;
    sUpdateLog.mType        = SMR_SMC_PERS_UPDATE_INPLACE_ROW;

    sLogSize = SMR_LOGREC_SIZE(smrUpdateLog)
        + sBImgSize + sAImgSize + ID_SIZEOF(smrLogTail);

    if( aIsReplSenderSend == SMC_LOG_REPL_SENDER_SEND_OK )
    {
        sPrimaryKeySize = getPrimaryKeySize( aHeader, aFixedRow );
        sLogSize += sPrimaryKeySize;
    }

    smrLogHeadI::setSize(&sUpdateLog.mHead, sLogSize);
    sUpdateLog.mAImgSize    = sAImgSize;
    sUpdateLog.mBImgSize    = sBImgSize;

    smrLogHeadI::setPrevLSN( &sUpdateLog.mHead, smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &(sUpdateLog),
                                                 0,
                                                 SMR_LOGREC_SIZE(smrUpdateLog) )
              != IDE_SUCCESS );

    *aPrimaryKeySize = sPrimaryKeySize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_column_size );
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    SM_TRC_MRECORD_INVALID_COL_SIZE,
                    aHeader->mSelfOID,
                    __LINE__,
                    sColumn->offset,
                    sCurValue->length);

        IDE_SET( ideSetErrorCode(smERR_ABORT_INVALID_COLUMN_SIZE) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Update Inplace로그시 기록하는 Log의 Before Image기록.
 *             ( BUG-47366로 인하여 redo 에서 before image를 참조한다.
 *               log변경시 undo 뿐 아니라 Redo도 수정해 주어야 한다.)
 *
 *   Befor  Image: 각각의 Update되는 Column에 대해서
 *      Fixed   Column : Flag(SChar) | Offset(UInt) |
 *                     ColumnID(UInt) | SIZE(UInt) | Value
 *
 *      Var/LOB Column : Flag(SChar) | Offset(UInt) |  ColumnID(UInt) | SIZE(UInt)
 *            In  Mode : Value
 *            Out Mode : if replicated table, value
                         Frist Variable Column Piece OID
 *      (LOB)          : PieceCount | firstLPCH | Frist LOB Column Piece OID
 *
 * aTrans            - [IN] Transaction Pointer
 * aFixedRow         - [IN] Update되는 Row Pointer
 * aColumnList       - [IN] Update될 Column List
 * aValueList        - [IN] Update될 Value List
 * aLogOffset        - [IN-OUT] Transaction Log Buffer에서 Offset
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeUIPBfrLg2TxLBf(void                 * aTrans,
                                            smcLogReplOpt          aIsReplSenderRead,
                                            const SChar          * aFixedRow,
                                            smOID                  aAfterOID,
                                            const smiColumnList  * aColumnList,
                                            UInt                 * aLogOffset,
                                            UInt                   aUtdVarColCnt,
                                            ULong                  aModifyIdxBit)
{
    const smiColumn       * sColumn         = NULL;
    const smiColumnList   * sCurColumnList  = NULL;
    smVCPieceHeader       * sVCPieceHeader  = NULL;
    smVCDesc              * sVCDesc         = NULL;
    smOID                   sVCPieceOID     = SM_NULL_OID;
    UInt                    sVCSize         = 0;
    UShort                  sVCColCount     = 0;
    UInt                    sCIDOffset      = 0;
    UInt                    sCIDFence       = 0;
    UInt                    sMax            = ID_UINT_MAX;  // BUG-43744

    UInt                    sBImgSize       = 0;

    IDE_ERROR( aTrans != NULL );

    /* 로그의 Before Image와 After Image 크기를 구한다.*/
    sCurColumnList = aColumnList;
    sColumn = sCurColumnList->column;

    /* BUG-47615 update inplace 에서 갱신되는 index 의 list를 기록한다.
     * Restart Recovery에서는 아직 index가 없으므로 사용되지 않고,
     * undo에서 index의 key를 복원 하기 위해 사용한다.*/
    IDE_TEST( smLayerCallback::writeLogToBuffer(
                  aTrans,
                  &aModifyIdxBit,
                  *aLogOffset,
                  ID_SIZEOF(ULong))
             != IDE_SUCCESS );

    *aLogOffset += ID_SIZEOF(aModifyIdxBit);

    /* proj-2419 united var logging */
    sVCPieceOID = ((smpSlotHeader*)aFixedRow)->mVarOID;

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sVCPieceOID, /* first OID of united var piece */
                                                 *aLogOffset,
                                                 ID_SIZEOF(smOID) )
              != IDE_SUCCESS );
    *aLogOffset += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        if ( sVCPieceOID != aAfterOID )
        {
            /* BUG-43744: null포함하는 variable 컬럼 개수 */
            sVCColCount = aUtdVarColCnt;
        }
        else
        {
            sVCColCount = 0;
        }

        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &sVCColCount, /* Column count in United var piece */
                                                     *aLogOffset,
                                                     ID_SIZEOF(UShort) )
                  != IDE_SUCCESS );
        *aLogOffset += ID_SIZEOF(UShort);

        if ( sVCColCount > 0 )
        {
            /* CID offset 저장했다가 column 순회할때 id 를 로깅한다 */
            sCIDOffset = *aLogOffset;

            *aLogOffset += ID_SIZEOF(UInt) * sVCColCount;

            sCIDFence   = *aLogOffset;

            //BUG-43744
            while ( sCIDOffset < sCIDFence )
            {
                IDE_TEST(smLayerCallback::writeLogToBuffer( aTrans,
                                                            &sMax,
                                                            sCIDOffset,
                                                            ID_SIZEOF(UInt))
                         != IDE_SUCCESS);

                sCIDOffset += ID_SIZEOF(UInt);
            }

            sCIDOffset -= (ID_SIZEOF(UInt) * sVCColCount);

            /* Variable Column Value 기록. */
            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( sColumn->colSpace,
                                                   sVCPieceOID,
                                                   (void**)&sVCPieceHeader )
                            == IDE_SUCCESS );

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->nxtPieceOID),
                                                             *aLogOffset,
                                                             ID_SIZEOF(smOID) )
                          != IDE_SUCCESS );
                *aLogOffset += ID_SIZEOF(smOID);


                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->colCount),
                                                             *aLogOffset,
                                                             ID_SIZEOF(UShort) )
                          != IDE_SUCCESS );
                *aLogOffset += ID_SIZEOF(UShort);

                sVCSize = getUnitedVCSize(sVCPieceHeader);

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             (SChar*)(sVCPieceHeader + 1),
                                                             *aLogOffset,
                                                             (UInt)sVCSize )
                          != IDE_SUCCESS );

                *aLogOffset += (UInt)sVCSize;

                sVCPieceOID = sVCPieceHeader->nxtPieceOID;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    for( ;
         sCurColumnList != NULL;
         sCurColumnList  = sCurColumnList->next )
    {
        sColumn = sCurColumnList->column;

        // BUG-37460 update log 에서 compress column 이 고려되지 않았습니다.
        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
    
                    sVCDesc = (smVCDesc*)(aFixedRow + sColumn->offset);
    
                    IDE_TEST( writeUInplaceColumnLog( aTrans,
                                                    SMC_LOG_REPL_SENDER_SEND_NO,
                                                    sColumn,
                                                    aLogOffset,
                                                    aFixedRow + sColumn->offset,
                                                    sVCDesc->length,
                                                    SMC_UI_LOG_WRITE_TYPE_BEFORIMG )
                            != IDE_SUCCESS );
    
                    break;
    
                case SMI_COLUMN_TYPE_VARIABLE:
                    /* var image 는 united var log 에 포함되어있다
                     * 위에서 공간 남겨둔 united var column id 를 로깅한다 */

                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 &(sColumn->id),
                                                                 sCIDOffset + ((sColumn->varOrder)*ID_SIZEOF(UInt)),
                                                                 ID_SIZEOF(UInt) )
                              != IDE_SUCCESS );

                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                    sVCDesc = (smVCDesc*)(aFixedRow + sColumn->offset);

                    IDE_TEST( writeUInplaceColumnLog( aTrans,
                                                      aIsReplSenderRead,
                                                      sColumn,
                                                      aLogOffset,
                                                      aFixedRow + sColumn->offset,
                                                      sVCDesc->length,
                                                      SMC_UI_LOG_WRITE_TYPE_BEFORIMG )
                            != IDE_SUCCESS );
               
                    break;
    
                case SMI_COLUMN_TYPE_FIXED:
    
                    IDE_TEST( writeUInplaceColumnLog( aTrans,
                                                    aIsReplSenderRead,
                                                    sColumn,
                                                    aLogOffset,
                                                    aFixedRow + sColumn->offset,
                                                    sColumn->size,
                                                    SMC_UI_LOG_WRITE_TYPE_BEFORIMG )
                            != IDE_SUCCESS );
    
                    break;
    
                default:
                    /* Column은 Lob, Variable, Fixed 이어야 한다. */
                    IDE_ERROR_MSG( 0,
                                "sColumn->id    :%"ID_UINT32_FMT"\n"
                                "sColumn->flag  :%"ID_UINT32_FMT"\n"
                                "sColumn->offset:%"ID_UINT32_FMT"\n"
                                "sColumn->vcInOu:%"ID_UINT32_FMT"\n"
                                "sColumn->size  :%"ID_UINT32_FMT"\n",
                                sColumn->id,
                                sColumn->flag,
                                sColumn->offset,
                                sColumn->vcInOutBaseSize,
                                sColumn->size );
                    break;
            }
        }
        else
        {
            IDE_TEST( writeUInplaceColumnLog( aTrans,
                                              aIsReplSenderRead,
                                              sColumn,
                                              aLogOffset,
                                              aFixedRow + sColumn->offset,
                                              ID_SIZEOF(smOID),
                                              SMC_UI_LOG_WRITE_TYPE_BEFORIMG )
                      != IDE_SUCCESS );
        }
    }

    sBImgSize = *aLogOffset + ID_SIZEOF(UInt) - SMR_LOGREC_SIZE(smrUpdateLog);

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sBImgSize, /* first OID of united var piece */
                                                 *aLogOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    *aLogOffset += ID_SIZEOF(UInt);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Update Inplace로그시 기록하는 Log의 After Image기록.
 *
 *   After  Image: 각각의 Update되는 Column에 대해서
 *      Fixed   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) |
 *                     SIZE(UInt) | Value
 *
 *      Var/LOB Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) |
 *                     SIZE(UInt)
 *            In  Mode : Value
 *            Out Mode : OID 들.
 *     (LOB)           : PieceCount | OID 들
 *
 * aTrans            - [IN] Transaction Pointer
 * aFixedRowOID      - [IN] Replication Sender가 읽어서 보내는 로그인가?
 * aFixedRow         - [IN] Update되는 Row Pointer
 * aColumnList       - [IN] Update될 Column List
 * aValueList        - [IN] Update될 Value List
 * aLogOffset        - [IN-OUT] Transaction Log Buffer에서 Offset
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeUIPAftLg2TxLBf(void                 * aTrans,
                                            smcLogReplOpt          aIsReplSenderRead,
                                            const SChar          * aFixedRow,
                                            smOID                  aBeforeOID,
                                            const smiColumnList  * aColumnList,
                                            const smiValue       * aValueList,
                                            UInt                 * aLogOffset,
                                            UInt                   aUtdVarColCnt)
{
    const smiValue        * sValue          = NULL;
    const smiColumn       * sColumn         = NULL;
    const smiColumnList   * sCurColumnList  = NULL;
    smVCPieceHeader       * sVCPieceHeader  = NULL;
    smOID                   sVCPieceOID     = SM_NULL_OID;
    UInt                    sVCSize         = 0;
    UShort                  sVCColCount     = 0;
    UInt                    sCIDOffset      = 0;
    UInt                    sCIDFence       = 0;
    UInt                    sMax            = ID_UINT_MAX;  // BUG-43744

    IDE_ERROR( aTrans != NULL );

    sCurColumnList  = aColumnList;
    sColumn         = sCurColumnList->column;
    sValue          = aValueList;

    /* proj-2419 united var logging */
    sVCPieceOID = ((smpSlotHeader*)aFixedRow)->mVarOID;

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sVCPieceOID, /* first OID of united var piece */
                                                 *aLogOffset,
                                                 ID_SIZEOF(smOID) )
              != IDE_SUCCESS );
    *aLogOffset += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        if ( sVCPieceOID != aBeforeOID )
        {
            /* BUG-43744: null포함하는 variable 컬럼 개수 */
            sVCColCount = aUtdVarColCnt;
        }
        else
        {
            sVCColCount = 0;
        }

        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &sVCColCount, /* Column count in United var piece */
                                                     *aLogOffset,
                                                     ID_SIZEOF(UShort) )
                  != IDE_SUCCESS );
        *aLogOffset += ID_SIZEOF(UShort);

        if ( sVCColCount > 0 )
        {
            /* CID offset 저장했다가 column 순회할때 id 를 로깅한다 */
            sCIDOffset = *aLogOffset;

            *aLogOffset += ID_SIZEOF(UInt) * sVCColCount;

            sCIDFence   = *aLogOffset;

            //BUG-43744
            while ( sCIDOffset < sCIDFence )
            {
                IDE_TEST(smLayerCallback::writeLogToBuffer( aTrans,
                                                            &sMax,
                                                            sCIDOffset,
                                                            ID_SIZEOF(UInt))
                         != IDE_SUCCESS);

                sCIDOffset += ID_SIZEOF(UInt);
            }

            sCIDOffset -= (ID_SIZEOF(UInt) * sVCColCount);

            /* Variable Column Value 기록. */
            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( sColumn->colSpace,
                                                   sVCPieceOID,
                                                   (void**)&sVCPieceHeader )
                            == IDE_SUCCESS );

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->nxtPieceOID),
                                                             *aLogOffset,
                                                             ID_SIZEOF(smOID) )
                          != IDE_SUCCESS );
                *aLogOffset += ID_SIZEOF(smOID);


                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &(sVCPieceHeader->colCount),
                                                             *aLogOffset,
                                                             ID_SIZEOF(UShort) )
                          != IDE_SUCCESS );
                *aLogOffset += ID_SIZEOF(UShort);

                sVCSize = getUnitedVCSize(sVCPieceHeader);

                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             (SChar*)(sVCPieceHeader + 1),
                                                             *aLogOffset,
                                                             (UInt)sVCSize )
                          != IDE_SUCCESS );

                *aLogOffset += (UInt)sVCSize;

                sVCPieceOID = sVCPieceHeader->nxtPieceOID;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    for( ;
         sCurColumnList != NULL;
         sCurColumnList  = sCurColumnList->next, sValue++ )
    {
        sColumn = sCurColumnList->column;

        // BUG-37460 update log 에서 compress column 이 고려되지 않았습니다.
        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
                    IDE_TEST( writeUInplaceColumnLog( aTrans,
                                                    aIsReplSenderRead,
                                                    sColumn,
                                                    aLogOffset,
                                                    aFixedRow + sColumn->offset,
                                                    sValue->length,
                                                    SMC_UI_LOG_WRITE_TYPE_AFTERIMG )
                            != IDE_SUCCESS );
    
                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                    /* var image 는 united var log 에 포함되어있다
                     * 위에서 공간 남겨둔 united var column id 를 로깅한다 */

                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 &(sColumn->id),
                                                                 sCIDOffset + ((sColumn->varOrder)*ID_SIZEOF(UInt)),
                                                                 ID_SIZEOF(UInt) )
                              != IDE_SUCCESS );

                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                    IDE_TEST( writeUInplaceColumnLog( aTrans,
                                                      aIsReplSenderRead,
                                                      sColumn,
                                                      aLogOffset,
                                                      aFixedRow + sColumn->offset,
                                                      sValue->length,
                                                      SMC_UI_LOG_WRITE_TYPE_AFTERIMG )
                            != IDE_SUCCESS );

                    break;

                case SMI_COLUMN_TYPE_FIXED:
    
                    IDE_TEST( writeUInplaceColumnLog( aTrans,
                                                    aIsReplSenderRead,
                                                    sColumn,
                                                    aLogOffset,
                                                    (const SChar*)(sValue->value),
                                                    sValue->length,
                                                    SMC_UI_LOG_WRITE_TYPE_AFTERIMG )
                            != IDE_SUCCESS );
    
                    break;
    
                default:
                    /* Column은 Lob, Variable, Fixed 이어야 한다. */
                    IDE_ERROR_MSG( 0,
                                "sColumn->id    :%"ID_UINT32_FMT"\n"
                                "sColumn->flag  :%"ID_UINT32_FMT"\n"
                                "sColumn->offset:%"ID_UINT32_FMT"\n"
                                "sColumn->vcInOu:%"ID_UINT32_FMT"\n"
                                "sColumn->size  :%"ID_UINT32_FMT"\n",
                                sColumn->id,
                                sColumn->flag,
                                sColumn->offset,
                                sColumn->vcInOutBaseSize,
                                sColumn->size );
                    break;
            }
        }
        else
        {
            IDE_TEST( writeUInplaceColumnLog( aTrans,
                                              aIsReplSenderRead,
                                              sColumn,
                                              aLogOffset,
                                              (const SChar*)(sValue->value),
                                              ID_SIZEOF(smOID),
                                              SMC_UI_LOG_WRITE_TYPE_AFTERIMG )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SMR_SMC_PERS_UPDATE_INPLACE_ROW 의 Update 로그의
 *               Undo Function 이다.
 *
 * Befor  Image: 각각의 Update되는 Column에 대해서
 *         Fixed   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *                        | Value
 *
 *         Var/LOB Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SMP_VCDESC_MODE_OUT:
 *                        | Value | OID
 *        (LOB)           | Value | PieceCount | firstLPCH(pointer) | OID
 *               SMP_VCDESC_MODE_IN:
 *                        | Value
 *
 * 부가설명 :  Flag   (SChar) : SMP_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 *
 * aTID      - [IN] Transaction ID
 * aPID      - [IN] Page ID
 * aOffset   - [IN] Offset
 * aData     - [IN] Table ID
 * aImage    - [IN] After Image
 * aSize     - [IN] After Image Size
 * aFlag     - [IN] smrLogHeader의 Flag.
 ***********************************************************************/
IDE_RC smcRecordUpdate::undo_SMC_PERS_UPDATE_INPLACE_ROW(
                                                   smTID      aTID,
                                                   scSpaceID  aSpaceID,
                                                   scPageID   aPID,
                                                   scOffset   aOffset,
                                                   vULong     aData,
                                                   SChar     *aImage,
                                                   SInt       aSize,
                                                   UInt       aFlag)
{
    SChar            sColumnLogFlag;
    UInt             sColOffset     = 0;
    UInt             sValueSize     = 0;
    smcLobDesc     * sLobDesc       = NULL;
    smVCDesc       * sVCDesc        = NULL;
    smVCDescInMode * sVCDescInMode  = NULL;
    SChar          * sCurImage      = NULL;
    SChar          * sFence         = NULL;
    smOID            sTableOID      = SM_NULL_OID;
    smOID            sFixRowID      = SM_NULL_OID;
    smOID            sVCPieceOID    = SM_NULL_OID;
    smOID            sNxtPieceOID   = SM_NULL_OID;
    scPageID         sFixPageID     = SM_NULL_PID;
    SChar          * sFixRow        = NULL;
    SChar          * sVCPiecePtr    = NULL;
    UShort           sVarColumnCnt  = 0;
    UShort           sVCPieceSize   = 0;
    smVCPieceHeader* sVCPieceHeader = NULL;
    void           * sTrans         = NULL;
    ULong            sModifyIdxBit;

    // BUG-47366
    SChar          * sAftImage      = NULL;

    sTableOID   = (smOID)aData;
    sFixRowID   = SM_MAKE_OID(aPID, aOffset);
    sFixPageID  = aPID;
    sCurImage   = aImage;
    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                       sFixRowID,
                                       (void**)&sFixRow )
                == IDE_SUCCESS );

    idlOS::memcpy( &sModifyIdxBit, sCurImage, ID_SIZEOF(sModifyIdxBit) );
    sCurImage += ID_SIZEOF(sModifyIdxBit);

    if ( sModifyIdxBit != 0 )
    {
        if(smrRecoveryMgr::isRestart() == ID_FALSE)
        {
            IDE_TEST( deleteRowFromTBIdx( aSpaceID,
                                          sTableOID,
                                          sFixRowID,
                                          sModifyIdxBit )
                      != IDE_SUCCESS );
        }
    }

    /* PROJ-2419 united variable column redo */
    idlOS::memcpy(&sVCPieceOID, sCurImage, ID_SIZEOF(smOID));

    sCurImage += ID_SIZEOF(smOID);

    ((smpSlotHeader*)sFixRow)->mVarOID = sVCPieceOID;

    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* get Column count in united var piece */
        idlOS::memcpy(&sVarColumnCnt, sCurImage, ID_SIZEOF(UShort));

        sCurImage += ID_SIZEOF(UShort);

        /* update 에서 united var가 전혀 변경되지 않는 경우 */
        if ( sVarColumnCnt > 0 )
        {
            /* skip column IDs */
            sCurImage += ID_SIZEOF(UInt) * sVarColumnCnt;

            /* redo united var pieces*/
            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );

                /* get next piece oid */
                idlOS::memcpy(&sNxtPieceOID, sCurImage, ID_SIZEOF(smOID));

                sCurImage += ID_SIZEOF(smOID);

                /* get column count in this piece */
                idlOS::memcpy(&sVarColumnCnt, sCurImage, ID_SIZEOF(UShort));

                sCurImage += ID_SIZEOF(UShort);

                /* get VC Piece Size
                 * 이 값은 offset array 의 끝에 있으므로 해당 값만 가져온후
                 * sCurImage를 증가시키지 않고 value 복사할때 전부 한번에 다시 복사한다 */
                idlOS::memcpy( &sVCPieceSize,
                               sCurImage + ( ID_SIZEOF(UShort) * sVarColumnCnt ),
                               ID_SIZEOF(UShort));

                /* offset 이므로 header size 를 제외해야 value length 가 된다*/
                sVCPieceSize -= ID_SIZEOF(smVCPieceHeader);

                /* set VC piece header */
                sVCPieceHeader              = (smVCPieceHeader*)sVCPiecePtr;
                sVCPieceHeader->nxtPieceOID = sNxtPieceOID;
                sVCPieceHeader->colCount    = sVarColumnCnt;

                /* get VC value */
                idlOS::memcpy( sVCPiecePtr + ID_SIZEOF(smVCPieceHeader),
                               sCurImage,
                               sVCPieceSize );

                sCurImage += sVCPieceSize;

                // BUG-47366 UnitedVar의 AllocSlot에 대한 Redo 수행
                sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
                sVCPieceHeader->flag &= ~SM_VCPIECE_TYPE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_TYPE_OTHER;

                IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                       SM_MAKE_PID(sVCPieceOID))
                        != IDE_SUCCESS);

                sVCPieceOID = sNxtPieceOID;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-47366 BeforeImg 끝에 Size가 추가되었다. Fence 값은 Size를 빼주어야 한다. */
    sFence = aImage + aSize - ID_SIZEOF(UInt);

    while( sCurImage < sFence )
    {
        /* Log 의 Flag값을 가져온다. SMP_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit) */
        idlOS::memcpy(&sColumnLogFlag, sCurImage, ID_SIZEOF(SChar));
        sCurImage += ID_SIZEOF(SChar);

        /* Offset */
        idlOS::memcpy(&sColOffset, sCurImage, ID_SIZEOF(UInt));
        sCurImage += ID_SIZEOF(UInt);

        IDE_ERROR( sColOffset < SM_PAGE_SIZE );

        /* Skip Column ID */
        sCurImage += ID_SIZEOF(UInt);

        /* Value Size */
        idlOS::memcpy(&sValueSize, sCurImage, ID_SIZEOF(UInt));
        sCurImage += ID_SIZEOF(UInt);

        switch (sColumnLogFlag & SMI_COLUMN_TYPE_MASK)
        {
            case SMI_COLUMN_TYPE_FIXED:
                idlOS::memcpy(sFixRow + sColOffset, sCurImage, sValueSize);
                sCurImage += sValueSize;

                break;

            case SMI_COLUMN_TYPE_LOB:

                if( sValueSize != 0 )
                {
                    sLobDesc = (smcLobDesc*)(sFixRow + sColOffset);

                    idlOS::memcpy(&(sLobDesc->mLPCHCount),
                                  sCurImage,
                                  ID_SIZEOF(UInt));
                    sCurImage += ID_SIZEOF(UInt);

                    idlOS::memcpy(&(sLobDesc->mFirstLPCH),
                                  sCurImage,
                                  ID_SIZEOF(smcLPCH*));
                    sCurImage += ID_SIZEOF(smcLPCH*);
                }
                // LOB 처리를 VARIABLE에서 이어서 한다.
            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                if( sValueSize == 0 )
                {
                    sVCDescInMode = (smVCDescInMode*)(sFixRow + sColOffset);

                    sVCDescInMode->flag   = SM_VCDESC_MODE_IN;
                    sVCDescInMode->length = 0;
                }
                else
                {
                    if( (sColumnLogFlag & SMI_COLUMN_MODE_MASK)
                        == SM_VCDESC_MODE_OUT )
                    {
                        //PROJ-1608 Recovery From Replication
                        //SMR_LOG_TYPE_REPL_RECOVERY는 Recovery Sender가 사용 함
                        if(((aFlag & SMR_LOG_TYPE_MASK) == SMR_LOG_TYPE_NORMAL) ||
                           ((aFlag & SMR_LOG_TYPE_MASK) == SMR_LOG_TYPE_REPL_RECOVERY))
                        {
                            // BUG-37433 Lob은 REPL_SENDER NO로 처리한다.
                            if( SMI_IS_LOB_COLUMN(aFlag) != ID_TRUE )
                            {
                                /* BUG-40282 LOB 아닌 VC만 기록 */
                                sCurImage += sValueSize;
                            }
                        }

                        /* Fixed Row에 있는 VC Desc를 Setting한다. */
                        sVCDesc = (smVCDesc*)(sFixRow + sColOffset);
                        sVCDesc->flag = SM_VCDESC_MODE_OUT;
                        sVCDesc->length = sValueSize;

                        idlOS::memcpy(&(sVCDesc->fstPieceOID), sCurImage, ID_SIZEOF(smOID));

                        sCurImage += ID_SIZEOF(smOID);
                    }
                    else
                    {
                        /* SMP_VCDESC_MODE_IN */
                        sVCDescInMode = (smVCDescInMode*)(sFixRow + sColOffset);

                        sVCDescInMode->flag   = SM_VCDESC_MODE_IN;
                        sVCDescInMode->length = sValueSize;

                        idlOS::memcpy(sVCDescInMode + 1, sCurImage, sValueSize);
                        sCurImage += sValueSize;
                    }
                }

                break;

            default:
                IDE_ERROR_MSG( 0,
                               "sColOffset     :%"ID_UINT32_FMT"\n"
                               "sColumnLogFlag :%"ID_UINT32_FMT"\n",
                               sColOffset,
                               sColumnLogFlag );
                break;
        }
    }

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, sFixPageID) != IDE_SUCCESS);

    if ( sModifyIdxBit != 0 )
    {
        if( smrRecoveryMgr::isRestart() == ID_FALSE )
        {
            sTrans = smLayerCallback::getTransByTID( aTID );

            IDE_TEST( insertRow2TBIdx( sTrans,
                                       aSpaceID,
                                       sTableOID,
                                       sFixRowID,
                                       sModifyIdxBit )
                      != IDE_SUCCESS );
        }
    }

    /* BUG-47366 UnitedVar의 AllocSlot에 대한 Undo 수행 */
    sAftImage = aImage + aSize;

    idlOS::memcpy(&sVCPieceOID, sAftImage, ID_SIZEOF(smOID));
    sAftImage += ID_SIZEOF(smOID);
    
    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* get Column count in united var piece */
        idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));
        sAftImage += ID_SIZEOF(UShort);

        /* update 에서 united var가 전혀 변경되지 않는 경우 */
        if ( sVarColumnCnt > 0 )
        {
            /* skip column IDs */
            sAftImage += ID_SIZEOF(UInt) * sVarColumnCnt;

            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );

                sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                
                sVCPieceHeader->flag = SM_VCPIECE_FREE_OK | SM_VCPIECE_TYPE_OTHER;

                IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                          != IDE_SUCCESS);

                /* get next piece oid */
                idlOS::memcpy(&sNxtPieceOID, sAftImage, ID_SIZEOF(smOID));
                sAftImage += ID_SIZEOF(smOID);

                /* get column count in this piece */
                idlOS::memcpy(&sVarColumnCnt, sAftImage, ID_SIZEOF(UShort));

                sAftImage += ID_SIZEOF(UShort);

                /* get VC Piece Size */
                idlOS::memcpy( &sVCPieceSize,
                               sAftImage + ( ID_SIZEOF(UShort) * sVarColumnCnt ),
                               ID_SIZEOF(UShort));

                /* offset 이므로 헤더를 제외해야 사이즈가 된다 */
                sVCPieceSize -= ID_SIZEOF(smVCPieceHeader);

                sAftImage += sVCPieceSize;

                sVCPieceOID = sNxtPieceOID;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sTrans = smLayerCallback::getTransByTID( aTID );

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTrans,
                                               SMR_RTOI_TYPE_MEMPAGE,
                                               sTableOID,
                                               0, /* IndexID */
                                               aSpaceID,
                                               aPID );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SMR_SMC_PERS_UPDATE_INPLACE_ROW 의 Update 로그의
 *               Redo Function 이다.
 *
 * After  Image: 각각의 Update되는 Column에 대해서
 *         Fixed   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) |
 *                        SIZE(UInt) | Value
 *
 *         Var/LOB Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SMP_VCDESC_MODE_OUT:
 *                        | Value | OID 들...
 *
 *        (LOB)           | Value | PieceCount | OID 들...
 *               SMP_VCDESC_MODE_IN:
 *                        | Value
 * 부가설명 :  Flag   (SChar) : SMP_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 *
 * aTID      - [IN] Transaction ID
 * aPID      - [IN] Page ID
 * aOffset   - [IN] Offset
 * aData     - [IN] Table ID
 * aImage    - [IN] After Image
 * aSize     - [IN] After Image Size
 * aFlag     - [IN] None
 ***********************************************************************/
IDE_RC smcRecordUpdate::redo_SMC_PERS_UPDATE_INPLACE_ROW(
                                                   smTID      aTID,
                                                   scSpaceID  aSpaceID,
                                                   scPageID   aPID,
                                                   scOffset   aOffset,
                                                   vULong     aData,
                                                   SChar     *aImage,
                                                   SInt       aSize,
                                                   UInt       /*aFlag*/)
{
    SChar            sColumnLogFlag;
    UInt             sColOffset         = 0;
    UInt             sValueSize         = 0;
    smcLobDesc      *sLobDesc           = NULL;
    smVCDesc        *sVCDesc            = NULL;
    smVCDescInMode  *sVCDescInMode      = NULL;
    smVCPieceHeader *sVCPieceHeader     = NULL;
    UInt             sVCPieceCnt        = 0;
    SChar           *sVCPieceValuePtr   = NULL;
    UInt             sVCPartSize        = 0;
    SChar           *sCurImage          = NULL;
    SChar           *sFence             = NULL;
    smOID            sTableOID          = SM_NULL_OID;
    smOID            sFixRowID          = SM_NULL_OID;
    SChar          * sFixRow            = NULL;
    scPageID         sFixPageID         = SM_NULL_PID;
    UInt             i                  = 0;
    smOID            sVCPieceOID        = SM_NULL_OID;
    smOID            sNxtPieceOID       = SM_NULL_OID;
    SChar          * sVCPiecePtr        = NULL;
    UShort           sVCPieceSize       = 0;
    UShort           sVarColumnCnt      = 0;
    void           * sTransPtr          = NULL;

    /* BUG-47366 */
    SChar          * sBfrImage          = NULL;
    UInt             sBImgSize          = 0;

    sTransPtr   = smLayerCallback::getTransByTID( aTID );

    sTableOID   = (smOID)aData;
    sFixRowID   = SM_MAKE_OID(aPID, aOffset);
    sFixPageID  = aPID;
    sCurImage   = aImage;
    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                       sFixRowID,
                                       (void**)&sFixRow )
                == IDE_SUCCESS );

    /* PROJ-2419 united variable column redo */
    idlOS::memcpy(&sVCPieceOID, sCurImage, ID_SIZEOF(smOID));

    sCurImage += ID_SIZEOF(smOID);

    ((smpSlotHeader*)sFixRow)->mVarOID = sVCPieceOID;

    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* get Column count in united var piece */
        idlOS::memcpy(&sVarColumnCnt, sCurImage, ID_SIZEOF(UShort));

        sCurImage += ID_SIZEOF(UShort);

        /* update 에서 united var가 전혀 변경되지 않는 경우 */
        if ( sVarColumnCnt > 0 )
        {
            /* skip column IDs */
            sCurImage += ID_SIZEOF(UInt) * sVarColumnCnt;

            /* redo united var pieces*/
            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );

                /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
                   수행되어야 한다.*/
                if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
                {
                    /* for global transaction, add OID into OID_List */
                    IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                            sTableOID,
                                                            sVCPieceOID,
                                                            aSpaceID,
                                                            SM_OID_NEW_VARIABLE_SLOT )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }

                /* get next piece oid */
                idlOS::memcpy(&sNxtPieceOID, sCurImage, ID_SIZEOF(smOID));

                sCurImage += ID_SIZEOF(smOID);

                /* get column count in this piece */
                idlOS::memcpy(&sVarColumnCnt, sCurImage, ID_SIZEOF(UShort));

                sCurImage += ID_SIZEOF(UShort);

                /* get VC Piece Size
                 * 이 값은 value에 달린 array 의 끝에 있으므로 해당 값만 가져온후
                 * sCurImage를 손대지 않고 value 복사할때 전부 한번에 다시 복사한다 */
                idlOS::memcpy( &sVCPieceSize,
                               sCurImage + ( ID_SIZEOF(UShort) * sVarColumnCnt ),
                               ID_SIZEOF(UShort));

                sVCPieceSize -= ID_SIZEOF(smVCPieceHeader);

                /* set VC piece header */
                sVCPieceHeader              = (smVCPieceHeader*)sVCPiecePtr;
                sVCPieceHeader->nxtPieceOID = sNxtPieceOID;
                sVCPieceHeader->colCount    = sVarColumnCnt;

                /* get VC value */
                idlOS::memcpy( sVCPiecePtr + ID_SIZEOF(smVCPieceHeader),
                               sCurImage,
                               sVCPieceSize );

                sCurImage += sVCPieceSize;

                // BUG-47366 UnitedVar의 AllocSlot에 대한 Redo 수행
                sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
                sVCPieceHeader->flag &= ~SM_VCPIECE_TYPE_MASK;
                sVCPieceHeader->flag |= SM_VCPIECE_TYPE_OTHER;

                IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                       SM_MAKE_PID(sVCPieceOID))
                        != IDE_SUCCESS);

                sVCPieceOID = sNxtPieceOID;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    sFence = aImage + aSize;

    while( sCurImage < sFence )
    {
        /* Log 의 Flag값을 가져온다. SMP_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit) */
        idlOS::memcpy(&sColumnLogFlag, sCurImage, ID_SIZEOF(SChar));
        sCurImage += ID_SIZEOF(SChar);

        /* Offset */
        idlOS::memcpy(&sColOffset, sCurImage, ID_SIZEOF(UInt));
        sCurImage += ID_SIZEOF(UInt);

        IDE_ERROR( sColOffset < SM_PAGE_SIZE );

        /* Skip Column ID */
        sCurImage += ID_SIZEOF(UInt);

        /* Value Size */
        idlOS::memcpy(&sValueSize, sCurImage, ID_SIZEOF(UInt));
        sCurImage += ID_SIZEOF(UInt);

        switch (sColumnLogFlag & SMI_COLUMN_TYPE_MASK)
        {
            case SMI_COLUMN_TYPE_FIXED:
                idlOS::memcpy(sFixRow + sColOffset, sCurImage, sValueSize);
                sCurImage += sValueSize;

                break;

            case SMI_COLUMN_TYPE_LOB:
                // LOB 처리를 VARIABLE에서 이어서 한다.
            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                if( sValueSize == 0 )
                {
                    sVCDescInMode = (smVCDescInMode*)(sFixRow + sColOffset);

                    sVCDescInMode->flag   = SM_VCDESC_MODE_IN;
                    sVCDescInMode->length = 0;
                }
                else
                {
                    if( (sColumnLogFlag & SMI_COLUMN_MODE_MASK)
                        == SM_VCDESC_MODE_OUT )
                    {
                        /* Fixed Row에 있는 VC Desc를 Setting한다. */
                        sVCDesc = (smVCDesc*)(sFixRow + sColOffset);
                        sVCDesc->flag = SM_VCDESC_MODE_OUT;
                        sVCDesc->length = sValueSize;

                        /* Get Value */
                        sVCPieceValuePtr = sCurImage;
                        sVCPartSize = sValueSize;
                        sCurImage  += sValueSize;

                        /* Get VC Piece Count */
                        // BUG-28089 Wrong REDO for UPDATE_INPLACE_ROW
                        idlOS::memcpy( &sVCPieceCnt, sCurImage, ID_SIZEOF(UInt) );
                        sCurImage += ID_SIZEOF(UInt);

                        // BUG-28089 Wrong REDO for UPDATE_INPLACE_ROW
                        // OUT-MODE LOB의 경우 LobDesc에 Piece Count를 설정함
                        if( (sColumnLogFlag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
                        {
                            /* PROJ-2118 codesonar 오류로 상위에서 옮겨옴
                             * sLobDesc는 여기에서만 사용함 */
                            sLobDesc = (smcLobDesc*)(sFixRow + sColOffset);
                            sLobDesc->mLPCHCount = sVCPieceCnt;
                        }

                        /* Get OID */
                        idlOS::memcpy(&(sVCDesc->fstPieceOID), sCurImage, ID_SIZEOF(smOID));

                        /* VC Piece영역에 대해서 Redo연산을 수행한다. */
                        for( i = 0; i < sVCPieceCnt; i++ )
                        {
                            idlOS::memcpy( &sVCPieceOID, sCurImage, ID_SIZEOF( smOID ) );
                            sCurImage += ID_SIZEOF( smOID );

                            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                                               sVCPieceOID,
                                                               (void**)&sVCPiecePtr )
                                        == IDE_SUCCESS );

                            /* BUG-14558:OID List에 대한 Add는 Transaction
                               Begin되었을 때만 수행되어야 한다.*/
                            if ( smLayerCallback::IsBeginTrans( sTransPtr )
                                 == ID_TRUE )
                            {
                                /* for global transaction, add OID into OID_List */
                                IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                                        sTableOID,
                                                                        sVCPieceOID,
                                                                        aSpaceID,
                                                                        SM_OID_NEW_VARIABLE_SLOT )
                                          != IDE_SUCCESS );
                            }

                            if( i == (sVCPieceCnt - 1) )
                            {
                                sVCPieceSize = sVCPartSize;
                            }
                            else
                            {
                                sVCPieceSize = SMP_VC_PIECE_MAX_SIZE;
                            }

                            /* BUG-15354: [A4] SM VARCHAR 32K: Varchar저장시 PieceHeader에 대한 logging이
                             * 누락되어 PieceHeader에 대한 Redo, Undo가 되지않음. */
                            sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                            sVCPieceHeader->length = sVCPieceSize;

                            /* Redo VC Piece영역의 Value영역을 Redo한다 */
                            idlOS::memcpy( sVCPiecePtr + ID_SIZEOF(smVCPieceHeader),
                                           sVCPieceValuePtr,
                                           sVCPieceSize );

                            IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                                   SM_MAKE_PID(sVCPieceOID))
                                     != IDE_SUCCESS);

                            sVCPieceValuePtr += sVCPieceSize;
                            sVCPartSize -= sVCPieceSize;
                        }
                    }
                    else
                    {
                        /* SMP_VCDESC_MODE_IN */
                        sVCDescInMode = (smVCDescInMode*)(sFixRow + sColOffset);

                        sVCDescInMode->flag   = SM_VCDESC_MODE_IN;
                        sVCDescInMode->length = sValueSize;

                        idlOS::memcpy(sVCDescInMode + 1, sCurImage, sValueSize);
                        sCurImage += sValueSize;
                    }
                }

                break;

            default:
                IDE_ERROR_MSG( 0,
                               "sColOffset     :%"ID_UINT32_FMT"\n"
                               "sColumnLogFlag :%"ID_UINT32_FMT"\n",
                               sColOffset,
                               sColumnLogFlag );
                break;
        }
    }

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, sFixPageID) != IDE_SUCCESS);


    /* BUG-47366 UnitedVar의 Oldversion의 Free 수행 */
    /* Before Img의 Size를 구한다. */
    sBfrImage = aImage;
    sBfrImage -= ID_SIZEOF(UInt);
    idlOS::memcpy(&sBImgSize, sBfrImage, ID_SIZEOF(UInt));

    sBfrImage = aImage - sBImgSize;
    sBfrImage += ID_SIZEOF( ULong ); /* sModifyIdxBit BUG-47632,47615 */

    idlOS::memcpy(&sVCPieceOID, sBfrImage, ID_SIZEOF(smOID));
    sBfrImage += ID_SIZEOF(smOID);
    
    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* get Column count in united var piece */
        idlOS::memcpy(&sVarColumnCnt, sBfrImage, ID_SIZEOF(UShort));
        sBfrImage += ID_SIZEOF(UShort);

        /* update 에서 united var가 전혀 변경되지 않는 경우 */
        if ( sVarColumnCnt > 0 )
        {
            /* skip column IDs */
            sBfrImage += ID_SIZEOF(UInt) * sVarColumnCnt;

            while ( sVCPieceOID != SM_NULL_OID )
            {
                IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                                   sVCPieceOID,
                                                   (void**)&sVCPiecePtr )
                            == IDE_SUCCESS );

                if ( smLayerCallback::IsBeginTrans( sTransPtr )
                     == ID_TRUE )
                {
                    /* for global transaction, add OID into OID_List */
                    IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                            sTableOID,
                                                            sVCPieceOID,
                                                            aSpaceID,
                                                            SM_OID_OLD_VARIABLE_SLOT )
                              != IDE_SUCCESS );
                }

                sVCPieceHeader = (smVCPieceHeader*)sVCPiecePtr;
                
                sVCPieceHeader->flag = SM_VCPIECE_FREE_OK | SM_VCPIECE_TYPE_OTHER;

                IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, SM_MAKE_PID( sVCPieceOID ) )
                          != IDE_SUCCESS);

                /* get next piece oid */
                idlOS::memcpy(&sNxtPieceOID, sBfrImage, ID_SIZEOF(smOID));
                sBfrImage += ID_SIZEOF(smOID);

                /* get column count in this piece */
                idlOS::memcpy(&sVarColumnCnt, sBfrImage, ID_SIZEOF(UShort));

                sBfrImage += ID_SIZEOF(UShort);

                /* get VC Piece Size */
                idlOS::memcpy( &sVCPieceSize,
                               sBfrImage + ( ID_SIZEOF(UShort) * sVarColumnCnt ),
                               ID_SIZEOF(UShort));

                /* offset 이므로 헤더를 제외해야 사이즈가 된다 */
                sVCPieceSize -= ID_SIZEOF(smVCPieceHeader);

                sBfrImage += sVCPieceSize;

                sVCPieceOID = sNxtPieceOID;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : DML중 Update시 발생하는 Log를 기록한다. 이 Update로그는
 *               만약 이 Table에 Replication이 걸려 있을 경우 Replication
 *               Sender가 읽어 들인다. 그리고 이런 Replication을 위해서
 *               Fixed Row와 Variable Log가 각기 다른 Page영역에 저장되지만
 *               하나의 DML 로그로 기록한다.
 *
 * LOG HEADER :  smrUpdateLog : SMR_SMC_PERS_DELETE_VERSION_ROW
 * BODY       :
 *     Before Image: None
 *
 *     After  Image: Delete Version Row ID(smOID)
 *
 *     Primary Key : Repl Sender가 이 로그를 읽어야 할때만 기록된다.
 *         Key마다 다음과 같은 데이타가 각각 존재
 *
 *        # PK SIZE(UInt) | PK Column Cnt(UInt)
 *          PK를 구성하는 Column들
 *         1. (Column ID | Length | DATA)
 *         2. (Column ID | Length | DATA)
 *           ...
 *         n. (Column ID | Length | DATA)
 *
 *      TASK-5030 Full XLog
 *      FXLog : Supplemental log가 설정되어 있는경우,
 *          Lob을 제외한 컬럼의 before value를 기록한다.
 *
 *          # FXLog SIZE(UInt) | FXLog COUNT(UInt)
 *            FXLog를 구성하는 Column 들
 *            1. (Column ID | Length | DATA)
 *            2. (Column ID | Length | DATA)
 *              ...
 *            n. (Column ID | Length | DATA)
 *
 * aTrans            - [IN] Transaction Pointer
 * aHeader           - [IN] Table Header Pointer
 * aRow              - [IN] Delete할 Row Pointer
 * aBfrNxt           - [IN] Delete할 부분의 befor img,
 * aAftNxt           - [IN] Delete할 부분의 after img
 *
 * aOpt              - [IN] 1. Log Flag에 SMR_LOG_ALLOC_FIXEDSLOT_OK을 원하면.
 *                             SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK,
 *                          2. Log Flag에 SMR_LOG_ALLOC_FIXEDSLOT_NO을 원하면.
 *                             SMC_MKLOGFLAG_SET_ALLOC_FIXED_NO
 *
 * aIsSetImpSvp      - [IN] 기록할 Log가 Implicit Savepoint 정보가 Setting되어 있으면
 *                          ID_TRUE, 아니면 ID_FALSE
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeRemoveVersionLog( void            * aTrans,
                                               smcTableHeader  * aHeader,
                                               SChar           * aRow,
                                               ULong             aBfrNxt,
                                               ULong             aAftNxt,
                                               smcMakeLogFlagOpt aOpt,
                                               idBool          * aIsSetImpSvp)
{
    smrUpdateLog      sUpdateLog;
    scPageID          sPageID;
    UInt              sPrimaryKeySize = 0;
    UInt              sLogSize;
    UInt              sOffset;
    idBool            sIsReplSenderSend;
    UInt              sLogFlag;
    smrLogType        sLogType;
    UInt              sColumnLength     = 0;
    UInt              i                 = 0;
    UInt              sReadPos          = 0;
    UInt              sRemainedReadSize = 0;
    UInt              sReadSize         = 0;
    UInt              sFullXLogSize     = 0;
    UInt              sFullXLogCnt      = 0;
    const smiColumn * sColumn;
    SChar           * sColumnPtr;
    SChar             sVarColumnData[SMP_VC_PIECE_MAX_SIZE];
    smVCDesc        * sVCDesc;
    smOID             sVCPieceOID       = SM_NULL_OID;    
    UInt              sTmpColCnt        = 0;    // var piece 플래그 변경되는 컬럼 갯수 
    UInt              sTmpOffsetCol     = 0;    // 컬럼 갯수 쓸 위치 

    sIsReplSenderSend = smcTable::needReplicate( aHeader, aTrans);

    sPageID = SMP_SLOT_GET_PID(aRow);

    sLogSize = 0;

    if( sIsReplSenderSend == ID_TRUE )
    {
        sPrimaryKeySize = getPrimaryKeySize(aHeader, aRow);
        sLogSize += sPrimaryKeySize;
    }

    /* TASK-5030
     * FXLog size 계산 */
    if( smcTable::isSupplementalTable( aHeader ) == ID_TRUE )
    {
        sFullXLogSize   = 0;
        sFullXLogCnt    = 0;

        for( i = 0 ; i < smcTable::getColumnCount(aHeader) ; i++ )
        {
            sColumn       = smcTable::getColumn( aHeader, i );
            sColumnLength = smcRecord::getColumnLen( sColumn, aRow );

            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
                    // LOB type은 제외
                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                case SMI_COLUMN_TYPE_FIXED:
                    sFullXLogSize += (ID_SIZEOF(UInt) + /* column id */
                                      ID_SIZEOF(UInt) + /* column length */
                                      sColumnLength);   /* column value */

                    sFullXLogCnt++;
                    break;

                default:
                    break;
            }
        }

        // set new log size
        sFullXLogSize   += (ID_SIZEOF(UInt) +   /* FXLog size */
                            ID_SIZEOF(UInt));   /* FXLog count */
        sLogSize        += sFullXLogSize;
    }


    smrLogHeadI::setType(&sUpdateLog.mHead, SMR_LT_UPDATE);
    smrLogHeadI::setTransID( &sUpdateLog.mHead, smLayerCallback::getTransID( aTrans ) );
    SC_MAKE_GRID( sUpdateLog.mGRID,
                  aHeader->mSpaceID,
                  sPageID,
                  SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRow ) );

    sUpdateLog.mData        = aHeader->mSelfOID;
    sUpdateLog.mType        = SMR_SMC_PERS_DELETE_VERSION_ROW;
    sUpdateLog.mAImgSize    = ID_SIZEOF(ULong);
    /* BUG-14959: Delete Undo시 이전 Image로 복구해야 함.*/
    sUpdateLog.mBImgSize    = ID_SIZEOF(ULong);

    sLogSize += (SMR_LOGREC_SIZE(smrUpdateLog) + sUpdateLog.mBImgSize +
        sUpdateLog.mAImgSize + ID_SIZEOF(smrLogTail));

    // To Fix PR-14581
    // sLogFlag = smrLogHeadI::getFlag(&sUpdateLog.mHead);
    IDE_TEST( makeLogFlag(aTrans,
                          aHeader,
                          SMR_SMC_PERS_DELETE_VERSION_ROW,
                          aOpt,
                          &sLogFlag)
              != IDE_SUCCESS );

    /* makeLogFlag가 설정되이후에 호출되어야 한다. 왜냐면 안에서
     * smLayerCallback::getLstReplStmtDepth가 return하는 값을 변경한다.*/
    smrLogHeadI::setReplStmtDepth( &sUpdateLog.mHead,
                                   smLayerCallback::getLstReplStmtDepth( aTrans ) );

    smrLogHeadI::setFlag(&sUpdateLog.mHead, sLogFlag);

    /* BUG-46854: Var Piece 플래그 변경 내용 로그 사이즈 계산을 추가 함에 
     * 따라서 setSize() 및 UpdateLog 헤더 쓰는 것을 뒤로 미룬다. 
     */ 

    smrLogHeadI::setPrevLSN( &sUpdateLog.mHead, smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    sOffset = 0;
    smLayerCallback::initLogBuffer( aTrans );

    sOffset += SMR_LOGREC_SIZE(smrUpdateLog);

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aBfrNxt,
                                                 sOffset,
                                                 ID_SIZEOF(ULong) )
              != IDE_SUCCESS);

    sOffset += ID_SIZEOF(ULong);

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aAftNxt,
                                                 sOffset,
                                                 ID_SIZEOF(ULong) )
              != IDE_SUCCESS);

    sOffset += ID_SIZEOF(ULong);

    if( sIsReplSenderSend == ID_TRUE )
    {
        IDE_TEST( writePrimaryKeyLog(aTrans,
                                     aHeader,
                                     aRow,
                                     sPrimaryKeySize,
                                     sOffset)
                  != IDE_SUCCESS);

        sOffset += sPrimaryKeySize;
    }

    /* TASK-5030 */
    if( smcTable::isSupplementalTable( aHeader ) == ID_TRUE )
    {
        // Full XLOG Size - UInt
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &sFullXLogSize, 
                                                     sOffset,
                                                     ID_SIZEOF(UInt) )
                  != IDE_SUCCESS);
        sOffset += ID_SIZEOF(UInt);

        // Full XLOG Count - UInt
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &sFullXLogCnt, 
                                                     sOffset,
                                                     ID_SIZEOF(UInt) )
                  != IDE_SUCCESS );
        sOffset += ID_SIZEOF(UInt);

        for( i = 0 ; i < smcTable::getColumnCount(aHeader) ; i++ )
        {
            sColumn       = smcTable::getColumn( aHeader, i );
            sColumnLength = smcRecord::getColumnLen( sColumn, aRow );

            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
                    // LOB type은 제외
                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    // Variable Column ID - UInt
                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 &(sColumn->id), 
                                                                 sOffset,
                                                                 ID_SIZEOF(UInt) )
                              != IDE_SUCCESS );
                    sOffset += ID_SIZEOF(UInt);

                    // Variable Column Length - UInt
                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 &(sColumnLength),
                                                                 sOffset,
                                                                 ID_SIZEOF(UInt) )
                              != IDE_SUCCESS );
                    sOffset += ID_SIZEOF(UInt);

                    // Variable Column Value
                    sRemainedReadSize   = sColumnLength;
                    sReadPos            = 0;

                    while( sRemainedReadSize > 0 )
                    {
                        if( sRemainedReadSize < SMP_VC_PIECE_MAX_SIZE )
                        {
                            sReadSize = sRemainedReadSize;
                        }
                        else
                        {
                            sReadSize = SMP_VC_PIECE_MAX_SIZE;
                        }

                        sColumnPtr = smcRecord::getVarRow( aRow,
                                                           sColumn,
                                                           sReadPos,
                                                           &sReadSize,
                                                           (SChar *)sVarColumnData,
                                                           ID_FALSE );

                        IDE_ASSERT( sColumnPtr != NULL );

                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     sColumnPtr,
                                                                     sOffset,
                                                                     sReadSize )
                                  != IDE_SUCCESS );
                        sOffset += sReadSize;

                        sRemainedReadSize -= sReadSize;
                        sReadPos          += sReadSize;
                    }

                    break;

                case SMI_COLUMN_TYPE_FIXED:
                    // Fixed Column ID - UInt
                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 &(sColumn->id), 
                                                                 sOffset,
                                                                 ID_SIZEOF(UInt) )
                              != IDE_SUCCESS );
                    sOffset += ID_SIZEOF(UInt);

                    // Fixed Column Length - UInt
                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 &(sColumnLength),
                                                                 sOffset,
                                                                 ID_SIZEOF(UInt) )
                              != IDE_SUCCESS );
                    sOffset += ID_SIZEOF(UInt);


                    // Fixed Column Value
                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 aRow + sColumn->offset,
                                                                 sOffset,
                                                                 sColumnLength )
                              != IDE_SUCCESS );
                    sOffset += sColumnLength;

                    break;

                default:
                    break;
            }
        }
    }
   
    /* BUG-46854: Var Piece 플래그 로깅 추가 */ 
 
    // BUG-46854: 컬럼 개수 로깅 할 위치 미리 계산해 놓기
    sTmpOffsetCol = sOffset;
    sOffset  += ID_SIZEOF(UInt);
    sLogSize += ID_SIZEOF(UInt);

    // BUG-46854: Var Piece 플래그 변경 로그 추가 
    for ( i = 0 ; i < smcTable::getColumnCount(aHeader) ; i++ )
    {
        sColumn = smcTable::getColumn( aHeader, i );
        
        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            //첫번째 VCPiece 가져오기  
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_LOB:
                    sVCDesc = smcRecord::getVCDesc(sColumn, aRow);
                    sVCPieceOID = sVCDesc->fstPieceOID;

                    if ( ( sVCDesc->flag & SM_VCDESC_MODE_MASK ) == SM_VCDESC_MODE_OUT )
                    {
                        IDE_TEST( writeVCPieceFlagLog( aTrans,
                                                       aHeader,
                                                       &sOffset, 
                                                       &sLogSize, 
                                                       sVCPieceOID )
                                  != IDE_SUCCESS );     
                        sTmpColCnt++;      
                    }
                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    sVCDesc = smcRecord::getVCDesc(sColumn, aRow);
                    sVCPieceOID = sVCDesc->fstPieceOID;

                    if ( !SM_VCDESC_IS_MODE_IN( sVCDesc ) )
                    {
                        IDE_TEST( writeVCPieceFlagLog( aTrans,
                                                       aHeader,
                                                       &sOffset, 
                                                       &sLogSize, 
                                                       sVCPieceOID )
                                  != IDE_SUCCESS ); 
                        sTmpColCnt++;
                    }
                    break;

                case SMI_COLUMN_TYPE_VARIABLE:
                case SMI_COLUMN_TYPE_FIXED:
                default:
                    break;
            }
        }
    }       

    sVCPieceOID = ((smpSlotHeader*)aRow)->mVarOID;
    IDE_TEST ( writeVCPieceFlagLog( aTrans,
                                    aHeader, 
                                    &sOffset, 
                                    &sLogSize, 
                                    sVCPieceOID ) 
               != IDE_SUCCESS );        
    sTmpColCnt++;     
    /* BUG-46854: 플래그 로깅 추가 끝 */

    // BUG-46854: 플래그 변경된 var piece 컬럼을 가진 개수 로깅
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sTmpColCnt,
                                                 sTmpOffsetCol,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS);
    
    // BUG-46854: 로그 사이즈 셋팅 및 업데이트 로그헤더 로깅 
    smrLogHeadI::setSize(&sUpdateLog.mHead, sLogSize);
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &(sUpdateLog),
                                                 0,
                                                 SMR_LOGREC_SIZE(smrUpdateLog) )
          != IDE_SUCCESS );

    sLogType = smrLogHeadI::getType(&sUpdateLog.mHead);
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sLogType,
                                                 sOffset,
                                                 ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );
    smrLogHeadI::setType(&sUpdateLog.mHead, sLogType);

    IDE_TEST( smLayerCallback::writeTransLog( aTrans, sUpdateLog.mData ) != IDE_SUCCESS );

    if((smrLogHeadI::getFlag(&sUpdateLog.mHead) & SMR_LOG_SAVEPOINT_MASK) ==
       SMR_LOG_SAVEPOINT_OK)
    {
        *aIsSetImpSvp = ID_TRUE;
    }
    else
    {
        *aIsSetImpSvp = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-46854: Var Piece 플래그 변경 로그 씀. */
IDE_RC smcRecordUpdate::writeVCPieceFlagLog( void            * aTrans,
                                             smcTableHeader  * aHeader,
                                             UInt            * aOffset,
                                             UInt            * aLogSize, 
                                             smOID             aVCPieceOID )
{                                       
    SChar*  sVCPiecePtr       = NULL;
    smOID   sVCPieceOID       = aVCPieceOID; 
    smOID   sNxtVCPieceOID    = SM_NULL_OID;
    UShort  sBfrVCPieceFlag;
    UShort  sAftVCPieceFlag;
    UInt    sTmpPieceCnt      = 0;
    
    // 피스 개수 로깅 자리 예약
    UInt    sTmpOffset = *aOffset;
    *aOffset   += ID_SIZEOF(UInt);
    *aLogSize  += ID_SIZEOF(UInt);

    while ( sVCPieceOID != SM_NULL_OID )
    {
        // VCPieceOID 로깅
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &sVCPieceOID,
                                                     *aOffset,
                                                     ID_SIZEOF(smOID) )
                  != IDE_SUCCESS);

        *aOffset += ID_SIZEOF(smOID);
        *aLogSize += ID_SIZEOF(smOID);

        IDE_ASSERT( smmManager::getOIDPtr( aHeader->mSpaceID,
                                           sVCPieceOID,
                                           (void**)&sVCPiecePtr )
                    == IDE_SUCCESS );

        sNxtVCPieceOID = ((smVCPieceHeader*)sVCPiecePtr)->nxtPieceOID;

        // before Flag 로깅
        sBfrVCPieceFlag = ((smVCPieceHeader*)sVCPiecePtr)->flag;
        
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &sBfrVCPieceFlag,
                                                     *aOffset,
                                                     ID_SIZEOF(UShort) )
                  != IDE_SUCCESS);

        *aOffset += ID_SIZEOF(UShort);
        *aLogSize += ID_SIZEOF(UShort);

        // after Flag 로깅
        sAftVCPieceFlag = sBfrVCPieceFlag;
        sAftVCPieceFlag &= ~SM_VCPIECE_FREE_MASK;
        sAftVCPieceFlag |= SM_VCPIECE_FREE_OK;

        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &sAftVCPieceFlag,
                                                     *aOffset,
                                                     ID_SIZEOF(UShort) )
                  != IDE_SUCCESS);

        *aOffset += ID_SIZEOF(UShort);
        *aLogSize += ID_SIZEOF(UShort);


        sVCPieceOID = sNxtVCPieceOID;
        sTmpPieceCnt++;
    }

    //(*aColCnt)++;

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sTmpPieceCnt,
                                                 sTmpOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*>>>>>>>>>>>>>> redo: SMR_SMC_PERS_DELETE_VERSION_ROW                        */
IDE_RC smcRecordUpdate::redo_SMC_PERS_DELETE_VERSION_ROW(
    smTID       aTID,
    scSpaceID   aSpaceID,
    scPageID    aPID,
    scOffset    aOffset,
    vULong      aData,
    SChar     * aImage,     // BUG-46854: VAR 피스 플래그 로그 처리하기 위함 
    SInt        aSize,      // BUG-46854: VAR 피스 플래그 로그 처리하기 위함 
    UInt        aFlag)      // BUG-46854: VAR 피스 플래그 로그 처리하기 위함
{
    smpSlotHeader    *sSlotHeader;
    smOID             sRowID;
    smOID             sTableOID;
    void             *sTransPtr;
    smSCN             sDeleteSCN;
    UInt              sColCnt   = 0;
    UInt              sPieceCnt = 0;
    smOID             sVCPieceOID;
    UShort            sBefFlag;
    UShort            sAftFlag;
    idBool            sBeginTx = ID_FALSE;
    smVCPieceHeader  *sVCPieceHeader;
    UChar            *sLogPtr;
    UInt              sFullXLogSize;
    UInt              sPrimaryKeySize;
    UInt              i;
    UInt              j;

    sLogPtr = (UChar*)aImage;

    sTableOID = (smOID)aData;
    sRowID = SM_MAKE_OID( aPID, aOffset );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                       sRowID,
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );
    sTransPtr     = smLayerCallback::getTransByTID( aTID );

    SM_INIT_SCN( &sDeleteSCN );
    SM_SET_SCN_DELETE_BIT( &sDeleteSCN );
    SM_SET_SCN( &(sSlotHeader->mLimitSCN), &sDeleteSCN );

    /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
       수행되어야 한다.*/
    if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::addOID( sTransPtr,
                                           sTableOID,
                                           sRowID,
                                           aSpaceID,
                                           SM_OID_DELETE_FIXED_SLOT )
                  != IDE_SUCCESS );

        sBeginTx = ID_TRUE;;
    }

    // BUG-46854: Var 피스 플래그 로그 처리 
    // afterimage 크기 만큼 포인터 이동
    sLogPtr += aSize;
    
    // 프라이머리 키 있을 때
    if( ( aFlag & SMR_LOG_RP_INFO_LOG_MASK ) == SMR_LOG_RP_INFO_LOG_OK )
    {
        idlOS::memcpy(&sPrimaryKeySize, sLogPtr, ID_SIZEOF(UInt));
        sLogPtr += sPrimaryKeySize;
    }

    // Full XLog있을 때
    if( ( aFlag & SMR_LOG_FULL_XLOG_MASK ) == SMR_LOG_FULL_XLOG_OK )
    {
        idlOS::memcpy(&sFullXLogSize, sLogPtr, ID_SIZEOF(UInt));
        sLogPtr += sFullXLogSize;
    }    

    idlOS::memcpy(&sColCnt, sLogPtr, ID_SIZEOF(UInt));
    sLogPtr += ID_SIZEOF(UInt);

    for ( i = 0 ; i < sColCnt ; i++ )
    {
        idlOS::memcpy(&sPieceCnt, sLogPtr, ID_SIZEOF(UInt));
        sLogPtr += ID_SIZEOF(UInt);

        for ( j = 0 ; j < sPieceCnt ; j++ )
        {
            idlOS::memcpy(&sVCPieceOID, sLogPtr, ID_SIZEOF(smOID));
            sLogPtr += ID_SIZEOF(smOID);

            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                               sVCPieceOID,
                                               (void**)&sVCPieceHeader )
                        == IDE_SUCCESS );

            /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
               수행되어야 한다.*/
            if ( sBeginTx == ID_TRUE )
            {
                /* ---------------------------------------------
                   for global transaction, add OID into OID_List
                   --------------------------------------------- */
                IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                        (smOID)aData,
                                                        sVCPieceOID,
                                                        aSpaceID,
                                                        SM_OID_OLD_VARIABLE_SLOT )
                          != IDE_SUCCESS );
            }

            idlOS::memcpy(&sBefFlag, sLogPtr, ID_SIZEOF(UShort));
            sLogPtr += ID_SIZEOF(UShort);

            idlOS::memcpy(&sAftFlag, sLogPtr, ID_SIZEOF(UShort));
            sLogPtr += ID_SIZEOF(UShort);

            idlOS::memcpy(&(sVCPieceHeader->flag),
                          &sAftFlag,
                          ID_SIZEOF(UShort));
            
            if ( aPID != SM_MAKE_PID( sVCPieceOID ))
            {
                IDE_TEST(smmDirtyPageMgr::insDirtyPage( aSpaceID, 
                                                        SM_MAKE_PID( sVCPieceOID ) )
                         != IDE_SUCCESS);
            }
        }
    }

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*>>>>>>>>>>>>>> undo: SMR_SMC_PERS_DELETE_VERSION_ROW                        */
IDE_RC smcRecordUpdate::undo_SMC_PERS_DELETE_VERSION_ROW(
    smTID       aTID,
    scSpaceID   aSpaceID,
    scPageID    aPID,
    scOffset    aOffset,
    vULong      aData,
    SChar      *aImage,
    SInt        aSize,
    UInt        aFlag)
{
    smpSlotHeader    *sSlotHeader;
    smOID             sRowID;
    smOID             sTableOID;
    void             *sTransPtr;
    smSCN             sBfrSCN;
    UInt              sColCnt   = 0;
    UInt              sPieceCnt = 0;
    smOID             sVCPieceOID;
    UShort            sBefFlag;
    UShort            sAftFlag;
    smVCPieceHeader  *sVCPieceHeader;
    UInt              sFullXLogSize;
    UInt              sPrimaryKeySize;
    UChar            *sLogPtr;
    UInt              i;
    UInt              j;    

    sLogPtr = (UChar*)aImage;   // before image 위치

    sTableOID = (smOID)aData;
    sRowID = SM_MAKE_OID( aPID, aOffset );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                       sRowID,
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );
    sTransPtr     = smLayerCallback::getTransByTID( aTID );

    if (smrRecoveryMgr::isRestart() == ID_FALSE)
    {
        IDE_TEST( smLayerCallback::undoDeleteOfTableInfo( sTransPtr, sTableOID )
                  != IDE_SUCCESS );
    }

    if(aSize == 0)
    {
        /* Before Image일 경우 */
        SM_SET_SCN_FREE_ROW( &(sSlotHeader->mLimitSCN) );

        if((aFlag & SMR_LOG_ALLOC_FIXEDSLOT_MASK) ==
           SMR_LOG_ALLOC_FIXEDSLOT_OK)
        {
            /* Undo가 필요없음. SCN값에 Delete Bit이 setting되어있기
               때문에 undo가 필요없다.*/
        }
    }
    else
    {
        /* BUG-15073 : Row의 Next OID는 Atomic하게 바뀌어야 한다.*/
        idlOS::memcpy(&sBfrSCN, aImage, ID_SIZEOF(ULong));

        //delete가 수행될수 있는 row의 next는 NULL아니면 LOCK이 걸린 ROW이다.
        IDE_ERROR( SM_SCN_IS_FREE_ROW( sBfrSCN ) || 
                   SM_SCN_IS_LOCK_ROW( sBfrSCN ) );
        /* After Image일 경우 */
        SM_SET_SCN( &(sSlotHeader->mLimitSCN), &sBfrSCN );
    }
    
    // BUG-46854: Var 피스 플래그 로그 처리 

    if( ( aFlag & SMR_LOG_CMPS_LOG_MASK ) != SMR_LOG_CMPS_LOG_OK )
    {
        // before + after image 크기만큼 포인터 뒤로 이동
        sLogPtr += ( ID_SIZEOF(ULong) + ID_SIZEOF(ULong) );
    }
    else
    {
        /* CLR 로그(롤백할 때 쓴 로그)에서는 before image만 씀. 
         * before image 크기만큼 포인터 뒤로 이동 */
        sLogPtr += ID_SIZEOF(ULong);
    }

    // 프라이머리 키 있을 때
    if( ( aFlag & SMR_LOG_RP_INFO_LOG_MASK ) == SMR_LOG_RP_INFO_LOG_OK )
    {
        idlOS::memcpy(&sPrimaryKeySize, sLogPtr, ID_SIZEOF(UInt));
        sLogPtr += sPrimaryKeySize;
    }

    // Full XLog있을 때
    if( ( aFlag & SMR_LOG_FULL_XLOG_MASK ) == SMR_LOG_FULL_XLOG_OK )
    {
        idlOS::memcpy(&sFullXLogSize, sLogPtr, ID_SIZEOF(UInt));
        sLogPtr += sFullXLogSize;
    }    

    idlOS::memcpy(&sColCnt, sLogPtr, ID_SIZEOF(UInt));
    sLogPtr += ID_SIZEOF(UInt);

    for ( i = 0 ; i < sColCnt ; i++ )
    {
        idlOS::memcpy(&sPieceCnt, sLogPtr, ID_SIZEOF(UInt));
        sLogPtr += ID_SIZEOF(UInt);

        for ( j = 0 ; j < sPieceCnt ; j++ )
        {
            idlOS::memcpy(&sVCPieceOID, sLogPtr, ID_SIZEOF(smOID));
            sLogPtr += ID_SIZEOF(smOID);

            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                               sVCPieceOID,
                                               (void**)&sVCPieceHeader )
                        == IDE_SUCCESS );

            idlOS::memcpy(&sBefFlag, sLogPtr, ID_SIZEOF(UShort));
            sLogPtr += ID_SIZEOF(UShort);
           
            // CLR 로그(롤백할 때 쓴 로그)에서는 before 플래그만 쓰여짐. 
            if( ( aFlag & SMR_LOG_CMPS_LOG_MASK ) == SMR_LOG_CMPS_LOG_NO )
            {
                idlOS::memcpy(&sAftFlag, sLogPtr, ID_SIZEOF(UShort));
                sLogPtr += ID_SIZEOF(UShort);
            }
           
            idlOS::memcpy(&(sVCPieceHeader->flag),
                          &sBefFlag,
                          ID_SIZEOF(UShort));
            
            if ( aPID != SM_MAKE_PID( sVCPieceOID ))
            {
                IDE_TEST(smmDirtyPageMgr::insDirtyPage( aSpaceID, 
                                                        SM_MAKE_PID( sVCPieceOID ) )
                         != IDE_SUCCESS);
            }
        }
    }
   

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                               SMR_RTOI_TYPE_MEMPAGE,
                                               sTableOID,
                                               0, /* IndexID */
                                               aSpaceID,
                                               aPID );

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW                    */
IDE_RC smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_FIXED_ROW( smTID        aTID,
                                                             scSpaceID    aSpaceID,
                                                             scPageID     aPID,
                                                             scOffset     aOffset,
                                                             vULong       aData,
                                                             SChar       *aImage,
                                                             SInt         aSize,
                                                             UInt       /*aFlag*/)
{
    SChar             *sRow;
    void              *sTransPtr;

    IDE_ERROR(aSize == ID_SIZEOF(smpSlotHeader));

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sRow )
                == IDE_SUCCESS );

    idlOS::memcpy(sRow, aImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( smrRecoveryMgr::isRedo() == ID_FALSE )
    {
        sTransPtr     = smLayerCallback::getTransByTID( aTID );

        smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                                   SMR_RTOI_TYPE_MEMPAGE,
                                                   aData,
                                                   0, /* IndexID */
                                                   aSpaceID,
                                                   aPID );

        return IDE_FAILURE;
    }
    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_FREE     */
IDE_RC smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_FREE(
    smTID        aTID,
    scSpaceID    aSpaceID,
    scPageID     aPID,
    scOffset     aOffset,
    vULong       aData,
    SChar    * /*aImage*/,
    SInt       /*aSize*/,
    UInt       /*aFlag*/)
{
    SChar             *sRow;
    void              *sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sRow )
                == IDE_SUCCESS );

    idlOS::memcpy(sRow + SMP_SLOT_HEADER_SIZE, (SChar*)&aData, ID_SIZEOF(smOID));

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( smrRecoveryMgr::isRedo() == ID_FALSE )
    {
        sTransPtr     = smLayerCallback::getTransByTID( aTID );

        smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                                   SMR_RTOI_TYPE_MEMPAGE,
                                                   aData,
                                                   0, /* IndexID */
                                                   aSpaceID,
                                                   aPID );

        return IDE_FAILURE;
    }
    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION     */
IDE_RC smcRecordUpdate::redo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION(
                                                              smTID     aTID,
                                                              scSpaceID aSpaceID,
                                                              scPageID  aPID,
                                                              scOffset  aOffset,
                                                              vULong    aData,
                                                              SChar    *aImage,
                                                              SInt      aSize,
                                                              UInt     /*aFlag*/)
{
    smpSlotHeader     *sSlotHeader;
    ULong              sHeaderNext;
    void              *sTransPtr;

    sTransPtr = smLayerCallback::getTransByTID( aTID );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );

    // To Fix BUG-14639
    IDE_ERROR( aSize == ID_SIZEOF(ULong) );
    idlOS::memcpy( &sHeaderNext, aImage, ID_SIZEOF(ULong) );

    SMP_SLOT_INIT_POSITION( sSlotHeader );
    SMP_SLOT_SET_USED( sSlotHeader );

    if( SM_IS_NULL_OID( sHeaderNext ) )
    {
        SM_SET_SCN_FREE_ROW( &(sSlotHeader->mLimitSCN) );
    }
    else
    {
        SM_INIT_SCN( &(sSlotHeader->mLimitSCN) );
        SM_SET_SCN_DELETE_BIT( &(sSlotHeader->mLimitSCN) );
        SMP_SLOT_SET_NEXT_OID( sSlotHeader, sHeaderNext );
    }

    /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
       수행되어야 한다.*/
    if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
    {
        /* ---------------------------------------------
           for global transaction, add OID into OID_List
           --------------------------------------------- */
        IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                (smOID)aData,
                                                SM_MAKE_OID(aPID, aOffset),
                                                aSpaceID,
                                                SM_OID_OLD_UPDATE_FIXED_SLOT )
                  != IDE_SUCCESS );
    }

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION     */
IDE_RC smcRecordUpdate::undo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION(
    smTID     aTID,
    scSpaceID aSpaceID,
    scPageID  aPID,
    scOffset  aOffset,
    vULong    aData,
    SChar    *aImage,
    SInt      aSize,
    UInt      /*aFlag*/)
{
    smpSlotHeader     *sSlotHeader;
    smSCN              sHeaderNext;
    void              *sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );

    if(aSize == 0)
    {
        // Before Fixing BUG-14639
        SM_SET_SCN_FREE_ROW( &(sSlotHeader->mLimitSCN) );
    }
    else
    {
        /* BUG-15073 : Row의 Next OID는 Atomic하게 바뀌어야 한다.*/
        idlOS::memcpy(&sHeaderNext, aImage, ID_SIZEOF(ULong));

        /* PROJ-1381 sHeaderNext is Lock SCN */
        IDE_ERROR_MSG( SM_SCN_IS_NOT_LOCK_ROW(sHeaderNext),
                       "CreateSCN : %"ID_XINT64_FMT", "
                       "LimitSCN : %"ID_XINT64_FMT", "
                       "Position : %"ID_XINT64_FMT", "
                       "HeaderNExt : %"ID_XINT64_FMT,
                       sSlotHeader->mCreateSCN,
                       sSlotHeader->mLimitSCN,
                       sSlotHeader->mPosition,
                       sHeaderNext );

        // After Fixing BUG-14639
        SM_SET_SCN( &(sSlotHeader->mLimitSCN), &sHeaderNext);
    }

    SMP_SLOT_INIT_POSITION( sSlotHeader );
    SMP_SLOT_SET_USED( sSlotHeader );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( smrRecoveryMgr::isRedo() == ID_FALSE )
    {
        sTransPtr     = smLayerCallback::getTransByTID( aTID );

        smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                                   SMR_RTOI_TYPE_MEMPAGE,
                                                   aData,
                                                   0, /* IndexID */
                                                   aSpaceID,
                                                   aPID );

        return IDE_FAILURE;
    }
 
    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG                */
IDE_RC smcRecordUpdate::redo_SMC_PERS_SET_FIX_ROW_DROP_FLAG(
                                                      smTID      /*aTID*/,
                                                      scSpaceID    aSpaceID,
                                                      scPageID     aPID,
                                                      scOffset     aOffset,
                                                      vULong       aData,
                                                      SChar     * /*aImage*/,
                                                      SInt        /*aSize*/,
                                                      UInt        /*aFlag*/)
{
    return smcTable::setTableHeaderDropFlag( aSpaceID,
                                             aPID,
                                             aOffset,
                                             (idBool)aData );
}

IDE_RC smcRecordUpdate::undo_SMC_PERS_SET_FIX_ROW_DROP_FLAG(
                                                      smTID         aTID,
                                                      scSpaceID     aSpaceID,
                                                      scPageID      aPID,
                                                      scOffset      aOffset,
                                                      vULong        aData,
                                                      SChar     * /*aImage*/,
                                                      SInt        /*aSize*/,
                                                      UInt        /*aFlag*/)
{
    void              *sTransPtr;

    aData = ( (idBool)aData == ID_TRUE ? ID_FALSE : ID_TRUE );

    /* Table에 대한 Drop 설정. */
    IDE_TEST( smcTable::setTableHeaderDropFlag( aSpaceID,
                                                aPID,
                                                aOffset,
                                                (idBool)aData )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sTransPtr = smLayerCallback::getTransByTID( aTID );

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                               SMR_RTOI_TYPE_TABLE,
                                               SM_MAKE_OID( aPID, aOffset ),
                                               0, /* IndexID */
                                               SC_NULL_SPACEID,
                                               SC_NULL_PID );

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT                */
IDE_RC smcRecordUpdate::redo_SMC_PERS_SET_FIX_ROW_DELETE_BIT(
                                                       smTID     /*aTID*/,
                                                       scSpaceID    aSpaceID,
                                                       scPageID     aPID,
                                                       scOffset     aOffset,
                                                       vULong     /*aData*/,
                                                       SChar    * /*aImage*/,
                                                       SInt       /*aSize*/,
                                                       UInt       /*aFlag*/)
{
    smpSlotHeader*  sSlotHeader;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );
    SM_SET_SCN_DELETE_BIT( &(sSlotHeader->mCreateSCN) );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcRecordUpdate::undo_SMC_PERS_SET_FIX_ROW_DELETE_BIT(
                                                       smTID         aTID,
                                                       scSpaceID     aSpaceID,
                                                       scPageID      aPID,
                                                       scOffset      aOffset,
                                                       vULong      /*aData*/,
                                                       SChar     * /*aImage*/,
                                                       SInt        /*aSize*/,
                                                       UInt        /*aFlag*/)
{
    smpSlotHeader  * sSlotHeader;
    void           * sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );
    SM_CLEAR_SCN_DELETE_BIT( &(sSlotHeader->mCreateSCN) );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sTransPtr = smLayerCallback::getTransByTID( aTID );

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                               SMR_RTOI_TYPE_MEMPAGE,
                                               SM_NULL_OID, /*TableOID */
                                               0, /* IndexID */
                                               aSpaceID,
                                               aPID );

    return IDE_FAILURE;
}


/* Update type:  SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD       */
IDE_RC smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_VAR_ROW_HEAD(
    smTID         aTID,
    scSpaceID     aSpaceID,
    scPageID      aPID,
    scOffset      aOffset,
    vULong      /*aData*/,
    SChar        *aImage,
    SInt          aSize,
    UInt        /*aFlag*/)
{
    SChar             *sRow;
    void              *sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sRow )
                == IDE_SUCCESS );

    idlOS::memcpy(sRow, aImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( smrRecoveryMgr::isRedo() == ID_FALSE )
    {
        sTransPtr     = smLayerCallback::getTransByTID( aTID );

        smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                                   SMR_RTOI_TYPE_MEMPAGE,
                                                   SM_NULL_OID,
                                                   0, /* IndexID */
                                                   aSpaceID,
                                                   aPID );

        return IDE_FAILURE;
    }
    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_VAR_ROW                 */
IDE_RC smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_VAR_ROW(
                                                    smTID      aTID,
                                                    scSpaceID  aSpaceID,
                                                    scPageID   aPID,
                                                    scOffset   aOffset,
                                                    vULong     /*aData*/,
                                                    SChar     *aImage,
                                                    SInt       /*aSize*/,
                                                    UInt       /*aFlag*/)
{
    SChar           *sRow;
    UShort           sSize;
    smVCPieceHeader *sVCPieceHeader;
    void            *sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sRow )
                == IDE_SUCCESS );

    sVCPieceHeader = (smVCPieceHeader*)sRow;

    idlOS::memcpy(&sSize, aImage, ID_SIZEOF(UShort));

    sVCPieceHeader->length = sSize;

    if(sSize != 0)
    {
        idlOS::memcpy(sVCPieceHeader + 1,
                      aImage + ID_SIZEOF(UShort),
                      sSize);
    }

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( smrRecoveryMgr::isRedo() == ID_FALSE )
    {
        sTransPtr     = smLayerCallback::getTransByTID( aTID );

        smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                                   SMR_RTOI_TYPE_MEMPAGE,
                                                   SM_NULL_OID,
                                                   0, /* IndexID */
                                                   aSpaceID,
                                                   aPID );

        return IDE_FAILURE;
    }

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_SET_VAR_ROW_FLAG         */
IDE_RC smcRecordUpdate::redo_SMC_PERS_SET_VAR_ROW_FLAG(
                                                        smTID      aTID,
                                                        scSpaceID  aSpaceID,
                                                        scPageID   aPID,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar     *aImage,
                                                        SInt     /*aSize*/,
                                                        UInt     /*aFlag*/)
{
    smVCPieceHeader  *sVCPieceHeader;
    smOID             sVCPieceOID = SM_MAKE_OID(aPID, aOffset);
    void             *sTransPtr;

    sTransPtr = smLayerCallback::getTransByTID( aTID );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sVCPieceHeader )
                == IDE_SUCCESS );

    /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
       수행되어야 한다.*/
    if ( smLayerCallback::IsBeginTrans( sTransPtr ) == ID_TRUE )
    {
        /* ---------------------------------------------
           for global transaction, add OID into OID_List
           --------------------------------------------- */
        IDE_TEST( smLayerCallback::addOIDByTID( aTID,
                                                (smOID)aData,
                                                sVCPieceOID,
                                                aSpaceID,
                                                SM_OID_OLD_VARIABLE_SLOT )
                  != IDE_SUCCESS );
    }

    idlOS::memcpy(&(sVCPieceHeader->flag),
                  aImage,
                  ID_SIZEOF(UShort));

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcRecordUpdate::undo_SMC_PERS_SET_VAR_ROW_FLAG(
                                                        smTID      aTID,
                                                        scSpaceID  aSpaceID,
                                                        scPageID   aPID,
                                                        scOffset   aOffset,
                                                        vULong     /*aData*/,
                                                        SChar     *aImage,
                                                        SInt       /*aSize*/,
                                                        UInt       /*aFlag*/)
{
    smVCPieceHeader *sVCPieceHeader;
    void            *sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sVCPieceHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sVCPieceHeader->flag), aImage, ID_SIZEOF(UShort));

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sTransPtr     = smLayerCallback::getTransByTID( aTID );

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                               SMR_RTOI_TYPE_MEMPAGE,
                                               SM_NULL_OID,
                                               0, /* IndexID */
                                               aSpaceID,
                                               aPID );

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_INDEX_SET_DROP_FLAG         */
IDE_RC smcRecordUpdate::redo_undo_SMC_INDEX_SET_DROP_FLAG( smTID      aTID,
                                                           scSpaceID  aSpaceID,
                                                           scPageID   aPID,
                                                           scOffset   aOffset,
                                                           vULong    ,/* aData */
                                                           SChar    * aImage,
                                                           SInt       aSize,
                                                           UInt       /* aFlag */ )
{
    smnIndexHeader  * sIndexHeader;
    void            * sTransPtr;
    UInt              sTmpFlag = 0;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );

    /* PROJ-2433
     * smnIndexHeader.mDropFlag가 UInt -> UShort로 변경됨 */
    if ( (UInt)aSize == ID_SIZEOF(UInt) )
    {
        /* 이전코드 */
        idlOS::memcpy( &sTmpFlag,
                       aImage,
                       ID_SIZEOF(UInt) );

        sIndexHeader->mDropFlag = (UShort)sTmpFlag;
    }
    else
    {
        /* 신코드 */
        idlOS::memcpy( &(sIndexHeader->mDropFlag),
                       aImage,
                       ID_SIZEOF(UShort) );
    }

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( smrRecoveryMgr::isRedo() == ID_FALSE )
    {
        sTransPtr     = smLayerCallback::getTransByTID( aTID );


        smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                                   SMR_RTOI_TYPE_MEMPAGE,
                                                   sIndexHeader->mTableOID,
                                                   sIndexHeader->mId,
                                                   SC_NULL_SPACEID,
                                                   SC_NULL_PID );

        return IDE_FAILURE;
    }

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_SET_VAR_ROW_NXT_OID     */
IDE_RC smcRecordUpdate::redo_undo_SMC_PERS_SET_VAR_ROW_NXT_OID(
                                                        smTID      aTID,
                                                        scSpaceID  aSpaceID,
                                                        scPageID   aPID,
                                                        scOffset   aOffset,
                                                        vULong     /*aData*/,
                                                        SChar     *aImage,
                                                        SInt       /*aSize*/,
                                                        UInt       /*aFlag*/)
{
    smVCPieceHeader *sVCPieceHeader;
    void            *sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sVCPieceHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sVCPieceHeader->nxtPieceOID), aImage, ID_SIZEOF(smOID));

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( smrRecoveryMgr::isRedo() == ID_FALSE )
    {
        sTransPtr     = smLayerCallback::getTransByTID( aTID );

        smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                                   SMR_RTOI_TYPE_MEMPAGE,
                                                   SM_NULL_OID,
                                                   0, /* IndexID */
                                                   aSpaceID,
                                                   aPID );

        return IDE_FAILURE;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Fixed Column에 관한 Update의 Log기록시 사용한다.
 *
 * aTrans      - [IN] Transaction Pointer
 * aColumn     - [IN] Column Desc
 * aLogOffset  - [IN-OUT] Transaction Log Buffer Offset
 * aValue      - [IN] Fixed Column Value
 * aLength     - [IN] Value Length
 *
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeFCLog4MVCC(void              *aTrans,
                                        const smiColumn   *aColumn,
                                        UInt              *aLogOffset,
                                        void              *aValue,
                                        UInt               aLength)
{
    /* Fixed Column Log Header : ID | SIZE  | Value */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &(aColumn->id) /* ID */,
                                                 *aLogOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    *aLogOffset += ID_SIZEOF(UInt);

    /* Fixed Column Length 기록 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &(aLength) /* Length */,
                                                 *aLogOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    *aLogOffset += ID_SIZEOF(UInt);

    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 aValue /* Value */,
                                                 *aLogOffset,
                                                 aLength )
              != IDE_SUCCESS );

    *aLogOffset += aLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Variable Column에 관한 Update의 Log기록시 사용한다.
 *
 * 1. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_OUT
 *    - Column ID(UInt) | Length(UInt) | Value | OID Cnt | OID List
 *
 * 2. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_IN
 *    - None (After Image인 경우는 Fixed Row에 In Mode로 저장되고 또한 Fixed
 *      Row에 대한 Logging을 별도로 수행하기 때문에 VC에 대한 Logging이 불필요.
 *
 * 3. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SMP_VCDESC_MODE_OUT
 *    - Column ID(UInt) | Length(UInt) | Value
 *
 * 4. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SMP_VCDESC_MODE_IN
 *    - Column ID(UInt) | Length(UInt) | Value
 *
 * aTrans   - [IN] Transaction Pointer
 * aColumn  - [IN] Column Desc
 * aVCDesc  - [IN] Fixed Row Pointer
 * aOffset  - [IN] After Image Size
 * aOption  - [IN] SMC_VC_LOG_WRITE_TYPE_AFTERIMG :
 *                   VC에 대한 After Image를 Logging시 이용.
 *
 *                 SMC_VC_LOG_WRITE_TYPE_BEFORIMG :
 *                   VC에 Before Image를 Logging시 이용.
 *
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeVCLog4MVCC(void              *aTrans,
                                        const smiColumn   *aColumn,
                                        smVCDesc          *aVCDesc,
                                        UInt              *aOffset,
                                        smcVCLogWrtOpt     aOption)
{
    smOID       sVCPieceOID;
    UInt        sOffset;
    UInt        sPieceCnt;
    
    smVCPieceHeader *sVCPieceHeader;
    SInt  sStoreMode = aVCDesc->flag & SM_VCDESC_MODE_MASK;

    /* sStoreMode == SMP_VCDESC_MODE_IN이고 After Image는 VC Log로
       기록하지 않는다. Fixed Row의 After Image에 포함되어 있다.*/
    if( sStoreMode == SM_VCDESC_MODE_OUT ||
        aOption == SMC_VC_LOG_WRITE_TYPE_BEFORIMG)
    {
        /* Variable Column ID기록 */
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &(aColumn->id),
                                                     *aOffset,
                                                     ID_SIZEOF(UInt) )
                  != IDE_SUCCESS );
        *aOffset += ID_SIZEOF(UInt);

        /* Variable Column Length 기록 */
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     &(aVCDesc->length),
                                                     *aOffset,
                                                     ID_SIZEOF(UInt) )
                  != IDE_SUCCESS );
        *aOffset += ID_SIZEOF(UInt);

        if( aVCDesc->length != 0 )
        {
            if( sStoreMode == SM_VCDESC_MODE_OUT )
            {
                IDE_TEST( writeVCValue4OutMode(aTrans,
                                               aColumn,
                                               aVCDesc,
                                               aOffset)
                          != IDE_SUCCESS );
            }
            else
            {
                /* In-Mode로 저장된 Variable Column Value저장 */
                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             (SChar*)(aVCDesc) + ID_SIZEOF(smVCDescInMode),
                                                             *aOffset,
                                                             aVCDesc->length )
                          != IDE_SUCCESS );
                *aOffset += aVCDesc->length;
            }

            if( aOption == SMC_VC_LOG_WRITE_TYPE_AFTERIMG &&
                sStoreMode == SM_VCDESC_MODE_OUT )
            {
                /* Variable Piece OID List 기록. */
                sVCPieceOID = aVCDesc->fstPieceOID;
                sPieceCnt = 0;

                sOffset = *aOffset;
                *aOffset += ID_SIZEOF(UInt);

                while(sVCPieceOID != SM_NULL_OID)
                {
                    sPieceCnt++;
                    IDE_ASSERT( smmManager::getOIDPtr( aColumn->colSpace,
                                                       sVCPieceOID,
                                                       (void**)&sVCPieceHeader )
                                == IDE_SUCCESS );

                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 &(sVCPieceOID),
                                                                 *aOffset,
                                                                 ID_SIZEOF(smOID) )
                              != IDE_SUCCESS );

                    *aOffset += ID_SIZEOF(smOID);

                    sVCPieceOID = sVCPieceHeader->nxtPieceOID;
                }

                /* Write Piece Count */
                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             &sPieceCnt,
                                                             sOffset,
                                                             ID_SIZEOF(UInt) )
                          != IDE_SUCCESS );

            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcRecordUpdate::writeVCValue4OutMode(void              *aTrans,
                                             const smiColumn   *aColumn,
                                             smVCDesc          *aVCDesc,
                                             UInt              *aOffset)
{
    smVCPieceHeader *sVCPieceHeader;
    smOID            sVCPieceOID;
    UInt             sLobLen;
    UInt             sPieceSize;
    smcLobDesc      *sLobDesc;

    if( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
        == SMI_COLUMN_TYPE_LOB )
    {
        sLobDesc = (smcLobDesc*)aVCDesc;
        sLobLen  = sLobDesc->length;
                    
        sVCPieceOID = aVCDesc->fstPieceOID;

        /* Lob Column Value 기록. */
        while(sVCPieceOID != SM_NULL_OID)
        {
            /*
             * Lob의 경우 Slot의 크기와 실제 value의 길이는 다를 수 있다.
             * 따라서 Lob Desc의 Lob Length를 기준으로 Value를 기록한다.
             */
            IDE_ASSERT( smmManager::getOIDPtr(
                            aColumn->colSpace,
                            sVCPieceOID,
                            (void**)&sVCPieceHeader )
                        == IDE_SUCCESS );

            if( sLobLen > SMP_VC_PIECE_MAX_SIZE )
            {
                IDE_ASSERT( sVCPieceHeader->length
                            == SMP_VC_PIECE_MAX_SIZE );
                            
                sPieceSize = sVCPieceHeader->length;
            }
            else
            {
                sPieceSize = sLobLen;
            }
                        
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                        (SChar*)(sVCPieceHeader + 1),
                                                        *aOffset,
                                                        sPieceSize )
                      != IDE_SUCCESS );

            *aOffset += sPieceSize;
            sLobLen  -= sPieceSize;

            sVCPieceOID = sVCPieceHeader->nxtPieceOID;
        }

        IDE_ASSERT( sLobLen == 0 );
    }
    else
    {
        sVCPieceOID = aVCDesc->fstPieceOID;

        /* Variable Column Value 기록. */
        while(sVCPieceOID != SM_NULL_OID)
        {
            IDE_ASSERT( smmManager::getOIDPtr(
                            aColumn->colSpace,
                            sVCPieceOID,
                            (void**)&sVCPieceHeader )
                        == IDE_SUCCESS );

            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         (SChar*)(sVCPieceHeader + 1),
                                                         *aOffset,
                                                         sVCPieceHeader->length )
                      != IDE_SUCCESS );

            *aOffset += sVCPieceHeader->length;

            sVCPieceOID = sVCPieceHeader->nxtPieceOID;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LOB Column의 Dummy Before Image를 기록.
 *
 * 1.  Column ID(UInt) | Length(UInt)
 *
 * aTrans   - [IN] Transaction Pointer
 * aOffset  - [IN] After Image Size
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeDummyBVCLog4Lob(void              *aTrans,
                                             UInt               aColumnID,
                                             UInt              *aOffset)
{
    UInt sLength = 0;

    /* LOB Column ID기록 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aColumnID,
                                                 *aOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    *aOffset += ID_SIZEOF(UInt);

    /* LOB Column Length 기록 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sLength,
                                                 *aOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    *aOffset += ID_SIZEOF(UInt);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Update Inplace로 Column을 Update시에 각 Column에 대해서
 *               Logging을 수행합니다. 이 Log는 Replication Sender가 읽기도
 *               하며 Recovery시 Redo, Undo시 이용합니다.
 *
 * 공통 Head:
 *    Flag   (SChar) : SM_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 *    Offset (UInt)  : Offset
 *    ID     (UInt)  : Column ID
 *    Length (UInt)  : Column Length
 * Body
 *    1. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_OUT | BEFORE
 *       OID    : Variable Column을 구성하는 첫번째 VC Piece OID
 *
 *    2. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_OUT | AFTER
 *       Value   : Variable Column Value
 *       OID Cnt : Lob Piece Count
 *       OID 들  : Variable Column을 구성하는 VC Piece OID들
 *
 *    3. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_IN
 *       Value  : Variable Column Value
 *
 *    4. SMI_COLUMN_TYPE_FIXED
 *       Value  : Fixed Column Value
 *
 *    5. SMI_COLUMN_TYPE_LOB | SM_VCDESC_MODE_OUT | BEFORE
 *       PieceCount : Lob Piece Count
 *       firstLPCH  : First Lob Piece Control Header
 *       OID        : LOB Column을 구성하는 첫번째 VC Piece OID
 *
 *    6. SMI_COLUMN_TYPE_LOB | SM_VCDESC_MODE_OUT | AFTER
 *       Value      : LOB Column Value
 *       OID Cnt    : Lob Piece Count
 *       OID 들     : LOB Column을 구성하는 VC Piece OID들
 *
 * aTrans      - [IN] Transaction Pointer
 * aIsReplSenderRead - [IN] Replication sender가 읽을 로그
 * aColumn     - [IN] Update될 Column Desc
 * aLogOffset  - [IN-OUT] Fixed Column Value
 * aValue      - [IN] Value
 * aLength     - [IN] Value Length
 * aOpt        - [IN] Before Image를 기록하면 : SMC_UI_LOG_WRITE_TYPE_BEFORIMG
 *                    After  Image를 기록하면 : SMC_UI_LOG_WRITE_TYPE_AFTORIMG
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeUInplaceColumnLog( void              *aTrans,
                                                smcLogReplOpt      aIsReplSenderRead,
                                                const smiColumn   *aColumn,
                                                UInt              *aLogOffset,
                                                const SChar       *aValue,
                                                UInt               aLength,
                                                smcUILogWrtOpt     aOpt)
{
    smOID            sVCPieceOID;
    SChar            sFlag;
    smcLobDesc      *sLobDesc;
    smVCDesc        *sVCDesc;
    smVCPieceHeader *sVCPieceHeader;
    SInt             sStoreMode;
    SInt             sType;
    UInt             sOffset;
    UInt             sOIDCnt;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aColumn != NULL );
    IDE_ERROR( aLogOffset != NULL );
    IDE_ERROR( aValue != NULL );
    IDE_ERROR( (aOpt == SMC_UI_LOG_WRITE_TYPE_AFTERIMG) ||
               (aOpt == SMC_UI_LOG_WRITE_TYPE_BEFORIMG) );

    IDE_ERROR( ( SM_VCDESC_MODE_MASK << 1 ) != SMI_COLUMN_TYPE_MASK );

    sType = aColumn->flag & SMI_COLUMN_TYPE_MASK;

    // BUG-37460 update log 에서 compress column 이 고려되지 않았습니다.
    if( ( aColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
          != SMI_COLUMN_COMPRESSION_TRUE )
    {
        switch( sType )
        {
            case SMI_COLUMN_TYPE_LOB:
                    // LOB 처리를 VARIABLE에서 동일하게 한다.
            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                sStoreMode = smcRecord::getVCStoreMode( aColumn, aLength );
                break;
    
            case SMI_COLUMN_TYPE_FIXED:
                sStoreMode = 0;
                break;
    
            default:
                IDE_ERROR_MSG( 0,
                            "sType          :%"ID_UINT32_FMT"\n"
                            "aColumn->id    :%"ID_UINT32_FMT"\n"
                            "aColumn->flag  :%"ID_UINT32_FMT"\n"
                            "aColumn->offset:%"ID_UINT32_FMT"\n"
                            "aColumn->vcInOu:%"ID_UINT32_FMT"\n"
                            "aColumn->size  :%"ID_UINT32_FMT"\n",
                            sType,
                            aColumn->id,
                            aColumn->flag,
                            aColumn->offset,
                            aColumn->vcInOutBaseSize,
                            aColumn->size );
                break;
        }

        sFlag = (SChar)(( aColumn->flag & SMI_COLUMN_TYPE_MASK ) | sStoreMode );
    }
    else
    {
        sStoreMode = 0;
    
        /* BUG-39282 
         * dictionary compression column은 variable 타입이더라도
         * OID값만 저장하기때문에 fixed 타입과 동일 하게 처리한다.
         * 이렇게 처리하지 않을 경우 getCompressionColumn함수에서
         * OID앞에 추가된 smVCDescInMode값 때문에 제대로 읽을 수 없다. */
        sFlag = (SChar)sStoreMode;
    }

    /* Log Flag 기록 (SChar) */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sFlag,
                                                 *aLogOffset,
                                                 ID_SIZEOF(SChar) )
              != IDE_SUCCESS );

    *aLogOffset += ID_SIZEOF(SChar);

    /* Column Offset 기록 (UInt) */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &(aColumn->offset),
                                                 *aLogOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );

    *aLogOffset += ID_SIZEOF(UInt);

    /* Column ID기록 (UInt) */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &(aColumn->id),
                                                 *aLogOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );

    *aLogOffset += ID_SIZEOF(UInt);

    /* Column Length 기록 (UInt) */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aLength,
                                                 *aLogOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );
    *aLogOffset += ID_SIZEOF(UInt);

    if( aLength != 0 )
    {
        // BUG-37460 update log 에서 compress column 이 고려되지 않았습니다.
        if( ( aColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
            != SMI_COLUMN_COMPRESSION_TRUE )
        {
            switch( sType )
            {
                case SMI_COLUMN_TYPE_LOB:
    
                    sLobDesc = (smcLobDesc*)aValue;
    
                    if( aOpt == SMC_UI_LOG_WRITE_TYPE_BEFORIMG )
                    {
                        /* Piece Count 기록 */
                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     &(sLobDesc->mLPCHCount),
                                                                     *aLogOffset,
                                                                     ID_SIZEOF(UInt) )
                                  != IDE_SUCCESS );
    
                        *aLogOffset += ID_SIZEOF(UInt);
    
                        /* LPCH 기록 */
                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     &(sLobDesc->mFirstLPCH),
                                                                     *aLogOffset,
                                                                     ID_SIZEOF(smcLPCH*) )
                                  != IDE_SUCCESS );
    
                        *aLogOffset += ID_SIZEOF(smcLPCH*);
                    }
    
                // LOB 처리를 VARIABLE에서 이어서 한다.
                case SMI_COLUMN_TYPE_VARIABLE:
                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
    
                    sVCDesc = (smVCDesc*)aValue;
    
                    if( sStoreMode == SM_VCDESC_MODE_OUT )
                    {
                        if( ( aIsReplSenderRead == SMC_LOG_REPL_SENDER_SEND_OK ) ||
                            ( aOpt == SMC_UI_LOG_WRITE_TYPE_AFTERIMG) )
                        {
                            IDE_TEST( writeVCValue4OutMode(aTrans,
                                                        aColumn,
                                                        sVCDesc,
                                                        aLogOffset)
                                    != IDE_SUCCESS );
                        }
    
                        /* Variable Piece OID List 기록 */
                        sVCPieceOID = sVCDesc->fstPieceOID;
    
                        if( aOpt == SMC_UI_LOG_WRITE_TYPE_BEFORIMG )
                        {
                            /* Before Image일 경우 fst piece OID만 기록. */
                            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                         &sVCPieceOID,
                                                                         *aLogOffset,
                                                                         ID_SIZEOF(smOID) )
                                      != IDE_SUCCESS );
    
                            *aLogOffset += ID_SIZEOF(smOID);
                        }
                        else
                        {
                            /* After Image일 경우 모든 piece OID 기록. */
                            sOffset = *aLogOffset;
                            sOIDCnt = 0;
                            /* OID Count를 기록할 공간을 남겨두고, 실제 OID를 쓴다. */
                            *aLogOffset += ID_SIZEOF(UInt);
    
                            while( sVCPieceOID != SM_NULL_OID )
                            {
                                sOIDCnt++;
                                IDE_ASSERT( smmManager::getOIDPtr( 
                                                aColumn->colSpace,
                                                sVCPieceOID,
                                                (void**)&sVCPieceHeader )
                                            == IDE_SUCCESS );
    
                                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                             &(sVCPieceOID),
                                                                             *aLogOffset,
                                                                             ID_SIZEOF(smOID) )
                                          != IDE_SUCCESS );
    
                                *aLogOffset += ID_SIZEOF(smOID);
    
                                sVCPieceOID = sVCPieceHeader->nxtPieceOID;
                            }
    
                            /* OID갯수를 기록 */
                            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                         &sOIDCnt,
                                                                         sOffset,
                                                                         ID_SIZEOF(UInt) )
                                      != IDE_SUCCESS );
    
                        }
                    }
                    else
                    {
                        /* In-Mode로 저장된 Variable Column Value저장 */
                        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                     aValue + ID_SIZEOF(smVCDescInMode),
                                                                     *aLogOffset,
                                                                     aLength )
                                  != IDE_SUCCESS );
                        *aLogOffset += aLength;
                    }
    
                    break;
    
                case SMI_COLUMN_TYPE_FIXED:
    
                    /* Fixed Column Value저장 */
                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 aValue /* Value */,
                                                                 *aLogOffset,
                                                                 aLength )
                              != IDE_SUCCESS );
    
                    *aLogOffset += aLength;
    
                    break;
    
                default:
                    /* Column은 Lob, Variable, Fixed 이어야 한다. */
                    IDE_ERROR_MSG( 0,
                                "sType          :%"ID_UINT32_FMT"\n"
                                "aColumn->id    :%"ID_UINT32_FMT"\n"
                                "aColumn->flag  :%"ID_UINT32_FMT"\n"
                                "aColumn->offset:%"ID_UINT32_FMT"\n"
                                "aColumn->vcInOu:%"ID_UINT32_FMT"\n"
                                "aColumn->size  :%"ID_UINT32_FMT"\n",
                                sType,
                                aColumn->id,
                                aColumn->flag,
                                aColumn->offset,
                                aColumn->vcInOutBaseSize,
                                aColumn->size );
                    break;
            }
        }
        else
        {
            IDE_DASSERT( ID_SIZEOF(smOID) == aLength );

            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         aValue /* Value */,
                                                         *aLogOffset,
                                                         ID_SIZEOF(smOID) )
                      != IDE_SUCCESS );

            *aLogOffset += aLength;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Transaction Log Buffer의 aOffset위치에 aFixRow의 Primary
 *               Primary Key Value를 복사한다.
 *
 * aTrans  - [IN] Transaction Pointer
 * aHeader - [IN] Table Header
 * aFixRow - [IN] Row Pointer
 * aPKSize - [IN] Primary Key Size
 * aOffset - [IN] Transaction Log Buffer Offset
 ***********************************************************************/
IDE_RC smcRecordUpdate::writePrimaryKeyLog( void*                    aTrans,
                                            const smcTableHeader*    aHeader,
                                            const SChar*             aFixRow,
                                            const UInt               aPKSize,
                                            UInt                     aOffset )
{
    const smiColumn * sCurColumn        = NULL;
    const smiColumn * sColumnList       = NULL;
    smVCDesc        * sCurVCDesc        = NULL;
    void            * sIndexHeader      = NULL;
    UInt              i                 = 0;
    UInt              sKeyCount         = 0;
    UInt              sPhysicalColumnID = 0;
    smOID             sVCPieceOID       = SM_NULL_OID;
    smVCPieceHeader * sVCPieceHeader    = NULL;
    UInt              sLogOffset        = 0;
    SInt              sStoreMode        = 0;
    UInt              sLength           = 0;
    SChar           * sValue            = NULL;
    SChar           * sBuff             = NULL;

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aHeader->mColumns.fstPieceOID 
                                           + ID_SIZEOF(smVCPieceHeader),
                                       (void**)&sColumnList )
                == IDE_SUCCESS );
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aHeader->mIndexes[0].fstPieceOID  
                                           + ID_SIZEOF(smVCPieceHeader),
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );


    IDE_ERROR( smLayerCallback::isPrimaryIndex( sIndexHeader )
               == ID_TRUE );

    sKeyCount = smLayerCallback::getColumnCountOfIndexHeader( sIndexHeader );

    /* PK Size */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &aPKSize,
                                                 aOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );

    aOffset += ID_SIZEOF(UInt);

    /* PK Count */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sKeyCount,
                                                 aOffset,
                                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );

    aOffset += ID_SIZEOF(UInt);

    for(i = 0; i < sKeyCount; i++)
    {
        /* Primary Key: Length | Column ID | DATA */
        sPhysicalColumnID = ( *(smLayerCallback::getColumnIDPtrOfIndexHeader( sIndexHeader, i ))
                              & SMI_COLUMN_ID_MASK );

        sCurColumn = smcTable::getColumn(aHeader, sPhysicalColumnID);

        if ((sCurColumn->flag & SMI_COLUMN_TYPE_MASK)
           == SMI_COLUMN_TYPE_VARIABLE)
        {
            sValue = smcRecord::getVarRow( aFixRow,
                                           sCurColumn,
                                           0,
                                           &sLength,
                                           sBuff,
                                           ID_TRUE );

            /* Length */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         &sLength,
                                                         aOffset,
                                                         ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );

            aOffset += ID_SIZEOF(UInt);

            /* index key column id */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         smLayerCallback::getColumnIDPtrOfIndexHeader(
                                                             sIndexHeader, i ),
                                                         aOffset,
                                                         ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );

            aOffset += ID_SIZEOF(UInt);

            /* Value */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         sValue,
                                                         aOffset,
                                                         sLength )
                      != IDE_SUCCESS );

            aOffset += sLength;
        }
        else if ( (sCurColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE_LARGE )
        {
            sCurVCDesc = (smVCDesc*)(aFixRow + sCurColumn->offset);

            /* varColumn length */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans, 
                                                         &(sCurVCDesc->length),
                                                         aOffset,
                                                         ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );

            aOffset += ID_SIZEOF(UInt);

            /* index key column id */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         smLayerCallback::getColumnIDPtrOfIndexHeader(
                                                             sIndexHeader, i ),
                                                         aOffset,
                                                         ID_SIZEOF(UInt) )
                      != IDE_SUCCESS);

            aOffset += ID_SIZEOF(UInt);

            /* index key column value */
            sStoreMode = sCurVCDesc->flag & SM_VCDESC_MODE_MASK;

            if ( sStoreMode == SM_VCDESC_MODE_IN )
            {
                /* index key column value */
                IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                             (SChar*)(((smVCDescInMode*)sCurVCDesc) + 1),
                                                             aOffset,
                                                             sCurVCDesc->length )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_ERROR( sStoreMode == SM_VCDESC_MODE_OUT );

                sVCPieceOID = sCurVCDesc->fstPieceOID;
                sLogOffset = aOffset;

                /* Out Mode로 저장된 Variable Column Value 기록. */
                while ( sVCPieceOID != SM_NULL_OID )
                {
                    IDE_ASSERT( smmManager::getOIDPtr( sCurColumn->colSpace,
                                                       sVCPieceOID,
                                                       (void**)&sVCPieceHeader )
                                == IDE_SUCCESS );

                    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                                 (SChar*)(sVCPieceHeader + 1),
                                                                 sLogOffset,
                                                                 sVCPieceHeader->length )
                              != IDE_SUCCESS );

                    sLogOffset += sVCPieceHeader->length;

                    sVCPieceOID = sVCPieceHeader->nxtPieceOID;
                }

                IDE_ERROR(sLogOffset == (sCurVCDesc->length + aOffset));
            }

            aOffset += sCurVCDesc->length;

        }
        else
        {
            /* Length */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         &(sCurColumn->size),
                                                         aOffset,
                                                         ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );

            aOffset += ID_SIZEOF(UInt);

            /* index key column id */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         smLayerCallback::getColumnIDPtrOfIndexHeader( sIndexHeader,i ),
                                                         aOffset,
                                                         ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );

            aOffset += ID_SIZEOF(UInt);

            /* Value */
            IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                         aFixRow + sCurColumn->offset,
                                                         aOffset,
                                                         sCurColumn->size )
                      != IDE_SUCCESS );

            aOffset += sCurColumn->size;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aUpdateLogType
 *
 * aTrans   - [IN] Transaction Pointer
 * aHeader  - [IN] Table Header
 * aUpdateLogType    - [IN] Update Log Type: Insert, Delete, Update관련로그
 *                          이어야 한다.
 * aOpt     - [IN] 1. Log Flag에 SMR_LOG_ALLOC_FIXEDSLOT_OK을 원하면.
 *                    SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK,
 *                 2. Log Flag에 SMR_LOG_ALLOC_FIXEDSLOT_NO을 원하면.
 *                    SMC_MKLOGFLAG_SET_ALLOC_FIXED_NO
 * aFlag    - [OUT] Flag
 ***********************************************************************/
IDE_RC smcRecordUpdate::makeLogFlag(void                 *aTrans,
                                    const smcTableHeader *aHeader,
                                    smrUpdateType         aUpdateLogType,
                                    smcMakeLogFlagOpt     aOpt,
                                    UInt                 *aFlag)
{
    UInt            sLogFlag = 0;
    idBool          sReplicate;

    IDE_ERROR(aTrans != NULL);
    IDE_ERROR(aHeader != NULL);
    IDE_ERROR(aFlag != NULL);

    /* Log가 적절한 Update Log Type인지 조사한다. */
    IDE_ERROR( (aUpdateLogType == SMR_SMC_SET_CREATE_SCN)          ||
               (aUpdateLogType == SMR_SMC_PERS_INSERT_ROW)         ||
               (aUpdateLogType == SMR_SMC_PERS_UPDATE_VERSION_ROW) ||
               (aUpdateLogType == SMR_SMC_PERS_UPDATE_INPLACE_ROW) ||
               (aUpdateLogType == SMR_SMC_PERS_DELETE_VERSION_ROW) ||
               (aUpdateLogType == SMR_SMC_PERS_WRITE_LOB_PIECE) );

    if( (aOpt & SMC_MKLOGFLAG_REPL_SKIP_LOG) != SMC_MKLOGFLAG_REPL_SKIP_LOG )
    {
        sLogFlag = smLayerCallback::getLogTypeFlagOfTrans( aTrans );
        // Replication로그를 남겨야 하는 경우
        if( smcTable::needReplicate(aHeader, aTrans ) == ID_TRUE )
        {
            //replication이 사용할 필요가 있다면
            /* BUG-17033: 최상위 Statement가 아닌 Statment에 대해서도
             * Partial Rollback을 지원해야 합니다. */
            if ( smLayerCallback::checkAndSetImplSVPStmtDepth4Repl( aTrans )
                 == ID_FALSE )
            {
                sLogFlag |= SMR_LOG_SAVEPOINT_OK;
            }

            if ( smLayerCallback::isPsmSvpReserved( aTrans ) == ID_TRUE )
            {
                IDE_TEST( smLayerCallback::writePsmSvp( aTrans )
                          != IDE_SUCCESS );
            }
            else
            {
                // do nothing
            }

            sReplicate = smcTable::isTransWaitReplicationTable( (const smcTableHeader*)aHeader );

            if ( sReplicate == ID_TRUE )
            {
                smLayerCallback::setIsTransWaitRepl( aTrans, ID_TRUE );
            }

            // BUG-46854: RP 정보 ( 프라이머리 키 ) 로그 작성 여부 플래그 추가 
            sLogFlag |= SMR_LOG_RP_INFO_LOG_OK;
        }
        else
        {
            sLogFlag = SMR_LOG_TYPE_REPLICATED;
        }
    }
    else
    {
        // aOpt에 SMC_MKLOGFLAG_REPL_SKIP_LOG가 설정되어 있다.
        sLogFlag = SMR_LOG_TYPE_REPLICATED;
    }

    /* BUG-14513:
       MVCC의 DML Log(Insert, Update, Delete)는 자신의
       Header의 Flag를 보고 Alloc Slot에 대한 Redo Undo결정.
    */
    if( (aOpt & SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK) == (SInt)
        SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK)
    {
        sLogFlag |= SMR_LOG_ALLOC_FIXEDSLOT_OK;
    }

    /* TASK-5030
     * FXLog일 경우 flag 셋팅 */
    if( smcTable::isSupplementalTable(aHeader) == ID_TRUE )
    {
        sLogFlag |= SMR_LOG_FULL_XLOG_OK;
    }

    *aFlag = sLogFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
UInt smcRecordUpdate::getPrimaryKeySize( const smcTableHeader   * aHeader,
                                         const SChar            * aFixRow )
{
    const smiColumn * sCurColumn;
    const smiColumn * sColumnList;
    void            * sIndexHeader;
    UInt              i;
    UInt              sKeyCount;
    UInt              sPhysicalColumnID;
    UInt              sPrimaryKeySize   = 0;
    UInt              sVCLen            = 0;

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aHeader->mColumns.fstPieceOID 
                                            + ID_SIZEOF(smVCPieceHeader),
                                       (void**)&sColumnList )
                == IDE_SUCCESS );

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aHeader->mIndexes[0].fstPieceOID 
                                       + ID_SIZEOF(smVCPieceHeader),
                                       (void**)&sIndexHeader )
               == IDE_SUCCESS );

    sKeyCount     = smLayerCallback::getColumnCountOfIndexHeader( sIndexHeader );

    for(i = 0; i < sKeyCount; i++)
    {
        sPhysicalColumnID = ( *(smLayerCallback::getColumnIDPtrOfIndexHeader( sIndexHeader,i ))
                              & SMI_COLUMN_ID_MASK );

        sCurColumn  =  smcTable::getColumn(aHeader,sPhysicalColumnID);

        if ( ( (sCurColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE ) ||
             ( (sCurColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            sVCLen = smcRecord::getVarColumnLen( sCurColumn, aFixRow );
            sPrimaryKeySize += ID_SIZEOF(UInt) * 2 + sVCLen;
        }
        else
        {
            sPrimaryKeySize += ID_SIZEOF(UInt) * 2 + sCurColumn->size;
        }
    }

    sPrimaryKeySize += ID_SIZEOF(UInt) * 2;
    return sPrimaryKeySize;
}

IDE_RC smcRecordUpdate::deleteRowFromTBIdx( scSpaceID aSpaceID,
                                            smOID     aTableOID,
                                            smOID     aRowID,
                                            ULong     aModifyIdxBit )
{
    smcTableHeader * sTableHeader;
    SChar          * sIndexHeader;
    SInt             i;
    SInt             sIndexCnt;
    void           * sRowPtr;
    idBool           sIsExistFreeKey;
    idBool           sBeforeExistKeyResult = ID_FALSE;
    ULong            sBitMask;

    IDE_ASSERT( smcTable::getTableHeaderFromOID( aTableOID,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );
    sIndexCnt = smcTable::getIndexCount(sTableHeader);

    sBitMask = ((ULong)1 << (63 - (sIndexCnt - 1)));
    for( i = sIndexCnt - 1; i >= 0; i-- )
    {
        /* BUG-47615 inplace update에서는 index를 선별적으로 갱신 한다.
         * 그러므로 update 시에 접근했던 index 만 처리한다.*/
        IDE_DASSERT( sBitMask == ((ULong)1 << ( 63 - i ) ) );
        if (( aModifyIdxBit & sBitMask ) != 0 )
        {
            sIndexHeader = (SChar *) smcTable::getTableIndex(sTableHeader, i);

            /* BUG-19098: Disable된 Index에 대해서 insert, delete하다가
             *            서버 사망 */
            if( smnManager::isIndexEnabled( sIndexHeader ) == ID_TRUE )
            {
                IDE_ASSERT( smmManager::getOIDPtr(aSpaceID, aRowID, &sRowPtr)
                            == IDE_SUCCESS );

                IDE_TEST( smLayerCallback::indexDeleteFunc( sIndexHeader,
                                                            (SChar*)sRowPtr,
                                                            ID_TRUE,  /*aIgnoreNotFoundKey*/
                                                            &sIsExistFreeKey )
                          != IDE_SUCCESS );

                if ( sIsExistFreeKey == ID_FALSE )
                {
                    IDE_ASSERT( sBeforeExistKeyResult == ID_FALSE );
                }
                else
                {
                    /* nothing */
                }

                sBeforeExistKeyResult = sIsExistFreeKey;
            }
        }
        sBitMask = sBitMask << 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcRecordUpdate::insertRow2TBIdx(void*     aTrans,
                                        scSpaceID aSpaceID,
                                        smOID     aTableOID,
                                        smOID     aRowID,
                                        ULong     aModifyIdxBit )
{
    smcTableHeader * sTableHeader;
    SChar          * sIndexHeader;
    UInt             i;
    UInt             sIndexCnt;
    void           * sRowPtr;
    void           * sNullRowPtr;
    ULong            sBitMask = ((ULong)1 << 63);

    IDE_ASSERT( smcTable::getTableHeaderFromOID( aTableOID,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );

    sIndexCnt = smcTable::getIndexCount( sTableHeader );

    for( i = 0;  i < sIndexCnt ; i++)
    {
        /* BUG-47615 inplace update에서는 index를 선별적으로 갱신 한다.
         * 그러므로 update 시에 접근했던 index 만 처리한다.*/
        if (( aModifyIdxBit & sBitMask ) != 0 )
        {
            sIndexHeader = (SChar *) smcTable::getTableIndex(sTableHeader, i);

            /* BUG-19098: Disable된 Index에 대해서 insert, delete하다가
             *            서버 사망 */
            if( smnManager::isIndexEnabled( sIndexHeader ) == ID_TRUE )
            {
                IDE_ASSERT( smmManager::getOIDPtr(aSpaceID, aRowID, &sRowPtr)
                            == IDE_SUCCESS );
                IDE_ASSERT( smmManager::getOIDPtr(aSpaceID, sTableHeader->mNullOID, &sNullRowPtr)
                            == IDE_SUCCESS );

                //No Unique Check
                if ( smnManager::indexInsertWithoutUniqueCheck( aTrans,
                                                                sTableHeader,
                                                                sIndexHeader,
                                                                (SChar*)sRowPtr,
                                                                (SChar*)sNullRowPtr )
                     != IDE_SUCCESS )
                {
                    IDE_TEST(ideGetErrorCode() != smERR_ABORT_smnUniqueViolation);
                }
            }
        }
        sBitMask = sBitMask >> 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_INIT_FIXED_ROW     */
IDE_RC smcRecordUpdate::redo_SMC_PERS_INIT_FIXED_ROW(smTID      /*aTID*/,
                                                     scSpaceID    aSpaceID,
                                                     scPageID     aPID,
                                                     scOffset     aOffset,
                                                     vULong       aData,
                                                     SChar*     /*aAfterImage*/,
                                                     SInt       /*aSize*/,
                                                     UInt       /*aFlag*/)
{
    smpSlotHeader     *sSlotHeader;
    smSCN              sSCN;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );

    SM_INIT_SCN( &sSCN );

    if((idBool)aData == ID_TRUE)
    {
        SM_SET_SCN_DELETE_BIT( &sSCN );
    }

    sSlotHeader->mCreateSCN = sSCN;
    SM_SET_SCN_FREE_ROW( &(sSlotHeader->mLimitSCN) );
    SMP_SLOT_SET_OFFSET( sSlotHeader, aOffset );
    SMP_SLOT_SET_USED( sSlotHeader );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*>>>>>>>>>>>>>> Undo: SMR_SMC_PERS_INIT_FIXED_ROW                */
IDE_RC smcRecordUpdate::undo_SMC_PERS_INIT_FIXED_ROW(
                                               smTID       aTID,
                                               scSpaceID   aSpaceID,
                                               scPageID    aPID,
                                               scOffset    aOffset,
                                               vULong    /*aData*/,
                                               SChar*      aBeforeImage,
                                               SInt        aSize,
                                               UInt      /*aFlag*/)
{
    smpSlotHeader  * sSlotHeader;
    void           * sTransPtr;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(sSlotHeader, aBeforeImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sTransPtr     = smLayerCallback::getTransByTID( aTID );

    smrRecoveryMgr::prepareRTOIForUndoFailure( sTransPtr,
                                               SMR_RTOI_TYPE_MEMPAGE,
                                               SM_NULL_OID,
                                               0, /* IndexID */
                                               aSpaceID,
                                               aPID );

    return IDE_FAILURE;
}


/***********************************************************************
 * PROJ-2429 Dictionary based data compress for on-disk DB
 * Description : Dictionary Table에 record를 insert하면 이후 Tx가
 *               rollback되더라도 undo 되면 안된다.
 *               NTA로그로 undo되는것을 막지만 Tx가 commit이 되지 않았기
 *               때문에 slotHeader의 mCreateSCN이 commit SCN이 아니다.
 *               따라서 해당 record를 조회 할 수 없다.
 *               또한 mCreateSCN이 infifnite SCN임으로 refine과정에서
 *               정리 되어버린다.
 *               이를 방지 하기 위해 해당 record의 mCreateSCN을 다른 Tx가
 *               볼 수 있고 refine시 정리 되지않게 값을 세팅 한다.
 *
 *
 * Type       :  SMR_SMC_SET_CREATE_SCN
 *
 * LOG HEADER :  smrUpdateLog
 * BODY       :  Before Image : slotHeader에 저장된 mCreateSCN.
 *               After Image  : 다른 Tx가 값을 볼 수 있고,
 *                              refine시 정리 되지 않는 SCN값.
 *
 * aTrans          - [IN] Transaction Pointer
 * aHeader         - [IN] Table Header Pointer
 * adRow           - [IN] Fixed Row Pointer
 *
 ***********************************************************************/
IDE_RC smcRecordUpdate::writeSetSCNLog( void*             aTrans,
                                        smcTableHeader*   aHeader,
                                        SChar*            aRow )
{
    UInt         sLogSize;
    UInt         sOffset;
    smrUpdateLog sUpdateLog;
    scPageID     sPageID;
    UInt         sLogFlag;
    smrLogType   sLogType;
    smSCN        sSCN;

    IDE_ASSERT( aHeader->mSpaceID != 0 );

    sPageID = SMP_SLOT_GET_PID(aRow);

    /* Insert Row에 대하여 추가의 로그 헤더 정보 구성 */
    sLogSize = SMR_LOGREC_SIZE(smrUpdateLog) 
                + ID_SIZEOF(smrLogTail) 
                + ID_SIZEOF(smSCN)
                + ID_SIZEOF(smSCN);

    /* Insert한 Row에 대하여 로그 헤더 정보 구성 */
    smrLogHeadI::setType(&sUpdateLog.mHead, SMR_LT_UPDATE);
    smrLogHeadI::setTransID( &sUpdateLog.mHead, smLayerCallback::getTransID( aTrans ) );

    IDE_TEST( makeLogFlag(aTrans,
                          aHeader,
                          SMR_SMC_SET_CREATE_SCN,
                          SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK,
                          &sLogFlag)
              != IDE_SUCCESS );

    /* makeLogFlag가 설정되이후에 호출되어야 한다. 왜냐면 안에서
     * smLayerCallback::getLstReplStmtDepth가 return하는 값을 변경한다.*/
    smrLogHeadI::setReplStmtDepth( &sUpdateLog.mHead,
                                   smLayerCallback::getLstReplStmtDepth( aTrans ) );

    smrLogHeadI::setFlag(&sUpdateLog.mHead, sLogFlag);
    SC_MAKE_GRID( sUpdateLog.mGRID,
                  aHeader->mSpaceID,
                  sPageID,
                  SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRow ) );

    /* set table oid */
    sUpdateLog.mData        = aHeader->mSelfOID;
    sUpdateLog.mType        = SMR_SMC_SET_CREATE_SCN;
    sUpdateLog.mBImgSize    = ID_SIZEOF(smSCN);

    smrLogHeadI::setSize(&sUpdateLog.mHead, sLogSize);
    sUpdateLog.mAImgSize   = ID_SIZEOF(smSCN);

    smrLogHeadI::setPrevLSN( &sUpdateLog.mHead, smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    sOffset = 0;

    /* Log Header를 Transaction Log Buffer에 기록 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sUpdateLog, /* Update Log Header */
                                                 sOffset,
                                                 SMR_LOGREC_SIZE(smrUpdateLog) )
              != IDE_SUCCESS );
    sOffset += SMR_LOGREC_SIZE(smrUpdateLog);

    /* smpSlotHeader의 현재 mCreateSCN로깅 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 (void*)&((smpSlotHeader*)aRow)->mCreateSCN,
                                                 sOffset,
                                                 ID_SIZEOF(smSCN) )
              != IDE_SUCCESS );
    sOffset += ID_SIZEOF(smSCN);

    SM_INIT_SCN( &sSCN );
    /* smpSlotHeader가 갱신 되어야할 SCN로깅 */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 (void*)&sSCN,
                                                 sOffset,
                                                 ID_SIZEOF(smSCN) )
              != IDE_SUCCESS );
    sOffset += ID_SIZEOF(smSCN);

    /* Log Tail을 Transaciton Log Buffer에 기록 */
    sLogType = smrLogHeadI::getType(&sUpdateLog.mHead);
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 &sLogType,
                                                 sOffset,
                                                 ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );
    smrLogHeadI::setType(&sUpdateLog.mHead, sLogType);

    IDE_TEST( smLayerCallback::writeTransLog( aTrans, sUpdateLog.mData ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2429 Dictionary based data compress for on-disk DB
 * Update type:  SMR_SMC_SET_CREATE_SCN */
IDE_RC smcRecordUpdate::redo_SMC_SET_CREATE_SCN(smTID      /*aTID*/,
                                                scSpaceID    aSpaceID,
                                                scPageID     aPID,
                                                scOffset     aOffset,
                                                vULong     /*aData*/,
                                                SChar*     /*aAfterImage*/,
                                                SInt       /*aSize*/,
                                                UInt       /*aFlag*/)
{
    smpSlotHeader     *sSlotHeader = NULL;
    smSCN              sSCN;

    IDE_ASSERT( aSpaceID != SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );

    SM_INIT_SCN( &sSCN );

    sSlotHeader->mCreateSCN = sSCN;

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
