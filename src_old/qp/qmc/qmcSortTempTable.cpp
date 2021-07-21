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
 * $Id: qmcSortTempTable.cpp 86786 2020-02-27 08:04:12Z donovan.seo $
 *
 * Description :
 *     Sort Temp Table을 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 *
 **********************************************************************/

#include <qmoKeyRange.h>
#include <qmcSortTempTable.h>
#include <qmxResultCache.h>

IDE_RC
qmcSortTemp::init( qmcdSortTemp * aTempTable,
                   qcTemplate   * aTemplate,
                   UInt           aMemoryIdx,
                   qmdMtrNode   * aRecordNode,
                   qmdMtrNode   * aSortNode,
                   SDouble        aStoreRowCount,
                   UInt           aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Sort Temp Table을 초기화한다.
 *
 * Implementation :
 *    Memory Temp Table과 Disk Temp Table의 사용을 구분하여
 *    그에 맞는 초기화를 수행한다.
 *
 *    BUG-38290
 *    Temp table 은 내부에 qcTemplate 과 qmcMemory 를 가지고
 *    temp table 생성에 사용한다.
 *    이 두가지는 temp table 을 init 할 때의 template 과 그 template 에
 *    연결된 QMX memory 이다.
 *    만약 temp table init 시점과 temp table build 시점에 서로 다른
 *    template 을 사용해야 한다면 이 구조가 변경되어야 한다.
 *    Parallel query 대상이 증가하면서 temp table build 가 parallel 로
 *    진행될 경우 이 내용을 고려해야 한다.
 *
 *    Temp table build 는 temp table 이 존재하는 plan node 의
 *    init 시점에 실행된다.
 *    하지만 현재 parallel query 는 partitioned table 이나 HASH, SORT,
 *    GRAG  노드에만 PRLQ 노드를 생성, parallel 실행하므로 
 *    temp table 이 존재하는 plan node 를 직접 init  하지 않는다.
 *
 *    다만 subqeury filter 내에서 temp table 을 사용할 경우는 예외가 있을 수
 *    있으나, subquery filter 는 plan node 가 실행될 때 초기화 되므로
 *    동시에 temp table  이 생성되는 일은 없다.
 *    (qmcTempTableMgr::addTempTable 의 관련 주석 참조)
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::init"));

    mtcColumn  * sColumn;
    UInt         i;

    // 적합성 검사
    IDE_DASSERT( aRecordNode != NULL );

    aTempTable->flag       = aFlag;
    aTempTable->mTemplate  = aTemplate;
    aTempTable->recordNode = aRecordNode;
    aTempTable->sortNode   = aSortNode;
    
    IDE_DASSERT( aTempTable->recordNode->dstTuple->rowOffset > 0 );
    aTempTable->mtrRowSize = aTempTable->recordNode->dstTuple->rowOffset;
    aTempTable->nullRowSize = aTempTable->mtrRowSize;

    aTempTable->recordCnt = 0;

    aTempTable->memoryMgr = NULL;
    aTempTable->nullRow = NULL;
    aTempTable->range = NULL;
    aTempTable->rangeArea = NULL;
    aTempTable->rangeAreaSize = 0;
    aTempTable->memoryTemp = NULL;
    aTempTable->diskTemp = NULL;

    aTempTable->existTempType = ID_FALSE;
    aTempTable->insertRow = NULL;

    aTempTable->memoryIdx = aMemoryIdx;
    if ( aMemoryIdx == ID_UINT_MAX )
    {
        aTempTable->memory = aTemplate->stmt->qmxMem;
    }
    else
    {
        aTempTable->memory = aTemplate->resultCache.memArray[aMemoryIdx];
    }

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //    1. Null Row 생성
        //    2. Memory Sort Temp Table 초기화
        //    3. Record용 Memory 관리자 초기화
        //-----------------------------------------

        // Memory Sort Temp Table 객체의 생성 및 초기화
        IDU_FIT_POINT( "qmcSortTemp::init::alloc::memoryTemp",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->memory->alloc( ID_SIZEOF(qmcdMemSortTemp),
                                             (void**)& aTempTable->memoryTemp )
                  != IDE_SUCCESS );

        IDE_TEST( qmcMemSort::init( aTempTable->memoryTemp,
                                    aTempTable->mTemplate,
                                    aTempTable->memory,
                                    aTempTable->sortNode ) != IDE_SUCCESS );

        // Record 공간 할당을 위한 메모리 관리자 초기화
        IDU_FIT_POINT( "qmcSortTemp::init::alloc::memoryMgr",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->memory->alloc(
                  ID_SIZEOF(qmcMemory),
                  (void**) & aTempTable->memoryMgr ) != IDE_SUCCESS);

        aTempTable->memoryMgr = new (aTempTable->memoryMgr)qmcMemory();

        /* BUG-38290 */
        aTempTable->memoryMgr->init( aTempTable->mtrRowSize );

        // PROJ-2362 memory temp 저장 효율성 개선
        for ( i = 0, sColumn = aTempTable->recordNode->dstTuple->columns;
              i < aTempTable->recordNode->dstTuple->columnCount;
              i++, sColumn++ )
        {
            if ( SMI_COLUMN_TYPE_IS_TEMP( sColumn->column.flag ) == ID_TRUE )
            {
                aTempTable->existTempType = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
            
        // 미리 버퍼를 할당한다.
        if ( aTempTable->existTempType == ID_TRUE )
        {
            /* BUG-38290 */
            IDE_TEST( aTempTable->memoryMgr->cralloc(
                    aTempTable->memory,
                    aTempTable->mtrRowSize,
                    &(aTempTable->insertRow) )
                != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }

        // Memory Temp Table을 위한 Null Row를 생성한다.
        IDE_TEST( makeMemNullRow( aTempTable ) != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk Temp Table을 위한 Null Row 생성은
        // Disk Temp Table에서 하며,
        // Null Row의 획득은 Disk Temp Table로부터 얻어온다.

        // Disk Sort Temp Table 객체의 생성 및 초기화
        IDU_FIT_POINT( "qmcSortTemp::init::alloc::diskTemp",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                  ID_SIZEOF(qmcdDiskSortTemp),
                  (void**)& aTempTable->diskTemp ) != IDE_SUCCESS);

        IDE_TEST( qmcDiskSort::init( aTempTable->diskTemp,
                                     aTempTable->mTemplate,
                                     aTempTable->recordNode,
                                     aTempTable->sortNode,
                                     aStoreRowCount,
                                     aTempTable->mtrRowSize,
                                     aTempTable->flag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::clear( qmcdSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     Temp Table내의 모든 데이터를 제거하고, 초기화한다.
 *     Dependency 변경에 의하여 중간 결과를 재구성할 필요가 있을 때,
 *     사용된다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::clear"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::clear"));

    aTempTable->recordCnt = 0;

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //     1. Record를 위해 할당된 공간 제거
        //     2. Memory Sort Temp Table 의 clear
        //-----------------------------------------

        // Memory Sort Temp Table의 clear
        IDE_TEST( qmcMemSort::clear( aTempTable->memoryTemp )
                  != IDE_SUCCESS );
            
        // 미리 버퍼를 할당한다.
        if ( aTempTable->existTempType == ID_TRUE )
        {
            /* BUG-38290 */
            IDE_TEST( aTempTable->memoryMgr->cralloc(
                    aTempTable->memory,
                    aTempTable->mtrRowSize,
                    &(aTempTable->insertRow) )
                != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk Sort Temp Table의 clear
        IDE_TEST( qmcDiskSort::clear( aTempTable->diskTemp )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::clearHitFlag( qmcdSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     모든 Record의 Hit Flag을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::clearHitFlag"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::clearHitFlag"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::clearHitFlag( aTempTable->memoryTemp )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::clearHitFlag( aTempTable->diskTemp )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::alloc( qmcdSortTemp  * aTempTable,
                    void         ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    Record를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::alloc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::alloc"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            *aRow = aTempTable->insertRow;
        }
        else
        {
            IDU_FIT_POINT( "qmcSortTemp::alloc::alloc::aRow",
                            idERR_ABORT_InsufficientMemory );

            /* BUG-38290 */
            IDE_TEST( aTempTable->memoryMgr->alloc(
                    aTempTable->memory,
                    aTempTable->mtrRowSize,
                    aRow )
                != IDE_SUCCESS);
           
            // PROJ-2462 ResultCache
            // ResultCache가 사용되면 CacheMemory 와 qmxMemory의 합게로
            // ExecuteMemoryMax를 체크한다.
            if ( ( aTempTable->mTemplate->resultCache.count > 0 ) &&
                 ( ( aTempTable->mTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
                   == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
                 ( ( aTempTable->mTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
                   == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
            {
                IDE_TEST( qmxResultCache::checkExecuteMemoryMax( aTempTable->mTemplate,
                                                                 aTempTable->memoryIdx )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk Temp Table을 사용하는 경우
        // 별도의 Memory 공간을 할당받지 않고 처음에 할당한
        // 메모리 영역을 반복적으로 사용한다.

        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::addRow( qmcdSortTemp * aTempTable,
                     void         * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table에 Record를 삽입한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::addRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::addRow"));

    qmcMemSortElement * sElement;
    void              * sRow;

    IDE_DASSERT( aRow != NULL );

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            IDE_TEST( makeTempTypeRow( aTempTable,
                                       aRow,
                                       & sRow )
                      != IDE_SUCCESS );
        }
        else
        {
            sRow = aRow;
        }
        
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //    1. Hit Flag을 초기화한다.
        //    2. Memory Temp Table에 삽입한다.
        //-----------------------------------------

        // Hit Flag을 초기화한다.
        sElement = (qmcMemSortElement*) sRow;

        sElement->flag  = QMC_ROW_INITIALIZE;
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_FALSE;

        // Memory Temp Table에 삽입한다.
        IDE_TEST( qmcMemSort::attach( aTempTable->memoryTemp, sRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::insert( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    aTempTable->recordCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::makeTempTypeRow( qmcdSortTemp  * aTempTable,
                              void          * aRow,
                              void         ** aExtRow )
{
/***********************************************************************
 *
 * Description :
 *    일반 memory row를 memory 확장 row 형태로 만든다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::makeTempTypeRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::makeTempTypeRow"));

    mtcColumn  * sColumn;
    UInt         sRowSize;
    UInt         sActualSize;
    void       * sExtRow;
    idBool       sIsTempType;
    UInt         i;

    sRowSize = aTempTable->mtrRowSize;
    
    for ( i = 0, sColumn = aTempTable->recordNode->dstTuple->columns;
          i < aTempTable->recordNode->dstTuple->columnCount;
          i++, sColumn++ )
    {
        if ( SMI_COLUMN_TYPE_IS_TEMP( sColumn->column.flag ) == ID_TRUE )
        {
            sActualSize = sColumn->module->actualSize( sColumn,
                                                       sColumn->column.value );
            
            sRowSize = idlOS::align( sRowSize, sColumn->module->align );
            sRowSize += sActualSize;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    sRowSize = idlOS::align8( sRowSize );

    IDE_DASSERT( aTempTable->mtrRowSize < sRowSize );

    /* BUG-38290 */
    IDE_TEST( aTempTable->memoryMgr->alloc(
            aTempTable->mTemplate->stmt->qmxMem,
            sRowSize,
            & sExtRow )
        != IDE_SUCCESS);

    // fixed row 복사
    idlOS::memcpy( (SChar*)sExtRow, (SChar*)aRow,
                   aTempTable->mtrRowSize );

    sRowSize = aTempTable->mtrRowSize;
    
    for ( i = 0, sColumn = aTempTable->recordNode->dstTuple->columns;
          i < aTempTable->recordNode->dstTuple->columnCount;
          i++, sColumn++ )
    {
        // offset 저장
        if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_TEMP_1B )
        {
            sIsTempType = ID_TRUE;
                
            sRowSize = idlOS::align( sRowSize, sColumn->module->align );
            IDE_DASSERT( sRowSize < 256 );
                
            *(UChar*)((UChar*)sExtRow + sColumn->column.offset) = (UChar)sRowSize;
        }
        else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_2B )
        {
            sIsTempType = ID_TRUE;
                
            sRowSize = idlOS::align( sRowSize, sColumn->module->align );
            IDE_DASSERT( sRowSize < 65536 );

            *(UShort*)((UChar*)sExtRow + sColumn->column.offset) = (UShort)sRowSize;
        }
        else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_TEMP_4B )
        {
            sIsTempType = ID_TRUE;
                
            sRowSize = idlOS::align( sRowSize, sColumn->module->align );
            
            *(UInt*)((UChar*)sExtRow + sColumn->column.offset) = sRowSize;
        }
        else
        {
            sIsTempType = ID_FALSE;
        }

        if ( sIsTempType == ID_TRUE )
        {
            sActualSize = sColumn->module->actualSize( sColumn,
                                                       sColumn->column.value );

            idlOS::memcpy( (SChar*)sExtRow + sRowSize,
                           (SChar*)sColumn->column.value,
                           sActualSize );
            
            sRowSize += sActualSize;
        }
        else
        {
            // Nothing to do.
        }
    }

    *aExtRow = sExtRow;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::sort( qmcdSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     저장된 Row들의 정렬을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::sort"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------
        IDE_TEST( qmcMemSort::sort( aTempTable->memoryTemp )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------
        // PROJ-1431 : bottom-up index build를 위해 row를 모두 채운 후
        // index(sparse cluster b-tree)를 build함
        if( aTempTable->diskTemp->sortNode != NULL )
        {
            IDE_TEST( qmcDiskSort::sort( aTempTable->diskTemp )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::shiftAndAppend( qmcdSortTemp * aTempTable,
                             void         * aRow,
                             void        ** aReturnRow )
{
/***********************************************************************
 *
 * Description :
 *     Limit Sorting을 수행한다.
 *
 * Implementation :
 *     Memory Temp Table일 경우만 유효함
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::shiftAndAppend"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void   * sRow;
    
    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            IDE_TEST( makeTempTypeRow( aTempTable,
                                       aRow,
                                       & sRow )
                      != IDE_SUCCESS );
        }
        else
        {
            sRow = aRow;
        }
        
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::shiftAndAppend( aTempTable->memoryTemp,
                                              sRow,
                                              aReturnRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Memory Temp Table에서만 사용 가능함.
        IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::changeMinMax( qmcdSortTemp * aTempTable,
                           void         * aRow,
                           void        ** aReturnRow )
{
/***********************************************************************
 *
 * Description :
 *     Min-Max 저장을 위한 Limit Sorting을 수행한다.
 *
 * Implementation :
 *     Memory Temp Table일 경우만 유효함
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::changeMinMax"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sRow;
    
    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            IDE_TEST( makeTempTypeRow( aTempTable,
                                       aRow,
                                       & sRow )
                      != IDE_SUCCESS );
        }
        else
        {
            sRow = aRow;
        }
        
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::changeMinMax( aTempTable->memoryTemp,
                                            sRow,
                                            aReturnRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Memory Temp Table에서만 사용 가능함.
        IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::getFirstSequence( qmcdSortTemp * aTempTable,
                               void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 순차 검색
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::getFirstSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::getFirstSequence( aTempTable->memoryTemp,
                                                aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getFirstSequence( aTempTable->diskTemp,
                                                 aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcSortTemp::getNextSequence( qmcdSortTemp * aTempTable,
                              void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::getNextSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::getNextSequence"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::getNextSequence( aTempTable->memoryTemp,
                                               aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getNextSequence( aTempTable->diskTemp,
                                                aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcSortTemp::getFirstRange( qmcdSortTemp * aTempTable,
                            qtcNode      * aRangePredicate,
                            void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Range 검색
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::getFirstRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::getFirstRange"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Key Range를 생성
        IDE_TEST( makeMemKeyRange( aTempTable, aRangePredicate )
                  != IDE_SUCCESS );

        // 생성한 Key Range를 이용한 검색
        IDE_TEST( qmcMemSort::getFirstRange( aTempTable->memoryTemp,
                                             aTempTable->range,
                                             aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Key Range의 생성을 위해서는 Index 정보등이 필요하다.
        // 따라서, Disk Temp Table내에서 Key Range를 생성하여
        // 처리한다.
        IDE_TEST( qmcDiskSort::getFirstRange( aTempTable->diskTemp,
                                              aRangePredicate,
                                              aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmcSortTemp::getLastKey( qmcdSortTemp * aTempTable,
                                SLong        * aLastKey )
{
    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        *aLastKey = aTempTable->memoryTemp->mLastKey;
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmcSortTemp::getLastKey",
                                  "unsupported feature" ));
    }
    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

IDE_RC qmcSortTemp::setLastKey( qmcdSortTemp * aTempTable,
                                SLong          aLastKey )
{
    if ( ( aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE )
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        aTempTable->memoryTemp->mLastKey = aLastKey;
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmcSortTemp::setLastKey",
                                  "unsupported feature" ));
    }
    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

IDE_RC
qmcSortTemp::getNextRange( qmcdSortTemp * aTempTable,
                           void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::getNextRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::getNextRange"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // 이미 생성된 Key Range를 이용한 검색
        IDE_TEST( qmcMemSort::getNextRange( aTempTable->memoryTemp, aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getNextRange( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qmcSortTemp::getFirstHit( qmcdSortTemp * aTempTable,
                                 void        ** aRow )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *     첫번째 Hit 된 Row검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::getFirstHit( aTempTable->memoryTemp, aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getFirstHit( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcSortTemp::getNextHit( qmcdSortTemp * aTempTable,
                                void        ** aRow )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *     다음 Hit 된 Row검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::getNextHit( aTempTable->memoryTemp, aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getNextHit( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcSortTemp::getFirstNonHit( qmcdSortTemp * aTempTable,
                             void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    Hit되지 않은 Record를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::getFirstNonHit( aTempTable->memoryTemp, aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getFirstNonHit( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcSortTemp::getNextNonHit( qmcdSortTemp * aTempTable,
                            void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    Hit되지 않은 Record를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcMemSort::getNextNonHit( aTempTable->memoryTemp, aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getNextNonHit( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcSortTemp::getNullRow( qmcdSortTemp * aTempTable,
                         void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    NULL Row를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qmdMtrNode * sNode;
    
    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_DASSERT( aTempTable->nullRow != NULL );
        
        *aRow = aTempTable->nullRow;
        
        // PROJ-2362 memory temp 저장 효율성 개선
        for ( sNode = aTempTable->recordNode;
              sNode != NULL;
              sNode = sNode->next )
        {
            sNode->func.makeNull( sNode,
                                  aTempTable->nullRow );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getNullRow( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcSortTemp::setHitFlag( qmcdSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    현재 읽어간 Record에 Hit Flag을 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qmcMemSortElement * sElement;

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //    1. 현재 읽고 있는 Record를 찾는다.
        //    2. 해당 Record에 Hit Flag을 setting한다.
        //-----------------------------------------

        // 현재 읽고 있는 Record 검색
        sElement = (qmcMemSortElement*) aTempTable->recordNode->dstTuple->row;

        // 해당 Record에 Hit Flag Setting
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_TRUE;
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::setHitFlag( aTempTable->diskTemp )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmcSortTemp::isHitFlagged( qmcdSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *    현재 읽어간 Record에 Hit Flag가 있는지 판단한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcMemSortElement * sElement;
    idBool              sIsHitFlagged = ID_FALSE;

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //----------------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //    1. 현재 읽고 있는 Record를 찾는다.
        //    2. 해당 Record에 Hit Flag가 있는지 판단한다.
        //----------------------------------------------

        // 현재 읽고 있는 Record 검색
        sElement = (qmcMemSortElement*) aTempTable->recordNode->dstTuple->row;

        // 해당 Record에 Hit Flag가 있는지 판단
        if ( ( sElement->flag & QMC_ROW_HIT_MASK ) == QMC_ROW_HIT_TRUE )
        {
            sIsHitFlagged = ID_TRUE;
        }
        else
        {
            sIsHitFlagged = ID_FALSE;
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        sIsHitFlagged = qmcDiskSort::isHitFlagged( aTempTable->diskTemp );
    }

    return sIsHitFlagged;
}

IDE_RC
qmcSortTemp::storeCursor( qmcdSortTemp     * aTempTable,
                          smiCursorPosInfo * aCursorInfo )
{
/***********************************************************************
 *
 * Description :
 *    Merge Join을 위한 기능으로 현재 Cursor를 저장한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::storeCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::storeCursor"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Cursor Index 저장
        IDE_TEST( qmcMemSort::getCurrPosition(
                      aTempTable->memoryTemp,
                      & aCursorInfo->mCursor.mTRPos.mIndexPos )
                  != IDE_SUCCESS );

        // Row Pointer 저장
        aCursorInfo->mCursor.mTRPos.mRowPtr =
            aTempTable->recordNode->dstTuple->row;
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getCurrPosition( aTempTable->diskTemp,
                                                aCursorInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcSortTemp::restoreCursor( qmcdSortTemp     * aTempTable,
                            smiCursorPosInfo * aCursorInfo )
{
/***********************************************************************
 *
 * Description :
 *    Merge Join을 위한 기능으로 지정된 Cursor로 복원한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::restoreCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::restoreCursor"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Cursor Index 복원
        IDE_TEST( qmcMemSort::setCurrPosition(
                      aTempTable->memoryTemp,
                      aCursorInfo->mCursor.mTRPos.mIndexPos )
                  != IDE_SUCCESS );

        // Row Pointer 복원
        aTempTable->recordNode->dstTuple->row =
            aCursorInfo->mCursor.mTRPos.mRowPtr;
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::setCurrPosition( aTempTable->diskTemp,
                                                aCursorInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcSortTemp::getCursorInfo( qmcdSortTemp     * aTempTable,
                            void            ** aTableHandle,
                            void            ** aIndexHandle,
                            qmcdMemSortTemp ** aMemSortTemp,
                            qmdMtrNode      ** aMemSortRecord )
{
/***********************************************************************
 *
 * Description :
 *    View SCAN등에서 접근하기 위한 정보 추출
 *
 * Implementation :
 *    Memory Temp Table일 경우, Memory Temp Table 객체를 리턴
 *    Disk Temp Table일 경우, Table Handle과 Index Handle을 리턴함.
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::getCursorInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::getCursorInfo"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Memory Temp Table 객체를 준다.
        *aTableHandle = NULL;
        *aIndexHandle = NULL;
        *aMemSortTemp = aTempTable->memoryTemp;

        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            *aMemSortRecord = aTempTable->recordNode;
        }
        else
        {
            *aMemSortRecord = NULL;
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk Temp Table의 Handle을 준다.
        IDE_TEST( qmcDiskSort::getCursorInfo( aTempTable->diskTemp,
                                              aTableHandle,
                                              aIndexHandle )
                  != IDE_SUCCESS );

        *aMemSortTemp   = NULL;
        *aMemSortRecord = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::getDisplayInfo( qmcdSortTemp * aTempTable,
                             ULong        * aDiskPageCount,
                             SLong        * aRecordCount )
{
/***********************************************************************
 *
 * Description :
 *    Plan Display를 위한 정보를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::getDisplayInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::getDisplayInfo"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        *aDiskPageCount = 0;
        *aRecordCount = aTempTable->recordCnt;
    }
    else
    {
        //-----------------------------------------
        // Disk Sort Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskSort::getDisplayInfo( aTempTable->diskTemp,
                                               aDiskPageCount,
                                               aRecordCount )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::setSortNode( qmcdSortTemp     * aTempTable,
                          const qmdMtrNode * aSortNode )
/***********************************************************************
 *
 * Description :
 *    초기화되고, 데이터가 입력된 Temp Table의 정렬키를 변경한다.
 *    (Window Sort에서 사용)
 *
 * Implementation :
 *    현재 Disk Sort Temp의 경우 중간에 정렬키 변경이 불가하므로
 *    Memory Sort의 경우만 설정함
 *
 ***********************************************************************/
{
#define IDE_FN "qmcSortTemp::setSortNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::setSortNode"));


    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_MEMORY )
    {
        IDE_TEST( qmcMemSort::setSortNode( aTempTable->memoryTemp,
                                           aSortNode )
                  != IDE_SUCCESS );
    }
    else
    {

        /********************************************************************
         * PROJ-2201 Innovation in sorting and hashing(temp)
         ********************************************************************/
        IDE_ERROR ((aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
                    == QMCD_SORT_TMP_STORAGE_DISK );

        IDE_TEST( qmcDiskSort::setSortNode( aTempTable->diskTemp,
                                            aSortNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::setUpdateColumnList( qmcdSortTemp     * aTempTable,
                                  const qmdMtrNode * aUpdateColumns )
/***********************************************************************
 *
 * Description :
 *    update를 수행할 column list를 설정
 *    주의: Disk Sort Temp Table의 경우, hitFlag와 함께 사용 불가
 *
 * Implementation :
 *
 ***********************************************************************/
{
#define IDE_FN "qmcSortTemp::setUpdateColumnList"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::setUpdateColumnList"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_DISK )
    {
        IDE_TEST( qmcDiskSort::setUpdateColumnList( aTempTable->diskTemp,
                                                    aUpdateColumns )
                  != IDE_SUCCESS );
        
    }
    else
    {
        // Memory Sort Temp Table의 경우, 의미가 없음
        // do nothing
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    

    
#undef IDE_FN
}

IDE_RC
qmcSortTemp::updateRow( qmcdSortTemp * aTempTable )
/***********************************************************************
 *
 * Description :
 *    현재 검색 중인 위치의 row를 update
 *    (Disk의 경우만 의미가 있음)
 *
 * Implementation :
 *
 ***********************************************************************/
{
#define IDE_FN "qmcSortTemp::updateRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::updateRow"));

    if ( (aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE)
         == QMCD_SORT_TMP_STORAGE_DISK )
    {
        IDE_TEST( qmcDiskSort::updateRow( aTempTable->diskTemp )
                  != IDE_SUCCESS );
        
    }
    else
    {
        // Memory Sort Temp Table의 경우 의미가 없음
        // do nothing
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    

#undef IDE_FN
}

IDE_RC
qmcSortTemp::makeMemNullRow( qmcdSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Memory Sort Temp Table을 위한 Null Row를 생성한다.
 *
 * Implementation :
 *    값을 저장하는 Column에 대해서만 Null Value를 생성하고,
 *    Pointer/RID등을 저장하는 Column에 대해서는 Null Value를 생성하지
 *    않는다.
 *
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::makeMemNullRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::makeMemNullRow"));

    qmdMtrNode * sNode;
    mtcColumn  * sColumn;
    UInt         sRowSize;
    UInt         sActualSize;
    idBool       sIsTempType;
    UInt         i;

    // PROJ-2362 memory temp 저장 효율성 개선
    if ( aTempTable->existTempType == ID_TRUE )
    {
        sRowSize = aTempTable->mtrRowSize;
        
        for ( i = 0, sColumn = aTempTable->recordNode->dstTuple->columns;
              i < aTempTable->recordNode->dstTuple->columnCount;
              i++, sColumn++ )
        {
            if ( SMI_COLUMN_TYPE_IS_TEMP( sColumn->column.flag ) == ID_TRUE )
            {
                sActualSize = sColumn->module->actualSize( sColumn,
                                                           sColumn->module->staticNull );
            
                sRowSize = idlOS::align( sRowSize, sColumn->module->align );
                sRowSize += sActualSize;
            }
            else
            {
                // Nothing to do.
            }
        }

        aTempTable->nullRowSize = idlOS::align8( sRowSize );

        // Null Row를 위한 공간 할당
        IDU_FIT_POINT( "qmcSortTemp::makeMemNullRow::cralloc::nullRow",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->memory->cralloc( aTempTable->nullRowSize,
                                               (void**) & aTempTable->nullRow )
                  != IDE_SUCCESS);

        sRowSize = aTempTable->mtrRowSize;
        
        for ( i = 0, sColumn = aTempTable->recordNode->dstTuple->columns;
              i < aTempTable->recordNode->dstTuple->columnCount;
              i++, sColumn++ )
        {
            // offset 저장
            if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                 == SMI_COLUMN_TYPE_TEMP_1B )
            {
                sIsTempType = ID_TRUE;
                
                sRowSize = idlOS::align( sRowSize, sColumn->module->align );
                IDE_DASSERT( sRowSize < 256 );
                
                *(UChar*)((UChar*)aTempTable->nullRow + sColumn->column.offset) = (UChar)sRowSize;
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_2B )
            {
                sIsTempType = ID_TRUE;
                
                sRowSize = idlOS::align( sRowSize, sColumn->module->align );
                IDE_DASSERT( sRowSize < 65536 );

                *(UShort*)((UChar*)aTempTable->nullRow + sColumn->column.offset) = (UShort)sRowSize;
            }
            else if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                      == SMI_COLUMN_TYPE_TEMP_4B )
            {
                sIsTempType = ID_TRUE;
                
                sRowSize = idlOS::align( sRowSize, sColumn->module->align );
            
                *(UInt*)((UChar*)aTempTable->nullRow + sColumn->column.offset) = sRowSize;
            }
            else
            {
                sIsTempType = ID_FALSE;
            }

            if ( sIsTempType == ID_TRUE )
            {
                sActualSize = sColumn->module->actualSize( sColumn,
                                                           sColumn->module->staticNull );

                idlOS::memcpy( (SChar*)aTempTable->nullRow + sRowSize,
                               (SChar*)sColumn->module->staticNull,
                               sActualSize );
            
                sRowSize += sActualSize;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Null Row를 위한 공간 할당
        IDU_LIMITPOINT("qmcSortTemp::makeMemNullRow::malloc");
        IDE_TEST( aTempTable->memory->cralloc( aTempTable->nullRowSize,
                                               (void**) & aTempTable->nullRow )
                  != IDE_SUCCESS);
    }

    for ( sNode = aTempTable->recordNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        //-----------------------------------------------
        // 실제 값을 저장하는 Column에 대해서만
        // NULL Value를 생성한다.
        //-----------------------------------------------

        sNode->func.makeNull( sNode,
                              aTempTable->nullRow );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcSortTemp::makeMemKeyRange( qmcdSortTemp * aTempTable,
                              qtcNode      * aRangePredicate )
{
/***********************************************************************
 *
 * Description :
 *    Memory Temp Table을 위한 Key Range를 생성한다.
 *
 * Implementation :
 *    다음과 같은 절차로 Key Range를 생성한다.
 *    - Key Range의 크기 계산
 *    - Key Range를 위한 공간 확보
 *    - Key Range 생성
 ***********************************************************************/

#define IDE_FN "qmcSortTemp::makeMemKeyRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcSortTemp::makeMemKeyRange"));

    qtcNode * sFilter;
    UInt sKeyColsFlag;
    UInt sCompareType;

    // Key Range의 크기 계산 및 공간 확보
    if ( aTempTable->rangeArea == NULL )
    {
        // Sort Temp Table의 Key Range는 한 Column에 대해서만 가능함

        // Key Range의 Size계산
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTempTable->mTemplate,
                                           aRangePredicate,
                                           &aTempTable->rangeAreaSize )
            != IDE_SUCCESS);

        // Key Range를 위한 공간 할당
        IDU_FIT_POINT( "qmcSortTemp::makeMemKeyRange::alloc::rangeArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->memory->cralloc( aTempTable->rangeAreaSize,
                                               (void**)& aTempTable->rangeArea )
                  != IDE_SUCCESS);
    }
    else
    {
        // 이미 할당되어 있음
        // Nothing To Do
    }

    // Key Range 생성

    if ( (aTempTable->sortNode->myNode->flag & QMC_MTR_SORT_ORDER_MASK)
         == QMC_MTR_SORT_ASCENDING )
    {
        sKeyColsFlag = SMI_COLUMN_ORDER_ASCENDING;
    }
    else
    {
        sKeyColsFlag = SMI_COLUMN_ORDER_DESCENDING;
    }

    // Memory Temp Table의 key range는 MT 타입간의 compare
    sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;

    // To Fix PR-8109
    // Key Range 생성을 위해서는 비교 대상이 되는
    // Column을 입력 인자로 사용하여야 한다.
    // Key Range 생성
    IDE_TEST(
        qmoKeyRange::makeKeyRange( aTempTable->mTemplate,
                                   aRangePredicate,
                                   1,
                                   aTempTable->sortNode->func.compareColumn,
                                   & sKeyColsFlag,
                                   sCompareType,
                                   aTempTable->rangeArea,
                                   aTempTable->rangeAreaSize,
                                   & (aTempTable->range),
                                   & sFilter ) != IDE_SUCCESS );

    // 적합성 검사
    // 반드시 Range 생성이 성공해야 함
    IDE_DASSERT( sFilter == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
