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
 * $Id: qcmDictionary.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCM_DICTIONARY_H_
#define _O_QCM_DICTIONARY_H_ 1

typedef struct qcmDictionaryTable
{
    qcmTableInfo        * dictionaryTableInfo;
    qcmColumn           * dataColumn;
    qcmDictionaryTable  * next;
} qcmDictionaryTable;

#define QCM_DICTIONARY_TABLE_INIT(_col_)                            \
{                                                                   \
    (_col_)->dictionaryTableInfo = NULL;                            \
    (_col_)->dataColumn          = NULL;                            \
    (_col_)->next                = NULL;                            \
}

class qcmDictionary
{
public:
    static IDE_RC executeDDL( qcStatement * aStatement,
                              SChar       * aText,
                              UInt          aDictionary = 0  );

    static IDE_RC selectDicSmOID( qcTemplate     * aTemplate,
                                  void           * aTableHandle,
                                  void           * aIndexHeader,
                                  const void     * aValue,
                                  smOID          * aSmOID );

    static IDE_RC recreateDictionaryTable( qcStatement           * aStatement,
                                           qcmTableInfo          * aTableInfo,
                                           qcmColumn             * aColumn,
                                           scSpaceID               aTBSID,
                                           qcmDictionaryTable   ** aCompInfo );

    static IDE_RC makeDictionaryTable( qcStatement           * aStatement,
                                       scSpaceID               aTBSID,
                                       qcmColumn             * aColumns,
                                       qcmCompressionColumn  * aCompCol,
                                       qcmDictionaryTable   ** aCompInfo );

    static IDE_RC updateCompressionTableSpecMeta( qcStatement * aStatement,
                                                  UInt          aOldDicTableID,
                                                  UInt          aNewDicTableID );

    /* BUG-45641 disk partitioned table에 압축 컬럼을 추가하다가 실패하는데, memory partitioned table 오류가 나옵니다. */
    static IDE_RC validateCompressColumn( qcStatement      * /* aStatement */,
                                          qdTableParseTree * aParseTree,
                                          smiTableSpaceType  aType );

    static IDE_RC rebuildDictionaryTable( qcStatement        * aStatement,
                                          qcmTableInfo       * aOldTableInfo,
                                          qcmTableInfo       * aNewTableInfo,
                                          qcmDictionaryTable * aDicTable );

    static IDE_RC removeDictionaryTable( qcStatement  * aStatement,
                                         qcmColumn    * aColumn );

    static IDE_RC updateColumnIDInCompressionTableSpecMeta(
                              qcStatement * aStatement,
                              UInt          aDicTableID,
                              UInt          aNewColumnID );

    static IDE_RC dropDictionaryTable( qcStatement  * aStatement,
                                       qcmTableInfo * aTableInfo );

    static IDE_RC removeDictionaryTableWithInfo( qcStatement  * aStatement,
                                                 qcmTableInfo * aTableInfo );

    static IDE_RC createDictionaryTable( qcStatement         * aStatement,
                                         UInt                  aUserID,
                                         qcmColumn           * aColumns,
                                         UInt                  aColumnCount,
                                         scSpaceID             aTBSID,
                                         ULong                 aMaxRows,
                                         UInt                  aInitFlagMask,
                                         UInt                  aInitFlagValue,
                                         UInt                  aParallelDegree,
                                         qcmDictionaryTable ** aDicTable );

    // PROJ-2397 Compressed Column Table Replication
    static IDE_RC makeDictValueForCompress( smiStatement  * aStatement,
                                            void          * aTableHandle,
                                            void          * aIndexHeader,
                                            smiValue      * aInsertedRow,
                                            void          * aOIDValue );

    // PROJ-2397 Compressed Column Table Replication
    static IDE_RC selectDicSmOID( smiStatement   * aSmiStatement,
                                  void           * aTableHandle,
                                  void           * aIndexHeader,
                                  const void     * aValue,
                                  smOID          * aSmOID );
private:
};

#endif /* _O_QCM_DICTIONARY_H_ */
