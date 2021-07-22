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
 * $Id: dumpddf.cpp 85333 2019-04-26 02:34:41Z et16 $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smu.h>
#include <sdp.h>
#include <sdn.h>

/* xxxxxxxxxxxxx */
#include <sdptb.h>
#include <sdcLob.h>
#include <sdtTempRow.h>
#include <sdcTableCTL.h>
#include <sdcTSSegment.h>
#include <sdtHashModule.h>
#include <sdcUndoSegment.h>


#define SM_DUMP_TEMP_PAGE_NONE      (0)
#define SM_DUMP_TEMP_PAGE_TEMP_HASH (1)
#define SM_DUMP_TEMP_PAGE_TEMP_SORT (2)

void showCopyRight( void );

idBool  gDumpDatafileHdr  = ID_FALSE;
idBool  gDisplayHeader    = ID_TRUE;
idBool  gInvalidArgs      = ID_FALSE;
SChar * gDbf              = NULL;
idBool  gReadHexa         = ID_FALSE;
UInt    gDumpTempPage     = SM_DUMP_TEMP_PAGE_NONE;

/* ideLog::ideMemToHexStr참조
 * IDE_DUMP_FORMAT_PIECE_4BYTE 기준 */
UInt    gReadHexaLineSize  = 32;
UInt    gReadHexaBlockSize = 4;

SInt    gPID               = -1;
UInt    gPageType          = 0;


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

IDE_RC dumpMeta()
{
    iduFile            sDataFile;
    sddDataFileHdr   * sDfHdr = NULL;
    ULong              sPage[ SD_PAGE_SIZE/ID_SIZEOF(ULong) ] = {0,};
    UInt               sState = 0;
    struct tm          sBackupEnd;
    struct tm          sBackupBegin;
    time_t             sTime;
    UInt               sBackupType;

    IDE_TEST( sDataFile.initialize(IDU_MEM_SM_SMU,
                                   1, /* Max Open FD Count */
                                   IDU_FIO_STAT_OFF,
                                   IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sDataFile.setFileName( gDbf ) != IDE_SUCCESS );
    IDE_TEST( sDataFile.open() != IDE_SUCCESS );
    sState = 2;

    (void)sDataFile.read( NULL,
                          0,
                          (void*)sPage,
                          SM_DBFILE_METAHDR_PAGE_SIZE,
                          NULL );

    sDfHdr = (sddDataFileHdr*) sPage;

    idlOS::printf( "[BEGIN DATABASE FILE HEADER]\n\n" );

    idlOS::printf( "Binary DB Version             [ %"ID_xINT32_FMT".%"ID_xINT32_FMT".%"ID_xINT32_FMT" ]\n",
                   ( ( sDfHdr->mSmVersion & SM_MAJOR_VERSION_MASK ) >> 24 ),
                   ( ( sDfHdr->mSmVersion & SM_MINOR_VERSION_MASK ) >> 16 ),
                   ( sDfHdr->mSmVersion  & SM_PATCH_VERSION_MASK) );

    idlOS::printf( "Redo LSN                      [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sDfHdr->mRedoLSN.mFileNo,
                   sDfHdr->mRedoLSN.mOffset );

    idlOS::printf( "Create LSN                    [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sDfHdr->mCreateLSN.mFileNo,
                   sDfHdr->mCreateLSN.mOffset );

    idlOS::printf( "MustRedo LSN                  [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sDfHdr->mMustRedoToLSN.mFileNo,
                   sDfHdr->mMustRedoToLSN.mOffset );

    //PROJ-2133 incremental backup
    idlOS::printf( "DataFileDescSlot ID           [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sDfHdr->mDataFileDescSlotID.mBlockID,
                   sDfHdr->mDataFileDescSlotID.mSlotIdx );

    idlOS::printf( "\n  [BEGIN BACKUPFILE INFORMATION]\n\n" );

    sTime =  (time_t)sDfHdr->mBackupInfo.mBeginBackupTime;
    idlOS::localtime_r( (time_t *)&sTime, &sBackupBegin );
    idlOS::printf( "        Begin Backup Time             [ %04"ID_UINT32_FMT"_%02"ID_UINT32_FMT"_%02"ID_UINT32_FMT"",
                   sBackupBegin.tm_year + 1900,
                   sBackupBegin.tm_mon  + 1,
                   sBackupBegin.tm_mday );
    idlOS::printf( " %02"ID_UINT32_FMT":%02"ID_UINT32_FMT":%02"ID_UINT32_FMT" ]\n",
                   sBackupBegin.tm_hour,
                   sBackupBegin.tm_min,
                   sBackupBegin.tm_sec );

    sTime =  (time_t)sDfHdr->mBackupInfo.mEndBackupTime;
    idlOS::localtime_r( (time_t *)&sTime, &sBackupEnd );
    idlOS::printf( "        End Backup Time               [ %04"ID_UINT32_FMT"_%02"ID_UINT32_FMT"_%02"ID_UINT32_FMT"",
                   sBackupEnd.tm_year + 1900,
                   sBackupEnd.tm_mon  + 1,
                   sBackupEnd.tm_mday );
    idlOS::printf( " %02"ID_UINT32_FMT":%02"ID_UINT32_FMT":%02"ID_UINT32_FMT" ]\n",
                   sBackupEnd.tm_hour,
                   sBackupEnd.tm_min,
                   sBackupEnd.tm_sec );

    idlOS::printf( "        IBChunk Count                 [ %"ID_UINT32_FMT" ]\n",
                   sDfHdr->mBackupInfo.mIBChunkCNT );

    switch( sDfHdr->mBackupInfo.mBackupTarget )
    {
        case SMRI_BI_BACKUP_TARGET_NONE:
            idlOS::printf( "        Backup Target                 [ NONE ]\n" );
            break;
        case SMRI_BI_BACKUP_TARGET_DATABASE:
            idlOS::printf( "        Backup Target                 [ DATABASE ]\n" );
            break;
        case SMRI_BI_BACKUP_TARGET_TABLESPACE:
            idlOS::printf( "        Backup Target                 [ TABLESPACE ]\n" );
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    switch( sDfHdr->mBackupInfo.mBackupLevel )
    {
        case SMI_BACKUP_LEVEL_0:
            idlOS::printf( "        Backup Level                  [ LEVEL0 ]\n" );
            break;
        case SMI_BACKUP_LEVEL_1:
            idlOS::printf( "        Backup Level                  [ LEVEL1 ]\n" );
            break;
        case SMI_BACKUP_LEVEL_NONE:
            idlOS::printf( "        Backup Level                  [ NONE ]\n" );
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    sBackupType = sDfHdr->mBackupInfo.mBackupType;
    idlOS::printf( "        Backup Type                   [ " );
    if( (sBackupType & SMI_BACKUP_TYPE_FULL) != 0 )
    {
        idlOS::printf( "FULL" );
        sBackupType &= ~SMI_BACKUP_TYPE_FULL;
        if( sBackupType != 0 )
        {
            idlOS::printf( ", " );
        }

    }
    if( (sBackupType & SMI_BACKUP_TYPE_DIFFERENTIAL) != 0 )
    {
        idlOS::printf( "DIFFERENTIAL" );
    }
    if( (sBackupType & SMI_BACKUP_TYPE_CUMULATIVE) != 0 )
    {
        idlOS::printf( "CUMULATIVE" );
    }
    idlOS::printf( " ]\n" );

    idlOS::printf( "        TableSpace ID                 [ %"ID_UINT32_FMT" ]\n",
                   sDfHdr->mBackupInfo.mSpaceID );

    idlOS::printf( "        File ID                       [ %"ID_UINT32_FMT" ]\n",
                   sDfHdr->mBackupInfo.mFileID );

    idlOS::printf( "        Backup Tag Name               [ %s ]\n",
                   sDfHdr->mBackupInfo.mBackupTag );

    idlOS::printf( "        Backup File Name              [ %s ]\n",
                   sDfHdr->mBackupInfo.mBackupFileName );

    idlOS::printf( "\n  [END BACKUPFILE INFORMATION]\n" );

    idlOS::printf( "\n[END DATABASE FILE HEADER]\n" );

    sState = 1;
    IDE_TEST( sDataFile.close() != IDE_SUCCESS );
    sState = 0;
    IDE_TEST( sDataFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 2:
            IDE_ASSERT( sDataFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sDataFile.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
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

    sLineCount     = aPageSize / gReadHexaLineSize;

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

    (void)aDataFile->read( NULL, 0 , (void*)sSrc, sSrcSize, NULL);

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


IDE_RC dumpPage()
{
    ULong           sBuf[ (SD_PAGE_SIZE/ID_SIZEOF(ULong)) * 2 ];
    iduFile         sDataFile;
    ULong           sOffset = SD_PAGE_SIZE * gPID;
    SChar         * sPage = (SChar*)sBuf + (SD_PAGE_SIZE - ((vULong)sBuf % SD_PAGE_SIZE));
    UInt            sPageType;
    SChar         * sTempBuf;
    sdpPhyPageHdr * sPhyPageHdr;
    UInt            sCalculatedChecksum;
    UInt            sChecksumInPage;
    UInt            sState = 0;

    IDE_TEST( sDataFile.initialize(IDU_MEM_SM_SMU,
                                   1, /* Max Open FD Count */
                                   IDU_FIO_STAT_OFF,
                                   IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sDataFile.setFileName(gDbf) != IDE_SUCCESS );
    IDE_TEST( sDataFile.open() != IDE_SUCCESS );
    sState = 2;

    idlOS::memset( sBuf, 0, (SD_PAGE_SIZE/ID_SIZEOF(ULong)) * 2 );

    sOffset += SM_DBFILE_METAHDR_PAGE_SIZE;

    idlOS::printf( "PID(%"ID_INT32_FMT"), Start Offset(%"ID_INT64_FMT")\n",gPID, sOffset);

    /* BUG-31206    improve usability of DUMPCI and DUMPDDF */
    if( gReadHexa == ID_TRUE )
    {
        IDE_TEST( readPage( &sDataFile,
                            sPage,
                            SD_PAGE_SIZE )
                  != IDE_SUCCESS );
    }
    else
    {
        (void)sDataFile.read( NULL, sOffset , (void*)sPage, SD_PAGE_SIZE, NULL);
    }

    switch( gDumpTempPage )
    {
        case SM_DUMP_TEMP_PAGE_TEMP_HASH :
            {
                smuUtility::printFuncWithBuffer( sdtHashModule::dumpHashRowPage,
                                                 sPage );
            }
            break;
        case SM_DUMP_TEMP_PAGE_TEMP_SORT :
            {
                smuUtility::printFuncWithBuffer( sdtTempPage::dumpTempPage,
                                                 sPage );
                smuUtility::printFuncWithBuffer( sdtTempRow::dumpTempPageByRow,
                                                 sPage );
            }
            break;
        case SM_DUMP_TEMP_PAGE_NONE :
            {
                /* 해당 메모리 영역을 Dump한 결과값을 저장할 버퍼를 확보합니다.
                 * Stack에 선언할 경우, 이 함수를 통해 서버가 종료될 수 있으므로
                 * Heap에 할당을 시도한 후, 성공하면 기록, 성공하지 않으면 그냥
                 * return합니다. */
                IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                             ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                             (void**)&sTempBuf )
                          != IDE_SUCCESS );
                sState = 3;

                /* page를 Hexa code로 dump하여 출력한다. */
                if( ideLog::ideMemToHexStr( (UChar*)sPage,
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

                /* PhyPageHeader를 dump하여 출력한다. */
                if( sdpPhyPage::dumpHdr( (UChar*) sPage,
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
                sPhyPageHdr = sdpPhyPage::getHdr( (UChar*)sPage );
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



                /* PageType을 지정해주지 않았을 경우, PhyPageHeader에서 읽어서 적용한다 */
                if( gPageType == 0 )
                {
                    sPageType = sdpPhyPage::getPageType( sPhyPageHdr );
                }
                else
                {
                    sPageType = gPageType;
                }

                // Logical Header 및 Body를 Dump한다.
                if( sPageType >= SDP_PAGE_TYPE_MAX )
                {
                    idlOS::printf("invalidate page type(%"ID_UINT32_FMT")\n", sPageType );
                }
                else
                {
                    if( gDumpVector[ sPageType ].dumpLogicalHdr( (UChar*) sPage,
                                                                 sTempBuf,
                                                                 IDE_DUMP_DEST_LIMIT )
                        == IDE_SUCCESS )
                    {
                        idlOS::printf( "%s", sTempBuf );
                    }

                    if( gDumpVector[ sPageType ].dumpPageBody( (UChar*) sPage,
                                                               sTempBuf,
                                                               IDE_DUMP_DEST_LIMIT )
                        == IDE_SUCCESS )
                    {
                        idlOS::printf( "%s", sTempBuf );
                    }
                }

                if( sdpSlotDirectory::dump( (UChar*) sPage,
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
            }
            break;
        default:
            break;
    }

    sState = 1;
    IDE_TEST( sDataFile.close() != IDE_SUCCESS );
    sState = 0;
    IDE_TEST( sDataFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
    case 3:
        IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
    case 2:
        IDE_ASSERT( sDataFile.close() == IDE_SUCCESS );
    case 1:
        IDE_ASSERT( sDataFile.destroy() == IDE_SUCCESS );
    default:
        break;
    }

    return IDE_FAILURE;
}

void usage()
{
    idlOS::printf( "\n%-6s:"
                   "dumpddf {-f file} [-m] [-p pid] [-t page_type] "
                   "[-e]\n",
                   "Usage" );

    idlOS::printf( "dumpddf (Altibase Ver. %s) ( SM Ver. version %s )\n\n",
                   iduVersionString,
                   smVersionString );
    idlOS::printf( " %-4s : %s\n", "-f", "specify database file name" );
    idlOS::printf( " %-4s : %s\n", "-m", "display datafile hdr" );
    idlOS::printf( " %-4s : %s\n", "-p", "specify page id" );
    idlOS::printf( " %-4s : specify page type [0~%"ID_UINT32_FMT",h,s]\n", "-t",
                   SDP_PAGE_TYPE_MAX );
}

void parseArgs( int &aArgc, char **&aArgv )
{
    SChar sOptString[] = "f:mhl:b:p:t:x";
    SInt  sOpr;

    sOpr = idlOS::getopt(aArgc, aArgv, sOptString );

    if( sOpr != EOF )
    {
        do
        {
            switch( sOpr )
            {
            case 'f':
                gDbf = optarg;
                break;
            case 'h':
                gReadHexa          = ID_TRUE;
                break;
            case 'l':
                gReadHexaLineSize  = idlOS::atoi( optarg );
                break;
            case 'b':
                gReadHexaBlockSize = idlOS::atoi( optarg );
                break;
            case 'm':
                gDumpDatafileHdr   = ID_TRUE;
                break;
            case 'p':
                gPID               = idlOS::atoi( optarg );
                break;
            case 't':
                if (( *optarg >= '0' ) && ( *optarg <= '9' ))
                {
                    gPageType      = idlOS::atoi( optarg );
                }
                else
                {
                    if( *optarg == 'h' )
                    {
                        gDumpTempPage = SM_DUMP_TEMP_PAGE_TEMP_HASH;
                    }
                    if( *optarg == 's' )
                    {
                        gDumpTempPage = SM_DUMP_TEMP_PAGE_TEMP_SORT;
                    }
                }
                break;
            case 'x':
                gDisplayHeader     = ID_FALSE;
                break;
            default:
                gInvalidArgs       = ID_TRUE;
                break;
            }
        }
        while ( (sOpr = idlOS::getopt(aArgc, aArgv, sOptString)) != EOF) ;
    }
    else
    {
        gInvalidArgs = ID_TRUE;
    }
}

int main(int aArgc, char *aArgv[])
{
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST_RAISE(iduCond::initializeStatic() != IDE_SUCCESS,
                   err_with_ide_dump );


    initializeStatic();

    parseArgs( aArgc, aArgv );

    if(gDisplayHeader == ID_TRUE)
    {
        showCopyRight();
    }

    IDE_TEST_RAISE(gInvalidArgs == ID_TRUE, err_invalid_args);

    IDE_TEST_RAISE( gPageType >= SDP_PAGE_TYPE_MAX,
                    err_invalid_page_type );


    IDE_TEST_RAISE( gDbf == NULL,
                    err_not_specified_file );

    IDE_TEST_RAISE( idlOS::access( gDbf, F_OK ) != 0,
                    err_not_exist_file );

    if( gDumpDatafileHdr == ID_TRUE )
    {
        IDE_TEST_RAISE( dumpMeta() != IDE_SUCCESS,
                        err_with_ide_dump );
    }
    if( (gPID >= 0 ) || ( gReadHexa  == ID_TRUE ) )
    {
        IDE_TEST_RAISE( dumpPage() != IDE_SUCCESS,
                        err_with_ide_dump );
    }

    IDE_TEST_RAISE( ( gDumpDatafileHdr != ID_TRUE ) &&
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
    IDE_EXCEPTION( err_invalid_page_type );
    {
        idlOS::printf( "\nerror : invalid page type(%"ID_UINT32_FMT")\n\n",
                       gPageType);
    }
    IDE_EXCEPTION( err_not_exist_file );
    {
        idlOS::printf( "\nerror : can't access db file\n\n");
    }
    IDE_EXCEPTION( err_not_specified_file );
    {
        idlOS::printf( "\nerror : Please specify a db file\n\n");

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
    const SChar * sBanner = "DUMPDDF.ban";

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
