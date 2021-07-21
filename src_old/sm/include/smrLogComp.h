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
 * $Id: smrLogComp.h 15368 2006-03-23 01:14:43Z kmkim $
 **********************************************************************/

#ifndef _O_SMR_LOG_COMP_H_
#define _O_SMR_LOG_COMP_H_ 1

#include <idl.h>
#include <iduMemoryHandle.h>

#include <smDef.h>
#include <smrLogFile.h>

/*
   Log Compressor/Decompressor

   본 클래스는 로그의 압축과 해제를 담당한다.

   Log File Group단위로 로그를 읽고 쓰는 smrLogMgr과
   실제 로그레코드를 파일로부터 읽고 쓰는 smrLogFile의 중간에서
   로그의 압축과 해제를 담당한다.
   
   * 로그의 압축 과정
     - smrLogMgr은 tryLogCompression에서 압축할 로그의 내용을
       smrLogComp에 전달한다.
     - 압축된 로그를 이용하여 lockAndWriteLog 에서 로그를
       로그파일에 기록한다.
   |  ---------------------------------------------- 
   V          smrLogMgr::writeLog
   |  ----------------------------------------------
              smrLogMgr::tryLogCompression
   V          smrLogMgr::lockAndWriteLog
   |  ----------------------------------------------
   V          smrLogFile::writeLog
      ----------------------------------------------
   
   * 로그의 압축해제 과정
     - 최하위 Layer인 smrLogFile로부터 하나의 로그를 읽는다.
     - smrLogComp에서는 압축된 로그의 압축을 해제하여
       압축해제된 로그의 포인터를 리턴한다.
     - smrLogMgr은 이를 받아서 그대로 전달한다.
      ---------------------------------------------- 
   ^         smrLogMgr::readLog
      ---------------------------------------------- 
   ^         smrLogMgr::readLog
   |  ----------------------------------------------
   ^         smrLogComp::readLog
   |  ----------------------------------------------
   ^         smrLogFile::readLog
   |  ----------------------------------------------

    * BUG-35392 적용을 위해 Flag는 1->4 byte로 변경 되었다.
   * 압축된 로그의 포맷
      +---------+----------------+-----------------------------------------+
      | 4 byte  | 압축로그 Flag  | 0x80 비트를 1로 세팅한                  |
      |         |                | 원본로그의 Flag                         |
      +---------+----------------+-----------------------------------------+
      | 4 bytes | 압축로그크기   | 원본로그의 압축된 로그의 크기           |
      +---------+----------------+-----------------------------------------+
      | 8 bytes | LSN            | 로그의 Serial Number(mFileNo + mOffset) |
      +---------+----------------+-----------------------------------------+
      | 2 bytes | Magic          | 로그의 Magic Value                      |
      +---------+----------------+-----------------------------------------+
      | 4 bytes | 원본로그크기   | 원본 로그의 크기                        |
      +---------+----------------+-----------------------------------------+
      | N bytes | 압축로그       | 압축된 로그 데이터                      |
      +---------+----------------+-----------------------------------------+
      | 4 bytes | 로그 Tail      | 일부만 기록된 로그 판별을 위한 Tail     |
      |         |                | 원본 로그의 크기가 기록됨               |
      +---------+----------------+-----------------------------------------+

   * 압축로그의 기록 절차
   
     - 로그의 SN과 Magic이 기록되지 않은 원본로그를 압축한다
       - 최적의 성능을 낼 수 있도록, 로그 끝단의 Mutex를 잡지
         않은 채로 기록할 Log를 압축한다.
     
     - Log 끝단의 Mutex를 잡는다
       - SN과 Magic값을 따온다.
       - 압축로그의 Head에 SN과 Magic값을 기록한다.
       - 압축로그를 로그파일에 기록한다
     - Log 끝단의 Mutex를 푼다. 

   * 압축로그의 판독 절차
     - 압축로그의 압축을 해제한다.
     - 압축로그의 Head에 기록된 SN과 Magic을 원본로그(비압축로그)의
       Head에 설정한다.
 */

/* BUG-35392 */
// 압축된 로그의 Head 크기
// smrLogComp.h에 기술된 압축로그 포맷참고

// Flag, Uchar -> UInt로 타입이 변경 되었음
#define SMR_COMP_LOG_FLAG_SIZE      ( ID_SIZEOF( UInt) )

// 압축 된 로그 크기
#define SMR_COMP_LOG_COMP_SIZE      ( ID_SIZEOF( UInt ) )
// LSN 
#define SMR_COMP_LOG_LSN_SIZE       ( ID_SIZEOF( smLSN ) )
// Magic
#define SMR_COMP_LOG_MAGIC_SIZE     ( ID_SIZEOF( smMagic ) )
// 원본 로그 크기
#define SMR_COMP_LOG_DECOMP_SIZE    ( ID_SIZEOF( UInt ) )
// BUG-46944 Comp Log에 추가되는 TableOID의 크기
#define SMR_COMP_LOG_TABLEOID_SIZE  ( ID_SIZEOF( smOID ) )

// 압축된 로그의 Tail 크기
#define SMR_COMP_LOG_TAIL_SIZE      ( ID_SIZEOF( UInt ) )

#define SMR_COMP_LOG_HEAD_SIZE      ( SMR_COMP_LOG_FLAG_SIZE +      \
                                      SMR_COMP_LOG_COMP_SIZE +      \
                                      SMR_COMP_LOG_LSN_SIZE  +      \
                                      SMR_COMP_LOG_MAGIC_SIZE +     \
                                      SMR_COMP_LOG_DECOMP_SIZE +    \
                                      SMR_COMP_LOG_TABLEOID_SIZE )

// 압축된 로그의 오버헤드
// N바이트의 "압축로그"를 제외한 나머지 부분의 크기
#define SMR_COMP_LOG_OVERHEAD \
           ( SMR_COMP_LOG_HEAD_SIZE + SMR_COMP_LOG_TAIL_SIZE )

//압축 된 로그의 Flag offset
#define SMR_COMP_LOG_FLAG_OFFSET    ( 0 ) // 항상 제일 앞에 있어야 함

//압축된 로그의 크기가 저장된 offset
#define SMR_COMP_LOG_SIZE_OFFSET    ( SMR_COMP_LOG_FLAG_SIZE )

// 압축로그 Head상의 Log SN offset
#define SMR_COMP_LOG_LSN_OFFSET     ( SMR_COMP_LOG_FLAG_SIZE +      \
                                      SMR_COMP_LOG_COMP_SIZE )

// 압축로그 Head상의 MAGIC Value offset
#define SMR_COMP_LOG_MAGIC_OFFSET   ( SMR_COMP_LOG_FLAG_SIZE +      \
                                      SMR_COMP_LOG_COMP_SIZE +      \
                                      SMR_COMP_LOG_LSN_SIZE )

// 압축로그 Head상의 원본 로그 크기 offset
#define SMR_COMP_LOG_DECOMP_OFFSET  ( SMR_COMP_LOG_FLAG_SIZE +      \
                                      SMR_COMP_LOG_COMP_SIZE +      \
                                      SMR_COMP_LOG_LSN_SIZE  +      \
                                      SMR_COMP_LOG_MAGIC_SIZE )

// 압축로그 Head상의 TableOID offset
#define SMR_COMP_LOG_TABLEOID_OFFSET   ( SMR_COMP_LOG_FLAG_SIZE +      \
                                         SMR_COMP_LOG_COMP_SIZE +      \
                                         SMR_COMP_LOG_LSN_SIZE  +      \
                                         SMR_COMP_LOG_MAGIC_SIZE +     \
                                         SMR_COMP_LOG_DECOMP_SIZE )



class smrLogComp
{
public :
    /* 압축버퍼에 압축로그를 기록한다. */
    static IDE_RC createCompLog( iduMemoryHandle    * aCompBufferHandle,
                                 SChar              * aRawLog,
                                 UInt                 aRawLogSize,
                                 SChar             ** aCompLog,
                                 UInt               * aCompLogSize,
                                 smOID                aTableOID );

    // 압축 로그의 Head에 SN을 세팅한다.
    static void setLogLSN( SChar * aCompLog,
                           smLSN   aLogLSN );

    // 압축 로그의 Head에 MAGIC값을 세팅한다.
    static void setLogMagic( SChar * aCompLog,
                             UShort  aMagicValue );
    
    
    /* 로그파일의 특정 Offset에서 로그 레코드를 읽어온다.
       압축된 로그의 경우, 로그 압축해제를 수행한다.
     */
    static IDE_RC readLog( iduMemoryHandle    * aDecompBufferHandle,
                           smrLogFile         * aLogFile,
                           UInt                 aLogOffset,
                           smrLogHead         * aRawLogHead,
                           SChar             ** aRawLogPtr,
                           UInt               * aLogSizeAtDisk );

    /* 로그파일의 특정 Offset에서 로그 레코드를 읽어온다.
       압축된 로그라면 압축된 형태 그대로 반환한다.
     */
    static IDE_RC readLog4RP( smrLogFile         * aLogFile,
                              UInt                 aLogOffset,
                              smrLogHead         * aRawLogHead,
                              SChar             ** aRawLogPtr,
                              UInt               * aLogSizeAtDisk );

   /* 압축된, 혹은 압축되지 않은 로그 레코드로부터
      압축되지 않은 형태의 Log Head와 Log Ptr을 가져온다.
   */
    static IDE_RC getRawLog( iduMemoryHandle    * aDecompBufferHandle,
                             UInt                 aRawLogOffset,
                             SChar              * aRawOrCompLog,
                             smMagic              aValidLogMagic,
                             smrLogHead         * aRawLogHead,
                             SChar             ** aRawLogPtr,
                             UInt               * aLogSizeAtDisk );

    /* 압축된 로그의 압축해제를 수행한다 */
    static IDE_RC decompressCompLog( iduMemoryHandle    * aDecompBufferHandle,
                                     UInt                 aCompLogOffset,
                                     SChar              * aCompLog,
                                     smMagic              aValidLogMagic,
                                     smrLogHead         * aRawLogHead,
                                     SChar             ** aRawLog,
                                     UInt               * aLogSizeAtDisk );

    /* 압축 대상 로그인지 판별한다 */
    static IDE_RC shouldLogBeCompressed( SChar * aRawLog,
                                         UInt aRawLogSize,
                                         idBool * aDoCompLog );

    
    /* 압축된 로그인지 여부를 판별한다. */
    static inline idBool isCompressedLog( SChar * aRawOrCompLog );
    /* 압축로그면 Log에 적혀있는 TableOID를 아니면 NULL OID를 반환한다. */
    static inline smOID getTableOID( SChar * aRawOrCompLog );

    static UInt getCompressedLogSize( SChar  * aCompLog );

    static void setCompressedLogSize( SChar  * aCompLog,
                                      UInt     aCompLogSize );

    /* PROJ-1923 ALTIBASE HDB Disaster Recovery
     * 타 모듈에서 사용하기 위해 private -> public 전환 */
    /* 압축해제를 위한 메모리 공간 준비 */
    static IDE_RC prepareDecompBuffer( iduMemoryHandle   * aDecompBufferHandle,
                                        UInt                aRawLogSize,
                                        SChar            ** aDecompBuffer,
                                        UInt              * aDecompBufferSize);


private:
    /* 압축된 로그의 압축해제를 수행한다 */
    static IDE_RC decompressLog( iduMemoryHandle    * aDecompBufferHandle,
                                 UInt                 aCompLogOffset,
                                 SChar              * aCompLog,
                                 smMagic              aValidLogMagic,
                                 SChar             ** aRawLog,
                                 smMagic            * aMagicValue,
                                 smLSN              * aLogLSN,
                                 UInt               * aCompLogSize );
    
    /* 원본로그를 압축한 데이터를 압축로그 Body로 기록한다. */
    static IDE_RC writeCompBody( SChar  * aCompDestPtr,
                                 UInt     aCompDestSize,
                                 SChar  * aRawLog,
                                 UInt     aRawLogSize,
                                 UInt   * aCompressedRawLogSize );
        
    /* 압축로그의 Head를 기록한다. */
    static void writeCompHead( SChar  * aHeadDestPtr,
                               SChar  * aRawLog,
                               UInt     aRawLogSize,
                               UInt     aCompressedRawLogSize,
                               smOID    aTableOID );
    
    /*  압축로그의 Tail을 기록한다. */
    static void writeCompTail( SChar * aTailDestPtr,
                               UInt    aRawLogSize );
    
    /* 새로 기록할 압축된 로그를 위한 메모리 공간 준비 */
    static IDE_RC prepareCompBuffer( iduMemoryHandle    * aCompBufferHandle,
                                     UInt                 aRawLogSize,
                                     SChar             ** aCompBuffer,
                                     UInt               * aCompBufferSize);

    /* 압축로그를 해석한다. */
    static IDE_RC analizeCompLog( SChar   * aCompLog,
                                  UInt      aCompLogOffset,
                                  smMagic   aValidLogMagic,
                                  idBool  * aIsValid,
                                  smMagic * aMagicValue,
                                  smLSN   * aLogLSN,
                                  UInt    * aRawLogSize,
                                  UInt    * aCompressedRawLogSize );
    
    /* VALID하지 않은 로그를 압축되지 않은 형태로 생성한다. */
    static IDE_RC createInvalidLog( iduMemoryHandle    * aDecompBufferHandle,
                                    SChar             ** aInvalidRawLog );
    
    // Log Record에 기록된 Tablespace ID를 리턴한다.
    static void getSpaceIDOfLog( smrLogHead * aLogHead,
                                 SChar      * aRawLog,
                                 scSpaceID  * aSpaceID );

};

/******************************************************************************
 * BUG-35392
 * 압축 log의 head와 tail의 크기가 포함된 압축 로그크기를 반환한다.
 *
 * [IN]  aCompLog      - 압축된 로그의 시작 주소
 * [OUT] aCompLogSize  - 압축된 로그의 크기
 *****************************************************************************/
inline UInt smrLogComp::getCompressedLogSize( SChar  * aCompLog )
{
    UInt    sCompLogSizeWithoutHdrNTail;
    UInt    sLogFlag    = 0;

    IDE_DASSERT( aCompLog   != NULL );

    idlOS::memcpy( &sLogFlag,
                   aCompLog,
                   ID_SIZEOF( UInt ) );    

    IDE_DASSERT( (sLogFlag & SMR_LOG_COMPRESSED_MASK) == SMR_LOG_COMPRESSED_OK );

    //BUG-34791 SIGBUS occurs when recovering multiplex logfiles
    idlOS::memcpy( &sCompLogSizeWithoutHdrNTail, 
                   (aCompLog + SMR_COMP_LOG_SIZE_OFFSET), SMR_COMP_LOG_COMP_SIZE);

    return sCompLogSizeWithoutHdrNTail;
}

/******************************************************************************
 * BUG-35392
 * 압축 log의 head와 tail의 크기가 포함된 압축 로그크기를 설정한다.
 *
 * [IN]  aCompLog      - 압축된 로그의 시작 주소
 * [OUT] aCompLogSize  - 압축된 로그의 크기
 *****************************************************************************/
inline void smrLogComp::setCompressedLogSize( SChar  * aCompLog,
                                              UInt     aCompLogSize )
{
    UInt    sLogFlag    = 0;

    IDE_DASSERT( aCompLog   != NULL );

    idlOS::memcpy( &sLogFlag,
                   aCompLog,
                   ID_SIZEOF( UInt ) );    

    IDE_DASSERT( (sLogFlag & SMR_LOG_COMPRESSED_MASK) != SMR_LOG_COMPRESSED_OK );

    //BUG-34791 SIGBUS occurs when recovering multiplex logfiles
    idlOS::memcpy( (aCompLog + SMR_COMP_LOG_SIZE_OFFSET),
                   &aCompLogSize,
                   SMR_COMP_LOG_COMP_SIZE );
}


/******************************************************************************
 * BUG-35392
 * 압축 로그의 Head에 LSN을 세팅한다.
 *
 * - 최초 압축로그 생성시 압축 로그의 Head에 Log의 SN은 0으로 기록된다.
 * - 추후 로그파일에 로그 기록할때 로그 끝단의 Mutex를 잡은 상태로
 *   로그가 기록될 SN을 따고 이 함수를 이용하여 기록한다.
 *
 * [IN] aCompLog - 압축로그
 * [IN] aLogSN   - 로그의 SN
 *****************************************************************************/
inline void smrLogComp::setLogLSN( SChar * aCompLog,
                                   smLSN   aLogLSN )
{
    IDE_DASSERT( aCompLog != NULL );

    idlOS::memcpy( aCompLog + SMR_COMP_LOG_LSN_OFFSET,
                   &aLogLSN,
                   SMR_COMP_LOG_LSN_SIZE );
}


/******************************************************************************
 * BUG-35392
 * 압축 로그의 Head에 MAGIC값을 세팅한다.
 *
 * - 최초 압축로그 생성시 압축 로그의 Head에 Log의 Magic은 0으로 기록된다.
 *
 * - 추후 로그파일에 로그 기록할때 로그 끝단의 Mutex를 잡은 상태로
 *   로그가 기록될 LSN을 따서 Magic을 계산하고 이 함수를 이용하여 기록한다.
 *
 * [IN] aCompLog - 압축로그
 * [IN] aLogSN   - 로그의 SN
 *****************************************************************************/
inline void smrLogComp::setLogMagic( SChar  * aCompLog,
                                     smMagic  aMagicValue )
{
    IDE_DASSERT( aCompLog != NULL );

    idlOS::memcpy( aCompLog + SMR_COMP_LOG_MAGIC_OFFSET,
                   & aMagicValue,
                   SMR_COMP_LOG_MAGIC_SIZE );
}

/******************************************************************************
 * 압축된 로그인지 여부를 판별한다.
 *
 * 압축로그, 비압축 로그 모두 첫번째 바이트에 Flag를 두고,
 * 압축되었는지 여부를 SMR_LOG_COMPRESSED_OK 비트에 표시한다.
 *
 *  [IN] aRawOrCompLog - 압축로그 혹은 압축되지 않은 상태의 로그
 *****************************************************************************/
inline idBool smrLogComp::isCompressedLog( SChar * aRawOrCompLog )
{
    IDE_DASSERT( aRawOrCompLog != NULL );

    idBool  sIsCompressed;
    UInt    sLogFlag;

    /* BUG-35392 smrLogHead의 mFlag는 UInt 이므로 memcpy로 읽어야 
     * align 문제를 회피할 수 있다. */
    idlOS::memcpy( &sLogFlag,
                   aRawOrCompLog,
                   SMR_COMP_LOG_FLAG_SIZE );

    if ( ( sLogFlag & SMR_LOG_COMPRESSED_MASK )
         ==  SMR_LOG_COMPRESSED_OK )
    {
        sIsCompressed = ID_TRUE;
    }
    else
    {
        sIsCompressed = ID_FALSE;
    }

    return sIsCompressed;
}

/******************************************************************************
 * BUG-46944 LogHeader를 넘겨주면 압축된 로그인지 확인 해서
 * 압축로그면 Log에 적혀있는 TableOID를
 * 압축로그가 아니면 NULL OID를 반환한다.
 * 압축헤더에 기록된 TableOID가 NULL OID일 경우 일단 열어 봐야한다.
 *****************************************************************************/
inline smOID smrLogComp::getTableOID( SChar * aRawOrCompLog )
{
    smOID sTableOID = SM_OID_NULL;

    if ( isCompressedLog( aRawOrCompLog ) == ID_TRUE )
    {
        idlOS::memcpy( &sTableOID,
                       aRawOrCompLog + SMR_COMP_LOG_TABLEOID_OFFSET,
                       SMR_COMP_LOG_TABLEOID_SIZE );
    }

    return sTableOID;
}

#endif /* _O_SMR_LOG_COMP_H_ */

