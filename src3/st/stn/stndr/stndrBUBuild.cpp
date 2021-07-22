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
 * $Id: stndrBUBuild.cpp 89495 2020-12-14 05:19:22Z emlee $
 *
 * Description :
 *
 * Implementation Of Parallel Building Disk Index
 *
 * # Function
 *
 *
 **********************************************************************/
#include <smErrorCode.h>
#include <ide.h>
#include <smu.h>
#include <sdrMiniTrans.h>
#include <sdp.h>
#include <smc.h>
#include <sdc.h>
#include <smn.h>
#include <sdnReq.h>
#include <stndrBUBuild.h>
#include <smuQueueMgr.h>
#include <sdnIndexCTL.h>
#include <sdbMPRMgr.h>
#include <sdbBufferMgr.h>


typedef struct stndrReqFunc
{
    void (*getCPFromBuildKey)( UChar            ** aBuildKeyMap,
                               UInt                aPos,
                               stndrCenterPoint  * aCenterPoint );

    SInt (*compareBuildKeyAndCP)( stndrCenterPoint   aCenterPoint,
                                  void             * aBuildKey,
                                  stndrSortType      aSortType );
 
    UInt (*getBuildKeyLength)( UInt aBuildKeyValueLength );

    void (*insertBuildKey)( sdpPhyPageHdr * aNode,
                            SShort          aSlotSeq,
                            void          * aBuildKey );

    SInt (*compareBuildKeyAndBuildKey)( void          * aBuildKey1,
                                        void          * aBuildKey2,
                                        stndrSortType   aSortType );

    idBool (*checkSortBuildKey)( UInt             aHead,
                                 UInt             aTail,
                                 UChar         ** aBuildKeyMap,
                                 stndrSortType    aSortType );

    UInt (*getNodeKeyLength)( UInt aNodeKeyValueLength );

    UInt (*getFreeSize4EmptyNode)( stndrHeader * aIndexHeader );

    IDE_RC (*insertNodeKey)( sdrMtx        * aMtx,
                             stndrHeader   * aIndexHeader,
                             sdpPhyPageHdr * aCurrPage,
                             SShort          aSlotSeq,
                             UInt            aNodeKeyValueLength,
                             void          * aBuildKey );
} stndrReqFunc;

stndrReqFunc gCallbackFuncs4Build[2] =
{
    {
        stndrBUBuild::getCPFromLBuildKey,
        stndrBUBuild::compareLBuildKeyAndCP,
        stndrBUBuild::getLBuildKeyLength,
        stndrBUBuild::insertLBuildKey,
        stndrBUBuild::compareLBuildKeyAndLBuildKey,
        stndrBUBuild::checkSortLBuildKey,
        stndrBUBuild::getLNodeKeyLength,
        stndrBUBuild::getFreeSize4EmptyLNode,
        stndrBUBuild::insertLNodeKey,
    },
    {
        stndrBUBuild::getCPFromIBuildKey,
        stndrBUBuild::compareIBuildKeyAndCP,
        stndrBUBuild::getIBuildKeyLength,
        stndrBUBuild::insertIBuildKey,
        stndrBUBuild::compareIBuildKeyAndIBuildKey,
        stndrBUBuild::checkSortIBuildKey,
        stndrBUBuild::getINodeKeyLength,
        stndrBUBuild::getFreeSize4EmptyINode,
        stndrBUBuild::insertINodeKey,
    }
};


static UInt gMtxDLogType = SM_DLOG_ATTR_DEFAULT;


stndrBUBuild::stndrBUBuild() : idtBaseThread()
{
}


stndrBUBuild::~stndrBUBuild()
{
}


UInt stndrBUBuild::getMinimumBuildKeyLength( UInt aBuildKeyValueLength )
{
    UInt    sLBuildKeyLength;
    UInt    sIBuildKeyLength;

    sLBuildKeyLength = ID_SIZEOF(stndrLBuildKey);
    sIBuildKeyLength = ID_SIZEOF(stndrIBuildKey);

    if( sLBuildKeyLength > sIBuildKeyLength )
    {
        return sIBuildKeyLength + aBuildKeyValueLength;
    }
    else
    {
        return sLBuildKeyLength + aBuildKeyValueLength;
    }
}


UInt stndrBUBuild::getAvailablePageSize()
{
    UInt    sAvailablePageSize;

    sAvailablePageSize  = sdpPhyPage::getEmptyPageFreeSize();
    sAvailablePageSize -= idlOS::align8( (UInt)ID_SIZEOF(stndrNodeHdr) );
    sAvailablePageSize -= ID_SIZEOF(sdpSlotDirHdr);

    return sAvailablePageSize;
}


IDE_RC stndrBUBuild::insertLNodeKey( sdrMtx        * aMtx,
                                     stndrHeader   * aIndexHeader,
                                     sdpPhyPageHdr * aCurrPage,
                                     SShort          aSlotSeq,
                                     UInt            aNodeKeyValueLength,
                                     void          * aBuildKey )
{
    stndrKeyInfo     sNodeKeyInfo;
    stndrLBuildKey * sLBuildKey;
    smSCN            sCreateSCN;

    IDE_ASSERT( aIndexHeader != NULL );

    sLBuildKey = (stndrLBuildKey*)aBuildKey;

    STNDR_LBUILDKEY_TO_NODEKEYINFO( sLBuildKey, sNodeKeyInfo );
    SM_INIT_SCN( &sCreateSCN );

    IDE_TEST( stndrRTree::insertLKey( aMtx,
                                      aIndexHeader,
                                      aCurrPage,
                                      aSlotSeq,
                                      SDN_CTS_INFINITE,
                                      &sCreateSCN,
                                      STNDR_KEY_TB_CTS,
                                      &sNodeKeyInfo,
                                      aNodeKeyValueLength,
                                      ID_FALSE, // no logging
                                      NULL )    // key offset
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::insertINodeKey( sdrMtx        * aMtx,
                                     stndrHeader   * aIndexHeader,
                                     sdpPhyPageHdr * aCurrPage,
                                     SShort          aSlotSeq,
                                     UInt            aNodeKeyValueLength,
                                     void          * aBuildKey )
{
    stndrKeyInfo     sNodeKeyInfo;
    stndrIBuildKey * sIBuildKey;
    smSCN            sCreateSCN;

    IDE_ASSERT( aIndexHeader != NULL );

    sIBuildKey = (stndrIBuildKey*)aBuildKey;

    STNDR_IBUILDKEY_TO_NODEKEYINFO( sIBuildKey, sNodeKeyInfo );
    SM_INIT_SCN( &sCreateSCN );

    IDE_TEST( stndrRTree::insertIKey( aMtx,
                                      aIndexHeader,
                                      aCurrPage,
                                      aSlotSeq,
                                      &sNodeKeyInfo,
                                      aNodeKeyValueLength,
                                      sIBuildKey->mInternalKey.mChildPID,
                                      ID_FALSE ) // no logging
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


UInt stndrBUBuild::getFreeSize4EmptyLNode( stndrHeader * aIndexHeader )
{
    SShort    sCTSCount;
    UInt      sFreeSize4EmptyNode;
    UInt      sAvailablePageSize;

    IDE_ASSERT( aIndexHeader != NULL );

    sCTSCount = aIndexHeader->mSdnHeader.mSegmentDesc.mSegHandle.mSegAttr.mInitTrans;

    sAvailablePageSize = getAvailablePageSize();

    sFreeSize4EmptyNode = sAvailablePageSize
                          - idlOS::align8( ID_SIZEOF(sdnCTLayerHdr) )
                          - ID_SIZEOF(sdnCTS) * sCTSCount;

    return sFreeSize4EmptyNode;
}



UInt stndrBUBuild::getFreeSize4EmptyINode( stndrHeader * /* aIndexHeader */ )
{
    UInt    sFreeSize4EmptyNode;

    sFreeSize4EmptyNode = getAvailablePageSize();

    return sFreeSize4EmptyNode;
}


/* ------------------------------------------------
 * check sorted block
 * ----------------------------------------------*/
idBool stndrBUBuild::checkSortLBuildKey( UInt             aHead,
                                         UInt             aTail,
                                         UChar         ** aBuildKeyMap,
                                         stndrSortType    aSortType )
{
    UInt              i;
    idBool            sRet = ID_TRUE;
    stndrLBuildKey ** sLBuildKeyMap;

    IDE_ASSERT( aBuildKeyMap != NULL );

    sLBuildKeyMap = (stndrLBuildKey**)aBuildKeyMap;

    if( aSortType == STNDR_SORT_X )
    {
        for( i = aHead + 1; i <= aTail; i++ )
        {
            if( sLBuildKeyMap[ i-1 ]->mCenterPoint.mPointX > sLBuildKeyMap[ i ]->mCenterPoint.mPointX )
            {
                sRet = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        for( i = aHead + 1; i <= aTail; i++ )
        {
            if( sLBuildKeyMap[ i-1 ]->mCenterPoint.mPointY > sLBuildKeyMap[ i ]->mCenterPoint.mPointY )
            {
                sRet = ID_FALSE;
                break;
            }
        }
    }

    return sRet;
}


/* ------------------------------------------------
 * check sorted block
 * ----------------------------------------------*/
idBool stndrBUBuild::checkSortIBuildKey( UInt             aHead,
                                         UInt             aTail,
                                         UChar         ** aBuildKeyMap,
                                         stndrSortType    aSortType )
{
    UInt              i;
    idBool            sRet = ID_TRUE;
    stndrIBuildKey ** sIBuildKeyMap;

    IDE_ASSERT( aBuildKeyMap != NULL );

    sIBuildKeyMap = (stndrIBuildKey**)aBuildKeyMap;

    if( aSortType == STNDR_SORT_X )
    {
        for( i = aHead + 1; i <= aTail; i++ )
        {
            if( sIBuildKeyMap[ i-1 ]->mCenterPoint.mPointX > sIBuildKeyMap[ i ]->mCenterPoint.mPointX )
            {
                sRet = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        for( i = aHead + 1; i <= aTail; i++ )
        {
            if( sIBuildKeyMap[ i-1 ]->mCenterPoint.mPointY > sIBuildKeyMap[ i ]->mCenterPoint.mPointY )
            {
                sRet = ID_FALSE;
                break;
            }
        }
    }

    return sRet;
}


UInt stndrBUBuild::getLNodeKeyLength( UInt aNodeKeyValueLength )
{
    return aNodeKeyValueLength + ID_SIZEOF(stndrLKey);
}


UInt stndrBUBuild::getINodeKeyLength( UInt aNodeKeyValueLength )
{
    return aNodeKeyValueLength + ID_SIZEOF(stndrIKey);
}


UInt stndrBUBuild::getLBuildKeyLength( UInt aBuildKeyValueLength )
{
    return aBuildKeyValueLength + ID_SIZEOF(stndrLBuildKey);
}


UInt stndrBUBuild::getIBuildKeyLength( UInt aBuildKeyValueLength )
{
    return aBuildKeyValueLength + ID_SIZEOF(stndrIBuildKey);
}


SInt stndrBUBuild::compareLBuildKeyAndLBuildKey( void          * aBuildKey1,
                                                 void          * aBuildKey2,
                                                 stndrSortType   aSortType )
{
    stndrLBuildKey * sBuildKey1;
    stndrLBuildKey * sBuildKey2;

    IDE_ASSERT( aBuildKey1 != NULL );
    IDE_ASSERT( aBuildKey2 != NULL );

    sBuildKey1 = (stndrLBuildKey*)aBuildKey1;
    sBuildKey2 = (stndrLBuildKey*)aBuildKey2;

    if( aSortType == STNDR_SORT_X )
    {
        if( sBuildKey1->mCenterPoint.mPointX > sBuildKey2->mCenterPoint.mPointX )
        {
            return 1;
        }
        else if( sBuildKey1->mCenterPoint.mPointX < sBuildKey2->mCenterPoint.mPointX )
        {
            return -1;
        }
        else
        {
            if( sBuildKey1->mCenterPoint.mPointY > sBuildKey2->mCenterPoint.mPointY )
            {
                return 1;
            }
            else if( sBuildKey1->mCenterPoint.mPointY < sBuildKey2->mCenterPoint.mPointY )
            {
                return -1;
            }

            return 0;
        }
    }
    else
    {
        if( sBuildKey1->mCenterPoint.mPointY > sBuildKey2->mCenterPoint.mPointY )
        {
            return 1;
        }
        else if( sBuildKey1->mCenterPoint.mPointY < sBuildKey2->mCenterPoint.mPointY )
        {
            return -1;
        }
        else
        {
            if( sBuildKey1->mCenterPoint.mPointX > sBuildKey2->mCenterPoint.mPointX )
            {
                return 1;
            }
            else if( sBuildKey1->mCenterPoint.mPointX < sBuildKey2->mCenterPoint.mPointX )
            {
                return -1;
            }

            return 0;
        }
    }
}


SInt stndrBUBuild::compareIBuildKeyAndIBuildKey( void          * aBuildKey1,
                                                 void          * aBuildKey2,
                                                 stndrSortType   aSortType )
{
    stndrIBuildKey * sBuildKey1;
    stndrIBuildKey * sBuildKey2;

    IDE_ASSERT( aBuildKey1 != NULL );
    IDE_ASSERT( aBuildKey2 != NULL );

    sBuildKey1 = (stndrIBuildKey*)aBuildKey1;
    sBuildKey2 = (stndrIBuildKey*)aBuildKey2;

    if( aSortType == STNDR_SORT_X )
    {
        if( sBuildKey1->mCenterPoint.mPointX > sBuildKey2->mCenterPoint.mPointX )
        {
            return 1;
        }
        else if( sBuildKey1->mCenterPoint.mPointX < sBuildKey2->mCenterPoint.mPointX )
        {
            return -1;
        }
        else
        {
            if( sBuildKey1->mCenterPoint.mPointY > sBuildKey2->mCenterPoint.mPointY )
            {
                return 1;
            }
            else if( sBuildKey1->mCenterPoint.mPointY < sBuildKey2->mCenterPoint.mPointY )
            {
                return -1;
            }

            return 0;
        }
    }
    else
    {
        if( sBuildKey1->mCenterPoint.mPointY > sBuildKey2->mCenterPoint.mPointY )
        {
            return 1;
        }
        else if( sBuildKey1->mCenterPoint.mPointY < sBuildKey2->mCenterPoint.mPointY )
        {
            return -1;
        }
        else
        {
            if( sBuildKey1->mCenterPoint.mPointX > sBuildKey2->mCenterPoint.mPointX )
            {
                return 1;
            }
            else if( sBuildKey1->mCenterPoint.mPointX < sBuildKey2->mCenterPoint.mPointX )
            {
                return -1;
            }

            return 0;
        }
    }
}


IDE_RC stndrBUBuild::getBuildKeyFromInfo( scPageID         aPageID,
                                          SShort           aSlotSeq,
                                          sdpPhyPageHdr  * aPage,
                                          void          ** aBuildKey )
{
    UChar  * sSlotDirPtr;
    SShort   sSlotSeq = aSlotSeq;

    IDE_ASSERT( aPageID   != SC_NULL_PID );
    IDE_ASSERT( aBuildKey != NULL );
    IDE_ASSERT( aSlotSeq  >= 0 );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aPage );

    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                       sSlotSeq,
                                                       (UChar**)aBuildKey )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void stndrBUBuild::insertLBuildKey( sdpPhyPageHdr * aNode,
                                    SShort          aSlotSeq,
                                    void          * aBuildKey )
{
    void     * sDestBuildKey;
    UShort     sAllowedSize;
    scOffset   sBuildKeyOffset;
    UInt       sBuildKeyLength;

    IDE_ASSERT( aBuildKey != NULL );

    sBuildKeyLength = ID_SIZEOF(stndrLBuildKey) + ID_SIZEOF(stdMBR);

    IDE_ASSERT( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aNode,
                                       aSlotSeq,
                                       sBuildKeyLength,
                                       ID_TRUE,
                                       &sAllowedSize,
                                       (UChar**)&sDestBuildKey,
                                       &sBuildKeyOffset,
                                       1 )
                == IDE_SUCCESS );

    IDE_ASSERT( sAllowedSize >= sBuildKeyLength );

    idlOS::memcpy( sDestBuildKey, aBuildKey, sBuildKeyLength );
}


void stndrBUBuild::insertIBuildKey( sdpPhyPageHdr * aNode,
                                    SShort          aSlotSeq,
                                    void          * aBuildKey )
{
    void     * sDestBuildKey;
    UShort     sAllowedSize;
    scOffset   sBuildKeyOffset;
    UInt       sBuildKeyLength;

    IDE_ASSERT( aBuildKey != NULL );

    sBuildKeyLength = ID_SIZEOF(stndrIBuildKey) + ID_SIZEOF(stdMBR);

    IDE_ASSERT( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aNode,
                                       aSlotSeq,
                                       sBuildKeyLength,
                                       ID_TRUE,
                                       &sAllowedSize,
                                       (UChar**)&sDestBuildKey,
                                       &sBuildKeyOffset,
                                       1 )
                == IDE_SUCCESS );

    IDE_ASSERT( sAllowedSize >= sBuildKeyLength );

    idlOS::memcpy( sDestBuildKey, aBuildKey, sBuildKeyLength );
}


SInt stndrBUBuild::compareLBuildKeyAndCP( stndrCenterPoint   aCenterPoint,
                                          void             * aBuildKey,
                                          stndrSortType      aSortType )
{
    stndrLBuildKey * sBuildKey;

    IDE_ASSERT( aBuildKey != NULL );

    sBuildKey = (stndrLBuildKey*)aBuildKey;

    if( aSortType == STNDR_SORT_X )
    {
        if( aCenterPoint.mPointX > sBuildKey->mCenterPoint.mPointX )
        {
            return 1;
        }
        else if( aCenterPoint.mPointX < sBuildKey->mCenterPoint.mPointX )
        {
            return -1;
        }
        else
        {
            if( aCenterPoint.mPointY > sBuildKey->mCenterPoint.mPointY )
            {
                return 1;
            }
            else if( aCenterPoint.mPointY < sBuildKey->mCenterPoint.mPointY )
            {
                return -1;
            }

            return 0;
        }
    }
    else
    {
        if( aCenterPoint.mPointY > sBuildKey->mCenterPoint.mPointY )
        {
            return 1;
        }
        else if( aCenterPoint.mPointY < sBuildKey->mCenterPoint.mPointY )
        {
            return -1;
        }
        else
        {
            if( aCenterPoint.mPointX > sBuildKey->mCenterPoint.mPointX )
            {
                return 1;
            }
            else if( aCenterPoint.mPointX < sBuildKey->mCenterPoint.mPointX )
            {
                return -1;
            }

            return 0;
        }
    }
}


SInt stndrBUBuild::compareIBuildKeyAndCP( stndrCenterPoint   aCenterPoint,
                                          void             * aBuildKey,
                                          stndrSortType      aSortType )
{
    stndrIBuildKey * sBuildKey;

    IDE_ASSERT( aBuildKey != NULL );

    sBuildKey = (stndrIBuildKey*)aBuildKey;

    if( aSortType == STNDR_SORT_X )
    {
        if( aCenterPoint.mPointX > sBuildKey->mCenterPoint.mPointX )
        {
            return 1;
        }
        else if( aCenterPoint.mPointX < sBuildKey->mCenterPoint.mPointX )
        {
            return -1;
        }
        else
        {
            if( aCenterPoint.mPointY > sBuildKey->mCenterPoint.mPointY )
            {
                return 1;
            }
            else if( aCenterPoint.mPointY < sBuildKey->mCenterPoint.mPointY )
            {
                return -1;
            }

            return 0;
        }
    }
    else
    {
        if( aCenterPoint.mPointY > sBuildKey->mCenterPoint.mPointY )
        {
            return 1;
        }
        else if( aCenterPoint.mPointY < sBuildKey->mCenterPoint.mPointY )
        {
            return -1;
        }
        else
        {
            if( aCenterPoint.mPointX > sBuildKey->mCenterPoint.mPointX )
            {
                return 1;
            }
            else if( aCenterPoint.mPointX < sBuildKey->mCenterPoint.mPointX )
            {
                return -1;
            }

            return 0;
        }
    }
}


void stndrBUBuild::getCPFromLBuildKey( UChar            ** aBuildKeyMap,
                                       UInt                aPos,
                                       stndrCenterPoint  * aCenterPoint )
{
    stndrLBuildKey ** sBuildKeyMap;

    IDE_ASSERT( aBuildKeyMap != NULL );

    sBuildKeyMap = (stndrLBuildKey**)aBuildKeyMap;

    idlOS::memcpy( aCenterPoint, &sBuildKeyMap[aPos]->mCenterPoint, ID_SIZEOF(stndrCenterPoint) );
}


void stndrBUBuild::getCPFromIBuildKey( UChar            ** aBuildKeyMap,
                                       UInt                aPos,
                                       stndrCenterPoint  * aCenterPoint )
{
    stndrIBuildKey ** sBuildKeyMap;

    IDE_ASSERT( aBuildKeyMap != NULL );

    sBuildKeyMap = (stndrIBuildKey**)aBuildKeyMap;

    idlOS::memcpy( aCenterPoint, &sBuildKeyMap[aPos]->mCenterPoint, ID_SIZEOF(stndrCenterPoint) );
}


/* ------------------------------------------------
 * KeyMap에서 swap
 * ----------------------------------------------*/
void stndrBUBuild::swapBuildKeyMap( UChar ** aBuildKeyMap,
                                    UInt     aPos1,
                                    UInt     aPos2 )
{
    UChar * sBuildKey;

    IDE_ASSERT( aBuildKeyMap != NULL );

    sBuildKey           = aBuildKeyMap[aPos1];
    aBuildKeyMap[aPos1] = aBuildKeyMap[aPos2];
    aBuildKeyMap[aPos2] = sBuildKey;

    return;
}


/* ------------------------------------------------
 * 쓰레드 작업 시작 루틴
 * ----------------------------------------------*/
IDE_RC stndrBUBuild::threadRun( UInt            aPhase,
                                UInt            aThreadCnt,
                                stndrBUBuild   *aThreads )
{
    UInt             i;

    // Working Thread 실행
    for( i = 0; i < aThreadCnt; i++ )
    {
        aThreads[i].mPhase = aPhase;

        IDE_TEST( aThreads[i].start() != IDE_SUCCESS );
    }

    // Working Thread 종료 대기
    for( i = 0; i < aThreadCnt; i++ )
    {
        IDE_TEST(aThreads[i].join() != IDE_SUCCESS);
    }

    // Working Thread 수행 결과 확인
    for( i = 0; i < aThreadCnt; i++ )
    {
        if( aThreads[i].mIsSuccess == ID_FALSE )
        {
            IDE_TEST_RAISE( aThreads[i].getErrorCode() != 0, THREADS_ERR_RAISE )
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( THREADS_ERR_RAISE );
    {
        ideCopyErrorInfo( ideGetErrorMgr(),
                          aThreads[i].getErrorMgr() );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* ------------------------------------------------
 * Disk Index Build Main Routine
 *
 * 0. Working Thread Initialize
 * 1. Key Extraction   &
 *    In-Memory X Sort
 * 2. X Merge
 * 3. Make Partition
 * 4. In-Memory Y Sort &
 *    Y Merge
 * 5. Make Node (leaf or internal) &
 *    Get Node MBR                 &
 *    X Sort
 * 6. Goto 2 when Node count is not one.
 * ----------------------------------------------*/
IDE_RC stndrBUBuild::main( idvSQL          *aStatistics,
                           void            *aTrans,
                           smcTableHeader  *aTable,
                           smnIndexHeader  *aIndex,
                           UInt             aThreadCnt,
                           UInt             aTotalSortAreaSize,
                           UInt             aTotalMergePageCnt,
                           idBool           aIsNeedValidation,
                           UInt             aBuildFlag )
{
    UInt                   i;
    UInt                   sState = 0;
    stndrBUBuild         * sThreads = NULL;
    smuQueueMgr            sPIDBlkQueue;
    UInt                   sSortAreaSize;
    UInt                   sMergePageCnt;
    UInt                   sMaxKeyMapCount;
    UInt                   sThrState = 0;
    UInt                   sAvailablePageSize;
    UInt                   sBuildKeyValueLength;
    UInt                   sNodeCount;
    stndrStatistic         sIndexStat;       // BUG-18201
    scPageID               sLastNodePID = SD_NULL_PID;
    UShort                 sHeight = 0;
    SLong                  sKeyCount = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aTotalSortAreaSize > 0 );
    IDE_DASSERT( aTotalMergePageCnt > 0 );
    IDE_DASSERT( aThreadCnt > 0 );

    sMergePageCnt      = aTotalMergePageCnt / aThreadCnt;
    sSortAreaSize      = aTotalSortAreaSize / aThreadCnt;
    sAvailablePageSize = getAvailablePageSize();

    // 통계치 정보
    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF(stndrStatistic) );

    sBuildKeyValueLength = stndrRTree::getKeyValueLength();
    sMaxKeyMapCount      = sSortAreaSize / getMinimumBuildKeyLength( sBuildKeyValueLength );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ST_TEMP, 1,
                                 ID_SIZEOF(stndrBUBuild) * aThreadCnt,
                                 (void**)&sThreads )
              != IDE_SUCCESS );
    sState = 1;

    ideLog::log( IDE_SM_0, "\
===========================================\n\
 [IDX_CRE]    BUILD INDEX                  \n\
              NAME : %s                    \n\
              ID   : %u                    \n\
===========================================\n\
    1. Key Extraction   &                  \n\
       In-Memory X Sort                    \n\
    2. X Merge                             \n\
    3. Make Partition                      \n\
    4. In-Memory Y Sort &                  \n\
       Y Merge                             \n\
    5. Make Node (leaf or internal) &      \n\
       Get Node MBR                 &      \n\
       X Sort                              \n\
    6. Goto 2 when Node count is not one.  \n\
===========================================\n\
          BUILD_THREAD_COUNT     : %d      \n\
          TOTAL_SORT_AREA_SIZE   : %d      \n\
          SORT_AREA_SIZE         : %d      \n\
          MERGE_PAGE_COUNT       : %d      \n\
          MAX_KEYMAP_COUNT       : %d      \n\
          KEY_VALUE_LENGTH       : %d      \n\
===========================================\n",
                 aIndex->mName,
                 aIndex->mId,
                 aThreadCnt,
                 aTotalSortAreaSize,
                 sSortAreaSize,
                 sMergePageCnt,
                 sMaxKeyMapCount,
                 sBuildKeyValueLength );

    for( i = 0; i < aThreadCnt; i++)
    {
        new (sThreads + i) stndrBUBuild;

        IDE_TEST( sThreads[i].initialize(
                      aThreadCnt,
                      i,
                      aTable,
                      aIndex,
                      sMergePageCnt,
                      sAvailablePageSize,
                      sSortAreaSize,
                      sMaxKeyMapCount,
                      aIsNeedValidation,
                      aBuildFlag,
                      aStatistics )
                  != IDE_SUCCESS );
        sThrState++;
    }

    IDE_TEST( threadRun( STNDR_EXTRACT_AND_SORT,
                         aThreadCnt,
                         sThreads )
              != IDE_SUCCESS );

    for( i = 0; i < aThreadCnt; i++ )
    {
        sKeyCount += sThreads[i].mPropagatedKeyCount;
    }

    IDE_TEST( threadRun( STNDR_MERGE_RUN,
                         aThreadCnt,
                         sThreads )
              != IDE_SUCCESS );
    
    IDU_FIT_POINT( "2.PROJ-1591@stndrBUBuild::main" );

    IDE_TEST( sThreads[0].collectFreePage( sThreads, aThreadCnt ) != IDE_SUCCESS );

    IDE_TEST( sThreads[0].collectRun( sThreads, aThreadCnt ) != IDE_SUCCESS );

    // BUG-29315: 런의 갯수가 0일 경우 비정상 종료하는 문제
    IDE_TEST_RAISE( sThreads[0].mRunQueue.getQueueLength() == 0, SKIP_MERGE );

    sThreads[0].makePartition( STNDR_NODE_LKEY );

    IDE_TEST( sThreads[0].sortY( aTotalMergePageCnt,
                                 STNDR_NODE_LKEY )
              != IDE_SUCCESS );

    IDE_TEST( sThreads[0].makeNodeAndSort( sHeight,
                                           &sLastNodePID,
                                           &sNodeCount,
                                           STNDR_NODE_LKEY )
              != IDE_SUCCESS );

    while( sThrState > 1 )
    {
        IDE_TEST( sThreads[--sThrState].destroy() != IDE_SUCCESS );
    }

    if( sThreads[0].mKeyBuffer != NULL )
    {
        IDE_TEST( iduMemMgr::free( sThreads[0].mKeyBuffer ) != IDE_SUCCESS );
        sThreads[0].mKeyBuffer = NULL;
    }

    if( sThreads[0].mKeyMap != NULL )
    {
        IDE_TEST( iduMemMgr::free( sThreads[0].mKeyMap ) != IDE_SUCCESS );
        sThreads[0].mKeyMap = NULL;
    }

    sThreads[0].mMaxKeyMapCount = aTotalSortAreaSize
                                  / getMinimumBuildKeyLength( sBuildKeyValueLength );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ST_TEMP,
                                 sThreads[0].mMaxKeyMapCount, 
                                 ID_SIZEOF(UChar*),
                                 (void**)&sThreads[0].mKeyMap )
              != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ST_TEMP, aTotalSortAreaSize, 1,
                                 (void**)&sThreads[0].mKeyBuffer )
              != IDE_SUCCESS );

    // nodeCount 가 1 이면 root node 이므로 중단한다.
    while( sNodeCount > 1 )
    {
        sHeight++;

        IDE_TEST( sThreads[0].merge( sThreads[0].mMergePageCount,
                                     STNDR_NODE_IKEY,
                                     STNDR_SORT_X )
                  != IDE_SUCCESS );

        sThreads[0].makePartition( STNDR_NODE_IKEY );

        IDE_TEST( sThreads[0].sortY( sThreads[0].mMergePageCount,
                                     STNDR_NODE_IKEY )
                  != IDE_SUCCESS );

        IDE_TEST( sThreads[0].makeNodeAndSort( sHeight,
                                               &sLastNodePID,
                                               &sNodeCount,
                                               STNDR_NODE_IKEY )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_MERGE );

    ((stndrHeader*)sThreads[0].mIndex->mHeader)->mRootNode = sLastNodePID;

    IDE_TEST( sThreads[0].removeFreePage() != IDE_SUCCESS );

    // BUG-18201 : Memory/Disk Index 통계치
    STNDR_ADD_STATISTIC( &(((stndrHeader*)aIndex->mHeader)->mDMLStat),
                         &sIndexStat );

    // BUG-29290: Disk R-Tree Bottom-Up Build시 TreeMBR이 갱신되진 않는 문제
    IDE_TEST( updateStatistics( aStatistics,
                                NULL, // aMtx
                                aIndex,
                                sKeyCount,
                                0 ) // aNumDist
              != IDE_SUCCESS );

    IDE_TEST( sThreads[0].destroy() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sThreads ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    for( i = 0; i < sThrState; i++ )
    {
        (void)sThreads[i].destroy();
    }

    if( sState == 1 )
    {
        (void)iduMemMgr::free( sThreads );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * Index build 쓰레드 초기화
 * ----------------------------------------------*/
IDE_RC stndrBUBuild::initialize( UInt             aTotalThreadCnt,
                                 UInt             aID,
                                 smcTableHeader * aTable,
                                 smnIndexHeader * aIndex,
                                 UInt             aMergePageCnt,
                                 UInt             aAvailablePageSize,
                                 UInt             aSortAreaSize,
                                 UInt             aMaxKeyMapCount,
                                 idBool           aIsNeedValidation,
                                 UInt             aBuildFlag,
                                 idvSQL*          aStatistics )
{
    UInt sState = 0;

    mTotalThreadCnt     = aTotalThreadCnt;
    mID                 = aID;
    mTable              = aTable;
    mIndex              = aIndex;
    mMergePageCount     = aMergePageCnt;
    mKeyBufferSize      = aSortAreaSize;
    mLeftSizeThreshold  = aAvailablePageSize;
    mMaxKeyMapCount     = aMaxKeyMapCount;
    mIsNeedValidation   = aIsNeedValidation;
    mStatistics         = aStatistics;
    mPhase              = 0;
    mFinished           = ID_FALSE;
    mBuildFlag          = aBuildFlag;
    mErrorCode          = 0;
    mIsSuccess          = ID_TRUE;
    mFreePageCnt        = 0;
    mPropagatedKeyCount = 0;
    mLoggingMode        = ((aBuildFlag & SMI_INDEX_BUILD_LOGGING_MASK) ==
                          SMI_INDEX_BUILD_LOGGING )? SDR_MTX_LOGGING :
                          SDR_MTX_NOLOGGING;
    mIsForceMode        = ((aBuildFlag & SMI_INDEX_BUILD_FORCE_MASK) ==
                          SMI_INDEX_BUILD_FORCE )? ID_TRUE : ID_FALSE;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ST_TEMP,
                                 mMaxKeyMapCount, 
                                 ID_SIZEOF(UChar*),
                                 (void**)&mKeyMap )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ST_TEMP, mKeyBufferSize, 1,
                                 (void**)&mKeyBuffer )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mFreePage.initialize( IDU_MEM_ST_TEMP,
                                    ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( mRunQueue.initialize( IDU_MEM_ST_TEMP,
                                    ID_SIZEOF(stndrSortedBlk) )
              != IDE_SUCCESS);
    sState = 4;

    IDE_TEST( mPartitionQueue.initialize( IDU_MEM_ST_TEMP,
                                          ID_SIZEOF(stndrSortedBlk) )
              != IDE_SUCCESS);
    sState = 5;

    mTrans = NULL;
    IDE_TEST(smLayerCallback::allocNestedTrans( &mTrans )
             != IDE_SUCCESS);
    sState = 6;

    // fix BUG-27403 - for Stack overflow
    IDE_TEST( mSortStack.initialize( IDU_MEM_ST_TEMP, ID_SIZEOF(smnSortStack) )
              != IDE_SUCCESS);
    sState = 7;

    IDE_TEST(smLayerCallback::beginTrans(mTrans, 
                                         (SMI_TRANSACTION_REPL_NONE |
                                          SMI_COMMIT_WRITE_NOWAIT), 
                                         NULL)
             != IDE_SUCCESS);   

    ideLog::log( IDE_SM_0, "\
===========================================\n\
 [IDX_CRE] 0. initialize                   \n\
-------------------------------------------\n\
    -- KEYMAP      = %d                    \n\
    -- BUFFER      = %d                    \n\
===========================================\n",
                 mMaxKeyMapCount * ID_SIZEOF(stndrLKey*),
                 mKeyBufferSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 7:
            (void)mSortStack.destroy();
        case 6:
            (void)smLayerCallback::freeTrans( mTrans );
            mTrans = NULL;
        case 5:
            (void)mPartitionQueue.destroy();
        case 4:
            (void)mRunQueue.destroy();
        case 3:
            (void)mFreePage.destroy();
        case 2:
            (void)iduMemMgr::free( mKeyBuffer );
            mKeyBuffer = NULL;
        case 1:
            (void)iduMemMgr::free( mKeyMap );
            mKeyMap = NULL;
        default:
            break;
    }
            
    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * buffer flush 쓰레드 해제
 * ----------------------------------------------*/
IDE_RC stndrBUBuild::destroy()
{
    UInt sStatus = 0;

    IDE_TEST( mRunQueue.destroy()       != IDE_SUCCESS );
    IDE_TEST( mPartitionQueue.destroy() != IDE_SUCCESS );
    IDE_TEST( mFreePage.destroy()       != IDE_SUCCESS );
    IDE_TEST( mSortStack.destroy()      != IDE_SUCCESS );

    if( mTrans != NULL )
    {
        sStatus = 1;
        IDE_TEST( smLayerCallback::commitTrans(mTrans) != IDE_SUCCESS );
        sStatus = 0;
        IDE_TEST( smLayerCallback::freeTrans(mTrans) != IDE_SUCCESS );
        mTrans = NULL;
    }

    if( mKeyBuffer != NULL )
    {
        IDE_TEST( iduMemMgr::free( mKeyBuffer ) != IDE_SUCCESS );
        mKeyBuffer = NULL;
    }
    if( mKeyMap != NULL )
    {
        IDE_TEST( iduMemMgr::free( mKeyMap ) != IDE_SUCCESS );
        mKeyMap = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sStatus == 1)
    {
        (void)smLayerCallback::freeTrans( mTrans );
        mTrans = NULL;
    }
    if( mKeyBuffer != NULL )
    {
        (void)iduMemMgr::free( mKeyBuffer );
        mKeyBuffer = NULL;
    }
    if( mKeyMap != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( mKeyMap ) == IDE_SUCCESS );
        mKeyMap = NULL;
    }

    return IDE_FAILURE;
}


/* ------------------------------------------------
 * Description :
 *
 * 쓰레드 메인 실행 루틴
 *
 * ----------------------------------------------*/
void stndrBUBuild::run()
{
    stndrStatistic     sIndexStat;

    // BUG-18201
    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF(stndrStatistic) );

    switch(mPhase)
    {
        case STNDR_EXTRACT_AND_SORT:
            /* Phase 1. Key Extraction & In-Memory Sort */
            IDE_TEST( extractNSort() != IDE_SUCCESS );
            break;

        case STNDR_MERGE_RUN:
            /* Phase 2. merge sorted run */
            IDE_TEST( merge( mMergePageCount,
                             STNDR_NODE_LKEY,
                             STNDR_SORT_X )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_ASSERT(0);
    }

    return;

    IDE_EXCEPTION_END;

    mErrorCode = ideGetErrorCode();
    ideCopyErrorInfo( &mErrorMgr, ideGetErrorMgr() );

    return;
}

/* ------------------------------------------------
 * Phase 1. Key Extraction & In-Memory Sort
 * ----------------------------------------------*/
IDE_RC stndrBUBuild::extractNSort()
{
    UInt                 sState = 0;
    scSpaceID            sTBSID;
    UChar              * sPage;
    UChar              * sSlotDirPtr;
    UChar              * sSlot;
    SInt                 sSlotCount;
    UInt                 i, j;
    UInt                 sNextPos = 0;
    sdSID                sRowSID;
    smSCN                sInfiniteSCN;
    idBool               sIsSuccess;
    sdrMtx               sMtx;
    UInt                 sLeftPos = 0;
    UInt                 sLeftCnt = 0;
    UInt                 sUsedBufferSize = 0;
    UInt                 sBuildKeyValueLength = 0;
    UInt                 sBuildKeyLength = 0;
    UInt                 sInsertedBuildKeyCnt = 0;
    scPageID             sCurPageID;
    sdpPhyPageHdr      * sPageHdr;
    ULong                sKeyValueBuf[ ID_SIZEOF(stdGeometryHeader) ];
    idBool               sIsRowDeleted;
    sdbMPRMgr            sMPRMgr;
    UChar                sBuffer4Compaction[ SD_PAGE_SIZE ]; //짜투리를 저장해두는 공간
    idBool               sIsPageLatchReleased = ID_TRUE;
    scPageID             sRowPID;
    scSlotNum            sRowSlotNum;
    idBool               sIsIndexable;
    sdbMPRFilter4PScan   sFilter4Scan;

    SM_SET_SCN_INFINITE( &sInfiniteSCN );

    sBuildKeyValueLength = stndrRTree::getKeyValueLength();
    sBuildKeyLength      = gCallbackFuncs4Build[ STNDR_NODE_LKEY ].getBuildKeyLength( sBuildKeyValueLength );
 
    sTBSID = mTable->mSpaceID;

    IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                   &sMtx,
                                   (void*)NULL,
                                   SDR_MTX_NOLOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sMPRMgr.initialize(
                  mStatistics,
                  sTBSID,
                  sdpSegDescMgr::getSegHandle( &mTable->mFixed.mDRDB ),
                  sdbMPRMgr::filter4PScan )
              != IDE_SUCCESS );
    sFilter4Scan.mThreadCnt = mTotalThreadCnt;
    sFilter4Scan.mThreadID  = mID;
    sState = 2;

    IDE_TEST( sMPRMgr.beforeFst() != IDE_SUCCESS );

    // table에 alloc된 extent list를 순회하며 작업
    while (1)              // 1
    {
        IDE_TEST( sMPRMgr.getNxtPageID( (void*)&sFilter4Scan,
                                        &sCurPageID )
                  != IDE_SUCCESS );

        if( sCurPageID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( sdbBufferMgr::getPage( mStatistics,
                                         sTBSID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_MULTI_PAGE_READ,
                                         &sPage,
                                         &sIsSuccess )
                  != IDE_SUCCESS );
        IDE_ASSERT( sPage != NULL );

        sIsPageLatchReleased = ID_FALSE;

        sPageHdr = sdpPhyPage::getHdr( sPage );

        if( sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_DATA )
        {
            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sPage );
            sSlotCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

            // page에 alloc된 slot 순회
            for( i = 0; i < (UInt)sSlotCount; i++ )       // 4
            {
                if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, i)
                    == ID_TRUE )
                {
                    continue;
                }
              
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(sSlotDirPtr, 
                                                                  i,
                                                                  &sSlot)
                          != IDE_SUCCESS );
                IDE_DASSERT(sSlot != NULL);

                if( sdcRow::isHeadRowPiece(sSlot) != ID_TRUE )
                {
                    continue;
                }
                if( sdcRow::isDeleted(sSlot) == ID_TRUE ) 
                {
                    continue;
                }
                
                sRowSID = SD_MAKE_SID( sPageHdr->mPageID, i );

                IDE_TEST( stndrRTree::makeKeyValueFromRow(
                                              mStatistics,
                                              NULL, /* aMtx */
                                              NULL, /* aSP */
                                              mTrans,
                                              mTable,
                                              mIndex,
                                              sSlot,
                                              SDB_MULTI_PAGE_READ,
                                              sTBSID,
                                              SMI_FETCH_VERSION_LAST,
                                              SD_NULL_RID, /* aTssRID */
                                              NULL,        /* aSCN */
                                              NULL,        /* aInfiniteSCN */
                                              (UChar*)sKeyValueBuf,
                                              &sIsRowDeleted,
                                              &sIsPageLatchReleased )
                          != IDE_SUCCESS );

                IDE_ASSERT( sIsRowDeleted == ID_FALSE );

                /* BUG-23319
                 * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
                /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
                 * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
                 * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
                 * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
                 * 상황에 따라 적절한 처리를 해야 한다. */
                if( sIsPageLatchReleased == ID_TRUE )
                {
                    /* BUG-25126
                     * [5.3.1 SD] Index Bottom-up Build 시 Page fetch시 서버사망!! */
                    IDE_TEST( sdbBufferMgr::getPage( mStatistics,
                                                     sTBSID,
                                                     sCurPageID,
                                                     SDB_S_LATCH,
                                                     SDB_WAIT_NORMAL,
                                                     SDB_MULTI_PAGE_READ,
                                                     &sPage,
                                                     &sIsSuccess )
                              != IDE_SUCCESS );
                    sIsPageLatchReleased = ID_FALSE;

                    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sPage );
                    sSlotCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

                    /* page latch가 풀린 사이에 다른 트랜잭션이
                     * 동일 페이지에 접근하여 변경 연산을 수행할 수 있다. */
                    if( sdpSlotDirectory::isUnusedSlotEntry( sSlotDirPtr, i )
                        == ID_TRUE )
                    {
                        continue;
                    }
                }

                stndrRTree::isIndexableRow( mIndex,
                                            (SChar*)sKeyValueBuf,
                                            &sIsIndexable );

                if( sIsIndexable == ID_FALSE )
                {
                    continue;
                }
                else
                {
                    /* do nothing */
                }

                // makePartition() 에서 사용하기 위해 상위 height 의 node 갯수 계산에 이용
                mPropagatedKeyCount++;

                // fix BUG-23525 InsertableKeyCnt 에 도달하면
                if( (sUsedBufferSize + sBuildKeyLength) > mKeyBufferSize )
                {
                    IDE_DASSERT( sInsertedBuildKeyCnt <= mMaxKeyMapCount );

                    // sort area의 key 정렬
                    IDE_TEST( quickSort( 0,
                                         sNextPos - 1,
                                         STNDR_NODE_LKEY,
                                         STNDR_SORT_X )
                              != IDE_SUCCESS );

                    // temp segment 에 내림
                    IDE_TEST( storeSortedRun( 0,
                                              sNextPos - 1,
                                              &sLeftPos,
                                              &sUsedBufferSize,
                                              STNDR_NODE_LKEY )
                              != IDE_SUCCESS );

                    sLeftCnt = sNextPos - sLeftPos;
                    sUsedBufferSize = 0;


                    if( sLeftCnt > 0 )
                    {
                        //KeyValue들을 맨 앞쪽으로 붙여넣기
                        for( j = 0; j < sLeftCnt; j++ )
                        {
                            swapBuildKeyMap( mKeyMap, j, (sLeftPos + j) );

                            // BUG-31024: Bottom-UP 빌드 시에 행이 걸리는 문제
                            //            Buffer Overflow로 인해 발생
                            IDE_ASSERT( (sUsedBufferSize + sBuildKeyLength)
                                        <= ID_SIZEOF(sBuffer4Compaction) );

                            /* fix BUG-23525 저장되는 키의 길이가 Varaibale하면 
                             * Fragmentation이 일어날 수 밖에 없고, 따라서 Compaction과정은 필수 */
                            idlOS::memcpy( &sBuffer4Compaction[ sUsedBufferSize ],
                                           mKeyMap[j],
                                           sBuildKeyLength );

                            mKeyMap[j] = &mKeyBuffer[ sUsedBufferSize ] ;
                            sUsedBufferSize += sBuildKeyLength;
                        }
                        idlOS::memcpy( mKeyBuffer, sBuffer4Compaction, sUsedBufferSize );

                        sInsertedBuildKeyCnt = sLeftCnt;
                    }
                    else
                    {
                        sInsertedBuildKeyCnt = 0;
                    }

                    sNextPos = sInsertedBuildKeyCnt;
                }

                IDE_DASSERT( sInsertedBuildKeyCnt < mMaxKeyMapCount );
                IDE_DASSERT( (sUsedBufferSize + sBuildKeyLength) <= mKeyBufferSize );

                // key extraction
                mKeyMap[sNextPos] = &mKeyBuffer[ sUsedBufferSize ];
                sRowPID           = SD_MAKE_PID( sRowSID );
                sRowSlotNum       = SD_MAKE_SLOTNUM( sRowSID );

                STNDR_WRITE_LBUILDKEY( sRowPID,
                                       sRowSlotNum,
                                       &(((stdGeometryHeader*)sKeyValueBuf)->mMbr),
                                       sBuildKeyValueLength,
                                       mKeyMap[sNextPos] );

                sUsedBufferSize += sBuildKeyLength;
                sInsertedBuildKeyCnt++;
                sNextPos++;
            }// for each slot                              // -4
        }

        // release s-latch
        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage( mStatistics,
                                             sPage )
                  != IDE_SUCCESS );
        sPage  = NULL;

        IDE_TEST( iduCheckSessionEvent( mStatistics )
                  != IDE_SUCCESS );
    }                                                     // -2

    sState = 1;
    IDE_TEST( sMPRMgr.destroy() != IDE_SUCCESS );

    // 작업 공간에 남아 있는 것들 처리
    if( sNextPos > 0 )
    {
        // in-memory sort
        IDE_TEST( quickSort( 0,
                             sNextPos - 1,
                             STNDR_NODE_LKEY,
                             STNDR_SORT_X )
                  != IDE_SUCCESS );
        // temp segment로 내린다.
        IDE_TEST( storeSortedRun( 0,
                                  sNextPos - 1,
                                  NULL,
                                  NULL,
                                  STNDR_NODE_LKEY )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    IDE_PUSH();

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_ASSERT( sPage != NULL );
        IDE_ASSERT( sdbBufferMgr::releasePage( mStatistics,
                                               sPage )
                    == IDE_SUCCESS );
    }

    switch( sState )
    {
        case 2 :
            IDE_ASSERT( sMPRMgr.destroy() == IDE_SUCCESS );
        case 1 :
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx )
                        == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * quick sort with KeyMap
 * ----------------------------------------------*/
IDE_RC stndrBUBuild::quickSort( UInt              aHead,
                                UInt              aTail,
                                stndrNodeType     aNodeType,
                                stndrSortType     aSortType )
{
    UInt                 i;
    UInt                 j;
    UInt                 m;
    stndrCenterPoint     sPivotKey;
    smnSortStack         sCurStack; // fix BUG-27403 - for Stack overflow
    smnSortStack         sNewStack; 
    idBool               sEmpty;
    SInt                 sRet;

    // fix BUG-27403 최초 해야할 일 입력
    sCurStack.mLeftPos   = aHead;
    sCurStack.mRightPos  = aTail;
    IDE_TEST( mSortStack.push( ID_FALSE, &sCurStack ) != IDE_SUCCESS );

    sEmpty  = ID_FALSE;

    while( 1 )
    {
        IDE_TEST( mSortStack.pop( ID_FALSE, &sCurStack, &sEmpty )
                  != IDE_SUCCESS);

        // Bug-27403
        // QuickSort의 알고리즘상, CallStack은 ItemCount보다 많아질 수 없다.
        // 따라서 이보다 많으면, 무한루프에 빠졌을 가능성이 높다.
        IDE_ASSERT( (aTail - aHead + 1 ) > mSortStack.getTotItemCnt() );

        if( sEmpty == ID_TRUE)
        {
            break;
        }

        i = sCurStack.mLeftPos;
        j = sCurStack.mRightPos + 1;
        m = (sCurStack.mLeftPos + sCurStack.mRightPos) / 2;

        if( sCurStack.mLeftPos < sCurStack.mRightPos )
        {
            swapBuildKeyMap( mKeyMap, m, sCurStack.mLeftPos );

            gCallbackFuncs4Build[ aNodeType ].getCPFromBuildKey(
                mKeyMap,
                sCurStack.mLeftPos,
                &sPivotKey );

            while( 1 )
            {
                while( (SInt)(++i) <= sCurStack.mRightPos )
                {
                    sRet = gCallbackFuncs4Build[ aNodeType ].compareBuildKeyAndCP(
                        sPivotKey,
                        mKeyMap[ i ],
                        aSortType );

                    if( sRet < 0 )
                    {
                        break;
                    }
                }
    
                while( (SInt)(--j) > sCurStack.mLeftPos )
                {
                    sRet = gCallbackFuncs4Build[ aNodeType ].compareBuildKeyAndCP(
                        sPivotKey,
                        mKeyMap[ j ],
                        aSortType );

                    if( sRet > 0 )
                    {
                        break;
                    }
                }

                if( i < j )
                {
                    swapBuildKeyMap( mKeyMap, i, j );
                }
                else
                {
                    break;
                }
            }

            swapBuildKeyMap( mKeyMap, (i - 1), sCurStack.mLeftPos );

            if( (SInt)i > (sCurStack.mLeftPos + 2) )
            {
                sNewStack.mLeftPos  = sCurStack.mLeftPos;
                sNewStack.mRightPos = i - 2;

                IDE_TEST( mSortStack.push( ID_FALSE, &sNewStack )
                          != IDE_SUCCESS );
            }
            if( (SInt)i < sCurStack.mRightPos )
            {
                sNewStack.mLeftPos  = i;
                sNewStack.mRightPos = sCurStack.mRightPos;

                IDE_TEST( mSortStack.push( ID_FALSE, &sNewStack )
                          != IDE_SUCCESS );
            }
        }
    }

    IDE_DASSERT( gCallbackFuncs4Build[ aNodeType ].checkSortBuildKey( aHead,
                                                                      aTail,
                                                                      mKeyMap,
                                                                      aSortType )
                 == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* ------------------------------------------------
 * sorted block을 disk에 내림
 * ----------------------------------------------*/
IDE_RC stndrBUBuild::storeSortedRun( UInt             aHead,
                                     UInt             aTail,
                                     UInt           * aLeftPos,
                                     UInt           * aLeftSize,
                                     stndrNodeType    aNodeType )
{
    UInt                  i;
    sdrMtx                sMtx;
    SShort                sSlotSeq;
    stndrSortedBlk        sQueueInfo;
    UInt                  sBuildKeyValueLength;
    scPageID              sPrevPID;
    scPageID              sCurrPID;
    scPageID              sNullPID = SD_NULL_PID;
    sdpPhyPageHdr        *sCurrPage;
    stndrNodeHdr         *sNodeHdr;
    idBool                sIsSuccess;
    UInt                  sState = 0;
    UInt                  sAllocatedPageCnt = 0;
    UInt                  sBuildKeyLength;

    IDE_DASSERT( aTail < mMaxKeyMapCount );

    sBuildKeyValueLength = stndrRTree::getKeyValueLength();
    sBuildKeyLength      = gCallbackFuncs4Build[ aNodeType ].getBuildKeyLength( sBuildKeyValueLength );

    // sorted run을 저장할 page를 할당
    sAllocatedPageCnt++;

    IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                   &sMtx,
                                   mTrans,
                                   mLoggingMode,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    sState = 1;

    // NOLOGGING mini-transaction의 Persistent flag 설정
    // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
    sdrMiniTrans::setNologgingPersistent ( &sMtx );

    IDE_TEST( allocAndLinkPage( &sMtx,
                                &sCurrPID,
                                &sCurrPage,
                                &sNodeHdr,
                                NULL,       // aPrevPageHdr
                                &sNullPID,  // aPrevPID
                                &sNullPID ) // aNextPID
              != IDE_SUCCESS );

    // sorted run의 첫번째 page의 PID를 PIDBlkQueue에 enqueue
    sQueueInfo.mStartPID = sCurrPID;
    IDE_TEST( mRunQueue.enqueue( ID_FALSE, (void*)&sQueueInfo )
              != IDE_SUCCESS );

    sSlotSeq = -1;

    if( aLeftPos != NULL )
    {
        /* BUG-23525 남은 키 크기 계산을 위해 aLeftSize도 있어야 함. */
        IDE_ASSERT( aLeftSize != NULL ); 
        *aLeftPos = aTail + 1;
    }

    for( i = aHead; i <= aTail; i++ )
    {
        if( sdpPhyPage::canAllocSlot(
                sCurrPage,
                sBuildKeyLength,
                ID_TRUE,
                1 )
            != ID_TRUE )
        {
            // 남은 키가 일정 이상이라면 다시 삽입을, 일정 이하로 작다면 다음으로 미룬다.
            // 키의 크기가 작다면 다음에 다시 한번 sorting한다.
            if( aLeftPos != NULL )
            {
                if( mLeftSizeThreshold > *aLeftSize )
                {
                    *aLeftPos = i;
                    break;
                }
            }

            // Page에 충분한 공간이 남아있지 않다면 새로운 Page Alloc
            sPrevPID = sCurrPage->mPageID;

            IDE_TEST( allocAndLinkPage( &sMtx,
                                        &sCurrPID,
                                        &sCurrPage,
                                        &sNodeHdr,
                                        sCurrPage, // Prev Page Pointer
                                        &sPrevPID,
                                        &sNullPID )
                      != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
            sAllocatedPageCnt++;

            IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                           &sMtx,
                                           mTrans,
                                           mLoggingMode,
                                           ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                           gMtxDLogType )
                      != IDE_SUCCESS );
            sState = 1;

            // NOLOGGING mini-transaction의 Persistent flag 설정
            // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
            sdrMiniTrans::setNologgingPersistent ( &sMtx );

            // 이전 page를 PrevTail에 getpage
            IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  sCurrPID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  &sMtx,
                                                  (UChar**)&sCurrPage,
                                                  &sIsSuccess,
                                                  NULL )
                      != IDE_SUCCESS );

            /* BUG-27527 Disk Index Bottom Up Build시, 로그 없이 Key Insert할 경우
             * 페이지가 Dirty되지 않음.
             * 이후에 이 페이지에 로그를 남기지 않는 Write작업이 일어나, 페이지가
             * Dirty되지 않는다.  따라서 강제로 Dirt시킨다.*/
            sdbBufferMgr::setDirtyPageToBCB( mStatistics, (UChar*)sCurrPage );
            
            sSlotSeq = -1;
        }
        sSlotSeq++;

        //fix BUG-23525 페이지 할당을 위한 판단 자료
        if( aLeftSize != NULL )
        {
            IDE_DASSERT( sBuildKeyLength <= (*aLeftSize) );
            (*aLeftSize) -= sBuildKeyLength;
        }

        gCallbackFuncs4Build[ aNodeType ].insertBuildKey( sCurrPage,
                                                          sSlotSeq,
                                                          mKeyMap[i] );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrBUBuild::preparePages                 *
 * ------------------------------------------------------------------*
 * 주어진 개수만큼의 페이지를 할당받을수 있을지 검사한다.            *
 *********************************************************************/
IDE_RC stndrBUBuild::preparePages( UInt aNeedPageCnt )
{
    stndrHeader  * sIdxHeader = NULL;
    sdrMtx        sMtx;
    UInt          sState = 0;

    if( mFreePageCnt < aNeedPageCnt )
    {
        sIdxHeader = (stndrHeader *)(mIndex->mHeader);
        
        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdpSegDescMgr::getSegMgmtOp(
                      &(sIdxHeader->mSdnHeader.mSegmentDesc) )->mPrepareNewPages(
                          mStatistics,
                          &sMtx,
                          sIdxHeader->mSdnHeader.mIndexTSID,
                          &(sIdxHeader->mSdnHeader.mSegmentDesc.mSegHandle),
                          aNeedPageCnt - mFreePageCnt )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }
    
    return IDE_FAILURE;
}

IDE_RC stndrBUBuild::allocAndLinkPage(sdrMtx         * aMtx,
                                      scPageID       * aPageID,
                                      sdpPhyPageHdr ** aPageHdr,
                                      stndrNodeHdr  ** aNodeHdr,
                                      sdpPhyPageHdr  * aPrevPageHdr,
                                      scPageID       * aPrevPID,
                                      scPageID       * aNextPID )
{
    sdrMtx            sMtx;
    stndrNodeHdr     *sNodeHdr;
    sdpPhyPageHdr    *sCurrPage;    
    UInt              sState = 0;
    scPageID          sPopPID;
    idBool            sIsEmpty;
    idBool            sIsSuccess;
    UChar             sIsConsistent = SDP_PAGE_INCONSISTENT;
    sdpSegmentDesc   *sSegmentDesc;

    IDE_DASSERT( aPrevPID != NULL);
    IDE_DASSERT( aNextPID != NULL);

    sSegmentDesc = &(((stndrHeader*)(mIndex->mHeader))->mSdnHeader.mSegmentDesc);

    IDE_TEST( mFreePage.pop( ID_FALSE,
                             (void*)&sPopPID,
                             &sIsEmpty )
              != IDE_SUCCESS );

    if( sIsEmpty == ID_TRUE ) // 사용후 free 된 page가 없을 경우
    {
        /* Bottom-up index build 시 사용할 page들을 할당한다.
         * Logging mode로 new page를 alloc 받음 : index build를
         * undo할 경우 extent를 free 시키기 위해서   */
        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdpSegDescMgr::getSegMgmtOp( sSegmentDesc )->mAllocNewPage(
                      mStatistics, /* idvSQL* */
                      &sMtx,
                      mIndex->mIndexSegDesc.mSpaceID,
                      sdpSegDescMgr::getSegHandle( sSegmentDesc ),
                      SDP_PAGE_INDEX_RTREE,
                      (UChar**)&sCurrPage ) != IDE_SUCCESS );

        IDE_TEST( sdpPhyPage::logAndInitLogicalHdr( sCurrPage,
                                                    ID_SIZEOF(stndrNodeHdr),
                                                    &sMtx,
                                                    (UChar**)&sNodeHdr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpSlotDirectory::logAndInit( sCurrPage,
                                                &sMtx )
                  != IDE_SUCCESS );

        if( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( &sMtx,
                                                      sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }

        *aPageID = sCurrPage->mPageID;

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              *aPageID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sCurrPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        mFreePageCnt--;
        *aPageID = sPopPID;

        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              *aPageID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sCurrPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        // BUG-17615 로깅양 줄이기
        // 1단계(extractNSort), 2단계(merge)에서 반환된 페이지들은
        // LOGICAL HEADER는 사용된적이 없기 때문에 PHYSICAL HEADER만
        // RESET 한다.
        IDE_TEST( sdpPhyPage::reset((sdpPhyPageHdr*)sCurrPage,
                                    ID_SIZEOF(stndrNodeHdr),
                                    aMtx) 
                  != IDE_SUCCESS );

        IDE_TEST( sdpSlotDirectory::logAndInit( sCurrPage,
                                                aMtx )
                  != IDE_SUCCESS );

        sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sCurrPage );

        if( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( aMtx,
                                                      sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }
    }

    if( aPrevPageHdr != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&aPrevPageHdr->mListNode.mNext,
                      (void*)aPageID,
                      ID_SIZEOF(scPageID) )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sCurrPage->mListNode.mPrev,
                  (void*)aPrevPID,
                  ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sCurrPage->mListNode.mNext,
                  (void*)aNextPID,
                  ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );

    *aPageHdr = sCurrPage;
    *aNodeHdr = sNodeHdr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::freePage( scPageID aPageID )
{

    IDE_TEST( mFreePage.push( ID_FALSE,
                              (void*)&aPageID )
              != IDE_SUCCESS );

    mFreePageCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::collectFreePage( stndrBUBuild  * aThreads,
                                      UInt            aThreadCnt )
{
    scPageID  sPopPID;
    idBool    sIsEmpty;
    UInt      i;

    for( i = 1; i < aThreadCnt; i++ )
    {
        while(1)
        {
            IDE_TEST( aThreads[i].mFreePage.pop( ID_FALSE,
                                                 (void*)&sPopPID,
                                                 &sIsEmpty )
                      != IDE_SUCCESS );

            if( sIsEmpty == ID_TRUE )
            {
                break;
            }

            aThreads[i].mFreePageCnt--;

            IDE_TEST( aThreads[0].mFreePage.push( ID_FALSE,
                                                  (void*)&sPopPID )
                      != IDE_SUCCESS );

            aThreads[0].mFreePageCnt++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::removeFreePage()
{
    scPageID        sPopPID;
    idBool          sIsEmpty;
    UInt            sState = 0;
    sdrMtx          sMtx;
    UChar         * sSegDesc;
    sdpPhyPageHdr * sPage;
    idBool          sIsSuccess;
    sdpSegmentDesc *sSegmentDesc;

    sSegmentDesc = &(((stndrHeader*)(mIndex->mHeader))->mSdnHeader.mSegmentDesc);

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] FREE PAGE (%d)               \n\
========================================\n", mFreePageCnt );

    while(1)
    {
        IDE_TEST( mFreePage.pop( ID_FALSE,
                                 (void*)&sPopPID,
                                 &sIsEmpty )
                  != IDE_SUCCESS );

        if( sIsEmpty == ID_TRUE )
        {
            break;
        }

        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              sPopPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              &sMtx,
                                              (UChar**)&sPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByGRID( mStatistics,
                                               mIndex->mIndexSegDesc,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               &sSegDesc,
                                               &sIsSuccess )
                  != IDE_SUCCESS );

        IDE_TEST( sdpSegDescMgr::getSegMgmtOp( sSegmentDesc )->mFreePage(
                      mStatistics,
                      &sMtx,
                      mIndex->mIndexSegDesc.mSpaceID,
                      sdpSegDescMgr::getSegHandle( sSegmentDesc ),
                      (UChar*)sPage )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)sdrMiniTrans::rollback( &sMtx );
        default:
            break;
    }

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::merge( UInt            aMergePageCount,
                            stndrNodeType   aNodeType,
                            stndrSortType   aSortType )
{
    sdrMtx              sMtx;
    idBool              sIsEmpty;
    idBool              sIsSuccess;
    idBool              sIsClosedRun;
    SShort              sResultSlotSeq = -1;
    UInt                i;
    UInt                sRunInfoCount = 0;
    UInt                sMinIdx;
    UInt                sBuildKeyValueLength;
    UInt                sBuildKeyLength;
    UInt                sQueueLength;
    UInt                sClosedRun = 0;
    SInt               *sHeapMap = NULL;
    UInt                sState = 0;
    UInt                sHeapMapCount = 1;

    // merge에 사용되는 run의 자료구조
    stndrRunInfo       *sRunInfo = NULL;

    scPageID            sCurrResultPID;
    scPageID            sPrevResultPID;
    scPageID            sNullPID = SD_NULL_PID;

    stndrNodeHdr       *sNodeHdr;
    stndrSortedBlk      sQueueInfo;
    sdpPhyPageHdr      *sCurrResultRun;

    IDE_ASSERT( aMergePageCount > 0 );
    IDE_ASSERT( mMergePageCount >= aMergePageCount );
    // BUG-29315: 런의 갯수가 0일 경우 비정상 종료하는 문제
    IDE_TEST_RAISE( mRunQueue.getQueueLength() <= 1, SKIP_RUN_MERGE );

    sBuildKeyValueLength = stndrRTree::getKeyValueLength();
    sBuildKeyLength      = gCallbackFuncs4Build[ aNodeType ].getBuildKeyLength( sBuildKeyValueLength );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ST_TEMP, 1, 
                                 mMergePageCount * ID_SIZEOF(stndrRunInfo),
                                 (void**)&sRunInfo )
              != IDE_SUCCESS );

    sState = 1;

    while( sHeapMapCount < (mMergePageCount * 2) ) // get heap size
    {
        sHeapMapCount = sHeapMapCount * 2;
    }

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ST_TEMP,
                                 sHeapMapCount * ID_SIZEOF(SInt),
                                 (void**)&sHeapMap )
              != IDE_SUCCESS );

    sState = 2;

    for( i = 0; i < sHeapMapCount; i++ )
    {
        sHeapMap[i] = -1;
    }

    while( 1 )
    {
        sRunInfoCount = 0;

        sQueueLength = mRunQueue.getQueueLength();

        // merge해야 할 run의 수가 입력받은 수보다 작으면 merge 를 종료한다.
        if( sQueueLength <= aMergePageCount )
        {
            break;
        }

        // 한번에 merge할 page 수만큼 각 run의 첫번째 page를 fix
        while( 1 )
        {
            sQueueLength = mRunQueue.getQueueLength();

            if( sQueueLength == 0 )
            {
                break;
            }

            IDE_TEST( mRunQueue.dequeue( ID_FALSE,
                                         (void*)&sQueueInfo,
                                         &sIsEmpty )
                      != IDE_SUCCESS );

            IDE_ASSERT( sIsEmpty != ID_TRUE );

            sRunInfo[sRunInfoCount].mHeadPID = sQueueInfo.mStartPID;

            IDE_TEST( sdbBufferMgr::fixPageByPID( mStatistics, /* idvSQL* */
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  sRunInfo[sRunInfoCount].mHeadPID,
                                                  (UChar**)&sRunInfo[sRunInfoCount].mPageHdr,
                                                  &sIsSuccess )
                      != IDE_SUCCESS );

            sRunInfoCount++;

            // 배열 범위를 넘어갈 경우 중단한다.
            if( sRunInfoCount > (mMergePageCount - 1) )
            {
                break;
            }
        }

        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       mLoggingMode,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 3;

        // NOLOGGING mini-transaction의 Persistent flag 설정
        // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
        sdrMiniTrans::setNologgingPersistent ( &sMtx );

        IDE_TEST( allocAndLinkPage( &sMtx,
                                    &sCurrResultPID,
                                    &sCurrResultRun,
                                    &sNodeHdr,
                                    NULL,       // aPrevPageHdr
                                    &sNullPID,  // aPrevPID
                                    &sNullPID ) // aNextPID
                  != IDE_SUCCESS );        

        sResultSlotSeq = -1;

        sQueueInfo.mStartPID = sCurrResultPID;

        IDE_TEST( mRunQueue.enqueue( ID_FALSE, (void*)&sQueueInfo )
                  != IDE_SUCCESS );

        // initialize the selection tree to merge
        IDE_TEST( heapInit( sRunInfoCount,
                            sHeapMapCount,
                            sHeapMap,
                            sRunInfo,
                            aNodeType,
                            aSortType )
                  != IDE_SUCCESS );

        sClosedRun = 0;

        while( 1 )
        {
            if( sClosedRun == sRunInfoCount )
            {
                sState = 2;
                IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
                break;
            }

            sMinIdx = sHeapMap[1];   // the root of selection tree

            if( sdpPhyPage::canAllocSlot(
                    sCurrResultRun,
                    sBuildKeyLength,
                    ID_TRUE,
                    1 )
                != ID_TRUE )
            {
                // Page에 충분한 공간이 남아있지 않다면
                // 새로운 Page Alloc
                sPrevResultPID = sCurrResultPID;

                IDE_TEST( allocAndLinkPage( &sMtx,
                                            &sCurrResultPID,
                                            &sCurrResultRun,
                                            &sNodeHdr,
                                            sCurrResultRun,
                                            &sPrevResultPID,
                                            &sNullPID )
                          != IDE_SUCCESS );

                sState = 2;
                IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

                IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                               &sMtx,
                                               mTrans,
                                               mLoggingMode,
                                               ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                               gMtxDLogType )
                          != IDE_SUCCESS );
                sState = 3;

                // NOLOGGING mini-transaction의 Persistent flag 설정
                // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
                sdrMiniTrans::setNologgingPersistent ( &sMtx );

                IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                                      mIndex->mIndexSegDesc.mSpaceID,
                                                      sCurrResultPID,
                                                      SDB_X_LATCH,
                                                      SDB_WAIT_NORMAL,
                                                      SDB_SINGLE_PAGE_READ,
                                                      &sMtx,
                                                      (UChar**)&sCurrResultRun,
                                                      &sIsSuccess,
                                                      NULL )
                          != IDE_SUCCESS );                                

                /* BUG-27527 Disk Index Bottom Up Build시, 로그 없이 Key Insert할 경우
                 * 페이지가 Dirty되지 않음.
                 * 이후에 이 페이지에 로그를 남기지 않는 Write작업이 일어나, 페이지가
                 * Dirty되지 않는다.  따라서 강제로 Dirty시킨다.*/
                sdbBufferMgr::setDirtyPageToBCB( mStatistics, (UChar*)sCurrResultRun );

                sResultSlotSeq = -1;
            }
            sResultSlotSeq++;

            gCallbackFuncs4Build[ aNodeType ].insertBuildKey( sCurrResultRun,
                                                              sResultSlotSeq,
                                                              sRunInfo[sMinIdx].mKey );

            IDE_TEST( heapPop( sMinIdx,
                               &sIsClosedRun,
                               sHeapMapCount,
                               sHeapMap,
                               sRunInfo,
                               aNodeType,
                               aSortType )
                      != IDE_SUCCESS );

            if( sIsClosedRun == ID_TRUE )
            {
                sClosedRun++;
            }
        }

        IDE_TEST( iduCheckSessionEvent( mStatistics ) != IDE_SUCCESS );
    }

    sState = 1;
    IDE_TEST( iduMemMgr::free( sHeapMap ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sRunInfo ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_RUN_MERGE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    for( i = 0; i < sRunInfoCount; i++ )
    {
        if( sRunInfo[i].mPageHdr != NULL )
        {
            (void)sdbBufferMgr::unfixPage( mStatistics, /* idvSQL* */
                                           (UChar*)sRunInfo[i].mPageHdr );
        }
    }

    switch( sState )
    {
        case 3:
            (void)sdrMiniTrans::rollback( &sMtx );
        case 2:
            (void)iduMemMgr::free( sHeapMap );
        case 1:
            (void)iduMemMgr::free( sRunInfo );
        default:
            break;
    }
            
    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::heapInit( UInt            aRunCount,
                               UInt            aHeapMapCount,
                               SInt          * aHeapMap,
                               stndrRunInfo  * aRunInfo,
                               stndrNodeType   aNodeType,
                               stndrSortType   aSortType )
{
    SInt               sRet;
    UInt               i;
    UInt               sLChild;
    UInt               sRChild;
    UInt               sPos = aHeapMapCount / 2;
    UInt               sHeapLevelCnt = aRunCount;
    UChar            * sPagePtr;
    UChar            * sSlotDirPtr;

    for( i = 0; i < aRunCount; i++)
    {
        aRunInfo[i].mSlotSeq = 0;

        IDE_TEST( getBuildKeyFromInfo( aRunInfo[i].mHeadPID,
                                       aRunInfo[i].mSlotSeq,
                                       aRunInfo[i].mPageHdr,
                                       &aRunInfo[i].mKey )
                  != IDE_SUCCESS );

        sPagePtr    = (UChar*)aRunInfo[i].mPageHdr;
        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sPagePtr );
        aRunInfo[i].mSlotCnt = sdpSlotDirectory::getCount( sSlotDirPtr );
    }

    for( i = 0; i < sHeapLevelCnt; i++)
    {
        aHeapMap[i + sPos] = i;
    }

    for( i = i + sPos; i < aHeapMapCount; i++ )
    {
        aHeapMap[i] = -1;
    }

    IDE_DASSERT( i != 0 );

    while( 1 )
    {
        sPos = sPos / 2;
        sHeapLevelCnt  = (sHeapLevelCnt + 1) / 2;
        
        for( i = 0; i < sHeapLevelCnt; i++ )
        {
            sLChild  = (i + sPos) * 2;
            sRChild = sLChild + 1;
            
            if( (aHeapMap[sLChild] == -1) && (aHeapMap[sRChild] == -1) )
            {
                aHeapMap[i + sPos] = -1;
            }
            else if( aHeapMap[sLChild] == -1 )
            {
                aHeapMap[i + sPos] = aHeapMap[sRChild];
            }
            else if( aHeapMap[sRChild] == -1 )
            {
                aHeapMap[i + sPos] = aHeapMap[sLChild];
            }
            else
            {
                sRet = gCallbackFuncs4Build[ aNodeType ].compareBuildKeyAndBuildKey(
                    aRunInfo[aHeapMap[sLChild]].mKey,
                    aRunInfo[aHeapMap[sRChild]].mKey,
                    aSortType );

                if( sRet > 0 )
                {
                    aHeapMap[i + sPos] = aHeapMap[sRChild];
                }
                else
                {
                    // BUG-29998: 동일 키에 대해서 HeapMap 설정하지 않는 문제
                    aHeapMap[i + sPos] = aHeapMap[sLChild];
                }
            }                
        }
        if( sPos == 1 )    // the root of selection tree
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::heapPop( UInt               aMinIdx,
                              idBool           * aIsClosedRun,
                              UInt               aHeapMapCount,
                              SInt             * aHeapMap,
                              stndrRunInfo     * aRunInfo,
                              stndrNodeType      aNodeType,
                              stndrSortType      aSortType )
{
    SInt               sRet;
    UInt               sLChild;
    UInt               sRChild;
    UChar            * sPagePtr;
    UChar            * sSlotDirPtr;
    UInt               sPos = aHeapMapCount / 2;
    idBool             sIsSuccess;

    aRunInfo[aMinIdx].mSlotSeq++;
    *aIsClosedRun = ID_FALSE;

    if( aRunInfo[aMinIdx].mSlotSeq ==
        aRunInfo[aMinIdx].mSlotCnt )
    {
        IDE_TEST( freePage( aRunInfo[aMinIdx].mHeadPID )
                  != IDE_SUCCESS );

        aRunInfo[aMinIdx].mHeadPID =
            aRunInfo[aMinIdx].mPageHdr->mListNode.mNext;

        IDE_TEST( sdbBufferMgr::unfixPage(
                      mStatistics, /* idvSQL* */
                      (UChar*)aRunInfo[aMinIdx].mPageHdr )
                  != IDE_SUCCESS );

        aRunInfo[aMinIdx].mPageHdr = NULL;

        if( aRunInfo[aMinIdx].mHeadPID == SD_NULL_PID )
        {
            *aIsClosedRun = ID_TRUE;
            aHeapMap[sPos + aMinIdx] = -1;
        }
        else
        {
            IDE_TEST( sdbBufferMgr::fixPageByPID( mStatistics, /* idvSQL* */
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  aRunInfo[aMinIdx].mHeadPID,
                                                  (UChar**)&aRunInfo[aMinIdx].mPageHdr,
                                                  &sIsSuccess )
                      != IDE_SUCCESS );

            aRunInfo[aMinIdx].mSlotSeq = 0;

            sPagePtr    = (UChar*)aRunInfo[aMinIdx].mPageHdr;
            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sPagePtr );
            aRunInfo[aMinIdx].mSlotCnt = sdpSlotDirectory::getCount( sSlotDirPtr );

            IDE_DASSERT( aRunInfo[aMinIdx].mSlotCnt > 0 );
        }
    }
    else
    {
        IDE_DASSERT( aRunInfo[aMinIdx].mSlotSeq <
                     aRunInfo[aMinIdx].mSlotCnt );
    }

    if( aRunInfo[aMinIdx].mHeadPID != SD_NULL_PID )
    {
        IDE_DASSERT( aRunInfo[aMinIdx].mPageHdr != NULL );

        IDE_TEST( getBuildKeyFromInfo( aRunInfo[aMinIdx].mHeadPID,
                                       aRunInfo[aMinIdx].mSlotSeq,
                                       aRunInfo[aMinIdx].mPageHdr,
                                       &aRunInfo[aMinIdx].mKey )
                  != IDE_SUCCESS );
    }

    sPos = (sPos + aMinIdx) / 2;

    while( 1 )
    {
        sLChild = sPos * 2;      // Left child position
        sRChild = sLChild + 1;   // Right child position
        
        if( (aHeapMap[sLChild] == -1) && (aHeapMap[sRChild] == -1) )
        {
            aHeapMap[sPos] = -1;
        }
        else if( aHeapMap[sLChild] == -1 )
        {
            aHeapMap[sPos] = aHeapMap[sRChild];
        }
        else if( aHeapMap[sRChild] == -1 )
        {
            aHeapMap[sPos] = aHeapMap[sLChild];
        }
        else
        {
            sRet = gCallbackFuncs4Build[ aNodeType ].compareBuildKeyAndBuildKey(
                aRunInfo[aHeapMap[sLChild]].mKey,
                aRunInfo[aHeapMap[sRChild]].mKey,
                aSortType );

            if( sRet > 0 )
            {
                aHeapMap[sPos] = aHeapMap[sRChild];
            }
            else
            {
                // BUG-29273: 동일 키에 대해서 HeapMap 설정하지 않는 문제
                aHeapMap[sPos] = aHeapMap[sLChild];
            }
        }
        if( sPos == 1 )    // the root of selection tree
        {
            break;
        }
        sPos = sPos / 2;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;    

    return IDE_FAILURE;
}


void stndrBUBuild::makePartition( stndrNodeType aNodeType )
{
    UInt          sPageCount4Matrix;    // matrix 에 필요한 page count
    UInt          sTempSqrt;            // 완전제곱수 판단하는데 필요한 임시변수
    UInt          sSqrt;                // sPerfectSqrt 의 양의 제곱근
    UInt          sKeyValueLength;
    UInt          sNodeKeyLength;
    UInt          sPropertyKeyCount;
    stndrHeader * sIndexHeader;

    IDE_ASSERT( mPropagatedKeyCount != 0 );

    sPropertyKeyCount = smuProperty::getRTreeMaxKeyCount(); // __DISK_INDEX_RTREE_MAX_KEY_COUNT
    sKeyValueLength   = stndrRTree::getKeyValueLength();
    sNodeKeyLength    = gCallbackFuncs4Build[ aNodeType ].getNodeKeyLength( sKeyValueLength );
    sIndexHeader      = (stndrHeader*)mIndex->mHeader;

    mFreeSize4EmptyNode = gCallbackFuncs4Build[ aNodeType ].getFreeSize4EmptyNode( sIndexHeader );

    mKeyCount4EmptyNode = mFreeSize4EmptyNode
                          / ( ID_SIZEOF(sdpSlotEntry) + sNodeKeyLength );

    if( mKeyCount4EmptyNode > sPropertyKeyCount )
    {
        mKeyCount4EmptyNode = sPropertyKeyCount;
    }
    else
    {
        // nothing to do
    }

    // matrix 에 필요한 전체 페이지 갯수를 구한다.
    sPageCount4Matrix = ( mPropagatedKeyCount + mKeyCount4EmptyNode - 1 ) / mKeyCount4EmptyNode;

    // 완전제곱수가 맞는지 체크한다.
    sTempSqrt = (UInt)idlOS::sqrt( sPageCount4Matrix );
    if( sPageCount4Matrix == (sTempSqrt * sTempSqrt) )
    {
        sSqrt = sTempSqrt;
    }
    else
    {
        sSqrt = sTempSqrt + 1;
    }

    // 1개의 Y Sorted Run 에 들어가는 key count
    mKeyCount4Partition = sSqrt * mKeyCount4EmptyNode;
}


IDE_RC stndrBUBuild::collectRun( stndrBUBuild   * aThreads,
                                 UInt             aThreadCnt )
{
    UInt                 i;
    idBool               sIsEmpty;
    stndrSortedBlk       sQueueInfo;

    // 여러 쓰레드에 흩어져 있는 run 들을 thread[0] 에 합친다.
    // thread[1] 부터 수행을 해야 한다.
    for( i = 1; i < aThreadCnt; i++ )
    {
        while(1)
        {
            IDE_TEST( aThreads[i].mRunQueue.dequeue( ID_FALSE,
                                                     (void*)&sQueueInfo,
                                                     &sIsEmpty )
                      != IDE_SUCCESS );

            if( sIsEmpty == ID_TRUE )
            {
                break;
            }

            IDE_TEST( aThreads[0].mRunQueue.enqueue( ID_FALSE,
                                                     (void*)&sQueueInfo )
                      != IDE_SUCCESS );
        }

        aThreads[0].mPropagatedKeyCount += aThreads[i].mPropagatedKeyCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::getBuildKeyFromRun( idBool            * aIsClosedRun,
                                         stndrRunInfo      * aRunInfo,
                                         void             ** aBuildKey,
                                         stndrNodeType       /* aNodeType */ )
{
    UChar     * sPagePtr;
    UChar     * sSlotDirPtr;
    idBool      sIsSuccess;

    IDE_ASSERT( aRunInfo  != NULL );
    IDE_ASSERT( aBuildKey != NULL );

    *aIsClosedRun = ID_FALSE;

    if( aRunInfo->mSlotSeq ==
        aRunInfo->mSlotCnt )
    {
        IDE_TEST( freePage( aRunInfo->mHeadPID )
                  != IDE_SUCCESS);

        aRunInfo->mHeadPID =
            aRunInfo->mPageHdr->mListNode.mNext;

        IDE_TEST( sdbBufferMgr::unfixPage(
                      mStatistics, /* idvSQL* */
                      (UChar*)aRunInfo->mPageHdr )
                  != IDE_SUCCESS );

        aRunInfo->mPageHdr = NULL;

        if( aRunInfo->mHeadPID == SD_NULL_PID )
        {
            *aIsClosedRun = ID_TRUE;
        }
        else
        {
            IDE_TEST( sdbBufferMgr::fixPageByPID( mStatistics, /* idvSQL* */
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  aRunInfo->mHeadPID,
                                                  (UChar**)&aRunInfo->mPageHdr,
                                                  &sIsSuccess )
                      != IDE_SUCCESS );

            aRunInfo->mSlotSeq = 0;

            sPagePtr      = (UChar*)aRunInfo->mPageHdr;
            sSlotDirPtr   = sdpPhyPage::getSlotDirStartPtr( sPagePtr );
            aRunInfo->mSlotCnt = sdpSlotDirectory::getCount( sSlotDirPtr );

            IDE_DASSERT( aRunInfo->mSlotCnt > 0 );
        }
    }
    else
    {
        IDE_DASSERT( aRunInfo->mSlotSeq <
                     aRunInfo->mSlotCnt );
    }

    if( aRunInfo->mHeadPID != SD_NULL_PID )
    {
        IDE_DASSERT( aRunInfo->mPageHdr != NULL );

        IDE_TEST( getBuildKeyFromInfo( aRunInfo->mHeadPID,
                                       aRunInfo->mSlotSeq,
                                       aRunInfo->mPageHdr,
                                       &aRunInfo->mKey )
                  != IDE_SUCCESS );

        *aBuildKey = aRunInfo->mKey;

        aRunInfo->mSlotSeq++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC stndrBUBuild::sortY( UInt          aMergePageCnt,
                            stndrNodeType aNodeType )
{
    idBool               sIsEmpty;
    idBool               sIsSuccess;
    idBool               sIsClosedRun;
    UInt                 i, j;
    UInt                 sRunInfoCount = 0;
    UInt                 sMinIdx;
    UInt                 sNextPos = 0;
    UInt                 sLeftPos = 0;
    UInt                 sLeftCnt = 0;
    UInt                 sState = 0;
    UInt                 sHeapMapCount = 1;
    SInt               * sHeapMap;
    UInt                 sClosedRun = 0;

    stndrSortedBlk       sQueueInfo;
    stndrRunInfo       * sRunInfo;

    UInt                 sBuildKeyValueLength;
    UInt                 sBuildKeyLength;
    UInt                 sInsertedBuildKeyCnt = 0;
    UInt                 sUsedBufferSize = 0;
    UChar                sBuffer4Compaction[ SD_PAGE_SIZE ]; //짜투리를 저장해두는 공간

    sBuildKeyValueLength = stndrRTree::getKeyValueLength();
    sBuildKeyLength      = gCallbackFuncs4Build[ aNodeType ].getBuildKeyLength( sBuildKeyValueLength );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ST_TEMP, 1,
                                 aMergePageCnt * ID_SIZEOF(stndrRunInfo),
                                 (void**)&sRunInfo )
              != IDE_SUCCESS );
    sState = 1;

    while( sHeapMapCount < (aMergePageCnt * 2) ) // get heap size
    {
        sHeapMapCount *= 2;
    }

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ST_TEMP,
                                 sHeapMapCount * ID_SIZEOF(SInt),
                                 (void**)&sHeapMap )
              != IDE_SUCCESS );
    sState = 2;

    for( i = 0; i < sHeapMapCount; i++ )
    {
        sHeapMap[i] = -1;
    }

    while(1)
    {
        IDE_TEST( mRunQueue.dequeue( ID_FALSE,
                                     (void*)&sQueueInfo,
                                     &sIsEmpty )
                  != IDE_SUCCESS );
        
        if( sIsEmpty == ID_TRUE )
        {
            break;
        }

        sRunInfo[sRunInfoCount].mHeadPID = sQueueInfo.mStartPID;

        IDE_TEST( sdbBufferMgr::fixPageByPID(
                      mStatistics, /* idvSQL* */
                      mIndex->mIndexSegDesc.mSpaceID,
                      sRunInfo[sRunInfoCount].mHeadPID,
                      (UChar**)&sRunInfo[sRunInfoCount].mPageHdr,
                      &sIsSuccess )
                  != IDE_SUCCESS );

        sRunInfoCount++;
    }

    IDE_ASSERT( sRunInfoCount <= aMergePageCnt );

    // initialize the selection tree to merge
    IDE_TEST( heapInit( sRunInfoCount,
                        sHeapMapCount,
                        sHeapMap,
                        sRunInfo,
                        aNodeType,
                        STNDR_SORT_X )
              != IDE_SUCCESS );

    sClosedRun = 0;

    // 여기서부터 종료조건 잘 봐야함.
    // sState 도 나중에 정리해야 함.
    while(1)
    {
        sNextPos = 0;
        sLeftPos = 0;
        sLeftCnt = 0;
        sInsertedBuildKeyCnt = 0;
        sUsedBufferSize = 0;

        for( i = 0; i < mKeyCount4Partition; i++ )
        {
            if( sClosedRun == sRunInfoCount )
            {
                break;
            }

            sMinIdx = sHeapMap[1];   // the root of selection tree

            // fix BUG-23525 InsertableKeyCnt 에 도달하면
            if( (sUsedBufferSize + sBuildKeyLength) > mKeyBufferSize )
            {
                IDE_DASSERT( sInsertedBuildKeyCnt <= mMaxKeyMapCount );

                // sort area의 key 정렬
                IDE_TEST( quickSort( 0,
                                     sNextPos - 1,
                                     aNodeType,
                                     STNDR_SORT_Y )
                          != IDE_SUCCESS );

                // temp segment 에 내림
                IDE_TEST( storeSortedRun( 0,
                                          sNextPos - 1,
                                          &sLeftPos,
                                          &sUsedBufferSize,
                                          aNodeType )
                          != IDE_SUCCESS );

                sLeftCnt = sNextPos - sLeftPos;
                sUsedBufferSize = 0;

                if( sLeftCnt > 0 )
                {
                    //KeyValue들을 맨 앞쪽으로 붙여넣기
                    for( j = 0; j < sLeftCnt; j++ )
                    {
                        swapBuildKeyMap( mKeyMap, j, (sLeftPos + j) );

                        // BUG-31024: Bottom-UP 빌드 시에 행이 걸리는 문제
                        //            Buffer Overflow로 인해 발생
                        IDE_ASSERT( (sUsedBufferSize + sBuildKeyLength)
                                    <= ID_SIZEOF(sBuffer4Compaction) );

                        /* fix BUG-23525 저장되는 키의 길이가 Varaibale하면 
                         * Fragmentation이 일어날 수 밖에 없고, 따라서 Compaction과정은 필수 */
                        idlOS::memcpy( &sBuffer4Compaction[ sUsedBufferSize ],
                                       mKeyMap[j],
                                       sBuildKeyLength );

                        mKeyMap[j] = &mKeyBuffer[ sUsedBufferSize ] ;
                        sUsedBufferSize += sBuildKeyLength;
                    }
                    idlOS::memcpy( mKeyBuffer, sBuffer4Compaction, sUsedBufferSize );

                    sInsertedBuildKeyCnt = sLeftCnt;
                }
                else
                {
                    sInsertedBuildKeyCnt = 0;
                }

                sNextPos = sInsertedBuildKeyCnt;
            }

            IDE_ASSERT( sInsertedBuildKeyCnt < mMaxKeyMapCount );
            IDE_ASSERT( (sUsedBufferSize + sBuildKeyLength) <= mKeyBufferSize );

            // key extraction
            mKeyMap[sNextPos] = &mKeyBuffer[ sUsedBufferSize ] ;
            idlOS::memcpy( mKeyMap[sNextPos], sRunInfo[sMinIdx].mKey, sBuildKeyLength );

            sUsedBufferSize += sBuildKeyLength;
            sInsertedBuildKeyCnt++;
            sNextPos++;

            IDE_TEST( heapPop( sMinIdx,
                               &sIsClosedRun,
                               sHeapMapCount,
                               sHeapMap,
                               sRunInfo,
                               aNodeType,
                               STNDR_SORT_X )
                      != IDE_SUCCESS );

            if( sIsClosedRun == ID_TRUE )
            {
                sClosedRun++;
            }
        } // for( i = 0; i < mKeyCount4Partition; i++ )

        // 작업 공간에 남아 있는 것들 처리
        if( sNextPos > 0 )
        {
            // in-memory sort
            IDE_TEST( quickSort( 0,
                                 sNextPos - 1,
                                 aNodeType,
                                 STNDR_SORT_Y )
                      != IDE_SUCCESS );
            // temp segment로 내린다.
            IDE_TEST( storeSortedRun( 0,
                                      sNextPos - 1,
                                      NULL,
                                      NULL,
                                      aNodeType )
                      != IDE_SUCCESS );
        }

        // Y merge
        IDE_TEST( merge( 1, aNodeType, STNDR_SORT_Y ) != IDE_SUCCESS );

        // Y merge 가 정상적으로 끝났었다면, 무조건 1개의 run 만 존재해야 한다.
        IDE_ASSERT( mRunQueue.getQueueLength() == 1 );

        IDE_TEST( mRunQueue.dequeue( ID_FALSE,
                                     (void*)&sQueueInfo,
                                     &sIsEmpty )
                  != IDE_SUCCESS );

        IDE_ASSERT( sIsEmpty != ID_TRUE );

        IDE_TEST( mPartitionQueue.enqueue( ID_FALSE, (void*)&sQueueInfo )
                  != IDE_SUCCESS );

        if( sClosedRun == sRunInfoCount )
        {
            break;
        }
    } // while(1)

    IDE_TEST( iduCheckSessionEvent( mStatistics )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    IDE_PUSH();

    for( i = 0; i < sRunInfoCount; i++ )
    {
        if( sRunInfo[i].mPageHdr != NULL )
        {
            (void)sdbBufferMgr::unfixPage( mStatistics, /* idvSQL* */
                                           (UChar*)sRunInfo[i].mPageHdr );
        }
    }

    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( sHeapMap );
        case 1:
            (void)iduMemMgr::free( sRunInfo );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;    
}


IDE_RC stndrBUBuild::write2Node( sdrMtx          *aMtx,
                                 UShort           aHeight,
                                 sdpPhyPageHdr  **aPage,
                                 scPageID        *aPageID,
                                 SShort          *aSlotSeq,
                                 void            *aBuildKey,
                                 stndrNodeType    aNodeType )
{
    UInt             sNodeKeyValueLength;
    SShort           sSlotSeq;
    scPageID         sPrevPID;
    sdpPhyPageHdr  * sCurrPage;
    stndrNodeHdr   * sNodeHdr;
    idBool           sIsSuccess;
    scPageID         sCurrPID;
    scPageID         sNullPID = SD_NULL_PID;
    UChar            sIsConsistent = SDP_PAGE_CONSISTENT;
    stndrHeader    * sIndexHeader;

    IDE_ASSERT( aBuildKey != NULL );

    sIndexHeader = (stndrHeader*)mIndex->mHeader;
    sCurrPage = *aPage;
    sCurrPID  = *aPageID;

    sNodeKeyValueLength = stndrRTree::getKeyValueLength();

    if( *aPage == NULL )
    {
        IDE_TEST( allocAndLinkPage( aMtx,
                                    &sCurrPID,
                                    &sCurrPage,
                                    &sNodeHdr,
                                    NULL,
                                    &sNullPID,
                                    &sNullPID )
                  != IDE_SUCCESS );

        if( aNodeType == STNDR_NODE_LKEY )
        {
            IDE_TEST( sdnIndexCTL::init( aMtx,
                                         &sIndexHeader->mSdnHeader.mSegmentDesc.mSegHandle,
                                         sCurrPage,
                                         aHeight )
                      != IDE_SUCCESS );
        }

        IDE_TEST( stndrRTree::initializeNodeHdr( aMtx,
                                                 sCurrPage,
                                                 aHeight,
                                                 ID_TRUE )
                  != IDE_SUCCESS );

        sSlotSeq = 0;
    }
    else
    {
        sSlotSeq = *aSlotSeq;
    }

    if( sSlotSeq == mKeyCount4EmptyNode )
    {
        /*
         *  page 할당 실패로 인한 예외 발생으로 인하여
         *  log가 부분적으로만 쓰일 수 있다.
         *  따라서 미리 page가 할당 가능한지 판단해야 한다.
         */
        IDE_TEST( preparePages( 1 ) != IDE_SUCCESS );

        // Page에 충분한 공간이 없다면
        // 새로운 Page Alloc
        sPrevPID = sCurrPage->mPageID;

        // BUG-17615 로깅량 줄이기
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx, 
                                             (UChar*)sCurrPage,
                                             (void*)sCurrPage,
                                             SD_PAGE_SIZE, 
                                             SDR_SDP_BINARY )
                  != IDE_SUCCESS );

        /*
         * To fix BUG-23676
         * NOLOGGING에서의 Consistent Flag는 삽입된 키와 같이 커밋되어야 한다.
         */
        if( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( aMtx,
                                                      sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }

        IDE_TEST( allocAndLinkPage( aMtx,
                                    &sCurrPID,
                                    &sCurrPage,
                                    &sNodeHdr,
                                    sCurrPage,
                                    &sPrevPID,
                                    &sNullPID )
                  != IDE_SUCCESS );

        if( aNodeType == STNDR_NODE_LKEY )
        {
            IDE_TEST( sdnIndexCTL::init( aMtx,
                                         &sIndexHeader->mSdnHeader.mSegmentDesc.mSegHandle,
                                         sCurrPage,
                                         aHeight )
                      != IDE_SUCCESS );
        }

        IDE_TEST( stndrRTree::initializeNodeHdr( aMtx,
                                                 sCurrPage,
                                                 aHeight,
                                                 ID_TRUE )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::commit( aMtx ) != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       aMtx,
                                       mTrans,
                                       mLoggingMode,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );

        // NOLOGGING mini-transaction의 Persistent flag 설정
        // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
        sdrMiniTrans::setNologgingPersistent ( aMtx );

        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics, /* idvSQL* */
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              sCurrPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sCurrPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        sSlotSeq = 0;
    }

    IDE_ASSERT( sSlotSeq < mKeyCount4EmptyNode );

    IDE_ASSERT( gCallbackFuncs4Build[ aNodeType ].insertNodeKey( aMtx,
                                                                 (stndrHeader*)mIndex->mHeader,
                                                                 sCurrPage,
                                                                 sSlotSeq,
                                                                 sNodeKeyValueLength,
                                                                 aBuildKey )
                == IDE_SUCCESS );

    *aPage = sCurrPage;
    *aSlotSeq = sSlotSeq + 1;
    // 맨 마지막 node 의 PID 를 얻어온다.
    *aPageID = sCurrPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


idBool stndrBUBuild::isNodeFull( UChar * aPage )
{
    UChar    * sSlotDirPtr;
    SInt       sSlotCount;
    idBool     sIsFull = ID_FALSE;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aPage );
    sSlotCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

    if( sSlotCount == mKeyCount4EmptyNode )
    {
        sIsFull = ID_TRUE;
    }

    return sIsFull;
}


IDE_RC stndrBUBuild::makeNodeAndSort( UShort           aHeight,
                                      scPageID       * aLastNodePID,
                                      UInt           * aNodeCount,
                                      stndrNodeType    aNodeType )
{
    sdrMtx               sMtx;
    idBool               sIsEmpty;
    idBool               sIsClosedRun;
    idBool               sIsSuccess;
    UInt                 j;
    UInt                 sUsedBufferSize = 0;
    UInt                 sBuildKeyValueLength = 0;
    UInt                 sBuildKeyLength = 0;
    UInt                 sInsertedBuildKeyCnt = 0;
    UInt                 sNextPos = 0;
    UInt                 sLeftPos = 0;
    UInt                 sLeftCnt = 0;
    SShort               sSlotSeq = 0;
    SInt                 sSlotCount;
    UInt                 sState = 0;
    UInt                 sQueueLength;

    stndrSortedBlk       sQueueInfo;
    stndrRunInfo         sRunInfo;
    stdMBR               sNodeMBR;
    sdpPhyPageHdr      * sCurrPage = NULL;
    stndrHeader        * sIndexHeader;

    scPageID             sPrevPID;
    scPageID             sNullPID = SD_NULL_PID;
    scPageID             sCurrPID = SD_NULL_PID;
    stndrNodeHdr       * sNodeHdr;

    UChar              * sPagePtr;
    UChar              * sSlotDirPtr;
    void               * sBuildKey = NULL;
    UChar                sBuffer4Compaction[ SD_PAGE_SIZE ]; //짜투리를 저장해두는 공간
    UChar                sIsConsistent = SDP_PAGE_CONSISTENT;


    // BUG-29515: Codesonar (Uninitialized Value)
    idlOS::memset( &sRunInfo, 0x00, ID_SIZEOF(stndrRunInfo) );
    
    sIndexHeader         = (stndrHeader*)mIndex->mHeader;
    sBuildKeyValueLength = stndrRTree::getKeyValueLength();
    sBuildKeyLength      = gCallbackFuncs4Build[ STNDR_NODE_IKEY ].getBuildKeyLength(
        sBuildKeyValueLength );

    // makePartition() 에서 사용하기 위해 상위 height 의 node 갯수 계산에 이용
    mPropagatedKeyCount = 0;

    IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                   &sMtx,
                                   mTrans,
                                   mLoggingMode,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    sState = 1;

    // BUG-29568
    // NOLOGGING mini-transaction의 Persistent flag 설정
    // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
    sdrMiniTrans::setNologgingPersistent ( &sMtx );

    while(1)
    {
        sQueueLength = mPartitionQueue.getQueueLength();

        if( sQueueLength == 0 )
        {
            break;
        }

        IDE_TEST( mPartitionQueue.dequeue( ID_FALSE,
                                           (void*)&sQueueInfo,
                                           &sIsEmpty )
                  != IDE_SUCCESS );

        IDE_ASSERT( sIsEmpty != ID_TRUE );

        sRunInfo.mHeadPID = sQueueInfo.mStartPID;

        IDE_TEST( sdbBufferMgr::fixPageByPID( mStatistics, /* idvSQL* */
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              sRunInfo.mHeadPID,
                                              (UChar**)&sRunInfo.mPageHdr,
                                              &sIsSuccess )
                  != IDE_SUCCESS );

        sRunInfo.mSlotSeq = 0;

        sPagePtr    = (UChar*)sRunInfo.mPageHdr;
        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sPagePtr );
        sRunInfo.mSlotCnt = sdpSlotDirectory::getCount( sSlotDirPtr );

        IDE_DASSERT( sRunInfo.mSlotCnt > 0 );

        while(1)
        {
            sBuildKey = NULL;

            IDE_TEST( getBuildKeyFromRun( &sIsClosedRun,
                                          &sRunInfo,
                                          &sBuildKey,
                                          aNodeType )
                      != IDE_SUCCESS);

            if( sIsClosedRun == ID_TRUE )
            {
                break;
            }

            // BUG-29272: CodeSonar ( Uninitialized Variable )
            IDE_ASSERT( sBuildKey != NULL );

            if( (sCurrPage != NULL) &&
                (isNodeFull( (UChar*)sCurrPage ) == ID_TRUE) )
            {
                mPropagatedKeyCount++;

                // fix BUG-23525 InsertableKeyCnt 에 도달하면
                if( (sUsedBufferSize + sBuildKeyLength) > mKeyBufferSize )
                {
                    IDE_DASSERT( sInsertedBuildKeyCnt <= mMaxKeyMapCount );

                    // sort area의 key 정렬
                    IDE_TEST( quickSort( 0,
                                         sNextPos - 1,
                                         STNDR_NODE_IKEY,
                                         STNDR_SORT_X )
                              != IDE_SUCCESS );

                    // BUG-31024: 저장되는 노드 타입은 STNDR_NODE_IKEY 이어야한다.
                    // temp segment 에 내림
                    IDE_TEST( storeSortedRun( 0,
                                              sNextPos - 1,
                                              &sLeftPos,
                                              &sUsedBufferSize,
                                              STNDR_NODE_IKEY )
                              != IDE_SUCCESS );

                    sLeftCnt = sNextPos - sLeftPos;
                    sUsedBufferSize = 0;

                    if( sLeftCnt > 0 )
                    {
                        //KeyValue들을 맨 앞쪽으로 붙여넣기
                        for( j = 0; j < sLeftCnt; j++ )
                        {
                            swapBuildKeyMap( mKeyMap, j, (sLeftPos + j) );

                            // BUG-31024: Bottom-UP 빌드 시에 행이 걸리는 문제
                            //            Buffer Overflow로 인해 발생
                            IDE_ASSERT( (sUsedBufferSize + sBuildKeyLength)
                                        <= ID_SIZEOF(sBuffer4Compaction) );

                            /* fix BUG-23525 저장되는 키의 길이가 Varaibale하면 
                             * Fragmentation이 일어날 수 밖에 없고, 따라서 Compaction과정은 필수 */
                            idlOS::memcpy( &sBuffer4Compaction[ sUsedBufferSize ],
                                           mKeyMap[j],
                                           sBuildKeyLength );

                            mKeyMap[j] = &mKeyBuffer[ sUsedBufferSize ] ;
                            sUsedBufferSize += sBuildKeyLength;
                        }
                        idlOS::memcpy( mKeyBuffer, sBuffer4Compaction, sUsedBufferSize );

                        sInsertedBuildKeyCnt = sLeftCnt;
                    }
                    else
                    {
                        sInsertedBuildKeyCnt = 0;
                    }

                    sNextPos = sInsertedBuildKeyCnt;
                }

                IDE_DASSERT( sInsertedBuildKeyCnt < mMaxKeyMapCount );
                IDE_DASSERT( (sUsedBufferSize + sBuildKeyLength) <= mKeyBufferSize );

                // key extraction
                mKeyMap[sNextPos] = &mKeyBuffer[ sUsedBufferSize ] ;

                // 현재 node 의 새로운 MBR 을 구한다.
                IDE_ASSERT( stndrRTree::adjustNodeMBR(
                                (stndrHeader*)mIndex->mHeader,
                                sCurrPage,
                                NULL,                  /* aInsertMBR */
                                STNDR_INVALID_KEY_SEQ, /* aUpdateKeySeq */
                                NULL,                  /* aUpdateMBR */
                                STNDR_INVALID_KEY_SEQ, /* aDeleteKeySeq */
                                &sNodeMBR )
                            == IDE_SUCCESS );

                sNodeHdr = (stndrNodeHdr*)
                    sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sCurrPage );
                sNodeHdr->mMBR = sNodeMBR;
            
                STNDR_WRITE_IBUILDKEY( sCurrPage->mPageID,
                                       &sNodeMBR,
                                       sBuildKeyValueLength,
                                       mKeyMap[sNextPos] );

                sUsedBufferSize += sBuildKeyLength;
                sInsertedBuildKeyCnt++;
                sNextPos++;
            }
            
            IDE_TEST( write2Node( &sMtx,
                                  aHeight,
                                  &sCurrPage,
                                  &sCurrPID,
                                  &sSlotSeq,
                                  sBuildKey,
                                  aNodeType )
                      != IDE_SUCCESS );
        } // while(1)

        // BUG-29272: CodeSonar ( Uninitialized Variable )
        IDE_ASSERT( sCurrPage != NULL );

        /*
         * To fix BUG-23676
         * NOLOGGING에서의 Consistent Flag는 삽입된 키와 같이 커밋되어야 한다.
         */
        if( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( &sMtx,
                                                      sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sCurrPage );
        sSlotCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

        // 만약 현재 node 에 slot 이 존재하면, node MBR 을 저장한다.
        // 현재 node 상태가 full 이 아니더라도, 더 이상 slot 을 넣으면 안된다.
        if( sSlotCount > 0 )
        {
            mPropagatedKeyCount++;

            // fix BUG-23525 InsertableKeyCnt 에 도달하면
            if( (sUsedBufferSize + sBuildKeyLength) > mKeyBufferSize )
            {
                IDE_DASSERT( sInsertedBuildKeyCnt <= mMaxKeyMapCount );

                // sort area의 key 정렬
                IDE_TEST( quickSort( 0,
                                     sNextPos - 1,
                                     STNDR_NODE_IKEY,
                                     STNDR_SORT_X )
                          != IDE_SUCCESS );

                // BUG-31024: 저장되는 노드 타입은 STNDR_NODE_IKEY 이어야한다.
                // temp segment 에 내림
                IDE_TEST( storeSortedRun( 0,
                                          sNextPos - 1,
                                          &sLeftPos,
                                          &sUsedBufferSize,
                                          STNDR_NODE_IKEY )
                          != IDE_SUCCESS );

                sLeftCnt = sNextPos - sLeftPos;
                sUsedBufferSize = 0;


                if( sLeftCnt > 0 )
                {
                    //KeyValue들을 맨 앞쪽으로 붙여넣기
                    for( j = 0; j < sLeftCnt; j++ )
                    {
                        swapBuildKeyMap( mKeyMap, j, (sLeftPos + j) );

                        // BUG-31024: Bottom-UP 빌드 시에 행이 걸리는 문제
                        //            Buffer Overflow로 인해 발생
                        IDE_ASSERT( (sUsedBufferSize + sBuildKeyLength)
                                    <= ID_SIZEOF(sBuffer4Compaction) );

                        /* fix BUG-23525 저장되는 키의 길이가 Varaibale하면 
                         * Fragmentation이 일어날 수 밖에 없고, 따라서 Compaction과정은 필수 */
                        idlOS::memcpy( &sBuffer4Compaction[ sUsedBufferSize ],
                                       mKeyMap[j],
                                       sBuildKeyLength );

                        mKeyMap[j] = &mKeyBuffer[ sUsedBufferSize ] ;
                        sUsedBufferSize += sBuildKeyLength;
                    }
                    idlOS::memcpy( mKeyBuffer, sBuffer4Compaction, sUsedBufferSize );

                    sInsertedBuildKeyCnt = sLeftCnt;
                }
                else
                {
                    sInsertedBuildKeyCnt = 0;
                }

                sNextPos = sInsertedBuildKeyCnt;
            }

            IDE_DASSERT( sInsertedBuildKeyCnt < mMaxKeyMapCount );
            IDE_DASSERT( (sUsedBufferSize + sBuildKeyLength) <= mKeyBufferSize );

            // key extraction
            mKeyMap[sNextPos] = &mKeyBuffer[ sUsedBufferSize ] ;

            // 현재 node 의 새로운 MBR 을 구한다.
            IDE_ASSERT( stndrRTree::adjustNodeMBR(
                            (stndrHeader*)mIndex->mHeader,
                            sCurrPage,
                            NULL,                  /* aInsertMBR */
                            STNDR_INVALID_KEY_SEQ, /* aUpdateKeySeq */
                            NULL,                  /* aUpdateMBR */
                            STNDR_INVALID_KEY_SEQ, /* aDeleteKeySeq */
                            &sNodeMBR )
                        == IDE_SUCCESS );

            sNodeHdr = (stndrNodeHdr*)
                sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sCurrPage );
            sNodeHdr->mMBR = sNodeMBR;

            // To fix BUG-29558
            // 컴파일 최적화에 의해서 변수간 dependency가 보장되지 않습니다.
            IDL_MEM_BARRIER;

            IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx, 
                                                 (UChar*)sCurrPage,
                                                 (void*)sCurrPage,
                                                 SD_PAGE_SIZE, 
                                                 SDR_SDP_BINARY )
                      != IDE_SUCCESS );

            STNDR_WRITE_IBUILDKEY( sCurrPage->mPageID,
                                   &sNodeMBR,
                                   sBuildKeyValueLength,
                                   mKeyMap[sNextPos] );

            sUsedBufferSize += sBuildKeyLength;
            sInsertedBuildKeyCnt++;
            sNextPos++;
        } // if( sSlotCount > 0 )

        // 만약 run 이 더 존재한다면 새로운 node 를 할당받아야 한다.
        // 현재 node 상태가 full 이 아니더라도 새로운 node 를 할당받아야 한다.
        if( mPartitionQueue.getQueueLength() > 0 )
        {
            sSlotSeq = 0;
            sPrevPID = sCurrPage->mPageID;

            IDE_TEST( allocAndLinkPage( &sMtx,
                                        &sCurrPID,
                                        &sCurrPage,
                                        &sNodeHdr,
                                        sCurrPage,
                                        &sPrevPID,
                                        &sNullPID )
                      != IDE_SUCCESS );

            if( aNodeType == STNDR_NODE_LKEY )
            {
                IDE_TEST( sdnIndexCTL::init( &sMtx,
                                             &sIndexHeader->mSdnHeader.mSegmentDesc.mSegHandle,
                                             sCurrPage,
                                             aHeight )
                          != IDE_SUCCESS );
            }

            IDE_TEST( stndrRTree::initializeNodeHdr( &sMtx,
                                                     sCurrPage,
                                                     aHeight,
                                                     ID_TRUE )
                      != IDE_SUCCESS );
        }
    } // while(1)


    // 작업 공간에 남아 있는 것들 처리
    if( sNextPos > 0 )
    {
        // in-memory sort
        IDE_TEST( quickSort( 0,
                             sNextPos - 1,
                             STNDR_NODE_IKEY,
                             STNDR_SORT_X )
                  != IDE_SUCCESS );

        // BUG-31024: 저장되는 노드 타입은 STNDR_NODE_IKEY 이어야한다.
        // temp segment로 내린다.
        IDE_TEST( storeSortedRun( 0,
                                  sNextPos - 1,
                                  NULL,
                                  NULL,
                                  STNDR_NODE_IKEY )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    // BUG-29272: CodeSonar ( Uninitialized Variable )
    IDE_ASSERT( sCurrPID != SD_NULL_PID );

    *aLastNodePID = sCurrPID;
    *aNodeCount   = mPropagatedKeyCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    IDE_PUSH();

    if( sRunInfo.mPageHdr != NULL )
    {
        (void)sdbBufferMgr::unfixPage( mStatistics, /* idvSQL* */
                                       (UChar*)sRunInfo.mPageHdr );
    }

    switch( sState )
    {
        case 1 :
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx )
                        == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC stndrBUBuild::updateStatistics( idvSQL         * aStatistics,
                                       sdrMtx         * aMtx,
                                       smnIndexHeader * aIndex,
                                       SLong            /*aKeyCount*/,
                                       SLong            /*aNumDist*/ )
{
    stndrHeader     * sHeader = (stndrHeader*)aIndex->mHeader;
    idBool            sIsSuccess;
    UChar           * sPage = NULL;
    UInt              sState = 0;
    stndrNodeHdr    * sNodeHdr = NULL;
    

    IDE_TEST_RAISE( ((stndrHeader*)aIndex->mHeader)->mRootNode == SD_NULL_PID,
                    SKIP_UPDATE_INDEX_STATISTICS );
    
    IDE_TEST( sdbBufferMgr::getPageByPID(
                  aStatistics, /* idvSQL* */
                  aIndex->mIndexSegDesc.mSpaceID,
                  ((stndrHeader*)aIndex->mHeader)->mRootNode,
                  SDB_X_LATCH,
                  SDB_WAIT_NORMAL,
                  SDB_SINGLE_PAGE_READ,
                  aMtx,
                  (UChar**)&sPage,
                  &sIsSuccess,
                  NULL )
              != IDE_SUCCESS );
    sState = 1;

    sNodeHdr = (stndrNodeHdr *)sdpPhyPage::getLogicalHdrStartPtr( sPage );
    
    sHeader->mTreeMBR = sNodeHdr->mMBR;

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sPage )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_UPDATE_INDEX_STATISTICS );

    sHeader->mSdnHeader.mIsConsistent = ID_TRUE;
    smrLogMgr::getLstLSN( &(sHeader->mSdnHeader.mCompletionLSN) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdbBufferMgr::releasePage( aStatistics, (UChar*)sPage );
    }

    return IDE_FAILURE;
}
