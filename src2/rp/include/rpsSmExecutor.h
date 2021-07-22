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
 * $Id: rpsSmExecutor.h 85848 2019-07-15 07:36:08Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_RPS_SM_EXECUTOR_H_
#define _O_RPS_SM_EXECUTOR_H_ 1

#include <smiTrans.h>
#include <smiDef.h>

#include <rpdQueue.h>
#include <rpdTransTbl.h>

#define RP_UPDATE_SAVEPOINT_NAME "metaUpdateSavePoint"
#define RP_DELETE_SAVEPOINT_NAME "deleteUpdateSavePoint"
#define RP_PRIMARY_KEY_INDEX_ID  (0)

class rpsSmExecutor
{
private:
    smiColumnList        mUpdateColList[ QCI_MAX_COLUMN_COUNT ];
    UInt                 mUpdateColID[ QCI_MAX_COLUMN_COUNT ];
    mtcColumn            mCol[ QCI_MAX_COLUMN_COUNT ]; /* related to update column*/
    smiFetchColumnList   mFetchColumnList[ QCI_MAX_COLUMN_COUNT ];
    SChar              * mRealRow;
    UInt                 mRealRowSize;
    idvSQL             * mOpStatistics;
    idBool               mIsConflictWhenNotEnoughSpace;
    UInt                 mDeleteRowCount;
public:
    UInt mCursorOpenFlag;

public:
    rpsSmExecutor();

    // PROJ-1705 readRow를 위한 공간 할당을 위하여
    // meta 정보를 넘겨준다. 레코드 최대크기로 할당함.
    IDE_RC initialize( idvSQL  * aOpStatistics,
                       rpdMeta * aMeta,
                       idBool    aIsConflictWhenNotEnoughSpace );

    void   destroy();

    IDE_RC executeInsert( smiTrans         * aTrans,
                          rpdXLog          * aXLog,
                          rpdMetaItem      * aMetaItem,
                          qciIndexTableRef * aIndexTableRef,
                          rpApplyFailType  * aFailType );

    IDE_RC executeSyncInsert( rpdXLog          * aXLog,
                              smiTrans         * aSmiTrans,
                              smiStatement     * aSmiStmt,
                              smiTableCursor   * aCursor,
                              rpdMetaItem      * aMetaItem,
                              ULong              aSyncTupleSuccessCount,
                              idBool           * aIsBegunSyncStmt,
                              idBool           * aIsOpenedSyncCursor,
                              rpApplyFailType  * aFailType );

    IDE_RC setCursorOpenFlag( smiTrans   * aTrans,
                              const void * aTable,
                              UInt       * aFlag );

    IDE_RC stmtEndAndCursorClose( smiStatement   * mSmiStmt,
                                  smiTableCursor * aCursor,
                                  idBool         * aIsBegunSyncStmt,
                                  idBool         * aIsOpenedCursor,
                                  ULong          * aSyncTupleSuccessCount,
                                  SInt             aStmtEndFalg );

    IDE_RC stmtBeginAndCursorOpen( smiTrans       * aTrans,
                                   smiStatement   * aSmiStmt,
                                   smiTableCursor * aCursor,
                                   rpdMetaItem    * aMetaItem,
                                   idBool         * aIsBegunSyncStmt,
                                   idBool         * aIsOpenedCursor );

    IDE_RC executeUpdate( smiTrans         * aTrans,
                          rpdXLog          * aXLog,
                          rpdMetaItem      * aMetaItem,
                          qciIndexTableRef * aIndexTableRef,
                          smiRange         * aKeyRange,
                          rpApplyFailType  * aFailType,
                          idBool             aCompareBeforeImage,
                          mtcColumn        * aTsFlag );

    IDE_RC executeDelete( smiTrans         * aTrans,
                          rpdXLog          * aXLog,
                          rpdMetaItem      * aMetaItem,
                          qciIndexTableRef * aIndexTableRef,
                          smiRange         * aKeyRange,
                          rpApplyFailType  * aFailType,
                          idBool             aCheckRowExistence );

    // PROJ-1705
    IDE_RC makeFetchColumnList(rpdXLog            *aXLog,
                               const smOID         aTableOID,
                               const void         *aIndexHandle);

    IDE_RC makeUpdateColumnList(rpdXLog *aXLog, smOID aTableOID);
    IDE_RC reinitUpdateColumnList();

    IDE_RC compareImageTS(rpdXLog    *aXLog,
                          smOID       aTableOID,
                          const void *aRow,
                          idBool     *aIsConflict,
                          mtcColumn  *aTsFlag);

    IDE_RC compareInsertImage( smiTrans         * aTrans,
                               rpdXLog          * aXLog,
                               rpdMetaItem      * aMetaItem,
                               smiRange         * aKeyRange,
                               idBool           * aIsConflict,
                               mtcColumn        * aTsFlag,
                               rpApplyFailType  * aFailType );

    IDE_RC compareUpdateImage(rpdXLog    *aXLog,
                              smOID       aTableOID,
                              const void *aRow,
                              idBool     *aIsConflict);

    IDE_RC openLOBCursor( smiTrans         * aTrans,
                          rpdXLog          * aXLog,
                          rpdMetaItem      * aMetaItem,
                          smiRange         * aKeyRange,
                          rpdTransTbl      * aTransTbl,
                          idBool           * aIsLOBOperationException,
                          rpApplyFailType  * aFailType );

    IDE_RC closeLOBCursor( rpdXLog     *aXLog,
                           rpdTransTbl *aTransTbl,
                           idBool      *aIsLOBOperationException );

    IDE_RC prepareLOBWrite( rpdXLog     *aXLog,
                            rpdTransTbl *aTransTbl,
                            idBool      *aIsLOBOperationException );

    IDE_RC finishLOBWrite( rpdXLog     *aXLog,
                           rpdTransTbl *aTransTbl,
                           idBool      *aIsLOBOperationException);

    IDE_RC trimLOB( rpdXLog     *aXLog,
                    rpdTransTbl *aTransTbl,
                    idBool      *aIsLOBOperationException );

    IDE_RC writeLOBPiece( rpdXLog     *aXLog,
                          rpdTransTbl *aTransTbl,
                          idBool      *aIsLOBOperationException );

    // PROJ-1705
    IDE_RC convertToNonMtdValue(rpdXLog    * aXLog,
                                smiValue   * aACols,
                                const void * aTable);

    IDE_RC convertXlogToSmiValue(rpdXLog        * aXLog,
                                 smiValue       * aACols,
                                 const void     * aTable);
    /* Compress column replication */
    IDE_RC convertValueToOID( rpdXLog        * aXLog,
                              smiValue       * aACols,
                              smiValue       * aAConvertedCols,
                              const void     * aTable,
                              smiStatement   * aStmt,
                              smOID          * aCompressColOIDs );

    /* BUG-31545 replication statistics wrapper functions */
    IDE_RC cursorOpen(smiTableCursor      * aCursor,
                      smiStatement        * aStatement,
                      const void          * aTable,
                      const void          * aIndex,
                      smSCN                 aSCN,
                      const smiColumnList * aColumns,
                      const smiRange      * aKeyRange,
                      const smiRange      * aKeyFilter,
                      const smiCallBack   * aRowFilter,
                      UInt                  aFlag,
                      smiCursorType         aCursorType,
                      smiCursorProperties * aProperties);

    IDE_RC cursorClose(smiTableCursor * aCursor);

    IDE_RC correctQMsgIDSeq( smiStatement     * aSmiStmt,
                             rpdXLog          * aXLog,
                             rpdMetaItem      * aMetaItem );

    inline UInt getDeleteRowCount()
    {
        return mDeleteRowCount;
    }
};

#endif /* _O_RPS_SM_EXECUTOR_H_ */

