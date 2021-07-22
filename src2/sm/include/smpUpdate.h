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
 * $Id: smpUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMP_UPDATE_H_
#define _O_SMP_UPDATE_H_ 1

#include <smpDef.h>

class smpUpdate
{
//For Operation
public:

    /* Update type:  SMR_SMM_PERS_UPDATE_LINK                       */
    static IDE_RC redo_undo_SMM_PERS_UPDATE_LINK(smTID     /*a_tid*/,
                                                 scSpaceID aSpaceID,
                                                 scPageID  a_pid,
                                                 scOffset  a_offset,
                                                 vULong    a_data,
                                                 SChar    *a_pImage,
                                                 SInt      a_nSize,
                                                 UInt      /*aFlag*/);
    
    /* Update type: SMR_SMC_PERS_INIT_FIXED_PAGE                         */
    static IDE_RC redo_SMC_PERS_INIT_FIXED_PAGE(smTID      /*a_tid*/,
                                                scSpaceID  aSpaceID,
                                                scPageID   a_pid,
                                                scOffset   a_offset,
                                                vULong     a_data, 
                                                SChar     *a_pImage,
                                                SInt       a_nSize,
                                                UInt       /*aFlag*/);
    
    
    /* Update type: SMR_SMC_PERS_INIT_VAR_PAGE                         */
    static IDE_RC redo_SMC_PERS_INIT_VAR_PAGE(smTID     /*a_tid*/,
                                              scSpaceID aSpaceID,
                                              scPageID  a_pid,
                                              scOffset  a_offset,
                                              vULong    a_data,
                                              SChar    *a_pAfterImage,
                                              SInt      a_nSize,
                                              UInt      /*aFlag*/);
    
    static IDE_RC redo_SMP_NTA_ALLOC_FIXED_ROW( scSpaceID    aSpaceID,
                                                scPageID     aPageID,
                                                scOffset     aOffset,
                                                idBool       aIsSetDeleteBit );
    
    // ALTER TABLESPACE TBS1 OFFLINE .... 에 대한 REDO 수행
    static IDE_RC redo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE(
                       idvSQL*              aStatistics, 
                       void*                aTrans,
                       smLSN                aCurLSN,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart );

    // ALTER TABLESPACE TBS1 OFFLINE .... 에 대한 UNDO 수행
    static IDE_RC undo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE( 
                       idvSQL*              aStatistics, 
                       void*                aTrans,
                       smLSN                aCurLSN,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart );
    

    // ALTER TABLESPACE TBS1 ONLINE .... 에 대한 REDO 수행
    static IDE_RC redo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE(
                       idvSQL*              aStatistics, 
                       void*                aTrans,
                       smLSN                aCurLSN,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart );
    
    // ALTER TABLESPACE TBS1 ONLINE .... 에 대한 UNDO 수행
    static IDE_RC undo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE(
                      idvSQL*              aStatistics, 
                      void*                aTrans,
                      smLSN                aCurLSN,
                      scSpaceID            aSpaceID,
                      UInt                 /*aFileID*/,
                      UInt                 aValueSize,
                      SChar*               aValuePtr,
                      idBool               aIsRestart );
    
    static IDE_RC redo_undo_SMC_PERS_SET_INCONSISTENCY( 
        smTID        /*aTid*/,
        scSpaceID      aSpaceID,
        scPageID       aPid,
        scOffset     /*aOffset*/,
        vULong       /*aData*/,
        SChar         *aImage,
        SInt         /*aSize*/,
        UInt         /*aFlag*/ );

//For Member
public:

private:
    // ALTER TABLESPACE TBS1 ONLINE/OFFLINE .... 에 대한 Log Image를 분석한다.
    static IDE_RC getAlterTBSOnOffImage( UInt       aValueSize,
                                         SChar    * aValuePtr,
                                         UInt     * aState );
    
    
};

#endif /* _O_SMP_UPDATE_H_ */
