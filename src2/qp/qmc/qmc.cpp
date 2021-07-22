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
 * $Id: qmc.cpp 89218 2020-11-12 01:36:24Z donovan.seo $
 *
 * Description :
 *     Execution에서 사용하는 공통 모듈로
 *     Materialized Column에 대한 처리를 하는 작업이 주를 이룬다.
 *
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idu.h>
#include <smi.h>
#include <mtcDef.h>
#include <qtc.h>
#include <qmc.h>
#include <qdbCommon.h>
#include <qmn.h>
#include <qci.h>
#include <qmv.h>
#include <qsxUtil.h>

extern mtdModule mtdBigint;
extern mtdModule mtdByte;
extern mtdModule mtdList;

mtcExecute qmc::valueExecute = {
    mtf::calculateNA,     // Aggregation 초기화 함수, 없음
    mtf::calculateNA,     // Aggregation 수행 함수, 없음
    mtf::calculateNA,
    mtf::calculateNA,     // Aggregation 종료 함수, 없음
    qmc::calculateValue,  // VALUE 연산 함수
    NULL,                 // 연산을 위한 부가 정보, 없음
    mtx::calculateNA,
    mtk::estimateRangeNA, // Key Range 크기 추출 함수, 없음
    mtk::extractRangeNA   // Key Range 생성 함수, 없음
};

IDE_RC
qmc::calculateValue( mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*,
                     mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Value의 연산을 수행한다.
 *
 * Implementation :
 *
 *    Stack에 column정보와 Value 정보를 Setting한다.
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;

    aStack->value  = (void*) mtd::valueForModule(
        (smiColumn*)aStack->column,
        aTemplate->rows[aNode->table].row,
        MTD_OFFSET_USE,
        aStack->column->module->staticNull );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::setMtrNA( qcTemplate  * /* aTemplate */,
               qmdMtrNode  * /* aNode */,
               void        * /* aRow */ )
{
    // 이 함수가 호출되면 안됨
    IDE_DASSERT(0);

    return IDE_FAILURE;
}

IDE_RC
qmc::setMtrByPointer( qcTemplate  * /* aTemplate */,
                      qmdMtrNode  * aNode,
                      void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Pointer형 Column을 Materialized Row에 저장한다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Memory Base Table의 저장
 *        - Memory Column의 저장
 *
 * Implementation :
 *    Materialized Row에서의 저장 위치를 구하고,
 *    Source Tuple의 row pointer를 그 위치에 저장한다.
 ***********************************************************************/

    SChar ** sPos = (SChar**)((SChar*)aRow + aNode->dstColumn->column.offset);

    *sPos = (SChar*) aNode->srcTuple->row;

    return IDE_SUCCESS;
}

IDE_RC
qmc::setMtrByPointerAndTupleID( qcTemplate  * /* aTemplate */,
                                qmdMtrNode  * aNode,
                                void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Pointer형 Column을 Materialized Row에 저장한다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Memory Partitioned Table의 저장
 *        - Memory Column의 저장
 *
 * Implementation :
 *    Materialized Row에서의 저장 위치를 구하고,
 *    Source Tuple의 row pointer를 그 위치에 저장한다.
 ***********************************************************************/

    qmcMemPartRowInfo   sRowInfo;
    mtdByteType       * sByte = (mtdByteType*)((SChar*)aRow + aNode->dstColumn->column.offset);

    sRowInfo.partitionTupleID = aNode->srcTuple->partitionTupleID;

    // BUG-38309
    // 메모리 파티션일때도 rid 를 저장해야 한다.
    sRowInfo.grid             = aNode->srcTuple->rid;
    sRowInfo.position         = (SChar*) aNode->srcTuple->row;

    sByte->length = ID_SIZEOF(qmcMemPartRowInfo);
    idlOS::memcpy( sByte->value, & sRowInfo, ID_SIZEOF(qmcMemPartRowInfo) );

    return IDE_SUCCESS;
}

IDE_RC
qmc::setMtrByRID( qcTemplate  * /* aTemplate */,
                  qmdMtrNode  * aNode,
                  void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    RID를 Materialized Row에 저장한다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Disk Base Table의 저장
 *
 * Implementation :
 *    Materialized Row에서의 저장 위치를 구하고,
 *    Source Tuple의 RID를 그 위치에 복사한다.
 ***********************************************************************/

    SChar * sPos = (SChar*)((SChar*)aRow + aNode->dstColumn->column.offset);

    idlOS::memcpy( sPos, & aNode->srcTuple->rid, ID_SIZEOF(scGRID) );

    return IDE_SUCCESS;
}

IDE_RC
qmc::setMtrByRIDAndTupleID( qcTemplate  * /* aTemplate */,
                            qmdMtrNode  * aNode,
                            void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    RID, tupleID를 Materialized Row에 저장한다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Disk Partitioned Table의 저장
 *
 * Implementation :
 *    Materialized Row에서의 저장 위치를 구하고,
 *    Source Tuple의 RID, partitionTupleID를 그 위치에 복사한다.
 ***********************************************************************/

    qmnCursorInfo       * sCursorInfo;
    qmcDiskPartRowInfo    sRowInfo;
    mtdByteType         * sByte = (mtdByteType*)((SChar*)aRow + aNode->dstColumn->column.offset);

    IDE_DASSERT( ( aNode->srcTuple->lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
                 == MTC_TUPLE_PARTITIONED_TABLE_TRUE );

    sCursorInfo = (qmnCursorInfo *) aNode->srcTuple->cursorInfo;

    sRowInfo.partitionTupleID = aNode->srcTuple->partitionTupleID;
    sRowInfo.grid             = aNode->srcTuple->rid;

    if ( sCursorInfo != NULL )
    {
        if ( sCursorInfo->selectedIndexTuple != NULL )
        {
            sRowInfo.indexGrid = sCursorInfo->selectedIndexTuple->rid;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    sByte->length = ID_SIZEOF(qmcDiskPartRowInfo);

    idlOS::memcpy( sByte->value, & sRowInfo, ID_SIZEOF(qmcDiskPartRowInfo) );

    return IDE_SUCCESS;
}

IDE_RC
qmc::setMtrByValue( qcTemplate  * aTemplate,
                    qmdMtrNode  * aNode,
                    void        * /* aRow */)
{
/***********************************************************************
 *
 * Description :
 *    연산으로 구성된 Column을 Materialized Row에 저장한다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - i1 + 1 의 [+]와 같이 Expression으로 구성된 Column
 *
 * Implementation :
 *    Source node룰 수행함으로서 Materialized Row로의 구성이
 *    자연스럽게 이루어진다.
 *    이는 Source Column의 수행 정보를 그대로 Destination에 구성함으로서
 *    가능하며, Source의 수행 후 Destination으로 복사하는 부하를 없애기
 *    위함이다.
 ***********************************************************************/

#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // PROJ-2179 dstNode 대신 srcNode를 수정한다.
    // 최종 결과는 여전히 dstColumn에 출력된다.
    return qtc::calculate( aNode->srcNode, aTemplate );

#undef IDE_FN
}

IDE_RC
qmc::setMtrByCopy( qcTemplate  * /*aTemplate*/,
                   qmdMtrNode  * aNode,
                   void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Source Column을 Materialized Row에 복사한다.
 *    해당 Data를 직접 저장하고 있어야 처리가 가능한 Column에 대하여
 *    Materialized Row를 구성할 때 이를 사용한다.
 *
 * Implementation :
 *    Source Column의 Data를 Destination Column의 위치에 복사한다.
 *
 ***********************************************************************/

    SChar * sSrcValue;
    SChar * sDstValue;

    sSrcValue = (SChar*) mtd::valueForModule(
        (smiColumn*)aNode->srcColumn,
        aNode->srcTuple->row,
        MTD_OFFSET_USE,
        aNode->srcColumn->module->staticNull );

    sDstValue = (SChar*) mtd::valueForModule(
        (smiColumn*)aNode->dstColumn,
        aRow,
        MTD_OFFSET_USE,
        aNode->dstColumn->module->staticNull );

    idlOS::memcpy( sDstValue,
                   sSrcValue,
                   aNode->srcColumn->module->actualSize( aNode->srcColumn,
                                                         sSrcValue ) );

    return IDE_SUCCESS;
}

IDE_RC
qmc::setMtrByConvert( qcTemplate  * aTemplate,
                      qmdMtrNode  * aNode,
                      void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Source Column의 수행 결과를 Materialized Row에 복사한다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Indirect node인 경우
 *    Source Node의 수행 결과인 Stack을 참조하여 그 결과를 복사해야
 *    하는 경우에 사용한다.
 *
 * Implementation :
 *    Source Column을 수행하고,
 *    Stack 정보를 이용하여, Destination Column에 복사한다.
 *
 ***********************************************************************/

    void * sValue;

    IDE_TEST( qtc::calculate( aNode->srcNode, aTemplate ) != IDE_SUCCESS );

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->dstColumn,
        aRow,
        MTD_OFFSET_USE,
        aNode->dstColumn->module->staticNull );

    idlOS::memcpy( (SChar*) sValue,
                   aTemplate->tmplate.stack->value,
                   aTemplate->tmplate.stack->column->module->actualSize(
                       aTemplate->tmplate.stack->column,
                       aTemplate->tmplate.stack->value ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Tuple ID로 저장 매체를 판단하여, Source Column 또는 Source Column 수행 결과를 Materialized Row에 복사한다.
 *
 * Implementation :
 ***********************************************************************/
IDE_RC qmc::setMtrByCopyOrConvert( qcTemplate  * aTemplate,
                                   qmdMtrNode  * aNode,
                                   void        * aRow )
{
    mtcColumn * sColumn = NULL;

    if ( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
         == SMI_COLUMN_TYPE_FIXED )
    {
        sColumn = &aNode->srcTuple->columns[aNode->srcNode->node.column];

        IDE_DASSERT( aNode->srcColumn->column.id == sColumn->column.id );

        aNode->srcColumn = sColumn;

        IDE_TEST( qmc::setMtrByCopy( aTemplate,
                                     aNode,
                                     aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmc::setMtrByConvert( aTemplate,
                                        aNode,
                                        aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Tuple ID로 저장 매체를 판단하여, Pointer 또는 RID를 Materialized Row에 저장한다.
 *
 * Implementation :
 ***********************************************************************/
IDE_RC qmc::setMtrByPointerOrRIDAndTupleID( qcTemplate  * aTemplate,
                                            qmdMtrNode  * aNode,
                                            void        * aRow )
{
    qmcPartRowInfo   sRowInfo;
    mtdByteType    * sByte = (mtdByteType *)((SChar *)aRow + aNode->dstColumn->column.offset);

    if ( ( aTemplate->tmplate.rows[aNode->srcTuple->partitionTupleID].lflag & MTC_TUPLE_STORAGE_MASK )
         == MTC_TUPLE_STORAGE_MEMORY )
    {
        IDE_TEST( qmc::setMtrByPointerAndTupleID( aTemplate,
                                                  aNode,
                                                  aRow )
                  != IDE_SUCCESS );

        // byte align이어서 복사해야 한다.
        idlOS::memcpy( &sRowInfo, sByte->value, ID_SIZEOF(qmcMemPartRowInfo) );

        sRowInfo.isDisk = ID_FALSE;

        idlOS::memcpy( sByte->value, & sRowInfo, ID_SIZEOF(qmcPartRowInfo) );
        sByte->length = ID_SIZEOF(qmcPartRowInfo);
    }
    else
    {
        IDE_TEST( qmc::setMtrByRIDAndTupleID( aTemplate,
                                              aNode,
                                              aRow )
                  != IDE_SUCCESS );

        // byte align이어서 복사해야 한다.
        idlOS::memcpy( &sRowInfo, sByte->value, ID_SIZEOF(qmcDiskPartRowInfo) );

        sRowInfo.isDisk = ID_TRUE;

        idlOS::memcpy( sByte->value, & sRowInfo, ID_SIZEOF(qmcPartRowInfo) );
        sByte->length = ID_SIZEOF(qmcPartRowInfo);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::setMtrByNull( qcTemplate  * /*aTemplate*/,
                   qmdMtrNode  * aNode,
                   void        * aRow )
{
/***********************************************************************
 *
 * Description : 
 *    상위에서 사용되지 않는 Materialized Node도 경우에 따라
 *    연산될 수 있어 null로 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->dstColumn,
        aRow,
        MTD_OFFSET_USE,
        aNode->dstColumn->module->staticNull );

    // BUG-41681 null로 초기화한다.
    aNode->dstColumn->module->null( aNode->dstColumn, sValue );

    return IDE_SUCCESS;
}

IDE_RC
qmc::setTupleNA( qcTemplate  * /* aTemplate */,
                 qmdMtrNode  * /* aNode */,
                 void        * /* aRow */ )
{
    // 이 함수가 호출되면 안됨.
    IDE_DASSERT(0);

    return IDE_SUCCESS;
}

IDE_RC
qmc::setTupleByPointer( qcTemplate  * /* aTemplate */,
                        qmdMtrNode  * aNode,
                        void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row에 저장된 pointer형 column을 Tuple Set에 복원시킨다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Temp Table에 저장된 Memory Base Table을 복원
 *        - Memory Temp Table에 저장도니 Memory Column을 복원
 *
 * Implementation :
 *    Materialized Row에서 저장된 위치를 구하고,
 *    그 위치에 저장된 pointer를 원래 Tuple에 복원시킨다.
 ***********************************************************************/

    SChar     ** sPos = (SChar**) ((SChar*)aRow + aNode->dstColumn->column.offset);
    mtcColumn  * sColumn;
    UInt         sTempTypeOffset;
    UInt         i;

    aNode->srcTuple->row = (void*) *sPos;

    /* PROJ-2334 PMT memory variable column */
    if ( ( ( aNode->srcTuple->lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
           == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
         &&
         ( ( ( aNode->srcTuple->columns[aNode->srcNode->node.column].
               column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE ) ||
           ( ( aNode->srcTuple->columns[aNode->srcNode->node.column].
               column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ) )
    {
        sColumn = &aNode->srcTuple->columns[aNode->srcNode->node.column];

        IDE_DASSERT( aNode->srcColumn->column.id == sColumn->column.id );

        aNode->srcColumn = sColumn;

        // 값이 아닌 Pointer가 저장되는 경우
        aNode->func.compareColumn = sColumn;
    }
    else
    {
        // Nothing to do.
    }
    
    if ( ( aNode->flag & QMC_MTR_BASETABLE_MASK ) == QMC_MTR_BASETABLE_TRUE )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        // baseTable의 원복시 TEMP_TYPE를 고려해야 한다.
        sColumn = aNode->srcTuple->columns;

        for ( i = 0; i < aNode->srcTuple->columnCount; i++, sColumn++ )
        {
            if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                 == SMI_COLUMN_TYPE_TEMP_1B )
            {
                sTempTypeOffset =
                    *(UChar*)((UChar*)aNode->srcTuple->row + sColumn->column.offset);
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_2B )
            {
                sTempTypeOffset =
                    *(UShort*)((UChar*)aNode->srcTuple->row + sColumn->column.offset);
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_4B )
            {
                sTempTypeOffset =
                    *(UInt*)((UChar*)aNode->srcTuple->row + sColumn->column.offset);
            }
            else
            {
                sTempTypeOffset = 0;
            }

            if ( sTempTypeOffset > 0 )
            {
                IDE_DASSERT( sColumn->module->actualSize(
                                 sColumn,
                                 (SChar*)aNode->srcTuple->row + sTempTypeOffset )
                             <= sColumn->column.size );

                idlOS::memcpy( (SChar*)sColumn->column.value,
                               (SChar*)aNode->srcTuple->row + sTempTypeOffset,
                               sColumn->module->actualSize(
                                   sColumn,
                                   (SChar*)aNode->srcTuple->row + sTempTypeOffset ) );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        // src가 TEMP_TYPE인 경우 원복시 고려해야 한다.
        if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_TEMP_1B )
        {
            sTempTypeOffset =
                *(UChar*)((UChar*)aNode->srcTuple->row + aNode->srcColumn->column.offset);
        }
        else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_2B )
        {
            sTempTypeOffset =
                *(UShort*)((UChar*)aNode->srcTuple->row + aNode->srcColumn->column.offset);
        }
        else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_4B )
        {
            sTempTypeOffset =
                *(UInt*)((UChar*)aNode->srcTuple->row + aNode->srcColumn->column.offset);
        }
        else
        {
            sTempTypeOffset = 0;
        }

        if ( sTempTypeOffset > 0 )
        {
            IDE_DASSERT( aNode->srcColumn->module->actualSize(
                             aNode->srcColumn,
                             (SChar*)aNode->srcTuple->row + sTempTypeOffset )
                         <= aNode->srcColumn->column.size );

            idlOS::memcpy( (SChar*)aNode->srcColumn->column.value,
                           (SChar*)aNode->srcTuple->row + sTempTypeOffset,
                           aNode->srcColumn->module->actualSize(
                               aNode->srcColumn,
                               (SChar*)aNode->srcTuple->row + sTempTypeOffset ) );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC qmc::setTupleByPointer4Rid(qcTemplate  * /* aTemplate */,
                                  qmdMtrNode  * aNode,
                                  void        * aRow)
{
    IDE_RC  sRc;
    SChar** sPos;

    sPos = (SChar**)((SChar*)aRow + aNode->dstColumn->column.offset);
    aNode->srcTuple->row = (void*)*sPos;

    sRc = smiGetGRIDFromRowPtr((void*)*sPos, &aNode->srcTuple->rid);
    IDE_TEST(sRc != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::setTupleByPointerAndTupleID( qcTemplate  * aTemplate,
                                  qmdMtrNode  * aNode,
                                  void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row에 저장된 pointer형 column을 Tuple Set에 복원시킨다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Temp Table에 저장된 Memory Base Table을 복원
 *        - Memory Temp Table에 저장도니 Memory Column을 복원
 *
 * Implementation :
 *    Materialized Row에서 저장된 위치를 구하고,
 *    그 위치에 저장된 pointer를 원래 Tuple에 복원시킨다.
 ***********************************************************************/

    qmcMemPartRowInfo   sRowInfo;
    mtdByteType       * sByte = (mtdByteType*) ((SChar*)aRow + aNode->dstColumn->column.offset);

    mtcColumn  * sColumn;
    UInt         sTempTypeOffset;
    UInt         i;

    IDE_DASSERT( ( sByte->length == ID_SIZEOF(qmcMemPartRowInfo) ) ||
                 ( sByte->length == ID_SIZEOF(qmcPartRowInfo) ) );

    // byte align이어서 복사해야한다.
    idlOS::memcpy( & sRowInfo, sByte->value, ID_SIZEOF(qmcMemPartRowInfo) );

    aNode->srcTuple->partitionTupleID = sRowInfo.partitionTupleID;
    aNode->srcTuple->row = (void*)(sRowInfo.position);

    aTemplate->tmplate.rows[sRowInfo.partitionTupleID].row =
        aNode->srcTuple->row;

    aNode->srcTuple->columns =
        aTemplate->tmplate.rows[sRowInfo.partitionTupleID].columns;

    /* PROJ-2464 hybrid partitioned table 지원 */
    aNode->srcTuple->rowOffset   = aTemplate->tmplate.rows[sRowInfo.partitionTupleID].rowOffset;
    aNode->srcTuple->rowMaximum  = aTemplate->tmplate.rows[sRowInfo.partitionTupleID].rowMaximum;
    aNode->srcTuple->tableHandle = aTemplate->tmplate.rows[sRowInfo.partitionTupleID].tableHandle;

    // BUG-38309
    // 메모리 파티션일때도 rid 를 저장해야 한다.
    aNode->srcTuple->rid = sRowInfo.grid;

    /* PROJ-2334 PMT memory variable column */
    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( ( sByte->length == ID_SIZEOF(qmcPartRowInfo) )
         ||
         ( ( ( aNode->srcTuple->lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
             == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
           &&
           ( ( ( aNode->srcTuple->columns[aNode->srcNode->node.column].
                 column.flag & SMI_COLUMN_TYPE_MASK )
               == SMI_COLUMN_TYPE_VARIABLE ) ||
             ( ( aNode->srcTuple->columns[aNode->srcNode->node.column].
                 column.flag & SMI_COLUMN_TYPE_MASK )
               == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ) ) )
    {
        sColumn = &aNode->srcTuple->columns[aNode->srcNode->node.column];

        IDE_DASSERT( aNode->srcColumn->column.id == sColumn->column.id );

        aNode->srcColumn = sColumn;

        // 값이 아닌 Pointer가 저장되는 경우
        aNode->func.compareColumn = sColumn;
    }
    else
    {
        // Nothing to do.
    }
    
    if ( ( aNode->flag & QMC_MTR_BASETABLE_MASK ) == QMC_MTR_BASETABLE_TRUE )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        // baseTable의 원복시 TEMP_TYPE를 고려해야 한다.
        sColumn = aNode->srcTuple->columns;

        for ( i = 0; i < aNode->srcTuple->columnCount; i++, sColumn++ )
        {
            if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                 == SMI_COLUMN_TYPE_TEMP_1B )
            {
                sTempTypeOffset =
                    *(UChar*)((UChar*)aNode->srcTuple->row + sColumn->column.offset);
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_2B )
            {
                sTempTypeOffset =
                    *(UShort*)((UChar*)aNode->srcTuple->row + sColumn->column.offset);
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_4B )
            {
                sTempTypeOffset =
                    *(UInt*)((UChar*)aNode->srcTuple->row + sColumn->column.offset);
            }
            else
            {
                sTempTypeOffset = 0;
            }

            if ( sTempTypeOffset > 0 )
            {
                IDE_DASSERT( sColumn->module->actualSize(
                                 sColumn,
                                 (SChar*)aNode->srcTuple->row + sTempTypeOffset )
                             <= sColumn->column.size );

                idlOS::memcpy( (SChar*)sColumn->column.value,
                               (SChar*)aNode->srcTuple->row + sTempTypeOffset,
                               sColumn->module->actualSize(
                                   sColumn,
                                   (SChar*)aNode->srcTuple->row + sTempTypeOffset ) );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        // src가 TEMP_TYPE인 경우 원복시 고려해야 한다.
        if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_TEMP_1B )
        {
            sTempTypeOffset =
                *(UChar*)((UChar*)aNode->srcTuple->row + aNode->srcColumn->column.offset);
        }
        else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_2B )
        {
            sTempTypeOffset =
                *(UShort*)((UChar*)aNode->srcTuple->row + aNode->srcColumn->column.offset);
        }
        else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_4B )
        {
            sTempTypeOffset =
                *(UInt*)((UChar*)aNode->srcTuple->row + aNode->srcColumn->column.offset);
        }
        else
        {
            sTempTypeOffset = 0;
        }

        if ( sTempTypeOffset > 0 )
        {
            IDE_DASSERT( aNode->srcColumn->module->actualSize(
                             aNode->srcColumn,
                             (SChar*)aNode->srcTuple->row + sTempTypeOffset )
                         <= aNode->srcColumn->column.size );

            idlOS::memcpy( (SChar*)aNode->srcColumn->column.value,
                           (SChar*)aNode->srcTuple->row + sTempTypeOffset,
                           aNode->srcColumn->module->actualSize(
                               aNode->srcColumn,
                               (SChar*)aNode->srcTuple->row + sTempTypeOffset ) );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmc::setTupleByRID( qcTemplate  * aTemplate,
                    qmdMtrNode  * aNode,
                    void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row에 저장된 RID column을 Tuple Set에 복원시킨다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Temp Table에 저장된 Disk Base Table을 복원
 *
 * Implementation :
 *    Materialized Row에서 저장된 위치를 구하고,
 *    RID와 RID에 해당하는 Record를 복원시킨다.
 ***********************************************************************/

    smiStatement * sStmt;
    IDE_RC         sRc;
    mtcColumn    * sColumn;
    void         * sValueTemp;
    UInt           i;

    scGRID * sRID = (scGRID*) ((SChar*)aRow + aNode->dstColumn->column.offset);

    //--------------------------------
    // RID 복원
    //--------------------------------

    idlOS::memcpy( & aNode->srcTuple->rid, sRID, ID_SIZEOF(scGRID) );

    //--------------------------------
    // RID로부터 Record 복원
    //--------------------------------

    // BUG-24330
    // outer-join 등에서 생성된 null rid인 경우 직접 null row를 생성한다.
    if ( SMI_GRID_IS_VIRTUAL_NULL( aNode->srcTuple->rid ) )
    {
        for ( i = 0, sColumn = aNode->srcTuple->columns;
              i < aNode->srcTuple->columnCount;
              i++, sColumn++ )
        {
            // BUG-39238
            // Set NULL OID for compressed column.
            if ( ( ((smiColumn*)sColumn)->flag & SMI_COLUMN_COMPRESSION_MASK )
                 == SMI_COLUMN_COMPRESSION_TRUE )
            {
                *((smOID*)((SChar*)aNode->srcTuple->row + ((smiColumn*)sColumn)->offset)) = SMI_NULL_OID;
            }
            else
            {
                // NULL Padding
                sValueTemp = (void*) mtd::valueForModule(
                    (smiColumn*)sColumn,
                    aNode->srcTuple->row,
                    MTD_OFFSET_USE,
                    sColumn->module->staticNull );

                sColumn->module->null( sColumn,
                                       sValueTemp );
            }
        }
    }
    else
    {
        sStmt = QC_SMI_STMT( aTemplate->stmt );

        // PROJ-1705
        // rid로 레코드를 패치하는 경우,
        // disk table과 disk temp table을 구분하기 위한 플래그정보.
        // PROJ-1705 프로젝트는 디스크테이블에 대해서만 적용되기때문에
        // 디스크테이블과 디스크템프테이블의 rid가 다르다.
        // 이 rid를 qp는 GRID라는 구조체로
        // 디스크테이블과 디스크템프테이블 구분없이 사용하기로 하고,
        // rid로 레코드패치시 ( smiFetchRowFromGRID() )
        // sm에 디스크테이블과 디스크템프테이블에 맞는 rid로
        // 구분해서 내려주기로 함.

        // BUG-37277
        IDE_TEST_RAISE( SC_GRID_IS_NULL( aNode->srcTuple->rid ),
                        ERR_NULL_GRID );

        if( ( aNode->flag & QMC_MTR_BASETABLE_TYPE_MASK )
            == QMC_MTR_BASETABLE_TYPE_DISKTABLE )
        {
            sRc = smiFetchRowFromGRID( aTemplate->stmt->mStatistics,
                                       sStmt,
                                       SMI_TABLE_DISK,
                                       aNode->srcTuple->rid,
                                       aNode->fetchColumnList,
                                       aNode->srcTuple->tableHandle,
                                       aNode->srcTuple->row );
        }
        else
        {
            /* PROJ-2201 Innovation in sorting and hashing(temp) */
            sRc = smiTempTable::fetchFromGRID(
                aNode->srcTuple->tableHandle,
                aNode->srcTuple->rid,
                aNode->srcTuple->row );
        }

        IDE_TEST( sRc != IDE_SUCCESS );

        // 원하는 Data가 없으면 안됨
        IDE_DASSERT( aNode->srcTuple->row != NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_GRID )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmc::setTupleByRID",
                                  "null grid" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::setTupleByRIDAndTupleID( qcTemplate  * aTemplate,
                              qmdMtrNode  * aNode,
                              void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row에 저장된 RID column을 Tuple Set에 복원시킨다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Temp Table에 저장된 Disk Base Table을 복원
 *
 * Implementation :
 *    Materialized Row에서 저장된 위치를 구하고,
 *    RID와 RID에 해당하는 Record를 복원시킨다.
 ***********************************************************************/

    smiStatement * sStmt;
    mtcTuple     * sPartTuple;
    mtcColumn    * sColumn;
    void         * sValueTemp;
    UInt           i;

    qmcDiskPartRowInfo   sRowInfo;
    mtdByteType        * sByte       = (mtdByteType*) ((SChar*)aRow + aNode->dstColumn->column.offset);
    qmnCursorInfo      * sCursorInfo = (qmnCursorInfo *) aNode->srcTuple->cursorInfo;

    //--------------------------------
    // RID 복원
    //--------------------------------

    IDE_DASSERT( ( sByte->length == ID_SIZEOF(qmcDiskPartRowInfo) ) ||
                 ( sByte->length == ID_SIZEOF(qmcPartRowInfo) ) );

    // byte align이어서 복사해야한다.
    idlOS::memcpy( & sRowInfo, sByte->value, ID_SIZEOF(qmcDiskPartRowInfo) );

    // BUG-37507 partitionTupleID 가 ID_USHORT_MAX 일때는 외부참조일때이다.
    // 불필요한 mtr node 이다.
    IDE_TEST_CONT( sRowInfo.partitionTupleID == ID_USHORT_MAX, skip );

    sPartTuple = & aTemplate->tmplate.rows[sRowInfo.partitionTupleID];

    aNode->srcTuple->rid = sRowInfo.grid;
    aNode->srcTuple->partitionTupleID = sRowInfo.partitionTupleID;

    if ( sCursorInfo != NULL )
    {
        if ( sCursorInfo->selectedIndexTuple != NULL )
        {
            sCursorInfo->selectedIndexTuple->rid = sRowInfo.indexGrid;
        }
    }

    aNode->srcTuple->columns = sPartTuple->columns;

    /* PROJ-2464 hybrid partitioned table 지원 */
    aNode->srcTuple->rowOffset   = sPartTuple->rowOffset;
    aNode->srcTuple->rowMaximum  = sPartTuple->rowMaximum;
    aNode->srcTuple->tableHandle = sPartTuple->tableHandle;

    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( sByte->length == ID_SIZEOF(qmcPartRowInfo) )
    {
        sColumn = &aNode->srcTuple->columns[aNode->srcNode->node.column];

        IDE_DASSERT( aNode->srcColumn->column.id == sColumn->column.id );

        aNode->srcColumn = sColumn;

        // 값이 저장되지 않는 경우
        aNode->func.compareColumn = sColumn;
    }
    else
    {
        /* Nothing to do */
    }

    //--------------------------------
    // RID로부터 Record 복원
    //--------------------------------

    // BUG-24330
    // outer-join 등에서 생성된 null rid인 경우 직접 null row를 생성한다.
    if ( SMI_GRID_IS_VIRTUAL_NULL( aNode->srcTuple->rid ) )
    {
        // 원하는 Data가 없으면 안됨
        IDE_DASSERT( sPartTuple->row != NULL );

        for ( i = 0, sColumn = sPartTuple->columns;
              i < sPartTuple->columnCount;
              i++, sColumn++ )
        {
            // BUG-39238
            // Set NULL OID for compressed column.
            // BUG-42417 Partitioned Table은 Compressed Column을 지원하지 않는다.

            // NULL Padding
            sValueTemp = (void*) mtd::valueForModule(
                (smiColumn*)sColumn,
                sPartTuple->row,
                MTD_OFFSET_USE,
                sColumn->module->staticNull );

            sColumn->module->null( sColumn,
                                   sValueTemp );
        }

        sPartTuple->rid = aNode->srcTuple->rid;

        aNode->srcTuple->row = sPartTuple->row;
    }
    else
    {
        sStmt = QC_SMI_STMT( aTemplate->stmt );

        //-------------------------------------------------
        // PROJ-1705
        // QMC_MTR_TYPE_DISK_TABLE 인 경우
        // rid로 레코드 패치시 smiColumnList가 필요함.
        // 여기서 생성된 smiColumnList는 smiFetchRowFromGRID()함수의 인자로 쓰임.
        // (디스크템프테이블은 해당되지 않는다.)
        //-------------------------------------------------

        /* PROJ-2464 hybrid partitioned table 지원
         *  Hybrid인 경우에는 Partitioned의 Type이 Disk가 아닐 수 있다.
         *  따라서, qmc::setMtrNode()에서 할당한 fetchColumnList를 Disk에 맞게 조정한다.
         */
        if ( aNode->fetchColumnList != NULL )
        {
            if ( ( aNode->fetchColumnList->column->flag & SMI_COLUMN_STORAGE_MASK ) !=
                 ( sPartTuple->columns->column.flag & SMI_COLUMN_STORAGE_MASK ) )
            {
                IDE_TEST( qdbCommon::makeFetchColumnList4TupleID(
                              aTemplate,
                              sRowInfo.partitionTupleID,
                              ID_FALSE,  // aIsNeedAllFetchColumn
                              NULL,      // index 정보
                              ID_FALSE,  // aIsAllocSmiColumnList
                              & aNode->fetchColumnList )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        // BUG-37277
        IDE_TEST_RAISE( SC_GRID_IS_NULL( aNode->srcTuple->rid ),
                        ERR_NULL_GRID );

        IDE_TEST( smiFetchRowFromGRID( aTemplate->stmt->mStatistics,
                                       sStmt,
                                       SMI_TABLE_DISK,
                                       aNode->srcTuple->rid,
                                       aNode->fetchColumnList,
                                       aNode->srcTuple->tableHandle,
                                       sPartTuple->row )
                  != IDE_SUCCESS );

        // 원하는 Data가 없으면 안됨
        IDE_DASSERT( sPartTuple->row != NULL );

        sPartTuple->rid = aNode->srcTuple->rid;

        aNode->srcTuple->row = sPartTuple->row;
    }

    IDE_EXCEPTION_CONT( skip );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_GRID )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmc::setTupleByRIDAndTupleID",
                                  "null grid" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmc::setTupleByValue( qcTemplate  * /* aTemplate */,
                      qmdMtrNode  * aNode,
                      void        * aRow )
{
/***********************************************************************
 *
 * Description :
 *    처리 절차상 원복시키지 않아도 되는 Column으로 아무 일도 하지 않는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcStack * sStack;
    UChar    * sValue;
    UInt       sTempTypeOffset;
    SInt       i;

    if ( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
         == SMI_COLUMN_TYPE_TEMP_1B )
    {
        sTempTypeOffset = *(UChar*)((UChar*)aRow + aNode->dstColumn->column.offset);
    }
    else if ( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
              == SMI_COLUMN_TYPE_TEMP_2B )
    {
        sTempTypeOffset = *(UShort*)((UChar*)aRow + aNode->dstColumn->column.offset);
    }
    else if ( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
              == SMI_COLUMN_TYPE_TEMP_4B )
    {
        sTempTypeOffset = *(UInt*)((UChar*)aRow + aNode->dstColumn->column.offset);
    }
    else
    {
        sTempTypeOffset = 0;
    }

    if ( sTempTypeOffset > 0 )
    {
        IDE_DASSERT( aNode->dstColumn->module->actualSize(
                         aNode->dstColumn,
                         (SChar*)aRow + sTempTypeOffset )
                     <= aNode->dstColumn->column.size );

        idlOS::memcpy( (SChar*)aNode->dstColumn->column.value,
                       (SChar*)aRow + sTempTypeOffset,
                       aNode->dstColumn->module->actualSize(
                           aNode->dstColumn,
                           (SChar*)aRow + sTempTypeOffset ) );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-39552 list의 value pointer 재조정
    if ( ( aNode->dstColumn->module == &mtdList ) &&
         ( aNode->dstColumn->precision > 0 ) &&
         ( aNode->dstColumn->scale > 0 ) )
    {
        IDE_DASSERT( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_FIXED );
        IDE_DASSERT( ID_SIZEOF(mtcStack) * aNode->dstColumn->precision +
                     aNode->dstColumn->scale
                     == aNode->dstColumn->column.size );

        sStack = (mtcStack*)((UChar*)aRow + aNode->dstColumn->column.offset);
        sValue = (UChar*)(sStack + aNode->dstColumn->precision);

        for ( i = 0; i < aNode->dstColumn->precision; i++, sStack++ )
        {
            sStack->value = (void*)sValue;
            /* BUG-48273 */
            sValue += idlOS::align( sStack->column->column.size, sStack->column->module->align );
        }
        
        //IDE_DASSERT( (UChar*)aRow + aNode->dstColumn->column.offset +
        //             aNode->dstColumn->column.size
        //             == sValue );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description :
 *    Tuple ID로 저장 매체를 판단하여, Materialized Row에 저장된 Pointer 또는 RID를 Tuple Set에 복원한다.
 *
 * Implementation :
 ***********************************************************************/
IDE_RC qmc::setTupleByPointerOrRIDAndTupleID( qcTemplate  * aTemplate,
                                              qmdMtrNode  * aNode,
                                              void        * aRow )
{
    qmcPartRowInfo   sRowInfo;
    mtdByteType    * sByte = (mtdByteType *)((SChar *)aRow + aNode->dstColumn->column.offset);

    IDE_DASSERT( sByte->length == ID_SIZEOF(qmcPartRowInfo) );

    // byte align이어서 복사해야 한다.
    idlOS::memcpy( &sRowInfo, sByte->value, ID_SIZEOF(qmcPartRowInfo) );

    if ( sRowInfo.isDisk != ID_TRUE )
    {
        IDE_TEST( qmc::setTupleByPointerAndTupleID( aTemplate,
                                                    aNode,
                                                    aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmc::setTupleByRIDAndTupleID( aTemplate,
                                                aNode,
                                                aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::setTupleByNull( qcTemplate  * /* aTemplate */,
                     qmdMtrNode  * /* aNode */,
                     void        * /* aRow */ )
{
/***********************************************************************
 *
 * Description :
 *    상위에서 사용되지 않는 Materialized Node에 대해
 *    아무것도 하지 않는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    return IDE_SUCCESS;
}

UInt
qmc::getHashNA( UInt         /* aValue */,
                qmdMtrNode * /* aNode */,
                void       * /* aRow */ )
{
    // 이 함수가 호출되면 안됨.

    IDE_DASSERT(0);

    return 0;
}

UInt
qmc::getHashByPointer( UInt         aValue,
                       qmdMtrNode * aNode,
                       void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Column의 Hash값을 얻는다.
 *
 * Implementation :
 *    Pointer가 저장된 경우로 실제 Record Pointer를 획득하고,
 *    Source Column정보를 이용하여 Hash값을 얻는다.
 *
 *    PROJ-2334 PMT memory partitioned table or variable column인 경우
 *    partition table의 column정보를 이용 하여 Hash값을 얻는다.
 ***********************************************************************/

    void * sRow = qmc::getRowByPointer( aNode, aRow );
    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->srcColumn,
        sRow,
        MTD_OFFSET_USE,
        aNode->srcColumn->module->staticNull );

    return aNode->srcColumn->module->hash( aValue,
                                           aNode->srcColumn,
                                           sValue );
}

UInt
qmc::getHashByPointerAndTupleID( UInt         aValue,
                                 qmdMtrNode * aNode,
                                 void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Column의 Hash값을 얻는다.
 *
 * Implementation :
 *    Pointer가 저장된 경우로 실제 Record Pointer를 획득하고,
 *    Source Column정보를 이용하여 Hash값을 얻는다.
 *
 *    PROJ-2334 PMT memory partitioned table or variable column인 경우
 *    partition table의 column정보를 이용 하여 Hash값을 얻는다.
 ***********************************************************************/

    void * sRow = qmc::getRowByPointerAndTupleID( aNode, aRow );
    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->srcColumn,
        sRow,
        MTD_OFFSET_USE,
        aNode->srcColumn->module->staticNull );

    return aNode->srcColumn->module->hash( aValue,
                                           aNode->srcColumn,
                                           sValue );
}

UInt
qmc::getHashByValue( UInt         aValue,
                     qmdMtrNode * aNode,
                     void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Column의 Hash값을 얻는다.
 *
 * Implementation :
 *    Value값 자체가 저장된 경우로,
 *    Destination Column정보를 이용하여 Hash값을 얻는다.
 *
 ***********************************************************************/

    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->dstColumn,
        aRow,
        MTD_OFFSET_USE,
        aNode->dstColumn->module->staticNull );

    return aNode->dstColumn->module->hash( aValue,
                                           aNode->dstColumn,
                                           sValue );
}

idBool
qmc::isNullNA( qmdMtrNode * /* aNode */,
               void       * /* aRow */ )
{
    // 이 함수가 호출되면 안됨.

    IDE_DASSERT(0);

    return ID_FALSE;
}

idBool
qmc::isNullByPointer( qmdMtrNode * aNode,
                      void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Row에 저장된 pointer 정보로 hash 값을 얻는다.
 *
 * Implementation :
 *    Row의 pointer값을 얻는다.
 *    Source Node의 정보와 Row로부터 hash값을 얻는다.
 *
 ***********************************************************************/

    void * sRow = qmc::getRowByPointer( aNode, aRow );
    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->srcColumn,
        sRow,
        MTD_OFFSET_USE,
        aNode->srcColumn->module->staticNull );

    return aNode->srcColumn->module->isNull( aNode->srcColumn,
                                             sValue );
}

idBool
qmc::isNullByPointerAndTupleID(  qmdMtrNode * aNode,
                                 void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Row에 저장된 pointer 정보로 hash 값을 얻는다.
 *
 * Implementation :
 *    Row의 pointer값을 얻는다.
 *    Source Node의 정보와 Row로부터 hash값을 얻는다.
 *
 ***********************************************************************/

    void * sRow = qmc::getRowByPointerAndTupleID( aNode, aRow );
    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->srcColumn,
        sRow,
        MTD_OFFSET_USE,
        aNode->srcColumn->module->staticNull );

    return aNode->srcColumn->module->isNull( aNode->srcColumn,
                                             sValue );
}

idBool
qmc::isNullByValue( qmdMtrNode * aNode,
                    void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Row에 저장된 Value정보로 NULL 여부를 판단한다.
 *
 * Implementation :
 *    Destine Node의 정보와 현재 Row로부터 hash값을 얻는다.
 *
 ***********************************************************************/

    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->dstColumn,
        aRow,
        MTD_OFFSET_USE,
        aNode->dstColumn->module->staticNull );

    return aNode->dstColumn->module->isNull( aNode->dstColumn,
                                             sValue );
}

void
qmc::makeNullNA( qmdMtrNode * /* aNode */,
                 void       * /* aRow */ )
{
    // 이 함수가 호출되면 안됨.

    IDE_DASSERT(0);
}

void
qmc::makeNullNothing(  qmdMtrNode * /* aNode */,
                       void       * /* aRow */)
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Column의 NULL Value를 생성하지 않는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmc::makeNullNothing"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return;

#undef IDE_FN
}

void
qmc::makeNullValue( qmdMtrNode * aNode,
                    void       * aRow )
{
/***********************************************************************
 *
 * Description :
 *    해당 Materialized Column의 NULL Value를생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void * sValue;

    sValue = (void*) mtd::valueForModule(
        (smiColumn*)aNode->dstColumn,
        aRow,
        MTD_OFFSET_USE,
        aNode->dstColumn->module->staticNull );

    // To Fix PR-8005
    aNode->dstColumn->module->null( aNode->dstColumn,
                                    (void*) sValue );
}

void *
qmc::getRowNA ( qmdMtrNode * /* aNode */,
                const void * /* aRow  */ )
{
    // 이 함수가 호출되면 안됨
    IDE_DASSERT( 0 );

    return NULL;
}

void *
qmc::getRowByPointer ( qmdMtrNode * aNode,
                       const void * aRow )
{
/***********************************************************************
 *
 * Description :
 *
 *    Materialized Row에 저장된 Row Pointer값을 Return한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar ** sPos = (SChar**)( (SChar*) aRow + aNode->dstColumn->column.offset);
    UInt     sTempTypeOffset;
    void   * sRow;
    mtcColumn * sColumn;

    sRow = (void*) *sPos;

    /* PROJ-2334 PMT memory variable column */
    if ( ( ( aNode->srcTuple->lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
           == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
         &&
         ( ( ( aNode->srcTuple->columns[aNode->srcNode->node.column].
               column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE ) ||
           ( ( aNode->srcTuple->columns[aNode->srcNode->node.column].
               column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ) )
    {
        sColumn = &aNode->srcTuple->columns[aNode->srcNode->node.column];

        IDE_DASSERT( aNode->srcColumn->column.id == sColumn->column.id );

        aNode->srcColumn = sColumn;
        
        // 값이 아닌 Pointer가 저장되는 경우
        aNode->func.compareColumn = sColumn;
    }
    else
    {
        // Nothing to do.
    }
    
    if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
         == SMI_COLUMN_TYPE_TEMP_1B )
    {
        sTempTypeOffset = *(UChar*)((UChar*)sRow + aNode->srcColumn->column.offset);
    }
    else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
              == SMI_COLUMN_TYPE_TEMP_2B )
    {
        sTempTypeOffset = *(UShort*)((UChar*)sRow + aNode->srcColumn->column.offset);
    }
    else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
              == SMI_COLUMN_TYPE_TEMP_4B )
    {
        sTempTypeOffset = *(UInt*)((UChar*)sRow + aNode->srcColumn->column.offset);
    }
    else
    {
        sTempTypeOffset = 0;
    }

    if ( sTempTypeOffset > 0 )
    {
        sRow = (void*)((SChar*)sRow + sTempTypeOffset);

        // TEMP_TYPE인 경우 OFFSET_USELESS로 비교한다.
        // 애초에 setCompareFunction에서 compare함수를 모두 결정하는 것이 맞지만
        // 최초 parent의 mtrNode는 TEMP_TYPE가 아니었으나, child의 mtrNode에 의해
        // TEMP_TYPE로 변경될 수 있어 실행시간에도 판단해서 변경한다.
        if ( ( aNode->flag & QMC_MTR_SORT_ORDER_MASK )
             == QMC_MTR_SORT_ASCENDING )
        {
            if ( aNode->func.compare !=
                 aNode->srcColumn->module->logicalCompare[MTD_COMPARE_ASCENDING] )
            {
                aNode->func.compare =
                    aNode->srcColumn->module->logicalCompare[MTD_COMPARE_ASCENDING];
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( aNode->func.compare !=
                 aNode->srcColumn->module->logicalCompare[MTD_COMPARE_DESCENDING] )
            {
                aNode->func.compare =
                    aNode->srcColumn->module->logicalCompare[MTD_COMPARE_DESCENDING];
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return sRow;
}

void *
qmc::getRowByPointerAndTupleID( qmdMtrNode * aNode,
                                const void * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row에 저장된 pointer형 column을 Tuple Set에 복원시킨다.
 *    다음과 같은 용도를 위해 사용된다.
 *        - Temp Table에 저장된 Memory Base Table을 복원
 *        - Memory Temp Table에 저장도니 Memory Column을 복원
 *
 * Implementation :
 *    Materialized Row에서 저장된 위치를 구하고,
 *    그 위치에 저장된 pointer를 원래 Tuple에 복원시킨다.
 ***********************************************************************/

    qmcMemPartRowInfo   sRowInfo;
    mtdByteType       * sByte = (mtdByteType*)((SChar*)aRow + aNode->dstColumn->column.offset);

    mtcColumn  * sColumn;
    UInt         sTempTypeOffset;
    UShort       sPartitionTupleID;
    void       * sRow;
    UInt         i;

    IDE_DASSERT( ( sByte->length == ID_SIZEOF(qmcMemPartRowInfo) ) ||
                 ( sByte->length == ID_SIZEOF(qmcPartRowInfo) ) );

    // byte align이어서 복사해야한다.
    idlOS::memcpy( & sRowInfo, sByte->value, ID_SIZEOF(qmcMemPartRowInfo) );

    sPartitionTupleID = sRowInfo.partitionTupleID;
    sRow = (void*)(sRowInfo.position);

    /* PROJ-2334 PMT memory variable column */
    if ( ( ( aNode->srcTuple->lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
           == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
         &&
         ( ( ( aNode->tmplate->rows[sPartitionTupleID].columns[aNode->srcNode->node.column].
               column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE ) ||
           ( ( aNode->tmplate->rows[sPartitionTupleID].columns[aNode->srcNode->node.column].
               column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ) )
    {
        sColumn = &aNode->tmplate->rows[sPartitionTupleID].columns[aNode->srcNode->node.column];

        IDE_DASSERT( aNode->srcColumn->column.id == sColumn->column.id );

        aNode->srcColumn = sColumn;

        // 값이 아닌 Pointer가 저장되는 경우
        aNode->func.compareColumn = sColumn;
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-2362 memory temp 저장 효율성 개선
    // baseTable의 원복시 TEMP_TYPE를 고려해야 한다.
    sColumn = aNode->tmplate->rows[sPartitionTupleID].columns;
    
    if ( ( aNode->flag & QMC_MTR_BASETABLE_MASK ) == QMC_MTR_BASETABLE_TRUE )
    {
        for ( i = 0; i < aNode->tmplate->rows[sPartitionTupleID].columnCount; i++, sColumn++ )
        {
            if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                 == SMI_COLUMN_TYPE_TEMP_1B )
            {
                sTempTypeOffset = *(UChar*)((UChar*)aRow + sColumn->column.offset);
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_2B )
            {
                sTempTypeOffset = *(UShort*)((UChar*)aRow + sColumn->column.offset);
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_4B )
            {
                sTempTypeOffset = *(UInt*)((UChar*)aRow + sColumn->column.offset);
            }
            else
            {
                sTempTypeOffset = 0;
            }

            if ( sTempTypeOffset > 0 )
            {
                IDE_DASSERT( sColumn->module->actualSize( sColumn,
                                                          (SChar*)sRow + sTempTypeOffset )
                             <= sColumn->column.size );

                idlOS::memcpy( (SChar*)sColumn->column.value,
                               (SChar*)sRow + sTempTypeOffset,
                               sColumn->module->actualSize( sColumn,
                                                            (SChar*)sRow + sTempTypeOffset ) );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        // src가 TEMP_TYPE인 경우 원복시 고려해야 한다.
        if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_TEMP_1B )
        {
            sTempTypeOffset = *(UChar*)((UChar*)sRow + aNode->srcColumn->column.offset);
        }
        else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_2B )
        {
            sTempTypeOffset = *(UShort*)((UChar*)sRow + aNode->srcColumn->column.offset);
        }
        else if ( ( aNode->srcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_4B )
        {
            sTempTypeOffset = *(UInt*)((UChar*)sRow + aNode->srcColumn->column.offset);
        }
        else
        {
            sTempTypeOffset = 0;
        }

        if ( sTempTypeOffset > 0 )
        {
            IDE_DASSERT( aNode->srcColumn->module->actualSize(
                             aNode->srcColumn,
                             (SChar*)sRow + sTempTypeOffset )
                         <= aNode->srcColumn->column.size );

            idlOS::memcpy( (SChar*)aNode->srcColumn->column.value,
                           (SChar*)sRow + sTempTypeOffset,
                           aNode->srcColumn->module->actualSize(
                               aNode->srcColumn,
                               (SChar*)sRow + sTempTypeOffset ) );
        }
        else
        {
            // Nothing to do.
        }
    }

    return sRow;
}

void *
qmc::getRowByValue ( qmdMtrNode * aNode,
                     const void * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row에 VALUE 자체가 저장되어 있으므로,
 *    현재 Row 자체를 Return한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt   sTempTypeOffset;
    void * sRow = (void*)aRow;

    if ( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
         == SMI_COLUMN_TYPE_TEMP_1B )
    {
        sTempTypeOffset = *(UChar*)((UChar*)aRow + aNode->dstColumn->column.offset);
    }
    else if ( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
              == SMI_COLUMN_TYPE_TEMP_2B )
    {
        sTempTypeOffset = *(UShort*)((UChar*)aRow + aNode->dstColumn->column.offset);
    }
    else if ( ( aNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK )
              == SMI_COLUMN_TYPE_TEMP_4B )
    {
        sTempTypeOffset = *(UInt*)((UChar*)aRow + aNode->dstColumn->column.offset);
    }
    else
    {
        sTempTypeOffset = 0;
    }

    if ( sTempTypeOffset > 0 )
    {
        sRow = (void*)((SChar*)aRow + sTempTypeOffset);

        // TEMP_TYPE인 경우 OFFSET_USELESS로 비교한다.
        // 애초에 setCompareFunction에서 compare함수를 모두 결정하는 것이 맞지만
        // 최초 parent의 mtrNode는 TEMP_TYPE가 아니었으나, child의 mtrNode에 의해
        // TEMP_TYPE로 변경될 수 있어 실행시간에도 판단해서 변경한다.
        if ( ( aNode->flag & QMC_MTR_SORT_ORDER_MASK )
             == QMC_MTR_SORT_ASCENDING )
        {
            if ( aNode->func.compare !=
                 aNode->dstColumn->module->logicalCompare[MTD_COMPARE_ASCENDING] )
            {
                aNode->func.compare =
                    aNode->dstColumn->module->logicalCompare[MTD_COMPARE_ASCENDING];
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( aNode->func.compare !=
                 aNode->dstColumn->module->logicalCompare[MTD_COMPARE_DESCENDING] )
            {
                aNode->func.compare =
                    aNode->dstColumn->module->logicalCompare[MTD_COMPARE_DESCENDING];
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return sRow;
}

IDE_RC
qmc::linkMtrNode( const qmcMtrNode * aCodeNode,
                  qmdMtrNode       * aDataNode )
{
/***********************************************************************
 *
 * Description :
 *    Data 영역의 Materialized Node를 연결한다.
 *
 * Implementation :
 *    Data 영역의 연결시 Code 영역의 Materialize Node 개수만큼 반복한다.
 *
 ***********************************************************************/

    const qmcMtrNode * sCNode;
    qmdMtrNode       * sDNode;

    for ( sCNode = aCodeNode, sDNode = aDataNode;
          sCNode != NULL;
          sCNode = sCNode->next, sDNode = sDNode->next )
    {
        IDE_ASSERT( sDNode != NULL );

        sDNode->myNode = (qmcMtrNode*)sCNode;
        sDNode->srcNode = NULL;
        sDNode->next = sDNode + 1;
        if ( sCNode->next == NULL )
        {
            sDNode->next = NULL;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmc::initMtrNode( qcTemplate * aTemplate,
                  qmdMtrNode * aNode,
                  UShort       aBaseTableCount )
{
/***********************************************************************
 *
 * Description :
 *    Code Materialized Node(aNode->myNode) 정보로부터
 *    Data Materialized Node(aNode)를 구성한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    qmdMtrNode * sNode;
    UShort       sTableCount = 0;
    idBool       sBaseTable;

    for ( sNode = aNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        if ( sTableCount < aBaseTableCount )
        {
            sBaseTable = ID_TRUE;
        }
        else
        {
            sBaseTable = ID_FALSE;
        }
        sTableCount++;

        IDE_TEST( qmc::setMtrNode( aTemplate,
                                   sNode,
                                   sNode->myNode,
                                   sBaseTable ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::refineOffsets( qmdMtrNode * aNode, UInt aStartOffset )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row를 구성하는 Column들의 offset을 재조정한다.
 *
 * Implementation :
 *    Materialized Row는 다음과 같이 구성된다.
 *       - Record Header의 크기 : aStartOffset
 *    각 Column의 종류와 Memory/Disk Temp Table의 여부에 따라,
 *    그 Offset과 Size의 구성이 달라진다.
 ***********************************************************************/

    qmdMtrNode * sNode;
    qmdMtrNode * sTmpNode;
    UInt         sOffset;
    UInt         sColumnCnt;
    UInt         sTempTypeColCount = 0;
    UInt         sTempTypeColSize;
    UInt         sArgument;
    SInt         sPrecision;
    SInt         sScale;
    UInt         i;

    //--------------------------------------------------
    // 구성된 Materialzied Column들을 순회하며,
    // offset과 size등을 재조정한다.
    // 필요할 경우, Fixed->Variable로 변경하는 작업도
    // 진행한다.
    //--------------------------------------------------

    sOffset = aStartOffset;

    for ( sNode = aNode; sNode != NULL; sNode = sNode->next )
    {
        // BUG-46678
        // Precision을 명시하지 않은 return column의 size가 원복되지 않는 경우가 있습니다.
        if ( (sNode->dstColumn->flag & MTC_COLUMN_SP_ADJUST_PRECISION_MASK) == MTC_COLUMN_SP_ADJUST_PRECISION_TRUE )
        {
            IDE_TEST( qsxUtil::finalizeParamAndReturnColumnInfo( sNode->dstColumn )
                      != IDE_SUCCESS );
        }

        if ( (sNode->srcColumn->flag & MTC_COLUMN_SP_ADJUST_PRECISION_MASK) == MTC_COLUMN_SP_ADJUST_PRECISION_TRUE )
        {
            IDE_TEST( qsxUtil::finalizeParamAndReturnColumnInfo( sNode->srcColumn )
                      != IDE_SUCCESS );
        }

        switch ( sNode->flag & QMC_MTR_TYPE_MASK )
        {
            //-----------------------------------------------------------
            // Base Table에 대한 처리 방식
            //-----------------------------------------------------------

            case QMC_MTR_TYPE_MEMORY_TABLE :
            {
                //-----------------------------------
                // Memory Base Table의 경우
                //-----------------------------------
                // Disk Temp Table에서의 처리를 위해 Column을
                // BIGINT Type으로 지정( 32/64bit pointer의 저장,
                // RID와 동일한 크기 저장). RID의 크기가 BIGINT크기를 초과할
                // 경우, 이에 대한 수정이 이루어져야 한다.
                IDE_DASSERT( ID_SIZEOF(scGRID) <= ID_SIZEOF(mtdBigintType) );

                IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                 & mtdBigint,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );

                // fix BUG-10243
                sOffset = idlOS::align( sOffset,
                                        sNode->dstColumn->module->align );

                sNode->dstColumn->column.offset = sOffset;

                sOffset += sNode->dstColumn->column.size;
                break;
            }
            case QMC_MTR_TYPE_DISK_TABLE :
            {
                //-----------------------------------
                // Disk Base Table의 경우
                //-----------------------------------
                // Disk Temp Table에서의 처리를 위해
                // Column을 BIGINT Type으로 지정
                // 32/64bit pointer를 저장, RID와 동일한 크기 저장
                // RID의 크기가 BIGINT크기를 초과할 경우,
                // 이에 대한 수정이 이루어져야 한다.
                IDE_DASSERT( ID_SIZEOF(scGRID) <= ID_SIZEOF(mtdBigintType) );

                IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                 & mtdBigint,
                                                 0,
                                                 0,
                                                 0 )
                          != IDE_SUCCESS );

                // fix BUG-10243
                sOffset = idlOS::align( sOffset,
                                        sNode->dstColumn->module->align );

                sNode->dstColumn->column.offset = sOffset;

                sOffset += sNode->dstColumn->column.size;
                break;
            }

            // PROJ-1502 PARTITIONED DISK TABLE
            case QMC_MTR_TYPE_MEMORY_PARTITIONED_TABLE :
            {
                //-----------------------------------
                // Memory Partitioned Table의 경우
                //-----------------------------------

                // BUG-38309
                // 이제는 16byte보다 커서 mtdByte를 사용한다.
                IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                 & mtdByte,
                                                 1,
                                                 ID_SIZEOF(qmcMemPartRowInfo),
                                                 0 )
                          != IDE_SUCCESS );

                // fix BUG-10243
                sOffset = idlOS::align( sOffset,
                                        sNode->dstColumn->module->align );

                sNode->dstColumn->column.offset = sOffset;

                sOffset += sNode->dstColumn->column.size;
                break;
            }

            case QMC_MTR_TYPE_DISK_PARTITIONED_TABLE :
            {
                //-----------------------------------
                // Disk Partitioned Table의 경우
                //-----------------------------------

                // BUG-38309
                // 이제는 16byte보다 커서 mtdByte를 사용한다.
                // PROJ-2204 join update, delete
                // rid뿐만아니라 indexTuple rid도 저장하고 원복한다.
                IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                 & mtdByte,
                                                 1,
                                                 ID_SIZEOF(qmcDiskPartRowInfo),
                                                 0 )
                          != IDE_SUCCESS );

                // fix BUG-10243
                sOffset = idlOS::align( sOffset,
                                        sNode->dstColumn->module->align );

                sNode->dstColumn->column.offset = sOffset;

                sOffset += sNode->dstColumn->column.size;
                break;
            }

            case QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE : /* PROJ-2464 hybrid partitioned table 지원 */
            {
                //-----------------------------------
                // Hybrid Partitioned Table의 경우
                //-----------------------------------

                // BUG-38309
                // 이제는 16byte보다 커서 mtdByte를 사용한다.
                // PROJ-2204 join update, delete
                // rid뿐만아니라 indexTuple rid도 저장하고 원복한다.
                IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                 & mtdByte,
                                                 1,
                                                 ID_SIZEOF(qmcPartRowInfo),
                                                 0 )
                          != IDE_SUCCESS );

                // fix BUG-10243
                sOffset = idlOS::align( sOffset,
                                        sNode->dstColumn->module->align );

                sNode->dstColumn->column.offset = sOffset;

                sOffset += sNode->dstColumn->column.size;
                break;
            }

            //-----------------------------------------------------------
            // Table 내의 Column에 대한 처리
            //-----------------------------------------------------------

            case QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN :
            {
                //-----------------------------------
                // Memory Column인 경우
                //-----------------------------------

                IDE_TEST( refineOffset4MemoryPartitionColumn( aNode, sNode, & sOffset )
                          != IDE_SUCCESS );
                break;
            }
           
            case QMC_MTR_TYPE_MEMORY_KEY_COLUMN :
            {
                //-----------------------------------
                // Memory Column인 경우
                //-----------------------------------

                IDE_TEST( refineOffset4MemoryColumn( aNode, sNode, & sOffset )
                          != IDE_SUCCESS );
                break;
            }

            //-----------------------------------------------------------
            // Expression 및 특수 Column에 대한 처리
            //-----------------------------------------------------------
            case QMC_MTR_TYPE_CALCULATE:
            {
                // To Fix PR-8146
                // AVG(), STDDEV()등의 처리를 위해
                // Offset 계산을 위해서 해당 Expression의 Column개수를
                // 이용하여 계산하여야 한다.

                // To Fix PR-12093
                // Destine Node를 사용하여
                // mtcColumn의 Count를 구하는 것이 원칙에 맞음
                // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
                //     - Memory 공간 낭비
                //     - offset 조정 오류 (PR-12093)
                sColumnCnt =
                    sNode->dstNode->node.module->lflag &
                    MTC_NODE_COLUMN_COUNT_MASK;

                for ( i = 0; i < sColumnCnt; i++ )
                {
                    // 값 자체가 저장되는 경우이므로
                    // Data Type에 맞도록 offset등을 align한다.
                    sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
                    sNode->dstColumn[i].column.flag |= SMI_COLUMN_TYPE_FIXED;

                    // BUG-38494
                    // Compressed Column 역시 값 자체가 저장되므로
                    // Compressed 속성을 삭제한다
                    sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                    sNode->dstColumn[i].column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

                    sOffset =
                        idlOS::align( sOffset,
                                      sNode->dstColumn[i].module->align );
                    sNode->dstColumn[i].column.offset = sOffset;
                    sOffset += sNode->dstColumn[i].column.size;
                }

                break;
            }

            case QMC_MTR_TYPE_COPY_VALUE:
            case QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE:
            case QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN : /* PROJ-2464 hybrid partitioned table 지원 */
            {
                // To Fix PR-8146
                // AVG(), STDDEV()등의 처리를 위해
                // Offset 계산을 위해서 해당 Expression의 Column개수를
                // 이용하여 계산하여야 한다.

                // To Fix PR-12093
                // Destine Node를 사용하여
                // mtcColumn의 Count를 구하는 것이 원칙에 맞음
                // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
                //     - Memory 공간 낭비
                //     - offset 조정 오류 (PR-12093)
                sColumnCnt =
                    sNode->dstNode->node.module->lflag &
                    MTC_NODE_COLUMN_COUNT_MASK;

                for ( i = 0; i < sColumnCnt; i++ )
                {
                    // 값 자체가 저장되는 경우이므로
                    // Data Type에 맞도록 offset등을 align한다.
                    sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
                    sNode->dstColumn[i].column.flag |= SMI_COLUMN_TYPE_FIXED;

                    // BUG-38494
                    // Compressed Column 역시 값 자체가 저장되므로
                    // Compressed 속성을 삭제한다
                    sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                    sNode->dstColumn[i].column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

                    sOffset =
                        idlOS::align( sOffset,
                                      sNode->dstColumn[i].module->align );
                    sNode->dstColumn[i].column.offset = sOffset;
                    sOffset += sNode->dstColumn[i].column.size;

                    // PROJ-2362 memory temp 저장 효율성 개선
                    if ( ( ( sNode->flag & QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK )
                           == QMC_MTR_TEMP_VAR_TYPE_ENABLE_TRUE ) &&
                         ( QMC_IS_MTR_TEMP_VAR_COLUMN( sNode->dstColumn[i] ) == ID_TRUE ) )
                    {
                        sTempTypeColCount++;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                break;
            }
            case QMC_MTR_TYPE_USELESS_COLUMN:
            {
                // PROJ-2469 Optimize View Materialization
                // BUG-41681
                // 동일 타입의 useless column이 있다면 같이 사용하고
                // 상위에서 사용하지 않아, Materialize 할 필요 없는
                // View Column에 대해서 null value만 저장하도록 한다.
                for ( sTmpNode = aNode; sTmpNode != sNode; sTmpNode = sTmpNode->next )
                {
                    if ( ( ( sTmpNode->flag & QMC_MTR_TYPE_MASK )
                           == QMC_MTR_TYPE_USELESS_COLUMN ) &&
                         ( sTmpNode->dstColumn->module->id
                           == sNode->dstColumn->module->id ) )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                if ( sTmpNode != sNode )
                {
                    mtc::initializeColumn( sNode->dstColumn,
                                           sTmpNode->dstColumn );
                        
                    sNode->dstColumn->column.offset =
                        sTmpNode->dstColumn->column.offset;
                }
                else
                {
                    // column type은 유지하되, null value만 저장하도록
                    // column size를 줄인다.
                    if ( ( sNode->dstColumn->module->flag & MTD_CREATE_PARAM_MASK )
                         == MTD_CREATE_PARAM_NONE )
                    {
                        sArgument  = 0;
                        sPrecision = 0;
                        sScale     = 0;
                    }
                    else
                    {
                        if ( ( sNode->dstColumn->module->flag & MTD_CREATE_PARAM_MASK )
                             == MTD_CREATE_PARAM_PRECISION )
                        {
                            sArgument = 1;
                        }
                        else
                        {
                            sArgument = 2;
                        }

                        IDE_TEST( sNode->dstColumn->module->getPrecision(
                                      sNode->dstColumn,
                                      sNode->dstColumn->module->staticNull,
                                      & sPrecision,
                                      & sScale )
                                  != IDE_SUCCESS );
                    }

                    IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                     sNode->dstColumn->module,
                                                     sArgument,
                                                     sPrecision,
                                                     sScale )
                              != IDE_SUCCESS );
                        
                    IDE_DASSERT( sNode->dstColumn->column.size >=
                                 sNode->dstColumn->module->actualSize(
                                     sNode->dstColumn,
                                     sNode->dstColumn->module->staticNull ) );
                    
                    sOffset = idlOS::align( sOffset,
                                            sNode->dstColumn->module->align );
                    
                    sNode->dstColumn->column.offset = sOffset;

                    sOffset += sNode->dstColumn->column.size;
                }
                break;
            }
            default :
            {
                // Materialized Column의 종류가 지정되어 있어야 함.
                IDE_DASSERT(0);

                break;
            }
        } // end of switch

        //----------------------------------------------------
        // Column이 저장될 매체의 지정
        //     - Memory Temp Table일 경우
        //     - Disk Temp Table일 경우
        //----------------------------------------------------

        if ( (sNode->dstTuple->lflag & MTC_TUPLE_STORAGE_MASK)
             == MTC_TUPLE_STORAGE_MEMORY )
        {
            sNode->dstColumn->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sNode->dstColumn->column.flag |= SMI_COLUMN_STORAGE_MEMORY;
        }
        else
        {
            sNode->dstColumn->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sNode->dstColumn->column.flag |= SMI_COLUMN_STORAGE_DISK;
        }
    }

    //-----------------------------------------------------
    // PROJ-2362 memory temp 저장 효율성 개선
    // TEMP_TYPE이 사용할 컬럼 크기의 계산
    //-----------------------------------------------------

    if ( sTempTypeColCount > 0 )
    {
        if ( sOffset + sTempTypeColCount < 256 )
        {
            // 1 byte offset
            sTempTypeColSize = 1;
        }
        else if ( sOffset + ( sTempTypeColCount + 1 ) * 2 < 65536 ) // 2byte align을 고려한다.
        {
            // 2 byte offset
            sTempTypeColSize = 2;
        }
        else
        {
            // 4 byte offset
            sTempTypeColSize = 4;
        }

        //--------------------------------------------------
        // 구성된 Materialzied Column들을 순회하며,
        // offset과 size등을 재조정한다.
        // 필요할 경우, Fixed->Variable로 변경하는 작업도
        // 진행한다.
        //--------------------------------------------------

        sOffset = aStartOffset;

        for ( sNode = aNode; sNode != NULL; sNode = sNode->next )
        {
            switch ( sNode->flag & QMC_MTR_TYPE_MASK )
            {
                //-----------------------------------------------------------
                // Base Table에 대한 처리 방식
                //-----------------------------------------------------------

                case QMC_MTR_TYPE_MEMORY_TABLE :
                {
                    //-----------------------------------
                    // Memory Base Table의 경우
                    //-----------------------------------
                    // Disk Temp Table에서의 처리를 위해 Column을
                    // BIGINT Type으로 지정( 32/64bit pointer의 저장,
                    // RID와 동일한 크기 저장). RID의 크기가 BIGINT크기를 초과할
                    // 경우, 이에 대한 수정이 이루어져야 한다.
                    IDE_DASSERT( ID_SIZEOF(scGRID) <= ID_SIZEOF(mtdBigintType) );

                    IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                     & mtdBigint,
                                                     0,
                                                     0,
                                                     0 )
                              != IDE_SUCCESS );

                    // fix BUG-10243
                    sOffset = idlOS::align( sOffset,
                                            sNode->dstColumn->module->align );

                    sNode->dstColumn->column.offset = sOffset;

                    sOffset += sNode->dstColumn->column.size;
                    break;
                }
                case QMC_MTR_TYPE_DISK_TABLE :
                {
                    //-----------------------------------
                    // Disk Base Table의 경우
                    //-----------------------------------
                    // Disk Temp Table에서의 처리를 위해
                    // Column을 BIGINT Type으로 지정
                    // 32/64bit pointer를 저장, RID와 동일한 크기 저장
                    // RID의 크기가 BIGINT크기를 초과할 경우,
                    // 이에 대한 수정이 이루어져야 한다.
                    IDE_DASSERT( ID_SIZEOF(scGRID) <= ID_SIZEOF(mtdBigintType) );

                    IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                     & mtdBigint,
                                                     0,
                                                     0,
                                                     0 )
                              != IDE_SUCCESS );

                    // fix BUG-10243
                    sOffset = idlOS::align( sOffset,
                                            sNode->dstColumn->module->align );

                    sNode->dstColumn->column.offset = sOffset;

                    sOffset += sNode->dstColumn->column.size;
                    break;
                }

                // PROJ-1502 PARTITIONED DISK TABLE
                case QMC_MTR_TYPE_MEMORY_PARTITIONED_TABLE :
                {
                    //-----------------------------------
                    // Memory Partitioned Table의 경우
                    //-----------------------------------

                    // BUG-38309
                    // 이제는 16byte보다 커서 mtdByte를 사용한다.
                    IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                     & mtdByte,
                                                     1,
                                                     ID_SIZEOF(qmcMemPartRowInfo),
                                                     0 )
                              != IDE_SUCCESS );

                    // fix BUG-10243
                    sOffset = idlOS::align( sOffset,
                                            sNode->dstColumn->module->align );

                    sNode->dstColumn->column.offset = sOffset;

                    sOffset += sNode->dstColumn->column.size;
                    break;
                }

                case QMC_MTR_TYPE_DISK_PARTITIONED_TABLE :
                {
                    //-----------------------------------
                    // Disk Partitioned Table의 경우
                    //-----------------------------------

                    // BUG-38309
                    // 이제는 16byte보다 커서 mtdByte를 사용한다.
                    // PROJ-2204 join update, delete
                    // rid뿐만아니라 indexTuple rid도 저장하고 원복한다.
                    IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                     & mtdByte,
                                                     1,
                                                     ID_SIZEOF(qmcDiskPartRowInfo),
                                                     0 )
                              != IDE_SUCCESS );

                    // fix BUG-10243
                    sOffset = idlOS::align( sOffset,
                                            sNode->dstColumn->module->align );

                    sNode->dstColumn->column.offset = sOffset;

                    sOffset += sNode->dstColumn->column.size;
                    break;
                }

                case QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE : /* PROJ-2464 hybrid partitioned table 지원 */
                {
                    //-----------------------------------
                    // Hybrid Partitioned Table의 경우
                    //-----------------------------------

                    // BUG-38309
                    // 이제는 16byte보다 커서 mtdByte를 사용한다.
                    // PROJ-2204 join update, delete
                    // rid뿐만아니라 indexTuple rid도 저장하고 원복한다.
                    IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                     & mtdByte,
                                                     1,
                                                     ID_SIZEOF(qmcPartRowInfo),
                                                     0 )
                              != IDE_SUCCESS );

                    // fix BUG-10243
                    sOffset = idlOS::align( sOffset,
                                            sNode->dstColumn->module->align );

                    sNode->dstColumn->column.offset = sOffset;

                    sOffset += sNode->dstColumn->column.size;
                    break;
                }

                //-----------------------------------------------------------
                // Table 내의 Column에 대한 처리
                //-----------------------------------------------------------

                case QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN :
                {
                    //-----------------------------------
                    // Memory Column인 경우
                    //-----------------------------------

                    IDE_TEST( refineOffset4MemoryPartitionColumn( aNode, sNode, & sOffset )
                              != IDE_SUCCESS );
                    break;
                }

                case QMC_MTR_TYPE_MEMORY_KEY_COLUMN :
                {
                    //-----------------------------------
                    // Memory Column인 경우
                    //-----------------------------------

                    IDE_TEST( refineOffset4MemoryColumn( aNode, sNode, & sOffset )
                              != IDE_SUCCESS );
                    break;
                }

                //-----------------------------------------------------------
                // Expression 및 특수 Column에 대한 처리
                //-----------------------------------------------------------
                case QMC_MTR_TYPE_CALCULATE:
                {
                    // To Fix PR-8146
                    // AVG(), STDDEV()등의 처리를 위해
                    // Offset 계산을 위해서 해당 Expression의 Column개수를
                    // 이용하여 계산하여야 한다.

                    // To Fix PR-12093
                    // Destine Node를 사용하여
                    // mtcColumn의 Count를 구하는 것이 원칙에 맞음
                    // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
                    //     - Memory 공간 낭비
                    //     - offset 조정 오류 (PR-12093)
                    sColumnCnt =
                        sNode->dstNode->node.module->lflag &
                        MTC_NODE_COLUMN_COUNT_MASK;

                    for ( i = 0; i < sColumnCnt; i++ )
                    {
                        // 값 자체가 저장되는 경우이므로
                        // Data Type에 맞도록 offset등을 align한다.
                        sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
                        sNode->dstColumn[i].column.flag |= SMI_COLUMN_TYPE_FIXED;

                        // BUG-38494
                        // Compressed Column 역시 값 자체가 저장되므로
                        // Compressed 속성을 삭제한다
                        sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                        sNode->dstColumn[i].column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

                        sOffset =
                            idlOS::align( sOffset,
                                          sNode->dstColumn[i].module->align );
                        sNode->dstColumn[i].column.offset = sOffset;
                        sOffset += sNode->dstColumn[i].column.size;
                    }

                    break;
                }

                case QMC_MTR_TYPE_COPY_VALUE:
                case QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE:
                case QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN : /* PROJ-2464 hybrid partitioned table 지원 */
                {
                    // To Fix PR-8146
                    // AVG(), STDDEV()등의 처리를 위해
                    // Offset 계산을 위해서 해당 Expression의 Column개수를
                    // 이용하여 계산하여야 한다.

                    // To Fix PR-12093
                    // Destine Node를 사용하여
                    // mtcColumn의 Count를 구하는 것이 원칙에 맞음
                    // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
                    //     - Memory 공간 낭비
                    //     - offset 조정 오류 (PR-12093)
                    sColumnCnt =
                        sNode->dstNode->node.module->lflag &
                        MTC_NODE_COLUMN_COUNT_MASK;

                    for ( i = 0; i < sColumnCnt; i++ )
                    {
                        // PROJ-2362 memory temp 저장 효율성 개선
                        if ( ( ( sNode->flag & QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK )
                               == QMC_MTR_TEMP_VAR_TYPE_ENABLE_TRUE ) &&
                             ( QMC_IS_MTR_TEMP_VAR_COLUMN( sNode->dstColumn[i] ) == ID_TRUE ) )
                        {
                            // TEMP_TYPE으로 설정한다.
                            if ( sTempTypeColSize == 1 )
                            {
                                sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
                                sNode->dstColumn[i].column.flag |= SMI_COLUMN_TYPE_TEMP_1B;
                            }
                            else if ( sTempTypeColSize == 2 )
                            {
                                sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
                                sNode->dstColumn[i].column.flag |= SMI_COLUMN_TYPE_TEMP_2B;
                            }
                            else
                            {
                                sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
                                sNode->dstColumn[i].column.flag |= SMI_COLUMN_TYPE_TEMP_4B;
                            }

                            sOffset =
                                idlOS::align( sOffset,
                                              sTempTypeColSize );
                            sNode->dstColumn[i].column.offset = sOffset;
                            sOffset += sTempTypeColSize;
                        }
                        else
                        {
                            // 값 자체가 저장되는 경우이므로
                            // Data Type에 맞도록 offset등을 align한다.
                            sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_TYPE_MASK;
                            sNode->dstColumn[i].column.flag |= SMI_COLUMN_TYPE_FIXED;

                            // BUG-38494
                            // Compressed Column 역시 값 자체가 저장되므로
                            // Compressed 속성을 삭제한다
                            sNode->dstColumn[i].column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                            sNode->dstColumn[i].column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

                            sOffset =
                                idlOS::align( sOffset,
                                              sNode->dstColumn[i].module->align );
                            sNode->dstColumn[i].column.offset = sOffset;
                            sOffset += sNode->dstColumn[i].column.size;
                        }
                    }

                    break;
                }
                case QMC_MTR_TYPE_USELESS_COLUMN:
                {
                    // PROJ-2469 Optimize View Materialization
                    // BUG-41681
                    // 동일 타입의 useless column이 있다면 같이 사용하고
                    // 상위에서 사용하지 않아, Materialize 할 필요 없는
                    // View Column에 대해서 null value만 저장하도록 한다.
                    for ( sTmpNode = aNode; sTmpNode != sNode; sTmpNode = sTmpNode->next )
                    {
                        if ( ( ( sTmpNode->flag & QMC_MTR_TYPE_MASK )
                               == QMC_MTR_TYPE_USELESS_COLUMN ) &&
                             ( sTmpNode->dstColumn->module->id
                               == sNode->dstColumn->module->id ) )
                        {
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }

                    if ( sTmpNode != sNode )
                    {
                        mtc::initializeColumn( sNode->dstColumn,
                                               sTmpNode->dstColumn );
                        
                        sNode->dstColumn->column.offset =
                            sTmpNode->dstColumn->column.offset;
                    }
                    else
                    {
                        // column type은 유지하되, null value만 저장하도록
                        // column size를 줄인다.
                        if ( ( sNode->dstColumn->module->flag & MTD_CREATE_PARAM_MASK )
                             == MTD_CREATE_PARAM_NONE )
                        {
                            sArgument  = 0;
                            sPrecision = 0;
                            sScale     = 0;
                        }
                        else
                        {
                            if ( ( sNode->dstColumn->module->flag & MTD_CREATE_PARAM_MASK )
                                 == MTD_CREATE_PARAM_PRECISION )
                            {
                                sArgument = 1;
                            }
                            else
                            {
                                sArgument = 2;
                            }

                            IDE_TEST( sNode->dstColumn->module->getPrecision(
                                          sNode->dstColumn,
                                          sNode->dstColumn->module->staticNull,
                                          & sPrecision,
                                          & sScale )
                                      != IDE_SUCCESS );
                        }

                        IDE_TEST( mtc::initializeColumn( sNode->dstColumn,
                                                         sNode->dstColumn->module,
                                                         sArgument,
                                                         sPrecision,
                                                         sScale )
                                  != IDE_SUCCESS );
                        
                        IDE_DASSERT( sNode->dstColumn->column.size >=
                                     sNode->dstColumn->module->actualSize(
                                         sNode->dstColumn,
                                         sNode->dstColumn->module->staticNull ) );
                        
                        sOffset = idlOS::align( sOffset,
                                                sNode->dstColumn->module->align );
                        
                        sNode->dstColumn->column.offset = sOffset;
                        
                        sOffset += sNode->dstColumn->column.size;
                    }
                    break;                
                }                   
                default :
                {
                    // Materialized Column의 종류가 지정되어 있어야 함.
                    IDE_DASSERT(0);

                    break;
                }
            } // end of switch
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    해당 Tuple의 Fixed Record Size를 계산한다.
 *
 * Implementation :
 *
 ***********************************************************************/
UInt qmc::getRowOffsetForTuple( mtcTemplate * aTemplate,
                                UShort        aTupleID )
{

    mtcColumn * sColumn;
    UInt        sFixedRecordSize;
    UShort      i;

    //-----------------------------------------------------
    // Fixed Record의 Size 계산
    //-----------------------------------------------------

    // To Fix PR-8200
    // 저장 공간의 크기 계산시 다음과 같은 사항을 고려하여야 한다.
    // 예를 들어 다음과 같이 저장 컬럼을 구성할 때
    // [T1]-->[T2]-->[T3]-->[T3.i1]-->[T2.i1]-->[T3.i1]
    // 각각의 offset은 다음과 같이 결정된다.
    //  0  -->  8 --> 16 -->  16   -->  8    -->   0
    // 이는 ::refineOffsets()에 의해 메모리 컬럼은 별도로
    // 저장 공간을 확보하지 않기 때문이다.
    // 따라서, 가장 Offset이 큰 컬럼을 기준으로 Fixed Record의
    // Size를 계산하여야 한다.

    // 가장 Offset이 큰 Column을 획득
    sColumn = aTemplate->rows[aTupleID].columns;
    for ( i = 1; i < aTemplate->rows[aTupleID].columnCount; i++ )
    {
        /* PROJ-2419 반영 이전에는 모든 Column의 Offset이 달랐으나,
         * PROJ-2419 반영 이후에는 Offset이 0인 Column이 여러 개일 수 있다.
         * 그러나, Fixed Column/Large Variable Column인 경우,
         * Fixed Slot Header Size(32) 때문에 Offset이 32 이상이므로 문제가 없다.
         */
        if ( aTemplate->rows[aTupleID].columns[i].column.offset
             > sColumn->column.offset )
        {
            sColumn = & aTemplate->rows[aTupleID].columns[i];
        }
        else
        {
            // Nothing To Do
        }
    }

    // PROJ-2264 Dictionary table
    if( (sColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK)
        == SMI_COLUMN_COMPRESSION_FALSE )
    {
        if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_VARIABLE )
        {
            // Disk Variable Column일 경우는 없다.
            IDE_ASSERT( ( sColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
                        == SMI_COLUMN_STORAGE_MEMORY );

            // 마지막 Column이 Variable Column일 경우
            // Variable Column의 Header Size를 계산
            /* Fixed Slot에 Column이 없는 경우, Fixed Slot Header Size 만큼의 공간이 필요하다.
             * Variable Slot의 OID는 Fixed Slot Header에 존재한다.
             */
            sFixedRecordSize = smiGetRowHeaderSize( SMI_TABLE_MEMORY );
        }
        else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_VARIABLE_LARGE )
        {
            // Disk Variable Column일 경우는 없다.
            IDE_ASSERT( ( sColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
                        == SMI_COLUMN_STORAGE_MEMORY );

            // 마지막 Column이 Variable Column일 경우
            // Variable Column의 Header Size를 계산
            // Large Variable Column일 경우
            sFixedRecordSize = sColumn->column.offset +
                IDL_MAX( smiGetVariableColumnSize( SMI_TABLE_MEMORY ),
                         smiGetVCDescInModeSize() + sColumn->column.vcInOutBaseSize );
        }
        else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_LOB )
        {
            // PROJ-1362
            if ( (sColumn->column.flag & SMI_COLUMN_STORAGE_MASK)
                 == SMI_COLUMN_STORAGE_MEMORY )
            {
                sFixedRecordSize = sColumn->column.offset +
                    IDL_MAX( smiGetLobColumnSize( SMI_TABLE_MEMORY ),
                             smiGetVCDescInModeSize() + sColumn->column.vcInOutBaseSize );
            }
            else
            {
                sFixedRecordSize = sColumn->column.offset +
                    smiGetLobColumnSize( SMI_TABLE_DISK );
            }
        }
        // PROJ-2362 memory temp 저장 효율성 개선
        else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_TEMP_1B )
        {
            sFixedRecordSize = sColumn->column.offset + 1;
        }
        else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_TEMP_2B )
        {
            sFixedRecordSize = sColumn->column.offset + 2;
        }
        else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_TEMP_4B )
        {
            sFixedRecordSize = sColumn->column.offset + 4;
        }
        else
        {
            // 마지막 Column이 Fixed Column일 경우
            sFixedRecordSize = sColumn->column.offset + sColumn->column.size;
        }
    }
    else
    {
        // PROJ-2264 Dictionary table
        sFixedRecordSize = sColumn->column.offset + ID_SIZEOF( smOID );
    }

    sFixedRecordSize = idlOS::align8( sFixedRecordSize );

    return sFixedRecordSize;
}

IDE_RC qmc::setRowSize( iduMemory  * aMemory,
                        mtcTemplate* aTemplate,
                        UShort       aTupleID )
{
/***********************************************************************
 *
 * Description :
 *    해당 Tuple의 Record Size를 계산하고, Memory를 할당함.
 *    Disk Table, Disk Temp Table, Memory Temp Table에 한하여 사용함.
 *
 * Implementation :
 *
 *    - Fixed Record의 Size를 구한다.
 *        - Offset이 가장 큰 Column의 종류(Fixed/Variable)에 따라,
 *          Record 크기를 결정한다.
 *    - Disk Table(Disk Temp Table)의 경우
 *        - Variable Column들을 위한 공간의 크기를 계산한다.
 *        - 각 Variable Column의 공간은 Variable Column
 *    - 크기에 맞는 Memory를 할당함.
 *    - Disk Table(Disk Temp Table)의 경우
 *        - Variable Column을 위한 value pointer를 설정한다.
 *
 ***********************************************************************/

    mtcColumn * sColumn             = NULL;
    UInt        sVariableRecordSize = 0;
    UInt        sVarColumnOffset    = 0;
    SChar     * sColumnBuffer       = NULL;
    UShort      i                   = 0;

    //-----------------------------------------------------
    // Memory Table의 경우 Variable Column을 위한
    // 공간의 크기 계산
    //-----------------------------------------------------

    sVariableRecordSize = 0;

    if ( ( aTemplate->rows[aTupleID].lflag & MTC_TUPLE_STORAGE_MASK)
         == MTC_TUPLE_STORAGE_DISK )
    {
        // Nothing To Do
    }
    else
    {
        /* PROJ-2160 */
        if( (aTemplate->rows[aTupleID].lflag & MTC_TUPLE_ROW_GEOMETRY_MASK)
            == MTC_TUPLE_ROW_GEOMETRY_TRUE )
        {
            //PROJ-1583 large geometry
            for ( i = 0; i < aTemplate->rows[aTupleID].columnCount; i++ )
            {
                sColumn = aTemplate->rows[aTupleID].columns + i;

                // To fix BUG-24356
                // geometry에 대해서만 value buffer할당
                if ( ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                       == SMI_COLUMN_TYPE_VARIABLE_LARGE ) &&
                     (sColumn->module->id == MTD_GEOMETRY_ID) )
                {
                    sVariableRecordSize = idlOS::align8( sVariableRecordSize );
                    sVariableRecordSize = sVariableRecordSize +
                        smiGetVariableColumnSize(SMI_TABLE_MEMORY) +
                        sColumn->column.size;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
        else
        {
            // Nothing To Do
        }

        // Memory Table(Temp Table)의 경우,
        // Variable Column을 위한 별도의 공간이 필요 없다.
        // Memory Temp Table의 Disk Variable Column => Fixed Column으로 처리
        // Disk Temp Table의 Memory Variable Column => Fixed Column으로 처리

        // PROJ-2362 memory temp 저장 효율성 개선
        // disk column이 memory temp에 쌓이는 경우 variable length type에 대해서
        // SMI_COLUMN_TEMP_TYPE으로 저장한다.
        for ( i = 0; i < aTemplate->rows[aTupleID].columnCount; i++ )
        {
            sColumn = aTemplate->rows[aTupleID].columns + i;

            if ( SMI_COLUMN_TYPE_IS_TEMP( sColumn->column.flag ) == ID_TRUE )
            {
                sVariableRecordSize = idlOS::align( sVariableRecordSize,
                                                    sColumn->module->align );
                sVariableRecordSize = sVariableRecordSize +
                    sColumn->column.size;
            }
            else
            {
                // Nothing To Do
            }
        }

        sVariableRecordSize = idlOS::align8( sVariableRecordSize );

        // BUG-10858
        aTemplate->rows[aTupleID].row = NULL;
    }

    //-----------------------------------------------------
    // Record Size의 Setting
    //-----------------------------------------------------

    aTemplate->rows[aTupleID].rowOffset = qmc::getRowOffsetForTuple( aTemplate, aTupleID );

    //-----------------------------------------------------
    // 공간 할당 및 Variable Column Value Pointer Setting
    //-----------------------------------------------------

    if ( ( aTemplate->rows[aTupleID].lflag & MTC_TUPLE_STORAGE_MASK)
         == MTC_TUPLE_STORAGE_DISK )
    {
        // 디스크테이블에 대한 공간 할당
        IDU_LIMITPOINT("qmc::setRowSize::malloc2");
        IDE_TEST( aMemory->cralloc( aTemplate->rows[aTupleID].rowOffset,
                                    & aTemplate->rows[aTupleID].row )
                  != IDE_SUCCESS );
    }
    else
    {
        // PROJ-1583 large geometry
        // 공간 할당 및 Variable Column Value Pointer Setting

        // 공간 할당
        // To Fix PR-8561
        // Variable Column에 RID등의 정보를 담고 있기 때문에
        // 초기화하여야 함.
        if( sVariableRecordSize > 0 )
        {
            IDU_LIMITPOINT("qmc::setRowSize::malloc3");
            IDE_TEST( aMemory->cralloc( sVariableRecordSize,
                                        (void **)&sColumnBuffer )
                      != IDE_SUCCESS );

            sVarColumnOffset = 0;

            // Memory Variable Column을 할당된 공간에 Memory를 Setting함.
            for ( i = 0; i < aTemplate->rows[aTupleID].columnCount; i++ )
            {
                sColumn = aTemplate->rows[aTupleID].columns + i;

                // To fix BUG-24356
                // geometry에 대해서만 value buffer할당
                if ( ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                       == SMI_COLUMN_TYPE_VARIABLE_LARGE ) &&
                     (sColumn->module->id == MTD_GEOMETRY_ID) )
                {
                    sVarColumnOffset = idlOS::align8( sVarColumnOffset );
                    sColumn->column.value =
                        (void *)(sColumnBuffer + sVarColumnOffset);
                    sVarColumnOffset = sVarColumnOffset +
                        smiGetVariableColumnSize(SMI_TABLE_MEMORY) +
                        sColumn->column.size;
                }
                else
                {
                    // Nothing To Do
                }
            }

            // PROJ-2362 memory temp 저장 효율성 개선
            for ( i = 0; i < aTemplate->rows[aTupleID].columnCount; i++ )
            {
                sColumn = aTemplate->rows[aTupleID].columns + i;

                if ( SMI_COLUMN_TYPE_IS_TEMP( sColumn->column.flag ) == ID_TRUE )
                {
                    sVarColumnOffset = idlOS::align( sVarColumnOffset,
                                                     sColumn->module->align );
                    sColumn->column.value =
                        (void *)(sColumnBuffer + sVarColumnOffset);
                    sVarColumnOffset = sVarColumnOffset +
                        sColumn->column.size;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt
qmc::getMtrRowSize( qmdMtrNode * aNode )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Row의 공간 크기를 구한다.
 *    ::setRowSize()후에 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmc::getMtrRowSize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return aNode->dstTuple->rowOffset;

#undef IDE_FN
}

IDE_RC
qmc::setMtrNode( qcTemplate * aTemplate,
                 qmdMtrNode * aDataNode,
                 qmcMtrNode * aCodeNode,
                 idBool       aBaseTable )
{
/***********************************************************************
 *
 * Description :
 *    Code 영역의 Materialized Node 정보(aCodeNode)로부터
 *    Data 영역의 Materialized Node 정보(aDataNode)를 구성한다.
 *    이 때, Destination Node영역의 Column정보 및 Execute 정보도 구성한다.
 *
 * Implementation :
 *                 destine node
 *                  ^
 *    qmcMtrNode----|
 *                  V
 *                 source node
 *
 *    보다 빠른 접근을 위해 qmdMtrNode에는 node 정보 뿐 아니라,
 *    Source와 Destine의 Column 정보 및 Execute 정보를 별도로 관리한다.
 *    또한, source node의 정보를 이용하여 destine node의 column정보 및
 *    exectue 정보도 이 시점에서 결정된다.
 *
 ***********************************************************************/

    qtcNode    * sSrcNode;
    mtcColumn  * sColumn;
    mtcExecute * sExecute;
    UInt         sColumnCnt;

    //----------------------------------------------------
    // Base Table 여부에 따라,
    // 정확한 Source Node를 추출한다.
    //----------------------------------------------------

    if ( aBaseTable == ID_TRUE )
    {
        // To Fix PR-11567
        // Base Table 중에도 Pass Node가 존재할 수 있다.
        // Ex) SELECT /+ DISTINCT SORT +/
        //     DISTINCT i1 + 1, i2 FROM T1 ORDER BY (i1 + 1) % 2, i2;
        // Pass Node의 경우 Size가 없는 Column 정보를 갖기 때문에
        // 이를 이용하여 destination column 정보를 구축하면 안된다.

        // To FIX-PR-20820
        // Base Table이 SubQuery 일 경우도 있다.
        // Order by 에 들어가는 Column이 Group By 에 나오는 Column 또는 Aggregation Function 등이 아닐 수도 있다.
        // Ex) SELECT
        //         ( SELECT s1 FROM t1 )
        //     FROM t1
        //        GROUP BY s1, s2
        //        ORDER BY s2;
        if ( QTC_IS_SUBQUERY( aCodeNode->srcNode ) == ID_TRUE )
        {
            sSrcNode = (qtcNode*)mtf::convertedNode( &aCodeNode->srcNode->node,
                                                     &aTemplate->tmplate );
        }
        else
        {
            // Aggregation Column의 경우도 Conversion된 값을
            // 사용하지 않도록 Base Table로 처리되어야 한다.
            sSrcNode = aCodeNode->srcNode;
        }
    }
    else
    {
        // Base Table이 아니라면,
        // 실제 값이 저장된 Node를 중심으로 그 정보를 구성하여야 한다.
        sSrcNode = (qtcNode*)mtf::convertedNode( &aCodeNode->srcNode->node,
                                                 &aTemplate->tmplate );
    }

    //------------------------------------------------------
    // Data Materialized Node 의 기본 정보 Setting
    //    - flag 정보 복사
    //    - Destination 영역의 정보 Setting
    //        - dstNode : destination Node
    //        - dstTuple : destination node의 tuple 위치
    //        - dstColumn : destination node의 column 위치
    //    - Source Node 영역의 정보 Setting
    //        - srcNode :
    //        - srcTuple
    //        - srcColumn
    //------------------------------------------------------

    // Destie Node의 정보 Setting
    aDataNode->flag = aCodeNode->flag;
    aDataNode->dstNode = aCodeNode->dstNode;
    aDataNode->dstTuple =
        & aTemplate->tmplate.rows[aCodeNode->dstNode->node.table];
    aDataNode->dstColumn =
        QTC_TUPLE_COLUMN(aDataNode->dstTuple, aCodeNode->dstNode);

    // Source Node의 정보 Setting
    aDataNode->srcNode = aCodeNode->srcNode;
    aDataNode->srcTuple =
        & aTemplate->tmplate.rows[aCodeNode->srcNode->node.table];
    aDataNode->srcColumn =
        QTC_TUPLE_COLUMN(aDataNode->srcTuple, aCodeNode->srcNode);

    //--------------------------------------------------------
    // Destination Node영역에 해당하는 Column 정보 및 Execute
    // 정보를 구성한다.
    // 이 때, 원래 Source Node가 아닌
    // 저장 대상이 되는 Source Node의 Column 정보와
    // Execute 정보를 복사한다.
    //
    // 참조) Host 변수가 없는 경우라면, Destination Node의 정보가
    //       이미 결정되어 있기도 하지만, 동일한 결과를 복사하기
    //       때문에 큰 문제가 되지 않는다.
    //--------------------------------------------------------

    // 저장할 Source Node의 Column 정보 및 Execute 정보 획득
    sColumn  = QTC_TMPL_COLUMN(aTemplate, sSrcNode);
    sExecute = QTC_TMPL_EXECUTE(aTemplate, sSrcNode);

    // To Fix PR-8146
    // Destine Node가 아닌 Source Node를 사용하여야 함.
    // To Fix PR-12093
    // Destine Node를 사용하여 mtcColumn의 Count를 구하는 것이 원칙에 맞음
    // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
    //     - Memory 공간 낭비
    //     - offset 조정 오류 (PR-12093)
    sColumnCnt =
        aDataNode->dstNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK;

    // Column 정보의 복사
    if( aDataNode->dstColumn != sColumn )
    {
        // mtc::copyColumn()을 사용하지 않는다.
        // Variable은 Variable 그대로 유지한다.
        // 필요시 Fixed로 변경한다.
        idlOS::memcpy( aDataNode->dstColumn,
                       sColumn,
                       ID_SIZEOF(mtcColumn) * sColumnCnt );
    }
    else
    {
        // BUG-34737
        // src와 dst가 같은 경우 복사할 필요가 없다.
        // Nothing to do.
    }

    // Execute 정보의 복사
    if( aDataNode->dstTuple->execute + aDataNode->dstNode->node.column != sExecute )
    {
        idlOS::memcpy(
            aDataNode->dstTuple->execute + aDataNode->dstNode->node.column,
            sExecute,
            ID_SIZEOF(mtcExecute) * sColumnCnt );
    }
    else
    {
        // Nothing to do.
    }

    //--------------------------------------------------------
    // [Materialized Node의 함수 포인터 연결]
    // Materialized Node의 함수 포인터를 연결한다.
    //    - Conversion이 발생한 경우
    //    - 각 Materialized Node의 종류에 따른 설정
    //--------------------------------------------------------

    // BUG-36472
    // dstNode 가 valueModule 일 경우 보정
    // valueModule 은 QMC_MTR_TYPE_CALCULATE 일 수 없음
    IDE_DASSERT( ( aDataNode->dstNode->node.module != &qtc::valueModule ) ||
                 ( ( aCodeNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_CALCULATE ) );

    if( ( aDataNode->dstNode->node.module == &qtc::valueModule ) ||
        ( ( sSrcNode != aCodeNode->srcNode ) &&
          ( ( aCodeNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_CALCULATE ) ) )
    {
        // Conversion이 발생하는 경우 또는 Indirection이 발생하는 경우

        // To Fix PR-8333
        // Value와 동일한 취급을 해야 함.
        aDataNode->dstTuple->execute[aDataNode->dstNode->node.column] =
            qmc::valueExecute;

        aDataNode->dstColumn->column.flag &= ~SMI_COLUMN_TYPE_MASK;
        aDataNode->dstColumn->column.flag |= SMI_COLUMN_TYPE_FIXED;

        // BUG-38494
        // Compressed Column 역시 값 자체가 저장되므로
        // Compressed 속성을 삭제한다
        aDataNode->dstColumn->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
        aDataNode->dstColumn->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( setFunctionPointer( aDataNode ) != IDE_SUCCESS );

    IDE_TEST( setCompareFunction( aDataNode ) != IDE_SUCCESS );

    //-------------------------------------------------
    // PROJ-1705
    // QMC_MTR_TYPE_DISK_TABLE 인 경우
    // rid로 레코드 패치시 smiColumnList가 필요함.
    // 여기서 생성된 smiColumnList는 smiFetchRowFromGRID()함수의 인자로 쓰임.
    // (디스크템프테이블은 해당되지 않는다.)
    //-------------------------------------------------

    if( ( ( ( aDataNode->flag & QMC_MTR_TYPE_MASK )  == QMC_MTR_TYPE_DISK_TABLE ) &&
          ( ( aDataNode->flag & QMC_MTR_BASETABLE_TYPE_MASK ) == QMC_MTR_BASETABLE_TYPE_DISKTABLE ) )
        ||
        ( ( aDataNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_DISK_PARTITIONED_TABLE )
        ||
        ( ( aDataNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE )
        ||
        ((aDataNode->flag & QMC_MTR_REMOTE_TABLE_MASK) == QMC_MTR_REMOTE_TABLE_TRUE)
        )
    {
        /*
         * 베이스테이블이 디스크테이블(디스크템프테이블은제외)
         * (QMC_MTR_TYPE_DISK_TABLE, QMC_MTR_BASETABLE_DISKTABLE)인 경우와
         * 베이스테이블이 디스크 파티션드테이블
         * (QMC_MTR_TYPE_DISK_PARTITIONED_TABLE)인 경우
         * 베이스테이블이 Hybrid Partitioned Table
         * (QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE)인 경우 (PROJ-2464 hybrid partitioned table 지원)
         * Remote Table 인 경우 (BUG-39837)
         */

        IDE_TEST( qdbCommon::makeFetchColumnList4TupleID(
                      aTemplate,
                      aDataNode->srcNode->node.table,
                      ID_FALSE, // aIsNeedAllFetchColumn
                      NULL,     // index 정보
                      ID_TRUE,  // aIsAllocSmiColumnList
                      & aDataNode->fetchColumnList ) != IDE_SUCCESS );
    }
    else
    {
        aDataNode->fetchColumnList = NULL;
    }

    // tmplate 추가
    aDataNode->tmplate = & aTemplate->tmplate;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::setFunctionPointer( qmdMtrNode * aDataNode )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Node의 종류에 따라 해당 함수 Pointer를 결정한다.
 *
 * Implementation :
 *
 *    해당 Column의 종류에 따라 함수 pointer를 Setting한다.
 *
 ***********************************************************************/

    // Column 종류에 따른 Function Pointer 설정

    switch ( aDataNode->flag & QMC_MTR_TYPE_MASK )
    {
        //-----------------------------------------------------------
        // Base Table의 종류에 대한 처리
        //-----------------------------------------------------------

        case QMC_MTR_TYPE_MEMORY_TABLE :
        {
            //-----------------------------------
            // Memory Base Table의 경우
            //-----------------------------------

            aDataNode->func.setMtr   = qmc::setMtrByPointer;
            aDataNode->func.getHash  = qmc::getHashNA;   // N/A
            aDataNode->func.isNull   = qmc::isNullNA;    // N/A
            aDataNode->func.makeNull = qmc::makeNullNothing;
            aDataNode->func.getRow   = qmc::getRowNA;    // N/A

            if ((aDataNode->flag & QMC_MTR_RECOVER_RID_MASK) ==
                QMC_MTR_RECOVER_RID_TRUE)
            {
                /*
                 * select target 에 _prowid 가 있음
                 * => rid 복원이 필요
                 */
                aDataNode->func.setTuple = qmc::setTupleByPointer4Rid;
            }
            else
            {
                aDataNode->func.setTuple = qmc::setTupleByPointer;
            }
            break;
        }
        case QMC_MTR_TYPE_DISK_TABLE :
        {
            //-----------------------------------
            // Disk Base Table의 경우
            //-----------------------------------

            aDataNode->func.setMtr   = qmc::setMtrByRID;
            aDataNode->func.getHash  = qmc::getHashNA;   // N/A
            aDataNode->func.isNull   = qmc::isNullNA;    // N/A
            aDataNode->func.makeNull = qmc::makeNullNothing;
            aDataNode->func.getRow   = qmc::getRowNA;    // N/A
            aDataNode->func.setTuple = qmc::setTupleByRID;

            break;
        }
        // PROJ-1502 PARTITIONED DISK TABLE
        // partitioned table은 각 row마다 맞는 tuple id를 저장하여야 한다.
        case QMC_MTR_TYPE_MEMORY_PARTITIONED_TABLE :
        {
            //-----------------------------------
            // Memory Partitioned Table의 경우
            //-----------------------------------

            aDataNode->func.setMtr   = qmc::setMtrByPointerAndTupleID;
            aDataNode->func.getHash  = qmc::getHashNA;   // N/A
            aDataNode->func.isNull   = qmc::isNullNA;    // N/A
            aDataNode->func.makeNull = qmc::makeNullNothing;
            aDataNode->func.getRow   = qmc::getRowNA;    // N/A
            aDataNode->func.setTuple = qmc::setTupleByPointerAndTupleID;

            break;
        }
        case QMC_MTR_TYPE_DISK_PARTITIONED_TABLE :
        {
            //-----------------------------------
            // Disk Partitioned Table의 경우
            //-----------------------------------

            aDataNode->func.setMtr   = qmc::setMtrByRIDAndTupleID;
            aDataNode->func.getHash  = qmc::getHashNA;   // N/A
            aDataNode->func.isNull   = qmc::isNullNA;    // N/A
            aDataNode->func.makeNull = qmc::makeNullNothing;
            aDataNode->func.getRow   = qmc::getRowNA;    // N/A
            aDataNode->func.setTuple = qmc::setTupleByRIDAndTupleID;

            break;
        }
        case QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE : /* PROJ-2464 hybrid partitioned table 지원 */
        {
            //-----------------------------------
            // Hybrid Partitioned Table의 경우
            //-----------------------------------

            aDataNode->func.setMtr   = qmc::setMtrByPointerOrRIDAndTupleID;
            aDataNode->func.getHash  = qmc::getHashNA;   // N/A
            aDataNode->func.isNull   = qmc::isNullNA;    // N/A
            aDataNode->func.makeNull = qmc::makeNullNothing;
            aDataNode->func.getRow   = qmc::getRowNA;    // N/A
            aDataNode->func.setTuple = qmc::setTupleByPointerOrRIDAndTupleID;

            break;
        }

        //-----------------------------------------------------------
        // Column 에 대한 처리
        //-----------------------------------------------------------

        case QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN :
        {
            //-----------------------------------
            // Memory Column인 경우
            //-----------------------------------

            IDE_TEST( setFunction4MemoryPartitionColumn( aDataNode ) != IDE_SUCCESS );

            break;
        }

        case QMC_MTR_TYPE_MEMORY_KEY_COLUMN :
        {
            //-----------------------------------
            // Memory Column인 경우
            //-----------------------------------

            IDE_TEST( setFunction4MemoryColumn( aDataNode ) != IDE_SUCCESS );

            break;
        }

        case QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN : /* PROJ-2464 hybrid partitioned table 지원 */
        {
            aDataNode->func.setMtr   = qmc::setMtrByCopyOrConvert;
            aDataNode->func.getHash  = qmc::getHashByValue;
            aDataNode->func.isNull   = qmc::isNullByValue;
            aDataNode->func.makeNull = qmc::makeNullValue;
            aDataNode->func.getRow   = qmc::getRowByValue;
            aDataNode->func.setTuple = qmc::setTupleByValue;
            break;
        }

        case QMC_MTR_TYPE_COPY_VALUE :
        {
            aDataNode->func.setMtr   = qmc::setMtrByCopy;
            aDataNode->func.getHash  = qmc::getHashByValue;
            aDataNode->func.isNull   = qmc::isNullByValue;
            aDataNode->func.makeNull = qmc::makeNullValue;
            aDataNode->func.getRow   = qmc::getRowByValue;
            aDataNode->func.setTuple = qmc::setTupleByValue;
            break;
        }
        case QMC_MTR_TYPE_CALCULATE :
        {
            aDataNode->func.setMtr   = qmc::setMtrByValue;
            aDataNode->func.getHash  = qmc::getHashByValue;
            aDataNode->func.isNull   = qmc::isNullByValue;
            aDataNode->func.makeNull = qmc::makeNullValue;
            aDataNode->func.getRow   = qmc::getRowByValue;
            aDataNode->func.setTuple = qmc::setTupleByValue;
            break;
        }
        case QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE :
        {
            aDataNode->func.setMtr   = qmc::setMtrByConvert;
            aDataNode->func.getHash  = qmc::getHashByValue;
            aDataNode->func.isNull   = qmc::isNullByValue;
            aDataNode->func.makeNull = qmc::makeNullValue;
            aDataNode->func.getRow   = qmc::getRowByValue;
            aDataNode->func.setTuple = qmc::setTupleByValue;
            break;
        }
        case QMC_MTR_TYPE_USELESS_COLUMN :
        {
            /***********************************************************
             * PROJ-2469 Optimize View Materialization
             * View Materialization의 MtrNode 중
             * 상위 Query Block에서 사용 되지 않는 Column Node는
             * Dummy 취급한다.
             ***********************************************************/
            aDataNode->func.setMtr   = qmc::setMtrByNull;
            aDataNode->func.getHash  = qmc::getHashNA;   // N/A
            aDataNode->func.isNull   = qmc::isNullNA;    // N/A
            aDataNode->func.makeNull = qmc::makeNullNothing;             
            aDataNode->func.getRow   = qmc::getRowNA;    // N/A
            aDataNode->func.setTuple = qmc::setTupleByNull;
            break;
        }            
        default :
        {
            // Materialized Column의 종류가 지정되어 있어야 함.
            IDE_DASSERT(0);

            break;
        }
    } // end of switch

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::setCompareFunction( qmdMtrNode * aDataNode )
{
/***********************************************************************
 *
 * Description :
 *    Materialized Node의 비교 함수 및 비교 대상 Column을 지정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    //----------------------------------------------------
    // Column의 Sorting Order에 따른 비교 함수 결정
    // - Asc/Desc에 따라 비교 함수를 결정한다.
    // - 값의 저장/Pointer의 저장에 따라 대상 Column을 결정한다.
    //----------------------------------------------------
    UInt    sCompareType;

    // 비교 대상 Column의 결정
    // fix BUG-10114
    // disk variable column인 경우, 저장 공간의 낭비를 줄이기 위해,
    // mtrRow 구성시, smiVarColumn의 header만 저장하게 된다.
    // 이 경우도 pointer가 아닌, 값이 저장되는 경우이므로,
    // dstColumn이 compareColumn이 되도록 지정해야 한다.
    //
    // PROJ-2469 Optimize View Materialization
    // QMC_MTR_TYPE_USELESS_COLUMN Type일 경우 dstColumn이 compareColumn이 되도록 지정해야 한다.
    if ( aDataNode->func.getRow == qmc::getRowByValue )
    {
        // 값 자체가 저장되는 경우
        aDataNode->func.compareColumn = aDataNode->dstColumn;
    }
    else
    {
        // 값이 아닌 Pointer가 저장되는 경우
        aDataNode->func.compareColumn = aDataNode->srcColumn;
    }

    // PROJ-2362 memory temp 저장 효율성 개선
    // TEMP_TYPE인 경우 OFFSET_USELESS로 비교한다.
    if ( SMI_COLUMN_TYPE_IS_TEMP( aDataNode->func.compareColumn->column.flag ) == ID_TRUE )
    {
        if ( ( aDataNode->flag & QMC_MTR_SORT_ORDER_MASK )
             == QMC_MTR_SORT_ASCENDING )
        {
            aDataNode->func.compare =
                aDataNode->func.compareColumn->module->logicalCompare
                [MTD_COMPARE_ASCENDING];
        }
        else
        {
            aDataNode->func.compare =
                aDataNode->func.compareColumn->module->logicalCompare
                [MTD_COMPARE_DESCENDING];
        }
    }
    else
    {
        // BUG-38492 Compressed Column's Key Compare Function
        if ( (aDataNode->func.compareColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK )
             == SMI_COLUMN_COMPRESSION_TRUE )
        {
            sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
        }
        else
        {
            if ( (aDataNode->func.compareColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                 == SMI_COLUMN_TYPE_FIXED )
            {
                sCompareType = MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL;
            }
            else
            {
                sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
            }
        }

        // 비교 함수의 결정
        if ( ( aDataNode->flag & QMC_MTR_SORT_ORDER_MASK )
             == QMC_MTR_SORT_ASCENDING )
        {
            aDataNode->func.compare =
                aDataNode->func.compareColumn->module->keyCompare[sCompareType]
                [MTD_COMPARE_ASCENDING];
        }
        else
        {
            aDataNode->func.compare =
                aDataNode->func.compareColumn->module->keyCompare[sCompareType]
                [MTD_COMPARE_DESCENDING];
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmc::setFunction4MemoryColumn( qmdMtrNode * aDataNode )
{
/***********************************************************************
 *
 * Description :
 *    Memory Column을 위한 함수 포인터를 결정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aDataNode->dstTuple->lflag & MTC_TUPLE_STORAGE_MASK)
         == MTC_TUPLE_STORAGE_MEMORY )
    {
        //----------------------------------
        // Memory Temp Table에 저장될 경우
        //----------------------------------

        // Fixed/Variable에 관계 없이 Pointer만 저장함.
        aDataNode->func.setMtr   = qmc::setMtrByPointer;
        aDataNode->func.getHash  = qmc::getHashByPointer;
        aDataNode->func.isNull   = qmc::isNullByPointer;
        aDataNode->func.makeNull = qmc::makeNullNothing;
        aDataNode->func.getRow   = qmc::getRowByPointer;
        aDataNode->func.setTuple = qmc::setTupleByPointer;
    }
    else
    {
        //----------------------------------
        // Disk Temp Table에 저장될 경우
        //----------------------------------

        if ( (aDataNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_FIXED )
        {
            // Fixed Memory Column일 경우
            aDataNode->func.setMtr   = qmc::setMtrByCopy;
            aDataNode->func.getHash  = qmc::getHashByValue;
            aDataNode->func.isNull   = qmc::isNullByValue;
            aDataNode->func.makeNull = qmc::makeNullNA;
            aDataNode->func.getRow   = qmc::getRowByValue;
            aDataNode->func.setTuple = qmc::setTupleByValue;
        }
        else
        {
            // Variable Memory Column일 경우
            aDataNode->func.setMtr   = qmc::setMtrByConvert;
            aDataNode->func.getHash  = qmc::getHashByValue;
            aDataNode->func.isNull   = qmc::isNullByValue;
            aDataNode->func.makeNull = qmc::makeNullNA;
            aDataNode->func.getRow   = qmc::getRowByValue;
            aDataNode->func.setTuple = qmc::setTupleByValue;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmc::setFunction4MemoryPartitionColumn( qmdMtrNode * aDataNode )
{
/***********************************************************************
 *
 * Description :
 *    Memory Column을 위한 함수 포인터를 결정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aDataNode->dstTuple->lflag & MTC_TUPLE_STORAGE_MASK)
         == MTC_TUPLE_STORAGE_MEMORY )
    {
        //----------------------------------
        // Memory Temp Table에 저장될 경우
        //----------------------------------

        // Fixed/Variable에 관계 없이 Partition Tuple ID와 Pointer를 저장함.
        aDataNode->func.setMtr   = qmc::setMtrByPointerAndTupleID;
        aDataNode->func.getHash  = qmc::getHashByPointerAndTupleID;
        aDataNode->func.isNull   = qmc::isNullByPointerAndTupleID;
        aDataNode->func.makeNull = qmc::makeNullNothing;
        aDataNode->func.getRow   = qmc::getRowByPointerAndTupleID;
        aDataNode->func.setTuple = qmc::setTupleByPointerAndTupleID;
    }
    else
    {
        //----------------------------------
        // Disk Temp Table에 저장될 경우
        //----------------------------------

        IDE_TEST( setFunction4MemoryColumn( aDataNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::refineOffset4MemoryColumn( qmdMtrNode * aMtrList,
                                qmdMtrNode * aColumnNode,
                                UInt       * aOffset )
{
/***********************************************************************
 *
 * Description :
 *     Memory Column이 Temp Table에 저장될 경우의 offset을 결정한다.
 *
 * Implementation :
 *     Memory Temp Table에 저장되는 경우와 Disk Temp Table에 저장되는
 *     경우를 구분하여 처리하여야 한다.
 *         - Memory Temp Table에 저장되는 경우 pointer를 중복 저장하지
 *           않도록 하며,
 *         - Disk Temp Table에 저장되는 경우는 값 자체를 저장할 수 있도록
 *           하여야 한다.
 *
 ***********************************************************************/

    qmdMtrNode * sFindNode;

    // 적합성 검사
    IDE_DASSERT( aMtrList != NULL );
    IDE_DASSERT( aColumnNode != NULL );
    IDE_DASSERT( aOffset != NULL );

    if ( (aColumnNode->dstTuple->lflag & MTC_TUPLE_STORAGE_MASK)
         == MTC_TUPLE_STORAGE_MEMORY )
    {
        //----------------------------------
        // Memory Temp Table에 저장될 경우
        //----------------------------------
        // 이미 해당 Pointer를 관리하는 Node가 있을 경우,
        // 별도의 공간을 할당받지 않고, 기존의 공간을 그대로
        // 사용한다.

        // 동일한 Node가 있는 지 찾는다.

        for ( sFindNode = aMtrList;
              sFindNode != aColumnNode;
              sFindNode = sFindNode->next )
        {
            if ( ( aColumnNode->srcTuple == sFindNode->srcTuple ) &&
                 ( ( ( sFindNode->flag & QMC_MTR_TYPE_MASK )
                     == QMC_MTR_TYPE_MEMORY_TABLE )
                   ||
                   ( ( sFindNode->flag & QMC_MTR_TYPE_MASK )
                     == QMC_MTR_TYPE_MEMORY_KEY_COLUMN ) ) )
            {
                if( aColumnNode->srcNode->node.conversion == NULL )
                {
                    // 동일한 Source Tuple을 갖고
                    // 찾은 노드가 Memory Base Table이거나,
                    // Memory Column인 경우에 동일한 저장 영역을
                    // 사용할 수 있다.
                    break;
                }
                else
                {
                    // jhseong, BUG-8737, if conversion exists, passNode is used.
                    continue;
                }
            }
            else
            {
                // nothing to do
            }
        }
        if ( aColumnNode == sFindNode )
        {
            // 동일한 Node를 찾지 못한 경우
            // 32/64bit pointer를 저장할 공간을 할당
            // RID와 동일한 공간 영역을 할당함.
            *aOffset = idlOS::align8( *aOffset );

            aColumnNode->dstColumn->column.offset = *aOffset;
            aColumnNode->dstColumn->column.size = ID_SIZEOF(scGRID);

            *aOffset += ID_SIZEOF(scGRID);
        }
        else
        {
            // 동일한 Node를 찾은 경우
            // 찾은 Node의 공간을 사용한다.
            aColumnNode->dstColumn->column.offset =
                sFindNode->dstColumn->column.offset;
            aColumnNode->dstColumn->column.size = ID_SIZEOF(scGRID);

            // offset의 변경 필요 없음.
        }

        // pointer만 저장되므로 fixed로 설정한다.
        aColumnNode->dstColumn->column.flag &= ~SMI_COLUMN_TYPE_MASK;
        aColumnNode->dstColumn->column.flag |= SMI_COLUMN_TYPE_FIXED;

        // BUG-38494
        // Compressed Column 역시 값 자체가 저장되므로
        // Compressed 속성을 삭제한다
        aColumnNode->dstColumn->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
        aColumnNode->dstColumn->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;
    }
    else
    {
        //----------------------------------
        // Disk Temp Table에 저장될 경우
        //----------------------------------

        // BUG-38592
        // Temp Table에는 실제 값이 들어오므로,
        // Fixed/Variable에 상관없이 Compressed 속성을 삭제한다
        aColumnNode->dstColumn->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
        aColumnNode->dstColumn->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

        if ( (aColumnNode->dstColumn->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_FIXED )
        {
            // Fixed Memory Column일 경우
        }
        else
        {
            // Variable Memory Column일 경우

            // To Fix PR-8367
            // mtcColumn이 아닌 smiColumn을 사용해야 함.
            // Memory Variable Column의 경우 Disk Temp Table에
            // 저장될 때 Data 자체를 저장하여야 함.
            aColumnNode->dstColumn->column.flag &= ~SMI_COLUMN_TYPE_MASK;
            aColumnNode->dstColumn->column.flag |= SMI_COLUMN_TYPE_FIXED;
        }

        // Offset만 결정하면 된다.
        *aOffset = idlOS::align( *aOffset,
                                 aColumnNode->dstColumn->module->align );
        aColumnNode->dstColumn->column.offset = *aOffset;

        *aOffset += aColumnNode->dstColumn->column.size;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmc::refineOffset4MemoryPartitionColumn( qmdMtrNode * aMtrList,
                                         qmdMtrNode * aColumnNode,
                                         UInt       * aOffset )
{
/***********************************************************************
 *
 * Description :
 *     Memory Column이 Temp Table에 저장될 경우의 offset을 결정한다.
 *
 * Implementation :
 *     Memory Temp Table에 저장되는 경우와 Disk Temp Table에 저장되는
 *     경우를 구분하여 처리하여야 한다.
 *         - Memory Temp Table에 저장되는 경우 pointer를 중복 저장하지
 *           않도록 하며,
 *         - Disk Temp Table에 저장되는 경우는 값 자체를 저장할 수 있도록
 *           하여야 한다.
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( aMtrList != NULL );
    IDE_DASSERT( aColumnNode != NULL );
    IDE_DASSERT( aOffset != NULL );

    if ( (aColumnNode->dstTuple->lflag & MTC_TUPLE_STORAGE_MASK)
         == MTC_TUPLE_STORAGE_MEMORY )
    {
        //----------------------------------
        // Memory Temp Table에 저장될 경우
        //----------------------------------
        
        // BUG-38309
        // 이제는 16byte보다 커서 mtdByte를 사용한다.
        IDE_TEST( mtc::initializeColumn( aColumnNode->dstColumn,
                                         & mtdByte,
                                         1,
                                         ID_SIZEOF(qmcMemPartRowInfo),
                                         0 )
                  != IDE_SUCCESS );

        // fix BUG-10243
        *aOffset = idlOS::align( *aOffset,
                                 aColumnNode->dstColumn->module->align );

        aColumnNode->dstColumn->column.offset = *aOffset;

        *aOffset += aColumnNode->dstColumn->column.size;
    }
    else
    {
        //----------------------------------
        // Disk Temp Table에 저장될 경우
        //----------------------------------

        IDE_TEST( refineOffset4MemoryColumn( aMtrList, aColumnNode, aOffset )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::findAttribute( qcStatement  * aStatement,
                    qmcAttrDesc  * aResult,
                    qtcNode      * aExpr,
                    idBool         aMakePassNode,
                    qmcAttrDesc ** aAttr )
{
/***********************************************************************
 *
 * Description :
 *    주어진 result descriptor에서 특정한 attribute를 찾아 반환한다.
 *
 * Implementation :
 *     Result descriptor의 각 attribute들을 순차 탐색하며 같은
 *     expression인지 확인한다.
 *     qtc::isEquivalentExpression의 결과 외 conversion까지 같아야
 *     같은 것으로 간주한다.
 *
 ***********************************************************************/

    mtcNode * sExpr1;
    mtcNode * sExpr2;
    mtcNode * sConverted1;
    mtcNode * sConverted2;
    qmcAttrDesc * sItrAttr;
    idBool    sIsSame;

    IDU_FIT_POINT_FATAL( "qmc::findAttribute::__FT__" );

    sExpr1 = &aExpr->node;

    while( sExpr1->module == &qtc::passModule )
    {
        sExpr1 = sExpr1->arguments;
    }

    for( sItrAttr = aResult;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        sExpr2 = &sItrAttr->expr->node;

        if( sExpr1 == sExpr2 )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                               (qtcNode *)sExpr1,
                                               (qtcNode *)sExpr2,
                                               &sIsSame )
                  != IDE_SUCCESS );
        if( sIsSame == ID_TRUE )
        {
            sConverted1 = mtf::convertedNode(sExpr1, NULL);
            sConverted2 = mtf::convertedNode(sExpr2, NULL);

            // 최종 conversion된 결과까지 같아야 같은 expression으로 간주한다.
            if ( QTC_STMT_COLUMN( aStatement, (qtcNode*)sConverted1 )->module ==
                 QTC_STMT_COLUMN( aStatement, (qtcNode*)sConverted2 )->module )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            // BUG-39875
            // UNION 을 사용시에는 select 의 target 에 컨버젼노드가 달릴수 있다.
            // 이때문에 aMakePassNode 가 TRUE 이면 컨버젼이 있어도 같다고 판단해야 한다.
            // qmc::makeReference 함수에서 컨버젼을 연결해준다.
            if( aMakePassNode == ID_TRUE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    *aAttr = sItrAttr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::makeReference( qcStatement  * aStatement,
                    idBool         aMakePassNode,
                    qtcNode      * aRefNode,
                    qtcNode     ** aDepNode )
{
/***********************************************************************
 *
 * Description :
 *    aDepNode가 aRefNode를 참조하도록 관계를 설정한다.
 *    (두 node가 같은 column 또는 expression임을 가정한다)
 *
 * Implementation :
 *    - aDepNode가 pass node라면 argument로 aRefNode를 가리킨다.
 *    - aDepNode가 pass node가 아닌 경우
 *      - 만약 aMakePassNode가 true이며 aDepNode가 expression이라면
 *        aRefNode를 가리키는 pass node를 생성하여 aDepNode에 설정한다.
 *      - 그 외의 경우 tuple-set 내의 위치만 동일하게 설정한다.
 *
 ***********************************************************************/

    qtcNode * sPassNode;

    IDU_FIT_POINT_FATAL( "qmc::makeReference::__FT__" );

    if( (*aDepNode)->node.module == &qtc::passModule )
    {
        // Parent의 node가 pass node인 생성된 경우
        (*aDepNode)->node.arguments = &aRefNode->node;
    }
    else
    {
        // BUG-36997
        // _prowid 는 연산을 수행하지 않고 column,value 와 같이 읽기만 하면된다.
        // 따라서 PASS 노드를 생성할 필요가 없다.
        if( ( aMakePassNode == ID_TRUE ) &&
            ( QMC_NEED_CALC( *aDepNode ) == ID_TRUE ) &&
            ( (*aDepNode)->node.module != &gQtcRidModule ) )
        {
            // Pass node를 생성할 필요가 있는 경우
            if( ( (*aDepNode)->node.module == &qtc::subqueryModule ) &&
                ( (*aDepNode) == aRefNode ) )
            {
                // Nothing to do.
                // Subquery의 경우 materialize된 후에 value node로 변경되므로
                // pass node를 설정하지 않아도 된다.
            }
            else
            {
                IDE_TEST( qtc::makePassNode( aStatement,
                                             NULL,
                                             aRefNode,
                                             &sPassNode )
                          != IDE_SUCCESS );

                // BUG-39875
                // PASS 노드를 만들때 기존의 컨버젼을 유지해준다.
                // 이유는 UNION 을 사용하면 target 에 컨버젼이 존재할수 있기 때문이다.
                sPassNode->node.conversion = ((*aDepNode)->node).conversion;
                sPassNode->node.next       = ((*aDepNode)->node).next;

                *aDepNode = sPassNode;
            }
        }
        else
        {
            // Pass node를 생성할 필요가 없는 경우
            (*aDepNode)->node.table  = aRefNode->node.table;
            (*aDepNode)->node.column = aRefNode->node.column;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::appendPredicate( qcStatement  * aStatement,
                      qmsQuerySet  * aQuerySet,
                      qmcAttrDesc ** aResult,
                      qmoPredicate * aPredicate,
                      UInt           aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Result descriptor에 predicate에 포함된 attribute들을 추가한다.
 *
 * Implementation :
 *    qmoPredicate에 next와 more를 순회하며 포함된 expression들을 추가한다.
 *
 ***********************************************************************/

    const qmoPredicate * sCurrent;

    IDU_FIT_POINT_FATAL( "qmc::appendPredicate::__FT__" );

    for( sCurrent = aPredicate;
         sCurrent != NULL;
         sCurrent = sCurrent->next )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        aResult,
                                        sCurrent->node,
                                        aFlag )
                  != IDE_SUCCESS );
        if( sCurrent->more != NULL )
        {
            IDE_TEST( qmc::appendPredicate( aStatement,
                                            aQuerySet,
                                            aResult,
                                            sCurrent->more,
                                            aFlag )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::appendPredicate( qcStatement  * aStatement,
                      qmsQuerySet  * aQuerySet,
                      qmcAttrDesc ** aResult,
                      qtcNode      * aPredicate,
                      UInt           aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Result descriptor에 predicate에 포함된 attribute들을 추가한다.
 *
 * Implementation :
 *    - 논리연산자나 비교연산자인 경우 argument들로 재귀호출한다.
 *    - 그렇지 않은 경우 appendAttribute를 호출한다.
 *
 ***********************************************************************/

    const mtcNode * sArg;

    IDU_FIT_POINT_FATAL( "qmc::appendPredicate::__FT__" );

    if( ( ( aPredicate->node.module->lflag & MTC_NODE_COMPARISON_MASK )
          == MTC_NODE_COMPARISON_TRUE ) ||
        ( ( aPredicate->node.module->lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
          == MTC_NODE_LOGICAL_CONDITION_TRUE ) ||
        ( ( aPredicate->node.module->lflag & MTC_NODE_OPERATOR_MASK )
          == MTC_NODE_OPERATOR_LIST ) )
    {
        // 논리연산자 또는 비교 연산자인 경우
        for( sArg = aPredicate->node.arguments;
             sArg != NULL;
             sArg = sArg->next )
        {
            IDE_TEST( qmc::appendPredicate( aStatement,
                                            aQuerySet,
                                            aResult,
                                            (qtcNode *)sArg,
                                            aFlag )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* BUG-39611 The sys_connect_by_path function support expression
         * argument.
         * SYS_CONNEC_BY_PATH가 사용된 경우 전체를 result descript에 
         * 등록해줘야 나중에 CNBY Plan생성시이미 생성된 Arguments의
         * Tuple을 변경할 수 있다.
         */
        if ( ( aPredicate->node.lflag & MTC_NODE_FUNCTION_CONNECT_BY_MASK )
             == MTC_NODE_FUNCTION_CONNECT_BY_TRUE )
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            aResult,
                                            aPredicate,
                                            QMC_ATTR_SEALED_TRUE,
                                            QMC_APPEND_EXPRESSION_TRUE,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmc::appendAttribute( aStatement,
                                            aQuerySet,
                                            aResult,
                                            aPredicate,
                                            aFlag,
                                            0,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::appendAttribute( qcStatement  * aStatement,
                      qmsQuerySet  * aQuerySet,
                      qmcAttrDesc ** aResult,
                      qtcNode      * aExpr,
                      UInt           aFlag,
                      UInt           aAppendOption,
                      idBool         aMakePassNode )
{
/***********************************************************************
 *
 * Description :
 *    Result descriptor에 정해진 option에 따라 attribute를 추가한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcAttrDesc   * sAttrs;
    qmcAttrDesc   * sNewAttr;
    qmcAttrDesc   * sItrAttr;
    qtcNode       * sCopiedNode;
    mtcNode       * sArg;
    mtcNode       * sExpr;
    idBool          sExist = ID_FALSE;
    UInt            sFlag1;
    UInt            sFlag2;
    UInt            sMask;

    IDU_FIT_POINT_FATAL( "qmc::appendAttribute::__FT__" );

    sExpr = &aExpr->node;

    /*BUG-44649
      조인 메소드에 따라서 level 컬럼을 사용하는 질의의 결과값이 달라질 수 있습니다. 
      result descriptor에서 level 정보는 hierarchy 구문이 없는 경우 하위로 내려 주면 안 됩니다.*/
    if( (aQuerySet->SFWGH != NULL) && (sExpr->arguments == NULL) )
    {
        if( aQuerySet->SFWGH->hierarchy == NULL )
        {
            if( ((aExpr->lflag & QTC_NODE_LEVEL_MASK) == QTC_NODE_LEVEL_EXIST) ||
                ((aExpr->lflag & QTC_NODE_ISLEAF_MASK) == QTC_NODE_ISLEAF_EXIST) )
            {
                aFlag &= ~QMC_ATTR_TERMINAL_MASK;
                aFlag |= QMC_ATTR_TERMINAL_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    while( sExpr->module == &qtc::passModule )
    {
        sExpr = sExpr->arguments;

        // Pass node의 경우 expression을 가리키더라도 최종 결과를 하나의 attribute로 본다.
        aAppendOption &= ~QMC_APPEND_EXPRESSION_MASK;
        aAppendOption |= QMC_APPEND_EXPRESSION_TRUE;

        // Pass node의 경우 calculate할 필요 없이 원본을 참조하면 된다.
        aFlag &= ~QMC_ATTR_SEALED_MASK;
        aFlag |= QMC_ATTR_SEALED_TRUE;
    }

    if( ( aFlag & QMC_ATTR_SEALED_MASK ) == QMC_ATTR_SEALED_TRUE )
    {
        // Expression이 아닌 경우 sealed flag를 해제한다.
        if( ( sExpr->module == &qtc::columnModule ) ||
            ( sExpr->module == &qtc::valueModule ) )
        {
            aFlag &= ~QMC_ATTR_SEALED_MASK;
            aFlag |= QMC_ATTR_SEALED_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if( ( aAppendOption & QMC_APPEND_ALLOW_DUP_MASK ) == QMC_APPEND_ALLOW_DUP_FALSE )
    {
        // 중복을 허용하도록 flag가 설정되지 않았다면 이미 존재하는 attribute인지 확인
        IDE_TEST( findAttribute( aStatement,
                                 *aResult,
                                 (qtcNode *)sExpr,
                                 ID_FALSE,
                                 &sAttrs )
                  != IDE_SUCCESS );
        if( sAttrs == NULL )
        {
            sExist = ID_FALSE;
        }
        else
        {
            if( ( aAppendOption & QMC_APPEND_CHECK_ANALYTIC_MASK ) == QMC_APPEND_CHECK_ANALYTIC_TRUE )
            {
                // Analytic function의 analytic절 확인
                // ex) RANK() OVER (ORDER BY c1, c2)
                //     에서 c1, c2의 경우 정렬 여부와 정렬 방향이 같아야 한다.
                //     Window sorting에서만 이 flag를 설정한다.
                // BUG-42145 NULLS ORDER 역시 같아야한다.
                sMask = (QMC_ATTR_ANALYTIC_SORT_MASK | QMC_ATTR_SORT_ORDER_MASK | QMC_ATTR_SORT_NULLS_ORDER_MASK);
                sFlag1 = (sAttrs->flag & sMask);
                sFlag2 = (aFlag & sMask);

                if( sFlag1 == sFlag2 )
                {
                    sExist = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                sExist = ID_TRUE;
            }

            if( sExist == ID_TRUE )
            {
                // 이미 존재하는 경우 flag를 병합한다.
                sMask = (QMC_ATTR_SEALED_MASK | QMC_ATTR_DISTINCT_MASK | QMC_ATTR_ORDER_BY_MASK);
                sAttrs->flag |= (aFlag & sMask);

                if( &sAttrs->expr->node != sExpr )
                {
                    sCopiedNode = (qtcNode *)sExpr;

                    IDE_TEST( makeReference( aStatement,
                                             aMakePassNode,
                                             sAttrs->expr,
                                             (qtcNode**)&sExpr )
                              != IDE_SUCCESS );
             
                    // BUG-46424
                    if ( sExpr->module == &qtc::passModule )
                    {
                        for ( sArg = sCopiedNode->node.arguments;
                              sArg != NULL;
                              sArg = sArg->next )
                        {
                            IDE_TEST( appendAttribute( aStatement,
                                                       aQuerySet,
                                                       aResult,
                                                       (qtcNode *)sArg,
                                                       0,
                                                       0,
                                                       ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    if( sExist == ID_FALSE )
    {
        // 존재하지 않는 경우 새로 추가한다.

        if( ( ( aFlag & QMC_ATTR_CONVERSION_MASK ) == QMC_ATTR_CONVERSION_FALSE ) &&
            ( sExpr->conversion != NULL ) )
        {
            // Conversion flag가 false이면서 conversion이 존재하는 경우
            // conversion이 제거된 node를 추가한다.
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                       (void **)&sCopiedNode )
                      != IDE_SUCCESS );

            idlOS::memcpy( sCopiedNode, sExpr, ID_SIZEOF( qtcNode ) );
            sCopiedNode->node.conversion = NULL;

            IDE_TEST( appendAttribute( aStatement,
                                       aQuerySet,
                                       aResult,
                                       sCopiedNode,
                                       aFlag,
                                       aAppendOption,
                                       aMakePassNode )
                      != IDE_SUCCESS );

            sExpr->table  = sCopiedNode->node.table;
            sExpr->column = sCopiedNode->node.column;

            // BUG-45320 변경된 table column값으로 바꾸기위해 flag를 세팅한다. 
            sExpr->lflag &= ~MTC_NODE_COLUMN_LOCATE_CHANGE_MASK;
            sExpr->lflag |= MTC_NODE_COLUMN_LOCATE_CHANGE_TRUE;
        }
        else
        {
            if( ( QMC_NEED_CALC( (qtcNode*)sExpr ) == ID_FALSE ) ||
                ( ( aAppendOption & QMC_APPEND_EXPRESSION_MASK ) == QMC_APPEND_EXPRESSION_TRUE ) )
            {
                /*
                 * 다음에 해당하는 경우 하위 node를 순회하지 않고 바로 추가한다.
                 *
                 *  1. 별도의 evaluation이 필요하지 않은 경우
                 *     Column, value, aggregate function의 경우 결과를 바로 참조하면 된다.
                 *
                 *  2. Expression flag가 설정된 경우
                 *     sorting key, grouping key등은 expression 자체의 결과를 출력해야 한다.
                 *     반면 sorting key가 아닌 column을 select절에서 사용하는 경우에는 하위
                 *     node들을 탐색하여 전달할 column들을 수집해야 한다.
                 *
                 *     ex) SELECT c1 * c2, c2 * c3 FROM t1 ORDER BY c1 * c2;
                 *         => PROJ -- [c1 * c2], [c2 * c3]
                 *            SORT -- #[c1 * c2], [c2], [c3]
                 *            SCAN -- [c1], [c2], [c3]
                 *            + #표시는 sorting key
                 */
                if( ( ( aAppendOption & QMC_APPEND_ALLOW_CONST_MASK ) == QMC_APPEND_ALLOW_CONST_FALSE ) &&
                    ( sExpr->module == &qtc::valueModule ) )
                {
                    // Nothing to do.
                    // 상수나 bind 변수는 별도의 flag가 설정되어있지 않다면 무시한다.
                }
                else if( ( ( (qtcNode *)sExpr)->lflag & QTC_NODE_SEQUENCE_MASK ) == QTC_NODE_SEQUENCE_EXIST )
                {
                    // Nothing to do.
                    // Sequence의 경우 projection에만 존재할 수 있으므로 추가하지 않는다.
                }
                else
                {
                    // Result descriptor에서 마지막 attribute를 찾아 이어붙인다.
                    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                  ID_SIZEOF(qmcAttrDesc),
                                  (void**)&sNewAttr )
                              != IDE_SUCCESS );

                    sNewAttr->expr = (qtcNode*)sExpr;
                    sNewAttr->flag = aFlag;
                    sNewAttr->next = NULL;

                    if( *aResult == NULL )
                    {
                        // Result descriptor가 비어있는 경우
                        *aResult = sNewAttr;
                    }
                    else
                    {
                        // Result descriptor가 비어있지 않은 경우 마지막 node에 이어준다.
                        for( sItrAttr = *aResult;
                             sItrAttr->next != NULL;
                             sItrAttr = sItrAttr->next )
                        {
                        }
                        sItrAttr->next = sNewAttr;
                    }
                }
            }
            else
            {
                if( sExpr->module == &qtc::subqueryModule )
                {
                    IDE_TEST( appendSubqueryCorrelation( aStatement,
                                                         aQuerySet,
                                                         aResult,
                                                         aExpr )
                              != IDE_SUCCESS );

                }
                else
                {
                    for( sArg = sExpr->arguments;
                         sArg != NULL;
                         sArg = sArg->next )
                    {
                        IDE_TEST( appendAttribute( aStatement,
                                                   aQuerySet,
                                                   aResult,
                                                   (qtcNode *)sArg,
                                                   aFlag,
                                                   aAppendOption,
                                                   aMakePassNode )
                                  != IDE_SUCCESS );
                    }
                }
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::createResultFromQuerySet( qcStatement  * aStatement,
                               qmsQuerySet  * aQuerySet,
                               qmcAttrDesc ** aResult )
{
/***********************************************************************
 *
 * Description :
 *    Query-set 자료구조로부터 result descriptor를 생성한다.
 *    다음 두 operator에서 사용한다.
 *    1. PROJ(top projection인 경우)
 *    2. VMTR
 *
 * Implementation :
 *    Query-set에 설정된 target list를 탐색하며 attribute를 추가한다.
 *
 ***********************************************************************/

    UInt                sFlag = 0;
    qmsTarget         * sTarget;
    qmcAttrDesc       * sNewAttr;
    qmcAttrDesc       * sLastAttr = NULL;

    IDU_FIT_POINT_FATAL( "qmc::createResultFromQuerySet::__FT__" );

    sFlag &= ~QMC_ATTR_CONVERSION_MASK;
    sFlag |= QMC_ATTR_CONVERSION_TRUE;

    // Query-set의 최상위 projection인 경우
    for( sTarget = aQuerySet->target;
         sTarget != NULL;
         sTarget = sTarget->next )
    {
        if( aQuerySet->SFWGH != NULL )
        {
            // 일반적인 PROJ 또는 VMTR
            if( ( sTarget->targetColumn->node.module != &qtc::columnModule ) &&
                ( sTarget->targetColumn->node.module != &qtc::valueModule ) )
            {
                // 단순 column이나 상수 외 expression인 경우
                if( ( aQuerySet->SFWGH->selectType == QMS_DISTINCT ) ||
                    ( sTarget->targetColumn->node.module == &qtc::passModule ) ||
                    ( QTC_IS_AGGREGATE( sTarget->targetColumn ) == ID_TRUE ) )
                {
                    // DISTINCT절을 사용한 경우 HSDS에서,
                    // Pass node가 설정되거나 aggregate function을 사용한 경우에는
                    // GRAG/AGGR에서 결과를 전달한다.
                    sFlag &= ~QMC_ATTR_SEALED_MASK;
                    sFlag |= QMC_ATTR_SEALED_TRUE;
                }
                else
                {
                    sFlag &= ~QMC_ATTR_SEALED_MASK;
                    sFlag |= QMC_ATTR_SEALED_FALSE;
                }
            }
            else
            {
                sFlag &= ~QMC_ATTR_SEALED_MASK;
                sFlag |= QMC_ATTR_SEALED_FALSE;
            }

            if( aQuerySet->SFWGH->selectType == QMS_DISTINCT )
            {
                sFlag &= ~QMC_ATTR_DISTINCT_MASK;
                sFlag |= QMC_ATTR_DISTINCT_TRUE;
            }
            else
            {
                sFlag &= ~QMC_ATTR_DISTINCT_MASK;
                sFlag |= QMC_ATTR_DISTINCT_FALSE;
            }
        }
        else
        {
            // UNION 등 SET 연산자의 상위 PROJ
            sFlag &= ~QMC_ATTR_SEALED_MASK;
            sFlag |= QMC_ATTR_SEALED_FALSE;
        }

        if( ( sTarget->flag & QMS_TARGET_ORDER_BY_MASK ) == QMS_TARGET_ORDER_BY_TRUE )
        {
            // ORDER BY절에서 alias나 indicator로 참조하는 경우
            sFlag &= ~QMC_ATTR_ORDER_BY_MASK;
            sFlag |= QMC_ATTR_ORDER_BY_TRUE;

            if( ( sTarget->targetColumn->node.module != &qtc::columnModule ) &&
                ( sTarget->targetColumn->node.module != &qtc::valueModule ) )
            {
                sFlag &= ~QMC_ATTR_SEALED_MASK;
                sFlag |= QMC_ATTR_SEALED_TRUE;
            }
        }
        else
        {
            sFlag &= ~QMC_ATTR_ORDER_BY_MASK;
            sFlag |= QMC_ATTR_ORDER_BY_FALSE;
        }

        // PROJ-2469 Optimize View Materialization
        // 상위 Query Block에서 참조하지 않는 View의 Column에 대해서 flag 처리한다.
        if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) == QMS_TARGET_IS_USELESS_TRUE )
        {
            sFlag &= ~QMC_ATTR_USELESS_RESULT_MASK;
            sFlag |=  QMC_ATTR_USELESS_RESULT_TRUE;
        }
        else
        {
            sFlag &= ~QMC_ATTR_USELESS_RESULT_MASK;
            sFlag |=  QMC_ATTR_USELESS_RESULT_FALSE;
        }
        
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmcAttrDesc ),
                                                   (void**)&sNewAttr )
                  != IDE_SUCCESS );

        sNewAttr->expr = sTarget->targetColumn;
        sNewAttr->flag = sFlag;
        sNewAttr->next = NULL;

        if( sLastAttr == NULL )
        {
            *aResult = sNewAttr;
        }
        else
        {
            sLastAttr->next = sNewAttr;
        }
        sLastAttr = sNewAttr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::copyResultDesc( qcStatement        * aStatement,
                     const qmcAttrDesc  * aParent,
                     qmcAttrDesc       ** aChild )
{
/***********************************************************************
 *
 * Description :
 *    Parent에서 child로 result descriptor를 복사한다.
 *
 * Implementation :
 *    일반적인 linked-list의 복사와 동일하다.
 *
 ***********************************************************************/
    const qmcAttrDesc * sItrAttr;
    qmcAttrDesc       * sNewAttr;
    qmcAttrDesc       * sLastAttr = NULL;

    IDU_FIT_POINT_FATAL( "qmc::copyResultDesc::__FT__" );

    IDE_FT_ASSERT( *aChild == NULL );

    for( sItrAttr = aParent;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(qmcAttrDesc),
                      (void**)&sNewAttr )
                  != IDE_SUCCESS );

        sNewAttr->expr = sItrAttr->expr;
        sNewAttr->flag = sItrAttr->flag;
        sNewAttr->next = NULL;

        if( sLastAttr == NULL )
        {
            *aChild = sNewAttr;
        }
        else
        {
            sLastAttr->next = sNewAttr;
        }
        sLastAttr = sNewAttr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::appendQuerySetCorrelation( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                qmcAttrDesc ** aResult,
                                qmsQuerySet  * aSubquerySet,
                                qcDepInfo    * aDepInfo )
{
/***********************************************************************
 *
 * Description :
 *    Query-set을 재귀적으로 순회하며 correlation attribute들을 추가한다.
 *
 * Implementation :
 *    Set 연산이 아닌 경우 SFWGH에서 outerColumn에서 correlation을 찾는다.
 *
 ***********************************************************************/

    qmsSFWGH     * sSFWGH;
    qmsOuterNode * sOuterNode;
    qcDepInfo      sDepInfo;

    IDU_FIT_POINT_FATAL( "qmc::appendQuerySetCorrelation::__FT__" );

    if( aSubquerySet->setOp == QMS_NONE )
    {
        sSFWGH = aSubquerySet->SFWGH;

        for( sOuterNode = sSFWGH->outerColumns;
             sOuterNode != NULL;
             sOuterNode = sOuterNode->next )
        {
            qtc::dependencyAnd( &sOuterNode->column->depInfo, aDepInfo, &sDepInfo );

            if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
            {
                IDE_TEST( appendAttribute( aStatement,
                                           aQuerySet,
                                           aResult,
                                           sOuterNode->column,
                                           0,
                                           0,
                                           ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        IDE_TEST( appendQuerySetCorrelation( aStatement,
                                             aQuerySet,
                                             aResult,
                                             aSubquerySet->left,
                                             aDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( appendQuerySetCorrelation( aStatement,
                                             aQuerySet,
                                             aResult,
                                             aSubquerySet->right,
                                             aDepInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::appendSubqueryCorrelation( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                qmcAttrDesc ** aResult,
                                qtcNode      * aSubqueryNode )
{
/***********************************************************************
 *
 * Description :
 *    Subquery에 correlation attribute들을 result descriptor에 추가한다.
 *
 * Implementation :
 *    Outer query의 dependency를 구해 전달하면 subquery에서 correlation
 *    여부를 판단한다.
 *
 ***********************************************************************/

    qmsParseTree     * sParseTree;
    qmsQuerySet      * sSubquerySet;
    qcDepInfo        * sDepInfo;

    IDU_FIT_POINT_FATAL( "qmc::appendSubqueryCorrelation::__FT__" );

    IDE_FT_ASSERT( aSubqueryNode->node.module == &qtc::subqueryModule );

    if( aQuerySet->setOp == QMS_NONE )
    {
        sParseTree = (qmsParseTree*)aSubqueryNode->subquery->myPlan->parseTree;
        sSubquerySet = sParseTree->querySet;

        // Outer query의 dependency를 얻어온다.
        sDepInfo = &aQuerySet->SFWGH->depInfo;

        IDE_TEST( appendQuerySetCorrelation( aStatement,
                                             aQuerySet,
                                             aResult,
                                             sSubquerySet,
                                             sDepInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::pushResultDesc( qcStatement  * aStatement,
                     qmsQuerySet  * aQuerySet,
                     idBool         aMakePassNode,
                     qmcAttrDesc  * aParent,
                     qmcAttrDesc ** aChild )
{
/***********************************************************************
 *
 * Description :
 *    Parent에서 child로 result descriptor를 전달한다.
 *    이 과정을 통해 각 operator들마다 push projection이 구현된다.
 *
 * Implementation :
 *    Parent의 각 attribute에 sealed flag가 설정된 경우 expression을
 *    그대로 child에 추가하고, 그렇지 않은 경우 expression의 각 node들을
 *    별개로 child에 추가한다.
 *
 ***********************************************************************/

    UInt sFlag = 0;
    UInt sMask;
    const mtcNode * sArg;
    qmcAttrDesc * sAttr;
    qmcAttrDesc * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmc::pushResultDesc::__FT__" );

    for( sItrAttr = aParent;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        if( ( sItrAttr->flag & QMC_ATTR_TERMINAL_MASK ) == QMC_ATTR_TERMINAL_TRUE )
        {
            continue;
        }
        else
        {
            // Nothing to do.
        }

        if( ( sItrAttr->expr->node.module == &qtc::subqueryModule ) &&
            ( ( sItrAttr->flag & QMC_ATTR_DISTINCT_MASK ) == QMC_ATTR_DISTINCT_FALSE ) &&
            ( ( sItrAttr->flag & QMC_ATTR_ORDER_BY_MASK ) == QMC_ATTR_ORDER_BY_FALSE ) )
        {
            // Subquery는 다음의 조건 중 해당하지 않으면 무시한다.
            // (Subquery는 수행 비용이 높으므로 가능한 PROJ에서 수행 유도)
            // 1. SELECT DISTINCT절에서 사용된 경우(HSDS에서 수행)
            // 2. SELECT절에서 사용되고 ORDER BY절에서 참조된 경우(SORT에서 수행)

            // BUG-35082 subquery와 correlation이 있는 attribute들을 추가해준다.
            IDE_TEST( appendSubqueryCorrelation( aStatement,
                                                 aQuerySet,
                                                 aChild,
                                                 sItrAttr->expr )
                      != IDE_SUCCESS );
            continue;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( findAttribute( aStatement,
                                 *aChild,
                                 sItrAttr->expr,
                                 ID_FALSE,
                                 &sAttr )
                  != IDE_SUCCESS );

        if( sAttr != NULL )
        {
            // Result descriptor에 이미 존재하는 경우 참조관계를 만든다.
            IDE_TEST( makeReference( aStatement,
                                     aMakePassNode,
                                     sAttr->expr,
                                     &sItrAttr->expr )
                      != IDE_SUCCESS );

            sMask = (QMC_ATTR_DISTINCT_MASK | QMC_ATTR_ORDER_BY_MASK);
            sAttr->flag |= (sItrAttr->flag & sMask);
        }
        else
        {
            // Result descriptor에 존재하지 않는 경우

            /*
             * 아래 조건이 모두 충족되어야 expression의 argument들을 추가한다.
             * 1. GROUP BY, DISTINCT, ORDER BY절의 expression이 아니어야 한다.
             *    (여기에 해당되면 sealed flag가 설정됨)
             * 2. Column/value module이 아니어야 한다(argument가 존재하지 않음).
             * 3. Subquery가 아니어야 한다.
             */
            if( ( ( sItrAttr->flag & QMC_ATTR_SEALED_MASK ) == QMC_ATTR_SEALED_FALSE ) &&
                ( sItrAttr->expr->node.module != &qtc::columnModule ) &&
                ( sItrAttr->expr->node.module != &gQtcRidModule ) &&
                ( sItrAttr->expr->node.module != &qtc::valueModule ) &&
                ( sItrAttr->expr->node.module != &qtc::subqueryModule ) )
            {
                for( sArg = sItrAttr->expr->node.arguments;
                     sArg != NULL;
                     sArg = sArg->next )
                {
                    IDE_TEST( appendAttribute( aStatement,
                                               aQuerySet,
                                               aChild,
                                               (qtcNode *)sArg,
                                               0,
                                               0,
                                               aMakePassNode )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                sMask = (QMC_ATTR_SEALED_MASK | QMC_ATTR_DISTINCT_MASK | QMC_ATTR_ORDER_BY_MASK);
                sFlag = sItrAttr->flag & sMask;

                IDE_TEST( appendAttribute( aStatement,
                                           aQuerySet,
                                           aChild,
                                           sItrAttr->expr,
                                           sFlag,
                                           QMC_APPEND_EXPRESSION_TRUE,
                                           aMakePassNode )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::makeReferenceResult( qcStatement * aStatement,
                          idBool        aMakePassNode,
                          qmcAttrDesc * aParent,
                          qmcAttrDesc * aChild )
{
/***********************************************************************
 *
 * Description :
 *    두 result descriptor간의 참조 관계를 설정해준다.
 *    pushResultDesc를 호출하지 않는 다음 operator들에서 호출한다.
 *      - HSDS
 *      - GRBY
 *
 * Implementation :
 *    두 result간 동일한 attribute가 존재하면 makeReference를 호출한다.
 *
 ***********************************************************************/

    qmcAttrDesc * sAttr;
    qmcAttrDesc * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmc::makeReferenceResult::__FT__" );

    // BUG-48101 parents의 중복된 result desc에 참조관계가 설정되지 않습니다.
    for( sItrAttr = aParent;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        IDE_TEST( findAttribute( aStatement,
                                 aChild,
                                 sItrAttr->expr,
                                 aMakePassNode,
                                 &sAttr )
                  != IDE_SUCCESS );

        if( sAttr != NULL )
        {
            IDE_TEST( makeReference( aStatement,
                                     aMakePassNode,
                                     sAttr->expr,
                                     &sItrAttr->expr )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::filterResultDesc( qcStatement  * aStatement,
                       qmcAttrDesc ** aResult,
                       qcDepInfo    * aDepInfo,
                       qcDepInfo    * aDepInfo2 )
{
/***********************************************************************
 *
 * Description :
 *    Result descriptor에서 attribute의 dependency가 aDepInfo에
 *    포함되지 않는 경우 제거한다.
 *    단, 예외적으로 key flag가 설정된 경우 이를 무시하고 포함한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmcAttrDesc * sItrAttr  = *aResult;
    qmcAttrDesc * sPrevAttr = NULL;
    qmcAttrDesc * sNextAttr;
    qcDepInfo     sAndDependencies;

    IDU_FIT_POINT_FATAL( "qmc::filterResultDesc::__FT__" );

    // BUG-37057
    // target을 제외한 외부참조 컬럼의 경우 result desc 가 될수 없다.
    qtc::dependencyAnd( aDepInfo,
                        aDepInfo2,
                        &sAndDependencies );

    while ( sItrAttr != NULL )
    {
        sNextAttr = sItrAttr->next;

        // aDepInfo에 포함되지 않고 key attribute도 아닌 경우 삭제
        if( ( qtc::dependencyContains(
                  &sAndDependencies,
                  &sItrAttr->expr->depInfo ) == ID_FALSE ) &&
            ( ( sItrAttr->flag & QMC_ATTR_KEY_MASK ) == QMC_ATTR_KEY_FALSE ) )
        {
            if( sPrevAttr == NULL )
            {
                // Result descriptr의 첫 번째 attribute인 경우
                *aResult = sNextAttr;
            }
            else
            {
                // Result descriptr의 첫 번째가 아닌 attribute인 경우
                sPrevAttr->next = sNextAttr;
            }

            IDE_TEST( QC_QMP_MEM( aStatement )->free( sItrAttr ) != IDE_SUCCESS );
        }
        else
        {
            sPrevAttr = sItrAttr;
        }

        sItrAttr = sNextAttr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmc::appendViewPredicate( qcStatement  * aStatement,
                                 qmsQuerySet  * aQuerySet,
                                 qmcAttrDesc ** aResult,
                                 UInt           aFlag )
{
/***********************************************************************
 *
 * Description :
 *    BUG-43077
 *    view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가한다.
 *    다음과 같은 경우에는 외부 참조 컬럼들이 Result descriptor에 없을수 있다.
 *    SELECT count(a.i1) FROM t2 a, t2 b, Lateral (select * from t1 where t1.i1=a.i1 and t1.i1=b.i1) v;
 *
 * Implementation :
 *
 ***********************************************************************/
    qmsOuterNode * sNode;

    IDU_FIT_POINT_FATAL( "qmc::appendViewPredicate::__FT__" );

    if ( aQuerySet->setOp == QMS_NONE )
    {
        for ( sNode = aQuerySet->SFWGH->outerColumns;
              sNode != NULL;
              sNode = sNode->next )
        {
            IDE_TEST( appendPredicate( aStatement,
                                       aQuerySet,
                                       aResult,
                                       sNode->column,
                                       aFlag )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_TEST( appendViewPredicate( aStatement,
                                       aQuerySet->left,
                                       aResult,
                                       aFlag )
                  != IDE_SUCCESS );

        IDE_TEST( appendViewPredicate( aStatement,
                                       aQuerySet->right,
                                       aResult,
                                       aFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmc::duplicateGroupExpression( qcStatement * aStatement,
                               qtcNode     * aExpr )
{
/***********************************************************************
 *
 * Description :
 *    HAVING절 또는 analytic function에 포함된 group expression은 반드시
 *    GRAG의 결과를 가리켜야 한다. 그런데 DISTINCT/ORDER BY절을 사용한
 *    경우 HSDS/SORT의 결과를 가리키도록 변경될 수 있어 별도로 복사한다.
 *
 * Implementation :
 *    Group expression은 pass node를 통해 참조하므로 pass node를 찾아
 *    argument를 복사한후 가리키도록 한다.
 *
 ***********************************************************************/

    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmc::duplicateGroupExpression::__FT__" );

    if( aExpr->node.module == &qtc::passModule )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                   (void**)&sNode )
                  != IDE_SUCCESS );
        idlOS::memcpy( sNode, aExpr->node.arguments, ID_SIZEOF( qtcNode ) );
        aExpr->node.arguments = (mtcNode *)sNode;
    }
    else
    {
        sNode = (qtcNode *)aExpr->node.arguments;

        for( sNode = (qtcNode *)aExpr->node.arguments;
             sNode != NULL;
             sNode = (qtcNode *)sNode->node.next )
        {
            IDE_TEST( qmc::duplicateGroupExpression( aStatement,
                                                     sNode )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmc::disableSealTrueFlag( qmcAttrDesc * aResult )
{
    qmcAttrDesc * sItrAttr;

    for ( sItrAttr = aResult;
          sItrAttr != NULL;
          sItrAttr = sItrAttr->next )
    {
        if ( ( sItrAttr->flag & QMC_ATTR_SEALED_MASK ) == QMC_ATTR_SEALED_TRUE )
        {
            sItrAttr->flag &= ~QMC_ATTR_SEALED_MASK;
            sItrAttr->flag |= QMC_ATTR_SEALED_FALSE;
        }
        else
        {
            /* Nothing to do */
        }
    }
}

