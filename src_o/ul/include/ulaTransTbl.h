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
 
#ifndef _O_ULA_TRANSTBL_H_
#define _O_ULA_TRANSTBL_H_ 1

#include <ula.h>
#include <ulaXLogLinkedList.h>

#define ULA_TRANS_TABLE_NODE_MUTEX_NAME (acp_char_t *)"TRANS_TABLE_NODE_MUTEX"
#define ULA_COLLECTION_LIST_MUTEX_NAME  (acp_char_t *)"COLLECTION_LIST_MUTEX"

typedef struct ulaLOBLocEntry
{
    ulaLobLocator  mRemoteLL;   // XLog Sender Lob Locator
    ulaLobLocator  mLocalLL;    // XLog Collector Lob Locator

    struct ulaLOBLocEntry *mNext;
} ulaLOBLocEntry;

typedef struct ulaTransTblNode
{
    ulaTID              mTID;        // Transaction ID
    ulaSN               mBeginSN;    // if ULA_SN_NULL, then inactive.
    ulaLOBLocEntry      mLocHead;

    ulaXLogLinkedList   mCollectionList;  // for Committed Tx Collection Only
} ulaTransTblNode;


/*
 * -----------------------------------------------------------------------------
 *  Transaction Table struct
 * -----------------------------------------------------------------------------
 */
typedef struct ulaTransTbl
{
    ulaTransTblNode *mTransTbl;            // Transaction Table
    acp_uint32_t     mTblSize;             // Transactin Table Size
    acp_sint32_t     mATransCnt;           // Active Transaction Count
    acp_thr_mutex_t  mTransTblNodeMutex;   // for Transaction Table Node

    acp_bool_t       mUseCollectionList;
    acp_thr_mutex_t  mCollectionListMutex; // for Committed TX Collection
} ulaTransTbl;

/*
 * -----------------------------------------------------------------------------
 *  Initialize / Destroy
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaTransTblInitialize(ulaTransTbl  *aTbl,
                             acp_uint32_t  aTblSize,
                             acp_bool_t    aUseCollectionList,
                             ulaErrorMgr  *aOutErrorMgr);
ACI_RC ulaTransTblDestroy(ulaTransTbl *aTbl, ulaErrorMgr *aOutErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Is Active Transaction
 * -----------------------------------------------------------------------------
 */
acp_bool_t         ulaTransTblIsATransNode(ulaTransTblNode *aNode);
acp_bool_t         ulaTransTblIsATrans(ulaTransTbl *aTbl, ulaTID aTID);
acp_uint32_t       ulaTransTblGetTableSize(ulaTransTbl *aTbl);
ulaXLogLinkedList *ulaTransTblGetCollectionList(ulaTransTbl *aTbl, ulaTID aTID);

ulaTransTblNode *ulaTransTblGetTrNodeDirect(ulaTransTbl  *aTbl,
                                            acp_uint32_t  aIndex);

/*
 * -----------------------------------------------------------------------------
 *  Insert / Remove Transaction
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaTransTblInsertTrans(ulaTransTbl *aTbl,
                              ulaTID       aTID,
                              ulaSN        aBeginSN,
                              ulaErrorMgr *aOutErrorMgr);
ACI_RC ulaTransTblRemoveTrans(ulaTransTbl *aTbl,
                              ulaTID       aTID,
                              ulaErrorMgr *aOutErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Lob Locator Related Functions
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaTransTblInsertLocator(ulaTransTbl    *aTbl,
                                ulaTID          aTID,
                                ulaLobLocator   aRemoteLL,
                                ulaLobLocator   aLocalLL,
                                ulaErrorMgr    *aOutErrorMgr);
ACI_RC ulaTransTblRemoveLocator(ulaTransTbl   *aTbl,
                                ulaTID         aTID,
                                ulaLobLocator  aRemoteLL,
                                ulaErrorMgr   *aOutErrorMgr);
ACI_RC ulaTransTblRemoveAllLocator(ulaTransTbl  *aTbl,
                                   ulaTID        aTID,
                                   ulaErrorMgr  *aOutErrorMgr);
ACI_RC ulaTransTblSearchLocator(ulaTransTbl   *aTbl,
                                ulaTID         aTID,
                                ulaLobLocator  aRemoteLL,
                                ulaLobLocator *aOutLocalLL,
                                acp_bool_t    *aOutIsFound,
                                ulaErrorMgr   *aOutErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Lock / Unlock Collection list
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaTransTblLockCollectionList(ulaTransTbl *aTbl,
                                     ulaErrorMgr *aOutErrorMgr);
ACI_RC ulaTransTblUnlockCollectionList(ulaTransTbl *aTbl,
                                       ulaErrorMgr *aOutErrorMgr);

#endif /* _O_ULA_TRANSTBL_H_ */
