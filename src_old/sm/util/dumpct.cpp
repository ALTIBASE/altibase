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
 * $Id: dumpct.cpp 50206 2011-12-21 07:18:01Z sliod $
 **********************************************************************/

#include <idl.h>
#include <smiDef.h>
#include <smriChangeTrackingMgr.h>

smriCTBody * gCTBodyPtr[SMRI_CT_MAX_CT_BODY_CNT];
iduFile      gFile;
UInt         gCTBodySize;
UInt         gCTBodyBlockCnt;

void ShowCopyRight(void);
IDE_RC dumpChangeTrakingFile( SChar*  aLogAnchorPath );
void dumpDataFileDescSlot( smriCTDataFileDescBlock * aDataFileDescBlock );
void dumpBmpExtList( smriCTBmpExtList * aBmpExtList );
IDE_RC loadCTBody( ULong aCTBodyOffset, UInt aCTBodyIdx );
void getBlock( UInt aBlockID, void ** aBlock );


IDE_RC dumpChangeTrakingFile( SChar * aChangeTrakingFilePath )
{
    smriCTFileHdrBlock          sCTFileHdr;
    smriCTMetaExt             * sCTMetaExt;
    smriCTDataFileDescBlock   * sDataFileDescBlock;
    UInt                        sDataFileDescBlockIdx;
    UInt                        sBodyIdx;
    UInt                        sLoadCTBodyCNT;
    UInt                        sCTBodyIdx;
    UInt                        sCTBodyOffset;

    IDE_DASSERT( aChangeTrakingFilePath != NULL );

    gCTBodySize = ( SMRI_CT_EXT_CNT
                   * SMRI_CT_EXT_BLOCK_CNT
                   * SMRI_CT_BLOCK_SIZE );
                   
    gCTBodyBlockCnt = (gCTBodySize / SMRI_CT_BLOCK_SIZE);

    IDE_TEST( gFile.initialize(IDU_MEM_SM_SMR,
                               1, /* Max Open FD Count */
                               IDU_FIO_STAT_OFF,
                               IDV_WAIT_INDEX_NULL) 
              != IDE_SUCCESS );


    IDE_TEST( gFile.setFileName( aChangeTrakingFilePath ) != IDE_SUCCESS );

    IDE_TEST( gFile.open() != IDE_SUCCESS );

    IDE_TEST( gFile.read(NULL,
                         SMRI_CT_HDR_OFFSET, 
                         (SChar*)&sCTFileHdr,
                         (UInt)ID_SIZEOF(smriCTFileHdrBlock))
              != IDE_SUCCESS );

    for( sLoadCTBodyCNT = 0, sCTBodyOffset = SMRI_CT_BLOCK_SIZE; 
         sLoadCTBodyCNT < sCTFileHdr.mCTBodyCNT; 
         sLoadCTBodyCNT++, sCTBodyOffset += gCTBodySize )
    {
        IDE_TEST( loadCTBody( sCTBodyOffset, sLoadCTBodyCNT ) != IDE_SUCCESS );
    }

    idlOS::printf("<< DUMP OF CHANGETRACKING FILE - %s >>\n\n",
                  aChangeTrakingFilePath );

    idlOS::printf("\n------------------------\n");
    idlOS::printf("[CHANGE TRAKING FILE HDR]\n");

    idlOS::printf("Change tracking body count    [ %"ID_UINT32_FMT" ]\n",
                   sCTFileHdr.mCTBodyCNT );

    idlOS::printf("Icremental backup chunk size  [ %"ID_UINT32_FMT" ]\n",
                   sCTFileHdr.mIBChunkSize );

    idlOS::printf("Last flush LSN                [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sCTFileHdr.mLastFlushLSN.mFileNo,
                   sCTFileHdr.mLastFlushLSN.mOffset );

    idlOS::printf("Database name                 [ %s ]\n",
                   sCTFileHdr.mDBName );
    idlOS::printf("\n------------------------\n");
                   
    idlOS::printf("[CHANGE TRAKING FILE BODY]\n");
    for( sBodyIdx = 0; sBodyIdx < sCTFileHdr.mCTBodyCNT; sBodyIdx++ )
    {
        sCTMetaExt = &gCTBodyPtr[sBodyIdx]->mMetaExtent;
        
        idlOS::printf("Change tracking body ID   [ %"ID_UINT32_FMT" ]\n", sBodyIdx );

        idlOS::printf("Flush LSN                 [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                       sCTMetaExt->mExtMapBlock.mFlushLSN.mFileNo,
                       sCTMetaExt->mExtMapBlock.mFlushLSN.mOffset );

        idlOS::printf("\nDatafile descriptor slot\n");

        for( sDataFileDescBlockIdx = 0; 
             sDataFileDescBlockIdx < SMRI_CT_EXT_BLOCK_CNT - 1; 
             sDataFileDescBlockIdx++ )
        {
            sDataFileDescBlock = &sCTMetaExt->mDataFileDescBlock[ sDataFileDescBlockIdx ];

            dumpDataFileDescSlot( sDataFileDescBlock );
        }
    }

    IDE_TEST( gFile.close() != IDE_SUCCESS );

    IDE_TEST( gFile.destroy() != IDE_SUCCESS );

    for( sCTBodyIdx = 0;
         sCTBodyIdx < sLoadCTBodyCNT ;
         sCTBodyIdx++ )
    {
        IDE_TEST( iduMemMgr::free( gCTBodyPtr[ sCTBodyIdx ] ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dumpDataFileDescSlot( smriCTDataFileDescBlock * aDataFileDescBlock )
{
    UInt    sDataFileDescSlotIdx;

    for( sDataFileDescSlotIdx = 0; 
         sDataFileDescSlotIdx < SMRI_CT_DATAFILE_DESC_SLOT_CNT; 
         sDataFileDescSlotIdx++ )
    {
        if( aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mBmpExtCnt != 0 )
        {
            idlOS::printf("\n");

            idlOS::printf("------------------------\n");
            idlOS::printf("Slot ID                      [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mSlotID.mBlockID,
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mSlotID.mSlotIdx );
            
            idlOS::printf("Tracking state               [ %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mTrackingState );
            
            idlOS::printf("Tablespace type              [ %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mTBSType );

            idlOS::printf("Page size                    [ %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mPageSize );

            idlOS::printf("Bitmap extent count          [ %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mBmpExtCnt );

            idlOS::printf("Current tracking list ID     [ %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mCurTrackingListID );

            idlOS::printf("Differential0 BmpExt list    \n");
            dumpBmpExtList( &aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mBmpExtList[0] );
            
            idlOS::printf("Differential1 BmpExt list    \n");
            dumpBmpExtList( &aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mBmpExtList[1] );

            idlOS::printf("Cumulative BmpExt list       \n");
            dumpBmpExtList( &aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mBmpExtList[2] );

            idlOS::printf("Tablespace ID                [ %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mSpaceID );

            idlOS::printf("File ID                      [ %"ID_UINT32_FMT" ]\n",
                          aDataFileDescBlock->mSlot[ sDataFileDescSlotIdx ].mFileID );
            idlOS::printf("------------------------\n");
        }               
    }
    return;
}

void dumpBmpExtList( smriCTBmpExtList * aBmpExtList )
{
    UInt                    i;
    UInt                    sFirstBlockID;
    UInt                    sNextBlockID;
    smriCTBmpExtHdrBlock  * sBmpExtHdrBlock;

    sFirstBlockID = aBmpExtList->mBmpExtListHint[0];

    sNextBlockID = sFirstBlockID;

    idlOS::printf("List :                       ");
    while(1)
    {
        getBlock( sNextBlockID, (void**)&sBmpExtHdrBlock ); 

        idlOS::printf("[ %"ID_UINT32_FMT" ]", sNextBlockID );

        if( sFirstBlockID == sBmpExtHdrBlock->mNextBmpExtHdrBlockID )
        {
            break;
        }

        sNextBlockID = sBmpExtHdrBlock->mNextBmpExtHdrBlockID;
    }
    idlOS::printf("\n");

    idlOS::printf("Hint :                       ");
    for( i = 0; i < SMRI_CT_BMP_EXT_LIST_HINT_CNT; i++ )
    {
        if( aBmpExtList->mBmpExtListHint[i] != SMRI_CT_INVALID_BLOCK_ID )
        {
            idlOS::printf("[ %"ID_UINT32_FMT" ]",
                          aBmpExtList->mBmpExtListHint[i] );
        }
    }
    idlOS::printf("\n");

    return;
}

void getBlock( UInt aBlockID, void ** aBlock )
{
    UInt            sCTBodyIdx;
    UInt            sBlockIdx;
    smriCTBody    * sCTBody;

    IDE_DASSERT( aBlock != NULL );

    sCTBodyIdx  = aBlockID / gCTBodyBlockCnt;
    sCTBody     = gCTBodyPtr[ sCTBodyIdx ] ;
    sBlockIdx   = aBlockID - ( sCTBodyIdx * gCTBodyBlockCnt );

    *aBlock = (void *)&sCTBody->mBlock[ sBlockIdx ];

    return;
}

IDE_RC loadCTBody( ULong aCTBodyOffset, UInt aCTBodyIdx )
{
    smriCTBody    * sCTBody;
    UInt            sState = 0;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                 1,
                                 ID_SIZEOF( smriCTBody ),
                                 (void**)&sCTBody )
              != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( gFile.read( NULL,  //aStatistics
                          aCTBodyOffset,
                          (void *)sCTBody,
                          gCTBodySize,
                          NULL ) //ReadSize
              != IDE_SUCCESS );

    sCTBody->mBlock = (smriCTBmpBlock *)sCTBody;
    
    gCTBodyPtr[ aCTBodyIdx ]  = sCTBody;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sCTBody ) == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

int main(SInt argc, SChar *argv[])
{
    IDE_RC rc;

    ShowCopyRight();
    
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
   
    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST(iduCond::initializeStatic() != IDE_SUCCESS);
 
    if(argc == 2)
    {
        rc = dumpChangeTrakingFile(argv[1]);
        IDE_TEST_RAISE(rc != IDE_SUCCESS, err_file_invalid);
    }
    else
    {
        idlOS::printf("usage: dumpct changetraking_file\n");
        IDE_RAISE(err_arg_invalid);
    }

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS);
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );
    
    return 0;

    IDE_EXCEPTION(err_arg_invalid);
    {
    }
    IDE_EXCEPTION(err_file_invalid);
    {
    idlOS::printf("Check file type\n");
    }
    IDE_EXCEPTION_END;
    
    return -1;
}

void ShowCopyRight(void)
{
    SChar         sBuf[1024 + 1];
    SChar         sBannerFile[1024];
    SInt          sCount;
    FILE        * sFP;
    SChar       * sAltiHome;
    const SChar * sBanner = "DUMPCT.ban";

    sAltiHome = idlOS::getenv(IDP_HOME_ENV);
    IDE_TEST_RAISE( sAltiHome == NULL, err_altibase_home );

    // make full path banner file name
    idlOS::memset(sBannerFile, 0, ID_SIZEOF(sBannerFile));
    idlOS::snprintf(sBannerFile, ID_SIZEOF(sBannerFile), "%s%c%s%c%s"
        , sAltiHome, IDL_FILE_SEPARATOR, "msg", IDL_FILE_SEPARATOR, sBanner);

    sFP = idlOS::fopen(sBannerFile, "r");
    IDE_TEST_RAISE( sFP == NULL, err_file_open );

    sCount = idlOS::fread( (void*) sBuf, 1, 1024, sFP );
    IDE_TEST_RAISE( sCount <= 0, err_file_read );

    sBuf[sCount] = '\0';
    idlOS::printf("%s", sBuf);
    idlOS::fflush(stdout);

    idlOS::fclose(sFP);

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
        idlOS::fclose(sFP);
    }
    IDE_EXCEPTION_END;
}
