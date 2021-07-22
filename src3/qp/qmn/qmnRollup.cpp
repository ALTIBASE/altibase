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
 * $Id: qmnRollup.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * PROJ-1353
 *
 * Rollup은 Sort된 자료를 한번 읽어들여 ( n + 1 ) 개의 그룹의 계산을 수행한다.
 *
 * Rollup Plan은 하위에 Sort Plan이 오거나 Sort된 자료를 읽어야 한다.
 *
 * select i1, i2 from t1 group by rollup(i1), i2 와 같이 되어 있을지라도
 * Sort 되어야하는 컬럼 순서는 i2, i1 순으로 rollup에 해당하는 컬럼이 가장 마지막에
 * 와야한다.
 *
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <qmnRollup.h>
#include <qcuProperty.h>
#include <qmxResultCache.h>

/**
 * init
 *
 *  ROLL Plan 초기화 수행
 */
IDE_RC qmnROLL::init( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    qmncROLL * sCodePlan = (qmncROLL *)aPlan;
    qmndROLL * sDataPlan = (qmndROLL *)(aTemplate->tmplate.data + aPlan->offset);
    idBool     sDep = ID_FALSE;

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnROLL::doItDefault;

    if ( ( *sDataPlan->flag & QMND_ROLL_INIT_DONE_MASK )
         == QMND_ROLL_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit( aTemplate, sCodePlan, sDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        sDataPlan->plan.myTuple->row = sDataPlan->myRow;
    }

    /* doIt중에 init이 호출되는 경우 재수행을 위해 초기화한다. */
    if ( sDataPlan->groupIndex != -1 )
    {
        sDataPlan->isDataNone = ID_FALSE;
        sDataPlan->needCopy   = ID_FALSE;
        sDataPlan->groupIndex = -1;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( checkDependency( aTemplate, sCodePlan, sDataPlan, &sDep )
              != IDE_SUCCESS );

    /* Dependency 가 다르다면 다시 쌓아야한다. */
    if ( sDep == ID_TRUE )
    {
        if ( sCodePlan->preservedOrderMode == ID_FALSE )
        {
            IDE_TEST( qmcSortTemp::clear( & sDataPlan->sortMTR ) != IDE_SUCCESS );

            IDE_TEST( storeChild( aTemplate, sCodePlan, sDataPlan )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( sCodePlan->valueTempNode != NULL )
            {
                IDE_TEST( aPlan->left->init( aTemplate, aPlan->left ) != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* memory valeu node가 있을 경우 rollup의 수행결과를 memory temp에 쌓는다 */
        if ( sCodePlan->valueTempNode != NULL )
        {
            IDE_TEST( qmcSortTemp::clear( sDataPlan->valueTempMTR ) != IDE_SUCCESS );

            IDE_TEST( valueTempStore( aTemplate, aPlan ) != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->depValue = sDataPlan->depTuple->modify;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sCodePlan->valueTempNode != NULL )
    {
        sDataPlan->doIt = qmnROLL::doItFirstValueTempMTR;
    }
    else
    {
        if ( sCodePlan->preservedOrderMode == ID_FALSE )
        {
            sDataPlan->doIt = qmnROLL::doItFirstSortMTR;
        }
        else
        {
            IDE_TEST( aPlan->left->init( aTemplate, aPlan->left ) != IDE_SUCCESS );
            sDataPlan->doIt = qmnROLL::doItFirst;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmnROLL::checkDependency( qcTemplate * aTemplate,
                                 qmncROLL   * aCodePlan,
                                 qmndROLL   * aDataPlan,
                                 idBool     * aDependent )
{
    idBool sDep = ID_FALSE;

    if ( aDataPlan->depValue != aDataPlan->depTuple->modify )
    {
        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
        {
            sDep = ID_TRUE;
        }
        else
        {
            aDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
            if ( ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_STORED_MASK )
                   == QMX_RESULT_CACHE_STORED_TRUE ) &&
                ( aDataPlan->depValue == QMN_PLAN_DEFAULT_DEPENDENCY_VALUE ) )
            {
                sDep = ID_FALSE;
            }
            else
            {
                sDep = ID_TRUE;
            }
        }
    }
    else
    {
        sDep = ID_FALSE;
    }

    *aDependent = sDep;

    return IDE_SUCCESS;
}

/**
 * linkAggrNode
 *
 *  CodePlan의 AggrNode를 DataPlan의 aggrNode로 연결한다.
 */
IDE_RC qmnROLL::linkAggrNode( qmncROLL * aCodePlan, qmndROLL * aDataPlan )
{
    UInt          i;
    qmcMtrNode  * sNode;
    qmdAggrNode * sAggrNode;
    qmdAggrNode * sPrevNode;

    sNode = aCodePlan->aggrNode;
    sAggrNode = aDataPlan->aggrNode;

    for ( i = 0, sPrevNode = NULL;
          i < aCodePlan->aggrNodeCount;
          i++, sNode = sNode->next, sAggrNode++ )
    {
        sAggrNode->myNode = sNode;
        sAggrNode->srcNode = NULL;
        sAggrNode->next = NULL;

        if ( sPrevNode != NULL )
        {
            sPrevNode->next = sAggrNode;
        }

        sPrevNode = sAggrNode;
    }

    return IDE_SUCCESS;
}

/**
 * initDistNode
 *
 *  Distinct 컬럼이 d 개일 경우  Rollup Group 수( n + 1 ) * d 만큼의 DistNode가 생성된다.
 *  이는 2중 배열을 통해서 관리되며 각각 Distinct를 위한 HashTemp를 생성한다.
 */
IDE_RC qmnROLL::initDistNode( qcTemplate * aTemplate,
                              qmncROLL   * aCodePlan,
                              qmndROLL   * aDataPlan )
{
    qmcMtrNode  * sCNode;
    qmdDistNode * sDistNode;
    UInt          sFlag;
    UInt          sHeaderSize;
    UInt          i;
    UInt          sGroup;

    aDataPlan->distNode = ( qmdDistNode ** )( aTemplate->tmplate.data +
                                              aCodePlan->distNodePtrOffset);

    for ( i = 0; i < aCodePlan->groupCount; i++ )
    {
        aDataPlan->distNode[i] = (qmdDistNode *)(aTemplate->tmplate.data +
                                 aCodePlan->distNodeOffset +
                                 idlOS::align8( ID_SIZEOF(qmdDistNode) *
                                 aCodePlan->distNodeCount * i ) );

        for ( sCNode = aCodePlan->distNode, sDistNode = aDataPlan->distNode[i];
              sCNode != NULL;
              sCNode = sCNode->next, sDistNode++ )
        {
            sDistNode->myNode  = sCNode;
            sDistNode->srcNode = NULL;
            sDistNode->next    = NULL;
        }
    }
    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sFlag =
            QMCD_HASH_TMP_STORAGE_MEMORY |
            QMCD_HASH_TMP_DISTINCT_TRUE | QMCD_HASH_TMP_PRIMARY_TRUE;

        sHeaderSize = QMC_MEMHASH_TEMPHEADER_SIZE;
    }
    else
    {
        sFlag =
            QMCD_HASH_TMP_STORAGE_DISK |
            QMCD_HASH_TMP_DISTINCT_TRUE | QMCD_HASH_TMP_PRIMARY_TRUE;

        sHeaderSize = QMC_DISKHASH_TEMPHEADER_SIZE;
    }

    // PROJ-2553
    // DISTINCT Hashing은 Bucket List Hashing 방법을 써야 한다.
    sFlag &= ~QMCD_HASH_TMP_HASHING_TYPE;
    sFlag |= QMCD_HASH_TMP_HASHING_BUCKET;

    /* Rollup Group count 만큼 distinct 노드를 생성한다. */
    for ( sGroup = 0; sGroup < (UInt)aCodePlan->groupCount; sGroup++ )
    {
        for ( i = 0, sDistNode = aDataPlan->distNode[sGroup];
              i < aCodePlan->distNodeCount;
              i++, sDistNode++ )
        {
            IDE_TEST( qmc::initMtrNode( aTemplate,
                                        (qmdMtrNode *)sDistNode,
                                        0 )
                      != IDE_SUCCESS );

            IDE_TEST( qmc::refineOffsets( (qmdMtrNode *)sDistNode,
                                          sHeaderSize )
                      != IDE_SUCCESS );
            IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                       &aTemplate->tmplate,
                                       sDistNode->dstNode->node.table )
                      != IDE_SUCCESS );

            sDistNode->mtrRow = sDistNode->dstTuple->row;
            sDistNode->isDistinct = ID_TRUE;

            IDE_TEST( qmcHashTemp::init( &sDistNode->hashMgr,
                                         aTemplate,
                                         ID_UINT_MAX,
                                         (qmdMtrNode *)sDistNode,
                                         (qmdMtrNode *)sDistNode,
                                         NULL,
                                         sDistNode->myNode->bucketCnt,
                                         sFlag )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * clearDistNode
 *
 *   그룹 index를 인자로 받아 해당하는 Distinct노드의 초기화를 수행한다.
 */
IDE_RC qmnROLL::clearDistNode( qmncROLL * aCodePlan,
                               qmndROLL * aDataPlan,
                               UInt       aGroupIndex )
{
    UInt          i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDataPlan->distNode[aGroupIndex];
          i < aCodePlan->distNodeCount;
          i++, sDistNode ++ )
    {
        IDE_TEST( qmcHashTemp::clear( &sDistNode->hashMgr )
                  != IDE_SUCCESS );
        sDistNode->mtrRow = sDistNode->dstTuple->row;
        sDistNode->isDistinct = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * initMtrNode
 *
 *  mtrNode, myNode, aggrNode, distNode, valeTempNode의 초기화를 수행한다.
 *
 *  aggrNode는 Rollup용으로 1개만 생성되지만 distNode는 Rollup그룹 갯수 만큼 생성된다.
 *  일단 distNode의 0번째로 연결해 놓는다.
 */
IDE_RC qmnROLL::initMtrNode( qcTemplate * aTemplate,
                             qmncROLL   * aCodePlan,
                             qmndROLL   * aDataPlan )
{
    qmdAggrNode * sAggrNode = NULL;
    qmdDistNode * sDistNode = NULL;
    qmdMtrNode  * sNode     = NULL;
    qmdMtrNode  * sSNode    = NULL;
    qmcMtrNode  * sCNode    = NULL;
    qmdMtrNode  * sNext     = NULL;
    UInt          i         = 0;
    UInt          sFlag     = QMCD_SORT_TMP_INITIALIZE;
    UInt          sHeaderSize;
    iduMemory   * sMemory        = NULL;
    qmndROLL    * sCacheDataPlan = NULL;

    /* myNode */
    aDataPlan->myNode = ( qmdMtrNode * )( aTemplate->tmplate.data +
                                          aCodePlan->myNodeOffset );
    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->myNode )
              != IDE_SUCCESS );
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->myNode,
                                0 )
              != IDE_SUCCESS );
    IDE_TEST( qmc::refineOffsets( aDataPlan->myNode, 0 )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple  = aDataPlan->myNode->dstTuple;

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               &aTemplate->tmplate,
                               aDataPlan->myNode->dstNode->node.table )
              != IDE_SUCCESS );
    aDataPlan->myRowSize = qmc::getMtrRowSize( aDataPlan->myNode );

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sHeaderSize = QMC_MEMSORT_TEMPHEADER_SIZE;
        sFlag      &= ~QMCD_SORT_TMP_STORAGE_TYPE;
        sFlag      |= QMCD_SORT_TMP_STORAGE_MEMORY;
    }
    else
    {
        sHeaderSize = QMC_DISKSORT_TEMPHEADER_SIZE;
        sFlag      &= ~QMCD_SORT_TMP_STORAGE_TYPE;
        sFlag      |= QMCD_SORT_TMP_STORAGE_DISK;
    }

    /* mtrNode */
    aDataPlan->mtrNode = ( qmdMtrNode * )( aTemplate->tmplate.data +
                                           aCodePlan->mtrNodeOffset );
    IDE_TEST( qmc::linkMtrNode( aCodePlan->mtrNode,
                                aDataPlan->mtrNode )
              != IDE_SUCCESS );
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                0 )
              != IDE_SUCCESS );
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode, sHeaderSize )
              != IDE_SUCCESS );
    aDataPlan->mtrTuple = aDataPlan->mtrNode->dstTuple;
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               &aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );
    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    if ( aCodePlan->preservedOrderMode == ID_FALSE )
    {
        aDataPlan->sortNode = ( qmdMtrNode * )( aTemplate->tmplate.data +
                                                aCodePlan->sortNodeOffset );
        for ( i = 0, sCNode = aCodePlan->mtrNode, sSNode = aDataPlan->sortNode;
              i < aCodePlan->myNodeCount;
              i++, sCNode = sCNode->next, sSNode = sSNode->next )
        {
            sSNode->myNode = (qmcMtrNode*)sCNode;
            sSNode->srcNode = NULL;
            sSNode->next = sSNode + 1;
            if ( sCNode->next == NULL )
            {
                sSNode->next = NULL;
            }
            else
            {
                /* Nothing to do */
            }
        }

        for ( i = 0, sNode = aDataPlan->mtrNode, sSNode = aDataPlan->sortNode;
              i < aCodePlan->myNodeCount;
              i++, sNode = sNode->next, sSNode = sSNode->next )
        {
            sNext        = sSNode->next;
            *sSNode      = *sNode;

            if ( i + 1 == aCodePlan->myNodeCount )
            {
                sSNode->next = NULL;
                break;
            }
            else
            {
                sSNode->next = sNext;
            }
        }

        IDE_TEST( qmcSortTemp::init( &aDataPlan->sortMTR,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->sortNode,
                                     0,
                                     sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aCodePlan->distNode != NULL )
    {
        IDE_TEST( initDistNode( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        aDataPlan->distNode = NULL;
    }

    if ( aCodePlan->aggrNode != NULL )
    {
        aDataPlan->aggrNode = ( qmdAggrNode * )( aTemplate->tmplate.data +
                                                 aCodePlan->aggrNodeOffset );

        IDE_TEST( linkAggrNode( aCodePlan, aDataPlan )
                  != IDE_SUCCESS );
        IDE_TEST( qmc::initMtrNode( aTemplate,
                                    ( qmdMtrNode * )aDataPlan->aggrNode,
                                    0 )
                  != IDE_SUCCESS );
        IDE_TEST( qmc::refineOffsets( ( qmdMtrNode * )aDataPlan->aggrNode, 0 )
                  != IDE_SUCCESS );
        aDataPlan->aggrTuple = aDataPlan->aggrNode->dstTuple;

        for ( sAggrNode = aDataPlan->aggrNode;
              sAggrNode != NULL;
              sAggrNode = sAggrNode->next )
        {
            if ( sAggrNode->myNode->myDist != NULL )
            {
                for ( i = 0, sDistNode = aDataPlan->distNode[0];
                      i < aCodePlan->distNodeCount;
                      i++, sDistNode++ )
                {
                    if ( sDistNode->myNode == sAggrNode->myNode->myDist )
                    {
                        sAggrNode->myDist = sDistNode;
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
                sAggrNode->myDist = NULL;
            }
        }
        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   &aTemplate->tmplate,
                                   aDataPlan->aggrNode->dstNode->node.table )
                  != IDE_SUCCESS );
        aDataPlan->aggrRowSize =
                       qmc::getMtrRowSize( (qmdMtrNode *)aDataPlan->aggrNode );
    }
    else
    {
        aDataPlan->aggrNode = NULL;
    }

    sMemory = aTemplate->stmt->qmxMem;

    /* valueTempNode가 있다면 MTR Node를 생성하고 STORE용 TempTable을 initialize한다 */
    if ( aCodePlan->valueTempNode != NULL )
    {
        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
        {
            aDataPlan->valueTempNode = ( qmdMtrNode * )( aTemplate->tmplate.data +
                                                         aCodePlan->valueTempOffset );
        }
        else
        {
            /* PROJ-2462 Result Cache */
            sCacheDataPlan = (qmndROLL *) (aTemplate->resultCache.data +
                                          aCodePlan->plan.offset );
            sMemory = sCacheDataPlan->resultData.memory;

            aDataPlan->valueTempNode = ( qmdMtrNode * )( aTemplate->resultCache.data +
                                                         aCodePlan->valueTempOffset );
        }

        IDE_TEST( qmc::linkMtrNode( aCodePlan->valueTempNode,
                                    aDataPlan->valueTempNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmc::initMtrNode( aTemplate, aDataPlan->valueTempNode, 0 )
                  != IDE_SUCCESS );
        IDE_TEST( qmc::refineOffsets( aDataPlan->valueTempNode, sHeaderSize )
                  != IDE_SUCCESS );
        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   &aTemplate->tmplate,
                                   aDataPlan->valueTempNode->dstNode->node.table )
                  != IDE_SUCCESS );

        aDataPlan->valueTempTuple = aDataPlan->valueTempNode->dstTuple;
        aDataPlan->valueTempRowSize = qmc::getMtrRowSize( aDataPlan->valueTempNode );

        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
        {
            IDU_FIT_POINT( "qmnRoll::initMtrNode::qmxAlloc:valueTempMTR",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sMemory->alloc( ID_SIZEOF( qmcdSortTemp ),
                                      (void **)&aDataPlan->valueTempMTR )
                      != IDE_SUCCESS );
            IDE_TEST( qmcSortTemp::init( aDataPlan->valueTempMTR,
                                         aTemplate,
                                         ID_UINT_MAX,
                                         aDataPlan->valueTempNode,
                                         aDataPlan->valueTempNode,
                                         0,
                                         sFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            /* PROJ-2462 Result Cache */
            sCacheDataPlan = (qmndROLL *) (aTemplate->resultCache.data +
                                          aCodePlan->plan.offset);
            if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
                 == QMX_RESULT_CACHE_INIT_DONE_FALSE )
            {
                IDU_FIT_POINT( "qmnRoll::initMtrNode::qrcAlloc:valueTempMTR",
                               idERR_ABORT_InsufficientMemory );
                IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                                   (void **)&aDataPlan->valueTempMTR )
                          != IDE_SUCCESS );

                IDE_TEST( qmcSortTemp::init( aDataPlan->valueTempMTR,
                                             aTemplate,
                                             sCacheDataPlan->resultData.memoryIdx,
                                             aDataPlan->valueTempNode,
                                             aDataPlan->valueTempNode,
                                             0,
                                             sFlag )
                          != IDE_SUCCESS );
                sCacheDataPlan->valueTempMTR = aDataPlan->valueTempMTR;
            }
            else
            {
                aDataPlan->valueTempMTR = sCacheDataPlan->valueTempMTR;
            }
        }
    }
    else
    {
        aDataPlan->valueTempRowSize = 0;
        aDataPlan->valueTempNode    = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * allocMtrRow
 *
 *   각각 필요한 Row의 크기를 할당한다.
 */
IDE_RC qmnROLL::allocMtrRow( qcTemplate * aTemplate,
                             qmncROLL   * aCodePlan,
                             qmndROLL   * aDataPlan )
{
    iduMemory  * sMemory;
    void       * sRow;
    UInt         i;

    sMemory = aTemplate->stmt->qmxMem;
    aDataPlan->compareResults = NULL;
    aDataPlan->groupStatus    = NULL;

    IDE_TEST( sMemory->alloc( aDataPlan->mtrRowSize,
                              ( void ** )&aDataPlan->mtrRow )
              != IDE_SUCCESS );
    aDataPlan->mtrTuple->row = aDataPlan->mtrRow;

    IDE_TEST( sMemory->alloc( aDataPlan->myRowSize,
                              ( void ** )&aDataPlan->myRow )
               != IDE_SUCCESS );

    IDE_TEST( sMemory->alloc( aDataPlan->myRowSize,
                              ( void** )&aDataPlan->nullRow )
              != IDE_SUCCESS );
    
    IDE_TEST( sMemory->alloc( idlOS::align8( ID_SIZEOF( UChar ) *
                                             aCodePlan->mtrNodeCount ),
                              ( void ** )&aDataPlan->compareResults )
              != IDE_SUCCESS );

    IDE_TEST( sMemory->alloc( idlOS::align8( ID_SIZEOF( UChar ) *
                                             aCodePlan->groupCount ),
                              ( void ** )&aDataPlan->groupStatus )
              != IDE_SUCCESS );
    
    IDE_TEST( qmn::makeNullRow( aDataPlan->plan.myTuple,
                                aDataPlan->nullRow )
              != IDE_SUCCESS );
    
    /**
     * aggrNode의 경우 mtrNode는 1개만 있지만 Row는 Rollup 그룹수(n+1) 만큼 존재하게 된다.
     * 이를 통해서 한번 비교를 통해서 ( n + 1 ) 그룹의 aggregation을 수행할 수 있다.
     */
    if ( aCodePlan->aggrNode != NULL )
    {
        IDE_TEST( sMemory->alloc( idlOS::align8( ID_SIZEOF( void * ) *
                                                 aCodePlan->groupCount ),
                                  ( void ** )&aDataPlan->aggrRow )
                  != IDE_SUCCESS );

        for ( i = 0; i < aCodePlan->groupCount; i ++ )
        {
            IDE_TEST( sMemory->alloc( idlOS::align8( aDataPlan->aggrRowSize ),
                                      ( void ** )&sRow )
                      != IDE_SUCCESS );

            aDataPlan->aggrRow[i] = sRow;
        }

        aDataPlan->aggrTuple->row = aDataPlan->aggrRow[0];
    }
    else
    {
        /* Nothing to do */
    }

    if ( ( aCodePlan->valueTempNode != NULL ) &&
         ( ( aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK )
           == QMN_PLAN_STORAGE_DISK ) )
    {
        sRow = NULL;
        IDE_TEST( sMemory->alloc( aDataPlan->valueTempRowSize,
                                  ( void ** )&sRow )
                  != IDE_SUCCESS );
        aDataPlan->valueTempTuple->row = sRow;
    }
    else
    {
        /* Nothing to do */
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * firstInit
 *
 *   Rollup 초기 initialize를 수행한다.
 */
IDE_RC qmnROLL::firstInit( qcTemplate * aTemplate,
                           qmncROLL   * aCodePlan,
                           qmndROLL   * aDataPlan )
{
    mtcTuple * sTuple = NULL;
    qmndROLL * sCacheDataPlan = NULL;

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndROLL *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
        aDataPlan->resultData.flag     = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
        if ( qmxResultCache::initResultCache( aTemplate,
                                              aCodePlan->componentInfo,
                                              &sCacheDataPlan->resultData )
             != IDE_SUCCESS )
        {
            *aDataPlan->flag &= ~QMN_PLAN_RESULT_CACHE_EXIST_MASK;
            *aDataPlan->flag |= QMN_PLAN_RESULT_CACHE_EXIST_FALSE;
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

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    IDE_TEST( allocMtrRow( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    aDataPlan->isDataNone   = ID_FALSE;
    aDataPlan->groupIndex   = -1;
    aDataPlan->plan.myTuple->row = aDataPlan->myRow;

    aDataPlan->groupingFuncData.info.type  = QMS_GROUPBY_ROLLUP;
    aDataPlan->groupingFuncData.info.index = &aDataPlan->groupIndex;
    aDataPlan->groupingFuncData.count      = (SInt)aCodePlan->groupCount;

    sTuple = &aTemplate->tmplate.rows[aCodePlan->groupingFuncRowID];
    aDataPlan->groupingFuncPtr  = (SLong *)sTuple->row;
    *aDataPlan->groupingFuncPtr = (SLong)( &aDataPlan->groupingFuncData );

    aDataPlan->depTuple = &aTemplate->tmplate.rows[ aCodePlan->depTupleID ];
    aDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    *aDataPlan->flag &= ~QMND_ROLL_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_ROLL_INIT_DONE_TRUE;

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        *aDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_INIT_DONE_MASK;
        *aDataPlan->resultData.flag |= QMX_RESULT_CACHE_INIT_DONE_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnROLL::doIt( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * aFlag )
{
    qmndROLL * sDataPlan = (qmndROLL *)(aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * initAggregation
 *
 *   Aggregation Node의 초기화를 수행한다.
 */
IDE_RC qmnROLL::initAggregation( qcTemplate * aTemplate,
                                 qmndROLL   * aDataPlan,
                                 UInt         aGroupIndex )
{
    qmdAggrNode * sNode;

    aDataPlan->aggrTuple->row = aDataPlan->aggrRow[aGroupIndex];
    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( qtc::initialize( sNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * setDistMtrColumns
 *
 *   그룹 Index를 인자로 받아 해당 인덱스의 distint Node의 HashTemp에 현제
 *   값을 넣어보고 겹치는 자료인지 아닌지를 설정한다.
 */
IDE_RC qmnROLL::setDistMtrColumns( qcTemplate * aTemplate,
                                   qmncROLL   * aCodePlan,
                                   qmndROLL   * aDataPlan,
                                   UInt         aGroupIndex )
{
    UInt          i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDataPlan->distNode[aGroupIndex];
          i < aCodePlan->distNodeCount;
          i++, sDistNode++ )
    {
        if ( sDistNode->isDistinct == ID_TRUE )
        {
            IDE_TEST( qmcHashTemp::alloc( &sDistNode->hashMgr,
                                          &sDistNode->mtrRow )
                      != IDE_SUCCESS );
            sDistNode->dstTuple->row = sDistNode->mtrRow;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( sDistNode->func.setMtr( aTemplate,
                                          (qmdMtrNode *)sDistNode,
                                          sDistNode->mtrRow )
                  != IDE_SUCCESS );

        IDE_TEST( qmcHashTemp::addDistRow( &sDistNode->hashMgr,
                                           &sDistNode->mtrRow,
                                           &sDistNode->isDistinct )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * execAggregation
 *
 *   해당하는 그룹의 Row의 Aggregation의 계산을 수행한다.
 *   만약 Distinct 노드가 있는경우에는 해당하는 distNode의 distinct여부를 보고
 *   aggregate 의 수행 여부를 결정한다.
 */
IDE_RC qmnROLL::execAggregation( qcTemplate * aTemplate,
                                 qmncROLL   * aCodePlan,
                                 qmndROLL   * aDataPlan,
                                 UInt         aGroupIndex )
{
    qmdAggrNode * sNode = NULL;
    UInt          i     = 0;

    aDataPlan->aggrTuple->row = aDataPlan->aggrRow[aGroupIndex];

    // BUG-42277
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        if ( sNode->myDist == NULL )
        {
            IDE_TEST( qtc::aggregate( sNode->dstNode, aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_DASSERT( i < aCodePlan->distNodeCount );
            if ( aDataPlan->distNode[aGroupIndex][i].isDistinct == ID_TRUE )
            {
                IDE_TEST( qtc::aggregate( sNode->dstNode, aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            i++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 *  finiAggregation
 *
 *   finialize를 수행한다.
 */
IDE_RC qmnROLL::finiAggregation( qcTemplate * aTemplate,
                                 qmndROLL   * aDataPlan )
{
    qmdAggrNode * sNode;

    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( qtc::finalize( sNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * setMtrRow
 *
 *   현제 Row에 setMtr을 수행한다. 이를 통해서 각 컬럼이 제대로 값을 얻는다.
 */
IDE_RC qmnROLL::setMtrRow( qcTemplate * aTemplate,
                           qmndROLL   * aDataPlan )
{
    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL ;
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setMtr( aTemplate,
                                      sNode,
                                      aDataPlan->mtrTuple->row )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * copyMtrRowToMyRow
 *
 *   읽어온 mtrRow에서 myRow로 데이터를 copy한다.
 *   myRow는 myNode에서 사용하는 Row로 1개만 존재 한다. 경우에 따라서 컬럼이 NULL이 될수 있다.
 *
 *   RowNum이나 level같은 경우 Pointer가 아닌 value로 올라온다. 따라서 value처리해야한다.
 */
IDE_RC qmnROLL::copyMtrRowToMyRow( qmndROLL * aDataPlan )
{

    qmdMtrNode * sNode;
    qmdMtrNode * sMtrNode;
    void       * sSrcValue;
    void       * sDstValue;

    for ( sNode = aDataPlan->myNode, sMtrNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next, sMtrNode = sMtrNode->next )
    {
        if ( sMtrNode->func.setTuple == &qmc::setTupleByValue )
        {
            sSrcValue = (void *) mtc::value( sMtrNode->dstColumn,
                                             aDataPlan->mtrTuple->row,
                                             MTD_OFFSET_USE );

            sDstValue = (void *) mtc::value( sNode->dstColumn,
                                             aDataPlan->myRow,
                                             MTD_OFFSET_USE );
            
            idlOS::memcpy( (SChar *)sDstValue,
                           (SChar *)sSrcValue,
                           sMtrNode->dstColumn->module->actualSize(
                               sMtrNode->dstColumn,
                               sSrcValue ) );
        }
        else
        {
            sSrcValue = (void *) mtc::value( sMtrNode->srcColumn,
                                             sMtrNode->srcTuple->row,
                                             MTD_OFFSET_USE );

            sDstValue = (void *) mtc::value( sNode->dstColumn,
                                             aDataPlan->myRow,
                                             MTD_OFFSET_USE );
            
            idlOS::memcpy( (SChar *)sDstValue,
                           (SChar *)sSrcValue,
                           sMtrNode->srcColumn->module->actualSize(
                               sMtrNode->srcColumn,
                               sSrcValue ) );
        }
    }
    aDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;
}

/**
 * doItFirst
 *
 *  doItFrist에서는 먼저 하위 Plan에서 Row를 하나 얻고서 초기화를 수행한다.
 *  그리고 이 첫 번째 Row를 myRow에 복사해 놓는다.
 */
IDE_RC qmnROLL::doItFirst( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
    qmncROLL * sCodePlan = (qmncROLL *)aPlan;
    qmndROLL * sDataPlan = (qmndROLL *)(aTemplate->tmplate.data + aPlan->offset);
    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;
    UInt       i;

    if ( sDataPlan->isDataNone == ID_FALSE )
    {
        IDE_TEST( sCodePlan->plan.left->doIt( aTemplate,
                                              sCodePlan->plan.left,
                                              &sFlag )
                  != IDE_SUCCESS );

        if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( setMtrRow( aTemplate, sDataPlan )
                      != IDE_SUCCESS );
            IDE_TEST( copyMtrRowToMyRow( sDataPlan )
                      != IDE_SUCCESS );

            sDataPlan->doIt       = qmnROLL::doItNext;
            sDataPlan->needCopy   = ID_FALSE;
            sDataPlan->groupIndex = 0;

            if ( sCodePlan->distNode != NULL )
            {
                for ( i = 0; i < sCodePlan->groupCount; i ++ )
                {
                    IDE_TEST( clearDistNode( sCodePlan, sDataPlan, i )
                              != IDE_SUCCESS );
                    IDE_TEST( setDistMtrColumns( aTemplate,
                                                 sCodePlan,
                                                 sDataPlan,
                                                 i )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }
            for ( i = 0; i < sCodePlan->groupCount; i ++ )
            {
                sDataPlan->groupStatus[i] = QMND_ROLL_GROUP_MATCHED;
                
                if ( sCodePlan->aggrNode != NULL )
                {
                    IDE_TEST( initAggregation( aTemplate, sDataPlan, i )
                              != IDE_SUCCESS );
                    IDE_TEST( execAggregation( aTemplate, sCodePlan, sDataPlan, i )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            IDE_TEST( qmnROLL::doItNext( aTemplate, aPlan, aFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            *aFlag = QMC_ROW_DATA_NONE;
        }
    }
    else
    {
        sDataPlan->isDataNone = ID_FALSE;
        sDataPlan->needCopy   = ID_FALSE;
        sDataPlan->groupIndex = -1;

        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * compareRows
 *
 *  myNode의 Row와 mtrNode의 Row를 비료해서 각 컬럼이 matching 여부를 표시한다.
 */
IDE_RC qmnROLL::compareRows( qmndROLL * aDataPlan )
{
    qmdMtrNode * sNode    = NULL;
    qmdMtrNode * sMtrNode = NULL;
    SInt         sResult  = -1;
    UInt         i        = 0;
    void       * sRow;
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;

    sRow = aDataPlan->mtrTuple->row;

    for ( sNode = aDataPlan->myNode, sMtrNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next, sMtrNode = sMtrNode->next, i++ )
    {
        sResult = -1;

        if ( sNode->srcNode->node.module == &qtc::valueModule )
        {
            sResult = 0;
        }
        else
        {
            // PROJ-2362 memory temp 저장 효율성 개선
            // mtrNode가 TEMP_TYPE인 경우 compare function은 logical compare이므로
            // myRow의 value를 offset_useless로 변경한다.
            if ( sMtrNode->func.setTuple == &qmc::setTupleByValue )
            {
                if ( SMI_COLUMN_TYPE_IS_TEMP( sMtrNode->dstColumn->column.flag ) == ID_TRUE )
                {
                    sValueInfo1.column = ( const mtcColumn *)sNode->func.compareColumn;
                    sValueInfo1.value  = (void *) mtc::value( sNode->dstColumn,
                                                              aDataPlan->myRow,
                                                              MTD_OFFSET_USE );
                    sValueInfo1.flag   = MTD_OFFSET_USELESS;
                
                    sValueInfo2.column = ( const mtcColumn *)sMtrNode->func.compareColumn;
                    sValueInfo2.value  = (void *) mtc::value( sMtrNode->dstColumn,
                                                              aDataPlan->mtrTuple->row,
                                                              MTD_OFFSET_USE );
                    sValueInfo2.flag   = MTD_OFFSET_USELESS;
                    
                    if ( ( sMtrNode->flag & QMC_MTR_SORT_ORDER_MASK )
                         == QMC_MTR_SORT_ASCENDING )
                    {
                        if ( sMtrNode->func.compare !=
                             sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_ASCENDING] )
                        {
                            sMtrNode->func.compare =
                                sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_ASCENDING];
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        if ( sMtrNode->func.compare !=
                             sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_DESCENDING] )
                        {
                            sMtrNode->func.compare =
                                sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_DESCENDING];
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    sValueInfo1.value  = sNode->func.getRow( sNode, aDataPlan->myRow );
                    sValueInfo1.column = ( const mtcColumn *)sNode->func.compareColumn;
                    sValueInfo1.flag   = MTD_OFFSET_USE;
                    
                    sValueInfo2.value  = sMtrNode->func.getRow( sMtrNode, sRow );
                    sValueInfo2.column = ( const mtcColumn *)sMtrNode->func.compareColumn;
                    sValueInfo2.flag   = MTD_OFFSET_USE;
                }
            }
            else
            {
                if ( SMI_COLUMN_TYPE_IS_TEMP( sMtrNode->srcColumn->column.flag ) == ID_TRUE )
                {
                    sValueInfo1.column = ( const mtcColumn *)sNode->func.compareColumn;
                    sValueInfo1.value  = (void *) mtc::value( sNode->dstColumn,
                                                              aDataPlan->myRow,
                                                              MTD_OFFSET_USE );
                    sValueInfo1.flag   = MTD_OFFSET_USELESS;
                
                    sValueInfo2.column = ( const mtcColumn *)sMtrNode->func.compareColumn;
                    sValueInfo2.value  = (void *) mtc::value( sMtrNode->srcColumn,
                                                              sMtrNode->srcTuple->row,
                                                              MTD_OFFSET_USE );
                    sValueInfo2.flag   = MTD_OFFSET_USELESS;
                    
                    if ( ( sMtrNode->flag & QMC_MTR_SORT_ORDER_MASK )
                         == QMC_MTR_SORT_ASCENDING )
                    {
                        if ( sMtrNode->func.compare !=
                             sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_ASCENDING] )
                        {
                            sMtrNode->func.compare =
                                sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_ASCENDING];
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        if ( sMtrNode->func.compare !=
                             sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_DESCENDING] )
                        {
                            sMtrNode->func.compare =
                                sMtrNode->func.compareColumn->module->logicalCompare[MTD_COMPARE_DESCENDING];
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    sValueInfo1.value = sNode->func.getRow( sNode, aDataPlan->myRow );
                    sValueInfo1.column = ( const mtcColumn *)sNode->func.compareColumn;
                    sValueInfo1.flag   = MTD_OFFSET_USE;
                    
                    sValueInfo2.value = sMtrNode->func.getRow( sMtrNode, sRow );
                    sValueInfo2.column = ( const mtcColumn *)sMtrNode->func.compareColumn;
                    sValueInfo2.flag   = MTD_OFFSET_USE;
                }
            }
            
            sResult = sMtrNode->func.compare( &sValueInfo1, &sValueInfo2 );
        }

        if ( sResult == 0 )
        {
            aDataPlan->compareResults[i] = QMND_ROLL_COLUMN_MACHED; /* matched */
        }
        else
        {
            aDataPlan->compareResults[i] = QMND_ROLL_COLUMN_NOT_MATCHED; /* not matched */
        }
    }

    return IDE_SUCCESS;
}

/**
 * compareGroupExecAggr
 *
 *   Rollup의 그룹별로 matched 되어야 하는 컬럼이 다르므로 각 그룹별로 matched되어야
 *   하는 컬럼만 선별하여 compareResults가 되는지를 판별하고
 *   이 그룹이 myNode에 있는 자료와 그룹화 되어있다면 execAggregation을 수행한다.
 */
IDE_RC qmnROLL::compareGroupsExecAggr( qcTemplate * aTemplate,
                                       qmncROLL   * aCodePlan,
                                       qmndROLL   * aDataPlan,
                                       idBool     * aAllMatched )
{
    idBool sIsGroup;
    idBool sAllMatch = ID_TRUE;;
    UInt   i;
    UInt   j;

    for ( i = 0; i < aCodePlan->groupCount; i ++ )
    {
        sIsGroup = ID_TRUE;

        /* 총계의 경우에는 항상 Aggregation을 수행한다. */
        if ( ( (SInt)i == aCodePlan->groupCount - 1 ) &&
             ( aCodePlan->partialRollup == -1 ) )
        {
            /* Nothing to do */
        }
        else
        {
            /* 현제 그룹의 컬럼중 해당사항이 있는것만 compareResuts를 비료한다
             * mtrGroup에는  111 과 같은 rollup 컬럼중 비교해야할 컬럼이 지정되어있다.
             *              110
             *              100
             *              000
             */
            for ( j = 0; j < aCodePlan->myNodeCount; j++ )
            {
                if ( aCodePlan->mtrGroup[i][j] == QMND_ROLL_COLUMN_CHECK_NEED )
                {
                    if ( aDataPlan->compareResults[j] == QMND_ROLL_COLUMN_MACHED )
                    {
                        /* Nothing to do */
                    }
                    else
                    {
                        sIsGroup = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        if ( sIsGroup == ID_TRUE )
        {
            /* 현제 그룹이 맞다면 Status를 업데이트하고 Aggregation을 수행한다. */
            aDataPlan->groupStatus[i] = QMND_ROLL_GROUP_MATCHED;
            if ( aCodePlan->aggrNode != NULL )
            {
                IDE_TEST( execAggregation( aTemplate, aCodePlan, aDataPlan, i )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            aDataPlan->groupStatus[i] = QMND_ROLL_GROUP_NOT_MATCHED;
            sAllMatch = ID_FALSE;
        }
    }

    *aAllMatched = sAllMatch;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * doItNext
 *
 *   doItNext를 수행한다.
 *
 *
 */
IDE_RC qmnROLL::doItNext( qcTemplate * aTemplate,
                          qmnPlan    * aPlan,
                          qmcRowFlag * aFlag )
{
    qmncROLL   * sCodePlan = (qmncROLL *)aPlan;
    qmndROLL   * sDataPlan = (qmndROLL *)(aTemplate->tmplate.data + aPlan->offset);
    qmcRowFlag   sFlag     = QMC_ROW_INITIALIZE;
    mtcColumn  * sColumn   = NULL;
    idBool       sAllMatched;
    UInt         sGroupCount;
    UShort       i;

    if ( ( sCodePlan->aggrNode   != NULL ) &&
         ( sDataPlan->groupIndex != 0 ) )
    {
        /* 이미 올려진 그룹에 대해 Aggregation을 초기화한다. */
        IDE_TEST( initAggregation( aTemplate, sDataPlan, sDataPlan->groupIndex - 1 )
                  != IDE_SUCCESS );
        if ( sCodePlan->distNode != NULL )
        {
            IDE_TEST( clearDistNode( sCodePlan, sDataPlan, sDataPlan->groupIndex - 1 )
                      != IDE_SUCCESS);

            IDE_TEST( setDistMtrColumns( aTemplate,
                                         sCodePlan,
                                         sDataPlan,
                                         sDataPlan->groupIndex -1)
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( execAggregation( aTemplate,
                                   sCodePlan,
                                   sDataPlan,
                                   sDataPlan->groupIndex - 1 )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* Rollup 그룹 중 예를 들어 rollup( i1, i2, i3 ) 중에 (i1,i2) (i1) 그룹에서
     * 그룹이 달라져서 상위 로 보내져야할 때 아래와 같이 적절한 컬럼의 NULL로 만들고 해당
     * 그룹의 Aggr Row를 지정해서 NORMAL_EXIT로 끝나게 된다.
     */
    if ( sCodePlan->partialRollup != -1 )
    {
        sGroupCount = sCodePlan->groupCount; /* pratial Rollup */
    }
    else
    {
        sGroupCount = sCodePlan->groupCount - 1; /* just Rollup */
    }

    if ( ( sDataPlan->groupIndex < (SInt)( sGroupCount ) ) &&
         ( sDataPlan->groupStatus[sDataPlan->groupIndex] == QMND_ROLL_GROUP_NOT_MATCHED ) )
    {
        for ( i = 0, sColumn = sDataPlan->plan.myTuple->columns;
              i < sCodePlan->myNodeCount;
              i++, sColumn++ )
        {
            if ( sCodePlan->mtrGroup[sDataPlan->groupIndex][i] == QMND_ROLL_COLUMN_CHECK_NONE )
            {
                if ( sColumn != NULL )
                {
                    sColumn->module->null( sColumn,
                                           (void *) mtc::value(
                                               sColumn,
                                               sDataPlan->plan.myTuple->row,
                                               MTD_OFFSET_USE ) );
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
        }
        sDataPlan->plan.myTuple->modify++;

        if ( sCodePlan->aggrNode != NULL )
        {
            sDataPlan->aggrTuple->row = sDataPlan->aggrRow[sDataPlan->groupIndex];
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->groupIndex++;

        /* BUG-38093 The number of rollup's result is not corrent */
        if ( ( sDataPlan->isDataNone == ID_TRUE ) &&
             ( sCodePlan->partialRollup != -1 ) &&
             ( (UInt)sDataPlan->groupIndex == sGroupCount ) )
        {
            sDataPlan->doIt = qmnROLL::doItFirst;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sDataPlan->needCopy == ID_TRUE )
    {
        /* 서로 다른 그룹이 발견되어 이전 그룹의 myNode가 처리되어 myNode에 새로운 값을 넣게 된다. */
        IDE_TEST( copyMtrRowToMyRow( sDataPlan )
                  != IDE_SUCCESS );
        sDataPlan->needCopy = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    /* 데이터가 없을 경우 총계에 대한 Row를 위로 올려보낸다. 역시 마지막 column을 NULL로 만들고
     * 총계에 해당하는 Aggr Row를 지정한뒤에 doIt 함수를 doItFirst로 지정해 doItFirst에서
     * data none을 올려주도록 한다.
     */
    if ( sDataPlan->isDataNone == ID_TRUE )
    {
        sColumn = sDataPlan->plan.myTuple->columns;
        if ( sCodePlan->partialRollup != -1 )
        {
            for ( i = 0, sColumn = sDataPlan->plan.myTuple->columns;
                  i < (UInt)sCodePlan->partialRollup; i++, sColumn++ )
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        for ( i = 0; i < sCodePlan->elementCount[0]; i++, sColumn++ )
        {
            if ( sColumn != NULL )
            {
                sColumn->module->null( sColumn,
                                       (void *) mtc::value(
                                           sColumn,
                                           sDataPlan->plan.myTuple->row,
                                           MTD_OFFSET_USE ) );
            }
            else
            {
                /* Nothing to do */
            }
        }
        sDataPlan->plan.myTuple->modify++;

        if ( sCodePlan->aggrNode != NULL )
        {
            sDataPlan->aggrTuple->row = sDataPlan->aggrRow[sCodePlan->groupCount - 1];
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->groupIndex++;
        sDataPlan->doIt = qmnROLL::doItFirst;
        IDE_CONT( NORMAL_EXIT )
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( sCodePlan->plan.left->doIt( aTemplate,
                                          sCodePlan->plan.left,
                                          &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        while ( 1 )
        {
            IDE_TEST( setMtrRow( aTemplate, sDataPlan )
                      != IDE_SUCCESS );

            /* distNode가 있을 경우 현제 읽은 Row의 Distinct 여부를 판별한다. */
            if ( sCodePlan->distNode != NULL )
            {
                for ( i = 0; i < sCodePlan->groupCount; i ++ )
                {
                    IDE_TEST( setDistMtrColumns( aTemplate,
                                                 sCodePlan,
                                                 sDataPlan,
                                                 i )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }

            /* myNode와 mtrNode의 Row를 서로 비교해 각 컬럼이 맞는지 비교한다 */
            IDE_TEST( compareRows( sDataPlan )
                      != IDE_SUCCESS );

            /* 각 그룹별로 그룹이 되는지 아닌지를 판별하고 그룹이라면 exeAggr을 수행한다. */
            IDE_TEST( compareGroupsExecAggr( aTemplate,
                                             sCodePlan,
                                             sDataPlan,
                                             &sAllMatched )
                      != IDE_SUCCESS );

            /* 모든 그룹이 맞다면 다음 Row를 읽어서 반복한다 */
            if ( sAllMatched == ID_TRUE )
            {
                IDE_TEST( sCodePlan->plan.left->doIt( aTemplate,
                                                      sCodePlan->plan.left,
                                                      &sFlag )
                          != IDE_SUCCESS );
                if ( sFlag == QMC_ROW_DATA_NONE )
                {
                    /* 데이터가 존재 하지 않는다면 groupStatus를 모두 not matched로 만들어
                     * 모든 그룹의 데이터를 올려보내게 한다.
                     */
                    sDataPlan->isDataNone = ID_TRUE;
                    sDataPlan->groupIndex = 1;

                    if ( sCodePlan->aggrNode != NULL )
                    {
                         sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
                    }
                    else
                    {
                         /* Nothing to do */
                    }

                    for ( i = 1; i < sCodePlan->groupCount - 1; i ++ )
                    {
                        sDataPlan->groupStatus[i] = QMND_ROLL_GROUP_NOT_MATCHED;
                    }
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* [0] 번 그룹에 대당하는 Aggregation노드를 지정후에 나간다 */
                sDataPlan->needCopy   = ID_TRUE;
                sDataPlan->groupIndex = 1;

                if ( sCodePlan->aggrNode != NULL )
                {
                    sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            }
        }
    }
    else
    {
        /* 데이터가 존재 하지 않는다면 groupStatus를 모두 not matched로 만들어
         * 모든 그룹의 데이터를 올려보내게 한다.
         */
        sDataPlan->isDataNone = ID_TRUE;
        sDataPlan->groupIndex = 1;

        if ( sCodePlan->aggrNode != NULL )
        {
             sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
        }
        else
        {
             /* Nothing to do */
        }
        for ( i = 1; i < sCodePlan->groupCount - 1; i ++ )
        {
            sDataPlan->groupStatus[i] = QMND_ROLL_GROUP_NOT_MATCHED;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    if ( sCodePlan->aggrNode != NULL )
    {
        IDE_TEST( finiAggregation( aTemplate, sDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    *aFlag = QMC_ROW_DATA_EXIST;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * padNull
 *
 *   Null pading을 수행한다.
 */
IDE_RC qmnROLL::padNull( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    qmncROLL  * sCodePlan = ( qmncROLL *)aPlan;
    qmndROLL  * sDataPlan = ( qmndROLL *)( aTemplate->tmplate.data + aPlan->offset );
    mtcColumn * sColumn;
    UInt        i;

    if ( ( aTemplate->planFlag[sCodePlan->planID] & QMND_ROLL_INIT_DONE_MASK )
         == QMND_ROLL_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    if ( sCodePlan->valueTempNode == NULL )
    {
        sDataPlan->plan.myTuple->row = sDataPlan->nullRow;
        
        // PROJ-2362 memory temp 저장 효율성 개선
        sColumn = sDataPlan->plan.myTuple->columns;
        for ( i = 0; i < sDataPlan->plan.myTuple->columnCount; i++, sColumn++ )
        {
            if ( SMI_COLUMN_TYPE_IS_TEMP( sColumn->column.flag ) == ID_TRUE )
            {
                sColumn->module->null( sColumn,
                                       sColumn->column.value );
            }
            else
            {
                // Nothing to do.
            }
        }
        
        sDataPlan->plan.myTuple->modify++;
    }
    else
    {
        IDE_TEST( qmcSortTemp::getNullRow( sDataPlan->valueTempMTR,
                                           &sDataPlan->valueTempTuple->row )
                  != IDE_SUCCESS );
    }

    sDataPlan->depValue = sDataPlan->depTuple->modify;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnROLL::doItDefault( qcTemplate *,
                             qmnPlan    *,
                             qmcRowFlag * )
{
    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC qmnROLL::setTupleMtrNode( qcTemplate * aTemplate,
                                 qmndROLL   * aDataPlan )
{
    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setTuple( aTemplate,
                                        sNode,
                                        aDataPlan->mtrTuple->row )
                  != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnROLL::setTupleValueTempNode( qcTemplate * aTemplate,
                                       qmndROLL   * aDataPlan )
{
    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->valueTempNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setTuple( aTemplate,
                                        sNode,
                                        aDataPlan->valueTempTuple->row )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnROLL::storeChild( qcTemplate * aTemplate,
                            qmncROLL   * aCodePlan,
                            qmndROLL   * aDataPlan )
{
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;

    /* child Plan initialize */
    IDE_TEST( aCodePlan->plan.left->init( aTemplate,
                                          aCodePlan->plan.left )
              != IDE_SUCCESS );

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          &sFlag )
              != IDE_SUCCESS );

    if (( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        aDataPlan->isDataNone = ID_TRUE;
    }
    else
    {
        while (( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( qmcSortTemp::alloc( &aDataPlan->sortMTR,
                                          &aDataPlan->mtrTuple->row )
                      != IDE_SUCCESS);
            IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                      != IDE_SUCCESS );

            IDE_TEST( qmcSortTemp::addRow( &aDataPlan->sortMTR,
                                           aDataPlan->mtrTuple->row )
                      != IDE_SUCCESS );

            IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                                  aCodePlan->plan.left,
                                                  &sFlag )
                      != IDE_SUCCESS );
        }

        if ( aDataPlan->sortNode != NULL )
        {
            IDE_TEST( qmcSortTemp::sort( &aDataPlan->sortMTR )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnROLL::doItFirstSortMTR( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan,
                                  qmcRowFlag * aFlag )
{
    qmncROLL * sCodePlan = (qmncROLL *)aPlan;
    qmndROLL * sDataPlan = (qmndROLL *)(aTemplate->tmplate.data + aPlan->offset);
    void     * sSearchRow;
    void     * sOrgRow;
    UInt       i;

    if ( sDataPlan->isDataNone == ID_FALSE )
    {
        sOrgRow = sSearchRow = sDataPlan->mtrTuple->row;
        IDE_TEST( qmcSortTemp::getFirstSequence( &sDataPlan->sortMTR,
                                                 &sSearchRow )
                  != IDE_SUCCESS );
        sDataPlan->mtrTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;
        sDataPlan->mtrTuple->modify++;

        if ( sSearchRow == NULL )
        {
            *aFlag = QMC_ROW_DATA_NONE;
        }
        else
        {
            IDE_TEST( setTupleMtrNode( aTemplate, sDataPlan )
                      != IDE_SUCCESS );
            IDE_TEST( copyMtrRowToMyRow( sDataPlan )
                      != IDE_SUCCESS );

            sDataPlan->doIt       = qmnROLL::doItNextSortMTR;
            sDataPlan->needCopy   = ID_FALSE;
            sDataPlan->groupIndex = 0;

            if ( sCodePlan->distNode != NULL )
            {
                for ( i = 0; i < sCodePlan->groupCount; i ++ )
                {
                    IDE_TEST( clearDistNode( sCodePlan, sDataPlan, i )
                              != IDE_SUCCESS );
                    IDE_TEST( setDistMtrColumns( aTemplate,
                                                 sCodePlan,
                                                 sDataPlan,
                                                 i )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }
            for ( i = 0; i < sCodePlan->groupCount; i ++ )
            {
                sDataPlan->groupStatus[i] = 1;
                if ( sCodePlan->aggrNode != NULL )
                {
                    IDE_TEST( initAggregation( aTemplate, sDataPlan, i )
                              != IDE_SUCCESS );
                    IDE_TEST( execAggregation( aTemplate, sCodePlan, sDataPlan, i )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            IDE_TEST( qmnROLL::doItNextSortMTR( aTemplate, aPlan, aFlag )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sDataPlan->isDataNone = ID_FALSE;
        sDataPlan->needCopy   = ID_FALSE;
        sDataPlan->groupIndex = -1;

        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnROLL::doItNextSortMTR( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag )
{
    qmncROLL   * sCodePlan = (qmncROLL *)aPlan;
    qmndROLL   * sDataPlan = (qmndROLL *)(aTemplate->tmplate.data + aPlan->offset);
    mtcColumn  * sColumn   = NULL;
    void       * sSearchRow;
    void       * sOrgRow;
    idBool       sAllMatched;
    UInt         sGroupCount;
    UShort       i;

    if ( ( sCodePlan->aggrNode   != NULL ) &&
         ( sDataPlan->groupIndex != 0 ) )
    {
        /* 이미 올려진 그룹에 대해 Aggregation을 초기화한다. */
        IDE_TEST( initAggregation( aTemplate, sDataPlan, sDataPlan->groupIndex - 1 )
                  != IDE_SUCCESS );
        if ( sCodePlan->distNode != NULL )
        {
            IDE_TEST( clearDistNode( sCodePlan, sDataPlan, sDataPlan->groupIndex - 1 )
                      != IDE_SUCCESS);

            IDE_TEST( setDistMtrColumns( aTemplate,
                                         sCodePlan,
                                         sDataPlan,
                                         sDataPlan->groupIndex -1)
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( execAggregation( aTemplate,
                                   sCodePlan,
                                   sDataPlan,
                                   sDataPlan->groupIndex - 1 )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* Rollup 그룹 중 예를 들어 rollup( i1, i2, i3 ) 중에 (i1,i2) (i1) 그룹에서
     * 그룹이 달라져서 상위 로 보내져야할 때 아래와 같이 적절한 컬럼의 NULL로 만들고 해당
     * 그룹의 Aggr Row를 지정해서 NORMAL_EXIT로 끝나게 된다.
     */
    if ( sCodePlan->partialRollup != -1 )
    {
        sGroupCount = sCodePlan->groupCount; /* pratial Rollup */
    }
    else
    {
        sGroupCount = sCodePlan->groupCount - 1; /* just Rollup */
    }

    if ( ( sDataPlan->groupIndex < (SInt)( sGroupCount ) ) &&
         ( sDataPlan->groupStatus[sDataPlan->groupIndex] == QMND_ROLL_GROUP_NOT_MATCHED ) )
    {
        for ( i = 0, sColumn = sDataPlan->plan.myTuple->columns;
              i < sCodePlan->myNodeCount;
              i++, sColumn++ )
        {
            if ( sCodePlan->mtrGroup[sDataPlan->groupIndex][i] == QMND_ROLL_COLUMN_CHECK_NONE )
            {
                if ( sColumn != NULL )
                {
                    sColumn->module->null( sColumn,
                                           (void *) mtc::value(
                                               sColumn,
                                               sDataPlan->plan.myTuple->row,
                                               MTD_OFFSET_USE ) );
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
        }
        sDataPlan->plan.myTuple->modify++;

        if ( sCodePlan->aggrNode != NULL )
        {
            sDataPlan->aggrTuple->row = sDataPlan->aggrRow[sDataPlan->groupIndex];
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->groupIndex++;

        /* BUG-38093 The number of rollup's result is not corrent */
        if ( ( sDataPlan->isDataNone == ID_TRUE ) &&
             ( sCodePlan->partialRollup != -1 ) &&
             ( (UInt)sDataPlan->groupIndex == sGroupCount ) )
        {
            sDataPlan->doIt = qmnROLL::doItFirstSortMTR;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sDataPlan->needCopy == ID_TRUE )
    {
        /* 서로 다른 그룹이 발견되어 이전 그룹의 myNode가 처리되어 myNode에 새로운 값을 넣게 된다. */
        IDE_TEST( copyMtrRowToMyRow( sDataPlan )
                  != IDE_SUCCESS );
        sDataPlan->needCopy = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    /* 데이터가 없을 경우 총계에 대한 Row를 위로 올려보낸다. 역시 마지막 column을 NULL로 만들고
     * 총계에 해당하는 Aggr Row를 지정한뒤에 doIt 함수를 doItFirst로 지정해 doItFirst에서
     * data none을 올려주도록 한다.
     */
    if ( sDataPlan->isDataNone == ID_TRUE )
    {
        sColumn = sDataPlan->plan.myTuple->columns;
        if ( sCodePlan->partialRollup != -1 )
        {
            for ( i = 0, sColumn = sDataPlan->plan.myTuple->columns;
                  i < (UInt)sCodePlan->partialRollup; i++, sColumn++ )
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        for ( i = 0; i < sCodePlan->elementCount[0]; i++, sColumn++ )
        {
            if ( sColumn != NULL )
            {
                sColumn->module->null( sColumn,
                                       (void *) mtc::value(
                                           sColumn,
                                           sDataPlan->plan.myTuple->row,
                                           MTD_OFFSET_USE ) );
            }
            else
            {
                /* Nothing to do */
            }
        }
        sDataPlan->plan.myTuple->modify++;

        if ( sCodePlan->aggrNode != NULL )
        {
            sDataPlan->aggrTuple->row = sDataPlan->aggrRow[sCodePlan->groupCount - 1];
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->groupIndex++;
        sDataPlan->doIt = qmnROLL::doItFirstSortMTR;
        IDE_CONT( NORMAL_EXIT )
    }
    else
    {
        /* Nothing to do */
    }

    sOrgRow = sSearchRow = sDataPlan->mtrTuple->row;
    IDE_TEST( qmcSortTemp::getNextSequence( &sDataPlan->sortMTR,
                                            &sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->mtrTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;
    sDataPlan->mtrTuple->modify++;

    if ( sSearchRow != NULL )
    {
        while( 1 )
        {
            IDE_TEST( setTupleMtrNode( aTemplate, sDataPlan )
                      != IDE_SUCCESS );

            /* distNode가 있을 경우 현제 읽은 Row의 Distinct 여부를 판별한다. */
            if ( sCodePlan->distNode != NULL )
            {
                for ( i = 0; i < sCodePlan->groupCount; i ++ )
                {
                    IDE_TEST( setDistMtrColumns( aTemplate,
                                                 sCodePlan,
                                                 sDataPlan,
                                                 i )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }

            /* myNode와 mtrNode의 Row를 서로 비교해 각 컬럼이 맞는지 비교한다 */
            IDE_TEST( compareRows( sDataPlan )
                      != IDE_SUCCESS );

            /* 각 그룹별로 그룹이 되는지 아닌지를 판별하고 그룹이라면 exeAggr을 수행한다. */
            IDE_TEST( compareGroupsExecAggr( aTemplate,
                                             sCodePlan,
                                             sDataPlan,
                                             &sAllMatched )
                      != IDE_SUCCESS );

            /* 모든 그룹이 맞다면 다음 Row를 읽어서 반복한다 */
            if ( sAllMatched == ID_TRUE )
            {
                sOrgRow = sSearchRow = sDataPlan->mtrTuple->row;
                IDE_TEST( qmcSortTemp::getNextSequence( &sDataPlan->sortMTR,
                                                        &sSearchRow )
                          != IDE_SUCCESS );
                sDataPlan->mtrTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;
                sDataPlan->mtrTuple->modify++;

                if ( sSearchRow == NULL )
                {
                    /* 데이터가 존재 하지 않는다면 groupStatus를 모두 not matched로 만들어
                     * 모든 그룹의 데이터를 올려보내게 한다.
                     */
                    sDataPlan->isDataNone = ID_TRUE;
                    sDataPlan->groupIndex = 1;

                    if ( sCodePlan->aggrNode != NULL )
                    {
                         sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
                    }
                    else
                    {
                         /* Nothing to do */
                    }

                    for ( i = 1; i < sCodePlan->groupCount - 1; i ++ )
                    {
                        sDataPlan->groupStatus[i] = QMND_ROLL_GROUP_NOT_MATCHED;
                    }
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* [0] 번 그룹에 대당하는 Aggregation노드를 지정후에 나간다 */
                sDataPlan->needCopy   = ID_TRUE;
                sDataPlan->groupIndex = 1;

                if ( sCodePlan->aggrNode != NULL )
                {
                    sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            }
        }
    }
    else
    {
        /* 데이터가 존재 하지 않는다면 groupStatus를 모두 not matched로 만들어
         * 모든 그룹의 데이터를 올려보내게 한다.
         */
        sDataPlan->isDataNone = ID_TRUE;
        sDataPlan->groupIndex = 1;

        if ( sCodePlan->aggrNode != NULL )
        {
             sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
        }
        else
        {
             /* Nothing to do */
        }
        for ( i = 1; i < sCodePlan->groupCount - 1; i ++ )
        {
            sDataPlan->groupStatus[i] = QMND_ROLL_GROUP_NOT_MATCHED;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT )

    if ( sCodePlan->aggrNode != NULL )
    {
        IDE_TEST( finiAggregation( aTemplate, sDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    *aFlag = QMC_ROW_DATA_EXIST;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * valueTempStore
 *
 *  데이블이 메모리이고 상위에서 Sort나 Window Sort와 같이 메모리 포인터를 쌓아서 작업을 하는
 *  경우에 기존 로직을 동작시켜 새로운 Temp에 저장한다.
 */
IDE_RC qmnROLL::valueTempStore( qcTemplate * aTemplate,
                                qmnPlan    * aPlan )
{
    qmncROLL    * sCodePlan = ( qmncROLL * )aPlan;
    qmndROLL    * sDataPlan = ( qmndROLL * )( aTemplate->tmplate.data + aPlan->offset );
    qmcRowFlag    sFlag     = QMC_ROW_INITIALIZE;
    qmdMtrNode  * sNode     = NULL;
    qmdMtrNode  * sMemNode  = NULL;
    qmdAggrNode * sAggrNode = NULL;
    void        * sRow      = NULL;
    void        * sSrcValue;
    void        * sDstValue;
    UInt          sSize;
    UInt          i         = 0;

    if ( sCodePlan->preservedOrderMode == ID_TRUE )
    {
        IDE_TEST( qmnROLL::doItFirst( aTemplate, aPlan, &sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmnROLL::doItFirstSortMTR( aTemplate, aPlan, &sFlag )
                  != IDE_SUCCESS );
    }

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sRow = sDataPlan->valueTempTuple->row;
        IDE_TEST( qmcSortTemp::alloc( sDataPlan->valueTempMTR, &sRow )
                  != IDE_SUCCESS );
        
        for ( i = 0, sNode = sDataPlan->myNode, sMemNode = sDataPlan->valueTempNode;
              i < sCodePlan->myNodeCount;
              i++, sNode = sNode->next, sMemNode = sMemNode->next )
        {
            sSrcValue = (void *) mtc::value( sNode->dstColumn,
                                             sDataPlan->myRow,
                                             MTD_OFFSET_USE );

            sDstValue = (void *) mtc::value( sMemNode->dstColumn,
                                             sRow,
                                             MTD_OFFSET_USE );
                
            sSize = sNode->dstColumn->module->actualSize( sNode->dstColumn,
                                                          sSrcValue );

            idlOS::memcpy( (SChar *)sDstValue,
                           (SChar *)sSrcValue,
                           sSize );
        }
        
        for ( i = 0, sAggrNode = sDataPlan->aggrNode;
              i < sCodePlan->aggrNodeCount;
              i++, sAggrNode = sAggrNode->next, sMemNode = sMemNode->next )
        {
            sSrcValue = (void *) mtc::value( sAggrNode->dstColumn,
                                             sDataPlan->aggrTuple->row,
                                             MTD_OFFSET_USE );

            sDstValue = (void *) mtc::value( sMemNode->dstColumn,
                                             sRow,
                                             MTD_OFFSET_USE );

            sSize = sAggrNode->dstColumn->module->actualSize( sAggrNode->dstColumn,
                                                              sSrcValue );
            
            idlOS::memcpy( (SChar *)sDstValue,
                           (SChar *)sSrcValue,
                           sSize );
        }

        IDE_TEST( qmcSortTemp::addRow( sDataPlan->valueTempMTR, sRow )
                  != IDE_SUCCESS );
        
        IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, &sFlag )
                  != IDE_SUCCESS );
    }

    // PROJ-2462 Result Cache
    if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        *sDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_STORED_MASK;
        *sDataPlan->resultData.flag |= QMX_RESULT_CACHE_STORED_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * doItFirstValeTempMTR
 *
 *  데이블이 메모리이고 상위에서 Sort나 Window Sort와 같이 메모리 포인터를 쌓아서 작업을 하는
 *  경우에 기존 로직을 동작시켜 새로운 Memory Temp에 저장시키고 이때Temp에서 첫번 째 Row를 얻는다.
 */
IDE_RC qmnROLL::doItFirstValueTempMTR( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag )
{
    qmndROLL * sDataPlan = ( qmndROLL * )( aTemplate->tmplate.data + aPlan->offset );
    void     * sSearchRow;
    void     * sOrgRow;

    sOrgRow = sSearchRow = sDataPlan->valueTempTuple->row;
    IDE_TEST( qmcSortTemp::getFirstSequence( sDataPlan->valueTempMTR,
                                             &sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->valueTempTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;

    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        IDE_TEST( setTupleValueTempNode( aTemplate,
                                         sDataPlan )
                  != IDE_SUCCESS );
        
        sDataPlan->doIt = doItNextValueTempMTR;
        *aFlag = QMC_ROW_DATA_EXIST;
        sDataPlan->valueTempTuple->modify++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * doItNextValueTempMTR
 *
 *  데이블이 메모리이고 상위에서 Sort나 Window Sort와 같이 메모리 포인터를 쌓아서 작업을 하는
 *  경우에 기존 로직을 동작시켜 새로운 Memory Temp에 저장시키고 이때Temp에서 다 째 Row를 얻는다.
 */
IDE_RC qmnROLL::doItNextValueTempMTR( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan,
                                      qmcRowFlag * aFlag )
{
    qmndROLL * sDataPlan = ( qmndROLL * )( aTemplate->tmplate.data + aPlan->offset );
    void     * sSearchRow;
    void     * sOrgRow;

    sOrgRow = sSearchRow = sDataPlan->valueTempTuple->row;
    IDE_TEST( qmcSortTemp::getNextSequence( sDataPlan->valueTempMTR,
                                            &sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->valueTempTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;

    if ( sSearchRow == NULL )
    {
        sDataPlan->doIt = doItFirstValueTempMTR;
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        IDE_TEST( setTupleValueTempNode( aTemplate,
                                         sDataPlan )
                  != IDE_SUCCESS );
        
        *aFlag = QMC_ROW_DATA_EXIST;
        sDataPlan->valueTempTuple->modify++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnROLL::printPlan( qcTemplate   * aTemplate,
                           qmnPlan      * aPlan,
                           ULong          aDepth,
                           iduVarString * aString,
                           qmnDisplay     aMode )
{
    qmncROLL   * sCodePlan = ( qmncROLL * )aPlan;
    qmndROLL   * sDataPlan = ( qmndROLL * )( aTemplate->tmplate.data +
                                           aPlan->offset );
    qmndROLL   * sCacheDataPlan = NULL;
    idBool       sIsInit       = ID_FALSE;
    UInt         sSelfID;
    ULong        i;

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    if ( sCodePlan->valueTempNode != NULL )
    {
        sSelfID = (UInt)sCodePlan->valueTempNode->dstNode->node.table;
    }
    else
    {
        sSelfID = (UInt)sCodePlan->myNode->dstNode->node.table;
    }

    for ( i = 0; i < aDepth; i ++ )
    {
        iduVarStringAppend( aString, " " );
    }

    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( ( *sDataPlan->flag & QMND_ROLL_INIT_DONE_MASK )
              == QMND_ROLL_INIT_DONE_TRUE )
        {
            sIsInit = ID_TRUE;
            if ( sCodePlan->valueTempNode != NULL )
            {
                iduVarStringAppendFormat( aString,
                                          "GROUP-ROLLUP ( ACCESS: %"ID_UINT32_FMT,
                                          sDataPlan->valueTempTuple->modify );
            }
            else
            {
                iduVarStringAppendFormat( aString,
                                          "GROUP-ROLLUP ( ACCESS: %"ID_UINT32_FMT,
                                          sDataPlan->plan.myTuple->modify );
            }
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "GROUP-ROLLUP ( ACCESS: 0" );
        }
    }
    else
    {
        iduVarStringAppendFormat( aString,
                                  "GROUP-ROLLUP ( ACCESS: ??" );
    }

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    /* PROJ-2462 Result Cache */
    if ( QCU_TRCLOG_DETAIL_RESULTCACHE == 1 )
    {
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            qmn::printResultCacheRef( aString,
                                      aDepth,
                                      sCodePlan->componentInfo );
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
    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        /* PROJ-2462 Result Cache */
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            sCacheDataPlan = (qmndROLL *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
            qmn::printResultCacheInfo( aString,
                                       aDepth,
                                       aMode,
                                       sIsInit,
                                       &sCacheDataPlan->resultData );
        }
        else
        {
            /* Nothing to do */
        }
        if ( sCodePlan->valueTempNode != NULL )
        {
            // BUG-36716
            qmn::printMTRinfo( aString,
                            aDepth,
                            sCodePlan->valueTempNode,
                            "valueTempNode",
                            sSelfID,
                            sCodePlan->depTupleID,
                            ID_USHORT_MAX );
        }
        else
        {
            /* Nothing to do */
        }

        // BUG-36716
        qmn::printMTRinfo( aString,
                           aDepth,
                           sCodePlan->myNode,
                           "myNode",
                           sSelfID,
                           sCodePlan->depTupleID,
                           ID_USHORT_MAX );

        if ( sCodePlan->aggrNode != NULL )
        {
            // BUG-36716
            qmn::printMTRinfo( aString,
                               aDepth,
                               sCodePlan->aggrNode,
                               "aggrNode",
                               sSelfID,
                               sCodePlan->depTupleID,
                               ID_USHORT_MAX );

            if ( sCodePlan->distNode != NULL )
            {
                // BUG-36716
                qmn::printMTRinfo( aString,
                                   aDepth,
                                   sCodePlan->distNode,
                                   "distNode",
                                   sSelfID,
                                   sCodePlan->depTupleID,
                                   ID_USHORT_MAX );
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

        // BUG-36716
        qmn::printMTRinfo( aString,
                           aDepth,
                           sCodePlan->mtrNode,
                           "mtrNode",
                           sSelfID,
                           sCodePlan->depTupleID,
                           ID_USHORT_MAX );
    }
    else
    {
        /* Nothing to do */
    }

    if ( QCU_TRCLOG_RESULT_DESC == 1 )
    {
        IDE_TEST( qmn::printResult( aTemplate,
                                    aDepth,
                                    aString,
                                    sCodePlan->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( aPlan->left->printPlan( aTemplate,
                                      aPlan->left,
                                      aDepth + 1,
                                      aString,
                                      aMode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

