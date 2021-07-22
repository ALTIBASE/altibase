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
 *
 * Description :
 *
 * - Incremental chunk change tracking manager
 *
 **********************************************************************/

#ifndef _O_SMRI_CT_MANAGER_H
#define _O_SMRI_CT_MANAGER_H 1

#include <smr.h>
#include <smriDef.h>
#include <smiDef.h>
#include <sctDef.h>


class smriChangeTrackingMgr
{
private:
    /*change tracking파일 헤더*/
    static smriCTFileHdrBlock   mCTFileHdr;

    static smriCTHdrState       mCTHdrState;

    /*CT파일 */
    static iduFile              mFile;

    /*CT body에 대한 pointer*/
    static smriCTBody         * mCTBodyPtr[SMRI_CT_MAX_CT_BODY_CNT];       

    /*CT body를 flush하기 위한 buffer */
    static smriCTBody         * mCTBodyFlushBuffer;

    static iduMutex             mMutex;

    static UInt                 mBmpBlockBitmapSize;
    static UInt                 mCTBodyExtCnt;
    static UInt                 mCTExtMapSize;
    static UInt                 mCTBodySize;
    static UInt                 mCTBodyBlockCnt;
    static UInt                 mCTBodyBmpExtCnt;
    static UInt                 mBmpBlockBitCnt;
    static UInt                 mBmpExtBitCnt;

    static iduFile            * mSrcFile;
    static iduFile            * mDestFile;
    static SChar              * mIBChunkBuffer;
    static ULong                mCopySize;
    static smriCTTBSType        mTBSType;
    static UInt                 mIBChunkCnt;
    static UInt                 mIBChunkID;

    static UInt                 mFileNo;
    static scPageID             mSplitFilePageCount;
    
    static UInt                 mCurrChangeTrackingThreadCnt;

private:

    /*CTBody 초기화*/
    static IDE_RC createCTBody( smriCTBody * aCTBody );

    /*파일 헤더 블럭 초기화*/
    static IDE_RC createFileHdrBlock( smriCTFileHdrBlock * aCTFileHdrBlock );

    /*extent map 블럭 초기화*/
    static IDE_RC createExtMapBlock( smriCTExtMapBlock * aExtMapBlock, 
                                     UInt                aBlockID );

    /*DataFile Desc 블럭 초기화*/
    static IDE_RC createDataFileDescBlock( 
                        smriCTDataFileDescBlock * aDataFileDescBlock,
                        UInt                      aBlockID );

    /*DataFile Desc slot 초기화*/
    static IDE_RC createDataFileDescSlot( 
                        smriCTDataFileDescSlot * aDataFileDescSlot,
                        UInt                     aBlockID,
                        UInt                     aSlotIdx );

    /*BmpExtHdr블럭 초기화*/
    static IDE_RC createBmpExtHdrBlock( smriCTBmpExtHdrBlock * aBmpExtHdrBlock,
                                        UInt                   aBlockID );

    /*Bmp 블럭 초기화*/
    static IDE_RC createBmpBlock( smriCTBmpBlock * aBmpBlock,
                                  UInt             aBlockID );

    /*블럭 헤더 초기화*/
    static IDE_RC createBlockHdr( smriCTBlockHdr    * aBlockHdr, 
                                  smriCTBlockType     aBlockType, 
                                  UInt                aBlockID );

    /*이미 존재 하는 모든 테이블스페이스의 데이터파일들을 CT파일에 등록*/
    static IDE_RC addAllExistingDataFile2CTFile( idvSQL * aStatistic );

    /*이미 존재하는 한 테이블 스페이스의 데이터파일을 CT파일에 등록*/
    static IDE_RC addExistingDataFile2CTFile( 
                        idvSQL                      * aStatistic,
                        sctTableSpaceNode           * aSpaceNode,
                        void                        * aActionArg );

    static IDE_RC loadCTBody( ULong aCTBodyOffset, UInt aCTBodyIdx );
    static IDE_RC unloadCTBody( smriCTBody * aCTBody );

    /*CT파일검증 함수*/
    static IDE_RC validateCTFile();
    static IDE_RC validateBmpExtList( smriCTBmpExtList * aBmpExtList, 
                                      UInt               aBmpExtListLen );

    static IDE_RC initBlockMutex( smriCTBody * aCTBody );
    static IDE_RC destroyBlockMutex( smriCTBody * aCTBody );

    static IDE_RC initBmpExtLatch( smriCTBody * aCTBody );
    static IDE_RC destroyBmpExtLatch( smriCTBody * aCTBody );

    /*flush하기위한 준비*/
    static IDE_RC startFlush();

    /*flush 완료*/
    static IDE_RC completeFlush();

    /*CT파일 헤더 flush*/
    static IDE_RC flushCTFileHdr(); 

    /*CTBody flush*/
    static IDE_RC flushCTBody( smLSN aFlushLSN );

    /*extend 하기위한 준비*/
    static IDE_RC startExtend( idBool * aIsNeedExtend );

    /*extend 완료*/
    static IDE_RC completeExtend();

    /*CT파일의 flush,extend수행이 완료되는것을 대기*/
    static IDE_RC wait4FlushAndExtend();

    /*DataFile Desc slot을 할당다고 가져온다.*/
    static IDE_RC allocDataFileDescSlot( smriCTDataFileDescSlot ** aSlot );
    
    /*사용중이지 않은 DataFile Desc slot의 Idx를 가져온다.*/
    static void getFreeSlotIdx( UInt aAllocSlotFlag, UInt * aSlotIdx );

    /*DataFile Desc slot에 BmpExt를 추가*/
    static IDE_RC addBmpExt2DataFileDescSlot( 
                                    smriCTDataFileDescSlot * aSlot,
                                    smriCTBmpExt          ** sAllocCurBmpExt );

    /*사용할 BmpExt를 할당*/
    static IDE_RC allocBmpExt( smriCTBmpExt ** aAllocBmpExt );

    /*할당 받은 BmpExt를 dataFile Desc slot의 BmpExtList에 추가*/
    static void addBmpExt2List( smriCTBmpExtList       * aBmpExtList,
                                smriCTBmpExt           * aNewBmpExt );

    /* DataFile Desc slot에 있는 BmpExtList에 할당된 BmpExt를 할당해제하고 초기화
     * 한다.*/
    static IDE_RC deleteBmpExtFromDataFileDescSlot( smriCTDataFileDescSlot * aSlot );

    static IDE_RC deallocDataFileDescSlot( smriCTDataFileDescSlot  * aSlot );

    static IDE_RC deleteBmpExtFromList( smriCTBmpExtList      * aBmpExtList,
                                      smriCTBmpExtHdrBlock ** aBmpExtHdrBlock );

    /*할당되어 사용중이던 BmpExt의 할당을 해제한다.*/
    static IDE_RC deallocBmpExt( smriCTBmpExtHdrBlock  * sBmpExtHdrBlock );

    /*변경된 IBChunk를 추적중인 current BmpExt를 가져온다.*/
    static IDE_RC getCurBmpExt( 
                        smriCTDataFileDescSlot   * aSlot,
                        UInt                       aIBChunkID,
                        smriCTBmpExt            ** aCurBmpExt );
    
    /*BmpExtList에서 aBmpExtSequence번째의 BmpExt를 가져온다.*/
    static IDE_RC getCurBmpExtInternal( smriCTDataFileDescSlot   * aSlot,
                                      UInt                       aBmpExtSeq,
                                      smriCTBmpExt            ** aCurBmpExt );

    /*IBChunkID와 맵핑되는 bit의 위치를 구하고 bit를 set한다.*/
    static IDE_RC calcBitPositionAndSet( smriCTBmpExt             * aCurBmpExt,
                                         UInt                       aIBChunkID,
                                         smriCTDataFileDescSlot   * sSlot );

    /*bit를 set하기위한 함수*/
    static IDE_RC setBit( smriCTBmpExt     * aBmpExt, 
                          UInt               aBmpBlockIdx, 
                          UInt               aByteIdx, 
                          UInt               aBitPosition,
                          smriCTBmpExtList * aBmpExtList );

    static IDE_RC makeLevel1BackupFilePerEachBmpExt( 
                                        smriCTBmpExt * aBmpExt,
                                        UInt           aBmpExtSeq );
    
    static IDE_RC makeLevel1BackupFilePerEachBmpBlock( 
                                         smriCTBmpBlock   * aBmpBlock,  
                                         UInt               aBmpExtSeq, 
                                         UInt               aBmpBlockIdx );

    static IDE_RC makeLevel1BackupFilePerEachBmpByte( SChar * aBmpByte,
                                                      UInt    aBmpExtSeq,
                                                      UInt    aBmpBlockIdx,
                                                      UInt    aBmpByteIdx );

    static IDE_RC copyIBChunk( UInt aIBChunkID );

    /*DifBmpExtList의 첫번째 BmpExt를 가져온다.*/
    static void getFirstDifBmpExt( smriCTDataFileDescSlot   * aSlot,
                                   UInt                     * aBmpExtBlockID);

    /*CurBmpExtList의 첫번째 BmpExt를 가져온다.*/
    static void getFirstCumBmpExt( smriCTDataFileDescSlot   * aSlot,
                                   UInt                     * aBmpExtBlockID);

    /*BlockID를 이용해 해당블록을 가져온다.*/
    static void getBlock( UInt aBlockID, void ** aBlock );

    static void setBlockCheckSum( smriCTBlockHdr * aBlockHdr );
    static IDE_RC checkBlockCheckSum( smriCTBlockHdr * aBlockHdr );

    static void setExtCheckSum( smriCTBlockHdr * aBlockHdr );
    static IDE_RC checkExtCheckSum( smriCTBlockHdr * aBlockHdr );

    static void setCTBodyCheckSum( smriCTBody * aCTBody, smLSN aFlushLSN );
    static IDE_RC checkCTBodyCheckSum( smriCTBody * aCTBody, smLSN aFlushLSN );

    inline static IDE_RC lockCTMgr()
    {
        return mMutex.lock ( NULL );
    }

    inline static IDE_RC unlockCTMgr()
    {
        return mMutex.unlock ();
    }

    static inline UInt getCurrChangeTrackingThreadCnt()
    { return idCore::acpAtomicGet32( &mCurrChangeTrackingThreadCnt ); };

public:

    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC begin();
    static IDE_RC end();
 
    /*CT파일 생성*/
    static IDE_RC createCTFile();
    static IDE_RC removeCTFile();

    /*flush 수행함수*/
    static IDE_RC flush();

    /*extend 수행 함수*/
    static IDE_RC extend( idBool aFlushBody, smriCTBody ** aCTBody );

    /*데이터파일을 CT파일에 등록*/
    static IDE_RC addDataFile2CTFile( scSpaceID                     aSpaceID, 
                                      UInt                          aDataFileID, 
                                      smriCTTBSType                 aTBSType,
                                      smiDataFileDescSlotID      ** aSlotID );

    /*CT파일에 등록되어있는 데이터파일을 등록해제한다.*/
    //static IDE_RC deallocDataFileDescSlot( smiDataFileDescSlotID * aSlotID );
    static IDE_RC deleteDataFileFromCTFile( smiDataFileDescSlotID * aSlotID );


    /*differential bakcup변경 정보를 추적중인 BmpExtList를 switch한다.*/
    static IDE_RC switchBmpExtListID( smriCTDataFileDescSlot * aSlot );

    static IDE_RC changeTracking( sddDataFileNode * sDataFileNode,
                                  smmDatabaseFile * aDatabaseFile,
                                  UInt              aIBChunkID );

    /*BmpExtList에 속한 모든 Bmp block들을 초기화 한다.*/
    static IDE_RC initBmpExtListBlocks( 
                            smriCTDataFileDescSlot * aDataFileDescSlot,
                            UInt                     sBmpExtListID );

    /* incrementalBackup을 수행한다. */
    static IDE_RC performIncrementalBackup(                        
                                smriCTDataFileDescSlot * aDataFileDescSlot,           
                                iduFile                * aSrcFile,
                                iduFile                * aDestFile,
                                smriCTTBSType            aTBSType,
                                scSpaceID                aSpaceID,
                                UInt                     aFileNo,
                                smriBISlot             * aBackupInfo );
    
    /* restart recovery중 DataFileDescSlot을 할당할 필요가있는지 검사한다.*/
    static IDE_RC isNeedAllocDataFileDescSlot( 
                                    smiDataFileDescSlotID * aDataFileDescSlotID,
                                    scSpaceID               aSpaceID,
                                    UInt                    aFileNum,
                                    idBool                * aResult );

    /* DataFileSlotID에 해당하는 Slot을 가져온다. */
    static void getDataFileDescSlot( smiDataFileDescSlotID    * aSlotID, 
                                     smriCTDataFileDescSlot  ** aSlot );

    static IDE_RC makeLevel1BackupFile( smriCTDataFileDescSlot * aCTSlot,
                                        smriBISlot             * aBackupInfo,
                                        iduFile                * aSrcFile,
                                        iduFile                * aDestFile,
                                        smriCTTBSType            aTBSType,
                                        scSpaceID                aSpaceID,
                                        UInt                     aFileNo );

    /*DataFile Desc slot에 할당된 모든 BmpExtHdr의 latch를 획득한다.*/
    static IDE_RC lockBmpExtHdrLatchX( smriCTDataFileDescSlot * aSlot,
                                       UShort                   alockListID );

    /*DataFile Desc slot에 할당된 모든 BmpExtHdr의 latch를 해제한다.*/
    static IDE_RC unlockBmpExtHdrLatchX( smriCTDataFileDescSlot * aSlot,
                                         UShort                   alockListID );

    static IDE_RC checkDBName( SChar * aDBName );

    inline static void getIBChunkSize( UInt * aIBChunkSize ) 
    { 
       *aIBChunkSize  = mCTFileHdr.mIBChunkSize; 
    }
    
    inline static UInt calcIBChunkID4DiskPage( scPageID aPageID )
    {
        return SD_MAKE_FPID(aPageID) / mCTFileHdr.mIBChunkSize;
    }

    inline static UInt calcIBChunkID4MemPage( scPageID aPageID )
    {
        return aPageID / mCTFileHdr.mIBChunkSize;
    }

    inline static idBool isAllocDataFileDescSlotID( 
                            smiDataFileDescSlotID * aSlotID )
    {
        idBool sResult;        

        if( (aSlotID->mBlockID != SMRI_CT_INVALID_BLOCK_ID) &&
            (aSlotID->mSlotIdx != SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX) )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }

        return sResult;
    }

    inline static idBool isValidDataFileDescSlot4Mem( 
                                    smmDatabaseFile          * aDatabaseFile, 
                                    smiDataFileDescSlotID    * aSlotID )
    {
        idBool                      sResult;
        smriCTDataFileDescSlot    * sSlot;
        scSpaceID                   sSpaceID;
        UInt                        sFileNum;
        
        getDataFileDescSlot( aSlotID, &sSlot );

        sFileNum    = aDatabaseFile->getFileNum();
        sSpaceID    = aDatabaseFile->getSpaceID(); 
        
        if( ( sSlot->mSpaceID == sSpaceID ) &&
            ( sSlot->mFileID  == sFileNum ) )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
        
        return sResult;
    }

    inline static idBool isValidDataFileDescSlot4Disk( 
                                    sddDataFileNode          * aFileNode,
                                    smiDataFileDescSlotID    * aSlotID )
    {
        idBool                      sResult;
        smriCTDataFileDescSlot    * sSlot;
        
        getDataFileDescSlot( aSlotID, &sSlot );

        if( ( sSlot->mSpaceID == aFileNode->mSpaceID ) &&
            ( sSlot->mFileID == aFileNode->mID ) )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
        
        return sResult;
    }

    static IDE_RC changeTracking4WriteDiskPage( sddDataFileNode * aDataFileNode,
                                                scPageID          aPageID );
};
#endif //_O_SMRI_CT_MANAGER_H
