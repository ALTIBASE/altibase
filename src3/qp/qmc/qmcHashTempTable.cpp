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
 * $Id: qmcHashTempTable.cpp 85333 2019-04-26 02:34:41Z et16 $
 *
 * Description :
 *     Hash Temp Table을 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 *
 **********************************************************************/
#include <idl.h> // for win32 porting
#include <qmcHashTempTable.h>
#include <qmxResultCache.h>

IDE_RC
qmcHashTemp::init( qmcdHashTemp * aTempTable,
                   qcTemplate   * aTemplate,
                   UInt           aMemoryIdx,
                   qmdMtrNode   * aRecordNode,
                   qmdMtrNode   * aHashNode,
                   qmdMtrNode   * aAggrNode,
                   UInt           aBucketCnt,
                   UInt           aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Hash Temp Table을 초기화한다.
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

    idBool       sIsDistinct;
    idBool       sIsPrimary;
    mtcColumn  * sColumn;
    UInt         i;

    // 적합성 검사
    IDE_DASSERT( aTempTable != NULL );
    IDE_DASSERT( aRecordNode != NULL );

    aTempTable->flag = aFlag;
    aTempTable->mTemplate = aTemplate;
    aTempTable->recordNode = aRecordNode;
    aTempTable->hashNode = aHashNode;
    aTempTable->aggrNode = aAggrNode;

    aTempTable->bucketCnt = aBucketCnt;

    IDE_DASSERT( aTempTable->recordNode->dstTuple->rowOffset > 0 );
    aTempTable->mtrRowSize = aTempTable->recordNode->dstTuple->rowOffset;
    aTempTable->nullRowSize = aTempTable->mtrRowSize;

    aTempTable->memoryMgr = NULL;
    //  BUG-10511
    aTempTable->nullRow = NULL;
    aTempTable->hashKey = 0;

    aTempTable->memoryTemp = NULL;
    aTempTable->memoryPartTemp = NULL;
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
    // Distinct 여부 추출
    if ( (aTempTable->flag & QMCD_HASH_TMP_DISTINCT_MASK)
         == QMCD_HASH_TMP_DISTINCT_TRUE )
    {
        sIsDistinct = ID_TRUE;
    }
    else
    {
        sIsDistinct = ID_FALSE;
    }

    // Primary 여부 추출
    if ( (aTempTable->flag & QMCD_HASH_TMP_PRIMARY_MASK)
         == QMCD_HASH_TMP_PRIMARY_TRUE )
    {
        sIsPrimary = ID_TRUE;
    }
    else
    {
        sIsPrimary = ID_FALSE;
    }

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //    1. Memory Hash Temp Table 초기화
        //    2. Null Row 생성
        //    3. Record용 Memory 관리자 초기화
        //-----------------------------------------

        // Memory Temp Table을 위한 Null Row를 생성한다.

        if ( aTempTable->hashNode != NULL )
        {
            // Memory Hash Temp Table 객체의 생성 및 초기화
            if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
                 == QMCD_HASH_TMP_HASHING_PARTITIONED )
            {
                IDU_LIMITPOINT("qmcHashTemp::init::malloc1");
                IDE_TEST( aTempTable->memory->alloc( ID_SIZEOF( qmcdMemPartHashTemp ),
                                                     (void**)& aTempTable->memoryPartTemp )
                          != IDE_SUCCESS );

                IDE_TEST( qmcMemPartHash::init( aTempTable->memoryPartTemp,
                                                aTempTable->mTemplate,
                                                aTempTable->memory,
                                                aTempTable->recordNode,
                                                aTempTable->hashNode,
                                                aTempTable->bucketCnt ) != IDE_SUCCESS );
            }
            else
            {
                IDU_FIT_POINT( "qmcHashTemp::init::alloc::memoryTemp",
                                idERR_ABORT_InsufficientMemory );

                IDE_TEST( aTempTable->memory->alloc( ID_SIZEOF(qmcdMemHashTemp),
                                                     (void**)& aTempTable->memoryTemp )
                          != IDE_SUCCESS);

                IDE_TEST( qmcMemHash::init( aTempTable->memoryTemp,
                                            aTempTable->mTemplate,
                                            aTempTable->memory,
                                            aTempTable->recordNode,
                                            aTempTable->hashNode,
                                            aTempTable->bucketCnt,
                                            sIsDistinct ) != IDE_SUCCESS );
            }
        }
        else
        {
            // To Fix PR-7960
            // GROUP BY가 없는 경우 반드시 Memory Temp Table을 사용하게 되나,
            // 실제로 Memory Hash Temp Table을 필요로 하지는 않는다.
        }

        if ( sIsPrimary == ID_TRUE )
        {
            //----------------------------------
            // Primary Hash Temp Table인 경우
            //----------------------------------

            // Record 공간 할당을 위한 메모리 관리자 초기화
            IDU_FIT_POINT( "qmcHashTemp::init::alloc::primaryMemoryMgr",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( aTempTable->memory->alloc( ID_SIZEOF(qmcMemory),
                                                (void**) & aTempTable->memoryMgr )
                      != IDE_SUCCESS);
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

            // BUG-10512
            // primary hash temp table에서만 null row 생성
            IDE_TEST( makeMemNullRow( aTempTable ) != IDE_SUCCESS );
        }
        else
        {
            //----------------------------------
            // Secondary Hash Temp Table인 경우
            //----------------------------------

            // Record 공간 할당을 위한 메모리 관리자 초기화
            IDU_FIT_POINT( "qmcHashTemp::init::alloc::secondaryMemoryMgr",
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
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk Temp Table을 위한 Null Row 생성은 Disk Temp Table에서 하며,
        // Null Row의 획득은 Disk Temp Table로부터 얻어온다.

        // Disk Hash Temp Table 객체의 생성 및 초기화
        IDU_LIMITPOINT("qmcHashTemp::init::malloc3");
        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                ID_SIZEOF(qmcdDiskHashTemp),
                (void**)& aTempTable->diskTemp ) != IDE_SUCCESS);

        IDE_TEST( qmcDiskHash::init( aTempTable->diskTemp,
                                     aTempTable->mTemplate,
                                     aTempTable->recordNode,
                                     aTempTable->hashNode,
                                     aTempTable->aggrNode,
                                     aTempTable->bucketCnt,
                                     aTempTable->mtrRowSize,
                                     sIsDistinct )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::clear( qmcdHashTemp * aTempTable )
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

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //     1. Record를 위해 할당된 공간 제거
        //     2. Memory Hash Temp Table 의 clear
        //-----------------------------------------

        // Record를 위해 할당된 공간 제거
        if ( (aTempTable->flag & QMCD_HASH_TMP_PRIMARY_MASK)
             == QMCD_HASH_TMP_PRIMARY_TRUE )
        {
            // Primary 인 경우
            // To fix BUG-17591
            // 할당된 공간을 재사용 하므로
            // qmcMemory::clearForReuse를 호출한다.
            aTempTable->memoryMgr->clearForReuse();
        }
        else
        {
            // Secondary인 경우 Memory 관리자가 존재하지 않는다.
        }

        if ( aTempTable->hashNode != NULL )
        {
            // Memory Hash Temp Table의 clear
            if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
                 == QMCD_HASH_TMP_HASHING_PARTITIONED )
            {
                IDE_TEST( qmcMemPartHash::clear( aTempTable->memoryPartTemp )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( qmcMemHash::clear( aTempTable->memoryTemp )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // To Fix PR-7960
            // GROUP BY가 없는 경우 반드시 Memory Temp Table을 사용하게 되나,
            // 실제로 Memory Hash Temp Table을 필요로 하지는 않는다.

            // Nothing To Do
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
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk Hash Temp Table의 clear
        IDE_TEST( qmcDiskHash::clear( aTempTable->diskTemp )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::clearHitFlag( qmcdHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     모든 Record의 Hit Flag을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::clearHitFlag( aTempTable->memoryPartTemp )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::clearHitFlag( aTempTable->memoryTemp )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::clearHitFlag( aTempTable->diskTemp )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::alloc( qmcdHashTemp * aTempTable,
                    void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    Record를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            *aRow = aTempTable->insertRow;
        }
        else
        {
            IDU_FIT_POINT( "qmcHashTemp::alloc::cralloc::aRow",
                            idERR_ABORT_InsufficientMemory );

            /* BUG-38290 */
            IDE_TEST( aTempTable->memoryMgr->cralloc(
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
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk Temp Table을 사용하는 경우
        // 별도의 Memory 공간을 할당받지 않고 처음에 할당한
        // 메모리 영역을 반복적으로 사용한다.

        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::addRow( qmcdHashTemp * aTempTable,
                     void         * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table에 Record를 삽입한다.
 *    Non-Distinction Insert시 사용된다.  따라서, 삽입이 실패할 수 없다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;
    UInt                sHashKey;
    void              * sResultRow = NULL;
    void              * sRow       = NULL;
    idBool              sResult;

    // 적합성 검사
    IDE_DASSERT( aRow != NULL );


    // Hash Key 획득
    IDE_TEST( getHashKey( aTempTable, aRow, & sHashKey )
              != IDE_SUCCESS );

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
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
        // Memory Hash Temp Table을 사용하는 경우
        //    1. Hit Flag 및 연결관계를 초기화한다.
        //    2. Hash Key 획득
        //    3. Memory Temp Table에 삽입한다.
        //-----------------------------------------

        // Hit Flag을 초기화한다.
        sElement = (qmcMemHashElement*) sRow;

        sElement->flag  = QMC_ROW_INITIALIZE;
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_FALSE;
        sElement->next  = NULL;

        // Memory Temp Table에 삽입한다.
        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::insertAny( aTempTable->memoryPartTemp,
                                                 sHashKey,
                                                 sRow,
                                                 & sResultRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( aTempTable->memoryTemp->insert( aTempTable->memoryTemp,
                                                      sHashKey,
                                                      sRow,
                                                      & sResultRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // PROJ-1597 Temp record size 제약 제거
        // hash key를 SM에서 계산하지 않고 QP가 내려주도록 한다.
        IDE_TEST( qmcDiskHash::insert( aTempTable->diskTemp,
                                       sHashKey,
                                       & sResult )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::addDistRow( qmcdHashTemp  * aTempTable,
                         void         ** aRow,
                         idBool        * aResult )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table에 Record를 삽입한다.
 *    Distinction Insert시 사용된다.  삽입이 실패할 수 있다.
 *
 * Implementation :
 *    Data 삽입을 시도하고, 성공 실패에 대한 결과를 리턴
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;
    UInt                sHashKey;
    void              * sResultRow = NULL;
    void              * sRow       = NULL;

    // Hash Key 획득
    IDE_TEST( getHashKey( aTempTable, *aRow, & sHashKey )
              != IDE_SUCCESS );

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            IDE_TEST( makeTempTypeRow( aTempTable,
                                       *aRow,
                                       & sRow )
                      != IDE_SUCCESS );
            
            // 새로 생성한 row로 교체한다.
            *aRow = sRow;
        }
        else
        {
            sRow = *aRow;
        }
        
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //    1. Hit Flag 및 연결관계를 초기화한다.
        //    2. Hash Key 획득
        //    3. Memory Temp Table에 삽입한다.
        //-----------------------------------------

        // Hit Flag을 초기화한다.
        sElement = (qmcMemHashElement*) sRow;

        sElement->flag  = QMC_ROW_INITIALIZE;
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_FALSE;
        sElement->next  = NULL;

        // Memory Temp Table에 삽입한다.
        // PROJ-2553 Distinct Hashing에서는 
        // Partitioned Array Hashing을 사용해서는 안 된다.
        IDE_DASSERT( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
                        != QMCD_HASH_TMP_HASHING_PARTITIONED );

        IDE_TEST( aTempTable->memoryTemp->insert( aTempTable->memoryTemp,
                                                  sHashKey,
                                                  sRow,
                                                  & sResultRow )
                  != IDE_SUCCESS );

        if ( sResultRow == NULL )
        {
            // 삽입 성공
            *aResult = ID_TRUE;
        }
        else
        {
            // 삽입 실패
            *aResult = ID_FALSE;
        }
    }
    else
    {
        // PROJ-1597 Temp record size 제약 제거
        // hash key를 SM에서 계산하지 않고 QP가 내려주도록 한다.
        IDE_TEST( qmcDiskHash::insert( aTempTable->diskTemp,
                                       sHashKey,
                                       aResult )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::makeTempTypeRow( qmcdHashTemp  * aTempTable,
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
            aTempTable->memory,
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
}

IDE_RC
qmcHashTemp::updateAggr( qmcdHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column에 대한 Update를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // 해당 Plan Node에서의 Record에 대한 Aggregation 자체가
        // 이미 Temp Table에 반영되어 별도의 작업이 필요없다.
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk에 변경을 반영하여야 한다.
        IDE_TEST( qmcDiskHash::updateAggr( aTempTable->diskTemp )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::updateFiniAggr( qmcdHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column에 대한 최종 Update를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // 해당 Plan Node에서의 Record에 대한 Aggregation 자체가
        // 이미 Temp Table에 반영되어 별도의 작업이 필요없다.
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // Disk에 변경을 반영하여야 한다.
        IDE_TEST( qmcDiskHash::updateFiniAggr( aTempTable->diskTemp )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::addNewGroup( qmcdHashTemp * aTempTable,
                          void         * aRow )
{
/***********************************************************************
 *
 * Description :
 *    To Fix PR-8213
 *       - Group Aggregation을 위해서만 사용되며, 새로운 Group을
 *         Temp Table에 등록한다.
 *       - 이미 동일한 Group이 없음이 판단되었기 때문에 반드시
 *         성공하여야 한다.
 *
 * Implementation :
 *
 *    - Memory Temp Table인 경우
 *      : 동일 Group을 검색할 때, 동일 Group이 없는 경우 삽입되어 있음
 *      : 따라서 아무런 작업도 수행하지 않는다.
 *
 *    - Disk Temp Table인 경우
 *      : 새로운 Group을 삽입한다.
 *      : 이미 새로운 Group임이 판별되었으므로 반드시 성공하여야 함.
 *
 ***********************************************************************/

    idBool sResult;
    UInt   sHashKey;

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // 해당 Plan Node에서의 Record에 대한 Aggregation 자체가
        // 이미 Temp Table에 반영되어 별도의 작업이 필요없다.
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // Hash Key 획득
        IDE_TEST( getHashKey( aTempTable, aRow, & sHashKey )
                  != IDE_SUCCESS );

        // Disk에 새로운 Row를 삽입한다.
        IDE_TEST( qmcDiskHash::insert( aTempTable->diskTemp,
                                       sHashKey,
                                       & sResult )
                  != IDE_SUCCESS );

        // 반드시 삽입이 성공해야 함.
        IDE_DASSERT( sResult == ID_TRUE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::getFirstGroup( qmcdHashTemp * aTempTable,
                            void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Group 순차 검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // PROJ-2553 Grouping Hashing에서는 
        // Partitioned Array Hashing을 사용해서는 안 된다.
        IDE_DASSERT( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
                         != QMCD_HASH_TMP_HASHING_PARTITIONED );

        IDE_TEST( qmcMemHash::getFirstSequence( aTempTable->memoryTemp,
                                                aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getFirstGroup( aTempTable->diskTemp,
                                              aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::getFirstSequence( qmcdHashTemp * aTempTable,
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

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getFirstSequence( aTempTable->memoryPartTemp,
                                                        aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getFirstSequence( aTempTable->memoryTemp,
                                                    aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getFirstSequence( aTempTable->diskTemp,
                                                 aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::getFirstHit( qmcdHashTemp * aTempTable,
                          void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *     첫번째 Hit 된 Row검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getFirstHit( aTempTable->memoryPartTemp, aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getFirstHit( aTempTable->memoryTemp, aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getFirstHit( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::getFirstNonHit( qmcdHashTemp * aTempTable,
                             void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Non-Hit Row검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getFirstNonHit( aTempTable->memoryPartTemp, aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getFirstNonHit( aTempTable->memoryTemp, aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getFirstNonHit( aTempTable->diskTemp,
                                               aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::getSameRowAndNonHit( qmcdHashTemp * aTempTable,
                                  void         * aRow,
                                  void        ** aResultRow )
{
/***********************************************************************
 *
 * Description :
 *     주어진 Row와 동일한 Row이면서 Hit Flag이 Setting이 되지 않은
 *     Record를 검색함.
 *     Set Intersection에서 사용됨.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt sHashKey;

    //-----------------------------------------
    // 입력 Row의 Hash Key값 획득
    //-----------------------------------------

    IDE_TEST( getHashKey( aTempTable, aRow, & sHashKey )
              != IDE_SUCCESS );

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // PROJ-2553 Set Operation에서는 Partitioned Array Hashing을
        // 사용해서는 안 된다.
        IDE_DASSERT( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
                         != QMCD_HASH_TMP_HASHING_PARTITIONED );

        IDE_TEST( qmcMemHash::getSameRowAndNonHit( aTempTable->memoryTemp,
                                                   sHashKey,
                                                   aRow,
                                                   aResultRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getSameRowAndNonHit( aTempTable->diskTemp,
                                                    sHashKey,
                                                    aRow,
                                                    aResultRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::getNullRow( qmcdHashTemp * aTempTable,
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
    
    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
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
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getNullRow( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::getSameGroup( qmcdHashTemp  * aTempTable,
                           void         ** aRow,
                           void         ** aResultRow )
{
/***********************************************************************
 *
 * Description :
 *
 *    To Fix PR-8213
 *    주어진 Row와 동일한 Group인 Row를 검색한다.
 *    Group Aggregation에서만 사용한다.
 *
 * Implementation :
 *
 *    Memory Temp Table인 경우 검색과 동시에 삽입을 시도함.
 *       - 동일한 Group 이 있는 경우 검색됨.
 *       - 동일한 Group 이 없는 경우 삽입됨.
 *
 *    Disk Temp Table인 경우 검색만 수행함.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;
    UInt                sHashKey;
    void              * sRow;

    // Hash Key 획득
    // Disk Temp Table의 경우도 Select를 위해 Hash Key가 필요하다.
    IDE_TEST( getHashKey( aTempTable, *aRow, & sHashKey )
              != IDE_SUCCESS );

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        // PROJ-2362 memory temp 저장 효율성 개선
        if ( aTempTable->existTempType == ID_TRUE )
        {
            IDE_TEST( makeTempTypeRow( aTempTable,
                                       *aRow,
                                       & sRow )
                      != IDE_SUCCESS );
            
            // 새로 생성한 row로 교체한다.
            *aRow = sRow;
        }
        else
        {
            sRow = *aRow;
        }
        
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //    1. Hit Flag 및 연결관계를 초기화한다.
        //    2. Memory Temp Table에 삽입한다.
        //-----------------------------------------

        // Hit Flag을 초기화한다.
        sElement = (qmcMemHashElement*) sRow;

        sElement->flag  = QMC_ROW_INITIALIZE;
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_FALSE;
        sElement->next  = NULL;

        // Memory Temp Table에 삽입한다.
        // 동일 Group이 있는 경우 삽입은 실패하고 aResultRow에 값이
        // 설정됨.
        // 없는 경우, 삽입이 성공하고 aResultRow는 NULL이 됨.

        // PROJ-2553 Group Aggregation에서는 
        // Partitioned Array Hashing을 사용해서는 안 된다.
        IDE_DASSERT( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
                         != QMCD_HASH_TMP_HASHING_PARTITIONED );

        IDE_TEST( aTempTable->memoryTemp->insert( aTempTable->memoryTemp,
                                                  sHashKey,
                                                  sRow,
                                                  aResultRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getSameGroup( aTempTable->diskTemp,
                                             sHashKey,
                                             *aRow,
                                             aResultRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::getDisplayInfo( qmcdHashTemp * aTempTable,
                             ULong        * aDiskPage,
                             SLong        * aRecordCnt,
                             UInt         * aBucketCnt )
{
/***********************************************************************
 *
 * Description :
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        *aDiskPage = 0;

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getDisplayInfo( aTempTable->memoryPartTemp,
                                                      aRecordCnt,
                                                      aBucketCnt )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getDisplayInfo( aTempTable->memoryTemp,
                                                  aRecordCnt,
                                                  aBucketCnt )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getDisplayInfo( aTempTable->diskTemp,
                                               aDiskPage,
                                               aRecordCnt )
                  != IDE_SUCCESS );
        *aBucketCnt = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::makeMemNullRow( qmcdHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Memory Hash Temp Table을 위한 Null Row를 생성한다.
 *
 * Implementation :
 *    값을 저장하는 Column에 대해서만 Null Value를 생성하고,
 *    Pointer/RID등을 저장하는 Column에 대해서는 Null Value를 생성하지
 *    않는다.
 *
 ***********************************************************************/

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
        IDU_LIMITPOINT("qmcSortTemp::makeMemNullRow::malloc");
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
        IDU_FIT_POINT( "qmcHashTemp::makeMemNullRow::cralloc::nullRow",
                        idERR_ABORT_InsufficientMemory );

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
}


IDE_RC
qmcHashTemp::getHashKey( qmcdHashTemp * aTempTable,
                         void         * aRow,
                         UInt         * aHashKey )
{
/***********************************************************************
 *
 * Description :
 *    주어진 Row로부터 Hash Key를 획득한다.
 * Implementation :
 *
 ***********************************************************************/

    UInt sValue = mtc::hashInitialValue;
    qmdMtrNode * sNode;

    for ( sNode = aTempTable->hashNode; sNode != NULL; sNode = sNode->next )
    {
        sValue = sNode->func.getHash( sValue,
                                      sNode,
                                      aRow );
    }

    *aHashKey = sValue;

    return IDE_SUCCESS;
}
