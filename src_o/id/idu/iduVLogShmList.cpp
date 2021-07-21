/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <iduProperty.h>
#include <iduVLogShmList.h>

idrUndoFunc iduVLogShmList::mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_MAX];
SChar       iduVLogShmList::mStrVLogType[IDU_VLOG_TYPE_SHM_LIST_MAX+1][100] =
{
    "IDU_VLOG_TYPE_SHM_LIST_NONE",
    "IDU_VLOG_TYPE_SHM_LIST_ADD_BEFORE",
    "IDU_VLOG_TYPE_SHM_LIST_ADD_AFTER",
    "IDU_VLOG_TYPE_SHM_LIST_ADDLIST_FIRST",
    "IDU_VLOG_TYPE_SHM_LIST_CUT_BETWEEN",
    "IDU_VLOG_TYPE_SHM_LIST_REMOVE",
    "IDU_VLOG_TYPE_SHM_LIST_MAX"
};

IDE_RC iduVLogShmList::initialize()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_NONE] = NULL;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_ADD_BEFORE] =
        undo_SHM_LIST_ADD_BEFORE;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_ADD_AFTER] =
        undo_SHM_LIST_ADD_AFTER;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_CUT_BETWEEN] =
        undo_SHM_LIST_CUT_BETWEEN;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_ADDLIST_FIRST] =
        undo_SHM_LIST_ADDLIST_FIRST;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LIST_REMOVE] =
        undo_SHM_LIST_REMOVE;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::destroy()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
                                 NULL /* aArrUndoFunc */ );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::writeAddBefore( iduShmTxInfo   * aShmTxInfo,
                                       iduShmListNode * aTarget,
                                       iduShmListNode * aTgtNodePrev,
                                       iduShmListNode * aNode )
{
    idrUImgInfo  sArrUImgInfo[7];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aNode->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aNode->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aNode->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aTgtNodePrev->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[4], &aTgtNodePrev->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[5], &aTarget->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[6], &aTarget->mAddrPrev, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
                         IDU_VLOG_TYPE_SHM_LIST_ADD_BEFORE,
                         7,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::undo_SHM_LIST_ADD_BEFORE( idvSQL       * /*aStatistics*/,
                                                 iduShmTxInfo * /*aShmTxInfo*/,
                                                 idrLSN         /*aNTALsn*/,
                                                 UShort         /*aSize*/,
                                                 SChar        * aImage )
{

    idShmAddr       sAddObj4Node;
    idShmAddr       sAddObj4TgtNodePrev;
    idShmAddr       sAddObj4TgtNode;
    SChar         * sCurLogPtr;

    iduShmListNode * sNode;
    iduShmListNode * sTgtNodePrev;
    iduShmListNode * sTgtNode;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Node, ID_SIZEOF(idShmAddr) );
    sNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4Node );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNode->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNode->mAddrPrev, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4TgtNodePrev, ID_SIZEOF(idShmAddr) );
    sTgtNodePrev = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4TgtNodePrev );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sTgtNodePrev->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4TgtNode, ID_SIZEOF(idShmAddr) );
    sTgtNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4TgtNode );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sTgtNode->mAddrPrev, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::writeAddAfter( iduShmTxInfo    * aShmTxInfo,
                                      iduShmListNode  * aTarget,
                                      iduShmListNode  * aTgtNodeNext,
                                      iduShmListNode  * aNode )
{
    idrUImgInfo  sArrUImgInfo[7];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aNode->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aNode->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aNode->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aTgtNodeNext->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[4], &aTgtNodeNext->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[5], &aTarget->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[6], &aTarget->mAddrNext, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
                         IDU_VLOG_TYPE_SHM_LIST_ADD_AFTER,
                         7,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::undo_SHM_LIST_ADD_AFTER( idvSQL         * /*aStatistics*/,
                                                iduShmTxInfo   * /*aShmTxInfo*/,
                                                idrLSN           /*aNTALsn*/,
                                                UShort           /*aSize*/,
                                                SChar          * aImage )
{

    idShmAddr       sAddObj4Node;
    idShmAddr       sAddObj4TgtNodeNext;
    idShmAddr       sAddObj4TgtNode;
    SChar         * sCurLogPtr;

    iduShmListNode * sNode;
    iduShmListNode * sTgtNodeNext;
    iduShmListNode * sTgtNode;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Node, ID_SIZEOF(idShmAddr) );
    sNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4Node );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNode->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNode->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4TgtNodeNext, ID_SIZEOF(idShmAddr) );
    sTgtNodeNext = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4TgtNodeNext );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sTgtNodeNext->mAddrPrev, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4TgtNode, ID_SIZEOF(idShmAddr) );
    sTgtNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4TgtNode );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sTgtNode->mAddrNext, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::writeCutBetween( iduShmTxInfo    * aShmTxInfo,
                                        iduShmListNode  * aPrev,
                                        iduShmListNode  * aNext )
{
    idrUImgInfo  sArrUImgInfo[4];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aPrev->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aPrev->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aNext->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aNext->mAddrPrev, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
                         IDU_VLOG_TYPE_SHM_LIST_CUT_BETWEEN,
                         4,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::undo_SHM_LIST_CUT_BETWEEN( idvSQL         * /*aStatistics*/,
                                                  iduShmTxInfo   * /*aShmTxInfo*/,
                                                  idrLSN           /*aNTALsn*/,
                                                  UShort   /*aSize*/,
                                                  SChar  * aImage )
{

    idShmAddr       sAddObj4Prev;
    idShmAddr       sAddObj4Next;
    SChar         * sCurLogPtr;

    iduShmListNode * sPrev;
    iduShmListNode * sNext;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Prev, ID_SIZEOF(idShmAddr) );
    sPrev = ( iduShmListNode* )IDU_SHM_GET_ADDR_PTR( sAddObj4Prev );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sPrev->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Next, ID_SIZEOF(idShmAddr) );
    sNext = ( iduShmListNode* )IDU_SHM_GET_ADDR_PTR( sAddObj4Next );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNext->mAddrPrev, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::writeAddListFirst( iduShmTxInfo    * aShmTxInfo,
                                          iduShmListNode  * aBase,
                                          iduShmListNode  * aBaseNext,
                                          iduShmListNode  * aFirst,
                                          iduShmListNode  * aLast )
{
    idrUImgInfo  sArrUImgInfo[8];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aFirst->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aFirst->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aLast->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aLast->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[4], &aBaseNext->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[5], &aBaseNext->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[6], &aBase->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[7], &aBase->mAddrNext, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
                         IDU_VLOG_TYPE_SHM_LIST_ADDLIST_FIRST,
                         8,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::undo_SHM_LIST_ADDLIST_FIRST( idvSQL         * /*aStatistics*/,
                                                    iduShmTxInfo   * /*aShmTxInfo*/,
                                                    idrLSN           /*aNTALsn*/,
                                                    UShort           /*aSize*/,
                                                    SChar          * aImage )
{

    idShmAddr       sAddObj4Base;
    idShmAddr       sAddObj4BaseNext;
    idShmAddr       sAddObj4First;
    idShmAddr       sAddObj4Last;
    SChar         * sCurLogPtr;

    iduShmListNode * sBase;
    iduShmListNode * sBaseNext;
    iduShmListNode * sFirst;
    iduShmListNode * sLast;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4First, ID_SIZEOF(idShmAddr) );
    sFirst = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4First );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sFirst->mAddrPrev, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Last, ID_SIZEOF(idShmAddr) );
    sLast = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4Last );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sLast->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4BaseNext, ID_SIZEOF(idShmAddr) );
    sBaseNext = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4BaseNext );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sBaseNext->mAddrPrev, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Base, ID_SIZEOF(idShmAddr) );
    sBase = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4Base );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sBase->mAddrNext, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::writeRemove( iduShmTxInfo   * aShmTxInfo,
                                    iduShmListNode * aNode,
                                    iduShmListNode * aNodePrev,
                                    iduShmListNode * aNodeNext )
{
    idrUImgInfo  sArrUImgInfo[7];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aNode->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aNode->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aNode->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aNodePrev->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[4], &aNodePrev->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[5], &aNodeNext->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[6], &aNodeNext->mAddrPrev, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
                         IDU_VLOG_TYPE_SHM_LIST_REMOVE,
                         7,
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmList::undo_SHM_LIST_REMOVE( idvSQL         * /*aStatistics*/,
                                             iduShmTxInfo   * /*aShmTxInfo*/,
                                             idrLSN           /*aNTALsn*/,
                                             UShort           /*aSize*/,
                                             SChar          * aImage )
{
    idShmAddr       sAddObj4Node;
    idShmAddr       sAddObj4PrevNode;
    idShmAddr       sAddObj4NextNode;
    SChar         * sCurLogPtr;

    iduShmListNode * sNode;
    iduShmListNode * sPrevNode;
    iduShmListNode * sNextNode;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Node, ID_SIZEOF(idShmAddr) );
    sNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4Node );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNode->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNode->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4PrevNode, ID_SIZEOF(idShmAddr) );
    sPrevNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4PrevNode );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sPrevNode->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4NextNode, ID_SIZEOF(idShmAddr) );
    sNextNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( sAddObj4NextNode );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sNextNode->mAddrPrev, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;

}

