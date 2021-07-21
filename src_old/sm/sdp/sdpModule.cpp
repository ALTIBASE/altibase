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
 *
 * $Id: sdpModule.cpp 27220 2008-07-23 14:56:22Z newdaily $
 *
 * Page Layer의 테이블스페이스 및 세그먼트의 공간관리 모듈들을 정의한다.
 *
 **********************************************************************/

# include <sdptb.h>
# include <sdpsf.h>
# include <sdpst.h>
# include <sdpsc.h>
# include <sdpModule.h>

/*
 * [ FMS ]
 * Freelist Managed Segment 의 공간관리 모듈을 정의한다.
 */
sdpSegMgmtOp gSdpsfOp =
{
    (sdpsInitializeFunc)sdpsfSH::initialize,
    (sdpsDestroyFunc)sdpsfSH::destroy,

    (sdpsCreateSegmentFunc)sdpsfSegDDL::createSegment,
    (sdpsDropSegmentFunc)sdpsfSegDDL::dropSegment,
    (sdpsResetSegmentFunc)sdpsfSegDDL::resetSegment,
    (sdpsExtendSegmentFunc)sdpsfExtMgr::allocMutliExt,

    (sdpsAllocNewPageFunc)sdpsfAllocPage::allocPage,
    (sdpsPrepareNewPagesFunc)sdpsfAllocPage::prepareNewPages,
    (sdpsAllocNewPage4AppendFunc)sdpsfAllocPage::allocNewPage4Append,
    (sdpsPrepareNewPage4AppendFunc)NULL,
    (sdpsUpdatePageState)sdpsfFindPage::updatePageState,

    (sdpsFreePageFunc)sdpsfAllocPage::freePage,
    (sdpsIsFreePageFunc)sdpsfAllocPage::isFreePage,

    (sdpsUpdateHWMInfo4DPath)sdpsfSH::updateHWMInfo4DPath,
    (sdpsReformatPage4DPath)sdpsfSH::reformatPage4DPath,

    (sdpsFindInsertablePageFunc)sdpsfFindPage::findFreePage,

    (sdpsGetFmtPageCntFunc)sdpsfSH::getFmtPageCnt,
    (sdpsGetSegInfoFunc)sdpsfSH::getSegInfo,
    (sdpsGetExtInfoFunc)sdpsfExtMgr::getExtInfo,
    (sdpsGetNxtExtInfoFunc)sdpsfExtMgr::getNxtExtRID,
    (sdpsGetNxtAllocPageFunc)sdpsfExtMgr::getNxtAllocPage,
    (sdpsGetSegCacheInfoFunc)sdpsfSH::getSegCacheInfo,
    (sdpsSetLstAllocPageFunc)sdpsfSH::setLstAllocPage,

    (sdpsGetSegStateFunc)sdpsfSH::getSegState,
    (sdpsGetHintPosInfoFunc)NULL,

    (sdpsSetMetaPIDFunc)sdpsfSH::setMetaPID,
    (sdpsGetMetaPIDFunc)sdpsfSH::getMetaPID,

    (sdpsMarkSCN4ReCycleFunc)NULL,
    (sdpsSetSCNAtAllocFunc)NULL,
    (sdpsTryStealExtsFunc)NULL,
    (sdpsShrinkExtsFunc)NULL,

    (sdpsDumpFunc)NULL,
    (sdpsVerifyFunc)NULL
};

/*
 * [ TMS ]
 * Treelist Managed Segment 의 공간관리 모듈을 정의한다.
 */
sdpSegMgmtOp gSdpstOp =
{
    (sdpsInitializeFunc)sdpstCache::initialize,
    (sdpsDestroyFunc)sdpstCache::destroy,

    (sdpsCreateSegmentFunc)sdpstSegDDL::createSegment,
    (sdpsDropSegmentFunc)sdpstSegDDL::dropSegment,
    (sdpsResetSegmentFunc)sdpstSegDDL::resetSegment,
    (sdpsExtendSegmentFunc)sdpstSegDDL::allocateExtents,

    (sdpsAllocNewPageFunc)sdpstAllocPage::allocateNewPage,
    (sdpsPrepareNewPagesFunc)sdpstAllocPage::prepareNewPages,
    (sdpsAllocNewPage4AppendFunc)sdpstDPath::allocNewPage4Append,
    (sdpsPrepareNewPage4AppendFunc)NULL,
    (sdpsUpdatePageState)sdpstAllocPage::updatePageState,

    (sdpsFreePageFunc)sdpstFreePage::freePage,
    (sdpsIsFreePageFunc)sdpstFreePage::isFreePage,

    (sdpsUpdateHWMInfo4DPath)sdpstDPath::updateWMInfo4DPath,
    (sdpsReformatPage4DPath)sdpstDPath::reformatPage4DPath,

    (sdpsFindInsertablePageFunc)sdpstFindPage::findInsertablePage,

    (sdpsGetFmtPageCntFunc)sdpstCache::getFmtPageCnt,
    (sdpsGetSegInfoFunc)sdpstSH::getSegInfo,
    (sdpsGetExtInfoFunc)sdpstExtDir::getExtInfo,
    (sdpsGetNxtExtInfoFunc)sdpstExtDir::getNxtExtRID,
    (sdpsGetNxtAllocPageFunc)sdpstExtDir::getNxtAllocPage,
    (sdpsGetSegCacheInfoFunc)sdpstCache::getSegCacheInfo,
    (sdpsSetLstAllocPageFunc)sdpstCache::setLstAllocPage,

    (sdpsGetSegStateFunc)sdpstSH::getSegState,
    (sdpsGetHintPosInfoFunc)sdpstCache::getHintPosInfo,

    (sdpsSetMetaPIDFunc)sdpstSH::setMetaPID,
    (sdpsGetMetaPIDFunc)sdpstSH::getMetaPID,

    (sdpsMarkSCN4ReCycleFunc)NULL,
    (sdpsSetSCNAtAllocFunc)NULL,
    (sdpsTryStealExtsFunc)NULL,
    (sdpsShrinkExtsFunc)NULL,

    (sdpsDumpFunc)sdpstVerifyAndDump::dump,
    (sdpsVerifyFunc)sdpstVerifyAndDump::verify
};

/*
 * [ CMS ]
 * Circular-List Managed Segment 의 공간관리 모듈을 정의한다.
 */
sdpSegMgmtOp gSdpscOp =
{
    (sdpsInitializeFunc)sdpscCache::initialize,
    (sdpsDestroyFunc)sdpscCache::destroy,

    (sdpsCreateSegmentFunc)sdpscSegDDL::createSegment,
    (sdpsDropSegmentFunc)NULL,
    (sdpsResetSegmentFunc)NULL,
    (sdpsExtendSegmentFunc)NULL,

    (sdpsAllocNewPageFunc)NULL,
    (sdpsPrepareNewPagesFunc)NULL,
    (sdpsAllocNewPage4AppendFunc)sdpscAllocPage::allocNewPage4Append,
    (sdpsPrepareNewPage4AppendFunc)sdpscAllocPage::prepareNewPage4Append,
    (sdpsUpdatePageState)NULL,

    (sdpsFreePageFunc)NULL,
    (sdpsIsFreePageFunc)NULL,

    (sdpsUpdateHWMInfo4DPath)NULL,
    (sdpsReformatPage4DPath)NULL,

    (sdpsFindInsertablePageFunc)NULL,

    (sdpsGetFmtPageCntFunc)sdpscSegHdr::getFmtPageCnt,
    (sdpsGetSegInfoFunc)sdpscSegHdr::getSegInfo,
    (sdpsGetExtInfoFunc)sdpscExtDir::getExtInfo,
    (sdpsGetNxtExtInfoFunc)sdpscExtDir::getNxtExtRID,
    (sdpsGetNxtAllocPageFunc)sdpscExtDir::getNxtAllocPage,
    (sdpsGetSegCacheInfoFunc)sdpscCache::getSegCacheInfo,
    (sdpsSetLstAllocPageFunc)sdpscCache::setLstAllocPage,

    (sdpsGetSegStateFunc)sdpscSegHdr::getSegState,
    (sdpsGetHintPosInfoFunc)NULL,

    (sdpsSetMetaPIDFunc)NULL,
    (sdpsGetMetaPIDFunc)NULL,

    (sdpsMarkSCN4ReCycleFunc)sdpscExtDir::markSCN4ReCycle,
    (sdpsSetSCNAtAllocFunc)sdpscExtDir::setSCNAtAlloc,
    (sdpsTryStealExtsFunc)sdpscSegDDL::tryStealExts,
    (sdpsShrinkExtsFunc)sdpscExtDir::shrinkExts,

    (sdpsDumpFunc)NULL, //sdpscVerifyAndDump::dump,
    (sdpsVerifyFunc)NULL //sdpscVerifyAndDump::verify
};
