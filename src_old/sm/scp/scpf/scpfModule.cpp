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
 * $Id: scpfModule.cpp 
 **********************************************************************/

/* << SM Common DataPort File Module >>
 *  < Proj-2059 DB Upgrade 기능 >
 *
 *
 *
 *
 *  - 개요
 *   본 Module은 Table에 있는 데이터를 추출하고 넣는 DataPort 기능 중
 *  매개체를 File로 사용하는 Module입니다.
 *   본 Module은 scpModule에서 정의된 Interface를 구현합니다. 사용하는 
 * 쪽에서는 smiDataPort 인터페이스를 통해 scpModule을 선택하여 사용하면
 * 됩니다.
 *
 *
 *
 *
 *
 *
 *  - File구조
 *      Header
 *          Version(4Byte)
 *          CommonHeader( gSmiDataPortHeaderDesc )
 *          FileHeader( gScpfFileHeaderDesc ) 
 *          TableHeader( gQsfTableHeaderDesc )
 *          ColumnHeader( gQsfColumnHeaderDesc ) * ColumnCount
 *          PartitionHeader( gQsfPartitionHeaderDesc ) * PartitionCount
 *      Block
 *          BlockHeader( gScpfBlockHeaderDesc )
 *          BlockBody
 *
 *
 *
 *
 *
 *
 *  - Block Body의 구조
 *      - BasicColumn
 *          - 기본
 *              <Slot_Len(1~4Byte)><SlotVal(..Byte)>
 *
 *          - Block에 걸칠 경우 (Chained)
 *              <Slot_Len(1~4Byte)><SlotVal(..)> |  <VarLen(1~4Byte)><SlotVal(....)> 
 *              예) <Slot_Len:20><....20Byte>
 *                      =>  <Slot_Len:15><....15Byte> | <VarLen:5><..5Byte)
 *
 *      - LobColumn
 *          - 기본
 *              <LobLen(4Byte)><VarInt(1~4Byte)><SlotVal(..Byte)>
 *
 *          - Block에 걸칠 경우 (Chained)
 *              <LobLen(4Byte)><Slot_Len(1~4Byte)><SlotVal(..Byte)>
 *                      |  <Slot_Len(1~4Byte)><SlotVal(..Byte)> 
 *              예) <LobLen:20><Slot_Len:20><....20Byte>
 *                      =>  <LobLen:20><Slot_Len:15><....15Byte>
 *                              | <Slot_Len:5><..5Byte)
 *
 *
 *
 *
 *
 * - 용어
 *  Block        : 1~N개의 Block이 모여 File을 구성한다. 
 *                 Block은 0~N개의 Row로 구성된다.
 *  Row          : Row는 1~N개의 Column으로 구성된다.
 *  Column       : Column은 BasicColumn과 LobColumn이 있다.
 *  BasicColumn  : 1~N개의 Slot이 모여 하나의 BasicColumn이 된다.
 *  LobColumn    : LobLen하나와 1~N개의 Slot이 모여 하나의 BasicColumn이 된다.
 *
 *  isLobColmnInfo : 각 Column들이 Lob이냐 아니냐에 대한 정보. 
 *                      이번에 읽을 Column이 Lob인지 기록되어 있다.
 *
 *  ColumnMap    : BasicColumn과 LobColumn을 정리한 Map.
 *                 ex) Column이 NLNNL 순서일 경우, NNNLL로 배치되어야 함.
 *                     ColumnMap -> 0,2,3,1,4
 *
 *  SlotValLen   : Slot의 Length. 크기에 따라 Variable하게 1~4Byte 사이로
 *                 변경되어 기록된다.
 *      0~63        [0 | len ]
 *     64~16383     [1 | len>>8  & 63][len ]
 *  16384~4194303   [2 | len>>16 & 63][len>>8 & 255][len & 255]
 *4194304~1073741824[3 | len>>24 & 63][len>>16 & 255][len>>8 & 255][len & 255]
 *
 *  LobLen       : Lob Column하나의 전체 길이. 4Byte UInt로 기록된다.
 *
 *  Overflow     : Row가 Block에 걸쳐 기록될때, 소속된 Slot은 이전 Block에 기록
 *                 되어야 하는데 넘어갔으므로, OverflowBlock이다
 *
 *  Chaining     : Row를 구성하는 Column이 Block에 걸쳐 기록될때,
 *                 Chaining되었다고 한다.
 *
 *  Overflow와Chaining의 차이 : Chaining은 Column의 Slot이 쪼개진 것이고
 *                              OverFlow는 Row를 구성하는 Column이
 *                              나뉘어 기록된 것이다.
 *      ex) Table( Integer, Char(10) )
 *           Overflow N Chaining N : <Integer><Char(10)>
 *           Overflow Y Chaining N : <Integer> | <Char(10)>
 *           Overflow N Chaining Y :  - 있을수 없다 -
 *           Overflow Y Chaining Y : <Integer><Char | (10)>
 *           
 *  BuildRow     : Block을 해석하여 Value에 Pointer로 연결하는 일이다.
 *                 따라서 Import시, memcpy없이 Block에 있는 것으로
 *                 바로 insert하는데 사용할 수 있다.
 *
 *
 *
 *
 * - 함수 분류
 *    - interface
 *    scpfBeginExport
 *    scpfWrite
 *    scpfPrepareLob
 *    scpfWriteLob
 *    scpfEndExport
 * 
 *    scpfBeginImport
 *    scpfRead
 *    scpfReadLobLength
 *    scpfReadLob
 *    scpfEndImport
 * 
 *
 *
 * - Export용
 *    - Handle Alloc/Dealloc
 *    scpfInit4Export
 *    scpfDestroy4Export
 * 
 *    - 파일 Open/Close
 *    scpfPrepareFile
 *    scpfCloseFile4Export
 * 
 *    - 파일 읽고 쓰기
 *    scpfWriteHeader
 *    scpfAllocSlot
 *    scpfWriteBasicColumn
 *    scpfWriteLobColumn
 *    scpfWriteBlock
 *    scpfPrepareBlock
 * 
 * 
 * 
 * 
 * - Import용
 *    - Handle 제어
 *    scpfInit4Import
 *    scpfReleaseHeader
 *    scpfDestroy4Import
 * 
 *    - 파일 Open/Close
 *    scpfOpenFile
 *    scpfFindFile
 *    scpfCloseFile4Import
 * 
 *    - 파일 읽고 쓰기
 *    scpfFindFirstBlock
 *    scpfShiftBlock
 *    scpfPreFetchBlock
 *    scpfReadHeader
 *    scpfReadBlock
 * 
 *    scpfBuildRow
 *    scpfReadBasicColumn
 *    scpfReadLobColumn
 *    scpfReadSlot
 *
 */

#include <ide.h>
#include <scpReq.h>
#include <scpModule.h>
#include <scpfModule.h>
#include <smuProperty.h>
#include <smiMain.h>
#include <smiMisc.h>
#include <smiDataPort.h>
#include <smErrorCode.h>

extern smiGlobalCallBackList gSmiGlobalCallBackList;

/**********************************************************************
 * Interface
 **********************************************************************/

//export를 시작한다.
static IDE_RC scpfBeginExport( idvSQL               * aStatistics,
                               void                ** aHandle,
                               smiDataPortHeader    * aCommonHeader,
                               SChar                * aObjName,
                               SChar                * aDirectory,
                               SLong                  aSplit );

//Row를 하나 Write한다.
static IDE_RC scpfWrite( idvSQL         * aStatistics,
                         void           * aHandle,
                         smiValue       * aValueList );

// Lob을 쓸 준비를 한다.
static IDE_RC scpfPrepareLob( idvSQL         * aStatistics,
                              void           * aHandle,
                              UInt             aLobLength );


// Lob을 기록한다.
static IDE_RC scpfWriteLob( idvSQL         * aStatistics,
                            void           * aHandle,
                            UInt             aLobPieceLength,
                            UChar          * aLobPieceValue );

// Lob을 기록을 완료한다.
static IDE_RC scpfFinishLobWriting( idvSQL         * aStatistics,
                                    void           * aHandle );

// Export를 종료한다.
static IDE_RC scpfEndExport( idvSQL         * aStatistics,
                             void          ** aHandle );


//import를 시작한다. 헤더를 읽는다.
static IDE_RC scpfBeginImport( idvSQL              * aStatistics,
                               void               ** aHandle,
                               smiDataPortHeader   * aCommonHeader,
                               SLong                 aFirstRowSeq,
                               SLong                 aLastRowSeq,
                               SChar               * aObjName,
                               SChar               * aDirectory );

//row들을 읽는다.
static IDE_RC scpfRead( idvSQL         * aStatistics,
                        void           * aHandle,
                        smiRow4DP     ** aRows,
                        UInt           * aRowCount );

//Lob의 총 길이를 반환한다.
static IDE_RC scpfReadLobLength( idvSQL         * aStatistics,
                                 void           * aHandle,
                                 UInt           * aLength );

//Lob을 읽는다.
static IDE_RC scpfReadLob( idvSQL         * aStatistics,
                           void           * aHandle,
                           UInt           * aLobPieceLength,
                           UChar         ** aLobPieceValue );

// Lob을 읽기를 완료한다.
static IDE_RC scpfFinishLobReading( idvSQL         * aStatistics,
                                    void           * aHandle );
//import를 종료한다.
static IDE_RC scpfEndImport( idvSQL         * aStatistics,
                             void          ** aHandle );

scpModule scpfModule = {
    SMI_DATAPORT_TYPE_FILE,
    (scpBeginExport)      scpfBeginExport,
    (scpWrite)            scpfWrite,
    (scpPrepareLob)       scpfPrepareLob,
    (scpWriteLob)         scpfWriteLob,
    (scpFinishLobWriting) scpfFinishLobWriting,
    (scpEndExport)        scpfEndExport,
    (scpBeginImport)      scpfBeginImport,
    (scpRead)             scpfRead,
    (scpReadLobLength)    scpfReadLobLength,
    (scpReadLob)          scpfReadLob,
    (scpFinishLobReading) scpfFinishLobReading,
    (scpEndImport)        scpfEndImport
};



/**********************************************************************
 * Internal Functions for Exporting
 **********************************************************************/

// Export하기 위해 Handle을 기본 설정 한다.
static IDE_RC scpfInit4Export( idvSQL                * aStatistics,
                               scpfHandle           ** aHandle,
                               smiDataPortHeader     * aCommonHeader,
                               SChar                 * aObjName,
                               SChar                 * aDirectory,
                               SLong                   aSplit );

// Handle을 제거한다.
static IDE_RC scpfDestroy4Export( idvSQL         * aStatistics,
                                  scpfHandle    ** aHandle );

// 기록할 파일을 준비한다.
static IDE_RC scpfPrepareFile( idvSQL         * aStatistics,
                               scpfHandle     * aHandle );

// File을 닫는다.
static IDE_RC scpfCloseFile4Export( idvSQL         * aStatistics,
                                    scpfHandle     * aHandle );

// 기록 할Slot을 확보한다
static IDE_RC scpfAllocSlot( 
                idvSQL                  * aStatistics,
                iduFile                 * aFile,
                smiDataPortHeader       * aCommonHeader,
                scpfHeader              * aFileHeader,
                SLong                     aRowSeq,
                UInt                      aColumnChainingThreshold,
                idBool                  * aIsFirstSlotInRow,
                scpfBlockInfo           * aBlockInfo,
                idBool                    aWriteLobLen,
                UInt                      aNeedSize,
                scpfBlockBufferPosition * aBlockBufferPosition,
                scpfFilePosition        * aFilePosition,
                UInt                    * aAllocatedSlotValLen );

// BasicColumn을 write한다.
static IDE_RC scpfWriteBasicColumn( 
            idvSQL                  * aStatistics,
            iduFile                 * aFile,
            smiDataPortHeader       * aCommonHeader,
            scpfHeader              * aFileHeader,
            scpfBlockInfo           * aBlockInfo,
            UInt                      aColumnChainingThreshold,
            SLong                     aRowSeq,
            smiValue                * aValue,
            idBool                  * aIsFirstSlotInRow,
            scpfBlockBufferPosition * aBlockBufferPosition,
            scpfFilePosition        * aFilePosition );

// LobColumn( LobLen + slot...)를 Write한다.
static IDE_RC scpfWriteLobColumn( idvSQL         * aStatistics,
                                  scpfHandle     * aHandle,
                                  UInt             aLobPieceLength,
                                  UChar          * aLobPieceValue,
                                  UInt           * aRemainLobLength,
                                  UInt           * aAllocatedLobSlotSize );

// 다음 Block을 준비한다.
static IDE_RC scpfPrepareBlock( idvSQL                  * aStatistics,
                                scpfHeader              * aFileHeader,
                                SLong                     aRowSeq,
                                scpfBlockInfo           * aBlockInfo,
                                scpfBlockBufferPosition * aBlockBufferPosition );

// 현재 기록중인 Block을 파일에 기록한다.
static IDE_RC scpfWriteBlock( idvSQL                  * aStatistics, 
                              iduFile                 * aFile,
                              smiDataPortHeader       * aCommonHeader,
                              scpfHeader              * aFileHeader,
                              SLong                     aRowSeq,
                              scpfBlockInfo           * aBlockInfo,
                              scpfBlockBufferPosition * aBlockBufferPosition,
                              scpfFilePosition        * aFilePosition );

// Header를 File에 쓴다.
static IDE_RC scpfWriteHeader( idvSQL                     * aStatistics,
                               iduFile                    * aFile,
                               smiDataPortHeader          * aCommonHeader,
                               scpfHeader                 * aFileHeader,
                               scpfBlockBufferPosition    * aBlockBufferPosition,
                               scpfFilePosition           * aFilePosition );

/**********************************************************************
 * Internal Functions for Importing
 **********************************************************************/

// import하기 위해 Handle을 준비한다.
static IDE_RC scpfInit4Import( idvSQL                * aStatistics,
                               scpfHandle           ** aHandle,
                               smiDataPortHeader     * aCommonHeader,
                               SLong                   aFirstRowSeq,
                               SLong                   aLastRowSeq,
                               SChar                 * aObjName,
                               SChar                 * aDirectory );

// Handle을 제거한다.
static IDE_RC scpfDestroy4Import( idvSQL         * aStatistics,
                                  scpfHandle    ** aHandle );
// 해당 File을 연다.
static IDE_RC scpfOpenFile( idBool         * aOpenFile,
                            iduFile        * aFile,
                            SChar          * aObjName, 
                            SChar          * aDirectory,
                            UInt             aIdx,
                            SChar          * aFileName );

// 목표 File을 찾는다
static IDE_RC scpfFindFile( SChar            * aObjName,
                            SChar            * aDirectory,
                            UInt               aFileIdx,
                            SChar            * aFileName );

// File을 닫는다.
static IDE_RC scpfCloseFile4Import( idvSQL         * aStatistics,
                                    scpfHandle     * aHandle );

// Header를 읽는다.
static IDE_RC scpfReadHeader( idvSQL                    * aStatistics,
                              iduFile                   * aFile,
                              smiDataPortHeader         * aCommonHeader,
                              scpfHeader                * aFileHeader,
                              scpfBlockBufferPosition   * aBlockBufferPosition,
                              scpfFilePosition          * aFilePosition );

// Header를 읽는데 사용했던 메모리를 삭제
static IDE_RC scpfReleaseHeader( idvSQL         * aStatistics,
                                 scpfHandle     * aHandle );

// File로부터 Block을 읽는다.
static IDE_RC scpfReadBlock( idvSQL             * aStatistics,
                             iduFile            * aFile,
                             smiDataPortHeader  * aCommonHeader,
                             UInt                 aFileHeaderSize,
                             UInt                 aBlockHeaderSize,
                             UInt                 aBlockSize,
                             UInt                 aBlockID,
                             ULong              * aFileOffset,
                             scpfBlockInfo      * aBlockInfo );

// BinarySearch로 FirstRow를 가진 Block을 찾는다.
static IDE_RC scpfFindFirstBlock( idvSQL                   * aStatistics,
                                  iduFile                  * aFile,
                                  smiDataPortHeader        * aCommonHeader,
                                  SLong                      aFirstRowSeqInFile,
                                  SLong                      aTargetRow,
                                  UInt                       aBlockCount,
                                  UInt                       aFileHeaderSize,
                                  UInt                       aBlockHeaderSize,
                                  UInt                       aBlockSize,
                                  scpfBlockInfo           ** aBlockMap,
                                  scpfBlockBufferPosition  * aBlockBufferPosition );

// BlockMap을 Shift시켜서 아직 읽지 않은 Block을 BlockMap의 앞쪽에
// 오도록 이동시킨다
static void scpfShiftBlock( UInt                aShiftBlockCount,
                            UInt                aBlockInfoCount,
                            scpfBlockInfo    ** aBlockMap );

// 다음번 Block을 읽어서 Handle에 올린다.
static IDE_RC scpfPreFetchBlock( idvSQL             * aStatistics,
                                 iduFile            * aFile,
                                 smiDataPortHeader  * aCommonHeader,
                                 SChar              * aFileName,
                                 SLong                aTargetRow,
                                 UInt                 aBlockCount,
                                 UInt                 aFileHeaderSize,
                                 UInt                 aBlockHeaderSize,
                                 UInt                 aBlockSize,
                                 UInt                 aNextBlockID,
                                 scpfFilePosition   * aFilePosition,
                                 UInt                 aBlockInfoCount,
                                 UInt                 aReadBlockCount,
                                 scpfBlockInfo     ** aBlockMap );

// Block을 Parsing하여 Row를 만든다
static IDE_RC scpfBuildRow( idvSQL                    * aStatistics,
                            scpfHandle                * aHandle,
                            smiDataPortHeader         * aCommonHeader,
                            scpfHeader                * aFileHeader,
                            scpfBlockBufferPosition   * aBlockBufferPosition,
                            scpfBlockInfo            ** aBlockMap,
                            UChar                     * aChainedSlotBuffer,
                            UInt                      * aUsedChainedSlotBufSize,
                            UInt                      * aBuiltRowCount,
                            idBool                    * aHasSupplementLob,
                            smiRow4DP                 * aRowList );

// BasicColumn을 읽는다
static IDE_RC scpfReadBasicColumn( 
                    scpfBlockBufferPosition * aBlockBufferPosition,
                    scpfBlockInfo          ** aBlockMap,
                    UInt                      aBlockHeaderSize,
                    UChar                   * aChainedSlotBuffer,
                    UInt                    * aUsedChainedSlotBufferSize,
                    smiValue                * aValue );

// LobColumn을 읽는다.
static IDE_RC scpfReadLobColumn( scpfBlockBufferPosition  * aBlockBufferPosition,
                                 scpfBlockInfo           ** aBlockMap,
                                 UInt                       aBlockHeaderSize,
                                 smiValue                 * aValue );

// Slot하나를 읽는다.
static IDE_RC scpfReadSlot( scpfBlockBufferPosition * aBlockBufferPosition,
                            scpfBlockInfo          ** aBlockMap,
                            UInt                      aBlockHeaderSize,
                            UInt                    * aSlotValLen,
                            void                   ** aSlotVal );

/**********************************************************************
 * Internal Functions for Utility
 **********************************************************************/

IDE_RC scpfDumpBlockBody( scpfBlockInfo  * aBlockInfo,
                          UInt             aBeginOffset,
                          UInt             aEndOffset,
                          SChar          * aOutBuf,
                          UInt             aOutSize );




smiDataPortHeaderColDesc  gScpfBlockHeaderEncColDesc[]=
{
    {
        (SChar*)"CHECK_SUM",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mCheckSum ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mCheckSum ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"BLOCK_ID",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mBlockID ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mBlockID ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"ROW_SEQ_OF_FIRST_SLOT",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mRowSeqOfFirstSlot ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mRowSeqOfFirstSlot ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_LONG
    },
    {
        (SChar*)"ROW_SEQ_OF_LAST_SLOT",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mRowSeqOfLastSlot ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mRowSeqOfLastSlot ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_LONG
    },
    {
        (SChar*)"FIRST_ROW_SLOTSEQ",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mFirstRowSlotSeq ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mFirstRowSlotSeq ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"FIRST_ROW_OFFSET",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mFirstRowOffset ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mFirstRowOffset ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"HAS_LASTCHAINED_SLOT",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mHasLastChainedSlot ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mHasLastChainedSlot ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"SLOT_COUNT",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfBlockInfo, mSlotCount ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfBlockInfo, mSlotCount ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        NULL,
        0,
        0,
        0,0,
        0
    }
};

IDE_RC gScpfBlockHeaderValidation( void * aDesc, 
                                   UInt   aVersion,
                                   void * aHeader )
{
    smiDataPortHeaderDesc * sDesc;
    scpfBlockInfo         * sHeader;
    SChar                 * sTempBuf;
    UInt                    sState  = 0;

    sDesc   = (smiDataPortHeaderDesc*)aDesc;
    sHeader = (scpfBlockInfo*)aHeader;

    IDE_TEST_RAISE( 1 < sHeader->mHasLastChainedSlot,
                    ERR_ABORT_DATA_PORT_INTERNAL_ERROR );

    IDE_TEST_RAISE( sHeader->mFirstRowSlotSeq > sHeader->mSlotCount,
                    ERR_ABORT_DATA_PORT_INTERNAL_ERROR );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_DATA_PORT_INTERNAL_ERROR );
    {
        // Memory 할당 후, 해당 메모리에 본 Header를 Dump해둡니다
        // 그리고 Dump된 Header를 TRC로그에 출력합니다.
        if( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                               1, 
                               IDE_DUMP_DEST_LIMIT, 
                               (void**)&sTempBuf )
            == IDE_SUCCESS )
        {
            sState = 1;
            (void) smiDataPort::dumpHeader( sDesc,
                                            aVersion,
                                            aHeader,
                                            SMI_DATAPORT_HEADER_FLAG_FULL,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );
            ideLog::log( IDE_SERVER_0, "%s", sTempBuf );
            sState = 0;
            (void)iduMemMgr::free( sTempBuf );
        }

        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_DATA_PORT_INTERNAL_ERROR ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
            sTempBuf = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

smiDataPortHeaderDesc  gScpfBlockHeaderDesc[ SMI_DATAPORT_VERSION_COUNT ]=
{
    {
        (SChar*)"VERSION_NONE",
        0,
        NULL,
        NULL
    },
    {
        (SChar*)"BLOCK_HEADER",
        ID_SIZEOF( scpfBlockInfo),
        (smiDataPortHeaderColDesc*)gScpfBlockHeaderEncColDesc,
        gScpfBlockHeaderValidation
    }
};

smiDataPortHeaderColDesc  gScpfFileHeaderEncColDesc[]=
{
    {
        (SChar*)"FILE_HEADER_SIZE",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mFileHeaderSize ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mFileHeaderSize ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"BLOCK_HEADER_SIZE",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mBlockHeaderSize ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mBlockHeaderSize ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"BLOCK_SIZE",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mBlockSize ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mBlockSize ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"MAX_BLOCK_COUNT_PER_ROW",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mBlockInfoCount ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mBlockInfoCount ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"MAX_ROW_COUNT_IN_BLOCK",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mMaxRowCountInBlock ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mMaxRowCountInBlock ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"BLOCK_COUNT",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mBlockCount ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mBlockCount ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_INTEGER
    },
    {
        (SChar*)"ROW_COUNT",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mRowCount ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mRowCount ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_LONG
    },
    {
        (SChar*)"FIRST_ROW_SEQ",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mFirstRowSeqInFile ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mFirstRowSeqInFile ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_LONG
    },
    {
        (SChar*)"LAST_ROW_SEQ",
        SMI_DATAPORT_HEADER_OFFSETOF( scpfHeader, mLastRowSeqInFile ),
        SMI_DATAPORT_HEADER_SIZEOF( scpfHeader, mLastRowSeqInFile ),
        0, NULL,
        SMI_DATAPORT_HEADER_TYPE_LONG
    },
    {
        NULL,
        0,
        0,
        0,0,
        0
    }
};

IDE_RC gScpfFileHeaderValidation( void * aDesc, 
                                  UInt   aVersion,
                                  void * aHeader )
{
    smiDataPortHeaderDesc * sDesc;
    scpfHeader            * sHeader;
    SChar                 * sTempBuf;
    UInt                    sState  = 0;

    sDesc   = (smiDataPortHeaderDesc*)aDesc;
    sHeader = (scpfHeader*)aHeader;

    IDE_TEST_RAISE( sHeader->mBlockInfoCount == 0,
                    ERR_ABORT_DATA_PORT_INTERNAL_ERROR );

    IDE_TEST_RAISE( (sHeader->mMaxRowCountInBlock == 0) ||
                    (sHeader->mMaxRowCountInBlock > 1073741824 ),
                    ERR_ABORT_DATA_PORT_INTERNAL_ERROR );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_DATA_PORT_INTERNAL_ERROR );
    {
        // Memory 할당 후, 해당 메모리에 본 Header를 Dump해둡니다
        // 그리고 Dump된 Header를 TRC로그에 출력합니다.
        if( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                               1, 
                               IDE_DUMP_DEST_LIMIT, 
                               (void**)&sTempBuf )
            == IDE_SUCCESS )
        {
            sState = 1;
            (void) smiDataPort::dumpHeader( sDesc,
                                            aVersion,
                                            aHeader,
                                            SMI_DATAPORT_HEADER_FLAG_FULL,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );
            ideLog::log( IDE_SERVER_0, "%s", sTempBuf );
            sState = 0;
            (void)iduMemMgr::free( sTempBuf );
        }

        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_DATA_PORT_INTERNAL_ERROR ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
            sTempBuf = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}
 
smiDataPortHeaderDesc  gScpfFileHeaderDesc[ SMI_DATAPORT_VERSION_COUNT ]=
{
    {
        (SChar*)"VERSION_NONE",
        0,
        NULL,
        NULL
    },
    {
        (SChar*)"FILE_HEADER",
        ID_SIZEOF( scpfHeader ),
        (smiDataPortHeaderColDesc*)gScpfFileHeaderEncColDesc,
        gScpfFileHeaderValidation
    }
};




/***********************************************************************
 *
 * Description :
 *  Export를 위한 준비를 한다.
 *
 *  1) Object Handle을 초기화한다.
 *  2) 파일을 생성합니다.
 *  3) Header를 기록한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN/OUT] Object Handle
 *  aCommonHeader       - [IN] File에 내릴 공통 Header(Column,Table등 포함)
 *  aObjName            - [IN] Object 이름
 *  aSplit              - [IN] Split되는 기점 Row 개수
 *
 **********************************************************************/

static IDE_RC scpfBeginExport( idvSQL               * aStatistics,
                               void                ** aHandle,
                               smiDataPortHeader    * aCommonHeader,
                               SChar                * aObjName,
                               SChar                * aDirectory,
                               SLong                  aSplit )
{
    scpfHandle **sHandle;

    IDE_DASSERT( aHandle       != NULL );
    IDE_DASSERT( aCommonHeader != NULL );
    IDE_DASSERT( aObjName      != NULL );

    sHandle = (scpfHandle**)aHandle;

    // Handle을 초기화한다
    IDE_TEST( scpfInit4Export( aStatistics, 
                               sHandle, 
                               aCommonHeader,
                               aObjName,
                               aDirectory,
                               aSplit )
              != IDE_SUCCESS);

    // File을 생성한다.
    IDE_TEST( scpfPrepareFile( aStatistics, 
                               *sHandle )
              != IDE_SUCCESS);

    IDU_FIT_POINT( "1.PROJ-2059@scpfModule::scpfBeginExport" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//row를 하나 write한다.
static IDE_RC scpfWrite( idvSQL         * aStatistics,
                         void           * aHandle,
                         smiValue       * aValueList )
{
    scpfHandle               * sHandle;
    scpfBlockInfo            * sBlockInfo;
    scpfBlockBufferPosition  * sBlockBufferPosition;
    smiDataPortHeader        * sCommonHeader;
    UInt                     * sColumnMap;
    UInt                       sBeginBlockID = 0;
    UInt                       i = 0;

    IDE_DASSERT( aHandle    != NULL );
    IDE_DASSERT( aValueList != NULL );

    sHandle             = (scpfHandle*)aHandle;
    sCommonHeader       = sHandle->mCommonHandle.mHeader;
    sColumnMap          = sHandle->mColumnMap;
    sBlockInfo          = sHandle->mBlockMap[ 0 ]; // Export시에는 0번만 이용
    sBlockBufferPosition= &(sHandle->mBlockBufferPosition);

    // File Split 여부 체크
    if( sHandle->mSplit != 0 )
    {
        if( sHandle->mSplit == sHandle->mFileHeader.mRowCount  )
        {
            IDE_TEST( scpfCloseFile4Export( aStatistics, sHandle )
                      != IDE_SUCCESS);
            IDE_TEST( scpfPrepareFile( aStatistics, sHandle ) 
                      != IDE_SUCCESS );
        }
    }

    sBeginBlockID = sBlockBufferPosition->mID;
    sHandle->mIsFirstSlotInRow = ID_TRUE;

    // BasicColumn이 없었을 경우, Skip
    IDE_TEST_CONT( sCommonHeader->mBasicColumnCount == 0,
                    SKIP_WRITE_BASICCOLUMN );

    for( i=0; i < sCommonHeader->mBasicColumnCount; i++)
    {
        // N L N N L 일 경우, ColumnMap(0,2,3,1,4)에 의해 
        // 0,  2,3  번째에 있는 BasicColumn을 찾아 Write한다.
        IDE_TEST( scpfWriteBasicColumn( aStatistics,
                                         &sHandle->mFile,
                                         sHandle->mCommonHandle.mHeader,
                                         &sHandle->mFileHeader,
                                         sBlockInfo,
                                         sHandle->mColumnChainingThreshold,
                                         sHandle->mRowSeq,
                                         aValueList + sColumnMap[ i ],
                                         &sHandle->mIsFirstSlotInRow,
                                         &sHandle->mBlockBufferPosition,
                                         &sHandle->mFilePosition )
                  != IDE_SUCCESS );
    }

    // Row가 Block에 걸쳐 기록되면, 
    if( sBeginBlockID != sBlockBufferPosition->mID )
    {
        // firstBlock부터 LastBlock까지 Row가 걸쳐 기록되기 때문에, 
        // import시 걸쳐 기록된 만큼의 BlockInfo가 필요하다.
        sHandle->mFileHeader.mBlockInfoCount = 
            IDL_MAX( sHandle->mFileHeader.mBlockInfoCount,
                     sBlockBufferPosition->mID - sBeginBlockID + 1 );
    }

    // LobColumn이 없다면, BasicColumn에 대한 기록만으로도 Row에 대한
    // 기록은 완료된 것이다.
    if( sCommonHeader->mLobColumnCount == 0 )
    {
        sHandle->mFileHeader.mRowCount ++;
        sHandle->mRowSeq               ++;
    }

    IDE_EXCEPTION_CONT( SKIP_WRITE_BASICCOLUMN );

    sHandle->mRemainLobColumnCount = sCommonHeader->mLobColumnCount;

    IDU_FIT_POINT( "1.PROJ-2059@scpfModule::scpfWrite" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  Lob을 기록하기 전, 몇 Length짜리 Lob이 기록되는지 먼저 알리는
 * 함수이다.
 *  Block에 LobLength와 SlotValLen를 기록한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *  aLength             - [IN] 기록할 Lob의 길이
 *
 * - Lob Column
 *   [LobLen (4byte)][SlotValLen(1~4Byte)][SlotVal]
 *   Slot_Len는 역시 순수 SlotVal의 크기. 즉 LobLen의 4Byte는 포함되지 않는다.
 *   여기서 LobLen는 Slot이 Chaining된 것과 상관 없이, 하나의 LobColumn
 * 전체의 길이이다. 만약 Lob이 Chaining되면, 첫번째 Slot에만 기록한다.
 *
 **********************************************************************/
static IDE_RC scpfPrepareLob( idvSQL         * aStatistics,
                              void           * aHandle,
                              UInt             aLength )
{
    scpfHandle               * sHandle;
    UInt                       sAllocatedSlotValLen;

    IDE_DASSERT( aHandle != NULL );

    sHandle               = (scpfHandle*) aHandle;

    // Slot 확보
    IDE_TEST( scpfAllocSlot( aStatistics, 
                             &sHandle->mFile,
                             sHandle->mCommonHandle.mHeader,
                             &sHandle->mFileHeader,
                             sHandle->mRowSeq,
                             sHandle->mColumnChainingThreshold,
                             &sHandle->mIsFirstSlotInRow,
                             sHandle->mBlockMap[ 0 ],
                             ID_TRUE, // write lob len
                             aLength, // need size
                             &sHandle->mBlockBufferPosition,
                             &sHandle->mFilePosition,
                             &sAllocatedSlotValLen )
              != IDE_SUCCESS );

    sHandle->mRemainLobLength      = aLength;
    sHandle->mAllocatedLobSlotSize = sAllocatedSlotValLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  prepareLob 후, lob에 대한 write를 수행한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *  aLobPieceLength     - [IN] 기록할 Lob 조각의 길이
 *  aLobPieceValue      - [IN] 기록할 Lob 조각 값
 *
 **********************************************************************/
static IDE_RC scpfWriteLob( idvSQL         * aStatistics,
                            void           * aHandle,
                            UInt             aLobPieceLength,
                            UChar          * aLobPieceValue )
{
    scpfHandle    * sHandle;

    IDE_DASSERT( aHandle        != NULL );
    IDE_DASSERT( aLobPieceValue != NULL );

    sHandle = (scpfHandle*)aHandle;

    // 남은 Lob의 길이라 생각했던 것 보다 더 많은 LobPiece이 들어올 수 없다.
    // 이는 이 모듈을 잘못 사용했을때 나타나는 결과이다.
    IDE_TEST_RAISE( sHandle->mRemainLobLength < aLobPieceLength,
                    ERR_ABORT_PREPARE_WRITE_PROCOTOL );

    IDE_TEST( scpfWriteLobColumn( aStatistics,
                                  sHandle,
                                  aLobPieceLength,
                                  aLobPieceValue,
                                  &sHandle->mRemainLobLength,
                                  &sHandle->mAllocatedLobSlotSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_PREPARE_WRITE_PROCOTOL);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_PrePareWriteProtocol ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  prepareLob, Writelob 후 마지막으로 Writing을 완료한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *  aLobPieceLength     - [IN] 기록할 Lob 조각의 길이
 *  aLobPieceValue      - [IN] 기록할 Lob 조각 값
 *
 **********************************************************************/
static IDE_RC scpfFinishLobWriting( idvSQL         * /*aStatistics*/,
                                    void           * aHandle )
{
    scpfHandle    * sHandle;

    IDE_DASSERT( aHandle        != NULL );

    sHandle = (scpfHandle*)aHandle;

    // Lob기록이 완료되어야함. 그렇지 않으면 모듈을 잘못 사용한 경우.
    IDE_TEST_RAISE( sHandle->mRemainLobLength > 0,
                    ERR_ABORT_FINISH_WRITE_PROCOTOL );

    sHandle->mRemainLobColumnCount --;

    // Row 기록 완료
    if( sHandle->mRemainLobColumnCount == 0 )
    {
        sHandle->mFileHeader.mRowCount ++;
        sHandle->mRowSeq               ++;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_FINISH_WRITE_PROCOTOL);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_FinishWriteProtocol ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description :
 *  export를 종료한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *
 **********************************************************************/

static IDE_RC scpfEndExport( idvSQL         * aStatistics,
                             void          ** aHandle )
{
    scpfHandle    * sHandle;

    IDE_DASSERT( aHandle        != NULL );

    sHandle                                = (scpfHandle*)(*aHandle);
    sHandle->mFileHeader.mLastRowSeqInFile = sHandle->mRowSeq;

    IDE_TEST( scpfCloseFile4Export( aStatistics, 
                                    sHandle )
              != IDE_SUCCESS);

    IDE_TEST( scpfDestroy4Export( aStatistics,
                                  (scpfHandle**) aHandle )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  import를 시작한다.
 *
 *  aStatistics         - [IN]  통계정보
 *  aHandle             - [IN]  Object Handle
 *  aCommonHeader       - [IN/OUT] File로부터 읽은 Header 정보
 *  aFirstRowSeq        - [IN]  Import할 첫번째 Row
 *  aFirstRowSeq        - [IN]  Export할 첫번째 Row
 *  aObjName            - [IN]  Object 이름
 *
 **********************************************************************/
static IDE_RC scpfBeginImport( idvSQL              * aStatistics,
                               void               ** aHandle,
                               smiDataPortHeader   * aCommonHeader,
                               SLong                 aFirstRowSeq,
                               SLong                 aLastRowSeq,
                               SChar               * aObjName,
                               SChar               * aDirectory )
{
    scpfHandle         * sHandle;

    IDE_DASSERT( aHandle                   != NULL );
    IDE_DASSERT( aCommonHeader             != NULL );
    IDE_DASSERT( aObjName                  != NULL );

    // Handle을 초기화하고 파일의 Header를 읽는다.
    IDE_TEST( scpfInit4Import( aStatistics, 
                               (scpfHandle**)aHandle, 
                               aCommonHeader,
                               aFirstRowSeq,
                               aLastRowSeq,
                               aObjName,
                               aDirectory )
              != IDE_SUCCESS);

    sHandle = (scpfHandle*)*aHandle;

    IDE_TEST( scpfFindFirstBlock( aStatistics,
                                  &sHandle->mFile,
                                  sHandle->mCommonHandle.mHeader,
                                  sHandle->mFileHeader.mFirstRowSeqInFile,
                                  sHandle->mRowSeq,
                                  sHandle->mFileHeader.mBlockCount,
                                  sHandle->mFileHeader.mFileHeaderSize,
                                  sHandle->mFileHeader.mBlockHeaderSize,
                                  sHandle->mFileHeader.mBlockSize,
                                  sHandle->mBlockMap,
                                  &sHandle->mBlockBufferPosition )
              != IDE_SUCCESS );

    // Block을 전부 읽는다
    IDE_TEST( scpfPreFetchBlock( aStatistics,
                                 &sHandle->mFile,
                                 sHandle->mCommonHandle.mHeader,
                                 sHandle->mFileName,
                                 sHandle->mRowSeq,
                                 sHandle->mFileHeader.mBlockCount,
                                 sHandle->mFileHeader.mFileHeaderSize,
                                 sHandle->mFileHeader.mBlockHeaderSize,
                                 sHandle->mFileHeader.mBlockSize,
                                 sHandle->mBlockBufferPosition.mID,
                                 &sHandle->mFilePosition,
                                 sHandle->mFileHeader.mBlockInfoCount, //max
                                 sHandle->mFileHeader.mBlockInfoCount, //read
                                 sHandle->mBlockMap )
              != IDE_SUCCESS );


    IDU_FIT_POINT( "1.PROJ-2059@scpfModule::scpfBeginImport" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  row들을 읽는다.
 *
 *  1) Block을 읽는다.
 *  1-1) 만약 Build된 Row가 없다면, 처음 파일을 읽는것이므로 첫번째
 *       Row를 가진 Block을 BinarySearch로 찾아 거기서부터 시작한다.
 *  1-2) 처음이 아니라면, 다음 Block을 읽는다.
 *  2) Build된 Block들을 넘겨준다.
 *
 *  aStatistics         - [IN]  통계정보
 *  aHandle             - [IN]  Object Handle
 *  aRows               - [OUT] 읽은 Row
 *  aRowCount           - [OUT] 읽은 Row의 개수
 *
 **********************************************************************/
static IDE_RC scpfRead( idvSQL         * aStatistics,
                        void           * aHandle,
                        smiRow4DP     ** aRows,
                        UInt           * aRowCount )
{
    scpfHandle         * sHandle;
    smiDataPortHeader  * sCommonHeader;
    scpfHeader         * sFileHeader;
    scpfBlockInfo      * sBlockInfo;
    UInt                 sSkipRowCount;
    UInt                 sBuiltRowCount;
    idBool               sHasSupplementLob;
    SLong                sFirstRowSeqInBlock;

    IDE_DASSERT( aHandle   != NULL );
    IDE_DASSERT( aRows     != NULL );
    IDE_DASSERT( aRowCount != NULL );

    sHandle       = (scpfHandle*)aHandle;
    sCommonHeader = sHandle->mCommonHandle.mHeader;
    sFileHeader   = &(sHandle->mFileHeader);

    *aRowCount                     = 0;
    sHandle->mRemainLobColumnCount = 0;
    sHasSupplementLob              = ID_FALSE;

    // Row개수가 Limit을 넘어갈 경우
    IDE_TEST_CONT( sHandle->mRowSeq >= sHandle->mLimitRowSeq,
                    NO_MORE_ROW);

    scpfShiftBlock( sHandle->mBlockBufferPosition.mSeq,
                    sFileHeader->mBlockInfoCount,
                    sHandle->mBlockMap );

    IDE_TEST( scpfPreFetchBlock( aStatistics,
                                 &sHandle->mFile,
                                 sHandle->mCommonHandle.mHeader,
                                 sHandle->mFileName,
                                 sHandle->mRowSeq,
                                 sFileHeader->mBlockCount,
                                 sFileHeader->mFileHeaderSize,
                                 sFileHeader->mBlockHeaderSize,
                                 sFileHeader->mBlockSize,
                                 sHandle->mBlockBufferPosition.mID,
                                 &sHandle->mFilePosition,
                                 sHandle->mFileHeader.mBlockInfoCount,
                                 sHandle->mBlockBufferPosition.mSeq,
                                 sHandle->mBlockMap )
              != IDE_SUCCESS );

    sHandle->mBlockBufferPosition.mSeq = 0;

    // mRowSeqOfFirstSlot => Block내 첫번째 Slot이 어떤 RowSeq에 속하는가
    // sFirstRowSeqInBlock => 이 Block에서부터 시작되는 Row의 Seq
    //
    // ex)
    //    Block 0{ <Row0><Row1><Row2| }
    //    Blcok 1{ |Row2><Row3><Row4> }
    //      위 예제에서 Row2가 Block 0~1 사이에 걸쳐 기록(Overflow)되어 있다.
    //
    //      Block1의 mRowSeqOfFirstSlot는 2이다.
    //          왜냐하면, Block1의 첫 부분이 Row2의 Slot이기 때문이다.
    //      Block1의 sFirstRowSeqInBlock는 3이다.
    //          왜냐하면, Block1에서 온전히 시작하는 Row는 Row3이기 때문이다.
    //
    sBlockInfo              = sHandle->mBlockMap[0];

    // Read시 현 Block의 FirstValue부터 읽어야 하기 때문에
    // Overflowed된 Slot만큼 건너뛴다.
    sHandle->mBlockBufferPosition.mSlotSeq  = sBlockInfo->mFirstRowSlotSeq;
    sHandle->mBlockBufferPosition.mOffset   = sBlockInfo->mFirstRowOffset;

    if( 0 < sBlockInfo->mFirstRowSlotSeq )
    {
        // 이전 Block에서 Overflow된 Slot가 있으면, 
        // FirstRow는 그 다음 Row이다.
        sFirstRowSeqInBlock = sBlockInfo->mRowSeqOfFirstSlot + 1;
    }
    else
    {
        sFirstRowSeqInBlock = sBlockInfo->mRowSeqOfFirstSlot ;
    }

    // Block 중간의 Row부터 원하고 있다면
    if( sFirstRowSeqInBlock < sHandle->mRowSeq )
    {
        //Skip해야 하는 Row의 개수를 계산한다.
        sSkipRowCount = sHandle->mRowSeq - sFirstRowSeqInBlock;
    }
    else
    {
        sSkipRowCount = 0;
    }

    // Block의 Row를 Build한다.
    IDE_TEST( scpfBuildRow( aStatistics, 
                            sHandle,
                            sCommonHeader,
                            sFileHeader,
                            &sHandle->mBlockBufferPosition,
                            sHandle->mBlockMap,
                            sHandle->mChainedSlotBuffer,
                            &sHandle->mUsedChainedSlotBufSize,
                            &sBuiltRowCount,
                            &sHasSupplementLob,
                            sHandle->mRowList )
              !=IDE_SUCCESS );


    //Built된 row의 개수는, Block 하나가 가진 최대 Row개수보다 클 수 없다.
    IDE_TEST_RAISE( sBuiltRowCount > sFileHeader->mMaxRowCountInBlock,
                    ERR_ABORT_CORRUPTED_BLOCK );

    // BuiltRowCount는 반드시 SkipRowCount와 같거나 커야 한다.
    // SkipRowCount는 <Block내 Row번호> - <Block내 첫Row번호>로
    // <Block내 Row번호>인 mRowSeq가 Block내의 Row만 제대로 가
    // 가르키면, <Block의 총 Row개수>인 mBuiltRowCount보다 많을
    // 수 없기 때문이다.
    IDE_TEST_RAISE( sBuiltRowCount < sSkipRowCount,
                    ERR_ABORT_CORRUPTED_BLOCK );

    // BuildRow를 하면, Block내의 모든 Row를 가져다준다.
    // 그중 필요한 부분만 남기고 앞쪽은 Skip한다.
    *aRows            = &(sHandle->mRowList[ sSkipRowCount ]);
    *aRowCount        = sBuiltRowCount - sSkipRowCount;

    // Block 중간의 Row까지만 원하고 있다면,
    // RowCount를 줄여서 올려보내는 Row의 개수를 줄인다.
    if( (sHandle->mRowSeq + *aRowCount) > sHandle->mLimitRowSeq )
    {
        // First와 Last가 역전되지 않는 이상, 다음과 같은 오류는 있을 수 없다.
        IDE_TEST_RAISE( sHandle->mLimitRowSeq < sHandle->mRowSeq,
                        ERR_ABORT_CORRUPTED_BLOCK );

        *aRowCount = sHandle->mLimitRowSeq - sHandle->mRowSeq;
    }

    // 처리된 Row개수를 반영한다. 
    sHandle->mRowSeq += sBuiltRowCount - sSkipRowCount;

    // 추가 Lob처리를 해줘야 한다면
    if( sHasSupplementLob == ID_TRUE )
    {
        // 아직 Row 하나가 처리 완료 안되었다.
        sHandle->mRowSeq --;

        sHandle->mRemainLobColumnCount = sCommonHeader->mLobColumnCount;
        sHandle->mRemainLobLength      = 0;
    }

    // Row가 없는 경우
    IDE_EXCEPTION_CONT( NO_MORE_ROW );

    sHandle->mRemainLobLength = 0;

    IDU_FIT_POINT( "1.PROJ-2059@scpfModule::scpfRead" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_CORRUPTED_BLOCK,
                sBlockInfo->mBlockID ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  Lob의 총 길이를 반환한다.
 *
 *  aStatistics         - [IN]  통계정보
 *  aHandle             - [IN]  Object Handle
 *  aLength             - [OUT] Lob의 총 길이
 *
 **********************************************************************/
static IDE_RC scpfReadLobLength( idvSQL         * aStatistics,
                                 void           * aHandle,
                                 UInt           * aLength )
{
    scpfBlockInfo           * sBlockInfo;
    scpfHandle              * sHandle;
    scpfHeader              * sFileHeader;
    scpfBlockBufferPosition * sBlockBufferPosition;

    IDE_DASSERT( aHandle                   != NULL );
    IDE_DASSERT( aLength                   != NULL );

    sHandle              = (scpfHandle*)aHandle;
    sBlockBufferPosition = &(sHandle->mBlockBufferPosition);
    sFileHeader          = &(sHandle->mFileHeader);

    // 새롭게 LobColumn을 읽을 준비가 되었어야 함.
    // 그렇지 않으면 모듈을 잘못 사용한 경우.
    IDE_TEST_RAISE( ( sHandle->mRemainLobLength != 0 ) ||
                    ( sHandle->mRemainLobColumnCount == 0),
                    ERR_ABORT_PREPARE_WRITE_PROCOTOL );

    scpfShiftBlock( sHandle->mBlockBufferPosition.mSeq,
                    sFileHeader->mBlockInfoCount,
                    sHandle->mBlockMap );

    IDE_TEST( scpfPreFetchBlock( aStatistics,
                                 &sHandle->mFile,
                                 sHandle->mCommonHandle.mHeader,
                                 sHandle->mFileName,
                                 sHandle->mRowSeq,
                                 sFileHeader->mBlockCount,
                                 sFileHeader->mFileHeaderSize,
                                 sFileHeader->mBlockHeaderSize,
                                 sFileHeader->mBlockSize,
                                 sHandle->mBlockBufferPosition.mID,
                                 &sHandle->mFilePosition,
                                 sHandle->mFileHeader.mBlockInfoCount,
                                 sHandle->mBlockBufferPosition.mSeq,
                                 sHandle->mBlockMap )
              != IDE_SUCCESS );

    sHandle->mBlockBufferPosition.mSeq = 0;

    sBlockInfo = sHandle->mBlockMap[ sBlockBufferPosition->mSeq ];

    // LobLen을 읽음
    SCPF_READ_UINT( sBlockInfo->mBlockPtr + sBlockBufferPosition->mOffset, 
                    &(sHandle->mRemainLobLength) );
    sBlockBufferPosition->mOffset += ID_SIZEOF(UInt);
    *aLength = sHandle->mRemainLobLength;

    IDU_FIT_POINT( "1.PROJ-2059@scpfModule::scpfReadLobLength" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_PREPARE_WRITE_PROCOTOL);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_PrePareWriteProtocol ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  LobPiece를 읽는다.
 *
 *  aStatistics         - [IN]  통계정보
 *  aHandle             - [IN]  Object Handle
 *  aLobPieceLength     - [OUT] 읽은 Lob 조각의 길이
 *  aLobPieceValue      - [OUT] 읽은 Lob 조각의 Value
 *
 **********************************************************************/
static IDE_RC scpfReadLob( idvSQL         * aStatistics,
                           void           * aHandle,
                           UInt           * aLobPieceLength,
                           UChar         ** aLobPieceValue )
{
    scpfHandle              * sHandle;
    scpfHeader              * sFileHeader;
    scpfBlockBufferPosition * sBlockBufferPosition;

    IDE_DASSERT( aHandle                   != NULL );
    IDE_DASSERT( aLobPieceLength           != NULL );
    IDE_DASSERT( aLobPieceValue            != NULL );

    sHandle              = (scpfHandle*)aHandle;
    sFileHeader          = &(sHandle->mFileHeader);
    sBlockBufferPosition = &(sHandle->mBlockBufferPosition);

    //RemainLobLength가 0인 것은 본 Module을 잘못 사용했을때이다.
    IDE_TEST_RAISE( (sHandle->mRemainLobLength == 0) ||
                    (sHandle->mRemainLobColumnCount == 0),
                    ERR_ABORT_FINISH_WRITE_PROCOTOL );

    scpfShiftBlock( sHandle->mBlockBufferPosition.mSeq,
                    sFileHeader->mBlockInfoCount,
                    sHandle->mBlockMap );

    IDE_TEST( scpfPreFetchBlock( aStatistics,
                                 &sHandle->mFile,
                                 sHandle->mCommonHandle.mHeader,
                                 sHandle->mFileName,
                                 sHandle->mRowSeq,
                                 sFileHeader->mBlockCount,
                                 sFileHeader->mFileHeaderSize,
                                 sFileHeader->mBlockHeaderSize,
                                 sFileHeader->mBlockSize,
                                 sHandle->mBlockBufferPosition.mID,
                                 &sHandle->mFilePosition,
                                 sHandle->mFileHeader.mBlockInfoCount,
                                 sHandle->mBlockBufferPosition.mSeq,
                                 sHandle->mBlockMap )
              != IDE_SUCCESS );

    sHandle->mBlockBufferPosition.mSeq = 0;

    //Slot하나 읽는다.
    IDE_TEST( scpfReadSlot( &sHandle->mBlockBufferPosition,
                            sHandle->mBlockMap,
                            sFileHeader->mBlockHeaderSize, 
                            aLobPieceLength,
                            (void**)aLobPieceValue )
              != IDE_SUCCESS);

    IDE_TEST_RAISE( sHandle->mRemainLobLength < *aLobPieceLength,
                    ERR_ABORT_CORRUPTED_BLOCK  );

    sHandle->mRemainLobLength -= *aLobPieceLength;



    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_FINISH_WRITE_PROCOTOL);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_FinishWriteProtocol ) );
    }
    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( 
            smERR_ABORT_CORRUPTED_BLOCK,
            sHandle->mBlockMap[ sBlockBufferPosition->mSeq ]->mBlockID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  readLob 후 마지막으로 Reading을 완료한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *  aLobPieceLength     - [IN] 기록할 Lob 조각의 길이
 *  aLobPieceValue      - [IN] 기록할 Lob 조각 값
 *
 **********************************************************************/
static IDE_RC scpfFinishLobReading( idvSQL         * /*aStatistics*/,
                                    void           * aHandle )
{
    scpfHandle              * sHandle;

    IDE_DASSERT( aHandle != NULL );

    sHandle = (scpfHandle*)aHandle;

    // Lob Reading이 완료되어야함. 그렇지 않으면 모듈을 잘못 사용한 경우.
    IDE_TEST_RAISE( (sHandle->mRemainLobLength > 0) ||
                    (sHandle->mRemainLobColumnCount == 0),
                    ERR_ABORT_FINISH_WRITE_PROCOTOL );

    sHandle->mRemainLobColumnCount --;

    // 한 Row에 대한 읽기가 완료되면
    if( sHandle->mRemainLobColumnCount == 0 )
    {
        sHandle->mRowSeq ++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_FINISH_WRITE_PROCOTOL);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_FinishWriteProtocol ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description :
 *  Import를 종료한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *
 **********************************************************************/

static IDE_RC scpfEndImport( idvSQL         * aStatistics,
                             void          ** aHandle )
{
    scpfHandle    * sHandle;

    IDE_DASSERT( aHandle        != NULL );

    sHandle                 = *((scpfHandle**)aHandle);

    IDE_TEST( scpfCloseFile4Import( aStatistics, 
                                    sHandle )
              != IDE_SUCCESS);

    IDE_TEST( scpfDestroy4Import( aStatistics,
                                  (scpfHandle**) aHandle )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}






/**********************************************************************
 * Internal Functions for Exporting
 **********************************************************************/

/***********************************************************************
 *
 * Description :
 *  Object Handle을 초기화한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN/OUT] Object Handle
 *  aCommonHeader       - [IN] File에 내릴 Header정보(Column,Table등 포함)
 *  aObjName            - [IN] Object 이름
 *  aSplit              - [IN] Split되는 기점 Row 개수
 *
 **********************************************************************/
static IDE_RC scpfInit4Export( idvSQL                * /*aStatistics*/,
                               scpfHandle           ** aHandle,
                               smiDataPortHeader     * aCommonHeader,
                               SChar                 * aObjName,
                               SChar                 * aDirectory,
                               SLong                   aSplit )
{
    scpfHandle        * sHandle;
    scpfHeader        * sFileHeader;
    scpfBlockInfo     * sBlockInfo;
    UChar             * sAlignedBlockPtr;
    UInt                sBlockSize;
    UInt                sState = 0;
    UInt                i;

    IDE_DASSERT( aHandle       != NULL );
    IDE_DASSERT( aCommonHeader != NULL );
    IDE_DASSERT( aObjName      != NULL );


    // Writable property 이기에 미리 Snapshot을 뜬다
    sBlockSize               = smuProperty::getDataPortFileBlockSize(); 

    // -----------------------------------------
    // Memory Allocation
    // -----------------------------------------
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 ID_SIZEOF(scpfHandle),
                                 (void**)&(sHandle) )
              != IDE_SUCCESS );
    *aHandle    = sHandle;
    sFileHeader = &sHandle->mFileHeader;
    sState = 1;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 ID_SIZEOF(scpfBlockInfo),
                                 (void**)&(sHandle->mBlockBuffer) )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 ID_SIZEOF(scpfBlockInfo*),
                                 (void**)&(sHandle->mBlockMap) ) 
              != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 sBlockSize + ID_MAX_DIO_PAGE_SIZE,
                                 (void**)&(sHandle->mAllocatedBlock) )
              != IDE_SUCCESS );
    sState = 4;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 ID_SIZEOF( UInt  ),
                                 aCommonHeader->mColumnCount,
                                 (void**)&(sHandle->mColumnMap ) )
              != IDE_SUCCESS );
    sState = 5;

    // DirectIO를 위해 Align을 맞춰줘야 합니다.
    sAlignedBlockPtr = (UChar*)idlOS::align( sHandle->mAllocatedBlock,
                                             ID_MAX_DIO_PAGE_SIZE );


    // --------------------------------------
    // Initialize attribute 
    // ------------------------------------- 
    // common header
    sHandle->mCommonHandle.mHeader   = aCommonHeader;

    // fileHeader
    sFileHeader->mBlockHeaderSize    = 
        smiDataPort::getEncodedHeaderSize(
            gScpfBlockHeaderDesc,
            aCommonHeader->mVersion );
    sFileHeader->mBlockSize          = sBlockSize;
    sFileHeader->mMaxRowCountInBlock = 1; //Block에는 최소 1개의 Row가 들어간다
    sFileHeader->mBlockInfoCount     = SCPF_MIN_BLOCKINFO_COUNT;

    // runtime
    sHandle->mSplit                      = aSplit;
    sHandle->mColumnChainingThreshold    = 
        smuProperty::getExportColumnChainingThreshold();

    idlOS::strncpy( (SChar*)sHandle->mDirectory, 
                    (SChar*)aDirectory,
                    SM_MAX_FILE_NAME );

    idlOS::strncpy( (SChar*)sHandle->mObjName, 
                    (SChar*)aObjName,
                    SM_MAX_FILE_NAME );

    // Export시에는 Block 1개만 사용함
    sBlockInfo            = &sHandle->mBlockBuffer[ 0 ];
    sBlockInfo->mBlockPtr = sAlignedBlockPtr;
    sHandle->mBlockMap[0] = sBlockInfo;

    // Column map
    for( i=0; i<aCommonHeader->mColumnCount; i++)
    {
        sHandle->mColumnMap[i] = 
            gSmiGlobalCallBackList.getColumnMapFromColumnHeaderFunc(
                aCommonHeader->mColumnHeader,
                i );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 5:
            (void) iduMemMgr::free( sHandle->mColumnMap );
        case 4:
            (void) iduMemMgr::free( sHandle->mAllocatedBlock );
        case 3:
            (void) iduMemMgr::free( sHandle->mBlockMap ); 
        case 2:
            (void) iduMemMgr::free( sHandle->mBlockBuffer );
        case 1:
            (void) iduMemMgr::free( sHandle ); 
            *aHandle = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  사용한 ObjectHandle을 제거한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *
 **********************************************************************/
static IDE_RC scpfDestroy4Export( idvSQL         * /*aStatistics*/,
                                  scpfHandle    ** aHandle )
{
    UInt sState = 5;

    sState = 4;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mAllocatedBlock ) 
              != IDE_SUCCESS );

    sState = 3;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mColumnMap )
              != IDE_SUCCESS );

    sState = 2;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mBlockMap  ) 
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mBlockBuffer ) 
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( *aHandle ) 
              != IDE_SUCCESS );
    *aHandle = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 5:
            (void) iduMemMgr::free( (*aHandle)->mAllocatedBlock );
        case 4:
            (void) iduMemMgr::free( (*aHandle)->mColumnMap );
        case 3:
            (void) iduMemMgr::free( (*aHandle)->mBlockMap );
        case 2:
            (void) iduMemMgr::free( (*aHandle)->mBlockBuffer );
        case 1:
            (void) iduMemMgr::free( *aHandle );
            *aHandle = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  기록할 파일을 준비한다 (Split)
 *
 * 1) 새 파일에 맞게 Header를 정리한다.
 * 2) 새 파일에createFile, writeHeader를 수행한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *
 **********************************************************************/
static IDE_RC scpfPrepareFile( idvSQL         * aStatistics,
                               scpfHandle     * aHandle )
{
    scpfBlockInfo           * sBlockInfo;
    scpfHeader              * sFileHeader;
    scpfBlockBufferPosition * sBlockBufferPosition;
    UInt                      sLen;
    UInt                      sFileState = 0;
    iduFile                 * sFile;


    IDE_DASSERT( aHandle                        != NULL );

    IDE_TEST_RAISE( (aHandle->mRemainLobLength > 0) ||
                    (aHandle->mRemainLobColumnCount > 0),
                    ERR_ABORT_FINISH_WRITE_PROCOTOL );

    // Export시에는 0번 Block만 사용한다
    sFileHeader          = &aHandle->mFileHeader;
    sBlockInfo           = aHandle->mBlockMap[0];
    sFile                = &(aHandle->mFile);
    sBlockBufferPosition = &(aHandle->mBlockBufferPosition);

    // Header reinitialize
    // persistent
    sFileHeader->mMaxRowCountInBlock  = 1;
    sFileHeader->mBlockCount          = 0;
    sFileHeader->mRowCount            = 0;
    sFileHeader->mFirstRowSeqInFile   = aHandle->mRowSeq;
    sFileHeader->mLastRowSeqInFile    = aHandle->mRowSeq;

    // runtime
    aHandle->mFilePosition.mOffset         = 0;
    aHandle->mRemainLobLength              = 0;
    aHandle->mRemainLobColumnCount         = 0;

    // Block
    sBlockBufferPosition->mID         = 0;
    sBlockBufferPosition->mOffset     = 0;
    sBlockBufferPosition->mSlotSeq    = 0;

    // 새 Block에 대한 초기화
    sBlockInfo->mRowSeqOfFirstSlot   = aHandle->mRowSeq;
    sBlockInfo->mRowSeqOfLastSlot    = aHandle->mRowSeq;
    sBlockInfo->mFirstRowSlotSeq     = 0;
    sBlockInfo->mFirstRowOffset      = SCPF_OFFSET_NULL;
    sBlockInfo->mHasLastChainedSlot  = ID_FALSE;
    sBlockInfo->mSlotCount           = 0;
    sBlockInfo->mCheckSum            = 0;
    sBlockInfo->mBlockID             = 0;

    // --------------------------------------
    // Create file 
    // ------------------------------------- 
    // Directory에 데이터가 있는데, /로 끝나지 않을 경우 붙여줌
    sLen = idlOS::strnlen( aHandle->mDirectory, SM_MAX_FILE_NAME );
    if( 0 < sLen )
    {
        if( aHandle->mDirectory[ sLen - 1 ] != IDL_FILE_SEPARATOR )
        {
            idlVA::appendFormat( aHandle->mDirectory,
                                 SM_MAX_FILE_NAME,
                                 "%c",
                                 IDL_FILE_SEPARATOR );
        }
    }

    idlOS::snprintf((SChar*)aHandle->mFileName, 
                    SM_MAX_FILE_NAME,
                    "%s%s%c%"ID_UINT32_FMT"%s",
                    aHandle->mDirectory, 
                    aHandle->mObjName, 
                    SCPF_DPFILE_SEPARATOR,
                    aHandle->mFilePosition.mIdx,
                    SCPF_DPFILE_EXT);

    IDE_TEST( sFile->initialize( IDU_MEM_SM_SCP,
                                 1, /* Max Open FD Count */
                                 IDU_FIO_STAT_OFF,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);
    sFileState = 1;

    IDE_TEST( sFile->setFileName( aHandle->mFileName )
              != IDE_SUCCESS);
    IDE_TEST( sFile->createUntilSuccess( smLayerCallback::setEmergency )
              != IDE_SUCCESS );
    if( smuProperty::getDataPortDirectIOEnable() == 1 )
    {
        IDE_TEST( sFile->open( ID_TRUE )  //DIRECT_IO
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sFile->open( ID_FALSE ) //DIRECT_IO
                  != IDE_SUCCESS );
    }
    sFileState = 2;
    aHandle->mOpenFile = ID_TRUE;

    // Header를 기록한다.
    IDE_TEST( scpfWriteHeader( aStatistics,
                               &aHandle->mFile,
                               aHandle->mCommonHandle.mHeader,
                               &aHandle->mFileHeader,
                               &aHandle->mBlockBufferPosition,
                               &aHandle->mFilePosition )
              != IDE_SUCCESS);

    IDE_TEST( scpfPrepareBlock( aStatistics,
                                &aHandle->mFileHeader,
                                aHandle->mRowSeq,
                                sBlockInfo,
                                &aHandle->mBlockBufferPosition )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_FINISH_WRITE_PROCOTOL);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_FinishWriteProtocol ) );
    }
    IDE_EXCEPTION_END;

    switch( sFileState )
    {
        case 2:
            (void) sFile->close() ;
            aHandle->mOpenFile = ID_FALSE;
        case 1:
            (void) sFile->destroy();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  지금까지 기록한 파일을 닫는다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *
 **********************************************************************/
static IDE_RC scpfCloseFile4Export( idvSQL         * aStatistics,
                                    scpfHandle     * aHandle )
{
    scpfBlockInfo   * sBlockInfo;
    scpfHeader      * sFileHeader;
    UInt              sFileState = 2;

    IDE_DASSERT( aHandle                        != NULL );

    // Export시에는 0번 Block만 사용한다
    sFileHeader  = &aHandle->mFileHeader;
    sBlockInfo   = aHandle->mBlockMap[0];

    sFileHeader->mLastRowSeqInFile = aHandle->mRowSeq;

    IDE_TEST( scpfWriteBlock( aStatistics, 
                              &aHandle->mFile,
                              aHandle->mCommonHandle.mHeader,
                              &aHandle->mFileHeader,
                              aHandle->mRowSeq,
                              sBlockInfo,
                              &aHandle->mBlockBufferPosition,
                              &aHandle->mFilePosition )
              != IDE_SUCCESS);

    IDE_TEST( scpfWriteHeader( aStatistics,
                               &aHandle->mFile,
                               aHandle->mCommonHandle.mHeader,
                               &aHandle->mFileHeader,
                               &aHandle->mBlockBufferPosition,
                               &aHandle->mFilePosition )
              != IDE_SUCCESS);

    IDE_TEST( aHandle->mFile.sync() != IDE_SUCCESS);

    sFileState = 1;
    IDE_TEST( aHandle->mFile.close() != IDE_SUCCESS);

    aHandle->mOpenFile = ID_FALSE;
    sFileState = 0;
    IDE_TEST( aHandle->mFile.destroy() != IDE_SUCCESS);

    aHandle->mFilePosition.mIdx ++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sFileState )
    {
        case 2:
            (void) aHandle->mFile.close() ;
            aHandle->mOpenFile = ID_FALSE;
        case 1:
            (void) aHandle->mFile.destroy() ;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  Slot을 쓸 공간을 확보한다.
 *
 *  aStatistics              - [IN] 통계정보
 *  aFile                    - [IN] 기록할 file
 *  aFileHeader              - [IN] 기록할 File에 관한 정보 헤더
 *  aRowSeq                  - [IN] 현재 기록중인 Row 번호
 *  aColumnChainingThreshold - [IN] Column을 자르는 기준 크기
 *  aIsFirstSlotInRow        - [IN/OUT] Block내 첫번째 Slot을 기록하는 중인가?
 *  aBlockInfo               - [IN] 사용하고 있는 BlockInfo
 *  aWriteLobLen             - [IN] LobLength을 기록할 것인가?
 *  aNeedSize                - [IN] 확보하려는 칼럼의 크기
 *  aBlockBufferPosition     - [OUT]    다음번 Block으로 커서 옮김
 *  aFilePosition            - [IN/OUT] 기록할 현재 Block의 위치를 가져오고
 *                                      기록한 Block크기만큼 Offset을 옮김
 *  aAllocatedSlotValLen     - [OUT]    확보된 SlotVal 기록용 공간의 크기
 *
 **********************************************************************/
static IDE_RC scpfAllocSlot( 
                idvSQL                  * aStatistics,
                iduFile                 * aFile,
                smiDataPortHeader       * aCommonHeader,
                scpfHeader              * aFileHeader,
                SLong                     aRowSeq,
                UInt                      aColumnChainingThreshold,
                idBool                  * aIsFirstSlotInRow,
                scpfBlockInfo           * aBlockInfo,
                idBool                    aWriteLobLen,
                UInt                      aNeedSlotValLen,
                scpfBlockBufferPosition * aBlockBufferPosition,
                scpfFilePosition        * aFilePosition,
                UInt                    * aAllocatedSlotValLen )
{
    UInt     sBlockRemainSize;
    UInt     sNeedSize;

    IDE_DASSERT( aFile               != NULL );
    IDE_DASSERT( aFileHeader         != NULL );
    IDE_DASSERT( aFilePosition       != NULL );
    IDE_DASSERT( aBlockInfo          != NULL );
    IDE_DASSERT( aFilePosition       != NULL );
    IDE_DASSERT( aBlockBufferPosition!= NULL );

    sBlockRemainSize = aFileHeader->mBlockSize - aBlockBufferPosition->mOffset;

    sNeedSize = SCPF_GET_VARINT_SIZE(&aNeedSlotValLen) + aNeedSlotValLen;
    if( aWriteLobLen == ID_TRUE )
    {
        sNeedSize += SCPF_LOB_LEN_SIZE;
    }

    // 기록할 공간이 부족하며,
    // 남은 공간이 Threshold이하일 경우,
    // 기존 Block을 쓰고 새 Block을 준비하여 Threshold이상의 공간을 확보한다.
    if( (sNeedSize > sBlockRemainSize) &&
        (aColumnChainingThreshold > sBlockRemainSize ) )
    {
        IDE_TEST( scpfWriteBlock( aStatistics, 
                                  aFile,
                                  aCommonHeader,
                                  aFileHeader,
                                  aRowSeq,
                                  aBlockInfo,
                                  aBlockBufferPosition,
                                  aFilePosition )
                  != IDE_SUCCESS );

        IDE_TEST( scpfPrepareBlock( aStatistics, 
                                    aFileHeader,
                                    aRowSeq,
                                    aBlockInfo,
                                    aBlockBufferPosition )
                  != IDE_SUCCESS );

        sBlockRemainSize = 
            aFileHeader->mBlockSize - aBlockBufferPosition->mOffset;

        //새 Block을 마련했으면 최소한의 공간이 확보되어야 함
        IDE_TEST_RAISE( aColumnChainingThreshold > sBlockRemainSize,
                        ERR_ABORT_CORRUPTED_BLOCK  );
    }

    // 이번이 Row에 대한 첫 기록이면
    if( *aIsFirstSlotInRow == ID_TRUE )
    {
        *aIsFirstSlotInRow = ID_FALSE;

        // 현 Block에서의 첫번째 Row이기도 하다면
        if( aBlockInfo->mFirstRowOffset == SCPF_OFFSET_NULL )
        {
            aBlockInfo->mFirstRowOffset  = aBlockBufferPosition->mOffset;
            aBlockInfo->mFirstRowSlotSeq = aBlockBufferPosition->mSlotSeq ;
        }
    }

    // Slot 하나가 확보됨
    aBlockInfo->mSlotCount ++;
    aBlockBufferPosition->mSlotSeq ++;



    // LobLen 기록
    if( aWriteLobLen == ID_TRUE )
    {
        SCPF_WRITE_UINT( &aNeedSlotValLen, 
                         aBlockInfo->mBlockPtr 
                             + aBlockBufferPosition->mOffset );

        aBlockBufferPosition->mOffset += SCPF_LOB_LEN_SIZE;
        sNeedSize                     -= SCPF_LOB_LEN_SIZE;

        sBlockRemainSize = 
            aFileHeader->mBlockSize - aBlockBufferPosition->mOffset;
    }



    //SlotValLen 기록
    // 공간이 부족할 경우
    if( sBlockRemainSize < sNeedSize )
    {
        *aAllocatedSlotValLen = SCPF_GET_SLOT_VAL_LEN( &sBlockRemainSize );
    }
    else
    {
        //넉넉할 경우
        *aAllocatedSlotValLen = aNeedSlotValLen;
    }

    SCPF_WRITE_VARINT( aAllocatedSlotValLen,
                       aBlockInfo->mBlockPtr 
                           + aBlockBufferPosition->mOffset );
    aBlockBufferPosition->mOffset += 
                       SCPF_GET_VARINT_SIZE( aAllocatedSlotValLen );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_CORRUPTED_BLOCK,
                aBlockInfo->mBlockID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  Slot 하나를 Block에 기록한다.
 *  Chaining, overflow, Block의 실제 File에 대한 기록도 수행한다.
 *
 *  aStatistics             - [IN]  통계정보
 *  aFile                   - [IN]  기록할 파일
 *  aFileHeader             - [IN]  파일 헤더 정보
 *  aBlockInfo              - [IN]  기록할 Block을 담은 Info
 *  aBlockSize              - [IN]  블륵 하나의 크기
 *  aColumnChainingThreshold- [IN]  Slot Chaining을 결정하는 Threshold
 *  aRowSeq                 - [IN]  현재 기록하는 Row의 번호
 *  aValue                  - [IN]  기록할 Slot 
 *  aIsFirstSlotInRow       - [IN/OUT] Row내 첫번째 Slot 를 기록하는 것인가?
 *  aBlockBufferPosition    - [IN/OUT] 기록하고 Position를 옮긴다.
 *  aFilePosition           - [IN/OUT] 기록하고 Position를 옮긴다.
 * 
 * Slot 의 구조
 * - VarInt (Length 기록에 이용됨)
 *      0~63        [0 | len ]
 *     64~16383     [1 | len>>8  & 63][len ]
 *  16384~4194303   [2 | len>>16 & 63][len>>8 & 255][len & 255]
 *4194304~1073741824[3 | len>>24 & 63][len>>16 & 255][len>>8 & 255][len & 255]
 *
 * - BasicColumn
 *   [Slot_Len:VarInt(1~4Byte)][Body]
 * 
 *   Slot_Len는 Length로, 순수 Body의 크기이다. 또한 Chaining돼었을 경우, 
 * 현 Block에 기록된 Length만 기록된다.
 *   BasicColumn은 LobLength가 0으로 오며, 기록되지 않는다.
 **********************************************************************/

static IDE_RC scpfWriteBasicColumn( 
            idvSQL                  * aStatistics,
            iduFile                 * aFile,
            smiDataPortHeader       * aCommonHeader,
            scpfHeader              * aFileHeader,
            scpfBlockInfo           * aBlockInfo,
            UInt                      aColumnChainingThreshold,
            SLong                     aRowSeq,
            smiValue                * aValue,
            idBool                  * aIsFirstSlotInRow,
            scpfBlockBufferPosition * aBlockBufferPosition,
            scpfFilePosition        * aFilePosition )
{
    UInt              sRemainLength;
    UChar           * sRemainValue;
    UInt              sAllocatedSlotValLen;
    idBool            sDone = ID_FALSE;

    IDE_DASSERT( aFile                    != NULL );
    IDE_DASSERT( aFileHeader              != NULL );
    IDE_DASSERT( aBlockInfo               != NULL );
    IDE_DASSERT( aIsFirstSlotInRow        != NULL );
    IDE_DASSERT( aValue                   != NULL );
    IDE_DASSERT( aBlockBufferPosition     != NULL );
    IDE_DASSERT( aFilePosition            != NULL );

    // Export시에는 0번 Block만 사용한다
    sRemainLength = aValue->length;
    sRemainValue  = (UChar*)aValue->value;

    while( sDone == ID_FALSE )
    {
        // Slot 확보
        IDE_TEST( scpfAllocSlot( aStatistics, 
                                 aFile,
                                 aCommonHeader,
                                 aFileHeader,
                                 aRowSeq,
                                 aColumnChainingThreshold,
                                 aIsFirstSlotInRow,
                                 aBlockInfo,
                                 ID_FALSE,       // write lob len
                                 sRemainLength,  // need size
                                 aBlockBufferPosition,
                                 aFilePosition,
                                 &sAllocatedSlotValLen )
                  != IDE_SUCCESS );

        // Write Slot Val
        idlOS::memcpy( aBlockInfo->mBlockPtr + aBlockBufferPosition->mOffset,
                       sRemainValue,
                       sAllocatedSlotValLen );
        aBlockBufferPosition->mOffset   += sAllocatedSlotValLen;
        sRemainValue                    += sAllocatedSlotValLen;
        sRemainLength                   -= sAllocatedSlotValLen;

        // 아직 기록할 데이터가 남았다면 Chaining
        if( 0 < sRemainLength )
        {
            // Block의 여유 공간이 없는 상태여야 한다.
            IDE_TEST_RAISE( aFileHeader->mBlockSize 
                                - aBlockBufferPosition->mOffset 
                                >= SCPF_GET_VARINT_MAXSIZE,
                            ERR_ABORT_CORRUPTED_BLOCK  );
            aBlockInfo->mHasLastChainedSlot = ID_TRUE;
        }
        else
        {
            sDone = ID_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( 
                    smERR_ABORT_CORRUPTED_BLOCK,
                    aBlockInfo->mBlockID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  LobValue를 Block에 기록한다.
 *
 *  aStatistics          - [IN]     통계정보
 *  aHandle              - [IN]     Object Handle
 *  aLobPieceLength      - [IN]     기록할 Lob조각 길이
 *  aLobPieceValue       - [IN]     기록할 Lob조각
 *  aRemainLobLength     - [IN/OUT] 남은 Lob 전체의 크기
 *  aAllocatedLobSlotSize- [IN/OUT] 현 Block에서 예약된 Lob 기록 공간의 크기
 *
 * - Lob Column
 *   [LobLen (4byte)][Slot_Len(1~4Byte)][Body]
 *   Slot_Len는 역시 순수 Body의 크기. 즉 LobLen의 4Byte는 포함되지 않는다.
 *   여기서 LobLen는 Slot 가 Chaining된 것과 상관 없이, 하나의 LobColumn
 * 전체의 길이이다. 만약 Lob이 Chaining되면, 첫번째 Slot 에만 기록한다.
 *
 ************************************************************************/

static IDE_RC scpfWriteLobColumn( idvSQL         * aStatistics,
                                  scpfHandle     * aHandle,
                                  UInt             aLobPieceLength,
                                  UChar          * aLobPieceValue,
                                  UInt           * aRemainLobLength,
                                  UInt           * aAllocatedLobSlotValLen )
{
    scpfBlockBufferPosition * sBlockBufferPosition;
    scpfBlockInfo           * sBlockInfo;
    UInt                      sBodySize;
    UInt                      sRemainLobPieceLength;
    UChar                   * sRemainLobPieceValue;

    IDE_DASSERT( aHandle         != NULL );

    // Export시에는 0번 Block만 사용한다
    sBlockInfo              = aHandle->mBlockMap[0];
    sBlockBufferPosition    = &(aHandle->mBlockBufferPosition);

    sRemainLobPieceLength = aLobPieceLength;
    sRemainLobPieceValue  = aLobPieceValue;

    // 현 LobPiece를 모두 기록할때까지 반복한다.
    while( 0 < sRemainLobPieceLength )
    {
        //Write Lob piece
        sBodySize = IDL_MIN( sRemainLobPieceLength,*aAllocatedLobSlotValLen );
        idlOS::memcpy( sBlockInfo->mBlockPtr + sBlockBufferPosition->mOffset,
                       sRemainLobPieceValue,
                       sBodySize );
        sBlockBufferPosition->mOffset   += sBodySize;
        sRemainLobPieceValue            += sBodySize;
        sRemainLobPieceLength           -= sBodySize;
        *aRemainLobLength               -= sBodySize;
        *aAllocatedLobSlotValLen        -= sBodySize;

        // Slot하나 다썼는데, 아직 기록할 Lob이 있으면
        // 새 Slot 확보
        if( ( 0 == *aAllocatedLobSlotValLen ) &&
            ( 0 <  *aRemainLobLength  ) )
        {
            sBlockInfo->mHasLastChainedSlot = ID_TRUE;

            // Slot 확보
            IDE_TEST( scpfAllocSlot( 
                    aStatistics, 
                    &aHandle->mFile,
                    aHandle->mCommonHandle.mHeader,
                    &aHandle->mFileHeader,
                    aHandle->mRowSeq,
                    aHandle->mColumnChainingThreshold,
                    &aHandle->mIsFirstSlotInRow,
                    sBlockInfo,
                    ID_FALSE,          // write lob len
                    *aRemainLobLength, // need size
                    &aHandle->mBlockBufferPosition,
                    &aHandle->mFilePosition,
                    aAllocatedLobSlotValLen )
                != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  Block을 File에 Write한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aFile               - [IN] 기록할 file
 *  aCommonHeader       - [IN] 공통헤더
 *  aFileHeader         - [IN] 기록할 File에 관한 정보 헤더
 *  aRowSeq             - [IN] 현재 기록중인 Row 번호
 *  aBlockInfo          - [IN] 기록할 Block
 *  aBlockBufferPosition- [OUT]    다음번 Block으로 커서 옮김
 *  aFilePosition       - [IN/OUT] 기록할 현재 Block의 위치를 가져오고
 *                                 기록한 Block크기만큼 Offset을 옮김
 *
 **********************************************************************/
static IDE_RC scpfWriteBlock( idvSQL                  * aStatistics,
                              iduFile                 * aFile,
                              smiDataPortHeader       * aCommonHeader,
                              scpfHeader              * aFileHeader,
                              SLong                     aRowSeq,
                              scpfBlockInfo           * aBlockInfo,
                              scpfBlockBufferPosition * aBlockBufferPosition,
                              scpfFilePosition        * aFilePosition )
{
    UInt            sBlockOffset;
#if defined(DEBUG)
    SChar         * sTempBuf;
    UInt i;
#endif

    IDE_DASSERT( aFile         != NULL );
    IDE_DASSERT( aFileHeader   != NULL );
    IDE_DASSERT( aFilePosition != NULL );
    IDE_DASSERT( aBlockInfo    != NULL );

    // 현 Block에서부터 시작된 Row가 없는 경우, 
    // FirstRowOffset이 설정되지 않아 NULL이다.
    if( aBlockInfo->mFirstRowOffset == SCPF_OFFSET_NULL )
    {
        // 이전 Block으로 부터 넘어온 Slot 만 있다
        aBlockInfo->mFirstRowSlotSeq = aBlockInfo->mSlotCount ;
    }

    aBlockInfo->mRowSeqOfLastSlot    = aRowSeq;
    aFileHeader->mMaxRowCountInBlock =
        IDL_MAX( aFileHeader->mMaxRowCountInBlock ,
                 (aBlockInfo->mRowSeqOfLastSlot
                  - aBlockInfo->mRowSeqOfFirstSlot) + 1 );
    aFileHeader->mBlockCount ++;

#if defined(DEBUG)
    for( i=0; i< aFileHeader->mBlockHeaderSize; i++)
    {
        // Block내 Header 부분은 여기서 기록하기 때문에 0으로 초기화 되어
        // 있어야 한다.

        if( aBlockInfo->mBlockPtr[i] != 0 )
        {
            if( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                   1, 
                                   IDE_DUMP_DEST_LIMIT, 
                                   (void**)&sTempBuf )
                == IDE_SUCCESS )
            {
                idlOS::snprintf( sTempBuf,
                                 IDE_DUMP_DEST_LIMIT,
                                 "----------- DumpBlock  ----------\n" );
                IDE_TEST( smiDataPort::dumpHeader( 
                        gScpfBlockHeaderDesc,
                        aCommonHeader->mVersion,
                        (void*)aBlockInfo,
                        SMI_DATAPORT_HEADER_FLAG_FULL,
                        sTempBuf,
                        IDE_DUMP_DEST_LIMIT )
                    != IDE_SUCCESS );
                ideLog::log( IDE_SERVER_0, "%s", sTempBuf );

                sTempBuf[0] = '\0';
                if( scpfDumpBlockBody( 
                        aBlockInfo, 
                        0,                             // begin offset
                        aFileHeader->mBlockHeaderSize, // end offset
                        sTempBuf, 
                        IDE_DUMP_DEST_LIMIT )
                    == IDE_SUCCESS )
                {
                    ideLog::log( IDE_SERVER_0, "%s", sTempBuf );
                }

                (void)iduMemMgr::free( sTempBuf );
            }
            IDE_ASSERT(0);
        }
    }
#endif

    // Body에 대한 Checksum 계산
    aBlockInfo->mCheckSum = smuUtility::foldBinary( 
        aBlockInfo->mBlockPtr + aFileHeader->mBlockHeaderSize,
        aFileHeader->mBlockSize - aFileHeader->mBlockHeaderSize );

    // Header를 Block에 쓴다.
    sBlockOffset = 0; // Block 내 Offset
    IDE_TEST( smiDataPort::writeHeader(
            gScpfBlockHeaderDesc,
            aCommonHeader->mVersion,
            aBlockInfo,
            aBlockInfo->mBlockPtr,
            &sBlockOffset,
            aFileHeader->mBlockSize )
        != IDE_SUCCESS );

    IDE_TEST( aFile->write( aStatistics,
                            aFilePosition->mOffset,
                            aBlockInfo->mBlockPtr,
                            aFileHeader->mBlockSize )
              != IDE_SUCCESS);

    aBlockBufferPosition->mID  ++;
    aFilePosition->mOffset += aFileHeader->mBlockSize ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  다음 Block을 준비한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aFileHeader         - [IN] File정보를 담은 Header
 *  aRowSeq             - [IN] 현재 기록중인 Row 번호
 *  aBlockInfo          - [IN] 사용하는 BlockBuffer 
 *  aBlockBufferPosition- [IN/OUT] Block내 첫 Slot 위치로 커서 이동
 *
 **********************************************************************/
static IDE_RC scpfPrepareBlock( idvSQL                  * /*aStatistics*/,
                                scpfHeader              * aFileHeader,
                                SLong                     aRowSeq,
                                scpfBlockInfo           * aBlockInfo,
                                scpfBlockBufferPosition * aBlockBufferPosition )
{
    IDE_DASSERT( aFileHeader              != NULL );
    IDE_DASSERT( aBlockBufferPosition     != NULL );
    IDE_DASSERT( aBlockInfo               != NULL );

    IDE_TEST_RAISE( aBlockInfo->mSlotCount != aBlockBufferPosition->mSlotSeq ,
                    ERR_ABORT_CORRUPTED_BLOCK  );

    // 커서 이동
    aBlockBufferPosition->mOffset   = aFileHeader->mBlockHeaderSize;
    aBlockBufferPosition->mSlotSeq  = 0;

    // 새 Block에 대한 초기화
    aBlockInfo->mRowSeqOfFirstSlot   = aRowSeq;
    aBlockInfo->mRowSeqOfLastSlot    = aRowSeq;
    aBlockInfo->mFirstRowSlotSeq     = 0;
    aBlockInfo->mFirstRowOffset      = SCPF_OFFSET_NULL;
    aBlockInfo->mHasLastChainedSlot  = ID_FALSE;
    aBlockInfo->mSlotCount           = 0;
    aBlockInfo->mCheckSum            = 0;
    aBlockInfo->mBlockID             = aBlockBufferPosition->mID;

    // PBT를 쉽게 하기 위해 초기화 
    idlOS::memset( aBlockInfo->mBlockPtr, 0, aFileHeader->mBlockSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_CORRUPTED_BLOCK,
                aBlockInfo->mBlockID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  Header를 File에 Write한다.(BigEndian으로)
 *
 *  1) Version을 바탕으로 DATA_PORT_HEADER선택
 *  2) Size Estimation
 *  3) Buffer에 기록
 *  4) File에 기록
 *
 *  aStatistics         - [IN] 통계정보
 *  aFile               - [IN] 기록할 파일
 *  aCommonHeader       - [IN] 공통 헤더
 *  aFileHeader         - [IN] 파일 헤더
 *  aBlockBufferPosition- [OUT] 커서를 Header 바로 다음으로 옮김
 *  aFilePosition       - [OUT] 커서를 Header 바로 다음으로 옮김
 *
 **********************************************************************/
static IDE_RC scpfWriteHeader( idvSQL                     * aStatistics,
                               iduFile                    * aFile,
                               smiDataPortHeader          * aCommonHeader,
                               scpfHeader                 * aFileHeader,
                               scpfBlockBufferPosition    * aBlockBufferPosition,
                               scpfFilePosition           * aFilePosition )
{
    UInt                      sVersion;
    UInt                      sColumnCount;
    UInt                      sPartitionCount;
    smiDataPortHeaderDesc   * sCommonHeaderDesc;
    smiDataPortHeaderDesc   * sObjectHeaderDesc;
    smiDataPortHeaderDesc   * sColumnHeaderDesc;
    smiDataPortHeaderDesc   * sTableHeaderDesc;
    smiDataPortHeaderDesc   * sPartitionHeaderDesc;
    void                    * sObjectHeader;
    void                    * sTableHeader;
    void                    * sColumnHeader;
    void                    * sPartitionHeader;
    UInt                      sHeaderSize;
    UChar                   * sBuffer;
    UChar                   * sAlignedBufferPtr;
    UInt                      sBufferSize;
    UInt                      sBufferOffset = 0;
    UInt                      sState  = 0;
    UInt                      i;

    IDE_DASSERT( aFile                 != NULL );
    IDE_DASSERT( aCommonHeader         != NULL );
    IDE_DASSERT( aFileHeader           != NULL );
    IDE_DASSERT( aBlockBufferPosition  != NULL );
    IDE_DASSERT( aFilePosition         != NULL );

    sVersion    = aCommonHeader->mVersion;

    IDE_TEST_RAISE( SMI_DATAPORT_VERSION_LATEST < sVersion,
                    ERR_ABORT_DATA_PORT_VERSION );

    /********************************************************************
     * Select DATA_PORT_HEADER Desc
     *
     * Version을 바탕으로 DATA_PORT_HEADER Desc 선택
     ********************************************************************/
    sCommonHeaderDesc    = gSmiDataPortHeaderDesc;
    sObjectHeaderDesc    = gScpfFileHeaderDesc;
    sTableHeaderDesc     = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getTableHeaderDescFunc();
    sColumnHeaderDesc    = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getColumnHeaderDescFunc();
    sPartitionHeaderDesc = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getPartitionHeaderDescFunc();

    sColumnCount         = aCommonHeader->mColumnCount;
    sPartitionCount      = aCommonHeader->mPartitionCount;

    sObjectHeader        = aFileHeader;
    sColumnHeader        = aCommonHeader->mColumnHeader;
    sTableHeader         = aCommonHeader->mTableHeader;
    sPartitionHeader     = aCommonHeader->mPartitionHeader;

    /*********************************************************************
     * Size Estimation
     *
     * Header는 Version +
     *          CommonHeader +
     *          scpfHeader +
     *          qsfTableInfo +
     *          qsfColumnInfo*ColCount + 
     *          qsfPartitionInfo*PartitionCount +
     * 의 형태로 구성되며, 따라서 Header를 저장할 버퍼의 크기도 이와
     * 같다.
     **********************************************************************/
    sBufferSize = 
        ID_SIZEOF(UInt) +
        smiDataPort::getEncodedHeaderSize( sCommonHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sObjectHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sTableHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sColumnHeaderDesc, sVersion ) * sColumnCount +
        smiDataPort::getEncodedHeaderSize( sPartitionHeaderDesc, sVersion ) * sPartitionCount;

    // DirectIO를 위해 Align을 맞춰줘야 합니다.
    sBufferSize = idlOS::align( sBufferSize, ID_MAX_DIO_PAGE_SIZE );

    aFileHeader->mFileHeaderSize = sBufferSize;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 sBufferSize + ID_MAX_DIO_PAGE_SIZE,
                                 (void**)&(sBuffer) )
              != IDE_SUCCESS );
    sAlignedBufferPtr = (UChar*)idlOS::align( sBuffer, ID_MAX_DIO_PAGE_SIZE );
    sState = 1;



    // Write
    sBufferOffset = 0;

    SCPF_WRITE_UINT( &sVersion, sAlignedBufferPtr + sBufferOffset );
    sBufferOffset += ID_SIZEOF( UInt );


    IDE_TEST( smiDataPort::writeHeader( sCommonHeaderDesc,
                                        sVersion,
                                        aCommonHeader,
                                        sAlignedBufferPtr,
                                        &sBufferOffset,
                                        sBufferSize )
              != IDE_SUCCESS );

    IDE_TEST( smiDataPort::writeHeader( sObjectHeaderDesc,
                                        sVersion,
                                        sObjectHeader,
                                        sAlignedBufferPtr,
                                        &sBufferOffset,
                                        sBufferSize )
              != IDE_SUCCESS );

    IDE_TEST( smiDataPort::writeHeader( sTableHeaderDesc,
                                        sVersion,
                                        sTableHeader,
                                        sAlignedBufferPtr,
                                        &sBufferOffset,
                                        sBufferSize )
              != IDE_SUCCESS );

    sHeaderSize = smiDataPort::getHeaderSize( sColumnHeaderDesc,
                                              sVersion );
    for( i=0; i<sColumnCount; i++ )
    {
        IDE_TEST( smiDataPort::writeHeader( sColumnHeaderDesc,
                                            sVersion,
                                            ((UChar*)sColumnHeader)
                                            + sHeaderSize * i,
                                            sAlignedBufferPtr,
                                            &sBufferOffset,
                                            sBufferSize )
                  != IDE_SUCCESS );
    }

    sHeaderSize = smiDataPort::getHeaderSize( sPartitionHeaderDesc,
                                              sVersion );
    for( i=0; i<sPartitionCount; i++ )
    {
        IDE_TEST( smiDataPort::writeHeader( sPartitionHeaderDesc,
                                            sVersion,
                                            ((UChar*)sPartitionHeader)
                                            + sHeaderSize * i,
                                            sAlignedBufferPtr,
                                            &sBufferOffset,
                                            sBufferSize )
                  != IDE_SUCCESS );
    }

    IDE_TEST( aFile->write( aStatistics,
                            0, // header offset
                            sAlignedBufferPtr,
                            sBufferSize )
              != IDE_SUCCESS);


    sState = 0;
    IDE_TEST( iduMemMgr::free( sBuffer ) != IDE_SUCCESS );

    aBlockBufferPosition->mSeq      = 0;
    aBlockBufferPosition->mID       = 0;
    aBlockBufferPosition->mOffset   = aFileHeader->mBlockHeaderSize;
    aBlockBufferPosition->mSlotSeq  = 0;
    aFilePosition->mOffset          = aFileHeader->mFileHeaderSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_DATA_PORT_VERSION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_VERSION_NOT_SUPPORTED,
                                  sVersion,
                                  SMI_DATAPORT_VERSION_LATEST ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)iduMemMgr::free( sBuffer );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}








/**********************************************************************
 * Internal Functions for Importing
 **********************************************************************/

/***********************************************************************
 *
 * Description :
 *  Object Handle을 초기화한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aCommonHeader       - [IN/OUT] File로부터 읽은 Header 정보
 *  aFirstRowSeq        - [IN]  Import할 첫번째 Row
 *  aLastRowSeq         - [IN]  Import할 마지막 Row
 *  aObjName            - [IN]  Object 이름
 *
 **********************************************************************/
static IDE_RC scpfInit4Import( idvSQL                * aStatistics,
                               scpfHandle           ** aHandle,
                               smiDataPortHeader     * aCommonHeader,
                               SLong                   aFirstRowSeq,
                               SLong                   aLastRowSeq,
                               SChar                 * aObjName,
                               SChar                 * aDirectory )
{
    scpfHandle        * sHandle;
    scpfBlockInfo     * sBlockInfo;
    UInt                sBlockSize;
    UInt                sAllocatedBlockInfoCount;
    UChar             * sAlignedBlockPtr;
    UInt                sState = 0;
    UInt                i;

    IDE_DASSERT( aHandle       != NULL );
    IDE_DASSERT( aCommonHeader != NULL );
    IDE_DASSERT( aObjName      != NULL );

    // -----------------------------------------
    // Handle Allocation
    // -----------------------------------------
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 ID_SIZEOF(scpfHandle),
                                 (void**)&(sHandle) )
              != IDE_SUCCESS );
    *aHandle = sHandle;
    sState = 1;



    // --------------------------------------
    // Initialize basic attribute 
    // ------------------------------------- 
    sHandle->mBlockBufferPosition.mID        = 0;
    sHandle->mBlockBufferPosition.mOffset    = 0;
    sHandle->mBlockBufferPosition.mSlotSeq   = 0;
    sHandle->mFilePosition.mIdx              = 0;
    sHandle->mFilePosition.mOffset           = 0;

    sHandle->mSplit                    = 0;  // Import시 사용 안함
    sHandle->mColumnChainingThreshold  = 0;  // Import시 사용 안함
    sHandle->mRemainLobLength          = 0; 
    sHandle->mRemainLobColumnCount     = 0;

    sHandle->mCommonHandle.mHeader   = aCommonHeader;

    idlOS::strncpy( (SChar*)sHandle->mDirectory, 
                    (SChar*)aDirectory,
                    SM_MAX_FILE_NAME );

    idlOS::strncpy( (SChar*)sHandle->mObjName,
                    (SChar*)aObjName,
                    SM_MAX_FILE_NAME );

    // --------------------------------------
    // Open file and Load header
    // ------------------------------------- 
    // File을 연다.
    IDE_TEST( scpfOpenFile( &(sHandle->mOpenFile),
                            &(sHandle->mFile),
                            sHandle->mObjName, 
                            sHandle->mDirectory, 
                            sHandle->mFilePosition.mIdx,
                            sHandle->mFileName )
              != IDE_SUCCESS);


    // Header를 불러온다
    IDE_TEST( scpfReadHeader( aStatistics, 
                              &(sHandle->mFile),
                              sHandle->mCommonHandle.mHeader,
                              &sHandle->mFileHeader,
                              &sHandle->mBlockBufferPosition,
                              &sHandle->mFilePosition )
              != IDE_SUCCESS);

    // Row범위를 조정함
    if( sHandle->mFileHeader.mLastRowSeqInFile < aLastRowSeq)
    {
        sHandle->mLimitRowSeq = sHandle->mFileHeader.mLastRowSeqInFile;
    }
    else
    {
        sHandle->mLimitRowSeq = aLastRowSeq;
    }

    // FirstRowSeq는 한계보다 크면 안된다.
    if( sHandle->mLimitRowSeq < aFirstRowSeq )
    {
        aFirstRowSeq = sHandle->mLimitRowSeq;
    }

    if( aFirstRowSeq < sHandle->mFileHeader.mFirstRowSeqInFile )
    {
        sHandle->mRowSeq = sHandle->mFileHeader.mFirstRowSeqInFile;
    }
    else
    {
        sHandle->mRowSeq = aFirstRowSeq;
    }

    // -----------------------------------------
    // Memory Allocation
    // -----------------------------------------
    sAllocatedBlockInfoCount = sHandle->mFileHeader.mBlockInfoCount;
    sBlockSize               = sHandle->mFileHeader.mBlockSize;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 sAllocatedBlockInfoCount,
                                 ID_SIZEOF(scpfBlockInfo),
                                 (void**)&(sHandle->mBlockBuffer) )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 sAllocatedBlockInfoCount,
                                 ID_SIZEOF(scpfBlockInfo*),
                                 (void**)&(sHandle->mBlockMap) )
              != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 1,
                                 (ULong)sAllocatedBlockInfoCount * sBlockSize,
                                 (void**)&(sHandle->mChainedSlotBuffer) )
              != IDE_SUCCESS );
    sState = 4;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 ID_SIZEOF( smiRow4DP ),
                                 sHandle->mFileHeader.mMaxRowCountInBlock,
                                 (void**)&(sHandle->mRowList) )
              != IDE_SUCCESS );
    sState = 5;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 ID_SIZEOF( smiValue ),
                                 aCommonHeader->mColumnCount *
                                 sHandle->mFileHeader.mMaxRowCountInBlock,
                                 (void**)&(sHandle->mValueBuffer ) )
              != IDE_SUCCESS );
    sState = 6;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 sAllocatedBlockInfoCount * sBlockSize 
                                    + ID_MAX_DIO_PAGE_SIZE,
                                 (void**)&(sHandle->mAllocatedBlock) )
              != IDE_SUCCESS );
    sState = 7;

    // DirectIO를 위해 Align을 맞춰줘야 합니다.
    sAlignedBlockPtr = (UChar*)idlOS::align( sHandle->mAllocatedBlock,
                                             ID_MAX_DIO_PAGE_SIZE );


    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 ID_SIZEOF( UInt  ),
                                 aCommonHeader->mColumnCount,
                                 (void**)&(sHandle->mColumnMap ) )
              != IDE_SUCCESS );
    sState = 8;

    // Column map
    for( i=0; i<aCommonHeader->mColumnCount; i++)
    {
        sHandle->mColumnMap[i] = 
            gSmiGlobalCallBackList.getColumnMapFromColumnHeaderFunc(
                aCommonHeader->mColumnHeader,
                i );
    }

    // block
    for( i=0; i<sAllocatedBlockInfoCount; i++)
    {
        sBlockInfo = &sHandle->mBlockBuffer[ i ];
        sBlockInfo->mBlockID             = 0;
        sBlockInfo->mRowSeqOfFirstSlot   = 0;
        sBlockInfo->mRowSeqOfLastSlot    = 0;
        sBlockInfo->mFirstRowSlotSeq     = 0;
        sBlockInfo->mFirstRowOffset      = SCPF_OFFSET_NULL;
        sBlockInfo->mHasLastChainedSlot  = ID_FALSE;
        sBlockInfo->mSlotCount           = 0;
        sBlockInfo->mCheckSum            = 0;

        sBlockInfo->mBlockPtr = sAlignedBlockPtr + i * sBlockSize;
        sHandle->mBlockMap[i] = sBlockInfo;
    }

    // ValueList
    for( i=0; i<sHandle->mFileHeader.mMaxRowCountInBlock; i++)
    {
        sHandle->mRowList[ i ].mValueList = 
            &(sHandle->mValueBuffer[ 
              i * aCommonHeader->mColumnCount ]);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 8:
            (void) iduMemMgr::free( sHandle->mColumnMap );
        case 7:
            (void) iduMemMgr::free( sHandle->mAllocatedBlock );
        case 6:
            (void) iduMemMgr::free( sHandle->mValueBuffer );
        case 5:
            (void) iduMemMgr::free( sHandle->mRowList );
        case 4:
            (void) iduMemMgr::free( sHandle->mChainedSlotBuffer );
        case 3:
            (void) iduMemMgr::free( sHandle->mBlockMap );
        case 2:
            (void) iduMemMgr::free( sHandle->mBlockBuffer );
        case 1:
            (void) iduMemMgr::free( sHandle    );
            *aHandle = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  사용한 ObjectHandle을 제거한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *
 **********************************************************************/
static IDE_RC scpfDestroy4Import( idvSQL         * aStatistics,
                                  scpfHandle    ** aHandle )
{
    UInt        sState = 10;

    IDE_DASSERT( aHandle       != NULL );


    sState = 9;
    IDE_TEST( scpfReleaseHeader( aStatistics,
                                 (*aHandle) )
              != IDE_SUCCESS );

    sState = 8;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mColumnMap )
              != IDE_SUCCESS );

    sState = 7;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mAllocatedBlock )
              != IDE_SUCCESS );

    sState = 5;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mValueBuffer )
              != IDE_SUCCESS );

    sState = 4;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mRowList )
              != IDE_SUCCESS );

    sState = 3;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mChainedSlotBuffer ) 
              != IDE_SUCCESS );

    sState = 2;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mBlockMap ) 
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( iduMemMgr::free( (*aHandle)->mBlockBuffer ) 
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( *aHandle ) 
              != IDE_SUCCESS );
    *aHandle = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 9:
            (void) scpfReleaseHeader( aStatistics,
                                      (*aHandle) );
        case 8:
            (void) iduMemMgr::free( (*aHandle)->mColumnMap );
        case 7:
            (void) iduMemMgr::free( (*aHandle)->mAllocatedBlock );
        case 6:
            (void) iduMemMgr::free( (*aHandle)->mValueBuffer );
        case 5:
            (void) iduMemMgr::free( (*aHandle)->mRowList );
        case 4:
            (void) iduMemMgr::free( (*aHandle)->mChainedSlotBuffer );
        case 3:
            (void) iduMemMgr::free( (*aHandle)->mBlockMap );
        case 2:
            (void) iduMemMgr::free( (*aHandle)->mBlockBuffer );
        case 1:
            (void) iduMemMgr::free( *aHandle );
            *aHandle = NULL;
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  해당 File을 연다.
 *
 *  aOpenFile           - [IN]  File이 Open되어 있는가
 *  aFile               - [IN]  Open할 File 핸들
 *  aObjName            - [IN]  File이름을 결정하기 위한 Obj이름
 *  aIdx                - [IN]  File의 번호
 *  aFileName           - [OUT] 결정된 파일 이름
 *
 **********************************************************************/
static IDE_RC scpfOpenFile( idBool         * aOpenFile,
                            iduFile        * aFile,
                            SChar          * aObjName, 
                            SChar          * aDirectory,
                            UInt             aIdx,
                            SChar          * aFileName )
{
    UInt         sFileState = 0;

    IDE_DASSERT( aFile        != NULL );
    IDE_DASSERT( aObjName     != NULL );
    IDE_DASSERT( aFileName    != NULL );

    // --------------------------------------
    // Create file 
    // ------------------------------------- 
    IDE_TEST( scpfFindFile( aObjName, 
                            aDirectory,
                            aIdx,
                            aFileName )
              != IDE_SUCCESS );

    IDE_TEST( aFile->initialize( IDU_MEM_SM_SCP,
                                 1, /* Max Open FD Count */
                                 IDU_FIO_STAT_OFF,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);
    sFileState = 1;

    IDE_TEST( aFile->setFileName( aFileName )
              != IDE_SUCCESS ) ;

    IDE_TEST( aFile->open()
              != IDE_SUCCESS );
    sFileState = 2;
    *aOpenFile = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sFileState )
    {
        case 2:
            (void) aFile->close() ;
            *aOpenFile = ID_FALSE;
        case 1:
            (void) aFile->destroy();
            break;
        default:
            break;
    }


    return IDE_FAILURE;

}
/***********************************************************************
 *
 * Description :
 *  ObjectName에 맞는 File을 찾는다 
 *  <Directory><DirectorySperator><FileName>_<Idx>.<EXT>  
 *
 *  aObjName            - [IN]  Object 이름
 *  aDirectory          - [IN]  파일이 있는 directory
 *  aFileIdx            - [IN]  File 번호
 *  aFileName           - [OUT] 생성된 파일 이름
 *
 **********************************************************************/

static IDE_RC scpfFindFile( SChar            * aObjName,
                            SChar            * aDirectory,
                            UInt               aFileIdx,
                            SChar            * aFileName )
{
    UInt    sLen;

    IDE_DASSERT( aObjName   != NULL );
    IDE_DASSERT( aDirectory != NULL );
    IDE_DASSERT( aFileName  != NULL );


    // Directory에 데이터가 있는데, /로 끝나지 않을 경우 붙여줌
    sLen = idlOS::strnlen( aDirectory, SM_MAX_FILE_NAME );
    if( 0 < sLen )
    {
        if( aDirectory[ sLen - 1 ] != IDL_FILE_SEPARATOR )
        {
            idlVA::appendFormat( aDirectory,
                                 SM_MAX_FILE_NAME,
                                 "%c",
                                 IDL_FILE_SEPARATOR );
        }
    }

    // 1. Original
    idlOS::snprintf( aFileName, 
                     SM_MAX_FILE_NAME,
                     "%s%s",
                     aDirectory, 
                     aObjName );
    IDE_TEST_CONT( idf::access(aFileName, F_OK) == 0, DONE );


    // 2. Extension
    // 확장자를 붙임
    idlOS::snprintf( aFileName, 
                     SM_MAX_FILE_NAME,
                     "%s%s%s",
                     aDirectory, 
                     aObjName, 
                     SCPF_DPFILE_EXT);
    IDE_TEST_CONT( idf::access(aFileName, F_OK) == 0, DONE );


    // 3. FileIdx
    // FileIndex와 확장자를 붙임
    idlOS::snprintf( aFileName, 
                     SM_MAX_FILE_NAME,
                     "%s%s%c%"ID_UINT32_FMT"%s",
                     aDirectory, 
                     aObjName, 
                     SCPF_DPFILE_SEPARATOR,
                     aFileIdx,
                     SCPF_DPFILE_EXT);
    IDE_TEST_CONT( idf::access(aFileName, F_OK) == 0, DONE );

    // 탐색 실패
    IDE_RAISE( ERR_ABORT_CANT_OPEN_FILE );

    IDE_EXCEPTION_CONT( DONE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CANT_OPEN_FILE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CANT_OPEN_FILE,
                                  aFileName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description :
 *  파일을 열어 Header를 읽고 이를 바탕으로 Handle을 초기화 한다.
 *
 * 1) Header를 읽는다.
 * 2) Header의 Version을 바탕으로 DATA_PORT_HEADERDesc을 선택한다.
 * 3) Size Estimation
 * 4) 선택한 DATA_PORT_HEADER를 바탕으로 Header의 세부 데이터를 읽음
 * 5) Size를 바탕으로 Validation
 *
 *  aStatistics         - [IN] 통계정보
 *  aFile               - [IN] 기록할 파일
 *  aCommonHeader       - [IN] 공통 헤더
 *  aFileHeader         - [IN] 파일 헤더
 *  aBlockBufferPosition- [OUT] 커서를 Header 바로 다음으로 옮김
 *  aFilePosition       - [OUT] 커서를 Header 바로 다음으로 옮김
 *
 **********************************************************************/

static IDE_RC scpfReadHeader( idvSQL                    * aStatistics,
                              iduFile                   * aFile,
                              smiDataPortHeader         * aCommonHeader,
                              scpfHeader                * aFileHeader,
                              scpfBlockBufferPosition   * aBlockBufferPosition,
                              scpfFilePosition          * aFilePosition )
{
    UInt                      sVersionBuffer;
    UInt                      sVersion;
    UInt                      sColumnCount;
    UInt                      sPartitionCount;
    smiDataPortHeaderDesc   * sCommonHeaderDesc;
    smiDataPortHeaderDesc   * sObjectHeaderDesc;
    smiDataPortHeaderDesc   * sTableHeaderDesc;
    smiDataPortHeaderDesc   * sColumnHeaderDesc;
    smiDataPortHeaderDesc   * sPartitionHeaderDesc;
    void                    * sObjectHeader;
    void                    * sTableHeader;
    void                    * sColumnHeader;
    void                    * sPartitionHeader; 
    UInt                      sTotalHeaderSize;
    UInt                      sHeaderSize;
    UChar                   * sBuffer;
    UInt                      sBufferSize;
    UInt                      sBufferOffset = 0;
    UInt                      sBufferState  = 0;
    UInt                      sOffset       = 0;
    UInt                      sState        = 0;
    UInt                      i;

    IDE_DASSERT( aStatistics   != NULL );
    IDE_DASSERT( aFile         != NULL );
    IDE_DASSERT( aCommonHeader != NULL );
    IDE_DASSERT( aFileHeader   != NULL );

    /********************************************************************
     * Read Version
     * Version을 읽어야 공통헤더의 형태를 알 수 있다.
     ********************************************************************/

    // Version 또한 Endian 상관없이 읽어야 하기 때문에, 버퍼에 저장 후
    // 다시 읽는다.
    IDE_TEST( aFile->read( aStatistics,
                           0, // header offset
                           &sVersionBuffer,
                           ID_SIZEOF(UInt) )
              != IDE_SUCCESS);
    sOffset = ID_SIZEOF(UInt);

    SCPF_READ_UINT( &sVersionBuffer,
                    &sVersion );

    IDE_TEST_RAISE( SMI_DATAPORT_VERSION_LATEST < sVersion,
                    ERR_ABORT_DATA_PORT_VERSION );
    aCommonHeader->mVersion = sVersion;

    /********************************************************************
     * Read CommonHeader
     ********************************************************************/

    sCommonHeaderDesc = gSmiDataPortHeaderDesc;
    sBufferSize = smiDataPort::getEncodedHeaderSize( sCommonHeaderDesc,
                                                     sVersion );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 sBufferSize,
                                 (void**)&(sBuffer) )
              != IDE_SUCCESS );
    sBufferState = 1;

    IDE_TEST( aFile->read( aStatistics,
                           sOffset,
                           sBuffer,
                           sBufferSize )
              != IDE_SUCCESS);
    sOffset += sBufferSize;

    sBufferOffset = 0;
    IDE_TEST( smiDataPort::readHeader( sCommonHeaderDesc,
                                       sVersion,
                                       sBuffer,
                                       &sBufferOffset,
                                       sBufferSize,
                                       aCommonHeader )
              != IDE_SUCCESS);

    sBufferState = 0;
    IDE_TEST( iduMemMgr::free( sBuffer ) != IDE_SUCCESS );

    /********************************************************************
     * Select DATA_PORT_HEADER Desc
     *
     * Version을 바탕으로 DATA_PORT_HEADER Desc 선택
     ********************************************************************/
    sObjectHeaderDesc     = gScpfFileHeaderDesc;
    sTableHeaderDesc      = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getTableHeaderDescFunc();
    sColumnHeaderDesc     = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getColumnHeaderDescFunc();
    sPartitionHeaderDesc  = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getPartitionHeaderDescFunc();

    sColumnCount    = aCommonHeader->mColumnCount;
    sPartitionCount = aCommonHeader->mPartitionCount;

    /********************************************************************
     * Size Estimation
     *
     * Header는 Version +
     *          commonHeader + 
     *          scpfHeader +
     *          qsfTableInfo +
     *          qsfColumnInfo*ColCount + 
     *          qsfPartitionInfo*PartitionCount +
     * 의 형태로 구성된다. 이중 위의 둘은 이미 읽었으므로 제외한다.
     ********************************************************************/
    sBufferSize = 
        smiDataPort::getEncodedHeaderSize( sObjectHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sTableHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sColumnHeaderDesc, sVersion ) 
            * sColumnCount +
        smiDataPort::getEncodedHeaderSize( sPartitionHeaderDesc, sVersion ) 
            * sPartitionCount;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 sBufferSize,
                                 (void**)&(sBuffer) )
              != IDE_SUCCESS );
    sBufferState = 1;

    // Size를 바탕으로 Structure를 저장할 버퍼 할당

    // ObjectHeader(fileHeader)는 이미 선언되어 있기에 그것을 이용한다.
    sObjectHeader                = aFileHeader;
    aCommonHeader->mObjectHeader = aFileHeader;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 1,
                                 smiDataPort::getHeaderSize( 
                                     sTableHeaderDesc,
                                     sVersion ),
                                 (void**)&(aCommonHeader->mTableHeader ) )
              != IDE_SUCCESS );
    sTableHeader =  aCommonHeader->mTableHeader;
    sState = 1;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 sColumnCount,
                                 smiDataPort::getHeaderSize( 
                                     sColumnHeaderDesc,
                                     sVersion ),
                                 (void**)&(aCommonHeader->mColumnHeader ) )
              != IDE_SUCCESS );
    sColumnHeader = aCommonHeader->mColumnHeader;
    sState = 2;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 
                                 sPartitionCount,
                                 smiDataPort::getHeaderSize( 
                                     sPartitionHeaderDesc,
                                     sVersion ),
                                 (void**)&(aCommonHeader->mPartitionHeader ) )
              != IDE_SUCCESS );
    sPartitionHeader = aCommonHeader->mPartitionHeader;
    sState = 3;

    /********************************************************************
     * Load File & Table & Column & Partition Headers
     ********************************************************************/

    // Read
    sBufferOffset = 0;
    IDE_TEST( aFile->read( aStatistics,
                           sOffset,
                           sBuffer,
                           sBufferSize )
              != IDE_SUCCESS);
    sOffset += sBufferSize;

    IDE_TEST( smiDataPort::readHeader( sObjectHeaderDesc,
                                       sVersion,
                                       sBuffer,
                                       &sBufferOffset,
                                       sBufferSize,
                                       sObjectHeader )
              != IDE_SUCCESS );

    IDE_TEST( smiDataPort::readHeader( sTableHeaderDesc,
                                       sVersion,
                                       sBuffer,
                                       &sBufferOffset,
                                       sBufferSize,
                                       sTableHeader )
              != IDE_SUCCESS );

    sHeaderSize = smiDataPort::getHeaderSize( sColumnHeaderDesc,
                                              sVersion );
    for( i=0; i<sColumnCount; i++ )
    {
        IDE_TEST( smiDataPort::readHeader( sColumnHeaderDesc,
                                           sVersion,
                                           sBuffer,
                                           &sBufferOffset,
                                           sBufferSize,
                                           ((UChar*)sColumnHeader) 
                                           + sHeaderSize * i )
                  != IDE_SUCCESS );
    }

    sHeaderSize = smiDataPort::getHeaderSize( sPartitionHeaderDesc,
                                              sVersion );
    for( i=0; i<sPartitionCount; i++ )
    {
        IDE_TEST( smiDataPort::readHeader( sPartitionHeaderDesc,
                                           sVersion,
                                           sBuffer,
                                           &sBufferOffset,
                                           sBufferSize,
                                           ((UChar*)sPartitionHeader)
                                           + sHeaderSize * i )
                  != IDE_SUCCESS );
    }

    /********************************************************************
     * Size validation
     *******************************************************************/

    if( aFileHeader->mBlockHeaderSize 
        != smiDataPort::getEncodedHeaderSize( 
            gScpfBlockHeaderDesc,
            sVersion ) )
    {
        ideLog::log( IDE_SERVER_0,
                     "INTERNAL_ERROR[%s:%u]\n"
                     "BlockHeaderSize  :%u\n"
                     "EstimatedSize    :%u\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__,
                     aFileHeader->mBlockHeaderSize,
                     smiDataPort::getEncodedHeaderSize( 
                         gScpfBlockHeaderDesc,
                         sVersion ) );

        IDE_RAISE( ERR_ABORT_DATA_PORT_INTERNAL_ERROR );
    }

    // 저장된 Header크기와 실제 Header 크기가 동일해야 한다.
    sTotalHeaderSize = 
        ID_SIZEOF(UInt) +
        smiDataPort::getEncodedHeaderSize( sCommonHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sObjectHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sTableHeaderDesc, sVersion ) +
        smiDataPort::getEncodedHeaderSize( sColumnHeaderDesc, sVersion ) 
            * sColumnCount +
        smiDataPort::getEncodedHeaderSize( sPartitionHeaderDesc, sVersion ) 
            * sPartitionCount;

    // DirectIO를 위해 Align을 맞춰줘야 합니다.
    sTotalHeaderSize = 
        idlOS::align( sTotalHeaderSize, ID_MAX_DIO_PAGE_SIZE );

    if( aFileHeader->mFileHeaderSize != sTotalHeaderSize )
    {
        ideLog::log( IDE_SERVER_0,
                     "INTERNAL_ERROR[%s:%u]\n"
                     "FileHeaderSize  :%u\n"
                     "EstimatedSize   :%u\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__,
                     aFileHeader->mBlockHeaderSize,
                     sTotalHeaderSize );

        IDE_RAISE( ERR_ABORT_DATA_PORT_INTERNAL_ERROR );
    }

    sBufferState = 0;
    IDE_TEST( iduMemMgr::free( sBuffer ) != IDE_SUCCESS );

    aBlockBufferPosition->mSeq      = 0;
    aBlockBufferPosition->mID       = 0;
    aBlockBufferPosition->mOffset   = aFileHeader->mBlockHeaderSize;
    aBlockBufferPosition->mSlotSeq  = 0;
    aFilePosition->mOffset          = aFileHeader->mFileHeaderSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_DATA_PORT_VERSION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_VERSION_NOT_SUPPORTED,
                                  sVersion,
                                  SMI_DATAPORT_VERSION_LATEST ) );
    }
    IDE_EXCEPTION( ERR_ABORT_DATA_PORT_INTERNAL_ERROR );
    {
        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_DATA_PORT_INTERNAL_ERROR ) );
    }   
    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 3:
        (void)iduMemMgr::free( sPartitionHeader );
        break;
    case 2:
        (void)iduMemMgr::free( sColumnHeader );
        break;
    case 1:
        (void)iduMemMgr::free( sTableHeader );
        break;
    default:
        break;
    }

    switch( sBufferState )
    {
        case 1:
            (void)iduMemMgr::free( sBuffer );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

}

// Header를 날린다.
static IDE_RC scpfReleaseHeader( idvSQL         * /*aStatistics*/,
                                 scpfHandle     * aHandle )
{
    UInt                sState = 3;
    smiDataPortHeader  *sCommonHeader;

    sCommonHeader = aHandle->mCommonHandle.mHeader;

    sState = 2;
    IDE_TEST( iduMemMgr::free( sCommonHeader->mPartitionHeader ) 
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( iduMemMgr::free( sCommonHeader->mColumnHeader ) 
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sCommonHeader->mTableHeader ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 3:
        (void)iduMemMgr::free( sCommonHeader->mPartitionHeader );
        break;
    case 2:
        (void)iduMemMgr::free( sCommonHeader->mColumnHeader );
        break;
    case 1:
        (void)iduMemMgr::free( sCommonHeader->mTableHeader );
        break;
    default:
        break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  Block을 File로 부터 읽는다.
 *  순수하게 Block 자체와 BlockHeader Parsing만 수행한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aFile               - [IN] FileHandle
 *  aCommonHeader       - [IN] 공통헤더
 *  aFileHeaderSize     - [IN] File의 Header 크기
 *  aBlockHeaderSize    - [IN] Block의 Header 크기
 *  aBlockSize          - [IN] Block의 크기
 *  aBlockID            - [IN] 읽을 Block ID
 *  aFileOffset         - [OUT] 읽은 Block의 File 위치
 *  aBlockInfo          - [OUT] 읽은 Block
 *
 **********************************************************************/
static IDE_RC scpfReadBlock( idvSQL             * aStatistics,
                             iduFile            * aFile,
                             smiDataPortHeader  * aCommonHeader,
                             UInt                 aFileHeaderSize,
                             UInt                 aBlockHeaderSize,
                             UInt                 aBlockSize,
                             UInt                 aBlockID,
                             ULong              * aFileOffset,
                             scpfBlockInfo      * aBlockInfo )
{
    UInt            sBufferOffset;
    UInt            sCheckSum;

    IDE_DASSERT( aFile       != NULL );
    IDE_DASSERT( aFileOffset != NULL );
    IDE_DASSERT( aBlockInfo  != NULL );

    /*BUG-32115  [sm-util] The Import operation in DataPort 
     * calculates file offset uses UINT type. */
    *aFileOffset  =  aFileHeaderSize +  ((ULong)aBlockSize) * aBlockID;

    IDE_TEST( aFile->read( aStatistics,
                           *aFileOffset,
                           aBlockInfo->mBlockPtr,
                           aBlockSize )
              != IDE_SUCCESS);

    /******************************************************************
     * Read Block Header
     *****************************************************************/
    sBufferOffset = 0;
    IDE_TEST( smiDataPort::readHeader( 
            gScpfBlockHeaderDesc,
            aCommonHeader->mVersion,
            aBlockInfo->mBlockPtr,
            &sBufferOffset,
            aBlockSize,
            aBlockInfo )
        != IDE_SUCCESS );

    // 요구한 Block을 읽었어야함
    IDE_TEST_RAISE( aBlockInfo->mBlockID != aBlockID,
                    ERR_ABORT_CORRUPTED_BLOCK );

    // Body에 대한 Checksum 확인 (Checksum은 BlockHeader 이후)
    sCheckSum = smuUtility::foldBinary( 
        aBlockInfo->mBlockPtr + aBlockHeaderSize,
        aBlockSize- aBlockHeaderSize );
    IDE_TEST_RAISE( aBlockInfo->mCheckSum != sCheckSum, 
                    ERR_ABORT_CORRUPTED_BLOCK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CORRUPTED_BLOCK, 
                                  aBlockID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  BinarySearch로 FirstRow를 가진 Block을 찾아서 Handle에 올린다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aFile               - [IN] 읽을 File
 *  aCommonHeader       - [IN] 공통헤더
 *  aFirstRowSeqInFile  - [IN] 파일 내 첫번째 row의 번호
 *  aTargetRow          - [IN] 찾으려 하는 FirstRow
 *  aBlockCount         - [IN] Block의 개수
 *  aFileHeaderSize     - [IN] FileHeader의 크기
 *  aBlockHeaderSize    - [IN] BlockHeader의 크기
 *  aBlockSize          - [IN] Block의 크기
 *  aBlockMap           - [OUT] 읽은 Block을 저장할 BlockInfo
 *  aBlockCorsur        - [IN/OUT] BlockBufferPosition
 *
 **********************************************************************/
static IDE_RC scpfFindFirstBlock( idvSQL                  * aStatistics,
                                  iduFile                 * aFile,
                                  smiDataPortHeader       * aCommonHeader,
                                  SLong                     aFirstRowSeqInFile,
                                  SLong                     aTargetRow,
                                  UInt                      aBlockCount,
                                  UInt                      aFileHeaderSize,
                                  UInt                      aBlockHeaderSize,
                                  UInt                      aBlockSize,
                                  scpfBlockInfo          ** aBlockMap,
                                  scpfBlockBufferPosition * aBlockBufferPosition )

{
    scpfBlockInfo *sBlockInfo;
    SLong          sFirstRowSeq;
    UInt           sMin;
    UInt           sMed;
    UInt           sMax;
    ULong          sFileOffset;
    UInt           sBlockID;

    IDE_DASSERT( aBlockMap     != NULL );

    sBlockID = 0;
    // 파일 내 첫번째 Row를 가져오라는 의미였으면, 
    // Binary search를 Skip하고 첫번째 Block부터 시작
    IDE_TEST_CONT( (aTargetRow == aFirstRowSeqInFile), SKIP_FIND_ROW );

    sMin       = 0;
    sMax       = aBlockCount - 1;
    sBlockInfo = aBlockMap[ 0 ]; // 0번 Info를 Buffer로 사용한다.

    while( sMin <= sMax )
    {
        sMed = (sMin + sMax) >> 1;

        IDE_TEST( scpfReadBlock( aStatistics,
                                 aFile,
                                 aCommonHeader,
                                 aFileHeaderSize,
                                 aBlockHeaderSize,
                                 aBlockSize,
                                 sMed,
                                 &sFileOffset,
                                 sBlockInfo )
                  != IDE_SUCCESS );

        if( 0 < sBlockInfo->mFirstRowSlotSeq )
        {
            // 이전 Block에서 Overflow된 Slot 가 있으면, 
            // FirstRow는 그 다음 Row이다.
            sFirstRowSeq = sBlockInfo->mRowSeqOfFirstSlot + 1;
        }
        else
        {
            sFirstRowSeq = sBlockInfo->mRowSeqOfFirstSlot ;
        }

        if( aTargetRow < sFirstRowSeq )
        {
            sMax = sMed - 1;
        }
        else
        {
            sMin = sMed + 1;
        }
    }
    sBlockID = (sMin + sMax) >> 1;

    IDE_EXCEPTION_CONT( SKIP_FIND_ROW );

    aBlockBufferPosition->mID = sBlockID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description :
 *  BlockMap을 Shift시켜서 아직 읽지 않은 Block을 BlockMap의
 *  앞쪽에 오도록 이동시킨다
 * 
 *  예) BlockMap 0,1,2,3
 *     => 0,1번 Block의 Row는 모두 처리하였음
 *        (BlockBufferPosition.mSeq가 2를 가리킴)
 *     => 2개 만큼 Shift시킴
 *        BlockMap  2,3,0,1
 *     => 이후 0,1번에 4,5번 Block을 올림
 *        BlockMap  2,3,4,5
 *     
 *
 *  aShiftBlockCount    - [IN] Shift할 Block 개수
 *  aBlockInfoCount     - [IN] 한번에 읽을 수 있는 BlockInfo 개수
 *  aBlockMap           - [IN/OUT] 읽은 Block을 저장할 BlockInfo 
 *
 **********************************************************************/
static void scpfShiftBlock( UInt                aShiftBlockCount,
                            UInt                aBlockInfoCount,
                            scpfBlockInfo    ** aBlockMap )
{
    scpfBlockInfo    * sBlockInfoPtr;
    UInt               i;
    UInt               j;

    IDE_DASSERT( aBlockMap    != NULL );

    // 읽은 Block만큼 Shift
    for( j=0; j<aShiftBlockCount; j++ )
    {
        sBlockInfoPtr = aBlockMap[ 0 ];
        for( i=0; i<aBlockInfoCount-1; i++ )
        {
            aBlockMap[ i ] = aBlockMap[ i+1 ];
        }
        aBlockMap[ aBlockInfoCount-1 ] = sBlockInfoPtr;
    }
}

/***********************************************************************
 *
 * Description :
 *  다음번 Block을 읽어서 Handle에 올린다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aFile               - [IN] 읽을 File
 *  aCommonHeader       - [IN]  공통헤더
 *  aFileName           - [IN] 읽을 File이름
 *  aTargetRow          - [IN] Block을 읽으면, 이 Row가 있어야 한다.
 *  aBlockCount         - [IN] 총 Block의 개수
 *  aFileHeaderSize     - [IN] FileHeader의 크기
 *  aBlockHeaderSize    - [IN] BlockHeader의 크기
 *  aBlockSize          - [IN] Block의 크기
 *  aNextBlockID        - [IN] 이번에 준비되는 Block의 첫번째 Block
 *  aFilePosition       - [OUT] 읽은만큼 FilePosition이동
 *  aBlockInfoCount     - [IN] 메모리상의 BlockInfo의 개수
 *  aReadBlockCount     - [IN] 이번에 읽을 Block 개수
 *  aBlockMap           - [OUT] 읽은 Block을 저장할 BlockInfo 
 *
 **********************************************************************/
static IDE_RC scpfPreFetchBlock( idvSQL             * aStatistics,
                                 iduFile            * aFile,
                                 smiDataPortHeader  * aCommonHeader,
                                 SChar              * aFileName,
                                 SLong                aTargetRow,
                                 UInt                 aBlockCount,
                                 UInt                 aFileHeaderSize,
                                 UInt                 aBlockHeaderSize,
                                 UInt                 aBlockSize,
                                 UInt                 aNextBlockID,
                                 scpfFilePosition   * aFilePosition,
                                 UInt                 aBlockInfoCount,
                                 UInt                 aReadBlockCount,
                                 scpfBlockInfo     ** aBlockMap )
{
    ULong              sFileOffset;
    scpfBlockInfo    * sFirstBlock;
    UInt               sCurBlockID;
    UInt               sRemainBlockCount;
    UInt               i;

    IDE_DASSERT( aFilePosition!= NULL );
    IDE_DASSERT( aBlockMap    != NULL );

    // Block을 전부 읽었거나, 읽을 Block이 없는가
    IDE_TEST_CONT( ( aNextBlockID >= aBlockCount ) ||
                    ( aReadBlockCount == 0) ,
                    SKIP_READBLOCK );

    sFileOffset       = 0;
    sRemainBlockCount = aBlockInfoCount - aReadBlockCount;

    for( i=sRemainBlockCount; i<sRemainBlockCount + aReadBlockCount; i++ )
    {
        sCurBlockID = aNextBlockID + i;
        if( aBlockCount <= sCurBlockID )
        {
            break;
        }

        IDE_TEST( scpfReadBlock( aStatistics,
                                 aFile,
                                 aCommonHeader,
                                 aFileHeaderSize,
                                 aBlockHeaderSize,
                                 aBlockSize,
                                 sCurBlockID,
                                 &sFileOffset,
                                 aBlockMap[ i ] )
                  != IDE_SUCCESS );
    }


    // 바르게 Shift돼었으면, BlockMap의 첫번째 Block은
    // NextBlockID번째 Block이며, TargetRow를 가진다.
    sFirstBlock   = aBlockMap[ 0 ];
    if( ( aNextBlockID !=  sFirstBlock->mBlockID ) ||
        ( aTargetRow < sFirstBlock->mRowSeqOfFirstSlot ) ||
        ( sFirstBlock->mRowSeqOfLastSlot < aTargetRow ) )
    {
        ideLog::log( IDE_SERVER_0,
                     "INTERNAL_ERROR[%s:%u]\n"
                     "FileName          :%s\n"
                     "NextBlockID       :%u\n"
                     "ReadBlockID       :%u\n"
                     "TargetRowID       :%llu\n"
                     "FirstRowSeqInBlock:%llu\n"
                     "LastRowSeqInBlock :%llu\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__,
                     aFileName,
                     aNextBlockID,
                     sFirstBlock->mBlockID,
                     aTargetRow,
                     sFirstBlock->mRowSeqOfFirstSlot,
                     sFirstBlock->mRowSeqOfLastSlot );

        IDE_RAISE( ERR_ABORT_DATA_PORT_INTERNAL_ERROR );
    }

    aFilePosition->mOffset    = sFileOffset;

    IDE_EXCEPTION_CONT( SKIP_READBLOCK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_DATA_PORT_INTERNAL_ERROR );
    {
        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_DATA_PORT_INTERNAL_ERROR ) );
    }   
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description :
 *  Block을 Parsing하여 Row별로 Build한다.
 *  Block은 복수개의 Slot 의 집합이다. 각각의 Slot 는 1개 또는 N개가 모여
 * 하나의 Column, 각 Column이 모여 하나의 Row가 된다.
 *  따라서 본 함수에서는, Block을 arsing하여 Handle의 smiRow4DP에
 * 각 Slot 들을 Row로 만든다.
 *
 *  aStatistics            - [IN]  통계정보
 *  aCommonHeader          - [IN]  공통헤더
 *  aFileHeader            - [IN]  파일헤더
 *  aBlockBufferPosition   - [IN]  BlockBufferPosition
 *  aBlockMap              - [IN]  Row를 읽을 BlockInfo의 map
 *  aChainedSlotBuffer     - [OUT] ChainedRow를 저장하는 임시 버퍼
 *  aUsedChainedSlotBufSize- [OUT] 임시 버퍼의 사용량
 *  aBuiltRowCount         - [OUT] Build된 Row의 개수
 *  aHasSupplementLob      - [OUT] 추가로 해야할 Lob처리가 있는가?
 *  aRowList               - [OUT] Row목록
 *
 **********************************************************************/

static IDE_RC scpfBuildRow( idvSQL                    * /*aStatistics*/,
                            scpfHandle                * aHandle,
                            smiDataPortHeader         * aCommonHeader,
                            scpfHeader                * aFileHeader,
                            scpfBlockBufferPosition   * aBlockBufferPosition,
                            scpfBlockInfo            ** aBlockMap,
                            UChar                     * aChainedSlotBuffer,
                            UInt                      * aUsedChainedSlotBufSize,
                            UInt                      * aBuiltRowCount,
                            idBool                    * aHasSupplementLob,
                            smiRow4DP                 * aRowList )
{
    smiRow4DP     * sRow;
    scpfBlockInfo * sBlockInfo;
    UInt            sBeginBlockID;
    UInt            sUnchainedSlotCount;
    UInt            sBasicColumnCount;
    UInt            sLobColumnCount;
    UInt          * sColumnMap;
    UInt            sMaxRowCountInBlock;
    UInt            sBlockHeaderSize;
    idBool          sDone;
    UInt            i;

    IDE_DASSERT( aCommonHeader          != NULL );
    IDE_DASSERT( aFileHeader            != NULL );
    IDE_DASSERT( aBlockBufferPosition   != NULL );
    IDE_DASSERT( aBlockMap              != NULL );
    IDE_DASSERT( aChainedSlotBuffer     != NULL );
    IDE_DASSERT( aUsedChainedSlotBufSize!= NULL );
    IDE_DASSERT( aBuiltRowCount         != NULL );
    IDE_DASSERT( aHasSupplementLob      != NULL );
    IDE_DASSERT( aRowList               != NULL );

    sBeginBlockID           = aBlockBufferPosition->mID;
    sRow                    = aRowList;
    sBasicColumnCount       = aCommonHeader->mBasicColumnCount;
    sLobColumnCount         = aCommonHeader->mLobColumnCount;
    sColumnMap              = aHandle->mColumnMap;
    sMaxRowCountInBlock     = aFileHeader->mMaxRowCountInBlock;
    sBlockHeaderSize        = aFileHeader->mBlockHeaderSize;

    *aUsedChainedSlotBufSize   = 0;
    IDE_DASSERT( aBlockBufferPosition->mSeq == 0);
    sDone                      = ID_FALSE;

    *aHasSupplementLob         = ID_FALSE;
    *aBuiltRowCount            = 0;

    while( sDone == ID_FALSE )
    {
        // BuildRowCount는 이미 구축된 Row의 개수이다.
        // MaxRowCountInBlock은 Block내에 최대 Row개수이며, 
        // 이를 기준으로 row4DP를 할당한다.
        // 따라서 이 둘이 같다는 것은, 이미 준비한 row4DP를 전부
        // 사용했다는 뜻이다.
        IDE_TEST_RAISE( (*aBuiltRowCount) >= sMaxRowCountInBlock,
                        ERR_ABORT_CORRUPTED_BLOCK );

        for( i=0; i<sBasicColumnCount; i++)
        {
            IDE_TEST( scpfReadBasicColumn(
                        aBlockBufferPosition,
                        aBlockMap,
                        sBlockHeaderSize,
                        aChainedSlotBuffer,
                        aUsedChainedSlotBufSize,
                        &(sRow->mValueList[ sColumnMap[ i ] ]) )
                != IDE_SUCCESS );
        }

        // Lob이 있을 경우
        if( sLobColumnCount > 0)
        {
            // 현재 Row의 Lob이 Block을 넘어가는지 확인한다.
            sBlockInfo = aBlockMap[ aBlockBufferPosition->mSeq ];

            // Chained되지 않은 Slot 의 개수를 구한다
            sUnchainedSlotCount = sBlockInfo->mSlotCount ;
            if( sBlockInfo->mHasLastChainedSlot == ID_TRUE )
            {
                sUnchainedSlotCount = sBlockInfo->mSlotCount - 1;
            }
    
            if( sUnchainedSlotCount
                < ( aBlockBufferPosition->mSlotSeq  + sLobColumnCount ) )
            {
                // 현재 Row의 Lob이 Block을 넘어가는 경우
                sRow->mHasSupplementLob = ID_TRUE;
                *aHasSupplementLob      = ID_TRUE;

                for( i = sBasicColumnCount; 
                     i < (sLobColumnCount + sBasicColumnCount);
                     i ++)
                {
                    // 일단 Null로 삽입된 후, 나중에 Append된다.
                    sRow->mValueList[ sColumnMap[ i ] ].length = 0;
                    sRow->mValueList[ sColumnMap[ i ] ].value  = 0;
                }

                sDone = ID_TRUE;
            }
            else
            {
                // 일반 Column과 동일한 방식으로 처리한다.
                sRow->mHasSupplementLob = ID_FALSE;

                for( i = sBasicColumnCount; 
                     i < (sLobColumnCount + sBasicColumnCount);
                     i ++)
                {
                    IDE_TEST( scpfReadLobColumn( 
                                aBlockBufferPosition,
                                aBlockMap,
                                sBlockHeaderSize,
                                &(sRow->mValueList[ sColumnMap[ i ] ]) )
                        != IDE_SUCCESS );
                }
            }
        }

        // BeginBlockID는 BuildRow 시작 지점에 따놓은 ID다.
        // 따라서 이것이 다르다는 의니느 ReadColumn한수들에 의해
        // 다음 Block으로 Position이 넘어갔다는 뜻이다.
        // 즉, 한 Block을 처리하였기 때문에 Build작업은 완료된다.
        if( sBeginBlockID  != aBlockBufferPosition->mID )
        {
            sDone = ID_TRUE;
        }

        sRow ++;
        (*aBuiltRowCount) ++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_CORRUPTED_BLOCK,
                aBlockBufferPosition->mID ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  Block으로부터 BasicColumn을 읽는다.
 *  만약 Column이 Chaine되면, ChainedSlotBuffer를 이용한다.
 *
 *  aStatistics                - [IN]  통계정보
 *  aBlockBufferPosition       - [IN]  BlockBufferPosition
 *  aBlockInfoCount            - [IN]  Row를 읽어드릴 Block의 총 개수
 *  aBlockInfo                 - [IN]  Row를 읽어드릴 Block
 *  aBlockHeaderSize           - [IN]  BlockHeader의 크기
 *  aChainedSlotBuffer         - [IN]  Chained된 Row를 저장해둘 임시 버퍼
 *  aUsedChainedSlotBufferSize - [OUT] 사용한 임시 버퍼의 크기
 *  aValue                     - [OUT] 읽은 Value
 *
 * Slot 의 구조
 * - VarInt (Length 기록에 이용됨)
 *      0~63        [0|len ]
 *     64~16383     [1|len>>8  & 63][len ]
 *  16384~4194303   [2|len>>16 & 63][len>>8 & 255][len & 255]
 *4194304~1073741824[3|len>>24 & 63][len>>16 & 255][len>>8 & 255][len & 255]
 *
 * - BasicColumn
 *   [Header:VarInt(1~4Byte)][Body]
 **********************************************************************/

static IDE_RC scpfReadBasicColumn( 
                    scpfBlockBufferPosition * aBlockBufferPosition,
                    scpfBlockInfo          ** aBlockMap,
                    UInt                      aBlockHeaderSize,
                    UChar                   * aChainedSlotBuffer,
                    UInt                    * aUsedChainedSlotBufSize,
                    smiValue                * aValue )
{
    scpfBlockInfo   * sBlockInfo;
    idBool            sReadNext;
    UInt              sSlotValLen;
    void            * sSlotVal;

    IDE_DASSERT( aBlockBufferPosition        != NULL );
    IDE_DASSERT( aBlockMap                   != NULL );
    IDE_DASSERT( aChainedSlotBuffer          != NULL );
    IDE_DASSERT( aUsedChainedSlotBufSize     != NULL );
    IDE_DASSERT( aValue                      != NULL );

    sSlotValLen = 0;
    sBlockInfo  = aBlockMap[ aBlockBufferPosition->mSeq ];

    // 복수의 Slot으로 이루어진 경우
    if( (aBlockBufferPosition->mSlotSeq  == sBlockInfo->mSlotCount - 1) &&
        (sBlockInfo->mHasLastChainedSlot == ID_TRUE) )
    {
        aValue->length = 0;
        aValue->value  = aChainedSlotBuffer + *aUsedChainedSlotBufSize;
        sReadNext      = ID_TRUE;

        while( sReadNext == ID_TRUE )
        {
            sBlockInfo = aBlockMap[ aBlockBufferPosition->mSeq ];

            sReadNext      = ID_FALSE;
            // 현재 읽는 Slot 가 ChainedSlot 인가
            if( (aBlockBufferPosition->mSlotSeq + 1 
                    == sBlockInfo->mSlotCount ) &&
                (sBlockInfo->mHasLastChainedSlot 
                    == ID_TRUE) )
            {
                sReadNext      = ID_TRUE;
            }

            // Slot을 하나 읽어서
            IDE_TEST( scpfReadSlot( aBlockBufferPosition,
                                    aBlockMap,
                                    aBlockHeaderSize,
                                    &sSlotValLen,
                                    &sSlotVal )
                      != IDE_SUCCESS);

            // ChainedBuffer에 복제한다.
            aValue->length += sSlotValLen;
            idlOS::memcpy( aChainedSlotBuffer + *aUsedChainedSlotBufSize,
                           sSlotVal,
                           sSlotValLen );
            (*aUsedChainedSlotBufSize) += sSlotValLen;
        }
    }
    else
    {
        // Chaining 안될 경우
        IDE_TEST( scpfReadSlot( aBlockBufferPosition,
                                aBlockMap,
                                aBlockHeaderSize,
                                &aValue->length,
                                (void**)&aValue->value )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description :
 *  Block으로부터 Lob Column을 읽는다.
 *  Slot이 하나 있는 경우만 이쪽으로 온다.
 *
 *  aStatistics                - [IN]  통계정보
 *  aBlockMap                  - [IN]  Slot을 읽을 Block
 *  aBlockHeaderSize           - [IN]  BlockHeader의 크기
 *  aValue                     - [OUT] 읽은 Value
 *
 * - Lob Column
 *   [LobLen (4byte)][Slot_Len(1~4Byte)][Body]
 ***********************************************************************/

static IDE_RC scpfReadLobColumn( scpfBlockBufferPosition * aBlockBufferPosition,
                                 scpfBlockInfo          ** aBlockMap,
                                 UInt                      aBlockHeaderSize,
                                 smiValue                * aValue )
{
    scpfBlockInfo   * sBlockInfo;
    UInt              sLobLength;

    IDE_DASSERT( aBlockBufferPosition!= NULL );
    IDE_DASSERT( aBlockMap           != NULL );
    IDE_DASSERT( aValue              != NULL );

    sBlockInfo = aBlockMap[ aBlockBufferPosition->mSeq ];

    // Block에는 Chained 안된 Slot이 있어야 한다
    IDE_TEST_RAISE( 
        (aBlockBufferPosition->mSlotSeq  == sBlockInfo->mSlotCount - 1) &&
        (sBlockInfo->mHasLastChainedSlot == ID_TRUE),
        ERR_ABORT_CORRUPTED_BLOCK  );

    // Lob Len
    SCPF_READ_UINT( sBlockInfo->mBlockPtr + aBlockBufferPosition->mOffset, 
                    &sLobLength );
    aBlockBufferPosition->mOffset += ID_SIZEOF(UInt);

    // Slot
    IDE_TEST( scpfReadSlot( aBlockBufferPosition,
                            aBlockMap,
                            aBlockHeaderSize,
                            &aValue->length,
                            (void**)&aValue->value )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_CORRUPTED_BLOCK );
    {
        IDE_SET( ideSetErrorCode( 
                smERR_ABORT_CORRUPTED_BLOCK,
                sBlockInfo->mBlockID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description :
 *  Block으로부터 Slot을 하나 읽는다.
 *
 *  aBlockBufferPosition       - [IN]  BlockBufferPosition
 *  aBlockMap                  - [IN]  Slot을 읽을 Block
 *  aBlockHeaderSize           - [IN]  BlockHeader의 크기
 *  aSlotValLen                - [OUT] 읽은 SlotVal의 length
 *  aSlotVal                   - [OUT] 읽은 Slotvalue
 *
 **********************************************************************/

static IDE_RC scpfReadSlot( scpfBlockBufferPosition * aBlockBufferPosition,
                            scpfBlockInfo          ** aBlockMap,
                            UInt                      aBlockHeaderSize,
                            UInt                    * aSlotValLen,
                            void                   ** aSlotVal )
{
    scpfBlockInfo   * sBlockInfo;

    IDE_DASSERT( aBlockBufferPosition        != NULL );
    IDE_DASSERT( aBlockMap                   != NULL );
    IDE_DASSERT( aSlotValLen                 != NULL );
    IDE_DASSERT( aSlotVal                    != NULL );

    sBlockInfo = aBlockMap[ aBlockBufferPosition->mSeq ];

    // SlotValLen 읽음
    SCPF_READ_VARINT( sBlockInfo->mBlockPtr + aBlockBufferPosition->mOffset,
                      aSlotValLen );
    aBlockBufferPosition->mOffset += SCPF_GET_VARINT_SIZE( aSlotValLen );
    aBlockBufferPosition->mSlotSeq  ++;

    // SlotVal 연결
    if( *aSlotValLen == 0 )
    {
        *aSlotVal = NULL;
    }
    else
    {
        *aSlotVal = sBlockInfo->mBlockPtr + aBlockBufferPosition->mOffset;
        aBlockBufferPosition->mOffset += *aSlotValLen;
    }

    // Block내의 모든 Slot 를 읽었을 경우
    if( aBlockBufferPosition->mSlotSeq  == sBlockInfo->mSlotCount )
    {
        aBlockBufferPosition->mSeq        ++;
        aBlockBufferPosition->mID         ++;
        aBlockBufferPosition->mOffset     = aBlockHeaderSize;
        aBlockBufferPosition->mSlotSeq    = 0;
    }

    return IDE_SUCCESS;

}


/***********************************************************************
 *
 * Description :
 *  지금까지 읽은 파일을 닫는다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Object Handle
 *
 **********************************************************************/
static IDE_RC scpfCloseFile4Import( idvSQL         * /*aStatistics*/,
                                    scpfHandle     * aHandle )
{
    UInt                      sFileState = 2;

    sFileState = 1;
    IDE_TEST( aHandle->mFile.close() != IDE_SUCCESS);

    aHandle->mOpenFile = ID_FALSE;
    sFileState = 0;
    IDE_TEST( aHandle->mFile.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sFileState )
    {
        case 2:
            (void) aHandle->mFile.close() ;
            aHandle->mOpenFile = ID_FALSE;
        case 1:
            (void) aHandle->mFile.destroy() ;
            break;
        default:
            break;
    }


    return IDE_FAILURE;
}

/***********************************************************************
 * Dump - mm/main/dumpte를 위한 함수들
 **********************************************************************/

/***********************************************************************
 *
 * Description :
 *  Block을 Dump한다
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Handle 
 *  aFirstBlock         - [IN] Dump할 Block의 시작 번호
 *  aLastBlock          - [IN] Dump할 Block의 끝 번호
 *  aDetail             - [IN] Body까지 자세히 출력할 것인가?
 *
 **********************************************************************/
IDE_RC scpfDumpBlocks( idvSQL         * aStatistics,
                       scpfHandle     * aHandle,
                       SLong            aFirstBlock,
                       SLong            aLastBlock,
                       idBool           aDetail )
{
    scpfBlockInfo * sBlockInfo;
    SChar         * sTempBuf;
    UInt            sState = 0;
    ULong           sFileOffset;
    UInt            sFlag;
    UInt            sDumpSize;
    UInt            i;
    UInt            j;

    IDE_DASSERT( aHandle    != NULL );


    if( aHandle->mFileHeader.mBlockCount < aLastBlock )
    {
        aLastBlock = aHandle->mFileHeader.mBlockCount;
    }

    if( aLastBlock < aFirstBlock )
    {
        aFirstBlock = aLastBlock-1;
    }



    /* Block 내부를 Dump한 결과값을 저장할 버퍼를 확보합니다. */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 1;


    /* Block Dump */
    sBlockInfo = aHandle->mBlockMap[0];
    for( i = aFirstBlock; i < aLastBlock; i ++ )
    {
        IDE_TEST( scpfReadBlock( aStatistics,
                                 &aHandle->mFile,
                                 aHandle->mCommonHandle.mHeader,
                                 aHandle->mFileHeader.mFileHeaderSize,
                                 aHandle->mFileHeader.mBlockHeaderSize,
                                 aHandle->mFileHeader.mBlockSize,
                                 i,
                                 &sFileOffset,
                                 sBlockInfo )
                  != IDE_SUCCESS );

        idlOS::printf( "[BlockID:%"ID_UINT32_FMT" - FileOffset:%"ID_UINT64_FMT"]\n", 
                       i,
                       sFileOffset );

        if( aDetail == ID_TRUE )
        {
            sFlag = SMI_DATAPORT_HEADER_FLAG_FULL;
        }
        else
        {
            sFlag = SMI_DATAPORT_HEADER_FLAG_DESCNAME_YES |
                SMI_DATAPORT_HEADER_FLAG_COLNAME_YES ;
        }

        sTempBuf[0] = '\0';
        IDE_TEST( smiDataPort::dumpHeader( 
                gScpfBlockHeaderDesc,
                aHandle->mCommonHandle.mHeader->mVersion,
                (void*)sBlockInfo,
                sFlag,
                sTempBuf,
                IDE_DUMP_DEST_LIMIT )
                  != IDE_SUCCESS );

        idlOS::printf( "%s\n",sTempBuf );

        if( aDetail == ID_TRUE )
        {
            for( j=0; j<aHandle->mFileHeader.mBlockSize; j+=IDE_DUMP_SRC_LIMIT )
            {
                sTempBuf[0] = '\0';
                sDumpSize   = aHandle->mFileHeader.mBlockSize - j;
                if( IDE_DUMP_SRC_LIMIT < sDumpSize )
                {
                    sDumpSize = IDE_DUMP_SRC_LIMIT;
                }
                IDE_TEST( scpfDumpBlockBody( sBlockInfo, 
                                             j,
                                             j + sDumpSize,
                                             sTempBuf, 
                                             IDE_DUMP_DEST_LIMIT )
                          != IDE_SUCCESS );
                idlOS::printf( "%s\n",sTempBuf );
            }
        }
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 1:
            (void) iduMemMgr::free( sTempBuf ) ;
        default:
            break;
    }


    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  Header를 Dump한다.
 *
 *  sHandle         - [IN] Dump할 대상
 *
 **********************************************************************/
IDE_RC scpfDumpHeader( scpfHandle     * aHandle,
                       idBool           aDetail )
{
    smiDataPortHeaderDesc        * sCommonHeaderDesc;
    smiDataPortHeaderDesc        * sObjectHeaderDesc;
    smiDataPortHeaderDesc        * sTableHeaderDesc;
    smiDataPortHeaderDesc        * sColumnHeaderDesc;
    smiDataPortHeaderDesc        * sPartitionHeaderDesc;
    UInt                           sHeaderSize;
    smiDataPortHeader            * sHeader;
    SChar                        * sTempBuf;
    UInt                           sState = 0;
    UInt                           sFlag;
    UInt                           i;

    IDE_DASSERT( aHandle != NULL     );

    /* Dump할 Buffer 확보 */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 1;

    sHeader = aHandle->mCommonHandle.mHeader;

    sCommonHeaderDesc     = gSmiDataPortHeaderDesc;
    sObjectHeaderDesc     = gScpfFileHeaderDesc;
    sTableHeaderDesc      = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getTableHeaderDescFunc();
    sColumnHeaderDesc     = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getColumnHeaderDescFunc();
    sPartitionHeaderDesc  = (smiDataPortHeaderDesc*)
        gSmiGlobalCallBackList.getPartitionHeaderDescFunc();

    if( aDetail == ID_TRUE )
    {
        sFlag = SMI_DATAPORT_HEADER_FLAG_FULL;
    }
    else
    {
        sFlag = SMI_DATAPORT_HEADER_FLAG_DESCNAME_YES |
            SMI_DATAPORT_HEADER_FLAG_COLNAME_YES ;
    }

    idlOS::snprintf( sTempBuf,
                     IDE_DUMP_DEST_LIMIT,
                     "----------- Header ----------\n" );
    IDE_TEST( smiDataPort::dumpHeader( sCommonHeaderDesc,
                                       sHeader->mVersion,
                                       sHeader,
                                       sFlag,
                                       sTempBuf,
                                       IDE_DUMP_DEST_LIMIT )
              != IDE_SUCCESS );

    if( aDetail == ID_TRUE )
    {
        IDE_TEST( smiDataPort::dumpHeader( sObjectHeaderDesc,
                                           sHeader->mVersion,
                                           sHeader->mObjectHeader,
                                           sFlag,
                                           sTempBuf,
                                           IDE_DUMP_DEST_LIMIT )
                  != IDE_SUCCESS );

        IDE_TEST( smiDataPort::dumpHeader( sTableHeaderDesc,
                                           sHeader->mVersion,
                                           sHeader->mTableHeader,
                                           sFlag,
                                           sTempBuf,
                                           IDE_DUMP_DEST_LIMIT )
                  != IDE_SUCCESS );
    }

    sHeaderSize = smiDataPort::getHeaderSize( sColumnHeaderDesc,
                                              sHeader->mVersion );
    for( i=0; i<sHeader->mColumnCount; i++ )
    {
        IDE_TEST( smiDataPort::dumpHeader( sColumnHeaderDesc,
                                           sHeader->mVersion,
                                           ((UChar*)sHeader->mColumnHeader)
                                           + sHeaderSize * i,
                                           sFlag,
                                           sTempBuf,
                                           IDE_DUMP_DEST_LIMIT )
                  != IDE_SUCCESS );
    }

    if( aDetail == ID_TRUE )
    {
        sHeaderSize = smiDataPort::getHeaderSize( sPartitionHeaderDesc,
                                                  sHeader->mVersion );
        for( i=0; i<sHeader->mPartitionCount; i++ )
        {
            IDE_TEST( smiDataPort::dumpHeader( 
                            sPartitionHeaderDesc,
                            sHeader->mVersion,
                            ((UChar*)sHeader->mPartitionHeader)
                            + sHeaderSize * i,
                            sFlag,
                            sTempBuf,
                            IDE_DUMP_DEST_LIMIT )
                        != IDE_SUCCESS );
        }
    }

    idlOS::printf("%s\n",sTempBuf);

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 1:
            (void) iduMemMgr::free( sTempBuf ) ;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  Block의 Body를 Dump한다.
 *
 *  aBlockInfo      - [IN]  Dump할 Block
 *  aBeginOffset    - [IN]  Block내 Dump할 시작 지점
 *  aEndOffset      - [IN]  Block내 Dump종료할 지점
 *  aOutBuf         - [OUT] 결과값을 저장할 버퍼
 *  aOutSize        - [OUT] 결과 버퍼의 크기
 *
 **********************************************************************/
IDE_RC scpfDumpBlockBody( scpfBlockInfo  * aBlockInfo,
                          UInt             aBeginOffset,
                          UInt             aEndOffset,
                          SChar          * aOutBuf,
                          UInt             aOutSize )
{
    SChar     * sTempBuf;
    UInt        sState = 0;

    IDE_DASSERT( aBlockInfo != NULL );
    IDE_DASSERT( aOutBuf    != NULL );

    /* Block 내부를 Dump한 결과값을 저장할 버퍼를 확보합니다. */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( ideLog::ideMemToHexStr( 
            aBlockInfo->mBlockPtr + aBeginOffset,
            aEndOffset - aBeginOffset,
            IDE_DUMP_FORMAT_NORMAL,
            sTempBuf,
            IDE_DUMP_DEST_LIMIT )
        != IDE_SUCCESS );


    idlVA::appendFormat( aOutBuf, aOutSize, "%s\n", sTempBuf );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 1:
            (void) iduMemMgr::free( sTempBuf ) ;
        default:
            break;
    }

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  Row의 Slot 들을 출력한다.
 *
 *  aStatistics         - [IN] 통계정보
 *  aHandle             - [IN] Handle 
 *
 **********************************************************************/
IDE_RC scpfDumpRows( idvSQL         * aStatistics,
                     scpfHandle     * aHandle )

{
    UInt        sRowCount;
    smiRow4DP * sRows;
    SChar     * sTempBuf;
    UInt        sDumpOffset;
    UInt        sDumpSize;
    UInt        sState = 0;
    UInt        sBasicColumnCount;
    UInt        sLobColumnCount;
    SLong       sRowSeq;
    UInt        sLobLength;
    UInt        sLobPieceLength;
    UChar     * sLobPieceValue;
    UInt        i;
    UInt        j;

    IDE_DASSERT( aStatistics != NULL );
    IDE_DASSERT( aHandle     != NULL );

    sBasicColumnCount  = aHandle->mCommonHandle.mHeader->mBasicColumnCount;
    sLobColumnCount    = aHandle->mCommonHandle.mHeader->mLobColumnCount;

    /* Row를 Dump할 Buffer 확보 */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCP, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 1;

    sRowSeq = aHandle->mRowSeq;

    IDE_TEST( scpfRead( aStatistics,
                        aHandle,
                        &sRows,
                        &sRowCount )
              != IDE_SUCCESS );

    while( 0 < sRowCount )
    {
        for( i=0; i<sRowCount; i ++)
        {
            // [ <RowSeq ]
            idlOS::printf(
                "[%8"ID_UINT64_FMT"]",
                sRowSeq );

            for( j=0; j<sBasicColumnCount; j++)
            {
                // [ <RowSeq ][ <ValueLength> |
                idlOS::printf(
                    "[%4"ID_UINT32_FMT"|",
                    sRows[i].mValueList[j].length );

                if( sRows[i].mValueList[j].value != NULL )
                {
                    for( sDumpOffset = 0;
                         sDumpOffset < sRows[i].mValueList[j].length;
                         sDumpOffset += IDE_DUMP_SRC_LIMIT )
                    {
                        sDumpSize = 
                            sRows[i].mValueList[j].length - sDumpOffset;

                        if( IDE_DUMP_SRC_LIMIT < sDumpSize )
                        {
                            sDumpSize = IDE_DUMP_SRC_LIMIT;
                        }

                        IDE_TEST( ideLog::ideMemToHexStr( 
                                (UChar*)sRows[i].mValueList[j].value
                                + sDumpOffset,
                                sDumpSize,
                                IDE_DUMP_FORMAT_VALUEONLY,
                                sTempBuf,
                                IDE_DUMP_DEST_LIMIT )
                            != IDE_SUCCESS );

                        // [ <RowSeq ][ <ValueLength> | <Value>
                        idlOS::printf( "%s", sTempBuf );

                        //이후 더 출력할 것이 있으면, 개행 후 추가 출력
                        if( ( sDumpOffset + IDE_DUMP_SRC_LIMIT ) 
                            < sRows[i].mValueList[j].length )
                        {
                            idlOS::printf( "\n" );
                        }
                    }
                    // [ <RowSeq ][ <ValueLength> | <Value> ]
                    idlOS::printf( "] " );
                }
                else
                {
                    // [ <RowSeq ][ <NULL> ]
                    idlOS::printf( "<NULL>] " );
                }

            }

            if( sRows[i].mHasSupplementLob == ID_FALSE )
            {
                for( j = sBasicColumnCount; 
                     j < sBasicColumnCount + sLobColumnCount;
                     j++)
                {
                    // [ <RowSeq ][ <ValueLength> |
                    idlOS::printf(
                        "[%4"ID_UINT32_FMT"|",
                        sRows[i].mValueList[j].length );

                    if( sRows[i].mValueList[j].value != NULL )
                    {
                        for( sDumpOffset = 0;
                             sDumpOffset < sRows[i].mValueList[j].length;
                             sDumpOffset += IDE_DUMP_SRC_LIMIT )
                        {
                            sDumpSize = 
                                sRows[i].mValueList[j].length - sDumpOffset;

                            if( IDE_DUMP_SRC_LIMIT < sDumpSize )
                            {
                                sDumpSize = IDE_DUMP_SRC_LIMIT;
                            }

                            IDE_TEST( ideLog::ideMemToHexStr( 
                                    (UChar*)sRows[i].mValueList[j].value
                                    + sDumpOffset,
                                    sDumpSize,
                                    IDE_DUMP_FORMAT_VALUEONLY,
                                    sTempBuf,
                                    IDE_DUMP_DEST_LIMIT )
                                != IDE_SUCCESS );

                            // [ <RowSeq ][ <ValueLength> | <Value>
                            idlOS::printf( "%s", sTempBuf );

                            //이후 더 출력할 것이 있으면, 개행 후 추가 출력
                            if( ( sDumpOffset + IDE_DUMP_SRC_LIMIT ) 
                                < sRows[i].mValueList[j].length )
                            {
                                idlOS::printf( "\n" );
                            }
                        }
                        // [ <RowSeq ][ <ValueLength> | <Value> ]
                        idlOS::printf( "] " );
                    }
                    else
                    {
                        // [ <RowSeq ][ <NULL> ]
                        idlOS::printf( "<NULL>] " );
                    }
                }
            }
            else
            {
                for( j = sBasicColumnCount; 
                     j < sBasicColumnCount + sLobColumnCount;
                     j++)
                {
                    IDE_TEST( scpfReadLobLength( aStatistics,
                                                 aHandle,
                                                 &sLobLength )
                              != IDE_SUCCESS );

                    idlOS::printf( "[%"ID_UINT32_FMT"|\n", sLobLength );

                    while( 0 < sLobLength )
                    {
                        IDE_TEST( scpfReadLob( aStatistics,
                                               aHandle,
                                               &sLobPieceLength,
                                               &sLobPieceValue )
                                  != IDE_SUCCESS );

                        for( sDumpOffset = 0;
                             sDumpOffset < sLobPieceLength;
                             sDumpOffset += IDE_DUMP_SRC_LIMIT )
                        {
                            sDumpSize = 
                                sLobPieceLength - sDumpOffset;

                            if( IDE_DUMP_SRC_LIMIT < sDumpSize )
                            {
                                sDumpSize = IDE_DUMP_SRC_LIMIT;
                            }

                            IDE_TEST( ideLog::ideMemToHexStr( 
                                    (UChar*)sLobPieceValue
                                    + sDumpOffset,
                                    sDumpSize,
                                    IDE_DUMP_FORMAT_VALUEONLY,
                                    sTempBuf,
                                    IDE_DUMP_DEST_LIMIT )
                                != IDE_SUCCESS );

                            idlOS::printf( "%s", sTempBuf );
                        }
                        sLobLength -= sLobPieceLength;
                    }
                    IDE_TEST( scpfFinishLobReading( aStatistics,
                                                    aHandle )
                              != IDE_SUCCESS );
                    idlOS::printf( "]" );
                }
            }
            idlOS::printf("\n");

            sRowSeq++;
        }

        IDE_TEST( scpfRead( aStatistics,
                            aHandle,
                            &sRows,
                            &sRowCount )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 1:
            (void) iduMemMgr::free( sTempBuf ) ;
        default:
            break;
    }

    return IDE_FAILURE;
}

