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
 * $Id$
 **********************************************************************/

#include <smu.h>
#include <sdp.h>
#include <sdn.h>
#include <sdptb.h>
#include <sdcTableCTL.h>
#include <sdcLob.h>
#include <sdcTSSegment.h>
#include <sdcUndoSegment.h>
#include <sdsBCB.h>
#include <sdsBufferArea.h>
#include <sdsFile.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h>
 
 /* get frame(F) offset 
 * |F|F|F|F|F|M |F|F|F|F|F|M | 
   ^ here
 */
#define SDS_META_TABLE_SEQ(id)      ( id / SDS_META_DATA_MAX_COUNT_IN_GROUP )
#define SDS_META_TABLES_LENTH(id)   ( SDS_META_TABLE_SEQ(id) *                 \
                                      SDS_META_TABLE_SIZE )
#define SDS_META_FRAME_OFFSET(id)   ( id*SD_PAGE_SIZE )          
#define SDS_MAKE_FOFFSET(id)        ( SDS_META_FRAME_OFFSET(id) +              \
                                      SDS_META_TABLES_LENTH(id) )

/* get MetaTable(M) offset
 * |F|F|F|F|F|M |F|F|F|F|F|M | 
 ^ here
 */
#define SDS_META_FRAMES_LENTH(seq)  ( (seq+1) * SDS_META_FRAME_MAX_COUNT_IN_GROUP \
                                      * SD_PAGE_SIZE )
#define SDS_META_TABLE_OFFSET(seq)  ( seq*SDS_META_TABLE_SIZE )
#define SDS_MAKE_TOFFSET(seq)       ( SDS_META_TABLE_OFFSET(seq) +                \
                                      SDS_META_FRAMES_LENTH(seq) )


void showCopyRight( void );

idBool  gSBufferFileHdr    = ID_FALSE;
idBool  gDisplayHeader     = ID_TRUE;
idBool  gInvalidArgs       = ID_FALSE;
SChar * gSbf               = NULL;
idBool  gReadHexa          = ID_FALSE;
idBool  gHideHexa          = ID_FALSE;

/* ideLog::ideMemToHexStr참조 
 * IDE_DUMP_FORMAT_PIECE_4BYTE 기준 */
UInt    gReadHexaLineSize  = 32;
UInt    gReadHexaBlockSize = 4;

SInt    gPID               = -1;

SInt    gSBMetaTable       = -1;

/*************************************************************
 * Dump Map
 *************************************************************/

#define SET_DUMP_HDR( aType, aFunc )              \
    IDE_ASSERT( aType <  SDP_PAGE_TYPE_MAX );     \
    gDumpVector[ aType ].dumpLogicalHdr = aFunc;

#define SET_DUMP_BODY( aType, aFunc )             \
    IDE_ASSERT( aType <  SDP_PAGE_TYPE_MAX );     \
    gDumpVector[ aType ].dumpPageBody = aFunc;

typedef struct dumpFuncType
{
    smPageDump dumpLogicalHdr;    // logical hdr print
    smPageDump dumpPageBody;      // page의 내용 print
} dumpFuncType;

dumpFuncType gDumpVector[ SDP_PAGE_TYPE_MAX ];

IDE_RC dummy( UChar * /*aPageHdr*/,
              SChar * aOutBuf,
              UInt    aOutSize )
{
     idlOS::snprintf( aOutBuf, 
                      aOutSize,
                      "dump : Nothing to display\n" );
 
    return IDE_SUCCESS ;
}

void initializeStatic()
{
    UInt i;

    /* initialze map */
    for( i = 0; i < SDP_PAGE_TYPE_MAX ; i ++ )
    {
        SET_DUMP_HDR(  i, dummy );
        SET_DUMP_BODY( i, dummy );
    }

    SET_DUMP_BODY( SDP_PAGE_INDEX_META_BTREE, sdnbBTree::dumpMeta );

    SET_DUMP_HDR(  SDP_PAGE_INDEX_BTREE, sdnbBTree::dumpNodeHdr );
    SET_DUMP_BODY( SDP_PAGE_INDEX_BTREE, sdnbBTree::dump );

    SET_DUMP_HDR(  SDP_PAGE_DATA, sdcTableCTL::dump );
    SET_DUMP_BODY( SDP_PAGE_DATA, sdcRow::dump );

    SET_DUMP_BODY( SDP_PAGE_TSS, sdcTSSegment::dump );

    SET_DUMP_BODY( SDP_PAGE_UNDO, sdcUndoSegment::dump );

    SET_DUMP_HDR(  SDP_PAGE_LOB_DATA, sdcLob::dumpLobDataPageHdr ) ;
    SET_DUMP_BODY( SDP_PAGE_LOB_DATA, sdcLob::dumpLobDataPageBody ) ;

    SET_DUMP_HDR(  SDP_PAGE_TMS_SEGHDR, sdpstSH::dumpHdr  );
    SET_DUMP_BODY( SDP_PAGE_TMS_SEGHDR, sdpstSH::dumpBody );

    SET_DUMP_HDR(  SDP_PAGE_TMS_LFBMP, sdpstLfBMP::dumpHdr  );
    SET_DUMP_BODY( SDP_PAGE_TMS_LFBMP, sdpstLfBMP::dumpBody );

    SET_DUMP_HDR(  SDP_PAGE_TMS_ITBMP, sdpstBMP::dumpHdr  );
    SET_DUMP_BODY( SDP_PAGE_TMS_ITBMP, sdpstBMP::dumpBody );

    SET_DUMP_HDR(  SDP_PAGE_TMS_RTBMP, sdpstBMP::dumpHdr  );
    SET_DUMP_BODY( SDP_PAGE_TMS_RTBMP, sdpstBMP::dumpBody );

    SET_DUMP_HDR(  SDP_PAGE_TMS_EXTDIR, sdpstExtDir::dumpHdr  );
    SET_DUMP_BODY( SDP_PAGE_TMS_EXTDIR, sdpstExtDir::dumpBody );

    SET_DUMP_HDR(  SDP_PAGE_FEBT_GGHDR, sdptbVerifyAndDump::dumpGGHdr );

    SET_DUMP_HDR(  SDP_PAGE_FEBT_LGHDR, sdptbVerifyAndDump::dumpLGHdr );

    SET_DUMP_BODY( SDP_PAGE_LOB_META, sdcLob::dumpLobMeta );
}

UInt htoi( UChar *aSrc )
{
    UInt sRet = 0;

    if( ( '0' <= (*aSrc) ) && ( (*aSrc) <= '9' ) )
    {
        sRet = ( (*aSrc) - '0' );
    }
    else if( ( 'a' <= (*aSrc) ) &&  ( (*aSrc) <= 'f' ) )
    {
        sRet = ( (*aSrc) - 'a' ) + 10;
    }
    else if( ( 'A' <= (*aSrc) ) &&  ( (*aSrc) <= 'F' ) )
    {
        sRet = ( (*aSrc) - 'A' ) + 10;
    }

    return sRet;
}

/* BUG-31206   improve usability of DUMPCI and DUMPDDF */
IDE_RC readPage( iduFile * aDataFile,
                 SChar   * aOutBuf,
                 UInt      aPageSize )
{
    UInt         sSrcSize;
    UInt         sLineCount;
    UInt         sLineFormatSize;
    UChar      * sSrc;
    UChar        sTempValue;
    UInt         sOffsetInSrc;
    UInt         i;
    UInt         j;
    UInt         k;
    UInt         sState = 0;

    sLineCount = aPageSize / gReadHexaLineSize;

    /* ideLog::ideMemToHexStr참조, Full format */
    sLineFormatSize = 
        gReadHexaLineSize * 2 +                  /* Body             */
        gReadHexaLineSize / gReadHexaBlockSize + /* Body Seperator   */
        20+                                      /* AbsAddr          */
        2 +                                      /* AbsAddr Format   */
        10+                                      /* RelAddr          */
        2 +                                      /* RelAddr Format   */
        gReadHexaLineSize +                      /* Character        */
        2 +                                      /* Character Format */
        1;                                       /* Carriage Return  */

    sSrcSize = sLineFormatSize * sLineCount;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID, 
                                 (ULong)ID_SIZEOF( SChar ) * sSrcSize,
                                 (void**)&sSrc )
              != IDE_SUCCESS );
    sState = 1;

    (void)aDataFile->read( NULL, 	/* aStatistics */
                           0,    	/* sWhere */
                           (void*)sSrc, 
                           sSrcSize, 
                           NULL ); 	/* setEmergency */

    sOffsetInSrc = 0 ; 

    for( i = 0 ;  i < sLineCount ; i ++ )
    {
        /* Body 시작 부분 탐색 */
        for( j = 0 ; 
             j < sLineFormatSize ; 
             j++, sOffsetInSrc ++ )
        {
            if( ( sSrc[ sOffsetInSrc ] == '|' ) && 
                ( sSrc[ sOffsetInSrc + 1 ] == ' ' ) )
            {
                /* Address 구분자 '| ' */
                sOffsetInSrc += 2;
                break;
            }
        }

        /* Address 구분자를 찾지 못했을 경우 */
        if( j == sLineFormatSize )
        {
            break;
        }

        for( j = 0 ; j < gReadHexaLineSize/gReadHexaBlockSize ; j ++ )
        {
            for( k = 0 ; k < gReadHexaBlockSize ; k ++ )
            {
                sTempValue = 
                    ( htoi( &sSrc[ sOffsetInSrc ] ) * 16)
                    + htoi( &sSrc[ sOffsetInSrc + 1] ) ;
                sOffsetInSrc += 2;

                *aOutBuf = sTempValue;
                aOutBuf ++;
            }
            sOffsetInSrc ++ ; /* Body Seperator */
        }
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sSrc ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
    case 1:
        IDE_ASSERT( iduMemMgr::free( sSrc ) == IDE_SUCCESS );
    default:
        break;
    }

    return IDE_FAILURE;
}

/*************************************************************
 * Dump Secondary Buffer File Header 
 *************************************************************/
IDE_RC dumpHdr()
{
    iduFile      sSBufferFile;
    sdsFileHdr * sSbfHdr = NULL;
    ULong        sAlignedPage[ SD_PAGE_SIZE/ID_SIZEOF(ULong) ] = {0,};
    UInt         sState = 0;

    IDE_TEST( sSBufferFile.initialize( IDU_MEM_SM_SMU,
                                       1, /* Max Open FD Count */
                                       IDU_FIO_STAT_OFF,
                                       IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSBufferFile.setFileName( gSbf ) != IDE_SUCCESS );
    IDE_TEST( sSBufferFile.open() != IDE_SUCCESS );
    sState = 2;

    (void)sSBufferFile.read( NULL,
                             0, 
                             (void*)sAlignedPage, 
                             SM_DBFILE_METAHDR_PAGE_SIZE, 
                             NULL );

    sSbfHdr = (sdsFileHdr*) sAlignedPage;

    idlOS::printf( "[BEGIN SECONDARY BUFFER FILE HEADER]\n\n" );

    idlOS::printf( "Binary DB Version              [ %"ID_xINT32_FMT".%"ID_xINT32_FMT".%"ID_xINT32_FMT" ]\n",
                   ( ( sSbfHdr->mSmVersion & SM_MAJOR_VERSION_MASK ) >> 24 ),
                   ( ( sSbfHdr->mSmVersion & SM_MINOR_VERSION_MASK ) >> 16 ),
                   ( sSbfHdr->mSmVersion  & SM_PATCH_VERSION_MASK) );

    idlOS::printf( "Redo LSN                       [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sSbfHdr->mRedoLSN.mFileNo,
                   sSbfHdr->mRedoLSN.mOffset );

    idlOS::printf( "Create LSN                     [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sSbfHdr->mCreateLSN.mFileNo,
                   sSbfHdr->mCreateLSN.mOffset );

    idlOS::printf( "Frame Count in Extent          [ %"ID_UINT32_FMT" ]\n",
                   sSbfHdr->mFrameCntInExt );

    idlOS::printf( "Last MetaTable Sequence Number [ %"ID_UINT32_FMT" ]\n",
                   sSbfHdr->mLstMetaTableSeqNum );

    idlOS::printf( "\n[END SECONDATY FILE HEADER]\n" );

    sState = 1;
    IDE_TEST( sSBufferFile.close() != IDE_SUCCESS );
    sState = 0;
    IDE_TEST( sSBufferFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 2:
            IDE_ASSERT( sSBufferFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sSBufferFile.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/*************************************************************
 * Dump Secondary Buffer File Frame
 *************************************************************/
IDE_RC dumpPage()
{
    ULong           sBuf[ (SD_PAGE_SIZE/ID_SIZEOF(ULong)) * 2 ];
    iduFile         sSBufferFile;
    ULong           sOffset;
    SChar         * sAlignedPage = (SChar*)sBuf + ( SD_PAGE_SIZE - ((vULong)sBuf % SD_PAGE_SIZE) );
    UInt            sPageType;
    SChar         * sTempBuf;
    sdpPhyPageHdr * sPhyPageHdr;
    UInt            sCalculatedChecksum;
    UInt            sChecksumInPage;
    UInt            sState = 0;

    sOffset = SDS_MAKE_FOFFSET( gPID );

    IDE_TEST( sSBufferFile.initialize( IDU_MEM_SM_SMU,
                                       1, /* Max Open FD Count */
                                       IDU_FIO_STAT_OFF,
                                       IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    
    IDE_TEST( sSBufferFile.setFileName(gSbf) != IDE_SUCCESS );
    IDE_TEST( sSBufferFile.open() != IDE_SUCCESS );
    sState = 2;

    idlOS::memset( sBuf, 0, (SD_PAGE_SIZE/ID_SIZEOF(ULong)) * 2 );

    sOffset += SM_DBFILE_METAHDR_PAGE_SIZE;

    idlOS::printf( "PID(%"ID_INT32_FMT"), Start Offset(%"ID_INT64_FMT")\n",gPID, sOffset);

    /* BUG-31206    improve usability of DUMPCI and DUMPDDF */
    if( gReadHexa == ID_TRUE )
    {
        IDE_TEST( readPage( &sSBufferFile,
                            sAlignedPage, 
                            SD_PAGE_SIZE ) 
                  != IDE_SUCCESS );
    }
    else
    {
        (void)sSBufferFile.read( NULL, sOffset, (void*)sAlignedPage, SD_PAGE_SIZE, NULL);
    }

    /* 해당 메모리 영역을 Dump한 결과값을 저장할 버퍼를 확보합니다.
     * Stack에 선언할 경우, 이 함수를 통해 서버가 종료될 수 있으므로
     * Heap에 할당을 시도한 후, 성공하면 기록, 성공하지 않으면 그냥
     * return합니다. */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 3;

    if( gHideHexa != ID_TRUE )
    {
        /* page를 Hexa code로 dump하여 출력한다. */
        if( ideLog::ideMemToHexStr( (UChar*)sAlignedPage, 
                                    SD_PAGE_SIZE,
                                    IDE_DUMP_FORMAT_NORMAL,
                                    sTempBuf,
                                    IDE_DUMP_DEST_LIMIT )
            == IDE_SUCCESS )
        {
            idlOS::printf("%s\n", sTempBuf );
        }
        else
        {
            /* nothing to do ... */
        }
    } 
    /* PhyPageHeader를 dump하여 출력한다. */
    if( sdpPhyPage::dumpHdr( (UChar*) sAlignedPage,
                             sTempBuf,
                             IDE_DUMP_DEST_LIMIT )
        == IDE_SUCCESS )
    {
        idlOS::printf("%s", sTempBuf );
    }
    else
    {
        /* nothing to do ... */
    }
    
    /* BUG-31628 [sm-util] dumpddf does not check checksum of DRDB page.
     * sdpPhyPage::isPageCorrupted 함수는 property에 따라 Checksum검사를
     * 안 할 수도 있기 때문에 항상 하도록 수정합니다. */
    sPhyPageHdr = sdpPhyPage::getHdr( (UChar*)sAlignedPage );
    sCalculatedChecksum = sdpPhyPage::calcCheckSum( sPhyPageHdr );
    sChecksumInPage     = sdpPhyPage::getCheckSum( sPhyPageHdr );

    if( sCalculatedChecksum == sChecksumInPage )
    {
        idlOS::printf( "This page(%"ID_UINT32_FMT") is not corrupted.\n",
                       gPID );
    }
    else
    {
        idlOS::printf( "This page(%"ID_UINT32_FMT") is corrupted."
                       "( %"ID_UINT32_FMT" == %"ID_UINT32_FMT" )\n",
                       gPID,
                       sCalculatedChecksum,
                       sChecksumInPage );
    }

    sPageType = sdpPhyPage::getPageType( sPhyPageHdr );

    // Logical Header 및 Body를 Dump한다.
    if( sPageType >= SDP_PAGE_TYPE_MAX )
    {
        idlOS::printf("invalidate page type(%"ID_UINT32_FMT")\n", sPageType );
    }
    else
    {
        if( gDumpVector[ sPageType ].dumpLogicalHdr( (UChar*) sAlignedPage,
                                                     sTempBuf,
                                                     IDE_DUMP_DEST_LIMIT )
            == IDE_SUCCESS )
        {
            idlOS::printf( "%s", sTempBuf );
        }

        if( gDumpVector[ sPageType ].dumpPageBody( (UChar*) sAlignedPage,
                                                   sTempBuf,
                                                   IDE_DUMP_DEST_LIMIT )
            == IDE_SUCCESS )
        {
            idlOS::printf( "%s", sTempBuf );
        }
    }

    if( sdpSlotDirectory::dump( (UChar*) sAlignedPage,
                                sTempBuf,
                                IDE_DUMP_DEST_LIMIT )
         == IDE_SUCCESS )
    {
        idlOS::printf("%s", sTempBuf );
    }
    else
    {
        /* nothing to do ... */
    }

    sState = 2;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );
    sState = 1;
    IDE_TEST( sSBufferFile.close() != IDE_SUCCESS );
    sState = 0;
    IDE_TEST( sSBufferFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
    case 3:
        IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
    case 2:
        IDE_ASSERT( sSBufferFile.close() == IDE_SUCCESS );
    case 1:
        IDE_ASSERT( sSBufferFile.destroy() == IDE_SUCCESS );
    default:
        break;
    }

    return IDE_FAILURE;
}

/*************************************************************
 * Dump Secondary Buffer One Meta Table
 *************************************************************/
IDE_RC dumpMetaTable()
{
    ULong     sBuf[ (SDS_META_TABLE_SIZE/ID_SIZEOF(ULong)) + SD_PAGE_SIZE ];
    iduFile	  sSBufferFile;
    ULong     sOffset;
    SChar   * sAlignedPage = (SChar*)sBuf + ( SD_PAGE_SIZE - ((vULong)sBuf % SD_PAGE_SIZE) );
    SChar   * sTempBuf;
    UInt      sState = 0;

    sOffset  = SDS_MAKE_TOFFSET( gSBMetaTable );
    sOffset += SM_DBFILE_METAHDR_PAGE_SIZE;

    IDE_TEST( sSBufferFile.initialize( IDU_MEM_SM_SMU,
                                       1,               /* Max Open FD Count */
                                       IDU_FIO_STAT_OFF,
                                       IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSBufferFile.setFileName(gSbf) != IDE_SUCCESS );
    IDE_TEST( sSBufferFile.open() != IDE_SUCCESS );
    sState = 2;

    idlOS::memset( sBuf, 0, SDS_META_TABLE_SIZE + SD_PAGE_SIZE );
 
    idlOS::printf( "Meta Table Seq(%"ID_INT32_FMT")," 
                   "Start Offset(%"ID_INT64_FMT") (%"ID_INT64_FMT"page) \n",
                   gSBMetaTable, 
                   sOffset, 
                   sOffset/SD_PAGE_SIZE);

    /* 해당 메모리 영역을 Dump한 결과값을 저장할 버퍼를 확보합니다.
     * Stack에 선언할 경우, 이 함수를 통해 서버가 종료될 수 있으므로
     * Heap에 할당을 시도한 후, 성공하면 기록, 성공하지 않으면 그냥
     * return합니다. */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 3;

    (void)sSBufferFile.read( NULL, 
                             sOffset, 
                             (void*)sAlignedPage, 
                             SDS_META_TABLE_SIZE, 
                             NULL );

    if( gHideHexa != ID_TRUE )
    {
        /* page를 Hexa code로 dump하여 출력한다. */
        if( ideLog::ideMemToHexStr( (UChar*)sAlignedPage, 
                                    SDS_META_TABLE_SIZE,
                                    IDE_DUMP_FORMAT_NORMAL,
                                    sTempBuf,
                                    IDE_DUMP_DEST_LIMIT )
            == IDE_SUCCESS )
        {
            idlOS::printf("%s\n", sTempBuf );
        }
        else
        {
            /* nothing to do ... */
        }
    }

    sdsBufferMgr::dumpMetaTable( (UChar*) sAlignedPage+ID_SIZEOF(UInt)+ID_SIZEOF(smLSN),
                                 sTempBuf,
                                 IDE_DUMP_DEST_LIMIT );

    idlOS::printf("%s", sTempBuf );
    idlOS::printf("---------------------------------------------\n" );

    sState = 2;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );
    sState = 1;
    IDE_TEST( sSBufferFile.close() != IDE_SUCCESS );
    sState = 0;
    IDE_TEST( sSBufferFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 3:
            IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( sSBufferFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sSBufferFile.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

void usage()
{
    idlOS::printf( "\n%-6s:  dumpsbf {-f file} [-m] [-p pid] [-e sid] \n", "Usage" );

    idlOS::printf( "dumpsbf (Altibase Ver. %s) ( SM Ver. version %s )\n\n",
                   iduVersionString,
                   smVersionString );
    idlOS::printf( " %-4s : %s\n", "-f", "specify database file name" );
    idlOS::printf( " %-4s : %s\n", "-m", "display second buffer file hdr" );
    idlOS::printf( " %-4s : %s\n", "-e", "display meta table" );
    idlOS::printf( " %-4s : %s\n", "-p", "specify page id" );
}

void parseArgs( int &aArgc, char **&aArgv )
{
    SChar sOptString[] = "f:mhl:b:e:p:x:i";
    SInt  sOpr;
    
    sOpr = idlOS::getopt(aArgc, aArgv, sOptString );
    
    if( sOpr != EOF )
    {
        do
        {
            switch( sOpr )
            {
            case 'f':
                gSbf = optarg;
                break;
            case 'm':
                gSBufferFileHdr = ID_TRUE;
                break;
            case 'e':
                gSBMetaTable = idlOS::atoi( optarg );
                break;
            case 'p':
                gPID = idlOS::atoi( optarg );
                break;
/* 숨겨진 옵션들 ?? */
            case 'i':
                gHideHexa = ID_TRUE;
                break;
            case 'h':
                gReadHexa = ID_TRUE;
                break;
            case 'l':
                gReadHexaLineSize = idlOS::atoi( optarg );
                break;
            case 'b':
                gReadHexaBlockSize = idlOS::atoi( optarg );
                break;
            case 'x':
                gDisplayHeader = ID_FALSE;
                break;
            default:
                gInvalidArgs = ID_TRUE;
                break;
            }
        }                                     
        while ( ( sOpr = idlOS::getopt( aArgc, aArgv, sOptString ) ) != EOF );
    }
    else
    {
        gInvalidArgs = ID_TRUE;
    }
}

int main(int aArgc, char *aArgv[])
{
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST_RAISE( iduCond::initializeStatic() != IDE_SUCCESS,
                    err_with_ide_dump );

    initializeStatic();

    parseArgs( aArgc, aArgv );

    if( gDisplayHeader == ID_TRUE )
    {
        showCopyRight();
    }

    IDE_TEST_RAISE( gInvalidArgs == ID_TRUE, err_invalid_args );

    IDE_TEST_RAISE( gSbf == NULL,
                    err_not_specified_file );

    IDE_TEST_RAISE( idlOS::access( gSbf, F_OK ) != 0,
                    err_not_exist_file );

    if( gSBufferFileHdr == ID_TRUE )
    {
        IDE_TEST_RAISE( dumpHdr() != IDE_SUCCESS,
                        err_with_ide_dump );
    }
    if( gSBMetaTable >= 0 )
    {
       IDE_TEST_RAISE( dumpMetaTable() != IDE_SUCCESS,
                       err_with_ide_dump );

    }
    if( (gPID >= 0 ) || ( gReadHexa  == ID_TRUE ) )
    {
        IDE_TEST_RAISE( dumpPage() != IDE_SUCCESS,
                        err_with_ide_dump );
    }

    IDE_TEST_RAISE( ( gSBufferFileHdr != ID_TRUE ) && 
                    ( gSBMetaTable < 0) &&
                    ( gPID < 0 ) &&
                    ( gReadHexa != ID_TRUE ),
                    err_no_display_option );

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::destroyStatic();

    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_with_ide_dump );
    {
        ideDump();
    }
    IDE_EXCEPTION( err_no_display_option );
    {
        idlOS::printf( "\nerror : no display option\n\n" );
    }
    IDE_EXCEPTION( err_invalid_args );
    {
        (void)usage();
    }
    IDE_EXCEPTION( err_not_exist_file );
    {
        idlOS::printf( "\nerror : can't access file [ %s]\n\n",gSbf );
    }
    IDE_EXCEPTION( err_not_specified_file );
    {
        idlOS::printf( "\nerror : Please specify a secondary buffer file\n\n");

        (void)usage();
    }
 
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

// BUG-28510 dump유틸들의 Banner양식을 통일해야 합니다.
// DUMPCI.ban을 통해 dumpci의 타이틀을 출력해줍니다.
void showCopyRight( void )
{
    SChar         sBuf[1024 + 1];
    SChar         sBannerFile[1024];
    SInt          sCount;
    FILE        * sFP;
    SChar       * sAltiHome;
    const SChar * sBanner = "DUMPSBF.ban";

    sAltiHome = idlOS::getenv( IDP_HOME_ENV );
    IDE_TEST_RAISE( sAltiHome == NULL, err_altibase_home );

    // make full path banner file name
    idlOS::memset(   sBannerFile, 0, ID_SIZEOF( sBannerFile ) );
    idlOS::snprintf( sBannerFile, 
                     ID_SIZEOF( sBannerFile ), 
                     "%s%c%s%c%s",
                     sAltiHome, 
                     IDL_FILE_SEPARATOR, 
                     "msg",
                     IDL_FILE_SEPARATOR, 
                     sBanner );

    sFP = idlOS::fopen( sBannerFile, "r" );
    IDE_TEST_RAISE( sFP == NULL, err_file_open );

    sCount = idlOS::fread( (void*) sBuf, 1, 1024, sFP );
    IDE_TEST_RAISE( sCount <= 0, err_file_read );

    sBuf[sCount] = '\0';
    idlOS::printf( "%s", sBuf );
    idlOS::fflush( stdout );
    idlOS::fclose( sFP );

    IDE_EXCEPTION( err_altibase_home );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_open );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_read );
    {
        idlOS::fclose( sFP );
    }

    IDE_EXCEPTION_END;
}

