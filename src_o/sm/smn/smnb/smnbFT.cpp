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
 * $Id: smnbFT.cpp 19860 2007-02-07 02:09:39Z kimmkeun $
 *
 * Description
 *
 *   PROJ-1618
 *   Mem BTree Index 의 DUMP를 위한 함수
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smErrorCode.h>

#include <sdbBufferMgr.h>
#include <sdpPhyPage.h>
#include <smnbModule.h>
#include <smnbFT.h>
#include <smnReq.h>

/***********************************************************************
 * Description
 *
 *   D$MEM_INDEX_BTREE_STRUCTURE
 *   : MEMORY BTREE INDEX의 Page Tree 구조 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$MEM_INDEX_BTREE_STRUCTURE Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpMemBTreeStructureColDesc[]=
{
    {
        (SChar*)"DEPTH",
        offsetof(smnbDumpTreePage, mDepth ),
        IDU_FT_SIZEOF(smnbDumpTreePage, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(smnbDumpTreePage, mIsLeaf ),
        IDU_FT_SIZEOF(smnbDumpTreePage, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(smnbDumpTreePage, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof(smnbDumpTreePage, mSlotCount ),
        IDU_FT_SIZEOF(smnbDumpTreePage, mSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(smnbDumpTreePage, mParentPagePtr ),
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
// D$MEM_INDEX_BTREE_STRUCTURE Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpMemBTreeStructureTableDesc =
{
    (SChar *)"D$MEM_INDEX_BTREE_STRUCTURE",
    smnbFT::buildRecordTreeStructure,
    gDumpMemBTreeStructureColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$VOL_INDEX_BTREE_STRUCTURE Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpVolBTreeStructureColDesc[]=
{
    {
        (SChar*)"DEPTH",
        offsetof(smnbDumpTreePage, mDepth ),
        IDU_FT_SIZEOF(smnbDumpTreePage, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(smnbDumpTreePage, mIsLeaf ),
        IDU_FT_SIZEOF(smnbDumpTreePage, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(smnbDumpTreePage, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof(smnbDumpTreePage, mSlotCount ),
        IDU_FT_SIZEOF(smnbDumpTreePage, mSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(smnbDumpTreePage, mParentPagePtr ),
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
// D$VOL_INDEX_BTREE_STRUCTURE Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpVolBTreeStructureTableDesc =
{
    (SChar *)"D$VOL_INDEX_BTREE_STRUCTURE",
    smnbFT::buildRecordTreeStructure,
    gDumpVolBTreeStructureColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$MEM_INDEX_BTREE_STRUCTURE Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC
smnbFT::buildRecordTreeStructure( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * aDumpObj,
                                  iduFixedTableMemory * aMemory )
{
    smnbHeader       * sIdxHdr = NULL;
    smnbNode         * sRootNode;
    UInt               sState = 0;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );
    
    //------------------------------------------
    // Get Memory BTree Index Header
    //------------------------------------------
    sIdxHdr = (smnbHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr == NULL )
    {
        /* BUG-32417 [sm-mem-index] The fixed table 'X$MEM_BTREE_HEADER'
         * doesn't consider that indices is disabled. 
         * IndexRuntimeHeader가 없는 경우는 제외한다. */
    }
    else
    {
        smnbBTree::lockTree( sIdxHdr );
        sState = 1;

        sRootNode = (smnbNode*)(sIdxHdr->root);

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
        smnbBTree::unlockTree( sIdxHdr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_INVALID_DUMP_OBJECT ));
    }
    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ));
    }
    
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        sState = 0;
        IDE_PUSH();
        smnbBTree::unlockTree( sIdxHdr );
        IDE_POP();
    }

    return IDE_FAILURE;    
}
    
IDE_RC
smnbFT::traverseBuildTreePage( void                * aHeader,
                               iduFixedTableMemory * aMemory,
                               SInt                  aDepth,
                               smnbNode            * aCurNode,
                               smnbNode            * aParentNode )
{
    SInt               i;
    smnbNode         * sChildNode;
    smnbDumpTreePage   sDumpTreePage;
    SInt               sSlotCount;
    SChar              sCurNodePtr[100];
    SChar              sParentNodePtr[100];

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );
    IDE_ERROR( aCurNode != NULL );

    //------------------------------------------
    // Initialize
    //------------------------------------------
    
    idlOS::memset( & sDumpTreePage, 0x00, ID_SIZEOF( smnbDumpTreePage ) );
        
    //------------------------------------------
    // Build My Record
    //------------------------------------------

    if ( SMNB_IS_INTERNAL_NODE( aCurNode ) )
    {
        sSlotCount = ((smnbINode*)aCurNode)->mSlotCount;
    }
    else
    {
        sSlotCount = ((smnbLNode*)aCurNode)->mSlotCount;
    }
    sDumpTreePage.mSlotCount = sSlotCount;
    sDumpTreePage.mDepth = aDepth;
    sDumpTreePage.mIsLeaf = SMNB_IS_INTERNAL_NODE( aCurNode ) ? 'F' : 'T';

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

    if ( SMNB_IS_INTERNAL_NODE( aCurNode ) )
    {
        for (i = 0; i < sSlotCount; i++ )
        {
            sChildNode = (smnbNode*)(((smnbINode*)aCurNode)->mChildPtrs[i]);

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
        IDE_DASSERT( SMNB_IS_LEAF_NODE( aCurNode ) );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description
 *
 *   D$MEM_INDEX_BTREE_KEY
 *   : MEMORY BTREE INDEX의 KEY 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$MEM_INDEX_BTREE_KEY Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpMemBTreeKeyColDesc[]=
{
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(smnbDumpKey, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DEPTH",
        offsetof(smnbDumpKey, mDepth ),
        IDU_FT_SIZEOF(smnbDumpKey, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(smnbDumpKey, mIsLeaf ),
        IDU_FT_SIZEOF(smnbDumpKey, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(smnbDumpKey, mParentPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(smnbDumpKey, mNthSlot ),
        IDU_FT_SIZEOF(smnbDumpKey, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_COLUMN",
        offsetof(smnbDumpKey, mNthColumn ),
        IDU_FT_SIZEOF(smnbDumpKey, mNthColumn ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"VALUE24B",
        offsetof(smnbDumpKey, mValue ),
        IDU_FT_SIZEOF(smnbDumpKey, mValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHILD_PAGE_PTR",
        offsetof(smnbDumpKey, mChildPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_PTR",
        offsetof(smnbDumpKey, mRowPtr ),
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
// D$MEM_INDEX_BTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpMemBTreeKeyTableDesc =
{
    (SChar *)"D$MEM_INDEX_BTREE_KEY",
    smnbFT::buildRecordKey,
    gDumpMemBTreeKeyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$VOL_INDEX_BTREE_KEY Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpVolBTreeKeyColDesc[]=
{
    {
        (SChar*)"MY_PAGE_PTR",
        offsetof(smnbDumpKey, mMyPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DEPTH",
        offsetof(smnbDumpKey, mDepth ),
        IDU_FT_SIZEOF(smnbDumpKey, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(smnbDumpKey, mIsLeaf ),
        IDU_FT_SIZEOF(smnbDumpKey, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGE_PTR",
        offsetof(smnbDumpKey, mParentPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(smnbDumpKey, mNthSlot ),
        IDU_FT_SIZEOF(smnbDumpKey, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_COLUMN",
        offsetof(smnbDumpKey, mNthColumn ),
        IDU_FT_SIZEOF(smnbDumpKey, mNthColumn ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"VALUE24B",
        offsetof(smnbDumpKey, mValue ),
        IDU_FT_SIZEOF(smnbDumpKey, mValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHILD_PAGE_PTR",
        offsetof(smnbDumpKey, mChildPagePtr ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_PTR",
        offsetof(smnbDumpKey, mRowPtr ),
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
// D$VOL_INDEX_BTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpVolBTreeKeyTableDesc =
{
    (SChar *)"D$VOL_INDEX_BTREE_KEY",
    smnbFT::buildRecordKey,
    gDumpVolBTreeKeyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$MEM_INDEX_BTREE_HEADER Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC smnbFT::buildRecordKey( idvSQL              * /*aStatistics*/,
                               void                * aHeader,
                               void                * aDumpObj,
                               iduFixedTableMemory * aMemory )
{
    smnbHeader       * sIdxHdr = NULL;
    smnbNode         * sRootNode;
    UInt               sState = 0;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Mem BTree Index Header
    //------------------------------------------

    sIdxHdr = (smnbHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr == NULL )
    {
        /* BUG-32417 [sm-mem-index] The fixed table 'X$MEM_BTREE_HEADER'
         * doesn't consider that indices is disabled. 
         * IndexRuntimeHeader가 없는 경우는 제외한다. */
    }
    else
    {
        // Set Tree Latch
        smnbBTree::lockTree( sIdxHdr );
        sState = 1;

        // Get Root Page ID
        sRootNode = (smnbNode*)(sIdxHdr->root);

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
        smnbBTree::unlockTree( sIdxHdr );
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_INVALID_DUMP_OBJECT ));
    }
    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ));
    }
    
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        sState = 0;
        IDE_PUSH();
        smnbBTree::unlockTree( sIdxHdr );
        IDE_POP();
    }
    
    return IDE_FAILURE;    
}
    
IDE_RC smnbFT::traverseBuildKey( void                * aHeader,
                                 iduFixedTableMemory * aMemory,
                                 smnbHeader          * aIdxHdr,
                                 SInt                  aDepth,
                                 smnbNode            * aCurNode,
                                 smnbNode            * aParentNode )
{
    SInt               i, j;
    idBool             sIsLeaf = ID_FALSE;
    smnbNode         * sChildNode;
    smnbDumpKey        sDumpKey;
    
    SInt               sSlotCount;
    SChar              sCurNodePtrString[100];
    SChar              sParentNodePtrString[100];
    SChar              sChildNodePtrString[100];
    SChar              sRowPtrString[100];
    void             * sRowSlot     = NULL;
    void             * sChildSlot   = NULL;
    void             * sRowPtr;
    smnbColumn       * sKeyColumn;
    UChar              sValueBuffer[SM_DUMP_VALUE_BUFFER_SIZE];
    UInt               sValueLength;
    ULong              sKeyValue[MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)];
    UShort             sKeySize;
    IDE_RC             sReturn;
    void             * sNullPtr = NULL;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );
    IDE_ERROR( aCurNode != NULL );

    //------------------------------------------
    // Initialize
    //------------------------------------------
    
    idlOS::memset( & sDumpKey, 0x00, ID_SIZEOF( smnbDumpKey ) );
        
    //------------------------------------------
    // Build My Record
    //------------------------------------------

    if ( SMNB_IS_LEAF_NODE( aCurNode ) )
    {
        sIsLeaf = ID_TRUE;
    }

    if( sIsLeaf == ID_TRUE )
    {
        sSlotCount = ((smnbLNode*)aCurNode)->mSlotCount;
    }
    else
    {
        sSlotCount = ((smnbINode*)aCurNode)->mSlotCount;
    }
    
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
        if ( sIsLeaf == ID_TRUE )
        {
            sRowSlot = (void *)(((smnbLNode *)aCurNode)->mRowPtrs[i]);
            
            sDumpKey.mNthSlot = i;
            (void)idlOS::sprintf( (SChar*)sChildNodePtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sNullPtr );
            sDumpKey.mChildPagePtr = sChildNodePtrString;
            
            (void)idlOS::sprintf( (SChar*)sRowPtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sRowSlot );
            sDumpKey.mRowPtr = sRowPtrString;
            sRowPtr = sRowSlot;
        }
        else
        {
            sRowSlot    = (void *)(((smnbINode *)aCurNode)->mRowPtrs[i]);
            sChildSlot  = (void *)(((smnbINode *)aCurNode)->mChildPtrs[i]);

            sDumpKey.mNthSlot = i;
            (void)idlOS::sprintf( (SChar*)sChildNodePtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sChildSlot );
            sDumpKey.mChildPagePtr = sChildNodePtrString;
            
            (void)idlOS::sprintf( (SChar*)sRowPtrString,
                                  "%"ID_XPOINTER_FMT,
                                  sRowSlot );
            sDumpKey.mRowPtr = sRowPtrString;
            sRowPtr = sRowSlot;
        }
        
        //------------------------------
        // Column별 Value String 생성
        //------------------------------
        
        for ( sKeyColumn = aIdxHdr->columns, j = 0;
              sKeyColumn < aIdxHdr->fence;
              sKeyColumn++, j++ )
        {
            sDumpKey.mNthColumn = j;
            idlOS::memset( sDumpKey.mValue, 0x00, SM_DUMP_VALUE_LENGTH );

            if( sRowPtr != NULL )
            {
                if( smnbBTree::isNullColumn( sKeyColumn,
                                             &(sKeyColumn->column),
                                             (SChar*)sRowPtr ) == ID_TRUE )
                {
                    // Null 
                }
                else
                {
                    (void) smnbBTree::getKeyValueAndSize( (SChar*)sRowPtr,
                                                          sKeyColumn,
                                                          (void*) sKeyValue,
                                                          & sKeySize );
                
                    sValueLength = SM_DUMP_VALUE_BUFFER_SIZE;
                    IDE_TEST( sKeyColumn->key2String(
                                  &sKeyColumn->column,
                                  (void*) sKeyValue,
                                  sKeySize,
                                  (UChar*) SM_DUMP_VALUE_DATE_FMT,
                                  idlOS::strlen( SM_DUMP_VALUE_DATE_FMT ),
                                  sValueBuffer,
                                  &sValueLength,
                                  &sReturn ) != IDE_SUCCESS );
                    
                    idlOS::memcpy( sDumpKey.mValue,
                                   sValueBuffer,
                                   ( sValueLength > SM_DUMP_VALUE_LENGTH ) ?
                                   SM_DUMP_VALUE_LENGTH : sValueLength );
                }
            }

            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *) & sDumpKey )
                     != IDE_SUCCESS);
        }
    }
    
    //------------------------------------------
    // Build Child Record
    //------------------------------------------

    if( sIsLeaf == ID_FALSE )
    {
        for (i = 0; i < sSlotCount; i++ )
        {
            sChildNode = (smnbNode*)(((smnbINode*)aCurNode)->mChildPtrs[i]);

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

//======================================================================
//  X$MEM_BTREE_HEADER
//  memory index의 run-time header를 보여주는 peformance view
//======================================================================
IDE_RC smnbFT::buildRecordForMemBTreeHeader(idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * /* aDumpObj */,
                                            iduFixedTableMemory * aMemory)
{
    IDE_TEST( buildRecordForBTreeHeader( aHeader,
                                         aMemory,
                                         SMI_TABLE_META,
                                         SMC_CAT_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( buildRecordForBTreeHeader( aHeader,
                                         aMemory,
                                         SMI_TABLE_MEMORY,
                                         SMC_CAT_TABLE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//======================================================================
//  X$VOL_BTREE_HEADER
//  volatile index의 run-time header를 보여주는 peformance view
//======================================================================
IDE_RC smnbFT::buildRecordForVolBTreeHeader(idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * /* aDumpObj */,
                                            iduFixedTableMemory * aMemory)
{
    return buildRecordForBTreeHeader( aHeader,
                                      aMemory,
                                      SMI_TABLE_VOLATILE,
                                      SMC_CAT_TABLE );
}

//======================================================================
//  X$TEMP_BTREE_HEADER
//  temporary index의 run-time header를 보여주는 peformance view
//======================================================================
IDE_RC smnbFT::buildRecordForTempBTreeHeader(idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory)
{
    return buildRecordForBTreeHeader( aHeader,
                                      aMemory,
                                      SMI_TABLE_VOLATILE,
                                      SMC_CAT_TEMPTABLE );
}

IDE_RC smnbFT::buildRecordForBTreeHeader( void                * aHeader,
                                          iduFixedTableMemory * aMemory,
                                          UInt                  aTableType,
                                          smcTableHeader      * aCatTblHdr )
{
    smcTableHeader   * sTableHeader;
    smpSlotHeader    * sPtr;
    SChar            * sCurPtr;
    SChar            * sNxtPtr;
    UInt               sTableType;
    smnIndexHeader   * sIndexCursor;
    smnbHeader       * sIndexHeader;
    smnbHeader4PerfV   sIndexHeader4PerfV;
    void             * sTrans;
    UInt               sIndexCnt;
    UInt               i;
    void             * sISavepoint = NULL;
    UInt               sDummy = 0;
    UInt               sState = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurPtr = NULL;

    if ( aMemory->useExternalMemory() == ID_TRUE )
    {
        /* BUG-43006 FixedTable Indexing Filter */
        sTrans = ((smiFixedTableProperties *)aMemory->getContext())->mTrans;
    }
    else
    {
        /* BUG-32292 [sm-util] Self deadlock occur since fixed-table building
         * operation uses another transaction.
         * NestedTransaction을 사용하면 Self-deadlock 우려가 있다.
         * 따라서 id Memory 영역으로부터 Iterator를 얻어 Transaction을 얻어낸다. */
        sTrans = ((smiIterator*)aMemory->getContext())->trans;
    }

    while(1)
    {
        IDE_TEST(smcRecord::nextOIDall(aCatTblHdr,sCurPtr,&sNxtPtr)
                  != IDE_SUCCESS);
        if( sNxtPtr == NULL )
        {
            break;
        }
        sPtr = (smpSlotHeader *)sNxtPtr;

        if( SM_SCN_IS_INFINITE(sPtr->mCreateSCN) == ID_TRUE )
        {
            /* BUG-14974: 무한 Loop발생.*/
            sCurPtr = sNxtPtr;
            continue;
        }
        sTableHeader = (smcTableHeader *)( sPtr + 1 );

        sTableType = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;

        // memory & meta table only
        if( sTableType != aTableType )
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        //lock을 잡았지만 table이 drop된 경우에는 skip;
        // BUG-30867 Discard 된 Tablespace에 속한 Table도 Skip되어야 함
        if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
           ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                         SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        sIndexCnt =  smcTable::getIndexCount(sTableHeader);

        if( sIndexCnt != 0  )
        {
            //DDL 을 방지.
            sState = 1;
            IDE_TEST( smLayerCallback::setImpSavepoint( sTrans, 
                                                        &sISavepoint,
                                                        sDummy )
                      != IDE_SUCCESS );

            /* BUG-48160 lock 잡힌 table 은 제외하고 출력하는 기능 추가.
             * Property : __SKIP_LOCKED_TABLE_AT_FIXED_TABLE */
            if ( smLayerCallback::lockTableModeIS4FixedTable( sTrans,
                                                              SMC_TABLE_LOCK( sTableHeader ) )
                 == IDE_SUCCESS )
            {
                sState = 2;
                //lock을 잡았지만 table이 drop된 경우에는 skip;
                // BUG-30867 Discard 된 Tablespace에 속한 Table도 Skip되어야 함
                if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
                   ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                                 SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
                {
                    sState = 1;
                    IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                    sISavepoint )
                              != IDE_SUCCESS );
                    sState = 0;
                    IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                                  sISavepoint )
                              != IDE_SUCCESS );
                    sCurPtr = sNxtPtr;
                    continue;
                }//if

                // lock을 대기하는 동안 index가 drop되었거나, 새로운 index가
                // 생성되었을 수 있으므로 정확한 index 수를 다시 구한다.
                // 뿐만 아니라, index cnt를 증가시킨 후 index를 생성하므로
                // index가 완료되지 못하면 index cnt가 감소하므로 다시 구해야 함.
                sIndexCnt = smcTable::getIndexCount(sTableHeader);

                for( i = 0; i < sIndexCnt; i++ )
                {
                    sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader, i );
                    if( sIndexCursor->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID )
                    {
                        continue;
                    }

                    sIndexHeader = (smnbHeader*)(sIndexCursor->mHeader);
                    if( sIndexHeader == NULL )
                    {
                        idlOS::memset( &sIndexHeader4PerfV, 0x00, ID_SIZEOF(smnbHeader4PerfV) );

                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;

                        sIndexHeader4PerfV.mIsConsistent = 'F';
                    }
                    else
                    {
                        idlOS::memset( &sIndexHeader4PerfV, 0x00, ID_SIZEOF(smnbHeader4PerfV) );
                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;

                        sIndexHeader4PerfV.mIndexTSID = sIndexHeader->mSpaceID;
                        sIndexHeader4PerfV.mTableTSID = sTableHeader->mSpaceID;
                        sIndexHeader4PerfV.mIsNotNull =
                            ( sIndexHeader->mIsNotNull == ID_TRUE ) ? 'T' : 'F';
                        sIndexHeader4PerfV.mIsUnique =
                            ( (sIndexCursor->mFlag & SMI_INDEX_UNIQUE_MASK) ==
                              SMI_INDEX_UNIQUE_ENABLE ) ? 'T' : 'F';
                        sIndexHeader4PerfV.mUsedNodeCount = sIndexHeader->nodeCount;

                        // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
                        sIndexHeader4PerfV.mPrepareNodeCount =
                            sIndexHeader->mNodePool.getFreeSlotCount();

                        // BUG-19249
                        sIndexHeader4PerfV.mBuiltType    = sIndexHeader->mBuiltType;
                        sIndexHeader4PerfV.mIsConsistent =
                            ( sIndexHeader->mIsConsistent == ID_TRUE ) ? 'T' : 'F';
                    }

                    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                          aMemory,
                                                          (void *)&sIndexHeader4PerfV )
                              != IDE_SUCCESS);
                }//for
                sState = 1;
                IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                sISavepoint )
                          != IDE_SUCCESS );
            }
            sState = 0;
            IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                          sISavepoint )
                      != IDE_SUCCESS );
        }// if 인덱스가 있으면
        sCurPtr = sNxtPtr;
    }// while

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( smLayerCallback::abortToImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smLayerCallback::unsetImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gMemBTreeHeaderColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(smnbHeader4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(smnbHeader4PerfV, mIndexID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_TBS_ID",
        offsetof(smnbHeader4PerfV, mIndexTSID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TABLE_TBS_ID",
        offsetof(smnbHeader4PerfV, mTableTSID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mTableTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_UNIQUE",
        offsetof(smnbHeader4PerfV, mIsUnique ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsUnique ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_NOT_NULL",
        offsetof(smnbHeader4PerfV, mIsNotNull ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsNotNull ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USED_NODE_COUNT",
        offsetof(smnbHeader4PerfV, mUsedNodeCount ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mUsedNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREPARE_NODE_COUNT",
        offsetof(smnbHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"BUILT_TYPE",
        offsetof(smnbHeader4PerfV, mBuiltType ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mBuiltType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(smnbHeader4PerfV, mIsConsistent ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsConsistent ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
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

iduFixedTableDesc  gMemBTreeHeaderDesc=
{
    (SChar *)"X$MEM_BTREE_HEADER",
    smnbFT::buildRecordForMemBTreeHeader,
    gMemBTreeHeaderColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};


static iduFixedTableColDesc  gVolBTreeHeaderColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(smnbHeader4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(smnbHeader4PerfV, mIndexID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_TBS_ID",
        offsetof(smnbHeader4PerfV, mIndexTSID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TABLE_TBS_ID",
        offsetof(smnbHeader4PerfV, mTableTSID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mTableTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_UNIQUE",
        offsetof(smnbHeader4PerfV, mIsUnique ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsUnique ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_NOT_NULL",
        offsetof(smnbHeader4PerfV, mIsNotNull ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsNotNull ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USED_NODE_COUNT",
        offsetof(smnbHeader4PerfV, mUsedNodeCount ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mUsedNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREPARE_NODE_COUNT",
        offsetof(smnbHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"BUILT_TYPE",
        offsetof(smnbHeader4PerfV, mBuiltType ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mBuiltType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(smnbHeader4PerfV, mIsConsistent ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsConsistent ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
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

iduFixedTableDesc  gVolBTreeHeaderDesc=
{
    (SChar *)"X$VOL_BTREE_HEADER",
    smnbFT::buildRecordForVolBTreeHeader,
    gVolBTreeHeaderColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

//======================================================================
//  X$MEM_BTREE_STAT
//  memory index의 run-time statistic information을 위한 peformance view
//======================================================================
IDE_RC smnbFT::buildRecordForMemBTreeStat(idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * /* aDumpObj */,
                                          iduFixedTableMemory * aMemory)
{
    IDE_TEST( buildRecordForBTreeStat( aHeader,
                                       aMemory,
                                       SMI_TABLE_META,
                                       SMC_CAT_TABLE  )
              != IDE_SUCCESS );

    IDE_TEST( buildRecordForBTreeStat( aHeader,
                                       aMemory,
                                       SMI_TABLE_MEMORY,
                                       SMC_CAT_TABLE  )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//======================================================================
//  X$VOL_BTREE_STAT
//  volatile index의 run-time statistic information을 위한 peformance view
//======================================================================
IDE_RC smnbFT::buildRecordForVolBTreeStat(idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * /* aDumpObj */,
                                          iduFixedTableMemory * aMemory)
{
    return buildRecordForBTreeStat( aHeader,
                                    aMemory,
                                    SMI_TABLE_VOLATILE,
                                    SMC_CAT_TABLE );
}

//======================================================================
//  X$TEMP_BTREE_STAT
//  temp index의 run-time statistic information을 위한 peformance view
//======================================================================
IDE_RC smnbFT::buildRecordForTempBTreeStat(idvSQL              * /*aStatistics*/,
                                           void                * aHeader,
                                           void                * /* aDumpObj */,
                                           iduFixedTableMemory * aMemory)
{
    return buildRecordForBTreeStat( aHeader,
                                    aMemory,
                                    SMI_TABLE_VOLATILE,
                                    SMC_CAT_TEMPTABLE );
}
IDE_RC smnbFT::buildRecordForBTreeStat(void                * aHeader,
                                       iduFixedTableMemory * aMemory,
                                       UInt                  aTableType,
                                       smcTableHeader      * aCatTblHdr)
{
    ;
    smcTableHeader   * sTableHeader;
    smpSlotHeader    * sPtr;
    SChar            * sCurPtr;
    SChar            * sNxtPtr;
    UInt               sTableType;
    smnIndexHeader   * sIndexCursor;
    smnbHeader       * sIndexHeader;
    smnbStat4PerfV     sIndexStat4PerfV;
    void             * sTrans;
    UInt               sIndexCnt;
    UInt               i;
    UChar              sValueBuffer[SM_DUMP_VALUE_BUFFER_SIZE];
    UInt               sValueLength;
    IDE_RC             sReturn;
    void             * sISavepoint = NULL;
    UInt               sDummy = 0;
    UInt               sState = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCurPtr = NULL;

    if ( aMemory->useExternalMemory() == ID_TRUE )
    {
        /* BUG-43006 FixedTable Indexing Filter */
        sTrans = ((smiFixedTableProperties *)aMemory->getContext())->mTrans;
    }
    else
    {
        /* BUG-32292 [sm-util] Self deadlock occur since fixed-table building
         * operation uses another transaction. 
         * NestedTransaction을 사용하면 Self-deadlock 우려가 있다.
         * 따라서 id Memory 영역으로부터 Iterator를 얻어 Transaction을 얻어낸다. */
        sTrans = ((smiIterator*)aMemory->getContext())->trans;
    }

    while(1)
    {
        IDE_TEST(smcRecord::nextOIDall(aCatTblHdr,sCurPtr,&sNxtPtr)
                  != IDE_SUCCESS);
        if( sNxtPtr == NULL )
        {
            break;
        }
        sPtr = (smpSlotHeader *)sNxtPtr;

        if( SM_SCN_IS_INFINITE(sPtr->mCreateSCN) == ID_TRUE )
        {
            /* BUG-14974: 무한 Loop발생.*/
            sCurPtr = sNxtPtr;
            continue;
        }
        sTableHeader = (smcTableHeader *)( sPtr + 1 );

        sTableType = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;

        // memory & meta & vol table only
        if( sTableType != aTableType )
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        //lock을 잡았지만 table이 drop된 경우에는 skip;
        // BUG-30867 Discard 된 Tablespace에 속한 Table도 Skip되어야 함
        if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
           ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                         SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        sIndexCnt =  smcTable::getIndexCount(sTableHeader);

        if( sIndexCnt != 0  )
        {
            //DDL 을 방지.
            sState = 1;
            IDE_TEST( smLayerCallback::setImpSavepoint( sTrans, 
                                                        &sISavepoint,
                                                        sDummy )
                      != IDE_SUCCESS );

            /* BUG-48160 lock 잡힌 table 은 제외하고 출력하는 기능 추가.
             * Property : __SKIP_LOCKED_TABLE_AT_FIXED_TABLE */
            if ( smLayerCallback::lockTableModeIS4FixedTable( sTrans,
                                                              SMC_TABLE_LOCK( sTableHeader ) )
                 == IDE_SUCCESS )
            {
                sState = 2;
                //lock을 잡았지만 table이 drop된 경우에는 skip;
                // BUG-30867 Discard 된 Tablespace에 속한 Table도 Skip되어야 함
                if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
                   ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                                 SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
                {
                    sState = 1;
                    IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                    sISavepoint )
                              != IDE_SUCCESS );
                    sState = 0;
                    IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                                  sISavepoint )
                              != IDE_SUCCESS );
                    sCurPtr = sNxtPtr;
                    continue;
                }//if

                // lock을 대기하는 동안 index가 drop되었거나, 새로운 index가
                // 생성되었을 수 있으므로 정확한 index 수를 다시 구한다.
                // 뿐만 아니라, index cnt를 증가시킨 후 index를 생성하므로
                // index가 완료되지 못하면 index cnt가 감소하므로 다시 구해야 함.
                sIndexCnt = smcTable::getIndexCount(sTableHeader);

                for( i = 0; i < sIndexCnt; i++ )
                {
                    sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader, i );

                    // BUG-30867 Table에 R Tree가 포함 되었을 경우 FATAL발생.
                    // B Tree만 처리하도록 수정합니다.
                    if( sIndexCursor->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID )
                    {
                        continue;
                    }

                    sIndexHeader = (smnbHeader*)(sIndexCursor->mHeader);
                    if( sIndexHeader == NULL )
                    {
                        continue;
                    }

                    idlOS::memset( &sIndexStat4PerfV, 0x00, ID_SIZEOF(smnbStat4PerfV) );

                    idlOS::memcpy( &sIndexStat4PerfV.mName,
                                   &sIndexCursor->mName,
                                   SMN_MAX_INDEX_NAME_SIZE+8);
                    sIndexStat4PerfV.mIndexID = sIndexCursor->mId;

                    sIndexStat4PerfV.mTreeLatchStat = 
                        *(sIndexHeader->mTreeMutex.getMutexStat());

                    sIndexStat4PerfV.mKeyCount = sIndexCursor->mStat.mKeyCount;
                    sIndexStat4PerfV.mStmtStat = sIndexHeader->mStmtStat;
                    sIndexStat4PerfV.mAgerStat = sIndexHeader->mAgerStat;

                    sIndexStat4PerfV.mNumDist  = sIndexCursor->mStat.mNumDist;

                    // BUG-18188 : MIN_VALUE
                    sValueLength = SM_DUMP_VALUE_BUFFER_SIZE;
                    IDE_TEST( sIndexHeader->columns[0].key2String(
                                  & sIndexHeader->columns[0].column,
                                  sIndexCursor->mStat.mMinValue,
                                  sIndexHeader->columns[0].column.size,
                                  (UChar*) SM_DUMP_VALUE_DATE_FMT,
                                  idlOS::strlen( SM_DUMP_VALUE_DATE_FMT ),
                                  sValueBuffer,
                                  &sValueLength,
                                  &sReturn ) != IDE_SUCCESS );

                    idlOS::memcpy( sIndexStat4PerfV.mMinValue,
                                   sValueBuffer,
                                   ( sValueLength > SM_DUMP_VALUE_LENGTH ) ?
                                   SM_DUMP_VALUE_LENGTH : sValueLength );

                    // BUG-18188 : MAX_VALUE
                    sValueLength = SM_DUMP_VALUE_BUFFER_SIZE;
                    IDE_TEST( sIndexHeader->columns[0].key2String(
                                  & sIndexHeader->columns[0].column,
                                  sIndexCursor->mStat.mMaxValue,
                                  sIndexHeader->columns[0].column.size,
                                  (UChar*) SM_DUMP_VALUE_DATE_FMT,
                                  idlOS::strlen( SM_DUMP_VALUE_DATE_FMT ),
                                  sValueBuffer,
                                  &sValueLength,
                                  &sReturn ) != IDE_SUCCESS );

                    idlOS::memcpy( sIndexStat4PerfV.mMaxValue,
                                   sValueBuffer,
                                   ( sValueLength > SM_DUMP_VALUE_LENGTH ) ?
                                   SM_DUMP_VALUE_LENGTH : sValueLength );

                    /* PROJ-2433 */
                    sIndexStat4PerfV.mDirectKeySize     = sIndexHeader->mKeySize;
                    sIndexStat4PerfV.mINodeMaxSlotCount = sIndexHeader->mINodeMaxSlotCount;
                    sIndexStat4PerfV.mLNodeMaxSlotCount = sIndexHeader->mLNodeMaxSlotCount;

                    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                          aMemory,
                                                          (void *)&sIndexStat4PerfV )
                              != IDE_SUCCESS);
                }//for
                sState = 1;
                IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                            sISavepoint )
                          != IDE_SUCCESS );
            }
            sState = 0;
            IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                          sISavepoint )
                      != IDE_SUCCESS );
        }// if 인덱스가 있으면
        sCurPtr = sNxtPtr;
    }// while

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( smLayerCallback::abortToImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smLayerCallback::unsetImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gMemBTreeStatColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(smnbStat4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(smnbStat4PerfV, mIndexID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_TRY_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mTryCount),
        IDU_FT_SIZEOF(iduMutexStat, mTryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_LOCK_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mLockCount),
        IDU_FT_SIZEOF(iduMutexStat, mLockCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_MISS_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mMissCount),
        IDU_FT_SIZEOF(iduMutexStat, mMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"KEY_COUNT",
        offsetof(smnbStat4PerfV, mKeyCount),
        IDU_FT_SIZEOF(smnbStat4PerfV, mKeyCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NUMDIST",
        offsetof(smnbStat4PerfV, mNumDist ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mNumDist ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_VALUE",
        offsetof(smnbStat4PerfV, mMinValue ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mMinValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_VALUE",
        offsetof(smnbStat4PerfV, mMaxValue ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mMaxValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {   /* PROJ-2433 */
        (SChar *)"DIRECTKEY_SIZE",
        offsetof( smnbStat4PerfV, mDirectKeySize ),
        IDU_FT_SIZEOF( smnbStat4PerfV, mDirectKeySize ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {   /* PROJ-2433 */
        (SChar *)"INTERNAL_NODE_MAX_SLOT_CNT",
        offsetof( smnbStat4PerfV, mINodeMaxSlotCount ),
        IDU_FT_SIZEOF( smnbStat4PerfV, mINodeMaxSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0, NULL
    },
    {   /* PROJ-2433 */
        (SChar *)"LEAF_NODE_MAX_SLOT_CNT",
        offsetof( smnbStat4PerfV, mLNodeMaxSlotCount ),
        IDU_FT_SIZEOF( smnbStat4PerfV, mLNodeMaxSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0, NULL
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

iduFixedTableDesc  gMemBTreeStatDesc=
{
    (SChar *)"X$MEM_BTREE_STAT",
    smnbFT::buildRecordForMemBTreeStat,
    gMemBTreeStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

// BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
//======================================================================
//  X$MEM_BTREE_NODEPOOL
//  memory index의 node pool을 보여주는 peformance view
//======================================================================

IDE_RC smnbFT::buildRecordForMemBTreeNodePool(idvSQL              * /*aStatistics*/,
                                              void                * aHeader,
                                              void                * /* aDumpObj */,
                                              iduFixedTableMemory * aMemory)
{
    smnbNodePool4PerfV  sIndexNodePool4PerfV;
    smnFreeNodeList    *sFreeNodeList = (smnFreeNodeList*)smnbBTree::mSmnbFreeNodeList;

    UInt sSlotPerPage    = smnbBTree::mSmnbNodePool.getSlotPerPage();
    UInt sAllocSlotCount = smnbBTree::mSmnbNodePool.getAllocSlotCount();

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    /* BUG-46402 : 페이지 개념이 없어졌으나, 호환성을 위해서 계산함 */
    sIndexNodePool4PerfV.mTotalPageCount = ( sSlotPerPage == 0 ) ? 0 
                                           : ( ( sAllocSlotCount + sSlotPerPage - 1 ) / sSlotPerPage );

    sIndexNodePool4PerfV.mTotalNodeCount = sAllocSlotCount;
    sIndexNodePool4PerfV.mFreeNodeCount  = smnbBTree::mSmnbNodePool.getFreeSlotCount();
    sIndexNodePool4PerfV.mUsedNodeCount  =
        sIndexNodePool4PerfV.mTotalNodeCount - sIndexNodePool4PerfV.mFreeNodeCount;
    sIndexNodePool4PerfV.mNodeSize       = smnbBTree::mSmnbNodePool.getSlotSize();
    sIndexNodePool4PerfV.mTotalAllocReq  = smnbBTree::mSmnbNodePool.getTotalAllocReq();
    sIndexNodePool4PerfV.mTotalFreeReq   = smnbBTree::mSmnbNodePool.getTotalFreeReq();

    // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
    sIndexNodePool4PerfV.mFreeReqCount   =
        sFreeNodeList->mAddCnt - sFreeNodeList->mHandledCnt;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sIndexNodePool4PerfV )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gMemBTreeNodePoolColDesc[]=
{
    {
        (SChar*)"TOTAL_PAGE_COUNT",
        offsetof(smnbNodePool4PerfV, mTotalPageCount ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mTotalPageCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_NODE_COUNT",
        offsetof(smnbNodePool4PerfV, mTotalNodeCount ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mTotalNodeCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_NODE_COUNT",
        offsetof(smnbNodePool4PerfV, mFreeNodeCount ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mFreeNodeCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USED_NODE_COUNT",
        offsetof(smnbNodePool4PerfV, mUsedNodeCount ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mUsedNodeCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NODE_SIZE",
        offsetof(smnbNodePool4PerfV, mNodeSize ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mNodeSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_ALLOC_REQ",
        offsetof(smnbNodePool4PerfV, mTotalAllocReq ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mTotalAllocReq),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_FREE_REQ",
        offsetof(smnbNodePool4PerfV, mTotalFreeReq ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mTotalFreeReq),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_REQ_COUNT",
        offsetof(smnbNodePool4PerfV, mFreeReqCount ),
        IDU_FT_SIZEOF(smnbNodePool4PerfV, mFreeReqCount),
        IDU_FT_TYPE_UINTEGER,
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

iduFixedTableDesc  gMemBTreeNodePoolDesc=
{
    (SChar *)"X$MEM_BTREE_NODEPOOL",
    smnbFT::buildRecordForMemBTreeNodePool,
    gMemBTreeNodePoolColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc  gTempBTreeHeaderColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(smnbHeader4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(smnbHeader4PerfV, mIndexID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_TBS_ID",
        offsetof(smnbHeader4PerfV, mIndexTSID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TABLE_TBS_ID",
        offsetof(smnbHeader4PerfV, mTableTSID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mTableTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_UNIQUE",
        offsetof(smnbHeader4PerfV, mIsUnique ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsUnique ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_NOT_NULL",
        offsetof(smnbHeader4PerfV, mIsNotNull ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsNotNull ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USED_NODE_COUNT",
        offsetof(smnbHeader4PerfV, mUsedNodeCount ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mUsedNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREPARE_NODE_COUNT",
        offsetof(smnbHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"BUILT_TYPE",
        offsetof(smnbHeader4PerfV, mBuiltType ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mBuiltType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(smnbHeader4PerfV, mIsConsistent ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIsConsistent ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
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

iduFixedTableDesc  gTempBTreeHeaderDesc=
{
    (SChar *)"X$TEMP_BTREE_HEADER",
    smnbFT::buildRecordForTempBTreeHeader,
    gTempBTreeHeaderColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

static iduFixedTableColDesc  gTempBTreeStatColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(smnbStat4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(smnbStat4PerfV, mIndexID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_TRY_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mTryCount),
        IDU_FT_SIZEOF(iduMutexStat, mTryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_LOCK_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mLockCount),
        IDU_FT_SIZEOF(iduMutexStat, mLockCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_MISS_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mMissCount),
        IDU_FT_SIZEOF(iduMutexStat, mMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"KEY_COUNT",
        offsetof(smnbStat4PerfV, mKeyCount),
        IDU_FT_SIZEOF(smnbStat4PerfV, mKeyCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NUMDIST",
        offsetof(smnbStat4PerfV, mNumDist ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mNumDist ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_VALUE",
        offsetof(smnbStat4PerfV, mMinValue ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mMinValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_VALUE",
        offsetof(smnbStat4PerfV, mMaxValue ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mMaxValue ),
        IDU_FT_TYPE_CHAR,
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

iduFixedTableDesc  gTempBTreeStatDesc=
{
    (SChar *)"X$TEMP_BTREE_STAT",
    smnbFT::buildRecordForTempBTreeStat,
    gTempBTreeStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

static iduFixedTableColDesc  gVolBTreeStatColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(smnbStat4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(smnbStat4PerfV, mIndexID ),
        IDU_FT_SIZEOF(smnbHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_TRY_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mTryCount),
        IDU_FT_SIZEOF(iduMutexStat, mTryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_LOCK_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mLockCount),
        IDU_FT_SIZEOF(iduMutexStat, mLockCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_MISS_COUNT",
        offsetof(smnbStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mMissCount),
        IDU_FT_SIZEOF(iduMutexStat, mMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"KEY_COUNT",
        offsetof(smnbStat4PerfV, mKeyCount),
        IDU_FT_SIZEOF(smnbStat4PerfV, mKeyCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_STMT",
        offsetof(smnbStat4PerfV, mStmtStat) + offsetof(smnbStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(smnbStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_AGER",
        offsetof(smnbStat4PerfV, mAgerStat) + offsetof(smnbStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(smnbStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NUMDIST",
        offsetof(smnbStat4PerfV, mNumDist ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mNumDist ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_VALUE",
        offsetof(smnbStat4PerfV, mMinValue ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mMinValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_VALUE",
        offsetof(smnbStat4PerfV, mMaxValue ),
        IDU_FT_SIZEOF(smnbStat4PerfV, mMaxValue ),
        IDU_FT_TYPE_CHAR,
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

iduFixedTableDesc  gVolBTreeStatDesc=
{
    (SChar *)"X$VOL_BTREE_STAT",
    smnbFT::buildRecordForVolBTreeStat,
    gVolBTreeStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};


