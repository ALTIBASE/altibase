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
 * $Id: smcTableUpdate.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smr.h>
#include <smm.h>
#include <smp.h>
#include <smcDef.h>
#include <smcReq.h>
#include <smcTableUpdate.h>


/* Commit Log : SMR_LT_TRANS_COMMIT */
IDE_RC smcTableUpdate::redo_SMR_LT_TRANS_COMMIT(SChar     *aAfterImage,
                                                SInt       aSize,
                                                idBool     aForMediaRecovery )
{

    smcTableHeader *sTableHeader;
    scPageID        sPageID;
    scOffset        sOffset;
    UInt            sTableCnt;
    SChar          *sCurLogPtr;
    UInt            i;
    idBool          sIsExistTBS;
    idBool          sIsApplyLog;

    IDE_ERROR( aAfterImage != NULL );
    IDE_ERROR( aSize != 0 );

    sCurLogPtr = aAfterImage;
    sTableCnt  = (aSize / (ID_SIZEOF(scPageID) + ID_SIZEOF(scOffset) +
                 ID_SIZEOF(ULong)));

    for (i = 0; i < sTableCnt; i++)
    {
        idlOS::memcpy(&sPageID, sCurLogPtr, ID_SIZEOF(scPageID));
        sCurLogPtr += ID_SIZEOF(scPageID);

        if ( aForMediaRecovery == ID_TRUE )
        {
            IDE_TEST ( smmTBSMediaRecovery::findMatchFailureDBF(
                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                          sPageID,
                          &sIsExistTBS,
                          &sIsApplyLog ) // sIsFailureDBF 복구대상이라면
                       != IDE_SUCCESS );
        }
        else
        {
            // 미디어복구가 아닌경우에는 무조건 적용한다.
            sIsExistTBS = ID_TRUE;
            sIsApplyLog = ID_TRUE;
        }

        if ( sIsExistTBS == ID_TRUE && sIsApplyLog == ID_TRUE )
        {
            idlOS::memcpy(&sOffset, sCurLogPtr, ID_SIZEOF(scOffset));
            sCurLogPtr += ID_SIZEOF(scOffset);

            IDE_ASSERT( smmManager::getOIDPtr( 
                            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                            SM_MAKE_OID( sPageID, sOffset ),
                            (void**)&sTableHeader )
                        == IDE_SUCCESS );

            /* BUG-15710: Redo시에 특정영역의 데이타가 올바른지 ASSERT를
             * 걸고 있습니다.
             * Redo시에 해당데이타에 redo하기전까지는 Valid하다는 것을
             * 보장하지 못합니다.
             * IDE_ASSERT( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_TRUE );
             */
            idlOS::memcpy( &(sTableHeader->mFixed.mDRDB.mRecCnt),
                           sCurLogPtr,
                           ID_SIZEOF(ULong) );

            sCurLogPtr += ID_SIZEOF(ULong);

            IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                              sPageID ) != IDE_SUCCESS );
        }
        else
        {
            // 미디어 복구시에만 적용하지 않고 넘어가는 경우가 존재한다.
            IDE_ERROR_MSG( aForMediaRecovery == ID_TRUE, 
                           "aForMediaRecovery : %"ID_UINT32_FMT, aForMediaRecovery );

            sCurLogPtr += ID_SIZEOF(scOffset);
            sCurLogPtr += ID_SIZEOF(ULong);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Update Type: SMR_SMC_TABLEHEADER_INIT             */
IDE_RC smcTableUpdate::redo_SMC_TABLEHEADER_INIT(
                                            smTID        /*aTID*/,
                                            scSpaceID     /* aSpaceID */,
                                            scPageID      aPID,
                                            scOffset      aOffset,
                                            vULong        /*aData*/,
                                            SChar       * aAfterImage,
                                            SInt          aSize,
                                            UInt          /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                SM_MAKE_OID( aPID, aOffset ),
                                (void**)&sTableHeader )
                == IDE_SUCCESS );

    IDE_DASSERT( aSize == ID_SIZEOF(smcTableHeader) );

    idlOS::memcpy( sTableHeader, aAfterImage, aSize );
    /* BUG-48588 */ 
    SM_INIT_SCN( &(sTableHeader->mTableCreateSCN) );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, aPID)
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INDEX                     */
IDE_RC smcTableUpdate::redo_SMC_TABLEHEADER_UPDATE_INDEX(
                                                    smTID       /*aTID*/,
                                                    scSpaceID    /* aSpaceID */,
                                                    scPageID    /*aPID*/,
                                                    scOffset    /*aOffset*/,
                                                    vULong        aData,
                                                    SChar        *aImage,
                                                    SInt          aSize,
                                                    UInt        /*aFlag*/)
{

    smcTableHeader *sTableHeader;
    smOID           sOIDTable;
    UInt            sOIDIdx;

    IDE_ERROR_MSG( aSize == (ID_SIZEOF(smVCDesc) + ID_SIZEOF(UInt)),
                   "aSize : %"ID_UINT32_FMT, aSize );

    sOIDTable = (smOID)aData;
    IDE_ASSERT( smcTable::getTableHeaderFromOID( sOIDTable,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );
    idlOS::memcpy(&sOIDIdx, aImage, ID_SIZEOF(UInt));

    IDE_ERROR_MSG( sOIDIdx < SMC_MAX_INDEX_OID_CNT,
                   "sOIDIdx : %"ID_UINT32_FMT, sOIDIdx );

    aImage += ID_SIZEOF(UInt);

    idlOS::memcpy(&(sTableHeader->mIndexes[sOIDIdx]), aImage,  sizeof(smVCDesc));

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 SM_MAKE_PID(sOIDTable)) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smcTableUpdate::undo_SMC_TABLEHEADER_UPDATE_INDEX(
                                                    smTID       /*aTID*/,
                                                    scSpaceID    /* aSpaceID*/,
                                                    scPageID    /*aPID*/,
                                                    scOffset    /*aOffset*/,
                                                    vULong        aData,
                                                    SChar        *aImage,
                                                    SInt          aSize,
                                                    UInt        /*aFlag*/)
{

    smcTableHeader *sTableHeader;
    smOID           sOIDTable;
    UInt            sState = 0;
    UInt            sOIDIdx;

    IDE_ERROR_MSG( aSize == (ID_SIZEOF(smVCDesc) + ID_SIZEOF(UInt)),
                   "aSize : %"ID_UINT32_FMT, aSize );

    sOIDTable = (smOID)aData;
    IDE_ASSERT( smcTable::getTableHeaderFromOID( sOIDTable,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&sOIDIdx, aImage, ID_SIZEOF(UInt));

    IDE_ERROR_MSG( sOIDIdx < SMC_MAX_INDEX_OID_CNT,
                   "sOIDIdx : %"ID_UINT32_FMT, sOIDIdx );



    if(smrRecoveryMgr::isRestart() == ID_FALSE)
    {
        IDE_TEST( smcTable::latchExclusive( sTableHeader ) != IDE_SUCCESS );
        sState = 1;
    }
    else
    {
        // PR-14912
        // 이전 index slot header에 build했던 index runtime header르
        // 제거한다.
        // but, do not redo CLR
        if( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_TRUE &&
            ( smrRecoveryMgr::isRefineDRDBIdx()    == ID_TRUE ) )
        {
            IDE_TEST( smLayerCallback::dropIndexes( sTableHeader )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothing to do...
        }
    }

    aImage += sizeof(UInt);

    idlOS::memcpy(&(sTableHeader->mIndexes[sOIDIdx]),
                  aImage,
                  sizeof(smVCDesc));

    if((smrRecoveryMgr::isRestart() == ID_FALSE)
       && (sTableHeader->mDropIndexLst != NULL))
    {
        IDE_TEST(smcTable::mDropIdxPagePool.memfree(sTableHeader->mDropIndexLst)
                 != IDE_SUCCESS);
        sTableHeader->mDropIndex    = 0;
        sTableHeader->mDropIndexLst = NULL;
    }

    if(smrRecoveryMgr::isRestart() == ID_FALSE)
    {
        sState = 0;
        IDE_TEST( smcTable::unlatch( sTableHeader ) != IDE_SUCCESS );
    }
    else // restart시
    {
        /* ------------------------------------------------
         * PR-14912
         * 해당경우는 index 생성 및 제거과정에서 system crash가
         * 발생하여 restart recovery하는 과정에 이전(old) index header
         * 들을 저장하고 있는 index header slot에 대해서
         * 다시 index runtime header를 build 해주어야 한다.
         *
         * [ 주의사항 ]
         * new index header slot에 대해서 redoAll 완료직후에
         * 생성은 해주지만, 깔끔하게 다시 생성해준다.
         * ----------------------------------------------*/
        // but, do not redo CLR
        if( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_TRUE &&
            ( smrRecoveryMgr::isRefineDRDBIdx()    == ID_TRUE ) )
        {
            // Lock & RuntimeItem 는 이미 존재하므로 초기화할 필요없다.
            IDE_TEST( smcTable::rebuildRuntimeIndexHeaders(
                                  NULL,  /* idvSQL* */
                                  sTableHeader,
                                  0 /* aMaxSmoNo */ ) != IDE_SUCCESS );
        }
        else
        {
           // nothing to do...
        }
    }


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 SM_MAKE_PID(sOIDTable)) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();

        IDE_ASSERT( smcTable::unlatch( sTableHeader ) == IDE_SUCCESS );

        IDE_POP();
    }

    return IDE_FAILURE;

}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_COLUMNS                    */
IDE_RC smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMNS(
                                                           smTID       /*aTID*/,
                                                           scSpaceID    /*aSpaceID*/,
                                                           scPageID      aPID,
                                                           scOffset      aOffset,
                                                           vULong      /*aData*/,
                                                           SChar        *aAfterImage,
                                                           SInt          aSize,
                                                           UInt        /*aFlag*/)
{
    smcTableHeader  * sTableHeader;
    smnIndexHeader  * sIndexHeader;
    smnIndexModule  * sIndexModule;
    UInt              i;

    IDE_ERROR_MSG( aSize == (ID_SIZEOF(smVCDesc)
                                + ID_SIZEOF(UInt)
                                + ID_SIZEOF(UInt)),
                   "aSize : %"ID_UINT32_FMT, aSize );

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sTableHeader->mColumns), aAfterImage, ID_SIZEOF(smVCDesc));
    aAfterImage += ID_SIZEOF(smVCDesc);
    idlOS::memcpy(&(sTableHeader->mLobColumnCount), aAfterImage, ID_SIZEOF(UInt));
    aAfterImage += ID_SIZEOF(UInt);
    idlOS::memcpy(&(sTableHeader->mColumnCount), aAfterImage, ID_SIZEOF(UInt));


    /* Runtime Header가 존재하는 경우, 즉 Undo 시점에서만 수행함 */
    if ( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_TRUE )
    {
        /* BUG-31949 [qp-ddl-dcl-execute] failure of index visibility check
         * after abort real-time DDL in partitioned table. 
         * 이 경우는 RestartRecovery 중에도 수행해야 할 필요가 있다. 왜냐하면 DRDB
         * Index Header가 Redo 후 만들어지기 때문에, 다음과 같은 문제가 발생할 수
         * 있다.
         *
         * 1) Alter table modify column ( A -> A~)
         * 2) Commit되지 못하고 비정상 종료
         * 3) Restart Recovery
         * 4) Redo 시작
         * 5) Redo SMR_SMC_TABLEHEADER_UPDATE_COLUMNS
         *      Table Column 정보를 A -> A~ 로 변경
         * 6) DRDB Index Runtime Header 생성
         *      Table Column의 A~를 바탕으로 Index Runtime Header 생성됨
         * 7) Undo SMR_SMC_TABLEHEADER_UPDATE_COLUMNS
         *      Table Column 정보를 A~-> A 로 변경
         *      하지만 Index Runtime Header는 변경하지 않음
         * 8) Table Column은 Column이 A인데,  Index Column은 A~가 됨 */
        if ( smrRecoveryMgr::isRefineDRDBIdx() == ID_TRUE )
        {
            for ( i = 0 ; i < smcTable::getIndexCount( sTableHeader ) ; i ++ )
            {
                sIndexHeader = 
                    (smnIndexHeader*) smcTable::getTableIndex( sTableHeader, i );
 
                sIndexModule = (smnIndexModule*)sIndexHeader->mModule;
                IDE_TEST( sIndexModule->mRebuildIndexColumn( 
                            sIndexHeader,
                            sTableHeader,
                            sIndexHeader->mHeader )
                    != IDE_SUCCESS );
            }
        }

        /* PROJ-2399 Row Template
         * 업데이트된 column정보를 바탕으로 RowTemplate를 재구성 한다. 
         * rowTemplate는 restart이후에 만들어진다.(refine완료 이후) 
         * 따라서 restart가 아닌 서비스상태일때 undo만 처리하면 된다. */
        if ( (smrRecoveryMgr::isRestart() != ID_TRUE) && 
             (sTableHeader->mColumns.fstPieceOID != SM_NULL_OID) )
        {
            IDE_TEST( smcTable::destroyRowTemplate( sTableHeader ) != IDE_SUCCESS );
            IDE_TEST( smcTable::initRowTemplate( NULL,  /* aStatistics */
                                                 sTableHeader,
                                                 NULL ) /* aActionArg */
                      != IDE_SUCCESS );
        }
    }


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INFO                       */
IDE_RC smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_INFO(
                                                        smTID       /*aTID*/,
                                                        scSpaceID    /*aSpaceID*/,
                                                        scPageID      aPID,
                                                        scOffset      aOffset,
                                                        vULong      /*aData*/,
                                                        SChar        *aAfterImage,
                                                        SInt          aSize,
                                                        UInt        /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ERROR_MSG( aSize == ID_SIZEOF(smVCDesc),
                   "aSize : %"ID_UINT32_FMT, aSize );

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );
    idlOS::memcpy(&(sTableHeader->mInfo), aAfterImage, aSize);


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Update Type: SMR_SMC_TABLEHEADER_SET_NULLROW                    */
IDE_RC smcTableUpdate::redo_SMC_TABLEHEADER_SET_NULLROW(
                                                   smTID       /*aTID*/,
                                                   scSpaceID   /*aSpaceID*/,
                                                   scPageID      aPID,
                                                   scOffset      aOffset,
                                                   vULong        aData,
                                                   SChar      */*aAfterImage*/,
                                                   SInt        /*aSize*/,
                                                   UInt        /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                   (void**) &sTableHeader )
                == IDE_SUCCESS );

    IDE_ERROR( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_FALSE );

    sTableHeader->mNullOID = (smOID)aData;

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALL         */
IDE_RC smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_ALL(
                                                       smTID       /*aTID*/,
                                                       scSpaceID    /*aSpaceID*/,
                                                       scPageID      aPID,
                                                       scOffset      aOffset,
                                                       vULong      /*aData*/,
                                                       SChar        *aImage,
                                                       SInt        /*aSize*/,
                                                       UInt        /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sTableHeader->mColumns), aImage, ID_SIZEOF(smVCDesc));
    aImage += ID_SIZEOF(smVCDesc);
    idlOS::memcpy(&(sTableHeader->mInfo), aImage, ID_SIZEOF(smVCDesc));
    aImage += ID_SIZEOF(smVCDesc);
    idlOS::memcpy(&(sTableHeader->mColumnSize), aImage, ID_SIZEOF(UInt));
    aImage += ID_SIZEOF(UInt);
    idlOS::memcpy(&(sTableHeader->mFlag), aImage, ID_SIZEOF(UInt));
    aImage += ID_SIZEOF(UInt);
    idlOS::memcpy(&(sTableHeader->mMaxRow), aImage, ID_SIZEOF(ULong));
    aImage += ID_SIZEOF(ULong);
    idlOS::memcpy(&(sTableHeader->mParallelDegree), aImage, ID_SIZEOF(UInt));


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO   */
/* before image: PageCount | HeadPageID | TailPageID */
/* after  image: PageCount | HeadPageID | TailPageID */
IDE_RC smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_ALLOCINFO(
                                                         smTID       /*aTID*/,
                                                         scSpaceID    /*aSpaceID*/,
                                                         scPageID       aPID,
                                                         scOffset       aOffset,
                                                         vULong       /* aData */,
                                                         SChar         *aImage,
                                                         SInt         /* aSize */,
                                                         UInt         /*aFlag*/)
{
    smpAllocPageListEntry* sAllocPageList;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sAllocPageList )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sAllocPageList->mPageCount), aImage, ID_SIZEOF(vULong));
    aImage += ID_SIZEOF(vULong);
    idlOS::memcpy(&(sAllocPageList->mHeadPageID), aImage, ID_SIZEOF(scPageID));
    aImage += ID_SIZEOF(scPageID);
    idlOS::memcpy(&(sAllocPageList->mTailPageID), aImage, ID_SIZEOF(scPageID));


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* SMR_SMC_TABLEHEADER_UPDATE_FLAG                 */
IDE_RC smcTableUpdate::redo_SMC_TABLEHEADER_UPDATE_FLAG(
                                                   smTID       /*aTID*/,
                                                   scSpaceID    /*aSpaceID*/,
                                                   scPageID      aPID,
                                                   scOffset      aOffset,
                                                   vULong      /*aData*/,
                                                   SChar        *aImage,
                                                   SInt          aSize,
                                                   UInt         /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sTableHeader->mFlag), aImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smcTableUpdate::undo_SMC_TABLEHEADER_UPDATE_FLAG(
                                                   smTID       /*aTID*/,
                                                   scSpaceID    /*aSpaceID*/,
                                                   scPageID      aPID,
                                                   scOffset      aOffset,
                                                   vULong      /*aData*/,
                                                   SChar        *aImage,
                                                   SInt          aSize,
                                                   UInt        /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                   (void**) &sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sTableHeader->mFlag), aImage, aSize);


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Update type:  SMR_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT         */
IDE_RC smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT(smTID    /* aTID */,
                                                                     scSpaceID    /*aSpaceID*/,
                                                                     scPageID aPID,
                                                                     scOffset aOffset,
                                                                     vULong   /* aData */,
                                                                     SChar*   aImage,
                                                                     SInt     /* aSize */,
                                                                     UInt       /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );
    idlOS::memcpy(&(sTableHeader->mColumnCount), aImage, ID_SIZEOF(UInt));


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* PROJ-2162 */
/* SMR_SMC_TABLEHEADER_SET_INCONSISTENT                 */
IDE_RC smcTableUpdate::redo_SMC_TABLEHEADER_SET_INCONSISTENT(
                                                   smTID        /*aTID*/,
                                                   scSpaceID    /*aSpaceID*/,
                                                   scPageID     aPID,
                                                   scOffset     aOffset,
                                                   vULong       aData,
                                                   SChar      * aImage,
                                                   SInt         aSize,
                                                   UInt         /*aFlag*/)
{
    smcTableHeader * sTableHeader;
    idBool           sForMediaRecovery;

    sForMediaRecovery = (idBool)aData;

    /* MediaRecovery 외에도 해야하는 경우이거나,
     * 현재 MediaReovery 중일 경우 */
    if( ( sForMediaRecovery == ID_FALSE ) ||
        ( smrRecoveryMgr::isMediaRecoveryPhase() == ID_TRUE ) )
    {
        IDE_ASSERT( smmManager::getOIDPtr(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                    SM_MAKE_OID( aPID, aOffset ),
                                    (void**)&sTableHeader )
                    == IDE_SUCCESS );

        idlOS::memcpy(&(sTableHeader->mIsConsistent), aImage, aSize);

        IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                    aPID) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smcTableUpdate::undo_SMC_TABLEHEADER_SET_INCONSISTENT(
                                                   smTID       /*aTID*/,
                                                   scSpaceID    /*aSpaceID*/,
                                                   scPageID      aPID,
                                                   scOffset      aOffset,
                                                   vULong      /*aData*/,
                                                   SChar        *aImage,
                                                   SInt          aSize,
                                                   UInt        /*aFlag*/)
{

    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                    SM_MAKE_OID( aPID, aOffset ),
                                   (void**) &sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sTableHeader->mIsConsistent), aImage, aSize);


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                     aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_INDEX_SET_FLAG             */
IDE_RC smcTableUpdate::redo_SMC_INDEX_SET_FLAG(
                                          smTID      /*aTID*/,
                                          scSpaceID    /*aSpaceID*/,
                                          scPageID      aPID,
                                          scOffset      aOffset,
                                          vULong      /*aData*/,
                                          SChar        *aImage,
                                          SInt          aSize,
                                          UInt        /*aFlag*/)
{

    smOID     sOIDIndex;
    void    * sIndexHeader;
    UInt    * sIndexHeaderFlag;

    sOIDIndex    = SM_MAKE_OID(aPID, aOffset);
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       sOIDIndex,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );

    // IndexHeader
    sIndexHeaderFlag = smLayerCallback::getFlagPtrOfIndexHeader( sIndexHeader );
    idlOS::memcpy(sIndexHeaderFlag, aImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smcTableUpdate::undo_SMC_INDEX_SET_FLAG(
                                          smTID      /*aTID*/,
                                          scSpaceID    /*aSpaceID*/,
                                          scPageID      aPID,
                                          scOffset      aOffset,
                                          vULong      /*aData*/,
                                          SChar        *aImage,
                                          SInt          aSize,
                                          UInt        /*aFlag*/)
{

    void           * sIndexHeader;
    UInt           * sIndexHeaderFlag;
    smOID            sOIDIndex;
    UInt             sFlag;
    smcTableHeader * sTable;
    UInt             sState = 0;

    sOIDIndex    = SM_MAKE_OID(aPID, aOffset);
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       sOIDIndex,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );
    IDE_ASSERT( smcTable::getTableHeaderFromOID( smLayerCallback::getTableOIDOfIndexHeader( sIndexHeader ),
                                                 (void**)&sTable )
                == IDE_SUCCESS );

    if(smrRecoveryMgr::isRestart()== ID_FALSE)
    {
        IDE_TEST( smcTable::latchExclusive( sTable ) != IDE_SUCCESS );
        sState=1;
    }

    idlOS::memcpy(&sFlag, aImage, aSize);

    if(smrRecoveryMgr::isRestart() == ID_FALSE)
    {
        sState = 0;
        IDE_TEST( smcTable::unlatch( sTable ) != IDE_SUCCESS );
    }

    sIndexHeaderFlag = smLayerCallback::getFlagPtrOfIndexHeader( sIndexHeader );
    idlOS::memcpy(sIndexHeaderFlag, aImage, aSize);


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( smcTable::unlatch( sTable ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}


/* UPDATE TYPE : SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT */
IDE_RC smcTableUpdate::redo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT(smTID        /*aTID*/,
                                                                 scSpaceID    /*aSpaceID*/,
                                                                 scPageID     aPID,
                                                                 scOffset     aOffset,
                                                                 vULong       /*aData*/,
                                                                 SChar       *aAfterImage,
                                                                 SInt         aAfterSize,
                                                                 UInt         /*aFlag*/)
{
    scPageID         sSegPID;
    SChar*           sAfterPtr;
    smcTableHeader*  sTableHeader;

    IDE_ERROR(     aAfterImage != NULL );
    IDE_ERROR_MSG( aAfterSize == ID_SIZEOF(scPageID),
                   "aAfterSize : %"ID_UINT32_FMT, aAfterSize );

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );

    sAfterPtr = aAfterImage;

    idlOS::memcpy(&sSegPID, sAfterPtr, ID_SIZEOF(scPageID));

    sTableHeader->mFixed.mDRDB.mSegDesc.mSegHandle.mSegPID = sSegPID;
    sAfterPtr += ID_SIZEOF(scPageID);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcTableUpdate::undo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT(smTID        /*aTID*/,
                                                                 scSpaceID    /*aSpaceID*/,
                                                                 scPageID     aPID,
                                                                 scOffset     aOffset,
                                                                 vULong       /*aData*/,
                                                                 SChar       *aBeforeImage,
                                                                 SInt         aBeforeSize,
                                                                 UInt         /*aFlag*/)
{
    scPageID         sSegPID;
    SChar*           sBeforePtr;
    smcTableHeader*  sTableHeader;

    IDE_ERROR(     aBeforeImage != NULL );
    IDE_ERROR_MSG( aBeforeSize == ID_SIZEOF(scPageID),
                   "aBeforeSize : %"ID_UINT32_FMT, aBeforeSize );

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );

    /* BUG-15710: Redo시에 특정영역의 데이타가 올바른지 ASSERT를 걸고 있습니다.
       IDE_DASSERT( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_TRUE );
    */

    sBeforePtr = aBeforeImage;

    idlOS::memcpy(&sSegPID, sBeforePtr, ID_SIZEOF(scPageID));
    sTableHeader->mFixed.mDRDB.mSegDesc.mSegHandle.mSegPID = sSegPID;
    sBeforePtr += ID_SIZEOF(scPageID);


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Update Type: SMR_SMC_TABLEHEADER_SET_SEGSTOATTR                  */
IDE_RC smcTableUpdate::redo_undo_SMC_TABLEHEADER_SET_SEGSTOATTR(smTID    /*aTID*/,
                                                                scSpaceID/* aSpaceID */,
                                                                scPageID aPID,
                                                                scOffset aOffset,
                                                                vULong   /*aData */,
                                                                SChar   *aImage,
                                                                SInt     /* aSize */,
                                                                UInt    /*aFlag*/)
{
    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy( &(sTableHeader->mFixed.mDRDB.mSegDesc.mSegHandle.mSegStoAttr),
                   aImage,
                   ID_SIZEOF(smiSegStorageAttr) );


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_SET_INSERTLIMIT                  */
IDE_RC smcTableUpdate::redo_undo_SMC_TABLEHEADER_SET_INSERTLIMIT(smTID    /*aTID*/,
                                                                 scSpaceID/* aSpaceID*/,
                                                                 scPageID aPID,
                                                                 scOffset aOffset,
                                                                 vULong   /* aData */,
                                                                 SChar   *aImage,
                                                                 SInt     /* aSize */,
                                                                 UInt    /*aFlag*/)
{
    smcTableHeader *sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    SM_MAKE_OID( aPID, aOffset ),
                    (void**)&sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy( &(sTableHeader->mFixed.mDRDB.mSegDesc.mSegHandle.mSegAttr),
                   aImage,
                   ID_SIZEOF(smiSegAttr) );


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}





/* Update type: SMR_SMC_INDEX_SET_SEGSTOATTR             */
IDE_RC smcTableUpdate::redo_SMC_INDEX_SET_SEGSTOATTR( smTID      /*aTID*/,
                                                      scSpaceID    /*aSpaceID*/,
                                                      scPageID      aPID,
                                                      scOffset      aOffset,
                                                      vULong      /*aData*/,
                                                      SChar        *aImage,
                                                      SInt          aSize,
                                                      UInt        /*aFlag*/)
{

    smOID                  sOIDIndex;
    void                 * sIndexHeader;
    smiSegStorageAttr    * sSegStoAttr;

    sOIDIndex    = SM_MAKE_OID(aPID, aOffset);
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       sOIDIndex,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );


    // IndexHeader
    sSegStoAttr = smLayerCallback::getIndexSegStoAttrPtr( sIndexHeader );
    idlOS::memcpy(sSegStoAttr, aImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcTableUpdate::undo_SMC_INDEX_SET_SEGSTOATTR(
                                          smTID      /*aTID*/,
                                          scSpaceID    /*aSpaceID*/,
                                          scPageID      aPID,
                                          scOffset      aOffset,
                                          vULong      /*aData*/,
                                          SChar        *aImage,
                                          SInt          aSize,
                                          UInt        /*aFlag*/)
{

    void              * sIndexHeader;
    smOID               sOIDIndex;
    smcTableHeader    * sTable;
    UInt                sState = 0;
    smiSegStorageAttr * sSegStoAttr;

    sOIDIndex    = SM_MAKE_OID(aPID, aOffset);
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       sOIDIndex,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );
    IDE_ASSERT( smcTable::getTableHeaderFromOID( smLayerCallback::getTableOIDOfIndexHeader( sIndexHeader ),
                                                 (void**)&sTable )
                == IDE_SUCCESS );

    sSegStoAttr = smLayerCallback::getIndexSegStoAttrPtr( sIndexHeader );
    idlOS::memcpy(sSegStoAttr, aImage, aSize);


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( smcTable::unlatch( sTable ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_INDEX_SET_SEGATTR             */
IDE_RC smcTableUpdate::redo_SMC_SET_INDEX_SEGATTR( smTID      /*aTID*/,
                                                   scSpaceID    /*aSpaceID*/,
                                                   scPageID      aPID,
                                                   scOffset      aOffset,
                                                   vULong      /*aData*/,
                                                   SChar        *aImage,
                                                   SInt          aSize,
                                                   UInt        /*aFlag*/)
{
    smOID           sOIDIndex;
    void          * sIndexHeader;
    smiSegAttr    * sSegAttr;

    sOIDIndex    = SM_MAKE_OID(aPID, aOffset);
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       sOIDIndex,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );


    // IndexHeader
    sSegAttr = smLayerCallback::getIndexSegAttrPtr( sIndexHeader );
    idlOS::memcpy(sSegAttr, aImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcTableUpdate::undo_SMC_SET_INDEX_SEGATTR(
    smTID      /*aTID*/,
    scSpaceID    /*aSpaceID*/,
    scPageID      aPID,
    scOffset      aOffset,
    vULong      /*aData*/,
    SChar        *aImage,
    SInt          aSize,
    UInt        /*aFlag*/)
{

    void              * sIndexHeader;
    smOID               sOIDIndex;
    smcTableHeader    * sTable;
    UInt                sState = 0;
    smiSegAttr        * sSegAttr;

    sOIDIndex    = SM_MAKE_OID(aPID, aOffset);
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       sOIDIndex,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );
    IDE_ASSERT( smcTable::getTableHeaderFromOID( smLayerCallback::getTableOIDOfIndexHeader( sIndexHeader ),
                                                 (void**)&sTable )
                == IDE_SUCCESS );

    sSegAttr = smLayerCallback::getIndexSegAttrPtr( sIndexHeader );
    idlOS::memcpy(sSegAttr, aImage, aSize);


    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                 aPID) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( smcTable::unlatch( sTable ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}


