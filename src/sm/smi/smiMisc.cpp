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
 * Copyright 1999-2001, ATIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: smiMisc.cpp 91224 2021-07-14 05:36:12Z minku.kang $
 **********************************************************************/

#include <smErrorCode.h>

#include <smu.h>
#include <smm.h>
#include <svm.h>
#include <sdd.h>
#include <smp.h>
#include <sdp.h>
#include <smc.h>
#include <sdc.h>
#include <sml.h>
#include <smx.h>
#include <sma.h>
#include <sds.h>
#include <smi.h>

#include <sctTableSpaceMgr.h>
#include <sgmManager.h>
#include <scpfModule.h>
#include <sdpscFT.h>

extern smiGlobalCallBackList gSmiGlobalCallBackList;

/* PROJ-1915 */
extern const UInt            smVersionID;

// BUG-14113
// isolation level에 따른 table lock mode
static const smiTableLockMode smiTableLockModeOnIsolationLevel[3][6] =
{
    {
        // SMI_ISOLATION_CONSISTENT (==0x00000000)
        SMI_TABLE_NLOCK,    // SMI_TABLE_NLOCK
        SMI_TABLE_LOCK_S,   // SMI_TABLE_LOCK_S
        SMI_TABLE_LOCK_X,   // SMI_TABLE_LOCK_X
        SMI_TABLE_LOCK_IS,  // SMI_TABLE_LOCK_IS
        SMI_TABLE_LOCK_IX,  // SMI_TABLE_LOCK_IX
        SMI_TABLE_LOCK_SIX  // SMI_TABLE_LOCK_SIX
    },
    {
        // SMI_ISOLATION_REPEATABLE (==0x00000001)
        SMI_TABLE_NLOCK,    // SMI_TABLE_NLOCK
        SMI_TABLE_LOCK_S,   // SMI_TABLE_LOCK_S
        SMI_TABLE_LOCK_X,   // SMI_TABLE_LOCK_X
        SMI_TABLE_LOCK_IX,  // SMI_TABLE_LOCK_IS
        SMI_TABLE_LOCK_IX,  // SMI_TABLE_LOCK_IX
        SMI_TABLE_LOCK_SIX  // SMI_TABLE_LOCK_SIX
    },
    {
        // SMI_ISOLATION_NO_PHANTOM (0x00000002)
        SMI_TABLE_NLOCK,    // SMI_TABLE_NOLOCK
        SMI_TABLE_LOCK_S,   // SMI_TABLE_LOCK_S
        SMI_TABLE_LOCK_X,   // SMI_TABLE_LOCK_X
        SMI_TABLE_LOCK_S,   // SMI_TABLE_LOCK_IS
        SMI_TABLE_LOCK_X,   // SMI_TABLE_LOCK_IX
        SMI_TABLE_LOCK_SIX  // SMI_TABLE_LOCK_SIX
    }
};


// For A4 : Table Space Type에 대한 인자 추가
UInt smiGetPageSize( smiTableSpaceType aTSType )
{

    if ( (aTSType == SMI_MEMORY_SYSTEM_DICTIONARY) ||
         (aTSType == SMI_MEMORY_SYSTEM_DATA) ||
         (aTSType == SMI_MEMORY_USER_DATA) )
    {
        return (UInt)SM_PAGE_SIZE;
    }
    else
    {
        return (UInt)SD_PAGE_SIZE;
    }

}

/* 현재 디스크 DB의 총 크기를 구한다. */
ULong smiGetDiskDBFullSize()
{
    ULong               sDiskDBFullSize = 0; 
    sctTableSpaceNode*  sCurrSpaceNode;
    sctTableSpaceNode*  sNextSpaceNode;
    smiTableSpaceAttr   sTableSpaceAttr;
    sddDataFileNode*    sFileNode;
    smiDataFileAttr     sDataFileAttr;
    UInt                i = 0; 

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurrSpaceNode != NULL )
    {    
       sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );

        /* Disk tablespace의 데이터의 크기만 구하는 것이 목표이므로
         * undo tablespace는 목표에서 제외하도록 한다. */
        if ( ( sCurrSpaceNode->mType == SMI_DISK_SYSTEM_DATA ) || 
             ( sCurrSpaceNode->mType == SMI_DISK_USER_DATA ) )
        {    
            if ( SMI_TBS_IS_DROPPED(sCurrSpaceNode->mState) )
            {    
                sCurrSpaceNode = sNextSpaceNode;
                continue;
            }    

            sddTableSpace::getTableSpaceAttr( (sddTableSpaceNode*)sCurrSpaceNode,
                                              &sTableSpaceAttr );
            
            for( i = 0 ; i < ((sddTableSpaceNode*)sCurrSpaceNode)->mNewFileID ; i++ )
            {   
                sFileNode = ((sddTableSpaceNode*)sCurrSpaceNode)->mFileNodeArr[i];
                
                if ( sFileNode == NULL )
                {   
                    continue;
                }
                
                if ( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
                {   
                    continue;
                }
                
                sddDataFile::getDataFileAttr(sFileNode, &sDataFileAttr);

                if ( sDataFileAttr.mMaxSize > 0 )
                {
                    sDiskDBFullSize += sDataFileAttr.mMaxSize;
                }
                else
                {
                    /* auto extend가 꺼져 있을 경우 mMaxSize가 0 일 경우가 있다.
                     * 이 경우에는 현재 크기를 MaxSize 대용으로 사용하도록 한다. */
                    sDiskDBFullSize += sDataFileAttr.mCurrSize;
                }        
            }
        }
        sCurrSpaceNode = sNextSpaceNode;
    }
    return sDiskDBFullSize;
}

UInt smiGetBufferPoolSize( )
{
    return sdbBufferMgr::getPageCount();
}

SChar * smiGetDBName( )
{
    return smmDatabase::getDBName();
}

// PROJ-1579 NCHAR
SChar * smiGetDBCharSet( )
{
    return smmDatabase::getDBCharSet();
}

// PROJ-1579 NCHAR
SChar * smiGetNationalCharSet( )
{
    return smmDatabase::getNationalCharSet();
}
    
const void* smiGetCatalogTable( void )
{
    return (void*)( (UChar*)smmManager::m_catTableHeader - SMP_SLOT_HEADER_SIZE );
}

// For A4 : Table Type에 대한 인자 추가
UInt smiGetVariableColumnSize( UInt aTableType )
{
    UInt sTableType = aTableType & SMI_TABLE_TYPE_MASK;

    IDE_ASSERT( ( sTableType == SMI_TABLE_MEMORY ) ||
                ( sTableType == SMI_TABLE_VOLATILE ) );

    return idlOS::align8(ID_SIZEOF(smVCDesc));
}

UInt smiGetVariableColumnSize4DiskIndex()
{
    return idlOS::align8(ID_SIZEOF(sdcVarColHdr));
}

UInt smiGetVCDescInModeSize()
{
    return idlOS::align8(ID_SIZEOF(smVCDescInMode));
}


// For A4 : Table Type에 대한 인자 추가
UInt smiGetRowHeaderSize( UInt aTableType )
{

    UInt sTableType = aTableType & SMI_TABLE_TYPE_MASK;

    if ( ( sTableType == SMI_TABLE_MEMORY   ) ||
         ( sTableType == SMI_TABLE_VOLATILE ) )
    {
        return SMP_SLOT_HEADER_SIZE;
    }
    else
    {
        IDE_ASSERT(0);
    }
}

// For A4 : Table Type에 대한 인자 추가 안함. table handle에 대해서만 사용됨
//          !! 절대 Disk Row에 대해 사용되어서는 안됨 !!
smSCN smiGetRowSCN( const void * aRow )
{
    smSCN sSCN;
    smTID sDummyTID;

    SMX_GET_SCN_AND_TID( ((smpSlotHeader*)aRow)->mCreateSCN, sSCN, sDummyTID );

    return sSCN;
}

// For A4 : Index Module 저장 구조 변경
IDE_RC smiFindIndexType( SChar * aIndexName,
                         UInt *  aIndexType )
{

    UInt i;

    for( i = 0 ; i < SMI_INDEXTYPE_COUNT ; i++)
    {
        if ( gSmnAllIndex[i] != NULL )
        {
            if ( idlOS::strcmp( gSmnAllIndex[i]->mName, aIndexName ) == 0 )
            {
                *aIndexType = gSmnAllIndex[i]->mTypeID;
                break;
            }
        }
        else
        {
            // Nothing To Do
        }
    }

    IDE_TEST( i == SMI_INDEXTYPE_COUNT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( smERR_ABORT_smnNotFoundByIndexName ) );

    return IDE_FAILURE;

}

// BUG-17449
idBool smiCanMakeIndex( UInt    aTableType,
                        UInt    aIndexType )
{
    void   * sIndexModule;
    idBool   sResult;

    sIndexModule = smnManager::getIndexModule(aTableType, aIndexType);

    if ( sIndexModule == NULL )
    {
        sResult = ID_FALSE;
    }
    else
    {
        sResult = ID_TRUE;
    }

    return sResult;
}

const SChar* smiGetIndexName( UInt aIndexType )
{

    IDE_TEST( aIndexType >= SMI_INDEXTYPE_COUNT );

    IDE_TEST( gSmnAllIndex[aIndexType] == NULL );

    return gSmnAllIndex[aIndexType]->mName;

    IDE_EXCEPTION_END;

    return NULL;

}

idBool smiGetIndexUnique( const void * aIndex )
{

    idBool sResult;

    if ( ( (((const smnIndexHeader*)aIndex)->mFlag & SMI_INDEX_UNIQUE_MASK)
           == SMI_INDEX_UNIQUE_ENABLE ) ||
         ( (((const smnIndexHeader*)aIndex)->mFlag & SMI_INDEX_LOCAL_UNIQUE_MASK)
           == SMI_INDEX_LOCAL_UNIQUE_ENABLE ) )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

// For A4 : Table Type에 대한 인자 추가
UInt smiGetDefaultIndexType( void )
{

    return (UInt) SMI_BUILTIN_B_TREE_INDEXTYPE_ID; // 1 : BTREE

}

idBool smiCanUseUniqueIndex( UInt aIndexType )
{
    idBool sResult;

    if ( gSmnAllIndex[aIndexType] != NULL )
    {
        if ( ( gSmnAllIndex[aIndexType]->mFlag & SMN_INDEX_UNIQUE_MASK )
             == SMN_INDEX_UNIQUE_ENABLE )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

idBool smiCanUseCompositeIndex( UInt aIndexType )
{
    idBool sResult;

    if ( gSmnAllIndex[aIndexType] != NULL )
    {
        if ( (gSmnAllIndex[aIndexType]->mFlag & SMN_INDEX_COMPOSITE_MASK)
             == SMN_INDEX_COMPOSITE_ENABLE )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

// PROJ-1502 PARTITIONED DISK TABLE
// 대소 비교 가능한 인덱스 타입인지 체크한다.
idBool smiGreaterLessValidIndexType( UInt aIndexType )
{
    idBool sResult;

    if ( gSmnAllIndex[aIndexType] != NULL )
    {
        if ( (gSmnAllIndex[aIndexType]->mFlag & SMN_INDEX_NUMERIC_COMPARE_MASK)
             == SMN_INDEX_NUMERIC_COMPARE_ENABLE )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

// PROJ-1704 MVCC Renewal
// AGING 가능한 인덱스 타입인지 검사한다.
idBool smiIsAgableIndex( const void * aIndex )
{
    idBool sResult;
    UInt   sIndexType;

    sIndexType = smiGetIndexType( aIndex );
    
    if ( gSmnAllIndex[sIndexType] != NULL )
    {
        if ( (gSmnAllIndex[sIndexType]->mFlag & SMN_INDEX_AGING_MASK)
             == SMN_INDEX_AGING_ENABLE )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

const void * smiGetTable( smOID aTableOID )
{
    void * sTableHeader;

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aTableOID,
                                       (void**)&sTableHeader )
                == IDE_SUCCESS );

    return sTableHeader;
}

smOID smiGetTableId( const void * aTable )
{
    return SMI_MISC_TABLE_HEADER(aTable)->mSelfOID;
}


UInt smiGetTableColumnCount( const void * aTable )
{

    return SMI_MISC_TABLE_HEADER(aTable)->mColumnCount;

}

UInt smiGetTableColumnSize( const void * aTable )
{

    return SMI_MISC_TABLE_HEADER(aTable)->mColumnSize;

}

// BUG-28321 drop tablespace 구문 수행 시 비정상 종료가 발생합니다.
// qp의 Meta와 sm 간의 index의 순서가 서로 다를 수 있습니다.
// 처음 Meta생성시 이 외에는 본 인터페이스를 사용하면 안됩니다.
const void * smiGetTableIndexes( const void * aTable,
                                 const UInt   aIdx )
{
    return smcTable::getTableIndex( (void*)SMI_MISC_TABLE_HEADER(aTable),aIdx );
}

const void * smiGetTableIndexByID( const void * aTable,
                                   const UInt   aIndexId )
{
    // BUG-28321 drop tablespace 구문 수행 시 비정상 종료가 발생합니다.
    // Index handle 을 반환할 때 Index의 ID를 기준으로 반환 하도록 수정
    return smcTable::getTableIndexByID( (void*)SMI_MISC_TABLE_HEADER(aTable),
                                        aIndexId );
}

// Primary Key Index의 Handle을 가져오는 인터페이스 함수
const void* smiGetTablePrimaryKeyIndex( const void * aTable )
{
    return smcTable::getTableIndex( (void*)SMI_MISC_TABLE_HEADER(aTable),
                                    0 ); // 0번째 Index가 Primary Index이다.
}

IDE_RC smiGetTableColumns( const void        * aTable,
                           const UInt          aIndex,
                           const smiColumn  ** aColumn )
{
    *aColumn = (smiColumn *)smcTable::getColumn(SMI_MISC_TABLE_HEADER(aTable), aIndex);

    return IDE_SUCCESS;
}

idBool smiIsConsistentIndices( const void * aTable )
{
    UInt                 i                = 0;
    UInt                 sIndexCount      = 0;
    idBool               sIsConsistentIdx = ID_TRUE;
    smnIndexHeader     * sIndexCursor     = NULL;

    sIndexCount = smcTable::getIndexCount( (void*)SMI_MISC_TABLE_HEADER( aTable ) );

    for ( i = 0 ; i < sIndexCount ; i++ )
    {
        sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( 
                                               (void*)SMI_MISC_TABLE_HEADER( aTable ), i );
        if ( smnManager::getIsConsistentOfIndexHeader( (void*)sIndexCursor ) == ID_FALSE )
        {
            sIsConsistentIdx = ID_FALSE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
    return sIsConsistentIdx;
}

UInt smiGetTableIndexCount( const void * aTable )
{
    return smcTable::getIndexCount( (void*)SMI_MISC_TABLE_HEADER(aTable) );
}

const void* smiGetTableInfo( const void * aTable )
{
    const smVCDesc* sColumnVCDesc;
    const smVCPieceHeader *sVCPieceHeader;
    const void* sInfoPtr = NULL;

    sColumnVCDesc = &(SMI_MISC_TABLE_HEADER(aTable)->mInfo);

    if ( sColumnVCDesc->length != 0 )
    {
        IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           sColumnVCDesc->fstPieceOID,
                                           (void**)&sVCPieceHeader )
                    == IDE_SUCCESS );

        sInfoPtr = (void*)(sVCPieceHeader + 1);
    }
    else
    {
        /* nothing to do */
    }

    return sInfoPtr;
}

IDE_RC smiGetTableTempInfo( const void    * aTable,
                            void         ** aRuntimeInfo )
{
    *aRuntimeInfo = SMI_MISC_TABLE_HEADER(aTable)->mRuntimeInfo;
    return IDE_SUCCESS;
}

void smiSetTableTempInfo( const void  * aTable,
                          void        * aTempInfo )
{
    ((smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable))->mRuntimeInfo = aTempInfo;
}

void smiSetTableReplacementLock( const void * aDstTable,
                                 const void * aSrcTable )
{
    ((smcTableHeader *)SMI_MISC_TABLE_HEADER(aDstTable))->mReplacementLock =
    ((smcTableHeader *)SMI_MISC_TABLE_HEADER(aSrcTable))->mLock;
}

void smiInitTableReplacementLock( const void * aTable )
{
    ((smcTableHeader *)SMI_MISC_TABLE_HEADER(aTable))->mReplacementLock = NULL;
}

IDE_RC smiGetTableNullRow( const void * aTable,
                           void      ** aRow,
                           scGRID     * aRowGRID )
{
    UInt            sTableType;
    smcTableHeader *sTableHeader;

    sTableHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);
    sTableType   = SMI_GET_TABLE_TYPE( sTableHeader );

    if ( (sTableType == SMI_TABLE_MEMORY) ||
         (sTableType == SMI_TABLE_META) ||
         (sTableType == SMI_TABLE_VOLATILE) )
    {
        if ( sTableHeader->mNullOID != SM_NULL_OID )
        {
            IDE_ERROR( smmManager::getOIDPtr( sTableHeader->mSpaceID,
                                              sTableHeader->mNullOID, 
                                              aRow )
                        == IDE_SUCCESS );

            SC_MAKE_GRID( *aRowGRID,
                          smcTable::getTableSpaceID(sTableHeader),
                          SM_MAKE_PID(sTableHeader->mNullOID),
                          SM_MAKE_OFFSET(sTableHeader->mNullOID) );
        }
        else
        {
            SC_MAKE_NULL_GRID( *aRowGRID );
        }
    }
    else if ( sTableType == SMI_TABLE_FIXED )
    {
        /* ------------------------------------------------
         * Fixed Table의 경우 Null Row를 smiFixedTableHeader의 mNullRow에 저장한다.
         * BUG-11268
         * ----------------------------------------------*/

        IDE_ASSERT( sTableType == SMI_TABLE_FIXED );

        *aRow = ((smiFixedTableHeader *)aTable)->mNullRow;
        SC_MAKE_NULL_GRID( *aRowGRID );

        IDE_DASSERT( *aRow != NULL );
    }
    else 
    {
        // sTableType == SMI_TABLE_DISK
        IDE_ASSERT ( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt smiGetTableFlag(const void * aTable)
{

    return ((smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable))->mFlag;
}

// PROJ-1665
UInt smiGetTableLoggingMode(const void * aTable)
{
    UInt sLoggingMode;

    sLoggingMode = (((smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable))->mFlag) & SMI_TABLE_LOGGING_MASK;

    return sLoggingMode;
}

UInt smiGetTableParallelDegree(const void * aTable)
{
    UInt sParallelDegree;

    sParallelDegree = ((smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable))->mParallelDegree;

    return sParallelDegree;
}

// FOR A4 : table hadle을 보고 DISK 테이블인지를 반환함.
idBool smiIsDiskTable(const void * aTable)
{
    smcTableHeader * sHeader;
     
    sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    return ( SMI_TABLE_TYPE_IS_DISK( sHeader ) == ID_TRUE ) ? ID_TRUE : ID_FALSE;
}

// FOR A4 : table hadle을 보고 Memory 테이블인지를 반환함.
idBool smiIsMemTable(const void * aTable)
{
    smcTableHeader * sHeader;
    
    sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    return ( (SMI_TABLE_TYPE_IS_MEMORY( sHeader ) == ID_TRUE) ||
             (SMI_TABLE_TYPE_IS_META( sHeader )   == ID_TRUE) )
           ? ID_TRUE : ID_FALSE;
}

idBool smiIsUserMemTable( const void * aTable )
{
    smcTableHeader * sHeader;

    sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    return ( ( SMI_TABLE_TYPE_IS_MEMORY( sHeader ) == ID_TRUE ) ? ID_TRUE : ID_FALSE ); 
}

// table hadle을 보고 Volatile 테이블인지를 반환함.
idBool smiIsVolTable(const void * aTable)
{
    smcTableHeader * sHeader;

    sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    return ( SMI_TABLE_TYPE_IS_VOLATILE( sHeader ) == ID_TRUE ) ? ID_TRUE : ID_FALSE;

}
IDE_RC smiGetTableBlockCount(const void * aTable, ULong * aBlockCnt )
{
    smcTableHeader *sTblHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    IDE_TEST( smcTable::getTablePageCount(sTblHeader, aBlockCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
IDE_RC smiGetTableExtentCount(const void * aTable, UInt * aBlockCnt );
{
    return IDE_SUCCESS;
}
*/

UInt smiGetIndexId( const void * aIndex )
{
    return ((const smnIndexHeader*)aIndex)->mId;
}

UInt smiGetIndexType( const void * aIndex )
{
    if ( aIndex == NULL )
    {
        return SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID;
    }
    else
    {
        return ((const smnIndexHeader*)aIndex)->mType;
    }
}

idBool smiGetIndexRange( const void * aIndex )
{
    if ( ((const smnIndexHeader*)aIndex)->mModule != NULL )
    {
        return (((const smnIndexHeader*)aIndex)->mModule->mFlag&SMN_RANGE_MASK)
               == SMN_RANGE_ENABLE ? ID_TRUE : ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    return ID_FALSE;
}

idBool smiGetIndexDimension( const void * aIndex )
{
    if ( ((const smnIndexHeader*)aIndex)->mModule != NULL )
    {
        return (((const smnIndexHeader*)aIndex)->mModule->mFlag
                &SMN_DIMENSION_MASK) == SMN_DIMENSION_ENABLE ?
          ID_TRUE : ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    return ID_FALSE;
}

const UInt* smiGetIndexColumns( const void * aIndex )
{
    return ((const smnIndexHeader*)aIndex)->mColumns;
}

const UInt* smiGetIndexColumnFlags( const void * aIndex )
{
    return ((const smnIndexHeader*)aIndex)->mColumnFlags;
}

UInt smiGetIndexColumnCount( const void * aIndex )
{
    return ((const smnIndexHeader*)aIndex)->mColumnCount;
}

idBool smiGetIndexBuiltIn( const void * aIndex )
{
    UInt sType;
    sType = smiGetIndexType( aIndex );

    IDE_DASSERT( gSmnAllIndex[sType] != NULL );
    return ( ( gSmnAllIndex[sType]->mBuiltIn == SMN_BUILTIN_INDEX ) ?
             ID_TRUE : ID_FALSE );
}

UInt smiEstimateMaxKeySize( UInt        aColumnCount,
                            smiColumn * aColumns,
                            UInt *      aMaxLengths )
{
    SInt          i;
    UInt          sFixed    = 0;
    UInt          sVariable = 0;
    UInt          sColSize;
    smiColumn *   sColumn;

    sColumn = aColumns;

    for( i = 1 ; i < (SInt)aColumnCount ; i++ )
    {
        if ( aColumns[i].offset > sColumn->offset )
        {
            sColumn = & aColumns[i];
        }
        else
        {
            // Nothing to do
        }
    }

    if ( SMI_IS_VARIABLE_COLUMN(sColumn->flag) || 
         SMI_IS_VARIABLE_LARGE_COLUMN(sColumn->flag) )
    {
        sColSize = idlOS::align8(ID_SIZEOF(sdcVarColHdr));
    }
    else
    {
        sColSize = sColumn->size;
    }

    sFixed = idlOS::align8( sColumn->offset +
                            sColSize );

    for(i = 0; i < (SInt)aColumnCount; i++)
    {

        if ( SMI_IS_VARIABLE_COLUMN(aColumns[i].flag) || 
             SMI_IS_VARIABLE_LARGE_COLUMN(aColumns[i].flag) )
        {
            sVariable += idlOS::align8(aMaxLengths[i]);
        }
        else
        {
            // Nothing to do
        }
    }

    return sFixed + idlOS::align8(sVariable);
}


UInt smiGetIndexKeySizeLimit( UInt        aTableType,
                              UInt        aIndexType )
{
    smnIndexModule * sIdxModule;

    sIdxModule = (smnIndexModule*)
        smnManager::getIndexModule(aTableType,
                                   aIndexType);
    
    return sIdxModule->mMaxKeySize;
}




// For A4 : Variable Column Data retrieval
/***********************************************************************
 * FUNCTION DESCRIPTION : smiVarAccess::smiGetVarColumn()              *
 * ------------------------------------------------------------------- *
 * 본 함수는 fix record 영역에 저장되어 있는 variable column header를  *
 * 이용하여 실제 column의 값을 가져오는 함수이다.                      *
 * 메모리 테이블의 경우에는 데이터가 하나의 메모리 덩어리에 저장되어   *
 * 있으므로 단순히 OID를 포인터로 변환하여 반환하면 된다.              *
 * 디스크 테이블의 경우에는 데이터가 디스크에 있으므로 메모리(버퍼)에  *
 * 올려야하며 또한 한 페이지를 넘는 데이터는 여러 덩어리로 떨어져      *
 * 존재하므로 이를 하나로 합치는 부분이 필요하다.                      *
 * 디스크 테이블의 가변 컬럼에 대한 접근은 크게 네 가지로 나뉠 수      *
 * 있는데,                                                             *
 *    1. QP에서 미리 저장된 fix record의 variable header의 RID를       *
 *       이용하여 가변 컬럼에 접근하는 것.                             *
 *    2. QP가 내려준 FIlter를 이용하여 데이터의 가변 컬럼에 접근하는   *
 *       것.                                                           *
 *    3. QP가 내려준 Key Range를 이용하여 index node에 있는 가변       *
 *       컬럼에 접근하는 것                                            *
 *    4. Insert 혹은 delete key 시에 key 값에 접근하는 것.             *
 * 이 있다.                                                            *
 * 여기서 index에 존재하는 variable key column은 항상 해당 node 안에   *
 * 같이 존재하므로 따로 fix/unfix를 할 필요가 없다. 따라서 특정 버퍼에 *
 * 다시 복사할 필요가 없다. 이 경우에 해당하는 것이 3,4번이다.         *
 * 1,2 번의 경우에는 본 함수를 호출할 때 aColumn의 value에 버퍼의      *
 * 위치(포인터)를 지정해서, 본 함수에서 해당 가변 컬럼의 값을 복사할   *
 * 수 있도록 한다.                                                     *
 *                                                                   *
 * 예외로, 4번 경우에서 이미 Data page에 insert된 Row의 포인터로       *
 * key의 위치를 찾는데, 이때 Row가 가변컬럼을 가질 경우에 복사해서     *
 * 비교할 버퍼가 존재하지 않는다. 이를 위해 insert cursor의 close시에  *
 * row의 after image를 생성한 후, 이 row의 모든 가변컬럼중 row와 다른  *
 * 페이지에 있는 것들을 모두 fix한 후에 index key insert를 수행하는    *
 * 방법으로 한다.                                                      *
 *                                                                   *
 * smiColumn->value의 값이 NULL이 아닌 경우에 가변컬럼 값을 복사하는데,*
 * 같은 위치의 값을 두번 이상 복사하지 않기 위해서, value의 처음에     *
 * 가변컬럼의 위치(RID)를 저장하고 그 이후에 값을 저장한다.
 ***********************************************************************/
const void* smiGetVarColumn( const void       * aRow,
                             const smiColumn  * aColumn,
                             UInt             * aLength )
{
    SChar    * sRet = NULL;
        
    sRet = sgmManager::getVarColumnDirect( (SChar*)aRow,
                                           aColumn,
                                           aLength );
    
    //BUG-48746: variable 저장 구조 변경에 따른 패딩 보정
    if ( *aLength > aColumn->size )
    {
        *aLength = aColumn->size;
    }

    return sRet;
}

/* Description: Variable Column을 읽어갈 경우 버퍼에 다음과 같은 경우
 *              데이타를 버퍼에 복사해 준다.
 *
 *              1. Disk Table에 있는 Var Column
 *              2. Memory Table에 있으면서 Row가 여러개의 Variable
 *                 Column Piece에 걸쳐서 저장될 경우
 *
 *              그런데 QP에서 같은 버퍼를 주면서 같은 데이타를 여러번
 *              읽는 경우가 있다고 한다. 단순히 Row Pointer를 준다면
 *              문제가 되지 않지만 버퍼에 복사할 경우 비용이 크다. 이 문제를
 *              해결하기 위해서 Buffer영역의 첫 8byte영역에 표시를 해둔다.
 *
 *              1. Memory: Variable Column의 첫 Piece의 포인터
 *              2. Disk : Variable Column의 SDRID
 *
 *              sdRID는 ULong이고 Memory Pointer는 32비트일때는
 *              4바이트이지만 큰것을 기준으로 해서 8바이트로 한다.
 *
 */
UInt smiGetVarColumnBufHeadSize( const smiColumn * aColumn )
{
    IDE_ASSERT( (aColumn->flag & SMI_COLUMN_STORAGE_MASK)
                == SMI_COLUMN_STORAGE_MEMORY );

    return ID_SIZEOF(ULong);
}

// For A4 : Table Type에 대한 인자 추가
UInt smiGetVarColumnLength( const void*       aRow,
                            const smiColumn * aColumn )
{
    IDE_ASSERT( (aColumn->flag & SMI_COLUMN_STORAGE_MASK)
                == SMI_COLUMN_STORAGE_MEMORY );

    return smcRecord::getVarColumnLen( aColumn, (const SChar*)aRow );

}

/* FOR A4 : Cursor 관련 함수들 */
smiRange * smiGetDefaultKeyRange( )
{
    return (smiRange*)smiTableCursor::getDefaultKeyRange();
}

smiCallBack * smiGetDefaultFilter( )
{
    return (smiCallBack*)smiTableCursor::getDefaultFilter();
}


/* -----------------------
   For Global Transaction
   ----------------------- */
IDE_RC smiXaRecover( SInt           *a_slotID,
                     /* BUG-18981 */
                     ID_XID       *a_xid,
                     timeval        *a_preparedTime,
                     smiCommitState *a_state )
{

    return smxTransMgr::recover( a_slotID,
                                 a_xid,
                                 a_preparedTime,
                                 (smxCommitState *)a_state );

}

/* -----------------------
   For Global Transaction
   ----------------------- */
/***********************************************************************
 * Description : checkpoint쓰레드를 통해 checkpoint를 수행한다.
 *               수행시 입력받은 aStart가 True인경우 Turn off 상태인
 *               Flusher들을 깨운뒤 수행한다.

 *                                                                       
 * aStatistics   - [IN] None                                             
 * a_pTrans      - [IN] Transaction Pointer                              
 * aStart        - [IN] Turn Off상태인 Flusher들을 깨워야 하는지 여부 
 ***********************************************************************/
IDE_RC smiCheckPoint( idvSQL   * aStatistics,
                      idBool     aStart )
{
    if ( aStart == ID_TRUE )
    {
        IDE_TEST( sdsFlushMgr::turnOnAllflushers() != IDE_SUCCESS );
        IDE_TEST( sdbFlushMgr::turnOnAllflushers() != IDE_SUCCESS );
    }
    else
    {
      // Nothing To do
    }
  
    IDE_TEST( gSmrChkptThread.resumeAndWait( aStatistics )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC _smiSetAger( idBool aValue )
{

    static idBool sValue = ID_TRUE;

    if ( aValue != sValue )
    {
        if ( aValue == ID_TRUE )
        {
            IDE_TEST( smaLogicalAger::unblock() != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( smaLogicalAger::block() != IDE_SUCCESS );
        }
        sValue = aValue;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

#define MSG_BUFFER_SIZE (128 * 1024)

void smiGetTxLockInfo( smiTrans *aTrans, smTID *aOwnerList, UInt *aOwnerCount )
{
    smxTrans *sTrans;

    //for fix Bug PR-5827
    *aOwnerCount=0;
    sTrans = (smxTrans*)aTrans->getTrans();
    if ( sTrans != NULL )
    {
        smlLockMgr::getTxLockInfo( sTrans->mSlotN, aOwnerList, aOwnerCount );
    }
    else
    {
        /* nothing to do */
    }
}

/***********************************************************************
 * Description : [PROJ-1362] Table의 Type별로 LOB Column Piece의
 *               크기를 반환한다.
 *               Disk의 경우 idBool로 NULL의 여부만 반환한다.
 *               이를 변경하려면 mtdClob, mtdBlob의
 *               mtdStoredValue2MtdValu() 와 같이 처리해야 한다.
 *
 *    aTableType  - [IN] LOB Column의 Table의 Type
 **********************************************************************/
UInt smiGetLobColumnSize(UInt aTableType)
{
    UInt sTableType = aTableType & SMI_TABLE_TYPE_MASK;
    
    if ( sTableType == SMI_TABLE_DISK )
    {
        return idlOS::align8(ID_SIZEOF(sdcLobDesc));
    }
    else
    {
        return idlOS::align8(ID_SIZEOF(smcLobDesc));
    }
    
}

/***********************************************************************
 * Description : [PROJ-1362] LOB Column이 Null ( length == 0 )인지
 *               유무를 반환한다.
 *
 *    aRow       - [IN] Fetch해서 읽은 LOB Column Data
 *                      Memory일 경우 LOB Descriptor가 들어있고
 *                      Disk의 경우 Lob Column의 크기가 들어있다.
 *    aColumn    - [IN] LOB Column 정보
 **********************************************************************/
idBool smiIsNullLobColumn( const void*       aRow,
                           const smiColumn * aColumn )
{
    SChar       * sRow;
    sdcLobDesc    sDLobDesc;
    smcLobDesc  * sMLobDesc;
    idBool        sIsNullLob;

    sRow = (SChar*)aRow + aColumn->offset;

    if ((aColumn->flag & SMI_COLUMN_STORAGE_MASK) == SMI_COLUMN_STORAGE_DISK)
    {
        idlOS::memcpy( &sDLobDesc, sRow, ID_SIZEOF(sdcLobDesc) );

        if ( (sDLobDesc.mLobDescFlag & SDC_LOB_DESC_NULL_MASK) ==
             SDC_LOB_DESC_NULL_TRUE )
        {
            sIsNullLob = ID_TRUE;
        }
        else
        {
            sIsNullLob = ID_FALSE;
        }
    }
    else
    {
        sMLobDesc = (smcLobDesc*)sRow;
        if ( (sMLobDesc->flag & SM_VCDESC_NULL_LOB_MASK) ==
             SM_VCDESC_NULL_LOB_TRUE )
        {
            sIsNullLob = ID_TRUE;
        }
        else
        {
            sIsNullLob = ID_FALSE;
        }
    }

    return sIsNullLob;
}

//  For A4 : TableSpace type별로 Maximum fixed row size를 반환한다.
//  slot header 포함.
UInt smiGetMaxFixedRowSize( smiTableSpaceType aTblSpaceType )
{
    if ( (aTblSpaceType == SMI_MEMORY_SYSTEM_DICTIONARY) ||
         (aTblSpaceType == SMI_MEMORY_SYSTEM_DATA) ||
         (aTblSpaceType == SMI_MEMORY_USER_DATA) )
    {
        return SMP_MAX_FIXED_ROW_SIZE;
    }
    else
    {
        IDE_ASSERT(0);
    }
}

/*
    SMI Layer의 Tablespace Lock Mode가 Exclusive Lock인지 여부 확인

    [IN] aLockMode - SMI Layer의 Lock Mode
 */
idBool isExclusiveTBSLock( smiTBSLockMode aLockMode )
{
    idBool sIsExclusive ;


    switch ( aLockMode )
    {
        case SMI_TBS_LOCK_EXCLUSIVE :
            sIsExclusive = ID_TRUE;
            break;

        case SMI_TBS_LOCK_SHARED :
            sIsExclusive = ID_FALSE;
            break;
        default:
            // 위의 두가지 값중 하나여야 한다.
            IDE_ASSERT(0);
    }

    return sIsExclusive;
}

/* 테이블 스페이스에 대해 Lock을 획득하고 Validation을 수행한다.

   [IN]  aStmt         : Statement의 void* 형
   [IN]  scSpaceID     : Lock을 획득할 Tablespace의  ID
   [IN]  aTBSLockMode  : Tablespace의 Lock Mode
   [IN]  aTBSLvType    : Tablespace 상태 Validation옵션
   [IN]  aLockWaitMicroSec : 잠금요청후 Wait 시간
 */
IDE_RC smiValidateAndLockTBS( smiStatement        * aStmt,
                              scSpaceID             aSpaceID,
                              smiTBSLockMode        aLockMode,
                              smiTBSLockValidType   aTBSLvType,
                              ULong                 aLockWaitMicroSec )
{
    smiTrans *sTrans;
    idBool    sIsExclusiveLock;

    IDE_ASSERT( aStmt != NULL );
    IDE_ASSERT( aLockMode  != SMI_TBS_LOCK_NONE );
    IDE_ASSERT( aTBSLvType != SMI_TBSLV_NONE );

    sTrans = aStmt->getTrans();

    sIsExclusiveLock = isExclusiveTBSLock( aLockMode );

    IDE_TEST_RAISE( sctTableSpaceMgr::lockAndValidateTBS(
                                    (void*)(sTrans->getTrans()),
                                    aSpaceID,
                                    sctTableSpaceMgr::getTBSLvType2Opt( aTBSLvType ),
                                    ID_FALSE,    /* Is Intent */
                                    sIsExclusiveLock,
                                    aLockWaitMicroSec )
                    != IDE_SUCCESS, ERR_TBS_LOCK_VALIDATE );

    /* BUG-18279: Drop Table Space시에 생성된 Table을 빠뜨리고
     *            Drop이 수행됩니다.
     *
     * Tn: Transaction n, TBS = Tablespace
     * 1. TBS에 Drop을 수행하는 Transaction인 T1이 ViewSCN을 딴다.
     * 2. DDL(Create Table, Drop, Alter)을 T2가 수행한다. 그리고 Commit한다.
     * 3. T1이 Drop Tablespace를 수행하는데 T2가 수행한 결과를 보지 못한다.
     *    왜냐하면 T2가 commit하기전에 ViewSCN을 따기때문이다.
     *
     * 위 문제해결을 위해서 Tablespace에 DDLCommitSCN을 두고 Table에 대한 DDL마다
     * 자신의 Commit SCN을 설정하도록 하였다. 그리고 Tablespace에 대해서
     * Drop을 수행할때마다 자신의 ViewSCN이 Tablespace의 DDLCommitSCN보다
     * 큰지를 검사하고 작으면 Statement Rebuild Error를 낸다.
     */
    if ( sIsExclusiveLock == ID_TRUE )
    {
        IDE_TEST( sctTableSpaceMgr::canDropByViewSCN( aSpaceID,
                                                      aStmt->getSCN() )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TBS_LOCK_VALIDATE );
    {
        /* Tablespace가 발견되지 않은 경우
           Tablespace가 Drop되었다가
           같은 이름으로 다시 생성된 경우일 수 있다.

           이 경우 Rebuild Error를 올려서 QP에서 Validation을 다시
           수행하여 새로운 Tablespace ID로 다시 수행되도록 유도한다.
        */

        if ( ( ideGetErrorCode() == smERR_ABORT_NotFoundTableSpaceNode ) ||
             ( ideGetErrorCode() == smERR_ABORT_NotFoundTableSpaceNodeByName ) )
        {
            IDE_SET( ideSetErrorCode( smERR_REBUILD_smiTBSModified ) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
   PRJ-1548 User Memory Tablespace

   테이블에 대한 유효성 검사 및 잠금 획득

   테이블과 관련된 모든 테이블스페이스(데이타, 인덱스)에 대하여
   잠금을 구문에 따라 획득한다.

   fix BUG-17121
   반드시, 테이블스페이스 -> 테이블 -> Index,Lob 테이블스페이스
   순으로 잠금을 획득한다.

   예외적으로 메타 테이블과 시스템 테이블스페이스는 잠금을 획득하지 않는다.

   A. 테이블에 IX 또는 X 잠금을 획득하기 위해서는
      테이블스페이스 IX 잠금을 먼저 획득해야함.

   B. 테이블에 IS 또는 S 잠금을 획득하기 위해서는
      테이블스페이스 IS 잠금을 먼저 획득해야함.
 

   BUG-28752 lock table ... in row share mode 구문이 먹히지 않습니다.

   implicit/explicit lock을 구분하여 겁니다.
   implicit is lock만 statement end시 풀어주기 위함입니다. */


IDE_RC smiValidateAndLockObjects( smiTrans           * aTrans,
                                  const void         * aTable,
                                  smSCN                aSCN,
                                  smiTBSLockValidType  aTBSLvType,
                                  smiTableLockMode     aLockMode,
                                  ULong                aLockWaitMicroSec,
                                  idBool               aIsExplicitLock )
{
    scSpaceID             sSpaceID;
    smcTableHeader      * sTable;
    idBool                sLocked;
    smlLockNode         * sCurLockNodePtr = NULL;
    smlLockSlot         * sCurLockSlotPtr = NULL;
    smiTableLockMode      sTableLockMode;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );

    // Lock Table
    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    sSpaceID = smcTable::getTableSpaceID( (void*)sTable );

    IDU_FIT_POINT_RAISE( "smiValidateAndLockObjects::ERR_TBS_LOCK_VALIDATE", ERR_TBS_LOCK_VALIDATE ); 

    // 테이블의 테이블스페이스들에 대하여 INTENTION 잠금을 획득한다.
    IDE_TEST_RAISE( sctTableSpaceMgr::lockAndValidateTBS(
                                     (void*)((smxTrans*)aTrans->getTrans()), /* smxTrans* */
                                     sSpaceID,          /* LOCK 획득할 TBSID */
                                     sctTableSpaceMgr::getTBSLvType2Opt( aTBSLvType ),/* TBS Lock Validation Type */
                                     ID_TRUE,           /* intent lock  여부 */
                                     smlLockMgr::isNotISorS((smlLockMode)aLockMode),
                                     aLockWaitMicroSec ) != IDE_SUCCESS,
                    ERR_TBS_LOCK_VALIDATE );

    // PRJ-1476
    IDE_TEST_CONT( SMI_TABLE_TYPE_IS_META( sTable ) == ID_TRUE,
                   skip_lock_meta_table);

    // BUG-14113
    // Isolation Level에 맞는 table lock mode를 구함
    sTableLockMode = smiTableLockModeOnIsolationLevel[(UInt)aTrans->getIsolationLevel()]
                                                     [(UInt)aLockMode];

    /* BUG-32237 [sm_transaction] Free lock node when dropping table.
     * DropTablePending 에서 연기해둔 freeLockNode를 수행합니다. */
    /* 이미 Drop된 경우, rebuild 하면 문제 없음.
     * QP에서 과거에 기록해둔 TableOID를 보고 쫓아올 수 있기 때문에
     * 정상 상황. */
    IDE_TEST( smiValidateObjects( aTable, aSCN ) != IDE_SUCCESS );

    // 테이블 대하여 잠금을 획득한다.
    IDE_TEST( smlLockMgr::lockTable( ((smxTrans*)aTrans->getTrans())->mSlotN,
                                     (smlLockItem *)SMC_TABLE_LOCK( sTable ),
                                     (smlLockMode)sTableLockMode,
                                     aLockWaitMicroSec,
                                     NULL,
                                     &sLocked,
                                     &sCurLockNodePtr,
                                     &sCurLockSlotPtr,
                                     aIsExplicitLock ) != IDE_SUCCESS );

    IDE_TEST( sLocked == ID_FALSE );

    IDU_FIT_POINT("smiValidateAndLockObjects::_FT_");

    IDE_TEST( smiValidateObjects( aTable, aSCN ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT(skip_lock_meta_table);

    // 테이블의 Index, Lob 컬럼 관련 테이블스페이스들에 대하여
    // INTENTION 잠금을 획득한다.
    IDE_TEST( sctTableSpaceMgr::lockAndValidateRelTBSs(
                                     (void*)((smxTrans*)aTrans->getTrans()), /* smxTrans* */
                                     (void*)sTable,                          /* smcTableHeader */
                                     sctTableSpaceMgr::getTBSLvType2Opt( aTBSLvType ),/* TBS Lock Validation Type */
                                     ID_TRUE,                                /* intent lock  여부 */
                                     smlLockMgr::isNotISorS((smlLockMode)aLockMode),
                                     aLockWaitMicroSec ) != IDE_SUCCESS );

    return IDE_SUCCESS;


    IDE_EXCEPTION( ERR_TBS_LOCK_VALIDATE );
    {
        if ( (ideGetErrorCode() == smERR_ABORT_NotFoundTableSpaceNode ) ||
             (ideGetErrorCode() == smERR_ABORT_NotFoundTableSpaceNodeByName ) )
        {
            IDE_SET( ideSetErrorCode( smERR_REBUILD_smiTBSModified ) );
        }
    }

    IDE_EXCEPTION_END;

    if ( sCurLockSlotPtr != NULL )
    {
        (void)smlLockMgr::unlockTable( ((smxTrans*)aTrans->getTrans())->mSlotN,
                                       sCurLockNodePtr,
                                       sCurLockSlotPtr );
    }

    if ( ideGetErrorCode() == smERR_REBUILD_smiTableModified )
    {
        SMX_INC_SESSION_STATISTIC( aTrans->mTrans,
                                   IDV_STAT_INDEX_STMT_REBUILD_COUNT,
                                   1 /* Increase Value */ );
    }

    return IDE_FAILURE;
}


/*
   테이블에 대한 유효성 검사
*/
IDE_RC smiValidateObjects( const void         * aTable,
                           smSCN                aCachedSCN )
{
    smcTableHeader      * sTable;
    smSCN sSCN;
    smTID sDummyTID;

    IDE_DASSERT( aTable != NULL );

    SMX_GET_SCN_AND_TID( ((smpSlotHeader*)aTable)->mCreateSCN, sSCN, sDummyTID );
    
    IDU_FIT_POINT_RAISE( "smiValidateObjects::ERR_TABLE_MODIFIED", ERR_TABLE_MODIFIED );

    IDE_TEST_RAISE ( SM_SCN_IS_DELETED(sSCN), ERR_TABLE_MODIFIED );

    sTable = (smcTableHeader*)( (UChar*)aTable + SMP_SLOT_HEADER_SIZE );
    IDE_TEST_RAISE( smcTable::isDropedTable( sTable ) == ID_TRUE,
                    ERR_TABLE_MODIFIED );

    /* PROJ-2268 Table의 SCN과 cached SCN이 같지 않다면 재활용된 Slot이다. */
    IDE_TEST_RAISE( SM_SCN_IS_EQ( &sSCN, &aCachedSCN ) != ID_TRUE,
                    ERR_TABLE_MODIFIED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TABLE_MODIFIED );
    {
        IDE_SET( ideSetErrorCode( smERR_REBUILD_smiTableModified ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2268 Reuse Catalog Table Slot
 * Plan에 등록되어 있는 Slot이 재사용되었는지 확인한다. */
IDE_RC smiValidatePlanHandle( const void * aHandle,
                              smSCN        aCachedSCN )
{
    /* slot이 재활용되지 않는다면 validation 할 필요가 없다. */
    if ( smuProperty::getCatalogSlotReusable() == 1 )
    {
        IDE_TEST( smiValidateObjects( aHandle, aCachedSCN )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smiGetGRIDFromRowPtr( const void   * aRowPtr,
                             scGRID       * aRowGRID )
{
    smpPersPageHeader   * sPageHdrPtr;
    void                * sTableHeader;

    IDE_DASSERT( aRowPtr != NULL );
    IDE_DASSERT( aRowGRID != NULL );

    SC_MAKE_NULL_GRID( *aRowGRID );

    sPageHdrPtr = SMP_GET_PERS_PAGE_HEADER( aRowPtr );

    IDE_TEST( smcTable::getTableHeaderFromOID( sPageHdrPtr->mTableOID,
                                               &sTableHeader )
              != IDE_SUCCESS );

    SC_MAKE_GRID( *aRowGRID,
                  smcTable::getTableSpaceID( sTableHeader ),
                  SMP_SLOT_GET_PID( aRowPtr ),
                  SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRowPtr ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smiFetchRowFromGRID( idvSQL                      * aStatistics,
                            smiStatement                * aStatement,
                            UInt                          aTableType,
                            scGRID                        aRowGRID,
                            const smiFetchColumnList    * aFetchColumnList,
                            void                        * aTableHdr,
                            void                        * aDestRowBuf )
{
    UChar                * sSlotPtr;
    idBool                 sIsSuccess;
    smxTrans             * sTrans;
    sdSID                  sTSSlotSID;
    sdpPageType            sPageType;
    smSCN                  sSCN;
    idBool                 sIsRowDeleted;
    smSCN                  sInfiniteSCN;
    idBool                 sIsPageLatchReleased = ID_TRUE;
    const smcTableHeader * sTableHeader;
    idBool                 sSkipFetch = ID_FALSE;   /* BUG-43844 */

    SM_MAX_SCN( &sInfiniteSCN );
    SM_MAX_SCN( &sSCN );

    sTrans = (smxTrans*)aStatement->getTrans()->getTrans();
    sTSSlotSID = smxTrans::getTSSlotSID(sTrans);
    sTableHeader = SMI_MISC_TABLE_HEADER( aTableHdr );

    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aRowGRID) );
    IDE_ASSERT( aTableType == SMI_TABLE_DISK );

    /* BUG-43801 : disk일 경우 slotnum으로 호출 */ 
    IDE_TEST_RAISE( SC_GRID_IS_NOT_WITH_SLOTNUM( aRowGRID ), error_invalid_grid );

    IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                           aRowGRID,
                                           SDB_S_LATCH,
                                           SDB_WAIT_NORMAL,
                                           NULL, /* aMtx */
                                           (UChar**)&sSlotPtr,
                                           &sIsSuccess,
                                           &sSkipFetch )
              != IDE_SUCCESS );
    sIsPageLatchReleased = ID_FALSE;

    /* BUG-43844 : 이미 free된 슬롯(unused 플래그 체크된)에 접근하여 fetch를
     * 시도 할 경우 스킵 */
    if ( sSkipFetch == ID_TRUE )
    {
        IDE_CONT( SKIP_FETCH );
    }
    else
    {
        /* do nothing */    
    }

    sPageType = sdpPhyPage::getPageType( sdpPhyPage::getHdr(sSlotPtr) );
    IDE_ASSERT(sPageType == SDP_PAGE_DATA);

    sSCN         = aStatement->getSCN();
    
    /* 동일 statement의 이전 cursor가 INSERT한 row를 보지 못하는 문제등으로
     * Transaction으로부터 infiniteSCN를 가져오도록 되어 있었으나 (BUG-33889)
     * closeCursor()에서 infiniteSCN을 증가 시키도록 수정되었음.(BUG-32963)
     * BUG-43801 등의 문제도 발생하므로 statement에서 infiniteSCN을 가져오도록 수정함.*/
    sInfiniteSCN = aStatement->getInfiniteSCN();

    IDU_FIT_POINT( "smiFetchRowFromGRID::fetch" );

    IDE_TEST( sdcRow::fetch( aStatistics,
                             NULL, /* aMtx */
                             NULL, /* aSP */
                             sTrans,
                             SC_MAKE_SPACE(aRowGRID),
                             sSlotPtr,
                             ID_TRUE, /* aIsPersSlot */
                             SDB_SINGLE_PAGE_READ,
                             aFetchColumnList,
                             SMI_FETCH_VERSION_CONSISTENT,
                             sTSSlotSID,
                             &sSCN,
                             &sInfiniteSCN,
                             NULL, /* aIndexInfo4Fetch */
                             NULL, /* aLobInfo4Fetch */
                             sTableHeader->mRowTemplate,
                             (UChar*)aDestRowBuf,
                             &sIsRowDeleted,
                             &sIsPageLatchReleased )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsRowDeleted == ID_TRUE, error_deletedrow );

    IDE_EXCEPTION_CONT( SKIP_FETCH );

    /* BUG-23319
     * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
    /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
     * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
     * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
     * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
     * 상황에 따라 적절한 처리를 해야 한다. */
    if ( sIsPageLatchReleased == ID_FALSE )
    {
        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             sSlotPtr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    /* BUG-33889  [sm_transaction] The fetching by RID uses incorrect
     * ViewSCN. */
    IDE_EXCEPTION( error_deletedrow );
    {
        ideLog::log( IDE_SM_0, 
                     "InternalError[%s:%"ID_INT32_FMT"]\n"
                     "GRID     : <%"ID_UINT32_FMT",%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n"
                     "TSSRID   : %"ID_UINT64_FMT"\n"
                     "SCN      : 0X%"ID_UINT64_FMT", 0X%"ID_XINT64_FMT"\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__,
                     SC_MAKE_SPACE( aRowGRID ),
                     SC_MAKE_PID( aRowGRID ),
                     SC_MAKE_SLOTNUM( aRowGRID ),
                     sTSSlotSID,
                     SM_SCN_TO_LONG(sSCN),
                     SM_SCN_TO_LONG(sInfiniteSCN) );
        ideLog::logCallStack( IDE_SM_0 );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL_ARG, 
                                  "Disk Fetch by RID" ) );
        
    }
    IDE_EXCEPTION( error_invalid_grid )
    {
        ideLog::log( IDE_SM_0,
                     "InternalError[%s:%d]\n"
                     "GRID     : <%"ID_UINT32_FMT",%"ID_UINT32_FMT",%"ID_UINT32_FMT">\n"
                     "TSSRID   : %"ID_UINT64_FMT"\n"
                     "SCN      : 0X%"ID_UINT64_FMT", 0X%"ID_XINT64_FMT"\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__,
                     SC_MAKE_SPACE( aRowGRID ),
                     SC_MAKE_PID( aRowGRID ),
                     SC_MAKE_OFFSET( aRowGRID ),
                     sTSSlotSID,
                     SM_SCN_TO_LONG(sSCN),
                     SM_SCN_TO_LONG(sInfiniteSCN) );
        ideLog::logCallStack( IDE_SM_0 );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL_ARG,
                                  "Disk Fetch by Invalid GRID " ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                               sSlotPtr )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/*
  FOR A4 :
      TableSpace별로 Size를 알아내기 위해 id 인자를 추가
*/

// smiAPI function
// callback for gSmiGlobalBackList.XXXEmergencyFunc.
void smiSetEmergency(idBool aFlag)
{
    if ( aFlag == ID_TRUE )
    {
        gSmiGlobalCallBackList.setEmergencyFunc(SMI_EMERGENCY_DB_SET);
    }
    else
    {
        gSmiGlobalCallBackList.clrEmergencyFunc(SMI_EMERGENCY_DB_CLR);
    }

}

void smiSwitchDDL(UInt aFlag)
{
    gSmiGlobalCallBackList.switchDDLFunc(aFlag);
}

UInt smiGetCurrTime()
{
    return gSmiGlobalCallBackList.getCurrTimeFunc();
}

/* ------------------------------------------------
 * for replication RPI function
 * ----------------------------------------------*/
void smiSetCallbackFunction(
                smGetMinSN                   aGetMinSNFunc,
                smIsReplCompleteBeforeCommit aIsReplCompleteBeforeCommitFunc,
                smIsReplCompleteAfterCommit  aIsReplCompleteAfterCommitFunc,
                smCopyToRPLogBuf             aCopyToRPLogBufFunc,
                smSendXLog                   aSendXLogFunc,
                smIsReplWaitGlobalTxAfterPrepare aIsReplWaitGlobalTxAfterPrepare )
{
    smrRecoveryMgr::setCallbackFunction( aGetMinSNFunc,
                                         aIsReplCompleteBeforeCommitFunc,
                                         aIsReplCompleteAfterCommitFunc,
                                         aCopyToRPLogBufFunc,
                                         aSendXLogFunc,
                                         aIsReplWaitGlobalTxAfterPrepare );
}

void smiGetLstDeleteLogFileNo( UInt *aFileNo )
{
    smrRecoveryMgr::getLstDeleteLogFileNo( aFileNo );
}

smSN smiGetReplRecoverySN()
{
    return SM_MAKE_SN( smrRecoveryMgr::getReplRecoveryLSN() );
}

IDE_RC smiSetReplRecoverySN( smSN aReplRecoverySN )
{
    smLSN sLSN;
    SM_MAKE_LSN( sLSN, aReplRecoverySN );
    return smrRecoveryMgr::setReplRecoveryLSN( sLSN );
}

IDE_RC smiGetFirstNeedLFN( smSN         aMinSN,
                           const UInt * aFirstFileNo,
                           const UInt * aEndFileNo,
                           UInt       * aNeedFirstFileNo )
{
    smLSN   sMinLSN;
    SM_MAKE_LSN( sMinLSN, aMinSN );
    return smrLogMgr::getFirstNeedLFN( sMinLSN,
                                       *aFirstFileNo,
                                       *aEndFileNo,
                                       aNeedFirstFileNo );
}

// SM에 존재하는 시스템의 통계정보를 반영한다.
// 이 함수에서는 SM 내부의 각 모듈에서 시스템에 반영되어야 하는
// 통계정보를 v$sysstat으로 반영하도록 하면 된다.
void smiApplyStatisticsForSystem()
{
    /*
     * TASK-2356 제품문제분석
     *
     * ALTIBASE WAIT INTERFACE
     * WaitEvent 대기시간 통계정보 수집
     *
     * System Thread들의 대기시간 통계정보는
     * Session/Statement Level이 아닌
     * System Level 통계정보에 주기적으로 누적된다.
     *
     * ( Call By mmtSessionManager::checkSessionTimeout() )
     *
     * Buffer Manager에는 별도의 대기시간 통계정보를 유지하지
     * 않으며, 일반적인 상태 관련 통계정보를 유지한다.
     *
     */

    if ( smiGetStartupPhase() > SMI_STARTUP_CONTROL )
    {
        // Buffer Manager의 통계 정보를 반영한다.
        sdbBufferMgr::applyStatisticsForSystem();

        gSmrChkptThread.applyStatisticsForSystem();
    }
}

/***********************************************************************
 * BUG-35392
 * Description : Fast Unlock Log Alloc Mutex 기능을 사용하지는지 여부
 *
 **********************************************************************/
idBool smiIsFastUnlockLogAllocMutex()
{
    return smuProperty::isFastUnlockLogAllocMutex();
}

/***********************************************************************

 * Description : 마지막으로 Log를 기록하기 위해 "사용된/저장된" LSN값을 리턴한다.
                 smiGetLastValidLSN 과 같은 작업임 
 * -----------------------------------------------------
 *  LSN    | 100 | 101 | 102   | 103 | 104   | 105 | 106 |
 *  Status | ok  | ok  | dummy | ok  | dummy | ok  | ok  |
 * ------------- A ------------------------------- B --
 *
 * dummy 로그를 포함한 최대 LSN (B) : 사용된 값
 * aUncompletedLSN (A) : 마지막으로 valid 한 로그를 저장한 시점
 *  FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 A를 반환함 
 * aSN  - [OUT] output parameter
 **********************************************************************/
IDE_RC smiGetLastValidGSN( smSN *aSN )
{
   /*
    * PROJ-1923
    */
    smLSN sLSN;
    SM_LSN_MAX(sLSN);

    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        /* Dummy Log를 포함하지 않는 마지막 로그 레코드의 LSN */
        (void)smrLogMgr::getUncompletedLstWriteLSN( &sLSN );
    }
    else
    {
        /* 마지막으로 기록한 로그 레코드의 LSN */
        smrLogMgr::getLstWriteLSN( &sLSN );
    }

    *aSN = SM_MAKE_SN( sLSN );

    return IDE_SUCCESS;
}


/***********************************************************************
 * Description : 마지막으로 Log를 기록하기 위해 "사용된/저장된" LSN값을 리턴한다.
 * ----------------------------------------------------
 *  LSN    | 100 | 101 | 102   | 103 | 104   | 105 | 106 |
 *  Status | ok  | ok  | dummy | ok  | dummy | ok  | ok  |
 * ------------- A ------------------------------- B --
 *
 * dummy 로그를 포함한 최대 LSN (B) : 사용된 값
 * aUncompletedLSN (A) : 마지막으로 valid 한 로그를 저장한 시점
 *  FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 A를 반환함 
 * aLSN  - [OUT] output parameter
 **********************************************************************/
IDE_RC smiGetLastValidLSN( smLSN *aLSN )
{
    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        /* Dummy Log를 포함하지 않는 마지막 로그 레코드의 LSN */
        (void)smrLogMgr::getUncompletedLstWriteLSN( aLSN );
    }
    else
    {
        /* 마지막으로 기록한 로그 레코드의 LSN */
        smrLogMgr::getLstWriteLSN( aLSN );
    }

    return IDE_SUCCESS;
}


/***********************************************************************
 * BUG-43426
 *
 *   LSN 106까지 쓰여진 시점에서 이 함수가 호출되었고,
 *   FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우
 *   106번까지의 dummy 로그가 정상로그가 될때까지 WAIT하였다가
 *   106을 리턴하게 된다.
 *
 * -----------------------------------------------------
 *  LSN    | 100 | 101 | 102   | 103 | 104   | 105 | 106 |
 *  Status | ok  | ok  | dummy | ok  | dummy | ok  | ok  |
 * ------------- A ------------------------------- B --
 *
 * dummy 로그를 포함한 최대 LSN (B) : 사용된 값
 * aUncompletedLSN (A) : 마지막으로 valid 한 로그를 저장한 시점
 * (B) 와 (A) 가 같아지는것을 대기 함. 
 **********************************************************************/
IDE_RC smiWaitAndGetLastValidGSN( smSN *aSN )
{
    smLSN           sLstWriteLSN;
    smLSN           sUncompletedLstWriteLSN;
    UInt            sIntervalUSec  = smuProperty::getUCLSNChkThrInterval();
    PDL_Time_Value  sTimeOut;

    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        smrLogMgr::getLstWriteLSN( &sLstWriteLSN );
        smrLogMgr::getUncompletedLstWriteLSN( &sUncompletedLstWriteLSN );

        while ( smrCompareLSN::isGT( &sLstWriteLSN, &sUncompletedLstWriteLSN ) )
        {
            /* UCLSNChk 쓰레드는
               smuProperty::getUCLSNChkThrInterval() 주기로
               MinUncompletedSN 값을 변경한다.
               여기서도 이값과 동일하게 WAIT한다. */
            sTimeOut.set( 0, sIntervalUSec );
            idlOS::sleep( sTimeOut );

            smrLogMgr::getUncompletedLstWriteLSN( &sUncompletedLstWriteLSN );
        }
    }
    else
    {
        smrLogMgr::getLstWriteLSN( &sLstWriteLSN );
    }

    *aSN = SM_MAKE_SN( sLstWriteLSN );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : 마지막으로 Log를 기록하기 위해 사용된 LSN값을 리턴한다.
 *               FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 더미를 포함한 로그 레코드의 LSN
 *               (smiGetLastUsedLSN 과 같은 작업임)
 **********************************************************************/
IDE_RC smiGetLastUsedGSN( smSN *aSN )
{
    smLSN sLSN;
    SM_LSN_MAX(sLSN);
    
    smrLogMgr::getLstWriteLSN( &sLSN );
    *aSN = SM_MAKE_SN( sLSN );

    return IDE_SUCCESS;
}

#if 0
/***********************************************************************
 * Description : 마지막으로 Log를 기록하기 위해 사용된 LSN값을 리턴한다.
 *               FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 더미를 포함한 로그 레코드의 LSN
 **********************************************************************/
IDE_RC smiGetLastUsedLSN( smLSN *aLSN )
{
    smrLogMgr::getLstWriteLSN( aLSN );

    return IDE_SUCCESS;
}
#endif

/***********************************************************************
 * Description : PROJ-1969
 *               마지막으로 기록한 로그 레코드의 "Last Offset"을 반환한다.  
 **********************************************************************/
IDE_RC smiGetLstLSN( smLSN      * aEndLSN )
{
    smrLogMgr::getLstLSN( aEndLSN );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Transaction Table Size를 Return한다.
 *
 **********************************************************************/
UInt smiGetTransTblSize()
{
    // BUG-27598 TRANSACTION_TABLE_SIZE 를 SM 과 RP 가
    // 서로 다르게 해석하여 이중화시 비정상 종료할 수 있음.
    return smxTransMgr::getCurTransCnt();
}

/***********************************************************************
 * Description :
 *
 **********************************************************************/
const SChar * smiGetArchiveDirPath()
{
    return smuProperty::getArchiveDirPath();
}

smiArchiveMode smiGetArchiveMode()
{
    return smrRecoveryMgr::getArchiveMode();
}

/***********************************************************************

 Description :

 SM의 한 tablespace의 내용을 검증한다.

 ALTER SYSTEM VERIFY 와 같은 류의 SQL을 사용자가 처리한다면
 QP에서 구문 파싱후  이 함수를 호출한다.

 현재는 SMI_VERIFY_TBS 만 동작하지만, 추후에 기능이 추가될 수 있다.

 호출 예) smiVerifySM(SMI_VERIFY_TBS)

 Implementation :

 wihle(모든 tablespace)
     tablespace verify

**********************************************************************/
IDE_RC smiVerifySM( idvSQL  * aStatistics,
                    UInt      aFlag)
{

    sctTableSpaceNode  *sCurrSpaceNode;
    sctTableSpaceNode  *sNextSpaceNode;

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurrSpaceNode != NULL )
    {
        sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );

        if ( (sctTableSpaceMgr::isMemTableSpace( sCurrSpaceNode )  == ID_TRUE) ||
             (sctTableSpaceMgr::isVolatileTableSpace( sCurrSpaceNode ) == ID_TRUE) )
        {
            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }

        IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( sCurrSpaceNode )
                     == ID_TRUE );

        if ( aFlag & SMI_VERIFY_TBS )
        {
            // fix BUG-17377
            // ENABLE_RECOVERY_TEST = 1 인 경우에만 수행됨.
            if ( sctTableSpaceMgr::hasState( sCurrSpaceNode,
                                             SCT_SS_SKIP_IDENTIFY_DB )
                 == ID_FALSE )
            {
                // Verify 수행함.
                IDE_TEST( sdpTableSpace::verify( aStatistics,
                                                 sCurrSpaceNode->mID,
                                                 aFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                // DISCARDED/DROPPED 된 TBS에 대한 Verify는 수행하지 않음.
            }
        }
        else
        {
            IDE_DASSERT(ID_FALSE);
        }

        sCurrSpaceNode = sNextSpaceNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiUpdateIndexModule(void *aIndexModule)
{
    return smnManager::updateIndexModule((smnIndexModule *)aIndexModule);
}

/*
 * Enhancement BUG-15010
 * [유지보수성] 쿼리 튜닝시 buffer pool을 initialize하는 기능이 절실히 필요합니다.
 *
 * 본 인터페이스 호출시에 모든 buffer pool에 존재하는 모든 페이지를 FREE LIST로
 * 반환하므로, 운영시 호출될 경우 100% buffer miss가 발생할 수 있다.
 *
 * SYSDBA 권한 획득이 필요하다.
 *
 * iSQL> alter system flush buffer_pool;
 *
 * [ 인자 ]
 *
 * aStatistics - 통계정보
 *
 */
IDE_RC smiFlushBufferPool( idvSQL * aStatistics )
{
    IDE_ASSERT( aStatistics != NULL );

    ideLog::log( SM_TRC_LOG_LEVEL_INTERFACE,
                 SM_TRC_BUFFER_FLUSH_BUFFER_POOL );

    IDE_TEST( sdbBufferMgr::pageOutAll( aStatistics)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( SM_TRC_LOG_LEVEL_INTERFACE,
                 SM_TRC_BUFFER_FLUSH_BUFFER_POOL_FAILURE );

    return IDE_FAILURE;
}

/*
 * iSQL> alter system flush secondary_buffer;
 *
 * [ 인자 ]
 *
 * aStatistics - 통계정보
 *
 */
IDE_RC smiFlushSBuffer( idvSQL * aStatistics )
{
    IDE_ASSERT( aStatistics != NULL );

    ideLog::log( SM_TRC_LOG_LEVEL_INTERFACE,
                 SM_TRC_BUFFER_FLUSH_SECONDARY_BUFFER );

    IDE_TEST( sdsBufferMgr::pageOutAll( aStatistics )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( SM_TRC_LOG_LEVEL_INTERFACE,
                 SM_TRC_BUFFER_FLUSH_SECONDARY_BUFFER_FAILURE );

    return IDE_FAILURE;
}

/* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발

   Table/Index/Sequence의
   Create/Alter/Drop DDL에 대해 Query String을 로깅한다.
 */
IDE_RC smiWriteDDLStmtTextLog( idvSQL*          aStatistics,
                               smiStatement   * aStmt,
                               smiDDLStmtMeta * aDDLStmtMeta,
                               SChar *          aStmtText,
                               SInt             aStmtTextLen )
{
    IDE_DASSERT( aStmt != NULL );
    IDE_DASSERT( aDDLStmtMeta != NULL );
    IDE_DASSERT( aStmtText != NULL );
    IDE_DASSERT( aStmtTextLen > 0 );

    IDE_TEST( smrLogMgr::writeDDLStmtTextLog( aStatistics,
                                              aStmt->getTrans()->getTrans(),
                                              (smrDDLStmtMeta *)aDDLStmtMeta,
                                              aStmtText,
                                              aStmtTextLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : sync된 log file의 첫 번째 로그 중 가장 작은 값을 리턴한다.
 *
 * aLSN  - [OUT] output parameter
 **********************************************************************/
IDE_RC smiGetSyncedMinFirstLogSN( smSN   *aSN )
{
    smLSN sLSN;
    IDE_TEST( smrLogMgr::getSyncedMinFirstLogLSN( &sLSN ) != IDE_SUCCESS );
    *aSN = SM_MAKE_SN(sLSN); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  checkpoint 에서 제거한 마지막 파일 번호를 얻어와 smSN으로 변화해서 리턴한다.
 *                제거된 파일의 마지막 위치를 찾는 것이 목적이기 때문에 offset은 MAX로 셋팅한다.
 * aLSN  - [OUT] output parameter
 **********************************************************************/
void  smiGetRemoveMaxLastLogSN( smSN   *aSN )
{
    UInt  sFileNo = 0;
    smLSN sLSN;
    
    SM_LSN_MAX( sLSN );
    
    smrRecoveryMgr::getLastRemovedFileNo( &sFileNo );
    if ( sFileNo != 0 )
    {
        sLSN.mFileNo = sFileNo;
    }

    *aSN = SM_MAKE_SN(sLSN); 
}

/***********************************************************************
 * Description :
 *
 **********************************************************************/
IDE_RC smiFlusherOnOff( idvSQL  * aStatistics,
                        UInt      aFlusherID,
                        idBool    aStart )
{
    IDE_TEST_RAISE( aFlusherID >= sdbFlushMgr::getFlusherCount(),
                    ERROR_INVALID_FLUSHER_ID );
   
    if ( aStart == ID_TRUE )
    {
        IDE_TEST( sdbFlushMgr::turnOnFlusher(aFlusherID)
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdbFlushMgr::turnOffFlusher(aStatistics, aFlusherID)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_FLUSHER_ID );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smiInvalidFlusherID,
                                  aFlusherID,
                                  sdbFlushMgr::getFlusherCount()) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiSBufferFlusherOnOff( idvSQL *aStatistics,
                               UInt    aFlusherID,
                               idBool  aStart )
{ 
    IDE_TEST_RAISE( aFlusherID >= sdbFlushMgr::getFlusherCount(),
                    ERROR_INVALID_FLUSHER_ID );

    if ( aStart == ID_TRUE )
    {
        IDE_TEST( sdsFlushMgr::turnOnFlusher( aFlusherID )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdsFlushMgr::turnOffFlusher( aStatistics, aFlusherID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_FLUSHER_ID );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smiInvalidFlusherID,
                                  aFlusherID,
                                  sdbFlushMgr::getFlusherCount() ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// Disk Segment를 생성하기 위한 최소 Extent 개수
UInt smiGetMinExtPageCnt()
{
    return SDP_MIN_EXTENT_PAGE_CNT;
}

/* BUG-20727 */
IDE_RC smiExistPreparedTrans( idBool *aExistFlag )
{
    return smxTransMgr::existPreparedTrans( aExistFlag );
}

/**********************************************************************
 *
 * Description : smxDef.h에서 define된 PSM Savepoint Name을 리턴
 *
 * BUG-21800 [RP] Receiver에서 PSM Savepoint를 구분하지 않아 Explicit
 *                Savepoint로 처리 되었습니다
 **********************************************************************/
SChar * smiGetPsmSvpName()
{
    return (SChar*)SMX_PSM_SAVEPOINT_NAME;
}

/**********************************************************************
 *
 * Description : 트랜잭션관리자에 Active 트랜잭션들의 최소 Min ViewSCN을
 *               구하도록 요청함.
 *
 * BUG-23637 최소 디스크 ViewSCN을 트랜잭션레벨에서 Statement 레벨로 구해야함.
 *
 * aStatistics  - [IN] statistics
 *
 **********************************************************************/
IDE_RC smiRebuildMinViewSCN( idvSQL  * aStatistics )
{
    return smxTransMgr::rebuildMinViewSCN( aStatistics );
}

/* PROJ-1915 : Log 파일 사이즈 리턴 */
ULong smiGetLogFileSize()
{
    return smuProperty::getLogFileSize();
}

/* PROJ-1915 : smVersion 체크 */
UInt smiGetSmVersionID()
{
    return smVersionID;
}


/*
 * BUG-27949 [5.3.3] add datafile 에서. DDL_LOCK_TIMEOUT 을
 *           무시하고 있습니다. (근로복지공단)
 *
 * SM,QP 에서 독립적으로 관리하던 DDL_LOCK_TIMEOUT을 sm에서 통합해서 관리함.
 */
/* BUG-47577: Int 로 받아서 Long의 연산을 해서 overflow  발생함.
 *            SInt -> SLong 변경
 */
ULong smiGetDDLLockTimeOut(smiTrans * aTrans)
{
    SInt sDDLLockTimeoutProperty = smuProperty::getDDLLockTimeOut();
    ULong sDDLLockTimeout = 0;
    idvSQL * sStat = NULL;

    if ( aTrans != NULL )
    {
        IDE_ASSERT( aTrans->mTrans != NULL ); /* trans must be begun */
        sStat = ((smxTrans*)(aTrans->mTrans))->mStatistics;
        if (sStat != NULL )
        {
            IDE_ASSERT( sStat->mSess != NULL ); /* mSess is idvSession */
            if ( sStat->mSess->mSession != NULL ) /* mm session */
            {
                sDDLLockTimeoutProperty = gSmiGlobalCallBackList.getDDLLockTimeout(sStat->mSess->mSession);
            }
            else
            {
                /* internal session */
            }
        }
        else
        {
            /* internal transaction */
        }
    }
    else
    {
        /* aTrans is NULL */
    }

    if (sDDLLockTimeoutProperty == -1 )
    {
        sDDLLockTimeout = ID_ULONG_MAX;
    }
    else
    {
        sDDLLockTimeout = sDDLLockTimeoutProperty * 1000000;
    }

    return sDDLLockTimeout;
}

SInt smiGetDDLLockTimeOutProperty()
{
    return smuProperty::getDDLLockTimeOut();
}

IDE_RC smiWriteDummyLog()
{
    return smrLogMgr::writeDummyLog();
}


/*fix BUG-31514 While a dequeue rollback ,
  another dequeue statement which currenlty is waiting for enqueue event  might lost the  event  */
void   smiGetLastSystemSCN( smSCN *aLastSystemSCN )
{
    smSCN sSCN = smmDatabase::getLstSystemSCN();

    SM_GET_SCN(aLastSystemSCN, &sSCN);
}

/**********************************************************************
 * PROJ-2733 분산 트랜잭션 정합성
 **********************************************************************/
IDE_RC smiSetLastSystemSCN( smSCN * aLastSystemSCN, smSCN * aNewLastSystemSCN )
{
   return smmDatabase::setLstSystemSCN( aLastSystemSCN, aNewLastSystemSCN );
}

/**********************************************************************
 * PROJ-2733 분산 트랜잭션 정합성
 **********************************************************************/
void   smiInaccurateGetLastSystemSCN( smSCN *aLastSystemSCN )
{
    /* PROJ-2733 정확도 보다는 성능 하락 방지용 */
    smSCN sSCN = smmDatabase::inaccurateGetLstSystemSCN();

    SM_GET_SCN(aLastSystemSCN, &sSCN);
}

/* active transcation 중 가장 작은 SN을 리턴한다.
 * 만약 NULL인 경우 실제 로그에 쓰인 GSN을 리턴한다.
 * log가 계속 쓰이는 경우 GSN값이 변경될 수 있기때문에 가장 먼저 smiGetLastValidGSN을 호출한다.  */
smSN smiGetValidMinSNOfAllActiveTrans()
{
    smLSN sLSN;
    smSN  sMinSN = SM_SN_NULL;

    IDE_ASSERT( smiGetLastValidGSN( &sMinSN ) == IDE_SUCCESS );
    smxTransMgr::getMinLSNOfAllActiveTrans( &sLSN );
        
    if ( SM_MAKE_SN( sLSN ) < sMinSN )
    {
        sMinSN = SM_MAKE_SN( sLSN );
    }

    return sMinSN;
}

/***********************************************************************
 * Description : Retry Info를 초기화한다.

 *   aRetryInfo - [IN] 초기화 할 Retry Info
 ***********************************************************************/
void smiInitDMLRetryInfo( smiDMLRetryInfo * aRetryInfo )
{
    IDE_DASSERT( aRetryInfo != NULL );

    if ( aRetryInfo != NULL )
    {
        aRetryInfo->mIsWithoutRetry  = ID_FALSE;
        aRetryInfo->mIsRowRetry      = ID_FALSE;
        aRetryInfo->mStmtRetryColLst = NULL;
        aRetryInfo->mStmtRetryValLst = NULL;
        aRetryInfo->mRowRetryColLst  = NULL;
        aRetryInfo->mRowRetryValLst  = NULL;
    }
}

// PROJ-2264
const void * smiGetCompressionColumn( const void      * aRow,
                                      const smiColumn * aColumn,
                                      idBool            aUseColumnOffset,
                                      UInt            * aLength )
{
    smOID    sCompressionRowOID;
    UInt     sColumnOffset;
    smcTableHeader *sTableHeader;

    void  * sRet;

    IDE_ASSERT( aRow    != NULL );
    IDE_ASSERT( aColumn != NULL );

    IDE_DASSERT( (aColumn->flag & SMI_COLUMN_COMPRESSION_MASK)
                 == SMI_COLUMN_COMPRESSION_TRUE );

    if ( aUseColumnOffset == ID_TRUE )
    {
        sColumnOffset = aColumn->offset;
    }
    else
    {
        sColumnOffset = 0;
    }

    idlOS::memcpy( &sCompressionRowOID,
                   (SChar*)aRow + sColumnOffset,
                   ID_SIZEOF(smOID) );

    if ( sCompressionRowOID != SM_NULL_OID )
    {
        sRet = smiGetCompressionColumnFromOID( &sCompressionRowOID,
                                               aColumn,
                                               aLength );
    }
    else
    {
        sTableHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER( smiGetTable( aColumn->mDictionaryTableOID ) );

        sRet = smiGetCompressionColumnFromOID( &sTableHeader->mNullOID,
                                               aColumn,
                                               aLength );
    }
    return sRet;
}

// PROJ-2397
void * smiGetCompressionColumnFromOID( smOID           * aCompressionRowOID,
                                       const smiColumn * aColumn,
                                       UInt            * aLength )
{
    void      * sCompressionRowPtr  = NULL;
    void      * sRet                = NULL;
    smSCN       sCreateSCN          = SM_SCN_FREE_ROW;
    scSpaceID   sTableSpaceID       = 0;

    // PROJ-2429 Dictionary based data compress for on-disk DB
    if ( sctTableSpaceMgr::isDiskTableSpace( aColumn->colSpace ) == ID_TRUE )
    {
        sTableSpaceID = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
    }
    else
    {
        IDE_ASSERT( sctTableSpaceMgr::isMemTableSpace( aColumn->colSpace ) == ID_TRUE );

        sTableSpaceID = aColumn->colSpace;
    }

    IDE_ASSERT( smmManager::getOIDPtr( sTableSpaceID,
                                       *aCompressionRowOID,
                                       &sCompressionRowPtr )
                == IDE_SUCCESS );

    /* BUG-41686 insert 후 commit되지 않은 row가 restart 후 undo로 삭제된 다음
     * replication이 연결되어 insert에 대한 Xlog를 생성할 경우
     * compress가 걸려있는 column에 대한 smVCDesc가 초기화 되어 정상적인 값을 읽을 수 없다.  */

    sCreateSCN = ((smpSlotHeader*)sCompressionRowPtr)->mCreateSCN;
    if ( SM_SCN_IS_FREE_ROW( sCreateSCN ) == ID_TRUE )
    {
        sRet     = NULL;
        *aLength = 0;
    }
    else
    {
        // __FORCE_COMPRESSION_COLUMN = 1
        // infinite scn assert test, hidden property all comp + natc
        // IDE_ASSERT( SM_SCN_IS_INFINITE(sCreateSCN) == ID_TRUE);

        if ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_FIXED )
        {
            // BUG-38573 
            // FIXED type이지만 vcInOutBaseSize가 0이 아닌 경우는
            // Source Column이 VARIABLE인 Key Build Column을 의미한다.
            // VARIABLE type의 원래 value가 smVCDescInMode size 다음에 위치하므로
            // 실제 value를 가리키기 위해서는 offset을 아래와 같이 조정해야 한다.
            if ( aColumn->vcInOutBaseSize != 0 )
            {
                // VARIABLE type의 Source Column Value를 구하려고 할 때
                sRet = (void*)( (SChar*)sCompressionRowPtr + 
                                SMP_SLOT_HEADER_SIZE + 
                                ID_SIZEOF(smVCDescInMode) );
            }
            else
            {
                // FIXED type의 Source Column Value를 구하려고 하거나,
                // 현재 Column의 Value를 구하려고 할 때
                sRet = (void*)( (SChar*)sCompressionRowPtr + 
                                SMP_SLOT_HEADER_SIZE );
            }
            *aLength = aColumn->size;
        }
        else if ( SMI_IS_VARIABLE_COLUMN(aColumn->flag) || 
                  SMI_IS_VARIABLE_LARGE_COLUMN(aColumn->flag) )
        {
            *aLength = 0;

            sRet = sgmManager::getCompressionVarColumn(
                                        (SChar*)sCompressionRowPtr,
                                        aColumn,
                                        aLength );
        }
        else
        {
            IDE_ASSERT(0);
        }
    }

    return sRet;
}

const void * smiGetCompressionColumnLight( const void * aRow,
                                           scSpaceID    aColSpace,
                                           UInt         aVcInOutBaseSize,
                                           UInt         aFlag,
                                           UInt       * aLength )
{
    smiColumn sColumn;

    sColumn.colSpace = aColSpace;
    sColumn.flag     = aFlag;
    sColumn.offset   = 0;
    
    sColumn.vcInOutBaseSize = aVcInOutBaseSize;
    sColumn.value    = NULL;
    
    return smiGetCompressionColumn( aRow, 
                                    &sColumn, 
                                    ID_TRUE, // aUseColumnOffset
                                    aLength );
    
}

// PROJ-2264
void smiGetSmiColumnFromMemTableOID( smOID        aTableOID,
                                     UInt         aIndex,
                                     smiColumn ** aSmiColumn )
{
    SChar          * sOIDPtr;
    smcTableHeader * sTableHeader;
    smOID            sColumnOID;
    smiColumn      * sSmiColumn = NULL;

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aTableOID,
                                       (void**)&sOIDPtr )
                == IDE_SUCCESS );

    sTableHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(sOIDPtr);

    sSmiColumn = smcTable::getColumnAndOID( sTableHeader,
                                            aIndex,
                                            &sColumnOID );
    IDE_ASSERT( sSmiColumn != NULL );

    *aSmiColumn = sSmiColumn;
}

// PROJ-2264
idBool smiIsDictionaryTable( void * aTableHandle )
{
    UInt   sTableFlag;
    idBool sRet;

    IDE_ASSERT( aTableHandle != NULL );

    sTableFlag = (SMI_MISC_TABLE_HEADER(aTableHandle))->mFlag;

    if ( (sTableFlag & SMI_TABLE_DICTIONARY_MASK) == SMI_TABLE_DICTIONARY_TRUE )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

// PROJ-2264
void * smiGetTableRuntimeInfoFromTableOID( smOID aTableOID )
{
    void           * sOIDPtr;
    smcTableHeader * sTableHeader;
    void           * sRuntimeInfo;

    IDE_ASSERT( aTableOID != SM_NULL_OID );

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aTableOID,
                                       (void**)&sOIDPtr )
                == IDE_SUCCESS );

    sTableHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(sOIDPtr);

    sRuntimeInfo = sTableHeader->mRuntimeInfo;

    return sRuntimeInfo;
}

// PROJ-2402
IDE_RC smiPrepareForParallel( smiTrans * aTrans,
                              UInt     * aParallelReadGroupID )
{
    IDE_ERROR( aTrans               != NULL );
    IDE_ERROR( aParallelReadGroupID != NULL );

    *aParallelReadGroupID = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-2433
 * Description : memory btree index의 direct key 최소크기 (8).
 **********************************************************************/
UInt smiGetMemIndexKeyMinSize()
{
    return SMI_MEM_INDEX_MIN_KEY_SIZE;
}

/***********************************************************************
 * PROJ-2433
 * Description : memory btree index의 direct key 최대크기.
 *
 *               node에는 direct key 뿐만아니라
 *               node헤더, pointer(child,row)도 포함되므로
 *               간단히 node크기 / 3 으로 direct key 최대크기를 제한한다
 *
 *               ( __MEM_BTREE_NODE_SIZE의 값 / 3 )
 **********************************************************************/
UInt smiGetMemIndexKeyMaxSize()
{
    return ( smuProperty::getMemBtreeNodeSize() / 3 );
}

/***********************************************************************
 * PROJ-2433
 * Description : property __FORCE_INDEX_DIRECTKEY 값 반환.
 *               (테스트용 property)
 *              
 *               위 property가 설정되면 이후생성되는
 *               memory btree index는 모두 direct key index로 생성된다
 **********************************************************************/
idBool smiIsForceIndexDirectKey()
{
    return smuProperty::isForceIndexDirectKey();
}

/***********************************************************************
 * BUG-41121, BUG-41541
 * Description : property __FORCE_INDEX_PERSISTENCE 값 반환.
 *               (테스트용 property)
 *              
 *               해당 property의 값이 0일 경우 persistent index 기능은 사용되지 않는다.(기본값)
 *                                    1일 경우 persistent index로 세팅된 index만
 *                                            persistent index로 생성된다.
 *                                    2일 경우 이후 생성되는 모든 memory btree index는
 *                                            모두 persistent index로 생성된다.
 **********************************************************************/
UInt smiForceIndexPersistenceMode()
{
    return smuProperty::forceIndexPersistenceMode();
}

/***********************************************************************
 * PROJ-2462 Result Cache
 *
 * Description: Table Header의mSequence.mCurSequence
 * 값을 읽어온다. 이값은 Table이 Modify되었는지를
 * 판단할 때 사용된다.
 **********************************************************************/
void smiGetTableModifyCount( const void   * aTable,
                             SLong        * aModifyCount )
{
    smcTableHeader * sTableHeader = NULL;

    IDE_DASSERT( aTable       != NULL );
    IDE_DASSERT( aModifyCount != NULL );

    sTableHeader = ( smcTableHeader * )SMI_MISC_TABLE_HEADER( aTable );

    *aModifyCount = idCore::acpAtomicGet64( &sTableHeader->mSequence.mCurSequence );
}

IDE_RC smiWriteXaStartReqLog( ID_XID * aGlobalXID,
                              smTID    aTID,
                              smLSN  * aLSN )

{
    return smrUpdate::writeXaStartReqLog( aGlobalXID,
                                          aTID,
                                          aLSN );
}

IDE_RC smiWriteXaPrepareReqLog( ID_XID * aGlobalXID,
                                smTID    aTID,
                                UInt     aGlobalTxId,
                                UChar  * aBranchTxInfo,
                                UInt     aBranchTxInfoSize,
                                smLSN  * aLSN )

{
    return smrUpdate::writeXaPrepareReqLog( aGlobalXID,
                                            aTID,
                                            aGlobalTxId,
                                            aBranchTxInfo,
                                            aBranchTxInfoSize,
                                            aLSN );
}

IDE_RC smiWriteXaEndLog( smTID    aTID,
                         UInt     aGlobalTxId )

{
    return smrUpdate::writeXaEndLog( aTID,
                                     aGlobalTxId );
}

void smiSetGlobalTxId ( smTID   aTID,
                        UInt    aGlobalTxId )
{
    smxTrans * sTrans = smxTransMgr::getTransByTID( aTID );

    if ( sTrans->mTransID != aTID )
    {
        return;
    }

    if ( sTrans->mStatus == SMX_TX_END )
    {
        return;
    } 

    /* BUG-48703 : 최초값만 설정한다. */
    if ( sTrans->mGlobalTxId == SM_INIT_GTX_ID )
    {
        sTrans->mGlobalTxId = aGlobalTxId;
    }
}

IDE_RC smiSet2PCCallbackFunction( smGetDtxMinLSN aGetDtxMinLSN,
                                  smManageDtxInfo aManageDtxInfo )
{
    smrRecoveryMgr::set2PCCallback( aGetDtxMinLSN, aManageDtxInfo );

    return IDE_SUCCESS;
}

void smiSetTransactionalDDLCallback( smiTransactionalDDLCallback * aTransactionalDDLCallback )
{
    smiTrans::setTransactionalDDLCallback( aTransactionalDDLCallback );
}

/* PROJ-2626 Memory UsedSize 를 구한다. */
void smiGetMemUsedSize( ULong * aMemSize )
{
    ULong        sAlloc  = 0;
    ULong        sFree   = 0;
    smmTBSNode * sCurTBS = NULL;
    UInt         i       = 0;

    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while ( sCurTBS != NULL )
    {
        if ( ( sctTableSpaceMgr::isMemTableSpace( sCurTBS ) == ID_TRUE ) &&
             ( sctTableSpaceMgr::hasState( sCurTBS->mHeader.mID,
                                           SCT_SS_SKIP_BUILD_FIXED_TABLE ) == ID_FALSE ) )
        {
            sAlloc += ( ( ULong )sCurTBS->mMemBase->mAllocPersPageCount );

            for ( i = 0 ; i < sCurTBS->mMemBase->mFreePageListCount ; i++ )
            {
                sFree += ( ULong )sCurTBS->mMemBase->mFreePageLists[i].mFreePageCount;
            }
        }
        else
        {
            /* Nothing to do */
        }

        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }

    *aMemSize = ( sAlloc - sFree ) * SM_PAGE_SIZE;
}

/* PROJ-2626 Disk Undo UsedSize 를 구한다. */
IDE_RC smiGetDiskUndoUsedSize( ULong * aSize )
{
    sdpscDumpSegHdrInfo   sDumpSegHdr;
    sdpSegMgmtOp        * sSegMgmtOp   = NULL;
    sdcTXSegEntry       * sEntry       = NULL;
    UInt                  sTotEntryCnt = 0;
    ULong                 sTotal       = 0;
    UInt                  sSegPID      = 0;
    UInt                  i            = 0;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );

    IDE_ERROR( sSegMgmtOp != NULL );

    sTotEntryCnt = sdcTXSegMgr::getTotEntryCnt();

    for ( i = 0 ; i < sTotEntryCnt ; i++ )
    {
        sEntry  = sdcTXSegMgr::getEntryByIdx( i );
        sSegPID = sdcTXSegMgr::getTSSegPtr(sEntry)->getSegPID();

        // TSS Segment
        IDE_TEST( sdpscFT::dumpSegHdr( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       sSegPID,
                                       SDP_SEG_TYPE_TSS,
                                       sEntry,
                                       &sDumpSegHdr,
                                       NULL,
                                       NULL )
                  != IDE_SUCCESS );

        sTotal += sDumpSegHdr.mTxExtCnt;

        sSegPID = sdcTXSegMgr::getUDSegPtr(sEntry)->getSegPID();
        // Undo Segment
        IDE_TEST( sdpscFT::dumpSegHdr( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       sSegPID,
                                       SDP_SEG_TYPE_UNDO,
                                       sEntry,
                                       &sDumpSegHdr,
                                       NULL,
                                       NULL )
                  != IDE_SUCCESS );

        sTotal += sDumpSegHdr.mTxExtCnt + sDumpSegHdr.mUnExpiredExtCnt + sDumpSegHdr.mUnStealExtCnt;

        if ( sDumpSegHdr.mIsOnline == 'Y' )
        {
            sTotal += sDumpSegHdr.mExpiredExtCnt;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aSize = sTotal * smuProperty::getSysUndoTBSExtentSize();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* MM Callback을 SM에 등록하는 함수 */
IDE_RC smiSetSessionCallback( smiSessionCallback *aCallback )
{
    smxTransMgr::setSessionCallback( aCallback );

    return IDE_SUCCESS;
}

/******************************************************************************
 * PROJ-2733 분산 트랜잭션 정합성
 *******************************************************************************/
void smiSetReplicaSCN()
{
    smxTransMgr::setGlobalConsistentSCNAsSystemSCN();
}
