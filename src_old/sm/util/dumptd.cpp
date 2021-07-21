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
 * $Id: dumptdf.cpp 52614 2012-04-05 08:35:30Z sliod $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <iduMemPoolMgr.h>
#include <smu.h>
#include <smi.h>
#include <sdb.h>
#include <sdt.h>

void usage();
void parseArgs( int &aArgc, char **&aArgv );
IDE_RC processSortDump();
IDE_RC processHashDump();
IDE_RC loadTempDump();

typedef enum 
{
    SMUTIL_JOB_NONE          = -1,
    SMUTIL_JOB_WASEGHEADER   = 0,
    SMUTIL_JOB_WCB           = 1,
    SMUTIL_JOB_WAPAGEHEADERS = 2,
    SMUTIL_JOB_WAPAGE        = 3,
    SMUTIL_JOB_EXTENTMAP     = 4,
    SMUTIL_JOB_SORTHASHMAP   = 5,
    SMUTIL_JOB_MAX           = 6
} smuJobType;

idBool          gDisplayHeader     = ID_TRUE;
idBool          gInvalidArgs       = ID_FALSE;
SChar         * gDbf               = NULL;
smuJobType      gJob               = SMUTIL_JOB_NONE;
scPageID        gWPID              = ID_UINT_MAX;

UChar         * gWASegImagePtr     = NULL;
sdtSortSegHdr   gRawSortSegHdr;
sdtSortSegHdr * gSortSegHdr        = NULL;
sdtHashSegHdr   gRawHashSegHdr;
sdtHashSegHdr * gHashSegHdr        = NULL;
sdtWorkType     gWorkType = SDT_WORK_TYPE_NONE;

IDE_RC loadTempDump()
{
    switch( gWorkType )
    {
        case SDT_WORK_TYPE_SORT :
            IDE_TEST( sdtSortSegment::importSortSegmentFromFile( gDbf,
                                                                 &gSortSegHdr,
                                                                 &gRawSortSegHdr,
                                                                 &gWASegImagePtr )
                      != IDE_SUCCESS );
            break;
        case SDT_WORK_TYPE_HASH :
            IDE_TEST( sdtHashModule::importHashSegmentFromFile( gDbf,
                                                                &gHashSegHdr,
                                                                &gRawHashSegHdr,
                                                                &gWASegImagePtr )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_TEST( 1 );
            break;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC processHashDump()
{
    SChar  * sTempBuf = NULL;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                                 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );

    sTempBuf[0] = '\0';


    switch( gJob )
    {
        case SMUTIL_JOB_WASEGHEADER:
            sdtHashModule::dumpHashSegment( gHashSegHdr,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_WCB:
            sdtHashModule::dumpWCBs( gHashSegHdr,
                                     sTempBuf,
                                     IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_WAPAGEHEADERS:
            sdtHashModule::dumpHashPageHeaders( gHashSegHdr,
                                                sTempBuf,
                                                IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_WAPAGE:
            IDE_TEST_RAISE( gWPID >= sdtHashModule::getMaxWAPageCount( gHashSegHdr ) ,
                            err_invalid_args);

            sdtHashModule::dumpHashTempPage( gHashSegHdr,
                                             gWPID,
                                             sTempBuf,
                                             IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_EXTENTMAP:
            sdtWAExtentMgr::dumpNExtentList( gHashSegHdr->mNExtFstPIDList4Row,
                                             sTempBuf,
                                             IDE_DUMP_DEST_LIMIT );
            sdtWAExtentMgr::dumpNExtentList( gHashSegHdr->mNExtFstPIDList4SubHash,
                                             sTempBuf,
                                             IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_SORTHASHMAP:
            sdtHashModule::dumpWAHashMap( gHashSegHdr,
                                          sTempBuf,
                                          IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_MAX:
            break;
        default:
            break;
    }


    idlOS::printf( "%s", sTempBuf );

    (void) iduMemMgr::free( sTempBuf );
    sTempBuf = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_args );
    {
        (void)usage();
    }
    IDE_EXCEPTION_END;

    if( sTempBuf != NULL )
    {
        (void) iduMemMgr::free( sTempBuf );
    }

    return IDE_FAILURE;
}


IDE_RC processSortDump()
{
    sdtWAReusePolicy sReusePolicy;
    UChar  * sPagePtr;
    SChar  * sTempBuf = NULL;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                                 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );

    sTempBuf[0] = '\0';

    switch( gJob )
    {
        case SMUTIL_JOB_WASEGHEADER:
            sdtSortSegment::dumpWASegment( gSortSegHdr,
                                           sTempBuf,
                                           IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_WCB:
            sdtSortSegment::dumpWCBs( gSortSegHdr,
                                      sTempBuf,
                                      IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_WAPAGEHEADERS:
            sdtTempPage::dumpWAPageHeaders( gSortSegHdr,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_WAPAGE:
            IDE_TEST_RAISE( gWPID >= sdtSortSegment::getMaxWAPageCount( gSortSegHdr ) ,
                            err_invalid_args);

            sPagePtr = gSortSegHdr->mWCBMap[gWPID].mWAPagePtr;

            if( sPagePtr != NULL )
            {
                sReusePolicy = sdtSortSegment::getReusePolicy( gSortSegHdr,
                                                               gWPID );
                switch( sReusePolicy )
                {
                    case  SDT_WA_REUSE_INMEMORY:
                    case  SDT_WA_REUSE_LRU:
                    case  SDT_WA_REUSE_FIFO:
                        sdtTempPage::dumpTempPage( sPagePtr,
                                                   sTempBuf,
                                                   IDE_DUMP_DEST_LIMIT );
                        sdtTempRow::dumpTempPageByRow( sPagePtr,
                                                       sTempBuf,
                                                       IDE_DUMP_DEST_LIMIT );
                        break;
                    default:
                        break;
                }
            }
            break;
        case SMUTIL_JOB_EXTENTMAP:
            sdtWAExtentMgr::dumpNExtentList( gSortSegHdr->mNExtFstPIDList,
                                             sTempBuf,
                                             IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_SORTHASHMAP:
            sdtWASortMap::dumpWASortMap( &gSortSegHdr->mSortMapHdr,
                                         sTempBuf,
                                         IDE_DUMP_DEST_LIMIT );
            break;
        case SMUTIL_JOB_MAX:
        default:
            break;
    }


    idlOS::printf( "%s", sTempBuf );

    (void) iduMemMgr::free( sTempBuf );
    sTempBuf = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_args );
    {
        (void)usage();
    }
    IDE_EXCEPTION_END;

    if( sTempBuf != NULL )
    {
        (void) iduMemMgr::free( sTempBuf );
    }

    return IDE_FAILURE;
}

void usage()
{
    idlOS::printf( "\n%-6s:  dumptd {-f file} {-j jobid} {-t temp_type }[-p pid] [-s]\n"
                   "dumptd (Altibase Ver. %s) ( SM Ver. version %s )\n\n"
                   " %-4s : %s\n"
                   " %-4s : %s\n"
                   "%18"ID_UINT32_FMT" : %-15s %s\n"
                   "%18"ID_UINT32_FMT" : %-15s %s\n"
                   "%18"ID_UINT32_FMT" : %-15s %s\n"
                   "%18"ID_UINT32_FMT" : %-15s %s\n"
                   "%18"ID_UINT32_FMT" : %-15s %s\n"
                   "%18"ID_UINT32_FMT" : %-25s %s\n"
                   " %-4s : %s\n"
                   " %-4s : %s\n"
                   " %-4s : %s\n",
                   "Usage",
                   iduVersionString,
                   smVersionString,
                   "-f", "specify database file name",
                   "-j", "specify job",
                   SMUTIL_JOB_WASEGHEADER,"segment header","",
                   SMUTIL_JOB_WCB,"WCB","",
                   SMUTIL_JOB_WAPAGEHEADERS,"page headers","",
                   SMUTIL_JOB_WAPAGE,"page","-p",
                   SMUTIL_JOB_EXTENTMAP,"NExtentMap","",
                   SMUTIL_JOB_SORTHASHMAP,"Sort,HashMap","",
                   "-p", "specify page id",
                   "-s", "silent",
                   "-t", "temp table type (h:HASH, s:SORT)");
}

void parseArgs( int &aArgc, char **&aArgv )
{
    SChar sOptString[] = "f:j:sp:t:";
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
            case 's':
                gDisplayHeader = ID_FALSE;
                break;
            case 'j':
                gJob = (smuJobType)idlOS::atoi( optarg );
                break;
            case 'p':
                gWPID = idlOS::atoi( optarg );
                break;

            case 't':
                if( gWorkType != SDT_WORK_TYPE_NONE )
                {
                    gInvalidArgs = ID_TRUE;
                }

                switch( *optarg )
                {
                    case 's':
                    case 'S':
                        gWorkType = SDT_WORK_TYPE_SORT;
                        break;
                    case 'h':
                    case 'H':
                        gWorkType = SDT_WORK_TYPE_HASH;
                        break;
                    default:
                        gInvalidArgs = ID_TRUE;
                        break;
                }
                break;
            default:
                gInvalidArgs = ID_TRUE;
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
    /* -------------------------
     * [0] Property Loading
     * ------------------------*/
    IDE_TEST( idp::initialize(NULL, NULL) != IDE_SUCCESS );
    IDE_TEST( iduProperty::load() != IDE_SUCCESS);
    IDE_TEST( smuProperty::load() != IDE_SUCCESS );
    smuProperty::init4Util();

    /* -------------------------
     * [1] Init Idu module
     * ------------------------*/
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    if( IDU_USE_MEMORY_POOL == 1 )
    {
        IDE_TEST( iduMemPoolMgr::initialize() != IDE_SUCCESS );
    }
    iduLatch::initializeStatic(IDU_CLIENT_TYPE);
    IDE_TEST( iduCond::initializeStatic() != IDE_SUCCESS);

    /* -------------------------
     * [2] Message Loading
     * ------------------------*/
    IDE_TEST(smuUtility::loadErrorMsb(idp::getHomeDir(),
                                      (SChar*)"KO16KSC5601")
                   != IDE_SUCCESS );
    ideClearError();

    IDE_TEST_RAISE(iduCond::initializeStatic() != IDE_SUCCESS,
                   err_with_ide_dump );

    parseArgs( aArgc, aArgv );

    if( gDisplayHeader == ID_TRUE )
    {
        smuUtility::outputUtilityHeader("dumptb");
    }

    IDE_TEST_RAISE(gInvalidArgs == ID_TRUE, err_invalid_args);

    IDE_TEST_RAISE( gDbf == NULL,
                    err_not_specified_file );

    IDE_TEST_RAISE( idlOS::access( gDbf, F_OK ) != 0,
                    err_not_exist_file );

    IDE_TEST_RAISE( loadTempDump() != IDE_SUCCESS,
                    err_with_ide_dump );

    switch( gWorkType )
    {
        case SDT_WORK_TYPE_HASH:
            IDE_TEST_RAISE( processHashDump() != IDE_SUCCESS,
                            err_with_ide_dump );
            break;
        case SDT_WORK_TYPE_SORT:
            IDE_TEST_RAISE( processSortDump() != IDE_SUCCESS,
                            err_with_ide_dump );
            break;
        default:
            break;
    }

    iduMemMgr::free( gWASegImagePtr );

    ideDump();

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
    IDE_EXCEPTION( err_invalid_args );
    {
        (void)usage();
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


