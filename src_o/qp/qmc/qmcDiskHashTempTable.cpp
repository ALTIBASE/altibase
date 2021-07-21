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
 * $Id: qmcDiskHashTempTable.cpp 85333 2019-04-26 02:34:41Z et16 $
 *
 * Description :
 *     Disk Hash Temp Table
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smErrorCode.h>
#include <qcg.h>
#include <qdParseTree.h>
#include <qdx.h>
#include <qcm.h>
#include <qcmTableSpace.h>
#include <qmcDiskHashTempTable.h>
#include <qdbCommon.h>

extern mtdModule mtdInteger;

IDE_RC
qmcDiskHash::init( qmcdDiskHashTemp * aTempTable,
                   qcTemplate       * aTemplate,
                   qmdMtrNode       * aRecordNode,
                   qmdMtrNode       * aHashNode,
                   qmdMtrNode       * aAggrNode,
                   UInt               aBucketCnt,
                   UInt               aMtrRowSize,
                   idBool             aDistinct )
{
/***********************************************************************
 *
 * Description :
 *    Disk Temp Table을 초기화한다.
 *
 * Implementation :
 *    - Disk Temp Table의 기본 자료 초기화
 *    - Record 구성 정보의 초기화
 *    - Hashing 구성 정보의 초기화
 *    - 검색을 위한 자료 구조의 초기화
 *    - Aggregation을 위한 자료 구조의 초기화
 *    - Temp Table의 생성
 *    - Index의 생성
 *    - Insert Cursor의 Open
 *
 ***********************************************************************/

    UInt         i;
    qmdMtrNode * sNode;
    qmdMtrNode * sAggrNode;
    
    // 적합성 검사
    IDE_DASSERT( aTempTable != NULL );
    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aRecordNode != NULL );
    IDE_DASSERT( aHashNode != NULL );

    //----------------------------------------------------------------
    // Disk Temp Table 멤버의 초기화
    //----------------------------------------------------------------

    aTempTable->flag = QMCD_DISK_HASH_TEMP_INITIALIZE;

    if ( aDistinct == ID_TRUE )
    {
        // Distinct Insertion인 경우
        aTempTable->flag &= ~QMCD_DISK_HASH_INSERT_DISTINCT_MASK;
        aTempTable->flag |= QMCD_DISK_HASH_INSERT_DISTINCT_TRUE;
    }
    else
    {
        // Non-Distinct Insertion인 경우
        aTempTable->flag &= ~QMCD_DISK_HASH_INSERT_DISTINCT_MASK;
        aTempTable->flag |= QMCD_DISK_HASH_INSERT_DISTINCT_FALSE;
    }

    aTempTable->mTemplate      = aTemplate;
    aTempTable->tableHandle    = NULL;

    tempSizeEstimate( aTempTable, aBucketCnt, aMtrRowSize );

    //-----------------------------------------
    // Record 구성 정보의 초기화
    //-----------------------------------------
    aTempTable->recordNode = aRecordNode;

    // [Record Column 개수의 결정]
    // Record Node의 개수로 판단하지 않고,
    // Tuple Set의 정보로부터 Column 개수를 판단한다.
    // 이는 하나의 Record Node가 여러 개의 Column으로 구성될 수 있기 때문이다.

    IDE_DASSERT( aRecordNode->dstTuple->columnCount > 0 );
    aTempTable->recordColumnCnt = aRecordNode->dstTuple->columnCount;

    aTempTable->insertColumn = NULL;
    aTempTable->insertValue = NULL;

    //---------------------------------------
    // Hashing 을 위한 자료 구조의 초기화
    //---------------------------------------
    aTempTable->hashNode = aHashNode;

    // [Hash Column 개수의 결정]
    // Record Column과 달리 Hash Node 정보로부터 Column 개수를 판단한다.
    // 하나의 Node가 여러 개의 Column으로 구성된다 하더라도
    // Hashing 대상이 되는 Column은 하나이기 때문이다.
    for ( i = 0, sNode = aTempTable->hashNode;
          sNode != NULL;
          sNode = sNode->next, i++ ) ;
    aTempTable->hashColumnCnt = i;

    aTempTable->hashKeyColumnList = NULL;
    aTempTable->hashKeyColumn = NULL;

    //---------------------------------------
    // 검색을 위한 자료 구조의 초기화
    //---------------------------------------
    aTempTable->searchCursor    = NULL;
    aTempTable->groupCursor     = NULL;
    aTempTable->groupFiniCursor = NULL;
    aTempTable->hitFlagCursor   = NULL;

    // To Fix PR-8289
    aTempTable->hashFilter = NULL;

    //---------------------------------------
    // Aggregation을 위한 자료 구조의 초기화
    //---------------------------------------

    // [Aggr Column 개수의 결정]
    // Aggr Column은 하나의 Node가 여러 개의 Column으로 구성되며
    // 모든 Column이 UPDATE 대상이 된다.  따라서, 모든 Column
    // 개수를 계산하여야 한다.
    aTempTable->aggrNode = aAggrNode;
    for ( i = 0, sAggrNode = aTempTable->aggrNode;
          sAggrNode != NULL;
          sAggrNode = sAggrNode->next )
    {
        // To Fix PR-7994, PR-8415
        // Source Node를 이용하여 Column Count를 획득

        // To Fix PR-12093
        // Destine Node를 사용하여 mtcColumn의 Count를 구하는 것이 원칙에 맞음
        // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
        //     - Memory 공간 낭비
        //     - offset 조정 오류 (PR-12093)
        i += ( sAggrNode->dstNode->node.module->lflag
               & MTC_NODE_COLUMN_COUNT_MASK);
    }
    aTempTable->aggrColumnCnt = i;
    aTempTable->aggrColumnList = NULL;
    aTempTable->aggrValue = NULL;

    //----------------------------------------------------------------
    // Disk Temp Table 의 생성
    //----------------------------------------------------------------
    IDE_TEST( createTempTable( aTempTable ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 *
 * Description : BUG-37778 disk hash temp table size estimate
 *                WAMap size + temp table size
 *
 *****************************************************************************/
void  qmcDiskHash::tempSizeEstimate( qmcdDiskHashTemp * aTempTable,
                                     UInt               aBucketCnt,
                                     UInt               aMtrRowSize )
{
    ULong        sTempSize;

    // BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
    if ( QCU_DISK_TEMP_SIZE_ESTIMATE == 1 )
    {
        // sdtUniqueHashModule::destroy( void * aHeader ) 를 참조함
        sTempSize = (ULong)(SMI_TR_HEADER_SIZE_FULL + aMtrRowSize) * aBucketCnt * 4;

        sTempSize = (ULong)(sTempSize
                            * 100.0 / (100.0 - ( smuProperty::getTempHashGroupRatio() +
                                                 smuProperty::getTempSubHashGroupRatio() )
                                       * 1.2));

        aTempTable->mTempTableSize = IDL_MIN( sTempSize, smuProperty::getHashAreaSize());
    }
    else
    {
        aTempTable->mTempTableSize = 0;
    }

}

IDE_RC
qmcDiskHash::clear( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    해당 Temp Table을 Truncate한다.
 *
 * Implementation :
 *    열려 있는 커서를 모두 닫고, Table을 Truncate한다.
 *
 ***********************************************************************/

    //-----------------------------
    // Temp Table을 Truncate한다.
    //-----------------------------
    IDE_TEST( smiHashTempTable::clear( aTempTable->tableHandle )
              != IDE_SUCCESS );

    aTempTable->searchCursor    = NULL;
    aTempTable->groupCursor     = NULL;
    aTempTable->groupFiniCursor = NULL;
    aTempTable->hitFlagCursor   = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-39644
    aTempTable->tableHandle = NULL;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::clearHitFlag( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table내에 존재하는 모든 Record의 Hit Flag을 초기화한다.
 *
 * Implementation :
 *    PROJ-2201 Innovation in sorting and hashing(temp)
 *    HitFlag는 각 Row에 설정된 HitSequence값으로, Header의 HitSequence값과
 *    같으면 Hit된 것으로 판단한다.
 *    따라서 Header의 HitSequence를 1 올려주면 I/O없이 초기화 가능하다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskHash::clearHitFlag"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskHash::clearHitFlag"));

    smiHashTempTable::clearHitFlag( aTempTable->tableHandle );

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC
qmcDiskHash::insert( qmcdDiskHashTemp * aTempTable,
                     UInt               aHashKey,
                     idBool           * aResult )
{
/***********************************************************************
 *
 * Description :
 *    Record를 Temp Table에 삽입한다.
 *
 * Implementation :
 *    - Cursor를 별도로 열지 않는다.  초기화 또는 Clear시 Insert Cursor는
 *      이미 열어 놓는다.
 *    - Row의 Hit Flag을 Clear한다.
 *    - smiValue를 삽입한다
 *    - Distinct Insertion의 경우, 삽입이 실패할 수 있다.
 *        - 적절한 Error Code를 검사하여 삽입이 실패함을 리턴한다.
 ***********************************************************************/

    // PROJ-1597 Temp record size 제약 제거
    // 매 row마다 smiValue를 재구성한다.
    IDE_TEST( makeInsertSmiValue(aTempTable) != IDE_SUCCESS );

    // Temp Table에 Record를 삽입한다.
    IDE_TEST( smiHashTempTable::insert( aTempTable->tableHandle,
                                        aTempTable->insertValue,
                                        aHashKey,
                                        &aTempTable->recordNode->dstTuple->rid,
                                        aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-39644
    aTempTable->tableHandle = NULL;

    return IDE_FAILURE;
}


IDE_RC
qmcDiskHash::updateAggr( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column들을  Update한다.
 *
 * Implementation :
 *    Group Cursor를 이용하여 Update하며,
 *    Group Cursor는 반드시 Open되어 있어야 한다.
 *    Update Value는 Group Cursor Open시 생성되었고,
 *    Value에 대한 pointer 역시 row pointer가 변하지 않기 때문에,
 *    변경시킬 필요가 없다.
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( aTempTable->groupCursor != NULL );

    // Aggregation Column을 위한 smiValue 구성
    IDE_TEST( makeAggrSmiValue( aTempTable ) != IDE_SUCCESS );

    // Aggregation Column을 Update한다.
    IDE_TEST( smiHashTempTable::update( aTempTable->groupCursor, 
                                        aTempTable->aggrValue )
         != IDE_SUCCESS )

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-39644
    aTempTable->tableHandle = NULL;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::updateFiniAggr( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column들을 최종 Update한다.
 *
 * Implementation :
 *    Search Cursor를 이용하여 Update하며,
 *    Search Cursor는 반드시 Open되어 있어야 한다.
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( aTempTable->groupFiniCursor != NULL );

    // Aggregation Column을 위한 smiValue 구성
    IDE_TEST( makeAggrSmiValue( aTempTable ) != IDE_SUCCESS );

    // Aggregation Column을 최종 Update한다.
    IDE_TEST( smiHashTempTable::update( aTempTable->groupFiniCursor, 
                                        aTempTable->aggrValue )
         != IDE_SUCCESS )

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-39644
    aTempTable->tableHandle = NULL;

    return IDE_FAILURE;
}


IDE_RC
qmcDiskHash::setHitFlag( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    현재 Cursor 위치의 Record의 Hit Flag을 Setting한다.
 *
 * Implementation :
 *    검색 후에만 호출된다.
 *    이미 검색된 Record에 대한 Hit Flag을 Setting하면 된다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskHash::setHitFlag"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskHash::setHitFlag"));

    smiHashTempTable::setHitFlag( aTempTable->searchCursor );

    return IDE_SUCCESS;
#undef IDE_FN
}

idBool qmcDiskHash::isHitFlagged( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    현재 Cursor 위치의 Record에 Hit Flag가 있는지 판단.
 *
 * Implementation :
 *    검색 후에만 호출된다.
 *    이미 검색된 Record에 대한 Hit Flag 여부를 검사하면 된다.
 *
 ***********************************************************************/

    return smiHashTempTable::isHitFlagged( aTempTable->searchCursor );
}

IDE_RC
qmcDiskHash::getFirstGroup( qmcdDiskHashTemp * aTempTable,
                            void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Group 순차 검색을 한다.
 *    Group Aggregation의 최종 처리를 위한 Cursor를 연다.
 *    Group에 대한 Sequetial 검색을 하게 되며,
 *    Aggregation Column의 최종 결과를 갱신하기 위해 사용한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if( aTempTable->groupFiniCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::allocHashCursor( aTempTable->tableHandle,
                                                     aTempTable->aggrColumnList,   // Update Column정보
                                                     SMI_HASH_CURSOR_FULL_SCAN,
                                                     &aTempTable->groupFiniCursor )
                  != IDE_SUCCESS );
    }

    // Group의 최종 Aggregation을 위한 Cursor를 연다.
    IDE_TEST( smiHashTempTable::openFullScanCursor( aTempTable->groupFiniCursor, 
                                                    SMI_TCFLAG_FORWARD | 
                                                    SMI_TCFLAG_FULLSCAN |
                                                    SMI_TCFLAG_IGNOREHIT,
                                                    (UChar**) aRow,
                                                    & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getNextGroup( qmcdDiskHashTemp * aTempTable,
                           void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Group 순차 검색을 한다.
 *    반드시 getFirstGroup() 이후에 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // Record를 얻어 온다.
    IDE_TEST( smiHashTempTable::fetchFullNext( aTempTable->groupFiniCursor,
                                               (UChar**) aRow,
                                               & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getFirstSequence( qmcdDiskHashTemp * aTempTable,
                               void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫 순차 검색을 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if( aTempTable->searchCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::allocHashCursor( aTempTable->tableHandle,
                                                     aTempTable->aggrColumnList,   // Update Column정보
                                                     SMI_HASH_CURSOR_FULL_SCAN,
                                                     &aTempTable->searchCursor )
                  != IDE_SUCCESS );
    }

    // 순차 검색 Cursor를 연다.
    IDE_TEST( smiHashTempTable::openFullScanCursor( aTempTable->searchCursor,
                                                    SMI_TCFLAG_FORWARD |
                                                    SMI_TCFLAG_FULLSCAN |
                                                    SMI_TCFLAG_IGNOREHIT,
                                                    (UChar**) aRow,
                                                    & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getNextSequence( qmcdDiskHashTemp * aTempTable,
                              void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색을 한다.
 *    반드시 getFirstSequence() 이후에 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // Record를 얻어 온다.
    IDE_TEST( smiHashTempTable::fetchFullNext( aTempTable->searchCursor,
                                               (UChar**) aRow,
                                               & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getFirstRange( qmcdDiskHashTemp * aTempTable,
                            UInt               aHash,
                            qtcNode          * aHashFilter,
                            void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Range 검색을 한다.
 *
 * Implementation :
 *    다음과 같은 절차에 의하여 Range검색을 수행한다.
 *
 *    - 주어진 Hash Value를 이용하여 Key Range를 생성한다.
 *    - Range Cursor를 Open 한다.
 *    - Cursor를 이용하여 조건에 맞는 Row를 가져온다.
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( aHashFilter != NULL );

    if( aTempTable->searchCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::allocHashCursor( aTempTable->tableHandle,
                                                     aTempTable->aggrColumnList,   // Update Column정보
                                                     SMI_HASH_CURSOR_HASH_SCAN,
                                                     &aTempTable->searchCursor )
            != IDE_SUCCESS );
    }

    if( smiHashTempTable::checkHashSlot( aTempTable->searchCursor,
                                         aHash ) == ID_TRUE )
    {
        // Filter 생성
        aTempTable->hashFilter = aHashFilter;
        IDE_TEST( makeHashFilterCallBack( aTempTable ) != IDE_SUCCESS );

        // Range Cursor를 Open한다.
        IDE_TEST( smiHashTempTable::openHashCursor( aTempTable->searchCursor,
                                                    SMI_TCFLAG_FORWARD |
                                                    SMI_TCFLAG_HASHSCAN |
                                                    SMI_TCFLAG_IGNOREHIT,
                                                    &aTempTable->filterCallBack,  // Filter
                                                    (UChar**) aRow,
                                                    & aTempTable->recordNode->dstTuple->rid )
                  != IDE_SUCCESS );

        // To Fix PR-8645
        // Hash Key 값을 검사하기 위한 Filter에서 Access값을 증가하였고,
        // 이 후 Execution Node에서 이를 다시 증가시키게 됨.
        // 따라서, 마지막 한번의 검사에 대해서는 access회수를 감소시켜야 함
        if ( *aRow != NULL )
        {
            aTempTable->recordNode->dstTuple->modify--;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getNextRange( qmcdDiskHashTemp * aTempTable,
                           void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색을 한다.
 *    반드시 getFirstRange() 이후에 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // Cursor를 이용하여 조건에 맞는 Row를 가져온다.
    IDE_TEST( smiHashTempTable::fetchHashNext( aTempTable->searchCursor,
                                               (UChar**) aRow,
                                               & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    // To Fix PR-8645
    // Hash Key 값을 검사하기 위한 Filter에서 Access값을 증가하였고,
    // 이 후 Execution Node에서 이를 다시 증가시키게 됨.
    // 따라서, 마지막 한번의 검사에 대해서는 access회수를 감소시켜야 함
    if ( *aRow != NULL )
    {
        aTempTable->recordNode->dstTuple->modify--;
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
qmcDiskHash::getFirstHit( qmcdDiskHashTemp * aTempTable,
                          void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    최초 Hit된 Record를 검색한다.
 *
 * Implementation :
 *    다음과 같은 절차에 의하여 Hit 검색을 수행한다.
 *        - Hit Filter를 생성한다.
 *        - Hit을 위한 Cursor를 연다.
 *        - 조건을 만족하는 Record를 읽어온다.
 *
 ***********************************************************************/

    if( aTempTable->hitFlagCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::allocHashCursor( aTempTable->tableHandle,
                                                     aTempTable->aggrColumnList,   // Update Column정보
                                                     SMI_HASH_CURSOR_FULL_SCAN,
                                                     &aTempTable->hitFlagCursor )
                  != IDE_SUCCESS );
    }

    /* HitFlag가 설정된 Row만 가져온다. */
    IDE_TEST( smiHashTempTable::openFullScanCursor( aTempTable->hitFlagCursor,
                                                    SMI_TCFLAG_FORWARD |
                                                    SMI_TCFLAG_FULLSCAN |
                                                    SMI_TCFLAG_HIT,
                                                    (UChar**) aRow,
                                                    & aTempTable->recordNode->dstTuple->rid)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getNextHit( qmcdDiskHashTemp * aTempTable,
                         void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Hit된 Record를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // Cursor를 이용하여 조건에 맞는 Row를 가져온다.
    IDE_TEST( smiHashTempTable::fetchFullNext( aTempTable->hitFlagCursor,
                                               (UChar**) aRow,
                                               & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcDiskHash::getFirstNonHit( qmcdDiskHashTemp * aTempTable,
                             void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    최초 Hit되지 않은 Record를 검색한다.
 *
 * Implementation :
 *    다음과 같은 절차에 의하여 NonHiit 검색을 수행한다.
 *        - NonHit Filter를 생성한다.
 *        - NonHit을 위한 Cursor를 연다.
 *        - 조건을 만족하는 Record를 읽어온다.
 *
 ***********************************************************************/

    if( aTempTable->hitFlagCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::allocHashCursor( aTempTable->tableHandle,
                                                     aTempTable->aggrColumnList,   // Update Column정보
                                                     SMI_HASH_CURSOR_FULL_SCAN,
                                                     &aTempTable->hitFlagCursor )
                  != IDE_SUCCESS );
    }

    /* HitFlag가 설정 안된 Row만 가져온다. */
    IDE_TEST( smiHashTempTable::openFullScanCursor( aTempTable->hitFlagCursor,
                                                    SMI_TCFLAG_FORWARD |
                                                    SMI_TCFLAG_FULLSCAN |
                                                    SMI_TCFLAG_NOHIT,
                                                    (UChar**) aRow,
                                                    & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getNextNonHit( qmcdDiskHashTemp * aTempTable,
                            void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Hit되지 않은 Record를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // Cursor를 이용하여 조건에 맞는 Row를 가져온다.
    IDE_TEST( smiHashTempTable::fetchFullNext( aTempTable->hitFlagCursor,
                                               (UChar**) aRow,
                                               & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getSameRowAndNonHit( qmcdDiskHashTemp * aTempTable,
                                  UInt               aHash,
                                  void             * aRow,
                                  void            ** aResultRow )
{
/***********************************************************************
 *
 * Description :
 *    Set Intersection에서 검색을 위해 사용하며,
 *    주어진 Row와 동일한 Hash Column의 값을 갖고,
 *    Hit되지 않는 Row를 찾기 위해 사용한다.
 *
 * Implementation :
 *    다음과 같은 절차로 수행된다.
 *        - Range 생성
 *        - Filter 생성(동일 Hash & NonHit)
 *        - 원하는 Record 검색 후 Hit Flag 갱신
 *
 ***********************************************************************/

    //-----------------------------------------
    // Hash Index를 위한 Range 설정
    //-----------------------------------------

    //-----------------------------------------
    // Filter 설정
    //     - Non-Hit 여부 검사
    //     - 동일 Data 여부 검사.
    // 이를 처리하기 위한 함수를 따로 구성할 수 있으나,
    // CallBack Function 만 다를 뿐 모든 조건이 동일하므로,
    // 구현 부하를 줄이기 위해 아래와 같이 처리한다.
    //-----------------------------------------

    if( aTempTable->searchCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::allocHashCursor( aTempTable->tableHandle,
                                                     aTempTable->aggrColumnList,   // Update Column정보
                                                     SMI_HASH_CURSOR_HASH_SCAN,
                                                     &aTempTable->searchCursor )
            != IDE_SUCCESS );
    }

    if( smiHashTempTable::checkHashSlot( aTempTable->searchCursor,
                                         aHash ) == ID_TRUE )
    {
        // 동일 여부 검사를 위한 Filter 설정
        IDE_TEST( makeTwoRowHashFilterCallBack( aTempTable, aRow )
                  != IDE_SUCCESS );

        /* HitFlag가 설정 안된 Row만 가져온다. */
        IDE_TEST( smiHashTempTable::openHashCursor( aTempTable->searchCursor,
                                                    SMI_TCFLAG_FORWARD | 
                                                    SMI_TCFLAG_HASHSCAN |
                                                    SMI_TCFLAG_NOHIT,
                                                    &aTempTable->filterCallBack,  // Filter
                                                    (UChar**) aResultRow,
                                                    & aTempTable->recordNode->dstTuple->rid )
                  != IDE_SUCCESS );
    }
    else
    {
        *aResultRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getNullRow( qmcdDiskHashTemp * aTempTable,
                         void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    NULL ROW를 획득한다.
 *
 * Implementation :
 *    Null Row를 획득한 적이 없다면, Null Row를 획득
 *    저장된 해당 영역에 Null Row를 복사한다.
 *    SM의 Null Row를 저장하는 이유는 빈번한 호출로 인한 Disk I/O를
 *    방지하기 위함이다.
 *
 ***********************************************************************/

    IDE_TEST( smiTempTable::getNullRow( aTempTable->tableHandle,
                                        (UChar**)aRow )
              != IDE_SUCCESS );

    SMI_MAKE_VIRTUAL_NULL_GRID( aTempTable->recordNode->dstTuple->rid );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getSameGroup( qmcdDiskHashTemp * aTempTable,
                           UInt               aHash,
                           void             * aRow,
                           void            ** aResultRow )
{
/***********************************************************************
 *
 * Description :
 *    Group Aggregation에서 사용되며, 현재 Row와 동일한
 *    Row가 존재하는 지를 검사하고, 존재한다면 이를 Return하고,
 *    존재하지 않는다면 Record를 삽입한다.
 *    기존 A3에서의 Group Aggregation 절차가 다음과 같은데,
 *    이는 Disk Temp Table을 사용할 경우 치명적인 성능 악확를
 *    가져 올 수 있기 때문이다.
 *         A3  : Insert --> 성공 --> Aggregation --> Update
 *                      --> 실패 --> 검색 --> Aggregation --> Update
 *         A4  : Aggregation --> 검색 --> 성공 --> Aggregation --> Update
 *                                    --> 실패 --> Insert
 *
 *    To Fix PR-8213
 *        - 그러나, 위와 같은 절차는 Memory Table에서 심각한 성능
 *          저하를 가져온다.
 *          따라서, 다음과 같은 방식으로 수정한다.
 *
 *    New A4 :
 *        동일 Group 검색 -->
 *        --> 성공 --> Aggregation --> Update
 *        --> 실패(Memory의 경우 삽입됨) --> Init, Aggregation
 *            --> Add New Group(Memory의 경우 별도의 작업을 수행하지 않음)
 *
 * Implementation :
 *
 *    다음과 같은 절차로 검색한다.
 *        - 검색을 위한 Range와 Filter의 생성
 *        - Group Cursor를 열고 조건에 맞는 Record 검색
 *            - 존재한다면, 이를 Return
 *            - 존재하지 않는다면, NULL을 Return
 *
 ***********************************************************************/

    // 적합성 검사
    // Aggregation이 있는 경우에만 사용하여야 한다.
    IDE_DASSERT( aTempTable->aggrNode != NULL );

    if( aTempTable->groupCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        // Group Cursor 는 여기에서만 open
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::allocHashCursor( aTempTable->tableHandle,
                                                     aTempTable->aggrColumnList,   // Update Column정보
                                                     SMI_HASH_CURSOR_HASH_UPDATE,
                                                     &aTempTable->groupCursor )
                  != IDE_SUCCESS );
    }

    if( smiHashTempTable::checkUpdateHashSlot( aTempTable->groupCursor,
                                               aHash ) == ID_TRUE )
    {
        // BUG-10662
        // qmnGRAG::groupAggregation()함수내에서 처리하고 있음
        // Record를 읽을 공간 설정
        // sOrgRow = aTempTable->recordNode->dstTuple->row;

        //-----------------------------------------
        // Hash Index를 위한 두 Row간의 Filter 설정
        //-----------------------------------------
        IDE_TEST( makeTwoRowHashFilterCallBack( aTempTable, aRow )
                  != IDE_SUCCESS );

        //-----------------------------------------
        // 동일 Group을 찾기 위한 Cursor를 연다.
        //-----------------------------------------
        IDE_TEST( smiHashTempTable::openUpdateHashCursor( aTempTable->groupCursor,
                                                          SMI_TCFLAG_FORWARD |
                                                          SMI_TCFLAG_HASHSCAN |
                                                          SMI_TCFLAG_IGNOREHIT,
                                                          &aTempTable->filterCallBack,  // Filter
                                                          (UChar**) aResultRow,
                                                          & aTempTable->recordNode->dstTuple->rid )
                  != IDE_SUCCESS );
    }
    else
    {
        *aResultRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::getDisplayInfo( qmcdDiskHashTemp * aTempTable,
                             ULong            * aPageCount,
                             SLong            * aRecordCount )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table이 관리하고 있는 Data에 대한 통계 정보 추출
 *
 * Implementation :
 *
 ***********************************************************************/

    // BUG-39644
    // temp table create 가 실패해도
    // MM 에서 plan text 를 생성하기 위해서 호출할수 있다.
    if ( aTempTable->tableHandle != NULL )
    {
        smiHashTempTable::getDisplayInfo(
                      aTempTable->tableHandle,
                      aPageCount,
                      aRecordCount );
    }
    else
    {
        *aPageCount   = 0;
        *aRecordCount = 0;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmcDiskHash::createTempTable( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Disk Temp Table을 생성한다.
 *
 * Implementation :
 *    1.  Disk Temp Table 생성을 위한 정보 구성
 *        - Hit Flag을 위한 자료 구조 초기화
 *        - Insert Column List 정보의 구성
 *        - Aggregation Column List 정보의 구성
 *        - NULL ROW의 생성
 *    2.  Disk Temp Table 생성
 *    3.  Temp Table Manager 에 Table Handle 등록
 *
 ***********************************************************************/

    UInt                   sFlag;

    //-------------------------------------------------
    // 1.  Disk Temp Table 생성을 위한 정보 구성
    //-------------------------------------------------

    // Insert Column List 정보의 구성
    IDE_TEST( setInsertColumnList( aTempTable ) != IDE_SUCCESS );

    // Aggregation Column List 정보의 구성
    if ( aTempTable->aggrNode != NULL )
    {
        // Aggregation 정보가 있다면 Column List정보를 구성
        IDE_TEST( setAggrColumnList( aTempTable ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //-----------------------------------------
    // Key 내에서의 Index Column 정보의 구성
    // createTempTable하기 위해서는 key column 정보를 먼저 구축해야 한다.
    //-----------------------------------------
    IDE_TEST( makeIndexColumnInfoInKEY( aTempTable ) != IDE_SUCCESS );

    //-------------------------------------------------
    // 2.  Disk Temp Table의 생성
    //-------------------------------------------------
    // Uniqueness의 결정
    if ( (aTempTable->flag & QMCD_DISK_HASH_INSERT_DISTINCT_MASK)
         == QMCD_DISK_HASH_INSERT_DISTINCT_TRUE )
    {
        // UNIQUE INDEX 생성
        sFlag = SMI_TTFLAG_UNIQUE;
    }
    else
    {
        sFlag = SMI_TTFLAG_NONE;
    }

    IDE_TEST( smiHashTempTable::create( 
                aTempTable->mTemplate->stmt->mStatistics,
                QCG_GET_SESSION_TEMPSPACE_ID( aTempTable->mTemplate->stmt ),
                aTempTable->mTempTableSize, /* aWorkAreaSize */
                QC_SMI_STMT( aTempTable->mTemplate->stmt ), // smi statement
                sFlag,
                aTempTable->insertColumn,        // Table의 Column 구성
                aTempTable->hashKeyColumnList,   // key column list
                (const void**)& aTempTable->tableHandle ) // Table 핸들
        != IDE_SUCCESS );

    aTempTable->recordNode->dstTuple->tableHandle = aTempTable->tableHandle;

    //-------------------------------------------------
    // 3.  Table Handle을 Temp Table Manager에 등록
    //-------------------------------------------------
    /* BUG-38290 
     * Temp table manager 의 addTempTable 함수는 동시에 호출될 수 없으므로
     * mutex 를 통한 동시성 제어를 하지 않는다.
     */
    IDE_TEST(
        qmcTempTableMgr::addTempTable( aTempTable->mTemplate->stmt->qmxMem,
                                       aTempTable->mTemplate->tempTableMgr,
                                       aTempTable->tableHandle )
        != IDE_SUCCESS );

    //-------------------------------------------------
    // 4.  삽입할 Value를 넣을 공간을 할당받음
    //-------------------------------------------------
    IDE_ERROR( aTempTable->insertValue == NULL );

    // Insert Value 정보를 위한 공간을 할당한다.
    IDU_LIMITPOINT("qmcDiskHash::openInsertCursor::malloc");
    IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
            ID_SIZEOF(smiValue) * (aTempTable->recordColumnCnt),
            (void**) & aTempTable->insertValue ) != IDE_SUCCESS);


    if( aTempTable->aggrColumnCnt != 0 )
    {
        IDU_LIMITPOINT("qmcDiskHash::openGroupCursor::malloc");
        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                ID_SIZEOF(smiValue) * aTempTable->aggrColumnCnt,
                (void**) & aTempTable->aggrValue ) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-39644
    aTempTable->tableHandle = NULL;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::setInsertColumnList( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *
 *    Table의 Column List정보를 구성한다.
 *
 * Implementation :
 *    - Column List를 위한 공간 할당
 *    - Column List에 Column 정보의 연결
 *        - Hit Flag을 위한 정보 연결
 *        - 각 Column 정보의 연결
 *
 ***********************************************************************/

    UInt        i;
    mtcColumn * sColumn;

    // Column 개수만큼 Memory 할당
    IDU_FIT_POINT( "qmcDiskHash::setInsertColumnList::alloc::insertColumn",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
            ID_SIZEOF(smiColumnList) * aTempTable->recordColumnCnt,
            (void**) & aTempTable->insertColumn ) != IDE_SUCCESS);

    //-------------------------------------------
    // 최초 Column은 Hit Flag을 연결하고,
    // 이후의 Column은 Tuple Set 정보를 이용한다.
    //-------------------------------------------

    // To Fix PR-7995
    // Column ID를 초기화해주어야 함
    // 각 Column들의 Column 정보 연결
    for ( i = 0, sColumn = aTempTable->recordNode->dstTuple->columns;
          i < aTempTable->recordColumnCnt;
          i++ )
    {
        // 이전 Column List의 연결 정보 구성
        if( i < aTempTable->recordColumnCnt - 1 )
        {
            aTempTable->insertColumn[i].next = & aTempTable->insertColumn[i+1];
        }
        else
        {
            /* 마지막 칼럼이면, Null을 연결 */
            aTempTable->insertColumn[i].next = NULL;
        }

        // To Fix PR-7995
        // Column ID를 초기화해주어야 함
        // 현재 Column List의 Column 정보 연결
        sColumn[i].column.id = i;
        aTempTable->insertColumn[i].column = & sColumn[i].column;
    }
    // To Fix PR-7995
    // Fix UMR
    IDE_ERROR( aTempTable->insertColumn[i-1].next == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::setAggrColumnList( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *
 *    Aggregation 정보에 대한 UPDATE를 수행하기 위해
 *    Aggreagtion Column List정보를 구성한다.
 *
 * Implementation :
 *    - Column List를 위한 공간 할당
 *    - Aggregation Node를 순회하며, 하나의 Aggregation Node가 가진
 *      Column개수만큼 Column List를 구성한다.
 *
 ***********************************************************************/

    UInt i;
    UInt j;
    UInt sColumnCnt;
    qmdMtrNode * sNode;

    // 적합성 검사.
    IDE_DASSERT ( aTempTable->aggrColumnCnt != 0 );

    // Column 개수만큼 Memory 할당
    IDU_FIT_POINT( "qmcDiskHash::setAggrColumnList::alloc::aggrColumnList",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
            ID_SIZEOF(smiColumnList) * aTempTable->aggrColumnCnt,
            (void**) & aTempTable->aggrColumnList ) != IDE_SUCCESS);

    i = 0;
    for ( sNode = aTempTable->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        // To Fix PR-7994, PR-8415
        // Source Node를 이용하여 Column Count를 획득

        // To Fix PR-12093
        // Destine Node를 사용하여 mtcColumn의 Count를 구하는 것이 원칙에 맞음
        // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
        //     - Memory 공간 낭비
        //     - offset 조정 오류 (PR-12093)

        sColumnCnt =
            sNode->dstNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK;

        // Column 개수만큼 Column List 정보를 연결(8365)
        for ( j = 0; j < sColumnCnt; j++, i++ )
        {
            // Column 정보의 연결
            aTempTable->aggrColumnList[i].column =
                & sNode->dstColumn[j].column;

            // ABR을 막기 위한 검사
            if ( (i + 1) < aTempTable->aggrColumnCnt )
            {
                // Column List간의 연결 관계
                aTempTable->aggrColumnList[i].next =
                    & aTempTable->aggrColumnList[i+1];
            }
            else
            {
                aTempTable->aggrColumnList[i].next = NULL;
            }
        }
    }

    // 적합성 검사.
    // Aggregation Column개수만큼 Column List 정보가 Setting되어 있어야 함
    IDE_DASSERT( i == aTempTable->aggrColumnCnt );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::makeIndexColumnInfoInKEY( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Index생성 시 Record내에서 Index Key로 포함될 컬럼의 정보를
 *    구성한다.  Disk Index의 경우 Key 값을 별도로 저장하기 때문에,
 *    Index 생성 시, Record내에서의 Index Key Column의 정보와
 *    Key내에서의 Index Key Column정보를 구분하여 처리하여야 한다.
 *
 *    즉, Index Column의 경우 Record내에서의 offset과 Key내에서의
 *    offset이 다르게 된다.
 *
 * Implementation :
 *
 *    - Key Column을 위한 mtcColumn 정보 구축
 *    - Key Column을 위한 smiColumnList 구축
 *
 ***********************************************************************/

    UInt            i;
    UInt            sOffset;
    qmdMtrNode    * sNode;

    //------------------------------------------------------------
    // Key 내에서의 Index 대상 Column의 정보 구성
    // 해당 Column이 Key값으로 저장되었을 때,
    // Record내에서의 offset 정보와 달리 Key 내에서의 offset정보로
    // 변경되어야 한다.
    //------------------------------------------------------------

    // Key Column 정보를 위한 공간 할당
    IDU_FIT_POINT( "qmcDiskHash::makeIndexColumnInfoInKEY::alloc::hashKeyColumn",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                        ID_SIZEOF(mtcColumn) * aTempTable->hashColumnCnt,
                        (void**) & aTempTable->hashKeyColumn ) != IDE_SUCCESS);

    //---------------------------------------
    // Key Column의 mtcColumn 정보 생성
    //---------------------------------------

    sOffset = 0;
    for ( i = 0, sNode = aTempTable->hashNode;
          i < aTempTable->hashColumnCnt;
          i++, sNode = sNode->next )
    {
        // Record내의 Column 정보를 복사
        idlOS::memcpy( & aTempTable->hashKeyColumn[i],
                       sNode->dstColumn,
                       ID_SIZEOF(mtcColumn) );

        // fix BUG-8763
        // Index Column의 경우 다음 Flag을 표시하여야 함.
        aTempTable->hashKeyColumn[i].column.flag &= ~SMI_COLUMN_USAGE_MASK;
        aTempTable->hashKeyColumn[i].column.flag |= SMI_COLUMN_USAGE_INDEX;

        // Offset 재조정
        if ( (aTempTable->hashKeyColumn[i].column.flag
              & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
        {
            // Fixed Column 일 경우
            // Data Type에 맞는 align 조정
            sOffset =
                idlOS::align( sOffset,
                              aTempTable->hashKeyColumn[i].module->align );
            aTempTable->hashKeyColumn[i].column.offset = sOffset;
            aTempTable->hashKeyColumn[i].column.value = NULL;
            sOffset += aTempTable->hashKeyColumn[i].column.size;
        }
        else
        {
            //------------------------------------------
            // Variable Column일 경우
            // Variable Column Header 크기가 저장됨
            // 이에 맞는 align을 수행해야 함.
            //------------------------------------------
            sOffset = idlOS::align( sOffset, 8 );
            aTempTable->hashKeyColumn[i].column.offset = sOffset;
            aTempTable->hashKeyColumn[i].column.value = NULL;

// PROJ_1705_PEH_TODO            
            // Fixed 영역 내에는 Header만 저장된다.
            sOffset += smiGetVariableColumnSize( SMI_TABLE_DISK );
        }
    }

    //---------------------------------------
    // Key Column을 위한 smiColumnList 구축
    // 상위에서 구축한 indexKeyColumn정보를 이용하여 구축한다.
    //---------------------------------------

    // Key Column List를 위한 공간 할당
    IDU_FIT_POINT( "qmcDiskHash::makeIndexColumnInfoInKEY::alloc::hashKeyColumnList",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                        ID_SIZEOF(smiColumnList) * aTempTable->hashColumnCnt,
                        (void**) & aTempTable->hashKeyColumnList ) != IDE_SUCCESS);

    // Key Column List의 정보 구성
    for ( i = 0; i < aTempTable->hashColumnCnt; i++ )
    {
        // 상위에서 구성한 Column 정보를 이용
        aTempTable->hashKeyColumnList[i].column =
            & aTempTable->hashKeyColumn[i].column;

        // Array Bound Read를 방지하기 위한 검사.
        if ( (i + 1) < aTempTable->hashColumnCnt )
        {
            aTempTable->hashKeyColumnList[i].next =
                & aTempTable->hashKeyColumnList[i+1];
        }
        else
        {
            aTempTable->hashKeyColumnList[i].next = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcDiskHash::makeInsertSmiValue( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Insert를 위한 Smi Value 구성 정보를 생성한다.
 *
 * Implementation :
 *    Disk Temp Table의 Record를 위하여 생성된 공간에서
 *    각 Column의 Value 정보를 구성한다.
 *    최초 한 번만 구성하며, 이는 Temp Table이 사라질 때까지
 *    이미 할당된 Record 공간은 절대 변하지 않기 때문이다.
 *
 ***********************************************************************/

    UInt        i;
    mtcColumn  *sColumn;

    for ( i = 0; i < aTempTable->recordColumnCnt; i++ )
    {
        // PROJ-1597 Temp record size 제약 제거
        // smiValue의 length, value를 storing format으로 바꾼다.

        sColumn = &aTempTable->recordNode->dstTuple->columns[i];

        aTempTable->insertValue[i].value = 
            (SChar*)aTempTable->recordNode->dstTuple->row 
            + sColumn->column.offset;

        aTempTable->insertValue[i].length = 
            sColumn->module->actualSize( sColumn,
                                         (SChar*)aTempTable->recordNode->dstTuple->row
                                         + sColumn->column.offset );

    }

    return IDE_SUCCESS;
}

IDE_RC
qmcDiskHash::makeAggrSmiValue( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation의 UPDATE를 위한 Smi Value 구성 정보를 생성한다.
 *
 * Implementation :
 *    Disk Temp Table의 Record를 위하여 생성된 공간에서
 *    각 Column의 Value 정보를 구성한다.
 *    최초 한 번만 구성하며, 이는 Temp Table이 사라질 때까지
 *    이미 할당된 Record 공간은 절대 변하지 않기 때문이다.
 *    구성 시 Aggregation Node로부터 접근하여 하나의 노드당
 *    Column개수만큼 Value정보를 설정하여야 한다.
 *
 ***********************************************************************/

    UInt         i;
    UInt         j;
    UInt         sColumnCnt;
    qmdMtrNode * sNode;
    mtcColumn  * sColumn;

    // 적합성 검사
    IDE_DASSERT( aTempTable->recordNode->dstTuple->row != NULL );

    i = 0;
    for ( sNode = aTempTable->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        // To Fix PR-7994, PR-8415
        // Source Node를 이용하여 Column Count를 획득

        // To Fix PR-12093

        // Destine Node를 사용하여 mtcColumn의 Count를
        // 구하는 것이 원칙에 맞음
        // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
        //     - Memory 공간 낭비
        //     - offset 조정 오류 (PR-12093)
        sColumnCnt =
            sNode->dstNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK;

        // Column 개수만큼 Value 정보를 연결
        for ( j = 0; j < sColumnCnt; j++, i++ )
        {
            sColumn = &sNode->dstColumn[j];

            aTempTable->aggrValue[i].value = 
                (SChar*)sNode->dstTuple->row
                + sColumn->column.offset;

            aTempTable->aggrValue[i].length = sColumn->module->actualSize( 
                sColumn,
                (SChar*)sNode->dstTuple->row
                + sColumn->column.offset );
        }
    }

    // 적합성 검사.
    // Aggregation Column개수만큼 Value정보가 Setting되어 있어야 함
    IDE_DASSERT( i == aTempTable->aggrColumnCnt );

    return IDE_SUCCESS;
}

IDE_RC
qmcDiskHash::makeHashFilterCallBack( qmcdDiskHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    주어진 Filter를 만족하는 지를 검사하기 위한 Filter CallBack생성
 *    Hash Join등과 같이 명백히 Filter가 주어지는 경우에 사용된다.
 *
 * Implementation :
 *    주어진 Filter를 이용하여 Filter CallBack을 생성한다.
 *    절차는 일반 Filter의 생성 절차와 동일한 방법으로 구성한다.
 *        - Call Back Data의 구성
 *        - CallBack의 구성
 *
 ***********************************************************************/

    //적합성 검사
    IDE_DASSERT( aTempTable->hashFilter != NULL);

    //-------------------------------
    // CallBack Data 의 구성
    //-------------------------------

    qtc::setSmiCallBack( & aTempTable->filterCallBackData,
                         aTempTable->hashFilter,
                         aTempTable->mTemplate,
                         aTempTable->recordNode->dstNode->node.table );

    //-------------------------------
    // CallBack 의 구성
    //-------------------------------

    // CallBack 함수의 연결
    aTempTable->filterCallBack.callback = qtc::smiCallBack;

    // CallBack Data의 Setting
    aTempTable->filterCallBack.data = & aTempTable->filterCallBackData;

    return IDE_SUCCESS;
}

IDE_RC
qmcDiskHash::makeTwoRowHashFilterCallBack( qmcdDiskHashTemp * aTempTable,
                                           void             * aRow )
{
/***********************************************************************
 *
 * Description :
 *    지정된 Record와 동일한 hash value를 갖는 Record를 찾기 위한
 *    Filter CallBack을 생성한다.
 *
 * Implementation :
 *    Row간의 동일한 Hash Column에 대한 비교를 하기 위하여,
 *    별다른 qtcNode 형태의 filter가 존재하지 않으므로,
 *    Hashing Column의 구성 정보를 이용하여 Filtering 해야 한다.
 *
 ***********************************************************************/

    //-------------------------------
    // CallBack Data 의 구성
    //-------------------------------

    // CallBack Data를 위한 기본 정보를 설정한다.
    // 설정된 CallBack Data의 정보 중 유효한 것은 data->table뿐이다.
    qtc::setSmiCallBack( & aTempTable->filterCallBackData,
                         aTempTable->recordNode->dstNode,
                         aTempTable->mTemplate,
                         aTempTable->recordNode->dstNode->node.table );

    // 비교 대상 Row를 저장
    aTempTable->filterCallBackData.calculateInfo = aRow;

    // Hashing Column 정보를 강제로 Type Casting하여 저장
    aTempTable->filterCallBackData.node = (mtcNode*) aTempTable->hashNode;

    //-------------------------------
    // CallBack 의 구성
    //-------------------------------

    // CallBack 함수의 연결
    aTempTable->filterCallBack.callback =
        qmcDiskHash::hashTwoRowRangeFilter;

    // CallBack Data의 Setting
    aTempTable->filterCallBack.data = & aTempTable->filterCallBackData;

    return IDE_SUCCESS;
}

IDE_RC
qmcDiskHash::hashTwoRowRangeFilter( idBool         * aResult,
                                    const void     * aRow,
                                    void           *, /* aDirectKey */
                                    UInt            , /* aDirectKeyPartialSize */
                                    const scGRID,
                                    void           * aData )
{
/***********************************************************************
 *
 * Description :
 *    현재 Record와 동일한 Temp Table내의 Record를 찾기 위한 Filter
 *
 * Implementation :
 *    강제로 Setting하여 넘긴 CallBackData를 이용하여
 *    Filtering을 수행함.
 *
 ***********************************************************************/

    qtcSmiCallBackData * sData;
    qmdMtrNode         * sNode;
    SInt                 sResult;
    void               * sRow;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    sResult = -1;
    sData = (qtcSmiCallBackData *) aData;

    //-----------------------------------------------------
    // 생성하여 넘긴 CallBackData로부터 필요한 정보를 추출
    //     - sData->caculateInfo : 검색할 Row정보를 기록함
    //     - sData->node :
    //        qmdMtrNode의 hash node를 강제로 type casting하여
    //        저장함.
    //-----------------------------------------------------

    sRow = sData->calculateInfo;
    sNode = (qmdMtrNode*) sData->node;

    for(  ; sNode != NULL; sNode = sNode->next )
    {
        // 두 Record의 Hashing Column의 값을 비교
        sValueInfo1.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo1.value  = sNode->func.getRow(sNode, aRow);
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo2.value  = sNode->func.getRow(sNode, sRow);
        sValueInfo2.flag   = MTD_OFFSET_USE;

        sResult = sNode->func.compare( &sValueInfo1, &sValueInfo2 );
        if( sResult != 0)
        {
            // 서로 다른 경우, 더 이상 비교를 하지 않음.
            break;
        }
    }

    if ( sResult == 0 )
    {
        // 동일한 Record를 찾음
        *aResult = ID_TRUE;
    }
    else
    {
        // 동일한 Record가 아님
        *aResult = ID_FALSE;
    }

    return IDE_SUCCESS;
}

