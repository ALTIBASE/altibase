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

#include <dkdRemoteTableMgr.h>
#include <dkdMisc.h>
#include <dktDef.h>

/***********************************************************************
 * Description: Remote table manager 를 초기화한다.
 *
 *  aBufBlockCnt    - [IN] 이 record buffer manager 가 할당받을 buffer
 *                         를 구성하는 buffer block 의 개수
 *  aAllocator      - [IN] record buffer 를 할당받을 때 사용하기 위한 
 *                         TLSF memory allocator 를 가리키는 포인터
 *
 *  aBuffSize  - [OUT] REMOTE_TABLE 의 record buffer size
 *
 **********************************************************************/
IDE_RC  dkdRemoteTableMgr::initialize( UInt * aBuffSize )
{
    mIsEndOfFetch           = ID_FALSE;
    mIsFetchBufReadAll      = ID_FALSE;
    mIsFetchBufFull         = ID_FALSE;
    mRowSize                = 0;
    mFetchRowBufSize        = DKU_DBLINK_REMOTE_TABLE_BUFFER_SIZE * DKD_BUFFER_BLOCK_SIZE_UNIT_MB;
    mConvertedRowBufSize    = 0;
    mFetchRowCnt            = 0;
    mInsertRowCnt           = 0;
    mRecordCnt              = 0;
    mFetchRowBuffer         = NULL;
    mConvertedRowBuffer     = NULL;
    mInsertPos              = NULL;
    mTypeConverter          = NULL;
    mCurRow                 = NULL;
    *aBuffSize              = mFetchRowBufSize;

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : 원격서버로부터 가져온 레코드를 일시적으로 저장할 버퍼 
 *               등 필요한 자원을 할당한다.
 *
 ************************************************************************/
IDE_RC  dkdRemoteTableMgr::activate()
{
    IDE_ASSERT( mTypeConverter != NULL );

    mConvertedRowBuffer = NULL;
    
    /* mRowSize */
    IDE_TEST( dkdTypeConverterGetRecordLength( mTypeConverter, &mRowSize )
              != IDE_SUCCESS );

    /* mFetchRowBufSize / mFetchRowCnt / mFetchRowBuffer */
    IDE_TEST( makeFetchRowBuffer() != IDE_SUCCESS );

    /* mConvertedRowBufSize / mConvertedRowBuffer */
    if ( mRowSize > DKP_ADLP_PACKET_DATA_MAX_LEN ) 
    {
        mConvertedRowBufSize = mRowSize;
    }
    else
    {
        mConvertedRowBufSize = DK_MAX_PACKET_LEN;
    }

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                       1,
                                       mConvertedRowBufSize,
                                       (void **)&mConvertedRowBuffer,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_FETCH_ROW_BUFFER );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_FETCH_ROW_BUFFER );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    if ( mConvertedRowBuffer != NULL )
    {
        (void)iduMemMgr::free( mConvertedRowBuffer );
        mConvertedRowBuffer = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : 원격서버로부터 fetch 해온 레코드를 insert 하기 위해 
 *               일시적으로 보관할 버퍼를 할당한다.
 *
 ************************************************************************/
IDE_RC  dkdRemoteTableMgr::makeFetchRowBuffer()
{
    idBool              sIsAllocated    = ID_FALSE;

    if ( mFetchRowBuffer == NULL )
    {
        if ( mRowSize > 0 )
        {
            IDE_TEST( dkdMisc::getFetchRowCnt( mRowSize,
                                               mFetchRowBufSize,
                                               &mFetchRowCnt,
                                               DKT_STMT_TYPE_REMOTE_TABLE )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               mFetchRowBufSize,
                                               (void **)&mFetchRowBuffer,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOC_FETCH_ROW_BUFFER );

            sIsAllocated = ID_TRUE;

            cleanFetchRowBuffer();
        }
        else
        {
            /* row size is '0' */
            mFetchRowBufSize = 0;
        }
    }
    else
    {
        /* already allocated */
        cleanFetchRowBuffer();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_FETCH_ROW_BUFFER );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAllocated == ID_TRUE )
    {
        (void)iduMemMgr::free( mFetchRowBuffer );
        mFetchRowBuffer = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : fetch row buffer 를 메모리에서 해제한다. 
 *
 ************************************************************************/
void    dkdRemoteTableMgr::freeFetchRowBuffer()
{
    if ( mFetchRowBuffer != NULL )
    {
        (void)iduMemMgr::free( mFetchRowBuffer );

        mFetchRowBuffer = NULL;
        mInsertPos      = NULL;
        mCurRow         = NULL;
    }
    else
    {
        /* do nothing */
    }
}

/************************************************************************
 * Description : converted row buffer 를 메모리에서 해제한다. 
 *
 ************************************************************************/
void    dkdRemoteTableMgr::freeConvertedRowBuffer()
{
    if ( mConvertedRowBuffer != NULL )
    {
        (void)iduMemMgr::free( mConvertedRowBuffer );

        mConvertedRowBuffer  = NULL;
        mConvertedRowBufSize = 0;
    }
    else
    {
        /* do nothing */
    }
}
/************************************************************************
 * Description : 원격서버로부터 fetch 해온 레코드를 insert 하기 위해 
 *               일시적으로 보관할 버퍼를 할당한다.
 *
 ************************************************************************/
void    dkdRemoteTableMgr::cleanFetchRowBuffer()
{
    if ( mFetchRowBuffer != NULL )
    {
        idlOS::memset( mFetchRowBuffer, 0, mFetchRowBufSize );
    
        mInsertRowCnt      = 0;
        mIsFetchBufFull    = ID_FALSE;
        mIsFetchBufReadAll = ID_FALSE;
        mInsertPos         = mFetchRowBuffer;
    }
    else
    {
        /* do nothing */
    }
}

/************************************************************************
 * Description : Remote table manager 를 정리한다.
 *
 ************************************************************************/
void dkdRemoteTableMgr::finalize()
{
    freeFetchRowBuffer();
    freeConvertedRowBuffer();

    if ( mTypeConverter != NULL )
    {
        (void)destroyTypeConverter();
    }
    else
    {
        /* do nothing */
    }
}

/************************************************************************
 * Description : Type converter 를 생성한다. Type converter 는 result
 *               set meta 정보를 갖고 있다. 
 *
 ************************************************************************/
IDE_RC  dkdRemoteTableMgr::createTypeConverter( dkpColumn   *aColMetaArr, 
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
IDE_RC  dkdRemoteTableMgr::destroyTypeConverter()
{
    return dkdTypeConverterDestroy( mTypeConverter );
}

/************************************************************************
 * Description : Type converter 가 altibase type 으로 변환해 갖고 있는 
 *               meta 정보를 요청한다.
 *
 *  aMeta       - [IN] 요청한 meta 정보가 담길 구조체 포인터
 *
 ************************************************************************/
IDE_RC  dkdRemoteTableMgr::getConvertedMeta( mtcColumn **aMeta )
{
    IDE_TEST( dkdTypeConverterGetConvertedMeta( mTypeConverter, aMeta )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Fetch row buffer 로부터 record 하나를 fetch 해온다. 
 *
 *  aRow        - [OUT] fetch 해올 row 를 가리키는 포인터
 *
 ************************************************************************/
void    dkdRemoteTableMgr::fetchRow( void  **aRow )
{
    /* fetch row */
    if ( mCurRow != NULL )
    {
        idlOS::memcpy( *aRow, (void *)mCurRow, mRowSize );

        if ( mInsertRowCnt > 0 )
        {
            mInsertRowCnt--;
            mCurRow += mRowSize;
        }
        else
        {
            /* insert row count is 0 */
            if ( mIsEndOfFetch != ID_TRUE )
            {
                /* remote data remained */
            }
            else
            {
                /* no more record */
                mCurRow = NULL;
                *aRow   = NULL;
            }

            mIsFetchBufReadAll = ID_TRUE;
        }
    }
    else
    {
        /* no more rows exists, success */
        *aRow = NULL;
    }
}

/************************************************************************
 * Description : REMOTE_TABLE 용 fetch row buffer 로 record 하나를 
 *               insert 한다. 이 때, 입력받는 row 는 cm block 으로부터 
 *               얻어온 raw data 로 mt type 으로 변환한 후 insert 한다.
 *
 *  aRow        - [IN] insert 할 record 를 가리키는 포인터
 *
 ************************************************************************/
IDE_RC  dkdRemoteTableMgr::insertRow( void *aRow )
{ 
    IDE_DASSERT ( mInsertRowCnt < mFetchRowCnt );   

    if ( aRow != NULL )
    {
        IDE_TEST( dkdTypeConverterConvertRecord( mTypeConverter, 
                                                 aRow, 
                                                 (void *)mInsertPos ) 
                  != IDE_SUCCESS );

        mInsertRowCnt++;
        mRecordCnt++;

        if ( mInsertRowCnt != mFetchRowCnt )
        {
            mInsertPos += mRowSize;
        }
        else
        {
            mIsFetchBufFull = ID_TRUE;
            mInsertPos      = mFetchRowBuffer;
        }
    }
    else
    {
        /* NULL insertion represents the end of fetch */
        mIsEndOfFetch = ID_TRUE;    
        mInsertPos    = mFetchRowBuffer;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Record buffer 의 iteration 을 restart 시킨다.
 *
 ************************************************************************/
IDE_RC  dkdRemoteTableMgr::restart()
{    
    IDE_TEST( makeFetchRowBuffer() != IDE_SUCCESS );

    mIsEndOfFetch = ID_FALSE;
    mInsertRowCnt = 0;
    mRecordCnt    = 0;
    mCurRow       = mFetchRowBuffer;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

