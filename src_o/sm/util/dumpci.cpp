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
 * $Id: dumpci.cpp 37115 2009-12-04 03:06:08Z orc $
 **********************************************************************/

#include <idl.h>
#include <idn.h>
#include <ideErrorMgr.h>
#include <smp.h>
#include <smr.h>
#include <smu.h>
#include <sct.h>
#include <smmExpandChunk.h>
#include "callback.h"

/**********************************************************************
 Definitions, Global Variables
 **********************************************************************/

#define LSP "((((((((((((((("
#define RSP ")))))))))))))))"

typedef struct dumpciOption
{
    SChar     *mDBFileName;
    smOID      mOID;
    scPageID   mPID;
    scOffset   mPos;
    UInt       mSlotSize;
    SChar     *mJOB;
} dumpciOption;

dumpciOption   gOpt;
smmTBSNode   * gTBSNode = NULL;
UChar       ** gPages = NULL;
idBool         gDisplayHeader = ID_TRUE;
idBool         gInvalidArgs   = ID_FALSE;

/**********************************************************************
 Function Prototypes
 **********************************************************************/

/* init functions for util */
void showCopyRight( void );
void printUsage();

IDE_RC loadProperty();
IDE_RC loadErrorMessage();

void parseArgs( int argc, char **argv, dumpciOption * aOpt );

/* Job Functions */
IDE_RC doJOB();

IDE_RC dumpMeta();
IDE_RC dumpMemBase(smmMemBase * aMemBase);
IDE_RC dumpCatalog();
IDE_RC dumpTPL();
IDE_RC dumpPH( scPageID aPid );
IDE_RC dumpPage( scPageID aPid );

IDE_RC dumpTHWithOID();
IDE_RC dumpTCWithOID();
IDE_RC dumpTC( smcTableHeader *aTH );

IDE_RC dumpFreePageLists( smmTBSNode * aTBSNode );
IDE_RC dumpFreePageList( smmTBSNode * aTBSNode,
                          UInt         aListNo,
                          scPageID     aFirstPID,
                          vULong       aPageCount );
IDE_RC dumpFLIPages( smmTBSNode * aTBSNode );
IDE_RC dumpFLIPage( smmTBSNode * aTBSNode, vULong aChunkNo );

/* Primitive operations */
IDE_RC allocDbf( smmDatabaseFile *aDb, scPageID aPid );
IDE_RC destroyDbf( smmDatabaseFile *aDb );

IDE_RC allocPages(scPageID aPageCount );
IDE_RC destroyPages(scPageID aPageCount );

IDE_RC readPage( SInt aPid );
IDE_RC readMB( smmMemBase * aMemBase);

IDE_RC verifyDbf(smmMemBase *aMembase);

IDE_RC nextOIDall( smcTableHeader    * aCatTblHdr,
                   SChar            ** aRowPtr,
                   smpPersPage      ** aPagePtr );


const smiColumn* getColumn( smcTableHeader  * aTableHeader,
                            const UInt        aIndex );

IDE_RC getNextFreePage( smmTBSNode * aTBSNode,
                        scPageID     aFreePageID,
                        scPageID   * aNextFreePageID );

/* startup functions */
IDE_RC findTBSNode( smmTBSNode ** aTBSNode ) ;
IDE_RC initDumpOption( dumpciOption * aOpt );

IDE_RC getTBSNameFromDBF(SChar * aTBSName ) ;
IDE_RC getPingPongNoFromDBF( UInt * aPingPongNo );

IDE_RC setupMemBase(smmTBSNode * aTBSNode) ;

/**********************************************************************
 Function Implementation
 **********************************************************************/

int
main(int argc, char *argv[])
{
    scPageID sAllocPageCount;

    IDE_TEST_RAISE( initDumpOption( &gOpt ) != IDE_SUCCESS,
                    err_execution_fail );

    parseArgs( argc, argv, &gOpt );

    if(gDisplayHeader == ID_TRUE)
    {
        showCopyRight();
    }

    IDE_TEST_RAISE( gInvalidArgs == ID_TRUE,
                    err_invalid_args );

    IDE_TEST_RAISE( gOpt.mDBFileName == NULL,
                    err_not_specified_db_file );

    IDE_TEST_RAISE( idlOS::access( gOpt.mDBFileName, F_OK) != 0,
                    err_unable_to_access_db_file );

    IDE_TEST( smiSmUtilInit( &gCallBack  ) != IDE_SUCCESS );

    IDE_TEST_RAISE( findTBSNode( & gTBSNode ) != IDE_SUCCESS,
                    err_execution_fail );

    IDE_TEST_RAISE( setupMemBase( gTBSNode ) != IDE_SUCCESS,
                    err_execution_fail );

    sAllocPageCount = gTBSNode->mMemBase->mAllocPersPageCount;

    IDE_TEST_RAISE( allocPages( sAllocPageCount ) != IDE_SUCCESS,
                    err_execution_fail );

    IDE_TEST_RAISE( doJOB() != IDE_SUCCESS,
            err_execution_fail );

    IDE_TEST_RAISE( destroyPages( sAllocPageCount ) != IDE_SUCCESS,
                    err_execution_fail );

    IDE_TEST( smiSmUtilShutdown( NULL ) != IDE_SUCCESS );

    idlOS::printf("End Of Dump.\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_args);
    {
        idlOS::printf( "\nInvalid Option\n\n" );
        printUsage();
    }
    IDE_EXCEPTION(err_execution_fail);
    {
        smuUtility::outputMsg( ideGetErrorMsg(ideGetErrorCode()) );
    }
    IDE_EXCEPTION(err_unable_to_access_db_file) ;
    {
        idlOS::printf( "\nUnable to access DB File specified by -f option\n" );
    }
    IDE_EXCEPTION(err_not_specified_db_file);
    {
        idlOS::printf( "\nPlease specify a DB File\n" );

        printUsage();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
     Membase를 설정한다.
 */
IDE_RC setupMemBase(smmTBSNode * aTBSNode)
{
    static ULong sPage0Buffer[ SM_PAGE_SIZE / ID_SIZEOF(ULong) ];

    smmDatabaseFile * sDBFile;

    UInt  sPingPongNo;

    IDE_TEST( getPingPongNoFromDBF( & sPingPongNo ) != IDE_SUCCESS );

    IDE_TEST( smmManager::getDBFile( aTBSNode,
                                     sPingPongNo,
                                     0,  //aDBFileNo
                                     SMM_GETDBFILEOP_SEARCH_FILE,
                                     & sDBFile )
              != IDE_SUCCESS );

    if (sDBFile->isOpen() != ID_TRUE )
    {
        IDE_TEST(sDBFile->open() != IDE_SUCCESS);
    }

    IDE_TEST(sDBFile->readPage( aTBSNode,
                                SMM_MEMBASE_PAGEID,
                                (UChar *)sPage0Buffer )
             != IDE_SUCCESS);

    IDE_TEST( smmManager::setupBasePageInfo( aTBSNode,
                                             (UChar *)sPage0Buffer )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Parse DB File Name String
    DB File 이름의  Format : "TableSpaceName-PingPong-FileNo"

    [IN] aDBFileName - Parsing할 DB File의 이름

    [OUT] aTBSName - DB File의 이름
    [OUT] aPingPongNo - PingPong 번호 0 or 1
    [OUT] aFileNo     - DB File의 번호
 */
IDE_RC parseDBFileName(SChar * aDBFileName,
                       SChar * aTBSName,
                       UInt  * aPingPongNo,
                       UInt  * aFileNo )
{
    SChar * sTempChar;

    IDE_ASSERT( aDBFileName != NULL );
    IDE_ASSERT( aTBSName    != NULL );
    IDE_ASSERT( aPingPongNo != NULL );
    IDE_ASSERT( aFileNo     != NULL );


    SChar sFileName[ SM_MAX_FILE_NAME+1 ];


    idlOS::strcpy( sFileName, aDBFileName );

    idlOS::strcpy( aTBSName, idlOS::strtok( sFileName, "-") );

    sTempChar = idlOS::strtok( NULL, "-");
    IDE_ASSERT( sTempChar != NULL );
    *aPingPongNo = idlOS::atoi( sTempChar );

    sTempChar = idlOS::strtok( NULL, "-");
    IDE_ASSERT( sTempChar != NULL );
    *aFileNo     = idlOS::atoi( sTempChar );

    return IDE_SUCCESS;
}

/*
    사용자가 지정한 DB File 이름으로부터 Tablespace이름을 얻어낸다.

    [OUT] aTBSName - Tablespace 이름
 */
IDE_RC getTBSNameFromDBF(SChar * aTBSName )
{
    UInt         sPingPong;
    UInt         sFileNo;

    IDE_TEST( parseDBFileName( gOpt.mDBFileName, aTBSName, & sPingPong, & sFileNo  )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC getPingPongNoFromDBF( UInt * aPingPongNo )
{
    IDE_ASSERT( aPingPongNo != NULL );

    SChar        sTBSName[ SM_MAX_FILE_NAME+1 ];
    UInt         sPingPongNo;
    UInt         sFileNo;

    IDE_TEST( parseDBFileName( gOpt.mDBFileName,
                               sTBSName,
                               & sPingPongNo,
                               & sFileNo  )
              != IDE_SUCCESS );

    *aPingPongNo = sPingPongNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




IDE_RC findTBSNode( smmTBSNode ** aTBSNode )
{
    IDE_ASSERT( aTBSNode != NULL );

    smmTBSNode * sTBSNode;
    SChar        sTBSName[ SM_MAX_FILE_NAME+1 ];

    IDE_TEST(getTBSNameFromDBF( sTBSName )   != IDE_SUCCESS );

    IDE_TEST(sctTableSpaceMgr::findSpaceNodeByName(
                 sTBSName,
                 (void**)& sTBSNode,
                 ID_FALSE ) != IDE_SUCCESS );

    *aTBSNode = sTBSNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Dictionary Tablespace가 아니면 에러를 발생시킨다.
 */
IDE_RC checkDicTBS()
{
    SChar  sTBSName[ SM_MAX_FILE_NAME+1 ];

    IDE_TEST(getTBSNameFromDBF( sTBSName ) != IDE_SUCCESS );

    if ( idlOS::strCaselessMatch( sTBSName,
                                  SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DIC )
         != 0 )
    {
        IDE_RAISE( err_tablespace_is_not_dictionary_tablespace );
    }

    return IDE_SUCCESS ;

    IDE_EXCEPTION(err_tablespace_is_not_dictionary_tablespace );
    {
       idlOS::printf( "The operation is allowed only on dictionary tablespace (%s). \n",
                      SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DIC );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}



IDE_RC doJOB()
{
    idBool      sDoJob;

    IDE_TEST_RAISE( gOpt.mJOB == NULL, invalid_job);

    sDoJob = ID_FALSE;

    if( idlOS::strcmp( "meta", gOpt.mJOB ) == 0  )
    {
        IDE_TEST( dumpMeta() != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }
    if( idlOS::strcmp( "membase", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpMemBase( gTBSNode->mMemBase ) != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }
    if( idlOS::strcmp( "catalog", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpCatalog() != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }
    if( idlOS::strcmp( "table-header", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpTHWithOID() != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }
    if( idlOS::strcmp( "table-columns", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpTCWithOID() != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }
    if( idlOS::strcmp( "table-page-list", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpTPL() != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }
    if( idlOS::strcmp( "page", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpPage( gOpt.mPID ) != IDE_SUCCESS );
        sDoJob = ID_TRUE;

    }
    if( idlOS::strcmp( "free-page-lists", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpFreePageLists( gTBSNode ) != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }
    if( idlOS::strcmp( "FLI-pages", gOpt.mJOB ) == 0 )
    {
        IDE_TEST( dumpFLIPages( gTBSNode ) != IDE_SUCCESS );
        sDoJob = ID_TRUE;
    }

    IDE_TEST_RAISE( sDoJob == ID_FALSE, invalid_job);

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_job );
    {
        idlOS::printf( "\nPlease insert job type. (ex: -j membase )\n" );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* Membase를 Dump */
IDE_RC dumpMemBase(smmMemBase * aMemBase)
{
    UInt i;

    idlOS::printf( "%s %s %s\n\n",
                   LSP,
                   "DUMP MEMBASE",
                   RSP );

    idlOS::printf( "%-31s: %s\n",
                   "DBNAME",
                   aMemBase->mDBname );
    idlOS::printf( "%-31s: %s\n",
                   "PRODUCT_SIGNATURE",
                   aMemBase->mProductSignature );
    idlOS::printf( "%-31s: %s\n",
                   "DATAFILE_SIGNATURE",
                   aMemBase->mDBFileSignature );
    idlOS::printf( "%-31s: 0x%x\n",
                   "VERSION_ID",
                   aMemBase->mVersionID );
    idlOS::printf( "%-31s: %"ID_UINT32_FMT"\n",
                   "COMPILE_BIT",
                   aMemBase->mCompileBit );
    idlOS::printf( "%-31s: %"ID_UINT32_FMT"\n",
                   "BIG_ENDIAN",
                   aMemBase->mBigEndian );
    idlOS::printf( "%-31s: %"ID_vULONG_FMT"\n",
                   "LOG_SIZE",
                   aMemBase->mLogSize );
    idlOS::printf( "%-31s: %"ID_vULONG_FMT"\n",
                   "DATAFILE_PAGE_COUNT",
                   aMemBase->mDBFilePageCount );
    idlOS::printf( "%-31s: %"ID_UINT32_FMT"\n",
                   "TX_TABLE_SIZE",
                   aMemBase->mTxTBLSize );
    idlOS::printf( "%-31s: F: %"ID_UINT32_FMT", S: %"ID_UINT32_FMT"\n",
                   "DATAFILE_COUNT",
                   aMemBase->mDBFileCount[0],
                   aMemBase->mDBFileCount[1] );
    idlOS::printf( "%-31s: %"ID_vULONG_FMT"\n",
                   "ALLOC_PERS_PAGE_COUNT",
                   aMemBase->mAllocPersPageCount );
    idlOS::printf( "%-31s: %"ID_vULONG_FMT"\n", 
                   "EXPAND_CHUNK_PAGE_COUNT",
                   aMemBase->mExpandChunkPageCnt );
    idlOS::printf( "%-31s: %"ID_vULONG_FMT"\n",
                   "CURRENT_EXPAND_CHUNK_PAGE_COUNT",
                   aMemBase->mCurrentExpandChunkCnt );

    // 각각의 Free Page List의 정보를 출력한다.
    for ( i=0; i< aMemBase->mFreePageListCount; i++ )
    {
        idlOS::printf( "LIST#%02"ID_UINT32_FMT".%-23s: %"ID_vULONG_FMT"\n",
                       i,
                       "FREE_PERS_PAGE_COUNT",
                       aMemBase->mFreePageLists[i].mFreePageCount );

        idlOS::printf( "LIST#%02"ID_UINT32_FMT".%-23s: %"ID_vULONG_FMT"\n",
                       i,
                       "FIRST_FREE_PERS_PAGE",
                       aMemBase->mFreePageLists[i].mFirstFreePageID );
    }



    idlOS::printf( "%-31s: %"ID_UINT64_FMT"\n",
                   "SYSTEM_SCN",
                   SM_SCN_TO_LONG( aMemBase->mSystemSCN ) );

    // PROJ-1579 NCHAR
    idlOS::printf( "%-31s: %s\n",
                   "NLS_CHARACTERSET",
                   aMemBase->mDBCharSet);

    // PROJ-1579 NCHAR
    idlOS::printf( "%-31s: %s\n",
                   "NLS_NCHAR_CHARACTERSET",
                   aMemBase->mNationalCharSet);

    return IDE_SUCCESS;

}

/* Catalog Table의 slot들 (내부 페이지) Dump */
IDE_RC dumpCatalog()
{
    smpSlotHeader  * sSlotHeaderPtr;
    smcTableHeader * sCatalogHeader;
    smcTableHeader * sHeader;
    SChar          * sRowPtr;
    smpPersPage    * sPagePtr;
    scPageID         sPid = 0;
    SChar          * sTempBuffer;
    UInt             sState = 0;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMC,
                           1,
                           ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                           (void**)&sTempBuffer )
              == IDE_FAILURE );
    sState = 1;


    /* Dictionary Tablespace에서만 Dump 가능 */
    IDE_TEST( checkDicTBS() != IDE_SUCCESS );

    /* CatalogTable은 반드시 0번 페이지에 위치함 */
    IDE_TEST( readPage( sPid ) != IDE_SUCCESS );

    idlOS::printf( "%s %s %s\n\n",
                   LSP,
                   "DUMP CATALOG",
                   RSP );

    sCatalogHeader = (smcTableHeader*)(gPages[sPid] +
                                       SMM_CAT_TABLE_OFFSET +
                                       SMP_SLOT_HEADER_SIZE );

    /* Catalog Table에 대한 Loop 시작 */
    sRowPtr  = NULL;
    sPagePtr = NULL;

    /* [SlotPID ~ Flag]     come from smpFixedPageList::dumpSlotHeaderByBuffer
     * [TableType ~ MaxRow] come from smcTable::dumpTableHeaderByBuffer */
    idlOS::printf(
"SlotPID     CreateSCN       CreateTID     LimitSCN            LimitTID"
"       NextOID  Offset   Flag      "
"Type           Flag     TableType     ObjType  SelfOID     NullOID  "
"ColSize   ColCnt   SID MaxRow\n");

    while(1)
    {
        IDE_TEST( nextOIDall( sCatalogHeader,
                              &sRowPtr,
                              &sPagePtr )
                  != IDE_SUCCESS );

        if( sRowPtr == NULL )
        {
            break;
        }

        sSlotHeaderPtr = (smpSlotHeader *)sRowPtr;
        if( SMP_SLOT_IS_NOT_USED( sSlotHeaderPtr ) ||
            SMP_SLOT_IS_DROP( sSlotHeaderPtr ) )
        {
            continue;
        }

        sHeader = (smcTableHeader *)(sSlotHeaderPtr + 1);

        sTempBuffer[0] = '\0';
        smpFixedPageList::dumpSlotHeaderByBuffer( sSlotHeaderPtr,
                                                  ID_TRUE, // display table
                                                  sTempBuffer,
                                                  IDE_DUMP_DEST_LIMIT );

        smcTable::dumpTableHeaderByBuffer( sHeader,
                                           ID_FALSE,  // dump binary
                                           ID_TRUE,   // display table
                                           sTempBuffer,
                                           IDE_DUMP_DEST_LIMIT );

        idlOS::printf("%s\n",sTempBuffer);
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuffer ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( sTempBuffer ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/* Table의 PageList를 Loop돌며 PageHeader를 출력한다. */
IDE_RC dumpPH( scPageID aPid )
{
    scPageID sPid = aPid;

    while( sPid != SM_NULL_PID )
    {
        IDE_TEST( readPage( sPid ) != IDE_SUCCESS )

            idlOS::printf( "%-7"ID_UINT32_FMT" "
                           "%-7"ID_UINT32_FMT" "
                           "%-7"ID_UINT32_FMT" "
                           "%-7"ID_UINT32_FMT"\n",
                           ((smpPersPageHeader *)gPages[sPid])->mSelfPageID,
                           ((smpPersPageHeader *)gPages[sPid])->mType,
                           ((smpPersPageHeader *)gPages[sPid])->mPrevPageID,
                           ((smpPersPageHeader *)gPages[sPid])->mNextPageID);

        sPid = ((smpPersPageHeader*)gPages[sPid])->mNextPageID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC dumpPage( scPageID aPid )
{
    scPageID   sPid = aPid;
    SChar    * sTempBuf;
    UInt       sState = 0;

    IDE_TEST( readPage( sPid ) != IDE_SUCCESS );

    /* 해당 메모리 영역을 Dump한 결과값을 저장할 버퍼를 확보합니다.
     * Stack에 선언할 경우, 이 함수를 통해 서버가 종료될 수 있으므로
     * Heap에 할당을 시도한 후, 성공하면 기록, 성공하지 않으면 그냥
     * return합니다. */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 1;


    /* Page Type (Fixed / Variable) 에 따라 달리 Dump합니다. */
    sTempBuf[0] = '\0';
    if( ((smpPersPageHeader *)gPages[sPid])->mType == SMP_PAGETYPE_FIX )
    {
        smpFixedPageList::dumpFixedPageByBuffer( gPages[sPid],
                                                 gOpt.mSlotSize,
                                                 sTempBuf,
                                                 IDE_DUMP_DEST_LIMIT );
        idlOS::printf("%s\n", sTempBuf );
    }
    else
    {
        if( ((smpPersPageHeader *)gPages[sPid])->mType == SMP_PAGETYPE_VAR )
        {
            smpVarPageList::dumpVarPageByBuffer( gPages[sPid],
                                                 sTempBuf,
                                                 IDE_DUMP_DEST_LIMIT );

            idlOS::printf("%s\n", sTempBuf );
        }
        else
        {
            /* 어느 Type도 아니라면, 초기화 안된 페이지 입니다. */
            ideLog::ideMemToHexStr( gPages[sPid],
                                    SM_PAGE_SIZE,
                                    IDE_DUMP_FORMAT_NORMAL,
                                    sTempBuf,
                                    IDE_DUMP_DEST_LIMIT );

            idlOS::printf( "Unformated page\n%s\n", sTempBuf );
        }
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
       IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* 사용자가 지정한 OID로 Table Header를 출력한다 */
IDE_RC dumpTHWithOID()
{
    SChar          * sTempBuffer;
    UInt             sState = 0;
    smcTableHeader * sHeader;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMC,
                           1,
                           ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                           (void**)&sTempBuffer )
              == IDE_FAILURE );
    sState = 1;

    gOpt.mPID = SM_MAKE_PID( gOpt.mOID );
    gOpt.mPos = SM_MAKE_OFFSET( gOpt.mOID );

    IDE_TEST( readPage( gOpt.mPID ) != IDE_SUCCESS );

    sHeader = (smcTableHeader *)
        &( gPages[gOpt.mPID][gOpt.mPos + SMP_SLOT_HEADER_SIZE]);

    sTempBuffer[0] = '\0';
    smcTable::dumpTableHeaderByBuffer( sHeader,
                                       ID_TRUE,   // dump binary
                                       ID_FALSE,  // display table
                                       sTempBuffer,
                                       IDE_DUMP_DEST_LIMIT );
    idlOS::printf("%s\n",sTempBuffer);

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuffer ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( sTempBuffer ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*
 * BUG-30713: Table Column 정보를 출력하는 기능을 추가한다.
 */
IDE_RC dumpTCWithOID()
{

    gOpt.mPID = SM_MAKE_PID( gOpt.mOID );
    gOpt.mPos = SM_MAKE_OFFSET( gOpt.mOID );

    IDE_TEST( readPage( gOpt.mPID ) != IDE_SUCCESS );

    IDE_TEST( dumpTC( (smcTableHeader *)
                      &(gPages[gOpt.mPID]
                        [gOpt.mPos + SMP_SLOT_HEADER_SIZE]) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dumpTC( smcTableHeader *aTH )
{
    const smiColumn *sColumn;
    UInt            i;


    idlOS::printf( "%s %s %s\n\n",
                   LSP,
                   "DUMP TABLE COLUMNS",
                   RSP );

    idlOS::printf( "# %s : %"ID_vULONG_FMT"\n",
                   "OID",
                   gOpt.mOID );

    idlOS::printf( "%-13s : %"ID_UINT32_FMT"\n",
                   "COLUMN_COUNT",
                   aTH->mColumnCount );

    for( i = 0; i < aTH->mColumnCount; i++ )
    {
        sColumn = getColumn( aTH, i );
        idlOS::printf( "COLUMN(%2"ID_UINT32_FMT")\n", i );
        idlOS::printf( "    %-15s : %"ID_UINT32_FMT"\n",
                       "* id",
                       sColumn->id );
        idlOS::printf( "    %-15s : %"ID_UINT32_FMT"\n",
                       "* flag",
                       sColumn->flag );
        idlOS::printf( "    %-15s : %"ID_UINT32_FMT"\n",
                       "* offset",
                       sColumn->offset );
        idlOS::printf( "    %-15s : %"ID_UINT32_FMT"\n",
                       "* InOutBaseSize",
                       sColumn->vcInOutBaseSize );
        idlOS::printf( "    %-15s : %"ID_UINT32_FMT"\n",
                       "* size",
                       sColumn->size );
        idlOS::printf( "    %-15s : %"ID_UINT32_FMT"\n",
                       "* colSpace",
                       sColumn->colSpace );
        idlOS::printf( "    %-15s : "
                       "SpaceID(%"ID_UINT32_FMT"), "
                       "Offset(%"ID_UINT32_FMT"), "
                       "PageID(%"ID_UINT32_FMT")\n",
                       "* colSeg",
                       sColumn->colSeg.mSpaceID,
                       sColumn->colSeg.mOffset,
                       sColumn->colSeg.mPageID );
    }

    return IDE_SUCCESS;
}

IDE_RC allocDbf( smmDatabaseFile *aDb, scPageID aPid )
{
    SChar    sDbf[1024*2];
    SChar    *sNoF = NULL;
    SChar    *sNoS = NULL;
    SInt     sFno;

    idlOS::memset( sDbf, 0, ID_SIZEOF(sDbf) );
    idlOS::memcpy( sDbf, gOpt.mDBFileName, strlen( gOpt.mDBFileName ) );

    sNoF = idlOS::strstr( sDbf, "-" );

    IDE_TEST_RAISE( sNoF == NULL, strstr_error );

    *sNoF = '\0';
    sNoF++;
    sNoS = idlOS::strstr( sNoF, "-" );

    IDE_TEST_RAISE( sNoS == NULL, strstr_error );

    sNoS++;

    sFno = smmManager::getDbFileNo( gTBSNode, aPid );

    /* BUG-15648: 파일이 여러개 있을때 dumpci의 Page-Header-Dump가
     * 오동작합니다.*/
    idlVA::appendFormat( sDbf, 1024 * 2, "-%d-%d",
                         *sNoS - '0',
                         sFno);

    IDE_TEST( aDb->initialize( gTBSNode->mHeader.mID,
                               (*sNoS - '0'),
                               sFno ) != IDE_SUCCESS );

    IDE_TEST( aDb->setFileName( sDbf ) != IDE_SUCCESS );

    IDE_TEST( aDb->open() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( strstr_error );
    {
        idlOS::printf( "\nstrstr error\n\n" );
    }
    IDE_EXCEPTION_END;

    idlOS::printf( "\nallocDbf error\n\n" );

    return IDE_FAILURE;
}

IDE_RC destroyDbf( smmDatabaseFile *aDb )
{
    IDE_TEST( aDb->destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    idlOS::printf( "\ndestroyDbf error\n\n" );

    return IDE_FAILURE;
}

IDE_RC readPage( SInt aPid )
{
    SInt            sPid = aPid;
    smmDatabaseFile sDb;

    if(  gPages[aPid] == NULL )
    {
        gPages[aPid] = (UChar *)idlOS::malloc( SM_PAGE_SIZE );

        IDE_TEST_RAISE( gPages[aPid] == NULL, malloc_error );

        IDE_TEST( allocDbf( &sDb, sPid ) != IDE_SUCCESS );

        IDE_TEST( sDb.readPageWithoutCheck( gTBSNode,
                                            sPid,
                                            gPages[sPid] ) != IDE_SUCCESS );

        IDE_TEST( destroyDbf( &sDb ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( malloc_error );
    {
        idlOS::printf( "\nmalloc error\n\n" );
    }
    IDE_EXCEPTION_END;

    idlOS::printf( "\nreadPage error\n\n" );

    return IDE_FAILURE;
}

IDE_RC readMB( smmMemBase * aMemBase )
{
    scPageID         sPid = 0;
    smmDatabaseFile  sDb;
    UChar            sPage[SM_PAGE_SIZE];
    smmMemBase      *sMemBasePtr;

    IDE_TEST_RAISE( allocDbf( &sDb, sPid ) != IDE_SUCCESS, err_execute_fail );

    IDE_TEST_RAISE( sDb.readPage( gTBSNode, sPid,  sPage ) != IDE_SUCCESS, err_execute_fail );

    sMemBasePtr = ( smmMemBase * )( sPage + SMM_MEMBASE_OFFSET );

    IDE_TEST( verifyDbf( sMemBasePtr ) != IDE_SUCCESS );

    IDE_TEST_RAISE( destroyDbf( &sDb ) != IDE_SUCCESS, err_execute_fail );

    *aMemBase = *sMemBasePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_execute_fail );
    {
        idlOS::printf( "\nreadMB error\n\n" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC nextOIDall( smcTableHeader    * aCatTblHdr,
                   SChar            ** aRowPtr,
                   smpPersPage      ** aPagePtr )
{
    scPageID                sCurPID;
    SChar                 * sRowPtr;
    smpPersPage           * sPagePtr;
    SChar                 * sFence;
    smpAllocPageListEntry * sAllocPageListEntry;
    UInt                    sSlotSize;

    sAllocPageListEntry = &aCatTblHdr->mFixedAllocList.mMRDB[0];
    sSlotSize           = aCatTblHdr->mFixed.mMRDB.mSlotSize;
    sRowPtr             = *aRowPtr;
    sPagePtr            = *aPagePtr;

    while( 1 )
    {
        /* Fetch next */
        if( sRowPtr == NULL )
        {
            /* First scan or Read next page */
            sCurPID = smpAllocPageList::getNextAllocPageID(
                sAllocPageListEntry,
                (smpPersPageHeader*)sPagePtr);

            /* No page */
            if( sCurPID == SM_NULL_PID )
            {
                break;
            }

            IDE_TEST( readPage( sCurPID ) != IDE_SUCCESS );
            sPagePtr = (smpPersPage *) gPages[ sCurPID ];
            sRowPtr = (SChar *)(sPagePtr->mBody);
        }
        else
        {
            sRowPtr += sSlotSize;
        }

        /* Fence Check */
        sFence = (SChar *)(sPagePtr + 1);
        if( ( sRowPtr + sSlotSize ) > sFence )
        {
            /* 페이지내 모든 슬롯을 읽음 */
            sRowPtr = NULL;
        }
        else
        {
            break;
        }
    }

    *aRowPtr  = sRowPtr;
    *aPagePtr = sPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC allocPages(scPageID aPageCount)
{
    UInt y;

    gPages = (UChar **)idlOS::malloc( (size_t)ID_SIZEOF(UChar *) * aPageCount );
    IDE_TEST_RAISE( gPages == NULL, malloc_error );

    for( y = 0; y < aPageCount; y++ )
    {
        gPages[y] = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( malloc_error )
    {
        idlOS::printf( "\nmalloc error\n\n" );
    }
    IDE_EXCEPTION_END;

    idlOS::printf( "\nallocPages error\n\n" );

    return IDE_FAILURE;
}

IDE_RC destroyPages( scPageID aPageCount )
{
    UInt y;

    for( y = 0; y < aPageCount; y++ )
    {
        if( gPages[y] != NULL )
        {
            idlOS::free( gPages[y] );
            gPages[y] = NULL;
        }
    }

    idlOS::free( gPages );

    return IDE_SUCCESS;
}

void printUsage()
{
    idlOS::printf("\n%-6s:  dumpci  {-f file} [-m] [-j job] [-o oid] [-p pid] [-s slot_size]\n\n", "Usage" );

    idlOS::printf( "\ndumpci (Altibase Ver. %s) ( SM Ver. version %s )\n\n",
                   iduVersionString,
                   smVersionString );
    idlOS::printf(" %-4s : %s\n", "-f", "specify database file name" );
    idlOS::printf(" %-4s : %s\n", "-m", "display database file header" );
    idlOS::printf(" %-4s : %s\n", "-j", "specify job" );
    idlOS::printf("\t%-18s : %s\n", "a. membase", "dump membase" );
    idlOS::printf("\t%-18s : %s\n", "b. catalog", "dump catalog" );
    idlOS::printf("\t%-18s : %s\n", "c. table-header", "dump table header" );
    idlOS::printf("\t%-18s : %s\n", "d. table-columns", "dump table columns" );
    idlOS::printf("\t%-18s : %s\n", "e. table-page-list", "dump table page list" );
    idlOS::printf("\t%-18s : %s\n", "f. page", "dump page" );
    idlOS::printf("\t%-18s : %s\n", "g. free-page-lists", "dump free page lists" );
    idlOS::printf("\t%-18s : %s\n", "h. FLI-pages", "dump FLI page" );
    idlOS::printf(" %-4s : %s\n", "-o", "specify oid" );
    idlOS::printf(" %-4s : %s\n", "-p", "specify page id" );
    idlOS::printf(" %-4s : %s\n", "-s", "specify slot size" );
    idlOS::printf("\n" );

    idlOS::printf("%-4s\n", "example)" );
    idlOS::printf("%-4s\n", "a. dumpci -f SYS_TBS_MEM_DIC-0-0 -m" );
    idlOS::printf("%-4s\n", "b. dumpci -f SYS_TBS_MEM_DIC-0-0 -j membase" );
    idlOS::printf("%-4s\n", "c. dumpci -f SYS_TBS_MEM_DIC-0-0 -j catalog" );
    idlOS::printf("%-4s\n", "d. dumpci -f SYS_TBS_MEM_DIC-0-0 -j table-header    -o 32784" );
    idlOS::printf("%-4s\n", "e. dumpci -f SYS_TBS_MEM_DIC-0-0 -j table-columns   -o 32784" );
    idlOS::printf("%-4s\n", "f. dumpci -f SYS_TBS_MEM_DIC-0-0 -j table-page-list -o 32784" );
    idlOS::printf("%-4s\n", "g. dumpci -f SYS_TBS_MEM_DIC-0-0 -j page -p 3" );
    idlOS::printf("%-4s\n", "h. dumpci -f SYS_TBS_MEM_DIC-0-0 -j free-page-lists" );
    idlOS::printf("%-4s\n", "i. dumpci -f SYS_TBS_MEM_DIC-0-0 -j FLI-pages" );
    idlOS::printf("%-4s\n", "k. dumpci -h" );
    idlOS::printf("\n" );
}

IDE_RC loadProperty()
{
    IDE_TEST(iduProperty::load() != IDE_SUCCESS);

    IDE_TEST( smuProperty::load()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC loadErrorMessage()
{

    /* --------------------
     *  에러 Message 로딩
     * -------------------*/
    IDE_TEST_RAISE(smuUtility::loadErrorMsb(idp::getHomeDir(),
                                            (SChar*)"KO16KSC5601")
                   != IDE_SUCCESS, load_error_msb_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(load_error_msb_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't Load Error Files. \n");
        smuUtility::outputMsg("Check Directory in $ALTIBASE_HOME"IDL_FILE_SEPARATORS"msg. \n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC initDumpOption( dumpciOption * aOpt )
{
    aOpt->mDBFileName = NULL;
    aOpt->mOID        = SM_NULL_OID;
    aOpt->mPID        = SM_NULL_PID;
    aOpt->mPos        = 0;
    aOpt->mSlotSize   = 0;
    aOpt->mJOB        = NULL;

    return IDE_SUCCESS;
}

void parseArgs( int argc, char **argv, dumpciOption * aOpt )
{
    SInt opr;

    opr = idlOS::getopt(argc, argv, "mf:j:o:p:xs:" );

    if( opr != EOF )
    {
        do
        {
            switch( opr )
            {
            case 'f':
                aOpt->mDBFileName = optarg;
                break;
            case 'j':
                aOpt->mJOB = optarg;
                break;
            case 'o':
                aOpt->mOID = idlOS::atoi( optarg );
                break;
            case 'p':
                aOpt->mPID = idlOS::atoi( optarg );
                break;
            case 'm':
                aOpt->mJOB = (SChar*)"meta";
                break;
            case 'x':
                gDisplayHeader = ID_FALSE;
                break;
            case 's':
                aOpt->mSlotSize = idlOS::atoi( optarg );
                break;
            default:
                gInvalidArgs = ID_TRUE;
                break;
            }
        }
        while ( (opr = idlOS::getopt(argc, argv, "mf:j:o:p:xs:"))
                != EOF) ;
    }
    else
    {
        gInvalidArgs = ID_TRUE;
    }
}

IDE_RC
verifyDbf(smmMemBase *aMembase)
{
    UInt s_DbVersion;
    UInt s_SrcVersion;
    UInt sCheckVersion;
    UInt sCheckBitMode;
    UInt sCheckEndian;
    UInt sCheckLogSize;
    UInt sLogFileSize;
    UInt sTxTableSize;

    /* ------------------------------------------------
     * [1] Product Version Check
     * ----------------------------------------------*/
    sCheckVersion = smuProperty::getCheckStartupVersion();

    if (sCheckVersion != 0)
    {
        s_DbVersion   = aMembase->mVersionID & SM_CHECK_VERSION_MASK;
        s_SrcVersion  = smVersionID & SM_CHECK_VERSION_MASK;

        IDE_TEST_RAISE (s_DbVersion != s_SrcVersion, version_mismatch_error);
    }

    /* ------------------------------------------------
     * [2] Bit Mode Check 32/64
     * ----------------------------------------------*/
    sCheckBitMode = smuProperty::getCheckStartupBitMode();

    if (sCheckBitMode != 0)
    {
        IDE_TEST_RAISE(aMembase->mCompileBit != iduCompileBit,
                       version_mismatch_error);
    }

    /* ------------------------------------------------
     * [3] Endian Check
     * ----------------------------------------------*/
    sCheckEndian = smuProperty::getCheckStartupEndian();

    if (sCheckEndian != 0)
    {
        IDE_TEST_RAISE(aMembase->mBigEndian != iduBigEndian,
                       version_mismatch_error);
    }

    /* ------------------------------------------------
     * [4] Log Size Check
     * ----------------------------------------------*/
    sCheckLogSize = smuProperty::getCheckStartupLogSize();

    if (sCheckLogSize != 0)
    {
        sLogFileSize = (UInt)smuProperty::getLogFileSize();

        IDE_TEST_RAISE(aMembase->mLogSize != sLogFileSize,
                       version_mismatch_error);
    }

    /* ------------------------------------------------
     * [5] DB File Size Check
     * ----------------------------------------------*/

    /* ------------------------------------------------
     * [5] Transaction Table Size
     * ----------------------------------------------*/
    sTxTableSize = smuProperty::getTransTblSize();

    IDE_TEST_RAISE(aMembase->mTxTBLSize != sTxTableSize,
                   version_mismatch_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(version_mismatch_error);
    {
       idlOS::printf( "\nThe %s is not compatible with the dumpci\n\n" , gOpt.mDBFileName );
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

/* DataFile의 Meta정보(Checkpoint imgage hdr)를 dump */
IDE_RC dumpMeta()
{
    UInt                sState = 0;
    iduFile             sDataFile;
    smmChkptImageHdr  * sDfHdr;
    ULong               sPage[SD_PAGE_SIZE/ID_SIZEOF(ULong)] = {0,};

    IDE_TEST( sDataFile.initialize( IDU_MEM_SM_SMU,
                                    1, /* Max Open FD Count */
                                    IDU_FIO_STAT_OFF,
                                    IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sDataFile.setFileName(gOpt.mDBFileName) != IDE_SUCCESS );
    IDE_TEST( sDataFile.open() != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sDataFile.read( NULL,
                              0,
                              (void*)sPage,
                              SM_DBFILE_METAHDR_PAGE_SIZE,
                              NULL ) != IDE_SUCCESS );

    sDfHdr = (smmChkptImageHdr*)sPage;

    idlOS::printf("[BEGIN CHECKPOINT IMAGE HEADER]\n\n");
    idlOS::printf("Binary DB Version             [ %"ID_xINT32_FMT".%"ID_xINT32_FMT".%"ID_xINT32_FMT" ]\n",
                   ((sDfHdr->mSmVersion & SM_MAJOR_VERSION_MASK) >> 24),
                   ((sDfHdr->mSmVersion & SM_MINOR_VERSION_MASK) >> 16),
                   (sDfHdr->mSmVersion  & SM_PATCH_VERSION_MASK));

    idlOS::printf("Redo LSN       [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                  sDfHdr->mMemRedoLSN.mFileNo,
                  sDfHdr->mMemRedoLSN.mOffset);

    idlOS::printf("Create LSN     [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                  sDfHdr->mMemCreateLSN.mFileNo,
                  sDfHdr->mMemCreateLSN.mOffset);
    idlOS::printf("\n");

    idlOS::printf("[END CHECKPOINT IMAGE HEADER]\n");

    /*
     *BUG-31741 Ignored return value(codesonar warning)
     */
    sState = 1;
    IDE_TEST( sDataFile.close() != IDE_SUCCESS);
    sState = 0;
    IDE_TEST( sDataFile.destroy() != IDE_SUCCESS);

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

/* Table의 Page List를 출력한다 */
IDE_RC dumpTPL()
{
    UInt y;
    smcTableHeader * sTH;

    idlOS::printf( "%s %s %s\n\n",
                   LSP,
                   "DUMP TABLE PAGE LIST",
                   RSP );

    gOpt.mPID = SM_MAKE_PID( gOpt.mOID );
    gOpt.mPos = SM_MAKE_OFFSET( gOpt.mOID );

    IDE_TEST( readPage( gOpt.mPID ) != IDE_SUCCESS );

    idlOS::printf( "# OID : %"ID_vULONG_FMT"\n", gOpt.mOID );

    sTH = (smcTableHeader *)&(gPages[gOpt.mPID][gOpt.mPos + SMP_SLOT_HEADER_SIZE]);
    if( ( sTH->mType == SMC_TABLE_NORMAL ) &&
        ( SMI_TABLE_TYPE_IS_DISK( sTH ) ==ID_TRUE ) )
    {
        IDE_RAISE(err_disk_or_temp_tbs);
    }

    for( y = 0; y < SMP_PAGE_LIST_COUNT; y++ )
    {
        if( sTH->mFixedAllocList.mMRDB[y].mPageCount > 0 )
        {
            idlOS::printf(
                "[Fixed Alloc Page List %3d]\t"
                "SlotSize:%d SlotCount:%d\n",
                y,
                sTH->mFixed.mMRDB.mSlotSize,
                sTH->mFixed.mMRDB.mSlotCount );
            idlOS::printf( "%-7s %-7s %-7s %-7s %-7s\n",
                           "SELF",
                           "TYPE",
                           "PREV",
                           "NEXT",
                           "FLAG" );
            IDE_TEST( dumpPH( smpAllocPageList::getFirstAllocPageID(
                              &sTH->mFixedAllocList.mMRDB[y]) )
                != IDE_SUCCESS );
        }
    }
    for( y = 0; y < SMP_PAGE_LIST_COUNT; y++ )
    {
        if( sTH->mVarAllocList.mMRDB[y].mPageCount > 0 )
        {
            idlOS::printf("[Variable Alloc Page List %3d]\n",y);
            idlOS::printf( "%-7s %-7s %-7s %-7s %-7s\n",
                           "SELF",
                           "TYPE",
                           "PREV",
                           "NEXT",
                           "FLAG" );
            IDE_TEST(dumpPH( smpAllocPageList::getFirstAllocPageID(
                             &sTH->mVarAllocList.mMRDB[y]) )
                != IDE_SUCCESS );
        }

    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_disk_or_temp_tbs);
    {
        idlOS::printf("[ERR] Can't Display Disk or Temp Table Headers\n");
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
    const SChar * sBanner = "DUMPCI.ban";

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

const smiColumn* getColumn( smcTableHeader  * aTableHeader,
                            const UInt        aIndex )
{
    UInt                   sCurSize;
    const smiColumn      * sColumn = NULL;
    smVCPieceHeader      * sVCPieceHeader;
    UInt                   sOffset;
    scPageID               sPID;
    scOffset               sPos;

    IDE_DASSERT( aTableHeader != NULL );

    IDE_DASSERT( aIndex < smcTable::getColumnCount(aTableHeader) );

    /* 테이블의 컬럼정보를 담은 시작 variable slot */
    sPID = SM_MAKE_PID( aTableHeader->mColumns.fstPieceOID );
    sPos = SM_MAKE_OFFSET( aTableHeader->mColumns.fstPieceOID );
    IDE_ASSERT( readPage( sPID ) == IDE_SUCCESS );

    sVCPieceHeader = (smVCPieceHeader*)&(gPages[sPID][sPos]);

    sOffset = aTableHeader->mColumnSize * aIndex;

    for( sCurSize = sVCPieceHeader->length; sCurSize <= sOffset; )
    {
        sOffset -= sCurSize;
        // next variable slot으로 이동
        IDE_ASSERT(sVCPieceHeader->nxtPieceOID != SM_NULL_OID);

        sPID = SM_MAKE_PID( sVCPieceHeader->nxtPieceOID );
        sPos = SM_MAKE_OFFSET( sVCPieceHeader->nxtPieceOID );
        IDE_ASSERT( readPage( sPID ) == IDE_SUCCESS );

        sVCPieceHeader = (smVCPieceHeader*)&(gPages[sPID][sPos]);

        sCurSize = sVCPieceHeader->length;
    }

    sColumn = (smiColumn*)
        ((UChar*)sVCPieceHeader + ID_SIZEOF(smVCPieceHeader)+ sOffset);

    IDE_ASSERT( sColumn != NULL );

    return sColumn;
}

/* BUG-31863 [sm-util] The dump utility is necessary to verify
 * FreePageList of memory tablespace. */

/* Memory Tablespace의 Free Page List를 덤프한다. */
IDE_RC dumpFreePageLists( smmTBSNode * aTBSNode )
{
    UInt i;

    idlOS::printf( "%s %s %s\n\n",
                   LSP, 
                   "DUMP FREE PAGE LISTS", 
                   RSP );

    idlOS::printf( "FREE PAGE LIST COUNT: %"ID_UINT32_FMT"\n",
                   aTBSNode->mMemBase->mFreePageListCount );

    for( i = 0; i < aTBSNode->mMemBase->mFreePageListCount; i++ )
    {
        IDE_TEST( dumpFreePageList(
                      aTBSNode,
                      i,
                      aTBSNode->mMemBase->mFreePageLists[i].mFirstFreePageID,
                      aTBSNode->mMemBase->mFreePageLists[i].mFreePageCount )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dumpFreePageList( smmTBSNode * aTBSNode,
                         UInt         aListNo,
                         scPageID     aFirstPID,
                         vULong       aPageCount )
{
    idBool        sIsValid;
    scPageID      sPID;
    scPageID      sNextPID;
    vULong        sPageCount;
    const SChar * sMsg;

    sIsValid    = ID_TRUE;
    sPID        = aFirstPID;
    sNextPID    = SM_NULL_PID;
    sPageCount  = 0;

    // print title
    idlOS::printf( "FREE PAGE LIST #%02"ID_UINT32_FMT"\n", aListNo );
    idlOS::printf( "  - First PID: %"ID_UINT32_FMT"\n", aFirstPID );
    idlOS::printf( "  - Page Count: %"ID_vULONG_FMT"\n", aPageCount );

    // dump list
    idlOS::printf( "  - PID List: \n" );

    // Seq | PID
    idlOS::printf( "    %7s  %11s\n", "Seq", "PID" );

    while( sPID != SM_NULL_PID )
    {
        idlOS::printf( "    %7"ID_vULONG_FMT"  %11"ID_UINT32_FMT"\n",
                       sPageCount, sPID );

        if( smmExpandChunk::isDataPageID(aTBSNode, sPID) != ID_TRUE )
        {
            sMsg = "This page isn't data page.";
            sIsValid = ID_FALSE;
            break;
        }

        sPageCount++;
        IDE_TEST( getNextFreePage(aTBSNode, sPID, &sNextPID) != IDE_SUCCESS );

        if( sNextPID == SMM_FLI_ALLOCATED_PID )
        {
            sMsg = "This page is already allocated.";
            sIsValid = ID_FALSE;
            break;
        }

        sPID = sNextPID;
    }

    if( (sIsValid == ID_TRUE) && (sPageCount != aPageCount) )
    {
        sMsg = "There is a mismatch in free page count.";
        sIsValid = ID_FALSE;
    }

    if( sIsValid != ID_TRUE )
    {
        idlOS::printf( "========================================\n" );
        idlOS::printf( "FREE PAGE LIST IS INVALID: %s\n", sMsg );
        idlOS::printf( "========================================\n" );
        idlOS::printf( "* Page Count: %"ID_vULONG_FMT"\n"
                       "* PID:        %"ID_UINT32_FMT"\n"
                       "* Next PID:   %"ID_UINT32_FMT"\n",
                       sPageCount,
                       sPID,
                       sNextPID );
        idlOS::printf( "========================================\n" );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC getNextFreePage( smmTBSNode * aTBSNode,
                        scPageID     aFreePageID,
                        scPageID   * aNextFreePageID )
{
    scPageID       sFLIPageID;
    UInt           sSlotOffset;
    smpPersPage  * sFLIPageAddr;
    smmFLISlot   * sSlotAddr;

    IDE_DASSERT( aNextFreePageID != NULL );
    
    // 데이터 페이지의 Slot을 저장하는 FLI Page의 ID를 알아낸다.
    sFLIPageID = smmExpandChunk::getFLIPageID( aTBSNode, aFreePageID );

    // FLI Page안에서 데이터페이지의 Slot Offset을 구한다.
    sSlotOffset = smmExpandChunk::getSlotOffsetInFLIPage( aTBSNode, aFreePageID );

    // Free List Info Page 의 주소를 알아낸다.
    IDE_TEST( readPage(sFLIPageID) != IDE_SUCCESS );
    sFLIPageAddr = (smpPersPage *)gPages[ sFLIPageID ];

    // Slot 의 주소를 계산한다.
    sSlotAddr = (smmFLISlot *)
        ( ((SChar*)sFLIPageAddr) + aTBSNode->mFLISlotBase + sSlotOffset );

    *aNextFreePageID = sSlotAddr->mNextFreePageID;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* FLI Page를 덤프한다. */
IDE_RC dumpFLIPages( smmTBSNode * aTBSNode )
{
    vULong  i;

    idlOS::printf( "%s %s %s\n\n",
                   LSP, 
                   "DUMP FLI PAGES", 
                   RSP );

    idlOS::printf( "SMM_DATABASE_META_PAGE_CNT:      %"ID_vULONG_FMT"\n"
                   "CURRENT_EXPAND_CHUNK_PAGE_COUNT: %"ID_vULONG_FMT"\n"
                   "CHUNK_PAGE_COUNT:                %"ID_vULONG_FMT"\n"
                   "FLI_PAGE_COUNT:                  %"ID_UINT32_FMT"\n"
                   "FLI_SLOT_COUNT:                  %"ID_UINT32_FMT"\n",
                   SMM_DATABASE_META_PAGE_CNT,
                   aTBSNode->mMemBase->mCurrentExpandChunkCnt,
                   aTBSNode->mChunkPageCnt,
                   smmExpandChunk::getChunkFLIPageCnt(aTBSNode),
                   smmExpandChunk::getFLISlotCount() );

    for( i = 0; i < aTBSNode->mMemBase->mCurrentExpandChunkCnt; i++ )
    {
        IDE_TEST( dumpFLIPage(aTBSNode, i) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dumpFLIPage( smmTBSNode * aTBSNode, vULong aChunkNo )
{
    scPageID      sFstDataPID;
    scPageID      sFstChunkPID;
    scPageID      sFLIPageID;
    smpPersPage * sFLIPageAddr;
    UInt          sFLIPageCnt;
    UInt          sFLISlotCnt;
    smmFLISlot  * sSlotBase;
    smmFLISlot  * sSlotAddr;
    vULong        sChunkPageCnt;
    UInt          i;
    UInt          j;

    sChunkPageCnt = aTBSNode->mChunkPageCnt;
    
    sFLIPageCnt   = smmExpandChunk::getChunkFLIPageCnt(aTBSNode);
    sFLISlotCnt   = smmExpandChunk::getFLISlotCount();

    sFstChunkPID  = (aChunkNo * sChunkPageCnt) + SMM_DATABASE_META_PAGE_CNT;
    sFstDataPID   = sFstChunkPID + sFLIPageCnt;

    // print title
    idlOS::printf( "CHUNK NO #%02"ID_vULONG_FMT"\n", aChunkNo );
    idlOS::printf( "  - FLIPage Count: %"ID_UINT32_FMT"\n", sFLIPageCnt );

    // dump slot
    for( i = 0; i < sFLIPageCnt; i++ )
    {
        sFLIPageID = sFstChunkPID + i;
        idlOS::printf( "  - FLIPage #%02"ID_UINT32_FMT""
                       "(PID: %"ID_UINT32_FMT")\n",
                       i,
                       sFLIPageID );
        
        IDE_TEST( readPage(sFLIPageID) != IDE_SUCCESS );
        sFLIPageAddr = (smpPersPage *)gPages[ sFLIPageID ];

        sSlotBase = (smmFLISlot *)
            ( ((SChar *)sFLIPageAddr) + aTBSNode->mFLISlotBase );

        // SlotNum | PID | NextFreePID | Used
        idlOS::printf( "    %7s  %11s  %11s  %4s\n", "SlotNum", "PID", "NextFreePID", "Used" );

        for( j = 0; j < sFLISlotCnt; j++ )
        {
            sSlotAddr = sSlotBase + j;

            idlOS::printf("    %7"ID_UINT32_FMT"  %11"ID_UINT32_FMT"  %11"ID_UINT32_FMT"  %4s\n",
                          j,
                          sFstDataPID + (i * sFLISlotCnt) + j,
                          sSlotAddr->mNextFreePageID,
                          (sFLIPageCnt + (i*sFLISlotCnt) + j) < sChunkPageCnt ? "Y" : "N" );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

