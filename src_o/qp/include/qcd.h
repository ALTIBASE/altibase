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
 * $Id: qcd.h 3741 2004-02-10 01:20:36Z bethy $
 **********************************************************************/
#ifndef _O_QCD_H_
#define  _O_QCD_H_  1

#include <qc.h>
#include <qci.h>

typedef void* QCD_HSTMT;

#define QCD_MAKE_BIND_DATA( _BindData_, _Value_, _ValueSize_, _BindId_, _Column_ )      \
{                                           \
    (_BindData_)->id     = (_BindId_);      \
    (_BindData_)->name   = NULL;            \
    (_BindData_)->column = (_Column_);      \
    (_BindData_)->data   = (_Value_);       \
    (_BindData_)->size   = (_ValueSize_);   \
    (_BindData_)->next   = NULL;            \
}


class qcd
{
 public:

    static void initStmt(QCD_HSTMT * aHstmt);

    static IDE_RC allocStmt( qcStatement * aStatement,
                             QCD_HSTMT   * aHstmt /* OUT */ );

    static IDE_RC prepare( QCD_HSTMT     aHstmt,
                           qcStatement * aQcStmt,     // parent
                           qcStatement * aExecQcStmt, // child
                           qciStmtType * aStmtType,
                           SChar       * aSqlString,
                           UInt          aSqlStringLen,
                           idBool        aExecMode );

    static IDE_RC bindParamInfoSet( QCD_HSTMT      aHstmt,
                                    mtcColumn    * aColumn,
                                    UShort         aBindId,
                                    qsInOutType    aInOutType );

    static IDE_RC bindParamData( QCD_HSTMT      aHstmt,
                                 void         * aValue,
                                 UInt           aValueSize,
                                 UShort         aBindId,
                                 iduMemory    * aMemory,
                                 qciBindData ** aBindDataList,
                                 qsInOutType    aInOutType );

    static IDE_RC execute( QCD_HSTMT     aHstmt,
                           qcStatement * aQcStmt, // BUG-37467
                           qciBindData * aOutBindParamDataList,
                           vSLong      * aAffectedRowCount, /* OUT */
                           idBool      * aResultSetExist, /* OUT */
                           idBool      * aRecordExist, /* OUT */
                           idBool        aIsFirst );

    static IDE_RC fetch( qcStatement * aQcStmt,
                         iduMemory   * aMemory,
                         QCD_HSTMT     aHstmt,
                         qciBindData * aBindColumnDataList, /* OUT */
                         idBool      * aNextRecordExist /* OUT */ );

    static IDE_RC freeStmt( QCD_HSTMT   aHstmt,
                            idBool      aFreeMode );

    static IDE_RC addBindColumnDataList( iduMemory    * aMemory,
                                         qciBindData ** aBindDataList,
                                         mtcColumn    * aColumn,
                                         void         * aData,
                                         UShort         aBindId );

    static IDE_RC checkBindParamCount( QCD_HSTMT aHstmt,
                                       UShort    aBindParamCount );

    static IDE_RC checkBindColumnCount( QCD_HSTMT aHstmt,
                                        UShort     aBindColumnCount );

    /* PROJ-2197 PSM Renewal
     * PSM에서 일반 DML을 수행하는 경우에 해당 DML을 수행하는
     * qcStatement를 가져오기 위한 함수 */
    static IDE_RC getQcStmt( QCD_HSTMT      aHstmt,
                             qcStatement ** aQcStmt );

    static IDE_RC endFetch( QCD_HSTMT aHstmt );

    // BUG-41248 DBMS_SQL package
    static IDE_RC allocStmtNoParent( void        * aMmSession,
                                     idBool        aDedicatedMode,
                                     QCD_HSTMT   * aHstmt /* OUT */ );

    static IDE_RC executeNoParent( QCD_HSTMT     aHstmt,
                                   qciBindData * aOutBindParamDataList,
                                   vSLong      * aAffectedRowCount, /* OUT */
                                   idBool      * aResultSetExist, /* OUT */
                                   idBool      * aRecordExist /* OUT */,
                                   idBool        aIsFirst );
    
    static IDE_RC bindParamInfoSetByName( QCD_HSTMT      aHstmt,
                                          mtcColumn    * aColumn,
                                          SChar        * aBindName,
                                          qsInOutType    aInOutType );

    static IDE_RC bindParamDataByName( QCD_HSTMT     aHstmt,
                                       void        * aValue,
                                       UInt          aValueSize,
                                       SChar       * aBindName );

 private:

    static void makeBindParamInfo( qciBindParam * aBindParam,
                                   mtcColumn    * aColumn,
                                   UShort         aBindId,
                                   qsInOutType    aInOutType );

    // BUG-41248 DBMS_SQL package
    static void makeBindParamInfoByName( qciBindParam * aBindParam,
                                         mtcColumn    * aColumn,
                                         SChar        * aBindName,
                                         qsInOutType    aInOutType );

    static void makeBindDataByName( qciBindData * aBindData,
                                    void        * aValue,
                                    UInt          aValueSize,
                                    SChar       * aBindName,
                                    mtcColumn   * aColumn );
};

#endif // _O_QCD_H_
