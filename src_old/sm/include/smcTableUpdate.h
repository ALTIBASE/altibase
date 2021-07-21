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
 * $Id: smcTableUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMC_TABLE_UPDATE_H_
#define _O_SMC_TABLE_UPDATE_H_ 1

#include <smcDef.h>

class smcTableUpdate
{
//For Operation
public:

/* Commit Log Type: PRJ-1496 table row count */ 
    static IDE_RC redo_SMR_LT_TRANS_COMMIT(SChar     *aAfterImage,
                                           SInt       aSize, 
                                           idBool     aForMediaRecovery );

/* Update Type: SMR_SMC_TABLEHEADER_INIT                           */ 
    static IDE_RC redo_SMC_TABLEHEADER_INIT(smTID      /*aTID*/,
                                            scSpaceID  aSpaceID,
                                            scPageID   aPID,
                                            scOffset   aOffset,
                                            vULong     aData,
                                            SChar     *aAfterImage,
                                            SInt       aSize,
                                            UInt       /*aFlag*/);

    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INDEX                   */
    static IDE_RC redo_SMC_TABLEHEADER_UPDATE_INDEX(smTID      /*aTID*/,
                                                    scSpaceID  aSpaceID,
                                                    scPageID   aPID,
                                                    scOffset   aOffset,
                                                    vULong     aData,
                                                    SChar     *aImage,
                                                    SInt       aSize,
                                                    UInt       /*aFlag*/);

    static IDE_RC undo_SMC_TABLEHEADER_UPDATE_INDEX(smTID      /*aTID*/,
                                                    scSpaceID  aSpaceID,
                                                    scPageID   aPID,
                                                    scOffset   aOffset,
                                                    vULong     aData,
                                                    SChar     *aImage,
                                                    SInt       aSize,
                                                    UInt       /*aFlag*/);

    
    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_COLUMNS                     */
    static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_COLUMNS(smTID   /*aTID*/,
                                                           scSpaceID  aSpaceID,
                                                           scPageID   aPID,
                                                           scOffset   aOffset,
                                                           vULong   /*aData*/,
                                                           SChar     *aImage,
                                                           SInt       aSize,
                                                           UInt       /*aFlag*/);

    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INFO                       */
    static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_INFO(smTID      /*aTID*/,
                                                        scSpaceID  aSpaceID,
                                                        scPageID   aPID,
                                                        scOffset   aOffset,
                                                        vULong     /*aData*/,
                                                        SChar     *aImage,
                                                        SInt       aSize,
                                                        UInt       /*aFlag*/);
    
    /* Update Type: SMR_SMC_TABLEHEADER_SET_NULLROW                       */
    static IDE_RC redo_SMC_TABLEHEADER_SET_NULLROW(smTID       /*aTID*/,
                                                   scSpaceID  aSpaceID,
                                                   scPageID    aPID,
                                                   scOffset    aOffset,
                                                   vULong      aData,
                                                   SChar      *aAfterImage,
                                                   SInt        aSize,
                                                   UInt       /*aFlag*/);

    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALL                       */
    static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_ALL(smTID       /*aTID*/,
                                                       scSpaceID  aSpaceID,
                                                       scPageID    aPID,
                                                       scOffset    aOffset,
                                                       vULong      aData,
                                                       SChar      *aImage,
                                                       SInt        aSize,
                                                       UInt       /*aFlag*/);

    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO                  */
    static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_ALLOCINFO(smTID    /*aTID*/,
                                                             scSpaceID aSpaceID,
                                                             scPageID aPID,
                                                             scOffset aOffset,
                                                             vULong   aData,
                                                             SChar   *aImage,
                                                             SInt     aSize,
                                                             UInt    /*aFlag*/);
    /* Update Type: SMR_SMC_TABLEHEADER_SET_SEGSTOATTR                  */
    static IDE_RC redo_undo_SMC_TABLEHEADER_SET_SEGSTOATTR(smTID    /*aTID*/,
                                                           scSpaceID aSpaceID,
                                                           scPageID aPID,
                                                           scOffset aOffset,
                                                           vULong   aData,
                                                           SChar   *aImage,
                                                           SInt     aSize,
                                                           UInt    /*aFlag*/);
    /* Update Type: SMR_SMC_TABLEHEADER_SET_INSERTLIMIT                  */
    static IDE_RC redo_undo_SMC_TABLEHEADER_SET_INSERTLIMIT(smTID    /*aTID*/,
                                                            scSpaceID aSpaceID,
                                                            scPageID aPID,
                                                            scOffset aOffset,
                                                            vULong   aData,
                                                            SChar   *aImage,
                                                            SInt     aSize,
                                                            UInt    /*aFlag*/);
    
    /* SMR_SMC_TABLEHEADER_UPDATE_FLAG                                */
    static IDE_RC redo_SMC_TABLEHEADER_UPDATE_FLAG(smTID        /*aTID*/,
                                                   scSpaceID  aSpaceID,
                                                   scPageID     aPID,
                                                   scOffset     aOffset,
                                                   vULong       aData, 
                                                   SChar     * /*aImage*/,
                                                   SInt       /*aSize*/,
                                                   UInt       /*aFlag*/);
    
    static IDE_RC undo_SMC_TABLEHEADER_UPDATE_FLAG(smTID        /*aTID*/,
                                                   scSpaceID  aSpaceID,
                                                   scPageID     aPID,
                                                   scOffset     aOffset,
                                                   vULong       aData, 
                                                   SChar      * /*aImage*/,
                                                   SInt         /*aSize*/,
                                                   UInt         /*aFlag*/);

    /* Update type:  SMR_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT         */
    static IDE_RC 
    redo_undo_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT(smTID    /* aTID */,
                                                  scSpaceID  aSpaceID,
                                                  scPageID aPID,
                                                  scOffset aOffset,
                                                  vULong   /* aData */,
                                                  SChar*   aImage,
                                                  SInt     /* aSize */,
                                                  UInt       /*aFlag*/);

    /* PROJ-2162 */
    /* SMR_SMC_TABLEHEADER_SET_INCONSISTENT                           */
    static IDE_RC redo_SMC_TABLEHEADER_SET_INCONSISTENT(
        smTID        /*aTID*/,
        scSpaceID    aSpaceID,
        scPageID     aPID,
        scOffset     aOffset,
        vULong       aData, 
        SChar      * /*aImage*/,
        SInt         /*aSize*/,
        UInt         /*aFlag*/);

    static IDE_RC undo_SMC_TABLEHEADER_SET_INCONSISTENT(
        smTID        /*aTID*/,
        scSpaceID    aSpaceID,
        scPageID     aPID,
        scOffset     aOffset,
        vULong       aData, 
        SChar      * /*aImage*/,
        SInt         /*aSize*/,
        UInt         /*aFlag*/);

    /* Update type: SMR_SMC_INDEX_SET_FLAG                      */
    static IDE_RC redo_SMC_INDEX_SET_FLAG(smTID      /*aTID*/,
                                          scSpaceID  aSpaceID,
                                          scPageID   aPID,
                                          scOffset   aOffset,
                                          vULong     aData, 
                                          SChar     *aImage,
                                          SInt       aSize,
                                          UInt       /*aFlag*/);

    static IDE_RC undo_SMC_INDEX_SET_FLAG(smTID      /*aTID*/,
                                          scSpaceID  aSpaceID,
                                          scPageID   aPID,
                                          scOffset   aOffset,
                                          vULong     aData, 
                                          SChar     *aImage,
                                          SInt       aSize,
                                          UInt       /*aFlag*/);


    /* Update type: SMR_SMC_INDEX_SET_SEGSTOATTR                      */
    static IDE_RC redo_SMC_INDEX_SET_SEGSTOATTR(smTID      /*aTID*/,
                                                scSpaceID  aSpaceID,
                                                scPageID   aPID,
                                                scOffset   aOffset,
                                                vULong     aData, 
                                                SChar     *aImage,
                                                SInt       aSize,
                                                UInt       /*aFlag*/);

    static IDE_RC undo_SMC_INDEX_SET_SEGSTOATTR(smTID      /*aTID*/,
                                                scSpaceID  aSpaceID,
                                                scPageID   aPID,
                                                scOffset   aOffset,
                                                vULong     aData, 
                                                SChar     *aImage,
                                                SInt       aSize,
                                                UInt       /*aFlag*/);

    /* Update type: SMR_SMC_INDEX_SET_SEGATTR                      */
    static IDE_RC redo_SMC_SET_INDEX_SEGATTR(smTID      /*aTID*/,
                                             scSpaceID  aSpaceID,
                                             scPageID   aPID,
                                             scOffset   aOffset,
                                             vULong     aData, 
                                             SChar     *aImage,
                                             SInt       aSize,
                                             UInt       /*aFlag*/);

    static IDE_RC undo_SMC_SET_INDEX_SEGATTR(smTID      /*aTID*/,
                                             scSpaceID  aSpaceID,
                                             scPageID   aPID,
                                             scOffset   aOffset,
                                             vULong     aData, 
                                             SChar     *aImage,
                                             SInt       aSize,
                                             UInt       /*aFlag*/);

    /* Update type:  SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT         */
    static IDE_RC redo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT(smTID        /*aTID*/,
                                                            scSpaceID  aSpaceID,
                                                            scPageID     aPID,
                                                            scOffset     aOffset,
                                                            vULong       aData,
                                                            SChar       *aAfterImage,
                                                            SInt         aAfterSize,
                                                            UInt       /*aFlag*/);
    
    static IDE_RC undo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT(smTID        /*aTID*/,
                                                            scSpaceID  aSpaceID,
                                                            scPageID     aPID,
                                                            scOffset     aOffset,
                                                            vULong       aData,
                                                            SChar       *aBeforeImage,
                                                            SInt         aBeforeSize,
                                                            UInt       /*aFlag*/);
    
//For Member
public:
};

#endif
