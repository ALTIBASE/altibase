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
 * $Id: stnmrFT.cpp 19860 2007-02-07 02:09:39Z kimmkeun $
 *
 * Description
 *
 *   BUG-18601
 *   Mem RTree Index 의 DUMP를 위한 함수
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <stErrorCode.h>
#include <stix.h>
#include <stnmrModule.h>
#include <stnmrFT.h>

/***********************************************************************
 * Description
 *
 *   D$MEM_INDEX_RTREE_STRUCTURE
 *   : MEMORY RTREE INDEX의 Page Tree 구조 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$MEM_INDEX_RTREE_STRUCTURE Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpMemRTreeStructureColDesc[]=
{
    {
        (SChar*)"DEPTH",
        offsetof(stnmrDumpTreePage, mDepth ),
        IDU_FT_SIZEOF(stnmrDumpTreePage, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(stnmrDumpTreePage, mIsLeaf ),
        IDU_FT_SIZEOF(stnmrDumpTreePage, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(stnmrDumpTreePage, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof(stnmrDumpTreePage, mSlotCount ),
        IDU_FT_SIZEOF(stnmrDumpTreePage, mSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(stnmrDumpTreePage, mParentPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$MEM_INDEX_RTREE_STRUCTURE Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpMemRTreeStructureTableDesc =
{
    (SChar *)"D$MEM_INDEX_RTREE_STRUCTURE",
    stnmrFT::buildRecordTreeStructure,
    gDumpMemRTreeStructureColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$VOL_INDEX_RTREE_STRUCTURE Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpVolRTreeStructureColDesc[]=
{
    {
        (SChar*)"DEPTH",
        offsetof(stnmrDumpTreePage, mDepth ),
        IDU_FT_SIZEOF(stnmrDumpTreePage, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(stnmrDumpTreePage, mIsLeaf ),
        IDU_FT_SIZEOF(stnmrDumpTreePage, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(stnmrDumpTreePage, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof(stnmrDumpTreePage, mSlotCount ),
        IDU_FT_SIZEOF(stnmrDumpTreePage, mSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(stnmrDumpTreePage, mParentPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$VOL_INDEX_RTREE_STRUCTURE Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpVolRTreeStructureTableDesc =
{
    (SChar *)"D$VOL_INDEX_RTREE_STRUCTURE",
    stnmrFT::buildRecordTreeStructure,
    gDumpVolRTreeStructureColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$MEM_INDEX_RTREE_STRUCTURE Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC
stnmrFT::buildRecordTreeStructure( idvSQL              * /* aStatistics */,
                                   void                * aHeader,
                                   void                * aDumpObj,
                                   iduFixedTableMemory * aMemory )
{
    stnmrHeader      * sIdxHdr;
    stnmrNode        * sRootNode;
    UInt               sState = 0;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );
    
    //------------------------------------------
    // Get Memory RTree Index Header
    //------------------------------------------
    sIdxHdr = (stnmrHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr == NULL )
    {
        /* BUG-32417 [sm-mem-index] The fixed table 'X$MEM_BTREE_HEADER'
         * doesn't consider that indices is disabled. 
         * IndexRuntimeHeader가 없는 경우는 제외한다. */
    }
    else
    {
        IDE_TEST( sIdxHdr->mMutex.lock( NULL /* idvSQL* */) != IDE_SUCCESS );
        sState = 1;

        sRootNode = (stnmrNode*)(sIdxHdr->mRoot);

        //------------------------------------------
        // Set Root Page Record
        //------------------------------------------

        if( sRootNode == NULL )
        {
            // Nothing To Do
        }
        else
        {
            IDE_TEST( traverseBuildTreePage( aHeader,
                                             aMemory,
                                             0,
                                             sRootNode,
                                             NULL )
                      != IDE_SUCCESS );
        }

        //------------------------------------------
        // Finalize
        //------------------------------------------

        // unlatch tree latch
        sState = 0;
        IDE_TEST( sIdxHdr->mMutex.unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_STNMR_INVALID_DUMP_OBJECT));
    }
    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_STNMR_DUMP_EMPTY_OBJECT));
    }
    
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        sState = 0;
        IDE_PUSH();
        (void)sIdxHdr->mMutex.unlock();
        IDE_POP();
    }

    return IDE_FAILURE;    
}
    
IDE_RC
stnmrFT::traverseBuildTreePage( void                * aHeader,
                                iduFixedTableMemory * aMemory,
                                SInt                  aDepth,
                                stnmrNode           * aCurNode,
                                stnmrNode           * aParentNode )
{
    SInt                i;
    stnmrNode         * sChildNode;
    stnmrDumpTreePage   sDumpTreePage;
    SInt                sSlotCount;
    SChar               sCurNodePtr[100];
    SChar               sParentNodePtr[100];

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );
    IDE_DASSERT( aCurNode != NULL );

    //------------------------------------------
    // Initialize
    //------------------------------------------
    
    idlOS::memset( & sDumpTreePage, 0x00, ID_SIZEOF( stnmrDumpTreePage ) );
        
    //------------------------------------------
    // Build My Record
    //------------------------------------------

    sSlotCount = aCurNode->mSlotCount;

    sDumpTreePage.mSlotCount = sSlotCount;
    sDumpTreePage.mDepth = aDepth;
    sDumpTreePage.mIsLeaf = ((aCurNode->mFlag & STNMR_NODE_TYPE_MASK) ==
                             STNMR_NODE_TYPE_INTERNAL)? 'F' : 'T';

    idlOS::sprintf( (SChar*)sCurNodePtr, "%"ID_XPOINTER_FMT, (SChar*)aCurNode );
    sDumpTreePage.mMyPagePtr = sCurNodePtr;
    idlOS::sprintf( (SChar*)sParentNodePtr, "%"ID_XPOINTER_FMT, (SChar*)aParentNode );
    sDumpTreePage.mParentPagePtr = sParentNodePtr;
  
    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void *) & sDumpTreePage )
             != IDE_SUCCESS);

    //------------------------------------------
    // Build Child Record
    //------------------------------------------

    if((aCurNode->mFlag & STNMR_NODE_TYPE_MASK) == STNMR_NODE_TYPE_INTERNAL)
    {
        for (i = 0; i < sSlotCount; i++ )
        {
            sChildNode = aCurNode->mSlots[i].mNodePtr;

            IDE_TEST( traverseBuildTreePage( aHeader,
                                             aMemory,
                                             aDepth + 1,
                                             sChildNode,
                                             aCurNode )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_DASSERT( (aCurNode->mFlag & STNMR_NODE_TYPE_MASK) ==
                     STNMR_NODE_TYPE_LEAF );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description
 *
 *   D$MEM_INDEX_RTREE_KEY
 *   : MEMORY RTREE INDEX의 KEY 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$MEM_INDEX_RTREE_KEY Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpMemRTreeKeyColDesc[]=
{
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(stnmrDumpKey, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DEPTH",
        offsetof(stnmrDumpKey, mDepth ),
        IDU_FT_SIZEOF(stnmrDumpKey, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(stnmrDumpKey, mIsLeaf ),
        IDU_FT_SIZEOF(stnmrDumpKey, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(stnmrDumpKey, mParentPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(stnmrDumpKey, mNthSlot ),
        IDU_FT_SIZEOF(stnmrDumpKey, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_X",
        offsetof(stnmrDumpKey, mMinX ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_Y",
        offsetof(stnmrDumpKey, mMinY ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_X",
        offsetof(stnmrDumpKey, mMaxX ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_Y",
        offsetof(stnmrDumpKey, mMaxY ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHILD_PAGE_PTR",
        offsetof(stnmrDumpKey, mChildPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_PTR",
        offsetof(stnmrDumpKey, mRowPtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$MEM_INDEX_RTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpMemRTreeKeyTableDesc =
{
    (SChar *)"D$MEM_INDEX_RTREE_KEY",
    stnmrFT::buildRecordKey,
    gDumpMemRTreeKeyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$VOL_INDEX_RTREE_KEY Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpVolRTreeKeyColDesc[]=
{
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(stnmrDumpKey, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DEPTH",
        offsetof(stnmrDumpKey, mDepth ),
        IDU_FT_SIZEOF(stnmrDumpKey, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(stnmrDumpKey, mIsLeaf ),
        IDU_FT_SIZEOF(stnmrDumpKey, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(stnmrDumpKey, mParentPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(stnmrDumpKey, mNthSlot ),
        IDU_FT_SIZEOF(stnmrDumpKey, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_X",
        offsetof(stnmrDumpKey, mMinX ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_Y",
        offsetof(stnmrDumpKey, mMinY ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_X",
        offsetof(stnmrDumpKey, mMaxX ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_Y",
        offsetof(stnmrDumpKey, mMaxY ),
        IDU_FT_SIZEOF(stnmrDumpKey, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHILD_PAGE_PTR",
        offsetof(stnmrDumpKey, mChildPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_PTR",
        offsetof(stnmrDumpKey, mRowPtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$VOL_INDEX_RTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpVolRTreeKeyTableDesc =
{
    (SChar *)"D$VOL_INDEX_RTREE_KEY",
    stnmrFT::buildRecordKey,
    gDumpVolRTreeKeyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$MEM_INDEX_RTREE_HEADER Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC stnmrFT::buildRecordKey( idvSQL              * /*aStatistics*/,
                                void                * aHeader,
                                void                * aDumpObj,
                                iduFixedTableMemory * aMemory )
{
    stnmrHeader       * sIdxHdr;
    stnmrNode         * sRootNode;
    UInt                sState = 0;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Memory RTree Index Header
    //------------------------------------------
    
    sIdxHdr = (stnmrHeader*) (((smnIndexHeader*)aDumpObj)->mHeader);

    if( sIdxHdr == NULL )
    {
        /* BUG-32417 [sm-mem-index] The fixed table 'X$MEM_BTREE_HEADER'
         * doesn't consider that indices is disabled. 
         * IndexRuntimeHeader가 없는 경우는 제외한다. */
    }
    else
    {
        // Set Tree Latch
        IDE_TEST( sIdxHdr->mMutex.lock( NULL /* idvSQL* */) != IDE_SUCCESS );
        sState = 1;

        // Get Root Page ID
        sRootNode = (stnmrNode*)(sIdxHdr->mRoot);

        //------------------------------------------
        // Set Root Page Record
        //------------------------------------------

        if( sRootNode == NULL )
        {
            // Nothing To Do
        }
        else
        {
            IDE_TEST( traverseBuildKey( aHeader,
                                        aMemory,
                                        sIdxHdr,
                                        0,        // DEPTH
                                        sRootNode,// START NODE POINTER
                                        NULL )    // PARENT NODE POINTER
                      != IDE_SUCCESS );
        }

        //------------------------------------------
        // Finalize
        //------------------------------------------

        // unlatch tree latch
        sState = 0;
        IDE_TEST( sIdxHdr->mMutex.unlock() != IDE_SUCCESS );
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_STNMR_INVALID_DUMP_OBJECT));
    }
    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_STNMR_DUMP_EMPTY_OBJECT));
    }
    
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        sState = 0;
        IDE_PUSH();
        (void)sIdxHdr->mMutex.unlock();
        IDE_POP();
    }
    
    return IDE_FAILURE;    
}
    
IDE_RC stnmrFT::traverseBuildKey( void                * aHeader,
                                  iduFixedTableMemory * aMemory,
                                  stnmrHeader         * aIdxHdr,
                                  SInt                  aDepth,
                                  stnmrNode           * aCurNode,
                                  stnmrNode           * aParentNode )
{
    SInt               i;
    idBool             sIsLeaf = ID_FALSE;
    stnmrNode        * sChildNode;
    stnmrDumpKey       sDumpKey;
    
    SInt               sSlotCount;
    SChar              sCurNodePtrString[100];
    SChar              sParentNodePtrString[100];
    SChar              sChildNodePtrString[100];
    SChar              sRowPtrString[100];
    
    stnmrSlot        * sSlot;
    void             * sNullPtr = NULL;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );
    IDE_DASSERT( aCurNode != NULL );

    //------------------------------------------
    // Initialize
    //------------------------------------------
    
    idlOS::memset( & sDumpKey, 0x00, ID_SIZEOF( stnmrDumpKey ) );
        
    //------------------------------------------
    // Build My Record
    //------------------------------------------

    if( (aCurNode->mFlag & STNMR_NODE_TYPE_MASK) == STNMR_NODE_TYPE_LEAF )
    {
        sIsLeaf = ID_TRUE;
    }

    sSlotCount = aCurNode->mSlotCount;
    
    sDumpKey.mDepth = aDepth;
    sDumpKey.mIsLeaf = ( sIsLeaf == ID_TRUE )? 'T' : 'F';

    (void)idlOS::sprintf( (SChar*)sCurNodePtrString,
                          "%"ID_XPOINTER_FMT,
                          (SChar*)aCurNode );
    sDumpKey.mMyPagePtr = sCurNodePtrString;
    
    (void)idlOS::sprintf( (SChar*)sParentNodePtrString,
                          "%"ID_XPOINTER_FMT,
                          (SChar*)aParentNode );
    sDumpKey.mParentPagePtr = sParentNodePtrString;
  
    for ( i = 0; i < sSlotCount; i++ )
    {
        sSlot = (stnmrSlot*)&(aCurNode->mSlots[i]);
        
        if ( sIsLeaf == ID_TRUE )
        {
            sDumpKey.mNthSlot = i;
            (void)idlOS::sprintf( (SChar*)sChildNodePtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sNullPtr );
            sDumpKey.mChildPagePtr = sChildNodePtrString;
            
            (void)idlOS::sprintf( (SChar*)sRowPtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sSlot->mPtr );
            sDumpKey.mRowPtr = sRowPtrString;
        }
        else
        {
            sDumpKey.mNthSlot = i;
            (void)idlOS::sprintf( (SChar*)sChildNodePtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sSlot->mNodePtr );
            sDumpKey.mChildPagePtr = sChildNodePtrString;
            
            (void)idlOS::sprintf( (SChar*)sRowPtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sNullPtr );
            sDumpKey.mRowPtr = sRowPtrString;
        }
        
        //------------------------------
        // MBR 설정
        //------------------------------
        sDumpKey.mMinX = sSlot->mMbr.mMinX;
        sDumpKey.mMinY = sSlot->mMbr.mMinY;
        sDumpKey.mMaxX = sSlot->mMbr.mMaxX;
        sDumpKey.mMaxY = sSlot->mMbr.mMaxY;
        
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) & sDumpKey )
                 != IDE_SUCCESS);
    }
    
    //------------------------------------------
    // Build Child Record
    //------------------------------------------

    if( sIsLeaf == ID_FALSE )
    {
        for (i = 0; i < sSlotCount; i++ )
        {
            sChildNode = aCurNode->mSlots[i].mNodePtr;

            IDE_TEST( traverseBuildKey( aHeader,
                                        aMemory,
                                        aIdxHdr,
                                        aDepth + 1,
                                        sChildNode,
                                        aCurNode )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
