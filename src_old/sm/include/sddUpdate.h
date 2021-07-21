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
 * $Id: sddUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 * 
 * 본 파일은 디스크 관련 redo/undo 함수에 대한 헤더파일이다.
 *
 **********************************************************************/

#ifndef _O_SDD_UPDATE_H_
#define _O_SDD_UPDATE_H_ 1

#include <smDef.h>

class sddUpdate
{
public:

    static IDE_RC redo_SCT_UPDATE_DRDB_CREATE_TBS(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );
    
    static IDE_RC undo_SCT_UPDATE_DRDB_CREATE_TBS(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart = ID_FALSE );

    static IDE_RC redo_SCT_UPDATE_DRDB_DROP_TBS(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );
    
    static IDE_RC undo_SCT_UPDATE_DRDB_DROP_TBS(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );

    static IDE_RC redo_SCT_UPDATE_DRDB_CREATE_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );
    
    static IDE_RC undo_SCT_UPDATE_DRDB_CREATE_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart = ID_FALSE );

    static IDE_RC redo_SCT_UPDATE_DRDB_DROP_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );
    
    static IDE_RC undo_SCT_UPDATE_DRDB_DROP_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart = ID_FALSE );

    static IDE_RC redo_SCT_UPDATE_DRDB_EXTEND_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );

    static IDE_RC undo_SCT_UPDATE_DRDB_EXTEND_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart = ID_FALSE );

    static IDE_RC redo_SCT_UPDATE_DRDB_SHRINK_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );

    static IDE_RC undo_SCT_UPDATE_DRDB_SHRINK_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart = ID_FALSE );
    
    static IDE_RC redo_SCT_UPDATE_DRDB_AUTOEXTEND_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          /* aIsRestart */ );
    
    static IDE_RC undo_SCT_UPDATE_DRDB_AUTOEXTEND_DBF(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart = ID_FALSE );

    // ALTER TABLESPACE TBS1 OFFLINE .... 에 대한 REDO 수행
    static IDE_RC redo_SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

    // ALTER TABLESPACE TBS1 OFFLINE .... 에 대한 UNDO 수행
    static IDE_RC undo_SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

    // ALTER TABLESPACE TBS1 ONLINE .... 에 대한 REDO 수행
    static IDE_RC redo_SCT_UPDATE_DRDB_ALTER_TBS_ONLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

    // ALTER TABLESPACE TBS1 ONLINE .... 에 대한 UNDO 수행
    static IDE_RC undo_SCT_UPDATE_DRDB_ALTER_TBS_ONLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            /* aFileID */,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

    static IDE_RC redo_SCT_UPDATE_DRDB_ALTER_DBF_ONLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

    static IDE_RC undo_SCT_UPDATE_DRDB_ALTER_DBF_ONLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

    static IDE_RC redo_SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans, 
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

    static IDE_RC undo_SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE(
                        idvSQL        * aStatistics, 
                        void          * aTrans,
                        smLSN           aCurLSN,
                        scSpaceID       aSpaceID,
                        UInt            aFileID,
                        UInt            aValueSize,
                        SChar         * aValuePtr,
                        idBool          aIsRestart );

//For Member
public:

private:
    // ALTER TABLESPACE TBS1 ONLINE/OFFLINE .... 에 대한 Log Image를 분석한다.
    static IDE_RC getAlterTBSOnOffImage( UInt       aValueSize,
                                         SChar    * aValuePtr,
                                         UInt     * aState );

    static IDE_RC getAlterDBFOnOffImage( UInt       aValueSize,
                                         SChar    * aValuePtr,
                                         UInt     * aState );

    static IDE_RC  getTBSDBF( scSpaceID             aSpaceID,
                              UInt                  aFileID,
                              sddTableSpaceNode  ** aSpaceNode,
                              sddDataFileNode    ** aFileNode );
    
};

#endif // _O_SDD_UPDATE_H_


