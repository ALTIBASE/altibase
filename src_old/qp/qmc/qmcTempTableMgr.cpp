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
 * $Id: qmcTempTableMgr.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 *   Temp Table Manager
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qmcTempTableMgr.h>
#include <smiTempTable.h>


IDE_RC
qmcTempTableMgr::init( qmcdTempTableMgr * aTableMgr,
                       iduMemory        * aMemory )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table Manager를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
#define IDE_FN "qmcTempTableMgr::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcTempTableMgr::init"));

    IDE_DASSERT( aTableMgr != NULL );

    // Table Manager의 멤버 초기화
    aTableMgr->mTop = NULL;
    aTableMgr->mCurrent = NULL;
    aTableMgr->mMemory = NULL;

    // Table Handle 객체 관리를 위한 Memory 관리자의 초기화
    IDU_FIT_POINT( "qmcTempTableMgr::init::alloc::mMemory",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aMemory->alloc( ID_SIZEOF(qmcMemory),
                              (void**) & aTableMgr->mMemory )
              != IDE_SUCCESS);

    aTableMgr->mMemory = new (aTableMgr->mMemory)qmcMemory();

    /* BUG-38290 */
    aTableMgr->mMemory->init( ID_SIZEOF(qmcCreatedTable) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

/* BUG-38290 
 * Table manager 의 addTempTable 함수는 동시에 호출될 수 없어서
 * 동시성 문제가 발생하지 않는다.
 * 따라서 addTempTable 함수는 mutex 를 통한 동시성 제어를 할 필요가 없다.
 *
 * 동시성 문제가 발생하려면 addTempTable 함수가 동시에 호출되어야 한다.
 * 그러기 위해서는 service thread 와 worker thread,
 * 혹은 worker thread 와 worker thread 가 동시에 temp table 을 생성해야 한다.
 *
 * 현재 temp table 을 생성하는 곳은 다음 노드들의 firstInit 함수이다.
 *   AGGR
 *   CUBE
 *   GRAG
 *   HASH
 *   HSDS
 *   ROLL
 *   SDIF
 *   SITS
 *   WNST
 *
 * Worker thread 에 의해 init 이 실행되려면 PRLQ 의 아래에
 * 플랜 노드가 위치해야 하는데, 위 노드들은 현재 PRLQ 의 아래에 올 수 없다.
 * 그러므로 일반적인 경우에는 worker thread 에 의해 temp table 이
 * 생성될 수 없고, 동시성 문제도 발생하지 않는다.
 *
 * 다만 한가지 예외로서 위 노드들이 subquery filter 에 존재할 경우에는
 * worker thread 를 통해 init 될 수 있다.
 *
 * 하지만 subquery filter 는 partitioned table 의 SCAN 에 달릴 수 없으므로
 * PRLQ 아래에 있는 SCAN 중 subqeury filter 를 가지는 경우는
 * HASH, SORT, GRAG 등의 노드로 제한된다.
 *
 * 이런 노드들은 각 노드가 init 될 때 SCAN 의 doIt 을 모두 마치므로
 * 동시성 문제가 발생하려면 두 개 이상의 HASH, SORT 혹은 GRAG 가
 * 동시에 init 되어야 한다.
 *
 * 하지만 플랜 노드가 동시에 init 되는 경우는 존재하지 않으므로,
 * temp table manager 의 동시성 문제는 발생하지 않는다.
 */
IDE_RC qmcTempTableMgr::addTempTable( iduMemory        * aMemory,
                                      qmcdTempTableMgr * aTableMgr,
                                      void             * aTableHandle )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table Manager에 생성된 Temp Table의 Handle을 등록한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
#define IDE_FN "qmcTempTableMgr::addTempTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcTempTableMgr::addTempTable"));

    qmcCreatedTable * sTable;

    // Table Handle 저장을 위한 공간 할당
    IDU_FIT_POINT( "qmcTempTableMgr::addTempTable::alloc::sTable",
                    idERR_ABORT_InsufficientMemory );

    /* BUG-38290 */
    IDE_TEST( aTableMgr->mMemory->alloc( aMemory,
                                         ID_SIZEOF(qmcCreatedTable),
                                         (void**) & sTable )
              != IDE_SUCCESS);

    // Table Handle 정보의 Setting
    sTable->tableHandle = aTableHandle;
    sTable->next   = NULL;

    // Temp Table Manager에 연결
    if ( aTableMgr->mTop == NULL )
    {
        // 최초 등록인 경우
        aTableMgr->mTop = aTableMgr->mCurrent = sTable;
    }
    else
    {
        // 이미 등록되어 있는 경우
        aTableMgr->mCurrent->next = sTable;
        aTableMgr->mCurrent = sTable;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}


IDE_RC
qmcTempTableMgr::dropAllTempTable( qmcdTempTableMgr   * aTableMgr )
{
/***********************************************************************
 *
 * Description :
 *     Temp Table Manager에 등록된 모든 Table Handle에 대한
 *     Temp Table들을 모두 삭제한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
#define IDE_FN "qmcTempTableMgr::dropAllTempTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcTempTableMgr::dropAllTempTable"));

    qmcCreatedTable * sTable;

    // 모든 Table Handle에 대하여 Table을 Drop 한다.
    for ( sTable = aTableMgr->mTop;
          sTable != NULL;
          sTable = sTable->next )
    {
        IDE_TEST(smiTempTable::drop( sTable->tableHandle ) != IDE_SUCCESS);
    }

    // 재사용을 위해 Memory를 Clear한다.
    if ( aTableMgr->mMemory != NULL )
    {
        // To fix BUG-17591
        // 본 함수가 불리는 시점은 qmxMemory가 query statement단위로
        // execute시작지점으로 돌아갈 때 이기 때문에
        // 메모리를 전부 초기화 하기 위해 qmcMemory::clear함수를 호출한다.
        aTableMgr->mMemory->clear( ID_SIZEOF(qmcCreatedTable) );
    }
    aTableMgr->mTop = aTableMgr->mCurrent = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // Error 발생 시 남아 있는 Handle을 모두 제거한다.
    sTable = sTable->next;

    for ( ; sTable != NULL; sTable = sTable->next )
    {
        (void) smiTempTable::drop( sTable->tableHandle );
    } 
    if ( aTableMgr->mMemory != NULL )
    {
        aTableMgr->mMemory->clear( ID_SIZEOF(qmcCreatedTable) );
    }

    aTableMgr->mTop = aTableMgr->mCurrent = NULL;

    return IDE_FAILURE;
    
#undef IDE_FN
}
