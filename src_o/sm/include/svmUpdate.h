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
 
#ifndef _O_SVM_UPDATE_H_
#define _O_SVM_UPDATE_H_ 1

#include <svm.h>

class svmUpdate
{
//For Operation
public:
    static IDE_RC redo_SCT_UPDATE_VRDB_CREATE_TBS(
                      idvSQL*            aStatistics, 
                      void*              aTrans,
                      smLSN              aCurLSN,
                      scSpaceID          aSpaceID,
                      UInt               aFileID,
                      UInt               aValueSize ,
                      SChar*             aValuePtr ,
                      idBool             aIsRestart );
    
    static IDE_RC undo_SCT_UPDATE_VRDB_CREATE_TBS(
                      idvSQL*            aStatistics, 
                      void*              aTrans,
                      smLSN              aCurLSN,
                      scSpaceID          aSpaceID,
                      UInt               aFileID,
                      UInt               aValueSize ,
                      SChar*             aValuePtr ,
                      idBool             aIsRestart );
    
    static IDE_RC redo_SCT_UPDATE_VRDB_DROP_TBS(
                      idvSQL*            aStatistics, 
                      void*              aTrans,
                      smLSN              aCurLSN,
                      scSpaceID          aSpaceID,
                      UInt               aFileID,
                      UInt               aValueSize ,
                      SChar*             aValuePtr ,
                      idBool             aIsRestart );
    
    static IDE_RC undo_SCT_UPDATE_VRDB_DROP_TBS(
                      idvSQL*            aStatistics, 
                      void*              aTrans,
                      smLSN              aCurLSN,
                      scSpaceID          aSpaceID,
                      UInt               aFileID,
                      UInt               aValueSize ,
                      SChar*             aValuePtr ,
                      idBool             aIsRestart );


    // ALTER TABLESPACE TBS1 AUTOEXTEND .... 에 대한 REDO 수행
    static IDE_RC redo_SCT_UPDATE_VRDB_ALTER_AUTOEXTEND(
                       idvSQL*              aStatistics, 
                       void*                aTrans,
                       smLSN                aCurLSN,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               /* aIsRestart */ );


    // ALTER TABLESPACE TBS1 AUTOEXTEND .... 에 대한 REDO 수행
    static IDE_RC undo_SCT_UPDATE_VRDB_ALTER_AUTOEXTEND(
                       idvSQL*              aStatistics, 
                       void*                aTrans,
                       smLSN                aCurLSN,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart );

private:
    // ALTER TABLESPACE TBS1 AUTOEXTEND .... 에 대한 Log Image를 분석한다.
    static IDE_RC getAlterAutoExtendImage( UInt       aValueSize,
                                           SChar    * aValuePtr,
                                           idBool   * aAutoExtMode,
                                           scPageID * aNextPageCount,
                                           scPageID * aMaxPageCount );
    
    
};

#endif
