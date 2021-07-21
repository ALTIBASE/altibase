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
 * $Id: qmoStatMgr.cpp 90850 2021-05-17 05:09:54Z donovan.seo $
 *
 * Description :
 *     Statistical Information Manager
 *
 *     다음과 같은 각종 실시간 통계 정보의 추출을 담당한다.
 *          - Table의 Record 개수
 *          - Table의 Disk Page 개수
 *          - Index의 Cardinality
 *          - Column의 Cardinality
 *          - Column의 MIN Value
 *          - Column의 MAX Value
 *
 * 용어 설명 :
 *
 *     1. columnNDV        : 중복을 제거한 value 수
 *     2. index columnNDV  : index가 가지고 있는 columnNDV 정보
 *     3. column columnNDV : 컬럼의 실제 columnNDV
 *     4. MIN, MAX value     : 컬럼값 중 가장 작은 값, 가장 큰 값
 *                             ( 숫자형과 날짜형만 의미를 부여함. )
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcuProperty.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <qmoCostDef.h>
#include <qmoStatMgr.h>
#include <qmgProjection.h>
#include <smiTableSpace.h>
#include <qcmPartition.h>

extern mtdModule mtdSmallint;
extern mtdModule mtdInteger;
extern mtdModule mtdBigint;
extern mtdModule mtdReal;
extern mtdModule mtdDouble;
extern mtdModule mtdDate;

extern "C" SInt
compareKeyCountAsceding( const void * aElem1,
                         const void * aElem2 )
{
/***********************************************************************
 *
 * Description : 통계정보의 indexCardInfo를
 *               인덱스의 key column count가 작은 순서대로 정렬한다.
 *               ( DISK table의 index : access 횟수를 줄이기 위해 )
 *
 * Implementation :
 *
 *     인자로 넘어온 두 indexCardInfo의 index key column count 비교해서,
 *     인덱스의 key column count가 작은 순서대로 정렬
 *     만약, key column count가 같다면, selectivity가 작은 순서로 정렬
 *
 ***********************************************************************/

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aElem1 != NULL );
    IDE_DASSERT( aElem2 != NULL );

    //--------------------------------------
    // key column count 비교
    //--------------------------------------

    if( ((qmoIdxCardInfo *)aElem1)->index->keyColCount >
        ((qmoIdxCardInfo *)aElem2)->index->keyColCount )
    {
        return 1;
    }
    else if( ((qmoIdxCardInfo *)aElem1)->index->keyColCount <
             ((qmoIdxCardInfo *)aElem2)->index->keyColCount )
    {
        return -1;
    }
    else
    {
        // 두개의 인덱스 key count가 같으면,
        // selectivity가 작은 인덱스가 먼저 정렬되도록 한다.

        if( ((qmoIdxCardInfo *)aElem1)->KeyNDV >
            ((qmoIdxCardInfo *)aElem2)->KeyNDV )
        {
            return -1;
        }
        else if( ((qmoIdxCardInfo *)aElem1)->KeyNDV <
                 ((qmoIdxCardInfo *)aElem2)->KeyNDV )
        {
            return 1;
        }
        else
        {
            // index의 key count, selectivity가 모두 같다면,
            // Index ID 값을 이용해서 정렬한다.
            // 플랫폼간의 diff 방지를 위한 내용임.
            if( ((qmoIdxCardInfo *)aElem1)->indexId <
                ((qmoIdxCardInfo *)aElem2)->indexId )
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
    }
}


extern "C" SInt
compareKeyCountDescending( const void * aElem1,
                           const void * aElem2 )
{
/***********************************************************************
 *
 * Description : 통계정보의 indexCardInfo를
 *               인덱스의 key column count가 큰 순서대로 정렬한다.
 *               ( MEMORY table의 index : 인덱스 사용 극대화 )
 *
 * Implementation :
 *
 *     인자로 넘어온 두 indexCardInfo의 index key column count 비교해서,
 *     인덱스의 key column count가 큰 순서대로 정렬.
 *     만약, key column count가 같다면, selectivity가 작은 순서로 정렬.
 *
 ***********************************************************************/
    
    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aElem1 != NULL );
    IDE_DASSERT( aElem2 != NULL );

    //--------------------------------------
    // key column count 비교
    //--------------------------------------

    if( ((qmoIdxCardInfo *)aElem1)->index->keyColCount <
        ((qmoIdxCardInfo *)aElem2)->index->keyColCount )
    {
        return 1;
    }
    else if( ((qmoIdxCardInfo *)aElem1)->index->keyColCount >
             ((qmoIdxCardInfo *)aElem2)->index->keyColCount )
    {
        return -1;
    }
    else
    {
        // 두개의 인덱스 key count가 같으면,
        // selectivity가 작은 인덱스가 먼저 정렬되도록 한다.

        if( ((qmoIdxCardInfo *)aElem1)->KeyNDV >
            ((qmoIdxCardInfo *)aElem2)->KeyNDV )
        {
            return -1;
        }
        else if( ((qmoIdxCardInfo *)aElem1)->KeyNDV <
                 ((qmoIdxCardInfo *)aElem2)->KeyNDV )
        {
            return 1;
        }
        else
        {
            // index의 key count, selectivity가 모두 같다면,
            // Index ID 값을 이용해서 정렬한다.
            // 플랫폼간의 diff 방지를 위한 내용임.
            if( ((qmoIdxCardInfo *)aElem1)->indexId <
                ((qmoIdxCardInfo *)aElem2)->indexId )
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
    }
}


IDE_RC
qmoStat::getStatInfo4View( qcStatement    * aStatement,
                           qmgGraph       * aGraph,
                           qmoStatistics ** aStatInfo)
{
/***********************************************************************
 *
 * Description : View에 대한 통계 정보 구축
 *
 * Implementation :
 *    (1) qmoStatistics 만큼 메모리 할당 받음
 *    (2) totalRecorCnt 설정
 *    (3) pageCnt 설정
 *    (4) indexCnt는 0으로, indexCardInfo는 NULL로 설정한다.
 *    (5) columnCnt는 하위 qmgProjection::myTarget 개수로,
 *        colCardInfo는 columnCnt만큼 배열공간 할당 후, 값 설정
 *
 ***********************************************************************/

    qmoStatistics  * sStatInfo;
    qmgPROJ        * sProjGraph;
    qmsTarget      * sTarget;
    qmoColCardInfo * sViewColCardInfo;
    qmoColCardInfo * sCurColCardInfo;
    UInt             sViewColCardCnt;
    qmoColCardInfo * sColCardInfo;
    qtcNode        * sNode;
    UInt             i;

    IDU_FIT_POINT_FATAL( "qmoStat::getStatInfo4View::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sViewColCardCnt = 0;
    sProjGraph = ((qmgPROJ *)(aGraph->left));

    // qmoStatistics 만큼 메모리 할당 받음
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoStatistics ),
                                             (void**) & sStatInfo )
              != IDE_SUCCESS );

    // BUG-40913 v$table 조인 질의가 성능이 느림
    if ( aGraph->myFrom->tableRef->tableType == QCM_PERFORMANCE_VIEW )
    {
        sStatInfo->totalRecordCnt  = QMO_STAT_PFVIEW_RECORD_COUNT;
    }
    else
    {
        sStatInfo->totalRecordCnt  = aGraph->left->costInfo.outputRecordCnt;
    }

    // qmoStatistics
    sStatInfo->optGoleType     = QMO_OPT_GOAL_TYPE_ALL_ROWS;
    sStatInfo->isValidStat     = ID_TRUE;

    sStatInfo->pageCnt         = 0;
    sStatInfo->avgRowLen       = aGraph->left->costInfo.recordSize;
    sStatInfo->readRowTime     = QMO_STAT_TABLE_MEM_ROW_READ_TIME;
    sStatInfo->firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
    sStatInfo->firstRowsN      = 0;

    // indexCnt는 0, indexCardInfo는 NULL로 설정
    sStatInfo->indexCnt = 0;
    sStatInfo->idxCardInfo = NULL;

    // columnCnt는 하위 qmgProjection::myTarget개수
    for ( sTarget = sProjGraph->target;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        sViewColCardCnt++;
    }

    sStatInfo->columnCnt = sViewColCardCnt;

    // columnCnt 개의 배열공간 확보
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoColCardInfo) *
                                             sViewColCardCnt,
                                             (void**) & sViewColCardInfo )
              != IDE_SUCCESS );

    for ( sTarget = sProjGraph->target, i = 0;
          sTarget != NULL;
          sTarget = sTarget->next, i++ )
    {
        // BUG-39219 pass 노드를 고려해야 합니다.
        if ( sTarget->targetColumn->node.module == &qtc::passModule )
        {
            sNode = (qtcNode*)sTarget->targetColumn->node.arguments;
        }
        else
        {
            sNode = sTarget->targetColumn;
        }

        sCurColCardInfo = & sViewColCardInfo[i];

        sCurColCardInfo->isValidStat = ID_TRUE;

        // To Fix BUG-11480 flag를 초기화 함.
        sCurColCardInfo->flag = QMO_STAT_CLEAR;


        // To Fix BUG-8241
        sCurColCardInfo->column = QTC_STMT_COLUMN( aStatement, sNode );

        // To Fix BUG-8697
        if ( ( sCurColCardInfo->column->module->flag &
               MTD_SELECTIVITY_MASK ) == MTD_SELECTIVITY_ENABLE )
        {
            // Min, Max Value 설정 가능한 Data Type인 경우
            // Min Value, Max Value를 초기화
            sCurColCardInfo->column->module->null( sCurColCardInfo->column,
                                                   sCurColCardInfo->minValue );

            sCurColCardInfo->column->module->null( sCurColCardInfo->column,
                                                   sCurColCardInfo->maxValue );
        }
        else
        {
            // nothing to do
        }

        // BUG-38613 _prowid 를 사용한 view 를 select 할수 있어야함
        if ( (QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE) &&
             (sNode->node.column != MTC_RID_COLUMN_ID) )
        {
            // target의 qtcNode Type이 칼럼인 경우
            // tableMap을 이용하여 해당 칼럼의 columnNDV, MIN, MAX값을 구함

            sColCardInfo = &(QC_SHARED_TMPLATE(aStatement)->tableMap[sNode->node.table].
                             from->tableRef->statInfo->colCardInfo[sNode->node.column]);

            idlOS::memcpy( sCurColCardInfo,
                           sColCardInfo,
                           ID_SIZEOF(qmoColCardInfo) );

            if( sCurColCardInfo->columnNDV > sProjGraph->graph.costInfo.outputRecordCnt )
            {
                sCurColCardInfo->columnNDV = sProjGraph->graph.costInfo.outputRecordCnt;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            if ( QMO_STAT_COLUMN_NDV >
                 sProjGraph->graph.costInfo.outputRecordCnt )
            {
                sCurColCardInfo->columnNDV =
                    sProjGraph->graph.costInfo.outputRecordCnt;
            }
            else
            {
                sCurColCardInfo->columnNDV = QMO_STAT_COLUMN_NDV;
            }

            sCurColCardInfo->nullValueCount = QMO_STAT_COLUMN_NULL_VALUE_COUNT;
            sCurColCardInfo->avgColumnLen   = sProjGraph->graph.costInfo.recordSize /
                sViewColCardCnt;
        }
    }

    // To Fix BUG-8241
    sStatInfo->colCardInfo = sViewColCardInfo;
    *aStatInfo = sStatInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmoStat::printStat( qmsFrom       * aFrom,
                    ULong           aDepth,
                    iduVarString  * aString )
{
/***********************************************************************
 *
 * Description :
 *    통계 정보를 출력한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  i;
    UInt  j;

    qmoStatistics * sStatInfo;
    qmsTableRef   * sTableRef;

    qcmColumn      * sColumn;
    qmoColCardInfo * sStatColumn;
    qmoIdxCardInfo * sStatIndex;

    mtdDateType    * sMinDate;
    mtdDateType    * sMaxDate;

    SChar      sNameBuffer[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    IDU_FIT_POINT_FATAL( "qmoStat::printStat::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aFrom->tableRef != NULL );
    IDE_DASSERT( aFrom->tableRef->statInfo != NULL );

    IDE_DASSERT( aString != NULL );

    sTableRef = aFrom->tableRef;
    sStatInfo = aFrom->tableRef->statInfo;

    //-----------------------------------
    // Table 정보의 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Table Information ==" );

    // Table Name
    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "TABLE NAME         : " );

    if ( sTableRef->aliasName.size > 0 )
    {
        if ( sTableRef->aliasName.size <= QC_MAX_OBJECT_NAME_LEN )
        {
            idlOS::memcpy ( sNameBuffer,
                            sTableRef->aliasName.stmtText +
                            sTableRef->aliasName.offset,
                            sTableRef->aliasName.size );
            sNameBuffer[sTableRef->aliasName.size] = '\0';
        }
        else
        {
            sNameBuffer[0] = '\0';
        }
        iduVarStringAppend( aString,
                            sNameBuffer );
    }
    else
    {
        if ( sTableRef->tableName.size > 0 )
        {
            if ( sTableRef->tableName.size <= QC_MAX_OBJECT_NAME_LEN )
            {
                idlOS::memcpy ( sNameBuffer,
                                sTableRef->tableName.stmtText +
                                sTableRef->tableName.offset,
                                sTableRef->tableName.size );
                sNameBuffer[sTableRef->tableName.size] = '\0';
            }
            else
            {
                sNameBuffer[0] = '\0';
            }
            iduVarStringAppend( aString,
                                sNameBuffer );
        }
        else
        {
            iduVarStringAppend( aString,
                                "N/A" );
        }
    }

    //-----------------------------------
    // Column 정보의 출력
    //-----------------------------------

    sColumn = sTableRef->tableInfo->columns;
    sStatColumn = sStatInfo->colCardInfo;

    for ( j = 0; j < sStatInfo->columnCnt; j++ )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );

        // Column Information
        // Name & Cardinality
        iduVarStringAppendFormat( aString,
                                  "  %s : %"ID_PRINT_G_FMT,
                                  sColumn[j].name,
                                  sStatColumn[j].columnNDV );

        // fix BUG-13516 valgrind UMR
        // MIN, MAX 값은 selectivity를 예측할 수 있는 data type에 대해서만
        // 실제 MIN, MAX, NULL 값을 저장하기때문에
        // 해당 data type에 대해서만 출력하도록 함.
        // PROJ-2242 컬럼의 min,max 를 설정되었는지 flag 를 봐야 한다.
        if( (sStatColumn[j].flag & QMO_STAT_MINMAX_COLUMN_SET_MASK) ==
            QMO_STAT_MINMAX_COLUMN_SET_TRUE )
        {

            if ( sColumn[j].basicInfo->
                 module->isNull( sColumn[j].basicInfo,
                                 sStatColumn[j].minValue )
                 != ID_TRUE )
            {
                // Min, Max Value
                if ( sColumn[j].basicInfo->module == & mtdSmallint )
                {
                    iduVarStringAppendFormat( aString,
                                              ", %"ID_INT32_FMT", %"ID_INT32_FMT,
                                              *(SShort *) sStatColumn[j].minValue,
                                              *(SShort *) sStatColumn[j].maxValue );
                }
                else if ( sColumn[j].basicInfo->module == & mtdInteger )
                {
                    iduVarStringAppendFormat( aString,
                                              ", %"ID_INT32_FMT", %"ID_INT32_FMT,
                                              *(SInt *) sStatColumn[j].minValue,
                                              *(SInt *) sStatColumn[j].maxValue );
                }
                else if ( sColumn[j].basicInfo->module == & mtdBigint )
                {
                    iduVarStringAppendFormat( aString,
                                              ", %"ID_INT64_FMT", %"ID_INT64_FMT,
                                              *(SLong *) sStatColumn[j].minValue,
                                              *(SLong *) sStatColumn[j].maxValue );
                }
                else if ( sColumn[j].basicInfo->module == & mtdReal )
                {
                    iduVarStringAppendFormat( aString,
                                              ", %"ID_PRINT_G_FMT", %"ID_PRINT_G_FMT,
                                              *(SFloat *) sStatColumn[j].minValue,
                                              *(SFloat *) sStatColumn[j].maxValue );
                }
                else if ( sColumn[j].basicInfo->module == & mtdDouble )
                {
                    iduVarStringAppendFormat( aString,
                                              ", %"ID_PRINT_G_FMT", %"ID_PRINT_G_FMT,
                                              *(SDouble *) sStatColumn[j].minValue,
                                              *(SDouble *) sStatColumn[j].maxValue );
                }
                else if ( sColumn[j].basicInfo->module == & mtdDate )
                {
                    sMinDate = (mtdDateType*) sStatColumn[j].minValue;
                    sMaxDate = (mtdDateType*) sStatColumn[j].maxValue;

                    iduVarStringAppendFormat( aString,
                                              ", %"ID_INT32_FMT"/%"ID_INT32_FMT"/%"ID_INT32_FMT
                                              " %"ID_INT32_FMT":%"ID_INT32_FMT":%"ID_INT32_FMT
                                              ", %"ID_INT32_FMT"/%"ID_INT32_FMT"/%"ID_INT32_FMT
                                              " %"ID_INT32_FMT":%"ID_INT32_FMT":%"ID_INT32_FMT,
                                              mtdDateInterface::year(sMinDate),
                                              (SInt)mtdDateInterface::month(sMinDate),
                                              (SInt)mtdDateInterface::day(sMinDate),
                                              (SInt)mtdDateInterface::hour(sMinDate),
                                              (SInt)mtdDateInterface::minute(sMinDate),
                                              (SInt)mtdDateInterface::second(sMinDate),
                                              mtdDateInterface::year(sMaxDate),
                                              (SInt)mtdDateInterface::month(sMaxDate),
                                              (SInt)mtdDateInterface::day(sMaxDate),
                                              (SInt)mtdDateInterface::hour(sMaxDate),
                                              (SInt)mtdDateInterface::minute(sMaxDate),
                                              (SInt)mtdDateInterface::second(sMaxDate) );
                }
                else
                {
                    // Numeric 등의 Type은 출력 방법이 난해하다.
                    // 일단 보류함.
                }
            }
            else
            {
                // Null Value인 경우로 통계 정보를 출력하지 않음
            }

        }
        else
        {
            // selectivity를 구할 수 없는 data type
            // Nothing To Do
        }
    }

    //-----------------------------------
    // Index 정보의 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Index Information ==" );

    sStatIndex = sStatInfo->idxCardInfo;
    for ( j = 0; j < sStatInfo->indexCnt; j++ )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );

        // Column Information
        // Name & Cardinality

        iduVarStringAppendFormat( aString,
                                  "  %s : %"ID_PRINT_G_FMT,
                                  sStatIndex[j].index->name,
                                  sStatIndex[j].KeyNDV );
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoStat::printStat4Partition( qmsTableRef     * aTableRef,
                              qmsPartitionRef * aPartitionRef,
                              SChar           * aPartitionName,
                              ULong             aDepth,
                              iduVarString    * aString )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *    partition에 대한 통계 정보를 출력한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  i;
    UInt  j;

    qmoStatistics * sStatInfo;

    qcmColumn      * sColumn;
    qmoColCardInfo * sStatColumn;
    qmoIdxCardInfo * sStatIndex;
    qcmIndex       * sIndex;
    mtdDateType    * sMinDate;
    mtdDateType    * sMaxDate;

    IDU_FIT_POINT_FATAL( "qmoStat::printStat4Partition::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aPartitionRef != NULL );
    IDE_DASSERT( aPartitionRef->statInfo != NULL );

    IDE_DASSERT( aString != NULL );

    sStatInfo = aPartitionRef->statInfo;

    //-----------------------------------
    // Table 정보의 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Partition Information ==" );

    // Table Name
    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "PARTITION NAME         : " );

    /* BUG-44659 미사용 Partition의 통계 정보를 출력하다가,
     *           Graph의 Partition/Column/Index Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  qmgSELT에서 Partition Name을 보관하도록 수정합니다.
     */
    iduVarStringAppend( aString, aPartitionName );

    //-----------------------------------
    // Column 정보의 출력
    //-----------------------------------

    /* BUG-44659 미사용 Partition의 통계 정보를 출력하다가,
     *           Graph의 Partition/Column/Index Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  Column Name을 Partitioned Table에서 얻도록 수정합니다.
     */
    sColumn = aTableRef->tableInfo->columns;
    sStatColumn = sStatInfo->colCardInfo;

    for ( j = 0; j < sStatInfo->columnCnt; j++ )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );

        // Column Information
        // Name & Cardinality
        iduVarStringAppendFormat( aString,
                                  "  %s : %"ID_PRINT_G_FMT,
                                  sColumn[j].name,
                                  sStatColumn[j].columnNDV );

        // fix BUG-13516 valgrind UMR
        // MIN, MAX 값은 selectivity를 예측할 수 있는 data type에 대해서만
        // 실제 MIN, MAX, NULL 값을 저장하기때문에
        // 해당 data type에 대해서만 출력하도록 함.
        if( ( sColumn[j].basicInfo->module->flag & MTD_SELECTIVITY_MASK )
            == MTD_SELECTIVITY_ENABLE )
        {
            if ( ( sStatColumn[j].flag & QMO_STAT_MINMAX_COLUMN_SET_MASK) ==
                   QMO_STAT_MINMAX_COLUMN_SET_TRUE )
            {

                if ( sColumn[j].basicInfo->
                     module->isNull( sColumn[j].basicInfo,
                                     sStatColumn[j].minValue )
                     != ID_TRUE )
                {
                    // Min, Max Value
                    if ( sColumn[j].basicInfo->module == & mtdSmallint )
                    {
                        iduVarStringAppendFormat( aString,
                                                  ", %"ID_INT32_FMT", %"ID_INT32_FMT,
                                                  *(SShort *) sStatColumn[j].minValue,
                                                  *(SShort *) sStatColumn[j].maxValue );
                    }
                    else if ( sColumn[j].basicInfo->module == & mtdInteger )
                    {
                        iduVarStringAppendFormat( aString,
                                                  ", %"ID_INT32_FMT", %"ID_INT32_FMT,
                                                  *(SInt *) sStatColumn[j].minValue,
                                                  *(SInt *) sStatColumn[j].maxValue );
                    }
                    else if ( sColumn[j].basicInfo->module == & mtdBigint )
                    {
                        iduVarStringAppendFormat( aString,
                                                  ", %"ID_INT64_FMT", %"ID_INT64_FMT,
                                                  *(SLong *) sStatColumn[j].minValue,
                                                  *(SLong *) sStatColumn[j].maxValue );
                    }
                    else if ( sColumn[j].basicInfo->module == & mtdReal )
                    {
                        iduVarStringAppendFormat( aString,
                                                  ", %"ID_PRINT_G_FMT", %"ID_PRINT_G_FMT,
                                                  *(SFloat *) sStatColumn[j].minValue,
                                                  *(SFloat *) sStatColumn[j].maxValue );
                    }
                    else if ( sColumn[j].basicInfo->module == & mtdDouble )
                    {
                        iduVarStringAppendFormat( aString,
                                                  ", %"ID_PRINT_G_FMT", %"ID_PRINT_G_FMT,
                                                  *(SDouble *) sStatColumn[j].minValue,
                                                  *(SDouble *) sStatColumn[j].maxValue );
                    }
                    else if ( sColumn[j].basicInfo->module == & mtdDate )
                    {
                        sMinDate = (mtdDateType*) sStatColumn[j].minValue;
                        sMaxDate = (mtdDateType*) sStatColumn[j].maxValue;

                        iduVarStringAppendFormat( aString,
                                                  ", %"ID_INT32_FMT"/%"ID_INT32_FMT"/%"ID_INT32_FMT
                                                  " %"ID_INT32_FMT":%"ID_INT32_FMT":%"ID_INT32_FMT
                                                  ", %"ID_INT32_FMT"/%"ID_INT32_FMT"/%"ID_INT32_FMT
                                                  " %"ID_INT32_FMT":%"ID_INT32_FMT":%"ID_INT32_FMT,
                                                  mtdDateInterface::year(sMinDate),
                                                  (SInt)mtdDateInterface::month(sMinDate),
                                                  (SInt)mtdDateInterface::day(sMinDate),
                                                  (SInt)mtdDateInterface::hour(sMinDate),
                                                  (SInt)mtdDateInterface::minute(sMinDate),
                                                  (SInt)mtdDateInterface::second(sMinDate),
                                                  mtdDateInterface::year(sMaxDate),
                                                  (SInt)mtdDateInterface::month(sMaxDate),
                                                  (SInt)mtdDateInterface::day(sMaxDate),
                                                  (SInt)mtdDateInterface::hour(sMaxDate),
                                                  (SInt)mtdDateInterface::minute(sMaxDate),
                                                  (SInt)mtdDateInterface::second(sMaxDate) );
                    }
                    else
                    {
                        // Numeric 등의 Type은 출력 방법이 난해하다.
                        // 일단 보류함.
                    }
                }
                else
                {
                    // Null Value인 경우로 통계 정보를 출력하지 않음
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            // selectivity를 구할 수 없는 data type
            // Nothing To Do
        }
    }

    //-----------------------------------
    // Index 정보의 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Index Information ==" );

    sStatIndex = sStatInfo->idxCardInfo;
    for ( j = 0; j < sStatInfo->indexCnt; j++ )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );

        // Column Information
        // Name & Cardinality
        IDE_TEST( qcmPartition::getPartIdxFromIdxId( sStatIndex[j].indexId,
                                                     aTableRef,
                                                     & sIndex )
                  != IDE_SUCCESS );

        iduVarStringAppendFormat( aString,
                                  "  %s : %"ID_PRINT_G_FMT,
                                  sIndex->name,
                                  sStatIndex[j].KeyNDV );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoStat::getStatInfo4AllBaseTables( qcStatement * aStatement,
                                    qmsSFWGH    * aSFWGH )
{
/***********************************************************************
 *
 * Description : 일반 테이블에 대한 통계 정보를 구축한다.
 *
 *     qmsSFWGH->from에 달려있는 모든 Base Table에 대한 통계정보를 구축한다.
 *     [ JOIN인 경우, 하위 일반 테이블도 모두 검사 ]
 *
 *      일반 Table : qmsSFWGH->from->joinType = QMS_NO_JOIN 이고,
 *                   qmsSFWGH->from->tableRef->view == NULL 인지를 검사.
 *      VIEW 는 skip
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom  * sFrom;

    IDU_FIT_POINT_FATAL( "qmoStat::getStatInfo4AllBaseTables::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSFWGH != NULL );

    //--------------------------------------
    // 모든 base Table에 통계정보 설정.
    //--------------------------------------

    for( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        if( sFrom->joinType == QMS_NO_JOIN )
        {
            // PROJ-2582 recursive with
            if( ( sFrom->tableRef->view == NULL ) &&
                ( ( sFrom->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                  == QMS_TABLE_REF_RECURSIVE_VIEW_FALSE ) )
            {
                //-----------------------------------------
                // 현재 테이블이 base table로 통계정보 설정.
                //-----------------------------------------

                /* PROJ-1832 New database link */
                if ( sFrom->tableRef->remoteTable != NULL )
                {
                    IDE_TEST( getStatInfo4RemoteTable( aStatement,
                                                       sFrom->tableRef->tableInfo,
                                                       &( sFrom->tableRef->statInfo ) )
                              != IDE_SUCCESS );
                }
                else if( sFrom->tableRef->tableInfo->tablePartitionType ==
                         QCM_PARTITIONED_TABLE )
                {
                    // partitioned table인 경우 rule-based통계정보를 만든다.
                    // 오로지 partition keyrange를 뽑아내기 위함임.
                    IDE_TEST( getStatInfo4BaseTable( aStatement,
                                                     QMO_OPT_GOAL_TYPE_RULE,
                                                     sFrom->tableRef->tableInfo,
                                                     &( sFrom->tableRef->statInfo ) )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( getStatInfo4BaseTable( aStatement,
                                                     aSFWGH->hints->optGoalType,
                                                     sFrom->tableRef->tableInfo,
                                                     &( sFrom->tableRef->statInfo ) )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                // VIEW 혹은 Recursive View 인 경우로,
                // optimizer단계에서 view에 대한 통계정보 설정.
            }
        }
        else
        {
            //---------------------------------------
            // 현재 테이블이 base 테이블이 아니므로,
            // left, right from을 순회하면서,
            // base table을 찾아서 통계정보 설정.
            //---------------------------------------

            IDE_TEST( findBaseTableNGetStatInfo( aStatement,
                                                 aSFWGH,
                                                 sFrom ) != IDE_SUCCESS );
        }
    }

    // qmoSystemStatistics 메모리 할당
    IDE_TEST( QC_QME_MEM(aStatement)->alloc( ID_SIZEOF(qmoSystemStatistics),
                                             (void **)&(aStatement->mSysStat) )
              != IDE_SUCCESS );

    // PROJ-2242
    if( aSFWGH->hints->optGoalType != QMO_OPT_GOAL_TYPE_RULE )
    {
        IDE_TEST( getSystemStatistics( aStatement->mSysStat ) != IDE_SUCCESS );
    }
    else
    {
        getSystemStatistics4Rule( aStatement->mSysStat );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::findBaseTableNGetStatInfo( qcStatement * aStatement,
                                    qmsSFWGH    * aSFWGH,
                                    qmsFrom     * aFrom )
{
/***********************************************************************
 *
 * Description : 현재 테이블의 left, right 테이블을 순회하면서,
 *               base table을 찾아서 통계 정보를 구축한다.
 *
 * Implementation :
 *     이 함수는 base table을 찾을때까지 재귀적으로 호출된다.
 *     1. left From에 대한 처리
 *     2. right From에 대한 처리
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoStat::findBaseTableNGetStatInfo::__FT__" );

    //--------------------------------------
    // 현재 From의 left, right 를 방문해서,
    // base table을 찾고, 통계정보를 설정한다.
    //--------------------------------------

    //--------------------------------------
    // left From에 대한 처리
    //--------------------------------------

    if( aFrom->left->joinType == QMS_NO_JOIN )
    {
        // PROJ-2582 recursive with
        if ( ( aFrom->left->tableRef->view == NULL ) &&
             ( ( aFrom->left->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
               == QMS_TABLE_REF_RECURSIVE_VIEW_FALSE ) )
        {
            /* PROJ-1832 New database link */
            if ( aFrom->left->tableRef->remoteTable != NULL )
            {
                IDE_TEST( getStatInfo4RemoteTable( aStatement,
                                                   aFrom->left->tableRef->tableInfo,
                                                   &( aFrom->left->tableRef->statInfo ) )
                          != IDE_SUCCESS );
            }
            else
            {
                // 테이블에 대한 통계정보를 구축한다.
                IDE_TEST( getStatInfo4BaseTable( aStatement,
                                                 aSFWGH->hints->optGoalType,
                                                 aFrom->left->tableRef->tableInfo,
                                                 &( aFrom->left->tableRef->statInfo ) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // VIEW, RECURSIVE VIEW 인 경우로,
            // optimizer단계에서 view에 대한 통계정보 설정
        }
    }
    else
    {
        IDE_TEST( findBaseTableNGetStatInfo( aStatement,
                                             aSFWGH,
                                             aFrom->left )
                  != IDE_SUCCESS );
    }

    //--------------------------------------
    // right From에 대한 처리
    //--------------------------------------

    if( aFrom->right->joinType == QMS_NO_JOIN )
    {
        // PROJ-2582 recursive with
        if ( ( aFrom->right->tableRef->view == NULL ) &&
             ( ( aFrom->right->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
               == QMS_TABLE_REF_RECURSIVE_VIEW_FALSE ) )
        {
            /* PROJ-1832 New database link */
            if ( aFrom->right->tableRef->remoteTable != NULL )
            {
                IDE_TEST( getStatInfo4RemoteTable( aStatement,
                                                   aFrom->right->tableRef->tableInfo,
                                                   &( aFrom->right->tableRef->statInfo ) )
                          != IDE_SUCCESS );
            }
            else
            {
                // 테이블에 대한 통계정보를 구축한다.
                IDE_TEST( getStatInfo4BaseTable( aStatement,
                                                 aSFWGH->hints->optGoalType,
                                                 aFrom->right->tableRef->tableInfo,
                                                 &( aFrom->right->tableRef->statInfo ) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // VIEW, RECURSIVE VIEW 인 경우로,
            // optimizer 단계에서 view에 대한 통계정보 설정
        }
    }
    else
    {
        IDE_TEST( findBaseTableNGetStatInfo( aStatement,
                                             aSFWGH,
                                             aFrom->right )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::getSystemStatistics( qmoSystemStatistics * aStatistics )
{
/***********************************************************************
 *
 * Description : 각 index columnNDV,
 * Implementation :
 *
 ***********************************************************************/

    idBool   sIsValid;
    SDouble  sSingleReadTime;
    SDouble  sMultiReadTime;
    SDouble  sHashTime;
    SDouble  sCompareTime;
    SDouble  sStoreTime;

    IDU_FIT_POINT_FATAL( "qmoStat::getSystemStatistics::__FT__" );

    sIsValid = smiStatistics::isValidSystemStat();

    if( sIsValid == ID_TRUE )
    {
        IDE_TEST( smiStatistics::getSystemStatSReadTime( ID_FALSE, &sSingleReadTime )
                  != IDE_SUCCESS );
        IDE_TEST( smiStatistics::getSystemStatMReadTime( ID_FALSE, &sMultiReadTime )
                  != IDE_SUCCESS );
        IDE_TEST( smiStatistics::getSystemStatHashTime( &sHashTime )
                  != IDE_SUCCESS );
        IDE_TEST( smiStatistics::getSystemStatCompareTime( &sCompareTime )
                  != IDE_SUCCESS );
        IDE_TEST( smiStatistics::getSystemStatStoreTime( &sStoreTime )
                  != IDE_SUCCESS );

        aStatistics->isValidStat      = sIsValid;

        // BUG-37125 tpch plan optimization
        // Time이 0이 나올때 기본값을 설정하면 안된다.
        aStatistics->singleIoScanTime = IDL_MAX(sSingleReadTime, QMO_STAT_TIME_MIN);
        aStatistics->multiIoScanTime  = IDL_MAX(sMultiReadTime,  QMO_STAT_TIME_MIN);
        aStatistics->mMTCallTime      = IDL_MAX(sCompareTime,    QMO_STAT_TIME_MIN);
        aStatistics->mHashTime        = IDL_MAX(sHashTime,       QMO_STAT_TIME_MIN);
        aStatistics->mCompareTime     = IDL_MAX(sCompareTime,    QMO_STAT_TIME_MIN);
        aStatistics->mStoreTime       = IDL_MAX(sStoreTime,      QMO_STAT_TIME_MIN);
        aStatistics->mIndexNlJoinPenalty = QCU_OPTIMIZER_INDEX_NL_JOIN_PENALTY;
    }
    else
    {
        getSystemStatistics4Rule( aStatistics );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmoStat::getSystemStatistics4Rule( qmoSystemStatistics * aStatistics )
{
    aStatistics->isValidStat      = ID_TRUE;

    aStatistics->singleIoScanTime = QMO_STAT_SYSTEM_SREAD_TIME;
    aStatistics->multiIoScanTime  = QMO_STAT_SYSTEM_MREAD_TIME;
    aStatistics->mMTCallTime      = QMO_STAT_SYSTEM_MTCALL_TIME;
    aStatistics->mHashTime        = QMO_STAT_SYSTEM_HASH_TIME;
    aStatistics->mCompareTime     = QMO_STAT_SYSTEM_COMPARE_TIME;
    aStatistics->mStoreTime       = QMO_STAT_SYSTEM_STORE_TIME;
    aStatistics->mIndexNlJoinPenalty = QCU_OPTIMIZER_INDEX_NL_JOIN_PENALTY;
}

IDE_RC
qmoStat::getStatInfo4BaseTable( qcStatement    * aStatement,
                                qmoOptGoalType   aOptimizerMode,
                                qcmTableInfo   * aTableInfo,
                                qmoStatistics ** aStatInfo )
{
/***********************************************************************
 *
 * Description : 일반 테이블에 대한 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     1. qmoStatistics에 대한 메모리를 할당받는다.
 *     2. table record count 설정
 *     3. table disk blcok count 설정
 *     4. index/column columnNDV, MIN/MAX 정보 설정
 *
 ***********************************************************************/

    UInt             sCnt;
    idBool           sIsDiskTable;
    qmoStatistics  * sStatistics;
    qmoIdxCardInfo * sSortedIdxInfoArray;
    smiAllStat     * sAllStat           = NULL;
    idBool           sDynamicStats      = ID_FALSE;
    UInt             sAutoStatsLevel    = 0;
    SFloat           sPercentage        = 0.0;
    idBool           sIsAutoDBMSStat    = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoStat::getStatInfo4BaseTable::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aOptimizerMode != QMO_OPT_GOAL_TYPE_UNKNOWN );
    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aStatInfo != NULL );

    //--------------------------------------
    // 통계정보구축을 위한 자료구조에 대한 메모리 할당
    //--------------------------------------

    IDU_FIT_POINT("qmoStat::getStatInfo4BaseTable::malloc1");

    // qmoStatistics 메모리 할당
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoStatistics ),
                                               (void **)& sStatistics )
              != IDE_SUCCESS );

    // PROJ-2492 Dynamic sample selection
    // 통계정보를 모두 가져올 메모리를 할당받는다.
    IDE_TEST( QC_QME_MEM( aStatement )->alloc( ID_SIZEOF( smiAllStat ),
                                               (void **)& sAllStat )
              != IDE_SUCCESS );

    if( aTableInfo->indexCount != 0 )
    {
        // fix BUG-10095
        // table의 index 생성정보가 enable인 경우만 인덱스 통계정보생성
        if ( ( aTableInfo->tableFlag & SMI_TABLE_DISABLE_ALL_INDEX_MASK )
             == SMI_TABLE_ENABLE_ALL_INDEX )
        {
            /* BUG-43006 FixedTable Indexing Filter
             * optimizer formance vie propery 가 0이라면
             * FixedTable 의 index는 없다고 설정해줘야한다
             */
            if ( ( ( aTableInfo->tableType == QCM_FIXED_TABLE ) ||
                   ( aTableInfo->tableType == QCM_DUMP_TABLE ) ||
                   ( aTableInfo->tableType == QCM_PERFORMANCE_VIEW ) ) &&
                 ( QCG_GET_SESSION_OPTIMIZER_PERFORMANCE_VIEW(aStatement) == 0 ) )
            {
                sStatistics->indexCnt    = 0;
                sStatistics->idxCardInfo = NULL;

                sAllStat->mIndexCount = 0;
                sAllStat->mIndexStat  = NULL;
            }
            else
            {
                sStatistics->indexCnt = aTableInfo->indexCount;
                sAllStat->mIndexCount = aTableInfo->indexCount;

                IDU_FIT_POINT("qmoStat::getStatInfo4BaseTable::malloc2");

                // index 통계정보를 위한 메모리 할당
                IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoIdxCardInfo ) * aTableInfo->indexCount,
                                                           (void **)& sStatistics->idxCardInfo )
                          != IDE_SUCCESS );

                IDE_TEST( QC_QME_MEM( aStatement )->alloc( ID_SIZEOF( smiIndexStat ) * aTableInfo->indexCount,
                                                           (void **)& sAllStat->mIndexStat )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            sStatistics->indexCnt    = 0;
            sStatistics->idxCardInfo = NULL;

            sAllStat->mIndexCount = 0;
            sAllStat->mIndexStat  = NULL;
        }
    }
    else
    {
        sStatistics->indexCnt    = 0;
        sStatistics->idxCardInfo = NULL;

        sAllStat->mIndexCount = 0;
        sAllStat->mIndexStat  = NULL;
    }

    // column 통계정보를 위한 메모리 할당
    sStatistics->columnCnt = aTableInfo->columnCount;
    sAllStat->mColumnCount = aTableInfo->columnCount;

    IDU_FIT_POINT("qmoStat::getStatInfo4BaseTable::malloc3");

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoColCardInfo ) * aTableInfo->columnCount,
                                               (void **)& sStatistics->colCardInfo )
              != IDE_SUCCESS );

    // PROJ-2492 Dynamic sample selection
    // 통계정보를 모두 가져올 메모리를 할당받는다.
    IDE_TEST( QC_QME_MEM( aStatement )->alloc( ID_SIZEOF( smiColumnStat ) * aTableInfo->columnCount,
                                               (void **)& sAllStat->mColumnStat )
              != IDE_SUCCESS );

    //--------------------------------------
    // 통계정보 자료구조의 index와 컬럼 정보 저장
    //--------------------------------------

    sIsDiskTable = QCM_TABLE_TYPE_IS_DISK( aTableInfo->tableFlag );

    for( sCnt=0; sCnt < sStatistics->indexCnt; sCnt++ )
    {
        /* Meta Cache의 Index는 Index ID 순서로 정렬되어 있다. */
        sStatistics->idxCardInfo[sCnt].indexId = aTableInfo->indices[sCnt].indexId;
        sStatistics->idxCardInfo[sCnt].index = &(aTableInfo->indices[sCnt]);
        sStatistics->idxCardInfo[sCnt].flag  = QMO_STAT_CLEAR;
    }

    for( sCnt=0; sCnt < sStatistics->columnCnt ; sCnt++ )
    {
        sStatistics->colCardInfo[sCnt].column
            = aTableInfo->columns[sCnt].basicInfo;
        sStatistics->colCardInfo[sCnt].flag = QMO_STAT_CLEAR;
    }

    /* BUG-35460 Add TABLE_TYPE G in SYS_TABLES_ */
    if( (aTableInfo->tableType == QCM_USER_TABLE) ||
        (aTableInfo->tableType == QCM_MVIEW_TABLE) ||
        (aTableInfo->tableType == QCM_META_TABLE) ||
        (aTableInfo->tableType == QCM_INDEX_TABLE) ||
        (aTableInfo->tableType == QCM_SEQUENCE_TABLE) ||
        (aTableInfo->tableType == QCM_RECYCLEBIN_TABLE) ) //PROJ-2441
    {
        sStatistics->optGoleType = aOptimizerMode;
    }
    else
    {
        // FIXED TABLE
        sStatistics->optGoleType = QMO_OPT_GOAL_TYPE_RULE;
    }

    switch( sStatistics->optGoleType )
    {
        case QMO_OPT_GOAL_TYPE_RULE :
            //------------------------------
            // 테이블 통계정보 수집
            //------------------------------
            getTableStatistics4Rule( sStatistics,
                                     aTableInfo,
                                     sIsDiskTable );

            //------------------------------
            // 인덱스 통계정보 수집
            //------------------------------
            getIndexStatistics4Rule( sStatistics,
                                     sIsDiskTable );

            //------------------------------
            // 컬럼 통계정보 수집
            //------------------------------
            getColumnStatistics4Rule( sStatistics );
            break;

        case QMO_OPT_GOAL_TYPE_ALL_ROWS :
        case QMO_OPT_GOAL_TYPE_FIRST_ROWS_N:

            // PROJ-2492 Dynamic sample selection
            sAutoStatsLevel = QCG_GET_SESSION_OPTIMIZER_AUTO_STATS( aStatement );

            // startup 이 완료된후에만 샘플링을 한다.
            // SMU_DBMS_STAT_METHOD_AUTO 는 예전방식으로
            // 통계정보를 수집하기때문에 샘플링을 하지 않는다.

            // BUG-43629 OPTIMIZER_AUTO_STATS 동작시 aexport 가 느려집니다.
            // system_.sys_* 테이블들은 auto_stats 를 사용하지 않도록 한다.
            // SMI_MEMORY_SYSTEM_DICTIONARY 테이블 스페이스것은 모두 제외시킨다.
            if ( (sAutoStatsLevel != 0) &&
                 (smiGetStartupPhase() == SMI_STARTUP_SERVICE) &&
                 (smuProperty::getDBMSStatMethod() == SMU_DBMS_STAT_METHOD_MANUAL ) &&
                 (smiStatistics::isValidTableStat( (void*)aTableInfo->tableHandle ) == ID_FALSE) &&
                 (aTableInfo->TBSType != SMI_MEMORY_SYSTEM_DICTIONARY) )
            {
                sDynamicStats = ID_TRUE;

                IDE_TEST( calculateSamplePercentage( aTableInfo,
                                                     sAutoStatsLevel,
                                                     &sPercentage )
                          != IDE_SUCCESS);

                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_OPTIMIZER_AUTO_STATS );
            }
            else
            {
                sDynamicStats = ID_FALSE;
                sPercentage   = 0;
            }

            if ( smuProperty::getDBMSStatMethod() == SMU_DBMS_STAT_METHOD_AUTO )
            {
                sIsAutoDBMSStat = ID_TRUE;
            }

            // 통계정보를 가져온다.
            // sDynamicStats ID_TRUE 일때 통계정보를 수집해서 가져온다.
            // 단 Index 의 경우 기존의 통계정보가 있을때는 기존의 통계정보를 가져온다.
            IDE_NOFT_TEST( smiStatistics::getTableAllStat(
                               aStatement->mStatistics,
                               (QC_SMI_STMT(aStatement))->getTrans(),
                               aTableInfo->tableHandle,
                               sAllStat,
                               sPercentage,
                               sDynamicStats )
                           != IDE_SUCCESS );

            //------------------------------
            // 테이블 통계정보 수집
            //------------------------------
            IDE_TEST ( getTableStatistics( aStatement,
                                           aTableInfo,
                                           sStatistics,
                                           &sAllStat->mTableStat ) != IDE_SUCCESS );

            //------------------------------
            // 컬럼 통계정보 수집
            //------------------------------
            IDE_TEST( getColumnStatistics( sStatistics,
                                           sAllStat->mColumnStat ) != IDE_SUCCESS );

            //------------------------------
            // 인덱스 통계정보 수집
            //------------------------------
            IDE_TEST( getIndexStatistics( aStatement,
                                          sStatistics,
                                          sIsAutoDBMSStat,
                                          sAllStat->mIndexStat ) != IDE_SUCCESS );
            break;
        default :
            IDE_DASSERT(0);
    }

    //--------------------------------------
    // 구축된 index 통계정보를 정렬한다.
    // disk table   : index key Column Count가 작은 순서로
    // memory table : index key Column Count가 큰 순서로
    //--------------------------------------
    if ( sStatistics->idxCardInfo != NULL )
    {
        if( sIsDiskTable == ID_TRUE )
        {
            //--------------------------------------
            // Disk Table의 경우 Column Cardinaltiy 획득 과정에서
            // 구축된 Index 정렬 정보를 그대로 사용함.
            //--------------------------------------

            // Nothing To Do
        }
        else
        {
            //--------------------------------------
            // Memory Table의 경우
            // Key Column의 개수가 많은 순서로 Index를 정렬한다.
            // 따라서, 기존에 구성된 정렬 정보를 재정렬한다.
            //--------------------------------------

            IDE_TEST( sortIndexInfo( sStatistics,
                                     ID_FALSE, // Descending 정렬
                                     &sSortedIdxInfoArray ) != IDE_SUCCESS );

            sStatistics->idxCardInfo = sSortedIdxInfoArray;
        }
    }
    else
    {
        // Index가 없음
        // Nothing To Do
    }

    // TPC-H를 위한 가상 통계 정보 구축
    if ( QCU_FAKE_TPCH_SCALE_FACTOR > 0 )
    {
        IDE_TEST( getFakeStatInfo( aStatement, aTableInfo, sStatistics )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    *aStatInfo = sStatistics;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::getTableStatistics( qcStatement    * aStatement,
                             qcmTableInfo   * aTableInfo,
                             qmoStatistics  * aStatInfo,
                             smiTableStat   * aData )
{
    SLong sRecordCnt    = 0;

    IDU_FIT_POINT_FATAL( "qmoStat::getTableStatistics::__FT__" );

    //-----------------------------
    // valid
    //-----------------------------

    if ( aData->mCreateTV > 0 )
    {
        aStatInfo->isValidStat = ID_TRUE;

        //-----------------------------
        // 테이블의 레코드 평균 길이
        //-----------------------------

        if( aData->mAverageRowLen <= 0 )
        {
            aStatInfo->avgRowLen = ID_SIZEOF(UShort);
        }
        else
        {
            aStatInfo->avgRowLen = aData->mAverageRowLen;
        }

        //-----------------------------
        // 테이블의 레코드 평균 읽기 시간
        //-----------------------------
        aStatInfo->readRowTime = IDL_MAX(aData->mOneRowReadTime, QMO_STAT_READROW_TIME_MIN);
    }
    else
    {
        aStatInfo->isValidStat = ID_FALSE;
        aStatInfo->avgRowLen   = QMO_STAT_TABLE_AVG_ROW_LEN;
        aStatInfo->readRowTime = QMO_STAT_TABLE_MEM_ROW_READ_TIME;
    }

    //-----------------------------
    // 테이블의 총 레코드 수
    //-----------------------------
    // BUG-44795
    if ( ( QC_SHARED_TMPLATE(aStatement)->optimizerDBMSStatPolicy == 1 ) ||
         ( QCU_PLAN_REBUILD_ENABLE == 1 ) )
    {
        IDE_TEST( smiStatistics::getTableStatNumRow( (void*)aTableInfo->tableHandle,
                                                     ID_TRUE,
                                                     NULL,
                                                     &sRecordCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( smiStatistics::isValidTableStat( aTableInfo->tableHandle ) == ID_TRUE )
        {
            IDE_TEST( smiStatistics::getTableStatNumRow( (void*)aTableInfo->tableHandle,
                                                         ID_FALSE,
                                                         NULL,
                                                         &sRecordCnt )
                      != IDE_SUCCESS );
        }
        else
        {
            sRecordCnt = QMO_STAT_TABLE_RECORD_COUNT;
        }
    }

    if ( sRecordCnt == SMI_STAT_NULL )
    {
        aStatInfo->totalRecordCnt = QMO_STAT_TABLE_RECORD_COUNT;

        IDE_DASSERT( 0 );
    }
    else if ( sRecordCnt <= 0 )
    {
        aStatInfo->totalRecordCnt = 1;
    }
    else
    {
        aStatInfo->totalRecordCnt = sRecordCnt;
    }

    //-----------------------------
    // 테이블의 총 디스크 페이지 수
    //-----------------------------
    IDE_TEST( setTablePageCount( aStatement,
                                 aTableInfo,
                                 aStatInfo,
                                 aData ) != IDE_SUCCESS );

    aStatInfo->firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
    aStatInfo->firstRowsN      = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmoStat::getTableStatistics4Rule( qmoStatistics * aStatistics,
                                  qcmTableInfo  * aTableInfo,
                                  idBool          aIsDiskTable )
{
    UInt sTableFlag = 0;
        
    aStatistics->isValidStat      = ID_TRUE;

    aStatistics->firstRowsFactor  = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
    aStatistics->firstRowsN       = 0;

    if( aIsDiskTable == ID_TRUE )
    {
        aStatistics->totalRecordCnt   = QMO_STAT_TABLE_RECORD_COUNT;
        aStatistics->pageCnt          = QMO_STAT_TABLE_DISK_PAGE_COUNT;
        aStatistics->avgRowLen        = QMO_STAT_TABLE_AVG_ROW_LEN;
        aStatistics->readRowTime      = QMO_STAT_TABLE_DISK_ROW_READ_TIME;
    }
    else
    {
        // BUG-43324
        sTableFlag = smiGetTableFlag( aTableInfo->tableHandle );

        if ( (sTableFlag & SMI_TABLE_TYPE_MASK) == SMI_TABLE_FIXED )
        {
            if ( (sTableFlag & SMI_TABLE_DUAL_MASK) == SMI_TABLE_DUAL_TRUE )
            {
                /* DUAL TALBE */
                aStatistics->totalRecordCnt = 1;
            }
            else
            {
                /* FIXED TALBE */
                aStatistics->totalRecordCnt = QMO_STAT_PFVIEW_RECORD_COUNT;
            }
        }
        else
        {
            aStatistics->totalRecordCnt   = QMO_STAT_TABLE_RECORD_COUNT;
        }

        aStatistics->pageCnt          = 0;
        aStatistics->avgRowLen        = QMO_STAT_TABLE_AVG_ROW_LEN;
        aStatistics->readRowTime      = QMO_STAT_TABLE_MEM_ROW_READ_TIME;
    }
}

IDE_RC
qmoStat::setTablePageCount( qcStatement    * aStatement,
                            qcmTableInfo   * aTableInfo,
                            qmoStatistics  * aStatInfo,
                            smiTableStat   * aData )
{
    SLong    sPageCnt;

    IDU_FIT_POINT_FATAL( "qmoStat::setTablePageCount::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aStatInfo  != NULL );

    //--------------------------------------
    // disk page count 구하기
    //--------------------------------------
    if( aStatInfo->isValidStat == ID_TRUE )
    {
        sPageCnt       = aData->mNumPage;
    }
    else
    {
        // BUG-44795
        if ( QC_SHARED_TMPLATE(aStatement)->optimizerDBMSStatPolicy == 1 )
        {
            IDE_TEST( smiStatistics::getTableStatNumPage( aTableInfo->tableHandle,
                                                          ID_TRUE,
                                                          &sPageCnt )
                      != IDE_SUCCESS);
        }
        else
        {
            sPageCnt = QMO_STAT_TABLE_DISK_PAGE_COUNT;
        }
    }

    // 현재 테이블이 disk인지 memory인지를 판단한다.
    if( smiTableSpace::isDiskTableSpaceType( aTableInfo->TBSType ) == ID_TRUE )
    {
        if( sPageCnt == SMI_STAT_NULL )
        {
            aStatInfo->pageCnt = QMO_STAT_TABLE_DISK_PAGE_COUNT;

            IDE_DASSERT( 0 );
        }
        else if( sPageCnt < 0 )
        {
            aStatInfo->pageCnt = 0;
        }
        else
        {
            aStatInfo->pageCnt = sPageCnt;
        }
    }
    else
    {
        aStatInfo->pageCnt = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoStat::getIndexStatistics( qcStatement   * aStatement,
                                    qmoStatistics * aStatistics,
                                    idBool          aIsAutoDBMStat,
                                    smiIndexStat  * aData )
{
    qmoIdxCardInfo * sIdxCardInfo   = NULL;
    smiIndexStat   * sIdxData       = NULL;
    UInt             i  = 0;
    UInt             j  = 0;

    IDU_FIT_POINT_FATAL( "qmoStat::getIndexStatistics::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aStatistics != NULL );

    for ( i = 0; i < aStatistics->indexCnt; i++ )
    {
        sIdxCardInfo = &(aStatistics->idxCardInfo[i]);

        // BUG-47884
        sIdxData = NULL;
        for ( j = 0; j < aStatistics->indexCnt; j++ )
        {
            if ( sIdxCardInfo->indexId == aData[j].mId )
            {
                sIdxData     = &(aData[j]);
                break;
            }
        }

        IDE_TEST_RAISE( sIdxData == NULL, ERR_NOT_FOUND );

        if ( smiStatistics::isValidIndexStat( sIdxCardInfo->index->indexHandle ) == ID_TRUE )
        {
            sIdxCardInfo->isValidStat = ID_TRUE;
        }
        else
        {
            if ( sIdxData->mCreateTV > 0 )
            {
                sIdxCardInfo->isValidStat = ID_TRUE;
            }
            else
            {
                sIdxCardInfo->isValidStat = ID_FALSE;
            }
        }

        if ( sIdxCardInfo->isValidStat == ID_TRUE )
        {
            //--------------------------------------
            // 인덱스 keyNDV 얻기
            //--------------------------------------
            if ( sIdxData->mNumDist <= 0 )
            {
                /* BUG-48971 */
                if ( aIsAutoDBMStat == ID_TRUE )
                {
                    sIdxCardInfo->KeyNDV = 1;
                }
                else
                {
                    sIdxCardInfo->KeyNDV = QMO_STAT_INDEX_KEY_NDV + 1;
                }
            }
            else
            {
                sIdxCardInfo->KeyNDV = sIdxData->mNumDist;
            }

            //--------------------------------------
            // 인덱스 평균 Slot 갯수
            //--------------------------------------
            if( sIdxData->mAvgSlotCnt <= 0 )
            {
                sIdxCardInfo->avgSlotCount = QMO_STAT_INDEX_AVG_SLOT_COUNT;
            }
            else
            {
                sIdxCardInfo->avgSlotCount = sIdxData->mAvgSlotCnt;
            }

            //--------------------------------------
            // 인덱스 높이
            //--------------------------------------
            if( sIdxData->mIndexHeight <= 0 )
            {
                sIdxCardInfo->indexLevel = 1;
            }
            else
            {
                sIdxCardInfo->indexLevel = sIdxData->mIndexHeight;
            }

            //--------------------------------------
            // 인덱스 클러스터링 팩터
            //--------------------------------------
            if( sIdxData->mClusteringFactor <= 0 )
            {
                sIdxCardInfo->clusteringFactor = 1;
            }
            else
            {
                sIdxCardInfo->clusteringFactor = sIdxData->mClusteringFactor;

                // BUG-43258
                sIdxCardInfo->clusteringFactor *= QCU_OPTIMIZER_INDEX_CLUSTERING_FACTOR_ADJ;
                sIdxCardInfo->clusteringFactor /= 100.0;
            }
        }
        else
        {
            sIdxCardInfo->KeyNDV            = QMO_STAT_INDEX_KEY_NDV;
            sIdxCardInfo->avgSlotCount      = QMO_STAT_INDEX_AVG_SLOT_COUNT;
            sIdxCardInfo->indexLevel        = QMO_STAT_INDEX_HEIGHT;
            sIdxCardInfo->clusteringFactor  = QMO_STAT_INDEX_CLUSTER_FACTOR;
        }

        //--------------------------------------
        // 인덱스 디스크 page 갯수
        //--------------------------------------
        IDE_TEST( setIndexPageCnt( aStatement,
                                   sIdxCardInfo->index,
                                   sIdxCardInfo,
                                   sIdxData )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        ideLog::log( IDE_QP_0,
                     "[qmoStat::getIndexStatistics] not found index id : %"ID_UINT32_FMT"",
                     sIdxCardInfo->indexId );

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoStat::getIndexStatistics",
                                  "index statistic not found" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmoStat::getIndexStatistics4Rule( qmoStatistics * aStatistics,
                                  idBool          aIsDiskTable )
{
    UInt             sCnt;
    qmoIdxCardInfo * sIdxCardInfo;

    sIdxCardInfo = aStatistics->idxCardInfo;
        
    for( sCnt = 0; sCnt < aStatistics->indexCnt; sCnt++ )
    {
        sIdxCardInfo[sCnt].isValidStat       = ID_TRUE;

        sIdxCardInfo[sCnt].KeyNDV            = QMO_STAT_INDEX_KEY_NDV;
        sIdxCardInfo[sCnt].avgSlotCount      = QMO_STAT_INDEX_AVG_SLOT_COUNT;
        sIdxCardInfo[sCnt].indexLevel        = QMO_STAT_INDEX_HEIGHT;

        if( aIsDiskTable == ID_TRUE )
        {
            sIdxCardInfo[sCnt].pageCnt           = QMO_STAT_INDEX_DISK_PAGE_COUNT;
            sIdxCardInfo[sCnt].clusteringFactor  = QMO_STAT_INDEX_CLUSTER_FACTOR;
        }
        else
        {
            sIdxCardInfo[sCnt].pageCnt           = 0;
            sIdxCardInfo[sCnt].clusteringFactor  = QMO_STAT_INDEX_CLUSTER_FACTOR;
        }
    }
}

IDE_RC qmoStat::setIndexPageCnt( qcStatement    * aStatement,
                                 qcmIndex       * aIndex,
                                 qmoIdxCardInfo * aIdxInfo,
                                 smiIndexStat   * aData )
{
    SLong  sPageCnt;

    IDU_FIT_POINT_FATAL( "qmoStat::setIndexPageCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aIndex       != NULL );
    IDE_DASSERT( aIdxInfo     != NULL );

    if( aIdxInfo->isValidStat == ID_TRUE )
    {
        sPageCnt = aData->mNumPage;
    }
    else
    {
        // BUG-44795
        if ( QC_SHARED_TMPLATE(aStatement)->optimizerDBMSStatPolicy == 1 )
        {
            IDE_TEST( smiStatistics::getIndexStatNumPage( aIndex->indexHandle,
                                                          ID_TRUE,
                                                          &sPageCnt )
                      != IDE_SUCCESS);
        }
        else
        {
            sPageCnt = QMO_STAT_INDEX_DISK_PAGE_COUNT;
        }
    }

    if ( ( aIndex->keyColumns->column.flag & SMI_COLUMN_STORAGE_MASK )
         == SMI_COLUMN_STORAGE_DISK )
    {
        if( sPageCnt == SMI_STAT_NULL )
        {
            aIdxInfo->pageCnt = QMO_STAT_INDEX_DISK_PAGE_COUNT;

            IDE_DASSERT( 0 );
        }
        else if( sPageCnt < 0 )
        {
            aIdxInfo->pageCnt = 0;
        }
        else
        {
            aIdxInfo->pageCnt = sPageCnt;
        }
    }
    else
    {
        aIdxInfo->pageCnt = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoStat::getColumnStatistics( qmoStatistics * aStatistics,
                                     smiColumnStat * aData )
{
    qmoColCardInfo   * sColCardInfo   = NULL;
    smiColumnStat    * sColData       = NULL;
    UInt               i = 0;

    IDU_FIT_POINT_FATAL( "qmoStat::getColumnStatistics::__FT__" );

    for ( i = 0; i < aStatistics->columnCnt; i++ )
    {
        sColCardInfo = &(aStatistics->colCardInfo[i]);
        sColData     = &(aData[i]);

        if ( sColData->mCreateTV > 0 )
        {
            sColCardInfo->isValidStat   = ID_TRUE;

            //--------------------------------------
            // 컬럼 NDV
            //--------------------------------------
            if( sColData->mNumDist <= 0 )
            {
                sColCardInfo->columnNDV = 1;
            }
            else
            {
                sColCardInfo->columnNDV = sColData->mNumDist;
            }

            //--------------------------------------
            // 컬럼 null value 갯수
            //--------------------------------------
            if( sColData->mNumNull < 0 )
            {
                sColCardInfo->nullValueCount = QMO_STAT_COLUMN_NULL_VALUE_COUNT;
            }
            else
            {
                sColCardInfo->nullValueCount = sColData->mNumNull;
            }

            //--------------------------------------
            // 컬럼 평균 길이
            //--------------------------------------
            if( sColData->mAverageColumnLen <= 0 )
            {
                sColCardInfo->avgColumnLen = ID_SIZEOF(UShort);
            }
            else
            {
                sColCardInfo->avgColumnLen = sColData->mAverageColumnLen;
            }

            //--------------------------------------
            // 컬럼 MIN, MAX
            //--------------------------------------
            if( ( sColCardInfo->column->module->flag & MTD_SELECTIVITY_MASK )
                == MTD_SELECTIVITY_ENABLE )
            {
                idlOS::memcpy( sColCardInfo->minValue,
                               sColData->mMinValue,
                               SMI_MAX_MINMAX_VALUE_SIZE );

                idlOS::memcpy( sColCardInfo->maxValue,
                               sColData->mMaxValue,
                               SMI_MAX_MINMAX_VALUE_SIZE );

                sColCardInfo->flag &= ~QMO_STAT_MINMAX_COLUMN_SET_MASK;
                sColCardInfo->flag |= QMO_STAT_MINMAX_COLUMN_SET_TRUE;
            }
            else
            {
                sColCardInfo->flag &= ~QMO_STAT_MINMAX_COLUMN_SET_MASK;
                sColCardInfo->flag |= QMO_STAT_MINMAX_COLUMN_SET_FALSE;
            }
        }
        else
        {
            sColCardInfo->isValidStat       = ID_FALSE;
            sColCardInfo->columnNDV         = QMO_STAT_COLUMN_NDV;
            sColCardInfo->nullValueCount    = QMO_STAT_COLUMN_NULL_VALUE_COUNT;
            sColCardInfo->avgColumnLen      = QMO_STAT_COLUMN_AVG_LEN;

            if( ( sColCardInfo->column->module->flag & MTD_SELECTIVITY_MASK )
                == MTD_SELECTIVITY_ENABLE )
            {
                sColCardInfo->column->module->null(
                    sColCardInfo->column,
                    sColCardInfo->minValue );

                sColCardInfo->column->module->null(
                    sColCardInfo->column,
                    sColCardInfo->maxValue );

                sColCardInfo->flag &= ~QMO_STAT_MINMAX_COLUMN_SET_MASK;
                sColCardInfo->flag |= QMO_STAT_MINMAX_COLUMN_SET_TRUE;
            }
            else
            {
                sColCardInfo->flag &= ~QMO_STAT_MINMAX_COLUMN_SET_MASK;
                sColCardInfo->flag |= QMO_STAT_MINMAX_COLUMN_SET_FALSE;
            }
        }
    }

    return IDE_SUCCESS;
}

void
qmoStat::getColumnStatistics4Rule( qmoStatistics * aStatistics )
{
    UInt               sColCnt;
    qmoColCardInfo   * sColCardInfo;

    sColCardInfo = aStatistics->colCardInfo;

    for( sColCnt = 0; sColCnt < aStatistics->columnCnt; sColCnt++ )
    {
        sColCardInfo[sColCnt].isValidStat      = ID_TRUE;
        sColCardInfo[sColCnt].flag             = QMO_STAT_CLEAR;
        sColCardInfo[sColCnt].columnNDV        = QMO_STAT_COLUMN_NDV;
        sColCardInfo[sColCnt].nullValueCount   = QMO_STAT_COLUMN_NULL_VALUE_COUNT;
        sColCardInfo[sColCnt].avgColumnLen     = QMO_STAT_COLUMN_AVG_LEN;
    }

}

IDE_RC
qmoStat::setColumnNDV( qcmTableInfo   * aTableInfo,
                       qmoColCardInfo * aColInfo )
{
    SLong  sColumnNDV = 0;

    IDU_FIT_POINT_FATAL( "qmoStat::setColumnNDV::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aColInfo     != NULL );

    IDE_TEST( smiStatistics::getColumnStatNumDist( aTableInfo->tableHandle,
                                                   ( aColInfo->column->column.id & SMI_COLUMN_ID_MASK ),
                                                   & sColumnNDV )
              != IDE_SUCCESS );

    if( sColumnNDV == SMI_STAT_NULL )
    {
        aColInfo->columnNDV = QMO_STAT_COLUMN_NDV;
    }
    else if( sColumnNDV <= 0 )
    {
        aColInfo->columnNDV = 1;
    }
    else
    {
        aColInfo->columnNDV = sColumnNDV;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::setColumnNullCount( qcmTableInfo   * aTableInfo,
                             qmoColCardInfo * aColInfo )
{
    SLong  sNullCount = 0;

    IDU_FIT_POINT_FATAL( "qmoStat::setColumnNullCount::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aColInfo     != NULL );

    IDE_TEST( smiStatistics::getColumnStatNumNull( aTableInfo->tableHandle,
                                                   ( aColInfo->column->column.id & SMI_COLUMN_ID_MASK ),
                                                   & sNullCount )
              != IDE_SUCCESS );

    if( sNullCount == SMI_STAT_NULL )
    {
        aColInfo->nullValueCount = QMO_STAT_COLUMN_NULL_VALUE_COUNT;
    }
    else if( sNullCount < 0 )
    {
        aColInfo->nullValueCount = QMO_STAT_COLUMN_NULL_VALUE_COUNT;
    }
    else
    {
        aColInfo->nullValueCount = sNullCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::setColumnAvgLen( qcmTableInfo   * aTableInfo,
                          qmoColCardInfo * aColInfo )
{
    SLong  sAvgColLen;

    IDU_FIT_POINT_FATAL( "qmoStat::setColumnAvgLen::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aColInfo     != NULL );

    IDE_TEST( smiStatistics::getColumnStatAverageColumnLength( aTableInfo->tableHandle,
                                                               ( aColInfo->column->column.id & SMI_COLUMN_ID_MASK ),
                                                               & sAvgColLen )
              != IDE_SUCCESS );

    if( sAvgColLen == SMI_STAT_NULL )
    {
        aColInfo->avgColumnLen = QMO_STAT_COLUMN_AVG_LEN;
    }
    else if( sAvgColLen <= 0 )
    {
        aColInfo->avgColumnLen = ID_SIZEOF(UShort);
    }
    else
    {
        aColInfo->avgColumnLen = sAvgColLen;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::sortIndexInfo( qmoStatistics   * aStatInfo,
                        idBool            aIsAscending,
                        qmoIdxCardInfo ** aSortedIdxInfoArray )
{
/***********************************************************************
 *
 * Description : 통계정보의 idxCardInfo를 정렬한다.
 *
 *    memory table index : index key column count가 많은 순서로
 *    disk table index   : index key column count가 작은 순서로
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoIdxCardInfo   * sIdxCardInfoArray;

    IDU_FIT_POINT_FATAL( "qmoStat::sortIndexInfo::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatInfo != NULL );
    IDE_DASSERT( aStatInfo->idxCardInfo != NULL );

    //--------------------------------------
    // 통계정보의 idxCardInfo를 정렬한다.
    // memory table index : index key column count가 많은 순서로
    // disk table index   : index key column count가 작은 순서로
    //--------------------------------------

    sIdxCardInfoArray = aStatInfo->idxCardInfo;

    if( aStatInfo->indexCnt > 1 )
    {
        if( aIsAscending == ID_TRUE )
        {
            // qsort:
            // disk table index는 access 횟수를 줄이기 위해,
            // key column count가 작은순서대로 정렬
            idlOS::qsort( sIdxCardInfoArray,
                          aStatInfo->indexCnt,
                          ID_SIZEOF(qmoIdxCardInfo),
                          compareKeyCountAsceding );
        }
        else
        {
            // qsort:
            // memory table index는 index 사용을 극대화하기 위해서,
            // key column count가 큰 순서대로 정렬
            idlOS::qsort( sIdxCardInfoArray,
                          aStatInfo->indexCnt,
                          ID_SIZEOF(qmoIdxCardInfo),
                          compareKeyCountDescending );
        }
    }
    else
    {
        sIdxCardInfoArray = aStatInfo->idxCardInfo;
    }

    *aSortedIdxInfoArray = sIdxCardInfoArray;

    return IDE_SUCCESS;
}

IDE_RC
qmoStat::getStatInfo4PartitionedTable( qcStatement    * aStatement,
                                       qmoOptGoalType   aOptimizerMode,
                                       qmsTableRef    * aTableRef,
                                       qmoStatistics  * aStatInfo )
{
    UInt               sCnt;
    UInt               sCnt2 = 0;

    qmsPartitionRef  * sPartitionRef;
    qmsIndexTableRef * sIndexTable;
    qcmIndex         * sIndex;
    idBool             sFound;

    IDU_FIT_POINT_FATAL( "qmoStat::getStatInfo4PartitionedTable::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aOptimizerMode != QMO_OPT_GOAL_TYPE_UNKNOWN );
    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aStatInfo != NULL );
    IDE_DASSERT( aTableRef->tableInfo != NULL );
    IDE_DASSERT( aTableRef->tableInfo->tablePartitionType ==
                 QCM_PARTITIONED_TABLE );

    aStatInfo->optGoleType = aOptimizerMode;

    if( aOptimizerMode == QMO_OPT_GOAL_TYPE_RULE )
    {
        // partitioned table은 기본적으로 rule-based로 계산이 되었다.
        // Nothing to do.
    }
    else
    {
        if ( aTableRef->partitionRef != NULL )
        {
            aStatInfo->isValidStat     = ID_TRUE;
            aStatInfo->totalRecordCnt  = 0;
            aStatInfo->pageCnt         = 0;
            aStatInfo->avgRowLen       = 0;
            aStatInfo->readRowTime     = 0;
            aStatInfo->firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
            aStatInfo->firstRowsN      = 0;
        }
        else
        {
            aStatInfo->isValidStat     = ID_TRUE;
            aStatInfo->totalRecordCnt  = 1;
            aStatInfo->pageCnt         = 0;
            aStatInfo->avgRowLen       = ID_SIZEOF(UShort);
            aStatInfo->readRowTime     = QMO_STAT_READROW_TIME_MIN;
            aStatInfo->firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
            aStatInfo->firstRowsN      = 0;
        }

        // record count, page count를 합산한다.
        for( sPartitionRef = aTableRef->partitionRef;
             sPartitionRef != NULL;
             sPartitionRef = sPartitionRef->next )
        {
            if( sPartitionRef->statInfo->isValidStat == ID_FALSE )
            {
                aStatInfo->isValidStat = ID_FALSE;
            }
            else
            {
                // nothing todo.
            }

            aStatInfo->totalRecordCnt += sPartitionRef->statInfo->totalRecordCnt;
            aStatInfo->pageCnt        += sPartitionRef->statInfo->pageCnt;

            aStatInfo->avgRowLen   = IDL_MAX( aStatInfo->avgRowLen,
                                              sPartitionRef->statInfo->avgRowLen );

            aStatInfo->readRowTime = IDL_MAX( aStatInfo->readRowTime,
                                              sPartitionRef->statInfo->readRowTime );
        }

        //--------------------------------------
        // Index 통계정보
        //--------------------------------------
        for( sCnt=0; sCnt < aStatInfo->indexCnt ; sCnt++ )
        {
            if ( aStatInfo->idxCardInfo[sCnt].index->indexPartitionType ==
                 QCM_NONE_PARTITIONED_INDEX )
            {
                sFound = ID_FALSE;

                for ( sIndexTable = aTableRef->indexTableRef;
                      sIndexTable != NULL;
                      sIndexTable = sIndexTable->next )
                {
                    if ( aStatInfo->idxCardInfo[sCnt].index->indexTableID ==
                         sIndexTable->tableID )
                    {
                        sFound = ID_TRUE;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                // 찾지못하면 뭔가 문제가 있는 것임
                IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NOT_FOUND );

                for( sCnt2 = 0;
                     sCnt2 < sIndexTable->statInfo->indexCnt;
                     sCnt2++ )
                {
                    if ( ( idlOS::strlen( sIndexTable->statInfo->idxCardInfo[sCnt2].index->name ) >=
                           QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE ) &&
                         ( idlOS::strMatch( sIndexTable->statInfo->idxCardInfo[sCnt2].index->name,
                                            QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE,
                                            QD_INDEX_TABLE_KEY_INDEX_PREFIX,
                                            QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE ) == 0 ) )
                    {
                        sIndex = aStatInfo->idxCardInfo[sCnt].index;

                        idlOS::memcpy( &(aStatInfo->idxCardInfo[sCnt]),
                                       &(sIndexTable->statInfo->idxCardInfo[sCnt2]),
                                       ID_SIZEOF(qmoIdxCardInfo) );

                        aStatInfo->idxCardInfo[sCnt].index = sIndex;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                // BUG-42372 파티션 프루닝에 의해서 파티션이 모두 제거가된 경우
                // Selectivity 가 잘못 계산됨
                if ( aTableRef->partitionRef != NULL )
                {
                    // 누적 시키기 위한 초기값
                    aStatInfo->idxCardInfo[sCnt].isValidStat        = aStatInfo->isValidStat;
                    aStatInfo->idxCardInfo[sCnt].flag               = QMO_STAT_CLEAR;
                    aStatInfo->idxCardInfo[sCnt].KeyNDV             = 0;
                    aStatInfo->idxCardInfo[sCnt].avgSlotCount       = 1;
                    aStatInfo->idxCardInfo[sCnt].pageCnt            = 0;
                    aStatInfo->idxCardInfo[sCnt].indexLevel         = 0;
                    aStatInfo->idxCardInfo[sCnt].clusteringFactor   = 1;
                }
                else
                {
                    // 파티션 프루닝에 의해서 파티션이 모두 제거된 경우
                    // Default 값으로 세팅함
                    aStatInfo->idxCardInfo[sCnt].isValidStat        = ID_FALSE;
                    aStatInfo->idxCardInfo[sCnt].flag               = QMO_STAT_CLEAR;
                    aStatInfo->idxCardInfo[sCnt].KeyNDV             = QMO_STAT_INDEX_KEY_NDV;
                    aStatInfo->idxCardInfo[sCnt].avgSlotCount       = QMO_STAT_INDEX_AVG_SLOT_COUNT;
                    aStatInfo->idxCardInfo[sCnt].pageCnt            = 0;
                    aStatInfo->idxCardInfo[sCnt].indexLevel         = QMO_STAT_INDEX_HEIGHT;
                    aStatInfo->idxCardInfo[sCnt].clusteringFactor   = QMO_STAT_INDEX_CLUSTER_FACTOR;
                }

                for( sPartitionRef = aTableRef->partitionRef;
                     sPartitionRef != NULL;
                     sPartitionRef = sPartitionRef->next )
                {
                    for( sCnt2 = 0;
                         sCnt2 < sPartitionRef->statInfo->indexCnt;
                         sCnt2++ )
                    {
                        if( aStatInfo->idxCardInfo[sCnt].index->indexId ==
                            sPartitionRef->statInfo->idxCardInfo[sCnt2].index->indexId )
                        {
                            // KeyNDV 모두 더한다.
                            aStatInfo->idxCardInfo[sCnt].KeyNDV +=
                                sPartitionRef->statInfo->idxCardInfo[sCnt2].KeyNDV;

                            // avgSlotCount 최대값을 세팅한다.
                            aStatInfo->idxCardInfo[sCnt].avgSlotCount =
                                IDL_MAX( aStatInfo->idxCardInfo[sCnt].avgSlotCount,
                                         sPartitionRef->statInfo->idxCardInfo[sCnt2].avgSlotCount );

                            // pageCnt 모두 더한다.
                            aStatInfo->idxCardInfo[sCnt].pageCnt +=
                                sPartitionRef->statInfo->idxCardInfo[sCnt2].pageCnt;

                            // indexLevel 최대값을 세팅한다.
                            aStatInfo->idxCardInfo[sCnt].indexLevel =
                                IDL_MAX( aStatInfo->idxCardInfo[sCnt].indexLevel,
                                         sPartitionRef->statInfo->idxCardInfo[sCnt2].indexLevel );

                            // clusteringFactor 최대값을 세팅한다.
                            aStatInfo->idxCardInfo[sCnt].clusteringFactor =
                                IDL_MAX( aStatInfo->idxCardInfo[sCnt].clusteringFactor,
                                         sPartitionRef->statInfo->idxCardInfo[sCnt2].clusteringFactor );
                            // min, max 값은 별도로 세팅하지 않는다.
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    } // end for ( sCnt2 = ...
                } // end for ( sPartitionRef = ...

                /* BUG-46432 */
                if ( sCnt2 == 0 )
                {
                    aStatInfo->idxCardInfo[sCnt].isValidStat        = ID_FALSE;
                    aStatInfo->idxCardInfo[sCnt].flag               = QMO_STAT_CLEAR;
                    aStatInfo->idxCardInfo[sCnt].KeyNDV             = QMO_STAT_INDEX_KEY_NDV;
                    aStatInfo->idxCardInfo[sCnt].avgSlotCount       = QMO_STAT_INDEX_AVG_SLOT_COUNT;
                    aStatInfo->idxCardInfo[sCnt].pageCnt            = 0;
                    aStatInfo->idxCardInfo[sCnt].indexLevel         = QMO_STAT_INDEX_HEIGHT;
                    aStatInfo->idxCardInfo[sCnt].clusteringFactor   = QMO_STAT_INDEX_CLUSTER_FACTOR;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        } // end for ( sCnt = ...


        //--------------------------------------
        // 컬럼 통계정보
        //--------------------------------------
        for( sCnt=0; sCnt < aStatInfo->columnCnt ; sCnt++ )
        {
            aStatInfo->colCardInfo[sCnt].isValidStat    = aStatInfo->isValidStat;
            aStatInfo->colCardInfo[sCnt].flag           = QMO_STAT_CLEAR;
            aStatInfo->colCardInfo[sCnt].columnNDV      = 0;
            aStatInfo->colCardInfo[sCnt].nullValueCount = 0;

            IDE_TEST( setColumnNDV( aTableRef->tableInfo,
                                    &(aStatInfo->colCardInfo[sCnt]) )
                      != IDE_SUCCESS );

            IDE_TEST( setColumnNullCount( aTableRef->tableInfo,
                                          &(aStatInfo->colCardInfo[sCnt]) )
                      != IDE_SUCCESS );

            IDE_TEST( setColumnAvgLen( aTableRef->tableInfo,
                                       &(aStatInfo->colCardInfo[sCnt]) )
                      != IDE_SUCCESS );
        }

        //--------------------------------------
        // TPC-H를 위한 가상 통계 정보 구축
        //--------------------------------------
        if ( QCU_FAKE_TPCH_SCALE_FACTOR > 0 )
        {
            IDE_TEST( getFakeStatInfo( aStatement, aTableRef->tableInfo, aStatInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoStat::getStatInfo4PartitionedTable",
                                  "index table not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::getFakeStatInfo( qcStatement    * aStatement,
                          qcmTableInfo   * aTableInfo,
                          qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     1. qmoStatistics에 대한 메모리를 할당받는다.
 *     2. table record count 설정
 *     3. table disk blcok count 설정
 *     4. index/column columnNDV, MIN/MAX 정보 설정
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStatInfo::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aStatInfo != NULL );

    //--------------------------------------
    // TPC-H 전용 테이블 여부의 검사
    //--------------------------------------

    // REGION_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "REGION_M",
                          8 ) == 0 )
    {
        IDE_TEST( getFakeStat4Region( ID_FALSE, // Memory Table
                                      aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // REGION_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "REGION_D",
                          8 ) == 0 )
    {
        IDE_TEST( getFakeStat4Region( ID_TRUE, // Disk Table
                                      aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // NATION_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "NATION_M",
                          8 ) == 0 )
    {
        IDE_TEST( getFakeStat4Nation( ID_FALSE, // Memory Table
                                      aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // NATION_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "NATION_D",
                          8 ) == 0 )
    {
        IDE_TEST( getFakeStat4Nation( ID_TRUE, // Disk Table
                                      aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    // SUPPLIER_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "SUPPLIER_M",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4Supplier( ID_FALSE, // Memory Table
                                        aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // SUPPLIER_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "SUPPLIER_D",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4Supplier( ID_TRUE, // Disk Table
                                        aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // CUSTOMER_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "CUSTOMER_M",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4Customer( ID_FALSE, // Memory Table
                                        aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    // CUSTOMER_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "CUSTOMER_D",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4Customer( ID_TRUE, // Disk Table
                                        aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    // PART_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "PART_M",
                          6 ) == 0 )
    {
        IDE_TEST( getFakeStat4Part( ID_FALSE, // Memory Table
                                    aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    // PART_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "PART_D",
                          6 ) == 0 )
    {
        IDE_TEST( getFakeStat4Part( ID_TRUE, // Disk Table
                                    aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // PARTSUPP_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "PARTSUPP_M",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4PartSupp( ID_FALSE, // Memory Table
                                        aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // PARTSUPP_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "PARTSUPP_D",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4PartSupp( ID_TRUE, // Disk Table
                                        aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    // ORDERS_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "ORDERS_M",
                          8 ) == 0 )
    {
        IDE_TEST( getFakeStat4Orders( aStatement,
                                      ID_FALSE, // Memory Table
                                      aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    // ORDERS_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "ORDERS_D",
                          8 ) == 0 )
    {
        IDE_TEST( getFakeStat4Orders( aStatement,
                                      ID_TRUE, // Disk Table
                                      aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    // LINEITEM_M 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "LINEITEM_M",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4LineItem( aStatement,
                                        ID_FALSE, // Memory Table
                                        aStatInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // LINEITEM_D 테이블인 경우
    if ( idlOS::strMatch( aTableInfo->name,
                          idlOS::strlen( aTableInfo->name ),
                          "LINEITEM_D",
                          10 ) == 0 )
    {
        IDE_TEST( getFakeStat4LineItem( aStatement,
                                        ID_TRUE, // Disk Table
                                        aStatInfo )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoStat::getFakeStat4Region( idBool           aIsDisk,
                             qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     REGION 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt i;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4Region::__FT__" );

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
        case 10:

            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 5;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 2;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // R_PK_REGIONKEY_M 또는 R_PK_REGIONKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "R_PK_REGIONKEY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 5;
                }
                else
                {
                    // Nothing To Do
                }
            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // R_REGIONKEY
                        aStatInfo->colCardInfo[i].columnNDV = 5;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 0;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 4;
                        break;
                    case 1: // R_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 5;
                        break;
                    case 2: // R_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 5;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoStat::getFakeStat4Nation( idBool           aIsDisk,
                             qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     NATION 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt i;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4Nation::__FT__" );

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
        case 10:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 25;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 2;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // N_PK_NATIONKEY_M 또는 N_PK_NATIONKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "N_PK_NATIONKEY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

                // N_IDX_NAME_M 또는 N_IDX_NAME_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 10 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        10,
                                        "N_IDX_NAME",
                                        10 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

                // N_FK_REGIONKEY_M 또는 N_FK_REGIONKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "N_FK_REGIONKEY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 5;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // N_NATIONKEY
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 0;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 24;
                        break;
                    case 1: // N_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        break;
                    case 2: // N_REGIONKEY
                        aStatInfo->colCardInfo[i].columnNDV = 5;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 0;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 4;
                        break;
                    case 3: // N_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoStat::getFakeStat4Supplier( idBool           aIsDisk,
                               qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     SUPPLIER 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt i;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4Supplier::__FT__" );

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 10000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 343;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // S_PK_SUPPKEY_M 또는 S_PK_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "S_PK_SUPPKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 10000;
                }
                else
                {
                    // Nothing To Do
                }

                // S_FK_NATIONKEY_M 또는 S_FK_NATIONKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "S_FK_NATIONKEY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // S_SUPPKEY
                        aStatInfo->colCardInfo[i].columnNDV = 10000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 10000;
                        break;
                    case 1: // S_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 2: // S_ADDRESS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // S_NATIONKEY
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 0;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 24;
                        break;
                    case 4: // S_PHONE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 5: // S_ACCTBAL
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 6: // S_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        case 10:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 100000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 3428;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // S_PK_SUPPKEY_M 또는 S_PK_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "S_PK_SUPPKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 100000;
                }
                else
                {
                    // Nothing To Do
                }

                // S_FK_NATIONKEY_M 또는 S_FK_NATIONKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "S_FK_NATIONKEY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // S_SUPPKEY
                        aStatInfo->colCardInfo[i].columnNDV = 100000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 100000;
                        break;
                    case 1: // S_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 2: // S_ADDRESS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // S_NATIONKEY
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 0;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 24;
                        break;
                    case 4: // S_PHONE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 5: // S_ACCTBAL
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 6: // S_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoStat::getFakeStat4Customer( idBool           aIsDisk,
                               qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     CUSTOMER 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt i;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4Customer::__FT__" );

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 150000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 6024;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // C_PK_CUSTKEY_M 또는 C_PK_CUSTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "C_PK_CUSTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 150000;
                }
                else
                {
                    // Nothing To Do
                }

                // C_IDX_ACCTBAL_M 또는 C_IDX_ACCTBAL_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "C_IDX_ACCTBAL",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 140187;
                }
                else
                {
                    // Nothing To Do
                }

                // C_FK_NATIONKEY_M 또는 C_FK_NATIONKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "C_FK_NATIONKEY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

                // C_IDX_MKTSEGMENT_M 또는 C_IDX_MKTSEGMENT_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 16 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        16,
                                        "C_IDX_MKTSEGMENT",
                                        16 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 5;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // C_CUSTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 150000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 150000;
                        break;
                    case 1: // C_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 2: // C_ADDRESS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // C_NATIONKEY
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 0;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 24;
                        break;
                    case 4: // C_PHONE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 5: // C_ACCTBAL
                        aStatInfo->colCardInfo[i].columnNDV = 140187;
                        *(SDouble*)aStatInfo->colCardInfo[i].minValue =
                            -999.99;
                        *(SDouble*)aStatInfo->colCardInfo[i].maxValue =
                            9999.99;
                        break;
                    case 6: // C_MKTSEGMENT
                        aStatInfo->colCardInfo[i].columnNDV = 5;
                        break;
                    case 7: // C_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        case 10:

            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 1500000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 60417;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // C_PK_CUSTKEY_M 또는 C_PK_CUSTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "C_PK_CUSTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 1500000;
                }
                else
                {
                    // Nothing To Do
                }

                // C_IDX_ACCTBAL_M 또는 C_IDX_ACCTBAL_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "C_IDX_ACCTBAL",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 818834;
                }
                else
                {
                    // Nothing To Do
                }

                // C_FK_NATIONKEY_M 또는 C_FK_NATIONKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "C_FK_NATIONKEY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

                // C_IDX_MKTSEGMENT_M 또는 C_IDX_MKTSEGMENT_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 16 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        16,
                                        "C_IDX_MKTSEGMENT",
                                        16 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 5;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // C_CUSTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 1500000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 1500000;
                        break;
                    case 1: // C_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 2: // C_ADDRESS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // C_NATIONKEY
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 0;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 24;
                        break;
                    case 4: // C_PHONE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 5: // C_ACCTBAL
                        aStatInfo->colCardInfo[i].columnNDV = 818834;
                        *(SDouble*)aStatInfo->colCardInfo[i].minValue =
                            -999.99;
                        *(SDouble*)aStatInfo->colCardInfo[i].maxValue =
                            9999.99;
                        break;
                    case 6: // C_MKTSEGMENT
                        aStatInfo->colCardInfo[i].columnNDV = 5;
                        break;
                    case 7: // C_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoStat::getFakeStat4Part( idBool           aIsDisk,
                           qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     PART 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt i;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4Part::__FT__" );

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 200000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 7367;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // P_PK_PARTKEY_M 또는 P_PK_PARTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "P_PK_PARTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 200000;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_TYPE_M 또는 P_IDX_TYPE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 10 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        10,
                                        "P_IDX_TYPE",
                                        10 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 150;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_SIZE_M 또는 P_IDX_SIZE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 10 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        10,
                                        "P_IDX_SIZE",
                                        10 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 50;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_CONTAINER_M 또는 P_IDX_CONTAINER_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 15 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        15,
                                        "P_IDX_CONTAINER",
                                        15 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 40;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_BRAND_M 또는 P_IDX_BRAND_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 11 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        11,
                                        "P_IDX_BRAND",
                                        11 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

                // P_COMP_SIZE_BRAND_TYPE_M 또는
                // P_COMP_SIZE_BRAND_TYPE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 22 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        22,
                                        "P_COMP_SIZE_BRAND_TYPE",
                                        22 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 123039;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // P_PARTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 200000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 200000;
                        break;
                    case 1: // P_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 2: // P_MFGR
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // P_BRAND
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        break;
                    case 4: // P_TYPE
                        aStatInfo->colCardInfo[i].columnNDV = 150;
                        break;
                    case 5: // P_SIZE
                        aStatInfo->colCardInfo[i].columnNDV = 50;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 50;
                        break;
                    case 6: // P_CONTAINER
                        aStatInfo->colCardInfo[i].columnNDV = 40;
                        break;
                    case 7: // P_RETAILPRICE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 8: // P_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        case 10:

            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 2000000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 73936;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // P_PK_PARTKEY_M 또는 P_PK_PARTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "P_PK_PARTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2000000;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_TYPE_M 또는 P_IDX_TYPE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 10 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        10,
                                        "P_IDX_TYPE",
                                        10 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 150;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_SIZE_M 또는 P_IDX_SIZE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 10 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        10,
                                        "P_IDX_SIZE",
                                        10 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 50;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_CONTAINER_M 또는 P_IDX_CONTAINER_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 15 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        15,
                                        "P_IDX_CONTAINER",
                                        15 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 40;
                }
                else
                {
                    // Nothing To Do
                }

                // P_IDX_BRAND_M 또는 P_IDX_BRAND_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 11 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        11,
                                        "P_IDX_BRAND",
                                        11 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 25;
                }
                else
                {
                    // Nothing To Do
                }

                // P_COMP_SIZE_BRAND_TYPE_M 또는
                // P_COMP_SIZE_BRAND_TYPE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 22 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        22,
                                        "P_COMP_SIZE_BRAND_TYPE",
                                        22 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 187495;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // P_PARTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 2000000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 2000000;
                        break;
                    case 1: // P_NAME
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 2: // P_MFGR
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // P_BRAND
                        aStatInfo->colCardInfo[i].columnNDV = 25;
                        break;
                    case 4: // P_TYPE
                        aStatInfo->colCardInfo[i].columnNDV = 150;
                        break;
                    case 5: // P_SIZE
                        aStatInfo->colCardInfo[i].columnNDV = 50;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 50;
                        break;
                    case 6: // P_CONTAINER
                        aStatInfo->colCardInfo[i].columnNDV = 40;
                        break;
                    case 7: // P_RETAILPRICE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 8: // P_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoStat::getFakeStat4PartSupp( idBool           aIsDisk,
                               qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     PARTSUPP 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt i;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4PartSupp::__FT__" );

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 800000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 23259;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // PS_FK_PARTKEY_M 또는 PS_FK_PARTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "PS_FK_PARTKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 200000;
                }
                else
                {
                    // Nothing To Do
                }

                // PS_IDX_SUPPLYCOST_M 또는 PS_IDX_SUPPLYCOST_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 17 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        17,
                                        "PS_IDX_SUPPLYCOST",
                                        17 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 99865;
                }
                else
                {
                    // Nothing To Do
                }

                // PS_FK_SUPPKEY_M 또는 PS_FK_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "PS_FK_SUPPKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 10000;
                }
                else
                {
                    // Nothing To Do
                }

                // PS_PK_PARTKEY_SUPPKEY_M 또는 PS_PK_PARTKEY_SUPPKEY_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 21 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        21,
                                        "PS_PK_PARTKEY_SUPPKEY",
                                        21 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 800000;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // PS_PARTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 200000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 200000;
                        break;
                    case 1: // PS_SUPPKEY
                        aStatInfo->colCardInfo[i].columnNDV = 10000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 10000;
                        break;
                    case 2: // PS_AVAILQTY
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // PS_SUPPLYCOST
                        aStatInfo->colCardInfo[i].columnNDV = 99865;
                        *(SDouble*)aStatInfo->colCardInfo[i].minValue = 1.0;
                        *(SDouble*)aStatInfo->colCardInfo[i].maxValue = 1000.0;
                        break;
                    case 4: // PS_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        case 10:

            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 8000000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 233450;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // PS_FK_PARTKEY_M 또는 PS_FK_PARTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "PS_FK_PARTKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2000000;
                }
                else
                {
                    // Nothing To Do
                }

                // PS_IDX_SUPPLYCOST_M 또는 PS_IDX_SUPPLYCOST_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 17 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        17,
                                        "PS_IDX_SUPPLYCOST",
                                        17 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 99901;
                }
                else
                {
                    // Nothing To Do
                }

                // PS_FK_SUPPKEY_M 또는 PS_FK_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "PS_FK_SUPPKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 100000;
                }
                else
                {
                    // Nothing To Do
                }

                // PS_PK_PARTKEY_SUPPKEY_M 또는 PS_PK_PARTKEY_SUPPKEY_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 21 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        21,
                                        "PS_PK_PARTKEY_SUPPKEY",
                                        21 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 8000000;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // PS_PARTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 2000000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 2000000;
                        break;
                    case 1: // PS_SUPPKEY
                        aStatInfo->colCardInfo[i].columnNDV = 100000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 100000;
                        break;
                    case 2: // PS_AVAILQTY
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // PS_SUPPLYCOST
                        aStatInfo->colCardInfo[i].columnNDV = 99901;
                        *(SDouble*)aStatInfo->colCardInfo[i].minValue = 1.0;
                        *(SDouble*)aStatInfo->colCardInfo[i].maxValue = 1000.0;
                        break;
                    case 4: // PS_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoStat::getFakeStat4Orders( qcStatement    * aStatement,
                             idBool           aIsDisk,
                             qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     ORDERS 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt          i;
    mtcTemplate * sTemplate;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4Orders::__FT__" );

    sTemplate = &(QC_SHARED_TMPLATE(aStatement)->tmplate);

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 1500000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 38108;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // O_PK_ORDERKEY_M 또는 O_PK_ORDERKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "O_PK_ORDERKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 1500000;
                }
                else
                {
                    // Nothing To Do
                }

                // O_FK_CUSTKEY_M 또는 O_FK_CUSTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "O_FK_CUSTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 99996;
                }
                else
                {
                    // Nothing To Do
                }

                // O_IDX_ORDERDATE_M 또는 O_IDX_ORDERDATE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 15 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        15,
                                        "O_IDX_ORDERDATE",
                                        15 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2406;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // O_ORDERKEY
                        aStatInfo->colCardInfo[i].columnNDV = 1500000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 6000000;
                        break;
                    case 1: // O_CUSTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 99996;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 149999;
                        break;
                    case 2: // O_ORDERSTATUS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // O_TOTALPRICE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 4: // O_ORDERDATE
                        aStatInfo->colCardInfo[i].columnNDV = 2406;

                        // fix BUG-31884
                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].minValue,
                                                            (UChar*)"01-JAN-1992",
                                                            ID_SIZEOF( "01-JAN-1992" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );

                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].maxValue,
                                                            (UChar*)"02-AUG-1998",
                                                            ID_SIZEOF( "02-AUG-1998" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );
                        break;
                    case 5: // O_ORDERPRIORITY
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 6: // O_CLERK
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 7: // O_SHIPPRIORITY
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 8: // O_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        case 10:

            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 15000000;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 382414;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // O_PK_ORDERKEY_M 또는 O_PK_ORDERKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "O_PK_ORDERKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 15000000;
                }
                else
                {
                    // Nothing To Do
                }

                // O_FK_CUSTKEY_M 또는 O_FK_CUSTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "O_FK_CUSTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 999982;
                }
                else
                {
                    // Nothing To Do
                }

                // O_IDX_ORDERDATE_M 또는 O_IDX_ORDERDATE_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 15 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        15,
                                        "O_IDX_ORDERDATE",
                                        15 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2406;
                }
                else
                {
                    // Nothing To Do
                }

            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // O_ORDERKEY
                        aStatInfo->colCardInfo[i].columnNDV = 15000000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 60000000;
                        break;
                    case 1: // O_CUSTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 999982;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 1499999;
                        break;
                    case 2: // O_ORDERSTATUS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 3: // O_TOTALPRICE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 4: // O_ORDERDATE
                        aStatInfo->colCardInfo[i].columnNDV = 2406;

                        // fix BUG-31884
                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].minValue,
                                                            (UChar*)"01-JAN-1992",
                                                            ID_SIZEOF( "01-JAN-1992" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );

                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].maxValue,
                                                            (UChar*)"02-AUG-1998",
                                                            ID_SIZEOF( "02-AUG-1998" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );
                        break;
                    case 5: // O_ORDERPRIORITY
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 6: // O_CLERK
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 7: // O_SHIPPRIORITY
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 8: // O_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoStat::getFakeStat4LineItem( qcStatement    * aStatement,
                               idBool           aIsDisk,
                               qmoStatistics  * aStatInfo )
{
/***********************************************************************
 *
 * Description :
 *     해당 함수는 테스트 용도로만 사용되어야 함.
 *     TPC-H 용 테이블일 경우 가상의 통계 정보를 구축한다.
 *
 * Implementation :
 *
 *     LINEITEM 테이블 용 가상 통계 정보를 구축한다.
 *
 ***********************************************************************/

    UInt          i;
    mtcTemplate * sTemplate;

    IDU_FIT_POINT_FATAL( "qmoStat::getFakeStat4LineItem::__FT__" );

    sTemplate = &(QC_SHARED_TMPLATE(aStatement)->tmplate);
    
    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aStatInfo != NULL );

    //-----------------------------
    // Scale Factor에 따른 통계 정보 구축
    //-----------------------------

    switch ( QCU_FAKE_TPCH_SCALE_FACTOR )
    {
        case 1:
            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 6001215;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 176355;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // L_FK_ORDERKEY_M 또는 L_FK_ORDERKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "L_FK_ORDERKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 1500000;
                }
                else
                {
                    // Nothing To Do
                }

                // L_FK_PARTKEY_M 또는 L_FK_PARTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "L_FK_PARTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 200000;
                }
                else
                {
                    // Nothing To Do
                }

                // L_FK_SUPPKEY_M 또는 L_FK_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "L_FK_SUPPKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 10000;
                }
                else
                {
                    // Nothing To Do
                }

                // L_IDX_RECEIPTDATE_M 또는 L_IDX_RECEIPTDATE_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 17 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        17,
                                        "L_IDX_RECEIPTDATE",
                                        17 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2554;
                }
                else
                {
                    // Nothing To Do
                }

                // L_IDX_SHIPDATE_M 또는 L_IDX_SHIPDATE_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "L_IDX_SHIPDATE",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2526;
                }
                else
                {
                    // Nothing To Do
                }

                // L_IDX_QUANTITY_M 또는 L_IDX_QUANTITY_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "L_IDX_QUANTITY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 50;
                }
                else
                {
                    // Nothing To Do
                }

                // L_PK_ORDERKEY_LINENUMBER_M 또는
                // L_PK_ORDERKEY_LINENUMBER_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 24 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        24,
                                        "L_PK_ORDERKEY_LINENUMBER",
                                        24 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 6001215;
                }
                else
                {
                    // Nothing To Do
                }

                // L_COMP_PARTKEY_SUPPKEY_M 또는
                // L_COMP_PARTKEY_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 22 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        22,
                                        "L_COMP_PARTKEY_SUPPKEY",
                                        22 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 799541;
                }
                else
                {
                    // Nothing To Do
                }
            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // L_ORDERKEY
                        aStatInfo->colCardInfo[i].columnNDV = 1500000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 6000000;
                        break;
                    case 1: // L_PARTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 200000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 200000;
                        break;
                    case 2: // L_SUPPKEY
                        aStatInfo->colCardInfo[i].columnNDV = 10000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 10000;
                        break;
                    case 3: // L_LINENUMBER
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 4: // L_QUANTITY
                        aStatInfo->colCardInfo[i].columnNDV = 50;
                        *(SDouble*)aStatInfo->colCardInfo[i].minValue = 1.0;
                        *(SDouble*)aStatInfo->colCardInfo[i].maxValue = 50.0;
                        break;
                    case 5: // L_EXTENDEDPRICE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 6: // L_DISCOUNT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 7: // L_TAX
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 8: // L_RETURNFLAG
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 9: // L_LINESTATUS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 10: // L_SHIPDATE
                        aStatInfo->colCardInfo[i].columnNDV = 2526;

                        // fix BUG-31884
                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].minValue,
                                                            (UChar*)"02-JAN-1992",
                                                            ID_SIZEOF( "02-JAN-1992" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );

                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].maxValue,
                                                            (UChar*)"01-DEC-1998",
                                                            ID_SIZEOF( "01-DEC-1998" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );
                        break;
                    case 11: // L_COMMITDATE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 12: // L_RECEIPTDATE
                        aStatInfo->colCardInfo[i].columnNDV = 2554;

                        // fix BUG-31884
                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].minValue,
                                                            (UChar*)"04-JAN-1992",
                                                            ID_SIZEOF( "04-JAN-1992" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );

                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].maxValue,
                                                            (UChar*)"31-DEC-1998",
                                                            ID_SIZEOF( "31-DEC-1998" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );
                        break;
                    case 13: // L_SHIPINSTRUCT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 14: // L_SHIPMODE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 15: // L_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;

                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        case 10:

            //-----------------------
            // 테이블 통계 정보
            //-----------------------

            aStatInfo->totalRecordCnt = 59986052;
            if ( aIsDisk == ID_TRUE )
            {
                aStatInfo->pageCnt = 1766969;
            }
            else
            {
                aStatInfo->pageCnt = 0;
            }

            //-----------------------
            // 인덱스 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->indexCnt; i++ )
            {
                // L_FK_ORDERKEY_M 또는 L_FK_ORDERKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 13 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        13,
                                        "L_FK_ORDERKEY",
                                        13 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 15000000;
                }
                else
                {
                    // Nothing To Do
                }

                // L_FK_PARTKEY_M 또는 L_FK_PARTKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "L_FK_PARTKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2000000;
                }
                else
                {
                    // Nothing To Do
                }

                // L_FK_SUPPKEY_M 또는 L_FK_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 12 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        12,
                                        "L_FK_SUPPKEY",
                                        12 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 100000;
                }
                else
                {
                    // Nothing To Do
                }

                // L_IDX_RECEIPTDATE_M 또는 L_IDX_RECEIPTDATE_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 17 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        17,
                                        "L_IDX_RECEIPTDATE",
                                        17 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2555;
                }
                else
                {
                    // Nothing To Do
                }

                // L_IDX_SHIPDATE_M 또는 L_IDX_SHIPDATE_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "L_IDX_SHIPDATE",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 2526;
                }
                else
                {
                    // Nothing To Do
                }

                // L_IDX_QUANTITY_M 또는 L_IDX_QUANTITY_D 를
                // 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 14 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        14,
                                        "L_IDX_QUANTITY",
                                        14 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 50;
                }
                else
                {
                    // Nothing To Do
                }

                // L_PK_ORDERKEY_LINENUMBER_M 또는
                // L_PK_ORDERKEY_LINENUMBER_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 24 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        24,
                                        "L_PK_ORDERKEY_LINENUMBER",
                                        24 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 59986052;
                }
                else
                {
                    // Nothing To Do
                }

                // L_COMP_PARTKEY_SUPPKEY_M 또는
                // L_COMP_PARTKEY_SUPPKEY_D 를 위한 통계 정보
                if ( ( idlOS::strlen( aStatInfo->idxCardInfo[i].index->name ) >= 22 ) &&
                     ( idlOS::strMatch( aStatInfo->idxCardInfo[i].index->name,
                                        22,
                                        "L_COMP_PARTKEY_SUPPKEY",
                                        22 ) == 0 ) )
                {
                    aStatInfo->idxCardInfo[i].KeyNDV = 7995955;
                }
                else
                {
                    // Nothing To Do
                }
            }

            //-----------------------
            // 컬럼 통계 정보
            //-----------------------

            for ( i = 0; i < aStatInfo->columnCnt; i++ )
            {
                //
                switch ( i )
                {
                    case 0: // L_ORDERKEY
                        aStatInfo->colCardInfo[i].columnNDV = 15000000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 60000000;
                        break;
                    case 1: // L_PARTKEY
                        aStatInfo->colCardInfo[i].columnNDV = 2000000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 2000000;
                        break;
                    case 2: // L_SUPPKEY
                        aStatInfo->colCardInfo[i].columnNDV = 100000;
                        *(SInt*)aStatInfo->colCardInfo[i].minValue = 1;
                        *(SInt*)aStatInfo->colCardInfo[i].maxValue = 100000;
                        break;
                    case 3: // L_LINENUMBER
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 4: // L_QUANTITY
                        aStatInfo->colCardInfo[i].columnNDV = 50;
                        *(SDouble*)aStatInfo->colCardInfo[i].minValue = 1.0;
                        *(SDouble*)aStatInfo->colCardInfo[i].maxValue = 50.0;
                        break;
                    case 5: // L_EXTENDEDPRICE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 6: // L_DISCOUNT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 7: // L_TAX
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 8: // L_RETURNFLAG
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 9: // L_LINESTATUS
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 10: // L_SHIPDATE
                        aStatInfo->colCardInfo[i].columnNDV = 2526;

                        // fix BUG-31884
                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].minValue,
                                                            (UChar*)"02-JAN-1992",
                                                            ID_SIZEOF( "02-JAN-1992" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );

                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].maxValue,
                                                            (UChar*)"01-DEC-1998",
                                                            ID_SIZEOF( "01-DEC-1998" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );
                        break;
                    case 11: // L_COMMITDATE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 12: // L_RECEIPTDATE
                        aStatInfo->colCardInfo[i].columnNDV = 2555;

                        // fix BUG-31884
                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].minValue,
                                                            (UChar*)"03-JAN-1992",
                                                            ID_SIZEOF( "03-JAN-1992" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );

                        IDE_TEST( mtdDateInterface::toDate( (mtdDateType *)aStatInfo->colCardInfo[i].maxValue,
                                                            (UChar*)"31-DEC-1998",
                                                            ID_SIZEOF( "31-DEC-1998" ) - 1,
                                                            (UChar*)sTemplate->dateFormat,
                                                            idlOS::strlen( sTemplate->dateFormat ) )
                                  != IDE_SUCCESS );
                        break;
                    case 13: // L_SHIPINSTRUCT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 14: // L_SHIPMODE
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;
                    case 15: // L_COMMENT
                        aStatInfo->colCardInfo[i].columnNDV = 10;
                        break;

                    default:
                        IDE_DASSERT(0);
                        break;
                }
            }

            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qmoStat::getStatInfo4RemoteTable( qcStatement     * aStatement,
                                         qcmTableInfo    * aTableInfo,
                                         qmoStatistics  ** aStatInfo )
{
    UInt i = 0;
    qmoStatistics * sStatistics = NULL;

    IDU_FIT_POINT_FATAL( "qmoStat::getStatInfo4RemoteTable::__FT__" );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( *sStatistics ),
                                               (void **)&sStatistics )
              != IDE_SUCCESS );

    /* no index */
    sStatistics->indexCnt = 0;
    sStatistics->idxCardInfo = NULL;

    sStatistics->columnCnt = aTableInfo->columnCount;

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoColCardInfo ) * aTableInfo->columnCount,
                                               (void **)&( sStatistics->colCardInfo ) )
              != IDE_SUCCESS );

    for ( i = 0; i < sStatistics->columnCnt; i++ )
    {
        sStatistics->colCardInfo[ i ].column         = aTableInfo->columns[ i ].basicInfo;
        sStatistics->colCardInfo[ i ].flag           = QMO_STAT_CLEAR;
        sStatistics->colCardInfo[ i ].isValidStat    = ID_TRUE;

        sStatistics->colCardInfo[ i ].columnNDV      = QMO_STAT_COLUMN_NDV;
        sStatistics->colCardInfo[ i ].nullValueCount = QMO_STAT_COLUMN_NULL_VALUE_COUNT;
        sStatistics->colCardInfo[ i ].avgColumnLen   = QMO_STAT_COLUMN_AVG_LEN;

        /* TASK-7219 Shard Transformer Refactoring
         *  - To Fix BUG-8697
         */
        if ( ( sStatistics->colCardInfo[ i ].column->module->flag & MTD_SELECTIVITY_MASK )
             == MTD_SELECTIVITY_ENABLE )
        {
            sStatistics->colCardInfo[ i ].column->module->null( sStatistics->colCardInfo[ i ].column,
                                                                sStatistics->colCardInfo[ i ].minValue );
            sStatistics->colCardInfo[ i ].column->module->null( sStatistics->colCardInfo[ i ].column,
                                                                sStatistics->colCardInfo[ i ].maxValue );
        }
        else
        {
            /* Nothing to do */
        }
    }

    sStatistics->totalRecordCnt = QMO_STAT_TABLE_RECORD_COUNT;
    sStatistics->pageCnt        = QMO_STAT_TABLE_DISK_PAGE_COUNT;
    sStatistics->avgRowLen      = QMO_STAT_TABLE_AVG_ROW_LEN;
    sStatistics->readRowTime    = QMO_STAT_TABLE_DISK_ROW_READ_TIME;
    sStatistics->isValidStat    = ID_TRUE;

    *aStatInfo = sStatistics;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoStat::calculateSamplePercentage( qcmTableInfo   * aTableInfo,
                                           UInt             aAutoStatsLevel,
                                           SFloat         * aPercentage )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2492 Dynamic sample selection
 *    프로퍼티에 맞는 % 를 계산한다.
 *
 * Implementation :
 *    1. 프로퍼티 / 실제 PAGE 갯수
 *    2. 결과값 보정 : 0 <= 결과값 <= 1
 *
 ***********************************************************************/

    SLong   sCurrentPage    = 0;
    SLong   sSamplePage     = 0;
    SFloat  sPercentage     = 0.0;

    IDU_FIT_POINT_FATAL( "qmoStat::calculateSamplePercentage::__FT__" );

    IDE_DASSERT ( aAutoStatsLevel != 0 );

    IDE_TEST( smiStatistics::getTableStatNumPage( aTableInfo->tableHandle,
                                                  ID_TRUE,
                                                  &sCurrentPage )
              != IDE_SUCCESS);

    switch ( aAutoStatsLevel )
    {
        case 1 :
            sSamplePage = 32;
            break;
        case 2 :
            sSamplePage = 64;
            break;
        case 3 :
            sSamplePage = 128;
            break;
        case 4 :
            sSamplePage = 256;
            break;
        case 5 :
            sSamplePage = 512;
            break;
        case 6 :
            sSamplePage = 1024;
            break;
        case 7 :
            sSamplePage = 4096;
            break;
        case 8 :
            sSamplePage = 16384;
            break;
        case 9 :
            sSamplePage = 65536;
            break;
        case 10:
            sSamplePage = sCurrentPage;
            break;
        default :
            sSamplePage = 0;
            break;
    }

    sPercentage = (SFloat)sSamplePage / (SFloat)sCurrentPage;

    if ( sPercentage < 0.0f )
    {
        sPercentage = 0.0f;
    }
    else if ( sPercentage > 1.0f )
    {
        sPercentage = 1.0f;
    }
    else
    {
        // Nothing to do.
    }

    *aPercentage = sPercentage;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
