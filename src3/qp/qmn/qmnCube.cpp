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
 * $Id: qmnCube.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * PROJ-1353
 *
 * CUBE는 모든 Row를 Sort Temp에 쌓아서 Sorting을 여러번 수행하므로써 CUBE를 수행한다.
 * CUBE는 2^n의 그룹의 계산을 수행하며 2^(n-1) 만큼의 Sort를 재 수행하므로써 CUBE
 * 를 수행한다.
 *
 *
 ***********************************************************************/
#include <idl.h>
#include <ide.h>
#include <qmnCube.h>
#include <qcuProperty.h>
#include <qmxResultCache.h>

/**
 * init
 *
 *  CUBE Plan 초기화 수행
 */
IDE_RC qmnCUBE::init( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    qmncCUBE * sCodePlan = (qmncCUBE *)aPlan;
    qmndCUBE * sDataPlan = (qmndCUBE *)(aTemplate->tmplate.data + aPlan->offset);
    idBool     sDep = ID_FALSE;

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnCUBE::doItDefault;

    if ( ( *sDataPlan->flag & QMND_CUBE_INIT_DONE_MASK )
         == QMND_CUBE_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit( aTemplate, sCodePlan, sDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        sDataPlan->plan.myTuple->row = sDataPlan->myRow;
    }

    /* doIt중에 init이 호출되는 경우 재수행을 위해 초기화한다. */
    if ( sDataPlan->groupIndex != sCodePlan->groupCount - 1 )
    {
        if ( findRemainGroup( sDataPlan ) == ID_UINT_MAX )
        {
            sDataPlan->isDataNone     = ID_FALSE;
            sDataPlan->noStoreChild   = ID_FALSE;
            sDataPlan->groupIndex     = sCodePlan->groupCount - 1;
            sDataPlan->compareResults = 0;
            sDataPlan->subStatus      = 0;
            sDataPlan->subIndex       = -1;

            initGroups( sCodePlan, sDataPlan );
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

    IDE_TEST( checkDependency( aTemplate, sCodePlan, sDataPlan, &sDep )
              != IDE_SUCCESS );

    if ( sDep == ID_TRUE )
    {
        IDE_TEST( qmcSortTemp::clear( & sDataPlan->sortMTR ) != IDE_SUCCESS );

        IDE_TEST( storeChild( aTemplate, sCodePlan, sDataPlan )
                  != IDE_SUCCESS );

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
        sDataPlan->doIt = qmnCUBE::doItFirstValueTemp;
    }
    else
    {
        sDataPlan->doIt = qmnCUBE::doItFirst;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnCUBE::checkDependency( qcTemplate * aTemplate,
                                 qmncCUBE   * aCodePlan,
                                 qmndCUBE   * aDataPlan,
                                 idBool     * aDependent )
{
    idBool sDep = ID_FALSE;

    /* Dependency가 같고 valueTemp가 있는 경우만 재사용이 가능하다. */
    if ( aCodePlan->valueTempNode != NULL )
    {
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
    }
    else
    {
        sDep = ID_TRUE;
    }

    *aDependent = sDep;

    return IDE_SUCCESS;
}

IDE_RC qmnCUBE::doItDefault( qcTemplate *,
                             qmnPlan    *,
                             qmcRowFlag * )
{
    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}


IDE_RC qmnCUBE::doIt( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * aFlag )
{
    qmndCUBE * sDataPlan = (qmndCUBE *)(aTemplate->tmplate.data
                                        + aPlan->offset );

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * linkAggrNode
 *
 *  CodePlan의 AggrNode를 DataPlan의 aggrNode로 연결한다.
 */
IDE_RC qmnCUBE::linkAggrNode( qmncCUBE * aCodePlan, qmndCUBE * aDataPlan )
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
 *  Distinct 컬럼이 d 개일 경우  Group 수( n + 1 ) * d 만큼의 DistNode가 생성된다.
 *  이는 2중 배열을 통해서 관리되며 각각 Distinct를 위한 HashTemp를 생성한다.
 */
IDE_RC qmnCUBE::initDistNode( qcTemplate * aTemplate,
                              qmncCUBE   * aCodePlan,
                              qmndCUBE   * aDataPlan )
{
    qmcMtrNode  * sCNode;
    qmdDistNode * sDistNode;
    UInt          sFlag;
    UInt          sHeaderSize;
    UInt          sGroup;
    UInt          i;

    aDataPlan->distNode = ( qmdDistNode ** )( aTemplate->tmplate.data +
                                              aCodePlan->distNodePtrOffset );

    for ( i = 0; i < (UInt)aCodePlan->cubeCount + 1; i++ )
    {
        aDataPlan->distNode[i] = ( qmdDistNode * )( aTemplate->tmplate.data +
                                  aCodePlan->distNodeOffset +
                                  idlOS::align8(ID_SIZEOF(qmdDistNode) *
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

    for( sGroup = 0; sGroup < (UInt)aCodePlan->cubeCount + 1; sGroup++ )
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
                                          sHeaderSize)
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
 * initMtrNode
 *
 *  mtrNode, myNode, aggrNode, distNode, memoeryValeNode의 초기화를 수행한다.
 *
 *  aggrNode는 Cube용으로 1개만 생성되지만 distNode는 Cube 컬럼 + 1개 만큼 생성한다.
 *  일단 distNode의 0번째로 연결해 놓는다.
 */
IDE_RC qmnCUBE::initMtrNode( qcTemplate * aTemplate,
                             qmncCUBE   * aCodePlan,
                             qmndCUBE   * aDataPlan )
{
    qmdAggrNode * sAggrNode = NULL;
    qmdDistNode * sDistNode = NULL;
    qmdMtrNode  * sNode     = NULL;
    qmdMtrNode  * sSNode    = NULL;
    qmdMtrNode  * sNext     = NULL;
    qmcMtrNode  * sCNode    = NULL;
    UInt          sFlag     = QMCD_SORT_TMP_INITIALIZE;
    UInt          sHeaderSize;
    UInt          i;
    iduMemory   * sMemory        = NULL;
    qmndCUBE    * sCacheDataPlan = NULL;

    /* myNode */
    aDataPlan->myNode = ( qmdMtrNode * )( aTemplate->tmplate.data +
                                          aCodePlan->myNodeOffset );
    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->myNode )
              != IDE_SUCCESS );
    IDE_TEST( qmc::initMtrNode( aTemplate, aDataPlan->myNode, 0 )
              != IDE_SUCCESS );
    IDE_TEST( qmc::refineOffsets( aDataPlan->myNode, 0 )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple = aDataPlan->myNode->dstTuple;

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               &aTemplate->tmplate,
                               aDataPlan->myNode->dstNode->node.table )
              != IDE_SUCCESS );
    aDataPlan->myRowSize = qmc::getMtrRowSize( aDataPlan->myNode );

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sHeaderSize  = QMC_MEMSORT_TEMPHEADER_SIZE;
        sFlag       &= ~QMCD_SORT_TMP_STORAGE_TYPE;
        sFlag       |= QMCD_SORT_TMP_STORAGE_MEMORY;
    }
    else
    {
        sHeaderSize  = QMC_DISKSORT_TEMPHEADER_SIZE;
        sFlag       &= ~QMCD_SORT_TMP_STORAGE_TYPE;
        sFlag       |= QMCD_SORT_TMP_STORAGE_DISK;
        /* PROJ-2201 재정렬, BackwardScan등을 하려면 RangeFlag를 줘야함 */
        sFlag       &= ~QMCD_SORT_TMP_SEARCH_MASK;
        sFlag       |= QMCD_SORT_TMP_SEARCH_RANGE;
    }

    /* mtrNode */
    aDataPlan->mtrNode = ( qmdMtrNode * )( aTemplate->tmplate.data +
                                           aCodePlan->mtrNodeOffset );
    IDE_TEST( qmc::linkMtrNode( aCodePlan->mtrNode,
                                aDataPlan->mtrNode )
              != IDE_SUCCESS );
    IDE_TEST( qmc::initMtrNode( aTemplate, aDataPlan->mtrNode, 0 )
              != IDE_SUCCESS );
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode, sHeaderSize )
              != IDE_SUCCESS );
    aDataPlan->mtrTuple = aDataPlan->mtrNode->dstTuple;

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               &aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );
    aDataPlan->mtrRowSize  = qmc::getMtrRowSize( aDataPlan->mtrNode );

    aDataPlan->sortNode = ( qmdMtrNode * )( aTemplate->tmplate.data +
                                            aCodePlan->sortNodeOffset );
    /* sortNode와 mtrNode과 myNode 갯수만 큼만 같다 */
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

    if ( aCodePlan->distNode != NULL )
    {
        IDE_TEST( initDistNode( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* aggrNode */
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
        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   &aTemplate->tmplate,
                                   aDataPlan->aggrNode->dstNode->node.table )
                  != IDE_SUCCESS );
        aDataPlan->aggrRowSize =
                       qmc::getMtrRowSize( (qmdMtrNode *)aDataPlan->aggrNode );
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
            sCacheDataPlan = (qmndCUBE *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
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
            IDU_FIT_POINT( "qmnCUBE::initMtrNode::qmxAlloc:valueTempMTR",
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
            sCacheDataPlan = (qmndCUBE *) (aTemplate->resultCache.data +
                                          aCodePlan->plan.offset);
            if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
                 == QMX_RESULT_CACHE_INIT_DONE_FALSE )
            {
                IDU_FIT_POINT( "qmnCUBE::initMtrNode::qrcAlloc:valueTempMTR",
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
IDE_RC qmnCUBE::allocMtrRow( qcTemplate * aTemplate,
                             qmncCUBE   * aCodePlan,
                             qmndCUBE   * aDataPlan )
{
    iduMemory * sMemory;
    void      * sRow;
    UInt        i;

    sMemory = aTemplate->stmt->qmxMem;

    IDE_TEST( sMemory->alloc( aDataPlan->mtrRowSize,
                              ( void ** )&aDataPlan->mtrRow )
              != IDE_SUCCESS );
    aDataPlan->mtrTuple->row = aDataPlan->mtrRow;

    IDE_TEST( sMemory->alloc( aDataPlan->myRowSize,
                              ( void ** )&aDataPlan->myRow )
              != IDE_SUCCESS );

    IDE_TEST( sMemory->alloc( aDataPlan->myRowSize,
                              ( void ** )&aDataPlan->nullRow )
              != IDE_SUCCESS );

    IDE_TEST( sMemory->alloc( idlOS::align( ID_SIZEOF( UInt )
                                            * ( aCodePlan->cubeCount + 1 )),
                              ( void ** )&aDataPlan->subIndexToGroupIndex )
              != IDE_SUCCESS );

    IDE_TEST( qmn::makeNullRow( aDataPlan->plan.myTuple,
                                aDataPlan->nullRow )
              != IDE_SUCCESS );

    /**
     * aggrNode의 경우 mtrNode는 1개만 있지만 Row는 Rollup 그룹수(n+1) 만큼 존재하게 된다.
     * 이를 통해서 한번 비교를 통해서 ( n + 1 ) 그룹의 aggregation을 수행할 수 있다.
     * Cube는 Rollup을 반복해서 수행하므로써 수행하므로 Aggregaion Row 역시 Rollup 그룹수
     * 만큼 생성된다.
     */
    if ( aCodePlan->aggrNode != NULL )
    {
        IDE_TEST( sMemory->alloc( idlOS::align( ID_SIZEOF( void * )
                                                * ( aCodePlan->cubeCount + 1 )),
                                  ( void ** )&aDataPlan->aggrRow )
                  != IDE_SUCCESS );

        for ( i = 0; i < (UInt)aCodePlan->cubeCount + 1; i ++ )
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
 * initAggregation
 *
 *   Aggregation Node의 초기화를 수행한다.
 */
IDE_RC qmnCUBE::initAggregation( qcTemplate * aTemplate,
                                 qmndCUBE   * aDataPlan,
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
 * execAggregation
 *
 *   해당하는 그룹의 Row의 Aggregation의 계산을 수행한다.
 *   만약 Distinct 노드가 있는경우에는 해당하는 distNode의 distinct여부를 보고
 *   aggregate 의 수행 여부를 결정한다.
 */
IDE_RC qmnCUBE::execAggregation( qcTemplate * aTemplate,
                                 qmncCUBE   * aCodePlan,
                                 qmndCUBE   * aDataPlan,
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
IDE_RC qmnCUBE::finiAggregation( qcTemplate * aTemplate,
                                 qmndCUBE   * aDataPlan )
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
 * setTupleMtrNode
 *
 *  memory pointer를 쌓았을 경우 이를 원본한다.
 */
IDE_RC qmnCUBE::setTupleMtrNode( qcTemplate * aTemplate,
                                 qmndCUBE   * aDataPlan )
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

IDE_RC qmnCUBE::setTupleValueTempNode( qcTemplate * aTemplate,
                                       qmndCUBE   * aDataPlan )
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

/**
 * setMtrRow
 *
 *   현제 Row에 setMtr을 수행한다. 이를 통해서 각 컬럼이 제대로 값을 얻는다.
 */
IDE_RC qmnCUBE::setMtrRow( qcTemplate * aTemplate,
                           qmndCUBE   * aDataPlan )
{
    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
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
 * copy MtrRow to My Row using pointer
 *
 *  pointer로 올라온 데이터를 myRow에 복사해준다.
 *
 *   RowNum이나 level같은 경우 Pointer가 아닌 value로 올라온다. 따라서 value처리해야한다.
 */
IDE_RC qmnCUBE::copyMtrRowToMyRow( qmndCUBE * aDataPlan )
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

    return IDE_SUCCESS;
}

/**
 * makeGroups
 *
 *  CUBE group을 생성한다. CUBE의 경우 UShort Type의 Bitmap형태로 group을 표현한다.
 *  CUBE는 총 2^n개의 그룹이 나올 수 있으므로 2^n개 만큼의 UShort를 할당해서 여기에
 *  그룹을 표시한다. 이 그룹은 POWER SET을 구하는 알고리즘을 참조해서 구현했음
 *  POWER SET 알고리즘 ( http://rosettacode.org/wiki/Power_set )
 */
IDE_RC qmnCUBE::makeGroups( qcTemplate * aTemplate,
                            qmncCUBE   * aCodePlan,
                            qmndCUBE   * aDataPlan )
{
    iduMemory * sMemory;

    sMemory = aTemplate->stmt->qmxMem;

    IDE_TEST( sMemory->alloc( idlOS::align( ID_SIZEOF( UShort )
                                            * aCodePlan->groupCount ),
                              ( void ** )&aDataPlan->cubeGroups )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aDataPlan->cubeGroups == NULL, err_mem_alloc );

    initGroups( aCodePlan, aDataPlan );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_mem_alloc )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MEMORY_ALLOCATION));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * initGroups
 */
void qmnCUBE::initGroups( qmncCUBE   * aCodePlan,
                          qmndCUBE   * aDataPlan )
{
    UInt  sBits;
    UInt  i;
    UInt  j;

    for ( i = 0; i < aCodePlan->groupCount; i++ )
    {
        aDataPlan->cubeGroups[i] = 0;
        for ( sBits = i, j = 0; sBits != 0; sBits >>= 1, j++ )
        {
            if ( sBits & 0x1 )
            {
                aDataPlan->cubeGroups[i] |= 0x1 << j;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
}

/**
 * storeChild
 *
 *   모든 ROW를 SORT Temp에 쌓는다. 이렇게 쌓으면서 총계를 구한다.
 */
IDE_RC qmnCUBE::storeChild( qcTemplate * aTemplate,
                            qmncCUBE   * aCodePlan,
                            qmndCUBE   * aDataPlan )
{
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;

    /* Child Plan의 초기화 */
    IDE_TEST( aCodePlan->plan.left->init( aTemplate,
                                          aCodePlan->plan.left )
              != IDE_SUCCESS);

    /* Child Plan의 결과를 저장 */
    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* partialCube가 아닌경우 총계를 구하기 위한 초기화를 수행한다 */
        if ( aCodePlan->partialCube == -1 )
        {
            if ( aCodePlan->distNode != NULL )
            {
                IDE_TEST( qmnCUBE::clearDistNode( aCodePlan, aDataPlan, 0 )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            if ( aCodePlan->aggrNode != NULL )
            {
                IDE_TEST( initAggregation( aTemplate, aDataPlan, 0 )
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
    }
    else
    {
        aDataPlan->noStoreChild = ID_TRUE;
        IDE_CONT( NORMAL_EXIT );
    }

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::alloc( &aDataPlan->sortMTR,
                                      &aDataPlan->mtrTuple->row )
                  != IDE_SUCCESS);

        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        if ( aCodePlan->partialCube == -1 )
        {
            /* partialCube가 아닌경우 총계 계산을 수행한다 */
            if ( aCodePlan->distNode != NULL )
            {
                IDE_TEST( qmnCUBE::setDistMtrColumns( aTemplate,
                                                      aCodePlan,
                                                      aDataPlan,
                                                      0 )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            if ( aCodePlan->aggrNode != NULL )
            {
                IDE_TEST( execAggregation( aTemplate, aCodePlan, aDataPlan, 0 )
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
        IDE_TEST( qmcSortTemp::addRow( &aDataPlan->sortMTR,
                                       aDataPlan->mtrTuple->row )
                  != IDE_SUCCESS );

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              &sFlag )
                  != IDE_SUCCESS );
    }

    if ( aCodePlan->partialCube == -1 )
    {
        if ( aCodePlan->distNode != NULL )
        {
            IDE_TEST( qmnCUBE::clearDistNode( aCodePlan, aDataPlan, 0 )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
        if ( aCodePlan->aggrNode != NULL )
        {
            IDE_TEST( finiAggregation( aTemplate, aDataPlan )
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

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * firstInit
 *
 *  CUBE 초기 initialize를 수행한다.
 */
IDE_RC qmnCUBE::firstInit( qcTemplate * aTemplate,
                           qmncCUBE   * aCodePlan,
                           qmndCUBE   * aDataPlan )
{
    mtcTuple * sTuple = NULL;
    qmndCUBE * sCacheDataPlan = NULL;

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndCUBE *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
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
    IDE_TEST( makeGroups( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    aDataPlan->isDataNone     = ID_FALSE;
    aDataPlan->noStoreChild   = ID_FALSE;
    aDataPlan->groupIndex     = aCodePlan->groupCount - 1;
    aDataPlan->compareResults = 0;
    aDataPlan->subStatus      = 0;
    aDataPlan->subIndex       = -1;
    aDataPlan->plan.myTuple->row   = aDataPlan->myRow;

    aDataPlan->groupingFuncData.info.type   = QMS_GROUPBY_CUBE;
    aDataPlan->groupingFuncData.info.index  = &aDataPlan->subIndex;
    aDataPlan->groupingFuncData.subIndexMap = aDataPlan->subIndexToGroupIndex;
    aDataPlan->groupingFuncData.groups      = aDataPlan->cubeGroups;

    sTuple = &aTemplate->tmplate.rows[aCodePlan->groupingFuncRowID];
    aDataPlan->groupingFuncPtr  = (SLong *)sTuple->row;
    *aDataPlan->groupingFuncPtr = (SLong)( &aDataPlan->groupingFuncData );

    aDataPlan->depTuple = &aTemplate->tmplate.rows[ aCodePlan->depTupleID ];
    aDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    *aDataPlan->flag &= ~QMND_CUBE_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_CUBE_INIT_DONE_TRUE;

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

/**
 * findRemainGroup
 *
 *  뒤에서 부터 구해지지 않은 그룹을 찾는다.
 */
UInt qmnCUBE::findRemainGroup( qmndCUBE * aDataPlan )
{
    UInt    sGroupIndex = ID_UINT_MAX;
    SInt    i;

    for ( i = (SInt)aDataPlan->groupIndex; i >= 0; i-- )
    {
        if ( ( aDataPlan->cubeGroups[i] & QMND_CUBE_GROUP_DONE_MASK )
             == QMND_CUBE_GROUP_DONE_FALSE )
        {
            sGroupIndex = (UInt)i;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return sGroupIndex;
}

/**
 * makeSortNodeAndSort
 *
 *  Bitmap순으로 SortNode를 설정하고 Sort를 수행한다.
 */
IDE_RC qmnCUBE::makeSortNodeAndSort( qmncCUBE * aCodePlan,
                                     qmndCUBE * aDataPlan )
{
    UInt         i;
    UInt         j;
    UShort       sMask;
    UShort       sCubeSortKey;
    UInt         sGroupIndex;
    qmdMtrNode * sNode;
    qmdMtrNode * sFirst = NULL;
    qmdMtrNode * sLast  = NULL;

    sGroupIndex  = aDataPlan->subIndexToGroupIndex[0];
    sCubeSortKey = aDataPlan->cubeGroups[sGroupIndex] & ~QMND_CUBE_GROUP_DONE_MASK;

    sNode = aDataPlan->sortNode;

    /* 먼저 CUBE에 해당하지 않는 컬럼을 설정한다 */
    for ( i = 0; (SInt)i < aCodePlan->partialCube; i++, sNode++ )
    {
        sNode->next = NULL;
        if ( sFirst == NULL )
        {
            sFirst = sNode;
            sLast  = sNode;
        }
        else
        {
            sLast->next = sNode;
            sLast       = sNode;
        }
    }

    /* CUBE의 컬럼중 해당 그룹에 포함하는 컬럼만 Sort Key를 설정한다. */
    for ( i = 0, sMask = 0x0001;
          i < aCodePlan->cubeCount;
          i ++, sMask <<= 1 )
    {
        sNode->next = NULL;
        if ( ( sCubeSortKey & sMask ) != 0x0000 )
        {
            for ( j = 0; j < aCodePlan->elementCount[i]; j++, sNode++ )
            {
                if ( sFirst == NULL )
                {
                    sFirst = sNode;
                    sLast  = sNode;
                }
                else
                {
                    sLast->next = sNode;
                    sLast       = sNode;
                }
            }
        }
        else
        {
            sNode += aCodePlan->elementCount[i];
        }
    }

    IDE_TEST( qmcSortTemp::setSortNode( &aDataPlan->sortMTR,
                                        sFirst )
              != IDE_SUCCESS );
    IDE_TEST( qmcSortTemp::sort( &aDataPlan->sortMTR )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * clearDistNode
 *
 *   그룹 index를 인자로 받아 해당하는 Distinct노드의 초기화를 수행한다.
 */
IDE_RC qmnCUBE::clearDistNode( qmncCUBE * aCodePlan,
                               qmndCUBE * aDataPlan,
                               UInt       aSubIndex )
{
    UInt          i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDataPlan->distNode[aSubIndex];
          i < aCodePlan->distNodeCount;
          i++, sDistNode++ )
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
 * setDistMtrColumns
 *
 *   그룹 Index를 인자로 받아 해당 인덱스의 distint Node의 HashTemp에 현제
 *   값을 넣어보고 겹치는 자료인지 아닌지를 설정한다.
 */
IDE_RC qmnCUBE::setDistMtrColumns( qcTemplate * aTemplate,
                                   qmncCUBE   * aCodePlan,
                                   qmndCUBE   * aDataPlan,
                                   UInt         aSubIndex )
{
    UInt          i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDataPlan->distNode[aSubIndex];
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
 * setSubGroups
 *
 *  CUBE의 Group은 PowerSet 알고리즘에 의해 생성되는데 모든 컬럼을 포함하는 그룹이 가장
 *  마지막에 설정된다. 따라서 역순으로 그룹 set을 수행한다.
 */
void qmnCUBE::setSubGroups( qmncCUBE * aCodePlan, qmndCUBE * aDataPlan )
{
    UShort       sTemp   = 0;
    UShort       sCount  = 0;
    UShort       sMask   = 0;
    idBool       sFound  = ID_FALSE;
    SInt         i;
    SInt         j;

    /**
     * 가장 마지막 그룹부터 계산을 수행하기 시작한다.
     */
    aDataPlan->subIndexToGroupIndex[0] = aDataPlan->groupIndex;
    sTemp = aDataPlan->cubeGroups[aDataPlan->groupIndex];
    aDataPlan->cubeGroups[aDataPlan->groupIndex] |= QMND_CUBE_GROUP_DONE_TRUE;
    j = 1;
    for ( sCount = 0, sMask = 0x0001 << (aCodePlan->cubeCount - 1);
          sCount < aCodePlan->cubeCount - 1;
          sCount++, sMask >>= 1 )
    {
        if ( ( sTemp & sMask ) == 0x0000 )
        {
            continue;
        }
        else
        {
            sTemp &= ~sMask;
        }
        sFound = ID_FALSE;
        for ( i = (SInt)aCodePlan->groupCount - 1; i >= 0; i-- )
        {
            if ( ( aDataPlan->cubeGroups[i] & QMND_CUBE_GROUP_DONE_MASK )
                  == QMND_CUBE_GROUP_DONE_TRUE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }
            if ( aDataPlan->cubeGroups[i] == sTemp )
            {
                sFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        if ( sFound == ID_TRUE )
        {
            aDataPlan->cubeGroups[i] |= QMND_CUBE_GROUP_DONE_TRUE;
            aDataPlan->subIndexToGroupIndex[j] = i;
            j++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    aDataPlan->subGroupCount = j;
}

/**
 * compareRows
 *
 *  myNode의 Row와 mtrNode의 Row를 비료해서 각 컬럼이 matching 여부를 표시한다.
 */
IDE_RC qmnCUBE::compareRows( qmndCUBE * aDataPlan )
{
    qmdMtrNode * sNode     = NULL;
    qmdMtrNode * sMtrNode = NULL;
    SInt         sResult   = -1;
    ULong        sMask     = 0x1;
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;
    void       * sRow;

    aDataPlan->compareResults = 0;

    sRow = aDataPlan->mtrTuple->row;

    for ( sNode = aDataPlan->myNode, sMtrNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next, sMtrNode = sMtrNode->next, sMask <<= 1 )
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
            aDataPlan->compareResults |= sMask;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;
}

/**
 * compareGroupExecAggr
 *
 *  CUBE의 그룹별로 matched 되어야 하는 컬럼이 다르므로 각 그룹별로 matched되어야
 *  하는 컬럼만 선별하여 compareResults가 되는지를 판별하고
 *  이 그룹이 myNode에 있는 자료와 그룹화 되어있다면 execAggregation을 수행한다.
 */
IDE_RC qmnCUBE::compareGroupsExecAggr( qcTemplate * aTemplate,
                                       qmncCUBE   * aCodePlan,
                                       qmndCUBE   * aDataPlan,
                                       idBool     * aAllMatched )
{
    idBool sIsGroup;
    idBool sAllMatch = ID_TRUE;
    UShort sGroupMask;
    UShort sMask;
    ULong  sElementMask = 0;
    UInt   sGroupIndex;
    UInt   sSubMask;
    UInt   i;
    UInt   j;
    UInt   k;

    for ( i = 0, sSubMask = 0x1;
          i < aDataPlan->subGroupCount;
          i ++, sSubMask <<= 1 )
    {
        sGroupIndex = aDataPlan->subIndexToGroupIndex[i];
        sGroupMask  = aDataPlan->cubeGroups[sGroupIndex] & ~QMND_CUBE_GROUP_DONE_MASK;
        sIsGroup   = ID_TRUE;

        sElementMask = 0x1;
        if ( aCodePlan->partialCube > 0 )
        {
            for ( j = 0;
                  j < (UInt)aCodePlan->partialCube;
                  j ++, sElementMask <<= 1 )
            {
                if ( aDataPlan->compareResults & sElementMask )
                {
                    sIsGroup = ID_TRUE;
                }
                else
                {
                    sIsGroup = ID_FALSE;
                    break;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }

        if ( sIsGroup == ID_TRUE )
        {
            for ( j = 0, sMask = 0x1;
                  j < aCodePlan->cubeCount;
                  j++, sMask <<= 1 )
            {
                if ( sGroupMask & sMask )
                {
                    for ( k = 0;
                          k < aCodePlan->elementCount[j];
                          k++, sElementMask <<= 1 )
                    {
                        if ( aDataPlan->compareResults & sElementMask )
                        {
                            /* Nothing to do */
                        }
                        else
                        {
                            sIsGroup = ID_FALSE;
                            break;
                        }
                    }
                    if ( sIsGroup == ID_FALSE )
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
                    for ( k = 0; k < aCodePlan->elementCount[j]; k++ )
                    {
                        sElementMask <<= 1;
                    }
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
        if ( sIsGroup == ID_TRUE )
        {
            aDataPlan->subStatus |= sSubMask;
            if ( aCodePlan->aggrNodeCount > 0 )
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
            aDataPlan->subStatus &= ~sSubMask;
            sAllMatch = ID_FALSE;
        }
    }

    *aAllMatched = sAllMatch;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * setColumnNULL
 *
 *  aSubIndex로 부터 전체 CUBE 그룹인텍스를 구하고 전체 그룹인덱스에서 CUBE 컬럼의 Bitmap
 *  을 구해서 NULL이 되어야 할 컬럼의 NULL을 수행한다.
 */
void qmnCUBE::setColumnNull( qmncCUBE * aCodePlan,
                             qmndCUBE * aDataPlan,
                             SInt       aSubIndex )
{
    UInt         sGroupIndex = 0;
    UShort       sSubMask    = 0;
    mtcColumn  * sColumn     = NULL;
    UShort       i;
    UShort       j;

    sGroupIndex = aDataPlan->subIndexToGroupIndex[aSubIndex];

    sColumn = aDataPlan->plan.myTuple->columns;
    if ( aCodePlan->partialCube != -1 )
    {
        sColumn += aCodePlan->partialCube;
    }
    else
    {
        /* Nothing to do */
    }

    for ( i = 0, sSubMask = 0x1;
          i < aCodePlan->cubeCount;
          i ++, sSubMask <<= 1 )
    {
        if ( ( aDataPlan->cubeGroups[sGroupIndex] & sSubMask ) == 0x0000 )
        {
            for ( j = 0; j < aCodePlan->elementCount[i]; j++ )
            {
                sColumn->module->null( sColumn,
                                       (void *) mtc::value(
                                           sColumn,
                                           aDataPlan->plan.myTuple->row,
                                           MTD_OFFSET_USE ) );
                sColumn++;
            }
        }
        else
        {
            sColumn += aCodePlan->elementCount[i];
        }
    }
}

/**
 * padNull
 *
 *   Null pading을 수행한다.
 */
IDE_RC qmnCUBE::padNull( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    qmncCUBE  * sCodePlan = ( qmncCUBE * )aPlan;
    qmndCUBE  * sDataPlan = ( qmndCUBE * )( aTemplate->tmplate.data + aPlan->offset );
    mtcColumn * sColumn;
    UInt        i;
    
    if ( ( aTemplate->planFlag[sCodePlan->planID] & QMND_CUBE_INIT_DONE_MASK )
         == QMND_CUBE_INIT_DONE_FALSE )
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * doItFirst
 *
 *  CUBE는 이미 쌓여진 SortTemp에서 2^(n-1) 만큼 Sort를 수행하면서 2^n 만큼의 그룹 계산을
 *  수행한다.
 */
IDE_RC qmnCUBE::doItFirst( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
    qmncCUBE   * sCodePlan  = (qmncCUBE *)aPlan;
    qmndCUBE   * sDataPlan  = (qmndCUBE *)(aTemplate->tmplate.data + aPlan->offset);
    mtcColumn  * sColumn    = NULL;
    void       * sSearchRow = NULL;
    void       * sOrgRow    = NULL;
    UShort       sMask      = 0;
    UInt         sGroupIndex;
    SInt         i;

    if ( sDataPlan->noStoreChild == ID_TRUE )
    {
        sDataPlan->isDataNone     = ID_FALSE;
        sDataPlan->noStoreChild   = ID_FALSE;
        sDataPlan->groupIndex     = sCodePlan->groupCount - 1;
        sDataPlan->compareResults = 0;
        sDataPlan->subStatus      = 0;
        sDataPlan->subIndex       = -1;

        initGroups( sCodePlan, sDataPlan );

        /* Store시에 쌓을 데이터가 없다고 설정되면 DATA_NONE으로 설정한다. */
        *aFlag = QMC_ROW_DATA_NONE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* partialCube가 아니라면 Store시에 구해진 총계를 먼저 올려둔다 */
    if ( (( sDataPlan->cubeGroups[0] & QMND_CUBE_GROUP_DONE_MASK )
         == QMND_CUBE_GROUP_DONE_FALSE ) && ( sCodePlan->partialCube == -1 ) )
    {
        for ( i = 0, sColumn = sDataPlan->plan.myTuple->columns;
              i < sDataPlan->plan.myTuple->columnCount;
              i++, sColumn++ )
        {
            sColumn->module->null( sColumn,
                                   (void *) mtc::value(
                                       sColumn,
                                       sDataPlan->plan.myTuple->row,
                                       MTD_OFFSET_USE ) );
        }
        sDataPlan->cubeGroups[0] |= QMND_CUBE_GROUP_DONE_TRUE;
        sDataPlan->plan.myTuple->modify++;
        *aFlag = QMC_ROW_DATA_EXIST;
        sDataPlan->doIt = qmnCUBE::doItFirst;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* 맨 마지막 그룹이 아니라면 뒤에서 부터 구해지지 않은 그룹을 찾는다 */
    if ( sDataPlan->groupIndex != sCodePlan->groupCount - 1 )
    {
        sGroupIndex = findRemainGroup( sDataPlan );

        if ( sGroupIndex == ID_UINT_MAX )
        {
            sDataPlan->isDataNone     = ID_FALSE;
            sDataPlan->noStoreChild   = ID_FALSE;
            sDataPlan->groupIndex     = sCodePlan->groupCount - 1;
            sDataPlan->compareResults = 0;
            sDataPlan->subStatus      = 0;
            sDataPlan->subIndex       = -1;

            initGroups( sCodePlan, sDataPlan );

            *aFlag = QMC_ROW_DATA_NONE;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            sDataPlan->groupIndex = sGroupIndex;
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 구해진 그룹은 한번에 최대 ( n + 1 )개의 그룹을 구할 수 있으므로 하위 그룹을 설정한다.
     * 설정된 그룹으로 sortNode를 구성하고 Sort를 수행한다.
     */
    setSubGroups( sCodePlan, sDataPlan );
    IDE_TEST( makeSortNodeAndSort( sCodePlan, sDataPlan )
              != IDE_SUCCESS );

    /* 마지막 노드일 경우 다음 그룹으로 설정한다 */
    if ( sDataPlan->groupIndex == sCodePlan->groupCount - 1 )
    {
        --sDataPlan->groupIndex;
    }
    else
    {
        /* Nothing to do */
    }

    sOrgRow = sSearchRow = sDataPlan->mtrTuple->row;
    IDE_TEST( qmcSortTemp::getFirstSequence( &sDataPlan->sortMTR,
                                             &sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->mtrTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;
    sDataPlan->mtrTuple->modify++;

    if ( sSearchRow == NULL )
    {
        sDataPlan->isDataNone     = ID_FALSE;
        sDataPlan->noStoreChild   = ID_FALSE;
        sDataPlan->groupIndex     = sCodePlan->groupCount - 1;
        sDataPlan->compareResults = 0;
        sDataPlan->subStatus      = 0;
        sDataPlan->subIndex       = -1;

        initGroups( sCodePlan, sDataPlan );

        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        IDE_TEST( setTupleMtrNode( aTemplate, sDataPlan )
                  != IDE_SUCCESS );
        IDE_TEST( copyMtrRowToMyRow( sDataPlan )
                  != IDE_SUCCESS );

        sDataPlan->doIt       = qmnCUBE::doItNext;
        sDataPlan->needCopy   = ID_FALSE;
        sDataPlan->subStatus  = 0;
        sDataPlan->subIndex   = 0;
        sDataPlan->isDataNone = ID_FALSE;

        for ( i = 0, sMask = 0x1; i < sCodePlan->cubeCount; i++, sMask <<= 1 )
        {
            sDataPlan->subStatus |= sMask;
        }

        for ( i = 0; i < (SInt)sDataPlan->subGroupCount; i ++ )
        {
            if ( sCodePlan->distNode != NULL )
            {
                IDE_TEST( clearDistNode( sCodePlan, sDataPlan, i )
                          != IDE_SUCCESS );
                IDE_TEST( setDistMtrColumns( aTemplate, sCodePlan, sDataPlan, i )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
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

        IDE_TEST( doItNext( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * doItNext
 *
 *   doItNext는 실제로 Rollup과 마찬가지로 최대 (n+1)개의 그룹에 대한 계산을 수행한다.
 */
IDE_RC qmnCUBE::doItNext( qcTemplate * aTemplate,
                          qmnPlan    * aPlan,
                          qmcRowFlag * aFlag )
{
    qmncCUBE   * sCodePlan   = ( qmncCUBE * )aPlan;
    qmndCUBE   * sDataPlan   = ( qmndCUBE * )( aTemplate->tmplate.data +
                                               aPlan->offset );
    UInt         sMask       = 0;
    idBool       sAllMatched = ID_FALSE;
    void       * sSearchRow  = NULL;
    void       * sOrgRow     = NULL;
    UInt         i;

    if ( sDataPlan->subIndex != 0 )
    {
        /* 이미 올려진 그룹에 대해 Aggregation을 초기화한다. */
        if ( sCodePlan->distNode != NULL )
        {
            IDE_TEST( clearDistNode( sCodePlan, sDataPlan, sDataPlan->subIndex - 1 )
                      != IDE_SUCCESS);
            IDE_TEST( setDistMtrColumns( aTemplate, sCodePlan, sDataPlan, sDataPlan->subIndex - 1 )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
        if ( sCodePlan->aggrNode != NULL )
        {
            IDE_TEST( initAggregation( aTemplate, sDataPlan, sDataPlan->subIndex - 1 )
                      != IDE_SUCCESS );
            IDE_TEST( execAggregation( aTemplate, sCodePlan, sDataPlan, sDataPlan->subIndex - 1)
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

    sMask = 0x1 << ( sDataPlan->subIndex );
    if ( ( sDataPlan->subIndex < ( SInt )sDataPlan->subGroupCount ) &&
         ( ( sDataPlan->subStatus & sMask ) == 0 ) )
    {
        setColumnNull( sCodePlan,
                       sDataPlan,
                       sDataPlan->subIndex );

        if ( sCodePlan->aggrNode != NULL )
        {
            sDataPlan->aggrTuple->row = sDataPlan->aggrRow[sDataPlan->subIndex];
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->subIndex++;
        if ( ( sDataPlan->isDataNone == ID_TRUE ) &&
             ( sDataPlan->subIndex >= ( SInt )sDataPlan->subGroupCount ) )
        {
            sDataPlan->doIt = qmnCUBE::doItFirst;
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->plan.myTuple->modify++;

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sDataPlan->needCopy == ID_TRUE )
    {
        IDE_TEST( copyMtrRowToMyRow( sDataPlan )
                      != IDE_SUCCESS );
        sDataPlan->needCopy = ID_FALSE;
        sDataPlan->plan.myTuple->modify++;
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
            if ( sCodePlan->distNode != NULL )
            {
                for ( i = 0; i < sDataPlan->subGroupCount; i ++ )
                {
                    IDE_TEST( setDistMtrColumns( aTemplate, sCodePlan, sDataPlan, i )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }

            /* myNode와 mtrNode의 Row를 서로 비교해 각 컬럼이 맞는지 비교한다 */
            IDE_TEST( compareRows( sDataPlan ) != IDE_SUCCESS );

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
                    /* Data가 없다면 모든 하위 그룹을 올려보내준다 */
                    sDataPlan->isDataNone = ID_TRUE;
                    sDataPlan->subIndex   = 1;
                    sDataPlan->subStatus  = 0;
                    setColumnNull( sCodePlan, sDataPlan, 0 );

                    if ( sCodePlan->aggrNode != NULL )
                    {
                        sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
                    }
                    else
                    {
                         /* Nothing to do */
                    }
                    if ( sDataPlan->subGroupCount <= 1 )
                    {
                        sDataPlan->doIt = qmnCUBE::doItFirst;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    sDataPlan->plan.myTuple->modify++;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* 첫 번째 그룹 에서 그룹이 아니므로 이에 대한 NULL을 수행하고 위로 올려보내준다 */
                sDataPlan->needCopy = ID_TRUE;
                sDataPlan->subIndex = 1;
                setColumnNull( sCodePlan, sDataPlan, 0 );

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
        /* Data가 없다면 모든 하위 그룹을 올려보내준다 */
        sDataPlan->isDataNone = ID_TRUE;
        sDataPlan->subIndex   = 1;
        sDataPlan->subStatus  = 0;
        setColumnNull( sCodePlan, sDataPlan, 0 );

        if ( sCodePlan->aggrNode != NULL )
        {
             sDataPlan->aggrTuple->row = sDataPlan->aggrRow[0];
        }
        else
        {
             /* Nothing to do */
        }
        if ( sDataPlan->subGroupCount <= 1 )
        {
            sDataPlan->doIt = qmnCUBE::doItFirst;
        }
        else
        {
            /* Nothing to do */
        }
        sDataPlan->plan.myTuple->modify++;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    IDE_TEST( finiAggregation( aTemplate, sDataPlan )
              != IDE_SUCCESS );

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
IDE_RC qmnCUBE::valueTempStore( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    qmncCUBE    * sCodePlan = ( qmncCUBE * )aPlan;
    qmndCUBE    * sDataPlan = ( qmndCUBE * )( aTemplate->tmplate.data + aPlan->offset );
    qmcRowFlag    sFlag     = QMC_ROW_INITIALIZE;
    qmdMtrNode  * sNode     = NULL;
    qmdMtrNode  * sMemNode  = NULL;
    qmdAggrNode * sAggrNode = NULL;
    void        * sRow      = NULL;
    void        * sSrcValue;
    void        * sDstValue;
    UInt          sSize;
    UInt          i         = 0;

    IDE_TEST( qmnCUBE::doItFirst( aTemplate, aPlan, &sFlag )
              != IDE_SUCCESS );

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
 * doItFirstValueTemp
 *
 *  데이블이 메모리이고 상위에서 Sort나 Window Sort와 같이 메모리 포인터를 쌓아서 작업을 하는
 *  경우에 기존 로직을 동작시켜 새로운 Memory Temp에 저장시키고 이때Temp에서 첫번 째 Row를 얻는다.
 */
IDE_RC qmnCUBE::doItFirstValueTemp( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan,
                                  qmcRowFlag * aFlag )
{
    qmndCUBE * sDataPlan = ( qmndCUBE * )( aTemplate->tmplate.data + aPlan->offset );
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
        
        sDataPlan->doIt = qmnCUBE::doItNextValueTemp;
        *aFlag = QMC_ROW_DATA_EXIST;
        sDataPlan->valueTempTuple->modify++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * doItNextValueTemp
 *
 *  데이블이 메모리이고 상위에서 Sort나 Window Sort와 같이 메모리 포인터를 쌓아서 작업을 하는
 *  경우에 기존 로직을 동작시켜 새로운 Memory Temp에 저장시키고 이때Temp에서 다 째 Row를 얻는다.
 */
IDE_RC qmnCUBE::doItNextValueTemp( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag )
{
    qmndCUBE * sDataPlan = ( qmndCUBE * )( aTemplate->tmplate.data + aPlan->offset );
    void     * sSearchRow;
    void     * sOrgRow;

    sOrgRow = sSearchRow = sDataPlan->valueTempTuple->row;
    IDE_TEST( qmcSortTemp::getNextSequence( sDataPlan->valueTempMTR,
                                            &sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->valueTempTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;

    if ( sSearchRow == NULL )
    {
        sDataPlan->doIt = qmnCUBE::doItFirstValueTemp;
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

IDE_RC qmnCUBE::printPlan( qcTemplate   * aTemplate,
                           qmnPlan      * aPlan,
                           ULong          aDepth,
                           iduVarString * aString,
                           qmnDisplay     aMode )
{
    qmncCUBE   * sCodePlan = ( qmncCUBE * )aPlan;
    qmndCUBE   * sDataPlan = ( qmndCUBE * )( aTemplate->tmplate.data +
                                           aPlan->offset );
    qmndCUBE   * sCacheDataPlan = NULL;
    idBool       sIsInit       = ID_FALSE;
    UShort       sSelfID;
    ULong        i;

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    if ( sCodePlan->valueTempNode != NULL )
    {
        sSelfID = sCodePlan->valueTempNode->dstNode->node.table;
    }
    else
    {
        sSelfID = sCodePlan->myNode->dstNode->node.table;
    }

    for ( i = 0; i < aDepth; i ++ )
    {
        iduVarStringAppend( aString, " " );
    }

    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( ( *sDataPlan->flag & QMND_CUBE_INIT_DONE_MASK )
             == QMND_CUBE_INIT_DONE_TRUE )
        {
            sIsInit = ID_TRUE;
            if ( sCodePlan->valueTempNode != NULL )
            {
                iduVarStringAppendFormat( aString,
                                          "GROUP-CUBE ( ACCESS: %"ID_UINT32_FMT,
                                          sDataPlan->valueTempTuple->modify );
            }
            else
            {
                iduVarStringAppendFormat( aString,
                                          "GROUP-CUBE ( ACCESS: %"ID_UINT32_FMT,
                                          sDataPlan->plan.myTuple->modify );
            }
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "GROUP-CUBE ( ACCESS: 0" );
        }
    }
    else
    {
        iduVarStringAppendFormat( aString,
                                  "GROUP-CUBE ( ACCESS: ??" );
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
        // PROJ-2462 Result Cache
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            sCacheDataPlan = (qmndCUBE *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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
