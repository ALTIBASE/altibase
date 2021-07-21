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
 * $Id: qsxUtil.h 83112 2018-05-29 02:04:03Z ahra.cho $
 **********************************************************************/

#ifndef _O_QSX_UTIL_H_
#define _O_QSX_UTIL_H_ 1

#include <smiStatement.h>

#include <iduMemory.h>

#include <qc.h>
#include <qsxEnv.h>

class qsxUtil
{
  protected :
  public :
    static IDE_RC assignNull (
        qtcNode      * aNode,
        qcTemplate   * aTemplate );

    // PROJ-1904 Extend UDT
    static IDE_RC assignNull ( mtcColumn * aColumn,
                               void      * aValue,
                               idBool      aCopyRef );

    static IDE_RC assignValue (
        iduMemory    * aMemory,
        mtcStack     * aSourceStack,
        SInt           aSourceStackRemain,
        qtcNode      * aDestNode,
        qcTemplate   * aDestTemplate,
        idBool         aCopyRef );

    // BUG-11192 date_format session property
    // mtv::executeConvert()시 mtcTemplate이 필요함
    // aDestTemplate 인자 추가.
    // by kumdory, 2005-04-08
    static IDE_RC assignValue (
        iduMemory    * aMemory,
        qtcNode      * aSourceNode,
        qcTemplate   * aSourceTemplate,
        mtcStack     * aDestStack,
        SInt           aDestStackRemain,
        qcTemplate   * aDestTemplate,
        idBool         aParamOrReturn,
        idBool         aCopyRef );

    // PROJ-1075 assignValue
    // record/row/array타입에 대해 분류하여 assign.
    // 나머지 primitive타입에 대한 연산은
    // assignPrimitiveValue를 내부적으로 호출함.
    static IDE_RC assignValue (
        iduMemory    * aMemory,
        mtcColumn    * aSourceColumn,
        void         * aSourceValue,
        mtcColumn    * aDestColumn,
        void         * aDestValue,
        mtcTemplate  * aDestTemplate,
        idBool         aCopyRef );

    // PROJ-1075 rowtype에 대한 assign
    static IDE_RC assignRowValue (
        iduMemory    * aMemory,
        mtcColumn    * aSourceColumn,
        void         * aSourceValue,
        mtcColumn    * aDestColumn,
        void         * aDestValue,
        mtcTemplate  * aDestTemplate,
        idBool         aCopyRef );

    // PROJ-1075 rowtype에 대한 assign.
    // cursor fetch시에 사용됨.
    static IDE_RC assignRowValueFromStack(
        iduMemory    * aMemory,
        mtcStack     * aSourceStack,
        SInt           aSourceStackRemain,
        qtcNode      * aDestNode,
        qcTemplate   * aDestTemplate,
        UInt           aTargetCount );

    // PROJ-1075 arraytype에 대한 assign
    static IDE_RC assignArrayValue (
        void         * aSourceValue,
        void         * aDestValue,
        mtcTemplate  * aDestTemplate );

    // BUG-11192 date_format session property
    // mtv::executeConvert()시 mtcTemplate이 필요함
    // aDestTemplate 인자 추가.
    // by kumdory, 2005-04-08
    // PROJ-1075 assignValue->assignPrimitiveValue로 함수이름 변경.
    static IDE_RC assignPrimitiveValue (
        iduMemory    * aMemory,
        mtcColumn    * aSourceColumn,
        void         * aSourceValue,
        mtcColumn    * aDestColumn,
        void         * aDestValue,
        mtcTemplate  * aDestTemplate );

    /* PROJ-1584 DML Return Clause */
    static IDE_RC assignNonPrimitiveValue (
        iduMemory    * aMemory,
        mtcColumn    * aSourceColumn,
        void         * aSourceValue,
        mtcColumn    * aDestColumn,
        void         * aDestValue,
        mtcTemplate  * aDestTemplate,
        idBool         aCopyRef );

    static IDE_RC calculateBoolean (
        qtcNode      * aNode,
        qcTemplate   * aTemplate,
        idBool       * aIsTrue );

    static IDE_RC calculateAndAssign (
        iduMemory    * aMemory,
        qtcNode      * aSourceNode,
        qcTemplate   * aSourceTemplate,
        qtcNode      * aDestNode,
        qcTemplate   * aDestTemplate,
        idBool         aCopyRef );

    static IDE_RC getSystemDefinedException (
        SChar           * aStmtText,
        qcNamePosition  * aName,
        SInt            * aId,
        UInt            * aErrorCode);

    static IDE_RC getSystemDefinedExceptionName (
        SInt    aId,     
        SChar **aName);


    static  IDE_RC arrayReturnToInto( qcTemplate         * aTemplate,
                                      qcTemplate         * aDestTemplate,
                                      qmmReturnValue     * aReturnValue,
                                      qmmReturnIntoValue * aReturnIntoValue,
                                      vSLong               aRowCnt );

    static  IDE_RC recordReturnToInto( qcTemplate         * aTemplate,
                                       qcTemplate         * aDestTemplate,
                                       qmmReturnValue     * aReturnValue,
                                       qmmReturnIntoValue * aReturnIntoValue,
                                       vSLong               aRowCnt,
                                       idBool               aIsBulk );

    // BUG-36131
    static  IDE_RC primitiveReturnToInto( qcTemplate         * aTemplate,
                                          qcTemplate         * aDestTemplate,
                                          qmmReturnValue     * aReturnValue,
                                          qmmReturnIntoValue * aReturnIntoValue );

    // BUG-37011
    static IDE_RC truncateArray ( qcStatement * aQcStmt,
                                  qtcNode     * aQtcNode );

    // PROJ-1904 Extend UDT
    static IDE_RC allocArrayInfo( qcStatement   * aQcStmt,
                                  qsxArrayInfo ** aArrayInfo );

    static IDE_RC freeArrayInfo( qsxArrayInfo * aArrayInfo );

    static IDE_RC allocArrayMemMgr( qcStatement  * aQcStmt,
                                    qsxArrayInfo * aArrayInfo );

    static IDE_RC destroyArrayMemMgr( qcSessionSpecific * aQPSpecific );

    static IDE_RC initRecordVar( qcStatement * aQcStmt,
                                 qcTemplate  * aTemplate,
                                 qtcNode     * aNode,
                                 idBool        aCopyRef );

    static IDE_RC initRecordVar( qcStatement     * aQcStmt,
                                 const mtcColumn * aColumn,
                                 void            * aRow,
                                 idBool            aCopyRef );

    static IDE_RC finalizeRecordVar( qcTemplate * aTemplate,
                                     qtcNode    * aNode );

    static IDE_RC finalizeRecordVar( const mtcColumn * aColumn,
                                     void            * aRow );

    /* PROJ-2586 PSM Parameters and return without precision */
    static IDE_RC adjustParamAndReturnColumnInfo( iduMemory   * aQmxMem, 
                                                  mtcColumn   * aSourceColumn,
                                                  mtcColumn   * aDestColumn,
                                                  mtcTemplate * aDestTemplate );

    /* PROJ-2586 PSM Parameters and return without precision */
    static IDE_RC finalizeParamAndReturnColumnInfo( mtcColumn   * aColumn );

    // BUG-46032
    static IDE_RC preCalculateArray ( qcStatement * aQcStmt,                          
                                      qtcNode     * aQtcNode ) ;   

    // BUG-45701
    static inline UShort reverseEndian( UShort Value )
    {
        return (((Value&0xFF00)>>8)|((Value&0x00FF)<<8));
    }
    static inline UInt reverseEndian( UInt Value )
    {
        return (UInt)(((Value&ID_ULONG(0xFF000000))>>24)|((Value&ID_ULONG(0x00FF0000))>>8)|
               ((Value&ID_ULONG(0x0000FF00))<<8)|((Value&ID_ULONG(0x000000FF))<<24));
    }
    static inline SInt reverseEndian( SInt Value )
    {
        return (UInt)(((Value&ID_ULONG(0xFF000000))>>24)|((Value&ID_ULONG(0x00FF0000))>>8)|
               ((Value&ID_ULONG(0x0000FF00))<<8)|((Value&ID_ULONG(0x000000FF))<<24));
    }
    static inline ULong reverseEndian( ULong Value )
    {
        return (((Value&ID_ULONG(0xFF00000000000000))>>56)|
               ((Value&ID_ULONG(0x00FF000000000000))>>40)|
               ((Value&ID_ULONG(0x0000FF0000000000))>>24)|
               ((Value&ID_ULONG(0x000000FF00000000))>>8)|
               ((Value&ID_ULONG(0x00000000FF000000))<<8)|
               ((Value&ID_ULONG(0x0000000000FF0000))<<24)|
               ((Value&ID_ULONG(0x000000000000FF00))<<40)|
               ((Value&ID_ULONG(0x00000000000000FF))<<56));
    }
};

#endif

