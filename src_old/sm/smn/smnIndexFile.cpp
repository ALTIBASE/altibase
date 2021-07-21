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
 * $Id: smnIndexFile.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idu.h>
#include <smErrorCode.h>
#include <smuUtility.h>
#include <smr.h>
#include <smm.h>
#include <smp.h>
#include <smnReq.h>
#include <smnIndexFile.h>

#define IDE_INDEX_FILE_BUFFER (2 * idlOS::getpagesize())

smnIndexFile::smnIndexFile()
{

}

smnIndexFile::~smnIndexFile()
{

}

IDE_RC smnIndexFile::initialize(smOID a_oidTable, UInt a_nOffset)
{
    SChar s_indexFileName[SM_MAX_FILE_NAME];
    
    mBufferPtr = NULL;
    m_pAlignedBuffer    = NULL;

    m_cItemInBuffer = 0;
    m_nOffset = 0;
    m_cLeftInBuffer = 0;
    m_cReadItemInBuffer = 0;
    
    m_bEnd = ID_FALSE;
       
    //To fix BUG-5049 
    idlOS::snprintf(s_indexFileName, SM_MAX_FILE_NAME,
                    "%s%cALTIBASE_INDEX_%s_%"ID_vULONG_FMT"_%"ID_UINT32_FMT"",
                    //  smmManager::mDbDir[0],
                    smuProperty::getDBDir(0),
                    IDL_FILE_SEPARATOR, 
                    smmDatabase::getDicMemBase()->mDBname,
                    a_oidTable,
                    a_nOffset);
    
    m_cMaxItemInBuffer = IDE_INDEX_FILE_BUFFER / ID_SIZEOF(smOID);

    IDE_TEST(m_file.initialize(IDU_MEM_SM_SMN,
                               1, /* Max Open FD Count */
                               IDU_FIO_STAT_OFF,
                               IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);
    
    IDE_TEST(m_file.setFileName(s_indexFileName) != IDE_SUCCESS);
    
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SMN,
                                           IDE_INDEX_FILE_BUFFER,
                                           (void**)&mBufferPtr,
                                           (void**)&m_pAlignedBuffer )
             != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnIndexFile::destroy()
{
    IDE_TEST(iduMemMgr::free(mBufferPtr)
             != IDE_SUCCESS);
    mBufferPtr = NULL;

    IDE_TEST(m_file.destroy() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnIndexFile::create()
{
    IDE_TEST( m_file.createUntilSuccess( smLayerCallback::setEmergency ) != IDE_SUCCESS );
    
    //====================================================================
    // To Fix BUG-13924
    //====================================================================
    
    ideLog::log(SM_TRC_LOG_LEVEL_MINDEX,
                SM_TRC_MINDEX_FILE_CREATE,
                m_file.getFileName());
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnIndexFile::open()
{
    IDE_TEST(m_file.open() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnIndexFile::close()
{
    IDE_TEST(m_file.close() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnIndexFile::write(SChar *a_pRow, idBool a_bFinal)
{
   
    if(a_pRow != NULL)
    {
        /* BUG-40688 
         * persistent index를 저장할때 free된 row는 저장하지않는다 */
        if ( SM_SCN_IS_FREE_ROW( ((smpSlotHeader *)a_pRow)->mCreateSCN ) == ID_FALSE )
        {
            *(m_pAlignedBuffer + m_cItemInBuffer) = SMP_SLOT_GET_OID( a_pRow );

            m_cItemInBuffer++;
        }
        else
        {
            /* nothing to do */
        }
    }
    
    if(m_cItemInBuffer == m_cMaxItemInBuffer)
    {
        //write oid list into file
        IDE_TEST( m_file.writeUntilSuccess( NULL, /* idvSQL* */
                                            m_nOffset,
                                            m_pAlignedBuffer,
                                            (size_t)m_cItemInBuffer * ID_SIZEOF(smOID),
                                            smLayerCallback::setEmergency )
                  != IDE_SUCCESS );

        m_nOffset += m_cItemInBuffer * ID_SIZEOF(smOID);
        m_cItemInBuffer = 0;
    }

    if(a_bFinal == ID_TRUE && m_cItemInBuffer != 0)
    {
        //write oid list into file
        IDE_TEST( m_file.writeUntilSuccess( NULL, /* idvSQL* */
                                            m_nOffset,
                                            m_pAlignedBuffer,
                                            (size_t)m_cItemInBuffer * ID_SIZEOF(smOID),
                                            smLayerCallback::setEmergency )
                  != IDE_SUCCESS );

        m_nOffset += m_cItemInBuffer * ID_SIZEOF(smOID);
        m_cItemInBuffer = 0;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnIndexFile::read                         *
 * ------------------------------------------------------------------*
 * (BUG-40688 : 이전 함수 구현이 복잡해서 다시 작성함.)
 *
 * aReadWantCnt 만큼의 OID를 persistent 파일로부터 읽은후
 * row pointer로 변환해 aReadBuffer로 저장한다.
 *
 * 실제 읽은 OID 갯수는 aReadCnt에 설정된다.
 * 일반적으로 aReadCnt 값은 aReadWantCnt와 동일한 값이 설정되고
 * 만약 0이나 ReadWantCnt 보다 작은값이 세팅된 경우는 파일 끝까지
 * 모두 읽었음을 의미한다.
 *
 * 읽지않고 남아있는 OID는 smnIndexFIle의 buffer (m_pAlignedBuffer)에
 * 남아있어 이 함수가 다시 호출될때 이용한다.
 *
 * < parameter 정보 > 
 *
 * aSpaceID     - [IN]  table space ID
 * aReadBuffer  - [IN]  읽은 row pointer를 저장할 버퍼
 *                      ( smIndexFIle의 buffer에 있는 OID를
 *                        row pointer로 변환하여 저장하게 된다. )
 * aReadWantCnt - [IN]  읽고자 하는 OID 갯수.
 * aReadCnt     - [OUT] 읽어진 OID 갯수.
 *                      : 보통 aReadWantCnt 값이 세팅됨.
 *                      : 그보다 작은값이나 0이 세팅되는 경우는
 *                        마지막이어서 더이상 OID가 없다는 의미이다.
 *
 * < 추가정보 >
 * m_pAlignedBuffer    : smnIndexFile의 buffer
 * m_cItemInBuffer     : smnINdexFile의 buffer에 있는 전체 OID 갯수
 * m_cLeftInBuffer     : smnIndexFile의 buffer에 읽지않고 남아있는 OID 갯수 
 * m_cReadItemInBuffer : smnIndexFile의 buffer에서 읽은 OID 갯수
 *
 *********************************************************************/
IDE_RC smnIndexFile::read( scSpaceID    aSpaceID,
                           SChar      * aReadBuffer[],
                           UInt         aReadWantCnt,
                           UInt       * aReadCnt )
{
    UInt    sReadCnt        = 0;
    size_t  sReadBufferSize = 0;
    SChar * sRowPtr         = NULL;

    while ( sReadCnt < aReadWantCnt )
    {
        /* smnIndexFile의 buffer에 OID가 남아있으면
         * OID => row pointer로 변환해서 aReadBuffer로 복사한다. */
        if ( m_cLeftInBuffer > 0 )
        {
            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                               *(m_pAlignedBuffer + m_cReadItemInBuffer),
                                               (void **)&sRowPtr )
                        == IDE_SUCCESS );

            if ( SM_SCN_IS_FREE_ROW( ((smpSlotHeader *)sRowPtr)->mCreateSCN ) == ID_FALSE )
            {
                aReadBuffer[sReadCnt] = sRowPtr;

                sReadCnt++;
            }
            else
            {
                /* free된 row이므로 skip 한다. */
            }

            m_cLeftInBuffer--;
            m_cReadItemInBuffer++;

            continue;
        }
        else
        {
            /* nothing to do */
        }

        /*
         * 여기에 도착한다면, 
         * smnIndexFile의 buffer가 비어있거나 모두 사용한 경우이다.
         *
         * 아래 코드에서는
         * persistent 파일의 다음 내용을 읽어서 smnIndexFile의 buffer에 세팅한다.
         */

        if ( m_bEnd == ID_FALSE )
        {
            /* 파일 읽어서 smnIndexFile의 buffer에 넣기 */
            IDE_TEST( m_file.read( NULL, /* idvSQL* */
                                   m_nOffset,
                                   m_pAlignedBuffer,
                                   IDE_INDEX_FILE_BUFFER,
                                   (size_t *)(&sReadBufferSize) )
                      != IDE_SUCCESS );
            m_nOffset += sReadBufferSize;

            if ( sReadBufferSize != (size_t)IDE_INDEX_FILE_BUFFER )
            {
                /* 파일의 끝까지 모두 읽었음을 표시함. */
                m_bEnd = ID_TRUE;
            }

            m_cReadItemInBuffer  = 0;
            m_cLeftInBuffer      = sReadBufferSize / ID_SIZEOF(smOID);
            m_cItemInBuffer      = m_cLeftInBuffer;
        }
        else
        {
            /* 파일을 모두 읽었으므로 종료함. */
            break;
        }
    }

    *aReadCnt = sReadCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnIndexFile::dump( SChar * aName, idBool aChangeEndian )
{
    UInt   sState       = 0;
    ULong  sOID;
    idBool sEnd         = ID_FALSE;
    size_t sReadSize    = 0;
    UInt   sOffset      = 0;
    UInt   sRecordCount = 0;
    UInt   i;
    UInt   j;

    IDE_TEST( m_file.initialize( IDU_MEM_SM_SMN,
                                 1, /* Max Open FD Count */
                                 IDU_FIO_STAT_OFF,
                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    // fix BUG-25544 [CodeSonar::DoubleClose]
    sState = 1;

    IDE_TEST( m_file.setFileName( aName ) != IDE_SUCCESS );
    IDE_TEST_RAISE( m_file.open() != IDE_SUCCESS , err_open_fail );
    sState = 2;

    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SMN,
                                           IDE_INDEX_FILE_BUFFER,
                                           (void**)&mBufferPtr,
                                           (void**)&m_pAlignedBuffer )
             != IDE_SUCCESS);

    sState = 3;



    sEnd         = ID_FALSE;
    sReadSize    = 0;
    sOffset      = 0;
    sRecordCount = 0;

    while( !sEnd )
    {
        IDE_TEST(m_file.read( NULL, /* idvSQL* */
                              sOffset,
                              m_pAlignedBuffer,
                              IDE_INDEX_FILE_BUFFER,
                              (size_t*)(&sReadSize))
                 != IDE_SUCCESS);
        sOffset += sReadSize;

        if(sReadSize < (size_t)IDE_INDEX_FILE_BUFFER )
        {
            sEnd = ID_TRUE;
        }
        else
        {
            // nothing to do...
        }

        for( i = 0, j = 0 ; i < sReadSize ; i += ID_SIZEOF( smOID ), j ++ )
        {
            sOID = m_pAlignedBuffer[ j ];

            if( aChangeEndian == ID_TRUE )
            {
                sOID = ( sOID >> 32 ) + ( (sOID & 0xFFFFFFFF ) << 32 );
            }
            else
            {
                // nothing to do...
            }
            idlOS::printf( "%16"ID_xINT64_FMT" "
                           "[ %10"ID_UINT32_FMT": %10"ID_UINT32_FMT"]\n",
                           sOID,
                           sOID >> SM_OFFSET_BIT_SIZE,
                           sOID & SM_OFFSET_MASK );
            sRecordCount ++;
        }
    }
    idlOS::printf("RecordCount:%"ID_UINT32_FMT"\n",sRecordCount );

    // fix BUG-25544 [CodeSonar::DoubleClose]
    sState = 2;
    IDE_TEST(iduMemMgr::free(mBufferPtr)
             != IDE_SUCCESS);
    mBufferPtr = NULL;
    sState = 1;
    IDE_TEST_RAISE( m_file.close()!= IDE_SUCCESS, err_close_fail );
    sState = 0;
    IDE_TEST( m_file.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_open_fail );
    {
        idlOS::printf( "file open error\n" );
    }
    IDE_EXCEPTION( err_close_fail );
    {
        idlOS::printf( "file close error\n" );
    }
    IDE_EXCEPTION_END;

    // fix BUG-25544 [CodeSonar::DoubleClose]
    switch( sState )
    {
    case 3:
        IDE_ASSERT(iduMemMgr::free(mBufferPtr) == IDE_SUCCESS);
    case 2:
        IDE_ASSERT( m_file.close()             == IDE_SUCCESS );
    case 1:
        IDE_ASSERT( m_file.destroy()           == IDE_SUCCESS );
    default :
        break;
    }
    return IDE_FAILURE;
}
