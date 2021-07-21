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
 * $Id$
 **********************************************************************/

#ifndef _Q_QMX_SIMPLE_H_
#define _Q_QMX_SIMPLE_H_ 1

#include <smiDef.h>
#include <qc.h>
#include <qtcDef.h>
#include <qcmTableInfo.h>
#include <mtc.h>

typedef struct qmxFastRow
{
    struct qmxFastRow   * leftRow;
    const void          * row;
} qmxFastRow;

typedef struct qmxFastRowInfo
{
    qmxFastRow          * rowBuf;
    qmxFastRowInfo      * next;
} qmxFastRowInfo;

typedef struct qmxFastScanInfo
{
    qmncSCAN            * scan;
    qcmIndex            * index;
    const void          * indexHandle;

    void                * mtdValue[QC_MAX_KEY_COLUMN_COUNT * 2];
    smiRange            * keyRange;
    smiRange              range;
    qtcMetaRangeColumn    rangeColumn[QC_MAX_KEY_COLUMN_COUNT * 2];

    smiTableCursor        cursor;
    smiCursorProperties   property;
    idBool                inited;
    idBool                opened;

    const void          * row;
    scGRID                rid;

    qmxFastRowInfo      * rowInfo;
    UInt                  rowCount;

    qmxFastRowInfo      * curRowInfo;
    qmxFastRow          * curRow;
    UInt                  curIdx;

} qmxFastScanInfo;

// PROJ-2551 simple query √÷¿˚»≠
class qmxSimple
{
public:

    static IDE_RC fastExecute( smiTrans     * aSmiTrans,
                               qcStatement  * aStatement,
                               UShort       * aBindColInfo,
                               UChar        * aBindBuffer,
                               UInt           aShmSize,
                               UChar        * aShmResult,
                               UInt         * aResultSize,
                               UInt         * aRowCount );

    static IDE_RC fastMoveNextResult( qcStatement  * aStatement,
                                      idBool       * aRecordExist );

private:

    static IDE_RC getSimpleCBigint( struct qciBindParam  * aParam,
                                    SInt                   aIndicator,
                                    UChar                * aBindBuffer,
                                    mtdBigintType        * aValue );

    static IDE_RC getSimpleCNumeric( struct qciBindParam  * aParam,
                                     SInt                   aIndicator,
                                     UChar                * aBindBuffer,
                                     mtdNumericType       * aNumeric );

    static IDE_RC getSimpleCTimestamp( struct qciBindParam  * aParam,
                                       SInt                   aIndicator,
                                       UChar                * aBindBuffer,
                                       mtdDateType          * aDate );

    static IDE_RC getSimpleCValue( qmnValueInfo             * aValueInfo,
                                   void                    ** aMtdValue,
                                   struct qciBindParamInfo  * aParamInfo,
                                   UChar                    * aBindBuffer,
                                   SChar                   ** aBuffer,
                                   idBool                     aIsMakeRange,
                                   idBool                     aNeedCanonize );

    static IDE_RC getSimpleConstMtdValue( qcStatement  * aStatement,
                                          mtcColumn    * aColumn,
                                          void       ** aMtdValue,
                                          void        * aValue,
                                          SChar      ** aBuffer,
                                          idBool        aNeedCanonize,
                                          idBool        aIsQueue,
                                          void        * aQueueMsgIDSeq );

    static IDE_RC getSimpleMtdBigint( struct qciBindParam  * aParam,
                                      mtdBigintType        * aValue );

    static IDE_RC getSimpleMtdNumeric( struct qciBindParam  * aParam,
                                       mtdNumericType       * aValue );

    static IDE_RC getSimpleMtdValue( qmnValueInfo             * aValueInfo,
                                     void                    ** aMtdValue,
                                     struct qciBindParamInfo  * aParamInfo,
                                     SChar                   ** aBuffer,
                                     idBool                     aIsMakeRange,
                                     idBool                     aNeedCanonize );

    static IDE_RC getSimpleMtdValueSize( mtcColumn * aColumn,
                                         void      * aMtdValue,
                                         UInt      * aSize );

    static IDE_RC setSimpleMtdValue( mtcColumn  * aColumn,
                                     SChar      * aResult,
                                     const void * aMtdValue );

    static IDE_RC makeSimpleRidRange( qcStatement          * aStatement,
                                      struct qmncSCAN      * aSCAN,
                                      UChar                * aBindBuffer,
                                      qmxFastScanInfo      * aScanInfo,
                                      UInt                   aScanCount,
                                      void                ** aMtdValue,
                                      qtcMetaRangeColumn   * aRangeColumn,
                                      smiRange             * aRange,
                                      smiRange            ** aKeyRange,
                                      idBool               * aIsNull,
                                      SChar               ** aBuffer );

    static IDE_RC makeSimpleKeyRange( qcStatement          * aStatement,
                                      struct qmncSCAN      * aSCAN,
                                      qcmIndex             * aIndex,
                                      UChar                * aBindBuffer,
                                      qmxFastScanInfo      * aScanInfo,
                                      UInt                   aScanCount,
                                      void                ** aMtdValue,
                                      qtcMetaRangeColumn   * aRangeColumn,
                                      smiRange             * aRange,
                                      smiRange            ** aKeyRange,
                                      idBool               * aIsNull,
                                      SChar               ** aBuffer );

    static IDE_RC updateSimpleKeyRange( qmncSCAN            * aSCAN,
                                        qmxFastScanInfo     * aScanInfo,
                                        UInt                  aScanCount,
                                        void               ** aMtdValue,
                                        qtcMetaRangeColumn  * aRangeColumn,
                                        idBool              * aIsNull );

    static IDE_RC calculateSimpleValues( qcStatement      * aStatement,
                                         struct qmncUPTE  * aUPTE,
                                         UChar            * aBindBuffer,
                                         const void       * aRow,
                                         smiValue         * aSmiValues,
                                         SChar            * aBuffer );

    static IDE_RC isSimpleNullValue( mtcColumn  * aColumn,
                                     const void * aValue,
                                     idBool     * aIsNull );

    static IDE_RC checkSimpleNullValue( mtcColumn  * aColumn,
                                        const void * aValue );

    static IDE_RC setSimpleSmiValue( mtcColumn  * aColumn,
                                     void       * aValue,
                                     smiValue   * aSmiValue );

    static IDE_RC calculateSimpleToChar( qmnValueInfo * aValueInfo,
                                         mtdDateType  * aDateValue,
                                         mtdCharType  * aCharValue );

    static IDE_RC executeFastSelect( smiTrans     * aSmiTrans,
                                     qcStatement  * aStatement,
                                     UShort       * aBindColInfo,
                                     UChar        * aBindBuffer,
                                     UInt           aShmSize,
                                     UChar        * aShmResult,
                                     UInt         * aResultSize,
                                     UInt         * aRowCount );

    static IDE_RC executeFastJoinSelect( smiTrans     * aSmiTrans,
                                         qcStatement  * aStatement,
                                         UShort       * aBindColInfo,
                                         UChar        * aBindBuffer,
                                         UInt           aShmSize,
                                         UChar        * aShmResult,
                                         UInt         * aResultSize,
                                         UInt         * aRowCount );

    static IDE_RC doFastScan( qcStatement      * aStatement,
                              smiStatement     * aSmiStmt,
                              UChar            * aBindBuffer,
                              SChar            * aBuffer,
                              qmxFastScanInfo  * aScanInfo,
                              UInt               aScanIndex );

    static IDE_RC executeFastInsert( smiTrans     * aSmiTrans,
                                     qcStatement  * aStatement,
                                     UChar        * aBindBuffer,
                                     UInt         * aRowCount );

    static IDE_RC executeFastUpdate( smiTrans     * aSmiTrans,
                                     qcStatement  * aStatement,
                                     UChar        * aBindBuffer,
                                     UInt         * aRowCount );

    static IDE_RC executeFastDelete( smiTrans     * aSmiTrans,
                                     qcStatement  * aStatement,
                                     UChar        * aBindBuffer,
                                     UInt         * aRowCount );

    static IDE_RC executeFastPartitionSelect( smiTrans     * aSmiTrans,
                                              qcStatement  * aStatement,
                                              UShort       * aBindColInfo,
                                              UChar        * aBindBuffer,
                                              UInt           aShmSize,
                                              UChar        * aShmResult,
                                              UInt         * aResultSize,
                                              UInt         * aRowCount );

    static IDE_RC executeFastPartitionInsert( smiTrans     * aSmiTrans,
                                              qcStatement  * aStatement,
                                              UChar        * aBindBuffer,
                                              UInt         * aRowCount );

    static IDE_RC executeFastPartitionUpdate( smiTrans     * aSmiTrans,
                                              qcStatement  * aStatement,
                                              UChar        * aBindBuffer,
                                              UInt         * aRowCount );

    static IDE_RC executeFastPartitionDelete( smiTrans     * aSmiTrans,
                                              qcStatement  * aStatement,
                                              UChar        * aBindBuffer,
                                              UInt         * aRowCount );

    static IDE_RC makeSimplePartKeyRange( qcStatement          * aStatement,
                                          struct qmncPCRD      * aPCRD,
                                          UChar                * aBindBuffer,
                                          void                ** aMtdValue,
                                          mtcColumn            * aPartKeyColumns,
                                          UInt                 * aPartKeyColsFlag,
                                          smiRange             * aRange,
                                          mtkRangeCallBack     * aMinimumCallBack,
                                          mtkRangeCallBack     * aMaximumCallBack,
                                          smiRange            ** aKeyRange,
                                          idBool               * aIsNull,
                                          SChar               ** aBuffer );
};

#endif  // _Q_QMX_H_
