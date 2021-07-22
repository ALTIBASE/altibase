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
 * $Id: qmxResultCache.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <qmv.h>
#include <qcg.h>
#include <qmoHint.h>
#include <qmxResultCache.h>

/**
 * PROJ-2462 ResultCache
 *
 * Execute 직전에 호출되는 함수로 다음과 같은 역활을 한다.
 *
 * 1. Bind 자료 구조가 존재한다면 이 Data역시 Cache하고
 * 만약 Bind값이 달라졌다면 모든 Cache된 자료를 free하고 다시
 * 구성하도록 한다.
 *
 * 2. ResultCache의 list에 있는ComponentInfo로 부터 관련있는 Table의
 * Modify Count를 구해 놓는다.
 */
void qmxResultCache::setUpResultCache( qcStatement * aStatement )
{
    qcComponentInfo * sInfo        = NULL;
    void            * sTable       = NULL;
    UShort            sTupleID     = 0;
    UShort            sBindCount   = 0;
    SLong             sModifyCount = 0;
    iduMemory       * sMemory      = NULL;
    void            * sAllocPtr    = NULL;
    qmsTableRef     * sTableRef    = NULL;
    qmsPartitionRef * sPartRef     = NULL;
    UInt              sCount       = 0;
    UInt              sBindSize    = 0;
    UInt              i            = 0;
    qcTemplate      * sTemplate    = NULL;

    sTemplate = QC_PRIVATE_TMPLATE( aStatement );
    sBindCount = aStatement->pBindParamCount;

    IDE_TEST_CONT( ( ( sTemplate->resultCache.count <= 0 ) ||
                   ( ( sTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
                     == QC_RESULT_CACHE_DATA_ALLOC_FALSE ) ), normal_exit );

    IDE_DASSERT( sTemplate->resultCache.modifyMap != NULL );
    IDE_DASSERT( sTemplate->resultCache.data != NULL );
    IDE_DASSERT( sTemplate->resultCache.memArray != NULL );

    checkResultCacheMax( sTemplate );

    IDE_TEST_CONT( ( sTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
                   == QC_RESULT_CACHE_MAX_EXCEED_TRUE, normal_exit );

    checkResultCacheCommitMode( aStatement );
    /**
     * Bind 가 있을 경우 Result Cache에 역시 Bind용 메모리를 할당하고
     * 이 메모리를 통해 Bind 값을 저장해 놓는다.
     * 이 경우에 Bind는 항상 memArray중 0번째에 할당한다
     *
     * 캐쉬된 bind value 가 없다면 전체 bind크기 만큼 메모리를 할당후
     * bind value를 copy 해 놓는다. 그 이후에 bind value를 비교해 bind 값이
     * 변경되었는지 체크한다.
     */
    if ( sBindCount > 0 )
    {
        sTupleID = sTemplate->tmplate.variableRow;
        sBindSize = sTemplate->tmplate.rows[sTupleID].rowOffset;

        if ( sTemplate->resultCache.bindValues == NULL )
        {
            IDU_FIT_POINT("qmxResultCache::setUpResultCache::alloc::iduMemory");

            IDE_TEST( qcg::allocIduMemory( &sAllocPtr )
                      != IDE_SUCCESS );

            sMemory = new (sAllocPtr) iduMemory();
            (void)sMemory->init( IDU_MEM_QRC );

            sTemplate->resultCache.memArray[0] = sMemory;

            IDU_FIT_POINT("qmxResultCache::setUpResultCache::alloc::bindValues");

            IDE_TEST( sMemory->alloc( sBindSize,
                                     (void**)&( sTemplate->resultCache.bindValues ) )
                      != IDE_SUCCESS );

            idlOS::memcpy( (SChar*)sTemplate->resultCache.bindValues,
                           sTemplate->tmplate.rows[sTupleID].row,
                           sBindSize );
            sTemplate->resultCache.isBindChanged = ID_FALSE;
        }
        else
        {
            if ( idlOS::memcmp( (SChar*)sTemplate->resultCache.bindValues,
                                sTemplate->tmplate.rows[sTupleID].row,
                                sBindSize ) != 0 )
            {
                sTemplate->resultCache.isBindChanged = ID_TRUE;
                idlOS::memcpy( (SChar*)sTemplate->resultCache.bindValues,
                               sTemplate->tmplate.rows[sTupleID].row,
                               sBindSize );
            }
            else
            {
                sTemplate->resultCache.isBindChanged = ID_FALSE;
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    /**
     * ResultCache list에는 Temp Table이 어떤 Table로 구성되어있는지에 대한
     * Component 정보가 있고 이 정보로 부터 각 Table의 Modify Count를 저장해
     * 놓는다. 이후에 Result Cache가 사용되는 Plan은 이 정보와 Cache가
     * 쌓여질 때의 값을 비교해 Cache를 버릴지 다시 활용할지 결정한다.
     */
    for ( i = 0, sInfo = sTemplate->resultCache.list;
          i < sTemplate->resultCache.count;
          i++, sInfo = sInfo->next )
    {
        for ( sCount = 0; sCount < sInfo->count; sCount++ )
        {
            sTable = NULL;
            sTupleID = sInfo->components[sCount];
            if ( sTemplate->resultCache.modifyMap[sTupleID].isChecked
                 == ID_FALSE )
            {
                sTableRef = sTemplate->tableMap[sTupleID].from->tableRef;

                if ( sTableRef->partitionCount > 0 )
                {
                    for ( sPartRef = sTableRef->partitionRef;
                          sPartRef != NULL;
                          sPartRef = sPartRef->next )
                    {
                        if ( sTupleID == sPartRef->table )
                        {
                            sTable = sPartRef->partitionHandle;
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                }
                else
                {
                    sTable = sTableRef->tableHandle;
                }

                if ( sTable != NULL )
                {
                    smiGetTableModifyCount( sTable, &sModifyCount );

                    sTemplate->resultCache.modifyMap[sTupleID].modifyCount = sModifyCount;
                }
                else
                {
                    /* Nothing to do */
                }
                sTemplate->resultCache.modifyMap[sTupleID].isChecked = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        sTemplate->planFlag[sInfo->planID] &= ~QMN_PLAN_RESULT_CACHE_EXIST_MASK;
        sTemplate->planFlag[sInfo->planID] |= QMN_PLAN_RESULT_CACHE_EXIST_TRUE;
    }

    for ( i = 0; i < sTemplate->tmplate.rowArrayCount; i++ )
    {
        sTemplate->resultCache.modifyMap[i].isChecked = ID_FALSE;
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return;

    IDE_EXCEPTION_END;

    destroyResultCache( sTemplate );

    for ( sInfo = sTemplate->resultCache.list;
          sInfo != NULL;
          sInfo = sInfo->next )
    {
        sTemplate->resultCache.dataFlag[sInfo->planID] = 0;

        sTemplate->planFlag[sInfo->planID] &= ~QMN_PLAN_RESULT_CACHE_EXIST_MASK;
        sTemplate->planFlag[sInfo->planID] |= QMN_PLAN_RESULT_CACHE_EXIST_FALSE;
    }

    sTemplate->resultCache.flag &= ~QC_RESULT_CACHE_DATA_ALLOC_MASK;
    sTemplate->resultCache.flag |= QC_RESULT_CACHE_DATA_ALLOC_FALSE;

    return;
}

/**
 * PROJ-2462 Result Cache
 *
 */
IDE_RC qmxResultCache::initResultCache( qcTemplate      * aTemplate,
                                        qcComponentInfo * aInfo,
                                        qmndResultCache * aResultData )
{
    UInt        i            = 0;
    void      * sAllocPtr    = NULL;
    SLong       sModifyCount = 0;
    UShort      sTupleID     = 0;
    iduMemory * sMemory      = NULL;

    /**
     *  ResultCache용 메모리를 새로 할당하고 이를 통해서 Temp Table을
     *  Alloc하도록 한다
     */
    if ( ( *aResultData->flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
         == QMX_RESULT_CACHE_INIT_DONE_FALSE )
    {
        IDU_FIT_POINT("qmxResultCache::initResultCache::alloc::sAllocPtr");

        IDE_TEST( qcg::allocIduMemory( &sAllocPtr )
                  != IDE_SUCCESS );

        sMemory = new (sAllocPtr) iduMemory();
        (void)sMemory->init( IDU_MEM_QRC );

        for ( i = 0; i < aTemplate->resultCache.count + 1; i++ )
        {
            if ( aTemplate->resultCache.memArray[i] == NULL )
            {
                aTemplate->resultCache.memArray[i] = sMemory;
                aResultData->memoryIdx = i;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        IDE_TEST_RAISE( i > aTemplate->resultCache.count, ERR_UNEXPECTED );

        for ( i = 0; i < aInfo->count; i++ )
        {
            sTupleID      = aInfo->components[i];
            sModifyCount += aTemplate->resultCache.modifyMap[sTupleID].modifyCount;
        }

        aResultData->tablesModify = sModifyCount;
        aResultData->memory       = sMemory;
        aResultData->missCount = 0;
        aResultData->hitCount  = 0;
        aResultData->status    = QMND_RESULT_CACHE_INIT;
    }
    else
    {
        for ( i = 0; i < aInfo->count; i++ )
        {
            sTupleID      = aInfo->components[i];
            sModifyCount += aTemplate->resultCache.modifyMap[sTupleID].modifyCount;
        }

        if ( ( aResultData->tablesModify != sModifyCount ) ||
             ( aTemplate->resultCache.isBindChanged == ID_TRUE ) )
        {
            sMemory = aResultData->memory;
            sMemory->freeAll(1);

            *aResultData->flag &= ~QMX_RESULT_CACHE_INIT_DONE_MASK;
            *aResultData->flag |= QMX_RESULT_CACHE_INIT_DONE_FALSE;
            *aResultData->flag &= ~QMX_RESULT_CACHE_STORED_MASK;
            *aResultData->flag |= QMX_RESULT_CACHE_STORED_FALSE;
            aResultData->tablesModify = sModifyCount;
            aResultData->missCount++;
            aResultData->status = QMND_RESULT_CACHE_MISS;
        }
        else
        {
            aResultData->hitCount++;
            aResultData->status = QMND_RESULT_CACHE_HIT;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmxResultCache::initResultCache",
                                  "Invalid memArrayIndex" ));
    }
    IDE_EXCEPTION_END;

    if ( sMemory != NULL )
    {
        qcg::freeIduMemory( sMemory );
    }
    else
    {
        /* Nothing to do */
    }

    aResultData->tablesModify = -1;
    aResultData->memory       = NULL;
    aResultData->missCount    = 0;
    aResultData->hitCount     = 0;

    return IDE_FAILURE;
}

void qmxResultCache::allocResultCacheData( qcTemplate    * aTemplate,
                                           iduVarMemList * aMemory )
{
    UInt i          = 0;
    UInt sTotalSize = 0;
    UInt sOffset    = 0;

    if ( ( aTemplate->resultCache.count > 0 ) &&
         ( aTemplate->tmplate.dataSize > 0 ) )
    {
        sTotalSize = idlOS::align8( aTemplate->tmplate.dataSize );
        sTotalSize += idlOS::align8( ID_SIZEOF( UInt ) ) * aTemplate->planCount;
        sTotalSize += idlOS::align8( ID_SIZEOF( qcTableModifyMap ) ) * aTemplate->tmplate.rowArrayCount;
        sTotalSize += idlOS::align8( ID_SIZEOF( iduMemory * ) ) * ( aTemplate->resultCache.count + 1 );
        sTotalSize += idlOS::align8( ID_SIZEOF( ULong ) ) * ( aTemplate->resultCache.count + 1 );

        IDU_FIT_POINT("qmxResultCache::allocResultCacheData::alloc::resutCache.data",
                      idERR_ABORT_InsufficientMemory );
        IDE_TEST_RAISE( aMemory->alloc( sTotalSize,
                                        (void**)&( aTemplate->resultCache.data ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
        sOffset = idlOS::align8( aTemplate->tmplate.dataSize );
        aTemplate->resultCache.dataFlag = ( UInt * )( aTemplate->resultCache.data + sOffset );
        sOffset += idlOS::align8( ID_SIZEOF( UInt ) ) * aTemplate->planCount;
        aTemplate->resultCache.modifyMap = ( qcTableModifyMap *)( aTemplate->resultCache.data + sOffset );
        sOffset += idlOS::align8( ID_SIZEOF( qcTableModifyMap ) ) * aTemplate->tmplate.rowArrayCount;
        aTemplate->resultCache.memArray = ( iduMemory ** )( aTemplate->resultCache.data + sOffset );
        sOffset += idlOS::align8( ID_SIZEOF( iduMemory * ) ) * ( aTemplate->resultCache.count + 1 );
        aTemplate->resultCache.memSizeArray = ( ULong * )( aTemplate->resultCache.data + sOffset );

        for ( i = 0; i < aTemplate->planCount; i++ )
        {
            aTemplate->resultCache.dataFlag[i] = 0;
        }

        for ( i = 0; i < aTemplate->tmplate.rowArrayCount; i++ )
        {
            aTemplate->resultCache.modifyMap[i].isChecked   = ID_FALSE;
            aTemplate->resultCache.modifyMap[i].modifyCount = -1;
        }

        for ( i = 0; i < aTemplate->resultCache.count + 1; i++ )
        {
            aTemplate->resultCache.memArray[i]     = NULL;
            aTemplate->resultCache.memSizeArray[i] = 0;
        }

        aTemplate->resultCache.flag &= ~QC_RESULT_CACHE_DATA_ALLOC_MASK;
        aTemplate->resultCache.flag |= QC_RESULT_CACHE_DATA_ALLOC_TRUE;
        aTemplate->resultCache.qrcMemMaximum = QCU_RESULT_CACHE_MEMORY_MAXIMUM;
        aTemplate->resultCache.qmxMemMaximum = iduProperty::getExecuteMemoryMax();
    }
    else
    {
        /* Nothing to do */
    }

    return;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    aTemplate->resultCache.flag &= ~QC_RESULT_CACHE_DATA_ALLOC_MASK;
    aTemplate->resultCache.flag |= QC_RESULT_CACHE_DATA_ALLOC_FALSE;

    return;
}

void qmxResultCache::checkResultCacheMax( qcTemplate * aTemplate )
{
    ULong             sSize   = 0;
    UInt              i       = 0;
    iduMemory       * sMemory = NULL;
    qcComponentInfo * sInfo   = NULL;
    idBool            sIsFree = ID_FALSE;

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_TEST_CONT( aTemplate == NULL, NORMAL_EXIT );

    if ( ( aTemplate->resultCache.count > 0 ) &&
         ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
           == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
    {
        if ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
             == QC_RESULT_CACHE_MAX_EXCEED_FALSE )
        {
            for ( i = 0 ; i < aTemplate->resultCache.count + 1; i++ )
            {
                sMemory = aTemplate->resultCache.memArray[i];
                if ( sMemory != NULL )
                {
                    sSize += sMemory->getSize();
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( sSize > QCU_RESULT_CACHE_MEMORY_MAXIMUM )
            {
                aTemplate->resultCache.qrcMemMaximum = QCU_RESULT_CACHE_MEMORY_MAXIMUM;
                sIsFree = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }

            if ( sSize > iduProperty::getExecuteMemoryMax() )
            {
                aTemplate->resultCache.qmxMemMaximum = iduProperty::getExecuteMemoryMax();
                sIsFree = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( ( aTemplate->resultCache.qrcMemMaximum != QCU_RESULT_CACHE_MEMORY_MAXIMUM ) ||
                 ( aTemplate->resultCache.qmxMemMaximum != iduProperty::getExecuteMemoryMax() ) )
            {
                aTemplate->resultCache.flag &= ~QC_RESULT_CACHE_MAX_EXCEED_MASK;
                aTemplate->resultCache.flag |= QC_RESULT_CACHE_MAX_EXCEED_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sIsFree == ID_TRUE )
        {
            destroyResultCache( aTemplate );
            aTemplate->resultCache.flag &= ~QC_RESULT_CACHE_MAX_EXCEED_MASK;
            aTemplate->resultCache.flag |= QC_RESULT_CACHE_MAX_EXCEED_TRUE;

            for ( sInfo = aTemplate->resultCache.list;
                  sInfo != NULL;
                  sInfo = sInfo->next )
            {
                aTemplate->resultCache.dataFlag[sInfo->planID] = 0;

                aTemplate->planFlag[sInfo->planID] &= ~QMN_PLAN_RESULT_CACHE_EXIST_MASK;
                aTemplate->planFlag[sInfo->planID] |= QMN_PLAN_RESULT_CACHE_EXIST_FALSE;
            }
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

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );
}

void qmxResultCache::checkResultCacheCommitMode( qcStatement * aStatement )
{
    idBool       sIsAutoCommit = ID_FALSE;
    smiTrans   * sTrans        = NULL;
    qcTemplate * sTemplate     = NULL;
    idBool       sIsFree       = ID_FALSE;
    qcComponentInfo * sInfo    = NULL;
    smTID        sTransID      = SM_NULL_TID;

    sTemplate = QC_PRIVATE_TMPLATE( aStatement );
    sIsAutoCommit = QCG_GET_SESSION_IS_AUTOCOMMIT( aStatement );

    if ( sIsAutoCommit == ID_TRUE )
    {
        if ( ( sTemplate->resultCache.flag & QC_RESULT_CACHE_AUTOCOMMIT_MASK )
             == QC_RESULT_CACHE_AUTOCOMMIT_FALSE )
        {
            sIsFree = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        sTemplate->resultCache.flag &= ~QC_RESULT_CACHE_AUTOCOMMIT_MASK;
        sTemplate->resultCache.flag |= QC_RESULT_CACHE_AUTOCOMMIT_TRUE;
    }
    else
    {
        qcg::getSmiTrans( aStatement, &sTrans );
        sTransID = sTrans->getTransID();
        if ( ( sTemplate->resultCache.flag & QC_RESULT_CACHE_AUTOCOMMIT_MASK )
             == QC_RESULT_CACHE_AUTOCOMMIT_FALSE )
        {
            if ( sTemplate->resultCache.transID != sTransID )
            {
                sIsFree = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            sIsFree = ID_TRUE;
        }
        sTemplate->resultCache.transID = sTransID;
        sTemplate->resultCache.flag &= ~QC_RESULT_CACHE_AUTOCOMMIT_MASK;
        sTemplate->resultCache.flag |= QC_RESULT_CACHE_AUTOCOMMIT_FALSE;
    }

    if ( sIsFree == ID_TRUE )
    {
        destroyResultCache( sTemplate );

        for ( sInfo = sTemplate->resultCache.list;
              sInfo != NULL;
              sInfo = sInfo->next )
        {
            sTemplate->resultCache.dataFlag[sInfo->planID] = 0;
            sTemplate->planFlag[sInfo->planID] &= ~QMN_PLAN_RESULT_CACHE_EXIST_MASK;
            sTemplate->planFlag[sInfo->planID] |= QMN_PLAN_RESULT_CACHE_EXIST_FALSE;
        }
    }
    else
    {
        /* Nothing to do */
    }
}

void qmxResultCache::destroyResultCache( qcTemplate * aTemplate )
{
    UInt        i       = 0;
    iduMemory * sMemory = NULL;

    if ( ( aTemplate->resultCache.count > 0 ) &&
         ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
           == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
    {
        // Bind가 사용되었다면 Cache Count가 1 늘어난다.
        for ( i = 0; i < aTemplate->resultCache.count + 1; i++ )
        {
            sMemory = aTemplate->resultCache.memArray[i];
            if ( sMemory != NULL )
            {
                sMemory->destroy();
                aTemplate->resultCache.memArray[i] = NULL;
                aTemplate->resultCache.memSizeArray[i]  = 0;
                qcg::freeIduMemory( sMemory );
            }
            else
            {
                /* Nothing to do */
            }
        }

        aTemplate->resultCache.isBindChanged = ID_FALSE;
        aTemplate->resultCache.bindValues    = NULL;
        aTemplate->resultCache.qmxMemSize    = 0;

        for ( i = 0; i < aTemplate->tmplate.rowArrayCount; i++ )
        {
            aTemplate->resultCache.modifyMap[i].isChecked = ID_FALSE;
            aTemplate->resultCache.modifyMap[i].modifyCount = -1;
        }
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * PROJ-2462 Result Cache
 *
 * Sort나 Hash Temp의 alloc 시 호출되며 execution Memory Max를 넘는지 체크한다.
 */
IDE_RC qmxResultCache::checkExecuteMemoryMax( qcTemplate * aTemplate,
                                              UInt         aMemoryIdx )
{
    iduMemory * sQrcMem    = NULL;
    iduMemory * sMemory    = NULL;
    ULong       sMemSize   = 0;
    ULong       sTotalSize = 0;
    idBool      sIsChange  = ID_FALSE;
    UInt        i;

    if ( aMemoryIdx == UINT_MAX )
    {
        sMemSize = aTemplate->stmt->qmxMem->getSize();

        if ( sMemSize != aTemplate->resultCache.qmxMemSize )
        {
            sIsChange = ID_TRUE;
            aTemplate->resultCache.qmxMemSize = sMemSize;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        sQrcMem = aTemplate->resultCache.memArray[aMemoryIdx];
        sMemSize = sQrcMem->getSize();
        if ( sMemSize != aTemplate->resultCache.memSizeArray[aMemoryIdx] )
        {
            sIsChange = ID_TRUE;
            aTemplate->resultCache.memSizeArray[aMemoryIdx] = sMemSize;
        }
        else
        {
            /* Nothing to  do */
        }
    }

    if ( sIsChange == ID_TRUE )
    {
        for ( i = 0; i <= aTemplate->resultCache.count; i++ )
        {
            sMemory = aTemplate->resultCache.memArray[i];
            if ( sMemory != NULL )
            {
                sTotalSize += sMemory->getSize();
            }
            else
            {
                /* Nothing to do */
            }
        }

        sTotalSize += aTemplate->stmt->qmxMem->getSize();

        IDE_TEST_RAISE( sTotalSize > iduProperty::getExecuteMemoryMax(),
                        ERR_EXEC_MEM_ALLOC );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXEC_MEM_ALLOC );
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_MAX_MEM_SIZE_EXCEED,
                                 iduMemMgr::mClientInfo[IDU_MEM_QMX].mName,
                                 sTotalSize,
                                 iduProperty::getExecuteMemoryMax() ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

