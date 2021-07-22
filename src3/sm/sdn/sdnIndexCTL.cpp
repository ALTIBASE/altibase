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
 * $Id: sdnCTL.cpp 29513 2008-11-21 11:12:43Z upinel9 $
 **********************************************************************/

#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <sdp.h>
#include <sdc.h>
#include <sdnReq.h>


/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::init                          *
 * ------------------------------------------------------------------*
 * Touched Transaction Layer를 초기화 한다.                          *
 *********************************************************************/
IDE_RC sdnIndexCTL::init( sdrMtx         * aMtx,
                          sdpSegHandle   * aSegHandle,
                          sdpPhyPageHdr  * aPage,
                          UChar            aInitSize )
{
    sdnCTLayerHdr  * sCTLayerHdr;
    UChar            sInitSize;

    sInitSize = IDL_MAX( aSegHandle->mSegAttr.mInitTrans, aInitSize );

    IDE_ERROR( sdnIndexCTL::initLow( aPage, sInitSize )
               == IDE_SUCCESS );
    sCTLayerHdr = (sdnCTLayerHdr*)getCTL( aPage );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aPage,
                                         NULL,
                                         ID_SIZEOF( UChar ),
                                         SDR_SDN_INIT_CTL )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (UChar*)&(sCTLayerHdr->mCount),
                                   ID_SIZEOF( UChar ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::initLow                       *
 * ------------------------------------------------------------------*
 * Physical Layer에서의 CTL정보를 초기화 한다.                       *
 *********************************************************************/
IDE_RC sdnIndexCTL::initLow( sdpPhyPageHdr  * aPage,
                             UChar            aInitSize )
{
    sdnCTLayerHdr  * sCTLayerHdr;
    sdnCTS         * sArrCTS;
    UInt             i;
    idBool           sTrySuccess = ID_FALSE;

    if( sdpSlotDirectory::getSize(aPage) > 0 )
    {
        sdpPhyPage::shiftSlotDirToBottom( aPage,
                                          idlOS::align8(ID_SIZEOF(sdnCTLayerHdr)) );
    }

    sdpPhyPage::initCTL( aPage,
                         ID_SIZEOF(sdnCTLayerHdr),
                         (UChar**)&sCTLayerHdr );

    if( aInitSize > 0 )
    {
        IDE_ERROR( sdpPhyPage::extendCTL( aPage,
                               aInitSize,
                               ID_SIZEOF( sdnCTS ),
                               (UChar**)&sArrCTS,
                               &sTrySuccess )
                   == IDE_SUCCESS );
        if( sTrySuccess != ID_TRUE )
        {
            ideLog::log( IDE_SERVER_0,
                         "Init size : %u\n",
                         aInitSize );
            dumpIndexNode( aPage );
            IDE_ASSERT( 0 );
        }
    }

    sCTLayerHdr->mUsedCount = 0;
    sCTLayerHdr->mCount = aInitSize;

    for( i = 0; i < sCTLayerHdr->mCount; i++ )
    {
        idlOS::memset( &sArrCTS[i], 0x00, ID_SIZEOF(sdnCTS) );
        sArrCTS[i].mState  = SDN_CTS_DEAD;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::allocCTS                      *
 * ------------------------------------------------------------------*
 * Key의 트랜잭션 정보를 저장하기 위한 공간을 확보하는 함수이다.     *
 * 해당 함수는 Key Insertion/Deletion에 의해서 호출된다.             *
 * 5가지 Step으로 구성되어 있으며,                                   *
 *  1. 자신이 소유한 트랜잭션이 있다면                               *
 *  2. DEAD CTS가 있다면                                             *
 *  3. Agable CTS가 있다면                                           *
 *  4. Uncommitted CTS중에 Commit된 CTS가 있다면                     *
 *  5. CTL을 확장할수 있다면                                         *
 * AllocCTS는 성공한다. 그렇지 않은 경우에는 반드시 재시도를 해야    *
 * 한다. 재시도를 가능하게 하려면 절대로 rollback될 내용을 기록해서  *
 * 는 안된다.                                                        *
 *********************************************************************/
IDE_RC sdnIndexCTL::allocCTS( idvSQL             * aStatistics,
                              sdrMtx             * aMtx,
                              sdpSegHandle       * aSegHandle,
                              sdpPhyPageHdr      * aPage,
                              UChar              * aCTSlotNum,
                              sdnCallbackFuncs   * aCallbackFunc,
                              UChar              * aContext )
{
    UInt        i;
    UInt        sState  = 0;
    sdnCTL    * sCTL;
    sdnCTS    * sCTS;
    idBool      sSuccess;
    UChar       sDeadCTS;
    UChar       sAgableCTS;
    UChar     * sPageStartPtr;
    SChar     * sDumpBuf;
    smSCN       sSysMinDskViewSCN;

    *aCTSlotNum          = SDN_CTS_INFINITE;
    sDeadCTS             = SDN_CTS_INFINITE;
    sAgableCTS           = SDN_CTS_INFINITE;

    // BUG-29506 TBT가 TBK로 전환시 offset을 CTS에 반영하지 않습니다.
    // 재현하기 위해 CTS 할당 여부를 임의로 제어하기 위한 PROPERTY를 추가
    IDE_TEST_CONT( smuProperty::getDisableTransactionBoundInCTS() == 1,
                    RETURN_SUCCESS );

    sCTL = getCTL( aPage );

    SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

    /*
     * 1. 자신의 트랜잭션이 소유한 CTS가 있는 경우
     */
    for( i = 0; i < getCount(sCTL); i++ )
    {
        sCTS = getCTS( sCTL, i );

        switch( sCTS->mState )
        {
            case SDN_CTS_UNCOMMITTED:
            {
                if( isMyTransaction( aMtx->mTrans, aPage, i ) == ID_TRUE )
                {
                    *aCTSlotNum = i;
                    IDE_CONT( RETURN_SUCCESS );
                }
                break;
            }

            case SDN_CTS_DEAD:
            {
                if( (sDeadCTS == SDN_CTS_INFINITE) &&
                    (sAgableCTS == SDN_CTS_INFINITE) )
                {
                    sDeadCTS = i;
                }
                break;
            }

            case SDN_CTS_STAMPED:
            {
                if( (sDeadCTS == SDN_CTS_INFINITE) &&
                    (sAgableCTS == SDN_CTS_INFINITE) )
                {
                    if( SM_SCN_IS_LT( &sCTS->mCommitSCN,
                                      &sSysMinDskViewSCN ) )
                    {
                        IDE_TEST( aCallbackFunc->mSoftKeyStamping( aMtx,
                                                                   aPage,
                                                                   i,
                                                                   aContext )
                                  != IDE_SUCCESS );

                        sAgableCTS = i;
                    }
                }
                break;
            }

            default:
            {
                ideLog::log( IDE_SERVER_0,
                             "CTS number : %u"
                             ", CTS state : %u"
                             ", CT slot number : %u\n",
                             i, sCTS->mState, aCTSlotNum );
                dumpIndexNode( aPage );

                // BUG-28785 Case-23923의 Server 비정상 종료에 대한 디버깅 코드 추가
                ideLog::log( IDE_SERVER_0,
                             "sCTL->CTS[%u] State(%u) is invalied\n",
                             i,
                             sCTS->mState );

                sPageStartPtr = sdpPhyPage::getPageStartPtr( aPage );

                // Dump Page Hex Data And Header
                (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                             sPageStartPtr,
                                             "Dump Page:" );

                IDE_ASSERT( iduMemMgr::calloc( IDU_MEM_SM_SDN, 1,
                                               ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                               (void**)&sDumpBuf )
                            == IDE_SUCCESS );
                sState = 1;

                // Dump CTL, CTS
                (void)dump( sPageStartPtr, (SChar*)sDumpBuf, IDE_DUMP_DEST_LIMIT );

                ideLog::log( IDE_SERVER_0, "%s\n", sDumpBuf );

                sState = 0;
                IDE_ASSERT( iduMemMgr::free( sDumpBuf ) == IDE_SUCCESS );

                IDE_ASSERT( 0 );
                break;
            }
        }
    }

    /*
     * 2. DEAD상태 CTS가 존재하는 경우 CTS를 바로 재사용한다.
     */
    if( sDeadCTS != SDN_CTS_INFINITE )
    {
        *aCTSlotNum = sDeadCTS;
    }

    IDE_TEST_CONT( *aCTSlotNum != SDN_CTS_INFINITE, RETURN_SUCCESS );

    /*
     * 3. AGING이 가능한 CTS가 존재하는 경우
     *    CTS를 바로 재사용한다.
     */
    if( sAgableCTS != SDN_CTS_INFINITE )
    {
        *aCTSlotNum = sAgableCTS;
    }

    IDE_TEST_CONT( *aCTSlotNum != SDN_CTS_INFINITE, RETURN_SUCCESS );

    /*
     * 4. UNCOMMITTED상태지만, 커밋된 트랜잭션이 있는 경우
     *    delayed tts timestamping을 하고 재사용한다.
     */
    for( i = 0; i < getCount(sCTL); i++ )
    {
        sCTS = getCTS( sCTL, i );

        if( sCTS->mState == SDN_CTS_UNCOMMITTED )
        {
            sSuccess = ID_FALSE;

            IDE_TEST( aCallbackFunc->mHardKeyStamping( aStatistics,
                                                       aMtx,
                                                       aPage,
                                                       i,
                                                       aContext,
                                                       &sSuccess )
                      != IDE_SUCCESS );

            if( sSuccess == ID_TRUE )
            {
                *aCTSlotNum = i;
                break;
            }
        }
    }

    IDE_TEST_CONT( *aCTSlotNum != SDN_CTS_INFINITE, RETURN_SUCCESS );

    /*
     * 5. CTL을 확장 가능하다면 확장된 CTS를 사용한다.
     */
    IDE_TEST( extend( aMtx,
                      aSegHandle,
                      aPage,
                      ID_TRUE,
                      &sSuccess ) != IDE_SUCCESS );

    if( sSuccess == ID_TRUE )
    {
        *aCTSlotNum = getCount( sCTL ) - 1;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sDumpBuf ) == IDE_SUCCESS );
            sDumpBuf = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::allocCTS                      *
 * ------------------------------------------------------------------*
 * Key의 트랜잭션 정보를 저장하기 위한 공간을 확보하는 함수이다.     *
 * 해당 함수는 SMO(KEY 재분배/SPLIT)시에 호출된다.                   *
 * 이미 같은 트랜잭션의 정보가 있는 경우에는 해당 CTS를 사용하고     *
 * 그렇지 않은 경우에는 DEAD CTS를 재사용한다.                       *
 * SMO시에 CTS를 위한 공간을 미리 할당받았기 때문에 해당 함수가 실패 *
 * 하는 경우는 없어야 한다.                                          *
 *********************************************************************/
IDE_RC sdnIndexCTL::allocCTS( sdpPhyPageHdr * aSrcPage,
                              UChar           aSrcCTSlotNum,
                              sdpPhyPageHdr * aDstPage,
                              UChar         * aDstCTSlotNum )
{
    sdnCTL  * sDstCTL;
    sdnCTL  * sSrcCTL;
    sdnCTS  * sDstCTS;
    sdnCTS  * sSrcCTS;
    UChar     sDeadCTS;
    UInt      i;

    sDstCTL = getCTL( aDstPage );
    sSrcCTL = getCTL( aSrcPage );

    sSrcCTS = getCTS( sSrcCTL, aSrcCTSlotNum );

    sDeadCTS = SDN_CTS_INFINITE;
    *aDstCTSlotNum = SDN_CTS_INFINITE;

    /*
     * 1. 같은 트랜잭션이 있는지 검사
     */
    for( i = 0; i < getCount(sDstCTL); i++ )
    {
        sDstCTS = getCTS( sDstCTL, i );

        if( sDstCTS->mState != SDN_CTS_DEAD )
        {
            if( isSameTransaction( sDstCTS, sSrcCTS ) == ID_TRUE )
            {
                *aDstCTSlotNum = i;
                break;
            }
        }
        else
        {
            if( sDeadCTS == SDN_CTS_INFINITE )
            {
                sDeadCTS = i;
            }
        }
    }

    IDE_TEST_CONT( *aDstCTSlotNum != SDN_CTS_INFINITE, RETURN_SUCCESS );

    /*
     * 2. DEAD상태 CTS가 존재하는 경우 CTS를 바로 재사용한다.
     */
    if( sDeadCTS != SDN_CTS_INFINITE )
    {
        *aDstCTSlotNum = sDeadCTS;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::bindCTS                       *
 * ------------------------------------------------------------------*
 * Key Insertion/Deletion에 의해서 호출되며,                         *
 * 트랜잭션 정보를 해당 CTS에 설정한다.                              *
 * DEAD상태인 경우는 초기화후 재사용한다.                            *
 * 해당 함수를 수행한 이후에는 절대로 연산이 롤백되어서는 안된다.    *
 *********************************************************************/
IDE_RC sdnIndexCTL::bindCTS( sdrMtx           * aMtx,
                             scSpaceID          aSpaceID,
                             sdpPhyPageHdr    * aPage,
                             UChar              aCTSlotNum,
                             UShort             aKeyOffset )
{
    sdnCTL * sCTL;
    sdnCTS * sCTS;
    sdSID    sTSSlotSID;
    UChar    sUsedCount;
    UInt     i;

    sCTL       = getCTL( aPage );
    sCTS       = getCTS( aPage, aCTSlotNum );
    sTSSlotSID = smLayerCallback::getTSSlotSID( aMtx->mTrans );

    /* BUG-48064 : CTS CHAING 기능을 제거했으므로 STAMPED 상태의 CTS는 올수없다. */
    IDE_DASSERT(sCTS->mState != SDN_CTS_STAMPED);

    if ( sCTS->mState == SDN_CTS_DEAD )
    {
        sUsedCount = sCTL->mUsedCount + 1;
        if ( sUsedCount >= SDN_CTS_INFINITE )
        {
            ideLog::log( IDE_SERVER_0,
                         "CT slot number : %u"
                         "\nUsed count : %u\n",
                         aCTSlotNum, sUsedCount );
            dumpIndexNode( aPage );
            IDE_ASSERT( 0 );
        }

        sCTS->mCommitSCN = smLayerCallback::getFstDskViewSCN( aMtx->mTrans );
        sCTS->mState     = SDN_CTS_UNCOMMITTED;
        sCTS->mRefCnt    = 1;

        idlOS::memset( sCTS->mRefKey,
                       0x00,
                       ID_SIZEOF(UShort) * SDN_CTS_MAX_KEY_CACHE );
        sCTS->mRefKey[0] = aKeyOffset;

        sCTS->mTSSlotPID = SD_MAKE_PID( sTSSlotSID );
        sCTS->mTSSlotNum = SD_MAKE_SLOTNUM( sTSSlotSID );

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&(sCTL->mUsedCount),
                                            (void*)&sUsedCount,
                                            ID_SIZEOF(UChar) )
                  != IDE_SUCCESS );
    }
    else
    {
        /*
         * UNCOMMITTED상태의 CTS는 Cache정보만을 갱신한다.
         */
        for( i = 0; i < SDN_CTS_MAX_KEY_CACHE; i++ )
        {
            if( sCTS->mRefKey[i] == SDN_CTS_KEY_CACHE_NULL )
            {
                sCTS->mRefKey[i] = aKeyOffset;
                break;
            }
        }
        sCTS->mRefCnt++;
    }

    if( sCTS->mRefCnt == 1 )
    {
        IDE_TEST( smLayerCallback::addTouchedPage( aMtx->mTrans,
                                                   aSpaceID,
                                                   aPage->mPageID,
                                                   (SShort)aCTSlotNum )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)sCTS,
                                        (void*)sCTS,
                                        ID_SIZEOF(sdnCTS),
                                        SDR_SDP_BINARY )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::bindCTS                       *
 * ------------------------------------------------------------------*
 * SMO(Key 재분배/SPLIT)에 의해서 호출되며,                          *
 * 트랜잭션 정보를 해당 CTS에 설정한다.                              *
 * DEAD상태인 경우는 초기화후 재사용한다.                            *
 *********************************************************************/
IDE_RC sdnIndexCTL::bindCTS( sdrMtx        * aMtx,
                             scSpaceID       aSpaceID,
                             UShort          aKeyOffset,
                             sdpPhyPageHdr * aSrcPage,
                             UChar           aSrcCTSlotNum,
                             sdpPhyPageHdr * aDstPage,
                             UChar           aDstCTSlotNum )
{
    sdnCTL * sDstCTL;
    sdnCTS * sDstCTS;
    sdnCTS * sSrcCTS;
    UChar    sUsedCount;
    UInt     i;

    sDstCTL       = getCTL( aDstPage );
    sDstCTS       = getCTS( aDstPage, aDstCTSlotNum );
    sSrcCTS       = getCTS( aSrcPage, aSrcCTSlotNum );

    if( sDstCTS->mState == SDN_CTS_DEAD )
    {
        idlOS::memcpy( sDstCTS, sSrcCTS, ID_SIZEOF(sdnCTS) );

        sDstCTS->mRefCnt = 1;

        idlOS::memset( sDstCTS->mRefKey,
                       0x00,
                       ID_SIZEOF(UShort) * SDN_CTS_MAX_KEY_CACHE );
        sDstCTS->mRefKey[0] = aKeyOffset;

        sUsedCount = sDstCTL->mUsedCount + 1;
        if( sUsedCount >= SDN_CTS_INFINITE )
        {
            ideLog::log( IDE_SERVER_0,
                         "Source CT slot number : %u"
                         ", Destination CT slot number : %u"
                         "\nUsed count : %u\n",
                         aSrcCTSlotNum, aDstCTSlotNum, sUsedCount );
            dumpIndexNode( aSrcPage );
            dumpIndexNode( aDstPage );
            IDE_ASSERT( 0 );
        }

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&(sDstCTL->mUsedCount),
                                            (void*)&sUsedCount,
                                            ID_SIZEOF(UChar) )
                  != IDE_SUCCESS );
    }
    else
    {
        for( i = 0; i < SDN_CTS_MAX_KEY_CACHE; i++ )
        {
            if( sDstCTS->mRefKey[i] == SDN_CTS_KEY_CACHE_NULL )
            {
                sDstCTS->mRefKey[i] = aKeyOffset;
                break;
            }
        }
        sDstCTS->mRefCnt++;
    }

    if( isMyTransaction( aMtx->mTrans,
                         aSrcPage,
                         aSrcCTSlotNum ) == ID_TRUE )
    {
        if( sDstCTS->mRefCnt == 1 )
        {
            IDE_TEST( smLayerCallback::addTouchedPage( aMtx->mTrans,
                                                       aSpaceID,
                                                       aDstPage->mPageID,
                                                       (SShort)aDstCTSlotNum )
                      != IDE_SUCCESS );
        }
    }

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)sDstCTS,
                                        (void*)sDstCTS,
                                        ID_SIZEOF(sdnCTS),
                                        SDR_SDP_BINARY )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::bindChainedCTS                *
 * ------------------------------------------------------------------*
 * Chain을 갖는 Dummy CTS를 만든다.                                  *
 * 이러한 경우는 SourceCTS가 Chain상에 있는 경우에 호출될수 있다.    *
 *********************************************************************/
IDE_RC sdnIndexCTL::bindChainedCTS( sdrMtx        * aMtx,
                                    scSpaceID       aSpaceID,
                                    sdpPhyPageHdr * aSrcPage,
                                    UChar           aSrcCTSlotNum,
                                    sdpPhyPageHdr * aDstPage,
                                    UChar           aDstCTSlotNum )
{
    sdnCTL * sDstCTL;
    sdnCTS * sDstCTS;
    sdnCTS * sSrcCTS;
    UChar    sUsedCount;

    sDstCTL       = getCTL( aDstPage );
    sDstCTS       = getCTS( aDstPage, aDstCTSlotNum );
    sSrcCTS       = getCTS( aSrcPage, aSrcCTSlotNum );

    if( sDstCTS->mState == SDN_CTS_DEAD )
    {
        /*
         * reference count가 0인 Dummy CTS를 만든다.
         */
        idlOS::memcpy( sDstCTS, sSrcCTS, ID_SIZEOF(sdnCTS) );

        sDstCTS->mRefCnt = 0;

        idlOS::memset( sDstCTS->mRefKey,
                       0x00,
                       ID_SIZEOF(UShort) * SDN_CTS_MAX_KEY_CACHE );

        sUsedCount = sDstCTL->mUsedCount + 1;
        if( sUsedCount >= SDN_CTS_INFINITE )
        {
            ideLog::log( IDE_SERVER_0,
                         "Source CT slot number : %u"
                         ", Destination CT slot number : %u"
                         "\nUsed count : %u\n",
                         aSrcCTSlotNum, aDstCTSlotNum, sUsedCount );
            dumpIndexNode( aSrcPage );
            dumpIndexNode( aDstPage );
            IDE_ASSERT( 0 );
        }

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&(sDstCTL->mUsedCount),
                                            (void*)&sUsedCount,
                                            ID_SIZEOF(UChar) )
                  != IDE_SUCCESS );

        if( isMyTransaction( aMtx->mTrans,
                             aSrcPage,
                             aSrcCTSlotNum ) == ID_TRUE )
        {
            IDE_TEST( smLayerCallback::addTouchedPage( aMtx->mTrans,
                                                       aSpaceID,
                                                       aDstPage->mPageID,
                                                       (SShort)aDstCTSlotNum )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /*
         * Chained CTS라면 Source Chain의 정보를 DstCTS에 설정한다.
         */
        if( hasChainedCTS( sSrcCTS ) == ID_TRUE )
        {
            SM_SET_SCN( &sDstCTS->mNxtCommitSCN, &sSrcCTS->mNxtCommitSCN );

            sDstCTS->mUndoPID = sSrcCTS->mUndoPID;
            sDstCTS->mUndoSlotNum = sSrcCTS->mUndoSlotNum;
        }
    }

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)sDstCTS,
                                        (void*)sDstCTS,
                                        ID_SIZEOF(sdnCTS),
                                        SDR_SDP_BINARY )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::freeCTS                       *
 * ------------------------------------------------------------------*
 * CTS를 DEAD상태로 만든다.                                          *
 *********************************************************************/
IDE_RC sdnIndexCTL::freeCTS( sdrMtx        * aMtx,
                             sdpPhyPageHdr * aPage,
                             UChar           aSlotNum,
                             idBool          aLogging )
{
    sdnCTL * sCTL;
    sdnCTS * sCTS;

    sCTL = getCTL( aPage );
    sCTS = getCTS( aPage, aSlotNum );

    /* BUG-38304 이미 DEAD로 처리된 CTS에 다시 freeCTS가 호출될 경우 
     * CTS와 CTL간 정보 불일치가 발생할 수 있다. */
    IDE_ASSERT( sCTS->mState != SDN_CTS_DEAD );

    setCTSlotState( sCTS, SDN_CTS_DEAD );
    sCTL->mUsedCount--;

    if( sCTL->mUsedCount >= SDN_CTS_INFINITE )
    {
        ideLog::log( IDE_SERVER_0,
                     "CT slot number : %u"
                     "\nUsed count : %u\n",
                     aSlotNum, sCTL->mUsedCount );
        dumpIndexNode( aPage );
        IDE_ASSERT( 0 );
    }

    if( aLogging == ID_TRUE )
    {
        IDE_TEST( logFreeCTS( aMtx, aPage, aSlotNum ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::freeCTS                       *
 * ------------------------------------------------------------------*
 * CTS가 DEAD상태가 되었음을 로깅한다.                               *
 *********************************************************************/
IDE_RC sdnIndexCTL::logFreeCTS( sdrMtx        * aMtx,
                                sdpPhyPageHdr * aPage,
                                UChar           aSlotNum )
{
    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)aPage,
                                        NULL,
                                        ID_SIZEOF(UChar),
                                        SDR_SDN_FREE_CTS )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&aSlotNum,
                                   ID_SIZEOF(UChar))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::extend                        *
 * ------------------------------------------------------------------*
 * CTL의 크기를 1만큼 확장한다..                                     *
 *********************************************************************/
IDE_RC sdnIndexCTL::extend( sdrMtx        * aMtx,
                            sdpSegHandle  * aSegHandle,
                            sdpPhyPageHdr * aPage,
                            idBool          aLogging,
                            idBool        * aSuccess )
{
    sdnCTL * sCTL;
    sdnCTS * sCTS;
    UChar    sCount;
    idBool   sTrySuccess = ID_FALSE;

    // BUG-29506 TBT가 TBK로 전환시 offset을 CTS에 반영하지 않습니다.
    // 재현하기 위해 CTS 할당 여부를 임의로 제어하기 위한 PROPERTY를 추가
    *aSuccess = ID_FALSE;
    IDE_TEST_CONT( smuProperty::getDisableTransactionBoundInCTS() == 1,
                    RETURN_SUCCESS );

    sCTL = getCTL( aPage );

    sCount = sCTL->mCount;

    if( (aSegHandle == NULL) ||
        (sCount < aSegHandle->mSegAttr.mMaxTrans) )
    {
        IDE_ERROR( sdpPhyPage::extendCTL( aPage,
                                          1,
                                          ID_SIZEOF(sdnCTS),
                                          (UChar**)&sCTS,
                                          &sTrySuccess )
                   == IDE_SUCCESS );

        if ( sTrySuccess == ID_TRUE )
        {
            idlOS::memset( sCTS, 0x00, ID_SIZEOF(sdnCTS) );
            sCTS->mState = SDN_CTS_DEAD;

            sCTL->mCount++;

            if( aLogging == ID_TRUE )
            {
                IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                                    (UChar*)aPage,
                                                    NULL,
                                                    ID_SIZEOF(UChar),
                                                    SDR_SDN_EXTEND_CTL )
                          != IDE_SUCCESS );

                IDE_TEST( sdrMiniTrans::write( aMtx,
                                               (void*)&sCount,
                                               ID_SIZEOF(UChar))
                          != IDE_SUCCESS );
            }

            *aSuccess = ID_TRUE;
        }
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::fastStamping                  *
 * ------------------------------------------------------------------*
 * Transaction Commit시에 호출되며, CTS에 CommitSCN을 설정하고 상태  *
 * 를 STAMPED로 변경한다.                                            *
 *********************************************************************/
IDE_RC sdnIndexCTL::fastStamping( void   * aTrans,
                                  UChar  * aPage,
                                  UChar    aSlotNum,
                                  smSCN  * aCommitSCN )
{
    sdnCTS    * sCTS;

    // BUG-29442: stamping 대상 Page가 free된 후 재사용될 경우
    //            잘못된 Stamping을 수행할 수 있다.

    IDE_TEST_CONT( aSlotNum >= getCount( (sdpPhyPageHdr*)aPage ),
                    SKIP_STAMPING );

    IDE_TEST_CONT( ((sdpPhyPageHdr*)aPage)->mSizeOfCTL == 0,
                    SKIP_STAMPING );

    sCTS = getCTS( (sdpPhyPageHdr*)aPage, aSlotNum );

    if( getCTSlotState( sCTS ) == SDN_CTS_UNCOMMITTED )
    {
        if( isMyTransaction( aTrans,
                             (sdpPhyPageHdr*)aPage,
                             aSlotNum ) == ID_TRUE )
        {
            sCTS->mCommitSCN = *aCommitSCN;
            sCTS->mState = SDN_CTS_STAMPED;
        }
    }

    IDE_EXCEPTION_CONT( SKIP_STAMPING );

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::fastStampingAll               
 * -------------------------------------------------------------------
 * Description : DRDBRedo Valdation을 위해 FastStamping을 시도합니다.
 *
 * 1) Stamping을 했느냐 하지 않았느냐에 따라 SCN이 다를 수 있기 때문이니다.
 * 2) Stamping 되었어도 다시 Stamping을 시도합니다. TSS가 완전히 재활용
 *    되어 CommitSCN이 0일 수도 있기 때문입니다.
 * 3) 두 페이지를 동시에 Stamping합니다. 두번에 걸쳐 Stamping하면
 *    getCommitSCN의 타이밍에 따라, 한쪽은 Stamping 되면서 다른 한쪽은
 *    Stamping 안될 수도 있기 때문입니다.
 *
 * aStatistics    - [IN] Dummy통계정보
 * aPagePtr1      - [IN] 페이지 헤더 시작 포인터
 * aPagePtr2      - [IN] 페이지 헤더 시작 포인터
 *********************************************************************/
IDE_RC sdnIndexCTL::stampingAll4RedoValidation( idvSQL * aStatistics,
                                                UChar  * aPage1,
                                                UChar  * aPage2 )
{
    sdnCTL *  sCTL1;
    sdnCTS *  sCTS1;
    sdnCTL *  sCTL2;
    sdnCTS *  sCTS2;
    sdSID     sTSSlotSID;
    smTID     sDummyTID4Wait;
    smSCN     sCommitSCN;
    UInt      i;

    /* Internal Node에는 CTS가 없으며, 타 모듈에서는 이를 구별할 수 없음.
     * 따라서 Index 내부에서 구별해서, CTS가 있는 경우에만 처리함 */
    IDE_TEST_CONT( sdpPhyPage::getSizeOfCTL( ((sdpPhyPageHdr*)aPage1) ) == 0,
                    SKIP );

    sCTL1 = sdnIndexCTL::getCTL( (sdpPhyPageHdr*) aPage1);
    sCTL2 = sdnIndexCTL::getCTL( (sdpPhyPageHdr*) aPage2);

    IDE_ERROR( sdnIndexCTL::getCount(sCTL1) == sdnIndexCTL::getCount(sCTL2) );

    for( i = 0; i < sdnIndexCTL::getCount(sCTL1); i++ )
    {
        sCTS1 = sdnIndexCTL::getCTS( sCTL1, i );
        sCTS2 = sdnIndexCTL::getCTS( sCTL2, i );

        /* Stamping 된 상태라도, 다시 Stampnig을 합니다.
         * 왜냐하면 TSS가 완전히 재활용되어 CommtSCN이 0인 상태일 수도
         * 있기 때문입니다. */
        if( ( sdnIndexCTL::getCTSlotState( sCTS1 ) == SDN_CTS_UNCOMMITTED ) ||
            ( sdnIndexCTL::getCTSlotState( sCTS1 ) == SDN_CTS_STAMPED ) )
        {
            IDE_ERROR( ( sdnIndexCTL::getCTSlotState( sCTS2 ) == SDN_CTS_UNCOMMITTED ) ||
                       ( sdnIndexCTL::getCTSlotState( sCTS2 ) == SDN_CTS_STAMPED ) );

            sTSSlotSID = SD_MAKE_SID( sCTS1->mTSSlotPID, sCTS1->mTSSlotNum );

            IDE_ERROR( sTSSlotSID == 
                       SD_MAKE_SID( sCTS2->mTSSlotPID, sCTS2->mTSSlotNum ) );

            IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                                  NULL,        /* aTrans */
                                                  sTSSlotSID,
                                                  &sCTS1->mCommitSCN,
                                                  SM_SCN_INIT, /* aStmtViewSCN */
                                                  &sDummyTID4Wait, 
                                                  &sCommitSCN )
                      != IDE_SUCCESS );
            if( SM_SCN_IS_NOT_INFINITE( sCommitSCN ) )
            {
                sCTS1->mCommitSCN = sCommitSCN;
                sCTS1->mState     = SDN_CTS_STAMPED;
                sCTS2->mCommitSCN = sCommitSCN;
                sCTS2->mState     = SDN_CTS_STAMPED;
            }
        }
        else
        {
            /* 불필요한 Diff가 없도록 초기화한다. */
            IDE_ERROR( ( sdnIndexCTL::getCTSlotState( sCTS1 ) == SDN_CTS_NONE ) || 
                       ( sdnIndexCTL::getCTSlotState( sCTS1 ) == SDN_CTS_DEAD ) );
            IDE_ERROR( ( sdnIndexCTL::getCTSlotState( sCTS2 ) == SDN_CTS_NONE ) || 
                       ( sdnIndexCTL::getCTSlotState( sCTS2 ) == SDN_CTS_DEAD ) );

            idlOS::memset( sCTS1, 0, ID_SIZEOF( sdnCTS ) );
            sCTS1->mState  = SDN_CTS_DEAD;
            idlOS::memset( sCTS2, 0, ID_SIZEOF( sdnCTS ) );
            sCTS2->mState  = SDN_CTS_DEAD;
        }
    }

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::delayedStamping               *
 * ------------------------------------------------------------------*
 * 다른 Transaction(SELECT/DML)에 의해서 수행되며, CTS에 CommitSCN을 *
 * 설정하고 상태 를 STAMPED로 변경한다.                              *
 *                                                                   *
 * BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다      *
 *            Aging 시에 MPR을 이용하기 때문에 Page Read Mode가 필요 * 
 *********************************************************************/
IDE_RC sdnIndexCTL::delayedStamping( idvSQL          * aStatistics,
                                     void            * aTrans,
                                     sdnCTS          * aCTS,
                                     sdbPageReadMode   aPageReadMode,
                                     smSCN             aStmtViewSCN,
                                     smSCN           * aCommitSCN,
                                     idBool          * aSuccess )
{
    sdSID         sTSSlotSID;
    smTID         sDummyTID4Wait;
    UChar       * sPageHdr;
    idBool        sTrySuccess = ID_FALSE;

    IDE_DASSERT( aCTS->mState == SDN_CTS_UNCOMMITTED );
     
    sTSSlotSID = SD_MAKE_SID( aCTS->mTSSlotPID, aCTS->mTSSlotNum );

    IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                          (smxTrans*)aTrans,
                                          sTSSlotSID,
                                          &aCTS->mCommitSCN,
                                          aStmtViewSCN,
                                          &sDummyTID4Wait, 
                                          aCommitSCN )
              != IDE_SUCCESS );

    if( SM_SCN_IS_NOT_INFINITE( *aCommitSCN ) )
    {
        sPageHdr = sdpPhyPage::getPageStartPtr( aCTS );

        sdbBufferMgr::tryEscalateLatchPage( aStatistics,
                                            sPageHdr,
                                            aPageReadMode,
                                            &sTrySuccess );

        if( sTrySuccess == ID_TRUE )
        {
            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sPageHdr );
            aCTS->mState = SDN_CTS_STAMPED;
            aCTS->mCommitSCN = *aCommitSCN;
            *aSuccess = ID_TRUE;
        }
        else
        {
            *aSuccess = ID_FALSE;
        }
    }
    else
    {
        *aSuccess = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCTL                        *
 * ------------------------------------------------------------------*
 * CTL Header를 얻는다.                                              *
 *********************************************************************/
sdnCTL* sdnIndexCTL::getCTL( sdpPhyPageHdr  * aPage )
{
    return (sdnCTL*)sdpPhyPage::getCTLStartPtr( (UChar*)aPage );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCTS                        *
 * ------------------------------------------------------------------*
 * CTS Pointer를 얻는다.                                             *
 *********************************************************************/
sdnCTS* sdnIndexCTL::getCTS( sdnCTL  * aCTL,
                             UChar     aSlotNum )
{
    IDE_DASSERT( aSlotNum < aCTL->mCount );

    return &(aCTL->mArrCTS[ aSlotNum ]);
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCTS                        *
 * ------------------------------------------------------------------*
 * CTS Pointer를 얻는다.                                             *
 *********************************************************************/
sdnCTS* sdnIndexCTL::getCTS( sdpPhyPageHdr  * aPage,
                             UChar            aSlotNum )
{
    return getCTS( getCTL(aPage), aSlotNum );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCount                      *
 * ------------------------------------------------------------------*
 * CTL의 크기를 리턴한다.                                            *
 *********************************************************************/
UChar sdnIndexCTL::getCount( sdnCTL  * aCTL )
{
    return aCTL->mCount;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCount                      *
 * ------------------------------------------------------------------*
 * CTL의 크기를 리턴한다.                                            *
 *********************************************************************/
UChar sdnIndexCTL::getCount( sdpPhyPageHdr  * aPage )
{
    return getCount( getCTL(aPage) );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCount                      *
 * ------------------------------------------------------------------*
 * Active CTS의 개수를 리턴한다.                                     *
 *********************************************************************/
UChar sdnIndexCTL::getUsedCount( sdnCTL  * aCTL )
{
    return aCTL->mUsedCount;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCount                      *
 * ------------------------------------------------------------------*
 * Active CTS의 개수를 리턴한다.                                     *
 *********************************************************************/
UChar sdnIndexCTL::getUsedCount( sdpPhyPageHdr  * aPage )
{
    return getUsedCount( getCTL(aPage) );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCTSlotState                *
 * ------------------------------------------------------------------*
 * CTS의 상태를 리턴한다.                                            *
 *********************************************************************/
UChar sdnIndexCTL::getCTSlotState( sdpPhyPageHdr * aPage,
                                   UChar           aSlotNum )
{
    sdnCTS * sCTS;

    sCTS = getCTS( aPage, aSlotNum );

    return getCTSlotState( sCTS );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCTSlotState                *
 * ------------------------------------------------------------------*
 * CTS의 상태를 리턴한다.                                            *
 *********************************************************************/
UChar sdnIndexCTL::getCTSlotState( sdnCTS  * aCTS )
{
    return aCTS->mState;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::setCTSlotState                *
 * ------------------------------------------------------------------*
 * CTS의 상태를 설정한다.                                            *
 *********************************************************************/
void sdnIndexCTL::setCTSlotState( sdnCTS * aCTS,
                                  UChar    aState )
{
    aCTS->mState = aState;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCommitSCN                  *
 * ------------------------------------------------------------------*
 * CTS.CommitSCN을 리턴한다.                                         *
 *********************************************************************/
smSCN sdnIndexCTL::getCommitSCN( sdnCTS  * aCTS )
{
    return (aCTS->mCommitSCN);
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCommitSCN                  *
 * ------------------------------------------------------------------*
 * Transaction의 CommitSCN을 구한다.                                 *
 * UNCOMMITTED 상태라면 Delayed Stamping을 통해서 CommitSCN을 얻어야 *
 * 한다.                                                             *
 *********************************************************************/
IDE_RC sdnIndexCTL::getCommitSCN( idvSQL           * aStatistics,
                                  void             * aTrans,
                                  sdpPhyPageHdr    * aPage,
                                  sdbPageReadMode    aPageReadMode,
                                  UChar              aCTSlotNum,
                                  smSCN              aStmtViewSCN,
                                  smSCN            * aCommitSCN )
{
    sdnCTS    * sCTS;
    idBool      sSuccess;

    sCTS = getCTS( aPage, aCTSlotNum );

    if ( sCTS->mState == SDN_CTS_STAMPED )
    {
        SM_GET_SCN(aCommitSCN, &sCTS->mCommitSCN);
    }
    else
    {
        IDE_TEST( delayedStamping( aStatistics,
                                   aTrans,
                                   sCTS,
                                   aPageReadMode,
                                   aStmtViewSCN,
                                   aCommitSCN,
                                   &sSuccess )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCommitSCNfromUndo          *
 * ------------------------------------------------------------------*
 * Undo Chain상에 저장되어 있는 CommitSCN을 구한다. Key가 저장되어   *
 * 있는 CTS를 찾아야 하고,                                           *
 * 그러한 CTS는 [ROW_PID, ROW_SLOTNUM, KEY_VALUE, WHICH_CTS]를       *
 * 가지고 찾는다. 같은 [ROW_PID, ROW_SLOTNUM, KEY_VALUE]를 갖는 키가 *
 * Chain상에 2개(Create/Limit)가 존재할수 있기 때문에 WHICH_CTS를    *
 * 가지고 Create인지 Limit인지 확인해야 한다.                        *
 *********************************************************************/
IDE_RC sdnIndexCTL::getCommitSCNfromUndo( idvSQL           * aStatistics,
                                          sdpPhyPageHdr    * aPage,
                                          sdnCTS           * aCTS,
                                          UChar              aCTSlotNum,
                                          smSCN            * aStmtViewSCN,
                                          sdnCallbackFuncs * aCallbackFunc,
                                          UChar            * aContext,
                                          idBool             aIsCreateSCN,
                                          smSCN            * aCommitSCN )
{
    sdnCTS   sCTS;
    UChar    sChainedCCTS   = SDN_CHAINED_NO;
    UChar    sChainedLCTS   = SDN_CHAINED_NO;
    UShort   sKeyListSize;
    ULong    sKeyList[SD_PAGE_SIZE / ID_SIZEOF(ULong)];
    idBool   sIsReusedUndoRec;

    idlOS::memcpy( &sCTS, aCTS, ID_SIZEOF( sdnCTS ) );

    while( 1 )
    {
        if( hasChainedCTS( &sCTS ) != ID_TRUE )
        {
            SM_INIT_SCN( aCommitSCN );
            break;
        }

        /* FIT/ART/sm/Bugs/BUG-29839/BUG-29839.sql */
        IDU_FIT_POINT( "1.BUG-29839@sdnIndexCTL::getCommitSCNfromUndo" );

        IDE_TEST( getChainedCTS( aStatistics,
                                 NULL, //aMtx
                                 aPage,
                                 aCallbackFunc,
                                 aContext,
                                 &(sCTS.mNxtCommitSCN),
                                 aCTSlotNum,
                                 ID_FALSE, // Try Hard Stamping
                                 &sCTS,
                                 (UChar*)sKeyList,
                                 &sKeyListSize,
                                 &sIsReusedUndoRec ) != IDE_SUCCESS );

        /* BUG-29839 BTree에서 재사용된 undo page에 접근할 수 있습니다.
         * chain된 CTS의 commit SCN을 구하는 과정에서 다른 tx에 의해
         * chain된 CTS의 undo page가 재사용되었을 수 있습니다.
         * 이로 인한 오류를 방지하기 위해 기존에 읽은 CTS의 mNxtCommitSCN
         * 재사용 조건인 sysMinDskSCN보다 큰 경우에는 무조건 INIT_SCN 으로
         * 판단하여 페이지에 대한 접근을 하지 않도록 합니다.
         * 이는 페이지에 대한 S Latch를 잡은 후에 비교하여야 합니다.*/

        if( sIsReusedUndoRec == ID_TRUE )
        {
            SM_INIT_SCN( aCommitSCN );
            break;
        }

        if( aCallbackFunc->mFindChainedKey( NULL,
                                            &sCTS,
                                            (UChar*)sKeyList,
                                            sKeyListSize,
                                            &sChainedCCTS,
                                            &sChainedLCTS,
                                            aContext ) == ID_TRUE )
        {
            /*
             * Chain상에는 같은 [ROW_PID, ROW_SLOTNUM, KEY_VALUE]를 갖는 Key들이
             * 존재할수 있기 때문에 WHICH_CHAIN인지를 구분해야 한다.
             * 만약 CreateCTS를 위한 Chain이라면 반드시 ChainedCCTS가
             * SDN_CHAINED_YES로 설정되어 있어야만 한다.
             */
            if ( ((aIsCreateSCN == ID_TRUE) &&
                  (sChainedCCTS == SDN_CHAINED_YES)) ||
                 ((aIsCreateSCN == ID_FALSE) &&
                  (sChainedLCTS == SDN_CHAINED_YES)) )
            {
                *aCommitSCN = sCTS.mCommitSCN;
                break;
            }
        }
        else
        {
            if( SM_SCN_IS_LT( &sCTS.mNxtCommitSCN, aStmtViewSCN ) )
            {
                *aCommitSCN = sCTS.mNxtCommitSCN;
                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCommitSCN                  *
 * ------------------------------------------------------------------*
 * CTS.CommitSCN을 리턴한다.                                         *
 *********************************************************************/
smSCN sdnIndexCTL::getCommitSCN( sdpPhyPageHdr * aPage,
                                 UChar           aSlotNum )
{
    sdnCTS * sCTS;

    sCTS = getCTS( aPage, aSlotNum );

    return getCommitSCN( sCTS );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::unbindCTS                     *
 * ------------------------------------------------------------------*
 * Key가 가리키는 Transaction정보의 참조개수를 감소시킨다. 만약 참조 *
 * 개수가 0이 되는 경우는 CTS를 DEAD상태로 만든다.
 *********************************************************************/
IDE_RC sdnIndexCTL::unbindCTS( sdrMtx           * aMtx,
                               sdpPhyPageHdr    * aPage,
                               UChar              aSlotNum,
                               UShort             aKeyOffset )
{
    sdnCTS  * sCTS;
    UShort    sRefCnt;
    SInt      i;

    sCTS = getCTS( aPage, aSlotNum );
    sRefCnt = sCTS->mRefCnt - 1;

    if ( sRefCnt == 0 )
    {
        IDE_TEST( freeCTS( aMtx,
                           aPage,
                           aSlotNum,
                           ID_TRUE )
                  != IDE_SUCCESS );

        IDE_CONT( SKIP_ADJUST_REFKEY );
    }

    for( i = SDN_CTS_MAX_KEY_CACHE - 1; i >= 0 ; i-- )
    {
        if( sCTS->mRefKey[i] == aKeyOffset )
        {
            sCTS->mRefKey[i] = SDN_CTS_KEY_CACHE_NULL;
            break;
        }
    }

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(sCTS->mRefCnt),
                                         &sRefCnt,
                                         ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    /* BUG-48064
       writeNBytes() 함수는 1bytes,2bytes,4bytes,8bytes만 길이로 받는다.
       UChar mRefKey[3]을 저장하기위해 4bytes, 2bytes 길이로 두번 호출한다.
       만약, SDN_CTS_MAX_KEY_CACHE 를 변경하게 되면 이곳도 수정해야한다.
     */
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)sCTS->mRefKey,
                                         (UChar*)sCTS->mRefKey,
                                         ID_SIZEOF(UShort)*2 )
              != IDE_SUCCESS );
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)((sCTS->mRefKey) + 2),
                                         (UChar*)((sCTS->mRefKey) + 2),
                                         ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_ADJUST_REFKEY );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::makeChainedCTS                *
 * ------------------------------------------------------------------*
 * Unchained Key를 Chained Key로 만들고, 해당 키 리스트를 undo page  *
 * 에 기록한다.                                                      *
 * Undo Format ==> | sdnCTS | KeyListSize | KeyList(Key1,Key2,...) | *
 *********************************************************************/
IDE_RC sdnIndexCTL::makeChainedCTS( idvSQL           * aStatistics,
                                    sdrMtx           * aMtx,
                                    sdpPhyPageHdr    * aPage,
                                    UChar              aCTSlotNum,
                                    sdnCallbackFuncs * aCallbackFunc,
                                    UChar            * aContext,
                                    sdSID            * aRollbackSID )
{
    ULong            sTempBuf[SD_PAGE_SIZE / ID_SIZEOF(ULong)];
    UChar          * sKeyList;
    sdcUndoSegment * sUndoSegment;
    sdrMtxStartInfo  sStartInfo;
    UShort         * sKeyListSize;
    sdnCTS         * sCTS;
    UShort           sChainedKeyCount = 0;
    idBool           sMadeChainedKeys = ID_FALSE;
    UShort           sUnchainedKeyCount = 0;
    UChar          * sUnchainedKey = NULL;
    UInt             sUnchainedKeySize = 0;
    UChar          * sSlotDirPtr;
    UShort           sSlotCount;

    sStartInfo.mTrans   = aMtx->mTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    sCTS = getCTS( aPage, aCTSlotNum );

    idlOS::memcpy( sTempBuf, sCTS, ID_SIZEOF( sdnCTS ) );
    sKeyListSize = (UShort*)(((UChar*)sTempBuf) + ID_SIZEOF(sdnCTS));
    sKeyList = ((UChar*)sTempBuf) + ID_SIZEOF(sdnCTS) + ID_SIZEOF(ULong);

    IDE_TEST( aCallbackFunc->mMakeChainedKeys( aPage,
                                               aCTSlotNum,
                                               aContext,
                                               sKeyList,
                                               sKeyListSize,
                                               &sChainedKeyCount )
              != IDE_SUCCESS );
    sMadeChainedKeys = ID_TRUE;

    sUndoSegment = smLayerCallback::getUDSegPtr( (smxTrans*)aMtx->mTrans );

    /*
     * aTableOID를 NULL_OID로 설정하여 Cursor Close시 Index 처리를
     * Skip 하게 한다.
     */
    if ( sUndoSegment->addIndexCTSlotUndoRec(
                          aStatistics,
                          &sStartInfo,
                          SM_NULL_OID, /* aTableOID */
                          (UChar*)sTempBuf,
                          ID_SIZEOF(sdnCTS) + ID_SIZEOF(ULong) + *sKeyListSize,
                          aRollbackSID ) != IDE_SUCCESS )
    {
        //BUG-47715: setIgnoreMtxUndo 삭제 
        IDE_TEST( 1 );
    }

    IDE_TEST( aCallbackFunc->mWriteChainedKeysLog( aMtx,
                                                   aPage,
                                                   aCTSlotNum )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /*
     * Undo Page Full로 인해서 더이상 공간이 없는 경우
     */
    if( sMadeChainedKeys == ID_TRUE )
    {
        cleanRefInfo( aPage, aCTSlotNum );

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aPage );
        sSlotCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

        /* SlotNum(UShort) + CreateCTS(UChar) + LimitCTS(UChar) */
        if ( iduMemMgr::malloc( IDU_MEM_SM_SDN,
                                ID_SIZEOF(UChar) * 4 * sSlotCount,
                                (void**)&sUnchainedKey,
                                IDU_MEM_IMMEDIATE ) == IDE_SUCCESS )
        {

            (void)aCallbackFunc->mMakeUnchainedKeys( NULL,
                                                     aPage,
                                                     sCTS,
                                                     aCTSlotNum,
                                                     sKeyList,
                                                     *sKeyListSize,
                                                     aContext,
                                                     sUnchainedKey,
                                                     &sUnchainedKeySize,
                                                     &sUnchainedKeyCount );

            (void)iduMemMgr::free( sUnchainedKey );
        }
    }

    return IDE_FAILURE;
}
#endif
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::canAllocCTS                   *
 * ------------------------------------------------------------------*
 * SMO시에 호출되며, CTS공간을 할당받을수 있을지 검사하는 함수이다.  *
 *********************************************************************/
idBool sdnIndexCTL::canAllocCTS( sdpPhyPageHdr * aPage,
                                 UChar           aNeedCount )
{
    sdnCTL * sCTL;

    sCTL = getCTL( aPage );

    if( getCount( sCTL ) >= (sCTL->mUsedCount + aNeedCount) )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::unchaining                    *
 * ------------------------------------------------------------------*
 * Undo page에 저장되어 있었던 Chained Key를 Unchained Key로 만든다. *
 * 해당 페이지에 chaining당시에는 있었지만 unchaining이 되기 전에    *
 * SMO에 의해서 다른 페이지로 이동된 경우도 있다. 따라서, Unchained  *
 * key count가 0인 경우 생길수 있으며, 만약 chain을 가지고 있지 않은 *
 * CTS라면 DEAD상태로 만든다.                                        *
 *********************************************************************/
IDE_RC sdnIndexCTL::unchainingCTS( idvSQL           * aStatistics,
                                   sdrMtx           * aMtx,
                                   sdpPhyPageHdr    * aPage,
                                   sdnCTS           * aCTS,
                                   UChar              aCTSlotNum,
                                   sdnCallbackFuncs * aCallbackFunc,
                                   UChar            * aContext )
{
    UShort   sUnchainedKeyCount = 0;
    UShort   sChainedKeySize;
    ULong    sKeyList[SD_PAGE_SIZE / ID_SIZEOF(ULong)];
    idBool   sIsReusedUndoRec;

    IDE_TEST( getChainedCTS( aStatistics,
                             aMtx,
                             aPage,
                             aCallbackFunc,
                             aContext,
                             &(aCTS->mCommitSCN),
                             aCTSlotNum,
                             ID_TRUE, // Try Hard Stamping
                             aCTS,
                             (UChar*)sKeyList,
                             &sChainedKeySize,
                             &sIsReusedUndoRec ) != IDE_SUCCESS );

    if( sIsReusedUndoRec == ID_FALSE )
    {
        /*
         * Chaining시점 이후에 Compact되었을수도 있기 때문에
         * Reference Info를 reset하고 MakeUnchainedKey에서 다시 구성한다.
         */
        cleanRefInfo( aPage, aCTSlotNum );

        IDE_TEST( aCallbackFunc->mLogAndMakeUnchainedKeys( NULL,
                                                           aMtx,
                                                           aPage,
                                                           aCTS,
                                                           aCTSlotNum,
                                                           (UChar*)sKeyList,
                                                           sChainedKeySize,
                                                           &sUnchainedKeyCount,
                                                           aContext )
                  != IDE_SUCCESS );

        /*
         * Unchaining될 키가 존재하지 않는다면 SMO로 인하여
         * 모든 키가 이동한 경우이다. 바로 삭제한다.
         */
        if( (sUnchainedKeyCount == 0) &&
            (hasChainedCTS( aCTS ) == ID_FALSE) )
        {
            // BUG-28010 mUsedCount Mismatch
            // Do Logging just below this block
            /* BUG-32118 [sm-disk-index] Do not logging FreeCTS if do
             * makeUnchainedKey operation. */
            IDE_TEST( freeCTS( aMtx,
                               aPage,
                               aCTSlotNum,
                               ID_TRUE ) // logging
                      != IDE_SUCCESS );
        }

        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)aCTS,
                                             (void*)aCTS,
                                             ID_SIZEOF(sdnCTS),
                                             SDR_SDP_BINARY)
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getChainedCTS                 *
 * ------------------------------------------------------------------*
 * BUG-29839,34499 BTree에서 재사용된 undo page에 접근할 수 있습니다.*
 * chain된 CTS의 commit SCN을 구하는 과정에서 다른 tx에 의해         *
 * chain된 CTS의 undo page가 재사용되었을 수 있습니다.               *
 * 이로 인한 오류를 방지하기 위해 기존에 읽은 CTS의 mNxtCommitSCN    *
 * 재사용 조건인 sysMinDskSCN보다 큰 경우에는 무조건 재활용으로      *
 * 판단하여 페이지에 대한 접근을 하지 않도록 합니다.                 *
 * 이는 페이지에 대한 S Latch를 잡은 후에 비교하여야 합니다.         *
 *
 *  aStatistics      - [IN] 통계정보
 *  aMtx             - [IN] Mini transaction,
 *  aPage            - [IN] CTS가 포함된 page의 pointer
 *  aCallbackFunc    - [IN] callback func
 *  aContext         - [IN] context
 *  aCommitSCN       - [IN] undo page 재활용을 확인 할 commit scn
 *  aCTSlotNum       - [IN] CTS의 slot number
 *  aCTS             - [IN/OUT] CTS를 받아서 Chained CTS로 반환
 *  aKeyList         - [OUT] Key List
 *  aKeyListSize     - [OUT] Key List의 크기
 *  aIsReusedUndoRec - [OUT] Undo Page의 재사용 여부
 *********************************************************************/
IDE_RC sdnIndexCTL::getChainedCTS( idvSQL           * aStatistics,
                                   sdrMtx           * aMtx,
                                   sdpPhyPageHdr    * aPage,
                                   sdnCallbackFuncs * aCallbackFunc,
                                   UChar            * aContext,
                                   smSCN            * aCommitSCN,
                                   UChar              aCTSlotNum,
                                   idBool             aTryHardKeyStamping,
                                   sdnCTS           * aCTS,
                                   UChar            * aKeyList,
                                   UShort           * aKeyListSize,
                                   idBool           * aIsReusedUndoRec )
{
    UChar         * sPagePtr;
    UChar         * sUNDOREC_HDR;
    UInt            sLatchState  = 0;
    UShort          sKeyListSize = 0;
    idBool          sIsReusedUndoRec = ID_TRUE;
    idBool          sIsSuccess;
    sdnCTS        * sCTS;
    sdnCTS          sChainedCTS;
    sdpPageType     sPageType;
    sdpSlotDirHdr * sSlotDirHdr = NULL;
    sdcUndoRecType  sType;
    smSCN           sSysMinDskSCN = SM_SCN_INIT;
    smSCN           sChainedCTSCommitSCN = SM_SCN_INIT;

    /* 버그 수정시 CTS의 mCommitSCN이 무한대인 경우가 없다고
     * 판단하였다. 만약 가정이 틀렸으면 다시 수정해야 한다.*/
    IDE_TEST_RAISE( SM_SCN_IS_INFINITE( *aCommitSCN ),
                    ERR_INFINITE_CTS_COMMIT_SCN );

    smLayerCallback::getSysMinDskViewSCN( &sSysMinDskSCN );

    /* BUG-38962
     * sSysMinDskSCN이 초기값이면, restart recovery 과정 중을 의미한다. */
    if( SM_SCN_IS_LT( aCommitSCN, &sSysMinDskSCN ) ||
        SM_SCN_IS_INIT( sSysMinDskSCN ) )
    {
        /* getCommitSCN등에서 사용하는 경우
         * hardkeystamping을 하지 않고 바로 빠져나간다.*/
        IDE_TEST_CONT( aTryHardKeyStamping == ID_FALSE, CONT_REUSE_UNDO_PAGE );

        IDE_ASSERT( aMtx != NULL );

        /* 재활용으로 판단 되면 바로 hard key stamping 합니다.*/
        IDE_TEST( aCallbackFunc->mHardKeyStamping( aStatistics,
                                                   aMtx,
                                                   aPage,
                                                   aCTSlotNum,
                                                   aContext,
                                                   &sIsSuccess )
                  != IDE_SUCCESS );

        /* hard key stamping이 성공할 경우 재활용 된 경우이다.*/

        /* BUG-44919 페이지 래치의 escalate가 실패할 경우에는 hardkeystamping이 실패합니다.
         *           이 경우에는 undo page가 재활용 되었을 가능성이 있으므로
         *           hardkeystamping의 성공 여부와 관계없이 빠져나오도록 합니다. */
    }
    else
    {
        IDE_TEST( sdbBufferMgr::getPage( aStatistics,
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         aCTS->mUndoPID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         &sPagePtr,
                                         &sIsSuccess ) != IDE_SUCCESS );
        sLatchState = 1;

        sIsReusedUndoRec = ID_FALSE;

        /* 이하는 undo page가 재활용 되지 않은 경우이니
         * 반드시 읽을 수 있어야 한다.*/
        sPageType = sdpPhyPage::getPageType( (sdpPhyPageHdr*)sPagePtr );

        IDE_TEST_RAISE( sPageType != SDP_PAGE_UNDO, ERR_INVALID_PAGE_TYPE );

        sSlotDirHdr = (sdpSlotDirHdr*)sdpPhyPage::getSlotDirStartPtr( sPagePtr );

        IDE_TEST_RAISE( aCTS->mUndoSlotNum >= sSlotDirHdr->mSlotEntryCount,
                        ERR_INVALID_UNDO_SLOT_NUM );

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( (UChar*)sSlotDirHdr,
                                                           aCTS->mUndoSlotNum,  
                                                           &sUNDOREC_HDR )
                  != IDE_SUCCESS );

        SDC_GET_UNDOREC_HDR_1B_FIELD( sUNDOREC_HDR,
                                      SDC_UNDOREC_HDR_TYPE,
                                      &sType );

        IDE_TEST_RAISE( sType != SDC_UNDO_INDEX_CTS, ERR_INVALID_UNDO_REC_TYPE );

        sCTS = (sdnCTS*)(sUNDOREC_HDR + SDC_UNDOREC_HDR_SIZE);

        /* BUG-34542 Undo Record의 CTS는 Align이 맞지 않습니다.
         * Commit SCN을 비교하기 위해 memcpy해와야 합니다.*/
        idlOS::memcpy( (UChar*)&sChainedCTSCommitSCN,
                       (UChar*)sCTS,
                       ID_SIZEOF( smSCN ) );

        IDU_FIT_POINT_RAISE( "BUG-45303@sdnIndexCTL::getCommitSCN", ERR_INVALID_CTS_COMMIT_SCN );

        /* NxtCommitSCN은 chaining된 CTS의 Commit SCN을 나타낸다. 같아야한다. */
        IDE_TEST_RAISE( ! SM_SCN_IS_EQ( &(aCTS->mNxtCommitSCN), &(sChainedCTSCommitSCN) ),
                        ERR_INVALID_CTS_COMMIT_SCN );

        idlOS::memcpy( aCTS,
                       ((UChar*)sCTS),
                       ID_SIZEOF( sdnCTS ) );

        idlOS::memcpy( (UChar*)&sKeyListSize,
                       ((UChar*)sCTS) + ID_SIZEOF( sdnCTS ),
                       ID_SIZEOF(UShort) );

        IDE_TEST_RAISE( sKeyListSize >= SD_PAGE_SIZE, ERR_INVALID_KEY_LIST_SIZE );

        idlOS::memcpy( aKeyList,
                       ((UChar*)sCTS) + ID_SIZEOF(sdnCTS) + ID_SIZEOF(ULong),
                       sKeyListSize );
    }

    IDE_EXCEPTION_CONT( CONT_REUSE_UNDO_PAGE );

    if( sLatchState != 0 )
    {
        sLatchState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
                  != IDE_SUCCESS );
    }

    *aIsReusedUndoRec = sIsReusedUndoRec;
    *aKeyListSize     = sKeyListSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INFINITE_CTS_COMMIT_SCN );
    {
        ideLog::log( IDE_SERVER_0,
                     "CTS Commit SCN             : %llu\n"
                     "CTS Next Commit SCN        : %llu\n"
                     "Min First Disk View SCN    : %llu\n",
#ifdef COMPILE_64BIT
                     aCTS->mCommitSCN,
                     aCTS->mNxtCommitSCN,
                     sSysMinDskSCN
#else
                     ((ULong)((((ULong)aCTS->mCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mCommitSCN.mLow)),
                     ((ULong)((((ULong)aCTS->mNxtCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mNxtCommitSCN.mLow)),
                     ((ULong)((((ULong)sSysMinDskSCN.mHigh) << 32) |
                             (ULong)sSysMinDskSCN.mLow))
#endif
            );
    }
    IDE_EXCEPTION( ERR_INVALID_PAGE_TYPE );
    {
        ideLog::log( IDE_SERVER_0,
                     "Page Type        : %u\n"
                     "CTS Commit SCN   : %llu\n"
                     "CTS NxtCommitSCN : %llu\n"
                     "Sys Min Disk SCN : %llu\n",
                     sPageType,
#ifdef COMPILE_64BIT
                     aCTS->mCommitSCN,
                     aCTS->mNxtCommitSCN,
                     sSysMinDskSCN
#else
                     ((ULong)((((ULong)aCTS->mCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mCommitSCN.mLow)),
                     ((ULong)((((ULong)aCTS->mNxtCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mNxtCommitSCN.mLow)),
                     ((ULong)((((ULong)sSysMinDskSCN.mHigh) << 32) |
                             (ULong)sSysMinDskSCN.mLow))
#endif
            );
    }
    IDE_EXCEPTION( ERR_INVALID_UNDO_SLOT_NUM );
    {
        ideLog::log( IDE_SERVER_0,
                     "CTS Undo Slot Num (%u) >="
                     "Undo Slot Entry Count (%u)\n"
                     "CTS Commit SCN   :%llu\n"
                     "CTS NxtCommitSCN :%llu\n"
                     "Sys Min Disk SCN :%llu\n",
                     aCTS->mUndoSlotNum,
                     sSlotDirHdr->mSlotEntryCount,
#ifdef COMPILE_64BIT
                     aCTS->mCommitSCN,
                     aCTS->mNxtCommitSCN,
                     sSysMinDskSCN
#else
                     ((ULong)((((ULong)aCTS->mCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mCommitSCN.mLow)),
                     ((ULong)((((ULong)aCTS->mNxtCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mNxtCommitSCN.mLow)),
                     ((ULong)((((ULong)sSysMinDskSCN.mHigh) << 32) |
                             (ULong)sSysMinDskSCN.mLow))
#endif
            );
    }
    IDE_EXCEPTION( ERR_INVALID_CTS_COMMIT_SCN );
    {
        idlOS::memcpy( (UChar*)&sChainedCTS,
                       (UChar*)sCTS,
                       ID_SIZEOF( sdnCTS ) );

       ideLog::log( IDE_SERVER_0,
                    "CTS Commit SCN             : %llu\n"
                    "CTS Next Commit SCN        : %llu\n"
                    "Chained CTS Commit SCN     : %llu\n"
                    "Chained CTS Next Commit SCN: %llu\n"
                    "Min First Disk View SCN    : %llu\n",
#ifdef COMPILE_64BIT
                    aCTS->mCommitSCN,
                    aCTS->mNxtCommitSCN,
                    sChainedCTS.mCommitSCN,
                    sChainedCTS.mNxtCommitSCN,
                    sSysMinDskSCN
#else
                    ((ULong)((((ULong)aCTS->mCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mCommitSCN.mLow)),
                    ((ULong)((((ULong)aCTS->mNxtCommitSCN.mHigh) << 32) |
                             (ULong)aCTS->mNxtCommitSCN.mLow)),
                    ((ULong)((((ULong)sChainedCTS.mCommitSCN.mHigh) << 32) |
                             (ULong)sChainedCTS.mCommitSCN.mLow)),
                    ((ULong)((((ULong)sChainedCTS.mNxtCommitSCN.mHigh) << 32) |
                             (ULong)sChainedCTS.mNxtCommitSCN.mLow)),
                    ((ULong)((((ULong)sSysMinDskSCN.mHigh) << 32) |
                             (ULong)sSysMinDskSCN.mLow))
#endif
            );
    }
    IDE_EXCEPTION( ERR_INVALID_UNDO_REC_TYPE );
    {
        ideLog::log( IDE_SERVER_0,
                     "Undo PageID               : %u\n"
                     "Undo Slot Number          : %u\n"
                     "UndoRec Hdr Type          : %u\n"
                     "Chained CTS\'s Commit SCN : %llu\n"
                     "Min DskView SCN           : %llu\n",
                     aCTS->mUndoPID,
                     aCTS->mUndoSlotNum,
                     sType,
#ifdef COMPILE_64BIT
                     aCTS->mNxtCommitSCN,
                     sSysMinDskSCN
#else
                     ((ULong)((((ULong)aCTS->mNxtCommitSCN.mHigh) << 32) |
                              (ULong)aCTS->mNxtCommitSCN.mLow)),
                     ((ULong)((((ULong)sSysMinDskSCN.mHigh) << 32) |
                              (ULong)sSysMinDskSCN.mLow))
#endif
            );
    }
    IDE_EXCEPTION( ERR_INVALID_KEY_LIST_SIZE );
    {
        ideLog::log( IDE_SERVER_0,
                     "Undo PageID      : %u\n"
                     "Undo Slot Number : %u\n"
                     "KeyList Size     : %u\n",
                     aCTS->mUndoPID,
                     aCTS->mUndoSlotNum,
                     sKeyListSize );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sLatchState != 0 )
        {
            // Dump Page and page Header
            (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                         (UChar*)aPage,
                                         "Index Page" );

            // Dump Page and page Header
            (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                         sPagePtr,
                                         "Undo Page" );

            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
                        == IDE_SUCCESS );
        }
    }
    IDE_POP();

    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}
#endif


/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::isMyTransaction               *
 * ------------------------------------------------------------------*
 * 자신의 트랜잭션인지를 검사한다.                                   *
 *********************************************************************/
idBool sdnIndexCTL::isMyTransaction( void          * aTrans,
                                     sdpPhyPageHdr * aPage,
                                     UChar           aSlotNum )
{
    smSCN    sSCN;

    sSCN = smLayerCallback::getFstDskViewSCN( aTrans );

    return isMyTransaction( aTrans, aPage, aSlotNum, &sSCN );
}

idBool sdnIndexCTL::isMyTransaction( void          * aTrans,
                                     sdpPhyPageHdr * aPage,
                                     UChar           aSlotNum,
                                     smSCN         * aSCN )
{
    sdSID    sTSSlotSID;
    sdnCTS * sCTS;

    sCTS = getCTS( aPage, aSlotNum );

    sTSSlotSID = smLayerCallback::getTSSlotSID( aTrans );

    if( (SD_MAKE_PID( sTSSlotSID ) == sCTS->mTSSlotPID) &&
        (SD_MAKE_SLOTNUM( sTSSlotSID ) == sCTS->mTSSlotNum) )
    {
        if( SM_SCN_IS_EQ( &sCTS->mCommitSCN, aSCN ) )
        {
            return ID_TRUE;
        }
    }
    return ID_FALSE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::isSameTransaction             *
 * ------------------------------------------------------------------*
 * 주어진 2개의 CTS가 같은 트랜잭션인지를 검사한다.                  *
 *********************************************************************/
idBool sdnIndexCTL::isSameTransaction( sdnCTS * aCTS1,
                                       sdnCTS * aCTS2 )
{
    if( (aCTS1->mTSSlotPID == aCTS2->mTSSlotPID) &&
        (aCTS1->mTSSlotNum == aCTS2->mTSSlotNum) )
    {
        if( ( (aCTS1->mState == SDN_CTS_UNCOMMITTED) &&
              (aCTS2->mState == SDN_CTS_UNCOMMITTED) ) ||
            ( (aCTS1->mState == SDN_CTS_STAMPED) &&
              (aCTS2->mState == SDN_CTS_STAMPED) ) )
        {
            if( SM_SCN_IS_EQ( &aCTS1->mCommitSCN, &aCTS2->mCommitSCN ) )
            {
                return ID_TRUE;
            }
        }
    }

    return ID_FALSE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getTSSlotSID                  *
 * ------------------------------------------------------------------*
 * CTSlotSID을 얻는다.                                               *
 *********************************************************************/
sdSID sdnIndexCTL::getTSSlotSID( sdpPhyPageHdr * aPage,
                                 UChar           aSlotNum )
{
    sdnCTS * sCTS;

    sCTS = getCTS( aPage, aSlotNum );

    return SD_MAKE_SID( sCTS->mTSSlotPID,
                        sCTS->mTSSlotNum );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getRefKey                     *
 * ------------------------------------------------------------------*
 * CTS reference infomation을 리턴한다.                              *
 *********************************************************************/
void sdnIndexCTL::getRefKey( sdpPhyPageHdr * aPage,
                             UChar           aSlotNum,
                             UShort        * aRefKeyCount,
                             UShort       ** aArrRefKey )
{
    sdnCTS * sCTS;

    sCTS = getCTS( aPage, aSlotNum );

    *aRefKeyCount = sCTS->mRefCnt;
    *aArrRefKey = sCTS->mRefKey;
}
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getRefKeyCount                *
 * ------------------------------------------------------------------*
 * CTS reference count를 리턴한다.                                   *
 *********************************************************************/
UShort sdnIndexCTL::getRefKeyCount( sdpPhyPageHdr * aPage,
                                    UChar           aSlotNum )
{
    sdnCTS * sCTS;

    sCTS = getCTS( aPage, aSlotNum );

    return (sCTS->mRefCnt);
}
#endif
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::addRefKey                     *
 * ------------------------------------------------------------------*
 * CTS reference key를 리턴한다.                                     *
 *********************************************************************/
void sdnIndexCTL::addRefKey( sdpPhyPageHdr * aPage,
                             UChar           aSlotNum,
                             UShort          aKeyOffset )
{
    sdnCTS * sCTS;
    UInt     i;

    sCTS = getCTS( aPage, aSlotNum );

    for( i = 0; i < SDN_CTS_MAX_KEY_CACHE; i++ )
    {
        if( sCTS->mRefKey[i] == SDN_CTS_KEY_CACHE_NULL )
        {
            sCTS->mRefKey[i] = aKeyOffset;
            break;
        }
    }

    sCTS->mRefCnt++;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::updateRefKey                  *
 * ------------------------------------------------------------------*
 * BUG-29506 TBT가 TBK로 전환시 offset을 CTS에 반영하지 않습니다.    *
 * CTS reference key를 변경한다.                                     *
 *********************************************************************/
IDE_RC sdnIndexCTL::updateRefKey( sdrMtx        * aMtx,
                                  sdpPhyPageHdr * aPage,
                                  UChar           aSlotNum,
                                  UShort          aOldKeyOffset,
                                  UShort          aNewKeyOffset )
{
    sdnCTS * sCTS;
    UInt     i;

    sCTS = getCTS( aPage, aSlotNum );

    for( i = 0; i < SDN_CTS_MAX_KEY_CACHE; i++ )
    {
        if( sCTS->mRefKey[i] == aOldKeyOffset )
        {
            IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                                 (UChar*)&(sCTS->mRefKey[i]),
                                                 (UChar*)&aNewKeyOffset,
                                                 ID_SIZEOF(UShort) )
                      != IDE_SUCCESS );
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::cleanRefInfo                  *
 * ------------------------------------------------------------------*
 * CTL의 reference 정보를 reset한다.                                 *
 *********************************************************************/
void sdnIndexCTL::cleanAllRefInfo( sdpPhyPageHdr * aPage )
{
    sdnCTL  * sCTL;
    sdnCTS  * sCTS;
    UInt      i, j;

    sCTL = getCTL( aPage );

    for( i = 0; i < getCount(sCTL); i++ )
    {
        sCTS = getCTS( sCTL, i );
        sCTS->mRefCnt = 0;

        for( j = 0; j < SDN_CTS_MAX_KEY_CACHE; j++ )
        {
            sCTS->mRefKey[j] = SDN_CTS_KEY_CACHE_NULL;
        }
    }
}
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::cleanRefInfo                  *
 * ------------------------------------------------------------------*
 * CTS의 reference 정보를 reset한다.                                 *
 *********************************************************************/
void sdnIndexCTL::cleanRefInfo( sdpPhyPageHdr * aPage,
                                UChar           aCTSlotNum )
{
    sdnCTL  * sCTL;
    sdnCTS  * sCTS;
    UInt      i;

    sCTL = getCTL( aPage );

    sCTS = getCTS( sCTL, aCTSlotNum );
    sCTS->mRefCnt = 0;

    for( i = 0; i < SDN_CTS_MAX_KEY_CACHE; i++ )
    {
        sCTS->mRefKey[i] = SDN_CTS_KEY_CACHE_NULL;
    }
}
#endif
#if 0 // not used
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getVictimTrans                *
 * ------------------------------------------------------------------*
 * Uncommitted CTS중 victim을 선정한다.                              *
 *********************************************************************/
IDE_RC sdnIndexCTL::getVictimTrans( idvSQL        * aStatistics,
                                    sdpPhyPageHdr * aPage,
                                    smTID         * aTransID )
{
    UInt      i;
    sdnCTL  * sCTL;
    sdnCTS  * sCTS;
    sdcTSS  * sTSSlot;
    UChar   * sPagePtr;
    smSCN     sTransCSCN;
    idBool    sIsFixed = ID_FALSE;

    sCTL = getCTL( aPage );

    *aTransID = SM_NULL_TID;

    for( i = 0; i < getCount(sCTL); i++ )
    {
        sCTS = getCTS( sCTL, i );

        if( sCTS->mState == SDN_CTS_UNCOMMITTED )
        {
            IDE_TEST( sdbBufferMgr::fixPageByPID(
                                            aStatistics,
                                            SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                            sCTS->mTSSlotPID,
                                            SDB_WAIT_NORMAL,
                                            &sPagePtr ) 
                      != IDE_SUCCESS );
            sIsFixed = ID_TRUE;

            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                        sdpPhyPage::getSlotDirStartPtr(sPagePtr),
                                        sCTS->mTSSlotNum,
                                        (UChar**)&sTSSlot )
                      != IDE_SUCCESS );

            sdcTSSlot::getTransInfo( sTSSlot, aTransID, &sTransCSCN );

            IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                      != IDE_SUCCESS );

            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if ( sIsFixed == ID_TRUE ) 
    {
        sIsFixed = ID_FALSE;
        (void)sdbBufferMgr::unfixPage( aStatistics, sPagePtr );
    }
    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::isChainedCTS                  *
 * ------------------------------------------------------------------*
 * chain을 가지고 있는 CTS인지를 검사한다.                           *
 *********************************************************************/
idBool sdnIndexCTL::hasChainedCTS( sdpPhyPageHdr * aPage,
                                  UChar           aCTSlotNum )
{
    sdnCTL  * sCTL;
    sdnCTS  * sCTS;

    sCTL = getCTL( aPage );
    sCTS = getCTS( sCTL, aCTSlotNum );

    return hasChainedCTS( sCTS );
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::isChainedCTS                  *
 * ------------------------------------------------------------------*
 * chain을 가지고 있는 CTS인지를 검사한다.                           *
 *********************************************************************/
idBool sdnIndexCTL::hasChainedCTS( sdnCTS * aCTS )
{
    if( (aCTS->mUndoPID == 0) && (aCTS->mUndoSlotNum == 0) )
    {
        return ID_FALSE;
    }
    return ID_TRUE;
}
#endif
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::getCTLayerSize                *
 * ------------------------------------------------------------------*
 * CTL의 총 크기를 리턴한다.                                         *
 *********************************************************************/
UShort sdnIndexCTL::getCTLayerSize( UChar * aPage )
{
    sdnCTL  * sCTL;

    sCTL = getCTL( (sdpPhyPageHdr*)aPage );

    return (getCount(sCTL) * ID_SIZEOF(sdnCTS)) +
           ID_SIZEOF(sdnCTLayerHdr);
}

#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::wait4Trans                    *
 * ------------------------------------------------------------------*
 * CTS를 할당한 트랜잭션이 완료되기를 특정시간만큼만 대기한다.
 *
 * BUG-24140 Index/Data CTL 할당과정에서 TRANS_WAIT_TIME_FOR_CTS
 *           를 초과하는 경우 Error가 발생해서는 안됨.
 *
 * 대기시간 초과로 인해서 Error 가 발생한 경우 IDE_SUCCESS를 반환한다.
 * 즉, 하나의 트랜잭션을 선택하지만, 그 트랜잭션이 완료될때까지 대기하지 않고,
 * 프로퍼티로 주어진 시간내에 완료되지 않았다면 AllocCTS 과정으로 처음부터 다시
 * 수행한다.
 *
 * aStatistics  - [IN] 통계정보
 * aTrans       - [IN] 트랜잭션 포인터
 * aWait4TID    - [IN] 대기해야할 트랜잭션 ID
 *
 ***********************************************************************/
IDE_RC sdnIndexCTL::wait4Trans( void   * aTrans,
                                smTID    aWait4TID )
{
    IDE_ASSERT( aTrans    != NULL );
    IDE_ASSERT( aWait4TID != SM_NULL_TID );

    if ( smLayerCallback::waitForTrans( aTrans,
                                        aWait4TID,
                                        smuProperty::getTransWaitTime4TTS() )
         != IDE_SUCCESS )
    {
        IDE_TEST( ideGetErrorCode() != smERR_ABORT_smcExceedLockTimeWait );
        IDE_CLEAR();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
/*********************************************************************
 * FUNCTION DESCRIPTION : sdnIndexCTL::dump                          *
 * ------------------------------------------------------------------*
 * TASK-4007 [SM] PBT를 위한 기능 추가 - dumpddf정상화               *
 * IndexPage에서 CTS를 Dump한다.                                     *
 *                                                                   *
 * BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서      *
 * local Array의 ptr를 반환하고 있습니다.                            *
 * Local Array대신 OutBuf를 받아 리턴하도록 수정합니다.              *
 *********************************************************************/
IDE_RC sdnIndexCTL::dump( UChar *aPage  ,
                          SChar *aOutBuf ,
                          UInt   aOutSize )

{
    UInt            i;
    sdnCTL        * sCTL;
    sdnCTS        * sCTS;

    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sCTL = getCTL( (sdpPhyPageHdr*) aPage );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "----------- Index CTL Begin ----------\n"
                     "mCount     : %"ID_UINT32_FMT"\n"
                     "mUsedCount : %"ID_UINT32_FMT"\n\n",
                     sCTL->mCount,
                     sCTL->mUsedCount );

    for( i = 0; i < getCount(sCTL); i++ )
    {
        sCTS = getCTS( sCTL, i );

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "[%"ID_UINT32_FMT"] CTS ----------------\n"
                             "mCommitSCN    : 0x%"ID_xINT64_FMT"\n"
                             "mTSSlotPID    : %"ID_UINT32_FMT"\n"
                             "mTSSlotNum    : %"ID_UINT32_FMT"\n"
                             "mState        : %"ID_UINT32_FMT"\n"
                             "mRefCnt       : %"ID_UINT32_FMT"\n"
                             "mRefKey[ 0 ]  : %"ID_UINT32_FMT"\n"
                             "mRefKey[ 1 ]  : %"ID_UINT32_FMT"\n"
                             "mRefKey[ 2 ]  : %"ID_UINT32_FMT"\n",
                             i,
                             SM_SCN_TO_LONG( sCTS->mCommitSCN ),
                             sCTS->mTSSlotPID,
                             sCTS->mTSSlotNum,
                             sCTS->mState,
                             sCTS->mRefCnt,
                             sCTS->mRefKey[ 0 ],
                             sCTS->mRefKey[ 1 ],
                             sCTS->mRefKey[ 2 ] );
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "----------- Index CTL End   ----------\n" );

    return IDE_SUCCESS;
}

