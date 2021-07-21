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

#include <dkdDataMgr.h>


/***********************************************************************
 * Description: Data manager 를 초기화한다.
 *              이 과정에서 DK data buffer 로부터 record buffer 할당이 
 *              가능한지 체크해보고 할당 가능한 경우는 record buffer 
 *              manager 를 생성하고, 그렇지 않다면 disk temp table 
 *              manager 를 생성한다.
 *
 *  aBuffSize  - [OUT] REMOTE_TABLE_STORE 의 record buffer size
 *
 **********************************************************************/
IDE_RC  dkdDataMgr::initialize( UInt * aBuffSize )
{
    mIsRecordBuffer     = ID_TRUE;
    mIsEndOfFetch       = ID_FALSE;
    mRecordCnt          = 0;
    mRecordLen          = 0;
    mRecord             = NULL;
    mRecordBufferSize   = 0;

    mDataBlockRecvCount = 0;
    mDataBlockReadCount = 0;
    mDataBlockPos       = NULL;

    mTypeConverter      = NULL;

    /* BUG-37215 */
    mAllocator          = dkdDataBufferMgr::getTlsfAllocator();
    mRecordBufferMgr    = NULL;
    mDiskTempTableMgr   = NULL;

    mMgrHandle          = NULL;

    mCurRow             = NULL;

    mFetchRowFunc       = NULL;
    mInsertRowFunc      = NULL;
    mRestartFunc        = NULL;

#ifdef ALTIBASE_PRODUCT_XDB
    mDataMgrType        = DKD_DATA_MGR_TYPE_MEMORY;    
#else
    mDataMgrType        = DKD_DATA_MGR_TYPE_DISK;
#endif

    mFetchRowCount      = 0;
    
    mNullRow            = NULL;

    mFlagRestartOnce    = ID_FALSE;
    
    IDE_TEST( initializeRecordBuffer() != IDE_SUCCESS );

    *aBuffSize = mRecordBufferSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Data manager 가 record buffer 이면 record buffer 자원을 할당
 *               REMOTE_TABLE_STORE function
 *               initialize 할 때 memory record buffer 자원을 할당한다.
 ************************************************************************/
IDE_RC  dkdDataMgr::initializeRecordBuffer()
{
    UInt    sAllocableBlockCnt;
    idBool  sLockFlag = ID_FALSE;
    
    IDE_ASSERT( dkdDataBufferMgr::lock() == IDE_SUCCESS );
    sLockFlag = ID_TRUE;

    sAllocableBlockCnt = dkdDataBufferMgr::getAllocableBufferBlockCnt();

    /* check remained data buffer ( DK buffer ) */
    if ( sAllocableBlockCnt > 1 )
    {
        IDE_TEST( createRecordBufferMgr( sAllocableBlockCnt )
                  != IDE_SUCCESS );

        dkdDataBufferMgr::incUsedBufferBlockCount( sAllocableBlockCnt );

        IDE_TEST( getRecordBufferSize() != IDE_SUCCESS );
    }
    else
    {
        switch ( mDataMgrType )
        {
            case DKD_DATA_MGR_TYPE_DISK:
                // disk temp table
                mIsRecordBuffer  = ID_FALSE;                
                break;
                
            case DKD_DATA_MGR_TYPE_MEMORY:
                sLockFlag = ID_FALSE;
                IDE_ASSERT( dkdDataBufferMgr::unlock() == IDE_SUCCESS );
                IDE_RAISE( MEMORY_LIMIT );
                break;                
        }
    }

    sLockFlag = ID_FALSE;
    IDE_ASSERT( dkdDataBufferMgr::unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( MEMORY_LIMIT )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKD_INTERNAL_BUFFER_FULL ) );
    }
    IDE_EXCEPTION_END;

    if ( sLockFlag == ID_TRUE )
    {
        IDE_ASSERT( dkdDataBufferMgr::unlock() == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Data manager 는 disk temp table 를 이용하여 필요한 자원을 할당받는다.
 *
 *  aQcStatement    - [IN] disk temp table 의 생성을 위해 필요한 정보
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::activate( void  *aQcStatement )
{
    IDE_ASSERT( getRecordLength() == IDE_SUCCESS );

    /* set function pointers */
    if ( mIsRecordBuffer == ID_TRUE )
    {
        mMgrHandle     = mRecordBufferMgr;
        mFetchRowFunc  = fetchRowFromRecordBuffer;
        mInsertRowFunc = insertRowIntoRecordBuffer;
        mRestartFunc   = restartRecordBuffer;
    }
    else
    {
        IDE_TEST( createDiskTempTableMgr( aQcStatement ) != IDE_SUCCESS );   // PROJ-2417 disk temp table
        mMgrHandle     = mDiskTempTableMgr;
        mFetchRowFunc  = fetchRowFromDiskTempTable;
        mInsertRowFunc = insertRowIntoDiskTempTable;
        mRestartFunc   = restartDiskTempTable;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Data manager 를 정리한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dkdDataMgr::finalize()
{
    UInt    sUsedBufferBlockCnt = 0;

    if ( mRecordBufferMgr != NULL )
    {
        sUsedBufferBlockCnt = mRecordBufferMgr->getBufferBlockCount();

        IDE_ASSERT( dkdDataBufferMgr::lock() == IDE_SUCCESS );

        dkdDataBufferMgr::decUsedBufferBlockCount( sUsedBufferBlockCnt );

        IDE_ASSERT( dkdDataBufferMgr::unlock() == IDE_SUCCESS );

        /* BUG-37487 : void */
        destroyRecordBufferMgr();
    }
    else
    {
        /* check disk temp table manager */
    }

    if ( mDiskTempTableMgr != NULL )
    {
        /* BUG-37487 : void */
        destroyDiskTempTableMgr();
    }
    else
    {
        /* do nothing */
    }

    if ( mNullRow != NULL )
    {
        (void)iduMemMgr::free( mNullRow, mAllocator );
        mNullRow = NULL;
    }
    else
    {
        /* nothing to do */
    }

    mFlagRestartOnce = ID_FALSE;
}

/************************************************************************
 * Description : Record buffer 혹은 disk temp table 로부터 record 하나를 
 *               fetch 한다.
 *
 *  aEndFlag    - [OUT] 더 이상 가져올 record가 없음을 나타낸다.
 *
 ************************************************************************/
IDE_RC dkdDataMgr::moveNextRow( idBool * aEndFlag )
{
    if ( mIsRecordBuffer != ID_TRUE )
    {
        mCurRow = mRecord->mData;
    }
    else
    {
        /* record buffer */
    }

    IDE_TEST( (*mFetchRowFunc)( mMgrHandle, &mCurRow ) != IDE_SUCCESS );

    if ( mCurRow != NULL )
    {
        *aEndFlag = ID_FALSE;
        
        mFetchRowCount++;
    }
    else
    {
#ifdef ALTIBASE_PRODUCT_XDB
        if ( mIsEndOfFetch == ID_TRUE )
        {
            *aEndFlag = ID_TRUE;
        }
        else
        {
            *aEndFlag = ID_FALSE;            
        }
#else
        mIsEndOfFetch = ID_TRUE;
        *aEndFlag = ID_TRUE;
#endif
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer 혹은 disk temp table 로부터 record 하나를 
 *               fetch 후 복사한다. 
 *
 *  aRow        - [OUT] fetch 해올 record 를 가리키는 포인터, 
 *                      NULL 인 경우 더이상 fetch 할 record 가 없음을 
 *                      나타낸다.
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::fetchRow( void **aRow )
{
    if ( mIsRecordBuffer != ID_TRUE )
    {
        mCurRow = mRecord->mData;
    }
    else
    {
        /* record buffer */
    }

    IDE_TEST( (*mFetchRowFunc)( mMgrHandle, &mCurRow ) != IDE_SUCCESS );

    if ( mCurRow != NULL )
    {
        mFetchRowCount++;

        switch ( mDataMgrType )
        {
            case DKD_DATA_MGR_TYPE_DISK:
                idlOS::memcpy( *aRow, mCurRow, mRowSize );
                break;

            case DKD_DATA_MGR_TYPE_MEMORY:
                *aRow = mCurRow;
                break;
        }
    }
    else
    {
#ifdef ALTIBASE_PRODUCT_XDB
        /* nothing to do */
#else
        mIsEndOfFetch = ID_TRUE;
#endif
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer 혹은 disk temp table 로 record 하나를 
 *               insert 한다. 이 때, 입력받는 row 는 cm block 으로부터 
 *               얻어온 raw data 로 mt type 으로 변환한 후 insert 한다.
 *
 *  aRow            - [IN] insert 할 record 를 가리키는 포인터
 *  aQcStatement    - [IN] disk temp table 로 switch 하는 경우 사용
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::insertRow( void *aRow, void *aQcStatement )
{
    idBool       sIsAllocated     = ID_FALSE;
    dkdRecord   *sRecord = NULL;

    if ( mIsRecordBuffer == ID_TRUE )
    {
        if ( isRemainedRecordBuffer() == ID_TRUE )
        {
            /* Allocate record catridge from TLSF allocator */
            IDU_FIT_POINT_RAISE( "dkdDataMgr::insertRow::malloc::Record", 
                                  ERR_MEMORY_ALLOC_RECORD );
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               mRecordLen,
                                               (void **)&sRecord,
                                               IDU_MEM_IMMEDIATE,
                                               mAllocator )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOC_RECORD );

            sIsAllocated = ID_TRUE;

            sRecord->mData = (SChar *)sRecord + ID_SIZEOF( dkdRecord );
        }
        else /* record buffer doesn't have enough space */
        {
            switch ( mDataMgrType )
            {
                case DKD_DATA_MGR_TYPE_DISK:
                    IDE_TEST( createDiskTempTableMgr( aQcStatement ) != IDE_SUCCESS );

                    if ( mRecordCnt > 0 )
                    {
                        IDE_TEST( moveRecordToDiskTempTable() != IDE_SUCCESS );
                    }
                    else
                    {
                        /* there is no data to move into disk temp table */
                    }
                    
                    switchToDiskTempTable();
                    
                    sRecord = mRecord;                    
                    break;
                    
                case DKD_DATA_MGR_TYPE_MEMORY:
                    IDE_RAISE( MEMORY_LIMIT );
                    break;

                default:
                    IDE_DASSERT(0); /* can't reach here in fact */
            }
        }
    }
    else    
    {
        /* use disk temp table */
        sRecord = mRecord;
    }

    IDE_TEST( dkdTypeConverterConvertRecord( mTypeConverter, 
                                             aRow, 
                                             sRecord->mData ) 
              != IDE_SUCCESS );

    IDE_TEST( (*mInsertRowFunc)( mMgrHandle, sRecord ) != IDE_SUCCESS );

    mRecordCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( MEMORY_LIMIT )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKD_INTERNAL_BUFFER_FULL ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_RECORD );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ));
    }
    IDE_EXCEPTION_END;

    if ( sIsAllocated == ID_TRUE )
    {
        (void)iduMemMgr::free( (void *)sRecord, mAllocator );
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer 혹은 disk temp table 의 cursor 를 restart
 *               시킨다.
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::restart()
{
    IDE_TEST( (*mRestartFunc)( mMgrHandle ) != IDE_SUCCESS );

    mFetchRowCount = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkdDataMgr::restartOnce( void )
{
    if ( mFlagRestartOnce == ID_FALSE )
    {
        IDE_TEST( restart() != IDE_SUCCESS );
        mFlagRestartOnce = ID_TRUE;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer manager 를 생성한다.
 *
 *  aAllocableBlockCnt - [IN] 이 record buffer 에 할당할 block 개수
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::createRecordBufferMgr( UInt aAllocableBlockCnt )
{
    idBool               sIsAllocated  = ID_FALSE; 
    dkdRecordBufferMgr  *sRecordBufMgr = NULL;

    IDU_FIT_POINT_RAISE( "dkdDataMgr::createRecordBufferMgr::malloc::RecordBufMgr",
                          ERR_MEMORY_ALLOC_RECORD_BUFFER_MGR );   
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dkdRecordBufferMgr ),
                                       (void **)&sRecordBufMgr,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_RECORD_BUFFER_MGR );

    sIsAllocated = ID_TRUE;

    /* BUG-37215 */
    IDE_TEST( sRecordBufMgr->initialize( aAllocableBlockCnt, mAllocator ) 
              != IDE_SUCCESS );

    mRecordBufferMgr = sRecordBufMgr;
    mIsRecordBuffer  = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_RECORD_BUFFER_MGR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAllocated == ID_TRUE )
    {
        (void)iduMemMgr::free( sRecordBufMgr );
        mRecordBufferMgr = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer manager 를 제거한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dkdDataMgr::destroyRecordBufferMgr()
{
    /* BUG-37487 : void */
    (void)mRecordBufferMgr->finalize();

    (void)iduMemMgr::free( mRecordBufferMgr );

    mRecordBufferMgr  = NULL;
    mRecordBufferSize = 0;
}

/************************************************************************
 * Description : Disk temp table manager 를 생성한다. 
 *
 *  aQcStatement    - [IN] disk temp table 의 생성을 위해 필요
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::createDiskTempTableMgr( void    *aQcStatement )
{
    UInt                  sColCnt;
    UInt                  sStage;
    mtcColumn            *sColMetaArr       = NULL;
    dkdDiskTempTableMgr  *sDiskTempTableMgr = NULL;

    sStage = 0;

    IDU_FIT_POINT_RAISE( "dkdDataMgr::createDiskTempTableMgr::malloc::DiskTempTableMgr",
                          ERR_MEMORY_ALLOC_DISK_TEMP_TABLE_MGR );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dkdDiskTempTableMgr ),
                                       (void **)&sDiskTempTableMgr,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_DISK_TEMP_TABLE_MGR );

    sStage = 1;

    /* get column meta array */
    IDE_TEST( dkdTypeConverterGetConvertedMeta( mTypeConverter,
                                                &sColMetaArr ) 
              != IDE_SUCCESS );

    /* get column count */
    IDE_TEST( dkdTypeConverterGetColumnCount( mTypeConverter,
                                              &sColCnt )
              != IDE_SUCCESS );

    /* create disk temp table */
    IDE_TEST( sDiskTempTableMgr->initialize( aQcStatement, 
                                             sColMetaArr, 
                                             sColCnt ) 
              != IDE_SUCCESS );

    sStage = 2;

    mDiskTempTableMgr = sDiskTempTableMgr;

    /* allocate record catridge for disk temp table */
    IDU_FIT_POINT_RAISE( "dkdDataMgr::createDiskTempTableMgr::malloc::Record",
                          ERR_MEMORY_ALLOC_RECORD_CATRIDGE );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       mRecordLen,
                                       (void **)&mRecord,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_RECORD_CATRIDGE );

    sStage = 3;

    mRecord->mData = (SChar *)mRecord + ID_SIZEOF( dkdRecord );

    mIsRecordBuffer = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DISK_TEMP_TABLE_MGR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ));
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_RECORD_CATRIDGE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            (void)iduMemMgr::free( mRecord );
            mRecord = NULL;
            /* keep going */
        case 2:
            (void)sDiskTempTableMgr->finalize();
            mDiskTempTableMgr = NULL;
            /* keep going */
        case 1:
            (void)iduMemMgr::free( sDiskTempTableMgr );
            /* keep going */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Disk temp table manager 를 제거한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dkdDataMgr::destroyDiskTempTableMgr()
{
    /* >> BUG-37487 */
    if ( mDiskTempTableMgr->finalize() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_DK_0 );
        ideLog::log( IDE_DK_0, "[WARNING] Failed to destroy DK disk temp table..");
    }
    else
    {
        /* success */
    }
    /* << BUG-37487 */

    (void)iduMemMgr::free( mDiskTempTableMgr );

    mDiskTempTableMgr = NULL;
}

/************************************************************************
 * Description : Type converter 를 생성한다. Type converter 는 result
 *               set meta 정보를 갖고 있다. 
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::createTypeConverter( dkpColumn   *aColMetaArr,
                                         UInt         aColCount )
{
    IDE_TEST( dkdTypeConverterCreate( aColMetaArr, 
                                      aColCount, 
                                      &mTypeConverter )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Type converter 를 제거한다.
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::destroyTypeConverter()
{
    IDE_TEST( dkdTypeConverterDestroy( mTypeConverter ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Type converter 가 altibase type 으로 변환해 갖고 있는 
 *               meta 정보를 요청한다.
 *
 *  aMeta       - [IN] 요청한 meta 정보가 담길 구조체 포인터
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::getConvertedMeta( mtcColumn **aMeta )
{
    IDE_TEST( dkdTypeConverterGetConvertedMeta( mTypeConverter, aMeta )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Type converter 로부터 converted row 의 길이를 얻어와 
 *               실제 DK 에서 저장할 record 의 길이를 구해 설정한다.
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::getRecordLength()
{
    UInt    sConvertedRowLen;

    IDE_TEST( dkdTypeConverterGetRecordLength( mTypeConverter,
                                               &sConvertedRowLen )
              != IDE_SUCCESS );

    mRowSize   = sConvertedRowLen;
    mRecordLen = ID_SIZEOF( dkdRecord ) + sConvertedRowLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Type converter 로부터 converted row 의 길이를 얻어와 
 *               실제 DK 에서 저장할 record 의 길이를 구해 설정한다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
void dkdDataMgr::switchToDiskTempTable()
{
    UInt    sUsedBufferBlockCnt;

    mMgrHandle          = mDiskTempTableMgr;
    mFetchRowFunc       = fetchRowFromDiskTempTable;
    mInsertRowFunc      = insertRowIntoDiskTempTable;
    mRestartFunc        = restartDiskTempTable;
    mIsRecordBuffer     = ID_FALSE;

    sUsedBufferBlockCnt = mRecordBufferMgr->getBufferBlockCount();

    IDE_ASSERT( dkdDataBufferMgr::lock() == IDE_SUCCESS );

    dkdDataBufferMgr::decUsedBufferBlockCount( sUsedBufferBlockCnt );

    IDE_ASSERT( dkdDataBufferMgr::unlock() == IDE_SUCCESS );

    /* return memory buffer to DK */
    destroyRecordBufferMgr();
}

IDE_RC  dkdDataMgr::getRecordBufferSize()
{
    if ( mRecordBufferMgr != NULL )
    {
        mRecordBufferSize = mRecordBufferMgr->getRecordBufferSize();
    }
    else
    {
        mRecordBufferSize = 0;
    }

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : Type converter 로부터 converted row 의 길이를 얻어와 
 *               실제 DK 에서 저장할 record 의 길이를 구해 설정한다.
 *
 ************************************************************************/
IDE_RC  dkdDataMgr::moveRecordToDiskTempTable()
{
    dkdRecord       *sRecord        = NULL;
    iduListNode     *sCurRecordNode = NULL;
    iduList         *sRecordList    = NULL;

    sRecordList = mRecordBufferMgr->getRecordList();

    IDU_LIST_ITERATE( sRecordList, sCurRecordNode )
    {
        sRecord = (dkdRecord *)sCurRecordNode->mObj;

        IDE_TEST( mDiskTempTableMgr->insertRow( sRecord ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 *
 */
IDE_RC dkdDataMgr::getNullRow( void ** aRow, scGRID * aRid )
{
    if ( mNullRow == NULL )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           mRecordLen,
                                           (void **)&mNullRow,
                                           IDU_MEM_IMMEDIATE,
                                           mAllocator )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_RECORD );

        IDE_TEST( dkdTypeConverterMakeNullRow( mTypeConverter, mNullRow ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    *aRow = mNullRow;

    SC_MAKE_GRID( *aRid,
                  SC_NULL_SPACEID,
                  SC_NULL_PID,
                  SC_NULL_OFFSET );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_RECORD )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
idBool dkdDataMgr::isNeedFetch( void )
{
    idBool sResult = ID_FALSE;

    if ( mIsEndOfFetch == ID_TRUE )
    {
        sResult = ID_FALSE;
    }
    else
    {
        IDE_DASSERT( mFetchRowCount <= mRecordCnt );

        if ( ( mRecordCnt == 0 ) ||
             ( mFetchRowCount + 1 == mRecordCnt ) )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }

    return sResult;
}
