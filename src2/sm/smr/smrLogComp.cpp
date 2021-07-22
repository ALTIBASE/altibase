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
 * $Id:$
 **********************************************************************/

#include <iduLZ4.h>

#include <smErrorCode.h>
#include <smrLogComp.h>
#include <smrLogHeadI.h>
#include <sctTableSpaceMgr.h>

/* 로그파일의 특정 Offset에서 로그 레코드를 읽어온다.
   압축된 로그의 경우, 로그 압축해제를 수행한다.

   [IN] aDecompBufferHandle   - 압축 해제 버퍼의 핸들
   [IN] aLogFile            - 로그를 읽어올 로그파일
   [IN] aLogOffset          - 로그를 읽어올 오프셋
   [OUT] aRawLogHead        - 로그의 Head
   [OUT] aRawLogPtr         - 읽어낸 로그 (압축해제된 로그)
   [OUT] aLogSizeAtDisk     - 파일에서 읽어낸 로그 데이터의 양
*/
IDE_RC smrLogComp::readLog( iduMemoryHandle    * aDecompBufferHandle,
                            smrLogFile         * aLogFile,
                            UInt                 aLogOffset,
                            smrLogHead         * aRawLogHead,
                            SChar             ** aRawLogPtr,
                            UInt               * aLogSizeAtDisk )
{
    // 비압축 로그를 읽는 경우 aDecompBufferHandle이 NULL로 들어온다
    IDE_DASSERT( aLogFile       != NULL );
    IDE_DASSERT( aRawLogHead    != NULL );
    IDE_DASSERT( aRawLogPtr     != NULL );
    IDE_DASSERT( aLogOffset < smuProperty::getLogFileSize() );
    IDE_DASSERT( aLogSizeAtDisk != NULL );

    smMagic     sValidLogMagic;
    SChar     * sRawOrCompLog;

    aLogFile->read(aLogOffset, &sRawOrCompLog);

    // File No와 Offset으로부터 Magic값 계산
    sValidLogMagic = smrLogFile::makeMagicNumber( aLogFile->getFileNo(),
                                                  aLogOffset );

    IDU_FIT_POINT( "BUG-48375@smrLogComp::readLog" );
    IDE_TEST( getRawLog( aDecompBufferHandle,
                         aLogOffset,
                         sRawOrCompLog,
                         sValidLogMagic,
                         aRawLogHead,
                         aRawLogPtr,
                         aLogSizeAtDisk ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-26695 log decompress size 불일치로 Recovery 실패합니다.
    IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_LOGFILE,
                              aLogFile->getFileName() ) );

    return IDE_FAILURE;
}

/* 압축된, 혹은 압축되지 않은 로그 레코드로부터
   압축되지 않은 형태의 Log Head와 Log Ptr을 가져온다.

   ( 압축된 로그의 경우, 로그 압축해제를 수행한다. )

   [IN] aDecompBufferHandle - 압축 해제 버퍼의 핸들
   [IN] aRawLogOffset       - 로그 Offset
   [IN] aRawOrCompLog       - 로그를 읽어올 오프셋
   [IN] aValidLogMagic      - 로그의 정상적인 Magic값
   [OUT] aRawLogHead        - 로그의 Head
   [OUT] aRawLogPtr         - 읽어낸 로그 (압축해제된 로그)
   [OUT] aLogSizeAtDisk     - 파일에서 읽어낸 로그 데이터의 양
*/
IDE_RC smrLogComp::getRawLog( iduMemoryHandle    * aDecompBufferHandle,
                              UInt                 aRawLogOffset,
                              SChar              * aRawOrCompLog,
                              smMagic              aValidLogMagic,
                              smrLogHead         * aRawLogHead,
                              SChar             ** aRawLogPtr,
                              UInt               * aLogSizeAtDisk )
{
    // 비압축 로그를 읽는 경우 aDecompBufferHandle이 NULL로 들어온다
    IDE_DASSERT( aRawOrCompLog  != NULL );
    IDE_DASSERT( aRawLogHead    != NULL );
    IDE_DASSERT( aRawLogPtr     != NULL );
    IDE_DASSERT( aLogSizeAtDisk != NULL );    

    smMagic      sMagicValue;
    smLSN        sLogLSN;
    smrLogHead   sInvalidLogHead;
    idBool       sIsValid = ID_TRUE;

    /* BUG-38962
     * Magic Number를 먼저 검사해서 invalid 여부를 판단한다.
     * invalid log는 무조건 비압축 로그로 취급한 후, 상위 모듈에서 오류 처리한다. 
     */
    idlOS::memcpy( &sMagicValue,
                   aRawOrCompLog + SMR_COMP_LOG_MAGIC_OFFSET,
                   SMR_COMP_LOG_MAGIC_SIZE );

    if ( aValidLogMagic == sMagicValue )
    {
        sIsValid = ID_TRUE;
    }
    else
    {
        sIsValid = ID_FALSE;
    }

    if( ( isCompressedLog( aRawOrCompLog ) == ID_TRUE ) &&
        ( sIsValid   == ID_TRUE ) /* BUG-39462 */ )
    {
        /* BUG-35392 */
        if( smrLogHeadI::isDummyLog( aRawOrCompLog ) == ID_FALSE )
        {
            IDE_TEST( decompressCompLog( aDecompBufferHandle,
                                        aRawLogOffset,
                                        aRawOrCompLog,
                                        aValidLogMagic,
                                        aRawLogHead,
                                        aRawLogPtr,
                                        aLogSizeAtDisk )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Dummy log이면 size만 return */
            *aRawLogPtr = aRawOrCompLog;

            /* Log Header를 복사한다.*/
            idlOS::memcpy(aRawLogHead, aRawOrCompLog, ID_SIZEOF(smrLogHead));

            idlOS::memcpy( (void*)&sLogLSN,
                           aRawOrCompLog + SMR_COMP_LOG_LSN_OFFSET,
                           SMR_COMP_LOG_LSN_SIZE );

            smrLogHeadI::setLSN(aRawLogHead, sLogLSN );

            *aLogSizeAtDisk = getCompressedLogSize( aRawOrCompLog );
            IDE_TEST_RAISE( *aLogSizeAtDisk > smuProperty::getLogFileSize(),
                            err_invalid_log );
        }
    }
    else
    {
        /* 압축된 로그가 아니다. 즉시 리턴. */
        *aRawLogPtr = aRawOrCompLog;

        /* Log Header를 복사한다.*/
        idlOS::memcpy(aRawLogHead, aRawOrCompLog, ID_SIZEOF(smrLogHead));

        /* sIsValid가 ID_FALSE가 있기 때문에 검사는 하지 않는다. */
        *aLogSizeAtDisk = smrLogHeadI::getSize( aRawLogHead );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_log );
    {
        idlOS::memcpy( &sInvalidLogHead,
                       aRawOrCompLog,
                       ID_SIZEOF(smrLogHead) );

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVER_INVALID_DECOMP_LOG_HEAD,
                     sInvalidLogHead.mFlag,
                     sInvalidLogHead.mType,
                     sInvalidLogHead.mMagic,
                     sInvalidLogHead.mSize,
                     sInvalidLogHead.mPrevUndoLSN.mFileNo,
                     sInvalidLogHead.mPrevUndoLSN.mOffset,
                     sInvalidLogHead.mTransID,
                     sInvalidLogHead.mReplSvPNumber );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 압축버퍼에 압축로그를 기록한다.

   원본로그의 압축된 크기를 알아내기 위해
   원본로그의 압축을 먼저 실시한다.

   [IN] aCompBufferHandle - 압축 버퍼의 핸들
   [IN] aRawLog           - 압축되기 전의 원본 로그
   [IN] aRawLogSize       - 압축되기 전의 원본 로그의 크기
   [IN] aCompLog          - 압축된 로그
   [IN] aCompLogSize      - 압축된 로그의 크기
 */
IDE_RC smrLogComp::createCompLog( iduMemoryHandle    * aCompBufferHandle,
                                  SChar              * aRawLog,
                                  UInt                 aRawLogSize,
                                  SChar             ** aCompLog,
                                  UInt               * aCompLogSize,
                                  smOID                aTableOID )
{
    IDE_DASSERT( aCompBufferHandle != NULL );
    IDE_DASSERT( aRawLog       != NULL );
    IDE_DASSERT( aRawLogSize    > 0  );
    IDE_DASSERT( aCompLog      != NULL );
    IDE_DASSERT( aCompLogSize  != NULL );

    SChar * sCompBuffer     = NULL;
    UInt    sCompBufferSize = 0;
    UInt    sCompressedRawLogSize;

    /* FIT/ART/sm/Bugs/BUG-31009/BUG-31009.tc */
    IDU_FIT_POINT( "1.BUG-31009@smrLogComp::createCompLog" );

    // ******************************************************
    // 압축을 위해 압축 버퍼 준비
    IDE_TEST( prepareCompBuffer( aCompBufferHandle,
                                 aRawLogSize,
                                 & sCompBuffer,
                                 & sCompBufferSize )
              != IDE_SUCCESS );

    // ******************************************************
    // 압축로그의 Body기록 ( 원본 로그를 압축한 데이터 )
    IDE_TEST( writeCompBody( sCompBuffer + SMR_COMP_LOG_HEAD_SIZE,
                             sCompBufferSize - SMR_COMP_LOG_OVERHEAD,
                             aRawLog,
                             aRawLogSize,
                             & sCompressedRawLogSize )
              != IDE_SUCCESS );

    // ******************************************************
    // 압축로그의 Head기록
    (void)writeCompHead( sCompBuffer,
                         aRawLog,
                         aRawLogSize,
                         sCompressedRawLogSize,
                         aTableOID );

    // ******************************************************
    // 압축로그의 Tail기록
    (void)writeCompTail( sCompBuffer +
                         SMR_COMP_LOG_HEAD_SIZE +
                         sCompressedRawLogSize,
                         aRawLogSize );

    *aCompLog     = sCompBuffer;
    *aCompLogSize = SMR_COMP_LOG_HEAD_SIZE +
                        sCompressedRawLogSize +
                    SMR_COMP_LOG_TAIL_SIZE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    원본로그를 압축한 데이터를 압축로그 Body로 기록한다.
    [IN] aCompDestPtr  - 압축로그 Body가 기록될 메모리 주소
    [IN] aCompDestSize - 압축로그 Body가 기록될 메모리의 크기
    [IN] aRawLog       - 원본로그
    [IN] aRawLogSize   - 원본로그 크기
    [OUT] aCompressedRawLogSize - 원본로그가 압축된 데이터의 크기
 */
IDE_RC smrLogComp::writeCompBody( SChar  * aCompDestPtr,
                                  UInt     aCompDestSize,
                                  SChar  * aRawLog,
                                  UInt     aRawLogSize,
                                  UInt   * aCompressedRawLogSize )
{
    IDE_DASSERT( aCompDestPtr != NULL );
    IDE_DASSERT( aCompDestSize > 0 );
    IDE_DASSERT( aRawLog      != NULL );
    IDE_DASSERT( aRawLogSize  > 0  );
    IDE_DASSERT( aCompressedRawLogSize != NULL );

    *aCompressedRawLogSize = iduLZ4_compress( aRawLog,       /* source */
                                              aCompDestPtr,  /* dest */
                                              aRawLogSize,   /* sourceSize */
                                              aCompDestSize, /* maxOutputSize */
                                              smuProperty::getLogCompAcceleration() );/* acceleration */

    IDE_ERROR_RAISE( *aCompressedRawLogSize > 0, err_fail_log_compress );
                     
    // 압축 데이터의 크기는 압축버퍼의 크기를 넘어설 수 없다.
    IDE_ERROR_RAISE( *aCompressedRawLogSize <= aCompDestSize,
                     err_fail_log_compress );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_fail_log_compress )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, 
                     "Log compress failed - invalid log size: %"ID_UINT32_FMT"", 
                     *aCompressedRawLogSize );

    }
    IDE_EXCEPTION_END;
    /* 로그 압축에 실패한 경우 예외가 아니라 그냥 비압축 로그를 사용 한다. */
    return IDE_FAILURE;
}

/*
    압축로그의 Head를 기록한다.

    [IN] aHeadDestPtr - 압축로그의 Head가 기록될 메모리 주소
    [IN] aRawLog      - 압축 이전의 원본 로그
    [IN] aRawLogSize  - 압축 이전의 원본 로그 크기
    [IN] aCompressedRawLogSize - 압축된 로그의 크기

 */
void smrLogComp::writeCompHead( SChar  * aHeadDestPtr,
                                SChar  * aRawLog,
                                UInt     aRawLogSize,
                                UInt     aCompressedRawLogSize,
                                smOID    aTableOID )
{
    IDE_DASSERT( aHeadDestPtr          != NULL );
    IDE_DASSERT( aRawLog               != NULL );
    IDE_DASSERT( aRawLogSize            > 0 );
    IDE_DASSERT( aCompressedRawLogSize  > 0 );

    UInt          sCompLogFlag  = 0;
#ifdef DEBUG
    UInt          sBufferOffset = 0;
#endif
    smrLogHead  * sRawLogHead = (smrLogHead *) aRawLog ;

    /* 4 byte Flag */
    {
        IDE_DASSERT( ID_SIZEOF( sCompLogFlag ) == SMR_COMP_LOG_FLAG_SIZE );

        sCompLogFlag = smrLogHeadI::getFlag( sRawLogHead );

        IDE_ASSERT( ( sCompLogFlag & SMR_LOG_COMPRESSED_MASK )
                    == SMR_LOG_COMPRESSED_NO );

        sCompLogFlag |= SMR_LOG_COMPRESSED_OK;

        /* smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
        smrLogHeadI::setFlag( (smrLogHead *)aHeadDestPtr, sCompLogFlag );
#ifdef DEBUG
        sBufferOffset += SMR_COMP_LOG_FLAG_SIZE;
#endif
    }

    /* 4 bytes  압축된 로그크기 */
    {
        IDE_DASSERT( ID_SIZEOF( aCompressedRawLogSize ) == SMR_COMP_LOG_COMP_SIZE );

        /* BUG-35392
         * 압축된 로그 크기 + head,tail의 크기를 더해 전체 압축로그 크기 저장 */
        aCompressedRawLogSize += SMR_COMP_LOG_OVERHEAD;
        /* smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
        smrLogHeadI::setSize( (smrLogHead *)aHeadDestPtr, aCompressedRawLogSize );
#ifdef DEBUG
        sBufferOffset += SMR_COMP_LOG_COMP_SIZE;
#endif
    }

    /* 8 bytes LSN */
    {
        IDE_DASSERT( sBufferOffset == SMR_COMP_LOG_LSN_OFFSET );
        /* 여기서는 0으로 세팅하고 추후 로그 기록시에
         * 로그 끝단의 Mutex를 잡은 상태로 LSN을 따서 기록한다. */
        /* smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
        SM_LSN_INIT( ((smrLogHead *)aHeadDestPtr)->mLSN );
#ifdef DEBUG
        sBufferOffset += SMR_COMP_LOG_LSN_SIZE;
#endif
    }

    /* 2 bytes Magic Number */
    {
        IDE_DASSERT( sBufferOffset == SMR_COMP_LOG_MAGIC_OFFSET );
        /* 여기서는 0으로 세팅하고 추후 로그 기록시에
         * 로그 끝단의 Mutex를 잡은 상태로
         * 로그가 기록될 LSN을 따서 Magic을 계산하고 기록한다. */
        /* smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
         smrLogHeadI::setMagic( (smrLogHead *)aHeadDestPtr, 0 );
#ifdef DEBUG
        sBufferOffset += SMR_COMP_LOG_MAGIC_SIZE;
#endif
    }

    /* 4 bytes  원본로그크기 */
    {
        IDE_DASSERT( ID_SIZEOF( aRawLogSize ) == SMR_COMP_LOG_DECOMP_SIZE );

        /* 4Byte align 안맞음.memcpy 필요  */
        idlOS::memcpy( aHeadDestPtr + SMR_COMP_LOG_DECOMP_OFFSET,
                       & aRawLogSize,
                       SMR_COMP_LOG_DECOMP_SIZE );
#ifdef DEBUG
        sBufferOffset += SMR_COMP_LOG_DECOMP_SIZE;
#endif
    }

    /* 8 bytes  TableOID */
    {
        idlOS::memcpy( aHeadDestPtr + SMR_COMP_LOG_TABLEOID_OFFSET,
                       &aTableOID,
                       SMR_COMP_LOG_TABLEOID_SIZE );
#ifdef DEBUG
        sBufferOffset += SMR_COMP_LOG_TABLEOID_SIZE;
#endif
    }

   IDE_DASSERT( sBufferOffset == SMR_COMP_LOG_HEAD_SIZE );
}


/*
   압축로그의 Tail을 기록한다.

   압축로그의 Tail의 구조는 다음과 같다

       [ 4 bytes ] 원본 로그의 크기

   [IN] aTailDestPtr      - 압축로그의 Tail이 기록될 위치
   [IN] aRawLogSize       - 원본 로그의 크기
 */
void smrLogComp::writeCompTail( SChar * aTailDestPtr,
                                  UInt    aRawLogSize )
{
    IDE_DASSERT( aTailDestPtr  != NULL );
    IDE_DASSERT( aRawLogSize  > 0 );

    idlOS::memcpy( aTailDestPtr,
                   & aRawLogSize,
                   ID_SIZEOF( aRawLogSize ) );

    IDE_ASSERT( ID_SIZEOF( aRawLogSize ) == SMR_COMP_LOG_TAIL_SIZE );
}

/*
   새로 기록할 압축된 로그를 위한 메모리 공간 준비

   [IN] aCompBufferHandle - 압축 버퍼의 핸들
   [IN] aRawLogSize       - 압축되기 전의 원본 로그의 크기
   [OUT] aCompBuffer      - 준비된 압축 버퍼 메모리
   [OUT] aCompBufferSize  - 준비된 압축 버퍼의 크기
 */
IDE_RC smrLogComp::prepareCompBuffer( iduMemoryHandle    * aCompBufferHandle,
                                      UInt                 aRawLogSize,
                                      SChar             ** aCompBuffer,
                                      UInt               * aCompBufferSize)
{
    IDE_DASSERT( aCompBufferHandle != NULL );
    IDE_DASSERT( aRawLogSize > 0 );
    IDE_DASSERT( aCompBuffer != NULL );
    IDE_DASSERT( aCompBufferSize != NULL );

    UInt sMaxCompLogSize =
             SMR_COMP_LOG_OVERHEAD + IDU_LZ4_COMPRESSBOUND(aRawLogSize);

    IDE_TEST( aCompBufferHandle->prepareMemory( sMaxCompLogSize,
                                                (void**)aCompBuffer )
              != IDE_SUCCESS );

    * aCompBufferSize = sMaxCompLogSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   압축해제를 위한 메모리 공간 준비

   [IN] aDecompBufferHandle - 압축 해제 버퍼의 핸들
   [IN] aRawLogSize       - 압축되기 전의 원본 로그의 크기
   [OUT] aDecompBuffer      - 준비된 압축 해제 버퍼 메모리
   [OUT] aDecompBufferSize  - 준비된 압축 해제 버퍼의 크기
 */
IDE_RC smrLogComp::prepareDecompBuffer(
                      iduMemoryHandle    * aDecompBufferHandle,
                      UInt                 aRawLogSize,
                      SChar             ** aDecompBuffer,
                      UInt               * aDecompBufferSize )
{
    IDE_DASSERT( aDecompBufferHandle != NULL );
    IDE_DASSERT( aRawLogSize > 0 );
    IDE_DASSERT( aDecompBuffer != NULL );
    IDE_DASSERT( aDecompBufferSize != NULL );

    IDE_TEST( aDecompBufferHandle->prepareMemory( aRawLogSize,
                                                  (void**)aDecompBuffer )
              != IDE_SUCCESS );

    * aDecompBufferSize = aRawLogSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 압축된 로그의 압축해제를 수행한다

   [IN] aDecompBufferHandle - 압축해제 버퍼의 핸들
   [IN] aCompLogOffset      - Compress된 로그의 Offset
   [IN] aValidLogMagic      - 로그의 정상적인 Magic값
   [IN] aCompLog            - 압축된 로그
   [OUT] aRawLog            - 압축해제된 로그
   [OUT] aMagicValue        - 로그의 Magic값
   [OUT] aLogLSN             - 로그의 일련번호
   [OUT] aCompLogSize       - 압축로그의 전체크기 (Head+Body+Tail)
 */
IDE_RC smrLogComp::decompressLog( iduMemoryHandle    * aDecompBufferHandle,
                                  UInt                 aCompLogOffset,
                                  SChar              * aCompLog,
                                  smMagic              aValidLogMagic,
                                  SChar             ** aRawLog,
                                  smMagic            * aMagicValue,
                                  smLSN              * aLogLSN,
                                  UInt               * aCompLogSize )
{
    IDE_DASSERT( aDecompBufferHandle    != NULL );
    IDE_DASSERT( aCompLog               != NULL );
    IDE_DASSERT( aRawLog                != NULL );
    IDE_DASSERT( aCompLogSize           != NULL );

    SChar  * sDecompLogBuffer;
    UInt     sDecompLogBufferSize = 0;
    idBool   sIsValidLog;
    UInt     sRawLogSize;
    UInt     sCompressedRawLogSize;
    SInt     sDecompressedLogSize;

    // 압축로그 분석
    //   1. 압축 Head의 필드 읽기
    //   2. 압축 Tail을 통해 일부만 기록된 비정상 로그인지 판별
    IDE_TEST( analizeCompLog( aCompLog,
                              aCompLogOffset,
                              aValidLogMagic,
                              &sIsValidLog,
                              aMagicValue,
                              aLogLSN,
                              &sRawLogSize,
                              &sCompressedRawLogSize )
              != IDE_SUCCESS );

    if ( sIsValidLog == ID_TRUE )
    {
        // 압축 해제할 압축 해제 버퍼 준비
        IDE_TEST( prepareDecompBuffer( aDecompBufferHandle,
                                       sRawLogSize,
                                       & sDecompLogBuffer,
                                       & sDecompLogBufferSize )
                  != IDE_SUCCESS );
        // 압축해제 실시
        sDecompressedLogSize = iduLZ4_decompress(
                       aCompLog + SMR_COMP_LOG_HEAD_SIZE, /* const char * source */
                       sDecompLogBuffer,                          /* char * dest */
                       sCompressedRawLogSize,              /* int compressedSize */
                       sDecompLogBufferSize );        /* int maxDecompressedSize */

        IDE_ERROR_RAISE( sDecompressedLogSize > 0, err_fail_log_decompress );
        // 원본 로그의 크기와 압축 해제후 로그의 크기가 같아야 함
        IDE_ERROR_RAISE( sRawLogSize == (UInt)sDecompressedLogSize,
                          err_fail_log_decompress );

        *aRawLog      = sDecompLogBuffer;
        *aCompLogSize = SMR_COMP_LOG_HEAD_SIZE +
                        sCompressedRawLogSize +
                        SMR_COMP_LOG_TAIL_SIZE;
    }
    else
    {
        /* 압축 해제된 형태로 Invalid Log를 일부러 만들어 놓는다 */
        /* 이 함수의 호출자는 압축 해제된 로그의 Head와 Tail을 보고
           로그의 모든 내용이 온전히 기록된 Valid한 로그인지
           판별해야 한다 */
        IDE_TEST( createInvalidLog( aDecompBufferHandle,
                                    aRawLog )
                  != IDE_SUCCESS );

        // Invalid한 로그이므로, 원본 압축로그의 크기는 큰 의미가 없다.
        // 압축로그의 HEAD + TAIL의 크기를 리턴
        *aCompLogSize = SMR_COMP_LOG_HEAD_SIZE + SMR_COMP_LOG_TAIL_SIZE ;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_fail_log_decompress )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, 
                     "Log decompress failed - invalid log size: %"ID_UINT32_FMT"", 
                     sDecompressedLogSize );

        // BUG-26695 log decompress size 불일치로 Recovery 실패합니다.
        // Decompressed Log Size가 Log Head Size보다 더 클 경우에만
        // 잘못된 Log Head 정보라도 찍어주기 위해 Decompressed된
        // Log 의 Buffer Pointer를 넘겨줍니다.
        if( ID_SIZEOF(smrLogHead) <= (UInt)sDecompressedLogSize )
        {
            *aRawLog = sDecompLogBuffer;
        }
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 압축로그를 해석한다.

   [IN] aDeompBufferHandle     - 압축 해제 버퍼의 핸들
   [IN] aCompLogOffset         - 압축 로그 Offset
   [IN] aValidLogMagic         - 로그의 정상적인 Magic값

   [OUT] aIsValid              - 압축로그가 VALID한지 여부
   [OUT] aFlag                 - 압축로그의 Flag
   [OUT] aMagicValue           - 로그의 LSN으로 계산된 Magic Value
   [OUT] aLogLSN               - 로그의 일련번호
   [OUT] aRawLogSize           - 압축되기전 원본 로그의 크기
   [OUT] aCompressedRawLogSize - 원본로그를 압축한 데이터의 크기
*/
IDE_RC smrLogComp::analizeCompLog( SChar    * aCompLog,
                                   UInt       aCompLogOffset,
                                   smMagic    aValidLogMagic,
                                   idBool   * aIsValid,
                                   smMagic  * aMagicValue,
                                   smLSN    * aLogLSN,
                                   UInt     * aRawLogSize,
                                   UInt     * aCompressedRawLogSize )
{
    static UInt sLogFileSize    = smuProperty::getLogFileSize();
    UInt        sMaxLogSize;
    UInt        sCompLogTail;
    UInt        sLogFlag        = 0;

    IDE_ASSERT( aLogLSN     != NULL );
    IDE_ASSERT( aCompLog    != NULL );
    IDE_ASSERT( aIsValid    != NULL );
    IDE_ASSERT( aRawLogSize != NULL );
    IDE_ASSERT( aCompressedRawLogSize != NULL );

    *aIsValid    = ID_TRUE;
    // BUG-27331 CodeSonar::Uninitialized Variable
    SM_LSN_INIT( *aLogLSN );
    *aRawLogSize = 0;
    *aCompressedRawLogSize = 0;

    /* 4 byte Flag */
    {
        /* BUG-35392
         * 압축 로그에서 플래그를 읽을때는 align을 고려해야 한다.
         * memcpy로 읽어야 함 */
        idlOS::memcpy( &sLogFlag, aCompLog, SMR_COMP_LOG_FLAG_SIZE );

        IDE_ASSERT( ( sLogFlag & SMR_LOG_COMPRESSED_MASK ) ==
                    SMR_LOG_COMPRESSED_OK );
        aCompLog += SMR_COMP_LOG_FLAG_SIZE;
    }

    /* 4 bytes 압축 된 로그크기 */
    {
        idlOS::memcpy( aCompressedRawLogSize, aCompLog, SMR_COMP_LOG_COMP_SIZE );
        aCompLog += SMR_COMP_LOG_COMP_SIZE;

        /* BUG-35392
         * head,tail 크기를 빼 실제 압축로그 크기를 구함 */
        *aCompressedRawLogSize -= SMR_COMP_LOG_OVERHEAD;
    }

    /* 8 bytes Log의 LSN */
    {
        idlOS::memcpy( aLogLSN, aCompLog, SMR_COMP_LOG_LSN_SIZE );
        aCompLog += SMR_COMP_LOG_LSN_SIZE;
    }

    /* 2 bytes Log의 Magic Value */
    {
        idlOS::memcpy( aMagicValue, aCompLog, SMR_COMP_LOG_MAGIC_SIZE );
        aCompLog += SMR_COMP_LOG_MAGIC_SIZE;
    }

    /* Magic값이 Valid하지 않은 경우 Invalid로그로 설정*/
    IDE_TEST_CONT( aValidLogMagic != *aMagicValue, skip_invalid_log );

    /* 4 bytes 원본로그크기 */
    {
        idlOS::memcpy( aRawLogSize, aCompLog, SMR_COMP_LOG_DECOMP_SIZE );
        aCompLog += SMR_COMP_LOG_DECOMP_SIZE;
    }

    /* BUG-46944  CompLogHeader에 TableOID 추가 */
    aCompLog += SMR_COMP_LOG_TABLEOID_SIZE;

    IDE_ASSERT( sLogFileSize > aCompLogOffset );

    sMaxLogSize = sLogFileSize - aCompLogOffset;

    /* BUG-24162: [SC] LogSize가 무지하게 큰값으로 되어 있어 LogTail을 찾기위해
     *            이 값을 이용하다가 invalid한 memory영역을 접근하여 서버 사망.
     *
     * LogHeader의 LogSize가 Valid한지 검사한다. */
    IDE_TEST_CONT( *aCompressedRawLogSize > sMaxLogSize, skip_invalid_log );
    
    /* 4 bytes [Tail] 원본로그크기 */
    {
        idlOS::memcpy( &sCompLogTail,
                       aCompLog + *aCompressedRawLogSize,
                       SMR_COMP_LOG_TAIL_SIZE);
    }

    IDE_TEST_CONT( *aRawLogSize != sCompLogTail, skip_invalid_log );

    // Size가 0인 경우 Invalid로그로 간주
    //  이유 : smrLogComp.h에 기술된 압축 로그 Head에서
    //         Magic까지 기록이 되고 원본/압축 로그크기 이후로
    //         전혀 기록이 되지 않은 경우,
    //         원본 로그 크기, 압축로그의 Tail이 모두 0이고
    //         Magic 값도 맞아떨어지기 때문에 Valid한 로그로 인식됨.
    //         => Size가 0이면 Invalid Log로 간주
    IDE_TEST_CONT( *aRawLogSize == 0, skip_invalid_log );

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( skip_invalid_log );

    *aIsValid = ID_FALSE;

    return IDE_SUCCESS;
}


/*
   압축된 로그가 Invalid한 경우
   압축 해제 버퍼에 일부러 압축해제 된 형식으로 Invalid로그 기록 */
typedef struct smrInvalidLog
{
    smrLogHead    mHead;
    smrLogTail    mTail;
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrInvalidLog;

/* VALID하지 않은 로그를 압축되지 않은 형태로 생성한다.

   압축로그가 온전히 기록되지 못하고 일부만 기록된 경우,
   압축 해제 버퍼상에 INVALID한 로그를 일부러 기록한다.

   => Compression Transparency를 보장하기 위해 이와 같이
      압축되지 않은 형태의 INVALID한 로그를 기록해둔다.
      이후 로그를 읽는 모듈에서 압축되지 않은 로그의
      VALID여부를 판단하는 방식과 동일한 방식으로
      해당 로그가 VALID한지 검사한다.

   [IN] aDeompBufferHandle - 압축 해제 버퍼의 핸들
   [OUT] aInvalidRawLog    - INVALID 로그의 주소
 */
IDE_RC smrLogComp::createInvalidLog( iduMemoryHandle    * aDecompBufferHandle,
                                     SChar             ** aInvalidRawLog )
{
    IDE_DASSERT( aDecompBufferHandle != NULL );
    IDE_DASSERT( aInvalidRawLog != NULL );

    smrInvalidLog sInvalidLog;

    void * sLogBuffer;

    // Invalid Log를 기록할 메모리 공간 준비
    IDE_TEST( aDecompBufferHandle->prepareMemory( ID_SIZEOF( sInvalidLog ),
                                                  & sLogBuffer )
              != IDE_SUCCESS );

    // sInvalidLog의 내용을 Invalid한 로그로 세팅
    {
        idlOS::memset( & sInvalidLog, 0, ID_SIZEOF( sInvalidLog ) );

        // Invalid하게 인식되도록 Head의 Type(1)과 Tail(0)을 다르게 기록
        smrLogHeadI::setType( &sInvalidLog.mHead, 1 );
        sInvalidLog.mTail = 0;

        smrLogHeadI::setTransID( &sInvalidLog.mHead, SM_NULL_TID );
        smrLogHeadI::setSize( &sInvalidLog.mHead,
                              SMR_LOGREC_SIZE(smrInvalidLog) );

        smrLogHeadI::setFlag( &sInvalidLog.mHead,
                              SMR_LOG_TYPE_NORMAL );

        smrLogHeadI::setPrevLSN( &sInvalidLog.mHead,
                                 ID_UINT_MAX,  // FILENO
                                 ID_UINT_MAX ); // OFFSET

        smrLogHeadI::setReplStmtDepth( &sInvalidLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    idlOS::memcpy( sLogBuffer, & sInvalidLog, ID_SIZEOF( sInvalidLog ) );

    *aInvalidRawLog = (SChar *) sLogBuffer;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 압축 대상 로그인지 판별한다

   [IN] aRawLog     - 압축대상 로그인지를 판별할 로그 레코드
   [IN] aRawLogSize - 로그 레코드의 크기 ( Head + Body + Tail )
   [OUT] aDoCompLog - 로그 압축 여부
 */
IDE_RC smrLogComp::shouldLogBeCompressed( SChar  * aRawLog,
                                          UInt     aRawLogSize,
                                          idBool * aDoCompLog )
{
    IDE_DASSERT( aRawLog != NULL );
    IDE_DASSERT( aRawLogSize > 0 );
    IDE_DASSERT( aDoCompLog != NULL );


    scSpaceID    sSpaceID;
    smrLogHead * sLogHead;
    idBool       sDoComp; /* Log compress 여부 */


    sLogHead = (smrLogHead*)aRawLog;

    if ( smuProperty::getMinLogRecordSizeForCompress() == 0 )
    {
        // Log Compression이 Disable된 상태
        // Log Compression을 수행하지 않는다.
        sDoComp = ID_FALSE;
    }
    else
    {
        if ( aRawLogSize >= smuProperty::getMinLogRecordSizeForCompress() )
        {
            // Tablespace별 로그 압축여부 판단 필요
            getSpaceIDOfLog( sLogHead, aRawLog, & sSpaceID );

            if ( sSpaceID == SC_NULL_SPACEID )
            {
                // 특정 Tablespace관련 로그가 아닌 경우
                sDoComp = ID_TRUE; // 로그 압축 실시
            }
            else
            {
                IDE_TEST( sctTableSpaceMgr::getSpaceLogCompFlag( sSpaceID,
                                                                 &sDoComp )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            sDoComp = ID_FALSE;
        }

        switch( smrLogHeadI::getType( sLogHead ) )
        {
            // 로그파일의 맨 앞에 기록되는
            //  File Begin Log의 경우 압축하지 않는다.
            // 이유 :
            //     File의 첫번째 Log의 LSN을 읽는 작업을
            //     빠르게 수행하기 위함
            case SMR_LT_FILE_BEGIN :
            // 로그파일의 Offset범위 체크를 쉽게 할 수 있도록 하기 위함
            //    => Offset < (로그파일크기 - smrLogHead - smrLogTail)
            case SMR_LT_FILE_END :

            // 로직의 단순화를 위해 압축하지 않는 로그들
            case SMR_LT_CHKPT_BEGIN :
            case SMR_LT_CHKPT_END :

                sDoComp = ID_FALSE;
                break;

            default:
                break;

        }
    }

    // 로그 압축을 하지 않도록 Log Head에 설정된 경우
    // 로그를 압축하지 않는다.
    if ( ( smrLogHeadI::getFlag(sLogHead) & SMR_LOG_FORBID_COMPRESS_MASK )
         == SMR_LOG_FORBID_COMPRESS_OK )
    {
        sDoComp = ID_FALSE;
    }

    *aDoCompLog = sDoComp;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Log Record에 기록된 Tablespace ID를 리턴한다.

    [IN] aLogHead - Log의 Head
    [IN] aRawLog  - Log의 주소
    [OUT] aSpaceID - Tablespace의 ID
 */
void smrLogComp::getSpaceIDOfLog( smrLogHead * aLogHead,
                                  SChar      * aRawLog,
                                  scSpaceID  * aSpaceID )
{
    IDE_DASSERT( aLogHead != NULL );
    IDE_DASSERT( aRawLog != NULL );
    IDE_DASSERT( aSpaceID != NULL );

    scSpaceID sSpaceID;

    switch( smrLogHeadI::getType( aLogHead ) )
    {
        case SMR_LT_UPDATE :
            sSpaceID = SC_MAKE_SPACE(((smrUpdateLog*)aRawLog)->mGRID);
            break;

        case SMR_LT_COMPENSATION :
            sSpaceID = SC_MAKE_SPACE(((smrCMPSLog*)aRawLog)->mGRID);
            break;

        case SMR_LT_TBS_UPDATE :
            sSpaceID = ((smrTBSUptLog*)aRawLog)->mSpaceID;
            break;

        case SMR_LT_NTA :
            sSpaceID = ((smrNTALog*)aRawLog)->mSpaceID;
            break;

        case SMR_DLT_NTA :
        case SMR_DLT_REF_NTA :
        case SMR_DLT_REDOONLY :
        case SMR_DLT_UNDOABLE :
        case SMR_DLT_COMPENSATION :
            /*
               Disk Log의 경우 두 개 이상의 Tablespace에 대한
               로그가 기록될 수 있다. (Ex> LOB Column )

               Space ID를 NULL로 넘긴다.
            */
        default :
            /* 기본적으로 NULL SPACE ID를 넘긴다
             */
            sSpaceID = SC_NULL_SPACEID;
            break;

    }

    *aSpaceID = sSpaceID ;
}


/* 로그파일의 특정 Offset에서 로그 레코드를 읽어온다.
   압축된 로그라면 압축된 형태 그대로 반환한다.

   [IN] aLogFile            - 로그를 읽어올 로그파일
   [IN] aLogOffset          - 로그를 읽어올 오프셋
   [OUT] aRawLogHead        - 로그의 Head
   [OUT] aRawLogPtr         - 읽어낸 로그 (압축해제되지 않은 로그)
   [OUT] aLogSizeAtDisk     - 파일에서 읽어낸 로그 데이터의 양
*/
IDE_RC smrLogComp::readLog4RP( smrLogFile         * aLogFile,
                               UInt                 aLogOffset,
                               smrLogHead         * aRawLogHead,
                               SChar             ** aRawLogPtr,
                               UInt               * aLogSizeAtDisk )
{
    IDE_DASSERT( aLogFile       != NULL );
    IDE_DASSERT( aRawLogHead    != NULL );
    IDE_DASSERT( aRawLogPtr     != NULL );
    IDE_DASSERT( aLogOffset < smuProperty::getLogFileSize() );
    IDE_DASSERT( aLogSizeAtDisk != NULL );

    smrLogHead  sInvalidLogHead;
    SChar     * sRawOrCompLog;

    aLogFile->read(aLogOffset, &sRawOrCompLog);

    /* valid log 검사 하지 않는다. 상위 모듈에서 오류 처리한다. */
    *aRawLogPtr = sRawOrCompLog;

    // 압축을 풀지 않을 것이기 때문에 압축로그일 경우 Header 뒤쪽엔 쓰레기 값이 들어가 있을수 있다.
    // Magic Number 까지만 유효하다.
    /* Log Header를 복사한다.*/
    idlOS::memcpy(aRawLogHead, sRawOrCompLog, ID_SIZEOF(smrLogHead));

    // 압축을 풀지 않을 것이기 때문에 실제 disk 저장 Size만 반환하면 된다
    // 압축 로그와 비압축 로그 모두 Disk저장 Size는 같은 위치에 존재하기 때문에
    // 그냥 반환하면 된다.
    *aLogSizeAtDisk = smrLogHeadI::getSize( aRawLogHead );
    IDE_TEST_RAISE( *aLogSizeAtDisk > smuProperty::getLogFileSize(),
                    err_invalid_log );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_log );
    {
        idlOS::memcpy( &sInvalidLogHead,
                       sRawOrCompLog,
                       ID_SIZEOF(smrLogHead) );

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVER_INVALID_DECOMP_LOG_HEAD,
                     sInvalidLogHead.mFlag,
                     sInvalidLogHead.mType,
                     sInvalidLogHead.mMagic,
                     sInvalidLogHead.mSize,
                     sInvalidLogHead.mPrevUndoLSN.mFileNo,
                     sInvalidLogHead.mPrevUndoLSN.mOffset,
                     sInvalidLogHead.mTransID,
                     sInvalidLogHead.mReplSvPNumber );
    }
    IDE_EXCEPTION_END;

    // BUG-26695 log decompress size 불일치로 Recovery 실패합니다.
    IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_LOGFILE,
                              aLogFile->getFileName() ) );
    return IDE_FAILURE;
}

IDE_RC smrLogComp::decompressCompLog( iduMemoryHandle    * aDecompBufferHandle,
                                     UInt                 aCompLogOffset,
                                     SChar              * aCompLog,
                                     smMagic              aValidLogMagic,
                                     smrLogHead         * aRawLogHead,
                                     SChar             ** aRawLog,
                                     UInt               * aLogSizeAtDisk )
{
    UInt         sCompLogSize = 0;
    SChar      * sDecompLog   = NULL;
    smMagic      sMagicValue;
    smLSN        sLogLSN;
    smrLogHead   sInvalidLogHead;

    if( smrLogHeadI::isDummyLog( aCompLog ) == ID_FALSE )
    {

        /* 압축된 로그를 읽기 위해서는
         * 압축로그 버퍼 핸들을 인자로 넘겨야 함 */
        IDE_ASSERT( aDecompBufferHandle != NULL );

        /* 압축된 로그이다. 압축 해제후 리턴. */
        IDE_TEST_RAISE( decompressLog( aDecompBufferHandle,
                                       aCompLogOffset,
                                       aCompLog,
                                       aValidLogMagic,
                                       &sDecompLog,
                                       &sMagicValue,
                                       &sLogLSN,
                                       &sCompLogSize )
                        != IDE_SUCCESS, err_fail_log_decompress );

        *aRawLog = sDecompLog;

        /* Log Header를 복사한다.
           Log가 기록될때 Log의 크기가 align되지 않았기
           때문에 복사해서 관리한다.*/
        idlOS::memcpy(aRawLogHead, sDecompLog, ID_SIZEOF(smrLogHead));

        /* 압축 해제된 Log의 Head에 LSN과 Magic을 세팅
         * - 이유 : 압축된 compHead를 작성할때 LSN과 magicNo는 0으로 세팅함
         * smrLogComp.h의 <압축로그의 판독 절차> 참고 */
        smrLogHeadI::setMagic( aRawLogHead, sMagicValue );
        smrLogHeadI::setLSN( aRawLogHead, sLogLSN );

        /* 변경된 Log의 Head를 압축해제된 로그에 복사 */
        idlOS::memcpy( sDecompLog, aRawLogHead, ID_SIZEOF(smrLogHead) );

        *aLogSizeAtDisk = sCompLogSize;
        IDE_TEST_RAISE( *aLogSizeAtDisk > smuProperty::getLogFileSize(),
                        err_invalid_log );
    }
    else
    {
        /* Dummy log이면 size만 return */
        *aRawLog = aCompLog;

        /* Log Header를 복사한다.*/
        idlOS::memcpy(aRawLogHead, aCompLog, ID_SIZEOF(smrLogHead));

        idlOS::memcpy( (void*)&sLogLSN,
                       aCompLog + SMR_COMP_LOG_LSN_OFFSET,
                       SMR_COMP_LOG_LSN_SIZE );

        smrLogHeadI::setLSN(aRawLogHead, sLogLSN );

        *aLogSizeAtDisk = getCompressedLogSize( aCompLog );
        IDE_TEST_RAISE( *aLogSizeAtDisk > smuProperty::getLogFileSize(),
                        err_invalid_log );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_fail_log_decompress );
    {
        // BUG-26695 log decompress size 불일치로 Recovery 실패합니다.
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVER_INVALID_DECOMP_LOG_LSN,
                     sLogLSN.mFileNo,
                     sLogLSN.mOffset );

        if( sDecompLog != NULL )
        {
            // Decompressed Log Size가 Log Head Size보다 더 클 경우에만
            // 어떻게 잘못되었는지 알기위해 잘못된 Log Head 정보를 출력
            idlOS::memcpy( &sInvalidLogHead,
                           sDecompLog,
                           ID_SIZEOF(smrLogHead));

            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_MRECOVER_INVALID_DECOMP_LOG_HEAD,
                         sInvalidLogHead.mFlag,
                         sInvalidLogHead.mType,
                         sInvalidLogHead.mMagic,
                         sInvalidLogHead.mSize,
                         sInvalidLogHead.mPrevUndoLSN.mFileNo,
                         sInvalidLogHead.mPrevUndoLSN.mOffset,
                         sInvalidLogHead.mTransID,
                         sInvalidLogHead.mReplSvPNumber );
        }
    }

    IDE_EXCEPTION( err_invalid_log );
    {
        idlOS::memcpy( &sInvalidLogHead,
                       aCompLog,
                       ID_SIZEOF(smrLogHead) );

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVER_INVALID_DECOMP_LOG_HEAD,
                     sInvalidLogHead.mFlag,
                     sInvalidLogHead.mType,
                     sInvalidLogHead.mMagic,
                     sInvalidLogHead.mSize,
                     sInvalidLogHead.mPrevUndoLSN.mFileNo,
                     sInvalidLogHead.mPrevUndoLSN.mOffset,
                     sInvalidLogHead.mTransID,
                     sInvalidLogHead.mReplSvPNumber );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
