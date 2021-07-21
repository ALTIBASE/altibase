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
 * $id$
 **********************************************************************/

#include <dktRemoteTx.h>
#include <dktGlobalCoordinator.h>

IDE_RC dktRemoteTx::initialize( UInt            aSessionId,
                                SChar          *aTargetName,
                                dktLinkerType   aLinkerType,
                                sdiConnectInfo *aDataNode,
                                UInt            aGlobalTxId,
                                UInt            aLocalTxId,
                                UInt            aRTxId )
{
	idBool sIsMutexInit = ID_FALSE;
    
	/* BUG-44672 
     * Performance View 조회할때 RemoteStatement 의 추가 삭제시 PV 를 조회하면 동시성 문제가 생긴다.
     * 이를 방지하기 위한 Lock 으로 일반 DK 도중 findRemoteStatement 와 같은 함수는
     * 같은 DK 세션에서만 들어오므로 동시성에 문제가 없으므로  
     * RemoteStatment Add 와 Remove 를 제외하고는 Lock 을 잡지 않는다. */
    IDE_TEST_RAISE( mDktRStmtMutex.initialize( (SChar *)"DKT_REMOTE_STATMENT_MUTEX",
                                               IDU_MUTEX_KIND_POSIX,
                                               IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
	sIsMutexInit = ID_TRUE;

    /* Initialize members */
    mIsTimedOut             = ID_FALSE;

    mRemoteNodeSessionId    = 0;
    mStmtIdBase             = 0;

    mLinkerSessionId        = aSessionId;
    mId                     = aRTxId;
    mLocalTxId              = aLocalTxId;
    mGlobalTxId             = aGlobalTxId;
    mStatus                 = DKT_RTX_STATUS_NON;

    mCurRemoteStmtId        = 0;
    mNextRemoteStmtId       = 0;
    mStmtCnt                = 0;

    idlOS::strncpy( mTargetName, aTargetName, DK_NAME_LEN );
    mTargetName[ DK_NAME_LEN ] = '\0';
    mLinkerType             = aLinkerType;
    mDataNode               = aDataNode;

    /* Remote statement 의 관리를 위한 list 초기화 */
    IDU_LIST_INIT( &mRemoteStmtList );

    /* Savepoint 의 관리를 위한 list 초기화 */
    IDU_LIST_INIT( &mSavepointList );

    idlOS::memset( (void*)&mXID, 0x00, ID_SIZEOF(ID_XID) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_SET( ideSetErrorCode( dkERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;

	if ( sIsMutexInit == ID_TRUE )
	{
		sIsMutexInit = ID_FALSE;
	    (void)mDktRStmtMutex.destroy();
	}
	else
	{
		/* nothing to do */
	}

    return IDE_FAILURE;
}

/* >> BUG-37487 : void */
void dktRemoteTx::finalize()
{
    /* Remote statement list 정리 */
    destroyAllRemoteStmt();

    /* Savepoint list 정리 */
    deleteAllSavepoint();

    (void)mDktRStmtMutex.destroy();
}
/* << BUG-37487 : void */

/************************************************************************
 * Description : Remote statement list 를 정리한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dktRemoteTx::destroyAllRemoteStmt()
{
    dktRemoteStmt   *sRemoteStmt = NULL;
    iduListNode     *sIterator   = NULL;
    iduListNode     *sNext       = NULL;

    IDE_ASSERT( mDktRStmtMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &mRemoteStmtList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &mRemoteStmtList, sIterator, sNext )
        {
            sRemoteStmt = (dktRemoteStmt *)sIterator->mObj;
            /* BUG-37487 : void */
            if ( sRemoteStmt != NULL )
            {
                IDU_LIST_REMOVE( &sRemoteStmt->mNode );
                mStmtCnt -= 1;

                /* BUG-37487 : void */
                sRemoteStmt->finalize();

                (void)iduMemMgr::free( sRemoteStmt );
                sRemoteStmt = NULL;
            }
            else
            {
                /* success */
            }
        }
    }
    else
    {
        /* success 로 간주 */
    }

    IDE_ASSERT( mDktRStmtMutex.unlock() == IDE_SUCCESS );

    mStmtIdBase = 0;
}

/************************************************************************
 * Description : Savepoint list 를 정리한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dktRemoteTx::deleteAllSavepoint()
{
    dktSavepoint    *sSavepoint = NULL;
    iduListNode     *sIterator  = NULL;
    iduListNode     *sNext      = NULL;

    if ( IDU_LIST_IS_EMPTY( &mSavepointList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &mSavepointList, sIterator, sNext )
        {
            sSavepoint = (dktSavepoint *)sIterator->mObj;
            IDU_LIST_REMOVE( sIterator );

            (void)iduMemMgr::free( sSavepoint );
        }
    }
    else
    {
        /* success 로 간주 */
    }
}

/************************************************************************
 * Description : 새로운 remote statement 를 하나 생성하여 초기화해준 뒤
 *               list (mRemoteStmtList) 에 추가한다.
 *
 *  aQcStatement    - [IN] qcStatement
 *  aStmtType       - [IN] 생성할 remote statement 의 타입
 *  aStmtStr        - [IN] 원격서버에서 수행할 구문
 *  aRemoteStmt     - [OUT] 생성한 remote statement 객체
 *
 ************************************************************************/
IDE_RC  dktRemoteTx::createRemoteStmt( UInt             aStmtType, 
                                       SChar           *aStmtStr,
                                       dktRemoteStmt  **aRemoteStmt )
{
    SLong            sRemoteStmtId  = 0;
    dktRemoteStmt   *sRemoteStmt    = NULL;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dktRemoteStmt ),
                                       (void **)&sRemoteStmt,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_REMOTE_STMT );

    sRemoteStmtId = generateRemoteStmtId(); 

    IDE_TEST( sRemoteStmt->initialize( mGlobalTxId,
                                       mLocalTxId,
                                       mId,
                                       sRemoteStmtId, 
                                       aStmtType, 
                                       aStmtStr )
              != IDE_SUCCESS );

	IDU_LIST_INIT_OBJ( &(sRemoteStmt->mNode), sRemoteStmt );

    IDE_ASSERT( mDktRStmtMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
    
    IDU_LIST_ADD_LAST( &mRemoteStmtList, &(sRemoteStmt->mNode) );
    mStmtCnt += 1;

    IDE_ASSERT( mDktRStmtMutex.unlock() == IDE_SUCCESS );

    *aRemoteStmt = sRemoteStmt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sRemoteStmt != NULL )
    {
        (void)iduMemMgr::free( sRemoteStmt );
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 해당하는 remote statement 를 remote statement list 
 *               로부터 제거한다.
 *
 *  aRemoteStmt -[IN] 제거할 remote statement 를 가리키는 포인터 
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dktRemoteTx::destroyRemoteStmt( dktRemoteStmt   *aRemoteStmt )
{
    if ( aRemoteStmt != NULL )
    {
        IDE_ASSERT( mDktRStmtMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

        IDU_LIST_REMOVE( &aRemoteStmt->mNode );
        mStmtCnt -= 1;

        IDE_ASSERT( mDktRStmtMutex.unlock() == IDE_SUCCESS );

        /* BUG-37487 : void */
        aRemoteStmt->finalize();

        (void)iduMemMgr::free( aRemoteStmt );
    }
    else
    {
        /* success */
    }
}

/************************************************************************
 * Description : 입력받은 id 를 갖는 remote statement 를 찾는다.
 * Return      : 찾은 remote statement 객체, 없으면 NULL
 *
 *  aRemoteStmtId  - [IN] 찾을 remote statement id
 *
 ************************************************************************/
dktRemoteStmt*  dktRemoteTx::findRemoteStmt( SLong  aRemoteStmtId )
{
    iduListNode     *sIterator   = NULL;
    dktRemoteStmt   *sRemoteStmt = NULL;

    IDU_LIST_ITERATE( &mRemoteStmtList, sIterator )
    {
        sRemoteStmt = (dktRemoteStmt *)sIterator->mObj;

        IDE_DASSERT( sRemoteStmt != NULL );

        if ( aRemoteStmtId == sRemoteStmt->getStmtId() )
        {
            break;
        }
        else
        {
            sRemoteStmt = NULL;
        }
    }

    return sRemoteStmt;
}

/************************************************************************
 * Description : 이 remote transaction 에 수행중인 remote statement 가 
 *               없는지 확인한다. 
 *
 ************************************************************************/
idBool  dktRemoteTx::isEmptyRemoteTx() 
{
    idBool  sIsEmpty;

    if ( IDU_LIST_IS_EMPTY( &mRemoteStmtList ) == ID_TRUE )
    {
        sIsEmpty = ID_TRUE;
    }
    else
    {
        sIsEmpty = ID_FALSE;
    }

    return sIsEmpty;
}

/************************************************************************
 * Description : Remote transaction 에 savepoint 를 설정한다.
 *
 *  aSavepointName  - [IN] 설정할 savepoint name 
 ************************************************************************/
IDE_RC  dktRemoteTx::setSavepoint( const SChar  *aSavepointName )
{
    UInt                sSavepointNameLen = 0;
    dktSavepoint       *sSavepoint        = NULL;

    IDE_TEST_RAISE( aSavepointName == NULL,
                    ERR_SAVEPOINT_NOT_EXIST );
    IDE_TEST_RAISE( findSavepoint( aSavepointName ) != NULL, 
                    ERR_SAVEPOINT_ALREADY_EXIST );

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                       1, /* alloc count */
                                       ID_SIZEOF( dktSavepoint ),
                                       (void **)&sSavepoint,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_SAVEPOINT );

    sSavepointNameLen = idlOS::strlen( aSavepointName );
    idlOS::memcpy( sSavepoint->mName, aSavepointName, sSavepointNameLen );

    IDU_LIST_INIT_OBJ( &(sSavepoint->mNode), sSavepoint );
    IDU_LIST_ADD_LAST( &mSavepointList, &(sSavepoint->mNode) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SAVEPOINT_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_SAVEPOINT_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_SAVEPOINT_ALREADY_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_SAVEPOINT_ALREADY_EXIST ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_SAVEPOINT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 입력받은 이름과 동일한 savepoint 가 list 에 존재하는지 
 *               검사하여 반환한다.
 *
 *  aSavepointName  - [IN] 찾을 savepoint name 
 *
 ************************************************************************/
dktSavepoint *  dktRemoteTx::findSavepoint( const SChar *aSavepointName )
{
    iduListNode     *sIterator  = NULL;
    dktSavepoint    *sSavepoint = NULL;

    IDU_LIST_ITERATE( &mSavepointList, sIterator )
    {
        sSavepoint = (dktSavepoint *)sIterator->mObj;

        IDE_DASSERT( sSavepoint != NULL );

        if ( idlOS::strcmp( aSavepointName, sSavepoint->mName ) == 0 )
        {
            break;
        }
        else
        {
            /* iterate next or NULL return */
            sSavepoint = NULL;
        }
    }

    return sSavepoint;
}

/************************************************************************
 * Description : Savepoint 를 list 에서 제거한다.
 *
 *  aSavepointName  - [IN] 제거할 savepoint name 
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 *  BUG-37512 : 원래 기능은 removeAllNextSavepoint 함수가 수행하고
 *              이 함수는 입력받은 savepoint 만 제거하는 함수로 변경.
 *
 ************************************************************************/
void    dktRemoteTx::removeSavepoint( const SChar   *aSavepointName )
{
    dktSavepoint    *sSavepoint = NULL;

    IDE_DASSERT( aSavepointName != NULL );

    sSavepoint = findSavepoint( aSavepointName );

    if ( sSavepoint != NULL )
    {
        IDU_LIST_REMOVE( &sSavepoint->mNode );
        (void)iduMemMgr::free( sSavepoint );
    }
    else
    {
        /* can't find */
    }
}

/************************************************************************
 * Description : 입력받은 savepoint 이후에 찍힌 모든 savepoint 를 list 
 *               에서 제거한다.
 *
 *  aSavepointName  - [IN] savepoint name 
 *
 *  BUG-37512 : 신설.
 *
 ************************************************************************/
void    dktRemoteTx::removeAllNextSavepoint( const SChar    *aSavepointName )
{
    iduList          sRemoveList;
    iduListNode     *sIterator        = NULL;
    iduListNode     *sNextNode        = NULL;
    iduListNode     *sNextSavepointNode = NULL;
    dktSavepoint    *sSavepoint       = NULL;
    dktSavepoint    *sRemoveSavepoint = NULL;

    IDE_DASSERT( aSavepointName != NULL );

    sSavepoint = findSavepoint( aSavepointName );

    if ( sSavepoint != NULL )
    {
        IDU_LIST_INIT( &sRemoveList );
        sNextSavepointNode = (iduListNode *)(&sSavepoint->mNode)->mNext;
        IDU_LIST_SPLIT_LIST( &mSavepointList, sNextSavepointNode, &sRemoveList );

        IDU_LIST_ITERATE_SAFE( &sRemoveList, sIterator, sNextNode )
        {
            sRemoveSavepoint = (dktSavepoint *)sIterator->mObj;

            IDU_LIST_REMOVE( &sRemoveSavepoint->mNode );
            (void)iduMemMgr::free( sRemoveSavepoint );
        }
    }
    else
    {
        /* can't find */
    }
}

/************************************************************************
 * Description : 이 Remote transaction 의 정보를 얻어온다. 
 *
 *  aInfo       - [IN] 이 remote transaction 의 정보를 담아줄 구조체를 
 *                     가리키는 포인터
 *
 ************************************************************************/
IDE_RC  dktRemoteTx::getRemoteTxInfo( dktRemoteTxInfo   *aInfo )
{
    IDE_DASSERT( aInfo != NULL );

    aInfo->mGlobalTxId = mGlobalTxId;
    aInfo->mLocalTxId  = mLocalTxId;
    aInfo->mRTxId      = mId;

    dktXid::copyXID( &(aInfo->mXID), &mXID );
    idlOS::strncpy( aInfo->mTargetInfo, mTargetName, DK_NAME_LEN );
    aInfo->mTargetInfo[ DK_NAME_LEN ] = '\0';

    aInfo->mStatus = mStatus;

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : 이 remote transaction 에 속한 모든 remote statement 의 
 *               정보를 얻어낸다.
 *
 *  aInfo           - [IN/OUT] Remote statement 정보가 담길 배열 포인터
 *  aRemainedCnt    - [OUT] 남아있는 remote statement 갯수
 *  aInfoCnt        - [OUT] 배열에 담긴 remote statement 정보의 갯수
 *  
 ************************************************************************/
IDE_RC  dktRemoteTx::getRemoteStmtInfo( dktRemoteStmtInfo    *aInfo,
                                        UInt                  aStmtCnt,
                                        UInt                 *aInfoCnt )
{
    idBool           sIsLock = ID_FALSE;
    UInt             sRemoteStmtCnt = *aInfoCnt;
    iduListNode     *sIterator   = NULL;
    dktRemoteStmt   *sRemoteStmt = NULL;

    IDE_ASSERT( mDktRStmtMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
    sIsLock = ID_TRUE;

    IDU_LIST_ITERATE( &mRemoteStmtList, sIterator )
    {
        sRemoteStmt = (dktRemoteStmt *)sIterator->mObj;

        IDE_TEST_CONT( sRemoteStmtCnt == aStmtCnt, FOUND_COMPLETE );

        if ( sRemoteStmt != NULL )
        {
            IDE_TEST( sRemoteStmt->getRemoteStmtInfo( &aInfo[sRemoteStmtCnt] ) 
                      != IDE_SUCCESS );

            sRemoteStmtCnt++;
        }
        else
        {
            /* no more remote statement */
        }
    }

    IDE_EXCEPTION_CONT( FOUND_COMPLETE );

    sIsLock = ID_FALSE;
    IDE_ASSERT( mDktRStmtMutex.unlock() == IDE_SUCCESS );

    *aInfoCnt = sRemoteStmtCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
		sIsLock = ID_FALSE;
        IDE_ASSERT( mDktRStmtMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement id 를 생성하여 반환한다. 
 *
 * Return      : 생성한 remote statement id 
 *
 ************************************************************************/
SLong   dktRemoteTx::generateRemoteStmtId()
{
    UShort  sTargetId       = 0;
    UInt    sTruncateBitCnt = 0;
    SLong   sId             = 0;

    mStmtIdBase += 1;

    if ( ( mStmtIdBase ^ DKT_REMOTE_STMT_ID_XOR_MASK ) == DKT_REMOTE_STMT_ID_END )
    {
        mStmtIdBase = DKT_REMOTE_STMT_ID_BEGIN;
    }
    else
    {
        /* use this number */
    }

    sId += (SLong)mStmtIdBase << ( ( ( ID_SIZEOF( sId ) / ID_SIZEOF( mStmtIdBase ) ) - 1 ) 
                                   * ID_SIZEOF( mStmtIdBase ) * 8 );

    sTruncateBitCnt = ID_SIZEOF( mId ) - ID_SIZEOF( sTargetId );

    if ( sTruncateBitCnt > 0 )
    {
        sTargetId = (UShort)( ( mId << sTruncateBitCnt ) >> sTruncateBitCnt );
    }
    else
    {
        sTargetId = (UShort)mId;
    }

    sId += (SLong)sTargetId << ( 
        ( ( ID_SIZEOF( sId ) / ID_SIZEOF( sTargetId ) ) - 2 ) 
        * ID_SIZEOF( sTargetId ) * 8 );

    sId += (SLong)mLocalTxId;

    return sId;
}

IDE_RC dktRemoteTx::freeAndDestroyAllRemoteStmt( dksSession *aSession, UInt  aSessionId )
{
    iduListNode     *sIterator   = NULL;
    iduListNode     *sDummy = NULL;
    dktRemoteStmt   *sRemoteStmt = NULL;

    IDU_LIST_ITERATE_SAFE( &mRemoteStmtList, sIterator, sDummy )
    {
        sRemoteStmt = (dktRemoteStmt *)sIterator->mObj;
        IDU_LIST_REMOVE( sIterator );

        if ( sRemoteStmt != NULL )
        {        
            IDE_TEST( sRemoteStmt->free( aSession, aSessionId )
                      != IDE_SUCCESS );
            
            destroyRemoteStmt( sRemoteStmt );
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
